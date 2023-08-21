////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//
// Not a Contribution.
// Apache license notifications and license are retained
// for attribution purposes only.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3metadatatags.h
/// @brief Declarations of wrapped HAL3 metadata tag types mirroring.
///
/// Original tag definitions are defined in system/media/camera/include/system/camera_metadata_tags.h
/// Additional documentation on Google defined metadata tags can be found at system/media/camera/docs/docs.html
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2012 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE DC012: Enum members not documented.
// NOWHINE FILE GR002: Little extra length == much greater readability

#ifndef CAMXHAL3METADATATAGS_H
#define CAMXHAL3METADATATAGS_H

#include "camxdefs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Maximum number of values possible for a metadata tag.
static const UINT MaxTagValues = 32;

// InvalidIndex is assigned an arbitrarily large value
static const UINT32 InvalidIndex = 0xffffffff;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Camera Metadata section definitions
enum CameraMetadataSection
{
    MetadataSectionColorCorrection,
    MetadataSectionControl,
    MetadataSectionDemosaic,
    MetadataSectionEdge,
    MetadataSectionFlash,
    MetadataSectionFlashInfo,
    MetadataSectionHotPixel,
    MetadataSectionJPEG,
    MetadataSectionLens,
    MetadataSectionLensInfo,
    MetadataSectionNoiseReduction,
    MetadataSectionQuirks,
    MetadataSectionRequest,
    MetadataSectionScaler,
    MetadataSectionSensor,
    MetadataSectionSensorInfo,
    MetadataSectionShading,
    MetadataSectionStatistics,
    MetadataSectionStatisticsInfo,
    MetadataSectionTonemap,
    MetadataSectionLed,
    MetadataSectionInfo,
    MetadataSectionBlackLevel,
    MetadataSectionSync,
    MetadataSectionReprocess,
    MetadataSectionDepth,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    MetadataSectionLogicalMultiCamera,
    MetadataSectionDistortionCorrection,
    MetadataSectionHEIC,
    MetadataSectionHEICInfo,
#endif // Android-P or better
    MetadataSectionSectionCount,
    MetadataSectionVendorSection = 0x8000
};

/// @brief Hierarchy positions in enum space. All vendor extension tags must be defined with tag >= VendorSectionStart
enum CameraMetadataSectionStart
{
    MetadataSectionColorCorrectionStart    = MetadataSectionColorCorrection   << 16,
    MetadataSectionControlStart            = MetadataSectionControl           << 16,
    MetadataSectionDemosaicStart           = MetadataSectionDemosaic          << 16,
    MetadataSectionEdgeStart               = MetadataSectionEdge              << 16,
    MetadataSectionFlashStart              = MetadataSectionFlash             << 16,
    MetadataSectionFlashInfoStart          = MetadataSectionFlashInfo         << 16,
    MetadataSectionHotPixelStart           = MetadataSectionHotPixel          << 16,
    MetadataSectionJPEGStart               = MetadataSectionJPEG              << 16,
    MetadataSectionLensStart               = MetadataSectionLens              << 16,
    MetadataSectionLensInfoStart           = MetadataSectionLensInfo          << 16,
    MetadataSectionNoiseReductionStart     = MetadataSectionNoiseReduction    << 16,
    MetadataSectionQuirksStart             = MetadataSectionQuirks            << 16,
    MetadataSectionRequestStart            = MetadataSectionRequest           << 16,
    MetadataSectionScalerStart             = MetadataSectionScaler            << 16,
    MetadataSectionSensorStart             = MetadataSectionSensor            << 16,
    MetadataSectionSensorInfoStart         = MetadataSectionSensorInfo        << 16,
    MetadataSectionShadingStart            = MetadataSectionShading           << 16,
    MetadataSectionStatisticsStart         = MetadataSectionStatistics        << 16,
    MetadataSectionStatisticsInfoStart     = MetadataSectionStatisticsInfo    << 16,
    MetadataSectionTonemapStart            = MetadataSectionTonemap           << 16,
    MetadataSectionLedStart                = MetadataSectionLed               << 16,
    MetadataSectionInfoStart               = MetadataSectionInfo              << 16,
    MetadataSectionBlackLevelStart         = MetadataSectionBlackLevel        << 16,
    MetadataSectionSyncStart               = MetadataSectionSync              << 16,
    MetadataSectionReprocessStart          = MetadataSectionReprocess         << 16,
    MetadataSectionDepthStart              = MetadataSectionDepth             << 16,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    MetadataSectionLogicalMultiCameraStart   = MetadataSectionLogicalMultiCamera   << 16,
    MetadataSectionDistortionCorrectionStart = MetadataSectionDistortionCorrection << 16,
    MetadataSectionHEICStart                 = MetadataSectionHEIC                 << 16,
    MetadataSectionHEICInfoStart             = MetadataSectionHEICInfo             << 16,
#endif // Android-P or better
    MetadataSectionVendorSectionStart        = MetadataSectionVendorSection        << 16
};

/// @brief This is setup to allow us to duplicate the definitions of metadata for certain pools use
enum class MetadataPoolSection : UINT32
{
    /// If any change is made to this enum, same change should be made in chicommon.h
    // Result is implicit, and 0x0000
    Input   = 0x0800, // Input must be bitwise exclusive to the other so we can uniquely identify for both tags and props
    Usecase = 0x2000,
    // 0x3000 - Result Prop
    // 0x4000 - Internal Prop
    // 0x5000 - Usecase Prop
    // 0x6000 - DebugData Prop
    Static  = 0x7000,
};

/// Mask that shows if using special groups
static const UINT32  DriverInternalGroupMask = 0x7800 << 16;

/// Mask that shows if using property groups
static const UINT32  PropertyGroupMask = 0xF0000000;

/// Mask that shows if the metadata is a vendor tag
static const UINT32 VendorTagMask = 0x80000000;

/// Mask to OR with tags to represent input pool residency
static const UINT32 InputMetadataSectionMask   = static_cast<UINT32>(MetadataPoolSection::Input)   << 16;

/// Mask to OR with tags to represent usecase pool residency
static const UINT32 UsecaseMetadataSectionMask = static_cast<UINT32>(MetadataPoolSection::Usecase) << 16;

/// Mask to OR with tags to represent static pool residency
static const UINT32 StaticMetadataSectionMask = static_cast<UINT32>(MetadataPoolSection::Static) << 16;

/// @brief Main enum for defining camera metadata tags.
/// @note  MUST modify HAL3MetadataUtils::Tag{Elements,Factor,Type} consts to define new tag properties in the proper order
///        Must update for other pool types too
enum CameraMetadataTag
{
    ColorCorrectionMode                             = MetadataSectionColorCorrectionStart,
    ColorCorrectionTransform,
    ColorCorrectionGains,
    ColorCorrectionAberrationMode,
    ColorCorrectionAvailableAberrationModes,
    ColorCorrectionEnd,

    ControlAEAntibandingMode                        = MetadataSectionControlStart,
    ControlAEExposureCompensation,
    ControlAELock,
    ControlAEMode,
    ControlAERegions,
    ControlAETargetFpsRange,
    ControlAEPrecaptureTrigger,
    ControlAFMode,
    ControlAFRegions,
    ControlAFTrigger,
    ControlAWBLock,
    ControlAWBMode,
    ControlAWBRegions,
    ControlCaptureIntent,
    ControlEffectMode,
    ControlMode,
    ControlSceneMode,
    ControlVideoStabilizationMode,
    ControlAEAvailableAntibandingModes,
    ControlAEAvailableModes,
    ControlAEAvailableTargetFPSRanges,
    ControlAECompensationRange,
    ControlAECompensationStep,
    ControlAFAvailableModes,
    ControlAvailableEffects,
    ControlAvailableSceneModes,
    ControlAvailableVideoStabilizationModes,
    ControlAWBAvailableModes,
    ControlMaxRegions,
    ControlSceneModeOverrides,
    ControlAEPrecaptureId,
    ControlAEState,
    ControlAFState,
    ControlAFTriggerId,
    ControlAWBState,
    ControlAvailableHighSpeedVideoConfigurations,
    ControlAELockAvailable,
    ControlAWBLockAvailable,
    ControlAvailableModes,
    ControlPostRawSensitivityBoostRange,
    ControlPostRawSensitivityBoost,
    ControlZslEnable,
    ControlEnd,

    DemosaicMode                                    = MetadataSectionDemosaicStart,
    DemosaicEnd,

    EdgeMode                                        = MetadataSectionEdgeStart,
    EdgeStrength,
    EdgeAvailableEdgeModes,
    EdgeEnd,

    FlashFiringPower                                = MetadataSectionFlashStart,
    FlashFiringTime,
    FlashMode,
    FlashColorTemperature,
    FlashMaxEnergy,
    FlashState,
    FlashEnd,

    FlashInfoAvailable                              = MetadataSectionFlashInfoStart,
    FlashInfoChargeDuration,
    FlashInfoEnd,

    HotPixelMode                                    = MetadataSectionHotPixelStart,
    HotPixelAvailableHotPixelModes,
    HotPixelEnd,

    JPEGGpsCoordinates                              = MetadataSectionJPEGStart,
    JPEGGpsProcessingMethod,
    JPEGGpsTimestamp,
    JPEGOrientation,
    JPEGQuality,
    JPEGThumbnailQuality,
    JPEGThumbnailSize,
    JPEGAvailableThumbnailSizes,
    JPEGMaxSize,
    JPEGSize,
    JPEGEnd,

    LensAperture                                    = MetadataSectionLensStart,
    LensFilterDensity,
    LensFocalLength,
    LensFocusDistance,
    LensOpticalStabilizationMode,
    LensFacing,
    LensPoseRotation,
    LensPoseTranslation,
    LensFocusRange,
    LensState,
    LensIntrinsicCalibration,
    LensRadialDistortion,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    LensPoseReference,
    LensDistortion,
#endif // Android-P or better
    LensEnd,

    LensInfoAvailableApertures                      = MetadataSectionLensInfoStart,
    LensInfoAvailableFilterDensities,
    LensInfoAvailableFocalLengths,
    LensInfoAvailableOpticalStabilization,
    LensInfoHyperfocalDistance,
    LensInfoMinimumFocusDistance,
    LensInfoShadingMapSize,
    LensInfoFocusDistanceCalibration,
    LensInfoEnd,

    NoiseReductionMode                              = MetadataSectionNoiseReductionStart,
    NoiseReductionStrength,
    NoiseReductionAvailableNoiseReductionModes,
    NoiseReductionEnd,

    QuirksMeteringCropRegion                        = MetadataSectionQuirksStart,
    QuirksTriggerAFWithAuto,
    QuirksUseZslFormat,
    QuirksUsePartialResult,
    QuirksPartialResult,
    QuirksEnd,

    RequestFrameCount                               = MetadataSectionRequestStart,
    RequestId,
    RequestInputStreams,
    RequestMetadataMode,
    RequestOutputStreams,
    RequestType,
    RequestMaxNumOutputStreams,
    RequestMaxNumReprocessStreams,
    RequestMaxNumInputStreams,
    RequestPipelineDepth,
    RequestPipelineMaxDepth,
    RequestPartialResultCount,
    RequestAvailableCapabilities,
    RequestAvailableRequestKeys,
    RequestAvailableResultKeys,
    RequestAvailableCharacteristicsKeys,
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    RequestAvailableSessionKeys,
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
    RequestEnd,

    ScalerCropRegion                                = MetadataSectionScalerStart,
    ScalerAvailableFormats,
    ScalerAvailableJPEGMinDurations,
    ScalerAvailableJPEGSizes,
    ScalerAvailableMaxDigitalZoom,
    ScalerAvailableProcessedMinDurations,
    ScalerAvailableProcessedSizes,
    ScalerAvailableRawMinDurations,
    ScalerAvailableRawSizes,
    ScalerAvailableInputOutputFormatsMap,
    ScalerAvailableStreamConfigurations,
    ScalerAvailableMinFrameDurations,
    ScalerAvailableStallDurations,
    ScalerCroppingType,
    ScalerEnd,

    SensorExposureTime                              = MetadataSectionSensorStart,
    SensorFrameDuration,
    SensorSensitivity,
    SensorReferenceIlluminant1,
    SensorReferenceIlluminant2,
    SensorCalibrationTransform1,
    SensorCalibrationTransform2,
    SensorColorTransform1,
    SensorColorTransform2,
    SensorForwardMatrix1,
    SensorForwardMatrix2,
    SensorBaseGainFactor,
    SensorBlackLevelPattern,
    SensorMaxAnalogSensitivity,
    SensorOrientation,
    SensorProfileHueSaturationMapDimensions,
    SensorTimestamp,
    SensorTemperature,
    SensorNeutralColorPoint,
    SensorNoiseProfile,
    SensorProfileHueSaturationMap,
    SensorProfileToneCurve,
    SensorGreenSplit,
    SensorTestPatternData,
    SensorTestPatternMode,
    SensorAvailableTestPatternModes,
    SensorRollingShutterSkew,
    SensorOpticalBlackRegions,
    SensorDynamicBlackLevel,
    SensorDynamicWhiteLevel,
    SensorOpaqueRawSize,
    SensorEnd,

    SensorInfoActiveArraySize                       = MetadataSectionSensorInfoStart,
    SensorInfoSensitivityRange,
    SensorInfoColorFilterArrangement,
    SensorInfoExposureTimeRange,
    SensorInfoMaxFrameDuration,
    SensorInfoPhysicalSize,
    SensorInfoPixelArraySize,
    SensorInfoWhiteLevel,
    SensorInfoTimestampSource,
    SensorInfoLensShadingApplied,
    SensorInfoPreCorrectionActiveArraySize,
    SensorInfoEnd,

    ShadingMode                                     = MetadataSectionShadingStart,
    ShadingStrength,
    ShadingAvailableModes,
    ShadingEnd,

    StatisticsFaceDetectMode                        = MetadataSectionStatisticsStart,
    StatisticsHistogramMode,
    StatisticsSharpnessMapMode,
    StatisticsHotPixelMapMode,
    StatisticsFaceIds,
    StatisticsFaceLandmarks,
    StatisticsFaceRectangles,
    StatisticsFaceScores,
    StatisticsHistogram,
    StatisticsSharpnessMap,
    StatisticsLensShadingCorrectionMap,
    StatisticsLensShadingMap,
    StatisticsPredictedColorGains,
    StatisticsPredictedColorTransform,
    StatisticsSceneFlicker,
    StatisticsHotPixelMap,
    StatisticsLensShadingMapMode,
    StatisticsEnd,

    StatisticsInfoAvailableFaceDetectModes          = MetadataSectionStatisticsInfoStart,
    StatisticsInfoHistogramBucketCount,
    StatisticsInfoMaxFaceCount,
    StatisticsInfoMaxHistogramCount,
    StatisticsInfoMaxSharpnessMapValue,
    StatisticsInfoSharpnessMapSize,
    StatisticsInfoAvailableHotPixelMapModes,
    StatisticsInfoAvailableLensShadingMapModes,
    StatisticsInfoEnd,

    TonemapCurveBlue                                = MetadataSectionTonemapStart,
    TonemapCurveGreen,
    TonemapCurveRed,
    TonemapMode,
    TonemapMaxCurvePoints,
    TonemapAvailableToneMapModes,
    TonemapGamma,
    TonemapPresetCurve,
    TonemapEnd,

    LedTransmit                                     = MetadataSectionLedStart,
    LedAvailableLeds,
    LedEnd,

    InfoSupportedHardwareLevel                      = MetadataSectionInfoStart,
    InfoEnd,

    BlackLevelLock                                  = MetadataSectionBlackLevelStart,
    BlackLevelEnd,

    SyncFrameNumber                                 = MetadataSectionSyncStart,
    SyncMaxLatency,
    SyncEnd,

    ReprocessEffectiveExposureFactor                = MetadataSectionReprocessStart,
    ReprocessMaxCaptureStall,
    ReprocessEnd,

