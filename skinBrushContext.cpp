
#include "skinBrushFlags.h"
#include "skinBrushTool.h"

// ---------------------------------------------------------------------
// the context
// ---------------------------------------------------------------------

const char helpString[] = "it's a custom brush weights.";

// ---------------------------------------------------------------------
// general methods when calling the context
// ---------------------------------------------------------------------
SkinBrushContext::SkinBrushContext() {
    setTitleString("Smooth Weights");
    setImage("brSkinBrush.svg", MPxContext::kImage1);
    setCursor(MCursor::editCursor);

    // Define the default values for the context.
    // These values will be used to reset the tool from the tool
    // properties window.
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

    pruneWeight = 0.0001;

    toleranceVal = 0.001;
    undersamplingVal = 2;
    stepsToDrawLineVal = 4;
    volumeVal = false;
    coverageVal = true;
    postSetting = true;

    commandIndex = 0;      // add
    soloColorTypeVal = 1;  // 1 lava
    soloColorVal = 0;
    // True, only if the smoothing is performed. False when adjusting
    // the brush settings. It's used to control whether undo/redo needs
    // to get called.
    performBrush = false;
}

void SkinBrushContext::toolOnSetup(MEvent &) {
    if (verbose)
        MGlobal::displayInfo(
            MString("---------------- [SkinBrushContext::toolOnSetup ()]------------------"));

    MStatus status = MStatus::kSuccess;

    setHelpString(helpString);
    setInViewMessage(true);

    MGlobal::executeCommand(enterToolCommandVal);

    MString cmdtoolOnSetupStart(
        "from brSkinBrush_pythonFunctions import toolOnSetupStart\ntoolOnSetupStart()\n");
    MGlobal::executePythonCommand(cmdtoolOnSetupStart);

    // command to check size of text for picking
    MString cmdFont("from brSkinBrush_pythonFunctions import fnFonts\n");
    MGlobal::executePythonCommand(cmdFont);

    MString cmdCatchEventsUI("import catchEventsUI\n");
    cmdCatchEventsUI += MString("reload(catchEventsUI)\n");
    cmdCatchEventsUI += MString("catchEventsUI.EVENTCATCHER = catchEventsUI.CatchEventsWidget()\n");
    cmdCatchEventsUI += MString("catchEventsUI.EVENTCATCHER.open()\n");
    MGlobal::executePythonCommand(cmdCatchEventsUI);

    /*
    cmd = MString("import catchEventsUI\n");
    cmd += MString("if not hasattr(catchEventsUI, \"EVENTCATCHER\") : catchEventsUI.EVENTCATCHER =
    catchEventsUI.CatchEventsWidget()\n"); cmd += MString("catchEventsUI.EVENTCATCHER.open()\n");
    */

    // MGlobal::executeCommand("brSkinBrushClear();");
    this->pickMaxInfluenceVal = false;
    this->pickInfluenceVal = false;

    status = getMesh();

    if (!skinObj.isNull()) {
        getListColorsJoints(skinObj, jointsColors, verbose);  // get the joints colors
        status = fillArrayValues(skinObj, true);  // get the skin data and all the colors

        this->lockJoints = MIntArray(this->nbJoints, 0);
        this->ignoreLockJoints = MIntArray(this->nbJoints, 0);

        getListLockJoints(skinObj, this->lockJoints);
        getListLockVertices(skinObj, this->lockVertices);

        if (verbose)
            MGlobal::displayInfo(MString("nb found joints colors ") + jointsColors.length());
    } else {
        MGlobal::displayInfo(MString("FAILED : skinObj.isNull"));
        abortAction();
        return;
    }
    // get face color assignments ----------
    MIntArray VertexCountPerPolygon, fullVertexList;
    meshFn.getVertices(VertexCountPerPolygon, fullVertexList);
    this->fullVertexListLength = fullVertexList.length();

    // this->multiCurrentColors = MColorArray (this->numVertices, MColor(0.0, 0.0, 0.0, 1.0));
    // solo colors -----------------------
    this->soloCurrentColors = MColorArray(this->numVertices, MColor(0.0, 0, 0.0));
    this->soloColorsValues = MDoubleArray(this->numVertices, 0.0);

    MStringArray currentColorSets;
    meshFn.getColorSetNames(currentColorSets);
    if (currentColorSets.indexOf(this->fullColorSet) == -1)  // multiColor
        meshFn.createColorSetWithName(this->fullColorSet);

    if (currentColorSets.indexOf(this->soloColorSet) == -1)  // soloColor
        meshFn.createColorSetWithName(this->soloColorSet);

    if (currentColorSets.indexOf(this->fullColorSet2) == -1)  // multiColor
        meshFn.createColorSetWithName(this->fullColorSet2);

    if (currentColorSets.indexOf(this->soloColorSet2) == -1)  // soloColor
        meshFn.createColorSetWithName(this->soloColorSet2);

    /*
    if (currentColorSets.indexOf(this->noColorSet) == -1)  // noColor
            meshFn.createColorSetWithName(this->noColorSet);
    */

    meshFn.setColors(this->multiCurrentColors, &this->fullColorSet);  // set the multi assignation
    meshFn.assignColors(fullVertexList, &this->fullColorSet);

    meshFn.setColors(this->soloCurrentColors, &this->soloColorSet);  // set the solo assignation
    meshFn.assignColors(fullVertexList, &this->soloColorSet);

    meshFn.setColors(this->multiCurrentColors, &this->fullColorSet2);  // set the multi assignation
    meshFn.assignColors(fullVertexList, &this->fullColorSet2);

    meshFn.setColors(this->soloCurrentColors, &this->soloColorSet2);  // set the solo assignation
    meshFn.assignColors(fullVertexList, &this->soloColorSet2);

    /*
    meshFn.setColors(this->soloCurrentColors, &this->noColorSet); // set the solo assignation
    meshFn.assignColors(fullVertexList, &this->noColorSet);
    */

    MString currentColorSet = meshFn.currentColorSetName();  // set multiColor as current Color
    if (soloColorVal == 1) {                                 // solo
        if (currentColorSet != this->soloColorSet)
            meshFn.setCurrentColorSetName(this->soloColorSet);
        editSoloColorSet(true);
    } else {
        if (currentColorSet != this->fullColorSet)
            meshFn.setCurrentColorSetName(this->fullColorSet);  // , &this->colorSetMod);
    }

    // display the locks
    MColorArray multiEditColors, soloEditColors;
    MIntArray editVertsIndices;
    for (int i = 0; i < this->numVertices; ++i) {
        if (this->lockVertices[i]) editVertsIndices.append(i);
    }
    refreshColors(editVertsIndices, multiEditColors, soloEditColors);
    this->skinValuesToSet.clear();

    meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet);
    meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet);

    meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet2);
    meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet2);

    MString cmdtoolOnSetupEnd(
        "from brSkinBrush_pythonFunctions import toolOnSetupEnd\ntoolOnSetupEnd()\n");
    MGlobal::executePythonCommand(cmdtoolOnSetupEnd);

    meshFn.setDisplayColors(true);

    view = M3dView::active3dView();
    view.refresh(false, true);
}

void SkinBrushContext::toolOffCleanup() {
    setInViewMessage(false);
    MGlobal::executeCommand(exitToolCommandVal);

    MString cmd("import catchEventsUI\n");
    cmd += MString(
        "if hasattr ( catchEventsUI,\"EVENTCATCHER\"): catchEventsUI.EVENTCATCHER.close()\n");
    MGlobal::executePythonCommand(cmd);

    cmd =
        MString("import brSkinBrush_pythonFunctions\nbrSkinBrush_pythonFunctions.toolOffCleanup()");
    MGlobal::executePythonCommand(cmd);
}

void SkinBrushContext::getClassName(MString &name) const { name.set("brSkinBrush"); }

void SkinBrushContext::refreshJointsLocks() {
    if (verbose) MGlobal::displayInfo(" - refreshJointsLocks-");
    if (!skinObj.isNull()) {
        // Get the skin cluster node from the history of the mesh.
        getListLockJoints(skinObj, this->lockJoints);
    }
}
void SkinBrushContext::refreshTheseVertices(MIntArray verticesIndices) {
    /*
    if (verbose) MGlobal::displayInfo(" - refreshThisVertices-");
    MString toDisplay("List Vertices : ");
    int nbVerts = verticesIndices.length();
    for (int i = 0; i < nbVerts; ++i) {
            int vtxIndex = verticesIndices[i];
            toDisplay += vtxIndex;
            toDisplay += MString(" - ");
    }
    MGlobal::displayInfo(toDisplay);
    */

    querySkinClusterValues(this->skinObj, verticesIndices, true);

    refreshPointsNormals();

    MColorArray multiEditColors, soloEditColors;
    refreshColors(verticesIndices, multiEditColors, soloEditColors);
    this->skinValuesToSet.clear();
    meshFn.setSomeColors(verticesIndices, multiEditColors, &this->fullColorSet);
    meshFn.setSomeColors(verticesIndices, soloEditColors, &this->soloColorSet);

    meshFn.setSomeColors(verticesIndices, multiEditColors, &this->fullColorSet2);
    meshFn.setSomeColors(verticesIndices, soloEditColors, &this->soloColorSet2);

    // refresh view and display
    // meshFn.setDisplayColors(true);
    view.refresh(false, true);

    this->previousPaint.clear();
}

void SkinBrushContext::refresh() {
    MStatus status;
    if (verbose) MGlobal::displayInfo(" - REFRESH In CPP -");
    // refresh skinCluster
    //
    refreshPointsNormals();

    if (!skinObj.isNull()) {
        // Get the skin cluster node from the history of the mesh.
        getListLockJoints(skinObj, this->lockJoints);
        getListColorsJoints(skinObj, this->jointsColors, this->verbose);  // get the joints colors
        status = getListLockVertices(skinObj, this->lockVertices);        // problem ?
        if (MS::kSuccess != status) {
            MGlobal::displayError(MString("error getListLockVertices"));
        }
        status = fillArrayValues(skinObj, true);  // get the skin data and all the colors
    } else {
        MGlobal::displayInfo(MString("FAILED : skinObj.isNull"));
        return;
    }

    this->skinValuesToSet.clear();

    meshFn.setColors(this->multiCurrentColors, &this->fullColorSet);  // set the multi assignation
    meshFn.setColors(this->soloCurrentColors, &this->soloColorSet);   // set the solo assignation

    meshFn.setColors(this->multiCurrentColors, &this->fullColorSet2);  // set the multi assignation
    meshFn.setColors(this->soloCurrentColors, &this->soloColorSet2);   // set the solo assignation

    // display the locks ----------------------
    MColorArray multiEditColors, soloEditColors;
    MIntArray editVertsIndices;
    for (int i = 0; i < this->numVertices; ++i) {
        if (this->lockVertices[i]) editVertsIndices.append(i);
    }
    refreshColors(editVertsIndices, multiEditColors, soloEditColors);
    meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet);
    meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet);

    meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet2);
    meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet2);

    if (soloColorVal == 1) editSoloColorSet(true);  // solo

    // refresh view and display
    meshFn.setDisplayColors(true);

    view = M3dView::active3dView();
    view.refresh(false, true);

    maya2019RefreshColors();
}

// ---------------------------------------------------------------------
// viewport 2.0
// ---------------------------------------------------------------------
int SkinBrushContext::getClosestInfluenceToCursor(int screenX, int screenY) {
    MStatus stat;
    int lent = this->inflDagPaths.length();
    MPoint zero(0, 0, 0);
    short x_pos, y_pos;

    int closest = 200;
    int closestOffset = 20;

    int closestInfluence = -1;

    // store pixels positions -----------
    std::vector<std::pair<short, short>> pixelsPosition;
    unsigned int i;
    /*
    if (this->tmpTestPrintClosestInfluence)
    {
            MGlobal::displayInfo(MString(" POSI    [") + screenX + MString(" | ") + screenY +
    MString("]     ") );
    }
    */

    for (unsigned int i = 0; i < lent; i++) {
        MPoint influencePosi = zero * this->inflDagPaths[i].inclusiveMatrix();
        view.worldToView(influencePosi, x_pos, y_pos, &stat);
        if (MStatus::kSuccess == stat) {
            auto myPair = std::make_pair(x_pos, y_pos);
            /*
            if (this->tmpTestPrintClosestInfluence)
            {
                    MGlobal::displayInfo(MString("    [") + x_pos + MString(" | ") + y_pos +
            MString("]     ") + this->inflNames[i]  );
            }
            */
            auto it = std::find(pixelsPosition.begin(), pixelsPosition.end(), myPair);
            if (it != pixelsPosition.end()) {
                myPair = std::make_pair(x_pos + closestOffset, y_pos + closestOffset);  // offset
                it = std::find(pixelsPosition.begin(), pixelsPosition.end(), myPair);
                if (it != pixelsPosition.end()) {
                    myPair =
                        std::make_pair(x_pos - closestOffset, y_pos - closestOffset);  // offset
                }
            }
            pixelsPosition.push_back(myPair);
        } else {
            pixelsPosition.push_back(std::make_pair(-1, -1));
        }
    }
    // now do the search -----------
    i = 0;
    for (auto mypair : pixelsPosition) {
        x_pos = mypair.first;
        y_pos = mypair.second;

        if (x_pos > 0 && y_pos > 0) {
            int dst = pow((x_pos - screenX), 2) + pow((y_pos - screenY), 2);
            if (dst < closest) {
                closestInfluence = i;
                closest = dst;
            }
        }
        i++;
    }

    return closestInfluence;
}

