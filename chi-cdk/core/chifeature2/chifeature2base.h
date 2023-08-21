////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2base.h
/// @brief CHI feature base class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2BASE_H
#define CHIFEATURE2BASE_H

#include <map>

#include "chxincs.h"
#include "chxextensionmodule.h"
#include "chxusecaseutils.h"
#include "chxutils.h"
#include "camxcdktypes.h"
#include "chxpipeline.h"
#include "chxsession.h"
#include "chxusecase.h"
#include "chicommon.h"

#include "chifeature2types.h"
#include "chifeature2requestobject.h"
#include "chifeature2usecaserequestobject.h"
#include "chithreadmanager.h"
#include "chistatspropertydefines.h"
#include "chxmulticamcontroller.h"


extern UINT32 g_enableChxLogs;
extern BOOL   g_logRequestMapping;
extern BOOL   g_enableSystemLog;

// NOWHINE FILE CP006:  used standard libraries for performance improvements
// NOWHINE FILE CP021:  used default arguments for non-virtual and private methods

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiFeature2RequestObject;

/// @brief Maximum number of requests which can be combined
static const UINT8 MaxCombinedRequests = 2;

/// @brief Function to execute flow
typedef CDKResult(ChiFeature2Base::*PFNBASEREQUESTFLOWFUNCTION)(
    ChiFeature2RequestObject* pRequestObject
    ) const;

/// @brief Callback method and context used by the Feature2 base class to callback to feature graph
struct ChiFeature2GraphCallbacks
{
    /// @brief Asynchronous notification message callback method
    CDKResult (*ChiFeature2ProcessMessage)(
        ChiFeature2RequestObject*   pFeatureRequestObj,         ///< The feature request object to operate on
        ChiFeature2Messages*        pMessages);                 ///< The message data to process

    CDKResult(*ChiFeature2Flush)();
};

/// @brief Feature instance creation information
struct ChiFeature2CreateInputInfo
{
    const ChiFeature2InstanceProps*     pInstanceProps;         ///< Pointer to instance properties
    LogicalCameraInfo*                  pCameraInfo;            ///< Pointer to logical camera info
    CHISTREAMCONFIGINFO*                pStreamConfig;          ///< Stream configuration for this feature.
    const ChiFeature2Descriptor*        pFeatureDescriptor;     ///< Feature descriptor containing pipeline
                                                                /// / session / target descriptors.
    const ChiUsecase*                   pUsecaseDescriptor;     ///< Pointer to usecase XML
    ChiFeature2GraphCallbacks*          pClientCallbacks;       ///< Methods used by base class to callback
                                                                ///  to the client
    ChiMetadataManager*                 pMetadataManager;       ///< Metadata manager for the usecase
    CHIThreadManager*                   pThreadManager;         ///< Feature Thread Manager
    std::vector<CHISTREAM*>*            pOwnedStreams;          ///< Streams allocated as part of negotiation

    union
    {
        struct
        {
            UINT32 bFrameworkVideoStream  : 1;   ///< flag which tells if the framework video stream available
            UINT32 bFrameworkHEICSnapshot : 1;   ///< flag which tells if the framework has HEIC stream
            UINT32 bDisableZoomCrop       : 1;   ///< disable zoom crop in IPE
            UINT32 bEnableResManager      : 1;   ///< flag to indicate if resourcemanager is needed
            UINT32 reserved               : 28;  ///< reserved for future flags
        };
        UINT32 flagValue; ///< above flags as UINT
    };
    ChiFeature2GraphManagerCallbacks*   pFeatureGraphManagerCallbacks;  ///< Feature Graph Manager Notification callbacks
    ChiFeature2InstanceFlags            featureFlags;                   ///< feature2 flags
};

/// @brief Feature query information
struct ChiFeature2QueryInfo
{
    UINT32       numCaps;             ///< Number of capabilities
    const CHAR** ppCapabilities;      ///< The array of capability strings
};

/// @brief Base request flow types
enum class ChiFeature2RequestFlowType
{
    Type0,      ///< Same external input port is used to ask for dependency and submission
    Type1,      ///< Multiple dependencies are asked on one external input port and copied over to multiple input ports during
                ///< request submission
    Invalid,    ///< Invalid flow type
};

/// @brief Contains required information for base to execute a request flow
struct ChiFeature2RequestFlow
{
    /// @brief One of base supported request flow types
    ChiFeature2RequestFlowType flowType;
    /// @brief Function Pointer to execute flow
    PFNBASEREQUESTFLOWFUNCTION pFlowFunc;
};

