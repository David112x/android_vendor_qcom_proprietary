////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2requestobject.h
/// @brief CHI feature2 request object class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2REQUESTOBJECT_H
#define CHIFEATURE2REQUESTOBJECT_H

#include "chibinarylog.h"
#include "chifeature2types.h"
#include "chifeature2usecaserequestobject.h"
#include "chxincs.h"
#include "chxutils.h"

// NOWHINE FILE CP006:  used standard libraries for performance improvements

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFeature2Base;
class ChiFeature2Graph;
class ChiFeature2RequestObject;

/// @brief Invalid Process Sequence ID.
static const INT InvalidProcessSequenceId      = -1;

/// @brief Invalid Stage ID.
static const UINT8  InvalidStageId             = 0xFF;

/// @brief Invalid Stage Sequence ID.
static const INT InvalidStageSequenceId        = -1;

/// @brief Invalid Feature Index.
static const UINT8 InvalidFeatureIndex         = 0xFF;

/// @brief ChiFeature2RequestObjectOpsType translation for map.
static const INT OpTypeMax = 3;

/// @brief 256 character string length
static const UINT32 MaxLength256 = 256;

/// @brief Feature request object state.
enum class ChiFeature2RequestState
{
    Initialized,                     ///< Initialized state.
    ReadyToExecute,                  ///< Posted in job queue & ready to execute.
    Executing,                       ///< Job for this request object is currently being executed.
    InputResourcePending,            ///< Input Resource Pending either from same feature (preceeding process sequence)
                                     ///  or upstream feature.
    InputResourcePendingScheduled,   ///< Input dependencies are satisfied and we posted a job to handle executing phase
    OutputResourcePending,           ///< Output Resource Pending from same feature.
    OutputErrorResourcePending,      ///< Output Resource Pending with error
    OutputNotificationPending,       ///< Output Notification Pending from downstream Feature.
    OutputErrorNotificationPending,  ///< Output Notification Pending with error from downstream Feature.
    Complete,                        ///< Request is completely processed.
    InvalidMax,                      ///< Invalid Max State.
};

/// @brief String name for type ChiFeature2RequestState. This must be in order of ChiFeature2RequestState.
#if __GNUC__
static const CHAR* ChiFeature2RequestStateStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* ChiFeature2RequestStateStrings[] =
#endif // _GNUC__
{
    "Initialized",
    "ReadyToExecute",
    "Executing",
    "InputResourcePending",
    "InputResourcePendingScheduled",
    "OutputResourcePending",
    "OutputErrorResourcePending",
    "OutputNotificationPending",
    "OutputErrorNotificationPending",
    "Complete",
    "Invalid",
};

CHX_STATIC_ASSERT(CHX_ARRAY_SIZE(ChiFeature2RequestStateStrings) ==
    (static_cast<UINT>(ChiFeature2RequestState::InvalidMax) + 1));

/// @brief Valid State Transitons for ChiFeature2RequestStateTable
static constexpr BOOL ChiFeature2RequestStateTable[][static_cast<UINT>(ChiFeature2RequestState::InvalidMax) + 1] =
{
    // Init, RTE  , EXE  , IRP  , IRPS   ORP,   OERP,  ONP  , OENP, COM,   MAX
    { FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE }, // Initialized.
    { FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE }, // ReadyToExecute.
    { FALSE, TRUE,  FALSE, TRUE,  FALSE, TRUE,  FALSE, TRUE,  TRUE, FALSE, TRUE }, // Executing.
    { FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, TRUE, FALSE, TRUE }, // InputResourcePending.
    { FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, TRUE,  FALSE, TRUE, FALSE, TRUE }, // InputResourcePendingScheduled.
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE,  TRUE, FALSE, TRUE }, // OutputResourcePending.
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE,  TRUE, FALSE, TRUE }, // OutputErrorResourcePending.
    { FALSE, FALSE, TRUE,  FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE,  TRUE }, // OutputNotificationPending.
    { TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE, TRUE,  TRUE, TRUE,  TRUE }, // OutputErrorNotificationPending.
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, TRUE }, // Complete.
    { TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  FALSE, TRUE,  TRUE, TRUE,  TRUE }, // InvalidMax.
};

/// @brief Contains the information associated with a single request
struct ChiFeature2RequestMap
{
    UINT8                               requestIndex;           ///< Request index for this output
    UINT8                               numRequestOutputs;      ///< Number of request outputs (ports).
    ChiFeature2RequestOutputInfo*       pRequestOutputs;        ///< Pointer to request output (port) list containing the
                                                                ///  enabled port descriptor and buffer/settings requested
                                                                ///  for the specified request
};

/// @brief Feature request object input information
struct ChiFeature2RequestObjectCreateInputInfo
{
    VOID*                                   pGraphPrivateData;        ///< Feature graph's private context/data for current URO.
    ChiFeature2UsecaseRequestObject*        pUsecaseRequestObj;       ///< Usecase request object (URO).
    ChiFeature2Base*                        pFeatureBase;             ///< Feature base class pointer.
    UINT8                                   numRequests;              ///< Number of Requests
    ChiFeature2RequestMap*                  pRequestTable;            ///< Two-dimensional array of requests and ports
                                                                      ///  per request
    ChiFeature2Hint*                        pFeatureHint;             ///< A feature specific custom hint from the feature graph
                                                                      ///  to the feature class.
    ChiModeUsecaseSubModeType               usecaseMode;              ///< Usecase mode
    const CHAR*                             pGraphName;               ///< Graph name
};

/// @brief Sequence order
enum class ChiFeature2SequenceOrder
{
    Next,       ///< Next process sequence order.
    Current,    ///< Current process sequence order.
};

/// @brief Operation type
enum class ChiFeature2RequestObjectOpsType
{
    RequestInput,           ///< Input manual request settings / buffer information.
    InputDependency,        ///< Input dependency descriptor or (buffer & metadata) information.
    InputConfiguration,     ///< Input configuration descriptor or (buffer & metadata) information.
    OutputConfiguration,    ///< Output configuration descriptor or (buffer & metadata) information.
    ObjectOpsTypeMax        ///< Maximum Type.
};

