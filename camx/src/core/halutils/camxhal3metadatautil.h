////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxhal3metadatautil.h
/// @brief Set of static methods to operate/access camera_metadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXHAL3METADATAUTIL_H
#define CAMXHAL3METADATAUTIL_H

#include <system/camera_metadata.h>

#include "camxdefs.h"
#include "camxtypes.h"
#include "camxpropertyblob.h"
#include "camxpropertydefs.h"
#include "camxhal3types.h"
#include "camxvendortags.h"

CAMX_NAMESPACE_BEGIN

static const UINT MaxTagNameChars  = 128; ///< Maximum number of characters in tag name, section name
static const UINT MaxNumProperties = (PropertyIDPerFrameResultEnd - PropertyIDPerFrameResultBegin);
static const UINT PropertyBlobSize = sizeof(MainPropertyBlob) + sizeof(MainPropertyLinearLUT) + sizeof(UINT32);

static const SIZE_T TagElements[] =
{
    1,                   // ColorCorrectionMode
    3 * 3,               // ColorCorrectionTransform
    4,                   // ColorCorrectionGains
    1,                   // ColorCorrectionAberrationMode
    1,                   // ColorCorrectionAvailableAberrationModes
    1,                   // ControlAEAntibandingMode
    1,                   // ControlAEExposureCompensation
    1,                   // ControlAELock
    1,                   // ControlAEMode
    5,                   // ControlAERegions
    2,                   // ControlAETargetFpsRange
    1,                   // ControlAEPrecaptureTrigger
    1,                   // ControlAFMode
    5,                   // ControlAFRegions
    1,                   // ControlAFTrigger
    1,                   // ControlAWBLock
    1,                   // ControlAWBMode
    5,                   // ControlAWBRegions
    1,                   // ControlCaptureIntent
    1,                   // ControlEffectMode
    1,                   // ControlMode
    1,                   // ControlSceneMode
    1,                   // ControlVideoStabilizationMode
    1,                   // ControlAEAvailableAntibandingModes
    1,                   // ControlAEAvailableModes
    2,                   // ControlAEAvailableTargetFPSRanges
    2,                   // ControlAECompensationRange
    1,                   // ControlAECompensationStep
    1,                   // ControlAFAvailableModes
    1,                   // ControlAvailableEffects
    1,                   // ControlAvailableSceneModes
    1,                   // ControlAvailableVideoStabilizationModes
    1,                   // ControlAWBAvailableModes
    3,                   // ControlMaxRegions
    3,                   // ControlSceneModeOverrides
    1,                   // ControlAEPrecaptureId
    1,                   // ControlAEState
    1,                   // ControlAFState
    1,                   // ControlAFTriggerId
    1,                   // ControlAWBState
    5,                   // ControlAvailableHighSpeedVideoConfigurations
    1,                   // ControlAELockAvailable
    1,                   // ControlAWBLockAvailable
    1,                   // ControlAvailableModes
    2,                   // ControlPostRawSensitivityBoostRange
    1,                   // ControlPostRawSensitivityBoost
    1,                   // ControlZslEnable
    1,                   // DemosaicMode
    1,                   // EdgeMode
    1,                   // EdgeStrength
    1,                   // EdgeAvailableEdgeModes
    1,                   // FlashFiringPower
    1,                   // FlashFiringTime
    1,                   // FlashMode
    1,                   // FlashColorTemperature
    1,                   // FlashMaxEnergy
    1,                   // FlashState
    1,                   // FlashInfoAvailable
    1,                   // FlashInfoChargeDuration
    1,                   // HotPixelMode
    1,                   // HotPixelAvailableHotPixelModes
    3,                   // JPEGGpsCoordinates
    64,                  // JPEGGpsProcessingMethod
    1,                   // JPEGGpsTimestamp
    1,                   // JPEGOrientation
    1,                   // JPEGQuality
    1,                   // JPEGThumbnailQuality
    2,                   // JPEGThumbnailSize
    2,                   // JPEGAvailableThumbnailSizes
    1,                   // JPEGMaxSize
    1,                   // JPEGSize
    1,                   // LensAperture
    1,                   // LensFilterDensity
    1,                   // LensFocalLength
    1,                   // LensFocusDistance
    1,                   // LensOpticalStabilizationMode
    1,                   // LensFacing
    4,                   // LensPoseRotation
    3,                   // LensPoseTranslation
    2,                   // LensFocusRange
    1,                   // LensState
    5,                   // LensIntrinsicCalibration
    6,                   // LensRadialDistortion
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    1,                   // LensPoseReference
    5,                   // LensDistortion
#endif // Android-P or better
    1,                   // LensInfoAvailableApertures
    1,                   // LensInfoAvailableFilterDensities
    1,                   // LensInfoAvailableFocalLengths
    1,                   // LensInfoAvailableOpticalStabilization
    1,                   // LensInfoHyperfocalDistance
    1,                   // LensInfoMinimumFocusDistance
    2,                   // LensInfoShadingMapSize
    1,                   // LensInfoFocusDistanceCalibration
    1,                   // NoiseReductionMode
    1,                   // NoiseReductionStrength
    1,                   // NoiseReductionAvailableNoiseReductionModes
    1,                   // QuirksMeteringCropRegion
    1,                   // QuirksTriggerAFWithAuto
    1,                   // QuirksUseZslFormat
    1,                   // QuirksUsePartialResult
    1,                   // QuirksPartialResult
    1,                   // RequestFrameCount
    1,                   // RequestId
    1,                   // RequestInputStreams
    1,                   // RequestMetadataMode
    1,                   // RequestOutputStreams
    1,                   // RequestType
    3,                   // RequestMaxNumOutputStreams
    1,                   // RequestMaxNumReprocessStreams
    1,                   // RequestMaxNumInputStreams
    1,                   // RequestPipelineDepth
    1,                   // RequestPipelineMaxDepth
    1,                   // RequestPartialResultCount
    1,                   // RequestAvailableCapabilities
    1,                   // RequestAvailableRequestKeys
    1,                   // RequestAvailableResultKeys
    1,                   // RequestAvailableCharacteristicsKeys
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    1,                   // RequestAvailableSessionKeys
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
    4,                   // ScalerCropRegion
    1,                   // ScalerAvailableFormats
    1,                   // ScalerAvailableJPEGMinDurations
    2,                   // ScalerAvailableJPEGSizes
    1,                   // ScalerAvailableMaxDigitalZoom
    1,                   // ScalerAvailableProcessedMinDurations
    2,                   // ScalerAvailableProcessedSizes
    1,                   // ScalerAvailableRawMinDurations
    2,                   // ScalerAvailableRawSizes
    1,                   // ScalerAvailableInputOutputFormatsMap
    4,                   // ScalerAvailableStreamConfigurations
    4,                   // ScalerAvailableMinFrameDurations
    4,                   // ScalerAvailableStallDurations
    1,                   // ScalerCroppingType
    1,                   // SensorExposureTime
    1,                   // SensorFrameDuration
    1,                   // SensorSensitivity
    1,                   // SensorReferenceIlluminant1
    1,                   // SensorReferenceIlluminant2
    3 * 3,               // SensorCalibrationTransform1
    3 * 3,               // SensorCalibrationTransform2
    3 * 3,               // SensorColorTransform1
    3 * 3,               // SensorColorTransform2
    3 * 3,               // SensorForwardMatrix1
    3 * 3,               // SensorForwardMatrix2
    1,                   // SensorBaseGainFactor
    4,                   // SensorBlackLevelPattern
    1,                   // SensorMaxAnalogSensitivity
    1,                   // SensorOrientation
    3,                   // SensorProfileHueSaturationMapDimensions
    1,                   // SensorTimestamp
    1,                   // SensorTemperature
    3,                   // SensorNeutralColorPoint
    2,                   // SensorNoiseProfile
    3,                   // SensorProfileHueSaturationMap
    2,                   // SensorProfileToneCurve
    1,                   // SensorGreenSplit
    4,                   // SensorTestPatternData
    1,                   // SensorTestPatternMode
    1,                   // SensorAvailableTestPatternModes
    1,                   // SensorRollingShutterSkew
    1,                   // SensorOpticalBlackRegions
    4,                   // SensorDynamicBlackLevel
    1,                   // SensorDynamicWhiteLevel
    3,                   // SensorOpaqueRawSize
    4,                   // SensorInfoActiveArraySize
    2,                   // SensorInfoSensitivityRange
    1,                   // SensorInfoColorFilterArrangement
    2,                   // SensorInfoExposureTimeRange
    1,                   // SensorInfoMaxFrameDuration
    2,                   // SensorInfoPhysicalSize
    2,                   // SensorInfoPixelArraySize
    1,                   // SensorInfoWhiteLevel
    1,                   // SensorInfoTimestampSource
    1,                   // SensorInfoLensShadingApplied
    4,                   // SensorInfoPreCorrectionActiveArraySize
    1,                   // ShadingMode
    1,                   // ShadingStrength
    1,                   // ShadingAvailableModes
    1,                   // StatisticsFaceDetectMode
    1,                   // StatisticsHistogramMode
    1,                   // StatisticsSharpnessMapMode
    1,                   // StatisticsHotPixelMapMode
    1,                   // StatisticsFaceIds
    6,                   // StatisticsFaceLandmarks
    4,                   // StatisticsFaceRectangles
    1,                   // StatisticsFaceScores
    3,                   // StatisticsHistogram
    3,                   // StatisticsSharpnessMap
    1,                   // StatisticsLensShadingCorrectionMap
    4,                   // StatisticsLensShadingMap
    4,                   // StatisticsPredictedColorGains
    3 * 3,               // StatisticsPredictedColorTransform
    1,                   // StatisticsSceneFlicker
    2,                   // StatisticsHotPixelMap
    1,                   // StatisticsLensShadingMapMode
    1,                   // StatisticsInfoAvailableFaceDetectModes
    1,                   // StatisticsInfoHistogramBucketCount
    1,                   // StatisticsInfoMaxFaceCount
    1,                   // StatisticsInfoMaxHistogramCount
    1,                   // StatisticsInfoMaxSharpnessMapValue
    2,                   // StatisticsInfoSharpnessMapSize
    1,                   // StatisticsInfoAvailableHotPixelMapModes
    1,                   // StatisticsInfoAvailableLensShadingMapModes
    2,                   // TonemapCurveBlue
    2,                   // TonemapCurveGreen
    2,                   // TonemapCurveRed
    1,                   // TonemapMode
    1,                   // TonemapMaxCurvePoints
    1,                   // TonemapAvailableToneMapModes
    1,                   // TonemapGamma
    1,                   // TonemapPresetCurve
    1,                   // LedTransmit
    1,                   // LedAvailableLeds
    1,                   // InfoSupportedHardwareLevel
    1,                   // BlackLevelLock
    1,                   // SyncFrameNumber
    1,                   // SyncMaxLatency
    1,                   // ReprocessEffectiveExposureFactor
    1,                   // ReprocessMaxCaptureStall
    1,                   // DepthMaxDepthSamples
    4,                   // DepthAvailableDepthStreamConfigurations
    4,                   // DepthAvailableDepthMinFrameDurations
    4,                   // DepthAvailableDepthStallDurations
    1,                   // DepthDepthIsExclusive
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    1,                   // LogicalMultiCameraPhysicalIDs
    1,                   // LogicalMultiCameraSensorSyncType
    1,                   // DistortionCorrectionMode
    1,                   // DistortionCorrectionAvailableModes
    4,                   // HEICAvailableHEICStreamConfigurations
    4,                   // HEICAvailableHEICMinFrameDurations
    4,                   // HEICAvailableHEICStallDurations
    1,                   // HEICInfoSupported
    1,                   // HEICInfoMaxJpegAppSegmentsCount
#endif // Android-P or better
};

