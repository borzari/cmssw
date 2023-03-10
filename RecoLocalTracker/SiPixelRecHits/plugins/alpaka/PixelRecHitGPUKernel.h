#ifndef RecoLocalTracker_SiPixelRecHits_plugins_alpaka_PixelRecHitGPUKernel_h
#define RecoLocalTracker_SiPixelRecHits_plugins_alpaka_PixelRecHitGPUKernel_h

#include <cstdint>

#include <alpaka/alpaka.hpp>

#include "DataFormats/BeamSpotAlpaka/interface/alpaka/BeamSpotAlpaka.h"
#include "DataFormats/SiPixelClusterSoA/interface/alpaka/SiPixelClustersDevice.h"
#include "DataFormats/SiPixelDigiSoA/interface/alpaka/SiPixelDigisDevice.h"
#include "DataFormats/TrackingRecHitSoA/interface/alpaka/TrackingRecHitSoADevice.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "DataFormats/TrackerCommon/interface/SimplePixelTopology.h"
#include "RecoLocalTracker/SiPixelRecHits/interface/pixelCPEforDevice.h"

namespace ALPAKA_ACCELERATOR_NAMESPACE {
  namespace pixelgpudetails {
    using namespace cms::alpakatools;

    template <typename TrackerTraits>
    class PixelRecHitGPUKernel {
    public:
      PixelRecHitGPUKernel() = default;
      ~PixelRecHitGPUKernel() = default;

      PixelRecHitGPUKernel(const PixelRecHitGPUKernel&) = delete;
      PixelRecHitGPUKernel(PixelRecHitGPUKernel&&) = delete;
      PixelRecHitGPUKernel& operator=(const PixelRecHitGPUKernel&) = delete;
      PixelRecHitGPUKernel& operator=(PixelRecHitGPUKernel&&) = delete;

      using ParamsOnGPU = pixelCPEforDevice::ParamsOnDeviceT<TrackerTraits>;

      TrackingRecHitAlpakaDevice<TrackerTraits> makeHitsAsync(SiPixelDigisDevice const& digis_d,
                                                              SiPixelClustersDevice const& clusters_d,
                                                              BeamSpotAlpaka const& bs_d,
                                                              ParamsOnGPU const* cpeParams,
                                                              Queue queue) const;
    };
  }  // namespace pixelgpudetails
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

#endif  // RecoLocalTracker_SiPixelRecHits_plugins_alpaka_PixelRecHitGPUKernel_h
