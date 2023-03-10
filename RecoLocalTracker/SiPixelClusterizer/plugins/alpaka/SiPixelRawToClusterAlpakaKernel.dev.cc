/* Sushil Dubey, Shashi Dugad, TIFR, July 2017
 *
 * File Name: RawToClusterGPU.cu
 * Description: It converts Raw data into Digi Format on GPU
 * Finaly the Output of RawToDigi data is given to pixelClusterizer
 *
**/

// C++ includes
#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

// CMSSW includes
#include "HeterogeneousCore/AlpakaUtilities/interface/prefixScan.h"
#include "HeterogeneousCore/AlpakaUtilities/interface/SimpleVector.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "HeterogeneousCore/AlpakaInterface/interface/memory.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "HeterogeneousCore/AlpakaInterface/interface/workdivision.h"

#include "DataFormats/SiPixelGainCalibrationForHLTSoA/interface/SiPixelGainCalibrationForHLTLayout.h"
#include "DataFormats/SiPixelMappingSoA/interface/SiPixelMappingLayout.h"
#include "DataFormats/SiPixelDigi/interface/SiPixelDigiConstants.h"

// local includes
#include "gpuClusterChargeCut.h"
#include "SiPixelRawToClusterAlpakaKernel.h"
#include "gpuCalibPixel.h"
#include "gpuClustering.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {
  namespace pixelgpudetails {

    SiPixelRawToClusterGPUKernel::WordFedAppender::WordFedAppender(uint32_t words)
        : word_{cms::alpakatools::make_host_buffer<unsigned int[], Platform>(words)},
          fedId_{cms::alpakatools::make_host_buffer<unsigned char[], Platform>(words)} {}

    void SiPixelRawToClusterGPUKernel::WordFedAppender::initializeWordFed(int fedId,
                                                                          unsigned int wordCounterGPU,
                                                                          const uint32_t *src,
                                                                          unsigned int length) {
      std::memcpy(word_.data() + wordCounterGPU, src, sizeof(uint32_t) * length);
      std::memset(fedId_.data() + wordCounterGPU / 2, fedId - 1200, length / 2);
    }

    ////////////////////

    ALPAKA_FN_ACC uint32_t getLink(uint32_t ww) {
      return ((ww >> ::sipixelconstants::LINK_shift) & ::sipixelconstants::LINK_mask);
    }

    ALPAKA_FN_ACC uint32_t getRoc(uint32_t ww) {
      return ((ww >> ::sipixelconstants::ROC_shift) & ::sipixelconstants::ROC_mask);
    }

    ALPAKA_FN_ACC uint32_t getADC(uint32_t ww) {
      return ((ww >> ::sipixelconstants::ADC_shift) & ::sipixelconstants::ADC_mask);
    }

    ALPAKA_FN_ACC bool isBarrel(uint32_t rawId) { return (1 == ((rawId >> 25) & 0x7)); }

    ALPAKA_FN_ACC ::pixelgpudetails::DetIdGPU getRawId(const SiPixelMappingLayoutSoAConstView &cablingMap,
                                                       uint8_t fed,
                                                       uint32_t link,
                                                       uint32_t roc) {
      using namespace ::pixelgpudetails;
      uint32_t index = fed * MAX_LINK * MAX_ROC + (link - 1) * MAX_ROC + roc;
      ::pixelgpudetails::DetIdGPU detId = {
          cablingMap.rawId()[index], cablingMap.rocInDet()[index], cablingMap.moduleId()[index]};
      return detId;
    }

    //reference http://cmsdoxygen.web.cern.ch/cmsdoxygen/CMSSW_9_2_0/doc/html/dd/d31/FrameConversion_8cc_source.html
    //http://cmslxr.fnal.gov/source/CondFormats/SiPixelObjects/src/PixelROC.cc?v=CMSSW_9_2_0#0071
    // Convert local pixel to pixelgpudetails::global pixel
    ALPAKA_FN_ACC ::pixelgpudetails::Pixel frameConversion(
        bool bpix, int side, uint32_t layer, uint32_t rocIdInDetUnit, ::pixelgpudetails::Pixel local) {
      int slopeRow = 0, slopeCol = 0;
      int rowOffset = 0, colOffset = 0;

      if (bpix) {
        if (side == -1 && layer != 1) {  // -Z side: 4 non-flipped modules oriented like 'dddd', except Layer 1
          if (rocIdInDetUnit < 8) {
            slopeRow = 1;
            slopeCol = -1;
            rowOffset = 0;
            colOffset = (8 - rocIdInDetUnit) * ::pixelgpudetails::numColsInRoc - 1;
          } else {
            slopeRow = -1;
            slopeCol = 1;
            rowOffset = 2 * ::pixelgpudetails::numRowsInRoc - 1;
            colOffset = (rocIdInDetUnit - 8) * ::pixelgpudetails::numColsInRoc;
          }       // if roc
        } else {  // +Z side: 4 non-flipped modules oriented like 'pppp', but all 8 in layer1
          if (rocIdInDetUnit < 8) {
            slopeRow = -1;
            slopeCol = 1;
            rowOffset = 2 * ::pixelgpudetails::numRowsInRoc - 1;
            colOffset = rocIdInDetUnit * ::pixelgpudetails::numColsInRoc;
          } else {
            slopeRow = 1;
            slopeCol = -1;
            rowOffset = 0;
            colOffset = (16 - rocIdInDetUnit) * ::pixelgpudetails::numColsInRoc - 1;
          }
        }

      } else {             // fpix
        if (side == -1) {  // pannel 1
          if (rocIdInDetUnit < 8) {
            slopeRow = 1;
            slopeCol = -1;
            rowOffset = 0;
            colOffset = (8 - rocIdInDetUnit) * ::pixelgpudetails::numColsInRoc - 1;
          } else {
            slopeRow = -1;
            slopeCol = 1;
            rowOffset = 2 * ::pixelgpudetails::numRowsInRoc - 1;
            colOffset = (rocIdInDetUnit - 8) * ::pixelgpudetails::numColsInRoc;
          }
        } else {  // pannel 2
          if (rocIdInDetUnit < 8) {
            slopeRow = 1;
            slopeCol = -1;
            rowOffset = 0;
            colOffset = (8 - rocIdInDetUnit) * ::pixelgpudetails::numColsInRoc - 1;
          } else {
            slopeRow = -1;
            slopeCol = 1;
            rowOffset = 2 * ::pixelgpudetails::numRowsInRoc - 1;
            colOffset = (rocIdInDetUnit - 8) * ::pixelgpudetails::numColsInRoc;
          }

        }  // side
      }

      uint32_t gRow = rowOffset + slopeRow * local.row;
      uint32_t gCol = colOffset + slopeCol * local.col;
      //printf("Inside frameConversion row: %u, column: %u\n", gRow, gCol);
      ::pixelgpudetails::Pixel global = {gRow, gCol};
      return global;
    }

    ALPAKA_FN_ACC uint8_t conversionError(uint8_t fedId, uint8_t status, bool debug = false) {
      uint8_t errorType = 0;

      // debug = true;

      switch (status) {
        case (1): {
          if (debug)
            printf("Error in Fed: %i, invalid channel Id (errorType = 35\n)", fedId);
          errorType = 35;
          break;
        }
        case (2): {
          if (debug)
            printf("Error in Fed: %i, invalid ROC Id (errorType = 36)\n", fedId);
          errorType = 36;
          break;
        }
        case (3): {
          if (debug)
            printf("Error in Fed: %i, invalid dcol/pixel value (errorType = 37)\n", fedId);
          errorType = 37;
          break;
        }
        case (4): {
          if (debug)
            printf("Error in Fed: %i, dcol/pixel read out of order (errorType = 38)\n", fedId);
          errorType = 38;
          break;
        }
        default:
          if (debug)
            printf("Cabling check returned unexpected result, status = %i\n", status);
      };

      return errorType;
    }

    ALPAKA_FN_ACC bool rocRowColIsValid(uint32_t rocRow, uint32_t rocCol) {
      uint32_t numRowsInRoc = 80;
      uint32_t numColsInRoc = 52;

      /// row and collumn in ROC representation
      return ((rocRow < numRowsInRoc) & (rocCol < numColsInRoc));
    }

    ALPAKA_FN_ACC bool dcolIsValid(uint32_t dcol, uint32_t pxid) { return ((dcol < 26) & (2 <= pxid) & (pxid < 162)); }

    ALPAKA_FN_ACC uint8_t checkROC(uint32_t errorWord,
                                   uint8_t fedId,
                                   uint32_t link,
                                   const SiPixelMappingLayoutSoAConstView &cablingMap,
                                   bool debug = false) {
      uint8_t errorType = (errorWord >> ::pixelgpudetails::ROC_shift) & ::pixelgpudetails::ERROR_mask;
      if (errorType < 25)
        return 0;
      bool errorFound = false;

      switch (errorType) {
        case (25): {
          errorFound = true;
          uint32_t index = fedId * ::pixelgpudetails::MAX_LINK * ::pixelgpudetails::MAX_ROC +
                           (link - 1) * ::pixelgpudetails::MAX_ROC + 1;
          if (index > 1 && index <= cablingMap.size()) {
            if (!(link == cablingMap.link()[index] && 1 == cablingMap.roc()[index]))
              errorFound = false;
          }
          if (debug and errorFound)
            printf("Invalid ROC = 25 found (errorType = 25)\n");
          break;
        }
        case (26): {
          if (debug)
            printf("Gap word found (errorType = 26)\n");
          errorFound = true;
          break;
        }
        case (27): {
          if (debug)
            printf("Dummy word found (errorType = 27)\n");
          errorFound = true;
          break;
        }
        case (28): {
          if (debug)
            printf("Error fifo nearly full (errorType = 28)\n");
          errorFound = true;
          break;
        }
        case (29): {
          if (debug)
            printf("Timeout on a channel (errorType = 29)\n");
          if ((errorWord >> ::pixelgpudetails::OMIT_ERR_shift) & ::pixelgpudetails::OMIT_ERR_mask) {
            if (debug)
              printf("...first errorType=29 error, this gets masked out\n");
          }
          errorFound = true;
          break;
        }
        case (30): {
          if (debug)
            printf("TBM error trailer (errorType = 30)\n");
          int StateMatch_bits = 4;
          int StateMatch_shift = 8;
          uint32_t StateMatch_mask = ~(~uint32_t(0) << StateMatch_bits);
          int StateMatch = (errorWord >> StateMatch_shift) & StateMatch_mask;
          if (StateMatch != 1 && StateMatch != 8) {
            if (debug)
              printf("FED error 30 with unexpected State Bits (errorType = 30)\n");
          }
          if (StateMatch == 1)
            errorType = 40;  // 1=Overflow -> 40, 8=number of ROCs -> 30
          errorFound = true;
          break;
        }
        case (31): {
          if (debug)
            printf("Event number error (errorType = 31)\n");
          errorFound = true;
          break;
        }
        default:
          errorFound = false;
      };

      return errorFound ? errorType : 0;
    }

    ALPAKA_FN_ACC uint32_t getErrRawID(uint8_t fedId,
                                       uint32_t errWord,
                                       uint32_t errorType,
                                       const SiPixelMappingLayoutSoAConstView &cablingMap,
                                       bool debug = false) {
      uint32_t rID = 0xffffffff;

      switch (errorType) {
        case 25:
        case 30:
        case 31:
        case 36:
        case 40: {
          //set dummy values for cabling just to get detId from link
          //cabling.dcol = 0;
          //cabling.pxid = 2;
          uint32_t roc = 1;
          uint32_t link = (errWord >> ::pixelgpudetails::LINK_shift) & ::pixelgpudetails::LINK_mask;
          uint32_t rID_temp = getRawId(cablingMap, fedId, link, roc).RawId;
          if (rID_temp != 9999)
            rID = rID_temp;
          break;
        }
        case 29: {
          int chanNmbr = 0;
          const int DB0_shift = 0;
          const int DB1_shift = DB0_shift + 1;
          const int DB2_shift = DB1_shift + 1;
          const int DB3_shift = DB2_shift + 1;
          const int DB4_shift = DB3_shift + 1;
          const uint32_t DataBit_mask = ~(~uint32_t(0) << 1);

          int CH1 = (errWord >> DB0_shift) & DataBit_mask;
          int CH2 = (errWord >> DB1_shift) & DataBit_mask;
          int CH3 = (errWord >> DB2_shift) & DataBit_mask;
          int CH4 = (errWord >> DB3_shift) & DataBit_mask;
          int CH5 = (errWord >> DB4_shift) & DataBit_mask;
          int BLOCK_bits = 3;
          int BLOCK_shift = 8;
          uint32_t BLOCK_mask = ~(~uint32_t(0) << BLOCK_bits);
          int BLOCK = (errWord >> BLOCK_shift) & BLOCK_mask;
          int localCH = 1 * CH1 + 2 * CH2 + 3 * CH3 + 4 * CH4 + 5 * CH5;
          if (BLOCK % 2 == 0)
            chanNmbr = (BLOCK / 2) * 9 + localCH;
          else
            chanNmbr = ((BLOCK - 1) / 2) * 9 + 4 + localCH;
          if ((chanNmbr < 1) || (chanNmbr > 36))
            break;  // signifies unexpected result

          // set dummy values for cabling just to get detId from link if in Barrel
          //cabling.dcol = 0;
          //cabling.pxid = 2;
          uint32_t roc = 1;
          uint32_t link = chanNmbr;
          uint32_t rID_temp = getRawId(cablingMap, fedId, link, roc).RawId;
          if (rID_temp != 9999)
            rID = rID_temp;
          break;
        }
        case 37:
        case 38: {
          //cabling.dcol = 0;
          //cabling.pxid = 2;
          uint32_t roc = (errWord >> ::pixelgpudetails::ROC_shift) & ::pixelgpudetails::ROC_mask;
          uint32_t link = (errWord >> ::pixelgpudetails::LINK_shift) & ::pixelgpudetails::LINK_mask;
          uint32_t rID_temp = getRawId(cablingMap, fedId, link, roc).RawId;
          if (rID_temp != 9999)
            rID = rID_temp;
          break;
        }
        default:
          break;
      };

      return rID;
    }

    // Kernel to perform Raw to Digi conversion
    struct RawToDigi_kernel {
      template <typename TAcc>
      ALPAKA_FN_ACC void operator()(const TAcc &acc,
                                    // const SiPixelFedCablingMapGPU *cablingMap,
                                    const SiPixelMappingLayoutSoAConstView &cablingMap,
                                    const unsigned char *modToUnp,
                                    const uint32_t wordCounter,
                                    const uint32_t *word,
                                    const uint8_t *fedIds,
                                    SiPixelDigisLayoutSoAView digisView,
                                    // uint16_t *xx,
                                    // uint16_t *yy,
                                    // uint16_t *adc,
                                    // uint32_t *pdigi,
                                    // uint32_t *rawIdArr,
                                    // uint16_t *moduleId,
                                    cms::alpakatools::SimpleVector<SiPixelErrorCompact> *err,
                                    bool useQualityInfo,
                                    bool includeErrors,
                                    bool debug) const {
        cms::alpakatools::for_each_element_in_grid_strided(acc, wordCounter, [&](uint32_t iloop) {
          auto gIndex = iloop;
          auto dvgi = digisView[gIndex];
          dvgi.xx() = 0;
          dvgi.yy() = 0;
          dvgi.adc() = 0;
          bool skipROC = false;

          uint8_t fedId = fedIds[gIndex / 2];  // +1200;

          // initialize (too many coninue below)
          dvgi.pdigi() = 0;
          dvgi.rawIdArr() = 0;
          dvgi.moduleId() = 9999;

          uint32_t ww = word[gIndex];  // Array containing 32 bit raw data
          if (ww == 0) {
            // 0 is an indicator of a noise/dead channel, skip these pixels during clusterization
            return;
          }

          uint32_t link = getLink(ww);  // Extract link
          uint32_t roc = getRoc(ww);    // Extract Roc in link
          ::pixelgpudetails::DetIdGPU detId = getRawId(cablingMap, fedId, link, roc);

          uint8_t errorType = checkROC(ww, fedId, link, cablingMap, debug);
          skipROC = (roc < ::pixelgpudetails::maxROCIndex) ? false : (errorType != 0);
          if (includeErrors and skipROC) {
            uint32_t rID = getErrRawID(fedId, ww, errorType, cablingMap, debug);
            err->push_back(acc, SiPixelErrorCompact{rID, ww, errorType, fedId});
            return;
          }

          uint32_t rawId = detId.RawId;
          uint32_t rocIdInDetUnit = detId.rocInDet;
          bool barrel = isBarrel(rawId);

          uint32_t index = fedId * ::pixelgpudetails::MAX_LINK * ::pixelgpudetails::MAX_ROC +
                           (link - 1) * ::pixelgpudetails::MAX_ROC + roc;
          if (useQualityInfo) {
            skipROC = cablingMap.badRocs()[index];
            if (skipROC)
              return;
          }
          skipROC = modToUnp[index];
          if (skipROC)
            return;

          uint32_t layer = 0;                   //, ladder =0;
          int side = 0, panel = 0, module = 0;  //disk = 0, blade = 0

          if (barrel) {
            layer = (rawId >> ::pixelgpudetails::layerStartBit) & ::pixelgpudetails::layerMask;
            module = (rawId >> ::pixelgpudetails::moduleStartBit) & ::pixelgpudetails::moduleMask;
            side = (module < 5) ? -1 : 1;
          } else {
            // endcap ids
            layer = 0;
            panel = (rawId >> ::pixelgpudetails::panelStartBit) & ::pixelgpudetails::panelMask;
            //disk  = (rawId >> diskStartBit_) & diskMask_;
            side = (panel == 1) ? -1 : 1;
            //blade = (rawId >> bladeStartBit_) & bladeMask_;
          }

          // ***special case of layer to 1 be handled here
          ::pixelgpudetails::Pixel localPix;
          if (layer == 1) {
            uint32_t col = (ww >> ::pixelgpudetails::COL_shift) & ::pixelgpudetails::COL_mask;
            uint32_t row = (ww >> ::pixelgpudetails::ROW_shift) & ::pixelgpudetails::ROW_mask;
            localPix.row = row;
            localPix.col = col;
            if (includeErrors) {
              if (not rocRowColIsValid(row, col)) {
                uint8_t error = conversionError(fedId, 3, debug);  //use the device function and fill the arrays
                err->push_back(acc, SiPixelErrorCompact{rawId, ww, error, fedId});
                if (debug)
                  printf("BPIX1  Error status: %i\n", error);
                return;
              }
            }
          } else {
            // ***conversion rules for dcol and pxid
            uint32_t dcol = (ww >> ::pixelgpudetails::DCOL_shift) & ::pixelgpudetails::DCOL_mask;
            uint32_t pxid = (ww >> ::pixelgpudetails::PXID_shift) & ::pixelgpudetails::PXID_mask;
            uint32_t row = ::pixelgpudetails::numRowsInRoc - pxid / 2;
            uint32_t col = dcol * 2 + pxid % 2;
            localPix.row = row;
            localPix.col = col;
            if (includeErrors and not dcolIsValid(dcol, pxid)) {
              uint8_t error = conversionError(fedId, 3, debug);
              err->push_back(acc, SiPixelErrorCompact{rawId, ww, error, fedId});
              if (debug)
                printf("Error status: %i %d %d %d %d\n", error, dcol, pxid, fedId, roc);
              return;
            }
          }

          ::pixelgpudetails::Pixel globalPix = frameConversion(barrel, side, layer, rocIdInDetUnit, localPix);
          dvgi.xx() = globalPix.row;  // origin shifting by 1 0-159
          dvgi.yy() = globalPix.col;  // origin shifting by 1 0-415
          dvgi.adc() = getADC(ww);
          dvgi.pdigi() = pixelgpudetails::pack(globalPix.row, globalPix.col, dvgi.adc());
          dvgi.moduleId() = detId.moduleId;
          dvgi.rawIdArr() = rawId;
        });  // end of stride on grid

      }  // end of Raw to Digi kernel operator()
    };   // end of Raw to Digi struct

  }  // namespace pixelgpudetails
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

