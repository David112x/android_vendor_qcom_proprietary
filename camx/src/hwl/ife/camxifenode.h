////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifenode.h
/// @brief IFENode class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIFENODE_H
#define CAMXIFENODE_H

#include "camxcmdbuffermanager.h"
#include "camxcslifedefs.h"
#include "camxcslispdefs.h"
#include "camxhwcontext.h"
#include "camxispiqmodule.h"
#include "camxispstatsmodule.h"
#include "camxmem.h"
#include "camxnode.h"
#include "camxpropertyblob.h"
#include "camxstatsdebugdatawriter.h"
#include "camxtitan17xcontext.h"
#include "camxisppipeline.h"
#include "camxifestripinginterface.h"
#include "NcLibWarp.h"
#include "NcLibWarpCommonDef.h"


CAMX_NAMESPACE_BEGIN

class DualIFEUtils;

static const UINT IFESupportedUBWCVersions2And3 = 1;   ///< Currently this type supports both UBWC 2.0 & 3.0
static const UINT IFESupportedUBWCVersions4     = 2;   ///< Currently this type supports both UBWC 2.0 & 3.0

///< The max below depends on the number of embeddings of DMI/indirect buffers in the command buffer. This value should be
///  calculated based on the design in this node, but upper bound is fine too.
///  @todo(CAMX-5034): IQ & Stats nodes should vote base on the specific needs
static const UINT IFEMaxNumPatches                      = 64;   ///< MaxNumPatches
static const UINT IFEMaxNumNestedAddrsCommon            = 32;   ///< MaxNumNestedAddrs for common modules
static const UINT IFEMaxNumNestedAddrsDualIFESensitive  = 32;   ///< MaxNumNestedAddrs for dual IFE sensitive modules

static const UINT IFEMetadataOutputTags[] =
{
    PropertyIDIFEDigitalZoom,
    PropertyIDIFEAppliedCrop,
    PropertyIDIFEGammaOutput,
    PropertyIDISPBFConfig,
    PropertyIDISPIHistConfig,
    PropertyIDISPAWBBGConfig,
    PropertyIDISPHDRBEConfig,
    PropertyIDISPBHistConfig,
    PropertyIDISPHDRBHistConfig,
    PropertyIDIFEScaleOutput,
    PropertyIDISPTintlessBGConfig,
    PropertyIDISPRSConfig,
    PropertyIDISPCSConfig,
    PropertyIDIFEADRCInfoOutput,
    PropertyIDIPEGamma15PreCalculation,
    ScalerCropRegion,
    ShadingMode,
    StatisticsLensShadingMapMode,
    LensInfoShadingMapSize,
    StatisticsLensShadingMap,
    SensorDynamicBlackLevel,
    SensorDynamicWhiteLevel,
    SensorBlackLevelPattern,
    SensorNeutralColorPoint,
    BlackLevelLock,
    ColorCorrectionGains,
    ControlPostRawSensitivityBoost,
    HotPixelMode,
    NoiseReductionMode,
    StatisticsHotPixelMapMode,
    TonemapMode,
};

static const UINT NumIFEMetadataOutputTags = CAMX_ARRAY_SIZE(IFEMetadataOutputTags);   ///< Number of output vendor tags

static const UINT IFEMetadataRawOutputTags[] =
{
    SensorNeutralColorPoint,
};

static const UINT NumIFEMetadataRawOutputTags = CAMX_ARRAY_SIZE(IFEMetadataRawOutputTags);

// @brief list of vendor tags
static const struct NodeVendorTag IFEOutputVendorTags[] =
{
    { "org.quic.camera.ifecropinfo",    "SensorIFEAppliedCrop"       },
    { "org.quic.camera.ifecropinfo",    "ResidualCrop"               },
    { "org.quic.camera.ifecropinfo",    "AppliedCrop"                },
    { "org.quic.camera.gammainfo"  ,    "GammaInfo"                  },
    { "com.qti.chi.multicamerainfo",    "MultiCameraIds"             },
    { "com.qti.chi.multicamerainfo",    "MasterCamera"               },
    { "com.qti.cropregions"        ,    "crop_regions"               },
    { "org.quic.camera2.ipeicaconfigs", "ICAInGridOut2InTransform"   },
    { "org.quic.camera2.ipeicaconfigs", "ICAInGridIn2OutTransform"   },
    { "com.qti.camera.streamCropInfo",  "StreamCropInfo" },
};

// @brief, index corresponding to above vendor tags
static const UINT32 SensorIFEAppliedCropIndex     = 0;
static const UINT32 ResidualCropIndex             = 1;
static const UINT32 AppliedCropIndex              = 2;
static const UINT32 GammaInfoIndex                = 3;
static const UINT32 MultiCameraIdIndex            = 4;
static const UINT32 MasterCameraIndex             = 5;
static const UINT32 CropRegionsIndex              = 6;
static const UINT32 ICAInGridOut2InTransformIndex = 7;
static const UINT32 ICAInGridIn2OutTransformIndex = 8;
static const UINT32 IFEMinVBI                     = 32;

static const UINT32 IFERequestQueueDepth = DefaultRequestQueueDepth;

static const UINT IFEMaxOutputVendorTags = CAMX_ARRAY_SIZE(IFEOutputVendorTags);

static const UINT IFETotalMetadataTags   = NumIFEMetadataOutputTags +IFEMaxOutputVendorTags;

/// @brief IFE LDC Path
enum IFELDCPath
{
    FullPath        = 0,    ///< Full path
    DisplayFullPath = 1,    ///< Display path
    LDCMaxPath,             ///< Max Path
};

/// @brief List of framework tags
static UINT32 IFEMetadataTags[] =
{
    InputBlackLevelLock                  ,
    InputColorCorrectionGains            ,
    InputColorCorrectionMode             ,
    InputColorCorrectionTransform        ,
    InputControlAEMode                   ,
    InputControlAWBMode                  ,
    InputControlMode                     ,
    InputControlPostRawSensitivityBoost  ,
    InputHotPixelMode                    ,
    InputNoiseReductionMode              ,
    InputShadingMode                     ,
    InputStatisticsHotPixelMapMode       ,
    InputStatisticsLensShadingMapMode    ,
    InputTonemapMode                     ,
    InputScalerCropRegion                ,
    InputControlAWBLock                  ,
    InputControlAELock
};

static const CHAR* IFEInputPortName[] =
{
    "CSIDTPG",          // 0
    "CAMIFTPG",         // 1
    "Sensor",           // 2
    "RDI0",             // 3
    "RDI1",             // 4
    "RDI2",             // 5
    "DualPD",           // 6
    "CustomHW"          // 7
    "Unknown",          // 8 Unknown
};

static const CHAR* IFEOutputPortName[] =
{
    "Full",             // 0
    "DS4",              // 1
    "DS16",             // 2
    "CAMIFRaw",         // 3
    "LSCRaw",           // 4
    "GTMRaw",           // 5
    "FD",               // 6
    "PDAF",             // 7
    "RDI0",             // 8
    "RDI1",             // 9
    "RDI2",             // 10
    "RDI3",             // 11
    "StatsRS",          // 12
    "StatsCS",          // 13
    "Unknown",          // 14
    "StatsIHIST",       // 15
    "StatsBHIST",       // 16
    "StatsHDRBE",       // 17
    "StatsHDRBHIST",    // 18
    "StatsTLBG",        // 19
    "StatsBF",          // 20
    "StatsAWBBG",       // 21
    "DisplayFull",      // 22
    "DisplayDS4",       // 23
    "DisplayDS16",      // 24
    "StatsDualPD",      // 25
    "RDIRD",            // 26
    "LCR",              // 27
    "Unknown",          // 28 Unknown
};

static const UINT IFEInputPortNameMaxIndex  = CAMX_ARRAY_SIZE(IFEInputPortName) - 1;    ///< Number of strings in
                                                                                        ///  IFEInputPortName
static const UINT IFEOutputPortNameMaxIndex = CAMX_ARRAY_SIZE(IFEOutputPortName) - 1;   ///< Number of strings in
                                                                                        ///  IFEOutputPortName

static const UINT NumIFEMetadataTags = CAMX_ARRAY_SIZE(IFEMetadataTags);   ///< Number of vendor tags

// NOWHINE NC003a: Don't actually want this to be g_
static UINT64 IFEMetadataTagReqOffset[NumIFEMetadataTags] = { 0 };

/// @brief IFE Channel Type
enum class IFEChannelType
{
    PIXEL,       ///< Pixel Channel
    RDI,         ///< RDI   Channel
};

/// @brief Identifier for IFE split, especially in dual-IFE Mode
enum class IFESplitID
{
    Left,        ///< Left  IFE
    Right        ///< Right IFE
};

/// @brief Dual IFE constants
static const UINT LeftIFE     = 0;
static const UINT RightIFE    = 1;
static const UINT CommonIFE   = 2;
static const UINT DualIFEMax  = 3;

/// @brief CAMIF Pixel extraction data
struct IFECAMIFPixelExtractionData
{
    UINT32 xCordrinate;      ///< X Coordinate of Pixel
    UINT32 yCordrinate;      ///< Y cooridnate of Pixel
    UINT32 flag;             ///< 0 - Left, 1 - Right
    UINT32 blockX;           ///< block Index
    UINT32 blockY;           ///< block Index
};

/// @brief IFE BW path Grouping
enum IFEBWPathGrouping
{
    DataPathLinear   = CSLAXIPathDataIFELinear,          ///< Linear data Path Used by DS4/Ds16/LCR/FD ports
    DataPathVideo    = CSLAXIPathDataIFEVideo,           ///< Video data Path used by Video full Port
    DataPathDisplay  = CSLAXIPathDataIFEDisplay,         ///< Display data Path used by Display full Port
    DataPathStats    = CSLAXIPathDataIFEStats,           ///< Stats data Path used by Stats Ports
    DataPathRDI0     = CSLAXIPathDataIFERDI0,            ///< RDI data Path used by RDI0
    DataPathRDI1     = CSLAXIPathDataIFERDI1,            ///< RDI data Path used by RDI1
    DataPathRDI2     = CSLAXIPathDataIFERDI2,            ///< RDI data Path used by RDI2
    DataPathRDI3     = CSLAXIPathDataIFERDI3,            ///< RDI data Path used by RDI3
    DataPathPDAF     = CSLAXIPathDataIFEPDAF,            ///< PDAF data Path used by PDAF port
    DataPathPixelRaw = CSLAXIPathDataIFEPixelRaw,        ///< Pixel raw data Path used by Ideal Raw Ports
    DataPathMax      = CSLAXIPathDataIFEPixelRaw + 1,    ///< Max
};

struct IFEStripingInput;

/// @brief ISP input resource information
struct IFEInputResourceInfo
{
    UINT32                 resourceType;       ///< Resource type
    UINT32                 laneType;           ///< Lane type (dphy/cphy)
    UINT32                 laneNum;            ///< Active lane number
    UINT32                 laneConfig;         ///< Lane configurations: 4 bits per lane
    UINT32                 VC[ISPMaxVCConfig]; ///< Virtual Channel
    UINT32                 DT[ISPMaxDTConfig]; ///< Data Type
    UINT32                 numValidVCDT;       ///< Number of valid VC and DT
    UINT32                 format;             ///< Input Image Format
    UINT32                 testPattern;        ///< Test Pattern for the TPG
    UINT32                 usageType;          ///< Single ISP or Dual ISP
    UINT32                 leftStart;          ///< Left input start offset in pixels
    UINT32                 leftStop;           ///< Left input stop offset in pixels
    UINT32                 leftWidth;          ///< Left input Width in pixels
    UINT32                 rightStart;         ///< Right input start offset in pixels
    UINT32                 rightStop;          ///< Right input stop offset in pixels
    UINT32                 rightWidth;         ///< Right input Width in pixels
    UINT32                 lineStart;          ///< Start offset of the pixel line
    UINT32                 lineStop;           ///< Stop offset of the pixel line
    UINT32                 height;             ///< Input height in lines
    UINT32                 pixleClock;         ///< Sensor output clock
    UINT32                 batchSize;          ///< Batch size for HFR mode
    UINT32                 DSPMode;            ///< DSP Mode
    UINT32                 HBICount;           ///< HBI Count
    UINT32                 customNode;         ///< Custom Node
    UINT32                 numberOutResource;  ///< Number of the output resource associated
    UINT32                 offlineMode;        ///< Offline mode
    UINT32                 horizontalBinning;  ///< Horiz binning
    UINT32                 QCFABinning;        ///< Quad CFA binning
};

/// @brief ISP output Resource Information
struct IFEOutputResourceInfo
{
    UINT32 resourceType;       ///< Resource type
    UINT32 format;             ///< Format of the output
    UINT32 width;              ///< Width of the output image
    UINT32 height;             ///< Height of the output image
    UINT32 compositeGroupId;   ///< Composite Group id of the group
    UINT32 splitPoint;         ///< Split point in pixels for Dual ISP case
    UINT32 secureMode;         ///< Output port to be secure or non-secure
    UINT32 wmMode;             ///< WM mode
    UINT32 outPortreserved1;   ///< Reserved field
    UINT32 outPortreserved2;   ///< Reserved field
};

