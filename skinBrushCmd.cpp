#include "skinBrushFlags.h"
#include "skinBrushTool.h"

// ---------------------------------------------------------------------
// command to create the context
// ---------------------------------------------------------------------
SkinBrushContextCmd::SkinBrushContextCmd() {}

MPxContext* SkinBrushContextCmd::makeObj() {
    smoothContext = new SkinBrushContext();
    return smoothContext;
}

void* SkinBrushContextCmd::creator() { return new SkinBrushContextCmd(); }
// ---------------------------------------------------------------------
// pointers for the argument flags
// ---------------------------------------------------------------------

MStatus SkinBrushContextCmd::appendSyntax() {
    MSyntax syn = syntax();

    syn.addFlag(kAffectSelectedFlag, kAffectSelectedFlagLong, MSyntax::kBoolean);
    syn.addFlag(kColorRFlag, kColorRFlagLong, MSyntax::kDouble);
    syn.addFlag(kColorGFlag, kColorGFlagLong, MSyntax::kDouble);
    syn.addFlag(kColorBFlag, kColorBFlagLong, MSyntax::kDouble);
    syn.addFlag(kCurveFlag, kCurveFlagLong, MSyntax::kLong);
    syn.addFlag(kDepthFlag, kDepthFlagLong, MSyntax::kLong);
    syn.addFlag(kDepthStartFlag, kDepthStartFlagLong, MSyntax::kLong);
    syn.addFlag(kDrawBrushFlag, kDrawBrushFlagLong, MSyntax::kBoolean);
    syn.addFlag(kDrawRangeFlag, kDrawRangeFlagLong, MSyntax::kBoolean);
    syn.addFlag(kEnterToolCommandFlag, kEnterToolCommandFlagLong, MSyntax::kString);
    syn.addFlag(kExitToolCommandFlag, kExitToolCommandFlagLong, MSyntax::kString);
    syn.addFlag(kFloodFlag, kFloodFlagLong, MSyntax::kDouble);
    syn.addFlag(kFractionOversamplingFlag, kFractionOversamplingFlagLong, MSyntax::kBoolean);
    syn.addFlag(kIgnoreLockFlag, kIgnoreLockFlagLong, MSyntax::kBoolean);
    syn.addFlag(kKeepShellsTogetherFlag, kKeepShellsTogetherFlagLong, MSyntax::kBoolean);
    syn.addFlag(kLineWidthFlag, kLineWidthFlagLong, MSyntax::kLong);
    syn.addFlag(kMessageFlag, kMessageFlagLong, MSyntax::kLong);
    syn.addFlag(kOversamplingFlag, kOversamplingFlagLong, MSyntax::kLong);
    syn.addFlag(kRangeFlag, kRangeFlagLong, MSyntax::kDouble);
    syn.addFlag(kSizeFlag, kSizeFlagLong, MSyntax::kDouble);
    syn.addFlag(kStrengthFlag, kStrengthFlagLong, MSyntax::kDouble);
    syn.addFlag(kToleranceFlag, kToleranceFlagLong, MSyntax::kDouble);
    syn.addFlag(kUndersamplingFlag, kUndersamplingFlagLong, MSyntax::kLong);
    syn.addFlag(kVolumeFlag, kVolumeFlagLong, MSyntax::kBoolean);

    syn.addFlag(kSmoothStrengthFlag, kSmoothStrengthFlagLong, MSyntax::kDouble);

    syn.addFlag(kPruneWeightsFlag, kPruneWeightsFlagLong, MSyntax::kDouble);
    syn.addFlag(kCommandIndexFlag, kCommandIndexFlagLong, MSyntax::kLong);
    syn.addFlag(kSoloColorFlag, kSoloColorFlagLong, MSyntax::kLong);
    syn.addFlag(kSoloColorTypeFlag, kSoloColorTypeFlagLong, MSyntax::kLong);
    syn.addFlag(kStepsLineFlag, kStepsLineLong, MSyntax::kLong);
    syn.addFlag(kCoverageFlag, kCoverageLong, MSyntax::kLong);
    syn.addFlag(kPickMaxInfluenceFlag, kPickMaxInfluenceFlagLong, MSyntax::kBoolean);
    syn.addFlag(kPickInfluenceFlag, kPickInfluenceFlagLong, MSyntax::kBoolean);
    syn.addFlag(kInfluenceIndexFlag, kInfluenceIndexFlagLong, MSyntax::kLong);
    syn.addFlag(kInfluenceNameFlag, kInfluenceNameFlagLong, MSyntax::kString);
    syn.addFlag(kPostSettingFlag, kPostSettingFlagLong, MSyntax::kBoolean);
    syn.addFlag(kRefreshFlag, kRefreshFlagLong, MSyntax::kBoolean);
    syn.addFlag(kRefreshPtsNmsFlag, kRefreshPtsNmsFlagLong, MSyntax::kBoolean);
    syn.addFlag(kRefreshLocksJointsFlag, kRefreshLocksJointsFlagLong, MSyntax::kBoolean);
    syn.addFlag(kSkinClusterNameFlag, kSkinClusterNameFlagLong, MSyntax::kString);

    syn.addFlag(kListVerticesIndicesFlag, kListVerticesIndicesFlagLong, MSyntax::kLong);
    syn.makeFlagMultiUse(kListVerticesIndicesFlag);

    return MStatus::kSuccess;
}

