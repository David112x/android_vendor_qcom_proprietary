////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxsession.h
/// @brief Declarations for Session class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXSESSION_H
#define CAMXSESSION_H

#include "camxchicontext.h"
#include "camxhwdefs.h"
#include "camxdefs.h"

// NOWHINE FILE NC003a: Long existing structures. To be cleaned up at a later point
// NOWHINE FILE NC008:  Long existing structures. To be cleaned up at a later point
// NOWHINE FILE DC012:  Long existing structures. To be cleaned up at a later point
// NOWHINE FILE CP021:  used default arguments for non-virtual and private methods


CAMX_NAMESPACE_BEGIN

static const UINT   ExtSensorNodeId = 13;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward Declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class  ChiContext;
class  Condition;
class  DeferredRequestQueue;
class  Device;
class  HAL3Queue;
class  MetadataPool;
class  Mutex;
class  Pipeline;
class  CmdBuffer;
class  CmdBufferManager;

enum class Format;

struct BufferRequirement;
struct PipelineProcessRequestData;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT32 NotificationListSize       = RequestQueueDepth * 3; ///< Max 3 times the Q depth for notification messages
static const UINT32 PartialMetadataListSize    = NotificationListSize;  ///< same as NotificationListSize

static const UINT32 OutputStreamBufferListSize = RequestQueueDepth * MaxNumOutputBuffers * 3; ///< Max 3 times the Q depth
                                                                                              ///  for output stream buffers

static const UINT32 InvalidPipelineIndex       = 0xFFFFFFFF;            ///< Invalid pipeline index
static const UINT32 MaxRealTimePipelines       = 4;                     ///< Max real time pipelines in one session
static const UINT32 MaxActiveRealtimePipelines = 2;                     ///< Max active realtime pipelines
static const UINT32 MaxSyncLinkCount           = 2;
static const UINT32 MaxBufferMemFlags          = 16;
static const UINT32 MaxLinkPropFlags           = 4;                     ///< Max link specific settings allowed
static const UINT32 MaxQueueDepth              = RequestQueueDepth * 8; ///< Max 8 times of the queue depth
static const UINT32 EmptyMetadataEntryCapacity = 1024;                  ///< Replaced metadata entry capacity
static const UINT32 EmptyMetadataDataCapacity  = 10 * 1024;             ///< Replaced metadata data capacity
static const UINT32 MetaBufferDoneMetaReady    = 0x1;                   ///< Flag to indicate if metadata is ready
static const UINT32 MetaBufferDoneBufferReady  = 0x2;                   ///< Flag to indicate if buffer is ready
static const UINT32 MaxSessionPerRequestInfo   = 50;                    ///< Max requests to store processing time
static const UINT32 RealTimeMetadataDumpMask   = 0xA;                   ///< Mask for realtime metadata dumps
static const UINT32 OfflineMetadataDumpMask    = 0x5;                   ///< Mask for offline metadata dumps
static const UINT32 MaxWaitTimeForFlush        = 500;                   ///< Wait time for flush
static const UINT   KmdBufferSize              = 7 * 1024 * 1024;       ///< KMD debug buffer size in bytes

enum class BufferMemFlag
{
    Hw           = 0,                ///< Hardware  access
    Protected    = 1,                ///< Protected access
    CmdBuffer    = 2,                ///< CmdBuffer access
    UMDAccess    = 3,                ///< UMD       access
    Cache        = 4,                ///< Cached    access
    PacketBuffer = 5,                ///< Packet    access
    KMDAccess    = 6,                ///< KMD       access
    Max          = 7,                ///< Max
};

enum class LinkPropFlags
{
    DisableLateBinding   = 0,        ///< Disables late binding on image buffer manager of the output port of this link
    DisableSelfShrinking = 1,        ///< Disables self shrinking of buffers allocated at the output port of this link
    SinkInplaceBuffer    = 2,        ///< Indicates that the buffer at this output port of the inplace node is a sink buffer
    Max                  = 3,        ///< Max
};

/// @todo (CAMX-1512) Add static asserts
enum class NodeClass
{
    Default = 0,                             ///< Default node type
    Bypass  = 1,                             ///< Bypass node
    Inplace = 2                              ///< Inplace node
};

enum class SessionDumpFlag
{
    Flush           = 0,                ///< Session dump from Flush
    SigAbort        = 1,                ///< Session dump from SigAbort
    ResetKMD        = 2,                ///< Session dump from Reset from KMD
    ResetUMD        = 3,                ///< Session dump from Reset from UMD
    ResetRecovery   = 4,                ///< Session dump from Reset from Recovery
    ChiContextDump  = 5                 ///< Session dump from ChiContextDump
};

