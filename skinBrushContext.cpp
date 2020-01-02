
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
    performRefreshViewPort = 0;

    colorVal = MColor(1.0, 0.0, 0.0);
    curveVal = 2;
    drawBrushVal = true;
    drawRangeVal = true;
    moduleImportString = MString("from brSkinBrush_pythonFunctions import ");
    enterToolCommandVal = "";
    exitToolCommandVal = "";
    fractionOversamplingVal = false;
    ignoreLockVal = false;
    lineWidthVal = 1;
    messageVal = 2;
    oversamplingVal = 1;
    rangeVal = 0.5;
    sizeVal = 5.0;
    strengthVal = 0.25;
    smoothStrengthVal = 1.0;

    pruneWeight = 0.0001;

    undersamplingVal = 2;
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
    MGlobal::displayInfo(MString(" --------->  moduleImportString : ") + moduleImportString);
    MGlobal::executePythonCommand(
        moduleImportString + MString("toolOnSetupEnd, toolOffCleanup, toolOnSetupStart, fnFonts, "
                                     "headsUpMessage, updateDisplayStrengthOrSize\n"));
    MGlobal::executePythonCommand("toolOnSetupStart()");

    this->pickMaxInfluenceVal = false;
    this->pickInfluenceVal = false;

    // first clear a bit the air --------------
    this->multiCurrentColors.clear();
    this->jointsColors.clear();
    this->soloCurrentColors.clear();

    status = getMesh();
    MIntArray editVertsIndices;
    if (!skinObj.isNull()) {
        getListColorsJoints(skinObj, jointsColors, verbose);  // get the joints colors

        // status = fillArrayValues(skinObj, true); // WAY TOO SLOW ... but accurate ?
        /*
        MGlobal::displayInfo(MString("SKN this->skinWeightList ") + this->skinWeightList.length());
        MGlobal::displayInfo(MString("SKN this->nbJoints  ") + this->nbJoints);
        int vertexIndex = 3805;
        displayWeightValue(vertexIndex);
        */
        this->skinWeightList.clear();
        status = fillArrayValuesDEP(skinObj, true);  // get the skin data and all the colors
        /*
        MGlobal::displayInfo(MString ("DEP this->skinWeightList ") + this->skinWeightList.length());
        MGlobal::displayInfo(MString ("DEP this->nbJoints  " )+ this->nbJoints);
        displayWeightValue(vertexIndex);
        */
        this->lockJoints = MIntArray(this->nbJoints, 0);
        this->ignoreLockJoints = MIntArray(this->nbJoints, 0);

        getListLockJoints(skinObj, this->lockJoints);
        getListLockVertices(skinObj, this->lockVertices, editVertsIndices);

        if (verbose)
            MGlobal::displayInfo(MString("nb found joints colors ") + jointsColors.length());
    } else {
        MGlobal::displayInfo(MString("FAILED : skinObj.isNull"));
        abortAction();
        return;
    }
    // get face color assignments ----------

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
    refreshColors(editVertsIndices, multiEditColors, soloEditColors);
    this->skinValuesToSet.clear();

    meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet);
    meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet);

    meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet2);
    meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet2);

    meshFn.setDisplayColors(true);

    view = M3dView::active3dView();
    view.refresh(false, true);

    MGlobal::executePythonCommand("toolOnSetupEnd()");
}

void SkinBrushContext::toolOffCleanup() {
    setInViewMessage(false);
    MGlobal::executeCommand(exitToolCommandVal);

    MGlobal::executePythonCommand("toolOffCleanup()");
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
    // this command is used when undo is called
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
    // query the Locks
    getListLockJoints(skinObj, this->lockJoints);
    MIntArray editVertsIndices;
    getListLockVertices(skinObj, this->lockVertices, editVertsIndices);

    // points and normals
    refreshPointsNormals();

    MColorArray multiEditColors, soloEditColors;
    refreshColors(verticesIndices, multiEditColors, soloEditColors);
    this->skinValuesToSet.clear();
    meshFn.setSomeColors(verticesIndices, multiEditColors, &this->fullColorSet);
    meshFn.setSomeColors(verticesIndices, soloEditColors, &this->soloColorSet);

    meshFn.setSomeColors(verticesIndices, multiEditColors, &this->fullColorSet2);
    meshFn.setSomeColors(verticesIndices, soloEditColors, &this->soloColorSet2);

    // if locking or unlocking
    // without that it doesn't refresh because mesh is not invalidated, meaning the skinCluster
    // hasn't changed
    meshFn.updateSurface();

    // refresh view and display
    // meshFn.setDisplayColors(true);
    maya2019RefreshColors();

    this->previousPaint.clear();
}

void SkinBrushContext::refresh() {
    MStatus status;
    if (verbose) MGlobal::displayInfo(" - REFRESH In CPP -");
    // refresh skinCluster
    //
    refreshPointsNormals();
    MIntArray editVertsIndices;

    if (!skinObj.isNull()) {
        // Get the skin cluster node from the history of the mesh.
        getListLockJoints(skinObj, this->lockJoints);
        getListColorsJoints(skinObj, this->jointsColors, this->verbose);  // get the joints colors
        status = getListLockVertices(skinObj, this->lockVertices, editVertsIndices);  // problem ?
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
    refreshColors(editVertsIndices, multiEditColors, soloEditColors);
    meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet);
    meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet);

    meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet2);
    meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet2);

    if (soloColorVal == 1) editSoloColorSet(true);  // solo

    // refresh view and display
    // meshFn.setDisplayColors(true);

    maya2019RefreshColors();
}

// ---------------------------------------------------------------------
// viewport 2.0
// ---------------------------------------------------------------------
int SkinBrushContext::getClosestInfluenceToCursor(int screenX, int screenY) {
    MStatus stat;
    MPoint nearClipPt, farClipPt;
    MVector direction, direction2;
    MPoint orig, orig2;
    view.viewToWorld(screenX, screenY, orig, direction);

    int lent = this->inflDagPaths.length();
    int closestInfluence = -1;
    double closestDistance = -1;

    for (unsigned int i = 0; i < lent; i++) {
        MMatrix matI = BBoxOfDeformers[i].mat.inverse();
        // MBoundingBox bbox = BBoxOfDeformers[i].bbox;
        orig2 = orig * matI;
        direction2 = direction * matI;
        // view.viewToObjectSpace(screenX, screenY, mat.inverse (), orig, direction);

        // MPoint intersection;
        // bool intersect  = bboxIntersection( bbox.min (), bbox.max (), mat, orig, direction,
        // intersection);

        // bool intersect = RayIntersectsBBox(bbox, orig, direction);
        MPoint minPt = BBoxOfDeformers[i].minPt;
        MPoint maxPt = BBoxOfDeformers[i].maxPt;
        MPoint center = BBoxOfDeformers[i].center;

        bool intersect = RayIntersectsBBox(minPt, maxPt, orig2, direction2);
        if (intersect) {
            double dst = center.distanceTo(orig2);
            // double dst =  intersection.distanceTo(orig);
            if ((closestInfluence == -1) || (dst < closestDistance)) {
                closestDistance = dst;
                closestInfluence = i;
            }
        }
    }
    return closestInfluence;
}

