import FWCore.ParameterSet.Config as cms
from HeterogeneousCore.AlpakaCore.functions import *

from RecoLocalTracker.SiStripRecHitConverter.StripCPEfromTrackAngle_cfi import *
from RecoLocalTracker.SiStripRecHitConverter.SiStripRecHitMatcher_cfi import *
from RecoTracker.TransientTrackingRecHit.TransientTrackingRecHitBuilder_cfi import *
import RecoTracker.TransientTrackingRecHit.TransientTrackingRecHitBuilder_cfi
myTTRHBuilderWithoutAngle = RecoTracker.TransientTrackingRecHit.TransientTrackingRecHitBuilder_cfi.ttrhbwr.clone(
    StripCPE = 'Fake',
    ComponentName = 'PixelTTRHBuilderWithoutAngle'
)
from RecoTracker.TkSeedingLayers.PixelLayerTriplets_cfi import *
from RecoTracker.TkSeedingLayers.TTRHBuilderWithoutAngle4PixelTriplets_cfi import *
from RecoTracker.PixelTrackFitting.pixelFitterByHelixProjections_cfi import pixelFitterByHelixProjections
from RecoTracker.PixelTrackFitting.pixelNtupletsFitter_cfi import pixelNtupletsFitter
from RecoTracker.PixelTrackFitting.pixelTrackFilterByKinematics_cfi import pixelTrackFilterByKinematics
from RecoTracker.PixelTrackFitting.pixelTrackCleanerBySharedHits_cfi import pixelTrackCleanerBySharedHits
from RecoTracker.PixelTrackFitting.pixelTracks_cfi import pixelTracks as _pixelTracks
from RecoTracker.TkTrackingRegions.globalTrackingRegion_cfi import globalTrackingRegion as _globalTrackingRegion
from RecoTracker.TkTrackingRegions.globalTrackingRegionFromBeamSpot_cfi import globalTrackingRegionFromBeamSpot as _globalTrackingRegionFromBeamSpot
from RecoTracker.TkHitPairs.hitPairEDProducer_cfi import hitPairEDProducer as _hitPairEDProducer
from RecoTracker.PixelSeeding.pixelTripletHLTEDProducer_cfi import pixelTripletHLTEDProducer as _pixelTripletHLTEDProducer
from RecoTracker.PixelLowPtUtilities.ClusterShapeHitFilterESProducer_cfi import *
import RecoTracker.PixelLowPtUtilities.LowPtClusterShapeSeedComparitor_cfi
from RecoTracker.FinalTrackSelectors.trackAlgoPriorityOrder_cfi import trackAlgoPriorityOrder

# Eras
from Configuration.Eras.Modifier_trackingLowPU_cff import trackingLowPU
from Configuration.Eras.Modifier_run3_common_cff import run3_common

# seeding layers
from RecoTracker.IterativeTracking.InitialStep_cff import initialStepSeedLayers, initialStepHitDoublets, _initialStepCAHitQuadruplets

# TrackingRegion
pixelTracksTrackingRegions = _globalTrackingRegion.clone()
trackingLowPU.toReplaceWith(pixelTracksTrackingRegions, _globalTrackingRegionFromBeamSpot.clone())


# Pixel quadruplets tracking
pixelTracksSeedLayers = initialStepSeedLayers.clone(
    BPix = dict(HitProducer = "siPixelRecHitsPreSplitting"),
    FPix = dict(HitProducer = "siPixelRecHitsPreSplitting")
)

pixelTracksHitDoublets = initialStepHitDoublets.clone(
    clusterCheck = "",
    seedingLayers = "pixelTracksSeedLayers",
    trackingRegions = "pixelTracksTrackingRegions"
)

pixelTracksHitQuadruplets = _initialStepCAHitQuadruplets.clone(
    doublets = "pixelTracksHitDoublets",
    SeedComparitorPSet = dict(clusterShapeCacheSrc = 'siPixelClusterShapeCachePreSplitting')
)

pixelTracks = _pixelTracks.clone(
    SeedingHitSets = "pixelTracksHitQuadruplets"
)

pixelTracksTask = cms.Task(
    pixelTracksTrackingRegions,
    pixelFitterByHelixProjections,
    pixelTrackFilterByKinematics,
    pixelTracksSeedLayers,
    pixelTracksHitDoublets,
    pixelTracksHitQuadruplets,
    pixelTracks
)

pixelTracksSequence = cms.Sequence(pixelTracksTask)


