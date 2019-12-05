#include "functions.h"

#include <limits>

unsigned int getMIntArrayIndex(MIntArray &myArray, int searching) {
    unsigned int toReturn = -1;
    for (unsigned int element = 0; element < myArray.length(); ++element) {
        if (myArray[element] == searching) {
            toReturn = element;
            break;
        }
    }
    return toReturn;
}

void CVsAround(int storedU, int storedV, int numCVsInU, int numCVsInV, bool UIsPeriodic,
               bool VIsPeriodic, MIntArray &vertices) {
    int resCV;
    // plus U
    int UNext = storedU + 1;
    if (UNext < numCVsInU) {
        resCV = numCVsInV * UNext + storedV;
        if (getMIntArrayIndex(vertices, resCV) == -1) vertices.append(resCV);
    } else if (UIsPeriodic) {
        UNext -= numCVsInU;
        resCV = numCVsInV * UNext + storedV;
        if (getMIntArrayIndex(vertices, resCV) == -1) vertices.append(resCV);
    }
    // minus U
    int UPrev = storedU - 1;
    if (UPrev >= 0) {
        resCV = numCVsInV * UPrev + storedV;
        if (getMIntArrayIndex(vertices, resCV) == -1) vertices.append(resCV);
    } else if (UIsPeriodic) {
        UPrev += numCVsInU;
        resCV = numCVsInV * UPrev + storedV;
        if (getMIntArrayIndex(vertices, resCV) == -1) vertices.append(resCV);
    }
    // plus V
    int VNext = storedV + 1;
    if (VNext < numCVsInV) {
        resCV = numCVsInV * storedU + VNext;
        if (getMIntArrayIndex(vertices, resCV) == -1) vertices.append(resCV);
    } else if (VIsPeriodic) {
        VNext -= numCVsInV;
        resCV = numCVsInV * storedU + VNext;
        if (getMIntArrayIndex(vertices, resCV) == -1) vertices.append(resCV);
    }
    // minus V
    int VPrev = storedV - 1;
    if (VPrev >= 0) {
        resCV = numCVsInV * storedU + VPrev;
        if (getMIntArrayIndex(vertices, resCV) == -1) vertices.append(resCV);
    } else if (VIsPeriodic) {
        VPrev += numCVsInV;
        resCV = numCVsInV * storedU + VPrev;
        if (getMIntArrayIndex(vertices, resCV) == -1) vertices.append(resCV);
    }
    // vertInd = numCVsInV * indexU + indexV;
}

// from the mesh retrieves the skinCluster
MStatus findSkinCluster(MDagPath MeshPath, MObject &theSkinCluster, int indSkinCluster,
                        bool verbose) {
    if (verbose) MGlobal::displayInfo(MString(" ---- findSkinCluster ----"));
    MStatus stat;

    MFnDagNode dagNode(MeshPath);  // path to the visible mesh
    // MFnMesh meshFn(MeshPath, &stat);     // this is the visible mesh
    MObject inObj;
    MObject dataObj1;

    MObjectArray listSkinClusters;
    // the deformed mesh comes into the visible mesh
    // through its "inmesh" plug
    MPlug inMeshPlug;
    if (MeshPath.apiType() == MFn::kMesh)
        inMeshPlug = dagNode.findPlug("inMesh", &stat);
    else if (MeshPath.apiType() == MFn::kNurbsSurface)
        inMeshPlug = dagNode.findPlug("create", &stat);

    if (stat == MS::kSuccess && inMeshPlug.isConnected()) {
        // walk the tree of stuff upstream from this plug
        MItDependencyGraph dgIt(inMeshPlug, MFn::kInvalid, MItDependencyGraph::kUpstream,
                                MItDependencyGraph::kDepthFirst, MItDependencyGraph::kPlugLevel,
                                &stat);
        if (MS::kSuccess == stat) {
            dgIt.disablePruningOnFilter();
            int count = 0;

            for (; !dgIt.isDone(); dgIt.next()) {
                MObject thisNode = dgIt.thisNode();
                // go until we find a skinCluster
                if (thisNode.apiType() == MFn::kSkinClusterFilter) {
                    listSkinClusters.append(thisNode);
                    // return MS::kSuccess;
                }
            }
        }
        int listSkinClustersLength = listSkinClusters.length();
        if (verbose)
            MGlobal::displayInfo(MString("    nb skinClusters is ") + listSkinClustersLength);
        if (listSkinClustersLength > indSkinCluster) {
            theSkinCluster = listSkinClusters[indSkinCluster];

            MFnDependencyNode nodeFn(theSkinCluster);
            if (verbose)
                MGlobal::displayInfo(MString("    returned skinCluster: ") + nodeFn.name());

            return MS::kSuccess;
        }
        // std::cout << "skinCluster: " << returnedSkinCluster.name().asChar() << "\n";
    }
    return MS::kFailure;
}

