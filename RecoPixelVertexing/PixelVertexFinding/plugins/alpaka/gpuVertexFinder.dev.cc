#include <alpaka/alpaka.hpp>
#include "DataFormats/Track/interface/alpaka/PixelTrackUtilities.h"
#include "DataFormats/Vertex/interface/alpaka/ZVertexUtilities.h"
#include "HeterogeneousCore/AlpakaInterface/interface/workdivision.h"
#include "HeterogeneousCore/AlpakaInterface/interface/traits.h"
#include "PixelVertexWorkSpaceUtilities.h"
#include "PixelVertexWorkSpaceSoADevice.h"

#include "gpuVertexFinder.h"
#include "gpuClusterTracksByDensity.h"
#include "gpuClusterTracksDBSCAN.h"
#include "gpuClusterTracksIterative.h"
#include "gpuFitVertices.h"
#include "gpuSortByPt2.h"
#include "gpuSplitVertices.h"

#undef PIXVERTEX_DEBUG_PRODUCE
namespace ALPAKA_ACCELERATOR_NAMESPACE {
  namespace gpuVertexFinder {
    using namespace cms::alpakatools;
    // reject outlier tracks that contribute more than this to the chi2 of the vertex fit
    constexpr float maxChi2ForFirstFit = 50.f;
    constexpr float maxChi2ForFinalFit = 5000.f;

    // split vertices with a chi2/NDoF greater than this
    constexpr float maxChi2ForSplit = 9.f;

    template <typename TrackerTraits>
    class loadTracks {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(const TAcc& acc,
                                    TrackSoAConstView<TrackerTraits> tracks_view,
                                    VtxSoAView soa,
                                    WsSoAView pws,
                                    float ptMin,
                                    float ptMax) const {
        auto const* quality = tracks_view.quality();
        using helper = TracksUtilities<TrackerTraits>;

        for (auto idx : cms::alpakatools::elements_with_stride(acc, tracks_view.nTracks())) {
          // auto nHits = helper::nHits(tracks_view, idx);
          // ALPAKA_ASSERT_OFFLOAD(nHits >= 3);

          // initialize soa...
          soa[idx].idv() = -1;

          if (helper::isTriplet(tracks_view, idx))
            continue;  // no triplets
          if (quality[idx] < ::pixelTrack::Quality::highPurity)
            continue;

          auto pt = tracks_view[idx].pt();

          if (pt < ptMin)
            continue;

          // clamp pt
          pt = std::min<float>(pt, ptMax);

          auto& data = pws;
          auto it = alpaka::atomicAdd(acc, &data.ntrks(), 1u, alpaka::hierarchy::Blocks{});
          data[it].itrk() = idx;
          data[it].zt() = helper::zip(tracks_view, idx);
          data[it].ezt2() = tracks_view[idx].covariance()(14);
          data[it].ptt2() = pt * pt;
        }
      }
    };
// #define THREE_KERNELS
#ifndef THREE_KERNELS
    class vertexFinderOneKernel {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(const TAcc& acc,
                                    VtxSoAView pdata,
                                    WsSoAView pws,
                                    int minT,      // min number of neighbours to be "seed"
                                    float eps,     // max absolute distance to cluster
                                    float errmax,  // max error to be "seed"
                                    float chi2max  // max normalized distance to cluster,
      ) const {
        clusterTracksByDensity(acc, pdata, pws, minT, eps, errmax, chi2max);
        alpaka::syncBlockThreads(acc);
        fitVertices(acc, pdata, pws, maxChi2ForFirstFit);
        alpaka::syncBlockThreads(acc);
        splitVertices(acc, pdata, pws, maxChi2ForSplit);
        alpaka::syncBlockThreads(acc);
        fitVertices(acc, pdata, pws, maxChi2ForFinalFit);
        alpaka::syncBlockThreads(acc);
        sortByPt2(acc, pdata, pws);
      }
    };
#else
    class vertexFinderOneKernel1 {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(const TAcc& acc,
                                    VtxSoAView pdata,
                                    WsSoAView pws,
                                    int minT,      // min number of neighbours to be "seed"
                                    float eps,     // max absolute distance to cluster
                                    float errmax,  // max error to be "seed"
                                    float chi2max  // max normalized distance to cluster,
      ) const {
        clusterTracksByDensity(pdata, pws, minT, eps, errmax, chi2max);
        alpaka::syncBlockThreads(acc);
        fitVertices(pdata, pws, maxChi2ForFirstFit);
      }
    };
    class vertexFinderKernel2 {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(const TAcc& acc, VtxSoAView pdata, WsSoAView pws) const {
        fitVertices(pdata, pws, maxChi2ForFinalFit);
        alpaka::syncBlockThreads(acc);
        sortByPt2(pdata, pws);
      }
    };
#endif

