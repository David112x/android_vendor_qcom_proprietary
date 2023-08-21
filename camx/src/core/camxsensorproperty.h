////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxsensorproperty.h
/// @brief Define sensor properties per usecase and per frame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXSENSORPROPERTY_H
#define CAMXSENSORPROPERTY_H

#include "camxdefs.h"
#include "camxsensordriver.h"
#include "camxstaticcaps.h"
#include "camxtypes.h"
#include "camxpdafconfig.h"

CAMX_NAMESPACE_BEGIN

static const UINT MaxSensorModes = 50; ///< Max number of sensor modes

/// @brief CSI Data Type (DT)
/// @note This is not the complete set, many more DT-s are sensor specific, to be added as needed
static const UINT8 CSIDataTypeEmbedData   = 0x12; ///< Data type for EmbedData format
static const UINT8 CSIDataTypeYUV422_8    = 0x1E; ///< Data type for YUV422_8 format
static const UINT8 CSIDataTypeRaw8        = 0x2A; ///< Data type for Raw8 format
static const UINT8 CSIDataTypeRaw10       = 0x2B; ///< Data type for Raw10 format
static const UINT8 CSIDataTypeRaw12       = 0x2C; ///< Data type for Raw12 format
static const UINT8 CSIDataTypeRaw14       = 0x2D; ///< Data type for Raw14 format

/// @brief CSID config
static const UINT8  CSIDecode6Bit         = 0; ///< CSID config data
static const UINT8  CSIDecode8Bit         = 1; ///< CSID config data
static const UINT8  CSIDecode10Bit        = 2; ///< CSID config data
static const UINT8  CSIDecode12Bit        = 3; ///< CSID config data
static const UINT8  CSIDecode14Bit        = 8; ///< CSID config data
static const UINT8  CSIDecodeDPCM_10_8_10 = 5; ///< CSID config data

/// @brief Sensor pixel format including filter arrangement
enum class PixelFormat
{
    Invalid = -1,           ///< Invalid type
    BayerBGGR,              ///< Bayer format BGGR
    BayerGBRG,              ///< Bayer format GBRG
    BayerGRBG,              ///< Bayer format GRBG
    BayerRGGB,              ///< Bayer format RGGB
    YUVFormatY,             ///< YCbCr format Y
    YUVFormatUYVY,          ///< YCbCr format UYVY
    YUVFormatYUYV,          ///< YCbCr format YUYV
    MetaStatsEmbed,         ///< Meta format Embedded
    MetaStatsPDAF,          ///< Meta format PDAF
    MetaStatsHDR,           ///< Meta format HDR
    Monochrome,             ///< Mono format
};

/// @brief Sensor operation mode for this usecase
enum class UsecaseMode
{
    Invalid = -1,    ///< Invalid type
    Default,         ///< Non HFR mode for normal camera, camcorder usecases
    HFR,             ///< HFR mode used to capture slow motion video
    HDR,             ///< HDR mode used to High Dynamic Range imaging
    RawHDR,          ///< RAW HDR mode used to stream raw HDR
};

/// @brief Sensor PDAF Buffer Format
enum class PDBufferFormat
{
    MIPI8,
    MIPI10,
    PACKED10,
    UNPACKED16
};

/// @brief Sensor PDAF Coordinates
struct PDPixelCoordinates
{
    UINT32  PDXCoordinate;
    UINT32  PDYCoordinate;
};

/// @brief Sensor output resolution
struct SensorResolution
{
    UINT    outputWidth;    ///< Output width
    UINT    outputHeight;   ///< Output height
};

/// @brief Sensor output offset
struct SensorPixelOffset
{
    UINT    xStart;    ///< x Start
    UINT    yStart;    ///< y Start
};

/// @brief Sensor crop info
struct SensorCropInfo
{
    UINT    firstPixel;     ///< left crop
    UINT    firstLine;      ///< top crop
    UINT    lastPixel;      ///< outputWidth - right crop -1
    UINT    lastLine;       ///< outputHeight - bottom crop -1
};

/// @brief Maximum stream configuration structure per sensor resolution usecase
static const UINT MaxSensorModeStreamConfigCount = 4;


/// @brief Maximum sensor capabilities supported per sensor resolution (Normal, HFR, HDR, PDAF, QUADCFA, ZZHDR, FASTAEC, DEPTH, SHDR, FS, Internal)
static const UINT MaxSensorCapabilityCount       = 11;