MStatus findMesh(MObject &skinCluster, MDagPath &theMeshPath, bool verbose) {
    if (verbose) MGlobal::displayInfo(MString(" ---- findMesh ----"));
    MFnSkinCluster theSkinCluster(skinCluster);
    MObjectArray objectsDeformed;
    theSkinCluster.getOutputGeometry(objectsDeformed);
    int objectsDeformedCount = objectsDeformed.length();
    bool doContinue = false;
    if (objectsDeformedCount == 0) {
        int j = 0;
        // for (int j = 0; j < objectsDeformedCount; j++) {
        theMeshPath.getAPathTo(objectsDeformed[j]);
        if (verbose) {
            MFnDependencyNode deformedNameMesh(objectsDeformed[j]);
            MString deformedNameMeshSTR = deformedNameMesh.name();
            if (verbose) MGlobal::displayInfo("     -> DEFORMING : " + deformedNameMeshSTR + "\n");
        }
        //}
        return MS::kSuccess;
    }
    return MS::kFailure;
}

MStatus findOrigMesh(MObject &skinCluster, MObject &origMesh, bool verbose) {
    if (verbose) MGlobal::displayInfo(MString(" ---- find Orig Mesh ----"));
    MFnSkinCluster theSkinCluster(skinCluster);
    MObjectArray objectsDeformed;
    theSkinCluster.getInputGeometry(objectsDeformed);
    origMesh = objectsDeformed[0];
    if (verbose) {
        MFnDependencyNode deformedNameMesh(origMesh);
        MGlobal::displayInfo("     -> DEFORMING : " + deformedNameMesh.name() + "\n");
    }
    return MS::kSuccess;
}

