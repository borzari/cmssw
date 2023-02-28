//
// Original Author: Felice Pantaleo, CERN
//

// #define NTUPLE_DEBUG
// #define GPU_DEBUG

#include <alpaka/alpaka.hpp>
#include <cmath>
#include <cstdint>
#include <limits>

#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "HeterogeneousCore/AlpakaInterface/interface/traits.h"
#include "HeterogeneousCore/AlpakaInterface/interface/workdivision.h"
#include "RecoLocalTracker/SiPixelRecHits/interface/pixelCPEforDevice.h"
#include "DataFormats/Track/interface/alpaka/PixelTrackUtilities.h"
#include "DataFormats/TrackingRecHitSoA/interface/TrackingRecHitsLayout.h"

#include "../CAStructures.h"
#include "CAHitNtupletGeneratorKernels.h"
#include "GPUCACell.h"
#include "gpuFishbone.h"
#include "gpuPixelDoublets.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {
  namespace caHitNtupletGeneratorKernels {

    constexpr uint32_t tkNotFound = std::numeric_limits<uint16_t>::max();
    constexpr float maxScore = std::numeric_limits<float>::max();
    constexpr float nSigma2 = 25.f;

    //all of these below are mostly to avoid brining around the relative namespace

    template <typename TrackerTraits>
    using HitToTuple = caStructures::HitToTupleT<TrackerTraits>;

    template <typename TrackerTraits>
    using TupleMultiplicity = caStructures::TupleMultiplicityT<TrackerTraits>;

    template <typename TrackerTraits>
    using CellNeighborsVector = caStructures::CellNeighborsVectorT<TrackerTraits>;

    template <typename TrackerTraits>
    using CellTracksVector = caStructures::CellTracksVectorT<TrackerTraits>;

    template <typename TrackerTraits>
    using OuterHitOfCell = caStructures::OuterHitOfCellT<TrackerTraits>;

    using Quality = ::pixelTrack::Quality;

    template <typename TrackerTraits>
    using TkSoAView = TrackSoAView<TrackerTraits>;

    template <typename TrackerTraits>
    using HitContainer = typename TrackSoA<TrackerTraits>::HitContainer;

    template <typename TrackerTraits>
    using HitsConstView = typename GPUCACellT<TrackerTraits>::HitsConstView;

    template <typename TrackerTraits>
    using QualityCuts = pixelTrack::QualityCutsT<TrackerTraits>;

    template <typename TrackerTraits>
    using CAParams = caHitNtupletGenerator::CAParamsT<TrackerTraits>;

    using Counters = caHitNtupletGenerator::Counters;

    template <typename TrackerTraits>
    class kernel_checkOverflows {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    TupleMultiplicity<TrackerTraits> const *tupleMultiplicity,
                                    HitToTuple<TrackerTraits> const *hitToTuple,
                                    cms::alpakatools::AtomicPairCounter *apc,
                                    GPUCACellT<TrackerTraits> const *__restrict__ cells,
                                    uint32_t const *__restrict__ nCells,
                                    CellNeighborsVector<TrackerTraits> const *cellNeighbors,
                                    CellTracksVector<TrackerTraits> const *cellTracks,
                                    OuterHitOfCell<TrackerTraits> const isOuterHitOfCell,
                                    int32_t nHits,
                                    uint32_t maxNumberOfDoublets,
                                    Counters *counters) const {
        const uint32_t threadIdx(alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[0u]);

        auto &c = *counters;
        // counters once per event
        if (0 == threadIdx) {
          alpaka::atomicAdd(acc, &c.nEvents, 1ull, alpaka::hierarchy::Blocks{});
          alpaka::atomicAdd(acc, &c.nHits, static_cast<unsigned long long>(nHits), alpaka::hierarchy::Blocks{});
          alpaka::atomicAdd(acc, &c.nCells, static_cast<unsigned long long>(*nCells), alpaka::hierarchy::Blocks{});
          alpaka::atomicAdd(
              acc, &c.nTuples, static_cast<unsigned long long>(apc->get().m), alpaka::hierarchy::Blocks{});
          alpaka::atomicAdd(acc,
                            &c.nFitTracks,
                            static_cast<unsigned long long>(tupleMultiplicity->size()),
                            alpaka::hierarchy::Blocks{});
        }

#ifdef NTUPLE_DEBUG
        if (0 == threadIdx) {
          printf("number of found cells %d \n found tuples %d with total hits %d out of %d\n",
                 *nCells,
                 apc->get().m,
                 apc->get().n,
                 nHits);
          if (apc->get().m < TrackerTraits::maxNumberOfQuadruplets) {
            ALPAKA_ASSERT_OFFLOAD(tracks_view.hitIndices().size(apc->get().m) == 0);
            ALPAKA_ASSERT_OFFLOAD(tracks_view.hitIndices().size() == apc->get().n);
          }
        }
        const auto ntNbins = foundNtuplets->nbins();

        // for (int idx = first, nt = tracks_view.hitIndices().nbins(); idx < nt; idx += gridDim.x * blockDim.x) {
        for (auto idx : cms::alpakatools::elements_with_stride(acc, ntBins)) {
          if (tracks_view.hitIndices().size(idx) > TrackerTraits::maxHitsOnTrack)  // current real limit
            printf("ERROR %d, %d\n", idx, tracks_view.hitIndices().size(idx));
          ALPAKA_ASSERT_OFFLOAD(ftracks_view.hitIndices().size(idx) <= TrackerTraits::maxHitsOnTrack);
          for (auto ih = tracks_view.hitIndices().begin(idx); ih != tracks_view.hitIndices().end(idx); ++ih)
            ALPAKA_ASSERT_OFFLOAD(int(*ih) < nHits);
        }
#endif

        if (0 == threadIdx) {
          if (apc->get().m >= TrackerTraits::maxNumberOfQuadruplets)
            printf("Tuples overflow\n");
          if (*nCells >= maxNumberOfDoublets)
            printf("Cells overflow\n");
          if (cellNeighbors && cellNeighbors->full())
            printf("cellNeighbors overflow %d %d \n", cellNeighbors->capacity(), cellNeighbors->size());
          if (cellTracks && cellTracks->full())
            printf("cellTracks overflow\n");
          if (int(hitToTuple->nbins()) < nHits)
            printf("ERROR hitToTuple  overflow %d %d\n", hitToTuple->nbins(), nHits);
#ifdef GPU_DEBUG
          printf("size of cellNeighbors %d \n cellTracks %d \n hitToTuple %d \n",
                 cellNeighbors->size(),
                 cellTracks->size(),
                 hitToTuple->size());
// printf("cellTracksSizes;");
// for (int i = 0; i < cellTracks->size(); i++) {
//   printf("%d;",cellTracks[i].size());
// }
//
// printf("\n");
// printf("cellNeighborsSizes;");
// for (int i = 0; i < cellNeighbors->size(); i++) {
//   printf("%d;",cellNeighbors[i].size());
// }
// printf("\n");
#endif
        }

        const auto ntNCells = (*nCells);
        // for (int idx = first, nt = (*nCells); idx < nt; idx += gridDim.x * blockDim.x) {
        for (auto idx : cms::alpakatools::elements_with_stride(acc, ntNCells)) {
          auto const &thisCell = cells[idx];
          if (thisCell.hasFishbone() && !thisCell.isKilled())
            alpaka::atomicAdd(acc, &c.nFishCells, 1ull, alpaka::hierarchy::Blocks{});
          if (thisCell.outerNeighbors().full())  //++tooManyNeighbors[thisCell.theLayerPairId];
            printf("OuterNeighbors overflow %d in %d\n", idx, thisCell.layerPairId());
          if (thisCell.tracks().full())  //++tooManyTracks[thisCell.theLayerPairId];
            printf("Tracks overflow %d in %d\n", idx, thisCell.layerPairId());
          if (thisCell.isKilled())
            alpaka::atomicAdd(acc, &c.nKilledCells, 1ull, alpaka::hierarchy::Blocks{});
          if (!thisCell.unused())
            alpaka::atomicAdd(acc, &c.nEmptyCells, 1ull, alpaka::hierarchy::Blocks{});
          if ((0 == hitToTuple->size(thisCell.inner_hit_id())) && (0 == hitToTuple->size(thisCell.outer_hit_id())))
            alpaka::atomicAdd(acc, &c.nZeroTrackCells, 1ull, alpaka::hierarchy::Blocks{});
        }

        for (auto idx : cms::alpakatools::elements_with_stride(acc, nHits))
          if (isOuterHitOfCell.container[idx].full())  // ++tooManyOuterHitOfCell;
            printf("OuterHitOfCell overflow %d\n", idx);
      }
    };

    template <typename TrackerTraits>
    class kernel_fishboneCleaner {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    GPUCACellT<TrackerTraits> const *cells,
                                    uint32_t const *__restrict__ nCells,
                                    TkSoAView<TrackerTraits> tracks_view) const {
        constexpr auto reject = Quality::dup;
        const auto ntNCells = (*nCells);

        for (auto idx : cms::alpakatools::elements_with_stride(acc, ntNCells)) {
          auto const &thisCell = cells[idx];
          if (!thisCell.isKilled())
            continue;

          for (auto it : thisCell.tracks())
            tracks_view[it].quality() = reject;
        }
      }
    };
    // remove shorter tracks if sharing a cell
    // It does not seem to affect efficiency in any way!
    template <typename TrackerTraits>
    class kernel_earlyDuplicateRemover {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    GPUCACellT<TrackerTraits> const *cells,
                                    uint32_t const *__restrict__ nCells,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    bool dupPassThrough) const {
        // quality to mark rejected
        constexpr auto reject = Quality::edup;  /// cannot be loose
        ALPAKA_ASSERT_OFFLOAD(nCells);
        const auto ntNCells = (*nCells);

        for (auto idx : cms::alpakatools::elements_with_stride(acc, ntNCells)) {
          auto const &thisCell = cells[idx];

          if (thisCell.tracks().size() < 2)
            continue;

          int8_t maxNl = 0;

          // find maxNl
          for (auto it : thisCell.tracks()) {
            auto nl = tracks_view[it].nLayers();
            maxNl = std::max(nl, maxNl);
          }

          // if (maxNl<4) continue;
          // quad pass through (leave it her for tests)
          //  maxNl = std::min(4, maxNl);

          for (auto it : thisCell.tracks()) {
            if (tracks_view[it].nLayers() < maxNl)
              tracks_view[it].quality() = reject;  //no race:  simple assignment of the same constant
          }
        }
      }
    };

    // assume the above (so, short tracks already removed)
    template <typename TrackerTraits>
    class kernel_fastDuplicateRemover {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    GPUCACellT<TrackerTraits> const *__restrict__ cells,
                                    uint32_t const *__restrict__ nCells,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    bool dupPassThrough) const {
        // quality to mark rejected
        auto const reject = dupPassThrough ? Quality::loose : Quality::dup;
        constexpr auto loose = Quality::loose;

        ALPAKA_ASSERT_OFFLOAD(nCells);
        const auto ntNCells = (*nCells);

        for (auto idx : cms::alpakatools::elements_with_stride(acc, ntNCells)) {
          auto const &thisCell = cells[idx];
          if (thisCell.tracks().size() < 2)
            continue;

          float mc = maxScore;
          uint16_t im = tkNotFound;

          auto score = [&](auto it) { return std::abs(TracksUtilities<TrackerTraits>::tip(tracks_view, it)); };

          // full crazy combinatorics
          // full crazy combinatorics
          int ntr = thisCell.tracks().size();
          for (int i = 0; i < ntr - 1; ++i) {
            auto it = thisCell.tracks()[i];
            auto qi = tracks_view[it].quality();
            if (qi <= reject)
              continue;
            auto opi = tracks_view[it].state()(2);
            auto e2opi = tracks_view[it].covariance()(9);
            auto cti = tracks_view[it].state()(3);
            auto e2cti = tracks_view[it].covariance()(12);
            for (auto j = i + 1; j < ntr; ++j) {
              auto jt = thisCell.tracks()[j];
              auto qj = tracks_view[jt].quality();
              if (qj <= reject)
                continue;
              auto opj = tracks_view[jt].state()(2);
              auto ctj = tracks_view[jt].state()(3);
              auto dct = nSigma2 * (tracks_view[jt].covariance()(12) + e2cti);
              if ((cti - ctj) * (cti - ctj) > dct)
                continue;
              auto dop = nSigma2 * (tracks_view[jt].covariance()(9) + e2opi);
              if ((opi - opj) * (opi - opj) > dop)
                continue;
              if ((qj < qi) || (qj == qi && score(it) < score(jt)))
                tracks_view[jt].quality() = reject;
              else {
                tracks_view[it].quality() = reject;
                break;
              }
            }
          }

          // find maxQual
          auto maxQual = reject;  // no duplicate!
          for (auto it : thisCell.tracks()) {
            if (tracks_view[it].quality() > maxQual)
              maxQual = tracks_view[it].quality();
          }

          if (maxQual <= loose)
            continue;

          // find min score
          for (auto it : thisCell.tracks()) {
            if (tracks_view[it].quality() == maxQual && score(it) < mc) {
              mc = score(it);
              im = it;
            }
          }

          if (tkNotFound == im)
            continue;

          // mark all other duplicates  (not yet, keep it loose)
          for (auto it : thisCell.tracks()) {
            if (tracks_view[it].quality() > loose && it != im)
              tracks_view[it].quality() = loose;  //no race:  simple assignment of the same constant
          }
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_connect {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    cms::alpakatools::AtomicPairCounter *apc1,
                                    cms::alpakatools::AtomicPairCounter *apc2,  // just to zero them
                                    HitsConstView<TrackerTraits> hh,
                                    GPUCACellT<TrackerTraits> *cells,
                                    uint32_t *nCells,
                                    CellNeighborsVector<TrackerTraits> *cellNeighbors,
                                    OuterHitOfCell<TrackerTraits> const isOuterHitOfCell,
                                    CAParams<TrackerTraits> params) const {
        using Cell = GPUCACellT<TrackerTraits>;

        const uint32_t dimIndexY = 0u;
        const uint32_t dimIndexX = 1u;
        const uint32_t threadIdxY(alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[dimIndexY]);
        const uint32_t threadIdxLocalX(alpaka::getIdx<alpaka::Block, alpaka::Threads>(acc)[dimIndexX]);

        if (0 == (threadIdxY + threadIdxLocalX)) {
          (*apc1) = 0;
          (*apc2) = 0;
        }  // ready for next kernel

        constexpr uint32_t last_bpix1_detIndex = TrackerTraits::last_bpix1_detIndex;
        constexpr uint32_t last_barrel_detIndex = TrackerTraits::last_barrel_detIndex;

        // const Vec2D nCellsVec{(*nCells), 1u};

        cms::alpakatools::for_each_element_in_grid_strided(
            acc,
            (*nCells),
            0u,
            [&](uint32_t idx) {
              auto cellIndex = idx;
              auto &thisCell = cells[idx];
              auto innerHitId = thisCell.inner_hit_id();
              // if (int(innerHitId) < isOuterHitOfCell.offset)
              //   continue;
              uint32_t numberOfPossibleNeighbors = isOuterHitOfCell[innerHitId].size();
              auto vi = isOuterHitOfCell[innerHitId].data();

              auto ri = thisCell.inner_r(hh);
              auto zi = thisCell.inner_z(hh);

              auto ro = thisCell.outer_r(hh);
              auto zo = thisCell.outer_z(hh);
              auto isBarrel = thisCell.inner_detIndex(hh) < last_barrel_detIndex;

              cms::alpakatools::for_each_element_in_block_strided(
                  acc,
                  numberOfPossibleNeighbors,
                  0u,
                  [&](uint32_t j) {
                    auto otherCell = (vi[j]);
                    auto &oc = cells[otherCell];
                    auto r1 = oc.inner_r(hh);
                    auto z1 = oc.inner_z(hh);
                    bool aligned =
                        Cell::areAlignedRZ(r1,
                                           z1,
                                           ri,
                                           zi,
                                           ro,
                                           zo,
                                           params.ptmin_,
                                           isBarrel ? params.CAThetaCutBarrel_
                                                    : params.CAThetaCutForward_);  // 2.f*thetaCut); // FIXME tune cuts
                    if (aligned &&
                        thisCell.dcaCut(hh,
                                        oc,
                                        oc.inner_detIndex(hh) < last_bpix1_detIndex ? params.dcaCutInnerTriplet_
                                                                                    : params.dcaCutOuterTriplet_,
                                        params.hardCurvCut_)) {  // FIXME tune cuts
                      oc.addOuterNeighbor(acc, cellIndex, *cellNeighbors);
                      thisCell.setStatusBits(Cell::StatusBit::kUsed);
                      oc.setStatusBits(Cell::StatusBit::kUsed);
                    }
                  },
                  dimIndexX);  // loop on inner cells
            },
            dimIndexY);  // loop on outer cells
      }
    };
    template <typename TrackerTraits>
    class kernel_find_ntuplets {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    HitsConstView<TrackerTraits> hh,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    GPUCACellT<TrackerTraits> *__restrict__ cells,
                                    uint32_t const *nCells,
                                    CellTracksVector<TrackerTraits> *cellTracks,
                                    cms::alpakatools::AtomicPairCounter *apc,
                                    CAParams<TrackerTraits> params) const {
        // recursive: not obvious to widen

        using Cell = GPUCACellT<TrackerTraits>;

#ifdef GPU_DEBUG
        const uint32_t threadIdx(alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[0u]);
        if (threadIdx == 0)
          printf("starting producing ntuplets from %d cells \n", *nCells);
#endif

        for (auto idx : cms::alpakatools::elements_with_stride(acc, (*nCells))) {
          auto const &thisCell = cells[idx];

          if (thisCell.isKilled())
            continue;  // cut by earlyFishbone

          // we require at least three hits...

          if (thisCell.outerNeighbors().empty())
            continue;

          auto pid = thisCell.layerPairId();
          bool doit = params.startingLayerPair(pid);

          constexpr uint32_t maxDepth = TrackerTraits::maxDepth;

          if (doit) {
            typename Cell::TmpTuple stack;
            stack.reset();
            bool bpix1Start = params.startAt0(pid);
            thisCell.template find_ntuplets<maxDepth, TAcc>(acc,
                                                            hh,
                                                            cells,
                                                            *cellTracks,
                                                            tracks_view.hitIndices(),
                                                            *apc,
                                                            tracks_view.quality(),
                                                            stack,
                                                            params.minHitsPerNtuplet_,
                                                            bpix1Start);
            ALPAKA_ASSERT_OFFLOAD(stack.empty());
          }
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_mark_used {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    GPUCACellT<TrackerTraits> *__restrict__ cells,
                                    uint32_t const *nCells) const {
        using Cell = GPUCACellT<TrackerTraits>;
        for (auto idx : cms::alpakatools::elements_with_stride(acc, (*nCells))) {
          auto &thisCell = cells[idx];
          if (!thisCell.tracks().empty())
            thisCell.setStatusBits(Cell::StatusBit::kInTrack);
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_countMultiplicity {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    TupleMultiplicity<TrackerTraits> *tupleMultiplicity) const {
        for (auto it : cms::alpakatools::elements_with_stride(acc, tracks_view.hitIndices().nbins())) {
          auto nhits = tracks_view.hitIndices().size(it);
          if (nhits < 3)
            continue;
          if (tracks_view[it].quality() == Quality::edup)
            continue;
          ALPAKA_ASSERT_OFFLOAD(tracks_view[it].quality() == Quality::bad);
          if (nhits > TrackerTraits::maxHitsOnTrack)  // current limit
            printf("wrong mult %d %d\n", it, nhits);
          ALPAKA_ASSERT_OFFLOAD(nhits <= TrackerTraits::maxHitsOnTrack);
          tupleMultiplicity->count(acc, nhits);
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_fillMultiplicity {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    TupleMultiplicity<TrackerTraits> *tupleMultiplicity) const {
        for (auto it : cms::alpakatools::elements_with_stride(acc, tracks_view.hitIndices().nbins())) {
          auto nhits = tracks_view.hitIndices().size(it);
          if (nhits < 3)
            continue;
          if (tracks_view[it].quality() == Quality::edup)
            continue;
          ALPAKA_ASSERT_OFFLOAD(tracks_view[it].quality() == Quality::bad);
          if (nhits > TrackerTraits::maxHitsOnTrack)
            printf("wrong mult %d %d\n", it, nhits);
          ALPAKA_ASSERT_OFFLOAD(nhits <= TrackerTraits::maxHitsOnTrack);
          tupleMultiplicity->fill(acc, nhits, it);
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_classifyTracks {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    QualityCuts<TrackerTraits> cuts) const {
        for (auto it : cms::alpakatools::elements_with_stride(acc, tracks_view.hitIndices().nbins())) {
          auto nhits = tracks_view.hitIndices().size(it);
          if (nhits == 0)
            break;  // guard

          // if duplicate: not even fit
          if (tracks_view[it].quality() == Quality::edup)
            continue;

          ALPAKA_ASSERT_OFFLOAD(tracks_view[it].quality() == Quality::bad);

          // mark doublets as bad
          if (nhits < 3)
            continue;

          // if the fit has any invalid parameters, mark it as bad
          bool isNaN = false;
          for (int i = 0; i < 5; ++i) {
            isNaN |= std::isnan(tracks_view[it].state()(i));
          }
          if (isNaN) {
#ifdef NTUPLE_DEBUG
            printf("NaN in fit %d size %d chi2 %f\n", it, tracks_view.hitIndices().size(it), tracks_view[it].chi2());
#endif
            continue;
          }

          tracks_view[it].quality() = Quality::strict;

          if (cuts.strictCut(tracks_view, it))
            continue;

          tracks_view[it].quality() = Quality::tight;

          if (cuts.isHP(tracks_view, nhits, it))
            tracks_view[it].quality() = Quality::highPurity;
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_doStatsForTracks {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc, TkSoAView<TrackerTraits> tracks_view, Counters *counters) const {
        for (auto idx : cms::alpakatools::elements_with_stride(acc, tracks_view.hitIndices().nbins())) {
          if (tracks_view.hitIndices().size(idx) == 0)
            break;  //guard
          if (tracks_view[idx].quality() < Quality::loose)
            continue;
          alpaka::atomicAdd(acc, &(counters->nLooseTracks), 1ull, alpaka::hierarchy::Blocks{});
          if (tracks_view[idx].quality() < Quality::strict)
            continue;
          alpaka::atomicAdd(acc, &(counters->nGoodTracks), 1ull, alpaka::hierarchy::Blocks{});
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_countHitInTracks {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    HitToTuple<TrackerTraits> *hitToTuple) const {
        for (auto idx : cms::alpakatools::elements_with_stride(acc, tracks_view.hitIndices().nbins())) {
          if (tracks_view.hitIndices().size(idx) == 0)
            break;  // guard
          for (auto h = tracks_view.hitIndices().begin(idx); h != tracks_view.hitIndices().end(idx); ++h)
            hitToTuple->count(acc, *h);
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_fillHitInTracks {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    HitToTuple<TrackerTraits> *hitToTuple) const {
        for (auto idx : cms::alpakatools::elements_with_stride(acc, tracks_view.hitIndices().nbins())) {
          if (tracks_view.hitIndices().size(idx) == 0)
            break;  // guard
          for (auto h = tracks_view.hitIndices().begin(idx); h != tracks_view.hitIndices().end(idx); ++h)
            hitToTuple->fill(acc, *h, idx);
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_fillHitDetIndices {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    HitsConstView<TrackerTraits> hh) const {
        // copy offsets
        for (auto idx : cms::alpakatools::elements_with_stride(acc, tracks_view.hitIndices().totbins())) {
          tracks_view.detIndices().off[idx] = tracks_view.hitIndices().off[idx];
        }
        // fill hit indices
        auto nhits = hh.nHits();
        for (auto idx : cms::alpakatools::elements_with_stride(acc, tracks_view.hitIndices().size())) {
          ALPAKA_ASSERT_OFFLOAD(tracks_view.hitIndices().bins[idx] < nhits);
          tracks_view.detIndices().bins[idx] = hh[tracks_view.hitIndices().bins[idx]].detectorIndex();
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_fillNLayers {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    cms::alpakatools::AtomicPairCounter *apc) const {
        // clamp the number of tracks to the capacity of the SoA
        auto ntracks = std::min<int>(apc->get().m, tracks_view.metadata().size() - 1);
        const uint32_t threadIdx(alpaka::getIdx<alpaka::Grid, alpaka::Threads>(acc)[0u]);
        if (0 == threadIdx)
          tracks_view.nTracks() = ntracks;
        for (auto idx : cms::alpakatools::elements_with_stride(acc, ntracks)) {
          auto nHits = TracksUtilities<TrackerTraits>::nHits(tracks_view, idx);
          ALPAKA_ASSERT_OFFLOAD(nHits >= 3);
          tracks_view[idx].nLayers() = TracksUtilities<TrackerTraits>::computeNumberOfLayers(tracks_view, idx);
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_doStatsForHitInTracks {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    HitToTuple<TrackerTraits> const *__restrict__ hitToTuple,
                                    Counters *counters) const {
        auto &c = *counters;
        for (auto idx : cms::alpakatools::elements_with_stride(acc, hitToTuple->nbins())) {
          if (hitToTuple->size(idx) == 0)
            continue;  // SHALL NOT BE break
          alpaka::atomicAdd(acc, &c.nUsedHits, 1ull, alpaka::hierarchy::Blocks{});
          if (hitToTuple->size(idx) > 1)
            alpaka::atomicAdd(acc, &c.nDupHits, 1ull, alpaka::hierarchy::Blocks{});
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_countSharedHit {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    int *__restrict__ nshared,
                                    HitContainer<TrackerTraits> const *__restrict__ ptuples,
                                    Quality const *__restrict__ quality,
                                    HitToTuple<TrackerTraits> const *__restrict__ phitToTuple) const {
        constexpr auto loose = Quality::loose;

        auto &hitToTuple = *phitToTuple;
        auto const &foundNtuplets = *ptuples;
        for (auto idx : cms::alpakatools::elements_with_stride(acc, hitToTuple->nbins())) {
          if (hitToTuple.size(idx) < 2)
            continue;

          int nt = 0;

          // count "good" tracks
          for (auto it = hitToTuple.begin(idx); it != hitToTuple.end(idx); ++it) {
            if (quality[*it] < loose)
              continue;
            ++nt;
          }

          if (nt < 2)
            continue;

          // now mark  each track triplet as sharing a hit
          for (auto it = hitToTuple.begin(idx); it != hitToTuple.end(idx); ++it) {
            if (foundNtuplets.size(*it) > 3)
              continue;
            alpaka::atomicAdd(acc, &nshared[*it], 1ull, alpaka::hierarchy::Blocks{});
          }

        }  //  hit loop
      }
    };

    template <typename TrackerTraits>
    class kernel_markSharedHit {
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    int const *__restrict__ nshared,
                                    HitContainer<TrackerTraits> const *__restrict__ tuples,
                                    Quality *__restrict__ quality,
                                    bool dupPassThrough) const {
        // constexpr auto bad = Quality::bad;
        constexpr auto dup = Quality::dup;
        constexpr auto loose = Quality::loose;
        // constexpr auto strict = Quality::strict;

        // quality to mark rejected
        auto const reject = dupPassThrough ? loose : dup;
        for (auto idx : cms::alpakatools::elements_with_stride(acc, tuples->nbins())) {
          if (tuples->size(idx) == 0)
            break;  //guard
          if (quality[idx] <= reject)
            continue;
          if (nshared[idx] > 2)
            quality[idx] = reject;
        }
      }
    };

    // mostly for very forward triplets.....
    template <typename TrackerTraits>
    class kernel_rejectDuplicate {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    uint16_t nmin,
                                    bool dupPassThrough,
                                    HitToTuple<TrackerTraits> const *__restrict__ phitToTuple) const {
        // quality to mark rejected
        auto const reject = dupPassThrough ? Quality::loose : Quality::dup;

        auto &hitToTuple = *phitToTuple;

        for (auto idx : cms::alpakatools::elements_with_stride(acc, hitToTuple.nbins())) {
          if (hitToTuple.size(idx) < 2)
            continue;

          auto score = [&](auto it, auto nl) { return std::abs(TracksUtilities<TrackerTraits>::tip(tracks_view, it)); };

          // full combinatorics
          for (auto ip = hitToTuple.begin(idx); ip < hitToTuple.end(idx) - 1; ++ip) {
            auto const it = *ip;
            auto qi = tracks_view[it].quality();
            if (qi <= reject)
              continue;
            auto opi = tracks_view[it].state()(2);
            auto e2opi = tracks_view[it].covariance()(9);
            auto cti = tracks_view[it].state()(3);
            auto e2cti = tracks_view[it].covariance()(12);
            auto nli = tracks_view[it].nLayers();
            for (auto jp = ip + 1; jp < hitToTuple.end(idx); ++jp) {
              auto const jt = *jp;
              auto qj = tracks_view[jt].quality();
              if (qj <= reject)
                continue;
              auto opj = tracks_view[jt].state()(2);
              auto ctj = tracks_view[jt].state()(3);
              auto dct = nSigma2 * (tracks_view[jt].covariance()(12) + e2cti);
              if ((cti - ctj) * (cti - ctj) > dct)
                continue;
              auto dop = nSigma2 * (tracks_view[jt].covariance()(9) + e2opi);
              if ((opi - opj) * (opi - opj) > dop)
                continue;
              auto nlj = tracks_view[jt].nLayers();
              if (nlj < nli || (nlj == nli && (qj < qi || (qj == qi && score(it, nli) < score(jt, nlj)))))
                tracks_view[jt].quality() = reject;
              else {
                tracks_view[it].quality() = reject;
                break;
              }
            }
          }
        }
      }
    };

    template <typename TrackerTraits>
    class kernel_sharedHitCleaner {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    HitsConstView<TrackerTraits> hh,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    int nmin,
                                    bool dupPassThrough,
                                    HitToTuple<TrackerTraits> const *__restrict__ phitToTuple) const {
        // quality to mark rejected
        auto const reject = dupPassThrough ? Quality::loose : Quality::dup;
        // quality of longest track
        auto const longTqual = Quality::highPurity;

        auto &hitToTuple = *phitToTuple;

        uint32_t l1end = hh.hitsLayerStart()[1];

        for (auto idx : cms::alpakatools::elements_with_stride(acc, hitToTuple.nbins())) {
          if (hitToTuple.size(idx) < 2)
            continue;

          int8_t maxNl = 0;

          // find maxNl
          for (auto it = hitToTuple.begin(idx); it != hitToTuple.end(idx); ++it) {
            if (tracks_view[*it].quality() < longTqual)
              continue;
            // if (tracks_view[*it].nHits()==3) continue;
            auto nl = tracks_view[*it].nLayers();
            maxNl = std::max(nl, maxNl);
          }

          if (maxNl < 4)
            continue;

          // quad pass through (leave for tests)
          // maxNl = std::min(4, maxNl);

          // kill all tracks shorter than maxHl (only triplets???
          for (auto it = hitToTuple.begin(idx); it != hitToTuple.end(idx); ++it) {
            auto nl = tracks_view[*it].nLayers();

            //checking if shared hit is on bpix1 and if the tuple is short enough
            if (idx < l1end and nl > nmin)
              continue;

            if (nl < maxNl && tracks_view[*it].quality() > reject)
              tracks_view[*it].quality() = reject;
          }
        }
      }
    };
    template <typename TrackerTraits>
    class kernel_tripletCleaner {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    uint16_t nmin,
                                    bool dupPassThrough,
                                    HitToTuple<TrackerTraits> const *__restrict__ phitToTuple) const {
        // quality to mark rejected
        auto const reject = Quality::loose;
        /// min quality of good
        auto const good = Quality::strict;

        auto &hitToTuple = *phitToTuple;

        for (auto idx : cms::alpakatools::elements_with_stride(acc, hitToTuple.nbins())) {
          if (hitToTuple.size(idx) < 2)
            continue;

          float mc = maxScore;
          uint16_t im = tkNotFound;
          bool onlyTriplets = true;

          // check if only triplets
          for (auto it = hitToTuple.begin(idx); it != hitToTuple.end(idx); ++it) {
            if (tracks_view[*it].quality() <= good)
              continue;
            onlyTriplets &= TracksUtilities<TrackerTraits>::isTriplet(tracks_view, *it);
            if (!onlyTriplets)
              break;
          }

          // only triplets
          if (!onlyTriplets)
            continue;

          // for triplets choose best tip!  (should we first find best quality???)
          for (auto ip = hitToTuple.begin(idx); ip != hitToTuple.end(idx); ++ip) {
            auto const it = *ip;
            if (tracks_view[it].quality() >= good &&
                std::abs(TracksUtilities<TrackerTraits>::tip(tracks_view, it)) < mc) {
              mc = std::abs(TracksUtilities<TrackerTraits>::tip(tracks_view, it));
              im = it;
            }
          }

          if (tkNotFound == im)
            continue;

          // mark worse ambiguities
          for (auto ip = hitToTuple.begin(idx); ip != hitToTuple.end(idx); ++ip) {
            auto const it = *ip;
            if (tracks_view[it].quality() > reject && it != im)
              tracks_view[it].quality() = reject;  //no race:  simple assignment of the same constant
          }

        }  // loop over hits
      }
    };

    template <typename TrackerTraits>
    class kernel_simpleTripletCleaner {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    uint16_t nmin,
                                    bool dupPassThrough,
                                    HitToTuple<TrackerTraits> const *__restrict__ phitToTuple) const {
        // quality to mark rejected
        auto const reject = Quality::loose;
        /// min quality of good
        auto const good = Quality::loose;

        auto &hitToTuple = *phitToTuple;

        for (auto idx : cms::alpakatools::elements_with_stride(acc, hitToTuple.nbins())) {
          if (hitToTuple.size(idx) < 2)
            continue;

          float mc = maxScore;
          uint16_t im = tkNotFound;

          // choose best tip!  (should we first find best quality???)
          for (auto ip = hitToTuple.begin(idx); ip != hitToTuple.end(idx); ++ip) {
            auto const it = *ip;
            if (tracks_view[it].quality() >= good &&
                std::abs(TracksUtilities<TrackerTraits>::tip(tracks_view, it)) < mc) {
              mc = std::abs(TracksUtilities<TrackerTraits>::tip(tracks_view, it));
              im = it;
            }
          }

          if (tkNotFound == im)
            continue;

          // mark worse ambiguities
          for (auto ip = hitToTuple.begin(idx); ip != hitToTuple.end(idx); ++ip) {
            auto const it = *ip;
            if (tracks_view[it].quality() > reject && TracksUtilities<TrackerTraits>::isTriplet(tracks_view, it) &&
                it != im)
              tracks_view[it].quality() = reject;  //no race:  simple assignment of the same constant
          }

        }  // loop over hits
      }
    };

    template <typename TrackerTraits>
    class kernel_print_found_ntuplets {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc,
                                    HitsConstView<TrackerTraits> hh,
                                    TkSoAView<TrackerTraits> tracks_view,
                                    HitToTuple<TrackerTraits> const *__restrict__ phitToTuple,
                                    int32_t firstPrint,
                                    int32_t lastPrint,
                                    int iev) const {
        constexpr auto loose = Quality::loose;

        for (auto i : cms::alpakatools::elements_with_stride(acc, tracks_view.hitIndices().nbins())) {
          auto nh = tracks_view.hitIndices().size(i);
          if (nh < 3)
            continue;
          if (tracks_view[i].quality() < loose)
            continue;
          printf("TK: %d %d %d %d %f %f %f %f %f %f %f %.3f %.3f %.3f %.3f %.3f %.3f %.3f\n",
                 10000 * iev + i,
                 int(tracks_view[i].quality()),
                 nh,
                 tracks_view[i].nLayers(),
                 TracksUtilities<TrackerTraits>::charge(tracks_view, i),
                 tracks_view[i].pt(),
                 tracks_view[i].eta(),
                 TracksUtilities<TrackerTraits>::phi(tracks_view, i),
                 TracksUtilities<TrackerTraits>::tip(tracks_view, i),
                 TracksUtilities<TrackerTraits>::zip(tracks_view, i),
                 tracks_view[i].chi2(),
                 hh[*tracks_view.hitIndices().begin(i)].zGlobal(),
                 hh[*(tracks_view.hitIndices().begin(i) + 1)].zGlobal(),
                 hh[*(tracks_view.hitIndices().begin(i) + 2)].zGlobal(),
                 nh > 3 ? hh[int(*(tracks_view.hitIndices().begin(i) + 3))].zGlobal() : 0,
                 nh > 4 ? hh[int(*(tracks_view.hitIndices().begin(i) + 4))].zGlobal() : 0,
                 nh > 5 ? hh[int(*(tracks_view.hitIndices().begin(i) + 5))].zGlobal() : 0,
                 nh > 6 ? hh[int(*(tracks_view.hitIndices().begin(i) + nh - 1))].zGlobal() : 0);
        }
      }
    };

    class kernel_printCounters {
    public:
      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(TAcc const &acc, Counters const *counters) const {
        auto const &c = *counters;
        printf(
            "||Counters | nEvents | nHits | nCells | nTuples | nFitTacks  |  nLooseTracks  |  nGoodTracks | "
            "nUsedHits "
            "| "
            "nDupHits | "
            "nFishCells | "
            "nKilledCells | "
            "nUsedCells | nZeroTrackCells ||\n");
        printf("Counters Raw %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld %lld\n",
               c.nEvents,
               c.nHits,
               c.nCells,
               c.nTuples,
               c.nFitTracks,
               c.nLooseTracks,
               c.nGoodTracks,
               c.nUsedHits,
               c.nDupHits,
               c.nFishCells,
               c.nKilledCells,
               c.nEmptyCells,
               c.nZeroTrackCells);
        printf(
            "Counters Norm %lld ||  %.1f|  %.1f|  %.1f|  %.1f|  %.1f|  %.1f|  %.1f|  %.1f|  %.3f|  %.3f|  "
            "%.3f|  "
            "%.3f||\n",
            c.nEvents,
            c.nHits / double(c.nEvents),
            c.nCells / double(c.nEvents),
            c.nTuples / double(c.nEvents),
            c.nFitTracks / double(c.nEvents),
            c.nLooseTracks / double(c.nEvents),
            c.nGoodTracks / double(c.nEvents),
            c.nUsedHits / double(c.nEvents),
            c.nDupHits / double(c.nEvents),
            c.nFishCells / double(c.nCells),
            c.nKilledCells / double(c.nCells),
            c.nEmptyCells / double(c.nCells),
            c.nZeroTrackCells / double(c.nCells));
      }
    };
  }  // namespace caHitNtupletGeneratorKernels
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE
