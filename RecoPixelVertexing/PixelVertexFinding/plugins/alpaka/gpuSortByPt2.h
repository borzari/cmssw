#ifndef RecoPixelVertexing_PixelVertexFinding_plugins_alpaka_gpuSortByPt2_h
#define RecoPixelVertexing_PixelVertexFinding_plugins_alpaka_gpuSortByPt2_h

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <alpaka/alpaka.hpp>
#include "HeterogeneousCore/AlpakaInterface/interface/workdivision.h"
#include "HeterogeneousCore/AlpakaUtilities/interface/HistoContainer.h"
#include "HeterogeneousCore/AlpakaUtilities/interface/radixSort.h"
#include "DataFormats/Vertex/interface/ZVertexLayout.h"
#include "../PixelVertexWorkSpaceLayout.h"

#include "gpuVertexFinder.h"
namespace ALPAKA_ACCELERATOR_NAMESPACE {
  namespace gpuVertexFinder {
    using VtxSoAView = ::zVertex::ZVertexSoAView;
    using WsSoAView = ::gpuVertexFinder::workSpace::PixelVertexWorkSpaceSoAView;

    template <typename TAcc>
    ALPAKA_FN_ACC ALPAKA_FN_INLINE void sortByPt2(const TAcc& acc, VtxSoAView& data, WsSoAView& ws) {
      auto nt = ws.ntrks();
      float const* __restrict__ ptt2 = ws.ptt2();
      uint32_t const& nvFinal = data.nvFinal();

      int32_t const* __restrict__ iv = ws.iv();
      float* __restrict__ ptv2 = data.ptv2();
      uint16_t* __restrict__ sortInd = data.sortInd();

      // if (threadIdxLocal == 0)
      //    printf("sorting %d vertices\n",nvFinal);

      if (nvFinal < 1)
        return;

      // fill indexing
      for (auto i : cms::alpakatools::elements_with_stride(acc, nt)) {
        data.idv()[ws.itrk()[i]] = iv[i];
      };

      // can be done asynchronously at the end of previous event
      for (auto i : cms::alpakatools::elements_with_stride(acc, nvFinal)) {
        ptv2[i] = 0;
      };
      alpaka::syncBlockThreads(acc);

      for (auto i : cms::alpakatools::elements_with_stride(acc, nt)) {
        if (iv[i] <= 9990) {
          alpaka::atomicAdd(acc, &ptv2[iv[i]], ptt2[i], alpaka::hierarchy::Blocks{});
        }
      };
      alpaka::syncBlockThreads(acc);

      const uint32_t threadIdxLocal(alpaka::getIdx<alpaka::Block, alpaka::Threads>(acc)[0u]);
      if (1 == nvFinal) {
        if (threadIdxLocal == 0)
          sortInd[0] = 0;
        return;
      }
#if defined(ALPAKA_ACC_GPU_CUDA_ENABLED) || defined(ALPAKA_ACC_GPU_HIP_ENABLED)
      auto& sws = alpaka::declareSharedVar<uint16_t[1024], __COUNTER__>(acc);
      // sort using only 16 bits
      cms::alpakatools::radixSort<Acc1D, float, 2>(acc, ptv2, sortInd, sws, nvFinal);
#else
      for (uint16_t i = 0; i < nvFinal; ++i)
        sortInd[i] = i;
      std::sort(sortInd, sortInd + nvFinal, [&](auto i, auto j) { return ptv2[i] < ptv2[j]; });
#endif
    }

    class sortByPt2Kernel {
    public:
      template <typename TAcc>
      ALPAKA_FN_ACC void operator()(const TAcc& acc, VtxSoAView pdata, WsSoAView pws) const {
        sortByPt2(acc, pdata, pws);
      }
    };
  }  // namespace gpuVertexFinder
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE
#endif  // RecoPixelVertexing_PixelVertexFinding_plugins_gpuSortByPt2_h
