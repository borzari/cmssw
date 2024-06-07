import os
from os.path import exists

crabDir = "crab/"
# checkDirs = ["crab_Muon_MET105IsoTrk50Sig_2022C","crab_Muon_MET105IsoTrk50Sig_2022D","crab_Muon_MET105IsoTrk50Sig_2022E","crab_Muon_MET105IsoTrk50Sig_2022F","crab_Muon_MET105IsoTrk50Sig_2022G","crab_Muon_MET105IsoTrk50Sig_2023C0","crab_Muon_MET105IsoTrk50Sig_2023C1","crab_Muon_MET105IsoTrk50Sig_2023D0","crab_Muon_MET105IsoTrk50Sig_2023D1"] # Data: 2022 + 2023
# checkDirs = ["crab_Muon_MET105IsoTrk50Sig_2022C","crab_Muon_MET105IsoTrk50Sig_2022D","crab_Muon_MET105IsoTrk50Sig_2022E","crab_Muon_MET105IsoTrk50Sig_2022F","crab_Muon_MET105IsoTrk50Sig_2022G"] # Data: 2022
checkDirs = ["crab_Muon_MET105IsoTrk50Sig_2023C0","crab_Muon_MET105IsoTrk50Sig_2023C1","crab_Muon_MET105IsoTrk50Sig_2023D0","crab_Muon_MET105IsoTrk50Sig_2023D1"] # Data: 2023
# checkDirs = ["crab_Muon_MET105IsoTrk50Sig_2023D1"] # Data: Failed tasks
# checkDirs = ["crab_WToLNu_MET105IsoTrk50Sig_2022","crab_WToLNu_MET105IsoTrk50Sig_2022ext","crab_WToLNu_MET105IsoTrk50Sig_2022EE","crab_WToLNu_MET105IsoTrk50Sig_2022EEext","crab_WToLNu_MET105IsoTrk50Sig_2023","crab_WToLNu_MET105IsoTrk50Sig_2023BPix"] # MC: 2022 + 2023
# checkDirs = ["crab_WToLNu_MET105IsoTrk50Sig_2022","crab_WToLNu_MET105IsoTrk50Sig_2022ext","crab_WToLNu_MET105IsoTrk50Sig_2022EE","crab_WToLNu_MET105IsoTrk50Sig_2022EEext"] # MC: 2022
# checkDirs = ["crab_WToLNu_MET105IsoTrk50Sig_2023","crab_WToLNu_MET105IsoTrk50Sig_2023BPix"] # MC: 2023

if exists("checkingCRAB.txt"): os.system("rm checkingCRAB.txt")

for dir in checkDirs:
    os.system("crab status -d " + crabDir + dir + " >> checkingCRAB.txt")