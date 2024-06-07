// -*- C++ -*-
//
// Package:    TriggerSignalAnalysis/triggerDataBGAnalyzer
// Class:      triggerDataBGAnalyzer
//
/**\class triggerDataBGAnalyzer triggerDataBGAnalyzer.cc TriggerSignalAnalysis/triggerDataBGAnalyzer/plugins/triggerDataBGAnalyzer.cc

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
#include <algorithm>
#include <iostream>
#include <fstream>

#include "TH1D.h"
#include "TLorentzVector.h"
#include "TTree.h"

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
#include "DataFormats/VertexReco/interface/Vertex.h"

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

class triggerDataBGAnalyzer : public edm::one::EDAnalyzer<edm::one::SharedResources> {
public:
  explicit triggerDataBGAnalyzer(const edm::ParameterSet&);
  ~triggerDataBGAnalyzer() override;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);
  bool isMatchedToTriggerObject (const edm::Event &, const edm::TriggerResults &, const pat::Muon &, const std::vector<pat::TriggerObjectStandAlone> &, const std::string &, const std::string &, const double = 0.1);

  Int_t nJet;

  Float_t muon_pt;
  Float_t muon_eta;
  Float_t muon_phi;
  Int_t muon_tightId;
  Int_t muon_missInnHits;
  Int_t muon_missMidHits;
  Int_t muon_missOutHits;
  Int_t muon_numValidHits;
  Int_t muon_numPixelHits;
  Int_t muon_numValidLayers;
  Float_t muon_iso;
  Int_t muon_isTrackPassingTrackLeg;

  Float_t met_pt;
  Float_t met_ptNoMu;
  Float_t met_phi;
  Float_t met_phiNoMu;

  Float_t jet_pt[1000];
  Float_t jet_eta[1000];
  Float_t jet_phi[1000];

  Int_t pass_HLT_MET105_IsoTrk50;
  Int_t pass_HLT_MET120_IsoTrk50;
  Int_t pass_HLT_PFMET105_IsoTrk50;
  Int_t pass_HLT_PFMET120_PFMHT120_IDTight;
  Int_t pass_HLT_PFMET130_PFMHT130_IDTight;
  Int_t pass_HLT_PFMET140_PFMHT140_IDTight;
  Int_t pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60;
  Int_t pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF;
  Int_t pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF;
  Int_t pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF;
  Int_t pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF;
  Int_t pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight;
  Int_t pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight;
  Int_t pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight;
  Int_t pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60;
  Int_t pass_HLT_IsoMu24;
  Int_t pass_hltMET105Filter;
  Int_t pass_hltTrk50Filter;

private:
  void analyze(const edm::Event&, const edm::EventSetup&) override;
  edm::Service<TFileService> fs_;
  std::map<std::string, TH1D *> oneDHists_;
  TTree* tree_;
  double minRange = 0.0;
  double maxRange = 1000.0;
  double bins = 100;

  // ----------member data ---------------------------
  edm::EDGetTokenT<std::vector<reco::Vertex> > verticesToken_;
  edm::EDGetTokenT<std::vector<pat::MET>> metsToken_;
  edm::EDGetTokenT<std::vector<pat::Muon>> muonsToken_;
  edm::EDGetTokenT<std::vector<pat::Jet>> jetsToken_;
  edm::EDGetTokenT<edm::TriggerResults> triggersPATToken_;
  edm::EDGetTokenT<edm::TriggerResults> triggersHLTToken_;
  edm::EDGetTokenT<std::vector<pat::TriggerObjectStandAlone>> trigobjsToken_;
  edm::EDGetTokenT<std::vector<pat::PackedCandidate>> pfToken_;
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
triggerDataBGAnalyzer::triggerDataBGAnalyzer(const edm::ParameterSet& iConfig)
    : verticesToken_(consumes<std::vector<reco::Vertex>>(iConfig.getParameter<edm::InputTag>("vertices"))),
      metsToken_(consumes<std::vector<pat::MET>>(iConfig.getParameter<edm::InputTag>("mets"))),
      muonsToken_(consumes<std::vector<pat::Muon>>(iConfig.getParameter<edm::InputTag>("muons"))),
      jetsToken_(consumes<std::vector<pat::Jet>>(iConfig.getParameter<edm::InputTag>("jets"))),
      triggersPATToken_(consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("triggersPAT"))),
      triggersHLTToken_(consumes<edm::TriggerResults>(iConfig.getParameter<edm::InputTag>("triggersHLT"))),
      trigobjsToken_(consumes<std::vector<pat::TriggerObjectStandAlone>>(iConfig.getParameter<edm::InputTag>("trigobjs"))),
      pfToken_(consumes<std::vector<pat::PackedCandidate>>(iConfig.getParameter<edm::InputTag>("pfcands"))) {
  //now do what ever initialization is needed

  oneDHists_["pfmet105"] = fs_->make<TH1D>("pfmet105", "", bins, minRange, maxRange);
  oneDHists_["hltpfmet105"] = fs_->make<TH1D>("hltpfmet105", "", bins, minRange, maxRange);

  oneDHists_["120pfmet105"] = fs_->make<TH1D>("120pfmet105", "", bins, minRange, maxRange);
  oneDHists_["120hltpfmet105"] = fs_->make<TH1D>("120hltpfmet105", "", bins, minRange, maxRange);

  oneDHists_["countTotal"] = fs_->make<TH1D>("countTotal", "", 1, 0.0, 1.0);
  oneDHists_["countHLTMu"] = fs_->make<TH1D>("countHLTMu", "", 1, 0.0, 1.0);
  oneDHists_["countJet"] = fs_->make<TH1D>("countJet", "", 1, 0.0, 1.0);
  oneDHists_["countPt55"] = fs_->make<TH1D>("countPt55", "", 1, 0.0, 1.0);
  oneDHists_["countEta2p1"] = fs_->make<TH1D>("countEta2p1", "", 1, 0.0, 1.0);
  oneDHists_["countIsTightID"] = fs_->make<TH1D>("countIsTightID", "", 1, 0.0, 1.0);
  oneDHists_["countMissInn"] = fs_->make<TH1D>("countMissInn", "", 1, 0.0, 1.0);
  oneDHists_["countMissMid"] = fs_->make<TH1D>("countMissMid", "", 1, 0.0, 1.0);
  oneDHists_["countTightPFIso"] = fs_->make<TH1D>("countTightPFIso", "", 1, 0.0, 1.0);
  oneDHists_["countHLTMET"] = fs_->make<TH1D>("countHLTMET", "", 1, 0.0, 1.0);

  nJet = 0;

  tree_ = fs_->make<TTree>("tree","tree");

  tree_->Branch("muon_pt",&muon_pt,"muon_pt/F");
  tree_->Branch("muon_eta",&muon_eta,"muon_eta/F");
  tree_->Branch("muon_phi",&muon_phi,"muon_phi/F");
  tree_->Branch("muon_tightId",&muon_tightId,"muon_tightId/I");
  tree_->Branch("muon_missInnHits",&muon_missInnHits,"muon_missInnHits/I");
  tree_->Branch("muon_missMidHits",&muon_missMidHits,"muon_missMidHits/I");
  tree_->Branch("muon_missOutHits",&muon_missOutHits,"muon_missOutHits/I");
  tree_->Branch("muon_numValidHits",&muon_numValidHits,"muon_numValidHits/I");
  tree_->Branch("muon_numPixelHits",&muon_numPixelHits,"muon_numPixelHits/I");
  tree_->Branch("muon_numValidLayers",&muon_numValidLayers,"muon_numValidLayers/I");
  tree_->Branch("muon_iso",&muon_iso,"muon_iso/F");
  tree_->Branch("muon_isTrackPassingTrackLeg",&muon_isTrackPassingTrackLeg,"muon_isTrackPassingTrackLeg/I");

  tree_->Branch("met_pt",&met_pt,"met_pt/F");
  tree_->Branch("met_ptNoMu",&met_ptNoMu,"met_ptNoMu/F");
  tree_->Branch("met_phi",&met_phi,"met_phi/F");
  tree_->Branch("met_phiNoMu",&met_phiNoMu,"met_phiNoMu/F");

  tree_->Branch("nJet",&nJet,"nJet/I");
  tree_->Branch("jet_pt",jet_pt,"jet_pt/F");
  tree_->Branch("jet_eta",jet_eta,"jet_eta/F");
  tree_->Branch("jet_phi",jet_phi,"jet_phi/F");

  tree_->Branch("hlt_pass_HLT_MET105_IsoTrk50",&pass_HLT_MET105_IsoTrk50,"pass_HLT_MET105_IsoTrk50/I");
  tree_->Branch("hlt_pass_HLT_MET120_IsoTrk50",&pass_HLT_MET120_IsoTrk50,"pass_HLT_MET120_IsoTrk50/I");
  tree_->Branch("hlt_pass_HLT_PFMET105_IsoTrk50",&pass_HLT_PFMET105_IsoTrk50,"pass_HLT_PFMET105_IsoTrk50/I");
  tree_->Branch("hlt_pass_HLT_PFMET120_PFMHT120_IDTight",&pass_HLT_PFMET120_PFMHT120_IDTight,"pass_HLT_PFMET120_PFMHT120_IDTight/I");
  tree_->Branch("hlt_pass_HLT_PFMET130_PFMHT130_IDTight",&pass_HLT_PFMET130_PFMHT130_IDTight,"pass_HLT_PFMET130_PFMHT130_IDTight/I");
  tree_->Branch("hlt_pass_HLT_PFMET140_PFMHT140_IDTight",&pass_HLT_PFMET140_PFMHT140_IDTight,"pass_HLT_PFMET140_PFMHT140_IDTight/I");
  tree_->Branch("hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60",&pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60,"pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60/I");
  tree_->Branch("hlt_pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF",&pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF,"pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF/I");
  tree_->Branch("hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF",&pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF,"pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF/I");
  tree_->Branch("hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF",&pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF,"pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF/I");
  tree_->Branch("hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF",&pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF,"pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF/I");
  tree_->Branch("hlt_pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight",&pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight,"pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight/I");
  tree_->Branch("hlt_pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight",&pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight,"pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight/I");
  tree_->Branch("hlt_pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight",&pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight,"pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight/I");
  tree_->Branch("hlt_pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60",&pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60,"pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60/I");
  tree_->Branch("hlt_pass_HLT_IsoMu24",&pass_HLT_IsoMu24,"pass_HLT_IsoMu24/I");
  tree_->Branch("hlt_pass_hltMET105Filter",&pass_hltMET105Filter,"pass_hltMET105Filter/I");
  tree_->Branch("hlt_pass_hltTrk50Filter",&pass_hltTrk50Filter,"pass_hltTrk50Filter/I");

}

triggerDataBGAnalyzer::~triggerDataBGAnalyzer() {
  // do anything here that needs to be done at desctruction time
  // (e.g. close files, deallocate resources etc.)
  //
  // please remove this method altogether if it would be left empty
}

//
// member functions
//

// ------------ method called for each event  ------------
void triggerDataBGAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup) {
  using namespace edm;

  bool passPt55 = false;
  bool passEta2p1 = false;
  bool passIsTightID = false;
  bool passMissInn = false;
  bool passMissMid = false;
  bool passTightPFIso = false;

  muon_isTrackPassingTrackLeg = 0;

  met_pt = 0.;
  met_ptNoMu = 0.;
  met_phi = 0.;
  met_phiNoMu = 0.;

  pass_HLT_MET105_IsoTrk50 = 0;
  pass_HLT_MET120_IsoTrk50 = 0;
  pass_HLT_PFMET105_IsoTrk50 = 0;
  pass_HLT_PFMET120_PFMHT120_IDTight = 0;
  pass_HLT_PFMET130_PFMHT130_IDTight = 0;
  pass_HLT_PFMET140_PFMHT140_IDTight = 0;
  pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60 = 0;
  pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF = 0;
  pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF = 0;
  pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF = 0;
  pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF = 0;
  pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight = 0;
  pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight = 0;
  pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight = 0;
  pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60 = 0;
  pass_HLT_IsoMu24 = 0;
  pass_hltMET105Filter = 0;
  pass_hltTrk50Filter = 0;

  Int_t pass_Flag_goodVertices = 0;
  Int_t pass_Flag_HBHENoiseFilter = 0;
  Int_t pass_Flag_HBHENoiseIsoFilter = 0;
  Int_t pass_Flag_EcalDeadCellTriggerPrimitiveFilter = 0;
  Int_t pass_Flag_globalSuperTightHalo2016Filter = 0;
  Int_t pass_Flag_BadPFMuonFilter = 0;
  Int_t pass_Flag_BadPFMuonDzFilter = 0;
  Int_t pass_Flag_eeBadScFilter = 0;
  Int_t pass_Flag_ecalBadCalibFilter = 0;

  nJet = 0;

  int deref = 0;

  oneDHists_.at("countTotal")->Fill(0.5);

  edm::Handle<edm::TriggerResults> triggerBitsPAT;
  iEvent.getByToken(triggersPATToken_, triggerBitsPAT);

  edm::Handle<edm::TriggerResults> triggerBitsHLT;
  iEvent.getByToken(triggersHLTToken_, triggerBitsHLT);

  edm::Handle<std::vector<pat::TriggerObjectStandAlone> > triggerObjs;
  iEvent.getByToken(trigobjsToken_, triggerObjs);

  for(auto triggerObj : *triggerObjs) {
    triggerObj.unpackNamesAndLabels(iEvent, *triggerBitsHLT);
    for (const auto &filterLabel : triggerObj.filterLabels ()){
      if (filterLabel == "hltMET105") pass_hltMET105Filter = 1;
      if (filterLabel == "hltTrk50Filter") pass_hltTrk50Filter = 1;
    }
  }

  const edm::TriggerNames &allTriggerNamesHLT = iEvent.triggerNames(*triggerBitsHLT);

  for(unsigned i = 0; i < allTriggerNamesHLT.size(); i++) {
    std::string thisName = allTriggerNamesHLT.triggerName(i);
    if (thisName.find("HLT_MET105_IsoTrk50_v") == 0) pass_HLT_MET105_IsoTrk50 = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_MET120_IsoTrk50_v") == 0) pass_HLT_MET120_IsoTrk50 = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMET105_IsoTrk50_v") == 0) pass_HLT_PFMET105_IsoTrk50 = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMET120_PFMHT120_IDTight_v") == 0) pass_HLT_PFMET120_PFMHT120_IDTight = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMET130_PFMHT130_IDTight_v") == 0) pass_HLT_PFMET130_PFMHT130_IDTight = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMET140_PFMHT140_IDTight_v") == 0) pass_HLT_PFMET140_PFMHT140_IDTight = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60_v") == 0) pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_PFHT60 = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF_v") == 0) pass_HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_FilterHF = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF_v") == 0) pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_FilterHF = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF_v") == 0) pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_FilterHF = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF_v") == 0) pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_FilterHF = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_v") == 0) pass_HLT_PFMETNoMu120_PFMHTNoMu120_IDTight = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMETNoMu130_PFMHTNoMu130_IDTight_v") == 0) pass_HLT_PFMETNoMu130_PFMHTNoMu130_IDTight = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMETNoMu140_PFMHTNoMu140_IDTight_v") == 0) pass_HLT_PFMETNoMu140_PFMHTNoMu140_IDTight = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_PFMET120_PFMHT120_IDTight_PFHT60_v") == 0) pass_HLT_PFMET120_PFMHT120_IDTight_PFHT60 = triggerBitsHLT->accept(i);
    if (thisName.find("HLT_IsoMu24_v") == 0) pass_HLT_IsoMu24 = triggerBitsHLT->accept(i);
  }

  const edm::TriggerNames &allTriggerNamesPAT = iEvent.triggerNames(*triggerBitsPAT);

  for(unsigned i = 0; i < allTriggerNamesPAT.size(); i++) {
    std::string thisName = allTriggerNamesPAT.triggerName(i);
    if (thisName.find("Flag_goodVertices") == 0) pass_Flag_goodVertices = triggerBitsPAT->accept(i);
    if (thisName.find("Flag_HBHENoiseFilter") == 0) pass_Flag_HBHENoiseFilter = triggerBitsPAT->accept(i);
    if (thisName.find("Flag_HBHENoiseIsoFilter") == 0) pass_Flag_HBHENoiseIsoFilter = triggerBitsPAT->accept(i);
    if (thisName.find("Flag_EcalDeadCellTriggerPrimitiveFilter") == 0) pass_Flag_EcalDeadCellTriggerPrimitiveFilter = triggerBitsPAT->accept(i);
    if (thisName.find("Flag_globalSuperTightHalo2016Filter") == 0) pass_Flag_globalSuperTightHalo2016Filter = triggerBitsPAT->accept(i);
    if (thisName.find("Flag_BadPFMuonFilter") == 0) pass_Flag_BadPFMuonFilter = triggerBitsPAT->accept(i);
    if (thisName.find("Flag_BadPFMuonDzFilter") == 0) pass_Flag_BadPFMuonDzFilter = triggerBitsPAT->accept(i);
    if (thisName.find("Flag_eeBadScFilter") == 0) pass_Flag_eeBadScFilter = triggerBitsPAT->accept(i);
    if (thisName.find("Flag_ecalBadCalibFilter") == 0) pass_Flag_ecalBadCalibFilter = triggerBitsPAT->accept(i);
  }

  if(pass_HLT_IsoMu24 == 1 && pass_Flag_goodVertices == 1 && pass_Flag_HBHENoiseFilter == 1 && pass_Flag_HBHENoiseIsoFilter == 1 && pass_Flag_EcalDeadCellTriggerPrimitiveFilter == 1 && pass_Flag_globalSuperTightHalo2016Filter == 1 && pass_Flag_BadPFMuonFilter == 1 && pass_Flag_BadPFMuonDzFilter == 1 && pass_Flag_eeBadScFilter == 1 && pass_Flag_ecalBadCalibFilter == 1){
    oneDHists_.at("countHLTMu")->Fill(0.5);

    edm::Handle<std::vector<reco::Vertex> > vertices;
    iEvent.getByToken(verticesToken_, vertices);
    const reco::Vertex &pv = vertices->at(0);
  
    bool isGoodJet = false;
    bool isGoodMuon = false;
  
    int nJet_aux = 0;
    for (const auto& jet : iEvent.get(jetsToken_)) {
    
      if(nJet_aux != 0) break;
      if(abs(jet.eta()) < 2.4 || jet.eta() < -998.9) isGoodJet = true;
      nJet_aux = nJet_aux + 1;
  
    }

    if(isGoodJet){
    
      oneDHists_.at("countJet")->Fill(0.5);
  
      edm::Handle<std::vector<pat::MET> > pfmet;
      iEvent.getByToken(metsToken_, pfmet);
  
      TLorentzVector pfMetNoMu;
      pfMetNoMu.SetPtEtaPhiM(pfmet->at(0).pt(),0.0,pfmet->at(0).phi(),0.0);

      pat::Muon passedMuon;
  
      for (const auto& muon : iEvent.get(muonsToken_)) {
      
        if(abs(muon.pt()) < 55.) continue;
        passPt55 = true;
        if(abs(muon.eta()) > 2.1) continue;
        passEta2p1 = true;
        if(!muon.isTightMuon(pv)) continue;
        passIsTightID = true;
        if(muon.innerTrack()->hitPattern().trackerLayersWithoutMeasurement(reco::HitPattern::MISSING_INNER_HITS) != 0) continue;
        passMissInn = true;
        if(muon.innerTrack()->hitPattern().trackerLayersWithoutMeasurement(reco::HitPattern::TRACK_HITS) != 0) continue;
        passMissMid = true;
        if(((muon.pfIsolationR04().sumChargedHadronPt + std::max(0.0,muon.pfIsolationR04().sumNeutralHadronEt + muon.pfIsolationR04().sumPhotonEt - 0.5 * muon.pfIsolationR04().sumPUPt)) / muon.pt()) > 0.15) continue;
        passTightPFIso = true;

        TLorentzVector mu;
        mu.SetPtEtaPhiM(muon.pt(),muon.eta(),muon.phi(),0.1057128);
        pfMetNoMu = (pfMetNoMu + mu);

        if(isGoodMuon) continue;

        if(isMatchedToTriggerObject(iEvent, *triggerBitsHLT, muon, *triggerObjs, "hltIterL3MuonCandidates::HLT", "hltL3crIsoL1sSingleMu22L1f0L2f10QL3f24QL3trkIsoFiltered")){

          isGoodMuon = true;
          passedMuon = muon;
          // break;

        }

      }

      if(passPt55) oneDHists_.at("countPt55")->Fill(0.5);
      if(passEta2p1) oneDHists_.at("countEta2p1")->Fill(0.5);
      if(passIsTightID) oneDHists_.at("countIsTightID")->Fill(0.5);
      if(passMissInn) oneDHists_.at("countMissInn")->Fill(0.5);
      if(passMissMid) oneDHists_.at("countMissMid")->Fill(0.5);
      if(passTightPFIso) oneDHists_.at("countTightPFIso")->Fill(0.5);
  
      if(isGoodMuon){

        oneDHists_.at("pfmet105")->Fill(pfMetNoMu.Pt());
        if(pfMetNoMu.Pt() > 120.0){
          oneDHists_.at("120pfmet105")->Fill(pfMetNoMu.Pt());
        }
  
        if(pass_HLT_MET105_IsoTrk50 == 1){
          oneDHists_.at("countHLTMET")->Fill(0.5);
          oneDHists_.at("hltpfmet105")->Fill(pfMetNoMu.Pt());
          if(pfMetNoMu.Pt() > 120.0){
            oneDHists_.at("120hltpfmet105")->Fill(pfMetNoMu.Pt());
          }
        }

        // try{

          met_pt = pfmet->at(0).pt();
          met_ptNoMu = pfMetNoMu.Pt();
          met_phi = pfmet->at(0).phi();
          met_phiNoMu = pfMetNoMu.Phi();

          for (const auto& jet : iEvent.get(jetsToken_)) {
            nJet = nJet + 1;
            jet_pt[nJet - 1] = jet.pt();
            jet_eta[nJet - 1] = jet.eta();
            jet_phi[nJet - 1] = jet.phi();
          }

          muon_pt = passedMuon.pt();
          muon_eta = passedMuon.eta();
          muon_phi = passedMuon.phi();
          muon_tightId = passedMuon.isTightMuon(pv);
          muon_missInnHits = passedMuon.innerTrack()->hitPattern().trackerLayersWithoutMeasurement(reco::HitPattern::MISSING_INNER_HITS);
          muon_missMidHits = passedMuon.innerTrack()->hitPattern().trackerLayersWithoutMeasurement(reco::HitPattern::TRACK_HITS);
          muon_missOutHits = passedMuon.innerTrack()->hitPattern().trackerLayersWithoutMeasurement(reco::HitPattern::MISSING_OUTER_HITS);
          muon_numValidHits = passedMuon.innerTrack()->hitPattern().numberOfValidTrackerHits();
          muon_numPixelHits = passedMuon.innerTrack()->hitPattern().numberOfValidPixelHits();
          muon_numValidLayers = passedMuon.innerTrack()->hitPattern().trackerLayersWithMeasurement();
          muon_iso = (passedMuon.pfIsolationR04().sumChargedHadronPt + std::max(0.0,passedMuon.pfIsolationR04().sumNeutralHadronEt + passedMuon.pfIsolationR04().sumPhotonEt - 0.5 * passedMuon.pfIsolationR04().sumPUPt)) / passedMuon.pt();
          if(isMatchedToTriggerObject(iEvent, *triggerBitsHLT, passedMuon, *triggerObjs, "hltMergedTracks::HLT", "hltTrk50Filter")) muon_isTrackPassingTrackLeg = 1;
          
        // }
        // catch (...)
        // {
        //   deref = deref + 1;
        // }

        tree_->Fill();
      
      }
  
    }
  
  }

}

bool triggerDataBGAnalyzer::isMatchedToTriggerObject (const edm::Event &event, const edm::TriggerResults &triggers, const pat::Muon &obj, const std::vector<pat::TriggerObjectStandAlone> &trigObjs, const std::string &collection, const std::string &filter, const double dR)
{
  if(collection == "") return false;
  for(auto trigObj : trigObjs) {
    trigObj.unpackNamesAndLabels(event, triggers);
    if(trigObj.collection() != collection) continue;
    if(filter != "") {
      bool flag = false;
      for(const auto &filterLabel : trigObj.filterLabels ())
        if(filterLabel == filter) {
          flag = true;
          break;
        }
      if (!flag) continue;
    }
    if(filter == "hltTrk50Filter") std::cout << trigObj.eta() << std::endl;
    if(deltaR (obj, trigObj) > dR) continue;
    return true;
  }
  return false;
}

// ------------ method fills 'descriptions' with the allowed parameters for the module  ------------
void triggerDataBGAnalyzer::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {

  edm::ParameterSetDescription desc;

  desc.add<edm::InputTag>("vertices", edm::InputTag("offlineSlimmedPrimaryVertices"));
  desc.add<edm::InputTag>("mets", edm::InputTag("slimmedMETs"));
  desc.add<edm::InputTag>("muons", edm::InputTag("slimmedMuons"));
  desc.add<edm::InputTag>("jets", edm::InputTag("slimmedJets"));
  desc.add<edm::InputTag>("triggersPAT", edm::InputTag("TriggerResults","","PAT"));
  desc.add<edm::InputTag>("triggersHLT", edm::InputTag("TriggerResults","","HLT"));
  desc.add<edm::InputTag>("trigobjs", edm::InputTag("slimmedPatTrigger"));
  desc.add<edm::InputTag>("pfcands", edm::InputTag("packedPFCandidates"));

  descriptions.addWithDefaultLabel(desc);

}

//define this as a plug-in
DEFINE_FWK_MODULE(triggerDataBGAnalyzer);
