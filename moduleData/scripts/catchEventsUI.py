from __future__ import absolute_import
from __future__ import print_function
from Qt import QtGui, QtCore, QtWidgets
from Qt import QtCompat
from Qt.QtWidgets import QApplication, QSplashScreen, QDialog, QMainWindow

from maya import OpenMayaUI, cmds, mel
import brSkinBrush_pythonFunctions
import six


PTR_TYPE = six.integer_types[-1]  # Long/int in python 2/3


def callMarkingMenu():
    if cmds.popupMenu("tempMM", exists=True):
        cmds.deleteUI("tempMM")

    res = mel.eval("findPanelPopupParent")
    cmds.popupMenu(
        "tempMM",
        button=1,
        ctrlModifier=False,
        altModifier=False,
        allowOptionBoxes=True,
        parent=res,
        markingMenu=True,
    )

    kwArgs = {
        "label": "add",
        "divider": False,
        "subMenu": False,
        "tearOff": False,
        "optionBox": False,
        "enable": True,
        "data": 0,
        "allowOptionBoxes": True,
        "postMenuCommandOnce": False,
        "enableCommandRepeat": True,
        "echoCommand": False,
        "italicized": False,
        "boldFont": True,
        "sourceType": "mel",
        "longDivider": True,
    }
    # 0 Add - 1 Remove - 2 AddPercent - 3 Absolute - 4 Smooth - 5 Sharpen - 6 LockVertices - 7 UnLockVertices

    lstCommands = [
        ("add", "N", "add", 0),
        ("remove", "S", "rmv", 1),
        ("addPercent", "NW", "addPerc", 2),
        ("absolute", "E", "abs", 3),
        ("smooth", "W", "smooth", 4),
        ("locks Verts", "SE", "locks", 6),
    ]
    for ind, (txt, posi, btn, cmdInd) in enumerate(lstCommands):
        kwArgs["radialPosition"] = posi
        kwArgs["label"] = txt
        kwArgs[
            "command"
        ] = "brSkinBrushContext -edit -commandIndex {} `currentCtx`;".format(cmdInd)
        cmds.menuItem("menuEditorMenuItem{0}".format(ind + 1), **kwArgs)

    kwArgs.pop("radialPosition", None)
    kwArgs["label"] = "solo color"
    kwArgs["subMenu"] = True

    cmds.menuItem("menuEditorMenuItem{0}".format(len(lstCommands) + 1), **kwArgs)
    kwArgs["subMenu"] = False
    for ind, colType in enumerate(["white", "lava", "influence"]):
        kwArgs["label"] = colType
        kwArgs[
            "command"
        ] = "brSkinBrushContext -edit -soloColorType {} `currentCtx`;".format(ind)
        cmds.menuItem("menuEditorMenuItemCol{0}".format(ind + 1), **kwArgs)

    mel.eval("setParent -menu ..;")
    mel.eval("setParent -menu ..;")


def rootWindow():
    """
    Returns the currently active QT main window
    Only works for QT UIs like Maya
    """
    window = None
    if QApplication.instance():
        inst = QApplication.instance()
        window = inst.activeWindow()
        if isinstance(window, QSplashScreen):
            return None
        if window == None:
            windows = []
            dialogs = []
            for w in QApplication.instance().topLevelWidgets():
                if w.parent() == None:
                    if isinstance(w, QMainWindow):
                        windows.append(w)
                    elif isinstance(w, QDialog):
                        dialogs.append(w)
            if windows:
                window = windows[0]
            elif dialogs:
                window = dialogs[0]

        # grab the root window
        if window:
            while True:
                parent = window.parent()
                if not parent:
                    break
                if isinstance(parent, QSplashScreen):
                    break
                window = parent

    return window


