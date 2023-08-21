////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxnode.h
/// @brief Node class declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXNODE_H
#define CAMXNODE_H

#include "camxthreadmanager.h" // doesn't belong here...just a bigger rabbit hole to remove at the moment
#include "camxpipeline.h"

#include "chinode.h"

struct ChiStream;
struct ChiPipelineInputOptions;
struct ChiTuningModeParameter;

CAMX_NAMESPACE_BEGIN

/// Forward Declarations

enum class FenceDropActionReturnType;
enum class Format;

class  ChiContext;
class  ContingencyInducer;
class  HwFactory;
class  ImageBuffer;
class  MetadataPool;
class  Node;
class  Pipeline;
class  TuningDataManager;
class  ImageBufferManager;
class  CmdBuffer;
class  CmdBufferManager;

struct BufferPropertiesInfo;
struct CaptureRequest;
struct ChiFence;
struct FinalizeInitializationData;
struct LinkPropertiesInfo;
struct NodeInfo;
struct OutputPort;
struct PerBatchedFrameInfo;
struct PerNodeInfo;
struct ResourceParams;
struct WatermarkPattern;

static const UINT32 MaxDstPortsPerLink            = 8;
static const UINT32 MaxRequestQueueDepth          = RequestQueueDepth + 2;
static const UINT32 MaxRequestBufferDepth         = 64;
static const UINT32 MaxTuningModeSelectors        = 10;
static const UINT32 BufferCountForRealTimeNodes   = 2;
static const UINT32 BufferCountForIFE             = 4;
static const SIZE_T InvalidImageBuffer            = 0xFFFFFFFF;
static const UINT32 NumProcReqData                = MaxRequestQueueDepth;    ///< Count of old requests to queue
static const UINT32 MaxTagsPublished              = ChiNodeMaxTagsPublished;
static const UINT32 MaxFenceErrorBufferDepth      = 50;
static const UINT32 MaxRTSessionHandles           = 2;
static const INT32  MaxProcessSequenceIdForTiming = 8;
static const UINT32 TimeToPrintSeqIdAverageInMs   = 5000;
static const UINT   InvalidPortIndex              = UINT_MAX;

CAMX_BEGIN_PACKED

// NOWHINE FILE CP006: used standard libraries for performance improvements

struct ConfigIORequest
{
    VOID*   pDeviceResourceParam;  ///< Untyped pointer to the resource specific structure, structure can be in/out.
} CAMX_PACKED;

CAMX_END_PACKED


/// @brief Enumeration of possible internal dependency sequence ids
enum InternalDependencySequenceId
{
    ResolveDeferredInputBuffers = 1, ///< If the parent node is a bypass node,
                                     ///  this sequenceId is used to resolve what input buffers should be used by this node
};

/// @brief Structure for Output buffer for dependency buffer or delayed buffer
struct DelayedOutputBufferInfo
{
    CSLFence        hFence;                         ///< Fence associated with all Imagebuffers in the output
                                                    ///  for delayed buffer
    BOOL            isParentInputBuffer;            ///< is the image buffer a parent's input buffer
    ImageBuffer**   ppImageBuffer;                  ///< image buffer pointers
    UINT32          sequenceId;                     ///< Sequence ID that this buffer is associated with
};

/// @brief A property dependency type
struct PropertyDependency
{
    UINT32      count;                              ///< Number of properties in this unit
    PropertyID  properties[MaxProperties];          ///< Property dependencies in this unit
    UINT64      offsets[MaxProperties];             ///< Offset from current request for dependency
    BOOL        negate[MaxProperties];              ///< Indicate if offsets are negative value
    UINT        pipelineIds[MaxProperties];         ///< Pipeline index for every Property
};

/// @brief A unit of dependency that Node reports back to Pipeline
struct DependencyUnit
{
    union
    {
        struct
        {
            UINT32 hasInputBuffersReadyDependency       :  1;       ///< Node reports buffer dependency
            UINT32 hasPropertyDependency                :  1;       ///< Node reports property dependency
            UINT32 hasFenceDependency                   :  1;       ///< Node reports Chi Fence dependency
            UINT32 isPreemptable                        :  1;       ///< Can dependency be preempted
            UINT32 hasIOBufferAvailabilityDependency    :  1;       ///< Node reports input, output buffers availability as
                                                                    ///  dependency. Until this is set, it is not gauranteed
                                                                    ///  the input, output ImageBuffers are backed with buffers.
                                                                    ///  Nodes have to carefully set this flag when it *really*
                                                                    ///  needs buffers, setting this early even if buffers are
                                                                    ///  not needed on this dependency unit, will cause
                                                                    ///  unnecessary memory consumption.
            UINT32 isInternalDependency                 :  1;       ///< Is dependency internal to base node
            UINT32 reserved                             : 26;       ///< Reserved
        };
        UINT32 dependencyFlagsMask;                                 ///< Flag mask Value
    } dependencyFlags;

    PropertyDependency propertyDependency;                          ///< Set of Property dependency units

    struct
    {
        UINT      fenceCount;                                       ///< Number of fences to wait for
        CSLFence* phFences[MaxDependentFences];                     ///< Fences to wait for
        UINT*     pIsFenceSignaled[MaxDependentFences];             ///< Fence signal status
    } bufferDependency;                                             ///< Buffer dependency unit

    struct
    {
        UINT                chiFenceCount;                           ///< Number of fences to wait for
        ChiFence*           pChiFences[MaxDependentFences];          ///< Fences to wait for
        BOOL*               pIsChiFenceSignaled[MaxDependentFences]; ///< Fence signal status
        PFNCHIFENCECALLBACK pChiFenceCallback;                       ///< Callback when all Chi fence dependencies satisfied
        VOID*               pUserData;                               ///< Client-provided data pointer for Chi callback
    } chiFenceDependency;                                            ///< Buffer dependency unit

    INT32 processSequenceId;            ///< SequenceId to provide back to node when dependency satisfied
};

/// @brief Node create input data
struct NodeCreateInputData
{
    const PerNodeInfo*            pNodeInfo;                ///< Node info
    Camera3Stream**               ppCamera3Streams;         ///< Camera3 streams
    Pipeline*                     pPipeline;                ///< Pipeline pointer
    UINT                          pipelineNodeIndex;        ///< This node instance's index in the pipeline nodes
    const HwFactory*              pFactory;                 ///< HwFactory pointer
    ChiContext*                   pChiContext;              ///< Chi Context
    ChiNodeCallbacks*             pChiNodeCallbacks;        ///< CHINode callbacks pointer
    CHIAFALGORITHMCALLBACKS*      pAFAlgoCallbacks;         ///< Algo Calllback Interface for Chi
    CHIAECALGORITHMCALLBACKS*     pAECAlgoCallbacks;        ///< Algo Calllback Interface for Chi
    CHIAWBALGORITHMCALLBACKS*     pAWBAlgoCallbacks;        ///< Algo Calllback Interface for Chi
    CHIAFDALGORITHMCALLBACKS*     pAFDAlgoCallbacks;        ///< Algo Calllback Interface for Chi
    CHIASDALGORITHMCALLBACKS*     pASDAlgoCallbacks;        ///< Algo Calllback Interface for Chi
    CHIPDLIBRARYCALLBACKS*        pPDLibCallbacks;          ///< Algo Calllback Interface for Chi
    CHIISPHVXALGORITHMCALLBACKS*  pHVXAlgoCallbacks;        ///< HVX Algo Callback Interface for Chi
    CHIHISTALGORITHMCALLBACKS*    pHistAlgoCallbacks;       ///< Hist Algo Calllback Interface for Chi
    CHITRACKERALGORITHMCALLBACKS* pTrackerAlgoCallbacks;    ///< Tracker algorithm callback
};

/// @brief NodeCreateFlags flags
union NodeCreateFlags
{
    struct
    {
        BIT isSinkBuffer                    : 1;  ///< Node with a sink port that outputs a buffer
        BIT isSinkNoBuffer                  : 1;  ///< Node with a sink port that does not output a buffer
        BIT isSourceBuffer                  : 1;  ///< Node who has a input port that is not connected to any other node
        BIT isInPlace                       : 1;  ///< Is this an in place node whose output buffer is the same as input buffer
        BIT willNotifyConfigDone            : 1;  ///< Node needs to notify config done
        BIT isBypassable                    : 1;  ///< Is node a bypassable node
        BIT canDRQPreemptOnStopRecording    : 1;  ///< Can DRQ preempt node on recording stop/pause
        BIT hasDelayedNotification          : 1;  ///< Node has delayed notification
        BIT isDeferNotifyPipelineCreate     : 1;  ///< Node has delayed notification
        BIT reserved                        : 23; ///< Reserved
    };

    UINT value;
};

/// @brief Metadata list used/published by the node
struct NodeMetadataList
{
    UINT32   tagCount;                    ///< Number of tags published by the node
    UINT32   partialTagCount;             ///< Count of partial metadata in the tag array
    UINT32   tagArray[MaxTagsPublished];  ///< Array of tags published by the node
};

/// @brief Node vendor tag information
struct NodeVendorTag
{
    const CHAR* pSectionName;  ///< Vendor tag section
    const CHAR* pTagName;      ///< Vendor tag name
};

/// @brief Node composite fence info
struct BufferGroup
{
    BOOL    hasCompositeMask;                   ///< Flag to indicate if the node has any composite masking
    UINT    portGroupID[MaxBufferComposite];    ///< Composite group to which each port is mapped
};

/// @brief Port compsoite fence info
struct CompositePortFenceInfo
{
    CSLFence         hFence;                ///< Composite fence handle
    volatile UINT    aRefCount;             ///< reference count for the fence
    OutputPort*      pPrimaryOutputPort;    ///< Pointer to primary output port
};

/// @brief Node create output data
struct NodeCreateOutputData
{
    Node*           pNode;                                ///< Created node object pointer
    const CHAR*     pNodeName;                            ///< Name of the node (filled in the by the derived node)
    NodeCreateFlags createFlags;                          ///< Create flags
    UINT            maxNumCmdBufferManagers;              ///< Max require number of command buffer managers
    UINT            maxOutputPorts;                       ///< Max number of output ports supported
    UINT            maxInputPorts;                        ///< Max number of input ports supported
    BufferGroup     bufferComposite;                      ///< Buffer composite info
};

/// @brief Node capture request data.
struct NodeProcessRequestData
{
    CaptureRequest*      pCaptureRequest;                    ///< Stream configuration pointer
    DependencyUnit       dependencyInfo[MaxDependencies];    ///< List of Dependency info that will be reported back
                                                             ///  to topology. Each element in the list is a set of
                                                             ///  property and buffer dependencies
    UINT                 numDependencyLists;                 ///  Number of entries in the Dependency List
    PerBatchedFrameInfo* pPerBatchedFrameInfo;               ///< Per batched frame info
    INT32                processSequenceId;                  ///< Identifier for the node to track its processing order
    BOOL                 bindIOBuffers;                      ///< Flag indicating whether node requested input, output buffers
                                                             ///  to be available on this sequenceId
    BOOL                 isSequenceIdInternal;               ///< True if processSequenceId belongs to base node
};

/// @brief Output buffer info
struct FenceHandlerBufferInfo
{
    ImageBuffer*    pImageBuffer;                       ///< ImageBuffer for the output buffer
    UINT32          sequenceId;                         ///< Sequence ID that this buffer is associated with
    ChiBufferInfo   bufferInfo;                         ///< CHI buffer (valid only for output buffer on a sink port)
};

/// @brief Passed to CSL when registering fence callback. CSL passes this back in the fence callback. The fence callback
///        simply invokes the Node* and passes it the pNodePrivateData. Nodes fence handler processes the fence signal.
struct FenceCallbackData
{
    Node* pNode;            ///< Node to which the fence (being signaled) belongs to
    VOID* pNodePrivateData; ///< Node private data - Pointer to "struct PortFenceData"
};

/// @brief Node fence handler data - CSL fence callback function passes this data to the Node fence handler that processes
///        the fence signal
struct NodeFenceHandlerData
{
    FenceCallbackData       nodeCSLFenceCallbackData;            ///< Private data passed to CSL when registering
                                                                 ///  node fence callback
    OutputPort*             pOutputPort;                         ///< Output port to which this fence/imagebuffers
                                                                 ///  belong to
    UINT64                  requestId;                           ///< Request Id that this fence is associated with
    CSLFence                hFence;                              ///< Fence associated with the port
    BOOL                    isDelayedBufferFence;                ///< Is the fence for delayed output
    UINT                    isFenceSignaled;                     ///< Atomic Load/Store - Has the fence already been
                                                                 ///  signaled
    UINT                    numOutputBuffers;                    ///< Number of output buffers
    FenceHandlerBufferInfo* pOutputBufferInfo;                   ///< Info related to the output buffer on the port
    DelayedOutputBufferInfo delayedOutputBufferData;             ///< Output buffer info for bypassable node
    CSLFenceResult          fenceResult;                         ///< Fence result
    BOOL                    primaryFence;                        ///< Fence which primary in the composite
    BOOL                    isChiFence;                          ///< boolean to check if the fence is allocated by chi or camx
};

/// @brief This structure contains a subset of NodeFenceHandler data to be used in debug logging
struct NodeFenceHandlerDataErrorSubset
{
    CSLFence                hFence;                              ///< Fence associated with the port
    UINT64                  requestId;                           ///< Request Id that this fence is associated with
    UINT                    portId;                              ///< Port Id associated with the failed fence
};

/// @brief This structure contains a looping array used to keep track of the 50 most recent fence errors
struct FenceErrorBuffer
{
    NodeFenceHandlerDataErrorSubset fenceErrorData[MaxFenceErrorBufferDepth]; ///< Fence buffers
    UINT                            freeIndex;                                ///< Next free element in the array
};

/// @brief This structure contains the buffer requirements
struct BufferRequirement
{
    UINT32        minWidth;                         ///< Min width that the node requires to generate its final output buffer
    UINT32        minHeight;                        ///< Min height that the node requires to generate its final output buffer
    UINT32        maxWidth;                         ///< Max width that the node requires to generate its final output buffer
    UINT32        maxHeight;                        ///< Max height that the node requires to generate its final output buffer
    UINT32        optimalWidth;                     ///< Optimal width that the node would want on its input buffer
    UINT32        optimalHeight;                    ///< Optimal height that the node would want on its input buffer
    Format        format;                           ///< Buffer format
    AlignmentInfo planeAlignment[FormatsMaxPlanes]; ///< Stride and scanline alignment for each plane
};

/// @brief This structure contains the buffer requirements of the input ports on a node
struct PortBufferOptions
{
    UINT32            nodeId;               ///< Node Id
    UINT32            instanceId;           ///< Node instance Id
    UINT32            portId;               ///< Port Id
    BufferRequirement bufferRequirement;    ///< Buffer requirement
};

/// @brief Negotiation data for the input ports
struct InputPortNegotiationData
{
    UINT               inputPortId;                              ///< Input portId
    const ImageFormat* pImageFormat;                             ///< Image format locked down by the parent's output port
};

/// @brief Negotiation data for the output buffer per output port
struct OutputPortNegotiationData
{
    UINT               outputPortIndex;                           ///< Output port index
    BufferRequirement  inputPortRequirement[MaxDstPortsPerLink];  ///< Buffer requirement requested by the input ports
                                                                  ///  connected to the outputPort
    UINT               numInputPortsNotification;                 ///< Number of input ports that have provided their buffer
                                                                  ///  requirements for this outputPort
    BufferRequirement  outputBufferRequirementOptions;            ///< Valid options from which the final buffer properties
                                                                  ///  must be set
    BufferProperties*  pFinalOutputBufferProperties;              ///< Final output buffer properties decided after receiving
                                                                  ///  buffer requirements of all input ports connected to this
                                                                  ///  outputPort
};

/// @brief Structure that contains negotiation data for the node
struct BufferNegotiationData
{
    OutputPortNegotiationData* pOutputPortNegotiationData;              ///< Array of negotiation data per output port. Filled
                                                                        ///  on the walk-back from sink-to-source
    InputPortNegotiationData*  pInputPortNegotiationData;               ///< Array of negotiation data per input port. Filled
                                                                        ///  on the walk-forward from source-to-sink
    UINT                       numOutputPortsNotified;                  ///< Output ports whose requirements have been obtained
    UINT                       numInputPorts;                           ///< Number of input ports
    PortBufferOptions          inputBufferOptions[MaxPipelineInputs];   ///< Valid options for input port buffer
};

/// @brief Source input port chi fence information
struct NodeSourceInputPortChiFenceCallbackData
{
    Node*                   pNode;                  ///< Pointer to this Node object
    DeferredRequestQueue*   pDeferredRequestQueue;  ///< Pointer to deferred processing object
    CSLFence*               phFence;                ///< Pointer to the CSL fence
    UINT*                   pIsFenceSignaled;       ///< Fence variable whose signaled status is checked
    UINT64                  requestId;              ///< Request Id that the fence is waiting in
};

/// @brief OutputPort flags
union OutputPortFlags
{
    struct
    {
        BIT isBatchMode               : 1;  ///< Is batch mode enabled
        BIT isLateBindingDisabled     : 1;  ///< Flag to indicate if the late binding on the image buffer manager of the output
                                            ///  port has to be disabled.
        BIT disableSelfShrinking      : 1;  ///< Flag to indicate if self shrinking of buffers allocated at this port need to
                                            ///  be disabled
        BIT isNonSinkHALBufferOutput  : 1;  ///< Flag to indicate that the output port is not a sinkport but still outputs a
                                            ///  HAL buffer. This will happen in cases if the output port is connected to an
                                            ///  inplace node that outputs a HAL buffer.
        BIT isSinkBuffer              : 1;  ///< Is it a sink port with output buffer
        BIT isSinkNoBuffer            : 1;  ///< Is it a sink port with no output buffer
        BIT isSharedSinkBuffer        : 1;  ///< Is it a sink port with output buffer that also is consumed by other nodes
        BIT isEnabled                 : 1;  ///< Is the port enabled
        BIT isSecurePort              : 1;  ///< Is the port is secure/Non-secure
        BIT isLoopback                : 1;  ///< Is it a loopback port connected to the same node
        BIT isSinkInplaceBuffer       : 1;  ///< Is it a sink port with output buffer for Inplace node
        BIT reserved                  : 21; ///< Reserved
    };

    UINT value;
};

/// @brief InputPort flags
union InputPortFlags
{
    struct
    {
        UINT isEnabled           : 1;            ///< Is the port enabled
        UINT isSourceBuffer      : 1;            ///< Is it a source port with a HAL buffer
        UINT isLoopback          : 1;            ///< Is it a loopback port connected to the same node
        UINT isParentInputBuffer : 1;            ///< Is it a parent's input buffer
        UINT isBypssablePort     : 1;            ///< Is this port can be bypassed
        UINT reserved            : 27;           ///< Reserved
    };

    UINT value;
};

/// @brief Stream specific data information
struct StreamData
{
    UINT32                portFPS;              ///< Max FPS of all streams' FPS active at this port
    UINT8                 streamTypeBitmask;    ///< Bitmask containing supported stream type flags
};

/// @brief Stream type for a given node
enum StreamProcessType
{
    StreamTypeNotSet         = 0x0,             ///< Stream type not set for a given node
    RealtimeStream           = 0x1,             ///< Realtime stream type
    OfflineStream            = 0x2,             ///< Offline stream type
    RealTimeAndOfflineStream = 0x3,             ///< Node that supports both realtime and offline stream type
};

