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
