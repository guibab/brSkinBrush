from __future__ import print_function
from __future__ import absolute_import
from maya import cmds, mel
import re
import time
import random
from collections import OrderedDict
from contextlib import contextmanager

import six
from six.moves import range
from six.moves import zip


@contextmanager
def disableUndoContext(raise_error=True, disableUndo=True):
    """
    **CONTEXT** class(*use* ``with`` *statement*)
    """
    if disableUndo:
        cmds.undoInfo(stateWithoutFlush=False)
    try:
        yield
    finally:
        if disableUndo:
            cmds.undoInfo(stateWithoutFlush=True)


@contextmanager
def UndoContext(chunkName="myProcessTrue"):
    """
    **CONTEXT** class(*use* ``with`` *statement*)
    """
    cmds.undoInfo(openChunk=True, chunkName=chunkName)
    try:
        yield
    finally:
        cmds.undoInfo(closeChunk=True)


def get_random_color(pastel_factor=0.5, valueMult=0.5, saturationMult=0.5):
    from Qt.QtGui import QColor

    with UndoContext("get_random_color"):
        col = [
            (x + pastel_factor) / (1.0 + pastel_factor)
            for x in [random.uniform(0, 1.0) for i in [1, 2, 3]]
        ]
        theCol = QColor.fromRgbF(*col)
        theCol.setHsvF(
            theCol.hueF(),
            valueMult * theCol.valueF() + 0.5 * valueMult,
            saturationMult * theCol.saturationF() + 0.5 * saturationMult,
        )
        return theCol.getRgbF()[:3]


def color_distance(c1, c2):
    return sum([abs(x[0] - x[1]) for x in zip(c1, c2)])


def generate_new_color(
    existing_colors, pastel_factor=0.5, valueMult=0.5, saturationMult=0.5
):
    with UndoContext("generate_new_color"):
        max_distance = None
        best_color = None
        for i in range(0, 100):
            color = get_random_color(
                pastel_factor=pastel_factor,
                valueMult=valueMult,
                saturationMult=saturationMult,
            )
            if not existing_colors:
                return color
            best_distance = min([color_distance(color, c) for c in existing_colors])
            if not max_distance or best_distance > max_distance:
                max_distance = best_distance
                best_color = color
        return best_color


def setColorsOnJoints():
    with UndoContext("setColorsOnJoints"):
        _colors = []
        for i in range(1, 9):
            col = cmds.displayRGBColor("userDefined{0}".format(i), q=True)
            _colors.append(col)
        for jnt in cmds.ls(type="joint"):
            theInd = cmds.getAttr(jnt + ".objectColor")
            currentCol = cmds.getAttr(jnt + ".wireColorRGB")[0]
            if currentCol == (0.0, 0.0, 0.0):
                cmds.setAttr(jnt + ".wireColorRGB", *_colors[theInd])
            for destConn in (
                cmds.listConnections(
                    jnt + ".objectColorRGB", d=True, s=False, p=True, type="skinCluster"
                )
                or []
            ):
                cmds.connectAttr(jnt + ".wireColorRGB", destConn, f=True)


def filterInfluences():
    with UndoContext("filterInfluences"):
        items = cmds.treeView("brSkinBrushJointTree", query=True, children="")
        newText = cmds.textFieldGrp("brSkinBrushSearchField", query=True, text=True)
        hideLocked = cmds.checkBoxGrp("brSkinBrushHideLockCheck", q=True, value1=True)
        itemsState = [True] * len(items)
        newTexts = []
        if newText:
            newTexts = newText.split(" ")
            while "" in newTexts:
                newTexts.remove("")
        for i, nm in enumerate(items):
            isLocked = cmds.getAttr(nm + ".lockInfluenceWeights")

            showItem = not (isLocked and hideLocked)
            if showItem and newTexts:
                showItem = False
                for txt in newTexts:
                    txt = txt.replace("*", ".*")
                    showItem = re.search(txt, nm, re.IGNORECASE) is not None
                    if showItem:
                        break
            itemsState[i] = showItem
            cmds.treeView("brSkinBrushJointTree", edit=True, itemVisible=[nm, showItem])


def addInfluences():
    with UndoContext("addInfluences"):
        sel = cmds.ls(sl=True, tr=True)
        skn = cmds.brSkinBrushContext(
            "brSkinBrushContext1", q=True, skinClusterName=True
        )

        deformedShape = cmds.skinCluster(skn, q=True, geometry=True)
        prt = (
            cmds.listRelatives(deformedShape, path=-True, parent=True)[0]
            if not cmds.nodeType(deformedShape) == "transform"
            else deformedShape
        )
        if prt in sel:
            sel.remove(prt)

        allInfluences = cmds.skinCluster(skn, query=True, influence=True)
        toAdd = [x for x in sel if x not in allInfluences]
        if toAdd:
            toAddStr = "add Influences :\n - "
            toAddStr += "\n - ".join(toAdd[:10])
            if len(toAdd) > 10:
                toAddStr += "\n -....and {0} others..... ".format(len(toAdd) - 10)

            res = cmds.confirmDialog(
                t="add Influences",
                m=toAddStr,
                button=["Yes", "No"],
                defaultButton="Yes",
                cancelButton="No",
                dismissString="No",
            )
            if res == "Yes":
                cmds.skinCluster(
                    skn, edit=True, lockWeights=False, weight=0.0, addInfluence=toAdd
                )


def removeUnusedInfluences(self):
    with UndoContext("removeUnusedInfluences"):
        skn = cmds.brSkinBrushContext(
            "brSkinBrushContext1", q=True, skinClusterName=True
        )
        if skn:
            allInfluences = set(cmds.skinCluster(skn, query=True, influence=True))
            weightedInfluences = set(
                cmds.skinCluster(skn, query=True, weightedInfluence=True)
            )
            zeroInfluences = list(allInfluences - weightedInfluences)
            if zeroInfluences:
                toRmvStr = "\n - ".join(zeroInfluences[:10])
                if len(zeroInfluences) > 10:
                    toRmvStr += "\n -....and {0} others..... ".format(
                        len(zeroInfluences) - 10
                    )

                res = cmds.confirmDialog(
                    t="remove Influences",
                    m="remove Unused Influences :\n - {0}".format(toRmvStr),
                    button=["Yes", "No"],
                    defaultButton="Yes",
                    cancelButton="No",
                    dismissString="No",
                )
                if res == "Yes":
                    self.delete_btn.click()
                    cmds.skinCluster(skn, e=True, removeInfluence=zeroInfluences)
                    cmds.evalDeferred(self.selectRefresh)


def doRemoveColorSets():
    with UndoContext("doRemoveColorSets"):
        msh = mel.eval("global string $gSkinBrushMesh; $tmp = $gSkinBrushMesh;")
        if cmds.objExists(msh):
            skinnedMesh_history = (
                cmds.listHistory(msh, lv=0, pruneDagObjects=True) or []
            )
            cmds.setAttr(msh + ".displayColors", 0)
        else:
            return
        while skinnedMesh_history:
            nd = skinnedMesh_history.pop(0)
            if cmds.nodeType(nd) != "createColorSet":
                break
            cmds.delete(nd)


def createWireframe(meshNode, hideOther=True, valAlpha=0.25):
    if not cmds.pluginInfo("wireframeDisplay", q=True, loaded=True):
        try:
            cmds.loadPlugin("wireframeDisplay")
        except RuntimeError:
            return
    with UndoContext("createWireframe"):
        if hideOther:
            wireDisplay = cmds.listRelatives(
                meshNode, s=True, path=True, type="wireframeDisplay"
            )
            if wireDisplay:
                cmds.hide(wireDisplay)
        meshes = cmds.listRelatives(meshNode, s=True, path=True, type="mesh")
        if not meshes:
            return None
        meshes = [
            shp for shp in meshes if not cmds.getAttr(shp + ".intermediateObject")
        ]

        if cmds.objExists("SkinningWireframe"):
            cmds.delete("SkinningWireframe")
        prt = cmds.createNode("transform", n="SkinningWireframe", p=meshNode)
        for msh in meshes:
            loc = cmds.createNode("wireframeDisplay", p=prt, n="SkinningWireframeShape")
            cmds.connectAttr(msh + ".outMesh", loc + ".inMesh", f=True)
            cmds.setAttr(loc + ".ihi", False)

            if cmds.attributeQuery("nurbsTessellate", node=msh, exists=True):
                cmds.setAttr(loc + ".enableSmooth", True)
        return prt


def getMeshTransfrom():
    with UndoContext("getMeshTransfrom"):
        currentContext = cmds.currentCtx()
        mshShape = cmds.brSkinBrushContext(currentContext, query=True, meshName=True)
        if mshShape and cmds.objExists(mshShape):
            (theMesh,) = cmds.listRelatives(mshShape, parent=True, path=True)
            return theMesh
        return None


def getShapesSelected(returnTransform=False):
    with UndoContext("getShapesSelected"):
        typeSurf = ["mesh", "nurbsSurface"]
        selectionShapes = cmds.ls(sl=True, o=True, type=typeSurf)
        if not selectionShapes:
            selection = cmds.ls(sl=True, tr=True) + cmds.ls(hilite=True)
            selectedMesh = cmds.listRelatives(selection, type=typeSurf)
            selectionShapes = cmds.ls(sl=True, o=True, type=typeSurf)
            if selectedMesh:
                selectionShapes += selectedMesh
            selectionShapes = [
                el
                for el in selectionShapes
                if not cmds.getAttr(el + ".intermediateObject")
            ]
        if selectionShapes and returnTransform:
            return cmds.listRelatives(selectionShapes, path=True, parent=True)
        return selectionShapes


def addLockVerticesAttribute():  # not used
    with UndoContext("addLockVerticesAttribute"):
        currentContext = cmds.currentCtx()
        mshShape = cmds.brSkinBrushContext(currentContext, query=True, meshName=True)
        if not cmds.attributeQuery("lockedVertices", node=mshShape, exists=True):
            cmds.addAttr(mshShape, longName="lockedVertices", dataType="Int32Array")


def addControllersToJoints():
    with UndoContext("addControllersToJoints"):
        allJnts = cmds.ls(type="joint")
        for jnt in allJnts:
            if not cmds.listConnections(jnt, type="controller"):
                cmds.controller(jnt)


######################################################
# used by cpp tool to get the font size for display
def fnFonts(txt):
    from Qt.QtGui import QFont, QFontMetrics

    sz = QFontMetrics(QFont("MS Shell Dlg 2", 14)).boundingRect(txt)
    return [sz.width() + 2, sz.height() + 2]


def setToDgMode():
    with UndoContext("setToDgMode"):
        goodMode = "off"  # "serial" anmd "serialUncached" and "parallel" crashes
        if cmds.evaluationManager(q=True, mode=True) != [goodMode]:
            val = cmds.optionVar(q="evaluationMode")
            cmds.evaluationManager(mode=goodMode)
            cmds.optionVar(intValue=["revertParallelEvaluationMode", val])
            # Set everything in the entire scene dirty
            cmds.dgdirty(allPlugs=True)
        else:
            cmds.optionVar(intValue=["revertParallelEvaluationMode", 0])


def retrieveParallelMode():
    with UndoContext("retrieveParallelMode"):
        val = cmds.optionVar(q="revertParallelEvaluationMode")
        if val != 0:
            cmds.optionVar(intValue=["revertParallelEvaluationMode", 0])
            mode = "parallel" if val == 3 else "serial"
            cmds.evaluationManager(mode=mode)


def toolOnSetupStart():
    with UndoContext("toolOnSetupStart"):
        cmds.optionVar(intValue=["startTime", time.time()])

        setToDgMode()
        # disable AutoSave --------------------------
        if cmds.autoSave(query=True, enable=True):
            if not cmds.optionVar(ex="autoSaveEnable"):
                cmds.optionVar(intValue=["autoSaveEnable", 1])
            cmds.autoSave(enable=False)

        # found that if not Shannon paint doesn't swap deformers
        cmds.optionVar(clearArray="colorShadedDisplay")
        cmds.optionVar(intValueAppend=["colorShadedDisplay", 1])
        cmds.optionVar(
            intValueAppend=["colorShadedDisplay", 1], intValue=["colorizeSkeleton", 1]
        )

        sel = cmds.ls(sl=True)
        cmds.optionVar(clearArray="brushPreviousSelection")
        for obj in sel:
            cmds.optionVar(stringValueAppend=["brushPreviousSelection", obj])
        shapeSelected = getShapesSelected(returnTransform=True)
        if not shapeSelected:  # if nothing selected
            mshShape = mel.eval(
                "global string $gSkinBrushMesh; $temp = $gSkinBrushMesh"
            )
            if mshShape and cmds.objExists(mshShape):
                (theMesh,) = cmds.listRelatives(mshShape, parent=True, path=True)
                cmds.select(theMesh)
        else:
            cmds.select(shapeSelected)
        mshShapeSelected = getShapesSelected(returnTransform=False)
        # add nurbs Tesselate
        selectedNurbs = cmds.ls(mshShapeSelected, type="nurbsSurface")

        if selectedNurbs:
            mshShapeSelected = addNurbsTessellate(selectedNurbs)
            for nrbs in selectedNurbs:
                cmds.hide(nrbs)
        # for colors
        for mshShape in cmds.ls(mshShapeSelected, type="mesh"):
            cmds.polyOptions(mshShape, colorShadedDisplay=True)
            if not cmds.attributeQuery("lockedVertices", node=mshShape, exists=True):
                cmds.addAttr(mshShape, longName="lockedVertices", dataType="Int32Array")
            cmds.setAttr(mshShape + ".colorSet", size=2)
            cmds.setAttr(
                mshShape + ".colorSet[0].colorName", "multiColorsSet", type="string"
            )
            cmds.setAttr(
                mshShape + ".colorSet[1].colorName", "soloColorsSet", type="string"
            )
            cmds.setAttr(mshShape + ".vertexColorSource", 2)
            cmds.setAttr(mshShape + ".displayColors", 1)
            cmds.setAttr(mshShape + ".displaySmoothMesh", 0)
        callEventCatcher()


def createMeshFromNurbs(att, prt):
    nurbsTessellate = cmds.createNode("nurbsTessellate", skipSelect=True)
    cmds.setAttr(nurbsTessellate + ".format", 3)
    cmds.setAttr(nurbsTessellate + ".polygonType", 1)
    cmds.setAttr(nurbsTessellate + ".matchNormalDir", 1)
    cmds.connectAttr(att, nurbsTessellate + ".inputSurface", f=True)

    msh = cmds.createNode("mesh", p=prt, skipSelect=True, n="brushTmpDELETEthisMesh")
    cmds.connectAttr(nurbsTessellate + ".outputPolygon", msh + ".inMesh", f=True)

    cmds.sets(msh, edit=True, forceElement="initialShadingGroup")
    return msh


def setSkinCluster(nrbs, state=True):
    skns = cmds.ls(
        cmds.listHistory(nrbs, lv=1, pruneDagObjects=True, interestLevel=True) or [],
        type="skinCluster",
    )
    val = 0 if state else 1
    for skn in skns:
        cmds.setAttr(skn + ".nodeState", val)


def getOrigShape(nrbs):
    (prt,) = cmds.listRelatives(nrbs, p=True, path=True)
    allShapes = cmds.listRelatives(prt, s=True, noIntermediate=False)
    deformedShapes = cmds.listRelatives(prt, s=True, noIntermediate=True)
    baseShapes = set(allShapes) - set(deformedShapes)
    if len(baseShapes) > 1:
        for origShape in baseShapes:
            hist = cmds.ls(cmds.listHistory(origShape, f=True), type="shape")
            if nrbs in hist:
                return origShape
    else:
        origShape = baseShapes.pop()
        return origShape
    return None


def addNurbsTessellate(selectedNurbs):
    mshs = []
    for nrbs in selectedNurbs:
        if cmds.listConnections(nrbs, s=0, d=1, type="nurbsTessellate"):
            continue
        (prt,) = cmds.listRelatives(nrbs, p=True, path=True)
        att = nrbs + ".local"
        msh = createMeshFromNurbs(att, prt)
        mshs.append(msh)
        cmds.addAttr(
            msh, longName="nurbsTessellate", attributeType="double", defaultValue=0.0
        )
        if not cmds.attributeQuery("nurbsTessellate", n=nrbs, ex=True):
            cmds.addAttr(
                nrbs,
                longName="nurbsTessellate",
                attributeType="double",
                defaultValue=0.0,
            )
        cmds.connectAttr(nrbs + ".nurbsTessellate", msh + ".nurbsTessellate", f=True)

        origShape = getOrigShape(nrbs)
        att = origShape + ".local"
        origMsh = createMeshFromNurbs(att, prt)
        cmds.setAttr(origMsh + ".v", 0)
        cmds.setAttr(origMsh + ".intermediateObject", 1)
        cmds.addAttr(
            origMsh, longName="origMeshNurbs", attributeType="double", defaultValue=0.0
        )
        cmds.addAttr(
            msh, longName="origMeshNurbs", attributeType="double", defaultValue=0.0
        )
        cmds.connectAttr(msh + ".origMeshNurbs", origMsh + ".origMeshNurbs", f=True)
    return mshs


def disconnectNurbs():
    sel = cmds.ls(sl=True)
    toDelete = []
    for nd in sel:
        prt = (
            nd
            if cmds.nodeType(nd) == "transform"
            else cmds.listRelatives(nd, parent=True, path=True)
        )
        mshs = cmds.listRelatives(prt, type="mesh", path=True)
        for msh in mshs:
            tesselates = cmds.ls(cmds.listHistory(msh), type="nurbsTessellate") or []
            if tesselates:
                toDelete.extend(tesselates)
    cmds.delete(toDelete)


def showBackNurbs(theMesh):
    shps = cmds.listRelatives(theMesh, s=True, path=True, type="nurbsSurface") or []
    mshs = cmds.listRelatives(theMesh, s=True, path=True, type="mesh") or []
    toDelete = []
    for msh in mshs:
        if cmds.attributeQuery("origMeshNurbs", n=msh, ex=True):
            toDelete.append(msh)
    cmds.delete(toDelete)
    for nrbs in shps:
        if cmds.attributeQuery("nurbsTessellate", n=nrbs, ex=True):
            cmds.deleteAttr(nrbs + ".nurbsTessellate")
    if shps:
        cmds.showHidden(shps)


def cleanTheNurbs(force=False):
    if cmds.currentCtx() != "brSkinBrushContext1" or force:
        nurbsTessellateAttrs = cmds.ls("*.nurbsTessellate")
        if nurbsTessellateAttrs:
            nurbsTessellateAttrsNodes = [
                el.split(".nurbsTessellate")[0] for el in nurbsTessellateAttrs
            ]
            prts = set(cmds.listRelatives(nurbsTessellateAttrsNodes, p=True, path=True))
            for prt in prts:
                showBackNurbs(prt)


def deferredDisconnect(mshTesselate, msh):
    inConns = cmds.listConnections(msh + ".inMesh", s=1, d=0, p=1, c=1)
    cmds.disconnectAttr(inConns[1], inConns[0])
    (BS,) = cmds.blendShape(mshTesselate, msh)
    cmds.setAttr(BS + ".w[0]", 1)
    cmds.setAttr(mshTesselate + ".v", 0)
    cmds.setAttr(mshTesselate + ".intermediateObject", 1)


def callEventCatcher():
    import catchEventsUI
    catchEventsUI.EVENTCATCHER = catchEventsUI.CatchEventsWidget()
    catchEventsUI.EVENTCATCHER.open()


def closeEventCatcher():
    import catchEventsUI

    if hasattr(catchEventsUI, "EVENTCATCHER"):
        catchEventsUI.EVENTCATCHER.close()


def toolOnSetupEndDeferred():
    with UndoContext("toolOnSetupEndDeferred"):
        disconnectNurbs()
        addWireFrameToMesh()
        cmds.select(clear=True)
        currentContext = cmds.currentCtx()
        mshShape = cmds.brSkinBrushContext(currentContext, query=True, meshName=True)
        mel.eval('global string $gSkinBrushMesh; $gSkinBrushMesh="' + mshShape + '";')
        cmds.evalDeferred(doUpdateWireFrameColorSoloMode)
        # ------ compute time ----------------------------------
        startTime = cmds.optionVar(q="startTime")
        completionTime = time.time() - startTime

        callPaintEditorFunction("paintStart")
        print(
            "----- load BRUSH for {} in  [{:.2f} secs] ------".format(
                mshShape, completionTime
            )
        )


def toolOnSetupEnd():
    with UndoContext("toolOnSetupEnd"):
        toolOnSetupEndDeferred()
    cleanOpenUndo()


def toolOffCleanup():
    with UndoContext("toolOffCleanup"):
        toolOffCleanupDeferred()


def toolOffCleanupDeferred():
    if cmds.objExists("SkinningWireframe"):
        cmds.delete("SkinningWireframe")
    closeEventCatcher()
    # unhide previous wireFrames :
    theMesh = getMeshTransfrom()
    if theMesh:
        try:
            wireDisplay = cmds.listRelatives(
                theMesh, s=True, path=True, type="wireframeDisplay"
            )
            if wireDisplay:
                cmds.showHidden(wireDisplay)
        except RuntimeError:  # RuntimeError: Unknown object type: wireframeDisplay
            pass
    showBackNurbs(theMesh)

    # delete colors on Q pressed
    doRemoveColorSets()
    retrieveParallelMode()

    # retrieve autoSave
    if cmds.optionVar(ex="autoSaveEnable") and cmds.optionVar(q="autoSaveEnable") == 1:
        cmds.autoSave(enable=True)

    callPaintEditorFunction("paintEnd")
    if cmds.optionVar(ex="brushPreviousSelection"):
        cmds.select(cmds.optionVar(q="brushPreviousSelection"))


def escapePressed():
    doRemoveColorSets()


def addWireFrameToMesh():
    wireframeCB = callPaintEditorFunction("wireframe_cb")
    if wireframeCB and not wireframeCB.isChecked():
        print("no wireframe")
        return

    theMesh = cmds.ls(sl=True, tr=True)[0]
    createWireframe(theMesh)


def updateWireFrameColorSoloMode(soloColor):
    with disableUndoContext():
        if cmds.objExists("SkinningWireframeShape"):
            overrideColorRGB = [0.8, 0.8, 0.8] if soloColor else [0.1, 0.1, 0.1]
            cmds.setAttr("SkinningWireframeShape.overrideEnabled", 1)
            cmds.setAttr("SkinningWireframeShape.overrideRGBColors", 1)
            cmds.setAttr("SkinningWireframeShape.overrideColorRGB", *overrideColorRGB)


def doUpdateWireFrameColorSoloMode():
    currentContext = cmds.currentCtx()
    soloColor = cmds.brSkinBrushContext(currentContext, q=True, soloColor=True)
    updateWireFrameColorSoloMode(soloColor)


def setSoloMode(soloColor):
    with UndoContext("setSoloMode"):
        ctx = cmds.currentCtx()
        cmds.brSkinBrushContext(ctx, e=True, soloColor=soloColor)
        updateWireFrameColorSoloMode(soloColor)


def toggleSoloMode():
    ctx = cmds.currentCtx()
    soloColor = cmds.brSkinBrushContext(ctx, q=True, soloColor=True)
    setSoloMode(not soloColor)
    callPaintEditorFunction("upateSoloModeRBs", not soloColor)


def fixOptionVarContext(**inputKargsToChange):
    with UndoContext("fixOptionVarContext"):
        kwargs = OrderedDict()
        if cmds.optionVar(exists="brSkinBrushContext1"):
            cmd = cmds.optionVar(q="brSkinBrushContext1")
            # remove command name and command object at the end : brSkinBrushContext anmd brSkinBrushContext1;
            splitofspaces = cmd.split(" ")
            cmd2 = " ".join(splitofspaces[1:-1])
            spl = cmd2.split("-")
            hlp = cmds.help("brSkinBrushContext")

            dicOfName = {}
            dicExpectedArgs = {}
            lsMulti = set()
            for ln in hlp.split("\n"):
                for expectedStuff in [
                    "(Query Arg Mandatory)",
                    "(Query Arg Optional)",
                    "[...]",
                    "(Query Arg Optional)",
                ]:
                    ln = ln.replace(expectedStuff, "")
                ln = ln.strip()
                res = ln.split()
                if len(res) >= 2 and res[0].startswith("-") and res[1].startswith("-"):
                    nmFlag = res[1][1:]
                    dicOfName[res[0][1:]] = nmFlag
                    dicOfName[res[1][1:]] = nmFlag
                    if "(multi-use)" in res:
                        lsMulti.add(nmFlag)
                        res.remove("(multi-use)")
                    dicExpectedArgs[nmFlag] = res[2:]

            newSpl = []
            for lne in spl:
                lineSplit = lne.strip().split(" ")

                if len(lineSplit) > 1:
                    kArg = "-" + lineSplit[0]
                    if kArg not in hlp:
                        continue
                    else:
                        value = " ".join(lineSplit[1:])
                        value = value.strip()
                        if value.startswith('"') and value.endswith('"'):
                            value = value[1:-1]
                        if lineSplit[0] in dicOfName:
                            keyToCheck = dicOfName[lineSplit[0]]
                            if keyToCheck in kwargs:
                                kwargs[keyToCheck] = value
                else:
                    if lineSplit[0] in dicOfName:
                        kwargs[dicOfName[lineSplit[0]]] = True
                newSpl.append(lne)

            # now rebuild command ---------------------------------
            kwargs.update(inputKargsToChange)
            cmdNew = "brSkinBrushContext "
            for key, value in six.iteritems(kwargs):
                if isinstance(value, bool):
                    cmdNew += "-{} ".format(key)
                else:
                    try:
                        float(value)
                        cmdNew += "-{} {} ".format(key, value)
                    except ValueError:
                        cmdNew += '-{} "{}" '.format(key, value)
            cmdNew += splitofspaces[-1]
            cmds.optionVar(stringValue=["brSkinBrushContext1", cmdNew])
        return kwargs


def deleteExistingColorSets():
    with UndoContext("deleteExistingColorSets"):
        sel = cmds.ls(sl=True)
        for obj in sel:
            skinnedMesh_history = (
                cmds.listHistory(obj, lv=0, pruneDagObjects=True) or []
            )
            cmds.setAttr(obj + ".displayColors", 0)
            res = cmds.ls(
                skinnedMesh_history, type=["createColorSet", "deleteColorSet"]
            )
            if res:
                cmds.delete(res)
            existingColorSets = cmds.polyColorSet(obj, q=True, allColorSets=True) or []
            for colSet in [
                "multiColorsSet",
                "multiColorsSet2",
                "soloColorsSet",
                "soloColorsSet2",
            ]:
                if colSet in existingColorSets:
                    cmds.polyColorSet(obj, delete=True, colorSet=colSet)


# --------------CALL FROM BRUSH-------------------------
def cleanOpenUndo():
    print("CALL cleanOpenUndo - pass")


def cleanCloseUndo():
    print("CALL cleanCloseUndo - pass")


def getPaintEditor():
    with UndoContext("getPaintEditor"):
        import mPaintEditor

        editor = mPaintEditor.PAINT_EDITOR
        if editor is not None and editor.isVisible():
            return editor
        return None


def afterPaint():
    with UndoContext("afterPaint"):
        import mWeightEditor
        from Qt.QtWidgets import QApplication

        editor = mWeightEditor.WEIGHT_EDITOR
        if editor is not None and editor in QApplication.instance().topLevelWidgets():
            editor.refreshSkinDisplay()


def callPaintEditorFunction(function, *args, **kwargs):
    with UndoContext("callPaintEditorFunction"):
        paintEditor = getPaintEditor()
        if paintEditor and hasattr(paintEditor, function):
            fn = getattr(paintEditor, function)
            if callable(fn):
                return fn(*args, **kwargs)
            else:
                return fn
        return None


def headsUpMessage(offsetX, offsetY, message, valueDisplay, precision):
    with UndoContext("headsUpMessage"):
        theMessage = "{}: {:.{}f}".format(message, valueDisplay, precision)
        cmds.headsUpMessage(
            theMessage, horizontalOffset=offsetX, verticalOffset=offsetY, time=0.1
        )


def orderedInfluence(strl):
    orderOfJoints = list(map(int, strl[:-1].split(" ")))
    callPaintEditorFunction("updateOrderOfInfluences", orderOfJoints)


def pickedInfluence(jointName):
    with UndoContext("pickedInfluence"):
        if cmds.treeView("brSkinBrushJointTree", q=True, ex=True):
            cmds.treeView("brSkinBrushJointTree", edit=True, clearSelection=True)
            cmds.treeView("brSkinBrushJointTree", edit=True, showItem=jointName)
            mel.eval(
                'global string $gSkinBrushInfluenceSelection[];    $gSkinBrushInfluenceSelection = { "'
                + jointName
                + '" };'
            )

        callPaintEditorFunction("updateCurrentInfluence", jointName)


def updateDisplayStrengthOrSize(sizeAdjust, value):
    with UndoContext("updateDisplayStrengthOrSize"):
        fsg = "brSkinBrushSize" if sizeAdjust else "brSkinBrushStrength"
        if cmds.floatSliderGrp(fsg, q=True, ex=True):
            cmds.floatSliderGrp(fsg, e=True, value=value)

        if sizeAdjust:
            callPaintEditorFunction("updateSizeVal", value)
        else:
            callPaintEditorFunction("updateStrengthVal", value)
