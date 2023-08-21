////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhal3metadatatagtypes.h
/// @brief Types associated with metadata tags, which are beyond basic types like int, float, enum etc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXHAL3METADATATAGTYPES_H
#define CAMXHAL3METADATATAGTYPES_H

#include "camxdefs.h"
#include "camxeepromdriver.h"
#include "camxpropertydefs.h"
#include "camxsensordriver.h"
#include "camxtypes.h"
#include "chivendortag.h"
CAMX_NAMESPACE_BEGIN

static const UINT MaxLightTypes = static_cast<UINT>(EEPROMIlluminantType::MAX);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Defines rational type
struct Rational
{
    INT32 numerator;    ///< Numerator in the rational
    INT32 denominator;  ///< Denominator in the rational
};

/// @brief Defines 32-bit range type
struct RangeINT32
{
    INT32 min;          ///< Minimum in the range
    INT32 max;          ///< Maximum in the range
};

/// @brief Defines 64-bit range type
struct RangeINT64
{
    INT64 min;          ///< Minimum in the range
    INT64 max;          ///< Maximum in the range
};

/// @brief Defines float range type
struct RangeFLOAT
{
    FLOAT min;          ///< Minimum in the range
    FLOAT max;          ///< Maximum in the range
};

/// @brief Defines 4 channel white balance gains
struct ColorCorrectionGain
{
    FLOAT red;          ///< Red pixels gain
    FLOAT greenEven;    ///< Green pixels gain on even rows
    FLOAT greenOdd;     ///< Green pixels gain on odd rows
    FLOAT blue;         ///< Blue pixels gain
};

/// @brief Defines a generic bounding rectangle
struct Rectangle
{
    INT32 xMin;         ///< Top-left X-coordinate
    INT32 yMin;         ///< Top-left y-coordinate
    INT32 xMax;         ///< Bottom-right x-coordinate
    INT32 yMax;         ///< Bottom-right y-coordinate
};

/// @brief Defines a weighted generic bounding rectangle
struct WeightedRectangle
{
    INT32 xMin;         ///< Top-left X-coordinate
    INT32 yMin;         ///< Top-left y-coordinate
    INT32 xMax;         ///< Bottom-right x-coordinate
    INT32 yMax;         ///< Bottom-right y-coordinate
    INT32 weight;       ///< Weight of the region
                        ///  The weight must be within [0, 1000], and represents a weight for every pixel in the area
};

/// @brief Defines a weighted rectangular region, used for focusing/metering areas
struct WeightedRegion
{
    INT32 xMin;         ///< Top-left x-coordinate of the region
    INT32 yMin;         ///< Top-left y-coordinate of the region
    INT32 xMax;        ///< Width of the region
    INT32 yMax;       ///< Height of the region
    INT32 weight;       ///< Weight of the region
                        ///  The weight must be within [0, 1000], and represents a weight for every pixel in the area
};

/// @brief Defines a 2x2 matrix of integers
struct Matrix2x2INT32
{
    INT32 val[2][2];    ///< Matrix values
};

/// @brief Defines a 3x3 matrix of floats
struct Matrix3x3Float
{
    FLOAT val[3][3];    ///< Matrix values
};

/// @brief Defines a 3x3 matrix of rationals
struct Matrix3x3Rational
{
    Rational val[3][3]; ///< Matrix values
};

/// @brief Defines maximum number of regions that can be used for metering in AE, AWB and AF
struct MaxNumRegions
{
    INT32 AE;           ///< Number of ae reqions
    INT32 AWB;          ///< Number of awb regions
    INT32 AF;           ///< Number of af regions
};

/// @brief structure for dimension capability.
struct DimensionCap
{
    INT32 width;    ///< Width
    INT32 height;   ///< Height
};

/// @brief structure for dimension capability expressed as floating point.
struct DimensionCapFloat
{
    FLOAT width;    ///< Width
    FLOAT height;   ///< Height
};