static const SIZE_T TagFactor[] =
{
    1,                                 // ColorCorrectionMode
    1,                                 // ColorCorrectionTransform
    1,                                 // ColorCorrectionGains
    1,                                 // ColorCorrectionAberrationMode
    ColorCorrectionAberrationModeEnd,  // ColorCorrectionAvailableAberrationModes
    1,                                 // ControlAEAntibandingMode
    1,                                 // ControlAEExposureCompensation
    1,                                 // ControlAELock
    1,                                 // ControlAEMode
    MaxROI,                            // ControlAERegions
    1,                                 // ControlAETargetFpsRange
    1,                                 // ControlAEPrecaptureTrigger
    1,                                 // ControlAFMode
    MaxROI,                            // ControlAFRegions
    1,                                 // ControlAFTrigger
    1,                                 // ControlAWBLock
    1,                                 // ControlAWBMode
    MaxROI,                            // ControlAWBRegions
    1,                                 // ControlCaptureIntent
    1,                                 // ControlEffectMode
    1,                                 // ControlMode
    1,                                 // ControlSceneMode
    1,                                 // ControlVideoStabilizationMode
    ControlAEAntibandingModeEnd,       // ControlAEAvailableAntibandingModes
    ControlAEModeEnd,                  // ControlAEAvailableModes
    MaxTagValues,                      // ControlAEAvailableTargetFPSRanges
    1,                                 // ControlAECompensationRange
    1,                                 // ControlAECompensationStep
    ControlAFModeEnd,                  // ControlAFAvailableModes
    ControlEffectModeEnd,              // ControlAvailableEffects
    ControlSceneModeEnd,               // ControlAvailableSceneModes
    ControlVideoStabilizationModeEnd,  // ControlAvailableVideoStabilizationModes
    ControlAWBModeEnd,                 // ControlAWBAvailableModes
    1,                                 // ControlMaxRegions
    ControlSceneModeEnd,               // ControlSceneModeOverrides
    1,                                 // ControlAEPrecaptureId
    1,                                 // ControlAEState
    1,                                 // ControlAFState
    1,                                 // ControlAFTriggerId
    1,                                 // ControlAWBState
    MaxHFRConfigs,                     // ControlAvailableHighSpeedVideoConfigurations
    ControlAELockEnd,                  // ControlAELockAvailable
    ControlAWBLockEnd,                 // ControlAWBLockAvailable
    ControlModeEnd,                    // ControlAvailableModes
    1,                                 // ControlPostRawSensitivityBoostRange
    1,                                 // ControlPostRawSensitivityBoost
    1,                                 // ControlZslEnable
    1,                                 // DemosaicMode
    1,                                 // EdgeMode
    1,                                 // EdgeStrength
    EdgeModeEnd,                       // EdgeAvailableEdgeModes
    1,                                 // FlashFiringPower
    1,                                 // FlashFiringTime
    1,                                 // FlashMode
    1,                                 // FlashColorTemperature
    1,                                 // FlashMaxEnergy
    1,                                 // FlashState
    1,                                 // FlashInfoAvailable
    1,                                 // FlashInfoChargeDuration
    1,                                 // HotPixelMode
    HotPixelModeEnd,                   // HotPixelAvailableHotPixelModes
    1,                                 // JPEGGpsCoordinates
    1,                                 // JPEGGpsProcessingMethod
    1,                                 // JPEGGpsTimestamp
    1,                                 // JPEGOrientation
    1,                                 // JPEGQuality
    1,                                 // JPEGThumbnailQuality
    1,                                 // JPEGThumbnailSize
    MaxResolutions,                    // JPEGAvailableThumbnailSizes
    1,                                 // JPEGMaxSize
    1,                                 // JPEGSize
    1,                                 // LensAperture
    1,                                 // LensFilterDensity
    1,                                 // LensFocalLength
    1,                                 // LensFocusDistance
    1,                                 // LensOpticalStabilizationMode
    1,                                 // LensFacing
    1,                                 // LensPoseRotation
    1,                                 // LensPoseTranslation
    1,                                 // LensFocusRange
    1,                                 // LensState
    1,                                 // LensIntrinsicCalibration
    1,                                 // LensRadialDistortion
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    1,                                 // LensPoseReference
    1,                                 // LensDistortion
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
    MaxTagValues,                      // LensInfoAvailableApertures
    MaxTagValues,                      // LensInfoAvailableFilterDensities
    MaxTagValues,                      // LensInfoAvailableFocalLengths
    LensOpticalStabilizationModeEnd,   // LensInfoAvailableOpticalStabilization
    1,                                 // LensInfoHyperfocalDistance
    1,                                 // LensInfoMinimumFocusDistance
    1,                                 // LensInfoShadingMapSize
    1,                                 // LensInfoFocusDistanceCalibration
    1,                                 // NoiseReductionMode
    1,                                 // NoiseReductionStrength
    NoiseReductionModeEnd,             // NoiseReductionAvailableNoiseReductionModes
    1,                                 // QuirksMeteringCropRegion
    1,                                 // QuirksTriggerAFWithAuto
    1,                                 // QuirksUseZslFormat
    1,                                 // QuirksUsePartialResult
    1,                                 // QuirksPartialResult
    1,                                 // RequestFrameCount
    1,                                 // RequestId
    1,                                 // RequestInputStreams
    1,                                 // RequestMetadataMode
    1,                                 // RequestOutputStreams
    1,                                 // RequestType
    1,                                 // RequestMaxNumOutputStreams
    1,                                 // RequestMaxNumReprocessStreams
    1,                                 // RequestMaxNumInputStreams
    1,                                 // RequestPipelineDepth
    1,                                 // RequestPipelineMaxDepth
    1,                                 // RequestPartialResultCount
    RequestAvailableCapabilitiesEnd,   // RequestAvailableCapabilities
    0,                                 // RequestAvailableRequestKeys
    0,                                 // RequestAvailableResultKeys
    0,                                 // RequestAvailableCharacteristicsKeys
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    0,                                 // RequestAvailableSessionKeys
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
    1,                                 // ScalerCropRegion
    MaxScalerFormats,                  // ScalerAvailableFormats
    MaxTagValues,                      // ScalerAvailableJPEGMinDurations
    MaxResolutions,                    // ScalerAvailableJPEGSizes
    1,                                 // ScalerAvailableMaxDigitalZoom
    MaxTagValues,                      // ScalerAvailableProcessedMinDurations
    MaxResolutions,                    // ScalerAvailableProcessedSizes
    MaxTagValues,                      // ScalerAvailableRawMinDurations
    MaxRawSizesSupported,              // ScalerAvailableRawSizes
    MaxTagValues,                      // ScalerAvailableInputOutputFormatsMap ??
    MaxResolutions * MaxScalerFormats * ScalerAvailableStreamConfigurationsEnd,
                                       // ScalerAvailableStreamConfigurations
    MaxResolutions * MaxScalerFormats, // ScalerAvailableMinFrameDurations
    MaxResolutions * MaxScalerFormats, // ScalerAvailableStallDurations
    1,                                 // ScalerCroppingType
    1,                                 // SensorExposureTime
    1,                                 // SensorFrameDuration
    1,                                 // SensorSensitivity
    1,                                 // SensorReferenceIlluminant1
    1,                                 // SensorReferenceIlluminant2
    1,                                 // SensorCalibrationTransform1
    1,                                 // SensorCalibrationTransform2
    1,                                 // SensorColorTransform1
    1,                                 // SensorColorTransform2
    1,                                 // SensorForwardMatrix1
    1,                                 // SensorForwardMatrix2
    1,                                 // SensorBaseGainFactor
    1,                                 // SensorBlackLevelPattern
    1,                                 // SensorMaxAnalogSensitivity
    1,                                 // SensorOrientation
    1,                                 // SensorProfileHueSaturationMapDimensions
    1,                                 // SensorTimestamp
    1,                                 // SensorTemperature
    1,                                 // SensorNeutralColorPoint
    MaxTagValues,                      // SensorNoiseProfile
    1,                                 // SensorProfileHueSaturationMap ??
    MaxCurvePoints,                    // SensorProfileToneCurve
    1,                                 // SensorGreenSplit
    1,                                 // SensorTestPatternData
    1,                                 // SensorTestPatternMode
    SensorTestPatternModeValuesCount,  // SensorAvailableTestPatternModes
    1,                                 // SensorRollingShutterSkew
    1,                                 // SensorOpticalBlackRegions
    1,                                 // SensorDynamicBlackLevel
    1,                                 // SensorDynamicWhiteLevel
    MaxOpaqueRawSizes,                 // SensorOpaqueRawSize
    1,                                 // SensorInfoActiveArraySize
    1,                                 // SensorInfoSensitivityRange
    1,                                 // SensorInfoColorFilterArrangement
    1,                                 // SensorInfoExposureTimeRange
    1,                                 // SensorInfoMaxFrameDuration
    1,                                 // SensorInfoPhysicalSize
    1,                                 // SensorInfoPixelArraySize
    1,                                 // SensorInfoWhiteLevel
    1,                                 // SensorInfoTimestampSource
    1,                                 // SensorInfoLensShadingApplied
    1,                                 // SensorInfoPreCorrectionActiveArraySize
    1,                                 // ShadingMode
    1,                                 // ShadingStrength
    ShadingModeEnd,                    // ShadingAvailableModes
    1,                                 // StatisticsFaceDetectMode
    1,                                 // StatisticsHistogramMode
    1,                                 // StatisticsSharpnessMapMode
    1,                                 // StatisticsHotPixelMapMode
    MaxROI,                            // StatisticsFaceIds
    MaxROI,                            // StatisticsFaceLandmarks
    MaxROI,                            // StatisticsFaceRectangles
    MaxROI,                            // StatisticsFaceScores
    MaxTagValues,                      // StatisticsHistogram
    MaxROI * MaxROI,                   // StatisticsSharpnessMap
    1,                                 // StatisticsLensShadingCorrectionMap
    ShadingV * ShadingH,               // StatisticsLensShadingMap
    1,                                 // StatisticsPredictedColorGains
    1,                                 // StatisticsPredictedColorTransform
    1,                                 // StatisticsSceneFlicker
    MaxHotPixels,                      // StatisticsHotPixelMap
    1,                                 // StatisticsLensShadingMapMode
    StatisticsFaceDetectModeEnd,       // StatisticsInfoAvailableFaceDetectModes
    1,                                 // StatisticsInfoHistogramBucketCount
    1,                                 // StatisticsInfoMaxFaceCount
    1,                                 // StatisticsInfoMaxHistogramCount
    1,                                 // StatisticsInfoMaxSharpnessMapValue
    1,                                 // StatisticsInfoSharpnessMapSize
    StatisticsHotPixelMapModeEnd,      // StatisticsInfoAvailableHotPixelMapModes
    StatisticsLensShadingMapModeEnd,   // StatisticsInfoAvailableLensShadingMapModes
    MaxCurvePoints,                    // TonemapCurveBlue
    MaxCurvePoints,                    // TonemapCurveGreen
    MaxCurvePoints,                    // TonemapCurveRed
    1,                                 // TonemapMode
    1,                                 // TonemapMaxCurvePoints
    TonemapModeEnd,                    // TonemapAvailableToneMapModes
    1,                                 // TonemapGamma
    1,                                 // TonemapPresetCurve
    1,                                 // LedTransmit
    LedAvailableLedsEnd,               // LedAvailableLeds
    1,                                 // InfoSupportedHardwareLevel
    1,                                 // BlackLevelLock
    1,                                 // SyncFrameNumber
    1,                                 // SyncMaxLatency
    1,                                 // ReprocessEffectiveExposureFactor
    1,                                 // ReprocessMaxCaptureStall
    1,                                 // DepthMaxDepthSamples
    1,                                 // DepthAvailableDepthStreamConfigurations
    1,                                 // DepthAvailableDepthMinFrameDurations
    1,                                 // DepthAvailableDepthStallDurations
    1,                                 // DepthDepthIsExclusive
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    1,                                 // LogicalMultiCameraPhysicalIDs
    1,                                 // LogicalMultiCameraSensorSyncType
    1,                                 // DistortionCorrectionMode
    DistortionCorrectionModesEnd,      // DistortionCorrectionAvailableModes
    MaxResolutions * MaxScalerFormats * ScalerAvailableStreamConfigurationsEnd,
                                       // HEICAvailableHEICStreamConfigurations
    MaxResolutions * MaxScalerFormats, // HEICAvailableHEICMinFrameDurations
    MaxResolutions * MaxScalerFormats, // HEICAvailableHEICStallDurations
    1,                                 // HEICInfoSupported
    1,                                 // HEICInfoMaxJpegAppSegmentsCount
#endif // Android-P or better
};

/// @brief map from tag type enum to size in bytes
static const UINT8 TagSizeByType[] =
{
    1, // TYPE_BYTE = 0,
    4, // TYPE_INT32 = 1,
    4, // TYPE_FLOAT = 2,
    8, // TYPE_INT64 = 3,
    8, // TYPE_DOUBLE = 4,
    8  // TYPE_RATIONAL = 5,
};

/// @brief data type of metadata tags
static const UINT8 TagType[] =
{
    TYPE_BYTE,           // ColorCorrectionMode
    TYPE_RATIONAL,       // ColorCorrectionTransform
    TYPE_FLOAT,          // ColorCorrectionGains
    TYPE_BYTE,           // ColorCorrectionAberrationMode
    TYPE_BYTE,           // ColorCorrectionAvailableAberrationModes
    TYPE_BYTE,           // ControlAEAntibandingMode
    TYPE_INT32,          // ControlAEExposureCompensation
    TYPE_BYTE,           // ControlAELock
    TYPE_BYTE,           // ControlAEMode
    TYPE_INT32,          // ControlAERegions
    TYPE_INT32,          // ControlAETargetFpsRange
    TYPE_BYTE,           // ControlAEPrecaptureTrigger
    TYPE_BYTE,           // ControlAFMode
    TYPE_INT32,          // ControlAFRegions
    TYPE_BYTE,           // ControlAFTrigger
    TYPE_BYTE,           // ControlAWBLock
    TYPE_BYTE,           // ControlAWBMode
    TYPE_INT32,          // ControlAWBRegions
    TYPE_BYTE,           // ControlCaptureIntent
    TYPE_BYTE,           // ControlEffectMode
    TYPE_BYTE,           // ControlMode
    TYPE_BYTE,           // ControlSceneMode
    TYPE_BYTE,           // ControlVideoStabilizationMode
    TYPE_BYTE,           // ControlAEAvailableAntibandingModes
    TYPE_BYTE,           // ControlAEAvailableModes
    TYPE_INT32,          // ControlAEAvailableTargetFPSRanges
    TYPE_INT32,          // ControlAECompensationRange
    TYPE_RATIONAL,       // ControlAECompensationStep
    TYPE_BYTE,           // ControlAFAvailableModes
    TYPE_BYTE,           // ControlAvailableEffects
    TYPE_BYTE,           // ControlAvailableSceneModes
    TYPE_BYTE,           // ControlAvailableVideoStabilizationModes
    TYPE_BYTE,           // ControlAWBAvailableModes
    TYPE_INT32,          // ControlMaxRegions
    TYPE_BYTE,           // ControlSceneModeOverrides
    TYPE_INT32,          // ControlAEPrecaptureId
    TYPE_BYTE,           // ControlAEState
    TYPE_BYTE,           // ControlAFState
    TYPE_INT32,          // ControlAFTriggerId
    TYPE_BYTE,           // ControlAWBState
    TYPE_INT32,          // ControlAvailableHighSpeedVideoConfigurations
    TYPE_BYTE,           // ControlAELockAvailable
    TYPE_BYTE,           // ControlAWBLockAvailable
    TYPE_BYTE,           // ControlAvailableModes
    TYPE_INT32,          // ControlPostRawSensitivityBoostRange
    TYPE_INT32,          // ControlPostRawSensitivityBoost
    TYPE_INT32,          // ControlZslEnable
    TYPE_BYTE,           // DemosaicMode
    TYPE_BYTE,           // EdgeMode
    TYPE_BYTE,           // EdgeStrength
    TYPE_BYTE,           // EdgeAvailableEdgeModes
    TYPE_BYTE,           // FlashFiringPower
    TYPE_INT64,          // FlashFiringTime
    TYPE_BYTE,           // FlashMode
    TYPE_BYTE,           // FlashColorTemperature
    TYPE_BYTE,           // FlashMaxEnergy
    TYPE_BYTE,           // FlashState
    TYPE_BYTE,           // FlashInfoAvailable
    TYPE_INT64,          // FlashInfoChargeDuration
    TYPE_BYTE,           // HotPixelMode
    TYPE_BYTE,           // HotPixelAvailableHotPixelModes
    TYPE_DOUBLE,         // JPEGGpsCoordinates
    TYPE_BYTE,           // JPEGGpsProcessingMethod
    TYPE_INT64,          // JPEGGpsTimestamp
    TYPE_INT32,          // JPEGOrientation
    TYPE_BYTE,           // JPEGQuality
    TYPE_BYTE,           // JPEGThumbnailQuality
    TYPE_INT32,          // JPEGThumbnailSize
    TYPE_INT32,          // JPEGAvailableThumbnailSizes
    TYPE_INT32,          // JPEGMaxSize
    TYPE_INT32,          // JPEGSize
    TYPE_FLOAT,          // LensAperture
    TYPE_FLOAT,          // LensFilterDensity
    TYPE_FLOAT,          // LensFocalLength
    TYPE_FLOAT,          // LensFocusDistance
    TYPE_BYTE,           // LensOpticalStabilizationMode
    TYPE_BYTE,           // LensFacing
    TYPE_FLOAT,          // LensPoseRotation
    TYPE_FLOAT,          // LensPoseTranslation
    TYPE_FLOAT,          // LensFocusRange
    TYPE_BYTE,           // LensState
    TYPE_FLOAT,          // LensIntrinsicCalibration
    TYPE_FLOAT,          // LensRadialDistortion
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    TYPE_BYTE,           // LensPoseReference
    TYPE_FLOAT,          // LensDistortion
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
    TYPE_FLOAT,          // LensInfoAvailableApertures
    TYPE_FLOAT,          // LensInfoAvailableFilterDensities
    TYPE_FLOAT,          // LensInfoAvailableFocalLengths
    TYPE_BYTE,           // LensInfoAvailableOpticalStabilization
    TYPE_FLOAT,          // LensInfoHyperfocalDistance
    TYPE_FLOAT,          // LensInfoMinimumFocusDistance
    TYPE_INT32,          // LensInfoShadingMapSize
    TYPE_BYTE,           // LensInfoFocusDistanceCalibration
    TYPE_BYTE,           // NoiseReductionMode
    TYPE_BYTE,           // NoiseReductionStrength
    TYPE_BYTE,           // NoiseReductionAvailableNoiseReductionModes
    TYPE_BYTE,           // QuirksMeteringCropRegion
    TYPE_BYTE,           // QuirksTriggerAFWithAuto
    TYPE_BYTE,           // QuirksUseZslFormat
    TYPE_BYTE,           // QuirksUsePartialResult
    TYPE_BYTE,           // QuirksPartialResult
    TYPE_INT32,          // RequestFrameCount
    TYPE_INT32,          // RequestId
    TYPE_INT32,          // RequestInputStreams
    TYPE_BYTE,           // RequestMetadataMode
    TYPE_INT32,          // RequestOutputStreams
    TYPE_BYTE,           // RequestType
    TYPE_INT32,          // RequestMaxNumOutputStreams
    TYPE_INT32,          // RequestMaxNumReprocessStreams
    TYPE_INT32,          // RequestMaxNumInputStreams
    TYPE_BYTE,           // RequestPipelineDepth
    TYPE_BYTE,           // RequestPipelineMaxDepth
    TYPE_INT32,          // RequestPartialResultCount
    TYPE_BYTE,           // RequestAvailableCapabilities
    TYPE_INT32,          // RequestAvailableRequestKeys
    TYPE_INT32,          // RequestAvailableResultKeys
    TYPE_INT32,          // RequestAvailableCharacteristicsKeys
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    TYPE_INT32,          // RequestAvailableSessionKeys
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
    TYPE_INT32,          // ScalerCropRegion
    TYPE_INT32,          // ScalerAvailableFormats
    TYPE_INT64,          // ScalerAvailableJPEGMinDurations
    TYPE_INT32,          // ScalerAvailableJPEGSizes
    TYPE_FLOAT,          // ScalerAvailableMaxDigitalZoom
    TYPE_INT64,          // ScalerAvailableProcessedMinDurations
    TYPE_INT32,          // ScalerAvailableProcessedSizes
    TYPE_INT64,          // ScalerAvailableRawMinDurations
    TYPE_INT32,          // ScalerAvailableRawSizes
    TYPE_INT32,          // ScalerAvailableInputOutputFormatsMap
    TYPE_INT32,          // ScalerAvailableStreamConfigurations
    TYPE_INT64,          // ScalerAvailableMinFrameDurations
    TYPE_INT64,          // ScalerAvailableStallDurations
    TYPE_BYTE,           // ScalerCroppingType
    TYPE_INT64,          // SensorExposureTime
    TYPE_INT64,          // SensorFrameDuration
    TYPE_INT32,          // SensorSensitivity
    TYPE_BYTE,           // SensorReferenceIlluminant1
    TYPE_BYTE,           // SensorReferenceIlluminant2
    TYPE_RATIONAL,       // SensorCalibrationTransform1
    TYPE_RATIONAL,       // SensorCalibrationTransform2
    TYPE_RATIONAL,       // SensorColorTransform1
    TYPE_RATIONAL,       // SensorColorTransform2
    TYPE_RATIONAL,       // SensorForwardMatrix1
    TYPE_RATIONAL,       // SensorForwardMatrix2
    TYPE_RATIONAL,       // SensorBaseGainFactor
    TYPE_INT32,          // SensorBlackLevelPattern
    TYPE_INT32,          // SensorMaxAnalogSensitivity
    TYPE_INT32,          // SensorOrientation
    TYPE_INT32,          // SensorProfileHueSaturationMapDimensions
    TYPE_INT64,          // SensorTimestamp
    TYPE_FLOAT,          // SensorTemperature
    TYPE_RATIONAL,       // SensorNeutralColorPoint
    TYPE_DOUBLE,         // SensorNoiseProfile
    TYPE_FLOAT,          // SensorProfileHueSaturationMap
    TYPE_FLOAT,          // SensorProfileToneCurve
    TYPE_FLOAT,          // SensorGreenSplit
    TYPE_INT32,          // SensorTestPatternData
    TYPE_INT32,          // SensorTestPatternMode
    TYPE_INT32,          // SensorAvailableTestPatternModes
    TYPE_INT64,          // SensorRollingShutterSkew
    TYPE_INT32,          // SensorOpticalBlackRegions
    TYPE_FLOAT,          // SensorDynamicBlackLevel
    TYPE_INT32,          // SensorDynamicWhiteLevel
    TYPE_INT32,          // SensorOpaqueRawSize
    TYPE_INT32,          // SensorInfoActiveArraySize
    TYPE_INT32,          // SensorInfoSensitivityRange
    TYPE_BYTE,           // SensorInfoColorFilterArrangement
    TYPE_INT64,          // SensorInfoExposureTimeRange
    TYPE_INT64,          // SensorInfoMaxFrameDuration
    TYPE_FLOAT,          // SensorInfoPhysicalSize
    TYPE_INT32,          // SensorInfoPixelArraySize
    TYPE_INT32,          // SensorInfoWhiteLevel
    TYPE_BYTE,           // SensorInfoTimestampSource
    TYPE_BYTE,           // SensorInfoLensShadingApplied
    TYPE_INT32,          // SensorInfoPreCorrectionActiveArraySize
    TYPE_BYTE,           // ShadingMode
    TYPE_BYTE,           // ShadingStrength
    TYPE_BYTE,           // ShadingAvailableModes
    TYPE_BYTE,           // StatisticsFaceDetectMode
    TYPE_BYTE,           // StatisticsHistogramMode
    TYPE_BYTE,           // StatisticsSharpnessMapMode
    TYPE_BYTE,           // StatisticsHotPixelMapMode
    TYPE_INT32,          // StatisticsFaceIds
    TYPE_INT32,          // StatisticsFaceLandmarks
    TYPE_INT32,          // StatisticsFaceRectangles
    TYPE_BYTE,           // StatisticsFaceScores
    TYPE_INT32,          // StatisticsHistogram
    TYPE_INT32,          // StatisticsSharpnessMap
    TYPE_BYTE,           // StatisticsLensShadingCorrectionMap
    TYPE_FLOAT,          // StatisticsLensShadingMap
    TYPE_FLOAT,          // StatisticsPredictedColorGains
    TYPE_RATIONAL,       // StatisticsPredictedColorTransform
    TYPE_BYTE,           // StatisticsSceneFlicker
    TYPE_INT32,          // StatisticsHotPixelMap
    TYPE_BYTE,           // StatisticsLensShadingMapMode
    TYPE_BYTE,           // StatisticsInfoAvailableFaceDetectModes
    TYPE_INT32,          // StatisticsInfoHistogramBucketCount
    TYPE_INT32,          // StatisticsInfoMaxFaceCount
    TYPE_INT32,          // StatisticsInfoMaxHistogramCount
    TYPE_INT32,          // StatisticsInfoMaxSharpnessMapValue
    TYPE_INT32,          // StatisticsInfoSharpnessMapSize
    TYPE_BYTE,           // StatisticsInfoAvailableHotPixelMapModes
    TYPE_BYTE,           // StatisticsInfoAvailableLensShadingMapModes
    TYPE_FLOAT,          // TonemapCurveBlue
    TYPE_FLOAT,          // TonemapCurveGreen
    TYPE_FLOAT,          // TonemapCurveRed
    TYPE_BYTE,           // TonemapMode
    TYPE_INT32,          // TonemapMaxCurvePoints
    TYPE_BYTE,           // TonemapAvailableToneMapModes
    TYPE_FLOAT,          // TonemapGamma
    TYPE_BYTE,           // TonemapPresetCurve
    TYPE_BYTE,           // LedTransmit
    TYPE_BYTE,           // LedAvailableLeds
    TYPE_BYTE,           // InfoSupportedHardwareLevel
    TYPE_BYTE,           // BlackLevelLock
    TYPE_INT64,          // SyncFrameNumber
    TYPE_INT32,          // SyncMaxLatency
    TYPE_FLOAT,          // ReprocessEffectiveExposureFactor
    TYPE_INT32,          // ReprocessMaxCaptureStall
    TYPE_INT32,          // DepthMaxDepthSamples
    TYPE_INT32,          // DepthAvailableDepthStreamConfigurations
    TYPE_INT64,          // DepthAvailableDepthMinFrameDurations
    TYPE_INT64,          // DepthAvailableDepthStallDurations
    TYPE_BYTE,           // DepthDepthIsExclusive
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    TYPE_BYTE,           // LogicalMultiCameraPhysicalIDs
    TYPE_BYTE,           // LogicalMultiCameraSensorSyncType
    TYPE_BYTE,           // DistortionCorrectionMode
    TYPE_BYTE,           // DistortionCorrectionAvailableModes
    TYPE_INT32,          // HEICAvailableHEICStreamConfigurations
    TYPE_INT64,          // HEICAvailableHEICMinFrameDurations
    TYPE_INT64,          // HEICAvailableHEICStallDurations
    TYPE_BYTE,           // HEICInfoSupported
    TYPE_BYTE,           // HEICInfoMaxJpegAppSegmentsCount
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
};

