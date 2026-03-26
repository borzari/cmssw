#include <alpaka/alpaka.hpp>

#include <numeric>
#include <TFormula.h>
#include "CommonTools/Utils/interface/FormulaEvaluator.h"

#include "DataFormats/TrackSoA/interface/TracksHost.h"
#include "DataFormats/TrackSoA/interface/alpaka/TracksSoACollection.h"
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

#include "CAHitNtupletGenerator.h"

// #define GPU_DEBUG
// #define NTRACKS_DEBUG
// #define DUPLICATE_DEBUG

namespace ALPAKA_ACCELERATOR_NAMESPACE {

  class PixelTracksSoAMerger : public global::EDProducer<> {

  using Algo = CAHitMaskingAndMerger;

  public:
    explicit PixelTracksSoAMerger(const edm::ParameterSet& iConfig);
    ~PixelTracksSoAMerger() override = default;

    static void fillDescriptions(edm::ConfigurationDescriptions& descriptions);

  private:
    void produce(edm::StreamID streamID, device::Event& iEvent, const device::EventSetup& iSetup) const override;
    // bool checkForDuplicate(const reco::TracksSoACollection& trks, int i, int j) const;

    pixelTrack::Quality const minQuality_;
    double const matchFraction_;

    std::vector<device::EDGetToken<reco::TracksSoACollection>> inputTkSoATokenV_;
    std::vector<edm::InputTag> inputTkSoATagV_;

    const device::EDPutToken<reco::TracksSoACollection> outputTkSoAToken_;

    Algo deviceAlgo_;
  };

  PixelTracksSoAMerger::PixelTracksSoAMerger(const edm::ParameterSet& iConfig)
      : EDProducer(iConfig),
        minQuality_(pixelTrack::qualityByName(iConfig.getParameter<std::string>("minQuality"))),
        matchFraction_(iConfig.getParameter<double>("matchFraction")),
        inputTkSoATagV_(iConfig.getParameter<std::vector<edm::InputTag>>("inputTkSoAs")),
        outputTkSoAToken_(produces()) {
          for(const auto& it : inputTkSoATagV_) {
            inputTkSoATokenV_.push_back(consumes(it));
          }
          if (minQuality_ == pixelTrack::Quality::notQuality) {
            throw cms::Exception("PixelTrackConfiguration")
                << iConfig.getParameter<std::string>("minQuality") + " is not a pixelTrack::Quality";
          }
          if (minQuality_ < pixelTrack::Quality::dup) {
            throw cms::Exception("PixelTrackConfiguration")
                << iConfig.getParameter<std::string>("minQuality") + " not supported";
          }
        }

