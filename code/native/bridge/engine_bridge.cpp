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

#include <atomic>
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

// ---------------------------------------------------------------------------
// std::cin / std::cout の差し替え(Issue #464 / #529)
//
// std::cin / std::cout はプロセスに1つしかない。iOS では libc++ がシステムの
// dylib なので、水匠5・AobaNNUE・つようら王の3つの framework も同じ
// std::cin / std::cout を使う(-fvisibility=hidden で分かれるのは YaneuraOu
// 本体のシンボルだけ)。一方、エンジンスレッドの管理(下の g_engine_thread)は
// framework ごとに別々なので、別の評価関数へ切り替えたときは前のエンジンの
// 終了を一切待たずに次のエンジンが始まる。同じ framework でも、前のエンジンが
// kPreviousThreadJoinTimeout 以内に終わらなければ detach して次を始める。
//
// 以前は run_engine() のローカル変数の streambuf を std::cin / std::cout に
// 差し込んでいた。そのため、前のエンジンがまだ動いているうちに次のエンジンが
// 差し替えると、前のエンジンの探索スレッドや USI ループが次のエンジンの
// streambuf を使い、次のエンジンが終わってその streambuf(スレッドのスタック)
// が消えたあとに読み書きして EXC_BAD_ACCESS になっていた
// (#529: on_update_full の info 出力中、#464: StandardInput::input の getline 中)。
// 前のエンジンが次のエンジンのソケットからコマンドを横取りすることもあった。
//
// そこで:
// - std::cin / std::cout に差し込む streambuf は framework ごとに1組だけ作り、
//   破棄しない(どのスレッドがいつ触っても解放済みメモリにならない)。
// - 接続(Connection)もヒープに置いて破棄しない。fd を閉じるのは、その接続へ
//   書き込み中のスレッドがいなくなってから(閉じた fd 番号が別のファイルに
//   再利用され、そこへ書いてしまうのを防ぐ)。
// - 入力は、その接続の USI ループを回しているスレッド(このファイルの
//   engine_thread)だけがソケットから読む。それ以外のスレッド(前のエンジン、
//   別の framework のエンジン)が読もうとしたら "quit" を返して終わらせる。
//   ソケットの切断(EOF)も "quit" の行として返し、共有の std::cin に
//   eofbit/failbit を立てない(立つと、同時に動いている次のエンジンの
//   getline まで失敗して "quit" 扱いになる)。
// - 出力は、USI ループのスレッドなら自分の接続へ、それ以外(探索スレッド)なら
//   この framework で今動いている接続へ送る。どの接続にも属さない出力は捨てる。
// ---------------------------------------------------------------------------

// ソケット1本分。ヒープに確保し、意図的に破棄しない(上のコメント参照)。
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
//     この恒久的沈黙を防ぐため、write() が失敗した場合はソケットを
//     shutdown(SHUT_RDWR) して明示的に切断する。close() と異なり
//     shutdown() は「別スレッドが read() でブロックしている」状態を
//     安全に(Linux/Darwin いずれでも)解除できるため、USI コマンド受信
//     ループ側の read() が即座に失敗/EOF を返して USIEngine::loop() が
//     自然終了する。これにより Dart 側は(無応答のまま [_handshakeTimeout]
//     いっぱい待たされる代わりに)ソケット切断([EngineProcess.lines] の
//     onDone)を即座に検知でき、[EngineController] が [EngineStatus.error]
//     へ遷移する(native/README.md・lib/core/engine/manager/engine_controller.dart
//     参照)。この対応単体では自動再起動までは行わない(呼び出し元が
//     [EngineStatus.error] や [EngineController.checkHealthy] の結果を見て
//     再起動する設計は lib/features/analysis 側に既存)。
class Connection {
 public:
  explicit Connection(int fd) : fd_(fd) {}

  // n バイトを書く。書けた、または接続が既に閉じていて捨てたときは true。
  bool write(const char* s, size_t n) {
    users_.fetch_add(1);
    bool ok = true;
    if (!closed_.load()) ok = write_all(s, n);
    users_.fetch_sub(1);
    return ok;
  }

  // 1バイト読む。切断・エラーなら false。この接続の USI ループのスレッド
  // だけが呼ぶ(finish() と同じスレッドなので、閉じた fd を読むことはない)。
  bool read_char(char& ch) {
    if (closed_.load()) return false;
    return ::read(fd_, &ch, 1) == 1;
  }

