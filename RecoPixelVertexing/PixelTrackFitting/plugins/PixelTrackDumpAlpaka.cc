#include <Eigen/Core>  // needed here by soa layout

#include "DataFormats/Portable/interface/Product.h"
#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/global/EDAnalyzer.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "FWCore/PluginManager/interface/ModuleDef.h"
#include "FWCore/Utilities/interface/EDGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/RunningAverage.h"
#include "RecoTracker/TkMSParametrization/interface/PixelRecoUtilities.h"

#include "DataFormats/Vertex/interface/ZVertexSoAHost.h"
#include "DataFormats/Track/interface/TrackSoAHost.h"

template <typename TrackerTraits>
class PixelTrackDumpAlpakaT : public edm::global::EDAnalyzer<> {
public:
  using TkSoAHost = TrackSoAHost<TrackerTraits>;
  using VertexSoAHost = ZVertexHost;

  explicit PixelTrackDumpAlpakaT(const edm::ParameterSet& iConfig);
  ~PixelTrackDumpAlpakaT() override = default;

  static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

private:
  void analyze(edm::StreamID streamID, edm::Event const& iEvent, const edm::EventSetup& iSetup) const override;
  edm::EDGetTokenT<TkSoAHost> tokenSoATrack_;
  edm::EDGetTokenT<VertexSoAHost> tokenSoAVertex_;
};

template <typename TrackerTraits>
PixelTrackDumpAlpakaT<TrackerTraits>::PixelTrackDumpAlpakaT(const edm::ParameterSet& iConfig) {
  tokenSoATrack_ = consumes(iConfig.getParameter<edm::InputTag>("pixelTrackSrc"));
  tokenSoAVertex_ = consumes(iConfig.getParameter<edm::InputTag>("pixelVertexSrc"));
}

template <typename TrackerTraits>
void PixelTrackDumpAlpakaT<TrackerTraits>::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("pixelTrackSrc", edm::InputTag("pixelTracksAlpaka"));
  desc.add<edm::InputTag>("pixelVertexSrc", edm::InputTag("pixelVerticesAlpaka"));
  descriptions.addWithDefaultLabel(desc);
}

template <typename TrackerTraits>
void PixelTrackDumpAlpakaT<TrackerTraits>::analyze(edm::StreamID streamID,
                                                   edm::Event const& iEvent,
                                                   const edm::EventSetup& iSetup) const {
  auto const& tracks = iEvent.get(tokenSoATrack_);
  assert(tracks.view().quality());
  assert(tracks.view().chi2());
  assert(tracks.view().nLayers());
  assert(tracks.view().eta());
  assert(tracks.view().pt());
  assert(tracks.view().state());
  assert(tracks.view().covariance());
  assert(tracks.view().nTracks());

  auto const& vertices = iEvent.get(tokenSoAVertex_);
  assert(vertices.view().idv());
  assert(vertices.view().zv());
  assert(vertices.view().wv());
  assert(vertices.view().chi2());
  assert(vertices.view().ptv2());
  assert(vertices.view().ndof());
  assert(vertices.view().sortInd());
  assert(vertices.view().nvFinal());
}
using PixelTrackDumpAlpakaPhase1 = PixelTrackDumpAlpakaT<pixelTopology::Phase1>;
using PixelTrackDumpAlpakaPhase2 = PixelTrackDumpAlpakaT<pixelTopology::Phase2>;

DEFINE_FWK_MODULE(PixelTrackDumpAlpakaPhase1);
DEFINE_FWK_MODULE(PixelTrackDumpAlpakaPhase2);
