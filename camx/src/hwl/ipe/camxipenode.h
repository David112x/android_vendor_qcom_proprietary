////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipenode.h
/// @brief IPENode class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIPENODE_H
#define CAMXIPENODE_H

#include "camxcmdbuffermanager.h"
#include "camxcslicpdefs.h"
#include "camxhwcontext.h"
#include "camxispiqmodule.h"
#include "camxisppipeline.h"
#include "camxmem.h"
#include "camxnode.h"
#include "ipeStripingLib.h"
#include "ipe_data.h"
#include "camximagedump.h"
#include "GeoLib.h"
#include "camxipeproperty.h"

CAMX_NAMESPACE_BEGIN

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT_IMAGE_FULL) ==
    static_cast<UINT32>(CSLIPEInputPortId::CSLIPEInputPortIdFull));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS4) ==
    static_cast<UINT32>(CSLIPEInputPortId::CSLIPEInputPortIdDS4));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS16) ==
    static_cast<UINT32>(CSLIPEInputPortId::CSLIPEInputPortIdDS16));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS64) ==
    static_cast<UINT32>(CSLIPEInputPortId::CSLIPEInputPortIdDS64));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT_IMAGE_FULL_REF) ==
    static_cast<UINT32>(CSLIPEInputPortId::CSLIPEInputPortIdFullRef));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS4_REF) ==
    static_cast<UINT32>(CSLIPEInputPortId::CSLIPEInputPortIdDS4Ref));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS16_REF) ==
    static_cast<UINT32>(CSLIPEInputPortId::CSLIPEInputPortIdDS16Ref));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT_IMAGE_DS64_REF) ==
    static_cast<UINT32>(CSLIPEInputPortId::CSLIPEInputPortIdDS64Ref));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_DISPLAY) ==
    static_cast<UINT32>(CSLIPEOutputPortId::CSLIPEOutputPortIdDisplay));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_VIDEO) ==
    static_cast<UINT32>(CSLIPEOutputPortId::CSLIPEOutputPortIdVideo));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_FULL_REF) ==
    static_cast<UINT32>(CSLIPEOutputPortId::CSLIPEOutputPortIdFullRef));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_DS4_REF) ==
    static_cast<UINT32>(CSLIPEOutputPortId::CSLIPEOutputPortIdDS4Ref));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_DS16_REF) ==
    static_cast<UINT32>(CSLIPEOutputPortId::CSLIPEOutputPortIdDS16Ref));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_OUTPUT_IMAGE_DS64_REF) ==
    static_cast<UINT32>(CSLIPEOutputPortId::CSLIPEOutputPortIdDS64Ref));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT2_IMAGE_FULL) ==
    static_cast<UINT32>(CSLIPEInputPortId::CSLIPEInput2PortIdFull));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT2_IMAGE_DSX) ==
    static_cast<UINT32>(CSLIPEInputPortId::CSLIPEInput2PortIdDSX));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_INPUT_OUTPUT_SCRATCHBUFFER) ==
    static_cast<UINT32>(CSLIPEOutputPortId::CSLIPEInputScratchBufer));

CAMX_STATIC_ASSERT(static_cast<UINT32>(IPE_IO_IMAGES::IPE_IO_IMAGES_MAX) ==
    static_cast<UINT32>(CSLIPEOutputPortId::CSLIPEOutputPortIdMaxNumPortResources));

CAMX_STATIC_ASSERT(static_cast<UINT32>(UbwcVersion::UBWC_VERSION_2) ==
    static_cast<UINT32>(UBWCVersion::UBWCVersion2));

CAMX_STATIC_ASSERT(static_cast<UINT32>(UbwcVersion::UBWC_VERSION_3) ==
    static_cast<UINT32>(UBWCVersion::UBWCVersion3));

CAMX_STATIC_ASSERT(static_cast<UINT32>(UbwcVersion::UBWC_VERSION_4) ==
    static_cast<UINT32>(UBWCVersion::UBWCVersion4));

CAMX_STATIC_ASSERT(sizeof(ImageDimensions) == sizeof(GeoLibImageSize));
CAMX_STATIC_ASSERT(sizeof(WindowROI)       == sizeof(GeoLibROI));


static const UINT32 ICAMinWidthPixels  = 30;
static const UINT32 ICAMinHeightPixels = 26;
static const UINT32 BPSMinWidthPixels  = 64;
static const UINT32 BPSMinHeightPixels = 64;

static const INT32 MinZSLNoiseReductionModeWidth  = 3840;
static const INT32 MinZSLNoiseReductionModeHeight = 2160;
static const INT32 MinZSLNoiseReductionModeSize   = MinZSLNoiseReductionModeWidth * MinZSLNoiseReductionModeHeight;


static const FLOAT MaxZoomFactor = 0.1f;

static const UINT32 IPEMustEnabledIQModulesMask = 1 <<(static_cast<UINT32>(ISPIQModuleType::IPEICA) - OffsetOfIPEIQModuleIndex);

/// @brief IPE Stats Vendor Tag Id
enum IPEStatsVendorTagId
{
    IPEVendorTagAECFrame = 0,       ///< Vendor Tag for AEC frame
    IPEVendorTagAWBFrame,           ///< Vendor Tag for AWB frame
    IPEVendorTagFlashNumber,        ///< Vendor Tag for Flash Number
    IPEVendorTagMax                 ///< Max number of Vendor Tag
};

/// @brief IPE IQ Vendor Tag Id
enum IPEIQVendorTagId
{
    IPEIQParam,         ///< Vendor Tag for IQ Param
    IPEIQVendorTagMax   ///< Max number of Vendor Tag
};

/// @brief IPE Image Information
struct IPEImageInfo
{
    UINT    width;             ///< Width of the image
    UINT    height;            ///< Height of the image
    FLOAT   bpp;               ///< bpp
    BOOL    UBWCEnable;        ///< UBWC Flag
    UINT32  UBWCVersion;       ///< Storing UBWC version
    BOOL    UBWCLossyMode;     ///< UBWC Lossy Mode Flag
};

/// @brief IPE Bandwidth Information
struct IPEBandwidthSource
{
    UINT64 compressedBW;        ///< compressed bandwidth
    UINT64 unCompressedBW;      ///< unCompressed bandwidth
};

/// @brief IPE Total Bandwidth
struct IPEClockAndBandwidth
{
    UINT64              budgetNS;               ///< Maximum duration to process a frame in Nano Second
    UINT32              frameCycles;            ///< number of pixels processed
    UINT32              realtimeFlag;           ///< Flag for Realtime stream
    IPEBandwidthSource  inputReadBW;            ///< input port read bandwidth
    IPEBandwidthSource  inputWriteBW;           ///< input port write bandwidth
    IPEBandwidthSource  displayWriteBW;         ///< display port write Bandwidth
    IPEBandwidthSource  videoWriteBW;           ///< video port write Bandwidth
    IPEBandwidthSource  refInputReadBW;         ///< reference input port read Bandwidth
    IPEBandwidthSource  refOutputWriteBW;       ///< reference output port write Bandwidth
    UINT                FPS;                    ///< FPS
    FLOAT               partialBW;              ///< partial Bandwidth
};

/// @brief IPE updates to metadata
static const UINT IPEMetadataOutputTags[] =
{
    ControlMode,
    ControlEffectMode,
    ControlSceneMode,
    EdgeMode,
    ControlVideoStabilizationMode,
    ColorCorrectionAberrationMode,
    NoiseReductionMode,
    TonemapMode,
    ColorCorrectionMode,
    ColorCorrectionTransform,
    TonemapCurveBlue,
    TonemapCurveGreen,
    TonemapCurveRed,
};

static const CHAR* IPEPortName[] =
{
    "Full",     // 0  - Input
    "DS4",      // 1  - Input
    "DS16",     // 2  - Input
    "DS64",     // 3  - Input
    "FullRef",  // 4  - Input
    "DS4Ref",   // 5  - Input
    "DS16Ref",  // 6  - Input
    "DS64Ref",  // 7  - Input
    "Display",  // 8  - Output
    "Video",    // 9  - Output
    "FullRef",  // 10 - Output
    "DS4Ref",   // 11 - Output
    "DS16Ref",  // 12 - Output
    "DS64Ref",  // 13 - Output
    "Unknown",  // 14 - Unknown
};


static const UINT IPEPortNameMaxIndex = CAMX_ARRAY_SIZE(IPEPortName) - 1; ///< Number of strings in IPEPortName array

static const UINT NumIPEMetadataOutputTags = CAMX_ARRAY_SIZE(IPEMetadataOutputTags);  ///< Number of output vendor tags

/// @brief Number of UBWC stat buffer
static const UINT MaxUBWCStatsBufferSize = RequestQueueDepth; ///< Max Number of UBWC stats Buffers has to be one more than
                                                              ///< the request queue depth
                                                              ///< One extra buffer is used to gurantee the result doesn't
                                                              ///< get overwritten before it is processed for all the port

/// @brief IPE UBWC stat buffer Information
struct IPEUBWCStatBufInfo
{
    CSLBufferInfo*  pUBWCStatsBuffer;                  ///< Pointer to ubwc stats buffers
    UINT64          requestId[MaxUBWCStatsBufferSize]; ///< request Id associated with this buffer
    SIZE_T          offset[MaxUBWCStatsBufferSize];    ///< Offset of the UBWC Stats buffer
};

// ping - pong  buffers used for reference
static const UINT ReferenceBufferCount    = 2;     ///< Reference buffer count
static const UINT MaxReferenceBufferCount = 32;    ///< Reference buffer count

static const UINT MaxMultipassBufferCount = 1;     ///< Multipass downscale buffer count

/// @brief IPE Loopback buffer parameters
struct LoopBackBufferParams
{
    ImageBufferManager*     pReferenceBufferManager;                   ///< Pointer to reference buffer manager
    ImageBuffer*            pReferenceBuffer[MaxReferenceBufferCount]; ///< Array of reference buffers
    ImageFormat             format;                                    ///< Image format
    CHAR                    bufferManagerName[MaxStringLength256];     ///< buffer manager name
    IPE_IO_IMAGES           outputPortID;                              ///< port ID for which reference buffer is allocated
    IPE_IO_IMAGES           inputPortID;                               ///< port ID for which reference buffer is allocated
    BOOL                    portEnable;                                ///< port Enable flag
    BOOL                    requestDelta;                              ///< delta between incoming requests
};

/// @brief IPE Multipass Input internal buffer parameters
struct MultipassBufferParams
{
    ImageBufferManager*     pMultipassBufferManager;                   ///< Pointer to multipass buffer manager
    ImageBuffer*            pMultipassBuffer;                          ///< pointer to multipass buffers
    ImageFormat             format;                                    ///< Image format
    CHAR                    bufferManagerName[MaxStringLength256];     ///< buffer manager name
    IPE_IO_IMAGES           inputPortID;                               ///< port ID for which multipass buffer is allocated
    BOOL                    portEnable;                                ///< port Enable flag
    UINT32                  passIdx;                                   ///< pass index

};

static const UINT IPEMaxSupportedBatchSize = MAX_HFR_GROUP; ///< Max batch size by firmware

