#ifndef RecoPixelVertexing_PixelTriplets_plugins_alpaka_gpuPixelDoubletsAlgos_h
#define RecoPixelVertexing_PixelTriplets_plugins_alpaka_gpuPixelDoubletsAlgos_h

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <alpaka/alpaka.hpp>
#include "HeterogeneousCore/AlpakaInterface/interface/traits.h"
#include "HeterogeneousCore/AlpakaInterface/interface/workdivision.h"
#include "HeterogeneousCore/AlpakaUtilities/interface/VecArray.h"
#include "DataFormats/TrackingRecHitSoA/interface/TrackingRecHitsLayout.h"
#include "DataFormats/Math/interface/approx_atan2.h"
#include "DataFormats/TrackerCommon/interface/SimplePixelTopology.h"
#include "../CAStructures.h"
#include "GPUCACell.h"

//#define GPU_DEBUG
//#define NTUPLE_DEBUG
namespace ALPAKA_ACCELERATOR_NAMESPACE {
  namespace gpuPixelDoublets {
    using namespace cms::alpakatools;

    template <typename TrackerTraits>
    using CellNeighbors = caStructures::CellNeighborsT<TrackerTraits>;
    template <typename TrackerTraits>
    using CellTracks = caStructures::CellTracksT<TrackerTraits>;
    template <typename TrackerTraits>
    using CellNeighborsVector = caStructures::CellNeighborsVectorT<TrackerTraits>;
    template <typename TrackerTraits>
    using CellTracksVector = caStructures::CellTracksVectorT<TrackerTraits>;
    template <typename TrackerTraits>
    using OuterHitOfCell = caStructures::OuterHitOfCellT<TrackerTraits>;
    template <typename TrackerTraits>
    using HitsConstView = typename GPUCACellT<TrackerTraits>::HitsConstView;

    template <typename TrackerTraits>
    struct CellCutsT {
      using H = HitsConstView<TrackerTraits>;
      using T = TrackerTraits;

      const uint32_t maxNumberOfDoublets_;
      const bool doClusterCut_;
      const bool doZ0Cut_;
      const bool doPtCut_;
      const bool idealConditions_;  //this is actually not used by phase2

      template <typename TAcc>
      ALPAKA_FN_ACC ALPAKA_FN_INLINE bool __attribute__((always_inline))
      zSizeCut(const TAcc& acc, H hh, int i, int o) const {
        const uint32_t mi = hh[i].detectorIndex();

        bool innerB1 = mi < T::last_bpix1_detIndex;
        bool isOuterLadder = idealConditions_ ? true : 0 == (mi / 8) % 2;
        auto mes = (!innerB1) || isOuterLadder ? hh[i].clusterSizeY() : -1;

        if (mes < 0)
          return false;

        const uint32_t mo = hh[o].detectorIndex();
        auto so = hh[o].clusterSizeY();

        auto dz = hh[i].zGlobal() - hh[o].zGlobal();
        auto dr = hh[i].rGlobal() - hh[o].rGlobal();

        auto innerBarrel = mi < T::last_barrel_detIndex;
        auto onlyBarrel = mo < T::last_barrel_detIndex;

        if (not innerBarrel and not onlyBarrel)
          return false;
        auto dy = innerB1 ? T::maxDYsize12 : T::maxDYsize;

        return onlyBarrel ? so > 0 && std::abs(so - mes) > dy
                          : innerBarrel && std::abs(mes - int(std::abs(dz / dr) * T::dzdrFact + 0.5f)) > T::maxDYPred;
      }

      template <typename TAcc>
      ALPAKA_FN_ACC ALPAKA_FN_INLINE bool __attribute__((always_inline))
      clusterCut(const TAcc& acc, H hh, uint32_t i) const {
        const uint32_t mi = hh[i].detectorIndex();
        bool innerB1orB2 = mi < T::last_bpix2_detIndex;

        if (!innerB1orB2)
          return false;

        bool innerB1 = mi < T::last_bpix1_detIndex;
        bool isOuterLadder = idealConditions_ ? true : 0 == (mi / 8) % 2;
        auto mes = (!innerB1) || isOuterLadder ? hh[i].clusterSizeY() : -1;

        if (innerB1)  // B1
          if (mes > 0 && mes < T::minYsizeB1)
            return true;                                                                 // only long cluster  (5*8)
        bool innerB2 = (mi >= T::last_bpix1_detIndex) && (mi < T::last_bpix2_detIndex);  //FIXME number
        if (innerB2)                                                                     // B2 and F1
          if (mes > 0 && mes < T::minYsizeB2)
            return true;

        return false;
      }
    };