/// @brief Sensor output stream configuration
struct SensorStreamConfiguration
{
    UINT32              virtualChannel;  ///< virtualChannel of the output stream
    UINT32              dataType;        ///< DataType of the output stream
    DimensionCap        dimension;       ///< Dimension
    UINT32              bitWidth;        ///< number of bits
    StreamType          streamType;      ///< type of the output stream.
};

/// @brief Sensor output configuration
struct SensorConfiguration
{
    UINT32                      numStreamConfig;                                ///< number of Streams
    DOUBLE                      maxFPS;                                         ///< max FrameRate
    DOUBLE                      minFPS;                                         ///< min FrameRate
    SensorStreamConfiguration   streamConfigs[MaxSensorStreamConfigurations];   ///< streamConfiguration
};

/// @brief Sensor Raw Opaque config
struct SensorRawOpaqueConfig
{
    UINT32 width;                          ///< Width
    UINT32 height;                         ///< Height
    UINT32 size;                           ///< Buffer size in bytes
};

/// @brief stream configuration
struct StreamConfigurationParams
{
    INT32 width;                          ///< Width
    INT32 height;                         ///< Height
    INT64 minFrameDurationNanoSeconds;    ///< minimum frame duration(ns) for this size
};

/// @brief scaler stream configuration
struct ScalerStreamConfig
{
    INT32  format;                         ///< Scaler format
    UINT32 width;                          ///< Width
    UINT32 height;                         ///< Height
    INT32  type;                           ///< Stream input/output type
};

/// @brief scaler frame duration
struct ScalerFrameDurationINT64
{
    INT64 format;                         ///< Scaler format
    INT64 width;                          ///< Width
    INT64 height;                         ///< Height
    INT64 minFrameDurationNanoSeconds;    ///< minimum frame duration(ns)
};

/// @brief scaler stall duration
struct ScalerStallDurationINT64
{
    INT64 format;                         ///< Scaler format
    INT64 width;                          ///< Width
    INT64 height;                         ///< Height
    INT64 stallDurationNanoSeconds;       ///< stall duration(ns)
};

/// @brief HFR configuration
struct HFRConfigurationParams
{
    INT32 width;                ///< Width
    INT32 height;               ///< Height
    INT32 minFPS;               ///< minimum preview FPS
    INT32 maxFPS;               ///< maximum video FPS
    INT32 batchSizeMax;         ///< maximum batch size
};

/// @brief Custom HFR configuration
struct HFRCustomParams
{
    INT32 width;                ///< Width
    INT32 height;               ///< Height
    INT32 maxFPS;               ///< maximum video FPS
};

/// @brief Custom HFR configuration
struct HFRCustomPreviewVideoParams
{
    INT32 width;                ///< Width
    INT32 height;               ///< Height
    INT32 previewFPS;           ///< preview FPS
    INT32 videoFPS;             ///< video FPS
};

/// @brief Custom HFR configuration
struct VideoMitigationsParams
{
    INT32 width;                ///< Width
    INT32 height;               ///< Height
    INT32 maxPreviewFPS;        ///< Max Preview FPS
    INT32 videoFPS;             ///< video FPS
    BOOL  isLiveshotSupported;  ///< liveshot supported or not
    BOOL  isEISSupported;       ///< EIS supported or not
};


/// @brief The orientation of the camera relative to the sensor coordinate system.
struct LensPoseRotationParams
{
    FLOAT x;    ///< Quaternion coefficient x
    FLOAT y;    ///< Quaternion coefficient y
    FLOAT z;    ///< Quaternion coefficient z
    FLOAT w;    ///< Quaternion coefficient w
};

/// @brief the position of the camera optical center relative to the center of the largest camera.
struct LensPoseTranslationParams
{
    FLOAT x;    ///< X axis translation
    FLOAT y;    ///< Y axis translation
    FLOAT z;    ///< Z axis translation
};

/// @brief The parameters for this camera device's intrinsic calibration.
struct LensIntrinsicCalibrationParams
{
    FLOAT horizontalFocalLength;    ///< Horizontal focal length
    FLOAT verticalFocalLength;      ///< Vertical focal length
    FLOAT opticalAxisX;             ///< Position of the optical X axis
    FLOAT opticalAxisY;             ///< Position of the optical Y axis
    FLOAT skew;                     ///< Lens skew
};