/// @brief IPE vendor tag nums for ICA data
static const UINT IPEICATagNums = 9;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the IPE node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPENode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create IPENode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete IPENode object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static IPENode* Create(
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
    /// SetConsumerFormatParameters
    ///
    /// @brief  Set Consumer format param info
    ///
    /// @param  pFormat             Format pointer
    /// @param  pFormatParamInfo    pointer to FormatParamInfo
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetConsumerFormatParameters(
        ImageFormat*     pFormat,
        FormatParamInfo* pFormatParamInfo)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, " format %d, version %d, lossy %d",
                         pFormat->format,
                         m_capability.UBWCSupportedVersionMask,
                         m_capability.UBWCLossySupport);
        if (TRUE == ImageFormatUtils::IsUBWC(pFormat->format))
        {
            BOOL is10BitFormat = ImageFormatUtils::Is10BitUBWCFormat(pFormat->format);
            pFormatParamInfo->LossyUBWCConsumerUsage   &=
                (((TRUE == is10BitFormat) &&
                (pFormat->width >= m_capability.lossy10bitWidth) &&
                (pFormat->height >= m_capability.lossy10bitHeight)) ||
                ((pFormat->width >= m_capability.lossy8bitWidth) &&
                (pFormat->height >= m_capability.lossy8bitHeight)))
                ? m_capability.UBWCLossySupport : UBWCLossless;
            pFormatParamInfo->UBWCVersionConsumerUsage &= m_capability.UBWCSupportedVersionMask;
        }
        else
        {
            pFormatParamInfo->LossyUBWCConsumerUsage   &= 0;
            pFormatParamInfo->UBWCVersionConsumerUsage &= 0;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, " format %d, consumer : lossy %d, version %d",
                         pFormat->format,
                         pFormatParamInfo->LossyUBWCConsumerUsage,
                         pFormatParamInfo->UBWCVersionConsumerUsage);

        return CamxResultSuccess;
    }

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
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, " format %d, version %d, lossy %d",
                         pFormat->format,
                         m_capability.UBWCSupportedVersionMask,
                         m_capability.UBWCLossySupport);
        if (TRUE == ImageFormatUtils::IsUBWC(pFormat->format))
        {
            BOOL is10BitFormat = ImageFormatUtils::Is10BitUBWCFormat(pFormat->format);
            pFormatParamInfo->LossyUBWCProducerUsage   &=
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
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, " format %d, producer : lossy %d, version %d",
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
    /// PostPipelineCreate
    ///
    /// @brief  virtual method to be called at NotifyTopologyCreated time; node should take care of updates and initialize
    ///         blocks that has dependency on other nodes in the topology at this time.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PostPipelineCreate();

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
    /// ProcessingNodeFinalizeInputRequirement
    ///
    /// @brief  Virtual method implemented by IPE node to determine its input buffer requirements based on all the output
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
        if (portId > IPEPortNameMaxIndex)
        {
            portId = IPEPortNameMaxIndex;
        }

        return IPEPortName[portId];
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
        if (portId > IPEPortNameMaxIndex)
        {
            portId = IPEPortNameMaxIndex;
        }

        return IPEPortName[portId];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IPENode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPENode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFrameDataCallBack
    ///
    /// @brief  A callback method to process the frame data
    ///
    /// @param  requestId       Request ID
    /// @param  portId          Output port ID
    /// @param  pBufferInfo     Chi Buffer info structure
    /// @param  pImageFormat    Image format pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ProcessFrameDataCallBack(
        UINT64             requestId,
        UINT               portId,
        ChiBufferInfo*     pBufferInfo,
        const ImageFormat* pImageFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSupportedPortConfiguration
    ///
    /// @brief  virtual method that will be called during NewActiveStreamsSetup. Nodes may use
    ///         this hook to disable processing if theyre disabled through settings.
    ///
    /// @param  portId  port id to check
    ///
    /// @return TRUE if supported port configuration
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsSupportedPortConfiguration(
        UINT portId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateNodeParamsOnPortStatus
    ///
    /// @brief  virtual method that will be called prior to setup request to update port status. Nodes may use this to
    ///         update their Final ports status dynamically
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID UpdateNodeParamsOnPortStatus();

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

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPENode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IPENode();
    IPENode(const IPENode&) = delete;                 ///< Disallow the copy constructor.
    IPENode& operator=(const IPENode&) = delete;      ///< Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupDeviceResource
    ///
    /// @brief  Setup List of the Required Resource
    ///
    /// @param  pConfigIOMem    Pointer to shared memory allocated buffer
    /// @param  pResource       Pointer to an arry of resource
    ///
    /// @return CamxResult      CamxResultSuccess when successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupDeviceResource(
        CSLBufferInfo*     pConfigIOMem,
        CSLDeviceResource* pResource);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslateToFirmwarePortId
    ///
    /// @brief     Helper method to find the firmware port id
    ///
    /// @param     portId             Input port id
    /// @param     pFirmwarePortId    pointer holding the firmware port id
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID TranslateToFirmwarePortId(
        UINT32         portId,
        IPE_IO_IMAGES* pFirmwarePortId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireDevice
    ///
    /// @brief  Helper method to acquire IPE device
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AcquireDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseDevice
    ///
    /// @brief  Helper method to release IPE device
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureIPECapability
    ///
    /// @brief  Set up IPE capability based on IPE revision number
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureIPECapability();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // UpdateIPEIOLimits
    ///
    /// @brief  Update IPE buffer negotiation data based on the the ports enabled
    ///
    /// @param  pBufferNegotiationData        Buffer negotiation data containing the port info.
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateIPEIOLimits(
       BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateIPEIQModules
    ///
    /// @brief  Create IQ Modules of the IPE Block
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateIPEIQModules();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateIQModulesCmdBufferManager
    ///
    /// @brief  Create IQ Modules Cmd Buffer Manager
    ///
    /// @param  pModuleInputData        module input data
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateIQModulesCmdBufferManager(
        IPEModuleCreateData*     pModuleInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateGeneralTuningMetadata
    ///
    /// @brief  Helper to populate tuning data easily obtain at IPE node level
    ///
    /// @param  pInputData Pointer to the data input and output stuctures are inside
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateGeneralTuningMetadata(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareTuningMetadataDump
    ///
    /// @brief  Helper to prepare publishing tuning metadata
    ///
    /// @param  pInputData          Pointer to the input data
    /// @param  ppTuningMetadata    Pointer to the pTuningMetadata. If tuning-metadata not required, will be set to NULL
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareTuningMetadataDump(
        ISPInputData*       pInputData,
        IPETuningMetadata** ppTuningMetadata);

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
    /// PostMetadata
    ///
    /// @brief  Post IPE metadata to main metadata pool
    ///
    /// @param  pInputData Pointer to the input data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PostMetadata(
        const ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProgramIQConfig
    ///
    /// @brief  Reprogram the settings for the IQ Modules
    ///
    /// @param  pInputData Pointer to the input data
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProgramIQConfig(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataTags
    ///
    /// @brief  Get all information from HAL metadata tags for all IPE modules
    ///
    /// @param  pModuleInput Pointer to the input and output data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetMetadataTags(
        ISPInputData* pModuleInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetIQModuleNumLUT
    ///
    /// @brief  Store number of Look Up Tables to be written for every IQ modules
    ///
    /// @param  type        IQ module type
    /// @param  numLUTs     Number of look up tables present in IQ module
    /// @param  path        path variable representing IPE path type
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetIQModuleNumLUT(
        ISPIQModuleType type,
        UINT            numLUTs,
        INT             path);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateIQCmdSize
    ///
    /// @brief  Calculate Maximum Command Buffer size required for all the available IQ Modules
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void UpdateIQCmdSize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Cleanup
    ///
    /// @brief  Clean up the allocated IQ modules and Stats Modules
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Cleanup();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyRequestProcessingError
    ///
    /// @brief  Function that does any error processing required for a fence callback. When CSL notifies the Node of a fence we
    ///         post a job in the threadpool. And when threadpool calls us to process the error fence-signal notification, this
    ///         function does the necessary processing.
    ///
    /// @param  pFenceHandlerData     Pointer to struct FenceHandlerData that belongs to this node
    /// @param  unSignaledFenceCount  Number of un-signalled fence count
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID NotifyRequestProcessingError(
            NodeFenceHandlerData* pFenceHandlerData,
            UINT unSignaledFenceCount);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDependencies
    ///
    /// @brief  Check the availability of the dependence data. Update the data if it is available
    ///
    /// @param  pExecuteProcessRequestData Process request data
    /// @param  parentNodeId               Parent node id for this request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDependencies(
        ExecuteProcessRequestData*  pExecuteProcessRequestData,
        UINT                        parentNodeId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetICADependencies
    ///
    /// @brief  Check the availability of the dependence data. Update the data if it is available
    ///
    /// @param  pNodeRequestData           Pointer to the incoming NodeProcessRequestData
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetICADependencies(
        NodeProcessRequestData*  pNodeRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGammaOutput
    ///
    /// @brief  Get gamma output for programming IPE IQ
    ///
    /// @param  pISPData   ISP Internal data to be set from metadata
    /// @param  parentID   Parent node ID
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetGammaOutput(
        ISPInternalData* pISPData,
        UINT32           parentID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGammaPreCalculationOutput
    ///
    /// @brief  Get gamma15 pre-calculated output from TMC 1.1
    ///
    /// @param  pISPData   ISP Internal data to be set from metadata
    /// @param  parentID   Parent node ID
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetGammaPreCalculationOutput(
        ISPInternalData* pISPData,
        UINT32           parentID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIntermediateDimension
    ///
    /// @brief  Get Intermediate Dimension
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetIntermediateDimension();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetADRCDependencies
    ///
    /// @brief  Set ADRC Dependency
    ///
    /// @param  pNodeRequestData Pointer to the incoming NodeProcessRequestData
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetADRCDependencies(
            NodeProcessRequestData* pNodeRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetADRCInfoOutput
    ///
    /// @brief  Get ADRC output for programming IPE IQ
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetADRCInfoOutput();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillIQSetting
    ///
    /// @brief  Fill IPE IQ settings
    ///
    /// @param  pInputData          Pointer to ISP Input Data
    /// @param  pIPEIQsettings      IQSettings payload for firmware
    /// @param  pPerRequestPorts    Pointer to per request active ports
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillIQSetting(
        ISPInputData*            pInputData,
        IpeIQSettings*           pIPEIQsettings,
        PerRequestActivePorts*   pPerRequestPorts);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // FillFramePerfParams
    ///
    /// @brief  Find the OutputChannel based on the port number
    ///
    /// @param  pFrameProcessData   Top level payload for firmware
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillFramePerfParams(
        IpeFrameProcessData* pFrameProcessData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // FillFrameUBWCParams
    ///
    /// @brief  Find the OutputChannel based on the port number
    ///
    /// @param  pMainCmdBuffer      Main command buffer
    /// @param  pFrameProcessData   Top level payload for firmware
    /// @param  pUBWCStatsBuf       Pointer to UBWC Stats buffer
    /// @param  pOutputPort         Pointer to the output port
    /// @param  outputPortIndex     Index of the output port
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillFrameUBWCParams(
        CmdBuffer*                pMainCmdBuffer,
        IpeFrameProcessData*      pFrameProcessData,
        CSLBufferInfo*            pUBWCStatsBuf,
        PerRequestOutputPortInfo* pOutputPort,
        UINT                      outputPortIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ValidateCorrectZoom
    ///
    /// @brief  Fill the zoom window for a frame
    ///
    /// @param  parentNodeId   Parent Node Id
    /// @param  pCropInfo      Crop Information
    /// @param  inputWidth     Input Width passed to zoom
    /// @param  inputHeight    Input Height passed to zoom
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID ValidateCorrectZoom(
        UINT          parentNodeId,
        CHIRectangle* pCropInfo,
        INT32         inputWidth,
        INT32         inputHeight);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // FillFrameZoomWindow
    ///
    /// @brief  Fill the zoom window for a frame
    ///
    /// @param  pInputData       pointer to ISP input data
    /// @param  parentNodeId     Parent node id for this request
    /// @param  pCmdBuffer       Command Buffer
    /// @param  requestId        Request id
    /// @param  disableZoom      flag indicating disable zoom
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillFrameZoomWindow(
        ISPInputData*     pInputData,
        UINT              parentNodeId,
        CmdBuffer*        pCmdBuffer,
        UINT64            requestId,
        BOOL              disableZoom);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // FillFrameBufferData
    ///
    /// @brief  Populate Frame buffer structure in firmware payload
    ///
    /// @param  pMainCmdBuffer              Pointer to top level firmware payload command buffer
    /// @param  pImageBuffer                Image buffer that needs to be attached to payload
    /// @param  payloadPatchedFrameIndex    In HFR mode this field indicates which frame within batch in payload
    /// @param  bufferPatchedFrameIndex     In HFR mode this field indicates which frame within batch in buffer
    /// @param  portId                      Port Id at which buffer should be assigned
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillFrameBufferData(
        CmdBuffer*      pMainCmdBuffer,
        ImageBuffer*    pImageBuffer,
        UINT32          payloadPatchedFrameIndex,
        UINT32          bufferPatchedFrameIndex,
        UINT32          portId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // FillInputFrameSetData
    ///
    /// @brief  Fill the frame data for all input ports/buffers , cosider batch size as well for HFR mode
    ///
    /// @param  pFrameProcessCmdBuffer  Pointer to top level firmware payload command buffer
    /// @param  portId                  Input or output port id
    /// @param  pImageBuffer            Image buffer associated with the port
    /// @param  numFramesInBuffer       Either 1 or usecase number of frames in a batch
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillInputFrameSetData(
        CmdBuffer*      pFrameProcessCmdBuffer,
        UINT            portId,
        ImageBuffer*    pImageBuffer,
        UINT32          numFramesInBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // UpdateOutputBufferConfig
    ///
    /// @brief  Fill the frame data for all output ports/buffers, cosider batch size as well for HFR mode
    ///
    /// @param  pFrameProcessData       Frameproces data pointer
    /// @param  pFrameProcessCmdBuffer  Pointer to top level firmware payload command buffer
    /// @param  pOutputPort             Per request Output port pointer
    /// @param  HALOutputBufferCombined If the HAL output buffer is combined for batched mode
    /// @param  requestId               request Id
    /// @param  portIndex               port index
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateOutputBufferConfig(
        IpeFrameProcessData*            pFrameProcessData,
        CmdBuffer*                      pFrameProcessCmdBuffer,
        PerRequestOutputPortInfo*       pOutputPort,
        BOOL                            HALOutputBufferCombined,
        UINT64                          requestId,
        UINT32                          portIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // FillOutputFrameSetData
    ///
    /// @brief  Fill the frame data for all output ports/buffers, cosider batch size as well for HFR mode
    ///
    /// @param  pFrameProcessCmdBuffer  Pointer to top level firmware payload command buffer
    /// @param  portId                  Input or output port id
    /// @param  pImageBuffer            Image buffer associated with the port
    /// @param  numFramesInBuffer       number of Frames in buffer either 1 or usecase number of frames in a batch
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillOutputFrameSetData(
        CmdBuffer*              pFrameProcessCmdBuffer,
        UINT                    portId,
        ImageBuffer*            pImageBuffer,
        UINT32                  numFramesInBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillPreLTMCDMProgram
    ///
    /// @brief  Fill the frame data for all ports, cosider batch size as well for HFR mode
    ///
    /// @param  ppIPECmdBuffer      Pointer to array of command buffers for given frame
    /// @param  pCdmProgramArray    Pointer to array of CDMProgramArray
    /// @param  pCdmProgram         Pointer to CdmProgram to be configured
    /// @param  programType         Program type of CDM Program defined by firmware interface
    /// @param  programIndex        Program index which determines order of CdmProgram
    /// @param  identifier          Unique identifier
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillPreLTMCDMProgram(
        CmdBuffer**             ppIPECmdBuffer,
        CDMProgramArray*        pCdmProgramArray,
        CdmProgram*             pCdmProgram,
        ProgramType             programType,
        PreLTMCDMProgramOrder   programIndex,
        UINT                    identifier);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillPostLTMCDMProgram
    ///
    /// @brief  Fill the frame data for all ports, cosider batch size as well for HFR mode
    ///
    /// @param  ppIPECmdBuffer      Pointer to array of command buffers for given frame
    /// @param  pCdmProgramArray    Pointer to array of CDMProgramArray
    /// @param  pCdmProgram         Pointer to CdmProgram to be configured
    /// @param  programType         Program type of CDM Program defined by firmware interface
    /// @param  programIndex        Program index which determines order of CdmProgram
    /// @param  identifier          Unique identifier
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillPostLTMCDMProgram(
        CmdBuffer**             ppIPECmdBuffer,
        CDMProgramArray*        pCdmProgramArray,
        CdmProgram*             pCdmProgram,
        ProgramType             programType,
        PostLTMCDMProgramOrder  programIndex,
        UINT                    identifier);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillNPSCDMProgram
    ///
    /// @brief  Fill the frame data for all ports, cosider batch size as well for HFR mode
    ///
    /// @param  ppIPECmdBuffer      Pointer to array of command buffers for given frame
    /// @param  pCdmProgramArray    Pointer to array of CDMProgramArray
    /// @param  pCdmProgram         Pointer to CdmProgram to be configured
    /// @param  programType         Program type of CDM Pgoram defined by firmware interface
    /// @param  arrayIndex          Index  of cdm buffer in main payload
    /// @param  passCmdBufferSize   Size of cmd buffer size
    /// @param  passOffset          Offset of the pass of CdmProgram
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillNPSCDMProgram(
        CmdBuffer**             ppIPECmdBuffer,
        CDMProgramArray*        pCdmProgramArray,
        CdmProgram*             pCdmProgram,
        ProgramType             programType,
        CDMProgramArrayOrder    arrayIndex,
        UINT32                  passCmdBufferSize,
        UINT32                  passOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillICACDMprograms
    ///
    /// @brief  Fill the frame data for all ports, cosider batch size as well for HFR mode
    ///
    /// @param  pFrameProcessData   Frame process data
    /// @param  ppIPECmdBuffer      Pointer to array of command buffers for given frame
    /// @param  programType         Program type of CDM Pgoram defined by firmware interface
    /// @param  programArrayOrder   Index  of cdm buffer in main payload
    /// @param  programIndex        Index for ICA program
    /// @param  identifier          Unique identifier
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    CamxResult FillICACDMprograms(
        IpeFrameProcessData*    pFrameProcessData,
        CmdBuffer**             ppIPECmdBuffer,
        ProgramType             programType,
        CDMProgramArrayOrder    programArrayOrder,
        ICAProgramOrder         programIndex,
        UINT                    identifier);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCDMProgramArrays
    ///
    /// @brief  Fill the frame data for all ports, cosider batch size as well for HFR mode
    ///
    /// @param  pFrameProcessData  Pointer to top level firmware payload data structure
    /// @param  pIpeIQSettings     IQ Settings pointer
    /// @param  ppIPECmdBuffer     Pointer to array of different command buffers
    /// @param  batchFrames        Number of batached frames
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillCDMProgramArrays(
        IpeFrameProcessData*    pFrameProcessData,
        IpeIQSettings*          pIpeIQSettings,
        CmdBuffer**             ppIPECmdBuffer,
        UINT                    batchFrames);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // CommitAllCommandBuffers
    ///
    /// @brief  Find the OutputChannel based on the port number
    ///
    /// @param  ppIPECmdBuffer           Pointer array of all command buffers managed by IPE node
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CommitAllCommandBuffers(
        CmdBuffer** ppIPECmdBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // GetCDMProgramArrayOffsetFromBase
    ///
    /// @brief  Calculate the offset in bytes, of the CDMProgram Array from base CDMProgramArray
    ///
    /// @param  arrayIndex  CDM Program array index (CDMProgramArrayOrder)
    ///
    /// @return return Offset of CDM program array from Base CDMProgramArray, -1 for failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT GetCDMProgramArrayOffsetFromBase(
        CDMProgramArrayOrder    arrayIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // GetCDMProgramArrayOffsetFromTop
    ///
    /// @brief  Calculate the offset in bytes, of the CDMProgram from start of IpeFrameProcess
    ///
    /// @param  arrayIndex     CDM Program index (CDMProgramArrayOrder)
    ///
    /// @return return Offset of CDM program array from start of IpeFrameProcess, -1 for failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT GetCDMProgramArrayOffsetFromTop(
        CDMProgramArrayOrder    arrayIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // GetCDMProgramOffset
    ///
    /// @brief  Calculate the offset in bytes, of the CDMProgram from start of IpeFrameProcess
    ///
    /// @param  arrayIndex      CDM Program Array index (CDMProgramArrayOrder)
    /// @param  cdmProgramIndex CDM Program index (PreLTMCDMProgramOrder or PostLTMCDMProgramOrder)
    ///
    /// @return return Offset of CDM program from start of IpeFrameProcess, -1 for failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT GetCDMProgramOffset(
        CDMProgramArrayOrder    arrayIndex,
        UINT                    cdmProgramIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OpenStripingLibrary
    ///
    /// @brief     Create the handle for striping library
    ///
    /// @return    return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult OpenStripingLibrary();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CloseStripingLibrary
    ///
    /// @brief     Destroy the handle for striping library
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void CloseStripingLibrary();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // InitializeStripingParams
    ///
    /// @brief  Initialize the striping library and create a command buffer
    ///
    /// @param  pConfigIOData  Ipe IO configuration data for firmware
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeStripingParams(
        IpeConfigIoData* pConfigIOData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeInitializeStripingParams
    ///
    /// @brief     DeInitialize the striping library and delete command buffers
    ///
    /// @return    return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DeInitializeStripingParams();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // FillStripingParams
    ///
    /// @brief  Get the striping params from striping library and fill the command buffer
    ///
    /// @param  pFrameProcessData             Top level payload for firmware
    /// @param  pIPEIQsettings                IQSettings payload for firmware
    /// @param  ppIPECmdBuffer                Pointer to array of all command buffers managed by IPE node
    /// @param  pICPClockAndBandWidth         Pointer to Clock and Bandwidth request
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillStripingParams(
        IpeFrameProcessData*         pFrameProcessData,
        IpeIQSettings*               pIPEIQsettings,
        CmdBuffer**                  ppIPECmdBuffer,
        IPEClockAndBandwidth*        pICPClockAndBandWidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // InitializeIPEIQSettings
    ///
    /// @brief  Disable the IQ modules by accessing the moduleCfgEn bit in IQ modules and memset ZoomWindow in ICA param
    ///
    /// @param  pIPEIQsettings                IQSettings payload for firmware
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeIPEIQSettings(
        IpeIQSettings*               pIPEIQsettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // PatchBLMemoryBuffer
    ///
    /// @brief  patch the BL command buffer
    ///
    /// @param  pFrameProcessData             Top level payload for firmware
    /// @param  ppIPECmdBuffer                Pointer to array of all command buffers managed by IPE node
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PatchBLMemoryBuffer(
        IpeFrameProcessData* pFrameProcessData,
        CmdBuffer**          ppIPECmdBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAAAInputData
    ///
    /// @brief  Hook up the AEC/AWB settings for the IQ Modules
    ///
    /// @param  pInputData    Pointer to the ISP input data for AAA setting updates
    /// @param  parentNodeID  Parent Node ID: IFE, JPEG, IPE, BPS, FDhw etc
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAAAInputData(
        ISPInputData* pInputData,
        UINT          parentNodeID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFaceROI
    ///
    /// @brief  Returns FD ROI information
    ///
    /// @param  pInputData       Pointer to the input data
    /// @param  parentNodeID     Parent node id
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFaceROI(
        ISPInputData* pInputData,
        UINT          parentNodeID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsIPEOnlyDownscalerEnabled
    ///
    /// @brief  Determine if IPE only downscaler is enabled
    ///
    /// @param  pBufferNegotiationData          Negotiation data for all output ports of a node
    ///
    /// @return return enabled/disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsIPEOnlyDownscalerEnabled(
        BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFDEnabled
    ///
    /// @brief  Determine if FD is enabled
    ///
    ///
    /// @return return enabled/disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsFDEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIPEDownscalerOnlyDimensions
    ///
    /// @brief  Determine if IPE only downscaler is enabled
    ///
    /// @param  width               Width
    /// @param  height              Height
    /// @param  pMaxWidth           Maximum width for IPE only downscaler
    /// @param  pMaxHeight          Maximum height for IPE only downscaler
    /// @param  downscaleLimit      Downscale limit
    /// @param  downScalarMode      Mode
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetIPEDownscalerOnlyDimensions(
        UINT32  width,
        UINT32  height,
        UINT32* pMaxWidth,
        UINT32* pMaxHeight,
        FLOAT   downscaleLimit,
        UINT    downScalarMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsStandardAspectRatio
    ///
    /// @brief  Determine if the aspect ratio is one of the standard 1:1, 4:3, 16:9 or 18.5:9
    ///
    /// @param  aspectRatio Aspect Ratio of check against the standard aspect ratios
    ///
    /// @return return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsStandardAspectRatio(
        FLOAT aspectRatio);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckDimensionRequirementsForIPEDownscaler
    ///
    /// @brief  Determine if IPE only downscaler is enabled
    ///
    /// @param  width               Width
    /// @param  height              Height
    /// @param  downScalarMode      Mode
    ///
    /// @return return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckDimensionRequirementsForIPEDownscaler(
        UINT32 width,
        UINT32 height,
        UINT   downScalarMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HardcodeSettings
    ///
    /// @brief  Hardcode settings for the IPE IQ Modules
    ///
    /// @param  pInputData Pointer to the ISP input data for setting updates
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HardcodeSettings(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateICADependencies
    ///
    /// @brief  Update the ICA Input Data
    ///
    /// @param  pInputData      Pointer to the ISP input data for setting updates
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateICADependencies(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishICADependencies
    ///
    /// @brief  Publish ICA Input Data
    ///
    /// @param  pNodeRequestData    Pointer to the ISP input data for setting updates
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishICADependencies(
        NodeProcessRequestData* pNodeRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HardcodeAAAInputData
    ///
    /// @brief  Hardcode AAA Input Data for now
    ///
    /// @param  pInputData    Pointer to the ISP input data for setting updates
    /// @param  parentNodeID  Parent Node ID: IFE, JPEG, IPE, BPS, FDhw etc
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HardcodeAAAInputData(
        ISPInputData* pInputData,
        UINT          parentNodeID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///  UpdateClock
    ///
    /// @brief  Update clock parameters in a command buffer
    ///
    /// @param  pExecuteProcessRequestData      Request specific data
    /// @param  pFrameProcessData               Frame process data
    /// @param  pIPEClockAndBandwidth           clock data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateClock(
        ExecuteProcessRequestData*   pExecuteProcessRequestData,
        IpeFrameProcessData*         pFrameProcessData,
        IPEClockAndBandwidth*        pIPEClockAndBandwidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  UpdateBandwidth
    ///
    /// @brief  Update Bandwidth parameters in a command buffer
    ///
    /// @param  pExecuteProcessRequestData Request specific data
    /// @param  pIPEClockAndBandwidth      Bandwidth data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateBandwidth(
        ExecuteProcessRequestData*   pExecuteProcessRequestData,
        IPEClockAndBandwidth*        pIPEClockAndBandwidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  CheckAndUpdateClockBW
    ///
    /// @brief  Update Bandwidth and Clock parameters in a command buffer
    ///
    /// @param  pCmdBuffer                 Command Buffer
    /// @param  pExecuteProcessRequestData Request specific data
    /// @param  pFrameProcessData          Frame process data
    /// @param  pIPEClockAndBandwidth      Bandwidth data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CheckAndUpdateClockBW(
        CmdBuffer*                   pCmdBuffer,
        ExecuteProcessRequestData*   pExecuteProcessRequestData,
        IpeFrameProcessData*         pFrameProcessData,
        IPEClockAndBandwidth*        pIPEClockAndBandwidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  UpdateUBWCCompressionRatio
    ///
    /// @brief  Update UBWC Compression Ratio
    ///
    /// @param  srcWidth      Souce Width
    /// @param  srcHeight     Source Height
    /// @param  bpp           Bits per pixel
    /// @param  ubwcVersion   UBWC Version
    /// @param  ubwcLossy     UBWC Lossy mode
    /// @param  pIPEUbwcRdCr  Pointer to the IPE UBWC RD CR
    /// @param  pIPEUbwcMCTFr Pointer to the IPE TF UBWC RD CR
    /// @param  pIPEUBWCWrCr  Pointer to the IPE UBWC WR CR
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateUBWCCompressionRatio(
        UINT   srcWidth,
        UINT   srcHeight,
        FLOAT  bpp,
        UINT32 ubwcVersion,
        BOOL   ubwcLossy,
        FLOAT* pIPEUbwcRdCr,
        FLOAT* pIPEUbwcMCTFr,
        FLOAT* pIPEUBWCWrCr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  CalculateIPERdBandwidth
    ///
    /// @brief  Calculate Read Bandwidth
    ///
    /// @param  pPerRequestPorts         Request specific data
    /// @param  pIPEClockAndBandwidth    Bandwidth data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculateIPERdBandwidth(
        PerRequestActivePorts*  pPerRequestPorts,
        IPEClockAndBandwidth*   pIPEClockAndBandwidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //  CalculateIPEWrBandwidth
    ///
    /// @brief  Calculate Write Bandwidth
    ///
    /// @param  pPerRequestPorts         Request specific data
    /// @param  pIPEClockAndBandwidth    Bandwidth data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculateIPEWrBandwidth(
        PerRequestActivePorts*  pPerRequestPorts,
        IPEClockAndBandwidth*   pIPEClockAndBandwidth);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateNumberofPassesonDimension
    ///
    /// @brief  Update the number of passes based on dimension
    ///
    /// @param  fullInputWidth  full pass input width
    /// @param  fullInputHeight full pass input height
    /// @param  pNumPasses      pointer that holds number of passess
    ///
    /// @return CamxResult Indicates if update is a failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateNumberofPassesonDimension(
        UINT32 fullInputWidth,
        UINT32 fullInputHeight,
        UINT32* pNumPasses);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOEMStatsConfig
    ///
    /// @brief  Returns OEM's custom stats configuration
    ///
    /// @param  pInputData    Pointer to the ISP input data for setting updates
    /// @param  parentNodeID  Parent Node ID: IFE, JPEG, IPE, BPS, FDhw etc
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetOEMStatsConfig(
        ISPInputData* pInputData,
        UINT          parentNodeID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOEMIQConfig
    ///
    /// @brief  Returns OEM's custom IQ configuration
    ///
    /// @param  pInputData    Pointer to the ISP input data for setting updates
    /// @param  parentNodeID  Parent Node ID: IFE, JPEG, IPE, BPS, FDhw etc
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetOEMIQConfig(
        ISPInputData* pInputData,
        UINT          parentNodeID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticMetadata
    ///
    /// @brief  Get all static information from HAL metadata tags for all IPE modules
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetStaticMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateUBWCLossyParams
    ///
    /// @brief  Update the UBWC lossy params based on the bpp and port
    ///
    /// @param  pConfigIOData  Pointer to configIO data
    /// @param  pImageFormat   Pointer to outputPort Image format to access format and dimension info
    /// @param  firmwarePortId Parameter referring firmwarePortID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateUBWCLossyParams(
        IpeConfigIoData*   pConfigIOData,
        const ImageFormat* pImageFormat,
        IPE_IO_IMAGES      firmwarePortId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCorrespondingPassInputPortForInputRefPort
    ///
    /// @brief  method to get the corresponding InputPort for the input reference port
    ///
    /// @param  inputRefPortId Input reference port id to check
    ///
    /// @return Input port Id corresponsing to the reference input port
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT GetCorrespondingPassInputPortForInputRefPort(
        UINT inputRefPortId
    ) const
    {
        CAMX_ASSERT(inputRefPortId <= IPEInputPortDS64Ref);

        UINT inputPortId = inputRefPortId;

        switch (inputRefPortId)
        {
            case IPEInputPortFullRef:
                inputPortId = IPEInputPortFull;
                break;
            case IPEInputPortDS4Ref:
                inputPortId = IPEInputPortDS4;
                break;
            case IPEInputPortDS16Ref:
                inputPortId = IPEInputPortDS16;
                break;
            case IPEInputPortDS64Ref:
                inputPortId = IPEInputPortDS64;
                break;
            default:
                break;
        }

        return inputPortId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCorrespondingInputRefPortForPassInputPort
    ///
    /// @brief  method to get the corresponding reference InputPort for the input port.
    ///
    /// @param  inputPortId Input port Id to check
    ///
    /// @return Input reference port Id corresponsing to the input port
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT GetCorrespondingInputRefPortForPassInputPort(
        UINT inputPortId
    ) const
    {
        CAMX_ASSERT(inputPortId <= IPEInputPortDS64);

        UINT inputRefPortId = inputPortId;

        switch (inputPortId)
        {
            case IPEInputPortFull:
                inputRefPortId = IPEInputPortFullRef;
                break;
            case IPEInputPortDS4:
                inputRefPortId = IPEInputPortDS4Ref;
                break;
            case IPEInputPortDS16:
                inputRefPortId = IPEInputPortDS16Ref;
                break;
            case IPEInputPortDS64:
                inputRefPortId = IPEInputPortDS64Ref;
                break;
            default:
                break;
        }

        return inputRefPortId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCorrespondingInputPortForOutputPort
    ///
    /// @brief  method to get the corresponding Input Port for the output port.
    ///
    /// @param  outputPortId to check
    /// @param  pInputPortId to update
    ///
    /// @return True if the Input port matched the corresponding output port
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL GetCorrespondingInputPortForOutputPort(
        UINT outputPortId,
        UINT* pInputPortId
    ) const
    {
        CAMX_ASSERT(outputPortId <= IPEOutputPortDS64Ref);

        BOOL dumpInputPort = FALSE;

        if (NULL != pInputPortId)
        {
            switch (outputPortId)
            {
                case IPEOutputPortDisplay:
                case IPEOutputPortFullRef:
                    *pInputPortId = IPEInputPortFull;
                    dumpInputPort = TRUE;
                    break;
                case IPEOutputPortDS4Ref:
                    *pInputPortId = IPEInputPortDS4;
                    dumpInputPort = TRUE;
                    break;
                case IPEOutputPortDS16Ref:
                    *pInputPortId = IPEInputPortDS16;
                    dumpInputPort = TRUE;
                    break;
                case IPEOutputPortDS64Ref:
                    *pInputPortId = IPEInputPortDS64;
                    dumpInputPort = TRUE;
                    break;
                default:
                    break;
            }
        }

        return dumpInputPort;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDumpInputPortBuffer
    ///
    /// @brief  Check whether to dump all inport buffers.
    ///
    /// @param  outputPortId          to check
    /// @param  unSignaledFenceCount  Number of un-signaled fence count
    ///
    /// @return True if the Output port is Full out
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsDumpInputPortBuffer(
        UINT outputPortId,
        UINT unSignaledFenceCount
    ) const
    {
        BOOL result = FALSE;

        if (outputPortId <= IPEOutputPortDS64Ref)
        {
            result = ((0 == unSignaledFenceCount) ? TRUE : FALSE);;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsEIS3Node
    ///
    /// @brief  Whether this node is an EIS3 node. Could be either the node that actually process EIS3 or one that is after
    ///         the node that process EIS3.
    ///
    /// @return TRUE if EIS3 node, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsEIS3Node()
    {
        BOOL bStabilizationTypeEIS3 = FALSE;

        if ((0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType)) ||
            (0 != (IPEStabilizationType::IPEStabilizationTypeSWEIS3 & m_instanceProperty.stabilizationType)))
        {
            bStabilizationTypeEIS3 = TRUE;
        }

        return bStabilizationTypeEIS3;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsValidProfileWithUpscaler
    ///
    /// @brief  Check if a profile ID supports upscaler
    ///
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsValidProfileWithUpscaler()
    {
        return ((IPEProfileIdPPS      == m_instanceProperty.profileId) ||
                (IPEProfileIdDefault  == m_instanceProperty.profileId) ||
                (IPEProfileIdHDR10    == m_instanceProperty.profileId) ||
                (IPEProfileIdUpscale  == m_instanceProperty.profileId)) ? TRUE : FALSE;

    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsReferencePort
    ///
    /// @brief  Check if a given port is Reference ports
    ///
    /// @param  portId    Port ID
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsReferencePort(
        UINT32 portId)
    {
        BOOL bReferencePort = FALSE;
        switch (portId)
        {
            case IPEOutputPortFullRef:
            case IPEOutputPortDS4Ref:
            case IPEOutputPortDS16Ref:
            case IPEOutputPortDS64Ref:
            case IPEInputPortFullRef:
            case IPEInputPortDS4Ref:
            case IPEInputPortDS16Ref:
            case IPEInputPortDS64Ref:
                bReferencePort = TRUE;
                break;
            default:
                break;
        }
        return bReferencePort;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // InitializeFrameProcessData
    ///
    /// @brief  Initialize CDM program array
    ///
    /// @param  pFrameProcessData    Frameprocess data payload for firmware
    /// @param  pIPEIQsettings       IQ settings
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeFrameProcessData(
        IpeFrameProcessData*    pFrameProcessData,
        IpeIQSettings*          pIPEIQsettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // InitializeCDMProgramArray
    ///
    /// @brief  Initialize CDM program array
    ///
    /// @param  pFrameProcessData    Frameprocess data payload for firmware
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID InitializeCDMProgramArray(
     IpeFrameProcessData*    pFrameProcessData)
    {
        CDMProgramArray* pCDMProgramArray = NULL;
        UINT8* pCDMPayload = reinterpret_cast<UINT8*>(pFrameProcessData);

        for (UINT32 programArrayOrder = 0; programArrayOrder < CDMProgramArrayOrder::ProgramArrayMax; programArrayOrder++)
        {
            pCDMProgramArray =
                reinterpret_cast<CDMProgramArray*>(pCDMPayload +
                    GetCDMProgramArrayOffsetFromTop(static_cast<CDMProgramArrayOrder>(programArrayOrder)));
            pCDMProgramArray->numPrograms = 0;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateIPEICAInputModule
    ///
    /// @brief  Create ICA1 input path IQ Module of the IPE Block
    ///
    /// @param  pModuleInputData   the module input data pointer for IQ module
    /// @param  pIQModule          the module info pointer to create IQ module
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult CreateIPEICAInputModule(
        IPEModuleCreateData* pModuleInputData,
        IPEIQModuleInfo*     pIQModule)
    {
        CamxResult result = CamxResultSuccess;

        pModuleInputData->path = IPEPath::INPUT;
        // ICA1 is first in the IPE hardware IQ modules order i.e. 0th index in the list
        if (TRUE == IsIQModuleInstalled(&pIQModule[0]))
        {
            result = pIQModule[0].IQCreate(pModuleInputData);
            if (CamxResultSuccess == result)
            {
                m_pEnabledIPEIQModule[m_numIPEIQModulesEnabled] = pModuleInputData->pModule;
                m_numIPEIQModulesEnabled++;
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "IPE:%d ICA input module is disabled, should be always enabled", InstanceID());
        }

        return result;
    }

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
        const IPEIQModuleInfo* pIQModuleInfo) const
    {
        return ((pIQModuleInfo->installed) &&
                ((1 <<(static_cast<UINT32>(pIQModuleInfo->moduleType) - OffsetOfIPEIQModuleIndex)) & m_enableIPEIQModulesMask));

    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsReferenceOutputPort
    ///
    /// @brief  Helper method to determine if the output port is a reference port.
    ///
    /// @param  outputPortId OutputportId to check
    ///
    /// @return TRUE if the output port is a reference port, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsReferenceOutputPort(
        UINT outputPortId
    ) const
    {
        BOOL bReferencePort = FALSE;

        switch (outputPortId)
        {
            case IPEOutputPortFullRef:
            case IPEOutputPortDS4Ref:
            case IPEOutputPortDS16Ref:
            case IPEOutputPortDS64Ref:
                bReferencePort = TRUE;
                break;

            default:
                break;
        }

        return bReferencePort;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsReferenceInputPort
    ///
    /// @brief  Helper method to determine if the input port is a reference port.
    ///
    /// @param  inputPortId InputportId to check
    ///
    /// @return TRUE if the input port is a reference port, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsReferenceInputPort(
        UINT inputPortId
    ) const
    {
        BOOL bReferencePort = FALSE;

        switch (inputPortId)
        {
            case IPEInputPortFullRef:
            case IPEInputPortDS4Ref:
            case IPEInputPortDS16Ref:
            case IPEInputPortDS64Ref:
                bReferencePort = TRUE;
                break;

            default:
                break;
        }

        return bReferencePort;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLoopBackPortEnable
    ///
    /// @brief  Helper method to set the Loop back port enable.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetLoopBackPortEnable()
    {
        m_loopbackPortEnable =
            ((0 != (IPEStabilizationType::IPEStabilizationMCTF & m_instanceProperty.stabilizationType)) &&
            (GetHwContext()->GetStaticSettings()->enableMCTF)) ? TRUE : FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsLoopBackPortEnabled
    ///
    /// @brief  Helper method to determine if the Loop back port enabled.
    ///
    ///
    /// @return TRUE if the loop back port is enabled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsLoopBackPortEnabled() const
    {
        return m_loopbackPortEnable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSrParams
    ///
    /// @brief  Helper method to set Sr enable
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetSrParams()
    {
        m_geolibData.srEnable        = IsMFSRProcessingType();
        m_geolibData.IPEModeRealtime = m_realTimeIPE;

    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetICAConfigMode
    ///
    /// @brief  Helper method to set ICA Configmode
    ///
    /// @param  rModuleInput    reference to ISP moduleinput data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetICAConfigMode(
        ISPInputData& rModuleInput)
    {
        if ((TRUE == IsMCTFEnabledWithICA1()) &&
            (TRUE == GetStaticSettings()->enableMCTF))
        {
            rModuleInput.pipelineIPEData.ICAMode = ICA_CONFIG_MCTF_ICA1;
        }
        else if ((IPEStabilizationType::IPEStabilizationMCTF == m_instanceProperty.stabilizationType) &&
            (TRUE == GetStaticSettings()->enableMCTF))
        {
            rModuleInput.pipelineIPEData.ICAMode = ICA_CONFIG_MCTF;
        }
        else if (IPEProcessingType::IPEMFNRBlend == m_instanceProperty.processingType)
        {
            rModuleInput.pipelineIPEData.ICAMode = ICA_CONFIG_MFNR_TEMPORAL;

        }
        else if ((IPEProcessingType::IPEMFSRBlend == m_instanceProperty.processingType))
        {
            rModuleInput.pipelineIPEData.ICAMode = ICA_CONFIG_MFNR_TEMPORAL_ANCHOR_AGGREGATE;
        }
        else
        {
            rModuleInput.pipelineIPEData.ICAMode = ICA_CONFIG_NONE;

        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSrEnabled
    ///
    /// @brief  Helper method to determine if SR is enabled
    ///
    ///
    /// @return TRUE if the SR is enabled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSrEnabled() const
    {
        return m_geolibData.srEnable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeGeoLibData
    ///
    /// @brief  Helper method to determine if the Loop back port enabled.
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID InitializeGeoLibData()
    {
        Utils::Memset(&m_geolibData, 0x0, sizeof(m_geolibData));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSkipReferenceInput
    ///
    /// @brief  Helper method to check if reference input needs to be skipped for a request
    ///
    /// @param  requestDelta         delta between incoming requests
    /// @param  firstValidRequest    indicates if this is a first valid request
    ///
    /// @return TRUE if the reference input needs to be skipped, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSkipReferenceInput(
        UINT64 requestDelta,
        UINT firstValidRequest
    ) const
    {
        return ((firstValidRequest == FirstValidRequestId) || (requestDelta > 1)) ? TRUE : FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsIPEHasInputReferencePort
    ///
    /// @brief  Helper method to check if reference input is present for a request
    ///
    /// @param  requestDelta         delta between incoming requests
    /// @param  firstValidRequest    indicates if this is a first valid request
    ///
    /// @return TRUE if the reference input is present FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsIPEHasInputReferencePort(
        UINT64 requestDelta,
        UINT firstValidRequest
    ) const
    {
        return ((TRUE == IsBlendWithNPS())         ||
                (TRUE == IsPostfilterWithNPS())    ||
                (IPEProfileIdIndications == m_instanceProperty.profileId) ||
                ((TRUE == IsLoopBackPortEnabled()) &&
                (FALSE == IsSkipReferenceInput(requestDelta, firstValidRequest)))) ? TRUE : FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsIPEReferenceDumpEnabled
    ///
    /// @brief  Helper method to check if reference dump is enabled
    ///
    /// @param  enabledNodeMask    IPE Node Mask to dump reference output
    /// @param  refOutputPortMask  IPE output port mask to dump reference output
    ///
    /// @return TRUE if the reference output dump is enabled FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsIPEReferenceDumpEnabled(
        UINT32  enabledNodeMask,
        UINT32  refOutputPortMask)
    {
        BOOL bIPEReferenceDumpEnabled = FALSE;

        if ((TRUE == GetStaticSettings()->autoImageDump) &&
            (0 != (enabledNodeMask & ImageDumpIPE))      &&
            (0 != (refOutputPortMask & GetStaticSettings()->autoImageDumpIPEoutputPortMask)))
        {
            bIPEReferenceDumpEnabled = TRUE;
        }

        return bIPEReferenceDumpEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLoopBackBufferBatchIndex
    ///
    /// @brief  Helper method to get a slot from loopback buffers offset for given requestID
    ///
    /// @param  requestId          Given requestId
    /// @param  numFramesInBuffer  Number of frames in given Batch
    ///
    /// @return Slot index in loopback buffers
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE INT GetLoopBackBufferBatchIndex(
        UINT64        requestId,
        UINT32        numFramesInBuffer)
    {
        INT numBufferSlots = 1;
        if (m_referenceBufferCount >= numFramesInBuffer)
        {
            numBufferSlots     = m_referenceBufferCount / numFramesInBuffer;
            return (1U >= numFramesInBuffer) ?
                (requestId % m_referenceBufferCount) : (requestId % numBufferSlots) * numFramesInBuffer;
        }
        return 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpLoopBackReferenceBuffer
    ///
    /// @brief  Helper method to dump reference output for a given request and portId
    ///
    /// @param  requestId             Request id
    /// @param  enableOutputPortMask  IPE output port Mask to dump any of the reference output portId
    /// @param  enableInstanceMask    IPE Instance mask which helps to dump only given instance
    /// @param  numBatchedFrames      Number of frames in a batch for given request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID DumpLoopBackReferenceBuffer(
        UINT64  requestId,
        UINT32  enableOutputPortMask,
        UINT32  enableInstanceMask,
        UINT    numBatchedFrames)
    {
        ImageDumpInfo dumpInfo      = { 0 };
        ImageBuffer*  pImageBuffer  = NULL;

        for (UINT pass = 0; pass < m_numPasses; pass++)
        {
            if (TRUE == m_loopBackBufferParams[pass].portEnable)
            {
                INT bufferBatchIndex = GetLoopBackBufferBatchIndex(requestId, numBatchedFrames);

                for (UINT batchId = 0; batchId < numBatchedFrames; batchId++)
                {
                    pImageBuffer =
                        m_loopBackBufferParams[pass].pReferenceBuffer[bufferBatchIndex + batchId];
                    DumpImageBuffers(pImageBuffer,
                                     requestId,
                                     batchId,
                                     enableOutputPortMask,
                                     enableInstanceMask,
                                     m_loopBackBufferParams[pass].outputPortID);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpImageBuffers
    ///
    /// @brief  Helper method to dump IPE image buffers
    ///
    /// @param  pImageBuffer          Image buffer to dump
    /// @param  requestId             Request id
    /// @param  batchId               Batch Id
    /// @param  enablePortMask        IPE Input/ Reference port Mask to dump
    /// @param  enableInstanceMask    IPE Instance mask which helps to dump only given instance
    /// @param  portId                port id of the image buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpImageBuffers(
        ImageBuffer*  pImageBuffer,
        UINT64        requestId,
        UINT          batchId,
        UINT32        enablePortMask,
        UINT32        enableInstanceMask,
        UINT          portId)
    {
        if (NULL != pImageBuffer)
        {
            const ImageFormat* pFormat = pImageBuffer->GetFormat();
            // Replace with real pipeline name as soon as it merges
            if (pFormat != NULL)
            {
                CHAR pipelineName[256]      = { 0 };
                OsUtils::SNPrintF(pipelineName, sizeof(pipelineName), "%s", GetPipelineName());
                ImageDumpInfo dumpInfo      = { 0 };
                dumpInfo.pPipelineName      = pipelineName;
                dumpInfo.pNodeName          = Name();
                dumpInfo.nodeInstance       = InstanceID();
                dumpInfo.portId             = portId;
                dumpInfo.requestId          = static_cast<UINT32>(requestId);
                dumpInfo.batchId            = batchId;
                dumpInfo.numFramesInBatch   = pImageBuffer->GetNumFramesInBatch();
                dumpInfo.pFormat            = pImageBuffer->GetFormat();
                dumpInfo.pBaseAddr          = pImageBuffer->GetCPUAddress();
                dumpInfo.width              = pFormat->width;
                dumpInfo.height             = pFormat->height;

                if ((enablePortMask & (1 << dumpInfo.portId)) &&
                    (enableInstanceMask & (1 << dumpInfo.nodeInstance)))
                {
                    ImageDump::Dump(&dumpInfo);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpInternalMultiPassBuffer
    ///
    /// @brief  Helper method to dump internal downscale IPE inputs
    ///
    /// @param  requestId             Request id
    /// @param  enableInputPortMask   IPE Input port Mask to dump
    /// @param  enableInstanceMask    IPE Instance mask which helps to dump only given instance
    /// @param  numBatchedFrames      Number of frames in a batch for given request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID DumpInternalMultiPassBuffer(
        UINT64  requestId,
        UINT32  enableInputPortMask,
        UINT32  enableInstanceMask,
        UINT    numBatchedFrames)
    {
        CAMX_UNREFERENCED_PARAM(numBatchedFrames);

        ImageDumpInfo dumpInfo = { 0 };
        ImageBuffer*  pImageBuffer = NULL;

        for (UINT pass = 0; pass < m_numPasses - 1; pass++)
        {
            if (TRUE == m_multipassBufferParams[pass].portEnable)
            {
                pImageBuffer = m_multipassBufferParams[pass].pMultipassBuffer;
                DumpImageBuffers(pImageBuffer,
                                 requestId,
                                 0,
                                 enableInputPortMask,
                                 enableInstanceMask,
                                 m_multipassBufferParams[pass].inputPortID);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsValidBatchSize
    ///
    /// @brief  Helper method to determine if the batch size is valid
    ///
    ///
    /// @return TRUE if batch size is valid FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsValidBatchSize() const
    {
        // if batch size is zero or batch size is a non zero odd value greater than 1
        return ((0 == m_maxBatchSize) ||
               ((1 < m_maxBatchSize) && (0 != (m_maxBatchSize % 2)))) ? FALSE : TRUE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFPSAndBatchSize
    ///
    /// @brief  Get FPS and batch Size
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFPSAndBatchSize();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOverrideScaleProfile
    ///
    /// @brief  Get the overrideScale vendor tag from the usecase pool
    ///
    /// @return false if Scale profile not to be overriden.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL GetOverrideScaleProfile()
    {
        CamxResult   result               = CamxResultSuccess;
        BOOL         bOverrideIPEScale    = FALSE;
        UINT32       overrideScalemetaTag = 0;
        result                            = VendorTagManager::QueryVendorTagLocation("org.quic.camera.overrideIPEScaleProfile",
                                            "OverrideIPEScaleProfile",
                                            &overrideScalemetaTag);

        if (CamxResultSuccess == result)
        {
            UINT              propertiesIPE[]   = { overrideScalemetaTag | UsecaseMetadataSectionMask };
            static const UINT Length            = CAMX_ARRAY_SIZE(propertiesIPE);
            VOID*             pData[Length]     = { 0 };
            UINT64            offsets[Length]   = { 0 };

            result = GetDataList(propertiesIPE, pData, offsets, Length);

            if (CamxResultSuccess == result)
            {
                if (NULL != pData[0])
                {
                    bOverrideIPEScale = *reinterpret_cast<UINT*>(pData[0]);
                }
                else
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Node %s Can't get overrideScalemetaTag!,"
                        "pData is NULL", NodeIdentifierString());
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Node:%s Can't query vendor tag: OverrideIPEScaleProfile",
                NodeIdentifierString());
        }
        CAMX_LOG_VERBOSE(CamxLogGroupPProc, "For Node %s GetOverrideScaleProfile val %d", bOverrideIPEScale);
        return bOverrideIPEScale;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsValidDimension
    ///
    /// @brief  Check if input param is between min and max of IPE
    ///
    /// @param  pZoomWindow Dimension to check
    ///
    /// @return false if dimension is lesser the min or gretear them max
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsValidDimension(
        IpeZoomWindow* pZoomWindow);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetEISMarginPixels
    ///
    /// @brief  Get EIS margin in pixels
    ///
     /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetEISMarginPixels();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetEISMargin
    ///
    /// @brief  Get EIS requested margin
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetEISMargin();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStabilizationMargins
    ///
    /// @brief  Get the stabilization margins depending on the Image Stabilization type for this node
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetStabilizationMargins();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetScaleRatios
    ///
    /// @brief  Set Scale Ratios for IPE triggers
    ///
    /// @param  pInputData        Pointer to the ISP input data for setting updates
    /// @param  parentNodeID      Parent Node ID: IFE, JPEG, IPE, BPS, FDhw etc
    /// @param  pCropInfo         Pointer to the IFE Crop Information
    /// @param  pIFEScalerOutput  Pointer to the IFE Scaler Output
    /// @param  pCmdBuffer        Command Buffer
    /// @param  upscaleonStream   Upscale on stream instance
    ///
    /// @return TRUE if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL SetScaleRatios(
        ISPInputData*     pInputData,
        UINT              parentNodeID,
        CHIRectangle*     pCropInfo,
        IFEScalerOutput*  pIFEScalerOutput,
        CmdBuffer*        pCmdBuffer,
        BOOL              upscaleonStream);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckIsIPERealtime
    ///
    /// @brief  Returns true if IPE is part of a realtime stream
    ///
    /// @param  numPasses Number of passes enabled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckIsIPERealtime(
        UINT numPasses);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateIPEConfigIOTopology
    ///
    /// @brief  Helper method to update IPE config IO topology type
    ///
    /// @param  pConfigIOData    pointer to IPE Config IO Data
    /// @param  numPasses        number of input passes
    /// @param  profileId        IPE node property profile Id
    /// @param  processType      IPE node processType
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateIPEConfigIOTopology(
        IpeConfigIoData*    pConfigIOData,
        UINT32              numPasses,
        IPEProfileId        profileId,
        IPEProcessingType   processType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIPEConfigIOTopology
    ///
    /// @brief  Helper method to get the IPE config IO topology type
    ///
    /// @return IpeTopologyType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE IpeTopologyType GetIPEConfigIOTopology() const
    {
        IpeConfigIo*     pConfigIO;
        IpeConfigIoData* pConfigIOData;

        pConfigIO     = reinterpret_cast<IpeConfigIo*>(m_configIOMem.pVirtualAddr);
        pConfigIOData = &pConfigIO->cmdData;

        return pConfigIOData->topologyType;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMFProcessingType
    ///
    /// @brief  Helper method to determine if the processing type is multiframe - MFNR/MFSR
    ///
    /// @return TRUE if the processing type is multiframe - MFNR/MFSR.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsMFProcessingType() const
    {
        BOOL bMFProcessingType = FALSE;

        if ((IPEProcessingType::IPEMFNRPrefilter  == m_instanceProperty.processingType) ||
            (IPEProcessingType::IPEMFNRBlend      == m_instanceProperty.processingType) ||
            (IPEProcessingType::IPEMFNRScale      == m_instanceProperty.processingType) ||
            (IPEProcessingType::IPEMFNRPostfilter == m_instanceProperty.processingType) ||
            (IPEProcessingType::IPEMFSRPrefilter  == m_instanceProperty.processingType) ||
            (IPEProcessingType::IPEMFSRBlend      == m_instanceProperty.processingType) ||
            (IPEProcessingType::IPEMFSRPostfilter == m_instanceProperty.processingType))
        {
            bMFProcessingType = TRUE;
        }

        return bMFProcessingType;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMFSRProcessingType
    ///
    /// @brief  Helper method to determine if the processing type is multiframe - MFSR
    ///
    /// @return TRUE if the processing type is multiframe - MFSR.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsMFSRProcessingType() const
    {
        BOOL bMFSRProcessingType = FALSE;

        if ((IPEProcessingType::IPEMFSRPrefilter  == m_instanceProperty.processingType) ||
            (IPEProcessingType::IPEMFSRBlend      == m_instanceProperty.processingType) ||
            (IPEProcessingType::IPEMFSRPostfilter == m_instanceProperty.processingType))
        {
            bMFSRProcessingType = TRUE;
        }

        return bMFSRProcessingType;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSupportedResolutionforMFProcessing
    ///
    /// @brief  Helper method to determine if resolution is supported for the MF processing
    ///
    /// @return TRUE if the MF processing resolution is within the capabilities.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSupportedResolutionforMFProcessing() const
    {
        BOOL isValidResolution = TRUE;

        if ((TRUE == IsMFProcessingType())    &&
            (TRUE == m_capability.hasMFlimit) &&
            ((m_fullInputWidth > (static_cast<INT32>(m_capability.maxInputWidth[ICA_MODE_ENABLED]))) ||
            (m_fullInputHeight > (static_cast<INT32>(m_capability.maxInputHeight[ICA_MODE_ENABLED])))))
        {
            isValidResolution = FALSE;
        }

        return isValidResolution;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPrefilterWithNPS
    ///
    /// @brief  Helper method to determine if the processing type is Prefilter and ProfileID is NPS.
    ///
    /// @return TRUE if the processing type is Prefilter and ProfileID is NPS, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPrefilterWithNPS () const
    {
        BOOL bPrefilterWithNPS = FALSE;

        if ((IPEProfileId::IPEProfileIdNPS        == m_instanceProperty.profileId) &&
            ((IPEProcessingType::IPEMFNRPrefilter == m_instanceProperty.processingType) ||
             (IPEProcessingType::IPEMFSRPrefilter == m_instanceProperty.processingType)))
        {
            bPrefilterWithNPS = TRUE;
        }

        return bPrefilterWithNPS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBlendWithNPS
    ///
    /// @brief  Helper method to determine if the processing type is BLEND and ProfileID is NPS.
    ///
    /// @return TRUE if the processing type is BLEND and ProfileID is NPS, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsBlendWithNPS () const
    {
        BOOL bBlendWithNPS = FALSE;

        if (((IPEProcessingType::IPEMFNRBlend == m_instanceProperty.processingType) ||
             (IPEProcessingType::IPEMFSRBlend == m_instanceProperty.processingType)) &&
            (IPEProfileId::IPEProfileIdNPS   == m_instanceProperty.profileId))
        {
            bBlendWithNPS = TRUE;
        }

        return bBlendWithNPS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsValidInputPrespectiveParams
    ///
    /// @brief  Helper method to determine if the processing type is BLEND or ProfileID is NPS or EIS2 stabilization type.
    ///
    /// @return TRUE if the processing type is BLEND or ProfileID is NPS or stabilization type EIS2, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsValidInputPrespectiveParams () const
    {
        BOOL bValid = FALSE;

        if ((TRUE == IsBlendWithNPS())          ||
            (TRUE == IsPostfilterWithNPS())     ||
            (TRUE == IsStabilizationTypeEIS())  ||
            (TRUE == IsStabilizationTypeSAT()))
        {
            bValid = TRUE;
        }

        return bValid;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPostfilterWithDefault
    ///
    /// @brief  Helper method to determine if the Postfilter with Profile Default.
    ///
    /// @return TRUE if the processing type is Postfilter with Profile Default, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPostfilterWithDefault() const
    {
        BOOL bPostfilterWithDefault = FALSE;

        if (((IPEProcessingType::IPEMFNRPostfilter == m_instanceProperty.processingType) ||
             (IPEProcessingType::IPEMFSRPostfilter == m_instanceProperty.processingType)) &&
            (IPEProfileId::IPEProfileIdDefault    == m_instanceProperty.profileId))
        {
            bPostfilterWithDefault = TRUE;
        }
        return bPostfilterWithDefault;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPostfilterWithNPS
    ///
    /// @brief  Helper method to determine if the Postfilter with Profile NPS.
    ///
    /// @return TRUE if the processing type is Postfilter with Profile NPS, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPostfilterWithNPS() const
    {
        BOOL bPostfilterWithNPS = FALSE;

        if (((IPEProcessingType::IPEMFNRPostfilter == m_instanceProperty.processingType) ||
             (IPEProcessingType::IPEMFSRPostfilter == m_instanceProperty.processingType)) &&
            (IPEProfileId::IPEProfileIdNPS == m_instanceProperty.profileId))
        {
            bPostfilterWithNPS = TRUE;
        }

        return bPostfilterWithNPS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NeedICAGridFromInputPool
    ///
    /// @brief  Helper method to determine if need ICA grid from input metadata pool
    ///
    /// @return TRUE if need ICA grid from input metadata pool, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL NeedICAGridFromInputPool() const
    {
        BOOL bNeedICAGridFromInputPool = FALSE;

        if ((TRUE == IsStabilizationTypeSAT()) ||
            ((IPEProfileId::IPEProfileIdNPS   == m_instanceProperty.profileId) &&
             (IPEProcessingType::IPEMFSRBlend == m_instanceProperty.processingType)))
        {
            bNeedICAGridFromInputPool = TRUE;
        }

        return bNeedICAGridFromInputPool;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckForNonEISLDCGridDependency
    ///
    /// @brief  Helper method to determine if need to check non EIS LDC Grid dependency
    ///
    /// @return TRUE if ipe instance can have non EIS LDC Grid dependency, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL CheckForNonEISLDCGridDependency() const
    {
        BOOL                  bCheckForNonEISLDCGridDependency = FALSE;
        const StaticSettings* pStaticSettings                  = HwEnvironment::GetInstance()->GetStaticSettings();

        if ((TRUE == m_capability.LDCSupport) && (TRUE == pStaticSettings->enableLDC))
        {
            if (IPEStabilizationType::IPEStabilizationMCTF == m_instanceProperty.stabilizationType)
            {
                bCheckForNonEISLDCGridDependency = TRUE;
            }
            else if ((IPEStabilizationType::IPEStabilizationTypeNone == m_instanceProperty.stabilizationType) &&
                     (IPEProcessingType::IPEProcessingTypeDefault == m_instanceProperty.processingType) &&
                     (IPEProfileId::IPEProfileIdDefault == m_instanceProperty.profileId))
            {
                bCheckForNonEISLDCGridDependency = TRUE;
            }
            else if ((TRUE == IsPrefilterWithNPS()) ||
                     ((IPEProfileId::IPEProfileIdNPS         == m_instanceProperty.profileId) &&
                      ((IPEProcessingType::IPEMFNRBlend      == m_instanceProperty.processingType) ||
                       (IPEProcessingType::IPEMFNRPostfilter == m_instanceProperty.processingType))))
            {
                bCheckForNonEISLDCGridDependency = TRUE;
            }
            else if (IPEProfileId::IPEProfileIdICAWarpOnly == m_instanceProperty.profileId)
            {
                bCheckForNonEISLDCGridDependency = TRUE;
            }
        }

        return bCheckForNonEISLDCGridDependency;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMuxMode
    ///
    /// @brief  To Set the Mux Mode.
    ///
    /// @param  pConfigIOData    Pointer to the IPE Config IO data for setting updates
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetMuxMode(
        IpeConfigIoData* pConfigIOData)
    {
        if (CSLCameraTitanVersion::CSLTitan150 == static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion())
        {
            pConfigIOData->muxMode = 1;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsUBWCLossyNeededForLoopBackBuffer
    ///
    /// @brief  Check if need UBWC Format to be Lossy for Loopback buffers
    ///
    /// @param  pImageFormat    Pointer to outputPort Image format to access format and dimension info
    ///
    /// @return TRUE if need UBWC format to be Lossy, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsUBWCLossyNeededForLoopBackBuffer(
        ImageFormat* pImageFormat) const
    {
        const PlatformStaticCaps* pStaticCaps    = HwEnvironment::GetInstance()->GetPlatformStaticCaps();
        BOOL                      bNeedUBWCLossy = FALSE;

        if ((TRUE == ImageFormatUtils::IsUBWC(pImageFormat->format))       &&
            (UBWC_VERSION_3 <= static_cast<UbwcVersion>(pImageFormat->ubwcVerInfo.version))          &&
            (pStaticCaps->ubwcLossyVideoMinWidth  <= pImageFormat->width)  &&
            (pStaticCaps->ubwcLossyVideoMinHeight <= pImageFormat->height) &&
            (30 < m_FPS))
        {
            bNeedUBWCLossy = TRUE;
        }

        return bNeedUBWCLossy;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMCTFEnabledWithICA1
    ///
    /// @brief  Helper method to determine if the MCTF is enabled together with ICA1 activation.
    ///
    /// @return TRUE if the stabilization type is MCTF with ICA1, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsMCTFEnabledWithICA1() const
    {
        BOOL  bMCTFWithEIS  = FALSE;
        UINT32 stabType     = m_instanceProperty.stabilizationType;

        bMCTFWithEIS = ((0 != (IPEStabilizationMCTF  & stabType)) &&
                        (0 != ((IPEStabilizationTypeEIS2 | IPEStabilizationTypeEIS3 | IPEStabilizationTypeSAT) & stabType))) ?
                         TRUE : FALSE;

        return bMCTFWithEIS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsEISEnabled
    ///
    /// @brief  Helper method to determine if the EIS is enabled
    ///
    /// @return TRUE if the stabilization type is EIS, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsEISEnabled() const
    {
        BOOL   bEISEnabled = FALSE;
        UINT32 stabType    = m_instanceProperty.stabilizationType;

        bEISEnabled = (0 != ((IPEStabilizationTypeEIS2   |
                              IPEStabilizationTypeEIS3   |
                              IPEStabilizationTypeSWEIS2 |
                              IPEStabilizationTypeSWEIS3) & stabType)) ? TRUE : FALSE;

        return bEISEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSkipPostMetadata
    ///
    /// @brief  Helper method to determine whether the PostMetadata is skipped or not.
    ///
    /// @return TRUE if posting metadata need to be skipped, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSkipPostMetadata() const
    {
        BOOL  bSkipPostMetadata = FALSE;

        bSkipPostMetadata = ((TRUE == IsScalerOnlyIPE()) || (TRUE == IsICAOnlyIPE())) ? TRUE : FALSE;

        return bSkipPostMetadata;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsScalerOnlyIPE
    ///
    /// @brief  Helper method to determine if default processing with Profile Scale.
    ///
    /// @return TRUE if the processing type is default with Profile Scale, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsScalerOnlyIPE() const
    {
        BOOL bScalerOnlyIPE = FALSE;

        if (((IPEProcessingType::IPEProcessingTypeDefault == m_instanceProperty.processingType) ||
            (IPEProcessingType::IPEMFNRScale    == m_instanceProperty.processingType))          &&
            ((IPEProfileId::IPEProfileIdScale   == m_instanceProperty.profileId)                ||
            (IPEProfileId::IPEProfileIdUpscale  == m_instanceProperty.profileId)))
        {
            bScalerOnlyIPE = TRUE;
        }

        return bScalerOnlyIPE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsZSLNRLowResolution
    ///
    /// @brief  Helper method to Get the low resoltion
    ///
    /// @return TRUE if resolution less else FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsZSLNRLowResolution() const
    {
        BOOL bLowResolution = FALSE;

        if ((m_fullOutputWidth * m_fullOutputHeight) <= MinZSLNoiseReductionModeSize)
        {
            bLowResolution = TRUE;
        }

        return bLowResolution;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IPEInstanceNeedsBufferNegotiation
    ///
    /// @brief  Helper method to check conditions for IPE Node instance to participate in buffer negotiation
    ///
    /// @param  outputPortId    output port Id participating in buffer negotiation
    ///
    /// @return TRUE if the IPE Node instance to participate in buffer negotiation, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IPEInstanceNeedsBufferNegotiation(
        UINT outputPortId) const
    {
        BOOL bNeedsBufferNegotiation = FALSE;

        if (((IPEProcessingType::IPEProcessingTypeDefault != m_instanceProperty.processingType)  &&
            (IPEProcessingType::IPEProcessingPreview != m_instanceProperty.processingType))      ||
            (TRUE == IsScalerOnlyIPE())                                                          ||
            (TRUE == IsICAOnlyIPE())                                                             ||
            (FALSE == IsReferenceOutputPort(outputPortId)))
        {
            bNeedsBufferNegotiation = TRUE;
        }
        return bNeedsBufferNegotiation;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsICAOnlyIPE
    ///
    /// @brief  Helper method to determine if ICA only IPE.
    ///
    /// @return TRUE if the processing type is dICA only IPE enabled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsICAOnlyIPE() const
    {
        BOOL bICAOnlyIPE = FALSE;

        if ((IPEProcessingType::IPEProcessingTypeDefault == m_instanceProperty.processingType) &&
            ((IPEProfileId::IPEProfileIdICAWarpOnly == m_instanceProperty.profileId)           ||
            (IPEProfileId::IPEProfileIdScale == m_instanceProperty.profileId)))
        {
            bICAOnlyIPE = TRUE;
        }

        return bICAOnlyIPE;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // InitializeUBWCStatsBuf
    ///
    /// @brief  Initialize UBWC Stat Buffer
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeUBWCStatsBuf();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // GetUBWCStatBuffer
    ///
    /// @brief  Get the pointer to next UBWC Stat Buffer
    ///
    /// @param  pBuf         Pointer to the next availabe buffer
    /// @param  requestId    Request ID associated with the buffer
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetUBWCStatBuffer(
        CSLBufferInfo*  pBuf,
        UINT64          requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateIntermediateSize
    ///
    /// @brief  Calculate Intermediate Size
    ///
    /// @param  pInputData        Pointer to the ISP input data for setting updates
    /// @param  pCropInfo         Pointer to the IFE Crop Information
    ///
    /// @return intermediateRatio
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FLOAT CalculateIntermediateSize(
        ISPInputData*     pInputData,
        CHIRectangle*     pCropInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpConfigIOData
    ///
    /// @brief  Dump Config IO data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpConfigIOData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpTuningModeData
    ///
    /// @brief  Dump Tuning Mode Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpTuningModeData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ApplyStatsFOVCCrop
    ///
    /// @brief     Apply FOVC as requested by stats
    ///
    /// @param     pCropInfo        IFE residual crop info
    /// @param     parentNodeId     parent Node Id
    /// @param     fullInputWidth   full input width
    /// @param     fullInputHeight  full input height
    /// @param     requestId        request Id
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ApplyStatsFOVCCrop(
        CHIRectangle* pCropInfo,
        UINT          parentNodeId,
        UINT32        fullInputWidth,
        UINT32        fullInputHeight,
        UINT64        requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OverrideConfigIOData
    ///
    /// @brief  Update Config IO parmaeters
    ///
    /// @param  pConfigIOData     Pointer to configIO data
    /// @param  pImageFormat      Pointer to Image format
    /// @param  firmwarePortId    Parameter referring firmwarePortID
    /// @param  parentNodeId      Parameter referring parentNodeId
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult OverrideConfigIOData(
        IpeConfigIoData*   pConfigIOData,
        const ImageFormat* pImageFormat,
        IPE_IO_IMAGES      firmwarePortId,
        UINT               parentNodeId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetConfigIOData
    ///
    /// @brief  Set Config IO parmaeters
    ///
    /// @param  pConfigIOData     Pointer to configIO data
    /// @param  pImageFormat      Pointer to Image format
    /// @param  firmwarePortId    Parameter referring firmwarePortID
    /// @param  pPortType         Name holding the port type
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetConfigIOData(
        IpeConfigIoData*   pConfigIOData,
        const ImageFormat* pImageFormat,
        IPE_IO_IMAGES      firmwarePortId,
        const CHAR*        pPortType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ComputeAndSetConfigIOData
    ///
    /// @brief  Compute and Set Config IO parmaeters
    ///
    /// @param  pConfigIOData     Pointer to configIO data
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ComputeAndSetConfigIOData(
        IpeConfigIoData*   pConfigIOData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeleteLoopBackBufferManagers
    ///
    /// @brief  delete Loop back Buffer Manager
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DeleteLoopBackBufferManagers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateLoopBackBufferManagers
    ///
    /// @brief  Create Loop back Buffer Managers
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateLoopBackBufferManagers();
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeleteDownscaleBufferManagers
    ///
    /// @brief  delete Downscale Buffer Manager
    ///
    /// @param  pMultipassBufferParams   Buffer parameters of DS ports
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DeleteDownscaleBufferManagers(
        MultipassBufferParams*  pMultipassBufferParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateDownscaleBufferManagers
    ///
    /// @brief  Create Downscale Buffer Managers
    ///
    /// @param  pPortType                 Input/ Reference
    /// @param  pMultipassBufferParams    Buffer parameters of DS ports
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateDownscaleBufferManagers(
        const CHAR*             pPortType,
        MultipassBufferParams*  pMultipassBufferParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillDownscaleBufferManagers
    ///
    /// @brief  Fill Downscale frame buffer data
    ///
    /// @param  requestId                 Request ID parameters
    /// @param  pFrameProcessCmdBuffer    frame process command buffer
    /// @param  pIPEIQsettings            IQ setting pointer
    /// @param  pMultipassBufferParams    Buffer parameters of DS ports
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillDownscaleBufferManagers(
        UINT64                  requestId,
        CmdBuffer*              pFrameProcessCmdBuffer,
        IpeIQSettings*          pIPEIQsettings,
        MultipassBufferParams*  pMultipassBufferParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///  NeedInternalMultipass
    ///
    /// @brief  Check if IPE has only full input port to add reference buffers
    ///
    /// @param  passes       Number of passes
    /// @param  batchSize    batch Size

    /// @return return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL  NeedInternalMultipass(
        UINT32 passes,
        UINT32 batchSize);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateFromDownscaleFlag
    ///
    /// @brief  Update the from downscale flag for each port
    ///
    /// @param  pConfigIOData       Ipe IO configuration data for firmware
    /// @param  firmwarePortId      firmware port ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateFromDownscaleFlag(
        IpeConfigIoData*   pConfigIOData,
        IPE_IO_IMAGES      firmwarePortId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeleteDS4LENRBuffers
    ///
    /// @brief  Delete LENR buffers
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DeleteDS4LENRBuffers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateDS4LENRBuffers
    ///
    /// @brief  Allocate LENR scratch buffers
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocateDS4LENRBuffers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateScratchBuffers
    ///
    /// @brief  Allocate LENR scratch buffers
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocateScratchBuffers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PatchDS4LENRBuffers
    ///
    /// @brief  Patch LENR DS4 buffers
    ///
    /// @param  requestId                 Request ID parameters
    /// @param  pFrameProcessCmdBuffer    frame process command buffer
    /// @param  pIPEIQsettings            IQ setting pointer
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PatchDS4LENRBuffers(
        UINT64          requestId,
        CmdBuffer*      pFrameProcessCmdBuffer,
        IpeIQSettings*  pIPEIQsettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PatchScratchBuffers
    ///
    /// @brief  Patch Scratch buffers
    ///
    /// @param  requestId                 Request ID parameters
    /// @param  pFrameProcessCmdBuffer    frame process command buffer
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PatchScratchBuffers(
        UINT64          requestId,
        CmdBuffer*      pFrameProcessCmdBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillLoopBackFrameBufferData
    ///
    /// @brief  Fill Loop back frame buffer data
    ///
    /// @param  requestId                 Request ID parameters
    /// @param  firstValidRequest         first valid Request
    /// @param  pFrameProcessCmdBuffer    frame process command buffer
    /// @param  numFramesInBuffer         number of frames in batch buffer
    /// @param  pPerRequestPorts          Pointer to per request active port data
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillLoopBackFrameBufferData(
        UINT64                  requestId,
        UINT                    firstValidRequest,
        CmdBuffer*              pFrameProcessCmdBuffer,
        UINT32                  numFramesInBuffer,
        PerRequestActivePorts*  pPerRequestPorts);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFWCommandBufferManagers
    ///
    /// @brief  Create Command buffer mangers which are needed by FW
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateFWCommandBufferManagers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendFWCmdRegionInfo
    ///
    /// @brief  Maps the firmware command buffer region to Kernel
    ///
    /// @param  opcode           Determines whther to map or unmap
    /// @param  ppBufferInfo     Pointer to the Buffer informaion pointer to be mapped to FW
    /// @param  numberOfMappings number of memory mappings
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SendFWCmdRegionInfo(
        UINT32          opcode,
        CSLBufferInfo** ppBufferInfo,
        UINT32          numberOfMappings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFWBufferAlignedSize
    ///
    /// @brief  Calculates the FW accssible buffer alignment
    ///
    /// @param  sizeInBytes size of the memory in bytes, which is to be aligned
    ///
    /// @return return aligned buffer size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE SIZE_T GetFWBufferAlignedSize(
        SIZE_T sizeInBytes)
    {
        SIZE_T alignment  = 0;
        SIZE_T bytesPerKb = 1024;

        if (sizeInBytes < (4 * bytesPerKb))
        {
            alignment = 4 * bytesPerKb;
        }
        else
        {
            if (0 != (sizeInBytes % (64 * bytesPerKb)))
            {
                alignment  = ((sizeInBytes / (64 * bytesPerKb)) + 1) * (64 * bytesPerKb);
            }
            else
            {
                alignment = sizeInBytes;
            }
        }

        return alignment;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillCmdBufferParams
    ///
    /// @brief  Packs the Resource Params with the values specified
    ///
    /// @param  pParams           Pointer to the Resource Params structure that needs to be filled
    /// @param  resourceSize      size of the resource
    /// @param  type              Type of the resource
    /// @param  memFlags          Memory Flags of the resource
    /// @param  maxNumNestedAddrs max number of nested buffers
    /// @param  pDeviceIndex      Pointer to the Devices of this resource
    /// @param  blobCount         Number of resources required
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID FillCmdBufferParams(
        ResourceParams* pParams,
        UINT            resourceSize,
        CmdType         type,
        UINT32          memFlags,
        UINT            maxNumNestedAddrs,
        const INT32*    pDeviceIndex,
        UINT            blobCount)
    {
        *pParams                                = { 0 };
        pParams->resourceSize                   = resourceSize;
        pParams->poolSize                       = blobCount*resourceSize;
        pParams->usageFlags.cmdBuffer           = 1;
        pParams->cmdParams.type                 = type;
        pParams->alignment                      = CamxCommandBufferAlignmentInBytes;
        pParams->cmdParams.enableAddrPatching   = (maxNumNestedAddrs > 0) ? 1 : 0;
        pParams->cmdParams.maxNumNestedAddrs    = maxNumNestedAddrs;
        pParams->memFlags                       = memFlags;
        pParams->pDeviceIndices                 = pDeviceIndex;
        pParams->numDevices                     = (pParams->pDeviceIndices != NULL) ? 1 : 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // GetScratchBufferSize
    ///
    /// @brief  Get scratch buffer size
    ///
    /// @param  pScratchBufferData    Pointer to scratch buffer data
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetScratchBufferSize(
        ScratchBufferData* pScratchBufferData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // PublishScratchBufferData
    ///
    /// @brief  Publish scratch buffer data
    ///
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishScratchBufferData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // UpdateTuningModeData
    ///
    /// @brief  Update tuning mode data
    ///
    /// @param  pTuningModeData    tining mode data
    /// @param  pModuleInput       ISP module input
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateTuningModeData(
        ChiTuningModeParameter* pTuningModeData,
        ISPInputData*           pModuleInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // GetScratchBufferData
    ///
    /// @brief  Publish scratch buffer data
    ///
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetScratchBufferData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // UpdateZoomWindowForStabilization
    ///
    /// @brief  Update zoom window when stabilization is enabled
    ///
    /// @param  pCropInfo           Pointer to crop rect
    /// @param  pAdjustedFullWidth  Pointer to adjusted full width
    /// @param  pAdjustedFullHeight Pointer to adjusted full height
    /// @param  requestId           Request id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateZoomWindowForStabilization(
        CHIRectangle*   pCropInfo,
        UINT32*         pAdjustedFullWidth,
        UINT32*         pAdjustedFullHeight,
        UINT64          requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSizeWithoutStablizationMargin
    ///
    /// @brief  Fill the full size without margins
    ///
    /// @param  widthWithMargin           width with margin
    /// @param  heightWithMargin          height with margin
    /// @param  pWidthWithoutMargin       Pointer to be filled with width without margin
    /// @param  pHeightWithoutMargin      Pointer to be filled with height without margin
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID GetSizeWithoutStablizationMargin(
        UINT32      widthWithMargin,
        UINT32      heightWithMargin,
        UINT32*     pWidthWithoutMargin,
        UINT32*     pHeightWithoutMargin)
    {
        if ((m_stabilizationMargin.widthPixels <= widthWithMargin) &&
            (m_stabilizationMargin.heightLines <= heightWithMargin))
        {
            *pWidthWithoutMargin  = widthWithMargin  - m_stabilizationMargin.widthPixels;
            *pHeightWithoutMargin = heightWithMargin - m_stabilizationMargin.heightLines;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInSensorSeamlessControltState
    ///
    /// @brief  Get the seamless in-sensor control state from vendor tags
    ///
    /// @param  pInputData Pointer to the input data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetInSensorSeamlessControltState(
        ISPInputData*   pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsStabilizationTypeEIS
    ///
    /// @brief  Helper method to check if IPE node stabilization type is EIS2 or EIS3
    ///
    /// @return True if IPE node stabilization type is EIS2 or EIS3, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsStabilizationTypeEIS() const
    {
        BOOL bStabilizationTypeEIS = FALSE;

        if ((0 != (IPEStabilizationType::IPEStabilizationTypeEIS2 & m_instanceProperty.stabilizationType)) ||
            (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType)))
        {
            bStabilizationTypeEIS = TRUE;
        }

        return bStabilizationTypeEIS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsStabilizationTypeSAT
    ///
    /// @brief  Helper method to check if IPE node stabilization type is SAT
    ///
    /// @return True if IPE node stabilization type is SAT, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsStabilizationTypeSAT() const
    {
        BOOL bStabilizationTypeSAT = FALSE;

        if (0 != (IPEStabilizationType::IPEStabilizationTypeSAT & m_instanceProperty.stabilizationType))
        {
            bStabilizationTypeSAT = TRUE;
        }

        return bStabilizationTypeSAT;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPortLinkICATransformDependency
    ///
    /// @brief  Set Port link metadata based ICA perspective and grid transform dependency
    ///
    /// @param  pNodeRequestData          Pointer to Node request data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetPortLinkICATransformDependency(
        NodeProcessRequestData* pNodeRequestData)
    {
        const Titan17xStaticSettings* pTitan17xStaticSettings = static_cast<Titan17xContext*>(
            GetHwContext())->GetTitan17xSettingsManager()->GetTitan17xStaticSettings();
        if (TRUE == IsStabilizationTypeSAT())
        {
            // Set SAT In perspective transform port link metadata dependency
            SetPSMetadataDependency(m_IPEICATAGLocation[0], IPEInputPortFull, 0, pNodeRequestData);
        }
        else if (TRUE == IsStabilizationTypeEIS())
        {
            // Set EIS2, EIS3 In perspective transform port link metadata dependency
            SetPSMetadataDependency(m_IPEICATAGLocation[0], IPEInputPortFull, 0, pNodeRequestData);
            if (TRUE == pTitan17xStaticSettings->enableICAInGrid)
            {
                // Set EIS2, EIS3 In grid out2in transform port link metadata dependency
                SetPSMetadataDependency(m_IPEICATAGLocation[1], IPEInputPortFull, 0, pNodeRequestData);
            }

            if (TRUE == IsLoopBackPortEnabled())
            {
                // Set EIS gyro based ICA referenceparam alignment matrice port link dependency
                SetPSMetadataDependency(m_IPEICATAGLocation[7], IPEInputPortFull, 0, pNodeRequestData);
            }
        }
        else if ((TRUE == CheckForNonEISLDCGridDependency()) &&
            (TRUE == pTitan17xStaticSettings->enableICAInGrid))
        {
            // Set non-EIS LDC grid out2in transform port link metadata dependency
            SetPSMetadataDependency(m_IPEICATAGLocation[1], IPEInputPortFull, 0, pNodeRequestData);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsvalidRefFormat
    ///
    /// @brief  Helper method to check Full ref port format
    ///
    /// @param  outputFormat Ref output format set through override setting
    /// @param  rFormat      camx format for ref port
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID IsvalidRefFormat(
        RefOutputFormatType outputFormat,
        Format&          rFormat)
    {
        switch (outputFormat)
        {
            case RefOutputFormatUBWCTP10:
                rFormat = Format::UBWCTP10;
                break;
            case RefOutputFormatUBWCNV124R:
                rFormat = Format::UBWCNV124R;
                break;
            case RefOutputFormatYUV420NV12:
                rFormat = Format::YUV420NV12;
                break;
            case RefOutputFormatYUV420NV12TP10:
                rFormat = Format::YUV420NV12TP10;
                break;
            case RefOutputFormatP010:
                rFormat = Format::P010;
                break;
            default:
                rFormat = Format::UBWCTP10;
                break;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAlignmentFusion
    ///
    /// @brief  Get alignment fusion
    ///
    /// @param  pImageAlignmentReferenceParams Image alignment reference params
    /// @param  pGyroAlignmentReferenceParams  Gyro alignment reference params
    /// @param  pInputData                     Pointer to the input data
    ///
    /// @return Selected alignment params
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IPEICAPerspectiveTransform* GetAlignmentFusion(
        IPEICAPerspectiveTransform* pImageAlignmentReferenceParams,
        IPEICAPerspectiveTransform* pGyroAlignmentReferenceParams,
        ISPInputData*               pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGeoLibStillFrameConfiguration
    ///
    /// @brief  This function gets the GeoLib still frame configuation for MFSR/MFNR
    ///
    /// @param  requestId               Current request Id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetGeoLibStillFrameConfiguration(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetGeoLibStillFrameConfigurationDependency
    ///
    /// @brief  This function set the dependency for the GeoLib frame configuration
    ///
    /// @param  pNodeRequestData          Pointer to Node request data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetGeoLibStillFrameConfigurationDependency(
        NodeProcessRequestData*  pNodeRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentStillFrameConfigurationTagId
    ///
    /// @brief  This function get the current tag id for the IPE node
    ///
    /// @param  pTagId                   Getlib vendor tag Id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCurrentStillFrameConfigurationTagId(
        UINT32* pTagId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpGeolibResult
    ///
    /// @brief  This function dumps the geolib result
    ///
    /// @param  requestId               current request Id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpGeolibResult(
        UINT64      requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputPortImageBuffer
    ///
    /// @brief  Get input port image buffer
    ///
    /// @param  inputPortId       input port id
    /// @param  pPerRequestPorts  Pointer to per request active port info
    ///
    /// @return Input puffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ImageBuffer* GetInputPortImageBuffer(
        UINT                    inputPortId,
        PerRequestActivePorts*  pPerRequestPorts)
    {
        ImageBuffer* pInputPortImageBuffer = NULL;
        for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
        {
            PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];
            if (pInputPort->portId == inputPortId)
            {
                pInputPortImageBuffer = pInputPort->pImageBuffer;
                break;
            }
        }

        return pInputPortImageBuffer;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputPortIdFromInputReferencePortId
    ///
    /// @brief  Get Input port Id from reference input port id
    ///
    /// @param  refInputPortID input reference port id
    ///
    /// @return input port id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetInputPortIdFromInputReferencePortId(
        IPE_IO_IMAGES refInputPortID)
    {
        UINT inputPortId = IPENodePortMaxNum;

        switch (refInputPortID)
        {
            case IPE_INPUT_IMAGE_FULL_REF:
                inputPortId = IPEInputPortFull;
                break;
            case IPE_INPUT_IMAGE_DS4_REF:
                inputPortId = IPEInputPortDS4;
                break;
            case IPE_INPUT_IMAGE_DS16_REF:
                inputPortId = IPEInputPortDS16;
                break;
            case IPE_INPUT_IMAGE_DS64_REF:
                inputPortId = IPEInputPortDS64;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid reference input port id");
                break;
        }

        return inputPortId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorModeData
    ///
    /// @brief  Get the Sensor mode for a requestId
    ///
    ///
    /// @return SensorMode      pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const SensorMode* GetSensorModeData();

    static const UINT   MaxIPEIQModule       = 30;    ///< Max Number of IQ Modules
    static const UINT   MaxScratchBuffer     = 1;     ///< Max Number of Scratch/Internal Buffers
    static const UINT   IPEMaxUBWCOutput     = 2;     ///< Max Number of UBWC Output Ports: Display and Video

    UINT                      m_IPECmdBlobCount;                          ///< Based on req queue depth or for MFNR few bufs
    CmdBufferManager*         m_pIQPacketManager;                         ///< IQ Packet buffer manager
    CmdBufferManager*         m_pIPECmdBufferManager[IPECmdBufferMaxIds]; ///< Array Cmd buffer managers for IPE node
    CSLBufferInfo*            m_pScratchMemoryBuffer[MaxScratchBuffer];   ///< Pointer to array of LENR scratch buffer
    CSLBufferInfo*            m_pLENRScratchMemoryBuffer[MaxScratchBuffer];   ///< Pointer to array of scratch buffers
    IPEUBWCStatBufInfo        m_UBWCStatBufInfo;                          ///< UBWC stats buffer information
    INT32                     m_deviceIndex;                              ///< ICP device index
    CSLDeviceHandle           m_hDevice;                                  ///< IPE device handle
    ISPIQModule*              m_pEnabledIPEIQModule[MaxIPEIQModule];      ///< List of IQ Modules
    CSLVersion                m_version;                                  ///< IPE Hardware Revision
    UINT32                    m_titanVersion;                             ///< Titan version
    IPECapabilityInfo         m_capability;                               ///< IPE Capability Configuration
    UINT                      m_numIPEIQModulesEnabled;                   ///< Number of IPE IQ Modules
    UINT                      m_firmwareScratchMemSize;                   ///< Maximum Size of scratch buffer needed by FW
    UINT                      m_numScratchBuffers;                        ///< Number of Scratch/Internal buffers
    UINT                      m_numLENRScratchBuffers;                    ///< Number of Scratch/Internal buffers
    UINT                      m_maxCmdBufferSizeBytes[IPECmdBufferMaxIds];///< Array Cmd buffer managers for IPE node
    UINT32                    m_preLTMLUTOffset[ProgramIndexMaxPreLTM];   ///< Array of offsets within DMI Header cmd buffer
    UINT32                    m_postLTMLUTOffset[ProgramIndexMaxPostLTM]; ///< Array of offsets within DMI Header cmd buffer
    UINT32                    m_preLTMLUTCount[ProgramIndexMaxPreLTM];    ///< Array of number of Look up tables in IQ blocks
    UINT32                    m_postLTMLUTCount[ProgramIndexMaxPostLTM];  ///< Array of number of Look up tables in IQ blocks
    SL_HANDLE                 m_hStripingLib;                             ///< Stripe library handle
    static FrameBuffers       s_frameBufferOffset[IPEMaxSupportedBatchSize][IPE_IO_IMAGES_MAX];   ///< Offset to buffer ptr base
    UINT32                    m_ANRPassOffset[PASS_NAME_MAX];             ///< Array of offsets for anr passes in cmd buffer
    UINT32                    m_ANRSinglePassCmdBufferSize;               ///< Size of single ANR pass cmd buffer
    BOOL                      m_isStatsNodeAvailable;                     ///< True when stats node available and enabled
    IPETuningMetadata*        m_pTuningMetadata;                          ///< Metadata for tuning support
    TuningDataManager*        m_pPreTuningDataManager;                    ///< Store previoustuningDataManager
                                                                          /// to compare if changed
    BOOL                      m_OEMStatsConfig;                           ///< Variable to track OEM controlled stats
    INT32                     m_fullInputWidth;                           ///< Width of the full input image in pixels.
    INT32                     m_fullInputHeight;                          ///< Height of the full input image in pixels.
    INT32                     m_fullOutputWidth;                          ///< Width of the full output image in pixels.
    INT32                     m_fullOutputHeight;                         ///< Height of the full output image in pix
    IPEInstanceProperty       m_instanceProperty;                         ///< IPE Node Instance Property
    CHIRectangle              m_previousCropInfo;                         ///< Last crop info from IFE
    ISPHALTagsData            m_HALTagsData;                              ///< Keep all input coming from tags
    // Region                    m_previousHALZoomCropRegion;               ///< Last HAL zoom crop region
    UINT                      m_FPS;                                      ///< FPS requested
    UINT                      m_maxBatchSize;                             ///< Number of frames in a batch
    UINT32                    m_ICALUTOffset[ProgramIndexICAMax];         ///< Number of LUTs for each instance of ICA
    UINT                      m_ICALUTCount[ProgramIndexICAMax];          ///< LUT Count for ICA1
    UINT32                    m_TFPassOffset[PASS_NAME_MAX];              ///< Array of offsets for tf passes in cmd buffer
    UINT32                    m_TFSinglePassCmdBufferSize;                ///< Size of single TF pass cmd buffer
    UINT32                    m_numOutputRefPorts;                        ///< number of output reference ports enabled in IPE
    BOOL                      m_OEMICASettingEnable;                      ///< indicator for OEM ICA Setting Mode
    BOOL                      m_OEMIQSettingEnable;                       ///< indicator for OEM IQ Setting Mode
    BOOL                      m_OEMStatsSettingEnable;                    ///< indicator for OEM Stats Setting Mode
    UINT                      m_IPEICATAGLocation[IPEICATagNums];         ///< ICA Tags
    UINT                      m_MFNRTotalNumFramesTAGLocation;            ///< MFNR Total number frames
    UINT                      m_MFSRTotalNumFramesTAGLocation;            ///< MFSR Total number frames
    UINT                      m_MFNRBlendFrameNumTAGLocation;            ///< MFNR Total number frames
    UINT                      m_MFSRBlendFrameNumTAGLocation;            ///< MFSR Total number frames
    UINT                      m_numPasses;                                ///< Number of IPE passes
    UINT                      m_inputPortIPEPassesMask;                   ///< input port id mask for IPE passes
    ISPInternalData           m_ISPData;                                  ///< Data Calculated by IQ Modules
    FLOAT                     m_prevFOVC;                                 ///< Last value of FOV compantation
    UINT                      m_enableIPEHangDump;                        ///<  IPE hangdump
    BOOL                      m_enableIPEStripeDump;                      ///<  IPE stripe dump
    UINT                      m_referenceBufferCount;                     ///<  IPE reference buffer count
    BOOL                      m_createdDownscalebuffers;                  ///< To check if downscale buffer are created
    DebugDataWriter*          m_pDebugDataWriter;                         ///< Pointer to the debug data pointer
    UINT                      m_firstValidRequest;                        ///< first valid request
    ImageDimensions           m_additionalCropOffset;                     ///< EIS additional crop offset
    StabilizationMargin       m_stabilizationMargin;                      ///< ICA margin Dimensions
    MarginRequest             m_EISMarginRequest;                         ///< EIS requested margin as a percentage
    ChiTuningModeParameter    m_tuningData;                               ///< Tuning data
    BOOL                      m_nodePropDisableZoomCrop;                  ///< Disables the zoom crop so IPE only outputs
    BOOL                      m_realTimeIPE;                              ///< IPE needs to run realtime
    BOOL                      m_compressiononOutput;                      ///< Compression enabled on output
    ADRCData                  m_adrcInfo;                                 ///< Adrc Info get from IFE/BPS
    CSLDeviceResource         m_deviceResourceRequest;                    ///< Copy of device resource request for
                                                                          /// later Config IO update
    CSLBufferInfo             m_configIOMem;                              ///< Config IO memory
    ConfigIORequest           m_configIORequest;                          ///< Config IO Request
    IntermediateDimensions    m_curIntermediateDimension;                 ///< Current Intermediate Dimension in MFSR usecase
    IntermediateDimensions    m_prevIntermediateDimension;                ///< Previous Intermediate Dimension in MFSR usecase
    FLOAT                     m_maxICAUpscaleLimit;                       ///< Maximum ICA Upscale Limit
    UINT32                    m_cameraId;                                 ///< Current camera id
    BOOL                      m_camIdChanged;                             ///< Flag to check if camera ID has changed or not
    FLOAT                     m_IPEClockEfficiency;                       ///< Clock efficiency based on number of IPE
    UINT64                    m_currentrequestID;                         ///< CurrentRequestID;
    BOOL                      m_loopbackPortEnable;                       ///< Loop back ports in IPE is enabled
    LoopBackBufferParams      m_loopBackBufferParams[PASS_NAME_MAX];      ///< Loop back buffer params
    MultipassBufferParams     m_multipassBufferParams[PASS_NAME_MAX-1];   ///< Multipass internal buffer params
    ISPPipeline*              m_pISPPipeline;                             ///< Pipeline object based on HW version
    BYTE*                     m_pIPEGenericClockAndBandwidthData;         ///< Clock and bandwidth data
    GeoLibParameters          m_geolibData;                               ///< Geo library data
    UINT32                    m_IPETimeoutInNumFrames;                    ///< IPE max timeout duration in number of frames
    UINT32                    m_enableIPEIQModulesMask;                   ///< IPE IQ modules enable mask
    BOOL                      m_scratchBufferPortEnabled;                 ///< scratch buffer port enabled
    BOOL                      m_IPESIMOMode;                              ///< IPE simo enabled for the instance
    NcLibAlignmentType        m_currentAlignmentType;                     ///< Current alignment type
    uint32_t                  m_currentAlignmentAge;                      ///< Current alignment age
    // Added for Geolib
    UINT32                    m_stillFrameConfigurationTagIdPrefilter;    ///< Tag Location for prefilter
    UINT32                    m_stillFrameConfigurationTagIdBlending;     ///< Tag Location for blenging
    UINT32                    m_stillFrameConfigurationTagIdPostfilter;   ///< Tag Location for postfilter
    BOOL                      m_dumpGeolibResult;                         ///< flag to enable the GeoLib dump

    ScratchBufferData         m_scratchBufferData;                        ///< scratch buffer data
    UINT32                    m_scratchBufferDataLocation;                ///< scratch buffer data location
    IpeOperationMode          m_frameOpMode;                              ///< Parameter to continue or override the
                                                                          ///< stream priority for a specific frame

    OSLIBRARYHANDLE           m_hHandle;                                  ///< Handle for loadPrebuiltLibrary fn
    IpeStripingLibraryContextCreate_t            m_funcPtrIPECreate;              ///< IPE striping lib context create
    IpeStripingLibraryContextDestroy_t           m_funcPtrIPEDestroy;             ///< IPE striping lib context destroy
    IpeStripingLibraryExecute_t                  m_funcPtrIPEExecute;             ///< IPE striping lib context execute
    GetStripingLibraryVersion_t                  m_funcPtrIPEGetStripingLibVer;   ///< IPE striping lib version
    IpeStripingLibraryCalculateScratchBufSize_t  m_funcPtrIPECalcScratchBufSize;  ///< striping lib calc Scratch BufSize

    UINT32                    m_fdDataOffset;                             ///< fd data metadata offset
};

CAMX_NAMESPACE_END

#endif // CAMXIPENODE_H
