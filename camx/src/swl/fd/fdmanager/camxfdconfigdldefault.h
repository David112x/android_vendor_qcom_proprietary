////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxfdconfigdldefault.h
/// @brief FD default configuration for DL solution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

.maxNumberOfFaces             = 5,
.maxFPSWithFaces              = 15,
.maxFPSWithNoFaces            = 15,
.multiCameraMaxFPSWithFaces   = 15,
.multiCameraMaxFPSWithNoFaces = 15,
.lockDetectedFaces            = TRUE,
.initialNoFrameSkipCount      = 15,
.maxPendingFrames             = 2,
.delayCount                   = 1,
.holdCount                    = 1,
.retryCount                   = 3,
.accuracy                     = FDAccuracyHigh,

.hwConfig =
{
    .enable                 = FALSE,
    .minFaceSize =
    {
        .type               = FDFaceAdjFixed,
        .size               = 32,
    },
    .enableUpFrontAngles    = FALSE,
    .angle                  = FDAngleAll,
    .threshold              = 1,
    .noFaceFrameSkip        = 0,
    .newFaceFrameSkip       = 0,
    .enableHWFP             = FALSE,
},

.swConfig =
{
    .enable = TRUE,
    .minFaceSize =
    {
        .type               = FDFaceAdjFixed,
        .size               = 45,
    },
    .enableUpFrontAngles          = FALSE,
    .newFaceSearchDensity         = FDSearchDensityNormal,
    .noFaceSearchCycle            = 15,
    .newFaceSearchCycle           = 15,
    .newFaceSearchInterval        = 0,
    .upFrontNoFaceSearchCycle     = 10,
    .upFrontNewFaceSearchCycle    = 10,
    .upFrontNewFaceSearchInterval = 0,
    .positionSteadiness           = 0,
    .sizeSteadiness               = 0,
    .yawAngleExtension            = TRUE,
    .rollAngleExtension           = TRUE,
    .frontProfileConfig =
    {
        .searchAngle               = FDAngleAll,
        .upFrontSearchAnggle       = FDAngle45,
        .priorityAngleRange        = FDAngle45,
        .priorityThreshold         = 600,
        .nonPriorityThreshold      = 600,
    },
    .halfProfileConfig =
    {
        .searchAngle               = FDAngle15,
        .upFrontSearchAnggle       = FDAngle45,
        .priorityAngleRange        = FDAngle45,
        .priorityThreshold         = 600,
        .nonPriorityThreshold      = 600,
    },
    .fullProfileConfig =
    {
        .searchAngle               = FDAngle15,
        .upFrontSearchAnggle       = FDAngle45,
        .priorityAngleRange        = FDAngle45,
        .priorityThreshold         = 600,
        .nonPriorityThreshold      = 600,
    },
},

.FPFilter =
{
    .enable                     = FALSE,
    .baseThreshold              = 500,
    .innerThreshold             = 800,
    .expandFaceSizePercentage   = 25,
    .expandBoxBorderPercentage  = 25,
    .faceSpreadTolerance        = 0.3,
    .searchDensity              = FDSearchDensityNormal,
},

.ROIGenerator =
{
    .enable                     = FALSE,
    .baseThreshold              = 500,
    .innerThreshold             = 800,
    .expandFaceSizePercentage   = 25,
    .expandBoxBorderPercentage  = 100,
    .faceSpreadTolerance        = 0.9,
    .searchDensity              = FDSearchDensityNormal,
},

.managerConfig =
{
    .enable                         = TRUE,
    .newGoodFaceConfidence          = 865,
    .newNormalFaceConfidence        = 815,
    .existingFaceConfidence         = 300,
    .angleDiffForStrictConfidence   = 55,
    .strictNewGoodFaceConfidence    = 865,
    .strictNewNormalFaceConfidence  = 815,
    .strictExistingFaceConfidence   = 300,
    .faceLinkMoveDistanceRatio      = 1.1,
    .faceLinkMinSizeRatio           = 0.666666,
    .faceLinkMaxSizeRatio           = 1.5,
    .faceLinkRollAngleDifference    = 60.0,
},

.stabilization =
{
    .enable = TRUE,
    .historyDepth = 10,

    .position =
    {
        .enable                  = TRUE,
        .mode                    = WithinThreshold,
        .minStableState          = 4,
        .stableThreshold         = 6,
        .threshold               = 170,
        .stateCount              = 4,
        .useReference            = FALSE,
        .filterType              = Average,
        .movingThreshold         = 10,
        .movingInitStateCount    = 2,
        .movingLinkFactor        = 1.f,
        .averageFilter =
        {
            .historyLength       = 5,
            .movingHistoryLength = 3,
        },
    },

    .size =
    {
        .enable                  = TRUE,
        .mode                    = WithinThreshold,
        .minStableState          = 4,
        .stableThreshold         = 8,
        .threshold               = 280,
        .stateCount              = 4,
        .useReference            = FALSE,
        .filterType              = Average,
        .movingThreshold         = 35,
        .movingInitStateCount    = 2,
        .movingLinkFactor        = 1.5f,
        .averageFilter =
        {
            .historyLength       = 5,
            .movingHistoryLength = 3,
        },
    },
},

.swPreprocess =
{
    .exposureShortBySafeThreshold = 0.8f,
    .deltaEVFromTargetThreshold   = -0.3f,
},

.facialAttrConfig =
{
    .PTDMaxNumberOfFaces      = 2,
    .SMDMaxNumberOfFaces      = 2,
    .GBDMaxNumberOfFaces      = 2,
    .CTDMaxNumberOfFaces      = 2,
    .bFDStabilizationOverride = FALSE,
},