/// @brief IFE CSID Clock to configure CSID HW
struct IFECSIDClockConfig
{
    UINT64  CSIDClockHz;         ///< CSID clock rate
};

CAMX_BEGIN_PACKED

/// @brief Dual IFE split params
struct DualIFESplitParams
{
    UINT32  splitPoint;     ///< x, where (0 < x < width) and left IFE's input ends at x + rightPadding and
                            ///  right IFE's input starts at x - leftPadding (in pixel)
    UINT32  rightPadding;   ///< The padding added past the split point for left IFE's input (in pixel)
    UINT32  leftPadding;    ///< The padding added before the split point for right IFE's input (in pixel)
    UINT32  reserved;       ///< Reserved
} CAMX_PACKED;

/// @brief IFE stripe info per bus client
struct IFEStripeConfig
{
    UINT32  offset;     ///< Start horizontal offset relative to output buffer (in byte)
                        ///  In UBWC mode, this will indicate the H_INIT value (in pixel)
    UINT32  width;      ///< Width of the stripe (in bytes)
    UINT32  tileConfig; ///< UBWC meta tile config (contains partial tile info)
    UINT32  portId;     ///< Reserved
}  CAMX_PACKED;

static const UINT32 CSLMaxNumIFEStripes = 2;    ///< Dual IFE hence 2

/// @brief Dual IFE configuration
struct IFEDualConfig
{
    UINT32              numPorts;       ///< Max number of ports
    UINT32              reserved;       ///< Reserverd
    DualIFESplitParams  splitParams;    ///< Input split
    IFEStripeConfig     stripes[1];     ///< Stripe information: stripes[CSLMaxNumIFEStripes][numPorts][CSLMaxNumPlanes]
} CAMX_PACKED;

/// @brief HFR configuration for a Port
struct IFEPortHFRConfig
{
    UINT32  portResourceId;     ///< Identifies port resource
    UINT32  subsamplePattern;   ///< Subsample pattern. Used in HFR mode. It should be
                                ///  consistent with batchSize and CAMIF programming.
    UINT32  subsamplePeriod;    ///< Subsample period. Used in HFR mode. It should be consistent
                                ///  with batchSize and CAMIF programming.
    UINT32  framedropPattern;   ///< Framedrop pattern.
    UINT32  framedropPeriod;    ///< Framedrop period. Must be multiple of subsamplePeriod if in HFR mode.
    UINT32  reserved;           ///< Reserved for alignment
} CAMX_PACKED;

/// @brief HFR configuration
struct IFEResourceHFRConfig
{
    UINT32              numPorts;           ///< Number of Ports to configure for HFR
    UINT32              reserved;           ///< Reserved for alignment
    IFEPortHFRConfig    portHFRConfig[1];   ///< Starting point for HFR Resource config information
} CAMX_PACKED;

static const UINT32 RDIMaxNum = 4;          ///< Max Number of RDIs

/// @brief IFE Clock and settings to configure the IFE core clocks
struct IFEResourceClockConfig
{
    UINT32  usageType;           ///< Single IFE or Dual IFE mode
    UINT32  numRdi;              ///< Number of RDIs - KMD expects this to match RDIMaxNum
    UINT64  leftPixelClockHz;    ///< Pixel clock for "left" IFE. If Single IFE in use, only this value is populated.
    UINT64  rightPixelClockHz;   ///< Pixel clock for "right" IFE. Only valid if dual IFEs are in use.
    UINT64  rdiClockHz[1];       ///< Clock rate for RDI path
} CAMX_PACKED;

/// @brief IFE Bandwidth votes needed by the IFE for the active use case
struct IFEResourceBWVote
{
    UINT32  usageType;           ///< Single IFE or Dual IFE mode
    UINT32  reserved;            ///< Number of RDIs
    UINT64  camnocBWbytes;       ///< Uncompressed BW in bytes required within CAMNOC
    UINT64  externalBWbytes;     ///< BW in bytes required outside of CAMNOC (MNOC etc). Accounts for compression if enabled.
} CAMX_PACKED;

/// @brief IFE Bandwidth votes needed by the IFE for the active use case
struct IFEResourceBWConfig
{
    UINT32             usageType;        ///< Single IFE or Dual IFE mode
    UINT32             numRdi;           ///< Number of RDIs
    IFEResourceBWVote  leftPixelVote;    ///< Bandwidth vote for "left" IFE, valid for single IFE
    IFEResourceBWVote  rightPixelVote;   ///< Bandwidth vote for "right" IFE, valid for dual IFEs
    IFEResourceBWVote  rdiVote[4];       ///< Bandwidth vote for RDI path
} CAMX_PACKED;

/// @brief IFE Bandwidth votes needed by the IFE for the active use case
struct IFEResourceBWConfigVer2
{
    UINT32              usageType;                                  ///< Single IFE or Dual IFE mode
    UINT32              numPaths;                                   ///< Number of AXI Data paths
    CSLAXIperPathBWVote outputPathBWInfo[2 * DataPathMax];         ///< Per Path Vote Info
} CAMX_PACKED;

static const UINT32 BusReadMinimumHBI = 64;    ///< Minimum HBI for timing generator in Read Path
static const UINT32 BusReadMinimumVBI = 4;     ///< Minimum VBI for timing generator in Read Path (counted as VBI*8 in h/w)

/// @brief IFE Bus Read configuration
struct IFEBusReadConfig
{
    UINT64 version;            ///< version
    UINT32 VBICount;           ///< VBI after fetching every frame from bus read (minimum - 32 lines)
    UINT32 FSMode;             ///< Enable/Disable FS mode
    UINT32 lineSyncEnable;     ///< Specify Line/Frame Sync 0 - Frame based, 1 - Line based
    UINT32 HBICount;           ///< HBI after fetching every line from bus read (minimum - 64 clock cycles)
    UINT32 syncEnable;         ///< FS sync enable/disable, 0 - frame mode
    UINT32 goCmdSelect;        ///< h/w based go_cmd select
    UINT32 clientEnable;       ///< enable read path
    UINT32 srcAddr;            ///< bus read address
    UINT32 width;              ///< width of the line to read
    UINT32 height;             ///< height of the frame to read
    UINT32 stride;             ///< stride of the line to read
    UINT32 fmt;                ///< fmt
    UINT32 unpackMode;         ///< unpack format during read
    UINT32 latencyBufSize;     ///< information to CAMNOC
} CAMX_PACKED;

CAMX_STATIC_ASSERT_MESSAGE((sizeof(IFEDualConfig) % 8) == 0,          "IFEDualConfig 64b-aligned");
CAMX_STATIC_ASSERT_MESSAGE((sizeof(IFEPortHFRConfig) % 8) == 0,       "IFEPortHFRConfig 64b-aligned");
CAMX_STATIC_ASSERT_MESSAGE((sizeof(IFEResourceHFRConfig) % 8) == 0,   "IFEResourceHFRConfig 64b-aligned");
CAMX_STATIC_ASSERT_MESSAGE((sizeof(IFEResourceClockConfig) % 8) == 0, "IFEResourceClockConfig 64b-aligned");
CAMX_STATIC_ASSERT_MESSAGE((sizeof(IFEResourceBWVote) % 8) == 0,       "IFEResourceBWVote 64b-aligned");
CAMX_STATIC_ASSERT_MESSAGE((sizeof(IFEResourceBWConfig) % 8) == 0,    "IFEResourceBWConfig 64b-aligned");
CAMX_STATIC_ASSERT_MESSAGE((sizeof(IFEBusReadConfig) % 8) == 0,       "IFEBusReadConfig 64b-aligned");

CAMX_END_PACKED

/// @brief IFE IO Buffer Configuration Info
struct IFEConfigIOBufInfo
{
    UINT32               portId;                                    ///< IFE PortID
    CSLMemHandle         hMemHandle;                                ///< IFE Buffer memory Handle
    CSLFence             hFence;                                    ///< IFE Buffer Fence Handle
};

/// @brief Dual IFE Configuration Info
struct DualIFEConfig
{
    IFEStripingInput                  stripingInput;                       ///< Striping library's input
    IFEStripingPassOutput             passOut;                             ///< Striping library's output
    IFEStripeInterfaceOutput          stripeOutput[2];                     ///< Striping Library interface output
};