# Pixel triplets for trackingLowPU
pixelTracksHitTriplets = _pixelTripletHLTEDProducer.clone(
    doublets = "pixelTracksHitDoublets",
    produceSeedingHitSets = True,
    SeedComparitorPSet = RecoTracker.PixelLowPtUtilities.LowPtClusterShapeSeedComparitor_cfi.LowPtClusterShapeSeedComparitor.clone(
        clusterShapeCacheSrc = "siPixelClusterShapeCachePreSplitting"
    )
)

trackingLowPU.toModify(pixelTracks,
    SeedingHitSets = "pixelTracksHitTriplets"
)

_pixelTracksTask_lowPU = pixelTracksTask.copy()
_pixelTracksTask_lowPU.replace(pixelTracksHitQuadruplets, pixelTracksHitTriplets)
trackingLowPU.toReplaceWith(pixelTracksTask, _pixelTracksTask_lowPU)

# Phase 2 modifier
from Configuration.Eras.Modifier_phase2_tracker_cff import phase2_tracker
# HIon modifiers
from Configuration.ProcessModifiers.pp_on_AA_cff import pp_on_AA

######################################################################

### Alpaka Pixel Track Reco

from Configuration.ProcessModifiers.alpaka_cff import alpaka

# pixel tracks SoA producer on the device
from RecoTracker.PixelSeeding.caHitNtupletAlpakaPhase1_cfi import caHitNtupletAlpakaPhase1 as _pixelTracksAlpakaPhase1
from RecoTracker.PixelSeeding.caHitNtupletAlpakaPhase2_cfi import caHitNtupletAlpakaPhase2 as _pixelTracksAlpakaPhase2
from RecoTracker.PixelSeeding.caHitNtupletAlpakaHIonPhase1_cfi import caHitNtupletAlpakaHIonPhase1 as _pixelTracksAlpakaHIonPhase1
from RecoTracker.PixelSeeding.caHitNtupletAlpakaPhase2OT_cfi import caHitNtupletAlpakaPhase2OT as _pixelTracksAlpakaPhase2Extended

pixelTracksHighPtAlpaka = _pixelTracksAlpakaPhase1.clone(
    avgHitsPerTrack    = 4.6,      
    avgCellsPerHit     = 13,
    avgCellsPerCell    = 0.0268, 
    avgTracksPerCell   = 0.0123, 
    maxNumberOfDoublets = str(512*1024),    # could be lowered to 315k, keeping the same for a fair comparison with master
    maxNumberOfTuples   = str(32 * 1024),   # this couul be much lower (2.1k, these are quads)
)

phase2_tracker.toReplaceWith(pixelTracksHighPtAlpaka,_pixelTracksAlpakaPhase2.clone())

def _modifyForPPonAAandNotPhase2(producer):
    nPairs = int(len(producer.geometry.pairGraph) / 2)
    producer.maxNumberOfDoublets = str(6 * 512 *1024)    # this could be 2.3M
    producer.maxNumberOfTuples = str(256 * 1024)         # this could be 4.7
    producer.avgHitsPerTrack = 5.0
    producer.avgCellsPerHit = 40
    producer.avgCellsPerCell = 0.07                      # with maxNumberOfDoublets ~= 3.14M; 0.02  for HLT HI on 2024 HI Data 
    producer.avgTracksPerCell = 0.03                     # with maxNumberOfDoublets ~= 3.14M; 0.005 for HLT HI on 2024 HI Data
    producer.cellZ0Cut = 8.0                             # setup currenlty used @ HLT (was 10.0) 
    producer.geometry.ptCuts = [0.5] * nPairs            # setup currenlty used @ HLT (was 0.0) 

(pp_on_AA & ~phase2_tracker).toModify(pixelTracksHighPtAlpaka, _modifyForPPonAAandNotPhase2)





from Configuration.ProcessModifiers.phase2CAExtension_cff import phase2CAExtension
phase2CAExtension.toReplaceWith(pixelTracksHighPtAlpaka,_pixelTracksAlpakaPhase2Extended.clone(
    hitMask = "siPixelRecHitsExtendedPreSplittingAlpaka",
    pixelRecHitSrc = "siPixelRecHitsExtendedPreSplittingAlpaka",
))

# pixel tracks SoA producer on the cpu, for validation
pixelTracksHighPtAlpakaSerial = makeSerialClone(pixelTracksHighPtAlpaka,
    pixelRecHitSrc = 'siPixelRecHitsPreSplittingAlpakaSerial'
)

phase2CAExtension.toModify(pixelTracksHighPtAlpakaSerial,
                           pixelRecHitSrc = 'siPixelRecHitsExtendedPreSplittingAlpakaSerial'
                           )

