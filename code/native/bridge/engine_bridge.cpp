// engine_bridge の実装(段階2: 実 YaneuraOu)。
//
// iOS/Android では別プロセスを起動できないため、エンジンを同一プロセス内で走らせる。
// 参照実装 YaneuraOuiOSSPM(ios_main.cpp)に倣い、エンジンスレッドで
// std::cin/std::cout をローカル TCP ソケットの streambuf に差し替え、YaneuraOu の
// 通常の USI ループ(標準入出力ベース)をそのまま回す。Dart 側はそのソケットへ
// USI テキストを送受信するだけでよい。
//
// 初期化順序は YaneuraOu 本体 source/main.cpp に準拠。

#include "engine_bridge.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <future>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <string>
#include <thread>

// YaneuraOu 本体。include ディレクトリは source/。
#include "bitboard.h"
#include "misc.h"
#include "position.h"
#include "types.h"

// V9.00 では YaneuraOu 本体のシンボルはすべて namespace YaneuraOu 配下に
// 移動した(Stockfish由来の大規模リファクタ)。
using namespace YaneuraOu;

namespace {

// ソケット fd への出力 streambuf。std::cout の rdbuf を差し替える。
//
// 🩹 shogi-vault独自対応(Issue #266、実機ログ調査): write() が
//     kSendTimeout(下記)で失敗した場合、その失敗は std::cout
//     (std::ostream)側に「一度立ったら誰も clear() しない限り永久に残る」
//     failbit/badbit として記録される。以後この std::cout への出力
//     (sync_cout 経由の "info"/"bestmove" を含む全出力)は sentry の
//     構築に失敗して無条件でスキップされるようになる。このコードベースは
//     どこでも std::cout.clear() を呼ばないため、たった一度の一時的な
//     書き込み詰まりが「エンジンのプロセス自体は生きているが、以後
//     bestmove を含め誰にも何も応答しなくなる」という恒久的な沈黙に
//     直結してしまう(実機ログでの再現: リアルタイムAI解析中に
//     「次の一手」ボタンを連打すると、EngineController.stop() が
//     bestmove を[_handshakeTimeout](30秒)待っても受信できない)。
//
//     実機でのみ顕在化した理由(仮説): kSendTimeout に到達するには、
//     ソケットの送信バッファが埋まった状態が10秒間続く必要がある。
//     シミュレータ(macOSホストの潤沢なCPU/カーネルネットワークスタック)
//     では、Dart側の受信ループがこれほど長く滞留することはまず無いが、
//     実機ではリアルタイム解析中の連打で stop→position→go が高頻度に
//     発生し、大量の info 行出力と Dart 側 UI の再描画(ボタン連打による
//     再ビルド)が競合してDart側の受信処理が遅延すると、この経路を
//     踏みうる。
//
//     この恒久的沈黙を防ぐため、write() が失敗した場合はソケットを
//     shutdown(SHUT_RDWR) して明示的に切断する。close() と異なり
//     shutdown() は「別スレッドが SocketInStreambuf::underflow() の
//     read() でブロックしている」状態を安全に(Linux/Darwin いずれでも)
//     解除できるため、USI コマンド受信ループ側の read() が即座に
//     失敗/EOF を返して USIEngine::loop() が自然終了する。これにより
//     Dart 側は(無応答のまま [_handshakeTimeout] いっぱい待たされる
//     代わりに)ソケット切断([EngineProcess.lines] の onDone)を即座に
//     検知でき、[EngineController] が [EngineStatus.error] へ遷移する
//     (native/README.md・lib/core/engine/manager/engine_controller.dart
//     参照)。「出力だけが壊れていて応答できない」半死状態を、既存の
//     異常終了検知経路に載せ替えることが目的であり、この対応単体では
//     自動再起動までは行わない(呼び出し元が [EngineStatus.error] や
//     [EngineController.checkHealthy] の結果を見て再起動する設計は
//     lib/features/analysis 側に既存)。
class SocketOutStreambuf : public std::streambuf {
 public:
  explicit SocketOutStreambuf(int fd) : fd_(fd) {}

 protected:
  int overflow(int c) override {
    if (c == EOF) return c;
    const char ch = static_cast<char>(c);
    return write_all(&ch, 1) ? c : EOF;
  }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    return write_all(s, static_cast<size_t>(n)) ? n : 0;
  }

 private:
  bool write_all(const char* s, size_t n) {
    size_t off = 0;
    while (off < n) {
      const ssize_t w = ::write(fd_, s + off, n - off);
      if (w <= 0) {
        // 書き込み失敗(kSendTimeout到達を含む)。以後この接続で
        // まともな双方向通信は成立しないとみなし、明示的に切断する
        // (上記コメント参照)。
        shutdown_once();
        return false;
      }
      off += static_cast<size_t>(w);
    }
    return true;
  }

