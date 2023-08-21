////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chicommon.h
/// @brief Defines Chi Common Tag interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHICOMMON_H
#define CHICOMMON_H

#include "camxcdktypes.h"
#include "chicommontypes.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/// @brief Pack data structures to ensure consistent layout.
#pragma pack(push, 8)

/// @brief Reserved ChxSettingsToken token ids
enum class ChxSettingsToken
{
    OverrideForceUsecaseId,
    OverrideDisableZSL,
    OverrideGPURotationUsecase,
    OverrideEnableMFNR,
    AnchorSelectionAlgoForMFNR,
    OverrideHFRNo3AUseCase,
    OverrideForceSensorMode,
    DefaultMaxFPS,
    FovcEnable,
    OverrideCameraClose,
    OverrideCameraOpen,
    EISV2Enable,
    EISV3Enable,
    NumPCRsBeforeStreamOn,
    StatsProcessingSkipFactor,
    DumpDebugDataEveryProcessResult,
    Enable3ADebugData,
    EnableConcise3ADebugData,
    EnableTuningMetadata,
    DebugDataSizeAEC,
    DebugDataSizeAWB,
    DebugDataSizeAF,
    ConciseDebugDataSizeAEC,
    ConciseDebugDataSizeAWB,
    ConciseDebugDataSizeAF,
    TuningDumpDataSizeIFE,
    TuningDumpDataSizeIPE,
    TuningDumpDataSizeBPS,
    MultiCameraVREnable,
    OverrideGPUDownscaleUsecase,
    AdvanceFeatureMask,
    DisableASDStatsProcessing,
    MultiCameraFrameSync,
    OutputFormat,
    EnableCHIPartialData,
    EnableFDStreamInRealTime,
    SelectInSensorHDR3ExpUsecase,
    EnableUnifiedBufferManager,
    EnableCHIImageBufferLateBinding,
    EnableCHIPartialDataRecovery,
    UseFeatureForQCFA,
    EnableOfflineNoiseReprocess,
    EnableAsciilog,
    EnableBinarylog,
    OverrideLogLevels,
    EnableRequestMapping,
    EnableSystemLogging,
    EnableFeature2Dump,
    ForceHWMFFixedNumOfFrames,
    ForceSWMFFixedNumOfFrames,
    EnableTBMChiFence,
    EnableRawHDR,
    BPSRealtimeSensorId,
    AECGainThresholdForQCFA,
    EnableMFSRChiFence,
    MultiCameraJPEG,
    MaxHALRequests,
    MultiCameraHWSyncMask,
    AnchorAlgoSelectionType,
    EnableBLMClient,
    OverrideForceBurstShot,
    ExposeFullSizeForQCFA,
    EnableScreenGrab,
    // Add your settings above this comment
    CHX_SETTINGS_TOKEN_COUNT
};

/// @brief This is setup to allow us to duplicate the definitions of metadata for certain pools use
enum class MetadataPoolSection : UINT32
{
    // Result is implicit, and 0x0000
    Input   = 0x0800, // Input must be bitwise exclusive to the other so we can uniquely identify for both tags and props
    Usecase = 0x2000,
    Static  = 0x7000,
    // 0x3000 - Result Prop
    // 0x4000 - Internal Prop
    // 0x5000 - Usecase Prop
    // 0x6000 - DebugData Prop
};

/// Mask to OR with tags to represent input pool residency
static const UINT32 InputMetadataSectionMask   = static_cast<UINT32>(MetadataPoolSection::Input)  << 16;

/// Mask to OR with tags to represent usecase pool residency
static const UINT32 UsecaseMetadataSectionMask = static_cast<UINT32>(MetadataPoolSection::Usecase) << 16;

/// Mask to OR with tags to represent static pool residency
static const UINT32 StaticMetadataSectionMask  = static_cast<UINT32>(MetadataPoolSection::Static) << 16;

