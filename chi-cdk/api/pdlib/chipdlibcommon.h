/*======================================================================
    Copyright (c) 2017 - 2019 Qualcomm Technologies, Inc.
    All Rights Reserved.
    Confidential and Proprietary - Qualcomm Technologies, Inc.
=======================================================================*/

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chipdlibcommon.h
/// @brief Phase Detection Library interface definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CHIPDLIBCOMMON_H
#define CHIPDLIBCOMMON_H

#include "camxcdktypes.h"
#include "chistatsinterfacedefs.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/// @brief Data structures that are exposed to OEM must be packed to have the expected layout.
#pragma pack(push, CDK_PACK_SIZE)

static const UINT16 PDLibMaxWindowCount            = 200;   ///< Maximum fixed pd window number
static const UINT16 PDLibMaxPixelCount             = 256;   ///< Maximum number of pd pixels in a block
static const UINT8  PDLibMaxBlockHeight            = 64;    ///< Maximum height of pd block
static const UINT16 PDLibAdaptiveDataSize          = 960;   ///< Adaptive k data size
static const UINT16 PDLibGainMapLUTSize            = 221;   ///< Size of gain map LUT table
static const UINT16 PDLibMaxLeftOrRightPDPixels    = 128;   ///< Max number of left/right PD pixels in a sparse PD block
static const UINT16 PDLibMaxPDPixelRows            = 16;    ///< Max number of PD pixel rows in each block
static const UINT16 PDLibResamplerInstructionSize  = 32;    ///< Indicates the number of instructions provided for resampler
static const UINT16 PDLibResamplerInputCount       = 4;     ///< Number of input pixels for resampler instruction
static const UINT16 PDLibLCRColorComponentCount    = 4;     ///< Number of color component in bayer pattern for LCR
static const UINT16 PDLibLCRMaskCount              = 2;     ///< Number of LCR HW Mask to be used
static const UINT16 PDLibMaxPixelsInBlock          = 4096;  ///< In PD extractor indicates if a pixel in a block is PD pixel
                                                            ///< or not. Max [64][64]

/// @brief Defines the peripheral window type.
typedef enum
{
    PDLibPeripheralWindowTopLeft = 0,      ///< selected Top left corner of the window
    PDLibPeripheralWindowTopCenter,        ///< selected Top center of the window
    PDLibPeripheralWindowTopRight,         ///< selected Top right corner of the window
    PDLibPeripheralWindowLeft,             ///< selected left corner of the window
    PDLibPeripheralWindowRight,            ///< selected right corner of the window
    PDLibPeripheralWindowBottomLeft,       ///< selected Bottom left corner of the window
    PDLibPeripheralWindowBottomCenter,     ///< selected Bottom center of the window
    PDLibPeripheralWindowBottomRight,      ///< selected Bottom right corner of the window
    PDLibPeripheralWindowMax               ///< Anchor to indicate the last item in the defines
} PDLibPeripheralWindow;

/// @brief Defines the pdlib processing result.
typedef enum
{
    PDLibResultInital,         ///< initial value, buffer not processed
    PDLibResultCenter,         ///< pdlib select center window
    PDLibResultPeripheral      ///< pdlib select peripheral window
} PDLibProcessingResult;

///< @brief Defines Bayer pattern streamed by sensor
typedef enum
{
    PDLibBayerBGGR, ///< bayer pattern : BGGR
    PDLibBayerGBRG, ///< bayer pattern : GBRG
    PDLibBayerGRBG, ///< bayer pattern : GRBG
    PDLibBayerRGGB  ///< bayer pattern : RGGB
} PDLibSensorBayerPattern;

///< @brief Defines the function return status */
typedef enum
{
    PDLibSuccess,           ///< returns success
    PDLibMemoryError,       ///< returns memory error
    PDLibParamError,        ///< returns invalid input parameter
    PDLibRunningError,      ///< returns processing error
    PDLibErrorMax = 1023    ///< Anchor to indicate the last item in the defines
} PDLibReturnStatus;

///< @brief Defines the shielded pd pixel pattern
typedef enum
{
    PDLibLeftDiodePixel        = 1,    ///< Shielded Left Pixel for dual diode and 2x1 sensors
    PDLibRightDiodePixel       = 0,    ///< Shielded Right Pixel for dual diode and 2x1 sensors

    /// Added change for Vertical / 2x2 sensors
    PDLibUpDiodePixel          = 2,   /// Shielded Up pixel
    PDLibDownDiodePixel        = 3,   /// Shielded Down pixel

    PDLibTopLeftDiodePixel     = 4,   /// TopLeft Diode for 2x2 sensors
    PDLibTopRightDiodePixel    = 5,   /// TopRight Diode for 2x2 sensors
    PDLibBottomLeftDiodePixel  = 6,   /// BottomLeft Diode for 2x2 sensors
    PDLibBottomRightDiodePixel = 7,   /// BottomRight Diode for 2x2 sensors


    PDLibLeftShieldedPixel     = 0,    ///< metal shielded left pd pixel
    PDLibRightShieldedPixel    = 1,    ///< metal shielded right pd pixel

    /// Added change for Vertical / 2x2 sensors
    PDLibUpShieldedPixel       = 3,    ///< metal shielded up pd pixel
    PDLibDownShieldedPixel     = 2     ///< metal shielded down pd pixel

} PDLibPixelShieldInfo;

