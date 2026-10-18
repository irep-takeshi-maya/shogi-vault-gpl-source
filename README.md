# 将棋Vault — GPL v3 対応ソースコード配布パッケージ

## 概要

このリポジトリは、iOS/Android アプリ「将棋Vault」が組み込む将棋エンジン
「やねうら王」(GPL v3) の**エンジン統合部分のソースコード**を、GPLv3第6条に基づく
対応ソース(Corresponding Source)として公開するものです。

## ライセンス

このリポジトリに含まれるすべてのコードは GPL v3 でライセンスされています。

- やねうら王: GPL v3
- ブリッジ(`code/native/bridge/`): GPL v3(やねうら王の派生物として)
- ビルド設定(CMakeLists.txt / podspec / engine_sources.rb / Gradle / Podfile): GPL v3
- やねうら王への patch(`code/native/patches/`)とその適用スクリプト: GPL v3(やねうら王への改変として)
- アーキテクチャ定義ヘッダ(`code/native/arch/`): GPL v3(やねうら王 / AobaNNUE 由来)

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
    │   ├── CMakeLists.txt       # Android: 3つの .so をビルド
    │   ├── engine_sources.rb    # iOS: 3つの podspec で共有するソース一覧(版ごと)
    │   ├── yaneuraou_engine.podspec   # 水匠5用(V9.00)
    │   ├── aobannue_engine.podspec    # AobaNNUE用(V9.00)
    │   ├── tsuyoura_engine.podspec    # つようら王11mini用(9.80git)
    │   ├── apply_patches.sh     # やねうら王の2つのソースへ patch を当てる
    │   ├── patches/             # V9.00 用 patch(0001, 0002)
    │   │   └── tsuyoura/        # 9.80git 用 patch(0001〜0004)
    │   ├── arch/                # NNUE アーキテクチャ定義ヘッダ
    │   │   ├── halfkp_768x2-16-64.h
    │   │   └── SFNN_ka2_4096_8_64_progress8.h
    │   └── bridge/
    │       ├── engine_bridge.h
    │       └── engine_bridge.cpp
    ├── android-integration/
    │   └── build.gradle.kts     # externalNativeBuild 設定(抜粋: android/app/build.gradle.kts)
    ├── ios-integration/
    │   └── Podfile               # CocoaPods 統合設定(ios/Podfile)
    └── scripts/
        ├── fetch_eval.sh         # 評価関数(水匠5)取得スクリプト
        └── setup_worktree.sh     # アプリのリポジトリでの取得〜patch適用〜評価関数取得の一括実行
```

注意: 容量・ライセンスの都合上、以下は含まれていません:

- やねうら王本体のソースコード(公式リポジトリから2つの pin コミットを取得。
  V9.00 `a5ee2786` と 9.80git `c1b80eaa`)。アプリ固有の改変は `code/native/patches/` に含む
- 評価関数ファイル(水匠5 は `fetch_eval.sh` で取得。AobaNNUE・つようら王11mini は
  本リポジトリの Releases から取得)
- アプリ本体のコード(独自ライセンス。UI・研究データ管理・永続化ロジック等)

詳細は [`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md) を参照してください。

## アーキテクチャ

```
アプリ本体(独自ライセンス, 非公開)
    ↓ USI プロトコル(ローカル TCP ソケット, 127.0.0.1)
engine_bridge (GPL v3, 本リポジトリ code/native/bridge/)
    ↓ 静的リンク
やねうら王 (GPL v3, pin コミット参照 + code/native/patches/)
  - third_party/YaneuraOu          V9.00   → 水匠5 / AobaNNUE
  - third_party/YaneuraOu-tsuyoura 9.80git → つようら王11mini
```

評価関数のアーキテクチャごとにエンジンバイナリを3本ビルドします。Android は NDK + CMake で
`libyaneuraou.so` / `libaobannue.so` / `libtsuyoura.so` をビルドし APK に同梱、iOS は CocoaPods
ローカル pod(`yaneuraou_engine` / `aobannue_engine` / `tsuyoura_engine`)経由でアプリバイナリへ
リンクします。詳細は [`BUILD_INSTRUCTIONS.md`](BUILD_INSTRUCTIONS.md) を参照してください。

## GPL v3 コンプライアンス

### このパッケージで提供されるもの

- ✅ engine_bridge の完全なソースコード
- ✅ やねうら王への改変(patch)の全文と、その適用スクリプト
- ✅ Android(CMake/Gradle)・iOS(CocoaPods)統合のビルド設定
- ✅ 評価関数取得スクリプト
- ✅ ビルド手順のドキュメント

### 提供されないもの

- ❌ やねうら王本体のソースコード(公式リポジトリから入手。pin コミットは `EXTERNAL_RESOURCES.md`)
- ❌ 評価関数ファイル(リポジトリには含めない。入手先は `EXTERNAL_RESOURCES.md`)
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
- 山下 宏 氏 — AobaNNUE の提供
- やねうら王プロジェクト — つようら王11mini の提供

---

将棋Vault