/// @brief Metadata tag type
enum MetadataTagType
{
    AndroidCameraTag, ///< camera metadata tag
    CamXVendorTag,    ///< vendor tag
    CamXProperty      ///< property
};

/// @brief Metadata details
struct MetadataInfo
{
    UINT32               tag;                        ///< 32-bit tag ID
    CHAR                 tagName[MaxTagNameChars];   ///< [Section Name].[Tag name]
    const CHAR*          pGroupName;                 ///< Group name
    MetadataTagType      tagType;                    ///< Type of the tag
    UINT32               size;                       ///< Size of the data
    UINT32               count;                      ///< Count of the data
    UINT8                type;                       ///< Type of the data
    UINT32               index;                      ///< Index of the data
    TagSectionVisibility visibility;                 ///< Visibility of the tag
};

/// @brief Table that contains all the metadata information
struct MetadataInfoTable
{
    UINT32       totalMetadataTags;                     ///< Declaration of number of metadata tags
    UINT32       totalCameraTags;                       ///< Declaration of number of camera metadata tags
    UINT32       totalVendorTags;                       ///< Declaration of number of total vendor tags
    UINT32       totalPropertyTags;                     ///< Declaration of number of property tags
    UINT32       totalTriggerTags;                      ///< Declaration of number of metadata tags that are
                                                        ///   only used as triggers
    UINT32       totalMetadataSize;                     ///< Declaration of size of metadata tags
    UINT32       totalCameraTagSize;                    ///< Declaration of size of camera metadata tags
    UINT32       totalPropertySize;                     ///< Declaration of size of vendor tags
    UINT32       totalVendorTagSize;                    ///< Declaration of size of property tags
    UINT32       mainPropertyIndex;                     ///< Index of the main property
    UINT32       internalPropertyIndex;                 ///< Index of the internal property
    UINT32       usecasePropertyIndex;                  ///< Index of the usecase property
    UINT32       debugPropertyIndex;                    ///< Index of the debug property
    MetadataInfo metadataInfoArray[MaxMetadataTags];    ///< Details of camera tags, vendor tags and properties
};

