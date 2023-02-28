#include "CondFormats/DataRecord/interface/SiPixelFedCablingMapRcd.h"
#include "CondFormats/DataRecord/interface/SiPixelQualityRcd.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelFedCablingMap.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelFedCablingTree.h"
#include "CondFormats/SiPixelObjects/interface/SiPixelQuality.h"
#include "DataFormats/SiPixelClusterSoA/interface/gpuClusteringConstants.h"
#include "DataFormats/SiPixelMappingSoA/interface/SiPixelMappingHost.h"
#include "DataFormats/SiPixelMappingSoA/interface/alpaka/SiPixelMappingDevice.h"
#include "FWCore/Framework/interface/ESTransientHandle.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/ESGetToken.h"

#include "Geometry/CommonDetUnit/interface/GeomDetType.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"

#include "HeterogeneousCore/AlpakaCore/interface/alpaka/ESProducer.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/ModuleFactory.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "HeterogeneousCore/AlpakaInterface/interface/memory.h"

#include "RecoTracker/Record/interface/CkfComponentsRecord.h"
#include "HeterogeneousCore/AlpakaInterface/interface/CopyToDevice.h"
#include "DataFormats/Portable/interface/alpaka/PortableCollection.h"

template struct cms::alpakatools::CopyToDevice<SiPixelMappingHost>;  //needed for the method to not be incomplete

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  using namespace cms::alpakatools;

  class SiPixelCablingSoAESProducer : public ESProducer {
  public:
    SiPixelCablingSoAESProducer(edm::ParameterSet const& iConfig)
        : useQuality_(iConfig.getParameter<bool>("UseQualityInfo")) {
      auto const& component = iConfig.getParameter<std::string>("ComponentName");
      auto cc = setWhatProduced(this, component);
      cablingMapToken_ = cc.consumes(edm::ESInputTag{"", iConfig.getParameter<std::string>("CablingMapLabel")});
      if (useQuality_) {
        qualityToken_ = cc.consumes();
      }
      geometryToken_ = cc.consumes();
    }

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
      edm::ParameterSetDescription desc;
      desc.add<std::string>("ComponentName", "");
      desc.add<std::string>("CablingMapLabel", "")->setComment("CablingMap label");
      desc.add<bool>("UseQualityInfo", false);
      descriptions.addWithDefaultLabel(desc);
    }

    std::unique_ptr<SiPixelMappingHost> produce(const CkfComponentsRecord& iRecord) {
      auto cablingMap = iRecord.getTransientHandle(cablingMapToken_);

      const SiPixelQuality* quality = nullptr;
      if (useQuality_) {
        auto qualityInfo = iRecord.getTransientHandle(qualityToken_);
        quality = qualityInfo.product();
      }

      bool hasQuality = quality != nullptr;
      auto geom = iRecord.getTransientHandle(geometryToken_);
      auto product = std::make_unique<SiPixelMappingHost>(
          pixelgpudetails::MAX_SIZE,
          cms::alpakatools::
              host());  //(pixelgpudetails::MAX_SIZE, *(cablingMap.product()), hasQuality, cms::alpakatools::host());

      std::vector<unsigned int> const& fedIds = cablingMap->fedIds();
      std::unique_ptr<SiPixelFedCablingTree> const& cabling = cablingMap->cablingTree();

      unsigned int startFed = *(fedIds.begin());
      unsigned int endFed = *(fedIds.end() - 1);

      sipixelobjects::CablingPathToDetUnit path;
      int index = 1;

      auto mapView = product->view();

      mapView.hasQuality() = hasQuality;

      for (unsigned int fed = startFed; fed <= endFed; fed++) {
        for (unsigned int link = 1; link <= pixelgpudetails::MAX_LINK; link++) {
          for (unsigned int roc = 1; roc <= pixelgpudetails::MAX_ROC; roc++) {
            path = {fed, link, roc};
            const sipixelobjects::PixelROC* pixelRoc = cabling->findItem(path);
            mapView[index].fed() = fed;
            mapView[index].link() = link;
            mapView[index].roc() = roc;
            if (pixelRoc != nullptr) {
              mapView[index].rawId() = pixelRoc->rawId();
              mapView[index].rocInDet() = pixelRoc->idInDetUnit();
              mapView[index].modToUnpDefault() = false;
              if (quality != nullptr)
                mapView[index].badRocs() = quality->IsRocBad(pixelRoc->rawId(), pixelRoc->idInDetUnit());
              else
                mapView[index].badRocs() = false;
            } else {  // store some dummy number
              mapView[index].rawId() = gpuClustering::invalidModuleId;
              mapView[index].rocInDet() = gpuClustering::invalidModuleId;
              mapView[index].badRocs() = true;
              mapView[index].modToUnpDefault() = true;
            }
            index++;
          }
        }
      }  // end of FED loop

      // Given FedId, Link and idinLnk; use the following formula
      // to get the rawId and idinDU
      // index = (FedID-1200) * MAX_LINK* MAX_ROC + (Link-1)* MAX_ROC + idinLnk;
      // where, MAX_LINK = 48, MAX_ROC = 8
      // FedID varies between 1200 to 1338 (In total 108 FED's)
      // Link varies between 1 to 48
      // idinLnk varies between 1 to 8

      auto trackerGeom = iRecord.getTransientHandle(geometryToken_);

      for (int i = 1; i < index; i++) {
        if (mapView[i].rawId() == gpuClustering::invalidModuleId) {
          mapView[i].moduleId() = gpuClustering::invalidModuleId;
        } else {
          /*
          std::cout << mapView[i].rawId()[i] << std::endl;
          */
          auto gdet = trackerGeom->idToDetUnit(mapView[i].rawId());
          if (!gdet) {
            LogDebug("SiPixelCablingSoAESProducer") << " Not found: " << mapView[i].rawId() << std::endl;
            continue;
          }
          mapView[i].moduleId() = gdet->index();
        }
        LogDebug("SiPixelCablingSoAESProducer")
            << "----------------------------------------------------------------------------" << std::endl;
        LogDebug("SiPixelCablingSoAESProducer") << i << std::setw(20) << mapView[i].fed() << std::setw(20)
                                                << mapView[i].link() << std::setw(20) << mapView[i].roc() << std::endl;
        LogDebug("SiPixelCablingSoAESProducer")
            << i << std::setw(20) << mapView[i].rawId() << std::setw(20) << mapView[i].rocInDet() << std::setw(20)
            << mapView[i].moduleId() << std::endl;
        LogDebug("SiPixelCablingSoAESProducer")
            << i << std::setw(20) << mapView[i].badRocs() << std::setw(20) << std::endl;
        LogDebug("SiPixelCablingSoAESProducer")
            << "----------------------------------------------------------------------------" << std::endl;
      }

      mapView.size() = index - 1;

      return product;
    }

  private:
    edm::ESGetToken<SiPixelFedCablingMap, SiPixelFedCablingMapRcd> cablingMapToken_;
    edm::ESGetToken<SiPixelQuality, SiPixelQualityRcd> qualityToken_;
    edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> geometryToken_;
    bool useQuality_;
  };
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

DEFINE_FWK_EVENTSETUP_ALPAKA_MODULE(SiPixelCablingSoAESProducer);
