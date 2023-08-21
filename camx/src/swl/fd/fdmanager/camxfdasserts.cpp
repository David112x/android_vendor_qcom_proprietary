////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxfdasserts.cpp
/// @brief Camx FD config tuning data and chi fd config comparison header file Version 1.0
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include "camxfdconfig.h"
#include "chifdproperty.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FD config tunign data comparison with chi exposed data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDFaceSize)                     == sizeof(struct FDFaceSize));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDHwConfig)                     == sizeof(struct FDHwConfig));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDSwConfig)                     == sizeof(struct FDSwConfig));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDFPFilterConfig)               == sizeof(struct FDFPFilterConfig));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDROIGeneratorConfig)           == sizeof(struct FDROIGeneratorConfig));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDROIManagerConfig)             == sizeof(struct FDROIManagerConfig));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDStabilizationConfig)          == sizeof(struct FDStabilizationConfig));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FaceDetectionCtrlType)          == sizeof(struct FDConfig));

CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDMedianFilter)                 == sizeof(struct FDMedianFilter));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDAverageFilter)                == sizeof(struct FDAverageFilter));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDHysteresisFilter)             == sizeof(struct FDHysteresisFilter));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDTemporalFilter)               == sizeof(struct FDTemporalFilter));
CAMX_STATIC_ASSERT(sizeof(camxfdconfig::FDStabilizationAttributeConfig) == sizeof(struct FDStabilizationAttributeConfig));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FaceDetectionCtrlType tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, maxNumberOfFaces)                ==
                    offsetof(struct FDConfig, maxNumberOfFaces));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, maxFPSWithFaces)                 ==
                    offsetof(struct FDConfig, maxFPSWithFaces));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, maxFPSWithNoFaces)               ==
                    offsetof(struct FDConfig, maxFPSWithNoFaces));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, multiCameraMaxFPSWithFaces)      ==
                    offsetof(struct FDConfig, multiCameraMaxFPSWithFaces));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, multiCameraMaxFPSWithNoFaces)    ==
                    offsetof(struct FDConfig, multiCameraMaxFPSWithNoFaces));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, lockDetectedFaces)               ==
                    offsetof(struct FDConfig, lockDetectedFaces));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, initialNoFrameSkipCount)         ==
                    offsetof(struct FDConfig, initialNoFrameSkipCount));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, maxPendingFrames)                ==
                    offsetof(struct FDConfig, maxPendingFrames));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, delayCount)                      ==
                    offsetof(struct FDConfig, delayCount));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, holdCount)                       ==
                    offsetof(struct FDConfig, holdCount));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, accuracy)                        ==
                    offsetof(struct FDConfig, accuracy));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, hwConfig)                        ==
                    offsetof(struct FDConfig, hwConfig));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, swConfig)                        ==
                    offsetof(struct FDConfig, swConfig));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, FPFilter)                        ==
                    offsetof(struct FDConfig, FPFilter));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, ROIGenerator)                    ==
                    offsetof(struct FDConfig, ROIGenerator));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, managerConfig)                   ==
                    offsetof(struct FDConfig, managerConfig));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FaceDetectionCtrlType, stabilization)                   ==
                    offsetof(struct FDConfig, stabilization));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDHwConfig tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHwConfig, enable)                   ==
                    offsetof(struct FDHwConfig, enable));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHwConfig, minFaceSize)              ==
                    offsetof(struct FDHwConfig, minFaceSize));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHwConfig, maxFaceSize)              ==
                    offsetof(struct FDHwConfig, maxFaceSize));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHwConfig, enableUpFrontAngles)      ==
                    offsetof(struct FDHwConfig, enableUpFrontAngles));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHwConfig, angle)                    ==
                    offsetof(struct FDHwConfig, angle));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHwConfig, upFrontAngle)             ==
                    offsetof(struct FDHwConfig, upFrontAngle));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHwConfig, threshold)                ==
                    offsetof(struct FDHwConfig, threshold));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHwConfig, noFaceFrameSkip)          ==
                    offsetof(struct FDHwConfig, noFaceFrameSkip));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHwConfig, newFaceFrameSkip)         ==
                    offsetof(struct FDHwConfig, newFaceFrameSkip));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDSwConfig tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, enable)                                   ==
                    offsetof(struct FDSwConfig, enable));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, minFaceSize)                              ==
                    offsetof(struct FDSwConfig, minFaceSize));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, maxFaceSize)                              ==
                    offsetof(struct FDSwConfig, maxFaceSize));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, frontProfileConfig)                       ==
                    offsetof(struct FDSwConfig, frontProfileConfig));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, halfProfileConfig)                        ==
                    offsetof(struct FDSwConfig, halfProfileConfig));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, fullProfileConfig)                        ==
                    offsetof(struct FDSwConfig, fullProfileConfig));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, enableUpFrontAngles)                      ==
                    offsetof(struct FDSwConfig, enableUpFrontAngles));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, skipIfNoOrientation)                      ==
                    offsetof(struct FDSwConfig, skipIfNoOrientation));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, detectionMode)                            ==
                    offsetof(struct FDSwConfig, detectionMode));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, newFaceSearchDensity)                     ==
                    offsetof(struct FDSwConfig, newFaceSearchDensity));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, existingFaceSearchDensity)                ==
                    offsetof(struct FDSwConfig, existingFaceSearchDensity));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, detectNewFacesInExistingFaceDirection)    ==
                    offsetof(struct FDSwConfig, detectNewFacesInExistingFaceDirection));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, noFaceSearchCycle)                        ==
                    offsetof(struct FDSwConfig, noFaceSearchCycle));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, newFaceSearchCycle)                       ==
                    offsetof(struct FDSwConfig, newFaceSearchCycle));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, newFaceSearchInterval)                    ==
                    offsetof(struct FDSwConfig, newFaceSearchInterval));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, upFrontNoFaceSearchCycle)                 ==
                    offsetof(struct FDSwConfig, upFrontNoFaceSearchCycle));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, upFrontNewFaceSearchCycle)                ==
                    offsetof(struct FDSwConfig, upFrontNewFaceSearchCycle));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, upFrontNewFaceSearchInterval)             ==
                    offsetof(struct FDSwConfig, upFrontNewFaceSearchInterval));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, positionSteadiness)                       ==
                    offsetof(struct FDSwConfig, positionSteadiness));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, sizeSteadiness)                           ==
                    offsetof(struct FDSwConfig, sizeSteadiness));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, rollAngleExtension)                       ==
                    offsetof(struct FDSwConfig, rollAngleExtension));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, yawAngleExtension)                        ==
                    offsetof(struct FDSwConfig, yawAngleExtension));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, noFaceFrameSkip)                          ==
                    offsetof(struct FDSwConfig, noFaceFrameSkip));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDSwConfig, newFaceFrameSkip)                         ==
                    offsetof(struct FDSwConfig, newFaceFrameSkip));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDFPFilterConfig tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, enable)                     ==
                    offsetof(struct FDFPFilterConfig, enable));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, baseThreshold)              ==
                    offsetof(struct FDFPFilterConfig, baseThreshold));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, innerThreshold)             ==
                    offsetof(struct FDFPFilterConfig, innerThreshold));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, expandFaceSizePercentage)   ==
                    offsetof(struct FDFPFilterConfig, expandFaceSizePercentage));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, expandBoxBorderPercentage)  ==
                    offsetof(struct FDFPFilterConfig, expandBoxBorderPercentage));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, faceSpreadTolerance)        ==
                    offsetof(struct FDFPFilterConfig, faceSpreadTolerance));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, searchDensity)              ==
                    offsetof(struct FDFPFilterConfig, searchDensity));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, maxNumOfFacesSecondCheck)   ==
                    offsetof(struct FDFPFilterConfig, maxNumOfFacesSecondCheck));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, maxFaceSizeSecondCheck)     ==
                    offsetof(struct FDFPFilterConfig, maxFaceSizeSecondCheck));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, minFaceSizeSecondCheck)     ==
                    offsetof(struct FDFPFilterConfig, minFaceSizeSecondCheck));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, minHWConfidenceSecondCheck) ==
                    offsetof(struct FDFPFilterConfig, minHWConfidenceSecondCheck));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, minDLConfidenceSecondCheck) ==
                    offsetof(struct FDFPFilterConfig, minDLConfidenceSecondCheck));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFPFilterConfig, uprightFaceOnlySecondCheck) ==
                    offsetof(struct FDFPFilterConfig, uprightFaceOnlySecondCheck));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDROIGeneratorConfig tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIGeneratorConfig, enable)                     ==
                    offsetof(struct FDROIGeneratorConfig, enable));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIGeneratorConfig, baseThreshold)              ==
                    offsetof(struct FDROIGeneratorConfig, baseThreshold));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIGeneratorConfig, innerThreshold)             ==
                    offsetof(struct FDROIGeneratorConfig, innerThreshold));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIGeneratorConfig, expandFaceSizePercentage)   ==
                    offsetof(struct FDROIGeneratorConfig, expandFaceSizePercentage));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIGeneratorConfig, expandBoxBorderPercentage)  ==
                    offsetof(struct FDROIGeneratorConfig, expandBoxBorderPercentage));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIGeneratorConfig, faceSpreadTolerance)        ==
                    offsetof(struct FDROIGeneratorConfig, faceSpreadTolerance));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIGeneratorConfig, searchDensity)              ==
                    offsetof(struct FDROIGeneratorConfig, searchDensity));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDROIManagerConfig tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, enable)                           ==
                    offsetof(struct FDROIManagerConfig, enable));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, newGoodFaceConfidence)            ==
                    offsetof(struct FDROIManagerConfig, newGoodFaceConfidence));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, newNormalFaceConfidence)          ==
                    offsetof(struct FDROIManagerConfig, newNormalFaceConfidence));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, existingFaceConfidence)           ==
                    offsetof(struct FDROIManagerConfig, existingFaceConfidence));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, angleDiffForStrictConfidence)     ==
                    offsetof(struct FDROIManagerConfig, angleDiffForStrictConfidence));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, strictNewGoodFaceConfidence)      ==
                    offsetof(struct FDROIManagerConfig, strictNewGoodFaceConfidence));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, strictNewNormalFaceConfidence)    ==
                    offsetof(struct FDROIManagerConfig, strictNewNormalFaceConfidence));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, strictExistingFaceConfidence)     ==
                    offsetof(struct FDROIManagerConfig, strictExistingFaceConfidence));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, faceLinkMoveDistanceRatio)        ==
                    offsetof(struct FDROIManagerConfig, faceLinkMoveDistanceRatio));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, faceLinkMinSizeRatio)             ==
                    offsetof(struct FDROIManagerConfig, faceLinkMinSizeRatio));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, faceLinkMaxSizeRatio)             ==
                    offsetof(struct FDROIManagerConfig, faceLinkMaxSizeRatio));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDROIManagerConfig, faceLinkRollAngleDifference)      ==
                    offsetof(struct FDROIManagerConfig, faceLinkRollAngleDifference));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDStabilizationConfig tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationConfig, enable)        ==
                    offsetof(struct FDStabilizationConfig, enable));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationConfig, historyDepth)  ==
                    offsetof(struct FDStabilizationConfig, historyDepth));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationConfig, position)      ==
                    offsetof(struct FDStabilizationConfig, position));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationConfig, size)          ==
                    offsetof(struct FDStabilizationConfig, size));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDFaceSize tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFaceSize, type)         ==
                    offsetof(struct FDFaceSize, type));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFaceSize, value.size)   ==
                    offsetof(struct FDFaceSize, size));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDFaceSize, value.ratio)  ==
                    offsetof(struct FDFaceSize, ratio));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDMedianFilter tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDMedianFilter, historyLength)  ==
                    offsetof(struct FDMedianFilter, historyLength));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDAverageFilter tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDAverageFilter, historyLength)  ==
                    offsetof(struct FDAverageFilter, historyLength));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDHysteresisFilter tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHysteresisFilter, startA)   ==
                    offsetof(struct FDHysteresisFilter, startA));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHysteresisFilter, endA)     ==
                    offsetof(struct FDHysteresisFilter, endA));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHysteresisFilter, startB)   ==
                    offsetof(struct FDHysteresisFilter, startB));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDHysteresisFilter, endB)     ==
                    offsetof(struct FDHysteresisFilter, endB));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDTemporalFilter tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDTemporalFilter, numerator)      ==
                    offsetof(struct FDTemporalFilter, numerator));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDTemporalFilter, denominator)    ==
                    offsetof(struct FDTemporalFilter, denominator));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDStabilizationAttributeConfig tuning definitions comparison with CHI definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, enable)                   ==
                    offsetof(struct FDStabilizationAttributeConfig, enable));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, mode)                     ==
                    offsetof(struct FDStabilizationAttributeConfig, mode));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, minStableState)           ==
                    offsetof(struct FDStabilizationAttributeConfig, minStableState));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, stableThreshold)          ==
                    offsetof(struct FDStabilizationAttributeConfig, stableThreshold));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, threshold)                ==
                    offsetof(struct FDStabilizationAttributeConfig, threshold));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, stateCount)               ==
                    offsetof(struct FDStabilizationAttributeConfig, stateCount));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, useReference)             ==
                    offsetof(struct FDStabilizationAttributeConfig, useReference));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, filterType)               ==
                    offsetof(struct FDStabilizationAttributeConfig, filterType));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, value.temporalFilter)     ==
                    offsetof(struct FDStabilizationAttributeConfig, temporalFilter));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, value.hysteresisFilter)   ==
                    offsetof(struct FDStabilizationAttributeConfig, hysteresisFilter));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, value.averageFilter)      ==
                    offsetof(struct FDStabilizationAttributeConfig, averageFilter));
CAMX_STATIC_ASSERT(offsetof(camxfdconfig::FDStabilizationAttributeConfig, value.medianFilter)       ==
                    offsetof(struct FDStabilizationAttributeConfig, medianFilter));

#endif // ANDROID