    template <typename TrackerTraits>
    ZVertexDevice Producer<TrackerTraits>::makeAsync(Queue& queue,
                                                     const TrackSoAConstView<TrackerTraits>& tracks_view,
                                                     float ptMin,
                                                     float ptMax) const {
#ifdef PIXVERTEX_DEBUG_PRODUCE
      std::cout << "producing Vertices on GPU" << std::endl;
#endif  // PIXVERTEX_DEBUG_PRODUCE
      ZVertexDevice vertices(queue);

      auto soa = vertices.view();

      ALPAKA_ASSERT_OFFLOAD(vertices.buffer());

      auto ws_d = workSpace::PixelVertexWorkSpaceSoADevice(queue);

      // Initialize
      const auto initWorkDiv = cms::alpakatools::make_workdiv<Acc1D>(1, 1);
      alpaka::exec<Acc1D>(queue, initWorkDiv, init{}, soa, ws_d.view());

      // Load Tracks
      const uint32_t blockSize = 128;
      const uint32_t numberOfBlocks =
          cms::alpakatools::divide_up_by(tracks_view.metadata().size() + blockSize - 1, blockSize);
      const auto loadTracksWorkDiv = cms::alpakatools::make_workdiv<Acc1D>(numberOfBlocks, blockSize);
      alpaka::exec<Acc1D>(
          queue, loadTracksWorkDiv, loadTracks<TrackerTraits>{}, tracks_view, soa, ws_d.view(), ptMin, ptMax);

      // Running too many thread lead to problems when printf is enabled.
      const auto finderSorterWorkDiv = cms::alpakatools::make_workdiv<Acc1D>(1, 1024 - 256);
      const auto splitterFitterWorkDiv = cms::alpakatools::make_workdiv<Acc1D>(1024, 128);

      if (oneKernel_) {
        // implemented only for density clustesrs
#ifndef THREE_KERNELS
        alpaka::exec<Acc1D>(
            queue, finderSorterWorkDiv, vertexFinderOneKernel{}, soa, ws_d.view(), minT, eps, errmax, chi2max);
#else
        alpaka::exec<Acc1D>(
            queue, finderSorterWorkDiv, vertexFinderOneKernel{}, soa, ws_d.view(), minT, eps, errmax, chi2max);

        // one block per vertex...
        alpaka::exec<Acc1D>(queue, splitterFitterWorkDiv, splitVerticesKernel{}, soa, ws_d.view(), maxChi2ForSplit);
        alpaka::exec<Acc1D>(queue, finderSorterWorkDiv{}, soa, ws_d.view());
#endif
      } else {  // five kernels
        if (useDensity_) {
          alpaka::exec<Acc1D>(
              queue, finderSorterWorkDiv, clusterTracksByDensityKernel{}, soa, ws_d.view(), minT, eps, errmax, chi2max);

        } else if (useDBSCAN_) {
          alpaka::exec<Acc1D>(
              queue, finderSorterWorkDiv, clusterTracksDBSCAN{}, soa, ws_d.view(), minT, eps, errmax, chi2max);
        } else if (useIterative_) {
          alpaka::exec<Acc1D>(
              queue, finderSorterWorkDiv, clusterTracksIterative{}, soa, ws_d.view(), minT, eps, errmax, chi2max);
        }
        alpaka::exec<Acc1D>(queue, finderSorterWorkDiv, fitVerticesKernel{}, soa, ws_d.view(), maxChi2ForFirstFit);

        // one block per vertex...
        alpaka::exec<Acc1D>(queue, splitterFitterWorkDiv, splitVerticesKernel{}, soa, ws_d.view(), maxChi2ForSplit);

        alpaka::exec<Acc1D>(queue, finderSorterWorkDiv, fitVerticesKernel{}, soa, ws_d.view(), maxChi2ForFinalFit);

        alpaka::exec<Acc1D>(queue, finderSorterWorkDiv, sortByPt2Kernel{}, soa, ws_d.view());
      }

      return vertices;
    }

    template class Producer<pixelTopology::Phase1>;
    template class Producer<pixelTopology::Phase2>;
  }  // namespace gpuVertexFinder
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE
