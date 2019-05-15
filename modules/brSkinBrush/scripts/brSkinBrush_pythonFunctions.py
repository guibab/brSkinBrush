from maya import cmds, mel
import re
"""
import brSkinBrush_pythonFunctions
reload (brSkinBrush_pythonFunctions)
brSkinBrush_pythonFunctions.setColorsOnJoints ()

"""
#To make your color choice reproducible, uncomment the following line:
#random.seed(10)
def get_random_color(pastel_factor = 0.5):
    return [(x+pastel_factor)/(1.0+pastel_factor) for x in [random.uniform(0,1.0) for i in [1,2,3]]]

def color_distance(c1,c2):
    return sum([abs(x[0]-x[1]) for x in zip(c1,c2)])

def generate_new_color(existing_colors,pastel_factor = 0.5):
    max_distance = None
    best_color = None
    for i in range(0,100):
        color = get_random_color(pastel_factor = pastel_factor)
        if not existing_colors:
            return color
        best_distance = min([color_distance(color,c) for c in existing_colors])
        if not max_distance or best_distance > max_distance:
            max_distance = best_distance
            best_color = color
    return best_color

def setColorsOnJoints ():
    _colors = []
    for i in xrange (1,9) :
        col = cmds.displayRGBColor( "userDefined{0}".format (i), q=True)
        _colors.append (col)
    
    for jnt in cmds.ls (type = "joint") :
        theInd = cmds.getAttr (jnt+".objectColor")
        currentCol = cmds.getAttr (jnt+".wireColorRGB")[0]
        if currentCol == (0.0, 0.0, 0.0) :
            cmds.setAttr (jnt+".wireColorRGB",*_colors[theInd] )
        for destConn in cmds.listConnections (jnt+".objectColorRGB", d=True, s=False, p=True, type = "skinCluster") or []:
            cmds.connectAttr (jnt+".wireColorRGB", destConn , f=True)


def filterInfluences () : 
    items = cmds.treeView( "brSkinBrushJointTree", query=True, children="")
    newText = cmds.textFieldGrp( "brSkinBrushSearchField", query=True, text=True)
    hideLocked = cmds.checkBoxGrp ("brSkinBrushHideLockCheck",q=True,value1=True)
    itemsState = [True]*len(items )
    newTexts = []
    if newText :
        newTexts = newText.split (" ")
        while "" in newTexts : newTexts.remove ("")

    for i, nm in enumerate (items):
        isLocked = cmds.getAttr (nm+".lockInfluenceWeights")

        showItem = not (isLocked and hideLocked)
        if showItem and newTexts:
            showItem = False   
            for txt in newTexts : 
                txt = txt.replace ("*", ".*")
                showItem = re.search ( txt, nm, re.IGNORECASE)!=None
                if showItem : break        
        itemsState [i] = showItem
        cmds.treeView( "brSkinBrushJointTree", edit=True, itemVisible=[nm, showItem] )

    """
    else : 
        for nm , item in self.uiInfluenceTREE.dicWidgName.iteritems ():
            item.setHidden (not self.showZeroDeformers and item.isZeroDfm )
    """

def addInfluences ():
    sel = cmds.ls (sl=True, tr=True)
    skn = cmds.brSkinBrushContext ("brSkinBrushContext1",q=True,skinClusterName=True)

    deformedShape = cmds.skinCluster (skn, q=True,geometry=True )
    prt = cmds.listRelatives (deformedShape, path=-True, parent=True)[0] if not cmds.nodeType (deformedShape) == "transform" else deformedShape
    if prt in sel : sel.remove (prt)

    allInfluences = cmds.skinCluster( skn , query=True, influence=True)
    toAdd = filter (lambda x :  x not in allInfluences, sel)
    if toAdd :
        toAddStr = "add Influences :\n - "
        toAddStr += "\n - ".join(toAdd[:10])
        if len (toAdd) > 10 : toAddStr += "\n -....and {0} others..... ".format(len (toAdd)-10)

        res = cmds.confirmDialog (t="add Influences",m=toAddStr,button=['Yes','No'], defaultButton='Yes', cancelButton='No', dismissString='No')
        if res == 'Yes' :             
            cmds.skinCluster(skn, edit=True, lockWeights=False, weight=0.0, addInfluence=toAdd)
            """
            toSelect = range(self.uiInfluenceTREE.topLevelItemCount(), self.uiInfluenceTREE.topLevelItemCount()+len(toAdd))
            cmds.evalDeferred (self.selectRefresh)
            cmds.evalDeferred (partial (self.reselectIndices,toSelect))
            """

def removeUnusedInfluences(self):
    skn = cmds.brSkinBrushContext ("brSkinBrushContext1",q=True,skinClusterName=True)

    if skn : 
        allInfluences = set(cmds.skinCluster( skn , query=True, influence=True))
        weightedInfluences = set(cmds.skinCluster( skn , query=True, weightedInfluence=True))
        zeroInfluences = list(allInfluences-weightedInfluences)
        if zeroInfluences : 
            toRmvStr = "\n - ".join(zeroInfluences[:10])
            if len (zeroInfluences) > 10 : toRmvStr += "\n -....and {0} others..... ".format(len (zeroInfluences)-10)

            res = cmds.confirmDialog (t="remove Influences", m="remove Unused Influences :\n - {0}".format (toRmvStr),button=['Yes','No'], defaultButton='Yes', cancelButton='No', dismissString='No')
            if res == 'Yes' : 
                self.delete_btn.click()
                cmds.skinCluster(skn, e=True, removeInfluence=zeroInfluences)
                cmds.evalDeferred (self.selectRefresh)

def rmvColorSets ():
    print "finishing tool\n"
    return
    doRemoveColorSets ()

def doRemoveColorSets ():
    skinnedMesh_history = cmds.ls(cmds.listHistory(lv = 0, pruneDagObjects = True))
    while skinnedMesh_history :
        nd = skinnedMesh_history.pop(0)
        if cmds.nodeType(nd) != 'createColorSet' : break
        cmds.delete (n)


