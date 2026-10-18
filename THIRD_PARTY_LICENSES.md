# サードパーティ・ライセンス告知

本アプリ(将棋Vault / Shogi Vault)は、モバイル(iOS/Android)版でオンデバイス解析を
実現するため、以下のGPLライセンスの将棋エンジン/評価関数を**アプリに組み込んで
配布**する。

組込みに用いるYaneuraOu/水匠5はGPLv3であるため、GPLv3 第6条の対応ソース提供義務が
生じる。本アプリは方針A(アプリ全体をGPLv3で配布)ではなく、**きふみAI方式
(エンジン統合部分のソースコードのみを独立公開する方式)を採択している**。
**本リポジトリがその対応ソース(エンジン統合部分)であり、アプリ本体のソースは
独自ライセンスのため非公開**。GPLv3のライセンス全文は[`LICENSE.txt`](LICENSE.txt)を参照
(アプリ本体全体のライセンス表明ではない)。

## 組み込むソフトウェア

### YaneuraOu(やねうら王)

- ライセンス: **GPLv3**
- 著作権: yaneurao および貢献者
- 由来: Stockfish / Apery / SilentMajority の派生物(そのためGPLv3)
- 取得元: <https://github.com/yaneurao/YaneuraOu>
- バージョン: **エンジンによって2つの版を使い分けている**(shogi-vault Issue #346、2026-10-17)。

  | エンジン | YaneuraOu の版 | 取得元コミット | 配置先(本リポジトリ) | アプリ固有の改変(patch) |
  |---|---|---|---|---|
  | 水匠5(同梱)・AobaNNUE(アプリ内DL) | **`V9.00`**(GitHub の安定版) | `a5ee2786` | `code/native/third_party/YaneuraOu` | `code/native/patches/*.patch` |
  | つようら王11mini(アプリ内DL、Pro) | **`9.80git`**(開発版、master) | `c1b80eaa` | `code/native/third_party/YaneuraOu-tsuyoura` | `code/native/patches/tsuyoura/*.patch` |

  - V9.00: 2026-09時点で pin を `v8.60git`(`eb2856f9`)から shogi-vault Issue #134 の方針に
    基づき更新した。V9.00はStockfish本家に合わせた大規模リファクタ(全シンボルが
    `namespace YaneuraOu`配下に移動、`Options`/`Threads`等のグローバルオブジェクトが廃止され
    `Engine`/`IEngine`クラスのメンバへ移行等)を含むが、評価関数(水匠5)は変更なし。
  - 9.80git: つようら王11mini の評価関数(SFNN `ka2` / `progress8`)を読める安定版がまだ無いため、
    **つようら王11mini 用のバイナリだけ**を master `c1b80eaa`(2026-09-14、`ENGINE_VERSION "9.80git"`)
    でビルドしている。水匠5・AobaNNUE のバイナリは V9.00 のまま。
  - 取得手順は [`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md)、ビルド手順は
    [`BUILD_INSTRUCTIONS.md`](BUILD_INSTRUCTIONS.md) を参照。
- アプリ固有の改変は patch として管理し、ソースの版ごとに分けている。いずれも GPLv3 の
  やねうら王への改変であり、**両方の版のソースと両方の patch を対応ソースに含める**
  (patch の全文は `code/native/patches/`)。
  - V9.00 用(`code/native/patches/`): go mate(0001)、玉なし局面の駒得フォールバック(0002)
  - 9.80git 用(`code/native/patches/tsuyoura/`): go mate(0001)、玉なし局面の駒得フォールバック(0002)、
    評価関数読み込み時の複製の回避(0003)、PawnHistory の大きさ(0004)。0001/0002 は V9.00 用と
    同じ内容を 9.80git のソースに合わせて作り直したもの

### 評価関数(NNUE): 水匠5(Suisho5)

- ライセンス: **GPLv3**
- アーキテクチャ: NNUE HalfKP KP256(`EVAL_NNUE_KP256`)
- ファイル: `nn.bin`(約62MB)
- 取得元: <https://github.com/yaneurao/YaneuraOu/releases/tag/suisho5>
  (`Suisho5.7z` を展開)
- 本アプリでは `nn.bin` をリポジトリに含めず、`scripts/fetch_eval.sh` でビルド前に
  取得してアプリへ同梱する。推奨 `FV_SCALE=24`。
- 別の評価関数へ差し替えた場合は本項を更新する。

### 評価関数(NNUE): AobaNNUE(アプリ内ダウンロード、shogi-vault Issue #282)

- ライセンス: **GPLv3**
- アーキテクチャ: NNUE HalfKP 768x2-16-64
- ファイル: `nn.bin`(約184MB)
- 取得元: <https://github.com/yssaya/AobaNNUE>
- 著作者: 山下 宏(yssaya)氏
- **アプリには同梱せず、ユーザーの操作でアプリ内ダウンロードして導入する**。
- 本アプリからの再配布先:
  <https://github.com/irep-takeshi-maya/shogi-vault-gpl-source/releases/tag/aobannue-eval-v1.1>
  (原典のファイルを内容を変えずに再配布。SHA-256 等は [`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md) の 2-b)
