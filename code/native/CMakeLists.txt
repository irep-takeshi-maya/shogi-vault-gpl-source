# 組込みエンジン libyaneuraou を生成する CMake。
#
# Flutter Android の externalNativeBuild から呼ばれ、abiFilters(arm64-v8a/x86_64)
# ごとに libyaneuraou.so を生成して APK に同梱する。
#
# 【段階2-a】まず評価関数が不要な "material" エンジン(YANEURAOU_ENGINE_MATERIAL)
# で YaneuraOu 本体が Android NDK でクロスコンパイル/リンクできることを確定する。
# 弱いが本物の探索が動く。強い NNUE への差し替えは段階2-b(要 embedded_nnue.cpp)。
#
# ソース一覧・定義は YaneuraOu 本体の source/Makefile(SOURCES / NNUE 節)に準拠。
#
# v8.60git 化(Issue #29)に伴い memory.cpp を追加。tt.cpp/tt.h・numa.h(header-only)が
# 新たに memory.h(aligned_large_pages_alloc 等)へ依存するようになったため。
#
# V9.00 化(Issue #134)に伴いソース一覧を全面更新。Stockfish本家に合わせた大規模
# リファクタで engine.cpp(旧: #if 0 で無効化されていたWIPコードだったが実体化)/
# search.cpp/score.cpp/benchmark.cpp/tune.cpp が新規に必須ファイルとなり、
# usi_option.cpp は usioption.cpp へ改名、testcmd/benchmark.cpp は削除され
# ルートの benchmark.cpp に統合、extra/super_sort.cpp は削除された。詳細は
# native/README.md の「段階5」を参照。