class CatchEventsWidget(QtWidgets.QWidget):
    # transparent widget over viewport to catch rightclicks
    verbose = False
    filterInstalled = False
    displayLabel = None
    EventFilterWidgetReceiver = None
    lstButtons = [
        "brSkinBrushAddRb",
        "brSkinBrushRemoveRb",
        "brSkinBrushAddPercentRb",
        "brSkinBrushAbsoluteRb",
        "brSkinBrushSmoothRb",
        "brSkinBrushSharpenRb",
        "brSkinBrushLockVerticesRb",
        "brSkinBrushUnLockVerticesRb",
    ]

    def __init__(self):
        super(CatchEventsWidget, self).__init__(rootWindow())
        self.setMask(QtGui.QRegion(0, 0, 1, 1))

        self.OPressed = False
        self.markingMenuShown = False
        self.closingNextPressMarkingMenu = False
        self.CtrlOrShiftPressed = False

        self.rootWin = rootWindow()
        ptr = OpenMayaUI.MQtUtil.mainWindow()
        self.mainMaya = QtCompat.wrapInstance(PTR_TYPE(ptr), QtWidgets.QWidget)
        self.prevButton = self.lstButtons[0]

    # ---------- GAMMA --------------------------------------
    restorePanels = []

    def setPanelsDisplayOn(self):
        self.restorePanels = []
        for panel in cmds.getPanel(vis=True):
            if cmds.getPanel(to=panel) == "modelPanel":
                valDic = {}
                for key in [
                    "displayLights",
                    "cmEnabled",
                    "selectionHiliteDisplay",
                    "wireframeOnShaded",
                ]:
                    dic = {"query": True, key: True}
                    valDic[key] = cmds.modelEditor(panel, **dic)
                self.restorePanels.append((panel, valDic))
                cmds.modelEditor(
                    panel, edit=True, displayLights="flat", wireframeOnShaded=False
                )
                # GAMMA ENABLED
                cmds.modelEditor(panel, edit=True, cmEnabled=False)

    def setPanelsDisplayOff(self):
        for panel, valDic in self.restorePanels:
            cmds.modelEditor(panel, edit=True, **valDic)

    # ---------- end GAMMA --------------------------------------
    def open(self):
        if not self.filterInstalled:
            self.installFilters()
        self.setPanelsDisplayOn()
        self.show()

    def fermer(self):
        self.setPanelsDisplayOff()
        self.removeFilters()

    def installFilters(self):
        listModelPanels = [
            el for el in cmds.getPanel(vis=True) if cmds.getPanel(to=el) == "modelPanel"
        ]
        ptr = OpenMayaUI.MQtUtil.findControl(listModelPanels[0])
        model_panel_4 = QtCompat.wrapInstance(PTR_TYPE(ptr), QtWidgets.QWidget)
        self.EventFilterWidgetReceiver = model_panel_4.parent().parent()

        self.filterInstalled = True
        QApplication.instance().installEventFilter(self)

    def removeFilters(self):
        self.hide()
        self.filterInstalled = False
        QApplication.instance().removeEventFilter(self)

    def eventFilter(self, obj, event):
        if event.type() == QtCore.QEvent.MouseMove:
            event.ignore()
            return super(CatchEventsWidget, self).eventFilter(obj, event)

        if event.type() == QtCore.QEvent.KeyRelease:
            if self.CtrlOrShiftPressed and event.key() in [
                QtCore.Qt.Key_Shift,
                QtCore.Qt.Key_Control,
            ]:
                if self.verbose:
                    print("custom SHIFT or CONTROL released")
                self.CtrlOrShiftPressed = False
                if cmds.radioButton(self.prevButton, ex=True):
                    cmds.radioButton(self.prevButton, edit=True, select=True)

                if hasattr(self, "prevStrengthValue"):
                    try:
                        cmds.floatSliderGrp(
                            "brSkinBrushStrength",
                            edit=True,
                            value=self.prevStrengthValue,
                        )
                    except:
                        pass
            elif event.key() == QtCore.Qt.Key_U:
                if obj is self.EventFilterWidgetReceiver and self.OPressed:
                    self.OPressed = False
                    event.ignore()
                    return True
                return super(CatchEventsWidget, self).eventFilter(obj, event)
            return super(CatchEventsWidget, self).eventFilter(obj, event)

        if (
            event.type()
            in [QtCore.QEvent.MouseButtonPress, QtCore.QEvent.MouseButtonRelease]
            and event.modifiers() != QtCore.Qt.AltModifier
        ):
            if event.modifiers() == QtCore.Qt.NoModifier:  # regular click
                if event.type() == QtCore.QEvent.MouseButtonPress:  # click
                    if self.OPressed:
                        if not self.markingMenuShown:
                            callMarkingMenu()
                            self.markingMenuShown = True
                            self.closingNextPressMarkingMenu = False
                    elif self.closingNextPressMarkingMenu:
                        if cmds.popupMenu("tempMM", exists=True):
                            cmds.deleteUI("tempMM")
                        self.markingMenuShown = False
                        self.OPressed = False
                        self.closingNextPressMarkingMenu = False

                elif event.type() == QtCore.QEvent.MouseButtonRelease:  # click release
                    if self.markingMenuShown:
                        self.closingNextPressMarkingMenu = True
                return super(CatchEventsWidget, self).eventFilter(obj, event)
            return super(CatchEventsWidget, self).eventFilter(obj, event)

        if event.type() == QtCore.QEvent.KeyPress:
            if event.key() == QtCore.Qt.Key_P:  # print info of the click press
                active_view = OpenMayaUI.M3dView.active3dView()
                sw = active_view.widget()
                res = QtCompat.wrapInstance(PTR_TYPE(sw), QtWidgets.QWidget)

                listModelPanels = [
                    el
                    for el in cmds.getPanel(vis=True)
                    if cmds.getPanel(to=el) == "modelPanel"
                ]
                listModelPanelsCompats = [
                    QtCompat.wrapInstance(
                        PTR_TYPE(OpenMayaUI.MQtUtil.findControl(el)), QtWidgets.QWidget
                    )
                    for el in listModelPanels
                ]
                listModelPanelsCompatsPrts = [
                    el.parent() for el in listModelPanelsCompats
                ]

                if res is obj:
                    print("ViewPort")
                elif res is self.mainMaya:
                    print()
                elif obj is self.mainMaya:
                    print("self.mainMaya")
                elif obj is self:
                    print("self")
                elif obj is self.parent():
                    print("self Prt")
                elif obj is self.parent().parent():
                    print("self Prt Prt")
                elif obj is self.rootWin:
                    print("self.rootWin")
                elif obj in listModelPanelsCompats:
                    print("it is a model_panel")
                elif obj in listModelPanelsCompatsPrts:
                    print("it is a model_panel Parent")
                else:
                    print(obj)
                return super(CatchEventsWidget, self).eventFilter(obj, event)

            if event.key() == QtCore.Qt.Key_U:
                if self.OPressed:
                    event.ignore()
                    return True
                if obj is self.EventFilterWidgetReceiver:
                    self.OPressed = True
                    return True
                else:
                    return super(CatchEventsWidget, self).eventFilter(obj, event)

            elif event.key() == QtCore.Qt.Key_Escape:
                brSkinBrush_pythonFunctions.escapePressed()
                event.ignore()
                mel.eval("setToolTo $gMove;")
                return True

            elif event.key() == QtCore.Qt.Key_D:
                listModelPanels = [
                    el
                    for el in cmds.getPanel(vis=True)
                    if cmds.getPanel(to=el) == "modelPanel"
                ]
                listModelPanelsCompats = [
                    QtCompat.wrapInstance(
                        PTR_TYPE(OpenMayaUI.MQtUtil.findControl(el)), QtWidgets.QWidget
                    )
                    for el in listModelPanels
                ]
                listModelPanelsCompatsPrts = [
                    el.parent() for el in listModelPanelsCompats
                ]
                if obj in listModelPanelsCompats or obj in listModelPanelsCompatsPrts:
                    print("it is a model_panel")
                    event.ignore()

                    if event.modifiers() == QtCore.Qt.AltModifier:
                        mel.eval(
                            "brSkinBrushContext -edit -pickMaxInfluence 1 `currentCtx`;"
                        )
                    else:
                        mel.eval(
                            "brSkinBrushContext -edit -pickInfluence 1 `currentCtx`;"
                        )
                elif obj is self.mainMaya:
                    event.ignore()
                    return True

            elif event.key() == QtCore.Qt.Key_Control:
                if QApplication.mouseButtons() == QtCore.Qt.NoButton:
                    if self.verbose:
                        print("custom CONTROL pressed")
                    event.ignore()
                    self.prevButton = self.lstButtons[
                        cmds.brSkinBrushContext(
                            "brSkinBrushContext1", query=True, commandIndex=True
                        )
                    ]

                    if self.prevButton != "brSkinBrushSmoothRb":
                        self.CtrlOrShiftPressed = True

                        if cmds.radioButton("brSkinBrushSmoothRb", ex=True):
                            cmds.radioButton(
                                "brSkinBrushSmoothRb", edit=True, select=True
                            )
                        self.prevStrengthValue = cmds.brSkinBrushContext(
                            "brSkinBrushContext1", query=True, strength=True
                        )
                        smoothValue = cmds.brSkinBrushContext(
                            "brSkinBrushContext1", query=True, smoothStrength=True
                        )
                        try:
                            cmds.floatSliderGrp(
                                "brSkinBrushStrength", edit=True, value=smoothValue
                            )
                        except:
                            pass
                    return True

            elif event.key() == QtCore.Qt.Key_Shift and not self.CtrlOrShiftPressed:
                if QApplication.mouseButtons() == QtCore.Qt.NoButton:
                    if self.verbose:
                        print("custom SHIFT pressed")
                    event.ignore()
                    self.CtrlOrShiftPressed = True
                    self.prevButton = self.lstButtons[
                        cmds.brSkinBrushContext(
                            "brSkinBrushContext1", query=True, commandIndex=True
                        )
                    ]
                    try:
                        if (self.prevButton == "brSkinBrushAddRb") and cmds.radioButton(
                            "brSkinBrushRemoveRb", ex=True
                        ):
                            cmds.radioButton(
                                "brSkinBrushRemoveRb", edit=True, select=True
                            )

                        elif (
                            self.prevButton == "brSkinBrushLockVerticesRb"
                        ) and cmds.radioButton("brSkinBrushUnLockVerticesRb", ex=True):
                            cmds.radioButton(
                                "brSkinBrushUnLockVerticesRb", edit=True, select=True
                            )
                    except:
                        pass
                    return True

            elif event.modifiers() == QtCore.Qt.AltModifier:
                if event.key() == QtCore.Qt.Key_X:
                    listModelPanels = [
                        el
                        for el in cmds.getPanel(vis=True)
                        if cmds.getPanel(to=el) == "modelPanel"
                    ]
                    val = not cmds.modelEditor(
                        listModelPanels[0], query=True, jointXray=True
                    )
                    for pnel in listModelPanels:
                        cmds.modelEditor(pnel, edit=True, jointXray=val)
                    event.ignore()
                    return True

                if event.key() == QtCore.Qt.Key_W:

                    if cmds.objExists("SkinningWireframe"):
                        vis = cmds.getAttr("SkinningWireframe.v")
                        cmds.setAttr("SkinningWireframe.v", not vis)

                    event.ignore()
                    return True

                if event.key() == QtCore.Qt.Key_S:
                    brSkinBrush_pythonFunctions.toggleSoloMode()
                    event.ignore()
                    return True

                if event.key() == QtCore.Qt.Key_M:
                    print("mirror active")
                    event.ignore()
                    return True

            return super(CatchEventsWidget, self).eventFilter(obj, event)
        return super(CatchEventsWidget, self).eventFilter(obj, event)

    def closeEvent(self, e):
        """
        Make sure the eventFilter is removed
        """
        self.fermer()
        return super(CatchEventsWidget, self).closeEvent(e)
