import os
import glob

def buildModFile(rootFolder):
    rootFolder = os.path.abspath(rootFolder)
    modFld = os.path.join(rootFolder, "modules")
    modPath = os.path.join(modFld, "brSkinBrush.mod")
    mllGlob = os.path.join(modFld, "brSkinBrush", "plug-ins", "win64", "*", "brSkinBrush.mll")
    mlls = sorted(glob.glob(mllGlob))

    lines = []
    for mllPath in mlls:
        mayaVer = os.path.basename(os.path.dirname(mllPath))
        lines.append('+ MAYAVERSION:{0} brSkinBrush any brSkinBrush'.format(mayaVer))
        lines.append('plug-ins: plug-ins/win64/{0}'.format(mayaVer))
        lines.append('')

    allLines = '\n'.join(lines)
    with open(modPath, 'w') as f:
        f.write(allLines)


if __name__ == "__main__":
    buildModFile(os.path.dirname(os.path.dirname(__file__)))