int SkinBrushContext::getHighestInfluence(int faceHit, MFloatPoint hitPoint) {
    // get closest vertex
    // auto verticesSet = this->perFaceVerticesSet[faceHit];
    auto verticesSet = getSurroundingVerticesPerFace(faceHit);
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

    int biggestInfluence = -1;
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
    for (const auto &pairEdges : this->perEdgeVertices) {
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
    drawManager.setDepthPriority(1);  // MRenderItem::sDormantWireDepthPriority
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
    bool displayPickInfluence = this->pickMaxInfluenceVal || this->pickInfluenceVal;
    if (this->pickInfluenceVal) {
        if (verbose) MGlobal::displayInfo("HERE pickInfluenceVal IS CALLED");
        // -------------------------------------------------------------------------------------------------
        // start fill jnts boundingBox
        // --------------------------------------------------------------------
        if (this->BBoxOfDeformers.size() == 0) {  // fill it
            int lent = this->inflDagPaths.length();
            if (verbose) MGlobal::displayInfo("\nfilling BBoxOfDeformers \n");
            MPoint zero(0, 0, 0);
            MVector up(0, 1, 0);
            MVector right(1, 0, 0);
            MVector side(0, 0, 1);
            for (unsigned int i = 0; i < lent; i++) {  // for all deformers
                MDagPath path = this->inflDagPaths[i];
                drawingDeformers newDef;

                MMatrix mat = path.inclusiveMatrix();
                MMatrix matEx = path.exclusiveMatrix();
                MBoundingBox bbox;
                up = MVector(mat[1]);
                right = MVector(mat[0]);
                side = MVector(mat[2]);

                unsigned int nbShapes;
                path.numberOfShapesDirectlyBelow(nbShapes);
                if (nbShapes != 0) {
                    path.extendToShapeDirectlyBelow(0);
                    MFnDagNode dag(path);
                    bbox = dag.boundingBox();

                    MPoint center = bbox.center() * matEx;
                    newDef.center = center;
                    newDef.width = 0.5 * bbox.width() * right.length();
                    newDef.height = 0.5 * bbox.height() * up.length();
                    newDef.depth = 0.5 * bbox.depth() * side.length();
                    newDef.mat = mat;
                    newDef.minPt = bbox.min();
                    newDef.maxPt = bbox.max();
                } else {
                    MPoint influencePosi = zero * mat;
                    MFnDagNode dag(path);
                    bbox = dag.boundingBox();

                    newDef.center = influencePosi;  // bbox.center()* matEx; //
                    newDef.width = 0.5 * right.length();
                    newDef.height = 0.5 * up.length();
                    newDef.depth = 0.5 * side.length();
                    newDef.mat = matEx;

                    // divide by 2
                    MPoint minPt = bbox.min();
                    MPoint maxPt = bbox.max();
                    newDef.minPt = minPt + 0.25 * (maxPt - minPt);
                    newDef.maxPt = maxPt + 0.25 * (minPt - maxPt);
                }
                newDef.up = up;
                newDef.right = right;
                // newDef.bbox = bbox;

                BBoxOfDeformers.push_back(newDef);
            }
        }  // end fill it

        // end fill jnts boundingBox
        // --------------------------------------------------------------------
        // -----------------------------------------------------------------------------------------------
        biggestInfluence = getClosestInfluenceToCursor(screenX, screenY);
    }

    int faceHit;
    successFullHit = computeHit(screenX, screenY, true, faceHit, this->centerOfBrush);

    if (!successFullHit && !this->refreshDone) {  // try to re-get accelParams in case no hit
        refreshPointsNormals();
        successFullHit = computeHit(screenX, screenY, true, faceHit, this->centerOfBrush);
        this->refreshDone = true;
        // MGlobal::displayInfo("refreshing ");
    }

    if (!successFullHit && !displayPickInfluence) return MStatus::kNotFound;

    drawManager.beginDrawable();
    drawManager.setColor(MColor(0.0, 0.0, 1.0));
    drawManager.setLineWidth((float)lineWidthVal);
    MColor biggestInfluenceColor(1.0, 0.0, 0.0);

    if (this->pickMaxInfluenceVal || this->pickInfluenceVal) {
        if (this->pickInfluenceVal) {
            // ---------------------------------------------------------------------------------------
            // start reDraw jnts
            // --------------------------------------------------------------------
            int lent = this->inflDagPaths.length();
            drawManager.setColor(MColor(0.0, 0.0, 0.0));
            for (unsigned int i = 0; i < lent; i++) {
                bool fillDraw = i == biggestInfluence;
                if (i == biggestInfluence) {
                    drawManager.setColor(biggestInfluenceColor);
                    // drawManager.setLineWidth((float)lineWidthVal+1.0);
                } else {
                    if (i == this->influenceIndex) fillDraw = true;
                    drawManager.setColor(jointsColors[i]);
                    // drawManager.setLineWidth((float)lineWidthVal);
                }
                drawingDeformers bbosDfm = BBoxOfDeformers[i];
                drawManager.box(bbosDfm.center, bbosDfm.up, bbosDfm.right, bbosDfm.width,
                                bbosDfm.height, bbosDfm.depth, fillDraw);
            }
            // end reDraw jnts
            // ----------------------------------------------------------------------
            // ---------------------------------------------------------------------------------------
            // drawManager.endDrawable();
            // drawManager.beginDrawable();
        }
        // drawManager.setDepthPriority(6); // MRenderItem::sActivePointDepthPriority

        drawManager.setFontSize(14);
        drawManager.setFontName(MString("MS Shell Dlg 2"));
        drawManager.setFontWeight(1);
        MColor Yellow(1.0, 1.0, 0.0);

        if (this->pickMaxInfluenceVal) {
            Yellow = MColor(1.0, 0.5, 0.0);
            if (successFullHit)
                biggestInfluence = getHighestInfluence(faceHit, this->centerOfBrush);
            else
                biggestInfluence = -1;
        }
        MString text("--");
        drawManager.setColor(MColor(0.0, 0.0, 0.0));

        int backgroundSize[] = {60, 20};
        if (biggestInfluence != -1) {
            text = this->inflNames[biggestInfluence];
            backgroundSize[0] = this->inflNamePixelSize[2 * biggestInfluence];
            backgroundSize[1] = this->inflNamePixelSize[2 * biggestInfluence + 1];
            worldPoint = worldPoint + .1 * worldVector.normal();
            drawManager.text(worldPoint, text, MHWRender::MUIDrawManager::TextAlignment::kCenter,
                             backgroundSize, &Yellow);
            // drawing full front camera
        } else {
            drawManager.text2d(MPoint(this->screenX, this->screenY, 0.0), text,
                               MHWRender::MUIDrawManager::TextAlignment::kCenter, backgroundSize,
                               &Yellow);
            // drawing behind bboxes
        }
    } else {
        drawManager.circle(this->centerOfBrush, normalVector, sizeVal);
        MVector worldVector;
        view.viewToWorld(this->screenX, this->screenY, worldPoint, worldVector);

        // drawTheMesh(drawManager, worldVector);
    }

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
    if (!this->useColorSetsWhilePainting) {
        drawManager.beginDrawable();
        drawMeshWhileDrag(drawManager);
        drawManager.endDrawable();
    }

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

MStatus SkinBrushContext::drawMeshWhileDrag(MHWRender::MUIDrawManager &drawManager) {
    int nbVtx = this->verticesPainted.size();

    float transparency = 1.0;

    MPointArray points(nbVtx);
    MVectorArray normals(nbVtx);
    MColor theCol(1, 1, 1), white(1, 1, 1, 1), black(0, 0, 0, 1);
    MIntArray editVertsIndices;

    MColorArray colors, colorsSolo;
    MColorArray pointsColors(nbVtx, theCol);

    MUintArray indices, indicesEdges;  // (nbVtx);
    MColorArray darkEdges;             // (nbVtx, MColor(0.5, 0.5, 0.5));

    MColor newCol, col;
    unsigned int i = 0;
    // for (const auto& ptIndex : this->verticesPainted) {
    // std::vector <int> facesSet;
    std::vector<int> verticesSet;
    std::unordered_set<int> fatFaces_set;
    std::unordered_set<int> fatEdges_set;

    MColor baseColor;
    float h, s, v;

    // get baseColor ----------------------------------
    // 0 Add - 1 Remove - 2 AddPercent - 3 Absolute - 4 Smooth - 5 Sharpen - 6 LockVertices - 7
    // UnLockVertices
    int theCommandIndex = this->commandIndex;
    if (this->commandIndex == 0 && this->modifierNoneShiftControl == 1)
        theCommandIndex = 1;  // remove
    if (this->commandIndex == 6 && this->modifierNoneShiftControl == 1)
        theCommandIndex = 7;                                       // unlockVertices
    if (this->modifierNoneShiftControl == 2) theCommandIndex = 4;  // smooth always
    if (drawTransparency || drawPoints) {
        if (theCommandIndex > 6)
            baseColor = white;
        else if (theCommandIndex == 6)
            baseColor = this->lockVertColor;
        else if (theCommandIndex > 3)
            baseColor = white;
        else if (commandIndex == 1)
            baseColor = black;
        else
            baseColor = this->jointsColors[this->influenceIndex];

        baseColor.get(MColor::kHSV, h, s, v);
        baseColor.set(MColor::kHSV, h, pow(s, 0.8), pow(v, 0.15));
    }

    for (const auto &pt : this->skinValuesToSet) {
        int ptIndex = pt.first;
        double weight = pt.second;
        if (theCommandIndex >= 6) weight = 1.0;  // no transparency on lock / unlocks verts
        MPoint posPoint(this->mayaRawPoints[ptIndex * 3], this->mayaRawPoints[ptIndex * 3 + 1],
                        this->mayaRawPoints[ptIndex * 3 + 2]);
        // points.append(posPoint);
        points.set(posPoint, i);
        normals.set(verticesNormals[ptIndex], i);

        // now for colors -------------------------------------------------
        if (drawTriangles) {
            /*
            if (useTransparency) {
                    colors.append(baseColor);
                    colorsSolo.append(baseColor);
                    col = colors[i];
                    transparency = (float)weight;
            }
            else {
                    setColor(ptIndex, weight, editVertsIndices, colors, colorsSolo);
                    if (this->soloColorVal)col = colorsSolo[i];
                    else col = colors[i];
            }
            */
            if (drawTransparency)
                transparency = (float)weight;
            else
                transparency = 1.0;

            setColor(ptIndex, weight, editVertsIndices, colors, colorsSolo);
            if (theCommandIndex != 6) {  // not painting locks
                if (this->soloColorVal) {
                    col = colorsSolo[i];
                } else {
                    col = colors[i];
                }
                // now gamma -----------------------------------------------------------------
                col.get(MColor::kHSV, h, s, v);
                // col.set(MColor::kHSV, h, pow(s, this->interactiveValue), pow(v,
                // this->interactiveValue1), (float)weight);
                col.set(MColor::kHSV, h, pow(s, 0.8), pow(v, 0.15), transparency);
                // MColor newCol = MColor(pow(col.r, gammaValue ), pow(col.g, gammaValue ),
                // pow(col.b, gammaValue )); MColor newCol = MColor(log2(col.r + 1), log2(col.g + 1),
                // log2(col.b + 1));
                if (this->soloColorVal) {
                    colorsSolo[i] = col;
                } else {
                    colors[i] = col;
                }
            }
        }
        if (drawPoints) {
            if (this->soloColorVal)
                newCol = weight * baseColor + (1.0 - weight) * this->soloCurrentColors[ptIndex];
            else
                newCol = weight * baseColor + (1.0 - weight) * this->multiCurrentColors[ptIndex];
            pointsColors[i] = newCol;
        }
        if (drawEdges) {
            darkEdges.append(MColor((float)0.5, (float)0.5, (float)0.5, transparency));
        }
        i++;

        // now for the indices of the triangles ------------------------------------
        if (drawTriangles || drawEdges) {
            verticesSet.push_back(ptIndex);
            if (drawTriangles) {
                for (int f : this->perVertexFaces[ptIndex]) fatFaces_set.insert(f);
            }
            if (drawEdges) {
                for (int e : this->perVertexEdges[ptIndex]) fatEdges_set.insert(e);
            }
        }
    }
    // now gamma the colors -------------------------------------
    /*
    float gammaValue = 1.0/ (float)1.8;//(float)2.2;
    gammaValue = 1.0 / (float)this->interactiveValue;//(float)2.2;
    MColor col;

    for (int i = 0; i < nbVtx; i++) {
            if (this->soloColorVal)col = colorsSolo[i];
            else col = colors[i];
            float h, s, v;
            col.get(MColor::kHSV, h, s, v);
            col.set(MColor::kHSV, h, pow(s, this->interactiveValue), pow(v,
    this->interactiveValue1));
            //MColor newCol = MColor(pow(col.r, gammaValue ), pow(col.g, gammaValue ), pow(col.b,
    gammaValue ));
            //MColor newCol = MColor(log2(col.r + 1), log2(col.g + 1), log2(col.b + 1));
            if (this->soloColorVal)colorsSolo[i] = col;
            else colors[i] = col;
    }
    */
    // make it unique .... hopefully
    if (drawTriangles) {
        std::vector<int> facesSet(fatFaces_set.begin(), fatFaces_set.end());
        // now make the triangles -------------------------------
        for (int f : facesSet) {
            for (auto tri : this->perFaceTriangleVertices[f]) {
                auto it0 = std::find(verticesSet.begin(), verticesSet.end(), tri[0]);
                if (it0 == verticesSet.end()) continue;
                auto it1 = std::find(verticesSet.begin(), verticesSet.end(), tri[1]);
                if (it1 == verticesSet.end()) continue;
                auto it2 = std::find(verticesSet.begin(), verticesSet.end(), tri[2]);
                if (it2 == verticesSet.end()) continue;
                indices.append(it0 - verticesSet.begin());
                indices.append(it1 - verticesSet.begin());
                indices.append(it2 - verticesSet.begin());
            }
        }
        auto style = MHWRender::MUIDrawManager::kFlat;
        // if (this->interactiveValue == 1.0) style = MHWRender::MUIDrawManager::kStippled;
        // if (this->interactiveValue == 2.0) style = MHWRender::MUIDrawManager::kShaded;
        drawManager.setPaintStyle(style);  // kFlat // kShaded //kStippled

        if (this->soloColorVal) {
            drawManager.mesh(MHWRender::MUIDrawManager::kTriangles, points, &normals, &colorsSolo,
                             &indices);
        } else {
            drawManager.mesh(MHWRender::MUIDrawManager::kTriangles, points, &normals, &colors,
                             &indices);
        }
    }
    if (drawEdges) {
        std::vector<int> edgesSet(fatEdges_set.begin(), fatEdges_set.end());
        // now make the edges -------------------------------
        for (int e : edgesSet) {
            auto pairEdges = this->perEdgeVertices[e];
            auto it0 = std::find(verticesSet.begin(), verticesSet.end(), pairEdges.first);
            if (it0 == verticesSet.end()) continue;
            auto it1 = std::find(verticesSet.begin(), verticesSet.end(), pairEdges.second);
            if (it1 == verticesSet.end()) continue;
            indicesEdges.append(it0 - verticesSet.begin());
            indicesEdges.append(it1 - verticesSet.begin());
        }
        drawManager.setDepthPriority(2);
        drawManager.mesh(MHWRender::MUIDrawManager::kLines, points, &normals, &darkEdges,
                         &indicesEdges);
    }
    if (drawPoints) {
        drawManager.setPointSize(4);
        drawManager.mesh(MHWRender::MUIDrawManager::kPoints, points, NULL, &pointsColors);
    }
    return MStatus::kSuccess;
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

#pragma omp parallel for
        for (int vertexInd = 0; vertexInd < this->numVertices; vertexInd++) {
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
        this->BBoxOfDeformers.clear();

        if (biggestInfluence != this->influenceIndex && biggestInfluence != -1) {
            setInfluenceIndex(biggestInfluence, true);  // true for select in UI
        }
        return MStatus::kNotFound;
    }

    // store for undo purposes --------------
    // only if painting not after
    if (!this->postSetting) {
        this->fullUndoSkinWeightList = MDoubleArray(this->skinWeightList);
    }
    // update values ---------------
    refreshPointsNormals();

    // first reset attribute to paint values off if we're doing that ------------------------
    paintArrayValues.copy(MDoubleArray(numVertices, 0.0));
    this->skinValuesToSet.clear();
    this->verticesPainted.clear();

    // reset values ---------------------------------
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
    // no need apparently, it gets done in the drag
    // maya2019RefreshColors();
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
            // setOfVertsGrow = setOfVertsGrow + this->perVertexVerticesSet[vertexIndex];
            setOfVertsGrow = setOfVertsGrow + getSurroundingVerticesPerVert(vertexIndex);
        }
        // get the vertices that are grown -----------------------------------------------------
        std::vector<int> verticesontheborder = setOfVertsGrow - vertsVisited;
        std::vector<int> foundGrowVertsWithinDistance;
        // for all vertices grown ------------------------------
        for (int vertexBorder : verticesontheborder) {
            // intersection of the connectedVerts of the grow vert and the already visisted Vertices
            // std::vector<int> intersection = this->perVertexVerticesSet[vertexBorder] &
            // vertsWithinDistance;
            std::vector<int> intersection =
                getSurroundingVerticesPerVert(vertexBorder) & vertsWithinDistance;

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
            // setOfVertsGrow = setOfVertsGrow + this->perVertexVerticesSet[vertexIndex];
            setOfVertsGrow = setOfVertsGrow + getSurroundingVerticesPerVert(vertexIndex);
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
        std::unordered_map<int, float> dicVertsDistSummedUp;
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
        MVector deltaPos(currentPos - startPos);

        // Switch if the size should get adjusted or the strength based
        // on the drag direction. A drag along the x axis defines size
        // and a drag along the y axis defines strength.
        // InitAdjust makes sure that direction gets set on the first
        // drag event and gets reset the next time a mouse button is
        // pressed.
        if (!initAdjust) {
            if (deltaPos.length() < 6)
                return status;  // only if we move at least 6 pixels do we know the direction to
                                // pick !
            sizeAdjust = (abs(deltaPos.x) > abs(deltaPos.y));
            initAdjust = true;
        }
        // Define the settings for either setting the brush size or the
        // brush strength.
        MString message = "Brush Size";
        MString slider = "Size";
        double dragDistance = deltaPos.x;
        double min = 0.001;
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
        short offsetX = startScreenX - viewCenterX;
        short offsetY = startScreenY - viewCenterY - 50;

        /*
char info[32];

#ifdef _WIN64
        if (event.isModifierShift()) sprintf_s(info, "%s: %.3f", message.asChar(), adjustValue);
        else sprintf_s(info, "%s: %.2f", message.asChar(), adjustValue);
#else
        if (event.isModifierShift()) sprintf(info, "%s: %.3f", message.asChar(), adjustValue);
        else sprintf(info, "%s: %.2f", message.asChar(), adjustValue);
#endif

// Calculate the position for the value display. Since the
// heads-up message starts at the center of the viewport an
// offset needs to get calculated based on the view size and the
// initial adjust position of the cursor.
MString cmd = "headsUpMessage -horizontalOffset ";
cmd += offsetX;
cmd += " -verticalOffset ";
cmd += offsetY;
cmd += " -time 0.1 ";
cmd += "\"" + MString(info) + "\"";
MGlobal::executeCommand(cmd);
        */
        int precision = 2;
        if (event.isModifierShift()) precision = 3;
        MString headsUpCommand = MString("headsUpMessage(") + offsetX + MString(", ") + offsetY +
                                 MString(", '") + message + MString("', ") + adjustValue +
                                 MString(", ") + precision + MString(")\n");

        MGlobal::executePythonCommand(headsUpCommand);
        // Also, adjust the slider in the tool settings window if it's
        // currently open.
        MString UIupdate = MString("updateDisplayStrengthOrSize(") + sizeAdjust + MString(", ") +
                           adjustValue + MString(")\n");
        // MGlobal::displayInfo(UIupdate);
        MGlobal::executePythonCommand(UIupdate);
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
        MColorArray multiEditColors, soloEditColors;
        MIntArray editVertsIndices((int)this->verticesPainted.size(), 0);
        MIntArray undoLocks, redoLocks;

        MDoubleArray prevWeights((int)this->verticesPainted.size() * this->nbJoints, 0);
        int i = 0;
        for (const auto &theVert : this->verticesPainted) {
            editVertsIndices[i] = theVert;
            // if (theVert == 241)MGlobal::displayInfo(MString("241  in this->verticesPainted "));
            i++;
        }
        int theCommandIndex = this->commandIndex;
        if (this->commandIndex == 6 && this->modifierNoneShiftControl == 1)
            theCommandIndex = 7;                                       // unlockVertices
        if (this->modifierNoneShiftControl == 2) theCommandIndex = 4;  // smooth always
        // MGlobal::displayInfo(MString("a| Vtx 241 ") + this->lockVertices[241]);
        if (theCommandIndex >= 6) {
            undoLocks.copy(this->lockVertices);
            bool addLocks = theCommandIndex == 6;
            // if (this->mirrorIsActive) editLocks(this->skinObj, editAndMirrorVerts, addLocks,
            // this->lockVertices);
            editLocks(this->skinObj, editVertsIndices, addLocks, this->lockVertices);
            // MGlobal::displayInfo(MString("b| Vtx 241 ") + this->lockVertices[241]);
            // MGlobal::displayInfo("editing locks");
            redoLocks.copy(this->lockVertices);
        } else {
            if (this->skinValuesToSet.size() > 0)
                applyCommand(this->influenceIndex, this->skinValuesToSet, !this->mirrorIsActive);

            if (!this->postSetting) {  // only store if not constant setting
                int i = 0;
                for (const auto &theVert : this->verticesPainted) {
                    for (int j = 0; j < this->nbJoints; ++j) {
                        prevWeights[i * this->nbJoints + j] =
                            this->fullUndoSkinWeightList[theVert * this->nbJoints + j];
                    }
                    i++;
                }
            }
        }
        // MGlobal::displayInfo(MString("c| Vtx 241 ") + this->lockVertices[241]);
        refreshColors(editVertsIndices, multiEditColors, soloEditColors);
        meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet);
        meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet);

        /*
        if (theCommandIndex >= 6) {
                toggleColorState = true;
                maya2019RefreshColors();
                view.refresh(true);
        }
        */
        meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet2);
        meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet2);

        if (theCommandIndex >= 6) {  // if locking or unlocking
            // without that it doesn't refresh because mesh is not invalidated, meaning the
            // skinCluster hasn't changed
            meshFn.updateSurface();
        }
        /*
        if (theCommandIndex >= 6) {
                maya2019RefreshColors();
                view.refresh(true);
        }
        */

        this->skinValuesToSet.clear();
        this->previousPaint.clear();

        cmd = (skinBrushTool *)newToolCommand();

        cmd->setColor(colorVal);
        cmd->setCurve(curveVal);
        cmd->setDrawBrush(drawBrushVal);
        cmd->setDrawRange(drawRangeVal);
        cmd->setPythonImportPath(moduleImportString);
        cmd->setEnterToolCommand(enterToolCommandVal);
        cmd->setExitToolCommand(exitToolCommandVal);
        cmd->setFractionOversampling(fractionOversamplingVal);
        cmd->setIgnoreLock(ignoreLockVal);
        cmd->setLineWidth(lineWidthVal);
        cmd->setMessage(messageVal);
        cmd->setOversampling(oversamplingVal);
        cmd->setRange(rangeVal);
        cmd->setSize(sizeVal);
        cmd->setStrength(strengthVal);

        cmd->setSmoothStrength(smoothStrengthVal);
        cmd->setUndersampling(undersamplingVal);
        cmd->setVolume(volumeVal);
        cmd->setCoverage(coverageVal);

        cmd->setCommandIndex(theCommandIndex);

        cmd->setUnoLocks(undoLocks);
        cmd->setRedoLocks(redoLocks);

        cmd->setMesh(meshDag);
        cmd->setSkinCluster(skinObj);
        cmd->setInfluenceIndices(influenceIndices);
        cmd->setVertexComponents(smoothedCompObj);

        cmd->setUnoVertices(editVertsIndices);
        if (!this->postSetting) {
            cmd->setWeights(prevWeights);
        } else {
            cmd->setWeights(this->skinWeightsForUndo);
        }

        cmd->setNormalize(normalize);
        cmd->setSelection(prevSelection, prevHilite);
        // cmd->redoIt();
        // Regular context implementations usually call
        // (MPxToolCommand)::redoIt at this point but in this case it
        // is not necessary since the the smoothing already has been
        // performed. There is no need to apply the values twice.
        cmd->finalize();
    }
    maya2019RefreshColors();
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
        theCommandIndex = 7;                                       // unlockVertices
    if (this->modifierNoneShiftControl == 2) theCommandIndex = 4;  // smooth always

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
                    // std::vector <int> vertsAround = this->perVertexVerticesSet[theVert];
                    std::vector<int> vertsAround = getSurroundingVerticesPerVert(theVert);

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
            this->skinWeightsForUndo.clear();
            skinFn.setWeights(meshDag, weightsObj, influenceIndices, theWeights, normalize,
                              &this->skinWeightsForUndo);

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
                soloColor = getASoloColor(val);
            }
        }
        this->multiCurrentColors[theVert] = multiColor;
        this->soloCurrentColors[theVert] = soloColor;
        if (isVtxLocked) {
            multiEditColors[i] = this->lockVertColor;
            soloEditColors[i] = this->lockVertColor;
        } else {
            multiEditColors[i] = multiColor;
            soloEditColors[i] = soloColor;
        }
        /*
        if (theVert == 241) {
                MGlobal::displayInfo(MString("refreshColors| Vtx 241 ") + isVtxLocked);
                MGlobal::displayInfo(MString("r ") + multiEditColors[i].r + MString(" g ") +
        multiEditColors[i].g + MString("b ") + multiEditColors[i].b );
        }
        */
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
    // -----------------------------------------------------------------
    // mesh
    // -----------------------------------------------------------------

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
    getConnectedVerticesSecond();
    getConnectedVerticesThird();
    // getConnectedVerticesTyler();
    getFromMeshNormals();
    getConnectedVerticesFlatten();

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
    this->inflDagPaths.clear();
    influenceIndices = getInfluenceIndices();  // this->skinObj, this->inflDagPaths);

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
    MStatus status;

    // MIntArray vertexCount, vertexList;
    status = meshFn.getVertices(VertexCountPerPolygon, fullVertexList);
    this->fullVertexListLength = fullVertexList.length();

    MIntArray triangleCounts, triangleVertices;  // get the triangles to draw the mesh
    status = meshFn.getTriangles(triangleCounts, triangleVertices);

    // First set array sizes ----------------------------------------------
    this->perFaceVertices.clear();
    this->perVertexFaces.clear();
    this->perFaceTriangleVertices.clear();

    this->perFaceVertices.resize(this->numFaces);
    this->perVertexFaces.resize(this->numVertices);
    this->perFaceTriangleVertices.resize(this->numFaces);

    // end set array sizes ----------------------------------------------
    // First run --------------------------------------------------------
    /*
    for (unsigned int faceId = 0, iter = 0; faceId < VertexCountPerPolygon.length(); ++faceId) {
            perFaceVertices[faceId].clear();
            for (int i = 0; i < VertexCountPerPolygon[faceId]; ++i, ++iter) {
                    int indVertex = fullVertexList[iter];
                    perFaceVertices[faceId].append(indVertex);
                    perVertexFaces[indVertex].append(faceId);
            }
    }
    */
    for (unsigned int faceId = 0, iter = 0, triIter = 0; faceId < VertexCountPerPolygon.length();
         ++faceId) {
        perFaceVertices[faceId].clear();
        for (int i = 0; i < VertexCountPerPolygon[faceId]; ++i, ++iter) {
            int indVertex = fullVertexList[iter];
            perFaceVertices[faceId].append(indVertex);
            perVertexFaces[indVertex].append(faceId);
        }
        perFaceTriangleVertices[faceId].resize(triangleCounts[faceId]);
        for (int triId = 0; triId < triangleCounts[faceId]; ++triId) {
            perFaceTriangleVertices[faceId][triId].setLength(3);
            perFaceTriangleVertices[faceId][triId][0] = triangleVertices[triIter++];
            perFaceTriangleVertices[faceId][triId][1] = triangleVertices[triIter++];
            perFaceTriangleVertices[faceId][triId][2] = triangleVertices[triIter++];
        }
    }
    // get the edgesIndices to draw the wireframe --------------------
    MItMeshEdge edgeIter(meshDag);

    this->perEdgeVertices.clear();
    this->perVertexEdges.clear();
    this->perEdgeVertices.resize(edgeIter.count());
    this->perVertexEdges.resize(this->numVertices);

    unsigned int i = 0;
    for (; !edgeIter.isDone(); edgeIter.next()) {
        int pt0Index = edgeIter.index(0);
        int pt1Index = edgeIter.index(1);
        this->perVertexEdges[pt0Index].append(i);
        this->perVertexEdges[pt1Index].append(i);
        this->perEdgeVertices[i++] = std::make_pair(pt0Index, pt1Index);
        /*
        if (pt0Index < numVertices || pt1Index < numVertices) {
                this->edgeVerticesIndices.append(pt0Index);
                this->edgeVerticesIndices.append(pt1Index);
        }
        */
    }
}
void SkinBrushContext::getConnectedVerticesSecond() {
    // Second run --------------------------------------------------------
    this->perFaceVerticesSet.clear();
    perFaceVerticesSet.resize(this->numFaces);
    for (int faceTmp = 0; faceTmp < numFaces; ++faceTmp) {
        std::vector<int> tmpSet;
        MIntArray surroundingVertices = perFaceVertices[faceTmp];
        tmpSet.resize(surroundingVertices.length());
        for (unsigned int itVtx = 0; itVtx < surroundingVertices.length(); itVtx++)
            tmpSet[itVtx] = surroundingVertices[itVtx];
        std::sort(tmpSet.begin(), tmpSet.end());

        perFaceVerticesSet[faceTmp] = tmpSet;
    }
}
void SkinBrushContext::getConnectedVerticesThird() {
    // fill the std_array connectedSetVertices ------------------------
    this->perVertexVerticesSet.clear();
    this->perVertexVerticesSet.resize(this->numVertices);

#pragma omp parallel for
    for (int vtxTmp = 0; vtxTmp < this->numVertices; ++vtxTmp) {
        std::vector<int> connVetsSet2;
        MIntArray connectFaces = perVertexFaces[vtxTmp];
        for (int fct = 0; fct < connectFaces.length(); ++fct) {
            auto surroundingVertices = perFaceVerticesSet[connectFaces[fct]];
            std::vector<int> connVetsSetTMP;

            std::set_union(connVetsSet2.begin(), connVetsSet2.end(), surroundingVertices.begin(),
                           surroundingVertices.end(), std::back_inserter(connVetsSetTMP));
            connVetsSet2 = connVetsSetTMP;
            // connVetsSet2 = connVetsSet2 + surroundingVertices;
        }
        auto it = std::find(connVetsSet2.begin(), connVetsSet2.end(), vtxTmp);
        connVetsSet2.erase(it);
        // connVetsSet2 = connVetsSet2 - std::vector<int>(1, vtxTmp);
        this->perVertexVerticesSet[vtxTmp] = connVetsSet2;
    }
}