/// Maximum number of per-port metadata
static const UINT32 ChiMaxPortMetadata = 10;

/// Invalid port identifier
static const UINT32 InvalidPortId = 0xFFFFFFFF;

typedef VOID*          CHIHANDLE;            ///< Handle to a chi context
typedef VOID*          CHIMETADATAHANDLE;    ///< Handle to a chi meta data
typedef VOID*          CHIFENCEHANDLE;       ///< Handle to a chi fence
typedef VOID*          CHIREQUESTPAYLOAD;    ///< Handle to a request payload
typedef VOID*          CHIBUFFERMANAGERHANDLE;
typedef VOID*          CHIBUFFERHANDLE;

/// @brief Buffer Heap
const UINT32 BufferHeapDefault        = 0xFFFFFFFF;

/// @brief Gralloc Usage flags
typedef UINT32 GrallocUsage;
typedef UINT64 GrallocUsage64;
typedef UINT64 GrallocConsumerUsage;
typedef UINT64 GrallocProducerUsage;

static const GrallocUsage64 GrallocUsageSwReadNever       = 0x00000000; ///< Buffer is never read in software
static const GrallocUsage64 GrallocUsageSwReadRarely      = 0x00000002; ///< Buffer is rarely read in software
static const GrallocUsage64 GrallocUsageSwReadOften       = 0x00000003; ///< Buffer is often read in software
static const GrallocUsage64 GrallocUsageSwReadMask        = 0x0000000F; ///< Mask for the software read values

static const GrallocUsage64 GrallocUsageSwWriteNever      = 0x00000000; ///< Buffer is never written in software
static const GrallocUsage64 GrallocUsageSwWriteRarely     = 0x00000020; ///< Buffer is rarely written in software
static const GrallocUsage64 GrallocUsageSwWriteOften      = 0x00000030; ///< Buffer is often written in software
static const GrallocUsage64 GrallocUsageSwWriteMask       = 0x000000F0; ///< Mask for the software write values

static const GrallocUsage64 GrallocUsageHwTexture         = 0x00000100; ///< Buffer will be used as an OpenGL ES texture
static const GrallocUsage64 GrallocUsageHwRender          = 0x00000200; ///< Buffer will be used as an OpenGL ES render target
static const GrallocUsage64 GrallocUsageHw2D              = 0x00000400; ///< Buffer will be used by the 2D hardware blitter
static const GrallocUsage64 GrallocUsageHwComposer        = 0x00000800; ///< Buffer will be used by the HWComposer module
static const GrallocUsage64 GrallocUsageHwFrameBuffer     = 0x00001000; ///< Buffer will be used with the framebuffer device

static const GrallocUsage64 GrallocUsageExternalDisplay   = 0x00002000; ///< Buffer should be displayed full-screen on an
                                                                        ///  external display when possible

static const GrallocUsage64 GrallocUsageProtected         = 0x00004000; ///< Must have a hardware-protected path to external
                                                                        ///  display sink for this buffer.  If a hardware-
                                                                        ///  protected path is not available, then either don't
                                                                        ///  composite only this buffer (preferred) to the
                                                                        ///  external sink, or (less desirable) do not route
                                                                        ///  the entire composition to the external sink.

static const GrallocUsage64 GrallocUsageCursor            = 0x00008000; ///< Buffer may be used as a cursor

static const GrallocUsage64 GrallocUsageHwVideoEncoder    = 0x00010000; ///< Buffer will be used with the HW video encoder
static const GrallocUsage64 GrallocUsageHwCameraWrite     = 0x00020000; ///< Buffer will be written by the HW camera pipeline
static const GrallocUsage64 GrallocUsageHwCameraRead      = 0x00040000; ///< Buffer will be read by the HW camera pipeline
static const GrallocUsage64 GrallocUsageHwCameraZSL       = 0x00060000; ///< Buffer will be used in zero-shutter-lag queue
static const GrallocUsage64 GrallocUsageHwCameraMask      = 0x00060000; ///< Mask for the camera access values
static const GrallocUsage64 GrallocUsageHwMask            = 0x00071F00; ///< Mask for the software usage bit-mask