    DepthMaxDepthSamples                            = MetadataSectionDepthStart,
    DepthAvailableDepthStreamConfigurations,
    DepthAvailableDepthMinFrameDurations,
    DepthAvailableDepthStallDurations,
    DepthDepthIsExclusive,
    DepthEnd,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    LogicalMultiCameraPhysicalIDs                     = MetadataSectionLogicalMultiCameraStart,
    LogicalMultiCameraSensorSyncType,
    LogicalMultiCameraEnd,
    DistortionCorrectionMode                          = MetadataSectionDistortionCorrectionStart,
    DistortionCorrectionAvailableModes,
    DistortionCorrectionEnd,
    HEICAvailableHEICStreamConfigurations             = MetadataSectionHEICStart,
    HEICAvailableHEICMinFrameDurations,
    HEICAvailableHEICStallDurations,
    HEICEnd,
    HEICInfoSupported                                 = MetadataSectionHEICInfoStart,
    HEICInfoMaxJpegAppSegmentsCount,
    HEICInfoEnd,
#endif // Android-P or better

    InputColorCorrectionMode                          = ColorCorrectionMode                          | InputMetadataSectionMask,
    InputColorCorrectionTransform                     = ColorCorrectionTransform                     | InputMetadataSectionMask,
    InputColorCorrectionGains                         = ColorCorrectionGains                         | InputMetadataSectionMask,
    InputColorCorrectionAberrationMode                = ColorCorrectionAberrationMode                | InputMetadataSectionMask,
    InputColorCorrectionAvailableAberrationModes      = ColorCorrectionAvailableAberrationModes      | InputMetadataSectionMask,
    InputColorCorrectionEnd                           = ColorCorrectionEnd                           | InputMetadataSectionMask,

    InputControlAEAntibandingMode                     = ControlAEAntibandingMode                     | InputMetadataSectionMask,
    InputControlAEExposureCompensation                = ControlAEExposureCompensation                | InputMetadataSectionMask,
    InputControlAELock                                = ControlAELock                                | InputMetadataSectionMask,
    InputControlAEMode                                = ControlAEMode                                | InputMetadataSectionMask,
    InputControlAERegions                             = ControlAERegions                             | InputMetadataSectionMask,
    InputControlAETargetFpsRange                      = ControlAETargetFpsRange                      | InputMetadataSectionMask,
    InputControlAEPrecaptureTrigger                   = ControlAEPrecaptureTrigger                   | InputMetadataSectionMask,
    InputControlAFMode                                = ControlAFMode                                | InputMetadataSectionMask,
    InputControlAFRegions                             = ControlAFRegions                             | InputMetadataSectionMask,
    InputControlAFTrigger                             = ControlAFTrigger                             | InputMetadataSectionMask,
    InputControlAWBLock                               = ControlAWBLock                               | InputMetadataSectionMask,
    InputControlAWBMode                               = ControlAWBMode                               | InputMetadataSectionMask,
    InputControlAWBRegions                            = ControlAWBRegions                            | InputMetadataSectionMask,
    InputControlCaptureIntent                         = ControlCaptureIntent                         | InputMetadataSectionMask,
    InputControlEffectMode                            = ControlEffectMode                            | InputMetadataSectionMask,
    InputControlMode                                  = ControlMode                                  | InputMetadataSectionMask,
    InputControlSceneMode                             = ControlSceneMode                             | InputMetadataSectionMask,
    InputControlVideoStabilizationMode                = ControlVideoStabilizationMode                | InputMetadataSectionMask,
    InputControlAEAvailableAntibandingModes           = ControlAEAvailableAntibandingModes           | InputMetadataSectionMask,
    InputControlAEAvailableModes                      = ControlAEAvailableModes                      | InputMetadataSectionMask,
    InputControlAEAvailableTargetFPSRanges            = ControlAEAvailableTargetFPSRanges            | InputMetadataSectionMask,
    InputControlAECompensationRange                   = ControlAECompensationRange                   | InputMetadataSectionMask,
    InputControlAECompensationStep                    = ControlAECompensationStep                    | InputMetadataSectionMask,
    InputControlAFAvailableModes                      = ControlAFAvailableModes                      | InputMetadataSectionMask,
    InputControlAvailableEffects                      = ControlAvailableEffects                      | InputMetadataSectionMask,
    InputControlAvailableSceneModes                   = ControlAvailableSceneModes                   | InputMetadataSectionMask,
    InputControlAvailableVideoStabilizationModes      = ControlAvailableVideoStabilizationModes      | InputMetadataSectionMask,
    InputControlAWBAvailableModes                     = ControlAWBAvailableModes                     | InputMetadataSectionMask,
    InputControlMaxRegions                            = ControlMaxRegions                            | InputMetadataSectionMask,
    InputControlSceneModeOverrides                    = ControlSceneModeOverrides                    | InputMetadataSectionMask,
    InputControlAEPrecaptureId                        = ControlAEPrecaptureId                        | InputMetadataSectionMask,
    InputControlAEState                               = ControlAEState                               | InputMetadataSectionMask,
    InputControlAFState                               = ControlAFState                               | InputMetadataSectionMask,
    InputControlAFTriggerId                           = ControlAFTriggerId                           | InputMetadataSectionMask,
    InputControlAWBState                              = ControlAWBState                              | InputMetadataSectionMask,
    InputControlAvailableHighSpeedVideoConfigurations = ControlAvailableHighSpeedVideoConfigurations | InputMetadataSectionMask,
    InputControlAELockAvailable                       = ControlAELockAvailable                       | InputMetadataSectionMask,
    InputControlAWBLockAvailable                      = ControlAWBLockAvailable                      | InputMetadataSectionMask,
    InputControlAvailableModes                        = ControlAvailableModes                        | InputMetadataSectionMask,
    InputControlPostRawSensitivityBoostRange          = ControlPostRawSensitivityBoostRange          | InputMetadataSectionMask,
    InputControlPostRawSensitivityBoost               = ControlPostRawSensitivityBoost               | InputMetadataSectionMask,
    InputControlZslEnable                             = ControlZslEnable                             | InputMetadataSectionMask,
    InputControlEnd                                   = ControlEnd                                   | InputMetadataSectionMask,

    InputDemosaicMode                                 = DemosaicMode                                 | InputMetadataSectionMask,
    InputDemosaicEnd                                  = DemosaicEnd                                  | InputMetadataSectionMask,

    InputEdgeMode                                     = EdgeMode                                     | InputMetadataSectionMask,
    InputEdgeStrength                                 = EdgeStrength                                 | InputMetadataSectionMask,
    InputEdgeAvailableEdgeModes                       = EdgeAvailableEdgeModes                       | InputMetadataSectionMask,
    InputEdgeEnd                                      = EdgeEnd                                      | InputMetadataSectionMask,

    InputFlashFiringPower                             = FlashFiringPower                             | InputMetadataSectionMask,
    InputFlashFiringTime                              = FlashFiringTime                              | InputMetadataSectionMask,
    InputFlashMode                                    = FlashMode                                    | InputMetadataSectionMask,
    InputFlashColorTemperature                        = FlashColorTemperature                        | InputMetadataSectionMask,
    InputFlashMaxEnergy                               = FlashMaxEnergy                               | InputMetadataSectionMask,
    InputFlashState                                   = FlashState                                   | InputMetadataSectionMask,
    InputFlashEnd                                     = FlashEnd                                     | InputMetadataSectionMask,

    InputFlashInfoAvailable                           = FlashInfoAvailable                           | InputMetadataSectionMask,
    InputFlashInfoChargeDuration                      = FlashInfoChargeDuration                      | InputMetadataSectionMask,
    InputFlashInfoEnd                                 = FlashInfoEnd                                 | InputMetadataSectionMask,

    InputHotPixelMode                                 = HotPixelMode                                 | InputMetadataSectionMask,
    InputHotPixelAvailableHotPixelModes               = HotPixelAvailableHotPixelModes               | InputMetadataSectionMask,
    InputHotPixelEnd                                  = HotPixelEnd                                  | InputMetadataSectionMask,

    InputJPEGGpsCoordinates                           = JPEGGpsCoordinates                           | InputMetadataSectionMask,
    InputJPEGGpsProcessingMethod                      = JPEGGpsProcessingMethod                      | InputMetadataSectionMask,
    InputJPEGGpsTimestamp                             = JPEGGpsTimestamp                             | InputMetadataSectionMask,
    InputJPEGOrientation                              = JPEGOrientation                              | InputMetadataSectionMask,
    InputJPEGQuality                                  = JPEGQuality                                  | InputMetadataSectionMask,
    InputJPEGThumbnailQuality                         = JPEGThumbnailQuality                         | InputMetadataSectionMask,
    InputJPEGThumbnailSize                            = JPEGThumbnailSize                            | InputMetadataSectionMask,
    InputJPEGAvailableThumbnailSizes                  = JPEGAvailableThumbnailSizes                  | InputMetadataSectionMask,
    InputJPEGMaxSize                                  = JPEGMaxSize                                  | InputMetadataSectionMask,
    InputJPEGSize                                     = JPEGSize                                     | InputMetadataSectionMask,
    InputJPEGEnd                                      = JPEGEnd                                      | InputMetadataSectionMask,

    InputLensAperture                                 = LensAperture                                 | InputMetadataSectionMask,
    InputLensFilterDensity                            = LensFilterDensity                            | InputMetadataSectionMask,
    InputLensFocalLength                              = LensFocalLength                              | InputMetadataSectionMask,
    InputLensFocusDistance                            = LensFocusDistance                            | InputMetadataSectionMask,
    InputLensOpticalStabilizationMode                 = LensOpticalStabilizationMode                 | InputMetadataSectionMask,
    InputLensFacing                                   = LensFacing                                   | InputMetadataSectionMask,
    InputLensPoseRotation                             = LensPoseRotation                             | InputMetadataSectionMask,
    InputLensPoseTranslation                          = LensPoseTranslation                          | InputMetadataSectionMask,
    InputLensFocusRange                               = LensFocusRange                               | InputMetadataSectionMask,
    InputLensState                                    = LensState                                    | InputMetadataSectionMask,
    InputLensIntrinsicCalibration                     = LensIntrinsicCalibration                     | InputMetadataSectionMask,
    InputLensRadialDistortion                         = LensRadialDistortion                         | InputMetadataSectionMask,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    InputLensPoseReference                            = LensPoseReference                            | InputMetadataSectionMask,
    InputLensDistortion                               = LensDistortion                               | InputMetadataSectionMask,
#endif // Android-P or better
    InputLensEnd                                      = LensEnd                                      | InputMetadataSectionMask,

    InputLensInfoAvailableApertures                   = LensInfoAvailableApertures                   | InputMetadataSectionMask,
    InputLensInfoAvailableFilterDensities             = LensInfoAvailableFilterDensities             | InputMetadataSectionMask,
    InputLensInfoAvailableFocalLengths                = LensInfoAvailableFocalLengths                | InputMetadataSectionMask,
    InputLensInfoAvailableOpticalStabilization        = LensInfoAvailableOpticalStabilization        | InputMetadataSectionMask,
    InputLensInfoHyperfocalDistance                   = LensInfoHyperfocalDistance                   | InputMetadataSectionMask,
    InputLensInfoMinimumFocusDistance                 = LensInfoMinimumFocusDistance                 | InputMetadataSectionMask,
    InputLensInfoShadingMapSize                       = LensInfoShadingMapSize                       | InputMetadataSectionMask,
    InputLensInfoFocusDistanceCalibration             = LensInfoFocusDistanceCalibration             | InputMetadataSectionMask,
    InputLensInfoEnd                                  = LensInfoEnd                                  | InputMetadataSectionMask,

    InputNoiseReductionMode                           = NoiseReductionMode                           | InputMetadataSectionMask,
    InputNoiseReductionStrength                       = NoiseReductionStrength                       | InputMetadataSectionMask,
    InputNoiseReductionAvailableNoiseReductionModes   = NoiseReductionAvailableNoiseReductionModes   | InputMetadataSectionMask,
    InputNoiseReductionEnd                            = NoiseReductionEnd                            | InputMetadataSectionMask,

    InputQuirksMeteringCropRegion                     = QuirksMeteringCropRegion                     | InputMetadataSectionMask,
    InputQuirksTriggerAFWithAuto                      = QuirksTriggerAFWithAuto                      | InputMetadataSectionMask,
    InputQuirksUseZslFormat                           = QuirksUseZslFormat                           | InputMetadataSectionMask,
    InputQuirksUsePartialResult                       = QuirksUsePartialResult                       | InputMetadataSectionMask,
    InputQuirksPartialResult                          = QuirksPartialResult                          | InputMetadataSectionMask,
    InputQuirksEnd                                    = QuirksEnd                                    | InputMetadataSectionMask,

    InputRequestFrameCount                            = RequestFrameCount                            | InputMetadataSectionMask,
    InputRequestId                                    = RequestId                                    | InputMetadataSectionMask,
    InputRequestInputStreams                          = RequestInputStreams                          | InputMetadataSectionMask,
    InputRequestMetadataMode                          = RequestMetadataMode                          | InputMetadataSectionMask,
    InputRequestOutputStreams                         = RequestOutputStreams                         | InputMetadataSectionMask,
    InputRequestType                                  = RequestType                                  | InputMetadataSectionMask,
    InputRequestMaxNumOutputStreams                   = RequestMaxNumOutputStreams                   | InputMetadataSectionMask,
    InputRequestMaxNumReprocessStreams                = RequestMaxNumReprocessStreams                | InputMetadataSectionMask,
    InputRequestMaxNumInputStreams                    = RequestMaxNumInputStreams                    | InputMetadataSectionMask,
    InputRequestPipelineDepth                         = RequestPipelineDepth                         | InputMetadataSectionMask,
    InputRequestPipelineMaxDepth                      = RequestPipelineMaxDepth                      | InputMetadataSectionMask,
    InputRequestPartialResultCount                    = RequestPartialResultCount                    | InputMetadataSectionMask,
    InputRequestAvailableCapabilities                 = RequestAvailableCapabilities                 | InputMetadataSectionMask,
    InputRequestAvailableRequestKeys                  = RequestAvailableRequestKeys                  | InputMetadataSectionMask,
    InputRequestAvailableResultKeys                   = RequestAvailableResultKeys                   | InputMetadataSectionMask,
    InputRequestAvailableCharacteristicsKeys          = RequestAvailableCharacteristicsKeys          | InputMetadataSectionMask,
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    InputRequestAvailableSessionKeys                  = RequestAvailableSessionKeys                  | InputMetadataSectionMask,
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
    InputRequestEnd                                   = RequestEnd                                   | InputMetadataSectionMask,

    InputScalerCropRegion                             = ScalerCropRegion                             | InputMetadataSectionMask,
    InputScalerAvailableFormats                       = ScalerAvailableFormats                       | InputMetadataSectionMask,
    InputScalerAvailableJPEGMinDurations              = ScalerAvailableJPEGMinDurations              | InputMetadataSectionMask,
    InputScalerAvailableJPEGSizes                     = ScalerAvailableJPEGSizes                     | InputMetadataSectionMask,
    InputScalerAvailableMaxDigitalZoom                = ScalerAvailableMaxDigitalZoom                | InputMetadataSectionMask,
    InputScalerAvailableProcessedMinDurations         = ScalerAvailableProcessedMinDurations         | InputMetadataSectionMask,
    InputScalerAvailableProcessedSizes                = ScalerAvailableProcessedSizes                | InputMetadataSectionMask,
    InputScalerAvailableRawMinDurations               = ScalerAvailableRawMinDurations               | InputMetadataSectionMask,
    InputScalerAvailableRawSizes                      = ScalerAvailableRawSizes                      | InputMetadataSectionMask,
    InputScalerAvailableInputOutputFormatsMap         = ScalerAvailableInputOutputFormatsMap         | InputMetadataSectionMask,
    InputScalerAvailableStreamConfigurations          = ScalerAvailableStreamConfigurations          | InputMetadataSectionMask,
    InputScalerAvailableMinFrameDurations             = ScalerAvailableMinFrameDurations             | InputMetadataSectionMask,
    InputScalerAvailableStallDurations                = ScalerAvailableStallDurations                | InputMetadataSectionMask,
    InputScalerCroppingType                           = ScalerCroppingType                           | InputMetadataSectionMask,
    InputScalerEnd                                    = ScalerEnd                                    | InputMetadataSectionMask,

