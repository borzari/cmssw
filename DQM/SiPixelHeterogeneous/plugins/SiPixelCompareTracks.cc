// -*- C++ -*-
// Package:    SiPixelCompareTracks
// Class:      SiPixelCompareTracks
//
/**\class SiPixelCompareTracks SiPixelCompareTracks.cc
*/
//
// Author: Suvankar Roy Chowdhury
//

// for string manipulations
#include <fmt/printf.h>
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Math/interface/deltaR.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
// DQM Histograming
#include "DQMServices/Core/interface/MonitorElement.h"
#include "DQMServices/Core/interface/DQMEDAnalyzer.h"
#include "DQMServices/Core/interface/DQMStore.h"
// DataFormats
#include "DataFormats/TrackSoA/interface/TracksHost.h"
#include "DataFormats/TrackSoA/interface/alpaka/TrackUtilities.h"
#include "CUDADataFormats/Track/interface/TrackSoAHeterogeneousHost.h"
#include "CUDADataFormats/Track/interface/TrackSoAHeterogeneousDevice.h"

namespace {
  // same logic used for the MTV:
  // cf https://github.com/cms-sw/cmssw/blob/master/Validation/RecoTrack/src/MTVHistoProducerAlgoForTracker.cc
  typedef dqm::reco::DQMStore DQMStore;

  void setBinLog(TAxis* axis) {
    int bins = axis->GetNbins();
    float from = axis->GetXmin();
    float to = axis->GetXmax();
    float width = (to - from) / bins;
    std::vector<float> new_bins(bins + 1, 0);
    for (int i = 0; i <= bins; i++) {
      new_bins[i] = TMath::Power(10, from + i * width);
    }
    axis->Set(bins, new_bins.data());
  }

  void setBinLogX(TH1* h) {
    TAxis* axis = h->GetXaxis();
    setBinLog(axis);
  }
  void setBinLogY(TH1* h) {
    TAxis* axis = h->GetYaxis();
    setBinLog(axis);
  }

  template <typename... Args>
  dqm::reco::MonitorElement* make2DIfLog(DQMStore::IBooker& ibook, bool logx, bool logy, Args&&... args) {
    auto h = std::make_unique<TH2I>(std::forward<Args>(args)...);
    if (logx)
      setBinLogX(h.get());
    if (logy)
      setBinLogY(h.get());
    const auto& name = h->GetName();
    return ibook.book2I(name, h.release());
  }
}  // namespace

template <typename T>
class SiPixelCompareTracks : public DQMEDAnalyzer {
public:
  using PixelTrackSoAAlpaka = TracksHost<T>;
  using PixelTrackSoACUDA = TrackSoAHeterogeneousHost<T>;

  explicit SiPixelCompareTracks(const edm::ParameterSet&);
  ~SiPixelCompareTracks() override = default;
  void bookHistograms(DQMStore::IBooker& ibooker, edm::Run const& iRun, edm::EventSetup const& iSetup) override;
  void analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) override;
  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  const edm::EDGetTokenT<PixelTrackSoAAlpaka> tokenSoATrackAlpaka_;
  const edm::EDGetTokenT<PixelTrackSoACUDA> tokenSoATrackCUDA_;
  const std::string topFolderName_;
  const bool useQualityCut_;
  const reco::pixelTrack::Quality minQuality_;
  const float dr2cut_;
  MonitorElement* hnTracks_;
  MonitorElement* hnLooseAndAboveTracks_;
  MonitorElement* hnLooseAndAboveTracks_matched_;
  MonitorElement* hnHits_;
  MonitorElement* hnHitsVsPhi_;
  MonitorElement* hnHitsVsEta_;
  MonitorElement* hnLayers_;
  MonitorElement* hnLayersVsPhi_;
  MonitorElement* hnLayersVsEta_;
  MonitorElement* hCharge_;
  MonitorElement* hchi2_;
  MonitorElement* hChi2VsPhi_;
  MonitorElement* hChi2VsEta_;
  MonitorElement* hpt_;
  MonitorElement* hptLogLog_;
  MonitorElement* heta_;
  MonitorElement* hphi_;
  MonitorElement* hz_;
  MonitorElement* htip_;
  MonitorElement* hquality_;
  //1D differences
  MonitorElement* hptdiffMatched_;
  MonitorElement* hCurvdiffMatched_;
  MonitorElement* hetadiffMatched_;
  MonitorElement* hphidiffMatched_;
  MonitorElement* hzdiffMatched_;
  MonitorElement* htipdiffMatched_;

  //for matching eff vs region: derive the ratio at harvesting
  MonitorElement* hpt_eta_tkAllAlpaka_;
  MonitorElement* hpt_eta_tkAllAlpakaMatched_;
  MonitorElement* hphi_z_tkAllAlpaka_;
  MonitorElement* hphi_z_tkAllAlpakaMatched_;
};