///< @brief Defined as a percentage of image size
typedef struct
{
    FLOAT startX;      ///< (0-1), percentage of start X coordinate in terms of image width
    FLOAT startY;      ///< (0-1), percentage of start Y coordinate in terms of image height
    FLOAT endX;        ///< (0-1), percentage of end X coordinate in terms of image width
    FLOAT endY;        ///< (0-1), percentage of end Y coordinate in terms of image height
} PDLibWindowInfo;

/// @brief Defines the main window type.
typedef enum
{
    PDLibRoiTypeCenter = 0x0,  ///<  Default type
    PDLibRoiTypeFace,          ///<  Face AF
    PDLibRoiTypeTouch,         ///<  Touch-AF
    PDLibRoiTypeTopLeft,       ///<  Peripheral TOP_LEFT ROI
    PDLibRoiTypeTopCenter,     ///<  Peripheral TOP_CENTER ROI
    PDLibRoiTypeTopRight,      ///<  Peripheral TOP_RIGHT ROI
    PDLibRoiTypeLeft,          ///<  Peripheral LEFT ROI
    PDLibRoiTypeRight,         ///<  Peripheral RIGHT ROI
    PDLibRoiTypeBottomLeft,    ///<  Peripheral BOTTOM_LEFT ROI
    PDLibRoiTypeBottomCenter,  ///<  Peripheral BOTTOM_CENTER ROI
    PDLibRoiTypeBottomRight,   ///<  Peripheral BOTTOM_RIGHT ROI
} PDLibRoiType;

/// @brief Defines window configuration in pdlib configuration
typedef struct
{
    PDLibRoiType      roiType;                ///< type of roi coordinates
    PDLibWindowInfo   fixedAFWindow;          ///< coordinates of ROI
    UINT              horizontalWindowCount;  ///<  number of windows along horizontal direction
    UINT              verticalWindowCount;    ///<  number of windows along vertical direction
} PDLibWindowConfig;

/// @brief Defines pd pixel location in one block
typedef struct
{
    INT                   x;      ///< pixel x coordinate
    INT                   y;      ///< pixel y coordinate
    PDLibPixelShieldInfo  type;   ///< pixel shield type
} PDLibPDPixelInfo;

/// @brief Defines sensor orientation
typedef enum
{
    PDLibOrientationDefault,       ///< default orientation
    PDLibOrientationMirror,        ///< mirrored orientation
    PDLibOrientationFlip,          ///< flipped orientation
    PDLibOrientationMirrorAndFlip, ///< flipped and mirrored orientation
    PDLibOrientationMax,           ///< Anchor to indicate the last item in the defines
} PDLibSensorOrientation;

/// @brief Defines camif dimensions
typedef struct
{
    UINT width;    ///< width of camif window
    UINT height;   ///< height of camif window
} PDLibDimensionInfo;

/// @brief Defines defocus range
typedef struct
{
    INT16 rangeNear;     ///< range near
    INT16 rangeFar;      ///< range far
} PDLibRangeInfo;

/// @brief Defines output data of pd library
typedef struct
{
    INT   defocus;                ///< defocus in DAC range
    UINT  confidence;             ///< confidence level
    FLOAT phaseDiff;              ///< phase difference
    PDLibRangeInfo defocusRange;  ///< defocus range
} PDLibDefocusInfo;

/// @brief Defines roi coordinates
typedef struct
{
    INT x;              ///< x coordinate
    INT y;              ///< y coordinate
    UINT width;         ///< width coordinate
    UINT height;        ///< height coordinate
} PDLibRectangleInfo;

/// @brief Defines the type of PD data.
typedef enum
{
    PDLibSensorInvalid,               ///< invalid information
    PDLibSensorType1,                 ///< sparse pd type 1
    PDLibSensorType2,                 ///< sparse pd type 2
    PDLibSensorType3,                 ///< sparse pd type 3
    PDLibSensorDualPD,                ///< dual PD Type 2
    PDLibSensorTypeMax,               ///< Anchor to indicate the last item in the defines
} PDLibSensorType;

// @brief Defines Sensor PD Stats Format for TYPE1
typedef enum
{
    PDLibSensorPDStatsCustom                = 0,
    PDLibSensorPDStatsRaw10Conf8BitPd10Bit  = 1,
    PDLibSensorPDStatsRaw10Conf11BitPd11Bit = 2,
    PDLibSensorPDStatsRaw10Conf11BitPd10Bit = 3
} PDLibSensorPDStatsFormat;

// @brief Defines the packing of pd data
typedef enum
{
    PDLibBufferFormatMipi8,        ///< mipi8
    PDLibBufferFormatMipi10,       ///< mipi10, [9:2] [9:2] [9:2] [9:2] [1:0][1:0][1:0][1:0]
    PDLibBufferFormatUnpacked16,   ///< raw 16 unpacked
    PDLibBufferFormatPacked10,     ///< raw 10 packed, Q10 format
} PDLibBufferFormat;

