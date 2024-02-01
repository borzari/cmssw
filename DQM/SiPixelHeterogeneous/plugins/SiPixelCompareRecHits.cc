#include "DQMServices/Core/interface/MonitorElement.h"
#include "DQMServices/Core/interface/DQMEDAnalyzer.h"
#include "DQMServices/Core/interface/DQMStore.h"
#include "DataFormats/Math/interface/approx_atan2.h"
#include "DataFormats/SiPixelDetId/interface/PixelSubdetector.h"
#include "DataFormats/TrackerCommon/interface/TrackerTopology.h"
#include "DataFormats/TrackingRecHitSoA/interface/TrackingRecHitsHost.h"
#include "DataFormats/TrackingRecHitSoA/interface/TrackingRecHitsSoA.h"
#include "CUDADataFormats/TrackingRecHit/interface/TrackingRecHitSoAHost.h"
#include "CUDADataFormats/TrackingRecHit/interface/TrackingRecHitsUtilities.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "Geometry/CommonDetUnit/interface/PixelGeomDetUnit.h"
#include "Geometry/CommonTopologies/interface/PixelTopology.h"
#include "Geometry/Records/interface/TrackerDigiGeometryRecord.h"
#include "Geometry/TrackerGeometryBuilder/interface/TrackerGeometry.h"

template <typename T>
class SiPixelCompareRecHits : public DQMEDAnalyzer {
public:
  using HitsAlpaka = TrackingRecHitHost<T>;
  using HitsCUDA = TrackingRecHitSoAHost<T>;

  explicit SiPixelCompareRecHits(const edm::ParameterSet&);
  ~SiPixelCompareRecHits() override = default;
  void dqmBeginRun(const edm::Run&, const edm::EventSetup&) override;
  void bookHistograms(DQMStore::IBooker& ibooker, edm::Run const& iRun, edm::EventSetup const& iSetup) override;
  void analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  const edm::ESGetToken<TrackerGeometry, TrackerDigiGeometryRecord> geomToken_;
  const edm::ESGetToken<TrackerTopology, TrackerTopologyRcd> topoToken_;
  const edm::EDGetTokenT<HitsAlpaka> tokenSoAHitsAlpaka_;    //these two are both on Host but originally they have been
  const edm::EDGetTokenT<HitsCUDA> tokenSoAHitsCUDA_;  //produced on Host or on Device
  const std::string topFolderName_;
  const float mind2cut_;
  static constexpr uint32_t invalidHit_ = std::numeric_limits<uint32_t>::max();
  static constexpr float micron_ = 10000.;
  const TrackerGeometry* tkGeom_ = nullptr;
  const TrackerTopology* tTopo_ = nullptr;
  MonitorElement* hnHits_;
  MonitorElement* hBchargeL_[4];  // max 4 barrel hits
  MonitorElement* hBsizexL_[4];
  MonitorElement* hBsizeyL_[4];
  MonitorElement* hBposxL_[4];
  MonitorElement* hBposyL_[4];
  MonitorElement* hFchargeD_[2][12];  // max 12 endcap disks
  MonitorElement* hFsizexD_[2][12];
  MonitorElement* hFsizeyD_[2][12];
  MonitorElement* hFposxD_[2][12];
  MonitorElement* hFposyD_[2][12];
  //differences
  MonitorElement* hBchargeDiff_;
  MonitorElement* hFchargeDiff_;
  MonitorElement* hBsizeXDiff_;
  MonitorElement* hFsizeXDiff_;
  MonitorElement* hBsizeYDiff_;
  MonitorElement* hFsizeYDiff_;
  MonitorElement* hBposXDiff_;
  MonitorElement* hFposXDiff_;
  MonitorElement* hBposYDiff_;
  MonitorElement* hFposYDiff_;
};

//
// constructors
//
template <typename T>
SiPixelCompareRecHits<T>::SiPixelCompareRecHits(const edm::ParameterSet& iConfig)
    : geomToken_(esConsumes<TrackerGeometry, TrackerDigiGeometryRecord, edm::Transition::BeginRun>()),
      topoToken_(esConsumes<TrackerTopology, TrackerTopologyRcd, edm::Transition::BeginRun>()),
      tokenSoAHitsAlpaka_(consumes(iConfig.getParameter<edm::InputTag>("pixelHitsSrcAlpaka"))),
      tokenSoAHitsCUDA_(consumes(iConfig.getParameter<edm::InputTag>("pixelHitsSrcCUDA"))),
      topFolderName_(iConfig.getParameter<std::string>("topFolderName")),
      mind2cut_(iConfig.getParameter<double>("minD2cut")) {}

