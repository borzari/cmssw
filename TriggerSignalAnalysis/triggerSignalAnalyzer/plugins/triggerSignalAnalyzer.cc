// -*- C++ -*-
//
// Package:    TriggerSignalAnalysis/triggerSignalAnalyzer
// Class:      triggerSignalAnalyzer
//
/**\class triggerSignalAnalyzer triggerSignalAnalyzer.cc TriggerSignalAnalysis/triggerSignalAnalyzer/plugins/triggerSignalAnalyzer.cc

 Description: [one line class summary]

 Implementation:
     [Notes on implementation]
*/
//
// Original Author:  Breno Orzari
//         Created:  Thu, 30 May 2024 22:50:06 GMT
//
//

// system include files
#include <memory>
#include <string>
#include <map>
#include <vector>

#include "TH1D.h"
#include "TLorentzVector.h"

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/one/EDAnalyzer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/PatCandidates/interface/IsolatedTrack.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#include "DataFormats/PatCandidates/interface/Muon.h"
#include "DataFormats/PatCandidates/interface/Jet.h"
#include "FWCore/Common/interface/TriggerNames.h"
#include "DataFormats/Common/interface/TriggerResults.h"
#include "DataFormats/PatCandidates/interface/TriggerObjectStandAlone.h"

#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/PluginManager/interface/ModuleDef.h"
//
// class declaration
//

// If the analyzer does not use TFileService, please remove
// the template argument to the base class so the class inherits
// from  edm::one::EDAnalyzer<>
// This will improve performance in multithreaded jobs.

// using reco::TrackCollection;

class triggerSignalAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit triggerSignalAnalyzer(const edm::ParameterSet&);
  ~triggerSignalAnalyzer() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  const double getTrackIsolation(const pat::IsolatedTrack&, const std::vector<pat::IsolatedTrack> &, const bool, const bool, const double, const double = 1.0e-12) const;

private:
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  edm::Service<TFileService> fs_;
  std::map<std::string, TH1D *> oneDHists_;
  double minRange = 0.0;
  double maxRange = 1000.0;
  double bins = 100;

  // ----------member data ---------------------------
  edm::EDGetTokenT<std::vector<pat::IsolatedTrack>> tracksToken_;
  edm::EDGetTokenT<std::vector<pat::MET>> metsToken_;
  edm::EDGetTokenT<std::vector<pat::Muon>> muonsToken_;
  edm::EDGetTokenT<std::vector<pat::Jet>> jetsToken_;
  edm::EDGetTokenT<edm::TriggerResults> triggersToken_;
  edm::EDGetTokenT<std::vector<pat::TriggerObjectStandAlone>> trigobjsToken_;
};

//
// constants, enums and typedefs
//

//
// static data member definitions
//

