import os

os.system("cd .. && scram b -j 8 && cd test/ && cmsRun triggerDataBGAnalysis_cfg.py")
