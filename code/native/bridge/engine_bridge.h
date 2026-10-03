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

// 公開するエントリ関数の名前。既定は `yaneuraou_start`。
//
// 評価関数のアーキテクチャ(halfkp_256x2-32-32 / halfkp_768x2-16-64 等)は
// YaneuraOu 本体にビルド時固定で焼き込まれるため、複数の評価関数へ対応するには
// **アーキテクチャごとに別のバイナリ**が要る(Issue #134/#282)。同一プロセスへ
// 2つ載せる以上、エントリ関数名が衝突してはいけないので、ビルド定義で
// `-DYANEURAOU_BRIDGE_SYMBOL=aobannue_start` のように差し替える。
//
// YaneuraOu 本体の内部シンボルは重複するが、Android は別々の .so、iOS は
// `use_frameworks!` による別々の framework として読み込まれ、それぞれ独立した
// シンボル名前空間を持つため衝突しない。
#ifndef YANEURAOU_BRIDGE_SYMBOL
#define YANEURAOU_BRIDGE_SYMBOL yaneuraou_start
#endif

#ifdef __cplusplus
extern "C" {
#endif

// エンジンスレッドを起動し、ip:port へ TCP 接続して USI ループを回す。
// 標準入出力の代わりにそのソケットで USI テキストを送受信する。
// この関数自体は即座に返る(呼び出し元をブロックしない)。
// 前回起動したスレッドがまだ終了していない場合、実際のエンジン起動は
// バックグラウンドでその終了を待ってから行われる(YaneuraOu 内部の
// グローバル状態を複数スレッドが同時に初期化しないようにするため)。
// 通常は呼び出し側が先にソケットを閉じて USI::loop() を終了させておくので、
// この待ちは短い。前のスレッドが一定時間(engine_bridge.cpp の
// kPreviousThreadJoinTimeout)以内に終了しない場合は、待たずに detach して
// 新しいエンジンの起動を進める(前のスレッドはリークするが、以後の
// yaneuraou_start() 呼び出しを永久にブロックさせないため)。
// 返り値: 0 = 起動成功(接続の成否はソケット側で観測する)。
//
// iOS では静的リンクされ、Dart から DynamicLibrary.executable() の dlsym で
// 解決する。リンカのデッドストリップで消えないよう used/visibility を付ける。
__attribute__((visibility("default"), used)) int YANEURAOU_BRIDGE_SYMBOL(
    const char* ip,
    int port);

#ifdef __cplusplus
}
#endif

#endif  // YANEURAOU_ENGINE_BRIDGE_H