    InputSensorExposureTime                           = SensorExposureTime                           | InputMetadataSectionMask,
    InputSensorFrameDuration                          = SensorFrameDuration                          | InputMetadataSectionMask,
    InputSensorSensitivity                            = SensorSensitivity                            | InputMetadataSectionMask,
    InputSensorReferenceIlluminant1                   = SensorReferenceIlluminant1                   | InputMetadataSectionMask,
    InputSensorReferenceIlluminant2                   = SensorReferenceIlluminant2                   | InputMetadataSectionMask,
    InputSensorCalibrationTransform1                  = SensorCalibrationTransform1                  | InputMetadataSectionMask,
    InputSensorCalibrationTransform2                  = SensorCalibrationTransform2                  | InputMetadataSectionMask,
    InputSensorColorTransform1                        = SensorColorTransform1                        | InputMetadataSectionMask,
    InputSensorColorTransform2                        = SensorColorTransform2                        | InputMetadataSectionMask,
    InputSensorForwardMatrix1                         = SensorForwardMatrix1                         | InputMetadataSectionMask,
    InputSensorForwardMatrix2                         = SensorForwardMatrix2                         | InputMetadataSectionMask,
    InputSensorBaseGainFactor                         = SensorBaseGainFactor                         | InputMetadataSectionMask,
    InputSensorBlackLevelPattern                      = SensorBlackLevelPattern                      | InputMetadataSectionMask,
    InputSensorMaxAnalogSensitivity                   = SensorMaxAnalogSensitivity                   | InputMetadataSectionMask,
    InputSensorOrientation                            = SensorOrientation                            | InputMetadataSectionMask,
    InputSensorProfileHueSaturationMapDimensions      = SensorProfileHueSaturationMapDimensions      | InputMetadataSectionMask,
    InputSensorTimestamp                              = SensorTimestamp                              | InputMetadataSectionMask,
    InputSensorTemperature                            = SensorTemperature                            | InputMetadataSectionMask,
    InputSensorNeutralColorPoint                      = SensorNeutralColorPoint                      | InputMetadataSectionMask,
    InputSensorNoiseProfile                           = SensorNoiseProfile                           | InputMetadataSectionMask,
    InputSensorProfileHueSaturationMap                = SensorProfileHueSaturationMap                | InputMetadataSectionMask,
    InputSensorProfileToneCurve                       = SensorProfileToneCurve                       | InputMetadataSectionMask,
    InputSensorGreenSplit                             = SensorGreenSplit                             | InputMetadataSectionMask,
    InputSensorTestPatternData                        = SensorTestPatternData                        | InputMetadataSectionMask,
    InputSensorTestPatternMode                        = SensorTestPatternMode                        | InputMetadataSectionMask,
    InputSensorAvailableTestPatternModes              = SensorAvailableTestPatternModes              | InputMetadataSectionMask,
    InputSensorRollingShutterSkew                     = SensorRollingShutterSkew                     | InputMetadataSectionMask,
    InputSensorDynamicBlackLevel                      = SensorDynamicBlackLevel                      | InputMetadataSectionMask,
    InputSensorDynamicWhiteLevel                      = SensorDynamicWhiteLevel                      | InputMetadataSectionMask,
    InputSensorEnd                                    = SensorEnd                                    | InputMetadataSectionMask,

    InputSensorInfoActiveArraySize                    = SensorInfoActiveArraySize                    | InputMetadataSectionMask,
    InputSensorInfoSensitivityRange                   = SensorInfoSensitivityRange                   | InputMetadataSectionMask,
    InputSensorInfoColorFilterArrangement             = SensorInfoColorFilterArrangement             | InputMetadataSectionMask,
    InputSensorInfoExposureTimeRange                  = SensorInfoExposureTimeRange                  | InputMetadataSectionMask,
    InputSensorInfoMaxFrameDuration                   = SensorInfoMaxFrameDuration                   | InputMetadataSectionMask,
    InputSensorInfoPhysicalSize                       = SensorInfoPhysicalSize                       | InputMetadataSectionMask,
    InputSensorInfoPixelArraySize                     = SensorInfoPixelArraySize                     | InputMetadataSectionMask,
    InputSensorInfoWhiteLevel                         = SensorInfoWhiteLevel                         | InputMetadataSectionMask,
    InputSensorInfoTimestampSource                    = SensorInfoTimestampSource                    | InputMetadataSectionMask,
    InputSensorInfoLensShadingApplied                 = SensorInfoLensShadingApplied                 | InputMetadataSectionMask,
    InputSensorInfoPreCorrectionActiveArraySize       = SensorInfoPreCorrectionActiveArraySize       | InputMetadataSectionMask,
    InputSensorInfoEnd                                = SensorInfoEnd                                | InputMetadataSectionMask,

    InputShadingMode                                  = ShadingMode                                  | InputMetadataSectionMask,
    InputShadingStrength                              = ShadingStrength                              | InputMetadataSectionMask,
    InputShadingAvailableModes                        = ShadingAvailableModes                        | InputMetadataSectionMask,
    InputShadingEnd                                   = ShadingEnd                                   | InputMetadataSectionMask,

    InputStatisticsFaceDetectMode                     = StatisticsFaceDetectMode                     | InputMetadataSectionMask,
    InputStatisticsHistogramMode                      = StatisticsHistogramMode                      | InputMetadataSectionMask,
    InputStatisticsSharpnessMapMode                   = StatisticsSharpnessMapMode                   | InputMetadataSectionMask,
    InputStatisticsHotPixelMapMode                    = StatisticsHotPixelMapMode                    | InputMetadataSectionMask,
    InputStatisticsFaceIds                            = StatisticsFaceIds                            | InputMetadataSectionMask,
    InputStatisticsFaceLandmarks                      = StatisticsFaceLandmarks                      | InputMetadataSectionMask,
    InputStatisticsFaceRectangles                     = StatisticsFaceRectangles                     | InputMetadataSectionMask,
    InputStatisticsFaceScores                         = StatisticsFaceScores                         | InputMetadataSectionMask,
    InputStatisticsHistogram                          = StatisticsHistogram                          | InputMetadataSectionMask,
    InputStatisticsSharpnessMap                       = StatisticsSharpnessMap                       | InputMetadataSectionMask,
    InputStatisticsLensShadingCorrectionMap           = StatisticsLensShadingCorrectionMap           | InputMetadataSectionMask,
    InputStatisticsLensShadingMap                     = StatisticsLensShadingMap                     | InputMetadataSectionMask,
    InputStatisticsPredictedColorGains                = StatisticsPredictedColorGains                | InputMetadataSectionMask,
    InputStatisticsPredictedColorTransform            = StatisticsPredictedColorTransform            | InputMetadataSectionMask,
    InputStatisticsSceneFlicker                       = StatisticsSceneFlicker                       | InputMetadataSectionMask,
    InputStatisticsHotPixelMap                        = StatisticsHotPixelMap                        | InputMetadataSectionMask,
    InputStatisticsLensShadingMapMode                 = StatisticsLensShadingMapMode                 | InputMetadataSectionMask,
    InputStatisticsEnd                                = StatisticsEnd                                | InputMetadataSectionMask,

    InputStatisticsInfoAvailableFaceDetectModes       = StatisticsInfoAvailableFaceDetectModes       | InputMetadataSectionMask,
    InputStatisticsInfoHistogramBucketCount           = StatisticsInfoHistogramBucketCount           | InputMetadataSectionMask,
    InputStatisticsInfoMaxFaceCount                   = StatisticsInfoMaxFaceCount                   | InputMetadataSectionMask,
    InputStatisticsInfoMaxHistogramCount              = StatisticsInfoMaxHistogramCount              | InputMetadataSectionMask,
    InputStatisticsInfoMaxSharpnessMapValue           = StatisticsInfoMaxSharpnessMapValue           | InputMetadataSectionMask,
    InputStatisticsInfoSharpnessMapSize               = StatisticsInfoSharpnessMapSize               | InputMetadataSectionMask,
    InputStatisticsInfoAvailableHotPixelMapModes      = StatisticsInfoAvailableHotPixelMapModes      | InputMetadataSectionMask,
    InputStatisticsInfoAvailableLensShadingMapModes   = StatisticsInfoAvailableLensShadingMapModes   | InputMetadataSectionMask,
    InputStatisticsInfoEnd                            = StatisticsInfoEnd                            | InputMetadataSectionMask,

    InputTonemapCurveBlue                             = TonemapCurveBlue                             | InputMetadataSectionMask,
    InputTonemapCurveGreen                            = TonemapCurveGreen                            | InputMetadataSectionMask,
    InputTonemapCurveRed                              = TonemapCurveRed                              | InputMetadataSectionMask,
    InputTonemapMode                                  = TonemapMode                                  | InputMetadataSectionMask,
    InputTonemapMaxCurvePoints                        = TonemapMaxCurvePoints                        | InputMetadataSectionMask,
    InputTonemapAvailableToneMapModes                 = TonemapAvailableToneMapModes                 | InputMetadataSectionMask,
    InputTonemapGamma                                 = TonemapGamma                                 | InputMetadataSectionMask,
    InputTonemapPresetCurve                           = TonemapPresetCurve                           | InputMetadataSectionMask,
    InputTonemapEnd                                   = TonemapEnd                                   | InputMetadataSectionMask,

    InputLedTransmit                                  = LedTransmit                                  | InputMetadataSectionMask,
    InputLedAvailableLeds                             = LedAvailableLeds                             | InputMetadataSectionMask,
    InputLedEnd                                       = LedEnd                                       | InputMetadataSectionMask,

    InputInfoSupportedHardwareLevel                   = InfoSupportedHardwareLevel                   | InputMetadataSectionMask,
    InputInfoEnd                                      = InfoEnd                                      | InputMetadataSectionMask,

    InputBlackLevelLock                               = BlackLevelLock                               | InputMetadataSectionMask,
    InputBlackLevelEnd                                = BlackLevelEnd                                | InputMetadataSectionMask,

    InputSyncFrameNumber                              = SyncFrameNumber                              | InputMetadataSectionMask,
    InputSyncMaxLatency                               = SyncMaxLatency                               | InputMetadataSectionMask,
    InputSyncEnd                                      = SyncEnd                                      | InputMetadataSectionMask,

    InputReprocessEffectiveExposureFactor             = ReprocessEffectiveExposureFactor             | InputMetadataSectionMask,
    InputReprocessMaxCaptureStall                     = ReprocessMaxCaptureStall                     | InputMetadataSectionMask,
    InputReprocessEnd                                 = ReprocessEnd                                 | InputMetadataSectionMask,

    InputDepthMaxDepthSamples                         = DepthMaxDepthSamples                         | InputMetadataSectionMask,
    InputDepthAvailableDepthStreamConfigurations      = DepthAvailableDepthStreamConfigurations      | InputMetadataSectionMask,
    InputDepthAvailableDepthMinFrameDurations         = DepthAvailableDepthMinFrameDurations         | InputMetadataSectionMask,
    InputDepthAvailableDepthStallDurations            = DepthAvailableDepthStallDurations            | InputMetadataSectionMask,
    InputDepthDepthIsExclusive                        = DepthDepthIsExclusive                        | InputMetadataSectionMask,
    InputDepthEnd                                     = DepthEnd                                     | InputMetadataSectionMask,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    InputLogicalMultiCameraPhysicalIDs                = LogicalMultiCameraPhysicalIDs                | InputMetadataSectionMask,
    InputLogicalMultiCameraSensorSyncType             = LogicalMultiCameraSensorSyncType             |InputMetadataSectionMask,
    InputLogicalMultiCameraEnd                        = LogicalMultiCameraEnd                        | InputMetadataSectionMask,
#endif // Android-P or better
    UsecaseColorCorrectionMode                          = ColorCorrectionMode                          | UsecaseMetadataSectionMask,
    UsecaseColorCorrectionTransform                     = ColorCorrectionTransform                     | UsecaseMetadataSectionMask,
    UsecaseColorCorrectionGains                         = ColorCorrectionGains                         | UsecaseMetadataSectionMask,
    UsecaseColorCorrectionAberrationMode                = ColorCorrectionAberrationMode                | UsecaseMetadataSectionMask,
    UsecaseColorCorrectionAvailableAberrationModes      = ColorCorrectionAvailableAberrationModes      | UsecaseMetadataSectionMask,
    UsecaseColorCorrectionEnd                           = ColorCorrectionEnd                           | UsecaseMetadataSectionMask,

    UsecaseControlAEAntibandingMode                     = ControlAEAntibandingMode                     | UsecaseMetadataSectionMask,
    UsecaseControlAEExposureCompensation                = ControlAEExposureCompensation                | UsecaseMetadataSectionMask,
    UsecaseControlAELock                                = ControlAELock                                | UsecaseMetadataSectionMask,
    UsecaseControlAEMode                                = ControlAEMode                                | UsecaseMetadataSectionMask,
    UsecaseControlAERegions                             = ControlAERegions                             | UsecaseMetadataSectionMask,
    UsecaseControlAETargetFpsRange                      = ControlAETargetFpsRange                      | UsecaseMetadataSectionMask,
    UsecaseControlAEPrecaptureTrigger                   = ControlAEPrecaptureTrigger                   | UsecaseMetadataSectionMask,
    UsecaseControlAFMode                                = ControlAFMode                                | UsecaseMetadataSectionMask,
    UsecaseControlAFRegions                             = ControlAFRegions                             | UsecaseMetadataSectionMask,
    UsecaseControlAFTrigger                             = ControlAFTrigger                             | UsecaseMetadataSectionMask,
    UsecaseControlAWBLock                               = ControlAWBLock                               | UsecaseMetadataSectionMask,
    UsecaseControlAWBMode                               = ControlAWBMode                               | UsecaseMetadataSectionMask,
    UsecaseControlAWBRegions                            = ControlAWBRegions                            | UsecaseMetadataSectionMask,
    UsecaseControlCaptureIntent                         = ControlCaptureIntent                         | UsecaseMetadataSectionMask,
    UsecaseControlEffectMode                            = ControlEffectMode                            | UsecaseMetadataSectionMask,
    UsecaseControlMode                                  = ControlMode                                  | UsecaseMetadataSectionMask,
    UsecaseControlSceneMode                             = ControlSceneMode                             | UsecaseMetadataSectionMask,
    UsecaseControlVideoStabilizationMode                = ControlVideoStabilizationMode                | UsecaseMetadataSectionMask,
    UsecaseControlAEAvailableAntibandingModes           = ControlAEAvailableAntibandingModes           | UsecaseMetadataSectionMask,
    UsecaseControlAEAvailableModes                      = ControlAEAvailableModes                      | UsecaseMetadataSectionMask,
    UsecaseControlAEAvailableTargetFPSRanges            = ControlAEAvailableTargetFPSRanges            | UsecaseMetadataSectionMask,
    UsecaseControlAECompensationRange                   = ControlAECompensationRange                   | UsecaseMetadataSectionMask,
    UsecaseControlAECompensationStep                    = ControlAECompensationStep                    | UsecaseMetadataSectionMask,
    UsecaseControlAFAvailableModes                      = ControlAFAvailableModes                      | UsecaseMetadataSectionMask,
    UsecaseControlAvailableEffects                      = ControlAvailableEffects                      | UsecaseMetadataSectionMask,
    UsecaseControlAvailableSceneModes                   = ControlAvailableSceneModes                   | UsecaseMetadataSectionMask,
    UsecaseControlAvailableVideoStabilizationModes      = ControlAvailableVideoStabilizationModes      | UsecaseMetadataSectionMask,
    UsecaseControlAWBAvailableModes                     = ControlAWBAvailableModes                     | UsecaseMetadataSectionMask,
    UsecaseControlMaxRegions                            = ControlMaxRegions                            | UsecaseMetadataSectionMask,
    UsecaseControlSceneModeOverrides                    = ControlSceneModeOverrides                    | UsecaseMetadataSectionMask,
    UsecaseControlAEPrecaptureId                        = ControlAEPrecaptureId                        | UsecaseMetadataSectionMask,
    UsecaseControlAEState                               = ControlAEState                               | UsecaseMetadataSectionMask,
    UsecaseControlAFState                               = ControlAFState                               | UsecaseMetadataSectionMask,
    UsecaseControlAFTriggerId                           = ControlAFTriggerId                           | UsecaseMetadataSectionMask,
    UsecaseControlAWBState                              = ControlAWBState                              | UsecaseMetadataSectionMask,
    UsecaseControlAvailableHighSpeedVideoConfigurations = ControlAvailableHighSpeedVideoConfigurations | UsecaseMetadataSectionMask,
    UsecaseControlAELockAvailable                       = ControlAELockAvailable                       | UsecaseMetadataSectionMask,
    UsecaseControlAWBLockAvailable                      = ControlAWBLockAvailable                      | UsecaseMetadataSectionMask,
    UsecaseControlAvailableModes                        = ControlAvailableModes                        | UsecaseMetadataSectionMask,
    UsecaseControlPostRawSensitivityBoostRange          = ControlPostRawSensitivityBoostRange          | UsecaseMetadataSectionMask,
    UsecaseControlPostRawSensitivityBoost               = ControlPostRawSensitivityBoost               | UsecaseMetadataSectionMask,
    UsecaseControlEnd                                   = ControlEnd                                   | UsecaseMetadataSectionMask,

    UsecaseDemosaicMode                                 = DemosaicMode                                 | UsecaseMetadataSectionMask,
    UsecaseDemosaicEnd                                  = DemosaicEnd                                  | UsecaseMetadataSectionMask,

    UsecaseEdgeMode                                     = EdgeMode                                     | UsecaseMetadataSectionMask,
    UsecaseEdgeStrength                                 = EdgeStrength                                 | UsecaseMetadataSectionMask,
    UsecaseEdgeAvailableEdgeModes                       = EdgeAvailableEdgeModes                       | UsecaseMetadataSectionMask,
    UsecaseEdgeEnd                                      = EdgeEnd                                      | UsecaseMetadataSectionMask,

    UsecaseFlashFiringPower                             = FlashFiringPower                             | UsecaseMetadataSectionMask,
    UsecaseFlashFiringTime                              = FlashFiringTime                              | UsecaseMetadataSectionMask,
    UsecaseFlashMode                                    = FlashMode                                    | UsecaseMetadataSectionMask,
    UsecaseFlashColorTemperature                        = FlashColorTemperature                        | UsecaseMetadataSectionMask,
    UsecaseFlashMaxEnergy                               = FlashMaxEnergy                               | UsecaseMetadataSectionMask,
    UsecaseFlashState                                   = FlashState                                   | UsecaseMetadataSectionMask,
    UsecaseFlashEnd                                     = FlashEnd                                     | UsecaseMetadataSectionMask,

    UsecaseFlashInfoAvailable                           = FlashInfoAvailable                           | UsecaseMetadataSectionMask,
    UsecaseFlashInfoChargeDuration                      = FlashInfoChargeDuration                      | UsecaseMetadataSectionMask,
    UsecaseFlashInfoEnd                                 = FlashInfoEnd                                 | UsecaseMetadataSectionMask,

    UsecaseHotPixelMode                                 = HotPixelMode                                 | UsecaseMetadataSectionMask,
    UsecaseHotPixelAvailableHotPixelModes               = HotPixelAvailableHotPixelModes               | UsecaseMetadataSectionMask,
    UsecaseHotPixelEnd                                  = HotPixelEnd                                  | UsecaseMetadataSectionMask,

    UsecaseJPEGGpsCoordinates                           = JPEGGpsCoordinates                           | UsecaseMetadataSectionMask,
    UsecaseJPEGGpsProcessingMethod                      = JPEGGpsProcessingMethod                      | UsecaseMetadataSectionMask,
    UsecaseJPEGGpsTimestamp                             = JPEGGpsTimestamp                             | UsecaseMetadataSectionMask,
    UsecaseJPEGOrientation                              = JPEGOrientation                              | UsecaseMetadataSectionMask,
    UsecaseJPEGQuality                                  = JPEGQuality                                  | UsecaseMetadataSectionMask,
    UsecaseJPEGThumbnailQuality                         = JPEGThumbnailQuality                         | UsecaseMetadataSectionMask,
    UsecaseJPEGThumbnailSize                            = JPEGThumbnailSize                            | UsecaseMetadataSectionMask,
    UsecaseJPEGAvailableThumbnailSizes                  = JPEGAvailableThumbnailSizes                  | UsecaseMetadataSectionMask,
    UsecaseJPEGMaxSize                                  = JPEGMaxSize                                  | UsecaseMetadataSectionMask,
    UsecaseJPEGSize                                     = JPEGSize                                     | UsecaseMetadataSectionMask,
    UsecaseJPEGEnd                                      = JPEGEnd                                      | UsecaseMetadataSectionMask,

    UsecaseLensAperture                                 = LensAperture                                 | UsecaseMetadataSectionMask,
    UsecaseLensFilterDensity                            = LensFilterDensity                            | UsecaseMetadataSectionMask,
    UsecaseLensFocalLength                              = LensFocalLength                              | UsecaseMetadataSectionMask,
    UsecaseLensFocusDistance                            = LensFocusDistance                            | UsecaseMetadataSectionMask,
    UsecaseLensOpticalStabilizationMode                 = LensOpticalStabilizationMode                 | UsecaseMetadataSectionMask,
    UsecaseLensFacing                                   = LensFacing                                   | UsecaseMetadataSectionMask,
    UsecaseLensPoseRotation                             = LensPoseRotation                             | UsecaseMetadataSectionMask,
    UsecaseLensPoseTranslation                          = LensPoseTranslation                          | UsecaseMetadataSectionMask,
    UsecaseLensFocusRange                               = LensFocusRange                               | UsecaseMetadataSectionMask,
    UsecaseLensState                                    = LensState                                    | UsecaseMetadataSectionMask,
    UsecaseLensIntrinsicCalibration                     = LensIntrinsicCalibration                     | UsecaseMetadataSectionMask,
    UsecaseLensRadialDistortion                         = LensRadialDistortion                         | UsecaseMetadataSectionMask,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    UsecaseLensPoseReference                            = LensPoseReference                            | UsecaseMetadataSectionMask,
    UsecaseLensDistortion                               = LensDistortion                               | UsecaseMetadataSectionMask,
#endif // Android-P or better
    UsecaseLensEnd                                      = LensEnd                                      | UsecaseMetadataSectionMask,

    UsecaseLensInfoAvailableApertures                   = LensInfoAvailableApertures                   | UsecaseMetadataSectionMask,
    UsecaseLensInfoAvailableFilterDensities             = LensInfoAvailableFilterDensities             | UsecaseMetadataSectionMask,
    UsecaseLensInfoAvailableFocalLengths                = LensInfoAvailableFocalLengths                | UsecaseMetadataSectionMask,
    UsecaseLensInfoAvailableOpticalStabilization        = LensInfoAvailableOpticalStabilization        | UsecaseMetadataSectionMask,
    UsecaseLensInfoHyperfocalDistance                   = LensInfoHyperfocalDistance                   | UsecaseMetadataSectionMask,
    UsecaseLensInfoMinimumFocusDistance                 = LensInfoMinimumFocusDistance                 | UsecaseMetadataSectionMask,
    UsecaseLensInfoShadingMapSize                       = LensInfoShadingMapSize                       | UsecaseMetadataSectionMask,
    UsecaseLensInfoFocusDistanceCalibration             = LensInfoFocusDistanceCalibration             | UsecaseMetadataSectionMask,
    UsecaseLensInfoEnd                                  = LensInfoEnd                                  | UsecaseMetadataSectionMask,

    UsecaseNoiseReductionMode                           = NoiseReductionMode                           | UsecaseMetadataSectionMask,
    UsecaseNoiseReductionStrength                       = NoiseReductionStrength                       | UsecaseMetadataSectionMask,
    UsecaseNoiseReductionAvailableNoiseReductionModes   = NoiseReductionAvailableNoiseReductionModes   | UsecaseMetadataSectionMask,
    UsecaseNoiseReductionEnd                            = NoiseReductionEnd                            | UsecaseMetadataSectionMask,

    UsecaseQuirksMeteringCropRegion                     = QuirksMeteringCropRegion                     | UsecaseMetadataSectionMask,
    UsecaseQuirksTriggerAFWithAuto                      = QuirksTriggerAFWithAuto                      | UsecaseMetadataSectionMask,
    UsecaseQuirksUseZslFormat                           = QuirksUseZslFormat                           | UsecaseMetadataSectionMask,
    UsecaseQuirksUsePartialResult                       = QuirksUsePartialResult                       | UsecaseMetadataSectionMask,
    UsecaseQuirksPartialResult                          = QuirksPartialResult                          | UsecaseMetadataSectionMask,
    UsecaseQuirksEnd                                    = QuirksEnd                                    | UsecaseMetadataSectionMask,

    UsecaseRequestFrameCount                            = RequestFrameCount                            | UsecaseMetadataSectionMask,
    UsecaseRequestId                                    = RequestId                                    | UsecaseMetadataSectionMask,
    UsecaseRequestInputStreams                          = RequestInputStreams                          | UsecaseMetadataSectionMask,
    UsecaseRequestMetadataMode                          = RequestMetadataMode                          | UsecaseMetadataSectionMask,
    UsecaseRequestOutputStreams                         = RequestOutputStreams                         | UsecaseMetadataSectionMask,
    UsecaseRequestType                                  = RequestType                                  | UsecaseMetadataSectionMask,
    UsecaseRequestMaxNumOutputStreams                   = RequestMaxNumOutputStreams                   | UsecaseMetadataSectionMask,
    UsecaseRequestMaxNumReprocessStreams                = RequestMaxNumReprocessStreams                | UsecaseMetadataSectionMask,
    UsecaseRequestMaxNumInputStreams                    = RequestMaxNumInputStreams                    | UsecaseMetadataSectionMask,
    UsecaseRequestPipelineDepth                         = RequestPipelineDepth                         | UsecaseMetadataSectionMask,
    UsecaseRequestPipelineMaxDepth                      = RequestPipelineMaxDepth                      | UsecaseMetadataSectionMask,
    UsecaseRequestPartialResultCount                    = RequestPartialResultCount                    | UsecaseMetadataSectionMask,
    UsecaseRequestAvailableCapabilities                 = RequestAvailableCapabilities                 | UsecaseMetadataSectionMask,
    UsecaseRequestAvailableRequestKeys                  = RequestAvailableRequestKeys                  | UsecaseMetadataSectionMask,
    UsecaseRequestAvailableResultKeys                   = RequestAvailableResultKeys                   | UsecaseMetadataSectionMask,
    UsecaseRequestAvailableCharacteristicsKeys          = RequestAvailableCharacteristicsKeys          | UsecaseMetadataSectionMask,
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    UsecaseRequestAvailableSessionKeys                  = RequestAvailableSessionKeys                  | UsecaseMetadataSectionMask,
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
    UsecaseRequestEnd                                   = RequestEnd                                   | UsecaseMetadataSectionMask,

    UsecaseScalerCropRegion                             = ScalerCropRegion                             | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableFormats                       = ScalerAvailableFormats                       | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableJPEGMinDurations              = ScalerAvailableJPEGMinDurations              | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableJPEGSizes                     = ScalerAvailableJPEGSizes                     | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableMaxDigitalZoom                = ScalerAvailableMaxDigitalZoom                | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableProcessedMinDurations         = ScalerAvailableProcessedMinDurations         | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableProcessedSizes                = ScalerAvailableProcessedSizes                | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableRawMinDurations               = ScalerAvailableRawMinDurations               | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableRawSizes                      = ScalerAvailableRawSizes                      | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableInputOutputFormatsMap         = ScalerAvailableInputOutputFormatsMap         | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableStreamConfigurations          = ScalerAvailableStreamConfigurations          | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableMinFrameDurations             = ScalerAvailableMinFrameDurations             | UsecaseMetadataSectionMask,
    UsecaseScalerAvailableStallDurations                = ScalerAvailableStallDurations                | UsecaseMetadataSectionMask,
    UsecaseScalerCroppingType                           = ScalerCroppingType                           | UsecaseMetadataSectionMask,
    UsecaseScalerEnd                                    = ScalerEnd                                    | UsecaseMetadataSectionMask,

    UsecaseSensorExposureTime                           = SensorExposureTime                           | UsecaseMetadataSectionMask,
    UsecaseSensorFrameDuration                          = SensorFrameDuration                          | UsecaseMetadataSectionMask,
    UsecaseSensorSensitivity                            = SensorSensitivity                            | UsecaseMetadataSectionMask,
    UsecaseSensorReferenceIlluminant1                   = SensorReferenceIlluminant1                   | UsecaseMetadataSectionMask,
    UsecaseSensorReferenceIlluminant2                   = SensorReferenceIlluminant2                   | UsecaseMetadataSectionMask,
    UsecaseSensorCalibrationTransform1                  = SensorCalibrationTransform1                  | UsecaseMetadataSectionMask,
    UsecaseSensorCalibrationTransform2                  = SensorCalibrationTransform2                  | UsecaseMetadataSectionMask,
    UsecaseSensorColorTransform1                        = SensorColorTransform1                        | UsecaseMetadataSectionMask,
    UsecaseSensorColorTransform2                        = SensorColorTransform2                        | UsecaseMetadataSectionMask,
    UsecaseSensorForwardMatrix1                         = SensorForwardMatrix1                         | UsecaseMetadataSectionMask,
    UsecaseSensorForwardMatrix2                         = SensorForwardMatrix2                         | UsecaseMetadataSectionMask,
    UsecaseSensorBaseGainFactor                         = SensorBaseGainFactor                         | UsecaseMetadataSectionMask,
    UsecaseSensorBlackLevelPattern                      = SensorBlackLevelPattern                      | UsecaseMetadataSectionMask,
    UsecaseSensorMaxAnalogSensitivity                   = SensorMaxAnalogSensitivity                   | UsecaseMetadataSectionMask,
    UsecaseSensorOrientation                            = SensorOrientation                            | UsecaseMetadataSectionMask,
    UsecaseSensorProfileHueSaturationMapDimensions      = SensorProfileHueSaturationMapDimensions      | UsecaseMetadataSectionMask,
    UsecaseSensorTimestamp                              = SensorTimestamp                              | UsecaseMetadataSectionMask,
    UsecaseSensorTemperature                            = SensorTemperature                            | UsecaseMetadataSectionMask,
    UsecaseSensorNeutralColorPoint                      = SensorNeutralColorPoint                      | UsecaseMetadataSectionMask,
    UsecaseSensorNoiseProfile                           = SensorNoiseProfile                           | UsecaseMetadataSectionMask,
    UsecaseSensorProfileHueSaturationMap                = SensorProfileHueSaturationMap                | UsecaseMetadataSectionMask,
    UsecaseSensorProfileToneCurve                       = SensorProfileToneCurve                       | UsecaseMetadataSectionMask,
    UsecaseSensorGreenSplit                             = SensorGreenSplit                             | UsecaseMetadataSectionMask,
    UsecaseSensorTestPatternData                        = SensorTestPatternData                        | UsecaseMetadataSectionMask,
    UsecaseSensorTestPatternMode                        = SensorTestPatternMode                        | UsecaseMetadataSectionMask,
    UsecaseSensorAvailableTestPatternModes              = SensorAvailableTestPatternModes              | UsecaseMetadataSectionMask,
    UsecaseSensorRollingShutterSkew                     = SensorRollingShutterSkew                     | UsecaseMetadataSectionMask,
    UsecaseSensorEnd                                    = SensorEnd                                    | UsecaseMetadataSectionMask,

