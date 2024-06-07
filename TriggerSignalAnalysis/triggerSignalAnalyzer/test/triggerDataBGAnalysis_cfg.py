import FWCore.ParameterSet.Config as cms
import glob, sys, os
import dataBGLists

###########################################################
##### Set up process #####
###########################################################

process = cms.Process ('TRIGDBG')
process.load ('FWCore.MessageService.MessageLogger_cfi')
process.MessageLogger.cerr.FwkReport.reportEvery = 1

process.maxEvents = cms.untracked.PSet (
    input = cms.untracked.int32 (-1)
    # input = cms.untracked.int32 (10000)
)

dataset = "Muon"
datayear = "2024"
dataera = "D"

# dataset = "WToLNu"
# datayear = "2023"
# dataera = "BPix"

inputList = []
if dataset == "Muon":
    if datayear == "2022":
        if dataera == "F": inputList = dataBGLists.Muon2022F

process.source = cms.Source ("PoolSource",
    # fileNames = cms.untracked.vstring (inputList[:10]),
    # fileNames = cms.untracked.vstring ("file:00f90c0a-f66e-4bf8-9985-377caf239002.root"),
    fileNames = cms.untracked.vstring ("/store/data/Run2024D/Muon1/MINIAOD/PromptReco-v1/000/380/306/00000/1338be12-e031-499c-afcf-b17846f5733f.root"),
)

process.TFileService = cms.Service ('TFileService',
    fileName = cms.string ('outputFile' + dataset + datayear + dataera + '.root')
)

process.options.SkipEvent = cms.untracked.vstring('ProductNotFound')

gt = ""
if dataset == "Muon":
    if datayear == "2022":
        if dataera == "C" or dataera == "D" or dataera == "E": gt = "130X_dataRun3_v2"
        if dataera == "F" or dataera == "G": gt = "130X_dataRun3_PromptAnalysis_v1"
    if datayear == "2023": gt = "130X_dataRun3_PromptAnalysis_v1"
    if datayear == "2024": gt = "140X_dataRun3_PromptAnalysis_v1"
if dataset == "WToLNu":
    if datayear == "2022":
        if dataera == "": gt = "130X_mcRun3_2022_realistic_v5"
        if dataera == "EE": gt = "130X_mcRun3_2022_realistic_postEE_v6"
    if datayear == "2023":
        if dataera == "": gt = "130X_mcRun3_2023_realistic_v14"
        if dataera == "BPix": gt = "130X_mcRun3_2023_realistic_postBPix_v2"
print(gt)

process.load('Configuration.StandardSequences.GeometryRecoDB_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, gt, '')

###########################################################
##### Set up the producer and the end path            #####
###########################################################

process.triggerDataBGAnalyzer = cms.EDAnalyzer ("triggerDataBGAnalyzer",
    vertices = cms.InputTag("offlineSlimmedPrimaryVertices",""),
    mets = cms.InputTag("slimmedMETs"),
    muons = cms.InputTag("slimmedMuons",""),
    jets = cms.InputTag("slimmedJets",""),
    triggersPAT = cms.InputTag('TriggerResults','','PAT'),
    triggersHLT = cms.InputTag('TriggerResults','','HLT'),
    trigobjs = cms.InputTag('slimmedPatTrigger'),
    pfcands = cms.InputTag('packedPFCandidates'),
)

process.myPath = cms.Path (process.triggerDataBGAnalyzer)