int SkinBrushContext::getHighestInfluence(int faceHit, MFloatPoint hitPoint) {
    // get closest vertex
    auto verticesSet = this->FaceToVerticesSet[faceHit];
    int indexVertex = -1;
    float closestDist;
    for (int ptIndex : verticesSet) {
        MFloatPoint posPoint(this->mayaRawPoints[ptIndex * 3], this->mayaRawPoints[ptIndex * 3 + 1],
                             this->mayaRawPoints[ptIndex * 3 + 2]);
        float dist = posPoint.distanceTo(hitPoint);
        if (indexVertex == -1 || dist < closestDist) {
            indexVertex = ptIndex;
            closestDist = dist;
        }
    }
    // now get highest influence for this vertex

    int biggestInfluence;
    double biggestVal = 0;

    for (int indexInfluence = 0; indexInfluence < this->nbJoints; ++indexInfluence) {
        double theWeight = this->skinWeightList[indexVertex * this->nbJoints + indexInfluence];
        if (theWeight > biggestVal) {
            biggestVal = theWeight;
            biggestInfluence = indexInfluence;
        }
    }
    /*
    auto weights = this->skin_weights_[indexVertex];
    for (const auto &elem : weights) {
            if (elem.second > biggestVal) {
                    biggestInfluence = elem.first;
                    biggestVal = elem.second;
            }
    }
    */
    return biggestInfluence;
}
MStatus SkinBrushContext::drawTheMesh(MHWRender::MUIDrawManager &drawManager, MVector worldVector) {
    drawManager.setColor(MColor(0.5, 0.5, 0.5, 1.0));
    drawManager.setLineWidth(1);

    MPointArray edgeVertices;  //(this->edgeVerticesIndices.size());
    unsigned int i = 0;
    for (const auto &pairEdges : this->edgeVerticesIndices) {
        double multVal = worldVector * this->verticesNormals[pairEdges.first];
        double multVal2 = worldVector * this->verticesNormals[pairEdges.second];
        if ((multVal > 0.0) && (multVal2 > 0.0)) {
            continue;
        }
        float element[4] = {this->mayaRawPoints[pairEdges.first * 3],
                            this->mayaRawPoints[pairEdges.first * 3 + 1],
                            this->mayaRawPoints[pairEdges.first * 3 + 2], 0};
        edgeVertices.append(element);

        float element2[4] = {this->mayaRawPoints[pairEdges.second * 3],
                             this->mayaRawPoints[pairEdges.second * 3 + 1],
                             this->mayaRawPoints[pairEdges.second * 3 + 2], 0};
        edgeVertices.append(element2);
        // use the normals to paint ...
    }
    drawManager.setColor(MColor(1., 0., 0., .5));
    // drawManager.lineList(edgeVertices, false);
    drawManager.setDepthPriority(5);
    drawManager.mesh(MHWRender::MUIDrawManager::kLines,
                     edgeVertices);  // edgeVertices.append (element); , &this->verticesNormals);
    /*
    MPointArray  thePoints(this->triangleVertices.length());
    for (unsigned int i = 0; i < this->triangleVertices.length(); ++i) {
            int ptIndex = this->triangleVertices[i];
            float element[4] = { this->mayaRawPoints[ptIndex * 3], this->mayaRawPoints[ptIndex * 3 +
    1], this->mayaRawPoints[ptIndex * 3 + 2], 0 }; thePoints.set(element, i++);
            // use the normals to paint ... nope
    }
    drawManager.mesh(MHWRender::MUIDrawManager::kTriangles, thePoints, NULL, NULL, NULL, NULL);
    */

    /*
    if (this->soloColorTypeVal == 0) {
            drawManager.mesh(MHWRender::MUIDrawManager::kTriangles, this->meshPoints, NULL,
    &this->soloCurrentColors, NULL, NULL);
    }
    else {
            drawManager.mesh(MHWRender::MUIDrawManager::kTriangles, this->meshPoints, NULL,
    &this->multiCurrentColors, NULL, NULL);
    }
    */
    return MS::kSuccess;
}
MStatus SkinBrushContext::doPtrMoved(MEvent &event, MHWRender::MUIDrawManager &drawManager,
                                     const MHWRender::MFrameContext &context) {
    // SkinBrushContext::doPtrMoved(event, drawMgr, context);

    event.getPosition(screenX, screenY);
    bool influencePosiFound = false;
    if (this->pickInfluenceVal) {
        // MGlobal::displayInfo("HERE pickInfluenceVal IS CALLED");
        biggestInfluence = getClosestInfluenceToCursor(screenX, screenY);
        influencePosiFound = biggestInfluence != -1;
        // this->pickInfluenceVal = false;
    }

    int faceHit;
    successFullHit = computeHit(screenX, screenY, true, faceHit, this->centerOfBrush);

    if (!successFullHit && !this->refreshDone) {  // try to re-get accelParams in case no hit
        refreshPointsNormals();
        successFullHit = computeHit(screenX, screenY, true, faceHit, this->centerOfBrush);
        this->refreshDone = true;
        // MGlobal::displayInfo("refreshing ");
    }

    if (!successFullHit && !influencePosiFound)
        return MStatus::kNotFound;
    else
        drawManager.beginDrawable();
    drawManager.setColor(MColor(0.0, 0.0, 1.0));
    drawManager.setLineWidth((float)lineWidthVal);

    if (this->pickMaxInfluenceVal || this->pickInfluenceVal) {
        // drawManager.setColor(MColor(1.0, 1.0, 0.0));
        // drawManager.rect2d(MPoint(this->screenX, this->screenY) , MVector(0.0, 1.0),  100, 50,
        // true);
        drawManager.setFontSize(14);
        drawManager.setFontName(MString("MS Shell Dlg 2"));
        drawManager.setFontWeight(1);
        if (this->pickMaxInfluenceVal)
            biggestInfluence = getHighestInfluence(faceHit, this->centerOfBrush);
        MString text("--");
        drawManager.setColor(MColor(0.0, 0.0, 0.0));
        MColor Yellow(1.0, 1.0, 0.0);
        int backgroundSize[] = {60, 20};
        if (biggestInfluence != -1) {
            text = this->inflNames[biggestInfluence];
            backgroundSize[0] = this->inflNamePixelSize[2 * biggestInfluence];
            backgroundSize[1] = this->inflNamePixelSize[2 * biggestInfluence + 1];
        }
        drawManager.text2d(MPoint(this->screenX, this->screenY), text,
                           MHWRender::MUIDrawManager::TextAlignment::kCenter, backgroundSize,
                           &Yellow);
    } else {
        drawManager.circle(this->centerOfBrush, normalVector, sizeVal);
        MPoint worldPt;
        MVector worldVector;
        view.viewToWorld(this->screenX, this->screenY, worldPt, worldVector);

        // drawTheMesh(drawManager, worldVector);
    }

    // drawManager.circle(surfacePoints[0], normalVector, sizeVal);
    drawManager.endDrawable();
    return MS::kSuccess;
}

MStatus SkinBrushContext::doPress(MEvent &event, MHWRender::MUIDrawManager &drawMgr,
                                  const MHWRender::MFrameContext &context) {
    selectionStatus = doPressCommon(event);
    CHECK_MSTATUS_AND_RETURN_SILENT(selectionStatus);

    doDrag(event, drawMgr, context);

    return MStatus::kSuccess;
}

MStatus SkinBrushContext::doDrag(MEvent &event, MHWRender::MUIDrawManager &drawManager,
                                 const MHWRender::MFrameContext &context) {
    CHECK_MSTATUS_AND_RETURN_SILENT(selectionStatus);

    // MGlobal::displayInfo("MUIDrawManager doDrag ");
    MStatus status = MStatus::kSuccess;

    status = doDragCommon(event);
    CHECK_MSTATUS_AND_RETURN_SILENT(status);

    // -----------------------------------------------------------------
    // display when painting or setting the brush size
    // -----------------------------------------------------------------
    if (drawBrushVal || event.mouseButton() == MEvent::kMiddleMouse) {
        drawManager.beginDrawable();

        drawManager.setColor(MColor((pow(colorVal.r, 0.454f)), (pow(colorVal.g, 0.454f)),
                                    (pow(colorVal.b, 0.454f))));
        drawManager.setLineWidth((float)lineWidthVal);
        // MGlobal::displayInfo("MUIDrawManager doDraw ");
        // Draw the circle in regular paint mode.
        // The range circle doens't get drawn here to avoid visual
        // clutter.
        if (event.mouseButton() == MEvent::kLeftMouse) {
            if (this->successFullDragHit) drawManager.circle(centerOfBrush, normalVector, sizeVal);
            // MPoint start(startScreenX, startScreenY);
            // drawManager.circle2d(start, this->sizeVal * this->pressDistance, true);
        }
        // Adjusting the brush settings with the middle mouse button.
        else if (event.mouseButton() == MEvent::kMiddleMouse) {
            // When adjusting the size the circle needs to remain with
            // a static position but the size needs to change.
            drawManager.setColor(MColor(1, 0, 1));

            if (sizeAdjust) {
                drawManager.circle(surfacePointAdjust, worldVectorAdjust, adjustValue);
                if (volumeVal && drawRangeVal)
                    drawManager.circle(surfacePointAdjust, worldVectorAdjust,
                                       adjustValue * rangeVal);
            }
            // When adjusting the strength the circle needs to remain
            // fixed and only the strength indicator changes.
            else {
                drawManager.circle(surfacePointAdjust, worldVectorAdjust, sizeVal);
                if (volumeVal && drawRangeVal)
                    drawManager.circle(surfacePointAdjust, worldVectorAdjust, sizeVal * rangeVal);

                MPoint start(startScreenX, startScreenY);
                MPoint end(startScreenX, startScreenY + adjustValue * 500);
                drawManager.line2d(start, end);

                drawManager.circle2d(end, lineWidthVal + 3.0, true);
            }
        }
        // drawTheMesh(drawManager);

        drawManager.endDrawable();
    }

    return status;
}

MStatus SkinBrushContext::doRelease(MEvent &event, MHWRender::MUIDrawManager &drawMgr,
                                    const MHWRender::MFrameContext &context) {
    CHECK_MSTATUS_AND_RETURN_SILENT(selectionStatus);

    doReleaseCommon(event);
    return MStatus::kSuccess;
}

MStatus SkinBrushContext::refreshPointsNormals() {
    MStatus status = MStatus::kSuccess;
    if (!skinObj.isNull()) {
        this->meshFn.freeCachedIntersectionAccelerator();  // yes ?
        this->mayaRawPoints = this->meshFn.getRawPoints(&status);

        // this->meshFn.getPoints(this->meshPoints);
        // this->meshFn.getTriangles(this->triangleCounts, this->triangleVertices);
        this->rawNormals = this->meshFn.getRawNormals(&status);
        for (unsigned int vertexInd = 0; vertexInd < this->numVertices; vertexInd++) {
            int indNormal = this->verticesNormalsIndices[vertexInd];
            MVector theNormal(this->rawNormals[indNormal * 3], this->rawNormals[indNormal * 3 + 1],
                              this->rawNormals[indNormal * 3 + 2]);
            this->verticesNormals.set(theNormal, vertexInd);
        }
    }
    return status;
}

// ---------------------------------------------------------------------
// common methods for legacy viewport and viewport 2.0
// ---------------------------------------------------------------------
MStatus SkinBrushContext::doPressCommon(MEvent event) {
    MStatus status = MStatus::kSuccess;

    unsigned int i;

    if (meshDag.node().isNull()) return MStatus::kNotFound;

    view = M3dView::active3dView();

    if (this->pickMaxInfluenceVal || this->pickInfluenceVal) {
        this->pickMaxInfluenceVal = false;
        this->pickInfluenceVal = false;

        if (biggestInfluence != this->influenceIndex && biggestInfluence != -1) {
            setInfluenceIndex(biggestInfluence, true);  // true for select in UI
        }
        return MStatus::kNotFound;
    }

    // store for undo purposes --------------
    this->fullUndoSkinWeightList = MDoubleArray(this->skinWeightList);
    // update values ---------------
    refreshPointsNormals();

    // first reset attribute to paint values off if we're doing that ------------------------
    paintArrayValues.copy(MDoubleArray(numVertices, 0.0));
    this->skinValuesToSet.clear();
    this->verticesPainted.clear();
    // reset values ---------------------------------

    // this->multiCurrentColors = MColorArray(this->numVertices, MColor(0.0, 0.0, 0.0, 1.0));
    // meshFn.setColors(this->multiCurrentColors, &this->fullColorSet);

    // meshFn.setDisplayColors(true);

    this->intensityValues = std::vector<float>(this->numVertices, 0);
    // initialize --
    undersamplingSteps = 0;
    performBrush = false;

    event.getPosition(this->screenX, this->screenY);

    // Get the size of the viewport and calculate the center for placing
    // the value messages when adjusting the brush settings.
    unsigned int x;
    unsigned int y;
    view.viewport(x, y, width, height);
    viewCenterX = (short)width / 2;
    viewCenterY = (short)height / 2;

    // Store the initial mouse position. These get used when adjusting
    // the brush size and strength values.
    startScreenX = this->screenX;
    startScreenY = this->screenY;

    // Reset the adjustment from the previous drag.
    initAdjust = false;
    sizeAdjust = true;
    adjustValue = 0.0;

    // -----------------------------------------------------------------
    // vertex selection
    // -----------------------------------------------------------------
    vtxSelection = getSelectionVertices();
    unsigned int numSelection = vtxSelection.length();

    // Create an array marking which indices are affected. This depends
    // on the selection as well as the Affect Selected setting.
    bool state = affectSelectedVal;
    if (numSelection) state = !affectSelectedVal;
    selectedIndices = std::vector<bool>(numVertices, state);
    for (i = 0; i < numSelection; i++) selectedIndices[(unsigned)vtxSelection[i]] = !state;

    // -----------------------------------------------------------------
    // closest point on surface
    // -----------------------------------------------------------------
    // Getting the closest index cannot be performed when in flood mode.
    if (eventIsValid(event)) {
        // Get the vertex index which is closest to the cursor position.
        // This method also defines the surface point and view vector.
        this->dicVertsDistSTART.clear();
        successFullHit =
            computeHit(screenX, screenY, false, this->previousfaceHit, this->centerOfBrush);
        if (!successFullHit) {
            return MStatus::kNotFound;
        }
        // store the current length position
        this->previousHitDistance = this->pressDistance;
        successFullHit =
            expandHit(this->previousfaceHit, this->centerOfBrush, this->dicVertsDistSTART);

        // Store the initial surface point and view vector to use when
        // the brush settings are adjusted because the brush circle
        // needs to be static during the adjustment.
        surfacePointAdjust = this->centerOfBrush;
        worldVectorAdjust = this->worldVector;
    }
    // precopy
    maya2019RefreshColors();
    // -----------------------------------------------------------------
    // selection & current weights
    // -----------------------------------------------------------------
    /*
// Store the current selection and hilite for undo.
MGlobal::getActiveSelectionList(prevSelection);
MGlobal::getHiliteList(prevHilite);

if (event.mouseButton() == MEvent::kLeftMouse)
{
    if (event.isModifierShift() || event.isModifierControl())
    {
        // If the selection should be reset, clear the current
        // selection but select the mesh to have it available for
        // the next usage.
        if (event.isModifierShift() && event.isModifierControl())
        {
            MGlobal::clearSelectionList();
            MSelectionList sel;
            sel.add(meshDag);
            MGlobal::setActiveSelectionList(sel);
        }

        // If only the shift or control modifier is pressed deselect
        // the mesh so that only components are effectively selected
        // but add the mesh to the hilite list.
        else
        {
            MSelectionList sel;
            sel.add(meshDag);
            MObject meshObj = meshDag.node();
            MGlobal::unselect(meshObj);
            meshObj = meshDag.transform();
            MGlobal::unselect(meshObj);

            MGlobal::setHiliteList(sel);
        }
    }
}
    */
    return status;
}
/*
MVector SkinBrushContext::getNormal(int vertexInd, int faceIndex) {
        int indFace = this->connectedFaces[vertexInd][faceIndex];
        int indNormal = this->normalsIds[indFace][0];
        return MVector(this->rawNormals[indNormal * 3], this->rawNormals[indNormal * 3 + 1],
this->rawNormals[indNormal * 3 + 2]);
}
*/

