# 組込み YaneuraOu(水匠5 NNUE)エンジンを iOS の Runner に静的リンクする
# CocoaPods ローカル pod。Android の Gradle+CMake と同じソース/定義で、
# `flutter build ios` の一工程でコンパイルされる。
#
# ソース一覧・定義は native/CMakeLists.txt と一致させること。
Pod::Spec.new do |s|
  s.name             = 'yaneuraou_engine'
  s.version          = '0.1.0'
  s.summary          = 'Embedded YaneuraOu (Suisho5 NNUE) engine for on-device shogi analysis'
  s.description      = 'Compiles YaneuraOu + the socket bridge (yaneuraou_start) into the app.'
  s.homepage         = 'https://github.com/irep-takeshi-maya/shogi-vault'
  s.license          = { :type => 'GPLv3' }
  s.author           = { 'shogi-vault' => 'noreply@example.com' }
  s.source           = { :path => '.' }
  s.platform         = :ios, '13.0'
  s.requires_arc     = false

  yo = 'third_party/YaneuraOu/source'

  s.source_files = [
    'bridge/engine_bridge.cpp',
    'bridge/engine_bridge.h',
    # --- YaneuraOu 本体(material の基本セット)。main.cpp は除外。
    "#{yo}/types.cpp",
    "#{yo}/bitboard.cpp",
    "#{yo}/misc.cpp",
    "#{yo}/movegen.cpp",
    "#{yo}/position.cpp",
    "#{yo}/usi.cpp",
    "#{yo}/usi_option.cpp",
    "#{yo}/thread.cpp",
    "#{yo}/tt.cpp",
    "#{yo}/movepick.cpp",
    "#{yo}/timeman.cpp",
    "#{yo}/book/book.cpp",
    "#{yo}/book/apery_book.cpp",
    "#{yo}/extra/bitop.cpp",
    "#{yo}/extra/long_effect.cpp",
    "#{yo}/extra/sfen_packer.cpp",
    "#{yo}/extra/super_sort.cpp",
    "#{yo}/mate/mate.cpp",
    "#{yo}/mate/mate1ply_without_effect.cpp",
    "#{yo}/mate/mate1ply_with_effect.cpp",
    "#{yo}/mate/mate_solver.cpp",
    "#{yo}/eval/evaluate_bona_piece.cpp",
    "#{yo}/eval/evaluate.cpp",
    "#{yo}/eval/evaluate_io.cpp",
    "#{yo}/eval/evaluate_mir_inv_tools.cpp",
    "#{yo}/eval/material/evaluate_material.cpp",
    "#{yo}/testcmd/unit_test.cpp",
    "#{yo}/testcmd/mate_test_cmd.cpp",
    "#{yo}/testcmd/normal_test_cmd.cpp",
    "#{yo}/testcmd/benchmark.cpp",
    "#{yo}/engine/yaneuraou-engine/yaneuraou-search.cpp",
    # --- NNUE(水匠5 = 標準 halfKP256)
    "#{yo}/eval/nnue/evaluate_nnue.cpp",
    "#{yo}/eval/nnue/evaluate_nnue_learner.cpp",
    "#{yo}/eval/nnue/nnue_test_command.cpp",
    "#{yo}/eval/nnue/features/k.cpp",
    "#{yo}/eval/nnue/features/p.cpp",
    "#{yo}/eval/nnue/features/half_kp.cpp",
    "#{yo}/eval/nnue/features/half_kp_vm.cpp",
    "#{yo}/eval/nnue/features/half_relative_kp.cpp",
    "#{yo}/eval/nnue/features/half_kpe9.cpp",
    "#{yo}/eval/nnue/features/pe9.cpp",
  ]

  # Dart(dart:ffi)から dlsym する公開ヘッダ。
  s.public_header_files = 'bridge/engine_bridge.h'

  # YaneuraOu のヘッダは source/ 相対で #include されるため探索パスを通す。
  # 定義は native/CMakeLists.txt と一致(iOS 実機は arm64 = USE_NEON)。
  # 評価関数は実行時に EvalDir から読むため EVAL_EMBEDDING は付けない。
  #
  # USE_NEON は実機(iphoneos SDK)のみ付与する。iOS Simulator SDK のヘッダは
  # arm_neon.h の一部組込み関数(vmull_s8等)が未定義でコンパイルエラーになるため
  # (Apple Silicon Mac上のarm64シミュレータでも発生)、シミュレータ向けは
  # YaneuraOu本体が備える非NEONの汎用フォールバック実装(affine_transform.h等の
  # #else分岐)を使う。実機ビルドの挙動・性能は変わらない。
  s.pod_target_xcconfig = {
    'HEADER_SEARCH_PATHS' => '"$(PODS_TARGET_SRCROOT)/third_party/YaneuraOu/source" "$(PODS_TARGET_SRCROOT)/bridge"',
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
    'CLANG_CXX_LIBRARY' => 'libc++',
    'GCC_PREPROCESSOR_DEFINITIONS' => 'NDEBUG=1 _LINUX=1 UNICODE=1 NO_EXCEPTIONS=1 IS_64BIT=1 YANEURAOU_ENGINE_NNUE=1 EVAL_NNUE_HALFKP256=1',
    'GCC_PREPROCESSOR_DEFINITIONS[sdk=iphoneos*]' => '$(inherited) USE_NEON=1',
    'OTHER_CPLUSPLUSFLAGS' => '-fno-exceptions -fno-rtti -fpermissive -w',
    # dlsym で解決するため公開シンボルをストリップしない。
    'DEAD_CODE_STRIPPING' => 'NO',
  }
end