    UsecaseSensorInfoActiveArraySize                    = SensorInfoActiveArraySize                    | UsecaseMetadataSectionMask,
    UsecaseSensorInfoSensitivityRange                   = SensorInfoSensitivityRange                   | UsecaseMetadataSectionMask,
    UsecaseSensorInfoColorFilterArrangement             = SensorInfoColorFilterArrangement             | UsecaseMetadataSectionMask,
    UsecaseSensorInfoExposureTimeRange                  = SensorInfoExposureTimeRange                  | UsecaseMetadataSectionMask,
    UsecaseSensorInfoMaxFrameDuration                   = SensorInfoMaxFrameDuration                   | UsecaseMetadataSectionMask,
    UsecaseSensorInfoPhysicalSize                       = SensorInfoPhysicalSize                       | UsecaseMetadataSectionMask,
    UsecaseSensorInfoPixelArraySize                     = SensorInfoPixelArraySize                     | UsecaseMetadataSectionMask,
    UsecaseSensorInfoWhiteLevel                         = SensorInfoWhiteLevel                         | UsecaseMetadataSectionMask,
    UsecaseSensorInfoTimestampSource                    = SensorInfoTimestampSource                    | UsecaseMetadataSectionMask,
    UsecaseSensorInfoLensShadingApplied                 = SensorInfoLensShadingApplied                 | UsecaseMetadataSectionMask,
    UsecaseSensorInfoPreCorrectionActiveArraySize       = SensorInfoPreCorrectionActiveArraySize       | UsecaseMetadataSectionMask,
    UsecaseSensorInfoEnd                                = SensorInfoEnd                                | UsecaseMetadataSectionMask,

    UsecaseShadingMode                                  = ShadingMode                                  | UsecaseMetadataSectionMask,
    UsecaseShadingStrength                              = ShadingStrength                              | UsecaseMetadataSectionMask,
    UsecaseShadingAvailableModes                        = ShadingAvailableModes                        | UsecaseMetadataSectionMask,
    UsecaseShadingEnd                                   = ShadingEnd                                   | UsecaseMetadataSectionMask,

    UsecaseStatisticsFaceDetectMode                     = StatisticsFaceDetectMode                     | UsecaseMetadataSectionMask,
    UsecaseStatisticsHistogramMode                      = StatisticsHistogramMode                      | UsecaseMetadataSectionMask,
    UsecaseStatisticsSharpnessMapMode                   = StatisticsSharpnessMapMode                   | UsecaseMetadataSectionMask,
    UsecaseStatisticsHotPixelMapMode                    = StatisticsHotPixelMapMode                    | UsecaseMetadataSectionMask,
    UsecaseStatisticsFaceIds                            = StatisticsFaceIds                            | UsecaseMetadataSectionMask,
    UsecaseStatisticsFaceLandmarks                      = StatisticsFaceLandmarks                      | UsecaseMetadataSectionMask,
    UsecaseStatisticsFaceRectangles                     = StatisticsFaceRectangles                     | UsecaseMetadataSectionMask,
    UsecaseStatisticsFaceScores                         = StatisticsFaceScores                         | UsecaseMetadataSectionMask,
    UsecaseStatisticsHistogram                          = StatisticsHistogram                          | UsecaseMetadataSectionMask,
    UsecaseStatisticsSharpnessMap                       = StatisticsSharpnessMap                       | UsecaseMetadataSectionMask,
    UsecaseStatisticsLensShadingCorrectionMap           = StatisticsLensShadingCorrectionMap           | UsecaseMetadataSectionMask,
    UsecaseStatisticsLensShadingMap                     = StatisticsLensShadingMap                     | UsecaseMetadataSectionMask,
    UsecaseStatisticsPredictedColorGains                = StatisticsPredictedColorGains                | UsecaseMetadataSectionMask,
    UsecaseStatisticsPredictedColorTransform            = StatisticsPredictedColorTransform            | UsecaseMetadataSectionMask,
    UsecaseStatisticsSceneFlicker                       = StatisticsSceneFlicker                       | UsecaseMetadataSectionMask,
    UsecaseStatisticsHotPixelMap                        = StatisticsHotPixelMap                        | UsecaseMetadataSectionMask,
    UsecaseStatisticsLensShadingMapMode                 = StatisticsLensShadingMapMode                 | UsecaseMetadataSectionMask,
    UsecaseStatisticsEnd                                = StatisticsEnd                                | UsecaseMetadataSectionMask,

    UsecaseStatisticsInfoAvailableFaceDetectModes       = StatisticsInfoAvailableFaceDetectModes       | UsecaseMetadataSectionMask,
    UsecaseStatisticsInfoHistogramBucketCount           = StatisticsInfoHistogramBucketCount           | UsecaseMetadataSectionMask,
    UsecaseStatisticsInfoMaxFaceCount                   = StatisticsInfoMaxFaceCount                   | UsecaseMetadataSectionMask,
    UsecaseStatisticsInfoMaxHistogramCount              = StatisticsInfoMaxHistogramCount              | UsecaseMetadataSectionMask,
    UsecaseStatisticsInfoMaxSharpnessMapValue           = StatisticsInfoMaxSharpnessMapValue           | UsecaseMetadataSectionMask,
    UsecaseStatisticsInfoSharpnessMapSize               = StatisticsInfoSharpnessMapSize               | UsecaseMetadataSectionMask,
    UsecaseStatisticsInfoAvailableHotPixelMapModes      = StatisticsInfoAvailableHotPixelMapModes      | UsecaseMetadataSectionMask,
    UsecaseStatisticsInfoAvailableLensShadingMapModes   = StatisticsInfoAvailableLensShadingMapModes   | UsecaseMetadataSectionMask,
    UsecaseStatisticsInfoEnd                            = StatisticsInfoEnd                            | UsecaseMetadataSectionMask,

    UsecaseTonemapCurveBlue                             = TonemapCurveBlue                             | UsecaseMetadataSectionMask,
    UsecaseTonemapCurveGreen                            = TonemapCurveGreen                            | UsecaseMetadataSectionMask,
    UsecaseTonemapCurveRed                              = TonemapCurveRed                              | UsecaseMetadataSectionMask,
    UsecaseTonemapMode                                  = TonemapMode                                  | UsecaseMetadataSectionMask,
    UsecaseTonemapMaxCurvePoints                        = TonemapMaxCurvePoints                        | UsecaseMetadataSectionMask,
    UsecaseTonemapAvailableToneMapModes                 = TonemapAvailableToneMapModes                 | UsecaseMetadataSectionMask,
    UsecaseTonemapGamma                                 = TonemapGamma                                 | UsecaseMetadataSectionMask,
    UsecaseTonemapPresetCurve                           = TonemapPresetCurve                           | UsecaseMetadataSectionMask,
    UsecaseTonemapEnd                                   = TonemapEnd                                   | UsecaseMetadataSectionMask,

    UsecaseLedTransmit                                  = LedTransmit                                  | UsecaseMetadataSectionMask,
    UsecaseLedAvailableLeds                             = LedAvailableLeds                             | UsecaseMetadataSectionMask,
    UsecaseLedEnd                                       = LedEnd                                       | UsecaseMetadataSectionMask,

    UsecaseInfoSupportedHardwareLevel                   = InfoSupportedHardwareLevel                   | UsecaseMetadataSectionMask,
    UsecaseInfoEnd                                      = InfoEnd                                      | UsecaseMetadataSectionMask,

    UsecaseBlackLevelLock                               = BlackLevelLock                               | UsecaseMetadataSectionMask,
    UsecaseBlackLevelEnd                                = BlackLevelEnd                                | UsecaseMetadataSectionMask,

    UsecaseSyncFrameNumber                              = SyncFrameNumber                              | UsecaseMetadataSectionMask,
    UsecaseSyncMaxLatency                               = SyncMaxLatency                               | UsecaseMetadataSectionMask,
    UsecaseSyncEnd                                      = SyncEnd                                      | UsecaseMetadataSectionMask,

    UsecaseReprocessEffectiveExposureFactor             = ReprocessEffectiveExposureFactor             | UsecaseMetadataSectionMask,
    UsecaseReprocessMaxCaptureStall                     = ReprocessMaxCaptureStall                     | UsecaseMetadataSectionMask,
    UsecaseReprocessEnd                                 = ReprocessEnd                                 | UsecaseMetadataSectionMask,

    UsecaseDepthMaxDepthSamples                         = DepthMaxDepthSamples                         | UsecaseMetadataSectionMask,
    UsecaseDepthAvailableDepthStreamConfigurations      = DepthAvailableDepthStreamConfigurations      | UsecaseMetadataSectionMask,
    UsecaseDepthAvailableDepthMinFrameDurations         = DepthAvailableDepthMinFrameDurations         | UsecaseMetadataSectionMask,
    UsecaseDepthAvailableDepthStallDurations            = DepthAvailableDepthStallDurations            | UsecaseMetadataSectionMask,
    UsecaseDepthDepthIsExclusive                        = DepthDepthIsExclusive                        | UsecaseMetadataSectionMask,
    UsecaseDepthEnd                                     = DepthEnd                                     | UsecaseMetadataSectionMask,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    UsecaseLogicalMultiCameraPhysicalIDs                = LogicalMultiCameraPhysicalIDs                | UsecaseMetadataSectionMask,
    UsecaseLogicalMultiCameraSensorSyncType             = LogicalMultiCameraSensorSyncType             | UsecaseMetadataSectionMask,
    UsecaseLogicalMultiCameraEnd                        = LogicalMultiCameraEnd                        | UsecaseMetadataSectionMask,
#endif // Android-P or better
    StaticColorCorrectionMode                          = ColorCorrectionMode                          | StaticMetadataSectionMask,
    StaticColorCorrectionTransform                     = ColorCorrectionTransform                     | StaticMetadataSectionMask,
    StaticColorCorrectionGains                         = ColorCorrectionGains                         | StaticMetadataSectionMask,
    StaticColorCorrectionAberrationMode                = ColorCorrectionAberrationMode                | StaticMetadataSectionMask,
    StaticColorCorrectionAvailableAberrationModes      = ColorCorrectionAvailableAberrationModes      | StaticMetadataSectionMask,
    StaticColorCorrectionEnd                           = ColorCorrectionEnd                           | StaticMetadataSectionMask,

    StaticControlAEAntibandingMode                     = ControlAEAntibandingMode                     | StaticMetadataSectionMask,
    StaticControlAEExposureCompensation                = ControlAEExposureCompensation                | StaticMetadataSectionMask,
    StaticControlAELock                                = ControlAELock                                | StaticMetadataSectionMask,
    StaticControlAEMode                                = ControlAEMode                                | StaticMetadataSectionMask,
    StaticControlAERegions                             = ControlAERegions                             | StaticMetadataSectionMask,
    StaticControlAETargetFpsRange                      = ControlAETargetFpsRange                      | StaticMetadataSectionMask,
    StaticControlAEPrecaptureTrigger                   = ControlAEPrecaptureTrigger                   | StaticMetadataSectionMask,
    StaticControlAFMode                                = ControlAFMode                                | StaticMetadataSectionMask,
    StaticControlAFRegions                             = ControlAFRegions                             | StaticMetadataSectionMask,
    StaticControlAFTrigger                             = ControlAFTrigger                             | StaticMetadataSectionMask,
    StaticControlAWBLock                               = ControlAWBLock                               | StaticMetadataSectionMask,
    StaticControlAWBMode                               = ControlAWBMode                               | StaticMetadataSectionMask,
    StaticControlAWBRegions                            = ControlAWBRegions                            | StaticMetadataSectionMask,
    StaticControlCaptureIntent                         = ControlCaptureIntent                         | StaticMetadataSectionMask,
    StaticControlEffectMode                            = ControlEffectMode                            | StaticMetadataSectionMask,
    StaticControlMode                                  = ControlMode                                  | StaticMetadataSectionMask,
    StaticControlSceneMode                             = ControlSceneMode                             | StaticMetadataSectionMask,
    StaticControlVideoStabilizationMode                = ControlVideoStabilizationMode                | StaticMetadataSectionMask,
    StaticControlAEAvailableAntibandingModes           = ControlAEAvailableAntibandingModes           | StaticMetadataSectionMask,
    StaticControlAEAvailableModes                      = ControlAEAvailableModes                      | StaticMetadataSectionMask,
    StaticControlAEAvailableTargetFPSRanges            = ControlAEAvailableTargetFPSRanges            | StaticMetadataSectionMask,
    StaticControlAECompensationRange                   = ControlAECompensationRange                   | StaticMetadataSectionMask,
    StaticControlAECompensationStep                    = ControlAECompensationStep                    | StaticMetadataSectionMask,
    StaticControlAFAvailableModes                      = ControlAFAvailableModes                      | StaticMetadataSectionMask,
    StaticControlAvailableEffects                      = ControlAvailableEffects                      | StaticMetadataSectionMask,
    StaticControlAvailableSceneModes                   = ControlAvailableSceneModes                   | StaticMetadataSectionMask,
    StaticControlAvailableVideoStabilizationModes      = ControlAvailableVideoStabilizationModes      | StaticMetadataSectionMask,
    StaticControlAWBAvailableModes                     = ControlAWBAvailableModes                     | StaticMetadataSectionMask,
    StaticControlMaxRegions                            = ControlMaxRegions                            | StaticMetadataSectionMask,
    StaticControlSceneModeOverrides                    = ControlSceneModeOverrides                    | StaticMetadataSectionMask,
    StaticControlAEPrecaptureId                        = ControlAEPrecaptureId                        | StaticMetadataSectionMask,
    StaticControlAEState                               = ControlAEState                               | StaticMetadataSectionMask,
    StaticControlAFState                               = ControlAFState                               | StaticMetadataSectionMask,
    StaticControlAFTriggerId                           = ControlAFTriggerId                           | StaticMetadataSectionMask,
    StaticControlAWBState                              = ControlAWBState                              | StaticMetadataSectionMask,
    StaticControlAvailableHighSpeedVideoConfigurations = ControlAvailableHighSpeedVideoConfigurations | StaticMetadataSectionMask,
    StaticControlAELockAvailable                       = ControlAELockAvailable                       | StaticMetadataSectionMask,
    StaticControlAWBLockAvailable                      = ControlAWBLockAvailable                      | StaticMetadataSectionMask,
    StaticControlAvailableModes                        = ControlAvailableModes                        | StaticMetadataSectionMask,
    StaticControlPostRawSensitivityBoostRange          = ControlPostRawSensitivityBoostRange          | StaticMetadataSectionMask,
    StaticControlPostRawSensitivityBoost               = ControlPostRawSensitivityBoost               | StaticMetadataSectionMask,
    StaticControlEnd                                   = ControlEnd                                   | StaticMetadataSectionMask,

    StaticDemosaicMode                                 = DemosaicMode                                 | StaticMetadataSectionMask,
    StaticDemosaicEnd                                  = DemosaicEnd                                  | StaticMetadataSectionMask,

    StaticEdgeMode                                     = EdgeMode                                     | StaticMetadataSectionMask,
    StaticEdgeStrength                                 = EdgeStrength                                 | StaticMetadataSectionMask,
    StaticEdgeAvailableEdgeModes                       = EdgeAvailableEdgeModes                       | StaticMetadataSectionMask,
    StaticEdgeEnd                                      = EdgeEnd                                      | StaticMetadataSectionMask,

    StaticFlashFiringPower                             = FlashFiringPower                             | StaticMetadataSectionMask,
    StaticFlashFiringTime                              = FlashFiringTime                              | StaticMetadataSectionMask,
    StaticFlashMode                                    = FlashMode                                    | StaticMetadataSectionMask,
    StaticFlashColorTemperature                        = FlashColorTemperature                        | StaticMetadataSectionMask,
    StaticFlashMaxEnergy                               = FlashMaxEnergy                               | StaticMetadataSectionMask,
    StaticFlashState                                   = FlashState                                   | StaticMetadataSectionMask,
    StaticFlashEnd                                     = FlashEnd                                     | StaticMetadataSectionMask,