void SkinBrushContext::growArrayOfHits(std::unordered_map<int, float> &dicVertsDist) {
    // set of visited vertices
    std::vector<int> vertsVisited, vertsWithinDistance;

    for (const auto &element : dicVertsDist) vertsVisited.push_back(element.first);
    std::sort(vertsVisited.begin(), vertsVisited.end());
    vertsWithinDistance = vertsVisited;

    // start of growth---------------------
    bool processing = true;

    std::vector<int> borderOfGrowth;
    borderOfGrowth = vertsVisited;
    while (processing) {
        processing = false;
        // -------------------- grow the vertices ----------------------------------------------
        std::vector<int> setOfVertsGrow;
        for (int vertexIndex : borderOfGrowth) {
            setOfVertsGrow = setOfVertsGrow + this->connectedSetVertices[vertexIndex];
        }
        // get the vertices that are grown -----------------------------------------------------
        std::vector<int> verticesontheborder = setOfVertsGrow - vertsVisited;
        std::vector<int> foundGrowVertsWithinDistance;
        // for all vertices grown ------------------------------
        for (int vertexBorder : verticesontheborder) {
            // intersection of the connectedVerts of the grow vert and the already visisted Vertices
            std::vector<int> intersection =
                this->connectedSetVertices[vertexBorder] & vertsWithinDistance;

            float closestDist = 0.0;
            int closestVertex = -1;
            // find the closestDistance and closest Vertex from visited vertices
            // ------------------------
            for (int vertVisited : intersection) {
                float dist = dicVertsDist[vertVisited];
                if (closestVertex == -1 || dist < closestDist) {
                    closestVertex = vertVisited;
                    closestDist = dist;
                }
            }
            if (closestVertex == -1) continue;  // shouldnt be necessary

            // get the new distance between the closest visited vertex and the grow vertex
            MFloatPoint posVertex(this->mayaRawPoints[vertexBorder * 3],
                                  this->mayaRawPoints[vertexBorder * 3 + 1],
                                  this->mayaRawPoints[vertexBorder * 3 + 2]);
            MFloatPoint posClosestVertex(this->mayaRawPoints[closestVertex * 3],
                                         this->mayaRawPoints[closestVertex * 3 + 1],
                                         this->mayaRawPoints[closestVertex * 3 + 2]);
            closestDist += posVertex.distanceTo(posClosestVertex);  // sum this distance
            if (closestDist <= this->sizeVal) {                     // if in radius of the brush
                processing = true;  // we found a vertex in the radius

                // now add to the visited and add the distance to the dictionnary
                foundGrowVertsWithinDistance.push_back(vertexBorder);

                auto ret = dicVertsDist.insert(std::make_pair(vertexBorder, closestDist));
                if (!ret.second) ret.first->second = std::min(closestDist, ret.first->second);
            }
        }
        // this vertices has been visited, let's not consider them anymore
        std::sort(foundGrowVertsWithinDistance.begin(), foundGrowVertsWithinDistance.end());
        vertsVisited = vertsVisited + verticesontheborder;
        vertsWithinDistance = vertsWithinDistance + foundGrowVertsWithinDistance;

        borderOfGrowth = foundGrowVertsWithinDistance;
    }
}

void SkinBrushContext::growArrayOfHitsFromCenters(std::unordered_map<int, float> &dicVertsDist,
                                                  MFloatPointArray &AllHitPoints) {
    // set of visited vertices
    std::vector<int> vertsVisited, vertsWithinDistance;

    for (const auto &element : dicVertsDist) vertsVisited.push_back(element.first);
    std::sort(vertsVisited.begin(), vertsVisited.end());
    vertsWithinDistance = vertsVisited;

    // start of growth---------------------
    bool processing = true;
    double smallestMult = 5, biggestMult = -5;

    std::vector<int> borderOfGrowth;
    borderOfGrowth = vertsVisited;
    // MItMeshVertex vertexIter(meshDag);

    while (processing) {
        processing = false;
        // -------------------- grow the vertices ----------------------------------------------
        std::vector<int> setOfVertsGrow;
        for (int vertexIndex : borderOfGrowth) {
            setOfVertsGrow = setOfVertsGrow + this->connectedSetVertices[vertexIndex];
        }
        // get the vertices that are grown -----------------------------------------------------
        std::vector<int> verticesontheborder = setOfVertsGrow - vertsVisited;
        std::vector<int> foundGrowVertsWithinDistance;

        // for all vertices grown ------------------------------
        for (int vertexBorder : verticesontheborder) {
            // First check the normal
            if (!this->coverageVal) {
                /*
                MVector vertexBorderNormal;// = getNormal(vertexBorder, 0);
                int prevIndex;
                vertexIter.setIndex(vertexBorder, prevIndex);
                vertexIter.getNormal(vertexBorderNormal);
                */

                MVector vertexBorderNormal = this->verticesNormals[vertexBorder];
                double multVal = worldVector * vertexBorderNormal;
                /*
                biggestMult = std::max(biggestMult, multVal);
                smallestMult = std::min(smallestMult, multVal);
                */
                if (multVal > 0.0) continue;
            }

            float closestDist = -1;

            // find the closestDistance and closest Vertex from visited vertices
            // ------------------------
            MFloatPoint posVertex(this->mayaRawPoints[vertexBorder * 3],
                                  this->mayaRawPoints[vertexBorder * 3 + 1],
                                  this->mayaRawPoints[vertexBorder * 3 + 2]);
            for (unsigned int k = 0; k < AllHitPoints.length(); ++k) {
                float dist = posVertex.distanceTo(AllHitPoints[k]);
                if (closestDist == -1 || dist < closestDist) {
                    closestDist = dist;
                } else if (dist > 2. * closestDist) {
                    break;  // we passed it
                }
            }
            if (closestDist == -1) continue;  // shouldnt be necessary

            // get the new distance between the closest visited vertex and the grow vertex
            if (closestDist <= this->sizeVal) {  // if in radius of the brush
                processing = true;               // we found a vertex in the radius

                // now add to the visited and add the distance to the dictionnary
                foundGrowVertsWithinDistance.push_back(vertexBorder);
                auto ret = dicVertsDist.insert(std::make_pair(vertexBorder, closestDist));
                if (!ret.second) ret.first->second = std::min(closestDist, ret.first->second);
            }
        }
        // this vertices has been visited, let's not consider them anymore
        std::sort(foundGrowVertsWithinDistance.begin(), foundGrowVertsWithinDistance.end());
        vertsVisited = vertsVisited + verticesontheborder;
        vertsWithinDistance = vertsWithinDistance + foundGrowVertsWithinDistance;

        borderOfGrowth = foundGrowVertsWithinDistance;
    }
    /*
    MGlobal::displayInfo(MString(" ---------> ") + smallestMult + MString(" < mult < ") +
    biggestMult);
    */
}

MStatus SkinBrushContext::doDragCommon(MEvent event) {
    MStatus status = MStatus::kSuccess;

    // -----------------------------------------------------------------
    // Dragging with the left mouse button performs the smoothing.
    // -----------------------------------------------------------------
    if (event.mouseButton() == MEvent::kLeftMouse) {
        // from previous hit get a line----------
        short previousX = this->screenX;
        short previousY = this->screenY;
        event.getPosition(this->screenX, this->screenY);

        // dictionnary of visited vertices and distances --- prefill it with the previous hit ---
        std::unordered_map<int, float> dicVertsDistSummedUp;  // this->doAddingDots
        std::unordered_map<int, float> dicVertsDistToGrow = this->dicVertsDistSTART;
        std::unordered_map<int, float> dicVertsRed = this->dicVertsDistSTART;

        // store the index of the MFloatPoint Hit in the hit array for all vertices ---
        std::vector<int> vertexHitIndex(this->numVertices, -1);
        for (const auto &element : dicVertsDistToGrow) vertexHitIndex[element.first] = 0;
        // for linear growth --------
        MFloatPointArray AllHitPoints;
        AllHitPoints.append(this->centerOfBrush);

        if (this->doAddingDots) {
            growArrayOfHits(this->dicVertsDistSTART);
            for (const auto &element : this->dicVertsDistSTART) {
                float value = (float)1.0 - (element.second / this->sizeVal);
                auto ret = dicVertsDistSummedUp.insert(std::make_pair(element.first, value));
                if (!ret.second) ret.first->second += value;
            }
        }

        // --------- LINE OF PIXELS --------------------
        std::vector<std::pair<short, short>> line2dOfPixels;
        // get pixels of the line of pixels
        lineC(previousX, previousY, this->screenX, this->screenY, line2dOfPixels);
        int nbPixelsOfLine = (int)line2dOfPixels.size();

        MFloatPoint hitPoint;
        int faceHit;
        bool successFullHit2 = computeHit(this->screenX, this->screenY, true, faceHit, hitPoint);
        if (successFullHit2) {
            // stored in start dic for next call of drag function
            this->previousfaceHit = faceHit;
            this->dicVertsDistSTART.clear();
            expandHit(faceHit, hitPoint, this->dicVertsDistSTART);  // for next beginning
        }
        if (!this->successFullDragHit && !successFullHit2)  // moving in empty zone
            return MStatus::kNotFound;
        this->successFullDragHit = successFullHit2;

        // else
        //	MGlobal::displayInfo("FAILED HIT");

        int incrementValue = 1;
        if (this->doAddingDots) {  // we compute the increment for how many hits

            double worldDistanceBetweenHits;
            if (!successFullHit) {
                // now get the world position where it would hit if same distance as previous hit !!
                MPoint worldPt;
                MVector worldVector;
                view.viewToWorld(this->screenX, this->screenY, worldPt, worldVector);
                MPoint hitPosiForSameDist = worldPt + this->previousHitDistance * worldVector;
                worldDistanceBetweenHits = this->centerOfBrush.distanceTo(hitPosiForSameDist);

                this->centerOfBrush = hitPosiForSameDist;
            } else {
                worldDistanceBetweenHits = this->centerOfBrush.distanceTo(hitPoint);
                this->previousHitDistance = this->pressDistance;
                this->centerOfBrush = hitPoint;
            }

            // number of brush stamps to make ---------------
            int nbStamps = std::max(1, int(worldDistanceBetweenHits / this->sizeVal) *
                                           6);  //  *this->stepsToDrawLineVal;
            // MGlobal::displayInfo(MString("nbStamps ") + nbStamps + MString(" stepsLine ") +
            // this->stepsToDrawLineVal);
            incrementValue = std::max(1, nbPixelsOfLine / nbStamps);
        } else {
            if (this->successFullDragHit) {
                this->previousHitDistance = this->pressDistance;
                this->centerOfBrush = hitPoint;
            }
        }
        if (incrementValue < nbPixelsOfLine) {
            for (int i = incrementValue; i < nbPixelsOfLine; i += incrementValue) {
                auto myPair = line2dOfPixels[i];
                short x = myPair.first;
                short y = myPair.second;

                bool successFullHit2 = computeHit(x, y, false, faceHit, hitPoint);
                if (successFullHit2) {
                    AllHitPoints.append(hitPoint);

                    if (this->doAddingDots) {
                        std::unordered_map<int, float> dicVertsDist;
                        successFullHit2 = expandHit(faceHit, hitPoint, dicVertsDist);
                        growArrayOfHits(dicVertsDist);

                        for (const auto &element : dicVertsDist) {
                            float value = 1.0 - (element.second / this->sizeVal);
                            auto ret =
                                dicVertsDistSummedUp.insert(std::make_pair(element.first, value));
                            if (!ret.second) ret.first->second += value;
                        }
                    } else {
                        successFullHit2 = expandHit(faceHit, hitPoint, dicVertsDistToGrow);
                    }
                }
                // else
                //	MGlobal::displayInfo("-------- FAILED HIT 2");
            }
        }
        // only now add last hit
        if (this->successFullDragHit) {
            AllHitPoints.append(this->centerOfBrush);
            expandHit(faceHit, this->centerOfBrush, dicVertsDistToGrow);  // to get closest hit
            expandHit(faceHit, this->centerOfBrush, dicVertsRed);         // to get closest hit
        }

        // let's expand these arrays ----------------
        if (!this->doAddingDots) {
            growArrayOfHitsFromCenters(dicVertsDistToGrow, AllHitPoints);
            // growArrayOfHits(dicVertsDistToGrow);
            // growArrayOfHitsFromCenters(dicVertsDistToGrow, AllHitPoints, vertexHitIndex);
            dicVertsDistSummedUp = dicVertsDistToGrow;
        }

        if (event.isModifierNone())
            this->modifierNoneShiftControl = 0;
        else if (event.isModifierShift())
            this->modifierNoneShiftControl = 1;
        else if (event.isModifierControl())
            this->modifierNoneShiftControl = 2;

        prepareArray(dicVertsDistSummedUp);
        performPaint(dicVertsDistSummedUp, dicVertsRed);
        performBrush = true;

        /*
        if (event.isModifierNone())
        {
                prepareArray(dicVertsDistSummedUp);
                performPaint(dicVertsDistSummedUp , dicVertsRed);
                performBrush = true;
        }
        else
        {
                MIntArray closestIndices;
                MFloatArray closestDistances;
                for (const auto &element : dicVertsDistSummedUp){
                        closestIndices.append(element.first);
                        closestDistances.append(element.second);
                }
                performSelect(event, closestIndices, closestDistances);
                performBrush = true;
        }
        */

    }
    // -----------------------------------------------------------------
    // Dragging with the middle mouse button adjusts the settings.
    // -----------------------------------------------------------------
    else if (event.mouseButton() == MEvent::kMiddleMouse) {
        // Skip several evaluation steps. This has several reasons:
        // - It reduces the smoothing strength because not every evaluation
        //   triggers a calculation.
        // - It lets adjusting the brush appear smoother because the lines
        //   show less flicker.
        // - It also improves the differentiation between horizontal and
        //   vertical dragging when adjusting.
        undersamplingSteps++;
        if (undersamplingSteps < undersamplingVal) return status;
        undersamplingSteps = 0;

        // get screen position
        event.getPosition(this->screenX, this->screenY);
        // Get the current and initial cursor position and calculate the
        // delta movement from them.
        MPoint currentPos(this->screenX, this->screenY);
        MPoint startPos(startScreenX, startScreenY);
        MPoint deltaPos(currentPos - startPos);

        // Switch if the size should get adjusted or the strength based
        // on the drag direction. A drag along the x axis defines size
        // and a drag along the y axis defines strength.
        // InitAdjust makes sure that direction gets set on the first
        // drag event and gets reset the next time a mouse button is
        // pressed.
        if (!initAdjust) {
            if (abs(deltaPos.x) > abs(deltaPos.y)) {
                initAdjust = true;
            } else if (abs(deltaPos.x) < abs(deltaPos.y)) {
                sizeAdjust = false;
                initAdjust = true;
            }
        }

        // Define the settings for either setting the brush size or the
        // brush strength.
        MString message = "Brush Size";
        MString slider = "Size";
        double dragDistance = deltaPos.x;
        double min = 0.0;
        unsigned int max = 1000;
        double baseValue = sizeVal;
        // The adjustment speed depends on the distance to the mesh.
        // Closer distances allows for a feiner control whereas larger
        // distances need a coarser control.
        double speed = pow(0.001 * pressDistance, 0.9);

        // Vary the settings if the strength gets adjusted.
        if (!sizeAdjust) {
            if (event.isModifierControl()) {
                message = "Smooth Strength";
                baseValue = smoothStrengthVal;
            } else {
                message = "Brush Strength";
                baseValue = strengthVal;
            }
            slider = "Strength";
            dragDistance = deltaPos.y;
            max = 1;

            speed *= 0.1;
        }

        // The control modifier scales the speed for a fine adjustment.
        // if (event.isModifierControl())
        if (event.isModifierShift()) speed *= 0.1;

        // Calculate the new value by adding the drag distance to the
        // start value.
        double value = baseValue + dragDistance * speed;

        // Clamp the values to the min/max range.
        if (value < min)
            value = min;
        else if (value > max)
            value = max;

        // Store the modified value for drawing and for setting the
        // values when releasing the mouse button.
        adjustValue = value;

        // -------------------------------------------------------------
        // value display in the viewport
        // -------------------------------------------------------------
        char info[32];
#ifdef _WIN64
        sprintf_s(info, "%s: %.2f", message.asChar(), adjustValue);
#else
        sprintf(info, "%s: %.2f", message.asChar(), adjustValue);
#endif

        // Calculate the position for the value display. Since the
        // heads-up message starts at the center of the viewport an
        // offset needs to get calculated based on the view size and the
        // initial adjust position of the cursor.
        short offsetX = startScreenX - viewCenterX;
        short offsetY = startScreenY - viewCenterY - 50;

        MString cmd = "headsUpMessage -horizontalOffset ";
        cmd += offsetX;
        cmd += " -verticalOffset ";
        cmd += offsetY;
        cmd += " -time 0.1 ";
        cmd += "\"" + MString(info) + "\"";
        MGlobal::executeCommand(cmd);

        // Also, adjust the slider in the tool settings window if it's
        // currently open.
        MString tool("brSkinBrush");
        MGlobal::executeCommand("if (`columnLayout -exists " + tool + "`) " +
                                "floatSliderGrp -edit -value " + (MString() + adjustValue) + " " +
                                tool + slider + ";");
    }
    return status;
}

