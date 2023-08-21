////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxstaticcaps.h
/// @brief CamX capability description.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXSTATICCAPS_H
#define CAMXSTATICCAPS_H

#include "camxtypes.h"
#include "camxhal3types.h"
#include "camxhal3metadatatags.h"
#include "camxhal3metadatatagtypes.h"

CAMX_NAMESPACE_BEGIN

// Forward declaration
class HwContext;
struct HwEnvironmentStaticCapsExtended;
struct PlatformStaticCapsExtended;

static const UINT MaxSensorConfigs          = 64;
static const UINT MaxOpticalBlackRegions    = 5;
static const UINT MaxHotPixels              = 255;
/// brief Maximum number of resolution for ScalerAvailableStreamConfigurations metadata tag
static const UINT MaxResolutions            = 64;
static const UINT MaxRawSizesSupported      = MaxSensorStreamConfigurations * MaxResolutions;
static const UINT MaxHFRConfigs             = 64;
static const UINT MaxStreamConfigs          = MaxScalerFormats * MaxResolutions * ScalerAvailableStreamConfigurationsEnd;
static const UINT MaxFrameDurations         = MaxScalerFormats * MaxResolutions;
static const UINT MaxStallDurations         = MaxResolutions;

static const UINT CustomHFRTableEntrySize         = 3;        ///< Width, height, fps
static const UINT PreviewVideoFPSTableEntrySize   = 4;        ///< Width, height, previewfps and videofps

static const UINT MaxCustomHFRSizes         = MaxResolutions * CustomHFRTableEntrySize;
static const UINT MaxPreviewVideoFPS        = MaxResolutions * PreviewVideoFPSTableEntrySize;
static const UINT VideoMitigationsTableEntrySize  = 6;        ///< Width,height,Videofps,maxPreviewFPS,liveshot?,EIS?
static const UINT MaxNumVideoFPS                  = 6;        ///< 30 , 60, 120, 240, 480, 960
static const UINT MaxMitigationsTableSize   = MaxResolutions * MaxNumVideoFPS * VideoMitigationsTableEntrySize;

static const UINT32 QuantTableSize          = 64;
static const UINT SensorModeTableEntrySize  = 3;                                ///< Width, height, fps
static const UINT MaxSensorModeTableEntries = (MaxResolutions * SensorModeTableEntrySize) + 2;

// @brief Lens Characteristics Array Sizes
static const UINT LensPoseTranslationSize      = 3;
static const UINT LensPoseRotationSize         = 4;
static const UINT LensIntrinsicCalibrationSize = 5;
static const UINT LensDistortionSize           = 5;

