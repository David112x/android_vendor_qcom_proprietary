////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chi.h
/// @brief CamX Hardware Interface (CHI) definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHI_H
#define CHI_H

#include "camxcdktypes.h"
#include "chicommon.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#pragma pack(push, 8)

typedef UINT  VariantGroup;      // A prunable variant group identifier
typedef UINT  VariantType;       // A name to a prunable variant group

typedef VOID* IMAGEBUFFERHANDLE; // handle to Image Buffer

/// @brief A tuple like struct associating a VariantGroup and VariantType used in Pruning comparisions
struct PruneVariant {
    VariantGroup group; ///< The VariantGroup
    VariantType  type;  ///< The VariantType of the above VariantGroup
};

/// @brief A list of PruneVariants
struct PruneSettings {
    UINT                numSettings; ///< The length of pVariants
    const PruneVariant* pVariants;   ///< A list of PruneVariants
};

static const UINT32 MaxNumImageSensors              = 16;
static const UINT32 BufferQueueDepth                = 16;
static const UINT32 PrivBufferQueueDepth            = 64;  ///< FD+RDI,(2*MFSR request + 1 B2Y)*(2 active camera)*2
static const UINT32 MaximumChiPipelineTagsPublished = 300; ///< Maximum number of tags published by the pipeline
static const UINT32 MaxMetadataPerRequest           = 2;   ///< Maximum number of metadata buffers that can be sent
                                                           ///  per request
static const UINT32 MaxConfigSettings               = 64;  ///< Maximum number of configuration settings
static const UINT32 MaxOutputStreamsPerRequest      = 15;  ///< Maximum number of output streams per request
static const UINT32 MaxPerBufferMetadata            = 10;  ///< Maximum number of per-buffer metadata entries
static const UINT32 MaxSensorStreamConfigs          = 8;   ///< Maximum number of Sensor stream configs

// DebugData & TuningMetadata partitions
static const UINT DebugDataPartitionsBPS = 2; // Keep for backwards compatibilty (MFNR use-case)
static const UINT DebugDataPartitionsIPE = 3; // Also keep for backwards compatibility
                                              // This does NOT consider scaler data in MFNR use-case

/// @brief Buffer Heap
const UINT32 BufferHeapSystem         = 0;
const UINT32 BufferHeapIon            = 1;
const UINT32 BufferHeapDSP            = 2;
const UINT32 BufferHeapEGL            = 3;

/// @brief Buffer mem flags
const UINT32 BufferMemFlagHw          = (1 << 0);
const UINT32 BufferMemFlagProtected   = (1 << 1);
const UINT32 BufferMemFlagCache       = (1 << 2);
const UINT32 BufferMemFlagLockable    = (1 << 3);

/// @brief Link flags
const UINT32 LinkFlagDisableLateBinding   = (1 << 0);   ///< Disables image buffer manager late binding on link
                                                        ///  When set, ImageBuffer Manager gets buffers from Dedicated group,
                                                        ///  i.e Buffers at this port will not be shared with any other
                                                        ///  ImageBuffer Managers
const UINT32 LinkFlagDisableSelfShrinking = (1 << 1);   ///< Disables self shrinking of buffers allocated at this output port.
                                                        ///   When set, ImageBuffer Manager gets buffers from Dedicated group,
                                                        ///  i.e Buffers at this port will not be shared with any other
                                                        ///  ImageBuffer Managers
const UINT32 LinkFlagSinkInplaceBuffer    = (1 << 2);   ///< Indicates that the buffer at this output port of the inplace node
                                                        ///  is a sink buffer.
                                                        ///  When set, properties of this sink port is provided to the parent node
                                                        ///  to create a non-Sink Hal buffer
typedef VOID* CHIHANDLE;
typedef VOID* CHIPIPELINEDESCRIPTOR;
typedef VOID* CHIPIPELINEHANDLE;
typedef VOID* CHIMETAHANDLE;
typedef VOID* CHIMETADATAITERATOR;
typedef VOID* CHIMETAPRIVATEDATA;
typedef VOID* CHITARGETBUFFERINFOHANDLE;
typedef VOID* CHISTREAMHANDLE;
typedef VOID* CHISESSIONHANDLE;



typedef UINT32 NodeProperty;
static const NodeProperty NodePropertyCustomLib                  = 1;     ///< Custom lib
static const NodeProperty NodePropertyProfileId                  = 2;     ///< IPE/BPS Instance Property Profile
static const NodeProperty NodePropertyStabilizationType          = 3;     ///< IPE Instance Property Stabilization Type
static const NodeProperty NodePropertyProcessingType             = 4;     ///< IPE Instance Property Processing Type
static const NodeProperty NodePropertyIPEDownscale               = 5;     ///< IPE Instance IPE downscale
static const NodeProperty NodePropertyStatsSkipPattern           = 6;
static const NodeProperty NodePropertyIFECSIDHeight              = 7;
static const NodeProperty NodePropertyIFECSIDWidth               = 8;
static const NodeProperty NodePropertyIFECSIDLeft                = 9;
static const NodeProperty NodePropertyIFECSIDTop                 = 10;
static const NodeProperty NodePropertyNodeClass                  = 11;    ///< Node class -
                                                                          ///  Bypassable node or Default Node or inplace node
static const NodeProperty NodePropertyGPUCapsMaskType            = 12;    ///< GPU Instance Capabilities Mask Type
static const NodeProperty NodePropertyIPEDownscaleWidth          = 13;    ///< IPE Instance IPE downscale width
static const NodeProperty NodePropertyIPEDownscaleHeight         = 14;    ///< IPE Instance IPE downscale height
static const NodeProperty NodePropertyEnbaleIPECHICropDependency = 15;    ///< Indicates if we should wait on crop info in IPE
static const NodeProperty NodePropertyEnableFOVC                 = 16;    ///< FOVC enabled
static const NodeProperty NodePropertyStitchMaxJpegSize          = 17;    ///< Max Jpegsize for Dual Cam Stitched frame
static const NodeProperty NodePropertyForceSingleIFEOn           = 18;    ///< Flag to force IFE to single IFE
static const NodeProperty NodePropertySkipUpdatingBufferProperty = 19;    ///< Flag to skip creating buffer manager

static const NodeProperty NodePropertyVendorStart       = 1023;

/// @brief Debug-data type - Available debug data types
enum class DebugDataType
{
    AEC,        ///< AEC debug data type
    AWB,        ///< AWB debug data type
    AF,         ///< AF debug data type
    IFETuning,  ///< IFE Tuning data dump type
    BPSTuning,  ///< BPS Tuning data dump type
    IPETuning,  ///< IPE Tuning data dump type
    AllTypes,   ///< List all debug data types
};

/// @brief Structure describing the Debug Data
typedef struct
{
    SIZE_T  size;   ///< Size in bytes of the debug data buffer
    VOID*   pData;  ///< Pointer to the debug data buffer
} DebugData;

/// @brief Stream config mode
typedef enum StreamConfigMode
{
    StreamConfigModeNormal                  = 0x0000,   ///< Normal stream configuration operation mode
    StreamConfigModeConstrainedHighSpeed    = 0x0001,   ///< Special constrained high speed operation mode for devices that can
                                                        ///  not support high speed output in NORMAL mode
    StreamConfigModeVendorStart             = 0x8000,   ///< First value for vendor-defined stream configuration modes
    StreamConfigModeVendorEnd               = 0xEFFF,   ///< End value for vendor-defined stream configuration modes
    StreamConfigModeQTIStart                = 0xF000,   ///< First value for QTI-defined stream configuration modes
    StreamConfigModeQTITorchWidget          = 0xF001,   ///< Operation mode for Torch Widget.
    StreamConfigModeVideoHdr                = 0xF002,   ///< Video HDR On
    StreamConfigModeQTIEISRealTime          = 0xF004,   ///< Operation mode for Real-Time EIS recording usecase.
    StreamConfigModeQTIEISLookAhead         = 0xF008,   ///< Operation mode for Look Ahead EIS recording usecase
    StreamConfigModeQTIFOVC                 = 0xF010,   ///< Field of View compensation in recording usecase.
    StreamConfigModeQTIFAEC                 = 0xF020,   ///< Fast AEC mode
    StreamConfigModeFastShutter             = 0xF040,   ///< FastShutter mode
    StreamConfigModeSuperSlowMotionFRC      = 0xF080,   ///< Super Slow Motion with FRC interpolation usecase
    StreamConfigModeSensorMode              = 0xF0000,  ///< Sensor Mode selection for >=60fps
                                                        ///  require 4 bits for sensor mode index
} STREAMCONFIGMODE;

/// @brief SOC (Chipset ID)
typedef enum
{
    CHISocIdInvalid  = 0,     ///< Invalid SOC Id
    CHISocIdSDM845   = 321,   ///< SDM845 SOC Id
    CHISocIdSDM670   = 336,   ///< SDM670 SOC Id
    CHISocIdSDM855   = 339,   ///< SDM855 SOC Id
    CHISocIdSDM855P  = 361,   ///< SDM855P SOC Id
    CHISocIdQCS605   = 347,   ///< QCS605 SOC Id
    CHISocIdSM6150   = 355,   ///< SM6150 SOC Id
    CHISocIdSDM865   = 356,   ///< SDM865 SOC Id
    CHISocIdSM7150   = 365,   ///< SM7150 SOC Id
    CHISocIdSM7150P  = 366,   ///< SM7150P SOC Id
    CHISocIdSDM710   = 360,   ///< SDM710 SOC Id
    CHISocIdSXR1120  = 370,   ///< SXR1120 SOC Id
    CHISocIdSXR1130  = 371,   ///< SXR1130 SOC Id
    CHISocIdSDM712   = 393,   ///< SDM712 SOC Id
    CHISocIdSM7250   = 400,   ///< SM7250 SOC Id
    CHISocIdSM6250   = 407,   ///< SM6250 SOC Id
    CHISocIdSM4250   = 417,   ///< SM6250 SOC Id
    CHISocIdSM6350   = 434,   ///< SM6350 SOC Id
    CHISocIdSM7225   = 459,   ///< SM6350 AB SOC Id
} CHISocId;


/// @brief HDR mode for the stream
typedef enum StreamHDRMode
{
    HDRModeNone = 0,   ///< None HDR mode
    HDRModeHLG,               ///< HLG mode
    HDRModeHDR10,             ///< HDR 10 bit mode
    HDRModeHDR10Plus,         ///< HDR10+ mode
    HDRModePQ,                ///< PQ mode
    HDRModeMax                ///< Invalid Max value
} STREAMHDRMODE;

/// @brief select IHDR usecase
typedef enum SelectInSensorHDR3ExpUsecase
{
    InSensorHDR3ExpNone = 0,                        ///< None IHDR mode
    InSensorHDR3ExpPreview,                         ///< IHDR preview usecase
    InSensorHDR3ExpSeamlessSnapshot,                ///< Seamless IHDR snapshot
    InSensorHDR3ExpForceSeamlessSnapshot = 0x10,    ///< Force enable seamless IHDR snapshot for debug usage
} SELECTINSENSORHDRU3EXPSECASE;

typedef enum MultiCameraHWSyncType
{
    MultiCameraHWSyncDisabled   = 0,                ///< disable HW sync for all usecase
    MultiCameraHWSyncRTB        = 1,                ///< enable HW sync for RTB
    MultiCameraHWSyncSAT        = 2,                ///< enable HW sync for SAT
    MultiCameraHWSyncVR         = 4,                ///< enable HW sync for VR
} MULTICAMERAHWSYNCTYPE;

/// @brief camera facing values
typedef enum ChiDirection
{
    FacingBack              = 0,                ///< device facing back
    FacingFront             = 1,                ///< device facing front
    External                = 2                 ///< external devices
} CHIDIRECTION;

/// @brief camera sensor role/type
typedef enum ChiSensorPositionType
{
    NONE                    = 0,                /// not part of multicamera
    REAR                    = 1,                /// Rear main camera
    FRONT                   = 2,                /// Front main camera
    REAR_AUX                = 3,                /// Rear Aux Camera
    FRONT_AUX               = 4,                /// Front aux camera
} CHISENSORPOSITIONTYPE;

/// @brief Buffer Format
typedef enum ChiBufferFormat
{
    ChiFormatJpeg            = 0,       ///< JPEG format.
    ChiFormatY8              = 1,       ///< Luma only, 8 bits per pixel.
    ChiFormatY16             = 2,       ///< Luma only, 16 bits per pixel.
    ChiFormatYUV420NV12      = 3,       ///< YUV 420 format as described by the NV12 fourcc.
    ChiFormatYUV420NV21      = 4,       ///< YUV 420 format as described by the NV21 fourcc.
    ChiFormatYUV420NV16      = 5,       ///< YUV 422 format as described by the NV16 fourcc
    ChiFormatBlob            = 6,       ///< Any non image data
    ChiFormatRawYUV8BIT      = 7,       ///< Packed YUV/YVU raw format. 16 bpp: 8 bits Y and 8 bits UV. U and V are interleaved
                                        ///  as YUYV or YVYV.
    ChiFormatRawPrivate      = 8,       ///< Private RAW formats where data is packed into 64bit word.
                                        ///  8BPP:  64-bit word contains 8 pixels p0-p7, p0 is stored at LSB.
                                        ///  10BPP: 64-bit word contains 6 pixels p0-p5, most significant 4 bits are set to 0.
                                        ///         P0 is stored at LSB.
                                        ///  12BPP: 64-bit word contains 5 pixels p0-p4, most significant 4 bits are set to 0.
                                        ///         P0 is stored at LSB.
                                        ///  14BPP: 64-bit word contains 4 pixels p0-p3, most significant 8 bits are set to 0.
                                        ///         P0 is stored at LSB.
    ChiFormatRawMIPI         = 9,       ///< MIPI RAW formats based on MIPI CSI-2 specification.
                                        ///  8BPP: Each pixel occupies one bytes, starting at LSB.
                                        ///  Output width of image has no restrictions.
                                        ///  10BPP: 4 pixels are held in every 5 bytes.
                                        ///         The output width of image must be a multiple of 4 pixels.
                                        ///  12BPP: 2 pixels are held in every 3 bytes.
                                        ///         The output width of image must be a multiple of 2 pixels.
                                        ///  14BPP: 4 pixels are held in every 7 bytes.
                                        ///         The output width of image must be a multiple of 4 pixels.
    ChiFormatRawPlain16      = 10,      ///< Plain16 RAW format. Single pixel is packed into two bytes, little endian format.
                                        ///  Not all bits may be used as RAW data is generally 8, 10, or 12 bits per pixel.
                                        ///  Lower order bits are filled first.
    ChiFormatRawMeta8BIT     = 11,      ///< Generic 8-bit raw meta data for internal camera usage.
    ChiFormatUBWCTP10        = 12,      ///< UBWC TP10 format (as per UBWC2.0 design specification)
    ChiFormatUBWCNV12        = 13,      ///< UBWC NV12 format (as per UBWC2.0 design specification)
    ChiFormatUBWCNV124R      = 14,      ///< UBWC NV12-4R format (as per UBWC2.0 design specification)
    ChiFormatYUV420NV12TP10  = 15,      ///< YUV 420 format 10bits per comp tight packed format.
    ChiFormatYUV420NV21TP10  = 16,      ///< YUV 420 format 10bits per comp tight packed format.
    ChiFormatYUV422NV16TP10  = 17,      ///< YUV 422 format 10bits per comp tight packed format.
    ChiFormatPD10            = 18,      ///< PD10 format
    ChiFormatRawMIPI8        = 19,      ///< 8BPP: Each pixel occupies one bytes, starting at LSB.
                                        ///  Output width of image has no restrictions.
    ChiFormatP010            = 22,      ///< P010 format.
    ChiFormatRawPlain64      = 23,      ///< Raw Plain 64
    ChiFormatUBWCP010        = 24,      ///< UBWC P010 format.
    ChiFormatRawDepth        = 25,      ///< 16 bit depth
} CHIBUFFERFORMAT;