/// @brief Defines the pd data order
typedef enum
{
    PDLibPixelOrderDefault, ///< default order
    PDLibPixelOrderLR,      ///< left followed by right
    PDLibPixelOrderRL,      ///< right followed by left
} PDLibPixelOrderType;

/// @brief Defines the pd block pattern filled by sensor for type 2 and isp for type 3
typedef struct
{
    UINT                  pixelCount;                          ///< pd pixel number inside of a window
    PDLibPDPixelInfo      pixelCoordinate[PDLibMaxPixelCount]; ///< pixel 2D pos, left_pixel,right_pixel
    UINT32                horizontalPDOffset;                  ///< horizontal offsets
    UINT32                verticalPDOffset;                    ///< vertical offsets
    PDLibDimensionInfo    blockDimension;                      ///< pattern window width and height
} PDLibBlockPattern;

/// @brief Defines the lcr pd block pattern
typedef struct
{
    UINT                  lineCount;                          ///< number of lines of pd data in a block
    UINT                  linePosition[PDLibMaxBlockHeight];  ///< position of pd data in a block
    UINT32                horizontalPDOffset;                 ///< horizontal offsets
    UINT32                verticalPDOffset;                   ///< vertical offsets
    PDLibDimensionInfo    blockDimension;                     ///< pattern window width and height
} PDLibLcrBlockPattern;

/// @brief Defines the pd block pattern info
typedef struct
{
    UINT                  stride;         ///< stride information
    PDLibBufferFormat     bufferFormat;   ///< type of pd packing
    PDLibSensorType       sensorType;     ///< type of buffer
    PDLibBlockPattern     blockPattern;   ///< block pattern of pd data
} PDLibBlockPatternInfo;

/// @brief Defines native sensor pattern info
typedef struct
{
    PDLibBlockPattern blockPattern;           ///< defines block pattern
    UINT              horizontalBlockCount;   ///< number of PD blocks in x
    UINT              verticalBlockCount;     ///< number of PD blocks in y
} PDLibSensorNativePatternInfo;

/// @brief Defines native sensor pattern
typedef struct
{
    PDLibSensorNativePatternInfo    patternInfo;                ///< defines native sensor pattern
    PDLibRectangleInfo              cropRegion;                 ///< insensor cropped
    UINT                            originalImageWidth;         ///< orignal image width in pixel
    UINT                            originalImageHeight;        ///< orignal image height in pixel
    UINT                            currentImageWidth;          ///< current image width in pixel
    UINT                            currentImageHeight;         ///< current image height in pixel
    PDLibSensorOrientation          orientation;                ///< sensor orientation
    PDLibBufferFormat               bufferFormat;               ///< buffer data type
    FLOAT                           horizontalDownscaleFactor;  ///< horizontal downscale binning factor; 2 for 2PD IMX362
    FLOAT                           verticalDownscaleFactor;    ///< vertical downscale binning factor; 4 for 2PD IMX362
    FLOAT                           PDOffsetCorrection;         ///< pd offset correction
    FLOAT                           lcrPDOffset;                ///< lc pd offset
    PDLibLcrBlockPattern            lcrBlockPattern;            ///< lcr block pattern
    PDLibSensorBayerPattern         bayerPattern;               ///< bayer pattern streamed by sensor
} PDLibSensorNativeInfo;

/// @brief Defines lcr buffer data information
typedef struct
{
    PDLibLcrBlockPattern  isp1BlockPattern;       ///< left vfe camif data pattern
    PDLibLcrBlockPattern  isp2BlockPattern;       ///< right vfe camif data pattern
    UINT                  isp1BufferWidth;        ///< camif left buffer width
    UINT                  isp1BufferStride;       ///< camif left buffer stride
    UINT                  isp2BufferWidth;        ///< camif right buffer width
    UINT                  isp2BufferStride;       ///< camif right buffer stride
    UINT                  ispBufferHeight;        ///< camif buffer height
    UINT                  imageOverlap;           ///< pixel when images overlap for dual IFE
    UINT                  isp2ImageOffset;        ///< offset of right images
    PDLibBufferFormat     bufferFormat;           ///< buffer data type
} PDLibLcrBufferDataInfo;

/// @brief Defines pd data buffer information. or T3, camif data pattern. for T2, the pd stats pattern
typedef struct
{
    PDLibBlockPattern      isp1BlockPattern;            ///< isp1 camif data pattern, NULL in single VFE
    PDLibBlockPattern      isp2BlockPattern;            ///< isp2 camif data pattern, NULL in single VFE
    UINT                   isp1BufferWidth;             ///< subsampled image width in pixel
    UINT                   isp1BufferStride;            ///< subsampled image stride in byte
    UINT                   isp2BufferWidth;             ///< subsampled image width in pixel
    UINT                   isp2BufferStride;            ///< subsampled image stride in byte
    UINT                   ispBufferHeight;             ///< subsampled image height
    UINT                   imageOverlap;                ///< overlap in pixel for dual vfe in the original img prio subsample
    UINT                   isp2ImageOffset;             ///< right image offsets
    PDLibBufferFormat      bufferFormat;                ///< buffer data type
    PDLibSensorType        sensorType;                  ///< buffer status descriptor
    PDLibLcrBufferDataInfo lcrBufferData;               ///< lcr buffer data info
} PDLibDataBufferInfo;

