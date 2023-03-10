#ifndef RecoPixelVertexing_PixelTriplets_plugins_alpaka_CAHitNtupletGeneratorKernels_h
#define RecoPixelVertexing_PixelTriplets_plugins_alpaka_CAHitNtupletGeneratorKernels_h

// #define GPU_DEBUG
#include <alpaka/alpaka.hpp>
#include <cstdint>
#include "GPUCACell.h"
#include "gpuPixelDoublets.h"
#include "../CAStructures.h"

#include "DataFormats/Track/interface/alpaka/PixelTrackUtilities.h"
#include "DataFormats/Track/interface/PixelTrackDefinitions.h"
#include "DataFormats/Track/interface/TrackSoAHost.h"
#include "DataFormats/TrackingRecHitSoA/interface/TrackingRecHitsLayout.h"
#include "DataFormats/TrackingRecHitSoA/interface/alpaka/TrackingRecHitSoADevice.h"
#include "HeterogeneousCore/AlpakaUtilities/interface/AtomicPairCounter.h"
#include "HeterogeneousCore/AlpakaUtilities/interface/HistoContainer.h"
#include "HeterogeneousCore/AlpakaInterface/interface/memory.h"

// #define DUMP_GPU_TK_TUPLES

namespace ALPAKA_ACCELERATOR_NAMESPACE {
  namespace caHitNtupletGenerator {

    //Configuration params common to all topologies, for the algorithms
    struct AlgoParams {
      const bool onGPU_;
      const uint32_t minHitsForSharingCut_;
      const bool useRiemannFit_;
      const bool fitNas4_;
      const bool includeJumpingForwardDoublets_;
      const bool earlyFishbone_;
      const bool lateFishbone_;
      const bool doStats_;
      const bool doSharedHitCut_;
      const bool dupPassThrough_;
      const bool useSimpleTripletCleaner_;
    };

    //CAParams
    struct CACommon {
      const uint32_t minHitsPerNtuplet_;
      const float ptmin_;
      const float CAThetaCutBarrel_;
      const float CAThetaCutForward_;
      const float hardCurvCut_;
      const float dcaCutInnerTriplet_;
      const float dcaCutOuterTriplet_;
    };

    template <typename TrackerTraits, typename Enable = void>
    struct CAParamsT : public CACommon {
      ALPAKA_FN_ACC ALPAKA_FN_INLINE bool startingLayerPair(int16_t pid) const { return false; };
      ALPAKA_FN_ACC ALPAKA_FN_INLINE bool startAt0(int16_t pid) const { return false; };
    };

    template <typename TrackerTraits>
    struct CAParamsT<TrackerTraits, pixelTopology::isPhase1Topology<TrackerTraits>> : public CACommon {
      /// Is is a starting layer pair?
      ALPAKA_FN_ACC ALPAKA_FN_INLINE bool startingLayerPair(int16_t pid) const {
        return minHitsPerNtuplet_ > 3 ? pid < 3 : pid < 8 || pid > 12;
      }

      /// Is this a pair with inner == 0?
      ALPAKA_FN_ACC ALPAKA_FN_INLINE bool startAt0(int16_t pid) const {
        assert((pixelTopology::Phase1::layerPairs[pid * 2] == 0) ==
               (pid < 3 || pid == 13 || pid == 15 || pid == 16));  // to be 100% sure it's working, may be removed
        return pixelTopology::Phase1::layerPairs[pid * 2] == 0;
      }
    };

    template <typename TrackerTraits>
    struct CAParamsT<TrackerTraits, pixelTopology::isPhase2Topology<TrackerTraits>> : public CACommon {
      const bool includeFarForwards_;
      /// Is is a starting layer pair?
      ALPAKA_FN_ACC ALPAKA_FN_INLINE bool startingLayerPair(int16_t pid) const {
        return pid < 33;  // in principle one could remove 5,6,7 23, 28 and 29
      }