/// @brief Direction of buffer
typedef enum ChiCustomHwBufferDirection
{
    Input,                    ///< Input Buffer
    Output,                   ///< Output Buffer
    MaxBufferDirection        ///< Max Buffer Direction
} CHICUSTOMHWBUFFERDIRECTION;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc consumer flags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// These flags are used for allocating gralloc memory. These flags and values are inline gralloc definitions
enum ChiGralloc1ConsumerFlags : UINT64
{
    ChiGralloc1ConsumerUsageNone            = 0,                               ///< 0
    ChiGralloc1ConsumerUsageCpuReadNever    = 0,                               ///< 0
    ChiGralloc1ConsumerUsageCpuRead         = 1ULL << 1,                       ///< 2
    ChiGralloc1ConsumerUsageCpuReadOften    = 1ULL << 2 |                      ///< 6
                                              ChiGralloc1ConsumerUsageCpuRead,
    ChiGralloc1ConsumerUsageGpuTexture      = 1ULL << 8,
    ChiGralloc1ConsumerUsageHwComposer      = 1ULL << 11,
    ChiGralloc1ConsumerUsageClientTarget    = 1ULL << 12,
    ChiGralloc1ConsumerUsageCursor          = 1ULL << 15,
    ChiGralloc1ConsumerUsageVideoEncoder    = 1ULL << 16,
    ChiGralloc1ConsumerUsageCamera          = 1ULL << 18,
    ChiGralloc1ConsumerUsageRenderScript    = 1ULL << 20,
    ChiGralloc1ConsumerUsageForeignBuffers  = 1ULL << 21,
    ChiGralloc1ConsumerUsageGpuDataBuffer   = 1ULL << 23,
    ChiGralloc1ConsumerUsagePrivate_0       = 1ULL << 28,
    ChiGralloc1ConsumerUsagePrivate_1       = 1ULL << 29,
    ChiGralloc1ConsumerUsagePrivate_2       = 1ULL << 30,
    ChiGralloc1ConsumerUsagePrivate_3       = 1ULL << 31,
    ChiGralloc1ConsumerUsagePrivate_19      = 1ULL << 48,
    ChiGralloc1ConsumerUsagePrivate_18      = 1ULL << 49,
    ChiGralloc1ConsumerUsagePrivate_17      = 1ULL << 50,
    ChiGralloc1ConsumerUsagePrivate_16      = 1ULL << 51,
    ChiGralloc1ConsumerUsagePrivate_15      = 1ULL << 52,
    ChiGralloc1ConsumerUsagePrivate_14      = 1ULL << 53,
    ChiGralloc1ConsumerUsagePrivate_13      = 1ULL << 54,
    ChiGralloc1ConsumerUsagePrivate_12      = 1ULL << 55,
    ChiGralloc1ConsumerUsagePrivate_11      = 1ULL << 56,
    ChiGralloc1ConsumerUsagePrivate_10      = 1ULL << 57,
    ChiGralloc1ConsumerUsagePrivate_9       = 1ULL << 58,
    ChiGralloc1ConsumerUsagePrivate_8       = 1ULL << 59,
    ChiGralloc1ConsumerUsagePrivate_7       = 1ULL << 60,
    ChiGralloc1ConsumerUsagePrivate_6       = 1ULL << 61,
    ChiGralloc1ConsumerUsagePrivate_5       = 1ULL << 62,
    ChiGralloc1ConsumerUsagePrivate_4       = 1ULL << 63,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Gralloc producer flags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// These flags are used for allocating gralloc memory. These flags and values are inline gralloc definitions
enum ChiGralloc1ProducerFlags : UINT64
{
    ChiGralloc1ProducerUsageNone                    = 0,                               ///< 0
    ChiGralloc1ProducerUsageCpuWriteNever           = 0,                               ///< 0
    ChiGralloc1ProducerUsageCpuRead                 = 1ULL << 1,
    ChiGralloc1ProducerUsageCpuReadOften            = 1ULL << 2 |
                                                      ChiGralloc1ProducerUsageCpuRead,
    ChiGralloc1ProducerUsageCpuWrite                = 1ULL << 5,
    ChiGralloc1ProducerUsageCpuWriteOften           = 1ULL << 6 |
                                                      ChiGralloc1ProducerUsageCpuWrite,
    ChiGralloc1ProducerUsageGpuRenderTarget         = 1ULL << 9,
    ChiGralloc1ProducerUsageProtected               = 1ULL << 14,
    ChiGralloc1ProducerUsageCamera                  = 1ULL << 17,
    ChiGralloc1ProducerUsageVideoDecoder            = 1ULL << 22,
    ChiGralloc1ProducerUsageSensorDirectData        = 1ULL << 23,
    ChiGralloc1ProducerUsagePrivate_0               = 1ULL << 28,
    ChiGralloc1ProducerUsagePrivate_1               = 1ULL << 29,
    ChiGralloc1ProducerUsagePrivate_2               = 1ULL << 30,
    ChiGralloc1ProducerUsagePrivate_3               = 1ULL << 31,
    ChiGralloc1ProducerUsagePrivate_19              = 1ULL << 48,
    ChiGralloc1ProducerUsagePrivate_18              = 1ULL << 49,
    ChiGralloc1ProducerUsagePrivate_17              = 1ULL << 50,
    ChiGralloc1ProducerUsagePrivate_16              = 1ULL << 51,
    ChiGralloc1ProducerUsagePrivate_15              = 1ULL << 52,
    ChiGralloc1ProducerUsagePrivate_14              = 1ULL << 53,
    ChiGralloc1ProducerUsagePrivate_13              = 1ULL << 54,
    ChiGralloc1ProducerUsagePrivate_12              = 1ULL << 55,
    ChiGralloc1ProducerUsagePrivate_11              = 1ULL << 56,
    ChiGralloc1ProducerUsagePrivate_10              = 1ULL << 57,
    ChiGralloc1ProducerUsagePrivate_9               = 1ULL << 58,
    ChiGralloc1ProducerUsagePrivate_8               = 1ULL << 59,
    ChiGralloc1ProducerUsagePrivate_7               = 1ULL << 60,
    ChiGralloc1ProducerUsagePrivate_6               = 1ULL << 61,
    ChiGralloc1ProducerUsagePrivate_5               = 1ULL << 62,
    ChiGralloc1ProducerUsagePrivate_4               = 1ULL << 63,
};

/// @brief Chi Buffer type
typedef enum ChiBufferType
{
    HALGralloc,         ///< Gralloc buffer owned by HAL. Gralloc native handle is passed as buffer handle.
    ChiGralloc,         ///< Gralloc buffer allocated and owned by CHI. Gralloc native handle is passed as buffer handle.
    ChiNative,          ///< Native buffer allocated and owned by CHI through bufferManagerOps APIs.
                        ///  Native buffer object handle is passed as buffer handle.
} CHIBUFFERTYPE;

/// @brief Chi Stream param for stride and slice info
typedef struct ChiStreamParams
{
    UINT32              planeStride; ///< The number of bytes between the first byte of two sequential lines on plane 1. It
                                     ///  may be greater than nWidth * nDepth / 8 if the line includes padding.
                                     ///  Macro-tile width aligned for UBWC
    UINT32              sliceHeight; ///< The number of lines in the plane which can be equal to or larger than actual frame
                                     ///  height. Tile height aligned for UBWC
    UINT32              streamFPS;   ///< FPS of this stream
} CHISTREAMPARAMS;

/// @brief Chi Hal Stream param for override pixel format and usage flags
typedef struct ChiHalStream
{
    INT32 id;                           ///<Stream ID - a nonnegative integer identifier for a stream.
    CHISTREAMFORMAT overrideFormat;     ///< An override pixel format for the buffers in this stream
    GrallocProducerUsage producerUsage; ///<The gralloc producer usage flags for this stream, as needed by the HAL.
    GrallocConsumerUsage consumerUsage; ///<The gralloc consumer usage flags for this stream, as needed by the HAL.
    UINT32 maxBuffers;
} CHIHALSTREAM;

/// @brief Stream structure
typedef struct ChiStream
{
    CHISTREAMTYPE     streamType;       ///< The type of the stream
    UINT32            width;            ///< The width in pixels of the buffers in this stream
    UINT32            height;           ///< The height in pixels of the buffers in this stream
    CHISTREAMFORMAT   format;           ///< The pixel format for the buffers in this stream. Format is a value from
                                        ///  the PixelFormat* list or from device-specific headers.
    GrallocUsage      grallocUsage;     ///< The gralloc usage (GRALLOC_USAGE_*) flags defined in gralloc.h for this stream
    UINT32            maxNumBuffers;    ///< The maximum number of buffers that may be dequeued at the same time
    VOID*             pPrivateInfo;     ///< A handle to private information for the stream
    CHIDATASPACE      dataspace;        ///< A field that describes the contents of the buffer
    CHISTREAMROTATION rotation;         ///< The output rotation of the stream
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
    const char*       physicalCameraId; ///< The physical camera id of the stream
#endif
    CHIHALSTREAM*     pHalStream;       ///< placeholder to holde internal info
    CHISTREAMPARAMS   streamParams;     ///< Stream specific definitions.
    void*             reserved[3];      ///< Padding reserved for future use
} CHISTREAM;

/// @brief Stream config informations
typedef struct ChiStreamConfigInfo
{
    UINT32            numStreams;       ///< Number of streams
    CHISTREAM**       pChiStreams;      ///< An array of chi stream pointers.
    UINT32            operationMode;    ///< Operation mode of chi streams
    const VOID*       pSessionSettings; ///< Pointer to session metadata buffer
} CHISTREAMCONFIGINFO;

/// @brief New BufferManager create data structure
struct CHIBufferManagerCreateData
{
    UINT32              width;                  ///< Buffer width
    UINT32              height;                 ///< Buffer height
    UINT32              format;                 ///< Buffer format
    UINT64              producerFlags;          ///< Buffer producer gralloc flags
    UINT64              consumerFlags;          ///< Buffer consumer gralloc flags
    UINT32              bufferStride;           ///< Buffer stride
    UINT                minBufferCount;         ///< The Minimun number of buffers
    UINT                maxBufferCount;         ///< The Maximum number of buffers to be allocated
    UINT                immediateBufferCount;   ///< The number of buffers to be allocated immediately
    BOOL                bEnableLateBinding;     ///< Whether enable image buffer late binding for this buffer manager.
                                                ///  Used only if unified buffer manager is used for Chi Buffers
    UINT32              bufferHeap;             ///< Buffer heap decides whether buffers to be allocated through gralloc or csl
    ChiStream*          pChiStream;             ///< Stream info needed for image format parameters initialization
};

/// @brief Stream buffer
typedef struct ChiBufferInfo
{
    CHIBUFFERTYPE       bufferType;     ///< Buffer Handle type
    CHIBUFFERHANDLE     phBuffer;       ///< Output Buffer handle
} CHIBUFFERINFO;

/// @brief Buffer fence data
typedef struct ChiFenceInfo
{
    BOOL                valid;          ///< Whether this fence is valid
    CHIFENCETYPE        type;           ///< Fence type
    union
    {
        INT             nativeFenceFD;  ///< Android Native fence file descriptor
        CHIFENCEHANDLE  hChiFence;      ///< Internal chi fence handle
    };
} CHIFENCEINFO;

/// @brief Stream buffer
typedef struct ChiStreamBuffer
{
    UINT32              size;           ///< Size of this structure
    CHISTREAM*          pStream;        ///< Stream information
    CHIBUFFERINFO       bufferInfo;     ///< Chi Buffer info
    INT                 bufferStatus;   ///< Status of the buffer
    CHIFENCEINFO        acquireFence;   ///< Acquire fence
    CHIFENCEINFO        releaseFence;   ///< Release fence

    VOID*   pParentSinkStreamPrivateInfo; ///< Private information(ChiStream::pPrivateInfo) of the sink
                                          ///  stream from which this stream buffer is generated
                                          ///  Must be NULL for output streams and framework
                                          ///  reprocess input streams
} CHISTREAMBUFFER;

/// @brief ChiMetadata type
typedef enum ChiMetadataTagType : UINT8
{
    CMB_TYPE_BYTE       = 0, ///< Unsigned char - 8 bit
    CMB_TYPE_INT32      = 1, ///< Signed integer - 32 bit
    CMB_TYPE_FLOAT      = 2, ///< Float - 32 bit
    CMB_TYPE_INT64      = 3, ///< Signed integer 64 bit
    CMB_TYPE_DOUBLE     = 4, ///< Double 64 bit
    CMB_TYPE_RATIONAL   = 5, ///< Rational camera_metadata_rational_t
} CHIMETADATATAGTYPE;

/// @brief Metadata Entry
typedef struct ChiMetadataEntry
{
    UINT32             tagID;    ///< Tag Identifier
    VOID*              pTagData; ///< Pointer to the tag data
    UINT32             count;    ///< Count of the tags
    UINT32             size;     ///< Size of the tags
    CHIMETADATATAGTYPE type;     ///< Type of the tag data
} CHIMETADATAENTRY;

/// @brief Result Private data
typedef struct ChiPrivateData
{
    UINT32                 streamIndex;                           ///< Private data to save pipeline index from chi usecase
    UINT32                 featureType;                           ///< Feature used for this specific request
    UINT32                 numInputBuffers;                       ///< Number of input buffers for this request
    CHIBUFFERINFO          inputBuffers[PrivBufferQueueDepth];    ///< Input buffer handle pointers for this request
    CHIBUFFERMANAGERHANDLE bufferManagers[PrivBufferQueueDepth];  ///< The buffer managers which the input buffers belong to
    UINT32                 readyMask;                             ///< Ready mask for input buffer release. Set the bits
                                                                  ///  when the result is coming, and clear it after the input
                                                                  ///  buffer is released
} CHIPRIVDATA;

/// @brief Capture result
typedef struct ChiCaptureResult
{
    UINT32                      frameworkFrameNum;  ///< Unique frame number
    const VOID*                 pResultMetadata;    ///< Result metadata for this capture. Can be NULL
    UINT32                      numOutputBuffers;   ///< Number of output buffers returned in this result structure
    const CHISTREAMBUFFER*      pOutputBuffers;     ///< Handles for the output stream buffers for this capture
    const CHISTREAMBUFFER*      pInputBuffer;       ///< Handle for the input stream buffer for this capture. Can be NULL
    UINT32                      numPartialMetadata; ///< Number of partial metadata results
#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) //Android-P or better
    UINT32                      numPhysCamMetadata; ///< Number of physical camera metadata
    const CHAR**                physCamIds;         ///< The array of physical camera ids
    const VOID**                physCamMetadata;    ///< The array of physical camera metadata
#endif
    VOID*                       pPrivData;          ///< Pointer to private data. Managed by client.
    CHIMETAHANDLE               pInputMetadata;     ///< Input metadata for this request
    CHIMETAHANDLE               pOutputMetadata;    ///< Output metadata for this request.
} CHICAPTURERESULT;

/// @brief Partial Capture result
typedef struct ChiPartialCaptureResult
{
    UINT32                  frameworkFrameNum;      ///< Unique frame number
    CHIMETAHANDLE           pPartialResultMetadata; ///< Partial Result metadata for this capture.
    VOID*                   pPrivData;              ///< Pointer to private data. Managed by client.
} CHIPARTIALCAPTURERESULT;

/// @brief Message type
typedef enum ChiMessageType
{
    ChiMessageTypeError             = 1,   ///< An error has occurred
    ChiMessageTypeShutter           = 2,   ///< The exposure of a given request or processing a reprocess request has begun
    ChiMessageTypeSof               = 3,   ///< SOF event
    ChiMessageTypeMetaBufferDone    = 4,   ///< Metabuffer done event
    ChiMessageTypeTriggerRecovery   = 5    ///< Trigger recovery
} CHIMESSAGETYPE;

/// @brief Error message codes
typedef enum ChiErrorMessageCode
{
    MessageCodeDevice           = 1,    ///< A serious failure occurred and no further frames will be produced by the device
    MessageCodeRequest          = 2,    ///< An error has occurred in processing a request and no output will be produced for this
                                        ///  request
    MessageCodeResult           = 3,    ///< An error has occurred in producing an output result metadata buffer for a request, but
                                        ///  output stream buffers for it will still be available
    MessageCodeBuffer           = 4,    ///< An error has occurred in placing an output buffer into a stream for a request. The frame
                                        ///  metadata and other buffers may still be available
    MessageCodeTriggerRecovery  = 5     ///< An error has occurred and we need to trigger recovery
} CHIERRORMESSAGECODE;

/// @brief Pipeline output type
typedef enum ChiPipelineOutputType
{
    ChiPipelineOutputDefault,
    ChiPipelineOutputPreview,
    ChiPipelineOutputVideo,
    ChiPipelineOutputSnapshot,
    ChiPipelineOutputDepth,
    ChiPipelineOutputMax,
} CHIPIPELINEOUTPUTTYPE;

/// @brief Error message structure
typedef struct ChiErrorMessage
{
    UINT32              frameworkFrameNum;  ///< Frame number of the request the error applies to. 0 if the frame number
                                            ///  isn't applicable to the error
    CHISTREAM*          pErrorStream;       ///< Pointer to the stream that had a failure. NULL if the stream isn't
                                            ///  applicable to the error
    CHIERRORMESSAGECODE errorMessageCode;   ///< The code for this error
} CHIERRORMESSAGE;

/// @brief Shutter message structure
typedef struct ChiShutterMessage
{
    UINT32 frameworkFrameNum;               ///< Frame number of the request that has begun exposure or reprocessing
    UINT64 timestamp;                       ///< Timestamp for the start of capture
} CHISHUTTERMESSAGE;

/// @brief SOF message structure
typedef struct ChiSofMessage
{
    UINT32  frameworkFrameNum;         ///< Frame number of the request that has begun exposure or reprocessing
    UINT64  timestamp;                 ///< Timestamp for the start of capture
    UINT32  sofId;                     ///< SOF id
    BOOL    bIsFrameworkFrameNumValid; ///< if Frame Number is Valid
} CHISOFMESSAGE;

/// @brief Metadata done message structure
typedef struct ChiMetaBufferDoneMessage
{
    UINT32           frameworkFrameNum;  ///< Frame number of the request associated with the buffers
    CHIMETAHANDLE    inputMetabuffer;    ///< Input metadata buffer handle. Can be NULL
    CHIMETAHANDLE    outputMetabuffer;   ///< Output metadata buffer handle. Can be NULL
    UINT32           numMetaBuffers;     ///< Number of metabuffers
} CHIMETABUFFERDONEMESSAGE;

/// @brief Message descriptor structure
typedef struct ChiMessageDescriptor
{
    CHIMESSAGETYPE messageType;            ///< Message type that determines what to use from the union below

    union
    {
        CHIERRORMESSAGE          errorMessage;           ///< Error message contents. Valid if messageType is Error
        CHISHUTTERMESSAGE        shutterMessage;         ///< Shutter message contents. Valid if messageType is Shutter
        CHISOFMESSAGE            sofMessage;             ///< SOF message contents. Valid if messageType is SOF
        CHIMETABUFFERDONEMESSAGE metaBufferDoneMessage;  ///< Metabuffer done message. Valid if messageType
                                                         ///  is ChiMessageTypeMetaBuferDone
        UINT8                    generic[32];            ///< Generic message contents used to ensure a minimum size
                                                         ///  for custom message types
    } message;

    VOID*                        pPrivData;              ///< Pointer to private data. Managed by client.
} CHIMESSAGEDESCRIPTOR;

/// @brief Callbacks used by the CHI implementation to call into the CHI app
typedef struct ChiCallBacks
{
    /// @brief CHI app callback method to send back the capture results
    void (*ChiProcessCaptureResult)(
        CHICAPTURERESULT*   pCaptureResult,
        VOID*               pPrivateCallbackData);

    /// @brief Asynchronous notification callback method
    void (*ChiNotify)(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
        VOID*                       pPrivateCallbackData);

    /// @brief CHI app callback method to send back the partial capture results
    void (*ChiProcessPartialCaptureResult)(
         CHIPARTIALCAPTURERESULT*   pCaptureResult,
         VOID*                      pPrivateCallbackData);


} CHICALLBACKS;

/// @brief Chi link buffer properties
typedef struct ChiLinkBufferProperties
{
    UINT32  bufferFormat;                       ///< Buffer format
    UINT32  bufferSize;                         ///< Buffer size (in case its a raw bytes buffer)
    UINT32  bufferImmediateAllocCount;          ///< Initial buffers will be allocated for the link
    UINT32  bufferQueueDepth;                   ///< Max buffers that will ever exist on the link
    UINT32  bufferHeap;                         ///< Buffer heap
    UINT32  bufferFlags;                        ///< Buffer flags
} CHILINKBUFFERPROPERTIES;

/// @brief Chi output port descriptor
typedef struct ChiOutputPortDescriptor
{
    UINT32  portId;                             ///< Input/Output port id
    UINT32  isSinkPort;                         ///< Sink port indicator
    UINT32  isOutputStreamBuffer;               ///< Does the port output a stream buffer
    UINT32  portSourceTypeId;                   ///< Port source type id
    UINT32  numSourceIdsMapped;                 ///< Number of sources mapped to this output port for bypass
    UINT32* pMappedSourceIds;                   ///< Source Ids mapped to this output port for bypass
} CHIOUTPUTPORTDESCRIPTOR;

/// @brief Chi input port descriptor
typedef struct ChiInputPortDescriptor
{
    UINT32  portId;                             ///< Input/Output port id
    UINT32  isInputStreamBuffer;                ///< Does this input port take a source buffer as input
    UINT32  portSourceTypeId;                   ///< Port source type id
} CHIINPUTPORTDESCRIPTOR;

/// @brief Chi node ports info
typedef struct ChiNodePorts
{
    UINT32                   numInputPorts;     ///< Number of input ports
    CHIINPUTPORTDESCRIPTOR*  pInputPorts;       ///< Pointer to input ports
    UINT32                   numOutputPorts;    ///< Number of output ports
    CHIOUTPUTPORTDESCRIPTOR* pOutputPorts;      ///< Pointer to output ports
} CHINODEPORTS;

/// @brief Chi node property
typedef struct ChiNodeProperty
{
    UINT32       id;                            ///< ID of the property
    const VOID*  pValue;                        ///< Pointer to data. Type dependent on ID
} CHINODEPROPERTY;

/// @brief Chi node info
typedef struct ChiNode
{
    CHINODEPROPERTY* pNodeProperties;           ///< Properties associated with the node
    UINT32           nodeId;                    ///< Node identifier
    UINT32           nodeInstanceId;            ///< Node instance identifier
    CHINODEPORTS     nodeAllPorts;              ///< Information about all ports
    UINT32           numProperties;             ///< Count of NodeProperty instances in pNodeProperties
    PruneSettings    pruneProperties;           ///< A list of prune properties
} CHINODE;

/// @brief Information about a node in the link descriptor that defines connection between two nodes
typedef struct ChiLinkNodeDescriptor
{
    UINT32        nodeId;                        ///< Node identifier
    UINT32        nodeInstanceId;                ///< Node instance id
    UINT32        nodePortId;                    ///< Node port id
    UINT32        portSourceTypeId;              ///< Port source type id
    PruneSettings pruneProperties;               ///< A list of prune properties
} CHILINKNODEDESCRIPTOR;

/// @brief Chi link properties
typedef struct ChiLinkProperties
{
    UINT32  isBatchedMode;                    ///< Batched mode indicator
    UINT32  linkFlags;                        ///< Link flags
} CHILINKPROPERTIES;

/// @brief Structure that represents one link originating from a node
typedef struct ChiNodeLink
{
    CHILINKNODEDESCRIPTOR   srcNode;            ///< Src node in a link
    UINT32                  numDestNodes;       ///< Dest nodes in a link that the src node can be connected to
    CHILINKNODEDESCRIPTOR*  pDestNodes;         ///< Pointer to all the dest nodes connected to the src node
    CHILINKBUFFERPROPERTIES bufferProperties;   ///< Buffer properties
    CHILINKPROPERTIES       linkProperties;     ///< Link properties
} CHINODELINK;

/// @brief Structure that contains all information required to create a pipeline
typedef struct ChiPipelineCreateDescriptor
{
    UINT32              size;                   ///< Size of this structure
    UINT32              numNodes;               ///< Number of pipeline nodes
    CHINODE*            pNodes;                 ///< Pipeline nodes
    UINT32              numLinks;               ///< Number of links
    CHINODELINK*        pLinks;                 ///< Each link descriptor
    UINT32              isRealTime;             ///< Is this a realtime pipeline
    UINT                numBatchedFrames;       ///< Number of framework frames batched
    UINT                maxFPSValue;            ///< maxFPSValue info of batched frames
    UINT32              cameraId;               ///< Camera Id of pipeline
    CHIMETAHANDLE       hPipelineMetadata;      ///< Valid Metadata buffer handle that contains tags related to initialization
    BOOL                HALOutputBufferCombined;///< Is the HAL output buffer combined for batch mode
} CHIPIPELINECREATEDESCRIPTOR;

/// @brief Structure that contains information about a capture request
typedef struct ChiCaptureRequest
{
    UINT64            frameNumber;              ///< Frame number
    CHIPIPELINEHANDLE hPipelineHandle;          ///< Pipeline handle
    UINT32            numInputs;                ///< Number of inputs
    CHISTREAMBUFFER*  pInputBuffers;            ///< List of input buffers
    UINT32            numOutputs;               ///< Number of outputs
    CHISTREAMBUFFER*  pOutputBuffers;           ///< List of output buffers
    const VOID*       pMetadata;                ///< Deprecated. This field will not be used by CamX
    CHIMETAHANDLE     pInputMetadata;           ///< Valid Metadata buffer handle that contains the input tags
    CHIMETAHANDLE     pOutputMetadata;          ///< Valid Metadata buffer handle will contain the result tags
    VOID*             pPrivData;                ///< Pointer to private data. Managed by client.
} CHICAPTUREREQUEST;

/// @brief Structure that represents all requests submitted at once, possibly to different pipelines
typedef struct ChiPipelineRequest
{
    CHIHANDLE                pSessionHandle;     ///< Session handle
    UINT32                   numRequests;        ///< Number of pipeline requests
    const CHICAPTUREREQUEST* pCaptureRequests;   ///< Individual request structure array
} CHIPIPELINEREQUEST;

/// @brief Buffer dimension
typedef struct ChiBufferDimension
{
    UINT32 width;                               ///< Width
    UINT32 height;                              ///< Height
} CHIBUFFERDIMENSION;

/// @brief Sink Raw format
typedef struct ChiRawFormatPort
{
    UINT32 inputPortId;    ///< input port id
    UINT32 format;         ///< RAW format
    UINT32 sinkPortId;     ///< sink port ID
} CHIRAWFORMATPORT;

/// @brief Metadata client ID
typedef struct ChiMetadataClientID
{
    BIT frameNumber : 24;   ///< Frame number
    BIT clientIndex :  7;   ///< Index of the client
    BIT reserved    :  1;   ///< Reserved
} CHIMETADATACLIENTID;

/// @brief Buffer dimension options
typedef struct ChiBufferOptions
{
    UINT32             size;                    ///< Size of this structure
    CHIBUFFERDIMENSION minDimension;            ///< Min Dimension
    CHIBUFFERDIMENSION maxDimension;            ///< Max Dimension
    CHIBUFFERDIMENSION optimalDimension;        ///< Optimal Dimension
} CHIBUFFEROPTIONS;

/// @brief Structure to describe the buffer on a input/output port
typedef struct ChiPortBufferDescriptor
{
    UINT32                   size;                          ///< Size of this structure
    UINT                     numNodePorts;                  ///< number of NodePorts for buffer
    ChiLinkNodeDescriptor*   pNodePort;                     ///< NodePort
    CHISTREAM*               pStream;                       ///< Stream representing the buffer, which is also passed
                                                            ///  in pipeline request
    union
    {
        struct
        {
            UINT32 bIsOverrideImplDefinedWithRaw    : 1;  ///< Select MIPI Raw for app implementation defined format
            UINT32 hasValidBufferNegotiationOptions : 1;  ///< If true, pBufferNegotiationOptions will be used during buffer
                                                          ///  negotiations for pStream during the backwards walk
            UINT32 reserved                         : 30; ///< Reserved for future flags
        };
        UINT32 flagValue;
    };
    const CHIBUFFEROPTIONS* pBufferNegotiationsOptions;
} CHIPORTBUFFERDESCRIPTOR;

/// @brief Structure to describe the metadata information from the pipeline
typedef struct ChiPipelineMetadataInfo
{
    UINT32   maxNumMetaBuffers;                                ///< Maximum number of metadata buffers required by the
                                                               ///  pipeline
    UINT32   publishTagCount;                                  ///< Count of the tags published by the pipeline
    UINT32   publishPartialTagCount;                           ///< Count of the partial metadata tags published by the
                                                               ///  pipeline
    UINT32   publishTagArray[MaximumChiPipelineTagsPublished]; ///< Constant array of tags published by the pipeline
} CHIPIPELINEMETADATAINFO;

/// @brief Rational
typedef struct ChiRational
{
    UINT32   numerator;                         ///< Supported with no perf impact
    UINT32   denominator;                       ///< HQ mode available with perf impact
} CHIRATIONAL;

/// @brief Sensor capability
typedef union ChiSensorModeCaps
{
    struct
    {
        BIT Normal   : 1;                       ///< Normal       mode
        BIT HFR      : 1;                       ///< HFR          mode
        BIT IHDR     : 1;                       ///< IHDR         mode
        BIT PDAF     : 1;                       ///< PDAF         mode
        BIT QuadCFA  : 1;                       ///< Quad CFA     mode
        BIT ZZHDR    : 1;                       ///< ZZHDR        mode
        BIT DEPTH    : 1;                       ///< DEPTH        mode
        BIT SHDR     : 1;                       ///< SHDR         mode
        BIT FS       : 1;                       ///< Fast shutter mode
        BIT Internal : 1;                       ///< Internal     mode
        BIT reserved : 22;                      ///< Reserved
    }u;
    UINT32 value;
} CHISENSORMODECAPS;


/// @brief SensorStreamType Type of the stream: BLOB, IMAGE, PDAF, HDR, META
typedef enum ChiSensorStreamType
{
    StreamBlob  = 0,                            ///< Blob  stream
    StreamImage = 1,                            ///< Image stream
    StreamPDAF  = 2,                            ///< Pdaf  stream
    StreamHDR   = 3,                            ///< Hdr   stream
    StreamMeta  = 4,                            ///< Meta  stream
    StreamMAX   = 5                             ///< Total number of streams
} CHISENSORSTREAMTYPE;

/// @brief RemosaicType Type of the remosaic: SW or HW
typedef enum ChiRemosaicType
{
    UnKnown     = -1,
    SWRemosaic  = 0,
    HWRemosaic  = 1
} CHIREMOSAICTYPE;

/// @brief ChiPDAFType Type of the PDAF supported for a particular sensor mode
typedef enum ChiPDAFType
{
    PDTypeUnknown = 0, ///< PDAF Type unknown
    PDType1       = 1, ///< Type1 PDAF (calculated PD values are sent by sensor)
    PDType2       = 2, ///< Type2 PDAF (raw PD pixels are sent out by sensor to calculate PD)
    PDType3       = 3, ///< Type3 PDAF (PD pixels embedded in raw image, so need to seperate them and calculate PD)
    PDType2PD     = 4  ///< 2PD PDAF (Similar to type2 with dual photo diode PD pixels)
} CHIPDAFTYPE;

/// @brief Structure containing width and height integer values, along with a start offset
typedef struct ChiRect
{
    UINT32      left;                           ///< x coordinate for top-left point
    UINT32      top;                            ///< y coordinate for top-left point
    UINT32      width;                          ///< width
    UINT32      height;                         ///< height
} CHIRECT;

/// @brief Structure similar to ChiRect, but point's coordinate can be negative
typedef struct ChiRectINT
{
    INT32       left;                           ///< x coordinate for top-left point
    INT32       top;                            ///< y coordinate for top-left point
    UINT32      width;                          ///< width
    UINT32      height;                         ///< height
} CHIRECTINT;

/// @brief Structure containing width and height integer values, along with a start offset
typedef struct ChiRectEXT
{
    INT32      left;                           ///< x coordinate for top-left point
    INT32      top;                            ///< y coordinate for top-left point
    INT32      right;                          ///< x coordinate for bottom-right point
    INT32      bottom;                         ///< y coordinate for bottom-right point
} CHIRECTEXT;

/// @brief Structure containing width and height integer values
typedef struct ChiDimension
{
    UINT32      width;                          ///< width
    UINT32      height;                         ///< height
} CHIDIMENSION;

/// @brief Resource policy use to communicate use cases intent so the driver can make the right trade-off in optimizing power
///        and usage of HW resources.
typedef enum ChiResourcePolicy
{
    GreedyPowerOptimal = 0,    ///< Will try to use more HW resources to optimize for power if power criteria pass
                               ///  (which may lead to out-of-resource situation for multi-sensor sessions and use cases.)
    HWResourceMinimal          ///< Will limit the number of HW resources used for each stream to maxmize the number
                               ///  of simultaneous sensors (which implies each stream will be limited in resolution.)
} CHIRESOURCEPOLICY;

/// @brief Stream Configuration for sensors
typedef struct ChiSensorStreamConfig
{
    UINT32               vc;                               ///< Virtual Channel
    UINT32               dt;                               ///< Data type
    CHIRECT              frameDimension;                   ///< Sensor output Frame dimension
    UINT32               bpp;                              ///< Bits per pixel
    CHISENSORSTREAMTYPE  streamtype;                       ///< sensor stream type
} CHISENSORSTREAMCONFIG;

/// @brief Sensor mode information
typedef struct ChiSensorModeInfo
{
    UINT32                 size;                             ///< Size of this structure
    UINT32                 modeIndex;                        ///< Mode index
    UINT32                 arraySizeInMPix;                  ///< Array size
    CHIRECT                frameDimension;                   ///< Sensor output Frame dimension
    CHIRECT                cropInfo;                         ///< CSID crop requested
    CHIRATIONAL            aspectRatio;                      ///< Aspect ratio
    UINT32                 bpp;                              ///< Bits per pixel
    UINT32                 frameRate;                        ///< Frame rate
    UINT32                 batchedFrames;                    ///< Batched frames
    CHISENSORMODECAPS      sensorModeCaps;                   ///< sensor mode capabilities like HFR, Quadra etc
    CHISENSORSTREAMTYPE    streamtype;                       ///< sensor stream type
    CHIREMOSAICTYPE        remosaictype;                     ///< remosica type
    UINT32                 horizontalBinning;                ///< Horizontal binning value
    UINT32                 verticalBinning;                  ///< Vertical binning value
    BOOL                   HALOutputBufferCombined;          ///< Is the HAL output buffer combined for batch mode
    UINT32                 CSIPHYId;                         ///< CSI Phy ID
    UINT32                 is3Phase;                         ///< Is 3 Phase
    UINT32                 laneCount;                        ///< Lane Count
    UINT64                 outPixelClock;                    ///< Output pixel clock
    UINT64                 streamConfigCount;                ///< Stream Count
    UINT32                 laneCfg;                          ///< Lane config
    CHISENSORSTREAMCONFIG  streamConfig[StreamMAX];          ///< Stream Config Array
    CHIRectangle           activeArrayCropWindow;            ///< Crop co-ordinates with respect to active pixel array
    CHIPDAFTYPE            PDAFType;                         ///< Indicates type of the PD if its available in the mode
} CHISENSORMODEINFO;

/// @brief Sensor mode selection information
typedef struct ChiSensorModePickHint
{
    BOOL              postSensorUpscale;        ///< Flag to indicate if upscale is allowed for postprocess
    CHISENSORMODECAPS sensorModeCaps;           ///< Specific sensor mode caps used for sensor mode selection
    CHIDIMENSION      sensorOutputSize;         ///< Override output dimension for sensor mode selection
} CHISENSORMODEPICKHINT;

///< Capabilities related to the device's imaging characteristics
typedef struct ChiSensorCaps
{
    UINT32                  size;               ///< Size of this structure
    UINT32                  sensorId;           ///< Sensor Id.
    CHIDIRECTION            facing;             ///< Direction the camera faces relative to device screen
    CHISENSORPOSITIONTYPE   positionType;       ///< main, aux camera info
    FLOAT                   pixelSize;          ///< Physical size of one pixel in micro meter
    CHIRECT                 activeArray;        ///< Sensor active pixel array
    VOID*                   pRawOTPData;        ///< Pointer to raw OTP data
    UINT32                  rawOTPDataSize;     ///< Size of raw OTP data
    CHAR*                   pSensorName;        ///< Sensor Name
} CHISENSORCAPS;

/// @brief Capabilities related to the device's lens characteristics
typedef struct ChiLensCaps {
    UINT32      size;                           ///< Size of this structure
    FLOAT       focalLength;                    ///< Lens focal length
    BOOL        isFixedFocus;                   ///< flag to indicate is fixed focus or not
    FLOAT       horViewAngle;                   ///< Lens Horizontal view angle in degrees

} CHILENSCAPS;

/// @brief Structure to represent information about a single camera
typedef struct ChiCameraInfo
{
    UINT32           size;                      ///< Structure size
    UINT32           numSensorModes;            ///< Number of sensor modes
    VOID*            pLegacy;                   ///< Legacy
    CHISENSORCAPS    sensorCaps;                ///< Capabilities related to the device's imaging characteristics
    CHILENSCAPS      lensCaps;                  ///< Capabilities related to the device's lens characteristics
} CHICAMERAINFO;

/// @brief Input buffer info to be used during session create
typedef struct ChiInputBufferInfo
{
    UINT32                         numInputBuffers;         ///< Number of input buffers
    const CHIPORTBUFFERDESCRIPTOR* pInputBufferDescriptors; ///< Input buffer descriptors
} CHIINPUTBUFFERINFO;

/// @brief Info about the sensor used in a pipeline
typedef struct ChiSensorInfo
{
    UINT32                   cameraId;                      ///< Camera Id
    const CHISENSORMODEINFO* pSensorModeInfo;               ///< Sensor mode info
} CHISENSORINFO;

/// @brief Information about pipeline inputs
typedef struct ChiPipelineInputInfo
{
    UINT32             isInputSensor;                   ///< Is the pipeline input a sensor
    union
    {
        CHISENSORINFO      sensorInfo;                      ///< If pipeline input is sensor, use this structure
        CHIINPUTBUFFERINFO inputBufferInfo;                 ///< If pipeline input is not sensor, its input buffer(s)
    };

} CHIPIPELINEINPUTINFO;

/// @brief Pipeline output Dimension
typedef struct ChiPipelineOutputDimension
{
    CHIBUFFERDIMENSION      outputDimension;
    CHIPIPELINEOUTPUTTYPE   outputType;
} CHIPIPELINEOUTPUTDIMENSION;

/// @brief Pipeline output information
typedef struct ChiPipelineOutputInfo
{
    CHIPIPELINEHANDLE hPipelineHandle;                     ///< Pipeline handle that represents the created pipeline
} CHIPIPELINEOUTPUTINFO;

/// @brief Pipeline info used during session create
typedef struct ChiPipelineInfo
{
    CHIPIPELINEDESCRIPTOR hPipelineDescriptor;              ///< Pipeline descriptor
    CHIPIPELINEINPUTINFO  pipelineInputInfo;                ///< Pipeline input info
    CHIPIPELINEOUTPUTINFO pipelineOutputInfo;               ///< Pipeline output info
    UINT32                pipelineResourcePolicy;           ///< Resource policy (CHIRRESOURCEPOLICY)
    BOOL                  isDeferFinalizeNeeded;            ///< Flag to indicate if need defer pipeline finalization
} CHIPIPELINEINFO;

/// @brief Pipeline input options
typedef struct ChiPipelineInputOptions
{
    CHILINKNODEDESCRIPTOR nodePort;                         ///< Information about the input buffer node/port
    CHIBUFFEROPTIONS      bufferOptions;                    ///< Buffer requirement options
} CHIPIPELINEINPUTOPTIONS;

/// @brief pipeline deactivate modes bitmask
typedef UINT32 CHIDEACTIVATEPIPELINEMODE;
static const CHIDEACTIVATEPIPELINEMODE CHIDeactivateModeDefault         = (1 << 0); ///< Default mode, deactivate all nodes in
                                                                                    /// pipeline
static const CHIDEACTIVATEPIPELINEMODE CHIDeactivateModeSensorStandby   = (1 << 1); ///< Sensor stand by mode
static const CHIDEACTIVATEPIPELINEMODE CHIDeactivateModeRealTimeDevices = (1 << 2); ///< Sensor stand by mode
static const CHIDEACTIVATEPIPELINEMODE CHIDeactivateModeReleaseBuffer   = (1 << 3); ///< Release cslalloc buffers of all nodes
                                                                                    ///< in pipeline
static const CHIDEACTIVATEPIPELINEMODE CHIDeactivateModeUnlinkPipeline  = (1 << 4); ///< Stream Off and Unlink pipeline

/// @brief Session creation flags
typedef union ChiSessionFlags
{
    struct
    {
        BIT Deprecated          : 1;    ///< Do not use
        BIT SupportsPartialMeta : 1;    ///< TRUE if the session can handle a partial metadata callback
        BIT isNativeChi         : 1;    ///< To indicate if the session is related to native Chi
        BIT IsFastAecSession    : 1;    ///< TRUE if the session is a fast aec session
        BIT IsSSM               : 1;    ///< TO indicate if session is SSM
        BIT reserved            : 27;   ///< Reserved
    } u;
    UINT32 value;
} CHISESSIONFLAGS;

/// @brief Flush types
typedef enum ChiFlushType
{
    FlushSelective         = 1,  ///< Flush only selective requests
    FlushAll               = 2   ///< Flush all requests
} CHIFLUSHTYPE;

/// @breif various Camera Family SOC IDs
typedef enum
{
    ChiCameraTitanSocInvalid      = 0,     ///< Invalid SOC Id
    ChiCameraTitanSocSDM845       = 321,   ///< SDM845 SOC Id
    ChiCameraTitanSocSDM670       = 336,   ///< SDM670 SOC Id
    ChiCameraTitanSocSDM855       = 339,   ///< SDM855 SOC Id
    ChiCameraTitanSocSDM855P      = 361,   ///< SDM855P SOC Id
    ChiCameraTitanSocQCS605       = 347,   ///< QCS605 SOC Id
    ChiCameraTitanSocSM6150       = 355,   ///< SM6150 SOC Id
    ChiCameraTitanSocSDM865       = 356,   ///< SDM865 SOC Id
    ChiCameraTitanSocSM7150       = 365,   ///< SM7150 SOC Id
    ChiCameraTitanSocSM7150P      = 366,   ///< SM7150P SOC Id
    ChiCameraTitanSocSDM710       = 360,   ///< SDM710 SOC Id
    ChiCameraTitanSocSXR1120      = 370,   ///< SXR1120 SOC Id
    ChiCameraTitanSocSXR1130      = 371,   ///< SXR1130 SOC Id
    ChiCameraTitanSocSDM712       = 393,   ///< SDM712 SOC Id
    ChiCameraTitanSocSM7250       = 400,   ///< SM7250 SOC Id
    ChiCameraTitanSocSM6250       = 407,   ///< SM6250 SOC Id
    ChiCameraTitanSocQSM7250      = 440,   ///< QSM7250 SOC Id
    ChiCameraTitanSocSM6350       = 434,   ///< SM6350 SOC Id
    ChiCameraTitanSocSM7225       = 459,   ///< SM6350 AB SOC Id
} ChiCameraFamilySoc;


/// @brief Pipeline flush info
typedef struct ChiPipelineFlushInfo
{
    CHIFLUSHTYPE      flushType;         ///< Flush type
    CHIPIPELINEHANDLE hPipelineHandle;   ///< Pipeline handle
    VOID*             pPrivData;         ///< Pointer to private data. Managed by client
} CHIPIPELINEFLUSHINFO;

/// @brief Session flush info
typedef struct ChiSessionFlushInfo
{
    CHIHANDLE                   pSessionHandle;      ///< Session handle
    UINT32                      numPipelines;        ///< Number of pipelines
    const CHIPIPELINEFLUSHINFO* pPipelineFlushInfo;  ///< Individual request structure array
    BOOL                        isDestroyInProgress; ///< Flag to indicate whether destroy is in progress
} CHISESSIONFLUSHINFO;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiQueryVendorTagLocation
///
/// @brief  Query vendor tag location assigned by vendor tag manager
///
/// @return CDKResultSuccess if successful otherwise CDKResultNoSuch
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIQUERYVENDORTAGLOCATION)(
    const CHAR* pSectionName,
    const CHAR* pTagName,
    UINT32*     pTagLocation);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSetMetaData
///
/// @brief  Set meta data by tag
///
/// @param  [in] metaHandle meta data handle
/// @param  [in] tag        tag need to be set
/// @param  [in] pData      pointer to data buffer
/// @param  [in] count      size of data buffer in byte
///
/// @return Result status
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PSETMETADATA)(
   CHIHANDLE    metaHandle,
   UINT32       tag,
   VOID*        pData,
   SIZE_T       count);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGetMetaData
///
/// @brief  Get meta data by tag
///
/// @param  [in] metaHandle meta data handle
/// @param  [in] tag        tag need to be set
/// @param  [in] pData      pointer to buffer to receive data
/// @param  [in] count      size of data buffer in byte
///
/// @return Result status
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PGETMETADATA)(
   CHIHANDLE    metaHandle,
   UINT32       tag,
   VOID*        pData,
   SIZE_T       count);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiVendorTagOps
///
/// @brief  This structure contains basic functions for Vendor Tag IDs for camera_metadata structure
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ChiVendorTagsOps
{
    PFNCHIQUERYVENDORTAGLOCATION    pQueryVendorTagLocation;    ///< Get Vendor Tag Location
    PSETMETADATA                    pSetMetaData;               ///< Set meta data by tag id
    PGETMETADATA                    pGetMetaData;               ///< Get meta data by tag id
    VOID*                           reserved[8];                ///< Reserved for future use. These must be initialized to NULL.
} CHITAGSOPS;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGetTagOps
///
/// @brief  Gets the tag related function pointers
///
/// @param  [in][out] CHITAGSOPS
///
/// @return None
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNGETTAGOPS)(CHITAGSOPS*);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCREATEFENCE
///
/// @brief  The implementation for PFNCREATEFENCE
///
/// @param  [in] hChiContext CHI Context
/// @param  [in] pInfo       Pointer to the structure containing information about the fence.
/// @param  [in] phChiFence  Pointer to Chi fence handle to be filled.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCREATEFENCE)(
    CHIHANDLE              hChiContext,
    CHIFENCECREATEPARAMS*  pInfo,
    CHIFENCEHANDLE*        phChiFence);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNRELEASEFENCE
///
/// @brief  The implementation for PFNRELEASEFENCE
///
/// @param  [in] hChiContext CHI Context
/// @param  [in] hChiFence   Handle to Chi fence to be released
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNRELEASEFENCE)(
    CHIHANDLE       hChiContext,
    CHIFENCEHANDLE  hChiFence);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNWAITFENCEASYNC
///
/// @brief  The implementation for PFNWAITFENCEASYNC
///
/// @param  [in] hChiContext CHI Context
/// @param  [in] pCallbackFn Callback function
/// @param  [in] hChiFence   Handle to Chi fence to be released
/// @param  [in] pData       User data pointer
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNWAITFENCEASYNC)(
    CHIHANDLE            hChiContext,
    PFNCHIFENCECALLBACK  pCallbackFn,
    CHIFENCEHANDLE       hChiFence,
    VOID*                pData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNSIGNALFENCE
///
/// @brief  The implementation for PFNSIGNALFENCE
///
/// @param  [in] hChiContext  CHI Context
/// @param  [in] hChiFence    Handle to Chi fence to be released
/// @param  [in] statusResult Fence signalled status result
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNSIGNALFENCE)(
    CHIHANDLE       hChiContext,
    CHIFENCEHANDLE  hChiFence,
    CDKResult       statusResult);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNGETFENCESTATUS
///
/// @brief  The implementation for PFNGETFENCESTATUS
///
/// @param  [in] hChiContext CHI Context
/// @param  [in] hChiFence   Handle to Chi fence to be queried for status
/// @param  [in] pResult     Pointer to the CDKResult pointer to be filled with fence status
///                          CDKResultSuccess        - If the fence is signalled with Success
///                          CDKResultEFailed        - If the fence is signalled with Failure
///                          CDKResultEInvalidState  - If the fence is not yet signalled
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNGETFENCESTATUS)(
    CHIHANDLE       hChiContext,
    CHIFENCEHANDLE  hChiFence,
    CDKResult*      pResult);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFenceOps
///
/// @brief  This structure contains functions for CHI fence(s)
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ChiFenceOps
{
    PFNCREATEFENCE      pCreateFence;
    PFNRELEASEFENCE     pReleaseFence;
    PFNWAITFENCEASYNC   pWaitFenceAsync;
    PFNSIGNALFENCE      pSignalFence;
    PFNGETFENCESTATUS   pGetFenceStatus;
    VOID*               reserved[8];        ///< Reserved for future use. These must be initialized to NULL.
} CHIFENCEOPS;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNGETFENCEOPS
///
/// @brief  Gets the CHI fence related function pointers
///
/// @param  [in][out] CHIFENCEOPS
///
/// @return None
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNGETFENCEOPS)(
    CHIFENCEOPS*);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Chi Metadata Buffer (CMB)
///
/// CMB contains the camera metadata (all the camera control parameters and information specified by camera2 metadata
/// Vendor tags and Proprietary information which is exchanged between the nodes)
///
/// CMB Operations is classified into:-
/// 1. Basic metadata operations (Create/Destroy/Get/Set/Copy/Merge/Invalidate)
/// 2. Supplementary operations for metadata buffer linkage and management (Add/Release/ReferenceCount/Invalidate/Merge)
/// 3. Iterator for tag traversal
/// 4. Global Metadata Information Query operations that provides all the information regarding the metadata that are
//     visible to the client such as size, count, type etc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CMB Basic Operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_CREATE
///
/// @brief  Creates a new Metadata buffer with zero tags. Metadata handle will be returned on success
///
/// @param  [out] pMetaHandle   Pointer to the Metadata handle. pMetaHandle should not be NULL. If successful,
///                             *pMetaHandle will be updated with the new handle.
/// @param  [in]  pPrivateData  Sets the private data associated with the handle. Can be NULL
///
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_CREATE)(
    CHIMETAHANDLE*     pMetaHandle,
    CHIMETAPRIVATEDATA pPrivateData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_CREATE_WITH_TAGARRAY
///
/// @brief  Creates a Metadata handle given the tagList (pTagList parameter). pTagList must contain only the valid entries.
///         If using vendor tags, the user must make prior call to PFNCHIQUERYVENDORTAGLOCATION to get the 32-bit tagID
///         corresponding to each vendor tags. All the information regarding visible valid tags can be obtained by
///         PFN_CMB_GET_METADATA_INFO_TABLE operation
///
/// @param  [in]  pTagList      Pointer to the linear array of tag enumerations
/// @param  [in]  tagListCount  Number of tags in the array
/// @param  [out] pMetaHandle   Pointer to the Metadata handle. pMetaHandle should not be NULL. If successful,
///                             *pMetaHandle will be updated with the new handle.
/// @param  [in]  pPrivateData  Sets the private data associated with the handle. Can be NULL
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_CREATE_WITH_TAGARRAY)(
    const UINT32*      pTagList,
    UINT32             tagListCount,
    CHIMETAHANDLE*     pMetaHandle,
    CHIMETAPRIVATEDATA pPrivateData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_CREATE_WITH_ANDROIDMETADATA
///
/// @brief  Creates a Metadata handle given the android metadata (camera_metadata_t) specified by camera2 APIs
///
/// @param  [in]  pAndroidMeta  Pointer to android metadata
/// @param  [in]  pPrivateData  Sets the private data associated with the handle
/// @param  [out] pMetaHandle   Pointer to the Metadata handle. pMetaHandle should not be NULL. If successful,
///                             *pMetaHandle will be updated with the new handle.
/// @param  [in]  pPrivateData  Sets the private data associated with the handle. Can be NULL
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_CREATE_WITH_ANDROIDMETADATA)(
    const VOID*        pAndroidMeta,
    CHIMETAHANDLE*     pMetaHandle,
    CHIMETAPRIVATEDATA pPrivateData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_DESTROY
///
/// @brief  Destroys the Metadata buffer specified by the handle, hMetaHandle. Metadata buffer can be destroyed only
///         if the reference count is zero. The user can inquire the refernce count using PFN_CMB_REFERENCE_COUNT
///
/// @param  [in] hMetaHandle  Metadata handle
/// @param  [in] force        Flag to indicate whether the metadata must be force destroyed or not
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_DESTROY)(
    CHIMETAHANDLE hMetaHandle,
    BOOL          force);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_CLONE
///
/// @brief  Clone the Metadata buffer specified by hSrcMetaHandle and creates the new buffer. The handle for the new
///         metadata will updated in *pDstMetaHandle if successful
///
/// @param  [in]   hSrcMetaHandle Handle of the source metadata buffer
/// @param  [out]  phDstMetaHandle Pointer to the handle of the destination Metadata buffer
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_CLONE)(
    CHIMETAHANDLE  hSrcMetaHandle,
    CHIMETAHANDLE* phDstMetaHandle);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_GET_TAG
///
/// @brief  Get the tag data specified by the identifier, tagID. The memory of the tag (*ppData) belongs to the metadata
///         buffer. The client must ensure that the memory is not used after the metadata buffer (hMetaHandle) is
///         destroyed.
///
/// @param  [in]   hMetaHandle Metadata handle
/// @param  [in]   tagID       Tag Identifier
/// @param  [out]  ppData      Pointer to the payload of the tag. ppData must not be NULL. On success, *ppData will point to
///                            the valid data. The size and type of the data should be known prior using the API,
///                            PFN_CMB_GET_METADATA_INFO_TABLE.
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_GET_TAG)(
    CHIMETAHANDLE hMetaHandle,
    UINT32        tagID,
    VOID**        ppData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_GET_TAG_ENTRY
///
/// @brief  Get the tag entry specified by the tagID
///
/// @param  [in]   hMetaHandle Metadata handle
/// @param  [in]   tagID       Tag ID
/// @param  [out]  pEntry      Pointer to the tag entry
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_GET_TAG_ENTRY)(
    CHIMETAHANDLE       hMetaHandle,
    UINT32              tagID,
    CHIMETADATAENTRY*   pEntry);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_GET_VENDORTAG
