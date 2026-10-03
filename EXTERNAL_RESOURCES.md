# 外部リソース

このリポジトリには容量・ライセンスの都合上、以下のファイルは含まれていません。
ビルドに必要なリソースは、以下から入手してください。

## 必須リソース

### 1. やねうら王 本体(pin コミット)

必要なファイル: やねうら王のソースコード全体(`source/` 配下)

入手先:

```bash
git clone https://github.com/yaneurao/YaneuraOu.git
cd YaneuraOu
git checkout a5ee2786c0030edc7d4a1cdfe94b04dffec55493   # V9.00
```

配置先:

```
code/native/third_party/YaneuraOu/
```

公式リポジトリ: https://github.com/yaneurao/YaneuraOu
ライセンス: GPL v3
pin コミット: `a5ee2786c0030edc7d4a1cdfe94b04dffec55493`(タグ `V9.00`)。
旧pinは `eb2856f91e6088e5b8c1216e3d84d0563f7cd85f`(タグ `v8.60git`)だったが、
2026-09にshogi-vault Issue #134の方針に基づき更新した。V9.00はStockfish本家に
合わせた大規模リファクタ(全シンボルが`namespace YaneuraOu`配下に移動、
`Options`/`Threads`等のグローバルオブジェクトが廃止され`Engine`/`IEngine`クラスの
メンバへ移行、`usi_option.cpp`→`usioption.cpp`への改名、`engine.cpp`/`search.cpp`/
`score.cpp`/`benchmark.cpp`/`tune.cpp`の新規追加等)を含むが、評価関数
(水匠5, HalfKP256, `EVAL_NNUE_HALFKP256`)は変更なし。

### 2. 評価関数ファイル(nn.bin, 水匠5)

必要なファイル: NNUE評価関数ファイル(HalfKP KP256, 約62MB)

入手先: https://github.com/yaneurao/YaneuraOu/releases/download/suisho5/Suisho5.7z
(`Suisho5.7z` を展開し `nn.bin` を取り出す。取得手順は `code/scripts/fetch_eval.sh` を参照)

配置先(実行時に読み込み。ビルド時埋め込みは行わない):

```
assets/eval/nn.bin
```

ライセンス: GPL v3
推奨 `FV_SCALE`: 24

## リソースの配置例

```
code/native/
├── CMakeLists.txt
├── bridge/
├── yaneuraou_engine.podspec
└── third_party/
    └── YaneuraOu/          ← 上記 1. で取得(pin コミット)
        └── source/

assets/eval/
└── nn.bin                  ← 上記 2. で取得(fetch_eval.sh)
```

---

将棋Vault


### 2-b. 評価関数ファイル(nn.bin, AobaNNUE)

必要なファイル: NNUE評価関数ファイル(HalfKP 768x2-16-64, 約184MB)

アプリには同梱せず、ユーザーの操作でアプリ内ダウンロードして導入する任意の
評価関数。ビルドそのものには不要だが、`aobannue` ターゲット(AobaNNUE 用の
エンジンバイナリ)の動作確認には必要。

入手先(本プロジェクトが GPLv3 のもとで再配布しているミラー):

```
https://github.com/irep-takeshi-maya/shogi-vault-gpl-source/releases/tag/aobannue-eval-v1.1
```

原典: https://github.com/yssaya/AobaNNUE (著作者: 山下 宏 氏)
ライセンス: GPL v3
SHA-256: `f8ee839ae8c08537036f23345dd5ed0416958b22425476fc60177942903219b5`
サイズ: 192,624,720 バイト
推奨 FV_SCALE: **40**(水匠5の24とは異なる)

著作者が2026-09-27に nn.bin を含む配布物全体が GPLv3 であることを明言している
(http://www.yss-aya.com/bbs/patio.cgi?read=210)。

#### なぜエンジンバイナリが2つあるか

NNUE のネットワーク構造は、やねうら王本体に**ビルド時固定で焼き込まれる**。
実行時に切り替えることはできないため、評価関数のアーキテクチャごとに別の
バイナリが必要になる。

| 評価関数 | アーキテクチャ | エントリ関数 | Android | iOS pod |
|---|---|---|---|---|
| 水匠5 | halfkp_256x2-32-32 | `yaneuraou_start` | `libyaneuraou.so` | `yaneuraou_engine` |
| AobaNNUE | halfkp_768x2-16-64 | `aobannue_start` | `libaobannue.so` | `aobannue_engine` |

アーキテクチャ定義(`code/native/arch/halfkp_768x2-16-64.h`)は AobaNNUE 同梱
ソース由来。やねうら王本体の `nnue_architecture.h` が持つ
`NNUE_ARCHITECTURE_HEADER` フック経由で読み込ませており、**やねうら王本体の
ソースは一切改変していない**。

2つを同一プロセスへ載せるため、公開するエントリ関数名を
`YANEURAOU_BRIDGE_SYMBOL` で分け、`-fvisibility=hidden` で内部シンボルを
隠している。これが無いと、本体の C++ シンボルが両方から約970個ずつ export
され、動的リンカが呼び出し先を取り違えてクラッシュする。
