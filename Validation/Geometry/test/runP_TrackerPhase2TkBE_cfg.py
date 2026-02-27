import FWCore.ParameterSet.Config as cms
from Configuration.Eras.Era_Phase2C22I13M9_cff import Phase2C22I13M9
from FWCore.ParameterSet.VarParsing import VarParsing
import sys, re

from FWCore.PythonFramework.CmsRun import CmsRun

process = cms.Process("PROD",Phase2C22I13M9)

options = VarParsing('analysis')
options.register('geom',             #name
                 'ExtendedRun4D121',      #default value
                 VarParsing.multiplicity.singleton,   # kind of options
                 VarParsing.varType.string,           # type of option
                 "Select the geometry to be studied"  # help message
                )

options.register('label',         #name
                 'Tracker',              #default value
                 VarParsing.multiplicity.singleton,   # kind of options
                 VarParsing.varType.string,           # type of option
                 "Select the label to be used to create output files. Default to tracker. If multiple components are selected, it defaults to the join of all components, with '_' as separator."  # help message
                )

options.setDefault('inputFiles', ['file:single_neutrino_random.root'])

options.parseArguments()

process.load("SimGeneral.HepPDTESSource.pythiapdt_cfi")

#Geometry
#
process.load("Configuration.Geometry.GeometryExtendedRun4D121Reco_cff")

#Magnetic Field
#
process.load("Configuration.StandardSequences.MagneticField_38T_cff")

# Output of events, etc...
#
# Explicit note : since some histos/tree might be dumped directly,
#                 better NOT use PoolOutputModule !
# Detector simulation (Geant4-based)
#
process.load("SimG4Core.Application.g4SimHits_cfi")

process.load("IOMC.RandomEngine.IOMC_cff")
process.RandomNumberGeneratorService.g4SimHits.initialSeed = 9876

from Validation.Geometry.plot_utils import _LABELS2COMPS

_ALLOWED_LABELS = _LABELS2COMPS.keys()

process.MessageLogger = cms.Service(
    "MessageLogger",
    destinations   = cms.untracked.vstring('info'),
    categories = cms.untracked.vstring(['logMsg','MaterialBudget']),
    info = cms.untracked.PSet(
        threshold = cms.untracked.string('INFO'),
        filename = cms.untracked.string('Log_%s_%s' % (options.label,options.geom)),
        logMsg = cms.untracked.PSet(limit = cms.untracked.int32(-1))
        )
    )

if options.label not in _ALLOWED_LABELS:
    print("\n*** Error, '%s' not registered as a valid components to monitor." % options.label)
    print("Allowed components:", _ALLOWED_LABELS)
    raise RuntimeError("Unknown label")

_components = _LABELS2COMPS[options.label]

process.source = cms.Source("PoolSource",
    fileNames = cms.untracked.vstring('file:single_neutrino_random.root')
)

process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(-1)
)

process.p1 = cms.Path(process.g4SimHits)
process.g4SimHits.StackingAction.TrackNeutrino = cms.bool(True)
process.g4SimHits.UseMagneticField = False
process.g4SimHits.Physics.type = 'SimG4Core/Physics/DummyPhysics'
process.g4SimHits.Physics.DummyEMPhysics = True
process.g4SimHits.Physics.CutsPerRegion = False
process.g4SimHits.Watchers = cms.VPSet(cms.PSet(
    type = cms.string('MaterialBudgetAction'),
    MaterialBudgetAction = cms.PSet(
        HistosFile = cms.string('matbdg_%s_%s.root' % (options.label,
                                                       options.geom)),
        AllStepsToTree = cms.bool(True),
        HistogramList = cms.string('Tracker'),
        SelectedVolumes = cms.vstring(_components),
        TreeFile = cms.string('matbdg_tree_%s_%s.root' % (options.label,
                                                          options.geom)),
        StopAfterProcess = cms.string('None'),
        TextFile = cms.string('matbdg_%s_%s.txt' % (options.label,
                                                     options.geom))
    )
))

cmsRun = CmsRun(process)
cmsRun.run()
