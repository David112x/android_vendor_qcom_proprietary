////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2types.h
/// @brief CHI feature2 type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2TYPES_H
#define CHIFEATURE2TYPES_H

#include <vector>

#include "chxincs.h"
#include "chxutils.h"
#include "camxcdktypes.h"
#include "cdkutils.h"

// NOWHINE FILE GR017:  CHI files may overload operators in exceptional cases
// NOWHINE FILE CF003:  CHI files may overload operators in exceptional cases
// NOWHINE FILE CP003:  CHI files may overload operators in exceptional cases
// NOWHINE FILE CP004:  Need comparator for map
// NOWHINE FILE CP008:  Need comparator for map
// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Foward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFeature2Base;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Feature Base Class Types
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef VOID* CHIFEATUREREQUESTOBJECTHANDLE;
typedef VOID* CHIFEATURE2HANDLE;
typedef VOID* ChiFeature2MessageHandle;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ChiFeature2Messages;
class ChiFeature2UsecaseRequestObject;
enum class ChiFeature2Type;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constant Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT   DebugDataMaxOfflineFrames   = 32;   ///< Max offline debug-data frame capability for multi-frame processing

/// @brief Feature port handle type
enum class ChiFeature2HandleType
{
    PortDescriptor,             ///< Handle is of type ChiFeature2PortDescriptor.
    BufferMetaInfo,             ///< Handle is of type ChiFeature2BufferMetadataInfo.
    PortBufferMetaInfo,         ///< Handle is of type ChiFeature2RequestOutputInfo.
    DependencyConfigInfo,       ///< Handle is of type ChiFeature2DependencyConfigInfo.
    DependencyConfigDescriptor, ///< Handle is of type ChiFeature2DependencyConfigDescriptor.
};

/// @brief Feature Pipeline Info
struct ChiFeature2PipelineInfo
{
    UINT8                           pipelineId; ///< Pipeline Id.
    UINT8                           numHandles; ///< Number of ports held by the pipeline.
    ChiFeature2HandleType           type;       ///< Port handle type.
    CHIFEATURE2HANDLE               handle;     ///< Port handle as described by the type.
};

/// @brief Feature Session Info
struct ChiFeature2SessionInfo
{
    UINT8                       sessionId;      ///< Session Id.
    UINT8                       numPipelines;   ///< Number of Pipelines.
    ChiFeature2PipelineInfo*    pPipelineInfo;  ///< Pointer to PipelineInfo array.
};

/// @brief Feature port direction type
enum class ChiFeature2PortDirectionType
{
    InternalInput,   ///< Feature internal input port type.
    ExternalInput,   ///< Feature external input port type.
    InternalOutput,  ///< Feature internal output port type.
    ExternalOutput,  ///< Feature external output port type.
};

/// @brief Port Buffer status
enum class ChiFeature2PortBufferStatus
{
    InvalidBufferStatus,      ///< Buffer status not set
    DependencyPending,        ///< Port is waiting for buffer
    DependencyMetWithError,   ///< Port dependency met with error
    DependencyMetWithSuccess, ///< Port dependency met with success
    DependencyReleased,       ///< Port dependency released
};

/// @brief Feature port type
enum class ChiFeature2PortType
{
    ImageBuffer,   ///< Feature internal input port type.
    MetaData,          ///< Feature external input port type.
};

/// @brief Pipeline type
enum class ChiFeature2PipelineType
{
    CHI,        ///< CHI pipeline.
    Virtual,    ///< Virtual pipeline.
};

/// @brief Prune group to differentiate different camera types
struct ChiFeature2PruneGroup
{
    static const UINT8 INVALIDGROUP = 0x0;        ///< Invalid group
    static const UINT8 SingleZone   = 0x01;       ///< Single zone
    static const UINT8 DualZone     = 0x02;       ///< Dual zone
};


/// @brief Target descriptor
struct ChiFeature2TargetDescriptor
{
    const CHAR* pTargetName;    ///< Target name.
};

/// @brief Feature identity information
struct ChiFeature2Identifier
{
    UINT8                        session;             ///< Session Index.
    UINT8                        pipeline;            ///< Pipeline Index.
    UINT8                        port;                ///< Port Index.
    ChiFeature2PortDirectionType portDirectionType;   ///< Port Direction (Internal / External) type.
    ChiFeature2PortType          portType;            ///< Port (Buffer/Meta)type

