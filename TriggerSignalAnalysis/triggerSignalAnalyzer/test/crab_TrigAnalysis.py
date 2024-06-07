#!/usr/bin/env python3

import os
from CRABClient.UserUtilities import config
config = config()

dataera = "F"
# dataera = "BPix"

config.General.requestName = ''
config.General.workArea = 'crab'
config.General.transferOutputs = True
config.General.transferLogs = True
config.General.requestName = 'passedmuon_Muon_MET105IsoTrk50Sig_2022' + dataera
# config.General.requestName = 'WToLNu_MET105IsoTrk50Sig_2023' + dataera

config.JobType.pluginName = 'Analysis'
config.JobType.psetName = 'triggerDataBGAnalysis_cfg.py'
config.JobType.allowUndistributedCMSSW = True

# always use MINIAOD as inputDataset and AOD as secondaryInputDataset

# config.Data.inputDataset = '/Muon/Run2022C-22Sep2023-v1/MINIAOD'
# config.Data.inputDataset = '/Muon/Run2022D-22Sep2023-v1/MINIAOD'
# config.Data.inputDataset = '/Muon/Run2022E-22Sep2023-v1/MINIAOD'
config.Data.inputDataset = '/Muon/Run2022F-22Sep2023-v2/MINIAOD'
# config.Data.inputDataset = '/Muon/Run2022G-22Sep2023-v1/MINIAOD'
# config.Data.inputDataset = '/Muon0/Run2023C-22Sep2023_v4-v1/MINIAOD'
# config.Data.inputDataset = '/Muon1/Run2023C-22Sep2023_v4-v2/MINIAOD'
# config.Data.inputDataset = '/Muon0/Run2023D-22Sep2023_v2-v1/MINIAOD'
# config.Data.inputDataset = '/Muon1/Run2023D-22Sep2023_v2-v1/MINIAOD'
# config.Data.inputDataset = '/WtoLNu-4Jets_TuneCP5_13p6TeV_madgraphMLM-pythia8/Run3Summer22MiniAODv4-130X_mcRun3_2022_realistic_v5-v2/MINIAODSIM'
# config.Data.inputDataset = '/WtoLNu-4Jets_TuneCP5_13p6TeV_madgraphMLM-pythia8/Run3Summer22MiniAODv4-130X_mcRun3_2022_realistic_v5_ext1-v2/MINIAODSIM'
# config.Data.inputDataset = '/WtoLNu-4Jets_TuneCP5_13p6TeV_madgraphMLM-pythia8/Run3Summer22EEMiniAODv4-130X_mcRun3_2022_realistic_postEE_v6-v2/MINIAODSIM'
# config.Data.inputDataset = '/WtoLNu-4Jets_TuneCP5_13p6TeV_madgraphMLM-pythia8/Run3Summer22EEMiniAODv4-130X_mcRun3_2022_realistic_postEE_v6_ext1-v2/MINIAODSIM'
# config.Data.inputDataset = '/WtoLNu-4Jets_TuneCP5_13p6TeV_madgraphMLM-pythia8/Run3Summer23MiniAODv4-130X_mcRun3_2023_realistic_v14-v3/MINIAODSIM'
# config.Data.inputDataset = '/WtoLNu-4Jets_TuneCP5_13p6TeV_madgraphMLM-pythia8/Run3Summer23BPixMiniAODv4-130X_mcRun3_2023_realistic_postBPix_v2-v3/MINIAODSIM'
config.Data.lumiMask = "Cert_Collisions2022_355100_362760_Golden.json"
# config.Data.lumiMask = "Cert_Collisions2023_366442_370790_Golden.json"

config.Data.inputDBS = 'global'
config.Data.splitting = 'FileBased'
config.Data.unitsPerJob = 10 # leave this as one to avoid too much wall time and jobs failing

# config.Data.publication = True
config.Data.publication = False # CRAB can't publish more than one dataset per task; set publication as false when using multiple channels
config.Data.outputDatasetTag = 'passedmuon_TrigAnalysis_Muon2022' + dataera # this is just an example; it will be part of the name of the output dataset
# config.Data.outputDatasetTag = 'TrigAnalysis_WToLNu2023' + dataera # this is just an example; it will be part of the name of the output dataset

config.Data.outLFNDirBase = '/store/user/borzari/'
config.Site.storageSite = 'T2_BR_SPRACE'