# pixel tracks SoA merger
from RecoTracker.PixelSeeding.pixelTracksMaskingSoA_cfi import pixelTracksMaskingSoA as _pixelTracksMaskingSoA

pixelTracksHighPtMaskingSoA = _pixelTracksMaskingSoA.clone(
    tracksSoASrc = "pixelTracksHighPtAlpaka",
)

pixelTracksLowPtAlpaka = _pixelTracksAlpakaPhase1.clone(
    avgHitsPerTrack    = 4.6,
    avgCellsPerHit     = 13,
    avgCellsPerCell    = 0.0268,
    avgTracksPerCell   = 0.0123,
    maxNumberOfDoublets = str(512*1024),    # could be lowered to 315k, keeping the same for a fair comparison with master
    maxNumberOfTuples   = str(32 * 1024),   # this couul be much lower (2.1k, these are quads)
)

phase2CAExtension.toReplaceWith(pixelTracksLowPtAlpaka,_pixelTracksAlpakaPhase2Extended.clone(
    hitMask = "pixelTracksHighPtMaskingSoA",
    pixelRecHitSrc = "siPixelRecHitsExtendedPreSplittingAlpaka",
    ptmin              = 0.3,
    trackQualityCuts = cms.PSet(
        maxChi2 = cms.double(5),
        maxChi2Quintuplets = cms.double(3),
        maxChi2TripletsOrQuadruplets = cms.double(1),
        maxTip = cms.double(0.3),
        maxZip = cms.double(12),
        minPt = cms.double(0.3)
    ),
))

# pixel tracks SoA producer on the cpu, for validation
pixelTracksLowPtAlpakaSerial = makeSerialClone(pixelTracksLowPtAlpaka,
    pixelRecHitSrc = 'siPixelRecHitsPreSplittingAlpakaSerial'
)

phase2CAExtension.toModify(pixelTracksLowPtAlpakaSerial,
                            hitMask = "pixelTracksHighPtMaskingSoA",
                            pixelRecHitSrc = "siPixelRecHitsExtendedPreSplittingAlpaka",
                            ptmin  = 0.3,
                            trackQualityCuts = cms.PSet(
                                maxChi2 = cms.double(5),
                                maxChi2Quintuplets = cms.double(3),
                                maxChi2TripletsOrQuadruplets = cms.double(1),
                                maxTip = cms.double(0.3),
                                maxZip = cms.double(12),
                                minPt = cms.double(0.3)
                            ),
                           )

# legacy pixel tracks from SoA
from  RecoTracker.PixelTrackFitting.pixelTrackProducerFromSoAAlpaka_cfi import pixelTrackProducerFromSoAAlpaka as _pixelTrackProducerFromSoAAlpaka

(alpaka & ~phase2CAExtension).toReplaceWith(pixelTracks, _pixelTrackProducerFromSoAAlpaka.clone(
    pixelRecHitLegacySrc = "siPixelRecHitsPreSplitting",
))

# pixel tracks SoA merger
from RecoTracker.PixelSeeding.pixelTracksSoAMerger_cfi import pixelTracksSoAMerger as _pixelTracksSoAMerger

pixelTracksLowPtSoAMerger = _pixelTracksSoAMerger.clone()

pixelTracksLowPtMaskingSoA = _pixelTracksMaskingSoA.clone(
    recHitsMaskSoASrc = "pixelTracksHighPtMaskingSoA",
    tracksSoASrc = "pixelTracksLowPtAlpaka",
)

pixelTracksDisplHighPtAlpaka = _pixelTracksAlpakaPhase1.clone(
    avgHitsPerTrack    = 4.6,
    avgCellsPerHit     = 13,
    avgCellsPerCell    = 0.0268,
    avgTracksPerCell   = 0.0123,
    maxNumberOfDoublets = str(512*1024),    # could be lowered to 315k, keeping the same for a fair comparison with master
    maxNumberOfTuples   = str(32 * 1024),   # this couul be much lower (2.1k, these are quads)
)

phase2CAExtension.toReplaceWith(pixelTracksDisplHighPtAlpaka,_pixelTracksAlpakaPhase2Extended.clone(
    hitMask = "pixelTracksLowPtMaskingSoA",
    pixelRecHitSrc = "siPixelRecHitsExtendedPreSplittingAlpaka",
    trackQualityCuts = cms.PSet(
        maxChi2 = cms.double(5),
        maxChi2Quintuplets = cms.double(3),
        maxChi2TripletsOrQuadruplets = cms.double(1),
        maxTip = cms.double(10.0),
        maxZip = cms.double(12),
        minPt = cms.double(0.9)
    ),
))

