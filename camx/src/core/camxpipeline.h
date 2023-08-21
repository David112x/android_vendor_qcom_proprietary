////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxpipeline.h
/// @brief Pipeline class declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXPIPELINE_H
#define CAMXPIPELINE_H

// NOWHINE FILE PR007b: Whiner incorrectly concludes as non-library files
// NOWHINE FILE CP006: used standard libraries for performance improvements
// NOWHINE FILE CP003: C++ Structure used internally for STL

#include <vector>
#include <unordered_set>

#include "camxatomic.h"
#include "camxchicontext.h"
#include "camxcommontypes.h"
#include "camxdeferredrequestqueue.h"
#include "camxdefs.h"
#include "camxhal3types.h"
#include "camximageformatutils.h"
#include "camxsession.h"
#include "camxsettingsmanager.h"
#include "camxthreadmanager.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constant Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT32 LivePendingRequestTimeoutDefault  = 800;          ///< Default live pending request time out
static const UINT32 LivePendingRequestTimeOutExtendor = 100;          ///< Live pending request timeout extendor

/// Forward Declarations
enum class ChiFormatType;

class  ExternalSensor;
class  HwContext;
class  HwFactory;
class  ImageBufferManager;
class  MetadataPool;
class  Node;
class  Pipeline;
class  Session;
class  CmdBuffer;
class  CmdBufferManager;

struct NodeProcessRequestData;
struct Usecase;

/// @brief Pipeline status
enum class PipelineStatus
{
    UNINITIALIZED,              ///< initial status
    INITIALIZED,                ///< Status after pipeline initialize
    FINALIZED,                  ///< Status after pipeline finalization
    RESOURCES_RELEASED,         ///< Status stream resources released
    STREAM_OFF,                 ///< Status stream off
    RESOURCES_ACQUIRED,         ///< Status stream resources acquired
    PARTIAL_STREAM_ON,          ///< Status partial stream on
    STREAM_ON,                  ///< Status stream on
    MAX_PIPELINE_CREATE_STATUS  ///< Maximum status
};

/// @brief The string name of the type of the memory allocation. Must be in order of PipelineStatus codes.
#if __GNUC__
static const CHAR* PipelineStatusStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* PipelineStatusStrings[] =
#endif // _GNUC__
{
    "PipelineStatus::UNINITIALIZED",
    "PipelineStatus::INITIALIZED",
    "PipelineStatus::FINALIZED",
    "PipelineStatus::RESOURCES_RELEASED",
    "PipelineStatus::STREAM_OFF",
    "PipelineStatus::RESOURCES_ACQUIRED",
    "PipelineStatus::PARTIAL_STREAM_ON",
    "PipelineStatus::STREAM_ON",
    "PipelineStatus::MAX_PIPELINE_CREATE_STATUS",
};

CAMX_STATIC_ASSERT(CAMX_ARRAY_SIZE(PipelineStatusStrings) == (static_cast<UINT>(PipelineStatus::MAX_PIPELINE_CREATE_STATUS)+1));

/// @brief Pipeline flags
struct PipelineFlags
{
    union
    {
        struct
        {
            BIT hasStatsNode            : 1;    ///< Is there a stats node in this pipeline or not
            BIT isSecureMode            : 1;    ///< Is secure mode
            BIT enableQTimer            : 1;    ///< Use Qtimer
            BIT isInitialConfigPending  : 1;    ///< Is initial config pending
            BIT isHFRMode               : 1;    ///< Is HFR Mode
            BIT hasIFENode              : 1;    ///< Is there an IFE node in this pipeline or not
            BIT hasJPEGNode             : 1;    ///< Is there a JPEG (Hw or Aggr) node in this pipeline or not
            BIT isCameraRunningOnBPS    : 1;    ///< Indicate if camera is running on BPS instead of IFE
            BIT reserved                : 24;   ///< Reserved fields
        };
        UINT32 allFlagsValue;           ///< Value of all flags combined
    };
};

/// @brief Pipeline finalize create data
struct FinalizeInitializationData
{
    HwContext*               pHwContext;                     ///< Hw context pointer
    DeferredRequestQueue*    pDeferredRequestQueue;          ///< DRQ
    MetadataPool*            pDebugDataPool;                 ///< Debug-data request data pool pointer
    Session*                 pSession;                       ///< Session
    ThreadManager*           pThreadManager;                 ///< Pointer to Thread Manager
    UINT                     usecaseNumBatchedFrames;        ///< Number of batched frames in a usecase
    BOOL                     HALOutputBufferCombined;        ///< Is the HAL output buffer combined for batch mode
    UINT                     numSessionPipelines;            ///< Number of pipelines in the Session that this pipeline belongs
    BIT                      enableQTimer;                   ///< Qtimer flag
    const ChiSensorModeInfo* pSensorModeInfo;                ///< Sensor mode
    JobHandle                hThreadJobFamilyHandle;         ///< Job handle to be used for job submission
    UINT32                   resourcePolicy;                 ///< Prefered resource vs power trade-off
};

/// @brief Pipeline create input data
struct PipelineCreateInputData
{
    ChiContext*               pChiContext;                  ///< ChiContext pointer
    const PipelineDescriptor* pPipelineDescriptor;          ///< Pipeline descriptor
    UINT                      pipelineIndex;                ///< Pipeline Index from the Session
    BOOL                      isRealTime;                   ///< Is it a real time pipeline

    BOOL                      isSecureMode;                 ///< Mode of pipeline Secure/Non-secure
};

/// @brief Pipeline create output data
struct PipelineCreateOutputData
{
    Pipeline*                pPipeline;                 ///< Created pipeline
    UINT32                   numInputs;                 ///< Number of inputs
    /// @todo (CAMX-1512) Instead of Chi structures use wrappers
    ChiPipelineInputOptions* pPipelineInputOptions;     ///< Input buffer requirements for the create pipeline
};

/// @brief Information about a single frame in a batch
struct PerBatchedFrameInfo
{
    UINT32          sequenceId;                             ///< Framework frame number
    UINT            activeStreamIdMask;                     ///< Active Stream Id Mask
    ChiBufferInfo   bufferInfo[MaxNumStreams];              ///< Buffer info corresponding to activeStreamIds[index];
                                                            ///  Buffers for the active streams indexed with streamId
};

/// @brief Pipeline capture request data.
struct PipelineProcessRequestData
{
    CaptureRequest*      pCaptureRequest;                        ///< Current capture request
    PerBatchedFrameInfo* pPerBatchedFrameInfo;                   ///< Information about all batched frames
};

