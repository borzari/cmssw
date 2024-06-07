import os
from os.path import exists

lifetime = "1000"
outputDir = "Plots_TriggerEffs_" + lifetime + "cm"

if not exists(outputDir): os.system("mkdir " + outputDir)

os.system('root -l -b "create_efficiencies.C(\\"' + lifetime + '\\", \\"' +  outputDir+ '\\")"')