//
// Begin Run
//
template <typename T>
void SiPixelCompareRecHits<T>::dqmBeginRun(const edm::Run& iRun, const edm::EventSetup& iSetup) {
  tkGeom_ = &iSetup.getData(geomToken_);
  tTopo_ = &iSetup.getData(topoToken_);
}

//
// -- Analyze
//
template <typename T>
void SiPixelCompareRecHits<T>::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  const auto& rhsoaHandleAlpaka = iEvent.getHandle(tokenSoAHitsAlpaka_);
  const auto& rhsoaHandleCUDA = iEvent.getHandle(tokenSoAHitsCUDA_);
  if (not rhsoaHandleAlpaka or not rhsoaHandleCUDA) {
    edm::LogWarning out("SiPixelCompareRecHits");
    if (not rhsoaHandleAlpaka) {
      out << "reference (Alpaka) rechits not found; ";
    }
    if (not rhsoaHandleCUDA) {
      out << "target (CUDA) rechits not found; ";
    }
    out << "the comparison will not run.";
    return;
  }

  auto const& rhsoaAlpaka = *rhsoaHandleAlpaka;
  auto const& rhsoaCUDA = *rhsoaHandleCUDA;

  auto const& soa2dAlpaka = rhsoaAlpaka.const_view();
  auto const& soa2dCUDA = rhsoaCUDA.const_view();

  uint32_t nHitsAlpaka = soa2dAlpaka.metadata().size();
  uint32_t nHitsCUDA = soa2dCUDA.metadata().size();

  hnHits_->Fill(nHitsAlpaka, nHitsCUDA);
  auto detIds = tkGeom_->detUnitIds();
  for (uint32_t i = 0; i < nHitsAlpaka; i++) {
    float minD = mind2cut_;
    uint32_t matchedHit = invalidHit_;
    uint16_t indAlpaka = soa2dAlpaka[i].detectorIndex();
    float xLocalAlpaka = soa2dAlpaka[i].xLocal();
    float yLocalAlpaka = soa2dAlpaka[i].yLocal();
    for (uint32_t j = 0; j < nHitsCUDA; j++) {
      if (soa2dCUDA.detectorIndex(j) == indAlpaka) {
        float dx = xLocalAlpaka - soa2dCUDA[j].xLocal();
        float dy = yLocalAlpaka - soa2dCUDA[j].yLocal();
        float distance = dx * dx + dy * dy;
        if (distance < minD) {
          minD = distance;
          matchedHit = j;
        }
      }
    }
    DetId id = detIds[indAlpaka];
    uint32_t chargeAlpaka = soa2dAlpaka[i].chargeAndStatus().charge;
    int16_t sizeXAlpaka = std::ceil(float(std::abs(soa2dAlpaka[i].clusterSizeX()) / 8.));
    int16_t sizeYAlpaka = std::ceil(float(std::abs(soa2dAlpaka[i].clusterSizeY()) / 8.));
    uint32_t chargeCUDA = 0;
    int16_t sizeXCUDA = -99;
    int16_t sizeYCUDA = -99;
    float xLocalCUDA = -999.;
    float yLocalCUDA = -999.;
    if (matchedHit != invalidHit_) {
      chargeCUDA = soa2dCUDA[matchedHit].chargeAndStatus().charge;
      sizeXCUDA = std::ceil(float(std::abs(soa2dCUDA[matchedHit].clusterSizeX()) / 8.));
      sizeYCUDA = std::ceil(float(std::abs(soa2dCUDA[matchedHit].clusterSizeY()) / 8.));
      xLocalCUDA = soa2dCUDA[matchedHit].xLocal();
      yLocalCUDA = soa2dCUDA[matchedHit].yLocal();
    }
    switch (id.subdetId()) {
      case PixelSubdetector::PixelBarrel:
        hBchargeL_[tTopo_->pxbLayer(id) - 1]->Fill(chargeAlpaka, chargeCUDA);
        hBsizexL_[tTopo_->pxbLayer(id) - 1]->Fill(sizeXAlpaka, sizeXCUDA);
        hBsizeyL_[tTopo_->pxbLayer(id) - 1]->Fill(sizeYAlpaka, sizeYCUDA);
        hBposxL_[tTopo_->pxbLayer(id) - 1]->Fill(xLocalAlpaka, xLocalCUDA);
        hBposyL_[tTopo_->pxbLayer(id) - 1]->Fill(yLocalAlpaka, yLocalCUDA);
        hBchargeDiff_->Fill(chargeAlpaka - chargeCUDA);
        hBsizeXDiff_->Fill(sizeXAlpaka - sizeXCUDA);
        hBsizeYDiff_->Fill(sizeYAlpaka - sizeYCUDA);
        hBposXDiff_->Fill(micron_ * (xLocalAlpaka - xLocalCUDA));
        hBposYDiff_->Fill(micron_ * (yLocalAlpaka - yLocalCUDA));
        break;
      case PixelSubdetector::PixelEndcap:
        hFchargeD_[tTopo_->pxfSide(id) - 1][tTopo_->pxfDisk(id) - 1]->Fill(chargeAlpaka, chargeCUDA);
        hFsizexD_[tTopo_->pxfSide(id) - 1][tTopo_->pxfDisk(id) - 1]->Fill(sizeXAlpaka, sizeXCUDA);
        hFsizeyD_[tTopo_->pxfSide(id) - 1][tTopo_->pxfDisk(id) - 1]->Fill(sizeYAlpaka, sizeYCUDA);
        hFposxD_[tTopo_->pxfSide(id) - 1][tTopo_->pxfDisk(id) - 1]->Fill(xLocalAlpaka, xLocalCUDA);
        hFposyD_[tTopo_->pxfSide(id) - 1][tTopo_->pxfDisk(id) - 1]->Fill(yLocalAlpaka, yLocalCUDA);
        hFchargeDiff_->Fill(chargeAlpaka - chargeCUDA);
        hFsizeXDiff_->Fill(sizeXAlpaka - sizeXCUDA);
        hFsizeYDiff_->Fill(sizeYAlpaka - sizeYCUDA);
        hFposXDiff_->Fill(micron_ * (xLocalAlpaka - xLocalCUDA));
        hFposYDiff_->Fill(micron_ * (yLocalAlpaka - yLocalCUDA));
        break;
    }
  }
}