/// @brief Defines  lcr raw type
typedef enum
{
    PDLibMIPIRaw,    ///< Raw type MIPI type
    PDLibIdealRaw,   ///< ideal raw type
    PDLibCamifRaw    ///< camif raw type
} PDLibLcrRawType;

/// @brief Defines  lcr sensor configuration
typedef struct
{
    INT                   enable;              ///< enable flag
    PDLibLcrRawType       rawType;             ///< lcr raw type
    PDLibLcrBlockPattern  lcrNativePattern;    ///< lcr native pattern
} PDLibLcrSensorConfigure;

/// @brief PDLib settings from camxsettings.xml
typedef struct
{
    BOOL                        isPDHWEnabled;              ///< Flag to indicate PD hardware enabled
    BOOL                        enablePDLibProfiling;       ///< Enable profiling
    UINT                        enablePDLibTestMode;        ///< Enable Test Mode
    UINT                        enablePDLibLog;             ///< Enable PDAF_LOGH Logging
    UINT                        enablePDLibDump;            ///< Enable PDAF_LOGH Dump
    BOOL                        disableLCR;                 ///< Disable LCR processing
    UINT32                      profile3A;                  ///< PDLIB API profilling
} PDLibSettingsInfo;

/// @brief Represents if required conditions been met for PDHW. This will be published by AF Node and ultimately will be used by
///        PDLib
typedef struct
{
    BOOL    isDualPDEnableConditionsMet;    ///< If the required conditions have been met for dual PD HW to be enabled.
                                            ///< It doesn't include tuning enable flag. PDLib will consider tuning flag.
    BOOL    isSparsePDEnableConditionsMet;  ///< If the required conditions have been met for sparse PD HW to be enabled.
                                            ///< It doesn't include tuning enable flag. PDLib will consider tuning flag.
    BOOL    isLCREnableConditionsMet;       ///< If the required conditions have been met for LC HW to be enabled.
                                            ///< It doesn't include tuning enable flag. PDLib will consider tuning flag.
} PDHWEnableConditions;

/// @brief Sensor crop info
struct StatsSensorModeCropInfo
{
    UINT    firstPixel;     ///< left crop
    UINT    firstLine;      ///< top crop
    UINT    lastPixel;      ///< outputWidth - right crop -1
    UINT    lastLine;       ///< outputHeight - bottom crop -1
};

/// @brief Defines input paratemers for initialization
typedef struct
{
    PDLibSensorNativeInfo       nativePatternInfo;          ///< pd pattern, location of pd pixels
    UINT                        blackLevel;                 ///< sensor black level
    UINT                        pixelDepth;                 ///< pixel depth
    UINT                        isHdrModeEnabled;           ///< is hdr mode enabled
    UINT                        PDPixelOverflowThreshold;   ///< pix threshold
    INT16                       macroDac;                   ///< macro dac value
    INT16                       infinityDac;                ///< infinity dac value
    FLOAT                       actuatorSensitivity;        ///< actuator sensitivity
    PDLibPixelOrderType         pixelOrderType;             ///< pixel order type
    VOID*                       pCalibrationParam;          ///< pointer to calibration parameters from sensor
    UINT32                      defocusBitShift;            ///< defocus bit shift
    INT32                       defocusConfidenceThreshold; ///< defocus confidence threshold
    PDLibSensorPDStatsFormat    sensorPDStatsFormat;        ///< sensor pd stats format for type1
    PDLibDataBufferInfo         bufferDataInfo;             ///< buffer data info
    StatsCameraInfo             cameraInfo;                 ///< Holds camera information
    PDLibSettingsInfo           settingsInfo;               ///< Settings from camxsettings.xml
    PDHWEnableConditions        pdHWEnableConditions;       ///< PDAF HW Required conditions have been met. Tuning is not
                                                            ///< included. PDLib will include tuning enable/disable flag
    VOID*                       pChromatixPDLib;            ///< A pointer to PDLib tuning. The value for this field is not
                                                            ///< mandatory. This is provided for the client whom doesn't have
                                                            ///< access to QTI Tuning manager
    StatsSensorModeCropInfo     sensorCropInfo;             ///< Crop info for this sensor mode
} PDLibInitParam;