//
// constructors and destructor
//
triggerSignalAnalyzer::triggerSignalAnalyzer(const edm::ParameterSet& iConfig)
    : tracksToken_(consumes<std::vector<pat::IsolatedTrack>>(iConfig.getParameter<edm::InputTag>("tracks"))),
      metsToken_(consumes<std::vector<pat::MET>>(iConfig.getParameter<edm::InputTag>("mets"))),
      muonsToken_(consumes<std::vector<pat::Muon>>(iConfig.getParameter<edm::InputTag>("muons"))),
      jetsToken_(consumes<std::vector<pat::Jet>>(iConfig.getParameter<edm::InputTag>("jets"))),
      triggersToken_(consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("triggers"))),
      trigobjsToken_(consumes<std::vector<pat::TriggerObjectStandAlone>>(iConfig.getParameter<edm::InputTag>("trigobjs"))) {
  //now do what ever initialization is needed

  oneDHists_["4pfmet105"] = fs_->make<TH1D>("4pfmet105", "", bins, minRange, maxRange);
  oneDHists_["4hltpfmet105"] = fs_->make<TH1D>("4hltpfmet105", "", bins, minRange, maxRange);
  oneDHists_["5pfmet105"] = fs_->make<TH1D>("5pfmet105", "", bins, minRange, maxRange);
  oneDHists_["5hltpfmet105"] = fs_->make<TH1D>("5hltpfmet105", "", bins, minRange, maxRange);
  oneDHists_["6pfmet105"] = fs_->make<TH1D>("6pfmet105", "", bins, minRange, maxRange);
  oneDHists_["6hltpfmet105"] = fs_->make<TH1D>("6hltpfmet105", "", bins, minRange, maxRange);
  oneDHists_["pfmet105"] = fs_->make<TH1D>("pfmet105", "", bins, minRange, maxRange);
  oneDHists_["hltpfmet105"] = fs_->make<TH1D>("hltpfmet105", "", bins, minRange, maxRange);

  oneDHists_["1204pfmet105"] = fs_->make<TH1D>("1204pfmet105", "", bins, minRange, maxRange);
  oneDHists_["1204hltpfmet105"] = fs_->make<TH1D>("1204hltpfmet105", "", bins, minRange, maxRange);
  oneDHists_["1205pfmet105"] = fs_->make<TH1D>("1205pfmet105", "", bins, minRange, maxRange);
  oneDHists_["1205hltpfmet105"] = fs_->make<TH1D>("1205hltpfmet105", "", bins, minRange, maxRange);
  oneDHists_["1206pfmet105"] = fs_->make<TH1D>("1206pfmet105", "", bins, minRange, maxRange);
  oneDHists_["1206hltpfmet105"] = fs_->make<TH1D>("1206hltpfmet105", "", bins, minRange, maxRange);
  oneDHists_["120pfmet105"] = fs_->make<TH1D>("120pfmet105", "", bins, minRange, maxRange);
  oneDHists_["120hltpfmet105"] = fs_->make<TH1D>("120hltpfmet105", "", bins, minRange, maxRange);

  oneDHists_["countTotal"] = fs_->make<TH1D>("countTotal", "", 1, 0.0, 1.0);
  oneDHists_["countJet"] = fs_->make<TH1D>("countJet", "", 1, 0.0, 1.0);
  oneDHists_["countEta2p5"] = fs_->make<TH1D>("countEta2p5", "", 1, 0.0, 1.0);
  oneDHists_["countIsHP"] = fs_->make<TH1D>("countIsHP", "", 1, 0.0, 1.0);
  oneDHists_["countD0"] = fs_->make<TH1D>("countD0", "", 1, 0.0, 1.0);
  oneDHists_["countDz"] = fs_->make<TH1D>("countDz", "", 1, 0.0, 1.0);
  oneDHists_["countPixHit"] = fs_->make<TH1D>("countPixHit", "", 1, 0.0, 1.0);
  oneDHists_["countHit"] = fs_->make<TH1D>("countHit", "", 1, 0.0, 1.0);
  oneDHists_["countMissInn"] = fs_->make<TH1D>("countMissInn", "", 1, 0.0, 1.0);
  oneDHists_["countMissMid"] = fs_->make<TH1D>("countMissMid", "", 1, 0.0, 1.0);
  oneDHists_["countIso"] = fs_->make<TH1D>("countIso", "", 1, 0.0, 1.0);
  oneDHists_["countHLT"] = fs_->make<TH1D>("countHLT", "", 1, 0.0, 1.0);

}

triggerSignalAnalyzer::~triggerSignalAnalyzer() {
  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)
  //
  // please remove this method altogether if it would be left empty
}

//
// member functions
//