void SkinBrushContext::doReleaseCommon(MEvent event) {
    // Don't continue if no mesh has been set.
    if (meshFn.object().isNull()) return;
    this->refreshDone = false;
    // Define, which brush setting has been adjusted and needs to get
    // stored.
    if (event.mouseButton() == MEvent::kMiddleMouse) {
        if (sizeAdjust) {
            sizeVal = adjustValue;
        } else {
            if (event.isModifierControl()) {
                smoothStrengthVal = adjustValue;
            } else {
                strengthVal = adjustValue;
            }
        }
    }

    // If the smoothing has been performed send the current values to
    // the tool command along with the necessary data for undo and redo.
    // The same goes for the select mode.
    if (performBrush) {
        // redraw clors properly I guess- but I think we would need a set first
        // need to look into that eventually
        MColorArray multiEditColors, soloEditColors;
        MIntArray editVertsIndices((int)this->verticesPainted.size(), 0);
        MIntArray undoLocks, redoLocks;

        MDoubleArray prevWeights((int)this->verticesPainted.size() * this->nbJoints, 0);
        int i = 0;
        for (const auto &theVert : this->verticesPainted) {
            editVertsIndices[i] = theVert;
            i++;
        }
        int theCommandIndex = this->commandIndex;
        if (this->commandIndex == 6 && this->modifierNoneShiftControl == 1)
            theCommandIndex = 7;  // unlockVertices

        if (theCommandIndex >= 6) {
            undoLocks.copy(this->lockVertices);
            bool addLocks = theCommandIndex == 6;
            // if (this->mirrorIsActive) editLocks(this->skinObj, editAndMirrorVerts, addLocks,
            // this->lockVertices);
            editLocks(this->skinObj, editVertsIndices, addLocks, this->lockVertices);
            // MGlobal::displayInfo("editing locks");
            redoLocks.copy(this->lockVertices);
        } else {
            if (this->skinValuesToSet.size() > 0)
                applyCommand(this->influenceIndex, this->skinValuesToSet, !this->mirrorIsActive);

            int i = 0;
            for (const auto &theVert : this->verticesPainted) {
                for (int j = 0; j < this->nbJoints; ++j)
                    prevWeights[i * this->nbJoints + j] =
                        this->fullUndoSkinWeightList[theVert * this->nbJoints + j];
                i++;
            }
        }
        refreshColors(editVertsIndices, multiEditColors, soloEditColors);
        this->skinValuesToSet.clear();
        meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet);
        meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet);

        meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet2);
        meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet2);

        /*
        // For Maya 2019 ---------------------------------
        #if MAYA_API_VERSION >= 201900
        if (soloColorVal == 1) {
                meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->noColorSet);
        }
        else {
                meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->noColorSet);
        }
        #endif
        // end For Maya 2019 ---------------------------------
        */

        // refresh view and display
        view.refresh(false, true);

        this->previousPaint.clear();

        cmd = (skinBrushTool *)newToolCommand();

        cmd->setAffectSelected(affectSelectedVal);
        cmd->setColor(colorVal);
        cmd->setCurve(curveVal);
        cmd->setDepth(depthVal);
        cmd->setDepthStart(depthStartVal);
        cmd->setDrawBrush(drawBrushVal);
        cmd->setDrawRange(drawRangeVal);
        cmd->setEnterToolCommand(enterToolCommandVal);
        cmd->setExitToolCommand(exitToolCommandVal);
        cmd->setFractionOversampling(fractionOversamplingVal);
        cmd->setIgnoreLock(ignoreLockVal);
        cmd->setKeepShellsTogether(keepShellsTogetherVal);
        cmd->setLineWidth(lineWidthVal);
        cmd->setMessage(messageVal);
        cmd->setOversampling(oversamplingVal);
        cmd->setRange(rangeVal);
        cmd->setSize(sizeVal);
        cmd->setStrength(strengthVal);

        cmd->setSmoothStrength(smoothStrengthVal);
        cmd->setTolerance(toleranceVal);
        cmd->setUndersampling(undersamplingVal);
        cmd->setVolume(volumeVal);
        cmd->setStepLine(stepsToDrawLineVal);
        cmd->setCoverage(coverageVal);

        cmd->setCommandIndex(theCommandIndex);

        cmd->setUnoLocks(undoLocks);
        cmd->setRedoLocks(redoLocks);

        cmd->setMesh(meshDag);
        cmd->setSkinCluster(skinObj);
        cmd->setInfluenceIndices(influenceIndices);
        cmd->setVertexComponents(smoothedCompObj);

        cmd->setUnoVertices(editVertsIndices);
        cmd->setWeights(prevWeights);

        cmd->setNormalize(normalize);
        cmd->setSelection(prevSelection, prevHilite);
        // cmd->redoIt();
        // Regular context implementations usually call
        // (MPxToolCommand)::redoIt at this point but in this case it
        // is not necessary since the the smoothing already has been
        // performed. There is no need to apply the values twice.
        cmd->finalize();
    } else {
        // Refresh the view to erase the drawn circle. This might not
        // always be necessary but is just included to complete the process.
        view.refresh(false, true);
    }
}

// ---------------------------------------------------------------------
// Commands
// ---------------------------------------------------------------------
MStatus SkinBrushContext::applyCommandMirror(std::unordered_map<int, double> &valuesToSet) {
    MStatus status;
    if (verbose) MGlobal::displayInfo(MString(" applyCommandMirror ") + (int)valuesToSet.size());
    int mirrorInfluenceIndex = this->mirrorInfluences[this->influenceIndex];
    return applyCommand(mirrorInfluenceIndex, valuesToSet, false);
}

MStatus SkinBrushContext::applyCommand(int influence, std::unordered_map<int, double> &valuesToSet,
                                       bool storeUndo) {
    // we need to sort all of that one way or another ---------------- here it is ------
    std::map<int, double> valuesToSetOrdered(valuesToSet.begin(), valuesToSet.end());

    // modifierNoneShiftControl
    int theCommandIndex = this->commandIndex;

    if (this->commandIndex == 0 && this->modifierNoneShiftControl == 1)
        theCommandIndex = 1;  // remove
    if (this->commandIndex == 6 && this->modifierNoneShiftControl == 1)
        theCommandIndex = 7;  // unlockVertices
    if (this->modifierNoneShiftControl == 2 && theCommandIndex < 6)
        theCommandIndex = 4;  // smooth always

    double multiplier = 1.0;
    // if (!this->postSetting && theCommandIndex != 4) multiplier = .1; // less applying if dragging
    // paint 0 Add - 1 Remove - 2 AddPercent - 3 Absolute - 4 Smooth - 5 Sharpen - 6 LockVertices - 7
    // unlockVertices
    MStatus status;
    if (verbose) MGlobal::displayInfo(MString(" applyCommand Index is ") + theCommandIndex);
    if (theCommandIndex < 6) {  // not lock or unlock verts
        // MDoubleArray previousWeights(this->nbJoints*valuesToSetOrdered.size(), 0.0);

        // std::vector< double > previousWeights;
        // std::vector< int > undoVerts;
        // undoVerts.resize(theEditVerts.length());
        // previousWeights.resize(this->nbJoints*theEditVerts.length());

        MDoubleArray theWeights((int)this->nbJoints * valuesToSetOrdered.size(), 0.0);
        int repeatLimit = 1;
        if (theCommandIndex == 4 || theCommandIndex == 5) repeatLimit = this->smoothRepeat;
        for (int repeat = 0; repeat < repeatLimit; ++repeat) {
            if (theCommandIndex == 4) {  // smooth
                int i = 0;
                for (const auto &elem : valuesToSetOrdered) {
                    int theVert = elem.first;
                    double theWeight = elem.second;
                    std::vector<int> vertsAround = this->connectedSetVertices[theVert];

                    // this->smoothDepth to expand
                    status = setAverageWeight(vertsAround, theVert, i, this->nbJoints,
                                              this->lockJoints, this->skinWeightList, theWeights,
                                              this->smoothStrengthVal * theWeight);
                    i++;
                }
            } else {
                if (this->ignoreLockVal) {
                    status =
                        editArray(theCommandIndex, influence, this->nbJoints,
                                  this->ignoreLockJoints, this->skinWeightList, valuesToSetOrdered,
                                  theWeights, this->doNormalize, multiplier);
                } else {
                    if (this->lockJoints[influence] == 1 && theCommandIndex != 5)
                        return status;  //  if locked and it's not sharpen --> do nothing
                    status = editArray(theCommandIndex, influence, this->nbJoints, this->lockJoints,
                                       this->skinWeightList, valuesToSetOrdered, theWeights,
                                       this->doNormalize, multiplier);
                }
            }
            // now set the weights -----------------------------------------------------
            // doPruneWeight(theWeights, this->nbJoints, this->pruneWeight);

            // here we should normalize -----------------------------------------------------

            MIntArray objVertices;
            int i = 0;
            // int prevVert = -1;
            for (const auto &elem : valuesToSetOrdered) {
                int theVert = elem.first;
                // if (prevVert > theVert) MGlobal::displayInfo(MString("indices don't grow ") +
                // prevVert + MString(" ") + theVert); prevVert = theVert;
                objVertices.append(theVert);
                for (int j = 0; j < this->nbJoints; ++j) {
                    // if (repeat == 0 && storeUndo) previousWeights[i*this->nbJoints + j] =
                    // this->skinWeightList[theVert*this->nbJoints + j];
                    // this->skinWeightList[theVert*this->nbJoints + j] = verticesWeight[i] *
                    // theWeights[i*this->nbJoints + j] + (1.0 - verticesWeight[i]) *
                    // this->skinWeightList[theVert*this->nbJoints + j];
                    this->skinWeightList[theVert * this->nbJoints + j] =
                        theWeights[i * this->nbJoints + j];
                }
                i++;
            }
            MFnSingleIndexedComponent compFn;
            MObject weightsObj = compFn.create(MFn::kMeshVertComponent);
            compFn.addElements(objVertices);

            // Set the new weights.
            // Initialize the skin cluster.
            MFnSkinCluster skinFn(skinObj, &status);
            CHECK_MSTATUS_AND_RETURN_IT(status);
            skinFn.setWeights(meshDag, weightsObj, influenceIndices, theWeights, normalize);

            // in do press common
            // update values ---------------
            refreshPointsNormals();
        }
        /*
        if (storeUndo) {
                MIntArray undoVerts;
                undoVerts.copy(theEditVerts);
                // now store the undo ----------------
                //for (int i = 0; i < theEditVerts.length(); ++i) undoVerts[i] = theEditVerts[i];
                this->undoVertsIndices_.push_back(undoVerts);
                this->undoVertsValues_.push_back(previousWeights);
        }
        replace_weights(theEditVerts, theWeights);
        */
    }
    return status;
}