/// @brief The correction coefficients to correct for this camera device's radial and tangential lens distortion.
struct LensRadialDistortionParams
{
    FLOAT kappa0; ///< Kappa0
    FLOAT kappa1; ///< Kappa1
    FLOAT kappa2; ///< Kappa2
    FLOAT kappa3; ///< Kappa3
    FLOAT kappa4; ///< Kappa4
    FLOAT kappa5; ///< Kappa5
};

/// @brief Describe a region.
struct Region
{
    INT32 xMin;     ///< X axis minimum value
    INT32 yMin;     ///< Y axis minimum value
    INT32 width;    ///< Width
    INT32 height;   ///< Height
};

/// @brief Sensor coordinate.
struct SensorCoordinate
{
    INT32 x;    ///< X axis point
    INT32 y;    ///< Y axis point
};

/// @brief The number of input samples for each dimension of SensorProfileHueSatMap
struct ProfileHueSaturationMapDimensions
{
    INT32 hueSamples;           ///< Hue samples
    INT32 saturationSamples;    ///< Saturation samples
    INT32 valueSamples;         ///< Value samples
};

/// @brief override controls for a scene mode.
struct SceneModeOverrideConfig
{
    ControlAEModeValues     AEModeOverride;     ///< AE Mode override
    ControlAWBModeValues    AWBModeOverride;    ///< AWB Mode override
    ControlAFModeValues     AFModeOverride;     ///< AF Mode override
};

/// @brief The mapping of image formats that are supported for input streams, to their corresponding output formats.
struct InputOutputFormatMap
{
    INT32 inputformat;                  ///< Input format
    INT32 numOutputFormats;             ///< Number of supported output formats
    INT32 outputFormat[MaxTagValues];   ///< List of supported output formats
};

/// @brief raw(un-formatted) OTP data obtained from EEPROM
struct RawOTPData
{
    BYTE*   pRawData;       ///< pointer to the raw/un-formatted OTP data read from EEPROM
    UINT32  rawDataSize;    ///< size of the raw/un-formatted  OTP data read from EEPROM
};

/// @brief custom information for EEPROM data
struct CustomInfo
{
    CHAR   name[MaximumStringLength];  ///< Name of the custom element to identify what kind of information it is
    INT32  value;                      ///< value of the custom element corresponding to the custom name
};

/// @brief contains raw OTP data and any other custom information.
struct EEPROMInformation
{
    RawOTPData  rawOTPData;                             ///< unformatted/raw OTP data information
    CustomInfo  customInfo[MaxEEPROMCustomInfoCount];   ///< Custom information configured specific to EEPROM and sensor
    UINT32      customInfoCount;                        ///< number of valid elemnets in the custom info data
};

/// @brief contains integration time related information.
struct IntegrationInformation
{
    UINT32    integrationTimeMin;         ///< Minimum intergation time
    UINT32    integrationTimeStep;        ///< The step of intergation time
    UINT32    integrationTimeMargin;      ///< Integration time margin
};

/// @brief contains AF lens calibration chart ditsance and DAC values for AF sync.
struct AFLensData
{
    UINT32  chartDistanceCM;    ///< Chart distance in centimetres
    INT16   DACValue;           ///< Lens position DAC value at chart distance
    FLOAT   stepPosition;       ///< lens position step value(linearized) corresponding to DAC value
};

/// @brief Auto focus calibration data to generate step table and taking care of hall effect
struct AFCalibrationData
{
    BOOL        isAvailable;                ///< Indicates whether AF calibration data is avialabe or not
    INT16       macroDAC;                   ///< DAC value for the actuator macro region
    INT16       infinityDAC;                ///< DAC value for the actuator infinity region
    AFLensData  calibrationInfo[MaxAFCalibrationDistances]; ///< contains chart distance and correpsonding DAC values
    UINT32      numberOfDistances;          ///< Number of calibrated distances
    FLOAT       macroMargin;                ///< margin to extend towards macro region
    FLOAT       infinityMargin;             ///< margin to extend towards infinity region
    UINT16      hallOffsetBias;             ///< hall bias value to write
    UINT32      hallRegisterAddr;           ///< hall register address to update OTP data
    INT16       gravityOffset0to90;         ///< memory offset value value of gravity offset from 0 to 90
    INT16       gravityOffset90to180;       ///< memory offset value value of gravity offset from 90 to 180
    UINT32      actuatorID;                 ///< actuator ID to identify the actuator when there are multiple actuators
};

