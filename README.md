# 将棋Vault — GPL v3 対応ソースコード配布パッケージ

## 概要

このリポジトリは、iOS/Android アプリ「将棋Vault」が組み込む将棋エンジン
「やねうら王」(GPL v3) の**エンジン統合部分のソースコード**を、GPLv3第6条に基づく
対応ソース(Corresponding Source)として公開するものです。

## ライセンス

このリポジトリに含まれるすべてのコードは GPL v3 でライセンスされています。

- やねうら王: GPL v3
- ブリッジ(`code/native/bridge/`): GPL v3(やねうら王の派生物として)
- ビルド設定(CMakeLists.txt / podspec / Gradle / Podfile): GPL v3

詳細は [`LICENSE.txt`](LICENSE.txt) を参照してください。

## パッケージ内容

```
.
├── README.md                    # このファイル
├── LICENSE.txt                  # GPL v3 ライセンス全文
├── BUILD_INSTRUCTIONS.md        # ビルド手順
├── EXTERNAL_RESOURCES.md        # 外部リソース(やねうら王本体・評価関数)の入手方法
├── docs/
│   └── privacy.html             # プライバシーポリシー(参考)
└── code/
    ├── native/                  # エンジンブリッジ(Android/iOS共通, C++)
    │   ├── CMakeLists.txt
    │   ├── yaneuraou_engine.podspec
    │   └── bridge/
    │       ├── engine_bridge.h
    │       └── engine_bridge.cpp
    ├── android-integration/
    │   └── build.gradle.kts     # externalNativeBuild 設定(抜粋: android/app/build.gradle.kts)
    ├── ios-integration/
    │   └── Podfile               # CocoaPods 統合設定(ios/Podfile)
    └── scripts/
        └── fetch_eval.sh         # 評価関数(水匠5)取得スクリプト
```

注意: 容量・ライセンスの都合上、以下は含まれていません:

- やねうら王本体のソースコード(公式リポジトリから pin コミットを取得)
- 評価関数ファイル(水匠5, 別ライセンス。`fetch_eval.sh`で取得)
- アプリ本体のコード(独自ライセンス。UI・研究データ管理・永続化ロジック等)

詳細は [`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md) を参照してください。

## アーキテクチャ

```
アプリ本体(独自ライセンス, 非公開)
    ↓ USI プロトコル(ローカル TCP ソケット, 127.0.0.1)
engine_bridge (GPL v3, 本リポジトリ code/native/bridge/)
    ↓ 静的リンク
やねうら王 (GPL v3, third_party/YaneuraOu, pin コミット参照)
```

Android は NDK + CMake で `libyaneuraou.so` をビルドし APK に同梱、iOS は CocoaPods
ローカル pod(`yaneuraou_engine.podspec`)経由でアプリバイナリへ静的リンクします。

## GPL v3 コンプライアンス

### このパッケージで提供されるもの

- ✅ engine_bridge の完全なソースコード
- ✅ Android(CMake/Gradle)・iOS(CocoaPods)統合のビルド設定
- ✅ 評価関数取得スクリプト
- ✅ ビルド手順のドキュメント

### 提供されないもの

- ❌ やねうら王本体のソースコード(公式リポジトリから入手)
- ❌ 評価関数ファイル(別ライセンス)
- ❌ アプリ本体のコード(独自ライセンス)

## サポート

- やねうら王本体に関する質問: https://github.com/yaneurao/YaneuraOu/issues
- 本パッケージ(エンジン統合部分)に関する質問: 配布元アプリ内のお問い合わせ先まで

## ライセンス表示

```
YaneuraOu
Copyright (C) yaneurao
Licensed under GPL v3
https://github.com/yaneurao/YaneuraOu

engine_bridge (Android/iOS Integration)
Licensed under GPL v3 (as derivative work of YaneuraOu)
```

GPL v3 の全文は [`LICENSE.txt`](LICENSE.txt) を参照してください。

## 謝辞

- やねうらお氏 — やねうら王の開発
- 水匠開発チーム — 評価関数の提供

---

将棋Vault