    // NOWHINE DC002a: Need comparison operator for convenient comparison
    bool operator==(const ChiFeature2Identifier& rRHS) const
    {
        return (session           == rRHS.session  &&
                pipeline          == rRHS.pipeline &&
                port              == rRHS.port     &&
                portDirectionType == rRHS.portDirectionType &&
                portType          == rRHS.portType);
    }

    // NOWHINE DC002a: Need comparison operator for convenient comparison
    bool operator!=(const ChiFeature2Identifier& rRHS) const
    {
        return (session           != rRHS.session  ||
                pipeline          != rRHS.pipeline ||
                port              != rRHS.port     ||
                portType          != rRHS.portType);
    }
};

/// @brief Comparator function for port ids
struct ChiFeature2PortIdLessComparator :
    public std::binary_function<ChiFeature2Identifier, ChiFeature2Identifier, bool>
{
    bool operator()(const ChiFeature2Identifier LHS, const ChiFeature2Identifier RHS) const
    {
        BOOL isLHSLessThanRHS = FALSE;

        if (LHS.session < RHS.session)
        {
            isLHSLessThanRHS = TRUE;
        }
        else if ((LHS.session  == RHS.session) &&
                 (LHS.pipeline <  RHS.pipeline))
        {
            isLHSLessThanRHS = TRUE;
        }
        else if ((LHS.session  == RHS.session)  &&
                 (LHS.pipeline == RHS.pipeline) &&
                 (LHS.port     <  RHS.port))
        {
            isLHSLessThanRHS = TRUE;
        }
        else if ((LHS.session  == RHS.session)  &&
                 (LHS.pipeline == RHS.pipeline) &&
                 (LHS.port     == RHS.port)     &&
                 (LHS.portDirectionType <  RHS.portDirectionType))
        {
            isLHSLessThanRHS = TRUE;
        }
        else if ((LHS.session  == RHS.session)  &&
                 (LHS.pipeline == RHS.pipeline) &&
                 (LHS.port     == RHS.port)     &&
                 (LHS.portDirectionType ==  RHS.portDirectionType) &&
                 (LHS.portType < RHS.portType))
        {
            isLHSLessThanRHS = TRUE;
        }


        return isLHSLessThanRHS;
    }
};

/// @brief Comparator function for port ids
struct ChiFeature2StringLessComparator :
    public std::binary_function<const CHAR*, const CHAR*, bool>
{
    bool operator()(const CHAR* LHS, const CHAR* RHS) const
    {
        return CdkUtils::StrCmp(LHS, RHS) < 0;
    }
};

/// @brief Feature port descriptor
struct ChiFeature2PortDescriptor
{
    ChiFeature2Identifier               globalId;           ///< Port global Identifier
    const CHAR*                         pPortName;          ///< Port name.
    const ChiFeature2TargetDescriptor*  pTargetDescriptor;  ///< Pointer to Target descriptor for this port
};

/// @brief Feature pipeline descriptor
struct ChiFeature2PipelineDescriptor
{
    UINT8                               sessionId;              ///< Session Unique ID in the feature.
    UINT8                               pipelineId;             ///< Pipeline unique ID in the feature.
    const CHAR*                         pPipelineName;          ///< Pipeline name.
    ChiFeature2PipelineType             pipelineType;           ///< Pipeline (CamX / CHI) type.
    UINT8                               numInputPorts;          ///< Number of ports in this pipeline
    const ChiFeature2PortDescriptor*    pInputPortDescriptor;   ///< Pointer to input port descriptor
    UINT8                               numOutputPorts;         ///< Number of output Ports
    const ChiFeature2PortDescriptor*    pOutputPortDescriptor;  ///< Pointer to output port descriptor
};

/// @brief Feature session descriptor
struct ChiFeature2SessionDescriptor
{
    UINT8                                sessionId;      ///< Session unique ID in the feature.
    const CHAR*                          pSessionName;   ///< Session name.
    UINT8                                numPipelines;   ///< Number of pipelines in this session.
    const ChiFeature2PipelineDescriptor* pPipeline;      ///< Pointer to pipeline descriptor table in this session.
};

/// @brief Feature input dependency descriptor
struct ChiFeature2InputDependency
{
    UINT8                               numInputDependency; ///< Total number of input dependencies.
    const ChiFeature2PortDescriptor*    pInputDependency;   ///< Pointer to input dependency descriptor table.
};

/// @brief Feature dependency descriptor
struct ChiFeature2DependencyConfigDescriptor
{
    UINT8                               numInputDependency;   ///< Array size for list of input dependencies.
    ChiFeature2InputDependency*         pInputDependencyList; ///< Pointer to input dependency list.
    UINT8                               numInputConfig;       ///< Total number of input configurations.
    const ChiFeature2PortDescriptor*    pInputConfig;         ///< Pointer to array of input configuration descriptor table.
    UINT8                               numOutputConfig;      ///< Total number of output configurations.
    const ChiFeature2PortDescriptor*    pOutputConfig;        ///< Pointer to array of output configuration descriptor table.
};

/// @brief Feature stage descriptor
struct ChiFeature2StageDescriptor
{
    UINT8                           stageId;                        ///< Stage unique ID.
    const CHAR*                     pStageName;                     ///< Stage name.
    UINT8                           numDependencyConfigDescriptor;  ///< Number of input dependencies.
    const ChiFeature2SessionInfo*   pDependencyConfigDescriptor;    ///< Pointer to input dependency descriptor table.
};

/// @brief The container for the source and sink ports that describe a link between two internal ports in a feature
struct ChiFeature2InternalLinkDesc
{
    const ChiFeature2PortDescriptor*  pSrcPort;   ///< Source port for link
    const ChiFeature2PortDescriptor*  pSinkPort;  ///< Sink port for link
};

/// @brief Feature descriptor
struct ChiFeature2Descriptor
{
    UINT32                              featureId;          ///< Feature unique ID.
    const CHAR*                         pFeatureName;       ///< Feature name.
    UINT8                               numStages;          ///< Number of stages described for the feature.
    const ChiFeature2StageDescriptor*   pStages;            ///< Pointer to table to stages.
    UINT8                               numSessions;        ///< Number of sessions in this feature.
    const ChiFeature2SessionDescriptor* pSession;           ///< Pointer to session descriptor table.
    UINT8                               numInternalLinks;   ///< Number of internal links.
    const ChiFeature2InternalLinkDesc*  pInternalLinkDesc;  ///< Pointer to internal link desc
};

/// @brief The structure of a feature2 property
struct ChiFeature2Property
{
    UINT32      propertyId;      ///< ID of the property
    const CHAR* pPropertyName;   ///< Name of the property
    UINT32      size;            ///< size of property value
    const VOID* pValue;          ///< Pointer to data. Type dependent on ID
};

/// @brief Flags of a feature2 instance
union ChiFeature2InstanceFlags
{
    struct
    {
        UINT isNZSLSnapshot       : 1;      ///< Is it for non-zsl snapshot with different sensor output size
        UINT isSWRemosaicSnapshot : 1;      ///< Is it for SWRemosaic type snapshot
        UINT isHWRemosaicSnapshot : 1;      ///< Is it for HWRemosaic type snapshot
        UINT isBPSCamera          : 1;      ///< Is it for BPS Realtime camera
        UINT reserved             : 28;     ///< Reserved
    };

    UINT value;
};

/// @brief The container for all properties associated with a feature instance
struct ChiFeature2InstanceProps
{
    UINT32                   instanceId;               ///< ID of the feature instance
    UINT32                   cameraId;                 ///< ID of the camera associated with this feature instance
    UINT32                   numFeatureProps;          ///< Number of feature properties
    ChiFeature2Property*     pFeatureProperties;       ///< Array of feature properties
    ChiFeature2InstanceFlags instanceFlags;            ///< feature2 instance flags

    // NOWHINE DC002a: Need comparison operator for convenient comparison
    bool operator==(const ChiFeature2InstanceProps& rRHS) const
    {
        return ((instanceId == rRHS.instanceId) &&
                (instanceFlags.value == rRHS.instanceFlags.value));
    }
};

/// @brief The structure used to identify a unique feature instance
struct ChiFeature2InstanceId
{
    UINT32                   featureId;     ///< Feature Id
    UINT32                   instanceId;    ///< Instance Id
    UINT32                   cameraId;      ///< Camera Id
    ChiFeature2InstanceFlags flags;         ///< feature flags
};