// ------------ method called for each event  ------------
void triggerSignalAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  using namespace edm;

  bool passEta2p5 = false;
  bool passIsHP = false;
  bool passD0 = false;
  bool passDz = false;
  bool passPixHit = false;
  bool passHit = false;
  bool passMissInn = false;
  bool passMissMid = false;
  bool passIso = false;

  oneDHists_.at("countTotal")->Fill(0.5);

  edm::Handle<edm::TriggerResults> triggerBits;
  edm::Handle<std::vector<pat::TriggerObjectStandAlone> > triggerObjs;

  iEvent.getByToken(triggersToken_, triggerBits);
  iEvent.getByToken(trigobjsToken_, triggerObjs);

  for(auto triggerObj : *triggerObjs) {
    triggerObj.unpackNamesAndLabels(iEvent, *triggerBits);
  }

  const edm::TriggerNames &allTriggerNames = iEvent.triggerNames(*triggerBits);

  std::string triggerName = "HLT_MET105_IsoTrk50_v11";

  edm::Handle<std::vector<pat::MET> > pfmet;
  iEvent.getByToken(metsToken_, pfmet);

  TLorentzVector pfMetNoMu;
  pfMetNoMu.SetPtEtaPhiM(pfmet->at(0).pt(),0.0,pfmet->at(0).phi(),0.0);

  for (const auto& muon : iEvent.get(muonsToken_)) {

    TLorentzVector mu;
    mu.SetPtEtaPhiM(muon.pt(),muon.eta(),muon.phi(),0.1057128);
    pfMetNoMu = (pfMetNoMu + mu);

  }

  bool isGoodJet = false;
  bool isGoodTrack = false;

  bool isNLayers4 = false;
  bool isNLayers5 = false;
  bool isNLayers6p = false;

  int nJet_aux = 0;
  for (const auto& jet : iEvent.get(jetsToken_)) {

    if(nJet_aux != 0) break;
    if(abs(jet.eta()) < 2.4 || jet.eta() < -998.9) isGoodJet = true;
    nJet_aux = nJet_aux + 1;

  }

  if(isGoodJet){

    oneDHists_.at("countJet")->Fill(0.5);

    for (const auto& track : iEvent.get(tracksToken_)) {

      if(abs(track.eta()) > 2.5) continue;
      passEta2p5 = true;
      if(!(track.isHighPurityTrack())) continue;
      passIsHP = true;
      if(abs(track.dxy()) > 0.02) continue;
      passD0 = true;
      if(abs(track.dz()) > 0.5) continue;
      passDz = true;
      if(track.hitPattern().numberOfValidPixelHits() < 4) continue;
      passPixHit = true;
      if(track.hitPattern().numberOfValidHits() < 4) continue;
      passHit = true;
      if(track.lostInnerLayers() != 0) continue;
      passMissInn = true;
      if(track.lostLayers() != 0) continue;
      passMissMid = true;
      // if((track.pfIsolationDR03().chargedHadronIso() / track.pt()) > 0.01) continue;
      if((getTrackIsolation(track, iEvent.get(tracksToken_), true, false, 0.3))/track.pt() > 0.01) continue;
      passIso = true;

      isGoodTrack = true;

      if(track.hitPattern().trackerLayersWithMeasurement() == 4) {
        isNLayers4 = true;
      }
      if(track.hitPattern().trackerLayersWithMeasurement() == 5) {
        isNLayers5 = true;
      }
      if(track.hitPattern().trackerLayersWithMeasurement() >= 6) {
        isNLayers6p = true;
      }

    }

    if(passEta2p5) oneDHists_.at("countEta2p5")->Fill(0.5);
    if(passIsHP) oneDHists_.at("countIsHP")->Fill(0.5);
    if(passD0) oneDHists_.at("countD0")->Fill(0.5);
    if(passDz) oneDHists_.at("countDz")->Fill(0.5);
    if(passPixHit) oneDHists_.at("countPixHit")->Fill(0.5);
    if(passHit) oneDHists_.at("countHit")->Fill(0.5);
    if(passMissInn) oneDHists_.at("countMissInn")->Fill(0.5);
    if(passMissMid) oneDHists_.at("countMissMid")->Fill(0.5);
    if(passIso) oneDHists_.at("countIso")->Fill(0.5);

    if(isGoodTrack){

      if(isNLayers4) oneDHists_.at("4pfmet105")->Fill(pfMetNoMu.Pt());
      if(isNLayers5) oneDHists_.at("5pfmet105")->Fill(pfMetNoMu.Pt());
      if(isNLayers6p) oneDHists_.at("6pfmet105")->Fill(pfMetNoMu.Pt());
      oneDHists_.at("pfmet105")->Fill(pfMetNoMu.Pt());
      if(pfMetNoMu.Pt() > 120.0){
        if(isNLayers4) oneDHists_.at("1204pfmet105")->Fill(pfMetNoMu.Pt());
        if(isNLayers5) oneDHists_.at("1205pfmet105")->Fill(pfMetNoMu.Pt());
        if(isNLayers6p) oneDHists_.at("1206pfmet105")->Fill(pfMetNoMu.Pt());
        oneDHists_.at("120pfmet105")->Fill(pfMetNoMu.Pt());
      }

      int TriggerFired = 0;
      for(unsigned i = 0; i < allTriggerNames.size(); i++) {
        std::string thisName = allTriggerNames.triggerName(i);
        if (thisName.find(triggerName) == 0){
          TriggerFired |= triggerBits->accept(i);
        }
      }

      if(TriggerFired == 1){
        oneDHists_.at("countHLT")->Fill(0.5);
        if(isNLayers4) oneDHists_.at("4hltpfmet105")->Fill(pfMetNoMu.Pt());
        if(isNLayers5) oneDHists_.at("5hltpfmet105")->Fill(pfMetNoMu.Pt());
        if(isNLayers6p) oneDHists_.at("6hltpfmet105")->Fill(pfMetNoMu.Pt());
        oneDHists_.at("hltpfmet105")->Fill(pfMetNoMu.Pt());
        if(pfMetNoMu.Pt() > 120.0){
          if(isNLayers4) oneDHists_.at("1204hltpfmet105")->Fill(pfMetNoMu.Pt());
          if(isNLayers5) oneDHists_.at("1205hltpfmet105")->Fill(pfMetNoMu.Pt());
          if(isNLayers6p) oneDHists_.at("1206hltpfmet105")->Fill(pfMetNoMu.Pt());
          oneDHists_.at("120hltpfmet105")->Fill(pfMetNoMu.Pt());
        }
      }

    }

  }
  
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void triggerSignalAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  //The following says we do not know what parameters are allowed so do no validation
  // Please change this to state exactly what you do use, even if it is no parameters

  //Specify that only 'tracks' is allowed
  //To use, remove the default given above and uncomment below
  edm::ParameterSetDescription desc;

  desc.add<edm::InputTag>("tracks", edm::InputTag("isolatedTracks"));
  desc.add<edm::InputTag>("mets", edm::InputTag("slimmedMETs"));
  desc.add<edm::InputTag>("muons", edm::InputTag("slimmedMuons"));
  desc.add<edm::InputTag>("jets", edm::InputTag("slimmedJets"));
  desc.add<edm::InputTag>("triggers", edm::InputTag("TriggerResults"));
  desc.add<edm::InputTag>("trigobjs", edm::InputTag("slimmedPatTrigger"));

  descriptions.addWithDefaultLabel(desc);
}

const double triggerSignalAnalyzer::getTrackIsolation (const pat::IsolatedTrack& track, const std::vector<pat::IsolatedTrack> &tracks, const bool noPU, const bool noFakes, const double outerDeltaR, const double innerDeltaR) const
{
  double sumPt = 0.0;
  for (const auto &t : tracks)
    {
      if (noFakes && (t.hitPattern().pixelLayersWithMeasurement() < 2 || t.hitPattern().trackerLayersWithMeasurement() < 5 || fabs(t.dxy() / t.dxyError()) > 5.0)) continue;
      if (noPU && track.dz() > 3.0 * hypot(track.dzError(), t.dzError())) continue;
      double dR = deltaR (track, t);
      if (dR < outerDeltaR && dR > innerDeltaR) sumPt += t.pt ();
    }

  return sumPt;
}

//define this as a plug-in
DEFINE_FWK_MODULE(triggerSignalAnalyzer);