/// @brief IFE Configuration Info
struct IFEConfigInfo
{
    BOOL                              isValid;                             ///< Validity of IFE config Data
    UINT64                            requestID;                           ///< Request ID for IFE config
    BOOL                              forceTriggerUpdate;                  ///< Force Trigger Update
    ChiTuningModeParameter            tuningData;                          ///< Cached tuning mode selector data
                                                                           ///  tuning data (tree) search in the IQ modules
    ISPHALTagsData                    HALTagsData;                         ///< HAL framework tag
    ISPFrameInternalData              ISPFrameData;                        ///< Frame-level ISP internal data
    ISPStripeConfig                   frameConfig;                         ///< FrameLevel config
    ISPInternalData                   ISPData[DualIFEMax];                 ///< Data Calculated by IQ Modules (common,
                                                                           ///< left and right)
    ISPInternalData                   ISPFramelevelData;                   ///< Frame-level data (used for striping)
    DualIFEConfig                     dualIFEConfigData;                   ///< Dual IFE Config Data
    UINT32                            numIOConfig;                         ///< IFE buffer io config in number
    IFEConfigIOBufInfo                ioConfig[MaxDefinedIFEOutputPorts];  ///< IFE buffer io config
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the IFE node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFENode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create IFENode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete IFENode object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static IFENode* Create(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  This method destroys the derived instance of the interface
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

protected:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetProducerFormatParameters
    ///
    /// @brief  Set Producer format param info
    ///
    /// @param  pFormat             Format pointer
    /// @param  pFormatParamInfo    pointer to FormatParamInfo
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetProducerFormatParameters(
        ImageFormat*     pFormat,
        FormatParamInfo* pFormatParamInfo)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, " format %d, version %d, lossy %d",
                         pFormat->format,
                         m_capability.UBWCSupportedVersionMask,
                         m_capability.UBWCLossySupport);
        if (TRUE == ImageFormatUtils::IsUBWC(pFormat->format))
        {
            BOOL is10BitFormat = ImageFormatUtils::Is10BitUBWCFormat(pFormat->format);
            pFormatParamInfo->LossyUBWCProducerUsage &=
                (((TRUE == is10BitFormat)                           &&
                (pFormat->width >= m_capability.lossy10bitWidth)    &&
                (pFormat->height >= m_capability.lossy10bitHeight)) ||
                ((pFormat->width >= m_capability.lossy8bitWidth)    &&
                (pFormat->height >= m_capability.lossy8bitHeight)))
                ? m_capability.UBWCLossySupport : UBWCLossless;
            pFormatParamInfo->UBWCVersionProducerUsage &= m_capability.UBWCSupportedVersionMask;
        }
        else
        {
            pFormatParamInfo->LossyUBWCProducerUsage   &= 0;
            pFormatParamInfo->UBWCVersionProducerUsage &= 0;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupISP, " format %d, producer : lossy %d, version %d",
                         pFormat->format,
                         pFormatParamInfo->LossyUBWCProducerUsage,
                         pFormatParamInfo->UBWCVersionProducerUsage);

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeInitialize
    ///
    /// @brief  Initialize the hwl object
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeInitialize(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFinalizeInitialization
    ///
    /// @brief  Method to finalize the initialization of the node in the pipeline
    ///
    /// @param  pFinalizeInitializationData Finalize data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeFinalizeInitialization(
        FinalizeInitializationData* pFinalizeInitializationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostPipelineCreate
    ///
    /// @brief  virtual method to be called at NotifyTopologyCreated time; node should take care of updates and initialize
    ///         blocks that has dependency on other nodes in the topology at this time.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PostPipelineCreate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFinalizeInputRequirement
    ///
    /// @brief  Virtual method implemented by IFE node to determine its input buffer requirements based on all the output
    ///         buffer requirements
    ///
    /// @param  pBufferNegotiationData  Negotiation data for all output ports of a node
    ///
    /// @return Success if the negotiation was successful, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeFinalizeInputRequirement(
        BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FinalizeBufferProperties
    ///
    /// @brief  Finalize the buffer properties of each output port
    ///
    /// @param  pBufferNegotiationData Buffer negotiation data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID FinalizeBufferProperties(
        BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Pure virtual method to trigger process request for the hwl node object.
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ExecuteProcessRequest(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareStreamOn
    ///
    /// @brief  virtual method to that will be called before streamOn command is sent to HW. HW nodes may use
    ///         this hook to do any preparation, or per-configure_stream one-time configuration.
    ///         This is generally called in FinalizePipeline, i.e within a lifetime of pipeline, this is called only once.
    ///         Actual StreamOn may happen much later based on Activate Pipeline. Nodes can use this to do any one time
    ///         setup that is needed before stream.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PrepareStreamOn();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireResources
    ///
    /// @brief  Method that is called by topology before streamOn. This generally happens in Activate Pipeline.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult AcquireResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseResources
    ///
    /// @brief  virtual method to that will be called after streamOff command is sent to HW. HW nodes may use
    ///         this hook to do any post stream off actions. This is generally called everytime De-activatePipeline is called.
    ///         Nodes may use this to release hardware.
    ///
    /// @param  modeBitmask Deactivate pipeline mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ReleaseResources(
       CHIDEACTIVATEPIPELINEMODE modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnStreamOff
    ///
    /// @brief  virtual method to that will be called before streamOff command is sent to HW. HW nodes may use
    ///         this hook to do any preparation. This is generally called on every Deactivate Pipeline.
    ///         Nodes may use this to release things that are not required at the end of streaming. For exa, any resources
    ///         that are not needed after stream-on can be released here. Make sure to do light weight operations here as
    ///         releasing here may result in re-allocating resources in OnStreamOn.
    ///
    /// @param  modeBitmask Stream off mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult OnStreamOff(
        CHIDEACTIVATEPIPELINEMODE modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputPortName
    ///
    /// @brief  Get input port name for the given port id.
    ///
    /// @param  portId  Port Id for which name is required
    ///
    /// @return Pointer to Port name string
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual const CHAR* GetInputPortName(
        UINT portId) const
    {
        if (portId > IFEInputPortNameMaxIndex)
        {
            portId = IFEInputPortNameMaxIndex;
        }

        return IFEInputPortName[portId];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPortName
    ///
    /// @brief  Get output port name for the given port id.
    ///
    /// @param  portId  Port Id for which name is required
    ///
    /// @return Pointer to Port name string
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual const CHAR* GetOutputPortName(
        UINT portId) const
    {
        if (portId > IFEOutputPortNameMaxIndex)
        {
            portId = IFEOutputPortNameMaxIndex;
        }

        return IFEOutputPortName[portId];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFENode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFENode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFENode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFENode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataPublishList
    ///
    /// @brief  Method to query the publish list from the node
    ///
    /// @param  pPublistTagList List of tags published by the node
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult QueryMetadataPublishList(
        NodeMetadataList* pPublistTagList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryCDMDMIType
    ///
    /// @brief  Query the CDMDMI type based on specific titan version
    ///
    /// @param  titanVersion Titan version of the device
    ///
    /// @return CmdType for the device
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CmdType QueryCDMDMIType(
        UINT32 titanVersion);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckToUseHardcodedRegValues
    ///
    /// @brief  Check and [en|dis]able the use of hardcoded register values
    ///
    /// @param  pHwContext Pointer to the ISP HW context
    ///
    /// @return TRUE/FALSE   TRUE -> Enabled, FALSE -> Disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL CheckToUseHardcodedRegValues(
        HwContext* pHwContext)
    {
        Titan17xContext* pContext = static_cast<Titan17xContext*>(pHwContext);
        CAMX_ASSERT(NULL != pContext);

        return pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->IFEUseHardcodedRegisterValues;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckToEnableDualIFEStripeInfo
    ///
    /// @brief  Check and [en|dis]able the use of hardcoded register values
    ///
    /// @param  pHwContext Pointer to the ISP HW context
    ///
    /// @return TRUE/FALSE   TRUE -> Enabled, FALSE -> Disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL CheckToEnableDualIFEStripeInfo(
        HwContext* pHwContext)
    {
        Titan17xContext* pContext = static_cast<Titan17xContext*>(pHwContext);
        CAMX_ASSERT(NULL != pContext);

        return pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableIFEDualStripeInfo;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsIFESettingsDumpEnabled
    ///
    /// @brief  Checks if IFE Settings Dump is enabled
    ///
    /// @param  pHwContext Pointer to the ISP HW context
    ///
    /// @return TRUE/FALSE   TRUE -> Enabled, FALSE -> Disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsIFESettingsDumpEnabled(
        HwContext* pHwContext)
    {
        Titan17xContext* pContext = static_cast<Titan17xContext*>(pHwContext);
        CAMX_ASSERT(NULL != pContext);

        return pContext->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableIFESettingsDump;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NewActiveStreamsSetup
    ///
    /// @brief  When Pipeline receives a different set of enabled streams for a request as compared to the previous request,
    ///         it informs the node about it in. This function does the processing relating to the streams changing from
    ///         one request to another.
    ///
    /// @param  activeStreamIdMask  Active stream mask
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID NewActiveStreamsSetup(
        UINT activeStreamIdMask);

private:
    IFENode(const IFENode&) = delete;                 ///< Disallow the copy constructor.
    IFENode& operator=(const IFENode&) = delete;      ///< Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsIQModuleInstalled
    ///
    /// @brief  Check if the given module is installed and applicable to the current IFE HW support
    ///
    /// @param  pIQModuleInfo   the module info to check
    ///
    /// @return TRUE if the given module is installed and HW supported, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsIQModuleInstalled(
        const IFEIQModuleInfo* pIQModuleInfo) const
    {
        return pIQModuleInfo->installed;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorAspectRatioMode
    ///
    /// @brief  Helper method to get sensor Aspect Ratio for current sensor mode
    ///
    /// @return Sensor Aspect Ratio mode
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetSensorAspectRatioMode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireDevice
    ///
    /// @brief  Helper method to acquire IFE device
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AcquireDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseDevice
    ///
    /// @brief  Helper method to release IFE device
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsStatsOutputPort
    ///
    /// @brief  Helper method to determine if the output port is a stats or not.
    ///
    /// @param  outputPortId OutputportId to check
    ///
    /// @return TRUE if the output port is a stats port, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsStatsOutputPort(
        UINT outputPortId
        ) const
    {
        BOOL isStatsPort = FALSE;

        switch (outputPortId)
        {
            case IFEOutputPortStatsRS:
            case IFEOutputPortStatsCS:
            case IFEOutputPortStatsIHIST:
            case IFEOutputPortStatsBHIST:
            case IFEOutputPortStatsHDRBE:
            case IFEOutputPortStatsHDRBHIST:
            case IFEOutputPortStatsTLBG:
            case IFEOutputPortStatsBF:
            case IFEOutputPortStatsAWBBG:
                isStatsPort = TRUE;
                break;

            default:
                break;
        }

        return isStatsPort;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDSOutputPort
    ///
    /// @brief  Helper method to determine if the output port is a stats or DS port.
    ///
    /// @param  outputPortId OutputportId to check
    ///
    /// @return TRUE if the output port is a stats port, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsDSOutputPort(
        UINT outputPortId
        ) const
    {
        BOOL isDSPort = FALSE;

        switch (outputPortId)
        {
            case IFEOutputPortDS4:
            case IFEOutputPortDS16:
            case IFEOutputPortDisplayDS4:
            case IFEOutputPortDisplayDS16:
                isDSPort = TRUE;
                break;

            default:
                break;
        }

        return isDSPort;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnableCSIDCropOverridingForSingleIFE
    ///
    /// @brief  Helper method to determine if CSID crop override mode is enabled
    ///
    /// @return TRUE if the CSID crop override mode is enabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL EnableCSIDCropOverridingForSingleIFE() const
    {
        BOOL enableCSIDCropOverriding = FALSE;

        if ((m_instanceProperty.IFECSIDWidth > 0) && (m_instanceProperty.IFECSIDHeight> 0))
        {
            enableCSIDCropOverriding = TRUE;
        }

        return enableCSIDCropOverriding;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPixelOutputPortSourceType
    ///
    /// @brief  Helper method to determine if the output port is a pixel or an undefined port source type
    ///
    /// @param  outputPortId OutputportId to check
    ///
    /// @return TRUE if the output port is a pixel or an undefined port source type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPixelOutputPortSourceType(
        UINT outputPortId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslateCSIDataTypeToCSIDecodeFormat
    ///
    /// @brief  Helper method to map CSI DataType (DT) information to CSI decode format information for ISP.
    ///
    /// @param  CSIDataType the CSI data type
    ///
    /// @return CSID decode format
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8 TranslateCSIDataTypeToCSIDecodeFormat(
        const UINT8 CSIDataType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslateBitDepthToCSIDecodeFormat
    ///
    /// @brief  Helper method to map plain bit width to CSI decode format information for ISP.
    ///
    /// @param  bitWidth    the MIPI bit width
    ///
    /// @return CSID decode format
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8 TranslateBitDepthToCSIDecodeFormat(
        const UINT32 bitWidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslateSensorStreamConfigTypeToPortSourceType
    ///
    /// @brief  Helper method to map sensor stream type to port source type ID
    ///
    /// @param  streamType  the sensor stream type to be translated.
    ///
    /// @return port source type ID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT TranslateSensorStreamConfigTypeToPortSourceType(
        StreamType streamType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslatePortSourceTypeToSensorStreamConfigType
    ///
    /// @brief  Helper method to map port source type ID to sensor stream type
    ///
    /// @param  portSourceTypeId Port source type to translate
    ///
    /// @return Stream type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 TranslatePortSourceTypeToSensorStreamConfigType(
        UINT portSourceTypeId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslateColorFilterPatternToISPPattern
    ///
    /// @brief  Helper method to map sensor color filter arrangement value to ISP Bayer pattern
    ///
    /// @param  colorFilterArrangementValue the sensor provided color filter arrangement value
    ///
    /// @return ISP Bayer pattern value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 TranslateColorFilterPatternToISPPattern(
        const enum SensorInfoColorFilterArrangementValues colorFilterArrangementValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FindSensorStreamConfigIndex
    ///
    /// @brief  Find the index of the given sensor stream config for the given type
    ///
    /// @param  streamType      the sensor stream type to find
    /// @param  pStreamIndex    the sensor stream index found, or pass NULL if only the index is not needed.
    ///
    /// @return TRUE if the given stream type is found in in the availabe sensor config streams
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL FindSensorStreamConfigIndex(
        StreamType  streamType,
        UINT*       pStreamIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckOutputPortIndexIfUnsupported
    ///
    /// @brief  Check if the given output port index needs to be disabled if the current sensor mode does not support it
    ///
    /// @param  outputPortIndex the given output port index
    ///
    /// @return true of the given output port index needs to be disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckOutputPortIndexIfUnsupported(
        UINT outputPortIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFSensorType
    ///
    /// @brief  Get the PDAF Sensor type.
    ///
    /// @return the current PDAF Sensor type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PDLibSensorType GetPDAFSensorType();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckIfPDAFType3Supported
    ///
    /// @brief  Check if the current sensor mode support PDAF type 3.
    ///
    /// @return true if the current sensor mode support PDAF type 3
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckIfPDAFType3Supported();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetRDIOutputPortFormat
    ///
    /// @brief  Set the RDI output based on the port source type.
    ///
    /// @param  pOutputResource     the pointer to the RDI output resource to configure
    /// @param  format              the format from image format type
    /// @param  outputPortId        the given output port Id
    /// @param  portSourceTypeId    the the port source type Id
    ///
    /// @return CamxResultSuccess if successful, otherwise return the failure result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetRDIOutputPortFormat(
        IFEOutputResourceInfo* pOutputResource,
        Format                 format,
        UINT                   outputPortId,
        UINT                   portSourceTypeId);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HasOutputPortForIQModulePathConfig
    ///
    /// @brief  Check if the matching output port for a given pipeline path exist in topology
    ///
    /// @param  pipelinePath    the pipeline path to check against if the output of the path exists in current usecase
    ///
    /// @return TRUE for common path or other pipeline paths that the outport defined in this usecase, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL HasOutputPortForIQModulePathConfig(
        IFEPipelinePath pipelinePath) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateIFECapabilityBasedOnCameraPlatform
    ///
    /// @brief  Set up IFE capability based on IFE revision number
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateIFECapabilityBasedOnCameraPlatform();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureIFECapability
    ///
    /// @brief  Set up IFE capability based on IFE revision number
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureIFECapability();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateIFEIQModules
    ///
    /// @brief  Create IQ Modules of the IFE Block
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateIFEIQModules();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateIFEStatsModules
    ///
    /// @brief  Create Stats Modules of the IFE Block
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateIFEStatsModules();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateIFEHVXModules
    ///
    /// @brief  Create Stats Modules of the IFE Block
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateIFEHVXModules();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIFEIQModulesOfType
    ///
    /// @brief  Get IFE IQ module
    ///
    /// @param  moduleType Module type
    ///
    /// @return The IFE IQ module info filled with the information for the given IQ type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEIQModuleInfo GetIFEIQModulesOfType(
        ISPIQModuleType   moduleType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIFESWTMCModuleInstanceVersion
    ///
    /// @brief  Helper function to get IFE SW TMC installed for the current IFE node
    ///
    /// @return Version of SW TMC module
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SWTMCVersion GetIFESWTMCModuleInstanceVersion();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareStripingParameters
    ///
    /// @brief  Prepare input parameters for striping lib
    ///
    /// @param  pInputData Pointer to the input data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    CamxResult PrepareStripingParameters(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProgramIQConfig
    ///
    /// @brief  Reprogram the settings for the IQ Modules
    ///
    /// @param  pInputData Pointer to the input data
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProgramIQConfig(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitialCAMIFConfig
    ///
    /// @brief  Set all the output paths that are enabled
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitialCAMIFConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProgramIQEnable
    ///
    /// @brief  Helper to program enable bits for the IQ Modules
    ///
    /// @param  pInputData Pointer to the input data
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProgramIQEnable(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateGeneralTuningMetadata
    ///
    /// @brief  Helper to populate tuning data easily obtain at IFE node level
    ///
    /// @param  pInputData Pointer to the data input and output stuctures are inside
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateGeneralTuningMetadata(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpTuningMetadata
    ///
    /// @brief  Helper to publish tuning metadata
    ///
    /// @param  pInputData Pointer to the input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpTuningMetadata(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProgramStripeConfig
    ///
    /// @brief  Program output split info for dual IFE stripe
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProgramStripeConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticMetadata
    ///
    /// @brief  Get all static information from HAL metadata tags for all IFE modules
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetStaticMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataTags
    ///
    /// @brief  Get all information from HAL metadata tags for all IFE modules
    ///
    /// @param  pModuleInput Pointer to the input data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetMetadataTags(
        ISPInputData* pModuleInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareBFStatsMetadata
    ///
    /// @brief  Prepare BF stats metadata to be posted to property pool
    ///
    /// @param  pInputData  Pointer to internal ife data
    /// @param  pMetadata   Pointer to metadata to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareBFStatsMetadata(
        const ISPInputData* pInputData,
        PropertyISPBFStats* pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareHDRBEStatsMetadata
    ///
    /// @brief  Prepare HDR BE stats metadata to be posted to property pool
    ///
    /// @param  pInputData  Pointer to internal ife data
    /// @param  pMetadata   Pointer to metadata to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareHDRBEStatsMetadata(
        const ISPInputData*     pInputData,
        PropertyISPHDRBEStats*  pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareAWBBGStatsMetadata
    ///
    /// @brief  Prepare AWB BG stats metadata to be posted to property pool
    ///
    /// @param  pInputData  Pointer to internal ife data
    /// @param  pMetadata   Pointer to metadata to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareAWBBGStatsMetadata(
        const ISPInputData*     pInputData,
        PropertyISPAWBBGStats*  pMetadata);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareCSStatsMetadata
    ///
    /// @brief  Prepare CS stats metadata to be posted to property pool
    ///
    /// @param  pInputData  Pointer to internal ife data
    /// @param  pMetadata   Pointer to metadata to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareCSStatsMetadata(
        const ISPInputData* pInputData,
        PropertyISPCSStats* pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareRSStatsMetadata
    ///
    /// @brief  Prepare RS stats metadata to be posted to property pool
    ///
    /// @param  pInputData  Pointer to internal ife data
    /// @param  pMetadata   Pointer to metadata to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareRSStatsMetadata(
        const ISPInputData* pInputData,
        PropertyISPRSStats* pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareIHistStatsMetadata
    ///
    /// @brief  Prepare Ihist stats metadata to be posted to property pool
    ///
    /// @param  pInputData  Pointer to internal ife data
    /// @param  pMetadata   Pointer to metadata to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareIHistStatsMetadata(
        const ISPInputData*     pInputData,
        PropertyISPIHistStats*  pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareBHistStatsMetadata
    ///
    /// @brief  Prepare Bhist stats metadata to be posted to property pool
    ///
    /// @param  pInputData  Pointer to internal ife data
    /// @param  pMetadata   Pointer to metadata to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareBHistStatsMetadata(
        const ISPInputData*     pInputData,
        PropertyISPBHistStats*  pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareHDRBHistStatsMetadata
    ///
    /// @brief  Prepare HDR Bhist stats metadata to be posted to property pool
    ///
    /// @param  pInputData  Pointer to internal ife data
    /// @param  pMetadata   Pointer to metadata to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareHDRBHistStatsMetadata(
        const ISPInputData*         pInputData,
        PropertyISPHDRBHistStats*   pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareTintlessBGStatsMetadata
    ///
    /// @brief  Prepare Tintless BG stats metadata to be posted to property pool
    ///
    /// @param  pInputData  Pointer to internal ife data
    /// @param  pMetadata   Pointer to metadata to be updated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareTintlessBGStatsMetadata(
        const ISPInputData*     pInputData,
        PropertyISPTintlessBG*  pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostMetadata
    ///
    /// @brief  Post IFE metadata to main metadata pool
    ///
    /// @param  pInputData Pointer to the input data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PostMetadata(
        const ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostMetadataRaw
    ///
    /// @brief  Post IFE metadata to main metadata pool
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PostMetadataRaw();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareIFEProperties
    ///
    /// @brief  Prepare IFE properties for posting to metadata
    ///
    /// @param  pInputData      Pointer to the input data
    /// @param  pVendorTag      Pointer to all the vendor that need to be published
    /// @param  ppData          Pointer to data to fill out
    /// @param  pDataCount      Pointer to the data count
    /// @param  pDataIndex      Pointer to the index into pData array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareIFEProperties(
        const ISPInputData* pInputData,
        UINT*               pVendorTag,
        const VOID**        ppData,
        UINT*               pDataCount,
        UINT*               pDataIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareIFEHALTags
    ///
    /// @brief  Prepare IFE metadata tags to HAL
    ///
    /// @param  pInputData      Pointer to the input data
    /// @param  pVendorTag      Pointer to all the vendor that need to be published
    /// @param  ppData          Pointer to data to fill out
    /// @param  pDataCount      Pointer to the data count
    /// @param  pDataIndex      Pointer to the index into pData array
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PrepareIFEHALTags(
        const ISPInputData* pInputData,
        UINT*               pVendorTag,
        const VOID**        ppData,
        UINT*               pDataCount,
        UINT*               pDataIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareIFEVendorTags
    ///
    /// @brief  Prepare IFE vendor tag data to main metadata pool
    ///
    /// @param  pInputData      Pointer to the input data
    /// @param  pVendorTag      Pointer to all the vendor that need to be published
    /// @param  ppData          Pointer to data to fill out
    /// @param  pDataCount      Pointer to the data count
    /// @param  pDataIndex      Pointer to the index into pData array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareIFEVendorTags(
        const ISPInputData* pInputData,
        UINT*               pVendorTag,
        const VOID**        ppData,
        UINT*               pDataCount,
        UINT*               pDataIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ComputeNeutralPoint
    ///
    /// @brief  Post IFE Sensor Neutral Point to metadata tag to HAL
    ///
    /// @param  pInputData      Pointer to the input data
    /// @param  pNeutralPoint   Pointer to the computed neutral point
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ComputeNeutralPoint(
        const ISPInputData* pInputData,
        Rational*           pNeutralPoint);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateIQCmdSize
    ///
    /// @brief  Calculate Maximum Command Buffer size required for all the available IQ Modules
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculateIQCmdSize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Cleanup
    ///
    /// @brief  Clean up the allocated IQ modules and Stats Modules
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Cleanup();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOEMIQSettings
    ///
    /// @brief  Get IQ Settings configured by OEMs
    ///
    /// @param  ppOEMData   Pointer to the OEM data to be returned
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetOEMIQSettings(
        VOID** ppOEMData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOEMStatsConfig
    ///
    /// @brief  Returns OEM's custom stats configuration
    ///
    /// @param  pFrameConfig     Pointer to the Frame config data
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetOEMStatsConfig(
        ISPStripeConfig* pFrameConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Get3AFrameConfig
    ///
    /// @brief  Returns 3A frame configuration
    ///
    /// @param  pModuleInput     Pointer to the module input
    /// @param  pFrameConfig     Pointer to the Frame config data
    /// @param  requestId        Process Request ID
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Get3AFrameConfig(
        ISPInputData*    pModuleInput,
        ISPStripeConfig* pFrameConfig,
        UINT64           requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDependencies
    ///
    /// @brief  Check the availability of the dependence data. Update the data if it is available
    ///
    /// @param  pNodeRequestData        Pointer to the incoming NodeProcessRequestData
    /// @param  hasExplicitDependencies TRUE any explicit dependencies such as 3A stats etc. are required. Will be FALSE
    ///                                 if only PropertyIDISPExecProcDone dependency is set.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDependencies(
        NodeProcessRequestData* pNodeRequestData,
        BOOL                    hasExplicitDependencies);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HardcodeSettings
    ///
    /// @brief  Set the hardcode input values until 3A is completly integrated
    ///
    /// @param  pModuleInput Pointer to the IQ module input data
    /// @param  pFrameConfig Pointer to the Frame config data
    /// @param  initalConfig Flag to indicate if the config is during config streams
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HardcodeSettings(
        ISPInputData*     pModuleInput,
        ISPStripeConfig*  pFrameConfig,
        BOOL              initalConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HardcodeTintlessSettings
    ///
    /// @brief  Set the hardcode input values to Tintless
    ///
    /// @param  pModuleInput Pointer to the IQ module input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HardcodeTintlessSettings(
        ISPInputData*     pModuleInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DynamicCAMIFCrop
    ///
    /// @brief  Apply CAMIF crop if available from node instance
    ///
    /// @param  pStripeConfig Pointer to the Stripe config data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DynamicCAMIFCrop(
        ISPStripeConfig* pStripeConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdBuffers
    ///
    /// @brief  Helper method to initialize command manager and to allocate command buffers
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateCmdBuffers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchCmdBuffers
    ///
    /// @brief  Helper method to fetch command buffers for a particular request
    ///
    /// @param  requestId    Request number to which command buffer needs to be fetched
    /// @param  isInitPacket TRUE if Init Packet
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FetchCmdBuffers(
        UINT64  requestId,
        BOOL    isInitPacket);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddCmdBufferReference
    ///
    /// @brief  Helper method to add command buffer reference to CSL packet
    ///
    /// @param  requestId    Request number to which command buffer refrence needs to be added
    /// @param  opcode       CSL opcode init/Update
    /// @param  isInitPacket TRUE if Init Packet
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddCmdBufferReference(
        UINT64              requestId,
        CSLPacketOpcodesIFE opcode,
        BOOL                isInitPacket);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CommitAndSubmitPacket
    ///
    /// @brief  Helper method to commit DMI and Packet buffers and do CSL submit
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CommitAndSubmitPacket();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigBufferIO
    ///
    /// @brief  Helper method to pack Buffer IO into packet
    ///
    /// @param  pExecuteProcessRequestData Pointer to fetch Buffer for IO config
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigBufferIO(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckForRDIOnly
    ///
    /// @brief  Helper method to Check if it is RDI only use case
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckForRDIOnly();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupDeviceResource
    ///
    /// @brief  Setup List of the Required Resource
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupDeviceResource();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyResourcesToVer1
    ///
    /// @brief  Setup Input/output Resource for Version 1
    ///
    /// @param  pInputResource       Pointer to the Input Resource
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CopyResourcesToVer1(
        ISPInResourceInfo* pInputResource);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyResourcesToVer2
    ///
    /// @brief  Setup Input/output Resource for Version 2
    ///
    /// @param  pInputResource       Pointer to the Input Resource
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CopyResourcesToVer2(
        ISPInResourceInfoVer2* pInputResource);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateCSIDClockRate
    ///
    /// @brief  Calculate CSID clock rate
    ///
    /// @param  pClockRate      Pointer to clock rate to be filled
    /// @param  sensorBitWidth  Sensor Bit Width
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CalculateCSIDClockRate(
        UINT64* pClockRate,
        UINT32  sensorBitWidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupChannelResource
    ///
    /// @brief  Setup Channel Resource
    ///
    /// @param  inputPortId          InputPort ID
    /// @param  totalOutputPort      Number of output port in this use case
    /// @param  pOutputPortId        Pointer to the output port id array
    /// @param  portSourceTypeId     Port source type ID if more than one IFE node input port
    /// @param  streamIndex          Stream Index
    /// @param  pInputMapped         Pointer to the input method
    /// @param  pInputResource       Pointer to the Input Resource
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupChannelResource(
        UINT                   inputPortId,
        UINT                   totalOutputPort,
        const UINT*            pOutputPortId,
        UINT                   portSourceTypeId,
        UINT                   streamIndex,
        BOOL*                  pInputMapped,
        IFEInputResourceInfo*  pInputResource);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapPortIdToChannelId
    ///
    /// @brief  Find the OutputChannel based on the port Id
    ///
    /// @param  portId      Port ID defined in the use case xml
    /// @param  pChannelId  Pointer to the physical channel ID
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult MapPortIdToChannelId(
        UINT    portId,
        UINT32* pChannelId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDAFInformation
    ///
    /// @brief  Get sensor PDAF related data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetPDAFInformation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPixelsInSkipPattern
    ///
    /// @brief  Helper method to determine number of pixels in given Skip Pattern
    ///
    /// @param  skipPattern    Skip Pattern
    ///
    /// @return Number of pixels in a skip pattern
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetPixelsInSkipPattern(
        UINT16 skipPattern
    ) const
    {
        UINT32 count = 0;

        while (0 < skipPattern)
        {
            count++;
            skipPattern = skipPattern & (skipPattern - 1);
        }

        return count;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetReverseSkipPattern
    ///
    /// @brief  Helper method to get the inverse skip pattern
    ///
    /// @param  skipPattern Skip Pattern
    ///
    /// @return Inverse skip pattern
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT16 GetReverseSkipPattern(
        UINT16 skipPattern
    ) const
    {
        UINT16 reverseSkipPattern = 0;

        for (UINT index = 0; index < 15; index++)
        {
            reverseSkipPattern |= (skipPattern & 1);
            skipPattern         = skipPattern >> 1;
            reverseSkipPattern  = reverseSkipPattern << 1;
        }
        reverseSkipPattern |= (skipPattern & 1);
        return reverseSkipPattern;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PreparePDAFInformation
    ///
    /// @brief  PreparePDAFInformation
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PreparePDAFInformation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculatePDAFBufferParams
    ///
    /// @brief  Calculate PDAF Buffer Params
    ///
    /// @param  pBufferWidth  Pointer to the buffer Width
    /// @param  pBufferHeight Pointer to the Buffer Height
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculatePDAFBufferParams(
        UINT32* pBufferWidth,
        UINT32* pBufferHeight);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NeedsActiveIFEABVote
    ///
    /// @brief  return the chipsets needs ActiveIFEABVote
    ///
    /// @return TRUE if the chipset needs
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL NeedsActiveIFEABVote();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExtractCAMIFDecimatedPattern
    ///
    /// @brief  Extracts the PDAF information pattern from CAMIF subsample
    ///
    /// @param  horizontalOffset Pdaf pixels horizontal offset
    /// @param  verticalOffset   Pdaf pixels vertical offset
    /// @param  pBlockPattern    Pointer to PDAF block pattern data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ExtractCAMIFDecimatedPattern(
        UINT32             horizontalOffset,
        UINT32             verticalOffset,
        PDLibBlockPattern* pBlockPattern);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeOutputPathImageInfo
    ///
    /// @brief  Update ISP input data from the port
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeOutputPathImageInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSensorModeFormatBayer
    ///
    /// @brief  Indicate if the given Pixel format is a Bayer type or not.
    ///
    /// @param  format  PixelFormat format
    ///
    /// @return TRUE if the PixelFormat is a Bayer type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsSensorModeFormatBayer(
        PixelFormat format) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSensorModeFormatMono
    ///
    /// @brief  Indicate if the given Pixel format is a Mono type or not.
    ///
    /// @param  format  PixelFormat format
    ///
    /// @return TRUE if the PixelFormat is a Mono type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsSensorModeFormatMono(
        PixelFormat format) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSensorModeFormatYUV
    ///
    /// @brief  Indicate if the given Pixel format is a YUV type or not.
    ///
    /// @param  format  PixelFormat format
    ///
    /// @return TRUE if the PixelFormat is a YUV type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsSensorModeFormatYUV(
        PixelFormat format) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsTPGMode
    ///
    /// @brief  Indicate if current mode is TPG or not
    ///
    /// @return TRUE if TPG is enabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsTPGMode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HardcodeSettingsSetDefaultBFFilterInputConfig
    ///
    /// @brief  Hardcode BF settings with default input filter configuration
    ///
    /// @param  pAFConfigOutput  Pointer to the AF configuration parameter to fill
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HardcodeSettingsSetDefaultBFFilterInputConfig(
        AFStatsControl* pAFConfigOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HardcodeSettingsSetDefaultBFROIInputConfig
    ///
    /// @brief  Hardcode BF settings with default region of interest configuration
    ///
    /// @param  pAFConfigOutput Pointer to the AF configuration parameter to fill
    /// @param  CAMIFWidth      The current CAMIF width
    /// @param  CAMIFHeight     The current CAMIF height
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HardcodeSettingsSetDefaultBFROIInputConfig(
        AFStatsControl* pAFConfigOutput,
        UINT32          CAMIFWidth,
        UINT32          CAMIFHeight);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadDefaultStatsConfig
    ///
    /// @brief  Read the default configuration.
    ///
    ///
    /// @param  pModuleInput Pointer to the IQ module input data
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReadDefaultStatsConfig(
        ISPInputData* pModuleInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HardcodeSettingsBFStats
    ///
    /// @brief  Set the hardcode input values for BF stats
    ///
    /// @param  pStripeConfig    The pointer to the frameConfig
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HardcodeSettingsBFStats(
        ISPStripeConfig* pStripeConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupHFRInitialConfig
    ///
    /// @brief  Setup HFR initial configuration in Generic Blob Command Buffer
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupHFRInitialConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIFEInputWidth
    ///
    /// @brief  Returns the active pixel input width for the specified IFE. If dual-IFE is mode is active, the left/right
    ///         split and overlap is accounted for. In single IFE mode, will return 0 for the right-side IFE.
    ///
    /// @param  ifeIndex    IFE (Left or Right) for which width is needed; In single-IFE mode, use IFESplitID::Left
    ///
    /// @return Width in pixels
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetIFEInputWidth(
        IFESplitID ifeIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculatePixelClockRate
    ///
    /// @brief  Calculates IFE clock rate for the active sensor configuration based on the pixel path's needs.
    ///         Does not account for the override ifeClockFrequencyMHz.
    ///
    /// @param  inputWidth   input width of the data processed by the IFE
    ///
    /// @return Clock rate in Hz required for inputWidth
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 CalculatePixelClockRate(
        UINT32  inputWidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPixelClockRate
    ///
    /// @brief  Get IFE clock rate for the active sensor configuration based on the pixel path's needs.
    ///         The override ifeClockFrequencyMHz is also accounted for, and the returned value may not be optimal value
    ///         if the override is enabled.
    ///
    /// @param  ifeIndex  IFE (Left or Right) for which rate is needed; In single-IFE mode, use IFESplitID::Left
    ///
    /// @return Clock rate in Hz; 0 if the override ifeClockFrequencyMHz is negative
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 GetPixelClockRate(
        IFESplitID ifeIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateRDIClockRate
    ///
    /// @brief  Calculates IFE clock rate for the active sensor configuration for the RDI path
    ///
    /// @return clock rate needed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 CalculateRDIClockRate();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateSensorLineDuration
    ///
    /// @brief  Calculates the duration required to output one line of pixel data from the sensor.
    ///
    /// @param  ifeClockRate              IFE clock rate to use
    ///
    /// @return Line length in micro seconds
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FLOAT CalculateSensorLineDuration(
        UINT64 ifeClockRate);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculatePixelPortLineBandwidth
    ///
    /// @brief  Calculates the bandwidth required at the current clock rate to output a single line for a given port.
    ///
    /// @param  outputWidthInBytes        Output width for the desired port in bytes
    /// @param  IFEIndex                  IFE index
    ///
    /// @return Line length in micro seconds
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 CalculatePixelPortLineBandwidth(
        UINT32 outputWidthInBytes,
        UINT   IFEIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsRDIBWPath
    ///
    /// @brief  Check whether input BW path type is a RDI path or not
    ///
    /// @param  path        Input BW path type
    ///
    /// @return TRUE if RDI path
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsRDIBWPath(
        IFEBWPathGrouping  path);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeAllBWPaths
    ///
    /// @brief  Initialize all paths in BW array
    ///
    /// @param  numIFEs              Number of valid IFEs for initialization
    /// @param  camnocBWbytes        BW value to use for CAMNOC
    /// @param  externalBWbytes      BW value to use for MNOC
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeAllBWPaths(
        UINT32  numIFEs,
        UINT64  camnocBWbytes,
        UINT64  externalBWbytes);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOverrideBandwidth
    ///
    /// @brief  Takes overrides ifeCamnocBandwidthMBytes and ifeExternalBandwidthMBytes into consideration and calculate the
    ///         bandwidth votes needed.
    ///
    /// @param  overrideCamnocMBytes        Override BW value to use for CAMNOC
    /// @param  overrideExternalMBytes      Override BW value to use for MNOC
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetOverrideBandwidth(
        INT                  overrideCamnocMBytes,
        INT                  overrideExternalMBytes);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateBandwidth
    ///
    /// @brief  Calculates the BW required to operate the IFE for the current use case.
    ///         Takes overrides ifeCamnocBandwidthMBytes and ifeExternalBandwidthMBytes into consideration.
    ///
    /// @param  pExecuteProcessRequestData  Pointer to EPR data
    /// @param  requestId                   RequestId for which BW config is being setup
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CalculateBandwidth(
        ExecuteProcessRequestData* pExecuteProcessRequestData,
        UINT64                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateExternalBWBytes
    ///
    /// @brief  Calculates the external BW in Bytes
    ///
    /// @param  externalLineBW  External Line BW
    /// @param  IFEIndex        IFE index
    ///
    /// @return return UINT64 in bytes
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 CalculateExternalBWBytes(
        DOUBLE externalLineBW,
        UINT   IFEIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateCamnocBWBytes
    ///
    /// @brief  Calculates the external BW in Bytes
    ///
    /// @param  camnocLineBW    External Line BW
    /// @param  IFEIndex        IFE index
    ///
    /// @return return UINT64 in bytes
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 CalculateCamnocBWBytes(
        DOUBLE camnocLineBW,
        UINT   IFEIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBWPathGrouping
    ///
    /// @brief  Get BW grouping for the each client
    ///
    /// @param  outputPortId OutputportId to check
    ///
    /// @return IFEBWPathGrouping Grouping reult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFEBWPathGrouping GetBWPathGrouping(
        UINT32 outputPortId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupResourceClockConfig
    ///
    /// @brief  Setup Clock configuration required for the use case being activated
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupResourceClockConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupCSIDClockConfig
    ///
    /// @brief  Setup CSID Clock configuration required for the use case being activated
    ///
    /// @param  sensorBitWidth  Sensor Bit width
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupCSIDClockConfig(
    UINT32 sensorBitWidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupResourceBWConfig
    ///
    /// @brief  Setup Bandwidth configuration required for the use case being activated
    ///
    /// @param  pExecuteProcessRequestData  Pointer to EPR data
    /// @param  requestId                   RequestId for which BW config is being setup
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupResourceBWConfig(
        ExecuteProcessRequestData* pExecuteProcessRequestData,
        UINT64                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupResourceBWConfigV2
    ///
    /// @brief  Setup Bandwidth configuration required for the use case being activated
    ///
    /// @param  pExecuteProcessRequestData  Pointer to EPR data
    /// @param  requestId                   RequestId for which BW config is being setup
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupResourceBWConfigV2(
        ExecuteProcessRequestData* pExecuteProcessRequestData,
        UINT64                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupUBWCInitialConfig
    ///
    /// @brief  Setup UBWC initial configuration in Generic Blob Command Buffer
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupUBWCInitialConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  GetUBWCCompressionRatio
    ///
    /// @brief  Get UBWC Compression Ratio
    ///
    /// @param  srcWidth      Souce Width
    /// @param  srcHeight     Source Height
    /// @param  pImageFormat  Pointer to outputPort Image format to access format and dimension info
    ///
    /// @return return compression ratio.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FLOAT GetUBWCCompressionRatio(
        UINT   srcWidth,
        UINT   srcHeight,
        const  ImageFormat* pImageFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupBusReadInitialConfig
    ///
    /// @brief  Setup IFE Bus Read initial configuration in Generic Blob Command Buffer
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupBusReadInitialConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStripeConfigIndex
    ///
    /// @brief  Helper to calculate the index to the stripe config array
    ///
    /// @param  stripe      Stripe index (0 or 1)
    /// @param  maxPorts    Max number of ports in this stripe configuration
    /// @param  portIndex   Port index
    /// @param  plane       Plane index
    ///
    /// @return Index into stripe array
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetStripeConfigIndex(
        UINT32 stripe,
        UINT32 maxPorts,
        UINT32 portIndex,
        UINT32 plane) const
    {
        return (stripe * maxPorts * CSLMaxNumPlanes) + (portIndex * CSLMaxNumPlanes) + plane;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateIQStateConfiguration
    ///
    /// @brief  Update stripe specific configuration to ISP input configuration
    ///
    /// @param  pFrameConfig    The pointer to the frameConfig
    /// @param  pStripeConfig   Structure holding stripe specific configuration
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateIQStateConfiguration(
        ISPStripeConfig* pFrameConfig,
        ISPStripeConfig* pStripeConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitialSetupandConfig
    ///
    /// @brief  Update initial config with respect to clock, HFR, BW
    ///
    /// @param  pInputData    The pointer to the Input data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitialSetupandConfig(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupHVXInitialConfig
    ///
    /// @brief  Setup HVX initial configuration
    ///
    /// @param  pInputData    The pointer to the Input data
    ///
    /// @return CamxResult True on successful execution else False on failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupHVXInitialConfig(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPDHwEnabled
    ///
    /// @brief  Check if PD HW is enabled
    ///
    /// @return TRUE if Fast PD is enabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPDHwEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishIFEInputToUsecasePool
    ///
    /// @brief  Publish IFE input usecase pool
    ///
    /// @param  pIFEResolution    The pointer to the Input Resolution
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishIFEInputToUsecasePool(
        IFEInputResolution* pIFEResolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishIFEOutputToUsecasePool
    ///
    /// @brief  Publish IFE Output usecase pool
    ///
    /// @param  pIFEResolution    The pointer to the Output Resolution
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishIFEOutputToUsecasePool(
        IFEOutputResolution* pIFEResolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPDAFCapabilityToUsecasePool
    ///
    /// @brief  Publish IFE input usecase pool
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishPDAFCapabilityToUsecasePool();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CanSkipAlgoProcessing
    ///
    /// @brief  Decides whether we can skip tintless algo processing for the current frame based on skip factor
    ///
    /// @param  pInputData    The pointer to the Input data
    ///
    ///
    /// @return TRUE if we can skip otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CanSkipAlgoProcessing(
        ISPInputData* pInputData
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsADRCEnabled
    ///
    /// @brief  Decides whether ADRC is enabled
    ///
    /// @param  pInputData        The pointer to the Input data
    /// @param  pPercentageOfGTM  The pointer where GTM percentage is updated
    ///
    /// @return TRUE if ADRC is enabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsADRCEnabled(
        ISPInputData* pInputData,
        FLOAT*        pPercentageOfGTM);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateInitIQSettings
    ///
    /// @brief  Updates the IQ settings for init packet
    ///
    /// @return CamxResult CamxResultSuccess on successful execution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateInitIQSettings();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateInitSettings
    ///
    /// @brief  Updates the IQ  and Non IQ settings for init packet
    ///
    /// @return CamxResult CamxResultSuccess on successful execution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateInitSettings();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFSModeEnabled
    ///
    /// @brief  Read vendor tag enable/disable FS mode
    ///
    /// @param  pIsFSModeEnabled Pointer to fill with FS mode enable value
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult IsFSModeEnabled(
        BOOL* pIsFSModeEnabled);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFSSnapshot
    ///
    /// @brief  Check if FS snapshot is triggered
    ///
    /// @param  pExecuteProcessRequestData Pointer to process request data
    /// @param  requestID                  Process requestID
    ///
    /// @return TRUE/FALSE   TRUE -> Enabled, FALSE -> Disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsFSSnapshot(
        ExecuteProcessRequestData* pExecuteProcessRequestData,
        UINT64                     requestID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataContrastLevel
    ///
    /// @brief  Read level vendor tag and decide the contrast level
    ///
    /// @param  pHALTagsData Pointer to fill contrast level
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetMetadataContrastLevel(
        ISPHALTagsData* pHALTagsData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataTonemapCurve
    ///
    /// @brief  Read tonemap curve from vendor tag
    ///
    /// @param  pHALTagsData Pointer to fill tonemap curve
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetMetadataTonemapCurve(
        ISPHALTagsData* pHALTagsData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateCamifSettings
    ///
    /// @brief  Updates the camif programm setting
    ///
    /// @param  pInputData Pointer to the module input Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateCamifSettings(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFPS
    ///
    /// @brief  Get FPS
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFPS();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpIFESettings
    ///
    /// @brief  Dumps the IFE Settings
    ///
    /// @param  pInputData Pointer to the module input Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpIFESettings(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpPDAFSettings
    ///
    /// @brief  Dumps the PDAF Settings
    ///
    /// @param  fd          File Descriptor for Dump File
    /// @param  pPDHWConfig Pointer to the module input Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpPDAFSettings(
        INT         fd,
        PDHwConfig* pPDHWConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsTMC12Enalbed
    ///
    /// @brief  Check if TMC12 is available in current HW version and pipeline
    ///
    /// @return TRUE if TMC12 is enabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsTMC12Enalbed()
    {
        return (((CSLCameraTitanVersion::CSLTitan480 == m_titanVersion)  ||
            (CSLCameraTitanVersion::CSLTitan175      == m_titanVersion)  ||
            (CSLCameraTitanVersion::CSLTitan170      == m_titanVersion)) &&
            (SWTMCVersion::TMC12                     == GetIFESWTMCModuleInstanceVersion()));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataTMC12
    ///
    /// @brief  Get the calculated histogram and CDF from previous frame for TMC12
    ///
    /// @param  pModuleInput     Pointer to the module input
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetMetadataTMC12(
        ISPInputData*    pModuleInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsLDCEnabledByFlow
    ///
    /// @brief  Is LDC Enabled by flow
    ///
    /// @return TRUE if LDC enabled, Otherwise False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsLDCEnabledByFlow()
    {
        BOOL bIsLDCEnabledByFLow = FALSE;

        const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

        if ((TRUE == pStaticSettings->enableLDC) && (TRUE == m_capability.LDCSupport))
        {
            bIsLDCEnabledByFLow = TRUE;
        }

        return bIsLDCEnabledByFLow;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SkipTagForPublishList
    ///
    /// @brief  Skip Adding Tag into the Publish List
    ///
    /// @param  tagIndex     vendor tag index
    ///
    /// @return TRUE if need to skip Adding Tag into the Publish List, Otherwise False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL SkipTagForPublishList(
        UINT32 tagIndex)
    {
        BOOL bSkipAddTag = FALSE;

        const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

        if ((FALSE == m_publishLDCGridData) && ((ICAInGridOut2InTransformIndex == tagIndex) ||
            (ICAInGridIn2OutTransformIndex == tagIndex)))
        {
            bSkipAddTag = TRUE;
        }

        return bSkipAddTag;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupICAGrid
    ///
    /// @brief  Setup method to allocate memory for grid data and fill it from ICA chromatix
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupICAGrid();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateICAGridData
    ///
    /// @brief  Allocate memory for storing ICA grid from chromatix
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocateICAGridData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeallocateICAGridData
    ///
    /// @brief  Free memory allocated for storing ICA grid from chromatix
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DeallocateICAGridData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillICAChromatixGridData
    ///
    /// @brief  Fill Grid data from ICA30 chromatix
    ///
    /// @param  pTuningModeData tuning mode data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillICAChromatixGridData(
        ChiTuningModeParameter* pTuningModeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertIntoDistortedZoomWindow
    ///
    /// @brief  Convert zoom window from undistorted to distorted domain
    ///
    /// @param  pCrop       Pointer to CropWindow
    /// @param  fullWidth   input full width
    /// @param  fullHeight  input full height
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConvertIntoDistortedZoomWindow(
        CropWindow* pCrop,
        INT32 fullWidth,
        INT32 fullHeight);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResampleAndPublishICAGridPerCropWindow
    ///
    /// @brief  Resample and Publish ICA Grid as per Crop Window
    ///
    /// @param  pInputData          Pointer to isp inputdata
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ResampleAndPublishICAGridPerCropWindow(
        const ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishICAGridTransform
    ///
    /// @brief  Publish ICA Grid Transform on output port
    ///
    /// @param  pWarpGridDataOut  Pointer to Warp output Grid data to publish
    /// @param  ifeLDCPath        IFE LDC Path on which grid needs to be published
    /// @param  gridType          grid type
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishICAGridTransform(
        NcLibIcaGrid* pWarpGridDataOut,
        UINT32 ifeLDCPath,
        UINT32 gridType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillLDCIFEOutZoomwindow
    ///
    /// @brief  Fill LDC frame Zoomwindow at IFE out
    ///
    /// @param  pAppliedCropInfo        Pointer to ife applied crop info
    /// @param  pResidualCropInfo       Pointer to ife residual crop info
    /// @param  fullWidth               Full input width
    /// @param  fullHeight              Full input height
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillLDCIFEOutZoomwindow(
        const IFECropInfo* pAppliedCropInfo,
        const IFECropInfo* pResidualCropInfo,
        UINT32 fullWidth,
        UINT32 fullHeight);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateLDCTotalDistortedDomainCropZW
    ///
    /// @brief  Calculate zoom window contains total crop which inlcudes sensor crops, CSID crop and IFE crop happened in
    ///         distorted domain
    ///
    /// @param  pSensorAppliedCropInfo      Pointer to sensor applied crop info
    /// @param  pIFEAppliedCropInfo         Pointer to ife residual crop info
    /// @param  pDistortedDomainZoomWindow  Pointer to zoom window which contains total crop wrt sensor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID CalculateLDCTotalDistortedDomainCropZW(
        const WindowRegion* pSensorAppliedCropInfo,
        const WindowRegion* pIFEAppliedCropInfo,
        NcLibWindowRegion* pDistortedDomainZoomWindow);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpICAGridTransform
    ///
    /// @brief  Dump ICA Grid Transform
    ///
    /// @param  pInput          Pointer to isp inputdata
    /// @param  pWarpGridData   Pointer to Warp In2Out and Out2In output Grid data
    /// @param  ifeLDCPath      IFE LDC Path on which grid needs to be published
    /// @param  gridType        grid type
    /// @param  pGridDomain     Pointer to grid domain name
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpICAGridTransform(
        const ISPInputData* pInput,
        NcLibIcaGrid* pWarpGridData,
        UINT32 ifeLDCPath,
        UINT32 gridType,
        const CHAR* pGridDomain);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpIFEDebugConfig
    ///
    /// @brief  Dump IFE Debug Config
    ///
    /// @param  fd              File Descriptor for dump
    /// @param  pIFEDebugData   Pointer to isp DebugData
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpIFEDebugConfig(
        INT             fd,
        IFEConfigInfo*  pIFEDebugData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearIFEDebugConfig
    ///
    /// @brief  Update IFE Debug Config
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    CAMX_INLINE VOID ClearIFEDebugConfig()
    {
        for (UINT index = 0; index < IFERequestQueueDepth; index++)
        {
            m_IFEPerFrameData[index].isValid = FALSE;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIFEPerFrameDataIndex
    ///
    /// @brief  Get IFE Per Frame Queue Index
    ///
    /// @param  frameNum   Current Requet ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    CAMX_INLINE UINT32 GetIFEPerFrameDataIndex(
        UINT64  frameNum)
    {
        return static_cast<UINT32>(frameNum % IFERequestQueueDepth);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitIFEPerFrameConfig
    ///
    /// @brief  Update IFE Debug Config
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitIFEPerFrameConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateIFEDebugConfig
    ///
    /// @brief  Update IFE Debug Config
    ///
    /// @param  pInputData  Pointer to isp inputdata
    /// @param  requestID   Current Requet ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateIFEDebugConfig(
        ISPInputData* pInputData,
        UINT64 requestID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpDebugInternalNodeInfo
    ///
    /// @brief  Dump node specific debug info
    ///
    /// @param  requestId       requstId for dump
    /// @param  isPerFrameDump  ife per frame register dump indicator
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpDebugInternalNodeInfo(
        UINT64  requestId,
        BOOL    isPerFrameDump);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResubmitInitPacket
    ///
    /// @brief  Resubmit init packet
    ///
    /// @param  requestId   The current request ID
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ResubmitInitPacket(
        const UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchSensorInfo
    ///
    /// @brief  Fetch the sensor mode , PDAF and OTP data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FetchSensorInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateBufferConfig
    ///
    /// @brief  Validate thh Buffer Configuration
    ///
    /// @param  pBuffer  Pointer to the Buffer
    /// @param  portId   Port Id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ValidateBufferConfig(
        ImageBuffer* pBuffer,
        UINT         portId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataDarkBoostOffset
    ///
    /// @brief  Get the meta from apps for TMC user control about dark boost offset
    ///
    /// @param  pModuleInput     Pointer to the module input
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetMetadataDarkBoostOffset(
        ISPInputData*    pModuleInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataFourthToneAnchor
    ///
    /// @brief  Get the meta from apps for TMC user control about fourth tone anchor
    ///
    /// @param  pModuleInput     Pointer to the module input
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetMetadataFourthToneAnchor(
        ISPInputData*    pModuleInput);

    static const UINT8 MaxIFEOutputPorts = 22;                     ///< Max output ports
    static const UINT8 MaxIFEInputPorts  = 8;                      ///< Max input ports
    static const UINT8 MaxIFEIQModule    = 50;                     ///< Max Number of IQ Modules
    static const UINT8 MaxIFEStatsModule = 10;                     ///< Max Number of Stats Modules
    static const UINT8 MaxIFEResource    = 19;                     ///< Max Number of the IFE Resource Sent to CSI

    CmdBufferManager*        m_pIQPacketManager;                   ///< IQ Packet buffer manager
    CmdBufferManager*        m_pIQMainCmdBufferManager;            ///< Cmd buffer manager for the top-level command buffer
    CmdBufferManager*        m_pIQLeftCmdBufferManager;            ///< Cmd buffer manager for the left IFE (dual IFE)
    CmdBufferManager*        m_pIQRightCmdBufferManager;           ///< Cmd buffer manager for the right IFE (dual IFE)
    CmdBufferManager*        m_pDualIFEConfigCmdBufferManager;     ///< Cmd buffer manager for dual-IFE-specific config data
    CmdBufferManager*        m_p32bitDMIBufferManager;             ///< Buffer Manager for 32bit DMI data
    CmdBufferManager*        m_p64bitDMIBufferManager;             ///< Buffer Manager for 64bit DMI data
    CmdBufferManager*        m_pGenericBlobBufferManager;          ///< Buffer Manager for Generic Blob data
    CmdBufferManager*        m_pGenericBlobRightBufferManager;     ///< Buffer Manager for Generic Blob Data for Right IFE
    CmdBufferManager*        m_pGenericBlobLeftBufferManager;      ///< Buffer Manager for Generic Blob Data for Left IFE
    CmdBufferManager*        m_pFlushDumpBufferManager;            ///< Buffer Manager for flush Dump
    CmdBufferManager*        m_pHangDumpBufferManager;             ///< Buffer Manager for Hang Dump
    CmdBufferManager*        m_pRegDumpBufferManager;              ///< Buffer Manager for Reg Dump

    Packet*                  m_pIQPacket;                          ///< CSL packet
    CmdBuffer*               m_p64bitDMIBuffer;                    ///< 64 bit DMI Buffer
    CmdBuffer*               m_p32bitDMIBuffer;                    ///< 32 bit DMI Buffer
    UINT32*                  m_p64bitDMIBufferAddr;                ///< 64 bit DMI buffer where IQ modules could write LUT
    UINT32*                  m_p32bitDMIBufferAddr;                ///< 32 bit DMI buffer where IQ modules could write LUT
    UINT32*                  m_pHangDumpBufferAddr;                ///< Hand Dump Buffer Address
    UINT32*                  m_pFlushDumpBufferAddr;               ///< Flush Dump Buffer Address
    UINT32*                  m_pRegDumpBufferAddr;                 ///< Register Dump Buffer Address

    CSLDeviceHandle          m_hDevice;                             ///< IFE device handle
    ISPIQModule*             m_pIFEIQModule[MaxIFEIQModule];        ///< List of IQ Modules
    ISPIQModule*             m_pIFEHVXModule;                       ///< HVX IQ Modules
    ISPStatsModule*          m_pIFEStatsModule[MaxIFEStatsModule];  ///< List of Stats Modules
    const SensorMode*        m_pSensorModeData;                     ///< Sensor mode related data for the current mode
    const SensorMode*        m_pSensorModeRes0Data;                 ///< Sensor mode related data for FULL SIZE
    const EEPROMOTPData*     m_pOTPData;                            ///< OTP Data read from EEPROM to be used for calibration
    SensorMode               m_testGenModeData;                     ///< TPG mode related data
    IFEInputResolution       m_IFEinputResolution;                  ///< IFE Input resolution
    IFEOutputResolution      m_IFEOutputResolution;                 ///< IFE Output resolution
    CSLVersion               m_version;                             ///< IFE Hardware Revision
    UINT32                   m_titanVersion;                        ///< titan version
    UINT32                   m_hwMask;                              ///< Mask where each bit indicates supported h/w
    UINT32                   m_maxOutputWidthFD;                    ///< Max FD Output Width
    UINT32                   m_maxOutputHeightFD;                   ///< Max FD Output Height
    IFECapabilityInfo        m_capability;                          ///< IFE Capability Configuration
    UINT                     m_numIFEIQModule;                      ///< Number of IFE IQ Modules
    UINT                     m_totalIQCmdSizeDWord;                 ///< Total Size of IQ Cmd List, in dword
    UINT                     m_total32bitDMISizeDWord;              ///< Total Size of 32 bit DMI buffer, in dword
    UINT                     m_total64bitDMISizeDWord;              ///< Total Size of 64 bit DMI buffer, in dword
    UINT                     m_genericBlobCmdBufferSizeBytes;       ///< Total Size of Generic Blob buffer, in bytes
    UINT                     m_numIFEStatsModule;                   ///< Number of IFE Stats Modules
    IFEPipelineData          m_pipelineData;                        ///< Common Data for the IFE Pipeline
    IFEDualPDPipelineData    m_dualPDPipelineData;                  ///< Camif data for dual PD pipeline
    IFEModuleMode            m_mode;                                ///< Dual or single IFE mode
    DualIFESplitParams       m_dualIFESplitParams;                  ///< Dual IFE split params
    UINT                     m_usecaseNumBatchedFrames;             ///< Number of framework frames batched together if
                                                                    ///  batching is enabled. By default is 1
    UINT32                  m_sensorActiveArrayWidth;               ///< Sensor active array width
    UINT32                  m_sensorActiveArrayHeight;              ///< Sensor active array width
    ISPHALConfigureData     m_ISPInputHALData;                      ///< Cached HAL/App data
    ISPSensorConfigureData  m_ISPInputSensorData;                   ///< Cached sensor module data for ISP input
    ISPHALTagsData          m_HALTagsData;                          ///< Keep all input coming from tags
    IFEHVXInfo              m_HVXInputData;                         ///< HVX input info
    IFEPDAFInfo             m_PDAFInfo;                             ///< PDAF Information
    IFEDefaultModuleConfig  m_defaultConfig;                        ///< IFE default modules configuration HW specific
    IFECoreConfig           m_IFECoreConfig;                        ///< IFE core config
    ADRCData                m_ADRCData;                             ///< IFE ADRC Information

    IFEOutputPath           m_IFEOutputPathInfo[MaxDefinedIFEOutputPorts];  ///< output paths enabled based on output ports

    BOOL                    m_RDIOnlyUseCase;                       ///< variable to track RDI only use case
    BOOL                    m_hasStatsNode;                         ///< variable to track stats node availablilty
    BOOL                    m_enableHardcodedConfig;                ///< variable to track hardcoded config
    BOOL                    m_useStatsAlgoConfig;                   ///< track if stats algorithm should be used
    BOOL                    m_OEMStatsConfig;                       ///< Variable to track OEM controlled stats
    BOOL                    m_OEMIQSettingEnable;                   ///< indicator for OEM IQ Setting Mode
    UINT8                   m_CSIDecodeBitWidth;                    ///< Cached CSI decode bit width
    BOOL                    m_currentSensorModeSupportPDAF;         ///< Current sensor mode has PDAF stream or not
    BOOL                    m_currentSensorModeSupportHDR;          ///< Current sensor mode has HDR stream or not
    BOOL                    m_currentSensorModeSupportMeta;         ///< Current sensor mode has Meta stream or not
    UINT                    m_totalInputPorts;                      ///< Total number of input ports
    UINT                    m_disabledOutputPorts;                  ///< Number of disabled output ports

    BOOL                    m_isDisabledOutputPort[MaxDefinedIFEOutputPorts];   ///< List of disabed output ports check values

    UINT8                   m_PDAFCSIDecodeFormat;                  ///< Cached PDAF stream CSI decode format
    UINT8                   m_metaCSIDecodeFormat;                  ///< Cached Meta stream CSI decode format
    UINT8                   m_HDRCSIDecodeFormat;                   ///< Cached HDR stream CSI decode format
    UINT                    m_IFECmdBlobCount;                      ///< Number of blob cmd buffers in circulation

    const SensorModuleStaticCaps*   m_pSensorCaps;                  ///< Cached pointer to the sensor capability

    ChiTuningModeParameter  m_tuningData;                           ///< Cached tuning mode selector data, to help optimize
                                                                    ///  tuning data (tree) search in the IQ modules

    // Below fields are used to support dual IFE mode without affecting IQ modules. Depending on the IFE mode, IQ modules
    // will access pointer to the appropriate data below. IFE node sets the pointer to the right value and IQ modules just
    // work as if they always write to a single ISP.
    IFEStripingInput*       m_pStripingInput;                       ///< Striping library input
    IFEStripingPassOutput*  m_pPassOut;                             ///< Striping library's output
    ISPInternalData         m_ISPFramelevelData;                    ///< Frame-level data (used for striping)
    ISPStripeConfig         m_stripeConfigs[2];                     ///< ISP input configuration per stripe
    ISPInternalData         m_ISPData[DualIFEMax];                  ///< Data Calculated by IQ Modules (common, left and right)
    ISPFrameInternalData    m_ISPFrameData;                         ///< Frame-level ISP internal data
    CmdBuffer*              m_pCommonCmdBuffer;                     ///< Pointer to the Command Buffer common to left/right IFEs
    CmdBuffer*              m_pLeftCmdBuffer;                       ///< Pointer to the command buffer specific to the left IFE
    CmdBuffer*              m_pRightCmdBuffer;                      ///< Pointer to the command buffer specific to the right IFE
    CmdBuffer*              m_pDualIFEConfigCmdBuffer;              ///< Pointer to the command buffer containing
                                                                    ///  dual-IFE-specific configuration data
    CmdBuffer*              m_pGenericBlobCmdBuffer;                ///< Pointer to Generic Blob Command Buffer
    CmdBuffer*              m_pLeftGenericBlobCmdBuffer;            ///< Pointer to eneric Blob Left Buffer
    CmdBuffer*              m_pRightGenericBlobCmdBuffer;           ///< Pointer to eneric Blob Left Buffer
    CmdBuffer*              m_pFlushDumpCmdBuffer;                  ///< Pointer to the Flush Dump Command buffer
    CmdBuffer*              m_pHangDumpCmdBuffer;                   ///< Pointer to the Hnag dump command buffer
    CmdBuffer*              m_pRegDumpCmdBuffer;                    ///< Pointer to the reg dump command buffer

    IQLibInitialData        m_libInitialData;                       ///< IQ Library initial data
    DebugDataWriter*        m_pDebugDataWriter;                     ///< Pointer to the debug data pointer
    IFETuningMetadata*      m_pTuningMetadata;                      ///< Metadata for tuning support

    // Stats metadata field
    PropertyISPBFStats          m_BFStatsMetadata;                   ///< BF stats metadata to be updated and used by parser
    PropertyISPHDRBEStats       m_HDRBEStatsMetadata;                ///< HDR BE stats metadata to be updated and used by parser
    PropertyISPBHistStats       m_bhistStatsMetadata;                ///< BHist stats config
    PropertyISPAWBBGStats       m_AWBBGStatsMetadata;                ///< AWBBG stats metadata to be updated and used by parser
    PropertyISPRSStats          m_RSStatsMetadata;                   ///< RS stats metadata to be updated and used by parser
    PropertyISPCSStats          m_CSStatsMetadata;                   ///< CS stats metadata to be updated and used by parser
    PropertyISPHDRBHistStats    m_HDRBHistStatsMetadata;             ///< HDRBhist stats metadata to be updated
    PropertyISPTintlessBG       m_tintlessBGStatsMetadata;           ///< Tintless BG stats config
    PropertyISPIHistStats       m_IHistStatsMetadata;                ///< IHist stats metadata
    ISPStripeConfig             m_frameConfig;                       ///< FrameLevel config
    BOOL                        m_disableManual3ACCM;                ///< Override to disable manual 3A CCM
    BOOL                        m_initialConfigPending;              ///< Flag to track initial config
    UINT32                      m_resourcePolicy;                    ///< Prefered resource vs power trade-off
    IFEInstanceProperty         m_instanceProperty;                  ///< IFE Node Instance Property
    IFEResourceBWConfig         m_BWResourceConfig;                  ///< Resource BW calculated values
    IFEResourceBWConfigVer2     m_BWResourceConfigV2;                ///< Resource BW calculated values
    UINT                        m_highInitialBWCnt;                  ///< Counter for multiplying bw for initial frames
    FLOAT                       m_sensorLineTimeMSecs[2];            ///< Sensor line IFE sees including H blanking
    UINT64                      m_ifeClockRate[2];                   ///< Clock rate for left/right IFEs
    FLOAT                       m_IFEThroughPut[2];                  ///< Throught put rate for Left/right IFEs
    AFStatsControl              m_previousAFstatsControl;            ///< previous AF stats control
    UINT32                      m_maxNumOfCSLIFEPortId;              ///< CSL the max number of port Ids
    UINT32                      m_numResource;                       ///< Number of IFE resources
    ISPAcquireHWInfo*           m_pIFEResourceInfo;                  ///< Pointer to the IFE Acquire HW Information
    SIZE_T                      m_IFEResourceSize;                   ///< Size of the IFE resource
    IFEAcquiredHWInfo           m_IFEAcquiredHWInfo;                 ///< IFE Acquired HW info
    ISPInputData                m_IFEInitData;                       ///< IFE Init packet data
    BOOL                        m_isIFEResourceAcquried;             ///< If Acquired TRUE else FALSE
    UINT32                      m_vendorTagArray[IFEMaxOutputVendorTags];  ///< Array of tags published by the node
    CropWindow                  m_scalerCrop;                        ///< Crop window transalted back with respect to sensor APA
    Rational                    m_neutralPoint[3];                   ///< Neutral color point for posting metadata
    FLOAT                       m_dynamicBlackLevel[ISPChannelMax];  ///< Black level data for posting metdata
    IFECropInfo                 m_modifiedCropWindow;                ///< App crop modified with respect to aspect ratio
    BOOL                        m_enableBusRead;                     ///< Flag to indicate if IFE Read path is enabled
    BufferGroup                 m_bufferComposite;                   ///< Variable to cache the buffer composite info
    UINT                        m_FPS;                               ///< FPS requested
    ISPPipeline*                m_pIFEPipeline;                      ///< Pointer to the ISP pipeline Class
    IFEStripeInterface*         m_pIFEStripeInterface;               ///< Pointer to the striping interface
    OSLIBRARYHANDLE             m_hHandle;                           ///< IFE striping library handle
    CSIDBinningInfo             m_csidBinningInfo;                   ///< CSID binning Configuration
    IFEInputResourceInfo        m_IFEInputResource;                  ///< IFE Input resources

    IFEOutputResourceInfo       m_IFEOutputResource[MaxDefinedIFEOutputPorts]; ///< IFE Output Resources
    IFECSIDExtractionInfo       m_CSIDSubSampleInfo[IFECSIDMAX];               ///< IFE CSID Subsample Info
    ISPCalculatedMetadata       m_ISPMetadata;                                 ///< common IQ module data
    IFECAMIFCfgInfo             m_CAMIFConfigInfo[2];                          ///< IFE CAMIF Configure Info per stripe
    BOOL                        m_frameBased[IFECSIDMAX];                      ///< IFE write client config for frame based
    StreamDimension             m_RDIStreams[IFECSIDMAX];                      ///< RDI Stream dimensions

    // LDC
    BOOL                        m_ICAGridOut2InEnabled;                        ///< Warp Out2In Grid enabled
    BOOL                        m_ICAGridIn2OutEnabled;                        ///< Warp In2Out Grid enabled
    BOOL                        m_publishLDCGridData;                          ///< LDC Grid data publishing need flag
    UINT                        m_ICAGridGeometry;                             ///< Grid geometry as per ICA chromatix
    UINT                        m_ICAGridDomain;                               ///< Warp grid calibration domain:
                                                                               ///   0 Full Sensor, 1 Sensor output (IFE Input),
                                                                               ///   2 IFE output at DZx1
    ImageDimensions             m_ifeOutputImageSize;                          ///< IFE output image dimension
    NcLibIcaGrid*               m_pWarpGridDataIn[GridMaxType];                ///< warp grid data input
    NcLibIcaGrid*               m_pWarpGridDataOut[GridMaxType][LDCMaxPath];   ///< warp grid data output
    NcLibWindowRegion           m_ifeZoomWindowInDomain[LDCMaxPath];           ///< IFE zoom window in input sensor domain
    UINT32                      m_IFEPixelRawPort;                             ///< IFE pixel raw path that is enabled
    FLOAT                       m_sensorActiveArrayAR;                         ///< Sensor Active Array Aspect Ratio
    IFEConfigInfo               m_IFEPerFrameData[IFERequestQueueDepth];       ///< IFE Init packet data and Per Request config
    BufferRequirement           m_inputBufferRequirement;                      ///< Input Buffer Requirement
    BufferNegotiationData*      m_pBufferNegotiationData;                      ///< BufferNegotiation data for given pipeline
    UINT64                      m_IFELastAcceptedRequestId;                    ///< Latest RequestId that is accepted by
                                                                               ///< EPR with sequenceid = 1 just before
                                                                               ///< streamoff
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the Dual IFE utils class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DualIFEUtils
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EvaluateDualIFEMode
    ///
    /// @brief  Static method to evaluate whethere IFE node should run in dual mode or not
    ///
    /// @param  pISPInputdata              ISP configuration to be used to read frame-level settings
    /// @param  isFS2Enable                Is Fast Shutter mode enabled or disabled
    /// @param  forceSingleIFEForPDAFType3 Forced to single IFE mode if PDAF Type3 enabled.
    ///
    /// @return DualIFENormal if IFE node should run in dual IFE mode; SingleIFENormal otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static IFEModuleMode EvaluateDualIFEMode(
        ISPInputData* pISPInputdata,
        BOOL          isFS2Enable,
        BOOL          forceSingleIFEForPDAFType3);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCfgFromOneStripe
    ///
    /// @brief  Static method to fetch configurations of one stripe from striping output
    ///
    /// @param  pISPInputdata ISP configuration to be used to read frame-level settings
    /// @param  pStripeOut    Pointer to striping lib output
    /// @param  pStripeConfig Pointer to stripe config
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void FillCfgFromOneStripe(
        ISPInputData*             pISPInputdata,
        IFEStripeInterfaceOutput* pStripeOut,
        ISPStripeConfig*          pStripeConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FetchCfgWithStripeOutput
    ///
    /// @brief  Static method to fetch configurations from striping output
    ///
    /// @param  pISPInputdata Pointer to ISP configuration to be used to read frame-level settings
    /// @param  pPassOut      Pointer to striping lib output
    /// @param  pStripeConfig Pointer to stripe config
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void FetchCfgWithStripeOutput(
        ISPInputData*            pISPInputdata,
        IFEStripingPassOutput*   pPassOut,
        ISPStripeConfig*         pStripeConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ComputeSplitParams
    ///
    /// @brief  Static method to compute dual IFE split parameters
    ///
    /// @param  pISPInputdata   ISP configuration to be used to read frame-level settings, and to which the results will be
    ///                         written.
    /// @param  pSplitParams    Pointer to split parameters structure to be updated
    /// @param  pSensorMode     Pointer to the current sensor Mode
    /// @param  pStripeIface    Pointer to IFE stripe Interface
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ComputeSplitParams(
        ISPInputData*         pISPInputdata,
        DualIFESplitParams*   pSplitParams,
        const SensorMode*     pSensorMode,
        IFEStripeInterface*   pStripeIface);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseDualIfePassResult
    ///
    /// @brief  Static method to release Dual Ife stripping lib output
    ///
    /// @param  pPassOut             Stripe specific parameters used by striping library
    /// @param  pIFEStripeInterface  Pointer to IFE stripe Interface
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void ReleaseDualIfePassResult(
        IFEStripingPassOutput*  pPassOut,
        IFEStripeInterface*     pIFEStripeInterface);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintDualIfeInput
    ///
    /// @brief  Print Log for Dual IFE Stripe library input configurations
    ///
    /// @param  fd              Dump File Descriptor
    /// @param  pStripingInput  Stripe input configurations
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void PrintDualIfeInput(
        INT   fd,
        const IFEStripeInterfaceInput* pStripingInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintDualIfeOutput
    ///
    /// @brief  Print Log for Dual IFE Stripe library input configurations
    ///
    /// @param  fd            Dump File Descriptor
    /// @param  pStripeOut    Pointer to striping lib output
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void PrintDualIfeOutput(
        INT   fd,
        const IFEStripeInterfaceOutput* pStripeOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintDualIfeFrame
    ///
    /// @brief  Print Log for Dual IFE Stripe library input configurations
    ///
    /// @param  fd              Dump File Descriptor
    /// @param  pPassOut        Stripe specific parameters used by striping library
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void PrintDualIfeFrame(
        INT   fd,
        const IFEStripingPassOutput* pPassOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateDualIFEConfig
    ///
    /// @brief  Static method to update ISP configuration based on the current dual IFE split params
    ///
    /// @param  pISPInputdata   ISP configuration to be used to read frame-level settings
    /// @param  PDAFInfo        PDAF Pixel information
    /// @param  pStripeConfig   ISP stripe-level configuration to to be written to
    /// @param  pSplitParams    Split parameters
    /// @param  pPassOut        Stripe specific parameters used by striping library
    /// @param  pSensorMode     Pointer to the sensor mode Info
    /// @param  pStripeIface    Pointer to IFE stripe Interface
    /// @param  pNodeString     Pointer to Node Identifier String
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult UpdateDualIFEConfig(
        ISPInputData*            pISPInputdata,
        const IFEPDAFInfo        PDAFInfo,
        ISPStripeConfig*         pStripeConfig,
        DualIFESplitParams*      pSplitParams,
        IFEStripingPassOutput*   pPassOut,
        const SensorMode*        pSensorMode,
        IFEStripeInterface*      pStripeIface,
        const CHAR*              pNodeString);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateStripingInput
    ///
    /// @brief  Static method to update striping lib input params
    ///
    /// @param  pISPInputdata   ISP configuration to be used to read frame-level settings, and to which the results will be
    ///                         written.
    ///
    /// @return CamxResult CamxResultSuccess on successful execution
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult UpdateStripingInput(
        ISPInputData* pISPInputdata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslateOutputFormatToStripingLib
    ///
    /// @brief  Static method to translate CamX output image format to striping lib's format
    ///
    /// @param  srcFormat   Camx::Format
    ///
    /// @return Striping lib's format enum
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static int16_t TranslateOutputFormatToStripingLib(
        Format srcFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslateInputFormatToStripingLib
    ///
    /// @brief  Static method to translate CamX input image format to striping lib's format
    ///
    /// @param  srcFormat       Camx::Format
    /// @param  CSIDBitWidth    Sensor bitwidth
    ///
    /// @return Striping lib's format enum
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static int16_t TranslateInputFormatToStripingLib(
        PixelFormat srcFormat,
        UINT32      CSIDBitWidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPixelsInSkipPattern
    ///
    /// @brief  Helper method to determine number of pixels in given Skip Pattern
    ///
    /// @param  skipPattern    Skip Pattern
    ///
    /// @return Number of pixels in a skip pattern
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE UINT32 GetPixelsInSkipPattern(
        UINT16 skipPattern)
    {
        UINT32 count = 0;

        while (0 < skipPattern)
        {
            count++;
            skipPattern = skipPattern & (skipPattern - 1);
        }

        return count;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxStripeConfigSize
    ///
    /// @brief  Get the max stripe config size
    ///
    /// @param  maxNumOfCSLIFEPorts The max number of CSL IFE ports for the platform
    ///
    /// @return the size of max stripe config size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE UINT32 GetMaxStripeConfigSize(
        const UINT32 maxNumOfCSLIFEPorts)
    {
        UINT32 configSize;

        configSize = static_cast<UINT32>(sizeof(IFEDualConfig)) +
            (static_cast<UINT32>(sizeof(IFEStripeConfig)) *
            static_cast<UINT32>((2 * maxNumOfCSLIFEPorts * CSLMaxNumPlanes) - 1));

        return configSize;
    }

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillBFStats23CfgFromOneStripe
    ///
    /// @brief  Fill BF stats v2.3 from one stripe configuration
    ///
    /// @param  pBFStatsFromOneStripe       The pointer to the BF stats configuration from single stripe
    /// @param  pStripeOut                  The striping library output information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void FillBFStats23CfgFromOneStripe(
        BFStatsConfigParams*      pBFStatsFromOneStripe,
        IFEStripeInterfaceOutput* pStripeOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillBFStats24CfgFromOneStripe
    ///
    /// @brief  Fill BF stats v2.4 from one stripe configuration
    ///
    /// @param  pBFStatsFromOneStripe       The pointer to the BF stats configuration from single stripe
    /// @param  pStripeOut                  The striping library output information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void FillBFStats24CfgFromOneStripe(
        BFStatsConfigParams*       pBFStatsFromOneStripe,
        IFEStripeInterfaceOutput*  pStripeOut);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillBFStats25CfgFromOneStripe
    ///
    /// @brief  Fill BF stats v2.5 from one stripe configuration
    ///
    /// @param  pBFStatsFromOneStripe       The pointer to the BF stats configuration from single stripe
    /// @param  pStripeOut                  The striping library output information
    /// @param  totalROICountFromFrameLevel The total number of ROIs configured at the frame level
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void FillBFStats25CfgFromOneStripe(
        BFStatsConfigParams*      pBFStatsFromOneStripe,
        IFEStripeInterfaceOutput* pStripeOut,
        const UINT32              totalROICountFromFrameLevel);
};

CAMX_NAMESPACE_END

#endif // CAMXIFENODE_H