MStatus SkinBrushContextCmd::doEditFlags() {
    MStatus status = MStatus::kSuccess;

    MArgParser argData = parser();

    if (argData.isFlagSet(kAffectSelectedFlag)) {
        bool value;
        status = argData.getFlagArgument(kAffectSelectedFlag, 0, value);
        smoothContext->setAffectSelected(value);
    }

    if (argData.isFlagSet(kColorRFlag)) {
        double value;
        status = argData.getFlagArgument(kColorRFlag, 0, value);
        smoothContext->setColorR((float)value);
    }

    if (argData.isFlagSet(kColorGFlag)) {
        double value;
        status = argData.getFlagArgument(kColorGFlag, 0, value);
        smoothContext->setColorG((float)value);
    }

    if (argData.isFlagSet(kColorBFlag)) {
        double value;
        status = argData.getFlagArgument(kColorBFlag, 0, value);
        smoothContext->setColorB((float)value);
    }

    if (argData.isFlagSet(kCurveFlag)) {
        int value;
        status = argData.getFlagArgument(kCurveFlag, 0, value);
        smoothContext->setCurve(value);
    }

    if (argData.isFlagSet(kDepthFlag)) {
        int value;
        status = argData.getFlagArgument(kDepthFlag, 0, value);
        smoothContext->setDepth(value);
    }

    if (argData.isFlagSet(kDepthStartFlag)) {
        int value;
        status = argData.getFlagArgument(kDepthStartFlag, 0, value);
        smoothContext->setDepthStart(value);
    }

    if (argData.isFlagSet(kDrawBrushFlag)) {
        bool value;
        status = argData.getFlagArgument(kDrawBrushFlag, 0, value);
        smoothContext->setDrawBrush(value);
    }

    if (argData.isFlagSet(kDrawRangeFlag)) {
        bool value;
        status = argData.getFlagArgument(kDrawRangeFlag, 0, value);
        smoothContext->setDrawRange(value);
    }

    if (argData.isFlagSet(kEnterToolCommandFlag)) {
        MString value;
        status = argData.getFlagArgument(kEnterToolCommandFlag, 0, value);
        smoothContext->setEnterToolCommand(value);
    }

    if (argData.isFlagSet(kExitToolCommandFlag)) {
        MString value;
        status = argData.getFlagArgument(kExitToolCommandFlag, 0, value);
        smoothContext->setExitToolCommand(value);
    }

    if (argData.isFlagSet(kFloodFlag)) {
        double value;
        status = argData.getFlagArgument(kFloodFlag, 0, value);
        smoothContext->setFlood(value);
    }

    if (argData.isFlagSet(kRefreshFlag)) {
        smoothContext->refresh();
    }

    if (argData.isFlagSet(kRefreshPtsNmsFlag)) {
        smoothContext->refreshPointsNormals();
    }

    if (argData.isFlagSet(kRefreshLocksJointsFlag)) {
        smoothContext->refreshJointsLocks();
    }

    if (argData.isFlagSet(kListVerticesIndicesFlag)) {
        bool foundListVerticesIndices = true;
        MIntArray indicesVertices;
        int nbVerts;
        int nbUse = argData.numberOfFlagUses(kListVerticesIndicesFlag);
        MString toDisplay("List Vertices : ");
        for (int i = 0; i < nbUse; i++) {
            MArgList flagArgs;
            argData.getFlagArgumentList(kListVerticesIndicesFlag, i, flagArgs);
            int vtxIndex;
            flagArgs.get(0, vtxIndex);
            indicesVertices.append(vtxIndex);
            toDisplay += vtxIndex;
            toDisplay += MString(" - ");
        }
        // nbVerts = indicesVertices_.length();
        smoothContext->refreshTheseVertices(indicesVertices);
    }

    if (argData.isFlagSet(kPickMaxInfluenceFlag)) {
        bool value;
        status = argData.getFlagArgument(kPickMaxInfluenceFlag, 0, value);
        smoothContext->setPickMaxInfluence(value);
    }
    if (argData.isFlagSet(kPickInfluenceFlag)) {
        bool value;
        status = argData.getFlagArgument(kPickInfluenceFlag, 0, value);
        smoothContext->setPickInfluence(value);
    }

    if (argData.isFlagSet(kInfluenceIndexFlag)) {
        int value;
        status = argData.getFlagArgument(kInfluenceIndexFlag, 0, value);
        smoothContext->setInfluenceIndex(value, false);  // not reselect in UI
    }
    if (argData.isFlagSet(kInfluenceNameFlag)) {
        MString value;
        status = argData.getFlagArgument(kInfluenceNameFlag, 0, value);
        smoothContext->setInfluenceByName(value);  // not reselect in UI
    }
    if (argData.isFlagSet(kPostSettingFlag)) {
        bool value;
        status = argData.getFlagArgument(kPostSettingFlag, 0, value);
        smoothContext->setPostSetting(value);
    }

    if (argData.isFlagSet(kFractionOversamplingFlag)) {
        bool value;
        status = argData.getFlagArgument(kFractionOversamplingFlag, 0, value);
        smoothContext->setFractionOversampling(value);
    }

    if (argData.isFlagSet(kIgnoreLockFlag)) {
        bool value;
        status = argData.getFlagArgument(kIgnoreLockFlag, 0, value);
        smoothContext->setIgnoreLock(value);
    }

    if (argData.isFlagSet(kKeepShellsTogetherFlag)) {
        bool value;
        status = argData.getFlagArgument(kKeepShellsTogetherFlag, 0, value);
        smoothContext->setKeepShellsTogether(value);
    }

    if (argData.isFlagSet(kLineWidthFlag)) {
        int value;
        status = argData.getFlagArgument(kLineWidthFlag, 0, value);
        smoothContext->setLineWidth(value);
    }

    if (argData.isFlagSet(kMessageFlag)) {
        int value;
        status = argData.getFlagArgument(kMessageFlag, 0, value);
        smoothContext->setMessage(value);
    }

    if (argData.isFlagSet(kOversamplingFlag)) {
        int value;
        status = argData.getFlagArgument(kOversamplingFlag, 0, value);
        smoothContext->setOversampling(value);
    }

    if (argData.isFlagSet(kRangeFlag)) {
        double value;
        status = argData.getFlagArgument(kRangeFlag, 0, value);
        smoothContext->setRange(value);
    }

    if (argData.isFlagSet(kSizeFlag)) {
        double value;
        status = argData.getFlagArgument(kSizeFlag, 0, value);
        smoothContext->setSize(value);
    }

    if (argData.isFlagSet(kStrengthFlag)) {
        double value;
        status = argData.getFlagArgument(kStrengthFlag, 0, value);
        smoothContext->setStrength(value);
    }

    if (argData.isFlagSet(kSmoothStrengthFlag)) {
        double value;
        status = argData.getFlagArgument(kSmoothStrengthFlag, 0, value);
        smoothContext->setSmoothStrength(value);
    }

    if (argData.isFlagSet(kToleranceFlag)) {
        double value;
        status = argData.getFlagArgument(kToleranceFlag, 0, value);
        smoothContext->setTolerance(value);
    }

    if (argData.isFlagSet(kPruneWeightsFlag)) {
        double value;
        status = argData.getFlagArgument(kPruneWeightsFlag, 0, value);
        smoothContext->setPruneWeights(value);
    }

    if (argData.isFlagSet(kUndersamplingFlag)) {
        int value;
        status = argData.getFlagArgument(kUndersamplingFlag, 0, value);
        smoothContext->setUndersampling(value);
    }

    if (argData.isFlagSet(kVolumeFlag)) {
        bool value;
        status = argData.getFlagArgument(kVolumeFlag, 0, value);
        smoothContext->setVolume(value);
    }

    if (argData.isFlagSet(kStepsLineFlag)) {
        int value;
        status = argData.getFlagArgument(kStepsLineFlag, 0, value);
        smoothContext->setStepLine(value);
    }

    if (argData.isFlagSet(kCommandIndexFlag)) {
        int value;
        status = argData.getFlagArgument(kCommandIndexFlag, 0, value);
        smoothContext->setCommandIndex(value);
    }

    if (argData.isFlagSet(kSoloColorFlag)) {
        int value;
        status = argData.getFlagArgument(kSoloColorFlag, 0, value);
        smoothContext->setSoloColor(value);
    }
    if (argData.isFlagSet(kSoloColorTypeFlag)) {
        int value;
        status = argData.getFlagArgument(kSoloColorTypeFlag, 0, value);
        smoothContext->setSoloColorType(value);
    }

    if (argData.isFlagSet(kCoverageFlag)) {
        bool value;
        status = argData.getFlagArgument(kCoverageFlag, 0, value);
        smoothContext->setCoverage(value);
    }

    return status;
}