  void PixelTracksSoAMerger::fillDescriptions(edm::ConfigurationDescriptions& descriptions) {
    edm::ParameterSetDescription desc;

    desc.add<std::vector<edm::InputTag>>("inputTkSoAs", {edm::InputTag("pixelTracksHighPtAlpaka"),edm::InputTag("pixelTracksLowPtAlpaka"),edm::InputTag("pixelTracksDisplHighPtAlpaka"),edm::InputTag("pixelTracksDisplLowPtAlpaka")});
    desc.add<std::string>("minQuality", "highPurity");
    desc.add<double>("matchFraction", 0.0);

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

    std::vector<const reco::TracksSoACollection*> inputTkSoAs;
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

    // the outputTemp is also a SoA collection with the same layout as the input ones
    auto outputTemp = reco::TracksSoACollection({std::reduce(nTks.begin(), nTks.end()), std::reduce(nHits.begin(), nHits.end())}, queue);

#ifdef NTRACKS_DEBUG
    std::cout << "----------------- Merging Input Tracks -----------------\n";
    for(int i = 0; i < int(nTks.size()); ++i) std::cout << "Number of tracks input " << i+1 << ": " << nTks[i] << '\n';
    std::cout << "Total number of tracks: " << outputTemp.view().metadata().size() << '\n';
    for(int i = 0; i < int(nHits.size()); ++i) std::cout << "Number of hits input " << i+1 << ": " << nHits[i] << '\n';
    std::cout << "Total number of hits: " << outputTemp.view<::reco::TrackHitSoA>().metadata().size() << '\n'
    << "---------------------------------------------------------------------\n";
#endif

    // start from the tracks SoA, use metarecords to loop over all the columns
    auto outView = outputTemp.view();

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

      // update outputTemp hitOffsets to take into account the previous SoAs
      deviceAlgo_.updateHitOffsets(cumulNTks[nSoAsAux],cumulNTks[nSoAsAux + 1],cumulNHits[nSoAsAux],outputTemp,queue);

      // for(int i = cumulNTks[nSoAsAux]; i < cumulNTks[nSoAsAux + 1]; ++i){
      //    outputTemp.view()[i].hitOffsets() += cumulNHits[nSoAsAux];
      // }

      // copy track hits information for inp1 hits
      alpaka::memcpy(
          queue,
          cms::alpakatools::make_device_view(queue, outputTemp.view<::reco::TrackHitSoA>().id().data() + cumulNHits[nSoAsAux], nHits[nSoAsAux]),
          cms::alpakatools::make_device_view(queue, it->view<::reco::TrackHitSoA>().id().data(), nHits[nSoAsAux]));
      alpaka::memcpy(
          queue,
          cms::alpakatools::make_device_view(queue, outputTemp.view<::reco::TrackHitSoA>().detId().data() + cumulNHits[nSoAsAux], nHits[nSoAsAux]),
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

//     auto output = reco::TracksSoACollection({std::reduce(nTks.begin(), nTks.end()), std::reduce(nHits.begin(), nHits.end())}, queue);

//     int auxOutputTkIndex = 0;
//     int auxOutputHitIndex = 0;

//     for(int i = 0; i < outputTemp.view().metadata().size(); ++i){

//       if(outputTemp.view()[i].quality() < minQuality_) continue;

//       bool hasDuplicate = false;
//       for(int j = i + 1; j < outputTemp.view().metadata().size(); ++j) {
//         if(outputTemp.view()[j].quality() < minQuality_) continue;
//         hasDuplicate = checkForDuplicate(outputTemp,i,j);
//         if(hasDuplicate) break;
//       }
//       if(hasDuplicate) continue;

//       output.view()[auxOutputTkIndex].quality() = outputTemp.view()[i].quality();
//       output.view()[auxOutputTkIndex].chi2() = outputTemp.view()[i].chi2();
//       output.view()[auxOutputTkIndex].nLayers() = outputTemp.view()[i].nLayers();
//       output.view()[auxOutputTkIndex].eta() = outputTemp.view()[i].eta();
//       output.view()[auxOutputTkIndex].pt() = outputTemp.view()[i].pt();
//       for(int k = 0; k < 5; ++k) output.view()[auxOutputTkIndex].state()[k] = outputTemp.view()[i].state()[k];
//       for(int k = 0; k < 15; ++k) output.view()[auxOutputTkIndex].covariance()[k] = outputTemp.view()[i].covariance()[k];
//       if (auxOutputTkIndex != 0) {output.view()[auxOutputTkIndex].hitOffsets() = output.view()[auxOutputTkIndex - 1].hitOffsets() + ::reco::nHits(outputTemp.view(),i);}
//       else {output.view()[auxOutputTkIndex].hitOffsets() = ::reco::nHits(outputTemp.view(),i);}

//       int auxHitOffsetsIdBegin = 0;
//       if (i > 0) auxHitOffsetsIdBegin = outputTemp.view()[i - 1].hitOffsets();

//       int auxHitOffsetsIdEnd = outputTemp.view()[i].hitOffsets();
//       if (i > 0) auxHitOffsetsIdEnd = outputTemp.view()[i].hitOffsets();

//       for(int k = int(auxHitOffsetsIdBegin); k < int(auxHitOffsetsIdEnd); ++k){
//         output.view<::reco::TrackHitSoA>()[auxOutputHitIndex].id() = outputTemp.view<::reco::TrackHitSoA>()[k].id();
//         output.view<::reco::TrackHitSoA>()[auxOutputHitIndex].detId() = outputTemp.view<::reco::TrackHitSoA>()[k].detId();
//         ++auxOutputHitIndex;
//       }

//       ++auxOutputTkIndex;
//     }
//     output.view().nTracks() = auxOutputTkIndex;

// #ifdef NTRACKS_DEBUG
//     std::cout << "----------------- Removing duplicates -----------------\n";
//     std::cout << "Aux total number of tracks: " << auxOutputTkIndex << '\n';
//     std::cout << "Aux total number of hits: " << auxOutputHitIndex << '\n';
//     std::cout << "Total number of tracks: " << output.view().nTracks() << '\n';
//     std::cout << "Total number of hits: " << output.view()[auxOutputTkIndex - 1].hitOffsets() << '\n';
//     std::cout << "Aux total and Total should be the same" << '\n'
//     << "---------------------------------------------------------------------\n";
// #endif

    // calculate the total number of tracks and hits
    int totTracks = std::reduce(nTks.begin(), nTks.end());
    int totHits = std::reduce(nHits.begin(), nHits.end());

    // emplace the merged SoA collection in the event
    // iEvent.emplace(outputTkSoAToken_, std::move(output));
    iEvent.emplace(outputTkSoAToken_, deviceAlgo_.makeFilteredTracks(totTracks,totHits,outputTemp,minQuality_,matchFraction_,queue));
  }

//   bool PixelTracksSoAMerger::checkForDuplicate(const reco::TracksSoACollection& trks, int i, int j) const{
//     bool isDuplicate = false;
//     if(::reco::nHits(trks.view(),i) == ::reco::nHits(trks.view(),j)){
//       int matchedHits = 0;
//       for(int k = 0; k < ::reco::nHits(trks.view(),i); ++k){
//         int auxHitOffsetsId = 0;
//         if (i > 0) auxHitOffsetsId = trks.view()[i - 1].hitOffsets();
//         if(trks.view<::reco::TrackHitSoA>()[auxHitOffsetsId + k].id() == trks.view<::reco::TrackHitSoA>()[trks.view()[j - 1].hitOffsets() + k].id()) ++matchedHits;
// #ifdef DUPLICATE_DEBUG
//         if(trks.view<::reco::TrackHitSoA>()[auxHitOffsetsId + k].id() == trks.view<::reco::TrackHitSoA>()[trks.view()[j - 1].hitOffsets() + k].id()) std::cout << "Increased matchedHits" << std::endl;
// #endif
//       }
//       if(double(matchedHits) / double(::reco::nHits(trks.view(),i)) > matchFraction_) isDuplicate = true;
// #ifdef DUPLICATE_DEBUG
//         if(isDuplicate) std::cout << "isDuplicate is set to TRUE" << "\n -------------------------------------" << std::endl;
// #endif
//       return isDuplicate;
//     }
//     else {
//       return isDuplicate;
//     }
//   }

}  // namespace ALPAKA_ACCELERATOR_NAMESPACE

#include "HeterogeneousCore/AlpakaCore/interface/alpaka/MakerMacros.h"
DEFINE_FWK_ALPAKA_MODULE(PixelTracksSoAMerger);
