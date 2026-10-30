#ifndef SimTracker_Common_TruthGraphParticleSelector_h
#define SimTracker_Common_TruthGraphParticleSelector_h
/* \class TruthGraphParticleSelector
 *
 * \author Giuseppe Cerati, INFN
 *
 *  $Date: 2013/05/14 15:46:46 $
 *  $Revision: 1.5.4.2 $
 *
 */
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"
#include "DataFormats/Math/interface/PtEtaPhiMass.h"
#include "DataFormats/Math/interface/deltaPhi.h"
#include "SimDataFormats/TruthInfo/interface/Graph.h"
#include "SimDataFormats/TruthInfo/interface/Particle.h"
#include "SimDataFormats/TruthInfo/interface/Vertex.h"
#include "SimDataFormats/TruthInfo/interface/LogicalGraphHitIndex.h"

class TruthGraphParticleSelector {
public:
  TruthGraphParticleSelector() {}
  TruthGraphParticleSelector(double ptMin,
                           double ptMax,
                           double minRapidity,
                           double maxRapidity,
                           double tip,
                           double lip,
                           int minHit,
                           bool signalOnly,
                           bool intimeOnly,
                           bool chargedOnly,
                           bool stableOnly,
                           const std::vector<int> &pdgId = std::vector<int>(),
                           bool invertRapidityCut = false,
                           double minPhi = -3.2,
                           double maxPhi = 3.2)
      : ptMin2_(ptMin * ptMin),
        ptMax2_(ptMax * ptMax),
        minRapidity_(minRapidity),
        maxRapidity_(maxRapidity),
        meanPhi_((minPhi + maxPhi) / 2.),
        rangePhi_((maxPhi - minPhi) / 2.),
        tip2_(tip * tip),
        lip_(lip),
        minHit_(minHit),
        signalOnly_(signalOnly),
        intimeOnly_(intimeOnly),
        chargedOnly_(chargedOnly),
        stableOnly_(stableOnly),
        pdgId_(pdgId),
        invertRapidityCut_(invertRapidityCut) {
    if (minPhi >= maxPhi) {
      throw cms::Exception("Configuration")
          << "TruthGraphParticleSelector: minPhi (" << minPhi << ") must be smaller than maxPhi (" << maxPhi
          << "). The range is constructed from minPhi to maxPhi around their "
             "average.";
    }
    if (minPhi >= M_PI) {
      throw cms::Exception("Configuration") << "TruthGraphParticleSelector: minPhi (" << minPhi
                                            << ") must be smaller than PI. The range is constructed from minPhi "
                                               "to maxPhi around their average.";
    }
    if (maxPhi <= -M_PI) {
      throw cms::Exception("Configuration") << "TruthGraphParticleSelector: maxPhi (" << maxPhi
                                            << ") must be larger than -PI. The range is constructed from minPhi "
                                               "to maxPhi around their average.";
    }
  }

  bool isCharged(const truth::Particle * tp) const { return (tp->charge() == 0 ? false : true); }

  bool isInTime(const truth::Particle * tp) const { return (tp->data().bunchCrossing() == 0); }

  bool isSignal(const truth::Particle * tp) const {
    return (tp->isSignal());
  }

  // bool isStable(truth::Graph const& graph, uint32_t particleId) {
  //   if (particleId >= graph.nParticles())
  //     return false;

  //   auto const& tp = graph.particles()[particleId];

  //   return tp.hasGen() && tp.status == 1;
  // }

  // bool isStable(const truth::Particle * tp) { return tp->hasGen() && tp->status() == 1; }
  bool isStable(const truth::Particle * tp) const { return tp->hasGen() && tp->status() == 1; }

  /// Operator() performs the selection: e.g. if (tPSelector(tp)) {...}
  /// https://stackoverflow.com/questions/14466620/c-template-specialization-calling-methods-on-types-that-could-be-pointers-or/14466705
  bool operator()(const truth::Particle & tp, const truth::LogicalGraphHitIndex & hitIndex) const { return select(&tp, &hitIndex); }
  bool operator()(const truth::Particle * tp, const truth::LogicalGraphHitIndex * hitIndex) const { return select(tp, hitIndex); }

  bool select(const truth::Particle * tp, const truth::LogicalGraphHitIndex * hitIndex) const {
    // signal only means no PU particles
    if (signalOnly_ && !isSignal(tp))
      return false;
    // intime only means no OOT PU particles
    if (intimeOnly_ && !isInTime(tp))
      return false;

    // select only if charge!=0
    if (chargedOnly_ && !isCharged(tp))
      return false;

    // select for particle type
    if (!selectParticleType(tp)) {
      return false;
    }

    // select only stable particles
    if (stableOnly_ && !isStable(tp)) {
      return false;
    }

    return selectKinematics(tp, hitIndex);
  }