// ---------------------------------------------------------------------
// COLORS
// ---------------------------------------------------------------------

MStatus SkinBrushContext::editSoloColorSet(bool doBlack) {
    MStatus status;
    if (verbose) MGlobal::displayInfo(" editSoloColorSet CALL NEW 3 ");
    // meshFn.setCurrentColorSetName(this->noColorSet);

    MColorArray colToSet;
    MIntArray vtxToSet;
    for (unsigned int theVert = 0; theVert < this->numVertices; ++theVert) {
        double val = this->skinWeightList[theVert * this->nbJoints + this->influenceIndex];
        // if (theVert == 20266) MGlobal::displayInfo(MString("
        // Mesh_X_HeadBody_Pc_Sd1_SdDsp_.vtx[20266] -  ") + val + MString(" - storeValue ") +
        // this->soloColorsValues[theVert]);
        bool isVtxLocked = this->lockVertices[theVert] == 1;
        bool update = doBlack | !(this->soloColorsValues[theVert] == 0 && val == 0);
        if (update) {  // dont update the black
            MColor soloColor = getASoloColor(val);
            this->soloCurrentColors[theVert] = soloColor;
            this->soloColorsValues[theVert] = val;
            if (isVtxLocked)
                colToSet.append(this->lockVertColor);
            else
                colToSet.append(soloColor);
            vtxToSet.append(theVert);
        }
    }
    meshFn.setSomeColors(vtxToSet, colToSet, &this->soloColorSet);
    meshFn.setSomeColors(vtxToSet, colToSet, &this->soloColorSet2);

    // meshFn.setCurrentColorSetName(this->soloColorSet);

    // don't know why but we need that ------------
    meshFn.setDisplayColors(true);

    view = M3dView::active3dView();
    view.refresh(false, true);
    // view.refresh(false, true);
    return status;
}

MStatus SkinBrushContext::refreshColors(MIntArray &editVertsIndices, MColorArray &multiEditColors,
                                        MColorArray &soloEditColors) {
    MStatus status = MS::kSuccess;
    if (verbose)
        MGlobal::displayInfo(MString(" refreshColors CALL ") +
                             editVertsIndices.length());  // beginning opening of node
    if (multiEditColors.length() != editVertsIndices.length())
        multiEditColors.setLength(editVertsIndices.length());
    if (soloEditColors.length() != editVertsIndices.length())
        soloEditColors.setLength(editVertsIndices.length());

    for (unsigned int i = 0; i < editVertsIndices.length(); ++i) {
        int theVert = editVertsIndices[i];
        MColor multiColor, soloColor;
        bool isVtxLocked = this->lockVertices[theVert] == 1;
        for (int j = 0; j < this->nbJoints; ++j) {  // for each joint
            double val = this->skinWeightList[theVert * this->nbJoints + j];
            multiColor += jointsColors[j] * val;
            if (j == this->influenceIndex) {
                this->soloColorsValues[theVert] = val;
                if ((theVert == 22038) && verbose)
                    MGlobal::displayInfo(MString(" vert  22038 ") + val);
                soloColor = getASoloColor(val);
            }
        }
        if (!isVtxLocked) {
            multiEditColors[i] = multiColor;
            soloEditColors[i] = soloColor;
        } else {
            multiEditColors[i] = this->lockVertColor;
            soloEditColors[i] = this->lockVertColor;
        }
        this->multiCurrentColors[theVert] = multiColor;
        this->soloCurrentColors[theVert] = soloColor;
    }
    return status;
}

MColor SkinBrushContext::getASoloColor(double val) {
    // if (verbose) MGlobal::displayInfo(" getASoloColor CALL \n");

    if (val == 0) return MColor(0, 0, 0);
    val = (this->maxSoloColor - this->minSoloColor) * val + this->minSoloColor;
    MColor soloColor;
    if (this->soloColorTypeVal == 0) {  // black and white
        soloColor = MColor(val, val, val);
    } else if (this->soloColorTypeVal == 1) {  // lava
        val *= 2;
        if (val > 1)
            soloColor = MColor(val, (val - 1), 0);
        else
            soloColor = MColor(val, 0, 0);
    } else {  // influence
        soloColor = val * this->jointsColors[this->influenceIndex];
    }
    return soloColor;
}

// ---------------------------------------------------------------------
// brush methods
// ---------------------------------------------------------------------
MStatus SkinBrushContext::getMesh() {
    MStatus status = MStatus::kSuccess;

    // Clear the previous data.
    meshDag = MDagPath();
    skinObj = MObject();

    // Clear the weights arrays. Especially the prevWeights array since
    // this stores the weights for undo. Since the prevWeights are only
    // collected when smoothing and not in select mode this would cause
    // wrong undo weights when switching meshes. When the tool gets
    // reactivated after a tool change the undo method would still refer
    // to the undo weights from the previous mesh. Therefore it's
    // necessary to remove all previous weights when the tool changes.
    // currentWeights.clear();

    // -----------------------------------------------------------------
    // mesh
    // -----------------------------------------------------------------

    // Get the selected mesh and any selected vertex indices.
    // If nothing is selected the mesh at the cursor position will be
    // selected.
    MDagPath dagPath;
    status = getSelection(meshDag);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Set the mesh.
    meshFn.setObject(meshDag);
    numVertices = (unsigned)meshFn.numVertices();
    numFaces = (unsigned)meshFn.numPolygons();
    meshFn.freeCachedIntersectionAccelerator();
    this->accelParams =
        meshFn.uniformGridParams(33, 33, 33);  // I dont know why, but '33' seems to work well

    // Create the intersector for the closest point operation for
    // keeping the shells together.
    MObject meshObj = meshDag.node();
    status = intersector.create(meshObj, meshDag.inclusiveMatrix());
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // getConnected vertices Guillaume function
    getConnectedVertices();
    // meshFn.getPoints(this->vertexArray);
    this->mayaRawPoints = meshFn.getRawPoints(&status);
    this->lockVertices = MIntArray(this->numVertices, 0);

    // -----------------------------------------------------------------
    // skin cluster
    // -----------------------------------------------------------------
    // Get the skin cluster node from the history of the mesh.
    MObject skinClusterObj;
    status = getSkinCluster(meshDag, skinClusterObj);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    // -----------------------------------------------------------------
    // get the paintable attribute
    // -----------------------------------------------------------------
    if (!foundBlurSkinAttribute) {
        // -------------- paintAttribute on mesh -------------
        MFnDependencyNode deformedNameMesh(meshObj);
        MPlug paintAttrPlug = deformedNameMesh.findPlug("paintAttr");
        MDoubleArray paintArrayValues(numVertices, 0);
        status = paintAttrPlug.getValue(attrValue);
        MFnDoubleArrayData doubleArrayDataFn(attrValue, &status);
        valuesForAttribute = doubleArrayDataFn.array(&status);
        valuesForAttribute.copy(paintArrayValues);
    }

    // Store the skin cluster for undo.
    skinObj = skinClusterObj;
    // for the influences ----------------
    // Create a component object representing all vertices of the mesh.
    allVtxCompObj = allVertexComponents(meshDag);

    // Create a component object representing only the modified
    // vertices. This is needed to improve performance for undo/redo.
    // See skinBrushTool::undoIt() for more information.
    MFnSingleIndexedComponent compFn;
    smoothedCompObj = compFn.create(MFn::kMeshVertComponent);
    // Get the indices of all influences.
    influenceIndices = getInfluenceIndices(this->skinObj, this->inflDagPaths);

    // Get the skin cluster settings.
    unsigned int normalizeValue;
    getSkinClusterAttributes(skinClusterObj, maxInfluences, maintainMaxInfluences, normalizeValue);
    normalize = false;
    if (normalizeValue > 0) normalize = true;

    return status;
}
/*
store vertices connections
*/
void SkinBrushContext::getConnectedVertices() {
    MItMeshVertex vertexIter(meshDag);
    this->connectedVertices.resize(numVertices);
    this->connectedFaces.resize(numVertices);
    // The normals are in local space and are the per-polygon per-vertex normals
    this->verticesNormals.setLength(numVertices);

    for (int vtxTmp = 0; !vertexIter.isDone(); vertexIter.next(), ++vtxTmp) {
        MIntArray surroundingVertices, surroundingFaces;
        vertexIter.getConnectedVertices(surroundingVertices);
        this->connectedVertices[vtxTmp] = surroundingVertices;

        vertexIter.getConnectedFaces(surroundingFaces);
        this->connectedFaces[vtxTmp] = surroundingFaces;

        MVector theNormal;
        vertexIter.getNormal(theNormal);
        this->verticesNormals.set(theNormal, vtxTmp);
    }

    // get connections from polygons -------------------------
    MItMeshPolygon polyIter(meshDag);
    this->FaceToVerticesSet.resize(this->numFaces);
    this->FaceToVertices.resize(this->numFaces);

    for (int faceTmp = 0; !polyIter.isDone(); polyIter.next(), ++faceTmp) {
        MIntArray surroundingVertices;
        polyIter.getVertices(surroundingVertices);
        this->FaceToVertices[faceTmp] = surroundingVertices;

        std::vector<int> tmpSet;
        tmpSet.resize(surroundingVertices.length());
        for (unsigned int itVtx = 0; itVtx < surroundingVertices.length(); itVtx++)
            tmpSet[itVtx] = surroundingVertices[itVtx];
        std::sort(tmpSet.begin(), tmpSet.end());
        this->FaceToVerticesSet[faceTmp] = tmpSet;
    }

    // fill the std_array ------------------------
    this->connectedSetVertices.resize(this->numVertices);

    for (int vtxTmp = 0; vtxTmp < this->numVertices; ++vtxTmp) {
        std::vector<int> connVetsSet2;

        MIntArray connectFaces = this->connectedFaces[vtxTmp];

        for (int fct = 0; fct < connectFaces.length(); ++fct) {
            auto surroundingVertices = this->FaceToVerticesSet[connectFaces[fct]];
            connVetsSet2 = connVetsSet2 + surroundingVertices;
        }
        connVetsSet2 = connVetsSet2 - std::vector<int>(1, vtxTmp);
        this->connectedSetVertices[vtxTmp] = connVetsSet2;
    }
    // get the edgesIndices to draw the wireframe --------------------
    MItMeshEdge edgeIter(meshDag);
    this->edgeVerticesIndices.resize(edgeIter.count());
    unsigned int i = 0;
    for (; !edgeIter.isDone(); edgeIter.next()) {
        int pt0Index = edgeIter.index(0);
        int pt1Index = edgeIter.index(1);
        this->edgeVerticesIndices[i++] = std::make_pair(pt0Index, pt1Index);
        /*
        if (pt0Index < numVertices || pt1Index < numVertices) {
                this->edgeVerticesIndices.append(pt0Index);
                this->edgeVerticesIndices.append(pt1Index);
        }
        */
    }

    // fill the normals ----------------------------------------------------
    this->normalsIds.resize(this->numFaces);
    MIntArray normalCounts, normals;
    this->meshFn.getNormalIds(normalCounts, normals);

    int startIndex = 0;
    for (unsigned int faceTmp = 0; faceTmp < normalCounts.length(); ++faceTmp) {
        int nbNormals = normalCounts[faceTmp];
        std::vector<int> tmpNormalsIds(nbNormals, 0);
        for (unsigned int k = 0; k < nbNormals; ++k) {
            int normalId = normals[k + startIndex];
            tmpNormalsIds[k] = normalId;
        }
        this->normalsIds[faceTmp] = tmpNormalsIds;
        startIndex += nbNormals;
    }
    MStatus stat;
    this->rawNormals = this->meshFn.getRawNormals(&stat);

    // get vertexNormalIndex -------------
    this->verticesNormalsIndices.setLength(numVertices);
    for (unsigned int vertexInd = 0; vertexInd < this->numVertices; vertexInd++) {
        int indFace = this->connectedFaces[vertexInd][0];  // get the first face
        MIntArray surroundingVertices = this->FaceToVertices[indFace];
        int indNormal = -1;
        for (unsigned int j = 0; j < surroundingVertices.length(); ++j) {
            if (surroundingVertices[j] == vertexInd) {
                indNormal = this->normalsIds[indFace][0];  // get correct index of vertex in face
            }
        }
        if (indNormal == -1) {
            MGlobal::displayInfo(MString("cant find vertex [") + vertexInd +
                                 MString("] in face [") + indFace + MString("] ;"));
        }
        this->verticesNormalsIndices.set(indNormal, vertexInd);
    }

    // test it works ----------------------------------------------------
    /*
    this->meshFn.getFaceNormalIds(402, normals);

    MString msg("nb normals vtx[402] -> ");
    msg += MString(" ( ") + normals.length() + MString(" ) - ( ") + this->normalsIds[402].size() +
    MString(" )\n"); for (int i = 0; i < normals.length(); ++i) { msg += MString("    ") +
    normals[i] + MString("  -  ") + this->normalsIds[402][i] + MString(" ");
    }
    msg += MString("\n");
    int prevIndex;
    polyIter.setIndex(402, prevIndex);
    MVector normal;
    polyIter.getNormal(2, normal);
    int indNormal = this->normalsIds[402][2];
    for (int i = 0; i <3; ++i) {
            msg += MString("    ") + normal[i] + MString("  -  ") + this->rawNormals[indNormal*3 +i]
    + MString(" ");
    }
    MGlobal::displayInfo(msg);
    */

    /*
    //std::sort(this->connectedSetVertices2.begin, this->connectedSetVertices2.end);
    MString msg("nb vertices connected vtx[402] -> ");
    msg += MString(" (") + this->connectedSetVertices[402].size() + MString(")") ;
    for (int vtx : this->connectedSetVertices[402])
            msg += MString(" ") + vtx;

    std::vector<int> intersection = this->connectedSetVertices[402] &
    this->connectedSetVertices[403]; msg += MString("\nintersection  vtx[402] vtx[403]  -> "); msg
    += MString(" (") + intersection.size() + MString(")"); for (int vtx : intersection) msg +=
    MString(" ") + vtx;

    std::vector<int> substract = this->connectedSetVertices[402] - this->connectedSetVertices[403];
    msg += MString("\nsubstract  vtx[402] - vtx[403]  -> ");
    msg += MString(" (") + substract.size() + MString(")");
    for (int vtx : substract)
            msg += MString(" ") + vtx;

    MGlobal::displayInfo(msg);
    */
}

