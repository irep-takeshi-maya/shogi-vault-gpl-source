# 組込みエンジン libyaneuraou を生成する CMake。
#
# Flutter Android の externalNativeBuild から呼ばれ、abiFilters(arm64-v8a/x86_64)
# ごとに libyaneuraou.so を生成して APK に同梱する。
#
# 【段階2-a】まず評価関数が不要な "material" エンジン(YANEURAOU_ENGINE_MATERIAL)
# で YaneuraOu 本体が Android NDK でクロスコンパイル/リンクできることを確定する。
# 弱いが本物の探索が動く。強い NNUE への差し替えは段階2-b(要 embedded_nnue.cpp)。
#
# ソース一覧・定義は YaneuraOu 本体の source/Makefile(SOURCES / MATERIAL 節)に準拠。
#
# v8.60git 化(Issue #29)に伴い memory.cpp を追加。tt.cpp/tt.h・numa.h(header-only)が
# 新たに memory.h(aligned_large_pages_alloc 等)へ依存するようになったため。

cmake_minimum_required(VERSION 3.22)
project(yaneuraou CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(YO "${CMAKE_CURRENT_SOURCE_DIR}/third_party/YaneuraOu/source")

# --- YaneuraOu 本体(material 構成)。main.cpp は除外し、ブリッジで置き換える。
set(YO_SRC
    ${YO}/types.cpp
    ${YO}/bitboard.cpp
    ${YO}/misc.cpp
    ${YO}/memory.cpp
    ${YO}/movegen.cpp
    ${YO}/position.cpp
    ${YO}/usi.cpp
    ${YO}/usi_option.cpp
    ${YO}/thread.cpp
    ${YO}/tt.cpp
    ${YO}/movepick.cpp
    ${YO}/timeman.cpp
    ${YO}/book/book.cpp
    ${YO}/book/apery_book.cpp
    ${YO}/extra/bitop.cpp
    ${YO}/extra/long_effect.cpp
    ${YO}/extra/sfen_packer.cpp
    ${YO}/extra/super_sort.cpp
    ${YO}/mate/mate.cpp
    ${YO}/mate/mate1ply_without_effect.cpp
    ${YO}/mate/mate1ply_with_effect.cpp
    ${YO}/mate/mate_solver.cpp
    ${YO}/eval/evaluate_bona_piece.cpp
    ${YO}/eval/evaluate.cpp
    ${YO}/eval/evaluate_io.cpp
    ${YO}/eval/evaluate_mir_inv_tools.cpp
    ${YO}/eval/material/evaluate_material.cpp
    ${YO}/testcmd/unit_test.cpp
    ${YO}/testcmd/mate_test_cmd.cpp
    ${YO}/testcmd/normal_test_cmd.cpp
    ${YO}/testcmd/benchmark.cpp
    ${YO}/engine/yaneuraou-engine/yaneuraou-search.cpp
    # NNUE(水匠5 = HalfKP KP256)。Makefile の NNUE 節に準拠。
    # 評価関数(nn.bin)は埋め込まず、実行時に EvalDir から読む(EVAL_EMBEDDING 無し)。
    ${YO}/eval/nnue/evaluate_nnue.cpp
    ${YO}/eval/nnue/evaluate_nnue_learner.cpp
    ${YO}/eval/nnue/nnue_test_command.cpp
    ${YO}/eval/nnue/features/k.cpp
    ${YO}/eval/nnue/features/p.cpp
    ${YO}/eval/nnue/features/half_kp.cpp
    ${YO}/eval/nnue/features/half_kp_vm.cpp
    ${YO}/eval/nnue/features/half_relative_kp.cpp
    ${YO}/eval/nnue/features/half_kpe9.cpp
    ${YO}/eval/nnue/features/pe9.cpp
)

add_library(yaneuraou SHARED
    ${YO_SRC}
    bridge/engine_bridge.cpp
)

target_include_directories(yaneuraou PRIVATE ${YO} bridge)

# 共通定義(Makefile の base + NNUE 節に準拠)。
# 水匠5 は HalfKP KP256 → EVAL_NNUE_KP256。EVAL_EMBEDDING は付けず、
# nn.bin は実行時に EvalDir から読み込む。
target_compile_definitions(yaneuraou PRIVATE
    NDEBUG
    _LINUX
    UNICODE
    NO_EXCEPTIONS
    IS_64BIT
    YANEURAOU_ENGINE_NNUE
    # 水匠5 は標準 halfKP256(HalfKP 256x2-32-32)。EVAL_NNUE_KP256 は別物の
    # 小型 k-p アーキなので nn.bin のアーキハッシュ不一致で読込失敗し exit する。
    EVAL_NNUE_HALFKP256
)

# 参照実装(iOS clang)で実績のあるフラグ。-w で本体の警告を抑制。
target_compile_options(yaneuraou PRIVATE
    -O2
    -fno-exceptions
    -fno-rtti
    -fpermissive
    -w
)

# アーキテクチャ別 SIMD(Makefile: arm=NEON / x86_64=SSE4.2)。
if(ANDROID_ABI STREQUAL "arm64-v8a")
    target_compile_definitions(yaneuraou PRIVATE USE_NEON)
elseif(ANDROID_ABI STREQUAL "x86_64")
    target_compile_definitions(yaneuraou PRIVATE USE_SSE42)
    target_compile_options(yaneuraou PRIVATE -msse4.2)
endif()