    StaticFlashInfoAvailable                           = FlashInfoAvailable                           | StaticMetadataSectionMask,
    StaticFlashInfoChargeDuration                      = FlashInfoChargeDuration                      | StaticMetadataSectionMask,
    StaticFlashInfoEnd                                 = FlashInfoEnd                                 | StaticMetadataSectionMask,

    StaticHotPixelMode                                 = HotPixelMode                                 | StaticMetadataSectionMask,
    StaticHotPixelAvailableHotPixelModes               = HotPixelAvailableHotPixelModes               | StaticMetadataSectionMask,
    StaticHotPixelEnd                                  = HotPixelEnd                                  | StaticMetadataSectionMask,

    StaticJPEGGpsCoordinates                           = JPEGGpsCoordinates                           | StaticMetadataSectionMask,
    StaticJPEGGpsProcessingMethod                      = JPEGGpsProcessingMethod                      | StaticMetadataSectionMask,
    StaticJPEGGpsTimestamp                             = JPEGGpsTimestamp                             | StaticMetadataSectionMask,
    StaticJPEGOrientation                              = JPEGOrientation                              | StaticMetadataSectionMask,
    StaticJPEGQuality                                  = JPEGQuality                                  | StaticMetadataSectionMask,
    StaticJPEGThumbnailQuality                         = JPEGThumbnailQuality                         | StaticMetadataSectionMask,
    StaticJPEGThumbnailSize                            = JPEGThumbnailSize                            | StaticMetadataSectionMask,
    StaticJPEGAvailableThumbnailSizes                  = JPEGAvailableThumbnailSizes                  | StaticMetadataSectionMask,
    StaticJPEGMaxSize                                  = JPEGMaxSize                                  | StaticMetadataSectionMask,
    StaticJPEGSize                                     = JPEGSize                                     | StaticMetadataSectionMask,
    StaticJPEGEnd                                      = JPEGEnd                                      | StaticMetadataSectionMask,

    StaticLensAperture                                 = LensAperture                                 | StaticMetadataSectionMask,
    StaticLensFilterDensity                            = LensFilterDensity                            | StaticMetadataSectionMask,
    StaticLensFocalLength                              = LensFocalLength                              | StaticMetadataSectionMask,
    StaticLensFocusDistance                            = LensFocusDistance                            | StaticMetadataSectionMask,
    StaticLensOpticalStabilizationMode                 = LensOpticalStabilizationMode                 | StaticMetadataSectionMask,
    StaticLensFacing                                   = LensFacing                                   | StaticMetadataSectionMask,
    StaticLensPoseRotation                             = LensPoseRotation                             | StaticMetadataSectionMask,
    StaticLensPoseTranslation                          = LensPoseTranslation                          | StaticMetadataSectionMask,
    StaticLensFocusRange                               = LensFocusRange                               | StaticMetadataSectionMask,
    StaticLensState                                    = LensState                                    | StaticMetadataSectionMask,
    StaticLensIntrinsicCalibration                     = LensIntrinsicCalibration                     | StaticMetadataSectionMask,
    StaticLensRadialDistortion                         = LensRadialDistortion                         | StaticMetadataSectionMask,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    StaticLensPoseReference                            = LensPoseReference                            | StaticMetadataSectionMask,
    StaticLensDistortion                               = LensDistortion                               | StaticMetadataSectionMask,
#endif // Android-P or better
    StaticLensEnd                                      = LensEnd                                      | StaticMetadataSectionMask,

    StaticLensInfoAvailableApertures                   = LensInfoAvailableApertures                   | StaticMetadataSectionMask,
    StaticLensInfoAvailableFilterDensities             = LensInfoAvailableFilterDensities             | StaticMetadataSectionMask,
    StaticLensInfoAvailableFocalLengths                = LensInfoAvailableFocalLengths                | StaticMetadataSectionMask,
    StaticLensInfoAvailableOpticalStabilization        = LensInfoAvailableOpticalStabilization        | StaticMetadataSectionMask,
    StaticLensInfoHyperfocalDistance                   = LensInfoHyperfocalDistance                   | StaticMetadataSectionMask,
    StaticLensInfoMinimumFocusDistance                 = LensInfoMinimumFocusDistance                 | StaticMetadataSectionMask,
    StaticLensInfoShadingMapSize                       = LensInfoShadingMapSize                       | StaticMetadataSectionMask,
    StaticLensInfoFocusDistanceCalibration             = LensInfoFocusDistanceCalibration             | StaticMetadataSectionMask,
    StaticLensInfoEnd                                  = LensInfoEnd                                  | StaticMetadataSectionMask,

    StaticNoiseReductionMode                           = NoiseReductionMode                           | StaticMetadataSectionMask,
    StaticNoiseReductionStrength                       = NoiseReductionStrength                       | StaticMetadataSectionMask,
    StaticNoiseReductionAvailableNoiseReductionModes   = NoiseReductionAvailableNoiseReductionModes   | StaticMetadataSectionMask,
    StaticNoiseReductionEnd                            = NoiseReductionEnd                            | StaticMetadataSectionMask,

    StaticQuirksMeteringCropRegion                     = QuirksMeteringCropRegion                     | StaticMetadataSectionMask,
    StaticQuirksTriggerAFWithAuto                      = QuirksTriggerAFWithAuto                      | StaticMetadataSectionMask,
    StaticQuirksUseZslFormat                           = QuirksUseZslFormat                           | StaticMetadataSectionMask,
    StaticQuirksUsePartialResult                       = QuirksUsePartialResult                       | StaticMetadataSectionMask,
    StaticQuirksPartialResult                          = QuirksPartialResult                          | StaticMetadataSectionMask,
    StaticQuirksEnd                                    = QuirksEnd                                    | StaticMetadataSectionMask,

    StaticRequestFrameCount                            = RequestFrameCount                            | StaticMetadataSectionMask,
    StaticRequestId                                    = RequestId                                    | StaticMetadataSectionMask,
    StaticRequestInputStreams                          = RequestInputStreams                          | StaticMetadataSectionMask,
    StaticRequestMetadataMode                          = RequestMetadataMode                          | StaticMetadataSectionMask,
    StaticRequestOutputStreams                         = RequestOutputStreams                         | StaticMetadataSectionMask,
    StaticRequestType                                  = RequestType                                  | StaticMetadataSectionMask,
    StaticRequestMaxNumOutputStreams                   = RequestMaxNumOutputStreams                   | StaticMetadataSectionMask,
    StaticRequestMaxNumReprocessStreams                = RequestMaxNumReprocessStreams                | StaticMetadataSectionMask,
    StaticRequestMaxNumInputStreams                    = RequestMaxNumInputStreams                    | StaticMetadataSectionMask,
    StaticRequestPipelineDepth                         = RequestPipelineDepth                         | StaticMetadataSectionMask,
    StaticRequestPipelineMaxDepth                      = RequestPipelineMaxDepth                      | StaticMetadataSectionMask,
    StaticRequestPartialResultCount                    = RequestPartialResultCount                    | StaticMetadataSectionMask,
    StaticRequestAvailableCapabilities                 = RequestAvailableCapabilities                 | StaticMetadataSectionMask,
    StaticRequestAvailableRequestKeys                  = RequestAvailableRequestKeys                  | StaticMetadataSectionMask,
    StaticRequestAvailableResultKeys                   = RequestAvailableResultKeys                   | StaticMetadataSectionMask,
    StaticRequestAvailableCharacteristicsKeys          = RequestAvailableCharacteristicsKeys          | StaticMetadataSectionMask,
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    StaticRequestAvailableSessionKeys                  = RequestAvailableSessionKeys                  | StaticMetadataSectionMask,
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
    StaticRequestEnd                                   = RequestEnd                                   | StaticMetadataSectionMask,

    StaticScalerCropRegion                             = ScalerCropRegion                             | StaticMetadataSectionMask,
    StaticScalerAvailableFormats                       = ScalerAvailableFormats                       | StaticMetadataSectionMask,
    StaticScalerAvailableJPEGMinDurations              = ScalerAvailableJPEGMinDurations              | StaticMetadataSectionMask,
    StaticScalerAvailableJPEGSizes                     = ScalerAvailableJPEGSizes                     | StaticMetadataSectionMask,
    StaticScalerAvailableMaxDigitalZoom                = ScalerAvailableMaxDigitalZoom                | StaticMetadataSectionMask,
    StaticScalerAvailableProcessedMinDurations         = ScalerAvailableProcessedMinDurations         | StaticMetadataSectionMask,
    StaticScalerAvailableProcessedSizes                = ScalerAvailableProcessedSizes                | StaticMetadataSectionMask,
    StaticScalerAvailableRawMinDurations               = ScalerAvailableRawMinDurations               | StaticMetadataSectionMask,
    StaticScalerAvailableRawSizes                      = ScalerAvailableRawSizes                      | StaticMetadataSectionMask,
    StaticScalerAvailableInputOutputFormatsMap         = ScalerAvailableInputOutputFormatsMap         | StaticMetadataSectionMask,
    StaticScalerAvailableStreamConfigurations          = ScalerAvailableStreamConfigurations          | StaticMetadataSectionMask,
    StaticScalerAvailableMinFrameDurations             = ScalerAvailableMinFrameDurations             | StaticMetadataSectionMask,
    StaticScalerAvailableStallDurations                = ScalerAvailableStallDurations                | StaticMetadataSectionMask,
    StaticScalerCroppingType                           = ScalerCroppingType                           | StaticMetadataSectionMask,
    StaticScalerEnd                                    = ScalerEnd                                    | StaticMetadataSectionMask,

    StaticSensorExposureTime                           = SensorExposureTime                           | StaticMetadataSectionMask,
    StaticSensorFrameDuration                          = SensorFrameDuration                          | StaticMetadataSectionMask,
    StaticSensorSensitivity                            = SensorSensitivity                            | StaticMetadataSectionMask,
    StaticSensorReferenceIlluminant1                   = SensorReferenceIlluminant1                   | StaticMetadataSectionMask,
    StaticSensorReferenceIlluminant2                   = SensorReferenceIlluminant2                   | StaticMetadataSectionMask,
    StaticSensorCalibrationTransform1                  = SensorCalibrationTransform1                  | StaticMetadataSectionMask,
    StaticSensorCalibrationTransform2                  = SensorCalibrationTransform2                  | StaticMetadataSectionMask,
    StaticSensorColorTransform1                        = SensorColorTransform1                        | StaticMetadataSectionMask,
    StaticSensorColorTransform2                        = SensorColorTransform2                        | StaticMetadataSectionMask,
    StaticSensorForwardMatrix1                         = SensorForwardMatrix1                         | StaticMetadataSectionMask,
    StaticSensorForwardMatrix2                         = SensorForwardMatrix2                         | StaticMetadataSectionMask,
    StaticSensorBaseGainFactor                         = SensorBaseGainFactor                         | StaticMetadataSectionMask,
    StaticSensorBlackLevelPattern                      = SensorBlackLevelPattern                      | StaticMetadataSectionMask,
    StaticSensorMaxAnalogSensitivity                   = SensorMaxAnalogSensitivity                   | StaticMetadataSectionMask,
    StaticSensorOrientation                            = SensorOrientation                            | StaticMetadataSectionMask,
    StaticSensorProfileHueSaturationMapDimensions      = SensorProfileHueSaturationMapDimensions      | StaticMetadataSectionMask,
    StaticSensorTimestamp                              = SensorTimestamp                              | StaticMetadataSectionMask,
    StaticSensorTemperature                            = SensorTemperature                            | StaticMetadataSectionMask,
    StaticSensorNeutralColorPoint                      = SensorNeutralColorPoint                      | StaticMetadataSectionMask,
    StaticSensorNoiseProfile                           = SensorNoiseProfile                           | StaticMetadataSectionMask,
    StaticSensorProfileHueSaturationMap                = SensorProfileHueSaturationMap                | StaticMetadataSectionMask,
    StaticSensorProfileToneCurve                       = SensorProfileToneCurve                       | StaticMetadataSectionMask,
    StaticSensorGreenSplit                             = SensorGreenSplit                             | StaticMetadataSectionMask,
    StaticSensorTestPatternData                        = SensorTestPatternData                        | StaticMetadataSectionMask,
    StaticSensorTestPatternMode                        = SensorTestPatternMode                        | StaticMetadataSectionMask,
    StaticSensorAvailableTestPatternModes              = SensorAvailableTestPatternModes              | StaticMetadataSectionMask,
    StaticSensorRollingShutterSkew                     = SensorRollingShutterSkew                     | StaticMetadataSectionMask,
    StaticSensorOpaqueRawSize                          = SensorOpaqueRawSize                          | StaticMetadataSectionMask,
    StaticSensorEnd                                    = SensorEnd                                    | StaticMetadataSectionMask,

    StaticSensorInfoActiveArraySize                    = SensorInfoActiveArraySize                    | StaticMetadataSectionMask,
    StaticSensorInfoSensitivityRange                   = SensorInfoSensitivityRange                   | StaticMetadataSectionMask,
    StaticSensorInfoColorFilterArrangement             = SensorInfoColorFilterArrangement             | StaticMetadataSectionMask,
    StaticSensorInfoExposureTimeRange                  = SensorInfoExposureTimeRange                  | StaticMetadataSectionMask,
    StaticSensorInfoMaxFrameDuration                   = SensorInfoMaxFrameDuration                   | StaticMetadataSectionMask,
    StaticSensorInfoPhysicalSize                       = SensorInfoPhysicalSize                       | StaticMetadataSectionMask,
    StaticSensorInfoPixelArraySize                     = SensorInfoPixelArraySize                     | StaticMetadataSectionMask,
    StaticSensorInfoWhiteLevel                         = SensorInfoWhiteLevel                         | StaticMetadataSectionMask,
    StaticSensorInfoTimestampSource                    = SensorInfoTimestampSource                    | StaticMetadataSectionMask,
    StaticSensorInfoLensShadingApplied                 = SensorInfoLensShadingApplied                 | StaticMetadataSectionMask,
    StaticSensorInfoPreCorrectionActiveArraySize       = SensorInfoPreCorrectionActiveArraySize       | StaticMetadataSectionMask,
    StaticSensorInfoEnd                                = SensorInfoEnd                                | StaticMetadataSectionMask,

    StaticShadingMode                                  = ShadingMode                                  | StaticMetadataSectionMask,
    StaticShadingStrength                              = ShadingStrength                              | StaticMetadataSectionMask,
    StaticShadingAvailableModes                        = ShadingAvailableModes                        | StaticMetadataSectionMask,
    StaticShadingEnd                                   = ShadingEnd                                   | StaticMetadataSectionMask,

    StaticStatisticsFaceDetectMode                     = StatisticsFaceDetectMode                     | StaticMetadataSectionMask,
    StaticStatisticsHistogramMode                      = StatisticsHistogramMode                      | StaticMetadataSectionMask,
    StaticStatisticsSharpnessMapMode                   = StatisticsSharpnessMapMode                   | StaticMetadataSectionMask,
    StaticStatisticsHotPixelMapMode                    = StatisticsHotPixelMapMode                    | StaticMetadataSectionMask,
    StaticStatisticsFaceIds                            = StatisticsFaceIds                            | StaticMetadataSectionMask,
    StaticStatisticsFaceLandmarks                      = StatisticsFaceLandmarks                      | StaticMetadataSectionMask,
    StaticStatisticsFaceRectangles                     = StatisticsFaceRectangles                     | StaticMetadataSectionMask,
    StaticStatisticsFaceScores                         = StatisticsFaceScores                         | StaticMetadataSectionMask,
    StaticStatisticsHistogram                          = StatisticsHistogram                          | StaticMetadataSectionMask,
    StaticStatisticsSharpnessMap                       = StatisticsSharpnessMap                       | StaticMetadataSectionMask,
    StaticStatisticsLensShadingCorrectionMap           = StatisticsLensShadingCorrectionMap           | StaticMetadataSectionMask,
    StaticStatisticsLensShadingMap                     = StatisticsLensShadingMap                     | StaticMetadataSectionMask,
    StaticStatisticsPredictedColorGains                = StatisticsPredictedColorGains                | StaticMetadataSectionMask,
    StaticStatisticsPredictedColorTransform            = StatisticsPredictedColorTransform            | StaticMetadataSectionMask,
    StaticStatisticsSceneFlicker                       = StatisticsSceneFlicker                       | StaticMetadataSectionMask,
    StaticStatisticsHotPixelMap                        = StatisticsHotPixelMap                        | StaticMetadataSectionMask,
    StaticStatisticsLensShadingMapMode                 = StatisticsLensShadingMapMode                 | StaticMetadataSectionMask,
    StaticStatisticsEnd                                = StatisticsEnd                                | StaticMetadataSectionMask,