// @brief Structure to hold uniform tags
struct UniformMetadataMap
{
    UINT32      perFramePropertyID;      ///< Property ID
    UINT32      usecasePropertyID;       ///< Usecase propertyID
    const CHAR* pVendorTagSection;       ///< Section name
    const CHAR* pVendorTagName;          ///< Tag name
    UINT32      vendortagID;             ///< Vendor tag ID
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure to hold property info for sending to FW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PropertyPackingInfo
{
    UINT   count;                         ///< number of tags
    UINT32 tagId[MaxNumProperties];       ///< tagId
    BYTE*  pAddress[MaxNumProperties];    ///< payload
};

/// @brief Enum containing supported meta link data types to be stored in queue
enum PSMetadataType
{
    StreamCrop                = 0,        ///< Crop data type,
    ICAInPerspective          = 1,        ///< Input ICA Perspective
    ICAInGridOut2In           = 2,        ///< Input ICA Grid out2in
    ICAInGridIn2Out           = 3,        ///< Input ICA Grid in2out
    ICAReferenceParams        = 4,        ///< ICA reference params
    MaxPSMetaType                         ///< Maximum number of data type ids
};

/// @brief PS vendor tag information
struct PSVendorTagInfo
{
    const CHAR*     pSectionName;  ///< Vendor tag section
    const CHAR*     pTagName;      ///< Vendor tag name
    PSMetadataType  type;          ///< Port Specific Metadata  type
    BOOL            isPerPort;     ///< Indicate whether vendor tag is per port
    BOOL            multiInstance; ///< Indicates whether the data is multi-instance
    UINT            unitSize;      ///< Size of single entry
};

// @brief list of port specific vendor tags
static const struct PSVendorTagInfo PSVendorTags[] =
{
    {
        "com.qti.camera.streamCropInfo",
        "StreamCropInfo",
        PSMetadataType::StreamCrop,
        TRUE,
        TRUE,
        sizeof(StreamCropInfo),
    },
    {
        "org.quic.camera2.ipeicaconfigs",
        "ICAInPerspectiveTransform" ,
        PSMetadataType::ICAInPerspective,
        FALSE,
        FALSE,
        sizeof(IPEICAPerspectiveTransform),
    },
    {
        "org.quic.camera2.ipeicaconfigs",
        "ICAInGridOut2InTransform" ,
        PSMetadataType::ICAInGridOut2In,
        FALSE,
        FALSE,
        sizeof(IPEICAGridTransform),
    },
    {
        "org.quic.camera2.ipeicaconfigs",
        "ICAInGridIn2OutTransform" ,
        PSMetadataType::ICAInGridIn2Out,
        FALSE,
        FALSE,
        sizeof(IPEICAGridTransform),
    },
    {
        "org.quic.camera2.ipeicaconfigs",
        "ICAReferenceParams" ,
        PSMetadataType::ICAReferenceParams,
        FALSE,
        FALSE,
        sizeof(IPEICAPerspectiveTransform),
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Utility class of static methods to wrap around Android camera_metadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE CP017,CP018: All static class does not need copy/assignment overrides
class HAL3MetadataUtil
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateLinearOffsets
    ///
    /// @brief  Calculate the offsets of each metadata tag in a linear store of all possible metadata tags
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateLinearOffsets();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateSizeAllMeta
    ///
    /// @brief  Calculate the size of a camx metadata which should be able to hold request keys + result keys +
    ///         vendor tags + property blob
    ///
    /// @param  pEntryCapacity  Would be entry capacity for each metadata in the slot
    /// @param  pDataSize       Would be data capacity for each metadata in the slot
    /// @param  visibility      Visibility flag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateSizeAllMeta(
        SIZE_T* pEntryCapacity,
        SIZE_T* pDataSize,
        UINT32  visibility);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateSizeStaticMeta
    ///
    /// @brief  Calculate the size of a camx metadata which should be able to hold static keys
    ///
    /// @param  pEntryCapacity  Would be entry capacity for each metadata in the slot
    /// @param  pDataSize       Would be data capacity for each metadata in the slot
    /// @param  visibility      Visibility flag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateSizeStaticMeta(
        SIZE_T* pEntryCapacity,
        SIZE_T* pDataSize,
        UINT32  visibility);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateMetadata
    ///
    /// @brief  Create an Android metadata based on total size supplied
    ///
    /// @param  entryCapacity  Entry capacity for each metadata in the slot
    /// @param  dataSize       Data capacity for each metadata in the slot
    ///
    /// @return Created metadata
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static Metadata* CreateMetadata(
        SIZE_T entryCapacity,
        SIZE_T dataSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeMetadata
    ///
    /// @brief  Free memory for a previously created Android metadata
    ///
    /// @param  pMetadata Metadata pointer which needs to be freed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID FreeMetadata(
        Metadata* pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetMetadata
    ///
    /// @brief  Reset metadata to empty state
    ///
    /// @param  pMetadata       Metadata pointer which needs to be reset
    /// @param  dataCapacity    Data capacity
    /// @param  entryCapacity   Entry capacity
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    static VOID ResetMetadata(
        Metadata* pMetadata,
        SIZE_T    dataCapacity,
        SIZE_T    entryCapacity);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyMetadata
    ///
    /// @brief  Reset metadata to empty state
    ///
    /// @param  pDstMetadata    Destination metadata pointer
    /// @param  pSrcMetadata    Source metadata pointer
    /// @param  visibility      Visibility flag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CopyMetadata(
        Metadata*    pDstMetadata,
        Metadata*    pSrcMetadata,
        UINT32       visibility);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadata
    ///
    /// @brief  Retrieve the pointer to the data corresponding to the metadata tag
    ///
    /// @param  pMetadata   Metadata in which the tag should exist
    /// @param  tag         Specific metadata tag to look for
    /// @param  ppData      Pointer which will point to the data for the tag when the function returns
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetMetadata(
        Metadata*   pMetadata,
        UINT32      tag,
        VOID**      ppData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataCount
    ///
    /// @brief  Retrieve the count of data for metadata tag
    ///
    /// @param  pMetadata   Metadata in which the tag should exist
    /// @param  tag         Specific metadata tag to look for
    ///
    /// @return Count
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T GetMetadataCount(
        Metadata*   pMetadata,
        UINT32      tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllMetadataTagCount
    ///
    /// @brief  The count of all supported metadata google and vendor tags
    ///
    /// @return Count of tags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE UINT32 GetAllMetadataTagCount()
    {
        return MaxMetadataTagCount + VendorTagManager::GetAllTagCount();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMetadata
    ///
    /// @brief  Set contents of the metadata data, corresponding to the metadata tag
    ///
    /// @param  pMetadata     Metadata in which the tag should exist
    /// @param  tag           Specific metadata tag to look for
    /// @param  pData         Pointer to the data for the tag
    /// @param  count         Count of data the type of the tag
    /// @param  updateAllowed if a tag is already in the metadata, overwrite or retain existing value
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult UpdateMetadata(
        Metadata*   pMetadata,
        UINT32      tag,
        const VOID* pData,
        SIZE_T      count,
        BOOL        updateAllowed);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMetadata
    ///
    /// @brief  Overwrite the contents of the data, corresponding to the metadata tag
    ///
    /// @param  pMetadata     Metadata in which the tag should exist
    /// @param  tag           Specific metadata tag to look for
    /// @param  pData         Pointer to the data for the tag
    /// @param  count         Count of data the type of the tag
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static CamxResult SetMetadata(
        Metadata*   pMetadata,
        UINT32      tag,
        const VOID* pData,
        SIZE_T      count)
    {
        return UpdateMetadata(pMetadata, tag, pData, count, TRUE);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AppendMetadata
    ///
    /// @brief  Append src metadata to dst metadata
    ///
    /// @param  pDstMetadata   Dst metadata to which to append to
    /// @param  pSrcMetadata   Src metadata that needs to be appended to
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult AppendMetadata(
        Metadata*       pDstMetadata,
        const Metadata* pSrcMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MergeMetadata
    ///
    /// @brief  Merge src metadata into the dst i.e. if an entry is in the src metadata but not in the dst, then we add the
    ///         corresponding entry in the dst metadata. If both the src and dst metadata have the same entry, the value in
    ///         dst metadata is overwritten with the value in the src metadata
    ///
    /// @param  pDstMetadata   Dst metadata to which to append to
    /// @param  pSrcMetadata   Src metadata that needs to be appended to
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult MergeMetadata(
        Metadata*       pDstMetadata,
        const Metadata* pSrcMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPropertyBlob
    ///
    /// @brief  Retrieve the pointer to the MainPropertyBlob
    ///
    /// @param  pMetadata   Metadata in which the tag should exist
    /// @param  ppBlob      Pointer which will point to the blob when the function returns
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetPropertyBlob(
        Metadata*           pMetadata,
        MainPropertyBlob**  ppBlob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataEntryCount
    ///
    /// @brief  Get the number of active entries in the metadata
    ///
    /// @param  pMetadata   Metadata in which the tag should exist
    ///
    /// @return count of entries
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE SIZE_T GetMetadataEntryCount(
        const Metadata* pMetadata)
    {
        return get_camera_metadata_entry_count(static_cast<const camera_metadata*>(pMetadata));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSizeByType
    ///
    /// @brief  Get the size in bytes for the data type corresponding to a metadata tag
    ///
    /// @param  type Type being asked for
    ///
    /// @return Size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T GetSizeByType(
        UINT8 type);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTypeByTag
    ///
    /// @brief  Get the data type for a specific metadata tag
    ///
    /// @param  tag Tag being asked for
    ///
    /// @return Requested type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE UINT8 GetTypeByTag(
        UINT32 tag)
    {
        // Remove type identifer from tag
        tag &= ~DriverInternalGroupMask;
        return (0 == (tag & MetadataSectionVendorSectionStart)) ? TagType[CAMX_TAG_TO_INDEX(tag)] :
                                                                  static_cast<UINT8>(VendorTagManager::GetTagType(tag));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxMetadataSize
    ///
    /// @brief  Get the size to hold all metadata
    ///
    /// @return Size of all metadata
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE SIZE_T GetMaxMetadataSize()
    {
        return s_maxMetadataSize;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrieveMaxSizeByTag
    ///
    /// @brief  Get the max size, in number of bytes, in camx for a specific metadata tag/property
    ///
    /// @param  tag Tag being asked for
    ///
    /// @return Requested size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T RetrieveMaxSizeByTag(
        UINT32 tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxSizeByTag
    ///
    /// @brief  Get the max size, in number of bytes, in camx for a specific metadata tag
    ///
    /// @param  tag Tag being asked for
    ///
    /// @return Requested size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T GetMaxSizeByTag(
        UINT32 tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxSizeByIndex
    ///
    /// @brief  Get the max size, in number of bytes, in camx for a specific metadata tag referenced by index
    ///
    /// @param  index index of tag
    ///
    /// @return Requested size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE SIZE_T GetMaxSizeByIndex(
        SIZE_T index)
    {
        return ((GetAllMetadataTagCount() - 1) == index) ? (s_maxMetadataSize - s_tagOffset[index]) :
                                                           (s_tagOffset[index + 1] - s_tagOffset[index]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOffsetByIndex
    ///
    /// @brief  Lookup in LUT the offset in a linear struct of the tag represented by index
    ///
    /// @param  index Index corresponding to a tag
    ///
    /// @return Offset in a linear blob of the corresponding tag
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE SIZE_T GetOffsetByIndex(
        SIZE_T index)
    {
        return s_tagOffset[index];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDebugDataEnable
    ///
    /// @brief  Helper function to know if debug-data is enalbe or not
    ///
    /// @return TRUE if debug-data or tuning-metadata is enable
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IsDebugDataEnable();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DebugDataSize
    ///
    /// @brief  Get debug data size
    ///
    /// @param  debugDataType   Debug-data type
    ///
    /// @return Size required to allocate all debug data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T DebugDataSize(
        DebugDataType debugDataType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DebugDataOffset
    ///
    /// @brief  Get debug data offset
    ///
    /// @param  debugDataType   Debug-data type
    ///
    /// @return Size offset from start of debug data buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T DebugDataOffset(
        DebugDataType debugDataType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNameByTag
    ///
    /// @brief  Get a human readible string corresponding to the tag
    ///
    /// @param  pDst    Destination buffer
    /// @param  sizeDst Size of the destination buffer
    /// @param  tag     Tag to stringize
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetNameByTag(
        CHAR*       pDst,
        SIZE_T      sizeDst,
        UINT32      tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTagByIndex
    ///
    /// @brief  Get a metadata/vendortag/property tag corresponding to an index
    ///
    /// @param  index   Index of the LUT
    ///
    /// @return Tag for the corresponding index
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetTagByIndex(
        UINT32      index);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUniqueIndexByTag
    ///
    /// @brief  Get an unique metadata/vendortag/property index corresponding to tag
    ///
    /// @param  tag   Tag to get the index
    ///
    /// @return index for the corresponding tag
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE UINT32 GetUniqueIndexByTag(
        UINT32      tag)
    {
        UINT32 index;

        if (HAL3MetadataUtil::IsProperty(tag))
        {
            index = GetPropertyIndexByTag(tag);
        }
        else
        {
            index = CAMX_TAG_TO_INDEX(tag & ~DriverInternalGroupMask);
        }

        return index;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPropertyName
    ///
    /// @brief  Get a human readible string corresponding to the tag
    ///
    /// @param  tag     Tag to stringize
    ///
    /// @return Pointer to string for property name
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const CHAR* GetPropertyName(
        UINT32      tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpMetadata
    ///
    /// @brief  Structured dump of metadata
    ///
    /// @param  pMetadata   Metadata to dump
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpMetadata(
        const Metadata* pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintTagData
    ///
    /// @brief  Prints the data for a single tag
    ///
    /// @param  pMetadata   Metadata to dump
    /// @param  tag         Tag/Data to print
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID PrintTagData(
        const Metadata* pMetadata,
        UINT32          tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTagCount
    ///
    /// @brief  get number of tags based on visibility
    ///
    /// @param  visibility   visibility flag
    ///
    /// @return number of tags.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetTagCount(
        UINT32    visibility);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllTags
    ///
    /// @brief  get all the tags based on visibility
    ///
    /// @param  pVendorTags   array of tags as input
    /// @param  visibility    visibility flag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetAllTags(
        UINT32*   pVendorTags,
        UINT32    visibility);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTotalTagCount
    ///
    /// @brief  Returns the total tag count
    ///
    /// @return total tag count. Must be less than max metadata tags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE UINT32 GetTotalTagCount()
    {
        return s_metadataInfoTable.totalMetadataTags;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPropertyCount
    ///
    /// @brief  Returns the total property count
    ///
    /// @return total tag count. Must be less than max metadata tags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE UINT32 GetPropertyCount()
    {
        return s_metadataInfoTable.totalPropertyTags;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMetadataTableInitialized
    ///
    /// @brief  Checks if the metadata table is initialized
    ///
    /// @return TRUE is metadata table is initialized, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE BOOL IsMetadataTableInitialized()
    {
        return (0 < GetTotalTagCount()) ? TRUE : FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTagSizeByUniqueIndex
    ///
    /// @brief  Returns the total property count
    ///
    /// @param  tagIndex Unique index of the tag
    ///
    /// @return size of the tag
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetTagSizeByUniqueIndex(
        UINT32 tagIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeMetadataTable
    ///
    /// @brief  Update the metadata details in the table. This should be called when all the vendor tags are updated
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult InitializeMetadataTable();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsProperty
    ///
    /// @brief  Checks if the tag unit type is property or metadata
    ///
    /// @param  tag Tag to be queried
    ///
    /// @return true/false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE BOOL IsProperty(
        UINT32  tag)
    {
        return ((PropertyIDPerFrameResultBegin    <= (tag & PropertyGroupMask)) &&
                (PropertyIDPerFrameDebugDataBegin >= (tag & PropertyGroupMask)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AppendPropertyPackingInfo
    ///
    /// @brief  Append current property to packing info
    ///
    /// @param  tagId         TagId for the property to add
    /// @param  pValue        Pointer to the value for property to add
    /// @param  pPackingInfo  Pointer to the packing info
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult AppendPropertyPackingInfo(
        UINT32               tagId,
        BYTE*                pValue,
        PropertyPackingInfo* pPackingInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PackPropertyInfoToBlob
    ///
    /// @brief  Pack property info from metadata to blob for sending to framework
    ///
    /// @param  pPackingInfo   Pointer to the property info
    /// @param  pPropertyBlob  Pointer to the property blob to populate
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult PackPropertyInfoToBlob(
        PropertyPackingInfo* pPackingInfo,
        VOID*                pPropertyBlob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnPackBlobToPropertyInfo
    ///
    /// @brief  Unpack Blob to Property Info
    ///
    /// @param  pPackingInfo   Pointer to the property info to populate
    /// @param  pPropertyBlob  Pointer to the property blob
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult UnPackBlobToPropertyInfo(
        PropertyPackingInfo* pPackingInfo,
        VOID*                pPropertyBlob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataInfoByTag
    ///
    /// @brief  Gets the metadata information given the tag
    ///
    /// @param  tag Tag whose info info needs to be fetched
    ///
    /// @return Pointer to the metadata information
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const MetadataInfo* GetMetadataInfoByTag(
        UINT32  tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataInfoByIndex
    ///
    /// @brief  Gets the metadata information given the tag index
    ///
    /// @param  tagIndex Tag whose info info needs to be fetched
    ///
    /// @return Pointer to the metadata information
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE const MetadataInfo* GetMetadataInfoByIndex(
       UINT32  tagIndex)
    {
        CAMX_ASSERT(s_metadataInfoTable.totalMetadataTags > tagIndex);
        return &s_metadataInfoTable.metadataInfoArray[tagIndex];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintTag
    ///
    /// @brief  Checks if the tag unit type is property or metadata
    ///
    /// @param  tag      Tag to be printed
    /// @param  pPayload payload of the tag
    /// @param  count    count of the
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID PrintTag(
        UINT32          tag,
        VOID*           pPayload,
        UINT32          count);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpTag
    ///
    /// @brief  Dumps metadata tag
    ///
    /// @param  pFile    File pointer
    /// @param  tag      Tag to be printed
    /// @param  pData    Payload of the tag
    /// @param  count    Count of the tag
    /// @param  pInfo    Pointer to the metadat information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpTag(
        FILE*               pFile,
        UINT32              tag,
        BYTE*               pData,
        UINT32              count,
        const MetadataInfo* pInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataByIndex
    ///
    /// @brief  Retrieve the pointer to the data corresponding to the metadata index
    ///
    /// @param  pMetadata   Metadata in which the tag should exist
    /// @param  index       Index in the metadata to look up
    /// @param  pTag        Pointer to tag to fill with tag number
    /// @param  pCount      Pointer to size_t  to fill with tag count
    /// @param  ppData      Pointer which will point to the data for the tag when the function returns
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GetMetadataByIndex(
        Metadata*   pMetadata,
        SIZE_T      index,
        UINT32*     pTag,
        SIZE_T*     pCount,
        VOID**      ppData);

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumElementsByTag
    ///
    /// @brief  Get the number of elements for a specific metadata tag
    ///         For example, ColorCorrectionGains is a 1D array of floats for 4 color channel gains,
    ///         so the number of elements of ColorCorrectionGains is 4.
    ///
    /// @param  tag Tag being asked for
    ///
    /// @return number of basic elements
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE SIZE_T GetNumElementsByTag(
        UINT32 tag)
    {
        return (0 == (tag & MetadataSectionVendorSectionStart)) ? TagElements[CAMX_TAG_TO_INDEX(tag)] :
            (VendorTagManager::VendorTagSize(CAMX_TAG_TO_INDEX(tag) - MaxMetadataTagCount) /
                GetSizeByType(static_cast<UINT8>(VendorTagManager::GetTagType(tag))));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGroupName
    ///
    /// @brief  Update the metadata details in the table
    ///
    /// @param  tag TagID for which the group name must be retrieved
    ///
    /// @return Name of the metadata group
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const CHAR* GetGroupName(
        UINT32 tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPropertyTagSize
    ///
    /// @brief  Get the max size, in number of bytes, in camx for a specific property
    ///
    /// @param  tag Tag being asked for
    ///
    /// @return Requested size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static SIZE_T GetPropertyTagSize(
        UINT32 tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpMetadataTable
    ///
    /// @brief  Dump the metadata table
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpMetadataTable();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPropertyIndexByTag
    ///
    /// @brief  Get an unique property index corresponding to tag
    ///
    /// @param  tag   tag to get the index
    ///
    /// @return index for the corresponding tag
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE UINT32 GetPropertyIndexByTag(
        UINT32 tag)
    {
        UINT32 baseOffset = 0;
        UINT32 tagOffset  = (tag & ~DriverInternalGroupMask);

        switch (tag & PropertyGroupMask)
        {
            case PropertyIDPerFrameResultBegin:
                baseOffset = s_metadataInfoTable.mainPropertyIndex;
                break;
            case PropertyIDPerFrameInternalBegin:
                baseOffset = s_metadataInfoTable.internalPropertyIndex;
                break;
            case PropertyIDUsecaseBegin:
                baseOffset = s_metadataInfoTable.usecasePropertyIndex;
                break;
            case PropertyIDPerFrameDebugDataBegin:
                baseOffset = s_metadataInfoTable.debugPropertyIndex;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid Tag given: %x", tag);
                break;
        }

        return baseOffset + tagOffset;
    }

    static UINT32 s_allTags[MaxMetadataTags];       ///< Declaration of array that holds the superset
                                                    ///< of all request + result keys

    static UINT32 s_staticTags[MaxMetadataTags];    ///< Declaration of array that holds the superset
                                                    ///< of all request + result keys

    static UINT32 s_vendorTags[MaxMetadataTags];    ///< Declaration of array that holds the Vendor tags

    static SIZE_T s_tagOffset[MaxMetadataTags];     ///< Declaration of array that holds offset in a linear block to find each
                                                    ///  tag's data
    static SIZE_T s_maxMetadataSize;                ///< Declaration of the size of all metadata

    static MetadataInfoTable s_metadataInfoTable;   ///< Information regarding the metadata information

    HAL3MetadataUtil() = default;
};

CAMX_NAMESPACE_END

#endif // CAMXHAL3METADATAUTIL_H