/// @brief White balance calibration data
struct WBCalibrationData
{
    BOOL                 isAvailable;   ///< Indicates whether WB calibration data is avialabe or not.
    EEPROMIlluminantType illuminant;    ///< Indicates color temperature illuminant type
    FLOAT                rOverG;        ///< R over G data
    FLOAT                bOverG;        ///< B over G data
    FLOAT                grOverGB;      ///< GR over GB data
    SettingsInfo         settings;      ///< Contain the register settings to write for WB calibration
};

struct LSCCalibrationData
{
    BOOL                 isAvailable;                   ///< Indicates whether LSC calibration data is avialabe or not
    EEPROMIlluminantType illuminant;                    ///< Indicates color temperature illuminant type
    UINT16               meshHWRollOffSize;             ///< indicates the roll of table size
    FLOAT                rGain[HWRollOffTableSize];     ///< R gain table
    FLOAT                grGain[HWRollOffTableSize];    ///< GR gain table
    FLOAT                gbGain[HWRollOffTableSize];    ///< GB gain table
    FLOAT                bGain[HWRollOffTableSize];     ///< B gain table
    SettingsInfo         settings;                      ///< Contain the register settings to write for LSC calibration
};

/// @brief Absolute method AEC Sync info for multi-camera system
struct AbsoluteMethodAECSyncData
{
    UINT16 version;           ///< AEC sync OTP format version number
    FLOAT  gain;              ///< Gain of the camera when AEC is converged at D50 illuminant
    UINT16 averageLuma;       ///< Average luma measured from the setup when the camera is running AEC at D50 illuminant
    UINT16 exposureTimeUs;    ///< Exposure time in microseconds of the camera when AEC is converted at D50 illuminant
};

/// @brief dual camera parametrs specific to each sensor (main and auxiliary)
struct DualCameraLensCalibrationData
{
    FLOAT       focalLength;                        ///< normalized focal length of the lens
    UINT16      nativeSensorResolutionWidth;        ///< Native sensor resolution width used to capture calibration image
    UINT16      nativeSensorResolutionHeight;       ///< Native sensor resolution height used to capture calibration image
    UINT16      calibrationResolutionWidth;         ///< Image size width used internally by calibration tool
    UINT16      calibrationResolutionHeight;        ///< Image size height used internally by calibration tool
    FLOAT       focalLengthRatio;                   ///< focal length ratio
    AFLensData  AFSyncData[MaxAFCalibrationDistances]; ///< contains chart distance and correpsonding DAC values
    UINT32      numberOfDistances;                  ///< Number of calibrated distances
};

/// @brief dual camera parametrs for whole system
struct DualCameraSystemCalibrationdata
{
    UINT    calibrationFormatVersion;                    ///< Calibration format version information
    FLOAT   relativeRotationMatrix[9];                   ///< Relative viewpoint matching matrix w.r.t Main
    FLOAT   relativeGeometricSurfaceParameters[32];      ///< Relative geometric surface description parameters
    FLOAT   relativePrinciplePointXOfffset;              ///< Relative offset of sensor center from optical axis along
                                                         ///< horizontal dimension
    FLOAT   relativePrinciplePointYOffset;               ///< Relative offset of sensor center from optical axis along
                                                         ///< vertical dimension
    FLOAT   relativeBaselineDistance;                    ///< Camera separation in mm
    UINT16  relativePositionFlag;                        ///< 0=Main Camera is on the left of Aux; 1=Main Camera is on the
                                                         ///< right of Aux
    UINT16  masterSensorMirrorFlipSetting;               ///< master sensor mirror flip setting
    UINT16  auxSensorMirrorFlipSetting;                  ///< Aux sensor mirror flip setting
    UINT16  moduleOrientationFlag;                       ///< Flag indicating module orientation
    UINT16  rotationFlag;                                ///< rotation Flag
    FLOAT   brightnessRatio;                             ///< brightness ratio
    FLOAT   referenceSlaveGain;                          ///< reference module gain
    FLOAT   referenceSlaveExpTime;                       ///< Slave reference Exposure Time
    FLOAT   referenceMasterGain;                         ///< Master reference gain
    FLOAT   referenceMasterExpTime;                      ///< Master reference Exposure Time
    UINT32  referenceMasterColorTemperature;             ///< Master reference color temperature
    AbsoluteMethodAECSyncData absoluteMethodAECSyncData; ///< Absolute Method AEC Sync Data
};

