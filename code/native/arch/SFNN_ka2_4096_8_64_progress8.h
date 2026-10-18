// つようら王11mini(SFNN_ka2_4096_8_64_progress8)用の NNUE アーキテクチャ定義。
//
// 出典: やねうら王(https://github.com/yaneurao/YaneuraOu, GPLv3)の
//   source/eval/nnue/architectures/nnue_arch_gen.py(V1.03)で生成した。
//     python3 nnue_arch_gen.py SFNN_ka2_4096_8_64_progress8 <出力先>
//   本家 Makefile はビルドのたびにこのスクリプトでヘッダを生成するが、
//   アプリのビルド(Xcode / Gradle)に Python を要求しないよう、生成済みの
//   ものをここに置く(submodule の pin 先 c1b80eaa で生成)。
//
// 生成物からの変更点: include を source/ 起点の相対パスに書き換えた
// (halfkp_768x2-16-64.h と同じ。native/arch/ から include されるため)。
//
// SFNN 系はビルド時に SFNNwoPSQT の定義が要る(本家 Makefile と同じ)。
// position.h の StateInfo の中身(進行度のキャッシュ)が変わるので、
// このヘッダを使うバイナリの全ソースに付けること。評価関数ファイルは
// nn.bin に加えて progress.bin(進行度の係数)を EvalDir から読む。

// SFNN without PSQT architecture

#ifndef CLASSIC_NNUE_SFNN_SFNN_KA2_4096_8_64_PROGRESS8_H_INCLUDED
#define CLASSIC_NNUE_SFNN_SFNN_KA2_4096_8_64_PROGRESS8_H_INCLUDED

#include "eval/nnue/features/feature_set.h"

#include "eval/nnue/features/k.h"
#include "eval/nnue/features/a2.h"

#include "eval/nnue/architectures/sfnn_network.h"

namespace YaneuraOu {
namespace Eval::NNUE {

// Input features used in evaluation function
// 評価関数で用いる入力特徴量

    using RawFeatures = Features::FeatureSet<Features::K, Features::A2>;

    // Number of input feature dimensions after conversion
    // 変換後の入力特徴量の次元数
    constexpr IndexType kTransformedFeatureDimensions = 4096;

    // 小幅SFNN専用。従来幅のSFNNでは定義せず、既存の高速経路をそのまま使う。
    

    // Number of networks stored in the evaluation file
    constexpr int LayerStacks = 8;

    #define NNUE_SFNN_HAND_BUCKETS 1
    #define NNUE_SFNN_HAND_BUCKET_TYPE 0
    #define NNUE_SFNN_KING_BUCKETS 1
    #define NNUE_SFNN_KING_BUCKET_TYPE 0
    #define NNUE_SFNN_PROGRESS_BUCKETS 8

    // Number of groups for the first affine layer of SFNN.
    // common+shard fc_0でのみ2以上になる。
    constexpr IndexType kHidden1GroupCount = 1;

    // common+shard fc_0 settings. kHidden1ShardDimensions is per shard.
    constexpr bool kHidden1UsesCommonShard = false;
    constexpr IndexType kHidden1CommonDimensions = 0;
    constexpr IndexType kHidden1ShardDimensions = 0;

    // 各層の次元数
    constexpr IndexType kInputDims   = kTransformedFeatureDimensions;
    constexpr IndexType kHidden1Dims = 8;
    constexpr bool kUseShortcut = false;
    constexpr IndexType kHidden1OutputDims = kHidden1Dims + (kUseShortcut ? 1 : 0);
    constexpr IndexType kHidden2Dims = 64;                              

    #if !defined(ENABLE_SFNN_16BIT_WEIGHT)
    
    #endif

    using Fc0Layer = Layers::AffineTransformSparseInputExplicit<kInputDims, kHidden1OutputDims>;
    #if defined(ENABLE_SFNN_16BIT_WEIGHT)
    using NetworkBase = SfnnNetwork16<kInputDims, kHidden1Dims, kHidden2Dims, kUseShortcut>;
    #else
    using NetworkBase = SfnnNetwork<Fc0Layer, kInputDims, kHidden1Dims, kHidden2Dims, kUseShortcut>;
    #endif

    struct Network : NetworkBase {
        static std::string GetStructureString() {
            return "SFNN_KA2_4096_8_64_PROGRESS8";
        }
    };

}  // namespace Eval::NNUE
}  // namespace YaneuraOu

#endif // CLASSIC_NNUE_SFNN_KA2_4096_8_64_PROGRESS8_H_INCLUDED
