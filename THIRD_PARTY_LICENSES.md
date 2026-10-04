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

### 評価関数(NNUE): 水匠5(Suisho5)

- ライセンス: **GPLv3**
- アーキテクチャ: NNUE HalfKP KP256(`EVAL_NNUE_KP256`)
- ファイル: `nn.bin`(約62MB)
- 取得元: <https://github.com/yaneurao/YaneuraOu/releases/tag/suisho5>
  (`Suisho5.7z` を展開)
- 本アプリでは `nn.bin` をリポジトリに含めず、`scripts/fetch_eval.sh` でビルド前に
  取得してアプリへ同梱する。推奨 `FV_SCALE=24`。
- 別の評価関数へ差し替えた場合は本項を更新する。

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

- エンジン統合部分の対応ソース(engine_bridge・ビルド設定・評価関数取得スクリプト・
  GPLv3全文): [`README.md`](README.md) 参照
- やねうら王本体: [`EXTERNAL_RESOURCES.md`](EXTERNAL_RESOURCES.md) に記載の pin コミット
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