enum class StreamStatus
{
    NOT_STREAMING,      ///< Pipeline is not streaming
    NON_SYNC_STREAMING, ///< Pipeline is streaming in non sync mode
    SYNC_STREAMING      ///< Pipeline is streaming in sync mode
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SessionSync
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SessionSync
{
    StreamStatus    streamStatus[MaxPipelinesPerSession];    ///< Stream Status of the pipelines
    UINT32          syncModeTagID;                           ///< Pipeline Sync mode Tag ID
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NodePropertyType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NodePropertyType
{
    UINT32          NodePropertyId;
    CHAR*           NodePropertyValue;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NodeInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NodeInfo
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NodeProperty - Array
    /// Min Length:    0
    /// Max Length:    Unbounded
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32            NodePropertyCount;
    UINT32            NodePropertyID;
    NodePropertyType* NodeProperty;
    CHAR*             NodeName;
    UINT32            NodeId;
    CHAR*             NodeInstance;
    UINT32            NodeInstanceId;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// NodesListInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NodesListInfo
{

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Node - Array
    /// Min Length:    1
    /// Max Length:    Unbounded
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32    NodeCount;
    UINT32    NodeID;
    NodeInfo* Node;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PortInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PortInfo
{
    CHAR*   PortName;
    UINT32  PortId;
    UINT32  PortSrcTypeIdExists;
    UINT32  PortSrcTypeIdID;
    UINT32  PortSrcTypeId;
    UINT32  BypassPortSrcIdCount;
    UINT32  BypassPortSrcIdID;
    UINT32* BypassPortSrcId;
    CHAR*   NodeName;
    UINT32  NodeId;
    CHAR*   NodeInstance;
    UINT32  NodeInstanceId;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LinkPropertiesInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct LinkPropertiesInfo
{
    BOOL BatchMode;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BufferPropertiesInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BufferPropertiesInfo
{
    Format           BufferFormat;
    UINT32           BufferSizeExists;
    UINT32           BufferSizeID;
    UINT32           BufferSize;
    UINT32           BufferImmediateAllocCount;
    UINT32           BufferQueueDepth;
    UINT32           BufferHeap;
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BufferFlags - Array
    /// Min Length:    1
    /// Max Length:    Unbounded
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32           BufferFlagsCount;
    UINT32           BufferFlagsID;
    BufferMemFlag*   BufferFlags;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// LinkInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct LinkInfo
{
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LinkProperties - Optional
    /// Min Length:    0
    /// Max Length:    1
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32                LinkPropertiesExists;
    UINT32                LinkPropertiesID;
    LinkPropertiesInfo    LinkProperties;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SrcPort
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PortInfo              SrcPort;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DstPort - Array
    /// Min Length:    1
    /// Max Length:    Unbounded
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32                DstPortCount;
    UINT32                DstPortID;
    PortInfo*             DstPort;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BufferProperties - Optional
    /// Min Length:    0
    /// Max Length:    1
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32                BufferPropertiesExists;
    UINT32                BufferPropertiesID;
    BufferPropertiesInfo  BufferProperties;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PortLinkage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PortLinkage
{
    UINT32               SourceNodeExists;
    UINT32               SourceNodeID;
    CHAR*                SourceNode;
    UINT32               SourceNodeInstanceExists;
    UINT32               SourceNodeInstanceID;
    CHAR*                SourceNodeInstance;
    UINT32               TargetNameExists;
    UINT32               TargetNameID;
    CHAR*                TargetName;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TargetDirection - Optional
    /// Min Length:    0
    /// Max Length:    1
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32               TargetDirectionExists;
    UINT32               TargetDirectionID;
    ChiStreamType        TargetDirection;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Link - Array
    /// Min Length:    1
    /// Max Length:    Unbounded
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32               LinkCount;
    UINT32               LinkID;
    LinkInfo*            Link;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PipelineInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PipelineInfo
{
    CHAR*         PipelineName;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NodesList
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    NodesListInfo NodesList;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PortLinkages
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PortLinkage   PortLinkages;
};


/// @brief Input port info based on XML
struct InputPortInfo
{
    CHAR*                   pPortName;                ///< Input port name
    UINT                    portId;                   ///< Input port Id
    UINT                    parentOutputPortId;       ///< Parent output port id
    UINT                    parentNodeIndex;          ///< Index of the parent node
    UINT                    portSourceTypeId;         ///< Port source type id

    union Flags
    {
        struct
        {
            BIT isSourceBuffer              : 1;  ///< Node with a source port that inputs a buffer
            BIT reserved                    : 31; ///< Reserved
        };

        UINT value;
    } flags;
};

/// @brief Input ports list based on XML
struct InputPorts
{
    UINT            numPorts;      ///< Number of ports
    InputPortInfo*  pPortInfo;     ///< Individual port info
};

/// @brief Link Properties
struct LinkProperties
{
    BOOL            isBatchMode;                       ///< Batch mode indicator
    UINT            numLinkFlags;                      ///< Number of link flags
    LinkPropFlags   LinkFlags[MaxLinkPropFlags];       ///< Link flags indicator
};

/// @brief Buffer Properties
struct LinkBufferProperties
{
    Format         format;                          ///< Format
    UINT32         sizeBytes;                       ///< Size in bytes
    UINT32         immediateAllocCount;             ///< Count of buffers to allocate immediately
    UINT32         queueDepth;                      ///< Number of buffer allocations for the link
    UINT32         heap;                            ///< Buffer heap
    UINT           numMemFlags;                     ///< Number of mem flags
    BufferMemFlag  memFlags[MaxBufferMemFlags];     ///< Mem flags
};

/// @brief Port Link Information
struct PortLink
{
    UINT                 numInputPortsConnected;    ///< Number of inputs connected to this output port
    LinkProperties       linkProperties;            ///< Properties of the link itself
    LinkBufferProperties linkBufferProperties;      ///< Properties of the buffer associated with the link
};

/// @brief Output port info based on XML
struct OutputPortInfo
{
    PortLink portLink;                  ///< Info about the link
    CHAR*    pPortName;                 ///< Port Name
    UINT     portId;                    ///< Port Id
    UINT     portSourceTypeId;          ///< Port source type id
    UINT     numSourceIdsMapped;        ///< number of source port Ids mapped
    UINT*    pMappedSourcePortIds;      ///< pointer to the source port Ids mapped for bypass

    union Flags
    {
        struct
        {
            BIT isSinkBuffer    : 1;    ///< Node with a sink port that outputs a buffer
            BIT isSinkNoBuffer  : 1;    ///< Node with a sink port that does not output a buffer
            BIT reserved        : 30;   ///< Reserved
        };

        UINT value;                     ///< Value of the union
    } flags;
};

/// @brief Output ports list based on XML
struct OutputPorts
{
    UINT            numPorts;     ///< Number of ports
    OutputPortInfo* pPortInfo;    ///< Individual port info
};

/// @brief node property
struct PerNodeProperty
{
    UINT32 id;                            ///< ID of the property
    VOID*  pValue;                        ///< Pointer to data. Type dependent on ID
};

/// @brief Per node info based on XML
struct PerNodeInfo
{
    NodeInfo*        pNodeInfo;             ///< Node info coming from XML parser
    CHAR*            pNodeName;             ///< Node name
    CHAR*            pInstanceName;         ///< Node instance id
    UINT             nodePropertyCount;     ///< Node property count
    PerNodeProperty* pNodeProperties;       ///< Properties associated with the node
    UINT             nodeId;                ///< Node Id
    UINT             instanceId;            ///< Instance Id
    NodeClass        nodeClass;             ///< Node class, bypassable node or default node
    InputPorts       inputPorts;            ///< Input ports
    OutputPorts      outputPorts;           ///< Output ports
};

/// @brief Per pipeline info based on XML
struct PerPipelineInfo
{
    UINT         numNodes;    ///< Number of nodes in the topology matching the usecase
    PerNodeInfo* pNodeInfo;   ///< Individual node info
};

/// @brief Node Properties
/// @todo (CAMX-1512) Better way to handle this
struct NodePropertyHandle
{
    VOID*  pNodeProperties;             ///< Properties associated with the node
    UINT32 nodeId;                      ///< Node identifier
    UINT32 nodeInstanceId;              ///< Node instance identifier
};

/// @brief Pipeline output data
struct PipelineOutputData
{
    ChiLinkNodeDescriptor nodePort;                                 ///< Node/port with which this output is associated with
    ChiStreamWrapper*     pOutputStreamWrapper;                     ///< Output buffer descriptor
};

/// @brief Sensor info
struct SensorInfo
{
    UINT32            cameraId;                                    ///< CameraId
    ChiSensorModeInfo sensorMode;                                  ///< Mode info
};

/// @brief Pipeline input data, some of which is given at session create time along with the pipeline descriptor
///        Session create time data: sensorInfo / inputstreamWrapper+isWrapperOwner
struct PipelineInputData
{
    ChiLinkNodeDescriptor nodePort;                                 ///< Node/port with which this input is associated with

    union
    {
        ChiBufferOptions bufferOptions;                             ///< Input buffer options for the input port
        SensorInfo       sensorInfo;                                ///< Sensor info
    };

    ChiStreamWrapper*     pInputStreamWrapper;                      ///< App selected input buffer descriptor
    BOOL                  isWrapperOwner;                           ///< Is the pipeline descriptor the creator of this wrapper
                                                                    ///  so that it knows whether to destroy it or not
};

/// @brief Pipeline descriptor flags
union PipelineDescriptorFlags
{
    struct
    {
        BIT isRealTime      : 1;                                        ///< Is this a real time pipeline
        BIT isSensorInput   : 1;                                        ///< Is the pipeline's input sensor
        BIT isSecureMode    : 1;                                        ///< Mode of pipeline Secure/Non-secure
        BIT isTorchWidget   : 1;                                        ///< Is the torch widget mode
        BIT isHFRMode       : 1;                                        ///< Is HFR mode.
        BIT reserved        : 27;                                       ///< Reserved bits
    };

    UINT32 allFlagsValue;
};

/// @brief Pipeline descriptor handle used to describe a pipeline
struct PipelineDescriptor
{
    PerPipelineInfo         pipelineInfo;                               ///< Pipeline info that will be used to create the
                                                                        ///  actual pipeline object
    UINT                    numOutputs;                                 ///< Number of outputs (<= MaxPipelineOutputs)
    PipelineOutputData      outputData[MaxPipelineOutputs];             ///< Output data
    UINT                    numInputs;                                  ///< Number of inputs  (<= MaxPipelineInputs)
    PipelineInputData       inputData[MaxPipelineInputs];               ///< Input data for this pipeline
    PipelineDescriptorFlags flags;                                      ///< Flags
    UINT                    numBatchedFrames;                           ///< Number of framework frames batched
    BOOL                    HALOutputBufferCombined;                    ///< Is the HAL output buffer combined for batch mode
    UINT                    maxFPSValue;                                ///< maxFPSValue info of batched frames
    UINT32                  cameraId;                                   ///< Camera Id of pipeline
    CHAR                    pipelineName[MaxStringLength256];           ///< Name of this pipeline
    VOID*                   pPrivData;                                  ///< Camx private data, carrying pipeline obj ptr.
    MetaBuffer*             pSessionMetadata;                           ///< Metadata buffer published by the Chi Usecase
};

/// @brief Pipeline finalization data needed for pipeline defer finalization
struct PipelineDeferredFinalizeData
{
    UINT32 pipelineResourcePolicy;     ///< pipeline resource policy
};

/// @brief Pipeline entry for holding the requests for which metadata buffers are pending release
struct PipelineMetaBufferDoneEntry
{
    Pipeline* pPipeline;              ///< Pointer to the pipeline
    UINT32    oldestRequestId;        ///< Oldest request which is pending MetaBuffer notification
    UINT32    latestRequestId;        ///< Last request which is pending MetaBuffer notification
    UINT32    maxDelay;               ///< Max delay per pipeline
    BOOL      isReady[MaxQueueDepth]; ///< Flag to indicate if the metabuffer done is pending
};

/// @brief Session queue for holding the requests for which metadata buffers are pending release
struct PendingMetaBufferDoneQueue
{
    UINT32  oldestSequenceId;             ///< Oldest request which is pending MetaBuffer notification
    UINT32  latestSequenceId;             ///< Last request for which the results are available
    UINT32  pendingMask[MaxQueueDepth];   ///< Circular buffer to hold the mask for pending metadata/buffer
                                          ///  for the requests
    UINT32  pipelineIndex[MaxQueueDepth]; ///< Pipeline index of the sequenceId
    UINT32  requestId[MaxQueueDepth];     ///< Pipeline requestId corresponding to the sequenceId

    PipelineMetaBufferDoneEntry pipelineEntry[MaxPipelinesPerSession]; ///< Metadata done Entry per pipeline
};

/// @brief Session pipeline data containing information pertaining to the session only
struct SessionPerPipelineData
{
    PipelineDescriptor*          pPipelineDescriptor;        ///< Complete copy of what the API passes in
                                                             ///  (no reference to app memory)
    Pipeline*                    pPipeline;                  ///< Pipeline object
    PipelineDeferredFinalizeData pipelineFinalizeData;       ///< Pipeline finalize data needed for finalize
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Combine error and result status codes for easy processing in the HAL
enum class ResultType
{
    EarlyMetadataOK,///< Metadata result component looks good
    MetadataOK,     ///< Metadata result component looks good
    BufferOK,       ///< Buffer result component looks good
    MetadataError,  ///< Metadata result component is in error
    BufferError,    ///< Buffer result component is in error
    DeviceError,    ///< Equivalent to MessageCodeDevice
    RequestError,   ///< Equivalent to MessageCodeRequest
    Invalid         ///< Max out
};

/// @brief Holder structure to store a result per pipeline request
struct ResultHolder
{
    UINT32                  sequenceId;                              ///< ID assigned for this result
    UINT32                  numErrorBuffersSent;                     ///< Number of buffer errors notify that were injected
    UINT32                  numOkBuffersSent;                        ///< Number of good buffers injected
    UINT32                  numOutBuffers;                           ///< Expected number of Output buffers of this result
    BOOL                    isAlive;                                 ///< Is alive
    BOOL                    isCancelled;                             ///< Indicate whether the whole request was cancelled
    UINT32                  numInBuffers;                            ///< Number of Input buffers of this result
    UINT32                  pendingMetadataCount;                    ///< Outstanding metadata count for this result
    BOOL                    tentativeMetadata;                       ///< For offline PCRs, we may not have result metadata
    ChiMessageDescriptor*   pMetadataError;                          ///< Placeholder for metadata result if in error
    MetaBuffer*             pMetaBuffer[MaxMetadataCHI];             ///< Placeholder for good metadata result and input
                                                                     ///  metadata for this request
    UINT32                  metadataCbIndex;                         ///< Next index where we can the slot the metadata to
    VOID*                   pPrivData;                               ///< Result private data
    UINT32                  pipelineIndex;                           ///< Pipeline index
    UINT32                  requestId;                               ///< Request ID for the pipeline
    BOOL                    isShutterSentOut;                        ///< Did we dispatch a ShutterMessage to the framework
    UINT32                  expectedExposureTime;                    ///< The corresponding request's exposure time (in ms)

    struct BufferResult
    {
        ChiStream*            pStream;                          ///< O/P Stream pointer to which this buffer belongs
        ChiStreamBuffer*      pBuffer;                          ///< Placeholder for good output buffer result
        ChiMessageDescriptor* pBufferError;                     ///< Placeholder for output buffer if in error
        BOOL                  error;                            ///< TRUE if this buffer is in an error state
        BOOL                  valid;                            ///< Flag for buffer is invalid or not
    } bufferHolder[MaxNumOutputBuffers];                        ///< Placeholder for result buffers

    struct InputBufferInfo
    {
        ChiStream*       pStream;                        ///< I/P Stream pointer to which this buffer belongs
        ChiStreamBuffer* pBuffer;                        ///< Placeholder for input buffer
    } inputbufferHolder[MaxNumInputBuffers];             ///< Placeholder for input buffers
};

/// @brief Holder structure to store a result per session request
struct SessionResultHolder
{
    ResultHolder    resultHolders[MaxPipelinesPerSession];  ///< result holder per pipeline request
    UINT32          numResults;                             ///< NUmber of pipeline result holders
};

/// @brief Holder structure to store timestamps for a session request
struct PerResultHolderInfo
{
    UINT32 startTime;      ///< Timestamp for start of request processing
    UINT32 endTime;        ///< Timestamp for end of request processing
    UINT64 requestId;      ///< Associated requestId
    UINT64 CSLSyncId;      ///< Associated CSLSyncId
};

/// @brief Holder structure to store buffer of per session request timing info
struct SessionPerRequestInfo
{
    PerResultHolderInfo requestInfo[MaxSessionPerRequestInfo];      ///< Buffer to hold timing related information of request
    UINT                lastSessionRequestIndex;                    ///< Most recent requestId index we populated the
                                                                    ///  m_perRequestInfo buffer with
};


/// @brief Stream buffers used to convey the result of a capture request
/// @note The size of the array in the structure is sufficient to handle max outstanding requests i.e. pipeline depth. So there
///       should never be a case where we run out of result stream buffers.
struct ResultStreamBuffers
{
    ChiStreamBuffer resultStreamBuffer[OutputStreamBufferListSize]; ///< Stream buffers
    UINT            freeIndex;                                      ///< Next free element in the array
    Mutex*          pLock;
};

/// @brief Notify messages used to convey generic messages (type "ChiMessageType")
/// @note The size of the array in the structure is sufficient to handle max outstanding requests i.e. pipeline depth. So there
///       should never be a case where we run out of notify message structures.
struct NotifyMessages
{
    ChiMessageDescriptor* pNotifyMessage;                          ///< Stream buffers
    UINT                  freeIndex;                               ///< Next free element in the array
    Mutex*                pLock;
};

/// @brief Messages used for Partial Meta data
/// @note The size of the array in the structure is sufficient to handle max outstanding requests i.e. pipeline depth. So there
///       should never be a case where we run out of partial data message structures.
struct PartialdataMessages
{
    ChiPartialCaptureResult* pPartialCaptureMessage;               ///< Stream buffers
    UINT                     freeIndex;                            ///< Next free element in the array
    Mutex*                   pLock;
};

/// @brief Data required for creating a Pipeline
struct PipelineData
{
    PipelineDescriptor*   pPipelineDescriptor;                     ///< Pipeline descriptor
    ChiPipelineInputInfo* pChiPipelineInputInfo;                   ///< Chi pipeline input info
};

/// @brief Data required for creating a Session
struct SessionCreateData
{
    HwContext*       pHwContext;                                   ///< Context to which this session is associated with
    ChiContext*      pChiContext;                                  ///< CHI Context
    ThreadManager*   pThreadManager;                               ///< ThreadManager
    ChiPipelineInfo* pPipelineInfo;                                ///< Pipeline Info
    UINT             numPipelines;                                 ///< Number of pipeline handles being passed in
    UINT             usecaseNumBatchedFrames;                      ///< Number of frames batched
    BOOL             HALOutputBufferCombined;                      ///< Is the HAL output buffer combined for batch mode
    ChiCallBacks*    pChiAppCallBacks;                             ///< Result callbacks made into the Chi app
    VOID*            pPrivateCbData;                               ///< Private data
    BOOL             isNativeChi;                                  ///< to indicate if session is created from Native Chi
    ChiSessionFlags  flags;                                        ///< flags for session
};

/// @brief Overall callback structure used between Pipeline and Session
struct ResultsData
{
    CbType  type;                       ///< One of the different callback types
    UINT    pipelineIndex;              ///< Pipeline index in the session

    union
    {
        CbPayloadError              error;           ///< Error payload
        CbPayloadAsync              async;           ///< Async payload
        CbPayloadSof                sof;             ///< Sof payload
        CbPayloadMetadata           metadata;        ///< Metadata payload
        CbPayloadPartialMetadata    partialMetadata; ///< Partial Metadata payload
        CbPayloadBuffer             buffer;          ///< Buffer payload
        CbPayloadMetaBufferDone     metabufferDone;  ///< Metadata Buffer payload
    } cbPayload;                                     ///< One of the Callback payloads

    VOID* pPrivData;                    ///< private data for result
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetPipelineDescriptor
///
/// @brief  Get the Pipeline descriptor pointer
///
/// @param  hPipelineDescriptor Pipeline descriptor pointer
///
/// @return Pipeline descriptor pointer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CAMX_INLINE PipelineDescriptor* GetPipelineDescriptor(
    CHIPIPELINEDESCRIPTOR hPipelineDescriptor)
{
    CAMX_ASSERT(NULL != hPipelineDescriptor);

    return reinterpret_cast<PipelineDescriptor*>(hPipelineDescriptor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DetermineActiveStreams
///
/// @brief  Determine active streams in the process request
///
/// @param  pPipelineProcessRequestData active streams info will be written in here
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DetermineActiveStreams(
    PipelineProcessRequestData*  pPipelineProcessRequestData);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SetupRequestData
///
/// @brief  Setup the necessary data for a result pipeline request
///
/// @param  pRequest        The request whose results will be setup.
/// @param  pOutRequestData The pipeline data for the request.
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SetupRequestData(
    CaptureRequest*             pRequest,
    PipelineProcessRequestData* pOutRequestData);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GetBatchedHALOutputNum
///
/// @brief  Get the number of HAL output buffer for batch mode.
///         If the flag HALOutputBufferCombined is TRUE, the HAL output buffer will be combined into one single buffer.
///         Otherwise, one video output buffer maps to one HAL output buffer, so the output buffer number equal to
///         the numBatchedFrames.
///
/// @param  numBatchedFrames     the number of batched frames for HFR mode.
/// @param  HALOutputBufferCombined     the number of batched frames for HFR mode.
/// @return Number of batched frames
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE UINT GetBatchedHALOutputNum(
    UINT numBatchedFrames,
    BOOL HALOutputBufferCombined)
{
    UINT frames = numBatchedFrames;
    if (TRUE == HALOutputBufferCombined)
    {
        frames = 1;
    }

    return frames;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Session contains information about pipelines and requests/results related to them
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Session
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyResult
    ///
    /// @brief  Pipelines notifies about the results via this interface
    ///
    /// @param  pResultsData Structure containing the results data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyResult(
        ResultsData* pResultsData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsRealtimeSession
    ///
    /// @brief  Check if session is realtime
    ///
    /// @return TRUE if session has any realtime pipeline
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsRealtimeSession()
    {
        return m_isRealTime;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessCaptureRequest
    ///
    /// @brief  This method is used by the application framework to send a new capture request
    ///
    /// @param  pCaptureRequests    The metadata and input/output buffers to use for the capture request.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessCaptureRequest(
         const ChiPipelineRequest* pCaptureRequests);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StreamOn
    ///
    /// @brief  StreamOn the hardware using the pipeline handle
    ///
    /// @param  hPipelineDescriptor Pipeline Descriptor to which the request is issued
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StreamOn(
        CHIPIPELINEHANDLE hPipelineDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StreamOff
    ///
    /// @brief  StreamOff the hardware using the pipeline handle
    ///
    /// @param  hPipelineDescriptor Pipeline Descriptor to which the request is issued
    /// @param  modeBitmask         Deactivate mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StreamOff(
        CHIPIPELINEHANDLE           hPipelineDescriptor,
        CHIDEACTIVATEPIPELINEMODE   modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushStatus
    ///
    /// @brief  Get the Pipeline descriptor pointer
    ///
    /// @return Boolean status indicating flush
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL GetFlushStatus();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CanRequestProceed
    ///
    /// @brief  Push an incoming request to the request queue till space is available or there is device error
    ///
    /// @param  pRequest The incoming request
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CanRequestProceed(
        const ChiCaptureRequest* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMultiRequestSyncData
    ///
    /// @brief  Push an incoming request to the request queue till space is available or there is device error
    ///
    /// @param  pRequest The incoming request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateMultiRequestSyncData(
        const ChiPipelineRequest* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineIndex
    ///
    /// @brief  Get pipeline index by pipeline descriptor
    ///
    /// @param  hPipelineDescriptor pipeline descriptor
    ///
    /// @return index of pipeline
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetPipelineIndex(
        CHIPIPELINEHANDLE hPipelineDescriptor)
    {
        UINT32 pipelineIndex = InvalidPipelineIndex;
        UINT32 index;
        if (m_numPipelines >= MaxPipelinesPerSession)
        {
            return pipelineIndex;
        }
        for (index = 0; index < m_numPipelines; index++)
        {
            if (hPipelineDescriptor == m_pipelineData[index].pPipelineDescriptor)
            {
                   // found corresponding pipeline can use index to get to it
                pipelineIndex = index;
                break;
            }
        }
        return pipelineIndex;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckValidChiStreamBuffer
    ///
    /// @brief  Check if the incoming Chi Streambuffer is valid
    ///
    /// @param  pStreamBuffer    Incoming Chi StreamBuffer
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckValidChiStreamBuffer(
        const ChiStreamBuffer* pStreamBuffer
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckValidInputRequest
    ///
    /// @brief  Check if the incoming request is well formed and valid
    ///
    /// @param  pRequest    Incoming request
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckValidInputRequest(
        const ChiCaptureRequest* pRequest
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitOnAcquireFence
    ///
    /// @brief  Wait on the acquire fence of all HAL output buffers before accepting a request
    ///
    /// @param  pRequest The request to wait on
    ///
    /// @return CamxResultSuccess if wait is succesful on all buffers, CamxResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WaitOnAcquireFence(
        const ChiCaptureRequest* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDeviceInError
    ///
    /// @brief  Sets the device error status
    ///
    /// @param  isError Error status
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDeviceInError(
        BOOL isError)
    {
        CamxAtomicStore32(&m_aDeviceInError, isError);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDeviceInError
    ///
    /// @brief  Get the device error status
    ///
    /// @return TRUE if the device is in an error state, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL GetDeviceInError()
    {
        return static_cast<BOOL>(CamxAtomicLoad32(&m_aDeviceInError));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroys the object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief  Flush the session. This call is blocking.
    ///
    /// @param  pSessionFlushInfo Flush info param from Chi
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Flush(
        const CHISESSIONFLUSHINFO* pSessionFlushInfo = NULL);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FinalizeDeferPipeline
    ///
    /// @brief  Finalize the deferred pipeline
    ///
    /// @param  pipelineIndex        Pipeline index
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FinalizeDeferPipeline(
        UINT32 pipelineIndex);

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
    /// SetIPERTPipeline
    ///
    /// @brief  SetIPERTPipeline
    ///
    /// @param  isRealTime Boolean stating if pipeline is realtime
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetIPERTPipeline(
         BOOL isRealTime)
    {
        m_isIPERealtime = isRealTime;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIPERTPipeline
    ///
    /// @brief  GetIPERTPipeline
    ///
    /// @return True or False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL GetIPERTPipeline()
    {
        return m_isIPERealtime;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentSequenceId
    ///
    /// @brief  Get current session sequence Id
    ///
    /// @return Current sequence id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetCurrentSequenceId();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentRequestQueueDepth
    ///
    /// @brief  Get current request queue depth
    ///
    /// @return Current request queue depth
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetCurrentRequestQueueDepth()
    {
        return m_requestQueueDepth;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentPipelineRequestId
    ///
    /// @brief  Get current pipeline request Id based on pipeline index
    ///
    /// @param  pipelineIndex   Pipeline index
    ///
    /// @return Current pipeline request id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 GetCurrentPipelineRequestId(
        UINT pipelineIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsResultHolderEmpty
    ///
    /// @brief  Check whether result holder is empty
    ///
    /// @return True/False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsResultHolderEmpty();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPipelineRealTime
    ///
    /// @brief  Check if pipeline is realtime
    ///
    /// @param  hPipelineDescriptor   Pipeline descriptor
    ///
    /// @return True if realtime false if not
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPipelineRealTime(
        CHIPIPELINEHANDLE hPipelineDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMultiCamera
    ///
    /// @brief  Check if this session is multi camera or not
    ///
    /// @return True if Session has two real time pipelines, False if not
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsMultiCamera()
    {
        BOOL isMultiCamera = FALSE;

        if (m_numInputSensors > 1)
        {
            isMultiCamera = TRUE;
        }

        return isMultiCamera;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpState
    ///
    /// @brief  Dumps snapshot of current state to a file
    ///
    /// @param  fd      file descriptor.
    /// @param  indent  indent spaces.
    /// @param  flag    indicating why the session was dumped.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpState(
        INT             fd,
        UINT32          indent,
        SessionDumpFlag flag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpDebugInfo
    ///
    /// @brief  Dumps current session info
    ///
    /// @param  flag  Indicates what kind of session dump we are doing
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpDebugInfo(
        SessionDumpFlag flag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataInfo
    ///
    /// @brief  Method to query metadata information
    ///
    /// @param  hPipelineDescriptor     Pipeline handle
    /// @param  maxPublishTagArraySize  Publish tag array size
    /// @param  pPublishTagArray        Array of tags published by the pipeline
    /// @param  pPublishTagCount        Pointer to the count of the tags published by the pipeline
    /// @param  pPartialPublishTagCount Pointer to the count of the partial tags published by the pipeline
    /// @param  pMaxNumMetaBuffers      Pointer to the maximum number of metadata buffers required by the pipeline
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult QueryMetadataInfo(
        const CHIPIPELINEHANDLE hPipelineDescriptor,
        const UINT32            maxPublishTagArraySize,
        UINT32*                 pPublishTagArray,
        UINT32*                 pPublishTagCount,
        UINT32*                 pPartialPublishTagCount,
        UINT32*                 pMaxNumMetaBuffers);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLSession
    ///
    /// @brief  Get the CSL session handle for this session.
    ///
    /// @return The CSL session handle.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CSLHandle GetCSLSession() const
    {
        return m_hCSLSession;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyProcessingDone
    ///
    /// @brief  Notify the session that all the results were posted or all of a pipelines nodes were done, if both conditions
    ///         are true, we can proceed with flush.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyProcessingDone();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxLivePendingRequestsOfSession
    ///
    /// @brief  Return maxLivePendingRequests from session
    ///
    /// @return Max number of live pending requests
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetMaxLivePendingRequestsOfSession()
    {
        return m_maxLivePendingRequests;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessAcquireResource
    ///
    /// @brief  The callback function Resource Manager will call to ask this client to acquire resource.
    ///
    /// @param  pClientHandle         Parameters used for this callback
    /// @param  pPrivateCallbackData  Callback private data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static CamxResult ProcessAcquireResource(
        VOID*   pClientHandle,
        VOID*   pPrivateCallbackData)
    {
        Session* pSession = static_cast<Session*>(pPrivateCallbackData);
        CamxResult result = CamxResultSuccess;

        if (NULL != pSession)
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "session: %p, pipeline handle:%p", pSession, pClientHandle);
            result = pSession->StreamOn(static_cast<CHIPIPELINEHANDLE>(pClientHandle));
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessReleaseResource
    ///
    /// @brief  The callback function Resource Manager will call to ask this client to release resource.
    ///
    /// @param  pClientHandle         Parameters used for this callback
    /// @param  pPrivateCallbackData  Callback private data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static CamxResult ProcessReleaseResource(
        VOID*   pClientHandle,
        VOID*   pPrivateCallbackData)
    {
        CHIDEACTIVATEPIPELINEMODE mode     = CHIDeactivateModeUnlinkPipeline;
        Session*                  pSession = static_cast<Session*>(pPrivateCallbackData);
        CamxResult                result   = CamxResultSuccess;

        if (NULL != pSession)
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "session: %p, pipeline handle:%p", pSession, pClientHandle);
            result = pSession->StreamOff(static_cast<CHIPIPELINEHANDLE>(pClientHandle), mode);
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UsingResourceManager
    ///
    /// @brief  Check if resource manager is enabled for the given pipeline index
    ///
    /// @param  pipelineIdx   Pipeline index in the session
    ///
    /// @return TRUE if need to use resource manager, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL UsingResourceManager(
        UINT32 pipelineIdx)
    {
        BOOL useResourceManager = FALSE;

        if (MaxPipelinesPerSession > pipelineIdx)
        {
            useResourceManager = m_useResourceManager[pipelineIdx];
        }

        return useResourceManager;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateCurrExpTimeUseBySensor
    ///
    /// @brief  Pipleline update about currentExposureTime Use by Sensor
    ///
    /// @param  currExposureTimeUseBySensor containing the exposureTime value currently been used by sensor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateCurrExpTimeUseBySensor(
        UINT64 currExposureTimeUseBySensor);


protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Session
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Session() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Session
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~Session();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize the object after creation
    ///
    /// @param  pSessionCreateData   Create data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        SessionCreateData* pSessionCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessResults
    ///
    /// @brief  Called by the derived node when it wants to process the results
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessResults();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequest
    ///
    /// @brief  Called by the derived node when it wants to process a request from the queue
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessRequest();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitTillAllResultsAvailable
    ///
    /// @brief  Wait till all results are available
    ///
    /// @param  overrideTime time to wait
    ///
    /// @return CamxResultSuccess if all the results came back, CamxResultETimeout if we timed out before that.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WaitTillAllResultsAvailable(
        UINT overrideTime = 0);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareAndDispatchRequestError
    ///
    /// @brief  Called to Flush all HAL Requests
    ///
    /// @param  pSessionRequests Session Request structure
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareAndDispatchRequestError(
        SessionCaptureRequest*  pSessionRequests);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BuildSessionName
    ///
    /// @brief  Concatenate the pipeline names to create a "name" for the session
    ///
    /// @param  pCreateData  session creation data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID BuildSessionName(
       SessionCreateData* pCreateData);

    BOOL                    m_aCheckResults;                                ///< Results available indicator
    JobHandle               m_hJobFamilyHandle;                             ///< Job handle for HAL3 Worker
    ThreadManager*          m_pThreadManager;                               ///< Thread Manager
    UINT                    m_numPipelines;                                 ///< Number of pipelines in the Session
    SessionPerPipelineData  m_pipelineData[MaxPipelinesPerSession];         ///< Pipeline data
    UINT                    m_numRealtimePipelines;                         ///< Number of realtime pipelines in Session
    UINT                    m_numMetadataResults;                           ///< Max number of metadata results. More than 1
                                                                            ///  means partial metadata is enabled
    ChiCallBacks            m_chiCallBacks;                                 ///< Callbacks made into the app by the driver
    VOID*                   m_pPrivateCbData;                               ///< Private Cb data

private:

    Session(const Session&) = delete;                               ///< Disallow the copy constructor
    Session& operator=(const Session&) = delete;                    ///< Disallow assignment operator

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SyncProcessCaptureRequest
    ///
    /// @brief  Syncs the process capture request by setting the sync flag and setting AE lock range
    ///
    /// @param  pCaptureRequests   The metadata and input/output buffers to use for the capture request.
    /// @param  pPipelineIndexes   The array of pipeline indexes
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SyncProcessCaptureRequest(
         const ChiPipelineRequest* pCaptureRequests,
         UINT32*                   pPipelineIndexes);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMultiCameraSync
    ///
    /// @brief  This will set the Sync Flag for capture request
    ///
    /// @param  pCaptureRequest    The metadata and input/output buffers to use for the capture request.
    /// @param  enable             To enable the Sync flag
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetMultiCameraSync(
        ChiCaptureRequest* pCaptureRequest,
        BOOL               enable);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAELockRange
    ///
    /// @brief  Sets AE lock range from a start request ID to end Request ID
    ///
    /// @param  pipelineIndex    Pipeline index on which we need to set the AE lock range
    /// @param  startRequestID   Start request ID for which we want to set AE lock
    /// @param  stopRequestID    Stop request ID for which we want to set AE lock
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetAELockRange(
        UINT    pipelineIndex,
        UINT64  startRequestID,
        UINT64  stopRequestID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSyncStreamStatus
    ///
    /// @brief  Sets the Sync Stream Status
    ///
    /// @param  pipelineIndex    Pipeline index on which we need to set the sync stream status
    /// @param  status           Status to be set.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetSyncStreamStatus(
        UINT            pipelineIndex,
        StreamStatus    status)
    {
        if (status != m_sessionSync.streamStatus[pipelineIndex])
        {
            CAMX_LOG_CONFIG(CamxLogGroupCore, "SyncStreamStatus for PipelineIndex:%d changed from %d -> %d",
                pipelineIndex,
                m_sessionSync.streamStatus[pipelineIndex],
                status);
            m_sessionSync.streamStatus[pipelineIndex] = status;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSyncStreamStatus
    ///
    /// @brief  Gets the Sync Stream Status for the pipeline index
    ///
    /// @param  pipelineIndex    Pipeline index on which we need to get the sync stream status
    ///
    /// @return StreamStatus
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE StreamStatus GetSyncStreamStatus(
        UINT pipelineIndex)
    {
        return m_sessionSync.streamStatus[pipelineIndex];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NodeThreadJobFamilySessionCb
    ///
    /// @brief  Job callback function
    ///
    /// @param  pCdata Callback Data
    ///
    /// @return NULL on pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* NodeThreadJobFamilySessionCb(
        VOID* pCdata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeNewPipelines
    ///
    /// @brief  Initialize the new pipelines
    ///
    /// @param  pSessionCreateData   Create data containing new pipeline info
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeNewPipelines(
        SessionCreateData* pSessionCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FinalizePipeline
    ///
    /// @brief  Finalize the new pipelines
    ///
    /// @param  pSessionCreateData   Create data containing new pipeline info
    /// @param  pipelineIndex        Pipeline index
    /// @param  enableQTimer         Qtimer flag
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FinalizePipeline(
        SessionCreateData* pSessionCreateData,
        UINT32             pipelineIndex,
        BIT                enableQTimer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumInputSensors
    ///
    /// @brief  Get input sensor numbers of this session
    ///
    /// @param  pSessionCreateData   Create data containing new pipeline info
    ///
    /// @return numbers of input sensor of this session
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetNumInputSensors(
        SessionCreateData* pSessionCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorSyncMode
    ///
    /// @brief  Get multi camera sensor hardware sync mode from static metadata
    ///
    /// @param  pPipelineDescriptor   pipeline descriptor
    ///
    /// @return sensor sync mode
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SensorSyncMode GetSensorSyncMode(
        PipelineDescriptor* pPipelineDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsUsecaseBatchingEnabled
    ///
    /// @brief  For the selected usecase is batching enabled
    ///
    /// @return TRUE if usecase batching is enabled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsUsecaseBatchingEnabled() const
    {
        return ((1 < m_usecaseNumBatchedFrames) ? TRUE : FALSE);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InjectResult
    ///
    /// @brief  Dispatch a set of HAL3 results to the Android framework
    ///
    /// @param  type                Type of the result
    /// @param  pPayload            Payload of the result
    /// @param  frameworkFrameNum   frameworkFrameNum of the result
    /// @param  pPrivData           private data for result
    /// @param  pipelineIndex       Pipeline index
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InjectResult(
        ResultType  type,
        VOID*       pPayload,
        UINT32      frameworkFrameNum,
        VOID*       pPrivData,
        INT32       pipelineIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNotifyMessageDescriptor
    ///
    /// @brief  Gets a notify message structure
    ///
    /// @return Pointer to notify message structure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ChiMessageDescriptor* GetNotifyMessageDescriptor()
    {
        /// @todo (CAMX-562) Handle being called from multiple contexts if need be

        ChiMessageDescriptor* pNotifyMessage;

        m_notifyMessages.pLock->Lock();

        pNotifyMessage = &m_notifyMessages.pNotifyMessage[m_notifyMessages.freeIndex];

        m_notifyMessages.freeIndex = ((m_notifyMessages.freeIndex + 1) % m_notificationListSize);

        m_notifyMessages.pLock->Unlock();

        return pNotifyMessage;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPartialMetadataMessageDescriptor
    ///
    /// @brief  Gets a PartialMetadata message structure
    ///
    /// @return Pointer to partialmetadata message structure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ChiPartialCaptureResult* GetPartialMetadataMessageDescriptor()
    {

        ChiPartialCaptureResult* pPartialCaptureResultMessage;

        m_PartialDataMessages.pLock->Lock();

        pPartialCaptureResultMessage     = &m_PartialDataMessages.pPartialCaptureMessage[m_PartialDataMessages.freeIndex];

        m_PartialDataMessages.freeIndex  = ((m_PartialDataMessages.freeIndex + 1) % m_partialMetadataListSize);

        m_PartialDataMessages.pLock->Unlock();

        return pPartialCaptureResultMessage;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleErrorCb
    ///
    /// @brief  Handle Error Callback from the Topology
    ///
    /// @param  pError        Error callback payload
    /// @param  pipelineIndex Index of the pipeline which the output buffer corresponds to
    /// @param  pPrivData     Private data for result callback
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleErrorCb(
        CbPayloadError* pError,
        UINT            pipelineIndex,
        VOID*           pPrivData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleAsyncCb
    ///
    /// @brief  Handle Async callback from Topology
    ///
    /// @param  pAsync    Async callback payload
    /// @param  pPrivData Private data for result callback
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleAsyncCb(
        CbPayloadAsync* pAsync,
        VOID*           pPrivData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleSOFCb
    ///
    /// @brief  Handle SOF event callback from CSL
    ///
    /// @param  pAsync Async callback payload
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleSOFCb(
        CbPayloadSof* pAsync);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleMetaBufferDoneCb
    ///
    /// @brief  Handle Metabuffer done event callback from pipeline
    ///
    /// @param  pMetaBufferCbPayload  Array of metabuffers which are completed by the framework
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleMetaBufferDoneCb(
        CbPayloadMetaBufferDone* pMetaBufferCbPayload);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleMetadataCb
    ///
    /// @brief  Handle metadata callback from Topology
    ///
    /// @param  pMetadata     Metadata callback payload
    /// @param  pPrivData     Private data for result callback
    /// @param  pipelineIndex Index of the pipeline
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleMetadataCb(
        CbPayloadMetadata* pMetadata,
        VOID*              pPrivData,
        UINT32             pipelineIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandlePartialMetadataCb
    ///
    /// @brief  Handle Partial metadata callback from Pipeline
    ///
    /// @param  pMetadata     Metadata callback payload
    /// @param  pPrivData     Private data for result callback
    /// @param  pipelineIndex Index of the pipeline
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandlePartialMetadataCb(
        CbPayloadPartialMetadata* pMetadata,
        VOID*                     pPrivData,
        UINT32                    pipelineIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleEarlyMetadataCb
    ///
    /// @brief  Handle early metadata callback from Topology
    ///
    /// @param  pMetadata     Metadata callback payload
    /// @param  pipelineIndex Index of the pipeline which the output buffer corresponds to
    /// @param  pPrivData     Private data for result callback
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleEarlyMetadataCb(
        CbPayloadMetadata* pMetadata,
        UINT               pipelineIndex,
        VOID*              pPrivData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleBufferCb
    ///
    /// @brief  Handle Buffer callback from Topology
    ///
    /// @param  pPayload      Buffer callback payload
    /// @param  pipelineIndex Index of the pipeline which the output buffer corresponds to
    /// @param  pPrivData     Private data for result callback
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleBufferCb(
        CbPayloadBuffer* pPayload,
        UINT             pipelineIndex,
        VOID*            pPrivData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetResultHolderBySequenceId
    ///
    /// @brief  Return a result holder corresponding to a specific sequence id
    ///
    /// @param  sequenceId  Specific id for which the result holder is sought
    ///
    /// @return Pointer to ResultHolder if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ResultHolder* GetResultHolderBySequenceId(
        UINT32 sequenceId)
    {
        LightweightDoublyLinkedListNode* pNode                = NULL;
        SessionResultHolder*             pSessionResultHolder = NULL;
        ResultHolder*                    pHolder              = NULL;
        BOOL                             bFound               = FALSE;

        pNode = m_resultHolderList.Head();
        while (NULL != pNode)
        {
            CAMX_ASSERT(NULL != pNode->pData);
            if (NULL != pNode->pData)
            {
                pSessionResultHolder = reinterpret_cast<SessionResultHolder*>(pNode->pData);

                for (UINT32 i = 0 ; i < pSessionResultHolder->numResults; i++)
                {
                    pHolder = &pSessionResultHolder->resultHolders[i];
                    if (pHolder->sequenceId == sequenceId)
                    {
                        // Found the result holder at the sequence id we were looking for
                        // so break out of the while loop
                        bFound = TRUE;
                        break;
                    }

                }
            }
            if (TRUE == bFound)
            {
                break;
            }
            else
            {
                pHolder = NULL;
            }
            pNode = m_resultHolderList.NextNode(pNode);
        }

        return pHolder;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFrameworkFrameNumber
    ///
    /// @brief  Return the framework's frameNumber based on sequenceId
    ///
    /// @param  sequenceId   Sequence ID for which the result holder is sought
    ///
    /// @return Framework's frameNumber
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetFrameworkFrameNumber(
        UINT32 sequenceId)
    {
        return static_cast<UINT32>(m_fwFrameNumberMap[sequenceId % MaxQueueDepth]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessResultEarlyMetadata
    ///
    /// @brief  Processes metadata result for a given result
    ///
    /// @param  pResultHolder Sequence ID for which the result holder is sought
    /// @param  pNumResults   Current number of results
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL ProcessResultEarlyMetadata(
        ResultHolder* pResultHolder,
        UINT*         pNumResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessResultMetadata
    ///
    /// @brief  Processes metadata result for a given result
    ///
    /// @param  pResultHolder Sequence ID for which the result holder is sought
    /// @param  pNumResults   Current number of results
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL ProcessResultMetadata(
        ResultHolder* pResultHolder,
        UINT*         pNumResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessResultBuffers
    ///
    /// @brief  Processes metadata result for a given result
    ///
    /// @param  pResultHolder     Sequence ID for which the result holder is sought
    /// @param  metadataAvailable Meta available
    /// @param  pNumResults       Current number of results
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL ProcessResultBuffers(
        ResultHolder* pResultHolder,
        BOOL          metadataAvailable,
        UINT*         pNumResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStreamIndex
    ///
    /// @brief  Return the index corresponding to the output stream passed in
    ///
    /// @param  pStream Stream for which the index is required
    ///
    /// @return The index corresponding to the output stream passed in
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 GetStreamIndex(
        ChiStream* pStream
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushThreadJobCallback
    ///
    /// @brief  Flush thread job family
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID FlushThreadJobCallback();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnregisterThreadJobCallback
    ///
    /// @brief  Unregister thread job family
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID UnregisterThreadJobCallback();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DispatchResults
    ///
    /// @brief  Dispatch a set of HAL3 results to the Android framework
    ///
    /// @param  pCaptureResults      The set of results to be dispatched
    /// @param  numCaptureResults    The number of results in the set
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult DispatchResults(
        ChiCaptureResult* pCaptureResults,
        UINT32            numCaptureResults) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DispatchNotify
    ///
    /// @brief  Dispatch a HAL3 notify message to the Android framework
    ///
    /// @param  pNotifyMessage The message to notify
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult DispatchNotify(
        ChiMessageDescriptor* pNotifyMessage) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DispatchPartialMetadata
    ///
    /// @brief  Dispatch partial Metadata to CHI
    ///
    /// @param  pPartialCaptureResult The Partial Metadata
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult DispatchPartialMetadata(
        ChiPartialCaptureResult* pPartialCaptureResult)=0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearAllPendingItems
    ///
    /// @brief  Clear pending status of all metadata and output buffer streams for a result
    ///
    /// @param  pHolder The holder of the specific result
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ClearAllPendingItems(
        ResultHolder* pHolder
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MeetFrameworkNotifyCriteria
    ///
    /// @brief  Decide if a result component callback warrants triggering the HAL3 Worker to process results
    ///
    /// @param  pHolder Holder for the specific result
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL MeetFrameworkNotifyCriteria(
        ResultHolder* pHolder);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetResultStreamBuffer
    ///
    /// @brief  Gets a result stream buffer
    ///
    /// @return Pointer to result stream buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ChiStreamBuffer* GetResultStreamBuffer()
    {
        ChiStreamBuffer* pStreamBuffer;
        m_resultStreamBuffers.pLock->Lock();
        pStreamBuffer                   = &m_resultStreamBuffers.resultStreamBuffer[m_resultStreamBuffers.freeIndex];
        m_resultStreamBuffers.freeIndex = ((m_resultStreamBuffers.freeIndex + 1) % OutputStreamBufferListSize);
        m_resultStreamBuffers.pLock->Unlock();
        return pStreamBuffer;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MetadataReadyToFly
    ///
    /// @brief  Check if a Metadata result component can be notified to the framework
    ///
    /// @param  frameworkFrameNum  Frame number to which this metadata belongs
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL MetadataReadyToFly(
        UINT32 frameworkFrameNum);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BufferReadyToFly
    ///
    /// @brief  Check if a Buffer result component can be notified to the framework
    ///
    /// @param  frameworkFrameNum   RequestId to which this buffer belongs
    /// @param  pStream             Corresponding output stream of this buffer
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL BufferReadyToFly(
        UINT32     frameworkFrameNum,
        ChiStream* pStream);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AdvanceMinExpectedResult
    ///
    /// @brief  Check and advance the minimum frame number for which a result is expected
    ///
    /// @note   m_pResultLock must be acquired before entry to this function
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AdvanceMinExpectedResult();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PollPipelineFlushStatus
    ///
    /// @brief  Check if pipelines flushing are done if so signal
    ///
    /// @note   m_pResultLock must be acquired before entry to this function
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PollPipelineFlushStatus();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPerStreamColorMetadata
    ///
    /// @brief  Set color metadata for stream buffer
    ///
    /// @param  pRequest   The metadata and input/output buffers to use for the capture request.
    /// @param  pInputPool The Input MetadataPool it need get parameters
    /// @param  requestId  Request ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetPerStreamColorMetadata(
        const ChiCaptureRequest*   pRequest,
        MetadataPool*              pInputPool,
        UINT64                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryStreamHDRMode
    ///
    /// @brief  Query the HDR mode for the stream
    ///
    /// @param  pStream    The HAL stream to query from
    /// @param  pInputPool The Input MetadataPool it need get parameters
    /// @param  requestId  Request ID for the current query
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult QueryStreamHDRMode(
        HAL3Stream*                pStream,
        MetadataPool*              pInputPool,
        UINT64                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CSLSessionMessageHandler
    ///
    /// @brief  This method is a CSL message handler which receives Error events from CSL for Offline devices
    ///
    /// @param  pUserData Pointer to userdata(class object where message handler is registered) sent to the CSL
    /// @param  pMessage  Pointer to CSL frame message
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CSLSessionMessageHandler(
        VOID*       pUserData,
        CSLMessage* pMessage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsTorchWidgetSession
    ///
    /// @brief  Query whether this is Torch Widget session
    ///
    /// @return TRUE if node is Torch widget session.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsTorchWidgetSession()
    {
        return m_isTorchWidgetSession;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPerFrameVTTimestampMetadata
    ///
    /// @brief  This function sets VT timestamp in native buffer
    ///
    /// @param  phBufferHandle native buffer handle
    /// @param  pPool          pointer to metadata pool
    /// @param  requestId      request ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPerFrameVTTimestampMetadata(
        const NativeHandle* phBufferHandle,
        MetadataPool*       pPool,
        UINT64              requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPerFrameVideoPerfModeMetadata
    ///
    /// @brief  This function sets Video perf mode in native buffer
    ///
    /// @param  phBufferHandle native buffer handle
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPerFrameVideoPerfModeMetadata(
        const NativeHandle* phBufferHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPerFrameCVPMetadata
    ///
    /// @brief  This function sets CVP metadata in native buffer
    ///
    /// @param  phBufferHandle native buffer handle
    /// @param  pipelineIndex  pipeline index
    /// @param  sequenceId     sequence Id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPerFrameCVPMetadata(
        const NativeHandle* phBufferHandle,
        UINT32              pipelineIndex,
        UINT64              sequenceId);

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
    /// isPreviewStream
    ///
    /// @brief  Check if the stream is preview stream by looking at gralloc usage
    ///
    /// @param  usage gralloc usage of the stream
    ///
    /// @return TRUE if it is preview stream
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL isPreviewStream(
        GrallocUsage64 usage)
    {
        BOOL returnVal = FALSE;
        if (0 != (GrallocUsageHwComposer & usage))
        {
            returnVal = TRUE;
        }

        return returnVal;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpResultState
    ///
    /// @brief  Dumps snapshot of current state to a file
    ///
    /// @param  fd      file descriptor
    /// @param  indent  indent spaces.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpResultState(
        INT     fd,
        UINT32  indent);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetMetabufferPendingQueue
    ///
    /// @brief  Resets the pending metabuffer queue
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ResetMetabufferPendingQueue();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMetabufferPendingQueue
    ///
    /// @brief  Process the pending metabuffer queue
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessMetabufferPendingQueue();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpDebugData
    ///
    /// @brief  Dumps Debug logs
    ///
    /// @param  flag    indicating why the session was dumped.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpDebugData(
        SessionDumpFlag flag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpSessionState
    ///
    /// @brief  Dumps Debug logs
    ///
    /// @param  flag  Indicates what type of flush we are doing
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpSessionState(
        SessionDumpFlag flag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpKMDInfo
    ///
    /// @brief  Dumps KMD info
    ///
    /// @param  flag  Indicates what type of flush we are doing
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpKMDInfo(
        SessionDumpFlag flag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetRealtimePipeline
    ///
    /// @brief  Check if session has any realtime pipeline
    ///
    /// @param  pCreateData  session creation data
    ///
    /// @return True if realtime False if not
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL SetRealtimePipeline(
        SessionCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckActiveSensor
    ///
    /// @brief  Check if session has any realtime pipeline
    ///
    /// @param  pCreateData  session creation data
    ///
    /// @return True if realtime False if not
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckActiveSensor(
        SessionCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetActiveCSLSession
    ///
    /// @brief  Check if session has any realtime pipeline
    ///
    /// @param  pCreateData  session creation data
    /// @param  phCSLSession Pointer to CSL Session
    ///
    /// @return Success if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetActiveCSLSession(
        SessionCreateData* pCreateData,
        CSLHandle*         phCSLSession);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearPendingMBDoneQueue
    ///
    /// @brief  Clear the pending buffer queue
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID ClearPendingMBDoneQueue()
    {
        m_pPendingMBQueueLock->Lock();
        m_pendingMetabufferDoneQueue.latestSequenceId = 0;
        m_pendingMetabufferDoneQueue.oldestSequenceId = UINT32_MAX;
        m_pPendingMBQueueLock->Unlock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushResponseTime
    ///
    /// @brief  Get flush response time for the pending requests
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 GetFlushResponseTime();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDoneProcessing
    ///
    /// @brief  Checks if all the pipelines have finished processing their requests and that the result holder is empty.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsDoneProcessing();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllPipelinesFlushed
    ///
    /// @brief  Returns TRUE if all the Pipelines that are specified by the Session Flush Info have been Flushed
    ///
    /// @return BOOL flag indicating if all pipelies in m_sessionFlushInfo are flushed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AllPipelinesFlushed();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
     /// CalculateResultFPS
    ///
    /// @brief  Calculate the FPS of the result buffer. The FPS is calculated after 10 sec.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CalculateResultFPS();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndSyncLinks
    ///
    /// @brief  Check if the realtime pipeline links in the session need to be synced.
    ///         Sync the links if all required conditions met.
    ///
    /// @return Success if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckAndSyncLinks();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateSessionRequestTimingBuffer
    ///
    /// @brief  Populates session buffer that contains information about how long a request takes to process
    ///
    /// @param  pRequest    Session capture request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PopulateSessionRequestTimingBuffer(
        CaptureRequest* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateSessionRequestTimingBuffer
    ///
    /// @brief  Updates session buffer that contains information about how long a request takes to process
    ///
    /// @param  pResultHolder   Session result holder
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateSessionRequestTimingBuffer(
        ResultHolder* pResultHolder);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpSessionRequestProcessingTime
    ///
    /// @brief  Dumps last MaxSessionPerRequestInfo requests and their start/end/avg/min/max processing times
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpSessionRequestProcessingTime();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSessionRequestData
    ///
    /// @brief  Gets the Session Request Data struct instance for a given requestId
    ///
    /// @param  requestId     requestId
    /// @param  updateIndex   Used to indicate that we are done with a given space in the buffer
    ///
    /// @return SessionPerRequestInfo
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE PerResultHolderInfo* GetSessionRequestData(
        UINT64  requestId,
        BOOL    updateIndex)
    {
        // Use temp result holder for start time and final result holder for end time; this way we only put in
        // all the data in the final result holder that we dump at once to avoid printing data from half-completed requests
        PerResultHolderInfo*    pSessionRequestTemp;
        PerResultHolderInfo*    pSessionRequestFinal;

        UINT32 requestIdIndex   = requestId % MaxSessionPerRequestInfo;

        pSessionRequestTemp     = &m_tempPerRequestInfo.requestInfo[requestIdIndex];
        pSessionRequestFinal    = &m_perRequestInfo.requestInfo[requestIdIndex];

        // Keep track of lastest index used in the buffer
        if (TRUE == updateIndex)
        {
            // Time to return final buffer, populate final buffer with temp buffer's info
            m_perRequestInfo.lastSessionRequestIndex    = requestIdIndex;
            pSessionRequestFinal->startTime             = pSessionRequestTemp->startTime;
            pSessionRequestFinal->requestId             = pSessionRequestTemp->requestId;
            pSessionRequestFinal->CSLSyncId             = pSessionRequestTemp->CSLSyncId;

            return pSessionRequestFinal;
        }
        else
        {
            return pSessionRequestTemp;
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyPipelinesOfTriggeringRecovery
    ///
    /// @brief  Notifies the session recovery status for all pipelines
    ///
    /// @param  isRecovery Recovery status
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyPipelinesOfTriggeringRecovery(
        BOOL isRecovery);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSessionTriggeringRecovery
    ///
    /// @brief  Sets the session recovery status
    ///
    /// @param  isRecovery Recovery status
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetSessionTriggeringRecovery(
        BOOL isRecovery)
    {
        m_triggeringRecovery = isRecovery;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSessionTriggeringRecovery
    ///
    /// @brief  Get the session recovery status
    ///
    /// @return TRUE if the session is in recovery, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL GetSessionTriggeringRecovery()
    {
        return m_triggeringRecovery;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FallbackFlush
    ///
    /// @brief  Give nodes more time to process and call flush on nodes directly if there is a time out.
    ///
    /// @param  isSessionFlush  boolean flag indicating Session flush
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FallbackFlush(
        BOOL isSessionFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearResultHolderList
    ///
    /// @brief  empties the result holder list.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ClearResultHolderList();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushHALQueue
    ///
    /// @brief  Deques elements from the hal queue and dispatches an error request for them.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FlushHALQueue();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateDebugDataPool
    ///
    /// @brief  Allocate memory for all debug-data memory use in the session.
    ///
    /// @param  ppDebugDataBuffer   Return allocated memory pointer
    /// @param  numSlots            Memory allocated should be enough for this number of slots
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocateDebugDataPool(
        VOID**  ppDebugDataBuffer,
        UINT    numSlots);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDebugDataForSlot
    ///
    /// @brief  Get next debug-data available for a debug-data slot
    ///
    /// @param  ppSlotDebugData Pointer to next debug-data memory available for an slot.
    ///
    /// @return CamxResultSuccess if no error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDebugDataForSlot(
        VOID** ppSlotDebugData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UseInternalDebugDataMemory
    ///
    /// @brief  Inform the intent or the use of debug-data allocated by the session
    ///
    /// @return TRUE if using debug-data from session
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL UseInternalDebugDataMemory()
    {
        BOOL                    useInternalMemory   = FALSE;
        const StaticSettings*   pStaticSettings     = HwEnvironment::GetInstance()->GetStaticSettings();

        if ((TRUE == HAL3MetadataUtil::IsDebugDataEnable()) &&
            ((TRUE == m_isRealTime) ||
            (TRUE == pStaticSettings->debugDataOfflineAlloc)))
        {
            useInternalMemory = TRUE;
        }

        return useInternalMemory;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitDebugDataSlot
    ///
    /// @brief  Initialize debug-data slot blob. Initialize all pointers to point to debug-data memory.
    ///
    /// @param  pBlob           Slot blob to be initialize.
    /// @param  pDebugDataBase  Pointer to debug-data to be use to initialize the slot blob
    ///
    /// @return CamxResultSuccess if no error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitDebugDataSlot(
        VOID*   pBlob,
        VOID*   pDebugDataBase);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareAndDispatchChiRequestError
    ///
    /// @brief  Prepare and dispatch request error for the pipeline requests
    ///
    /// @param  pPipelineRequests Pointer to the Chi pipeline requests.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareAndDispatchChiRequestError(
        const ChiPipelineRequest* pPipelineRequests);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareChiRequestErrorForInflightRequests
    ///
    /// @brief  Prepare request error for the pipeline requests that are not enqueued, blokced during flush
    ///
    /// @param  pPipelineRequests Pointer to the Chi pipeline requests.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareChiRequestErrorForInflightRequests(
        const ChiPipelineRequest* pPipelineRequests);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DispatchResultsForInflightRequests
    ///
    /// @brief  Dispatch request error for the inflight pipeline requests not enqueued, blocked during flush
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DispatchResultsForInflightRequests();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PipelinesInflightRequestsNotification
    ///
    /// @brief  Decrement pipeline live pending request count and wakeup streamon waiting thread
    ///
    /// @param  pPipelineRequests Pointer to the Chi pipeline requests.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PipelinesInflightRequestsNotification(
        const ChiPipelineRequest* pPipelineRequests);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsAnyPendingRequest
    ///
    /// @brief  Is any live pending request for the session
    ///
    /// @return TRUE if there is any live pending request otherwise return FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsAnyPendingRequest()
    {
        BOOL livePendingRequests;

        m_pLivePendingRequestsLock->Lock();
        livePendingRequests = (0 < m_livePendingRequests) ? TRUE: FALSE;
        m_pLivePendingRequestsLock->Unlock();

        return livePendingRequests;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PreFlushLockOperations
    ///
    /// @brief  Flush the session. This call is blocking.
    ///
    /// @param  pPipelineIndexes     Pipeline index array
    /// @param  numPipelinesToFlush  Number of entries in array
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PreFlushLockOperations(
        UINT32* pPipelineIndexes,
        UINT32  numPipelinesToFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostFlushLockOperations
    ///
    /// @brief  Flush the session. This call is blocking.
    ///
    /// @param  pPipelineIndexes     Pipeline index array
    /// @param  numPipelinesToFlush  Number of entries in array
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PostFlushLockOperations(
        UINT32* pPipelineIndexes,
        UINT32  numPipelinesToFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PipelineFlush
    ///
    /// @brief  Flush the pipeline. This call is blocking.
    ///
    /// @param  pSessionFlushInfo Flush info param from Chi
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PipelineFlush(
        const CHISESSIONFLUSHINFO* pSessionFlushInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SessionFlush
    ///
    /// @brief  Flush the session. This call is blocking.
    ///
    /// @param  pSessionFlushInfo Flush info param from Chi
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SessionFlush(
        const CHISESSIONFLUSHINFO* pSessionFlushInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPipelineFlush
    ///
    /// @brief  Check to see if we are pipeline flushing
    ///
    /// @return TRUE if we are pipeline flushing
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPipelineFlush()
    {
        return CamxAtomicLoad32(&m_isPipelineFlush);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineFlushStatus
    ///
    /// @brief  Get the Pipeline descriptor pointer
    ///
    /// @param  pipelineIndex Pipeline Descriptor to which the request is issued
    ///
    /// @return Boolean status indicating Flush Status for the given pipelineIndex
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL GetPipelineFlushStatus(
        UINT32 pipelineIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyPipelineProcessingDone
    ///
    /// @brief  Notify the session that all the results for pipelines being flushed were done, if both conditions
    ///         are true, we can proceed with flush.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyPipelineProcessingDone();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitTillFlushResultsAvailable
    ///
    /// @brief  Wait till all results are available
    ///
    /// @return CamxResultSuccess if all the results came back, CamxResultETimeout if we timed out before that.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WaitTillFlushResultsAvailable();

    UINT                   m_requestQueueDepth;                        ///< RequestQueueDepth - for batched mode its
                                                                       ///  RequestQueueDepth * m_usecaseNumBatchedFrames
    ChiContext*            m_pChiContext;                              ///< Context pointer
    HAL3Queue*             m_pRequestQueue;                            ///< Input Request queue

    Mutex*                 m_pResultLock;                              ///< Used to serialize result processing
    Mutex*                 m_pWaitForResultsLock;                      ///< Used to serialize result processing
    Mutex*                 m_pRequestLock;                             ///< Used to serialize request processing

    Mutex*                 m_pStreamOnOffLock;                         ///< Used to halt requests until stream on/off complete
    Mutex*                 m_pResultHolderListLock;                    ///< Used to serialize the access to m_resultHolderList
                                                                       ///  This lock might be concurrently used with
                                                                       ///  m_pResultLock. In that case, m_pResultHolderListLock
                                                                       ///  should be locked after m_pResultLock and  should
                                                                       ///  not be interleaved with m_pResultLock.
    volatile UINT8         m_aFlushingPipeline;                        ///< There is a wait active on draining all requests and
                                                                       ///  results.  Only freely accessed when holding
                                                                       ///  m_pFlushLock
    volatile UINT8         m_aFlushingSession;                         ///< There is a wait active on draining all requests and
                                                                       ///  results.
    BOOL                   m_inputDefaultsInitialized;                 ///< Have defaults been setup for input pool
    MetadataPool**         m_ppPerFrameDebugDataPool;                  ///< Debug-data request data pool

    VOID*                  m_pDebugDataBuffer;                         ///< Data store, debug data for all slots.
                                                                       ///  numSlots x debug-data  size. Debug-data size
                                                                       ///  is obtain from properties.
    UINT                   m_debugDataSlots;                           ///< Number of slots available for debug-data pool
    INT                    m_debugDataBufferIndex;                     ///< Index to track debug-data buffer usage, keeps
                                                                       ///  the latest assined memory. It circulate though
                                                                       ///  the buffer.
    SessionCaptureRequest  m_captureRequest;                           ///< Capture request (that may or may not be batched)
    StreamBufferInfo*      m_pStreamBufferInfo;                        ///< Streambuffer info that'll be used by
                                                                       ///  m_captureRequest
    UINT                   m_batchedFrameIndex[MaxPipelinesPerSession];///< Running batch index
    BOOL                   m_isRequestBatchingOn;                      ///< Usecase selects whether batching *CAN* be enabled
                                                                       ///  or not in general. And we have to check per-request
                                                                       ///  to see if batching is ON or not. If batching
                                                                       ///  is ON in a request it remains ON for
                                                                       ///  m_usecaseNumBatchedFrames requests
    UINT64                 m_currentFrameCount;                        ///< Current frame count used for FPS calc
    UINT64                 m_lastFPSCountTime;                         ///< Used to calculate elapsed time
    Condition*             m_pWaitAllResultsAvailable;                 ///< Wait till all results are available
    DeferredRequestQueue*  m_pDeferredRequestQueue;                    ///< Pointer to the deferred process handler
    UINT                   m_usecaseNumBatchedFrames;                  ///< Number of framework frames batched together if
                                                                       ///  batching is enabled
    BOOL                   m_HALOutputBufferCombined;                  ///< Is the HAL output buffer combined for batch mode
    Condition*             m_pWaitLivePendingRequests;                 ///< Wait if the number of live pending requests
                                                                       ///  reaches pipeline delay
    Mutex*                 m_pLivePendingRequestsLock;                 ///< Used to block until ready to accept next
                                                                       ///  request from framework
    UINT                   m_livePendingRequests;                      ///< The number of live pending requests
    UINT                   m_maxLivePendingRequests;                   ///< The number of the maximum live pending requests
    UINT                   m_defaultMaxLivePendingRequests;            ///< The number of the default maximum live pending
                                                                       ///  requests for non-batch mode.
    UINT                   m_notificationListSize;                     ///< Notification list size corresponding to the
                                                                       ///  particular session
    UINT                   m_partialMetadataListSize;                  ///< PartialMetaData list size correspondiong to
                                                                       ///  the particular session

    BOOL                   m_isIPERealtime;                            ///< is IPE in RealTime

    LightweightDoublyLinkedList m_resultHolderList;                    ///< Result Holder list

    ChiCaptureResult*      m_pCaptureResult;                           ///< Final results to send to the Android framework
    ResultStreamBuffers    m_resultStreamBuffers;                      ///< Result stream buffers
    NotifyMessages         m_notifyMessages;                           ///< Notify messages
    PartialdataMessages    m_PartialDataMessages;                      ///< Partial data messages

    /// @todo (CAMX-1797) Need to rethink about the correct place to do this
    CmdBufferManager*     m_pDebugBufferManager;                       ///< Debug buffers
    CmdBufferManager*     m_pRegDumpDebugBufferManager;                ///< Register dump debug buffers
    BOOL                  m_aDeviceInError;                            ///< HAL3Device is in an error state
    MultiRequestSyncData  m_requestSyncData[MaxQueueDepth];    ///< Multi pipeline request sync information
    UINT64                m_syncSequenceId;                            ///< Multi request sequence id for sync
    UINT64                m_requestBatchId[MaxPipelinesPerSession];    ///< Running request id, incremented per batch
    UINT32                m_sequenceId;                                ///< Running counter, incremented for every Process* call
    /// @todo (CAMX-2876) The 8 limit is artificial...this should either be dynamically allocated, or create static
    ///                   const with a good max value
    UINT64                m_fwFrameNumberMap[MaxQueueDepth];   ///< Map of session request ID back to framework frame
                                                                       ///< number
    UINT32                m_vendorTagSensorModeIndex;                  ///< Index of the sensor mode vendor tag
    UINT32                m_numInputSensors;                           ///< sensor numbers of session input.
    BOOL                  m_isTorchWidgetSession;                      ///< Is torch widget session
    UINT32                m_vendorTagIndexTimestamp;                   ///< Index of the timestamp vendor tag
    UINT32                m_indexCVPMetaData;                          ///< Index of the timestamp vendor tag
    UINT                  m_qtimerErrIndicated;                        ///< Stop log spew after this counter hits a threshold
    JobHandle             m_hNodeJobHandle;                            ///< Job handle
    UINT32                m_videoStabilizationCaps;                    ///< EIS capabilities
    UINT32                m_recordingEndOfStreamTagId;                 ///< Recording end tag
    UINT32                m_recordingEndOfStreamRequestIdTagId;        ///< Recording end requestIDtag
    UINT32                m_numLinksSynced;                            ///< number of pipeline links that are synced.
    CSLSyncLinkMode       m_linkSyncMode;                              ///< Indicates current realtime pipeline sync mode
    CSLLinkHandle         m_hLinkHandles[MaxSyncLinkCount];            ///< Linked handles
    BOOL                  m_setVideoPerfModeFlag;                      ///< Flag to indicate video perf mode needs to be set
    BOOL                  m_sesssionInitComplete;                      ///< To check if session initialization is complete
    StatsOperationMode    m_statsOperationMode;                        ///< Stats operation mode to indicate whether it is in
                                                                       ///  Fast AEC or in normal mode
    UINT32                m_previewStreamPresentTagId;                 ///< Tag Id to know preview stream present in request

    PendingMetaBufferDoneQueue   m_pendingMetabufferDoneQueue;         ///< Queue to store the metadata results which are
                                                                       ///  completed.

    volatile UINT8        m_aSessionDumpComplete;                      ///< Session Dump is Complete
    Mutex*                m_pSessionDumpLock;                          ///< Used to serialize Dump

    CSLHandle             m_hCSLSession;                               ///< CSL Session handle
    BOOL                  m_isRealTime;                                ///< Flag to indicate whether session
                                                                       ///  has realtime pipeline
    Mutex*                m_pFlushLock;                                ///< Used to halt processing until requests and results
                                                                       ///  have drained
    Mutex*                m_pFlushDoneLock;                            ///< Used to halt external thread from submitting
                                                                       ///  a request while flush call is in progress
    Condition*            m_pWaitForFlushDone;                         ///< Wait for completion of ongoing flush
                                                                       ///  reaches pipeline delay
    const ChiPipelineRequest*   m_pInflightPipelineRequests;           ///< Any inflight pipeline requests that needed to be
                                                                       ///  flushed at the end of flush
    UINT32                m_numInflightRequests;                       ///< Num of inflight requests
    ChiCaptureResult*     m_pCaptureResultForInflightRequests;         ///< Final results to send to the Android framework
    CHISESSIONFLUSHINFO   m_sessionFlushInfo;                          ///< Local copy of the SessionFlushInfo received as
                                                                       ///  part of Flush Call. Do not modify without holding
                                                                       ///  flushLock
    BOOL                  m_pipelineFlushResultsAv[MaxPipelinesPerSession]; ///< Flag indicating if Flushed Results are
                                                                       ///  available for each pipeline
    Condition*            m_pWaitFlushResultsAvailable;                ///< Wait till pipeline flush results are available
    BOOL                  m_waitForPipelineFlushResults;               ///< Flag to indicate waiting for the result is needed
                                                                       ///  for Pipeline Flush scenario
    BOOL                  m_isPipelineFlush;                           ///< Flag to indicate that pipeline flush is occurring
    UINT64                m_lastCompletedRequestId;                    ///< requestId for the last request flushed or completed.
    CHAR                  m_pipelineNames[MaxStringLength256];         ///< The names of pipelines in this session.
    UINT64                m_lastCSLSyncId;                             ///< The Last CSL Sync Id Request
    SessionSync           m_sessionSync;                               ///< Session Sync Controller for Multi Cam
    volatile UINT32       m_aTotalLongExposureTimeout;                 ///< Long exposure timeout
    Mutex*                m_pPendingMBQueueLock;                       ///< Lock to serialize the pending metabuffer queue
    SessionPerRequestInfo m_perRequestInfo;                            ///< Hold final per request info
    SessionPerRequestInfo m_tempPerRequestInfo;                        ///< Hold temporary per request info
    BOOL                  m_triggeringRecovery;                        ///< Is recovery in progress
    UINT                  m_numStreamedOnRealtimePipelines;            ///< Number of streamed on realtime pipelines
    BOOL                  m_useResourceManager[MaxPipelinesPerSession];///< Use Resource Manger or not
    UINT32                m_exposurePriorityModeTagId;                 ///< Exposure Priority mode tag Id
    BOOL                  m_waitforResults;                            ///< Flag to indicate waiting for the result is needed
    BOOL                  m_additionalWaitTimeForLivePending;          ///< Flag to indicate whether additional wait is needed
                                                                       ///  for the initial requests at the start of the session
                                                                       ///  or for the initial requests after flush
    UINT32                m_currExposureTimeUseBySensor;               ///< Exposure Time currently used by sensor

};

CAMX_NAMESPACE_END

#endif // CAMXSESSION_H
