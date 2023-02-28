#ifndef RecoPixelVertexing_PixelTriplets_plugins_alpaka_HelixFitOnGPU_h
#define RecoPixelVertexing_PixelTriplets_plugins_alpaka_HelixFitOnGPU_h

#include <alpaka/alpaka.hpp>
#include "DataFormats/Track/interface/alpaka/PixelTrackUtilities.h"
#include "DataFormats/TrackingRecHitSoA/interface/TrackingRecHitsLayout.h"
#include "RecoPixelVertexing/PixelTrackFitting/interface/FitResult.h"
#include "DataFormats/TrackerCommon/interface/SimplePixelTopology.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"

#include "../CAStructures.h"
namespace riemannFit {
  // TODO: Can this be taken from TrackerTraits or somewhere else?
  // in case of memory issue can be made smaller
  constexpr uint32_t maxNumberOfConcurrentFits = 32 * 1024;
  constexpr uint32_t stride = maxNumberOfConcurrentFits;
  using Matrix3x4d = Eigen::Matrix<double, 3, 4>;
  using Map3x4d = Eigen::Map<Matrix3x4d, 0, Eigen::Stride<3 * stride, stride> >;
  using Matrix6x4f = Eigen::Matrix<float, 6, 4>;
  using Map6x4f = Eigen::Map<Matrix6x4f, 0, Eigen::Stride<6 * stride, stride> >;

  // hits
  template <int N>
  using Matrix3xNd = Eigen::Matrix<double, 3, N>;
  template <int N>
  using Map3xNd = Eigen::Map<Matrix3xNd<N>, 0, Eigen::Stride<3 * stride, stride> >;
  // errors
  template <int N>
  using Matrix6xNf = Eigen::Matrix<float, 6, N>;
  template <int N>
  using Map6xNf = Eigen::Map<Matrix6xNf<N>, 0, Eigen::Stride<6 * stride, stride> >;
  // fast fit
  using Map4d = Eigen::Map<Vector4d, 0, Eigen::InnerStride<stride> >;

  template <auto Start, auto End, auto Inc, class F>  //a compile-time bounded for loop
  constexpr void rolling_fits(F &&f) {
    if constexpr (Start < End) {
      f(std::integral_constant<decltype(Start), Start>());
      rolling_fits<Start + Inc, End, Inc>(f);
    }
  }

}  // namespace riemannFit

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  template <typename TrackerTraits>
  class HelixFitOnGPU {
  public:
    using TrackingRecHitSoAs = TrackingRecHitAlpakaSoA<TrackerTraits>;

    using HitView = TrackingRecHitAlpakaSoAView<TrackerTraits>;
    using HitConstView = TrackingRecHitAlpakaSoAConstView<TrackerTraits>;

    using Tuples = typename TrackSoA<TrackerTraits>::HitContainer;
    using OutputSoAView = TrackSoAView<TrackerTraits>;

    using TupleMultiplicity = caStructures::TupleMultiplicityT<TrackerTraits>;

    explicit HelixFitOnGPU(float bf, bool fitNas4) : bField_(bf), fitNas4_(fitNas4) {}
    ~HelixFitOnGPU() { deallocateOnGPU(); }

    void setBField(double bField) { bField_ = bField; }
    void launchRiemannKernels(const HitConstView &hv, uint32_t nhits, uint32_t maxNumberOfTuples, Queue &queue);
    void launchBrokenLineKernels(const HitConstView &hv, uint32_t nhits, uint32_t maxNumberOfTuples, Queue &queue);

    // void launchRiemannKernels(const HitConstView &hv, uint32_t nhits, uint32_t maxNumberOfTuples);
    // void launchBrokenLineKernels(const HitConstView &hv, uint32_t nhits, uint32_t maxNumberOfTuples);

    void allocateOnGPU(TupleMultiplicity const *tupleMultiplicity, OutputSoAView &helix_fit_results);
    void deallocateOnGPU();

  private:
    static constexpr uint32_t maxNumberOfConcurrentFits_ = riemannFit::maxNumberOfConcurrentFits;

    // fowarded
    Tuples const *tuples_ = nullptr;
    TupleMultiplicity const *tupleMultiplicity_ = nullptr;
    OutputSoAView outputSoa_;
    float bField_;

    const bool fitNas4_;
  };
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE
#endif  // RecoPixelVertexing_PixelTriplets_plugins_HelixFitOnGPU_h
