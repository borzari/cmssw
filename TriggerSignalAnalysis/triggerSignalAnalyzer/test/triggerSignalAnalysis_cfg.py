import FWCore.ParameterSet.Config as cms
import glob, sys, os
import sigLists

###########################################################
##### Set up process #####
###########################################################

process = cms.Process ('TRIGSIG')
process.load ('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 1000

process.maxEvents = cms.untracked.PSet (
    input = cms.untracked.int32 (-1)
    # input = cms.untracked.int32 (1000)
)

lifetime = "1000"
inputList = []
if lifetime == "10": inputList = sigLists.AMSB_700_10
if lifetime == "100": inputList = sigLists.AMSB_700_100
if lifetime == "1000": inputList = sigLists.AMSB_700_1000

process.source = cms.Source ("PoolSource",
    fileNames = cms.untracked.vstring (inputList),
)

process.TFileService = cms.Service ('TFileService',
    fileName = cms.string ('outputFile' + lifetime + '.root')
)

process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, '130X_mcRun3_2022_realistic_postEE_v6', '')

###########################################################
##### Set up the producer and the end path            #####
###########################################################

process.triggerSignalAnalyzer = cms.EDAnalyzer ("triggerSignalAnalyzer",
    tracks = cms.InputTag("isolatedTracks",""),
    mets = cms.InputTag("slimmedMETs"),
    muons = cms.InputTag("slimmedMuons",""),
    jets = cms.InputTag("slimmedJets",""),
    triggers = cms.InputTag('TriggerResults','','HLT'),
    trigobjs = cms.InputTag('slimmedPatTrigger'),
)

process.myPath = cms.Path (process.triggerSignalAnalyzer)