namespace pixelgpudetails {

  template <typename TrackerTraits>
  struct fillHitsModuleStart {
    template <typename TAcc>
    ALPAKA_FN_ACC void operator()(const TAcc &acc,
                                  // uint32_t const *__restrict__ cluStart,
                                  SiPixelClustersLayoutSoAView clus_view
                                  // uint32_t *__restrict__ moduleStart
    ) const {
      ALPAKA_ASSERT_OFFLOAD(TrackerTraits::numberOfModules < 2048);  // easy to extend at least till 32*1024

      constexpr int nMaxModules = TrackerTraits::numberOfModules;

#ifndef NDEBUG
      [[maybe_unused]] const uint32_t blockIdxLocal(alpaka::getIdx<alpaka::Grid, alpaka::Blocks>(acc)[0u]);
      ALPAKA_ASSERT_OFFLOAD(0 == blockIdxLocal);
      [[maybe_unused]] const uint32_t gridDimension(alpaka::getWorkDiv<alpaka::Grid, alpaka::Blocks>(acc)[0u]);
      ALPAKA_ASSERT_OFFLOAD(1 == gridDimension);
#endif

      // limit to MaxHitsInModule;
      cms::alpakatools::for_each_element_in_block_strided(acc, nMaxModules, [&](uint32_t i) {
        clus_view[i + 1].moduleStart() = std::min(gpuClustering::maxHitsInModule(), clus_view[i].clusInModule());
      });

      constexpr bool isPhase2 = std::is_base_of<pixelTopology::Phase2, TrackerTraits>::value;
      constexpr auto leftModules = isPhase2 ? 1024 : nMaxModules - 1024;

      auto &&ws = alpaka::declareSharedVar<uint32_t[32], __COUNTER__>(acc);
      cms::alpakatools::blockPrefixScan(acc, clus_view.moduleStart() + 1, clus_view.moduleStart() + 1, 1024, ws);
      cms::alpakatools::blockPrefixScan(
          acc, clus_view.moduleStart() + 1024 + 1, clus_view.moduleStart() + 1024 + 1, leftModules, ws);

      if constexpr (isPhase2) {
        cms::alpakatools::blockPrefixScan(
            acc, clus_view.moduleStart() + 2048 + 1, clus_view.moduleStart() + 2048 + 1, 1024, ws);
        cms::alpakatools::blockPrefixScan(
            acc, clus_view.moduleStart() + 3072 + 1, clus_view.moduleStart() + 3072 + 1, nMaxModules - 3072, ws);
      }

      constexpr auto lastModule = isPhase2 ? 2049u : nMaxModules + 1;
      cms::alpakatools::for_each_element_in_block_strided(
          acc, lastModule, 1025u, [&](uint32_t i) { clus_view[i].moduleStart() += clus_view[1024].moduleStart(); });
      alpaka::syncBlockThreads(acc);

      if constexpr (isPhase2) {
        cms::alpakatools::for_each_element_in_block_strided(
            acc, 3073u, 2049u, [&](uint32_t i) { clus_view[i].moduleStart() += clus_view[2048].moduleStart(); });
        alpaka::syncBlockThreads(acc);

        cms::alpakatools::for_each_element_in_block_strided(acc, nMaxModules + 1, 3073u, [&](uint32_t i) {
          clus_view[i].moduleStart() += clus_view[3072].moduleStart();
        });
        alpaka::syncBlockThreads(acc);
      }
#ifdef GPU_DEBUG
      ALPAKA_ASSERT_OFFLOAD(0 == clus_view.[0].moduleStart());
      auto c0 = std::min(gpuClustering::maxHitsInModule(), cluStart[0]);
      ALPAKA_ASSERT_OFFLOAD(c0 == clus_view.[1].moduleStart());
      ALPAKA_ASSERT_OFFLOAD(clus_view.[1024].moduleStart() >= clus_view.[1023].moduleStart());
      ALPAKA_ASSERT_OFFLOAD(clus_view.[1025].moduleStart() >= clus_view.[1024].moduleStart());
      ALPAKA_ASSERT_OFFLOAD(clus_view.[nMaxModules].moduleStart() >= clus_view.[1025].moduleStart());

      //for (int i = first, iend = nMaxModules + 1; i < iend; i += blockDim.x) {
      cms::alpakatools::for_each_element_in_block_strided(acc, nMaxModules + 1, [&](uint32_t i) {
        if (0 != i)
          ALPAKA_ASSERT_OFFLOAD(clus_view[i].moduleStart() >= clus_view[i - i].moduleStart());
        // [BPX1, BPX2, BPX3, BPX4,  FP1,  FP2,  FP3,  FN1,  FN2,  FN3, LAST_VALID]
        // [   0,   96,  320,  672, 1184, 1296, 1408, 1520, 1632, 1744,       1856]
        if (i == 96 || i == 1184 || i == 1744 || i == nMaxModules)
          printf("moduleStart %d %d\n", i, clus_view[i].moduleStart());
      });
#endif

      // avoid overflow
      constexpr auto MAX_HITS = 200000;  //gpuClustering::MaxNumClusters; //FIXME: in TrackerTraits
      cms::alpakatools::for_each_element_in_block_strided(acc, nMaxModules + 1, [&](uint32_t i) {
        if (clus_view[i].moduleStart() > MAX_HITS)
          clus_view[i].moduleStart() = MAX_HITS;
      });

    }  // end of fillHitsModuleStart kernel operator()
  };   // end of fillHitsModuleStart struct

}  // namespace pixelgpudetails