/// @brief The container for all data associated with a feature instance
struct ChiFeature2InstanceDesc
{
    const ChiFeature2Descriptor*    pFeatureDesc;   ///< The descriptor associated with this feature instance
    ChiFeature2InstanceProps*       pInstanceProps; ///< The properties associated with this feature instance
};

/// @brief The container for all data associated with prune information
struct ChiFeature2PrunableVariant
{
    UINT8                       pruneGroup;         ///< Prune Group
    ChiFeature2Type             pruneVariantType;   ///< Prune Variant

    // NOWHINE DC002a: Need comparison operator for convenient comparison
    bool operator==(const ChiFeature2PrunableVariant& rRHS) const
    {
        return (((pruneGroup & rRHS.pruneGroup) != ChiFeature2PruneGroup::INVALIDGROUP) &&
                (pruneVariantType == rRHS.pruneVariantType));
    }
};

/// @brief The type of the feature graph link
enum class ChiFeature2GraphLinkType
{
    ExternalSource, ///< Link connects the external source target to the input of a feature
    Internal,       ///< Link connects the output of one feature to the input of another feature
    ExternalSink    ///< Link connects the output of a feature to an external sink target
};

/// @brief The container for all data associated with rules used to prune feature descriptor
struct ChiFeature2PruneRule
{
    ChiFeature2PrunableVariant  prunableVariant;    ///< Prunable variant
    UINT32                      cameraId;           ///< Camera id for which prune rules are provided
    ChiFeature2GraphLinkType    linkType;           ///< Prune rule applicable for what link
};

/// @brief A globally unique identifier for ports, based on feature instance data and port identifier
struct ChiFeature2GlobalPortInstanceId
{
    UINT32                      featureId;      ///< The unique ID of the feature
    ChiFeature2InstanceProps    instanceProps;  ///< Properties to uniquely identify the feature instance
    ChiFeature2Identifier       portId;         ///< The feature-unique ID of the port

    // NOWHINE DC002a: Need comparison operator for convenient comparison
    bool operator==(const ChiFeature2GlobalPortInstanceId& rRHS) const
    {
        return ((featureId      == rRHS.featureId) &&
                (instanceProps  == rRHS.instanceProps)  &&
                (portId         == rRHS.portId));
    }
};

/// @brief Container for buffer and metadata information
struct ChiFeature2BufferMetadataInfo
{
    CHITARGETBUFFERINFOHANDLE   hBuffer;                ///< Buffer handle.
    BOOL                        bufferErrorPresent;     ///< Is buffer error present for this buffer handle
    BOOL                        bufferSkipped;          ///< Skip generating this buffer,
                                                        ///  will return as error buffer for framework output buffer
    UINT64                      key;                    ///< Unique key to extract buffer from handle
    CHIMETAHANDLE               hMetadata;              ///< Metadata handle output from feature. Not used for input.
    BOOL                        metadataErrorPresent;   ///< Is metadata error present for this metadata handle
};

/// @brief Container for port, buffer, and metadata information associated with the output for the request
struct ChiFeature2RequestOutputInfo
{
    const ChiFeature2PortDescriptor* pPortDescriptor;       ///< Pointer to feature port descriptor.
    ChiFeature2BufferMetadataInfo    bufferMetadataInfo;    ///< Buffer/metadata handle information.
};

/// @brief Container for port, buffer, and metadata information
struct ChiFeature2PortBufferMetadataInfo
{
    UINT8                            num;                   ///< Number of ports.
    const ChiFeature2PortDescriptor* pPortDescriptor;       ///< Pointer to array of feature port descriptor.
    CHITARGETBUFFERINFOHANDLE*       phTargetBufferHandle;  ///< Pointer to array of target buffer handles.
    BOOL*                            pBufferErrorPresent;   ///< Pointer array indicating if a buffer error is present for each
    ChiFeature2PortBufferStatus*     pPortBufferStatus;     ///< Pointer array indicating port buffer status
    UINT64*                          pKey;                  ///< Pointer to array unique key to extract buffer from handle.
    CHIMETAHANDLE*                   phMetadata;             ///< Metadata handle.
};

