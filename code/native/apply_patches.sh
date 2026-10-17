#!/usr/bin/env bash
# YaneuraOu の submodule(本家の pin コミット)に、アプリ固有の patch を当てる。
#
# YaneuraOu のソースは2つの版を持っている(Issue #346):
#
#   submodule                              pin                        エンジン           patch
#   native/third_party/YaneuraOu           V9.00 (a5ee2786)           水匠5 / AobaNNUE   native/patches/*.patch
#   native/third_party/YaneuraOu-tsuyoura  master c1b80eaa (9.80git)  つようら王11mini   native/patches/tsuyoura/*.patch
#
# 版が違うと同じ改変でも patch の文脈が合わないため、ソースごとに patch を分けて持つ。
# native/patches/*.patch は直下のファイルだけが対象で、tsuyoura/ の中は V9.00 に当てない。
#
# なぜsubmodule本体を直接コミットしないのか:
#   .gitmodules は https://github.com/yaneurao/YaneuraOu (本家、フォークではない)を
#   指しており、ローカルの改変コミットをここへpushすることはできない。
#   submoduleのpin先コミット自体は本家のものをそのまま保つことで、
#   `git submodule update --init --recursive` によるチェックアウトの再現性を
#   保証しつつ、アプリ固有の改変は patch ファイルとして本リポジトリ側で管理する。
#
# 冪等性: 適用済みかどうかを `git apply --check --reverse` で判定し、
#   二重適用やCI再実行時のエラーを避ける。
#
# 実行タイミング: `git submodule update --init --recursive` の直後、
#   ビルド(pod install / CMake)より前に必ず実行すること。
#   scripts/setup_worktree.sh と .github/workflows/mobile-build.yml から呼ばれる。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"

# apply_to <submodule のディレクトリ> <patch のディレクトリ>
apply_to() {
  local yo_dir="$1"
  local patch_dir="$2"
  local label="${yo_dir#"$ROOT"/}"

  if [ ! -d "$yo_dir/.git" ] && [ ! -f "$yo_dir/.git" ]; then
    echo "エラー: $yo_dir がgitリポジトリではありません。先に" >&2
    echo "  git submodule update --init --recursive" >&2
    echo "を実行してください。" >&2
    exit 1
  fi

  local patches=()
  local p
  for p in "$patch_dir"/*.patch; do
    [ -f "$p" ] && patches+=("$p")
  done

  if [ ${#patches[@]} -eq 0 ]; then
    echo "[$label] 適用対象のpatchがありません: $patch_dir"
    return 0
  fi

  local patch name
  for patch in "${patches[@]}"; do
    name="$(basename "$patch")"

    if git -C "$yo_dir" apply --check --reverse "$patch" >/dev/null 2>&1; then
      echo "[$label] 適用済みのためスキップ: $name"
      continue
    fi

    echo "[$label] 適用中: $name"
    git -C "$yo_dir" apply --check "$patch"
    git -C "$yo_dir" apply "$patch"
  done
}

# 水匠5 / AobaNNUE 用(V9.00)
apply_to "$ROOT/third_party/YaneuraOu" "$ROOT/patches"
# つようら王11mini 用(9.80git)
apply_to "$ROOT/third_party/YaneuraOu-tsuyoura" "$ROOT/patches/tsuyoura"

echo "完了: native/patches の適用処理が終わりました。"
