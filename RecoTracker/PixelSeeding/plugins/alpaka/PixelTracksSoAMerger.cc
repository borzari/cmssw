#include <alpaka/alpaka.hpp>

#include <numeric>
#include <TFormula.h>
#include "CommonTools/Utils/interface/FormulaEvaluator.h"

#include "DataFormats/TrackSoA/interface/TracksHost.h"
#include "DataFormats/TrackSoA/interface/alpaka/TracksSoACollection.h"
#include "DataFormats/TrackSoA/interface/TracksHost.h"
#include "DataFormats/TrackSoA/interface/TracksDevice.h"
#include "DataFormats/TrackingRecHitSoA/interface/alpaka/TrackingRecHitsSoACollection.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/global/EDProducer.h"
#include "FWCore/Utilities/interface/ESGetToken.h"
#include "FWCore/Utilities/interface/InputTag.h"
#include "FWCore/Utilities/interface/RunningAverage.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EDGetToken.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EDPutToken.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/Event.h"
#include "HeterogeneousCore/AlpakaCore/interface/alpaka/EventSetup.h"
#include "HeterogeneousCore/AlpakaInterface/interface/config.h"
#include "MagneticField/Records/interface/IdealMagneticFieldRecord.h"
#include "RecoTracker/TkMSParametrization/interface/PixelRecoUtilities.h"
#include "HeterogeneousCore/AlpakaInterface/interface/memory.h"