    StaticStatisticsInfoAvailableFaceDetectModes       = StatisticsInfoAvailableFaceDetectModes       | StaticMetadataSectionMask,
    StaticStatisticsInfoHistogramBucketCount           = StatisticsInfoHistogramBucketCount           | StaticMetadataSectionMask,
    StaticStatisticsInfoMaxFaceCount                   = StatisticsInfoMaxFaceCount                   | StaticMetadataSectionMask,
    StaticStatisticsInfoMaxHistogramCount              = StatisticsInfoMaxHistogramCount              | StaticMetadataSectionMask,
    StaticStatisticsInfoMaxSharpnessMapValue           = StatisticsInfoMaxSharpnessMapValue           | StaticMetadataSectionMask,
    StaticStatisticsInfoSharpnessMapSize               = StatisticsInfoSharpnessMapSize               | StaticMetadataSectionMask,
    StaticStatisticsInfoAvailableHotPixelMapModes      = StatisticsInfoAvailableHotPixelMapModes      | StaticMetadataSectionMask,
    StaticStatisticsInfoAvailableLensShadingMapModes   = StatisticsInfoAvailableLensShadingMapModes   | StaticMetadataSectionMask,
    StaticStatisticsInfoEnd                            = StatisticsInfoEnd                            | StaticMetadataSectionMask,

    StaticTonemapCurveBlue                             = TonemapCurveBlue                             | StaticMetadataSectionMask,
    StaticTonemapCurveGreen                            = TonemapCurveGreen                            | StaticMetadataSectionMask,
    StaticTonemapCurveRed                              = TonemapCurveRed                              | StaticMetadataSectionMask,
    StaticTonemapMode                                  = TonemapMode                                  | StaticMetadataSectionMask,
    StaticTonemapMaxCurvePoints                        = TonemapMaxCurvePoints                        | StaticMetadataSectionMask,
    StaticTonemapAvailableToneMapModes                 = TonemapAvailableToneMapModes                 | StaticMetadataSectionMask,
    StaticTonemapGamma                                 = TonemapGamma                                 | StaticMetadataSectionMask,
    StaticTonemapPresetCurve                           = TonemapPresetCurve                           | StaticMetadataSectionMask,
    StaticTonemapEnd                                   = TonemapEnd                                   | StaticMetadataSectionMask,

    StaticLedTransmit                                  = LedTransmit                                  | StaticMetadataSectionMask,
    StaticLedAvailableLeds                             = LedAvailableLeds                             | StaticMetadataSectionMask,
    StaticLedEnd                                       = LedEnd                                       | StaticMetadataSectionMask,

    StaticInfoSupportedHardwareLevel                   = InfoSupportedHardwareLevel                   | StaticMetadataSectionMask,
    StaticInfoEnd                                      = InfoEnd                                      | StaticMetadataSectionMask,

    StaticBlackLevelLock                               = BlackLevelLock                               | StaticMetadataSectionMask,
    StaticBlackLevelEnd                                = BlackLevelEnd                                | StaticMetadataSectionMask,

    StaticSyncFrameNumber                              = SyncFrameNumber                              | StaticMetadataSectionMask,
    StaticSyncMaxLatency                               = SyncMaxLatency                               | StaticMetadataSectionMask,
    StaticSyncEnd                                      = SyncEnd                                      | StaticMetadataSectionMask,

    StaticReprocessEffectiveExposureFactor             = ReprocessEffectiveExposureFactor             | StaticMetadataSectionMask,
    StaticReprocessMaxCaptureStall                     = ReprocessMaxCaptureStall                     | StaticMetadataSectionMask,
    StaticReprocessEnd                                 = ReprocessEnd                                 | StaticMetadataSectionMask,

    StaticDepthMaxDepthSamples                         = DepthMaxDepthSamples                         | StaticMetadataSectionMask,
    StaticDepthAvailableDepthStreamConfigurations      = DepthAvailableDepthStreamConfigurations      | StaticMetadataSectionMask,
    StaticDepthAvailableDepthMinFrameDurations         = DepthAvailableDepthMinFrameDurations         | StaticMetadataSectionMask,
    StaticDepthAvailableDepthStallDurations            = DepthAvailableDepthStallDurations            | StaticMetadataSectionMask,
    StaticDepthDepthIsExclusive                        = DepthDepthIsExclusive                        | StaticMetadataSectionMask,
    StaticDepthEnd                                     = DepthEnd                                     | StaticMetadataSectionMask,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    StaticLogicalMultiCameraPhysicalIDs                = LogicalMultiCameraPhysicalIDs                | StaticMetadataSectionMask,
    StaticLogicalMultiCameraSensorSyncType             = LogicalMultiCameraSensorSyncType             | StaticMetadataSectionMask,
    StaticLogicalMultiCameraEnd                        = LogicalMultiCameraEnd                        | StaticMetadataSectionMask,
    StaticHEICInfoSupported                            = HEICInfoSupported                            | StaticMetadataSectionMask,
    StaticHEICInfoMaxJpegAppSegmentsCount              = HEICInfoMaxJpegAppSegmentsCount              | StaticMetadataSectionMask,
    StaticHEICInfoSupportedEnd                         = HEICInfoEnd                                  | StaticMetadataSectionMask,
#endif // Android-P or better
};

/// @brief Definition of the starting linear offset of each section
enum SectionLinearTagOffsets
{
    ColorCorrectionOffset = 0,
    ControlOffset         = (ColorCorrectionOffset + ColorCorrectionEnd - MetadataSectionColorCorrectionStart),
    DemosaicOffset        = (ControlOffset         + ControlEnd         - MetadataSectionControlStart),
    EdgeOffset            = (DemosaicOffset        + DemosaicEnd        - MetadataSectionDemosaicStart),
    FlashOffset           = (EdgeOffset            + EdgeEnd            - MetadataSectionEdgeStart),
    FlashInfoOffset       = (FlashOffset           + FlashEnd           - MetadataSectionFlashStart),
    HotPixelOffset        = (FlashInfoOffset       + FlashInfoEnd       - MetadataSectionFlashInfoStart),
    JPEGOffset            = (HotPixelOffset        + HotPixelEnd        - MetadataSectionHotPixelStart),
    LensOffset            = (JPEGOffset            + JPEGEnd            - MetadataSectionJPEGStart),
    LensInfoOffset        = (LensOffset            + LensEnd            - MetadataSectionLensStart),
    NoiseReductionOffset  = (LensInfoOffset        + LensInfoEnd        - MetadataSectionLensInfoStart),
    QuirksOffset          = (NoiseReductionOffset  + NoiseReductionEnd  - MetadataSectionNoiseReductionStart),
    RequestOffset         = (QuirksOffset          + QuirksEnd          - MetadataSectionQuirksStart),
    ScalerOffset          = (RequestOffset         + RequestEnd         - MetadataSectionRequestStart),
    SensorOffset          = (ScalerOffset          + ScalerEnd          - MetadataSectionScalerStart),
    SensorInfoOffset      = (SensorOffset          + SensorEnd          - MetadataSectionSensorStart),
    ShadingOffset         = (SensorInfoOffset      + SensorInfoEnd      - MetadataSectionSensorInfoStart),
    StatisticsOffset      = (ShadingOffset         + ShadingEnd         - MetadataSectionShadingStart),
    StatisticsInfoOffset  = (StatisticsOffset      + StatisticsEnd      - MetadataSectionStatisticsStart),
    TonemapOffset         = (StatisticsInfoOffset  + StatisticsInfoEnd  - MetadataSectionStatisticsInfoStart),
    LedOffset             = (TonemapOffset         + TonemapEnd         - MetadataSectionTonemapStart),
    InfoOffset            = (LedOffset             + LedEnd             - MetadataSectionLedStart),
    BlackLevelOffset      = (InfoOffset            + InfoEnd            - MetadataSectionInfoStart),
    SyncOffset            = (BlackLevelOffset      + BlackLevelEnd      - MetadataSectionBlackLevelStart),
    ReprocessOffset       = (SyncOffset            + SyncEnd            - MetadataSectionSyncStart),
    DepthOffset           = (ReprocessOffset    + ReprocessEnd        - MetadataSectionReprocessStart),
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    LogicalMultiCameraOffset    = (DepthOffset                + DepthEnd                - MetadataSectionDepthStart),
    DistortionCorrectionOffset  = (LogicalMultiCameraOffset   + LogicalMultiCameraEnd   - MetadataSectionLogicalMultiCameraStart),
    HEICOffset                  = (DistortionCorrectionOffset + DistortionCorrectionEnd - MetadataSectionDistortionCorrectionStart),
    HEICInfoOffset              = (HEICOffset                 + HEICEnd                 - MetadataSectionHEICStart),
    VendorSectionOffset         = (HEICInfoOffset             + HEICInfoEnd             - MetadataSectionHEICInfoStart),
#else
    VendorSectionOffset   = (DepthOffset           + DepthEnd           - MetadataSectionDepthStart),
#endif // Android-P or better
};

/// @brief LUT for each section, allowing for a linear, contiguous id to be generated from tag. See CAMX_TAG_TO_INDEX
static const UINT32 SectionLinearTagLUT[] =
{
    ColorCorrectionOffset,
    ControlOffset,
    DemosaicOffset,
    EdgeOffset,
    FlashOffset,
    FlashInfoOffset,
    HotPixelOffset,
    JPEGOffset,
    LensOffset,
    LensInfoOffset,
    NoiseReductionOffset,
    QuirksOffset,
    RequestOffset,
    ScalerOffset,
    SensorOffset,
    SensorInfoOffset,
    ShadingOffset,
    StatisticsOffset,
    StatisticsInfoOffset,
    TonemapOffset,
    LedOffset,
    InfoOffset,
    BlackLevelOffset,
    SyncOffset,
    ReprocessOffset,
    DepthOffset,
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    LogicalMultiCameraOffset,
    DistortionCorrectionOffset,
    HEICOffset,
    HEICInfoOffset,
#endif // Android-P or better
    VendorSectionOffset,
};

/// @brief Maximum number of google fixed camera metadata tags
static const UINT MaxMetadataTagCount = VendorSectionOffset;

/// @brief ColorCorrectionMode
enum ColorCorrectionModeValues
{
    ColorCorrectionModeTransformMatrix,
    ColorCorrectionModeFast,
    ColorCorrectionModeHighQuality,
    ColorCorrectionModeEnd
};

/// @brief ColorCorrectionAberrationMode
enum ColorCorrectionAberrationModeValues
{
    ColorCorrectionAberrationModeOff,
    ColorCorrectionAberrationModeFast,
    ColorCorrectionAberrationModeHighQuality,
    ColorCorrectionAberrationModeEnd
};

/// @brief ControlAEAntibandingMode
enum ControlAEAntibandingModeValues
{
    ControlAEAntibandingModeOff,
    ControlAEAntibandingMode50Hz,
    ControlAEAntibandingMode60Hz,
    ControlAEAntibandingModeAuto,
    ControlAEAntibandingAuto_50HZ,
    ControlAEAntibandingAuto_60HZ,
    ControlAEAntibandingModeEnd
};

/// @brief ControlAELock
enum ControlAELockValues
{
    ControlAELockOff,
    ControlAELockOn,
    ControlAELockEnd
};

/// @brief ControlAEMode
enum ControlAEModeValues
{
    ControlAEModeOff,
    ControlAEModeOn,
    ControlAEModeOnAutoFlash,
    ControlAEModeOnAlwaysFlash,
    ControlAEModeOnAutoFlashRedeye,
    ControlAEModeEnd
};

/// @brief ControlAEPrecaptureTrigger
enum ControlAEPrecaptureTriggerValues
{
    ControlAEPrecaptureTriggerIdle,
    ControlAEPrecaptureTriggerStart,
    ControlAEPrecaptureTriggerCancel,
    ControlAEPrecaptureTriggerEnd
};

/// @brief ControlAFMode
enum ControlAFModeValues
{
    ControlAFModeOff,
    ControlAFModeAuto,
    ControlAFModeMacro,
    ControlAFModeContinuousVideo,
    ControlAFModeContinuousPicture,
    ControlAFModeEdof,
    ControlAFModeEnd
};

/// @brief ControlAFTrigger
enum ControlAFTriggerValues
{
    ControlAFTriggerIdle,
    ControlAFTriggerStart,
    ControlAFTriggerCancel,
    ControlAFTriggerEnd
};

/// @brief ControlAWBLock
enum ControlAWBLockValues
{
    ControlAWBLockOff,
    ControlAWBLockOn,
    ControlAWBLockEnd
};

/// @brief ControlAWBMode
enum ControlAWBModeValues
{
    ControlAWBModeOff,
    ControlAWBModeAuto,
    ControlAWBModeIncandescent,
    ControlAWBModeFluorescent,
    ControlAWBModeWarmFluorescent,
    ControlAWBModeDaylight,
    ControlAWBModeCloudyDaylight,
    ControlAWBModeTwilight,
    ControlAWBModeShade,
    ControlAWBModeEnd
};

/// @brief ControlCaptureIntent
enum ControlCaptureIntentValues
{
    ControlCaptureIntentCustom,
    ControlCaptureIntentPreview,
    ControlCaptureIntentStillCapture,
    ControlCaptureIntentVideoRecord,
    ControlCaptureIntentVideoSnapshot,
    ControlCaptureIntentZeroShutterLag,
    ControlCaptureIntentManual,
    ControlCaptureIntentEnd
};

/// @brief ControlEffectMode
enum ControlEffectModeValues
{
    ControlEffectModeOff,
    ControlEffectModeMono,
    ControlEffectModeNegative,
    ControlEffectModeSolarize,
    ControlEffectModeSepia,
    ControlEffectModePosterize,
    ControlEffectModeWhiteboard,
    ControlEffectModeBlackboard,
    ControlEffectModeAqua,
    ControlEffectModeEnd
};

/// @brief ControlMode
enum ControlModeValues
{
    ControlModeOff,
    ControlModeAuto,
    ControlModeUseSceneMode,
    ControlModeOffKeepState,
    ControlModeEnd
};

/// @brief ControlSceneMode
enum ControlSceneModeValues
{
    ControlSceneModeDisabled                        = 0,
    ControlSceneModeFacePriority,
    ControlSceneModeAction,
    ControlSceneModePortrait,
    ControlSceneModeLandscape,
    ControlSceneModeNight,
    ControlSceneModeNightPortrait,
    ControlSceneModeTheatre,
    ControlSceneModeBeach,
    ControlSceneModeSnow,
    ControlSceneModeSunset,
    ControlSceneModeSteadyphoto,
    ControlSceneModeFireworks,
    ControlSceneModeSports,
    ControlSceneModeParty,
    ControlSceneModeCandlelight,
    ControlSceneModeBarcode,
    ControlSceneModeHighSpeedVideo,
    ControlSceneModeHdr,
    ControlSceneModeFacePriorityLowLight,
    ControlSceneModeEnd
};

/// @brief ControlVideoStabilizationMode
enum ControlVideoStabilizationModeValues
{
    ControlVideoStabilizationModeOff,
    ControlVideoStabilizationModeOn,
    ControlVideoStabilizationModeEnd
};

/// @brief ControlAEState
enum ControlAEStateValues
{
    ControlAEStateInactive,
    ControlAEStateSearching,
    ControlAEStateConverged,
    ControlAEStateLocked,
    ControlAEStateFlashRequired,
    ControlAEStatePrecapture,
    ControlAEStateEnd
};