///
/// @brief  Get the tag data specified by vendor tag section name and tagname. The memory of the tag (*ppData) belongs
///         to the metadata buffer. The client must ensure that the memory is not used after the metadata buffer
///         (hMetaHandle) is destroyed.
///
/// @param  [in]   hMetaHandle      Metadata handle
/// @param  [in]   pTagSectionName  Section name of the vendor tag
/// @param  [in]   pTagName         Vendor tag name
/// @param  [out]  ppData           Pointer to the payload of the tag. ppData must not be NULL.
///                                 On success, *ppData will point to the valid data. The size and type of the data
///                                 should be known prior using the API, PFN_CMB_GET_METADATA_INFO_TABLE.
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_GET_VENDORTAG)(
    CHIMETAHANDLE hMetaHandle,
    const CHAR*   pTagSectionName,
    const CHAR*   pTagName,
    VOID**        ppData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_GET_TAG_ENTRY
///
/// @brief  Get the vendor tag entry specified by the section name and tag name
///
/// @param  [in]   hMetaHandle      Metadata handle
/// @param  [in]   pTagSectionName  Section name of the vendor tag
/// @param  [in]   pTagName         Vendor tag name
/// @param  [out]  pEntry           Pointer to the tag entry
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_GET_VENDORTAG_ENTRY)(
    CHIMETAHANDLE       hMetaHandle,
    const CHAR*         pTagSectionName,
    const CHAR*         pTagName,
    CHIMETADATAENTRY*   pEntry);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_GET_TAG_BY_CAMERAID
///
/// @brief  Get the tag data specified by the identifier, tagID from a specific cameraId. This API is only valid
///         for metadata handles that are created using PFN_CMB_MERGE_MULTICAMERA_METADATA.
///         The memory of the tag (*ppData) belongs to the metadata
///         buffer. The client must ensure that the memory is not used after the metadata buffer (hMetaHandle) is
///         destroyed.
///
/// @param  [in]   hMetaHandle Metadata handle
/// @param  [in]   tagID       Tag Identifier
/// @param  [in]   cameraId    Camera Identifier
/// @param  [out]  ppData      Pointer to the payload of the tag. ppData must not be NULL. On success, *ppData will point to
///                            the valid data. The size and type of the data should be known prior using the API,
///                            PFN_CMB_GET_METADATA_INFO_TABLE.
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_GET_TAG_BY_CAMERAID)(
    CHIMETAHANDLE hMetaHandle,
    UINT32        tagID,
    UINT32        cameraId,
    VOID**        ppData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_SET_TAG
///
/// @brief  Set the tag specified by the tagID into the metadata buffer
///
/// @param  [in]  hMetaHandle Metadata handle
/// @param  [in]  tagID       Tag ID
/// @param  [in]  pData       Pointer to the tag data
/// @param  [in]  count       Count of the tag
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_SET_TAG)(
    CHIMETAHANDLE hMetaHandle,
    UINT32        tagID,
    const VOID*   pData,
    UINT32        count);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_SET_VENDORTAG
///
/// @brief  Set the Vendor tag specified by the section and tag name into the metadata buffer
///
/// @param  [in]  hMetaHandle      Metadata handle
/// @param  [in]  pTagSectionName  Section name of the vendor tag
/// @param  [in]  pTagName         Vendor tag name
/// @param  [in]  pData            Pointer to the tag data
/// @param  [in]  count            Count of the tag
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_SET_VENDORTAG)(
    CHIMETAHANDLE hMetaHandle,
    const CHAR*   pTagSectionName,
    const CHAR*   pTagName,
    const VOID*   pData,
    UINT32        count);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_SET_ANDROIDMETADATA
///
/// @brief  Sets all the tags given by android camera metadata into the metadata buffer
///
/// @param  [in] hMetaHandle  Metadata handle
/// @param  [in] pAndroidMeta Pointer to android camera metadata, camera_metadat_t
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_SET_ANDROIDMETADATA)(
    CHIMETAHANDLE hMetaHandle,
    const VOID*   pAndroidMeta);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_DELETE_TAG
///
/// @brief  Delete the tag specified by the tag identifier, tagID
///
/// @param  [in]  hMetaHandle  Metadata handle
/// @param  [in]  tagID        Tag Identifier
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_DELETE_TAG)(
    CHIMETAHANDLE hMetaHandle,
    UINT32        tagID);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_INVALIDATE
///
/// @brief  Invalidate all the tags in the metadata buffer. Upon successful invalidate, the external metadata reference
///         count must be zero.
///
/// @param  [in]  hMetaHandle  Metadata handle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_INVALIDATE)(
    CHIMETAHANDLE hMetaHandle);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_MERGE
///
/// @brief  Function to merge source metadata with the destination metadata. Tags from source buffer wont be deep copied
///         to the destination metadata buffer. After the successful merge operation, destination buffer will have tags
///         referring the source buffer
///
/// @param  [out]   hDstMetaHandle   Destination metadata handle
/// @param  [in]    hSrcMetaHandle   Source metadata handle
/// @param  [in]    disjoint         Flag to indicate whether the merge is disjoint. If disjoint flag is set, only the tags
///                                  which are not present in the destination buffer will be merged, Else, in case of
///                                  overlapping tags, entries in hDstMetaHandle will be overwritten with entries from
///                                  hSrcMetaHandle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_MERGE)(
    CHIMETAHANDLE hDstMetaHandle,
    CHIMETAHANDLE hSrcMetaHandle,
    BOOL          disjoint);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_COPY
///
/// @brief  Function to copy source metadata with the destination metadata. Tags from source buffer will be allocated and
///         copied to the destination metadata buffer.
///
/// @param  [out]   hDstMetaHandle   Destination metadata handle
/// @param  [in]    hSrcMetaHandle   Source metadata handle
/// @param  [in]    disjoint         Flag to indicate whether the copy is disjoint. If disjoint flag is set, only the tags
///                                  which are not present in the destination buffer will be copied. Else, in case of
///                                  overlapping tags, entries in hDstMetaHandle will be overwritten with entries from
///                                  hSrcMetaHandle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_COPY)(
    CHIMETAHANDLE hDstMetaHandle,
    CHIMETAHANDLE hSrcMetaHandle,
    BOOL          disjoint);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_MERGE_MULTICAMERA_METADATA
///
/// @brief  Function to merge multi-camera source metadatas to the destination metatdata.
///         Tags from source buffers wont be deep copied to the destination metadata buffer.
///         After the successful merge operation, destination buffer will have tags
///         referring to the source buffer
///
/// @param  [out]   hDstMetaHandle      Destination metadata handle
/// @param  [int]   srcMetaHandleCount  Count of handles in the array, phSrcMetaHandles
/// @param  [in]    phSrcMetaHandles    Array of source handles
/// @param  [in]    pCameraIdArray      Array of cameraIds associated with each metahandle
/// @param  [in]    primaryCameraId     Id for the primary camera
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_MERGE_MULTICAMERA_METADATA)(
    CHIMETAHANDLE  hDstMetaHandle,
    UINT32         srcMetaHandleCount,
    CHIMETAHANDLE* phSrcMetaHandles,
    UINT32*        pCameraIdArray,
    UINT32         primaryCameraId);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_CAPACITY
///
/// @brief  Returns the size of the storage space currently allocated for the metadata buffer in terms of bytes
///
/// @param  [in]  hMetaHandle  Metadata handle
/// @param  [out] pCapacity    Capacity of the metadata handle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_CAPACITY)(
    CHIMETAHANDLE hMetaHandle,
    UINT32*       pCapacity);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_TAG_COUNT
///
/// @brief  Returns the count of the tags inside the metadata buffer
///
/// @param  [in]  hMetaHandle  Metadata handle
/// @param  [out] pCount       Count of the tags inside the metadata buffer
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_TAG_COUNT)(
    CHIMETAHANDLE hMetaHandle,
    UINT32*       pCount);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_PRINT
///
/// @brief  Function to print the metadata information into the logcat/standard output
///
/// @param  [in]  hMetaHandle  Metadata handle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_PRINT)(
    CHIMETAHANDLE hMetaHandle);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_DUMP
///
/// @brief  Function to dump the metadata information into the file
///
/// @param  [in]  hMetaHandle  Metadata handle
/// @param  [in]  pFilename    Filename to dump the metadata
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_DUMP)(
    CHIMETAHANDLE hMetaHandle,
    const CHAR*   pFilename);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_BINARYDUMP
///
/// @brief  Function to binary dump the metadata information into the file
///
/// @param  [in]  hMetaHandle  Metadata handle
/// @param  [in]  pFilename    Filename to dump the metadata
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_BINARYDUMP)(
    CHIMETAHANDLE hMetaHandle,
    const CHAR*   pFilename);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CMB Buffer Managment Operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_ADD_REFERENCE
///
/// @brief  Add reference to the metabuffer
///
/// @param  [in]  hMetaHandle  Metadata handle
/// @param  [in]  clientID     Unique identity of the client
/// @param  [out] pRefCount    Returns the current reference count after increment
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_ADD_REFERENCE)(
    CHIMETAHANDLE       hMetaHandle,
    CHIMETADATACLIENTID clientID,
    UINT32*             pRefCount);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_RELEASE_REFERENCE
///
/// @brief  Release reference from the metabuffer
///
/// @param  [in]  hMetaHandle  Metadata handle
/// @param  [in]  clientID     Unique identity of the client
/// @param  [out] pRefCount    Returns the current reference count after decrement
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_RELEASE_REFERENCE)(
    CHIMETAHANDLE       hMetaHandle,
    CHIMETADATACLIENTID clientID,
    UINT32*             pRefCount);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_RELEASE_ALL_REFERENCES
///
/// @brief  Release all references from the metabuffer
///
/// @param  [in]  hMetaHandle           Metadata handle
/// @param  [in]  bCHIAndCAMXReferences Flag to indicate whether to remove only the CHI references or both CAMX and CHI
///                                     references. This flag should be set to TRUE only for Flush or teardown cases
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_RELEASE_ALL_REFERENCES)(
    CHIMETAHANDLE hMetaHandle,
    BOOL          bCHIAndCAMXReferences);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_REFERENCE_COUNT
///
/// @brief  Reference count of the metabuffer
///
/// @param  [in]  hMetaHandle  Metadata handle
/// @param  [out] pRefCount    Returns the current reference count
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_REFERENCE_COUNT)(
    CHIMETAHANDLE  hMetaHandle,
    UINT32*        pRefCount);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CMB Iterator Operations
///
/// Example:
///     CHIMETADATAITERATOR    hIterator;
///     CDKResult result = metadataOps.pIteratorCreate(metadataHandle, &hIterator);
///     if (CDKResultSuccess == result)
///     {
///         result = metadataOps.pIteratorBegin(hIterator);
///         while (CDKResultSuccess == result)
///         {
///             CHIMETADATAENTRY entry;
///             metadataOps.pIteratorGetEntry(hIterator, &entry);
///             // entry will be updated with the tag information
///             result = metadataOps.pIteratorNext(hIterator);
///         }
///     }
///     // To resuse the iterator
///     result = metadataOps.pIteratorBegin(hIterator);
///     // After all the iterations
///     result = metadataOps.pIteratorDestroy(hIterator);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_ITERATOR_CREATE
///
/// @brief  Create an iterator for the current metadata buffer. The user can use PFN_CMB_ITERATOR_BEGIN and
///         PFN_CMB_ITERATOR_NEXT to iterate through all the tags
///
/// @param  [in]  hMetaHandle  Metadata handle
/// @param  [out] pIterator   Pointer to the iterator handle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_ITERATOR_CREATE)(
    CHIMETAHANDLE           hMetaHandle,
    CHIMETADATAITERATOR*    pIterator);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_ITERATOR_DESTROY
///
/// @brief  Destroys the iterator object
///
/// @param  [in] hIterator  Iterator handle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_ITERATOR_DESTROY)(
    CHIMETADATAITERATOR hIterator);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_ITERATOR_BEGIN
///
/// @brief  Resets the metadata iterator to the first element. If there are no elements in the metadata,
///         CamxResultENoMore will be returned
///
/// @param  [in] hIterator  Iterator handle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_ITERATOR_BEGIN)(
    CHIMETADATAITERATOR hIterator);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_ITERATOR_NEXT
///
/// @brief  Resets the metadata iterator to the next element. If there are no elements in the metadata,
///         CamxResultENoMore will be returned
///
/// @param  [in] hIterator  Iterator handle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_ITERATOR_NEXT)(
    CHIMETADATAITERATOR hIterator);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_ITERATOR_GETENTRY
///
/// @brief  Get the entry corresponding to the current position of the iterator. If the iterator position is invalid,
///         or when the iterator is NULL, CDKResultEInvalidArgs will be returned
///
/// @param  [in]  hIterator  Iterator handle
/// @param  [out] pEntry     Caller should allocate memory for pEntry. Upon success, the fields in CHIMETADATAENTRY will be
///                          filled by the callee
///
/// @return CDKResultSuccess if successful CDK error values otherwise
//////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_ITERATOR_GETENTRY)(
    CHIMETADATAITERATOR hIterator,
    CHIMETADATAENTRY*   pEntry);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CMB Global Metadata Information Query Operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_GET_METADATA_ENTRY_COUNT
///
/// @brief  Gets the count of all metadata tags visible to the CHI client
///
/// @param  [out] pMetadataCount Pointer to the count of all available metadata which are visible to the client
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_GET_METADATA_ENTRY_COUNT)(
    UINT32* pMetadataCount);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_GET_METADATA_TABLE
///
/// @brief  Obtains the table of metadata entries. The table contains all the necessary information pertaining to the
///         visible metadata tags.
///
/// @param  [out] pMetadataEntryTable   Pointer to the list of all entries. Count of the entries must be obtained by calling
///                                     PFN_CMB_GET_METADATA_ENTRY_COUNT prior to this function call.
///                                     Size of the table of sizeof(CHIMETADATAENTRY) * metadataCount must be allocated.
///                                     pTagData in the entry will be set to NULL by the Callee as the API is not
///                                     associated with any particular Metadata buffer.
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_GET_METADATA_TABLE)(
    CHIMETADATAENTRY* pMetadataEntryTable);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CMB Default Settings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_GET_DEFAULT_ANDROIDMETADATA
