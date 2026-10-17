# 外部リソース

このリポジトリには容量・ライセンスの都合上、以下のファイルは含まれていません。
ビルドに必要なリソースは、以下から入手してください。

## 必須リソース

### 1. やねうら王 本体(pin コミット、2つの版)

やねうら王のソースは、**エンジンによって2つの版を使い分けている**
(shogi-vault Issue #346、2026-10-17)。両方を取得する必要がある。

| 版 | pin コミット | 配置先 | 使うエンジン(バイナリ) | 当てる patch |
|---|---|---|---|---|
| **V9.00**(安定版、タグ `V9.00`) | `a5ee2786c0030edc7d4a1cdfe94b04dffec55493` | `code/native/third_party/YaneuraOu/` | 水匠5(`yaneuraou`)・AobaNNUE(`aobannue`) | `code/native/patches/*.patch`(0001, 0002) |
| **9.80git**(開発版、master) | `c1b80eaa09fe13d5f12b1599d1ae4d53c224de30` | `code/native/third_party/YaneuraOu-tsuyoura/` | つようら王11mini(`tsuyoura`) | `code/native/patches/tsuyoura/*.patch`(0001〜0004) |

公式リポジトリ: https://github.com/yaneurao/YaneuraOu (どちらの版も同じリポジトリ)
ライセンス: GPL v3

#### 1-a. V9.00(水匠5・AobaNNUE 用)

```bash
git clone https://github.com/yaneurao/YaneuraOu.git code/native/third_party/YaneuraOu
git -C code/native/third_party/YaneuraOu checkout a5ee2786c0030edc7d4a1cdfe94b04dffec55493   # V9.00
```

pin コミット: `a5ee2786c0030edc7d4a1cdfe94b04dffec55493`(タグ `V9.00`)。
旧pinは `eb2856f91e6088e5b8c1216e3d84d0563f7cd85f`(タグ `v8.60git`)だったが、
2026-09にshogi-vault Issue #134の方針に基づき更新した。V9.00はStockfish本家に
合わせた大規模リファクタ(全シンボルが`namespace YaneuraOu`配下に移動、
`Options`/`Threads`等のグローバルオブジェクトが廃止され`Engine`/`IEngine`クラスの
メンバへ移行、`usi_option.cpp`→`usioption.cpp`への改名、`engine.cpp`/`search.cpp`/
`score.cpp`/`benchmark.cpp`/`tune.cpp`の新規追加等)を含むが、評価関数
(水匠5, HalfKP256, `EVAL_NNUE_HALFKP256`)は変更なし。

#### 1-b. 9.80git(つようら王11mini 用)

```bash
git clone https://github.com/yaneurao/YaneuraOu.git code/native/third_party/YaneuraOu-tsuyoura
git -C code/native/third_party/YaneuraOu-tsuyoura checkout c1b80eaa09fe13d5f12b1599d1ae4d53c224de30   # master, 9.80git
```

pin コミット: `c1b80eaa09fe13d5f12b1599d1ae4d53c224de30`(master、2026-09-14、
`ENGINE_VERSION "9.80git"`。`git describe` では `v9.40-53-gc1b80eaa`)。
つようら王11mini の評価関数(SFNN の `ka2` 特徴量 / `progress8`)は V9.00 に無く、
これを読める安定版がまだ GitHub Releases に無いため、**つようら王11mini 用の
バイナリだけ**をこの版でビルドしている。水匠5・AobaNNUE のバイナリは V9.00 のまま。
本家が次の安定版を出したら、全エンジンをそちらへ揃える予定。

#### 1-c. アプリ固有の改変(patch)

取得した2つのソースには、それぞれの patch を当てる。どちらも GPL v3 のやねうら王への
改変で、本リポジトリの `code/native/patches/` に全文を置いている。

| patch | V9.00 用(`patches/`) | 9.80git 用(`patches/tsuyoura/`) |
|---|---|---|
| 0001 go mate(DFPN 詰将棋探索、shogi-vault Issue #231) | あり | あり(9.80git のソースに合わせて作り直したもの) |
| 0002 玉なし局面の駒得フォールバック(Issue #238) | あり | あり(同上) |
| 0003 評価関数読み込み時の複製の回避(Issue #281) | — | あり |
| 0004 PawnHistory の大きさ(Issue #281) | — | あり |

`code/native/apply_patches.sh` が、両方のソースにそれぞれの patch を当てる
(`patches/*.patch` は直下のファイルだけが対象で、`tsuyoura/` の中は V9.00 に当てない)。
適用済みかどうかを `git apply --check --reverse` で判定するので、何度実行してもよい。
手順は [`BUILD_INSTRUCTIONS.md`](BUILD_INSTRUCTIONS.md) を参照。

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

### 2-c. 評価関数ファイル(nn.bin + progress.bin, つようら王11mini)

必要なファイル: SFNN 評価関数ファイル `nn.bin` と、進行度の係数 `progress.bin`
(アーキテクチャ `SFNN_ka2_4096_8_64_progress8`)。**2つとも揃って初めて読める**。

アプリには同梱せず、Pro プランのユーザーがアプリ内ダウンロードで導入する任意の
評価関数。ビルドそのものには不要だが、`tsuyoura` ターゲット(つようら王11mini 用の
エンジンバイナリ)の動作確認には必要。

入手先(本プロジェクトによる再配布、タグ `tsuyoura11mini-eval-20261003`):

```
https://github.com/irep-takeshi-maya/shogi-vault-gpl-source/releases/tag/tsuyoura11mini-eval-20261003
```

| ファイル | バイト数 | SHA-256 |
|---|---|---|
| `nn.bin` | 7,711,372 | `45323c65135da65d6c2c364eea93a5af6a285c27ebfb07eab4afbe737ef74508` |
| `progress.bin` | 1,003,104 | `a2d18f0564639c945b5d6469e9b437cd64a526ad4fbcef02ab1d8f547d08e5f1` |

原典: https://yaneurao.fanbox.cc/posts/12761272 (2026-10-14 の全体公開記事)。
配布アーカイブ `TuyouraOu11mini-SFNN_ka2_4096_8_64_progress8-epoch0010-20261003.7z`
(https://storage.yaneu.com/yaneuraou/)から `nn.bin` と `progress.bin` だけを
取り出したもの(内容は変えていない)。
ライセンス: **MIT License**(著作権表示とライセンス全文は
[`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md))
推奨 FV_SCALE: 24。アーカイブ内の `eval_options.txt` は置かない
(エンジンが自動で読んで FV_SCALE を上書きするため、アプリから送る値に一本化している)。

#### なぜエンジンバイナリが3つあるか

NNUE のネットワーク構造は、やねうら王本体に**ビルド時固定で焼き込まれる**。
実行時に切り替えることはできないため、評価関数のアーキテクチャごとに別の
バイナリが必要になる。

| 評価関数 | アーキテクチャ | やねうら王の版 | エントリ関数 | Android | iOS pod |
|---|---|---|---|---|---|
| 水匠5 | halfkp_256x2-32-32 | V9.00 | `yaneuraou_start` | `libyaneuraou.so` | `yaneuraou_engine` |
| AobaNNUE | halfkp_768x2-16-64 | V9.00 | `aobannue_start` | `libaobannue.so` | `aobannue_engine` |
| つようら王11mini | SFNN_ka2_4096_8_64_progress8 | 9.80git | `tsuyoura_start` | `libtsuyoura.so` | `tsuyoura_engine` |

アーキテクチャ定義ヘッダは `code/native/arch/` に置き、やねうら王本体の
`nnue_architecture.h` が持つ `NNUE_ARCHITECTURE_HEADER` フック経由で読み込ませている
(アーキテクチャの差し替えのために本体のソースを改変してはいない。本体への改変は
上記 1-c の patch だけ)。

- `halfkp_768x2-16-64.h`: AobaNNUE 同梱ソース由来
- `SFNN_ka2_4096_8_64_progress8.h`: やねうら王 9.80git(`c1b80eaa`)の
  `source/eval/nnue/architectures/nnue_arch_gen.py` で生成したもの。本家 Makefile は
  ビルドのたびにこのスクリプトでヘッダを生成するが、アプリのビルドに Python を
  要求しないよう、生成済みのものを置いている(include パスのみ書き換え)

SFNN 系(つようら王11mini)は、全ソースに `SFNNwoPSQT` の定義が要る(本家 Makefile と同じ)。
詳しくは [`BUILD_INSTRUCTIONS.md`](BUILD_INSTRUCTIONS.md) を参照。

3つを同一プロセスへ載せるため、公開するエントリ関数名を
`YANEURAOU_BRIDGE_SYMBOL` で分け、`-fvisibility=hidden` で内部シンボルを
隠している。これが無いと、本体の C++ シンボルが各バイナリから約970個ずつ export
され、動的リンカが呼び出し先を取り違えてクラッシュする。版の違うソース
(V9.00 と 9.80git)が同じプロセスに載るため、なおさら必須である。

## リソースの配置例

```
code/native/
├── CMakeLists.txt
├── engine_sources.rb
├── apply_patches.sh
├── yaneuraou_engine.podspec
├── aobannue_engine.podspec
├── tsuyoura_engine.podspec
├── arch/
├── bridge/
├── patches/                   ← V9.00 用 patch(0001, 0002)
│   └── tsuyoura/              ← 9.80git 用 patch(0001〜0004)
└── third_party/
    ├── YaneuraOu/             ← 上記 1-a で取得(V9.00, a5ee2786)
    │   └── source/
    └── YaneuraOu-tsuyoura/    ← 上記 1-b で取得(9.80git, c1b80eaa)
        └── source/

assets/eval/
└── nn.bin                     ← 上記 2. で取得(fetch_eval.sh)
```

---

将棋Vault
