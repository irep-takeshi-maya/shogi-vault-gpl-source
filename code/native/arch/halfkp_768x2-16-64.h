// AobaNNUE(halfkp_768x2-16-64)用の NNUE アーキテクチャ定義。
//
// 出典: AobaNNUE v1.1 同梱ソース
//   source/eval/nnue/architectures/halfkp_768x2-16-64.h
//   https://github.com/yssaya/AobaNNUE (GPLv3, 著作者: 山下 宏 氏)
// AobaNNUE はやねうら王 V9.00 GitHub版をベースにした派生で、本ファイルも
// その一部。ライセンスは GPLv3(THIRD_PARTY_LICENSES.md 参照)。
//
// 【なぜ submodule ではなくここに置くか】
// やねうら王 V9.00 の source/eval/nnue/nnue_architecture.h には
// `NNUE_ARCHITECTURE_HEADER` が定義されていればそれを include する
// フックが最初から用意されている。そのためアーキテクチャの追加に
// submodule の改変は必要なく、ビルド定義で
//   -DNNUE_ARCHITECTURE_HEADER="\"halfkp_768x2-16-64.h\""
// を渡し、native/arch/ を include パスへ通すだけで差し替わる。
// submodule を汚さないことで、本体のバージョンアップ(Issue #281)時の
// 追随コストを増やさない。

// Definition of input features and network structure used in NNUE evaluation function
// NNUE評価関数で用いる入力特徴量とネットワーク構造の定義
#ifndef CLASSIC_NNUE_HALFKP_768X2_16_64_H_INCLUDED
#define CLASSIC_NNUE_HALFKP_768X2_16_64_H_INCLUDED

#include "eval/nnue/features/feature_set.h"
#include "eval/nnue/features/half_kp.h"

#include "eval/nnue/layers/input_slice.h"
#include "eval/nnue/layers/affine_transform.h"
#include "eval/nnue/layers/affine_transform_sparse_input.h"
#include "eval/nnue/layers/clipped_relu.h"

namespace YaneuraOu {
namespace Eval::NNUE {

// Input features used in evaluation function
// 評価関数で用いる入力特徴量
using RawFeatures = Features::FeatureSet<
    Features::HalfKP<Features::Side::kFriend>>;

// Number of input feature dimensions after conversion
// 変換後の入力特徴量の次元数
constexpr IndexType kTransformedFeatureDimensions = 768;

namespace Layers {

// Define network structure
// ネットワーク構造の定義
using InputLayer = InputSlice<kTransformedFeatureDimensions * 2>;
using HiddenLayer1 = ClippedReLU<AffineTransformSparseInput<InputLayer, 16>>;
using HiddenLayer2 = ClippedReLU<AffineTransform<HiddenLayer1, 64>>;
using OutputLayer = AffineTransform<HiddenLayer2, 1>;

}  // namespace Layers

using Network = Layers::OutputLayer;

} // namespace Eval::NNUE
} // namespace YaneuraOu

#endif // #ifndef NNUE_HALFKP_768X2_16_64_H_INCLUDED