///
/// @brief  Create an android metadata, camera_metadata_t with the default settings for the given camera, cameraId
///
/// @param  [in]    cameraId      Camera Identifier
/// @param  [out]   ppAndroidMeta Address of the pointer to android camera metadata. ppAndroidMeta should not be NULL.
///                               Upon success, ppAndroidMeta will contain valid camera_metadata_t packet
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_GET_DEFAULT_ANDROIDMETADATA)(
    UINT32          cameraId,
    const VOID**    ppAndroidMeta);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_GET_DEFAULT_METADATA
///
/// @brief  Create an metadata buffer with the default settings for the given camera, cameraId
///
/// @param  [in]    cameraId      Camera Identifier
/// @param  [out]   phMetaBuffer  Pointer to Chi metadata handle. phMetaBuffer should not be NULL. Upon success,
///                               *phMetaBuffer contains the valid handle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_GET_DEFAULT_METADATA)(
    UINT32          cameraId,
    CHIMETAHANDLE*  phMetaBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_FILTER
///
/// @brief  Filter the tags in the metadata buffer based on visibility parameters(frameworkTagsOnly,
///         filterProperties, pFilterTagArray etc) and generate android framework metadata
///
/// @param  [in]      hMetaHandle       Metadata handle
/// @param  [in][out] pAndroidMeta      Pointer to android camera metadata.
/// @param  [in]      frameworkTagsOnly Flag to indicate whether only the framework tags must be present in the
///                                     android metadata
/// @param  [in]      filterProperties  Flag to indicate whether to remove(filter out) Camx property tags
/// @param  [in]      filterTagCount    Count of the tags specified by pFilterTagArray.
/// @param  [in]      pFilterTagArray   List of camera metadata tags and vendor tags that needs to be removed from
///                                     framework metadata output, pAndroidMeta
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFN_CMB_FILTER)(
    CHIMETAHANDLE hMetaHandle,
    VOID*         pAndroidMeta,
    BOOL          frameworkTagsOnly,
    BOOL          filterProperties,
    UINT32        filterTagCount,
    UINT32*       pFilterTagArray);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFN_CMB_GET_PRIVATE_DATA
///
/// @brief  Retrieves the private data associated with the metadata handle
///
/// @param  [in]  hMetaHandle   Metadata handle
/// @param  [out] ppPrivateData Returns the private data associated with the handle
///
/// @return CDKResultSuccess if successful CDK error values otherwise
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFN_CMB_GET_PRIVATE_DATA)(
    CHIMETAHANDLE       hMetaHandle,
    CHIMETAPRIVATEDATA* ppPrivateData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiMetadataOps
///
/// @brief  This structure contains functions for CHI Metadata Buffer (CMB) Operations
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ChiMetadataOps
{
    /// Basic Metadata Operations
    PFN_CMB_CREATE                      pCreate;                    ///< Function pointer to create metadata buffer
    PFN_CMB_CREATE_WITH_TAGARRAY        pCreateWithTagArray;        ///< Function pointer to create metadata buffer
    PFN_CMB_CREATE_WITH_ANDROIDMETADATA pCreateWithAndroidMetadata; ///< Function pointer to create metadata buffer
    PFN_CMB_DESTROY                     pDestroy;                   ///< Function pointer to create metadata buffer
    PFN_CMB_GET_TAG                     pGetTag;                    ///< Function pointer to get tag from metadata buffer
    PFN_CMB_GET_TAG_ENTRY               pGetTagEntry;               ///< Function pointer to get the tag entry from
                                                                    ///< metadata buffer
    PFN_CMB_GET_VENDORTAG               pGetVendorTag;              ///< Function pointer to get vendor tag from
                                                                    ///  metadata buffer
    PFN_CMB_GET_VENDORTAG_ENTRY         pGetVendorTagEntry;         ///< Function pointer to get the vendor tag entry from
                                                                    ///  metadata buffer
    PFN_CMB_SET_TAG                     pSetTag;                    ///< Function pointer to set tag onto metadata buffer
    PFN_CMB_SET_VENDORTAG               pSetVendorTag;              ///< Function pointer to set tag onto metadata buffer
    PFN_CMB_SET_ANDROIDMETADATA         pSetAndroidMetadata;        ///< Function pointer to set tag onto metadata buffer
    PFN_CMB_DELETE_TAG                  pDeleteTag;                 ///< Function pointer to delete tag from metadata buffer
    PFN_CMB_INVALIDATE                  pInvalidate;                ///< Function pointer to invalidate all tags in the
                                                                    ///< metadata buffer
    PFN_CMB_MERGE                       pMerge;                     ///< Function pointer to merge metadata buffer with another
    PFN_CMB_COPY                        pCopy;                      ///< Function pointer to copy metadata from source to
                                                                    ///< the destination
    PFN_CMB_CLONE                       pClone;                     ///< Clone the metadata
    PFN_CMB_CAPACITY                    pCapacity;                  ///< Function pointer to get the capacity of metadata
    PFN_CMB_TAG_COUNT                   pCount;                     ///< Function pointer to get the count of tags
    PFN_CMB_PRINT                       pPrint;                     ///< Function pointer to print metadata information
    PFN_CMB_DUMP                        pDump;                      ///< Function pointer to dump metadata information
                                                                    ///< to the file
    PFN_CMB_BINARYDUMP                  pBinaryDump;                ///< Function pointer to binary dump metadata buffer

    /// Supplementary Metadata Buffer Management Operations
    PFN_CMB_ADD_REFERENCE               pAddReference;              ///< Function pointer to add reference to the metadata
                                                                    ///< buffer
    PFN_CMB_RELEASE_REFERENCE           pReleaseReference;          ///< Function pointer to release reference of the
                                                                    ///< metadata buffer
    PFN_CMB_REFERENCE_COUNT             pReferenceCount;            ///< Function pointer to get the reference count of the
                                                                    ///< metadata buffer

    /// Metadata Information Query Operations
    PFN_CMB_GET_METADATA_TABLE          pGetMetadataTable;          ///< Function pointer to get the information of the
                                                                    ///< available metadata entries
    PFN_CMB_GET_METADATA_ENTRY_COUNT    pGetMetadataEntryCount;     ///< Function pointer to get the count of all the
                                                                    ///< available metadata

    /// Iterator Operations
    PFN_CMB_ITERATOR_CREATE             pIteratorCreate;            ///< Creates an iterator for the metadata buffer tag
                                                                    ///< traversal
    PFN_CMB_ITERATOR_DESTROY            pIteratorDestroy;           ///< Destroys the iterator object
    PFN_CMB_ITERATOR_BEGIN              pIteratorBegin;             ///< Initializes the iterator to the first entry
    PFN_CMB_ITERATOR_NEXT               pIteratorNext;              ///< Moves the iterator to the next entry
    PFN_CMB_ITERATOR_GETENTRY           pIteratorGetEntry;          ///< Get the metadata entry corresponding to the
                                                                    ///< current iterator position

    /// Functions for creating metadata filled with default settings
    PFN_CMB_GET_DEFAULT_ANDROIDMETADATA pGetDefaultAndroidMeta;     ///< Function pointer to get default settings of
                                                                    ///< camera_metadata_t type
    PFN_CMB_GET_DEFAULT_METADATA        pGetDefaultMetadata;        ///< Function pointer to create a metadata buffer
                                                                    ///< with default settings

    PFN_CMB_FILTER                      pFilter;                    ///< Filter the metadata based on tag visibility
    PFN_CMB_GET_PRIVATE_DATA            pGetPrivateData;            ///< Gets the private data associated with the metadata
    PFN_CMB_RELEASE_ALL_REFERENCES      pReleaseAllReferences;      ///< Function pointer to release all the references of the
                                                                    ///< metadata buffer
    PFN_CMB_MERGE_MULTICAMERA_METADATA  pMergeMultiCameraMeta;      ///< Function pointer to merge multi-camera metadata
    PFN_CMB_GET_TAG_BY_CAMERAID         pGetTagByCameraId;          ///< Function pointer to get tag from metadata buffer
                                                                    ///  that belongs to a given cameraId
    VOID*                               reserved[8];                ///< Reserved for future use. Must be initialized to NULL
} CHIMETADATAOPS;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiMetadataOps
///
/// @brief  Gets the tag related function pointers
///
/// @param  pOps [in][out] CHIMETADATAOPS
///
/// @return None
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNGETMETADATAOPS)(
    CHIMETADATAOPS* pOps);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERCREATE
///
/// @brief  The implementation for PFNBUFFERMANAGERCREATE
///
/// @param  [in] pBufferManagerName String of Buffer manager's name
/// @param  [in] pCreateData        Pointer to BufferManager create data structure
///
/// @return Buffer manager handle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CHIBUFFERMANAGERHANDLE (*PFNBUFFERMANAGERCREATE)(
    const CHAR*                 pBufferManagerName,
    CHIBufferManagerCreateData* pCreateData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERDESTROY
///
/// @brief  The implementation for PFNBUFFERMANAGERDESTROY
///
/// @param  [in] hBufferManager Buffer manager handle
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNBUFFERMANAGERDESTROY)(
    CHIBUFFERMANAGERHANDLE hBufferManager);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERADDREFERENCE
///
/// @brief  The implementation for PFNBUFFERMANAGERADDREFERENCE
///
/// @param  [in] hBufferManager Buffer manager handle
/// @param  [in] hImageBuffer   Image buffer handle
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNBUFFERMANAGERADDREFERENCE)(
    CHIBUFFERMANAGERHANDLE hBufferManager,
    CHIBUFFERHANDLE        hImageBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERGETIMAGEBUFFER
///
/// @brief  The implementation for PFNBUFFERMANAGERGETIMAGEBUFFER
///
/// @param  [in] hBufferManager Buffer manager handle
///
/// @return Image buffer handle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CHIBUFFERHANDLE(*PFNBUFFERMANAGERGETIMAGEBUFFER)(
    CHIBUFFERMANAGERHANDLE hBufferManager);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERRELEASEREFERENCE
///
/// @brief  The implementation for PFNBUFFERMANAGERRELEASEREFERENCE
///
/// @param  [in] hBufferManager Buffer manager handle
/// @param  [in] hImageBuffer   Image buffer handle
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFNBUFFERMANAGERRELEASEREFERENCE)(
    CHIBUFFERMANAGERHANDLE hBufferManager,
    CHIBUFFERHANDLE        hImageBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERGETREFERENCE
///
/// @brief  The implementation for PFNBUFFERMANAGERGETREFERENCE
///
/// @param  [in] hImageBuffer   Image buffer handle
///
/// @return The reference count.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef UINT (*PFNBUFFERMANAGERGETREFERENCE)(
    CHIBUFFERHANDLE hImageBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERACTIVATE
///
/// @brief  The implementation for PFNBUFFERMANAGERACTIVATE
///
/// @param  [in] hBufferManager Buffer manager handle
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNBUFFERMANAGERACTIVATE)(
    CHIBUFFERMANAGERHANDLE hBufferManager);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERDEACTIVATE
///
/// @brief  The implementation for PFNBUFFERMANAGERDEACTIVATE
///
/// @param  [in] hBufferManager Buffer manager handle
/// @param  [in] isPartialFree  Boolean indicating if want to partially release buffers
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNBUFFERMANAGERDEACTIVATE)(
    CHIBUFFERMANAGERHANDLE hBufferManager,
    BOOL                   isPartialFree);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERBINDBUFFER
///
/// @brief  The implementation for PFNBUFFERMANAGERBINDBUFFER
///
/// @param  [in] hImageBuffer   Image buffer handle
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNBUFFERMANAGERBINDBUFFER)(
    CHIBUFFERHANDLE hImageBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERGETCPUADDRESS
///
/// @brief  The implementation for PFNBUFFERMANAGERGETCPUADDRESS
///
/// @param  [in] hImageBuffer   Image buffer handle
///
/// @return Pointer the to cpu address.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef const VOID* (*PFNBUFFERMANAGERGETCPUADDRESS)(
    CHIBUFFERHANDLE hImageBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERGETFILEDESCRIPTOR
///
/// @brief  The implementation for PFNBUFFERMANAGERGETFILEDESCRIPTOR
///
/// @param  [in] hImageBuffer   Image buffer handle
///
/// @return File Descriptor.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef INT (*PFNBUFFERMANAGERGETFILEDESCRIPTOR)(
    CHIBUFFERHANDLE hImageBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERGETGRALLOCHANDLE
///
/// @brief  The implementation for PFNBUFFERMANAGERGETGRALLOCHANDLE
///
/// @param  [in] hImageBuffer   Image buffer handle
///
/// @return Pointer to buffer_handle_t typecasted to VOID*.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID* (*PFNBUFFERMANAGERGETGRALLOCHANDLE)(
    CHIBUFFERHANDLE hImageBuffer);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNBUFFERMANAGERCACHEOPS
///
/// @brief  The implementation for PFNBUFFERMANAGERCACHEOPS
///
/// @param  [in] hImageBuffer   Image buffer handle
/// @param  [in] invalidate     Invalidate the cache
/// @param  [in] clean          Clean the cache
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNBUFFERMANAGERCACHEOPS)(
    CHIBUFFERHANDLE hImageBuffer,
    BOOL            invalidate,
    BOOL            clean);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiBufferManagerOps
///
/// @brief  This structure contains functions for CHI buffer manager
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ChiBufferManagerOps
{
    PFNBUFFERMANAGERCREATE            pCreate;
    PFNBUFFERMANAGERDESTROY           pDestroy;
    PFNBUFFERMANAGERGETIMAGEBUFFER    pGetImageBuffer;
    PFNBUFFERMANAGERADDREFERENCE      pAddReference;
    PFNBUFFERMANAGERRELEASEREFERENCE  pReleaseReference;
    PFNBUFFERMANAGERGETREFERENCE      pGetReference;
    PFNBUFFERMANAGERACTIVATE          pActivate;
    PFNBUFFERMANAGERDEACTIVATE        pDeactivate;
    PFNBUFFERMANAGERBINDBUFFER        pBindBuffer;
    PFNBUFFERMANAGERGETCPUADDRESS     pGetCPUAddress;
    PFNBUFFERMANAGERGETFILEDESCRIPTOR pGetFileDescriptor;
    PFNBUFFERMANAGERGETGRALLOCHANDLE  pGetGrallocHandle;
    PFNBUFFERMANAGERCACHEOPS          pCacheOps;
    VOID*                             reserved[7];    ///< Reserved for future use. These must be initialized to NULL.
} CHIBUFFERMANAGEROPS;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSettingToken
///
/// @brief  This structure contains token id and the size of the value
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ChiSettingToken
{
    UINT32    id;       ///< id
    UINT32    size;     ///< size
} CHISETTINGTOKEN;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiExtendSettings
///
/// @brief  This structure contains extend settings for the driver
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ChiExtendSettings
{
    UINT32              numTokens;  ///< number of tokens
    CHISETTINGTOKEN*    pTokens;    ///< pointer to tokens
} CHIEXTENDSETTINGS;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiModifySettings
///
/// @brief This structure contains modify settings for the driver
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct ChiModifySettings
{
    CHISETTINGTOKEN     token;  ///< token
    VOID*               pData;  ///< pointer to payload
} CHIMODIFYSETTINGS;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNGETBUFFERMANAGEROPS
///
/// @brief  Gets the CHI buffer manager related function pointers
///
/// @param  [in][out] PFNGETBUFFERMANAGEROPS
///
/// @return None
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNGETBUFFERMANAGEROPS)(CHIBUFFERMANAGEROPS*);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNGETSETTINGS
///
/// @brief  Gets the Driver Settings
///
/// @param  [in][out] CHIEXTENDSETTINGS
/// @param  [in][out] CHIMODIFYSETTINGS
///
/// @return None
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID(*PFNGETSETTINGS)(CHIEXTENDSETTINGS**, CHIMODIFYSETTINGS**);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiOpenContext
///
/// @brief  Creates and returns a CHI context
///
/// @return Chi context pointer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CHIHANDLE (*PFNCHIOPENCONTEXT)();

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCloseContext
///
/// @brief  Close the CHI context
///
/// @param  [in] hChiContext    CHI Context
///
/// @return None
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNCHICLOSECONTEXT)(CHIHANDLE hChiContext);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGetNumCameras
///
/// @brief  Get the number of cameras in the platform
///
/// This function is typically called at the camera service initialization
///
/// @return Number of camera devices in the platform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef UINT (*PFNCHIGETNUMCAMERAS)(CHIHANDLE hChiContext);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiGetCameraInfo
///
/// @brief  Get the information about a specific camera
///
/// @param  [in]  hChiContext   CHI Context
/// @param  [in]  cameraId      Camera identifier
/// @param  [out] pCameraInfo   Pointer to the camera info structure
///
/// @return Success or failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIGETCAMERAINFO)(
    CHIHANDLE      hChiContext,
    UINT32         cameraId,
    CHICAMERAINFO* pCameraInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEnumerateSensorModes
///
/// @brief  Get all the modes information about a specific sensor
///
/// @param  [in]  hChiContext      CHI Context
/// @param  [in]  cameraId         Camera identifier
/// @param  [in]  numSensorModes   Number of sensor modes
/// @param  [in]  pSensorModeInfo  Array of modes with numModes elements in the array filled in by the driver
///
/// @return Success or failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIENUMERATESENSORMODES)(
    CHIHANDLE          hChiContext,
    UINT32             cameraId,
    UINT32             numSensorModes,
    CHISENSORMODEINFO* pSensorModeInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCreatePipelineDescriptor
///
/// @brief  Creates a pipeline for use by either a live or offline processing session
///
/// This function reads a pipeline descriptor, validates the pipeline contained within the descriptor, and then returns
/// a handle to the internal driver representation of the pipeline descriptor. The actual pipeline containing all the nodes
/// is created using this pipeline descriptor only when the session is created. This function can be called at any time
/// after the context is created.
///
/// @param  [in]  hChiContext               CHI Context
/// @param  [in]  pPipelineName             Pipeline name
/// @param  [in]  pDescriptor               Pointer to a descriptor that contains all the necessary information to define a
///                                         processing pipeline.
/// @param  [in]  numOutputs                Number of outputs in the pipeline
/// @param  [in]  pOutputBufferDescriptors  Each output descriptor
/// @param  [in]  numInputs                 Number of input requirements for the pipeline
/// @param  [out] pInputBufferOptions       Each input buffer descriptor
///
/// @return A handle to the created pipeline descriptor, or 0 if it fails
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CHIPIPELINEDESCRIPTOR (*PFNCHICREATEPIPELINEDESCRIPTOR)(
    CHIHANDLE                          hChiContext,
    const CHAR*                        pPipelineName,
    const CHIPIPELINECREATEDESCRIPTOR* pDescriptor,
    UINT32                             numOutputs,
    CHIPORTBUFFERDESCRIPTOR*           pOutputBufferDescriptors,
    UINT32                             numInputs,
    CHIPIPELINEINPUTOPTIONS*           pInputBufferOptions);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDestroyPipelineDescriptor
///
/// @brief  Destroys a pipeline descriptor
///
/// @param  [in] hChiContext            CHI Context
/// @param  [in] hPipelineDescriptor    Pipeline descriptor
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNCHIDESTROYPIPELINEDESCRIPTOR)(
    CHIHANDLE             hChiContext,
    CHIPIPELINEDESCRIPTOR hPipelineDescriptor);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiCreateSession
///
/// @brief  Creates a camera session
///
/// This function creates a session that are independent processing units containing one or multiple pipelines. The
/// pipeline(s) for these processing sessions may get input data from memory(offline) or through streaming(realtime). This
/// function returns a handle, which is used by the driver to identify the processing session.
///
/// @param  [in] hChiContext            CHI Context
/// @param  [in] numPipelines           Number of pipelines associated with this session
/// @param  [in] pPipelineInfo          Pointer to array of structures describing the pipeline info
/// @param  [in] pCallbacks             Pointer to callback provided by the app to return session result/messages
/// @param  [in] pPrivateCallbackData   Private data passed back to the app as part of the callbacks
/// @param  [in] flags                  Session create flags
///
/// @return A handle to the processing session
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CHIHANDLE (*PFNCHICREATESESSION)(
    CHIHANDLE        hChiContext,
    UINT             numPipelines,
    CHIPIPELINEINFO* pPipelineInfo,
    CHICALLBACKS*    pCallbacks,
    VOID*            pPrivateCallbackData,
    CHISESSIONFLAGS  flags);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiActivatePipeline
///
/// @brief  Activate a pipeline in a session
///
/// This function will activate a pipeline in the session after which request submission to the pipeline can begin
///
/// @param  [in] hChiContext    CHI Context
/// @param  [in] hSession       Handle to session
/// @param  [in] pipeline       Pipeline (handle) to activate
/// @param  [in] pModeInfo      New sensor mode info
///
/// @return Success or failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIACTIVATEPIPELINE)(
    CHIHANDLE          hChiContext,
    CHIHANDLE          hSession,
    CHIHANDLE          pipeline,
    CHISENSORMODEINFO* pSensorModeInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDeactivatePipeline
///
/// @brief  Deactivates a pipeline after which no request can be submitted to the pipeline
///
/// @param  [in] hChiContext    CHI Context
/// @param  [in] hSession       Handle to session
/// @param  [in] pipeline       Pipeline (handle) to deactivate
/// @param  [in] mode           Deactivate mode
///
/// @return Success or failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIDEACTIVATEPIPELINE)(
    CHIHANDLE                   hChiContext,
    CHIHANDLE                   hSession,
    CHIHANDLE                   pipeline,
    CHIDEACTIVATEPIPELINEMODE   mode);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIQUERYPIPELINEMETADATAINFO
///
/// @brief  Queries the pipeline specific metadata information
///
/// @param  [in]  hChiContext           CHI Context
/// @param  [in]  hSession              Handle to session
/// @param  [in]  hPipelineDescriptor   CHI Pipeline descriptor
/// @param  [out] pPipelineMetadataInfo Pointer to the structure containing the pipeline metadata information which
///                                     must be filled by Camx
///
/// @return Success or failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFNCHIQUERYPIPELINEMETADATAINFO)(
    CHIHANDLE                   hChiContext,
    CHIHANDLE                   hSession,
    CHIPIPELINEDESCRIPTOR       hPipelineDescriptor,
    CHIPIPELINEMETADATAINFO*    pPipelineMetadataInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDestroySession
///
/// @brief  Destroys an camera session.
///
/// @param  [in] hChiContext    CHI Context
/// @param  [in] session        Session handle
/// @param  [in] isForced       forced flag
///
/// @return None.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNCHIDESTROYSESSION)(
    CHIHANDLE hChiContext,
    CHIHANDLE session,
    BOOL      isForced);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFlushSession
///
/// @brief  Flush a camera session.
///
/// @param  [in] hChiContext    CHI Context
/// @param  [in] hSessionFlushInfo Session Flush Info handle
///
/// @return ChiResultSuccess upon success.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIFLUSHSESSION)(
    CHIHANDLE           hChiContext,
    CHISESSIONFLUSHINFO hSessionFlushInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiSubmitPipelineRequest
///
/// @brief  Submits a request to (multiple) instantiated live or offline pipelines
///
/// @param  [in] hChiContext    CHI Context
/// @param  [in] pRequest       Handle of the offline processing session.
///
/// @return ChiResultSuccess upon success.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHISUBMITPIPELINEREQUEST)(
    CHIHANDLE           hChiContext,
    CHIPIPELINEREQUEST* pRequest);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetSocId
///
/// @brief  Get Platform SOC ID
///
/// @return ChiCameraFamilySoc SocId.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef ChiCameraFamilySoc(*PFNGETSOCID)();

/// @brief CHI Context
typedef struct ChiContextOps
{
    UINT32                          size;                           ///< Size of this structure
    UINT32                          majorVersion;                   ///< Major version of the interface
    UINT32                          minorVersion;                   ///< Minor version of the interface
    UINT32                          subVersion;                     ///< Sub version of the interface
    PFNCHIOPENCONTEXT               pOpenContext;                   ///< OpenContext function
    PFNCHICLOSECONTEXT              pCloseContext;                  ///< CloseContext function
    PFNCHIGETNUMCAMERAS             pGetNumCameras;                 ///< Get number of cameras
    PFNCHIGETCAMERAINFO             pGetCameraInfo;                 ///< Get camera info of a single camera
    PFNCHIENUMERATESENSORMODES      pEnumerateSensorModes;          ///< Enumerate the sensor modes
    PFNCHICREATEPIPELINEDESCRIPTOR  pCreatePipelineDescriptor;      ///< Create a pipeline descriptor
    PFNCHIDESTROYPIPELINEDESCRIPTOR pDestroyPipelineDescriptor;     ///< Destroy a pipeline descriptor
    PFNCHICREATESESSION             pCreateSession;                 ///< Create a session
    PFNCHIDESTROYSESSION            pDestroySession;                ///< Destroy a session
    PFNCHIACTIVATEPIPELINE          pActivatePipeline;              ///< Activate a pipeline contained in a session
    PFNCHIDEACTIVATEPIPELINE        pDeactivatePipeline;            ///< Deactivate a pipeline contained in a session
    PFNCHISUBMITPIPELINEREQUEST     pSubmitPipelineRequest;         ///< Submit a pipeline request
    PFNGETTAGOPS                    pTagOps;                        ///< Vendor Tag Operations
    PFNCHIFLUSHSESSION              pFlushSession;                  ///< Flush session
    PFNCHIQUERYPIPELINEMETADATAINFO pQueryPipelineMetadataInfo;     ///< Queries metadata information specific to pipeline
    PFNGETFENCEOPS                  pGetFenceOps;                   ///< Get Fence Operations
    PFNGETMETADATAOPS               pMetadataOps;                   ///< Metadata Tag Operations
    PFNGETBUFFERMANAGEROPS          pGetBufferManagerOps;           ///< Buffer Manager Operations
    PFNGETSETTINGS                  pGetSettings;                   ///< Get Driver Settings
    PFNGETSOCID                     pGetSocId;                      ///< Get Platform SOD ID
} CHICONTEXTOPS;

/// @brief Sensor PDAF Pixel Coordinates information
typedef struct ChiPDAFPixelCoordinates
{
    UINT32  PDXCoordinate;  ///< PDAF X-Coordinate
    UINT32  PDYCoordinate;  ///< PDAF Y-Coordinate
} CHIPDAFPIXELCOORDINATES;

/// @brief Sensor PDAF information
typedef struct ChiSensorPDAFInfo
{
    UINT32                  PDAFBlockWidth;       ///< PDAF block width
    UINT32                  PDAFBlockHeight;      ///< PDAF block height
    UINT32                  PDAFGlobaloffsetX;    ///< PDAF offset X
    UINT32                  PDAFGlobaloffsetY;    ///< PDAF offset Y
    UINT32                  PDAFPixelCount;       ///< PDAF Pixel count
    ChiPDAFPixelCoordinates PDAFPixelCoords[256]; ///< PDAF Pixel Coordinates
} CHISENSORPDAFINFO;

/// @brief AEC data
typedef struct ChiAECData
{
    UINT64  exposureTime;   ///< Exposure time in nanoseconds
    INT32   sensitivity;    ///< Sensor sensitivity
} CHIAECDATA;

/// @brief Timestamp info
typedef struct ChiTimestampInfo
{
     UINT64 timestamp;
     UINT64 frameId;
} CHITIMESTAMPINFO;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiEntry
///
/// @brief  Main entry point of the CHI driver
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PCHIENTRY)(CHICONTEXTOPS* pContextOps);

CDK_VISIBILITY_PUBLIC VOID ChiEntry(CHICONTEXTOPS* pContextOps);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiDumpState
///
/// @brief  Post-mortem dump function.  Used by tools.  Not intended for use in Chi applications.
///
/// @param  fd    file descriptor in which to write the dump
///
/// @note  Once Chi and below is in a separate .so, this should be a public entry function (like ChiEntry)
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiDumpState(INT fd);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHI_H
