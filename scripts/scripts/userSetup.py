# ----------------------------------------------------------------------
# userSetup.mel
#
# Automatically add the menu items for the smooth weights tool upon Maya
# startup.
#
# ----------------------------------------------------------------------
from maya import cmds, mel, utils

def addMenuItems():
    if not cmds.about(batch=True):
        mel.eval("source brSkinBrushCreateMenuItems; brSkinBrushAddMenuCommand;")

utils.executeDeferred(addMenuItems)