/// @brief Defines input structure for defocus  processing
typedef struct
{
    UINT16*             pPDBuffer;                  ///< pointer to pd buffer, 16bit per pixel
    INT                 PDBufferFd;                 ///< file descriptor used by PDLib to map the buffer
    SIZE_T              PDBufferSize;               ///< PD Buffer size
    UINT16*             pRawBuffer;                 ///< pointer to the pixel buffer
    PDLibWindowConfig   windowConfig;               ///< support fixed window configuration
    FLOAT               imageAnalogGain;            ///< image sensor gain
    FLOAT               currentFps;                 ///< real-time FPS
    FLOAT               integrationTime;            ///< exposure time in nanoseconds
    INT16               lensPosition;               ///< current logical lens position
    StatsCameraInfo     cameraInfo;                 ///< Holds camera information
    UINT64*             pPDHWBuffer;                ///< pointer to pd buffer, 16bit per pixel
    INT                 PDHWBufferFd;               ///< file descriptor used by PDLib to map the buffer
    SIZE_T              PDHWBufferSize;             ///< PD Buffer size
    BOOL                isPDHWEnable;               ///< flag to indicate if PD HW is enabled
    UINT16*             pLCRHWBuffer;               ///< pointer to lcr buffer, 16bit per pixel
} PDLibParam;

/// @brief Defines structure for version
typedef struct
{
    UINT        majorVersion;    ///< major version of the library
    UINT        minorVersion;    ///< minor version of the library
    UINT        tinyVersion;     ///< sub minor version of the library
} PDLibVersion;

/// @brief Defines depth map output from PDLib
typedef struct
{
    FLOAT   PDMap[PDLibMaxMapGridCount];                ///< PD value map
    UINT    ConfidenceMap[PDLibMaxMapGridCount];        ///< Confidence map
    INT16   DefocusRangeNearMap[PDLibMaxMapGridCount];  ///< Defocus Range Near map
    INT16   DefocusRangeFarMap[PDLibMaxMapGridCount];   ///< Defocus Range Far map
    INT     DefocusMap[PDLibMaxMapGridCount];           ///< Defocus map
    UINT32  horiNumOfRegion;                            ///< number of horizontal grid
    UINT32  vertNumOfRegion;                            ///< number of vertical grid
    UINT32  widthOfRegion;                              ///< width of each grid cell
    UINT32  heightOfRegion;                             ///< height of each grid cell
    UINT32  horiRegionOffset;                           ///< horizontal offset for grid
    UINT32  vertRegionOffset;                           ///< vertical offset for grid
} PDLibDepthMapInfo;


/// @brief Defines output structure for defocus
typedef struct
{
    PDLibDefocusInfo       defocus[PDLibMaxWindowCount];                 ///< defocus output for each roi
    UINT16                 isPeripheralValid;                            ///< is peripheral data valid
    PDLibDefocusInfo       resultPeripheral[PDLibPeripheralWindowMax];   ///< type of peripheral processing
    PDLibProcessingResult  processingResult;                             ///< pdlib processing results
    UINT                   horizontalWindowCount;                        ///< number of horizontal window count
    UINT                   verticalWindowCount;                          ///< number of vertical window count
    UINT                   windowNumber;                                 ///< number of grids
    PDLibDepthMapInfo      depthMapInfo;                                 ///< depth map output
} PDLibOutputData;

/// @brief Identifies the type of create parameter
typedef enum
{
    PDLibCreateParamTypeInvalid                 = -1,           ///< Type Invalid
    PDLibCreateParamTypeCameraOpenIndicator,                    ///< Actual Camera Open state
    PDLibCreateParamTypeLibName,                                ///< PDLib name
    PDLibCreateParamTypeCameraInfo,                             ///< Camera information
                                                                ///  Payload: StatsCameraInfo
    PDLibCreateParamTypeCount,                                  ///< Create Param Type Count
    PDLibCreateParamTypeMax                     = 0x7FFFFFFF    ///< Max Create Param Type
} PDLibCreateParamType;

/// @brief Represents an PDLib input parameter
typedef struct
{
    PDLibCreateParamType    createParamType; ///< Type of parameter passed
    VOID*                   pParam;          ///< Payload of the particular parameter type
    UINT32                  sizeOfParam;     ///< Size of the payload.
} PDLibCreateParam;

/// @brief Represents PDLib creation parameters
typedef struct
{
    PDLibCreateParam*   pParamList;    ///< Pointer to PDLib create-parameter
    UINT32              paramCount;    ///< Number of input create-parameters
} PDLibCreateParamList;

/// @brief Identifies the type of destroy parameter
typedef enum
{
    PDLibDestroyParamTypeInvalid                = -1,           ///< Type Invalid
    PDLibDestroyParamTypeCameraCloseIndicator,                  ///< Camera Close Indicator
                                                                ///< UINT 0 - Camera Close 1 Camera Open
    PDLibDestroyParamTypeCameraInfo,                            ///< Camera information
                                                                ///  Payload: StatsCameraInfo
    PDLibDestroyParamTypeCount,                                 ///< Destroy Param Type Count
    PDLibDestroyParamTypeMax                    = 0x7FFFFFFF    ///< Max Destroy Param Type
} PDLibDestroyParamType;

/// @brief Represents an PDLib destroy parameter
typedef struct
{
    PDLibDestroyParamType   destroyParamType;       ///< Type of parameter passed
    VOID*                   pParam;                 ///< Payload of the particular parameter type
    UINT32                  sizeOfParam;            ///< Size of the payload.
} PDLibDestroyParam;

/// @brief Represents PDLib destroy parameters
typedef struct
{
    PDLibDestroyParam*  pParamList;    ///< Pointer to PDLib destroy-parameter
    UINT32              paramCount;    ///< Number of input destroy-parameters
} PDLibDestroyParamList;