//
// Description:
//      Get the dagPath of the currently selected object's shape node.
//      If there are multiple shape nodes return the first
//      non-intermediate shape. Return kNotFound if the object is not a
//      mesh.
//
// Input Arguments:
//      dagPath             The MDagPath of the selected mesh.
//
// Return Value:
//      MStatus             Return kNotFound if nothing is selected or
//                          the selection is not a mesh.
//
MStatus SkinBrushContext::getSelection(MDagPath &dagPath) {
    MStatus status = MStatus::kSuccess;

    unsigned int i;

    MSelectionList sel;
    status = MGlobal::getActiveSelectionList(sel);

    if (sel.isEmpty()) return MStatus::kNotFound;

    // Get the dagPath of the mesh before evaluating any selected
    // components. If there are no components selected the dagPath would
    // be empty and the command would fail to apply the smoothing to the
    // entire mesh.
    sel.getDagPath(0, dagPath);
    status = dagPath.extendToShape();

    // If there is more than one shape node extend to shape will fail.
    // In this case the shape node needs to be found differently.
    if (status != MStatus::kSuccess) {
        unsigned int numShapes;
        dagPath.numberOfShapesDirectlyBelow(numShapes);
        for (i = 0; i < numShapes; i++) {
            status = dagPath.extendToShapeDirectlyBelow(i);
            if (status == MStatus::kSuccess) {
                MFnDagNode shapeDag(dagPath);
                if (!shapeDag.isIntermediateObject()) break;
            }
        }
    }

    if (!dagPath.hasFn(MFn::kMesh)) {
        dagPath = MDagPath();
        MGlobal::displayWarning("Only mesh objects are supported.");
        return MStatus::kNotFound;
    }

    if (!status) {
        MGlobal::clearSelectionList();
        dagPath = MDagPath();
    }

    return status;
}

//
// Description:
//      Parse the currently selected components and return a list of
//      related vertex indices. Edge or polygon selections are converted
//      to vertices.
//
// Input Arguments:
//      None
//
// Return Value:
//      The array of selected vertex indices
//
MIntArray SkinBrushContext::getSelectionVertices() {
    unsigned int i;

    MIntArray indices;
    MDagPath dagPath;
    MObject compObj;

    MSelectionList sel;
    MGlobal::getActiveSelectionList(sel);

    for (MItSelectionList selIter(sel, MFn::kMeshVertComponent); !selIter.isDone();
         selIter.next()) {
        selIter.getDagPath(dagPath, compObj);
        if (!compObj.isNull()) {
            for (MItMeshVertex vertexIter(dagPath, compObj); !vertexIter.isDone();
                 vertexIter.next()) {
                indices.append(vertexIter.index());
            }
        }
    }

    for (MItSelectionList selIter(sel, MFn::kMeshEdgeComponent); !selIter.isDone();
         selIter.next()) {
        selIter.getDagPath(dagPath, compObj);
        if (!compObj.isNull()) {
            for (MItMeshEdge edgeIter(dagPath, compObj); !edgeIter.isDone(); edgeIter.next()) {
                indices.append(edgeIter.index(0));
                indices.append(edgeIter.index(1));
            }
        }
    }

    for (MItSelectionList selIter(sel, MFn::kMeshPolygonComponent); !selIter.isDone();
         selIter.next()) {
        selIter.getDagPath(dagPath, compObj);
        if (!compObj.isNull()) {
            for (MItMeshPolygon polyIter(dagPath, compObj); !polyIter.isDone(); polyIter.next()) {
                MIntArray vertices;
                polyIter.getVertices(vertices);
                for (i = 0; i < vertices.length(); i++) indices.append(vertices[i]);
            }
        }
    }

    // Remove any double entries from the component list.
    // MFnSingleIndexedComponent does that automatically.
    MFnSingleIndexedComponent compFn;
    MObject verticesObj = compFn.create(MFn::kMeshVertComponent);
    compFn.addElements(indices);
    // Put the processed ids back into the array.
    compFn.getElements(indices);

    return indices;
}

//
// Description:
//      Parse the history of the mesh at the given dagPath and return
//      MObject of the skin cluster node.
//
// Input Arguments:
//      dagPath             The MDagPath of the selected mesh.
//      skinClusterObj      The MObject of the found skin cluster node.
//
// Return Value:
//      MStatus             The MStatus for the setting up the
//                          dependency graph iterator.
//
MStatus SkinBrushContext::getSkinCluster(MDagPath meshDag, MObject &skinClusterObj) {
    MStatus status;

    MObject meshObj = meshDag.node();

    MItDependencyGraph dependIter(meshObj, MFn::kSkinClusterFilter, MItDependencyGraph::kUpstream,
                                  MItDependencyGraph::kDepthFirst, MItDependencyGraph::kPlugLevel,
                                  &status);
    if (!status) {
        MGlobal::displayError("Failed setting up the dependency graph iterator.");
        return status;
    }

    if (!dependIter.isDone()) {
        skinClusterObj = dependIter.currentItem();
        MFnDependencyNode skinDep(skinClusterObj);
        // now find the blurSkinDisplay plug ----------------
        MPlug inputGeometryPlug = skinDep.findPlug("inputGeometry");
        MPlugArray connections;
        inputGeometryPlug.connectedTo(connections, true, false);
        if (connections.length() > 0) {
            MObject inConn = connections[0].node();
            MFnDependencyNode inDep(inConn);
            if (inDep.typeName() == "blurSkinDisplay") {
                MPlug paintAttrPlug = inDep.findPlug("paintAttr");
                // get values
                MDoubleArray paintArrayValues(numVertices, 0);
                status = paintAttrPlug.getValue(attrValue);
                MFnDoubleArrayData doubleArrayDataFn(attrValue, &status);
                valuesForAttribute = doubleArrayDataFn.array(&status);
                valuesForAttribute.copy(paintArrayValues);
                MGlobal::displayInfo(MString("found blurSkinDisplay -> ") + paintAttrPlug.name());
                foundBlurSkinAttribute = true;
            }
        }
    }

    // Make sure that the mesh is bound to a skin cluster.
    if (skinClusterObj.isNull()) {
        MGlobal::displayWarning("The selected mesh is not bound to a skin cluster.");
        return MStatus::kNotFound;
    }

    return status;
}