/// @brief Output port structure for single output port related information
struct OutputPort
{
    UINT                  portId;                            ///< Port id (from topology XML file)
    INT32                 deviceIndices[CamxMaxDeviceIndex]; ///< Device indices that may be access buffers owned by
                                                             ///< this port
    UINT                  deviceCount;                       ///< Number of devices that is in deviceIndices list
    BufferProperties      bufferProperties;                  ///< Buffer properties
    ImageBufferManager*   pImageBufferManager;               ///< Image buffer manager pointer
    ImageBuffer**         ppImageBuffers;                    ///< Array of Image buffers(numElements == maxImageBuffers)
    NodeFenceHandlerData* pFenceHandlerData;                 ///< Array of all fence related data - one array element
                                                             ///  for one image buffer/fence
    NodeFenceHandlerData* pDelayedBufferFenceHandlerData;    ///< Array of all fence related data - one array element
                                                             ///  for one image buffer/fence
    UINT                  sinkTargetStreamId;                ///< If this is a sink port that outputs an HAL buffer,
                                                             ///  the streamId that this sink port maps to
    UINT                  enabledInStreamMask;               ///< List of Stream Ids that enable this output port
                                                             ///  Bit 0 = 1 if Stream Id 0 enables it
                                                             ///        = 0 if Stream Id 0 does not enable it
                                                             ///  Same for other stream Ids. So for e.g. if Stream 0 and
                                                             ///  Stream 2 enables this output port - the mask value
                                                             ///  would be 5
    UINT                  numInputPortsConnected;            ///< These are the number of input ports connected to this output
                                                             ///  port
    UINT                  numInputPortsDisabled;             ///< Number of input ports that do not use this output although
                                                             ///  they are connected
    BufferPropertiesInfo* pBufferProperties;                 ///< Bufferproperties coming from the XML
    UINT                  numBatchedFrames;                  ///< Number of batched frames in this output port
    BOOL                  HALOutputBufferCombined;           ///< Is the HAL output buffer combined for batch mode
    OutputPortFlags       flags;                             ///< OutputPort flags
    UINT                  portSourceTypeId;                  ///< Port source type id
    UINT                  numSourcePortsMapped;              ///< Number of source Ports mapped to this output port for bypass
    UINT*                 pMappedSourcePortIds;              ///< Pointer to the source port Ids mapped to this output ports
    ChiStreamWrapper*     pChiStreamWrapper;                 ///< ChiStreamWrapper on the port
                                                             ///  for bypass
    UINT                  numInputPortsConnectedinBypass;    ///< These are the number of input ports connected to this output
                                                             ///  port through bypass
    UINT                  numOfNodesConnected;               ///< Number of nodes connected to this output port
    Node*                 pDstNodes[MaxNumPipelineNodes];    ///< List of nodes connected to this output port
    StreamData            streamData;                        ///< Stream specific data
    ///@ todo (CAMX-326) Buffer manager pointers will live here and image buffers for requests will be maintained explicitly
};

/// @brief Data for all output ports
struct AllOutputPortsData
{
    UINT        numPorts;                               ///< Number of ports
    OutputPort* pOutputPorts;                           ///< Pointer to an array of output ports
    UINT        sinkPortIndices[MaxNumStreams];         ///< Array of output (SINK) port index (in the pOutputPorts array)
    UINT        numSinkPorts;                           ///< Number of sink ports
    CSLFence    hBufferGroupFence[MaxBufferComposite];  ///< Fence handle for each buffer composite group
};

/// @brief Data for output port queried by clients (usually connected nodes querying each other)
struct OutputPortRequestedData
{
    ImageBuffer*             pImageBuffer;                  ///< Image buffer on the output port (for the queried request Id)
    CSLFence*                phFence;                       ///< Fence for this output port
    UINT*                    pIsFenceSignaled;              ///< Pointer to fence variable whose signaled status is checked
    DelayedOutputBufferInfo* pDelayedOutputBufferData;      ///< Delayed output buffer data
    const OutputPort*        pOutputPort;                   ///< The output port requested
    BOOL                     primaryFence;                  ///< Fence which primary in the composite
};

/// @brief Input port structure for single input port related information
struct InputPort
{
    UINT                  portId;                  ///< Port id (from topology XML file)
    Node*                 pParentNode;             ///< Parent node for triggering calls like
                                                   ///  pParentNode::GetImageBuffer(portId, requestId)
                                                   ///  The above buffer gets programmed as input buffer for
                                                   ///  the node with this input
    UINT                  parentOutputPortIndex;   ///< Parent output port to which this input port is connected port.
                                                   ///  Parent node may have an array of 'n' output ports, parentPortId tells
                                                   ///  the array element i.e. the parent output port to which this input port
                                                   ///  is connected
    UINT                  sourceTargetStreamId;    ///< If this is a source buffer port,
                                                   ///  the streamId that this source port maps to
    ImageFormat           imageFormat;             ///< Buffer image format if this input port is a source buffer port
    ImageBuffer**         ppImageBuffers;          ///< Container ImageBuffer if this input port is a source buffer port
    InputPortFlags        flags;                   ///< Flags
    CSLFence*             phFences;                ///< Input buffer fences
    UINT*                 pIsFenceSignaled;        ///< Fence variable whose signaled status is checked
    union FenceSourceFlags                         ///< CSL Fence association flags with external layer/modules to CAMX
    {
        struct
        {
            BIT isChiAssociatedFence : 1;          ///< Is this a CHI associated CSL fence?
            BIT reserved             : 31;         ///< Reserved
        };
        UINT allFlags;
    } *pFenceSourceFlags;
    UINT                  portSourceTypeId;        ///< Port source type id
    BOOL                  portDisabled;            ///< Input port disabled if set
    UINT32                bufferDelta;             ///< Indicates buffer requirement of current request - bufferDelta
    ImageBufferManager*   pImageBufferManager;     ///< Image buffer manager pointer
    UINT                  inputBufferIndex;        ///< inputBuffer index for this port
};

/// @brief Data for all input ports
struct AllInputPortsData
{
    UINT        numPorts;       ///< Number of ports
    InputPort*  pInputPorts;    ///< Pointer to an array of output ports
};

/// @brief Per request output port flags
union PerRequestOutputPortFlags
{
    struct
    {
        BIT isOutputHALBuffer  : 1;                     ///< Is the output of this port a HAL buffer
        BIT isBatch            : 1;                     ///< Is the batch mode enabled for this port
        BIT isDelayedBuffer    : 1;                     ///< Is delayed output buffer
        BIT isLoopback         : 1;                     ///< Is loopback port
        BIT reserved           : 28;                    ///< Reserved
    };

    UINT allFlags;
};

/// @brief Per request input port flags
union PerRequestInputPortFlags
{
    struct
    {
        BIT isInputHALBuffer    : 1;                     ///< Is the input of this port a HAL buffer
        BIT isPendingBuffer     : 1;                     ///< Is the image buffer of this port pending to be resolved
        BIT isParentInputBuffer : 1;                     ///< Is the buffer from parent's input
        BIT reserved            : 29;                    ///< Reserved
    };

    UINT allFlags;
};

/// @brief Per request info for one output port to be sent to the derived nodes that implement request processing
struct PerRequestOutputPortInfo
{
    UINT                        portId;                             ///< PortId from XML
    ImageBuffer**               ppImageBuffer;                      ///< All the output buffers of this port
    PerRequestOutputPortFlags   flags;                              ///< Flags
    UINT                        numOutputBuffers;                   ///< Number of output buffers that will be output
    CSLFence*                   phFence;                            ///< Fence associated with all Imagebuffers in the output
                                                                    ///  port
    UINT*                       pIsFenceSignaled;                   ///< Pointer to fence variables signaled status
    CSLFence*                   phDelayedBufferFence;               ///< Pointer to the associated fence for delayed buffer
    DelayedOutputBufferInfo*    pDelayedOutputBufferData;           ///< Pointer to the delayed output buffer
    UINT                        mappedInputPortd;                   ///< Mapped input port identifier to be used by bypass mode
    UINT                        numPerReqInputPortsDisabled;        ///< Number of input ports that are connected but not in
                                                                    ///  use (unused nodes)
};

/// @brief Per request info for one input port to be sent to the derived nodes that implement request processing
struct PerRequestInputPortInfo
{
    UINT                     portId;                    ///< PortId from XML
    ImageBuffer*             pImageBuffer;              ///< ImageBuffer associated with the port
    PerRequestInputPortFlags flags;                     ///< Flags
    CSLFence*                phFence;                   ///< Fence associated with the ImageBuffer
    BOOL                     primaryFence;              ///< Fence which primary in the composite
    UINT*                    pIsFenceSignaled;          ///< Pointer to fence variable whose signaled status is checked
    DelayedOutputBufferInfo* pDelayedOutputBufferData;  ///< Pointer to the delayed output buffer
    ChiStreamWrapper*        pParentSinkStream;         ///< Sink stream of the parent
};

/// @brief Per request info of all enabled input/output ports to be sent to the derived nodes that implement request processing
struct PerRequestActivePorts
{
    UINT                      numOutputPorts;          ///< Number of output ports per request
    UINT                      numInputPorts;           ///< Number of input ports per request
    PerRequestOutputPortInfo* pOutputPorts;            ///< Per request output ports - points to array size numOutputPorts
    PerRequestInputPortInfo*  pInputPorts;             ///< Per request input ports  - points to array size numInputPorts
};

/// @brief Request status within this node, if you modify this, change Node::GetRequestStatusString accordingly.
enum class PerRequestNodeStatus
{
    Uninitialized,  ///< Initial status, setup request was not called.
    Setup,          ///< The request is setup, setup request was called.
    Deferred,       ///< The request is in the DRQ
    Running,        ///< The request is currently processing
    Error,          ///< The request is in error state or ExecuteProcessRequest did not return success or cancelled.
    Cancelled,      ///< The request is cancelled
    Success,        ///< The request completed with success.
    Submit          ///< The request has been submitted to HW
};

/// @brief Basic node processing stages used to track processing time for a given node
enum class NodeStage
{
    Start,              ///< First time ProcessRequest is called for a given request
    DependenciesMet,    ///< Node's dependencies are met for a given request
    EPReqEnd,           ///< Execute Process Request for the sequence id is done
    End,                ///< Node finish processing for a given request
    MaxNodeStatuses     ///< Maximum node stages
};

static const CHAR* NodeStageStrings[] =
{
    "Start",
    "DepMet",
    "EPReqEnd",
    "End",
    "MaxNodeStatuses"
};

CAMX_STATIC_ASSERT(CAMX_ARRAY_SIZE(NodeStageStrings) == (static_cast<UINT>(NodeStage::MaxNodeStatuses) + 1));

/// @brief NodeStage name and timestamp of NodeStage completion
struct NodeProcessingInfo
{
    UINT32 timestamp;      ///< Timestamp that NodeStage was completed at
};

struct NodePerRequestInfo
{
    PerRequestActivePorts       activePorts;                  ///< Per request ports info passed to derived node
    UINT                        numUnsignaledFences;          ///< Number of unsigned fences in a particular request. This is
                                                              ///  decremented with each fence that triggers for the node.
                                                              ///  0 means all outputs are available.
    UINT                        numUnprocessedFences;         ///< Number of unprocessed fences.
    UINT64                      requestId;                    ///< incoming requestId
    UINT                        partialMetadataComplete;      ///< Binary flag representing whether partial metadata has been
                                                              ///  reported
    UINT                        metadataComplete;             ///< Binary flag representing whether metadata has been reported
    UINT                        requestComplete;              ///< Binary flag representing whether request has been reported
    std::unordered_set<UINT32>  partialPublishedSet;          ///< Partial Published list for the node
    PerRequestNodeStatus        requestStatus;                ///< Defines the status of the request. Set via SetRequestStatus
    UINT32                      nodeProcessingTime;           ///< Time it took for node to process

    NodeProcessingInfo          nodeProcessingStages[MaxProcessSequenceIdForTiming]
                                                    [static_cast<UINT>(NodeStage::MaxNodeStatuses)];
                                                              ///< Time it take for node to complete stage per sequence id
};

/// @brief Execute process request data to be passed to the derived nodes that implement request processing
struct ExecuteProcessRequestData
{
    NodeProcessRequestData* pNodeProcessRequestData;    ///< Request data coming in from Pipeline
    PerRequestActivePorts*  pEnabledPortsInfo;          ///< All enabled ports for the request
    ChiTuningModeParameter* pTuningModeData;            ///< Pointer to tuning mode selector data
};

/// @brief Input port crop info
struct PortCropInfo
{
    CHIRectangle appliedCrop;     ///< Applied crop
    CHIRectangle residualCrop;    ///< Residual crop
    CHIDimension residualFullDim; ///< residual full dim
};

/// @brief Node flags
union NodeFlags
{
    struct
    {
        BIT isEnabled                       :  1;      ///< Is the node enabled
        BIT isInplace                       :  1;      ///< Is inplace node i.e. output sink buffer same as input buffer
        BIT isRealTime                      :  1;      ///< Is this node part of a real time pipeline
        BIT isSecureMode                    :  1;      ///< Is this node  Secure/non-secure
        BIT isHwDeviceAcquired              :  1;      ///< Did this node successfully acquired a hw device or not
        BIT callNotifyConfigDone            :  1;      ///< Call notify config done
        BIT isBypassable                    :  1;      ///< Is this node bypassable
        BIT canDRQPreemptOnStopRecording    :  1;      ///< Can DRQ preempt node on recording stop/pause
        BIT hasDelayedNotification          :  1;      ///< Node has delayed notification
        BIT isDeferNotifyPipelineCreate     :  1;      ///< Is this node defer to NotifyPipelineCreated
        BIT reserved                        :  22;     ///< Reserved
    };

    UINT value;
};

/// @brief Data entry containing Port-Specific Metadata payload for each type
struct PSMQueueEntry
{
    VOID* pPSMData[MaxPSMetaType]; ///< Array of data pointers for each type
    BOOL  isOwner[MaxPSMetaType];  ///< Checks if the memory is owned by this entry
};

/// @brief Queue containing Port-Specific Metadata information for each output port
struct PSMQueue
{
    PSMQueueEntry*  pEntry;     ///< Array of PS Metadata entries
    UINT            maxSize;    ///< Max size of circular buffer
};

