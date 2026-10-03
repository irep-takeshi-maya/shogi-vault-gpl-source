# AobaNNUE(halfkp_768x2-16-64)用の第2エンジン pod。
#
# NNUE のネットワーク構造は YaneuraOu 本体にビルド時固定で焼き込まれるため、
# 水匠5(halfkp_256x2-32-32)とは**別のバイナリ**が要る(Issue #134/#282)。
# 実行時に切り替えることはできない。
#
# 同一プロセスへ2つ載せるため、公開するエントリ関数名を
# `YANEURAOU_BRIDGE_SYMBOL` で分ける(yaneuraou_start / aobannue_start)。
# YaneuraOu 本体の内部シンボルは重複するが、`use_frameworks!` により各 pod は
# 独立した framework になり、Darwin の two-level namespace のもとで
# それぞれ自分の定義を参照するため衝突しない。
#
# アーキテクチャ定義は submodule を改変せず、本体の nnue_architecture.h が持つ
# `NNUE_ARCHITECTURE_HEADER` フック経由で native/arch/ のヘッダを読ませる。
require_relative 'engine_sources'

Pod::Spec.new do |s|
  s.name             = 'aobannue_engine'
  s.version          = '0.1.0'
  s.summary          = 'Embedded YaneuraOu engine built for the AobaNNUE (halfkp_768x2-16-64) eval'
  s.description      = 'Compiles YaneuraOu + the socket bridge (aobannue_start) into the app.'
  s.homepage         = 'https://github.com/irep-takeshi-maya/shogi-vault'
  s.license          = { :type => 'GPLv3' }
  s.author           = { 'shogi-vault' => 'noreply@example.com' }
  s.source           = { :path => '.' }
  s.platform         = :ios, '14.0'
  s.requires_arc     = false

  s.source_files = YaneuraOuEngineSources.source_files

  s.public_header_files = 'bridge/engine_bridge.h'

  s.pod_target_xcconfig = YaneuraOuEngineSources.xcconfig(
    'NNUE_ARCHITECTURE_HEADER=\\"halfkp_768x2-16-64.h\\" ' \
    'YANEURAOU_BRIDGE_SYMBOL=aobannue_start'
  )
end