static const GrallocUsage64 GrallocUsageRenderScript      = 0x00100000; ///< Buffer will be used as a RenderScript Allocation

static const GrallocUsage   GrallocUsageForeignBuffers    = 0x00200000; ///< Set by the consumer to indicate to the producer
                                                                        ///  that they may attach a buffer that they did not
                                                                        ///  detach from the BufferQueue. Will be filtered out
                                                                        ///  by GrallocUsageAllocMask, so gralloc modules will
                                                                        ///  not need to handle this flag.
static const GrallocUsage64 GrallocUsageHwImageEncoder    = 0x8000000;  ///< Mask for the image encoding HEIF format

static const GrallocUsage GrallocUsageAllocMask         = ~(GrallocUsageForeignBuffers);    ///< Mask of all flags which could
                                                                                            ///  be passed to a gralloc module
                                                                                            ///  for buffer allocation. Any
                                                                                            ///  flags not in this mask do not
                                                                                            ///  need to be handled by gralloc
                                                                                            ///  modules.

static const GrallocUsage64 GrallocUsagePrivateCDSP     = (1ULL << 50); ///< This flag is set while CDSP accesses the buffer



typedef enum ChiMetadataType
{
    ChiMetadataStatic   = 0x1,          ///< Static pool of metadata
    ChiMetadataDynamic  = 0x2,          ///< The non-static metadata
    ChiMetadataEnd      = 0xFFFFFFFF    ///< Mark the end
} CHIMETADATATYPE;

/// @brief Define enum to tag visibility
typedef enum ChiTagSectionVisibility
{
    ChiTagSectionVisibleToOEM       = 0x1,              ///< VenderTag section visible to OEM
    ChiTagSectionVisibleToFramework = 0x2,              ///< VenderTag section visible to Android framework
    ChiTagSectionVisibleToAll       = 0xFFFFFFFF,       ///< VenderTag section visible to All
} CHITAGSECTIONVISIBILITY;

/// @brief Encapsulates essential data describing a vendor tag
typedef struct ChiVendorTagData
{
    const CHAR*     pVendorTagName; ///< The string representation of the vendor tag name
    UINT8           vendorTagType;  ///< The type of the vendor tag
    SIZE_T          numUnits;       ///< The number of units of vendorTagType needed to program this tag
} CHIVENDORTAGDATA;

/// @brief Encapsulates essential data describing a vendor tag section
typedef struct ChiVendorTagSectionData
{
    const CHAR*             pVendorTagSectionName;              ///< The string representing the vendor tag section name
    UINT32                  firstVendorTag;                     ///< The first vendor tag in the vendor tag section
    UINT32                  numTags;                            ///< The number of vendor tags in the section
    CHIVENDORTAGDATA*       pVendorTagaData;                    ///< An array of vendor tag data arrays
    CHITAGSECTIONVISIBILITY visbility;                          ///< Visibility of this tag section
} CHIVENDORTAGSECTIONDATA;

/// @brief Camera  vendor tags capabilities. These capabilities will be used to populate suported vendor tags by node.
typedef struct ChiVendorTagInfo
{
    CHIVENDORTAGSECTIONDATA*    pVendorTagDataArray;             ///< An array of vendor tag section
    UINT32                      numSections;                     ///< The number of vendor tag section in pVendorTagDataArray
} CHIVENDORTAGINFO;

typedef struct ChiQueryVendorTag
{
    UINT32               size;            ///< The size of this structure
    CHIVENDORTAGINFO*    pVendorTagInfo;  ///< Pointer to the vendor tag section
} CHIQUERYVENDORTAG;

