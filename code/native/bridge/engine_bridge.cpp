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
#include <streambuf>
#include <string>
#include <thread>

// YaneuraOu 本体。include ディレクトリは source/。
#include "bitboard.h"
#include "misc.h"
#include "position.h"
#include "search.h"
#include "thread.h"
#include "tt.h"
#include "usi.h"
#include "evaluate.h"

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
void run_engine(int fd) {
  SocketOutStreambuf out(fd);
  SocketInStreambuf in(fd);
  std::cout.rdbuf(&out);
  std::cin.rdbuf(&in);

  // main.cpp 準拠の初期化。argv[0] はダミーで良い。
  const char* prog = "yaneuraou";
  char* argv[] = {const_cast<char*>(prog), nullptr};
  int argc = 1;

  CommandLine::init(argc, argv);
  USI::init(Options);
  Bitboards::init();
  Position::init();
  Search::init();
  const size_t thread_num =
      Options.count("Threads") ? static_cast<size_t>(Options["Threads"]) : 1;
  Threads.set(thread_num);
  Eval::init();

  USI::loop(argc, argv);

  Threads.set(0);
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

}  // namespace

extern "C" int yaneuraou_start(const char* ip, int port) {
  std::thread(engine_thread, std::string(ip), port).detach();
  return 0;
}
