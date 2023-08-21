////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3metadatatags.cpp
/// @brief Contains static_asserts used to ensure binary compatibility between HAL3 API types and the corresponding CamX type.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE GR002:  Static asserts on one line exceed max line width.
#include <system/camera_metadata_tags.h>

#include "camxhal3metadatatags.h"
#include "camxincs.h"

CAMX_NAMESPACE_BEGIN

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_MODE) == static_cast<INT>(ColorCorrectionMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_TRANSFORM) == static_cast<INT>(ColorCorrectionTransform));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_GAINS) == static_cast<INT>(ColorCorrectionGains));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_ABERRATION_MODE) == static_cast<INT>(ColorCorrectionAberrationMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_AVAILABLE_ABERRATION_MODES) == static_cast<INT>(ColorCorrectionAvailableAberrationModes));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_ANTIBANDING_MODE) == static_cast<INT>(ControlAEAntibandingMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION) == static_cast<INT>(ControlAEExposureCompensation));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_LOCK) == static_cast<INT>(ControlAELock));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_MODE) == static_cast<INT>(ControlAEMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_REGIONS) == static_cast<INT>(ControlAERegions));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_TARGET_FPS_RANGE) == static_cast<INT>(ControlAETargetFpsRange));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER) == static_cast<INT>(ControlAEPrecaptureTrigger));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_MODE) == static_cast<INT>(ControlAFMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_REGIONS) == static_cast<INT>(ControlAFRegions));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_TRIGGER) == static_cast<INT>(ControlAFTrigger));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_LOCK) == static_cast<INT>(ControlAWBLock));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_MODE) == static_cast<INT>(ControlAWBMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_REGIONS) == static_cast<INT>(ControlAWBRegions));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_CAPTURE_INTENT) == static_cast<INT>(ControlCaptureIntent));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_EFFECT_MODE) == static_cast<INT>(ControlEffectMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_MODE) == static_cast<INT>(ControlMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE) == static_cast<INT>(ControlSceneMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE) == static_cast<INT>(ControlVideoStabilizationMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES) == static_cast<INT>(ControlAEAvailableAntibandingModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_AVAILABLE_MODES) == static_cast<INT>(ControlAEAvailableModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES) == static_cast<INT>(ControlAEAvailableTargetFPSRanges));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_COMPENSATION_RANGE) == static_cast<INT>(ControlAECompensationRange));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_COMPENSATION_STEP) == static_cast<INT>(ControlAECompensationStep));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_AVAILABLE_MODES) == static_cast<INT>(ControlAFAvailableModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AVAILABLE_EFFECTS) == static_cast<INT>(ControlAvailableEffects));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AVAILABLE_SCENE_MODES) == static_cast<INT>(ControlAvailableSceneModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AVAILABLE_VIDEO_STABILIZATION_MODES) == static_cast<INT>(ControlAvailableVideoStabilizationModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_AVAILABLE_MODES) == static_cast<INT>(ControlAWBAvailableModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_MAX_REGIONS) == static_cast<INT>(ControlMaxRegions));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_OVERRIDES) == static_cast<INT>(ControlSceneModeOverrides));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_PRECAPTURE_ID) == static_cast<INT>(ControlAEPrecaptureId));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_STATE) == static_cast<INT>(ControlAEState));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_STATE) == static_cast<INT>(ControlAFState));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_TRIGGER_ID) == static_cast<INT>(ControlAFTriggerId));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_STATE) == static_cast<INT>(ControlAWBState));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AVAILABLE_HIGH_SPEED_VIDEO_CONFIGURATIONS) == static_cast<INT>(ControlAvailableHighSpeedVideoConfigurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_LOCK_AVAILABLE) == static_cast<INT>(ControlAELockAvailable));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_LOCK_AVAILABLE) == static_cast<INT>(ControlAWBLockAvailable));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AVAILABLE_MODES) == static_cast<INT>(ControlAvailableModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST_RANGE) == static_cast<INT>(ControlPostRawSensitivityBoostRange));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_POST_RAW_SENSITIVITY_BOOST) == static_cast<INT>(ControlPostRawSensitivityBoost));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_ENABLE_ZSL) == static_cast<INT>(ControlZslEnable));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEMOSAIC_MODE) == static_cast<INT>(DemosaicMode));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_EDGE_MODE) == static_cast<INT>(EdgeMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_EDGE_STRENGTH) == static_cast<INT>(EdgeStrength));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_EDGE_AVAILABLE_EDGE_MODES) == static_cast<INT>(EdgeAvailableEdgeModes));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_FIRING_POWER) == static_cast<INT>(FlashFiringPower));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_FIRING_TIME) == static_cast<INT>(FlashFiringTime));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_MODE) == static_cast<INT>(FlashMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_COLOR_TEMPERATURE) == static_cast<INT>(FlashColorTemperature));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_MAX_ENERGY) == static_cast<INT>(FlashMaxEnergy));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_STATE) == static_cast<INT>(FlashState));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_INFO_AVAILABLE) == static_cast<INT>(FlashInfoAvailable));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_INFO_CHARGE_DURATION) == static_cast<INT>(FlashInfoChargeDuration));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_HOT_PIXEL_MODE) == static_cast<INT>(HotPixelMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_HOT_PIXEL_AVAILABLE_HOT_PIXEL_MODES) == static_cast<INT>(HotPixelAvailableHotPixelModes));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_JPEG_GPS_COORDINATES) == static_cast<INT>(JPEGGpsCoordinates));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_JPEG_GPS_PROCESSING_METHOD) == static_cast<INT>(JPEGGpsProcessingMethod));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_JPEG_GPS_TIMESTAMP) == static_cast<INT>(JPEGGpsTimestamp));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_JPEG_ORIENTATION) == static_cast<INT>(JPEGOrientation));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_JPEG_QUALITY) == static_cast<INT>(JPEGQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_JPEG_THUMBNAIL_QUALITY) == static_cast<INT>(JPEGThumbnailQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_JPEG_THUMBNAIL_SIZE) == static_cast<INT>(JPEGThumbnailSize));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_JPEG_AVAILABLE_THUMBNAIL_SIZES) == static_cast<INT>(JPEGAvailableThumbnailSizes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_JPEG_MAX_SIZE) == static_cast<INT>(JPEGMaxSize));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_JPEG_SIZE) == static_cast<INT>(JPEGSize));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_APERTURE) == static_cast<INT>(LensAperture));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_FILTER_DENSITY) == static_cast<INT>(LensFilterDensity));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_FOCAL_LENGTH) == static_cast<INT>(LensFocalLength));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_FOCUS_DISTANCE) == static_cast<INT>(LensFocusDistance));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_OPTICAL_STABILIZATION_MODE) == static_cast<INT>(LensOpticalStabilizationMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_FACING) == static_cast<INT>(LensFacing));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_POSE_ROTATION) == static_cast<INT>(LensPoseRotation));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_POSE_TRANSLATION) == static_cast<INT>(LensPoseTranslation));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_FOCUS_RANGE) == static_cast<INT>(LensFocusRange));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_STATE) == static_cast<INT>(LensState));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INTRINSIC_CALIBRATION) == static_cast<INT>(LensIntrinsicCalibration));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_RADIAL_DISTORTION) == static_cast<INT>(LensRadialDistortion));
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_POSE_REFERENCE) == static_cast<INT>(LensPoseReference));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_DISTORTION) == static_cast<INT>(LensDistortion));
#endif // Android-P or better
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_AVAILABLE_APERTURES) == static_cast<INT>(LensInfoAvailableApertures));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_AVAILABLE_FILTER_DENSITIES) == static_cast<INT>(LensInfoAvailableFilterDensities));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_AVAILABLE_FOCAL_LENGTHS) == static_cast<INT>(LensInfoAvailableFocalLengths));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_AVAILABLE_OPTICAL_STABILIZATION) == static_cast<INT>(LensInfoAvailableOpticalStabilization));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_HYPERFOCAL_DISTANCE) == static_cast<INT>(LensInfoHyperfocalDistance));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_MINIMUM_FOCUS_DISTANCE) == static_cast<INT>(LensInfoMinimumFocusDistance));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_SHADING_MAP_SIZE) == static_cast<INT>(LensInfoShadingMapSize));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION) == static_cast<INT>(LensInfoFocusDistanceCalibration));
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LOGICAL_MULTI_CAMERA_PHYSICAL_IDS) == static_cast<INT>(LogicalMultiCameraPhysicalIDs));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LOGICAL_MULTI_CAMERA_SENSOR_SYNC_TYPE) == static_cast<INT>(LogicalMultiCameraSensorSyncType));
#endif // Android-P or better
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_NOISE_REDUCTION_MODE) == static_cast<INT>(NoiseReductionMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_NOISE_REDUCTION_STRENGTH) == static_cast<INT>(NoiseReductionStrength));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_NOISE_REDUCTION_AVAILABLE_NOISE_REDUCTION_MODES) == static_cast<INT>(NoiseReductionAvailableNoiseReductionModes));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_QUIRKS_METERING_CROP_REGION) == static_cast<INT>(QuirksMeteringCropRegion));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_QUIRKS_TRIGGER_AF_WITH_AUTO) == static_cast<INT>(QuirksTriggerAFWithAuto));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_QUIRKS_USE_ZSL_FORMAT) == static_cast<INT>(QuirksUseZslFormat));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_QUIRKS_USE_PARTIAL_RESULT) == static_cast<INT>(QuirksUsePartialResult));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_QUIRKS_PARTIAL_RESULT) == static_cast<INT>(QuirksPartialResult));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_FRAME_COUNT) == static_cast<INT>(RequestFrameCount));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_ID) == static_cast<INT>(RequestId));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_INPUT_STREAMS) == static_cast<INT>(RequestInputStreams));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_METADATA_MODE) == static_cast<INT>(RequestMetadataMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_OUTPUT_STREAMS) == static_cast<INT>(RequestOutputStreams));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_TYPE) == static_cast<INT>(RequestType));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS) == static_cast<INT>(RequestMaxNumOutputStreams));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_MAX_NUM_REPROCESS_STREAMS) == static_cast<INT>(RequestMaxNumReprocessStreams));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_MAX_NUM_INPUT_STREAMS) == static_cast<INT>(RequestMaxNumInputStreams));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_PIPELINE_DEPTH) == static_cast<INT>(RequestPipelineDepth));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_PIPELINE_MAX_DEPTH) == static_cast<INT>(RequestPipelineMaxDepth));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_PARTIAL_RESULT_COUNT) == static_cast<INT>(RequestPartialResultCount));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES) == static_cast<INT>(RequestAvailableCapabilities));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_REQUEST_KEYS) == static_cast<INT>(RequestAvailableRequestKeys));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_RESULT_KEYS) == static_cast<INT>(RequestAvailableResultKeys));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CHARACTERISTICS_KEYS) == static_cast<INT>(RequestAvailableCharacteristicsKeys));
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_SESSION_KEYS) == static_cast<INT>(RequestAvailableSessionKeys));
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_CROP_REGION) == static_cast<INT>(ScalerCropRegion));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_FORMATS) == static_cast<INT>(ScalerAvailableFormats));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_JPEG_MIN_DURATIONS) == static_cast<INT>(ScalerAvailableJPEGMinDurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_JPEG_SIZES) == static_cast<INT>(ScalerAvailableJPEGSizes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_MAX_DIGITAL_ZOOM) == static_cast<INT>(ScalerAvailableMaxDigitalZoom));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_PROCESSED_MIN_DURATIONS) == static_cast<INT>(ScalerAvailableProcessedMinDurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_PROCESSED_SIZES) == static_cast<INT>(ScalerAvailableProcessedSizes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_RAW_MIN_DURATIONS) == static_cast<INT>(ScalerAvailableRawMinDurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_RAW_SIZES) == static_cast<INT>(ScalerAvailableRawSizes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_INPUT_OUTPUT_FORMATS_MAP) == static_cast<INT>(ScalerAvailableInputOutputFormatsMap));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS) == static_cast<INT>(ScalerAvailableStreamConfigurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_MIN_FRAME_DURATIONS) == static_cast<INT>(ScalerAvailableMinFrameDurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_STALL_DURATIONS) == static_cast<INT>(ScalerAvailableStallDurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_CROPPING_TYPE) == static_cast<INT>(ScalerCroppingType));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_EXPOSURE_TIME) == static_cast<INT>(SensorExposureTime));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_FRAME_DURATION) == static_cast<INT>(SensorFrameDuration));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_SENSITIVITY) == static_cast<INT>(SensorSensitivity));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1) == static_cast<INT>(SensorReferenceIlluminant1));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT2) == static_cast<INT>(SensorReferenceIlluminant2));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_CALIBRATION_TRANSFORM1) == static_cast<INT>(SensorCalibrationTransform1));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_CALIBRATION_TRANSFORM2) == static_cast<INT>(SensorCalibrationTransform2));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_COLOR_TRANSFORM1) == static_cast<INT>(SensorColorTransform1));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_COLOR_TRANSFORM2) == static_cast<INT>(SensorColorTransform2));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_FORWARD_MATRIX1) == static_cast<INT>(SensorForwardMatrix1));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_FORWARD_MATRIX2) == static_cast<INT>(SensorForwardMatrix2));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_BASE_GAIN_FACTOR) == static_cast<INT>(SensorBaseGainFactor));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_BLACK_LEVEL_PATTERN) == static_cast<INT>(SensorBlackLevelPattern));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_MAX_ANALOG_SENSITIVITY) == static_cast<INT>(SensorMaxAnalogSensitivity));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_ORIENTATION) == static_cast<INT>(SensorOrientation));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_PROFILE_HUE_SAT_MAP_DIMENSIONS) == static_cast<INT>(SensorProfileHueSaturationMapDimensions));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_TIMESTAMP) == static_cast<INT>(SensorTimestamp));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_TEMPERATURE) == static_cast<INT>(SensorTemperature));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_NEUTRAL_COLOR_POINT) == static_cast<INT>(SensorNeutralColorPoint));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_NOISE_PROFILE) == static_cast<INT>(SensorNoiseProfile));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_PROFILE_HUE_SAT_MAP) == static_cast<INT>(SensorProfileHueSaturationMap));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_PROFILE_TONE_CURVE) == static_cast<INT>(SensorProfileToneCurve));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_GREEN_SPLIT) == static_cast<INT>(SensorGreenSplit));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_TEST_PATTERN_DATA) == static_cast<INT>(SensorTestPatternData));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_TEST_PATTERN_MODE) == static_cast<INT>(SensorTestPatternMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_AVAILABLE_TEST_PATTERN_MODES) == static_cast<INT>(SensorAvailableTestPatternModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_ROLLING_SHUTTER_SKEW) == static_cast<INT>(SensorRollingShutterSkew));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_OPTICAL_BLACK_REGIONS) == static_cast<INT>(SensorOpticalBlackRegions));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_DYNAMIC_BLACK_LEVEL) == static_cast<INT>(SensorDynamicBlackLevel));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_DYNAMIC_WHITE_LEVEL) == static_cast<INT>(SensorDynamicWhiteLevel));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_OPAQUE_RAW_SIZE) == static_cast<INT>(SensorOpaqueRawSize));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE) == static_cast<INT>(SensorInfoActiveArraySize));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_SENSITIVITY_RANGE) == static_cast<INT>(SensorInfoSensitivityRange));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT) == static_cast<INT>(SensorInfoColorFilterArrangement));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_EXPOSURE_TIME_RANGE) == static_cast<INT>(SensorInfoExposureTimeRange));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_MAX_FRAME_DURATION) == static_cast<INT>(SensorInfoMaxFrameDuration));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_PHYSICAL_SIZE) == static_cast<INT>(SensorInfoPhysicalSize));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE) == static_cast<INT>(SensorInfoPixelArraySize));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_WHITE_LEVEL) == static_cast<INT>(SensorInfoWhiteLevel));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE) == static_cast<INT>(SensorInfoTimestampSource));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_LENS_SHADING_APPLIED) == static_cast<INT>(SensorInfoLensShadingApplied));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_PRE_CORRECTION_ACTIVE_ARRAY_SIZE) == static_cast<INT>(SensorInfoPreCorrectionActiveArraySize));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SHADING_MODE) == static_cast<INT>(ShadingMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SHADING_STRENGTH) == static_cast<INT>(ShadingStrength));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SHADING_AVAILABLE_MODES) == static_cast<INT>(ShadingAvailableModes));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_FACE_DETECT_MODE) == static_cast<INT>(StatisticsFaceDetectMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_HISTOGRAM_MODE) == static_cast<INT>(StatisticsHistogramMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_SHARPNESS_MAP_MODE) == static_cast<INT>(StatisticsSharpnessMapMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE) == static_cast<INT>(StatisticsHotPixelMapMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_FACE_IDS) == static_cast<INT>(StatisticsFaceIds));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_FACE_LANDMARKS) == static_cast<INT>(StatisticsFaceLandmarks));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_FACE_RECTANGLES) == static_cast<INT>(StatisticsFaceRectangles));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_FACE_SCORES) == static_cast<INT>(StatisticsFaceScores));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_HISTOGRAM) == static_cast<INT>(StatisticsHistogram));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_SHARPNESS_MAP) == static_cast<INT>(StatisticsSharpnessMap));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_LENS_SHADING_CORRECTION_MAP) == static_cast<INT>(StatisticsLensShadingCorrectionMap));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_LENS_SHADING_MAP) == static_cast<INT>(StatisticsLensShadingMap));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_PREDICTED_COLOR_GAINS) == static_cast<INT>(StatisticsPredictedColorGains));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_PREDICTED_COLOR_TRANSFORM) == static_cast<INT>(StatisticsPredictedColorTransform));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_SCENE_FLICKER) == static_cast<INT>(StatisticsSceneFlicker));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_HOT_PIXEL_MAP) == static_cast<INT>(StatisticsHotPixelMap));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE) == static_cast<INT>(StatisticsLensShadingMapMode));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_INFO_AVAILABLE_FACE_DETECT_MODES) == static_cast<INT>(StatisticsInfoAvailableFaceDetectModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_INFO_HISTOGRAM_BUCKET_COUNT) == static_cast<INT>(StatisticsInfoHistogramBucketCount));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_INFO_MAX_FACE_COUNT) == static_cast<INT>(StatisticsInfoMaxFaceCount));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_INFO_MAX_HISTOGRAM_COUNT) == static_cast<INT>(StatisticsInfoMaxHistogramCount));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_INFO_MAX_SHARPNESS_MAP_VALUE) == static_cast<INT>(StatisticsInfoMaxSharpnessMapValue));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_INFO_SHARPNESS_MAP_SIZE) == static_cast<INT>(StatisticsInfoSharpnessMapSize));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_INFO_AVAILABLE_HOT_PIXEL_MAP_MODES) == static_cast<INT>(StatisticsInfoAvailableHotPixelMapModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_INFO_AVAILABLE_LENS_SHADING_MAP_MODES) == static_cast<INT>(StatisticsInfoAvailableLensShadingMapModes));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_CURVE_BLUE) == static_cast<INT>(TonemapCurveBlue));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_CURVE_GREEN) == static_cast<INT>(TonemapCurveGreen));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_CURVE_RED) == static_cast<INT>(TonemapCurveRed));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_MODE) == static_cast<INT>(TonemapMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_MAX_CURVE_POINTS) == static_cast<INT>(TonemapMaxCurvePoints));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_AVAILABLE_TONE_MAP_MODES) == static_cast<INT>(TonemapAvailableToneMapModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_GAMMA) == static_cast<INT>(TonemapGamma));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_PRESET_CURVE) == static_cast<INT>(TonemapPresetCurve));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LED_TRANSMIT) == static_cast<INT>(LedTransmit));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LED_AVAILABLE_LEDS) == static_cast<INT>(LedAvailableLeds));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL) == static_cast<INT>(InfoSupportedHardwareLevel));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_BLACK_LEVEL_LOCK) == static_cast<INT>(BlackLevelLock));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SYNC_FRAME_NUMBER) == static_cast<INT>(SyncFrameNumber));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SYNC_MAX_LATENCY) == static_cast<INT>(SyncMaxLatency));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REPROCESS_EFFECTIVE_EXPOSURE_FACTOR) == static_cast<INT>(ReprocessEffectiveExposureFactor));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REPROCESS_MAX_CAPTURE_STALL) == static_cast<INT>(ReprocessMaxCaptureStall));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEPTH_MAX_DEPTH_SAMPLES) == static_cast<INT>(DepthMaxDepthSamples));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS) == static_cast<INT>(DepthAvailableDepthStreamConfigurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEPTH_AVAILABLE_DEPTH_MIN_FRAME_DURATIONS) == static_cast<INT>(DepthAvailableDepthMinFrameDurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEPTH_AVAILABLE_DEPTH_STALL_DURATIONS) == static_cast<INT>(DepthAvailableDepthStallDurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEPTH_DEPTH_IS_EXCLUSIVE) == static_cast<INT>(DepthDepthIsExclusive));

CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_MODE_TRANSFORM_MATRIX) == static_cast<INT>(ColorCorrectionModeTransformMatrix));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_MODE_FAST) == static_cast<INT>(ColorCorrectionModeFast));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_MODE_HIGH_QUALITY) == static_cast<INT>(ColorCorrectionModeHighQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_ABERRATION_MODE_OFF) == static_cast<INT>(ColorCorrectionAberrationModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_ABERRATION_MODE_FAST) == static_cast<INT>(ColorCorrectionAberrationModeFast));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_COLOR_CORRECTION_ABERRATION_MODE_HIGH_QUALITY) == static_cast<INT>(ColorCorrectionAberrationModeHighQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_ANTIBANDING_MODE_OFF) == static_cast<INT>(ControlAEAntibandingModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_ANTIBANDING_MODE_50HZ) == static_cast<INT>(ControlAEAntibandingMode50Hz));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_ANTIBANDING_MODE_60HZ) == static_cast<INT>(ControlAEAntibandingMode60Hz));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO) == static_cast<INT>(ControlAEAntibandingModeAuto));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_LOCK_OFF) == static_cast<INT>(ControlAELockOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_LOCK_ON) == static_cast<INT>(ControlAELockOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_MODE_OFF) == static_cast<INT>(ControlAEModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_MODE_ON) == static_cast<INT>(ControlAEModeOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH) == static_cast<INT>(ControlAEModeOnAutoFlash));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH) == static_cast<INT>(ControlAEModeOnAlwaysFlash));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE) == static_cast<INT>(ControlAEModeOnAutoFlashRedeye));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_IDLE) == static_cast<INT>(ControlAEPrecaptureTriggerIdle));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_START) == static_cast<INT>(ControlAEPrecaptureTriggerStart));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_PRECAPTURE_TRIGGER_CANCEL) == static_cast<INT>(ControlAEPrecaptureTriggerCancel));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_MODE_OFF) == static_cast<INT>(ControlAFModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_MODE_AUTO) == static_cast<INT>(ControlAFModeAuto));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_MODE_MACRO) == static_cast<INT>(ControlAFModeMacro));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO) == static_cast<INT>(ControlAFModeContinuousVideo));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE) == static_cast<INT>(ControlAFModeContinuousPicture));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_MODE_EDOF) == static_cast<INT>(ControlAFModeEdof));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_TRIGGER_IDLE) == static_cast<INT>(ControlAFTriggerIdle));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_TRIGGER_START) == static_cast<INT>(ControlAFTriggerStart));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_TRIGGER_CANCEL) == static_cast<INT>(ControlAFTriggerCancel));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_LOCK_OFF) == static_cast<INT>(ControlAWBLockOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_LOCK_ON) == static_cast<INT>(ControlAWBLockOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_MODE_OFF) == static_cast<INT>(ControlAWBModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_MODE_AUTO) == static_cast<INT>(ControlAWBModeAuto));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_MODE_INCANDESCENT) == static_cast<INT>(ControlAWBModeIncandescent));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_MODE_FLUORESCENT) == static_cast<INT>(ControlAWBModeFluorescent));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_MODE_WARM_FLUORESCENT) == static_cast<INT>(ControlAWBModeWarmFluorescent));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_MODE_DAYLIGHT) == static_cast<INT>(ControlAWBModeDaylight));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT) == static_cast<INT>(ControlAWBModeCloudyDaylight));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_MODE_TWILIGHT) == static_cast<INT>(ControlAWBModeTwilight));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_MODE_SHADE) == static_cast<INT>(ControlAWBModeShade));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_CAPTURE_INTENT_CUSTOM) == static_cast<INT>(ControlCaptureIntentCustom));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW) == static_cast<INT>(ControlCaptureIntentPreview));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_CAPTURE_INTENT_STILL_CAPTURE) == static_cast<INT>(ControlCaptureIntentStillCapture));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD) == static_cast<INT>(ControlCaptureIntentVideoRecord));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT) == static_cast<INT>(ControlCaptureIntentVideoSnapshot));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_CAPTURE_INTENT_ZERO_SHUTTER_LAG) == static_cast<INT>(ControlCaptureIntentZeroShutterLag));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_CAPTURE_INTENT_MANUAL) == static_cast<INT>(ControlCaptureIntentManual));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_EFFECT_MODE_OFF) == static_cast<INT>(ControlEffectModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_EFFECT_MODE_MONO) == static_cast<INT>(ControlEffectModeMono));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_EFFECT_MODE_NEGATIVE) == static_cast<INT>(ControlEffectModeNegative));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_EFFECT_MODE_SOLARIZE) == static_cast<INT>(ControlEffectModeSolarize));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_EFFECT_MODE_SEPIA) == static_cast<INT>(ControlEffectModeSepia));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_EFFECT_MODE_POSTERIZE) == static_cast<INT>(ControlEffectModePosterize));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_EFFECT_MODE_WHITEBOARD) == static_cast<INT>(ControlEffectModeWhiteboard));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_EFFECT_MODE_BLACKBOARD) == static_cast<INT>(ControlEffectModeBlackboard));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_EFFECT_MODE_AQUA) == static_cast<INT>(ControlEffectModeAqua));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_MODE_OFF) == static_cast<INT>(ControlModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_MODE_AUTO) == static_cast<INT>(ControlModeAuto));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_MODE_USE_SCENE_MODE) == static_cast<INT>(ControlModeUseSceneMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_MODE_OFF_KEEP_STATE) == static_cast<INT>(ControlModeOffKeepState));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_DISABLED) == static_cast<INT>(ControlSceneModeDisabled));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY) == static_cast<INT>(ControlSceneModeFacePriority));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_ACTION) == static_cast<INT>(ControlSceneModeAction));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_PORTRAIT) == static_cast<INT>(ControlSceneModePortrait));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_LANDSCAPE) == static_cast<INT>(ControlSceneModeLandscape));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_NIGHT) == static_cast<INT>(ControlSceneModeNight));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_NIGHT_PORTRAIT) == static_cast<INT>(ControlSceneModeNightPortrait));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_THEATRE) == static_cast<INT>(ControlSceneModeTheatre));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_BEACH) == static_cast<INT>(ControlSceneModeBeach));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_SNOW) == static_cast<INT>(ControlSceneModeSnow));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_SUNSET) == static_cast<INT>(ControlSceneModeSunset));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_STEADYPHOTO) == static_cast<INT>(ControlSceneModeSteadyphoto));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_FIREWORKS) == static_cast<INT>(ControlSceneModeFireworks));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_SPORTS) == static_cast<INT>(ControlSceneModeSports));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_PARTY) == static_cast<INT>(ControlSceneModeParty));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_CANDLELIGHT) == static_cast<INT>(ControlSceneModeCandlelight));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_BARCODE) == static_cast<INT>(ControlSceneModeBarcode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_HIGH_SPEED_VIDEO) == static_cast<INT>(ControlSceneModeHighSpeedVideo));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_HDR) == static_cast<INT>(ControlSceneModeHdr));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_SCENE_MODE_FACE_PRIORITY_LOW_LIGHT) == static_cast<INT>(ControlSceneModeFacePriorityLowLight));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_OFF) == static_cast<INT>(ControlVideoStabilizationModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_VIDEO_STABILIZATION_MODE_ON) == static_cast<INT>(ControlVideoStabilizationModeOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_STATE_INACTIVE) == static_cast<INT>(ControlAEStateInactive));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_STATE_SEARCHING) == static_cast<INT>(ControlAEStateSearching));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_STATE_CONVERGED) == static_cast<INT>(ControlAEStateConverged));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_STATE_LOCKED) == static_cast<INT>(ControlAEStateLocked));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_STATE_FLASH_REQUIRED) == static_cast<INT>(ControlAEStateFlashRequired));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_STATE_PRECAPTURE) == static_cast<INT>(ControlAEStatePrecapture));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_STATE_INACTIVE) == static_cast<INT>(ControlAFStateInactive));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN) == static_cast<INT>(ControlAFStatePassiveScan));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_STATE_PASSIVE_FOCUSED) == static_cast<INT>(ControlAFStatePassiveFocused));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_STATE_ACTIVE_SCAN) == static_cast<INT>(ControlAFStateActiveScan));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED) == static_cast<INT>(ControlAFStateFocusedLocked));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED) == static_cast<INT>(ControlAFStateNotFocusedLocked));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AF_STATE_PASSIVE_UNFOCUSED) == static_cast<INT>(ControlAFStatePassiveUnfocused));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_STATE_INACTIVE) == static_cast<INT>(ControlAWBStateInactive));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_STATE_SEARCHING) == static_cast<INT>(ControlAWBStateSearching));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_STATE_CONVERGED) == static_cast<INT>(ControlAWBStateConverged));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_STATE_LOCKED) == static_cast<INT>(ControlAWBStateLocked));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_LOCK_AVAILABLE_FALSE) == static_cast<INT>(ControlAELockAvailableFalse));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AE_LOCK_AVAILABLE_TRUE) == static_cast<INT>(ControlAELockAvailableTrue));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_LOCK_AVAILABLE_FALSE) == static_cast<INT>(ControlAWBLockAvailableFalse));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_CONTROL_AWB_LOCK_AVAILABLE_TRUE) == static_cast<INT>(ControlAWBLockAvailableTrue));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEMOSAIC_MODE_FAST) == static_cast<INT>(DemosaicModeFast));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEMOSAIC_MODE_HIGH_QUALITY) == static_cast<INT>(DemosaicModeHighQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_EDGE_MODE_OFF) == static_cast<INT>(EdgeModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_EDGE_MODE_FAST) == static_cast<INT>(EdgeModeFast));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_EDGE_MODE_HIGH_QUALITY) == static_cast<INT>(EdgeModeHighQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_EDGE_MODE_ZERO_SHUTTER_LAG) == static_cast<INT>(EdgeModeZeroShutterLag));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_MODE_OFF) == static_cast<INT>(FlashModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_MODE_SINGLE) == static_cast<INT>(FlashModeSingle));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_MODE_TORCH) == static_cast<INT>(FlashModeTorch));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_STATE_UNAVAILABLE) == static_cast<INT>(FlashStateUnavailable));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_STATE_CHARGING) == static_cast<INT>(FlashStateCharging));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_STATE_READY) == static_cast<INT>(FlashStateReady));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_STATE_FIRED) == static_cast<INT>(FlashStateFired));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_STATE_PARTIAL) == static_cast<INT>(FlashStatePartial));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_INFO_AVAILABLE_FALSE) == static_cast<INT>(FlashInfoAvailableFalse));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_FLASH_INFO_AVAILABLE_TRUE) == static_cast<INT>(FlashInfoAvailableTrue));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_HOT_PIXEL_MODE_OFF) == static_cast<INT>(HotPixelModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_HOT_PIXEL_MODE_FAST) == static_cast<INT>(HotPixelModeFast));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_HOT_PIXEL_MODE_HIGH_QUALITY) == static_cast<INT>(HotPixelModeHighQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_OPTICAL_STABILIZATION_MODE_OFF) == static_cast<INT>(LensOpticalStabilizationModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_OPTICAL_STABILIZATION_MODE_ON) == static_cast<INT>(LensOpticalStabilizationModeOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_FACING_FRONT) == static_cast<INT>(LensFacingFront));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_FACING_BACK) == static_cast<INT>(LensFacingBack));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_FACING_EXTERNAL) == static_cast<INT>(LensFacingExternal));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_STATE_STATIONARY) == static_cast<INT>(LensStateStationary));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_STATE_MOVING) == static_cast<INT>(LensStateMoving));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_UNCALIBRATED) == static_cast<INT>(LensInfoFocusDistanceCalibrationUncalibrated));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_APPROXIMATE) == static_cast<INT>(LensInfoFocusDistanceCalibrationApproximate));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LENS_INFO_FOCUS_DISTANCE_CALIBRATION_CALIBRATED) == static_cast<INT>(LensInfoFocusDistanceCalibrationCalibrated));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_NOISE_REDUCTION_MODE_OFF) == static_cast<INT>(NoiseReductionModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_NOISE_REDUCTION_MODE_FAST) == static_cast<INT>(NoiseReductionModeFast));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_NOISE_REDUCTION_MODE_HIGH_QUALITY) == static_cast<INT>(NoiseReductionModeHighQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_NOISE_REDUCTION_MODE_MINIMAL) == static_cast<INT>(NoiseReductionModeMinimal));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_NOISE_REDUCTION_MODE_ZERO_SHUTTER_LAG) == static_cast<INT>(NoiseReductionModeZeroShutterLag));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_QUIRKS_PARTIAL_RESULT_FINAL) == static_cast<INT>(QuirksPartialResultFinal));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_QUIRKS_PARTIAL_RESULT_PARTIAL) == static_cast<INT>(QuirksPartialResultPartial));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_METADATA_MODE_NONE) == static_cast<INT>(RequestMetadataModeNone));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_METADATA_MODE_FULL) == static_cast<INT>(RequestMetadataModeFull));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_TYPE_CAPTURE) == static_cast<INT>(RequestTypeCapture));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_TYPE_REPROCESS) == static_cast<INT>(RequestTypeReprocess));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE) == static_cast<INT>(RequestAvailableCapabilitiesBackwardCompatible));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_SENSOR) == static_cast<INT>(RequestAvailableCapabilitiesManualSensor));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_MANUAL_POST_PROCESSING) == static_cast<INT>(RequestAvailableCapabilitiesManualPostProcessing));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_RAW) == static_cast<INT>(RequestAvailableCapabilitiesRaw));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING) == static_cast<INT>(RequestAvailableCapabilitiesPrivateReprocessing));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_READ_SENSOR_SETTINGS) == static_cast<INT>(RequestAvailableCapabilitiesReadSensorSettings));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE) == static_cast<INT>(RequestAvailableCapabilitiesBurstCapture));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_YUV_REPROCESSING) == static_cast<INT>(RequestAvailableCapabilitiesYuvReprocessing));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_DEPTH_OUTPUT) == static_cast<INT>(RequestAvailableCapabilitiesDepthOutput));
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_LOGICAL_MULTI_CAMERA) == static_cast<INT>(RequestAvailableCapabilitiesLogicalMultiCamera));
#endif // Android-P or better
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_REQUEST_AVAILABLE_CAPABILITIES_CONSTRAINED_HIGH_SPEED_VIDEO) == static_cast<INT>(RequestAvailableCapabilitiesConstrainedHighSpeedVideo));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_FORMATS_RAW16) == static_cast<INT>(ScalerAvailableFormatsRaw16));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_FORMATS_RAW_OPAQUE) == static_cast<INT>(ScalerAvailableFormatsRawOpaque));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_FORMATS_YV12) == static_cast<INT>(ScalerAvailableFormatsYV12));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_FORMATS_YCrCb_420_SP) == static_cast<INT>(ScalerAvailableFormatsYCrCb420Sp));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_FORMATS_IMPLEMENTATION_DEFINED) == static_cast<INT>(ScalerAvailableFormatsImplementationDefined));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_FORMATS_YCbCr_420_888) == static_cast<INT>(ScalerAvailableFormatsYCbCr420888));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_FORMATS_BLOB) == static_cast<INT>(ScalerAvailableFormatsBlob));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT) == static_cast<INT>(ScalerAvailableStreamConfigurationsOutput));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_INPUT) == static_cast<INT>(ScalerAvailableStreamConfigurationsInput));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_CROPPING_TYPE_CENTER_ONLY) == static_cast<INT>(ScalerCroppingTypeCenterOnly));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SCALER_CROPPING_TYPE_FREEFORM) == static_cast<INT>(ScalerCroppingTypeFreeform));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT) == static_cast<INT>(SensorReferenceIlluminant1Daylight));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_FLUORESCENT) == static_cast<INT>(SensorReferenceIlluminant1Fluorescent));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_TUNGSTEN) == static_cast<INT>(SensorReferenceIlluminant1Tungsten));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_FLASH) == static_cast<INT>(SensorReferenceIlluminant1Flash));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_FINE_WEATHER) == static_cast<INT>(SensorReferenceIlluminant1FineWeather));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_CLOUDY_WEATHER) == static_cast<INT>(SensorReferenceIlluminant1CloudyWeather));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_SHADE) == static_cast<INT>(SensorReferenceIlluminant1Shade));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAYLIGHT_FLUORESCENT) == static_cast<INT>(SensorReferenceIlluminant1DaylightFluorescent));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_DAY_WHITE_FLUORESCENT) == static_cast<INT>(SensorReferenceIlluminant1DayWhiteFluorescent));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_COOL_WHITE_FLUORESCENT) == static_cast<INT>(SensorReferenceIlluminant1CoolWhiteFluorescent));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_WHITE_FLUORESCENT) == static_cast<INT>(SensorReferenceIlluminant1WhiteFluorescent));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_A) == static_cast<INT>(SensorReferenceIlluminant1StandardA));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_B) == static_cast<INT>(SensorReferenceIlluminant1StandardB));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_STANDARD_C) == static_cast<INT>(SensorReferenceIlluminant1StandardC));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D55) == static_cast<INT>(SensorReferenceIlluminant1D55));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D65) == static_cast<INT>(SensorReferenceIlluminant1D65));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D75) == static_cast<INT>(SensorReferenceIlluminant1D75));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_D50) == static_cast<INT>(SensorReferenceIlluminant1D50));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_REFERENCE_ILLUMINANT1_ISO_STUDIO_TUNGSTEN) == static_cast<INT>(SensorReferenceIlluminant1IsoStudioTungsten));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_TEST_PATTERN_MODE_OFF) == static_cast<INT>(SensorTestPatternModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_TEST_PATTERN_MODE_SOLID_COLOR) == static_cast<INT>(SensorTestPatternModeSolidColor));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_TEST_PATTERN_MODE_COLOR_BARS) == static_cast<INT>(SensorTestPatternModeColorBars));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_TEST_PATTERN_MODE_COLOR_BARS_FADE_TO_GRAY) == static_cast<INT>(SensorTestPatternModeColorBarsFadeToGray));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_TEST_PATTERN_MODE_PN9) == static_cast<INT>(SensorTestPatternModePn9));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_TEST_PATTERN_MODE_CUSTOM1) == static_cast<INT>(SensorTestPatternModeCustom1));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB) == static_cast<INT>(SensorInfoColorFilterArrangementRggb));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG) == static_cast<INT>(SensorInfoColorFilterArrangementGrbg));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG) == static_cast<INT>(SensorInfoColorFilterArrangementGbrg));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR) == static_cast<INT>(SensorInfoColorFilterArrangementBggr));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGB) == static_cast<INT>(SensorInfoColorFilterArrangementRgb));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_UNKNOWN) == static_cast<INT>(SensorInfoTimestampSourceUnknown));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_TIMESTAMP_SOURCE_REALTIME) == static_cast<INT>(SensorInfoTimestampSourceRealtime));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_LENS_SHADING_APPLIED_FALSE) == static_cast<INT>(SensorInfoLensShadingAppliedFalse));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SENSOR_INFO_LENS_SHADING_APPLIED_TRUE) == static_cast<INT>(SensorInfoLensShadingAppliedTrue));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SHADING_MODE_OFF) == static_cast<INT>(ShadingModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SHADING_MODE_FAST) == static_cast<INT>(ShadingModeFast));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SHADING_MODE_HIGH_QUALITY) == static_cast<INT>(ShadingModeHighQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_FACE_DETECT_MODE_OFF) == static_cast<INT>(StatisticsFaceDetectModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_FACE_DETECT_MODE_SIMPLE) == static_cast<INT>(StatisticsFaceDetectModeSimple));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_FACE_DETECT_MODE_FULL) == static_cast<INT>(StatisticsFaceDetectModeFull));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_HISTOGRAM_MODE_OFF) == static_cast<INT>(StatisticsHistogramModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_HISTOGRAM_MODE_ON) == static_cast<INT>(StatisticsHistogramModeOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_SHARPNESS_MAP_MODE_OFF) == static_cast<INT>(StatisticsSharpnessMapModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_SHARPNESS_MAP_MODE_ON) == static_cast<INT>(StatisticsSharpnessMapModeOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_OFF) == static_cast<INT>(StatisticsHotPixelMapModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_HOT_PIXEL_MAP_MODE_ON) == static_cast<INT>(StatisticsHotPixelMapModeOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_SCENE_FLICKER_NONE) == static_cast<INT>(StatisticsSceneFlickerNone));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_SCENE_FLICKER_50HZ) == static_cast<INT>(StatisticsSceneFlicker50Hz));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_SCENE_FLICKER_60HZ) == static_cast<INT>(StatisticsSceneFlicker60Hz));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_OFF) == static_cast<INT>(StatisticsLensShadingMapModeOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_STATISTICS_LENS_SHADING_MAP_MODE_ON) == static_cast<INT>(StatisticsLensShadingMapModeOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_MODE_CONTRAST_CURVE) == static_cast<INT>(TonemapModeContrastCurve));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_MODE_FAST) == static_cast<INT>(TonemapModeFast));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_MODE_HIGH_QUALITY) == static_cast<INT>(TonemapModeHighQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_MODE_GAMMA_VALUE) == static_cast<INT>(TonemapModeGammaValue));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_MODE_PRESET_CURVE) == static_cast<INT>(TonemapModePresetCurve));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_PRESET_CURVE_SRGB) == static_cast<INT>(TonemapPresetCurveSrgb));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_TONEMAP_PRESET_CURVE_REC709) == static_cast<INT>(TonemapPresetCurveRec709));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LED_TRANSMIT_OFF) == static_cast<INT>(LedTransmitOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LED_TRANSMIT_ON) == static_cast<INT>(LedTransmitOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_LED_AVAILABLE_LEDS_TRANSMIT) == static_cast<INT>(LedAvailableLedsTransmit));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LIMITED) == static_cast<INT>(InfoSupportedHardwareLevelLimited));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_FULL) == static_cast<INT>(InfoSupportedHardwareLevelFull));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_LEGACY) == static_cast<INT>(InfoSupportedHardwareLevelLegacy));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_INFO_SUPPORTED_HARDWARE_LEVEL_3) == static_cast<INT>(InfoSupportedHardwareLevel3));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_BLACK_LEVEL_LOCK_OFF) == static_cast<INT>(BlackLevelLockOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_BLACK_LEVEL_LOCK_ON) == static_cast<INT>(BlackLevelLockOn));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SYNC_FRAME_NUMBER_CONVERGING) == static_cast<INT>(SyncFrameNumberConverging));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SYNC_FRAME_NUMBER_UNKNOWN) == static_cast<INT>(SyncFrameNumberUnknown));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SYNC_MAX_LATENCY_PER_FRAME_CONTROL) == static_cast<INT>(SyncMaxLatencyPerFrameControl));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_SYNC_MAX_LATENCY_UNKNOWN) == static_cast<INT>(SyncMaxLatencyUnknown));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS_OUTPUT) == static_cast<INT>(DepthAvailableDepthStreamConfigurationsOutput));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEPTH_AVAILABLE_DEPTH_STREAM_CONFIGURATIONS_INPUT) == static_cast<INT>(DepthAvailableDepthStreamConfigurationsInput));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEPTH_DEPTH_IS_EXCLUSIVE_FALSE) == static_cast<INT>(DepthDepthIsExclusiveFalse));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DEPTH_DEPTH_IS_EXCLUSIVE_TRUE) == static_cast<INT>(DepthDepthIsExclusiveTrue));
#if (CAMERA_MODULE_API_VERSION_CURRENT > CAMERA_MODULE_API_VERSION_2_4) // Check Camera Module Version
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DISTORTION_CORRECTION_MODE) == static_cast<INT>(DistortionCorrectionMode));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DISTORTION_CORRECTION_AVAILABLE_MODES) == static_cast<INT>(DistortionCorrectionAvailableModes));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DISTORTION_CORRECTION_MODE_OFF) == static_cast<INT>(DistortionCorrectionModesOff));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DISTORTION_CORRECTION_MODE_FAST) == static_cast<INT>(DistortionCorrectionModesFast));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_DISTORTION_CORRECTION_MODE_HIGH_QUALITY) == static_cast<INT>(DistortionCorrectionModesHighQuality));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_HEIC_AVAILABLE_HEIC_STREAM_CONFIGURATIONS) == static_cast<INT>(HEICAvailableHEICStreamConfigurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_HEIC_AVAILABLE_HEIC_MIN_FRAME_DURATIONS) == static_cast<INT>(HEICAvailableHEICMinFrameDurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_HEIC_AVAILABLE_HEIC_STALL_DURATIONS) == static_cast<INT>(HEICAvailableHEICStallDurations));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_HEIC_INFO_SUPPORTED) == static_cast<INT>(HEICInfoSupported));
CAMX_STATIC_ASSERT(static_cast<INT>(ANDROID_HEIC_INFO_MAX_JPEG_APP_SEGMENTS_COUNT) == static_cast<INT>(HEICInfoMaxJpegAppSegmentsCount));
#endif // Check Camera Module Version

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionColorCorrectionStart)
static const CHAR* ColorCorrectionPropertyStrings[] =
{
    CAMX_STRINGIZE(ColorCorrectionMode),
    CAMX_STRINGIZE(ColorCorrectionTransform),
    CAMX_STRINGIZE(ColorCorrectionGains),
    CAMX_STRINGIZE(ColorCorrectionAberrationMode),
    CAMX_STRINGIZE(ColorCorrectionAvailableAberrationModes),
    CAMX_STRINGIZE(ColorCorrectionEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(ColorCorrectionPropertyStrings) == (1 + ColorCorrectionEnd - MetadataSectionColorCorrectionStart),
    "ColorCorrectionPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionControlStart)
static const CHAR* ControlPropertyStrings[] =
{
    CAMX_STRINGIZE(ControlAEAntibandingMode),
    CAMX_STRINGIZE(ControlAEExposureCompensation),
    CAMX_STRINGIZE(ControlAELock),
    CAMX_STRINGIZE(ControlAEMode),
    CAMX_STRINGIZE(ControlAERegions),
    CAMX_STRINGIZE(ControlAETargetFpsRange),
    CAMX_STRINGIZE(ControlAEPrecaptureTrigger),
    CAMX_STRINGIZE(ControlAFMode),
    CAMX_STRINGIZE(ControlAFRegions),
    CAMX_STRINGIZE(ControlAFTrigger),
    CAMX_STRINGIZE(ControlAWBLock),
    CAMX_STRINGIZE(ControlAWBMode),
    CAMX_STRINGIZE(ControlAWBRegions),
    CAMX_STRINGIZE(ControlCaptureIntent),
    CAMX_STRINGIZE(ControlEffectMode),
    CAMX_STRINGIZE(ControlMode),
    CAMX_STRINGIZE(ControlSceneMode),
    CAMX_STRINGIZE(ControlVideoStabilizationMode),
    CAMX_STRINGIZE(ControlAEAvailableAntibandingModes),
    CAMX_STRINGIZE(ControlAEAvailableModes),
    CAMX_STRINGIZE(ControlAEAvailableTargetFPSRanges),
    CAMX_STRINGIZE(ControlAECompensationRange),
    CAMX_STRINGIZE(ControlAECompensationStep),
    CAMX_STRINGIZE(ControlAFAvailableModes),
    CAMX_STRINGIZE(ControlAvailableEffects),
    CAMX_STRINGIZE(ControlAvailableSceneModes),
    CAMX_STRINGIZE(ControlAvailableVideoStabilizationModes),
    CAMX_STRINGIZE(ControlAWBAvailableModes),
    CAMX_STRINGIZE(ControlMaxRegions),
    CAMX_STRINGIZE(ControlSceneModeOverrides),
    CAMX_STRINGIZE(ControlAEPrecaptureId),
    CAMX_STRINGIZE(ControlAEState),
    CAMX_STRINGIZE(ControlAFState),
    CAMX_STRINGIZE(ControlAFTriggerId),
    CAMX_STRINGIZE(ControlAWBState),
    CAMX_STRINGIZE(ControlAvailableHighSpeedVideoConfigurations),
    CAMX_STRINGIZE(ControlAELockAvailable),
    CAMX_STRINGIZE(ControlAWBLockAvailable),
    CAMX_STRINGIZE(ControlAvailableModes),
    CAMX_STRINGIZE(ControlPostRawSensitivityBoostRange),
    CAMX_STRINGIZE(ControlPostRawSensitivityBoost),
    CAMX_STRINGIZE(ControlZslEnable),
    CAMX_STRINGIZE(ControlEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(ControlPropertyStrings) == (1 + ControlEnd - MetadataSectionControlStart),
    "ControlPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionDemosaicStart)
static const CHAR* DemosaicPropertyStrings[] =
{
    CAMX_STRINGIZE(DemosaicMode),
    CAMX_STRINGIZE(DemosaicEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(DemosaicPropertyStrings) == (1 + DemosaicEnd - MetadataSectionDemosaicStart),
    "DemosaicPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionEdgeStart)
static const CHAR* EdgePropertyStrings[] =
{
    CAMX_STRINGIZE(EdgeMode),
    CAMX_STRINGIZE(EdgeStrength),
    CAMX_STRINGIZE(EdgeAvailableEdgeModes),
    CAMX_STRINGIZE(EdgeEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(EdgePropertyStrings) == (1 + EdgeEnd - MetadataSectionEdgeStart),
    "EdgePropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionFlashStart)
static const CHAR* FlashPropertyStrings[] =
{
    CAMX_STRINGIZE(FlashFiringPower),
    CAMX_STRINGIZE(FlashFiringTime),
    CAMX_STRINGIZE(FlashMode),
    CAMX_STRINGIZE(FlashColorTemperature),
    CAMX_STRINGIZE(FlashMaxEnergy),
    CAMX_STRINGIZE(FlashState),
    CAMX_STRINGIZE(FlashEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(FlashPropertyStrings) == (1 + FlashEnd - MetadataSectionFlashStart),
    "FlashPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionFlashInfoStart)
static const CHAR* FlashInfoPropertyStrings[] =
{
    CAMX_STRINGIZE(FlashInfoAvailable),
    CAMX_STRINGIZE(FlashInfoChargeDuration),
    CAMX_STRINGIZE(FlashInfoEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(FlashInfoPropertyStrings) == (1 + FlashInfoEnd - MetadataSectionFlashInfoStart),
    "FlashInfoPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionHotPixelStart)
static const CHAR* HotPixelPropertyStrings[] =
{
    CAMX_STRINGIZE(HotPixelMode),
    CAMX_STRINGIZE(HotPixelAvailableHotPixelModes),
    CAMX_STRINGIZE(HotPixelEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(HotPixelPropertyStrings) == (1 + HotPixelEnd - MetadataSectionHotPixelStart),
    "HotPixelPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionJPEGStart)
static const CHAR* JPEGPropertyStrings[] =
{
    CAMX_STRINGIZE(JPEGGpsCoordinates),
    CAMX_STRINGIZE(JPEGGpsProcessingMethod),
    CAMX_STRINGIZE(JPEGGpsTimestamp),
    CAMX_STRINGIZE(JPEGOrientation),
    CAMX_STRINGIZE(JPEGQuality),
    CAMX_STRINGIZE(JPEGThumbnailQuality),
    CAMX_STRINGIZE(JPEGThumbnailSize),
    CAMX_STRINGIZE(JPEGAvailableThumbnailSizes),
    CAMX_STRINGIZE(JPEGMaxSize),
    CAMX_STRINGIZE(JPEGSize),
    CAMX_STRINGIZE(JPEGEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(JPEGPropertyStrings) == (1 + JPEGEnd - MetadataSectionJPEGStart),
    "JPEGPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionLensStart)
static const CHAR* LensPropertyStrings[] =
{
    CAMX_STRINGIZE(LensAperture),
    CAMX_STRINGIZE(LensFilterDensity),
    CAMX_STRINGIZE(LensFocalLength),
    CAMX_STRINGIZE(LensFocusDistance),
    CAMX_STRINGIZE(LensOpticalStabilizationMode),
    CAMX_STRINGIZE(LensFacing),
    CAMX_STRINGIZE(LensPoseRotation),
    CAMX_STRINGIZE(LensPoseTranslation),
    CAMX_STRINGIZE(LensFocusRange),
    CAMX_STRINGIZE(LensState),
    CAMX_STRINGIZE(LensIntrinsicCalibration),
    CAMX_STRINGIZE(LensRadialDistortion),
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    CAMX_STRINGIZE(LensPoseReference),
    CAMX_STRINGIZE(LensDistortion),
#endif // Android-P or better
    CAMX_STRINGIZE(LensEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(LensPropertyStrings) == (1 + LensEnd - MetadataSectionLensStart),
    "LensPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionLensInfoStart)
static const CHAR* LensInfoPropertyStrings[] =
{
    CAMX_STRINGIZE(LensInfoAvailableApertures),
    CAMX_STRINGIZE(LensInfoAvailableFilterDensities),
    CAMX_STRINGIZE(LensInfoAvailableFocalLengths),
    CAMX_STRINGIZE(LensInfoAvailableOpticalStabilization),
    CAMX_STRINGIZE(LensInfoHyperfocalDistance),
    CAMX_STRINGIZE(LensInfoMinimumFocusDistance),
    CAMX_STRINGIZE(LensInfoShadingMapSize),
    CAMX_STRINGIZE(LensInfoFocusDistanceCalibration),
    CAMX_STRINGIZE(LensInfoEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(LensInfoPropertyStrings) == (1 + LensInfoEnd - MetadataSectionLensInfoStart),
    "LensInfoPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
static const CHAR* LogicalMultiCameraPropertyStrings[] =
{
    CAMX_STRINGIZE(LogicalMultiCameraPhysicalIDs),
    CAMX_STRINGIZE(LogicalMultiCameraSensorSyncType),
    CAMX_STRINGIZE(LogicalMultiCameraEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(LogicalMultiCameraPropertyStrings) == (1 + LogicalMultiCameraEnd - MetadataSectionLogicalMultiCameraStart),
    "LogicalMultiCameraPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionDistortionCorrectionStart)
static const CHAR* DistortionCorrectionPropertyStrings[] =
{
    CAMX_STRINGIZE(DistortionCorrectionMode),
    CAMX_STRINGIZE(DistortionCorrectionAvailableModes),
    CAMX_STRINGIZE(DistortionCorrectionEnd),
};

CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(DistortionCorrectionPropertyStrings) == (1 + DistortionCorrectionEnd - MetadataSectionDistortionCorrectionStart),
    "DistortionCorrection must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionHEICStart)
static const CHAR* HEICPropertyStrings[] =
{
    CAMX_STRINGIZE(HEICAvailableHEICStreamConfigurations),
    CAMX_STRINGIZE(HEICAvailableHEICMinFrameDurations),
    CAMX_STRINGIZE(HEICAvailableHEICStallDurations),
    CAMX_STRINGIZE(HEICEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(HEICPropertyStrings) == (1 + HEICEnd - MetadataSectionHEICStart),
    "HEIC must be the same size as the range of Property IDs, check for missing definitions");

#endif // Android-P or better

static const CHAR* HEICInfoPropertyStrings[] =
{
    CAMX_STRINGIZE(HEICInfoSupported),
    CAMX_STRINGIZE(HEICInfoMaxJpegAppSegmentsCount),
    CAMX_STRINGIZE(HEICInfoEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(HEICInfoPropertyStrings) == (1 + HEICInfoEnd - MetadataSectionHEICInfoStart),
    "HEICPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionNoiseReductionStart)
static const CHAR* NoiseReductionPropertyStrings[] =
{
    CAMX_STRINGIZE(NoiseReductionMode),
    CAMX_STRINGIZE(NoiseReductionStrength),
    CAMX_STRINGIZE(NoiseReductionAvailableNoiseReductionModes),
    CAMX_STRINGIZE(NoiseReductionEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(NoiseReductionPropertyStrings) == (1 + NoiseReductionEnd - MetadataSectionNoiseReductionStart),
    "NoiseReductionPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionQuirksStart)
static const CHAR* QuirksPropertyStrings[] =
{
    CAMX_STRINGIZE(QuirksMeteringCropRegion),
    CAMX_STRINGIZE(QuirksTriggerAFWithAuto),
    CAMX_STRINGIZE(QuirksUseZslFormat),
    CAMX_STRINGIZE(QuirksUsePartialResult),
    CAMX_STRINGIZE(QuirksPartialResult),
    CAMX_STRINGIZE(QuirksEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(QuirksPropertyStrings) == (1 + QuirksEnd - MetadataSectionQuirksStart),
    "QuirksPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionRequestStart)
static const CHAR* RequestPropertyStrings[] =
{
    CAMX_STRINGIZE(RequestFrameCount),
    CAMX_STRINGIZE(RequestId),
    CAMX_STRINGIZE(RequestInputStreams),
    CAMX_STRINGIZE(RequestMetadataMode),
    CAMX_STRINGIZE(RequestOutputStreams),
    CAMX_STRINGIZE(RequestType),
    CAMX_STRINGIZE(RequestMaxNumOutputStreams),
    CAMX_STRINGIZE(RequestMaxNumReprocessStreams),
    CAMX_STRINGIZE(RequestMaxNumInputStreams),
    CAMX_STRINGIZE(RequestPipelineDepth),
    CAMX_STRINGIZE(RequestPipelineMaxDepth),
    CAMX_STRINGIZE(RequestPartialResultCount),
    CAMX_STRINGIZE(RequestAvailableCapabilities),
    CAMX_STRINGIZE(RequestAvailableRequestKeys),
    CAMX_STRINGIZE(RequestAvailableResultKeys),
    CAMX_STRINGIZE(RequestAvailableCharacteristicsKeys),
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    CAMX_STRINGIZE(RequestAvailableSessionKeys),
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))
    CAMX_STRINGIZE(RequestEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(RequestPropertyStrings) == (1 + RequestEnd - MetadataSectionRequestStart),
    "RequestPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionScalerStart)
static const CHAR* ScalerPropertyStrings[] =
{
    CAMX_STRINGIZE(ScalerCropRegion),
    CAMX_STRINGIZE(ScalerAvailableFormats),
    CAMX_STRINGIZE(ScalerAvailableJPEGMinDurations),
    CAMX_STRINGIZE(ScalerAvailableJPEGSizes),
    CAMX_STRINGIZE(ScalerAvailableMaxDigitalZoom),
    CAMX_STRINGIZE(ScalerAvailableProcessedMinDurations),
    CAMX_STRINGIZE(ScalerAvailableProcessedSizes),
    CAMX_STRINGIZE(ScalerAvailableRawMinDurations),
    CAMX_STRINGIZE(ScalerAvailableRawSizes),
    CAMX_STRINGIZE(ScalerAvailableInputOutputFormatsMap),
    CAMX_STRINGIZE(ScalerAvailableStreamConfigurations),
    CAMX_STRINGIZE(ScalerAvailableMinFrameDurations),
    CAMX_STRINGIZE(ScalerAvailableStallDurations),
    CAMX_STRINGIZE(ScalerCroppingType),
    CAMX_STRINGIZE(ScalerEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(ScalerPropertyStrings) == (1 + ScalerEnd - MetadataSectionScalerStart),
    "ScalerPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionSensorStart)
static const CHAR* SensorPropertyStrings[] =
{
    CAMX_STRINGIZE(SensorExposureTime),
    CAMX_STRINGIZE(SensorFrameDuration),
    CAMX_STRINGIZE(SensorSensitivity),
    CAMX_STRINGIZE(SensorReferenceIlluminant1),
    CAMX_STRINGIZE(SensorReferenceIlluminant2),
    CAMX_STRINGIZE(SensorCalibrationTransform1),
    CAMX_STRINGIZE(SensorCalibrationTransform2),
    CAMX_STRINGIZE(SensorColorTransform1),
    CAMX_STRINGIZE(SensorColorTransform2),
    CAMX_STRINGIZE(SensorForwardMatrix1),
    CAMX_STRINGIZE(SensorForwardMatrix2),
    CAMX_STRINGIZE(SensorBaseGainFactor),
    CAMX_STRINGIZE(SensorBlackLevelPattern),
    CAMX_STRINGIZE(SensorMaxAnalogSensitivity),
    CAMX_STRINGIZE(SensorOrientation),
    CAMX_STRINGIZE(SensorProfileHueSaturationMapDimensions),
    CAMX_STRINGIZE(SensorTimestamp),
    CAMX_STRINGIZE(SensorTemperature),
    CAMX_STRINGIZE(SensorNeutralColorPoint),
    CAMX_STRINGIZE(SensorNoiseProfile),
    CAMX_STRINGIZE(SensorProfileHueSaturationMap),
    CAMX_STRINGIZE(SensorProfileToneCurve),
    CAMX_STRINGIZE(SensorGreenSplit),
    CAMX_STRINGIZE(SensorTestPatternData),
    CAMX_STRINGIZE(SensorTestPatternMode),
    CAMX_STRINGIZE(SensorAvailableTestPatternModes),
    CAMX_STRINGIZE(SensorRollingShutterSkew),
    CAMX_STRINGIZE(SensorOpticalBlackRegions),
    CAMX_STRINGIZE(SensorDynamicBlackLevel),
    CAMX_STRINGIZE(SensorDynamicWhiteLevel),
    CAMX_STRINGIZE(SensorOpaqueRawSize),
    CAMX_STRINGIZE(SensorEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(SensorPropertyStrings) == (1 + SensorEnd - MetadataSectionSensorStart),
    "SensorPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionSensorInfoStart)
static const CHAR* SensorInfoPropertyStrings[] =
{
    CAMX_STRINGIZE(SensorInfoActiveArraySize),
    CAMX_STRINGIZE(SensorInfoSensitivityRange),
    CAMX_STRINGIZE(SensorInfoColorFilterArrangement),
    CAMX_STRINGIZE(SensorInfoExposureTimeRange),
    CAMX_STRINGIZE(SensorInfoMaxFrameDuration),
    CAMX_STRINGIZE(SensorInfoPhysicalSize),
    CAMX_STRINGIZE(SensorInfoPixelArraySize),
    CAMX_STRINGIZE(SensorInfoWhiteLevel),
    CAMX_STRINGIZE(SensorInfoTimestampSource),
    CAMX_STRINGIZE(SensorInfoLensShadingApplied),
    CAMX_STRINGIZE(SensorInfoPreCorrectionActiveArraySize),
    CAMX_STRINGIZE(SensorInfoEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(SensorInfoPropertyStrings) == (1 + SensorInfoEnd - MetadataSectionSensorInfoStart),
    "SensorInfoPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionShadingStart)
static const CHAR* ShadingPropertyStrings[] =
{
    CAMX_STRINGIZE(ShadingMode),
    CAMX_STRINGIZE(ShadingStrength),
    CAMX_STRINGIZE(ShadingAvailableModes),
    CAMX_STRINGIZE(ShadingEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(ShadingPropertyStrings) == (1 + ShadingEnd - MetadataSectionShadingStart),
    "ShadingPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionStatisticsStart)
static const CHAR* StatisticsPropertyStrings[] =
{
    CAMX_STRINGIZE(StatisticsFaceDetectMode),
    CAMX_STRINGIZE(StatisticsHistogramMode),
    CAMX_STRINGIZE(StatisticsSharpnessMapMode),
    CAMX_STRINGIZE(StatisticsHotPixelMapMode),
    CAMX_STRINGIZE(StatisticsFaceIds),
    CAMX_STRINGIZE(StatisticsFaceLandmarks),
    CAMX_STRINGIZE(StatisticsFaceRectangles),
    CAMX_STRINGIZE(StatisticsFaceScores),
    CAMX_STRINGIZE(StatisticsHistogram),
    CAMX_STRINGIZE(StatisticsSharpnessMap),
    CAMX_STRINGIZE(StatisticsLensShadingCorrectionMap),
    CAMX_STRINGIZE(StatisticsLensShadingMap),
    CAMX_STRINGIZE(StatisticsPredictedColorGains),
    CAMX_STRINGIZE(StatisticsPredictedColorTransform),
    CAMX_STRINGIZE(StatisticsSceneFlicker),
    CAMX_STRINGIZE(StatisticsHotPixelMap),
    CAMX_STRINGIZE(StatisticsLensShadingMapMode),
    CAMX_STRINGIZE(StatisticsEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(StatisticsPropertyStrings) == (1 + StatisticsEnd - MetadataSectionStatisticsStart),
    "StatisticsPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionStatisticsInfoStart)
static const CHAR* StatisticsInfoPropertyStrings[] =
{
    CAMX_STRINGIZE(StatisticsInfoAvailableFaceDetectModes),
    CAMX_STRINGIZE(StatisticsInfoHistogramBucketCount),
    CAMX_STRINGIZE(StatisticsInfoMaxFaceCount),
    CAMX_STRINGIZE(StatisticsInfoMaxHistogramCount),
    CAMX_STRINGIZE(StatisticsInfoMaxSharpnessMapValue),
    CAMX_STRINGIZE(StatisticsInfoSharpnessMapSize),
    CAMX_STRINGIZE(StatisticsInfoAvailableHotPixelMapModes),
    CAMX_STRINGIZE(StatisticsInfoAvailableLensShadingMapModes),
    CAMX_STRINGIZE(StatisticsInfoEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(StatisticsInfoPropertyStrings) == (1 + StatisticsInfoEnd - MetadataSectionStatisticsInfoStart),
    "StatisticsInfoPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionTonemapStart)
static const CHAR* TonemapPropertyStrings[] =
{
    CAMX_STRINGIZE(TonemapCurveBlue),
    CAMX_STRINGIZE(TonemapCurveGreen),
    CAMX_STRINGIZE(TonemapCurveRed),
    CAMX_STRINGIZE(TonemapMode),
    CAMX_STRINGIZE(TonemapMaxCurvePoints),
    CAMX_STRINGIZE(TonemapAvailableToneMapModes),
    CAMX_STRINGIZE(TonemapGamma),
    CAMX_STRINGIZE(TonemapPresetCurve),
    CAMX_STRINGIZE(TonemapEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(TonemapPropertyStrings) == (1 + TonemapEnd - MetadataSectionTonemapStart),
    "TonemapPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionLedStart)
static const CHAR* LedPropertyStrings[] =
{
    CAMX_STRINGIZE(LedTransmit),
    CAMX_STRINGIZE(LedAvailableLeds),
    CAMX_STRINGIZE(LedEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(LedPropertyStrings) == (1 + LedEnd - MetadataSectionLedStart),
    "LedPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionInfoStart)
static const CHAR* InfoPropertyStrings[] =
{
    CAMX_STRINGIZE(InfoSupportedHardwareLevel),
    CAMX_STRINGIZE(InfoEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(InfoPropertyStrings) == (1 + InfoEnd - MetadataSectionInfoStart),
    "InfoPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionBlackLevelStart)
static const CHAR* BlackLevelPropertyStrings[] =
{
    CAMX_STRINGIZE(BlackLevelLock),
    CAMX_STRINGIZE(BlackLevelEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(BlackLevelPropertyStrings) == (1 + BlackLevelEnd - MetadataSectionBlackLevelStart),
    "BlackLevelPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionSyncStart)
static const CHAR* SyncPropertyStrings[] =
{
    CAMX_STRINGIZE(SyncFrameNumber),
    CAMX_STRINGIZE(SyncMaxLatency),
    CAMX_STRINGIZE(SyncEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(SyncPropertyStrings) == (1 + SyncEnd - MetadataSectionSyncStart),
    "SyncPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionReprocessStart)
static const CHAR* ReprocessPropertyStrings[] =
{
    CAMX_STRINGIZE(ReprocessEffectiveExposureFactor),
    CAMX_STRINGIZE(ReprocessMaxCaptureStall),
    CAMX_STRINGIZE(ReprocessEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(ReprocessPropertyStrings) == (1 + ReprocessEnd - MetadataSectionReprocessStart),
    "ReprocessPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

/// @brief LUT that allows using the propertyID to determine a string name for the propertyID
///        Indexed with (id & ~MetadataSectionDepthStart)
static const CHAR* DepthPropertyStrings[] =
{
    CAMX_STRINGIZE(DepthMaxDepthSamples),
    CAMX_STRINGIZE(DepthAvailableDepthStreamConfigurations),
    CAMX_STRINGIZE(DepthAvailableDepthMinFrameDurations),
    CAMX_STRINGIZE(DepthAvailableDepthStallDurations),
    CAMX_STRINGIZE(DepthDepthIsExclusive),
    CAMX_STRINGIZE(DepthEnd),
};
CAMX_STATIC_ASSERT_MESSAGE(CAMX_ARRAY_SIZE(DepthPropertyStrings) == (1 + DepthEnd - MetadataSectionDepthStart),
    "DepthPropertyStrings must be the same size as the range of Property IDs, check for missing definitions");

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CameraMetadataTagToString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CameraMetadataTagToString(
    CameraMetadataTag   tag,
    const CHAR**        ppTagPrefix,
    const CHAR**        ppTagName)
{
    CamxResult  result  = CamxResultSuccess;
    UINT32      id      = static_cast<UINT32>(tag);

    if ((ppTagPrefix == NULL) || (ppTagName == NULL))
    {
        result = CamxResultEInvalidArg;
    }
    else
    {
        const CHAR* pTagPrefix  = NULL;
        const CHAR* pTagName    = NULL;

        // Section tags just have sections or'ed into standard tags, so remove any section mask and record the prefix
        //  InputMetadataSectionMask, UsecaseMetadataSectionMask, and StaticMetadataSectionMask
        if ((id & InputMetadataSectionMask) == InputMetadataSectionMask)
        {
            pTagPrefix = "Input";
            id &= ~InputMetadataSectionMask;
        }
        else if ((id & UsecaseMetadataSectionMask) == UsecaseMetadataSectionMask)
        {
            pTagPrefix = "Usecase";
            id &= ~UsecaseMetadataSectionMask;
        }
        else if ((id & StaticMetadataSectionMask) == StaticMetadataSectionMask)
        {
            pTagPrefix = "Static";
            id &= ~StaticMetadataSectionMask;
        }
        else
        {
            pTagPrefix = "Main";
        }

        switch (id >> 16)
        {
            // Table lookup for strings
            case (MetadataSectionColorCorrectionStart >> 16):
                if ((id & ~MetadataSectionColorCorrectionStart) < CAMX_ARRAY_SIZE(ColorCorrectionPropertyStrings))
                {
                    pTagName = ColorCorrectionPropertyStrings[id & ~MetadataSectionColorCorrectionStart];
                }
                break;
            case (MetadataSectionControlStart >> 16):
                if ((id & ~MetadataSectionControlStart) < CAMX_ARRAY_SIZE(ControlPropertyStrings))
                {
                    pTagName = ControlPropertyStrings[id & ~MetadataSectionControlStart];
                }
                break;
            case (MetadataSectionDemosaicStart >> 16):
                if ((id & ~MetadataSectionDemosaicStart) < CAMX_ARRAY_SIZE(DemosaicPropertyStrings))
                {
                    pTagName = DemosaicPropertyStrings[id & ~MetadataSectionDemosaicStart];
                }
                break;
            case (MetadataSectionEdgeStart >> 16):
                if ((id & ~MetadataSectionEdgeStart) < CAMX_ARRAY_SIZE(EdgePropertyStrings))
                {
                    pTagName = EdgePropertyStrings[id & ~MetadataSectionEdgeStart];
                }
                break;
            case (MetadataSectionFlashStart >> 16):
                if ((id & ~MetadataSectionFlashStart) < CAMX_ARRAY_SIZE(FlashPropertyStrings))
                {
                    pTagName = FlashPropertyStrings[id & ~MetadataSectionFlashStart];
                }
                break;
            case (MetadataSectionFlashInfoStart >> 16):
                if ((id & ~MetadataSectionFlashInfoStart) < CAMX_ARRAY_SIZE(FlashInfoPropertyStrings))
                {
                    pTagName = FlashInfoPropertyStrings[id & ~MetadataSectionFlashInfoStart];
                }
                break;
            case (MetadataSectionHotPixelStart >> 16):
                if ((id & ~MetadataSectionHotPixelStart) < CAMX_ARRAY_SIZE(HotPixelPropertyStrings))
                {
                    pTagName = HotPixelPropertyStrings[id & ~MetadataSectionHotPixelStart];
                }
                break;
            case (MetadataSectionJPEGStart >> 16):
                if ((id & ~MetadataSectionJPEGStart) < CAMX_ARRAY_SIZE(JPEGPropertyStrings))
                {
                    pTagName = JPEGPropertyStrings[id & ~MetadataSectionJPEGStart];
                }
                break;
            case (MetadataSectionLensStart >> 16):
                if ((id & ~MetadataSectionLensStart) < CAMX_ARRAY_SIZE(LensPropertyStrings))
                {
                    pTagName = LensPropertyStrings[id & ~MetadataSectionLensStart];
                }
                break;
            case (MetadataSectionLensInfoStart >> 16):
                if ((id & ~MetadataSectionLensInfoStart) < CAMX_ARRAY_SIZE(LensInfoPropertyStrings))
                {
                    pTagName = LensInfoPropertyStrings[id & ~MetadataSectionLensInfoStart];
                }
                break;
#if (defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
            case (MetadataSectionLogicalMultiCameraStart >> 16):
                if ((id & ~MetadataSectionLogicalMultiCameraStart) < CAMX_ARRAY_SIZE(LogicalMultiCameraPropertyStrings))
                {
                    pTagName = LogicalMultiCameraPropertyStrings[id & ~MetadataSectionLogicalMultiCameraStart];
                }
                break;
            case (MetadataSectionDistortionCorrectionStart >> 16):
                if ((id & ~MetadataSectionDistortionCorrectionStart) < CAMX_ARRAY_SIZE(DistortionCorrectionPropertyStrings))
                {
                    pTagName = DistortionCorrectionPropertyStrings[id & ~MetadataSectionDistortionCorrectionStart];
                }
                break;
            case (MetadataSectionHEICStart >> 16):
                if ((id & ~MetadataSectionHEICStart) < CAMX_ARRAY_SIZE(HEICPropertyStrings))
                {
                    pTagName = HEICPropertyStrings[id & ~MetadataSectionHEICStart];
                }
                break;
            case (MetadataSectionHEICInfoStart >> 16):
                if ((id & ~MetadataSectionHEICInfoStart) < CAMX_ARRAY_SIZE(HEICInfoPropertyStrings))
                {
                    pTagName = HEICInfoPropertyStrings[id & ~MetadataSectionHEICInfoStart];
                }
                break;
#endif // Android-P or better
            case (MetadataSectionNoiseReductionStart >> 16):
                if ((id & ~MetadataSectionNoiseReductionStart) < CAMX_ARRAY_SIZE(NoiseReductionPropertyStrings))
                {
                    pTagName = NoiseReductionPropertyStrings[id & ~MetadataSectionNoiseReductionStart];
                }
                break;
            case (MetadataSectionQuirksStart >> 16):
                if ((id & ~MetadataSectionQuirksStart) < CAMX_ARRAY_SIZE(QuirksPropertyStrings))
                {
                    pTagName = QuirksPropertyStrings[id & ~MetadataSectionQuirksStart];
                }
                break;
            case (MetadataSectionRequestStart >> 16):
                if ((id & ~MetadataSectionRequestStart) < CAMX_ARRAY_SIZE(RequestPropertyStrings))
                {
                    pTagName = RequestPropertyStrings[id & ~MetadataSectionRequestStart];
                }
                break;
            case (MetadataSectionScalerStart >> 16):
                if ((id & ~MetadataSectionScalerStart) < CAMX_ARRAY_SIZE(ScalerPropertyStrings))
                {
                    pTagName = ScalerPropertyStrings[id & ~MetadataSectionScalerStart];
                }
                break;
            case (MetadataSectionSensorStart >> 16):
                if ((id & ~MetadataSectionSensorStart) < CAMX_ARRAY_SIZE(SensorPropertyStrings))
                {
                    pTagName = SensorPropertyStrings[id & ~MetadataSectionSensorStart];
                }
                break;
            case (MetadataSectionSensorInfoStart >> 16):
                if ((id & ~MetadataSectionSensorInfoStart) < CAMX_ARRAY_SIZE(SensorInfoPropertyStrings))
                {
                    pTagName = SensorInfoPropertyStrings[id & ~MetadataSectionSensorInfoStart];
                }
                break;
            case (MetadataSectionShadingStart >> 16):
                if ((id & ~MetadataSectionShadingStart) < CAMX_ARRAY_SIZE(ShadingPropertyStrings))
                {
                    pTagName = ShadingPropertyStrings[id & ~MetadataSectionShadingStart];
                }
                break;
            case (MetadataSectionStatisticsStart >> 16):
                if ((id & ~MetadataSectionStatisticsStart) < CAMX_ARRAY_SIZE(StatisticsPropertyStrings))
                {
                    pTagName = StatisticsPropertyStrings[id & ~MetadataSectionStatisticsStart];
                }
                break;
            case (MetadataSectionStatisticsInfoStart >> 16):
                if ((id & ~MetadataSectionStatisticsInfoStart) < CAMX_ARRAY_SIZE(StatisticsInfoPropertyStrings))
                {
                    pTagName = StatisticsInfoPropertyStrings[id & ~MetadataSectionStatisticsInfoStart];
                }
                break;
            case (MetadataSectionTonemapStart >> 16):
                if ((id & ~MetadataSectionTonemapStart) < CAMX_ARRAY_SIZE(TonemapPropertyStrings))
                {
                    pTagName = TonemapPropertyStrings[id & ~MetadataSectionTonemapStart];
                }
                break;
            case (MetadataSectionLedStart >> 16):
                if ((id & ~MetadataSectionLedStart) < CAMX_ARRAY_SIZE(LedPropertyStrings))
                {
                    pTagName = LedPropertyStrings[id & ~MetadataSectionLedStart];
                }
                break;
            case (MetadataSectionInfoStart >> 16):
                if ((id & ~MetadataSectionInfoStart) < CAMX_ARRAY_SIZE(InfoPropertyStrings))
                {
                    pTagName = InfoPropertyStrings[id & ~MetadataSectionInfoStart];
                }
                break;
            case (MetadataSectionBlackLevelStart >> 16):
                if ((id & ~MetadataSectionBlackLevelStart) < CAMX_ARRAY_SIZE(BlackLevelPropertyStrings))
                {
                    pTagName = BlackLevelPropertyStrings[id & ~MetadataSectionBlackLevelStart];
                }
                break;
            case (MetadataSectionSyncStart >> 16):
                if ((id & ~MetadataSectionSyncStart) < CAMX_ARRAY_SIZE(SyncPropertyStrings))
                {
                    pTagName = SyncPropertyStrings[id & ~MetadataSectionSyncStart];
                }
                break;
            case (MetadataSectionReprocessStart >> 16):
                if ((id & ~MetadataSectionReprocessStart) < CAMX_ARRAY_SIZE(ReprocessPropertyStrings))
                {
                    pTagName = ReprocessPropertyStrings[id & ~MetadataSectionReprocessStart];
                }
                break;
            case (MetadataSectionDepthStart >> 16):
                if ((id & ~MetadataSectionDepthStart) < CAMX_ARRAY_SIZE(DepthPropertyStrings))
                {
                    pTagName = DepthPropertyStrings[id & ~MetadataSectionDepthStart];
                }
                break;
            default:
                // Unknown Property ID
                pTagPrefix = NULL;
                break;
        }

        // Only output if a name was found
        if ((pTagPrefix != NULL) && (pTagName != NULL))
        {
            *ppTagPrefix    = pTagPrefix;
            *ppTagName      = pTagName;
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Unknown PropertyID %08x", tag);
        }
    }

    return result;
}

CAMX_NAMESPACE_END
