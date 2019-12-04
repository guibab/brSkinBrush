// ---------------------------------------------------------------------
//
//  skinBrushTool.cpp
//  skinBrushTool
//
//  Created by ingo on 11/18/18.
//  Copyright (c) 2018 Ingo Clemens. All rights reserved.
//
// ---------------------------------------------------------------------

#include "skinBrushTool.h"

#include "skinBrushFlags.h"

// Macro for the press/drag/release methods in case there is nothing
// selected or the tool gets applied outside any geometry. If the actual
// MStatus would get returned an error can get listed in terminal on
// Linux. But it's unnecessary and needs to be avoided. Therefore a
// kSuccess is returned just for the sake of being invisible.
#define CHECK_MSTATUS_AND_RETURN_SILENT(status) \
    if (status != MStatus::kSuccess) return MStatus::kSuccess;

// ---------------------------------------------------------------------
// the tool
// ---------------------------------------------------------------------

// ---------------------------------------------------------------------
// general methods for the tool command
// ---------------------------------------------------------------------

skinBrushTool::skinBrushTool() {
    setCommandString("brSkinBrushCmd");

    affectSelectedVal = true;
    colorVal = MColor(1.0, 0.0, 0.0);
    curveVal = 2;
    depthVal = 1;
    depthStartVal = 1;
    drawBrushVal = true;
    drawRangeVal = true;
    enterToolCommandVal = "";
    exitToolCommandVal = "";
    fractionOversamplingVal = false;
    ignoreLockVal = false;
    keepShellsTogetherVal = true;
    lineWidthVal = 1;
    messageVal = 2;
    oversamplingVal = 1;
    rangeVal = 0.5;
    sizeVal = 5.0;
    strengthVal = 0.25;
    smoothStrengthVal = 1.0;
    toleranceVal = 0.001;
    undersamplingVal = 2;
    volumeVal = false;
    stepsToDrawLineVal = 4;
    coverageVal = true;

    pruneWeights = 0.0001;
    commandIndex = 0;      // add
    soloColorTypeVal = 1;  // 1 lava
    soloColorVal = 0;
    postSetting = true;
}

skinBrushTool::~skinBrushTool() {}

void* skinBrushTool::creator() { return new skinBrushTool; }

bool skinBrushTool::isUndoable() const { return true; }

MSyntax skinBrushTool::newSyntax() {
    MSyntax syntax;

    syntax.addFlag(kAffectSelectedFlag, kAffectSelectedFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kColorRFlag, kColorRFlagLong, MSyntax::kDouble);
    syntax.addFlag(kColorGFlag, kColorGFlagLong, MSyntax::kDouble);
    syntax.addFlag(kColorBFlag, kColorBFlagLong, MSyntax::kDouble);
    syntax.addFlag(kCurveFlag, kCurveFlagLong, MSyntax::kLong);
    syntax.addFlag(kDepthFlag, kDepthFlagLong, MSyntax::kLong);
    syntax.addFlag(kDepthStartFlag, kDepthStartFlagLong, MSyntax::kLong);
    syntax.addFlag(kDrawBrushFlag, kDrawBrushFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kDrawRangeFlag, kDrawRangeFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kEnterToolCommandFlag, kEnterToolCommandFlagLong, MSyntax::kString);
    syntax.addFlag(kExitToolCommandFlag, kExitToolCommandFlagLong, MSyntax::kString);
    syntax.addFlag(kFractionOversamplingFlag, kFractionOversamplingFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kIgnoreLockFlag, kIgnoreLockFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kKeepShellsTogetherFlag, kKeepShellsTogetherFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kLineWidthFlag, kLineWidthFlagLong, MSyntax::kLong);
    syntax.addFlag(kMessageFlag, kMessageFlagLong, MSyntax::kLong);
    syntax.addFlag(kOversamplingFlag, kOversamplingFlagLong, MSyntax::kLong);
    syntax.addFlag(kRangeFlag, kRangeFlagLong, MSyntax::kDouble);
    syntax.addFlag(kSizeFlag, kSizeFlagLong, MSyntax::kDouble);
    syntax.addFlag(kStrengthFlag, kStrengthFlagLong, MSyntax::kDouble);
    syntax.addFlag(kToleranceFlag, kToleranceFlagLong, MSyntax::kDouble);
    syntax.addFlag(kUndersamplingFlag, kUndersamplingFlagLong, MSyntax::kLong);
    syntax.addFlag(kVolumeFlag, kVolumeFlagLong, MSyntax::kBoolean);

    syntax.addFlag(kPruneWeightsFlag, kPruneWeightsFlagLong, MSyntax::kDouble);
    syntax.addFlag(kSmoothStrengthFlag, kSmoothStrengthFlagLong, MSyntax::kDouble);
    syntax.addFlag(kCommandIndexFlag, kCommandIndexFlagLong, MSyntax::kLong);
    syntax.addFlag(kStepsLineFlag, kStepsLineLong, MSyntax::kLong);
    syntax.addFlag(kSoloColorFlag, kSoloColorFlagLong, MSyntax::kLong);
    syntax.addFlag(kSoloColorTypeFlag, kSoloColorTypeFlagLong, MSyntax::kLong);
    syntax.addFlag(kCoverageFlag, kCoverageLong, MSyntax::kBoolean);
    // syntax.addFlag(kPickMaxInfluenceFlag, kPickMaxInfluenceFlagLong, MSyntax::kBoolean);
    syntax.addFlag(kInfluenceIndexFlag, kInfluenceIndexFlagLong, MSyntax::kLong);
    syntax.addFlag(kPostSettingFlag, kPostSettingFlagLong, MSyntax::kBoolean);

    return syntax;
}

