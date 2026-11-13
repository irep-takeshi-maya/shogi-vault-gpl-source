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
# 冪等性: patch が触るファイルについて、pin のままの内容に patch を1件ずつ当てた結果を
#   一時ディレクトリで作り、submodule の今の内容が「何件目まで当てた状態」かを調べて、
#   残りだけを当てる。二重適用やCI再実行時のエラーを避ける。
#   (以前は patch ごとに `git apply --check --reverse` で判定していたが、後の patch が
#   前の patch の変更箇所を書き換えると(tsuyoura/0006 は 0001 が足した go_mate() を直す)、
#   前の patch が未適用に見えて当て直そうとし、失敗して止まる)
#   どの状態とも一致しない(手で書き換えた等)ときは止まる。
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

  # patch が触るファイル(pin に無いファイル=新規作成は「無い」状態として比べる)
  local files=()
  local f
  while IFS= read -r f; do files+=("$f"); done < <(sed -n 's|^+++ b/||p' "${patches[@]}" | sort -u)

  local tmp
  tmp="$(mktemp -d)"
  mkdir -p "$tmp/work"
  for f in "${files[@]}"; do
    mkdir -p "$tmp/work/$(dirname "$f")"
    git -C "$yo_dir" show "HEAD:$f" > "$tmp/work/$f" 2>/dev/null || rm -f "$tmp/work/$f"
  done

  # 今の submodule の内容が、一時ディレクトリの内容と同じか
  same_as_work() {
    local g
    for g in "${files[@]}"; do
      if [ -e "$tmp/work/$g" ]; then
        cmp -s "$tmp/work/$g" "$yo_dir/$g" || return 1
      else
        [ ! -e "$yo_dir/$g" ] || return 1
      fi
    done
  }

  # k 件目まで当てた状態と一致する k を探す(0 = 何も当てていない)
  local applied=-1 i
  same_as_work && applied=0
  for i in "${!patches[@]}"; do
    # 一時ディレクトリは Git の作業ディレクトリではないので、git apply はただの patch として当てる
    (cd "$tmp/work" && GIT_CEILING_DIRECTORIES="$tmp" git apply "${patches[$i]}")
    same_as_work && applied=$((i + 1))
  done
  rm -rf "$tmp"

  if [ "$applied" -lt 0 ]; then
    echo "エラー: [$label] の内容が、patch を何件目まで当てた状態とも一致しません(手で書き換えた等)。" >&2
    echo "  git -C \"$yo_dir\" checkout -- . で pin の状態に戻してから、もう一度実行してください。" >&2
    exit 1
  fi

  local name
  for i in "${!patches[@]}"; do
    name="$(basename "${patches[$i]}")"
    if [ "$i" -lt "$applied" ]; then
      echo "[$label] 適用済みのためスキップ: $name"
      continue
    fi
    echo "[$label] 適用中: $name"
    git -C "$yo_dir" apply --check "${patches[$i]}"
    git -C "$yo_dir" apply "${patches[$i]}"
  done
}

# 水匠5 / AobaNNUE 用(V9.00)
apply_to "$ROOT/third_party/YaneuraOu" "$ROOT/patches"
# つようら王11mini 用(9.80git)
apply_to "$ROOT/third_party/YaneuraOu-tsuyoura" "$ROOT/patches/tsuyoura"

echo "完了: native/patches の適用処理が終わりました。"
