// ---------------------------------------------------------------------
// define the syntax, needed to make it work with mel and python
// ---------------------------------------------------------------------

// The color flag for the brush is originally supposed to be a single
// flag taking three argument values for the rgb components. But the
// Maya issue MAYA-20162 as of 03/12/2918 prevents the MPxContextCommand
// to correctly utilize the MPxCommand::setResult() and ::appendResult()
// to query the three argument values. Therefore the color is divided
// into three separate flags for each color component. This is currently
// a workaround and might get resolved in future versions of Maya.

#define kColorRFlag "-cr"
#define kColorRFlagLong "-colorR"
#define kColorGFlag "-cg"
#define kColorGFlagLong "-colorG"
#define kColorBFlag "-cb"
#define kColorBFlagLong "-colorB"
#define kCurveFlag "-c"
#define kCurveFlagLong "-curve"
#define kDepthFlag "-d"
#define kDepthFlagLong "-depth"
#define kDepthStartFlag "-ds"
#define kDepthStartFlagLong "-depthStart"
#define kDrawBrushFlag "-db"
#define kDrawBrushFlagLong "-drawBrush"
#define kDrawRangeFlag "-dr"
#define kDrawRangeFlagLong "-drawRange"
#define kEnterToolCommandFlag "-etc"
#define kEnterToolCommandFlagLong "-enterToolCommand"
#define kExitToolCommandFlag "-xtc"
#define kExitToolCommandFlagLong "-exitToolCommand"
#define kFloodFlag "-f"
#define kFloodFlagLong "-flood"
#define kFractionOversamplingFlag "-fo"
#define kFractionOversamplingFlagLong "-fractionOversampling"
#define kIgnoreLockFlag "-il"
#define kIgnoreLockFlagLong "-ignoreLock"
#define kKeepShellsTogetherFlag "-kst"
#define kKeepShellsTogetherFlagLong "-keepShellsTogether"
#define kLineWidthFlag "-lw"
#define kLineWidthFlagLong "-lineWidth"
#define kMessageFlag "-m"
#define kMessageFlagLong "-message"
#define kOversamplingFlag "-o"
#define kOversamplingFlagLong "-oversampling"
#define kRangeFlag "-r"
#define kRangeFlagLong "-range"
#define kSizeFlag "-s"
#define kSizeFlagLong "-size"
#define kStrengthFlag "-st"
#define kStrengthFlagLong "-strength"
#define kToleranceFlag "-to"
#define kToleranceFlagLong "-tolerance"
#define kUndersamplingFlag "-us"
#define kUndersamplingFlagLong "-undersampling"
#define kVolumeFlag "-v"
#define kVolumeFlagLong "-volume"
///// Added by Guillaume
#define kPruneWeightsFlag "-pw"
#define kPruneWeightsFlagLong "-pruneWeights"

#define kSmoothStrengthFlag "-sst"
#define kSmoothStrengthFlagLong "-smoothStrength"

#define kStepsLineFlag "-sl"
#define kStepsLineLong "-stepLine"

#define kCoverageFlag "-co"
#define kCoverageLong "-coverage"

#define kPickMaxInfluenceFlag "-pi"
#define kPickMaxInfluenceFlagLong "-pickMaxInfluence"

#define kPickInfluenceFlag "-pid"
#define kPickInfluenceFlagLong "-pickInfluence"

#define kInfluenceIndexFlag "-ii"
#define kInfluenceIndexFlagLong "-influenceIndex"

#define kInfluenceNameFlag "-in"
#define kInfluenceNameFlagLong "-influenceName"

#define kCommandIndexFlag "-ci"
#define kCommandIndexFlagLong "-commandIndex"

#define kSoloColorFlag "-sc"
#define kSoloColorFlagLong "-soloColor"

#define kSoloColorTypeFlag "-sct"
#define kSoloColorTypeFlagLong "-soloColorType"

#define kPostSettingFlag "-ps"
#define kPostSettingFlagLong "-postSetting"

#define kRefreshFlag "-rf"
#define kRefreshFlagLong "-refresh"

#define kRefreshPtsNmsFlag "-rfpn"
#define kRefreshPtsNmsFlagLong "-refreshPointsNormals"

#define kRefreshLocksJointsFlag "-rfl"
#define kRefreshLocksJointsFlagLong "-refreshLocksJnts"

#define kSkinClusterNameFlag "-skn"
#define kSkinClusterNameFlagLong "-skinClusterName"

#define kMeshNameFlag "-msh"
#define kMeshNameFlagLong "-meshName"

#define kListVerticesIndicesFlag "-liv"
#define kListVerticesIndicesFlagLong "-listVerticesIndices"

#define kUseColorSetWhilePaintingFlag "-ucs"
#define kUseColorSetWhilePaintingFlagLong "-useColorSetsWhilePainting"

#define kMeshDragDrawTrianglesFlag "-mdt"
#define kMeshDragDrawTrianglesFlagLong "-meshdrawTriangles"

#define kMeshDragDrawEdgesFlag "-mde"
#define kMeshDragDrawEdgesFlagLong "-meshdrawEdges"

#define kMeshDragDrawPointsFlag "-mdp"
#define kMeshDragDrawPointsFlagLong "-meshdrawPoints"