  // 接続を終わらせる。USI ループを抜けたあと、この接続のスレッドが呼ぶ。
  // 書き込み中のスレッドがいなくなってから fd を閉じる。
  void finish() {
    ::shutdown(fd_, SHUT_RDWR);  // ブロック中の write() もこれで戻る。
    closed_.store(true);
    // shutdown 後の write() はすぐ失敗するので、普通は待たずに抜ける。
    // 念のため上限を設け、待ちきれなければ fd を閉じずに残す(リークは
    // fd 1つ。閉じた番号が再利用されて別のファイルへ書くよりよい)。
    const auto deadline = std::chrono::steady_clock::now() + kDrainTimeout;
    while (users_.load() != 0) {
      if (std::chrono::steady_clock::now() > deadline) return;
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    ::close(fd_);
  }

 private:
  static constexpr std::chrono::seconds kDrainTimeout{15};

  // 相手が閉じたソケットへ書いても SIGPIPE でプロセスごと落とさない
  // (EPIPE として失敗させる)。前のエンジンは自分の(Dart 側が閉じた)接続へ
  // 書くことがあるため。Linux/Android は send() の MSG_NOSIGNAL、Darwin は
  // set_socket_options() の SO_NOSIGPIPE で止める。
#ifdef MSG_NOSIGNAL
  static constexpr int kSendFlags = MSG_NOSIGNAL;
#else
  static constexpr int kSendFlags = 0;
#endif

  bool write_all(const char* s, size_t n) {
    size_t off = 0;
    while (off < n) {
      const ssize_t w = ::send(fd_, s + off, n - off, kSendFlags);
      if (w <= 0) {
        // 書き込み失敗(kSendTimeout到達を含む)。以後この接続で
        // まともな双方向通信は成立しないとみなし、明示的に切断する
        // (上記コメント参照)。shutdown() は何度呼んでも安全。
        if (!shutdown_called_.exchange(true)) ::shutdown(fd_, SHUT_RDWR);
        return false;
      }
      off += static_cast<size_t>(w);
    }
    return true;
  }

  const int fd_;
  std::atomic<int> users_{0};
  std::atomic<bool> closed_{false};
  std::atomic<bool> shutdown_called_{false};
};

// この framework で今動いているエンジンの接続。無ければ nullptr。
std::atomic<Connection*> g_current{nullptr};

// このスレッドが USI ループを回している接続(engine_thread の中だけ非 null)。
// framework ごとに別の変数なので、別の framework のエンジンのスレッドからは
// nullptr に見える。
thread_local Connection* t_connection = nullptr;

// std::cout に差し込む streambuf。framework ごとに1つで、破棄しない。
// バッファを持たず、書くたびに接続へ送る(以前と同じ)。
class RoutingOutStreambuf : public std::streambuf {
 protected:
  int overflow(int c) override {
    if (c == EOF) return 0;
    const char ch = static_cast<char>(c);
    return put(&ch, 1) ? c : EOF;
  }
  std::streamsize xsputn(const char* s, std::streamsize n) override {
    return put(s, static_cast<size_t>(n)) ? n : 0;
  }

 private:
  // false を返すと std::cout に badbit が立つ(#266 の切断検知に使う)。
  // 立てるのは、今動いている接続への書き込みに失敗したときだけにする。
  // 前のエンジンの書き込み失敗で、共有の std::cout を止めないため。
  static bool put(const char* s, size_t n) {
    Connection* current = g_current.load();
    Connection* target = t_connection != nullptr ? t_connection : current;
    if (target == nullptr) return true;  // どの接続にも属さない出力は捨てる。
    if (target->write(s, n)) return true;
    return target != current;
  }
};

// std::cin に差し込む streambuf。framework ごとに1つで、破棄しない。
//
// get area(setg)は使わない。複数のスレッドが同時に getline しても、共有の
// ポインタがどれかのスレッドのメモリを指すことがないようにするため
// (#464 は getline が get area を memchr で走査している最中に落ちていた)。
// 先読みした1文字や、合成した "quit" の残りはスレッドごとに持つ。
class RoutingInStreambuf : public std::streambuf {
 protected:
  int_type underflow() override {
    State& st = state();
    if (!st.has_peek) {
      st.peek = next_char(st);
      st.has_peek = true;
    }
    return st.peek;
  }
  int_type uflow() override {
    State& st = state();
    if (st.has_peek) {
      st.has_peek = false;
      return st.peek;
    }
    return next_char(st);
  }
  std::streamsize showmanyc() override { return 0; }

 private:
  struct State {
    bool has_peek = false;
    int_type peek = 0;
    const char* synthetic = nullptr;  // 合成中の "quit\n" の残り
    bool at_line_start = true;
  };