/// @brief Contains required information for base to identify inflight buffer callback
struct ChiFeature2InFlightCallbackData
{
    const ChiFeature2Base*      pFeatureInstance;  ///< Feature instance for the callback
    ChiFeature2RequestObject*   pRequestObject;    ///< Request object Instance
    ChiFeature2Identifier       portId;            ///< PortId for callback
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature base class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiFeature2Base
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequest
    ///
    /// @brief  Process the request.
    ///         The current request gets executed through different stages & sequences of processing.
    ///         The feature base class invokes derived implementation to determine the input & output
    ///         resources at every feature processing sequence.
    ///
    /// @param  pRequestObject  Feature request object.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessRequest(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief  Flush.
    ///
    /// @param  isSynchronousFlush  Boolean indicating if base should flush synchronously or asynchronously
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Flush(
        BOOL isSynchronousFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy feature object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy() = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Deactivate
    ///
    /// @brief  Deactivate all the pipelines
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Deactivate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeatureId
    ///
    /// @brief  Returns the feature id.
    ///
    /// @return The feature id.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 GetFeatureId() const
    {
        return m_featureId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeatureName
    ///
    /// @brief  Returns the feature name.
    ///
    /// @return The feature name.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE const CHAR* GetFeatureName() const
    {
        return m_pFeatureName;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstanceProps
    ///
    /// @brief  Returns the instance properties associated with this feature instance.
    ///
    /// @return The instance properties associated with this feature instance.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE const ChiFeature2InstanceProps* GetInstanceProps() const
    {
        return m_pInstanceProps;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataFromHandle
    ///
    /// @brief  Returns the instance  of metadata associated with handle
    ///
    /// @param  hMetaHandle The metadata handle
    ///
    /// @return The instance of metadata using metadata manager
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiMetadata* GetMetadataFromHandle(
        CHIMETADATAHANDLE hMetaHandle)
    {
        if (NULL != m_pMetadataManager)
        {
            return m_pMetadataManager->GetMetadataFromHandle(hMetaHandle);
        }
        else
        {
            return NULL;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyGraphManagerDestroyInProgress
    ///
    /// @brief  Notify Feature that Graph Manager is destroy in progress.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID NotifyGraphManagerDestroyInProgress()
    {
        m_destroyInProgress = TRUE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetExternalGlobalPortIdList
    ///
    /// @brief  Returns a list of global port IDs for all external ports.
    ///
    /// @return A vector of global port IDs for all external ports.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<ChiFeature2Identifier> GetExternalGlobalPortIdList();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPortDescriptorFromPortId
    ///
    /// @brief  Returns the port descriptor associated with the given port id.
    ///
    /// @param  pPortId The port id used to find the associated port descriptor
    ///
    /// @return The port descriptor associated with the given port id.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const ChiFeature2PortDescriptor* GetPortDescriptorFromPortId(
        const ChiFeature2Identifier* pPortId
        ) const;

    /// @brief Feature per frame data
    struct ChiFeatureFrameCallbackData
    {
        ChiFeature2RequestObject*              pRequestObj;             ///< Feature request object pointer
        std::vector<ChiFeature2Identifier>     pInputPorts;             ///< Input ports belongs to this pipeline
        std::vector<ChiFeature2Identifier>     pOutputPorts;            ///< Output ports belongs to this pipeline
        INT32                                  sequenceId;              ///< FRO SequenceId
        UINT8                                  stageId;                 ///< Stage Id for this frame
        UINT8                                  stagesequenceId;         ///< Stage sequence Id for this frame
        UINT8                                  requestId;               ///< Batch request index
    };

    /// @brief Feature per frame data
    ///        When multiple FROs are combined to be submitted into CamX as one request, this structure is used to store
    ///        the callback data corresponding to each FRO
    struct ChiFeatureCombinedCallbackData
    {
        ChiFeatureFrameCallbackData*   pCombinedCallbackData[MaxCombinedRequests];    ///< Combined callback data
        UINT8                          numCallbackData;                               ///< Number of callback data
    };

    /// @brief Feature per frame data
    struct ChiFeatureSequenceData
    {
        UINT32                                 frameNumber;             ///< Chi frame number for this sequence
        ChiFeatureFrameCallbackData            frameCbData;             ///< Place holder for callback data
        ChiFeatureCombinedCallbackData         combinedCbData;          ///< Combined callback data
        ChiMetadata*                           pInputMetadata;          ///< Input metadata used for this sequence
        VOID*                                  pPrivateData;            ///< Derived Privatedata for sequence
        UINT8                                  skipSequence;            ///< Sequence can be skipped
    };

protected:
    /// @brief  Classified stream structure
    struct ChiFeatureTargetStreams
    {
        std::vector<CHISTREAM*>              pInputStreams;             ///< Input Target Streams
        std::vector<CHISTREAM*>              pOutputStreams;            ///< Input Target Streams
    };

    /// @brief Session callback data
    struct ChiFeature2SessionCallbackData
    {
        UINT8                               sessionId;                      ///< SessionId corresponding to this callback
        ChiFeature2Base*                    pFeatureInstance;               ///< Pointer to instance of Feature2 base class
        ChiFeature2GraphManagerCallbacks    featureGraphManagerCallbacks;   ///< Feature Graph Manager Notify Callbacks
    };

    /// @brief Feature ports data
    struct ChiFeatureStreamData
    {
        CHISTREAM*                           pStream;                   ///< internal Stream pointer
        CHISTREAM*                           pTargetStream;             ///< External Target Stream
    };

    /// @brief Feature ports data
    struct ChiFeaturePortData
    {
        ChiFeature2Identifier                 globalId;                 ///< Feature Key (Session, Pipeline, Port)
        const CHAR*                           pPortName;                ///< Port name.
        const ChiFeature2TargetDescriptor*    pTargetDesc;              ///< Target associated with port
        ChiTarget*                            pTarget;                  ///< Pointer to target
        CHITargetBufferManager*               pOutputBufferTbm;         ///< Pointer to output buffer TBM
        UINT8                                 minBufferCount;           ///< Min Buffer count for this port
        UINT8                                 maxBufferCount;           ///< Max Buffer count for this port
        UINT64                                producerFlags;            ///< Buffer manager producer gralloc flags
        UINT64                                consumerFlags;            ///< Buffer manager consumer gralloc flags
        ChiStream*                            pChiStream;               ///< Pointer to ChiStream
        UINT32                                metadataClientId;         ///< Metadata client Id
    };

    /// @brief Feature pipelines data
    struct ChiFeaturePipelineData
    {
        ChiFeature2Identifier                  globalId;                ///< Feature Key (Session, Pipeline, Port)
        const CHAR*                            pPipelineName;           ///< Pipeline name.
        Pipeline*                              pPipeline;               ///< Chi pipeline pointer
        std::vector<ChiFeaturePortData>        pInputPortData;          ///< Input ports belongs to this pipeline
        std::vector<ChiFeaturePortData>        pOutputPortData;         ///< Output ports belongs to this pipeline
        UINT                                   minMetaBufferCount;      ///< The Minimun number of buffers to trigger recycle
        UINT                                   maxMetaBufferCount;      ///< The Maximum number of buffers to be allocated
        CHITargetBufferManager*                pOutputMetaTbm;          ///< Pointer to Output metadata TBM
        CHITargetBufferManager*                pSettingMetaTbm;         ///< Pointer to Setting metadata TBM
        UINT32                                 metadataClientId;        ///< Metadata client Id
    };

    /// @brief Feature session data
    struct ChiFeatureSessionData
    {
        ChiFeature2Identifier                  globalId;                ///< Feature Key (Session, Pipeline, Port)
        const CHAR*                            pSessionName;            ///< Session name.
        Session*                               pSession;                ///< Chi session pointer
        std::vector<ChiFeaturePipelineData*>   pPipelineData;           ///< Pipeline info for this session
        ChiFeature2SessionCallbackData         sessionCbData;           ///< Per Session callback data
        ChiCallBacks                           callbacks;               ///< Session callbacks
        volatile UINT32                        isFlushInProgress;       ///< Is flush in progress
    };

    /// @brief Feature stage data
    struct ChiFeatureStageData
    {
        ChiFeature2StageDescriptor              stageDescriptor;       ///< StageDescriptor for this stage
        std::vector<ChiFeature2Identifier>      inputPorts;            ///< Input ports for this stage
        std::vector<ChiFeature2Identifier>      outputPorts;           ///< Output ports for this stage
    };

    /// @brief Private context for feature and derived features.
    struct ChiFeaturePrivateContext
    {
        UINT8                                 size;                    ///< Size of the private context data
        VOID*                                 pPrivateData;            ///< Derived feature private context pointer
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnInputResourcePending
    ///
    /// @brief  Use to create continuous callback in FrameSelect
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnInputResourcePending(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnReleaseInputDependency
    ///
    /// @brief  Used to call to derived features if override is needed during HandleOutputNotificationPending
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    /// @param  requestId       Request id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnReleaseInputDependency(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     requestId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnInflightBufferCallback
    ///
    /// @brief  Used to call to derived features when Inflight buffer is received
    ///
    /// @param  pRequestObject  Feature request object instance
    /// @param  pPortId         Port Id
    /// @param  hBuffer         ChitargetBuffer handle
    /// @param  key             Buffer key
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnInflightBufferCallback(
        ChiFeature2RequestObject*        pRequestObject,
        ChiFeature2Identifier*           pPortId,
        CHITARGETBUFFERINFOHANDLE        hBuffer,
        UINT64                           key
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFrameNumber
    ///
    /// @brief  Utility method to get frame number
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    /// @param  requestId       Request id
    ///
    /// @return Current stage Id if stage info has been set, InvalidStageId otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT64 GetFrameNumber(
        ChiFeature2RequestObject*  pRequestObject,
        UINT8                      requestId
        ) const
    {
        UINT64                  frameNumber   = CDKInvalidId;
        ChiFeatureSequenceData* pSequenceData = NULL;
        if (NULL != pRequestObject)
        {
            pSequenceData = static_cast<ChiFeatureSequenceData*>(pRequestObject->GetSequencePrivData(
                ChiFeature2SequenceOrder::Current, requestId));
            if (NULL != pSequenceData)
            {
                frameNumber = pSequenceData->frameNumber;
            }
        }
        else
        {
            CHX_LOG_ERROR("Invalid Arg! pRequestObject == NULL");
        }
        return frameNumber;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetThreadManager
    ///
    /// @brief  Utility method to get thread manager

    /// @return Pointer to thread manager
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE CHIThreadManager* GetThreadManager() const
    {
        return m_pThreadManager;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMetadataClientId
    ///
    /// @brief  Utility method to set metadata client id for pipeline
    ///
    /// @param  pPipelineData  The pointer of pipeline data.
    /// @param  metaclientId   The metadata client id will be set for pipeline
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetMetadataClientId(
        ChiFeaturePipelineData* pPipelineData,
        UINT32                  metaclientId)
    {
        if (NULL != pPipelineData)
        {
            pPipelineData->metadataClientId = metaclientId;
            if (NULL != pPipelineData->pPipeline)
            {
                pPipelineData->pPipeline->SetMetadataClientId(metaclientId);
            }
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initializes the feature base & trigger derived feature's initialization.
    ///         Instance level initialization operations are performed. This function will invoke the virtual method
    ///         "DoInitialize" & the derived feature class is expected to make all instance level initialization operations.
    ///         As a rule, the derived class is not expected to contain any per-request state within the class.
    ///         Only feature instance level members are expected to be initialized.
    ///
    /// @param  pCreateInfo Feature create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        ChiFeature2CreateInputInfo* pCreateInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeActivatePipeline
    ///
    /// @brief  Deactivates the Pipeline.
    ///
    /// @param  pPortIdentifier Port identifier on which the output is generated.
    /// @param  modeBitmask     CHIDEACTIVATEPIPELINEMODE
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DeActivatePipeline(
        const ChiFeature2Identifier* pPortIdentifier,
        CHIDEACTIVATEPIPELINEMODE    modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnQueryCaps
    ///
    /// @brief  Virtual method to query the capabilities of the derived feature.
    ///         The derived feature can provide its own capabilities.
    ///
    /// @param  pQueryInfo Feature query information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnQueryCaps(
        ChiFeature2QueryInfo* pQueryInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPrepareRequest
    ///
    /// @brief  Virtual method to prepare processing request.
    ///         As a rule, the derived class must store all of its per-request context within the request object.
    ///         The derived class must save all per-request resources allocated & set the container as the
    ///         current request object's private context.
    ///         This function will be called by base class only once per-request during ExecuteProcessRequest.
    ///         The derived class is expected to set the request object's context once during this function.
    ///         At later stages of request processing derived class can modify the request object's private
    ///         context members but cannot set a new private context.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPrepareRequest(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnExecuteProcessRequest
    ///
    /// @brief  Virtual method to execute process request.
    ///         As a rule, the derived class must store all of its per-request state within the request object.
    ///         The derived class is expected to set the input config, output config for current feature's processing sequence.
    ///         The input dependency is set by the derived feature if needed for next feature's processing sequence.
    ///         The input information is available for the derived feature to get from the request object.
    ///         This input information is fetched by base class based on input dependency that was set by derived feature
    ///         in last feature's processing sequence.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnExecuteProcessRequest(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnProcessRequest
    ///
    /// @brief  Virtual method to process request.
    ///
    /// @param  pRequestObject  Feature request object.
    /// @param  requestId       RequestId on which to operate
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnProcessRequest(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnBufferResult
    ///
    /// @brief  Virtual method to process buffer callback from CHI driver.
    ///
    /// @param  pRequestObject          Feature request object instance.
    /// @param  resultId                FRO batch requestId
    /// @param  pStageInfo              Stage info to which this callback belongs
    /// @param  pPortIdentifier         Port identifier on which the output is generated
    /// @param  pStreamBuffer           Buffer from CamX on output image port
    /// @param  frameNumber             frameNumber for CamX result
    /// @param  pPrivateData            Private data of derived
    ///
    /// @return TRUE if the buffer should be sent to graph, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL OnBufferResult(
        ChiFeature2RequestObject*  pRequestObject,
        UINT8                      resultId,
        ChiFeature2StageInfo*      pStageInfo,
        ChiFeature2Identifier*     pPortIdentifier,
        const CHISTREAMBUFFER*     pStreamBuffer,
        UINT32                     frameNumber,
        VOID*                      pPrivateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnBufferError
    ///
    /// @brief  Virtual method to process buffer error from CHI driver.
    ///
    /// @param  pRequestObject          Feature request object instance.
    /// @param  pPortIdentifier         Port identifier on which the output is generated
    /// @param  resultId                FRO batch requestId
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID OnBufferError(
        ChiFeature2RequestObject*  pRequestObject,
        ChiFeature2Identifier*     pPortIdentifier,
        UINT8                      resultId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPartialMetadataResult
    ///
    /// @brief  Virtual method to process metadata callback from CHI driver.
    ///
    /// @param  pRequestObject          Feature request object instance.
    /// @param  resultId                FRO batch requestId
    /// @param  pStageInfo              Stage info to which this callback belongs
    /// @param  pPortIdentifier         Port identifier on which the output is generated
    /// @param  pMetadata               Metadata from CamX on output image port
    /// @param  frameNumber             frameNumber for CamX result
    /// @param  pPrivateData            Private data of derived
    ///
    /// @return TRUE if the metadata should be sent to graph, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL OnPartialMetadataResult(
        ChiFeature2RequestObject*  pRequestObject,
        UINT8                      resultId,
        ChiFeature2StageInfo*      pStageInfo,
        ChiFeature2Identifier*     pPortIdentifier,
        ChiMetadata*               pMetadata,
        UINT32                     frameNumber,
        VOID*                      pPrivateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnMetadataResult
    ///
    /// @brief  Virtual method to process metadata callback from CHI driver.
    ///
    /// @param  pRequestObject          Feature request object instance.
    /// @param  resultId                FRO batch requestId
    /// @param  pStageInfo              Stage info to which this callback belongs
    /// @param  pPortIdentifier         Port identifier on which the output is generated
    /// @param  pMetadata               Metadata from CamX on output image port
    /// @param  frameNumber             frameNumber for CamX result
    /// @param  pPrivateData            Private data of derived
    ///
    /// @return TRUE if the metadata should be sent to graph, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL OnMetadataResult(
        ChiFeature2RequestObject*  pRequestObject,
        UINT8                      resultId,
        ChiFeature2StageInfo*      pStageInfo,
        ChiFeature2Identifier*     pPortIdentifier,
        ChiMetadata*               pMetadata,
        UINT32                     frameNumber,
        VOID*                      pPrivateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnReleaseDependencies
    ///
    /// @brief  Virtual method to process releasing all dependencies
    ///
    ///         Base class will by default release all dependencies asked for a particular sequence. If this function
    ///         isoverriden, derived class will need to convey whether to release a particular buffer asked on given portId
    ///
    /// @param  pPortIdentifier          PortIdentifier on which to release dependency
    /// @param  dependencyIndex          Dependency Index for the given port
    /// @param  pStageInfo               Stage info to which this callback belongs
    /// @param  pRequestObject           Feature request object instance.

    /// @return TRUE if the given port should be released with the particular index
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL OnReleaseDependencies(
        const ChiFeature2Identifier*    pPortIdentifier,
        UINT8                           dependencyIndex,
        ChiFeature2StageInfo*           pStageInfo,
        ChiFeature2RequestObject*       pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnProcessMessage
    ///
    /// @brief  Virtual method to process message callback from CHI driver.
    ///
    /// @param  pMessageDescriptor      Message from CHI driver.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID OnProcessMessage(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
        VOID*                       pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoCleanupRequest
    ///
    /// @brief  Pure virtual method to cleanup processing request.
    ///         This function will be called by base class only once at the end of the feature request object's execution.
    ///         The derived class is expected to cleanup any per-request resource allocated & set in current request object's
    ///         private context.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoCleanupRequest(
        ChiFeature2RequestObject* pRequestObject) const = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoFlush
    ///
    /// @brief  Pure virtual method to flush.
    ///         The derived feature executes & stores context on a per-request basis.
    ///         The derived feature should re-initialize any feature instance level data to defaults if applicable.
    ///         The DoCleanupRequest will be called for all the pending request objects by the base class & derived
    ///         feature should cleanup the request object context.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoFlush() = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPopulateDependency
    ///
    /// @brief  Virtual function to populate dependency as per stage descriptor
    ///         Derived features can override this to set its own feature setting and populating logic
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPopulateDependency(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPopulateDependencySettings
    ///
    /// @brief  Virtual function to populate previous Feature Settings
    ///         Derived features can override this to set its own feature setting and populating logic
    ///
    /// @param  pRequestObject    Feature request object instance.
    /// @param  dependencyIndex   Dependency index
    /// @param  pSettingPortId    Metadata port id
    /// @param  pFeatureSettings  Metadata Setting
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPopulateDependencySettings(
        ChiFeature2RequestObject*     pRequestObject,
        UINT8                         dependencyIndex,
        const ChiFeature2Identifier*  pSettingPortId,
        ChiMetadata*                  pFeatureSettings
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnProcessingDependenciesComplete
    ///
    /// @brief  Check whether the request is done processing and generate a feature message callback if it is
    ///
    /// @param  pRequestObject  The FRO that the request is associated with
    /// @param  requestId       The FRO batch requestId
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID OnProcessingDependenciesComplete(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateDependency
    ///
    /// @brief  Virtual function to populate dependency as per stage descriptor
    ///         Derived features can override this to set its own feature setting and populating logic
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PopulateDependency(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateDependencyPorts
    ///
    /// @brief  Virtual function to populate dependency on every ports. Also populate settings for metadata port
    ///
    /// @param  pRequestObject    Feature request object instance.
    /// @param  dependencyIndex   dependecy index
    /// @param  pInputDependency  Input dependency port info
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PopulateDependencyPorts(
        ChiFeature2RequestObject*         pRequestObject,
        UINT8                             dependencyIndex,
        const ChiFeature2InputDependency* pInputDependency
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateDependencySettings
    ///
    /// @brief  Virtual function to populate previous Feature Settings
    ///
    /// @param  pRequestObject     Feature request object instance.
    /// @param  dependencyIndex    Dependency index
    /// @param  pSettingPortId     Metadata port id
    /// @param  pFeatureSettings   Metadata Setting
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PopulateDependencySettings(
        ChiFeature2RequestObject*     pRequestObject,
        UINT8                         dependencyIndex,
        const ChiFeature2Identifier*  pSettingPortId,
        ChiMetadata*                  pFeatureSettings
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareZSLQueue
    ///
    /// @brief  Prepare ZSL Queue for this request
    ///
    /// @param  pRequestObject     Feature request object instance.
    /// @param  pList              List of ports on which ZSL Queue is needed
    /// @param  pSyncPort          Port id on which to synchronize the buffers
    /// @param  pSettingsPort      Port id from which to extract the ZSL/NZSL settings
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PrepareZSLQueue(
        ChiFeature2RequestObject*     pRequestObject,
        const ChiFeature2PortIdList*  pList,
        const ChiFeature2Identifier*  pSyncPort,
        const ChiFeature2Identifier*  pSettingsPort
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckZSLQueueEmptyForPort
    ///
    /// @brief  Check if ZSL Queue is empty
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information.
    /// @param  pIdentifier     Port identifier for the ZSL Queue
    ///
    /// @return TRUE if ZSL queue is empty FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckZSLQueueEmptyForPort(
        ChiFeature2RequestObject*         pRequestObject,
        const ChiFeature2Identifier*      pIdentifier
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessReleaseDependency
    ///
    /// @brief  Trigger release dependency details/Callback to client
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information.
    /// @param  requestId       Batch index for this callback
    /// @param  sequenceId      Request object sequence Id
    /// @param  pStageInfo      Stage Info for this callback

    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessReleaseDependency(
        ChiFeature2RequestObject*    pRequestObject,
        UINT8                        requestId,
        UINT8                        sequenceId,
        ChiFeature2StageInfo*        pStageInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetExternalInputSettings
    ///
    /// @brief  Virtual function to get Downstream features settings requested
    ///
    /// @param  pRequestObject     Feature request object instance.
    /// @param  pMetadataPortId    Metadata port id
    /// @param  requestId          RequestId
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiMetadata* GetExternalInputSettings(
        ChiFeature2RequestObject*     pRequestObject,
        const ChiFeature2Identifier*  pMetadataPortId,
        const UINT8                   requestId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MergeAppSettings
    ///
    /// @brief  Virtual function to merge APP metadata with feature settings to get  final Metadata
    ///
    /// @param  pRequestObject    Feature request object instance.
    /// @param  ppResultMetadata  result metadata
    /// @param  isDisjoint        Flag to indicate whether the merge is disjoint
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult MergeAppSettings(
        ChiFeature2RequestObject* pRequestObject,
        ChiMetadata**             ppResultMetadata,
        BOOL                      isDisjoint
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2Base
    ///
    /// @brief  Deafault constructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2Base() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2Base
    ///
    /// @brief  Virtual Destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFeature2Base();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnInitialize
    ///
    /// @brief  Function creates Sessions and Pipelines based on input descriptor.
    ///         Derived features can override this to create sessions and pipelines or for virtual camx impl.
    ///
    /// @param  pCreateInputInfo  Feature create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnInitialize(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnSessionCreate
    ///
    /// @brief  Function updates session specific datastructures
    ///         Derived features can override this to update session structures or activate pipelines.
    ///
    /// @param  pKey Session global Id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnSessionCreate(
        ChiFeature2Identifier* pKey);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPipelineSelect
    ///
    /// @brief  Function to select pipeline.
    ///         Derived features can override this to select the pipeline that needs to be created.
    ///
    /// @param  pPipelineName        Pipeline name.
    /// @param  pCreateInputInfo     Feature create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPipelineSelect(
        const CHAR*                         pPipelineName,
        const ChiFeature2CreateInputInfo*   pCreateInputInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPruneUsecaseDescriptor
    ///
    /// @brief  Function to select pipeline.
    ///         Derived features can override this to select the pipeline that needs to be created.
    ///
    /// @param  pCreateInputInfo   [IN] Feature create input information.
    /// @param  rPruneVariants    [OUT] Vector of prune properties
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPruneUsecaseDescriptor(
        const ChiFeature2CreateInputInfo*   pCreateInputInfo,
        std::vector<PruneVariant>&          rPruneVariants
        ) const;


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPreparePipelineCreate
    ///
    /// @brief  Function updates pipeline specific datastructures before creating camx pipeline
    ///         Derived features can override this to update session structures.
    ///
    /// @param  pKey Pipeline global Id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPreparePipelineCreate(
        ChiFeature2Identifier* pKey);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPipelineCreate
    ///
    /// @brief  Function to publish pipeline created.
    ///         Derived features can override this to update pipeline data structures.
    ///
    /// @param  pKey Pipeline global Id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPipelineCreate(
        ChiFeature2Identifier* pKey);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPortCreate
    ///
    /// @brief  Function to publish port data created.
    ///         Derived features can override this to update port data structures.
    ///
    /// @param  pKey Port global Id
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPortCreate(
        ChiFeature2Identifier* pKey);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnInitializeStream
    ///
    /// @brief  Selecting stream based on port specification
    ///         Derived classes can override to handle custom stream formats for intra stage ports.
    ///
    /// @param  pTargetDesc        Target descriptor
    /// @param  pPortDesc          Feature port descriptor
    /// @param  pOutputStreamData  Output stream data
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnInitializeStream(
        const ChiTargetPortDescriptor*    pTargetDesc,
        const ChiFeature2PortDescriptor*  pPortDesc,
        ChiFeatureStreamData*             pOutputStreamData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnSubmitRequestToSession
    ///
    /// @brief  Submitting pipeline request to session
    ///         Derived classes can override to handle custom logic for submitting requests to session
    ///
    /// @param  pPipelineRequest   Pipeline request
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnSubmitRequestToSession(
        ChiPipelineRequest*    pPipelineRequest) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeTargetStream
    ///
    /// @brief  Selecting stream based on port specification
    ///
    /// @param  pTargetDesc        Target descriptor
    /// @param  pPortDesc          Feature port descriptor
    /// @param  pOutputStreamData  Output stream data
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeTargetStream(
        const ChiTargetPortDescriptor*    pTargetDesc,
        const ChiFeature2PortDescriptor*  pPortDesc,
        ChiFeatureStreamData*             pOutputStreamData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateConfiguration
    ///
    /// @brief  Utility method to populate input/output configuration for the current stage
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PopulateConfiguration(
        ChiFeature2RequestObject*       pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPopulateConfiguration
    ///
    /// @brief  The base feature implementation populates request object configuration for current stage
    ///         Derived class can override this function
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPopulateConfiguration(
        ChiFeature2RequestObject*       pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulatePortConfiguration
    ///
    /// @brief  Utility method to populate input/output configuration for the current stage
    ///         The base feature implementation populates request object configuration for current stage
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    /// @param  pInputList          List of input ports
    /// @param  pOutputList         List of output ports
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PopulatePortConfiguration(
        ChiFeature2RequestObject*       pRequestObject,
        const ChiFeature2PortIdList*    pInputList,
        const ChiFeature2PortIdList*    pOutputList
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnSelectFlowToExecuteRequest
    ///
    /// @brief  Method for derived features to select base provided request flows
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    ///
    /// @return ChiFeature2RequestFlowType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ChiFeature2RequestFlowType OnSelectFlowToExecuteRequest(
        ChiFeature2RequestObject*       pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateConfigurationSettings
    ///
    /// @brief  Utility method to populate input/output configuration setting
    ///         The base feature implementation populates request object configuration for current stage
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    /// @param  pMetadataPortId     metadata portId
    /// @param  pInputMetadata      Input metadata setting
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PopulateConfigurationSettings(
        ChiFeature2RequestObject*     pRequestObject,
        const ChiFeature2Identifier*  pMetadataPortId,
        ChiMetadata*                  pInputMetadata
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetExternalOutputSettings
     ///
    /// @brief  Get input configuration from request object. This is set by upstream feature
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    /// @param  pMetadataPortId     metadata portId
    /// @param  requestId           requestId for external setting
    /// @param  dependencyIndex     dependency index
    ///
    /// @return input metadata on this metadata port
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiMetadata* GetExternalOutputSettings(
        ChiFeature2RequestObject*     pRequestObject,
        const ChiFeature2Identifier*  pMetadataPortId,
        UINT8                         requestId,
        UINT8                         dependencyIndex
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPopulateConfigurationSettings
    ///
    /// @brief  The base feature implementation populates request object configuration for current stage
    ///         Derived class can override this.
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    /// @param  pMetadataPortId     metadata portId
    /// @param  pInputMetadata      Input metadata setting
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPopulateConfigurationSettings(
        ChiFeature2RequestObject*     pRequestObject,
        const ChiFeature2Identifier*  pMetadataPortId,
        ChiMetadata*                  pInputMetadata
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetNextStageInfoFromStageDescriptor
    ///
    /// @brief  Utility method to set next stage info from given stage descriptor
    ///         The base feature implementation will populate and set next stage info from given stage descriptor
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information for request submission.
    /// @param  pStageDescriptor    Stage descriptor containing information about this particular stage
    /// @param  stageSequenceId     Stage sequence id for this stage
    /// @param  maxDependencies     Maximum dependencies for this stage

    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetNextStageInfoFromStageDescriptor(
        ChiFeature2RequestObject*           pRequestObject,
        const ChiFeature2StageDescriptor*   pStageDescriptor,
        UINT8                               stageSequenceId,
        UINT8                               maxDependencies
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SubmitRequestToSession
    ///
    /// @brief  Utility method to submit request to camx session.
    ///         The base feature implementation configures and submits a new request to camx session:pipelines
    ///         based on current stageId.
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SubmitRequestToSession(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFeatureMessage
    ///
    /// @brief  Process message callback from feature internal
    ///
    /// @param  pFeatureMessage      Message from CHI driver.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessFeatureMessage(
        ChiFeature2MessageDescriptor*   pFeatureMessage
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentStageInfo
    ///
    /// @brief  Utility method to query current stage Id
    ///         Query current stageId from request object. Will return InvalidStageId during initialization
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    /// @param  pStageInfo      Output Stage information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetCurrentStageInfo(
        ChiFeature2RequestObject*   pRequestObject,
        ChiFeature2StageInfo*       pStageInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsRequestInFinalStageSequence
    ///
    /// @brief  Utility method to query whether request is in its final stage sequence
    ///
    /// @param  stageInfo  Stage information for the request
    /// @param  pHint      Hint for the request
    ///
    /// @return True if this request is in final stage sequence, False otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsRequestInFinalStageSequence(
        ChiFeature2StageInfo   stageInfo,
        ChiFeature2Hint*       pHint)
    {
        BOOL isFinalStageSequence = FALSE;

        // If we are in the final stage, and the stage sequenceId is equal to the total number of stage sequences we
        // set in the hint, mark that we are in the final stage sequence
        if (((GetNumStages() - 1) == stageInfo.stageId) &&
            ((pHint->stageSequenceInfo.size() == (stageInfo.stageSequenceId + 1)) ||
             (0 == pHint->stageSequenceInfo.size())))
        {
            isFinalStageSequence = TRUE;
        }

        return isFinalStageSequence;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSensorModeIndex
    ///
    /// @brief  Utility method to query Sensor Mode Index
    ///
    /// @param  pMetadataPortId   Metadata port id
    ///
    /// @return Sensor Mode Index
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetSensorModeIndex(
        const ChiFeature2Identifier*  pMetadataPortId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStreamBuffer
    ///
    /// @brief  Get pointer to stream buffer from target buffer info handle
    ///
    /// @param  handle      Target buffer info handle containing the associated stream buffer
    /// @param  key         Unique key identifying the streamBuffer
    /// @param  pBuffer     Pointer to Underlying streamBuffer
    ///
    /// @return Stream Buffer pointer from given handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetStreamBuffer(
        CHITARGETBUFFERINFOHANDLE handle,
        UINT64                    key,
        CHISTREAMBUFFER*          pBuffer
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStream
    ///
    /// @brief  Get pointer to stream from resource list by index
    ///
    /// @param  streamIndex stream index for which stream is needed
    ///
    /// @return Pipeline pointer on successful or NULL for failures
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE CHISTREAM* GetStream(
        UINT32 streamIndex)
    {
        return streamIndex < m_pStreams.size() ?
            m_pStreams[streamIndex] : NULL;
    }
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSessionData
    ///
    /// @brief  Get pointer to session data
    ///
    /// @param  pKey Global key to get session
    ///
    /// @return session data pointer on successful or NULL for failures
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiFeatureSessionData* GetSessionData(
        const ChiFeature2Identifier* pKey) const
    {
        return ((NULL != pKey) && (pKey->session < m_pSessionData.size())) ?
            m_pSessionData[pKey->session] : NULL;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineData
    ///
    /// @brief  Get pointer to pipeline data
    ///
    /// @param  pKey Global key to get session
    ///
    /// @return Pipeline data pointer on successful or NULL for failures
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiFeaturePipelineData* GetPipelineData(
        const ChiFeature2Identifier* pKey) const
    {
        ChiFeatureSessionData* pSessionData = GetSessionData(pKey);
        return ((NULL != pKey) && (NULL != pSessionData) &&
            (pKey->pipeline < pSessionData->pPipelineData.size())) ?
            pSessionData->pPipelineData[pKey->pipeline] : NULL;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPortData
    ///
    /// @brief  Get pointer to port data
    ///
    /// @param  pKey Global key to get session
    ///
    /// @return port data pointer on successful or NULL for failures
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiFeaturePortData* GetPortData(
        const ChiFeature2Identifier* pKey) const
    {
        ChiFeaturePipelineData* pPipelineData = GetPipelineData(pKey);
        ChiFeaturePortData*     pPortData     = NULL;

        if ((NULL != pKey) && (NULL != pPipelineData))
        {
            if (pKey->portDirectionType == ChiFeature2PortDirectionType::InternalInput ||
                pKey->portDirectionType == ChiFeature2PortDirectionType::ExternalInput)
            {
                pPortData = (pKey->port < pPipelineData->pInputPortData.size()) ?
                    &pPipelineData->pInputPortData[pKey->port] :
                    NULL;
            }
            else
            {
                pPortData = (pKey->port < pPipelineData->pOutputPortData.size()) ?
                    &pPipelineData->pOutputPortData[pKey->port] :
                    NULL;
            }
        }
        return pPortData;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumStages
    ///
    /// @brief  Get Number of stages in descriptor
    ///
    /// @return Get Number of stages in descriptor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT8 GetNumStages() const
    {
        return m_pStageData.size();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStageDescriptor
    ///
    /// @brief  Get stageDescriptor for given stage
    ///
    /// @param  stageId Stage Index to get stage info
    ///
    /// @return ChiFeature2StageDescriptor pointer on successful or NULL for failures
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiFeature2StageDescriptor* GetStageDescriptor(
        UINT32 stageId) const
    {
        UINT8 numStages = GetNumStages();

        ChiFeature2StageDescriptor* pStageDesciptor = NULL;

        if (numStages > stageId)
        {
            pStageDesciptor = &m_pStageData[stageId]->stageDescriptor;
        }
        else
        {
            CHX_LOG_ERROR("Invalid stageId %d numStages %d", stageId, numStages);
        }

        return pStageDesciptor;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPortsForStage
    ///
    /// @brief  Get all Output Ports for given stage
    ///
    /// @param  stageId     Stage Index to get stage info
    /// @param  pList       PortList to get required list information
    ///
    /// @return CDKResultSuccess upon success, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE CDKResult GetOutputPortsForStage(
        UINT32                  stageId,
        ChiFeature2PortIdList*  pList
    ) const
    {
        CDKResult               result = CDKResultSuccess;

        UINT8                numStages  = GetNumStages();
        ChiFeatureStageData* pStageData = NULL;

        if (numStages > stageId)
        {
            pStageData = m_pStageData[stageId];

            if (NULL != pStageData && NULL != pList)
            {
                pList->numPorts = pStageData->outputPorts.size();
                pList->pPorts   = pStageData->outputPorts.data();
            }
        }
        else
        {
            CHX_LOG_ERROR("Invalid stageId %d numStages %d", stageId, numStages);
            result = CDKResultEInvalidArg;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputPortsForStage
    ///
    /// @brief  Get all Input Ports for given stage
    ///
    /// @param  stageId     Stage Index to get stage info
    /// @param  pList       PortList to get required list information
    ///
    /// @return CDKResultSuccess upon success, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE CDKResult GetInputPortsForStage(
        UINT32                  stageId,
        ChiFeature2PortIdList*  pList
    ) const
    {
        CDKResult               result = CDKResultSuccess;

        UINT8                numStages  = GetNumStages();
        ChiFeatureStageData* pStageData = NULL;

        if (numStages > stageId)
        {
            pStageData = m_pStageData[stageId];

            if (NULL != pStageData && NULL != pList)
            {
                pList->numPorts = pStageData->inputPorts.size();
                pList->pPorts   = pStageData->inputPorts.data();
            }
        }
        else
        {
            CHX_LOG_ERROR("Invalid stageId %d numStages %d", stageId, numStages);
            result = CDKResultEInvalidArg;
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetConfigInfo
    ///
    /// @brief  Set maximum number of sequences that the FRO will support. This is a mandatory API that needs to be called by
    ///         the derived class as FRO uses this to manage maximum sequences it can support.
    ///
    /// @param  pRequestObject  Feature request object instance.
    /// @param  maxSequence     Maximum Sequences.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetConfigInfo(
        ChiFeature2RequestObject* pRequestObject,
        UINT                      maxSequence
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetFeaturePrivContext
    ///
    /// @brief  Set private client data
    ///         As a rule, the derived feature is not expected to hold any state outside the request object context.
    ///         This function is used to set the derived feature's private client data.
    ///
    /// @param  pRequestObject  Feature request object instance.
    /// @param  pClientData     Pointer to private client data.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetFeaturePrivContext(
        ChiFeature2RequestObject* pRequestObject,
        VOID*                     pClientData
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeaturePrivContext
    ///
    /// @brief  Get private client data
    ///         The derived feature can get access to its own private data for current request.
    ///         As a rule, the derived feature is not expected to hold any state outside the request object context.
    ///         The size of the context is set during feature query caps.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return Pointer to the private client data upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetFeaturePrivContext (
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSequencePrivContext
    ///
    /// @brief  Get private client data for this sequence
    ///         The derived feature can get access to its own private data for current request.
    ///         As a rule, the derived feature is not expected to hold any state outside the request object context.
    ///         The size of the context is set during feature query caps.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return Pointer to the private client data upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID* GetSequencePrivContext(
        ChiFeature2RequestObject* pRequestObject) const
    {
        return pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current,
            pRequestObject->GetCurRequestId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPortEnabledInFinalOutput
    ///
    /// @brief  Check if given port is requested in final output
    ///
    /// @param  pRequestObject  Feature request object instance.
    /// @param  identifier      Port identifier to check
    /// @param  requestId       Batch requestId
    ///
    /// @return TRUE if the requested port is enabled in final output
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPortEnabledInFinalOutput(
        ChiFeature2RequestObject* pRequestObject,
        ChiFeature2Identifier     identifier,
        UINT8                     requestId = 0
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeatureRequestOpsData
    ///
    /// @brief  Get Feature request objects data by Ops type
    ///         The derived feature can get access data inside FRO's buffer and config data
    ///
    /// @param  pRequestObject   Feature request object instance.
    /// @param  opsType          Feature Request Ops type
    /// @param  hOpsData         Input data for ops type
    /// @param  handleType       Port handle type.
    /// @param  hTypeData        Port handle as described by the type.
    /// @param  requestId        RequestId
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetFeatureRequestOpsData (
        ChiFeature2RequestObject*        pRequestObject,
        ChiFeature2RequestObjectOpsType  opsType,
        CHIFEATURE2HANDLE                hOpsData,
        ChiFeature2HandleType            handleType,
        CHIFEATURE2HANDLE                hTypeData,
        UINT8                            requestId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataManager
    ///
    /// @brief  Get Metadata manager for this feature
    ///
    /// @return Pointer to Metadata manager
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiMetadataManager* GetMetadataManager() const
    {
        return m_pMetadataManager;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSessionSettings
    ///
    /// @brief  Get Session settings for this feature
    ///
    /// @return Pointer to Session settings
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const VOID* GetSessionSettings()
    {
        return m_pSessionSettings;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDependencyListFromStageDescriptor
    ///
    /// @brief  Utility method to extract dependency list from descriptor
    ///
    /// @param  pStageDescriptor Stage descriptor from which to extract dependencyList
    /// @param  sessionIndex     Session Index within the stage descriptor
    /// @param  pipelineIndex    Pipeline Index within the stage descriptor
    /// @param  listIndex        Listindex within the dependency array
    ///
    /// @return Pointer to dependency list
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const ChiFeature2InputDependency* GetDependencyListFromStageDescriptor(
        const ChiFeature2StageDescriptor* pStageDescriptor,
        UINT8                             sessionIndex,
        UINT8                             pipelineIndex,
        UINT8                             listIndex
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumDependencyListsFromStageDescriptor
    ///
    /// @brief  Utility method to extract number of dependency list from stage descriptor
    ///
    /// @param  pStageDescriptor Stage descriptor from which to extract number of dependencyList
    /// @param  sessionIndex     Session Index within the stage descriptor
    /// @param  pipelineIndex    Pipeline Index within the stage descriptor
    ///
    /// @return Number of dependency lists for given session and pipeline Index
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT8 GetNumDependencyListsFromStageDescriptor(
        const ChiFeature2StageDescriptor* pStageDescriptor,
        UINT8                             sessionIndex,
        UINT8                             pipelineIndex
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestCountByPort
    ///
    /// @brief  Number of batch request per FRO for this port
    ///
    /// @param  pRequestObject  Feature request object instance.
    /// @param  identifier      Port identifier to check
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT8 GetRequestCountByPort(
        ChiFeature2RequestObject* pRequestObject,
        ChiFeature2Identifier     identifier
        ) const
    {
        UINT8 count = 0;

        for (UINT8 reqIdx = 0; reqIdx < pRequestObject->GetNumRequests(); reqIdx++)
        {
            if (IsPortEnabledInFinalOutput(pRequestObject, identifier, reqIdx))
            {
                count++;
            }
        }

        return count;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSourcePort
    ///
    /// @brief  Get source port for a given sink port
    ///
    ///         Iterate through all internal link info from feature descriptor to identify the source port
    ///
    /// @param  pSinkPortDescriptor The sink port for which to get the corresponding source port
    ///
    /// @return ChiFeature2PortDescriptor* containing source port pointer, NULL otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const ChiFeature2PortDescriptor* GetSourcePort(
        const ChiFeature2PortDescriptor* pSinkPortDescriptor
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DebugDataCopy
    ///
    /// @brief  Deep-copy for debug-data, allocate memory for further processing and copy incoming data.
    ///         Debug-data errors are non-fatal, for that reason this function does not return anything.
    ///
    /// @param  pRequestObject      Feature request object containing the configuration information
    /// @param  pInputMetadata      Input metadata setting
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DebugDataCopy(
        ChiFeature2RequestObject*   pRequestObject,
        ChiMetadata*                pInputMetadata
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpDebugData
    ///
    /// @brief  Dump debug-data and tuning-metadata into a file
    ///
    /// @param  pDebugData          Data to write into file
    /// @param  sizeDebugData       Size of data to write
    /// @param  requestNumber       URO number
    /// @param  frameNumber         frameNumber for CamX result
    /// @param  pPipelineName       Pipeline name
    /// @param  pStageInfo          Stage info to which this callback belongs
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpDebugData(
        const VOID*                 pDebugData,
        const SIZE_T                sizeDebugData,
        const UINT                  requestNumber,
        const UINT                  frameNumber,
        const CHAR*                 pPipelineName,
        const ChiFeature2StageInfo* pStageInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessDebugData
    ///
    /// @brief  Gets all information required to to process debug-data and request a dump into a file
    ///
    /// @param  pRequestObject          Feature request object instance.
    /// @param  pStageInfo              Stage info to which this callback belongs
    /// @param  pPortIdentifier         Port identifier on which the output is generated
    /// @param  pMetadata               Metadata from CamX on output image port
    /// @param  frameNumber             frameNumber for CamX result
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessDebugData(
        ChiFeature2RequestObject*       pRequestObject,
        const ChiFeature2StageInfo*     pStageInfo,
        const ChiFeature2Identifier*    pPortIdentifier,
        ChiMetadata*                    pMetadata,
        UINT32                          frameNumber);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CanRequestContinueWithError
    ///
    /// @brief  Virtual method for determining whether a request can continue with error. Derived class
    ///         can override. Base returns false for any request that is partially or completely errored out
    ///
    /// @return TRUE if request can continue, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CHX_INLINE BOOL CanRequestContinueWithError() const
    {
        BOOL result = FALSE;
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMessage
    ///
    /// @brief  Process message callback from CHI driver.
    ///
    /// @param  pMessageDescriptor      Message from CHI driver.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessMessage(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
        VOID*                       pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendSubmitRequestMessage
    ///
    /// @brief  Utility function to generate and send a submit request message to graph
    ///
    /// @param  pPipelineRequest        Pipeline request pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SendSubmitRequestMessage(
        ChiPipelineRequest* pPipelineRequest) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleRequestError
    ///
    /// @brief  Handle request error by splitting the request error into buffer and result errors that we
    ///         then propagate through the graph and send to the framework
    ///
    /// @param  pMessageDescriptor      Message from CHI driver.
    /// @param  pFrameCbData            Frame Callback Data associated with the error message
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleRequestError(
        const CHIMESSAGEDESCRIPTOR*  pMessageDescriptor,
        ChiFeatureFrameCallbackData* pFrameCbData) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateStream
    ///
    /// @brief  Creates the stream.
    ///
    /// @return Index to stream array if successful or CDKInvalidId otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHISTREAM* CreateStream();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CloneStream
    ///
    /// @brief  Clone a given stream.
    ///
    /// @param  pSrc  the source stream to clone
    ///
    /// @return Pointer to the cloned stream, NULL otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHISTREAM* CloneStream(
        CHISTREAM * pSrc);

    UINT32                          m_featureId;        ///< Feature unique ID.
    const CHAR*                     m_pFeatureName;     ///< Feature name.
    const ChiFeature2InstanceProps* m_pInstanceProps;   ///< Instance properties.
    const LogicalCameraInfo*        m_pCameraInfo;      ///< Pointer to logical camera info
    JobHandle                       m_hFeatureJob;      ///< thread job handle for this feature
    ChiUsecase*                     m_pUsecaseDesc;     ///< Usecase descriptor pointer.
    std::vector<CHISTREAM *>        m_pStreams;         ///< allocated internal streams.

private:
    ///< Private datastructure definitions

    /// @brief Port ZSL Queue Data
    struct PortZSLQueueData
    {
        ChiFeature2Identifier              globalId;             ///< Feature Key (Session, Pipeline, Port)
        std::queue<UINT32>                 frameNumbers;         ///< Queue of frameNumbers in ZSL Queue
        std::vector<UINT32>                selectedZSLFrames;    ///< vector for storing picked ZSL frames
        ChiFeature2InFlightCallbackData    inflightCallbackData; ///< Inflight callback data
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequestThread
    ///
    /// @brief  Receives request for thread and calls StubThreadHandler
    ///
    /// @param  pThreadData  Thread info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* RequestThread(
        VOID* pThreadData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushThreadHandler
    ///
    /// @brief  Waits for thread to be signaled, calls ExecuteProcessRequestHelper after receiving signal
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FlushThreadHandler();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteFlush
    ///
    /// @brief  Executes flush after being called in thread
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ExecuteFlush();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteFlushHelper
    ///
    /// @brief  Executes flush for session
    ///
    /// @param  pSessionData    SessionData to be flushed
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteFlushHelper(
        ChiFeatureSessionData* pSessionData);

    /// @brief Private Context
    struct ChiFeatureRequestContext
    {
        ChiFeaturePrivateContext               privContext;             ///< Private Context for derived feature
        std::vector<PortZSLQueueData*>         pPortZSLQueues;          ///< List of all ZSL Queues
    };

    /// @brief Feature buffer data
    struct ChiFeatureBufferData
    {
        ChiFeature2Identifier                 globalId;                ///< Feature Key (Session, Pipeline, Port)
        INT8                                  offset;                  ///< Offset for port queue
        UINT8                                 numBufmeta;              ///< number of Buffer and Meta requested
        ChiFeature2BufferMetadataInfo*        pBufferMeta;             ///< Array of buffer and metadata holder
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleBatchRequest
    ///
    /// @brief  Per FRO, batch requests are process in this function
    ///
    /// @param  pRequestObject  Feature request object.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult HandleBatchRequest(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessInFlightBufferCallBack
    ///
    /// @brief  Static function to handle Inflight buffer callback from TBM
    ///         The callback function which is registered with TBM
    ///
    /// @param  hBuffer         Target buffer info handle
    /// @param  pCallbackdata   Callback data to identify port
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult ProcessInFlightBufferCallBack(
        CHITARGETBUFFERINFOHANDLE hBuffer,
        VOID*                     pCallbackdata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ThreadCallback
    ///
    /// @brief  Static function to handle the thread callback.
    ///         The thread callback function which is registered with the thread service.
    ///
    /// @param  pCallbackData   Callback data which will contain the feature object & current request object.
    ///
    /// @return NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* ThreadCallback(
        VOID* pCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessResultCallbackFromDriver
    ///
    /// @brief  Static function to process capture result callback from driver.
    ///
    /// @param  pCaptureResult          Capture result from driver
    /// @param  pPrivateCallbackData    Private callback data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ProcessResultCallbackFromDriver(
        CHICAPTURERESULT*   pCaptureResult,
        VOID*               pPrivateCallbackData)
    {
        ChiFeature2SessionCallbackData* pCallbackData = static_cast<ChiFeature2SessionCallbackData*>(pPrivateCallbackData);
        pCallbackData->pFeatureInstance->ProcessResult(pCaptureResult, pPrivateCallbackData);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMatchingIdentifier
    ///
    /// @brief  Gets the matching identifier for a given frame cb data
    ///
    /// @param  pFrameCbData          Frame callback data
    /// @param  pMessageDescriptor    Message descriptor related to the callback data
    ///
    /// @return ChiFeature2Identifier
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2Identifier* GetMatchingIdentifier(
        ChiFeatureFrameCallbackData*    pFrameCbData,
        const CHIMESSAGEDESCRIPTOR*     pMessageDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandlePostJob
    ///
    /// @brief  Post a job to threadmanager
    ///
    /// @param  pRequestObject  Feature request object.
    /// @param  requestId       requestId on which to operate
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult HandlePostJob(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessResult
    ///
    /// @brief  Process result callback from CHI driver.
    ///
    /// @param  pResult                 Result from CHI driver.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessResult(
        CHICAPTURERESULT*   pResult,
        VOID*               pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMessageCallbackFromDriver
    ///
    /// @brief  Static function to process message callback from driver.
    ///
    /// @param  pMessageDesc            Message descriptor containin message data
    /// @param  pPrivateCallbackData    Private callback data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ProcessMessageCallbackFromDriver(
        const CHIMESSAGEDESCRIPTOR* pMessageDesc,
        VOID*                       pPrivateCallbackData)
    {
        ChiFeature2SessionCallbackData* pCallbackData = static_cast<ChiFeature2SessionCallbackData*>(pPrivateCallbackData);
        pCallbackData->pFeatureInstance->ProcessMessage(pMessageDesc, pPrivateCallbackData);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseDependenciesOnInputResourcePending
    ///
    /// @brief  Destroy resource allocated during add dependency.
    ///
    /// @param  pRequestObject   Feature request object instance
    /// @param  requestId        request id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseDependenciesOnInputResourcePending(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseDependenciesOnDriverError
    ///
    /// @brief  Process error message callback from camx driver.
    ///
    /// @param  pIdentifier     Message from CHI driver.
    /// @param  pFrameCbData    Private callback data.
    /// @param  pFeatureReqObj  Feature Request Object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseDependenciesOnDriverError(
        const ChiFeature2Identifier*    pIdentifier,
        ChiFeatureFrameCallbackData*    pFrameCbData,
        ChiFeature2RequestObject*       pFeatureReqObj) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessErrorMessageFromDriver
    ///
    /// @brief  Process error message from CHI driver.
    ///
    /// @param  pMessageDescriptor      Message from CHI driver.
    /// @param  pFrameCbData            Frame callback Data
    /// @param  pSessionCallbackData    The session callback data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessErrorMessageFromDriver(
        const CHIMESSAGEDESCRIPTOR*     pMessageDescriptor,
        ChiFeatureFrameCallbackData*    pFrameCbData,
        ChiFeature2SessionCallbackData* pSessionCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateErrorFromIdentifiers
    ///
    /// @brief  Generate a Chi Error message from a given list of identifiers. Used when translating request
    ///         error to buffer and metadata errors. Because of graph dependency, we send out metadata errors last
    ///         so this helper function helps us generate and send out the errors for each list of identifiers we give
    ///         it: image buffers identifiers then metadata identifiers
    ///
    /// @param  pFrameCbData        Frame callback data
    /// @param  identifierList      List of port identifiers to create an error from
    /// @param  notification        Notification to hold the error
    /// @param  messageDescriptor   Message descripor for the error
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GenerateErrorFromIdentifiers(
        ChiFeatureFrameCallbackData*        pFrameCbData,
        std::vector<ChiFeature2Identifier>  identifierList,
        ChiFeature2Messages                 notification,
        CHIMESSAGEDESCRIPTOR                messageDescriptor
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleBufferAndResultError
    ///
    /// @brief  Handle buffer and result error message from CHI driver for special handling through the graph
    ///
    /// @param  pMessage            The feature message associated with the error
    /// @param  pFrameCbData        Frame Callback Data associated with the error message
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult HandleBufferAndResultError(
        ChiFeature2Messages*           pMessage,
        ChiFeatureFrameCallbackData*   pFrameCbData
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndReturnSkippedBufferAsError
    ///
    /// @brief  Check if any output buffers are skipped and return as error for framework buffers
    ///
    /// @param  pFeatureReqObj      The FRO associated with the error message
    /// @param  requestId           requestId on which to operate
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CheckAndReturnSkippedBufferAsError(
        ChiFeature2RequestObject* pFeatureReqObj,
        UINT8                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessPartialResultCallbackFromDriver
    ///
    /// @brief  Static function to process partial capture result callback from driver.
    ///
    /// @param  pCaptureResult          Partial capture result from driver
    /// @param  pPrivateCallbackData    Private callback data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE static VOID ProcessPartialResultCallbackFromDriver(
        CHIPARTIALCAPTURERESULT*    pCaptureResult,
        VOID*                       pPrivateCallbackData)
    {
        ChiFeature2SessionCallbackData* pCallbackData = static_cast<ChiFeature2SessionCallbackData*>(pPrivateCallbackData);
        pCallbackData->pFeatureInstance->ProcessPartialResult(pCaptureResult, pPrivateCallbackData);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessPartialResult
    ///
    /// @brief  Process partial capture result callback from CHI driver.
    ///
    /// @param  pCaptureResult          Partial capture result from CHI driver.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessPartialResult(
        CHIPARTIALCAPTURERESULT*   pCaptureResult,
        VOID*                      pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateRequest
    ///
    /// @brief  Validate the request.
    ///         Prepare request is the first place where a feature starts to process a request,
    ///         validate the request at the beginning of prepare request.
    ///
    /// @param  pRequestObject  Feature request object.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ValidateRequest(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandlePrepareRequest
    ///
    /// @brief  Prepares the request.
    ///         The current request just after initialization needs to be prepated for execution.
    ///         Any request level one-time operations can be done in this function. The derived class is also expected
    ///         to set its private context during this time.
    ///
    /// @param  pRequestObject  Feature request object.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult HandlePrepareRequest(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleExecuteProcessRequest
    ///
    /// @brief  Executes the request through different stages of processing.
    ///         The current request gets executed through different stages of processing. The feature base class invokes derived
    ///         implementation to determine the input & output resources depending on the current procesing stage.
    ///
    /// @param  pRequestObject  Feature request object.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult HandleExecuteProcessRequest(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleInputResourcePending
    ///
    /// @brief  Handles input dependency updates.
    ///         The request enters into input resource pending state when input dependencies are reported.
    ///         The input dependencies may be asychronously reported. This function will check for all available
    ///         input dependency information to be able to move to next state.
    ///
    /// @param  pRequestObject  Feature request object.
    /// @param  requestId       requestId on which to operate

    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult HandleInputResourcePending(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleOutputNotificationPending
    ///
    /// @brief  Handles notification of feature's output consumption.
    ///         The request enters into consumer notify pending state when output buffer is generated from the feature &
    ///         pending for the client to notify whether the generated output references are complete.
    ///         The notification from consumer may be asychronously reported. This function will check for all available
    ///         outputs pending for notification to be able to move to next state.
    ///
    /// @param  pRequestObject  Feature request object.
    /// @param  requestId       requestId on which to operate
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult HandleOutputNotificationPending(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleOutputResourcePending
    ///
    /// @brief  Handles notification of feature resource pending.
    ///         The request enters into consumer notify pending state when output buffer is generated from the feature &
    ///         pending for the client to notify whether the generated output references are complete.
    ///         The notification from consumer may be asychronously reported. This function will check for all available
    ///         outputs pending for notification to be able to move to next state.
    ///
    /// @param  pRequestObject  Feature request object.

    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult HandleOutputResourcePending(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CompleteRequest
    ///
    /// @brief  Moves the request status to complete. Upon successful completion of the request, destroys the
    ///         corresponding request information for this request.
    ///
    /// @param  pRequestObject  Feature request object.
    /// @param  requestId       requestId on which to operate

    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CompleteRequest(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareFeatureData
    ///
    /// @brief  Allocates feature resource and prepares features
    ///         Internal streams are created for every pipeline's input & output targets, that are specified
    ///         in the feature config.
    ///
    /// @param  pCreateInputInfo  Pointer to create input info
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PrepareFeatureData(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareStageData
    ///
    /// @brief  Allocates feature resource and prepares features
    ///         Internal streams are created for every pipeline's input & output targets, that are specified
    ///         in the feature config.
    ///
    /// @param  pCreateInputInfo  Pointer to create input info
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PrepareStageData(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroyFeatureData
    ///
    /// @brief  Deallocates feature resource data
    ///         Internal streams are created for every pipeline's input & output targets, that are specified in
    ///         the feature config.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DestroyFeatureData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AssignStreams
    ///
    /// @brief  Creates the internal streams and assign to Target.
    ///         Internal streams are created for every pipeline's input & output targets, that are specified in
    ///         the feature config.
    ///
    /// @param  pTargetDesc  Target Descriptor
    /// @param  pPortDesc    Port Descriptor
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult AssignStreams(
        ChiTargetPortDescriptor*          pTargetDesc,
        const ChiFeature2PortDescriptor*  pPortDesc);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AssignTargets
    ///
    /// @brief  Assign Target to all the feature ports
    ///
    /// @param  pTargetDesc   Target Descriptor
    /// @param  ppPortData    Output port data to be filled
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult AssignTargets(
        ChiTargetPortDescriptor*          pTargetDesc,
        std::vector<ChiFeaturePortData>*  ppPortData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFeatureData
    ///
    /// @brief  Creates Feature data like sessions, pipelines and streams
    ///
    /// @param  pFeatureDesc  Feature Descriptor
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateFeatureData(
        const ChiFeature2Descriptor* pFeatureDesc);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreatePipeline
    ///
    /// @brief  Creates the pipelines.
    ///         Feature's pipelines are created for every pipeline specified in the feature config descriptor.
    ///
    /// @param  pPipelineDesc  Pipeline Descriptor
    /// @param  pPipelineData  Pipeline data to be filled
    ///
    /// @return CHIPIPELINEHANDLE pointing to pipeline
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHIPIPELINEHANDLE CreatePipeline(
        const ChiFeature2PipelineDescriptor* pPipelineDesc,
        ChiFeaturePipelineData*              pPipelineData = NULL);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateSession
    ///
    /// @brief  Creates the sessions.
    ///         Feature's sessions are created for every pipeline group as specified in the feature config descriptor.
    ///
    /// @param  pSessionDescData  Session Data to be filled
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateSession(
        const ChiFeature2SessionDescriptor* pSessionDescData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateTargetBufferManagers
    ///
    /// @brief  Creates the target buffer manager objects.
    ///         The target buffer is created for every pipeline's output port.
    ///         The targets that are mapped to external feature ports can get buffers imported to them from framework
    ///         depending on per-request feature graph configuration.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateTargetBufferManagers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroyTargetBufferManagers
    ///
    /// @brief  Destroy the target buffer manager objects.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DestroyTargetBufferManagers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeThreadService
    ///
    /// @brief  Initializes the thread service.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeThreadService();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineIndex
    ///
    /// @brief  Get Pipeline index based on pipeline name
    ///
    /// @param  pUsecaseDesc   Usecase descriptor
    /// @param  pPipelineName  Pipeline name for which index is needed
    ///
    /// @return Pipeline index if successful or -1 for failures
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetPipelineIndex(
        const ChiUsecase*   pUsecaseDesc,
        const CHAR*         pPipelineName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputMetadataBuffer
    ///
    /// @brief  Get free output metadata buffer from given pipeline
    ///
    /// @param  pPipelineData   Pipeline data pointer
    /// @param  sequenceNumber  Sequence number for buffer
    ///
    /// @return Free metadata buffer from given pipeline, null otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHITARGETBUFFERINFOHANDLE GetOutputMetadataBuffer(
        const ChiFeaturePipelineData* pPipelineData,
        UINT32                        sequenceNumber
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputMetadataBuffer
    ///
    /// @brief  Get free input metadata buffer from given pipeline
    ///
    /// @param  pPipelineData   Pipeline data pointer
    /// @param  sequenceNumber  Sequence number for buffer
    ///
    /// @return Free metadata buffer from given pipeline, null otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHITARGETBUFFERINFOHANDLE GetInputMetadataBuffer(
        const ChiFeaturePipelineData* pPipelineData,
        UINT32                        sequenceNumber
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputBufferHandle
    ///
    /// @brief  Get free output target buffer for given port
    ///
    /// @param  pPortData       Port data pointer
    /// @param  sequenceNumber  Sequence number for buffer
    ///
    /// @return Target Buffer Info for given port null otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHITARGETBUFFERINFOHANDLE GetOutputBufferHandle(
        const ChiFeaturePortData* pPortData,
        UINT32                    sequenceNumber
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferData
    ///
    /// @brief  Get buffers by portIds
    ///
    /// @param  pRequestObject  Feature request object instance.
    /// @param  numData         number of output requested
    /// @param  pOutputBuffer   Pointer to list of ports and buffer data
    ///
    /// @return CDKResultSuccess upon success, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetBufferData(
        ChiFeature2RequestObject* pRequestObject,
        UINT8                     numData,
        ChiFeatureBufferData*     pOutputBuffer
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClassifyStream
    ///
    /// @brief  Read input streams and classfy based on internal structure
    ///
    /// @param  pStreams  Client configured streams
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ClassifyStream(
        const CHISTREAMCONFIGINFO* pStreams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ValidateFeatureDesc
    ///
    /// @brief  validate input feature descriptor data.
    ///
    /// @param  pCreateInputInfo  Input data for feature creation
    ///
    /// @return CDKResultSuccess  If successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ValidateFeatureDesc(
        ChiFeature2CreateInputInfo* pCreateInputInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AssignSessionSettings
    ///
    /// @brief  Session parameter updates are done in this function
    ///
    /// @param  pPipeline        Pipeline to set session parameter
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult AssignSessionSettings(
        Pipeline* pPipeline);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeFeatureContext
    ///
    /// @brief  allocate and initialize per frame feature context
    ///
    /// @param  pRequestObject  Per frame data to initialize
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeFeatureContext(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeSequenceData
    ///
    /// @brief  allocate and initialize sequence context
    ///
    /// @param  pRequestObject  Per frame data to initialize
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeSequenceData(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroySequenceData
    ///
    /// @brief  deallocate sequence context
    ///
    /// @param  pRequestObject  Per frame data to initialize
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DestroySequenceData(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CanRequestContinue
    ///
    /// @brief  Check if request can continue from current state
    ///         Check if request can continue to another iteration from current state
    ///
    /// @param  requestState  Validate request state
    ///
    /// @return TRUE if request can continue from current state, FALSE otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CanRequestContinue(
        ChiFeature2RequestState requestState);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PropagateError
    ///
    /// @brief  If an error has been detected, create correct message descriptors for error and propagate message to base's
    ///         error handling mechanism
    ///
    /// @param  pRequestObject  Feature request object.
    ///
    /// @return CDKResultSuccess or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PropagateError(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessDependency
    ///
    /// @brief  Trigger dependency details/Callback to client
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessDependency(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessBufferCallback
    ///
    /// @brief  Process Buffer Callback from CamX
    ///
    /// @param  pChiResult    Capture Result from CamX
    /// @param  pFrameCbData  Request callbackdata from CamX
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessBufferCallback(
        CHICAPTURERESULT*               pChiResult,
        ChiFeatureFrameCallbackData*    pFrameCbData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMetadataCallback
    ///
    /// @brief  Process metadata Callback from CamX
    ///
    /// @param  pChiResult    Capture Result from CamX
    /// @param  pFrameCbData  Request callbackdata from CamX
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessMetadataCallback(
        CHICAPTURERESULT*               pChiResult,
        ChiFeatureFrameCallbackData*    pFrameCbData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDependency
    ///
    /// @brief  Resolves dependencies reported by derived feature and updates graph/ buffer information accordingly.
    ///
    ///         The base feature implementation looks through each port reported as dependency by the derived feature.
    ///         For internal port dependency, updates the buffer/metadata info from corresponding output ports.
    ///         For external port dependency, informs graph about the dependency through callbacks.
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetDependency(
        ChiFeature2RequestObject* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLastShutterTimestamp
    ///
    /// @brief  Get last timestamp for this feature
    ///
    /// @return UINT64 Last shutter timestamp
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT64 GetLastShutterTimestamp() const
    {
        return m_lastShutterTimestamp;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetZSLOffsets
    ///
    /// @brief  Get ZSL offsets for a particular request
    ///
    ///         This function looks at all the requests in the FRO and calculates required offsets for ZSL Queue
    ///         Negative offsets indicate picking buffers from the consumer list
    ///         Positive offsets indicate picking buffers from the producer list
    ///
    /// @param  pRequestObject Feature request object containing the configuration information for request submission.
    /// @param  rProducerList  Reference to vector containing elements in the producer list
    /// @param  rConsumerList  Reference to vector containing elements in the consumer list
    /// @param  pSettingsPort      Port id from which to extract the ZSL/NZSL settings
    ///
    /// @return std::vector<INT32> containing offsets information
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<INT32> GetZSLOffsets(
        ChiFeature2RequestObject*       pRequestObject,
        std::vector<UINT32>&            rProducerList,
        std::vector<UINT32>&            rConsumerList,
        const ChiFeature2Identifier*    pSettingsPort
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetZSLQueueForPort
    ///
    /// @brief  Get ZSL Queue for a given port Id
    ///
    /// @param  pRequestObject Feature Request object
    /// @param  pIdentifier    PortId on which to get ZSL info
    /// @param  ppQueue        Output ZSL Queue

    /// @return CDKResultSuccess if queue is present
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetZSLQueueForPort(
        ChiFeature2RequestObject*       pRequestObject,
        const ChiFeature2Identifier*    pIdentifier,
        PortZSLQueueData**              ppQueue
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteBaseRequestFlow
    ///
    /// @brief  Execute request flow for given flow type
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    /// @param  flowType        Flowtype as selected by derived
    ///
    /// @return CDKResultSuccess upon successful execution, CDKResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteBaseRequestFlow(
        ChiFeature2RequestObject*   pRequestObject,
        ChiFeature2RequestFlowType  flowType
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteFlowType0
    ///
    /// @brief  Execute request flow for given flow type
    ///         This flow asks for single buffer on each of its input ports and uses the same port for submission to CamX
    ///         This flow should not be used where we need multiple buffers on same port
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess upon successful execution, CDKResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteFlowType0(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteFlowType1
    ///
    /// @brief  Execute request flow for given flow type
    ///         This flow asks for multiple batches of input dependencies on its ports based on buffer and metadata ports
    ///         and copies that into its internal input ports for submission
    ///
    /// @param  pRequestObject  Feature request object containing the configuration information for request submission.
    ///
    /// @return CDKResultSuccess upon successful execution, CDKResultEFailed otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ExecuteFlowType1(
        ChiFeature2RequestObject* pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpInputMetaBuffer
    ///
    /// @brief  Dump input meta data buffer for chifeature2Base
    ///
    /// @param  pInputMetaInfo  Input metadata info.
    /// @param  pBaseName       Base name to be used for determining the output dump file name
    /// @param  index           Multi frame sequence index.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpInputMetaBuffer(
        ChiFeature2BufferMetadataInfo* pInputMetaInfo,
        CHAR*                          pBaseName,
        UINT                           index
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpInputImageBuffer
    ///
    /// @brief  Dump input image buffer for chifeature2Base
    ///
    /// @param  pInputBufferInfo  Input raw image info.
    /// @param  pBaseName         Base name to be used for determining the output dump file name
    /// @param  index             Multi frame sequence index.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpInputImageBuffer(
        ChiFeature2BufferMetadataInfo* pInputBufferInfo,
        CHAR*                          pBaseName,
        UINT                           index
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUseCaseString
    ///
    /// @brief  Get a string corresponding to the use case passed in
    ///
    /// @param  useCase   Use case
    /// @param  output    Output string
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetUseCaseString(
        ChiModeUsecaseSubModeType useCase,
        std::string&              output
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDumpFileBaseName
    ///
    /// @brief  Get the base name to be used for all the dump files
    ///
    /// @param  pRequestObject      Feature request object
    /// @param  pDumpFileBaseName   Output dump file base name
    /// @param  size                Size of the array containing the dump file base name
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetDumpFileBaseName(
        ChiFeature2RequestObject* pRequestObject,
        CHAR*                     pDumpFileBaseName,
        UINT                      size
        ) const;

    ChiFeature2Base(const ChiFeature2Base&)             = delete;   ///< Disallow the copy constructor
    ChiFeature2Base& operator= (const ChiFeature2Base&) = delete;   ///< Disallow assignment operator

    /// Maps an external port id to a port descriptor
    std::map<ChiFeature2Identifier, ChiFeature2PortDescriptor, ChiFeature2PortIdLessComparator> m_portIdToPortDescMap;

    std::vector<ChiFeatureStageData*>   m_pStageData;             ///< Stage info for this feature.
    std::vector<ChiFeatureSessionData*> m_pSessionData;           ///< Session info for this feature.
    std::vector<ChiFeatureStreamData>   m_pStreamData;            ///< Stream info for this feature.
                                                                  ///  Target to internal stream map
    ChiFeature2GraphCallbacks           m_clientCallbacks;        ///< Methods used by the base class to callback to client

    ChiFeatureTargetStreams             m_targetStreams;          ///< Feature configured streams
    std::vector<CHISTREAM*>             m_configStreams;          ///< List of streams configured

    UINT8                               m_numInternalLinks;       ///< Number of internal links for this feature
    const ChiFeature2InternalLinkDesc*  m_pInternalLinkDesc;      ///< Pointer to graph link descriptor list

    UINT8                               m_numSessions;            ///< Number of sessions in this feature.
    const ChiFeature2SessionDescriptor* m_pSessionDescriptor;     ///< Pointer to session descriptor table.

    ChiMetadataManager*                 m_pMetadataManager;       ///< Metadata manager for this feature
    CHIThreadManager*                   m_pThreadManager;         ///< Thread Manager for this feature
    UINT64                              m_frameNumber;            ///< Frame number for feature
    UINT64                              m_lastShutterTimestamp;   ///< Latest shutter message received by this feature
    ChiUsecase*                         m_pClonedUsecase;         ///< cloned usecase
    static ChiFeature2RequestFlow       m_requestFlows[];         ///< Base provided requestFlows
    ChiSensorModePickHint               m_sensorModePickHint;     ///< Sensor mode pick hints
    const VOID*                         m_pSessionSettings;       ///< Session Settings
    Mutex*                              m_pProcessResultLock;     ///< Mutex to protect process result
    BOOL                                m_useResManager;          ///< Enable resource manager for realtime pipeline or not

    PerThreadData                       m_flushRequestProcessThread;    ///< Thread to process flush
    Mutex*                              m_pFlushThreadMutex;            ///< Flush Thread mutex
    Condition*                          m_pFlushRequestAvailable;       ///< Flush condition
    volatile BOOL                       m_flushRequestProcessTerminate; ///< indicate if thread is to be terminated
    BOOL                                m_shouldWaitFlush;              ///< indicate Flushhandler should enter wait state
    BOOL                                m_destroyInProgress;            ///< indicate graph manager destroy in progress
};

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/// @brief Pack data structures to ensure consistent layout.
#pragma pack(push, 8)

/// @brief feature2 stream negotiation input parameters
struct StreamNegotiationInfo
{
    LogicalCameraInfo*             pLogicalCameraInfo;        ///< Logical camera information
    ChiFeature2InstanceId*         pFeatureInstanceId;        ///< Feature Instance Id
    CHISTREAMCONFIGINFO*           pFwkStreamConfig;          ///< Framework stream
    CHISTREAMCONFIGINFO*           pDownStreamFeatureInput;   ///< Downstream feature input
    CHISTREAMCONFIGINFO*           pInputOption;              ///< Input option
    const CHAR*                    pFeatureName;              ///< feature Name
    ChiFeature2InstanceProps*      pInstanceProps;            ///< feature Instance Properties
};

typedef std::pair<UINT8, UINT8> PortIdCameraIndexMapPair;

/// @brief feature2 stream negotiation output information
struct StreamNegotiationOutput
{
    CHISTREAMCONFIGINFO*                    pDesiredStreamConfig;      ///< Desired stream configure
    std::vector<CHISTREAM*>*                pStreams;                  ///< Desired stream container
    std::vector<CHISTREAM*>*                pOwnedStreams;             ///< Streams allocated as part of negotiation
    BOOL                                    isFrameworkVideoStream;    ///< Flag to indicate framework video stream
    std::vector<PortIdCameraIndexMapPair>*  pInputPortMap;             ///< Input portId to cameraId map
    std::vector<PortIdCameraIndexMapPair>*  pOutputPortMap;            ///< Output portId to cameraId map
    BOOL                                    disableZoomCrop;           ///< is native resolution supported
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief feature2 ops APIs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// @brief to create a feature object
typedef VOID* (*PFNFEATURE2CREATE)(
    ChiFeature2CreateInputInfo* pCreateInputInfo);

// @brief to query the capabilities of this feature
typedef CDKResult (*PFNFEATURE2DOQUERYCAPS)(
    VOID*                 pConfig,
    ChiFeature2QueryInfo* pQueryInfo);

// @brief to query the vendor tags of this feature
typedef VOID (*PFNFEATURE2GETVENDORTAGS)(
    VOID* pVendorTagInfo);

// @brief to do stream negotiation of this feature
typedef CDKResult (*pFNSTREAMNEGOTIATION)(
    StreamNegotiationInfo*          pNegotiationInfo,         ///< Input information for negotiation
    StreamNegotiationOutput*        pNegotiationOutput);      ///< Output stream information

/// @brief feature2 ops
struct CHIFEATURE2OPS
{
    UINT32                    size;             ///< Size of this structure
    const CHAR*               pVendorName;      ///< The vendor of this feature
    UINT32                    majorVersion;     ///< Major version
    UINT32                    minorVersion;     ///< Minor version
    PFNFEATURE2CREATE         pCreate;          ///< Create this feature
    PFNFEATURE2DOQUERYCAPS    pQueryCaps;       ///< Query the capabilities
    PFNFEATURE2GETVENDORTAGS  pGetVendorTags;   ///< Initialize the feature
    pFNSTREAMNEGOTIATION      pStreamNegotiation; ///< Stream negotiation
};

// @brief function pointer for entry functions
typedef VOID (*PCHIFEATURE2OPSENTRY)(
    CHIFEATURE2OPS* pChiFeature2Ops);

#pragma pack(pop)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // CHIFEATURE2BASE_H