      /// Is this a pair with inner == 0
      ALPAKA_FN_ACC ALPAKA_FN_INLINE bool startAt0(int16_t pid) const {
        assert((pixelTopology::Phase2::layerPairs[pid * 2] == 0) == ((pid < 3) | (pid >= 23 && pid < 28)));
        return pixelTopology::Phase2::layerPairs[pid * 2] == 0;
      }
    };

    //Full list of params = algo params + ca params + cell params + quality cuts
    //Generic template
    template <typename TrackerTraits, typename Enable = void>
    struct ParamsT : public AlgoParams {
      // one should define the params for its own pixelTopology
      // not defining anything here
      inline uint32_t nPairs() const { return 0; }
    };

    template <typename TrackerTraits>
    struct ParamsT<TrackerTraits, pixelTopology::isPhase1Topology<TrackerTraits>> : public AlgoParams {
      using TT = TrackerTraits;
      using QualityCuts = pixelTrack::QualityCutsT<TT>;  //track quality cuts
      using CellCuts = gpuPixelDoublets::CellCutsT<TT>;  //cell building cuts
      using CAParams = CAParamsT<TT>;                    //params to be used on device

      ParamsT(AlgoParams const& commonCuts,
              CellCuts const& cellCuts,
              QualityCuts const& cutsCuts,
              CAParams const& caParams)
          : AlgoParams(commonCuts), cellCuts_(cellCuts), qualityCuts_(cutsCuts), caParams_(caParams) {}

      const CellCuts cellCuts_;
      const QualityCuts qualityCuts_{// polynomial coefficients for the pT-dependent chi2 cut
                                     {0.68177776, 0.74609577, -0.08035491, 0.00315399},
                                     // max pT used to determine the chi2 cut
                                     10.,
                                     // chi2 scale factor: 30 for broken line fit, 45 for Riemann fit
                                     30.,
                                     // regional cuts for triplets
                                     {
                                         0.3,  // |Tip| < 0.3 cm
                                         0.5,  // pT > 0.5 GeV
                                         12.0  // |Zip| < 12.0 cm
                                     },
                                     // regional cuts for quadruplets
                                     {
                                         0.5,  // |Tip| < 0.5 cm
                                         0.3,  // pT > 0.3 GeV
                                         12.0  // |Zip| < 12.0 cm
                                     }};
      const CAParams caParams_;
      /// Compute the number of pairs
      inline uint32_t nPairs() const {
        // take all layer pairs into account
        uint32_t nActualPairs = TT::nPairs;
        if (not includeJumpingForwardDoublets_) {
          // exclude forward "jumping" layer pairs
          nActualPairs = TT::nPairsForTriplets;
        }
        if (caParams_.minHitsPerNtuplet_ > 3) {
          // for quadruplets, exclude all "jumping" layer pairs
          nActualPairs = TT::nPairsForQuadruplets;
        }

        return nActualPairs;
      }

    };  // Params Phase1

    template <typename TrackerTraits>
    struct ParamsT<TrackerTraits, pixelTopology::isPhase2Topology<TrackerTraits>> : public AlgoParams {
      using TT = TrackerTraits;
      using QualityCuts = pixelTrack::QualityCutsT<TT>;
      using CellCuts = gpuPixelDoublets::CellCutsT<TT>;
      using CAParams = CAParamsT<TT>;

      ParamsT(AlgoParams const& commonCuts,
              CellCuts const& cellCuts,
              QualityCuts const& qualityCuts,
              CAParams const& caParams)
          : AlgoParams(commonCuts), cellCuts_(cellCuts), qualityCuts_(qualityCuts), caParams_(caParams) {}

      // quality cuts
      const CellCuts cellCuts_;
      const QualityCuts qualityCuts_{5.0f, /*chi2*/ 0.9f, /* pT in Gev*/ 0.4f, /*zip in cm*/ 12.0f /*tip in cm*/};
      const CAParams caParams_;