namespace ALPAKA_ACCELERATOR_NAMESPACE {
  namespace pixelgpudetails {

    // Interface to outside
    void SiPixelRawToClusterGPUKernel::makeClustersAsync(bool isRun2,
                                                         const SiPixelClusterThresholds clusterThresholds,
                                                         //  const SiPixelFedCablingMapGPU *cablingMap,
                                                         const SiPixelMappingLayoutSoAConstView &cablingMap,
                                                         const unsigned char *modToUnp,
                                                         //  const SiPixelGainForHLTonGPU *gains,
                                                         const SiPixelGainCalibrationForHLTSoAConstView &gains,
                                                         const WordFedAppender &wordFed,
                                                         SiPixelFormatterErrors &&errors,
                                                         const uint32_t wordCounter,
                                                         const uint32_t fedCounter,
                                                         bool useQualityInfo,
                                                         bool includeErrors,
                                                         bool debug,
                                                         Queue &queue) {
      nDigis = wordCounter;

#ifdef GPU_DEBUG
      std::cout << "decoding " << wordCounter << " digis. Max is " << MAX_FED_WORDS << std::endl;
#endif

      constexpr int numberOfModules = pixelTopology::Phase1::numberOfModules;
      digis_d = SiPixelDigisDevice(MAX_FED_WORDS, queue);
      if (includeErrors) {
        digiErrors_d = SiPixelDigiErrorsDevice(MAX_FED_WORDS, std::move(errors), queue);
      }
      clusters_d = SiPixelClustersDevice(numberOfModules, queue);

      if (wordCounter)  // protect in case of empty event....
      {
#if defined(ALPAKA_ACC_GPU_CUDA_ASYNC_BACKEND) || defined(ALPAKA_ACC_GPU_HIP_ASYNC_BACKEND)
        const int threadsPerBlockOrElementsPerThread = 512;
#else
        // NB: MPORTANT: This could be tuned to benefit from innermost loop.
        const int threadsPerBlockOrElementsPerThread = 32;
#endif
        // fill it all
        const uint32_t blocks = cms::alpakatools::divide_up_by(wordCounter, threadsPerBlockOrElementsPerThread);
        const auto workDiv = cms::alpakatools::make_workdiv<Acc1D>(blocks, threadsPerBlockOrElementsPerThread);

        ALPAKA_ASSERT_OFFLOAD(0 == wordCounter % 2);
        // wordCounter is the total no of words in each event to be trasfered on device
        auto word_d = cms::alpakatools::make_device_buffer<uint32_t[]>(queue, wordCounter);
        // NB: IMPORTANT: fedId_d: In legacy, wordCounter elements are allocated.
        // However, only the first half of elements end up eventually used:
        // hence, here, only wordCounter/2 elements are allocated.
        auto fedId_d = cms::alpakatools::make_device_buffer<uint8_t[]>(queue, wordCounter / 2);

        alpaka::memcpy(queue, word_d, wordFed.word(), wordCounter);
        alpaka::memcpy(queue, fedId_d, wordFed.fedId(), wordCounter / 2);

        // Launch rawToDigi kernel
        alpaka::enqueue(queue,
                        alpaka::createTaskKernel<Acc1D>(workDiv,
                                                        RawToDigi_kernel(),
                                                        cablingMap,
                                                        modToUnp,
                                                        wordCounter,
                                                        word_d.data(),
                                                        fedId_d.data(),
                                                        digis_d->view(),
                                                        // digis_d->xx(),
                                                        // digis_d->yy(),
                                                        // digis_d->adc(),
                                                        // digis_d->pdigi(),
                                                        // digis_d->rawIdArr(),
                                                        // digis_d->moduleInd(),
                                                        digiErrors_d->error(),
                                                        useQualityInfo,
                                                        includeErrors,
                                                        debug));

#ifdef GPU_DEBUG
        alpaka::wait(queue);
#endif

#ifdef TODO
        if (includeErrors) {
          digiErrors_d.copyErrorToHostAsync(stream);
        }
#endif
      }
      // End of Raw2Digi and passing data for clustering

      {
        // clusterizer ...
        using namespace gpuClustering;
#if defined(ALPAKA_ACC_GPU_CUDA_ASYNC_BACKEND) || defined(ALPAKA_ACC_GPU_HIP_ASYNC_BACKEND)
        const auto threadsPerBlockOrElementsPerThread = 256;
#else
        // NB: MPORTANT: This could be tuned to benefit from innermost loop.
        const auto threadsPerBlockOrElementsPerThread = 32;
#endif
        const auto blocks = cms::alpakatools::divide_up_by(std::max<int>(wordCounter, numberOfModules),
                                                           threadsPerBlockOrElementsPerThread);
        const auto workDiv = cms::alpakatools::make_workdiv<Acc1D>(blocks, threadsPerBlockOrElementsPerThread);

        alpaka::enqueue(queue,
                        alpaka::createTaskKernel<Acc1D>(workDiv,
                                                        gpuCalibPixel::calibDigis(),
                                                        isRun2,
                                                        digis_d->view(),
                                                        clusters_d->view(),
                                                        // digis_d->moduleInd(),
                                                        // digis_d->c_xx(),
                                                        // digis_d->c_yy(),
                                                        // digis_d->adc(),
                                                        gains,
                                                        wordCounter));

        // clusters_d->view().moduleStart(),
        // clusters_d->clusInModule(),
        // clusters_d->clusModuleStart()));
#ifdef GPU_DEBUG
        alpaka::wait(queue);
        std::cout << "CUDA countModules kernel launch with " << blocks << " blocks of "
                  << threadsPerBlockOrElementsPerThread << " threadsPerBlockOrElementsPerThread\n";
#endif

        alpaka::enqueue(
            queue,
            alpaka::createTaskKernel<Acc1D>(
                workDiv, countModules<pixelTopology::Phase1>(), digis_d->view(), clusters_d->view(), wordCounter));

        // .moduleInd(),
        // clusters_d->view().moduleStart(),
        // digis_d->view().clus(),
        // wordCounter));

        auto moduleStartFirstElement =
            cms::alpakatools::make_device_view(alpaka::getDev(queue), clusters_d->view().moduleStart(), 1u);
        alpaka::memcpy(queue, nModules_Clusters_h, moduleStartFirstElement);

        const auto workDivMaxNumModules = cms::alpakatools::make_workdiv<Acc1D>(numberOfModules, 256);
        // NB: With present findClus() / chargeCut() algorithm,
        // threadPerBlock (GPU) or elementsPerThread (CPU) = 256 show optimal performance.
        // Though, it does not have to be the same number for CPU/GPU cases.

#ifdef GPU_DEBUG
        std::cout << "CUDA findClus kernel launch with " << numberOfModules << " blocks of " << 256
                  << " threadsPerBlockOrElementsPerThread\n";
#endif

        alpaka::enqueue(queue,
                        alpaka::createTaskKernel<Acc1D>(workDivMaxNumModules,
                                                        findClus<pixelTopology::Phase1>(),
                                                        digis_d->view(),
                                                        clusters_d->view(),
                                                        wordCounter));

#ifdef GPU_DEBUG
        alpaka::wait(queue);
#endif

        // apply charge cut
        alpaka::enqueue(queue,
                        alpaka::createTaskKernel<Acc1D>(workDivMaxNumModules,
                                                        ::gpuClustering::clusterChargeCut<pixelTopology::Phase1>(),
                                                        digis_d->view(),
                                                        clusters_d->view(),
                                                        wordCounter));

        // count the module start indices already here (instead of
        // rechits) so that the number of clusters/hits can be made
        // available in the rechit producer without additional points of
        // synchronization/ExternalWork

        // MUST be ONE block
        const auto workDivOneBlock = cms::alpakatools::make_workdiv<Acc1D>(1u, 1024u);
        alpaka::enqueue(
            queue,
            alpaka::createTaskKernel<Acc1D>(
                workDivOneBlock, ::pixelgpudetails::fillHitsModuleStart<pixelTopology::Phase1>(), clusters_d->view()));
        // clusters_d->clusModuleStart()));

        // last element holds the number of all clusters
        const auto clusModuleStartLastElement = cms::alpakatools::make_device_view(
            alpaka::getDev(queue),
            const_cast<uint32_t const *>(clusters_d->view().clusModuleStart() + numberOfModules),
            1u);
        constexpr int startBPIX2 = pixelTopology::Phase1::layerStart[1];
        // element startBPIX2 hold the number of clusters until BPIX2
        const auto bpix2ClusterStart = cms::alpakatools::make_device_view(
            alpaka::getDev(queue), const_cast<uint32_t const *>(clusters_d->view().clusModuleStart() + startBPIX2), 1u);
        auto nModules_Clusters_h_1 = cms::alpakatools::make_host_view(nModules_Clusters_h.data() + 1, 1u);
        alpaka::memcpy(queue, nModules_Clusters_h_1, clusModuleStartLastElement);

        auto nModules_Clusters_h_2 = cms::alpakatools::make_host_view(nModules_Clusters_h.data() + 2, 1u);
        alpaka::memcpy(queue, nModules_Clusters_h_2, bpix2ClusterStart);

      }  // end clusterizer scope
    }
  }  // namespace pixelgpudetails
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE
