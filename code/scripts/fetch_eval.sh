#!/usr/bin/env bash
# 水匠5 の NNUE 評価関数 nn.bin(約62MB)を公式リリースから取得し、
# assets/eval/nn.bin へ展開する。実機/エミュレータ/リリース向けビルドの前に実行する。
#
# nn.bin はリポジトリに含めない(.gitignore)。CI ではキャッシュと併用する。
# 依存: curl, 7z(p7zip)。
#
# ライセンス: 水匠5 評価関数は GPLv3。THIRD_PARTY_LICENSES.md を参照。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EVAL_DIR="$ROOT/assets/eval"
OUT="$EVAL_DIR/nn.bin"
URL="https://github.com/yaneurao/YaneuraOu/releases/download/suisho5/Suisho5.7z"
ARCHIVE="$EVAL_DIR/Suisho5.7z"

mkdir -p "$EVAL_DIR"

if [ -f "$OUT" ]; then
  echo "nn.bin は既に存在します: $OUT ($(du -h "$OUT" | cut -f1))"
  exit 0
fi

echo "水匠5 評価関数をダウンロード中: $URL"
curl -fL --retry 3 -o "$ARCHIVE" "$URL"

echo "nn.bin を展開中..."
# 7z の実体は環境により 7z / 7za / 7zr。最初に見つかったものを使う。
SEVENZIP=""
for c in 7z 7za 7zr; do
  if command -v "$c" >/dev/null 2>&1; then SEVENZIP="$c"; break; fi
done
if [ -z "$SEVENZIP" ]; then
  echo "エラー: 7z(p7zip)が見つかりません。インストールしてください。" >&2
  exit 1
fi

# アーカイブ内の nn.bin だけを assets/eval/ 直下へ取り出す。
"$SEVENZIP" e -y -o"$EVAL_DIR" "$ARCHIVE" nn.bin >/dev/null
rm -f "$ARCHIVE"

if [ ! -f "$OUT" ]; then
  echo "エラー: 展開後に nn.bin が見つかりません。" >&2
  exit 1
fi
echo "完了: $OUT ($(du -h "$OUT" | cut -f1))"