      inline uint32_t nPairs() const {
        // take all layer pairs into account
        uint32_t nActualPairs = TT::nPairsMinimal;
        if (caParams_.includeFarForwards_) {
          // considera far forwards (> 11 & > 23)
          nActualPairs = TT::nPairsFarForwards;
        }
        if (includeJumpingForwardDoublets_) {
          // include jumping forwards
          nActualPairs = TT::nPairs;
        }

        return nActualPairs;
      }

    };  // Params Phase1

    // counters
    struct Counters {
      unsigned long long nEvents;
      unsigned long long nHits;
      unsigned long long nCells;
      unsigned long long nTuples;
      unsigned long long nFitTracks;
      unsigned long long nLooseTracks;
      unsigned long long nGoodTracks;
      unsigned long long nUsedHits;
      unsigned long long nDupHits;
      unsigned long long nFishCells;
      unsigned long long nKilledCells;
      unsigned long long nEmptyCells;
      unsigned long long nZeroTrackCells;
    };

    using Quality = ::pixelTrack::Quality;

  }  // namespace caHitNtupletGenerator

  template <typename TTTraits>
  class CAHitNtupletGeneratorKernels {
  public:
    using TrackerTraits = TTTraits;
    using QualityCuts = pixelTrack::QualityCutsT<TrackerTraits>;
    using Params = caHitNtupletGenerator::ParamsT<TrackerTraits>;
    using CAParams = caHitNtupletGenerator::CAParamsT<TrackerTraits>;
    using Counters = caHitNtupletGenerator::Counters;

    template <typename Device, typename T>
    using unique_ptr = typename cms::alpakatools::device_buffer<Device, T>;

    using HitsView = TrackingRecHitAlpakaSoAView<TrackerTraits>;
    using HitsConstView = TrackingRecHitAlpakaSoAConstView<TrackerTraits>;
    using TkSoAView = TrackSoAView<TrackerTraits>;

    using HitToTuple = caStructures::HitToTupleT<TrackerTraits>;
    using TupleMultiplicity = caStructures::TupleMultiplicityT<TrackerTraits>;
    using CellNeighborsVector = caStructures::CellNeighborsVectorT<TrackerTraits>;
    using CellNeighbors = caStructures::CellNeighborsT<TrackerTraits>;
    using CellTracksVector = caStructures::CellTracksVectorT<TrackerTraits>;
    using CellTracks = caStructures::CellTracksT<TrackerTraits>;
    using OuterHitOfCellContainer = caStructures::OuterHitOfCellContainerT<TrackerTraits>;
    using OuterHitOfCell = caStructures::OuterHitOfCellT<TrackerTraits>;

    using CACell = GPUCACellT<TrackerTraits>;

    using Quality = ::pixelTrack::Quality;
    using HitContainer = typename TrackSoA<TrackerTraits>::HitContainer;

    // CAHitNtupletGeneratorKernels() = default;

    CAHitNtupletGeneratorKernels(Params const& params, uint32_t nhits, Queue& queue)
        : m_params(params),
          //////////////////////////////////////////////////////////
          // ALLOCATIONS FOR THE INTERMEDIATE RESULTS (STAYS ON WORKER)
          //////////////////////////////////////////////////////////
          counters_{cms::alpakatools::make_device_buffer<Counters>(queue)},

          // workspace
          device_hitToTuple_{cms::alpakatools::make_device_buffer<HitToTuple>(queue)},
          device_tupleMultiplicity_{cms::alpakatools::make_device_buffer<TupleMultiplicity>(queue)},

          // NB: In legacy, device_theCells_ and device_isOuterHitOfCell_ were allocated inside buildDoublets
          device_theCells_{
              cms::alpakatools::make_device_buffer<CACell[]>(queue, m_params.cellCuts_.maxNumberOfDoublets_)},
          // in principle we can use "nhits" to heuristically dimension the workspace...
          device_isOuterHitOfCell_{
              cms::alpakatools::make_device_buffer<OuterHitOfCellContainer[]>(queue, std::max(1u, nhits))},