//
// -- Book Histograms
//
template <typename T>
void SiPixelCompareRecHits<T>::bookHistograms(DQMStore::IBooker& iBook,
                                                       edm::Run const& iRun,
                                                       edm::EventSetup const& iSetup) {
  iBook.cd();
  iBook.setCurrentFolder(topFolderName_);

  // clang-format off
  //Global
  hnHits_ = iBook.book2I("nHits", "AlpakavsCUDA RecHits per event;#Alpaka RecHits;#CUDA RecHits", 200, 0, 5000,200, 0, 5000);
  //Barrel Layer
  for(unsigned int il=0;il<tkGeom_->numberOfLayers(PixelSubdetector::PixelBarrel);il++){
    hBchargeL_[il] = iBook.book2I(Form("recHitsBLay%dCharge",il+1), Form("AlpakavsCUDA RecHits Charge Barrel Layer%d;Alpaka Charge;CUDA Charge",il+1), 250, 0, 100000, 250, 0, 100000);
    hBsizexL_[il] = iBook.book2I(Form("recHitsBLay%dSizex",il+1), Form("AlpakavsCUDA RecHits SizeX Barrel Layer%d;Alpaka SizeX;CUDA SizeX",il+1), 30, 0, 30, 30, 0, 30);
    hBsizeyL_[il] = iBook.book2I(Form("recHitsBLay%dSizey",il+1), Form("AlpakavsCUDA RecHits SizeY Barrel Layer%d;Alpaka SizeY;CUDA SizeY",il+1), 30, 0, 30, 30, 0, 30);
    hBposxL_[il] = iBook.book2D(Form("recHitsBLay%dPosx",il+1), Form("AlpakavsCUDA RecHits x-pos in Barrel Layer%d;Alpaka pos x;CUDA pos x",il+1), 200, -5, 5, 200,-5,5);
    hBposyL_[il] = iBook.book2D(Form("recHitsBLay%dPosy",il+1), Form("AlpakavsCUDA RecHits y-pos in Barrel Layer%d;Alpaka pos y;CUDA pos y",il+1), 200, -5, 5, 200,-5,5);
  }
  //Endcaps
  //Endcaps Disk
  for(int is=0;is<2;is++){
    int sign=is==0? -1:1;
    for(unsigned int id=0;id<tkGeom_->numberOfLayers(PixelSubdetector::PixelEndcap);id++){
      hFchargeD_[is][id] = iBook.book2I(Form("recHitsFDisk%+dCharge",id*sign+sign), Form("AlpakavsCUDA RecHits Charge Endcaps Disk%+d;Alpaka Charge;CUDA Charge",id*sign+sign), 250, 0, 100000, 250, 0, 100000);
      hFsizexD_[is][id] = iBook.book2I(Form("recHitsFDisk%+dSizex",id*sign+sign), Form("AlpakavsCUDA RecHits SizeX Endcaps Disk%+d;Alpaka SizeX;CUDA SizeX",id*sign+sign), 30, 0, 30, 30, 0, 30);
      hFsizeyD_[is][id] = iBook.book2I(Form("recHitsFDisk%+dSizey",id*sign+sign), Form("AlpakavsCUDA RecHits SizeY Endcaps Disk%+d;Alpaka SizeY;CUDA SizeY",id*sign+sign), 30, 0, 30, 30, 0, 30);
      hFposxD_[is][id] = iBook.book2D(Form("recHitsFDisk%+dPosx",id*sign+sign), Form("AlpakavsCUDA RecHits x-pos Endcaps Disk%+d;Alpaka pos x;CUDA pos x",id*sign+sign), 200, -5, 5, 200, -5, 5);
      hFposyD_[is][id] = iBook.book2D(Form("recHitsFDisk%+dPosy",id*sign+sign), Form("AlpakavsCUDA RecHits y-pos Endcaps Disk%+d;Alpaka pos y;CUDA pos y",id*sign+sign), 200, -5, 5, 200, -5, 5);
    }
  }
  //1D differences
  hBchargeDiff_ = iBook.book1D("rechitChargeDiffBpix","Charge differnce of rechits in BPix; rechit charge difference (Alpaka - CUDA)", 101, -50.5, 50.5);
  hFchargeDiff_ = iBook.book1D("rechitChargeDiffFpix","Charge differnce of rechits in FPix; rechit charge difference (Alpaka - CUDA)", 101, -50.5, 50.5);
  hBsizeXDiff_ = iBook.book1D("rechitsizeXDiffBpix","SizeX difference of rechits in BPix; rechit sizex difference (Alpaka - CUDA)", 21, -10.5, 10.5);
  hFsizeXDiff_ = iBook.book1D("rechitsizeXDiffFpix","SizeX difference of rechits in FPix; rechit sizex difference (Alpaka - CUDA)", 21, -10.5, 10.5);
  hBsizeYDiff_ = iBook.book1D("rechitsizeYDiffBpix","SizeY difference of rechits in BPix; rechit sizey difference (Alpaka - CUDA)", 21, -10.5, 10.5);
  hFsizeYDiff_ = iBook.book1D("rechitsizeYDiffFpix","SizeY difference of rechits in FPix; rechit sizey difference (Alpaka - CUDA)", 21, -10.5, 10.5);
  hBposXDiff_ = iBook.book1D("rechitsposXDiffBpix","x-position difference of rechits in BPix; rechit x-pos difference (Alpaka - CUDA)", 1000, -10, 10);
  hFposXDiff_ = iBook.book1D("rechitsposXDiffFpix","x-position difference of rechits in FPix; rechit x-pos difference (Alpaka - CUDA)", 1000, -10, 10);
  hBposYDiff_ = iBook.book1D("rechitsposYDiffBpix","y-position difference of rechits in BPix; rechit y-pos difference (Alpaka - CUDA)", 1000, -10, 10);
  hFposYDiff_ = iBook.book1D("rechitsposYDiffFpix","y-position difference of rechits in FPix; rechit y-pos difference (Alpaka - CUDA)", 1000, -10, 10);
}

