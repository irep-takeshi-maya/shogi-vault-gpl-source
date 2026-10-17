# 組込み YaneuraOu エンジンの podspec 3 つ(水匠5用 / AobaNNUE用 / つようら王11mini用)で共有する
# ソース一覧と共通ビルド設定。
#
# 評価関数のアーキテクチャは YaneuraOu 本体にビルド時固定で焼き込まれるため、
# 別アーキテクチャの評価関数には別バイナリが要る(Issue #134/#282)。ソース一覧は
# ほぼ同じなので、各 podspec へコピーすると必ず片方だけ更新されて壊れる。
# ここに1本化しておく。
#
# YaneuraOu のソースは2つの版を持つ(Issue #346):
#   :v900  native/third_party/YaneuraOu          V9.00 (a5ee2786)       水匠5 / AobaNNUE
#   :v980  native/third_party/YaneuraOu-tsuyoura master c1b80eaa (9.80git) つようら王11mini
# つようら王の arch(SFNN ka2 / progress8)は V9.00 に無いため、そのバイナリだけ 9.80git
# でビルドする。ソース一覧・include パスとも、必ず同じ版を使う(版の違うヘッダを混ぜない)。
#
# ソース一覧・定義は native/CMakeLists.txt と一致させること。
module YaneuraOuEngineSources
  SOURCE_DIRS = {
    v900: 'third_party/YaneuraOu',
    v980: 'third_party/YaneuraOu-tsuyoura',
  }.freeze

  def self.source_dir(version)
    SOURCE_DIRS.fetch(version) do
      raise ArgumentError, "unknown YaneuraOu version: #{version.inspect} (:v900 or :v980)"
    end
  end

  # submodule が未取得だと、CocoaPods は存在しないソースをエラーにせず黙って無視し、
  # bridge だけの壊れた framework ができる(native/README.md「未確定/要注意」)。
  # 2つ目の submodule(YaneuraOu-tsuyoura)は、既存のチェックアウトでは pull しただけでは
  # 取得されないので、ここで止める。
  def self.verify_source_dir!(version)
    marker = File.join(__dir__, source_dir(version), 'source', 'types.cpp')
    return if File.exist?(marker)

    raise <<~MSG
      YaneuraOu のソース native/#{source_dir(version)} が見つかりません(submodule 未取得)。
      次のコマンドを実行してから、再度 pod install / ビルドしてください:
        bash scripts/setup_worktree.sh
      (= git submodule update --init --recursive && bash native/apply_patches.sh)
    MSG
  end

  def self.source_files(version = :v900)
    verify_source_dir!(version)
    yo = "#{source_dir(version)}/source"
    common = [
      'bridge/engine_bridge.cpp',
      'bridge/engine_bridge.h',
      # --- YaneuraOu 本体(NNUE の基本セット)。main.cpp は除外。
      # V9.00化(Issue #134)でファイル一覧を刷新。native/CMakeLists.txt のコメント参照。
      "#{yo}/types.cpp",
      "#{yo}/bitboard.cpp",
      "#{yo}/misc.cpp",
      "#{yo}/memory.cpp",
      "#{yo}/movegen.cpp",
      "#{yo}/position.cpp",
      "#{yo}/usi.cpp",
      "#{yo}/usioption.cpp",
      "#{yo}/thread.cpp",
      "#{yo}/tt.cpp",
      "#{yo}/movepick.cpp",
      "#{yo}/timeman.cpp",
      "#{yo}/engine.cpp",
      "#{yo}/search.cpp",
      "#{yo}/score.cpp",
      "#{yo}/benchmark.cpp",
      "#{yo}/tune.cpp",
      "#{yo}/book/book.cpp",
      "#{yo}/book/apery_book.cpp",
      "#{yo}/book/policybook.cpp",
      # makebook.cpp は ENABLE_MAKEBOOK_CMD 経由で usi.cpp から常時参照されるため必須。
      "#{yo}/book/makebook.cpp",
      "#{yo}/book/makebook2025.cpp",
      "#{yo}/extra/bitop.cpp",
      "#{yo}/extra/long_effect.cpp",
      "#{yo}/extra/sfen_packer.cpp",
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
      "#{yo}/engine/yaneuraou-engine/yaneuraou-search.cpp",
      # --- NNUE
      "#{yo}/eval/nnue/evaluate_nnue.cpp",
      "#{yo}/eval/nnue/nnue_test_command.cpp",
      "#{yo}/eval/nnue/features/k.cpp",
      "#{yo}/eval/nnue/features/p.cpp",
      "#{yo}/eval/nnue/features/half_kp.cpp",
      "#{yo}/eval/nnue/features/half_kp_vm.cpp",
      "#{yo}/eval/nnue/features/half_relative_kp.cpp",
      "#{yo}/eval/nnue/features/half_kpe9.cpp",
      "#{yo}/eval/nnue/features/pe9.cpp",
    ]

    if version == :v900
      common + [
        # V9.00 のみ。makebook2015.cpp が参照する学習系シンボルのリンクに learn/ が要る。
        "#{yo}/book/makebook2015.cpp",
        "#{yo}/learn/learner.cpp",
        "#{yo}/learn/learning_tools.cpp",
        "#{yo}/learn/multi_think.cpp",
        "#{yo}/eval/nnue/evaluate_nnue_learner.cpp",
      ]
    else
      common + [
        # 9.80git のみ。makebook2015.cpp / learn/ / evaluate_nnue_learner.cpp は本家で削除され、
        # SFNN 系の特徴量(a2 = つようら王の ka2 が使う、half_ka*)が増えた。
        "#{yo}/eval/nnue/features/a2.cpp",
        "#{yo}/eval/nnue/features/half_ka1.cpp",
        "#{yo}/eval/nnue/features/half_ka_hm1.cpp",
        "#{yo}/eval/nnue/features/half_ka2.cpp",
        "#{yo}/eval/nnue/features/half_ka_hm2.cpp",
      ]
    end
  end

  # GCC_PREPROCESSOR_DEFINITIONS の共通部分。
  # 評価関数アーキテクチャの指定とエントリ関数名は呼び出し側で足す。
  def self.base_definitions
    'NDEBUG=1 _LINUX=1 UNICODE=1 NO_EXCEPTIONS=1 IS_64BIT=1 ' \
      'YANEURAOU_ENGINE_NNUE=1'
  end

  # pod_target_xcconfig の共通部分。[arch_definitions] に評価関数アーキテクチャ
  # 指定(例 'EVAL_NNUE_HALFKP256=1')とエントリ関数名の定義を渡す。
  #
  # USE_NEON は実機(iphoneos SDK)のみ付与する。iOS Simulator SDK のヘッダは
  # arm_neon.h の一部組込み関数(vmull_s8等)が未定義でコンパイルエラーになるため
  # (Apple Silicon Mac上のarm64シミュレータでも発生)、シミュレータ向けは
  # YaneuraOu本体が備える非NEONの汎用フォールバック実装(affine_transform.h等の
  # #else分岐)を使う。実機ビルドの挙動・性能は変わらない。
  #
  # [version] は使う YaneuraOu の版(:v900 / :v980)。source_files と必ず揃える。
  def self.xcconfig(arch_definitions, version = :v900)
    {
      'HEADER_SEARCH_PATHS' =>
        "\"$(PODS_TARGET_SRCROOT)/#{source_dir(version)}/source\" " \
        '"$(PODS_TARGET_SRCROOT)/bridge" ' \
        '"$(PODS_TARGET_SRCROOT)/arch"',
      'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
      'CLANG_CXX_LIBRARY' => 'libc++',
      'GCC_PREPROCESSOR_DEFINITIONS' =>
        "#{base_definitions} #{arch_definitions}",
      'GCC_PREPROCESSOR_DEFINITIONS[sdk=iphoneos*]' => '$(inherited) USE_NEON=1',
      # -fvisibility=hidden は**必須**。これが無いと YaneuraOu 本体の C++ シンボルが
      # 両 framework から約970個ずつ export され、動的リンカが呼び出し先を取り違える。
      # 実際に、水匠5側の探索が AobaNNUE 側の未初期化 TranspositionTable を呼んで
      # SIGSEGV するクラッシュを起こした(Issue #134、シミュレータで再現・修正確認済み)。
      # ブリッジのエントリ関数だけは engine_bridge.h で
      # __attribute__((visibility("default"))) を付けてあるので export される。
      'OTHER_CPLUSPLUSFLAGS' =>
        '-fno-exceptions -fno-rtti -fpermissive -w ' \
        '-fvisibility=hidden -fvisibility-inlines-hidden',
      # dlsym で解決するエントリ関数をストリップしない。
      'DEAD_CODE_STRIPPING' => 'NO',
    }
  end
end