/// @brief Defines the data structure for the Chi tag data
typedef struct ChiTagData
{
    UINT32  size;       ///< The size of this structure
    UINT64  requestId;  ///< The request id associated with this tag
    VOID*   pData;      ///< The actual data list
    SIZE_T  dataSize;   ///< The count of data
    UINT64  offset;     ///< The offset from the request id to request data from
    BOOL    negate;     ///< Flag to determine if the offset value should be treated as negative
} CHITAGDATA;

/// @brief Vendor tag base information.
typedef struct ChiVendorTagBaseInfo
{
    UINT32      size;           ///< Size of this structure
    UINT32      vendorTagBase;  ///< Chi assigned runtime vendor tag base for the component
    const CHAR* pComponentName; ///< Name of component associated with the vendor tag base
    const CHAR* pTagName;       ///< The tagName to query, this could be NULL, if it's null,
                                ///  the API just query for the base of giving section(pComponentName)
} CHIVENDORTAGBASEINFO;

/// @brief Metadata list information.
typedef struct ChiMetadataInfo
{
    UINT32              size;        ///< Size of this structure
    CHIHANDLE           chiSession;  ///< Chi driver handle that node can use to callback into Chi
    UINT32              tagNum;      ///< Number of tag in the tagList
    UINT32*             pTagList;    ///< pointer to list of tags
    CHITAGDATA*         pTagData;    ///< pointer to Chi tag data
    CHIMETADATATYPE     metadataType;///< The type of metadata to query for
} CHIMETADATAINFO;

/// @brief Specifies the type of the fence
typedef enum ChiFenceType
{
    ChiFenceTypeInternal = 0,   ///< Represents a fence internal to CHI
    ChiFenceTypeNative,         ///< Represent a native fence
    ChiFenceTypeEGL             ///< Represents an EGL sync object
} CHIFENCETYPE;

typedef struct ChiFenceCreateParams
{
    UINT32              size;           ///< Size of this structure
    CHIFENCETYPE        type;           ///< Type of the fence
    CHAR*               pName;          ///< chifence name
    union
    {
        UINT64          eglSync;        ///< EGL sync object (need to cast to EGLSyncKHR)
        INT             nativeFenceFD;  ///< Native fence file descriptor
    };
} CHIFENCECREATEPARAMS;

/// @brief Request done structure used for callback for a chi fence
typedef struct ChiFenceCallbackInfo
{
    UINT32    size;              ///< Size of this structure
    CHIHANDLE hChiSession;       ///< Chi driver handle that node can use to callback into Chi
    UINT64    frameNum;          ///< Frame number for current request
    INT32     processSequenceId; /// Sequence Id
    VOID*     pUserData;         ///< Userdata payload used for the callback function
} CHIFENCECALLBACKINFO;

/// @brief This enumerates the data request types possible
typedef enum  ChiDataRequestType
{
    ChiFetchData,     ///< Fetch data
    ChiIterateData,   ///< Iterate data
} CHIDATAREQUESTTYPE;

/// @brief This enumerates the data sources available.
typedef enum  ChiDataSource
{
    ChiDataGyro,      ///< Gyro Service
    ChiDataAccel,     ///< Accelerometer Service
    ChiTuningManager, ///< Tuning manager
    ChiDataMax,       ///< Data Source Max
} CHIDATASOURCETYPE;

