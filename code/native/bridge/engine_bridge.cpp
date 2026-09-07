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
#include <unistd.h>

#include <cstring>
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
      if (w <= 0) return false;
      off += static_cast<size_t>(w);
    }
    return true;
  }
  int fd_;
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

void engine_thread(std::string ip, int port) {
  const int fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) return;

  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(static_cast<uint16_t>(port));
  if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
    ::close(fd);
    return;
  }
  if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    ::close(fd);
    return;
  }

  run_engine(fd);
  ::close(fd);
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
std::mutex g_engine_thread_mutex;
std::thread g_engine_thread;

void start_engine_thread(std::thread previous, std::string ip, int port) {
  if (previous.joinable()) previous.join();
  engine_thread(std::move(ip), port);
}

}  // namespace

extern "C" int yaneuraou_start(const char* ip, int port) {
  std::lock_guard<std::mutex> lock(g_engine_thread_mutex);
  std::thread previous = std::move(g_engine_thread);
  g_engine_thread =
      std::thread(start_engine_thread, std::move(previous), std::string(ip), port);
  return 0;
}
