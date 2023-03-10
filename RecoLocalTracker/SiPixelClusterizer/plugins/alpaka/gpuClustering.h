#ifndef plugin_SiPixelClusterizer_alpaka_gpuClustering_h
#define plugin_SiPixelClusterizer_alpaka_gpuClustering_h

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <type_traits>
#include <alpaka/alpaka.hpp>

#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "HeterogeneousCore/AlpakaUtilities/interface/HistoContainer.h"
#include "DataFormats/SiPixelClusterSoA/interface/gpuClusteringConstants.h"
#include "DataFormats/TrackerCommon/interface/SimplePixelTopology.h"
#include "HeterogeneousCore/AlpakaUtilities/interface/SimpleVector.h"

// #include "gpuClusteringUtilities.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  namespace gpuClustering {

#ifdef GPU_DEBUG
    template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
    ALPAKA_STATIC_ACC_MEM_GLOBAL uint32_t gMaxHit = 0;
#endif

    struct PixelStatus {
      // Phase-1 pixel modules
      static constexpr uint32_t pixelSizeX = pixelTopology::Phase1::numRowsInModule;
      static constexpr uint32_t pixelSizeY = pixelTopology::Phase1::numColsInModule;

      // Use 0x00, 0x01, 0x03 so each can be OR'ed on top of the previous ones
      enum Status : uint32_t { kEmpty = 0x00, kFound = 0x01, kDuplicate = 0x03 };

      static constexpr uint32_t bits = 2;
      static constexpr uint32_t mask = (0x01 << bits) - 1;
      static constexpr uint32_t valuesPerWord = sizeof(uint32_t) * 8 / bits;
      static constexpr uint32_t size = pixelSizeX * pixelSizeY / valuesPerWord;

      ALPAKA_FN_ACC ALPAKA_FN_INLINE constexpr static uint32_t getIndex(uint16_t x, uint16_t y) {
        return (pixelSizeX * y + x) / valuesPerWord;
      }

      ALPAKA_FN_ACC ALPAKA_FN_INLINE constexpr static uint32_t getShift(uint16_t x, uint16_t y) {
        return (x % valuesPerWord) * 2;
      }

      ALPAKA_FN_ACC ALPAKA_FN_INLINE constexpr static Status getStatus(uint32_t const* __restrict__ status,
                                                                       uint16_t x,
                                                                       uint16_t y) {
        uint32_t index = getIndex(x, y);
        uint32_t shift = getShift(x, y);
        return Status{(status[index] >> shift) & mask};
      }

      ALPAKA_FN_ACC ALPAKA_FN_INLINE constexpr static bool isDuplicate(uint32_t const* __restrict__ status,
                                                                       uint16_t x,
                                                                       uint16_t y) {
        return getStatus(status, x, y) == kDuplicate;
      }

      template <typename TAcc, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC ALPAKA_FN_INLINE constexpr static void promote(TAcc const& acc,
                                                                   uint32_t* __restrict__ status,
                                                                   const uint16_t x,
                                                                   const uint16_t y) {
        uint32_t index = getIndex(x, y);
        uint32_t shift = getShift(x, y);
        uint32_t old_word = status[index];
        uint32_t expected = old_word;
        do {
          expected = old_word;
          Status old_status{(old_word >> shift) & mask};
          if (kDuplicate == old_status) {
            // nothing to do
            return;
          }
          Status new_status = (kEmpty == old_status) ? kFound : kDuplicate;
          uint32_t new_word = old_word | (static_cast<uint32_t>(new_status) << shift);
          old_word = alpaka::atomicCas(acc, &status[index], expected, new_word, alpaka::hierarchy::Blocks{});
        } while (expected != old_word);
      }

    };  // struct PixelStatus

    template <typename TrackerTraits>
    struct countModules {
      template <typename TAcc>  //, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(const TAcc& acc,
                                    SiPixelDigisLayoutSoAView digi_view,
                                    SiPixelClustersLayoutSoAView clus_view,
                                    // uint16_t const* __restrict__ id,
                                    // uint32_t* __restrict__ moduleStart,
                                    // int32_t* __restrict__ clusterId,
                                    const unsigned int numElements) const {
        [[maybe_unused]] constexpr int nMaxModules = TrackerTraits::numberOfModules;
        assert(nMaxModules < nMaxModules);  //TODO better naming

        cms::alpakatools::for_each_element_in_grid_strided(acc, numElements, [&](uint32_t i) {
          digi_view[i].clus() = i;
          if (::gpuClustering::invalidModuleId != digi_view[i].moduleId()) {
            int j = i - 1;
            while (j >= 0 and digi_view[j].moduleId() == ::gpuClustering::invalidModuleId)
              --j;
            if (j < 0 or digi_view[j].moduleId() != digi_view[i].moduleId()) {
              // boundary...
              auto loc = alpaka::atomicInc(
                  acc, clus_view.moduleStart(), std::decay_t<uint32_t>(nMaxModules), alpaka::hierarchy::Blocks{});

              clus_view[loc + 1].moduleStart() = i;
            }
          }
        });
      }
    };

    //  __launch_bounds__(256,4)
    template <typename TrackerTraits>
    struct findClus {
      template <typename TAcc>  //, typename = std::enable_if_t<alpaka::isAccelerator<TAcc>>>
      ALPAKA_FN_ACC void operator()(
          const TAcc& acc,
          SiPixelDigisLayoutSoAView digi_view,
          SiPixelClustersLayoutSoAView clus_view,
          // uint32_t* __restrict__ rawId,
          // uint16_t* __restrict__ id,           // module id of each pixel
          // uint16_t const* __restrict__ x,            // local coordinates of each pixel
          // uint16_t const* __restrict__ y,            //
          // uint32_t* __restrict__ nClustersInModule,  // output: number of clusters found in each module
          // uint32_t* __restrict__ moduleId,           // output: module id of each module
          // int32_t* __restrict__ clusterId,           // output: cluster id of each pixel
          const unsigned int numElements) const {
        const uint32_t blockIdx(alpaka::getIdx<alpaka::Grid, alpaka::Blocks>(acc)[0u]);
        if (blockIdx >= clus_view[0].moduleStart())
          return;

        auto firstPixel = clus_view[1 + blockIdx].moduleStart();
        auto thisModuleId = digi_view[firstPixel].moduleId();
        ALPAKA_ASSERT_OFFLOAD(thisModuleId < TrackerTraits::numberOfModules);

        const uint32_t threadIdxLocal(alpaka::getIdx<alpaka::Block, alpaka::Threads>(acc)[0u]);

#ifdef GPU_DEBUG
        if (thisModuleId % 100 == 1)
          if (threadIdxLocal == 0)
            printf("start clusterizer for module %d in block %d\n", thisModuleId, blockIdx);
#endif

        constexpr bool isPhase2 = std::is_base_of<pixelTopology::Phase2, TrackerTraits>::value;
        constexpr const uint32_t pixelStatusSize = isPhase2 ? 1 : PixelStatus::size;

        auto&& status = alpaka::declareSharedVar<uint32_t[pixelStatusSize], __COUNTER__>(
            acc);  // packed words array used to store the PixelStatus of each pixel

        // find the index of the first pixel not belonging to this module (or invalid)
        auto& msize = alpaka::declareSharedVar<unsigned int, __COUNTER__>(acc);
        msize = numElements;
        alpaka::syncBlockThreads(acc);

        // Stride = block size.
        const uint32_t blockDimension(alpaka::getWorkDiv<alpaka::Block, alpaka::Elems>(acc)[0u]);

        // Get thread / CPU element indices in block.
        const auto& [firstElementIdxNoStride, endElementIdxNoStride] =
            cms::alpakatools::element_index_range_in_block(acc, firstPixel);
        uint32_t firstElementIdx = firstElementIdxNoStride;
        uint32_t endElementIdx = endElementIdxNoStride;

        // skip threads not associated to an existing pixel
        for (uint32_t i = firstElementIdx; i < numElements; ++i) {
          if (not cms::alpakatools::next_valid_element_index_strided(
                  i, firstElementIdx, endElementIdx, blockDimension, numElements))
            break;
          auto id = digi_view[i].moduleId();
          if (id == ::gpuClustering::invalidModuleId)  // skip invalid pixels
            continue;
          if (id != thisModuleId) {  // find the first pixel in a different module
            alpaka::atomicMin(acc, &msize, i, alpaka::hierarchy::Threads{});
            break;
          }
        }

        //init hist  (ymax=416 < 512 : 9bits)
        constexpr uint32_t maxPixInModule = TrackerTraits::maxPixInModule;
        constexpr auto nbins = TrackerTraits::clusterBinning;
        constexpr auto nbits = TrackerTraits::clusterBits;

        using Hist = cms::alpakatools::HistoContainer<uint16_t, nbins, maxPixInModule, nbits, uint16_t>;
        auto& hist = alpaka::declareSharedVar<Hist, __COUNTER__>(acc);
        auto& ws = alpaka::declareSharedVar<Hist::Counter[32], __COUNTER__>(acc);

        cms::alpakatools::for_each_element_in_block_strided(acc, Hist::totbins(), [&](uint32_t j) { hist.off[j] = 0; });
        alpaka::syncBlockThreads(acc);

        ALPAKA_ASSERT_OFFLOAD((msize == numElements) or
                              ((msize < numElements) and (digi_view[msize].moduleId() != thisModuleId)));

        // limit to maxPixInModule  (FIXME if recurrent (and not limited to simulation with low threshold) one will need to implement something cleverer)
        if (0 == threadIdxLocal) {
          if (msize - firstPixel > maxPixInModule) {
            printf("too many pixels in module %d: %d > %d\n", thisModuleId, msize - firstPixel, maxPixInModule);
            msize = maxPixInModule + firstPixel;
          }
        }

        alpaka::syncBlockThreads(acc);
        ALPAKA_ASSERT_OFFLOAD(msize - firstPixel <= maxPixInModule);

#ifdef GPU_DEBUG
        auto& totGood = alpaka::declareSharedVar<uint32_t, __COUNTER__>(acc);
        totGood = 0;
        alpaka::syncBlockThreads(acc);
#endif

        // remove duplicate pixels
        if constexpr (not isPhase2) {
          if (msize > 1) {
            cms::alpakatools::for_each_element_in_block_strided(
                acc, PixelStatus::size, [&](uint32_t i) { status[i] = 0; });
            alpaka::syncBlockThreads(acc);

            // for (uint32_t i = firstElementIdx; i < msize - 1; ++i) {
            cms::alpakatools::for_each_element_in_block_strided(acc, msize - 1, firstElementIdx, [&](uint32_t i) {
              // skip invalid pixels
              if (digi_view[i].moduleId() == ::gpuClustering::invalidModuleId)
                return;
              PixelStatus::promote(acc, status, digi_view[i].xx(), digi_view[i].yy());
            });
            alpaka::syncBlockThreads(acc);
            // for (uint32_t i = firstElementIdx; i < msize - 1; ++i) {
            cms::alpakatools::for_each_element_in_block_strided(acc, msize - 1, firstElementIdx, [&](uint32_t i) {
              // skip invalid pixels
              if (digi_view[i].moduleId() == ::gpuClustering::invalidModuleId)
                return;
              if (PixelStatus::isDuplicate(status, digi_view[i].xx(), digi_view[i].yy())) {
                // printf("found dup %d %d %d %d\n", i, digi_view[i].moduleId(), digi_view[i].xx(), digi_view[i].yy());
                digi_view[i].moduleId() = ::gpuClustering::invalidModuleId;
                digi_view[i].rawIdArr() = 0;
              }
            });
            alpaka::syncBlockThreads(acc);
          }
        }

        // fill histo
        cms::alpakatools::for_each_element_in_block_strided(acc, msize, firstPixel, [&](uint32_t i) {
          if (digi_view[i].moduleId() != ::gpuClustering::invalidModuleId) {  // skip invalid pixels
            hist.count(acc, digi_view[i].yy());
#ifdef GPU_DEBUG
            alpaka::atomicAdd(acc, &totGood, 1u, alpaka::hierarchy::Blocks{});
#endif
          }
        });
        alpaka::syncBlockThreads(acc);
        cms::alpakatools::for_each_element_in_block(acc, 32u, [&](uint32_t i) {
          ws[i] = 0;  // used by prefix scan...
        });
        alpaka::syncBlockThreads(acc);
        hist.finalize(acc, ws);
        alpaka::syncBlockThreads(acc);
#ifdef GPU_DEBUG
        ALPAKA_ASSERT_OFFLOAD(hist.size() == totGood);
        if (thisModuleId % 100 == 1)
          if (threadIdxLocal == 0)
            printf("histo size %d\n", hist.size());
#endif
        cms::alpakatools::for_each_element_in_block_strided(acc, msize, firstPixel, [&](uint32_t i) {
          if (digi_view[i].moduleId() != ::gpuClustering::invalidModuleId) {  // skip invalid pixels
            hist.fill(acc, digi_view[i].yy(), i - firstPixel);
          }
        });

        // Assume that we can cover the whole module with up to 16 blockDimension-wide iterations
        // This maxiter value was tuned for GPU, with 256 or 512 threads per block.
        // Hence, also works for CPU case, with 256 or 512 elements per thread.
        // Real constrainst is maxiter = hist.size() / blockDimension,
        // with blockDimension = threadPerBlock * elementsPerThread.
        // Hence, maxiter can be tuned accordingly to the workdiv.
        constexpr unsigned int maxiter = 16;
        ALPAKA_ASSERT_OFFLOAD((hist.size() / blockDimension) <= maxiter);

#if defined(ALPAKA_ACC_GPU_CUDA_ASYNC_BACKEND) || defined(ALPAKA_ACC_GPU_HIP_ASYNC_BACKEND)
        constexpr uint32_t threadDimension = 1;
#else
        // NB: can be tuned.
        constexpr uint32_t threadDimension = 256;
#endif

#ifndef NDEBUG
        [[maybe_unused]] const uint32_t runTimeThreadDimension(
            alpaka::getWorkDiv<alpaka::Thread, alpaka::Elems>(acc)[0u]);
        ALPAKA_ASSERT_OFFLOAD(runTimeThreadDimension <= threadDimension);
#endif

        // nearest neighbour
        // allocate space for duplicate pixels: a pixel can appear more than once with different charge in the same event
        constexpr int maxNeighbours = 10;
        uint16_t nn[maxiter][threadDimension][maxNeighbours];
        uint8_t nnn[maxiter][threadDimension];  // number of nn
        for (uint32_t elementIdx = 0; elementIdx < threadDimension; ++elementIdx) {
          for (uint32_t k = 0; k < maxiter; ++k) {
            nnn[k][elementIdx] = 0;
          }
        }

        alpaka::syncBlockThreads(acc);  // for hit filling!

#ifdef GPU_DEBUG
        // look for anomalous high occupancy
        auto& n40 = alpaka::declareSharedVar<uint32_t, __COUNTER__>(acc);
        auto& n60 = alpaka::declareSharedVar<uint32_t, __COUNTER__>(acc);
        n40 = n60 = 0;
        alpaka::syncBlockThreads(acc);
        cms::alpakatools::for_each_element_in_block_strided(acc, Hist::nbins(), [&](uint32_t j) {
          if (hist.size(j) > 60)
            alpaka::atomicAdd(acc, &n60, 1u, alpaka::hierarchy::Blocks{});
          if (hist.size(j) > 40)
            alpaka::atomicAdd(acc, &n40, 1u, alpaka::hierarchy::Blocks{});
        });
        alpaka::syncBlockThreads(acc);
        if (0 == threadIdxLocal) {
          if (n60 > 0)
            printf("columns with more than 60 px %d in %d\n", n60, thisModuleId);
          else if (n40 > 0)
            printf("columns with more than 40 px %d in %d\n", n40, thisModuleId);
        }
        alpaka::syncBlockThreads(acc);
#endif

        // fill NN
        uint32_t k = 0u;
        cms::alpakatools::for_each_element_in_block_strided(acc, hist.size(), [&](uint32_t j) {
          const uint32_t jEquivalentClass = j % threadDimension;
          k = j / blockDimension;
          ALPAKA_ASSERT_OFFLOAD(k < maxiter);
          auto p = hist.begin() + j;
          auto i = *p + firstPixel;
          ALPAKA_ASSERT_OFFLOAD(digi_view[i].moduleId() != ::gpuClustering::invalidModuleId);
          ALPAKA_ASSERT_OFFLOAD(digi_view[i].moduleId() == thisModuleId);  // same module
          int be = Hist::bin(digi_view[i].yy() + 1);
          auto e = hist.end(be);
          ++p;
          ALPAKA_ASSERT_OFFLOAD(0 == nnn[k][jEquivalentClass]);
          for (; p < e; ++p) {
            auto m = (*p) + firstPixel;
            ALPAKA_ASSERT_OFFLOAD(m != i);
            ALPAKA_ASSERT_OFFLOAD(int(digi_view[m].yy()) - int(digi_view[i].yy()) >= 0);
            ALPAKA_ASSERT_OFFLOAD(int(digi_view[m].yy()) - int(digi_view[i].yy()) <= 1);
            if (std::abs(int(digi_view[m].xx()) - int(digi_view[i].xx())) <= 1) {
              auto l = nnn[k][jEquivalentClass]++;
              ALPAKA_ASSERT_OFFLOAD(l < maxNeighbours);
              nn[k][jEquivalentClass][l] = *p;
            }
          }
        });

        // for each pixel, look at all the pixels until the end of the module;
        // when two valid pixels within +/- 1 in x or y are found, set their id to the minimum;
        // after the loop, all the pixel in each cluster should have the id equeal to the lowest
        // pixel in the cluster ( clus[i] == i ).
        bool more = true;
        int nloops = 0;
        while (alpaka::syncBlockThreadsPredicate<alpaka::BlockOr>(acc, more)) {
          if (1 == nloops % 2) {
            cms::alpakatools::for_each_element_in_block_strided(acc, hist.size(), [&](uint32_t j) {
              auto p = hist.begin() + j;
              auto i = *p + firstPixel;
              auto m = digi_view[i].clus();
              while (m != digi_view[m].clus())
                m = digi_view[m].clus();
              digi_view[i].clus() = m;
            });
          } else {
            more = false;
            uint32_t k = 0u;
            cms::alpakatools::for_each_element_in_block_strided(acc, hist.size(), [&](uint32_t j) {
              k = j / blockDimension;
              const uint32_t jEquivalentClass = j % threadDimension;
              auto p = hist.begin() + j;
              auto i = *p + firstPixel;
              for (int kk = 0; kk < nnn[k][jEquivalentClass]; ++kk) {
                auto l = nn[k][jEquivalentClass][kk];
                auto m = l + firstPixel;
                ALPAKA_ASSERT_OFFLOAD(m != i);
                auto old =
                    alpaka::atomicMin(acc, &digi_view[m].clus(), digi_view[i].clus(), alpaka::hierarchy::Blocks{});
                if (old != digi_view[i].clus()) {
                  // end the loop only if no changes were applied
                  more = true;
                }
                alpaka::atomicMin(acc, &digi_view[i].clus(), old, alpaka::hierarchy::Blocks{});
              }  // nnloop
            });  // pixel loop
          }
          ++nloops;
        }  // end while

#ifdef GPU_DEBUG
        {
          auto& n0 = alpaka::declareSharedVar<int, __COUNTER__>(acc);
          if (threadIdxLocal == 0)
            n0 = nloops;
          alpaka::syncBlockThreads(acc);
#ifndef NDEBUG
          [[maybe_unused]] auto ok = n0 == nloops;
          ALPAKA_ASSERT_OFFLOAD(alpaka::syncBlockThreadsPredicate<alpaka::BlockAnd>(acc, ok));
#endif
          if (thisModuleId % 100 == 1)
            if (threadIdxLocal == 0)
              printf("# loops %d\n", nloops);
        }
#endif

        auto& foundClusters = alpaka::declareSharedVar<unsigned int, __COUNTER__>(acc);
        foundClusters = 0;
        alpaka::syncBlockThreads(acc);

        // find the number of different clusters, identified by a pixels with clus[i] == i;
        // mark these pixels with a negative id.
        cms::alpakatools::for_each_element_in_block_strided(acc, msize, firstPixel, [&](uint32_t i) {
          if (digi_view[i].moduleId() != ::gpuClustering::invalidModuleId) {  // skip invalid pixels
            if (digi_view[i].clus() == static_cast<int>(i)) {
              auto old = alpaka::atomicInc(acc, &foundClusters, 0xffffffff, alpaka::hierarchy::Threads{});
              digi_view[i].clus() = -(old + 1);
            }
          }
        });
        alpaka::syncBlockThreads(acc);

        // propagate the negative id to all the pixels in the cluster.
        cms::alpakatools::for_each_element_in_block_strided(acc, msize, firstPixel, [&](uint32_t i) {
          if (digi_view[i].moduleId() != ::gpuClustering::invalidModuleId) {  // skip invalid pixels
            if (digi_view[i].clus() >= 0) {
              // mark each pixel in a cluster with the same id as the first one
              digi_view[i].clus() = digi_view[digi_view[i].clus()].clus();
            }
          }
        });
        alpaka::syncBlockThreads(acc);

        // adjust the cluster id to be a positive value starting from 0
        cms::alpakatools::for_each_element_in_block_strided(acc, msize, firstPixel, [&](uint32_t i) {
          if (digi_view[i].moduleId() == ::gpuClustering::invalidModuleId) {  // skip invalid pixels
            digi_view[i].clus() = -9999;
          } else {
            digi_view[i].clus() = -digi_view[i].clus() - 1;
          }
        });
        alpaka::syncBlockThreads(acc);

        if (threadIdxLocal == 0) {
          clus_view[thisModuleId].clusInModule() = foundClusters;
          clus_view[blockIdx].moduleId() = thisModuleId;
#ifdef GPU_DEBUG
          if (foundClusters > gMaxHit<TAcc>) {
            gMaxHit<TAcc> = foundClusters;
            if (foundClusters > 8)
              printf("max hit %d in %d\n", foundClusters, thisModuleId);
          }
#endif
#ifdef GPU_DEBUG
          if (thisModuleId % 100 == 1)
            printf("%d clusters in module %d\n", foundClusters, thisModuleId);
#endif
        }
      }
    };

  }  // namespace gpuClustering
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE
#endif  // plugin_SiPixelClusterizer_alpaka_gpuClustering_h