void SkinBrushContext::getConnectedVerticesTyler() {
    std::vector<std::unordered_set<int>> faceNeighbors, edgeNeighbors;
    std::vector<int> fCounts, fIndices, eCounts, eIndices;

    getRawNeighbors(this->VertexCountPerPolygon, this->fullVertexList, this->numVertices,
                    faceNeighbors, edgeNeighbors);
    convertToCountIndex(faceNeighbors, fCounts, fIndices);
    convertToCountIndex(edgeNeighbors, eCounts, eIndices);
}

void SkinBrushContext::getFromMeshNormals() {
    this->verticesNormals.clear();
    this->verticesNormals.setLength(this->numVertices);
    // fill the normals ----------------------------------------------------
    this->normalsIds.clear();
    this->normalsIds.resize(this->numFaces);
    MIntArray normalCounts, normals;
    this->meshFn.getNormalIds(normalCounts, normals);

    int startIndex = 0;
#pragma omp parallel for
    for (int faceTmp = 0; faceTmp < normalCounts.length(); ++faceTmp) {
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

    // get vertexNormalIndex --------------------------------------------------
    this->verticesNormalsIndices.clear();
    this->verticesNormalsIndices.setLength(numVertices);
#pragma omp parallel for
    for (int vertexInd = 0; vertexInd < this->numVertices; vertexInd++) {
        int indFace = this->perVertexFaces[vertexInd][0];  // get the first face
        MIntArray surroundingVertices = this->perFaceVertices[indFace];
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
}
void SkinBrushContext::getConnectedVerticesFlatten() {
    perVertexVerticesSetFLAT.clear();
    perVertexVerticesSetINDEX.clear();
    int sum = 0;
    for (auto surroundingVtices : this->perVertexVerticesSet) {
        perVertexVerticesSetINDEX.push_back(sum);
        for (int vtx : surroundingVtices) {
            perVertexVerticesSetFLAT.push_back(vtx);
            sum++;
        }
    }
    perVertexVerticesSetINDEX.push_back(sum);  // one extra for easy access
    //------------------------------------------------------------------------
    perFaceVerticesSetFLAT.clear();
    perFaceVerticesSetINDEX.clear();
    sum = 0;
    for (auto surroundingVtices : this->perFaceVerticesSet) {
        perFaceVerticesSetINDEX.push_back(sum);
        for (int vtx : surroundingVtices) {
            perFaceVerticesSetFLAT.push_back(vtx);
            sum++;
        }
    }
    perFaceVerticesSetINDEX.push_back(sum);  // one extra for easy access
}
std::vector<int> SkinBrushContext::getSurroundingVerticesPerVert(int vertexIndex) {
    auto first = perVertexVerticesSetFLAT.begin() + perVertexVerticesSetINDEX[vertexIndex];
    auto last = perVertexVerticesSetFLAT.begin() + perVertexVerticesSetINDEX[vertexIndex + (int)1];
    std::vector<int> newVec(first, last);
    return newVec;
};

std::vector<int> SkinBrushContext::getSurroundingVerticesPerFace(int vertexIndex) {
    auto first = perFaceVerticesSetFLAT.begin() + perFaceVerticesSetINDEX[vertexIndex];
    auto last = perFaceVerticesSetFLAT.begin() + perFaceVerticesSetINDEX[vertexIndex + (int)1];
    std::vector<int> newVec(first, last);
    return newVec;
};

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
    }

    // Make sure that the mesh is bound to a skin cluster.
    if (skinClusterObj.isNull()) {
        MGlobal::displayWarning("The selected mesh is not bound to a skin cluster.");
        return MStatus::kNotFound;
    }

    return status;
}

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

