////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxfdconfighwdefault.h
/// @brief FD default configuration for HW Hybrid solution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

.maxNumberOfFaces             = 10,
.maxFPSWithFaces              = 15,
.maxFPSWithNoFaces            = 15,
.multiCameraMaxFPSWithFaces   = 15,
.multiCameraMaxFPSWithNoFaces = 15,
.lockDetectedFaces            = TRUE,
.initialNoFrameSkipCount      = 15,
.maxPendingFrames             = 2,
.delayCount                   = 1,
.holdCount                    = 2,
.accuracy                     = FDAccuracyNormal,

.hwConfig =
{
    .enable                 = TRUE,
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

.FPFilter =
{
    .enable                     = TRUE,
    .baseThreshold              = 500,
    .innerThreshold             = 800,
    .expandFaceSizePercentage   = 25,
    .expandBoxBorderPercentage  = 25,
    .faceSpreadTolerance        = 0.3,
    .searchDensity              = FDSearchDensityNormal,
},

.ROIGenerator =
{
    .enable                     = TRUE,
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
    .newGoodFaceConfidence          = 770,
    .newNormalFaceConfidence        = 690,
    .existingFaceConfidence         = 500,
    .angleDiffForStrictConfidence   = 55,
    .strictNewGoodFaceConfidence    = 900,
    .strictNewNormalFaceConfidence  = 720,
    .strictExistingFaceConfidence   = 500,
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
        .minStableState          = 6,
        .stableThreshold         = 3,
        .threshold               = 20,
        .stateCount              = 4,
        .useReference            = FALSE,
        .filterType              = Average,
        .movingThreshold         = 65,
        .movingInitStateCount    = 4,
        .movingLinkFactor        = 1.5f,
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
        .minStableState          = 6,
        .stableThreshold         = 4,
        .threshold               = 200,
        .stateCount              = 4,
        .useReference            = FALSE,
        .filterType              = Average,
        .movingThreshold         = 65,
        .movingInitStateCount    = 4,
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