/// @brief dual camera master structure holding all the master, auxilary and system parametrs
struct DualCameraCalibrationData
{
    BOOL                            isAvailable;               ///< Indicates whether DC calibration data is avialabe or not
    DualCameraLensCalibrationData   masterCalibrationData;     ///< data specific to the main sensor
    DualCameraLensCalibrationData   auxCalibrationData;        ///< data specific to the auxiliary sensor
    DualCameraSystemCalibrationdata systemCalibrationData;     ///< data specific whole DC system
    UINT32                          dualCameraOffset;          ///< data offset value
    UINT32                          dualCameraSize;            ///< data size
};

struct SPCCalibrationData
{
    BOOL            isAvailable;    ///< Indicates whether SPC calibration data is available or not
    SettingsInfo    settings;       ///< Contain the register settings to write for SPC calibration
};

/// @brief QSC (QCFA sensitivity correction) structure for holding QSC calibration data
struct QSCCalibrationData
{
    BOOL            isAvailable;    ///< Indicates whether QSC calibration data is available or not
    SettingsInfo    settings;       ///< Contain the register settings to write for QSC calibration
};

struct OISCalibrationData
{
    BOOL            isAvailable;    ///< Indicates whether OIS calibration data is available or not
    SettingsInfo    settings;       ///< Contain the register settings to write for OIS calibration
};

struct PDAFDCCCalibrationData
{
    BOOL    isAvailable;                                ///< Indicates whether PDAF DCC calibration data is avialabe or not
    UINT16  knotXCount;                                 ///< knot X count
    UINT16  knotYCount;                                 ///< Knot Y count
    FLOAT   slopeData[MaxPDAFKnotX*MaxPDAFKnotY];       ///< slope data
    FLOAT   offsetData[MaxPDAFKnotX*MaxPDAFKnotY];      ///< Offset Data
    UINT16  slopeOffsetAddressX[MaxPDAFKnotX];          ///< slope offset X address
    UINT16  slopeOffsetAddressY[MaxPDAFKnotY];          ///< slope offset Y address
};

struct PDAF2DCalibrationData
{
    BOOL    isAvailable;                                    ///< Indicates whether PDAF 2D calibration data is avialabe or not
    UINT16  versionNumber;                                  ///< PDAF 2D version number
    UINT16  mapWidth;                                       ///< map width value
    UINT16  mapHeight;                                      ///< map height value
    UINT16  leftGainMap[MaxPDAFWidth * MaxPDAFHeight];      ///< left gain map table
    UINT16  rightGainMap[MaxPDAFWidth * MaxPDAFHeight];     ///< right gain map table
    UINT16  centerGainMap[MaxPDAFWidth * MaxPDAFHeight];    ///< right gain map table
    INT16   conversionCoefficientPD[MaxPDAFWindow];         ///< PD conversion coefficient
    UINT16  versionNumDCC;                                  ///< DCC version number
    UINT16  qFactorDCC;                                     ///< DCC Q factor
    UINT16  mapWidthDCC;                                    ///< DCC map width
    UINT16  mapHeightDCC;                                   ///< DCC map height
    UINT16  upGainMap[MaxPDAFWidth * MaxPDAFHeight];        ///< up gain map table
    UINT16  downGainMap[MaxPDAFWidth * MaxPDAFHeight];      ///< down gain map table
};