  // 書き込み失敗を検知した際に一度だけソケットを切断する。
  // shutdown() 自体は複数回呼んでも安全(2回目以降は ENOTCONN 等で
  // 単に失敗するだけ)だが、無駄な syscall を避けるためフラグで抑制する。
  void shutdown_once() {
    if (shutdown_called_) return;
    shutdown_called_ = true;
    ::shutdown(fd_, SHUT_RDWR);
  }

  int fd_;
  bool shutdown_called_ = false;
};

// ソケット fd からの入力 streambuf。std::cin の rdbuf を差し替える。
class SocketInStreambuf : public std::streambuf {
 public:
  explicit SocketInStreambuf(int fd) : fd_(fd) {}

 protected:
  int underflow() override {
    const ssize_t n = ::read(fd_, &ch_, 1);
    if (n <= 0) return EOF;  // 切断で USI ループが終了する。
    setg(&ch_, &ch_, &ch_ + 1);
    return static_cast<unsigned char>(ch_);
  }

 private:
  int fd_;
  char ch_ = 0;
};

// YaneuraOu を初期化し、ソケットを標準入出力に見立てて USI ループを回す。
//
// V9.00 の source/main.cpp に準拠した初期化順序(v8.60git からの変更点):
// V9.00 は Stockfish 本家に合わせた大規模リファクタが入っており、`Options`/
// `Threads`/`Search::init`/`Eval::init` のようなグローバル関数・オブジェクトは
// 廃止された。エンジン実体は `IEngine`/`Engine` を継承する具象クラス
// (本アプリの構成では `YaneuraOuEngine`。source/engine/yaneuraou-engine/
// yaneuraou-search.cpp)が持つメンバに変わり、`Options.add("Threads", ...)`
// のようなオプション登録・`Threads.set(...)` によるスレッドプール構築・
// `Eval::init()` 相当の評価関数初期化はすべてそのクラスのコンストラクタ/
// isready ハンドラ内で行われるようになった。
//
// 本体をどのエンジン実体で動かすかは、リンクした .cpp が static
// `EngineFuncRegister` でエントリポイントを自己登録する仕組みに変わっている
// (source/engine.cpp の `run_engine_entry()` が登録済みのエンジンのうち
// priority 最大のものを起動する)。本アプリは
// `engine/yaneuraou-engine/yaneuraou-search.cpp` の1つしかリンクしないため、
// 常に `YaneuraOuEngine` が起動する。
//
// そのため、main.cpp と同じ手順(CommandLine::g.set_arg → Bitboards::init →
// Position::init → run_engine_entry)を踏むだけでよく、USIEngine/Options/
// Threads を直接操作する必要はなくなった(README/CHANGELOG も参照)。
// `run_engine_entry()` は内部の `USIEngine::loop()` が "quit" を受け取るまで
// 戻らない(=関数呼び出しがブロックする)。
void run_engine(int fd) {
  SocketOutStreambuf out(fd);
  SocketInStreambuf in(fd);
  std::cout.rdbuf(&out);
  std::cin.rdbuf(&in);

  // main.cpp 準拠の初期化。argv[0] はダミーで良い。
  const char* prog = "yaneuraou";
  char* argv[] = {const_cast<char*>(prog), nullptr};
  int argc = 1;

  CommandLine::g.set_arg(argc, argv);

  Bitboards::init();
  Position::init();

  // 登録済みエンジン(YaneuraOuEngine)の entry point を起動。"quit" が
  // 来るまでブロックする。
  run_engine_entry();
}

// ソケットへの書き込みに送信タイムアウトを設定する。
//
// USI の出力はすべて `sync_cout << ... << sync_endl` (source/misc.cpp) 経由で
// 行われ、この操作は書き込み中プロセス全体で共有される `static mutex` を
// ロックしたまま実際の write() を行う。もし Dart 側がソケットを読み切らず
// (アプリのバックグラウンド遷移でイベントループが滞留する等)、カーネルの
// 送信バッファが埋まった状態が続くと、write() がタイムアウトなく永久に
// ブロックし、その間ロックされたままの mutex が原因で「以後どのエンジン
// スレッドも sync_cout を呼んだ瞬間に道連れでブロックする」という、
// join タイムアウト(下記)だけでは救えない二次的なデッドロックに発展し
// うる。これを防ぐため、書き込み側にのみ送信タイムアウトを設定する
// (読み込み側は「次のコマンドが来るまで無期限に待つ」のが正しい動作の
// ため、タイムアウトを設定しない=EAGAIN を EOF と誤認させない)。
constexpr std::chrono::seconds kSendTimeout{10};

void set_send_timeout(int fd) {
  const auto sec = static_cast<time_t>(kSendTimeout.count());
  timeval tv{};
  tv.tv_sec = sec;
  tv.tv_usec = 0;
  ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

void engine_thread(std::string ip, int port, std::promise<void> finished) {
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    finished.set_value();
    return;
  }

  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(port));
  if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
    ::close(fd);
    finished.set_value();
    return;
  }
  if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    ::close(fd);
    finished.set_value();
    return;
  }

  set_send_timeout(fd);
  run_engine(fd);
  ::close(fd);
  // run_engine() から戻った(= "quit" を受けて USI::loop を抜けた、または
  // ソケットが切断された)ことを、次回 yaneuraou_start() 呼び出し時の
  // join 待ちに通知する。
  finished.set_value();
}