/// @brief Structure that contains all the required information for Port-Specific metadata
struct PSMInfo
{
    UINT                    tagId;                 ///< Tag Identifier
    UINT                    tagSize;               ///< Tag Size
    UniqueNodePortInfoMap   nodePortInfoMap;       ///< Publisher info for link metadata
    BOOL                    isMapInitialized;      ///< On demand initialization of PSMeta Map
    BOOL                    canPublish;            ///< Flag to indicate whether PSMeta will be
                                                   ///   published by the node
    UINT                    hasBypassableAncestor; ///< Flag to indicate whether the Node has an
                                                   ///   ancestor that publishes
    BOOL                    isSharedQueue;         ///< Flag to indicate whether the queue is shared
    UINT                    sharedPortQueueIndex;  ///< Queue Index of the shared port
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the hwl node.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create node object
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpDebugInfo
    ///
    /// @brief  Dumps current node state
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpDebugInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBufferSkipProperties
    ///
    /// @brief  Set Buffer Skip property
    ///
    /// @param  skip Set TRUE if we need to skip
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetBufferSkipProperties(
        BOOL skip)
    {
        m_skipUpdatingBufferProperties = skip;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyPipelineCreated
    ///
    /// @brief  Notification that the topology and all associated nodes are created
    ///
    /// @param  isDefer  whether to defer call PostPipelineCreate function
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult NotifyPipelineCreated(
        BOOL isDefer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DetermineBufferProperties
    ///
    /// @brief  Determine the buffer properties of each output port
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DetermineBufferProperties();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HasAnyDependency
    ///
    /// @brief  Checks if any dependencies are set
    ///
    /// @param  pDependency  Pointer to the dependency structure
    ///
    /// @return TRUE if there are any dependencies, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE BOOL HasAnyDependency(
        const DependencyUnit* pDependency)
    {
        BOOL hasAnyDependency = FALSE;

        if ((NULL != pDependency) && (0 != pDependency->dependencyFlags.dependencyFlagsMask))
        {
            hasAnyDependency = TRUE;
        }

        return hasAnyDependency;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestStatusString
    ///
    /// @brief  Get the status of a request in a string.
    ///
    /// @param  status    The status in question
    ///
    /// @return string format of the status of a request
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static const CHAR* GetRequestStatusString(
        PerRequestNodeStatus status)
    {
        static const CHAR* pStrings[] =
        {
            "UNINITIALIZED",
            "SETUP",
            "DEFERRED",
            "RUNNING",
            "ERROR",
            "CANCELLED",
            "SUCCESS",
            "SUBMIT"
        };

        CAMX_ASSERT(CAMX_ARRAY_SIZE(pStrings) > static_cast<UINT>(status));

        return pStrings[static_cast<UINT>(status)];
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
    /// CreateBufferManagers
    ///
    /// @brief  Create buffer managers in all the output ports of a non-inplace node
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateBufferManagers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy node object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyParentHALBufferOutput
    ///
    /// @brief  This function is only called by an inplace node to inform the parent node to output a HAL buffer (to its
    ///         output port to which this inplace node is connected)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyParentHALBufferOutput();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyHALBufferOutput
    ///
    /// @brief  This function notifies the node that its output port must output a HAL buffer. This is called by an inplace
    ///         child node that outputs a HAL buffer, to inform the parent node that it also needs to output to the same HAL
    ///         buffer
    ///
    /// @param  outputPortIndex    Output port index
    /// @param  pInplaceOutputPort Inplace child's output port info
    /// @param  pChiStreamWrapper  Output stream wrapper of inplace child
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID NotifyHALBufferOutput(
        UINT              outputPortIndex,
        OutputPort*       pInplaceOutputPort,
        ChiStreamWrapper* pChiStreamWrapper)
    {
        m_outputPortsData.pOutputPorts[outputPortIndex].flags.isNonSinkHALBufferOutput = TRUE;
        m_outputPortsData.pOutputPorts[outputPortIndex].sinkTargetStreamId = pInplaceOutputPort->sinkTargetStreamId;

        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];
        pOutputPort->flags.isNonSinkHALBufferOutput = TRUE;

        CAMX_ASSERT(CamxInvalidStreamId != pOutputPort->sinkTargetStreamId);

        pOutputPort->enabledInStreamMask = (1 << pOutputPort->sinkTargetStreamId);
        pOutputPort->pChiStreamWrapper = pChiStreamWrapper;
        InitializeNonSinkHALBufferPortBufferProperties(outputPortIndex, pInplaceOutputPort, pChiStreamWrapper);
        TriggerBufferNegotiation();

        if (TRUE == IsInplace())
        {
            NotifyParentHALBufferOutput();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareNodeStreamOn
    ///
    /// @brief  Method that is called by topology before streamOn is sent to HW. This generally happens in FinalizePipeline
    ///         This really doesn't mean StreamOn is on the way immediately.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult PrepareNodeStreamOn()
    {
        CamxResult result = PrepareStreamOn();

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnNodeStreamOn
    ///
    /// @brief  Method that is called by topology on streamOn. This generally happens in Activate Pipeline.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult OnNodeStreamOn()
    {
        CamxResult result = ActivateImageBuffers();

        if (CamxResultSuccess == result)
        {
            result = OnStreamOn();
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnNodeStreamOff
    ///
    /// @brief  Method that is called by topology before streamOff is sent to HW. Nodes may use
    ///         this hook to do preparation with respect to the bitmask passed in.
    ///
    /// @param  modeBitmask   Mode Bitmask that indicates specific tasks node needs to do during streamoff.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult OnNodeStreamOff(
        CHIDEACTIVATEPIPELINEMODE modeBitmask)
    {
        CamxResult result = CamxResultSuccess;

        if (CHIDeactivateModeReleaseBuffer != modeBitmask)
        {
            if (CamxResultSuccess == result)
            {
                result = OnStreamOff(modeBitmask);
            }
        }

        if ((CamxResultSuccess == result) &&
            (modeBitmask & CHIDeactivateModeReleaseBuffer))
        {
            result = DeactivateImageBuffers();
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NodeAcquireResources
    ///
    /// @brief  Method that is called by topology before streamOn. This generally happens in Activate Pipeline.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult NodeAcquireResources()
    {
        return AcquireResources();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NodeReleaseResources
    ///
    /// @brief  Method that is called by topology after streamOff. This generally happens in deactivate Pipeline.
    ///
    /// @param  modeBitmask Deactivate pipeline mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult NodeReleaseResources(
        CHIDEACTIVATEPIPELINEMODE modeBitmask)
    {
        return ReleaseResources(modeBitmask);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRequest
    ///
    /// @brief  Configure a new request
    ///
    /// @param  pPerBatchedFrameInfo       Per batch information for this request
    /// @param  pCurrentActiveStreams      Current active streams
    /// @param  differentStreams           flag to indicate that the current active stream is different from last stream
    /// @param  requestId                  Request to (re)process
    /// @param  syncId                     Sync ID for this request
    /// @param  pIsEnabled                 Updated to report whether node ended up enabled for request
    ///
    /// @return Pointer to the per frame property/metadata pool
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupRequest(
        PerBatchedFrameInfo* pPerBatchedFrameInfo,
        UINT*                pCurrentActiveStreams,
        BOOL                 differentStreams,
        UINT64               requestId,
        UINT64               syncId,
        BOOL*                pIsEnabled);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InvalidateRequest
    ///
    /// @brief  Invalidate the node data for the request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID InvalidateRequest()
    {
        Utils::Memset(m_outputPortsData.hBufferGroupFence, CSLInvalidHandle, sizeof(m_outputPortsData.hBufferGroupFence));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequest
    ///
    /// @brief  Method to trigger process request for a node object.
    ///
    /// @param  pNodeRequestData           Node process request data
    /// @param  requestId                  Request to process
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessRequest(
        NodeProcessRequestData* pNodeRequestData,
        UINT64                  requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushStatus
    ///
    /// @brief  Return Flush status
    ///
    /// @return TRUE if the session is in Flush state otherwise False
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL GetFlushStatus()
    {
        return m_pPipeline->GetFlushStatus();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Type
    ///
    /// @brief  Return type of the node
    ///
    /// @return type of the node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT Type() const
    {
        return m_nodeType;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineName
    ///
    /// @brief  Return name of the pipeline
    ///
    /// @return name of the pipeline
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CHAR* GetPipelineName() const
    {
        return m_pPipeline->GetPipelineName();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NodeHasDelayedNotification
    ///
    /// @brief  Update if node has opted for delayed execution
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL NodeHasDelayedNotification() const
    {
        return m_nodeFlags.hasDelayedNotification;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Name
    ///
    /// @brief  Return name of the node
    ///
    /// @return name of the node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CHAR* Name() const
    {
        return m_pNodeName;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NodeIdentifierString
    ///
    /// @brief  Return the string containing pipeline name, node name and node instance id
    ///
    /// @return name and instance id of the node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CHAR* NodeIdentifierString() const
    {
        return m_nodeIdentifierString;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSecureMode
    ///
    /// @brief  Return Mode of the node secure/non-secure
    ///
    /// @return TRUE means SECURE mode or FALSE is non secure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    CAMX_INLINE BOOL IsSecureMode() const
    {
        return m_nodeFlags.isSecureMode;
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
        return m_pPipeline->IsCameraRunningOnBPS();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InstanceID
    ///
    /// @brief  Return instance id of the node
    ///
    /// @return instance id of the node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT InstanceID() const
    {
        return m_instanceId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeviceIndices
    ///
    /// @brief  Return device index list of this node (the devices acquired and that access associated buffers)
    ///
    /// @return device index list pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const INT32* DeviceIndices() const
    {
        return &(m_deviceIndices[0]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeviceIndexCount
    ///
    /// @brief  Return device index count of this node
    ///
    /// @return device index count for this node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 DeviceIndexCount() const
    {
        return m_deviceIndexCount;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddCSLDeviceHandle
    ///
    /// @brief  Add the given CSL device handle to the list for this node for linking.
    ///
    /// @param  hCslDeiveHandle to add
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddCSLDeviceHandle(
        CSLDeviceHandle hCslDeiveHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLDeviceHandle
    ///
    /// @brief  Return csl handle for this node for given index.
    ///
    /// @param  index to the array of csl handle.
    ///
    /// @return device csl handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CSLDeviceHandle GetCSLDeviceHandle(
        UINT32 index) const
    {
        return (index < m_cslDeviceCount) ? m_hCSLDeviceHandles[index] : 0;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CSLDeviceHandleCount
    ///
    /// @brief  Return csl device index count of this node
    ///
    /// @return device index count for this node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 CSLDeviceHandleCount() const
    {
        return m_cslDeviceCount;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPortImageFormat
    ///
    /// @brief  Return output port buffer image format
    ///
    /// @param  outputPortIndex output port index into all output ports maintained in this node
    ///
    /// @return ImageFormat
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const ImageFormat* GetOutputPortImageFormat(
        UINT outputPortIndex
        ) const
    {
        return ((outputPortIndex < m_outputPortsData.numPorts) ?
                (&m_outputPortsData.pOutputPorts[outputPortIndex].bufferProperties.imageFormat) : NULL);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputPortImageFormat
    ///
    /// @brief  Return input port buffer image format
    ///
    /// @param  inputPortIndex input port index into all input ports maintained in this node
    ///
    /// @return ImageFormat
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const ImageFormat* GetInputPortImageFormat(
        UINT inputPortIndex
        ) const
    {
        const ImageFormat* pImageFormat = NULL;

        if (FALSE == IsSourceBufferInputPort(inputPortIndex))
        {
            UINT outputPortIndex = m_inputPortsData.pInputPorts[inputPortIndex].parentOutputPortIndex;

            if (inputPortIndex < m_inputPortsData.numPorts)
            {
                pImageFormat =
                    m_inputPortsData.pInputPorts[inputPortIndex].pParentNode->GetOutputPortImageFormat(outputPortIndex);
            }
        }
        else
        {
            pImageFormat = &m_inputPortsData.pInputPorts[inputPortIndex].imageFormat;
        }

        return pImageFormat;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetParentNodeType
    ///
    /// @brief  Return parent node type
    ///
    /// @param  inputPortId input port id
    ///
    /// @return parent node type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT GetParentNodeType(
        UINT inputPortId
        ) const
    {

        BOOL found     = FALSE;
        UINT nodeType  = 0;
        UINT portIndex = 0;

        for (portIndex = 0; portIndex < m_inputPortsData.numPorts; portIndex++)
        {
            if (inputPortId == m_inputPortsData.pInputPorts[portIndex].portId)
            {
                found = TRUE;
                break;
            }
        }
        if ((found == TRUE) &&
            (NULL != m_inputPortsData.pInputPorts[portIndex].pParentNode))
        {
            nodeType = m_inputPortsData.pInputPorts[portIndex].pParentNode->Type();
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupCore, "cannot find portId %d for %s", inputPortId, NodeIdentifierString());
        }

        return nodeType;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetParentNodeOutputPortId
    ///
    /// @brief  Return parent node output port Id
    ///
    /// @param  inputPortId input port id
    ///
    /// @return parent output port id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT GetParentNodeOutputPortId(
        UINT inputPortId
    ) const
    {
        BOOL found                  = FALSE;
        UINT portIndex              = 0;
        INT  parentNodeOutputPortId = 0;

        for (portIndex = 0; portIndex < m_inputPortsData.numPorts; portIndex++)
        {
            if (inputPortId == m_inputPortsData.pInputPorts[portIndex].portId)
            {
                found = TRUE;
                break;
            }
        }
        if ((found == TRUE) &&
            (NULL != m_inputPortsData.pInputPorts[portIndex].pParentNode))
        {
            UINT  parentOutputPortIndex = m_inputPortsData.pInputPorts[portIndex].parentOutputPortIndex;
            Node* pParentNode           = m_inputPortsData.pInputPorts[portIndex].pParentNode;

            parentNodeOutputPortId =
               static_cast<INT>(pParentNode->m_outputPortsData.pOutputPorts[parentOutputPortIndex].portId);
        }
        else
        {
            parentNodeOutputPortId = -1;
        }
        return parentNodeOutputPortId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPortSourceType
    ///
    /// @brief  Return output port source type ID
    ///
    /// @param  outputPortIndex output port index
    ///
    /// @return port source type ID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT GetOutputPortSourceType(
        UINT outputPortIndex
    ) const
    {
        return m_outputPortsData.pOutputPorts[outputPortIndex].portSourceTypeId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputPortSourceType
    ///
    /// @brief  Return input port source type ID
    ///
    /// @param  inputPortIndex input port index
    ///
    /// @return port source type ID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT GetInputPortSourceType(
        UINT inputPortIndex
    ) const
    {
        return m_inputPortsData.pInputPorts[inputPortIndex].portSourceTypeId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OutputPortDeviceIndices
    ///
    /// @brief  Return output port device index list for a given output port
    ///
    /// @param  portIndex port index
    ///
    /// @return output port device index list pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const INT32* OutputPortDeviceIndices(
        UINT portIndex)
    {
        CAMX_ASSERT(m_outputPortsData.pOutputPorts != NULL);

        return &(m_outputPortsData.pOutputPorts[portIndex].deviceIndices[0]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OutputPortDeviceCount
    ///
    /// @brief  Return output port device count for a given output port
    ///
    /// @param  portIndex port index
    ///
    /// @return output port device count
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 OutputPortDeviceCount(
        UINT portIndex)
    {
        CAMX_ASSERT(m_outputPortsData.pOutputPorts != NULL);

        return (m_outputPortsData.pOutputPorts[portIndex].deviceCount);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OutputPortCount
    ///
    /// @brief  Return number of output ports
    ///
    /// @return output port count
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 OutputPortCount()
    {
        return m_outputPortsData.numPorts;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InputPortIndex
    ///
    /// @brief  Return input port index for a given port id
    ///
    /// @param  portId port id
    ///
    /// @return port index
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT InputPortIndex(
        UINT portId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OutputPortIndex
    ///
    /// @brief  Return output port index for a given port id
    ///
    /// @param  portId port id
    ///
    /// @return port index if a match is found, InvalidPortIndex otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT OutputPortIndex(
        UINT portId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPortInfo
    ///
    /// @brief  Return output port info (mostly to be called by other nodes connected to this node)
    ///
    /// @param  requestId           requestId for which the info is seeked
    /// @param  sequenceId          sequenceId for which the info is seeked
    /// @param  outputPortIndex     output port index into the output port structure maintained in this node
    /// @param  pOutData            output data requested by the client
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetOutputPortInfo(
        UINT64                   requestId,
        UINT32                   sequenceId,
        UINT                     outputPortIndex,
        OutputPortRequestedData* pOutData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetImageBufferManager
    ///
    /// @brief  Set image buffer manager pointer for a given output port
    ///
    /// @param  portIndex             port index
    /// @param  pImageBufferManager   image buffer manager to be set for the given port
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetImageBufferManager(
        UINT                portIndex,
        ImageBufferManager* pImageBufferManager)
    {
        CAMX_ASSERT(m_outputPortsData.pOutputPorts != NULL);

        m_outputPortsData.pOutputPorts[portIndex].pImageBufferManager = pImageBufferManager;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSinkPort
    ///
    /// @brief  Query for if a port is a sink port
    ///
    /// @param  portIndex queried port index
    ///
    /// @return TRUE if it is a sink port, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSinkPort(
        UINT portIndex
        ) const
    {
        CAMX_ASSERT(portIndex < m_outputPortsData.numPorts);

        BOOL isSink = FALSE;

        if (portIndex < m_outputPortsData.numPorts)
        {
            isSink = (m_outputPortsData.pOutputPorts[portIndex].flags.isSinkBuffer ||
                      m_outputPortsData.pOutputPorts[portIndex].flags.isSinkNoBuffer);
        }

        return isSink;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSinkNoBufferNode
    ///
    /// @brief  Query for if a node is Sink node with no buffer
    ///
    /// @return TRUE if it is a sink node, with no buffer, else FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSinkNoBufferNode() const
    {
        BOOL isSinkNoBuffer = TRUE;

        for (UINT portIndex = 0; portIndex < m_outputPortsData.numPorts; portIndex++)
        {
            isSinkNoBuffer = m_outputPortsData.pOutputPorts[portIndex].flags.isSinkNoBuffer;
            if (FALSE == isSinkNoBuffer)
            {
                break;
            }
        }

        return isSinkNoBuffer;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSourceBufferInputPort
    ///
    /// @brief  Query for if a port is a source buffer port
    ///
    /// @param  portIndex queried port index
    ///
    /// @return TRUE if it is a source buffer input port, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSourceBufferInputPort(
        UINT portIndex
        ) const
    {
        CAMX_ASSERT(portIndex < m_inputPortsData.numPorts);

        BOOL isSource = FALSE;

        if (portIndex < m_inputPortsData.numPorts)
        {
            isSource = (m_inputPortsData.pInputPorts[portIndex].flags.isSourceBuffer);
        }

        return isSource;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsInplace
    ///
    /// @brief  Check if the node is an inplace node
    ///
    /// @return TRUE if it is a inplace node, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsInplace() const
    {
        return m_nodeFlags.isInplace;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBypassableNode
    ///
    /// @brief  Check if the bypass property of the node is set
    ///
    /// @return TRUE if it is bypassed, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsBypassableNode() const
    {
        return m_nodeFlags.isBypassable;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNonSinkHALBufferOutput
    ///
    /// @brief  Query for if a port is not a sink port but still outputs to a HAL buffer
    ///
    /// @param  portIndex queried port index
    ///
    /// @return TRUE if it is not a sink port but still outputs to a HAL buffer, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsNonSinkHALBufferOutput(
        UINT portIndex
        ) const
    {
        return (((portIndex < m_outputPortsData.numPorts) && (FALSE == IsSinkPort(portIndex))) ?
                   m_outputPortsData.pOutputPorts[portIndex].flags.isNonSinkHALBufferOutput : FALSE);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSinkPortWithBuffer
    ///
    /// @brief  Query for if a port is a sink port with output buffer
    ///
    /// @param  portIndex queried port index
    ///
    /// @return TRUE if it is a sink port with output buffer, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSinkPortWithBuffer(
        UINT portIndex
        ) const
    {
        CAMX_ASSERT(portIndex < m_outputPortsData.numPorts);

        BOOL isSink = FALSE;

        if (portIndex < m_outputPortsData.numPorts)
        {
            isSink = m_outputPortsData.pOutputPorts[portIndex].flags.isSinkBuffer;
        }

        return isSink;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSinkPortNoBuffer
    ///
    /// @brief  Query for if a port is a sink port with no output buffer
    ///
    /// @param  portIndex queried port index
    ///
    /// @return TRUE if it is a sink port with no output buffer, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSinkPortNoBuffer(
        UINT portIndex
        ) const
    {
        CAMX_ASSERT(portIndex < m_outputPortsData.numPorts);

        BOOL isSinkPortNoBuffer = FALSE;

        if (portIndex < m_outputPortsData.numPorts)
        {
            isSinkPortNoBuffer = m_outputPortsData.pOutputPorts[portIndex].flags.isSinkNoBuffer;
        }

        return isSinkPortNoBuffer;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBatchEnabledPort
    ///
    /// @brief  Query for if a port has batch mode enabled
    ///
    /// @param  portId  queried port ID
    ///
    /// @return TRUE if the port has batch mode enabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsBatchEnabledPort(
        UINT portId
    ) const
    {
        BOOL isBatchEnabledPort = FALSE;

        for (UINT portIndex = 0; portIndex < m_outputPortsData.numPorts; portIndex++)
        {
            if (m_outputPortsData.pOutputPorts[portIndex].portId == portId)
            {
                isBatchEnabledPort = m_outputPortsData.pOutputPorts[portIndex].flags.isBatchMode;
                break;
            }
        }

        return isBatchEnabledPort;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsOutputPortSecure
    ///
    /// @brief  Query for if a port is secure or Non-secure
    ///
    /// @param  portId queried port Id
    ///
    /// @return TRUE if the port is secure, else FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsOutputPortSecure(
        UINT portId)
    {
        UINT index = OutputPortIndex(portId);

        CAMX_ASSERT(index < m_outputPortsData.numPorts);

        BOOL isSecurePort = FALSE;

        if (index < m_outputPortsData.numPorts)
        {
            isSecurePort = m_outputPortsData.pOutputPorts[index].flags.isSecurePort;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid parameters.  nodeType = %d, portIndex = %d, numPorts = %d",
                m_nodeType, portId, m_outputPortsData.numPorts);
        }

        return isSecurePort;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsTorchWidgetNode
    ///
    /// @brief  Query whether this is Torch Widget node
    ///
    /// @return TRUE if node is Torch widget node.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsTorchWidgetNode()
    {
        return (Torch == Type());
    }

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

        if (NULL != m_pPipeline)
        {
            isMultiCamera = m_pPipeline->IsMultiCamera();
        }

        return isMultiCamera;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetInputLink
    ///
    /// @brief  Fill input link information
    ///
    /// @param  nodeInputLinkIndex  Input link index to be set
    /// @param  nodeInputPortId     Port name/id (coming from topology XML)
    /// @param  pParentNode         Parent node pointer for input link
    /// @param  parentPortId        Parent port id for the input link
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetInputLink(
        UINT  nodeInputLinkIndex,
        UINT  nodeInputPortId,
        Node* pParentNode,
        UINT  parentPortId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupSourcePort
    ///
    /// @brief  Fill input source port information
    ///
    /// @param  nodeInputPortIndex  Input link index to be set
    /// @param  nodeInputPortId     Port name/id (coming from topology XML)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupSourcePort(
        UINT  nodeInputPortIndex,
        UINT  nodeInputPortId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUpLoopBackPorts
    ///
    /// @brief  Fill input link information
    ///
    /// @param  inputPortIndex  Input link index to be set
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetUpLoopBackPorts(
        UINT  inputPortIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NodeThreadJobFamilyCb
    ///
    /// @brief  Static Node worker function that will be called by the threadpool for jobs registered by the node
    ///
    /// @param  pCbData Thread payload, which is the node private data (including the node object)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* NodeThreadJobFamilyCb(
        VOID* pCbData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyInputConsumed
    ///
    /// @brief  Notification that the input port buffer has been consumed by some other node (who used it as input)
    ///         , applicable for bypassable custom node
    ///
    /// @param  inputPortIndex  Input port whose buffer has been consumed
    /// @param  requestId       RequestId for which the output port data is consumed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyInputConsumed(
        UINT   inputPortIndex,
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyOutputConsumed
    ///
    /// @brief  Notification that the output port buffer has been consumed by some other node (who used it as input)
    ///
    /// @param  outputPortIndex Output port whose buffer has been consumed
    /// @param  requestId       RequestId for which the output port data is consumed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyOutputConsumed(
        UINT   outputPortIndex,
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearDependencies
    ///
    /// @brief  Checks if the node depends on previous requests's output, if so, there might be dependencies holding a refcount
    ///         waiting for us to clear it. Also clear any dependencies for future requests that may never come, or come and
    ///         we have no valid data for them.
    ///
    /// @param  requestId       RequestId for the last valid request before flush was called
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ClearDependencies(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyAndReleaseFence
    ///
    /// @brief  Notification the fence for a particular port and try release it if the refcount is met
    ///
    /// @param  pOutputPort     Output port whose buffer has been consumed
    /// @param  requestId       RequestId for which the output port data is consumed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyAndReleaseFence(
        OutputPort* const   pOutputPort,
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BufferRequirementNotification
    ///
    /// @brief  Once a node determines the buffer requirement of its input ports, it calls this function of the parent node
    ///         (connected to its input ports) to inform the parent node of the its output buffer requirement
    ///
    /// @param  outputPortIndex    Pointer to array of device indices that would be added to access list for the output port
    /// @param  pBufferRequirement The number of valid entries in the device index array
    ///
    /// @return Success or Failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult BufferRequirementNotification(
        UINT               outputPortIndex,
        BufferRequirement* pBufferRequirement);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddOutputDeviceIndices
    ///
    /// @brief  Update output port device index list by adding the device index list passed
    ///
    /// @param  portId           Output port id
    /// @param  pDeviceIndices   Pointer to array of device indices that would be added to the access list for the output port
    /// @param  deviceIndexCount The number of valid entries in the device index array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AddOutputDeviceIndices(
        UINT         portId,
        const INT32* pDeviceIndices,
        UINT         deviceIndexCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddOutputNodes
    ///
    /// @brief  Update list of nodes connected to this output port
    ///
    /// @param  portId      Output port id
    /// @param  pDstNode    One of the Node that is connected to the ouput port of this node
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AddOutputNodes(
        UINT    portId,
        Node*   pDstNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TriggerOutputPortStreamIdSetup
    ///
    /// @brief  This function is called only for nodes with one or more sink ports. It is called by topology after all nodes
    ///         for a usecase are created. Function triggers a graph-walk-back starting with this node and going all the way
    ///         back to the head node of the graph. As it traverses back to the head node all the nodes visited in between
    ///         will be updated with the information of which output ports in them need to be enabled if this node's sink port
    ///         is active. For a particular capture request if this node's sink port is not active, other nodes will disable
    ///         their output ports that need to be enabled only if this node's sink port is active.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID TriggerOutputPortStreamIdSetup();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnableParentOutputPorts
    ///
    /// @brief  This function is called only if the node has loop back ports, and thus require its parent node output ports
    ///         to be always enabled by setting the stream mask.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID EnableParentOutputPorts();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckSourcePortBufferRequirements
    ///
    /// @brief  This function is invoked for nodes with source ports to trigger the buffer negotiation process. If this function
    ///         returns failure, then trigger the ReNegotiation to fallback to NV12 for Sink streams
    ///
    /// @return Success if input dimensions are within the limit, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckSourcePortBufferRequirements();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TriggerBufferNegotiation
    ///
    /// @brief  This function is invoked for nodes with sink ports to trigger the buffer negotiation process. If this function
    ///         returns failure, configure_streams will return failure
    ///
    /// @return Success if buffer negotiation succeeded, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult TriggerBufferNegotiation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TriggerInplaceProcessing
    ///
    /// @brief  This function is invoked for inplace nodes with SinkBuffer ports to inform its parent (and potentially its
    ///         parent) to output the HAL buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID TriggerInplaceProcessing();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetOutputPortEnabledForStreams
    ///
    /// @brief  Set streamId (which if active) would enable this output port. The stream(Id) itself may or may not be enabled
    ///         per request. So based on whether the streamId is enabled in a request or not, will decide whether this output
    ///         port is enabled for that request or not.
    ///
    /// @param  outputPortIndex  Output port index (in the output ports array)
    /// @param  streamIdMask     StreamIds which if active for a request will enable the output port for that request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetOutputPortEnabledForStreams(
        UINT outputPortIndex,
        UINT streamIdMask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStreamProcessType
    ///
    /// @brief  Get Stream process type enum corresponding to Output port Id
    ///
    /// @return Stream process tyep enum
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE StreamProcessType GetStreamProcessType()
    {
        return static_cast<StreamProcessType>(m_streamTypeBitmask);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateStreamTypeForAllNodes
    ///
    /// @brief  Prints stream type bitmask for each node recursively
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GenerateStreamTypeForAllNodes();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetJobFamilyHandle
    ///
    /// @brief  Get the job family handle to post jobs to the job family
    ///
    /// @return JobHandle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE JobHandle GetJobFamilyHandle() const
    {
        return m_hThreadJobFamilyHandle;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FinalizeInitialization
    ///
    /// @brief  Method to finalize the initialization of the node in the pipeline
    ///
    /// @param  pFinalizeInitializationData Finalize data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FinalizeInitialization(
        FinalizeInitializationData* pFinalizeInitializationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SwitchNodeOutputFormat
    ///
    /// @brief  Method to finalize the initialization of the node in the pipeline
    ///
    /// @param  format Format
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SwitchNodeOutputFormat(
        Format format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetBufferNegotiationData
    ///
    /// @brief  Reset buffer negotiation data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ResetBufferNegotiationData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillPipelineInputOptions
    ///
    /// @brief  Get the buffer requirements on the source input ports
    ///
    /// @param  pInputOptions   Buffer options on the input ports
    ///
    /// @return number of inputs filled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT FillPipelineInputOptions(
        ChiPipelineInputOptions* pInputOptions);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EarlyReturnTag
    ///
    /// @brief  Determins if the tag is one of the speical early return tags
    ///
    /// @param  requestId   requestId
    /// @param  tag         Tag to check
    /// @param  pData       data to write
    /// @param  size        size
    ///
    /// @return TRUE if it is an early return tag
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL EarlyReturnTag(
        UINT64       requestId,
        UINT32       tag,
        const VOID*  pData,
        SIZE_T       size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDataCountFromPipeline
    ///
    /// @brief  Fetches tag count data pools for the provided data id
    ///
    /// @param  id          Tag to fetch count for
    /// @param  offset      Offset from the current request to read
    /// @param  pipelineID  The pipeline ID indicating the pipeline that owns the pool
    /// @param  allowSticky For input pool, allow use of sticky tag data, or force use of count in request metadata
    ///
    /// @return Count of tag
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SIZE_T GetDataCountFromPipeline(
        const UINT   id,
        INT64        offset,
        UINT         pipelineID,
        BOOL         allowSticky);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDataListFromPipeline
    ///
    /// @brief  Fetches data pointers from data pools for the provided data ids
    ///
    /// @param  pDataList   Array of data identifiers to be updated
    /// @param  ppData      Array of data to update in the pool
    /// @param  pOffsets    Array of offsets from the current requestID to determine slot from which data pointer is obtained
    /// @param  length      Length of all the arrays
    /// @param  pNegate     Array of negate flags to indicate if the value of offsets are negative
    /// @param  pipelineID  The pipeline ID indicating the pipeline that owns the pool
    ///
    /// @return Result of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult GetDataListFromPipeline(
        const UINT*  pDataList,
        VOID** const ppData,
        UINT64*      pOffsets,
        SIZE_T       length,
        BOOL*        pNegate,
        UINT         pipelineID)
    {
        return GetDataListFromMetadataPool(pDataList, ppData, pOffsets, length, pNegate, pipelineID, InvalidCameraId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDataListFromPipelineByCameraId
    ///
    /// @brief  Fetches data pointers from data pools for the provided data ids. Camera id must be set while
    ///         posting metadata
    ///
    /// @param  pDataList   Array of data identifiers to be updated
    /// @param  ppData      Array of data to update in the pool
    /// @param  pOffsets    Array of offsets from the current requestID to determine slot from which data pointer is obtained
    /// @param  length      Length of all the arrays
    /// @param  pNegate     Array of negate flags to indicate if the value of offsets are negative
    /// @param  pipelineID  The pipeline ID indicating the pipeline that owns the pool
    /// @param  cameraId    CameraId of the metadata from which the list of tags must be fetched. InvalidCameraId if dont care
    ///
    /// @return Result of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult GetDataListFromPipelineByCameraId(
        const UINT*  pDataList,
        VOID** const ppData,
        UINT64*      pOffsets,
        SIZE_T       length,
        BOOL*        pNegate,
        UINT         pipelineID,
        UINT32       cameraId)
    {
        return GetDataListFromMetadataPool(pDataList, ppData, pOffsets, length, pNegate, pipelineID, cameraId);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDataList
    ///
    /// @brief  Fetches data pointers from data pools for the provided data ids
    ///
    /// @param  pDataList Array of data identifiers to be updated
    /// @param  ppData    Array of data to update in the pool
    /// @param  pOffsets  Array of offsets from the current requestID to determine slot from which data pointer is obtained
    /// @param  length    Length of all the arrays
    ///
    /// @return Result of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult GetDataList(
        const UINT*  pDataList,
        VOID** const ppData,
        UINT64*      pOffsets,
        SIZE_T       length)
    {
        return GetDataListFromMetadataPool(pDataList, ppData, pOffsets, length, NULL, GetPipeline()->GetPipelineId(),
                                           GetCameraIdForMetadataQuery());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorModeData
    ///
    /// @brief  Fetches sensor mode
    ///
    /// @param  ppSensorData Pointer to fill with lookup
    ///
    /// @return Result of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetSensorModeData(
        const SensorMode** ppSensorData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorModeRes0Data
    ///
    /// @brief  Fetches sensor mode
    ///
    /// @param  ppSensorData Pointer to fill with lookup
    ///
    /// @return Result of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetSensorModeRes0Data(
        const SensorMode** ppSensorData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraConfiguration
    ///
    /// @brief  Fetches Camera Configuration
    ///
    /// @param  ppCameraConfigInfo Pointer to fill with lookup
    ///
    /// @return Result of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCameraConfiguration(
        const CameraConfigurationInformation** ppCameraConfigInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIFEInputResolution
    ///
    /// @brief  Fetches IFE input Resoulution
    ///
    /// @param  ppIFEInput Pointer to fill with lookup
    ///
    /// @return Result of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetIFEInputResolution(
        const IFEInputResolution** ppIFEInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSIDBinningInfo
    ///
    /// @brief  Get IFE CSID binning information
    ///
    /// @param  pCSIDBinningInfo Pointer to CSIDBinningInfo
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetCSIDBinningInfo(
        CSIDBinningInfo* pCSIDBinningInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPreviewDimension
    ///
    /// @brief  Retrieve the preview dimension
    ///
    /// @param  pPreviewDimension  Output pointer to preview dimension
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPreviewDimension(
        CamxDimension* pPreviewDimension);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WritePreviousData
    ///
    /// @brief  Copies tag/prop from request - 1 to request
    ///
    /// @param  dataId    Tag/Prop to update
    /// @param  size      size of tag data to copy
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID WritePreviousData(
        UINT32 dataId,
        UINT   size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteDataList
    ///
    /// @brief  Updates data in the data pools
    ///
    /// @param  pDataList Array of data identifiers to be updated
    /// @param  ppData    Array of data to update in the pool
    /// @param  pDataSize Array of sizes of the data in pData
    /// @param  length    Length of all the arrays
    ///
    /// @return Result of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WriteDataList(
        const UINT*  pDataList,
        const VOID** ppData,
        const UINT*  pDataSize,
        SIZE_T       length);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndUpdatePublishedPartialList
    ///
    /// @brief  Checks if the Tag ID is Partial and if so updates the publish list
    ///
    /// @param  tagID Tag ID which we are putting in the metadata
    ///
    /// @return Result of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckAndUpdatePublishedPartialList(
        const UINT tagID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyNodeCSLMessage
    ///
    /// @brief  Base Method to notify CSL message to nodes
    ///
    /// @param  pCSLMessage CSL message data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult NotifyNodeCSLMessage(
        CSLMessage* pCSLMessage)
    {
        m_tRequestId = pCSLMessage->message.frameMessage.requestID;

        return NotifyCSLMessage(pCSLMessage);
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyCSLMessage
    ///
    /// @brief  Method to notify CSL message to nodes
    ///
    /// @param  pCSLMessage CSL message data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult NotifyCSLMessage(
        CSLMessage* pCSLMessage)
    {
        CAMX_UNREFERENCED_PARAM(pCSLMessage);

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPerFramePool
    ///
    /// @brief  Gets the pointer to the per frame property/metadata pool
    ///
    /// @param  poolType Type of the per-frame pool asked for
    ///
    /// @return Pointer to the per frame property/metadata pool
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MetadataPool* GetPerFramePool(
        PoolType poolType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentRequest
    ///
    /// @brief  Gets the id of the request being processed
    ///
    /// @note   Should only call this within the ProcessRequest call stack (including ExecuteProcessRequest)
    ///
    /// @return Value of current request
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT64 GetCurrentRequest() const
    {
        return m_tRequestId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineId
    ///
    /// @brief  Gets the id of node's owning pipeline
    ///
    /// @return id of pipeline
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetPipelineId()
    {
        return m_pPipeline->GetPipelineId();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticSettings
    ///
    /// @brief  Helper method to return the settings pointer.
    ///
    /// @return HwContext*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const StaticSettings* GetStaticSettings() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPipelineStreamedOn
    ///
    /// @brief  Utility to check if the pipeline has been streamed on
    ///
    /// @return TRUE/FALSE based on stream on state
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPipelineStreamedOn();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BypassNodeProcessing
    ///
    /// @brief  Process Bypass node
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID BypassNodeProcessing();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsLoopBackNode
    ///
    /// @brief  Check if node has loop back ports
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsLoopBackNode()
    {
        return m_bHasLoopBackPorts;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpState
    ///
    /// @brief  Dumps snapshot of current state to a file
    ///
    /// @param  fd          file descriptor
    /// @param  indent      indent spaces.
    /// @param  requestId   requestId.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpState(
        INT     fd,
        UINT32  indent,
        UINT64  requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpNodeInfo
    ///
    /// @brief  Dumps static Node info
    ///
    /// @param  fd      file descriptor
    /// @param  indent  indent spaces.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpNodeInfo(
        INT     fd,
        UINT32  indent);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoDumpNodeInfo
    ///
    /// @brief  Dumps Node info per node type
    ///
    /// @param  fd      file descriptor
    /// @param  indent  indent spaces.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult DoDumpNodeInfo(
        INT     fd,
        UINT32  indent)
    {
        CAMX_UNREFERENCED_PARAM(fd);
        CAMX_UNREFERENCED_PARAM(indent);

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpFenceErrors
    ///
    /// @brief  Dumps Information related to fence errors created in fence callback
    ///
    /// @param  fd          File descriptor
    /// @param  indent      Indent spaces
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpFenceErrors(
        INT     fd,
        UINT32  indent);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpLinkInfo
    ///
    /// @brief  Dumps static Node link info
    ///
    /// @param  fd      file descriptor
    /// @param  indent  indent spaces.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpLinkInfo(
        INT     fd,
        UINT32  indent);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpDebugNodeInfo
    ///
    /// @brief  Dump Node data
    ///
    /// @param  requestId   requestID for dump
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpDebugNodeInfo(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpDebugInternalNodeInfo
    ///
    /// @brief  Dump node specific debug info
    ///
    /// @param  requestId       requestId of register dump
    /// @param  isPerFrameDump  per frame register dump indicator
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID DumpDebugInternalNodeInfo(
        UINT64  requestId,
        BOOL    isPerFrameDump)
    {
        CAMX_UNREFERENCED_PARAM(requestId);
        CAMX_UNREFERENCED_PARAM(isPerFrameDump);
        return;
    }


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CanDRQPreemptOnStopRecording
    ///
    /// @brief  Check if DRQ can preempt EPR for this node on stop recording
    ///
    /// @return TRUE if enabled, FALSE if disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL CanDRQPreemptOnStopRecording() const
    {
        return m_nodeFlags.canDRQPreemptOnStopRecording;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCSLSyncId
    ///
    /// @brief  Function to return CSL sync ID per request
    ///
    /// @param  requestId requestId to match csl sync id
    /// @param  syncId    CSL sync id.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetCSLSyncId(
        UINT64 requestId,
        UINT64 syncId)
    {
        CamxAtomicStoreU64(&m_CSLSyncID[requestId % MaxRequestBufferDepth], syncId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLSyncId
    ///
    /// @brief  Function to return CSL sync ID per request
    ///
    /// @param  requestId requestId to match csl sync id
    ///
    /// @return syncID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT64 GetCSLSyncId(
        UINT64 requestId)
    {
        UINT64 syncid = CamxAtomicLoadU64(&m_CSLSyncID[requestId % MaxRequestBufferDepth]);

        if (CamxInvalidRequestId == syncid || CamxInvalidRequestId == requestId)
        {
            syncid = requestId;
        }

        return syncid;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNodeCompleteProperty
    ///
    /// @brief  Utility to compute property updated at the end of request processing
    ///
    /// @return PropertyID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetNodeCompleteProperty()
    {
        CAMX_ASSERT(NodeCompleteCount > m_pipelineNodeIndex);

        return PropertyIDNodeComplete0 + m_pipelineNodeIndex;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsEarlyPCREnabled
    ///
    /// @brief  Read vendor tag and staticSettings to enable/disable earlyPCR
    ///
    /// @param  pIsEarlyPCREnabled Pointer to fill with early PCR enable value
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult IsEarlyPCREnabled(
        BOOL* pIsEarlyPCREnabled);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DisablePerRequestOutputPortLink
    ///
    /// @brief  Disable the perRequest OutputPort links for the given Node
    ///
    /// @param  requestId requestId for which to Disable output port link
    ///
    /// @return TRUE if port status got updated otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL DisablePerRequestOutputPortLink(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPortStatusUpdatedByOverride
    ///
    /// @brief  Enable Disable Node IO ports as per hardware limitations
    ///
    /// @return TRUE if port status got updated otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPortStatusUpdatedByOverride();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPacket
    ///
    /// @brief  Helper method to acquire a free packet from the given manager.
    ///
    /// @param  pCmdBufferManager   Command buffer manager from which a buffer is acquired
    ///
    /// @return Pointer to a Packet object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Packet* GetPacket(
    CmdBufferManager* pCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPacketForRequest
    ///
    /// @brief  Helper method to acquire a free packet from the given manager.
    ///
    /// @param  requestId           The request Id to which this resource's lifecycle will be tied to
    /// @param  pCmdBufferManager   Command buffer manager from which a buffer is acquired
    ///
    /// @return Pointer to a Packet object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Packet* GetPacketForRequest(
        UINT64              requestId,
        CmdBufferManager*   pCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCmdBuffer
    ///
    /// @brief  Helper method to acquire a free command buffer from the given manager.
    ///
    /// @param  pCmdBufferManager   Command buffer manager from which a buffer is acquired
    ///
    /// @return Pointer to a CmdBuffer object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CmdBuffer* GetCmdBuffer(
        CmdBufferManager* pCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCmdBufferForRequest
    ///
    /// @brief  Helper method to acquire a free command buffer from the given manager.
    ///
    /// @param  requestId           The request Id to which this resource's lifecycle will be tied to
    /// @param  pCmdBufferManager   Command buffer manager from which a buffer is acquired
    ///
    /// @return Pointer to a CmdBuffer object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CmdBuffer* GetCmdBufferForRequest(
        UINT64              requestId,
        CmdBufferManager*   pCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataPublishList
    ///
    /// @brief  Method to get the publish list from the node
    ///
    /// @param  ppTagArray        Pointer to the array of tags published by the node
    /// @param  pTagCount         Pointer to the count of tags published by the node
    /// @param  pPartialTagCount  Pointer to the count of partial tags published by the node
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetMetadataPublishList(
        const UINT32** ppTagArray,
        UINT32*        pTagCount,
        UINT32*        pPartialTagCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestIdOffsetFromLastFlush
    ///
    /// @brief  Method to query the request id offset from last frame which has been flushed
    ///
    /// @param  requestId   current requestId
    ///
    /// @return requestId offset from last frame which has been flushed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 GetRequestIdOffsetFromLastFlush(
        UINT64 requestId) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaximumPipelineDelay
    ///
    /// @brief  Method to get the pipelineDelay
    ///
    /// @return Pipeline Delay
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetMaximumPipelineDelay() const
    {
        return m_pPipeline->GetMaxPipelineDelay();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPipelineWarmedUpState
    ///
    /// @brief  Method to check if the pipeline is in warmed up state
    ///
    /// @param  requestId   current requestId
    ///
    /// @return TRUE if the last request ID offset from flush is greater than the maximum delay, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPipelineWarmedUpState(
        UINT64 requestId) const
    {
        return GetMaximumPipelineDelay() < GetRequestIdOffsetFromLastFlush(requestId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFenceErrorBuffer
    ///
    /// @brief  Gets a fence error structure
    ///
    /// @return Pointer to the fence error structure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE NodeFenceHandlerDataErrorSubset* GetFenceErrorBuffer()
    {
        NodeFenceHandlerDataErrorSubset* pFenceError;

        pFenceError             = &m_fenceErrors.fenceErrorData[m_fenceErrors.freeIndex];
        m_fenceErrors.freeIndex = ((m_fenceErrors.freeIndex + 1) % MaxFenceErrorBufferDepth);

        return pFenceError;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLSession
    ///
    /// @brief  Get CSL session handle for the node
    ///
    /// @return csl session handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CSLHandle GetCSLSession()
    {
        return m_pPipeline->GetCSLSession();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFullRecoveryFlagSet
    ///
    /// @brief  Full recovery flag set or not
    ///
    /// @return True if FullRecovery flag is set
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsFullRecoveryFlagSet()
    {
        return m_pPipeline->IsFullRecoveryFlagSet();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief  Method to release/clear the resources that node might have allocated/reserved for a request
    ///
    /// @param  requestId Request for which cmdbuffers are complete
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Flush(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFlushResponseTimeInMs
    ///
    /// @brief  Return worst-case response time of the node for flush call, defaults to the nodeResponseTime setting.
    ///
    /// @return worst-case response time in milliseconds.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE virtual UINT64 GetFlushResponseTimeInMs()
    {
        return HwEnvironment::GetInstance()->GetStaticSettings()->nodeResponseTime;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCurrentlyProcessing
    ///
    /// @brief  Checks whether this node is currently doing work. This gets set to true when ExecuteProcessRequest gets called,
    ///         and gets set to FALSE after the node finishes processing (output is produced or it gets flushed). This is a
    ///         thread safe call.
    ///
    /// @return TRUE if the node is currently doing work, FALSE otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsCurrentlyProcessing();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializePSMetadata
    ///
    /// @brief  Method to trigger port specific metadata initialization
    ///
    /// @param  tagId        Tag Identifier
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializePSMetadata(
        UINT32 tagId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeInitializePSMetadata
    ///
    /// @brief  Method to free all PS metadata memory allocations
    ///
    /// @param  tagId        Tag Identifier
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DeInitializePSMetadata(
        UINT32 tagId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPortsWithSinkBuffer
    ///
    /// @brief  Function to get the list of output ports that generate sink buffer
    ///
    /// @param  outputPorts Vector of output ports object pointers
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID GetOutputPortsWithSinkBuffer(
        std::vector<OutputPort*>& outputPorts)
    {
        for (UINT portIndex = 0; portIndex < m_outputPortsData.numPorts; ++portIndex)
        {
            OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[portIndex];

            if (TRUE == pOutputPort->flags.isSinkBuffer)
            {
                outputPorts.push_back(pOutputPort);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPSMetaByRequestId
    ///
    /// @brief  Gets port-specific metadata correponding to output port given requestId
    ///
    /// @param  outputPortIndex  Output port index
    /// @param  tagId            Metadata identifiers to be updated
    /// @param  requestId        RequestId corresponding to the call
    /// @param  ppData           Array of data to update in the pool
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult GetPSMetaByRequestId(
        UINT         outputPortIndex,
        UINT         tagId,
        UINT64       requestId,
        VOID** const ppData)
    {
        CamxResult result   = CamxResultENoSuch;
        UINT       tagIndex = GetPSMetaIndex(tagId);

        if ((NULL != m_pPSMQueue) && (NULL != ppData) && (MaxPSMetaType > tagIndex))
        {
            if (outputPortIndex < m_outputPortsData.numPorts)
            {
                PSMQueue* pQueue       = &m_pPSMQueue[outputPortIndex];
                UINT      requestIndex = requestId % pQueue->maxSize;

                *ppData = pQueue->pEntry[requestIndex].pPSMData[tagIndex];
                result  = CamxResultSuccess;
            }
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTagArrayIndicesFromInputMetadata
    ///
    /// @brief  Gets array index of the tag in the payload corresponding to the metadata
    ///
    /// @param  tagId             Metadata identifiers to be updated
    /// @param  requestId         RequestId corresponding to the call
    /// @param  pSourceStream     Pointer to the source stream
    /// @param  rIndex            Index of the tag data in the array
    /// @param  rRequestCropIndex Index of crop entry in request crop info payload
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetTagArrayIndicesFromInputMetadata(
        UINT   tagId,
        UINT64 requestId,
        VOID*  pSourceStream,
        UINT&  rIndex,
        UINT&  rRequestCropIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllAncestorNodes
    ///
    /// @brief  Get the list of all ancestors that publishes the given tag sorted in descending order
    ///
    /// @param  outputPortIndex           Output port for which ancestor node is required
    /// @param  tagId                     Data type ID for which ancestor node is required
    /// @param  requestId                 RequestId corresponding to the call. If 0 is passed, default list is passed
    /// @param  ancestorList              List of ancestors
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAllAncestorNodes(
        UINT                                outputPortIndex,
        UINT                                tagId,
        UINT64                              requestId,
        std::vector<UniqueNodePortInfo>&    ancestorList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAncestorNodePortInfo
    ///
    /// @brief  Get node-port-instance info for ancestor node that publishes the given tagId
    ///
    /// @param  outputPortIndex           Output port for which ancestor node information is required
    /// @param  tagId                     Tag Identifier corresponding to the given request
    /// @param  ancestorList              Vector of ancestors
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAncestorNodePortInfo(
        UINT                                outputPortIndex,
        UINT                                tagId,
        std::vector<UniqueNodePortInfo>&    ancestorList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetActiveAncestorNodePortInfo
    ///
    /// @brief  Get node-port-instance info for the active ancestor node that publishes the given tagId
    ///
    /// @param  outputPortIndex           Output port for which ancestor node is required
    /// @param  tagId                     Data type ID for which ancestor node is required
    /// @param  requestId                 Request Identifier
    /// @param  ancestorList              Vector of active ancestors
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetActiveAncestorNodePortInfo(
        UINT                             outputPortIndex,
        UINT                             tagId,
        UINT64                           requestId,
        std::vector<UniqueNodePortInfo>& ancestorList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllStreamCropInfo
    ///
    /// @brief  Gets a list of crop info correponding to all nodes in that stream
    ///
    /// @param  inputPortIndex      Input port index of current node
    /// @param  offset              Offset from the current requestID to determine slot from which data pointer is obtained
    /// @param  ppData              Array of data to update in the pool
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAllStreamCropInfo(
        UINT                            inputPortIndex,
        UINT64                          offset,
        VOID** const                    ppData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSourceNodePortInfo
    ///
    /// @brief  Gets NodePortInfo of the first node in given pipeline
    ///
    /// @param  inputPortIndex        Input port index of given node
    /// @param  rNodePortInfo         NodePortInfo of the first node in given pipeline
    /// @param  rSourceInputPortIndex Input port index of the first node in given pipeline
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetSourceNodePortInfo(
        UINT                inputPortIndex,
        UniqueNodePortInfo& rNodePortInfo,
        UINT&               rSourceInputPortIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPreviousPipelineMeta
    ///
    /// @brief  Gets meta info from previous pipeline
    ///
    /// @param  inputPortIndex Input port index corresponding to first node in given pipeline
    /// @param  tagId          Tag ID for which previous pipleline meta info is required
    /// @param  requestId      RequestId corresponding to the call. If 0 is passed, default list is passed
    /// @param  ppData         Data pointer previous pipleline meta info
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPreviousPipelineMeta(
        UINT   inputPortIndex,
        UINT   tagId,
        UINT64 requestId,
        VOID** ppData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateRequestCropInfo
    ///
    /// @brief  Fill RequestCropInfo payload structure with all ancestor stream crop info
    ///
    /// @param  outputPortIndex     Output port index for first ancestor node
    /// @param  tagId               Metadata identifiers to be updated
    /// @param  requestId           RequestId corresponding to the call
    /// @param  rRequestCropInfo    RequestCropInfo structure to be populated
    /// @param  rRequestCropIndex   Index of crop entry in request crop info payload
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulateRequestCropInfo(
        UINT                outputPortIndex,
        UINT                tagId,
        UINT64              requestId,
        RequestCropInfo&    rRequestCropInfo,
        UINT&               rRequestCropIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetParentSinkPortWithHALBuffer
    ///
    /// @brief  Get node-port-instance info for the active ancestor node that publishes the given tagId
    ///
    /// @param  outputPortIndex           Output port for which ancestor node is required
    /// @param  requestId                 Request Identifier
    /// @param  sourceStreamPortList      Vector of active input port information
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetParentSinkPortWithHALBuffer(
        UINT                                   outputPortIndex,
        UINT64                                 requestId,
        std::vector<PerRequestInputPortInfo*>& sourceStreamPortList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestStatus
    ///
    /// @brief  Helper function to check whether node request is completed or pending, It returns status of the request.
    ///
    /// @param  requestId Request Id whose status is logged
    ///
    /// @return the status of the request
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE PerRequestNodeStatus GetRequestStatus(
        UINT64 requestId)
    {
        return static_cast<PerRequestNodeStatus>(
            CamxAtomicLoadU(reinterpret_cast<UINT*>(&m_perRequestInfo[requestId % MaxRequestQueueDepth].requestStatus)));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetRequestStatus
    ///
    /// @brief  Helper function to set the status of the request.
    ///
    /// @param  requestId Request Id whose status is logged
    /// @param  reqStatus Status of the request to swtich to
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetRequestStatus(
        UINT64               requestId,
        PerRequestNodeStatus reqStatus)
    {
        if (PerRequestNodeStatus::Success == GetRequestStatus(requestId) && PerRequestNodeStatus::Success == reqStatus)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "%s, Id: %d is submitted to HW twice", NodeIdentifierString(), requestId);
            OsUtils::RaiseSignalAbort();
        }
        else
        {
            CamxAtomicStoreU(reinterpret_cast<UINT*>(&m_perRequestInfo[requestId % MaxRequestQueueDepth].requestStatus),
                static_cast<UINT>(reqStatus));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetNodeProcessingTime
    ///
    /// @brief  Set node processing timestamp in m_perRequestInfo
    ///
    /// @param  requestIdIndex      requestIdIndex to index m_perRequestInfo
    /// @param  sequenceId          process sequence id of request
    /// @param  stage               processing stage we want to set the timestamp for (start, dependencies met, end)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetNodeProcessingTime(
        UINT64      requestIdIndex,
        INT32       sequenceId,
        NodeStage   stage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpNodeProcessingTime
    ///
    /// @brief  Method to dump time it took for a node to process for a given requestId
    ///
    /// @param  requestId   Request to dump processing time for
    /// @param  milliSec    The time in milliseconds when this function is called
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpNodeProcessingTime(
        UINT64 requestId,
        UINT32 milliSec);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpNodeProcessingAverage
    ///
    /// @brief  Method to dump average node processing time (average based on requestqueuedepth)
    ///
    /// @param  requestId   RequestId to start index from for average
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpNodeProcessingAverage(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QuerySupportedPSMetadataList
    ///
    /// @brief  Gets a list of port specific metadata supported by the Driver
    ///
    /// @param  pMetadataIdArray   Pointer to a structure that defines the array of metadata Ids
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult QuerySupportedPSMetadataList(
        ChiMetadataIdArray* pMetadataIdArray);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpSequenceIdProcessingAverage
    ///
    /// @brief  Method to dump the average time the node is taking to process each sequence id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpSequenceIdProcessingAverage();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDeferPipelineCreate
    ///
    /// @brief  Check if node need to defer PostPipelineCreate
    ///
    /// @return TRUE/FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsDeferPipelineCreate()
    {
        return m_nodeFlags.isDeferNotifyPipelineCreate;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeferPipelineCreate
    ///
    /// @brief  Defer to call PostPipelineCreate function.
    ///
    /// @param  pData    Node data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* DeferPipelineCreate(
        VOID* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitForDeferPipelineCreate
    ///
    /// @brief  Wait for function DeferPipelineCreate finish
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID WaitForDeferPipelineCreate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPSMEnabled
    ///
    /// @brief  Check if PSM is enabled for given tagId
    ///
    /// @param  tagId Tag Identifier
    ///
    /// @return TRUE if PSM enabled for given tagId else return FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPSMEnabled(
        UINT32 tagId)
    {
        BOOL enabled = FALSE;
        UINT tagIndex = GetPSMetaIndex(tagId);
        if ((tagIndex < MaxPSMetaType) && (TRUE == m_pPSMData[tagIndex].canPublish))
        {
            enabled =  TRUE;
        }
        return enabled;
    }

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
        if (TRUE == ImageFormatUtils::IsUBWC(pFormat->format))
        {
            pFormatParamInfo->LossyUBWCConsumerUsage   &= 0x1;
            pFormatParamInfo->UBWCVersionConsumerUsage &= 0xFFFF;
        }
        else
        {
            pFormatParamInfo->LossyUBWCConsumerUsage   &= 0;
            pFormatParamInfo->UBWCVersionConsumerUsage &= 0;
        }
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
        CAMX_UNREFERENCED_PARAM(pFormat);
        CAMX_UNREFERENCED_PARAM(pFormatParamInfo);

        pFormatParamInfo->LossyUBWCProducerUsage   = 0;
        pFormatParamInfo->UBWCVersionProducerUsage = 0;
        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPLMetaPortGroups
    ///
    /// @brief   Function to get the port groups that pubhishes the tagId. Each group contains list of portIds
    ///          that belong to that group
    ///
    /// @param tagId             Tag Identifier
    /// @param plmGroups         Output Vector of list of portId groups
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetPLMetaPortGroups(
        UINT                            tagId,
        std::vector<std::vector<UINT>>& plmGroups);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUsageFlagsInBypass
    ///
    /// @brief  Set Producer format param info also checks if node is bypassable
    ///
    /// @param  pOutputPort         pointer to output port
    /// @param  pFormat             Format pointer
    /// @param  pFormatParamInfo    pointer to FormatParamInfo
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetUsageFlagsInBypass(
        OutputPort*      pOutputPort,
        ImageFormat*     pFormat,
        FormatParamInfo* pFormatParamInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HasParentNode
    ///
    /// @brief  Return parent if node has a parent
    ///
    /// @param  inputPortId input port id
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT HasParentNode(
        UINT inputPortId
    ) const
    {
        BOOL hasParent = FALSE;
        UINT portIndex = 0;

        for (portIndex = 0; portIndex < m_inputPortsData.numPorts; portIndex++)
        {
            if ((inputPortId == m_inputPortsData.pInputPorts[portIndex].portId) &&
                (NULL != m_inputPortsData.pInputPorts[portIndex].pParentNode))
            {
                hasParent = TRUE;
                break;
            }
        }
        return hasParent;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTuningDataManager
    ///
    /// @brief  Gets the pointer to the pipeline
    ///
    /// @return Pointer to the thread pool manager
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TuningDataManager* GetTuningDataManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTuningDataManagerWithCameraId
    ///
    /// @brief  Gets the pointer to the pipeline
    ///
    /// @param  cameraId  camera id
    ///
    /// @return Pointer to the thread pool manager
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TuningDataManager* GetTuningDataManagerWithCameraId(
    UINT32 cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsRealTime
    ///
    /// @brief  Method to query if node is part of realtime pipeline
    ///
    /// @return TRUE or FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsRealTime() const
    {
        return m_nodeFlags.isRealTime;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDeviceAcquired
    ///
    /// @brief  Check if any hw device is acquired
    ///
    /// @return TRUE if hw device is successfully acquired, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsDeviceAcquired() const
    {
        return m_nodeFlags.isHwDeviceAcquired;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDeviceAcquired
    ///
    /// @brief  Set hw device acquire state
    ///
    /// @param  isAcquired acquire state to be set
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetDeviceAcquired(
        BOOL isAcquired)
    {
        m_nodeFlags.isHwDeviceAcquired = (TRUE == isAcquired) ? 1 : 0;
    }

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
        FinalizeInitializationData* pFinalizeInitializationData)
    {
        CAMX_UNREFERENCED_PARAM(pFinalizeInitializationData);

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHwContext
    ///
    /// @brief  Helper method to return the HW context.
    ///
    /// @return HwContext*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE HwContext* GetHwContext() const
    {
        return m_pHwContext;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiContext
    ///
    /// @brief  Helper method to return the Chi context.
    ///
    /// @return ChiContext*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ChiContext* GetChiContext() const
    {
        return m_pChiContext;;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeInitialize
    ///
    /// @brief  Initialize the hwl or sw object
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
    /// ProcessingNodeFinalizeInputRequirement
    ///
    /// @brief  Implemented by derived nodes to determine its input buffer requirement based on all the output buffer
    ///         requirements
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
        BufferNegotiationData* pBufferNegotiationData)
    {
        // A node may not necessarily need to do any work here if:
        //
        // 1. A node with sink no buffer ports only
        // 2. A node with all output ports as sink buffers

        CAMX_UNREFERENCED_PARAM(pBufferNegotiationData);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessInputBufferRequirement
    ///
    /// @brief  This function is called after all the buffer negotiation data is available for the node. It will call the
    ///         derived node to compute the final input buffer requirement and output port buffer dimensions. It then informs
    ///         the parent nodes of the input buffer requirements
    ///
    /// @return Success or Failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessInputBufferRequirement();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostPipelineCreate
    ///
    /// @brief  virtual method to be called at NotifyPipelineCreated time; node should take care of updates and initialize
    ///         blocks that has dependency on other nodes in the topology at this time.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PostPipelineCreate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllOutputPortIds
    ///
    /// @brief  Get all the output port Ids of the node which are active
    ///
    /// @param  pNumPorts Number of output ports to be filled in by this function
    /// @param  pPortIds  Array of port Ids filled in by this function (array allocated by caller)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAllOutputPortIds(
        UINT* pNumPorts,
        UINT* pPortIds);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetSecureMode
    ///
    /// @brief  Resets Secure mode of port
    ///
    /// @param  portId  PortId
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ResetSecureMode(
        UINT32 portId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllInputPortIds
    ///
    /// @brief  Get all the input port Ids of the node
    ///
    /// @param  pNumPorts Number of input ports to be filled in by this function
    /// @param  pPortIds  Array of port Ids filled in by this function (array allocated by caller)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAllInputPortIds(
        UINT* pNumPorts,
        UINT* pPortIds);

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
    virtual CamxResult PrepareStreamOn()
    {
        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnStreamOn
    ///
    /// @brief  virtual method to that will be called after streamOn command is sent to HW. HW nodes may use
    ///         this hook to do any stream on configuration. This is generally called everytime ActivatePipeline is called.
    ///         Nodes may use this to setup things that are required while streaming. For exa, any resources that are needed
    ///         only during streaming can be allocated here. Make sure to do light weight operations here as this might delay
    ///         processing of the first request.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult OnStreamOn()
    {
        return CamxResultSuccess;
    }

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
        CHIDEACTIVATEPIPELINEMODE modeBitmask)
    {
        CAMX_UNREFERENCED_PARAM(modeBitmask);

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireResources
    ///
    /// @brief  virtual method to that will be called before streamOn command is sent to HW. HW nodes may use
    ///         this hook to do any stream on configuration. This is generally called everytime ActivatePipeline is called.
    ///         Nodes may use this to setup things that are required while streaming. For exa, any resources that are needed
    ///         only during streaming can be allocated here. Make sure to do light weight operations here as this might delay
    ///         processing of the first request.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult AcquireResources()
    {
        return CamxResultSuccess;
    }

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
        CHIDEACTIVATEPIPELINEMODE modeBitmask)
    {
        CAMX_UNREFERENCED_PARAM(modeBitmask);

        return CamxResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNodeDisabledWithOverride
    ///
    /// @brief  virtual method that will be called during NewActiveStreamsSetup. Nodes may use
    ///         this hook to disable processing if theyre disabled through settings.
    ///
    /// @return TRUE if Node got disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsNodeDisabledWithOverride();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSupportedPortConfiguration
    ///
    /// @brief  virtual method that will be called prior to setup request to update port status. Nodes may use this to
    ///         enable disable ports dynamically
    ///
    /// @param  portId  port Id to check
    ///
    /// @return TRUE if port status got updated
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsSupportedPortConfiguration(
        UINT portId)
    {
        CAMX_UNREFERENCED_PARAM(portId);

        return TRUE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateNodeParamsOnPortStatus
    ///
    /// @brief  virtual method that will be called prior to setup request to update node params on port status.
    ///         Nodes may use this to update their port dependent parameters
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID UpdateNodeParamsOnPortStatus()
    {
        return;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCorrespondingPassInputPortForInputRefPort
    ///
    /// @brief  Helper method to get the corresponding InputPort for the input reference port
    ///
    /// @param  inputRefPortId Input reference port id to check
    ///
    /// @return Input port Id corresponsing to the reference input port
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT GetCorrespondingPassInputPortForInputRefPort(
        UINT inputRefPortId
    ) const
    {
        return inputRefPortId;
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
        return inputPortId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCorrespondingInputPortForOutputPort
    ///
    /// @brief  method to get the corresponding Input Port for the output port.
    ///
    /// @param  outputPortId Output port Id to check
    /// @param  pInputPortId to update
    ///
    /// @return True if the Input port matched the corresponding output port
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL GetCorrespondingInputPortForOutputPort(
        UINT outputPortId,
        UINT* pInputPortId
    ) const
    {
        // Each node need to define this virtual function and return the input port to be dumped.
        // send FALSE from here.
        if (NULL != pInputPortId)
        {
            *pInputPortId = outputPortId;
        }
        return FALSE;
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
        // Each node need to define this virtual function and return the input port to be dumped.
        // send FALSE from here.
        CAMX_UNREFERENCED_PARAM(outputPortId);
        BOOL result = FALSE;

        result = ((0 == unSignaledFenceCount) ? TRUE : FALSE);

        return result;
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
        CAMX_UNREFERENCED_PARAM(portId);

        return " ";
    }

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
        CAMX_UNREFERENCED_PARAM(portId);

        return " ";
    }

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
        ExecuteProcessRequestData* pExecuteProcessRequestData) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeCmdBufferManagerList
    ///
    /// @brief  Helper method to allow HW subnodes to initialize command manager store based on their requirements.
    ///
    /// @param  maxNumManagers  Max number of managers that may be requested throughout the nodes' lifetime
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeCmdBufferManagerList(
        UINT maxNumManagers);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdBufferManager
    ///
    /// @brief  A helper function to allow sub-classes to create and add command buffer managers that parent node will manage.
    ///
    /// @param  pBufferManagerName  Cmd Buffer Manager name
    /// @param  pParams             Parameters the manager was created with.
    /// @param  ppCmdBufferManager  Pointer to the manager
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateCmdBufferManager(
        const CHAR*           pBufferManagerName,
        const ResourceParams* pParams,
        CmdBufferManager**    ppCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroyCmdBufferManager
    ///
    /// @brief  A helper function to allow sub-classes to destroy command buffer managers.
    ///
    /// @param  ppCmdBufferManager  Pointer to the manager
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DestroyCmdBufferManager(
        CmdBufferManager**    ppCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateMultiCmdBufferManager
    ///
    /// @brief  A helper function to allow sub-classes to create and add command buffer managers that parent node will manage.
    ///
    /// @param  pParams                  Pointer to the cmd buffer manager params
    /// @param  numberOfBufferManagers   number of buffer mangers to be created
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateMultiCmdBufferManager(
        struct CmdBufferManagerParam* pParams,
        UINT32                        numberOfBufferManagers);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckCmdBufferWithRequest
    ///
    /// @brief  Helper method to checkout a used command buffer from the given manager.
    ///
    /// @param  requestId           The request Id to which this resource's lifecycle will be tied to
    /// @param  pCmdBufferManager   Command buffer manager from which a buffer is acquired
    ///
    /// @return Pointer to a CmdBuffer object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CmdBuffer* CheckCmdBufferWithRequest(
            UINT64              requestId,
            CmdBufferManager*   pCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddDeviceIndex
    ///
    /// @brief  Add the given device index to the list of indices for this node
    ///
    /// @param  deviceIndex Device index to add
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddDeviceIndex(
        INT32 deviceIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Node
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Node();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Node
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~Node();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetIntraPipelinePerFramePool
    ///
    /// @brief  Gets the pointer to the per frame property/metadata pool of Intra pipeline
    ///
    /// @param  poolType          Type of the per-frame pool asked for
    /// @param  intraPipelineId   Intra pipeline id of the frame property tool want to get
    ///
    /// @return Pointer to the per frame property/metadata pool
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MetadataPool* GetIntraPipelinePerFramePool(
        PoolType poolType,
        UINT     intraPipelineId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipeline
    ///
    /// @brief  Gets the pointer to the pipeline
    ///
    /// @return Pointer to the thread pool manager
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE Pipeline* GetPipeline()
    {
        return m_pPipeline;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNodeInPipeline
    ///
    /// @brief  Check if the node in the pipeline
    ///
    /// @param  nodeType   node type
    ///
    /// @return TRUE if find the node, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsNodeInPipeline(
        UINT nodeType)
    {
        return m_pPipeline->IsNodeExist(nodeType);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsTagPresentInPublishList
    ///
    /// @brief  Checks if the tag defined by tagId is present in the Per-Frame publishlist of the pipeline
    ///
    /// @param  tagId  Identifier of the tag to be checked
    ///
    /// @return TRUE if the tag is present, FALSE otherwise
    ///
    /// @note   This API should be only used to validate against tags published in the mainpool
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsTagPresentInPublishList(
       UINT32 tagId) const
    {
        UINT tagIndex = GetPSMetaIndex(tagId);
        return (MaxPSMetaType <= tagIndex) && m_pPipeline->IsTagPresentInPublishList(tagId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPipelineHasSnapshotJPEGStream
    ///
    /// @brief  Check if the pipeline has snapshot JPEG stream
    ///
    /// @return TRUE if the pipeline has snapshot JPEG stream, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsPipelineHasSnapshotJPEGStream()
    {
        return m_pPipeline->HasSnapshotJPEGStream();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetThreadManager
    ///
    /// @brief  Gets the pointer to the thread pool manager
    ///
    /// @return Pointer to the thread pool manager
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ThreadManager* GetThreadManager()
    {
        return m_pThreadManager;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequestIdDone
    ///
    /// @brief  Method to indicate that the requestId has been completely handled (all output data generated)
    ///
    /// @param  requestId requestId for which all outputs are generated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessRequestIdDone(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMetadataDone
    ///
    /// @brief  Method to indicate that the metadata for this requestId has been completely handled
    ///
    /// @param  requestId requestId for which metadata is generated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessMetadataDone(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessPartialMetadataDone
    ///
    /// @brief  Method to indicate that the partial metadata for this requestId has been completely handled
    ///
    /// @param  requestId requestId for which partial metadata is generated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessPartialMetadataDone(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessEarlyMetadataDone
    ///
    /// @brief  Method to indicate that the metadata for this requestId has been completely handled
    ///
    /// @param  requestId requestId for which metadata is generated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessEarlyMetadataDone(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPortId
    ///
    /// @brief  Get the output port Id
    ///
    /// @param  outputPortIndex Output port index
    ///
    /// @return Port id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetOutputPortId(
        UINT outputPortIndex
        ) const
    {
        CAMX_ASSERT(outputPortIndex < m_outputPortsData.numPorts);

        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        return pOutputPort->portId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumOutputPorts
    ///
    /// @brief  Get the number of output ports
    ///
    /// @return Number of output ports
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetNumOutputPorts() const
    {
        return m_outputPortsData.numPorts;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetNodeEnabled
    ///
    /// @brief  Set the Node enabled flag
    ///
    /// @param  isEnabled Enable flag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetNodeEnabled(
        BOOL isEnabled)
    {
        m_nodeFlags.isEnabled = isEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPort
    ///
    /// @brief  Get OutputPort pointer
    ///
    /// @param  portIndex portIndex
    ///
    /// @return Pointer to the OutputPort
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE OutputPort* GetOutputPort(
        UINT portIndex)
    {
        return &m_outputPortsData.pOutputPorts[portIndex];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputPort
    ///
    /// @brief  Get Input Port Pointer
    ///
    /// @param  portIndex portIndex
    ///
    /// @return TRUE if enabled, FALSE if disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE InputPort* GetInputPort(
        UINT portIndex)
    {
        return &m_inputPortsData.pInputPorts[portIndex];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPortNumBatchFrames
    ///
    /// @brief  Get the output port number of batch frames
    ///
    /// @param  outputPortIndex Output port index
    ///
    /// @return number of batch frames
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetOutputPortNumBatchFrames(
        UINT outputPortIndex
    ) const
    {
        CAMX_ASSERT(outputPortIndex < m_outputPortsData.numPorts);

        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        return pOutputPort->numBatchedFrames;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseOutputPortImageBufferReference
    ///
    /// @brief Release an OutputPort's ImageBufferReference
    ///
    /// @param  pOutputPort         Pointer to the output port
    /// @param  requestId           RequestId for which ImageBuffer to release
    /// @param  batchIndex          BatchIndex to release
    ///
    /// @return The number of ImageBuffer references remaining
    UINT ReleaseOutputPortImageBufferReference(
        OutputPort* const   pOutputPort,
        const UINT64        requestId,
        const UINT          batchIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseOutputPortCSLFences
    ///
    /// @brief Release the CSL Fences for one of this node's output port fences if it has a valid handle
    ///
    /// @param  pOutputPort                        Pointer to the output port
    /// @param  requestId                          RequestId for which fences to release
    /// @param  pOutputBufferIndex                 Pointer to output buffer index (for CHI fence, if any)
    /// @param  checkIsPrimaryFenceBeforeRelease   Flag to indicate if need to check for primary fence before release
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseOutputPortCSLFences(
        OutputPort* const pOutputPort,
        const UINT64      requestId,
        UINT*             pOutputBufferIndex,
        BOOL              checkIsPrimaryFenceBeforeRelease);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetInputPortBufferDelta
    ///
    /// @brief  Sets the input ports buffer delta value. The delata indicates which previous buffer is needed for the request.
    ///
    /// @param  portIndex        Input Port Index
    /// @param  delta            The delta value to set
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetInputPortBufferDelta(
        UINT    portIndex,
        UINT32  delta);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputPortDisableMask
    ///
    /// @brief  function to get input port disable mask
    ///
    /// @return input port disable mask
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT GetInputPortDisableMask() const
    {
        return m_inputPortDisableMask;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnableInputOutputLink
    ///
    /// @brief  Enable one input link from the output port
    ///
    /// @param  nodeInputIndex  Input port index which is to be enabled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID EnableInputOutputLink(
        UINT nodeInputIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DisableInputOutputLink
    ///
    /// @brief  Disable one input link from the output port
    ///
    /// @param  nodeInputIndex  Input port index which is to be disabled
    /// @param  removeIndices   Indicates whether device indice should be removed or not
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DisableInputOutputLink(
        UINT nodeInputIndex,
        BOOL removeIndices);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemovePerRequestInputPortLinkForOutputPort
    ///
    /// @brief  Remove the input port link for the given output port index, updated every request
    ///
    /// @param  portIndex  Output Port index to remove per request input port links from
    /// @param  requestId  RequestId for which ports to disable
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RemovePerRequestInputPortLinkForOutputPort(
          UINT   portIndex,
          UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemoveOutputDeviceIndices
    ///
    /// @brief  Remove the list of devices from output port
    ///
    /// @param  portIndex        Output Port index to remove devices from
    /// @param  pDeviceIndices   List of devcie indices to be removed
    /// @param  deviceIndexCount Count of devices indices in pDeviceIndices
    /// @param  removeIndices    Indicates whether device indice should be removed or not
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RemoveOutputDeviceIndices(
        UINT         portIndex,
        const INT32* pDeviceIndices,
        UINT         deviceIndexCount,
        BOOL         removeIndices);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMultiCameraInfo
    ///
    /// @brief  Get Multi camera information
    ///
    /// @param  pIsMultiCameraUsecase   Whether current use case is multi camera use case
    /// @param  pNumberOfCamerasRunning Number of cameras currently running
    /// @param  pCurrentCameraId        Current camera id
    /// @param  pIsMasterCamera         Whether this is master camera (or aux camera)
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetMultiCameraInfo(
        BOOL*   pIsMultiCameraUsecase,
        UINT32* pNumberOfCamerasRunning,
        UINT32* pCurrentCameraId,
        BOOL*   pIsMasterCamera);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMultiCameraMasterDependency
    ///
    /// @brief  Set Dependency for multi-camera master resolution. If the dependency is set, all the GetDataList calls
    ///         for that request will be routed to GetDataListByCameraId
    ///
    /// @param  pNodeRequestData Pointer to the incoming NodeProcessRequestData
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetMultiCameraMasterDependency(
        NodeProcessRequestData*  pNodeRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ActivateImageBuffers
    ///
    /// @brief  Activate ImageBuffer Managers of this Node's output ports. This will give a hint to ImageBufferManager
    ///         to allocate/manage image buffers
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ActivateImageBuffers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeactivateImageBuffers
    ///
    /// @brief  Deactivate ImageBuffer Managers of this Node's output ports. This will give a hint to ImageBufferManager
    ///         to release image buffers allocated through cslalloc
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DeactivateImageBuffers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetInputBuffersReadyDependency
    ///
    /// @brief  Set fence dependencies for input buffers ready
    ///
    /// @param  pExecuteProcessRequestData  Process request data
    /// @param  dependencyUnitIndex         Dependency unit index at which input buffer dependencies have to be populated
    ///
    /// @return Number of fences with dependency setup
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT SetInputBuffersReadyDependency(
        ExecuteProcessRequestData* pExecuteProcessRequestData,
        UINT                       dependencyUnitIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BindInputOutputBuffers
    ///
    /// @brief  Bind buffers to input, output ImageBuffer holders. If Delayed/Late binding is enabled, only ImageBuffer
    ///         holders are created while setting up the request. Those ImageBuffers may not be having backing buffer.
    ///         So, nodes have to call this function to make sure the ImageBuffers are actually backed with buffers to start the
    ///         actual processing. Only Get functions related to Format return valid information before call to this function.
    ///         Get functions for cslhandle, fd, etc return invalid information until the ImageBuffer is backed with memory.
    ///
    /// @param  pPerRequestPorts    Pointer to Per request active port information for this node.
    /// @param  bBindInputBuffers   Whether to bind input ImageBuffers with backing buffers
    /// @param  bBindOutputBuffers  Whether to bind output ImageBuffers with backing buffers
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult BindInputOutputBuffers(
        const PerRequestActivePorts*    pPerRequestPorts,
        BOOL                            bBindInputBuffers,
        BOOL                            bBindOutputBuffers);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFrameDataCallBack
    ///
    /// @brief  A callback method to process the frame data
    ///
    /// @param  requestId       Request ID
    /// @param  portId          port ID
    /// @param  pBufferInfo     Chi Buffer info structure
    /// @param  pImageFormat    Image format pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ProcessFrameDataCallBack(
        UINT64                requestId,
        UINT                  portId,
        ChiBufferInfo*        pBufferInfo,
        const ImageFormat*    pImageFormat)
    {
        CAMX_UNREFERENCED_PARAM(requestId);
        CAMX_UNREFERENCED_PARAM(portId);
        CAMX_UNREFERENCED_PARAM(pBufferInfo);
        CAMX_UNREFERENCED_PARAM(pImageFormat);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDownscaleDimension
    ///
    /// @brief  Get downscale dimension from intermediate size
    ///
    /// @param  intermediateWidth    intermediate width
    /// @param  intermediateHeight   intermediate height
    /// @param  pDS4Dimension        pointer to dimension of DS4 of intermediate size
    /// @param  pDS16Dimension       pointer to dimension of DS4 of intermediate size
    /// @param  pDS64Dimension       pointer to dimension of DS4 of intermediate size
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetDownscaleDimension(
        UINT32           intermediateWidth,
        UINT32           intermediateHeight,
        ImageDimensions* pDS4Dimension,
        ImageDimensions* pDS16Dimension,
        ImageDimensions* pDS64Dimension);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CancelRequest
    ///
    /// @brief  Method to release/clear interanl resources that derive node might have allocated/reserved for a request
    ///
    /// @param  requestId Request for which cmdbuffers are complete
    ///
    /// @return return CamxResultSuccess if cancel request is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult CancelRequest(
        UINT64 requestId)
    {
        CAMX_UNREFERENCED_PARAM(requestId);

        return CamxResultSuccess;
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
        return m_pPipeline->GetFlushInfo(rFlushInfo);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPSMetadataDependency
    ///
    /// @brief  Set Dependency for port specific metadata
    ///
    /// @param  tagId            Metadata Identifier
    /// @param  inputPortId      Input port Identifier
    /// @param  offset           Offset from the requestId
    /// @param  pNodeRequestData Pointer to the incoming NodeProcessRequestData
    ///
    /// @return return CamxResultSuccess if cancel request is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetPSMetadataDependency(
        UINT                     tagId,
        UINT                     inputPortId,
        UINT64                   offset,
        NodeProcessRequestData*  pNodeRequestData);

    BOOL        m_derivedNodeHandlesMetaDone; ///< Derived nodes take responsiblity of calling ProcessMetadataDone
    const CHAR* m_pNodeName;                  ///< String representing node type
    UINT32      m_pipelineDelayOffset;        ///< Max offset used by node
    BOOL        m_parallelProcessRequests;    ///< Are ExecuteProcessRequest calls allowed to be threaded
    UINT        m_maxjpegsize;                ///< Max jpeg image buffer size


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateImageBufferManager
    ///
    /// @brief  Create image buffer manager
    ///
    /// @param  pBufferManagerName       Name of the image buffer manager
    /// @param  pBufferManagerCreateData Buffer manager create data
    /// @param  ppImageBufferManager     Pointer to Image Buffer Manager instance
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CreateImageBufferManager(
        const CHAR*              pBufferManagerName,
        BufferManagerCreateData* pBufferManagerCreateData,
        ImageBufferManager**     ppImageBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPortCrop
    ///
    /// @brief  Static function to get applied crop for a given port id, function will map the input port to parent output port
    ///         and provide crop info. Currnetly is function should only be used when parent node is IFE. Will be expanded to
    ///         all node when we move crop info from metadat to port info.
    ///
    /// @param  pNode               Pointer to caller node
    /// @param  inputPortId         Input port Id
    /// @param  pCropInfo           Pointer to crop info
    /// @param  pIsFindByRecurssion Pointer to check if find by recurssion is enabled. It should be used only when input to
    ///                             output mapping in a node is 1:1 and not n:1
    ///
    /// @return Status of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetPortCrop(
        Node*         pNode,
        UINT          inputPortId,
        PortCropInfo* pCropInfo,
        UINT32*       pIsFindByRecurssion);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RecycleRetiredCmdBuffers
    ///
    /// @brief  Evaluate retired cmdbuffers list and recycle retired ones
    ///
    /// @param  requestId Request for which cmdbuffers are complete
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RecycleRetiredCmdBuffers(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RecycleAllCmdBuffers
    ///
    /// @brief  Recycle all command buffers for all command buffer managers
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RecycleAllCmdBuffers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFDPerFrameMetaDataSettings
    ///
    /// @brief  Get FD meta data settings
    ///
    /// @param  offset                      Frame offset
    /// @param  pIsFDPostingResultsEnabled  Pointer to FD result-posting flag to output the per frame setting
    /// @param  pPerFrameSettings           Pointer to FD setting struct to output the per frame settings
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFDPerFrameMetaDataSettings(
        UINT64               offset,
        BOOL*                pIsFDPostingResultsEnabled,
        FDPerFrameSettings*  pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPreviewPresent
    ///
    /// @brief  Checks if Preview is present in the stream
    ///
    /// @return TRUE if Preview is present else return FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPreviewPresent();

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

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequestRecovery
    ///
    /// @brief  When node detects it is in erroneous state, it can request the pipeline to trigger session recovery.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID RequestRecovery()
    {
        CAMX_LOG_CONFIG(CamxLogGroupCore, "%s signaled pipeline id %d for recovery", NodeIdentifierString(), GetPipelineId());
        m_pPipeline->TriggerGenericRecovery();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPSDataList
    ///
    /// @brief  Fetches data pointers from data pools for the provided Port specific data ids
    ///
    /// @param  inputPortId Input portId from the data to be fetched
    /// @param  pDataList   Array of data identifiers to be updated
    /// @param  ppData      Array of data to update in the pool
    /// @param  pOffsets    Array of offsets from the current requestID to determine slot from which data pointer is obtained
    /// @param  length      Length of all the arrays
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPSDataList(
        UINT32       inputPortId,
        const UINT*  pDataList,
        VOID** const ppData,
        UINT64*      pOffsets,
        SIZE_T       length);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WritePSDataList
    ///
    /// @brief  Updates data in the data pools for Post specific data provided
    ///
    /// @param  outputPortId    Output port Identifier
    /// @param  pDataList       Array of data identifiers to be updated
    /// @param  ppData          Array of data to update in the pool
    /// @param  pDataSize       Array of sizes of the data in pData
    /// @param  length          Length of all the arrays
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WritePSDataList(
        UINT32       outputPortId,
        const UINT*  pDataList,
        const VOID** ppData,
        const UINT*  pDataSize,
        SIZE_T       length);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AnyPortHasLateBindingDisabled
    ///
    /// @brief  Checks if any currently active output port has late binding disabled.
    ///
    /// @param  pPerRequestPorts       Pointer to per request active ports for this Node
    ///
    /// @return TRUE if any active output port has LateBinding disabled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AnyPortHasLateBindingDisabled(
            PerRequestActivePorts*  pPerRequestPorts);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHal3CropInfo
    ///
    /// @brief  Get Hal3 crop info corresponding to first node
    ///
    /// @param  ppData          Array of data to update in the pool
    /// @param  offset          Offset from which HAL crop info must be fetched
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetHal3CropInfo(
        VOID** const     ppData,
        UINT64           offset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLinkedActiveInputPorts
    ///
    /// @brief  Get array of active input ports internally connect to given output port
    ///
    /// @param  outputPortId                Output port identifier for which input port array is required
    /// @param  requestId                   Request for which port info is requested
    /// @param  rInputPortInfo              Vector of active ports
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetLinkedActiveInputPorts(
        UINT                                    outputPortId,
        UINT64                                  requestId,
        std::vector<PerRequestInputPortInfo*>&  rInputPortInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStreamCropInfo
    ///
    /// @brief  Gets crop info correponding to specified publisher
    ///
    /// @param  inputPortIndex  Input port index
    /// @param  rPublisherInfo  Unique portinfo describing the PSMetadata
    /// @param  tagId           Tag identifiers to be updated
    /// @param  ppData          Array of data to update in the pool
    /// @param  offset          Offset from the current requestID to determine slot from which data pointer is obtained
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetStreamCropInfo(
        UINT                inputPortIndex,
        UniqueNodePortInfo& rPublisherInfo,
        UINT                tagId,
        VOID** const        ppData,
        UINT64              offset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPSMetadata
    ///
    /// @brief  Gets PS meta data correponding to input port
    ///
    /// @param  inputPortId     Input Port id corresponding for which crop info is requested
    /// @param  tagId           Data identifiers to be updated
    /// @param  ppData          Array of data to update in the pool
    /// @param  offset          Offset from the current requestID to determine slot from which data pointer is obtained
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPSMetadata(
        UINT            inputPortId,
        UINT            tagId,
        VOID** const    ppData,
        UINT64          offset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPSMetaPublisherInfo
    ///
    /// @brief  Gets the publisher port info
    ///
    /// @param  tagId            Tag Identifier
    /// @param  inputPortIndex   Index id the input port
    /// @param  rPublishPortInfo Publisher node port info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetPSMetaPublisherInfo(
        UINT                tagId,
        UINT                inputPortIndex,
        UniqueNodePortInfo& rPublishPortInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPSMetaTagId
    ///
    /// @brief  Gets port-specific metadata identifier given the type
    ///
    /// @param  type    PSMeta type
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetPSMetaTagId(
        PSMetadataType         type)
    {
        return m_pPSMData[type].tagId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPSData
    ///
    /// @brief  Publish PS metadata for all ports
    ///
    /// @param  tagId        Metadata identifier
    /// @param  pBypassInfo  If non-NULL, contains the map of input output ports to fetch the metadata
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishPSData(
        UINT32                   tagId,
        ChiPSMetadataBypassInfo* pBypassInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPSMetaIndex
    ///
    /// @brief  Get Port-Specific metadata index given the tag identifier
    ///
    /// @param  tagId Tag Identifier
    ///
    /// @return PLM Index
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetPSMetaIndex(
        UINT tagId) const
    {
        UINT index;

        for (index = 0; index < MaxPSMetaType; ++index)
        {
            if (m_pPSMData[index].tagId == tagId)
            {
                break;
            }
        }
        return index;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumChildImageBufferReferences
    ///
    /// @brief  Given an output port, return the number of image buffer references to be assigned for the that port's children.
    ///
    /// @param  pOutputPort Pointer to an output port
    ///
    /// @return Number of references assigned for that node's children
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetNumChildImageBufferReferences(
        const OutputPort* const pOutputPort) const
    {
        UINT result = (pOutputPort->numInputPortsConnected) + (pOutputPort->numInputPortsConnectedinBypass);

        // In the case of a shared sink buffer, the connected sink buffer is assumed to not need an image buffer reference
        // assigned by the node
        if ((TRUE == pOutputPort->flags.isSharedSinkBuffer) && (result > 0))
        {
            result--;
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsEndoOfStreamEnabled
    ///
    /// @brief  This function will return if end of stream is enabled for current requestid
    ///
    /// @return Flag for end of stream enabled/disabled for current request
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT8 IsEndoOfStreamEnabled()
    {
        UINT8 result = 0;
        UINT   props[]                          = { m_vendorTagEndOfStreamFlag | InputMetadataSectionMask };
        VOID*  pData[CAMX_ARRAY_SIZE(props)]    = { 0 };
        UINT64 offsets[CAMX_ARRAY_SIZE(props)]  = { 0 };
        GetDataList(props, pData, offsets, CAMX_ARRAY_SIZE(props));

        if (NULL != pData[0])
        {
            result = *static_cast<UINT8*>(pData[0]);
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetEndoOfStreamRequestId
    ///
    /// @brief  This function will return the request id on which end of stream was triggred
    ///
    /// @return End of stream request Id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT64 GetEndoOfStreamRequestId()
    {
        UINT64  requestId  = 0;
        UINT64* pRequestId = NULL;
        MetadataSlot* pUsecaseSlot = m_pUsecasePool->GetSlot(0);

        pRequestId = static_cast<UINT64*>(pUsecaseSlot->GetMetadataByTag(
            m_vendorTagEndOfStreamRequestId, NodeIdentifierString()));
        if (NULL != pRequestId)
        {
            requestId = *pRequestId;
        }

        return requestId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetCSLSyncIDLookupTable
    ///
    /// @brief  Method to reset to the initial state for CSL sync lookup table
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID ResetCSLSyncIDLookupTable()
    {
        Utils::Memset(&m_CSLSyncID[0], 0, sizeof(m_CSLSyncID));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetImageStabilizationMargins
    ///
    /// @brief  Get the actual image stabilization margin and margin ratio
    ///
    /// @param  rStabilizationMargin       Reference to the stabilization margin
    /// @param  rStabilizationMarginRatio  Reference to the ratio of stabilization margin
    ///
    /// @return CamxResultSuccess if successful, error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetImageStabilizationMargins(
        StabilizationMargin&    rStabilizationMargin,
        MarginRequest&          rStabilizationMarginRatio);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Method to initialize Node object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeNonSinkPortBufferProperties
    ///
    /// @brief  Method to initialize buffer properties of a non sink port buffer
    ///
    /// @param  outputPortIndex     Output port index
    /// @param  pPortLink           Port link information
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeNonSinkPortBufferProperties(
        UINT            outputPortIndex,
        const PortLink* pPortLink);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeSinkPortBufferProperties
    ///
    /// @brief  Method to initialize buffer properties of the sink port buffer
    ///
    /// @param  outputPortIndex      Output port index
    /// @param  pNodeCreateInputData Node create input data
    /// @param  pOutputPortInfo      Output port info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeSinkPortBufferProperties(
        UINT                        outputPortIndex,
        const NodeCreateInputData*  pNodeCreateInputData,
        const OutputPortInfo*       pOutputPortInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeSourcePortBufferFormat
    ///
    /// @brief  Method to initialize buffer format of the source port buffer
    ///
    /// @param  inPortIndex         Input port index
    /// @param  pChiStreamWrapper   Stream wrapper class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeSourcePortBufferFormat(
        UINT              inPortIndex,
        ChiStreamWrapper* pChiStreamWrapper);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeNonSinkHALBufferPortBufferProperties
    ///
    /// @brief  Method to initialize buffer properties of the Non sink HAL buffer port
    ///
    /// @param  outputPortIndex     Output port index
    /// @param  pInplaceOutputPort  Output port info
    /// @param  pChiStreamWrapper   Stream info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeNonSinkHALBufferPortBufferProperties(
        UINT                    outputPortIndex,
        const OutputPort*       pInplaceOutputPort,
        const ChiStreamWrapper* pChiStreamWrapper);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CSLFenceCallback
    ///
    /// @brief  CSL calls this function to inform about a signaled fence
    ///
    /// @param  pNodePrivateFenceData Private data for the callback. This data is passed by the client to CSL when the client
    ///                               registers the callback and CSL simply passes it back to the client
    /// @param  hSyncFence            Fence for which the fence is signaled
    /// @param  fenceResult           Indicates errors if any
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CSLFenceCallback(
        VOID*           pNodePrivateFenceData,
        CSLFence        hSyncFence,
        CSLFenceResult  fenceResult);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFenceCallback
    ///
    /// @brief  Function that does any processing required for a fence callback. When CSL notifies the Node of a fence we
    ///         post a job in the threadpool. And when threadpool calls us to process the fence-signal notification, this
    ///         function does the necessary processing.
    ///
    /// @param  pFenceHandlerData Pointer to struct FenceHandlerData that belongs to this node
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessFenceCallback(
        NodeFenceHandlerData* pFenceHandlerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFenceCallback
    ///
    /// @brief  Fence notify mechanism to the derived nodes in case they need to know when a output ports buffer is available
    ///
    /// @param  pFenceHandlerData       Pointer to struct FenceHandlerData that belongs to this node
    /// @param  requestId               Request Id whose fence has come back
    /// @param  portId                  Port Id whose fence has come back
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ProcessingNodeFenceCallback(
        NodeFenceHandlerData* pFenceHandlerData,
        UINT64 requestId,
        UINT   portId)
    {
        CAMX_UNREFERENCED_PARAM(pFenceHandlerData);
        CAMX_UNREFERENCED_PARAM(requestId);
        CAMX_UNREFERENCED_PARAM(portId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyRequestProcessingError
    ///
    /// @brief  Function that does any processing required for a error fence callback. When CSL notifies the Node of a fence we
    ///         post a job in the threadpool. And when threadpool calls us to process the fence-signal notification, this
    ///         function does the necessary error processing.
    ///
    /// @param  pFenceHandlerData     Pointer to struct FenceHandlerData that belongs to this node
    /// @param  unSignaledFenceCount  Number of un-signalled fence count
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID NotifyRequestProcessingError(
            NodeFenceHandlerData* pFenceHandlerData,
            UINT unSignaledFenceCount)
    {
        CAMX_UNREFERENCED_PARAM(unSignaledFenceCount);
        CAMX_ASSERT(NULL != pFenceHandlerData);
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
        return FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessSinkPortNewRequest
    ///
    /// @brief  Any output port processing required for a new request (should be called *ONLY* for ports that do output HAL
    ///         buffer
    ///
    /// @param  requestId           RequestId for which to perform any processing
    /// @param  batchIndex          batchIndex relating to this request
    /// @param  pOutputPort         Pointer to the output port
    /// @param  pOutputBufferIndex  Pointer to output buffer index (for CHI fence, if any)
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessSinkPortNewRequest(
        UINT64      requestId,
        UINT32      batchIndex,
        OutputPort* pOutputPort,
        UINT*       pOutputBufferIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataPublishList
    ///
    /// @brief  Virtual function that must be derived classes to query the publish list from the node
    ///
    /// @param  pPublistTagList List of tags published by the node. pTagArray and tagCount must be filled by the node
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult QueryMetadataPublishList(
        NodeMetadataList* pPublistTagList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLinkedInputPorts
    ///
    /// @brief  Virtual function to get Get array of input ports internally connect to given output port
    ///
    /// @param  outputPortIndex             Output port for which input port array is required
    /// @paramrInputPort                    Vector of input ports
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult GetLinkedInputPorts(
        UINT                     outputPortIndex,
        std::vector<InputPort*>& rInputPort);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessNonSinkPortNewRequest
    ///
    /// @brief  Any output port processing required for a new request (should be called *ONLY* for ports that do not output HAL
    ///         buffer
    ///
    /// @param  requestId           RequestId for which to perform any processing
    /// @param  sequenceId          sequenceId for which the info is seeked
    /// @param  pOutputPort         Pointer to the output port
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessNonSinkPortNewRequest(
        UINT64      requestId,
        UINT32      sequenceId,
        OutputPort* pOutputPort);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillTuningModeData
    ///
    /// @brief  Fill tuning mode data for process request
    ///
    /// @param  ppTuningModeData Pointer to fill with pointer to tuning mode data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillTuningModeData(
        VOID** ppTuningModeData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnableOutputPortForStreams
    ///
    /// @brief  Enable output port for some stream
    ///
    /// @param  outputPortIndex Output port that needs to be enabled for the passed in stream
    /// @param  streamIdMask    Streams to enable
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID EnableOutputPortForStreams(
        UINT outputPortIndex,
        UINT streamIdMask)
    {
        CAMX_ASSERT(outputPortIndex < m_outputPortsData.numPorts);

        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        pOutputPort->enabledInStreamMask |= streamIdMask;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DisableOutputPortForStream
    ///
    /// @brief  Disable output port for some stream
    ///
    /// @param  outputPortIndex Output port that needs to be disabled for the passed in stream
    /// @param  streamId        Stream to enable
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID DisableOutputPortForStream(
        UINT outputPortIndex,
        UINT streamId)
    {
        CAMX_ASSERT(outputPortIndex < m_outputPortsData.numPorts);

        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        pOutputPort->enabledInStreamMask = Utils::BitReset(pOutputPort->enabledInStreamMask, streamId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsOutputPortEnabledForStream
    ///
    /// @brief  Is output port enabled for some stream
    ///
    /// @param  outputPortIndex Output port to check
    /// @param  streamId        Stream to check for
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsOutputPortEnabledForStream(
        UINT outputPortIndex,
        UINT streamId
        ) const
    {
        CAMX_ASSERT(outputPortIndex < m_outputPortsData.numPorts);

        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        return Utils::IsBitSet(pOutputPort->enabledInStreamMask, streamId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsOutputPortEnabled
    ///
    /// @brief  Check if output port is enabled
    ///
    /// @param  outputPortIndex Outputport to check
    ///
    /// @return TRUE if enabled, FALSE if disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsOutputPortEnabled(
        UINT outputPortIndex
        ) const
    {
        CAMX_ASSERT(outputPortIndex < m_outputPortsData.numPorts);

        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        return pOutputPort->flags.isEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsOutputPortActive
    ///
    /// @brief  Check if output port is active for the node of a specific request id.
    ///
    /// @param  requestId       Request to check
    /// @param  outputPortIndex Outputport to check
    ///
    /// @return TRUE if active, FALSE if not
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsOutputPortActive(
        UINT64  requestId,
        UINT    outputPortIndex
        ) const
    {
        CAMX_ASSERT(outputPortIndex < m_outputPortsData.numPorts);

        BOOL                         isActive     = FALSE;
        UINT                         portId       = m_outputPortsData.pOutputPorts[outputPortIndex].portId;
        const PerRequestActivePorts* pActivePorts = &m_perRequestInfo[requestId % MaxRequestQueueDepth].activePorts;

        for (UINT i = 0; i < pActivePorts->numOutputPorts; i++)
        {
            if (portId == pActivePorts->pOutputPorts[i].portId)
            {
                isActive = TRUE;
                break;
            }
        }

        return isActive;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsInputPortEnabled
    ///
    /// @brief  Check if input port is enabled
    ///
    /// @param  inputPortIndex Inputport to check
    ///
    /// @return TRUE if enabled, FALSE if disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsInputPortEnabled(
        UINT inputPortIndex
        ) const
    {
        CAMX_ASSERT(inputPortIndex < m_inputPortsData.numPorts);

        InputPort* pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];

        return pInputPort->flags.isEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSinkInplaceBufferPort
    ///
    /// @brief  Query for if a output port is having the link property of SinkInplaceBuffer
    ///
    /// @param  portLink queried output port link
    ///
    /// @return TRUE if it is a sink inplace buffer port, else FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSinkInplaceBufferPort(
        PortLink portLink
        ) const
    {
        BOOL isSinkInplaceBufferPort = FALSE;

        for (UINT index = 0; index < portLink.linkProperties.numLinkFlags; index++)
        {
            if (LinkPropFlags::SinkInplaceBuffer == portLink.linkProperties.LinkFlags[index])
            {
                isSinkInplaceBufferPort = TRUE;
                break;
            }
        }

        return isSinkInplaceBufferPort;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLinkedOutputPortIndex
    ///
    /// @brief  Gets the linked output port index given the input port index
    ///
    /// @param  inputPortIndex Inputport to check
    ///
    /// @return Input port index if succesful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetLinkedOutputPortIndex(
        UINT    inputPortIndex)
    {
        InputPort* pInputPort  = &m_inputPortsData.pInputPorts[inputPortIndex];
        UINT32     portIndex;

        if (NULL != pInputPort->pParentNode)
        {
            portIndex = pInputPort->parentOutputPortIndex;
        }
        else
        {
            portIndex = InvalidIndex;
        }

        return portIndex;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConstructPSMetaMap
    ///
    /// @brief  Method to construct port-specific map
    ///
    /// @param  tagId        Tag Identifier
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConstructPSMetaMap(
        UINT32 tagId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteData
    ///
    /// @brief  Write data to the data pools and publish it
    ///
    /// @param  requestId Request for which data is being updated
    /// @param  dataId    Tag or Property being updated
    /// @param  size      Size of data to update, sizeof(struct) for properties, number or elements for tags
    /// @param  pData     Pointer to data to set in the pool
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WriteData(
        UINT64      requestId,
        UINT32      dataId,
        SIZE_T      size,
        const VOID* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNodeEnabled
    ///
    /// @brief  Check if the node is enabled
    ///
    /// @return TRUE if enabled, FALSE if disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsNodeEnabled() const
    {
        return m_nodeFlags.isEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRequestInputPorts
    ///
    /// @brief  Perform the setup of input ports for a new request
    ///
    /// @param  pPerBatchedFrameInfo Per batch information for the request
    ///
    /// @return Status of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupRequestInputPorts(
        PerBatchedFrameInfo* pPerBatchedFrameInfo);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRequestInputBypassReferences
    ///
    /// @brief  Setup image buffer references for bypassed input ports
    ///
    /// @param  pInputPort   Pointer to the input port
    /// @param  pImageBuffer Pointer to the image buffer to add references
    ///
    /// @return Status of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupRequestInputBypassReferences(
        InputPort*   pInputPort,
        ImageBuffer* pImageBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InputStreamMatches
    ///
    /// @brief  Compares an input port to buffer info
    ///
    /// @param  rInputBufferInfo Reference to the input buffer info
    /// @param  rInputPort       Reference to the input port
    ///
    /// @return TRUE if the input port and buffer info match (same portId, same buffer properties)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL InputStreamMatches(
        const StreamInputBufferInfo& rInputBufferInfo,
        const InputPort&             rInputPort);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRequestOutputPorts
    ///
    /// @brief  Perform the setup of output ports for a new request
    ///
    /// @param  pPerBatchedFrameInfo Per batch information for the request
    ///
    /// @return Status of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupRequestOutputPorts(
        PerBatchedFrameInfo* pPerBatchedFrameInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpInputImageBuffer
    ///
    /// @brief  Dump the image buffer of an input port
    ///
    /// @param  pInputPort   Pointer to the input port
    /// @param  pImageBuffer Pointer to the image buffer to dump
    ///
    /// @return Status of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DumpInputImageBuffer(
        InputPort*   pInputPort,
        ImageBuffer* pImageBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRequestOutputPortFence
    ///
    /// @brief  Perform the setup of an output port fence for a new request
    ///
    /// @param  pOutputPort Pointer to the output port
    /// @param  requestId   Request for which data is being setup
    /// @param  pOutputBufferIndex  Pointer to output buffer index (for CHI fence, if any)
    ///
    /// @return Status of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupRequestOutputPortFence(
        OutputPort* const pOutputPort,
        const UINT64      requestId,
        UINT*             pOutputBufferIndex = NULL);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRequestOutputPortImageBuffer
    ///
    /// @brief  Perform the setup of an output port's ImageBuffer for a new request
    ///
    /// @param  pOutputPort       Pointer to the output port
    /// @param  requestId         Request for which data is being setup
    /// @param  sequenceId        Sequence for which data is being setup
    /// @param  imageBufferIndex  index at which to place the new ImageBuffer
    ///
    /// @return Status of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupRequestOutputPortImageBuffer(
        OutputPort* const pOutputPort,
        const UINT64      requestId,
        const UINT32      sequenceId,
        const UINT        imageBufferIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseBufferManagerCompositeImageReference
    ///
    /// @brief  Iterate through all available ports and release the reference for composite image buffers
    ///
    /// @param  hFence    handle to the primary fence
    /// @param  requestId Request Id for which the composite buffer reference needs to be released
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseBufferManagerCompositeImageReference(
       CSLFence hFence,
       UINT64   requestId);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SignalCompositeFences
    ///
    /// @brief  Iterate through all available ports and signal the fences
    ///
    /// @param  pOutputPort         Pointer to the output Port
    /// @param  pFenceHandlerData   Pointer to the fence handler data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SignalCompositeFences(
       OutputPort*              pOutputPort,
       NodeFenceHandlerData*    pFenceHandlerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumOfBuffersToBeAllocatedImmediately
    ///
    /// @brief  Get the number of buffers to be allocated immediately during the call to InitializeBuffers in the
    ///         ImageBufferManager. For certain nodes, a small subset of the total buffers needed are allocated immediately
    ///         and the remainder are allocated later if needed
    ///
    /// @param  immediateBufferCount    ImmediateAllocCount defined in xml bufferProperties
    /// @param  maxBufferCount          Maximum buffer count to use when override buffer count is enabled and set to 0
    ///
    /// @return Number of buffers to be allocated immediately
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT GetNumOfBuffersToBeAllocatedImmediately(
        UINT immediateBufferCount,
        UINT maxBufferCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaximumNumOfBuffersToBeAllocated
    ///
    /// @brief  Get the maximum number of buffers to be allocated in the ImageBufferManager. If the override value is set, the
    ///         maximum buffer count value (as determined by the value inside buffer properties) is overriden with this value
    ///
    /// @param  maxBufferCount Maximum buffer count to use when override buffer count is enabled and set to 0
    ///
    /// @return Maximum number of buffers to be allocated
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT GetMaximumNumOfBuffersToBeAllocated(
        UINT maxBufferCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpData
    ///
    /// @brief  Helper function to dump the output of a Node
    ///
    /// @param  pFenceHandlerData Pointer to fence handler data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpData(
        NodeFenceHandlerData* pFenceHandlerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddInputConnectedforBypass
    ///
    /// @brief  Add number of input connected through to this output port
    ///
    /// @param  outputPortIndex         output port index
    /// @param  numInputPortsConnected  num of input ports connected through bypass
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VOID AddInputConnectedforBypass(
        UINT outputPortIndex,
        UINT numInputPortsConnected);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateBufferInfoforPendingInputPorts
    ///
    /// @brief  Update buffer info for pending input ports
    ///
    /// @param  pRequestPorts Info of all active ports
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateBufferInfoforPendingInputPorts(
        PerRequestActivePorts* pRequestPorts);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPendingBufferDependency
    ///
    /// @brief  Set pending buffer dependency for the node that has bypassable parent node
    ///
    /// @param  pExecuteProcessRequestData Process Request data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetPendingBufferDependency(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateDeviceIndices
    ///
    /// @brief  Update device indices
    ///
    /// @param  outputPortIndex output port index
    /// @param  pDeviceIndices  pointer to the devices that needs to be added
    /// @param  deviceCount     device indices count
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateDeviceIndices(
        UINT         outputPortIndex,
        const INT32* pDeviceIndices,
        UINT         deviceCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NodeSourceInputPortChiFenceCallback
    ///
    /// @brief  Callback method for Source input chi fences
    ///
    /// @param  pUserData   Opaque pointer provided by client to CSLFenceAsyncWait.
    /// @param  hSyncFence  The CSL fence that triggered the callback.
    /// @param  result      Indicates the success/failure status of the signal
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID NodeSourceInputPortChiFenceCallback(
        VOID*           pUserData,
        CSLFence        hSyncFence,
        CSLFenceResult  result);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleNodeSourceInputPortChiFenceCallback
    ///
    /// @brief  Callback handle method for Source input chi fences
    ///
    /// @param  pUserData   Opaque pointer provided by client to CSLFenceAsyncWait.
    /// @param  hSyncFence  The CSL fence that triggered the callback.
    /// @param  result      Indicates the success/failure status of the signal
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleNodeSourceInputPortChiFenceCallback(
        VOID*           pUserData,
        CSLFence        hSyncFence,
        CSLFenceResult  result);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WatermarkImage
    ///
    /// @brief  Helper function to watermark frame id onto frame
    ///
    /// @param  pFenceHandlerData Pointer to fence handler data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID WatermarkImage(
        NodeFenceHandlerData* pFenceHandlerData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NonSinkPortDSPSecureAccessNeeded
    ///
    /// @brief  Check if the buffers allocated at this output port need DSP Secure access.
    ///         For now, assume - When in secure mode, if this output port is connected to any CHI Node - we need DSP secure
    ///         access. This is by assuming the connected CHI node runs on DSP.
    ///
    /// @param  pOutputPort     Pointer to the output port
    ///
    /// @return TRUE if enabled, FALSE if disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL NonSinkPortDSPSecureAccessNeeded(
        const OutputPort* pOutputPort
        ) const
    {
        BOOL bDSPSecureAccessNeeded = FALSE;

        if (TRUE == pOutputPort->flags.isSecurePort)
        {
            // loop through all Nodes connected on this output port and check if any CHI node exists.

            for (UINT dstNodeIndex = 0; dstNodeIndex < pOutputPort->numOfNodesConnected; dstNodeIndex++)
            {
                if ((NULL            != pOutputPort->pDstNodes[dstNodeIndex]) &&
                    (ChiExternalNode == pOutputPort->pDstNodes[dstNodeIndex]->Type()))
                {
                    bDSPSecureAccessNeeded = TRUE;
                }
            }
        }

        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node[%s] Port[%d][%s] : bDSPSecureAccessNeeded=%d",
                         NodeIdentifierString(), pOutputPort->portId, GetOutputPortName(pOutputPort->portId),
                         bDSPSecureAccessNeeded);

        return bDSPSecureAccessNeeded;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDataListFromPipeline
    ///
    /// @brief  Fetches data pointers from data pools for the provided data ids
    ///
    /// @param  pDataList   Array of data identifiers to be updated
    /// @param  ppData      Array of data to update in the pool
    /// @param  pOffsets    Array of offsets from the current requestID to determine slot from which data pointer is obtained
    /// @param  length      Length of all the arrays
    /// @param  pNegate     Array of negate flags to indicate if the value of offsets are negative
    /// @param  pipelineID  The pipeline ID indicating the pipeline that owns the pool
    /// @param  cameraId    CameraId of the metadata from which the list of tags must be fetched. InvalidCameraId if dont care
    ///
    /// @return Result of operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDataListFromMetadataPool(
        const UINT*  pDataList,
        VOID** const ppData,
        UINT64*      pOffsets,
        SIZE_T       length,
        BOOL*        pNegate,
        UINT         pipelineID,
        UINT32       cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraIdForMetadataQuery
    ///
    /// @brief  Returns the cameraId for GetDataList() API
    ///
    /// @return cameraId
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetCameraIdForMetadataQuery();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CacheVendorTagLocation
    ///
    /// @brief  Caches the vendor tag location
    ///
    /// @return CamxResultSuccess if successful, error codes otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CacheVendorTagLocation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetStreamDataForOutputPort
    ///
    /// @brief  Set StreamData for Output port
    ///
    /// @param  outputPortIndex Output port index
    /// @param  pStreamData     SteamData containing stream related values
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetStreamDataForOutputPort(
        UINT        outputPortIndex,
        StreamData* pStreamData)
    {
        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        if (pStreamData->portFPS > pOutputPort->streamData.portFPS)
        {
            pOutputPort->streamData.portFPS = pStreamData->portFPS;
        }

        // update stream type bitmask for both node and port specific variable
        pOutputPort->streamData.streamTypeBitmask |= pStreamData->streamTypeBitmask;
        m_streamTypeBitmask                       |= pStreamData->streamTypeBitmask;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetStreamDataForOutputPorts
    ///
    /// @brief  Set StreamData for all ports by tracing back on ports in the path of the stream
    ///
    /// @param  outputPortIndex  Output port index (in the output ports array)
    /// @param  pStreamData      SteamData containing stream related values
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetStreamDataForOutputPorts(
        UINT        outputPortIndex,
        StreamData* pStreamData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUBWCusageflags
    ///
    /// @brief  Set UBWC format params
    ///
    /// @param  pOutputPort         pointer to output port
    /// @param  pFormat             Format pointer
    /// @param  pFormatParamInfo    pointer to FormatParamInfo
    ///
    /// @return BOOL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetUBWCusageflags(
        OutputPort*      pOutputPort,
        ImageFormat*     pFormat,
        FormatParamInfo* pFormatParamInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLastFlushedRequestId
    ///
    /// @brief  Get last flushed requestId of the pipeline
    ///
    /// @return return last flushed requestId of the pipeline
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT64 GetLastFlushedRequestId()
    {
        FlushInfo flushInfo;

        GetFlushInfo(flushInfo);

        return flushInfo.lastFlushRequestId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroyPSMQueue
    ///
    /// @brief  Method to destroy PSM queues and entries across all tags
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID DestroyPSMQueue()
    {
        if (NULL != m_pPSMQueue)
        {
            for (UINT portIdx  = 0; portIdx < m_outputPortsData.numPorts; ++portIdx)
            {
                PSMQueue* pQueue = &m_pPSMQueue[portIdx];

                if (NULL != pQueue->pEntry)
                {
                    CAMX_DELETE[] pQueue->pEntry;
                }
            }
            CAMX_DELETE[] m_pPSMQueue;
            m_pPSMQueue = NULL;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DisableDelayedUnmapRequired
    ///
    /// @brief  Whether Disabling Delayed Unamp feature required while importing these buffers.
    ///         Applicable only for Gralloc (HALGralloc or ChiGralloc) buffers.
    ///         With DelayedUnmap feature kernel smmu driver will keep the mappings even though camx releases the buffer.
    ///         This will help in reducing/avoiding map latency if the same buffer is used for a future request.
    ///         So, not disabling this feature (Default) would have KPI improvement, exa - for preview, video framework buffers.
    ///         When disabled, kernel will immediately unmap the buffers once buffer is released. So, if the buffer is used for
    ///         a future request, map will actually happen again at that time.
    ///         We might need to disable this feature in some cases where by keeping delayed maps for buffers that are already
    ///         released by camx, we run out of smmu memory address space for a new buffer to map.
    ///         Note - Be cautious while disabling this feature.
    ///         Currently identified cases where we need to disable DelayedUnmap feature to run the usecase by taking a hit
    ///         in KPI :
    ///         1. For IPE snapshot buffers of larger size. Exa, in QuadCFA 108M snapshot, where IPE imports Gralloc buffers.
    ///         2. 8K EIS3 video use case - framework video buffers.
    ///
    /// @param  flags           Current flags to import buffer
    /// @param  pImageFormat    ImageFormat info of the buffer being imported
    ///
    /// @return TRUE if disable required, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL DisableDelayedUnmapRequired(
        UINT32              flags,
        const ImageFormat*  pImageFormat)
    {
        BOOL disableDelayedUnmapRequired = FALSE;

        if (((BPS == Type()) || (IPE == Type()))                                                        &&
            (0                                                      != (flags & CSLMemFlagHw))          &&
            (GetStaticSettings()->MPMSizeThresToDisableDelayedUnmap <   ImageFormatUtils::GetTotalSize(pImageFormat)))
        {
            disableDelayedUnmapRequired = TRUE;
        }

        if ((0    != (flags & CSLMemFlagHw)) &&
            (TRUE == GetStaticSettings()->MPMForceDisableDelayedUnmap))
        {
            disableDelayedUnmapRequired = TRUE;
        }

        return disableDelayedUnmapRequired;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DelayedMapRequired
    ///
    /// @brief  Whether mapping of Gralloc buffers need to be delayed. By default, Gralloc (HALGralloc or ChiGralloc)
    ///         buffers are mapped immediately on ProcessRequest as the actual buffers are already allocated for
    ///         Gralloc buffers by this time even though those buffers may not be used immediately.
    ///         But this may sometimes increase number of mappings on smmu page table unnecessarily causing
    ///         non availability of smmu space for the buffers that are actually required at this point of time.
    ///         So, by delaying mapping of buffers in particular cases helps in reducing usage space on smmu table
    ///         with some compromise on latency. Such delayed mappings are mapped whenever corresponding node's all
    ///         dependencies are met, thus by causing 'map' latency after dependencies are met.
    ///         Note - Be cautious while delaying Maps.
    ///         Currently identified cases where we need to delay mapping to run the usecase by taking a hit in KPI :
    ///         1. 4K or 8K EIS3 (based on number of future frames) + depending upon live snapshot usage.
    ///
    /// @param  flags           Current flags to import buffer
    /// @param  pImageFormat    ImageFormat info of the buffer being imported
    ///
    /// @return TRUE if delay required, FALSE if not
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL DelayedMapRequired(
        UINT32              flags,
        const ImageFormat*  pImageFormat)
    {
        BOOL delayedMapRequired = FALSE;

        // Better to check number of lookahead frames also to better determine whether delayed map is required.
        // For exa - 8K with 15 frame lookahead may not need delayed map, whereas 8K with 30 frame lookahead needs delayed map.
        if ((TRUE                                           == IsEIS3Node())                &&
            (0                                              != (flags & CSLMemFlagHw))      &&
            (GetStaticSettings()->MPMSizeThresToDelayedMap  <  ImageFormatUtils::GetTotalSize(pImageFormat)))
        {
            delayedMapRequired = TRUE;
        }

        return delayedMapRequired;
    }

    Node(const Node&) = delete;                                       ///< Disallow the copy constructor
    Node& operator=(const Node&) = delete;                            ///< Disallow assignment operator

    NodeFlags              m_nodeFlags;                                ///< Node flags
    UINT                   m_nodeType;                                 ///< Node type
    UINT                   m_instanceId;                               ///< Node instance id
    CHAR                   m_nodeIdentifierString[MaxStringLength256]; ///< Pipeline Name, Node name and instance id
    UINT                   m_pipelineNodeIndex;                        ///< This instance's index in the array of pipeline nodes
    AllOutputPortsData     m_outputPortsData;                          ///< Node output port data - Do not remove from private
    AllInputPortsData      m_inputPortsData;                           ///< Node input port data - Do not remove from private
    Pipeline*              m_pPipeline;                                ///< Pipeline object pointer
    BOOL                   m_isIPERealtime;                            ///< is IPE in RealTime
    DeferredRequestQueue*  m_pDRQ;                                     ///< DRQ pointer
    INT32                  m_deviceIndices[CamxMaxDeviceIndex];        ///< Array of device indices
                                                                       ///  that is acquired by this node
    UINT                   m_cslDeviceCount;                           ///< The number of rt devices acquired by this node
    CSLDeviceHandle        m_hCSLDeviceHandles[CamxMaxDeviceIndex];    ///< Array of CSL device handle to be linked

    MetadataPool*          m_pInputPool;                               ///< Pointer to input metadata pool

    MetadataPool*          m_pMainPool;                                ///< Pointer to main metadata/property pool
    MetadataPool*          m_pEarlyMainPool;                           ///< Pointer to main metadata/property pool
    MetadataPool*          m_pInternalPool;                            ///< Pointer to internal property pool
    MetadataPool*          m_pUsecasePool;                             ///< Pointer to per usecase property pool
    MetadataPool*          m_pDebugDataPool;                           ///< Pointer to Debug-data metadata pool
    ThreadManager*         m_pThreadManager;                           ///< Pointer to Thread Manager
    UINT                   m_deviceIndexCount;                         ///< The number of devices acquired by this node
    CmdBufferManager**     m_ppCmdBufferManagers;                      ///< Array of cmd managers,allocated by subclassed HW
                                                                       ///  node
    UINT                   m_numCmdBufferManagers;                     ///< Number of cmd managers in m_pCmdManagers
    UINT                   m_maxNumCmdBufferManagers;                  ///< Max require number of command buffer managers
    NodePerRequestInfo     m_perRequestInfo[MaxRequestQueueDepth];     ///< Per request info
    UINT                   m_maxOutputPorts;                           ///< Max output ports(provided by derived node class)
    UINT                   m_maxInputPorts;                            ///< Max input ports(provided by derived node class)
    UINT                   m_inputPortDisableMask;                     ///< Input port disabled by node override mask
    JobHandle              m_hThreadJobFamilyHandle;                   ///< Handle to the registered job family.
    BufferNegotiationData  m_bufferNegotiationData;                    ///< BufferNegotiation data
    ChiContext*            m_pChiContext;                              ///< Chi Context pointer
    HwContext*             m_pHwContext;                               ///< HwContext pointer
    ExternalComponentInfo  m_nodeExtComponents[MaxExternalComponents]; ///< Node external component name
    UINT                   m_nodeExtComponentCount;                    ///< Node external component count
    ContingencyInducer*    m_pContingencyInducer;                      ///< Contingency Inducer pointer
    Mutex*                 m_pCmdBufferManagersLock;                   ///< Mutex to protect Cmd Buffers

    /// @todo (CAMX-2905)  Optimize all nodes to be re-entrant
    Mutex*                 m_pProcessRequestLock;                      ///< Mutex to protect node execution state
    Mutex*                 m_pBufferRequestLock;                       ///< Mutex to protect buffer request and release
    Mutex*                 m_pBufferReleaseLock;                       ///< Mutex to protect buffer state
    Mutex*                 m_pFenceCreateReleaseLock;                  ///< Mutex to protect node fence create/release state
    UINT64                 m_CSLSyncID[MaxRequestBufferDepth];         ///< CSL SyncID to synchronize links in CRM
    WatermarkPattern*      m_pWatermarkPattern;                        ///< Contains the info to watermark the buffer
    BOOL                   m_bHasLoopBackPorts;                        ///< Indicate if node has loopback ports
    NodeMetadataList       m_publishTagArray;                          ///< List of tags published by the node
    FenceErrorBuffer       m_fenceErrors;                              ///< Array of node data related to fence error
    BOOL                   m_bUseMultiCameraMasterMeta;                ///< Flag to use multi-camera metadata
    UINT32                 m_vendorTagMultiCamInput;                   ///< Input Vendor tag for master/slave role switch
    UINT32                 m_vendorTagMultiCamOutput;                  ///< Output Vendor tag for master/slave role switch

    BufferGroup            m_bufferComposite;                          ///< Variable to cache the buffer composite info
    UINT32                 m_streamMapVendorTag;                       ///< Vendor tag location for stream map
    UINT32                 m_allCropInfoVendorTag;                     ///< Vendor tag location for allCropInfo
    UINT32                 m_requestCropInfoVendorTag;                 ///< Vendor tag location for request crop info

    CompositePortFenceInfo  m_portFenceInfo[RequestQueueDepth][MaxBufferComposite];   ///< List of composite fences for each
                                                                                      /// port for request queue depth
    PSMQueue*               m_pPSMQueue;                                ///< Array of circular buffers containing Meta link info
    PSMInfo                 m_pPSMData[MaxPSMetaType];                  ///< PSM information for all datatypes
    UINT32                  m_activePixelArrayWidth;                    ///< Sensor active array width
    UINT32                  m_activePixelArrayHeight;                   ///< Sensor active array height

    UINT32                 m_responseTimeSum[MaxProcessSequenceIdForTiming]
                           [static_cast<UINT>(NodeStage::MaxNodeStatuses)]; ///< The sum of the reponse time for each node stage
    UINT32                 m_numberOfNodeResponses[MaxProcessSequenceIdForTiming]
                           [static_cast<UINT>(NodeStage::MaxNodeStatuses)]; ///< The total number of responses recorded
    UINT32                 m_seqIdNodeStartTime;                            ///< The start time of when we start measuring
                                                                            ///  sequence id performance
    UINT8                  m_streamTypeBitmask;                             ///< Bitmask containing supported stream type flags
    OSThreadHandle         m_hThreadPipelineCreate;                         ///< The thread handle of DeferPipelineCreation

    CAMX_TLS_STATIC_CLASS_DECLARE(UINT64, m_tRequestId);               ///< Thread request id
    BOOL                   m_skipUpdatingBufferProperties;             ///< Skip updating buffer properties
    UINT32                 m_vendorTagEndOfStreamFlag;                 ///< Input Vendor tag for master/slave role switch
    UINT32                 m_vendorTagEndOfStreamRequestId;            ///< Output Vendor tag for master/slave role switch
    StreamCropInfo*        m_pHalStreamCrop;                           ///< Pointer to hal stream crop info for a given node

    UINT32                 m_streamDimensionVideoVendorTag;            ///< Vendor tag location for video stream dimension
    UINT32                 m_streamDimensionPreviewVendorTag;          ///< Vendor tag location for preview stream dimension
    UINT32                 m_EISEnabledVendorTag;                      ///< Vendor tag location for EIS enable setting
    UINT32                 m_EISMinTotalMarginVendorTag;               ///< Vendor tag location for EIS min total margin
    UINT32                 m_IFEAppliedCropTag;                        ///< Vendor tag location for IFE applied crop
};

CAMX_NAMESPACE_END

#endif // CAMXNODE_H
