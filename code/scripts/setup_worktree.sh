#!/usr/bin/env bash
# `git worktree add` で作成した新規ディレクトリをビルド可能な状態にする。
#
# git worktree は .gitignore 対象のファイル(YaneuraOu submoduleの中身、
# NNUE評価関数 assets/eval/nn.bin)を引き継がないため、新規worktreeでは
# 素のままだとビルドに失敗する(サブモジュール未初期化)か、評価関数抜きの
# 壊れたビルドがサイレントに出来上がる(nn.bin未取得。2026-08-13に実際に
# 発生し、欠陥ビルドをPlay Console提出まで行ってしまった事故がある)。
#
# git worktree add の直後に、このスクリプトを実行すること。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# YaneuraOu は2つの submodule を持つ(Issue #346)。
#   native/third_party/YaneuraOu          V9.00(水匠5 / AobaNNUE)
#   native/third_party/YaneuraOu-tsuyoura 9.80git(つようら王11mini)
echo "== YaneuraOu submodule(V9.00 / つようら王用 9.80git)を初期化 =="
git submodule update --init --recursive

echo "== YaneuraOu submoduleへローカルpatchを適用(native/patches と native/patches/tsuyoura)=="
bash native/apply_patches.sh

echo "== 評価関数(水匠5 nn.bin)を取得 =="
bash scripts/fetch_eval.sh

echo "== 完了。ビルド前の確認 =="
echo "  submodule(V9.00)    : $(test -f native/third_party/YaneuraOu/README.md && echo OK || echo NG)"
echo "  submodule(つようら王): $(test -f native/third_party/YaneuraOu-tsuyoura/README.md && echo OK || echo NG)"
echo "  nn.bin   : $(du -h assets/eval/nn.bin 2>/dev/null | cut -f1 || echo NG)"
