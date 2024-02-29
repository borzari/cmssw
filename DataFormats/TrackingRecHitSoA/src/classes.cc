#include "DataFormats/Portable/interface/PortableHostCollectionReadRules.h"
#include "DataFormats/TrackingRecHitSoA/interface/TrackingRecHitsSoA.h"
#include "Geometry/CommonTopologies/interface/SimplePixelTopology.h"

SET_PORTABLEHOSTCOLLECTION_READ_RULES(PortableHostCollection<reco::TrackingRecHitLayout<pixelTopology::Phase1>>);
SET_PORTABLEHOSTCOLLECTION_READ_RULES(PortableHostCollection<reco::TrackingRecHitLayout<pixelTopology::Phase2>>);
SET_PORTABLEHOSTCOLLECTION_READ_RULES(PortableHostCollection<reco::TrackingRecHitLayout<pixelTopology::HIonPhase1>>);