struct LensData
{
    BOOL    isAvailable;                                    ///< Indicates whether PDAF 2D calibration data is avialabe or not
    FLOAT   horizontalFocalLength;                          ///< Horizontal focal length
    FLOAT   verticalFocalLength;                            ///< Vertical focal length
    FLOAT   opticalAxisX;                                   ///< Position of the optical X axis
    FLOAT   opticalAxisY;                                   ///< Position of the optical Y axis
    FLOAT   kappa0;                                         ///< Kappa0
    FLOAT   kappa1;                                         ///< Kappa1
    FLOAT   kappa2;                                         ///< Kappa2
    FLOAT   kappa3;                                         ///< Kappa3
    FLOAT   kappa4;                                         ///< Kappa4
};

/// @brief Structure describing OTP data
struct EEPROMOTPData
{
    AFCalibrationData               AFCalibration;                  ///< Auto Focus calibration data
    WBCalibrationData               WBCalibration[MaxLightTypes];   ///< White balance calibration data
    LSCCalibrationData              LSCCalibration[MaxLightTypes];  ///< lens shade correction calibration data
    DualCameraCalibrationData       dualCameraCalibration;          ///< Dual camera calibration data
    SPCCalibrationData              SPCCalibration;                 ///< Shield pixel correction calibration data
    QSCCalibrationData              QSCCalibration;                 ///< QBC sensitivity correction calibration data
    OISCalibrationData              OISCalibration;                 ///< Optical image stabilization calibration data
    PDAFDCCCalibrationData          PDAFDCCCalibration;             ///< PDAF defocus conversion coefficient
                                                                    ///< calibration data
    PDAF2DCalibrationData           PDAF2DCalibration;              ///< PDAF 2D calibration data
    LensData                        lensDataCalibration;            ///< Lens data
    EEPROMInformation               EEPROMInfo;                     ///< Information about Raw/unformatted OTP data read from
                                                                    /// eeprom and other configured custome info
};

/// @brief Static metadata keys array info which HW platform supported. Can be used to provide info of
///        RequestAvailableRequestKeys, RequestAvailableResultKeys, and RequestAvailableCharacteristicsKeys...etc.
struct StaticMetadataKeysInfo
{
    const CameraMetadataTag* pStaticKeys;  ///< Array of static metadata keys
    UINT                     numKeys;      ///< Number of keys in the staticKeys array
};

