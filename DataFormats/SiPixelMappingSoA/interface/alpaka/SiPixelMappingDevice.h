#ifndef DataFormats_SiPixelMappingSoA_interface_SiPixelClustersDevice_h
#define DataFormats_SiPixelMappingSoA_interface_SiPixelClustersDevice_h

#include <cstdint>
#include <alpaka/alpaka.hpp>
#include "DataFormats/Portable/interface/alpaka/PortableCollection.h"
#include "DataFormats/SiPixelMappingSoA/interface/SiPixelMappingLayout.h"


namespace ALPAKA_ACCELERATOR_NAMESPACE {

  class SiPixelMappingDevice : public PortableCollection<SiPixelMappingLayout<>> {
  public:
    SiPixelMappingDevice() = default;
    ~SiPixelMappingDevice() = default;

    template <typename TQueue>
    explicit SiPixelMappingDevice(size_t maxModules, SiPixelFedCablingMap const& cablingMap, bool hasQuality, TQueue queue)
        : PortableDeviceCollection<SiPixelMappingLayout<>>(maxModules + 1, queue), hasQuality_(hasQuality), 
        cablingMap_(&cablingMap)
        {}

    SiPixelMappingDevice(SiPixelMappingDevice &&) = default;
    SiPixelMappingDevice &operator=(SiPixelMappingDevice &&) = default;

    bool hasQuality() const { return hasQuality_; }

    template <typename TQueue>
    cms::alpakatools::device_buffer<unsigned char[]> getModToUnpRegionalAsync(std::set<unsigned int> const& modules, TQueue queue) const {
      auto modToUnpDevice = cms::alpakatools::make_device_buffer<unsigned char[]>(queue, pixelgpudetails::MAX_SIZE);
      auto modToUnpHost = cms::alpakatools::make_host_buffer<unsigned char[]>(queue, pixelgpudetails::MAX_SIZE);

      std::vector<unsigned int> const& fedIds = cablingMap_->fedIds();
      std::unique_ptr<SiPixelFedCablingTree> const& cabling = cablingMap_->cablingTree();

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

   private:
    bool hasQuality_;
    const SiPixelFedCablingMap *cablingMap_; //this is the cabling map that is ALWAYS on Host
  };
}
#endif  // DataFormats_SiPixelMappingSoA_interface_SiPixelClustersDevice_h
