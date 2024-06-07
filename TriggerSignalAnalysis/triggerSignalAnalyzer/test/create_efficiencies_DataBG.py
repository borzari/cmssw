import os
from os.path import exists

process = "Muon2022F_tree2"
outputDir = "Plots_TriggerEffs_" + process

if not exists(outputDir): os.system("mkdir " + outputDir)

os.system('root -l -b "create_efficiencies_DataBG.C(\\"' + process + '\\", \\"' +  outputDir+ '\\")"')