pixelTracksDisplHighPtAlpakaSerial = makeSerialClone(pixelTracksDisplHighPtAlpaka,
    pixelRecHitSrc = 'siPixelRecHitsPreSplittingAlpakaSerial'
)

phase2CAExtension.toModify(pixelTracksDisplHighPtAlpakaSerial,
                           hitMask = "pixelTracksLowPtMaskingSoA",
                           pixelRecHitSrc = 'siPixelRecHitsExtendedPreSplittingAlpakaSerial',
                           trackQualityCuts = cms.PSet(
                               maxChi2 = cms.double(5),
                               maxChi2Quintuplets = cms.double(3),
                               maxChi2TripletsOrQuadruplets = cms.double(1),
                               maxTip = cms.double(10.0),
                               maxZip = cms.double(12),
                               minPt = cms.double(0.9)
                           ),
                           )

pixelTracksDisplHighPtSoAMerger = _pixelTracksSoAMerger.clone(
    # inputTkSoA1 = "pixelTracksLowPtSoAMerger",
    # inputTkSoA2 = "pixelTracksDisplHighPtAlpaka",
)

pixelTracksDisplHighPtMaskingSoA = _pixelTracksMaskingSoA.clone(
    recHitsMaskSoASrc = "pixelTracksLowPtMaskingSoA",
    tracksSoASrc = "pixelTracksDisplHighPtAlpaka",
)



pixelTracksDisplLowPtAlpaka = _pixelTracksAlpakaPhase1.clone(
    avgHitsPerTrack    = 4.6,
    avgCellsPerHit     = 13,
    avgCellsPerCell    = 0.0268,
    avgTracksPerCell   = 0.0123,
    maxNumberOfDoublets = str(512*1024),    # could be lowered to 315k, keeping the same for a fair comparison with master
    maxNumberOfTuples   = str(32 * 1024),   # this couul be much lower (2.1k, these are quads)
)

phase2CAExtension.toReplaceWith(pixelTracksDisplLowPtAlpaka,_pixelTracksAlpakaPhase2Extended.clone(
    hitMask = "pixelTracksDisplHighPtMaskingSoA",
    pixelRecHitSrc = "siPixelRecHitsExtendedPreSplittingAlpaka",
    ptmin = 0.3,
    trackQualityCuts = cms.PSet(
        maxChi2 = cms.double(5),
        maxChi2Quintuplets = cms.double(3),
        maxChi2TripletsOrQuadruplets = cms.double(1),
        maxTip = cms.double(10.0),
        maxZip = cms.double(12),
        minPt = cms.double(0.3)
    ),
))

pixelTracksDisplLowPtAlpakaSerial = makeSerialClone(pixelTracksDisplLowPtAlpaka,
    pixelRecHitSrc = 'siPixelRecHitsPreSplittingAlpakaSerial'
)

phase2CAExtension.toModify(pixelTracksDisplLowPtAlpakaSerial,
                           hitMask = "pixelTracksDisplHighPtMaskingSoA",
                           pixelRecHitSrc = 'siPixelRecHitsExtendedPreSplittingAlpakaSerial',
                           ptmin = 0.3,
                           trackQualityCuts = cms.PSet(
                               maxChi2 = cms.double(5),
                               maxChi2Quintuplets = cms.double(3),
                               maxChi2TripletsOrQuadruplets = cms.double(1),
                               maxTip = cms.double(10.0),
                               maxZip = cms.double(12),
                               minPt = cms.double(0.3)
                           ),
                           )

pixelTracksSoAMerger = _pixelTracksSoAMerger.clone(
    # inputTkSoA1 = "pixelTracksDisplHighPtSoAMerger",
    # inputTkSoA2 = "pixelTracksDisplLowPtAlpaka",
)


phase2CAExtension.toReplaceWith(pixelTracks, _pixelTrackProducerFromSoAAlpaka.clone(
    pixelRecHitLegacySrc = "siPixelRecHitsPreSplitting",
    beamSpot = cms.InputTag("offlineBeamSpot"),
    minNumberOfHits = cms.int32(0),
    minQuality = cms.string('tight'),
    trackSrc = cms.InputTag("pixelTracksSoAMerger"),
    outerTrackerRecHitSrc = cms.InputTag("siPhase2RecHits"),
    outerTrackerRecHitSoAConverterSrc = cms.InputTag("phase2OTRecHitsSoAConverter"),
    useOTExtension = cms.bool(True),
    requireQuadsFromConsecutiveLayers = cms.bool(True)
))

