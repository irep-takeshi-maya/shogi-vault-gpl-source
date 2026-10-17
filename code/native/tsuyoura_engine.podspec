# つようら王11mini(SFNN_ka2_4096_8_64_progress8)用の第3エンジン pod(Issue #346)。
#
# NNUE のネットワーク構造は YaneuraOu 本体にビルド時固定で焼き込まれるため、
# 水匠5・AobaNNUE(どちらも halfkp 系)とは**別のバイナリ**が要る。
#
# 同一プロセスへ3つ載せるため、公開するエントリ関数名を
# `YANEURAOU_BRIDGE_SYMBOL` で分ける(tsuyoura_start)。内部シンボルの
# 取り違えを防ぐ -fvisibility=hidden は engine_sources.rb の共通設定に入っている。
#
# SFNN 系は SFNNwoPSQT の定義が要る(本家 Makefile と同じ)。position.h の
# StateInfo の中身が変わるので、全ソースに付く pod_target_xcconfig に入れる。
# アーキテクチャ定義は native/arch/ のヘッダを `NNUE_ARCHITECTURE_HEADER` で読ませる。
#
# V9.00 には ka2 / progress8 が無いため、この pod だけ 9.80git(master c1b80eaa)の
# ソース native/third_party/YaneuraOu-tsuyoura を使う(:v980)。水匠5・AobaNNUE は
# V9.00 のまま(Issue #346)。patch も native/patches/tsuyoura/ を当てる。
require_relative 'engine_sources'

Pod::Spec.new do |s|
  s.name             = 'tsuyoura_engine'
  s.version          = '0.1.0'
  s.summary          = 'Embedded YaneuraOu engine built for the TsuyouraOu11 mini (SFNN_ka2_4096_8_64_progress8) eval'
  s.description      = 'Compiles YaneuraOu + the socket bridge (tsuyoura_start) into the app.'
  s.homepage         = 'https://github.com/irep-takeshi-maya/shogi-vault'
  s.license          = { :type => 'GPLv3' }
  s.author           = { 'shogi-vault' => 'noreply@example.com' }
  s.source           = { :path => '.' }
  s.platform         = :ios, '14.0'
  s.requires_arc     = false

  s.source_files = YaneuraOuEngineSources.source_files(:v980)

  s.public_header_files = 'bridge/engine_bridge.h'

  s.pod_target_xcconfig = YaneuraOuEngineSources.xcconfig(
    'NNUE_ARCHITECTURE_HEADER=\\"SFNN_ka2_4096_8_64_progress8.h\\" ' \
    'SFNNwoPSQT=1 ' \
    'YANEURAOU_BRIDGE_SYMBOL=tsuyoura_start',
    :v980
  )
end