/// @brief The string name of the type of the memory allocation. Must be in order of PipelineStatus codes.
#if __GNUC__
static const CHAR* ChiFeature2RequestObjectOpsTypeStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* ChiFeature2RequestObjectOpsTypeStrings[] =
#endif // _GNUC__
{
    "RequestInput",
    "InputDependency",
    "InputConfiguration",
    "OutputConfiguration",
    "ObjectOpsTypeMax",
};

CHX_STATIC_ASSERT(CHX_ARRAY_SIZE(ChiFeature2RequestObjectOpsTypeStrings) ==
    (static_cast<UINT>(ChiFeature2RequestObjectOpsType::ObjectOpsTypeMax) + 1));

/// @brief Feature Stage Max information
struct ChiFeature2StageMaxInfo
{
    INT32   maxSequence;    ///< Maximum element size of ProcessSequenceInfo array.
    UINT8   maxSession;     ///< Maximum Session Index for the FID.
    UINT8   maxPipeline;    ///< Maximum Pipeline Index for the FID.
    UINT8   maxPort;        ///< Maximum Port Index for the FID.
};

enum class ChiFeature2InputDependencyStatus
{
    inputDependenciesSatisfied,             ///< Input Dependencies completeley satisfied
    inputDependenciesSatisfiedWithError,    ///< Input Dependencies satisfied, but some dependencies with error
    inputDependenciesNotSatisfied,          ///< Input Dependencies are not satisfied, with no error observed
    inputDependenciesErroredOut,            ///< Input Dependencies are not satisfied, with every dependency having error
    unknownInputDependencyStatus,           ///< The input dependency status is unknown and there is an issue elsewhere
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the Feature Request Object.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiFeature2RequestObject
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create feature request object instance.
    ///
    /// @param  pCreateInputInfo    Feature request object create input information.
    ///
    /// @return Pointer to the ChiFeature2RequestObject upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2RequestObject* Create(
        const ChiFeature2RequestObjectCreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetConfigInfo
    ///
    /// @brief  Set maximum sequence that the FRO will support
    ///
    /// @param  maxSequence      Maximum Sequences
    /// @param  numSessions      Number of Sessions
    /// @param  pSession         Session Descriptor pointer
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetConfigInfo(
        UINT                                maxSequence,
        UINT8                               numSessions,
        const ChiFeature2SessionDescriptor* pSession);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetNextStageInfo
    ///
    /// @brief  Set next stage sequence information.
    ///
    /// @param  stageId                         Stage ID.
    /// @param  stageSequenceId                 Stage Sequence ID.
    /// @param  numDependencyConfigDescriptor   Number of Config Descriptors.
    /// @param  pDependencyConfigDescriptor     Pointer to Config Descriptors.
    /// @param  numMaxDependencies              Number of maximum dependencies for this stage
    /// @param  requestIndex                    Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetNextStageInfo(
        UINT8                           stageId,
        UINT8                           stageSequenceId,
        UINT8                           numDependencyConfigDescriptor,
        const ChiFeature2SessionInfo*   pDependencyConfigDescriptor,
        UINT8                           numMaxDependencies,
        UINT8                           requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDependencyConfigBySequenceId
    ///
    /// @brief  Get Sequence information.
    ///
    /// @param  sequenceID      sequence id.
    /// @param  ppConfigInfo    Config information.
    /// @param  batchIndex      Output Batch Index.
    ///
    /// @return number of Dependency config
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8 GetDependencyConfigBySequenceId(
        INT32                       sequenceID,
        ChiFeature2SessionInfo**    ppConfigInfo,
        UINT8                       batchIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDependencyConfig
    ///
    /// @brief  Get Sequence information.
    ///
    /// @param  sequenceOrder   Current or Next process sequence order.
    /// @param  ppConfigInfo    Config information.
    /// @param  requestIndex    Request index.
    ///
    /// @return number of Dependency config
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8 GetDependencyConfig(
        ChiFeature2SequenceOrder    sequenceOrder,
        ChiFeature2SessionInfo**    ppConfigInfo,
        UINT8                       requestIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFinalBufferMetadataInfo
    ///
    /// @brief  Get final buffer meta info for port
    ///
    /// @param  identifier          Port identifier.
    /// @param  ppBufferMetaInfo    Port buffer metadata info.
    /// @param  requestIndex        Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetFinalBufferMetadataInfo(
        ChiFeature2Identifier               identifier,
        ChiFeature2BufferMetadataInfo**     ppBufferMetaInfo,
        UINT8                               requestIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFinalGlobalIdentifiers
    ///
    /// @brief  Get the list of global identifiers from the final buffer meta info
    ///
    /// @param  requestIndex    Request index.
    ///
    /// @return ChiFeature2Identifier* vector
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOWHINE CP006: Need whiner update: std::vector allowed in exceptional cases
    std::vector<const ChiFeature2Identifier*> GetFinalGlobalIdentifiers(
        UINT8 requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPrivContext
    ///
    /// @brief  Set private client data
    ///         As a rule, the derived feature is not expected to hold any state outside the request object context.
    ///         This function is used to set the derived feature's private client data.
    ///
    /// @param  pClientData Pointer to private client data.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetPrivContext(
        VOID* pClientData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPrivContext
    ///
    /// @brief  Get private client data
    ///         The derived feature can get access to its own private data for current request.
    ///         As a rule, the derived feature is not expected to hold any state outside the request object context.
    ///         The size of the context is set during feature query caps.
    ///
    /// @return Pointer to the private client data upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetPrivContext() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSequencePrivData
    ///
    /// @brief  Set private client data per sequence
    ///
    /// @param  sequenceOrder   Sequence Order.
    /// @param  pData           Pointer to private client data.
    /// @param  requestIndex    Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetSequencePrivData(
        ChiFeature2SequenceOrder    sequenceOrder,
        VOID*                       pData,
        UINT8                       requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSequencePrivData
    ///
    /// @brief  Get private client data
    ///
    /// @param  sequenceOrder   Sequence Order.
    /// @param  requestIndex    Request index.
    ///
    /// @return Pointer to the private client data upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetSequencePrivData(
        ChiFeature2SequenceOrder    sequenceOrder,
        UINT8                       requestIndex
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSequencePrivDataById
    ///
    /// @brief  Set private client data per sequence
    ///
    /// @param  sequenceId      process sequence number.
    /// @param  pData           Pointer to private client data.
    /// @param  requestIndex    Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetSequencePrivDataById(
        INT32   sequenceId,
        VOID*   pData,
        UINT8   requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSequencePrivDataById
    ///
    /// @brief  Get private client data
    ///
    /// @param  sequenceId      process sequence number.
    /// @param  requestIndex    Request index.
    ///
    /// @return Pointer to the private client data upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetSequencePrivDataById(
        INT32   sequenceId,
        UINT8   requestIndex
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCurRequestState
    ///
    /// @brief  Set current request state.
    ///
    /// @param  requestState    Request state.
    /// @param  requestIndex    Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetCurRequestState(
        ChiFeature2RequestState requestState,
        UINT8                   requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurRequestState
    ///
    /// @brief  Get current request state.
    ///
    /// @param  requestIndex     Request index.
    ///
    /// @return Pointer to the private client data upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2RequestState GetCurRequestState(
        UINT8 requestIndex
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPortDescriptor
    ///
    /// @brief  Set port descriptor in request object.
    ///         The feature uses this function to set one of the following by specifying the associated descriptor,
    ///         1. Next input dependency (or)
    ///         2. Current input configuration (or)
    ///         3. Current output configuration.
    ///         RequestInput is not a valid input for this function.
    ///
    /// @param  sequenceOrder           Sequence order (Next / Current).
    /// @param  type                    Operation Type.
    /// @param  pIdentifier             Pointer to Feature Identifier for the associated descriptor.
    /// @param  pPortDescriptor         Pointer to Port Descriptor.
    /// @param  batchIndex              Input batch index.
    /// @param  inputDependencyIndex    Index for input dependency.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetPortDescriptor(
        ChiFeature2SequenceOrder            sequenceOrder,
        ChiFeature2RequestObjectOpsType     type,
        const ChiFeature2Identifier*        pIdentifier,
        const ChiFeature2PortDescriptor*    pPortDescriptor,
        UINT8                               batchIndex,
        UINT8                               inputDependencyIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPortDescriptor
    ///
    /// @brief  Get descriptor from request object.
    ///         The feature uses this function to get one of the following descriptor from the feature request object,
    ///         1. Current input dependency (or)
    ///         2. Current input configuration (or)
    ///         3. Current output configuration.
    ///         RequestInput is not a valid input for this function.
    ///
    /// @param  sequenceOrder           Sequence order (Next / Current).
    /// @param  type                    Operation Type.
    /// @param  pIdentifier             Pointer to Feature Identifier for the associated descriptor.
    /// @param  ppPortDescriptor        Pointer to get port descriptor from feature request object.
    /// @param  batchIndex              Input batch index.
    /// @param  inputDependencyIndex    Index for input dependency.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetPortDescriptor(
        ChiFeature2SequenceOrder              sequenceOrder,
        ChiFeature2RequestObjectOpsType       type,
        const ChiFeature2Identifier*          pIdentifier,
        const ChiFeature2PortDescriptor**     ppPortDescriptor,
        UINT8                                 batchIndex,
        UINT8                                 inputDependencyIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBufferInfo
    ///
    /// @brief  Set the buffer information for the Port Id.
    ///         The buffer information is set by either the current feature itself or an upstream feature.
    ///         1. Upstream feature through feature graph for current dependency information.
    ///         2. Derived feature for input configuration information.
    ///         3. Derived feature for output configuration information.
    ///         RequestInput is not a valid input for this function.
    ///
    /// @param  type                    Operation Type.
    /// @param  pIdentifier             Pointer to Feature Identifier info for the associated descriptor.
    /// @param  hTargetBufferHandle     Target Buffer Handle to be set.
    /// @param  key                     Unique key to be set which is used extract buffer from handle.
    /// @param  setBufferError          Boolean indicating whether we want to set the buffer with an error
    /// @param  batchIndex              Input batch index.
    /// @param  inputDependencyIndex    Index for input dependency.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetBufferInfo(
        ChiFeature2RequestObjectOpsType   type,
        const ChiFeature2Identifier*      pIdentifier,
        const CHITARGETBUFFERINFOHANDLE   hTargetBufferHandle,
        UINT64                            key,
        BOOL                              setBufferError,
        UINT8                             batchIndex,
        UINT8                             inputDependencyIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBufferStatus
    ///
    /// @brief  Set the buffer information for the Port Id.
    ///         The buffer information is set by either the current feature itself or an upstream feature.
    ///         1. Upstream feature through feature graph for current dependency information.
    ///         2. Derived feature for input configuration information.
    ///         3. Derived feature for output configuration information.
    ///         RequestInput is not a valid input for this function.
    ///
    /// @param  type                    Operation Type.
    /// @param  pIdentifier             Pointer to Feature Identifier info for the associated descriptor.
    /// @param  sequenceId              Request object sequenceId
    /// @param  bufferStatus            Port buffer status
    /// @param  batchIndex              Input batch index.
    /// @param  inputDependencyIndex    Index for input dependency.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetBufferStatus(
        ChiFeature2RequestObjectOpsType   type,
        const ChiFeature2Identifier*      pIdentifier,
        UINT8                             sequenceId,
        ChiFeature2PortBufferStatus       bufferStatus,
        UINT8                             batchIndex,
        UINT8                             inputDependencyIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferInfo
    ///
    /// @brief  Get the buffer information for the Port Id.
    ///         RequestInput is not a valid input for this function.
    ///
    /// @param  type                    Operation Type.
    /// @param  pIdentifier             Pointer to Feature Identifier for the required buffer informtion.
    /// @param  phTargetBufferHandle    Pointer to Target Buffer Handle.
    /// @param  pKey                    Pointer to Unique key which is used extract buffer from handle.
    /// @param  batchIndex              Input batch index.
    /// @param  inputDependencyIndex    Index for input dependency.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetBufferInfo(
        ChiFeature2RequestObjectOpsType   type,
        const ChiFeature2Identifier*      pIdentifier,
        CHITARGETBUFFERINFOHANDLE*        phTargetBufferHandle,
        UINT64*                           pKey,
        UINT8                             batchIndex,
        UINT8                             inputDependencyIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMetadataInfo
    ///
    /// @brief  Set the metadata information for the Pipeline
    ///         The metadata information is set by either the current feature itself or an upstream feature.
    ///         1. Upstream feature through feature graph for current dependency information.
    ///         2. Derived feature for input configuration information.
    ///         3. Derived feature for output configuration information.
    ///         RequestInput is not a valid input for this function.
    ///
    /// @param  type                    Operation Type.
    /// @param  pIdentifier             Pointer to Feature Identifier info for the associated descriptor.
    /// @param  hMetadata               Metadata handle.
    /// @param  batchIndex              Input batch index.
    /// @param  inputDependencyIndex    Index for input dependency.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetMetadataInfo(
        ChiFeature2RequestObjectOpsType type,
        const ChiFeature2Identifier*    pIdentifier,
        const CHIMETAHANDLE             hMetadata,
        UINT8                           batchIndex,
        UINT8                           inputDependencyIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataInfo
    ///
    /// @brief  Get the metadata information for the pipeline.
    ///         RequestInput is not a valid input for this function.
    ///
    /// @param  type                    Operation Type.
    /// @param  pIdentifier             Pointer to Feature Identifier for the required buffer informtion.
    /// @param  phMetadata              Pointer to Metadata handle.
    /// @param  batchIndex              Input batch index.
    /// @param  inputDependencyIndex    Index for input dependency.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetMetadataInfo(
        ChiFeature2RequestObjectOpsType type,
        const ChiFeature2Identifier*    pIdentifier,
        CHIMETAHANDLE*                  phMetadata,
        UINT8                           batchIndex,
        UINT8                           inputDependencyIndex
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetRequestInputInfo
    ///
    /// @brief  Set the Request metadata information for the Port Identifier.
    ///
    /// @param  sequenceOrder       Sequence order (Next / Current).
    /// @param  pIdentifier         Pointer to Feature Identifier for the associated descriptor.
    /// @param  hMetadata           Metadata Handle to be set.
    /// @param  metadataHandleIndex Metadata Handle's index.
    /// @param  requestIndex        Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetRequestInputInfo(
        ChiFeature2SequenceOrder        sequenceOrder,
        const ChiFeature2Identifier*    pIdentifier,
        CHIMETAHANDLE                   hMetadata,
        UINT8                           metadataHandleIndex,
        UINT8                           requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetRequestInputInfo
    ///
    /// @brief  Reset the Request metadata information for the Port Identifier to NULL.
    ///
    /// @param  sequenceOrder       Sequence order (Next / Current).
    /// @param  pIdentifier         Pointer to Feature Identifier for the associated descriptor.
    /// @param  metadataHandleIndex Metadata Handle's index.
    /// @param  requestIndex        Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ResetRequestInputInfo(
        ChiFeature2SequenceOrder        sequenceOrder,
        const ChiFeature2Identifier*    pIdentifier,
        UINT8                           metadataHandleIndex,
        UINT8                           requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestInputInfo
    ///
    /// @brief  Get the Request metadata information for the Port Identifier.
    ///
    /// @param  sequenceOrder       Sequence order (Next / Current).
    /// @param  pIdentifier         Pointer to Feature Identifier for the associated descriptor
    /// @param  phMetadata          Pointer to Corresponding Metadata Handle for the feature identifier
    /// @param  metadataHandleIndex Metadata Handle's index.
    /// @param  requestIndex        Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetRequestInputInfo(
        ChiFeature2SequenceOrder        sequenceOrder,
        const ChiFeature2Identifier*    pIdentifier,
        CHIMETAHANDLE*                  phMetadata,
        UINT8                           metadataHandleIndex,
        UINT8                           requestIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetOutputNotifiedForPort
    ///
    /// @brief  Set the output Notified for the Port Id.
    ///         This is to ensure that the CTB(CHI Target Buffer) class can know when to do a release.
    ///         in cases where there are upstream features that are waiting on the target as input dependency.
    ///
    /// @param  portIdentifier Port Identifier for which output notification is to be set.
    /// @param  requestIndex   Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetOutputNotifiedForPort(
        ChiFeature2Identifier  portIdentifier,
        UINT8                  requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputNotifiedForPort
    ///
    /// @brief  Get the output Notified for the Port Id.
    ///
    /// @param  portIdentifier Port Identifier for which  we want to chekc for output notification.
    /// @param  requestIndex   Request index.
    ///
    /// @return TRUE if notified or FALSE.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL GetOutputNotifiedForPort(
        ChiFeature2Identifier  portIdentifier,
        UINT8                  requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetOutputGeneratedForPort
    ///
    /// @brief  Set the output generated for the Port Id.
    ///         This is to ensure that the CTB(CHI Target Buffer) class can know when to do a release.
    ///         in cases where there are upstream features that are waiting on the target as input dependency.
    ///
    /// @param  portIdentifier Port Identifier for which output notification is to be set.
    /// @param  requestIndex   Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetOutputGeneratedForPort(
        ChiFeature2Identifier  portIdentifier,
        UINT8                  requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputGeneratedForPort
    ///
    /// @brief  Get the output generated for the Port Id.
    ///
    /// @param  portIdentifier Port Identifier for which  we want to chekc for output notification.
    /// @param  requestIndex   Request index.
    ///
    /// @return TRUE if notified or FALSE.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL GetOutputGeneratedForPort(
        ChiFeature2Identifier  portIdentifier,
        UINT8                  requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetReleaseAcknowledgedForPort
    ///
    /// @brief  Set release acknowledged Port Id.
    ///
    /// @param  portIdentifier Port Identifier for which output notification is to be set.
    /// @param  requestIndex   Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetReleaseAcknowledgedForPort(
        ChiFeature2Identifier  portIdentifier,
        UINT8                  requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetReleaseAcknowledgedForPort
    ///
    /// @brief  Get the output Notified for the Port Id.
    ///
    /// @param  portIdentifier Port Identifier for which  we want to check if release has been acknowledged
    /// @param  requestIndex   Request index.
    ///
    /// @return TRUE if acknowledged or FALSE.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL GetReleaseAcknowledgedForPort(
        ChiFeature2Identifier  portIdentifier,
        UINT8                  requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AreOutputsNotified
    ///
    /// @brief  Helper to inform if all the output port's have been notified
    ///         Graph can use this to check if all output ports have been notified
    ///
    /// @param  requestIndex   Request index.
    ///
    /// @return TRUE if successful or FALSE.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AreOutputsNotified(
        UINT8   requestIndex
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AreOutputsReleased
    ///
    /// @brief  Helper to inform if all the output port's have been released
    ///         Graph can use this to check if all output ports have been released
    ///
    /// @param  requestIndex   Request index.
    ///
    /// @return TRUE if successful or FALSE.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AreOutputsReleased(
        UINT8   requestIndex
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AreOutputsGenerated
    ///
    /// @brief  Helper to inform if all the output port's have generated output
    ///         Graph can use this to check if all output ports have generated output
    ///
    /// @param  requestIndex   Request index.
    ///
    /// @return TRUE if successful or FALSE.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AreOutputsGenerated(
        UINT8   requestIndex
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AreInputDependenciesStatisfied
    ///
    /// @brief  Helper to inform if all the Input Dependencies have been Statisfied.
    ///
    /// @param  sequenceOrder  Sequence order (Next / Current).
    /// @param  requestIndex   Request index.
    ///
    /// @return The status of the input dependencies
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2InputDependencyStatus AreInputDependenciesStatisfied(
        ChiFeature2SequenceOrder    sequenceOrder,
        UINT8                       requestIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUsecaseRequestObject
    ///
    /// @brief  Get the Usecase Request Object (URO).
    ///
    /// @return The pointer of usecase request object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2UsecaseRequestObject* GetUsecaseRequestObject() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGraphPrivateData
    ///
    /// @brief  Get the feature graph's private data/context for current URO.
    ///
    /// @param  ppGraphPrivateData  Pointer to feature graph's private data.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetGraphPrivateData(
        VOID** ppGraphPrivateData) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetProcessSequenceId
    ///
    /// @brief  Get Process Sequence Id
    ///
    /// @param  order           Sequence Order.
    /// @param  requestIndex    Request index.
    ///
    /// @return Current Process Sequence Id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    INT32 GetProcessSequenceId(
        ChiFeature2SequenceOrder order,
        UINT8                    requestIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumRequestInputs
    ///
    /// @brief  Get the number of input request
    ///
    /// @param  pGlobalIdentifier     Pointer to Feature Identifier for the associated descriptor
    /// @param  requestIndex          Request index.
    ///
    /// @return number of port's input
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8 GetNumRequestInputs(
        const ChiFeature2Identifier*    pGlobalIdentifier,
        UINT8                           requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MoveToNextProcessSequenceInfo
    ///
    /// @brief  Move to Next Process Sequence Info
    ///
    /// @param  requestIndex    Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult MoveToNextProcessSequenceInfo(
        UINT8 requestIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetExternalRequestOutput
    ///
    /// @brief  Get the External Request Output
    ///
    /// @param  pNumRequestOutput     Pointer to number of request Output.
    /// @param  ppRequestOutputInfo   Pointer array of RequestOutput Info.
    /// @param  requestIndex          Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetExternalRequestOutput(
        UINT8*                                 pNumRequestOutput,
        const ChiFeature2RequestOutputInfo**   ppRequestOutputInfo,
        UINT8                                  requestIndex
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy feature request object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHandle
    ///
    /// @brief  Get Handle for the  feature request object
    ///
    /// @return CHIFEATUREREQUESTOBJECTHANDLE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHIFEATUREREQUESTOBJECTHANDLE GetHandle() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNextStageInfo
    ///
    /// @brief  Get Current Stage ID
    ///
    /// @param  pStageInfo      Output stageInfo
    /// @param  requestIndex    Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetNextStageInfo(
        ChiFeature2StageInfo* pStageInfo,
        UINT8                 requestIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentStageInfo
    ///
    /// @brief  Get Current Stage ID
    ///
    /// @param  pStageInfo      Output stageInfo
    /// @param  requestIndex    Request index.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetCurrentStageInfo(
        ChiFeature2StageInfo* pStageInfo,
        UINT8                 requestIndex) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump
    ///
    /// @brief  This method is used by CHI to print debugging state for the feature Request Object. It will be
    ///         called when using the dumpsys tool or capturing a bugreport.
    ///
    /// @param  fd The file descriptor which can be used to write debugging text using dprintf() or write().
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Dump(
        INT fd
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAppRequestSetting
    ///
    /// @brief  Gets the APP request settings
    ///
    /// @param  ppChiMetadata         Pointer to APP request setting
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAppRequestSetting(
        ChiMetadata** ppChiMetadata) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeatureHint
    ///
    /// @brief  Gets the feature request object's hint
    ///
    /// @return Pointer to the hint
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiFeature2Hint* GetFeatureHint()
    {
        return m_pHint;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeatureUsecaseMode
    ///
    /// @brief  Gets the feature request object's usecase mode
    ///
    /// @return Chi Usecase Mode
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiModeUsecaseSubModeType GetFeatureUsecaseMode()
    {
        return m_usecaseMode;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumRequests
    ///
    /// @brief  Gets the number of requests to output.
    ///
    /// @return UINT8
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT8 GetNumRequests()
    {
        return m_numRequests;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurRequestId
    ///
    /// @brief  Gets the current processing requestId
    ///
    /// @return UINT8
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT8 GetCurRequestId()
    {
        return m_curRequestId;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCurRequestId
    ///
    /// @brief  Sets the current processing requestId
    ///
    /// @param  requestId   Processing requestId to set
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetCurRequestId(
        UINT8 requestId)
    {
        if (requestId <= m_curRequestId)
        {
            m_curRequestId = requestId;
        }
        else
        {
            CHX_LOG_ERROR("%s Setting requestId to %d failed curRequestId %d", m_identifierString, requestId, m_curRequestId);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MoveToNextRequest
    ///
    /// @brief  Increment current requestId
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE CDKResult MoveToNextRequest()
    {
        CDKResult result = CDKResultSuccess;

        if ((m_curRequestId + 1) < m_numRequests)
        {
            m_curRequestId++;
        }
        else
        {
            result = CDKResultENoMore;
        }

        return result;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IdentifierString
    ///
    /// @brief  returns the FRO Identifer that has the URO, Feature info
    ///
    /// @return identifier
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE const CHAR* IdentifierString() const
    {
        return m_identifierString;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPortIdtoSeqIdVector
    ///
    /// @brief  Gets sequence ID vector for port Id
    ///
    /// @param  portKey global port Id
    ///
    /// @return vector of sequence Id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE std::vector<UINT32> GetPortIdtoSeqIdVector(
        ChiFeature2Identifier portKey) const
    {
        std::vector<UINT32> seqIdVector;

        for (UINT32 index = 0; index < m_portIdtoSeqIdVector.size(); index++)
        {
            if (m_portIdtoSeqIdVector[index].portId == portKey)
            {
                seqIdVector = m_portIdtoSeqIdVector[index].sequenceIdVector;
            }
        }

        return seqIdVector;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdatePortIdtoSeqIdVector
    ///
    /// @brief  Sets vector for port ID to all sequence ID which already submitted
    ///
    /// @param  portKey    global port ID
    /// @param  sequenceId sequence ID
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID UpdatePortIdtoSeqIdVector(
        ChiFeature2Identifier portKey,
        UINT32                sequenceId)
    {
        BOOL isFound = FALSE;

        for (UINT32 index = 0; index < m_portIdtoSeqIdVector.size(); index++)
        {
            if (m_portIdtoSeqIdVector[index].portId == portKey)
            {
                m_portIdtoSeqIdVector[index].sequenceIdVector.push_back(sequenceId);
                isFound = TRUE;
                break;
            }
        }

        if (FALSE == isFound)
        {
            std::vector<UINT32> SeqIdVec;
            SeqIdVec.push_back(sequenceId);
            ChiFeature2PortIdSequenceId portIdSeqId = {portKey, SeqIdVec};
            m_portIdtoSeqIdVector.push_back(portIdSeqId);
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeature
    ///
    /// @brief  Gets the feature instance
    ///
    /// @return chi feature instance
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiFeature2Base* GetFeature()
    {
        return m_pFeatureBase;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LockRequestObject
    ///
    /// @brief  Lock feature2 request object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID LockRequestObject()
    {
        if (NULL != m_pMutex)
        {
            m_pMutex->Lock();
        }
        else
        {
            CHX_LOG_WARN("m_pMutex = NULL!");
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnlockRequestObject
    ///
    /// @brief  UnLock feature2 request object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID UnlockRequestObject()
    {
        if (NULL != m_pMutex)
        {
            m_pMutex->Unlock();
        }
        else
        {
            CHX_LOG_WARN("m_pMutex = NULL!");
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitOnResult
    ///
    /// @brief  Wait on result of request
    ///
    /// @param  requestIndex    Request index of the request object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID WaitOnResult(
        UINT8 requestIndex)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        if (NULL != pRequestInfo)
        {
            Mutex*      pResultMutex        = pRequestInfo->pResultMutex;
            Condition*  pResultAvailable    = pRequestInfo->pResultAvailable;

            if ((NULL != pResultMutex) && (NULL != pResultAvailable))
            {
                pResultMutex->Lock();
                pResultAvailable->Wait(pResultMutex->GetNativeHandle());
                pResultMutex->Unlock();
            }
            else
            {
                CHX_LOG_ERROR("pResultMutex %p or pResultAvailable %p is NULL! cannot wait on result",
                              pResultMutex,
                              pResultAvailable);
            }
        }
        else
        {
            CHX_LOG_ERROR("Request info is NULL! cannot wait on result");
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyFinalResultReceived
    ///
    /// @brief  Method for notifying the final result for the request meaning we have moved to OutputNotificationPending
    ///
    /// @param  requestIndex    Request index of the request object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID NotifyFinalResultReceived(
        UINT8 requestIndex)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        if (NULL != pRequestInfo)
        {
            Mutex*      pResultMutex        = pRequestInfo->pResultMutex;
            Condition*  pResultAvailable    = pRequestInfo->pResultAvailable;

            if ((NULL != pResultMutex) && (NULL != pResultAvailable))
            {
                pResultMutex->Lock();
                pResultAvailable->Signal();
                pResultMutex->Unlock();
            }
            else
            {
                CHX_LOG_ERROR("pResultMutex %p or pResultAvailable %p is NULL! cannot signal result",
                              pResultMutex,
                              pResultAvailable);
            }
        }
        else
        {
            CHX_LOG_ERROR("Request info is NULL! cannot signal result");
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetProcessingCompleteForPort
    ///
    /// @brief  Method for marking an FRO's output port as processing complete. Setting this flag indicates that no more
    ///         dependencies will be asked of for this port
    ///
    /// @param  portIdentifier  Port Identifier for which processing complete notification is to be set.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetProcessingCompleteForPort(
        ChiFeature2Identifier portIdentifier);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPortsProcessingCompleteNotified
    ///
    /// @brief  Method to check if we have already sent a proccessing complete notification for the ports
    ///
    /// @return TRUE if we have notified that the ports are done processing; FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL GetOutputPortsProcessingCompleteNotified()
    {
        return m_outputPortsProcessingCompleteNotified;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetOutputPortsProcessingCompleteNotified
    ///
    /// @brief  Set that we have notified the graph that the ports are done processing
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetOutputPortsProcessingCompleteNotified()
    {
        m_outputPortsProcessingCompleteNotified = TRUE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllOutputPortsProcessingComplete
    ///
    /// @brief  Check whether all ports are done processing for this request object
    ///
    /// @return TRUE if all ports are done processing and FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AllOutputPortsProcessingComplete();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetInterFeatureRequestPrivateData
    ///
    /// @brief  Set inter-feature request private data
    ///
    /// @param  featureId       featureId
    /// @param  cameraId        cameraId of feature
    /// @param  instanceId      instanceId of feature
    /// @param  pPrivateData    Private data from feature
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetInterFeatureRequestPrivateData(
        UINT32  featureId,
        UINT32  cameraId,
        UINT32  instanceId,
        VOID*   pPrivateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInterFeatureRequestPrivateData
    ///
    /// @brief  Get inter-feature request private data
    ///
    /// @param  featureId       featureId
    /// @param  cameraId        cameraId of feature
    /// @param  instanceId      instanceId of feature
    ///
    /// @return VOID pointer to private data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetInterFeatureRequestPrivateData(
        UINT32  featureId,
        UINT32  cameraId,
        UINT32  instanceId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemoveInterFeatureRequestPrivateData
    ///
    /// @brief  Remove inter-feature request private data
    ///
    /// @param  featureId       featureId
    /// @param  cameraId        cameraId of feature
    /// @param  instanceId      instanceId of feature
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult RemoveInterFeatureRequestPrivateData(
        UINT32  featureId,
        UINT32  cameraId,
        UINT32  instanceId);

private:

    /// @brief Feature Port Map Info
    struct CHIFeature2PortMap
    {
        UINT8  lastPortIndex; ///< Last Port Index mapped.
        UINT8* pPortIndexs;   ///< Local Port Index Array.
    };

    /// @brief Feature Pipeline Map Info
    struct ChiFeature2PipelineMap
    {
        UINT8               pipelineIndex;         ///< Local Pipeline Index.
        CHIFeature2PortMap  portMap[OpTypeMax];    ///< Port Map Array.
    };

    /// @brief Feature Session Map Info
    struct ChiFeature2SessionMap
    {
        UINT8                     lastPipelineIndex;  ///< Last Pipeline Index mapped.
        UINT8                     sessionIndex;       ///< Local Session Index.
        ChiFeature2PipelineMap*   pPipelineMap;       ///< Local Pipeline Index Array.
    };

    /// @brief Feature Identifier Map Info
    struct CHIFeature2IdentifierMap
    {
        UINT8                  maxSession;          ///< Maximum Session Index for the FID.
        UINT8                  maxPipeline;         ///< Maximum Pipeline Index for the FID.
        UINT8                  maxPort;             ///< Maximum Port Index for the FID.
        UINT8                  lastSessionIndex;    ///< Last Session Index mapped.
        ChiFeature2SessionMap* pSessionMap;         ///< Local Session Index Array.
    };

    /// @brief Feature process sequence information
    struct ChiFeature2ProcessSequenceInfo
    {
        UINT8                       stageId;                                    ///< Stage ID.
        UINT8                       stageSequenceId;                            ///< Stage sequence ID.
        UINT8                       numDependencyConfig;                        ///< Total number of input dependencies
                                                                                ///  & configuration.
        ChiFeature2SessionInfo*     pDependencyConfig;                          ///< Pointer to dependency & configuration
                                                                                ///  of this stage.
        CHIFeature2IdentifierMap*   pMap;                                       ///< Feature Identifier Map.
        UINT8                       inputMetadataDependenciesToBeStatisfied;    ///< Total input Metadata dependecies to
                                                                                ///  be satisfied.
        UINT8                       inputBuffersDependenciesToBeStatisfied;     ///< Total input Buffers dependecies to be
                                                                                ///  satisfied.
        VOID*                       pSequencePrivData;                          ///< Private data per sequence
        UINT8                       maxPossibleErrors;                          ///< Number of possible errors; 1 error per
                                                                                ///  input dependency
        UINT8                       totalInputDependencies;                     ///< Total number of input dependencies
    };

    /// @brief Sequence Id vector on port information
    struct ChiFeature2PortIdSequenceId
    {
        ChiFeature2Identifier portId;                          ///< global port Id
        std::vector<UINT32>   sequenceIdVector;                ///< Sequence Id vector after submitted on port Id
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFeatureIdentifierMap
    ///
    /// @brief  Create feature identifier map to help mapping between global and local FID
    ///
    /// @param  ppMap                Pointer to FID Mapping Pointer.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateFeatureIdentifierMap(
        CHIFeature2IdentifierMap**       ppMap);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeleteFeatureIdentifierMap
    ///
    /// @brief  Delete Allocation from Feature Identifier Map
    ///
    /// @param  pMap   Pointer to FID Mapping Pointer.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DeleteFeatureIdentifierMap(
        CHIFeature2IdentifierMap* pMap);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapFeatureIdentifier
    ///
    /// @brief  Map the Global Feature Idenitifer to the Local identifier.
    ///
    /// @param  pMap          Pointer to Corresponding Feature Identifier Map.
    /// @param  opsType       Type of Operation.
    /// @param  pGlobalFID    Pointer to Global Feature Identifier.
    /// @param  pLocalFID     Pointer to Local Feature Identifier.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult MapFeatureIdentifier(
        CHIFeature2IdentifierMap*       pMap,
        ChiFeature2RequestObjectOpsType opsType,
        const ChiFeature2Identifier*    pGlobalFID,
        ChiFeature2Identifier*          pLocalFID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLocalFeatureIdentifier
    ///
    /// @brief  Get the Local Feature Identifier for the global
    ///
    /// @param  pMap          Pointer to Corresponding Feature Identifier Map.
    /// @param  opsType       Type of Operation
    /// @param  pGlobalFID    Pointer to Global Feature Identifier.
    /// @param  pLocalFID     Pointer to Local Feature Identifier.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetLocalFeatureIdentifier(
        CHIFeature2IdentifierMap*        pMap,
        ChiFeature2RequestObjectOpsType  opsType,
        const ChiFeature2Identifier*     pGlobalFID,
        ChiFeature2Identifier*           pLocalFID) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAndGetLocalFeatureIdentifier
    ///
    /// @brief  Sets and Get the Local Feature Identifier for the global
    ///
    /// @param  pMap          Pointer to Corresponding Feature Identifier Map.
    /// @param  opsType       Type of Operation
    /// @param  pGlobalFID    Pointer to Global Feature Identifier.
    /// @param  pLocalFID     Pointer to Local Feature Identifier.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetAndGetLocalFeatureIdentifier(
        CHIFeature2IdentifierMap*        pMap,
        ChiFeature2RequestObjectOpsType  opsType,
        const ChiFeature2Identifier*     pGlobalFID,
        ChiFeature2Identifier*           pLocalFID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateConfigInfoWithDescriptor
    ///
    /// @brief  Helper function to populate config info from config descriptor.
    ///
    /// @param  pConfigInfo          Stage Configuration Info.
    /// @param  pConfigDesc          Stage ConfigurationDescription.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PopulateConfigInfoWithDescriptor(
        ChiFeature2DependencyConfigInfo*             pConfigInfo,
        const ChiFeature2DependencyConfigDescriptor* pConfigDesc);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateDependencyInfo
    ///
    /// @brief  Helper function to populate dependency info
    ///
    /// @param  pConfigInfo          Stage Configuration Info.
    /// @param  pConfigDesc          Stage ConfigurationDescription.
    /// @param  maxDependencies      Maximum number of dependencies for this stage
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PopulateDependencyInfo(
        ChiFeature2DependencyConfigInfo*             pConfigInfo,
        const ChiFeature2DependencyConfigDescriptor* pConfigDesc,
        UINT8                                        maxDependencies);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpConfig
    ///
    /// @brief  This method is used to print the config for FRO.
    ///
    /// @param  fd              The file descriptor which can be used to write debugging text using dprintf() or write().
    /// @param  sessionIndex    The session index for the config to be dumped.
    /// @param  pipelineIndex   The pipeline index for the config to be dumped.
    /// @param  pInfo           The PBM info to be dumped.
    /// @param  cnt             Count of ports.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpConfig(
        INT                                fd,
        UINT8                              sessionIndex,
        UINT8                              pipelineIndex,
        ChiFeature2PortBufferMetadataInfo* pInfo,
        UINT8                              cnt
    )const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRequestInput
    ///
    /// @brief  This method is used to print the request input for the sequence
    ///
    /// @param  fd              The file descriptor which can be used to write debugging text using dprintf() or write().
    /// @param  sessionIndex    The session index for the config to be dumped.
    /// @param  pipelineIndex   The pipeline index for the config to be dumped.
    /// @param  pInfo           The metadata info to be dumped.
    /// @param  cnt             Count of ports.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRequestInput(
        INT                      fd,
        UINT8                    sessionIndex,
        UINT8                    pipelineIndex,
        ChiFeature2MetadataInfo* pInfo,
        UINT8                    cnt
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpFIDMapping
    ///
    /// @brief  This method is used to print the mapping from global FID- local FID
    ///
    /// @param  fd      The file descriptor which can be used to write debugging text using dprintf() or write().
    /// @param  pMap    Pointer to the FID map.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpFIDMapping(
        INT                             fd,
        CHIFeature2IdentifierMap*       pMap
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPortUnique
    ///
    /// @brief  Helper function to initialize m_outputPortProcessingInfo
    ///
    /// @param  port    Port we want to insert into the m_outputPortProcessingInfo vector
    ///
    /// @return TRUE if the port is unique to the vector, FALSE if the port is not unique to the vector
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPortUnique(
        UINT8 port);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2RequestObject
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2RequestObject()=default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2RequestObject
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~ChiFeature2RequestObject() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeRequestInfo
    ///
    /// @brief  Initialize the request information
    ///
    /// @param  pCreateInputInfo    Feature request object create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeRequestInfo(
        const ChiFeature2RequestObjectCreateInputInfo* pCreateInputInfo);

    ChiFeature2RequestObject(const ChiFeature2RequestObject&)             = delete;     ///< Disallow the copy constructor
    ChiFeature2RequestObject& operator= (const ChiFeature2RequestObject&) = delete;     ///< Disallow assignment operator

    /// @brief Feature request information
    struct ChiFeature2RequestInfo
    {
        ChiFeature2RequestState          state;                     ///< Request state.
        UINT8                            requestIndex;              ///< Request index of the output
        UINT8                            numRequestOutputs;         ///< Number of final request outputs.
        ChiFeature2RequestOutputInfo*    pRequestOutput;            ///< Pointer to External request output table.
        BOOL*                            pOutputNotified;           ///< Pointer to final request output table to
                                                                    ///  check if its been notified.
        BOOL*                            pReleaseAcked;             ///< Pointer to final request output table to
                                                                    ///  check if its been release acked.
        BOOL*                            pOutputGenerated;          ///< Pointer to final request output table to
                                                                    ///  check if its been generated.
        UINT8                            numOutputNotified;         ///< Number of request outputs notified.
        UINT8                            numOutputReleased;         ///< Number of output released
        UINT8                            numOutputGenerated;        ///< Number of output released
        INT32                            curProcessSequenceId;      ///< Current processing sequence Id.
        INT32                            nextProcessSequenceId;     ///< Next processing sequence Id.
        ChiFeature2ProcessSequenceInfo** ppProcessSequenceInfo;     ///< Process sequence info array of size
                                                                    ///  maxNumProcessSequence.
        Mutex*                           pResultMutex;              ///< Stage result mutex
        Condition*                       pResultAvailable;          ///< Wait till all stage results are available
        Mutex*                           pDependencyMutex;          ///< Dependency mutex
    };

    /// @brief Batch ouptut request information
    struct ChiFeature2BatchOutputRequestInfo
    {
        ChiFeature2ProcessSequenceInfo*      pBaseSequenceInfo;    ///< Pointer to Base Sequence Info Address.
        ChiFeature2RequestInfo*              pRequestInfo;         ///< Pointer to request object data.
    };

    /// @brief Processing information related to each unique output port
    struct ChiFeature2OutputPortProcessingInfo
    {
        UINT8       port;                       ///< Port number
        BOOL        isPortDoneProcessing;       ///< Is this port done processing
    };

    VOID*                                m_pFeaturePrivateData;     ///< Pointer holding the derived feature's private context.
    VOID*                                m_pGraphPrivateData;       ///< Private data/context for Feature graph.
    ChiFeature2UsecaseRequestObject*     m_pUsecaseRequestObj;      ///< Usecase request object (URO).
    ChiFeature2Base*                     m_pFeatureBase;            ///< Feature base class pointer.
    ChiFeature2BatchOutputRequestInfo*   m_pBatchOutputRequestInfo; ///< Pointer to Batch Output request info.
    CHIFEATUREREQUESTOBJECTHANDLE        m_hFroHandle;              ///< Handle for the FRO.
    UINT8                                m_numRequests;             ///< Number of requests to output.
    ChiModeUsecaseSubModeType            m_usecaseMode;             ///< Usecase mode.
    ChiFeature2Hint*                     m_pHint;                   ///< A feature specific custom hint from the feature graph
                                                                    ///  to the feature class.
    ChiFeature2StageMaxInfo              m_maxInfo;                 ///< Info for maximum number of sessions/pipelines/ports for
                                                                    ///  the internal map.
    UINT8                                m_curRequestId;            ///  Current processing requestId.

    CHAR                                 m_identifierString[MaxLength256]; ///< Identifier for FRO that consists of
                                                                           ///  URO/feature base info
    std::vector<ChiFeature2PortIdSequenceId> m_portIdtoSeqIdVector;        ///< Vector for port ID to all submitted sequenceId

    std::vector<ChiFeature2OutputPortProcessingInfo>    m_outputPortProcessingInfo;                 ///< Feature output port
                                                                                                    ///  information
    BOOL                                                m_outputPortsProcessingCompleteNotified;    ///< Indicates whether we
                                                                                                    ///  have notified the graph
                                                                                                    ///  that the ports are done
                                                                                                    ///  processing
    Mutex*                                              m_pMutex;
};

#endif // CHIFEATURE2REQUESTOBJECT_H