    template <typename TrackerTraits, typename TAcc>
    ALPAKA_FN_ACC ALPAKA_FN_INLINE void __attribute__((always_inline))
    doubletsFromHisto(const TAcc& acc,
                      uint32_t nPairs,
                      GPUCACellT<TrackerTraits>* cells,
                      uint32_t* nCells,
                      CellNeighborsVector<TrackerTraits>* cellNeighbors,
                      CellTracksVector<TrackerTraits>* cellTracks,
                      HitsConstView<TrackerTraits> hh,
                      OuterHitOfCell<TrackerTraits> isOuterHitOfCell,
                      CellCutsT<TrackerTraits> const& cuts) {  // ysize cuts (z in the barrel)  times 8
      // these are used if doClusterCut is true

      const bool doClusterCut = cuts.doClusterCut_;
      const bool doZ0Cut = cuts.doZ0Cut_;
      const bool doPtCut = cuts.doPtCut_;
      const uint32_t maxNumOfDoublets = cuts.maxNumberOfDoublets_;

      using PhiBinner = typename TrackingRecHitAlpakaSoA<TrackerTraits>::PhiBinner;

      auto const& __restrict__ phiBinner = hh.phiBinner();
      uint32_t const* __restrict__ offsets = hh.hitsLayerStart().data();
      ALPAKA_ASSERT_OFFLOAD(offsets);

      auto layerSize = [=](uint8_t li) { return offsets[li + 1] - offsets[li]; };

      // nPairsMax to be optimized later (originally was 64).
      // If it should much be bigger, consider using a block-wide parallel prefix scan,
      // e.g. see  https://nvlabs.github.io/cub/classcub_1_1_warp_scan.html
      auto& innerLayerCumulativeSize = alpaka::declareSharedVar<uint32_t[TrackerTraits::nPairs], __COUNTER__>(acc);
      auto& ntot = alpaka::declareSharedVar<uint32_t, __COUNTER__>(acc);

      const uint32_t dimIndexY = 0u;
      const uint32_t dimIndexX = 1u;
      const uint32_t threadIdxLocalY(alpaka::getIdx<alpaka::Block, alpaka::Threads>(acc)[dimIndexY]);
      const uint32_t threadIdxLocalX(alpaka::getIdx<alpaka::Block, alpaka::Threads>(acc)[dimIndexX]);

      if (threadIdxLocalY == 0 && threadIdxLocalX == 0) {
        innerLayerCumulativeSize[0] = layerSize(TrackerTraits::layerPairs[0]);
        for (uint32_t i = 1; i < nPairs; ++i) {
          innerLayerCumulativeSize[i] = innerLayerCumulativeSize[i - 1] + layerSize(TrackerTraits::layerPairs[2 * i]);
        }
        ntot = innerLayerCumulativeSize[nPairs - 1];
      }
      alpaka::syncBlockThreads(acc);

      // x runs faster
      const uint32_t blockDimensionX(alpaka::getWorkDiv<alpaka::Block, alpaka::Elems>(acc)[dimIndexX]);
      const auto& [firstElementIdxNoStrideX, endElementIdxNoStrideX] =
          cms::alpakatools::element_index_range_in_block(acc, 0u, dimIndexX);

      uint32_t pairLayerId = 0;  // cannot go backward

      // Outermost loop on Y
      const uint32_t gridDimensionY(alpaka::getWorkDiv<alpaka::Grid, alpaka::Elems>(acc)[dimIndexY]);
      const auto& [firstElementIdxNoStrideY, endElementIdxNoStrideY] =
          cms::alpakatools::element_index_range_in_grid(acc, 0u, dimIndexY);
      uint32_t firstElementIdxY = firstElementIdxNoStrideY;
      uint32_t endElementIdxY = endElementIdxNoStrideY;

      for (uint32_t j = firstElementIdxY; j < ntot; ++j) {
        if (not cms::alpakatools::next_valid_element_index_strided(
                j, firstElementIdxY, endElementIdxY, gridDimensionY, ntot))
          break;

        while (j >= innerLayerCumulativeSize[pairLayerId++])
          ;
        --pairLayerId;  // move to lower_bound ??

        ALPAKA_ASSERT_OFFLOAD(pairLayerId < nPairs);
        ALPAKA_ASSERT_OFFLOAD(j < innerLayerCumulativeSize[pairLayerId]);
        ALPAKA_ASSERT_OFFLOAD(0 == pairLayerId || j >= innerLayerCumulativeSize[pairLayerId - 1]);

        uint8_t inner = TrackerTraits::layerPairs[2 * pairLayerId];
        uint8_t outer = TrackerTraits::layerPairs[2 * pairLayerId + 1];
        ALPAKA_ASSERT_OFFLOAD(outer > inner);

        auto hoff = PhiBinner::histOff(outer);
        auto i = (0 == pairLayerId) ? j : j - innerLayerCumulativeSize[pairLayerId - 1];
        i += offsets[inner];

        ALPAKA_ASSERT_OFFLOAD(i >= offsets[inner]);
        ALPAKA_ASSERT_OFFLOAD(i < offsets[inner + 1]);

        // found hit corresponding to our cuda thread, now do the job

        if (hh[i].detectorIndex() > gpuClustering::maxNumModules)
          continue;  // invalid

        /* maybe clever, not effective when zoCut is on
      auto bpos = (mi%8)/4;  // if barrel is 1 for z>0
      auto fpos = (outer>3) & (outer<7);
      if ( ((inner<3) & (outer>3)) && bpos!=fpos) continue;
      */

        auto mez = hh[i].zGlobal();

        if (mez < TrackerTraits::minz[pairLayerId] || mez > TrackerTraits::maxz[pairLayerId])
          continue;

        if (doClusterCut && outer > pixelTopology::last_barrel_layer && cuts.clusterCut(acc, hh, i))
          continue;

        auto mep = hh[i].iphi();
        auto mer = hh[i].rGlobal();

        // all cuts: true if fails
        constexpr float z0cut = TrackerTraits::z0Cut;              // cm
        constexpr float hardPtCut = TrackerTraits::doubletHardPt;  // GeV
        // cm (1 GeV track has 1 GeV/c / (e * 3.8T) ~ 87 cm radius in a 3.8T field)
        constexpr float minRadius = hardPtCut * 87.78f;
        constexpr float minRadius2T4 = 4.f * minRadius * minRadius;
        auto ptcut = [&](int j, int16_t idphi) {
          auto r2t4 = minRadius2T4;
          auto ri = mer;
          auto ro = hh[j].rGlobal();
          auto dphi = short2phi(idphi);
          return dphi * dphi * (r2t4 - ri * ro) > (ro - ri) * (ro - ri);
        };
        auto z0cutoff = [&](int j) {
          auto zo = hh[j].zGlobal();
          auto ro = hh[j].rGlobal();
          auto dr = ro - mer;
          return dr > TrackerTraits::maxr[pairLayerId] || dr < 0 || std::abs((mez * ro - mer * zo)) > z0cut * dr;
        };

        auto iphicut = TrackerTraits::phicuts[pairLayerId];

        auto kl = PhiBinner::bin(int16_t(mep - iphicut));
        auto kh = PhiBinner::bin(int16_t(mep + iphicut));
        auto incr = [](auto& k) { return k = (k + 1) % PhiBinner::nbins(); };

#ifdef GPU_DEBUG
        int tot = 0;
        int nmin = 0;
        int tooMany = 0;
#endif

        auto khh = kh;
        incr(khh);
        for (auto kk = kl; kk != khh; incr(kk)) {
#ifdef GPU_DEBUG
          if (kk != kl && kk != kh)
            nmin += phiBinner.size(kk + hoff);
#endif
          auto const* __restrict__ p = phiBinner.begin(kk + hoff);
          auto const* __restrict__ e = phiBinner.end(kk + hoff);
          auto const maxpIndex = e - p;

          // Here we parallelize in X
          //p += first;
          //for (; p < e; p += blockDimensionX) {
          uint32_t firstElementIdxX = firstElementIdxNoStrideX;
          uint32_t endElementIdxX = endElementIdxNoStrideX;
          for (uint32_t pIndex = firstElementIdxX; pIndex < maxpIndex; ++pIndex) {
            if (not cms::alpakatools::next_valid_element_index_strided(
                    pIndex, firstElementIdxX, endElementIdxX, blockDimensionX, maxpIndex))
              break;

            auto oi = p[pIndex];  // auto oi = __ldg(p); is not allowed since __ldg is device-only
            ALPAKA_ASSERT_OFFLOAD(oi >= offsets[outer]);
            ALPAKA_ASSERT_OFFLOAD(oi < offsets[outer + 1]);
            auto mo = hh[oi].detectorIndex();

            if (mo > gpuClustering::maxNumModules)
              continue;  //    invalid

            if (doZ0Cut && z0cutoff(oi))
              continue;
            auto mop = hh[oi].iphi();
            uint16_t idphi = std::min(std::abs(int16_t(mop - mep)), std::abs(int16_t(mep - mop)));
            if (idphi > iphicut)
              continue;

            if (doClusterCut && cuts.zSizeCut(acc, hh, i, oi))
              continue;
            if (doPtCut && ptcut(oi, idphi))
              continue;

            auto ind = alpaka::atomicAdd(acc, nCells, (uint32_t)1, alpaka::hierarchy::Blocks{});
            if (ind >= maxNumOfDoublets) {
              alpaka::atomicSub(acc, nCells, (uint32_t)1, alpaka::hierarchy::Blocks{});
              break;
            }  // move to SimpleVector??
            // int layerPairId, int doubletId, int innerHitId, int outerHitId)
            cells[ind].init(*cellNeighbors, *cellTracks, hh, pairLayerId, i, oi);
            isOuterHitOfCell[oi].push_back(acc, ind);
#ifdef GPU_DEBUG
            if (isOuterHitOfCell[oi].full())
              ++tooMany;
            ++tot;
#endif
          }
        }
//      #endif
#ifdef GPU_DEBUG
        if (tooMany > 0)
          printf("OuterHitOfCell full for %d in layer %d/%d, %d,%d %d, %d %.3f %.3f\n",
                 i,
                 inner,
                 outer,
                 nmin,
                 tot,
                 tooMany,
                 iphicut,
                 TrackerTraits::minz[pairLayerId],
                 TrackerTraits::maxz[pairLayerId]);
#endif
      }  // loop in block...
    }    // namespace gpuPixelDoublets
  }      // namespace gpuPixelDoublets
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE
#endif  // RecoPixelVertexing_PixelTriplets_plugins_alpaka_gpuPixelDoubletsAlgos_h