template<typename T>
void SiPixelCompareRecHits<T>::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  // monitorpixelRecHitsSoAAlpaka
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("pixelHitsSrcAlpaka", edm::InputTag("siPixelRecHitsPreSplittingAlpakaSerial")); // This is changed in the cfg instances to also compare AlpakavsCUDA on GPU/Device
  desc.add<edm::InputTag>("pixelHitsSrcCUDA", edm::InputTag("siPixelRecHitsPreSplittingSoA@cpu")); // This is changed in the cfg instances to also compare AlpakavsCUDA on GPU/Device
  desc.add<std::string>("topFolderName", "SiPixelHeterogeneous/PixelRecHitsAlpakavsCUDA"); // This is changed in the cfg instances to also compare AlpakavsCUDA on GPU/Device
  desc.add<double>("minD2cut", 0.0001);
  descriptions.addWithDefaultLabel(desc);
}

using SiPixelPhase1CompareRecHits = SiPixelCompareRecHits<pixelTopology::Phase1>;
using SiPixelPhase2CompareRecHits = SiPixelCompareRecHits<pixelTopology::Phase2>;
using SiPixelHIonPhase1CompareRecHits = SiPixelCompareRecHits<pixelTopology::HIonPhase1>;

#include "FWCore/Framework/interface/MakerMacros.h"
DEFINE_FWK_MODULE(SiPixelPhase1CompareRecHits);
DEFINE_FWK_MODULE(SiPixelPhase2CompareRecHits);
DEFINE_FWK_MODULE(SiPixelHIonPhase1CompareRecHits);