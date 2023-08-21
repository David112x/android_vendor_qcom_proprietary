////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2realtime.cpp
/// @brief Realtime feature2 implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2realtime.h"
#include "chifeature2base.h"
#include "chifeature2utils.h"
#include "chifeature2featurepool.h"

// NOWHINE FILE CP006: Need vector to pass filtered port information
// NOWHINE FILE GR027: Need whiner update

static const UINT32 Feature2MajorVersion = 0;
static const UINT32 Feature2MinorVersion = 0;
static const CHAR*  VendorName           = "QTI";

static const CHAR*  Feature2RTCaps[]     =
{
    "RealTime",
    "RealTimeNZSL",
    "RealTimeWithSWRemosaic",
};

/// @brief  Realtime stage Id
static const UINT32   STAGE_REALTIME                 = 0;
static const UINT32   STAGE_SWREMOSAIC               = 1;
/// @brief  Realtime additional RDi Buffer Count
static const UINT32   REALTIME_CONSUMER_BUFFER_COUNT = 5;
static const UINT32   REALTIME_MAX_BUFFER_COUNT      = 32;

/// @brief  Display port Id
static const ChiFeature2Identifier RealtimeOutputPortDisplay  = RealTimeOutputPortDescriptors[0].globalId;
/// @brief  Raw port Id
static const ChiFeature2Identifier RealtimeOutputPortRaw      = RealTimeOutputPortDescriptors[1].globalId;
/// @brief  Fd port Id
static const ChiFeature2Identifier RealtimeOutputPortFd       = RealTimeOutputPortDescriptors[2].globalId;
/// @brief  Video port Id
static const ChiFeature2Identifier RealtimeOutputPortVideo    = RealTimeOutputPortDescriptors[3].globalId;
/// @brief  Raw callback port Id
static const ChiFeature2Identifier RealtimeOutputPortRawCb    = RealTimeOutputPortDescriptors[4].globalId;
/// @brief  MetaData port Id
static const ChiFeature2Identifier RealtimeOutputPortMetaData = RealTimeOutputPortDescriptors[5].globalId;
/// @brief  ZSL input port Id
static const ChiFeature2Identifier RawZSLInputPortId          = ZSLInputPortDescriptors[0].globalId;
/// @brief  ZSL input metadata port Id
static const ChiFeature2Identifier RawZSLInputMetaDataPortId  = ZSLInputPortDescriptors[1].globalId;
/// @brief  ZSL FD port Id
static const ChiFeature2Identifier RawZSLInputFdPortId        = ZSLInputPortDescriptors[2].globalId;