// 直近に起動したエンジンスレッド。Threads/Options/std::cin・cout は
// YaneuraOu 内部でプロセス全体を通した単一のグローバル状態であり、複数の
// run_engine() を同時に走らせる設計にはなっていない。前のスレッドが
// USI::loop() を抜けきる前に次のスレッドがそれらを再初期化すると、前の
// スレッドが参照中の Position/Thread が壊され、EXC_BAD_ACCESS 等の
// メモリ破壊を招く。そのため起動のたびに前のスレッドの終了を待ってから
// 新しいエンジン本体を走らせる。
//
// 前のスレッドの join は、そのスレッドを生成する側(下記の新しいラッパー
// スレッド)の中で行う。yaneuraou_start() 自身(呼び出し元である Dart の
// FFI 呼び出しスレッド、モバイルではメイン/UI スレッド)をここで
// ブロックしてしまうと、前のスレッドが万一終了しない場合にアプリ全体が
// 無期限にフリーズする。そのため yaneuraou_start() は常に即座に返し、
// join 待ちはバックグラウンドのラッパースレッド側で行う。
//
// previous.join() 自体にはタイムアウトがない(std::thread は timed_join を
// 持たない)。前のスレッドが USI::loop() から戻ってこない場合(Android の
// バックグラウンド時のプロセスフリーズに伴うソケット状態異常や、V9.00 の
// Stockfish 由来 ThreadPool 再構築で stop→quit を間髪入れず送った際の
// シャットダウン競合などが疑われる)、join() が永久に返らず、以後
// yaneuraou_start() を呼んでも新しいエンジンスレッドが二度と起動できなく
// なる(g_engine_finished を介して連鎖的に次の join もブロックし続ける
// ため)。これを避けるため、前のスレッドの終了を `finished` の
// shared_future で待ち、一定時間で見切りをつけて join せずに detach する
// (スレッドはリークするが、新しいエンジンの起動をブロックしない)。
//
// detach したスレッドを放置してよいかは V9.00 のリファクタ内容に依存する。
// `run_engine_entry()` (source/engine.cpp) が起動する `YaneuraOuEngine`
// (source/engine/yaneuraou-engine/yaneuraou-search.cpp) は Engine を
// 継承したローカルインスタンスであり、Threads/Options/局面はすべて
// そのインスタンスのメンバであるため、新旧のエンジンインスタンス同士が
// 直接データ競合することは基本的にない。ただし `sync_cout`/`sync_endl`
// (source/misc.cpp) が使う出力直列化用の `static mutex` はプロセス全体で
// 共有されており、detach したスレッドがまさにその mutex を保持したまま
// 停止した場合は新しいエンジンの出力もブロックしうる。これは上記の
// 送信タイムアウト(kSendTimeout)で「保持したまま停止する」こと自体を
// 防止することで軽減している。
constexpr std::chrono::seconds kPreviousThreadJoinTimeout{5};

std::mutex g_engine_thread_mutex;
std::thread g_engine_thread;
// 直近に起動したエンジンスレッドの終了通知。std::promise は複数箇所から
// 待てないため、待つ側では get_future().share() した shared_future を保持
// する。
std::shared_future<void> g_engine_finished;

void start_engine_thread(std::thread previous,
                          std::shared_future<void> previous_finished,
                          std::string ip, int port,
                          std::promise<void> finished) {
  if (previous.joinable()) {
    const bool exited = previous_finished.valid() &&
                         previous_finished.wait_for(
                             kPreviousThreadJoinTimeout) ==
                             std::future_status::ready;
    if (exited) {
      // 既に run_engine() から戻っている(=OS スレッドの実行はほぼ完了して
      // いる)ので、ここでの join() はブロックしない。
      previous.join();
    } else {
      // 前のスレッドがタイムアウト以内に終了しなかった。スタックしている
      // とみなし、待たずに detach して今回のエンジン起動を進める。
      previous.detach();
    }
  }
  engine_thread(std::move(ip), port, std::move(finished));
}

}  // namespace

extern "C" int YANEURAOU_BRIDGE_SYMBOL(const char* ip, int port) {
  std::lock_guard<std::mutex> lock(g_engine_thread_mutex);
  std::thread previous = std::move(g_engine_thread);
  std::shared_future<void> previous_finished = g_engine_finished;

  std::promise<void> finished;
  g_engine_finished = finished.get_future().share();

  g_engine_thread =
      std::thread(start_engine_thread, std::move(previous), previous_finished,
                  std::string(ip), port, std::move(finished));
  return 0;
}