/// @brief Feature metadata information
struct ChiFeature2MetadataInfo
{
    UINT8                            num;                   ///< Number of metadata.
    CHIMETAHANDLE*                   phMetadata;            ///< Pointer to array of Metadata handle.
};

/// @brief Feature stage information
struct ChiFeature2StageInfo
{
    UINT8   stageId;            ///< Stage Id
    UINT8   stageSequenceId;    ///< Stage sequenceId
};

/// @brief Feature dependency config information
struct ChiFeature2DependencyConfigInfo
{
    UINT8                               numRequestInput;       ///< Number of request inputs array.
    ChiFeature2MetadataInfo*            pRequestInput;         ///< Pointer to array of input manual request settings(optional).
    UINT8                               numInputDependency;    ///< Number of input dependency array.
    ChiFeature2PortBufferMetadataInfo*  pInputDependency;      ///< Pointer to array of input dependency
                                                               /// (port/buffer/metadata bundle).
    ChiFeature2PortBufferMetadataInfo   inputConfig;           ///< Pointer to input configuration(port/buffer/metadata).
    ChiFeature2PortBufferMetadataInfo   outputConfig;          ///< Pointer to output configuration(port/buffer/metadata).
    UINT8                               numEnabledInputPorts;  ///< Number of enabled input ports
    UINT8                               numEnabledOutputPorts; ///< Number of enabled output ports
};

/// @brief List of port identifiers
struct ChiFeature2PortIdList
{
    UINT8                          numPorts;               ///< Num of port identifiers.
    const ChiFeature2Identifier*   pPorts;                 ///< Array of port identifiers.
};

/// @brief capture modes
union ChiFeature2CaptureModes
{
    struct
    {
        BIT Manual          : 1;                       ///< Manual mode Capture
        BIT LowLight        : 1;                       ///< Low light Capture
        BIT InSensorHDR3Exp : 1;                       ///< In-sensor HDR 3 exposure capture
        BIT Reserved        : 29;                      ///< Reserved
    } u;
    UINT32 value;
};

/// @brief A feature-specific custom hint from the usecase
struct ChiFeature2Hint
{
    UINT                            numFrames;              ///< Number of frames to process from
                                                            ///  the feature graph to the feature class.
    ChiFeature2CaptureModes         captureMode;            ///< Capture Modes
    std::vector<UINT8>              stageSequenceInfo;      ///< Total number of sequences for a stage and number
                                                            ///  of frames per stage sequence
    UINT32                          numEarlyPCRs;           ///< Number of PCRs to handle before stream on
    VOID*                           pOEMChiFeatureHint;     ///< A feature specific custom hint from
                                                            ///  the feature graph to the feature class from OEM.
};

/// @brief Contains information related to the specific feature instance for the given request
struct ChiFeature2InstanceRequestInfo
{
    ChiFeature2Base*    pFeatureBase;   ///< The specific feature instance
    ChiFeature2Hint     featureHint;    ///< The hint associated with this feature instance for the given request
};

/// @brief Message types
enum class ChiFeature2MessageType
{
    GetInputDependency,             ///< Get input dependency
    ReleaseInputDependency,         ///< Release input dependency
    ResultNotification,             ///< Result notification
    MetadataNotification,           ///< Metadata notification
    PartialMetadataNotification,    ///< Partial metadata notification
    ProcessingDependenciesComplete, ///< Processing input dependencies is complete
    MessageNotification,            ///< Message or event notification from feature
    SubmitRequestNotification       ///< Notification for usecase to submit request

};

/// @brief Feature2 data callback result structure
struct ChiFeature2Result
{
    UINT8                          resultIndex;            ///< Result index to which these ports belong
    UINT8                          numPorts;               ///< Number of port identifiers
    const ChiFeature2Identifier*   pPorts;                 ///< Array of port identifiers
};

/// @brief Feature2 data message result structure
struct ChiFeature2Dependency
{
    UINT8                          dependencyIndex;        ///< Dependency index to which these ports belong
    UINT8                          numPorts;               ///< Number of port identifiers
    ChiFeature2Identifier*         pPorts;                 ///< Array of port identifiers
};

/// @brief Dependency Batch
struct ChiFeature2DependencyBatch
{
    UINT8                          batchIndex;             ///< Batch index to which these dependencies belong
    UINT8                          numDependencies;        ///< Number of dependencies
    ChiFeature2Dependency*         pDependencies;          ///< Array of dependencies
};

