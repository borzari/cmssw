
#ifndef DataFormats_SiPixelMappingSoA_interface_SiPixelMappingUtilities_h
#define DataFormats_SiPixelMappingSoA_interface_SiPixelMappingUtilities_h


#include <cstdint>
#include <alpaka/alpaka.hpp>
#include "DataFormats/Portable/interface/alpaka/PortableCollection.h"
#include "DataFormats/SiPixelMappingSoA/interface/SiPixelMappingLayout.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelFedCablingMap.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelFedCablingTree.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {

    struct SiPixelMappingUtilities
    { 
        ALPAKA_FN_HOST_ACC ALPAKA_FN_ACC ALPAKA_FN_INLINE static bool hasQuality(const SiPixelMappingLayoutSoAConstView &view)
        {
          return view.hasQuality();
        }
        
        ALPAKA_FN_HOST_ACC ALPAKA_FN_ACC ALPAKA_FN_INLINE static cms::alpakatools::device_buffer<Device, unsigned char[]> getModToUnpRegionalAsync(
            std::set<unsigned int> const& modules,
            const SiPixelFedCablingMap* cablingMap,
            Queue &queue)
        {
            auto modToUnpDevice = cms::alpakatools::make_device_buffer<unsigned char[]>(queue, pixelgpudetails::MAX_SIZE);
            auto modToUnpHost = cms::alpakatools::make_device_buffer<unsigned char[]>(queue, pixelgpudetails::MAX_SIZE);

            std::vector<unsigned int> const& fedIds = cablingMap->fedIds();
            std::unique_ptr<SiPixelFedCablingTree> const& cabling = cablingMap->cablingTree();

            unsigned int startFed = *(fedIds.begin());
            unsigned int endFed = *(fedIds.end() - 1);

            sipixelobjects::CablingPathToDetUnit path;
            int index = 1;

            for (unsigned int fed = startFed; fed <= endFed; fed++) {
              for (unsigned int link = 1; link <= pixelgpudetails::MAX_LINK; link++) {
                for (unsigned int roc = 1; roc <= pixelgpudetails::MAX_ROC; roc++) {
                  path = {fed, link, roc};
                  const sipixelobjects::PixelROC* pixelRoc = cabling->findItem(path);
                  if (pixelRoc != nullptr) {
                    modToUnpHost[index] = (not modules.empty()) and (modules.find(pixelRoc->rawId()) == modules.end());
                  } else {  // store some dummy number
                    modToUnpHost[index] = true;
                  }
                  index++;
                }
              }
            }

            alpaka::memcpy(queue, modToUnpDevice, modToUnpHost);

            return modToUnpDevice;
        }

 };
}
#endif