//
// Description:
//      Get weights for all vertices and populate the currentWeights
//      array. Also copy the weights to the prevWeights array for undo.
//
// Input Arguments:
//      None
//
// Return Value:
//      MStatus             The MStatus for creating the MFnSkinCluster.
//
/*
MStatus SkinBrushContext::getAllWeights()
{
    MStatus status = MStatus::kSuccess;
        return status;

    MFnSkinCluster skinFn(skinObj, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    skinFn.getWeights(meshDag, allVtxCompObj, currentWeights, this->influenceCount);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    // Copy the current weights for undo.
    prevWeights.copy(currentWeights);

    return status;

}
*/
MStatus SkinBrushContext::fillArrayValues(MObject skinCluster, bool doColors) {
    MStatus status = MS::kSuccess;
    if (verbose) MGlobal::displayInfo(" FILLED ARRAY VALUES ");

    MFnSkinCluster skinFn(skinCluster, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    unsigned int infCount;

    skinFn.getWeights(meshDag, allVtxCompObj, this->skinWeightList, infCount);
    CHECK_MSTATUS_AND_RETURN_IT(status);
    this->nbJoints = infCount;

    // quickly the ignore locks
    this->ignoreLockJoints.clear();
    this->ignoreLockJoints = MIntArray(this->nbJoints, 0);

    if (doColors) {
        skin_weights_.resize(this->numVertices);

        this->multiCurrentColors.clear();
        this->multiCurrentColors.setLength(this->numVertices);
        // get values for array --
        for (unsigned int vertexIndex = 0; vertexIndex < this->numVertices; ++vertexIndex) {
            MColor theColor(0.0, 0.0, 0.0);
            for (unsigned int indexInfluence = 0; indexInfluence < infCount;
                 indexInfluence++) {  // for each joint
                double theWeight = this->skinWeightList[vertexIndex * infCount + indexInfluence];

                if (doColors) {
                    theColor += this->jointsColors[indexInfluence] * theWeight;
                    if ((verbose) && (vertexIndex == 144)) {
                        MGlobal::displayInfo(MString(" jnt : [") + indexInfluence +
                                             MString("] VTX 144 R: ") + theColor.r +
                                             MString("  G: ") + theColor.g + MString("  B: ") +
                                             theColor.b + MString("  "));
                    }
                }
            }
            if (doColors) {  // not store lock vert color
                this->multiCurrentColors[vertexIndex] = theColor;
                if ((verbose) && (vertexIndex == 144)) {
                    MGlobal::displayInfo(MString(" VTX 144 R: ") + theColor.r + MString("  G: ") +
                                         theColor.g + MString("  B: ") + theColor.b +
                                         MString("  "));
                }
            }
        }
    }

    return status;
}

/*
MStatus SkinBrushContext::fillArrayValues(MObject skinCluster, bool doColors) {
        MStatus status = MS::kSuccess;
        if (verbose) MGlobal::displayInfo(" FILLED ARRAY VALUES ");

        MFnDependencyNode skinClusterDep(skinCluster);

        MPlug weight_list_plug = skinClusterDep.findPlug("weightList");
        MPlug matrix_plug = skinClusterDep.findPlug("matrix");
        //MGlobal::displayInfo(weight_list_plug.name());
        int nbElements = weight_list_plug.numElements();
        this->nbJoints = matrix_plug.numElements();

        matrix_plug.getExistingArrayAttributeIndices(this->deformersIndices);

        this->nbJointsBig = this->deformersIndices[this->deformersIndices.length() - 1] +
1;//matrix_plug.evaluateNumElements(); if (verbose) MGlobal::displayInfo(MString(" nb jnts ") +
this->nbJoints + MString("  ") + this->nbJointsBig); this->nbJoints = this->nbJointsBig;


        // quickly the ignore locks
        this->ignoreLockJoints.clear();
        this->ignoreLockJoints = MIntArray(this->nbJoints, 0);


        skin_weights_.resize(nbElements);
        if (doColors) {
                this->multiCurrentColors.clear();
                this->multiCurrentColors.setLength(nbElements);
        }
        this->skinWeightList = MDoubleArray(nbElements * this->nbJoints, 0.0);

        for (int i = 0; i < nbElements; ++i) {
                // weightList[i]
                MPlug ith_weights_plug = weight_list_plug.elementByPhysicalIndex(i);
                int vertexIndex = ith_weights_plug.logicalIndex();
                //MGlobal::displayInfo(ith_weights_plug.name());

                // weightList[i].weight
                MPlug plug_weights = ith_weights_plug.child(0); // access first compound child
                int  nb_weights = plug_weights.numElements();
                skin_weights_[i].resize(nb_weights);
                //skin_weights_[i].resize(nbJointPlugElements);
                //MGlobal::displayInfo(plug_weights.name() + nb_weights);

                MColor theColor;
                for (int j = 0; j < nb_weights; j++) { // for each joint
                        MPlug weight_plug = plug_weights.elementByPhysicalIndex(j);
                        // weightList[i].weight[j]
                        int indexInfluence = weight_plug.logicalIndex();
                        double theWeight = weight_plug.asDouble();

                        skin_weights_[i][j] = std::make_pair(indexInfluence, theWeight);
                        this->skinWeightList[vertexIndex*this->nbJoints + indexInfluence] =
theWeight; if (doColors) // and not locked theColor += this->jointsColors[indexInfluence] *
theWeight;
                }
                if (doColors) // not store lock vert color
                        this->multiCurrentColors[vertexIndex] = theColor;
        }
        return status;
}
*/

MStatus SkinBrushContext::querySkinClusterValues(MObject skinCluster, MIntArray &verticesIndices,
                                                 bool doColors) {
    MStatus status = MS::kSuccess;

    MFnSingleIndexedComponent compFn;
    MObject weightsObj = compFn.create(MFn::kMeshVertComponent);
    compFn.addElements(verticesIndices);

    if (meshDag.node().isNull()) return MStatus::kNotFound;

    MFnSkinCluster skinFn(skinCluster, &status);
    MDoubleArray weightsVertices;
    unsigned int infCount;
    skinFn.getWeights(meshDag, weightsObj, weightsVertices, infCount);

    for (unsigned int i = 0; i < verticesIndices.length(); ++i) {
        int vertexIndex = verticesIndices[i];

        MColor theColor;
        for (unsigned int j = 0; j < infCount; j++) {  // for each joint
            double theWeight = weightsVertices[i * infCount + j];
            this->skinWeightList[vertexIndex * this->nbJoints + j] = theWeight;

            if (doColors) {
                theColor += this->jointsColors[j] * theWeight;
            }
        }
    }
    return status;
    /*

    MFnDependencyNode skinClusterDep(skinCluster);

    MPlug weight_list_plug = skinClusterDep.findPlug("weightList");

    for (int i = 0; i < verticesIndices.length(); ++i) {
            int vertexIndex = verticesIndices[i];
            // weightList[i]
            MPlug ith_weights_plug = weight_list_plug.elementByLogicalIndex(vertexIndex);

            // weightList[i].weight
            MPlug plug_weights = ith_weights_plug.child(0); // access first compound child
            int  nb_weights = plug_weights.numElements();

            MColor theColor;
            for (int j = 0; j < nb_weights; j++) { // for each joint
                    MPlug weight_plug = plug_weights.elementByPhysicalIndex(j);
                    // weightList[i].weight[j]
                    int indexInfluence = weight_plug.logicalIndex();
                    double theWeight = weight_plug.asDouble();

                    this->skinWeightList[vertexIndex*this->nbJoints + indexInfluence] = theWeight;
                    if (doColors)
                            theColor += this->jointsColors[indexInfluence] * theWeight;
            }
            if (doColors) this->multiCurrentColors[vertexIndex] = theColor;
    }
    return status;
    */
}

//
// Description:
//      Get the influence attributes from the given skin cluster object.
//
// Input Arguments:
//      skinCluster             The MObject of the skin cluster node.
//      maxInfluences           The max number of influences per vertex.
//      maintainMaxInfluences   True, if the max influences count should
//                              be maintained.
//      normalize               True, if the weights are normalized.
//
// Return Value:
//      None
//
void SkinBrushContext::getSkinClusterAttributes(MObject skinCluster, unsigned int &maxInfluences,
                                                bool &maintainMaxInfluences,
                                                unsigned int &normalize) {
    // Get the settings from the skin cluster node.
    MFnDependencyNode skinMFn(skinCluster);

    MPlug maxInflPlug = skinMFn.findPlug("maxInfluences", false);
    maxInfluences = (unsigned)maxInflPlug.asInt();

    MPlug maintainInflPlug = skinMFn.findPlug("maintainMaxInfluences", false);
    maintainMaxInfluences = maintainInflPlug.asBool();

    MPlug normalizePlug = skinMFn.findPlug("normalizeWeights", false);
    normalize = (unsigned)normalizePlug.asInt();
}

//
// Description:
//      Return the influence indices of all influences of the given
//      skin cluster node.
//
// Input Arguments:
//      skinCluster         The MObject of the skin cluster node.
//
// Return Value:
//      int array           The array of all influence indices.
//
MIntArray SkinBrushContext::getInfluenceIndices(MObject skinCluster, MDagPathArray &dagPaths) {
    unsigned int i;
    MFnSkinCluster skinFn(skinCluster);

    MIntArray influenceIndices;
    skinFn.influenceObjects(dagPaths);
    int lent = dagPaths.length();
    this->inflNames.setLength(lent);
    this->inflNamePixelSize.setLength(2 * lent);
    for (i = 0; i < lent; i++) {
        influenceIndices.append((int)i);

        MFnDependencyNode influenceFn(dagPaths[i].node());
        this->inflNames[i] = influenceFn.name();
        // get pixels size ----------
        MIntArray result;
        MString cmd2 = MString("fnFonts (\"") + this->inflNames[i] + MString("\")");
        MGlobal::executePythonCommand(cmd2, result);
        this->inflNamePixelSize[2 * i] = result[0];
        this->inflNamePixelSize[2 * i + 1] = result[1];
        // MGlobal::displayInfo(MString(" ") + i + MString(" ") + this->inflNames[i] + MString("
        // "));
    }

    return influenceIndices;
}

bool SkinBrushContext::computeHit(short screenPixelX, short screenPixelY, bool getNormal,
                                  int &faceHit, MFloatPoint &hitPoint) {
    MStatus stat;

    MPoint worldPoint;
    view.viewToWorld(screenPixelX, screenPixelY, worldPoint, worldVector);

    // float hitRayParam;

    bool foundIntersect =
        meshFn.closestIntersection(worldPoint, worldVector, nullptr, nullptr, false, MSpace::kWorld,
                                   9999, false, &this->accelParams, hitPoint, &this->pressDistance,
                                   &faceHit, nullptr, nullptr, nullptr, 0.0001f, &stat);

    /*
    bool foundIntersect = meshFn.closestIntersection(worldPoint, worldVector, nullptr, nullptr,
    false, MSpace::kWorld, 9999, false, nullptr, hitPoint, &this->pressDistance, &faceHit, nullptr,
    nullptr, nullptr, 0.0001f, &stat);
    */
    if (!foundIntersect) return false;
    // ----------- get normal for display ---------------------
    if (getNormal)
        meshFn.getClosestNormal(hitPoint, normalVector);  // , MSpace::kWorld, &closestPolygon);
    return true;
}
bool SkinBrushContext::expandHit(int faceHit, MFloatPoint hitPoint,
                                 std::unordered_map<int, float> &dicVertsDist) {
    // ----------- compute the vertices around ---------------------
    auto verticesSet = this->FaceToVerticesSet[faceHit];
    bool foundHit = false;
    for (int ptIndex : verticesSet) {
        MFloatPoint posPoint(this->mayaRawPoints[ptIndex * 3], this->mayaRawPoints[ptIndex * 3 + 1],
                             this->mayaRawPoints[ptIndex * 3 + 2]);
        float dist = posPoint.distanceTo(hitPoint);
        if (dist <= this->sizeVal) {
            foundHit = true;
            auto ret = dicVertsDist.insert(std::make_pair(ptIndex, dist));
            if (!ret.second) ret.first->second = std::min(dist, ret.first->second);
            // faceVertsDist.push_back(std::make_pair(ptIndex, dist));
        }
    }
    return foundHit;
}

//
// Description:
//      Go through the all vertices which are closest to the cursor,
//      and get all connected vertices which are in range of the brush
//      radius. Perform the averaging of the weights for all found
//      vertices in a threaded loop.
//
// Input Arguments:
//      event               The mouse event.
//      indices             The list of vertex indices along the
//                          intersection ray.
//      distances           The list of distances of the vertices to the
//                          intersection ray.
//
// Return Value:
//      MStatus             The MStatus for initializing the skin
//                          cluster.
//

void SkinBrushContext::prepareArray(std::unordered_map<int, float> &dicVertsDist) {
    double valueStrength = strengthVal;
    if (this->modifierNoneShiftControl == 2 || this->commandIndex == 4)
        valueStrength = smoothStrengthVal;  // smooth always we use the smooth value different of
                                            // the regular value

    if (fractionOversamplingVal) valueStrength /= oversamplingVal;

    if (!this->doAddingDots) {  // set upToOne
        for (auto &element : dicVertsDist) {
            float value = 1.0 - (element.second / this->sizeVal);
            value = (float)getFalloffValue(value, valueStrength);
            element.second = value;
        }
    }
}

void SkinBrushContext::setColor(int vertexIndex, float value, MIntArray &editVertsIndices,
                                MColorArray &multiEditColors, MColorArray &soloEditColors) {
    if (verbose) MGlobal::displayInfo(MString("         --> actually painting weights"));
    MColor white(1, 1, 1, 1);
    MColor black(0, 0, 0, 1);

    int theCommandIndex = this->commandIndex;
    if (this->commandIndex == 0 && this->modifierNoneShiftControl == 1)
        theCommandIndex = 1;  // remove
    if (this->commandIndex == 6 && this->modifierNoneShiftControl == 1)
        theCommandIndex = 7;                                       // unlockVertices
    if (this->modifierNoneShiftControl == 2) theCommandIndex = 4;  // smooth always

    if (theCommandIndex >= 6) {  // painting locks
        bool doStoreLock = (theCommandIndex == 6 && !this->lockVertices[vertexIndex]) ||
                           (theCommandIndex == 7 && this->lockVertices[vertexIndex]);
        if (doStoreLock) {
            if (!this->mirrorIsActive) {     // we do the colors diferently if mirror is active
                if (theCommandIndex == 6) {  // lock verts if not already locked
                    multiEditColors.append(this->lockVertColor);
                    soloEditColors.append(this->lockVertColor);
                } else {  // unlock verts
                    multiEditColors.append(this->multiCurrentColors[vertexIndex]);
                    soloEditColors.append(this->soloCurrentColors[vertexIndex]);
                }
            }
            editVertsIndices.append(vertexIndex);
            // editVertsWeights.append(1.0);
            this->intensityValues[vertexIndex] = 1;  // store to not repaint
        }
    } else if (!this->lockVertices[vertexIndex]) {
        MColor soloColor, multColor;
        editVertsIndices.append(vertexIndex);
        // editVertsWeights.append(val);
        MColor currentColor = this->multiCurrentColors[vertexIndex];
        // float val = std::log10(value * 9 + 1);
        if (!this->mirrorIsActive) {  // we do the colors diferently if mirror is active

            // 0 Add - 1 Remove - 2 AddPercent - 3 Absolute - 4 Smooth - 5 Sharpen - 6 LockVertices
            // - 7 UnLockVertices
            if (theCommandIndex < 4) {
                double newW = this->skinWeightList[vertexIndex * nbJoints + this->influenceIndex];
                if (theCommandIndex == 0) {
                    newW += value;  // ADD
                    newW = std::min(1.0, newW);
                    multColor = currentColor * (1.0 - newW) +
                                this->jointsColors[this->influenceIndex] * newW;  // white
                } else if (theCommandIndex == 1) {
                    newW -= value;  // Remove
                    newW = std::max(0.0, newW);
                    multColor = currentColor * (1.0 - value) + black * value;  // white
                } else if (theCommandIndex == 2) {
                    newW += value * newW;  // AddPercent
                    newW = std::min(1.0, newW);
                    multColor = currentColor * (1.0 - newW) +
                                this->jointsColors[this->influenceIndex] * newW;  // white
                } else if (theCommandIndex == 3) {
                    newW = value;                                              // Absolute
                    multColor = currentColor * (1.0 - value) + black * value;  // white
                }
                soloColor = getASoloColor(newW);
            } else {  // smooth and sharpen
                soloColor = value * white + (1.0 - value) * this->soloCurrentColors[vertexIndex];
                multColor = value * white + (1.0 - value) * this->multiCurrentColors[vertexIndex];
            }
            multiEditColors.append(multColor);
            soloEditColors.append(soloColor);
        }
    }
    /*
    if (this->mirrorIsActive) {
            int mirrorInfluenceIndex = this->mirrorInfluences[this->influenceIndex];
            bool doMerge = (this->influenceIndex == mirrorInfluenceIndex) || (this->commandIndex ==
    4) || (this->commandIndex == 5); doMerge = false; getMirrorVertices(this->mirrorVertices,
    editVertsIndices, theMirrorVerts, editAndMirrorVerts, editVertsWeights, mirrorVertsWeights,
    editAndMirrorWeights, doMerge);
            // edit more colors ie the sym colors
            for (int i = 0; i < editAndMirrorVerts.length(); ++i) {
                    double val = editAndMirrorWeights[i];
                    int vert = editAndMirrorVerts[i];
                    if (this->commandIndex == 6) {//lock verts
                            multiEditColors.append(this->lockVertColor);
                            soloEditColors.append(this->lockVertColor);
                    }
                    else if (this->commandIndex == 7) {// unlock verts
                            multiEditColors.append(this->multiCurrentColors[vert]);
                            soloEditColors.append(this->soloCurrentColors[vert]);
                    }
                    else {
                            multiEditColors.append(val*multColor + (1.0 - val) *
    this->multiCurrentColors[vert]); soloEditColors.append(val*soloMultColor + (1.0 - val) *
    this->soloCurrentColors[vert]);
                    }
            }
    }
    */
}

MStatus SkinBrushContext::performPaint(std::unordered_map<int, float> &dicVertsDist,
                                       std::unordered_map<int, float> &dicVertsDistRed) {
    MStatus status = MStatus::kSuccess;

    // MGlobal::displayInfo("perform Paint");
    double multiplier = 1.0;
    if (!this->postSetting && this->commandIndex != 4)
        multiplier = .1;  // less applying if dragging paint

    if (this->modifierNoneShiftControl == 2 && this->commandIndex >= 6) return status;
    if (!volumeVal) {
        MColorArray multiEditColors, soloEditColors;
        MIntArray editVertsIndices, theMirrorVerts, editAndMirrorVerts;
        MDoubleArray editVertsWeights, mirrorVertsWeights, editAndMirrorWeights;

        MColor black(0, 0, 0, 1);

        auto endOfFind = previousPaint.end();
        for (const auto &element : dicVertsDist) {
            int index = element.first;
            float value = element.second * multiplier;

            // check if need to set this color, we store somehow -------
            if ((this->lockVertices[index] == 1 && this->commandIndex < 6) ||
                this->intensityValues[index] == 1)
                continue;

            // get the correct value of paint --
            value += this->intensityValues[index];
            auto res = this->previousPaint.find(index);
            if (res != endOfFind) {  // we substract the smallest
                value -= std::min(res->second, element.second);
            }
            value = std::min(value, (float)1.0);
            this->intensityValues[index] = value;

            // paintArrayValues [index] = value // only if we update an array and still
            // --------------
            // add to array of values to set at the end---------------
            auto ret = this->skinValuesToSet.insert(std::make_pair(index, double(value)));
            if (!ret.second)
                ret.first->second = std::max(double(value), ret.first->second);
            else
                this->verticesPainted.insert(index);

            // end add to array of values to set  at the end---------------

            // now for colors -------------------------------------------------
            setColor(index, value, editVertsIndices, multiEditColors, soloEditColors);
        }
        // store this paint ---------------
        this->previousPaint = dicVertsDist;

        // do actually set colors -----------------------------------
        if (this->soloColorVal == 0) {
            meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet);
            meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet2);
        } else {
            meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet);
            meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet2);
        }
    } else {
        MIntArray volumeIndices = getVerticesInVolume();
    }
    // don't know why but we need that ------------
    // meshFn.setDisplayColors(true);
    if (!this->postSetting) {
        // MGlobal::displayInfo("apply the skin stuff");
        // still have to deal with the colors damn it
        if (this->skinValuesToSet.size() > 0) {
            applyCommand(this->influenceIndex, this->skinValuesToSet, !this->mirrorIsActive);
            this->intensityValues = std::vector<float>(this->numVertices, 0);
            this->previousPaint.clear();
            this->skinValuesToSet.clear();
            // replace colors
        }
    }
    // view.refresh(false, true);
    maya2019RefreshColors(true);

    return status;
}

//
// Description:
//      Go through the all vertices which are closest to the cursor,
//      and get all connected vertices which are in range of the brush
//      radius. Perform the selection.
//
// Input Arguments:
//      event               The mouse event.
//      indices             The list of vertex indices along the
//                          intersection ray.
//      distances           The list of distances of the vertices to the
//                          intersection ray.
//
// Return Value:
//      MStatus             The MStatus for selecting the components.
//
MStatus SkinBrushContext::performSelect(MEvent event, MIntArray indices, MFloatArray distances) {
    MStatus status = MStatus::kSuccess;

    MFnSingleIndexedComponent comp;
    MObject compObj = comp.create(MFn::kMeshVertComponent);

    if (!volumeVal) {
        comp.addElements(indices);
    } else {
        MIntArray volumeIndices = getVerticesInVolume();
        comp.addElements(volumeIndices);
    }

    MSelectionList sel;
    sel.add(meshDag, compObj);
    if (event.isModifierShift()) MGlobal::setActiveSelectionList(sel, MGlobal::kAddToList);
    if (event.isModifierControl()) MGlobal::setActiveSelectionList(sel, MGlobal::kRemoveFromList);

    // Check the current selection. If any previsouly selected vertices
    // have been deselected with the control modifier, switch back to
    // object selection by selecting the mesh. If this doesn't happen
    // the active selection list will be empty for the next event,
    // resulting in a MStatus failure which prevents that the
    // selectedIndices list doesn't get reset and the brush still
    // operates on the previous vertex selection.
    MGlobal::getActiveSelectionList(sel);
    if (!sel.length()) {
        sel.add(meshDag);
        MGlobal::setActiveSelectionList(sel, MGlobal::kReplaceList);
    }

    view.refresh(false, true);

    return status;
}