/// @brief Camera image sensor static capabilities. These capabilities will be used to populate HAL static metadata.
struct SensorModuleStaticCaps
{
    UINT32                      sensorId;                                       ///< Sensor Id from module driver xml
    CHAR*                       pSensorName;                                    ///< Sensor Name from sensor driver xml
    BOOL                        isYUVCamera;                                    ///< Camera is a YUV camera
    UINT                        position;                                       ///< camera position and role
    SensorConfiguration         sensorConfigs[MaxResolutions];                  ///< Available sensor configurations
    UINT                        numSensorConfigs;                               ///< Number of valid entries in sensorConfig
    Region                      activeArraySize;                                ///< The area of the image sensor which
                                                                                ///  corresponds to active pixels after any
                                                                                ///  geometric distortion correction has been
                                                                                ///  applied
    Region                      QuadCFAActiveArraySize;                         ///< The active array size exposed to framework
                                                                                ///  for quadcfa sensor, usually half size of
                                                                                ///  the original actual active array size
    FLOAT                       pixelSize;                                      ///< Physical size of one pixel in micro metre
    DimensionCapFloat           physicalSensorSize;                             ///< The physical size of the full pixel array
    DimensionCap                pixelArraySize;                                 ///< Size of the full pixel array, possibly
                                                                                ///  including black calibration pixels
    DimensionCap                QuadCFAPixelArraySize;                          ///< The pixel array size exposed to framework
                                                                                ///  for quadcfa sensor, usually half size of
                                                                                ///  the original actual pixel array size
    BOOL                        hasFlash;                                       ///< Flash is associated with this camera
    INT32                       minISOSensitivity;                              ///< Minimum ISO sensitivity
    INT32                       maxISOSensitivity;                              ///< Maximum ISO sensitivity
    UINT64                      minExposureTime;                                ///< Minimum exposure time supported
    UINT64                      maxExposureTime;                                ///< Maximum exposure time supported
    UINT64                      maxFrameDuration;                               ///< Maximum frame duration
    BOOL                        lensShadingAppliedInSensor;                     ///< Lens shading correction applied in sensor
    // NOWHINE NC008: Abbreviation must be uppercase
    FLOAT                       NDFs[MaxTagValues];                             ///< Available neutral density filters in EV
    UINT                        numNDFs;                                        ///< Number of valid entries in NDFs
    INT32                       maxAnalogSensitivity;                           ///< Maximum sensitivity (ISO) that is
                                                                                ///  implemented purely through analog gain
    INT32                       testPatterns[SensorTestPatternModeValuesCount]; ///< Supported test patterns
    UINT                        numTestPatterns;                                ///< Number of valid entries in testPatterns
    UINT16                      CSILaneAssign;                                  ///< CSI lane assign value
    BOOL                        isQuadCFASensor;                                ///< Sensor has Quad CFA mode
    BOOL                        isZZHDRSupported;                               ///< Sensor has zzHDR Support
    BOOL                        isIHDRSupported;                                ///< Sensor has 3HDR Support
    DimensionCap                QuadCFADim;                                     ///< Dimension of Quad CFA mode
    BOOL                        isSHDRSupported;                                ///< Sensor has SHDR Support
    BOOL                        isFSSensor;                                     ///< Sensor has Fast shutter mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Lens Information
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL                                    isFixedFocus;               ///< Image sensor module is fixed focus
    FLOAT                                   focalLengths[MaxTagValues]; ///< Focal lengths available to the camera
    UINT                                    numFocalLengths;            ///< Number of valid entries in focalLengths
    BOOL                                    hasOIS;                     ///< Sensor support for OIS
    FLOAT                                   hyperfocalDistance;         ///< Hyperfocal distance of the camera
    FLOAT                                   minimumFocusDistance;       ///< Minimum focus distance in diopters
    LensInfoFocusDistanceCalibrationValues  focusDistanceCalibration;   ///< Supported focus calibration
    UINT                                    numAperatures;              ///< Number of valid entries in apertures
    FLOAT                                   aperatures[MaxTagValues];   ///< Supported aperture values as F number
    DimensionCap                            lensShadingMapSize;         ///< Lens shading map dimensions

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Raw Support
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL                                    rawCalibrationAvailable;                    ///< Raw calibration parameters are
                                                                                        ///  valid for this image sensor module
    SensorInfoColorFilterArrangementValues  colorFilterArrangement;                     ///< The Bayer filter arrangement
    Region                                  preCorrectionActiveArraySize;               ///< The area of the image sensor
                                                                                        ///  which corresponds to active
                                                                                        ///  pixels prior to the application
                                                                                        ///  of any geometric distortion
                                                                                        ///  correction
    Region                                  QuadCFAPreCorrectionActiveArraySize;        ///< The precorrection active array size
                                                                                        ///  exposed to fwk for qcfa sensor,
                                                                                        ///  usually half size of the origin
    SensorReferenceIlluminantValues         referenceIlluminant1;                       ///< Reference illuminant 1 type
    SensorReferenceIlluminantValues         referenceIlluminant2;                       ///< Reference illuminant 2 type
    Rational                                calibrationTransform1[3][3];                ///< Per device color transform
                                                                                        ///  calibration for illuminant 1
    Rational                                calibrationTransform2[3][3];                ///< Per device color transform
                                                                                        ///  calibration for illuminant 2
    Rational                                colorTransform1[3][3];                      ///< Color transform from reference
                                                                                        ///  color space for illuminant 1
    Rational                                colorTransform2[3][3];                      ///< Color transform from reference
                                                                                        ///  color space for illuminant 2
    Rational                                forwardMatrix1[3][3];                       ///< Color transform to reference
                                                                                        ///  color space for illuminant 1
    Rational                                forwardMatrix2[3][3];                       ///< Color transform to reference
                                                                                        ///  color space for illuminant 2
    INT32                                   whiteLevel;                                 ///< Maximum raw output value
    INT32                                   blackLevelPattern[4];                       ///< A fixed black level offset for
                                                                                        ///  each of the CFA channels
    Region                                  opticalBlackRegion[MaxOpticalBlackRegions]; ///< List of disjoint rectangles
                                                                                        ///  indicating the sensor optically
                                                                                        ///  shielded black pixel regions
    UINT                                    numOpticalBlackRegions;                     ///< Number of valid entries in
                                                                                        ///  opticalBlackRegion
    BOOL                                    hotPixelMapAvailable;                       ///< Mapped hot pixels are available
                                                                                        ///  from the sensor
    SensorCoordinate                        hotPixels[MaxHotPixels];                    ///< List of pixel coordinates of
                                                                                        ///  hot pixels
    UINT                                    numHotPixels;                               ///< Number of valid entries in
                                                                                        ///  hotPixels
    ProfileHueSaturationMapDimensions       profileHueSaturationMapDimensions;          ///< The number of input samples for
                                                                                        ///  each dimension of the hue sat map
                                                                                        ///  profile supported

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Depth Support
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL                            depthCalibrationAvailable;  ///< Depth calibration is valid for this image sensor module
    LensPoseRotationParams          lensPoseRotation;           ///< Lens pose rotation calibration
    LensPoseTranslationParams       lensPoseTranslation;        ///< Lens pose translation calibration
    LensIntrinsicCalibrationParams  lensIntrinsicCalibration;   ///< Lens intrinsic calibration
    LensRadialDistortionParams      lensRadialDistortion;       ///< Lens radial distortion calibration
    BOOL                            isDepthSensor;              ///< Image sensor supports depth
    BOOL                            isDepthExclusive;           ///< Sensor only capable of depth output
    INT32                           depthSamples;               ///< Maximum number of points that a depth point cloud may have

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Calibration Data from EEPROM
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    EEPROMOTPData OTPData; ///< OTP data obtained from the EEPROM

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Data for HAL to check and release unused handles while exiting app
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HwContext*                      pHwContext;                       /// HW Context
    CSLHandle                       hCSLSession[2];                   /// CSL Session handle
    VOID*                           pSensorDeviceHandles;             /// Sensor Device Handles
    VOID*                           pSensorDeviceCache;               /// Sensor Device Cache

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Lens Pose Information for HAL to publish for dual camera cases
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FLOAT      lensPoseTranslationDC[LensPoseTranslationSize];            /// Lens Pose Translation
    FLOAT      lensPoseRotationDC[LensPoseRotationSize];                  /// Lens Pose Rotation
    FLOAT      lensIntrinsicCalibrationDC[LensIntrinsicCalibrationSize];  /// Lens Intrinsic Calibration
    UINT8      lensPoseReferenceDC;                                       /// Lens Pose Reference
    FLOAT      lensDistortionDC[LensDistortionSize];                      /// Lens Distortion
    UINT       lensPoseTranslationCount;                                  /// Lens Pose Translation  Array Size
    UINT       lensPoseRotationCount;                                     /// Lens Pose Rotation  Array Size
    UINT       lensIntrinsicCalibrationCount;                             /// Lens Intrinsic Calibration  Array Size
    UINT       lensDistortionCount;                                       /// Lens Distortion Array Size
};