/// @brief Pixel Format
typedef enum ChiStreamFormat
{
    ChiStreamFormatYCrCb420_SP   = 0x00000113,   ///< YCrCb420_SP is mapped to ChiStreamFormatYCbCr420_888 with ZSL flags
    ChiStreamFormatRaw16         = 0x00000020,   ///< Blob format
    ChiStreamFormatBlob          = 0x00000021,   ///< Carries data which does not have a standard image structure (e.g. JPEG)
    ChiStreamFormatImplDefined   = 0x00000022,   ///< Format is up to the device-specific Gralloc implementation.
    ChiStreamFormatYCbCr420_888  = 0x00000023,   ///< Efficient YCbCr/YCrCb 4:2:0 buffer layout, layout-independent
    ChiStreamFormatRawOpaque     = 0x00000024,   ///< Raw Opaque
    ChiStreamFormatRaw10         = 0x00000025,   ///< Raw 10
    ChiStreamFormatRaw12         = 0x00000026,   ///< Raw 12
    ChiStreamFormatRaw64         = 0x00000027,   ///< Blob format
    ChiStreamFormatUBWCNV124R    = 0x00000028,   ///< UBWCNV12-4R
    ChiStreamFormatNV12HEIF      = 0x00000116,   ///< HEIF video YUV420 format
    ChiStreamFormatNV12UBWCFLEX  = 0x00000126,   ///< Flex NV12 UBWC format
    ChiStreamFormatY8            = 0x20203859,   ///< Y 8
    ChiStreamFormatY16           = 0x20363159,   ///< Y 16
    ChiStreamFormatP010          = 0x7FA30C0A,   ///< P010
    ChiStreamFormatUBWCTP10      = 0x7FA30C09,   ///< UBWCTP10
    ChiStreamFormatUBWCNV12      = 0x7FA30C06,   ///< UBWCNV12
    ChiStreamFormatPD10          = 0x7FA30C08,   ///< PD10
} CHISTREAMFORMAT;

/// @brief Stream Type
typedef enum ChiStreamType
{
    ChiStreamTypeOutput        = 0,                ///< Output stream
    ChiStreamTypeInput         = 1,                ///< Input stream
    ChiStreamTypeBidirectional = 2                 ///< Input and output
} CHISTREAMTYPE;

/// @brief Dataspace of the buffer
typedef enum ChiDataSpace
{
    DataspaceUnknown            = 0x0000,                       ///< Default-assumption data space
    DataspaceArbitrary          = 0x0001,                       ///< Arbitrary dataspace
    DataspaceStandardShift      = 16,                           ///< Standard shift
    DataspaceTransferShift      = 22,                           ///< Transfer shift
    DataspaceRangeShift         = 27,                           ///< Range shift
    DataspaceStandardBT601_625  = 2 << DataspaceStandardShift,  ///< This adjusts the luminance interpretation
                                                                ///  for RGB conversion from the one purely determined
                                                                ///  by the primaries to minimize the color shift into
                                                                ///  RGB space that uses BT.709 primaries.
    DataspaceTransferSmpte170M  = 3 << DataspaceTransferShift,  ///< BT.601 525, BT.601 625, BT.709, BT.2020
    DataspaceTransferSt2084     = 7 << DataspaceTransferShift,  ///< ARIB STD-B67 Hybrid Log Gamma, refer graphics.h
    DataspaceRangeFull          = 1 << DataspaceRangeShift,     ///< Full range uses all values for Y, Cb and Cr from
                                                                ///  0 to 2 ^ b - 1, where b is the bit depth of the
                                                                ///  color format.
    DataspaceJFIF               = 0x0101,                       ///< JPEG File Interchange Format(JFIF). Same model as
                                                                ///  BT.601-625, but all YCbCr values range from 0 to 255.
    DataspaceV0JFIF             = DataspaceStandardBT601_625 |
                                  DataspaceTransferSmpte170M |
                                  DataspaceRangeFull,           ///< JPEG File Interchange Format(JFIF). Same model as
                                                                ///  BT.601-625, but all YCbCr values range from
                                                                ///  0 to 255.
    DataspaceBT601_625          = 0x0102,                       ///< ITU-R Recommendation BT.601 - 625line
    DataspaceBT601_525          = 0x0103,                       ///< ITU-R Recommendation BT.601 - 525line
    DataspaceBT709              = 0x0104,                       ///< ITU-R Recommendation BT.709
    DataspaceSRGBLinear         = 0x0200,                       ///< The red, green, and blue components are stored in sRGB
                                                                ///  space, but are linear and not gamma-encoded.
    DataspaceSRGB               = 0x0201,                       ///< The red, green and blue components are stored in sRGB
                                                                ///  space and converted to linear space
    DataspaceDepth              = 0x1000,                       ///< The buffer contains depth ranging measurements from a
                                                                ///< Depth camera.
    DataspaceJPEGAPPSegments    = 0x1003,                       ///< The buffer contains HEIF Blob with exif and thumbnail info
    DataspaceHEIF               = 0x1004,                       ///< The buffer contains HEIF data
    DataspaceStandardBT2020     = 6 << DataspaceStandardShift,  ///< BT.2020
    DataspaceStandardBT2020_PQ  = (DataspaceStandardBT2020 |    ///< BT.2020-PQ
                                   DataspaceTransferSt2084 |
                                   DataspaceRangeFull),
} CHIDATASPACE;