          device_theCellNeighbors_{cms::alpakatools::make_device_buffer<CellNeighborsVector>(queue)},
          device_theCellTracks_{cms::alpakatools::make_device_buffer<CellTracksVector>(queue)},
          // NB: In legacy, cellStorage_ was allocated inside buildDoublets
          cellStorage_{cms::alpakatools::make_device_buffer<unsigned char[]>(
              queue,
              TrackerTraits::maxNumOfActiveDoublets * sizeof(CellNeighbors) +
                  TrackerTraits::maxNumOfActiveDoublets * sizeof(CellTracks))},
          device_theCellNeighborsContainer_{reinterpret_cast<CellNeighbors*>(cellStorage_.data())},
          device_theCellTracksContainer_{reinterpret_cast<CellTracks*>(
              cellStorage_.data() + TrackerTraits::maxNumOfActiveDoublets * sizeof(CellNeighbors))},

          // NB: In legacy, device_storage_ was allocated inside allocateOnGPU
          device_storage_{
              cms::alpakatools::make_device_buffer<cms::alpakatools::AtomicPairCounter::c_type[]>(queue, 3u)},
          device_hitTuple_apc_{reinterpret_cast<cms::alpakatools::AtomicPairCounter*>(device_storage_.data())},
          device_hitToTuple_apc_{reinterpret_cast<cms::alpakatools::AtomicPairCounter*>(device_storage_.data() + 1)},
          device_nCells_{cms::alpakatools::make_device_view(alpaka::getDev(queue),
                                                            *reinterpret_cast<uint32_t*>(device_storage_.data() + 2))} {
      alpaka::memset(queue, counters_, 0);
      alpaka::memset(queue, device_nCells_, 0);
      cms::alpakatools::launchZero<Acc1D>(device_tupleMultiplicity_.data(), queue);
      cms::alpakatools::launchZero<Acc1D>(device_hitToTuple_.data(), queue);
    }

    ~CAHitNtupletGeneratorKernels() = default;

    TupleMultiplicity const* tupleMultiplicity() const { return device_tupleMultiplicity_.data(); }

    void launchKernels(const HitsConstView& hh, TkSoAView& track_view, Queue& queue);

    void classifyTuples(const HitsConstView& hh, TkSoAView& track_view, Queue& queue);

    void buildDoublets(const HitsConstView& hh, int32_t offsetBPIX2, Queue& queue);
    void cleanup(Queue& queue);

    static void printCounters(Counters const* counters);

  protected:
    // params
    Params const& m_params;

    cms::alpakatools::device_buffer<Device, Counters> counters_;

    // workspace
    cms::alpakatools::device_buffer<Device, HitToTuple> device_hitToTuple_;
    cms::alpakatools::device_buffer<Device, TupleMultiplicity> device_tupleMultiplicity_;
    cms::alpakatools::device_buffer<Device, CACell[]> device_theCells_;
    cms::alpakatools::device_buffer<Device, OuterHitOfCellContainer[]> device_isOuterHitOfCell_;
    cms::alpakatools::device_buffer<Device, CellNeighborsVector> device_theCellNeighbors_;
    cms::alpakatools::device_buffer<Device, CellTracksVector> device_theCellTracks_;
    cms::alpakatools::device_buffer<Device, unsigned char[]> cellStorage_;
    CellNeighbors* device_theCellNeighborsContainer_;
    CellTracks* device_theCellTracksContainer_;
    cms::alpakatools::device_buffer<Device, cms::alpakatools::AtomicPairCounter::c_type[]> device_storage_;
    cms::alpakatools::AtomicPairCounter* device_hitTuple_apc_;
    cms::alpakatools::AtomicPairCounter* device_hitToTuple_apc_;
    cms::alpakatools::device_view<Device, uint32_t> device_nCells_;
    // uint32_t* device_nCells_;
    std::optional<cms::alpakatools::device_buffer<Device, OuterHitOfCell>> isOuterHitOfCell_;
  };
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

#endif  // RecoPixelVertexing_PixelTriplets_plugins_CAHitNtupletGeneratorKernels_h