  static State& state() {
    thread_local State st;
    return st;
  }

  static int_type next_char(State& st) {
    if (st.synthetic == nullptr) {
      Connection* conn = t_connection;
      char ch;
      if (conn != nullptr && conn == g_current.load() && conn->read_char(ch)) {
        st.at_line_start = ch == '\n';
        return traits_type::to_int_type(ch);
      }
      // 切断・前のエンジン・別の framework のスレッド: "quit" の行を返して
      // USI ループを終わらせる(EOF を返すと共有の std::cin に eofbit/failbit
      // が立つ。上のコメント参照)。行の途中なら、まず改行で区切る。
      st.synthetic = st.at_line_start ? "quit\n" : "\nquit\n";
    }
    const char ch = *st.synthetic++;
    if (*st.synthetic == '\0') st.synthetic = nullptr;
    st.at_line_start = ch == '\n';
    return traits_type::to_int_type(ch);
  }
};

// 破棄しない(exit() 時のデストラクタで、まだ動いているスレッドから
// 触られないように。Issue #444 と同じ考え方)。
RoutingOutStreambuf& g_out = *new RoutingOutStreambuf;
RoutingInStreambuf& g_in = *new RoutingInStreambuf;

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
void run_engine(Connection* conn) {
  t_connection = conn;
  g_current.store(conn);
  // rdbuf() の差し替えは、ストリームの状態(前の接続での書き込み失敗による
  // badbit 等)のクリアも兼ねる。
  std::cout.rdbuf(&g_out);
  std::cin.rdbuf(&g_in);

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

  // ここに来たときには、このエンジンの探索スレッドは終わっている
  // (USIEngine::loop() の最後で探索の終了を待ち、ThreadPool の破棄で join
  // する。native/patches/0003 参照)。std::cin / std::cout の streambuf は
  // 差し込んだまま残す(破棄しないので、残っていても安全)。
  Connection* expected = conn;
  g_current.compare_exchange_strong(expected, nullptr);
  t_connection = nullptr;
}

// ソケットへの書き込みに送信タイムアウトを設定する(あわせて SIGPIPE を止める)。
//
// USI の出力はすべて `sync_cout << ... << sync_endl` (source/misc.cpp) 経由で
// 行われ、この操作は書き込み中 framework 内で共有される `static mutex` を
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

void set_socket_options(int fd) {
  const auto sec = static_cast<time_t>(kSendTimeout.count());
  timeval tv{};
  tv.tv_sec = sec;
  tv.tv_usec = 0;
  ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#ifdef SO_NOSIGPIPE
  // Darwin: 相手が閉じたソケットへの書き込みで SIGPIPE を出さない
  // (Connection::kSendFlags のコメント参照)。
  const int on = 1;
  ::setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &on, sizeof(on));
#endif
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

  set_socket_options(fd);
  // 破棄しない(前のエンジンや別の framework のスレッドが、終わったあとの
  // 接続を参照しうるため。冒頭のコメント参照)。1回の起動で数十バイト。
  Connection* conn = new Connection(fd);
  run_engine(conn);
  conn->finish();
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
// 直接データ競合することは基本的にない。std::cin / std::cout は共有だが、
// 冒頭のとおり差し込む streambuf と接続を破棄しないので、解放済みメモリに
// は触れない。前のエンジンの USI ループは次に入力を読んだ時点で "quit" を
// 受け取って終わる(探索スレッドの出力は、終わるまでの間だけ新しい接続へ
// 混ざりうる)。ただし `sync_cout`/`sync_endl`
// (source/misc.cpp) が使う出力直列化用の `static mutex` は framework 内で
// 共有されており、detach したスレッドがまさにその mutex を保持したまま
// 停止した場合は新しいエンジンの出力もブロックしうる。これは上記の
// 送信タイムアウト(kSendTimeout)で「保持したまま停止する」こと自体を
// 防止することで軽減している。
constexpr std::chrono::seconds kPreviousThreadJoinTimeout{5};

std::mutex g_engine_thread_mutex;
// ヒープに確保し、意図的に破棄しない(Issue #444)。エンジンスレッドは
// アプリの生存中ずっと joinable のまま保持されるため、通常の static 変数に
// すると exit() 時のデストラクタで std::terminate() になる(iOS で終了時の
// watchdog タイムアウト後に exit() が走り、SIGABRT として記録されていた)。
// プロセス終了時はスレッドごと OS が片付けるので、join も detach も要らない。
std::thread& g_engine_thread = *new std::thread;
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