/// @brief Stream rotation
typedef enum ChiStreamRotation
{
    StreamRotationCCW0      = 0,                ///< No rotation
    StreamRotationCCW90     = 1,                ///< Rotate by 90 degree counterclockwise
    StreamRotationCCW180    = 2,                ///< Rotate by 180 degree counterclockwise
    StreamRotationCCW270    = 3                 ///< Rotate by 270 degree counterclockwise
} CHISTREAMROTATION;

/// @brief This enumerates the data sources configuration.
typedef struct  ChiDataSourceConfig
{
    CHIDATASOURCETYPE sourceType; ///< Data source type
    VOID*             pConfig;    ///< Data source config pointer
} CHIDATASOURCECONFIG;

/// @brief This enumerates the data sources handle
typedef struct ChiDataSourceHandle
{
    CHIDATASOURCETYPE dataSourceType;  ///< Data Source type
    VOID*             pHandle;         ///< Internal data handle
} CHIDATASOURCE;

/// @brief This defines the Chi data request payload structure
typedef struct ChiDataRequest
{
    ChiDataRequestType requestType;   ///< Data request type (fetch/iterate)
    CHIREQUESTPAYLOAD  hRequestPd;    ///< Handle to the request payload
    INT                index;         ///< Index of the data to fetch while iterating through a appropriate data source.
} CHIDATAREQUEST;

/// @brief BufferManager create data structure
struct CHINodeBufferManagerCreateData
{
    UINT32              width;                  ///< Buffer width
    UINT32              height;                 ///< Buffer height
    UINT32              format;                 ///< Buffer format
    UINT64              producerFlags;          ///< Buffer producer gralloc flags
    UINT64              consumerFlags;          ///< Buffer consumer gralloc flags
    UINT32              bufferStride;           ///< Buffer stride
    UINT                maxBufferCount;         ///< The Maximum number of buffers to be allocated
    UINT                immediateBufferCount;   ///< The number of buffers to be allocated immediately
    BOOL                bEnableLateBinding;     ///< Whether enable image buffer late binding for this buffer manager.
                                                ///  Used only if unified buffer manager is used for Chi Buffers
    UINT32              bufferHeap;             ///< Buffer heap decides whether buffers to be allocated through gralloc or csl
};

/// @brief Vendor tag or Camera Tag
typedef struct ChiMetadataId
{
    BOOL isVendorTag;                   ///< Flag to indicate if the metadataId is a vendor tag or camera tag
                                        ///  if TRUE, vendorTag structure will be filled
                                        ///  else, cameraTag will be filled

    union TagIdInfo
    {
        struct VendorTagInfo
        {
            const CHAR* pComponentName; ///< Name of component associated with the vendor tag base
            const CHAR* pTagName;       ///< The tagName to query, this could be NULL, if it's null,
                                        ///  the API just query for the base of giving section(pComponentName)
            UINT32      tagId;          ///< Vendor tag identifier
        } vendorTag;

        UINT32 cameraTag;   ///< Camera Tag Indentifier
    } u;

} CHIMETADATAID;

