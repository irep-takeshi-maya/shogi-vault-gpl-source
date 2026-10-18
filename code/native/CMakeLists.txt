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

# --- YaneuraOu のソースは2つの版を持つ(Issue #346)。
#
#   V900: native/third_party/YaneuraOu          V9.00 (a5ee2786)。水匠5 / AobaNNUE
#   V980: native/third_party/YaneuraOu-tsuyoura master c1b80eaa (9.80git)。つようら王11mini
#
# つようら王11mini の arch(SFNN ka2 / progress8)は V9.00 に無いため、そのバイナリ
# だけを 9.80git でビルドする。開発版の影響を Pro の新機能だけに閉じ込めるためで、
# 本家が次の安定版を出したら1つに揃える。patch もソースごとに分けてある
# (native/patches/ と native/patches/tsuyoura/、native/apply_patches.sh 参照)。
set(YO_V900 "${CMAKE_CURRENT_SOURCE_DIR}/third_party/YaneuraOu/source")
set(YO_V980 "${CMAKE_CURRENT_SOURCE_DIR}/third_party/YaneuraOu-tsuyoura/source")

# yaneuraou_sources(<出力変数> <V900|V980>)
# YaneuraOu 本体(NNUE 構成)のソース一覧を返す。main.cpp は除外し、ブリッジで置き換える。
# 版による違いは本家 source/Makefile の SOURCES / NNUE 節に合わせている。
# native/engine_sources.rb(iOS)と一致させること。
function(yaneuraou_sources out_var yo_version)
    set(YO "${YO_${yo_version}}")
    if(YO STREQUAL "")
        message(FATAL_ERROR "yaneuraou_sources: 不明な版 ${yo_version}(V900 か V980)")
    endif()

    set(src
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
        # makebook.cpp は ENABLE_MAKEBOOK_CMD(YANEURAOU_ENGINE 系では config.h で
        # 無条件に定義される)経由で usi.cpp から Book::makebook() が常時呼ばれる
        # ため必須。実際に定跡生成コマンドを使わなくてもリンクは必要。
        ${YO}/book/makebook.cpp
        ${YO}/book/makebook2025.cpp
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
        # NNUE。Makefile の NNUE 節に準拠。
        # 評価関数(nn.bin)は埋め込まず、実行時に EvalDir から読む(EVAL_EMBEDDING 無し)。
        ${YO}/eval/nnue/evaluate_nnue.cpp
        ${YO}/eval/nnue/nnue_test_command.cpp
        ${YO}/eval/nnue/features/k.cpp
        ${YO}/eval/nnue/features/p.cpp
        ${YO}/eval/nnue/features/half_kp.cpp
        ${YO}/eval/nnue/features/half_kp_vm.cpp
        ${YO}/eval/nnue/features/half_relative_kp.cpp
        ${YO}/eval/nnue/features/half_kpe9.cpp
        ${YO}/eval/nnue/features/pe9.cpp
    )

    if(yo_version STREQUAL "V900")
        list(APPEND src
            # V9.00 のみ。makebook2015.cpp が参照する学習系シンボル(Learner::search等)の
            # リンクに learn/ が要る。EVAL_LEARN を定義しない限り実処理は #if で無効化されたまま。
            ${YO}/book/makebook2015.cpp
            ${YO}/learn/learner.cpp
            ${YO}/learn/learning_tools.cpp
            ${YO}/learn/multi_think.cpp
            ${YO}/eval/nnue/evaluate_nnue_learner.cpp
        )
    else()
        list(APPEND src
            # 9.80git のみ。makebook2015.cpp / learn/ / evaluate_nnue_learner.cpp は本家で
            # 削除され、SFNN 系の特徴量(a2 = つようら王の ka2 が使う、half_ka*)が増えた。
            ${YO}/eval/nnue/features/a2.cpp
            ${YO}/eval/nnue/features/half_ka1.cpp
            ${YO}/eval/nnue/features/half_ka_hm1.cpp
            ${YO}/eval/nnue/features/half_ka2.cpp
            ${YO}/eval/nnue/features/half_ka_hm2.cpp
        )
    endif()

    set(${out_var} ${src} PARENT_SCOPE)
endfunction()

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
#
# yo_version は使う YaneuraOu の版(V900 / V980、上の yaneuraou_sources 参照)。
#
# 5つ目より後ろの引数(ARGN)は、そのバイナリの全ソースに付ける追加の定義。
# SFNN 系(つようら王)は SFNNwoPSQT が要る。position.h の StateInfo の中身が
# 変わるため、評価関数のソースだけでなく全ソースに揃えて付ける必要がある。
function(add_yaneuraou_engine target_name bridge_symbol yo_version arch_define arch_header)
    yaneuraou_sources(yo_src ${yo_version})
    add_library(${target_name} SHARED
        ${yo_src}
        bridge/engine_bridge.cpp
    )

    # include パスも同じ版のソースを向ける(版の違うヘッダを混ぜない)。
    target_include_directories(${target_name} PRIVATE
        ${YO_${yo_version}}
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

    if(ARGN)
        target_compile_definitions(${target_name} PRIVATE ${ARGN})
    endif()

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
add_yaneuraou_engine(yaneuraou yaneuraou_start V900 EVAL_NNUE_HALFKP256 "")

# AobaNNUE(アプリ内ダウンロード、halfkp_768x2-16-64)。
# ヘッダ名だけを渡し、実体は native/arch/ を include パスから解決する。
add_yaneuraou_engine(aobannue aobannue_start V900 "" "halfkp_768x2-16-64.h")

# つようら王11mini(アプリ内ダウンロード、Pro 限定、SFNN_ka2_4096_8_64_progress8)。
# SFNN 系なので SFNNwoPSQT を全ソースに付ける。V9.00 には ka2 / progress8 が無いため、
# このバイナリだけ 9.80git(native/third_party/YaneuraOu-tsuyoura)でビルドする(Issue #346)。
# USE_PTHREADS: 探索スレッドを std::thread(Android の既定スタックは約1MB)ではなく
# 8MB スタックの pthread で作らせる(thread_win32_osx.h。Apple では元から有効)。
# SFNN は局面ごとの状態が大きく、1MB では SearchManager::pv() でスタックが溢れて
# SIGSEGV になる(Galaxy A20 実機で、解析開始直後に毎回落ちた)。
add_yaneuraou_engine(tsuyoura tsuyoura_start V980 "" "SFNN_ka2_4096_8_64_progress8.h" SFNNwoPSQT USE_PTHREADS)