MStatus SkinBrushContext::displayWeightValue(int vertexIndex, bool displayZero) {
    MString toDisplay = MString("weigth of vtx (") + vertexIndex + MString(") : ");
    for (unsigned int indexInfluence = 0; indexInfluence < this->nbJoints;
         indexInfluence++) {  // for each joint
        double theWeight = this->skinWeightList[vertexIndex * this->nbJoints + indexInfluence];
        if (theWeight == 0 && !displayZero) continue;
        toDisplay += MString("[") + indexInfluence + MString(": ") + theWeight + MString("] ");
    }
    MGlobal::displayInfo(toDisplay);
    return MS::kSuccess;
}

MStatus SkinBrushContext::fillArrayValuesDEP(MObject skinCluster, bool doColors) {
    MStatus status = MS::kSuccess;
    if (verbose) MGlobal::displayInfo(" FILLED ARRAY VALUES ");

    MFnDependencyNode skinClusterDep(skinCluster);

    MPlug weight_list_plug = skinClusterDep.findPlug("weightList");
    MPlug matrix_plug = skinClusterDep.findPlug("matrix");
    // MGlobal::displayInfo(weight_list_plug.name());
    int nbElements = weight_list_plug.numElements();
    unsigned int infCount = matrix_plug.numElements();

    matrix_plug.getExistingArrayAttributeIndices(this->deformersIndices);

    this->nbJointsBig = this->deformersIndices[this->deformersIndices.length() - 1] +
                        1;  // matrix_plug.evaluateNumElements();
    if (verbose)
        MGlobal::displayInfo(MString(" nb jnts ") + this->nbJoints + MString(" nb jnts Big ") +
                             this->nbJointsBig + MString(" influenceIndices ") +
                             this->influenceIndices.length());
    this->nbJoints = infCount;

    // For the first component, the weights are ordered by influence object in the same order that
    // is returned by the MFnSkinCluster::influenceObjects method.
    // use influenceIndices
    skin_weights_.resize(nbElements);
    if (doColors) {
        this->multiCurrentColors.clear();
        this->multiCurrentColors.setLength(nbElements);
    }
    this->skinWeightList = MDoubleArray(nbElements * this->nbJoints, 0.0);

// MThreadUtils::syncNumOpenMPThreads();
// in option C/C++ turn omp on !!!!
#pragma omp parallel for  // collapse(2)
    for (int i = 0; i < nbElements; ++i) {
        // weightList[i]
        MPlug ith_weights_plug = weight_list_plug.elementByPhysicalIndex(i);
        int vertexIndex = ith_weights_plug.logicalIndex();

        // weightList[i].weight
        MPlug plug_weights = ith_weights_plug.child(0);  // access first compound child
        int nb_weights = plug_weights.numElements();

        MColor theColor(0, 0, 0, 1);
        for (int j = 0; j < nb_weights; j++) {  // for each joint
            MPlug weight_plug = plug_weights.elementByPhysicalIndex(j);
            // weightList[i].weight[j]
            int indexInfluence = weight_plug.logicalIndex();
            double theWeight = weight_plug.asDouble();
            // store in the correct Spot --
            indexInfluence = this->indicesForInfluenceObjects[indexInfluence];
            this->skinWeightList[vertexIndex * this->nbJoints + indexInfluence] = theWeight;
            if (doColors) {  // and not locked
                theColor += this->jointsColors[indexInfluence] * theWeight;
            }
        }
        if (doColors) {  // not store lock vert color
            this->multiCurrentColors[vertexIndex] = theColor;
        }
    }
    return status;
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
MIntArray SkinBrushContext::getInfluenceIndices() {
    unsigned int i;
    MFnSkinCluster skinFn(this->skinObj);

    //------------------------
    //------------------------
    /*
    MFnDependencyNode skinClusterDep(this->skinObj);
    MPlug matrix_plug = skinClusterDep.findPlug("matrix");

    int nbJoints = matrix_plug.numElements();
    for (int i = 0; i < nbJoints; ++i) {
            // matrix[i]
            MPlug matrixPlug = matrix_plug.elementByPhysicalIndex(i);
            int indexLogical = matrixPlug.logicalIndex();
            if (matrixPlug.isConnected()) {
                    MPlugArray connections;
                    matrixPlug.connectedTo(connections, true, false);
                    if (connections.length() > 0) {
                            MPlug theConn = connections[0];
                            MObject dfm = theConn.node();
                            MFnDependencyNode influenceFn (dfm);
                            MDagPath influencePath ;
                            influencePath.getAPathTo(dfm, influencePath);
                            int indexForInfluenceObject =
    skinFn.indexForInfluenceObject(influencePath); MGlobal::displayInfo(MString(" [") + i +
    MString(" || ") + indexLogical + MString(" || ") + indexForInfluenceObject + MString("] ") +
    influenceFn.name());
                    }
            }
    }
    */
    //------------------------
    //------------------------
    MIntArray influenceIndices;

    skinFn.influenceObjects(this->inflDagPaths);
    int lent = this->inflDagPaths.length();
    // first clear --------------------------
    this->inflNames.clear();
    this->inflNamePixelSize.clear();
    this->indicesForInfluenceObjects.clear();

    this->inflNames.setLength(lent);
    this->inflNamePixelSize.setLength(2 * lent);
    this->indicesForInfluenceObjects.setLength(lent);
    for (i = 0; i < lent; i++) {
        influenceIndices.append((int)i);
        MFnDependencyNode influenceFn(this->inflDagPaths[i].node());
        this->inflNames[i] = influenceFn.name();
        // get pixels size ----------
        MIntArray result;
        MString cmd2 = MString("fnFonts (\"") + this->inflNames[i] + MString("\")");
        MGlobal::executePythonCommand(cmd2, result);
        this->inflNamePixelSize[2 * i] = result[0];
        this->inflNamePixelSize[2 * i + 1] = result[1];
        int indexLogical = skinFn.indexForInfluenceObject(this->inflDagPaths[i]);
        this->indicesForInfluenceObjects[indexLogical] = i;
        // MGlobal::displayInfo(MString(" [") + i + MString(" || ") + indexLogical + MString("] ") +
        // this->inflNames[i] + MString(" "));
    }
    return influenceIndices;
}

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

bool SkinBrushContext::computeHit(short screenPixelX, short screenPixelY, bool getNormal,
                                  int &faceHit, MFloatPoint &hitPoint) {
    MStatus stat;

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
    // auto verticesSet = this->perFaceVerticesSet[faceHit];
    auto verticesSet = getSurroundingVerticesPerFace(faceHit);
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
    MColor soloColor, multColor;
    int theCommandIndex = this->commandIndex;
    if (this->commandIndex == 0 && this->modifierNoneShiftControl == 1)
        theCommandIndex = 1;  // remove
    if (this->commandIndex == 6 && this->modifierNoneShiftControl == 1)
        theCommandIndex = 7;                                       // unlockVertices
    if (this->modifierNoneShiftControl == 2) theCommandIndex = 4;  // smooth always

    if (theCommandIndex >= 6) {  // painting locks
        // if (!this->mirrorIsActive) {// we do the colors diferently if mirror is active
        if (theCommandIndex == 6) {  // lock verts if not already locked
            soloColor = this->lockVertColor;
            multColor = this->lockVertColor;
        } else {  // unlock verts
            multColor = this->multiCurrentColors[vertexIndex];
            soloColor = this->soloCurrentColors[vertexIndex];
        }
        this->intensityValues[vertexIndex] = 1;  // store to not repaint
    } else if (!this->lockVertices[vertexIndex]) {
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
        }
    }
    editVertsIndices.append(vertexIndex);
    multiEditColors.append(multColor);
    soloEditColors.append(soloColor);

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
    bool isCommandLock = this->commandIndex >= 6 && this->modifierNoneShiftControl != 2;
    // if (this->modifierNoneShiftControl == 2 && this->commandIndex >= 6) return status;
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
            if ((this->lockVertices[index] == 1 && !isCommandLock) ||
                this->intensityValues[index] == 1) {
                continue;
            }

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
            if (this->useColorSetsWhilePainting) {
                setColor(index, value, editVertsIndices, multiEditColors, soloEditColors);
            }
        }
        // store this paint ---------------
        this->previousPaint = dicVertsDist;

        if (this->useColorSetsWhilePainting) {
            // do actually set colors -----------------------------------
            if (this->soloColorVal == 0) {
                meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet);
                meshFn.setSomeColors(editVertsIndices, multiEditColors, &this->fullColorSet2);
            } else {
                meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet);
                meshFn.setSomeColors(editVertsIndices, soloEditColors, &this->soloColorSet2);
            }
        }
        // Now store the colors for an actual MUIDrawMAnager Draw ---------------------
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
    /*
    this->performRefreshViewPort++;
    if (this->performRefreshViewPort > this->maxRefreshValue) {
            maya2019RefreshColors(true);
            this->performRefreshViewPort = 0;
    }
    */
    if (this->useColorSetsWhilePainting) {
        if (this->commandIndex >= 6) {  // if locking or unlocking
            // without that it doesn't refresh because mesh is not invalidated, meaning the
            // skinCluster hasn't changed
            meshFn.updateSurface();
        }

        maya2019RefreshColors(true);
    }
    return status;
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
//      and ing the returned MStatus.
//
// Input Arguments:
//      event               The MEvent to .
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

void SkinBrushContext::setInViewMessage(bool display) {
    if (display && messageVal)
        MGlobal::executeCommand("brSkinBrushShowInViewMessage");
    else
        MGlobal::executeCommand("brSkinBrushHideInViewMessage");
}