/// @brief Vendor tag or Camera Tag
typedef struct ChiMetadataIdArray
{
    UINT32         size;                           ///< Size of this structure
    CHIHANDLE      chiSession;                     ///< Chi driver handle that node can use to callback into Chi
    UINT32         tagCount;                       ///< Count of valid data in the tagIdArray
    CHIMETADATAID  tagIdArray[ChiMaxPortMetadata]; ///< Array of tag Ids
} CHIMETADATAIDARRAY;

/// @brief Port Specific Metadata list information.
typedef struct ChiPSMetadata
{
    UINT32              size;               ///< Size of this structure
    CHIHANDLE           chiSession;         ///< Chi driver handle that node can use to callback into Chi
    UINT32              portId;             ///< Port Identifier
    UINT32              tagCount;           ///< Number of tags in pTagList
    UINT32*             pTagList;           ///< Pointer to list of tags
    CHITAGDATA*         pTagData;           ///< Pointer to Chi tag data
} CHIPSMETADATA;

/// @brief Port Specific Metadata list information.
typedef struct ChiPSMetadataBypassInfo
{
    UINT32              size;               ///< Size of this structure
    CHIHANDLE           chiSession;         ///< Chi driver handle that node can use to callback into Chi
    UINT32              portCount;          ///< Number of ports to be bypassed
    UINT32*             pInputPortIds;      ///< Pointer to list of input port ids
    UINT32*             pOutputPortIds;     ///< Pointer to list of output port ids
} CHIPSMETADATABYPASSINFO;

/// @brief Port Specific Tag Info
typedef struct ChiPSTagInfo
{
    UINT                tag;                ///< tag identifier
    CHIHANDLE           chiSession;         ///< chi driver handle that node can use to callback into Chi
} CHIPSTAGINFO;

typedef CHIDATASOURCE* CHIDATASOURCEHANDLE;  ///< Handle to a chi data source

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIFENCECALLBACK
///
/// @brief  Chi fence async wait callback signature
///
/// @param  hChiFence   Handle to Chi fence this callback is called for
/// @param  pUserData   User data provided when waiting on the fence
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID (*PFNCHIFENCECALLBACK)(CHIFENCEHANDLE hChiFence, VOID* pUserData);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIGETMETADATA
///
/// @brief  Gets a list of metadata information based on metadata tags.
///
/// The tag can be an Android tag or a vendor tag. If the metadata information associated with the tag is not published,
/// Chi returns those tags as unpublished when this function returns. The component can add them in the dependency reporting.
///
/// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the component.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIGETMETADATA)(CHIMETADATAINFO* pMetadataInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIGETMULTICAMDYNAMICMETABYCAMID
///
/// @brief  Gets a list of metadata information based on metadata tags for multicamera  based on camera id.
///
/// The tag can be an Android tag or a vendor tag. If the metadata information associated with the tag is not published,
/// Chi returns those tags as unpublished when this function returns. The component can add them in the dependency reporting.
///
/// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the component.
/// @param  cameraId        CameraId of the metadata from which the list of tags must be fetched.This camera id must
///                         be set while posting metadata. InvalidCameraId if dont care. InvalidCameraId if dont care
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIGETMULTICAMDYNAMICMETABYCAMID)(CHIMETADATAINFO* pMetadataInfo, UINT32 cameraId);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIGETSTATICMETADATA
///
/// @brief  Gets a list of static metadata information based on metadata tags.
///
/// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the component.
/// @param  cameraId        Camera id of the static metadata.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult(*PFNCHIGETSTATICMETADATA)(CHIMETADATAINFO* pMetadataInfo, UINT32 cameraId);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHISETMETADATA
///
/// @brief  Sets a list of metadata information based on metadata tags.
///
/// The tag can be an Android tag or a vendor tag. When published, Chi driver will notify all other component that reported
/// these tags as dependencies.
///
/// @param  pMetadataInfo Pointer to a structure that defines the list of metadata information published by the component.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHISETMETADATA)(CHIMETADATAINFO* pMetadataInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIGETVENDORTAGBASE
///
/// @brief  Get the base of vendor tags of other component.
///
/// It serves two purposes.
/// 1. Get the vendor tag base for the tags exported from the chi component.
/// The tag can be from Chi default components or other third-party custom component on which this component is dependent
/// for request processing. The component uses this base and the actual tag offset to derive the tag ID (= base + offset) to
/// report the dependencies.
/// 2. Get the tag value for a specific tag(string).
/// The tag could be any tags populated by other components.
///
/// @param  pVendorTagBaseInfo  Pointer to a structure that defines the run-time Chi assigned vendor tag base.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIGETVENDORTAGBASE)(CHIVENDORTAGBASEINFO* pVendorTagBaseInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIQUERYVENDORTAG
///
/// @brief  Query the vendor tag exported from the Chi node
///
/// It should export the vendor tag supported by componennt. Chi driver appends these vendor tags to overall vendor tag superset.
/// This superset comprises all enumerated vendor tags from other extension components, including Chi default
/// component-supported vendor tags.
///
/// @param  pQueryVendorTag Pointer to a structure that returns the exported vendor tag
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIQUERYVENDORTAG)(CHIQUERYVENDORTAG* pQueryVendorTag);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIPSGETSUPPORTEDMETADATALIST
///
/// @brief  Gets a list of port specific metadata supported by the Driver
///
/// @param  pMetadataIdArray   Pointer to a structure that defines the array of metadata Ids
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIPSGETSUPPORTEDMETADATALIST) (
    CHIMETADATAIDARRAY* pMetadataIdArray);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIPSGETMETADATA
///
/// @brief  Gets a list of port specific metadata information based on metadata tags.
///
/// The tag must be a per-port metadata defined by the driver. This can be queried through PFNCHIGETSUPPORTEDPORTMETADATALIST
/// If the metadata information associated with the tag is not published,
/// Chi returns those tags as unpublished when this function returns. The component can add them in the dependency reporting.
///
/// @param  pMetadataInfo   Pointer to a structure that defines the list of metadata information requested by the component.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIPSGETMETADATA) (
    CHIPSMETADATA* pMetadataInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIPSSETMETADATA
///
/// @brief  Sets a list of port specific metadata information based on metadata tags.
///
/// The tag must be a per-port metadata defined by the driver. This can be queried through PFNCHIGETSUPPORTEDPORTMETADATALIST
/// When published, Chi driver will notify all other component that reported
/// these tags as dependencies.
///
/// @param  pMetadataInfo Pointer to a structure that defines the list of metadata information published by the component.
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIPSSETMETADATA) (
    CHIPSMETADATA* pMetadataInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIPSPUBLISHMETADATA
///
/// @brief  Publishes a list of port specific metadata information based on metadata tags.
///
/// The tag must be a per-port metadata defined by the driver. This can be queried through PFNCHIGETSUPPORTEDPORTMETADATALIST
/// When published, Chi driver will notify all other component that reported
/// these tags as dependencies.
///
/// @param  metadataId   Metadata identifier
/// @param  pBypassInfo  If non-NULL, contains the map of input output ports to fetch the metadata
///
/// @return CDKResultSuccess if success or appropriate error code.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CDKResult (*PFNCHIPSPUBLISHMETADATA) (
    UINT32                   metadataId,
    CHIPSMETADATABYPASSINFO* pBypassInfo);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PFNCHIISPSMMETADATAPUBLISHED
///
/// @brief  Check if port metadata got published
///
/// @param  tag   tag identifier
/// @param  tag   pointer to chi node process request info
///
/// @return TRUE if published, otherwise FALSE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef BOOL (*PFNCHIISPSMETADATAPUBLISHED) (
    CHIPSTAGINFO* pTagInfo);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHICOMMON_H