/// @brief Information pertaining to per request active streams
struct PerRequestInfo
{
    UINT                        aMetadataReady;                    ///< 1, one of SOF or numNodesMetadataDone == numNodes done
                                                                   ///  0, both remain
    BOOL                        isSofDispatched;                   ///< True if SOF dispatched for a request
    UINT                        numNodesRequestIdDone;             ///< Number of nodes that are done processing
    UINT                        numNodesMetadataDone;              ///< Number of nodes that are done generating metadata
    UINT                        numNodesPartialMetadataDone;       ///< Number of nodes that are done generating partial
                                                                   ///  metadata
    UINT                        numNodesConfigDone;                ///< Number of nodes that are done config
    UINT32*                     pSequenceId;                       ///< Sequence ID mapped to each batch of the request
    UINT64                      batchFrameIntervalNanoSeconds;     ///< batch frame interval for HFR usecase
    CaptureRequest              request;                           ///< Copy of request from session
    LightweightDoublyLinkedList fences;                            ///< Fences registered for request
    std::atomic<UINT8>          bufferDone;                        ///< Atomic count to indicate whether the buffer is done
    BOOL                        isSlowdownPresent;                 ///< Did we try to send metadata but shutter was not ready
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure to hold node-link property information required for signaling
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NodeLinkPropertyInfo
{
    UINT32 propertyId; ///< Property Identifier
    Node*  pNode;      ///< Pointer to the node that publishes the property
};

/// @brief Map from PSMetadata to Property/Node instance
typedef std::unordered_map<UINT32, std::vector<NodeLinkPropertyInfo>> PSMetaMap;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure to hold unique port-link-instance tuple
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct UniqueNodePortInfo
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UniqueNodePortInfo
    ///
    /// @brief  constructor
    ///
    /// @param  pNodeArg  Pointer to the node
    /// @param  portIdArg Port Identifier
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UniqueNodePortInfo(
        Node* pNodeArg,
        UINT  portIdArg)
    {
        pNode  = pNodeArg;
        portId = portIdArg;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UniqueNodePortInfo
    ///
    /// @brief  constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UniqueNodePortInfo()
    {
        pNode  = NULL;
        portId = InvalidIndex;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// operator==
    ///
    /// @brief  Assignment operator for unique port information.
    ///
    /// @param  rPortInfo    Reference to the unique port info
    ///
    /// @return true/false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOWHINE GR017 : Operator overloaded to be used in STLs
    bool operator==(
        const UniqueNodePortInfo& rPortInfo) const
    {
        return (pNode == rPortInfo.pNode) &&
            (portId == rPortInfo.portId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// operator<
    ///
    /// @brief  Comparator operator for unique port information
    ///
    /// @param  rPortInfo    Reference to the unique port info
    ///
    /// @return true/false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOWHINE GR017: Operator overloaded to be used in STLs
    bool operator<(
        const UniqueNodePortInfo& rPortInfo) const
    {
        return (pNode < rPortInfo.pNode) ||
            ((pNode == rPortInfo.pNode) && (portId < rPortInfo.portId));
    }

    Node*  pNode;           ///< Node instance
    UINT   portId;          ///< Port Identifier
};

/// @brief PortId to UniqueNodePortInfo map
typedef std::map<UINT, std::vector<UniqueNodePortInfo>> UniqueNodePortInfoMap;

// @brief Flush information structure
struct FlushInfo
{
    CSLFlushType flushType;                      ///< Flush Type
    BOOL         hasFlushOccurred;               ///< Boolean flag to indicate whether flush
                                                 ///  has ever occurred
    UINT64       lastFlushRequestId;             ///< Request id of the last flushed request
    UINT64       lastValidRequestIdbeforeFlush;  ///< Last completed requestId with valid result
                                                 ///  before flush
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the hwl pipeline.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Pipeline
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static function to create the concrete pipeline object.
    ///
    /// @param  pCreateInputData    pipeline create input data
    /// @param  pCreateOutputData   pipeline create output data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        PipelineCreateInputData*  pCreateInputData,
        PipelineCreateOutputData* pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy the pipeline object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReusePipelineFromDescriptor
    ///
    /// @brief  Indicate that this pipeline is the same one that was created when the descriptor was created from the use case
    ///         and will be reused by the session as well.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID ReusePipelineFromDescriptor()
    {
        INT refCount = CamxAtomicInc(&m_referenceCount);
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "%s_%u: Updated m_referenceCount=%u", GetPipelineName(), GetPipelineId(), refCount);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetIPERTPipeline
    ///
    /// @brief  SetIPERTPipeline
    ///
    /// @param  isRealTime Boolean stating if pipeline is realtime
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetIPERTPipeline(
        BOOL isRealTime);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIPERTPipeline
    ///
    /// @brief  GetIPERTPipeline
    ///
    /// @return True or False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL GetIPERTPipeline();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckIPERTPipeline
    ///
    /// @brief  CheckIPERTPipeline
    ///
    /// @return True or False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckIPERTPipeline();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLDevices
    ///
    /// @brief  Get CSL device handles
    ///
    /// @return CSLDeviceHandle*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CSLDeviceHandle* GetCSLDevices()
    {
        return m_hDevices;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLSyncLink
    ///
    /// @brief  Get CSL link handles
    ///
    /// @return CSLLinkHandle*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CSLLinkHandle GetCSLSyncLink()
    {
        return m_hCSLSyncLinkHandle;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLLink
    ///
    /// @brief  Get CSL link handles
    ///
    /// @return CSLLinkHandle*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CSLLinkHandle* GetCSLLink()
    {
        return &m_hCSLLinkHandle;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareStreamOn
    ///
    /// @brief  Method that is called by HAL device before streamOn is sent to HW.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PrepareStreamOn();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StreamOn
    ///
    /// @brief  Method to stream on
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StreamOn();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StreamOff
    ///
    /// @brief  Method to stream off
    ///
    /// @param  modeBitmask Stream off mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StreamOff(
        CHIDEACTIVATEPIPELINEMODE modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Link
    ///
    /// @brief  Link pipeline
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Link();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Unlink
    ///
    /// @brief  Unlink pipeline
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Unlink();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseResources
    ///
    /// @brief  Method to release resources for all nodes in pipeline
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetParams
    ///
    /// @brief  Set pipeline specific parameters.
    ///
    /// @param  pipelineIndex  pipeline index
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetParams(
       UINT pipelineIndex)
    {
        m_pipelineIndex = pipelineIndex;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDualCameraSyncEnabled
    ///
    /// @brief  Method to get the sync enabled info which is required for syncing both the real time requests
    ///
    /// @param  requestId           Capture request id
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL GetDualCameraSyncEnabled(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineIdentifierString
    ///
    /// @brief  Return the string containing pipeline name, pipeline id
    ///
    /// @return name and id of the pipeline
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CHAR* GetPipelineIdentifierString() const
    {
        return m_pipelineIdentifierString;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OpenRequest
    ///
    /// @brief  Method to open CSL process request.
    ///
    /// @param  requestId            Capture request id
    /// @param  CSLSyncID            CSL sync id for the request
    /// @param  isSyncMode           Flag to indicate if it is sync mode
    /// @param  expectedExposureTime How long is the exposure time for this request (in Milliseconds)
    ///
    /// @return CamxResultSuccess   if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult OpenRequest(
        UINT64 requestId,
        UINT64 CSLSyncID,
        BOOL   isSyncMode,
        UINT32 expectedExposureTime);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequest
    ///
    /// @brief  Method to trigger process request.
    ///
    /// @param  pPipelineRequestData Pipeline process request data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessRequest(
        PipelineProcessRequestData* pPipelineRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyNodeRequestIdDone
    ///
    /// @brief  Nodes call this function when they are done generating all their outputs for a requestId
    ///
    /// @param  requestId RequestId for which the outputs are generated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyNodeRequestIdDone(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyNodeMetadataDone
    ///
    /// @brief  Nodes call this function when they are done generating all their metadata for a requestId
    ///
    /// @param  requestId RequestId for which the metadata are generated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID NotifyNodeMetadataDone(
        UINT64 requestId)
    {
        UINT            perRequestIdIndex = requestId % MaxPerRequestInfo;
        PerRequestInfo* pPerRequestInfo   = &m_perRequestInfo[perRequestIdIndex];

        if (m_nodeCount == CamxAtomicIncU(&(pPerRequestInfo->numNodesMetadataDone)))
        {
            CAMX_LOG_CONFIG(CamxLogGroupCore, "[%s] All nodes complete metadata with requestId: %llu.",
                GetPipelineIdentifierString(), requestId);
            BOOL sofOutstanding =
                CamxAtomicCompareExchangeU(&m_perRequestInfo[requestId % MaxPerRequestInfo].aMetadataReady, 0, 1);

            if (TRUE == RequestInErrorState(requestId))
            {
                ProcessMetadataRequestIdError(requestId);
            }
            else
            {
                if (FALSE == sofOutstanding)
                {
                    ProcessMetadataRequestIdDone(requestId, FALSE);
                }
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPartialMetadataEnabled
    ///
    /// @brief  Check if Partial Metadata is enabled or not
    ///
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPartialMetadataEnabled()
    {
        return m_bPartialMetadataEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyNodePartialMetadataDone
    ///
    /// @brief  Nodes call this function when they are done generating all their partial metadata for a requestId
    ///
    /// @param  requestId RequestId for which the partial metadata are generated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID NotifyNodePartialMetadataDone(
        UINT64 requestId)
    {
        UINT            perRequestIdIndex = requestId % MaxPerRequestInfo;
        PerRequestInfo* pPerRequestInfo = &m_perRequestInfo[perRequestIdIndex];

        if (m_nodeCount == CamxAtomicIncU(&(pPerRequestInfo->numNodesPartialMetadataDone)))
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "[%s] All nodes complete partial metadata with requestId: %llu.",
                GetPipelineIdentifierString(), requestId);
            if (FALSE == RequestInErrorState(requestId))
            {
                ProcessPartialMetadataRequestIdDone(requestId);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupCore, "Partial metadata not sent because request %d is in error state", requestId);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessPartialMetadataRequestIdDone
    ///
    /// @brief  Process when partial result metadata is ready to send to framework for a perticular request id
    ///
    /// @param  requestId     The request id the partial metadata applies to
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessPartialMetadataRequestIdDone(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMetadataRequestIdDone
    ///
    /// @brief  Process when result metadata is ready to send to framework for a perticular request id
    ///
    /// @param  requestId     The request id the metadata applies to.
    /// @param  earlyMetadata TRUE if metadata is the early metadata
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessMetadataRequestIdDone(
        UINT64 requestId,
        BOOL   earlyMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMetadataBufferDone
    ///
    /// @brief  Processed when metabuffers are done by the framework
    ///
    /// @param  requestId     The request id the metadata applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessMetadataBufferDone(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMetadataRequestIdError
    ///
    /// @brief  Process when result metadata is ready but the request had an error
    ///
    /// @param  requestId     The request id the metadata applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessMetadataRequestIdError(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyNodeConfigDone
    ///
    /// @brief  Nodes call this function when they have submitted HW config request
    ///
    /// @param  requestId RequestId for which HW config request is submitted
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID NotifyNodeConfigDone(
        UINT64 requestId)
    {
        UINT            perRequestIdIndex   = requestId % MaxPerRequestInfo;
        PerRequestInfo* pPerRequestInfo     = &m_perRequestInfo[perRequestIdIndex];

        if (m_numConfigDoneNodes == CamxAtomicIncU(&(pPerRequestInfo->numNodesConfigDone)))
        {
            // signal the the thread that is waiting for config done
            m_pConfigDoneLock->Lock();

            m_configDoneCount++;
            CAMX_LOG_INFO(CamxLogGroupCore,
                "[%s] All nodes complete stream config with requestId: %llu., config done count %d",
                GetPipelineIdentifierString(), requestId, m_configDoneCount);

            m_pWaitForConfigDone->Signal();
            m_pConfigDoneLock->Unlock();
        }
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AbortConfigDone
    ///
    /// @brief  signal config done if flush is in progress
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID AbortConfigDone()
    {
        m_pConfigDoneLock->Lock();
        if (TRUE == m_isWaitForConfigDone)
        {
            m_isConfigDoneAborted = TRUE;
            m_pWaitForConfigDone->Signal();
        }
        m_pConfigDoneLock->Unlock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SinkPortFenceSignaled
    ///
    /// @brief  Notification of a fence being triggered/signaled for a sink port
    ///
    /// @param  sinkPortId          Sink port Id
    /// @param  sequenceId          sequenceId of the HALBuffer whose output is available
    /// @param  requestID           RequestId of output available
    /// @param  pChiBufferInfo      Chi Buffer information whose output is available
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SinkPortFenceSignaled(
        UINT           sinkPortId,
        UINT32         sequenceId,
        UINT64         requestID,
        ChiBufferInfo* pChiBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SinkPortFenceErrorSignaled
    ///
    /// @brief  Notification of a fence being triggered/signaled for a sink port
    ///
    /// @param  sinkPortId          Sink port Id
    /// @param  sequenceId          sequenceId of the HALBuffer whose output is available
    /// @param  requestID           RequestId of output available
    /// @param  pChiBufferInfo      Chi Buffer information whose output is available
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID  SinkPortFenceErrorSignaled(
        UINT            sinkPortId,
        UINT32          sequenceId,
        UINT64          requestID,
        ChiBufferInfo*  pChiBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NonSinkPortFenceErrorSignaled
    ///
    /// @brief  Notification of a fence being triggered/signaled for a non sink port in error
    ///
    /// @param  phFence   CSL Fence that got signaled
    /// @param  requestId Request Id for which the fence was signaled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NonSinkPortFenceErrorSignaled(
        CSLFence* phFence,
        UINT64    requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NonSinkPortFenceSignaled
    ///
    /// @brief  Notification of a fence being triggered/signaled for a non sink port
    ///
    /// @param  phFence   CSL Fence that got signaled
    /// @param  requestId Request Id for which the fence was signaled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NonSinkPortFenceSignaled(
        CSLFence* phFence,
        UINT64    requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddFinalizeBufferPropertiesNodeList
    ///
    /// @brief  Nodes call this function during buffer negotiation to add itself to the list of nodes whose buffer properties
    ///         need to be finalized. This function should be called by the node during the negotiation process STRICTLY only
    ///         when it is ready to generate its input buffer requirement and NEVER before that
    ///
    /// @param  pNode Node to add to the list
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL AddFinalizeBufferPropertiesNodeList(
        Node* pNode)
    {
        // Buffer negotiation is a process in which we walk-back from the sink nodes to the source node and along the way
        // each node informs its parent output port about the buffer dimension/properties it requires the parent to output.
        // This process will terminate at the source node (with no input ports). The source node then determines the final
        // output buffer dimension/properties it is going to generate based on the input buffer requirement it received from
        // its child nodes. For e.g. Sensor will determine the final sensor resolution/mode based on the input buffer
        // requirements from the IFE.
        //
        // Once the source node determines the output buffer dimension/properties we need to walk-forward from the source
        // node to the sink node(s) and along the way each node decides what its final output port buffer dimension/properties
        // will be based on all its input (input meaning what the parent decided for the buffer dimension/properties on its
        // output ports)
        //
        // Walk-forward MUST happen from the source-to-the-sink-nodes and m_ppOrderedNodes is the ordered
        // list of the nodes from the source-to-the-sink-nodes. The ordering is established by the fact that each Node calls
        // this function STRICTLY only after it is ready to generate its input buffer requirement on the walk-back part of the
        // buffer negotiation process

        BOOL    result = TRUE;

        for (UINT32 i = 0 ; i < m_orderedNodeCount; i++)
        {
            if (pNode == m_ppOrderedNodes[i])
            {
                result = FALSE;
                break;
            }
        }
        if (TRUE == result)
        {
            if (m_orderedNodeCount < m_nodeCount)
            {
                m_ppOrderedNodes[m_orderedNodeCount] = pNode;
                m_orderedNodeCount++;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "m_orderedNodeCount > m_nodeCount, failure emminent");
            }
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckOfflinePipelineInputBufferRequirements
    ///
    /// @brief  This function is invoked for offline pipeline to check the source port buffer requirements
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckOfflinePipelineInputBufferRequirements();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FinalizePipeline
    ///
    /// @brief  Method to finalize the created pipeline
    ///
    /// @param  pFinalizeInitializationData Data required to finalize the initialization
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FinalizePipeline(
        FinalizeInitializationData* pFinalizeInitializationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HasStatsNode
    ///
    /// @brief  Method to query if the pipeline has a stats node in it (so that dependencies per properties can be enforced)
    ///
    /// @return TRUE if Stats Node exists in the pipeline, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL HasStatsNode() const
    {
        return m_flags.hasStatsNode;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HasIFENode
    ///
    /// @brief  Method to query if the pipeline has IFE node in it
    ///
    /// @return TRUE if IFE Node exists in the pipeline, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL HasIFENode() const
    {
        return m_flags.hasIFENode;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HasJPEGNode
    ///
    /// @brief  Method to query if the pipeline has a JPEG (hw or Aggr) node in it
    ///
    /// @return TRUE if JPEG Node exists in the pipeline, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL HasJPEGNode() const
    {
        return m_flags.hasJPEGNode;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMasterSensorInput
    ///
    /// @brief  Method to query if the pipeline input is the master sensor input.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsMasterSensorInput() const
    {
        return IsSensorInput() && (m_sensorSyncMode == MasterMode);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorSyncMode
    ///
    /// @brief  Method to query pipeline sensor hardware sync mode
    ///
    /// @return SensorSyncMode
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE SensorSyncMode GetSensorSyncMode() const
    {
        return m_sensorSyncMode;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSensorSyncMode
    ///
    /// @brief  Method to Set pipeline sensor hardware sync mode
    ///
    /// @param  sensorSyncMode Sensor hardware sync mode for this pipeline
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetSensorSyncMode(
        SensorSyncMode sensorSyncMode)
    {
        if (m_sensorSyncMode != sensorSyncMode)
        {
            CAMX_LOG_CONFIG(CamxLogGroupCore, "%s SensorSyncMode for CameraID:%d is %d",
                GetPipelineIdentifierString(),
                m_cameraId,
                sensorSyncMode);
            m_sensorSyncMode = sensorSyncMode;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsRealTime
    ///
    /// @brief  Method to query if the pipeline is real time
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsRealTime() const
    {
        return m_pPipelineDescriptor->flags.isRealTime;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSensorInput
    ///
    /// @brief  Method to query if the pipelines input is sensor
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsSensorInput() const
    {
        return m_pPipelineDescriptor->flags.isSensorInput;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSecureMode
    ///
    /// @brief  Method to query if the pipeline is Secure or Non-secure
    ///
    /// @return TRUE if Secure, FALSE if Non-secure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSecureMode() const
    {
        return m_flags.isSecureMode;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCameraRunningOnBPS
    ///
    /// @brief  Return indication whether camera is running on BPS instead of IFE
    ///
    /// @return TRUE for BPS camera, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsCameraRunningOnBPS() const
    {
        return m_flags.isCameraRunningOnBPS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CSLMessageHandler
    ///
    /// @brief  This method is a CSL message handler which receives all the events from CSL
    ///
    /// @param  pUserData Pointer to userdata(class object where message handler is registered) sent to the CSL
    /// @param  pMessage  Pointer to CSL frame message
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CSLMessageHandler(
        VOID*       pUserData,
        CSLMessage* pMessage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendErrorNotification
    ///
    /// @brief  Send error notification based on CSL error message
    ///
    /// @param  pMessage CSL error message
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SendErrorNotification(
        const CSLErrorMessage* pMessage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTuningDataManager
    ///
    /// @brief  Get the Tuning Manager for this HwContext instance
    ///
    /// @return Pointer to Tuning Manager
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE TuningDataManager* GetTuningDataManager() const
    {
        return m_pTuningManager;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequestInErrorState
    ///
    /// @brief  Gets current pipeline create status
    ///
    /// @param  requestId Id of the request to get data of
    ///
    /// @return TRUE if error reported
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL RequestInErrorState(
        UINT64 requestId)
    {
        return m_pDeferredRequestQueue->RequestInErrorState(requestId, GetPipelineId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraId
    ///
    /// @brief  Get the camera id associated with this pipeline
    ///
    /// @return Camera id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE INT32 GetCameraId() const
    {
        return m_cameraId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputStreamWrapper
    ///
    /// @brief  Get the pipeline descriptor
    ///
    /// @param  nodeId      Node Id
    /// @param  instanceId  Instance Id
    /// @param  portId      Port Id
    ///
    /// @return Wrapper*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ChiStreamWrapper* GetOutputStreamWrapper(
        UINT32 nodeId,
        UINT32 instanceId,
        UINT32 portId
        ) const
    {
        ChiStreamWrapper* pChiStreamWrapper = NULL;

        for (UINT i = 0; i < m_pPipelineDescriptor->numOutputs; i++)
        {
            const PipelineOutputData* pPipelineOutputData = &m_pPipelineDescriptor->outputData[i];

            if ((pPipelineOutputData->nodePort.nodeId         == nodeId)     &&
                (pPipelineOutputData->nodePort.nodeInstanceId == instanceId) &&
                (pPipelineOutputData->nodePort.nodePortId     == portId))
            {
                pChiStreamWrapper = pPipelineOutputData->pOutputStreamWrapper;
                break;
            }
        }

        return pChiStreamWrapper;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputStreamWrapper
    ///
    /// @brief  Get the pipeline descriptor
    ///
    /// @param  nodeId      Node Id
    /// @param  instanceId  Instance Id
    /// @param  portId      Port Id
    ///
    /// @return Camera id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ChiStreamWrapper* GetInputStreamWrapper(
        UINT32 nodeId,
        UINT32 instanceId,
        UINT32 portId
        ) const
    {
        ChiStreamWrapper* pChiStreamWrapper = NULL;

        for (UINT i = 0; i < m_pPipelineDescriptor->numInputs; i++)
        {
            const PipelineInputData* pPipelineInputData = &m_pPipelineDescriptor->inputData[i];

            if ((pPipelineInputData->nodePort.nodeId         == nodeId)     &&
                (pPipelineInputData->nodePort.nodeInstanceId == instanceId) &&
                (pPipelineInputData->nodePort.nodePortId     == portId))
            {
                pChiStreamWrapper = pPipelineInputData->pInputStreamWrapper;
                break;
            }
        }

        return pChiStreamWrapper;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIntraPipelinePerFramePool
    ///
    /// @brief  Get Frame Property/ metadata pool by pipeline ID
    ///
    /// @param  poolType    Type of the per-frame pool asked for
    /// @param  pipelineId  Pipeline ID of the property/ metadata pool want to get
    ///
    /// @return Pointer to the per frame property/metadata pool
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MetadataPool* GetIntraPipelinePerFramePool(
        PoolType poolType,
        UINT     pipelineId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPeerRealtimePipelineId
    ///
    /// @brief  Get peer realtime pipeline id
    ///
    /// @param  pMultiReqSync    Pointer to MultiRequestSyncData structure which used to idenfity multi request information
    /// @param  ownPipelineId    The pipeline id of pipeline which need get its peer pipeline
    /// @param  pPeerPipelineId  Output parameters that gives found peer pipeline ID
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPeerRealtimePipelineId(
        MultiRequestSyncData* pMultiReqSync,
        UINT                  ownPipelineId,
        UINT*                 pPeerPipelineId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMultiCamera
    ///
    /// @brief  Is Multi Camera use case or not.
    ///         This needs to be called once session::Initialize has trigerred otherwise it always return false.
    ///         Need to remove this logic after CHI create pipeline has capability to send extra vendor specifc information.
    ///
    /// @return True if corresponding session has two real time pipelines, False if not
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsMultiCamera()
    {
        BOOL isMultiCamera = FALSE;

        if (NULL != m_pSession)
        {
            isMultiCamera = m_pSession->IsMultiCamera();
        }

        return isMultiCamera;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineId
    ///
    /// @brief  Get current pipeline id
    ///
    /// @return pipeline id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetPipelineId()
    {
        return m_pipelineIndex;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineDescriptor
    ///
    /// @brief  Get current pipeline descriptor
    ///
    /// @return pipeline id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const PipelineDescriptor* GetPipelineDescriptor()
    {
        return m_pPipelineDescriptor;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineName
    ///
    /// @brief  Get current pipeline name
    ///
    /// @return pipeline id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CHAR* GetPipelineName() const
    {
        return m_pPipelineDescriptor->pipelineName;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPipelineTriggeringRecovery
    ///
    /// @brief  Sets the pipeline recovery status
    ///
    /// @param  isRecovery Recovery status
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetPipelineTriggeringRecovery(
        BOOL isRecovery)
    {
        m_isTriggeringRecovery = isRecovery;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineTriggeringRecovery
    ///
    /// @brief  Get the pipeline recovery status
    ///
    /// @return TRUE if the session is in recovery, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL GetPipelineTriggeringRecovery()
    {
        return m_isTriggeringRecovery;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsStreamedOn
    ///
    /// @brief  Is streamed on
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsStreamedOn()
    {
        if (PipelineStatus::STREAM_ON == GetPipelineStatus())
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeMetadataPools
    ///
    /// @brief  Call to initialize metadata pools of a pipeline
    ///
    /// @param  numSlots Number of slots each pool should allocate
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeMetadataPools(
        UINT numSlots);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPerFramePool
    ///
    /// @brief  Gets the pointer to the per frame property/metadata pool
    ///
    /// @param  poolType Type of the per-frame pool asked for
    ///
    /// @return Pointer to the per frame property/metadata pool
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE MetadataPool* GetPerFramePool(
        PoolType poolType)
    {
        MetadataPool* pPool = NULL;

        switch (poolType)
        {
            case PoolType::PerFrameInput:
                pPool = m_pInputPool;
                break;
            case PoolType::PerFrameResult:
                pPool = m_pMainPool;
                break;
            case PoolType::PerFrameResultEarly:
                pPool = m_pEarlyMainPool;
                break;
            case PoolType::PerFrameInternal:
                pPool = m_pInternalPool;
                break;
            case PoolType::PerUsecase:
                pPool = m_pUsecasePool;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid pool type %d", poolType);
                break;
        }

        return pPool;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBatchedHALOutputNum
    ///
    /// @brief  Gets number HAL output buffer of batched mode from this pipeline from usecase pool
    ///
    /// @return Number of batched frames
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetBatchedHALOutputNum()
    {
        UINT frames = 0;
        if (TRUE == m_HALOutputBufferCombined)
        {
            frames = 1;
        }
        else
        {
            frames = m_numMaxBatchedFrames;
        }
        return frames;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHALOutputBufferCombined
    ///
    /// @brief  Gets the HAL output buffer type for batched mode
    ///
    /// @return TRUE if the HAL output buffer for batched mode is combined
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL GetHALOutputBufferCombined()
    {
        return m_HALOutputBufferCombined;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFPSValue
    ///
    /// @brief  Gets fps value of this pipeline from usecase pool
    ///
    /// @return Fps value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT GetFPSValue();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequest
    ///
    /// @brief  Gets the pointer to the request structure for the provided requestId
    ///
    /// @param  requestId Id of the request to get data of
    ///
    /// @return Pointer to the CaptureRequest
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CaptureRequest* GetRequest(
        UINT64 requestId)
    {
        return &m_perRequestInfo[requestId % MaxPerRequestInfo].request;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterRequestFence
    ///
    /// @brief  Record a fence for a request
    ///
    /// @param  phFence   Fence for the request
    /// @param  requestId Id of the request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID RegisterRequestFence(
        CSLFence* phFence,
        UINT64    requestId)
    {
        m_pPerRequestInfoLock->Lock();
        LightweightDoublyLinkedListNode* pNode =
            reinterpret_cast<LightweightDoublyLinkedListNode*>(CAMX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

        if (NULL != pNode)
        {
            pNode->pData = phFence;
            m_perRequestInfo[requestId % MaxPerRequestInfo].fences.InsertToTail(pNode);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory to allocate node");
        }
        m_pPerRequestInfoLock->Unlock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemoveRequestFence
    ///
    /// @brief  Record a fence for a request
    ///
    /// @param  phFence   Fence for the request
    /// @param  requestId Id of the request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID RemoveRequestFence(
        CSLFence* phFence,
        UINT64    requestId)
    {
        m_pPerRequestInfoLock->Lock();
        LightweightDoublyLinkedListNode* pNode = m_perRequestInfo[requestId % MaxPerRequestInfo].fences.Head();
        LightweightDoublyLinkedListNode* pNext = NULL;

        while (NULL != pNode)
        {
            pNext = LightweightDoublyLinkedList::NextNode(pNode);

            if (phFence == pNode->pData)
            {
                m_perRequestInfo[requestId % MaxPerRequestInfo].fences.RemoveNode(pNode);
                CAMX_FREE(pNode);
            }
            pNode = pNext;
        }
        m_pPerRequestInfoLock->Unlock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineStatus
    ///
    /// @brief  Get current pipeline status
    ///
    /// @return current pipeline status
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE PipelineStatus GetPipelineStatus()
    {
        return m_currentPipelineStatus;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpState
    ///
    /// @brief  Dumps snapshot of current state to a file
    ///
    /// @param  fd      file descriptor
    /// @param  indent  indent spaces.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpState(
        INT     fd,
        UINT32  indent);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpDebugInfo
    ///
    /// @brief  Dumps pipeline information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpDebugInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNodeExist
    ///
    /// @brief  Check if corresponding node exist in the pipeline
    ///
    /// @param  nodeType   node type
    ///
    /// @return TRUE if find the node, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsNodeExist(
        UINT nodeType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNodeExistByName
    ///
    /// @brief  Check if corresponding node exist in the pipeline based on NodeName
    ///
    /// @param  pNodeName   node name
    ///
    /// @return TRUE if find the node, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsNodeExistByName(
        CHAR* pNodeName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNodeExistByNodePropertyValue
    ///
    /// @brief  Check if corresponding node exist in the pipeline based on NodePropertyValue
    ///
    /// @param  pNodePropertyValue   node PropertyValue
    ///
    /// @return TRUE if find the node, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsNodeExistByNodePropertyValue(
        CHAR* pNodePropertyValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HasSnapshotJPEGStream
    ///
    /// @brief  Check if the pipeline has snapshot JPEG stream
    ///
    /// @return TRUE if the pipeline has snapshot JPEG stream, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL HasSnapshotJPEGStream();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DetermineExtrabuffersNeeded
    ///
    /// @brief  Determine the DetermineExtrabuffersNeeded of the pipeline
    ///
    /// @return Return the value of the pipeline's ExtrabuffersNeeded
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 DetermineExtrabuffersNeeded();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DetermineFrameDelay
    ///
    /// @brief  Determine the frame delay of the pipeline
    ///
    /// @return Return the value of the pipeline's frame delay
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 DetermineFrameDelay();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsEISEnabled
    ///
    /// @brief  Determine if EIS is Enabled
    ///
    /// @return Return TRUE if EIS is enabled else FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsEISEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DetermineEISMiniamalTotalMargin
    ///
    /// @brief  Determine EIS MiniamalT otal Margin
    ///
    /// @param  pMargin Holds the margin output
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DetermineEISMiniamalTotalMargin(
        MarginRequest* pMargin);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestQueueDepth
    ///
    /// @brief  Determine the request queue depth of the pipeline. Function must be called after the Pipeline creates and
    ///         initializes it's nodes
    ///
    /// @return Return the value of the pipeline's request queue depth
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetRequestQueueDepth();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetaBufferDelay
    ///
    /// @brief  Determine the metabuffer notification delay from the pipeline
    ///
    /// @return Return the value of the pipeline's metabuffer delay
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetMetaBufferDelay()
    {
        return m_metaBufferDelay;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataInfo
    ///
    /// @brief  Method to query metadata information
    ///
    /// @param  maxPublishTagArraySize  publish tag array size
    /// @param  pPublishTagArray        Array of tags published by the pipeline
    /// @param  pPublishTagCount        Pointer to the Count of the tags published by the pipeline
    /// @param  pPartialPublishTagCount Pointer to the Count of the partial tags published by the pipeline
    /// @param  pMaxNumMetaBuffers      Pointer to the maximum number of metadata buffers required by the pipeline
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult QueryMetadataInfo(
        const UINT32    maxPublishTagArraySize,
        UINT32*         pPublishTagArray,
        UINT32*         pPublishTagCount,
        UINT32*         pPartialPublishTagCount,
        UINT32*         pMaxNumMetaBuffers);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxPipelineDelay
    ///
    /// @brief  Method to query Maximum Pipeline delay
    ///
    /// @return Pipelinedelay
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetMaxPipelineDelay()
    {
        return m_maxPipelineDelay;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsTagPresentInPublishList
    ///
    /// @brief  Checks if the tag defined by tagId is present in the Per-Frame publishlist
    ///
    /// @param  tagId  Identifier of the tag to be checked
    ///
    /// @return TRUE if the tag is present, FALSE otherwise
    ///
    /// @note   This API should be only used to validate against tags published in the mainpool
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsTagPresentInPublishList(
       UINT32 tagId) const
    {
        return (m_nodePublishSet.find(tagId) != m_nodePublishSet.end());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TriggerGenericRecovery
    ///
    /// @brief  Trigger session recovery
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID TriggerGenericRecovery()
    {
        PerRequestInfo* pPerRequestInfo      = &m_perRequestInfo[0];
        ResultsData errorData                = {};
        errorData.type                       = CbType::Error;
        errorData.pipelineIndex              = m_pipelineIndex;
        errorData.cbPayload.error.code       = MessageCodeRecovery;
        errorData.cbPayload.error.sequenceId = 0;
        errorData.cbPayload.error.streamId   = 0;
        errorData.pPrivData                  = pPerRequestInfo->request.pPrivData;
        m_pSession->NotifyResult(&errorData);
    }



    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TriggerRequestError
    ///
    /// @brief  Trigger request error
    ///
    /// @param  requestId request to trigger reqId
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID TriggerRequestError(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLSession
    ///
    /// @brief  Return CSL Session Handle
    ///
    /// @return CSL Session handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CSLHandle GetCSLSession() const
    {
        return m_hCSLSession;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFullRecoveryFlagSet
    ///
    /// @brief  Full recovery set or not
    ///
    /// @return True if fullrecovery
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsFullRecoveryFlagSet() const
    {
        return m_isFullRecovery;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushInfo
    ///
    /// @brief  Get information about the last flush
    ///
    /// @param  rFlushInfo reference to flush info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID GetFlushInfo(
        FlushInfo& rFlushInfo
        ) const
    {
        rFlushInfo = m_flushInfo;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyDRQofRequestError
    ///
    /// @brief  Marks the request as error and updates the dependencies in the DRQ accordingly.
    ///
    /// @param  requestId The request id to be marked as error.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID NotifyDRQofRequestError(
        UINT64 requestId)
    {
        m_pDeferredRequestQueue->HandleFenceError(GetPipelineId(), requestId, GetFlushStatus());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LogPendingNodes
    ///
    /// @brief  Prints the pending nodes to the info log.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID LogPendingNodes();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushPendingNodes
    ///
    /// @brief  Flush all the pending nodes.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FlushPendingNodes();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushMetadata
    ///
    /// @brief  Flush the metadata on pipeline
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FlushMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushResponseTimeInMs
    ///
    /// @brief  Return worst-case response time of the pipeline for flush call
    ///
    /// @return worst-case response time in microseconds.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 GetFlushResponseTimeInMs();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdatePipelineFlushState
    ///
    /// @brief  Atomic Update of the Pipeline Flush State
    ///
    /// @param  state  Boolean value indicating the state
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdatePipelineFlushState(
        BOOL state)
    {
        CamxAtomicStoreU8(&m_aPipelineFlushState, static_cast<UINT8>(state));
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineFlushState
    ///
    /// @brief  Atomic Update of the Pipeline Flush State
    ///
    /// @return Boolean value regarding the pipeline Flush State
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL GetPipelineFlushState()
    {
        return static_cast<BOOL>(TRUE == CamxAtomicLoadU8(&m_aPipelineFlushState));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushStatus
    ///
    /// @brief  Return Flush status
    ///
    /// @return TRUE if the session is in Flush state otherwise False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL GetFlushStatus()
    {
        return GetPipelineFlushState();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SaveLastValidRequestId
    ///
    /// @brief  Save last valid request Id before flush call
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SaveLastValidRequestId()
    {
        UINT64 requestId = m_lastCompletedRequestId;

        while ((0 < requestId) && (RequestQueueDepth >= (m_lastCompletedRequestId - requestId)))
        {
            if (FALSE == m_pDeferredRequestQueue->RequestInErrorState(requestId, GetPipelineId()))
            {
                m_flushInfo.lastValidRequestIdbeforeFlush = requestId;
                CAMX_LOG_INFO(CamxLogGroupCore, "Pipeline:%s Saving lastValidReqIdBeforeFlush: %llu lastCompletedReqId: %llu",
                              GetPipelineName(),
                              m_flushInfo.lastValidRequestIdbeforeFlush,
                              m_lastCompletedRequestId);
                break;
            }
            else
            {
                requestId--;
            }
        }

        if ((0 == requestId) || ((RequestQueueDepth) < m_lastCompletedRequestId - requestId))
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                          "Pipeline %s Failed to find lastValidReqIdBeforeFlush %llu ReqQueueDepth %d lastCompletedReqId %llu",
                           GetPipelineIdentifierString(),
                           requestId,
                           RequestQueueDepth,
                           m_lastCompletedRequestId);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateLastFlushedRequestId
    ///
    /// @brief  Update last flushed requestId
    ///
    /// @param  requestId request id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID UpdateLastFlushedRequestId(
        UINT64 requestId)
    {
        if (requestId > m_flushInfo.lastFlushRequestId)
        {
            m_flushInfo.lastFlushRequestId = requestId;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SaveFlushInfo
    ///
    /// @brief  Save flush type used
    ///
    /// @param  cslFlushType flush type used
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SaveFlushInfo(
        CSLFlushType cslFlushType)
    {
        m_flushInfo.flushType          = cslFlushType;
        m_flushInfo.hasFlushOccurred   = TRUE;
        m_flushInfo.lastFlushRequestId = GetLastSubmittedRequestId();
        m_lastRequestId = GetLastSubmittedRequestId();

        CAMX_LOG_CONFIG(CamxLogGroupCore, "Pipeline:%s Saving lastFlushRequestId: %llu flushType: %d",
                      GetPipelineName(),
                      m_flushInfo.lastFlushRequestId,
                      m_flushInfo.flushType);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearPendingResources
    ///
    /// @brief  Clear out all the nodes resources
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ClearPendingResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AreAllNodesDone
    ///
    /// @brief  Checks if the nodes are done processing by making sure that the last submitted request to this pipeline is the
    ///         same as the last completed request.
    ///
    /// @return TRUE is all the nodes are done processing
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL AreAllNodesDone()
    {
        return (m_lastSubmittedRequestId == m_lastInOrderCompletedRequestId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLastSubmittedRequestId
    ///
    /// @brief  Get the request id for the last request that was able to call addDeferredNode succesfully.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT64 GetLastSubmittedRequestId()
    {
        return m_lastSubmittedRequestId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitForAllNodesRequest
    ///
    /// @brief  Wait for all node requests done for the pipeline.
    ///
    /// @return success if all node requests are done
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WaitForAllNodesRequest();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendRecoveryError
    ///
    /// @brief  Send error notification based on node error message
    ///
    /// @param  requestId requestId
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SendRecoveryError(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxLivePendingRequestsOfSession
    ///
    /// @brief  Return maxLivePendingRequests from session
    ///
    /// @return Max number of live pending requests
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetMaxLivePendingRequestsOfSession()
    {
        return (NULL == m_pSession) ? 0 : m_pSession->GetMaxLivePendingRequestsOfSession();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDelayedPipeline
    ///
    /// @brief  Method to query if the pipeline is delayed
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsDelayedPipeline() const
    {
        return m_isDelayedPipeline;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLinkPropertyId
    ///
    /// @brief  Get the request id for the last request that was able to call addDeferredNode succesfully.
    ///
    /// @param  metadataId      Metadata identifier
    /// @param  pNode           Pointer to the node
    /// @param  pLinkPropertyId Dynamic propertyId assigned to the per port metadata
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetLinkPropertyId(
        UINT32  metadataId,
        Node*   pNode,
        UINT32* pLinkPropertyId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QuerySupportedMetadataList
    ///
    /// @brief  Gets a list of per-port metadata supported by the Driver
    ///
    /// @param  rMetadataIdArray   Reference to a structure that defines the array of metadata Ids
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID QuerySupportedMetadataList(
        ChiMetadataIdArray& rMetadataIdArray)
    {
        rMetadataIdArray = m_nodePSMetadataArray;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPSMPublished
    ///
    /// @brief  Checks if the PLM will be published by any node in the pipeline
    ///
    /// @param  tagId      Metadata identifier
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPSMPublished(
        UINT32  tagId)
    {
        return (m_psMetaMap.find(tagId) != m_psMetaMap.end());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetLastSubmittedShutterRequestId
    ///
    /// @brief  Helper function to reset m_lastSubmittedShutterRequestId during flush where shutter notifications are
    ///         allowed to me non-monotonic
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID ResetLastSubmittedShutterRequestId()
    {
        m_lastSubmittedShutterRequestId = 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitUntilStreamOnDone
    ///
    /// @brief  Wait for Device stream on to complete
    ///
    /// @return return CamxResultSuccess if stream On is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WaitUntilStreamOnDone();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyStreamOnWait
    ///
    /// @brief  Wakeup streamon waiting thread
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyStreamOnWait();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IncrementLivePendingRequest
    ///
    /// @brief  Increment live pending requests of the pipeline
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID IncrementLivePendingRequest()
    {
        CamxAtomicIncU(&m_livePendingRequest);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DecrementLivePendingRequest
    ///
    /// @brief  Decrement live pending requests of the pipeline
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID DecrementLivePendingRequest()
    {
        CamxAtomicDecU(&m_livePendingRequest);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLivePendingRequest
    ///
    /// @brief  Get live pending requests of the pipeline
    ///
    /// @return number of pending requests
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT64 GetLivePendingRequest()
    {
        return CamxAtomicLoadU(&m_livePendingRequest);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFirstPendingRequestId
    ///
    /// @brief  Get the first request id which is pending.
    ///
    /// @return First request id which is submitted to hw nodes but result still pending
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT64 GetFirstPendingRequestId()
    {
        return m_firstPendingRequestId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFirstPendingCSLSyncId
    ///
    /// @brief  Get the CSL sync id for the first request which is pending.
    ///
    /// @return CSL sync id for the first request id which is submitted to hw nodes but result still pending
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT64 GetFirstPendingCSLSyncId()
    {
        return m_firstPendingCSLSyncId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateFirstPendingRequest
    ///
    /// @brief  Calculate first request which is in pending state
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculateFirstPendingRequest();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CanProcessCSLMessage
    ///
    /// @brief  Check whether CSL message can be processed or not
    ///
    /// @return Return Success/Fail
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CanProcessCSLMessage();


protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Pipeline
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Pipeline();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Pipeline
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~Pipeline();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Method to initialize Pipeline object.
    ///
    /// @param  pCreateInputData    Pipeline create input data
    /// @param  pCreateOutputData   Pipeline create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        PipelineCreateInputData*  pCreateInputData,
        PipelineCreateOutputData* pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateNodes
    ///
    /// @brief  Method to create Node object instances for the current pipeline usecase selection.
    ///
    /// @param  pCreateInputData    Pipeline create input data
    /// @param  pCreateOutputData   Pipeline create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateNodes(
        PipelineCreateInputData*  pCreateInputData,
        PipelineCreateOutputData* pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroyNodes
    ///
    /// @brief  Method to destroy Node object instances previously created for matching pipeline usecase selection.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DestroyNodes();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNode
    ///
    /// @brief  Get node pointer for a given type and instance id
    ///
    /// @param  nodeType   node type
    /// @param  instanceId instance id of the node
    ///
    /// @return Corresponding node pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Node* GetNode(
        UINT nodeType,
        UINT instanceId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraRunningOnBPS
    ///
    /// @brief  Method to get the BPS camera info
    ///
    /// @param  pMetadataSlot       Pointer to metadata slot
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCameraRunningOnBPS(
        MetadataSlot* pMetadataSlot);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendOfflineShutterNotification
    ///
    /// @brief  Send shutter event for offline processing
    ///
    /// @param  requestId  id of the request to send
    /// @param  pTimestamp pointer to the timestamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SendOfflineShutterNotification(
        UINT64 requestId,
        UINT64* pTimestamp);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendShutterNotification
    ///
    /// @brief  Send shutter notification based on CSL frame message
    ///
    /// @param  pMessage CSL frame message
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SendShutterNotification(
        const CSLFrameMessage* pMessage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendSOFNotification
    ///
    /// @brief  Send SOF notification based on CSL frame message
    ///
    /// @param  pMessage CSL frame message
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SendSOFNotification(
        const CSLFrameMessage* pMessage);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateCurrExpTimeUseBySensor
    ///
    /// @brief  Pipeline update about current ExposureTime used by Sensor
    ///
    /// @param  currExposureTimeUseBySensor containing the current ExposureTime used by Sensor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateCurrExpTimeUseBySensor(
        UINT64 currExposureTimeUseBySensor);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RenegotiateInputBufferRequirement
    ///
    /// @brief  Method to switch node output format to NV12 and renegotiate
    ///
    /// @param  pCreateInputData    Pipeline create input data
    /// @param  pCreateOutputData   Pipeline create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RenegotiateInputBufferRequirement(
        PipelineCreateInputData*  pCreateInputData,
        PipelineCreateOutputData* pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FinalizeSensorModeInitalization
    ///
    /// @brief  Sensor mode intialization for External Sensors
    ///
    /// @param  pSensorModuleData    Image sensor module data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FinalizeSensorModeInitalization(
        const ImageSensorModuleData*  pSensorModuleData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetNumBatchedFrames
    ///
    /// @brief  SetNumBatchedFrames.
    ///
    /// @param  usecaseNumBatchedFrames        Number of batched frames
    /// @param  usecaseFPSValue                FPS info of batched frames
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetNumBatchedFrames(
        UINT usecaseNumBatchedFrames,
        UINT usecaseFPSValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLCRRawformatPorts
    ///
    /// @brief  SetLCRRawformatPorts
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetLCRRawformatPorts();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureMaxPipelineDelay
    ///
    /// @brief  ConfigureMaxPipelineDelay.
    ///
    /// @param  usecaseFPSValue                FPS info for the pipeline
    /// @param  maxPipelineDelay               Delay
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureMaxPipelineDelay(
        UINT usecaseFPSValue,
        UINT maxPipelineDelay);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeDebugDataBuffer
    ///
    /// @brief  Initialize debug/tuning data buffer, setting data blocks as un-used.
    ///
    /// @param  requestId   Request ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeDebugDataBuffer(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SealDebugDataBuffer
    ///
    /// @brief  Initialize debug/tuning data buffer, setting data blocks as un-used.
    ///
    /// @param  dataType    Debug-data type block to be seal/init
    /// @param  pDebugData  Curresponding debug-data buffer for the slot
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SealDebugDataBuffer(
        DebugDataType   dataType,
        DebugData*      pDebugData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishOutputDimensions
    ///
    /// @brief  Publish pipeline output dimensions
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishOutputDimensions();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishTargetFPS
    ///
    /// @brief  Publish pipeline target FPS
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishTargetFPS();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishSensorModeInformation
    ///
    /// @brief  Publish sensor mode information
    ///
    /// @param  requestId This is the request ID to publish the metadata
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishSensorModeInformation(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishSensorUsecaseProperties
    ///
    /// @brief  Publish sensor related information to usecase pool in case of offline pipeline
    ///
    /// @param  pSensorModuleData    Image sensor module data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishSensorUsecaseProperties(
        const ImageSensorModuleData* pSensorModuleData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FilterAndUpdatePublishSet
    ///
    /// @brief  Filter and update the pipeline publish set
    ///
    /// @param  pNode  Pointer to the Node instance
    ///
    /// @return CamxResultSuccess   if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FilterAndUpdatePublishSet(
        Node* pNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryEISCaps
    ///
    /// @brief  Fill EIS caps
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID QueryEISCaps()
    {
        const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();

        if (TRUE == pSettings->EISV2Enable)
        {
            /// @todo (CAMX-3135) Need to check node capabilities once that code is available
            m_videoStabilizationCaps = 1;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdatePublishTags
    ///
    /// @brief  Update the tags which will be published by the pipeline into the set
    ///
    /// @return CamxResultSuccess   if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdatePublishTags();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPipelineStatus
    ///
    /// @brief  Set pipeline status
    ///
    /// @param  status set pipeline status
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetPipelineStatus(
        PipelineStatus status)
    {
        if ((m_currentPipelineStatus != status) &&
            (PipelineStatus::INITIALIZED != status) &&
            (PipelineStatus::UNINITIALIZED != status))
        {
            CAMX_LOG_CONFIG(CamxLogGroupCore, "%s status is now %s",
                GetPipelineIdentifierString(),
                PipelineStatusStrings[static_cast<UINT>(status)]);
        }
        m_currentPipelineStatus = status;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSyncStatus
    ///
    /// @brief  Set Sync status
    ///
    /// @param  bSyncStatus sets the sync status
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetSyncStatus(
        BOOL bSyncStatus)
    {
        if (m_bCurrentSyncStatus != bSyncStatus)
        {
            CAMX_LOG_CONFIG(CamxLogGroupCore, "%s Sync Status is now %d and Sensor Sync Mode is %d",
                GetPipelineIdentifierString(),
                bSyncStatus,
                m_sensorSyncMode);
            m_bCurrentSyncStatus = bSyncStatus;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CallNodeAcquireResources
    ///
    /// @brief  Method to call acquire resources for all nodes in pipeline
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CallNodeAcquireResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CallNodeReleaseResources
    ///
    /// @brief  Method to call release resources for all nodes in pipeline
    ///
    /// @param  modeBitmask Deactivate pipeline mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CallNodeReleaseResources(
        CHIDEACTIVATEPIPELINEMODE modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulatePSMetadataSet
    ///
    /// @brief  Method to populate link based metadata set
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulatePSMetadataSet();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpPSMetadataInfo
    ///
    /// @brief  Method to dump port link based metadata info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpPSMetadataInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareResultPSM
    ///
    /// @brief  Prepare the result port-specific metadata that needs to be dispatched to the session
    ///
    /// @param  requestId       RequestId for which the result metadata must be prepared
    /// @param  pInputMetadata  Pointer to the input metadata
    /// @param  pOutputMetadata Pointer to the output metadata
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareResultPSM(
        UINT64      requestId,
        MetaBuffer* pInputMetadata,
        MetaBuffer* pOutputMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareResultMetadata
    ///
    /// @brief  Prepare the result metadata that needs to be dispatched to the session
    ///
    /// @param  pInputMetadata  Pointer to the input metadata
    /// @param  pOutputMetadata Pointer to the output metadata
    /// @param  requestId       Id of the request
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PrepareResultMetadata(
        MetaBuffer* pInputMetadata,
        MetaBuffer* pOutputMetadata,
        UINT64      requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckForRecovery
    ///
    /// @brief  Check for need to notify recovery based on CSL frame message
    ///
    /// @param  pMessage CSL frame message
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CheckForRecovery(
        const CSLFrameMessage* pMessage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// isVideoStream
    ///
    /// @brief  Check if the stream is video stream by looking at gralloc usage
    ///
    /// @param  usage gralloc usage of the stream
    ///
    /// @return TRUE if it is video stream
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL isVideoStream(
        GrallocUsage64 usage)
    {
        BOOL returnVal = FALSE;
        if (0 != (GrallocUsageHwVideoEncoder & usage))
        {
            returnVal = TRUE;
        }

        return returnVal;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishRequestHasVideoBufferTag
    ///
    /// @brief  Publish if request has video buffer tag indication
    ///
    /// @param  pPipelineRequestData Pointer to pipeline request data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID PublishRequestHasVideoBufferTag(
        PipelineProcessRequestData* pPipelineRequestData)
    {
        if ((NULL != pPipelineRequestData) &&
            (NULL != pPipelineRequestData->pCaptureRequest) &&
            (NULL != pPipelineRequestData->pCaptureRequest->pStreamBuffers))
        {
            StreamBufferInfo* pStreamBufferInfo        = pPipelineRequestData->pCaptureRequest->pStreamBuffers;
            UINT32            numOutputBuffers         = pStreamBufferInfo->numOutputBuffers;
            UINT32            requestHasVideoBufferTag = 0;
            UINT8             requestHasVideoBuffer    = 0;

            CamxResult result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.recording", "requestHasVideoBuffer",
                                                                         &requestHasVideoBufferTag);
            if (CamxResultSuccess == result)
            {
                for (UINT32 count = 0 ; count < numOutputBuffers; count++)
                {
                    GrallocUsage64 streamUsage = static_cast<GrallocUsage64>(
                                                 pStreamBufferInfo->outputBuffers[count].pStream->grallocUsage);
                    if (TRUE == isVideoStream(streamUsage))
                    {
                        requestHasVideoBuffer = 1;
                        break;
                    }
                }

                // Publish vendor tag to indicate that this request hold video output buffer
                MetadataPool* pMainPool = GetPerFramePool(PoolType::PerFrameResult);
                if (NULL != pMainPool)
                {
                    MetadataSlot* pMainMetaSlot = pMainPool->GetSlot(pPipelineRequestData->pCaptureRequest->requestId);
                    if (NULL != pMainMetaSlot)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupCore, "This request %llu has video request %d",
                                       pPipelineRequestData->pCaptureRequest->requestId,
                                       requestHasVideoBuffer);
                        pMainMetaSlot->SetMetadataByTag(requestHasVideoBufferTag,
                                                        &requestHasVideoBuffer,
                                                        sizeof(requestHasVideoBuffer),
                                                        GetPipelineIdentifierString());
                        pMainMetaSlot->PublishMetadataList(&requestHasVideoBufferTag, 1);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to obtain main pool matadata slot");
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to obtain matadata pool");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore,
                               "Failed to find org.quic.camera.recording.requestHasVideoBuffer, resultCode=%s",
                                Utils::CamxResultToString(result));
            }
        }
    }

    Pipeline(const Pipeline&) = delete;                                     ///< Disallow the copy constructor
    Pipeline& operator=(const Pipeline&) = delete;                          ///< Disallow assignment operator

    /// @brief Nodes that contain sink ports
    struct NodesWithSinkPorts
    {
        UINT numNodes;                                                      ///< Number of nodes with sink port(s)
        UINT nodeIndices[MaxNodeType];                                      ///< Index into m_ppNodes that are nodes with
                                                                            ///  sink port(s)
    };

    // Pipeline can never get called to process request 'n + RequestQueueDepth' until Request 'n' is done. But we consider
    // request 'n' to be done if all the outputs (buffers, metadata etc) for request 'n' is available. However there may be
    // nodes, like the StatsNode, that may still not be done for request 'n' and will complete at a later time. After it is
    // done, it will inform Pipeline that it is done for request n.
    //
    // Since request n is done, Pipeline may get called to process request n+RequestQueueDepth, at which time it will reset
    // its internal structures for request n+RequestQueueDepth. If we maintain an array of only RequestQueueDepth, then
    // request n and request n+RequestQueueDepth will map onto the same array element. And when Pipeline starts processing
    // request n+RequestQueueDepth it will reset the array element that was used for request n.
    //
    // Later when, for e.g. StatsNode is done and informs Pipeline for request n, Pipeline will refer to the structure that
    // was reset for request n+RequestQueueDepth!. To avoid the problem, we maintain twice the number of structures to be
    // on the safe side
    static const UINT     MaxPerRequestInfo = RequestQueueDepth * 2;    ///< Max number of per request info structures

    ChiContext*           m_pChiContext;                                ///< Chi context
    HwContext*            m_pHwContext;                                 ///< HW context pointer
    Session*              m_pSession;                                   ///< Session associated with this pipeline
    CSLHandle             m_hCSLSession;                                ///< CSL Session handle
    BOOL                  m_isFullRecovery;                             ///< indicate full recovery - incl sensor release
    MetadataPool*         m_pInputPool;                                 ///< Pointer to input metadata pool
    MetadataPool*         m_pMainPool;                                  ///< Pointer to main metadata/property pool
    MetadataPool*         m_pEarlyMainPool;                             ///< Pointer to main metadata/property pool
    MetadataPool*         m_pInternalPool;                              ///< Pointer to internal property pool
    MetadataPool*         m_pUsecasePool;                               ///< Pointer to usecase pool
    MetadataPool*         m_pDebugDataPool;                             ///< Pointer to debug-data pool
    MetadataPool*         m_pEmptyMetaDataPool;                         ///< Pointer to empty metadata pool
    ThreadManager*        m_pThreadManager;                             ///< Pointer to Thread Manager
    Node**                m_ppNodes;                                    ///< Created node pointers
    Node**                m_ppOrderedNodes;                             ///< List of nodes to call to finalize their buffer
                                                                        ///  properties and during per frame setup.
    UINT                  m_orderedNodeCount;                           ///< Number of nodes required to finalize the
                                                                        ///  negotiation process
    DeferredRequestQueue* m_pDeferredRequestQueue;                      ///< Pointer to the deferred process handler
    PerRequestInfo        m_perRequestInfo[MaxPerRequestInfo];          ///< Per request info
    StreamBufferInfo*     m_pStreamBufferBlob;                          ///< Rather than allocate separately for each
                                                                        ///  m_perRequestInfo[].request.pStreamBuffers, we'll
                                                                        ///  make them point into this array to avoid lots of
                                                                        ///  small mallocs.
    Mutex*                m_pPerRequestInfoLock;                        ///< Lock to access m_perRequestInfo
    NodesWithSinkPorts    m_nodesSinkOutPorts;                          ///< Information about nodes with sink port(s)
    NodesWithSinkPorts    m_nodesSourceInPorts;                         ///< Information about nodes with sink port(s)
    UINT                  m_nodeCount;                                  ///< Created node counts for m_ppNodes
    UINT                  m_lastRequestActiveStreamIdMask;              ///< Streams that were active in the last request
    UINT                  m_pipelineIndex;                              ///< Pipeline index in the session
    UINT                  m_numSessionPipelines;                        ///< Number of pipelines in the session this
                                                                        ///  pipeline belongs to
    /// @todo (CAMX-1797) Add back CSLMessage handler
    // CSLMessage            m_CSLMessage;                              ///< CSL frame message
    TuningDataManager*    m_pTuningManager;                             ///< Tuning manager
    PipelineFlags         m_flags;                                      ///< Pipeline flags
    INT32                 m_cameraId;                                   ///< CameraId associated with this pipeline if any
    const PipelineDescriptor* m_pPipelineDescriptor;                    ///< Pipeline descriptor
    UINT32                m_vendorTagIndexAVTimer;                      ///< Index for AV Timer vendor tag
    UINT32                m_vendorTagIndexTimestamp;                    ///< Index for timestamp vendor tag
    UINT32                m_vendorTagMultiCamInput;                     ///< Input Vendor tag for master/slave role switch
    UINT32                m_vendorTagMultiCamOutput;                    ///< Output Vendor tag for master/slave role switch
    CSLLinkHandle         m_hCSLLinkHandle;                             ///< CSL link handle
    CSLLinkHandle         m_hCSLSyncLinkHandle;                         ///< CSL sync link handle
    CSLDeviceHandle       m_hDevices[CamxMaxDeviceIndex];               ///< CSLDeviceHandles
    UINT32                m_currentSensorMode;                          ///< Sensor mode configured during node finalization
    SensorSyncMode        m_sensorSyncMode;                             ///< Sensor hardware sync mode
    UINT                  m_numConfigDoneNodes;                         ///< Num nodes the will report config done
    volatile UINT32       m_configDoneCount;                            ///< Config done count per reques, Only freely
                                                                        ///< accessed when holding m_pConfigDoneLock
    Mutex*                m_pConfigDoneLock;                            ///< Config done lock
    Condition*            m_pWaitForConfigDone;                         ///< Wait for config done
    UINT64                m_lastRequestId;                              ///< Last request id before stream off
    UINT64                m_lastQTimerTimestamp;                        ///< Last QTimer timestamp
    UINT64                m_lastMonoTimestamp;                          ///< Last MONOTONIC timestamp
    UINT32                m_videoStabilizationCaps;                     ///< EIS capabilities
    UINT64*               m_pCSLSyncIDToRequestId;                      ///< Mapping from CSL Sync ID to request ID
    ExternalSensor*       m_pExternalSensor;                            ///< External sensor Object
    BOOL                  m_debugDataInitialized;                       ///< Checks if the debug data is initialized

    volatile PipelineStatus        m_currentPipelineStatus;             ///< Flag to indicate pipeline status
    std::unordered_set<UINT32>     m_nodePublishSet;                    ///< Publish list from the pipeline
    std::unordered_set<UINT32>     m_nodePartialPublishSet;             ///< Partial Publist list
    UINT32                         m_metaBufferDelay;                   ///< Delay for releasing metabuffers
    BOOL                           m_bPartialMetadataEnabled;           ///< if partial Metadata is enabled
    UINT                           m_maxPipelineDelay;                  ///< Max pipeline delay configured
    Mutex*                         m_pResourceAcquireReleaseLock;       ///< Resource lock, used to syncronize
                                                                        ///< acquire resources and release resources
    UINT64                         m_lastCompletedRequestId;            ///< Last completed request Id
    UINT64                         m_lastInOrderCompletedRequestId;     ///< Last monotonically increasing request id, every
                                                                        ///< request id before it is guaranteed to be done,
                                                                        ///< NOT every request id after it is guaranteed to be
                                                                        ///< pending.
    BOOL                           m_isIPERealtime;                     ///< is IPE in RealTime
    UINT64                         m_lastSubmittedRequestId;            ///< Last request id before stream off
    UINT64                         m_lastSubmittedShutterRequestId;     ///< Last shutter notification request id
    UINT64                         m_firstPendingRequestId;             ///< First pending request id
    UINT64                         m_firstPendingCSLSyncId;             ///< First pending CSL sync id
    UINT                           m_numMaxBatchedFrames;               ///< Maximum number of batches supported
    BOOL                           m_HALOutputBufferCombined;           ///< Is the HAL output buffer combined for batch mode
    UINT                           m_invalidSOFCounter;                 ///< Keep track of consecutive SOF w/ invalid requestId
    volatile INT                   m_referenceCount;                    ///< Ref counted to help ensure that pipeline is only
                                                                        ///  destroyed when both the Session and Usecase have
                                                                        ///  given up references.
    volatile BOOL                  m_bCurrentSyncStatus;                ///< Flag to indicate sync status
    UINT                           m_portLinkPropertyCount;             ///< List of link based properties
    PSMetaMap                      m_psMetaMap;                         ///< Map of port-specific metadata to link information
    ChiMetadataIdArray             m_nodePSMetadataArray;               ///< List of port-specific metadata supported by the
                                                                        ///  pipeline
    UINT32                         m_streamMapVendorTag;                ///< Vendor tag location for stream map
    UINT32                         m_requestCropInfoVendorTag;          ///< Vendor tag location for request crop info
    UINT32                         m_enableAllCropFlagVendorTag;        ///< Vendor tag location for flag to enable all crop
    volatile UINT8                 m_aPipelineFlushState;               ///< There is a wait active on draining all
                                                                        ///  requests and results. May need to
                                                                        ///  acquire m_pFlushLock to access
                                                                        ///  this variable.

    CHAR                           m_pipelineIdentifierString[MaxStringLength256]; ///< Pipeline name and id
    Condition*                     m_pWaitAllNodesRequestDone;                     ///< Wait till all node requests are done
    volatile UINT                  m_isRequestDoneSignalNeeded;                    ///< Flag to indicate whether signal
                                                                                   ///  needed for all node requests
    Mutex*                         m_pNodesRequestDoneLock;                        ///< Lock for all node requests done
    BOOL                           m_isDelayedPipeline;                            ///< Flag to indicate if pipeline has a node
                                                                                   ///  with delayed notification
    BOOL                           m_isTriggeringRecovery;                           ///< Is recovery in progress
    FlushInfo                      m_flushInfo;                                    ///< Flush related information
    BOOL                           m_isStreamOnDone;                               ///< Flag to indicate whether stream on
                                                                                   ///  is done
                                                                                   ///  resource is done
    Mutex*                         m_pStreamOnDoneLock;                            ///< Lock used to signal stream on to flush
    Condition*                     m_pWaitForStreamOnDone;                         ///< Wait for stream done
    volatile UINT                  m_livePendingRequest;                           ///< Number of pending requests
    BOOL                           m_isWaitForConfigDone;                          ///< Is waiting for config done
    BOOL                           m_isConfigDoneAborted;                          ///< Is config done aborted
};

CAMX_NAMESPACE_END

#endif // CAMXPIPELINE_H