alpaka.toReplaceWith(pixelTracksTask, cms.Task(
    # Build the highPt pixel ntuplets and the pixel tracks in SoA format with alpaka on the device
    pixelTracksHighPtAlpaka,
    # Build the highPt pixel ntuplets and the pixel tracks in SoA format with alpaka on the cpu (if requested by the validation)
    pixelTracksHighPtAlpakaSerial,
    # Updates the TrackingRecHitsMasking collection for next iteration
    pixelTracksHighPtMaskingSoA,
    # Build the lowPt pixel ntuplets and the pixel tracks in SoA format with alpaka on the device
    pixelTracksLowPtAlpaka,
    # Build the lowPt pixel ntuplets and the pixel tracks in SoA format with alpaka on the cpu (if requested by the validation)
    pixelTracksLowPtAlpakaSerial,
    # # Merge the produced SoAs directly
    # pixelTracksLowPtSoAMerger,
    # Updates the TrackingRecHitsMasking collection for next iteration
    pixelTracksLowPtMaskingSoA,

    # Build the lowPt pixel ntuplets and the pixel tracks in SoA format with alpaka on the device
    pixelTracksDisplHighPtAlpaka,
    # Build the lowPt pixel ntuplets and the pixel tracks in SoA format with alpaka on the cpu (if requested by the validation)
    pixelTracksDisplHighPtAlpakaSerial,
    # # Merge the produced SoAs directly
    # pixelTracksDisplHighPtSoAMerger,
    # Updates the TrackingRecHitsMasking collection for next iteration
    pixelTracksDisplHighPtMaskingSoA,

    # Build the lowPt pixel ntuplets and the pixel tracks in SoA format with alpaka on the device
    pixelTracksDisplLowPtAlpaka,
    # Build the lowPt pixel ntuplets and the pixel tracks in SoA format with alpaka on the cpu (if requested by the validation)
    pixelTracksDisplLowPtAlpakaSerial,

    # Merge the produced SoAs directly
    pixelTracksSoAMerger,
    # Convert the pixel tracks from SoA to legacy format
    pixelTracks)
)





# from Configuration.ProcessModifiers.phase2CAExtension_cff import phase2CAExtension
# phase2CAExtension.toReplaceWith(pixelTracksHighPtAlpaka,_pixelTracksAlpakaPhase2Extended.clone(
#     hitMask = "siPixelRecHitsExtendedPreSplittingAlpaka",
#     pixelRecHitSrc = "siPixelRecHitsExtendedPreSplittingAlpaka",
# ))

# # pixel tracks SoA producer on the cpu, for validation
# pixelTracksHighPtAlpakaSerial = makeSerialClone(pixelTracksHighPtAlpaka,
#     pixelRecHitSrc = 'siPixelRecHitsPreSplittingAlpakaSerial'
# )

# phase2CAExtension.toModify(pixelTracksHighPtAlpakaSerial,
#                            pixelRecHitSrc = 'siPixelRecHitsExtendedPreSplittingAlpakaSerial'
#                            )

# # legacy pixel tracks from SoA
# from  RecoTracker.PixelTrackFitting.pixelTrackProducerFromSoAAlpaka_cfi import pixelTrackProducerFromSoAAlpaka as _pixelTrackProducerFromSoAAlpaka

# (alpaka & ~phase2CAExtension).toReplaceWith(pixelTracks, _pixelTrackProducerFromSoAAlpaka.clone(
#     pixelRecHitLegacySrc = "siPixelRecHitsPreSplitting",
# ))

# phase2CAExtension.toReplaceWith(pixelTracks, _pixelTrackProducerFromSoAAlpaka.clone(
#     pixelRecHitLegacySrc = "siPixelRecHitsPreSplitting",
#     beamSpot = cms.InputTag("offlineBeamSpot"),
#     minNumberOfHits = cms.int32(0),
#     minQuality = cms.string('tight'),
#     trackSrc = cms.InputTag("pixelTracksHighPtAlpaka"),
#     outerTrackerRecHitSrc = cms.InputTag("siPhase2RecHits"),
#     outerTrackerRecHitSoAConverterSrc = cms.InputTag("phase2OTRecHitsSoAConverter"),
#     useOTExtension = cms.bool(True),
#     requireQuadsFromConsecutiveLayers = cms.bool(True)
# ))

# alpaka.toReplaceWith(pixelTracksTask, cms.Task(
#     # Build the highPt pixel ntuplets and the pixel tracks in SoA format with alpaka on the device
#     pixelTracksHighPtAlpaka,
#     # Build the highPt pixel ntuplets and the pixel tracks in SoA format with alpaka on the cpu (if requested by the validation)
#     pixelTracksHighPtAlpakaSerial,
#     # Convert the pixel tracks from SoA to legacy format
#     pixelTracks)
# )