//
// Description:
//      Compute the smoothing for either all vertices if only the mesh
//      is selected or just the current vertex selection.
//
// Input Arguments:
//      None
//
// Return Value:
//      None
//
void SkinBrushContext::performFlood() {
    unsigned int i;

    MEvent event;
    // Execute the common press method to get the current selection and
    // initialize the smoothing. Pass an empty event which tells the
    // method to simply smooth with the current strength value.
    doPressCommon(event);

    // If the current selection doesn't contain any components fill the
    // array with the indices of all vertices of the mesh.
    if (!vtxSelection.length()) {
        MFnSingleIndexedComponent compFn(allVtxCompObj);
        compFn.getElements(vtxSelection);
    }

    // Safety measure just to make sure that there are indices present.
    if (!vtxSelection.length()) return;

    // Reverse the index list if only the unselected vertices should get
    // smoothed.
    if (!affectSelectedVal) {
        // Create an array for all indices and set the indices of the
        // current selection to true.
        std::vector<bool> current(numVertices, false);
        for (i = 0; i < vtxSelection.length(); i++) current[(unsigned)vtxSelection[i]] = true;

        // Clear the current index array.
        vtxSelection.clear();

        // Create a new array with all unselected indices.
        for (i = 0; i < numVertices; i++) {
            if (!current[i]) vtxSelection.append((int)i);
        }
    }

    // Create an array with only the first index of the selection to be
    // able to call performSmooth(). This is identical to passing the
    // closest vertex to the brush when painting.
    MIntArray indices;
    indices.append(vtxSelection[0]);
    MFloatArray values;

    // Perform the smoothing.
    performBrush = true;

    // Finalize.
    doReleaseCommon(event);
}

//
// Description:
//      Return a component MObject for all vertex components of the
//      given mesh.
//
// Input Arguments:
//      meshDag             The dagPath of the mesh object.
//
// Return Value:
//      MObject             The component object for all mesh vertices.
//
MObject SkinBrushContext::allVertexComponents(MDagPath meshDag) {
    MFnSingleIndexedComponent compFn;
    MObject vtxComponents = compFn.create(MFn::kMeshVertComponent);
    MFnMesh meshFn(meshDag);
    compFn.setCompleteData((int)numVertices);
    return vtxComponents;
}

//
// Description:
//      Return a sorted index array defined by the given value array.
//      The sorting is based on the insertion sorting algorithm:
//      www.sorting-algorithms.com
//
// Input Arguments:
//      indices             The array of indices to sort.
//      values              The value array the sorting is based on.
//
// Return Value:
//      The sorted array of indices.
//
MIntArray SkinBrushContext::sortIndicesByValues(MIntArray indices, MDoubleArray values) {
    unsigned int i, j;

    for (i = 0; i < values.length(); i++) {
        for (j = i; j >= 1 && values[j] < values[j - 1]; j--) {
            double value = values[j - 1];
            values.remove(j - 1);
            values.insert(value, j);

            int index = indices[j - 1];
            indices.remove(j - 1);
            indices.insert(index, j);
        }
    }

    return indices;
}

void SkinBrushContext::getVerticesInRangeFast(int index, int hitIndex, MIntArray &indices,
                                              MFloatArray &values) {
    // This array stored which indices have beeen visited by setting
    // their index to false.
    std::vector<bool> visited(numVertices, false);
    // we need 2 arrays to avoid expending an array while in a loop ?
    std::unordered_set<int> setOfVerts;
    // MIntArray setOfVerts;
    std::unordered_set<int> setOfVertsGrow;
    setOfVertsGrow.insert(index);

    MFloatPoint centerPt = surfacePoints[(unsigned)hitIndex];
    bool processing = true;
    while (processing) {
        setOfVerts.clear();
        for (int itVtx : setOfVertsGrow) setOfVerts.insert(itVtx);
        setOfVertsGrow.clear();
        processing = false;
        for (int vertexIndex : setOfVerts) {
            // for (i = 0; i < setOfVerts.length(); ++i) {
            // int vertexIndex = setOfVerts[i];
            if (visited[vertexIndex]) continue;
            visited[vertexIndex] = true;
            MFloatPoint posPoint(this->mayaRawPoints[vertexIndex * 3],
                                 this->mayaRawPoints[vertexIndex * 3 + 1],
                                 this->mayaRawPoints[vertexIndex * 3 + 2]);
            // here make a better distance computation
            float dist = posPoint.distanceTo(centerPt);
            // Find which index is closest and store it along with the distance
            if (dist <= sizeVal) {
                processing = true;
                indices.append(vertexIndex);
                values.append((float)(1 - (dist / sizeVal)));

                // add surrounded vertices for consideration
                MIntArray surroundingVertices = this->connectedVertices[vertexIndex];
                for (unsigned int itVtx = 0; itVtx < surroundingVertices.length(); itVtx++)
                    setOfVertsGrow.insert(surroundingVertices[itVtx]);
            }
        }
    }
}

//
// Description:
//      Get the connected vertices of the given index and append then to
//      the given array of indices.
//
// Input Arguments:
//      index               The vertex index.
//      indices             The array of connected indices to append to.
//
// Return Value:
//      None
//
void SkinBrushContext::appendConnectedIndices(int index, MIntArray &indices) {
    unsigned int i;

    MItMeshVertex vtxIter(meshDag);

    int prevIndex;
    vtxIter.setIndex(index, prevIndex);

    MIntArray vertexList;
    vtxIter.getConnectedVertices(vertexList);

    for (i = 0; i < vertexList.length(); i++) indices.append(vertexList[i]);
}

//
// Description:
//      Return the vertex indices within the brush volume.
//
// Input Arguments:
//      None
//
// Return Value:
//      int array           The array of indices in the volume.
//
MIntArray SkinBrushContext::getVerticesInVolume() {
    MIntArray indices;

    double radius = sizeVal * sizeVal;

    MItMeshVertex vtxIter(meshDag);
    while (!vtxIter.isDone()) {
        MPoint pnt = vtxIter.position(MSpace::kWorld);

        double x = pnt.x - surfacePoints[0].x;
        double y = pnt.y - surfacePoints[0].y;
        double z = pnt.z - surfacePoints[0].z;

        x *= x;
        y *= y;
        z *= z;

        if (x + y + z <= radius) indices.append(vtxIter.index());

        vtxIter.next();
    }

    return indices;
}

//
// Description:
//      Return the vertex indices of the brush volume which are within
//      the range of the current index.
//
// Input Arguments:
//      index               The vertex index.
//      volumeIndices       The array of all indices of the brush
//                          volume.
//      rangeIndices        The array of indices which are in range of
//                          the vertex.
//      values              The array of falloff values based on the
//                          distance to the center vertex.
//
// Return Value:
//      None
//
void SkinBrushContext::getVerticesInVolumeRange(int index, MIntArray volumeIndices,
                                                MIntArray &rangeIndices, MFloatArray &values) {
    unsigned int i;

    double radius = sizeVal * rangeVal;
    radius *= radius;

    double smoothStrength = strengthVal;
    if (fractionOversamplingVal) smoothStrength /= oversamplingVal;

    MItMeshVertex vtxIter(meshDag);
    int prevIndex;
    vtxIter.setIndex(index, prevIndex);

    MPoint point = vtxIter.position(MSpace::kWorld);

    for (i = 0; i < volumeIndices.length(); i++) {
        int volumeIndex = volumeIndices[i];

        vtxIter.setIndex(volumeIndex, prevIndex);

        MPoint pnt = vtxIter.position(MSpace::kWorld);

        double x = pnt.x - point.x;
        double y = pnt.y - point.y;
        double z = pnt.z - point.z;

        x *= x;
        y *= y;
        z *= z;

        double delta = x + y + z;

        if (volumeIndex != index && delta <= radius) {
            rangeIndices.append(volumeIndex);

            float value = (float)(1 - (delta / radius));
            value = (float)getFalloffValue(value, smoothStrength);
            values.append(value);
        }

        vtxIter.next();
    }
}

//
// Description:
//      Calculate the brush weight value based on the given linear
//      falloff value.
//
// Input Arguments:
//      value               The linear falloff value.
//      strength            The brush strength value.
//
// Return Value:
//      double              The brush curve-based falloff value.
//
double SkinBrushContext::getFalloffValue(double value, double strength) {
    if (curveVal == 0) return 1.0 * strength;
    // linear
    else if (curveVal == 1)
        return value * strength;
    // smoothstep
    else if (curveVal == 2)
        return (value * value * (3 - 2 * value)) * strength;
    // narrow - quadratic
    else if (curveVal == 3)
        return (1 - pow((1 - value) / 1, 0.4)) * strength;
    else
        return value;
}

//
// Description:
//      Return if the given event is valid by querying the mouse button
//      and testing the returned MStatus.
//
// Input Arguments:
//      event               The MEvent to test.
//
// Return Value:
//      bool                True, if the event is valid.
//
bool SkinBrushContext::eventIsValid(MEvent event) {
    MStatus status;
    event.mouseButton(&status);
    if (!status) return false;
    return true;
}

// ---------------------------------------------------------------------
// mesh boundary
// ---------------------------------------------------------------------

//
// Description:
//      Return if the given index is connected to a boundary edge.
//
// Input Arguments:
//      index               The index of the vertex.
//
// Return Value:
//      bool                True, if vertex lies on a boundary.
//
bool SkinBrushContext::onBoundary(int index) {
    unsigned int i;

    int prevIndex;

    MItMeshVertex vtxIter(meshDag);
    vtxIter.setIndex(index, prevIndex);

    MIntArray edges;
    vtxIter.getConnectedEdges(edges);

    MItMeshEdge edgeIter(meshDag);

    for (i = 0; i < edges.length(); i++) {
        edgeIter.setIndex(edges[i], prevIndex);
        if (edgeIter.onBoundary()) return true;
    }

    return false;
}

//
// Description:
//      Get the opposite index of the given boundary index which shares
//      the same position.
//
// Input Arguments:
//      point               The position of the source boundary index.
//      faces               The list of faces which are connected to the
//                          source vertex.
//      edges               The list of conneced edges to the source
//                          vertex which are needed to calculate an
//                          average edge length.
//      index               The index of the opposite vertex.
//
// Return Value:
//      bool                True, if an opposite vertex has been found.
//
bool SkinBrushContext::oppositeBoundaryIndex(MPoint point, MIntArray faces, MIntArray edges,
                                             int &index) {
    unsigned int i;

    bool result = false;

    int faceIndex;

    double edgeLength = averageEdgeLength(edges) * 0.25;

    // Make sure that the tolerance value is not larger than the average
    // edge length to avoid false assignments.
    if (toleranceVal > edgeLength) toleranceVal = edgeLength;

    // Find the opposite boundary index with the closestPoint operation.
    // At best, the first pass returns a face index which belongs to the
    // next shell. But it's possible that a face of the same shell is
    // returned. In this case the source point gets offset by a fraction
    // of the averaged edge length which eventually returns a face of
    // the next shell.
    if (!getClosestFace(point, faces, faceIndex)) {
        for (i = 0; i < 6; i++) {
            MPoint pnt = point;
            if (i == 0)
                pnt.x += edgeLength;
            else if (i == 1)
                pnt.y += edgeLength;
            else if (i == 2)
                pnt.z += edgeLength;
            else if (i == 3)
                pnt.x -= edgeLength;
            else if (i == 4)
                pnt.y -= edgeLength;
            else if (i == 5)
                pnt.z -= edgeLength;

            if (getClosestFace(pnt, faces, faceIndex)) break;
        }
    }

    MItMeshPolygon polyIter(meshDag);

    int prevIndex;
    polyIter.setIndex(faceIndex, prevIndex);

    // Get the vertices of the closest face.
    MIntArray vertices;
    polyIter.getVertices(vertices);

    // Go through the face vertices and check which one matches the
    // position of the initial boundary vertex.
    for (i = 0; i < vertices.length(); i++) {
        int vtx = vertices[i];

        MPoint vtxPoint;
        meshFn.getPoint(vtx, vtxPoint);
        if (vtxPoint.isEquivalent(point, toleranceVal)) {
            index = vtx;
            result = true;
            break;
        }
    }

    return result;
}

//
// Description:
//      Get the closest face to the given point and check if it belongs
//      to the list of given face indices.
//
// Input Arguments:
//      point               The position of the source boundary index.
//      faces               The list of faces which are connected to the
//                          source vertex.
//      index               The index of the closest face.
//
// Return Value:
//      bool                True, if the found face index doesn't
//                          belong to the source shell.
//
bool SkinBrushContext::getClosestFace(MPoint point, MIntArray faces, int &index) {
    unsigned int i;

    // Get the closest point to the given boundary point.
    MPointOnMesh meshPoint;
    intersector.getClosestPoint(point, meshPoint);

    // The face index of the closest point.
    index = meshPoint.faceIndex();

    // Check if the closest face is one of the source faces.
    for (i = 0; i < faces.length(); i++) {
        if (index == faces[i]) return false;
    }
    return true;
}

//
// Description:
//      Return the average edge length from the given list of edges.
//
// Input Arguments:
//      edges               The list of edge indices.
//
// Return Value:
//      double              The average edge length.
//
double SkinBrushContext::averageEdgeLength(MIntArray edges) {
    unsigned int i;

    double length = 0.0;

    unsigned int numEdges = edges.length();

    MItMeshEdge edgeIter(meshDag);

    int prevIndex;
    for (i = 0; i < numEdges; i++) {
        edgeIter.setIndex(edges[i], prevIndex);

        double value;
        edgeIter.getLength(value);

        length += value / numEdges;
    }

    return length;
}

void SkinBrushContext::setInViewMessage(bool display) {
    if (display && messageVal)
        MGlobal::executeCommand("brSkinBrushShowInViewMessage");
    else
        MGlobal::executeCommand("brSkinBrushHideInViewMessage");
}