cmake_minimum_required(VERSION 3.22)
project(yaneuraou CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(YO "${CMAKE_CURRENT_SOURCE_DIR}/third_party/YaneuraOu/source")

# --- YaneuraOu 本体(NNUE 構成)。main.cpp は除外し、ブリッジで置き換える。
set(YO_SRC
    ${YO}/types.cpp
    ${YO}/bitboard.cpp
    ${YO}/misc.cpp
    ${YO}/memory.cpp
    ${YO}/movegen.cpp
    ${YO}/position.cpp
    ${YO}/usi.cpp
    ${YO}/usioption.cpp
    ${YO}/thread.cpp
    ${YO}/tt.cpp
    ${YO}/movepick.cpp
    ${YO}/timeman.cpp
    ${YO}/engine.cpp
    ${YO}/search.cpp
    ${YO}/score.cpp
    ${YO}/benchmark.cpp
    ${YO}/tune.cpp
    ${YO}/book/book.cpp
    ${YO}/book/apery_book.cpp
    ${YO}/book/policybook.cpp
    # makebook.cpp は ENABLE_MAKEBOOK_CMD(V9.00 の YANEURAOU_ENGINE 系では
    # config.h で無条件に定義される)経由で usi.cpp から Book::makebook() が
    # 常時呼ばれるため必須。実際に定跡生成コマンドを使わなくてもリンクは必要。
    ${YO}/book/makebook.cpp
    ${YO}/book/makebook2015.cpp
    ${YO}/book/makebook2025.cpp
    # makebook2015.cpp が参照する学習系シンボル(Learner::search等)のリンクに必要。
    # EVAL_LEARN を定義しない限り実処理は #if で無効化されたままなのでサイズ増程度の影響。
    ${YO}/learn/learner.cpp
    ${YO}/learn/learning_tools.cpp
    ${YO}/learn/multi_think.cpp
    ${YO}/extra/bitop.cpp
    ${YO}/extra/long_effect.cpp
    ${YO}/extra/sfen_packer.cpp
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

# --- 評価関数アーキテクチャごとに1つの共有ライブラリを作る。
#
# NNUE のネットワーク構造は YaneuraOu 本体にビルド時固定で焼き込まれるため、
# 別アーキテクチャの評価関数(AobaNNUE = halfkp_768x2-16-64)に対応するには
# **別のバイナリ**が要る(Issue #134/#282)。実行時に切り替えることはできない。
#
# 同一プロセスへ複数載せるため、公開するエントリ関数名を
# `YANEURAOU_BRIDGE_SYMBOL` で分ける。YaneuraOu 本体の内部シンボルは重複するが、
# Android では別々の .so として dlopen され、それぞれ独立したシンボル
# 名前空間を持つ(Dart の DynamicLibrary.open は RTLD_LOCAL 相当)ため衝突しない。
#
# arch_header にファイル名を渡すと、本体の nnue_architecture.h が持つ
# `NNUE_ARCHITECTURE_HEADER` フック経由で include される。ヘッダは
# native/arch/ を include パスに通して解決するので、脆い相対パスを書かずに済む。
# このフックのおかげで、アーキテクチャ追加に submodule の改変は要らない。
function(add_yaneuraou_engine target_name bridge_symbol arch_define arch_header)
    add_library(${target_name} SHARED
        ${YO_SRC}
        bridge/engine_bridge.cpp
    )

    target_include_directories(${target_name} PRIVATE
        ${YO}
        bridge
        ${CMAKE_CURRENT_SOURCE_DIR}/arch
    )

    # 共通定義(Makefile の base + NNUE 節に準拠)。
    # EVAL_EMBEDDING は付けず、nn.bin は実行時に EvalDir から読み込む。
    target_compile_definitions(${target_name} PRIVATE
        NDEBUG
        _LINUX
        UNICODE
        NO_EXCEPTIONS
        IS_64BIT
        YANEURAOU_ENGINE_NNUE
        YANEURAOU_BRIDGE_SYMBOL=${bridge_symbol}
    )

    if(arch_header STREQUAL "")
        # 既定(水匠5 = 標準 halfKP256)。EVAL_NNUE_KP256 は別物の小型 k-p アーキ
        # なので nn.bin のアーキハッシュ不一致で読込失敗し exit する。
        target_compile_definitions(${target_name} PRIVATE ${arch_define})
    else()
        target_compile_definitions(${target_name} PRIVATE
            "NNUE_ARCHITECTURE_HEADER=\"${arch_header}\""
        )
    endif()

    # 参照実装(iOS clang)で実績のあるフラグ。-w で本体の警告を抑制。
    #
    # -fvisibility=hidden は複数エンジンを同一プロセスへ載せるために必須。
    # 付けないと YaneuraOu 本体の C++ シンボルが各ライブラリから約970個ずつ
    # export され、動的リンカが呼び出し先を取り違える(iOS で実際に、水匠5側の
    # 探索が AobaNNUE 側の未初期化 TranspositionTable を呼んで SIGSEGV した)。
    # Android は dlopen が RTLD_LOCAL 相当のため表面化しにくいが、条件を
    # 揃えておく。ブリッジのエントリ関数だけは engine_bridge.h の
    # __attribute__((visibility("default"))) で export される。
    target_compile_options(${target_name} PRIVATE
        -O2
        -fno-exceptions
        -fno-rtti
        -fpermissive
        -w
        -fvisibility=hidden
        -fvisibility-inlines-hidden
    )

    # アーキテクチャ別 SIMD(Makefile: arm=NEON / x86_64=SSE4.2)。
    if(ANDROID_ABI STREQUAL "arm64-v8a")
        target_compile_definitions(${target_name} PRIVATE USE_NEON)
    elseif(ANDROID_ABI STREQUAL "x86_64")
        target_compile_definitions(${target_name} PRIVATE USE_SSE42)
        target_compile_options(${target_name} PRIVATE -msse4.2)
    endif()
endfunction()

# 水匠5(アプリ同梱、halfkp_256x2-32-32)。
add_yaneuraou_engine(yaneuraou yaneuraou_start EVAL_NNUE_HALFKP256 "")

# AobaNNUE(アプリ内ダウンロード、halfkp_768x2-16-64)。
# ヘッダ名だけを渡し、実体は native/arch/ を include パスから解決する。
add_yaneuraou_engine(aobannue aobannue_start "" "halfkp_768x2-16-64.h")