/// @brief Identifies the type of output data selected
typedef enum
{
    PDHwOutputSelectInvalid = -1,       ///< Type Invalid
    PDHwOutputSelectIIRprocessedROI,    ///< Select raw PD data from IIR
    PDHwOutputSelectSADstats,           ///< Select SAD stats
} PDHwOutputSelect;

/// @brief Identifies the type of Dual PD path
typedef enum
{
    PDHwPathSelectInvalid = -1, ///< Type Invalid
    PDHwPathSelectHW,           ///< PD HW output
    PDHwPathSelectCAMIF,        ///< CAMIF extracted image
} PDHwPathSelect;

/// @brief Represents Dual PD crop information
typedef struct
{
    BOOL   enablePDCrop;    ///< Indicates if crop needs to be enabled
    UINT32 firstPixelCrop;  ///< First pixel for 2PD crop
    UINT32 lastPixelCrop;   ///< Last pixel for 2PD crop
    UINT32 firstLineCrop;   ///<First line for 2PD crop
    UINT32 lastLineCrop;    ///< Last line for 2PD crop
} PDHwCrop;

/// @brief Represents PD IIR filter configuration
typedef struct
{
    BOOL  IIREnable;    ///< Enable IIR enable
    FLOAT init0;        ///< Init0
    FLOAT init1;        ///< Init1
    FLOAT a0;           ///< IIR filter coefficient a0 Q15
    FLOAT a1;           ///< IIR filter coefficient a1 Q15
    FLOAT b0;           ///< IIR filter coefficient b0 Q15
    FLOAT b1;           ///< IIR filter coefficient b1 Q15
    FLOAT b2;           ///< IIR filter coefficient b2 Q15
} PDHwIIRConfig;

/// @brief Represents PD SAD configuration
typedef struct
{
    BOOL                sadEnable;          ///< Indicates whether SAD is enabled
    PDHwOutputSelect    sadSelect;          ///< 1: SAD stats, 0, IIR processed ROI
    UINT32              horizontalOffset;   ///< Horizontal offset of the ROI grid from the left crop window
    UINT32              verticalOffset;     ///< Vertical  offset of the ROI grid from the left crop window
    UINT32              regionWidth;        ///< Width of each region in the ROI grid (pixels)
    UINT32              regionHeight;       ///< Height of each region in the ROI grid (pixels)
    UINT32              horizontalNumber;   ///< Number of columns of regions in the ROI grid
    UINT32              verticalNumber;     ///< Number of rows of regions in the ROI grid
    UINT32              sadShift;           ///< Shift config for SAD to avoid overflow
    UINT32              config0Phase;       ///< indicates whether SAD needs to be computed. Phase -1 to Phase -32
    UINT32              config1Phase;       ///< +31 to 0 aligning from 32nd bit (MSB) to 1st bit (LSB)
} PDHwSADConfig;

/// @brief Identifies the HDR Pixel type
typedef enum
{
    PDHwHDRPixelInvalid = -1,   ///< Type Invalid
    PDHwHDRPixelLong,           ///< HDR Long pixel
    PDHwHDRPixelShort,          ///< HDR short pixel
} PDHwHDRPixelType;

/// @brief Identifies the HDR Mode type
typedef enum
{
    PDHwHDRModeInvalid = -1,  ///< Type Invalid
    PDHwHDRModeAuto,          ///< Auto mode
    PDHwHDRModeLong,          ///< Use Long pixel
    PDHwHDRModeShort,         ///< Use Short Pixel
} PDHwHDRModeType;

/// @brief Represents PD HDR configuration
typedef struct
{
    BOOL                hdrEnable;          ///< 0 - HDR disable, 1 - HDR enable
    PDHwHDRPixelType    hdrFirstPixel;      ///< 0 - T1, 1 - T2
    PDHwHDRModeType     hdrModeSel;         ///< 1 - Use Long pixels, 0 - Use Short Pixels
    FLOAT               hdrThreshhold;      ///< Threshold to select long/short()
    FLOAT               hdrExposureRatio;   ///< HDR exposure ratio, Q8
} PDHwHDRConfig;

/// @brief Represents PD BLS configuration
typedef struct
{
    UINT32  leftPixelBLSCorrection;     ///< BLS correction for left pixel offset
    UINT32  rightPixelBLSCorrection;    ///< BLS correction for right pixel offset
} PDHwBLSConfig;

/// @brief Represents PD BIN/SKIP configuration
typedef struct
{
    BOOL    enableSkipBinning;              ///< 0 - disable binning, 1 - enable binning
    UINT32  verticalBinningLineCount;       ///< Number of lines for vertical binning
    UINT32  verticalDecimateCount;          ///< Number of decimate lines
    UINT32  horizontalBinningPixelCount;    ///< Number of pixel for horizontal binning
    UINT32  horizontalBinningSkip;          ///< Horizontal bin shift
    UINT32  verticalBinningSkip;            ///< Vertical bin shift
} PDHwBinConfig;

