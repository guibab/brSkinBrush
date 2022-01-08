from __future__ import absolute_import
from __future__ import print_function
from shutil import copyfile
import os

folderSrc = r"C:\Users\guillaume\Documents\maya\modules\brSkinBrush\icons"
folderSrc = r"C:\Users\guillaume\Documents\maya\modules\brSkinBrush\icons\svg"

for pth in os.listdir(folderSrc):
    thePth = os.path.join(folderSrc, pth)
    if os.path.isfile(thePth)  : 
        print(thePth)
        thePthRename = os.path.join(folderSrc, pth.replace("SmoothWeights", "SkinBrush"))
        if  thePthRename != thePth:
            os.rename(thePth, thePthRename)


