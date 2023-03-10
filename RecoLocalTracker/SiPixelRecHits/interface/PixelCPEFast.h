#ifndef RecoLocalTracker_SiPixelRecHits_PixelCPEFast_h
#define RecoLocalTracker_SiPixelRecHits_PixelCPEFast_h

#include <utility>

#include "CondFormats/SiPixelTransient/interface/SiPixelGenError.h"
#include "CondFormats/SiPixelTransient/interface/SiPixelTemplate.h"
#include "DataFormats/PixelCPEFastParams/interface/PixelCPEFastParams.h"
#include "HeterogeneousCore/CUDACore/interface/ESProduct.h"
#include "HeterogeneousCore/CUDAUtilities/interface/HostAllocator.h"
#include "RecoLocalTracker/SiPixelRecHits/interface/PixelCPEGenericBase.h"
// #include "RecoLocalTracker/SiPixelRecHits/interface/alpaka/pixelCPEforGPU.h"
// #include "RecoLocalTracker/SiPixelRecHits/interface/pixelCPEforDevice.h"
#include "DataFormats/PixelCPEFastParams/interface/PixelCPEFastParams.h"
#include "DataFormats/TrackerCommon/interface/SimplePixelTopology.h"

class MagneticField;
template <typename TrackerTraits>
class PixelCPEFast final : public PixelCPEGenericBase {
public:
  PixelCPEFast(edm::ParameterSet const &conf,
               const MagneticField *,
               const TrackerGeometry &,
               const TrackerTopology &,
               const SiPixelLorentzAngle *,
               const SiPixelGenErrorDBObject *,
               const SiPixelLorentzAngle *);

  ~PixelCPEFast() override = default;

  static void fillPSetDescription(edm::ParameterSetDescription &desc);

  // The return value can only be used safely in kernels launched on
  // the same cudaStream, or after cudaStreamSynchronize.
  using ParamsOnDevice = pixelCPEforDevice::ParamsOnDeviceT<TrackerTraits>;
  using LayerGeometry = pixelCPEforDevice::LayerGeometryT<TrackerTraits>;
  using AverageGeometry = pixelTopology::AverageGeometryT<TrackerTraits>;

  ParamsOnDevice const &getCPEFastParams() const { return cpeParams_; }

private:
  LocalPoint localPosition(DetParam const &theDetParam, ClusterParam &theClusterParam) const override;
  LocalError localError(DetParam const &theDetParam, ClusterParam &theClusterParam) const override;

  void errorFromTemplates(DetParam const &theDetParam, ClusterParamGeneric &theClusterParam, float qclus) const;

  //--- DB Error Parametrization object, new light templates
  std::vector<SiPixelGenErrorStore> thePixelGenError_;
  ParamsOnDevice cpeParams_;

  void initializeParams();
};

#endif  // RecoLocalTracker_SiPixelRecHits_PixelCPEFast_h