/// @brief Represents Gain Map configuration
typedef struct
{
    BOOL                gainMapEnable;                  ///< 0 - disable, 1 - enable
    UINT32              numberHorizontalGrids;          ///< Number of horizontal grids
    UINT32              numberVerticalGrids;            ///< Number of vertical grids
    UINT32              horizontalPhaseInit;            ///< Horizontal Phase Init
    UINT32              verticalPhaseInit;              ///< Vertical Phase Init
    UINT32              horizontalPhaseStep;            ///< Horizontal Phase Step
    UINT32              verticalPhaseStep;              ///< Vertical Phase Step
    UINT16              leftLUT[PDLibGainMapLUTSize];   ///< Left gain map LUT
    UINT16              rightLUT[PDLibGainMapLUTSize];  ///< Right gain map LUT
} PDHwGainMapConfig;

/// @brief Represents crop coordinate
typedef struct
{
    UINT32 firstLine;   ///< Line buffer containing the top-left pixel of the region to be cropped.
    UINT32 lastLine;    ///< Line buffer containing the bottom-right pixel of the region to be cropped.
    UINT32 firstPixel;  ///< Location of the top-left pixel of the region to be cropped.
    UINT32 lastPixel;   ///< Location of the bottom-right pixel of the region to be cropped.
} CropCoordinate;

/// @brief Represents LCR HW configuration
typedef struct
{
    BOOL            enable;                                         ///< Indicates if LCR HW is enabled
    CropCoordinate  crop;                                           ///< Crop coordinate of the region to be cropped.
    UINT32          blockHeight;                                    ///< Block Height of repeating pattern
                                                                    ///< (maximum size 64X64 (N = N+1))
    UINT32          componentMask;                                  ///< A bit mask to include or exclude a color channel for
                                                                    ///< binning. (0, 1) to (exclude, include) the corresponding
                                                                    ///< color channel. Each bit indicate a color channel which
                                                                    ///< is on the sensor bayer pattern
    INT32           componentShift[PDLibLCRColorComponentCount];    ///< Indicates the number of bits by which
                                                                    ///< each color components needs to be shifted.
    UINT32          lineMask[PDLibLCRMaskCount];                    ///< Each bit indicates intercept or don't intercept for
                                                                    ///< binning.. (0, 1) means (don't intercept, intercept)
                                                                    ///< lineMask[0]: Bit 0 (LSB) to Bit 31 (MSB) represents
                                                                    ///< the mask for line 0 to line 31
                                                                    ///< lineMask[1]: Bit 0 (LSB) to Bit 31 (MSB) represents
                                                                    ///< the mask for line 32 to line 63
    UINT32          flushMask[PDLibLCRMaskCount];                   ///< Each bit indicates flush or don't flush a line buffer
                                                                    ///< (0, 1) means (don't flush, flush)
                                                                    ///< flushMask[0]: Bit 0 (LSB) to Bit 31 (MSB) represents
                                                                    ///< the mask for line 0 to line 31
                                                                    ///< flushMask[1]: Bit 0 (LSB) to Bit 31 (MSB) represents
                                                                    ///< the mask for line 32 to line 63
} PDHwLCRConfig;

/// @brief Identifies PD Hw Mode
typedef enum
{
    PDHwModeSelectInvalid = -1, ///< Type Invalid
    PDHwModeSelectDualPD,       ///< DualPD
    PDHwModeSelectSparsePD,     ///< SparsePD
} PDHwModeSelect;

/// @brief Represents Line Extractor configuration
typedef struct
{
    UINT16  globalOffsetLines;                      ///< Number of lines for which null pixels are sent before
                                                    ///< the actual transmission of PD pixels in T2 mode.
    UINT16  blockHeight;                            ///< Height of the PD pixels in a block. Maximum = 128
    UINT16  pdPixelWidth;                           ///< Maximum number of PD pixel in one row of input image.
                                                    ///< assuming a maximum packing density of 25%.
    UINT16  verticalBlockCount;                     ///< Number of PD pixel blocks in the vertical direction
    UINT16  blockPDRowCount;                        ///< Number of rows containing pd pixel in a block
    UINT16  horizontalOffset[PDLibMaxPDPixelRows];  ///< Horizontal offset per line in a block
    UINT16  verticalOffset[PDLibMaxPDPixelRows];    ///< Vertical offset per line in a block
    BOOL    halfLine[PDLibMaxPDPixelRows];          ///< Make half line full line by duplicating each pixel.
                                                    ///< (0, 1) means (    extract line as is, duplicate)
} PDHwLineExtractorConfig;

/// @brief Represents Pixel Separator configuration
typedef struct
{
    UINT16  rowPerBlock;                            ///< Number of PD pixel rows in each block. (A maximum of
                                                    ///< 16 lines is assumed for each sparse PD block).
    UINT16  pixelsPerBlock;                         ///< Pixels per block for each buffer
    UINT16  outputHeight;                           ///< Height of block output
    UINT16  outputWidth;                            ///< Width of block output
    UINT16  pdLMap[PDLibMaxLeftOrRightPDPixels];    ///< Pixel extracton array indices for L pixels. [4][32]
    UINT16  pdRMap[PDLibMaxLeftOrRightPDPixels];    ///< Pixel extracton array indices for R pixels.[4][32]
} PDHwPixelSeparatorConfig;

