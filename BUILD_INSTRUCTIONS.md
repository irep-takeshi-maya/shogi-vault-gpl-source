# ビルド手順

## 概要

やねうら王を、Android(`.so`, NDK/CMake)と iOS(静的リンク, CocoaPods)向けにビルドする
手順です。評価関数は実行時ロード(USI `EvalDir`)で読み込み、ビルド時には埋め込みません。

評価関数のアーキテクチャごとに、**3本のエンジンバイナリ**をビルドします。やねうら王の
ソースは2つの版を使い分けており、つようら王11mini 用のバイナリだけ別の版です
(shogi-vault Issue #346)。

| ターゲット | 評価関数 | アーキテクチャ | やねうら王の版 | 当てる patch | 追加の定義 |
|---|---|---|---|---|---|
| `yaneuraou` | 水匠5(同梱) | halfkp_256x2-32-32(`EVAL_NNUE_HALFKP256`) | V9.00 `a5ee2786` | `patches/*.patch` | — |
| `aobannue` | AobaNNUE(アプリ内DL) | halfkp_768x2-16-64 | V9.00 `a5ee2786` | `patches/*.patch` | — |
| `tsuyoura` | つようら王11mini(アプリ内DL、Pro) | SFNN_ka2_4096_8_64_progress8 | 9.80git `c1b80eaa` | `patches/tsuyoura/*.patch` | `SFNNwoPSQT` |

## 前提条件

- `code/native/` を実際のアプリの `native/` として配置すること(以下のコマンドは
  本リポジトリのルートで実行する例)。
- Android: NDK, CMake 3.22 以降
- iOS: Xcode, CocoaPods

## 1. やねうら王のソース(2つ)を取得し、patch を当てる

[`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md) の 1. と同じ内容です。

```bash
# V9.00(水匠5・AobaNNUE 用)
git clone https://github.com/yaneurao/YaneuraOu.git code/native/third_party/YaneuraOu
git -C code/native/third_party/YaneuraOu checkout a5ee2786c0030edc7d4a1cdfe94b04dffec55493

# 9.80git(つようら王11mini 用)
git clone https://github.com/yaneurao/YaneuraOu.git code/native/third_party/YaneuraOu-tsuyoura
git -C code/native/third_party/YaneuraOu-tsuyoura checkout c1b80eaa09fe13d5f12b1599d1ae4d53c224de30

# 両方のソースに、それぞれの patch を当てる
bash code/native/apply_patches.sh
```

`apply_patches.sh` は次のように当てます。**版を取り違えると文脈が合わず失敗する**ので、
patch を手で当てる場合も組み合わせを守ってください。

| ソース | patch |
|---|---|
| `third_party/YaneuraOu`(V9.00) | `patches/0001-issue231-go-mate-dfpn.patch`, `patches/0002-issue238-material-eval-fallback.patch` |
| `third_party/YaneuraOu-tsuyoura`(9.80git) | `patches/tsuyoura/0001-issue231-go-mate-dfpn.patch` 〜 `patches/tsuyoura/0004-issue281-pawn-history-size.patch` |

`patches/*.patch` は直下のファイルだけが対象で、`patches/tsuyoura/` の中は V9.00 に
当てません。適用済みの patch はスキップするので、何度実行してもかまいません。

アプリのリポジトリでは、2つのソースは同じ本家リポジトリを指す2つの git submodule
(`native/third_party/YaneuraOu` と `native/third_party/YaneuraOu-tsuyoura`)で、
`code/scripts/setup_worktree.sh` が `git submodule update --init --recursive` →
`native/apply_patches.sh` → `scripts/fetch_eval.sh` を順に実行します。本リポジトリには
submodule の定義が無いため、上の `git clone` で代わりに取得してください。

## 2. Android(`.so`, arm64-v8a / x86_64)

`code/android-integration/build.gradle.kts` は、Flutter アプリの `android/app/build.gradle.kts`
に以下を設定して `externalNativeBuild` から `code/native/CMakeLists.txt` を呼び出すことを
想定しています。

```kotlin
externalNativeBuild {
    cmake {
        path = file("../../native/CMakeLists.txt")
        version = "3.22.1"
    }
}
```

```bash
cmake -B build/android -S code/native \
  -DCMAKE_TOOLCHAIN_FILE=$ANDROID_NDK/build/cmake/android.toolchain.cmake \
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-24
cmake --build build/android
# 生成: libyaneuraou.so / libaobannue.so / libtsuyoura.so
```

`CMakeLists.txt` の `yaneuraou_sources(<出力変数> <V900|V980>)` が版ごとにソース一覧を
出し分け、`add_yaneuraou_engine()` の版の引数で、ソース一覧と include パスを同じ版に
揃えます(版の違うヘッダを混ぜない)。9.80git のソース一覧は、本家 Makefile に合わせて
`makebook2015`・`learn/*`・`evaluate_nnue_learner` を外し、SFNN 系の特徴量(`a2`、
`half_ka*`)を足したものです。

ビルドしたバイナリに埋め込まれる版の文字列は、`libyaneuraou.so` / `libaobannue.so` が
`9.00git`、`libtsuyoura.so` が `9.80git` です(`strings` で確認できます)。

32bit ABI(armeabi-v7a/x86)は YaneuraOu の `__int128` 非対応のため対象外
(`abiFilters` を 64bit限定にすること)。

## 3. iOS(CocoaPods 経由の静的リンク)

`code/native/` の3つの podspec(`yaneuraou_engine` / `aobannue_engine` / `tsuyoura_engine`)
を、アプリの `native/` に配置し、`code/ios-integration/Podfile` のとおり3つの pod を
アプリの `ios/Podfile` に追加したうえで:

```bash
cd ios && pod install
flutter build ios --release
```

ソース一覧と共通のビルド設定は `code/native/engine_sources.rb` にまとめてあり、
`yaneuraou_engine` / `aobannue_engine` は `:v900`(V9.00)、`tsuyoura_engine` は
`:v980`(9.80git)を使います。生成される xcconfig の `HEADER_SEARCH_PATHS` もそれぞれの
版を向きます。

**注意**: ソース(`third_party/YaneuraOu` / `third_party/YaneuraOu-tsuyoura`)が未取得のまま
`pod install` すると、CocoaPods は本来ソースファイル欠落をエラーにせず黙って無視し、
ブリッジのみを含む Xcode プロジェクトを生成してしまう。これを防ぐため、
`engine_sources.rb` はソースが見つからないと `pod install` をエラーで止める。
ソースを取得・更新したあとは必ず `pod install` を再実行すること。

## 4. `SFNNwoPSQT`(つようら王11mini 用のバイナリだけ)

つようら王11mini の評価関数は SFNN(PSQT なし)系のアーキテクチャで、やねうら王の
本家 Makefile と同じく **`SFNNwoPSQT` をそのバイナリの全ソースに定義する**必要が
あります。`position.h` の `StateInfo` の中身(進行度のキャッシュ)が変わるため、
一部のソースだけに付けると、ソースごとに構造体の大きさが食い違って壊れます。

- CMake: `add_yaneuraou_engine(tsuyoura tsuyoura_start V980 "" "SFNN_ka2_4096_8_64_progress8.h" SFNNwoPSQT)`
- CocoaPods: `tsuyoura_engine.podspec` の `pod_target_xcconfig` に `SFNNwoPSQT=1`

アーキテクチャは `NNUE_ARCHITECTURE_HEADER` に `SFNN_ka2_4096_8_64_progress8.h` を渡し、
実体は `code/native/arch/` を include パスから解決します(このヘッダは 9.80git の
`nnue_arch_gen.py` で生成したもの。詳細は [`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md))。

この評価関数は `nn.bin` に加えて `progress.bin`(進行度の係数)を `EvalDir` から読みます。

## 5. 評価関数(NNUE)の配置

`code/scripts/fetch_eval.sh` を実行し、水匠5の `assets/eval/nn.bin` を取得・配置してから
ビルドすること。ビルド時埋め込み(`EVAL_EMBEDDING`)は行わず、USI `EvalDir` オプションで
実行時にディレクトリを指定して読み込む方式です。

AobaNNUE・つようら王11mini の評価関数はアプリに同梱せず、アプリ内ダウンロードで
導入します(入手先・SHA-256 は [`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md) の 2-b / 2-c)。
ビルドには不要です。

## C ABI(ソケット転送)

`code/native/bridge/engine_bridge.h` に定義されたエントリ関数(`yaneuraou_start(ip, port)`。
バイナリごとに `YANEURAOU_BRIDGE_SYMBOL` で `aobannue_start` / `tsuyoura_start` に
名前を変えてビルドする)を FFI から呼び出すことで、エンジンスレッドを起動します。
詳細は `code/native/bridge/engine_bridge.cpp` を参照してください。

---

将棋Vault