//
// constructors
//

template <typename T>
SiPixelCompareTracks<T>::SiPixelCompareTracks(const edm::ParameterSet& iConfig)
    : tokenSoATrackAlpaka_(consumes<PixelTrackSoAAlpaka>(iConfig.getParameter<edm::InputTag>("pixelTrackSrcAlpaka"))),
      tokenSoATrackCUDA_(consumes<PixelTrackSoACUDA>(iConfig.getParameter<edm::InputTag>("pixelTrackSrcCUDA"))),
      topFolderName_(iConfig.getParameter<std::string>("topFolderName")),
      useQualityCut_(iConfig.getParameter<bool>("useQualityCut")),
      minQuality_(reco::pixelTrack::qualityByName(iConfig.getParameter<std::string>("minQuality"))),
      dr2cut_(iConfig.getParameter<double>("deltaR2cut")) {}

//
// -- Analyze
//
template <typename T>
void SiPixelCompareTracks<T>::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  using helperAlpaka = reco::TracksUtilities<T>;
  using helperCUDA = TracksUtilities<T>;
  const auto& tsoaHandleAlpaka = iEvent.getHandle(tokenSoATrackAlpaka_);
  const auto& tsoaHandleCUDA = iEvent.getHandle(tokenSoATrackCUDA_);
  if (not tsoaHandleAlpaka or not tsoaHandleCUDA) {
    edm::LogWarning out("SiPixelCompareTracks");
    if (not tsoaHandleAlpaka) {
      out << "reference (Alpaka) tracks not found; ";
    }
    if (not tsoaHandleCUDA) {
      out << "target (CUDA) tracks not found; ";
    }
    out << "the comparison will not run.";
    return;
  }

  auto const& tsoaAlpaka = *tsoaHandleAlpaka;
  auto const& tsoaCUDA = *tsoaHandleCUDA;
  auto maxTracksAlpaka = tsoaAlpaka.view().metadata().size();  //this should be same for both?
  auto maxTracksCUDA = tsoaCUDA.view().metadata().size();  //this should be same for both?
  auto const* qualityAlpaka = tsoaAlpaka.view().quality();
  auto const* qualityCUDA = tsoaCUDA.view().quality();
  int32_t nTracksAlpaka = 0;
  int32_t nTracksCUDA = 0;
  int32_t nLooseAndAboveTracksAlpaka = 0;
  int32_t nLooseAndAboveTracksAlpaka_matchedCUDA = 0;
  int32_t nLooseAndAboveTracksCUDA = 0;

  //Loop over CUDA tracks and store the indices of the loose tracks. Whats happens if useQualityCut_ is false?
  std::vector<int32_t> looseTrkidxCUDA;
  for (int32_t jt = 0; jt < maxTracksCUDA; ++jt) {
    if (helperCUDA::nHits(tsoaCUDA.view(), jt) == 0)
      break;  // this is a guard
    if (!(tsoaCUDA.view()[jt].pt() > 0.))
      continue;
    nTracksCUDA++;
    if (useQualityCut_ && (reco::pixelTrack::Quality) qualityCUDA[jt] < minQuality_)
      continue;
    nLooseAndAboveTracksCUDA++;
    looseTrkidxCUDA.emplace_back(jt);
  }

  //Now loop over Alpaka tracks//nested loop for loose CUDA tracks
  for (int32_t it = 0; it < maxTracksAlpaka; ++it) {
    int nHitsAlpaka = helperAlpaka::nHits(tsoaAlpaka.view(), it);

    if (nHitsAlpaka == 0)
      break;  // this is a guard

    float ptAlpaka = tsoaAlpaka.view()[it].pt();
    float etaAlpaka = tsoaAlpaka.view()[it].eta();
    float phiAlpaka = reco::phi(tsoaAlpaka.view(), it);
    float zipAlpaka = reco::zip(tsoaAlpaka.view(), it);
    float tipAlpaka = reco::tip(tsoaAlpaka.view(), it);

    if (!(ptAlpaka > 0.))
      continue;
    nTracksAlpaka++;
    if (useQualityCut_ && qualityAlpaka[it] < minQuality_)
      continue;
    nLooseAndAboveTracksAlpaka++;
    //Now loop over loose CUDA trk and find the closest in DeltaR//do we need pt cut?
    const int32_t notFound = -1;
    int32_t closestTkidx = notFound;
    float mindr2 = dr2cut_;

    for (auto gid : looseTrkidxCUDA) {
      float etaCUDA = tsoaCUDA.view()[gid].eta();
      float phiCUDA = helperCUDA::phi(tsoaCUDA.view(), gid);
      float dr2 = reco::deltaR2(etaAlpaka, phiAlpaka, etaCUDA, phiCUDA);
      if (dr2 > dr2cut_)
        continue;  // this is arbitrary
      if (mindr2 > dr2) {
        mindr2 = dr2;
        closestTkidx = gid;
      }
    }

    hpt_eta_tkAllAlpaka_->Fill(etaAlpaka, ptAlpaka);  //all Alpaka tk
    hphi_z_tkAllAlpaka_->Fill(phiAlpaka, zipAlpaka);
    if (closestTkidx == notFound)
      continue;
    nLooseAndAboveTracksAlpaka_matchedCUDA++;

    hchi2_->Fill(tsoaAlpaka.view()[it].chi2(), tsoaCUDA.view()[closestTkidx].chi2());
    hCharge_->Fill(reco::charge(tsoaAlpaka.view(), it), helperCUDA::charge(tsoaCUDA.view(), closestTkidx));
    hnHits_->Fill(helperAlpaka::nHits(tsoaAlpaka.view(), it), helperCUDA::nHits(tsoaCUDA.view(), closestTkidx));
    hnLayers_->Fill(tsoaAlpaka.view()[it].nLayers(), tsoaCUDA.view()[closestTkidx].nLayers());
    hpt_->Fill(tsoaAlpaka.view()[it].pt(), tsoaCUDA.view()[closestTkidx].pt());
    hptLogLog_->Fill(tsoaAlpaka.view()[it].pt(), tsoaCUDA.view()[closestTkidx].pt());
    heta_->Fill(etaAlpaka, tsoaCUDA.view()[closestTkidx].eta());
    hphi_->Fill(phiAlpaka, helperCUDA::phi(tsoaCUDA.view(), closestTkidx));
    hz_->Fill(zipAlpaka, helperCUDA::zip(tsoaCUDA.view(), closestTkidx));
    htip_->Fill(tipAlpaka, helperCUDA::tip(tsoaCUDA.view(), closestTkidx));
    hptdiffMatched_->Fill(ptAlpaka - tsoaCUDA.view()[closestTkidx].pt());
    hCurvdiffMatched_->Fill((reco::charge(tsoaAlpaka.view(), it) / tsoaAlpaka.view()[it].pt()) -
                            (helperCUDA::charge(tsoaCUDA.view(), closestTkidx) / tsoaCUDA.view()[closestTkidx].pt()));
    hetadiffMatched_->Fill(etaAlpaka - tsoaCUDA.view()[closestTkidx].eta());
    hphidiffMatched_->Fill(reco::deltaPhi(phiAlpaka, helperCUDA::phi(tsoaCUDA.view(), closestTkidx)));
    hzdiffMatched_->Fill(zipAlpaka - helperCUDA::zip(tsoaCUDA.view(), closestTkidx));
    htipdiffMatched_->Fill(tipAlpaka - helperCUDA::tip(tsoaCUDA.view(), closestTkidx));
    hpt_eta_tkAllAlpakaMatched_->Fill(etaAlpaka, tsoaAlpaka.view()[it].pt());  //matched to CUDA
    hphi_z_tkAllAlpakaMatched_->Fill(etaAlpaka, zipAlpaka);
  }
  hnTracks_->Fill(nTracksAlpaka, nTracksCUDA);
  hnLooseAndAboveTracks_->Fill(nLooseAndAboveTracksAlpaka, nLooseAndAboveTracksCUDA);
  hnLooseAndAboveTracks_matched_->Fill(nLooseAndAboveTracksAlpaka, nLooseAndAboveTracksAlpaka_matchedCUDA);
}