typedef struct
{
    INT16       x;      ///< x coordinate for resampler input
    INT16       y;      ///< y coordinate for resampler input
} PDHWResamplerPixel;

typedef struct
{
    FLOAT   k0;         ///< k0 coefficient for operator 6sQ3
    FLOAT   k1;         ///< k1 coefficient for operator 6sQ3
    FLOAT   k2;         ///< k2 coefficient for operator 6sQ3
    FLOAT   k3;         ///< k3 coefficient for operator 6sQ3
} PDHWResamplerCoeff;

typedef struct
{
    PDHWResamplerPixel  pixelLUT[PDLibResamplerInputCount];    ///< Input pixel coordinates for each instruction
    PDHWResamplerCoeff  coeffLUT[PDLibResamplerInputCount];    ///< Operator coefficients for each instruction
} PDHWResamplerInstruction;


/// @brief Represents Resampler configuration
typedef struct
{
    BOOL                        enable;                                     ///< Enable/Disable resampler
    PDHWResamplerInstruction    instruction[PDLibResamplerInstructionSize]; ///< Instructions for resampler
    UINT16                      inputHeight;                                ///< Height of block input of resampler after
                                                                            ///< adding padding pixels if required.
    UINT16                      inputWidth;                                 ///< Width of block input of resampler after
                                                                            ///< adding padding pixels if required
    UINT16                      outputHeight;                               ///< Height of block output of resampler
    UINT16                      outputWidth;                                ///< Number of pixels in each row of resampler output block
} PDHwResamplerConfig;

/// @brief Represents PD FIR filter configuration
typedef struct
{
    BOOL    enable;             ///< Enable/Disable FIR
    FLOAT   a0;                 ///< FIR filter coefficient a0
    FLOAT   a1;                 ///< FIR filter coefficient a1
    FLOAT   a2;                 ///< FIR filter coefficient a2
    FLOAT   a3;                 ///< FIR filter coefficient a3
    FLOAT   a4;                 ///< FIR filter coefficient a4
    INT16   shift;              ///< FIR shifter
} PDHwFIRConfig;

/// @brief Represents CSID Pixel Extractor configuration
typedef struct
{
    BOOL        enable;                             ///< Enable/Disable CSID Pixel Extractor
    PDHwCrop    cropConfig;                         ///< Pixel Extractor Crop
    UINT16      blockHeight;                        ///< Block height of the native sensor repeating pattern. Max 64
    UINT16      blockWidth;                         ///< Block width of the native sensor repeating pattern. Max 64
    BOOL        pdPixelMap[PDLibMaxPixelsInBlock];  ///< Indicate if a pixel in a block is PD pixel or not. Max [64][64]
} PDHwCSIDPixelExtractorConfig;

/// @brief Represents Sparse HW PD configuration
typedef struct
{
    PDHwLineExtractorConfig         LEConfig;                       ///< Line Extractor config
    PDHwPixelSeparatorConfig        PSConfig;                       ///< Pixel Separator config
    PDHwResamplerConfig             RSConfig;                       ///< Resampler config
    PDHwFIRConfig                   FIRConfig;                      ///< FIR config
    PDHwCSIDPixelExtractorConfig    PEConfig;                       ///< Pixel Extractor config
}  SparsePDHwConfig;

/// @brief Represents base HW PD configuration
typedef struct
{
    BOOL                            enablePDHw;                     ///< Indicates if dual/sparse PD HW is enabled or not.
                                                                    ///< If enablePDHw is false, it means PD HW will be disabled.
    PDHwPathSelect                  pathSelect;                     ///< 0: PD HW output, 1: CAMIF extracted image
    BOOL                            enableEarlyInterrupt;           ///< Indicates if early Interrupt needs to be enabled
    UINT32                          earlyInterruptTransactionCount; ///< Indicates if early Interrupt needs to be enabled
    UINT32                          firstPixelSelect;               ///< 0: Right Pixel, 1: Left Pixel -- TBD(Sensor)
    PDHwCrop                        cropConfig;                     ///< Dual PD crop
    PDHwGainMapConfig               gainMapConfig;                  ///< Gain Map config
    PDHwBinConfig                   binConfig;                      ///< Bin/skip config
    PDHwBLSConfig                   BLSConfig;                      ///< BLS config
    PDHwHDRConfig                   HDRConfig;                      ///< HDR config
    PDHwIIRConfig                   IIRConfig;                      ///< IIR config
    PDHwSADConfig                   SADConfig;                      ///< SAD config
    PDHwLCRConfig                   LCRConfig;                      ///< LCR Config
    PDHwModeSelect                  modeSelect;                     ///< 0: DualPDMode, 1: SparsePDMode
    SparsePDHwConfig                sparseConfig;                   ///< sparse PD HW config. The dual PD configuration is
                                                                    ///< being shared between dual and sparse PD.
} PDHwConfig;

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHIPDLIBCOMMON_H