/// @brief Camera chipset platform static capabilities. These capabilities will be used to populate HAL static metadata.
struct PlatformStaticCaps
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Color Correction
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ColorCorrectionAberrationModeValues abberationModes[ColorCorrectionAberrationModeEnd];  ///< Supported chromatic aberration
                                                                                            ///  modes
    UINT                                numAbberationsModes;                                ///< Count of supported chromatic
                                                                                            /// aberration modes

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 3A
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ControlAEAntibandingModeValues  antibandingModes[ControlAEAntibandingModeEnd];  ///< Supported Antibanding modes
    UINT                            numAntibandingModes;                            ///< Count of supported antibanding modes
    ControlAEModeValues             AEModes[ControlAEModeEnd];                      ///< Supported AE modes
    UINT                            numAEModes;                                     ///< Count of supported antibranding modes
    INT32                           minAECompensationValue;                         ///< Minimum supported AECompensation value
    INT32                           maxAECompensationValue;                         ///< Maximum supported AECompensation value
    INT32                           defaultAECompensationValue;                     ///< Default AECompensation value
    Rational                        AECompensationSteps;                            ///< AECompensation steps
    ControlAFModeValues             AFModes[ControlAFModeEnd];                      ///< Supported AF modes
    UINT                            numAFModes;                                     ///< Count of supported AF modes
    ControlAWBModeValues            AWBModes[ControlAWBModeEnd];                    ///< Supported AWB modes
    UINT                            numAWBModes;                                    ///< Count of supported AWB modes
    INT32                           maxRegionsAE;                                   ///< Number of AE metering regions
    INT32                           maxRegionsAWB;                                  ///< Number of AWB metering regions
    INT32                           maxRegionsAF;                                   ///< Number of AF metering regions
    BOOL                            lockAEAvailable;                                ///< AEC lock supported
    BOOL                            lockAWBAvailable;                               ///< AWB lock supported

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Controls
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ControlEffectModeValues             effectModes[ControlEffectModeEnd];                          ///< Supported effect modes
    UINT                                numEffectModes;                                             ///< Count of supported
                                                                                                    ///  effect modes
    ControlSceneModeValues              sceneModes[ControlSceneModeEnd];                            ///< Supported scene modes
    SceneModeOverrideConfig             sceneModeOverride[ControlSceneModeEnd];                     ///< Scene mode overrides
    UINT                                numSceneModes;                                              ///< Count of supported
                                                                                                    ///  scene modes
    ControlVideoStabilizationModeValues videoStabilizationsModes[ControlVideoStabilizationModeEnd]; ///< Supported video
                                                                                                    ///  stabilization modes
    UINT                                numVideoStabilizationModes;                                 ///< Count of supported
                                                                                                    ///  video stabilization
                                                                                                    ///  modes
    ControlModeValues                   availableModes[ControlModeEnd];                             ///< Supported control modes
    UINT                                numAvailableModes;                                          ///< Count of supported
                                                                                                    ///  control modes
    INT32                               minPostRawSensitivityBoost;                                 ///< Minimum post raw
                                                                                                    ///  sensitivity boost ISO
    INT32                               maxPostRawSensitivityBoost;                                 ///< Maximum post raw
                                                                                                    ///  sensitivity boost ISO
    HFRConfigurationParams              defaultHFRVideoSizes[MaxHFRConfigs];                        ///< Supported HFR >=120fps
                                                                                                    ///  resolutions and min
                                                                                                    ///  frame duration
    UINT                                numDefaultHFRVideoSizes;                                    ///< Valid entries in
                                                                                                    ///  defaultHFRVideoSizes
    INT32                               IFEMaxLineWidth;                                            ///<  MAX IFE Resolution

    UINT32                              numIFEsforGivenTarget;                                      ///< Number of IFE's

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Edge
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    EdgeModeValues  edgeModes[EdgeModeEnd]; ///< Supported edge enhancement modes
    UINT            numEdgeModes;           ///< Count of supported edge enhancement modes

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Hot pixel
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HotPixelModeValues  hotPixelModes[HotPixelModeEnd]; ///< Supported hot pixel modes
    UINT                numHotPixelModes;               ///< Count of supported hot pixel modes

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // JPEG
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DimensionCap    JPEGThumbnailSizes[MaxTagValues];   ///< Supported JPEG thumbnail sizes
    UINT            numJPEGThumbnailSizes;              ///< Count of supported JPEG thumbnail sizes
    FLOAT           JPEGFileSizeScaleFactor;            ///< Scale factor of JPEG resolution for max file size calculation
    DimensionCap    defaultJPEGThumbnailSize;           ///< Default Thumbnail size for JPEG

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Noise Reduction
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    NoiseReductionModeValues    noiseReductionModes[NoiseReductionModeEnd]; ///< Supported noise reduction modes
    UINT                        numNoiseReductionModes;                     ///< Count of supported noise reduction modes

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Request
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32   maxRawStreams;                  ///< Maximum concurrent raw streams
    INT32   maxProcessedStreams;            ///< Maximum concurrent processed (non stalling) streams
    INT32   maxProcessedStallingStreams;    ///< Maximum concurrent processed stalling streams
    INT32   maxInputStreams;                ///< Maximum input streams
    UINT8   maxPipelineDepth;               ///< maximum pipeline delay
    INT32   partialResultCount;             ///< Number of partial results

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Scaler
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FLOAT                        maxDigitalZoom;                        ///< Max digital zoom
    ScalerAvailableFormatsValues scalerFormats[MaxScalerFormats];       ///< Supported scaler formats
    UINT                         numScalerFormats;                      ///< Number of scaler formats
    INT32                        inputOutputFormatMap[MaxTagValues];    ///< Input output format map
    UINT                         numInputOutputFormatMaps;              ///< Number of valid entries in inputOutputFormatMap
    DimensionCap                 defaultImageSizes[MaxResolutions];     ///< Supported image resolutions
    UINT                         numDefaultImageSizes;                  ///< Number of supported resolutions
    DimensionCap                 videoResolutions[MaxResolutions];      ///< Supported Video resolutions
    UINT                         numVideoResolutions;                   ///< Number of supported video resolutions
    FLOAT                        JPEGFormatStallFactorPerPixel;         ///< Additional JPEG processing time per pixel in ns
    FLOAT                        rawSensorFormatStallFactorPerPixel;    ///< Additional raw conversion time per pixel in ns
    ScalerCroppingTypeValues     croppingSupport;                       ///< Support free form or center only cropping
    RangeINT32                   defaultTargetFpsRanges[MaxTagValues];  ///< Supported default fps ranges
    UINT                         numDefaultFpsRanges;                   ///< Number of default supported fps range

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sensor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SensorInfoTimestampSourceValues timestampSource;    ///< Supported time stamp source

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Shading
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ShadingModeValues   shadingModes[ShadingModeEnd];   ///< Supported lens shading correction modes
    UINT                numShadingModes;                ///< Number of supported lens shading correction modes
    BOOL                lensShadingMapAvailable;        ///< Lens shading map available

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Statistics
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    StatisticsFaceDetectModeValues  faceDetectModes[StatisticsFaceDetectModeEnd];   ///< Face detection modes available
    UINT                            numFaceDetectModes;                             ///< Number of face detection modes
    INT32                           maxFaceCount;                                   ///< Maximum number of faces detectable

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Tonemap
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT                maxTonemapCurvePoints;          ///< Maximum number of tonemap curve points supported
    TonemapModeValues   tonemapModes[TonemapModeEnd];   ///< Supported tonemap modes
    UINT                numTonemapModes;                ///< Number of supported tonemap modes

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Reprocess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32   maxCaptureStall;    ///< The maximal camera capture pipeline stall (in unit of frame count) introduced by a
                                ///  reprocess capture request

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Support level
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    InfoSupportedHardwareLevelValues    supportedHwLevel;                               ///< Supported HW level
    RequestAvailableCapabilitiesValues  requestCaps[RequestAvailableCapabilitiesEnd];   ///< Supported request caps
    UINT                                numRequestCaps;                                 ///< Number of supported request caps
    CameraMetadataTag                   requestKeys[MaxMetadataTagCount];               ///< Available supported request keys
    UINT                                numRequestKeys;                                 ///< Number of supported request keys
    CameraMetadataTag                   resultKeys[MaxMetadataTagCount];                ///< Available supported request keys
    UINT                                numResultKeys;                                  ///< Number of supported result keys
    CameraMetadataTag                   characteristicsKeys[MaxMetadataTagCount];       ///< Available supported
                                                                                        ///  characteristic keys
    UINT                                numCharacteristicsKeys;                         ///< Number of supported
                                                                                        ///  characteristic keys