/// @brief ControlAFState
enum ControlAFStateValues
{
    ControlAFStateInactive,
    ControlAFStatePassiveScan,
    ControlAFStatePassiveFocused,
    ControlAFStateActiveScan,
    ControlAFStateFocusedLocked,
    ControlAFStateNotFocusedLocked,
    ControlAFStatePassiveUnfocused,
    ControlAFStateEnd
};

/// @brief ControlAWBState
enum ControlAWBStateValues
{
    ControlAWBStateInactive,
    ControlAWBStateSearching,
    ControlAWBStateConverged,
    ControlAWBStateLocked,
    ControlAWBStateEnd
};

/// @brief ControlAELockAvailable
enum ControlAELockAvailableValues
{
    ControlAELockAvailableFalse,
    ControlAELockAvailableTrue,
    ControlAELockAvailableEnd
};

/// @brief ControlAWBLockAvailable
enum ControlAWBLockAvailableValues
{
    ControlAWBLockAvailableFalse,
    ControlAWBLockAvailableTrue,
    ControlAWBLockAvailableEnd
};

/// @brief DemosaicMode
enum DemosaicModeValues
{
    DemosaicModeFast,
    DemosaicModeHighQuality,
    DemosaicModeEnd
};

/// @brief EdgeMode
enum EdgeModeValues
{
    EdgeModeOff,
    EdgeModeFast,
    EdgeModeHighQuality,
    EdgeModeZeroShutterLag,
    EdgeModeEnd
};

/// @brief FlashMode
enum FlashModeValues
{
    FlashModeOff,
    FlashModeSingle,
    FlashModeTorch,
    FlashModeEnd
};

/// @brief FlashState
enum FlashStateValues
{
    FlashStateUnavailable,
    FlashStateCharging,
    FlashStateReady,
    FlashStateFired,
    FlashStatePartial,
    FlashStateEnd
};

/// @brief FlashInfoAvailable
enum FlashInfoAvailableValues
{
    FlashInfoAvailableFalse,
    FlashInfoAvailableTrue,
    FlashInfoAvailableEnd
};

/// @brief HotPixelMode
enum HotPixelModeValues
{
    HotPixelModeOff,
    HotPixelModeFast,
    HotPixelModeHighQuality,
    HotPixelModeEnd
};

/// @brief LensOpticalStabilizationMode
enum LensOpticalStabilizationModeValues
{
    LensOpticalStabilizationModeOff,
    LensOpticalStabilizationModeOn,
    LensOpticalStabilizationModeEnd
};

/// @brief LensFacing
enum LensFacingValues
{
    LensFacingFront,
    LensFacingBack,
    LensFacingExternal,
    LensFacingInvalid
};

/// @brief LensState
enum LensStateValues
{
    LensStateStationary,
    LensStateMoving,
    LensStateEnd
};

/// @brief LensInfoFocusDistanceCalibration
enum LensInfoFocusDistanceCalibrationValues
{
    LensInfoFocusDistanceCalibrationUncalibrated,
    LensInfoFocusDistanceCalibrationApproximate,
    LensInfoFocusDistanceCalibrationCalibrated,
    LensInfoFocusDistanceCalibrationEnd
};

/// @brief NoiseReductionMode
enum NoiseReductionModeValues
{
    NoiseReductionModeOff,
    NoiseReductionModeFast,
    NoiseReductionModeHighQuality,
    NoiseReductionModeMinimal,
    NoiseReductionModeZeroShutterLag,
    NoiseReductionModeEnd
};

/// @brief QuirksPartialResult
enum QuirksPartialResultValues
{
    QuirksPartialResultFinal,
    QuirksPartialResultPartial,
    QuirksPartialResultEnd
};

/// @brief RequestMetadataMode
enum RequestMetadataModeValues
{
    RequestMetadataModeNone,
    RequestMetadataModeFull,
    RequestMetadataModeEnd
};

/// @brief RequestType
enum RequestTypeValues
{
    RequestTypeCapture,
    RequestTypeReprocess,
    RequestTypeEnd
};

/// @brief RequestAvailableCapabilities
enum RequestAvailableCapabilitiesValues
{
    RequestAvailableCapabilitiesBackwardCompatible,
    RequestAvailableCapabilitiesManualSensor,
    RequestAvailableCapabilitiesManualPostProcessing,
    RequestAvailableCapabilitiesRaw,
    RequestAvailableCapabilitiesPrivateReprocessing,
    RequestAvailableCapabilitiesReadSensorSettings,
    RequestAvailableCapabilitiesBurstCapture,
    RequestAvailableCapabilitiesYuvReprocessing,
    RequestAvailableCapabilitiesDepthOutput,
    RequestAvailableCapabilitiesConstrainedHighSpeedVideo,
    RequestAvailableCapabilitiesLogicalMultiCamera = 11,
    RequestAvailableCapabilitiesEnd
};

/// @brief ScalerAvailableFormats. This enum redefines the unnamed enum defined in system/graphics.h. Additional
///        information can be found there.
enum ScalerAvailableFormatsValues
{
    ScalerAvailableFormatsY8                        = 0x20203859,
    ScalerAvailableFormatsRaw10                     = 0x25,
    ScalerAvailableFormatsRaw16                     = 0x20,
    ScalerAvailableFormatsRawOpaque                 = 0x24,
    ScalerAvailableFormatsYV12                      = 0x32315659,
    ScalerAvailableFormatsYCrCb420Sp                = 0x11,
    ScalerAvailableFormatsImplementationDefined     = 0x22,
    ScalerAvailableFormatsYCbCr420888               = 0x23,
    ScalerAvailableFormatsBlob                      = 0x21,
    // Modify below MaxScalerFormats value is new format is added or deleted.
};

/// @brief Maximum number of available scaler formats
static const UINT MaxScalerFormats = 8;

/// @brief Maximum number of available scaler formats
static const UINT MaxOpaqueRawSizes = 8;


/// @brief ScalerAvailableStreamConfigurations
enum ScalerAvailableStreamConfigurationsValues
{
    ScalerAvailableStreamConfigurationsOutput,
    ScalerAvailableStreamConfigurationsInput,
    ScalerAvailableStreamConfigurationsEnd
};

/// @brief ScalerCroppingType
enum ScalerCroppingTypeValues
{
    ScalerCroppingTypeCenterOnly,
    ScalerCroppingTypeFreeform,
    ScalerCroppingTypeEnd
};

/// @brief SensorReferenceIlluminant1, SensorReferenceIlluminant2
enum SensorReferenceIlluminantValues
{
    SensorReferenceIlluminant1Daylight              = 1,
    SensorReferenceIlluminant1Fluorescent           = 2,
    SensorReferenceIlluminant1Tungsten              = 3,
    SensorReferenceIlluminant1Flash                 = 4,
    SensorReferenceIlluminant1FineWeather           = 9,
    SensorReferenceIlluminant1CloudyWeather         = 10,
    SensorReferenceIlluminant1Shade                 = 11,
    SensorReferenceIlluminant1DaylightFluorescent   = 12,
    SensorReferenceIlluminant1DayWhiteFluorescent   = 13,
    SensorReferenceIlluminant1CoolWhiteFluorescent  = 14,
    SensorReferenceIlluminant1WhiteFluorescent      = 15,
    SensorReferenceIlluminant1StandardA             = 17,
    SensorReferenceIlluminant1StandardB             = 18,
    SensorReferenceIlluminant1StandardC             = 19,
    SensorReferenceIlluminant1D55                   = 20,
    SensorReferenceIlluminant1D65                   = 21,
    SensorReferenceIlluminant1D75                   = 22,
    SensorReferenceIlluminant1D50                   = 23,
    SensorReferenceIlluminant1IsoStudioTungsten     = 24,
};

// Number of SensorReferenceIlluminant1 values
static const UINT SensorReferenceIlluminant1ValueCount = 19;

/// @brief SensorTestPatternMode
enum SensorTestPatternModeValues
{
    SensorTestPatternModeOff,
    SensorTestPatternModeSolidColor,
    SensorTestPatternModeColorBars,
    SensorTestPatternModeColorBarsFadeToGray,
    SensorTestPatternModePn9,
    SensorTestPatternModeCustom1                    = 256,
};

// Number of defined SensorTestPatternMode values.
// Allowed values in the system may be greater as custom patterns are allowed.
static const UINT SensorTestPatternModeValuesCount = 6;

/// @brief SensorInfoColorFilterArrangement
enum SensorInfoColorFilterArrangementValues
{
    SensorInfoColorFilterArrangementRggb,
    SensorInfoColorFilterArrangementGrbg,
    SensorInfoColorFilterArrangementGbrg,
    SensorInfoColorFilterArrangementBggr,
    SensorInfoColorFilterArrangementRgb,
    SensorInfoColorFilterArrangementY,
    SensorInfoColorFilterArrangementEnd
};

/// @brief SensorInfoTimestampSource
enum SensorInfoTimestampSourceValues
{
    SensorInfoTimestampSourceUnknown,
    SensorInfoTimestampSourceRealtime,
    SensorInfoTimestampSourceEnd
};

/// @brief SensorInfoLensShadingApplied
enum SensorInfoLensShadingAppliedValues
{
    SensorInfoLensShadingAppliedFalse,
    SensorInfoLensShadingAppliedTrue,
    SensorInfoLensShadingAppliedEnd
};

/// @brief ShadingMode
enum ShadingModeValues
{
    ShadingModeOff,
    ShadingModeFast,
    ShadingModeHighQuality,
    ShadingModeEnd
};

/// @brief StatisticsFaceDetectMode
enum StatisticsFaceDetectModeValues
{
    StatisticsFaceDetectModeOff,
    StatisticsFaceDetectModeSimple,
    StatisticsFaceDetectModeFull,
    StatisticsFaceDetectModeEnd
};

/// @brief StatisticsHistogramMode
enum StatisticsHistogramModeValues
{
    StatisticsHistogramModeOff,
    StatisticsHistogramModeOn,
    StatisticsHistogramModeEnd
};

/// @brief StatisticsSharpnessMapMode
enum StatisticsSharpnessMapModeValues
{
    StatisticsSharpnessMapModeOff,
    StatisticsSharpnessMapModeOn,
    StatisticsSharpnessMapModeEnd
};

/// @brief StatisticsHotPixelMapMode
enum StatisticsHotPixelMapModeValues
{
    StatisticsHotPixelMapModeOff,
    StatisticsHotPixelMapModeOn,
    StatisticsHotPixelMapModeEnd
};

/// @brief StatisticsSceneFlicker
enum StatisticsSceneFlickerValues
{
    StatisticsSceneFlickerNone,
    StatisticsSceneFlicker50Hz,
    StatisticsSceneFlicker60Hz,
    StatisticsSceneFlickerEnd
};

/// @brief StatisticsLensShadingMapMode
enum StatisticsLensShadingMapModeValues
{
    StatisticsLensShadingMapModeOff,
    StatisticsLensShadingMapModeOn,
    StatisticsLensShadingMapModeEnd
};

/// @brief TonemapMode
enum TonemapModeValues
{
    TonemapModeContrastCurve,
    TonemapModeFast,
    TonemapModeHighQuality,
    TonemapModeGammaValue,
    TonemapModePresetCurve,
    TonemapModeEnd
};

/// @brief TonemapPresetCurve
enum TonemapPresetCurveValues
{
    TonemapPresetCurveSrgb,
    TonemapPresetCurveRec709,
    TonemapPresetCurveEnd
};

/// @brief LedTransmit
enum LedTransmitValues
{
    LedTransmitOff,
    LedTransmitOn,
    LedTransmitEnd
};

/// @brief LedAvailableLeds
enum LedAvailableLedsValues
{
    LedAvailableLedsTransmit,
    LedAvailableLedsEnd
};

/// @brief InfoSupportedHardwareLevel
enum InfoSupportedHardwareLevelValues
{
    InfoSupportedHardwareLevelLimited,
    InfoSupportedHardwareLevelFull,
    InfoSupportedHardwareLevelLegacy,
    InfoSupportedHardwareLevel3,
    InfoSupportedHardwareLevelEnd
};

/// @brief BlackLevelLock
enum BlackLevelLockValues
{
    BlackLevelLockOff,
    BlackLevelLockOn,
    BlackLevelLockEnd
};

/// @brief SyncFrameNumber
enum SyncFrameNumberValues
{
    SyncFrameNumberConverging                       = -1,
    SyncFrameNumberUnknown                          = -2,
};

// Number of SyncFrameNumber values.
static const UINT SyncFrameNumberValuesCount = 2;

/// @brief SyncMaxLatency
enum SyncMaxLatencyValues
{
    SyncMaxLatencyPerFrameControl                   = 0,
    SyncMaxLatencyUnknown                           = -1,
};

// Number of SyncMaxLatency values.
static const UINT SyncMaxLatencyValuesCount = 2;

/// @brief DepthAvailableFormats.
enum DepthAvailableFormatsValues
{
    DepthAvailableFormatsRawDepth              = 0x20,
    DepthAvailableFormatsY16                   = 0x20363159,
    DepthAvailableFormatsBlob                  = 0x21,
    DepthAvailableFormatsImplementationDefined = 0x22,
};

// Maximum number of available depth formats
static const UINT MaxDepthFormats = 4;

/// @brief DepthAvailableDepthStreamConfigurations
enum DepthAvailableDepthStreamConfigurationsValues
{
    DepthAvailableDepthStreamConfigurationsOutput,
    DepthAvailableDepthStreamConfigurationsInput,
    DepthAvailableDepthStreamConfigurationsEnd
};

/// @brief DepthDepthIsExclusive
enum DepthDepthIsExclusiveValues
{
    DepthDepthIsExclusiveFalse,
    DepthDepthIsExclusiveTrue,
    DepthDepthIsExclusiveEnd
};

/// @brief ExposureMeteringAvailMode
enum ExposureMeteringAvailableModes
{
    ExposureMeteringFrameAverage,
    ExposureMeteringCenterWeighted,
    ExposureMeteringSpotMetering,
    ExposureMeteringSmartMetering,
    ExposureMeteringUserMetering,
    ExposureMeteringSpotMeteringAdv,
    ExposureMeteringCenterWeightedAdv,
    ExposureMeteringCustom,
    ExposureMeteringEnd
};

/// @brief IsoAvailModes
enum ISOModes
{
    ISOModeAuto,
    ISOModeDeblur,
    ISOMode100,
    ISOMode200,
    ISOMode400,
    ISOMode800,
    ISOMode1600,
    ISOMode3200,
    ISOModeEnd
};

/// @brief InstantAecAvailableModes
enum InstantAecAvailableModes
{
    InstantAecNormalConvergence,
    InstantAecAggressiveConvergence,
    InstantAecFastConvergence,
    InstantAecEnd
};

/// @brief InstantVideoHDRModes
enum VideoHDRAvailableModes
{
    VideoHdrOff,
    VideoHdrOn,
    VideoHdrEnd
};

/// @brief ICATransformType
enum IPEICATransformType
{
    DefaultTransform    = 0,
    InputTransform      = 1,
    ReferenceTransform  = 2
};

/// @brief DistortionCorrectionModes
enum DistortionCorrectionModes
{
    DistortionCorrectionModesOff    = 0,
    DistortionCorrectionModesFast,
    DistortionCorrectionModesHighQuality,
    DistortionCorrectionModesEnd
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraMetadataTagToString
///
/// @brief  Returns a pointer to a const string which is a human readible representation of the metadata tag
///
/// @param  tag         Tag to stringize
/// @param  ppTagPrefix Pointer to a prefix string for the tag name
/// @param  ppTagName   Pointer to the string for the tag name
///
/// @return CamxResult status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CameraMetadataTagToString(
    CameraMetadataTag   tag,
    const CHAR**        ppTagPrefix,
    const CHAR**        ppTagName);

CAMX_NAMESPACE_END

#endif // CAMXHAL3METADATATAGS_H
