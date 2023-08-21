////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2realtime.h
/// @brief CHI realtime feature derived class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2REALTIME_H
#define CHIFEATURE2REALTIME_H


#include "chifeature2base.h"
// NOWHINE FILE CP006: Need vector to pass filtered port information

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Definitions of static data describing the RealTime Derived feature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern const ChiFeature2PortDescriptor  RealTimeOutputPortDescriptors[];
extern const ChiFeature2PortDescriptor  ZSLInputPortDescriptors[];

/// @brief Sensor Pipeline delay
static const UINT32   SensorPipelineDelay           = 2;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Feature derived class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiFeature2RealTime : public ChiFeature2Base
{

public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static function to create RealTime feature
    ///
    /// @param  pCreateInputInfo   Pointer to create input info for RealTime feature
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2RealTime* Create(
        ChiFeature2CreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Virtual method to destroy.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPrepareRequest
    ///
    /// @brief  Virtual method to prepare processing request for RealTime feature.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPrepareRequest(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnExecuteProcessRequest
    ///
    /// @brief  Virtual method to execute process request for RealTime feature.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnExecuteProcessRequest(
        ChiFeature2RequestObject* pRequestObject) const;

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
    /// @param  pPrivateData            Private data
    ///
    /// @return TRUE if the metadata should be dispatched to graph, FALSE otherwise
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
    /// @param  pPrivateData            Private data
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
    /// DoCleanupRequest
    ///
    /// @brief  Virtual method to cleanup processing request.
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoCleanupRequest(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoFlush
    ///
    /// @brief  Virtual method to flush.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult DoFlush();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnInitializeStream
    ///
    /// @brief  Selecting stream based on port specification
    ///         Derived classes can override to handle custom stream formats for intra stage ports.
    ///
    /// @param  pTargetDesc        Target descriptor of stream
    /// @param  pPortDesc          Feature port descriptor of stream
    /// @param  pOutputStreamData  Output stream data information
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnInitializeStream(
        const ChiTargetPortDescriptor*    pTargetDesc,
        const ChiFeature2PortDescriptor*  pPortDesc,
        ChiFeatureStreamData*             pOutputStreamData);

private:

    /// @brief Realtime thread callback data
    struct RealtimeThreadCallbackData
    {
        CHIPIPELINEREQUEST*         pRequest;           ///< Request to be enqueue
        const ChiFeature2RealTime*  pRealtimeInstance;  ///< This feature instance
    };

    /// @brief Realtime context information
    struct ChiFeatureRealtimeContext
    {
        UINT8                   maxSequence;                ///< Total sequence for this FRO
        UINT8                   isManualCaptureNeeded;      ///< Manual setting is enabled for this request
        UINT8                   isInSensorHDR3ExpCapture;   ///< TRUE if in-sensor HDR 3 exp capture is enabled for snapshot
        SeamlessInSensorState   seamlessInSensorState;      ///< Seamless in-sensor control state
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2RealTime
    ///
    /// @brief  Default constructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2RealTime() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2RealTime
    ///
    /// @brief  Virtual Destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFeature2RealTime();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SelectOutputPorts
    ///
    /// @brief  Select output ports to enable
    ///
    /// @param  pRequestObject    request object instance
    /// @param  pAllOutputPorts   all output ports for this stage
    /// @param  stageSequenceId   stage sequenceId
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<ChiFeature2Identifier>SelectOutputPorts(
        ChiFeature2RequestObject* pRequestObject,
        ChiFeature2PortIdList*    pAllOutputPorts,
        UINT8                     stageSequenceId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddOutputPortsForBPSCamera
    ///
    /// @brief  Select output ports to enable
    ///
    /// @param  rOuputPorts      Reference to ouput ports vector
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AddOutputPortsForBPSCamera(
        std::vector<ChiFeature2Identifier>& rOuputPorts) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessZSLRequest
    ///
    /// @brief  Private function for ZSL functionality
    ///
    /// @param  pRequestObject   Pointer to Request Object
    /// @param  pPortIdList      Pointer to Port Id List
    /// @param  requestId        requestId for the ZSL request
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessZSLRequest(
        ChiFeature2RequestObject * pRequestObject,
        ChiFeature2PortIdList*     pPortIdList,
        UINT8                      requestId
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessSWRemosaicSnapshot
    ///
    /// @brief  Function to handle sw remosaic snasphot
    ///
    /// @param  stageId          Current stage Id
    /// @param  pRequestObject   Pointer to create input info for RealTime feature
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessSWRemosaicSnapshot(
        UINT8                      stageId,
        ChiFeature2RequestObject*  pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessResultMetadata
    ///
    /// @brief  Process output metadata from real time pipelines
    ///
    /// @param  pResultMetadata   Pointer to current output metadata
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessResultMetadata(
        ChiMetadata* pResultMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequestMetadata
    ///
    /// @brief  Process request settings for realtime feature
    ///
    /// @param  pRequestObject   Pointer to FRO object
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessRequestMetadata(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsManualCaptureRequired
    ///
    /// @brief  Function to determine if manual setting is enabled
    ///
    /// @param  pRequestObject   Request Object
    ///
    /// @return TRUE if Manual setting needed or FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsManualCaptureRequired(
        ChiFeature2RequestObject* pRequestObject) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPipelineSelect
    ///
    /// @brief  Function to select pipeline. Real-time feature will select between camera / camcorder pipeline.
    ///
    /// @param  pPipelineName        Pipeline name.
    /// @param  pCreateInputInfo     Feature create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult OnPipelineSelect(
        const CHAR*                         pPipelineName,
        const ChiFeature2CreateInputInfo*   pCreateInputInfo) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SendResultForPort
    ///
    /// @brief  send the result for given port.
    ///
    /// @param  pFeatureReqObj  pointer for FeatureReqObj.
    /// @param  fromIdentifier  From Port identifier.
    /// @param  toIdentifier    To Port identifier.
    /// @param  requestId       Batch requestId
    ///
    /// @return TRUE if the requested port is enabled in final output
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL SendResultForPort(
        ChiFeature2RequestObject* pFeatureReqObj,
        ChiFeature2Identifier     fromIdentifier,
        ChiFeature2Identifier     toIdentifier,
        UINT8                     requestId
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
    /// OnPortCreate
    ///
    /// @brief  Function to determine if manual setting is enabled
    ///
    /// @param  pKey   Key pointer
    ///
    /// @return TRUE if Manual setting needed or FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult OnPortCreate(
        ChiFeature2Identifier* pKey);

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
    CDKResult OnPopulateConfigurationSettings(
        ChiFeature2RequestObject*     pRequestObject,
        const ChiFeature2Identifier*  pMetadataPortId,
        ChiMetadata*                  pInputMetadata
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
    CDKResult OnInflightBufferCallback(
        ChiFeature2RequestObject*        pRequestObject,
        ChiFeature2Identifier*           pPortId,
        CHITARGETBUFFERINFOHANDLE        hBuffer,
        UINT64                           key
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRegOutDimensions
    ///
    /// @brief  In the BPS camera scenario, returns the optimal registration output resolution relative
    ///         to the current sensor mode resolution.
    ///
    /// @param  sensorWidth     Sensor width.
    /// @param  sensorHeight    Sensor height.
    /// @param  pRegOutWidth    Resulting registration output width
    /// @param  pRegOutHeight   Resulting registration output height
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetRegOutDimensions(
            UINT32  sensorWidth,
            UINT32  sensorHeight,
            UINT32* pRegOutWidth,
            UINT32* pRegOutHeight);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsInternalSequence
    ///
    /// @brief  Internal requests to support. Can be skipped
    ///
    /// @param  pRequestObject    request object instance
    /// @param  stageSequenceId   stage sequenceId
    ///
    /// @return TRUE if Manual setting needed or FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsInternalSequence(
        ChiFeature2RequestObject*     pRequestObject,
        UINT8                         stageSequenceId) const
    {
        BOOL                       internalSeq  = FALSE;
        ChiFeatureRealtimeContext* pPrivContext = static_cast<ChiFeatureRealtimeContext *>(
            GetFeaturePrivContext(pRequestObject));

        // Decide if this sequence result needs to be skipped here.
        // In case of IHDR, we trigger internal request of count SensorPipelineDelay while
        // switching sensor mode. Same is done while resetting sensor mode.
        if ((NULL != pPrivContext) && (TRUE == pPrivContext->isInSensorHDR3ExpCapture))
        {
            //  For single frame case
            if (1 == pRequestObject->GetNumRequests())
            {
                if ((stageSequenceId < SensorPipelineDelay)  ||
                    (stageSequenceId >= (pPrivContext->maxSequence - SensorPipelineDelay)))
                {
                    internalSeq = TRUE;
                }
            }
            else
            {
                if (((pRequestObject->GetCurRequestId() == 0)    &&
                     (stageSequenceId < SensorPipelineDelay))    ||
                    ((pRequestObject->GetCurRequestId() + 1 == pRequestObject->GetNumRequests()) &&
                     (stageSequenceId >= (pPrivContext->maxSequence - SensorPipelineDelay))))
                {
                    internalSeq = TRUE;
                }
            }
        }

        return internalSeq;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSWRemosaicSnapshot
    ///
    /// @brief  Check if it is sw remosaic snapshot or not
    ///
    /// @return TRUE if it is sw remosaic snapshot or FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsSWRemosaicSnapshot() const
    {
        return ((NULL != GetInstanceProps()) &&
                (TRUE == GetInstanceProps()->instanceFlags.isNZSLSnapshot) &&
                (TRUE == GetInstanceProps()->instanceFlags.isSWRemosaicSnapshot));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBPSCameraPipeline
    ///
    /// @brief  Check if it is a BPS camera pipeline or not
    ///
    /// @return TRUE if it is a BPS camera pipeline or FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsBPSCameraPipeline() const
    {
        return ((NULL  != GetInstanceProps()) &&
                (TRUE  == GetInstanceProps()->instanceFlags.isBPSCamera) &&
                (FALSE == GetInstanceProps()->instanceFlags.isNZSLSnapshot));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInSensorHDR3ExpState
    ///
    /// @brief  Return the in-sensor HDR state for now depends on requestId, stageSequenceId, numRequests and maxSequence.
    ///
    /// @param  pRequestObject    request object instance
    /// @param  stageSequenceId   stage sequenceId
    ///
    /// @return The IHDR state for current sequence
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE SeamlessInSensorState GetInSensorHDR3ExpState(
        ChiFeature2RequestObject*     pRequestObject,
        UINT8                         stageSequenceId) const
    {
        SeamlessInSensorState      returnState  = SeamlessInSensorState::None;
        UINT8                      requestId    = pRequestObject->GetCurRequestId();
        UINT8                      numRequests  = pRequestObject->GetNumRequests();
        ChiFeatureRealtimeContext* pPrivContext = static_cast<ChiFeatureRealtimeContext *>(
                                                    GetFeaturePrivContext(pRequestObject));

        //  Decide the IHDR state depends on requestId, stageSequenceId, numRequests and maxSequence.
        if (0 == requestId)
        {
            if (0 == stageSequenceId)
            {
                returnState = SeamlessInSensorState::None;
            }
            else if (1 == stageSequenceId)
            {
                returnState = SeamlessInSensorState::InSensorHDR3ExpStart;
            }
            else if ((1 == numRequests)         &&
                     (NULL != pPrivContext)     &&
                     ((pPrivContext->maxSequence - 1) == stageSequenceId))
            {
                returnState = SeamlessInSensorState::InSensorHDR3ExpStop;
            }
            else
            {
                returnState = SeamlessInSensorState::InSensorHDR3ExpEnabled;
            }
        }
        else if (((requestId + 1) == numRequests) &&
                    (SensorPipelineDelay == stageSequenceId))
        {
            returnState = SeamlessInSensorState::InSensorHDR3ExpStop;
        }
        else
        {
            returnState = SeamlessInSensorState::InSensorHDR3ExpEnabled;
        }

        return returnState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnInitialize
    ///
    /// @brief  Function creates Sessions and Pipelines based on input descriptor.
    ///         Derived features can override this to create sessions and pipelines or for virtual camx impl.
    ///
    /// @param  pRequestObject  Feature create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult OnInitialize(
        ChiFeature2CreateInputInfo* pRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireResource
    ///
    /// @brief  Acquire semaphore resource before submitting a request
    ///
    /// @param  pRequestObject  Feature request object instance.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID AcquireResource(
        ChiFeature2RequestObject*  pRequestObject
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseResource
    ///
    /// @brief  Release semaphore resource before submitting a request
    ///
    /// @param  pRequestObject  Feature request object instance.
    /// @param  pPortId         port Id.
    /// @param  resultId        FRO batch requestId
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseResource(
        ChiFeature2RequestObject*  pRequestObject,
        ChiFeature2Identifier*     pPortId,
        UINT8                      resultId
        ) const;

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
    CDKResult OnSubmitRequestToSession(
        ChiPipelineRequest* pPipelineRequest) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AreRequestsCompatible
    ///
    /// @brief  Check whether two pipeline requests can be combined into one single request for submission to CamX
    ///
    /// @param  numRequests         Number of requests
    /// @param  ppPipelineRequests  Array of pipeline requests
    ///
    /// @return TRUE if the requests can be combined FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AreRequestsCompatible(
        UINT8                numRequests,
        ChiPipelineRequest** ppPipelineRequests) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MergePipelineRequests
    ///
    /// @brief  Merge two pipeline requests into one request
    ///
    /// @param  numRequests         Number of requests
    /// @param  ppPipelineRequests  Array of pipeline requests
    ///
    /// @return CDKResultSuccess if successful FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult MergePipelineRequests(
        UINT8                numRequests,
        ChiPipelineRequest** ppPipelineRequests) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleDebugDataCopy
    ///
    /// @brief  Handle debug-data deep copy
    ///
    /// @param  pRequestObject          Feature request object instance.
    /// @param  resultId                FRO batch requestId
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleDebugDataCopy(
        ChiFeature2RequestObject*  pRequestObject,
        UINT8                      resultId) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SubmitRequestHandler
    ///
    /// @brief  Static function to handle the request job submission
    ///         The thread callback function which is registered with the thread service.
    ///
    /// @param  pCallbackData   Callback data which will contain the Pipeline Request
    ///
    /// @return NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* SubmitRequestHandler(
        VOID* pCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsInSensorHDR3ExpSnapshot
    ///
    /// @brief  Check if it is in-sensor HDR 3 exposure snapshot or not
    ///
    /// @param  pHint                           Pointer for the feature hint
    /// @param  isPortEnabledInFinalOutput      Bool value which indicates this port is enabled in final output or not
    ///
    /// @return TRUE if it is in-sensor HDR 3 exposure snapshot or FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsInSensorHDR3ExpSnapshot(
        ChiFeature2Hint*    pHint,
        BOOL                isPortEnabledInFinalOutput
        ) const
    {
        return ((TRUE == isPortEnabledInFinalOutput)    &&
                (NULL != pHint)                         &&
                (TRUE == pHint->captureMode.u.InSensorHDR3Exp));
    }

    ChiFeature2RealTime(const ChiFeature2RealTime&)             = delete;   ///< Disallow the copy constructor
    ChiFeature2RealTime& operator= (const ChiFeature2RealTime&) = delete;   ///< Disallow assignment operator

    BOOL                m_isVideoStreamEnabled;   ///< TRUE is video streamis configured from framework
    Semaphore*          m_pHALRequestSem;         ///< Semaphore for max hal requests
    JobHandle           m_hSubmissionJob;         ///< Submission thread job handle
};

#endif // CHIFEATURE2REALTIME_H