#if ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) // Android-P or better
    CameraMetadataTag                   sessionKeys[MaxMetadataTagCount];               ///< Available supported
                                                                                        ///  session keys
    UINT                                numSessionKeys;                                 ///< Number of supported
#endif // ((CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28))                              ///  session keys

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sync
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SyncMaxLatencyValues                syncMaxLatency;                                 ///< Sync max latency, 0 for per-frame

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Exposure Metering
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ExposureMeteringAvailableModes      exposureMeteringModes[ExposureMeteringEnd];     ///< Supported Metering modes
    UINT                                numExposureMeteringModes;                       ///< Count of supported

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Saturation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SaturationRange                     saturationRange;                                ///< Supported Saturation Range

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // LTM CONTRAST
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    LtmContrastRange                    ltmContrastRange;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Additional Info
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HALPixelFormat internalPixelFormats[MaxHALPixelFormats];    ///< Supported internal HAL pixel formats
    UINT           numInternalPixelFormats;                     ///< Number of supported internal HAL pixel formats
    INT32          maxDownscaleRatio;                           ///< The maximum dowscale ratio of the pipeline.

    // ISO Modes
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ISOModes                            ISOAvailableModes[ISOModeEnd];                  ///< Supported ISO Modes
    UINT                                numISOAvailableModes;                           ///< Count of supported ISO Modes

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Default JPEG Quantization Table Luma
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT16                              defaultJPEGQuantTableLuma[QuantTableSize]; ///< Luma Table

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Default JPEG Quantization Table Chroma
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT16                              defaultJPEGQuantTableChroma[QuantTableSize]; ///< Chroma Table

    // Sharpness
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SharpnessRange                      sharpnessRange;                                 ///< Supported Sharpness Range

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Histogram
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32                               histogramBuckets;                               ///< Supported Histogram buckets
    INT32                               histogramCount;                                 ///< Supported Histogram Count

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Instant AEC
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    InstantAecAvailableModes            instantAecAvailableModes[InstantAecEnd];        ///< Supported Instant AEC modes
    UINT                                numInstantAecModes;                             ///< Count of supported

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Color Temperature
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ColorTemperatureRange                colorTemperatureRange;                           ///< Supported Color Temperature Range

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // White Balance Gains
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    WBGainsRange                         whiteBalanceGainsRange;                          ///< Supported WB Gains Range

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Custom Sensor Mode Table
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32                               supportedSensorModes[MaxSensorModeTableEntries];    ///< Supported Custom Fps Range

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Processing Pixel Clock
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT                                maxBPSProcessingClockNom;                       ///< Max processing Nom clock of BPS.
                                                                                        /// This value considers efficiency
                                                                                        /// of the HW and overhead.
    UINT                                maxIPEProcessingClockOffline;                   ///< Max processing Nom clock of IPE.
                                                                                        /// This value considers efficiency
                                                                                        /// of the HW and overhead.
    UINT                                maxIPEProcessingClockRealtime;                  ///< Max processing SVS_L1 clock of IPE
                                                                                        /// This value considers efficiency
                                                                                        /// of the HW and overhead.
    FLOAT                               offlineIPEEfficiency;                           ///< Efficiency of IPE in offline mode
    FLOAT                               realtimeIPEEfficiency;                          ///< Efficiency of IPE in realtime mode
    FLOAT                               offlineIPEOverhead;                             ///< Overhead of IPE in offline mode
    FLOAT                               realtimeIPEOverhead;                            ///< Overhead of IPE in realtime mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Processing CSID Clock
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64                              maxCSIDTURBOClock;                 ///< Max Turbo clock of CSID

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Processing Pixel Clock
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64                              maxIFELowSVSClock;                 ///< Max Low SVS clock of IFE
    UINT64                              maxIFESVSClock;                    ///< Max SVS clock of IFE
    UINT64                              maxIFESVSL1Clock;                  ///< Max SVSL1 clock of IFE
    UINT64                              maxIFENOMClock;                    ///< Max Nominal clock of IFE
    UINT64                              maxIFETURBOClock;                  ///< Max Turbo clock of IFE

    UINT64                              minIFEHWClkRate;                   ///< Minimum clock rate required by chipset

    UINT32                              IFEPixelsPerClock;                 ///< Procesing pixels per clock of IFE
    UINT32                              IFEStatsFlushingCycles;            ///< Cycles needed to flushing stats
    UINT32                              IFEImageFlushingVBI;               ///< Required VBI to flushing image
    UINT32                              IFEDefaultMinVBI;                  ///< Default minimum VBI
    UINT32                              IFEDefaultMinHBIWithHVX;           ///< Default minimum HBI with HVX

    INT                                 maxNumberOfIFEsRequired;           ///< Number of IFEs required in the worst case

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Depth
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DepthAvailableFormatsValues         depthFormats[MaxDepthFormats];                  ///< Supported depth formats
    UINT                                numDepthFormats;                                ///< Number of depth formats

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ICA Capabilities
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IPEICACapability                         IPEICACapability;                          ///< ICA Capabilitites

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Mode FS
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL                                sensorModeFastShutter;                       ///< FS feature supported

    DimensionCap                        maxPreviewDimension;                         ///< Dimension of Max preview size
    BOOL                                isLiveshotSizeSameAsVideoSize;               ///< LiveshotSize is same as Video
    BOOL                                isFDRenderingInVideoUISupported;             ///< LiveshotSize is same as Video
    INT32                               maxPreviewFPS;                               ///< Max Preview FPS
    INT32                               maxSnapshotBw;                               ///< Max snapshot bandwidth

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // UBWC version
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT                                ubwcVersion;                       ///< Supported UBWC version 2 or 3 or 4
    BOOL                                ubwcLossyPreviewSupported;         ///< Supported UBWC lossy preview or not
    UINT                                ubwcLossyPreviewMinWidth;          ///< Minimum width to enable UBWC lossy preview
    UINT                                ubwcLossyPreviewMinHeight;         ///< Minimum height to enable UBWC lossy preview
    UINT                                ubwcLossyPreview10BitMinWidth;     ///< Minimum width to enable UBWC lossy preview
    UINT                                ubwcLossyPreview10BitMinHeight;    ///< Minimum height to enable UBWC lossy preview
    BOOL                                ubwcLossyVideoSupported;           ///< Supported UBWC lossy video or not
    BOOL                                ubwctp10PreviewVideoIPESupported;   ///< 10-bit IPE output supported or not
    UINT                                ubwcLossyVideoMinWidth;            ///< Minimum width to enable UBWC lossy video
    UINT                                ubwcLossyVideoMinHeight;           ///< Minimum height to enable UBWC lossy video
    UINT                                ubwcLossyVideo10BitMinWidth;       ///< Minimum width to enable UBWC lossy video
    UINT                                ubwcLossyVideo10BitMinHeight;      ///< Minimum height to enable UBWC lossy video

    /// @todo (CAMX-534): Add depth support for dual camera
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // HEIC
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8                                heicInfoSupported;                             ///< Is HEIC feature supported
    UINT8                                heicInfoMaxJpegAppSegmentsCount;               ///< HEIC max app segments
    PlatformStaticCapsExtended*          pExtended;                                     ///< The OEM extended capabilities

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // SSM
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL                                 ssmEnable;                                     ///< Is SSM Enabled
};