static const std::pair <ChiFeature2Identifier, ChiFeature2Identifier> ZSLInputOutputMap[] =
{
    { RealtimeOutputPortRaw,      RawZSLInputPortId },
    { RealtimeOutputPortMetaData, RawZSLInputMetaDataPortId },
    { RealtimeOutputPortFd,       RawZSLInputFdPortId }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::SubmitRequestHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2RealTime::SubmitRequestHandler(
    VOID* pCallbackData)
{
    RealtimeThreadCallbackData* pRealtimeCb = static_cast<RealtimeThreadCallbackData*>(pCallbackData);
    if (NULL != pRealtimeCb)
    {
        std::vector<VOID*> jobList;

        const ChiFeature2RealTime* pRealtimeInstance = pRealtimeCb->pRealtimeInstance;

        if (NULL != pRealtimeInstance)
        {
            const ChiCaptureRequest*        pCurRequest  = &pRealtimeCb->pRequest->pCaptureRequests[0];
            ChiFeatureCombinedCallbackData* pCurCb       = static_cast<ChiFeatureCombinedCallbackData*>(pCurRequest->pPrivData);
            UINT                            curAppframe  =
                pCurCb->pCombinedCallbackData[0]->pRequestObj->GetUsecaseRequestObject()->GetAppFrameNumber();
            ChiMetadata*                    pCurMetadata = pRealtimeInstance->GetMetadataManager()->GetMetadataFromHandle(
                pCurRequest->pInputMetadata);
            UINT32*                         pEnableLivePreview = NULL;
            if (NULL != pCurMetadata)
            {
                ChxUtils::GetVendorTagValue(pCurMetadata, VendorTag::LivePreview, reinterpret_cast<VOID**>(
                    &pEnableLivePreview));
                if (NULL != pEnableLivePreview && 0 != *pEnableLivePreview)
                {
                    CHIThreadManager* pManager   = pRealtimeInstance->GetThreadManager();
                    JobHandle         hSubmitJob = pRealtimeInstance->m_hSubmissionJob;
                    if (NULL != pManager && 0 != hSubmitJob)
                    {
                        pManager->GetAllPostedJobs(hSubmitJob, jobList);
                    }

                    for (UINT32 i = 0; i < jobList.size(); ++i)
                    {
                        CHIPIPELINEREQUEST*             pPendingRequest =
                            (static_cast<RealtimeThreadCallbackData*>(jobList[i]))->pRequest;
                        ChiFeatureCombinedCallbackData* pPendingCb      = static_cast<ChiFeatureCombinedCallbackData*>(
                            pPendingRequest->pCaptureRequests[0].pPrivData);
                        UINT pendingAppframe =
                            pPendingCb->pCombinedCallbackData[0]->pRequestObj->GetUsecaseRequestObject()->GetAppFrameNumber();
                        if (pendingAppframe != curAppframe)
                        {
                            CHIPIPELINEREQUEST* pRequestList[] = { pRealtimeCb->pRequest , pPendingRequest };
                            if (TRUE == pRealtimeInstance->AreRequestsCompatible(CHX_ARRAY_SIZE(pRequestList), pRequestList))
                            {
                                CHX_LOG_INFO("Combining frames %" PRIu64 " and %" PRIu64,
                                    pRequestList[0]->pCaptureRequests[0].frameNumber,
                                    pRequestList[1]->pCaptureRequests[0].frameNumber);
                                pRealtimeInstance->MergePipelineRequests(CHX_ARRAY_SIZE(pRequestList), pRequestList);
                                pManager->RemoveJob(hSubmitJob, jobList[i]);
                                CHX_FREE(jobList[i]);
                            }
                            break;
                        }
                    }
                }
            }
            CDKResult result = pRealtimeInstance->SendSubmitRequestMessage(pRealtimeCb->pRequest);

            if (CDKResultSuccess != result)
            {
                ChiFeatureSequenceData* pRequestData = static_cast<ChiFeatureSequenceData*>(
                    pCurCb->pCombinedCallbackData[0]->pRequestObj->GetSequencePrivData(
                        ChiFeature2SequenceOrder::Current, pCurCb->pCombinedCallbackData[0]->pRequestObj->GetCurRequestId()));

                CHIMESSAGEDESCRIPTOR messageDescriptor                      = {};
                messageDescriptor.message.errorMessage.errorMessageCode     = MessageCodeRequest;
                messageDescriptor.message.errorMessage.frameworkFrameNum
                    = ((NULL != pRequestData) ? pRequestData->frameNumber : 0);
                messageDescriptor.message.errorMessage.pErrorStream         = NULL;
                messageDescriptor.messageType                               = ChiMessageType::ChiMessageTypeError;
                messageDescriptor.pPrivData                                 = ((NULL != pRequestData) ?
                                                                               &(pRequestData->frameCbData) : NULL);
                if (NULL != pCurCb->pCombinedCallbackData[0])
                {
                    pRealtimeInstance->HandleRequestError(&messageDescriptor, pCurCb->pCombinedCallbackData[0]);
                }
            }

            CHX_FREE(pRealtimeCb);
        }
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RealTime * ChiFeature2RealTime::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2RealTime*    pFeature    = CHX_NEW(ChiFeature2RealTime);
    CDKResult               result      = CDKResultSuccess;

    if (NULL == pFeature)
    {
        CHX_LOG_ERROR("Out of memory: pFeature is NULL");
    }
    else
    {
        result = pFeature->Initialize(pCreateInputInfo);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Feature failed to initialize!!");
            pFeature->Destroy();
            pFeature = NULL;
        }
        else
        {
            UINT maxHalRequests = ExtensionModule::GetInstance()->GetMaxHalRequests();
            if (60 <= ExtensionModule::GetInstance()->GetUsecaseMaxFPS())
            {
                maxHalRequests += 1;
            }

            pFeature->m_pHALRequestSem = Semaphore::Create(maxHalRequests);

        }
    }

    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RealTime::Destroy()
{
    if (NULL != m_pHALRequestSem)
    {
        m_pHALRequestSem->Destroy();
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::~ChiFeature2RealTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RealTime::~ChiFeature2RealTime()
{
    CHIThreadManager* pThreadManager = GetThreadManager();

    if (NULL != pThreadManager)
    {
        pThreadManager->UnregisterJobFamily(SubmitRequestHandler, "SubmitRequestJob", m_hSubmissionJob);
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::GetRegOutDimensions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RealTime::GetRegOutDimensions(
    UINT32  sensorWidth,
    UINT32  sensorHeight,
    UINT32* pRegOutWidth,
    UINT32* pRegOutHeight)
{
    static const CHIDimension supportedDimensions[] = {{1920, 1440}, {1920, 1080}};

    float previewAspectRatio = static_cast<float>(sensorWidth) / sensorHeight;
    float currentAspectRatio = 0.0;

    UINT32 outWidth  = 0;
    UINT32 outHeight = 0;

    for (UINT i = 0; i < CHX_ARRAY_SIZE(supportedDimensions); i++)
    {
        currentAspectRatio = static_cast<float>(supportedDimensions[i].width) / supportedDimensions[i].height;

        CHX_LOG_VERBOSE("Comparing RegOut dimensions %dx%d (AR: %f) with Sensor resolution %dx%d (AR: %f)",
            supportedDimensions[i].width,
            supportedDimensions[i].height,
            currentAspectRatio,
            sensorWidth,
            sensorHeight,
            previewAspectRatio);

        if (ChxUtils::FEqualCoarse(currentAspectRatio, previewAspectRatio))
        {
            outWidth  = supportedDimensions[i].width;
            outHeight = supportedDimensions[i].height;
            break;
        }
    }

    if ((0 == outWidth) || (0 == outHeight))
    {
        outWidth  = supportedDimensions[0].width;
        outHeight = supportedDimensions[0].height;
        CHX_LOG_ERROR("Unsupported RegOut for sensor with resolution %dx%d", sensorWidth, sensorHeight);
    }

    *pRegOutWidth  = outWidth;
    *pRegOutHeight = outHeight;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnInitialize(
    ChiFeature2CreateInputInfo* pRequestObject)
{
    CDKResult result = CDKResultSuccess;

    m_isVideoStreamEnabled = pRequestObject->bFrameworkVideoStream;

    result = ChiFeature2Base::OnInitialize(pRequestObject);

    CHIThreadManager* pThreadManager = GetThreadManager();

    if ((NULL != pThreadManager) && (CDKResultSuccess == result))
    {
        result = pThreadManager->RegisterJobFamily(SubmitRequestHandler, "SubmitRequestJob", &m_hSubmissionJob);
        if ((CDKResultSuccess != result) || (0 == m_hSubmissionJob))
        {
            CHX_LOG_INFO("Realtime feature submission thread create failed");
            result = CDKResultEFailed;
        }
    }
    else
    {
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnSubmitRequestToSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnSubmitRequestToSession(
    ChiPipelineRequest*    pPipelineRequest
    ) const
{
    CHIThreadManager* pThreadManager = GetThreadManager();
    CDKResult result = CDKResultSuccess;

    UINT64 frameCount = pPipelineRequest->pCaptureRequests[0].frameNumber;

    RealtimeThreadCallbackData* pRealtimeCb = CHX_NEW(RealtimeThreadCallbackData);

    if ((NULL != pThreadManager) && (NULL != pRealtimeCb))
    {
        pRealtimeCb->pRequest = pPipelineRequest;
        pRealtimeCb->pRealtimeInstance = this;
        result = pThreadManager->PostJob(m_hSubmissionJob, pRealtimeCb, frameCount);
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnPrepareRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult                       result         = CDKResultSuccess;
    const ChiFeature2InstanceProps* pInstanceprops = GetInstanceProps();

    if ((NULL != pRequestObject) && (NULL != pInstanceprops))
    {
        ChiFeatureRealtimeContext* pContext = static_cast<ChiFeatureRealtimeContext*>
            (ChxUtils::Calloc(sizeof(ChiFeatureRealtimeContext)));
        SetFeaturePrivContext(pRequestObject, pContext);

        if ((TRUE == IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortRaw, pRequestObject->GetCurRequestId())) &&
            (FALSE == pInstanceprops->instanceFlags.isNZSLSnapshot))
        {
            ChiFeature2Identifier ports [] = { RealtimeOutputPortRaw, RealtimeOutputPortMetaData, RealtimeOutputPortFd};
            ChiFeature2PortIdList portList = {0};
            portList.numPorts = CHX_ARRAY_SIZE(ports);
            portList.pPorts   = &ports[0];

            PrepareZSLQueue(pRequestObject, &portList, &RealtimeOutputPortRaw, &RealtimeOutputPortMetaData);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnInflightBufferCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnInflightBufferCallback(
    ChiFeature2RequestObject*        pRequestObject,
    ChiFeature2Identifier*           pPortId,
    CHITARGETBUFFERINFOHANDLE        hBuffer,
    UINT64                           key
    ) const
{
    CDKResult result = CDKResultSuccess;

    for (UINT8 i = 0; i < pRequestObject->GetNumRequests(); i++)
    {
        ChiFeature2BufferMetadataInfo* pBufferMetaInfo = NULL;

        // CamX will return the buffers for each Port in order so we will send this buffer to the first requestId pending
        // output for this port
        if (FALSE == pRequestObject->GetOutputGeneratedForPort(*pPortId, i))
        {
            CHX_LOG_INFO("Pushing inflight buffer to requestId %d", i);

            ChiFeature2Identifier srcPort  = *pPortId;
            ChiFeature2Identifier sinkPort = { 0 };

            for (UINT32 mapIndex = 0; mapIndex < CHX_ARRAY_SIZE(ZSLInputOutputMap); ++ mapIndex)
            {
                if (srcPort == ZSLInputOutputMap[mapIndex].first)
                {
                    sinkPort = ZSLInputOutputMap[mapIndex].second;
                }
            }
            pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                &sinkPort, hBuffer, key, FALSE, i, 0);

            ChiFeature2PortIdList inflightList = { 1, &sinkPort};
            ProcessZSLRequest(pRequestObject, &inflightList, i);
            break;
        }
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnExecuteProcessRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result            = CDKResultSuccess;
    UINT8     stageId           = InvalidStageId;
    UINT8     stageSequenceId   = InvalidStageSequenceId;

    const ChiFeature2StageDescriptor*   pStageDescriptor    = NULL;
    ChiFeature2StageInfo                stageInfo           = { 0 };
    ChiFeature2PortIdList               outputList          = { 0 };
    ChiMetadata*                        pInputMetadata      = NULL;
    BOOL                                isZSLEnabled        = FALSE;
    UINT                                maxSequence         = 1;
    ChiFeature2Hint*                    pHint               = NULL;
    ChiFeatureRealtimeContext*          pPrivContext        = NULL;
    UINT8                               requestId           = pRequestObject->GetCurRequestId();
    BOOL                                isRawEnabled        = FALSE;
    BOOL                                isRawCbEnabled      = FALSE;

    pHint =  pRequestObject->GetFeatureHint();

    if (TRUE == IsInSensorHDR3ExpSnapshot(pHint,
                                          IsPortEnabledInFinalOutput(pRequestObject,
                                                                     RealtimeOutputPortRaw,
                                                                     requestId)))
    {
        if (0 == requestId)
        {
            maxSequence += SensorPipelineDelay;
        }
        if ((requestId + 1) == pRequestObject->GetNumRequests())
        {
            maxSequence += SensorPipelineDelay;
        }

        pRequestObject->GetUsecaseRequestObject()->SetSkipPreviewFlag(TRUE);
    }
    else if (TRUE == IsSWRemosaicSnapshot())
    {
        // add one more sequence for sw remosaic snapshot
        maxSequence += 1;
        CHX_LOG_VERBOSE("maxSequence:%d", maxSequence);
    }

    result = GetCurrentStageInfo(pRequestObject, &stageInfo);

    if (CDKResultSuccess == result)
    {
        pPrivContext = static_cast<ChiFeatureRealtimeContext*>(
            GetFeaturePrivContext(pRequestObject));
        if (NULL != pPrivContext)
        {
            pPrivContext->maxSequence = maxSequence;
        }

        stageId = stageInfo.stageId;
        stageSequenceId = stageInfo.stageSequenceId;
        switch (stageId)
        {
            case InvalidStageId:

                // Setting this tells the request object how many overall sequences we will run
                SetConfigInfo(pRequestObject, maxSequence);

                pStageDescriptor = GetStageDescriptor(0);
                if (NULL != pStageDescriptor)
                {
                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, 1);

                    ProcessRequestMetadata(pRequestObject);

                    isRawEnabled   = IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortRaw,
                        pRequestObject->GetCurRequestId());
                    isRawCbEnabled = IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortRawCb,
                        pRequestObject->GetCurRequestId());

                    // If raw callback is enabled, then we support only live frame.
                    // The framework provided buffer is imported for RDI target.
                    if ((TRUE == isRawEnabled) && (FALSE == isRawCbEnabled))
                    {
                        if (FALSE == IsManualCaptureRequired(pRequestObject))
                        {
                            const ChiFeature2InputDependency* pInputDependency = NULL;
                            if (TRUE == IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortFd,
                                pRequestObject->GetCurRequestId()))
                            {
                                pInputDependency = GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);
                            }
                            else
                            {
                                pInputDependency = GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 1);
                            }
                            if (NULL != pInputDependency)
                            {
                                PopulateDependencyPorts(pRequestObject, 0, pInputDependency);
                            }
                        }
                    }
                }
                break;

            case STAGE_REALTIME:
                if (FALSE == IsSWRemosaicSnapshot())
                {
                    if (0 == stageSequenceId)
                    {
                        isRawEnabled   = IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortRaw,
                            pRequestObject->GetCurRequestId());
                        isRawCbEnabled = IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortRawCb,
                            pRequestObject->GetCurRequestId());

                        if ((TRUE == isRawEnabled) && (FALSE == isRawCbEnabled) &&
                             FALSE == IsManualCaptureRequired(pRequestObject))
                        {
                            ChiFeature2Identifier ZSLPorts[] = { RawZSLInputPortId, RawZSLInputMetaDataPortId,
                            RawZSLInputFdPortId};
                            ChiFeature2PortIdList ZSLList    = { CHX_ARRAY_SIZE(ZSLPorts), &ZSLPorts[0] };
                            result = ProcessZSLRequest(pRequestObject, &ZSLList, pRequestObject->GetCurRequestId());
                        }
                        else if ((TRUE == isRawEnabled) && (FALSE == isRawCbEnabled) &&
                            (TRUE == IsManualCaptureRequired(pRequestObject)))

                        {
                            // Send picked ZSL frame as messageii
                            ChiFeature2ZSLData  ZSLData;
                            ChiFeature2MessageDescriptor ZSLFrameMsg;
                            ChxUtils::Memset(&ZSLFrameMsg, 0, sizeof(ZSLFrameMsg));

                            ZSLFrameMsg.messageType                          = ChiFeature2MessageType::MessageNotification;
                            ZSLFrameMsg.message.notificationData.messageType = ChiFeature2ChiMessageType::ZSL;
                            ChiFeatureSequenceData*             pRequestData = static_cast<ChiFeatureSequenceData*>(
                                pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current,
                                    pRequestObject->GetCurRequestId()));
                            ZSLData.lastFrameNumber = ((NULL != pRequestData) ? pRequestData->frameNumber : 0);
                            ZSLFrameMsg.message.notificationData.message.ZSLData    = ZSLData;
                            ZSLFrameMsg.pPrivData                                   = pRequestObject;
                            ProcessFeatureMessage(&ZSLFrameMsg);

                        }

                    }

                    result = GetOutputPortsForStage(0, &outputList);
                    if (CDKResultSuccess == result)
                    {
                        std::vector<ChiFeature2Identifier> filteredList = SelectOutputPorts(pRequestObject,
                                                                                            &outputList,
                                                                                            stageSequenceId);

                        // If list is non-zero, we need a metadata port to submit to camx
                        if (filteredList.size() > 0)
                        {
                            if (TRUE == IsBPSCameraPipeline())
                            {
                                AddOutputPortsForBPSCamera(filteredList);
                            }

                            filteredList.push_back(RealtimeOutputPortMetaData);
                        }

                        ChiFeature2PortIdList filtered = {0};
                        filtered.numPorts = filteredList.size();
                        filtered.pPorts   = filteredList.data();

                        ChiFeatureSequenceData* pRequestData = static_cast<ChiFeatureSequenceData*>(
                            GetSequencePrivContext(pRequestObject));
                        if (NULL != pRequestData)
                        {
                            if (TRUE == IsInternalSequence(pRequestObject, stageSequenceId))
                            {
                                pRequestData->skipSequence = TRUE;
                            }
                        }

                        if ((NULL != pPrivContext) && TRUE == pPrivContext->isInSensorHDR3ExpCapture)
                        {
                            pPrivContext->seamlessInSensorState = GetInSensorHDR3ExpState(pRequestObject, stageSequenceId);
                        }

                        if (0 != filtered.numPorts)
                        {
                            PopulatePortConfiguration(pRequestObject,
                                NULL,
                                &filtered);
                        }

                        // Acquire semaphore before submitting request
                        if ((NULL != pRequestData) && (FALSE == pRequestData->skipSequence))
                        {
                            AcquireResource(pRequestObject);
                        }

                        if (0 != filtered.numPorts)
                        {
                            result = SubmitRequestToSession(pRequestObject);
                        }

                        if (CDKResultSuccess == result)
                        {
                            if ((stageSequenceId + 1) < maxSequence)
                            {
                                pStageDescriptor = GetStageDescriptor(0);
                                if (NULL != pStageDescriptor)
                                {
                                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor,
                                        stageSequenceId + 1, 1);
                                }
                            }
                        }
                        else
                        {
                            CHX_LOG_ERROR("Failed to submit request %d to session %d", requestId, result);
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("Unable to get output ports for stage %d", stageId);
                    }
                }
                else
                {
                    result = ProcessSWRemosaicSnapshot(stageId, pRequestObject);
                }

                break;

            case STAGE_SWREMOSAIC:
                if (TRUE == IsSWRemosaicSnapshot())
                {
                    result = ProcessSWRemosaicSnapshot(stageId, pRequestObject);
                }
                else
                {
                    CHX_LOG_ERROR("Stage only valid for qcfa swremosaic sensor!");
                    result = CDKResultEInvalidState;
                }

                break;

            default:
                CHX_LOG_ERROR("Unhandled stage id %d", stageId);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::SendResultForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RealTime::SendResultForPort(
    ChiFeature2RequestObject* pFeatureReqObj,
    ChiFeature2Identifier     fromIdentifier,
    ChiFeature2Identifier     toIdentifier,
    UINT8                     requestId
    ) const
{
    CDKResult                      result                  = CDKResultSuccess;
    ChiFeature2BufferMetadataInfo* pFromBufferMetadataInfo = NULL;
    ChiFeature2BufferMetadataInfo* pToBufferMetadataInfo   = NULL;

    pFeatureReqObj->GetFinalBufferMetadataInfo(fromIdentifier, &pFromBufferMetadataInfo, requestId);

    pFeatureReqObj->GetFinalBufferMetadataInfo(toIdentifier, &pToBufferMetadataInfo, requestId);

    if ((NULL != pToBufferMetadataInfo) && (NULL != pFromBufferMetadataInfo))
    {
        pToBufferMetadataInfo->hBuffer = pFromBufferMetadataInfo->hBuffer;
        pToBufferMetadataInfo->key     = pFromBufferMetadataInfo->key;

        ChiFeature2MessageDescriptor featureMessage;
        ChiFeature2Result*           pResult        = NULL;
        ChxUtils::Memset(&featureMessage, 0, sizeof(featureMessage));

        featureMessage.messageType    = ChiFeature2MessageType::ResultNotification;
        featureMessage.pPrivData      = pFeatureReqObj;
        pResult                       = &featureMessage.message.result;

        pResult->numPorts = 1;
        pResult->pPorts   = &toIdentifier;
        ProcessFeatureMessage(&featureMessage);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::AreRequestsCompatible
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RealTime::AreRequestsCompatible(
    UINT8 numRequests,
    ChiPipelineRequest** ppPipelineRequests
    ) const
{
    CDKResult           result            = CDKResultSuccess;
    BOOL                isCompatible      = FALSE;
    ChiPipelineRequest* pPipelineRequest1 = NULL;
    ChiPipelineRequest* pPipelineRequest2 = NULL;

    if (numRequests != 2)
    {
        CHX_LOG_ERROR("Can only have two requests for compatibility check");
    }
    else
    {
        if (NULL != ppPipelineRequests)
        {
            pPipelineRequest1 = ppPipelineRequests[0];
            pPipelineRequest2 = ppPipelineRequests[1];

            if (NULL != pPipelineRequest1 && NULL != pPipelineRequest2)
            {
                if ((pPipelineRequest1->numRequests == 1)  &&
                    (pPipelineRequest2->numRequests == 1)  &&
                    (pPipelineRequest1->pSessionHandle == pPipelineRequest2->pSessionHandle))
                {
                    const ChiCaptureRequest* pCaptureRequest1 = pPipelineRequest1->pCaptureRequests;
                    const ChiCaptureRequest* pCaptureRequest2 = pPipelineRequest2->pCaptureRequests;

                    if (pCaptureRequest1->hPipelineHandle == pCaptureRequest2->hPipelineHandle)
                    {
                        BOOL request1HasPreview = FALSE;
                        BOOL request2HasPreview = FALSE;
                        ChiFeaturePortData* pPortData = GetPortData(&RealtimeOutputPortDisplay);
                        if (NULL != pPortData)
                        {
                            for (UINT32 bufferIndex = 0; bufferIndex < pCaptureRequest1->numOutputs; ++bufferIndex)
                            {
                                CHISTREAM* pStream1 = pCaptureRequest1->pOutputBuffers[bufferIndex].pStream;
                                if (pPortData->pChiStream == pStream1)
                                {
                                    request1HasPreview = TRUE;
                                }
                            }

                            for (UINT buffer2Index = 0; buffer2Index < pCaptureRequest2->numOutputs;
                                ++buffer2Index)
                            {
                                ChiStream* pStream2 = pCaptureRequest2->pOutputBuffers[buffer2Index].pStream;
                                if (pPortData->pChiStream == pStream2)
                                {
                                    request2HasPreview = TRUE;
                                }
                            }

                            if (FALSE == request1HasPreview && TRUE == request2HasPreview)
                            {
                                isCompatible = TRUE;
                            }
                        }

                    }
                }
            }
        }
    }

    return isCompatible;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::MergePipelineRequests
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::MergePipelineRequests(
    UINT8                numRequests,
    ChiPipelineRequest** ppPipelineRequests
    ) const
{
    CDKResult result = CDKResultSuccess;

    ChiPipelineRequest*      pPipelineRequest1       = NULL;
    ChiPipelineRequest*      pPipelineRequest2       = NULL;
    ChiCaptureRequest*       pCaptureRequest1        = NULL;
    ChiCaptureRequest*       pCaptureRequest2        = NULL;
    ChiCaptureRequest*       pCombinedCaptureRequest = NULL;
    CHISTREAMBUFFER*         pOutputBuffers1         = NULL;
    CHISTREAMBUFFER*         pOutputBuffers2         = NULL;
    CHISTREAMBUFFER*         pCombinedBuffers        = NULL;


    ChiFeatureCombinedCallbackData* pCallbackData1   = NULL;
    ChiFeatureCombinedCallbackData* pCallbackData2   = NULL;
    ChiFeatureCombinedCallbackData* pMergedCallbackData = NULL;

    if (numRequests != 2)
    {
        CHX_LOG_ERROR("Can only have two requests for compatibility check");
        result = CDKResultEInvalidArg;
    }
    else
    {
        if (NULL != ppPipelineRequests)
        {
            pPipelineRequest1 = ppPipelineRequests[0];
            pPipelineRequest2 = ppPipelineRequests[1];

            if (NULL != pPipelineRequest1 && NULL != pPipelineRequest2)
            {
                // NOWHINE CP036a: Need to free the object
                pCaptureRequest1 = const_cast<ChiCaptureRequest*>(pPipelineRequest1->pCaptureRequests);
                // NOWHINE CP036a: Need to free the object
                pCaptureRequest2 = const_cast<ChiCaptureRequest*>(pPipelineRequest2->pCaptureRequests);
                pCombinedCaptureRequest                   = static_cast<ChiCaptureRequest*>
                    (CHX_CALLOC(sizeof(ChiCaptureRequest)));

                if ((NULL != pCaptureRequest1) && (NULL != pCaptureRequest2) && (NULL != pCombinedCaptureRequest))
                {
                    pCallbackData1 = static_cast<ChiFeatureCombinedCallbackData*>(pCaptureRequest1->pPrivData);
                    pCallbackData2 = static_cast<ChiFeatureCombinedCallbackData*>(pCaptureRequest2->pPrivData);

                    if ((NULL != pCallbackData1) && (NULL != pCallbackData2))
                    {
                        UINT32 numTotalOutputs = pCaptureRequest1->numOutputs + pCaptureRequest2->numOutputs;
                        UINT32 numMergedOutputs = 0;

                        pOutputBuffers1 = pCaptureRequest1->pOutputBuffers;
                        pOutputBuffers2 = pCaptureRequest2->pOutputBuffers;

                        pCombinedBuffers =
                            static_cast<CHISTREAMBUFFER*>(CHX_CALLOC(sizeof(CHISTREAMBUFFER) * numTotalOutputs));

                        if (NULL != pCombinedBuffers)
                        {
                            // Add all buffers from the first request
                            for (UINT32 bufferIndex = 0; bufferIndex < pCaptureRequest1->numOutputs; ++bufferIndex)
                            {
                                pCombinedBuffers[numMergedOutputs++] = pOutputBuffers1[bufferIndex];
                            }
                            // From second request, only add buffers that are not already present in the first
                            for (UINT32 bufferIndex = 0; bufferIndex < pCaptureRequest2->numOutputs; ++bufferIndex)
                            {
                                BOOL isBufferPresent = FALSE;
                                for (UINT32 buffer2Index = 0; buffer2Index < numMergedOutputs; ++buffer2Index)
                                {
                                    if (pOutputBuffers2[bufferIndex].pStream == pCombinedBuffers[buffer2Index].pStream)
                                    {
                                        isBufferPresent = TRUE;
                                        break;
                                    }
                                }
                                if (FALSE == isBufferPresent)
                                {
                                    pCombinedBuffers[numMergedOutputs++] = pOutputBuffers2[bufferIndex];
                                }
                                else
                                {
                                    // Release the buffer
                                    ChiFeatureFrameCallbackData* pFrameCb = pCallbackData2->pCombinedCallbackData[0];
                                    if (NULL != pFrameCb)
                                    {
                                        for (UINT8 i = 0; i < pFrameCb->pOutputPorts.size(); ++i)
                                        {
                                            ChiFeaturePortData*     pPortData = GetPortData(&pFrameCb->pOutputPorts[i]);
                                            if (NULL != pPortData &&
                                                pPortData->pChiStream == pOutputBuffers2[bufferIndex].pStream)
                                            {
                                                CHITargetBufferManager* pManager = pPortData->pOutputBufferTbm;
                                                CHX_LOG_INFO("Release combined buffer from port %s %" PRIu64,
                                                    pPortData->pPortName,
                                                    pCaptureRequest2->frameNumber);
                                                pManager->UpdateTarget(pCaptureRequest2->frameNumber,
                                                    reinterpret_cast<UINT64>(pPortData->pChiStream), NULL,
                                                    ChiTargetStatus::Error, NULL);
                                            }

                                        }
                                    }
                                }
                            }

                            *pCombinedCaptureRequest = *pCaptureRequest1;

                            pCombinedCaptureRequest->numOutputs = numMergedOutputs;
                            pCombinedCaptureRequest->pOutputBuffers = pCombinedBuffers;
                            pPipelineRequest1->pCaptureRequests = pCombinedCaptureRequest;

                            // Merge callback data
                            pCallbackData1->numCallbackData = MaxCombinedRequests;
                            pCallbackData1->pCombinedCallbackData[1] = pCallbackData2->pCombinedCallbackData[0];
                            pCombinedCaptureRequest->pPrivData = pCallbackData1;

                            // Release metadata and unused buffers for second request
                            ChiFeaturePipelineData* pPipelineData = GetPipelineData(&RealtimeOutputPortMetaData);
                            if (NULL != pPipelineData)
                            {
                                CHITargetBufferManager* pSettingManager = pPipelineData->pSettingMetaTbm;
                                CHITargetBufferManager* pOutputMetadataManager = pPipelineData->pOutputMetaTbm;

                                if (NULL != pSettingManager && NULL != pOutputMetadataManager)
                                {
                                    ChiMetadata*  pOutputMeta = GetMetadataManager()->GetMetadataFromHandle(
                                        pCaptureRequest2->pOutputMetadata);
                                    ChiMetadata*  pInputMetadata = GetMetadataManager()->GetMetadataFromHandle(
                                        pCaptureRequest2->pInputMetadata);
                                    if (NULL != pOutputMeta && NULL != pInputMetadata)
                                    {
                                        pSettingManager->UpdateTarget(pCaptureRequest2->frameNumber,
                                            pPipelineData->metadataClientId, NULL, ChiTargetStatus::Error, NULL);
                                        pOutputMetadataManager->UpdateTarget(pCaptureRequest2->frameNumber,
                                            pPipelineData->metadataClientId, NULL, ChiTargetStatus::Error, NULL);
                                    }
                                }
                            }
                            CHX_FREE(pOutputBuffers1);
                            CHX_FREE(pOutputBuffers2);
                            CHX_FREE(pCaptureRequest1);
                            CHX_FREE(pCaptureRequest2);
                        }

                    }
                }

            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::SelectOutputPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<ChiFeature2Identifier> ChiFeature2RealTime::SelectOutputPorts(
    ChiFeature2RequestObject* pRequestObject,
    ChiFeature2PortIdList*    pAllOutputPorts,
    UINT8                     stageSequenceId
    ) const
{
    std::vector<ChiFeature2Identifier> selectedList;
    UINT8              dummyRequest  = FALSE;
    BOOL               addZSLBuffers = FALSE;

    if (TRUE == IsInternalSequence(pRequestObject, stageSequenceId) &&
        (TRUE == IsPortEnabledInFinalOutput(pRequestObject,
        RealtimeOutputPortRaw, pRequestObject->GetCurRequestId())))
    {
        CHX_LOG_INFO("Dummy Request for seamless in-sensor cureReq = %d Total Req = %d stageSequenceId = %d",
            pRequestObject->GetCurRequestId(), pRequestObject->GetNumRequests(), stageSequenceId);
        dummyRequest = TRUE;
    }

    if ((TRUE == (IsPortEnabledInFinalOutput(pRequestObject,
        RealtimeOutputPortDisplay,
        pRequestObject->GetCurRequestId()))))
    {
        addZSLBuffers = TRUE;
    }

    selectedList.reserve(pAllOutputPorts->numPorts);

    for (UINT8 portIndex = 0; portIndex < pAllOutputPorts->numPorts; ++portIndex)
    {
        ChiFeature2Identifier portId = pAllOutputPorts->pPorts[portIndex];

        if (TRUE == dummyRequest)
        {
            if (RealtimeOutputPortRaw == portId)
            {
                selectedList.push_back(portId);
            }
            else if (RealtimeOutputPortFd == portId)
            {
                selectedList.push_back(portId);
            }
        }
        else
        {
            if ((RealtimeOutputPortRawCb == portId) || (RealtimeOutputPortMetaData == portId))
            {
                continue;
            }

            if (RealtimeOutputPortRaw == portId)
            {
                if ((IsPortEnabledInFinalOutput(pRequestObject,
                                                RealtimeOutputPortRawCb,
                                                pRequestObject->GetCurRequestId())))
                {
                    selectedList.push_back(RealtimeOutputPortRawCb);
                }
                else
                {
                    if (((TRUE == (IsPortEnabledInFinalOutput(pRequestObject,
                            RealtimeOutputPortRaw, pRequestObject->GetCurRequestId()))) &&
                        (TRUE == IsManualCaptureRequired(pRequestObject))) ||
                        (TRUE == addZSLBuffers))
                    {
                        selectedList.push_back(RealtimeOutputPortRaw);
                    }
                }
            }
            else if (RealtimeOutputPortFd == portId)
            {
                if (((TRUE == (IsPortEnabledInFinalOutput(pRequestObject,
                        RealtimeOutputPortFd, pRequestObject->GetCurRequestId()))) &&
                    (TRUE == IsManualCaptureRequired(pRequestObject))) ||
                    (TRUE == addZSLBuffers))
                {
                    selectedList.push_back(RealtimeOutputPortFd);
                }
            }
            else
            {
                if (TRUE == IsPortEnabledInFinalOutput(
                    pRequestObject, portId, pRequestObject->GetCurRequestId()))
                {
                    selectedList.push_back(portId);
                }
            }
        }
    }
    return selectedList;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::AddOutputPortsForBPSCamera
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RealTime::AddOutputPortsForBPSCamera(
    std::vector<ChiFeature2Identifier>& rOuputPorts
    ) const
{
    BOOL isRealtimeOutputPortRawAdded     = FALSE;
    BOOL isRealtimeOutputPortFdAdded      = FALSE;
    BOOL isRealtimeOutputPortDisplayAdded = FALSE;

    for (const ChiFeature2Identifier& port : rOuputPorts)
    {
        if (RealtimeOutputPortDisplay == port)
        {
            isRealtimeOutputPortDisplayAdded = TRUE;
        }
        else if (RealtimeOutputPortFd == port)
        {
            isRealtimeOutputPortFdAdded = TRUE;
        }
        else if (RealtimeOutputPortRaw == port)
        {
            isRealtimeOutputPortRawAdded = TRUE;
        }
    }

    // BPS camera usecase always needs RAW buffers for its Realtime stage.
    // The same goes for FD and Display output ports

    if (FALSE == isRealtimeOutputPortRawAdded)
    {
        rOuputPorts.push_back(RealtimeOutputPortRaw);
    }
    if (FALSE == isRealtimeOutputPortFdAdded)
    {
        rOuputPorts.push_back(RealtimeOutputPortFd);
    }
    if (FALSE == isRealtimeOutputPortDisplayAdded)
    {
        rOuputPorts.push_back(RealtimeOutputPortDisplay);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnMetadataResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RealTime::OnMetadataResult(
    ChiFeature2RequestObject*  pRequestObject,
    UINT8                      resultId,
    ChiFeature2StageInfo*      pStageInfo,
    ChiFeature2Identifier*     pPortIdentifier,
    ChiMetadata*               pMetadata,
    UINT32                     frameNumber,
    VOID*                      pPrivateData)
{
    BOOL                sendToGraph         = FALSE;
    ExtensionModule*    pExtensionModule    = ExtensionModule::GetInstance();

    if (NULL != pMetadata)
    {
        ProcessResultMetadata(pMetadata);
    }

    if (((DumpDebugDataPerVideoFrame <= pExtensionModule->EnableDumpDebugData())        &&
         (IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortVideo, resultId)))   ||
        (DumpDebugDataAllFrames <= pExtensionModule->EnableDumpDebugData()))
    {
        ProcessDebugData(pRequestObject,
                         pStageInfo,
                         pPortIdentifier,
                         pMetadata,
                         frameNumber);
    }

    sendToGraph = ChiFeature2Base::OnMetadataResult(pRequestObject, resultId, pStageInfo,
        pPortIdentifier, pMetadata, frameNumber, pPrivateData);

    return sendToGraph;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::ReleaseResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RealTime::ReleaseResource(
    ChiFeature2RequestObject*  pRequestObject,
    ChiFeature2Identifier*     pPortIdentifier,
    UINT8                      resultId
    ) const
{
    // wait for semaphore for any framework stream
    ChiFeaturePortData* pPortData = GetPortData(pPortIdentifier);

    BOOL previewRequest = (RealtimeOutputPortDisplay == *pPortIdentifier) &&
        (IsPortEnabledInFinalOutput(pRequestObject, *pPortIdentifier, resultId));

    BOOL videoRequest = (RealtimeOutputPortVideo == *pPortIdentifier) &&
        (IsPortEnabledInFinalOutput(pRequestObject, *pPortIdentifier, resultId));

    BOOL previewSkip = (RealtimeOutputPortRaw == *pPortIdentifier) &&
        (CheckBufferSkippedForPort(pRequestObject, &RealtimeOutputPortDisplay, resultId));

    ChiFeature2UsecaseRequestObjectFwkStreamData& rFwkStreamData =
        pRequestObject->GetUsecaseRequestObject()->GetOutputRTSinkStreams();

    if (previewRequest || videoRequest || previewSkip == TRUE)
    {
        rFwkStreamData.numOutputNotified++;
    }

    BOOL releaseSem = TRUE;

    if ((RealtimeOutputPortRaw == *pPortIdentifier) || (RealtimeOutputPortFd == *pPortIdentifier))
    {
        releaseSem = FALSE;
    }
    // In usecases such as Remosaic where preview is skipped we need to release the semaphore during
    // raw callback
    if (TRUE == previewSkip)
    {
        releaseSem = TRUE;
    }

    if ((NULL != pPortData) && (NULL != m_pHALRequestSem))
    {
        CHX_LOG_VERBOSE("numOutputs:%d, notified %d, port %s: %d, release sem %d, previewRequest %d, videoRequest %d",
                      rFwkStreamData.numOutputs, rFwkStreamData.numOutputNotified,
                      pPortData->pPortName, m_pHALRequestSem->GetSemaphoreCount(), releaseSem, previewRequest, videoRequest);

        if ((TRUE == releaseSem) && (0 < rFwkStreamData.numOutputs) &&
            (rFwkStreamData.numOutputNotified == rFwkStreamData.numOutputs))
        {
            m_pHALRequestSem->NotifyAllThreads();
            CHX_LOG_VERBOSE("Notify all threads : semaphore count %d, port %s", m_pHALRequestSem->GetSemaphoreCount(),
                          pPortData->pPortName);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::AcquireResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RealTime::AcquireResource(
    ChiFeature2RequestObject*  pRequestObject
    ) const
{
    ChiFeature2UsecaseRequestObjectFwkStreamData fwkStreamData =
        pRequestObject->GetUsecaseRequestObject()->GetOutputRTSinkStreams();

    UINT8 requestId = pRequestObject->GetCurRequestId();

    BOOL previewRequest = IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortDisplay, requestId);
    BOOL videoRequest   = IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortVideo, requestId);

    BOOL acquireSem = FALSE;
    if ((0 < fwkStreamData.numOutputs) && ((TRUE == previewRequest) || (TRUE == videoRequest)))
    {
        acquireSem = TRUE;
    }

    // wait for semaphore for any framework stream
    if ((TRUE == acquireSem) && (NULL != m_pHALRequestSem))
    {
        m_pHALRequestSem->WaitForSemaphore();

        CHX_LOG_VERBOSE("Acquire resource: semaphore count %d", m_pHALRequestSem->GetSemaphoreCount());

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnBufferError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RealTime::OnBufferError(
    ChiFeature2RequestObject* pRequestObject,
    ChiFeature2Identifier*    pPortIdentifier,
    UINT8                     resultId
    ) const
{
    if ((NULL != pRequestObject) &&
        (NULL != pPortIdentifier))
    {
        ReleaseResource(pRequestObject, pPortIdentifier, resultId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnBufferResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RealTime::OnBufferResult(
    ChiFeature2RequestObject*  pRequestObject,
    UINT8                      resultId,
    ChiFeature2StageInfo*      pStageInfo,
    ChiFeature2Identifier*     pPortIdentifier,
    const CHISTREAMBUFFER*     pStreamBuffer,
    UINT32                     frameNumber,
    VOID*                      pPrivateData)
{
    BOOL sendToGraph = ChiFeature2Base::OnBufferResult(pRequestObject, resultId, pStageInfo, pPortIdentifier,
        pStreamBuffer, frameNumber, pPrivateData);

    if  ((NULL != pRequestObject) &&
         (NULL != pPortIdentifier) &&
         (IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortRawCb, pRequestObject->GetCurRequestId())) &&
         (RealtimeOutputPortRawCb == *pPortIdentifier))
    {
        CHX_LOG_INFO("Received buffer callback on port %d %d %d stream %p", pPortIdentifier->session,
            pPortIdentifier->pipeline, pPortIdentifier->port, pStreamBuffer->pStream);
        SendResultForPort(pRequestObject, RealtimeOutputPortRawCb, RealtimeOutputPortRaw, pRequestObject->GetCurRequestId());
    }

    if ((TRUE == ExtensionModule::GetInstance()->Enable3ADebugData()) ||
        (TRUE == ExtensionModule::GetInstance()->EnableTuningMetadata()))
    {
        if ((NULL                   != pPortIdentifier)  &&
            (RealtimeOutputPortRaw  == *pPortIdentifier) &&
            (NULL                   != pRequestObject)   &&
            (FALSE                  == pRequestObject->GetOutputGeneratedForPort(*pPortIdentifier, resultId)))
        {
            HandleDebugDataCopy(pRequestObject, resultId);
        }
    }

    if ((NULL != pRequestObject) &&
        (NULL != pPortIdentifier))
    {
        ReleaseResource(pRequestObject, pPortIdentifier, resultId);
    }

    // If exist multi requests for RequestObj of realtime, set requests to OutputNotificationPending status expect resultId.
    // The reqeust for resultId will be set to OutputNotificationPending in ProcessBufferCallback.
    if (NULL != pRequestObject)
    {
        for (UINT8 reqId = 0; reqId < pRequestObject->GetNumRequests(); ++reqId)
        {
            if (resultId != reqId)
            {
                pRequestObject->SetCurRequestState(ChiFeature2RequestState::OutputNotificationPending, reqId);
            }
        }
    }

    return sendToGraph;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pRequestObject)
    {
        ChiFeatureRealtimeContext* pContext =
            static_cast<ChiFeatureRealtimeContext*>(GetFeaturePrivContext(pRequestObject));

        if (NULL != pContext)
        {
            ChxUtils::Free(pContext);
            pContext = NULL;
            SetFeaturePrivContext(pRequestObject, NULL);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnInitializeStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnInitializeStream(
    const ChiTargetPortDescriptor*    pTargetDesc,
    const ChiFeature2PortDescriptor*  pPortDesc,
    ChiFeatureStreamData*             pOutputStreamData)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL != pPortDesc) &&
        (NULL != pTargetDesc) &&
        (NULL != pOutputStreamData) &&
        (NULL != pOutputStreamData->pStream))
    {
        if (0 == CdkUtils::StrCmp("TARGET_BUFFER_FD", pTargetDesc->pTargetName))
        {
            ChiStream fdStream = UsecaseSelector::GetFDOutStream();

            pOutputStreamData->pTargetStream = CloneStream(&fdStream);
            if (NULL != pOutputStreamData->pTargetStream)
            {
                if (TRUE == IsBPSCameraPipeline())
                {
                    CHIDIMENSION sensorDimensions = { 0 };

                    for (CHISTREAM* pStream : m_pStreams)
                    {
                        // Locate the RAW RDI Stream.
                        if (ChiStreamFormatRaw10 == pStream->format ||
                            ChiStreamFormatRaw16 == pStream->format)
                        {
                            sensorDimensions.width  = pStream->width;
                            sensorDimensions.height = pStream->height;
                            break;
                        }
                    }

                    // Compute optimal registration output dimensions.
                    GetRegOutDimensions(
                        sensorDimensions.width,
                        sensorDimensions.height,
                        &pOutputStreamData->pTargetStream->width,
                        &pOutputStreamData->pTargetStream->height);

                    // Set Video Encoder gralloc flag to apply correct stride for GPU processing of
                    // the FD buffer. FD path from BPS pipeline is BPS (RegOut) -> GPU -> FD
                    pOutputStreamData->pTargetStream->grallocUsage |= GRALLOC_USAGE_HW_VIDEO_ENCODER;
                }

                *(pOutputStreamData->pStream) = *(pOutputStreamData->pTargetStream);
            }
            else
            {
                CHX_LOG_ERROR("Unable to clone FDStream. pOutputStreamData->pTargetStream is NULL.");
                result = CDKResultEInvalidArg;
            }
        }
        else
        {
            result = ChiFeature2Base::OnInitializeStream(pTargetDesc, pPortDesc, pOutputStreamData);
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid argument: NULL pTargetDesc = %p pPortDesc = %p pOutputStream = %p pStream %p",
            pTargetDesc, pPortDesc, pOutputStreamData,
            pOutputStreamData ? pOutputStreamData->pStream : NULL);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::DoFlush()
{
    m_pHALRequestSem->Reset();

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::ProcessZSLRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::ProcessZSLRequest(
    ChiFeature2RequestObject * pRequestObject,
    ChiFeature2PortIdList*     pPortIdList,
    UINT8                      requestId
    ) const
{

    CDKResult                            result             = CDKResultSuccess;

    std::vector<ChiFeature2Identifier> selectedList;

    for (UINT8 i = 0; i < pPortIdList->numPorts; i++)
    {
        // Identify sink port
        ChiFeature2BufferMetadataInfo*       pPortBufferInfo    = NULL;
        ChiFeature2Identifier sinkPort = pPortIdList->pPorts[i];
        ChiFeature2Identifier srcPort  = { 0 };

        for (UINT32 mapIndex = 0; mapIndex < CHX_ARRAY_SIZE(ZSLInputOutputMap); ++ mapIndex)
        {
            if (sinkPort == ZSLInputOutputMap[mapIndex].second)
            {
                srcPort = ZSLInputOutputMap[mapIndex].first;
            }
        }

        pRequestObject->GetFinalBufferMetadataInfo(srcPort, &pPortBufferInfo, requestId);

        if (NULL != pPortBufferInfo)
        {
            result = GetFeatureRequestOpsData(pRequestObject, ChiFeature2RequestObjectOpsType::InputDependency,
                    &sinkPort, ChiFeature2HandleType::BufferMetaInfo, pPortBufferInfo, requestId);

            if (CDKResultSuccess == result)
            {
                if (RealtimeOutputPortMetaData == srcPort)
                {
                    if (0 == requestId)
                    {
                        // Send partial result for ZSL frame
                        ChiFeature2MessageDescriptor partialMetadataResult;

                        partialMetadataResult.messageType                =
                            ChiFeature2MessageType::PartialMetadataNotification;
                        partialMetadataResult.message.result.resultIndex = requestId;
                        partialMetadataResult.message.result.numPorts    = 1;
                        partialMetadataResult.message.result.pPorts      = &RealtimeOutputPortMetaData;
                        partialMetadataResult.pPrivData                  = pRequestObject;
                        ProcessFeatureMessage(&partialMetadataResult);
                    }

                    ChiMetadata*     pZSLMetadata     = NULL;
                    ChiMetadata*     pAppMetadata     = NULL;
                    CHIMETAHANDLE    hMetadataBuffer  = NULL;

                    result = GetMetadataBuffer(pPortBufferInfo->hBuffer,
                        pPortBufferInfo->key, &hMetadataBuffer);

                    if (NULL != hMetadataBuffer)
                    {
                        pZSLMetadata = GetMetadataManager()->GetMetadataFromHandle(hMetadataBuffer);
                    }

                    if (NULL != pZSLMetadata)
                    {
                        MergeAppSettings(pRequestObject, &pZSLMetadata, TRUE);
                        pRequestObject->GetAppRequestSetting(&pAppMetadata);

                        if (NULL != pAppMetadata)
                        {
                            ChxUtils::UpdateMetadataWithInputSettings(*pAppMetadata, *pZSLMetadata);
                        }
                        ChxUtils::UpdateMetadataWithSnapshotSettings(*pZSLMetadata);

                        if ((TRUE == ExtensionModule::GetInstance()->Enable3ADebugData()) ||
                            (TRUE == ExtensionModule::GetInstance()->EnableTuningMetadata()))
                        {
                            HandleDebugDataCopy(pRequestObject, requestId);
                        }
                    }
                }
                selectedList.push_back(srcPort);
                pRequestObject->SetOutputGeneratedForPort(srcPort, requestId);
            }
            else
            {
                // Expected when inflight callback is registered
                CHX_LOG_WARN("Dependency not met yet for port %d %d %d src %d %d %d", sinkPort.session,
                    sinkPort.pipeline, sinkPort.port, srcPort.session, srcPort.pipeline, srcPort.port);
            }
        }
    }

    ChiFeature2MessageDescriptor featureMessage;
    ChiFeature2Result*           pResult        = NULL;
    // Added the selectedList Size incase for few port callback is registered
    if (CDKResultSuccess == result || 0 < selectedList.size())
    {
        ChxUtils::Memset(&featureMessage, 0, sizeof(featureMessage));
        featureMessage.messageType    = ChiFeature2MessageType::ResultNotification;
        featureMessage.pPrivData      = pRequestObject;
        pResult                       = &featureMessage.message.result;
        pResult->resultIndex          = requestId;

        pResult->numPorts  = selectedList.size();
        pResult->pPorts    = selectedList.data();

        ProcessFeatureMessage(&featureMessage);
    }

    if (CDKResultSuccess == result)
    {
        if (requestId == (pRequestObject->GetNumRequests() - 1))
        {
            UINT8 reqId         = 0;
            BOOL  bOnlySnapshot = TRUE;

            for (reqId = 0; reqId < pRequestObject->GetNumRequests(); ++reqId)
            {
                if ((TRUE == IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortDisplay, reqId)) ||
                    (TRUE == IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortVideo, reqId)))
                {
                    bOnlySnapshot = FALSE;
                    break;
                }
            }

            if (TRUE == bOnlySnapshot)
            {
                for (reqId = 0; reqId < pRequestObject->GetNumRequests(); ++reqId)
                {
                    pRequestObject->SetCurRequestState(ChiFeature2RequestState::OutputNotificationPending, reqId);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::ProcessSWRemosaicSnapshot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::ProcessSWRemosaicSnapshot(
    UINT8                      stageId,
    ChiFeature2RequestObject*  pRequestObject
    ) const
{
    CDKResult                         result           = CDKResultSuccess;
    const ChiFeature2StageDescriptor* pStageDescriptor = NULL;

    CHX_LOG_INFO("SWRemosaic sansphot stage: %d", stageId);

    if (STAGE_REALTIME == stageId)
    {
        ChiFeature2PortIdList outputList = { 0 };

        result = GetOutputPortsForStage(stageId, &outputList);

        if (CDKResultSuccess == result)
        {
            result = PopulatePortConfiguration(pRequestObject, NULL, &outputList);
        }

        if (CDKResultSuccess == result)
        {
            result = SubmitRequestToSession(pRequestObject);
        }

        if (CDKResultSuccess == result)
        {
            CHX_LOG_VERBOSE("Setting dependency for next stage.");

            pStageDescriptor = GetStageDescriptor(STAGE_SWREMOSAIC);
            if (NULL != pStageDescriptor)
            {
                SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, 1);
                PopulateDependency(pRequestObject);

                CHX_LOG_VERBOSE("Wait dependency for stage:%s, numSessions:%d, session[0]:%d",
                    pStageDescriptor->pStageName, pStageDescriptor->numDependencyConfigDescriptor,
                    pStageDescriptor->pDependencyConfigDescriptor[0].sessionId);
            }
        }
    }
    else if (STAGE_SWREMOSAIC == stageId)
    {
        ChiFeature2PortIdList outputList  = { 0 };
        ChiFeature2PortIdList inputList   = { 0 };

        result = GetOutputPortsForStage(STAGE_SWREMOSAIC, &outputList);

        if (CDKResultSuccess == result)
        {
            GetInputPortsForStage(STAGE_SWREMOSAIC, &inputList);
        }

        if (CDKResultSuccess == result)
        {
            std::vector<ChiFeature2Identifier> filteredList = SelectOutputPorts(pRequestObject, &outputList, 0);

            // If list is non-zero, we need a metadata port to submit to camx
            if (filteredList.size() > 0)
            {
                filteredList.push_back(RealtimeOutputPortMetaData);
            }

            ChiFeature2PortIdList filtered = {0};
            filtered.numPorts = filteredList.size();
            filtered.pPorts   = filteredList.data();

            result = PopulatePortConfiguration(pRequestObject, &inputList, &filtered);
        }

        if (CDKResultSuccess == result)
        {
            result = SubmitRequestToSession(pRequestObject);
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid staged Id: %d", stageId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::ProcessResultMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::ProcessResultMetadata(
    ChiMetadata* pResultMetadata)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pResultMetadata)
    {
        CHX_LOG_ERROR("pResultMetadata is NULL");
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::ProcessRequestMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::ProcessRequestMetadata(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult         result             = CDKResultSuccess;
    UINT8             numRequestOutputs  = 0;
    ChiMetadata*      pInputMetadata     = NULL;
    ChiMetadata*      pFeatureSetting    = NULL;

    if (NULL == pRequestObject)
    {
        CHX_LOG_ERROR("pRequestObject is NULL");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeatureRealtimeContext* pPrivContext = NULL;
        // Get metadata from feature request object and fill metadata
        pRequestObject->GetAppRequestSetting(&pInputMetadata);
        pFeatureSetting = GetExternalInputSettings(pRequestObject, &RealtimeOutputPortMetaData,
            pRequestObject->GetCurRequestId());
        pPrivContext = static_cast<ChiFeatureRealtimeContext*>(GetFeaturePrivContext(pRequestObject));

        if ((NULL != pPrivContext) && (NULL != pInputMetadata))
        {
            BOOL             isRDIRequested = FALSE;
            ChiFeature2Hint* pFeatureHint   = NULL;

            pPrivContext->isManualCaptureNeeded     = FALSE;
            pPrivContext->isInSensorHDR3ExpCapture  = FALSE;
            pPrivContext->seamlessInSensorState     = SeamlessInSensorState::None;

            if (TRUE == IsPortEnabledInFinalOutput(pRequestObject,
                RealtimeOutputPortRaw, pRequestObject->GetCurRequestId()))
            {
                isRDIRequested = TRUE;
            }

            pFeatureHint = pRequestObject->GetFeatureHint();

            if (TRUE == isRDIRequested)
            {
                UINT32 controlZSLValue = ANDROID_CONTROL_ENABLE_ZSL_TRUE;
                if (NULL != pFeatureSetting)
                {
                    INT* pControlZSL = static_cast<INT*>(pFeatureSetting->GetTag(
                        ANDROID_CONTROL_ENABLE_ZSL));
                    if ((NULL != pControlZSL) &&
                        (*pControlZSL == ANDROID_CONTROL_ENABLE_ZSL_FALSE))
                    {
                        controlZSLValue = *pControlZSL;
                    }
                }

                if ((NULL != pInputMetadata) &&
                    (ANDROID_CONTROL_ENABLE_ZSL_TRUE == controlZSLValue))
                {
                    INT* pControlZSL = static_cast<INT*>(pInputMetadata->GetTag(
                        ANDROID_CONTROL_ENABLE_ZSL));
                    if ((NULL != pControlZSL) &&
                        (*pControlZSL == ANDROID_CONTROL_ENABLE_ZSL_FALSE))
                    {
                        controlZSLValue = *pControlZSL;
                    }
                }

                if ((IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortVideo,
                    pRequestObject->GetCurRequestId())))
                {
                    pPrivContext->isManualCaptureNeeded = TRUE;
                }
                else if ((ANDROID_CONTROL_ENABLE_ZSL_FALSE == controlZSLValue) ||
                         (TRUE == CheckZSLQueueEmptyForPort(pRequestObject, &RealtimeOutputPortRaw)))
                {
                    pPrivContext->isManualCaptureNeeded = TRUE;
                }

                if (NULL != pFeatureHint)
                {
                    if (1 == pFeatureHint->captureMode.u.LowLight)
                    {
                        pPrivContext->isManualCaptureNeeded = TRUE;
                    }

                    if (1 == pFeatureHint->captureMode.u.Manual)
                    {
                        pPrivContext->isManualCaptureNeeded = TRUE;
                    }

                    if (1 == pFeatureHint->captureMode.u.InSensorHDR3Exp)
                    {
                        pPrivContext->isInSensorHDR3ExpCapture  = TRUE;
                        pPrivContext->isManualCaptureNeeded     = TRUE;
                    }
                }
                if (NULL != pInputMetadata)
                {
                    INT* pFlashMode = static_cast<INT*>(pInputMetadata->GetTag(
                        ANDROID_FLASH_MODE));
                    INT* pAEMode = static_cast<INT*>(pInputMetadata->GetTag(
                        ANDROID_CONTROL_AE_MODE));
                    if ((NULL != pFlashMode) && (NULL != pAEMode) &&
                        (ANDROID_FLASH_MODE_OFF != *pFlashMode) &&
                        (ANDROID_CONTROL_AE_MODE_ON == *pAEMode))
                    {
                        pPrivContext->isManualCaptureNeeded     = TRUE;
                    }
                }
            }
            else
            {
                pPrivContext->isManualCaptureNeeded     = TRUE;
            }
            CHX_LOG_INFO("IsManualcapture = %d isInSensorHDR3ExpCapture = %d",
                         pPrivContext->isManualCaptureNeeded, pPrivContext->isInSensorHDR3ExpCapture);

            if ((CDKResultSuccess == result) &&
                (FALSE == pPrivContext->isManualCaptureNeeded))
            {
                const ChiFeature2InstanceProps* pInstanceprops = GetInstanceProps();

                // always set manual capture to TRUE in real non-zsl snapshot (switch sensor mode)
                if ((NULL != pInstanceprops) && (TRUE == pInstanceprops->instanceFlags.isNZSLSnapshot))
                {
                    pPrivContext->isManualCaptureNeeded = TRUE;
                }
            }

            SetFeaturePrivContext(pRequestObject, pPrivContext);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::HandleDebugDataCopy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RealTime::HandleDebugDataCopy(
    ChiFeature2RequestObject*  pRequestObject,
    UINT8                      resultId
    ) const
{
    // Debug-Data handling only
    ChiFeature2BufferMetadataInfo*  pRealTimeMetaDataBufferInfo = NULL;
    ChiMetadata*                    pMetadata                   = NULL;
    CHIMETAHANDLE                   hMetadataBuffer             = NULL;

    if ((NULL   != pRequestObject)  &&
        (TRUE   == IsPortEnabledInFinalOutput(pRequestObject, RealtimeOutputPortRaw, pRequestObject->GetCurRequestId())))
    {
        pRequestObject->GetFinalBufferMetadataInfo(RealtimeOutputPortMetaData, &pRealTimeMetaDataBufferInfo,
                                                   resultId);
        if (NULL != pRealTimeMetaDataBufferInfo)
        {
            GetMetadataBuffer(pRealTimeMetaDataBufferInfo->hBuffer,
                              pRealTimeMetaDataBufferInfo->key, &hMetadataBuffer);
            if (NULL != hMetadataBuffer)
            {
                pMetadata = GetMetadataManager()->GetMetadataFromHandle(hMetadataBuffer);
                if (NULL != pMetadata)
                {
                    // Use offline debug-data buffer for offline metadata
                    BOOL doDebugDataCopy = IsMasterCamera(pMetadata, m_pCameraInfo);
                    if (TRUE == doDebugDataCopy)
                    {
                        DebugDataCopy(pRequestObject, pMetadata);
                    }
                }
            }
        }
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::IsManualCaptureRequired
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RealTime::IsManualCaptureRequired(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult result        = CDKResultSuccess;
    BOOL      manualCapture = FALSE;

    if (NULL == pRequestObject)
    {
        CHX_LOG_ERROR("pRequestObject is NULL");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeatureRealtimeContext* pPrivContext = NULL;

        pPrivContext = static_cast<ChiFeatureRealtimeContext *>(GetFeaturePrivContext(pRequestObject));
        if (NULL != pPrivContext)
        {
            if (TRUE == pPrivContext->isManualCaptureNeeded)
            {
                manualCapture = TRUE;
            }
        }

        CHX_LOG_INFO("manualCapture %d for requestID %d ", manualCapture, pRequestObject->GetCurRequestId());
    }

    return manualCapture;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RealTime::OnPipelineSelect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnPipelineSelect(
    const CHAR*                         pPipelineName,
    const ChiFeature2CreateInputInfo*   pCreateInputInfo
    ) const
{
    BOOL                 isPipelineSupported    = FALSE;
    UINT32               videoWidth             = 0;
    UINT32               videoHeight            = 0;
    BOOL                 isVideoStreamAvailable = FALSE;
    CHISTREAMCONFIGINFO* pConfigInfo            = NULL;

    if ((NULL == pPipelineName) ||
        (NULL == pCreateInputInfo) ||
        (NULL == pCreateInputInfo->pInstanceProps) ||
        (NULL == pCreateInputInfo->pStreamConfig) ||
        (NULL == pCreateInputInfo->pFeatureDescriptor))
    {
        CHX_LOG_ERROR("Invalid input");
    }
    else if (TRUE == IsBPSCameraPipeline())
    {
        if (!CdkUtils::StrCmp(pPipelineName, "RealTimeFeatureThirdCamera"))
        {
            isPipelineSupported = TRUE;
        }
    }
    else
    {
        // Real-time will select preview / video / nonzsl-snapshot pipeline, based on stream config.

        pConfigInfo = pCreateInputInfo->pStreamConfig;

        for (UINT32 streamIndex = 0; streamIndex < pConfigInfo->numStreams; streamIndex++)
        {
            CHISTREAM* pStream = pConfigInfo->pChiStreams[streamIndex];
            if ((TRUE == pCreateInputInfo->bFrameworkVideoStream) &&
                (TRUE == IsVideoStream(pStream->grallocUsage) &&
                (ChiDataSpace::DataspaceHEIF != pStream->dataspace)))
            {
                isVideoStreamAvailable = TRUE;
                videoWidth             = pStream->width;
                videoHeight            = pStream->height;
                break;
            }
        }

        if (TRUE == isVideoStreamAvailable)
        {
            if ((TRUE == IsVideoEISV2Enabled(pPipelineName, pConfigInfo->operationMode))  ||
                (TRUE == IsVideoEISV3Enabled(pPipelineName, pConfigInfo->operationMode))  ||
                (TRUE == IsVideo4KEISV3Enabled(pPipelineName, videoWidth, videoHeight))   ||
                (TRUE == IsVideoWithoutEIS(pPipelineName)))
            {
                isPipelineSupported = TRUE;
            }
            else
            {
                isPipelineSupported = FALSE;
            }
        }
        else if ((FALSE == pCreateInputInfo->pInstanceProps->instanceFlags.isNZSLSnapshot) &&
                 (TRUE  == IsPreviewPipeline(pPipelineName)))
        {
            isPipelineSupported = TRUE;
        }
        else if ((TRUE == pCreateInputInfo->pInstanceProps->instanceFlags.isNZSLSnapshot) &&
                 (!CdkUtils::StrCmp(pPipelineName, "RealTimeFeatureNZSLSnapshotRDI")))
        {
            isPipelineSupported = TRUE;
        }
        else
        {
            isPipelineSupported = FALSE;
        }
    }

    if (TRUE == isPipelineSupported)
    {
        CHX_LOG_INFO("featureName:%s, pipelineName:%s, instanceFlags {nzsl:%d, swremosaic:%d, hwremosaic:%d, bpscam:%d}",
            pCreateInputInfo->pFeatureDescriptor->pFeatureName,
            pPipelineName,
            pCreateInputInfo->pInstanceProps->instanceFlags.isNZSLSnapshot,
            pCreateInputInfo->pInstanceProps->instanceFlags.isSWRemosaicSnapshot,
            pCreateInputInfo->pInstanceProps->instanceFlags.isHWRemosaicSnapshot,
            pCreateInputInfo->pInstanceProps->instanceFlags.isBPSCamera);
    }

    return isPipelineSupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnPreparePipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnPreparePipelineCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = ChiFeature2Base::OnPreparePipelineCreate(pKey);

    if (CDKResultSuccess == result)
    {
        ChiFeaturePipelineData*         pPipelineData  = GetPipelineData(pKey);
        const ChiFeature2InstanceProps* pInstanceprops = GetInstanceProps();
        Pipeline*                       pPipeline      = NULL;

        if (NULL != pPipelineData)
        {
            pPipeline = pPipelineData->pPipeline;
        }

        if ((NULL != pPipeline) && (NULL != pInstanceprops) && (TRUE == pInstanceprops->instanceFlags.isBPSCamera))
        {
            ChiMetadata* pMetadata          = pPipeline->GetDescriptorMetadata();
            BOOL         cameraRunningOnBPS = TRUE;

            result = pMetadata->SetTag("com.qti.chi.bpsrealtimecam", "cameraRunningOnBPS", &cameraRunningOnBPS, sizeof(BOOL));

            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Unable to set BPS Vendor Tag in Usecase Pool for camera %d", pInstanceprops->cameraId);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnSessionCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnSessionCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pKey)
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeaturePipelineData* pPipelineData    = GetPipelineData(pKey);
        ChiMetadataManager*     pMetadataManager = GetMetadataManager();
        Pipeline*               pPipeline        = NULL;

        if (NULL != pPipelineData)
        {
            pPipeline = pPipelineData->pPipeline;

            if ((NULL != pPipeline) && (NULL != pMetadataManager))
            {
                UINT32 metaclientId =  pMetadataManager->RegisterClient(TRUE,
                    pPipeline->GetTagList(),
                    pPipeline->GetTagCount(),
                    pPipeline->GetPartialTagCount(),
                    BufferQueueDepth,
                    ChiMetadataUsage::RealtimeOutput);

                SetMetadataClientId(pPipelineData, metaclientId);
            }

            for (UINT32 portIdx = 0; portIdx < pPipelineData->pOutputPortData.size(); portIdx++)
            {
                ChiFeaturePortData* pPortData = GetPortData(&(pPipelineData->pOutputPortData[portIdx].globalId));
                if ((NULL != pPortData) && (NULL != pPortData->pTarget))
                {
                    if (pPortData->maxBufferCount < pPortData->pChiStream->maxNumBuffers)
                    {
                        pPortData->maxBufferCount = pPortData->pChiStream->maxNumBuffers;
                    }
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnPortCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnPortCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult                       result         = CDKResultSuccess;
    const ChiFeature2InstanceProps* pInstanceprops = GetInstanceProps();

    result = ChiFeature2Base::OnPortCreate(pKey);

    if ((CDKResultSuccess == result) && (NULL != pInstanceprops))
    {
        ChiFeaturePortData* pPortData = GetPortData(pKey);

        if ((NULL != pPortData) && (NULL != pPortData->pTarget))
        {
            if ((*pKey == RealtimeOutputPortRaw) || (*pKey == RealtimeOutputPortFd))
            {
                pPortData->minBufferCount =
                    pPortData->maxBufferCount + REALTIME_CONSUMER_BUFFER_COUNT;
                pPortData->maxBufferCount = REALTIME_MAX_BUFFER_COUNT;
            }

            if (TRUE == pInstanceprops->instanceFlags.isNZSLSnapshot)
            {
                pPortData->minBufferCount = 0;
            }

            if ((TRUE == m_isVideoStreamEnabled) &&
                (*pKey == RealtimeOutputPortRaw))
            {
                pPortData->minBufferCount = 0;
            }
        }
    }

    // FD stream requires additional gralloc flags due to the generation of SW stats
    // and link with the GPU in the BPS camera scenario.
    if ((*pKey == RealtimeOutputPortFd) && (TRUE == IsBPSCameraPipeline()))
    {
        ChiFeaturePortData*             pPortData      = GetPortData(pKey);

        if ((NULL != pPortData) && (NULL != pPortData->pChiStream))
        {
            // FD is technically the registration stream in the BPS scenario.
            // As this the data is used by the SW generation algorithms,
            // for optimal performance we need to adjust the gralloc usage flags.
            // Additionally, the video flags are used to properly set the buffer
            // stride so that it matches the GPU consumer node expectations.
            pPortData->pChiStream->grallocUsage = pPortData->pChiStream->grallocUsage |
                                                  GRALLOC_USAGE_SW_READ_OFTEN |
                                                  GRALLOC_USAGE_HW_VIDEO_ENCODER;
            pPortData->consumerFlags            = pPortData->consumerFlags |
                                                  GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN |
                                                  GRALLOC1_CONSUMER_USAGE_VIDEO_ENCODER;
            pPortData->producerFlags            = pPortData->producerFlags |
                                                  GRALLOC1_PRODUCER_USAGE_VIDEO_DECODER;
        }
        else
        {
            CHX_LOG_ERROR("Unable to set FD Port Config in BPS Scenario!");
        }
    }

    if (*pKey == RealtimeOutputPortFd)
    {
        ChiFeaturePortData*             pPortData = GetPortData(pKey);
        if (NULL != pPortData)
        {
            pPortData->producerFlags    |= GRALLOC1_PRODUCER_USAGE_CPU_WRITE;
            pPortData->consumerFlags    = GRALLOC1_PRODUCER_USAGE_CPU_READ;
        }
    }

    /* For realtime pipeline we need to reconfigure RDI based on new sensor mode*/
    if (*pKey == RealtimeOutputPortRaw)
    {
        ChiFeaturePortData*     pPortData      = GetPortData(pKey);
        ChiFeaturePipelineData* pPipelineData  = GetPipelineData(pKey);

        if ((NULL != pPipelineData) && (NULL != pPipelineData->pPipeline) && (NULL != pPortData))
        {
            CHISENSORMODEINFO* pSensorMode = pPipelineData->pPipeline->GetSensorModeInfo();

            if (NULL != pSensorMode)
            {
                if ((pSensorMode->frameDimension.width * pSensorMode->frameDimension.height) >
                     (pPortData->pTarget->pChiStream->width * pPortData->pTarget->pChiStream->height))
                {
                    CHX_LOG_INFO("Reconfigure RDI Original = %dX%d Modified = %dX%d",
                        pPortData->pTarget->pChiStream->width,
                        pPortData->pTarget->pChiStream->height,
                        pSensorMode->frameDimension.width,
                        pSensorMode->frameDimension.height);
                    pPortData->pTarget->pChiStream->width =
                        pSensorMode->frameDimension.width;
                    pPortData->pTarget->pChiStream->height =
                        pSensorMode->frameDimension.height;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RealTime::OnPopulateConfigurationSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RealTime::OnPopulateConfigurationSettings(
    ChiFeature2RequestObject*     pRequestObject,
    const ChiFeature2Identifier*  pMetadataPortId,
    ChiMetadata*                  pInputMetadata
    ) const
{
    CDKResult                  result             = CDKResultSuccess;
    UINT8                      requestId          = pRequestObject->GetCurRequestId();
    ChiMetadata*               pSrcFeatureSetting = NULL;
    UINT32                     sceneModeValue     = ANDROID_CONTROL_SCENE_MODE_DISABLED;
    UINT32                     effectModeValue    = ANDROID_CONTROL_EFFECT_MODE_OFF;
    UINT32                     feature2Value      = pRequestObject->GetUsecaseRequestObject()->GetTuningFeature2Value();
    UINT32                     feature1Value      = 0;
    camera3_capture_request_t* pSrcRequest        = pRequestObject->GetUsecaseRequestObject()->GetCaptureRequest();
    ChiFeaturePipelineData*    pPipelineData      = GetPipelineData(pMetadataPortId);
    CHISENSORMODEINFO*         pSensorMode        = NULL;

    // Do not query downstream metadata if we are in burst snapshot as B2YUV submit's first and
    // its metadata may have already been released
    if (TRUE == IsManualCaptureRequired(pRequestObject))
    {
        pSrcFeatureSetting = GetExternalInputSettings(pRequestObject, pMetadataPortId, pRequestObject->GetCurRequestId());
        if (NULL != pSrcFeatureSetting)
        {
            result = pInputMetadata->Copy(*pSrcFeatureSetting, true);
        }
    }

    MergeAppSettings(pRequestObject, &pInputMetadata, TRUE);

    result = ChiFeature2Base::OnPopulateConfigurationSettings(
        pRequestObject, pMetadataPortId, pInputMetadata);

    ChiFeatureRealtimeContext* pPrivContext = static_cast<ChiFeatureRealtimeContext *>(
        GetFeaturePrivContext(pRequestObject));

    if (NULL != pPrivContext && TRUE == pPrivContext->isInSensorHDR3ExpCapture)
    {
        ChiFeature2StageInfo stageInfo             = { 0 };
        UINT32               seamlessInSensorState = pPrivContext->seamlessInSensorState;

        pInputMetadata->SetTag("com.qti.insensor_control", "seamless_insensor_state", &seamlessInSensorState, 1);

        UINT32               feature1Value         = static_cast<UINT32>(ChiModeFeature1SubModeType::InSensorHDR3Exp);
        pInputMetadata->SetTag("org.quic.camera2.tuning.feature", "Feature1Mode", &feature1Value, 1);

        GetCurrentStageInfo(pRequestObject, &stageInfo);

        CHX_LOG_INFO("In-snesor HDR 3 exposure state: %d requestid = %d stageSequence = %d",
                     seamlessInSensorState,
                     requestId,
                     stageInfo.stageSequenceId);
    }

    if (NULL != pPipelineData && NULL != pPipelineData->pPipeline)
    {
        pSensorMode = pPipelineData->pPipeline->GetSensorModeInfo();

        // Override non-realtime usecases with corresponding realtime usecase values
        ChiModeUsecaseSubModeType usecaseMode = pRequestObject->GetFeatureUsecaseMode();

        if (TRUE == GetInstanceProps()->instanceFlags.isNZSLSnapshot)
        {
            // Set stats skip for non-zsl snapshot
            UINT32 statsSkip = 1;
            result = pInputMetadata->SetTag("com.qti.chi.statsSkip", "skipFrame", &statsSkip, 1);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Set stats skip for non-zsl snasphot failed");
            }
        }

        if (ChiModeUsecaseSubModeType::Liveshot == usecaseMode)
        {
            usecaseMode = ChiModeUsecaseSubModeType::Video;
        }

        if (ChiModeUsecaseSubModeType::Snapshot == usecaseMode)
        {
            usecaseMode = ChiModeUsecaseSubModeType::Preview;
        }

        ChxUtils::FillTuningModeData(pInputMetadata,
            usecaseMode,
            pSensorMode->modeIndex,
            &effectModeValue,
            &sceneModeValue,
            &feature1Value,
            &feature2Value);

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2RealTime* pFeatureRealtime = ChiFeature2RealTime::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeatureRealtime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoQueryCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DoQueryCaps(
    VOID*                 pConfig,
    ChiFeature2QueryInfo* pQueryInfo)
{
    CDK_UNUSED_PARAM(pConfig);
    CDKResult result = CDKResultSuccess;

    if (NULL != pQueryInfo)
    {
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2RTCaps);
        pQueryInfo->ppCapabilities = &Feature2RTCaps[0];
    }
    else
    {
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetVendorTags(
    VOID* pVendorTags)
{
    CDK_UNUSED_PARAM(pVendorTags);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoStreamNegotiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DoStreamNegotiation(
    StreamNegotiationInfo*          pNegotiationInfo,
    StreamNegotiationOutput*        pNegotiationOutput)
{
    CDKResult result          = CDKResultSuccess;
    BOOL isFrameworkYUVStream = FALSE;

    if (NULL == pNegotiationInfo || pNegotiationOutput == NULL)
    {
        CHX_LOG_ERROR("Invalid Arg! pNegotiation=%p, pDesiredStreamConfig=%p",
            pNegotiationInfo, pNegotiationOutput);
        result = CDKResultEInvalidArg;
    }
    else
    {

        // Clone a stream and put it into the list of streams that will be freed by the feature
        auto CloneStream = [&pNegotiationOutput](CHISTREAM* pSrcCameraStream) -> CHISTREAM* {
            ChiStream* pStream = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
            ChxUtils::Memcpy(pStream, pSrcCameraStream, sizeof(CHISTREAM));
            pNegotiationOutput->pOwnedStreams->push_back(pStream);
            return pStream;
        };

        if (FALSE == pNegotiationInfo->pFeatureInstanceId->flags.isNZSLSnapshot)
        {
            const UINT   numStreams     = pNegotiationInfo->pFwkStreamConfig->numStreams;
            CHISTREAM**  ppChiStreams   = pNegotiationInfo->pFwkStreamConfig->pChiStreams;
            CHISTREAM*   pRDIStream     = CloneStream(&RDIStream);
            CHISTREAM*   pVideoStream   = NULL;
            UINT8        physicalCamIdx = pNegotiationInfo->pFeatureInstanceId->cameraId;
            UINT32       physicalCamId  = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[physicalCamIdx]->cameraId;
            UINT8        displayIdx     = 0;

            if (NULL != pRDIStream)
            {
                GetSensorOutputDimension(physicalCamId,
                                         pNegotiationInfo->pFwkStreamConfig,
                                         pNegotiationInfo->pFeatureInstanceId->flags,
                                         &pRDIStream->width,
                                         &pRDIStream->height);

                pNegotiationOutput->pStreams->clear();
                for (UINT streamIdx = 0; streamIdx < numStreams; streamIdx++)
                {
                    camera3_stream_t* pStream = reinterpret_cast<camera3_stream_t*>(ppChiStreams[streamIdx]);
                    if (ppChiStreams[streamIdx]->format != ChiStreamFormatBlob)
                    {
                        if (ChiStreamFormatRaw10 == ppChiStreams[streamIdx]->format ||
                            ChiStreamFormatRaw16 == ppChiStreams[streamIdx]->format)
                        {
                            if (ppChiStreams[streamIdx]->width > pRDIStream->width ||
                                ppChiStreams[streamIdx]->height > pRDIStream->height)
                            {
                                CHX_LOG_INFO("feature flags:%x, selected sensor rdi:%dx%d, current stream:%dx%d",
                                    pNegotiationInfo->pFeatureInstanceId->flags.isNZSLSnapshot,
                                    pRDIStream->width, pRDIStream->height,
                                    ppChiStreams[streamIdx]->width, ppChiStreams[streamIdx]->height);
                            }
                            else
                            {
                                pNegotiationOutput->pStreams->push_back(ppChiStreams[streamIdx]);
                            }
                        }
                        else
                        {
                            if (FALSE == UsecaseSelector::IsYUVOutThreshold(pStream))
                            {
                                pNegotiationOutput->pStreams->push_back(ppChiStreams[streamIdx]);
                            }
                        }
                    }

                    if ((ppChiStreams[streamIdx]->format == ChiStreamFormatImplDefined) &&
                        (UsecaseSelector::IsVideoStream(pStream)))
                    {
                        pNegotiationOutput->isFrameworkVideoStream = TRUE;
                    }

                    if (UsecaseSelector::IsYUVSnapshotStream(pStream))
                    {
                        isFrameworkYUVStream = TRUE;
                    }

                    if (UsecaseSelector::IsPreviewStream(pStream))
                    {
                        // Update FD stream with respect Preview stream's aspect ratio
                        FLOAT previewAspectRatio  =
                            static_cast<FLOAT>(ppChiStreams[streamIdx]->width) / ppChiStreams[streamIdx]->height;
                        UsecaseSelector::UpdateFDStream(previewAspectRatio);
                        displayIdx    = streamIdx;
                    }
                }

                CHISTREAM    fdStream  = UsecaseSelector::GetFDOutStream();
                CHISTREAM*   pFDStream = CloneStream(&fdStream);

                pNegotiationOutput->pStreams->push_back(pRDIStream);
                pNegotiationOutput->pStreams->push_back(pFDStream);

                if ((FALSE == pNegotiationOutput->isFrameworkVideoStream) && (FALSE == isFrameworkYUVStream))
                {
                    pVideoStream = CloneStream(ppChiStreams[displayIdx]);
                    pNegotiationOutput->pStreams->push_back(pVideoStream);
                }
            }
            else
            {
                CHX_LOG_ERROR("Fail to clone RDI stream");
                result = CDKResultEFailed;
            }
        }
        else
        {
            ChiFeature2InstanceFlags flags = pNegotiationInfo->pFeatureInstanceId->flags;
            UINT32 maxSensorWidth  = 0;
            UINT32 maxSensorHeight = 0;
            for (UINT i = 0; i < pNegotiationInfo->pLogicalCameraInfo->m_cameraCaps.numSensorModes; i++)
            {
                CHIRECT* pSensorDimension = &(pNegotiationInfo->pLogicalCameraInfo->pSensorModeInfo[i].frameDimension);
                if ((pSensorDimension->width  > maxSensorWidth) ||
                    (pSensorDimension->height > maxSensorHeight))
                {
                    maxSensorWidth  = pSensorDimension->width;
                    maxSensorHeight = pSensorDimension->height;
                }
            }

            NZSLSnapshotRDIStream.width  = maxSensorWidth;
            NZSLSnapshotRDIStream.height = maxSensorHeight;
            if (flags.isSWRemosaicSnapshot == TRUE)
            {
                NZSLSnapshotRDIStream.format = ChiStreamFormatRaw16;
            }
            CHX_LOG_CONFIG("non-zsl snapshot rdi stream, size:%dx%d, format:%d",
                           NZSLSnapshotRDIStream.width, NZSLSnapshotRDIStream.height, NZSLSnapshotRDIStream.format);

            const UINT   numStreams     = pNegotiationInfo->pFwkStreamConfig->numStreams;
            CHISTREAM**  ppChiStreams   = pNegotiationInfo->pFwkStreamConfig->pChiStreams;
            CHISTREAM*   pFwkRDIStream  = NULL;

            for (UINT32 stream = 0; stream < numStreams; stream++)
            {
                if (UsecaseSelector::IsRawStream(reinterpret_cast<camera3_stream_t*>(ppChiStreams[stream])))
                {
                    pFwkRDIStream = ppChiStreams[stream];
                }
            }

            if (NULL != pFwkRDIStream)
            {
                pNegotiationOutput->pStreams->push_back(pFwkRDIStream);
            }
            else
            {
                // only one full size rdi stream
                pNegotiationOutput->pStreams->push_back(CloneStream(&NZSLSnapshotRDIStream));
            }
        }

        // configure desired stream
        pNegotiationOutput->pDesiredStreamConfig->numStreams       = pNegotiationOutput->pStreams->size();
        pNegotiationOutput->pDesiredStreamConfig->operationMode    = pNegotiationInfo->pFwkStreamConfig->operationMode;
        pNegotiationOutput->pDesiredStreamConfig->pChiStreams      = &(*(pNegotiationOutput->pStreams))[0];
        pNegotiationOutput->pDesiredStreamConfig->pSessionSettings = NULL;

        CHX_LOG_CONFIG("numStream:%d", pNegotiationOutput->pDesiredStreamConfig->numStreams);

    }

    return result;
}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2OpsEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDK_VISIBILITY_PUBLIC VOID ChiFeature2OpsEntry(
    CHIFEATURE2OPS* pChiFeature2Ops)
{
    if (NULL != pChiFeature2Ops)
    {
        pChiFeature2Ops->size               = sizeof(CHIFEATURE2OPS);
        pChiFeature2Ops->majorVersion       = Feature2MajorVersion;
        pChiFeature2Ops->minorVersion       = Feature2MinorVersion;
        pChiFeature2Ops->pVendorName        = VendorName;
        pChiFeature2Ops->pCreate            = CreateFeature;
        pChiFeature2Ops->pQueryCaps         = DoQueryCaps;
        pChiFeature2Ops->pGetVendorTags     = GetVendorTags;
        pChiFeature2Ops->pStreamNegotiation = DoStreamNegotiation;
    }
}
#ifdef __cplusplus
}
#endif // __cplusplus