//
// -- Book Histograms
//
template <typename T>
void SiPixelCompareTracks<T>::bookHistograms(DQMStore::IBooker& iBook,
                                                     edm::Run const& iRun,
                                                     edm::EventSetup const& iSetup) {
  iBook.cd();
  iBook.setCurrentFolder(topFolderName_);

  // clang-format off
  std::string toRep = "Number of tracks";
  // FIXME: all the 2D correlation plots are quite heavy in terms of memory consumption, so a as soon as DQM supports THnSparse
  // these should be moved to a less resource consuming format
  hnTracks_ = iBook.book2I("nTracks", fmt::sprintf("%s per event; Alpaka; CUDA",toRep), 501, -0.5, 500.5, 501, -0.5, 500.5);
  hnLooseAndAboveTracks_ = iBook.book2I("nLooseAndAboveTracks", fmt::sprintf("%s (quality #geq loose) per event; Alpaka; CUDA",toRep), 501, -0.5, 500.5, 501, -0.5, 500.5);
  hnLooseAndAboveTracks_matched_ = iBook.book2I("nLooseAndAboveTracks_matched", fmt::sprintf("%s (quality #geq loose) per event; Alpaka; CUDA",toRep), 501, -0.5, 500.5, 501, -0.5, 500.5);

  toRep = "Number of all RecHits per track (quality #geq loose)";
  hnHits_ = iBook.book2I("nRecHits", fmt::sprintf("%s;Alpaka;CUDA",toRep), 15, -0.5, 14.5, 15, -0.5, 14.5);

  toRep = "Number of all layers per track (quality #geq loose)";
  hnLayers_ = iBook.book2I("nLayers", fmt::sprintf("%s;Alpaka;CUDA",toRep), 15, -0.5, 14.5, 15, -0.5, 14.5);

  toRep = "Track (quality #geq loose) #chi^{2}/ndof";
  hchi2_ = iBook.book2I("nChi2ndof", fmt::sprintf("%s;Alpaka;CUDA",toRep), 40, 0., 20., 40, 0., 20.);

  toRep = "Track (quality #geq loose) charge";
  hCharge_ = iBook.book2I("charge",fmt::sprintf("%s;Alpaka;CUDA",toRep),3, -1.5, 1.5, 3, -1.5, 1.5);

  hpt_ = iBook.book2I("pt", "Track (quality #geq loose) p_{T} [GeV];Alpaka;CUDA", 200, 0., 200., 200, 0., 200.);
  hptLogLog_ = make2DIfLog(iBook, true, true, "ptLogLog", "Track (quality #geq loose) p_{T} [GeV];Alpaka;CUDA", 200, log10(0.5), log10(200.), 200, log10(0.5), log10(200.));
  heta_ = iBook.book2I("eta", "Track (quality #geq loose) #eta;Alpaka;CUDA", 30, -3., 3., 30, -3., 3.);
  hphi_ = iBook.book2I("phi", "Track (quality #geq loose) #phi;Alpaka;CUDA", 30, -M_PI, M_PI, 30, -M_PI, M_PI);
  hz_ = iBook.book2I("z", "Track (quality #geq loose) z [cm];Alpaka;CUDA", 30, -30., 30., 30, -30., 30.);
  htip_ = iBook.book2I("tip", "Track (quality #geq loose) TIP [cm];Alpaka;CUDA", 100, -0.5, 0.5, 100, -0.5, 0.5);
  //1D difference plots
  hptdiffMatched_ = iBook.book1D("ptdiffmatched", " p_{T} diff [GeV] between matched tracks; #Delta p_{T} [GeV]", 60, -30., 30.);
  hCurvdiffMatched_ = iBook.book1D("curvdiffmatched", "q/p_{T} diff [GeV] between matched tracks; #Delta q/p_{T} [GeV]", 60, -30., 30.);
  hetadiffMatched_ = iBook.book1D("etadiffmatched", " #eta diff between matched tracks; #Delta #eta", 160, -0.04 ,0.04);
  hphidiffMatched_ = iBook.book1D("phidiffmatched", " #phi diff between matched tracks; #Delta #phi",  160, -0.04 ,0.04);
  hzdiffMatched_ = iBook.book1D("zdiffmatched", " z diff between matched tracks; #Delta z [cm]", 300, -1.5, 1.5);
  htipdiffMatched_ = iBook.book1D("tipdiffmatched", " TIP diff between matched tracks; #Delta TIP [cm]", 300, -1.5, 1.5);
  //2D plots for eff
  hpt_eta_tkAllAlpaka_ = iBook.book2I("ptetatrkAllAlpaka", "Track (quality #geq loose) on Alpaka; #eta; p_{T} [GeV];", 30, -M_PI, M_PI, 200, 0., 200.);
  hpt_eta_tkAllAlpakaMatched_ = iBook.book2I("ptetatrkAllAlpakamatched", "Track (quality #geq loose) on Alpaka matched to CUDA track; #eta; p_{T} [GeV];", 30, -M_PI, M_PI, 200, 0., 200.);

  hphi_z_tkAllAlpaka_ = iBook.book2I("phiztrkAllAlpaka", "Track (quality #geq loose) on Alpaka; #phi; z [cm];",  30, -M_PI, M_PI, 30, -30., 30.);
  hphi_z_tkAllAlpakaMatched_ = iBook.book2I("phiztrkAllAlpakamatched", "Track (quality #geq loose) on Alpaka; #phi; z [cm];", 30, -M_PI, M_PI, 30, -30., 30.);

}

