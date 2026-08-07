// 組込みエンジンの公開 C ABI。
//
// iOS/Android では別プロセスを起動できないため、エンジンをアプリに静的リンク/
// 同梱し、同一プロセス内で起動する。参照実装(YaneuraOuiOSSPM)に倣い、エンジンは
// 自身の std::cin/std::cout をローカル TCP ソケットに差し替えて通常の USI ループを
// 回す。よって公開関数は起動 1 つで足りる。
//
// 設計: ../README.md, ../../docs/ios_engine_integration_research.md §4.4

#ifndef YANEURAOU_ENGINE_BRIDGE_H
#define YANEURAOU_ENGINE_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// エンジンスレッドを起動し、ip:port へ TCP 接続して USI ループを回す。
// 標準入出力の代わりにそのソケットで USI テキストを送受信する。
// スレッドは detach され、この関数はすぐ返る。
// 返り値: 0 = 起動成功(接続の成否はソケット側で観測する)。
//
// iOS では静的リンクされ、Dart から DynamicLibrary.executable() の dlsym で
// 解決する。リンカのデッドストリップで消えないよう used/visibility を付ける。
__attribute__((visibility("default"), used)) int yaneuraou_start(const char* ip,
                                                                 int port);

#ifdef __cplusplus
}
#endif

#endif  // YANEURAOU_ENGINE_BRIDGE_H
