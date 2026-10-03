# 組込み YaneuraOu(水匠5 NNUE = halfkp_256x2-32-32)エンジンを iOS の Runner へ
# 組み込む CocoaPods ローカル pod。Android の Gradle+CMake と同じソース/定義で、
# `flutter build ios` の一工程でコンパイルされる。
#
# ソース一覧・共通設定は engine_sources.rb に集約してある(AobaNNUE 用の
# aobannue_engine.podspec と共有)。
require_relative 'engine_sources'

Pod::Spec.new do |s|
  s.name             = 'yaneuraou_engine'
  s.version          = '0.1.0'
  s.summary          = 'Embedded YaneuraOu (Suisho5 NNUE) engine for on-device shogi analysis'
  s.description      = 'Compiles YaneuraOu + the socket bridge (yaneuraou_start) into the app.'
  s.homepage         = 'https://github.com/irep-takeshi-maya/shogi-vault'
  s.license          = { :type => 'GPLv3' }
  s.author           = { 'shogi-vault' => 'noreply@example.com' }
  s.source           = { :path => '.' }
  s.platform         = :ios, '14.0'
  s.requires_arc     = false

  s.source_files = YaneuraOuEngineSources.source_files

  # Dart(dart:ffi)から dlsym する公開ヘッダ。
  s.public_header_files = 'bridge/engine_bridge.h'

  # 水匠5 は標準 halfKP256(HalfKP 256x2-32-32)。EVAL_NNUE_KP256 は別物の
  # 小型 k-p アーキなので nn.bin のアーキハッシュ不一致で読込失敗し exit する。
  # 評価関数は実行時に EvalDir から読むため EVAL_EMBEDDING は付けない。
  s.pod_target_xcconfig = YaneuraOuEngineSources.xcconfig(
    'EVAL_NNUE_HALFKP256=1 YANEURAOU_BRIDGE_SYMBOL=yaneuraou_start'
  )
end