  bool selectKinematics(const truth::Particle * tp, const truth::LogicalGraphHitIndex * hitIndex) const {
    auto etaOk = [&](const truth::Particle *tp) -> bool {
      float eta = etaFromXYZ(tp->p4().px(), tp->p4().py(), tp->p4().pz());
      if (!invertRapidityCut_)
        return (eta >= minRapidity_) && (eta <= maxRapidity_);
      else
        return (eta < minRapidity_ || eta > maxRapidity_);
    };
    auto phiOk = [&](const truth::Particle *tp) {
      float dphi = deltaPhi(atan2f(tp->p4().py(), tp->p4().px()), meanPhi_);
      return dphi >= -rangePhi_ && dphi <= rangePhi_;
    };
    auto ptOk = [&](const truth::Particle *tp) {
      double pt2 = tp->p4().perp2();
      return pt2 >= ptMin2_ && pt2 <= ptMax2_;
    };
    auto const tpVert = tp->productionVertices();
    if (tpVert.empty()) {
      return false;
    }
    auto const& v = tpVert.front().position();
    // auto const& trackerHits = (hitIndex->directHits(truth::HitChannel::Tracker, tp->id())).size();
    auto const trackerHits = hitIndex->directHits(truth::HitChannel::Tracker, tp->id()).size();
    // return (tp->numberOfTrackerLayers() >= minHit_ && ptOk(tp) && etaOk(tp) && phiOk(tp) &&
    // return (trackerHits >= minHit_ && ptOk(tp) && etaOk(tp) && phiOk(tp) &&
    return (minHit_ >= 0 && trackerHits >= static_cast<decltype(trackerHits)>(minHit_) &&
            ptOk(tp) && etaOk(tp) && phiOk(tp) &&
            // vertex last to avoid to load it if not strictly necessary...
            std::abs(v.z()) <= lip_ &&
            (std::hypot(v.x(), v.y())*std::hypot(v.x(), v.y())) <= tip2_);
  }

  bool selectParticleType(const truth::Particle * tp) const {
    auto pdgid = tp->pdgId();
    if (!pdgId_.empty()) {
      for (auto id : pdgId_) {
        if (id == pdgid) {
          return true;
        }
      }
    } else {
      return true;
    }
    return false;
  }

  bool isSignalOnly() const { return signalOnly_; }
  bool isIntimeOnly() const { return intimeOnly_; }
  bool isChargedOnly() const { return chargedOnly_; }
  bool isStableOnly() const { return stableOnly_; }
  bool invertRapidityCut() const { return invertRapidityCut_; }

private:
  double ptMin2_;
  double ptMax2_;
  float minRapidity_;
  float maxRapidity_;
  float meanPhi_;
  float rangePhi_;
  double tip2_;
  double lip_;
  int minHit_;
  bool signalOnly_;
  bool intimeOnly_;
  bool chargedOnly_;
  bool stableOnly_;
  std::vector<int> pdgId_;
  bool invertRapidityCut_;
};

#include "CommonTools/UtilAlgos/interface/ParameterAdapter.h"
#include "FWCore/Framework/interface/ConsumesCollector.h"

namespace reco {
  namespace modules {

    template <>
    struct ParameterAdapter<TruthGraphParticleSelector> {
      static TruthGraphParticleSelector make(const edm::ParameterSet &cfg, edm::ConsumesCollector &iC) {
        return make(cfg);
      }

      static TruthGraphParticleSelector make(const edm::ParameterSet &cfg) {
        return TruthGraphParticleSelector(cfg.getParameter<double>("ptMin"),
                                        cfg.getParameter<double>("ptMax"),
                                        cfg.getParameter<double>("minRapidity"),
                                        cfg.getParameter<double>("maxRapidity"),
                                        cfg.getParameter<double>("tip"),
                                        cfg.getParameter<double>("lip"),
                                        cfg.getParameter<int>("minHit"),
                                        cfg.getParameter<bool>("signalOnly"),
                                        cfg.getParameter<bool>("intimeOnly"),
                                        cfg.getParameter<bool>("chargedOnly"),
                                        cfg.getParameter<bool>("stableOnly"),
                                        cfg.getParameter<std::vector<int>>("pdgId"),
                                        cfg.getParameter<bool>("invertRapidityCut"),
                                        cfg.getParameter<double>("minPhi"),
                                        cfg.getParameter<double>("maxPhi"));
      }

      static void fillPSetDescription(edm::ParameterSetDescription &desc) {
        desc.add<double>("ptMin", 0.9);
        desc.add<double>("ptMax", 1e100);
        desc.add<double>("minRapidity", -4.5);
        desc.add<double>("maxRapidity", 4.5);
        desc.add<double>("tip", 2.5);
        desc.add<double>("lip", 30.0);
        desc.add<int>("minHit", 0);
        desc.add<bool>("signalOnly", true);
        desc.add<bool>("intimeOnly", false);
        desc.add<bool>("chargedOnly", true);
        desc.add<bool>("stableOnly", false);
        desc.add<std::vector<int>>("pdgId", {});
        desc.add<bool>("invertRapidityCut", false);
        desc.add<double>("minPhi", -3.2);
        desc.add<double>("maxPhi", 3.2);
      }
    };

  }  // namespace modules
}  // namespace reco

#endif