template<typename T>
void SiPixelCompareTracks<T>::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  // monitorpixelTrackSoA
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("pixelTrackSrcAlpaka", edm::InputTag("pixelTracksAlpakaSerial")); // This is changed in the cfg instances to also compare AlpakavsCUDA on GPU/Device
  desc.add<edm::InputTag>("pixelTrackSrcCUDA", edm::InputTag("pixelTracksSoA@cpu")); // This is changed in the cfg instances to also compare AlpakavsCUDA on GPU/Device
  desc.add<std::string>("topFolderName", "SiPixelHeterogeneous/PixelTrackCompareAlpakavsCUDACPU"); // This is changed in the cfg instances to also compare AlpakavsCUDA on GPU/Device
  desc.add<bool>("useQualityCut", true);
  desc.add<std::string>("minQuality", "loose");
  desc.add<double>("deltaR2cut", 0.04);
  descriptions.addWithDefaultLabel(desc);
}

using SiPixelPhase1CompareTracks = SiPixelCompareTracks<pixelTopology::Phase1>;
using SiPixelPhase2CompareTracks = SiPixelCompareTracks<pixelTopology::Phase2>;
using SiPixelHIonPhase1CompareTracks = SiPixelCompareTracks<pixelTopology::HIonPhase1>;

DEFINE_FWK_MODULE(SiPixelPhase1CompareTracks);
DEFINE_FWK_MODULE(SiPixelPhase2CompareTracks);
DEFINE_FWK_MODULE(SiPixelHIonPhase1CompareTracks);