MStatus getListColorsJoints(MObject &skinCluster, MColorArray &jointsColors, bool verbose) {
    MStatus stat;
    if (verbose)
        MGlobal::displayInfo(MString("---------------- [getListColorsJoints()]------------------"));

    /*
    MDagPathArray  listOfJoints;
    MFnSkinCluster theSkinCluster(skinCluster);
    theSkinCluster.influenceObjects(listOfJoints, &stat);
    int nbJoints = listOfJoints.length();

    // now get the colors per joint ----------------------------------------
    MStringArray allJointsNames;
    float color[3];
    MColorArray jointsColors(nbJoints);
    for (int i = 0; i < nbJoints; i++) {
            MFnDagNode jnt(listOfJoints[i]);
            //jnt = MFnDagNode (listOfJoints[iterator]);
            MPlug lockInfluenceWeightsPlug = jnt.findPlug("wireColorRGB");
            //MPlug lockInfluenceWeightsPlug(jnt, MFnDagNode::wireColorRGB);

            color[0] = lockInfluenceWeightsPlug.child(0).asFloat();
            color[1] = lockInfluenceWeightsPlug.child(1).asFloat();
            color[2] = lockInfluenceWeightsPlug.child(2).asFloat();

            jointsColors[i] = MColor(color);
            //MString jointName = jnt.name();
    }
    */
    if (verbose) {
        MDagPathArray listOfJoints;
        MFnSkinCluster theSkinCluster(skinCluster);
        theSkinCluster.influenceObjects(listOfJoints, &stat);
        int nbJoints = listOfJoints.length();
        MStringArray allJointsNames;
        MGlobal::displayInfo(MString(" nbJoints from skinCluster ") + nbJoints);
        for (int i = 0; i < nbJoints; i++) {
            MFnDagNode jnt(listOfJoints[i]);
            MString jointName = jnt.name();
            MGlobal::displayInfo(jointName + " " + i);
        }
    }
    //----------------------------------------------------------------

    MFnDependencyNode skinClusterDep(skinCluster);
    MPlug influenceColor_plug = skinClusterDep.findPlug("influenceColor");
    int nbElements = influenceColor_plug.numElements();
    jointsColors.clear();
    MIntArray plugIndices;
    influenceColor_plug.getExistingArrayAttributeIndices(plugIndices);
    int nbJoints = plugIndices[plugIndices.length() - 1] + 1;
    if (verbose)
        MGlobal::displayInfo(influenceColor_plug.name() + " nbJoints [" + nbJoints +
                             "] nbElements [" + nbElements + "]");
    jointsColors.setLength(nbJoints);
    float black[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    for (int i = 0; i < nbJoints; ++i) {
        jointsColors.set(black, i);
    }
    for (int i = 0; i < nbElements; ++i) {
        // MGlobal::displayInfo(i);
        // weightList[i]
        MPlug colorPlug = influenceColor_plug.elementByPhysicalIndex(i);
        int logicalInd = colorPlug.logicalIndex();

        logicalInd = i;  // fix colors bug on joints

        if (colorPlug.isConnected()) {
            MPlugArray connections;
            colorPlug.connectedTo(connections, true, false);
            if (connections.length() > 0) {
                MPlug theConn = connections[0];
                float element[4] = {theConn.child(0).asFloat(), theConn.child(1).asFloat(),
                                    theConn.child(2).asFloat(), 1};
                if (verbose)
                    MGlobal::displayInfo(colorPlug.name() + " " + element[0] + " " + element[1] +
                                         " " + element[2]);
                jointsColors.set(element, logicalInd);
            } else {
                if (verbose)
                    MGlobal::displayInfo(colorPlug.name() + " " + black[0] + " " + black[1] + " " +
                                         black[2]);
                jointsColors.set(black, logicalInd);
            }
        } else {
            // MGlobal::displayInfo(colorPlug.name());
            float element[4] = {colorPlug.child(0).asFloat(), colorPlug.child(1).asFloat(),
                                colorPlug.child(2).asFloat(), 1};
            if (verbose)
                MGlobal::displayInfo(colorPlug.name() + " " + element[0] + " " + element[1] + " " +
                                     element[2]);
            jointsColors.set(element, logicalInd);
        }
    }
    return stat;
}

MStatus getListLockJoints(MObject &skinCluster, MIntArray &jointsLocks) {
    MStatus stat;

    MFnDependencyNode skinClusterDep(skinCluster);
    MPlug influenceColor_plug = skinClusterDep.findPlug("lockWeights");

    int nbJoints = influenceColor_plug.numElements();
    jointsLocks.clear();
    jointsLocks.setLength(nbJoints);

    for (int i = 0; i < nbJoints; ++i) {
        // weightList[i]
        MPlug lockPlug = influenceColor_plug.elementByPhysicalIndex(i);
        int isLocked = 0;
        if (lockPlug.isConnected()) {
            MPlugArray connections;
            lockPlug.connectedTo(connections, true, false);
            if (connections.length() > 0) {
                MPlug theConn = connections[0];
                isLocked = theConn.asInt();
            }
        } else {
            isLocked = lockPlug.asInt();
        }
        jointsLocks.set(isLocked, i);
        // MGlobal::displayInfo(lockPlug.name() + " " + isLocked);
    }
    return stat;
}

MStatus getListLockVertices(MObject &skinCluster, MIntArray &vertsLocks) {
    MStatus stat;

    MFnSkinCluster theSkinCluster(skinCluster);
    MObjectArray objectsDeformed;
    theSkinCluster.getOutputGeometry(objectsDeformed);
    MFnDependencyNode deformedNameMesh(objectsDeformed[0]);
    MPlug lockedVerticesPlug = deformedNameMesh.findPlug("lockedVertices", &stat);
    if (MS::kSuccess != stat) {
        MGlobal::displayError(MString("cant find lockerdVertices plug"));
        return stat;
    }

    MFnDependencyNode skinClusterDep(skinCluster);
    MPlug weight_list_plug = skinClusterDep.findPlug("weightList");

    int nbVertices = weight_list_plug.numElements();

    // vertsLocks.clear();
    MObject Data;
    stat = lockedVerticesPlug.getValue(Data);  // to get the attribute

    MFnIntArrayData intData(Data);
    MIntArray vertsLocksIndices = intData.array(&stat);
    vertsLocks.clear();
    vertsLocks = MIntArray(nbVertices, 0);
    for (unsigned int i = 0; i < vertsLocksIndices.length(); ++i)
        vertsLocks[vertsLocksIndices[i]] = 1;
    // MGlobal::displayInfo(MString(" getListLockVertices | ") + currentColorSet.name () + MString("
    // ") + vertsLocks.length());
    return stat;
}

MStatus getSymetryAttributes(MObject &skinCluster, MIntArray &symetryList) {
    MStatus stat;

    MFnSkinCluster theSkinCluster(skinCluster);
    MObjectArray objectsDeformed;
    theSkinCluster.getOutputGeometry(objectsDeformed);

    MFnDagNode deformedNameMesh(objectsDeformed[0]);
    MObject prt = deformedNameMesh.parent(0);

    MFnDependencyNode prtDep(prt);
    MPlug symVerticesPlug = prtDep.findPlug("symmetricVertices", &stat);
    if (MS::kSuccess != stat) {
        MGlobal::displayError(MString("cant find symmetricVertices plug"));
        return stat;
    }

    MObject Data;
    stat = symVerticesPlug.getValue(Data);  // to get the attribute

    MFnIntArrayData intData(Data);
    symetryList = intData.array(&stat);
}

MStatus getMirrorVertices(MIntArray mirrorVertices, MIntArray &theEditVerts,
                          MIntArray &theMirrorVerts, MIntArray &editAndMirrorVerts,
                          MDoubleArray &editVertsWeights, MDoubleArray &mirrorVertsWeights,
                          MDoubleArray &editAndMirrorWeights, bool doMerge) {
    // doMerge do we merge the weights ? if painting the same influence or smooth
    MStatus status;

    MIntArray vertExists(mirrorVertices.length(), -1);

    editAndMirrorVerts.copy(theEditVerts);
    editAndMirrorWeights.copy(editVertsWeights);
    if (!doMerge) {  // mirror verts same length and weights
        mirrorVertsWeights.copy(editVertsWeights);
        theMirrorVerts.setLength(theEditVerts.length());
    }

    for (unsigned int i = 0; i < theEditVerts.length(); ++i) vertExists[theEditVerts[i]] = i;
    for (unsigned int i = 0; i < theEditVerts.length(); ++i) {
        int theVert = theEditVerts[i];
        int theMirroredVert = mirrorVertices[theVert];

        if (!doMerge) theMirrorVerts[i] = theMirroredVert;  // to
        double theWeight = editVertsWeights[i];
        int indVertExists = vertExists[theMirroredVert];
        if (indVertExists == -1) {  // not in first array
            if (doMerge) {
                theMirrorVerts.append(theMirroredVert);
                mirrorVertsWeights.append(theWeight);
            }
            editAndMirrorVerts.append(theMirroredVert);
            editAndMirrorWeights.append(theWeight);
        } else if (doMerge && theWeight < 1.0) {  // clip weight at 1
            double prevWeight = editVertsWeights[indVertExists];
            if (theWeight >
                prevWeight) {  // add the remaining if existing weight is less than this new weight
                theMirrorVerts.append(theMirroredVert);
                mirrorVertsWeights.append(theWeight - prevWeight);
                editAndMirrorWeights[indVertExists] = theWeight;  // edit weight
            }
        }
    }
    // MGlobal::displayError(MString("theEditVerts ") + theEditVerts.length() + MString("
    // theMirrorVerts ") + theMirrorVerts.length() + MString(" editAndMirrorVerts ") +
    // editAndMirrorVerts.length() );

    return status;
}

MStatus editLocks(MObject &skinCluster, MIntArray &inputVertsToLock, bool addToLock,
                  MIntArray &vertsLocks) {
    MStatus stat;

    MFnSkinCluster theSkinCluster(skinCluster);
    MObjectArray objectsDeformed;
    theSkinCluster.getOutputGeometry(objectsDeformed);
    MFnDependencyNode deformedNameMesh(objectsDeformed[0]);
    MPlug lockedVerticesPlug = deformedNameMesh.findPlug("lockedVertices", &stat);
    if (MS::kSuccess != stat) {
        MGlobal::displayError(MString("cant find lockerdVertices plug"));
        return stat;
    }

    // now expand the array -----------------------
    int val = 0;
    if (addToLock) val = 1;
    for (unsigned int i = 0; i < inputVertsToLock.length(); ++i) {
        int vtx = inputVertsToLock[i];
        vertsLocks[vtx] = val;
    }
    MIntArray theArrayValues;
    for (unsigned int vtx = 0; vtx < vertsLocks.length(); ++vtx) {
        if (vertsLocks[vtx] == 1) theArrayValues.append(vtx);
    }
    // now set the value ---------------------------
    MFnIntArrayData tmpIntArray;
    stat = lockedVerticesPlug.setValue(tmpIntArray.create(theArrayValues));  // to set the attribute
    return stat;
}

MStatus getListColors(MObject &skinCluster, int nbVertices, MColorArray &currColors, bool verbose,
                      bool useMPlug) {
    MStatus stat;
    MFnDagNode skinClusterDag(skinCluster);
    MFnDependencyNode skinClusterDep(skinCluster);
    MColorArray jointsColors;
    getListColorsJoints(skinCluster, jointsColors, verbose);

    if (!useMPlug) {
        MFnSkinCluster theSkinCluster(skinCluster);
        int nbJoints = jointsColors.length();

        // now get the weights ----------------------------------------
        MObject allVerticesObj;

        MFnSingleIndexedComponent allVertices;
        MDoubleArray fullOrigWeights;

        allVertices.setCompleteData(nbVertices);
        allVerticesObj = allVertices.create(MFn::kMeshVertComponent);
        unsigned int infCount;

        MDagPath path;
        theSkinCluster.getPathAtIndex(0, path);
        theSkinCluster.getWeights(path, allVerticesObj, fullOrigWeights, infCount);

        // now get the colors per vertices ----------------------------------------
        int currentWeightsLength = fullOrigWeights.length();
        int indexInfluence, i, indexWeight;
        double theWeight, maxWeight;

        currColors.clear();
        currColors.setLength(nbVertices);

        for (i = 0; i < nbVertices; ++i) {
            MColor theColor;
            maxWeight = 0.;
            for (indexInfluence = 0; indexInfluence < nbJoints; indexInfluence++) {
                indexWeight = i * nbJoints + indexInfluence;
                theWeight = fullOrigWeights[indexWeight];
                if (theWeight > 0.) {
                    theColor += jointsColors[indexInfluence] * theWeight;
                }
                currColors[i] = theColor;
            }
        }
    } else {
        MPlug weight_list_plug = skinClusterDep.findPlug("weightList");
        // MGlobal::displayInfo(weight_list_plug.name());
        int nbElements = weight_list_plug.numElements();
        currColors.clear();
        currColors.setLength(nbVertices);

        for (int i = 0; i < nbVertices; ++i) {
            // weightList[i]
            // if (i > 50) break;
            MPlug ith_weights_plug = weight_list_plug.elementByPhysicalIndex(i);
            int vertexIndex = ith_weights_plug.logicalIndex();
            // MGlobal::displayInfo(ith_weights_plug.name());

            // weightList[i].weight
            MPlug plug_weights = ith_weights_plug.child(0);  // access first compound child
            int nb_weights = plug_weights.numElements();
            // MGlobal::displayInfo(plug_weights.name() + nb_weights);
            MColor theColor;
            for (int j = 0; j < nb_weights; j++) {  // for each joint
                MPlug weight_plug = plug_weights.elementByPhysicalIndex(j);
                // weightList[i].weight[j]
                int indexInfluence = weight_plug.logicalIndex();
                double theWeight = weight_plug.asDouble();
                // MGlobal::displayInfo(weight_plug.name() + " " + indexInfluence + " " +
                // theWeight);
                if (theWeight > 0.01) {
                    theColor += jointsColors[indexInfluence] * theWeight;
                }
            }
            currColors[vertexIndex] = theColor;
        }
    }

    return MS::kSuccess;
}

MStatus editArray(int command, int influence, int nbJoints, MIntArray &lockJoints,
                  MDoubleArray &fullWeightArray, std::map<int, double> &valuesToSet,
                  MDoubleArray &theWeights, bool normalize, double mutliplier) {
    MStatus stat;
    // 0 Add - 1 Remove - 2 AddPercent - 3 Absolute - 4 Smooth - 5 Sharpen - 6 LockVertices - 7
    // UnLockVertices
    //
    if (command == 5) {  // sharpen  -----------------------
        int i = 0;
        for (const auto &elem : valuesToSet) {
            int theVert = elem.first;
            double theVal = mutliplier * elem.second + 1.0;
            double substract = theVal / nbJoints;
            double sum = 0.0;
            for (int j = 0; j < nbJoints; ++j) {
                // check the zero val ----------
                double jntVal = (fullWeightArray[theVert * nbJoints + j] * theVal) - substract;
                jntVal = std::max(0.0, std::min(jntVal, 1.0));  // clamp
                theWeights[i * nbJoints + j] = jntVal;
                if (normalize) sum += jntVal;
            }
            // now normalize
            if ((normalize) && (sum != 1.0))
                for (int j = 0; j < nbJoints; ++j) theWeights[i * nbJoints + j] /= sum;
            i++;
        }
    } else {
        // do the command --------------------------
        int i = -1;  // i is a short index instead of theVert
        unsigned int jnt;
        for (const auto &elem : valuesToSet) {
            i++;
            int theVert = elem.first;
            double theVal = mutliplier * elem.second;
            // get the sum of weights

            double sumUnlockWeights = 0.0;
            for (jnt = 0; jnt < nbJoints; ++jnt) {
                if (lockJoints[jnt] == 0) {
                    sumUnlockWeights += fullWeightArray[theVert * nbJoints + jnt];
                }
                theWeights[i * nbJoints + jnt] =
                    fullWeightArray[theVert * nbJoints + jnt];  // preset array
            }

            double currentW = fullWeightArray[theVert * nbJoints + influence];
            // if (((command == 1) || (command == 3)) && (currentW > 0.99999)) { // value is 1 we
            // cant do anything

            if (((command == 1) || (command == 3)) &&
                (currentW > (sumUnlockWeights - .0001))) {  // value is 1(max) we cant do anything
                continue;                                   // we pass to next vertex
            }

            double newW = currentW;
            if (command == 0)
                newW += theVal;  // ADD
            else if (command == 1)
                newW -= theVal;  // Remove
            else if (command == 2)
                newW += theVal * newW;  // AddPercent
            else if (command == 3)
                newW = theVal;  // Absolute

            newW = std::max(0.0, std::min(newW, sumUnlockWeights));  // clamp
            /*
            double newRest = 1.0 - newW;
            double oldRest = 1.0 - currentW;
            double div = 1.0;
            */
            double newRest = sumUnlockWeights - newW;
            double oldRest = sumUnlockWeights - currentW;
            double div = sumUnlockWeights;

            if (newRest != 0.0) div = oldRest / newRest * sumUnlockWeights;

            // do the locks !!
            double sum = 0.0;
            for (jnt = 0; jnt < nbJoints; ++jnt) {
                if (lockJoints[jnt] == 1) {
                    continue;
                }
                // check the zero val ----------
                double weightValue = fullWeightArray[theVert * nbJoints + jnt];
                if (jnt == influence) {
                    weightValue = newW;
                } else {
                    if (newW == sumUnlockWeights) {
                        weightValue = 0.0;
                    } else {
                        weightValue /= div;
                    }
                }
                // if (normalize) theWeights[i*nbJoints + j] = std::max(0.0,
                // std::min(theWeights[i*nbJoints + j], 1.0));// clamp
                if (normalize) {
                    weightValue = std::max(0.0, std::min(weightValue, sumUnlockWeights));  // clamp
                }
                sum += weightValue;
                theWeights[i * nbJoints + jnt] = weightValue;
            }

            if ((sum == 0) ||
                (sum <
                 0.5 * sumUnlockWeights)) {  // zero problem revert weights ----------------------
                for (jnt = 0; jnt < nbJoints; ++jnt) {
                    theWeights[i * nbJoints + jnt] = fullWeightArray[theVert * nbJoints + jnt];
                }
            } else if (normalize && (sum != sumUnlockWeights)) {  // normalize ---------------
                for (jnt = 0; jnt < nbJoints; ++jnt)
                    if (lockJoints[jnt] == 0) {
                        theWeights[i * nbJoints + jnt] /= sum;               // to 1
                        theWeights[i * nbJoints + jnt] *= sumUnlockWeights;  // to sum weights
                    }
            }
        }
    }
    return stat;
}

MStatus setAverageWeight(std::vector<int> &verticesAround, int currentVertex, int indexCurrVert,
                         int nbJoints, MIntArray &lockJoints, MDoubleArray &fullWeightArray,
                         MDoubleArray &theWeights, double strengthVal) {
    MStatus stat;
    int sizeVertices = verticesAround.size();
    unsigned int i, jnt, posi;
    // MGlobal::displayInfo(MString(" paint smooth vtx [")+ currentVertex+ MString("] index - ") +
    // indexCurrVert + MString(" aroundCount ") + sizeVertices);

    MDoubleArray sumWeigths(nbJoints, 0.0);
    // compute sum weights
    for (int vertIndex : verticesAround) {
        for (jnt = 0; jnt < nbJoints; jnt++) {
            posi = vertIndex * nbJoints + jnt;
            sumWeigths[jnt] += fullWeightArray[posi];
        }
    }
    double total = 0.0;
    double totalBaseVtx = 0.0;
    for (jnt = 0; jnt < nbJoints; jnt++) {
        int posi = currentVertex * nbJoints + jnt;
        double origValue = fullWeightArray[posi];

        sumWeigths[jnt] /= sizeVertices;
        sumWeigths[jnt] =
            strengthVal * sumWeigths[jnt] + (1.0 - strengthVal) * origValue;  // add with strength

        total += sumWeigths[jnt];
        totalBaseVtx += origValue;
    }
    if (total > 0. && totalBaseVtx > 0.) {
        double mult = totalBaseVtx / total;
        for (jnt = 0; jnt < nbJoints; jnt++) {
            int posiToSet = indexCurrVert * nbJoints + jnt;
            sumWeigths[jnt] *= mult;  // normalement divide par 1
            theWeights[posiToSet] = sumWeigths[jnt];
        }
    }
    return MS::kSuccess;
}

MStatus doPruneWeight(MDoubleArray &theWeights, int nbJoints, double pruneCutWeight) {
    MStatus stat;

    int vertIndex, jnt, posiInArray;
    int nbElements = theWeights.length();
    int nbVertices = nbElements / nbJoints;
    double total = 0.0, val;

    // MDoubleArray WeightsCopy (theWeights);
    for (vertIndex = 0; vertIndex < nbVertices; ++vertIndex) {
        total = 0.0;
        for (jnt = 0; jnt < nbJoints; jnt++) {
            posiInArray = vertIndex * nbJoints + jnt;
            val = theWeights[posiInArray];
            if (val > pruneCutWeight) {
                total += val;
            } else {
                theWeights[posiInArray] = 0.0;
            }
        }
        // now normalize
        if (total != 1.0) {
            for (jnt = 0; jnt < nbJoints; jnt++) {
                posiInArray = vertIndex * nbJoints + jnt;
                theWeights[posiInArray] /= total;  // that should normalize
            }
        }
    }
    return MS::kSuccess;
};

// void Line(float x1, float y1, float x2, float y2, MIntArray &posiX, MIntArray &posiY)
void lineSTD(float x1, float y1, float x2, float y2, std::vector<std::pair<float, float>> &posi) {
    // Bresenham's line algorithm
    bool steep = (fabs(y2 - y1) > fabs(x2 - x1));
    if (steep) {
        std::swap(x1, y1);
        std::swap(x2, y2);
    }

    if (x1 > x2) {
        std::swap(x1, x2);
        std::swap(y1, y2);
    }

    float dx = x2 - x1;
    float dy = fabs(y2 - y1);

    float error = dx / 2.0f;
    int ystep = (y1 < y2) ? 1 : -1;
    int y = (int)y1;

    int maxX = (int)x2;

    for (int x = (int)x1; x < maxX; x++) {
        if (steep) {
            // SetPixel(y, x);
            posi.push_back(std::make_pair(y, x));
            // posiX.append(y);
            // posiY.append(x);
        } else {
            // SetPixel(x, y);
            posi.push_back(std::make_pair(x, y));
            // posiX.append(x);
            // posiY.append(y);
        }
        error -= dy;
        if (error < 0) {
            y += ystep;
            error += dx;
        }
    }
}

void lineC(short x0, short y0, short x1, short y1, std::vector<std::pair<short, short>> &posi) {
    short dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    short dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    short err = (dx > dy ? dx : -dy) / 2, e2;

    for (;;) {
        // setPixel(x0, y0);
        posi.push_back(std::make_pair(x0, y0));

        if (x0 == x1 && y0 == y1) break;
        e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}
float dist2D(short x0, short y0, short x1, short y1) {
    return sqrt((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0));
};

bool RayIntersectsBBox(MBoundingBox bbox, MPoint orig, MVector direction) {
    MPoint minPt = bbox.min();
    MPoint maxPt = bbox.max();

    double tmin = (minPt.x - orig.x) / direction.x;
    double tmax = (maxPt.x - orig.x) / direction.x;
    double tmpSwap;

    if (tmin > tmax) {
        tmpSwap = tmin;
        tmin = tmax;
        tmax = tmpSwap;
    }

    double tymin = (minPt.y - orig.y) / direction.y;
    double tymax = (maxPt.y - orig.y) / direction.y;

    if (tymin > tymax) {
        tmpSwap = tymin;
        tymin = tymax;
        tymax = tmpSwap;
    }

    if ((tmin > tymax) || (tymin > tmax)) return false;

    if (tymin > tmin) tmin = tymin;

    if (tymax < tmax) tmax = tymax;

    double tzmin = (minPt.z - orig.z) / direction.z;
    double tzmax = (maxPt.z - orig.z) / direction.z;

    if (tzmin > tzmax) {
        tmpSwap = tzmin;
        tzmin = tzmax;
        tzmax = tmpSwap;
    }

    if ((tmin > tzmax) || (tzmin > tmax)) return false;

    if (tzmin > tmin) tmin = tzmin;

    if (tzmax < tmax) tmax = tzmax;

    return true;
};

MPoint offsetIntersection(const MPoint &rayPoint, const MVector &rayVector,
                          const MVector &originNormal) {
    // A little hack to shift the input ray point around to get the intersections with the offset
    // planes
    MVector diff = rayPoint - originNormal;
    double prod = (diff * originNormal) / (rayVector * originNormal);
    return rayPoint - (rayVector * prod) + originNormal;
}

MMatrix bboxMatrix(const MPoint &minPoint, const MPoint &maxPoint, const MMatrix &bbSpace) {
    // Build the matrix of the bounding box as if it were a transformed 2x2x2 cube centered at the
    // origin
    MPoint c = (minPoint + maxPoint) / 2.0;
    MVector s = maxPoint - c;
    double matVals[4][4] = {
        {s.x, 0.0, 0.0, 0.0}, {0.0, s.y, 0.0, 0.0}, {0.0, 0.0, s.z, 0.0}, {c.x, c.y, c.z, 1.0}};
    return bbSpace * MMatrix(matVals);
}

inline bool inUnitPlane(const MPoint &inter) {
    // Quickly check if the intersection happened in the unit plane
    return (inter.x <= 1.0 && inter.x >= -1.0) && (inter.y <= 1.0 && inter.y >= -1.0) &&
           (inter.z <= 1.0 && inter.z >= -1.0);
}

bool bboxIntersection(const MPoint &minPoint, const MPoint &maxPoint, const MMatrix &bbSpace,
                      const MPoint &rayPoint, const MVector &rayVector, MPoint &intersection) {
    // Get the bbox matrix and its inverse
    MMatrix bbm = bboxMatrix(minPoint, maxPoint, bbSpace);
    MMatrix bbmi = bbm.inverse();

    // Transform the ray point/vector into the bbox matrix space
    MPoint rp = rayPoint * bbmi;
    MVector rv = rayVector * bbmi;

    MPoint rawInter;
    double rawDist = std::numeric_limits<double>::infinity();
    bool found = false;

    // For x/y/z
    for (int i = 0; i < 3; ++i) {
        // for +- 1
        for (int j = 0; j < 2; ++j) {
            MVector nrm;
            nrm[i] = 2 * j - 1;
            // Get the offset intersection
            MPoint inter = offsetIntersection(rp, rv, nrm);
            if (inUnitPlane(inter)) {
                // Keep track of the closest point
                double dist = inter.distanceTo(rp);
                if (dist < rawDist) {
                    rawDist = dist;
                    rawInter = inter;
                    found = true;
                }
            }
        }
    }

    // Only do the transformation if we found something
    if (found) intersection = rawInter * bbm;

    return found;
}