MStatus SkinBrushContextCmd::doQueryFlags() {
    MArgParser argData = parser();

    if (argData.isFlagSet(kAffectSelectedFlag)) setResult(smoothContext->getAffectSelected());

    if (argData.isFlagSet(kColorRFlag)) setResult(smoothContext->getColorR());

    if (argData.isFlagSet(kColorGFlag)) setResult(smoothContext->getColorG());

    if (argData.isFlagSet(kColorBFlag)) setResult(smoothContext->getColorB());

    if (argData.isFlagSet(kCurveFlag)) setResult(smoothContext->getCurve());

    if (argData.isFlagSet(kDepthFlag)) setResult(smoothContext->getDepth());

    if (argData.isFlagSet(kDepthStartFlag)) setResult(smoothContext->getDepthStart());

    if (argData.isFlagSet(kDrawBrushFlag)) setResult(smoothContext->getDrawBrush());

    if (argData.isFlagSet(kDrawRangeFlag)) setResult(smoothContext->getDrawRange());

    if (argData.isFlagSet(kEnterToolCommandFlag)) setResult(smoothContext->getEnterToolCommand());

    if (argData.isFlagSet(kExitToolCommandFlag)) setResult(smoothContext->getExitToolCommand());

    if (argData.isFlagSet(kFractionOversamplingFlag))
        setResult(smoothContext->getFractionOversampling());

    if (argData.isFlagSet(kIgnoreLockFlag)) setResult(smoothContext->getIgnoreLock());

    if (argData.isFlagSet(kKeepShellsTogetherFlag))
        setResult(smoothContext->getKeepShellsTogether());

    if (argData.isFlagSet(kLineWidthFlag)) setResult(smoothContext->getLineWidth());

    if (argData.isFlagSet(kMessageFlag)) setResult(smoothContext->getMessage());

    if (argData.isFlagSet(kOversamplingFlag)) setResult(smoothContext->getOversampling());

    if (argData.isFlagSet(kRangeFlag)) setResult(smoothContext->getRange());

    if (argData.isFlagSet(kSizeFlag)) setResult(smoothContext->getSize());

    if (argData.isFlagSet(kStrengthFlag)) setResult(smoothContext->getStrength());

    if (argData.isFlagSet(kSmoothStrengthFlag)) setResult(smoothContext->getSmoothStrength());

    if (argData.isFlagSet(kToleranceFlag)) setResult(smoothContext->getTolerance());

    if (argData.isFlagSet(kPruneWeightsFlag)) setResult(smoothContext->getPruneWeights());

    if (argData.isFlagSet(kUndersamplingFlag)) setResult(smoothContext->getUndersampling());

    if (argData.isFlagSet(kVolumeFlag)) setResult(smoothContext->getVolume());

    if (argData.isFlagSet(kStepsLineFlag)) setResult(smoothContext->getStepLine());

    if (argData.isFlagSet(kCoverageFlag)) setResult(smoothContext->getCoverage());

    if (argData.isFlagSet(kInfluenceIndexFlag)) setResult(smoothContext->getInfluenceIndex());

    if (argData.isFlagSet(kInfluenceNameFlag)) setResult(smoothContext->getInfluenceName());

    if (argData.isFlagSet(kSkinClusterNameFlag)) setResult(smoothContext->getSkinClusterName());

    if (argData.isFlagSet(kCommandIndexFlag)) setResult(smoothContext->getCommandIndex());

    if (argData.isFlagSet(kSoloColorFlag)) setResult(smoothContext->getSoloColor());

    if (argData.isFlagSet(kSoloColorTypeFlag)) setResult(smoothContext->getSoloColorType());

    if (argData.isFlagSet(kPostSettingFlag)) setResult(smoothContext->getPostSetting());

    return MStatus::kSuccess;
}