- 推奨 `FV_SCALE=40`。
- 再配布の根拠: 著作者が2026-09-27に「nn.bin も GPLv3」と明言している
  (<http://www.yss-aya.com/bbs/patio.cgi?read=210>)。
- 本評価関数を読むために、専用のエンジンバイナリ(`halfkp_768x2-16-64` でビルドした
  `libaobannue.so` / `aobannue_engine.framework`、やねうら王 V9.00)を同梱している。
  アーキテクチャ定義ヘッダ `code/native/arch/halfkp_768x2-16-64.h` は AobaNNUE 同梱
  ソース由来(GPLv3)で、include パスのみ書き換えている。

### 評価関数(NNUE): つようら王11mini(アプリ内ダウンロード、Pro 限定、shogi-vault Issue #346)

- ライセンス: **MIT License**(`nn.bin`・`progress.bin` とも)
- 著作権: やねうら王プロジェクト(yaneurao)
- アーキテクチャ: SFNN `SFNN_ka2_4096_8_64_progress8`(進行度つき)
- ファイル: `nn.bin`(7,711,372バイト)と `progress.bin`(1,003,104バイト)
- 取得元: <https://yaneurao.fanbox.cc/posts/12761272>(2026-10-14 の全体公開記事)。
  配布アーカイブ
  `TuyouraOu11mini-SFNN_ka2_4096_8_64_progress8-epoch0010-20261003.7z`
  (<https://storage.yaneu.com/yaneuraou/>)から `nn.bin` と `progress.bin` だけを取り出して使う
- 配布元の記事は「MIT License」「二次配布・商用利用可」と明記しているが、
  アーカイブにライセンス文と著作権表示行は含まれていない。下記の著作権表示は、
  アーカイブ内 `tag.json` の作者表記(「やねうら王プロジェクト」)と公開年から記載したもの
- **アプリには同梱せず、Pro プランのユーザーがアプリ内ダウンロードで導入する**。
  再配布先: 本リポジトリの Releases、タグ `tsuyoura11mini-eval-20261003`
  (<https://github.com/irep-takeshi-maya/shogi-vault-gpl-source/releases/tag/tsuyoura11mini-eval-20261003>。
  SHA-256 は [`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md) の 2-c)
- 推奨 `FV_SCALE=24`。アーカイブ内の `eval_options.txt` は置かない
  (エンジンが自動で読んで FV_SCALE を上書きするため、アプリから送る値に一本化する)
- 本評価関数を読むために、**専用のエンジンバイナリ**を同梱している
  (`SFNN_ka2_4096_8_64_progress8` と `SFNNwoPSQT` でビルドした `libtsuyoura.so` /
  `tsuyoura_engine.framework`)。エンジン本体はやねうら王(GPLv3)だが、他の2つ(V9.00)と違い
  **9.80git(master `c1b80eaa`)** でビルドしている(上記「YaneuraOu」の表)。組込みの方式
  (プロセス内リンク、評価関数は実行時ロード)は他の2つと同じ。対応ソースの提供義務は
  下記「対応するソース(Corresponding Source)の提供」に従う
- アーキテクチャ定義ヘッダ `code/native/arch/SFNN_ka2_4096_8_64_progress8.h` は、やねうら王 9.80git(`c1b80eaa`)の
  `source/eval/nnue/architectures/nnue_arch_gen.py` で生成したもの(GPLv3 のやねうら王の一部)。
  include パスのみ本リポジトリの配置に合わせて書き換えている

```
MIT License

Copyright (c) 2026 やねうら王プロジェクト (yaneurao)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

### Stockfish(上流)

- ライセンス: **GPLv3**
- YaneuraOu が参照する上流エンジン。
- 取得元: <https://github.com/official-stockfish/Stockfish>

## アプリに同梱するデータ

### 新ペタショック定跡(戦型ポートフォリオの定跡ツリー、Issue #306)

- ライセンス: **MIT License**
- 著作権: yaneurao(やねうら王チーム)
- 取得元: <https://github.com/yaneurao/YaneuraOu/releases/tag/new_petabook233>
  (`new_petabook_20250505c.7z`、2025-06-21 公開)。公開の経緯:
  <https://yaneuraou.yaneu.com/2025/06/22/2-33m-book-released-for-free/>
- 用途: 定跡ファイルそのものはアプリに含めない。開発側で `tool/portfolio_book/`
  により戦型ポートフォリオの各項目の手順・分岐・評価値を抜き出した派生データ
  (`assets/portfolio_trees/*.json`)だけを同梱する。
- 配布元(GitHub のリリース本文・公開記事)は「MIT License にて公開」と明記して
  いるが、MIT License の著作権表示行(`Copyright (c) <年> <著作権者>`)と
  ライセンス文の同梱は無い。下記の著作権表示は、配布者(GitHub アカウント
  `yaneurao`)と公開年から記載したもの。

```
MIT License

Copyright (c) 2025 yaneurao

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## UI フォント

### BIZ UDPゴシック(BIZ UDPGothic)

- ライセンス: **SIL Open Font License 1.1**(OFL)
- 著作権: The BIZ UDGothic Project Authors
- 用途: 盤面・一覧画面等アプリ全体の可読性向上(UDフォント)のため同梱。
- 取得元: <https://github.com/google/fonts/tree/main/ofl/bizudpgothic>
- ライセンス全文: `assets/fonts/OFL.txt`

## 対応するソース(Corresponding Source)の提供

GPLv3 第6条に基づき、配布したバイナリに対応する完全なソースコードを提供する。
**このリポジトリ自体が、その対応ソース(エンジン統合部分)である**。アプリ本体の
ソースは独自ライセンスのため非公開。

- エンジン統合部分の対応ソース(engine_bridge・ビルド設定・やねうら王への patch と
  その適用スクリプト・評価関数取得スクリプト・GPLv3全文): [`README.md`](README.md) 参照
- やねうら王本体: [`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md) に記載の pin コミット。
  **V9.00(`a5ee2786`、水匠5・AobaNNUE)と 9.80git(`c1b80eaa`、つようら王11mini)の2つ**を、
  それぞれに当てる patch(`code/native/patches/` と `code/native/patches/tsuyoura/`)とともに記載する
- 各リリースの対応コミット・タグは、リリースごとに本リポジトリを更新して記載する。

## 未確定事項(出荷前に確定すること)

- **App Store の利用規約と GPL の緊張関係**(VLC の App Store 撤去事例)。
  Apple の TOS による利用制限・DRM と GPL の相互作用について、配布前に
  法務確認を行う。GPLの著作権者(Stockfish 貢献者等)の意向確認を含む。
- 組込みに用いる評価関数の最終選定とそのライセンス条項の確認。
- **「エンジン統合部分のみ公開」方式の法的十分性**: アプリ本体は同一プロセス内
  静的リンクのため、GPLv3上「結合された著作物」全体の対応ソース提供が必要という
  厳格な解釈もあり得る。無料版リリースにあたっての当面の対応であり、有償化前に
  弁護士確認のうえ、必要であれば方針A(アプリ全体公開)への切替を検討する。