/// @brief Structure describing current sensor mode in a usecase
struct SensorMode
{
    UINT8                           decodeFormat;                                 ///< CSI Decode Format
    UINT64                          outPixelClock;                                ///< Output pixel clock
    UINT64                          vtPixelClock;                                 ///< Vt pixel clock
    PixelFormat                     format;                                       ///< Pixel format
    DOUBLE                          maxFPS;                                       ///< Max fps for the mode
    UINT32                          capabilityCount;                              ///< sensor capabilities count
    SensorCapability                capability[MaxSensorCapabilityCount];         ///< Normal, HFR, HDR, PDAF, QUADCFA, ZZHDR etc
    UINT                            binningTypeH;                                 ///< Binning horizontal
    UINT                            binningTypeV;                                 ///< Binning vertical
    UINT                            numPixelsPerLine;                             ///< Number of pixels per line (lineLengthPclk)
    UINT                            numLinesPerFrame;                             ///< Number of lines per frame (frameLengthLines)
    UINT                            maxLineCount;                                 ///< Max line count
    UINT32                          minHorizontalBlanking;                        ///< Minimum horizontal blanking interval in pixels
    UINT32                          minVerticalBlanking;                          ///< Minimum horizontal blanking interval in lines
    DOUBLE                          maxGain;                                      ///< Max gain
    SensorResolution                resolution;                                   ///< Sensor output resolution for this mode
    SensorPixelOffset               offset;                                       ///< Sensor offset
    SensorCropInfo                  cropInfo;                                     ///< CSID crop requested
    CHIRectangle                    activeArrayCropWindow;                        ///< Crop co-ordinates with respect to
                                                                                  ///  active pixel array.
    BOOL                            is3Phase;                                     ///< FALSE for DPHY, TRUE for CPHY
    FLOAT                           downScaleFactor;                              ///< down scale factor
    UINT32                          CSIPHYId;                                     ///< PHY slot id
    UINT32                          laneCount;                                    ///< lane Count
    UINT32                          laneMask;                                     ///< lane Mask, config which lane is used
    UINT64                          streamConfigCount;                            ///< Stream configurations count
    StreamConfiguration             streamConfig[MaxSensorModeStreamConfigCount]; ///< Stream Configurations
    UINT                            ZZHDRColorPattern;                            ///< ZZHDR Color Pattern Information
    ZZHDRFirstExposurePattern       ZZHDRFirstExposure;                           ///< ZZHDR First Exposure Pattern
    UINT                            HDR3ExposureTypeInfo;                         ///< 3-Exposure HDR type
    UINT                            RemosaicType;                                 ///< Remosaic type
    UINT                            sensorFrameSkip;                              ///< Initial frames that need to be skipped from sensor
    UINT32                          mipiFlags;                                    ///< mipi Receiver settings
};

/// @brief Structure describing sensor modes for the current usecase
struct UsecaseSensorModes
{
    SensorMode allModes[MaxSensorModes];    ///< Supported sensor modes for this camera session
};

/// @brief Structure describing Lens info
struct LensInfo
{
    FLOAT  focalLength;            ///< Focal length of the sensor
    FLOAT  fNumber;                ///< f-number or aperture of the lens
    FLOAT  pixelSize;              ///< Pixel size of the sensor
    FLOAT  totalFDistance;         ///< Total f-distance
    FLOAT  actuatorSensitivity;    ///< Sensitivity of the actuator
};

/// @brief Structure describing sensor metadata for a frame
struct SensorMetaData
{
    UINT32 width;               ///< Width of the frame
    UINT32 height;              ///< Height of this frame
    FLOAT  sensorGain;          ///< Sensor real gain programed in this frame
    UINT32 frameLengthLines;    ///< Frame length lines of this frame
    UINT64 exposureTime;        ///< Exposure time of this frame
    FLOAT  shortSensorGain;     ///< Short sensor real gain programed in this frame
    UINT64 shortExposureTime;   ///< Short sensor exposure time for this frame
    INT32  sensitivity;         ///< Sensor sensitivity
    UINT32 testPatternMode;     ///< Test pattern mode
    UINT32 filterArrangement;   ///< Colour filter pattern
    UINT64 frameDuration;       ///< Total frame duration
    UINT64 rollingShutterSkew;  ///< Rolling shutter skew
};

/// @brief Structure describing sensor current mode resolution info for a frame
struct SensorResolutionInfo
{
    UINT64 vtPixelClk;          ///< Video domain timing clock value
    UINT16 lineLengthPixelClk;  ///< Line length pixel clock of frame
    FLOAT  frameRate;           ///< Frame rate of this frame
    UINT32 frameLengthLine;     ///< Frame length line of this frame
    UINT64 lineReadoutTime;     ///< one line read out time
};

/// @brief Structure describing camera module configuration as read from the driver
struct CameraConfigurationInformation
{
    UINT32 mountAngle;        ///< Camera mount angle
    UINT32 imageOrientation;  ///< Camera image Orientation
    UINT   position;          ///< Camera facing position
};

/// @brief Sensor PDAF Information needed for BPS module configuration
struct SensorPDAFInfo
{
    UINT32               PDAFBlockWidth;          ///< PDAF block width
    UINT32               PDAFBlockHeight;         ///< PDAF block height
    UINT32               PDAFGlobaloffsetX;       ///< PDAF offset X
    UINT32               PDAFGlobaloffsetY;       ///< PDAF offset Y
    UINT32               PDAFPixelCount;          ///< PDAF Pixel count
    PDPixelCoordinates   PDAFPixelCoords[256];    ///< PDAF Pixel Coordinates
    PDBufferFormat       PDAFBufferFormat;        ///< PDAF Buffer Format
    PDBufferFormat       PDAFNativeBufferFormat;  ///< PDAF Native Buffer Format
    PDAFType             PDAFSensorType;          ///< PDAF Sensor type
};

/// @brief Sensor Properties
struct SensorProperties
{
    INT32 sensingMethod;
    FLOAT focalLengthIn35mm;
};

CAMX_NAMESPACE_END

#endif // CAMXSENSORPROPERTY_H