/// @brief The default request stucture which contains control request for all metadata in a template.
struct DefaultRequest
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Color correction
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ColorCorrectionModeValues           colorCorrectionMode;            ///< Color correction mode
    Matrix3x3Rational                   colorCorrectionTransform;       ///< Color correction transform
    ColorCorrectionGain                 colorCorrectionGains;           ///< Color correction gains
    ColorCorrectionAberrationModeValues colorCorrectionAberrationMode;  ///< Color correction aberration mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Control
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ControlAEAntibandingModeValues      AEAntibandingMode;          ///< AE antibanding mode
    INT32                               AEExposureCompensation;     ///< AE exposure compensation
    ControlAELockValues                 AELock;                     ///< AE lock
    ControlAEModeValues                 AEMode;                     ///< AE mode
    WeightedRectangle                   AERegions;                  ///< AE regions
    RangeINT32                          AETargetFPSRange;           ///< AE target FPS range
    ControlAEPrecaptureTriggerValues    AEPrecaptureTrigger;        ///< AE precapture trigger
    ControlAFModeValues                 AFMode;                     ///< AF mode
    WeightedRectangle                   AFRegions;                  ///< AF regions
    ControlAFTriggerValues              AFTrigger;                  ///< AF trigger
    ControlAWBLockValues                AWBLock;                    ///< AWB lock
    ControlAWBModeValues                AWBMode;                    ///< AWB mode
    WeightedRectangle                   AWBRegions;                 ///< AWB regions
    ControlCaptureIntentValues          captureIntent;              ///< Capture intent
    ControlEffectModeValues             effectMode;                 ///< Effect mode
    ControlModeValues                   controlMode;                ///< Control mode
    ControlSceneModeValues              sceneMode;                  ///< Scene mode
    ControlVideoStabilizationModeValues videoStabilizationMode;     ///< video stabilization mode
    INT32                               postRawSensitivityBoost;    ///< Additional sensitivity boost applied
    BOOL                                controlZslEnable;           ///< Control ZSL Enable

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Edge
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    EdgeModeValues                      edgeMode;   ///< Edge mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Flash
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FlashModeValues                     flashMode;  ///< Flash mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Hot pixel
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HotPixelModeValues                  hotPixelMode;   ///< Hot pixel mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Lens
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FLOAT                               aperture;                   ///< Lens aperture
    FLOAT                               filterDensity;              ///< Lens filter density
    FLOAT                               focalLength;                ///< Lens focal length
    FLOAT                               focalDistance;              ///< Lens focal distance
    LensOpticalStabilizationModeValues  opticalStabilizationMode;   ///< Lens optical stabilization mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Noise reduction
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    NoiseReductionModeValues            noiseRedutionMode;  ///< Noise reduction mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Request
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32                               requestId;  ///< Request id

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Scaler
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Region                              scalerCropRegion;   ///< Scaler crop region

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sensor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT64                               sensorExposureTime;     ///< Sensor exposure time
    INT64                               sensorFrameDuration;    ///< Sensor frame duration
    INT32                               sensorSensitivity;      ///< Sensor sensitivity
    SensorTestPatternModeValues         sensorTestPatternMode;  ///< Sensor test pattern mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Shading
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ShadingModeValues                   shadingMode;    ///< Shading mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Statistics
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    StatisticsFaceDetectModeValues      faceDetectMode;     ///< Statistics face detect mode
    StatisticsHotPixelMapModeValues     hotPixelMapMode;    ///< Statistics hot pixel map mode
    StatisticsLensShadingMapModeValues  lensShadingMapMode; ///< Statistics lens shading map mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Tonemap
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TonemapModeValues                   tonemapMode;    ///< Tonemap mode

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Black level
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BlackLevelLockValues                blackLevelLock; ///< Black level lock

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // JPEG
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DimensionCap thumbnailSize; ///< JPEG thumbnail size
};

/// @brief structure for saturation range.
struct SaturationRange
{
    INT32 minValue;      ///< Minimum Value
    INT32 maxValue;      ///< Maximum Value
    INT32 defaultValue;  ///< Default
    INT32 step;          ///< Step
};

/// @brief structure for Exposure time range.
struct ExposureTimeRange
{
    INT64 minExpTime;   ///< Minimum Value
    INT64 maxExpTime;   ///< Maximum Value
};

/// @brief structure for sharpness range.
struct SharpnessRange
{
    INT32 minValue;      ///< Minimum Value
    INT32 maxValue;      ///< Maximum Value
    INT32 defValue;      ///< Default Value
    INT32 step;          ///< Sharpness steps
};

/// @brief structure for Color Temperature range.
struct ColorTemperatureRange
{
    INT32 minValue;      ///< Minimum Value
    INT32 maxValue;      ///< Maximum Value
};

/// @brief structure for WB Gains range.
struct WBGainsRange
{
    FLOAT minValue;      ///< Minimum Value
    FLOAT maxValue;      ///< Maximum Value
};

/// @brief structure for ICA Capabilities.
struct IPEICACapability
{
    INT32 supportedIPEICATransformType;      ///< ICA Transform Type
    INT32 IPEICATransformGridSize;           ///< ICA Transform Grid Size
};

struct LtmContrastRange
{
    FLOAT min;       ///< Minimum Value
    FLOAT max;       ///< Mamimum Value
};

/// @brief structure forLtm Constrast strength Values.
struct LtmConstrast
{
    FLOAT ltmDynamicContrastStrength;    ///< Dynamic Constrast strength
    FLOAT ltmDarkBoostStrength;          ///< Dark Boost Strength
    FLOAT ltmBrightSupressStrength;      ///< Brightness Supress Strength
};

CAMX_NAMESPACE_END

#endif // CAMXHAL3METADATATAGTYPES_H