// #define GPU_DEBUG
#define NTRACKS_DEBUG

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  class PixelTracksSoAMerger : public global::EDProducer<> {
  public:
    explicit PixelTracksSoAMerger(const edm::ParameterSet& iConfig);
    ~PixelTracksSoAMerger() override = default;

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

  private:
    void produce(edm::StreamID streamID, device::Event& iEvent, const device::EventSetup& iSetup) const override;

    std::vector<edm::EDGetTokenT<reco::TracksHost>> inputTkSoATokenV_;
    std::vector<edm::InputTag> inputTkSoATagV_;

    const edm::EDPutTokenT<reco::TracksSoACollection> outputTkSoAToken_;
  };

  PixelTracksSoAMerger::PixelTracksSoAMerger(const edm::ParameterSet& iConfig)
      : EDProducer(iConfig),
        inputTkSoATagV_(iConfig.getParameter<std::vector<edm::InputTag>>("inputTkSoAs")),
        outputTkSoAToken_(produces()) {
          for(const auto& it : inputTkSoATagV_) {
            inputTkSoATokenV_.push_back(consumes<reco::TracksHost>(it));
          }
        }

  void PixelTracksSoAMerger::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;

    desc.add<std::vector<edm::InputTag>>("inputTkSoAs", {edm::InputTag("pixelTracksHighPtAlpaka"),edm::InputTag("pixelTracksLowPtAlpaka"),edm::InputTag("pixelTracksDisplHighPtAlpaka"),edm::InputTag("pixelTracksDisplLowPtAlpaka")});

    descriptions.addWithDefaultLabel(desc);
  }

  namespace {
    // This utility unrolls the SoA columns (tuples) at compile time, calling the provided functor 'f'
    // once for each element. The index is passed as a std::integral_constant so it
    // is available at compile time.
    template <typename F, std::size_t... Is>
    void unrollColumns(F&& f, std::index_sequence<Is...>) {
      (f(std::integral_constant<std::size_t, Is>{}), ...);
    }
    // User-facing wrapper to deduce the size of the tuple and create the index sequence
    // Usage: mergeSoAColumns<NumberOfColumns>([&](auto columnIndex) { ... });
    template <std::size_t N, typename F>
    void mergeSoAColumns(F&& f) {
      unrollColumns(std::forward<F>(f), std::make_index_sequence<N>{});
    }
  }  // namespace

  void PixelTracksSoAMerger::produce(edm::StreamID streamID,
                                            device::Event& iEvent,
                                            const device::EventSetup& es) const {
    // get both Pixel and Tracker SoA collections
    auto queue = iEvent.queue();

    std::vector<const reco::TracksHost*> inputTkSoAs;
    for(const auto& it : inputTkSoATokenV_) {
      auto const& aux = iEvent.get(it);
      inputTkSoAs.push_back(&aux);
    }

    // input SoA collections have the same layout
    // each of them is made up of two SoAs:
    // - one that contains the tracks
    // - one that contains the hits associated to the tracks
    // this code merges and copy them into a new SoA collection

    std::vector<int> nTks;
    std::vector<int> nHits;

    for(const auto& it : inputTkSoAs) {
      nTks.push_back(it->view().nTracks());
      int nHitsAux = 0;
      for(int i = 0; i < it->view().nTracks(); ++i) nHitsAux = nHitsAux + ::reco::nHits(it->view(),i);
      nHits.push_back(nHitsAux);
    }

    std::vector<int> cumulNTks{0};
    std::vector<int> cumulNHits{0};

    int auxCumulNTks = 0;
    int auxCumulNHits = 0;

    for(int i = 0; i < int(nTks.size()); ++i){
      auxCumulNTks = auxCumulNTks + nTks[i];
      auxCumulNHits = auxCumulNHits + nHits[i];
      cumulNTks.push_back(auxCumulNTks);
      cumulNHits.push_back(auxCumulNHits);
    }

    // the output is also a SoA collection with the same layout as the input ones
    auto output = reco::TracksSoACollection({std::reduce(nTks.begin(), nTks.end()), std::reduce(nHits.begin(), nHits.end())}, queue);

#ifdef NTRACKS_DEBUG
    std::cout << "----------------- Merging Input Tracks -----------------\n";
    for(int i = 0; i < int(nTks.size()); ++i) std::cout << "Number of tracks input " << i+1 << ": " << nTks[i] << '\n';
    std::cout << "Total number of tracks: " << output.view().metadata().size() << '\n';
    for(int i = 0; i < int(nHits.size()); ++i) std::cout << "Number of hits input " << i+1 << ": " << nHits[i] << '\n';
    std::cout << "Total number of hits: " << output.view<::reco::TrackHitSoA>().metadata().size() << '\n'
    << "---------------------------------------------------------------------\n";
#endif

    // start from the tracks SoA, use metarecords to loop over all the columns
    auto outView = output.view();

    // start a loop here over the input SoAs to be easier to access each object
    int nSoAsAux = 0; // auxiliar index to correctly access nTks and nHits
    for(const auto& it : inputTkSoAs) {

      if(nTks[nSoAsAux] == 0) {
        nSoAsAux = nSoAsAux + 1; // still need to increase the SoA position to correctly access the cumul vectors
        continue;
      }

      auto inpTkView = it->view();

      // auxiliar for correctly memcpy-ing eigen columns
      int nEigenAux = 5; 

      // layout type (same for all views)
      using ViewType = decltype(outView);
      using LayoutType = typename ViewType::Metadata::TypeOf_Layout;

      // build descriptors (tuple of spans: one span for each column)
      auto outDesc = LayoutType::Descriptor(outView);
      auto inpTkDesc = LayoutType::ConstDescriptor(inpTkView);

      // correctly copy total of nTracks
      const int nTotal = cumulNTks[cumulNTks.size() - 1];

      // merge all columns using a compile-time loop

      // number of columns (same for all hits SoAs)
      constexpr std::size_t N = std::tuple_size_v<decltype(outDesc.buff)>;
      mergeSoAColumns<N>([&](auto columnIndex) {
        auto& outCol = std::get<columnIndex>(outDesc.buff);
        const auto& inpTkCol = std::get<columnIndex>(inpTkDesc.buff);
        // distinguish between scalar and column types
        if constexpr (std::get<columnIndex>(outDesc.columnTypes) == cms::soa::SoAColumnType::scalar) {
          // scalar type, copy the value directly
          alpaka::memcpy(queue,
                         cms::alpakatools::make_device_view(queue, outCol.data(), 1),
                         cms::alpakatools::make_device_view(queue, &nTotal, 1));

#ifdef GPU_DEBUG
          alpaka::wait(queue);
          std::cout << "Copied scalar with index " << columnIndex << '\n';
#endif
        } else if constexpr (std::get<columnIndex>(outDesc.columnTypes) == cms::soa::SoAColumnType::eigen) {
          for(int i = 0; i < nEigenAux; ++i) {
            // eigen column type, copy the whole column with the number of eigen elements
            alpaka::memcpy(queue,
                           cms::alpakatools::make_device_view(queue, outCol.data() + cumulNTks[nSoAsAux] + (i*(outCol.size() / nEigenAux)), nTks[nSoAsAux]),
                           cms::alpakatools::make_device_view(queue, inpTkCol.data() + (i*(inpTkCol.size() / nEigenAux)), nTks[nSoAsAux]));

#ifdef GPU_DEBUG
          alpaka::wait(queue);
          std::cout << "Copied eigen column with index " << columnIndex << ", " << i << '\n';
#endif
          }
          nEigenAux = nEigenAux + 10;
        } else {
          // column type, copy the whole column
          alpaka::memcpy(queue,
                         cms::alpakatools::make_device_view(queue, outCol.data() + cumulNTks[nSoAsAux], nTks[nSoAsAux]),
                         cms::alpakatools::make_device_view(queue, inpTkCol.data(), nTks[nSoAsAux]));
#ifdef GPU_DEBUG
          alpaka::wait(queue);
          std::cout << "Copied column with index " << columnIndex << '\n';
#endif
        }

      });

      // update output hitOffsets to take into account the previous SoAs
      for(int i = cumulNTks[nSoAsAux]; i < cumulNTks[nSoAsAux + 1]; ++i){
         output.view()[i].hitOffsets() += cumulNHits[nSoAsAux];
      }

      // copy track hits information for inp1 hits
      alpaka::memcpy(
          queue,
          cms::alpakatools::make_device_view(queue, output.view<::reco::TrackHitSoA>().id().data() + cumulNHits[nSoAsAux], nHits[nSoAsAux]),
          cms::alpakatools::make_device_view(queue, it->view<::reco::TrackHitSoA>().id().data(), nHits[nSoAsAux]));
      alpaka::memcpy(
          queue,
          cms::alpakatools::make_device_view(queue, output.view<::reco::TrackHitSoA>().detId().data() + cumulNHits[nSoAsAux], nHits[nSoAsAux]),
          cms::alpakatools::make_device_view(queue, it->view<::reco::TrackHitSoA>().detId().data(), nHits[nSoAsAux]));
#ifdef GPU_DEBUG
      alpaka::wait(queue);
      std::cout << "Copied track hits\n";

#endif

        nSoAsAux = nSoAsAux + 1;

      }

#ifdef GPU_DEBUG
    for(int i = 0; i < std::min(int(*(std::min_element(nTks.begin(), nTks.end()))),10); ++i) {

      std::cout << "track number: " << outView.nTracks() << std::endl;
      std::cout << "------------------------------------------------------------------------------------------" << std::endl;

      std::cout << "track quality (";
      for(int k = 1; k < int(cumulNTks.size()); ++k) std::cout << "inp" << k << " - out" << k << " -- ";
      std::cout << "): ";
      for(int k = 0; k < int(cumulNTks.size()) - 1; ++k) std::cout << pixelTrack::qualityName[int(inputTkSoAs[k]->view()[i].quality())] << " - " << pixelTrack::qualityName[int(outView[i + cumulNTks[k]].quality())] << " -- ";
      std::cout << std::endl;
      std::cout << "------------------------------------------------------------------------------------------" << std::endl;

      std::cout << "track pt (";
      for(int k = 1; k < int(cumulNTks.size()); ++k) std::cout << "inp" << k << " - out" << k << " -- ";
      std::cout << "): ";
      for(int k = 0; k < int(cumulNTks.size()) - 1; ++k) std::cout << inputTkSoAs[k]->view()[i].pt() << " - " << outView[i + cumulNTks[k]].pt() << " -- ";
      std::cout << std::endl;
      std::cout << "------------------------------------------------------------------------------------------" << std::endl;

      std::cout << "track eta (";
      for(int k = 1; k < int(cumulNTks.size()); ++k) std::cout << "inp" << k << " - out" << k << " -- ";
      std::cout << "): ";
      for(int k = 0; k < int(cumulNTks.size()) - 1; ++k) std::cout << inputTkSoAs[k]->view()[i].eta() << " - " << outView[i + cumulNTks[k]].eta() << " -- ";
      std::cout << std::endl;
      std::cout << "------------------------------------------------------------------------------------------" << std::endl;
      
      for(int j = 0; j < 5; ++j) {
        std::cout << "track state " << j << "(";
        for(int k = 1; k < int(cumulNTks.size()); ++k) std::cout << "inp" << k << " - out" << k << " -- ";
        std::cout << "): ";
        for(int k = 0; k < int(cumulNTks.size()) - 1; ++k) std::cout <<  inputTkSoAs[k]->view()[i].state()(j) << " - " << outView[i + cumulNTks[k]].state()(j) << " -- ";
        std::cout << std::endl;
      }
      std::cout << "------------------------------------------------------------------------------------------" << std::endl;

      for(int j = 0; j < 15; ++j) {
        std::cout << "track covariance " << j << "(";
        for(int k = 1; k < int(cumulNTks.size()); ++k) std::cout << "inp" << k << " - out" << k << " -- ";
        std::cout << "): ";
        for(int k = 0; k < int(cumulNTks.size()) - 1; ++k) std::cout <<  inputTkSoAs[k]->view()[i].covariance()(j) << " - " << outView[i + cumulNTks[k]].covariance()(j) << " -- ";
        std::cout << std::endl;
      }
      std::cout << "==========================================================================================" << std::endl;
    }
#endif

    // emplace the merged SoA collection in the event
    iEvent.emplace(outputTkSoAToken_, std::move(output));
  }
}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

#include "HeterogeneousCore/AlpakaCore/interface/alpaka/MakerMacros.h"
DEFINE_FWK_ALPAKA_MODULE(PixelTracksSoAMerger);
