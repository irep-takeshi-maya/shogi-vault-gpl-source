# ビルド手順

## 概要

やねうら王(pin コミット `eb2856f9`、タグ `v8.60git`)を、Android(`.so`, NDK/CMake)と iOS(静的リンク,
CocoaPods)向けにビルドする手順です。エンジンは評価関数不要な material 構成をベースに、
NNUE(水匠5, HalfKP KP256)を実行時ロードで組み込みます。

## 前提条件

- `code/native/` を実際のアプリの `native/` として配置し、[`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md)
  に従って `third_party/YaneuraOu`(pin コミット)を取得済みであること。
- Android: NDK, CMake 3.22 以降
- iOS: Xcode, CocoaPods

## Android(`.so`, arm64-v8a / x86_64)

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
# 生成: libyaneuraou.so
```

32bit ABI(armeabi-v7a/x86)は YaneuraOu の `__int128` 非対応のため対象外
(`abiFilters` を 64bit限定にすること)。

## iOS(CocoaPods 経由の静的リンク)

`code/native/yaneuraou_engine.podspec` を、アプリの `native/yaneuraou_engine.podspec` として
配置し、`code/ios-integration/Podfile` の内容(`pod 'yaneuraou_engine', :path => '../native'`)を
アプリの `ios/Podfile` に追加したうえで:

```bash
cd ios && pod install
flutter build ios --release
```

**注意**: submodule(`third_party/YaneuraOu`)未初期化のまま `pod install` すると、
CocoaPods はソースファイル欠落をエラーにせず黙って無視し、ブリッジのみを含む
Xcodeプロジェクトを生成してしまう。submodule取得後は必ず `pod install` を再実行すること。

## 評価関数(NNUE)の配置

`code/scripts/fetch_eval.sh` を実行し、`assets/eval/nn.bin` を取得・配置してから
ビルドすること。ビルド時埋め込み(`EVAL_EMBEDDING`)は行わず、USI `EvalDir` オプションで
実行時にディレクトリを指定して読み込む方式です。

## C ABI(ソケット転送)

`code/native/bridge/engine_bridge.h` に定義された `yaneuraou_start(ip, port)` を FFI から
呼び出すことで、エンジンスレッドを起動します。詳細は `code/native/bridge/engine_bridge.cpp`
を参照してください。

---

将棋Vault