MStatus skinBrushTool::parseArgs(const MArgList& args) {
    MStatus status = MStatus::kSuccess;

    MArgDatabase argData(syntax(), args);

    if (argData.isFlagSet(kAffectSelectedFlag)) {
        status = argData.getFlagArgument(kAffectSelectedFlag, 0, affectSelectedVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kColorRFlag)) {
        double value;
        status = argData.getFlagArgument(kColorRFlag, 0, value);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        colorVal = MColor((float)value, colorVal.g, colorVal.b);
    }
    if (argData.isFlagSet(kColorGFlag)) {
        double value;
        status = argData.getFlagArgument(kColorGFlag, 0, value);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        colorVal = MColor(colorVal.r, (float)value, colorVal.b);
    }
    if (argData.isFlagSet(kColorBFlag)) {
        double value;
        status = argData.getFlagArgument(kColorBFlag, 0, value);
        CHECK_MSTATUS_AND_RETURN_IT(status);
        colorVal = MColor(colorVal.r, colorVal.g, (float)value);
    }
    if (argData.isFlagSet(kCurveFlag)) {
        status = argData.getFlagArgument(kCurveFlag, 0, curveVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kDepthFlag)) {
        status = argData.getFlagArgument(kDepthFlag, 0, depthVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kDepthStartFlag)) {
        status = argData.getFlagArgument(kDepthStartFlag, 0, depthStartVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kDrawBrushFlag)) {
        status = argData.getFlagArgument(kDrawBrushFlag, 0, drawBrushVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kDrawRangeFlag)) {
        status = argData.getFlagArgument(kDrawRangeFlag, 0, drawRangeVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kEnterToolCommandFlag)) {
        status = argData.getFlagArgument(kEnterToolCommandFlag, 0, enterToolCommandVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kExitToolCommandFlag)) {
        status = argData.getFlagArgument(kExitToolCommandFlag, 0, exitToolCommandVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kFractionOversamplingFlag)) {
        status = argData.getFlagArgument(kFractionOversamplingFlag, 0, fractionOversamplingVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kIgnoreLockFlag)) {
        status = argData.getFlagArgument(kIgnoreLockFlag, 0, ignoreLockVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kKeepShellsTogetherFlag)) {
        status = argData.getFlagArgument(kKeepShellsTogetherFlag, 0, keepShellsTogetherVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kLineWidthFlag)) {
        status = argData.getFlagArgument(kLineWidthFlag, 0, lineWidthVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kMessageFlag)) {
        status = argData.getFlagArgument(kMessageFlag, 0, messageVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kOversamplingFlag)) {
        status = argData.getFlagArgument(kOversamplingFlag, 0, oversamplingVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kRangeFlag)) {
        status = argData.getFlagArgument(kRangeFlag, 0, rangeVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kSizeFlag)) {
        status = argData.getFlagArgument(kSizeFlag, 0, sizeVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kStrengthFlag)) {
        status = argData.getFlagArgument(kStrengthFlag, 0, strengthVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kToleranceFlag)) {
        status = argData.getFlagArgument(kToleranceFlag, 0, toleranceVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kPruneWeightsFlag)) {
        status = argData.getFlagArgument(kPruneWeightsFlag, 0, pruneWeights);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kUndersamplingFlag)) {
        status = argData.getFlagArgument(kUndersamplingFlag, 0, undersamplingVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kVolumeFlag)) {
        status = argData.getFlagArgument(kVolumeFlag, 0, volumeVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kSmoothStrengthFlag)) {
        status = argData.getFlagArgument(kSmoothStrengthFlag, 0, smoothStrengthVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    if (argData.isFlagSet(kStepsLineFlag)) {
        status = argData.getFlagArgument(kStepsLineFlag, 0, stepsToDrawLineVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kCoverageFlag)) {
        status = argData.getFlagArgument(kCoverageFlag, 0, coverageVal);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }
    if (argData.isFlagSet(kInfluenceIndexFlag)) {
        status = argData.getFlagArgument(kInfluenceIndexFlag, 0, influenceIndex);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    return status;
}

// ---------------------------------------------------------------------
// main methods for the tool command
// ---------------------------------------------------------------------

MStatus skinBrushTool::doIt(const MArgList& args) {
    // MGlobal::displayInfo(MString("---------------- [skinBrushTool::doIt]------------------"));

    MStatus status = MStatus::kSuccess;

    status = parseArgs(args);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return redoIt();
}

MStatus skinBrushTool::redoIt() {
    MStatus status = MStatus::kSuccess;

    MGlobal::displayInfo(MString("skinBrushTool::redoIt is CALLED !!!! commandIndex : ") +
                         this->commandIndex);
    if (this->commandIndex >= 6) MGlobal::displayInfo("lock / unlock vertices");

    // Apply the redo weights and get the current weights for undo.
    MFnSkinCluster skinFn(skinObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (this->commandIndex < 6 && this->redoWeights.length()) {
        MFnSingleIndexedComponent compFn;
        MObject weightsObj = compFn.create(MFn::kMeshVertComponent);
        compFn.addElements(this->undoVertices);

        skinFn.setWeights(meshDag, weightsObj, influenceIndices, redoWeights, true);  // normalize);
        // refresh the tool points positions ---------
    }
    if (this->commandIndex >= 6) {
        MGlobal::displayInfo("lock / unlock vertices");

        MObjectArray objectsDeformed;
        skinFn.getOutputGeometry(objectsDeformed);
        MFnDependencyNode deformedNameMesh(objectsDeformed[0]);
        MPlug lockedVerticesPlug = deformedNameMesh.findPlug("lockedVertices", &stat);
        if (MS::kSuccess != status) {
            MGlobal::displayError(MString("cant find lockerdVertices plug"));
            return status;
        }
        // now set the value ---------------------------
        MFnIntArrayData tmpIntArray;

        MIntArray theArrayValues;
        for (unsigned int vtx = 0; vtx < redoLocks.length(); ++vtx) {
            if (redoLocks[vtx] == 1) theArrayValues.append(vtx);
        }
        status = lockedVerticesPlug.setValue(
            tmpIntArray.create(theArrayValues));  // to set the attribute

        return status;
    }
    callBrushRefresh();
    return MStatus::kSuccess;
}

MStatus skinBrushTool::undoIt() {
    MStatus status = MStatus::kSuccess;

    MGlobal::displayInfo(MString("skinBrushTool::undoIt is CALLED ! commandIndex : ") +
                         this->commandIndex);

    MFnSkinCluster skinFn(skinObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    if (this->commandIndex < 6 && this->undoWeights.length()) {
        MFnSingleIndexedComponent compFn;
        MObject weightsObj = compFn.create(MFn::kMeshVertComponent);
        compFn.addElements(this->undoVertices);

        // Apply the previous weights and get the current weights for
        // redo.
        // skinFn.setWeights(meshDag, weightsObj, influenceIndices, this->undoWeights, normalize,
        // &redoWeights);
        skinFn.setWeights(meshDag, weightsObj, influenceIndices, this->undoWeights, true,
                          &redoWeights);
    }

    if (this->commandIndex >= 6) {
        MGlobal::displayInfo("lock / unlock vertices");

        MObjectArray objectsDeformed;
        skinFn.getOutputGeometry(objectsDeformed);
        MFnDependencyNode deformedNameMesh(objectsDeformed[0]);
        MPlug lockedVerticesPlug = deformedNameMesh.findPlug("lockedVertices", &stat);
        if (MS::kSuccess != status) {
            MGlobal::displayError(MString("cant find lockerdVertices plug"));
            return status;
        }
        // now set the value ---------------------------
        MFnIntArrayData tmpIntArray;

        MIntArray theArrayValues;
        for (unsigned int vtx = 0; vtx < undoLocks.length(); ++vtx) {
            if (undoLocks[vtx] == 1) theArrayValues.append(vtx);
        }

        status = lockedVerticesPlug.setValue(
            tmpIntArray.create(theArrayValues));  // to set the attribute
        return status;
    }
    callBrushRefresh();
    /*
    MGlobal::getActiveSelectionList(redoSelection);
    MGlobal::getHiliteList(redoHilite);

    MGlobal::setActiveSelectionList(undoSelection);
    MGlobal::setHiliteList(undoHilite);
    */
    return MStatus::kSuccess;
}
MStatus skinBrushTool::callBrushRefresh() {
    /*
    ----------------
    ---------------- VERY IMPORTANT
    ---------------- refresh the tool points positions, normals, skin weight stored, and vertex
    colors  ---------
    ----------------
    ----------------
    */

    MString cmd;
    // cmd = "brSkinBrushContext -edit -refresh 1 brSkinBrushContext1;";
    // cmd = "evalDeferred  ( \"brSkinBrushContext -edit -refresh 1 brSkinBrushContext1\");";
    // cmd = "brSkinBrushContext -edit -refreshPointsNormals 1 brSkinBrushContext1;";
    // cmd = "evalDeferred  ( \"brSkinBrushContext -edit -refreshPointsNormals 1
    // brSkinBrushContext1\");"; MGlobal::executeCommandOnIdle(cmd);
    MString lst = "";
    int i, nbElements;
    nbElements = this->undoVertices.length();
    if (nbElements > 0) {
        for (i = 0; i < (nbElements - 1); ++i) {
            lst += this->undoVertices[i];
            lst += MString(", ");
        }
        lst += this->undoVertices[nbElements - 1];
        cmd =
            MString(
                "cmds.brSkinBrushContext('brSkinBrushContext1', e=True, listVerticesIndices = [") +
            lst + MString("])");

        /*
        MString melCmd = MString ("evalDeferred(\"python (\\\"")+ cmd +MString("\\\")\");" ) ;
        MGlobal::displayInfo(melCmd);
        MGlobal::executeCommandOnIdle(melCmd);
        */
        MGlobal::executePythonCommandOnIdle(cmd);
    }

    return MStatus::kSuccess;
}

MStatus skinBrushTool::finalize() {
    // Store the current settings as an option var. This way they are
    // properly available for the next usage.
    // MGlobal::displayInfo("skinBrushTool::finalize\n");
    MString cmd;
    cmd = "brSkinBrushContext -edit ";
    cmd += "-image1 \"brSkinBrush.svg\" -image2 \"vacantCell.png\" -image3 \"vacantCell.png\"";
    cmd += " " + MString(kAffectSelectedFlag) + " ";
    cmd += affectSelectedVal;
    cmd += " " + MString(kColorRFlag) + " ";
    cmd += colorVal.r;
    cmd += " " + MString(kColorGFlag) + " ";
    cmd += colorVal.g;
    cmd += " " + MString(kColorBFlag) + " ";
    cmd += colorVal.b;
    cmd += " " + MString(kCurveFlag) + " ";
    cmd += curveVal;
    cmd += " " + MString(kDepthFlag) + " ";
    cmd += depthVal;
    cmd += " " + MString(kDepthStartFlag) + " ";
    cmd += depthStartVal;
    cmd += " " + MString(kDrawBrushFlag) + " ";
    cmd += drawBrushVal;
    cmd += " " + MString(kDrawRangeFlag) + " ";
    cmd += drawRangeVal;
    cmd += " " + MString(kEnterToolCommandFlag) + " ";
    cmd += "\"" + enterToolCommandVal + "\"";
    cmd += " " + MString(kExitToolCommandFlag) + " ";
    cmd += "\"" + exitToolCommandVal + "\"";
    cmd += " " + MString(kFractionOversamplingFlag) + " ";
    cmd += fractionOversamplingVal;
    cmd += " " + MString(kIgnoreLockFlag) + " ";
    cmd += ignoreLockVal;
    cmd += " " + MString(kKeepShellsTogetherFlag) + " ";
    cmd += keepShellsTogetherVal;
    cmd += " " + MString(kLineWidthFlag) + " ";
    cmd += lineWidthVal;
    cmd += " " + MString(kMessageFlag) + " ";
    cmd += messageVal;
    cmd += " " + MString(kOversamplingFlag) + " ";
    cmd += oversamplingVal;
    cmd += " " + MString(kRangeFlag) + " ";
    cmd += rangeVal;
    cmd += " " + MString(kSizeFlag) + " ";
    cmd += sizeVal;
    cmd += " " + MString(kStrengthFlag) + " ";
    cmd += strengthVal;
    cmd += " " + MString(kSmoothStrengthFlag) + " ";
    cmd += smoothStrengthVal;
    cmd += " " + MString(kToleranceFlag) + " ";
    cmd += toleranceVal;
    cmd += " " + MString(kPruneWeightsFlag) + " ";
    cmd += pruneWeights;
    cmd += " " + MString(kUndersamplingFlag) + " ";
    cmd += undersamplingVal;
    cmd += " " + MString(kVolumeFlag) + " ";
    cmd += volumeVal;
    cmd += " " + MString(kStepsLineFlag) + " ";
    cmd += stepsToDrawLineVal;
    cmd += " " + MString(kPostSettingFlag) + " ";
    cmd += postSetting;

    cmd += " brSkinBrushContext1;";

    MGlobal::setOptionVarValue("brSkinBrushContext1", cmd);

    // Finalize the command by adding it to the undo queue and the
    // journal.
    MArgList command;
    command.addArg(commandString());

    return MPxToolCommand::doFinalize(command);
}

// ---------------------------------------------------------------------
// getting values from the command flags
// ---------------------------------------------------------------------

void skinBrushTool::setAffectSelected(bool value) { affectSelectedVal = value; }

void skinBrushTool::setColor(MColor value) { colorVal = value; }

void skinBrushTool::setCurve(int value) { curveVal = value; }

void skinBrushTool::setDepth(int value) { depthVal = value; }

void skinBrushTool::setDepthStart(int value) { depthStartVal = value; }

void skinBrushTool::setDrawBrush(bool value) { drawBrushVal = value; }

void skinBrushTool::setDrawRange(bool value) { drawRangeVal = value; }

void skinBrushTool::setEnterToolCommand(MString value) { enterToolCommandVal = value; }

void skinBrushTool::setExitToolCommand(MString value) { exitToolCommandVal = value; }

void skinBrushTool::setFractionOversampling(bool value) { fractionOversamplingVal = value; }

void skinBrushTool::setIgnoreLock(bool value) { ignoreLockVal = value; }

void skinBrushTool::setKeepShellsTogether(bool value) { keepShellsTogetherVal = value; }

void skinBrushTool::setLineWidth(int value) { lineWidthVal = value; }

void skinBrushTool::setMessage(int value) { messageVal = value; }

void skinBrushTool::setOversampling(int value) { oversamplingVal = value; }

void skinBrushTool::setRange(double value) { rangeVal = value; }

void skinBrushTool::setSize(double value) { sizeVal = value; }

void skinBrushTool::setStrength(double value) { strengthVal = value; }

void skinBrushTool::setSmoothStrength(double value) { smoothStrengthVal = value; }

void skinBrushTool::setTolerance(double value) { toleranceVal = value; }

void skinBrushTool::setPruneWeights(double value) { pruneWeights = value; }

void skinBrushTool::setUndersampling(int value) { undersamplingVal = value; }

void skinBrushTool::setVolume(bool value) { volumeVal = value; }

void skinBrushTool::setCommandIndex(int value) { commandIndex = value; }

void skinBrushTool::setSoloColorType(int value) { soloColorTypeVal = value; }

void skinBrushTool::setSoloColor(int value) { soloColorVal = value; }

void skinBrushTool::setStepLine(int value) { stepsToDrawLineVal = value; }

void skinBrushTool::setCoverage(bool value) { coverageVal = value; }

void skinBrushTool::setPostSetting(bool value) { postSetting = value; }

// ---------------------------------------------------------------------
// public methods for setting the undo/redo variables
// ---------------------------------------------------------------------

void skinBrushTool::setInfluenceIndices(MIntArray indices) { influenceIndices = indices; }

void skinBrushTool::setMesh(MDagPath dagPath) { meshDag = dagPath; }

void skinBrushTool::setNormalize(bool value) { normalize = value; }

void skinBrushTool::setSelection(MSelectionList selection, MSelectionList hilite) {
    undoSelection = selection;
    undoHilite = hilite;
}

void skinBrushTool::setSkinCluster(MObject skinCluster) { skinObj = skinCluster; }

void skinBrushTool::setVertexComponents(MObject components) { vertexComponents = components; }

void skinBrushTool::setWeights(MDoubleArray weights) { undoWeights = weights; }

void skinBrushTool::setUnoVertices(MIntArray editVertsIndices) { undoVertices = editVertsIndices; }

void skinBrushTool::setUnoLocks(MIntArray locks) { undoLocks = locks; }

void skinBrushTool::setRedoLocks(MIntArray locks) { redoLocks = locks; }

// ---------------------------------------------------------------------
// MIT License
//
// Copyright (c) 2018 Ingo Clemens, brave rabbit
// brSkinBrush is under the terms of the MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// Author: Ingo Clemens    www.braverabbit.com
// ---------------------------------------------------------------------