/// @brief Structure to hold dependency batch
struct ChiFeature2DependencyTable
{
    UINT8                          numBatches;              ///< Number of batch requests
    ChiFeature2DependencyBatch*    pBatches;                ///< Array of batch requests
};

/// @brief Message types
enum class ChiFeature2ChiMessageType
{
    AnchorPick,      ///< Anchor picked frame
    SOF,             ///< SOF message
    ZSL              ///< ZSL message
};

/// @brief message/event from feature2 to usecase
struct ChiFeature2AnchorSyncData
{
    // ChiFeature2ChiMessageType messageType;     ///< Chi message data type
    UINT8                     numberOfFrames;  ///< Number of frames
    UINT8                     anchorFrameIdx;  ///< Anchor frame index
};

/// @brief message/event from feature2 to usecase
struct ChiFeature2ZSLData
{
    UINT64                    lastFrameNumber;  ///< FrameNumber of last picked ZSL buffer
};

/// @brief Chi Message descriptor structure
struct ChiFeature2ChiMessageNotification
{
    ChiFeature2ChiMessageType      messageType;     ///< Chi message data type
    union
    {
        ChiFeature2AnchorSyncData  anchorData;      ///< anchor data associated with message type
        ChiFeature2ZSLData         ZSLData;         ///< ZSL data associated with message type
    } message;

    VOID*                          pPrivData;       ///< Feature request object as private data
};

/// @brief Message descriptor structure
struct ChiFeature2MessageDescriptor
{
    ChiFeature2MessageType      messageType;                        ///< Message type that determines
                                                                    ///  what to use from the union below
    union
    {
        ChiFeature2DependencyTable          getDependencyData;      ///< Dependency table data
        ChiFeature2DependencyBatch          releaseDependencyData;  ///< Release dependency data
        ChiFeature2Result                   result;                 ///< Result data
        ChiFeature2ChiMessageNotification   notificationData;       ///< Data associated with message type
        ChiPipelineRequest                  submitRequest;          ///< Submit request data
    } message;

    VOID*                            pPrivData;              ///< Feature request object as private data
};

/// @brief The callback methods provided by the feature graph manager and called by the feature graph
struct ChiFeature2GraphManagerCallbacks
{
    /// @brief Callback method to send back the capture results to feature graph manager
    void(*ChiFeatureGraphProcessResult)(
        CHICAPTURERESULT*                pCaptureResult,
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObject,
        VOID*                            pPrivateCallbackData);

    /// @brief Callback method to send back the asynchronous messages to feature graph manager
    CDKResult(*ChiFeatureGraphProcessMessage)(
        const ChiFeature2Messages*  pMessageDesc,
        VOID*                       pPrivateCallbackData);

    /// @brief Callback method to send back the partial capture results to the FGM
    void(*ChiFeatureGraphProcessPartialResult)(
        CHIPARTIALCAPTURERESULT*    pPartialCaptureResult,
        VOID*                       pPrivateCallbackData);

    /// @brief Callback method to send back the Flush to the FGM
    void(*ChiFeatureGraphFlush)(
        UINT32      featureId,
        CDKResult   result,
        VOID*       pPrivateCallbackData);

    VOID* pPrivateCallbackData; ///< The data passed back to the feature graph manager during a callback
};

/// @brief Feature2 CHI notification structure
struct ChiFeature2Notification
{
    UINT8                               requestIndex;                   ///< Request Index associated with the message
    const ChiFeature2Identifier*        pIdentifier;                    ///< Identifier associated with the message
    ChiFeature2GraphManagerCallbacks    featureGraphManagerCallbacks;   ///< Feature Graph Manager notification callback
    const CHIMESSAGEDESCRIPTOR*         pChiMessages;                   ///< Chi message descriptor
};

/// @brief Feature2 data callback structure
struct ChiFeature2Messages
{
    UINT32                                       featureId;            ///< Feature Id
    ChiFeature2Notification                      chiNotification;      ///< Chi notification info
    const ChiFeature2MessageDescriptor*          pFeatureMessages;     ///< Feature message info
};

#endif // CHIFEATURE2TYPES_H