/// @brief Camera Hw environment static capabilities. These capabilities will be used to populate HAL static metadata.
struct HwEnvironmentStaticCaps
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Controls
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    RangeINT32                  AETargetFPSRanges[MaxTagValues];                ///< Supported list of AE FPS ranges
    UINT                        numAETargetFPSRanges;                           ///< Count of supported AE FPS ranges

    /// @todo (CAMX-961) Add filed to support android.control.availableHighSpeedVideoConfigurations base on sensor capability

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // JPEG
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32                       JPEGMaxSizeInBytes;                             ///< Maximum size in bytes for the compressed
                                                                                ///  JPEG buffer


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // HFR
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    UINT                        numHFRRanges;                                  ///< HFR ranges
    HFRConfigurationParams      HFRVideoSizes[MaxHFRConfigs];                  ///< Supported HFR (60fps)
                                                                               ///  resolutions and min
                                                                               ///  frame duration


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Scaler
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ScalerStreamConfig          streamConfigs[MaxStreamConfigs];                ///< The available stream configurations that
                                                                                ///  this camera device supports (i.e. format,
                                                                                ///  width, height, output/input stream).
    UINT                        numStreamConfigs;                               ///< Number of supported stream configurations.
    ScalerFrameDurationINT64    minFrameDurations[MaxFrameDurations];           ///< This lists the minimum frame duration for
                                                                                ///  each format/size combination.
    SensorRawOpaqueConfig       opaqueRawSizes[MaxOpaqueRawSizes];              ///< Opaque RAW sizes supported
    UINT                        opaqueRawSizesCount;                            ///< Opaque RAW size count
    UINT                        numMinFrameDurations;                           ///< Number of available stream minimum frame
                                                                                ///  durations.
    ScalerStallDurationINT64    minStallDurations[MaxStallDurations];           ///< This lists the maximum stall duration
                                                                                ///  for each output format/size combination.
    UINT                        numStallDurations;                              ///< Number of available stall durations.
    HFRCustomParams             customHFRParams[MaxCustomHFRSizes];             ///< Number of HFR 60, 90 resolution supported
    UINT                        numCustomHFRParams;                             ///< Number of available custom HFR Params
    HFRCustomPreviewVideoParams supportedHFRPreviewVideoFPS[MaxCustomHFRSizes]; ///< Number of HFR 60, 90 resolution supported
    UINT                        numSupportedPreviewVideoFPS;                    ///< Number of available custom HFR Params

    VideoMitigationsParams      videoMitigationsTable[MaxMitigationsTableSize]; ///< Number of HFR 60, 90 resolution supported
    UINT                        numVideoMitigations;                            ///< Number of available custom HFR Params

    HwEnvironmentStaticCapsExtended* pExtended;                                 ///< The OEM extended capabilities
};

CAMX_NAMESPACE_END

#endif // CAMXSTATICCAPS_H
