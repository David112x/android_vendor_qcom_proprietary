////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2base.cpp
/// @brief CHI feature base class definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chibinarylog.h"
#include "chifeature2base.h"
#include "chifeature2utils.h"

// NOWHINE FILE CP006:  Used standard libraries for performance improvements

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::GetPipelineIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiFeature2Base::GetPipelineIndex(
    const ChiUsecase* pUsecaseDesc,
    const CHAR*       pPipelineName)
{
    UINT32 index = CDKInvalidId;

    for (UINT32 i = 0; i < pUsecaseDesc->numPipelines; i++)
    {
        if (0 == strcmp(pPipelineName, pUsecaseDesc->pPipelineTargetCreateDesc[i].pPipelineName))
        {
            index = i;
            break;
        }
    }
    if (CDKInvalidId == index)
    {
        CHX_LOG_ERROR("Unable to find matching index for pipeline %s", pPipelineName);
    }
    return index;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::GetOutputMetadataBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHITARGETBUFFERINFOHANDLE ChiFeature2Base::GetOutputMetadataBuffer(
    const ChiFeaturePipelineData* pPipelineData,
    UINT32                        sequenceNumber
    ) const
{
    CDKResult result                               = CDKResultSuccess;
    CHITargetBufferManager*   pTargetBufferManager = pPipelineData->pOutputMetaTbm;
    CHITARGETBUFFERINFOHANDLE hBuffer              = NULL;

    if (NULL == pTargetBufferManager)
    {
        CHX_LOG_ERROR("No metadata TBM for pipeline %s", pPipelineData->pPipelineName);
        result = CDKResultENoSuch;
    }

    if (CDKResultSuccess == result)
    {
        hBuffer = pTargetBufferManager->SetupTargetBuffer(sequenceNumber);
    }

    return hBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::GetInputMetadataBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHITARGETBUFFERINFOHANDLE ChiFeature2Base::GetInputMetadataBuffer(
    const ChiFeaturePipelineData* pPipelineData,
    UINT32                        sequenceNumber
    ) const
{
    CDKResult                 result               = CDKResultSuccess;
    CHITargetBufferManager*   pTargetBufferManager = pPipelineData->pSettingMetaTbm;
    CHITARGETBUFFERINFOHANDLE hBuffer              = NULL;

    if (NULL == pTargetBufferManager)
    {
        CHX_LOG_ERROR("No metadata TBM for pipeline %s", pPipelineData->pPipelineName);
        result = CDKResultENoSuch;
    }

    if (CDKResultSuccess == result)
    {
        hBuffer = pTargetBufferManager->SetupTargetBuffer(sequenceNumber);
    }

    return hBuffer;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::GetStreamBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::GetStreamBuffer(
    CHITARGETBUFFERINFOHANDLE   handle,
    UINT64                      key,
    CHISTREAMBUFFER*            pBuffer
    ) const
{
    CDKResult result = CDKResultSuccess;
    CHISTREAMBUFFER* pTargetBuffer = NULL;
    CHITargetBufferManager* pTargetBufferManager = CHITargetBufferManager::GetTargetBufferManager(handle);

    if (NULL != pTargetBufferManager)
    {
        pTargetBuffer = reinterpret_cast<CHISTREAMBUFFER*>(pTargetBufferManager->GetTarget(handle, key));

        if (pTargetBuffer != NULL)
        {
            ChxUtils::Memcpy(pBuffer, pTargetBuffer, sizeof(CHISTREAMBUFFER));
        }
        else
        {
            CHX_LOG_ERROR("Unable to get buffer info for handle %p", handle);
            result = CDKResultENoSuch;
        }
    }
    else
    {
        CHX_LOG_ERROR("Unable to get target buffer manager for handle %p", handle);
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::GetOutputBufferHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHITARGETBUFFERINFOHANDLE ChiFeature2Base::GetOutputBufferHandle(
    const ChiFeaturePortData* pPortData,
    UINT32                    sequenceNumber
    ) const
{
    CDKResult result = CDKResultSuccess;
    CHITargetBufferManager* pTargetBufferManager = NULL;
    CHITARGETBUFFERINFOHANDLE hBuffer = NULL;

    if ((NULL == pPortData) || (NULL == pPortData->pOutputBufferTbm))
    {
        CHX_LOG_ERROR("Invalid argument: NULL pPortData");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pTargetBufferManager = pPortData->pOutputBufferTbm;
        hBuffer              = pTargetBufferManager->SetupTargetBuffer(sequenceNumber);

        if (NULL == hBuffer)
        {
            CHX_LOG_ERROR("Returning null buffer handle for port %s", pPortData->pPortName);
            result = CDKResultEFailed;
        }
    }

    return hBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::~ChiFeature2Base
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Base::~ChiFeature2Base()
{
    DestroyFeatureData();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ProcessRequest(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult               result       = CDKResultSuccess;
    ChiFeature2RequestState requestState = ChiFeature2RequestState::InvalidMax;

    if (NULL == pRequestObject)
    {
        CHX_LOG_ERROR("Invalid argument: NULL pRequestObject");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        UINT8 requestId  = 0;
        BOOL  needUpdate = FALSE;
        UINT8 numRequest = pRequestObject->GetNumRequests();
        BOOL  canExecute = TRUE;
        do
        {
            requestState = pRequestObject->GetCurRequestState(requestId);

            switch (requestState)
            {
                case ChiFeature2RequestState::InputResourcePending:
                    result = HandleInputResourcePending(pRequestObject, requestId);
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("HandleInputResourcePending failed with result: %d", result);
                    }
                    canExecute = FALSE;
                    break;

                // Executing: FOR EXAMPLE:
                // RDI buffers from ZSL queue are supplied to B2Y feature. B2Y feature is done and
                // notified RT feature to release TargetBuffers for the RDI port.
                // RealTime feature has still not submitted request to session. We need to
                // release and acknowledge for RDI port. FRO completion will be taken care when the
                // backend result is processed by realtime
                case ChiFeature2RequestState::Executing:
                // When ProcessRequest comes from DownStream Feature, UpStream Feature is done with
                // its Camx submissions and is waiting for pending outputs from backend.
                case ChiFeature2RequestState::OutputResourcePending:
                case ChiFeature2RequestState::OutputNotificationPending:
                case ChiFeature2RequestState::OutputErrorNotificationPending:
                    result = HandleOutputNotificationPending(pRequestObject, requestId);
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("HandleOutputNotificationPending failed with result: %d", result);
                    }
                    canExecute = FALSE;
                    break;

                case ChiFeature2RequestState::Initialized:
                    if (TRUE == canExecute)
                    {
                        result = OnProcessRequest(pRequestObject, requestId);
                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("OnProcessRequest failed with result: %d", result);
                        }
                    }
                    break;

                case ChiFeature2RequestState::Complete:
                    OnProcessingDependenciesComplete(pRequestObject, requestId);
                    break;

                default:
                    break;
            }

            if (pRequestObject->GetCurRequestState(requestId) != requestState)
            {
                needUpdate = TRUE;
            }
            requestId++;
        } while ((CDKResultSuccess == result) && (requestId < numRequest));

        if ((CDKResultSuccess == result) && (TRUE == needUpdate) && (TRUE == canExecute))
        {
            result = HandleBatchRequest(pRequestObject);
        }
    }

    if (CDKResultECancelledRequest == result)
    {
        CHX_LOG_INFO("Flush is happening and thereby returning success");
        result = CDKResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::HandleBatchRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::HandleBatchRequest(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult               result            = CDKResultSuccess;
    ChiFeature2RequestState resourcePending   = ChiFeature2RequestState::InputResourcePending;

    for (UINT8 requestId = 0; requestId < pRequestObject->GetNumRequests(); requestId++)
    {
        if ((resourcePending != pRequestObject->GetCurRequestState(requestId)) ||
            ((pRequestObject->GetFeature()->GetFeatureId() == static_cast<UINT32>(ChiFeature2Type::FRAME_SELECT)) ||
             (pRequestObject->GetFeature()->GetFeatureId() == static_cast<UINT32>(ChiFeature2Type::DEMUX)) ||
             (pRequestObject->GetFeature()->GetFeatureId() == static_cast<UINT32>(ChiFeature2Type::SERIALIZER))))
        {
            resourcePending = pRequestObject->GetCurRequestState(requestId);
            break;
        }
    }

    if (ChiFeature2RequestState::InputResourcePending == resourcePending)
    {
        // Reset current requestId to handle case where dependency is met
        pRequestObject->SetCurRequestId(0);
        result = ProcessDependency(pRequestObject);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnProcessRequest(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId)
{
    CDKResult               result       = CDKResultSuccess;
    ChiFeature2RequestState requestState = ChiFeature2RequestState::InvalidMax;

    if (NULL == pRequestObject)
    {
        CHX_LOG_ERROR("Invalid argument: NULL pRequestObject");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        do
        {
            pRequestObject->SetCurRequestId(requestId);

            if (requestId != pRequestObject->GetCurRequestId())
            {
                CHX_LOG_ERROR("%s requestId:%d is not matching currReqId %d. Breaking here to avoid infinite loop",
                            pRequestObject->IdentifierString(), requestId, pRequestObject->GetCurRequestId());
                break;
            }

            requestState = pRequestObject->GetCurRequestState(requestId);
            CHX_LOG_INFO("%s requestState:%s, requestId:%d",
                         pRequestObject->IdentifierString(),
                         ChiFeature2RequestStateStrings[static_cast<UINT8>(requestState)],
                         requestId);

            switch (requestState)
            {
                case ChiFeature2RequestState::Initialized:
                {
                    ChiFeatureSessionData* pSessionData = m_pSessionData[0];
                    if ((NULL != pSessionData) &&
                        (TRUE == pSessionData->isFlushInProgress))
                    {
                        CHX_LOG_WARN("%s requestId:%d has been dropped as flush is in progress",
                            pRequestObject->IdentifierString(), requestId);
                        result = CDKResultECancelledRequest;
                    }
                    else
                    {
                        result = HandlePrepareRequest(pRequestObject);
                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("HandlePrepareRequest returned error: %d", result);
                        }
                    }
                    break;
                }
                case ChiFeature2RequestState::ReadyToExecute:
                case ChiFeature2RequestState::Executing:
                {
                    ChiFeatureSessionData* pSessionData = m_pSessionData[0];
                    if ((NULL != pSessionData) && (TRUE == pSessionData->isFlushInProgress))
                    {
                        result = CDKResultECancelledRequest;
                    }
                    else
                    {
                        result = HandleExecuteProcessRequest(pRequestObject);
                        if (CDKResultENoMore == result)
                        {
                            CHX_LOG_WARN("HandleExecuteProcessRequest is not continuing due to errors");
                        }
                        else if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("HandleExecuteProcessRequest returned error: %d", result);
                        }
                    }
                    break;
                }
                case ChiFeature2RequestState::OutputErrorResourcePending:
                    // Used during error to track state of error; only valid operation is from
                    // OutputResourcePending -> OutputErrorResourcePending and is placeholder
                    break;
                case ChiFeature2RequestState::Complete:
                    CHX_LOG_INFO("Already in complete state for requestId: %d", requestId);
                    break;
                default:
                    CHX_LOG_INFO("Move to State: %d", requestState);
                    break;
            }
        } while ((CDKResultSuccess == result) && (TRUE == CanRequestContinue(pRequestObject->GetCurRequestState(requestId))));

        if (CDKResultSuccess == result)
        {
            pRequestObject->MoveToNextRequest();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::RequestThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2Base::RequestThread(
    VOID* pThreadData)
{
    PerThreadData*   pPerThreadData   = reinterpret_cast<PerThreadData*>(pThreadData);
    ChiFeature2Base* pChiFeature2Base = reinterpret_cast<ChiFeature2Base*>(pPerThreadData->pPrivateData);

    pChiFeature2Base->FlushThreadHandler();

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::FlushThreadHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::FlushThreadHandler()
{
    while (TRUE)
    {
        m_pFlushThreadMutex->Lock();
        if (TRUE == m_shouldWaitFlush)
        {
            CHX_LOG_INFO("ChiFeature2Base::Flush WAIT featureId: %d", m_featureId);
            m_pFlushRequestAvailable->Wait(m_pFlushThreadMutex->GetNativeHandle());
        }
        CHX_LOG_INFO("ChiFeature2Base::Flush GO featureId: %d", m_featureId);

        if (m_flushRequestProcessTerminate == TRUE)
        {
            CHX_LOG_INFO("ChiFeature2Base::TERMINATE Flush Thread featureId: %d", m_featureId);
            m_pFlushThreadMutex->Unlock();
            break;
        }

        ExecuteFlush();
        m_shouldWaitFlush = TRUE;
        m_pFlushThreadMutex->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ExecuteFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::ExecuteFlush()
{
    CDKResult              result         = CDKResultSuccess;
    VOID*                  pPrivate       = NULL;
    ChiFeatureSessionData* pCbSessionData = NULL;

    m_pThreadManager->FlushJob(m_hFeatureJob, TRUE);

    CHX_LOG_INFO("ChiFeature2Base::Flush Executing featureid: %d p: %p", m_featureId, m_pSessionData[0]);
    for (auto pSessionData : m_pSessionData)
    {
        // If there're more than one sesion in the feature, use the last feature's cb data.
        // All the sessions in the same feature should share the same manager callback data.
        pPrivate       = pSessionData->sessionCbData.featureGraphManagerCallbacks.pPrivateCallbackData;
        pCbSessionData = pSessionData;

        result         = ExecuteFlushHelper(pSessionData);

        if (CDKResultSuccess != result)
        {
            break;
        }
    }

    CHX_LOG_INFO("ChiFeature2Base::Flush Done. featureid: %d", m_featureId);

    if ((NULL != pPrivate) && (NULL != pCbSessionData))
    {
        pCbSessionData->sessionCbData.featureGraphManagerCallbacks.ChiFeatureGraphFlush(m_featureId, result, pPrivate);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ExecuteFlushHelper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ExecuteFlushHelper(
    ChiFeatureSessionData* pSessionData)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pSessionData)
    {
        Session* pSession = pSessionData->pSession;
        ChxUtils::AtomicStoreU32(&pSessionData->isFlushInProgress, TRUE);
        CHX_LOG_CONFIG("pSessionData isFlushInProgress TRUE Feature %s Session %s",
                       m_pFeatureName, pSessionData->pSessionName);
        if (NULL != pSession)
        {
            result = ExtensionModule::GetInstance()->Flush(pSession->GetSessionHandle());
            if (CDKResultSuccess != result)
            {
                CHX_LOG_WARN("Flush ERROR for session: %s", pSessionData->pSessionName);
            }
            DoFlush();
        }
        ChxUtils::AtomicStoreU32(&pSessionData->isFlushInProgress, FALSE);
        CHX_LOG_CONFIG("pSessionData isFlushInProgress FALSE Feature %s Session %s",
                       m_pFeatureName, pSessionData->pSessionName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::Flush(
    BOOL isSynchronousFlush)
{
    CDKResult result      = CDKResultSuccess;
    CDKResult flushResult = CDKResultSuccess;

    if (TRUE == isSynchronousFlush)
    {
        m_pThreadManager->FlushJob(m_hFeatureJob, TRUE);
        // Synchronous Flush
        for (auto pSessionData : m_pSessionData)
        {
            result = ExecuteFlushHelper(pSessionData);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_WARN("Flush ERROR for session: %s", pSessionData->pSessionName);
                flushResult = result;
            }
        }
    }
    else
    {
        // Asynchronous Flush
        m_pFlushThreadMutex->Lock();
        m_shouldWaitFlush = FALSE;
        m_pFlushRequestAvailable->Signal();
        m_pFlushThreadMutex->Unlock();
    }

    return flushResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetExternalGlobalPortIdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<ChiFeature2Identifier> ChiFeature2Base::GetExternalGlobalPortIdList()
{
    // Iterate through all sessions, pipelines, and ports to find all external ports for the feature
    std::vector<ChiFeature2Identifier> externalGlobalPortIdList;
    for (std::vector<ChiFeatureSessionData*>::iterator pSessionData = m_pSessionData.begin();
            pSessionData != m_pSessionData.end();
            ++pSessionData)
    {
        for (std::vector<ChiFeaturePipelineData*>::iterator pPipelineData = (*pSessionData)->pPipelineData.begin();
                pPipelineData != (*pSessionData)->pPipelineData.end();
                ++pPipelineData)
        {
            for (std::vector<ChiFeaturePortData>::iterator inputPortData = (*pPipelineData)->pInputPortData.begin();
                    inputPortData != (*pPipelineData)->pInputPortData.end();
                    ++inputPortData)
            {
                if (ChiFeature2PortDirectionType::ExternalInput == (*inputPortData).globalId.portDirectionType)
                {
                    // We found a source port
                    ChiFeature2Identifier globalPortId = (*inputPortData).globalId;
                    externalGlobalPortIdList.push_back(globalPortId);
                }
            }

            for (std::vector<ChiFeaturePortData>::iterator outputPortData = (*pPipelineData)->pOutputPortData.begin();
                    outputPortData != (*pPipelineData)->pOutputPortData.end();
                    ++outputPortData)
            {
                if (ChiFeature2PortDirectionType::ExternalOutput == (*outputPortData).globalId.portDirectionType)
                {
                    // We found a sink port
                    ChiFeature2Identifier globalPortId = (*outputPortData).globalId;
                    externalGlobalPortIdList.push_back(globalPortId);
                }
            }
        }
    }

    return externalGlobalPortIdList;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetPortDescriptorFromPortId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const ChiFeature2PortDescriptor* ChiFeature2Base::GetPortDescriptorFromPortId(
    const ChiFeature2Identifier* pPortId
    ) const
{
    const ChiFeature2PortDescriptor* pPortDescriptor = NULL;

    if (NULL != pPortId)
    {
        auto portDescriptor = m_portIdToPortDescMap.find(*pPortId);
        pPortDescriptor     = &((*portDescriptor).second);
    }
    else
    {
        CHX_LOG_ERROR("pPortId is NULL");
    }

    return pPortDescriptor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::SubmitRequestToSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::SubmitRequestToSession(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    // Get the input configuration information from request object.

    // Get the output configuration information from request object.

    // Map the output port descriptors into session & pipeline.

    UINT8                               numChiFeature2DependencyConfigInfo  = 0;
    ChiFeature2DependencyConfigInfo*    pDependencyConfigInfo               = NULL;
    ChiFeature2SessionInfo*             pDependencyConfig                   = NULL;
    CDKResult                           result                              = CDKResultSuccess;

    numChiFeature2DependencyConfigInfo = pRequestObject->GetDependencyConfig(ChiFeature2SequenceOrder::Current,
        &pDependencyConfig, pRequestObject->GetCurRequestId());

    for (UINT8 sessionIdx = 0; sessionIdx < numChiFeature2DependencyConfigInfo; sessionIdx++)
    {
        CHIMETAHANDLE                    hInputMetadata      = NULL;
        ChiMetadata*                     pInputMetadata      = NULL;
        CHIMETAHANDLE                    hOutputMetadata     = NULL;
        ChiPipelineRequest*              pPipelineRequest    = NULL;
        ChiCaptureRequest*               pRequest            = NULL;
        Session*                         pSession            = NULL;
        Pipeline*                        pPipeline           = NULL;

        UINT32  numInputs                   = 0;
        UINT32  numOutputs                  = 0;

        ChiFeature2SessionInfo* pSessionConfigInfo  = &pDependencyConfig[sessionIdx];
        UINT8                   numPipelines        = pSessionConfigInfo->numPipelines;
        ChiFeature2Identifier   key                 = {pSessionConfigInfo->sessionId, 0, 0};
        ChiFeatureSessionData*  pSessionData        = GetSessionData(&key);
        CHISTREAMBUFFER**       ppInputBuffers;
        CHISTREAMBUFFER**       ppOutputBuffers;
        ChiFeature2StageInfo    stageInfo               = { 0 };
        BOOL hasInputMetadataPort                       = FALSE;
        BOOL hasOutputMetadataPort                      = FALSE;

        if (NULL == pSessionData)
        {
            CHX_LOG_ERROR("NULL: Session data is NULL for sessionId = %d",
                pSessionConfigInfo->sessionId);
            result = CDKResultENoSuch;
            break;
        }

        if (CDKResultSuccess == result)
        {
            pSession = pSessionData->pSession;
            pPipelineRequest  = static_cast<ChiPipelineRequest*>(CHX_CALLOC(sizeof(ChiPipelineRequest)));
            pRequest          = static_cast<ChiCaptureRequest*>(CHX_CALLOC(sizeof(ChiCaptureRequest) * numPipelines));
            ppInputBuffers    = static_cast<CHISTREAMBUFFER**>(CHX_CALLOC(sizeof(CHISTREAMBUFFER*) * numPipelines));
            ppOutputBuffers   = static_cast<CHISTREAMBUFFER**>(CHX_CALLOC(sizeof(CHISTREAMBUFFER*) * numPipelines));

            if ((NULL != pPipelineRequest) && (NULL != pRequest) && (NULL != ppInputBuffers) && (NULL != ppOutputBuffers))
            {
                ChiFeatureFrameCallbackData*    pFrameData   = NULL;
                ChiFeatureCombinedCallbackData* pCombinedCallback = NULL;
                ChiFeatureSequenceData*      pRequestData = static_cast<ChiFeatureSequenceData*>(
                    pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current, pRequestObject->GetCurRequestId()));

                if (NULL != pRequestData)
                {
                    pFrameData = &pRequestData->frameCbData;
                    pCombinedCallback = &pRequestData->combinedCbData;
                    pCombinedCallback->pCombinedCallbackData[0] = pFrameData;
                }

                if (NULL != pFrameData)
                {
                    pFrameData->pRequestObj    = pRequestObject;
                    pFrameData->requestId      = pRequestObject->GetCurRequestId();
                    pFrameData->sequenceId     = pRequestObject->GetProcessSequenceId(
                        ChiFeature2SequenceOrder::Current, pRequestObject->GetCurRequestId());
                    result = GetCurrentStageInfo(pRequestObject, &stageInfo);
                    if (CDKResultSuccess == result)
                    {
                        pFrameData->stageId          = stageInfo.stageId;
                        pFrameData->stagesequenceId  = stageInfo.stageSequenceId;
                    }

                    // Create the pipeline request.
                    for (UINT8 pipelineIdx = 0; pipelineIdx < numPipelines; pipelineIdx++)
                    {
                        ChiFeature2PipelineInfo*    pPipelineConfigInfo     = &pSessionConfigInfo->pPipelineInfo[pipelineIdx];
                        ChiFeaturePipelineData*     pFeaturePipelineData    =
                            pSessionData->pPipelineData[pPipelineConfigInfo->pipelineId];

                        if (NULL == pFeaturePipelineData)
                        {
                            CHX_LOG_ERROR("NULL: Pipeline data is NULL for sessionId = %d, pipelineId = %d",
                                pSessionConfigInfo->sessionId, pPipelineConfigInfo->pipelineId);
                            result = CDKResultENoSuch;
                            break;
                        }

                        pPipeline = pFeaturePipelineData->pPipeline;
                        if (NULL == pPipeline)
                        {
                            CHX_LOG_ERROR("pPipeline is NULL");
                            result = CDKResultEInvalidPointer;
                            break;
                        }

                        if (CDKResultSuccess == result)
                        {
                            pDependencyConfigInfo =
                                reinterpret_cast<ChiFeature2DependencyConfigInfo*>(pPipelineConfigInfo->handle);
                            if (NULL == pDependencyConfigInfo)
                            {
                                CHX_LOG_ERROR("pDependencyConfigInfo is NULL");
                                result = CDKResultEInvalidPointer;
                                break;
                            }
                            numInputs   = pDependencyConfigInfo->numEnabledInputPorts;
                            numOutputs  = pDependencyConfigInfo->numEnabledOutputPorts;

                            ppInputBuffers[pipelineIdx]   =
                                static_cast<CHISTREAMBUFFER*>(CHX_CALLOC(sizeof(CHISTREAMBUFFER) * numInputs));
                            ppOutputBuffers[pipelineIdx]  =
                                static_cast<CHISTREAMBUFFER*>(CHX_CALLOC(sizeof(CHISTREAMBUFFER) * numOutputs));

                            if ((NULL != ppInputBuffers[pipelineIdx]) && (NULL != ppOutputBuffers[pipelineIdx]))
                            {
                                pRequest[pipelineIdx].frameNumber        = pRequestData->frameNumber;
                                pRequest[pipelineIdx].hPipelineHandle    = reinterpret_cast<CHIPIPELINEHANDLE>(
                                    pSession->GetPipelineHandle(pPipelineConfigInfo->pipelineId));

                                ChiFeature2BufferMetadataInfo   bufferMetaInfo      = { 0 };
                                ChiFeaturePortData*             pPortData           = NULL;
                                ChiFeature2Identifier           portKey             = { 0 };
                                ChiFeature2Identifier           tempKey             = { 0 };
                                ChiModeUsecaseSubModeType       usecaseMode         = pRequestObject->GetFeatureUsecaseMode();

                                if (NULL != pSession->GetSensorModeInfo(pipelineIdx))
                                {
                                    UINT32                          modeIndex           =
                                    pSession->GetSensorModeInfo(pipelineIdx)->modeIndex;
                                }
                                else
                                {
                                    CHX_LOG_ERROR("Sensor mode info is Null");
                                }

                                // Use setting TBM for final meta
                                ChiMetadata*                    pInputMetadata      = NULL;
                                if (NULL != pRequestData->pInputMetadata)
                                {
                                    pInputMetadata = pRequestData->pInputMetadata;
                                }

                                if (NULL != pInputMetadata)
                                {
                                    hInputMetadata = pInputMetadata->GetHandle();
                                }
                                else
                                {
                                    CHX_LOG_ERROR("Input Metadata is Null!");
                                }

                                UINT32 numInputImg   = 0;
                                UINT32 numInputMeta  = 0;
                                UINT32 numOutputImg  = 0;
                                UINT32 numOutputMeta = 0;
                                for (UINT32 inputIndex = 0; inputIndex < numInputs; inputIndex++)
                                {
                                    ChxUtils::Memset(&bufferMetaInfo, 0, sizeof(ChiFeature2BufferMetadataInfo));
                                    ChxUtils::Memset(&portKey, 0, sizeof(ChiFeature2Identifier));

                                    // Get feature input metadata info from request object
                                    bufferMetaInfo.hBuffer =
                                        pDependencyConfigInfo->inputConfig.phTargetBufferHandle[inputIndex];
                                    bufferMetaInfo.key     =
                                        pDependencyConfigInfo->inputConfig.pKey[inputIndex];

                                    if (ChiFeature2PortType::MetaData ==
                                        pDependencyConfigInfo->inputConfig.pPortDescriptor[inputIndex].globalId.portType)
                                    {
                                        numInputMeta++;
                                        hasInputMetadataPort = TRUE;
                                    }
                                    else
                                    {
                                        // Populate input buffer info
                                        result = GetStreamBuffer(bufferMetaInfo.hBuffer,
                                                                 bufferMetaInfo.key,
                                                                 &ppInputBuffers[pipelineIdx][numInputImg]);

                                        if (CDKResultSuccess == result)
                                        {
                                            // Query stream information from port data and update stream information
                                            portKey = pDependencyConfigInfo->inputConfig.pPortDescriptor[inputIndex].globalId;
                                            pPortData = GetPortData(&portKey);
                                            if (NULL != pPortData)
                                            {
                                                ppInputBuffers[pipelineIdx][numInputImg].pStream =
                                                    pPortData->pTarget->pChiStream;
                                            }
                                            else
                                            {
                                                CHX_LOG_ERROR("GetPortData returned NULL");
                                                result = CDKResultEInvalidPointer;
                                                break;
                                            }
                                            numInputImg++;

                                            pFrameData->pInputPorts.push_back(portKey);
                                        }
                                        else
                                        {
                                            CHX_LOG_ERROR("GetStreamBuffer() failed for port %s",
                                                pDependencyConfigInfo->inputConfig.pPortDescriptor[inputIndex].pPortName);
                                            break;
                                        }
                                    }
                                }

                                if (CDKResultSuccess == result)
                                {
                                    for (UINT32 outputIndex = 0; outputIndex < numOutputs; outputIndex++)
                                    {
                                        auto& rOutputConfig   = pDependencyConfigInfo->outputConfig;
                                        auto* pPortDescriptor = rOutputConfig.pPortDescriptor;
                                        ChxUtils::Memset(&bufferMetaInfo, 0, sizeof(ChiFeature2BufferMetadataInfo));
                                        ChxUtils::Memset(&portKey, 0, sizeof(ChiFeature2Identifier));

                                        // Get output metadata info from request object
                                        bufferMetaInfo.hBuffer = rOutputConfig.phTargetBufferHandle[outputIndex];
                                        bufferMetaInfo.key     = rOutputConfig.pKey[outputIndex];
                                        portKey                = pPortDescriptor[outputIndex].globalId;
                                        pPortData              = GetPortData(&portKey);

                                        const BOOL isMetadata =
                                            (ChiFeature2PortType::MetaData == pPortDescriptor[outputIndex].globalId.portType);
                                        if (TRUE == isMetadata)
                                        {
                                            numOutputMeta++;
                                            hasOutputMetadataPort = TRUE;
                                            result = GetMetadataBuffer(bufferMetaInfo.hBuffer,
                                                bufferMetaInfo.key, &hOutputMetadata);

                                            if (CDKResultSuccess != result)
                                            {
                                                CHX_LOG_ERROR("GetMetadataBuffer() failed with result=%d", result);
                                            }
                                        }
                                        else
                                        {
                                            // Populate output buffer info
                                            result = GetStreamBuffer(bufferMetaInfo.hBuffer,
                                                                     bufferMetaInfo.key,
                                                                     &ppOutputBuffers[pipelineIdx][numOutputImg]);

                                            // Query stream information from port data and update stream information
                                            if (NULL != pPortData)
                                            {
                                                if (NULL != pPortData->pTarget)
                                                {
                                                    ppOutputBuffers[pipelineIdx][numOutputImg].pStream =
                                                        pPortData->pTarget->pChiStream;
                                                }
                                            }
                                            else
                                            {
                                                CHX_LOG_ERROR("pPortData is NULL");
                                            }

                                        }

                                        pFrameData->pOutputPorts.push_back(portKey);
                                        if (FALSE == isMetadata)
                                        {
                                            numOutputImg++;
                                        }

                                        if (CDKResultSuccess != result)
                                        {
                                            UINT8 port = (NULL == pPortData) ? 0xFF : pPortData->globalId.port;
                                            CHX_LOG_ERROR("%s - Error configuring output port: %u - Code: %u",
                                                          pPipeline->GetPipelineName(),
                                                          port,
                                                          result);
                                            break;
                                        }
                                    }

                                    numInputs = numInputImg;
                                    pRequest[pipelineIdx].numInputs         = numInputs;
                                    pRequest[pipelineIdx].pInputBuffers     = ppInputBuffers[pipelineIdx];

                                    numOutputs = numOutputImg;
                                    pRequest[pipelineIdx].numOutputs        = numOutputs;
                                    pRequest[pipelineIdx].pOutputBuffers    = ppOutputBuffers[pipelineIdx];

                                    if (NULL != hInputMetadata)
                                    {
                                        pRequest[pipelineIdx].pInputMetadata = hInputMetadata;
                                    }

                                    if (NULL != hOutputMetadata)
                                    {
                                        pRequest[pipelineIdx].pOutputMetadata = hOutputMetadata;
                                    }

                                    pCombinedCallback->numCallbackData          = 1;
                                    pCombinedCallback->pCombinedCallbackData[0] = &pFrameData[0];

                                    pRequest[pipelineIdx].pPrivData = pCombinedCallback;
                                }
                            }
                            else
                            {
                                CHX_LOG_ERROR("No memory: numInputs = %d, numOutputs = %d",
                                    numInputs, numOutputs);
                                result = CDKResultENoMemory;
                                break;
                            }

                            // If something went wrong with populating the frame data, make sure we populate the output ports
                            // so that we can attempt to send back a request error properly
                            if ((CDKResultSuccess != result) &&
                                (pDependencyConfigInfo->numEnabledOutputPorts != pFrameData->pOutputPorts.size()))
                            {
                                pFrameData->pOutputPorts.clear();

                                for (UINT32 outputIndex = 0; outputIndex < pDependencyConfigInfo->numEnabledOutputPorts;
                                     outputIndex++)
                                {
                                    auto& rOutputConfig = pDependencyConfigInfo->outputConfig;
                                    auto* pPortDescriptor = rOutputConfig.pPortDescriptor;

                                    if (NULL != pPortDescriptor)
                                    {
                                        pFrameData->pOutputPorts.push_back(pPortDescriptor[outputIndex].globalId);
                                    }
                                    else
                                    {
                                        CHX_LOG_ERROR("%s pPortDescriptor is NULL, pFrameData->pOutputPorts aren't populated "
                                                      "correctly! Expect issues with request error!",
                                                      pRequestObject->IdentifierString());
                                    }
                                }
                            }
                        }

                        UINT8                requestId = pRequestObject->GetCurRequestId();
                        ChiFeature2StageInfo stageInfo = { 0 };
                        pRequestObject->GetCurrentStageInfo(&stageInfo, requestId);

                        UINT   appFrameNum = pRequestObject->GetUsecaseRequestObject()->GetAppFrameNumber();
                        UINT64 chiFrameNum = pRequest[pipelineIdx].frameNumber;
                        UINT32 numOutputs  = pRequest[pipelineIdx].numOutputs;
                        auto   hFroHandle  = pRequestObject->GetHandle();
                        auto   hSession    = pSession->GetSessionHandle();
                        auto   hPipeline   = pPipeline->GetPipelineHandle();
                        BINARY_LOG(LogEvent::FT2_Base_SubmitSessionRequest, appFrameNum, chiFrameNum, numOutputs, hFroHandle,
                            hSession, hPipeline, requestId, stageInfo);
                        CHX_LOG_INFO("%s Submitting Request:%" PRIu64 " with numOutputs:%d",
                                     pRequestObject->IdentifierString(),
                                     pRequest[pipelineIdx].frameNumber,
                                     numOutputs);

                        if (CHX_IS_VERBOSE_ENABLED())
                        {
                            ATRACE_ASYNC_BEGIN(pRequestObject->IdentifierString(), pRequest[pipelineIdx].frameNumber);
                        }
                    }
                }

                if (CDKResultSuccess == result)
                {
                    // Create the session request.

                    pPipelineRequest->pSessionHandle        = reinterpret_cast<CHIHANDLE>(pSession->GetSessionHandle());
                    pPipelineRequest->numRequests           = numPipelines;
                    pPipelineRequest->pCaptureRequests      = &pRequest[0];

                    if (NULL != pPipeline)
                    {
                        BOOL enableEarlyPCR = FALSE;
                        if (NULL != pRequestObject->GetFeatureHint())
                        {
                            if ((0 != pRequestObject->GetFeatureHint()->numEarlyPCRs) &&
                                (TRUE == pPipeline->IsRealTime()))
                            {
                                enableEarlyPCR = TRUE;
                            }
                        }

                        if (FALSE == pSession->IsPipelineActive())
                        {
                            if (TRUE == enableEarlyPCR)
                            {
                                // If early PCR is enabled first request needs to be submitted from app context
                                if ((CDKResultSuccess == result) &&
                                    (FALSE == static_cast<BOOL>(ChxUtils::AtomicLoadU32(&pSessionData->isFlushInProgress))))
                                {
                                    SendSubmitRequestMessage(pPipelineRequest);
                                }
                                else
                                {
                                    CHX_LOG_WARN("Request: %" PRIu64 " has been dropped as flush is in progress",
                                        pPipelineRequest->pCaptureRequests->frameNumber);
                                    // Clean up allocations
                                    for (UINT32 requestIndex = 0; requestIndex < pPipelineRequest->numRequests; ++requestIndex)
                                    {
                                        ChiCaptureRequest* pRequest =
                                            // NOWHINE CP036a: Need to free the object
                                            const_cast<ChiCaptureRequest*>(&pPipelineRequest->pCaptureRequests[requestIndex]);

                                        if (NULL != pRequest)
                                        {
                                            if (NULL != pRequest->pInputBuffers)
                                            {
                                                CHX_FREE(pRequest->pInputBuffers);
                                                pRequest->pInputBuffers = NULL;
                                            }
                                            if (NULL != pRequest->pOutputBuffers)
                                            {
                                                CHX_FREE(pRequest->pOutputBuffers);
                                                pRequest->pOutputBuffers = NULL;
                                            }
                                            CHX_FREE(pRequest);
                                            pRequest = NULL;
                                        }
                                    }
                                    CHX_FREE(pPipelineRequest);
                                    pPipelineRequest = NULL;
                                }
                                result = ExtensionModule::GetInstance()->ActivatePipeline(pSession->GetSessionHandle(),
                                    pPipeline->GetPipelineHandle());
                                if (CDKResultSuccess == result)
                                {
                                    pPipeline->SetPipelineActivateFlag();
                                }
                            }
                            else
                            {
                                result = ExtensionModule::GetInstance()->ActivatePipeline(pSession->GetSessionHandle(),
                                    pPipeline->GetPipelineHandle());
                                if (CDKResultSuccess == result)
                                {
                                    pPipeline->SetPipelineActivateFlag();
                                    result = OnSubmitRequestToSession(pPipelineRequest);
                                }
                            }
                        }
                        else
                        {
                            if ((CDKResultSuccess == result) &&
                                (FALSE == static_cast<BOOL>(ChxUtils::AtomicLoadU32(&pSessionData->isFlushInProgress))))
                            {
                                result = OnSubmitRequestToSession(pPipelineRequest);
                            }
                            else
                            {
                                CHX_LOG_WARN("Request: %" PRIu64 " has been dropped as flush is in progress",
                                        pPipelineRequest->pCaptureRequests->frameNumber);
                                result = CDKResultECancelledRequest;

                                // Clean up allocations
                                for (UINT32 requestIndex = 0; requestIndex < pPipelineRequest->numRequests; ++requestIndex)
                                {
                                    ChiCaptureRequest* pRequest =
                                        // NOWHINE CP036a: Need to free the object
                                        const_cast<ChiCaptureRequest*>(&pPipelineRequest->pCaptureRequests[requestIndex]);

                                    if (NULL != pRequest)
                                    {
                                        if (NULL != pRequest->pInputBuffers)
                                        {
                                            CHX_FREE(pRequest->pInputBuffers);
                                            pRequest->pInputBuffers = NULL;
                                        }
                                        if (NULL != pRequest->pOutputBuffers)
                                        {
                                            CHX_FREE(pRequest->pOutputBuffers);
                                            pRequest->pOutputBuffers = NULL;
                                        }
                                        CHX_FREE(pRequest);
                                        pRequest = NULL;
                                    }
                                }
                                CHX_FREE(pPipelineRequest);
                                pPipelineRequest = NULL;
                            }
                        }

                        if ((CDKResultSuccess != result) && (CDKResultECancelledRequest != result))
                        {
                            CHX_LOG_ERROR("Submit request: %d failure for pipeline:%s",
                                pRequestData->frameNumber, pPipeline->GetPipelineName());
                            break;
                        }
                        else
                        {
                            // Here store sequence Id for all ports after request submitted. Then in GetBufferData function,
                            // get internal input buffer data according to the available sequecence Id.
                            for (UINT8 portIndex = 0; portIndex < pFrameData->pOutputPorts.size(); portIndex++)
                            {
                                pRequestObject->UpdatePortIdtoSeqIdVector(pFrameData->pOutputPorts[portIndex],
                                    pRequestData->frameNumber);
                            }
                        }
                    }
                }

                if (CDKResultSuccess != result)
                {
                    ChiFeatureSequenceData* pRequestData = static_cast<ChiFeatureSequenceData*>(
                    pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current, pRequestObject->GetCurRequestId()));

                    CHIMESSAGEDESCRIPTOR messageDescriptor                      = {};
                    messageDescriptor.message.errorMessage.errorMessageCode     = MessageCodeRequest;
                    messageDescriptor.message.errorMessage.frameworkFrameNum
                        = ((NULL != pRequestData) ? pRequestData->frameNumber : 0);
                    messageDescriptor.message.errorMessage.pErrorStream         = NULL;
                    messageDescriptor.messageType                               = ChiMessageType::ChiMessageTypeError;
                    messageDescriptor.pPrivData                                 = ((NULL != pRequestData) ?
                                                                                   &(pRequestData->frameCbData) : NULL);
                    if (NULL != pFrameData)
                    {
                        HandleRequestError(&messageDescriptor, pFrameData);
                        pFrameData = NULL;
                    }
                }
            }
            else
            {
                CHX_LOG_ERROR("No memory, numPipelines = %d", numPipelines);
            }
            if (NULL != ppInputBuffers)
            {
                CHX_FREE(ppInputBuffers);
                ppInputBuffers = NULL;
            }
            if (NULL != ppOutputBuffers)
            {
                CHX_FREE(ppOutputBuffers);
                ppOutputBuffers = NULL;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetCurrentStageInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::GetCurrentStageInfo(
    ChiFeature2RequestObject* pRequestObject,
    ChiFeature2StageInfo*     pStageInfo
    ) const
{
    return pRequestObject->GetCurrentStageInfo(pStageInfo, pRequestObject->GetCurRequestId());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetSensorModeIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiFeature2Base::GetSensorModeIndex(
    const ChiFeature2Identifier*    pMetadataPortId
    ) const
{
    UINT32 modeIndex = 0;

    ChiFeatureSessionData* pSessionData = GetSessionData(pMetadataPortId);

    if (NULL != pSessionData)
    {
        if (NULL != pSessionData->pSession)
        {
            if (NULL != pSessionData->pSession->GetSensorModeInfo())
            {
                modeIndex = pSessionData->pSession->GetSensorModeInfo()->modeIndex;
            }
            else
            {
                CHX_LOG_ERROR("Sensor mode info is Null");
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid session Data");
    }

    return modeIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ThreadCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2Base::ThreadCallback(
    VOID* pCallbackData)
{
    ChiFeature2RequestObject*     pRequestObject  = reinterpret_cast<ChiFeature2RequestObject*>(pCallbackData);
    ChiFeature2Base*              pFeature        = NULL;
    CDKResult                     result          = CDKResultSuccess;

    ChiFeature2RequestState       curRequestState = ChiFeature2RequestState::InvalidMax;
    if (NULL != pRequestObject)
    {


        for (UINT8 curRequestId = 0; curRequestId < pRequestObject->GetNumRequests(); ++curRequestId)
        {
            curRequestState = pRequestObject->GetCurRequestState(curRequestId);

            ChiFeature2InputDependencyStatus    dependencyStatus        =
                pRequestObject->AreInputDependenciesStatisfied(ChiFeature2SequenceOrder::Current, curRequestId);

            // Identify whose dependency has been met and call accordingly
            if (ChiFeature2RequestState::InputResourcePendingScheduled == curRequestState)
            {
                if (ChiFeature2InputDependencyStatus::inputDependenciesSatisfied          == dependencyStatus ||
                    ChiFeature2InputDependencyStatus::inputDependenciesSatisfiedWithError == dependencyStatus ||
                    ChiFeature2InputDependencyStatus::inputDependenciesErroredOut         == dependencyStatus)
                {
                    pFeature = pRequestObject->GetFeature();
                    if (NULL != pFeature)
                    {
                        pFeature->ReleaseDependenciesOnInputResourcePending(pRequestObject, curRequestId);
                        pRequestObject->SetCurRequestState(ChiFeature2RequestState::ReadyToExecute, curRequestId);
                        result = pFeature->OnProcessRequest(pRequestObject, curRequestId);
                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("%s Failed to process request", pRequestObject->IdentifierString());
                        }
                        else
                        {
                            // For stages that have multiple sequences, we need to notify the graph of the dependency for the
                            // next stage sequence
                            curRequestState = pRequestObject->GetCurRequestState(curRequestId);
                            if (ChiFeature2RequestState::InputResourcePending == curRequestState)
                            {
                                result = pFeature->ProcessDependency(pRequestObject);
                                if (CDKResultSuccess != result)
                                {
                                    CHX_LOG_ERROR("%s Failed to Process dependency!", pRequestObject->IdentifierString());
                                }
                            }
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("%s Failed to get feature", pRequestObject->IdentifierString());
                    }
                    break;
                }
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Failed to get requestObject");
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessInFlightBufferCallBack
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ProcessInFlightBufferCallBack(
    CHITARGETBUFFERINFOHANDLE  hBuffer,
    VOID*                      pCallbackdata)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pCallbackdata)
    {
        ChiFeature2InFlightCallbackData* pInflightCallback = static_cast<ChiFeature2InFlightCallbackData*>(pCallbackdata);
        const ChiFeature2Base*           pFeatureInstance  = pInflightCallback->pFeatureInstance;
        ChiFeaturePipelineData*          pPipelineData     = pFeatureInstance->GetPipelineData(&pInflightCallback->portId);
        ChiFeaturePortData*              pPortData         = pFeatureInstance->GetPortData(&pInflightCallback->portId);
        UINT64                           key               = 0;

        if (NULL != pPipelineData && NULL != pPortData)
        {
            if (ChiFeature2PortType::ImageBuffer == pPortData->globalId.portType)
            {
                key = reinterpret_cast<UINT64>(pPortData->pTarget->pChiStream);
            }
            else
            {
                key = pPipelineData->metadataClientId;
            }
            pFeatureInstance->OnInflightBufferCallback(pInflightCallback->pRequestObject,
                &pPortData->globalId, hBuffer, key);
        }
        else
        {
            CHX_LOG_ERROR("PipelineData %p or PortData is NULL %p", pPipelineData, pPortData);
        }
    }
    else
    {
        CHX_LOG_ERROR("Null callback data");
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ValidateRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ValidateRequest(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult                               result              = CDKResultSuccess;
    UINT8                                   requestId           = 0;
    UINT8                                   numOutputsRequested = -1;
    const ChiFeature2RequestOutputInfo*     pRequestOutputInfo  = NULL;
    ChiFeature2BufferMetadataInfo*          pBufferMetaInfo     = NULL;

    if (NULL == pRequestObject)
    {
        CHX_LOG_ERROR("Invalid argument: NULL pRequestObject");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        requestId = pRequestObject->GetCurRequestId();
        result    = pRequestObject->GetExternalRequestOutput(&numOutputsRequested, &pRequestOutputInfo, requestId);

        if (CDKResultSuccess == result)
        {
            // Check if we have valid pipeline target and target buffer manager mapped to this port
            // If not, mark the buffer as skipped, and will return as error buffer during process result path
            for (UINT i = 0; i < numOutputsRequested; i++)
            {
                ChiFeaturePipelineData* pPipelineData = GetPipelineData(&(pRequestOutputInfo[i].pPortDescriptor->globalId));
                ChiFeaturePortData*     pPortData     = GetPortData(&(pRequestOutputInfo[i].pPortDescriptor->globalId));

                if ((NULL != pPipelineData)      &&
                    (NULL != pPortData)          &&
                    (NULL == pPortData->pTarget) &&
                    (NULL == pPortData->pOutputBufferTbm))
                {
                    if (NULL != pRequestOutputInfo[i].bufferMetadataInfo.hBuffer)
                    {
                        result = pRequestObject->GetFinalBufferMetadataInfo(pPortData->globalId, &pBufferMetaInfo, requestId);

                        if ((CDKResultSuccess == result) && (NULL != pBufferMetaInfo))
                        {
                            pBufferMetaInfo->bufferSkipped = TRUE;

                            CHX_LOG_VERBOSE("mark framework buffer as skipped for pipeline:%s, port[%d]:%s",
                                pPipelineData->pPipelineName, i, pPortData->pPortName);
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("No TBM for port with CHI internal output buffer.");
                        result = CDKResultEInvalidArg;
                        break;
                    }
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::HandlePrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::HandlePrepareRequest(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult result = CDKResultSuccess;

    if (0 == pRequestObject->GetCurRequestId())
    {
        result = ValidateRequest(pRequestObject);

        if (CDKResultSuccess == result)
        {
            result = InitializeFeatureContext(pRequestObject);

            if (CDKResultSuccess == result)
            {
                result = OnPrepareRequest(pRequestObject);
            }
        }
    }

    if (CDKResultSuccess == result)
    {
        // The request object is scheduled for execution for the first time after creation.
        // Call Prepare request only once per request

        if ((CDKResultSuccess == result) && (NULL != pRequestObject))
        {
            // Move the state to "ReadyToExecute".
            result = pRequestObject->SetCurRequestState(ChiFeature2RequestState::ReadyToExecute,
                                                        pRequestObject->GetCurRequestId());
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("SetCurRequestState to ReadyToExecute returned error: %d", result);
            }
        }
        else
        {
            CHX_LOG_ERROR("OnPrepareRequest returned error: %d", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::HandleExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::HandleExecuteProcessRequest(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult                           result                  = CDKResultSuccess;
    INT32                               nextProcessSequenceId   = InvalidProcessSequenceId;
    UINT8                               requestId               = pRequestObject->GetCurRequestId();
    ChiFeature2InputDependencyStatus    dependencyStatus        =
        pRequestObject->AreInputDependenciesStatisfied(ChiFeature2SequenceOrder::Current, requestId);
    BOOL                                bSkipExecution          = FALSE;

    if (ChiFeature2RequestState::OutputErrorNotificationPending != pRequestObject->GetCurRequestState(requestId))
    {
        if ((dependencyStatus == ChiFeature2InputDependencyStatus::inputDependenciesErroredOut) ||
            (dependencyStatus == ChiFeature2InputDependencyStatus::inputDependenciesSatisfiedWithError))
        {
            if (FALSE == CanRequestContinueWithError())
            {
                PropagateError(pRequestObject);
                bSkipExecution  = TRUE;
                result          = CDKResultENoMore;
            }
        }
    }

    if (FALSE == bSkipExecution)
    {
        // Set the request into "Executing" state if the request is in good standing
        if ((CDKResultSuccess == result) &&
            (ChiFeature2RequestState::Executing != pRequestObject->GetCurRequestState(requestId)))
        {
            result = pRequestObject->SetCurRequestState(ChiFeature2RequestState::Executing, requestId);
        }

        if (CDKResultSuccess == result)
        {
            result = OnExecuteProcessRequest(pRequestObject);
            if (CDKResultSuccess == result)
            {
                nextProcessSequenceId = pRequestObject->GetProcessSequenceId(ChiFeature2SequenceOrder::Next, requestId);

                if (InvalidProcessSequenceId != nextProcessSequenceId)
                {
                    result = pRequestObject->MoveToNextProcessSequenceInfo(requestId);

                    result = InitializeSequenceData(pRequestObject);

                    if (CDKResultSuccess == result)
                    {
                        // This function will move the request state to input resource pending if there are
                        // external dependencies, otherwise we remain in executing state and continue execution
                        result = GetDependency(pRequestObject);
                        if (CDKResultSuccess != result)
                        {
                            if (FALSE == CanRequestContinueWithError())
                            {
                                PropagateError(pRequestObject);
                                result = CDKResultENoMore;
                            }
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("InitializeSequenceData(pRequestObject) returned error: %d", result);
                    }
                }
                else
                {
                    if ((ChiFeature2RequestState::OutputNotificationPending !=pRequestObject->GetCurRequestState(requestId)) &&
                        (ChiFeature2RequestState::OutputErrorNotificationPending !=
                         pRequestObject->GetCurRequestState(requestId)) &&
                            (ChiFeature2RequestState::Complete != pRequestObject->GetCurRequestState(requestId)))
                    {
                        result = pRequestObject->SetCurRequestState(ChiFeature2RequestState::OutputResourcePending, requestId);
                    }
                }
            }
            else
            {
                CDKResult resultState = CDKResultSuccess;
                resultState = pRequestObject->SetCurRequestState(ChiFeature2RequestState::InvalidMax, requestId);
                if (CDKResultSuccess != resultState)
                {
                    CHX_LOG_ERROR("Request state change failed");
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("OnExecuteProcessRequest returned error: %d", result);
            result = pRequestObject->SetCurRequestState(ChiFeature2RequestState::InvalidMax, requestId);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::HandleInputResourcePending
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::HandleInputResourcePending(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId)
{
    CDKResult                        result                 = CDKResultSuccess;
    ChiFeature2InputDependencyStatus inputDependencyStatus  =
        pRequestObject->AreInputDependenciesStatisfied(ChiFeature2SequenceOrder::Current, requestId);

    OnInputResourcePending(pRequestObject);

    CHX_LOG_INFO("%s requestState:%s, requestId:%d",
        pRequestObject->IdentifierString(),
        ChiFeature2RequestStateStrings[static_cast<UINT8>(pRequestObject->GetCurRequestState(requestId))],
        requestId);

    switch (inputDependencyStatus)
    {
        case ChiFeature2InputDependencyStatus::inputDependenciesSatisfied:
        case ChiFeature2InputDependencyStatus::inputDependenciesSatisfiedWithError:
        case ChiFeature2InputDependencyStatus::inputDependenciesErroredOut:
        {
            pRequestObject->LockRequestObject();
            ChiFeature2RequestState requestState = pRequestObject->GetCurRequestState(requestId);

            // We only want to post a job once, set ourselves to IRP with job scheduled, multiple threads can
            // get here in IRP state
            if (ChiFeature2RequestState::InputResourcePending == requestState)
            {
                result = pRequestObject->SetCurRequestState(ChiFeature2RequestState::InputResourcePendingScheduled, requestId);
                if (CDKResultSuccess == result)
                {
                    result = HandlePostJob(pRequestObject, requestId);
                }
            }
            pRequestObject->UnlockRequestObject();

            break;
        }
        case ChiFeature2InputDependencyStatus::inputDependenciesNotSatisfied:
            // Do nothing
            break;
        default:
            CHX_LOG_WARN("Hit unhandled case:%d", inputDependencyStatus);
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnInputResourcePending
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnInputResourcePending(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::HandleOutputNotificationPending
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::HandleOutputNotificationPending(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId)
{
    CDKResult                               result                    = CDKResultSuccess;
    BOOL                                    allPortsReleased          = FALSE;
    UINT8                                   numRequestOutputs         = 0;
    const ChiFeature2RequestOutputInfo*     pRequestOutputInfo        = NULL;

    result = pRequestObject->GetExternalRequestOutput(&numRequestOutputs, &pRequestOutputInfo, requestId);

    OnReleaseInputDependency(pRequestObject, requestId);

    if (0 == numRequestOutputs || NULL == pRequestOutputInfo)
    {
        CHX_LOG_ERROR("No output ports requested numOutput %d pRequestOutputInfo %p", numRequestOutputs, pRequestOutputInfo);
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT8 portIndex = 0; portIndex < numRequestOutputs; ++portIndex)
        {
            const ChiFeature2PortDescriptor*       pPortDescriptor = pRequestOutputInfo[portIndex].pPortDescriptor;
            const ChiFeature2BufferMetadataInfo*   pBufferMetaInfo = &pRequestOutputInfo[portIndex].bufferMetadataInfo;

            if (NULL != pPortDescriptor && NULL != pBufferMetaInfo)
            {
                BOOL    isPortNotified = pRequestObject->GetOutputNotifiedForPort(pPortDescriptor->globalId, requestId);
                BOOL    isPortReleased = pRequestObject->GetReleaseAcknowledgedForPort(pPortDescriptor->globalId, requestId);

                if (TRUE == isPortNotified && FALSE == isPortReleased)
                {
                    // Release buffers
                    CHITargetBufferManager* pManager = CHITargetBufferManager::GetTargetBufferManager(
                                                                                pBufferMetaInfo->hBuffer);
                    if ((NULL != pManager) &&
                        (FALSE == pBufferMetaInfo->bufferSkipped))
                    {
                        result = pManager->ReleaseTargetBuffer(pBufferMetaInfo->hBuffer);
                    }
                    else
                    {
                        CHX_LOG_WARN("%s TBM handle is null GID[Session:%d Pipeline:%d Port:%d Type:%d] reqIdx:%d",
                        pRequestObject->IdentifierString(),
                        pPortDescriptor->globalId.session,
                        pPortDescriptor->globalId.pipeline,
                        pPortDescriptor->globalId.port,
                        pPortDescriptor->globalId.portType,
                        requestId);
                    }
                    pRequestObject->SetReleaseAcknowledgedForPort(pPortDescriptor->globalId, requestId);
                }
            }
            else
            {
                CHX_LOG_ERROR("Invalid info in output index %d pPortDescriptor %p pBufferMetaInfo %p", portIndex,
                                                                                           pPortDescriptor, pBufferMetaInfo);
                result = CDKResultEFailed;
                break;
            }
        }

        if (TRUE == pRequestObject->AreOutputsReleased(requestId))
        {
            result = CompleteRequest(pRequestObject, requestId);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnInflightBufferCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnInflightBufferCallback(
    ChiFeature2RequestObject*        pRequestObject,
    ChiFeature2Identifier*           pPortId,
    CHITARGETBUFFERINFOHANDLE        hBuffer,
    UINT64                           key
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    CDK_UNUSED_PARAM(pPortId);
    CDK_UNUSED_PARAM(hBuffer);
    CDK_UNUSED_PARAM(key);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnReleaseInputDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnReleaseInputDependency(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    CDK_UNUSED_PARAM(requestId);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::HandleOutputResourcePending
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::HandleOutputResourcePending(
    ChiFeature2RequestObject* pRequestObject)
{
    CDK_UNUSED_PARAM(pRequestObject);
    CDKResult result = CDKResultSuccess;
    // Move the state to "OutputNotificationPending".
    result = pRequestObject->SetCurRequestState(ChiFeature2RequestState::OutputNotificationPending,
        pRequestObject->GetCurRequestId());
    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("SetCurRequestState to Complete returned error: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::CompleteRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::CompleteRequest(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId)
{
    CDKResult   result = CDKResultSuccess;
    BOOL        allRequestsComplete = TRUE;

    // Move the state to "Complete".

    if ((ChiFeature2RequestState::OutputNotificationPending      == pRequestObject->GetCurRequestState(requestId)) ||
        (ChiFeature2RequestState::OutputErrorNotificationPending == pRequestObject->GetCurRequestState(requestId)))
    {
        result = pRequestObject->SetCurRequestState(ChiFeature2RequestState::Complete, requestId);

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("SetCurRequestState to Complete returned error: %d", result);
        }
        else
        {
            for (UINT8 requestId = 0 ; requestId < pRequestObject->GetNumRequests(); ++requestId)
            {
                if (ChiFeature2RequestState::Complete != pRequestObject->GetCurRequestState(requestId))
                {
                    allRequestsComplete = FALSE;
                    break;
                }
            }

            if (TRUE == allRequestsComplete)
            {
                CHX_LOG_INFO("%s All %d requests complete, initiate cleanup", pRequestObject->IdentifierString(),
                    pRequestObject->GetNumRequests());

                pRequestObject->LockRequestObject();
                if (NULL != pRequestObject->GetPrivContext())
                {
                    DoCleanupRequest(pRequestObject);
                    ChiFeatureRequestContext * pRequestContext =
                        static_cast<ChiFeatureRequestContext*>(pRequestObject->GetPrivContext());
                    if (NULL != pRequestContext)
                    {
                        if (0 != pRequestContext->pPortZSLQueues.size())
                        {
                            PortZSLQueueData* pZSLQueueData  = NULL;
                            for (UINT8 index = 0 ; index < pRequestContext->pPortZSLQueues.size(); ++index)
                            {
                                pZSLQueueData  = pRequestContext->pPortZSLQueues.at(index);
                                for (UINT8 zslIndex = 0; zslIndex < pZSLQueueData->selectedZSLFrames.size(); ++zslIndex)
                                {
                                    // clear the call incase still set from TBM
                                    ChiFeature2Identifier    key            = pZSLQueueData->globalId;
                                    ChiFeaturePortData*      pPortData      = GetPortData(&key);
                                    if ((NULL != pPortData) && (NULL != pPortData->pOutputBufferTbm))
                                    {
                                        CHITargetBufferManager*  pBufferManager = pPortData->pOutputBufferTbm;
                                        pBufferManager->RemoveCallbackFromTarget(pZSLQueueData->selectedZSLFrames.at(zslIndex));
                                    }
                                }
                                CHX_DELETE(pRequestContext->pPortZSLQueues.at(index));
                            }
                        }
                        pRequestContext->pPortZSLQueues.clear();
                        pRequestContext->pPortZSLQueues.shrink_to_fit();
                        CHX_FREE(pRequestContext);
                        pRequestContext = NULL;
                        pRequestObject->SetPrivContext(NULL);
                    }
                }
                pRequestObject->UnlockRequestObject();
            }
        }
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PropagateError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::PropagateError(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult                       result              = CDKResultSuccess;
    UINT8                           requestId           = pRequestObject->GetCurRequestId();
    ChiFeature2Notification         chiNotification     = {0};
    CHIMESSAGEDESCRIPTOR            messageDescriptor   = {};
    ChiFeature2Messages             featureMessage      = {m_featureId, chiNotification, NULL};
    ChiFeature2BufferMetadataInfo*  pBufferMetaInfo     = NULL;
    ChiFeatureSequenceData*         pRequestData        = static_cast<ChiFeatureSequenceData*>(
        pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current, requestId));

    messageDescriptor.messageType = ChiMessageTypeError;

    if (NULL != pRequestData)
    {
        messageDescriptor.message.errorMessage.frameworkFrameNum = pRequestData->frameNumber;
    }
    else
    {
        CHX_LOG_ERROR("%s request data is NULL, unable to set frame number for error message",
                     pRequestObject->IdentifierString());
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        pRequestObject->SetCurRequestState(ChiFeature2RequestState::OutputErrorNotificationPending, requestId);

        // Assume at this point there are no valid buffer notification which needs to be sent and thereby
        // send notification for all the ports that are errored and to be marked as error
        std::vector<const ChiFeature2Identifier*> pGlobalIdentifierVector =
            pRequestObject->GetFinalGlobalIdentifiers(requestId);

        for (UINT8 index = 0; index < pGlobalIdentifierVector.size(); index++)
        {
            const ChiFeature2Identifier*    pGlobalIdentifier   = pGlobalIdentifierVector[index];
            ChiFeaturePortData*             pPortData           = GetPortData(pGlobalIdentifier);

            if (NULL != pGlobalIdentifier)
            {
                pRequestObject->GetFinalBufferMetadataInfo(*pGlobalIdentifier, &pBufferMetaInfo, requestId);

                if (NULL != pBufferMetaInfo)
                {
                    pBufferMetaInfo->bufferErrorPresent = TRUE;

                    if (ChiFeature2PortType::MetaData == pGlobalIdentifier->portType)
                    {
                        messageDescriptor.message.errorMessage.errorMessageCode = MessageCodeResult;
                        messageDescriptor.message.errorMessage.pErrorStream     = NULL;
                    }
                    else if (ChiFeature2PortType::ImageBuffer == pGlobalIdentifier->portType)
                    {
                        messageDescriptor.message.errorMessage.errorMessageCode = MessageCodeBuffer;
                        if (NULL != pPortData)
                        {
                            if (NULL != pPortData->pTarget)
                            {
                                messageDescriptor.message.errorMessage.pErrorStream = pPortData->pTarget->pChiStream;
                            }
                            else
                            {
                                CHX_LOG_ERROR("%s Unable to get the target for Error Stream, cannot propagate error",
                                              pRequestObject->IdentifierString());
                                result = CDKResultEFailed;
                            }
                        }
                        else
                        {
                            CHX_LOG_ERROR("%s Port data is NULL, cannot propagate error", pRequestObject->IdentifierString());
                            result = CDKResultEFailed;
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("%s final buffer metadata null, cannot propagate error",
                                  pRequestObject->IdentifierString());
                    result = CDKResultEFailed;
                }

                if (CDKResultSuccess == result)
                {
                    featureMessage.featureId                        = m_featureId;
                    featureMessage.chiNotification.pIdentifier      = pGlobalIdentifier;
                    featureMessage.chiNotification.pChiMessages     = &messageDescriptor;
                    featureMessage.chiNotification.requestIndex     = requestId;
                    featureMessage.pFeatureMessages                 = NULL;

                    CHX_LOG_INFO("%s Error being sent for [%d %d %d %d %d]",
                                 pRequestObject->IdentifierString(),
                                 featureMessage.chiNotification.pIdentifier->session,
                                 featureMessage.chiNotification.pIdentifier->pipeline,
                                 featureMessage.chiNotification.pIdentifier->port,
                                 featureMessage.chiNotification.pIdentifier->portDirectionType,
                                 featureMessage.chiNotification.pIdentifier->portType);
                    result = m_clientCallbacks.ChiFeature2ProcessMessage(pRequestObject, &featureMessage);
                }
            }
        }
    }
    CHX_LOG_INFO("%s Release Dependencies for requestID:[%d]",
        pRequestObject->IdentifierString(),
        requestId);
    ProcessReleaseDependency(pRequestObject, requestId, 0, NULL);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::Initialize(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    CDKResult result = CDKResultSuccess;

    result = ValidateFeatureDesc(pCreateInputInfo);

    if (CDKResultSuccess == result)
    {
        m_frameNumber       = 0;
        m_hFeatureJob       = 0;
        m_pInstanceProps    = pCreateInputInfo->pInstanceProps;
        m_pCameraInfo       = pCreateInputInfo->pCameraInfo;
        m_featureId         = pCreateInputInfo->pFeatureDescriptor->featureId;
        m_pFeatureName      = pCreateInputInfo->pFeatureDescriptor->pFeatureName;
        m_clientCallbacks   = *(pCreateInputInfo->pClientCallbacks);
        m_pMetadataManager  = pCreateInputInfo->pMetadataManager;
        m_pThreadManager    = pCreateInputInfo->pThreadManager;
        m_pSessionSettings  = pCreateInputInfo->pStreamConfig->pSessionSettings;
        m_useResManager     = pCreateInputInfo->bEnableResManager;

        ChiFeature2QueryInfo queryInfo = {0};

        result = OnQueryCaps(&queryInfo);
    }

    if (CDKResultSuccess == result)
    {
        result = ClassifyStream(pCreateInputInfo->pStreamConfig);
    }

    if (CDKResultSuccess == result)
    {
        PrepareFeatureData(pCreateInputInfo);
    }

    /// Create Sessions
    if (CDKResultSuccess == result)
    {
        result = OnInitialize(pCreateInputInfo);
    }

    /// Create internal Buffers
    if (CDKResultSuccess == result)
    {
        result = CreateTargetBufferManagers();
    }

    if (CDKResultSuccess == result)
    {
        result = PrepareStageData(pCreateInputInfo);
    }

    if (CDKResultSuccess == result)
    {
        result = InitializeThreadService();
    }

    // Take ownership of the streams the feature negotiated for, so they may be freed after the feature is destroyed.
    // There are some usecases, where Stream negotiation do not happen like unit test code or postproc feature.
    // Validate stream pointer before accessing.
    if ((NULL != pCreateInputInfo->pOwnedStreams) && (0 != pCreateInputInfo->pOwnedStreams->size()))
    {
        for (auto pStream : *(pCreateInputInfo->pOwnedStreams))
        {
            m_pStreams.push_back(pStream);
        }
    }
    else
    {
        CHX_LOG_VERBOSE("pOwnedStreams is NULL or size is 0");
    }

    // Copy CamX stream info to target streams
    for (UINT32 streamIndex = 0; streamIndex < m_pStreamData.size(); ++streamIndex)
    {
        *m_pStreamData[streamIndex].pTargetStream = *m_pStreamData[streamIndex].pStream;
    }

    // lock for synchronizing the result processing
    m_pProcessResultLock                     = Mutex::Create();

    // CreateThread for Flush
    m_pFlushThreadMutex                      = Mutex::Create();
    m_pFlushRequestAvailable                 = Condition::Create();
    m_flushRequestProcessTerminate           = FALSE;
    m_flushRequestProcessThread.pPrivateData = this;
    m_shouldWaitFlush                        = TRUE;
    m_destroyInProgress                      = FALSE;

    result = ChxUtils::ThreadCreate(ChiFeature2Base::RequestThread,
                                    &m_flushRequestProcessThread,
                                    &m_flushRequestProcessThread.hThreadHandle);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnQueryCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnQueryCaps(
    ChiFeature2QueryInfo* pQueryInfo)
{
    CDK_UNUSED_PARAM(pQueryInfo);

    CDKResult result = CDKResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnPrepareRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);

    CDKResult result = CDKResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::OnProcessMessage(
    const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
    VOID* pPrivateCallbackData)
{
    CDK_UNUSED_PARAM(pMessageDescriptor);
    CDK_UNUSED_PARAM(pPrivateCallbackData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::SetConfigInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::SetConfigInfo(
    ChiFeature2RequestObject* pRequestObject,
    UINT                      maxSequence
    ) const
{
    return pRequestObject->SetConfigInfo(maxSequence, m_numSessions, m_pSessionDescriptor);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::SetFeaturePrivContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::SetFeaturePrivContext(
    ChiFeature2RequestObject* pRequestObject,
    VOID* pClientData
    ) const
{
    CDKResult                 result          = CDKResultSuccess;
    ChiFeatureRequestContext* pFeatureContext = NULL;

    if (NULL == pRequestObject)
    {
        CHX_LOG_ERROR("Invalid argument: pRequestObject: %p", pRequestObject);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pFeatureContext = static_cast<ChiFeatureRequestContext*>(pRequestObject->GetPrivContext());

        if (NULL != pFeatureContext)
        {
            pFeatureContext->privContext.pPrivateData = pClientData;
        }
        else
        {
            CHX_LOG_ERROR("No valid Context available");
            result = CDKResultEInvalidArg;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetFeaturePrivContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2Base::GetFeaturePrivContext(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult                 result          = CDKResultSuccess;
    ChiFeatureRequestContext* pFeatureContext = NULL;
    VOID*                     pPrivateContext = NULL;

    if (NULL == pRequestObject)
    {
        CHX_LOG_ERROR("Invalid argument: pRequestObject: %p", pRequestObject);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pFeatureContext = static_cast<ChiFeatureRequestContext*>(pRequestObject->GetPrivContext());

        if (NULL != pFeatureContext)
        {
            pPrivateContext = pFeatureContext->privContext.pPrivateData;
        }
        else
        {
            CHX_LOG_ERROR("No valid Context available");
            result = CDKResultEInvalidArg;
        }
    }

    return pPrivateContext;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::IsPortEnabledInFinalOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Base::IsPortEnabledInFinalOutput(
    ChiFeature2RequestObject* pRequestObject,
    ChiFeature2Identifier     identifier,
    UINT8                     requestId
    ) const
{
    BOOL                                    isEnabled           = FALSE;
    UINT8                                   numRequest          = 0;
    const ChiFeature2RequestOutputInfo*     pRequestOutputInfo  = NULL;

    pRequestObject->GetExternalRequestOutput(&numRequest, &pRequestOutputInfo, requestId);

    if ((0 != numRequest) && (NULL != pRequestOutputInfo))
    {
        for (UINT8 portIndex = 0; portIndex < numRequest; ++portIndex)
        {
            const ChiFeature2PortDescriptor* pPortDescriptor = pRequestOutputInfo[portIndex].pPortDescriptor;
            if (NULL != pPortDescriptor)
            {
                if (pPortDescriptor->globalId == identifier)
                {
                    isEnabled = TRUE;
                }
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Unable to get any output from request object");
    }

    return isEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ValidateFeatureDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ValidateFeatureDesc(
    ChiFeature2CreateInputInfo* pCreateInputInfo
    ) const
{
    CDKResult          result       = CDKResultSuccess;
    const ChiUsecase*  pUsecaseDesc = NULL;

    if ((NULL == pCreateInputInfo) || (NULL == pCreateInputInfo->pClientCallbacks))
    {
        CHX_LOG_ERROR("Invalid argument: NULL pCreateInputInfo->pClientCallbacks %p", pCreateInputInfo);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        pUsecaseDesc = pCreateInputInfo->pUsecaseDescriptor;
        if (NULL == pUsecaseDesc)
        {
            CHX_LOG_ERROR("Invalid argument: NULL pCreateInputInfo->pUsecaseDescriptor");
            result = CDKResultEInvalidArg;
        }
    }

    if (CDKResultSuccess == result)
    {
        if (NULL == pCreateInputInfo->pCameraInfo)
        {
            CHX_LOG_ERROR("Invalid argument: NULL pCreateInputInfo->pCameraInfo");
            result = CDKResultEInvalidArg;
        }
    }

    if (CDKResultSuccess == result)
    {
        if (NULL == pCreateInputInfo->pFeatureDescriptor)
        {
            CHX_LOG_ERROR("Invalid argument: NULL pCreateInputInfo->pFeatureDescriptor");
            result = CDKResultEInvalidArg;
        }
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 sessionIdx = 0; sessionIdx < pCreateInputInfo->pFeatureDescriptor->numSessions; sessionIdx++)
        {
            const ChiFeature2Descriptor* pFeatureDesc = pCreateInputInfo->pFeatureDescriptor;
            if (sessionIdx != pFeatureDesc->pSession[sessionIdx].sessionId)
            {
                CHX_LOG_ERROR("Invalid argument: sessionIdx = %d: sessionId = %d",
                    sessionIdx, pFeatureDesc->pSession[sessionIdx].sessionId);
                result = CDKResultEInvalidArg;
                break;
            }

            for (UINT32 pipelineIdx = 0; pipelineIdx < pFeatureDesc->pSession[sessionIdx].numPipelines; pipelineIdx++)
            {
                const ChiFeature2PipelineDescriptor* pPipeline =
                    &pFeatureDesc->pSession[sessionIdx].pPipeline[pipelineIdx];
                if (pipelineIdx < pPipeline->pipelineId)
                {
                    CHX_LOG_ERROR("Invalid argument: sessionIdx = %d: sessionId = %d pipelineIdx = %d pipelineIdx = %d",
                        sessionIdx, pFeatureDesc->pSession[sessionIdx].sessionId, pipelineIdx, pPipeline->pipelineId);
                    result = CDKResultEInvalidArg;
                    break;
                }
            }

            if (CDKResultSuccess != result)
            {
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::DestroyFeatureData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::DestroyFeatureData()
{
    if (NULL != m_pThreadManager)
    {
        m_pThreadManager->UnregisterJobFamily(NULL, m_pFeatureName, m_hFeatureJob);
        m_pThreadManager = NULL;
        m_hFeatureJob    = 0;
    }

    if (NULL != m_pUsecaseDesc)
    {
        // Only free the UsecaseDesc here if it's not the same as the cloned descriptor
        if (m_pUsecaseDesc != m_pClonedUsecase)
        {
            UsecaseSelector::FreeUsecaseDescriptor(m_pUsecaseDesc);
        }
        m_pUsecaseDesc = NULL;
    }

    if (NULL != m_pClonedUsecase)
    {
        UsecaseSelector::DestroyUsecase(m_pClonedUsecase);
        m_pClonedUsecase = NULL;
    }

    // Destroy pipeline and session
    for (auto pSessionData : m_pSessionData)
    {
        Session* pSession = pSessionData->pSession;
        if (NULL != pSession)
        {
            pSession->Destroy(FALSE);
            pSessionData->pSession = NULL;
        }

        for (auto pPipelineData : pSessionData->pPipelineData)
        {
            Pipeline* pPipeline = pPipelineData->pPipeline;
            if (NULL != pPipeline)
            {
                pPipeline->Destroy();
                pPipelineData->pPipeline = NULL;
            }
        }
    }

    DestroyTargetBufferManagers();

    // clear pipeline and session data
    for (auto& pSessionData : m_pSessionData)
    {
        for (auto& pPipelineData : pSessionData->pPipelineData)
        {
            pPipelineData->pInputPortData.clear();
            pPipelineData->pOutputPortData.clear();
            CHX_DELETE pPipelineData;
            pPipelineData = NULL;
        }
        pSessionData->pPipelineData.clear();

        CHX_DELETE pSessionData;
        pSessionData = NULL;
    }
    m_pSessionData.clear();

    for (auto& pStageData : m_pStageData)
    {
        pStageData->inputPorts.clear();
        pStageData->outputPorts.clear();

        CHX_DELETE pStageData;
        pStageData = NULL;
    }
    m_pStageData.clear();

    if (FALSE == m_pStreams.empty())
    {
        for (auto& pStream : m_pStreams)
        {
            if (NULL != pStream)
            {
                ChxUtils::Free(pStream);
                pStream = NULL;
            }
        }

        m_pStreams.clear();
    }
    m_pStreamData.clear();

    m_targetStreams.pInputStreams.clear();
    m_targetStreams.pOutputStreams.clear();
    m_configStreams.clear();

    if (NULL != m_pProcessResultLock)
    {
        m_pProcessResultLock->Destroy();
        m_pProcessResultLock = NULL;
    }

    if ((NULL != m_pFlushThreadMutex) && (NULL != m_pFlushRequestAvailable))
    {
        m_pFlushThreadMutex->Lock();
        m_flushRequestProcessTerminate = TRUE;
        m_pFlushRequestAvailable->Signal();
        m_pFlushThreadMutex->Unlock();

        if (NULL != m_flushRequestProcessThread.pPrivateData)
        {
            ChxUtils::ThreadTerminate(m_flushRequestProcessThread.hThreadHandle);
            m_flushRequestProcessThread = { 0 };
        }

        m_pFlushThreadMutex->Destroy();
        m_pFlushThreadMutex = NULL;

        m_pFlushRequestAvailable->Destroy();
        m_pFlushRequestAvailable = NULL;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::OnPipelineSelect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnPipelineSelect(
    const CHAR*                         pPipelineName,
    const ChiFeature2CreateInputInfo*   pCreateInputInfo
    ) const
{
    CDK_UNUSED_PARAM(pPipelineName);
    CDK_UNUSED_PARAM(pCreateInputInfo);

    // Base class will un-conditionally select all the pipeline from feature descriptor
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PrepareFeatureData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnPruneUsecaseDescriptor(
    const ChiFeature2CreateInputInfo* pCreateInputInfo,
    std::vector<PruneVariant>&        rPruneVariants
    ) const
{
    CDK_UNUSED_PARAM(pCreateInputInfo);
    CDK_UNUSED_PARAM(rPruneVariants);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PrepareFeatureData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::PrepareFeatureData(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    CDKResult result           = CDKResultSuccess;
    UINT32    numSessions      = 0;
    UINT32    numPorts         = 0;
    UINT32*   pPipelineDescIdx = NULL;
    UINT32    totalPipelineIdx = 0;

    const ChiFeature2Descriptor* pFeatureDesc = pCreateInputInfo->pFeatureDescriptor;
    const ChiUsecase*            pUsecaseDesc = pCreateInputInfo->pUsecaseDescriptor;

    if (NULL == pFeatureDesc || NULL == pUsecaseDesc)
    {
        CHX_LOG_ERROR("Invalid argument: NULL pStreams pUsecaseDesc = %p pFeatureDesc = %p",
                      pUsecaseDesc,
                      pFeatureDesc);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        UINT32 numPipelines = 0;
        numSessions = pFeatureDesc->numSessions;
        // Count the number of pipelines
        for (UINT32 sessionIdx = 0; sessionIdx < numSessions; sessionIdx++)
        {
            for (UINT32 pipelineIdx = 0; pipelineIdx < pFeatureDesc->pSession[sessionIdx].numPipelines; pipelineIdx++)
            {
                auto& rPipelineDesc = pFeatureDesc->pSession[sessionIdx].pPipeline[pipelineIdx];
                numPorts += rPipelineDesc.numInputPorts;
                numPorts += rPipelineDesc.numOutputPorts;
                if (ChiFeature2PipelineType::Virtual != rPipelineDesc.pipelineType)
                {
                    numPipelines++; // Only count non-virtual pipelines
                }
            }
        }

        CHX_LOG_INFO("FeatureName = %s, numSessions = %d, numPipelines = %d numPorts = %d",
                     pFeatureDesc->pFeatureName, numSessions, numPipelines, numPorts);

        // In theory, a pure virtual feature will have no pipelines so don't treat this as an error
        if (numPipelines > 0)
        {
            pPipelineDescIdx = static_cast<UINT32*>(CHX_CALLOC(sizeof(UINT32) * numPipelines));
            if (NULL == pPipelineDescIdx)
            {
                CHX_LOG_ERROR("Allocation Failed");
                result = CDKResultENoMemory;
            }
        }

    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 sessionIdx = 0; sessionIdx < numSessions; sessionIdx++)
        {
            ChiFeatureSessionData* pSessionData = CHX_NEW ChiFeatureSessionData;

            if (NULL == pSessionData)
            {
                CHX_LOG_ERROR("Allocation Failed");
                result = CDKResultENoMemory;
                break;
            }

            pSessionData->globalId          = {pFeatureDesc->pSession[sessionIdx].sessionId, 0, 0};
            pSessionData->pSession          = NULL;
            pSessionData->isFlushInProgress = FALSE;
            pSessionData->pSessionName      = pFeatureDesc->pSession[sessionIdx].pSessionName;

            pSessionData->sessionCbData.pFeatureInstance = this;
            pSessionData->sessionCbData.sessionId        = pSessionData->globalId.session;

            if (NULL != pCreateInputInfo->pFeatureGraphManagerCallbacks)
            {
                pSessionData->sessionCbData.featureGraphManagerCallbacks = *(pCreateInputInfo->pFeatureGraphManagerCallbacks);
            }

            pSessionData->callbacks.ChiNotify                      = ChiFeature2Base::ProcessMessageCallbackFromDriver;
            pSessionData->callbacks.ChiProcessCaptureResult        = ChiFeature2Base::ProcessResultCallbackFromDriver;
            pSessionData->callbacks.ChiProcessPartialCaptureResult = ChiFeature2Base::ProcessPartialResultCallbackFromDriver;

            for (UINT32 pipelineIdx = 0; pipelineIdx < pFeatureDesc->pSession[sessionIdx].numPipelines; pipelineIdx++)
            {
                const CHAR* pSkipReason = NULL;

                const ChiFeature2PipelineDescriptor* pPipelineDesc = &pFeatureDesc->pSession[sessionIdx].pPipeline[pipelineIdx];

                for (UINT32 index = 0; index < pSessionData->pPipelineData.size(); index++)
                {
                    if (pSessionData->pPipelineData[index]->globalId.pipeline == pPipelineDesc->pipelineId)
                    {
                        pSkipReason = "Session has pipeline";
                        break; // If the session already has this pipeline, then skip it
                    }
                }

                BOOL pipelinesWithSameId = FALSE;
                for (UINT32 index = 0; index < pFeatureDesc->pSession[sessionIdx].numPipelines; index++)
                {
                    if ((pPipelineDesc->pipelineId == pFeatureDesc->pSession[sessionIdx].pPipeline[index].pipelineId) &&
                        (pipelineIdx != index))
                    {
                        pipelinesWithSameId = TRUE;
                        break; // If the session descriptor has duplicate pipeline ids, then skip it
                    }
                }

                if (NULL != pSkipReason)
                {
                    CHX_LOG_VERBOSE("Skipping: %s", pSkipReason);
                    continue;
                }

                // if there're more than one pipelines for the same pipeline id, choose one based on stream config,
                // otherwise just create the pipeline
                if ((FALSE == pipelinesWithSameId) ||
                    (TRUE == OnPipelineSelect(pPipelineDesc->pPipelineName, pCreateInputInfo)))
                {
                    if ((NULL != pPipelineDescIdx) && (ChiFeature2PipelineType::Virtual != pPipelineDesc->pipelineType))
                    {
                        pPipelineDescIdx[totalPipelineIdx++] = GetPipelineIndex(pUsecaseDesc, pPipelineDesc->pPipelineName);
                    }
                    ChiFeaturePipelineData* pPipelineData = CHX_NEW ChiFeaturePipelineData;

                    if (NULL == pPipelineData)
                    {
                        CHX_LOG_ERROR("Allocation Failed");
                        result = CDKResultENoMemory;
                        break;
                    }

                    pPipelineData->globalId =
                    {
                        pFeatureDesc->pSession[sessionIdx].sessionId,
                        pPipelineDesc->pipelineId,
                        0
                    };
                    pPipelineData->pPipelineName = pPipelineDesc->pPipelineName;

                    CHX_LOG_INFO("session: %d, name: %s, pipeline: %d, name: %s",
                                 pFeatureDesc->pSession[sessionIdx].sessionId, pSessionData->pSessionName,
                                 pPipelineDesc->pipelineId,
                                 pPipelineData->pPipelineName);

                    ChiFeaturePortData portData = {{0}};
                    for (UINT i = 0; i < pPipelineDesc->numInputPorts; i++)
                    {
                        portData.globalId         = pPipelineDesc->pInputPortDescriptor[i].globalId;
                        portData.pPortName        = pPipelineDesc->pInputPortDescriptor[i].pPortName;
                        portData.pTargetDesc      = pPipelineDesc->pInputPortDescriptor[i].pTargetDescriptor;
                        portData.pOutputBufferTbm = NULL;
                        pPipelineData->pInputPortData.push_back(portData);

                        m_portIdToPortDescMap.insert({portData.globalId, pPipelineDesc->pInputPortDescriptor[i]});
                    }

                    for (UINT i = 0; i < pPipelineDesc->numOutputPorts; i++)
                    {
                        portData.globalId         = pPipelineDesc->pOutputPortDescriptor[i].globalId;
                        portData.pPortName        = pPipelineDesc->pOutputPortDescriptor[i].pPortName;
                        portData.pTargetDesc      = pPipelineDesc->pOutputPortDescriptor[i].pTargetDescriptor;
                        portData.pOutputBufferTbm = NULL;
                        portData.metadataClientId = InvalidIndex;
                        pPipelineData->pOutputPortData.push_back(portData);
                        m_portIdToPortDescMap.insert({portData.globalId, pPipelineDesc->pOutputPortDescriptor[i]});
                    }
                    pSessionData->pPipelineData.push_back(pPipelineData);
                }
            }

            if (CDKResultSuccess == result)
            {
                m_pSessionData.push_back(pSessionData);
            }
        }

        m_numInternalLinks   = pFeatureDesc->numInternalLinks;
        m_pInternalLinkDesc  = pFeatureDesc->pInternalLinkDesc;
        m_numSessions        = pFeatureDesc->numSessions;
        m_pSessionDescriptor = pFeatureDesc->pSession;
    }


    if ((CDKResultSuccess == result) && (NULL != pPipelineDescIdx))
    {
        m_pClonedUsecase = UsecaseSelector::CloneUsecase(pUsecaseDesc, totalPipelineIdx, pPipelineDescIdx);

        if (NULL != m_pClonedUsecase)
        {
            std::vector<PruneVariant> pruneVariants;

            OnPruneUsecaseDescriptor(pCreateInputInfo, pruneVariants);

            if (TRUE == pruneVariants.empty())
            {
                m_pUsecaseDesc = m_pClonedUsecase;
            }
            else
            {
                result = UsecaseSelector::PruneUsecaseDescriptor(m_pClonedUsecase,
                                                                 pruneVariants.size(),
                                                                 pruneVariants.data(),
                                                                 &m_pUsecaseDesc);

                if (CDKResultSuccess != result || (NULL == m_pUsecaseDesc))
                {
                    CHX_LOG_ERROR("Error pruning cloned usecase: %s Code: %u", m_pClonedUsecase->pUsecaseName, result);
                    result = CDKResultEInvalidPointer;
                }
            }
        }
        else
        {
            result = CDKResultENoMemory; // Cloning will have logged failure message
        }

    }

    if (NULL != pPipelineDescIdx)
    {
        CHX_FREE(pPipelineDescIdx);
        pPipelineDescIdx = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PrepareStageData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::PrepareStageData(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    CDKResult                    result       = CDKResultSuccess;
    const ChiFeature2Descriptor* pFeatureDesc = pCreateInputInfo->pFeatureDescriptor;

    if (NULL == pFeatureDesc->pStages)
    {
        CHX_LOG_ERROR("Input stage descriptor is NULL");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        m_pStageData.reserve(pFeatureDesc->numStages);
    }

    for (UINT8 stageIndex = 0; ((stageIndex < pFeatureDesc->numStages) && (CDKResultSuccess == result)); ++stageIndex)
    {
        ChiFeatureStageData* pStageData = CHX_NEW ChiFeatureStageData;

        if (NULL == pStageData)
        {
            CHX_LOG_ERROR("Out of memory");
            result = CDKResultEFailed;
            break;
        }


        const ChiFeature2Descriptor&      rFeatureDescriptor = *pCreateInputInfo->pFeatureDescriptor;
        const ChiFeature2StageDescriptor* pStageDescriptor   = &pFeatureDesc->pStages[stageIndex];
        pStageData->stageDescriptor                          = *pStageDescriptor;

        for (UINT8 dependencyIdx = 0; dependencyIdx < pStageDescriptor->numDependencyConfigDescriptor; dependencyIdx++)
        {
            const ChiFeature2SessionInfo& rSessionInfo   = pStageDescriptor->pDependencyConfigDescriptor[dependencyIdx];
            UINT                          sessionDescIdx = INVALID_INDEX;

            for (UINT sessionIdx = 0; sessionIdx < rFeatureDescriptor.numSessions; sessionIdx++)
            {
                if (rFeatureDescriptor.pSession[sessionIdx].sessionId == rSessionInfo.sessionId)
                {
                    sessionDescIdx = sessionIdx;
                    break;
                }
            }

            if (INVALID_INDEX == sessionDescIdx)
            {
                CHX_LOG_ERROR("%s Could not find session id: %u for dependency: %u",
                              rFeatureDescriptor.pFeatureName,
                              rSessionInfo.sessionId,
                              dependencyIdx);
                result = CDKResultEInvalidState;
                break;
            }

            const ChiFeature2SessionDescriptor&   rSessionDesc      = rFeatureDescriptor.pSession[sessionDescIdx];
            std::vector<ChiFeaturePipelineData*>& rPipelineDataList = m_pSessionData[sessionDescIdx]->pPipelineData;

            CHX_LOG_VERBOSE("%s Session: %s %u (%u / %u)",
                            pCreateInputInfo->pFeatureDescriptor->pFeatureName,
                            rSessionDesc.pSessionName,
                            rSessionDesc.sessionId,
                            sessionDescIdx + 1,
                            pCreateInputInfo->pFeatureDescriptor->numSessions);

            for (UINT8 pipelineIndex = 0; pipelineIndex < rSessionInfo.numPipelines; ++pipelineIndex)
            {
                const ChiFeature2PipelineDescriptor rPipelineDesc = rSessionDesc.pPipeline[pipelineIndex];
                const ChiFeature2PipelineInfo       pipelineInfo  = rSessionInfo.pPipelineInfo[pipelineIndex];
                const ChiFeature2Identifier         pipelineId    =
                {
                    rSessionInfo.sessionId,
                    pipelineInfo.pipelineId,
                    0
                };

                auto* pConfigDesc  = reinterpret_cast<ChiFeature2DependencyConfigDescriptor*>(pipelineInfo.handle);
                // NOWHINE CF012: The "if" below is not a keyword
                auto  pipelineData = std::find_if(rPipelineDataList.begin(),
                                                  rPipelineDataList.end(),
                                                  [&] (ChiFeaturePipelineData* pOther)
                                                  {
                                                      return pOther->globalId == pipelineId;
                                                  });

                if (rPipelineDataList.end() == pipelineData)
                {
                    CHX_LOG_WARN("Could not find Session: %u Pipeline: %u", rSessionInfo.sessionId, pipelineInfo.pipelineId);
                    continue;
                }

                ChiFeaturePipelineData* pPipelineData = *pipelineData;

                for (UINT inputIndex = 0; inputIndex < pConfigDesc->numInputConfig; ++inputIndex)
                {
                    pStageData->inputPorts.push_back(pConfigDesc->pInputConfig[inputIndex].globalId);
                }

                for (UINT outputIndex = 0; outputIndex < pConfigDesc->numOutputConfig; ++outputIndex)
                {
                    ChiFeaturePortData*   pPortData = &pPipelineData->pOutputPortData[outputIndex];
                    ChiFeature2Identifier portId    = pConfigDesc->pOutputConfig[outputIndex].globalId;
                    if ((NULL != pPortData->pOutputBufferTbm) ||
                        (ChiFeature2PipelineType::Virtual == rPipelineDesc.pipelineType))
                    {
                        pStageData->outputPorts.push_back(portId);
                    }
                    else
                    {
                        CHX_LOG_INFO("%10s] %5s tbm: %33s %u:%u:%u - %s, %p (%u / %u)",
                                     pCreateInputInfo->pFeatureDescriptor->pFeatureName,
                                     (NULL != pPortData->pOutputBufferTbm) ? "valid" : "null",
                                     pPortData->pPortName,
                                     portId.session, portId.pipeline, portId.port,
                                     pPipelineData->pPipelineName,
                                     pPipelineData, pipelineIndex, rSessionInfo.numPipelines);
                    }
                }
            }
        }
        m_pStageData.push_back(pStageData);
    }

    if (NULL != m_pMetadataManager)
    {
        m_pMetadataManager->InitializeFrameworkInputClient(1);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ClassifyStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ClassifyStream (
    const CHISTREAMCONFIGINFO* pStreams)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pStreams)
    {
        CHX_LOG_ERROR("Invalid argument: NULL pStreams");
        result = CDKResultEInvalidArg;
    }
    else
    {
        for (UINT32 streamIdx = 0; streamIdx < pStreams->numStreams; streamIdx++)
        {
            CHISTREAM* pStream  = reinterpret_cast<CHISTREAM*>(pStreams->pChiStreams[streamIdx]);
            pStream->pHalStream = NULL;

            if (ChiStreamTypeInput == pStream->streamType)
            {
                m_targetStreams.pInputStreams.push_back(pStream);
            }
            else
            {
                m_targetStreams.pOutputStreams.push_back(pStream);
            }
            m_configStreams.push_back(pStream);

            CHX_LOG_INFO("Name = %s: Res: %d X %d Format = 0x%x Type = %d",
                m_pFeatureName, pStream->width, pStream->height, pStream->format,
                pStream->streamType);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnInitialize(
    ChiFeature2CreateInputInfo* pRequestObject)
{
    CDKResult result = CDKResultSuccess;

    result = CreateFeatureData(pRequestObject->pFeatureDescriptor);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnSubmitRequestToSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnSubmitRequestToSession(
    ChiPipelineRequest*    pPipelineRequest
    ) const
{
    CDKResult result = CDKResultSuccess;

    result = SendSubmitRequestMessage(pPipelineRequest);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnSessionCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnSessionCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pKey)
    {
        CHX_LOG_ERROR("invalid pKey! pKey is NULL");
        result = CDKResultEInvalidArg;
    }

    ChiFeatureSessionData* pSessionData = GetSessionData(pKey);

    if ((CDKResultSuccess == result) && (NULL != pSessionData))
    {
        for (UINT32 index = 0; index < pSessionData->pPipelineData.size(); index++)
        {
            ChiFeaturePipelineData* pPipelineData    = GetPipelineData(&pSessionData->pPipelineData[index]->globalId);
            ChiMetadataManager*     pMetadataManager = GetMetadataManager();
            Pipeline*               pPipeline        = NULL;
            if (NULL != pPipelineData)
            {
                pPipeline = pPipelineData->pPipeline;
            }

            if ((NULL != pPipeline) && (NULL != pMetadataManager))
            {
                UINT32 metaclientId =  pMetadataManager->RegisterClient(FALSE,
                    pPipeline->GetTagList(),
                    pPipeline->GetTagCount(),
                    pPipeline->GetPartialTagCount(),
                    1,
                    ChiMetadataUsage::OfflineOutput);

                SetMetadataClientId(pPipelineData, metaclientId);
            }
            else
            {
                CHX_LOG_ERROR("pPipeline=%p, pMetadataManager=%p", pPipeline, pMetadataManager);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnPreparePipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnPreparePipelineCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    ChiFeaturePipelineData* pPipelineData = GetPipelineData(pKey);
    if ((NULL != pPipelineData) && (NULL != pPipelineData->pPipeline))
    {
        AssignSessionSettings(pPipelineData->pPipeline);
    }
    else
    {
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnPipelineCreate(
    ChiFeature2Identifier* pKey)
{
    CDK_UNREFERENCED_PARAM(pKey);
    CDKResult result = CDKResultSuccess;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnPortCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnPortCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    ChiFeaturePortData* pPortData = GetPortData(pKey);

    if ((NULL != pPortData) && (NULL != pPortData->pTarget))
    {
        pPortData->maxBufferCount  = pPortData->pChiStream->maxNumBuffers;
        pPortData->minBufferCount  = 0;
        pPortData->producerFlags   = GRALLOC1_PRODUCER_USAGE_CAMERA;
        pPortData->consumerFlags   = GRALLOC1_CONSUMER_USAGE_CAMERA;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessPartialResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::ProcessPartialResult(
    CHIPARTIALCAPTURERESULT*   pCaptureResult,
    VOID*                      pPrivateCallbackData)
{
    CDK_UNUSED_PARAM(pPrivateCallbackData);
    CDKResult result = CDKResultSuccess;

    if (NULL == pCaptureResult || NULL == pCaptureResult->pPrivData)
    {
        result = CDKResultEInvalidArg;
        CHX_LOG_ERROR("ERROR Processing Partial Result pCaptureResult: %p, pCaptureResult->pPrivData: %p",
                      pCaptureResult,
                     (pCaptureResult != NULL) ? pCaptureResult->pPrivData : NULL);
    }

    // If feature graph manager destroy is in progress, we will not process partial result from driver
    if (TRUE == m_destroyInProgress)
    {
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {

        ChiFeatureCombinedCallbackData* pCombinedCbData =
            static_cast<ChiFeatureCombinedCallbackData*>(pCaptureResult->pPrivData);

        for (UINT8 cbIndex = 0; cbIndex < pCombinedCbData->numCallbackData; ++ cbIndex)
        {
            ChiFeatureFrameCallbackData* pFrameCbData   = pCombinedCbData->pCombinedCallbackData[cbIndex];
            ChiFeature2RequestObject*    pFeatureReqObj = pFrameCbData->pRequestObj;
            ChiFeatureSequenceData*      pRequestData   = NULL;

            if (NULL != pCaptureResult->pPartialResultMetadata)
            {
                ChiFeature2StageInfo   stageInfo = { 0 };

                stageInfo.stageId           = pFrameCbData->stageId;
                stageInfo.stageSequenceId   = pFrameCbData->stagesequenceId;

                pFeatureReqObj = static_cast<ChiFeature2RequestObject*>(pFrameCbData->pRequestObj);
                if (NULL == pFeatureReqObj)
                {
                    CHX_LOG_ERROR("No Request Object sequenceId = %d batchRequestId = %d",
                        pFrameCbData->sequenceId, pFrameCbData->requestId);
                    result = CDKResultEFailed;
                }

                if (CDKResultSuccess == result)
                {
                    pRequestData = static_cast<ChiFeatureSequenceData*>(
                        pFeatureReqObj->GetSequencePrivDataById(pFrameCbData->sequenceId,
                            pFrameCbData->requestId));
                    if (NULL == pRequestData)
                    {
                        CHX_LOG_INFO("No sequence private data sequenceId = %d batchRequestId = %d",
                            pFrameCbData->sequenceId, pFrameCbData->requestId);
                        result = CDKResultEFailed;
                    }
                }

                if (CDKResultSuccess == result)
                {
                    for (UINT8 portIndex = 0; portIndex < pFrameCbData->pOutputPorts.size(); ++portIndex)
                    {
                        if ((ChiFeature2PortDirectionType::ExternalOutput ==
                            pFrameCbData->pOutputPorts[portIndex].portDirectionType) &&
                            (ChiFeature2PortType::MetaData == pFrameCbData->pOutputPorts[portIndex].portType))
                        {
                            ChiFeature2Identifier portIdentifier  = pFrameCbData->pOutputPorts[portIndex];
                            BOOL                  sendMetadata    = TRUE;
                            ChiMetadata*          pOutputMeta     = m_pMetadataManager->GetMetadataFromHandle(
                                pCaptureResult->pPartialResultMetadata);

                            sendMetadata = OnPartialMetadataResult(pFeatureReqObj,
                                pFrameCbData->requestId,
                                &stageInfo,
                                &portIdentifier,
                                pOutputMeta,
                                pCaptureResult->frameworkFrameNum,
                                pRequestData);

                            if (FALSE == sendMetadata)
                            {
                                CHX_LOG_WARN("External PortId %d %d %d not requested for frame %d", portIdentifier.session,
                                    portIdentifier.pipeline, portIdentifier.port, pCaptureResult->frameworkFrameNum);
                            }

                            if (TRUE == sendMetadata)
                            {
                                ChiFeature2MessageDescriptor partialMetadataResult;

                                partialMetadataResult.messageType                =
                                    ChiFeature2MessageType::PartialMetadataNotification;
                                partialMetadataResult.message.result.resultIndex = pFrameCbData->requestId;
                                partialMetadataResult.message.result.numPorts    = 1;
                                partialMetadataResult.message.result.pPorts      = &portIdentifier;
                                partialMetadataResult.pPrivData                  = pFeatureReqObj;
                                ProcessFeatureMessage(&partialMetadataResult);
                            }
                            break;
                        }
                    }
                }
            }
        }
        ChiFeatureFrameCallbackData* pFrameCbData   = static_cast<ChiFeatureFrameCallbackData*>(pCaptureResult->pPrivData);
        ChiFeature2RequestObject*    pFeatureReqObj = NULL;
        ChiFeatureSequenceData*      pRequestData   = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::SendSubmitRequestMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::SendSubmitRequestMessage(
    ChiPipelineRequest* pPipelineRequest
    ) const
{
    CDKResult result = CDKResultEFailed;
    // Submit the request to CamX.
    if ((NULL != pPipelineRequest) && (NULL != pPipelineRequest->pCaptureRequests))
    {
        ChiFeatureCombinedCallbackData* pFrameData = static_cast<ChiFeatureCombinedCallbackData*>(
            pPipelineRequest->pCaptureRequests->pPrivData);

        ChiFeature2MessageDescriptor    featureMessage;

        ChxUtils::Memset(&featureMessage, 0, sizeof(ChiFeature2MessageDescriptor));
        featureMessage.messageType = ChiFeature2MessageType::SubmitRequestNotification;
        featureMessage.message.submitRequest = *pPipelineRequest;

        // Should not need FRO for this, need to revisit
        featureMessage.pPrivData = pFrameData->pCombinedCallbackData[0]->pRequestObj;

        result = ProcessFeatureMessage(&featureMessage);

        // Clean up allocations
        for (UINT32 requestIndex = 0; requestIndex < pPipelineRequest->numRequests; ++requestIndex)
        {
            // NOWHINE CP036a: Need to free the object
            ChiCaptureRequest* pRequest = const_cast<ChiCaptureRequest*>(&pPipelineRequest->pCaptureRequests[requestIndex]);
            if (NULL != pRequest)
            {
                CHX_LOG_VERBOSE("%s Instantiating submit request callback for request:%" PRIu64,
                    pFrameData->pCombinedCallbackData[0]->pRequestObj->IdentifierString(),
                    pRequest->frameNumber);

                if (NULL != pRequest->pInputBuffers)
                {
                    CHX_FREE(pRequest->pInputBuffers);
                }
                if (NULL != pRequest->pOutputBuffers)
                {
                    CHX_FREE(pRequest->pOutputBuffers);
                }
                CHX_FREE(pRequest);
            }
        }
        CHX_FREE(pPipelineRequest);
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::ProcessMessage(
    const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
    VOID*                       pPrivateCallbackData)
{
    CDKResult                       result          = CDKResultSuccess;
    ChiFeature2SessionCallbackData* pCallbackData   = static_cast<ChiFeature2SessionCallbackData*>(pPrivateCallbackData);

    if ((NULL == m_clientCallbacks.ChiFeature2ProcessMessage) || (NULL == pMessageDescriptor) || (NULL == pCallbackData))
    {
        CHX_LOG_ERROR("Invalid argument: NULL m_clientCallbacks.ChiFeature2ProcessMessage = %p, pMessageDescriptor = %p"
                      "pCallbackData = %p",
                      m_clientCallbacks.ChiFeature2ProcessMessage,
                      pMessageDescriptor,
                      pCallbackData);
        result = CDKResultEInvalidArg;
    }

    // If feature graph manager destroy is in progress, we will not process message from driver
    if (TRUE == m_destroyInProgress)
    {
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2Notification         chiNotification = {0};
        ChiFeature2RequestObject*       pFeatureReqObj  = NULL;
        ChiFeature2Messages             results         = {m_featureId, chiNotification, NULL};
        ChiFeatureCombinedCallbackData* pCombinedCbData    =
            static_cast<ChiFeatureCombinedCallbackData*>(pMessageDescriptor->pPrivData);

        results.chiNotification.featureGraphManagerCallbacks = pCallbackData->featureGraphManagerCallbacks;

        // SOF/MetaBufferDone does not have any private data associated with the message, send the null FRO in the callback
        // and let the graph handle
        if ((ChiMessageTypeSof == pMessageDescriptor->messageType) ||
            (ChiMessageTypeMetaBufferDone == pMessageDescriptor->messageType))
        {
            results.chiNotification.pChiMessages = pMessageDescriptor;
            m_clientCallbacks.ChiFeature2ProcessMessage(pFeatureReqObj, &results);
        }
        else if (NULL != pCombinedCbData)
        {
            for (UINT8 cbIndex = 0; cbIndex < pCombinedCbData->numCallbackData; ++ cbIndex)
            {
                ChiFeatureFrameCallbackData* pFrameCbData = pCombinedCbData->pCombinedCallbackData[cbIndex];

                if (NULL == pFrameCbData)
                {
                    CHX_LOG_ERROR("pFrameCbData is NULL for frame %d pCombinedCallback %p",
                        pMessageDescriptor->message.shutterMessage.frameworkFrameNum,
                        pCombinedCbData);
                }
                else
                {
                    pFeatureReqObj = static_cast<ChiFeature2RequestObject*>(pFrameCbData->pRequestObj);

                    switch (pMessageDescriptor->messageType)
                    {
                        case ChiMessageTypeError:
                            ProcessErrorMessageFromDriver(pMessageDescriptor, pFrameCbData, pCallbackData);
                            break;
                        case ChiMessageTypeShutter:
                            m_lastShutterTimestamp = pMessageDescriptor->message.shutterMessage.timestamp;
                            results.chiNotification.pChiMessages = pMessageDescriptor;
                            m_clientCallbacks.ChiFeature2ProcessMessage(pFeatureReqObj, &results);
                            break;
                        default:
                            CHX_LOG_ERROR("Need Implementation: %d", pMessageDescriptor->messageType);
                            break;
                    }
                }

            }
        }
        // Recovery and Device errors are not specific to a request, we don't require private data in order to
        // propagate these notifications
        else if ((ChiMessageTypeError == pMessageDescriptor->messageType) &&
                 ((pMessageDescriptor->message.errorMessage.errorMessageCode == MessageCodeTriggerRecovery) ||
                  (pMessageDescriptor->message.errorMessage.errorMessageCode == MessageCodeDevice)))
        {
            ChiFeatureFrameCallbackData frameCbData = {0};
            ProcessErrorMessageFromDriver(pMessageDescriptor, &frameCbData, pCallbackData);
        }
        else
        {
            CHX_LOG_ERROR("pCombinedCbData is null for message type:%d; not propagating any notification",
                          pMessageDescriptor->messageType);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::ReleaseDependenciesOnInputResourcePending
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::ReleaseDependenciesOnInputResourcePending(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId)
{
    // Destroy resource allocated during add dependency.
    UINT8                             numChiFeature2DependencyConfigInfo = 0;
    ChiFeature2DependencyConfigInfo*  pDependencyConfigInfo              = NULL;
    ChiFeature2SessionInfo*           pConfig                            = NULL;

    numChiFeature2DependencyConfigInfo = pRequestObject->GetDependencyConfig(
        ChiFeature2SequenceOrder::Current, &pConfig, requestId);
    if (NULL != pConfig)
    {
        for (UINT8 sessionIdx = 0; sessionIdx < numChiFeature2DependencyConfigInfo; sessionIdx++)
        {
            for (UINT8 pipelineIdx = 0; pipelineIdx < pConfig[sessionIdx].numPipelines; pipelineIdx++)
            {
                ChiFeature2DependencyConfigInfo* pDependencyConfigInfo =
                    reinterpret_cast<ChiFeature2DependencyConfigInfo*>(
                        pConfig[sessionIdx].pPipelineInfo[pipelineIdx].handle);

                if (NULL != pDependencyConfigInfo)
                {
                    for (UINT8 listIndex = 0; listIndex < pDependencyConfigInfo->numInputDependency; ++listIndex)
                    {
                        ChiFeature2PortBufferMetadataInfo* pBufferMetadataInfo =
                            &pDependencyConfigInfo->pInputDependency[listIndex];
                        if (NULL != pBufferMetadataInfo)
                        {
                            for (UINT8 portIndex = 0; portIndex < pBufferMetadataInfo->num; ++portIndex)
                            {
                                ChiFeature2Identifier     portidentifier =
                                    pBufferMetadataInfo->pPortDescriptor[portIndex].globalId;

                                if (portidentifier.portType == ChiFeature2PortType::MetaData)
                                {
                                    CHIMETAHANDLE  hSetting = NULL;
                                    pRequestObject->GetRequestInputInfo(
                                        ChiFeature2SequenceOrder::Current, &portidentifier,
                                        &hSetting, listIndex, requestId);

                                    if (NULL != hSetting)
                                    {
                                        CHX_LOG_INFO("Release Dependency meta: %s: %d %p %d requestId:%d",
                                            m_pFeatureName, listIndex,
                                            hSetting, portidentifier.port,
                                            requestId);
                                        ChiMetadata* pFeatureSettings = GetMetadataManager()->GetMetadataFromHandle(
                                            hSetting);
                                        if (NULL != pFeatureSettings)
                                        {
                                            pFeatureSettings->Destroy(TRUE);
                                            // clearing the featuresetting pointer from FRO
                                            pRequestObject->ResetRequestInputInfo(
                                                ChiFeature2SequenceOrder::Current, &portidentifier,
                                                listIndex, requestId);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::ReleaseDependenciesOnDriverError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::ReleaseDependenciesOnDriverError(
    const ChiFeature2Identifier*    pIdentifier,
    ChiFeatureFrameCallbackData*    pFrameCbData,
    ChiFeature2RequestObject*       pFeatureReqObj
    ) const
{
    ChiFeature2StageInfo stageInfo = {pFrameCbData->stageId, pFrameCbData->stagesequenceId};
    // Erase output port from frame cb data
    for (UINT portIdx = 0; portIdx < pFrameCbData->pOutputPorts.size(); portIdx++)
    {
        ChiFeaturePortData* pPortData = GetPortData(&pFrameCbData->pOutputPorts[portIdx]);
        if (NULL != pPortData)
        {
            if (pPortData->globalId == *pIdentifier)
            {
                CHX_LOG_VERBOSE("%s erasing GID: [%d %d %d %d] from FrameCbData output ports",
                                pFeatureReqObj->IdentifierString(),
                                pPortData->globalId.session,
                                pPortData->globalId.pipeline,
                                pPortData->globalId.port,
                                pPortData->globalId.portType);
                pFrameCbData->pOutputPorts.erase((pFrameCbData->pOutputPorts.begin() + portIdx));
                break;
            }
        }
    }

    if (TRUE == pFrameCbData->pOutputPorts.empty())
    {
        // If we have received all outputs from the last stage, we can go to notification pending state
        if ((GetNumStages() - 1) == pFrameCbData->stageId)
        {
            pFeatureReqObj->SetCurRequestState(ChiFeature2RequestState::OutputErrorNotificationPending,
                                               pFrameCbData->requestId);
        }
        if (0 != pFrameCbData->pInputPorts.size())
        {
            ProcessReleaseDependency(pFeatureReqObj, pFrameCbData->requestId, pFrameCbData->sequenceId, &stageInfo);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessErrorMessageFromDriver
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::ProcessErrorMessageFromDriver(
    const CHIMESSAGEDESCRIPTOR*     pMessageDescriptor,
    ChiFeatureFrameCallbackData*    pFrameCbData,
    ChiFeature2SessionCallbackData* pSessionCallbackData)
{
    CDKResult                       result          = CDKResultSuccess;
    ChiFeature2Notification         chiNotification = {0};
    ChiFeature2Messages             results         = {m_featureId, chiNotification, NULL};
    CHIERRORMESSAGECODE             errorMessage    = pMessageDescriptor->message.errorMessage.errorMessageCode;
    ChiFeatureSessionData *         pSessionData    = m_pSessionData[pSessionCallbackData->sessionId];
    BOOL                            flushInProgress = static_cast<BOOL>(ChxUtils::AtomicLoadU32(&pSessionData->
                                                                                                isFlushInProgress));
    // It is okay for frame data to be NULL with Recovery or Device error. It cannot get to this point and be
    // NULL for non-Recovery/Device errors
    ChiFeature2RequestObject*       pFeatureReqObj  = pFrameCbData->pRequestObj;

    // If feature graph manager destroy is in progress, we will not process message from driver
    if (TRUE == m_destroyInProgress)
    {
        result = CDKResultEFailed;
    }

    results.chiNotification.featureGraphManagerCallbacks = pSessionCallbackData->featureGraphManagerCallbacks;
    if (CDKResultSuccess == result && NULL != pFeatureReqObj)
    {
        pFeatureReqObj->SetCurRequestState(ChiFeature2RequestState::OutputErrorResourcePending, pFrameCbData->requestId);
    }

    m_pProcessResultLock->Lock();

    // If we have a buffer or result error, we need to propagate the error through the graph
    // else propagate it straight through the layer
    if (CDKResultSuccess == result)
    {
        if ((MessageCodeResult == errorMessage) || (MessageCodeBuffer == errorMessage))
        {
            results.chiNotification.pChiMessages    = pMessageDescriptor;
            results.chiNotification.requestIndex    = pFrameCbData->requestId;
            results.chiNotification.pIdentifier     = GetMatchingIdentifier(pFrameCbData, pMessageDescriptor);

            if (NULL != results.chiNotification.pIdentifier)
            {
                if (FALSE == flushInProgress)
                {
                    CHX_LOG_ERROR("%s Base received error type:%s",
                                  pFeatureReqObj->IdentifierString(),
                                  ChxUtils::ErrorMessageCodeToString(
                                      pMessageDescriptor->message.errorMessage.errorMessageCode));
                }
                else
                {
                    CHX_LOG_INFO("Flushing %s, servicing error type:%s",
                                  pFeatureReqObj->IdentifierString(),
                                  ChxUtils::ErrorMessageCodeToString(
                                      pMessageDescriptor->message.errorMessage.errorMessageCode));
                }
                HandleBufferAndResultError(&results, pFrameCbData);
            }
            else
            {
                CHX_LOG_ERROR("%s Unable to find feature identifier", pFeatureReqObj->IdentifierString());
            }
        }
        else
        {
            switch (pMessageDescriptor->message.errorMessage.errorMessageCode)
            {
                case MessageCodeRequest:
                    if (FALSE == flushInProgress)
                    {
                        CHX_LOG_ERROR("%s Base received a request error", pFeatureReqObj->IdentifierString());
                    }
                    else
                    {
                        CHX_LOG_INFO("Flushing %s, servicing error type:%s",
                                     pFeatureReqObj->IdentifierString(),
                                     ChxUtils::ErrorMessageCodeToString(
                                         pMessageDescriptor->message.errorMessage.errorMessageCode));
                    }
                    HandleRequestError(pMessageDescriptor, pFrameCbData);
                    break;
                case MessageCodeDevice:
                case MessageCodeTriggerRecovery:
                    if (FALSE == flushInProgress)
                    {
                        CHX_LOG_ERROR("Base received error type:%s",
                                      ChxUtils::ErrorMessageCodeToString(
                                          pMessageDescriptor->message.errorMessage.errorMessageCode));
                    }
                    else
                    {
                        CHX_LOG_INFO("servicing error type:%s",
                                     ChxUtils::ErrorMessageCodeToString(
                                         pMessageDescriptor->message.errorMessage.errorMessageCode));
                    }
                    results.chiNotification.pChiMessages = pMessageDescriptor;
                    results.chiNotification.requestIndex = pFrameCbData->requestId;
                    m_clientCallbacks.ChiFeature2ProcessMessage(pFeatureReqObj, &results);
                    break;
                default:
                    CHX_LOG_WARN("Unhandled error message type:%d, not propagating message",
                                 pMessageDescriptor->message.errorMessage.errorMessageCode);
                    break;
            }
        }
    }
    m_pProcessResultLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::HandleRequestError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::HandleRequestError(
    const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
    ChiFeatureFrameCallbackData*pFrameCbData
    ) const
{
    CDKResult                           result                  = CDKResultSuccess;
    ChiFeature2Notification             chiNotification         = {0};
    ChiFeature2Messages                 results                 = {m_featureId, chiNotification, NULL};
    ChiFeaturePortData*                 pPortData               = NULL;
    UINT                                outputPortSize          = 0;
    std::vector<ChiFeature2Identifier>  imageBufferIdentifiers;
    std::vector<ChiFeature2Identifier>  metadataIdentifiers;
    CHIMESSAGEDESCRIPTOR                messageDescriptor;

    messageDescriptor.pPrivData                                 = pFrameCbData;
    messageDescriptor.messageType                               = ChiMessageTypeError;
    messageDescriptor.message.errorMessage.frameworkFrameNum    =
        pMessageDescriptor->message.errorMessage.frameworkFrameNum;

    if (NULL != pFrameCbData)
    {
        for (UINT portIdx = 0; portIdx < pFrameCbData->pOutputPorts.size(); portIdx++)
        {
            ChiFeaturePortData* pPortData = GetPortData(&pFrameCbData->pOutputPorts[portIdx]);
            if (NULL != pPortData)
            {
                if (ChiFeature2PortType::ImageBuffer == pPortData->globalId.portType)
                {
                    imageBufferIdentifiers.push_back(pPortData->globalId);
                }
                else if (ChiFeature2PortType::MetaData == pPortData->globalId.portType)
                {
                    metadataIdentifiers.push_back(pPortData->globalId);
                }
            }
        }

        /// Generate a Chi Error message from a given list of identifiers. Used when translating request
        /// error to buffer and metadata errors. Because of graph dependency, we send out metadata errors last
        /// so this helper function helps us generate and send out the errors for each list of identifiers we give
        /// it: image buffers identifiers then metadata identifiers
        result = GenerateErrorFromIdentifiers(pFrameCbData, imageBufferIdentifiers, results, messageDescriptor);

        if (CDKResultSuccess == result)
        {
            result = GenerateErrorFromIdentifiers(pFrameCbData, metadataIdentifiers, results, messageDescriptor);
        }
    }
    else
    {
        CHX_LOG_ERROR("FrameCb Data is NULL! cannot process request error");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GenerateErrorFromIdentifiers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::GenerateErrorFromIdentifiers(
    ChiFeatureFrameCallbackData*        pFrameCbData,
    std::vector<ChiFeature2Identifier>  identifierList,
    ChiFeature2Messages                 notification,
    CHIMESSAGEDESCRIPTOR                messageDescriptor
    ) const
{
    CDKResult                       result            = CDKResultSuccess;
    ChiFeaturePortData*             pPortData         = NULL;
    ChiFeature2Identifier*          pPortIdentifier   = NULL;
    ChiFeature2RequestObject*       pFeatureReqObj    = pFrameCbData->pRequestObj;
    do
    {
        pPortIdentifier = (TRUE != identifierList.empty()) ? &identifierList.back() : NULL;
        pPortData       = GetPortData(pPortIdentifier);

        if (NULL != pPortData)
        {
            if (ChiFeature2PortType::ImageBuffer == pPortData->globalId.portType)
            {
                messageDescriptor.message.errorMessage.errorMessageCode = MessageCodeBuffer;

                if (NULL != pPortData->pTarget)
                {
                    CHX_LOG_VERBOSE("%s Creating buffer error from GID: [%d %d %d %d] using using "
                                    "stream:%p, format:%d",
                                    pFeatureReqObj->IdentifierString(),
                                    pPortIdentifier->session,
                                    pPortIdentifier->pipeline,
                                    pPortIdentifier->port,
                                    pPortIdentifier->portType,
                                    pPortData->pTarget->pChiStream,
                                    pPortData->pTarget->pChiStream->format);

                    messageDescriptor.message.errorMessage.pErrorStream = pPortData->pTarget->pChiStream;
                }
                else
                {
                    CHX_LOG_ERROR("%s Port data target is NULL! Not creating error!",
                                  pFeatureReqObj->IdentifierString());
                    result = CDKResultEInvalidPointer;
                }
            }
            else if (ChiFeature2PortType::MetaData == pPortData->globalId.portType)
            {
                CHX_LOG_VERBOSE("%s Creating metadata error from GID: [%d %d %d %d]",
                                pFeatureReqObj->IdentifierString(),
                                pPortIdentifier->session,
                                pPortIdentifier->pipeline,
                                pPortIdentifier->port,
                                pPortIdentifier->portType);

                messageDescriptor.message.errorMessage.errorMessageCode = MessageCodeResult;
                messageDescriptor.message.errorMessage.pErrorStream     = NULL;
            }
            else
            {
                CHX_LOG_ERROR("%s Unexpected port type:%d, not generating error",
                              pFeatureReqObj->IdentifierString(),
                              pPortData->globalId.portType);
                result = CDKResultEInvalidArg;
            }

            if (CDKResultSuccess == result)
            {
                notification.chiNotification.pChiMessages   = &messageDescriptor;
                notification.chiNotification.requestIndex   = pFrameCbData->requestId;
                notification.chiNotification.pIdentifier    = pPortIdentifier;

                result = HandleBufferAndResultError(&notification, pFrameCbData);
            }

            if (CDKResultSuccess == result)
            {
                CHX_LOG_VERBOSE("%s erasing GID: [%d %d %d %d]",
                                pFeatureReqObj->IdentifierString(),
                                pPortIdentifier->session,
                                pPortIdentifier->pipeline,
                                pPortIdentifier->port,
                                pPortIdentifier->portType);
                identifierList.erase((identifierList.end() - 1));
            }
        }
        else
        {
            CHX_LOG_ERROR("%s Port data is NULL! Not creating error!", pFeatureReqObj->IdentifierString());
            result = CDKResultENoMore;
        }
    } while ((CDKResultSuccess == result) && (TRUE != identifierList.empty()));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::HandleBufferAndResultError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::HandleBufferAndResultError(
    ChiFeature2Messages*           pMessage,
    ChiFeatureFrameCallbackData*   pFrameCbData
    ) const
{
    CDKResult                           result          = CDKResultSuccess;
    ChiFeature2BufferMetadataInfo*      pBufferMetaInfo = NULL;
    CHIERRORMESSAGECODE                 errorMessage    =
        pMessage->chiNotification.pChiMessages->message.errorMessage.errorMessageCode;

    ChiFeature2RequestObject*           pFeatureReqObj  = pFrameCbData->pRequestObj;

    if (NULL != pFeatureReqObj)
    {
        ChiFeatureSequenceData*  pRequestData = static_cast<ChiFeatureSequenceData*>(
            pFeatureReqObj->GetSequencePrivDataById(pFrameCbData->sequenceId, pFrameCbData->requestId));

        // Update the Target on Error
        if (MessageCodeResult == errorMessage)
        {
            ChiFeaturePipelineData* pPipelineData =
                (0 < pFrameCbData->pOutputPorts.size()) ? GetPipelineData(&pFrameCbData->pOutputPorts[0]) : NULL;

            if (NULL != pPipelineData)
            {
                CHITargetBufferManager*  pBufferManager = pPipelineData->pSettingMetaTbm;

                if (NULL != pRequestData)
                {
                    if (NULL != pBufferManager)
                    {
                        ChiMetadata* pInputMetadata = pRequestData->pInputMetadata;
                        result = pBufferManager->UpdateTarget(pRequestData->frameNumber,
                                                     pPipelineData->pPipeline->GetMetadataClientId(),
                                                     pInputMetadata,
                                                     ChiTargetStatus::Error,
                                                     NULL);
                    }
                    else
                    {
                        CHX_LOG_ERROR("%s pBufferManager is NULL! cannot update target on error",
                                      pFeatureReqObj->IdentifierString());
                        result = CDKResultEInvalidPointer;
                    }

                    pBufferManager = pPipelineData->pOutputMetaTbm;

                    if (NULL != pBufferManager)
                    {
                        result = pBufferManager->UpdateTarget(pRequestData->frameNumber,
                                                              pPipelineData->pPipeline->GetMetadataClientId(),
                                                              NULL,
                                                              ChiTargetStatus::Error,
                                                              NULL);
                        if (CDKResultSuccess == result)
                        {
                            CHX_LOG_VERBOSE("%s OutputMetaTbm setting port release acknowledged for identifier [%d %d %d %d]",
                                            pFeatureReqObj->IdentifierString(),
                                            pMessage->chiNotification.pIdentifier->session,
                                            pMessage->chiNotification.pIdentifier->pipeline,
                                            pMessage->chiNotification.pIdentifier->port,
                                            pMessage->chiNotification.pIdentifier->portType);
                            pFeatureReqObj->SetReleaseAcknowledgedForPort(*pMessage->chiNotification.pIdentifier,
                                                                          pFeatureReqObj->GetCurRequestId());
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("%s pBufferManager is NULL! cannot update target on error",
                                      pFeatureReqObj->IdentifierString());
                        result = CDKResultEInvalidPointer;
                    }
                }
                else
                {
                    CHX_LOG_ERROR("%s pRequestData is NULL! Cannot update target on error",
                                  pFeatureReqObj->IdentifierString());
                    result = CDKResultEInvalidPointer;
                }
            }
            else
            {
                CHX_LOG_ERROR("%s Pipeline data is NULL, cannot update target for metadata error",
                              pFeatureReqObj->IdentifierString());
                result = CDKResultEInvalidPointer;
            }
        }
        else
        {
            // Iterate through the output ports in the Frame CB Data to find the matching stream
            for (UINT portIdx = 0; portIdx < pFrameCbData->pOutputPorts.size(); portIdx++)
            {
                ChiFeaturePortData* pPortData = GetPortData(&pFrameCbData->pOutputPorts[portIdx]);
                if ((NULL != pPortData) && (NULL != pRequestData))
                {
                    if (NULL != pPortData->pTarget)
                    {
                        if (pPortData->pTarget->pChiStream ==
                            pMessage->chiNotification.pChiMessages->message.errorMessage.pErrorStream)
                        {
                            CHITargetBufferManager* pBufferManager = pPortData->pOutputBufferTbm;
                            if (NULL != pBufferManager)
                            {
                                result = pBufferManager->UpdateTarget(pRequestData->frameNumber,
                                                                      reinterpret_cast<UINT64>(pPortData->pTarget->pChiStream),
                                                                      NULL,
                                                                      ChiTargetStatus::Error,
                                                                      NULL);
                                if (CDKResultSuccess == result)
                                {
                                    OnBufferError(pFeatureReqObj,
                                                  &pPortData->globalId,
                                                  pFrameCbData->requestId);

                                    CHX_LOG_VERBOSE("%s OutputBufferTbm setting port release acknowledged for identifier "
                                                    "[%d %d %d %d]",
                                                    pFeatureReqObj->IdentifierString(),
                                                    pMessage->chiNotification.pIdentifier->session,
                                                    pMessage->chiNotification.pIdentifier->pipeline,
                                                    pMessage->chiNotification.pIdentifier->port,
                                                    pMessage->chiNotification.pIdentifier->portType);
                                    pFeatureReqObj->SetReleaseAcknowledgedForPort(*pMessage->chiNotification.pIdentifier,
                                                                                  pFeatureReqObj->GetCurRequestId());
                                }
                                break;
                            }
                            else
                            {
                                CHX_LOG_ERROR("%s pBufferManager is NULL! cannot update target on error",
                                              pFeatureReqObj->IdentifierString());
                                result = CDKResultEInvalidPointer;
                            }
                        }
                    }
                    else
                    {
                        CHX_LOG_WARN("%s Port Target is NULL, cannot create buffer error for this port",
                                      pFeatureReqObj->IdentifierString());
                    }
                }
                else
                {
                    CHX_LOG_ERROR("%s Port data is NULL, cannot check if buffer error corresponds to this port",
                                  pFeatureReqObj->IdentifierString());
                    result = CDKResultEInvalidPointer;
                }
            }
        }

        if (CDKResultSuccess == result)
        {
            ReleaseDependenciesOnDriverError(pMessage->chiNotification.pIdentifier,
                                             pFrameCbData,
                                             pFeatureReqObj);

            // If we are in the final stage, update the final buffer meta info and propagate the error
            if ((GetNumStages() - 1) == pFrameCbData->stageId)
            {
                // Only external sink ports will be available in final buffer meta info. It is not an error if we
                // don't get the final buffer meta info; this is the place in which internal buffer errors will be dropped
                pFeatureReqObj->GetFinalBufferMetadataInfo(*pMessage->chiNotification.pIdentifier,
                                                           &pBufferMetaInfo,
                                                           pFrameCbData->requestId);
                if (NULL != pBufferMetaInfo)
                {
                    CHX_LOG_INFO("%s, in final stage. Setting %s in final buffer/meta info and "
                                 "propagating error to graph",
                                 pFeatureReqObj->IdentifierString(),
                                 ChxUtils::ErrorMessageCodeToString(errorMessage));

                    // Set the error in the final buffer/meta info and induce the callback to the graph
                    (MessageCodeResult == errorMessage) ? (pBufferMetaInfo->metadataErrorPresent = TRUE) :
                        (pBufferMetaInfo->bufferErrorPresent = TRUE);

                    m_clientCallbacks.ChiFeature2ProcessMessage(pFeatureReqObj, pMessage);
                }
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Could not propagate error type:%s, encountered NULL parameter: pFeatureReqObj:%p, pFrameCbData:%p, ",
                      ChxUtils::ErrorMessageCodeToString(errorMessage), pFeatureReqObj, pFrameCbData);
        result = CDKResultEInvalidArg;
    }

    return result;

    }


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::CheckAndReturnSkippedBufferAsError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::CheckAndReturnSkippedBufferAsError(
    ChiFeature2RequestObject* pFeatureReqObj,
    UINT8                     requestId)
{
    CDKResult                           result              = CDKResultSuccess;
    const ChiFeature2RequestOutputInfo* pRequestOutputInfo  = NULL;
    UINT8                               numOutputsRequested = 0;
    ChiFeature2BufferMetadataInfo*      pBufferMetaInfo     = NULL;
    ChiFeatureSequenceData*             pRequestData        = static_cast<ChiFeatureSequenceData*>(
        pFeatureReqObj->GetSequencePrivData(ChiFeature2SequenceOrder::Current, requestId));

    if (NULL == pRequestData)
    {
        CHX_LOG_ERROR("pRequestData is NULL");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        result = pFeatureReqObj->GetExternalRequestOutput(&numOutputsRequested, &pRequestOutputInfo, requestId);
    }

    if (CDKResultSuccess == result)
    {
        for (UINT i = 0; i < numOutputsRequested; i++)
        {
            ChiFeaturePipelineData* pPipelineData = GetPipelineData(&(pRequestOutputInfo[i].pPortDescriptor->globalId));
            ChiFeaturePortData*     pPortData     = GetPortData(&(pRequestOutputInfo[i].pPortDescriptor->globalId));

            if ((NULL != pPipelineData) &&
                (NULL != pPortData) &&
                (NULL != pRequestOutputInfo[i].bufferMetadataInfo.hBuffer) &&
                (TRUE == pRequestOutputInfo[i].bufferMetadataInfo.bufferSkipped))
            {
                CHX_LOG_VERBOSE("pipeline:%s, port[%d]:%s, buffer is skipped, return as error to framework",
                    pPipelineData->pPipelineName, i, pPortData->pPortName);

                ChiFeature2Identifier portId = pRequestOutputInfo[i].pPortDescriptor->globalId;
                OnBufferError(pFeatureReqObj,
                              &portId,
                              requestId);

                CHIMESSAGEDESCRIPTOR messageDescriptor                      = {};
                messageDescriptor.pPrivData                                 = NULL;
                messageDescriptor.messageType                               = ChiMessageTypeError;
                messageDescriptor.message.errorMessage.frameworkFrameNum    = pRequestData->frameNumber;
                messageDescriptor.message.errorMessage.pErrorStream         = pPortData->pChiStream;
                messageDescriptor.message.errorMessage.errorMessageCode     = MessageCodeBuffer;

                ChiFeature2Notification             chiNotification         = {0};
                ChiFeature2Messages                 feature2Message         = {m_featureId, chiNotification, NULL};

                feature2Message.chiNotification.pChiMessages   = &messageDescriptor;
                feature2Message.chiNotification.requestIndex   = requestId;
                feature2Message.chiNotification.pIdentifier    = &pPortData->globalId;

                m_clientCallbacks.ChiFeature2ProcessMessage(pFeatureReqObj, &feature2Message);

                result = pFeatureReqObj->GetFinalBufferMetadataInfo(pPortData->globalId, &pBufferMetaInfo, requestId);

                if ((CDKResultSuccess == result) && (NULL != pBufferMetaInfo))
                {
                    pBufferMetaInfo->hBuffer       = NULL;
                    pBufferMetaInfo->bufferSkipped = FALSE;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetMatchingIdentifier
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Identifier* ChiFeature2Base::GetMatchingIdentifier(
    ChiFeatureFrameCallbackData*    pFrameCbData,
    const CHIMESSAGEDESCRIPTOR*     pMessageDescriptor)
{
    ChiFeature2Identifier* pIdentifier = NULL;

    for (UINT portIdx = 0; portIdx < pFrameCbData->pOutputPorts.size(); portIdx++)
    {
        ChiFeaturePortData* pPortData = GetPortData(&pFrameCbData->pOutputPorts[portIdx]);

        if (NULL != pPortData)
        {
            switch (pMessageDescriptor->message.errorMessage.errorMessageCode)
            {
                case MessageCodeResult:
                    if (ChiFeature2PortType::MetaData == pPortData->globalId.portType)
                    {
                        pIdentifier = &(pPortData->globalId);
                        break;
                    }
                    break;
                case MessageCodeBuffer:
                    if ((ChiFeature2PortType::ImageBuffer == pPortData->globalId.portType) &&
                        (pPortData->pTarget->pChiStream == pMessageDescriptor->message.errorMessage.pErrorStream))
                    {
                        pIdentifier = &(pPortData->globalId);
                        break;
                    }
                    break;
                default:
                    CHX_LOG_WARN("Not handling message type:%d, should only be called for buffer/result error",
                                 pMessageDescriptor->message.errorMessage.errorMessageCode);
                    break;
            }
        }
    }

    return pIdentifier;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::HandlePostJob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::HandlePostJob(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId)
{
    CDKResult                        result             = CDKResultSuccess;

    ChiFeature2UsecaseRequestObject* pUsecaseRequestObj = pRequestObject->GetUsecaseRequestObject();

    if (NULL != pUsecaseRequestObj)
    {
        UINT64 jobFrameNum = static_cast<UINT64>(pUsecaseRequestObj->GetAppFrameNumber() + requestId);

        CHX_LOG_VERBOSE("%s Post a job, handle %" PRIx64" requestId %d jobFrameNum %" PRIu64,
            pRequestObject->IdentifierString(),
            m_hFeatureJob,
            requestId,
            jobFrameNum);

        result = m_pThreadManager->PostJob(
            m_hFeatureJob,
            pRequestObject,
            jobFrameNum);

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("%s Failed to post a job, handle %" PRIx64", result %d jobFrameNum %" PRIu64,
                pRequestObject->IdentifierString(),
                m_hFeatureJob,
                result,
                jobFrameNum);
        }
    }
    else
    {
        CHX_LOG_ERROR("%s Failed to post a job, handle %" PRIx64" requestId %d",
            pRequestObject->IdentifierString(),
            m_hFeatureJob,
            requestId);

        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::ProcessResult(
    CHICAPTURERESULT*   pChiResult,
    VOID*               pPrivateCallbackData)
{
    CDK_UNUSED_PARAM(pPrivateCallbackData);

    CDKResult result = CDKResultSuccess;

    if ((NULL == m_clientCallbacks.ChiFeature2ProcessMessage) ||
        (NULL == pChiResult))
    {
        CHX_LOG_ERROR("Invalid argument: NULL m_clientCallbacks.ChiFeature2ProcessMessage = %p, pChiResult = %p",
            m_clientCallbacks.ChiFeature2ProcessMessage, pChiResult);
        result = CDKResultEInvalidArg;
    }

    // If feature graph manager destroy is in progress, we will not process result from driver
    if (TRUE == m_destroyInProgress)
    {
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeatureCombinedCallbackData* pCombinedCbData = static_cast<ChiFeatureCombinedCallbackData*>(pChiResult->pPrivData);
        if (NULL == pCombinedCbData)
        {
            CHX_LOG_ERROR("Cannot get frame callback data");
            result = CDKResultEInvalidArg;
        }

        if (CDKResultSuccess == result)
        {
            for (UINT8 resultIndex = 0; resultIndex < pCombinedCbData->numCallbackData; ++resultIndex)
            {
                ChiFeatureFrameCallbackData* pFrameCbData = pCombinedCbData->pCombinedCallbackData[resultIndex];
                ChiFeature2RequestObject*    pFeatureReqObj = static_cast<ChiFeature2RequestObject*>(pFrameCbData->pRequestObj);
                UINT32                       frameNumber = pFrameCbData->sequenceId;

                ChiFeature2Identifier   globalId;
                globalId.session = m_pSessionDescriptor->sessionId;
                ChiFeatureSessionData*  pSessionData = GetSessionData(&globalId);
                auto                    hSession = (NULL != pSessionData) ? pSessionData->pSession->GetSessionHandle() : 0;
                ChiFeaturePipelineData* pPipelineData = GetPipelineData(
                    (0 < pCombinedCbData->pCombinedCallbackData[0]->pOutputPorts.size()) ?
                    &pCombinedCbData->pCombinedCallbackData[0]->pOutputPorts[0] : NULL);

                auto                    hPipeline = (NULL != pPipelineData) ? pPipelineData->pPipeline->GetPipelineHandle() : 0;

                UINT                      appFrameNumber = pFeatureReqObj->GetUsecaseRequestObject()->GetAppFrameNumber();
                UINT32                    chiFrameNumber = pChiResult->frameworkFrameNum;
                UINT8                     requestId = pFrameCbData->requestId;
                ChiFeature2StageInfo      stageInfo = { pFrameCbData->stageId, pFrameCbData->stagesequenceId };
                BOOL                      hasMetadata = (NULL != pChiResult->pOutputMetadata);
                BOOL                      hasBuffers = (0 != pChiResult->numOutputBuffers);
                UINT32                    numOutputBuffers = pChiResult->numOutputBuffers;
                auto                      hFroHandle = pFeatureReqObj->GetHandle();

                BINARY_LOG(LogEvent::FT2_Base_ProcessResult, appFrameNumber, chiFrameNumber, requestId, stageInfo, hasMetadata,
                    hasBuffers, numOutputBuffers, hFroHandle, hPipeline, hSession);

                if (TRUE == hasMetadata)
                {
                    CHX_LOG_CONFIG("%s Result from Request:%d with metadata:%p",
                        pFeatureReqObj->IdentifierString(),
                        frameNumber,
                        pChiResult->pOutputMetadata);

                    ProcessMetadataCallback(pChiResult, pFrameCbData);
                }

                if (TRUE == hasBuffers)
                {
                    CHX_LOG_CONFIG("%s Result from Request:%d with %d buffer(s)",
                        pFeatureReqObj->IdentifierString(),
                        chiFrameNumber,
                        pChiResult->numOutputBuffers);

                    if (CHX_IS_VERBOSE_ENABLED())
                    {
                        ATRACE_ASYNC_END(pFeatureReqObj->IdentifierString(), chiFrameNumber);
                    }

                    ProcessBufferCallback(pChiResult, pFrameCbData);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessFeatureMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ProcessFeatureMessage(
    ChiFeature2MessageDescriptor* pFeatureMessage
    ) const
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == m_clientCallbacks.ChiFeature2ProcessMessage) ||
        (NULL == pFeatureMessage))
    {
        CHX_LOG_ERROR("Invalid argument: NULL m_clientCallbacks.ChiFeature2ProcessMessage = %p, pFeatureMessage = %p",
                      m_clientCallbacks.ChiFeature2ProcessMessage, pFeatureMessage);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2Notification     chiNotification = {0};
        ChiFeature2Messages         results         = {m_featureId, chiNotification, pFeatureMessage};
        ChiFeature2RequestObject*   pFeatureReqObj  = static_cast<ChiFeature2RequestObject*>(pFeatureMessage->pPrivData);
        result = m_clientCallbacks.ChiFeature2ProcessMessage(pFeatureReqObj, &results);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnPopulateConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnPopulateConfiguration(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult             result      = CDKResultSuccess;
    ChiFeature2StageInfo  stageInfo   = { 0 };
    UINT8                 requestId   = 0;

    UINT8                                numRequestOutputs  = 0;
    const ChiFeature2RequestOutputInfo*  pRequestOutputInfo = NULL;

    requestId       = pRequestObject->GetCurRequestId();
    pRequestObject->GetCurrentStageInfo(&stageInfo, requestId);
    pRequestObject->GetExternalRequestOutput(&numRequestOutputs, &pRequestOutputInfo, requestId);

    if (InvalidStageId == stageInfo.stageId)
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2PortIdList inputPortList  = { 0 };
        ChiFeature2PortIdList outputPortList = { 0 };

        result = GetInputPortsForStage(stageInfo.stageId, &inputPortList);

        if (CDKResultSuccess == result)
        {
            result = GetOutputPortsForStage(stageInfo.stageId, &outputPortList);
        }

        if (CDKResultSuccess == result)
        {
            result = PopulatePortConfiguration(pRequestObject,
                &inputPortList, &outputPortList);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PopulateConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::PopulateConfiguration(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;
    result = OnPopulateConfiguration(pRequestObject);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PopulatePortConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::PopulatePortConfiguration(
    ChiFeature2RequestObject*     pRequestObject,
    const ChiFeature2PortIdList*  pInputList,
    const ChiFeature2PortIdList*  pOutputList
    ) const
{
    CDKResult                  result          = CDKResultSuccess;
    UINT8                      requestId       = pRequestObject->GetCurRequestId();
    ChiMetadata*               pInputMetadata  = NULL;
    CHITARGETBUFFERINFOHANDLE  hSetting        = NULL;

    ChiFeatureSequenceData*  pRequestData = static_cast<ChiFeatureSequenceData*>(
        pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current, requestId));

    if (NULL != pOutputList)
    {
        for (UINT8 outputIndex = 0; outputIndex < pOutputList->numPorts; outputIndex++)
        {
            ChiFeature2Identifier    portDesc       = pOutputList->pPorts[outputIndex];
            ChiFeaturePipelineData*  pPipelineData  = GetPipelineData(&portDesc);

            if (NULL != pPipelineData)
            {
                if (NULL != pRequestData)
                {
                    hSetting = GetInputMetadataBuffer(
                        pPipelineData, pRequestData->frameNumber);
                }
                /// Assuming one metadata port. Need to override if more
                if (NULL != hSetting)
                {
                    CHIMETAHANDLE hMetadataBuffer = NULL;
                    result = GetMetadataBuffer(hSetting,
                        pPipelineData->metadataClientId, &hMetadataBuffer);

                    if (NULL != hMetadataBuffer)
                    {
                        pInputMetadata = GetMetadataManager()->GetMetadataFromHandle(hMetadataBuffer);

                        if (NULL == pInputMetadata)
                        {
                            CHX_LOG_ERROR("metadata associated with handle is NULL");
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("pointer to metadata buffer from target buffer info handle is NULL");
                    }
                }
                break;
            }
        }
    }

    if (NULL == pInputMetadata)
    {
        CHX_LOG_ERROR("metadata associated with handle is NULL");
        result = CDKResultEInvalidArg;
    }

    if ((NULL != pInputList) && (result == CDKResultSuccess))
    {
        for (UINT8 inputIndex = 0; inputIndex < pInputList->numPorts; ++inputIndex)
        {
            ChiFeature2Identifier               portidentifier      = pInputList->pPorts[inputIndex];
            const   ChiFeature2PortDescriptor*  pPortDescriptor     = GetPortDescriptorFromPortId(&portidentifier);
            ChiFeature2BufferMetadataInfo       inputBufferMetaInfo = { 0 };

            result = pRequestObject->SetPortDescriptor(ChiFeature2SequenceOrder::Current,
                ChiFeature2RequestObjectOpsType::InputConfiguration,
                &portidentifier, pPortDescriptor, requestId, 0);

            if (CDKResultSuccess == result)
            {
                pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                    &portidentifier, &inputBufferMetaInfo.hBuffer, &inputBufferMetaInfo.key, requestId, 0);

                pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputConfiguration,
                    &portidentifier, inputBufferMetaInfo.hBuffer, inputBufferMetaInfo.key, FALSE, requestId, 0);

                if (portidentifier.portType == ChiFeature2PortType::MetaData)
                {
                    PopulateConfigurationSettings(pRequestObject, &portidentifier, pInputMetadata);
                }
            }
            else
            {
                CHX_LOG_ERROR("Failed to set port descriptor for port %s", pPortDescriptor->pPortName);
            }
        }
    }

    if ((NULL != pOutputList) && (result == CDKResultSuccess))
    {
        ChiFeature2BufferMetadataInfo       outputBufferMetaInfo = { 0 };

        for (UINT8 outputIndex = 0; outputIndex < pOutputList->numPorts; ++outputIndex)
        {
            ChiFeature2Identifier               portidentifier  = pOutputList->pPorts[outputIndex];
            const ChiFeature2PortDescriptor*    pPortDescriptor = GetPortDescriptorFromPortId(&portidentifier);

            CHX_LOG_VERBOSE("Set output port config for port %s numPorts %d", pPortDescriptor->pPortName,
                    pOutputList->numPorts);

            if (CDKResultSuccess == result)
            {
                // Get buffer metadata handle for port
                ChiFeaturePipelineData* pPipelineData        = GetPipelineData(&portidentifier);
                ChiFeaturePortData*     pPortData            = GetPortData(&portidentifier);
                CHITargetBufferManager* pTargetBufferManager = NULL;

                if ((NULL != pPortData) && (NULL != pPortData->pOutputBufferTbm))
                {
                    pTargetBufferManager = pPortData->pOutputBufferTbm;
                }
                else if ((NULL != pPortData) && (NULL == pPortData->pTarget))
                {
                    continue;
                }
                else
                {
                    CHX_LOG_ERROR("Invalid Pointer pPortData: %p pOutputBufferTbm: %p",
                                  pPortData,
                                  (NULL == pPortData) ? NULL : pPortData->pOutputBufferTbm);
                    result = CDKResultEFailed;
                }
                if (portidentifier.portType == ChiFeature2PortType::MetaData)
                {
                    PopulateConfigurationSettings(pRequestObject, &portidentifier, pInputMetadata);
                }

                UINT8                                   numOutputsRequested = -1;
                const ChiFeature2RequestOutputInfo*     pRequestOutputInfo  = NULL;
                ChiFeatureSequenceData*                 pSequenceData       = static_cast<ChiFeatureSequenceData*>(
                    pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current, requestId));
                pRequestObject->GetExternalRequestOutput(&numOutputsRequested, &pRequestOutputInfo, requestId);

                if ((CDKResultSuccess == result) && (NULL != pSequenceData))
                {
                    for (UINT i = 0; i < numOutputsRequested; i++)
                    {
                        if (NULL != pRequestOutputInfo[i].bufferMetadataInfo.hBuffer)
                        {
                            if ((portidentifier == pRequestOutputInfo[i].pPortDescriptor->globalId) &&
                                (FALSE == pRequestObject->GetOutputGeneratedForPort(portidentifier, requestId)))
                            {
                                UINT64 targetId          = reinterpret_cast<UINT64>(pPortData->pChiStream);
                                CHISTREAMBUFFER* pBuffer = static_cast<CHISTREAMBUFFER*>(pTargetBufferManager->GetTarget(
                                    pRequestOutputInfo[i].bufferMetadataInfo.hBuffer, targetId));
                                result = pTargetBufferManager->ImportExternalTargetBuffer(
                                    pSequenceData->frameNumber, targetId, pBuffer);
                                if (CDKResultSuccess != result)
                                {
                                    CHX_LOG_ERROR("Import buffer failed Port = %d",
                                        portidentifier.port);
                                }
                            }
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("result: %u pSequenceData: %p",
                                  result,
                                  pSequenceData);
                    result = CDKResultEFailed;
                }

                if (CDKResultSuccess == result)
                {
                    result = pRequestObject->SetPortDescriptor(ChiFeature2SequenceOrder::Current,
                                                               ChiFeature2RequestObjectOpsType::OutputConfiguration,
                                                               &portidentifier,
                                                               pPortDescriptor,
                                                               requestId,
                                                               0);
                }

                if ((CDKResultSuccess == result) && (NULL != pPipelineData) && (NULL != pPortData))
                {
                    if (ChiFeature2PortType::MetaData == pPortData->globalId.portType)
                    {
                        outputBufferMetaInfo.hBuffer = GetOutputMetadataBuffer(pPipelineData, pSequenceData->frameNumber);
                        outputBufferMetaInfo.key     = pPipelineData->metadataClientId;
                    }
                    else
                    {
                        outputBufferMetaInfo.key     = reinterpret_cast<UINT64>(pPortData->pTarget->pChiStream);
                        outputBufferMetaInfo.hBuffer = GetOutputBufferHandle(pPortData, pSequenceData->frameNumber);
                    }
                    pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::OutputConfiguration,
                        &portidentifier, outputBufferMetaInfo.hBuffer, outputBufferMetaInfo.key, FALSE, requestId, 0);
                }
            }
            else
            {
                CHX_LOG_ERROR("Failed to set port descriptor for port %s", pPortDescriptor->pPortName);
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Output list is NULL");
        result = CDKResultEInvalidArg;
    }

    if (NULL != pInputMetadata)
    {
        pRequestData->pInputMetadata = pInputMetadata;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetDependencyListFromStageDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const ChiFeature2InputDependency* ChiFeature2Base::GetDependencyListFromStageDescriptor(
    const ChiFeature2StageDescriptor*  pStageDescriptor,
    UINT8                              sessionIndex,
    UINT8                              pipelineIndex,
    UINT8                              listIndex
    ) const
{
    const ChiFeature2InputDependency* pDependency = NULL;

    if (NULL != pStageDescriptor)
    {
        if (sessionIndex < pStageDescriptor->numDependencyConfigDescriptor)
        {
            ChiFeature2SessionInfo sessionInfo = pStageDescriptor->pDependencyConfigDescriptor[sessionIndex];

            if (pipelineIndex < sessionInfo.numPipelines)
            {
                ChiFeature2PipelineInfo                 pipelineInfo    = sessionInfo.pPipelineInfo[pipelineIndex];
                ChiFeature2DependencyConfigDescriptor*  pDescriptorList = reinterpret_cast
                    <ChiFeature2DependencyConfigDescriptor*>(pipelineInfo.handle);

                if (listIndex < pDescriptorList->numInputDependency)
                {
                    pDependency = &pDescriptorList->pInputDependencyList[listIndex];
                }
                else
                {
                    CHX_LOG_ERROR("listIndex %d is greater than total number of lists %d", listIndex,
                        pDescriptorList->numInputDependency);
                }
            }
            else
            {
                CHX_LOG_ERROR("Pipeline index %d is greater than total number of pipelines %d", pipelineIndex,
                    sessionInfo.numPipelines);
            }
        }
        else
        {
            CHX_LOG_ERROR("Session index %d is greater than total number of sessions %d", sessionIndex,
                pStageDescriptor->numDependencyConfigDescriptor);
        }
    }
    else
    {
        CHX_LOG_ERROR("Stage descriptor is NULL!");
    }
    return pDependency;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::SetNextStageInfoFromStageDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::SetNextStageInfoFromStageDescriptor(
    ChiFeature2RequestObject*           pRequestObject,
    const ChiFeature2StageDescriptor*   pStageDescriptor,
    UINT8                               stageSequenceId,
    UINT8                               maxDependencies
    ) const
{
    CDKResult result = CDKResultSuccess;

    result = pRequestObject->SetNextStageInfo(pStageDescriptor->stageId,
        stageSequenceId,
        pStageDescriptor->numDependencyConfigDescriptor,
        pStageDescriptor->pDependencyConfigDescriptor,
        maxDependencies,
        pRequestObject->GetCurRequestId());

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ProcessDependency(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult                   result           = CDKResultSuccess;
    ChiFeature2DependencyTable  dependencyTable  = { 0, NULL };

    ChiFeature2DependencyBatch* pBatches  = static_cast<ChiFeature2DependencyBatch*>
        (ChxUtils::Calloc(sizeof(ChiFeature2DependencyBatch) * pRequestObject->GetNumRequests()));
    if (NULL == pBatches)
    {
        CHX_LOG_ERROR("Out Of memory: NumBatch = %d", pRequestObject->GetNumRequests());
        result = CDKResultENoMemory;
    }

    if (CDKResultSuccess == result)
    {
        dependencyTable.pBatches           = pBatches;
        for (UINT8 requestId = 0; requestId < pRequestObject->GetNumRequests(); requestId++)
        {
            ChiFeature2DependencyBatch* pBatch              = &pBatches[requestId];
            ChiFeature2SessionInfo*     pConfigInfo         = NULL;
            UINT8                       numDependencyConfig = pRequestObject->GetDependencyConfig(
                ChiFeature2SequenceOrder::Current, &pConfigInfo, requestId);

            for (UINT8 sessionIdx = 0; sessionIdx < numDependencyConfig; sessionIdx++)
            {
                ChiFeature2SessionInfo* pSessionConfigInfo = &pConfigInfo[sessionIdx];
                UINT32                  sessionId          = pSessionConfigInfo->sessionId;

                for (UINT8 pipelineIdx = 0; pipelineIdx < pSessionConfigInfo->numPipelines; pipelineIdx++)
                {
                    ChiFeature2DependencyConfigInfo* pDependencyConfigInfo =
                        reinterpret_cast<ChiFeature2DependencyConfigInfo*>(
                            pSessionConfigInfo->pPipelineInfo[pipelineIdx].handle);

                    UINT8                  numDependencies  = pDependencyConfigInfo->numInputDependency;
                    ChiFeature2Dependency* pDependencies    = static_cast<ChiFeature2Dependency*>
                        (ChxUtils::Calloc(sizeof(ChiFeature2Dependency) * numDependencies));

                    if (NULL == pDependencies)
                    {
                        CHX_LOG_ERROR("Out Of memory: numInputsDependencyList = %d", numDependencies);
                        result = CDKResultENoMemory;
                    }

                    if (CDKResultSuccess == result)
                    {
                        pBatch->pDependencies = pDependencies;
                        pBatch->batchIndex    = dependencyTable.numBatches;
                        for (UINT8 listIndex = 0; listIndex < numDependencies; listIndex++)
                        {
                            UINT8                  numPorts = pDependencyConfigInfo->pInputDependency[listIndex].num;
                            ChiFeature2Identifier* pPorts   = static_cast<ChiFeature2Identifier*>
                                (ChxUtils::Calloc(sizeof(ChiFeature2Identifier) * numPorts));
                            if (NULL == pPorts)
                            {
                                CHX_LOG_ERROR("Out Of memory: numPorts = %d", numPorts);
                                result = CDKResultENoMemory;
                            }

                            if (CDKResultSuccess == result)
                            {
                                pDependencies[pBatch->numDependencies].dependencyIndex = listIndex;
                                pDependencies[pBatch->numDependencies].pPorts          = pPorts;

                                for (UINT8 portIndex = 0; portIndex < numPorts; ++portIndex)
                                {
                                    const ChiFeature2Identifier* pKey =
                                        &pDependencyConfigInfo->pInputDependency[listIndex].pPortDescriptor[portIndex].globalId;

                                    if (ChiFeature2PortDirectionType::ExternalInput == pKey->portDirectionType)
                                    {
                                        pPorts[pDependencies[pBatch->numDependencies].numPorts] = *pKey;
                                        pDependencies[pBatch->numDependencies].numPorts++;
                                    }
                                }
                                pBatch->numDependencies++;
                            }
                        }
                    }
                }
            }
            dependencyTable.numBatches++;
        }

        if (CDKResultSuccess == result)
        {
            if (0 != dependencyTable.numBatches)
            {
                ChiFeature2MessageDescriptor featureMessage;

                ChxUtils::Memset(&featureMessage, 0, sizeof(ChiFeature2MessageDescriptor));
                featureMessage.messageType               = ChiFeature2MessageType::GetInputDependency;
                featureMessage.message.getDependencyData = dependencyTable;
                featureMessage.pPrivData                 = pRequestObject;

                ProcessFeatureMessage(&featureMessage);
            }
        }

        for (UINT8 requestId = 0; requestId < dependencyTable.numBatches; requestId++)
        {
            if (NULL != dependencyTable.pBatches[requestId].pDependencies)
            {
                for (UINT8 dependencyId = 0; dependencyId < dependencyTable.pBatches[requestId].numDependencies; dependencyId++)
                {
                    if (NULL != dependencyTable.pBatches[requestId].pDependencies[dependencyId].pPorts)
                    {
                        ChxUtils::Free(
                            dependencyTable.pBatches[requestId].pDependencies[dependencyId].pPorts);
                        dependencyTable.pBatches[requestId].pDependencies[dependencyId].pPorts = NULL;
                    }
                }
                ChxUtils::Free(dependencyTable.pBatches[requestId].pDependencies);
                dependencyTable.pBatches[requestId].pDependencies = NULL;
            }
        }
        if (NULL != dependencyTable.pBatches)
        {
            ChxUtils::Free(dependencyTable.pBatches);
            dependencyTable.pBatches = NULL;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessReleaseDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ProcessReleaseDependency(
    ChiFeature2RequestObject*     pRequestObject,
    UINT8                         requestId,
    UINT8                         sequenceId,
    ChiFeature2StageInfo*         pStageInfo
    ) const
{
    CDKResult                   result = CDKResultSuccess;
    ChiFeature2DependencyBatch  batch  = { requestId, 0, NULL };
    ChiFeature2SessionInfo*     pConfigInfo         = NULL;

    for (UINT8 curSequenceId = 0; curSequenceId <= sequenceId; ++curSequenceId)
    {
        UINT8 numDependencyConfig = pRequestObject->GetDependencyConfigBySequenceId(
            curSequenceId, &pConfigInfo, requestId);

        if (NULL == pConfigInfo)
        {
            result = CDKResultEFailed;
            CHX_LOG_ERROR("Cannot get Dependency Config for request = %d Sequence = %d", requestId, curSequenceId);
        }

        if (CDKResultSuccess == result)
        {
            batch.batchIndex = requestId;
            batch.numDependencies = 0;
            for (UINT8 sessionIdx = 0; sessionIdx < numDependencyConfig; sessionIdx++)
            {
                ChiFeature2SessionInfo* pSessionConfigInfo = &pConfigInfo[sessionIdx];
                UINT32                  sessionId          = pSessionConfigInfo->sessionId;

                for (UINT8 pipelineIdx = 0; pipelineIdx < pSessionConfigInfo->numPipelines; pipelineIdx++)
                {
                    ChiFeature2DependencyConfigInfo* pDependencyConfigInfo =
                        reinterpret_cast<ChiFeature2DependencyConfigInfo*>(
                            pSessionConfigInfo->pPipelineInfo[pipelineIdx].handle);

                    UINT8                  numDependencies  = pDependencyConfigInfo->numInputDependency;
                    ChiFeature2Dependency* pDependencies    = static_cast<ChiFeature2Dependency*>
                        (ChxUtils::Calloc(sizeof(ChiFeature2Dependency) * numDependencies));
                    if (NULL == pDependencies)
                    {
                        CHX_LOG_ERROR("Out Of memory: numInputsDependencyList = %d", numDependencies);
                        result = CDKResultENoMemory;
                    }

                    if (CDKResultSuccess == result)
                    {
                        batch.pDependencies = pDependencies;
                        for (UINT8 listIndex = 0; listIndex < numDependencies; listIndex++)
                        {
                            UINT8                  numPorts = pDependencyConfigInfo->pInputDependency[listIndex].num;
                            ChiFeature2Identifier* pPorts   = static_cast<ChiFeature2Identifier*>
                                (ChxUtils::Calloc(sizeof(ChiFeature2Identifier) * numPorts));
                            if (NULL == pPorts)
                            {
                                CHX_LOG_ERROR("Out Of memory: numPorts = %d", numPorts);
                                result = CDKResultENoMemory;
                            }

                            if (CDKResultSuccess == result)
                            {
                                pDependencies[batch.numDependencies].dependencyIndex = listIndex;
                                pDependencies[batch.numDependencies].pPorts          = pPorts;

                                ChiFeature2PortBufferMetadataInfo dependencyList =
                                    pDependencyConfigInfo->pInputDependency[listIndex];

                                BOOL sendToGraph = FALSE;

                                for (UINT8 portIndex = 0; portIndex < numPorts; ++portIndex)
                                {
                                    if (ChiFeature2PortBufferStatus::DependencyReleased !=
                                        pDependencyConfigInfo->pInputDependency[listIndex].pPortBufferStatus[portIndex])
                                    {
                                        const ChiFeature2Identifier* pKey =
                                            &dependencyList.pPortDescriptor[portIndex].globalId;

                                        if (ChiFeature2PortDirectionType::ExternalInput == pKey->portDirectionType)
                                        {
                                            if (TRUE == OnReleaseDependencies(pKey, listIndex,
                                                pStageInfo, pRequestObject))
                                            {
                                                pDependencies[batch.numDependencies].dependencyIndex  = listIndex;
                                                pPorts[pDependencies[batch.numDependencies].numPorts] = *pKey;
                                                pDependencies[batch.numDependencies].numPorts++;

                                                CHX_LOG_INFO("%s Release external dependency for port %s index %d",
                                                             pRequestObject->IdentifierString(),
                                                             dependencyList.pPortDescriptor[portIndex].pPortName,
                                                             listIndex);

                                                pRequestObject->SetBufferStatus(
                                                    ChiFeature2RequestObjectOpsType::InputDependency, pKey, curSequenceId,
                                                    ChiFeature2PortBufferStatus::DependencyReleased, requestId, listIndex);

                                                sendToGraph = TRUE;
                                            }
                                        }
                                        else if (ChiFeature2PortDirectionType::InternalInput == pKey->portDirectionType)
                                        {
                                            if (TRUE == OnReleaseDependencies(pKey, listIndex, pStageInfo, pRequestObject))
                                            {
                                                CHITARGETBUFFERINFOHANDLE hBuffer =
                                                    dependencyList.phTargetBufferHandle[portIndex];

                                                CHX_LOG_INFO("%s Release internal dependency for port %s hBuffer %p",
                                                    pRequestObject->IdentifierString(),
                                                    dependencyList.pPortDescriptor[portIndex].pPortName,
                                                    hBuffer);

                                                if (NULL != hBuffer)
                                                {
                                                    CHITargetBufferManager* pManager =
                                                        CHITargetBufferManager::GetTargetBufferManager(hBuffer);

                                                    if (NULL != pManager)
                                                    {
                                                        pManager->ReleaseTargetBuffer(hBuffer);
                                                        pRequestObject->SetBufferStatus(
                                                            ChiFeature2RequestObjectOpsType::InputDependency, pKey,
                                                            curSequenceId,
                                                            ChiFeature2PortBufferStatus::DependencyReleased,
                                                            requestId, listIndex);
                                                    }
                                                    pRequestObject->SetReleaseAcknowledgedForPort(
                                                            dependencyList.pPortDescriptor[portIndex].globalId, requestId);
                                                }
                                            }
                                        }
                                    }
                                }

                                if (TRUE == sendToGraph)
                                {
                                    batch.numDependencies++;
                                }
                                else
                                {
                                    ChxUtils::Free(pDependencies[batch.numDependencies].pPorts);
                                    pDependencies[batch.numDependencies].pPorts = NULL;
                                }
                            }
                        }
                    }
                }
            }

            if ((0 < batch.numDependencies) && (CDKResultSuccess == result))
            {
                CHX_LOG_INFO("%s %s: numDependencies = %d requestId = %d SequenceId = %d",
                             pRequestObject->IdentifierString(),
                             m_pFeatureName, batch.numDependencies, requestId, curSequenceId);

                ChiFeature2MessageDescriptor featureMessage;

                ChxUtils::Memset(&featureMessage, 0, sizeof(ChiFeature2MessageDescriptor));
                featureMessage.messageType                   = ChiFeature2MessageType::ReleaseInputDependency;
                featureMessage.message.releaseDependencyData = batch;
                featureMessage.pPrivData                     = pRequestObject;

                ProcessFeatureMessage(&featureMessage);
            }

            if (NULL != batch.pDependencies)
            {
                for (UINT8 dependencyId = 0; dependencyId < batch.numDependencies; dependencyId++)
                {
                    if (NULL != batch.pDependencies[dependencyId].pPorts)
                    {
                        ChxUtils::Free(batch.pDependencies[dependencyId].pPorts);
                        batch.pDependencies[dependencyId].pPorts = NULL;
                    }
                }
                ChxUtils::Free(batch.pDependencies);
                batch.pDependencies = NULL;
            }
        }
    }

    // For stages with multiple sequences, signal that the next stage sequence can begin processing
    pRequestObject->NotifyFinalResultReceived(requestId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessMetadataCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ProcessMetadataCallback(
    CHICAPTURERESULT*               pChiResult,
    ChiFeatureFrameCallbackData*    pFrameCbData)
{

    CDKResult                    result                 = CDKResultSuccess;
    ChiFeature2RequestObject*    pFeatureReqObj         = NULL;
    BOOL                         sendMetadata           = FALSE;
    UINT32                       metadataPortIndex      = InvalidIndex;
    ChiFeature2Identifier        metadataPortIdentifier = { 0 };
    ChiFeature2StageInfo         stageInfo              = { 0 };

    UINT8              resultId     = pFrameCbData->requestId;

    pFeatureReqObj = static_cast<ChiFeature2RequestObject*>(pFrameCbData->pRequestObj);

    ChiFeatureSequenceData* pRequestData = static_cast<ChiFeatureSequenceData*>(
        pFeatureReqObj->GetSequencePrivDataById(pFrameCbData->sequenceId, pFrameCbData->requestId));

    if (NULL == pRequestData)
    {
        CHX_LOG_ERROR("Cannot get request data from request object");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        stageInfo.stageId           = pFrameCbData->stageId;
        stageInfo.stageSequenceId   = pFrameCbData->stagesequenceId;

        // Move metadata ready for consumption
        ChiMetadata*  pOutputMeta    = m_pMetadataManager->GetMetadataFromHandle(pChiResult->pOutputMetadata);
        ChiMetadata*  pInputMetadata = m_pMetadataManager->GetMetadataFromHandle(pChiResult->pInputMetadata);

        ChiFeaturePipelineData* pPipelineData =
            (0 < pFrameCbData->pOutputPorts.size()) ? GetPipelineData(&pFrameCbData->pOutputPorts[0]) : NULL;

        if (NULL != pPipelineData)
        {
            CHITargetBufferManager*  pBufferManager = pPipelineData->pSettingMetaTbm;

            if (NULL != pBufferManager)
            {
                pBufferManager = pPipelineData->pSettingMetaTbm;
                pBufferManager->UpdateTarget(pChiResult->frameworkFrameNum,
                    pPipelineData->metadataClientId, pInputMetadata, ChiTargetStatus::Ready,
                    NULL);
            }
        }

        // Filter out external ports for metadata callback

        for (UINT32 portIndex = 0; portIndex < pFrameCbData->pOutputPorts.size(); ++portIndex)
        {
            if (ChiFeature2PortType::MetaData == pFrameCbData->pOutputPorts[portIndex].portType)
            {
                CHITARGETBUFFERINFOHANDLE       hBuffer = NULL;
                ChiFeature2BufferMetadataInfo*  pBufferMetaInfo = NULL;

                metadataPortIdentifier  = pFrameCbData->pOutputPorts[portIndex];
                metadataPortIndex       = portIndex;

                sendMetadata = OnMetadataResult(pFeatureReqObj, resultId, &stageInfo,
                    &metadataPortIdentifier, pOutputMeta, pChiResult->frameworkFrameNum, pRequestData);

                if (FALSE == sendMetadata)
                {
                    CHX_LOG_WARN("External PortId %d %d %d not requested for frame %d", metadataPortIdentifier.session,
                        metadataPortIdentifier.pipeline, metadataPortIdentifier.port, pRequestData->frameNumber);
                }
            }
        }

        if (TRUE == sendMetadata)
        {
            ChiFeature2MessageDescriptor metadataResult;

            metadataResult.messageType                  = ChiFeature2MessageType::ResultNotification;
            metadataResult.message.result.resultIndex   = resultId;
            metadataResult.message.result.numPorts      = 1;
            metadataResult.message.result.pPorts        = &metadataPortIdentifier;
            metadataResult.pPrivData                    = pFeatureReqObj;
            ProcessFeatureMessage(&metadataResult);
        }

        if (metadataPortIndex != InvalidIndex)
        {
            pFrameCbData->pOutputPorts.erase((pFrameCbData->pOutputPorts.begin() + metadataPortIndex));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::ProcessBufferCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::ProcessBufferCallback(
    CHICAPTURERESULT*               pChiResult,
    ChiFeatureFrameCallbackData*    pFrameCbData)
{
    UINT8                       resultId        = pFrameCbData->requestId;
    ChiFeature2RequestObject*   pFeatureReqObj  = static_cast<ChiFeature2RequestObject*>(pFrameCbData->pRequestObj);
    CDKResult                   result          = CDKResultSuccess;
    ChiFeature2StageInfo        stageInfo       = { 0 };
    ChiFeature2Result*          pResult         = NULL;
    ChiFeature2Identifier*      pOutputPorts    =
        static_cast<ChiFeature2Identifier*>(CHX_CALLOC(sizeof(ChiFeature2Identifier) *
        pChiResult->numOutputBuffers));
    UINT8                       numBufferErrors = 0;

    if (NULL != pOutputPorts)
    {
        ChiFeature2MessageDescriptor featureMessage;
        ChiFeatureSequenceData*      pRequestData = NULL;
        ChxUtils::Memset(&featureMessage, 0, sizeof(featureMessage));

        featureMessage.messageType    = ChiFeature2MessageType::ResultNotification;
        featureMessage.pPrivData      = pFeatureReqObj;

        pResult                       = &featureMessage.message.result;
        pResult->numPorts    = 0;
        pResult->pPorts      = NULL;
        pResult->resultIndex = resultId;
        pResult->pPorts      = pOutputPorts;

        stageInfo.stageId           = pFrameCbData->stageId;
        stageInfo.stageSequenceId   = pFrameCbData->stagesequenceId;

        pRequestData = static_cast<ChiFeatureSequenceData*>(
            pFeatureReqObj->GetSequencePrivDataById(pFrameCbData->sequenceId,
                pFrameCbData->requestId));

        if (NULL == pRequestData)
        {
            CHX_LOG_INFO("No sequence private data sequenceId = %d batchRequestId = %d",
                pFrameCbData->sequenceId, pFrameCbData->requestId);
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess == result)
        {
            for (UINT bufIdx = 0; bufIdx < pChiResult->numOutputBuffers; bufIdx++)
            {
                UINT8 found = FALSE;

                if (0 == pChiResult->pOutputBuffers[bufIdx].bufferStatus)
                {
                    for (UINT portIdx = 0; portIdx < pFrameCbData->pOutputPorts.size(); portIdx++)
                    {
                        ChiFeaturePortData* pPortData = GetPortData(&pFrameCbData->pOutputPorts[portIdx]);
                        BOOL                sendToGraph = FALSE;

                        if (NULL != pPortData)
                        {
                            if (pPortData->pTarget->pChiStream == pChiResult->pOutputBuffers[bufIdx].pStream)
                            {
                                sendToGraph = OnBufferResult(pFeatureReqObj, resultId, &stageInfo,
                                    &pPortData->globalId, &pChiResult->pOutputBuffers[bufIdx], pRequestData->frameNumber,
                                    pRequestData);

                                if (TRUE == sendToGraph)
                                {
                                    pOutputPorts[pResult->numPorts++] = pPortData->globalId;
                                }
                                else
                                {
                                    pFeatureReqObj->SetProcessingCompleteForPort(pPortData->globalId);
                                }
                                pFrameCbData->pOutputPorts.erase((pFrameCbData->pOutputPorts.begin() + portIdx));
                                found = TRUE;
                                break;
                            }
                        }
                    }

                    if (FALSE == found)
                    {
                        CHX_LOG_ERROR("Cannot Match Buffer and Ports BufId = %d", bufIdx);
                    }
                }
                else
                {
                    numBufferErrors++;
                }
            }

            // If the result is errored out, don't try and do anything with the FRO, it could be deleted by now.
            // Error handling will take care of injecting result in case of buffer error
            if (pChiResult->numOutputBuffers != numBufferErrors)
            {
                if (TRUE == pFrameCbData->pOutputPorts.empty())
                {
                    // If we have received all outputs from the last stage, we can go to notification pending state
                    if ((GetNumStages() - 1) == pFrameCbData->stageId)
                    {
                        pFeatureReqObj->SetCurRequestState(ChiFeature2RequestState::OutputNotificationPending, resultId);
                    }

                    if (0 != pFrameCbData->pInputPorts.size())
                    {
                        ProcessReleaseDependency(pFeatureReqObj, pFrameCbData->requestId, pFrameCbData->sequenceId, &stageInfo);
                    }

                    // Last buffer is internal stream buffer. Graph will not be taking care of FRO in this case.
                    // Hence featureBase itself needs to processrequest.
                    if (pResult->numPorts == 0)
                    {
                        // ProcessRequest in OutputNotificationPending
                        result = ProcessRequest(pFeatureReqObj);
                        if (CDKResultSuccess == result)
                        {
                            // ProcessRequest in Complete
                            result = ProcessRequest(pFeatureReqObj);
                            if (CDKResultSuccess != result)
                            {
                                CHX_LOG_ERROR("ProcessRequest() failed with result %d", result);
                            }
                        }
                        else
                        {
                            CHX_LOG_ERROR("ProcessRequest() failed with result %d", result);
                        }
                    }
                }

                result = CheckAndReturnSkippedBufferAsError(pFeatureReqObj, resultId);

                ProcessFeatureMessage(&featureMessage);
            }
            else
            {
                CHX_LOG_INFO("Buffer callback had:%u errors, not propagating buffer callback, error handling will take care",
                              numBufferErrors);
            }

            CHX_FREE(pOutputPorts);

            pOutputPorts = NULL;
        }


    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::GetDependency(
    ChiFeature2RequestObject*           pRequestObject)
{
    CDKResult result      = CDKResultSuccess;
    BOOL      changeState = FALSE;

    ChiFeature2SessionInfo* pConfigInfo;
    UINT8                   requestId             = pRequestObject->GetCurRequestId();
    UINT8                   numDependencyConfig   = pRequestObject->GetDependencyConfig(
        ChiFeature2SequenceOrder::Current, &pConfigInfo, requestId);

    for (UINT8 sessionIdx = 0; sessionIdx < numDependencyConfig; sessionIdx++)
    {
        ChiFeature2SessionInfo* pSessionConfigInfo = &pConfigInfo[sessionIdx];
        UINT32                  sessionId          = pSessionConfigInfo->sessionId;

        for (UINT8 pipelineIdx = 0; pipelineIdx < pSessionConfigInfo->numPipelines; pipelineIdx++)
        {
            UINT8 numBufMeta = 0;
            UINT8 numPorts   = 0;
            std::vector<ChiFeature2Identifier> srcPorts;
            std::vector<ChiFeature2Identifier> dstports;

            ChiFeature2DependencyConfigInfo* pDependencyConfigInfo =
                reinterpret_cast<ChiFeature2DependencyConfigInfo*>(
                    pSessionConfigInfo->pPipelineInfo[pipelineIdx].handle);

            UINT8 numInputsDependencyList = pDependencyConfigInfo->numInputDependency;

            for (UINT8 listIndex = 0; listIndex < numInputsDependencyList; listIndex++)
            {
                for (UINT8 portIndex = 0; portIndex < pDependencyConfigInfo->pInputDependency[listIndex].num; ++portIndex)
                {
                    ChiFeature2Identifier key =
                        pDependencyConfigInfo->pInputDependency[listIndex].pPortDescriptor[portIndex].globalId;
                    if (ChiFeature2PortDirectionType::ExternalInput == key.portDirectionType)
                    {
                        changeState = TRUE;
                    }
                    else if ((ChiFeature2PortDirectionType::InternalInput == key.portDirectionType) &&
                             (ChiFeature2PortBufferStatus::DependencyPending ==
                        pDependencyConfigInfo->pInputDependency[listIndex].pPortBufferStatus[portIndex]))
                    {
                        const ChiFeature2PortDescriptor* pPortDesc = NULL;
                        // Get the latest available buffer from the requested port
                        // SH: Add capability to get buffer with given request Id
                        const ChiFeature2PortDescriptor* pSrcPortDescriptor =
                            GetSourcePort(&pDependencyConfigInfo->pInputDependency[listIndex].pPortDescriptor[portIndex]);

                        if (NULL != pSrcPortDescriptor)
                        {
                            pRequestObject->GetPortDescriptor(ChiFeature2SequenceOrder::Current,
                                ChiFeature2RequestObjectOpsType::InputDependency,
                                &key, &pPortDesc, requestId, 0);
                        }

                        if (NULL != pSrcPortDescriptor && NULL != pPortDesc)
                        {
                            srcPorts.push_back(pSrcPortDescriptor->globalId);
                            dstports.push_back(key);
                        }
                    }
                }
                numBufMeta++;
            }

            if ((0 != srcPorts.size()) && (0 != dstports.size()))
            {
                ChiFeatureBufferData* pBufferData = static_cast<ChiFeatureBufferData*>
                    (ChxUtils::Calloc(sizeof(ChiFeatureBufferData) * srcPorts.size()));

                if (NULL != pBufferData)
                {
                    for (UINT8 portIndex = 0; portIndex < srcPorts.size(); ++portIndex)
                    {
                        pBufferData[portIndex].globalId    = srcPorts[portIndex];

                        pBufferData[portIndex].offset = 0;

                        pBufferData[portIndex].numBufmeta  = 1;
                        pBufferData[portIndex].pBufferMeta = static_cast<ChiFeature2BufferMetadataInfo*>
                            (ChxUtils::Calloc(sizeof(ChiFeature2BufferMetadataInfo) * pBufferData[portIndex].numBufmeta));
                        if (NULL == pBufferData[portIndex].pBufferMeta)
                        {
                            result = CDKResultEFailed;
                        }
                    }

                    if (CDKResultSuccess == result)
                    {
                        result = GetBufferData(pRequestObject, srcPorts.size(), pBufferData);
                    }

                    if (CDKResultSuccess == result)
                    {
                        for (UINT8 portIndex = 0; portIndex < dstports.size(); ++portIndex)
                        {
                            if (NULL != pBufferData[portIndex].pBufferMeta[0].hBuffer)
                            {
                                pRequestObject->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                                    &dstports[portIndex], pBufferData[portIndex].pBufferMeta[0].hBuffer,
                                    pBufferData[portIndex].pBufferMeta[0].key, FALSE, requestId, 0);
                            }
                        }
                    }
                    for (UINT8 portIndex = 0; portIndex < srcPorts.size(); ++portIndex)
                    {
                        if (NULL != pBufferData[portIndex].pBufferMeta)
                        {
                            ChxUtils::Free(pBufferData[portIndex].pBufferMeta);
                            pBufferData[portIndex].pBufferMeta = NULL;
                        }
                    }
                    ChxUtils::Free(pBufferData);
                    pBufferData = NULL;
                }
            }

            if (TRUE == changeState)
            {
                pRequestObject->SetCurRequestState(ChiFeature2RequestState::InputResourcePending, requestId);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetSourcePort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const ChiFeature2PortDescriptor* ChiFeature2Base::GetSourcePort(
    const ChiFeature2PortDescriptor* pSinkPortDescriptor
    ) const
{
    const ChiFeature2PortDescriptor* pSrcPortDescriptor = NULL;

    for (UINT8 linkIndex = 0 ; linkIndex < m_numInternalLinks; ++linkIndex)
    {
        if (pSinkPortDescriptor->globalId == m_pInternalLinkDesc[linkIndex].pSinkPort->globalId)
        {
            pSrcPortDescriptor = m_pInternalLinkDesc[linkIndex].pSrcPort;
        }
    }

    if (NULL == pSrcPortDescriptor)
    {
        CHX_LOG_ERROR("No source port found for sink %s", pSinkPortDescriptor->pPortName);
    }

    return pSrcPortDescriptor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetZSLOffsets
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<INT32> ChiFeature2Base::GetZSLOffsets(
    ChiFeature2RequestObject*         pRequestObject,
    std::vector<UINT32>&              rProducerList,
    std::vector<UINT32>&              rConsumerList,
    const ChiFeature2Identifier*      pSettingsPort
    ) const
{
    std::vector<INT32> offsets;
    UINT8   numZSLRequests        = pRequestObject->GetNumRequests();
    UINT32  lastZSLFrameNumber    = pRequestObject->GetUsecaseRequestObject()->GetLastZSLFrameNumber();
    UINT8   numNonZSLRequests     = 0;
    UINT8   producerListSize      = rProducerList.size();
    UINT8   consumerListSize      = rConsumerList.size();
    UINT64  lastShutter           = GetLastShutterTimestamp();
    UINT32  pickedZSLFrame        = 0;
    UINT64  startTime             = 0;
    UINT64  endTime               = lastShutter;

    for (UINT32 requestIndex = 0; requestIndex < pRequestObject->GetNumRequests(); ++requestIndex)
    {
        // By default, we try to provide all frames from ZSL Queue unless downstream feature asks
        // NON ZSL Explicitly

        ChiMetadata* pSettings = GetExternalInputSettings(pRequestObject, pSettingsPort, requestIndex);
        if (NULL != pSettings)
        {
            INT* pControlZSL = static_cast<INT*>(pSettings->GetTag(ANDROID_CONTROL_ENABLE_ZSL));
            if (NULL != pControlZSL)
            {
                if (ANDROID_CONTROL_ENABLE_ZSL_FALSE == *pControlZSL)
                {
                    numZSLRequests--;
                    numNonZSLRequests++;
                }
            }
            INT64* pZSLTimeRange = NULL;
            ChxUtils::GetVendorTagValue(pSettings, VendorTag::ZSLTimestampRange, reinterpret_cast<VOID**>(&pZSLTimeRange));
            if (NULL != pZSLTimeRange)
            {
                CHX_LOG_INFO("ZSL TimeRange vendor tag set %" PRIu64 " %" PRIu64, pZSLTimeRange[0], pZSLTimeRange[1]);
                startTime = pZSLTimeRange[0];
                endTime   = pZSLTimeRange[1];
            }
        }
    }
    CHX_LOG_INFO("NumNonZSL requests %d NumZSL requests %d", numNonZSLRequests, numZSLRequests);

    // If we have a NON ZSL request, we need to pick ZSL requests from the producer list
    INT32 offset = 0;
    if (0 != numNonZSLRequests)
    {
        offset = producerListSize -1;
        for (UINT8 request = 0; request < (producerListSize + consumerListSize); ++ request)
        {
            if (0 < numZSLRequests)
            {
                offsets.push_back(offset);
                offset--;
                --numZSLRequests;
            }

            if (0 == numZSLRequests)
            {
                break;
            }
        }
    }
    else
    {
        UINT32 frameNumber = 0;
        // All ZSL requests, check if we can supply all from the consumer list
        if (consumerListSize >= numZSLRequests)
        {
            ChiFeaturePortData*         pPortData        = GetPortData(pSettingsPort);
            ChiFeaturePipelineData*     pPipelineData    = GetPipelineData(pSettingsPort);
            CHITARGETBUFFERINFOHANDLE   hBuffer[1]       = {0};
            ChiMetadata*                pMetadata        = NULL;
            UINT64*                     pSensorTimestamp = NULL;

            offset = -1;

            if (NULL != pPipelineData && NULL != pPortData && NULL != pPortData->pOutputBufferTbm)
            {
                for (UINT8 buffer = 0; buffer < consumerListSize; ++ buffer)
                {
                    // Find frame within timestamp tolerance
                    frameNumber       = rConsumerList.at(consumerListSize+offset);

                    pPortData->pOutputBufferTbm->GetTargetBuffers(frameNumber, 1, &hBuffer[0], TRUE);

                    if (NULL != hBuffer[0])
                    {
                        pMetadata = reinterpret_cast<ChiMetadata*>(
                            pPortData->pOutputBufferTbm->GetTarget(hBuffer[0],
                            pPipelineData->metadataClientId));

                        if (NULL != pMetadata)
                        {
                            pSensorTimestamp = static_cast<UINT64*>(pMetadata->GetTag(ANDROID_SENSOR_TIMESTAMP));
                            if (NULL != pSensorTimestamp)
                            {
                                if ((numZSLRequests > 0)                            &&
                                    (frameNumber > lastZSLFrameNumber)              &&
                                    (*pSensorTimestamp <= (lastShutter - startTime)) &&
                                    (*pSensorTimestamp > (lastShutter - endTime)))
                                {
                                    CHX_LOG_INFO("Matching offset found %d lastZSLFrame %d", offset,
                                        lastZSLFrameNumber);
                                    offsets.push_back(offset);
                                    --numZSLRequests;
                                }
                                else
                                {
                                    CHX_LOG("Timestamp %" PRIu64 "Offset %d lastShutter %" PRIu64,
                                        *pSensorTimestamp, offset, lastShutter);
                                }
                                offset--;
                            }
                            else
                            {
                                CHX_LOG_ERROR("Unable to get timestamp for frame %d", frameNumber);
                            }
                        }
                        pPortData->pOutputBufferTbm->ReleaseTargetBuffer(hBuffer[0]);
                        if (0 == numZSLRequests)
                        {
                            // We have found all the required frames
                            break;
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("Unable to get metadata buffer for frame %d", frameNumber);
                    }
                }
            }
            else
            {
                CHX_LOG_ERROR("Cannot find port data for provided settings port");
            }
        }
        else
        {
            // Consumer list does not contain enough buffers, start from producer
            offset = producerListSize - 1;

            for (UINT8 request = 0; request < (producerListSize + consumerListSize ); ++ request)
            {
                if (0 > offset)
                {
                    frameNumber = rConsumerList.at(consumerListSize + offset);
                }
                else
                {
                    frameNumber = rProducerList.at(offset);
                }

                if ((0 < numZSLRequests) && (frameNumber > lastZSLFrameNumber))
                {
                    offsets.push_back(offset);
                    numZSLRequests--;
                }
                offset--;

                if (0 == numZSLRequests)
                {
                    // We have found all the required frames
                    break;
                }
            }
        }

        if (0 == numZSLRequests)
        {
            pickedZSLFrame = frameNumber;
        }
        else
        {
            // Could not find any matching ZSL Frames
            // Set last ZSL frame to be current frameNumber
            pickedZSLFrame = m_frameNumber+pRequestObject->GetNumRequests();
        }

        // Send picked ZSL frame as message
        ChiFeature2MessageDescriptor ZSLFrameMsg;
        ChxUtils::Memset(&ZSLFrameMsg, 0, sizeof(ZSLFrameMsg));
        ZSLFrameMsg.messageType                                 = ChiFeature2MessageType::MessageNotification;
        ZSLFrameMsg.message.notificationData.messageType        = ChiFeature2ChiMessageType::ZSL;
        ChiFeature2ZSLData    ZSLData;
        ZSLData.lastFrameNumber                                 = pickedZSLFrame;
        ZSLFrameMsg.message.notificationData.message.ZSLData    = ZSLData;
        ZSLFrameMsg.pPrivData                                   = pRequestObject;
        ProcessFeatureMessage(&ZSLFrameMsg);
    }
    return offsets;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::CheckZSLQueueEmptyForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Base::CheckZSLQueueEmptyForPort(
    ChiFeature2RequestObject*       pRequestObject,
    const ChiFeature2Identifier*    pIdentifier
    ) const
{
    PortZSLQueueData*   pZSLQueue   = NULL;
    CDKResult           result      = CDKResultSuccess;
    BOOL                isEmpty     = TRUE;

    result = GetZSLQueueForPort(pRequestObject, pIdentifier, &pZSLQueue);

    if (CDKResultSuccess == result)
    {
        if (NULL != pZSLQueue)
        {
            if (!pZSLQueue->frameNumbers.empty())
            {
                isEmpty = FALSE;
            }
        }
    }
    return isEmpty;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetZSLQueueForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::GetZSLQueueForPort(
    ChiFeature2RequestObject*       pRequestObject,
    const ChiFeature2Identifier*    pIdentifier,
    PortZSLQueueData**              ppQueue
    ) const
{
    ChiFeatureRequestContext* pRequestContext = NULL;
    CDKResult                 result          = CDKResultSuccess;
    pRequestContext = static_cast<ChiFeatureRequestContext*>(pRequestObject->GetPrivContext());
    *ppQueue = NULL;
    if (NULL != pRequestContext)
    {
        for (UINT8 portIndex = 0; portIndex < pRequestContext->pPortZSLQueues.size(); ++portIndex)
        {
            if (*pIdentifier == pRequestContext->pPortZSLQueues.at(portIndex)->globalId)
            {
                *ppQueue = pRequestContext->pPortZSLQueues.at(portIndex);
            }
        }
        if (NULL == *ppQueue)
        {
            CHX_LOG_WARN("No queue found for port %d %d %d", pIdentifier->session, pIdentifier->pipeline, pIdentifier->port);
            result = CDKResultENoSuch;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::AssignSessionSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::AssignSessionSettings(
    Pipeline* pPipeline)
{
    CDK_UNREFERENCED_PARAM(pPipeline);
    CDKResult result = CDKResultSuccess;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::CanRequestContinue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Base::CanRequestContinue(
    ChiFeature2RequestState requestState)
{
    BOOL continueRequest = FALSE;

    switch (requestState)
    {
        case ChiFeature2RequestState::Initialized:
        case ChiFeature2RequestState::ReadyToExecute:
        case ChiFeature2RequestState::Executing:
            continueRequest = TRUE;
            break;
        default:
            continueRequest = FALSE;
            break;
    }
    CHX_LOG_INFO("Feature2RequestState %s continueRequest is %d ",
        ChiFeature2RequestStateStrings[static_cast<UINT8>(requestState)], continueRequest);
    return continueRequest;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::CreateStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHISTREAM* ChiFeature2Base::CreateStream()
{
    CHISTREAM* pStream = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
    if (NULL != pStream)
    {
        m_pStreams.push_back(pStream);
    }
    else
    {
        CHX_LOG_ERROR("Feature %s - Stream Allocation Failed", m_pFeatureName);
    }

    return pStream;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::CloneStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHISTREAM* ChiFeature2Base::CloneStream(
    CHISTREAM* pSrc)
{
    CHISTREAM* pCloned = CreateStream();
    if (NULL != pCloned)
    {
        ChxUtils::Memcpy(pCloned, pSrc, sizeof(CHISTREAM));
    }

    return pCloned;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnInitializeStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnInitializeStream(
    const ChiTargetPortDescriptor*    pTargetDesc,
    const ChiFeature2PortDescriptor*  pPortDesc,
    ChiFeatureStreamData*             pOutputStreamData)
{
    /// Derived features can override this function to assign stream for intra ports,
    /// special formats.
    CDKResult   result = CDKResultSuccess;

    if ((NULL == pPortDesc) || (NULL == pTargetDesc) || (NULL == pOutputStreamData))
    {
        CHX_LOG_ERROR("Invalid argument: NULL pTargetDesc = %p pPortDesc = %p pOutputStream = %p",
            pTargetDesc, pPortDesc, pOutputStreamData);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        if (NULL == pOutputStreamData->pStream)
        {
            CHX_LOG_ERROR("Invalid argument: NULL pOutputStreamData->pStream");
            result = CDKResultEInvalidArg;
        }
    }

    if (CDKResultSuccess == result)
    {
        result = GetTargetStream(m_pCameraInfo,
            pTargetDesc->pTargetName,
            static_cast<CHISTREAM**>(m_configStreams.data()),
            m_configStreams.size(),
            &pOutputStreamData->pTargetStream,
            pOutputStreamData->pStream);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::InitializeTargetStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::InitializeTargetStream(
    const ChiTargetPortDescriptor*    pTargetDesc,
    const ChiFeature2PortDescriptor*  pPortDesc,
    ChiFeatureStreamData*             pOutputStreamData)
{
    CDKResult   result             = CDKResultSuccess;

    result = OnInitializeStream(pTargetDesc, pPortDesc, pOutputStreamData);
    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("Cannot match Stream: target Name = %s, PortKey = %d:%d:%d:%d",
            pTargetDesc->pTargetName, pPortDesc->globalId.session, pPortDesc->globalId.pipeline,
            pPortDesc->globalId.port, pPortDesc->globalId.portDirectionType);
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::AssignStreams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::AssignStreams(
    ChiTargetPortDescriptor*          pTargetDesc,
    const ChiFeature2PortDescriptor*  pPortDesc)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pTargetDesc) || (NULL == pPortDesc))
    {
        CHX_LOG_ERROR("Invalid argument: NULL pTargetDesc = %p, portDesc = %p", pTargetDesc, pPortDesc);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        CHISTREAM* pStream = CreateStream();

        ChiFeatureStreamData outputStreamData  = {0};
        outputStreamData.pStream               = pStream;
        outputStreamData.pTargetStream         = NULL;

        result = InitializeTargetStream(pTargetDesc, pPortDesc, &outputStreamData);
        if (CDKResultSuccess == result)
        {
            m_pStreamData.push_back(outputStreamData);
            pTargetDesc->pTarget->pChiStream = pStream;
        }
        else
        {
            CHX_LOG_ERROR("Create Stream Failed: Target = %s", pTargetDesc->pTargetName);
            result = CDKResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::AssignTargets
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::AssignTargets(
    ChiTargetPortDescriptor*          pTargetDesc,
    std::vector<ChiFeaturePortData>*  ppPortData)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pTargetDesc) || (NULL == ppPortData))
    {
        CHX_LOG_ERROR("Invalid argument: NULL pTargetDesc = %p, ppPortData = %p", pTargetDesc, ppPortData);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        BOOL hasAssigned = FALSE;
        for (UINT dstPortIdx = 0; dstPortIdx < (*ppPortData).size(); dstPortIdx++)
        {
            ChiFeaturePortData* pDstPortData = &(*ppPortData)[dstPortIdx];
            if (NULL == pDstPortData->pTargetDesc)
            {
                continue;
            }
            if ((0    == CdkUtils::StrCmp(pTargetDesc->pTargetName, pDstPortData->pTargetDesc->pTargetName)) &&
                (NULL == pDstPortData->pTarget))
            {
                pDstPortData->pTarget     = pTargetDesc->pTarget;
                pDstPortData->pChiStream  = pTargetDesc->pTarget->pChiStream;
                hasAssigned               = TRUE;
            }
        }

        if (FALSE == hasAssigned)
        {
            CHX_LOG_WARN("Unassigned Target: %s", pTargetDesc->pTargetName);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::CreatePipelines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIPIPELINEHANDLE ChiFeature2Base::CreatePipeline(
    const ChiFeature2PipelineDescriptor* pPipelineDesc,
    ChiFeaturePipelineData* pPipelineData)
{
    CDKResult                       result         = CDKResultSuccess;
    Pipeline*                       pPipeline      = NULL;
    const ChiFeature2InstanceProps* pInstanceProps = GetInstanceProps();

    if ((NULL == pPipelineDesc) || (NULL == pPipelineData))
    {
        CHX_LOG_ERROR("Invalid Argument: pPipelineDesc = %p pPipelineData = %p",
            pPipelineDesc, pPipelineData);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        UINT pipelineIdx = CDKInvalidId;
        if (ChiFeature2PipelineType::Virtual != pPipelineDesc->pipelineType)
        {
            pipelineIdx = GetPipelineIndex(m_pUsecaseDesc, pPipelineDesc->pPipelineName);
        }

        if (CDKInvalidId != pipelineIdx)
        {
            UINT32 physicalCameraIndex = m_pInstanceProps->cameraId;
            pPipeline = Pipeline::Create(m_pCameraInfo->ppDeviceInfo[physicalCameraIndex]->cameraId, PipelineType::Default);

            ChiTargetPortDescriptorInfo* pSinkTarget = &m_pUsecaseDesc->pPipelineTargetCreateDesc[pipelineIdx].sinkTarget;
            ChiTargetPortDescriptorInfo* pSrcTarget  = &m_pUsecaseDesc->pPipelineTargetCreateDesc[pipelineIdx].sourceTarget;
            if (NULL != pPipeline)
            {
                ChiPortBufferDescriptor* pSinkBufferDescs = static_cast<ChiPortBufferDescriptor*>(
                    CHX_CALLOC(sizeof(ChiPortBufferDescriptor) * pSinkTarget->numTargets));
                ChiPortBufferDescriptor* pSrcBufferDescs = static_cast<ChiPortBufferDescriptor*>(
                    CHX_CALLOC(sizeof(ChiPortBufferDescriptor) * pSrcTarget->numTargets));

                if ((NULL != pSinkBufferDescs) && (NULL != pSrcBufferDescs))
                {
                    for (UINT sinkTargetIdx = 0 ; sinkTargetIdx < pSinkTarget->numTargets; sinkTargetIdx++)
                    {
                        result = AssignStreams(&pSinkTarget->pTargetPortDesc[sinkTargetIdx],
                                               &pPipelineDesc->pOutputPortDescriptor[sinkTargetIdx]);
                        if (CDKResultSuccess == result)
                        {
                            pSinkBufferDescs[sinkTargetIdx].pNodePort =
                                pSinkTarget->pTargetPortDesc[sinkTargetIdx].pNodePort;
                            pSinkBufferDescs[sinkTargetIdx].pStream  =
                                pSinkTarget->pTargetPortDesc[sinkTargetIdx].pTarget->pChiStream;

                            result = AssignTargets(&pSinkTarget->pTargetPortDesc[sinkTargetIdx],
                                                   &pPipelineData->pOutputPortData);
                        }
                        else
                        {
                            break;
                        }
                    }

                    if (CDKResultSuccess == result)
                    {
                        for (UINT srcTargetIdx = 0 ; srcTargetIdx < pSrcTarget->numTargets; srcTargetIdx++)
                        {
                            result = AssignStreams(&pSrcTarget->pTargetPortDesc[srcTargetIdx],
                                &pPipelineDesc->pInputPortDescriptor[srcTargetIdx]);
                            if (CDKResultSuccess == result)
                            {
                                pSrcBufferDescs[srcTargetIdx].pNodePort =
                                    pSrcTarget->pTargetPortDesc[srcTargetIdx].pNodePort;
                                pSrcBufferDescs[srcTargetIdx].pStream  =
                                    pSrcTarget->pTargetPortDesc[srcTargetIdx].pTarget->pChiStream;
                                pSrcBufferDescs[srcTargetIdx].numNodePorts =
                                    pSrcTarget->pTargetPortDesc[srcTargetIdx].numNodePorts;
                                result = AssignTargets(&pSrcTarget->pTargetPortDesc[srcTargetIdx],
                                    &pPipelineData->pInputPortData);
                            }
                            else
                            {
                                break;
                            }
                        }

                        pPipeline->SetOutputBuffers(pSinkTarget->numTargets, &pSinkBufferDescs[0]);
                        pPipeline->SetInputBuffers(pSrcTarget->numTargets, &pSrcBufferDescs[0]);
                        pPipeline->SetPipelineNodePorts(
                            &m_pUsecaseDesc->pPipelineTargetCreateDesc[pipelineIdx].pipelineCreateDesc);
                        pPipeline->SetPipelineName(m_pUsecaseDesc ->pPipelineTargetCreateDesc[pipelineIdx].pPipelineName);

                        CHX_LOG_INFO("Creating pipeline %s", pPipeline->GetPipelineName());

                        if (CDKResultSuccess == result)
                        {
                            pPipelineData->pPipeline = pPipeline;

                            if ((TRUE == pPipeline->IsRealTime()) && (TRUE == m_useResManager))
                            {
                                ChiMetadata* pMetadata = pPipeline->GetDescriptorMetadata();
                                if (NULL != pMetadata)
                                {
                                    INT32 useResManager = 1;
                                    result = pMetadata->SetTag("org.quic.camera.resource",
                                                            "enable",
                                                            &useResManager,
                                                            1);

                                    if (CDKResultSuccess != result)
                                    {
                                        CHX_LOG_ERROR("Failed to set session metadata for resource manager");
                                    }
                                    else
                                    {
                                        CHX_LOG_CONFIG("Enable resource manager for feature: %s, instance:%d, flag:%x",
                                            GetFeatureName(), pInstanceProps->instanceId, pInstanceProps->instanceFlags.value);
                                    }
                                }
                            }

                            // For non-zsl snapshot realtime pipeline, set defer finalize flag
                            if ((NULL != pInstanceProps) &&
                                (TRUE == pInstanceProps->instanceFlags.isNZSLSnapshot) &&
                                (TRUE == pPipeline->IsRealTime()))
                            {
                                CHX_LOG_INFO("set defer finalize flag for non-zsl rt snapshot pipeline");
                                pPipeline->SetDeferFinalizeFlag(TRUE);
                            }

                            // For QCFA remosaic snapshot, set sensor mode pick hint, to pick full size mode (quadra pattern).
                            // This is required for both realtime and offline pipeline, as the sensor mode may impact IQ
                            if ((NULL != pInstanceProps) &&
                                (TRUE == pInstanceProps->instanceFlags.isNZSLSnapshot) &&
                                ((TRUE == pInstanceProps->instanceFlags.isSWRemosaicSnapshot) ||
                                 (TRUE == pInstanceProps->instanceFlags.isHWRemosaicSnapshot)))
                            {
                                m_sensorModePickHint.sensorModeCaps.value     = 0;
                                m_sensorModePickHint.postSensorUpscale        = FALSE;
                                m_sensorModePickHint.sensorModeCaps.u.QuadCFA = TRUE;

                                CHX_LOG_INFO("set sensor mode pick hint for qcfa sensor");
                                pPipeline->SetSensorModePickHint(&m_sensorModePickHint);
                            }
                        }

                        //// Call prepare pipeline create
                        result = OnPreparePipelineCreate(&pPipelineData->globalId);
                    }

                    CHX_FREE(pSinkBufferDescs);
                    CHX_FREE(pSrcBufferDescs);
                }
                else
                {
                    CHX_LOG_ERROR("No memory: numSinkTarget = %d, numSrcTarget = %d",
                        pSinkTarget->numTargets, pSrcTarget->numTargets);
                    result = CDKResultENoMemory;
                }

                if (CDKResultSuccess == result)
                {
                    result = pPipeline->CreateDescriptor();
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("Create pipeline %s descriptor failed!",
                            m_pUsecaseDesc->pPipelineTargetCreateDesc[pipelineIdx].pPipelineName);
                    }
                }

                if (CDKResultSuccess == result)
                {
                    for (UINT32 portIdx = 0; portIdx < pPipelineData->pInputPortData.size(); portIdx++)
                    {
                        OnPortCreate(&pPipelineData->pInputPortData[portIdx].globalId);
                    }

                    for (UINT32 portIdx = 0; portIdx < pPipelineData->pOutputPortData.size(); portIdx++)
                    {
                        OnPortCreate(&pPipelineData->pOutputPortData[portIdx].globalId);
                    }
                }

                if (CDKResultSuccess == result)
                {
                    result = OnPipelineCreate(&pPipelineData->globalId);
                }

                if (CDKResultSuccess != result)
                {
                    pPipeline->Destroy();
                    pPipelineData->pPipeline = NULL;
                    pPipeline                = NULL;
                }
            }
            else
            {
                CHX_LOG_ERROR("Pipeline creation failed pipeline");
                result = CDKResultEFailed;
            }
        }
    }

    return pPipeline;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::CreateSessions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::CreateSession(
    const ChiFeature2SessionDescriptor* pSessionDesc)
{
    CDKResult              result        = CDKResultSuccess;
    Session*               pSession      = NULL;
    ChiFeatureSessionData* pSessionData  = NULL;
    Pipeline**             ppPipelines   = NULL;
    UINT32                 numPipelines  = 0;
    ChiFeature2Identifier  globalId;

    if (NULL != pSessionDesc)
    {
        globalId.session = pSessionDesc->sessionId;
        pSessionData     = GetSessionData(&globalId);
        if (NULL != pSessionData)
        {
            ppPipelines = static_cast<Pipeline**>(CHX_CALLOC(sizeof(Pipeline*) * pSessionData->pPipelineData.size()));
        }
        else
        {
            CHX_LOG_ERROR("NULL pSessionData");
            result = CDKResultEInvalidArg;
        }
    }
    else
    {
        CHX_LOG_ERROR("NULL pSessionDesc");
        result = CDKResultEInvalidArg;
    }

    if ((CDKResultSuccess == result) && (NULL != ppPipelines))
    {
        for (UINT pipelineIdx = 0; pipelineIdx < pSessionDesc->numPipelines; pipelineIdx++)
        {
            if (0 == CdkUtils::StrCmp(pSessionData->pPipelineData[numPipelines]->pPipelineName,
                pSessionDesc->pPipeline[pipelineIdx].pPipelineName))
            {
                globalId.pipeline                     = pSessionDesc->pPipeline[pipelineIdx].pipelineId;
                ChiFeaturePipelineData* pPipelineData = GetPipelineData(&globalId);

                if (NULL != pPipelineData)
                {
                    Pipeline* pPipeline = static_cast<Pipeline*>(
                        CreatePipeline(&pSessionDesc->pPipeline[pipelineIdx], pPipelineData));
                    if (NULL != pPipeline)
                    {
                        UINT32 numPhysicalCameras = m_pCameraInfo->numPhysicalCameras;
                        ppPipelines[numPipelines++] = pPipeline;

                        ChiMetadata* pMetadata = pPipeline->GetDescriptorMetadata();
                        pMetadata->SetTag("com.qti.chi.logicalcamerainfo",
                                          "NumPhysicalCameras",
                                          &numPhysicalCameras,
                                          sizeof(numPhysicalCameras));
                    }
                    else
                    {
                        result = CDKResultEFailed;
                        CHX_LOG_ERROR("Failed to Create feature = %s", pSessionDesc->pSessionName);
                        break;
                    }
                }
            }
            if (numPipelines == pSessionData->pPipelineData.size())
            {
                break;
            }
        }

        if (CDKResultSuccess == result)
        {
            pSession = Session::Create(ppPipelines, numPipelines,
                &pSessionData->callbacks, &pSessionData->sessionCbData);

            if (NULL != pSession)
            {
                pSessionData->pSession = pSession;
                result = OnSessionCreate(&pSessionData->globalId);
            }
            else
            {
                CHX_LOG_ERROR("Failed to Create Session = %s", pSessionDesc->pSessionName);
                result = CDKResultEFailed;
            }
        }

        CHX_FREE(ppPipelines);
        ppPipelines = NULL;
    }
    else
    {
        CHX_LOG_ERROR("Invalid Argument: ppPipelines = %p pSessionData = %p pSessionDesc = %p",
            ppPipelines, pSessionData, pSessionDesc);
        result = CDKResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::CreateFeatureData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::CreateFeatureData(
    const ChiFeature2Descriptor* pFeatureDesc)
{
    CDKResult result    = CDKResultSuccess;

    if (NULL == pFeatureDesc)
    {
        CHX_LOG_ERROR("Invalid argument: NULL pFeatureDesc");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 sessionIdx = 0; sessionIdx < pFeatureDesc->numSessions; sessionIdx++)
        {
            result = CreateSession(&pFeatureDesc->pSession[sessionIdx]);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Failed to Create feature = %s", pFeatureDesc->pFeatureName);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::CreateTargetBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::CreateTargetBufferManagers()
{
    CDKResult result = CDKResultSuccess;

    for (UINT8 sessionIndex = 0 ; sessionIndex < m_pSessionData.size(); ++sessionIndex)
    {
        std::vector<ChiFeaturePipelineData*> pPipelineDataList = m_pSessionData[sessionIndex]->pPipelineData;

        for (UINT8 pipelineIndex = 0 ; pipelineIndex < pPipelineDataList.size(); ++ pipelineIndex)
        {
            // Create metadata manager for pipeline output meta
            ChiFeaturePipelineData* pPipelineData = pPipelineDataList[pipelineIndex];

            CHITargetBufferManager* pTargetBufferManager         = NULL;

            ChiTargetBufferManagerCreateData    targetBuffercreateData  = { 0 };
            CHAR                                metamanagerName[100]    = { 0 };

            CdkUtils::SNPrintF(metamanagerName, sizeof(metamanagerName), "MetaTargetBuffer_%s", pPipelineData->pPipelineName);
            targetBuffercreateData.pTargetBufferName            = metamanagerName;
            targetBuffercreateData.numOfMetadataBuffers         = 1;
            targetBuffercreateData.minMetaBufferCount           = pPipelineData->minMetaBufferCount;
            targetBuffercreateData.metadataIds[0]               = pPipelineData->metadataClientId;
            targetBuffercreateData.pChiMetadataManager          = m_pMetadataManager;
            targetBuffercreateData.numOfInternalStreamBuffers   = 0;
            targetBuffercreateData.numOfExternalStreamBuffers   = 0;
            targetBuffercreateData.isChiFenceEnabled            = ExtensionModule::GetInstance()->EnableTBMChiFence();

            pTargetBufferManager = CHITargetBufferManager::Create(&targetBuffercreateData);

            if (NULL != pTargetBufferManager)
            {
                pPipelineData->pOutputMetaTbm = pTargetBufferManager;
            }
            else
            {
                CHX_LOG_ERROR("Failed to create metadata TBM for pipeline %s", pPipelineData->pPipelineName);
                result = CDKResultEFailed;
                break;
            }

            // Create setting metadata manager for pipeline
            CdkUtils::SNPrintF(metamanagerName, sizeof(metamanagerName),
                "SettingTargetBuffer_%s", pPipelineData->pPipelineName);

            targetBuffercreateData.minMetaBufferCount       = pPipelineData->minMetaBufferCount;
            targetBuffercreateData.metadataIds[0]           = pPipelineData->metadataClientId;
            targetBuffercreateData.pTargetBufferName        = metamanagerName;
            pTargetBufferManager                            = CHITargetBufferManager::Create(&targetBuffercreateData);
            if (NULL != pTargetBufferManager)
            {
                pPipelineData->pSettingMetaTbm = pTargetBufferManager;
            }
            else
            {
                CHX_LOG_ERROR("Failed to create setting TBM for pipeline %s", pPipelineData->pPipelineName);
                result = CDKResultEFailed;
                break;
            }

            CHX_LOG_VERBOSE("metamanagerName:%s pSettingMetaTbm:%p",
                            metamanagerName,
                            pPipelineData->pSettingMetaTbm);

            for (UINT8 portIndex = 0 ; portIndex < pPipelineData->pOutputPortData.size(); ++portIndex)
            {
                ChiFeaturePortData*      pPortData        = &pPipelineData->pOutputPortData[portIndex];

                // Metadata port uses TBM from respective pipeline
                if (ChiFeature2PortType::MetaData == pPortData->globalId.portType)
                {
                    // if MetadataId is invalid, it indicates it can share same metadata TBM of pipeline
                    // Otherwise, the metadata client id is assigned in derived feature.
                    // This is only for multiple metadata output for one pipeline requirements.
                    if (pPortData->metadataClientId == InvalidIndex)
                    {
                        pPortData->pOutputBufferTbm = pPipelineData->pOutputMetaTbm;
                    }
                    else
                    {
                        CdkUtils::SNPrintF(metamanagerName, sizeof(metamanagerName), "MetaTargetBuffer_%s_%s",
                            pPipelineData->pPipelineName,
                            pPortData->pPortName);

                        targetBuffercreateData.pTargetBufferName            = metamanagerName;
                        targetBuffercreateData.numOfMetadataBuffers         = 1;
                        targetBuffercreateData.metadataIds[0]               = pPortData->metadataClientId;;
                        targetBuffercreateData.pChiMetadataManager          = m_pMetadataManager;
                        targetBuffercreateData.numOfInternalStreamBuffers   = 0;
                        targetBuffercreateData.numOfExternalStreamBuffers   = 0;
                        targetBuffercreateData.minMetaBufferCount           = pPortData->minBufferCount;
                        targetBuffercreateData.maxMetaBufferCount           = pPortData->maxBufferCount;

                        pTargetBufferManager = CHITargetBufferManager::Create(&targetBuffercreateData);

                        if (NULL != pTargetBufferManager)
                        {
                            pPortData->pOutputBufferTbm = pTargetBufferManager;
                            CHX_LOG_CONFIG("metamanagerName:%s pMetaPortTBM:%p", metamanagerName, pPortData->pOutputBufferTbm);
                        }
                        else
                        {
                            CHX_LOG_ERROR("Failed to create setting TBM for pipeline %s", pPipelineData->pPipelineName);
                            result = CDKResultEFailed;
                            break;
                        }

                    }
                    continue;
                }

                if (NULL == pPortData->pChiStream)
                {
                    continue;
                }

                CHITargetBufferManager*  pOutputBufferTBM = NULL;

                for (UINT8 linkedPort = 0 ; linkedPort < portIndex; ++linkedPort)
                {
                    ChiFeaturePortData*     pLinkedPortData = &pPipelineData->pOutputPortData[linkedPort];

                    if (ChiFeature2PortType::MetaData == pLinkedPortData->globalId.portType)
                    {
                        continue; // Metadata won't have a pTargetDesc
                    }

                    if ((NULL != pLinkedPortData->pTargetDesc) &&
                        (NULL != pPortData->pTargetDesc))
                    {
                        if ((0 == CdkUtils::StrCmp(pLinkedPortData->pTargetDesc->pTargetName,
                            pPortData->pTargetDesc->pTargetName)) &&
                            (NULL != pLinkedPortData->pOutputBufferTbm))
                        {
                            pOutputBufferTBM = pLinkedPortData->pOutputBufferTbm;
                        }
                    }
                }

                if ((NULL == pPortData->pTarget) && (NULL == pPortData->pChiStream))
                {
                    // pTarget can be NULL in two cases:
                    //   1) A virtual pipeline such as Anchor Picking for MFNR
                    //   2) After usecase pruning
                    // We don't want to proceed here in the pruning case
                    CHX_LOG_WARN("Null pTarget on %s", pPortData->pPortName);
                }
                else if (NULL == pOutputBufferTBM)
                {
                    ChiStream* pStream = pPortData->pChiStream;

                    CHIBufferManagerCreateData createData   = {0};
                    targetBuffercreateData                  = {0};

                    createData.width                = pStream->width;
                    createData.height               = pStream->height;
                    createData.format               = pStream->format;
                    createData.producerFlags        = pPortData->producerFlags;
                    createData.consumerFlags        = pPortData->consumerFlags;
                    createData.minBufferCount       = pPortData->minBufferCount;
                    createData.maxBufferCount       = pPortData->maxBufferCount;
                    createData.immediateBufferCount = 0; // Will create buffers on request basis;
                    createData.bEnableLateBinding   = ExtensionModule::GetInstance()->EnableCHILateBinding();
                    createData.bufferHeap           = BufferHeapDefault;
                    createData.pChiStream           = pStream;

                    CHAR bufferManagerName[100] = {0};
                    CdkUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName),
                                       "PortTargetBuffer_%s:%s", m_pFeatureName, pPortData->pPortName);
                    CHX_LOG_CONFIG("Pipeline[%s], port:%s port idx:%d, Create CHI buffers, chistream:%p, "
                                    "format:%d, w x h: %d x %d",
                            pPipelineData->pPipelineName,
                            pPortData->pPortName,
                            pPortData->globalId.port,
                            pStream,
                            pStream->format,
                            pStream->width,
                            pStream->height);

                    targetBuffercreateData.pTargetBufferName            = bufferManagerName;
                    targetBuffercreateData.numOfMetadataBuffers         = 0;
                    targetBuffercreateData.pChiMetadataManager          = NULL;
                    targetBuffercreateData.numOfInternalStreamBuffers   = 1;
                    targetBuffercreateData.internalStreamIds[0]         = reinterpret_cast<UINT64>(pStream);
                    targetBuffercreateData.pBufferManagerNames[0]       = bufferManagerName;
                    targetBuffercreateData.pBufferManagerCreateData[0]  = &createData;
                    targetBuffercreateData.numOfExternalStreamBuffers   = 0;
                    targetBuffercreateData.isChiFenceEnabled            = ExtensionModule::GetInstance()->EnableTBMChiFence();

                    if ((!CdkUtils::StrCmp(m_pFeatureName, "MFSR")) &&
                        (TRUE == ExtensionModule::GetInstance()->EnableMFSRChiFence()))
                    {
                        // Enable chi fence only for MFSR
                        std::string targetBufferName = bufferManagerName;
                        if (targetBufferName.find("Blend") != std::string::npos)
                        {
                            targetBuffercreateData.isChiFenceEnabled = 1;
                        }
                    }

                    pTargetBufferManager = CHITargetBufferManager::Create(&targetBuffercreateData);

                    if (NULL != pTargetBufferManager)
                    {
                        CHX_LOG_INFO("isChiFenceEnabled:%d, %s: %s: Res: %dX%d Format = 0X%x, Min Buffer = %d, Max Buffer = %d",
                                     targetBuffercreateData.isChiFenceEnabled,
                                     m_pFeatureName, bufferManagerName,
                                     createData.width, createData.height, createData.format,
                                     createData.minBufferCount, createData.maxBufferCount);
                        pPortData->pOutputBufferTbm = pTargetBufferManager;
                    }
                    else
                    {
                        CHX_LOG_ERROR("Failed to create outbuffer TBM for port %s", pPortData->pPortName);
                        result = CDKResultEFailed;
                        break;
                    }
                }
                else
                {
                    CHX_LOG_INFO("Feature: %s Sharing TBM for port %s", m_pFeatureName, pPortData->pPortName);
                    pPortData->pOutputBufferTbm = pOutputBufferTBM;
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::DestroyTargetBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::DestroyTargetBufferManagers()
{
    CDKResult           result              = CDKResultSuccess;
    ChiMetadataManager* pMetadataManager    = GetMetadataManager();

    for (UINT8 sessionIndex = 0 ; sessionIndex < m_pSessionData.size(); ++sessionIndex)
    {
        std::vector<ChiFeaturePipelineData*> pPipelineDataList = m_pSessionData[sessionIndex]->pPipelineData;

        for (UINT8 pipelineIndex = 0 ; pipelineIndex < pPipelineDataList.size(); ++ pipelineIndex)
        {
            ChiFeaturePipelineData* pPipelineData     = pPipelineDataList[pipelineIndex];
            if ((NULL != pPipelineData->pPipeline) && (NULL != pMetadataManager))
            {
                UINT32  metadataClientId = pPipelineData->pPipeline->GetMetadataClientId();
                pMetadataManager->UnregisterClient(metadataClientId);
            }

            // Keep this to compare it to the per port manager. In some cases (like AnchorSync) these two are not
            // the same and we have to delete both.
            CHITargetBufferManager* pDestroyedManager = pPipelineData->pOutputMetaTbm;
            if (NULL != pPipelineData->pOutputMetaTbm)
            {
                pPipelineData->pOutputMetaTbm->Destroy();
                pPipelineData->pOutputMetaTbm = NULL;
            }

            if (NULL != pPipelineData->pSettingMetaTbm)
            {
                pPipelineData->pSettingMetaTbm->Destroy();
                pPipelineData->pSettingMetaTbm = NULL;
            }

            for (UINT8 portIndex = 0 ; portIndex < pPipelineData->pOutputPortData.size(); ++portIndex)
            {
                ChiFeaturePortData* pPortData = &pPipelineData->pOutputPortData[portIndex];

                if (NULL != pPortData->pOutputBufferTbm)
                {
                    // If the buffer tbm was shared, it would have been destroyed in the pPipelineData->pOutputMetaTbm above.
                    if (pDestroyedManager == pPortData->pOutputBufferTbm)
                    {
                        pPortData->pOutputBufferTbm = NULL;
                        continue;
                    }

                    // Clean up shared TBMs
                    for (UINT8 linkedPort = portIndex + 1; linkedPort < pPipelineData->pOutputPortData.size(); ++linkedPort)
                    {
                        ChiFeaturePortData* pLinkedPortData = &pPipelineData->pOutputPortData[linkedPort];

                        if (pLinkedPortData->pOutputBufferTbm == pPortData->pOutputBufferTbm)
                        {
                            pLinkedPortData->pOutputBufferTbm = NULL;
                        }
                    }

                    pPortData->pOutputBufferTbm->Destroy();
                    pPortData->pOutputBufferTbm = NULL;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::InitializeFeatureContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::InitializeFeatureContext(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pRequestObject)
    {
        CHX_LOG_ERROR("Invalid argument: NULL pRequestObject");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeatureRequestContext* pFeatureContext  = static_cast<ChiFeatureRequestContext*>(CHX_CALLOC(
                            sizeof(ChiFeatureRequestContext)));

        if (NULL != pFeatureContext)
        {
            pFeatureContext->privContext.pPrivateData  = NULL;
            pFeatureContext->privContext.size          = 0;
            pRequestObject->SetPrivContext(pFeatureContext);
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetFeatureRequestOpsData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::GetFeatureRequestOpsData (
    ChiFeature2RequestObject*        pRequestObject,
    ChiFeature2RequestObjectOpsType  opsType,
    CHIFEATURE2HANDLE                hOpsData,
    ChiFeature2HandleType            handleType,
    CHIFEATURE2HANDLE                hTypeData,
    UINT8                            requestId
    ) const
{
    CDKResult result     = CDKResultSuccess;

    switch (opsType)
    {
        case ChiFeature2RequestObjectOpsType::InputConfiguration:
        case ChiFeature2RequestObjectOpsType::InputDependency:
        {
            switch(handleType)
            {
                case ChiFeature2HandleType::BufferMetaInfo:
                {
                    ChiFeature2BufferMetadataInfo* pBufferMetaInfo =
                        static_cast<ChiFeature2BufferMetadataInfo *>(hTypeData);
                    ChiFeature2PortDescriptor*     pPortDesc =
                        static_cast<ChiFeature2PortDescriptor *>(hOpsData);

                    if ((NULL != pBufferMetaInfo) && (NULL != pPortDesc))
                    {
                        result = pRequestObject->GetBufferInfo(opsType,
                            &pPortDesc->globalId, &pBufferMetaInfo->hBuffer, &pBufferMetaInfo->key,
                            requestId, 0);
                        if (NULL == pBufferMetaInfo->hBuffer)
                        {
                            result = CDKResultEInvalidArg;
                        }
                    }
                    break;
                }
                default:
                    CHX_LOG_ERROR("Invalid handleType = %d. Need Implementation", handleType);
                    break;
            }
            break;
        }
        default:
            CHX_LOG_ERROR("Invalid opsType = %d. Need Implementation", opsType);
            break;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetBufferData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::GetBufferData(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     numData,
    ChiFeatureBufferData*     pOutputBuffer
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    CDKResult  result = CDKResultSuccess;

    if ((NULL == pOutputBuffer) || (NULL == pOutputBuffer->pBufferMeta))
    {
        CHX_LOG_ERROR("Invalid Argument: numData = %d", numData);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        // For ZSL case, need to use frameId from ZSL Queue

        for (UINT8 numPorts = 0; numPorts < numData; numPorts++)
        {
            ChiFeaturePipelineData*  pPipelinedata = GetPipelineData(&pOutputBuffer[numPorts].globalId);
            ChiFeaturePortData*      pPortData     = GetPortData(&pOutputBuffer[numPorts].globalId);
            PortZSLQueueData*        pZSLQueueData = NULL;
            if ((NULL == pPortData)     || (NULL == pPortData->pOutputBufferTbm) ||
                (NULL == pPipelinedata) || (NULL == pPipelinedata->pOutputMetaTbm))
            {
                CHX_LOG_ERROR("Invalid Argument: port = %d pPortData = %p",
                    pOutputBuffer[numPorts].globalId.port, pPortData);
                result = CDKResultEInvalidArg;
                break;
            }

            if (CDKResultSuccess == result)
            {
                CHITARGETBUFFERINFOHANDLE   hBuffer[1]                = {0};
                CHITargetBufferManager*     pBufferManager            = pPortData->pOutputBufferTbm;
                CHITargetBufferManager*     pMetaManager              = pPipelinedata->pOutputMetaTbm;
                UINT32                      latestAvailableSequenceId = 0;
                UINT32                      inputOffset               = 0;
                UINT32                      bufferOffset              = 0;
                std::vector<UINT32>         submitSequenceIdOnPort;

                for (UINT8 numBuf = 0; numBuf < pOutputBuffer[numPorts].numBufmeta; numBuf++)
                {
                    ChiTargetBufferCallbackData* pCallbackData = NULL;

                    GetZSLQueueForPort(pRequestObject, &pOutputBuffer[numPorts].globalId, &pZSLQueueData);
                    if (NULL != pZSLQueueData)
                    {
                        latestAvailableSequenceId = pZSLQueueData->frameNumbers.front();
                        pZSLQueueData->frameNumbers.pop();
                        pZSLQueueData->selectedZSLFrames.push_back(latestAvailableSequenceId);
                        CHX_LOG_INFO("Found ZSL Q for port %s %p popping frame %d", pPortData->pPortName,
                            pZSLQueueData, latestAvailableSequenceId);
                        pCallbackData = CHX_NEW ChiTargetBufferCallbackData;
                        if (NULL != pCallbackData)
                        {
                            pCallbackData->pCallback    = ChiFeature2Base::ProcessInFlightBufferCallBack;
                            pCallbackData->pPrivateData = &pZSLQueueData->inflightCallbackData;
                        }
                    }
                    else
                    {
                        submitSequenceIdOnPort    = pRequestObject->GetPortIdtoSeqIdVector(pOutputBuffer[numPorts].globalId);
                        latestAvailableSequenceId = submitSequenceIdOnPort.back();
                    }

                    CHX_LOG_INFO("%s, [%u, %u, %u, %d, %d] : latestAvailableSequenceId = %d",
                        pRequestObject->IdentifierString(),
                        pOutputBuffer[numPorts].globalId.session,
                        pOutputBuffer[numPorts].globalId.pipeline,
                        pOutputBuffer[numPorts].globalId.port,
                        pOutputBuffer[numPorts].globalId.portDirectionType,
                        pOutputBuffer[numPorts].globalId.portType,
                        latestAvailableSequenceId);

                    inputOffset = static_cast<UINT32>(pOutputBuffer[numPorts].offset);
                    if (latestAvailableSequenceId < inputOffset)
                    {
                        bufferOffset = (pBufferManager->GetFirstReadySequenceID() == INVALIDSEQUENCEID) ?
                            0 : latestAvailableSequenceId;
                    }
                    else
                    {
                        bufferOffset = latestAvailableSequenceId - inputOffset;
                    }

                    // For imagebuffer port type
                    if (ChiFeature2PortType::ImageBuffer == pPortData->globalId.portType)
                    {
                        result = pBufferManager->GetTargetBuffers(bufferOffset, 1, &hBuffer[0], TRUE, pCallbackData);
                        pOutputBuffer[numPorts].pBufferMeta[numBuf].hBuffer = hBuffer[0];
                        pOutputBuffer[numPorts].pBufferMeta[numBuf].key     =
                            reinterpret_cast<UINT64>(pPortData->pTarget->pChiStream);
                    }
                    else // For metadata port type
                    {
                        result = pMetaManager->GetTargetBuffers(bufferOffset, 1, &hBuffer[0], TRUE, pCallbackData);
                        pOutputBuffer[numPorts].pBufferMeta[numBuf].hBuffer = hBuffer[0];
                        pOutputBuffer[numPorts].pBufferMeta[numBuf].key     =
                            pPipelinedata->pPipeline->GetMetadataClientId();
                    }

                    if (NULL != pCallbackData)
                    {
                        CHX_DELETE(pCallbackData);
                    }
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::InitializeSequenceData.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::InitializeSequenceData(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult result = CDKResultSuccess;

    ChiFeatureSequenceData* pSequenceData = CHX_NEW ChiFeatureSequenceData;

    if (NULL != pSequenceData)
    {
        pSequenceData->frameNumber = m_frameNumber;
        pRequestObject->SetSequencePrivData(ChiFeature2SequenceOrder::Current, pSequenceData,
            pRequestObject->GetCurRequestId());
        m_frameNumber++;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::DestroySequenceData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::DestroySequenceData(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult result = CDKResultSuccess;

    for (UINT8 requestId = 0; requestId < pRequestObject->GetNumRequests(); ++ requestId)
    {
        INT32 sequenceId = pRequestObject->GetProcessSequenceId(
        ChiFeature2SequenceOrder::Current, requestId);

        if (InvalidProcessSequenceId != sequenceId)
        {
            for (INT32 index = 0; index <= sequenceId; index++)
            {
                ChiFeatureSequenceData* pData = static_cast<ChiFeatureSequenceData*>(
                    pRequestObject->GetSequencePrivDataById(index, requestId));

                pData->frameCbData.pInputPorts.clear();
                pData->frameCbData.pInputPorts.shrink_to_fit();
                pData->frameCbData.pOutputPorts.clear();
                pData->frameCbData.pOutputPorts.shrink_to_fit();

                if (NULL != pData)
                {
                    CHX_DELETE(pData);
                    pData = NULL;
                    pRequestObject->SetSequencePrivDataById(index, NULL,
                        requestId);
                }
            }
        }

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::InitializeThreadService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::InitializeThreadService()
{
    CDKResult result = CDKResultSuccess;

    if (NULL != m_pThreadManager)
    {
        result = m_pThreadManager->RegisterJobFamily(ThreadCallback, m_pFeatureName, &m_hFeatureJob);

        if ((CDKResultSuccess == result) && (0 != m_hFeatureJob))
        {
            CHX_LOG_INFO("ThreadManager %p feature %s job handle %" PRIx64,
                m_pThreadManager,
                m_pFeatureName,
                m_hFeatureJob);
        }
        else
        {
            CHX_LOG_ERROR("Faild to register a job, ThreadManager %p feature %s job handle %" PRIx64", result %d",
                m_pThreadManager,
                m_pFeatureName,
                m_hFeatureJob,
                result);

            result = CDKResultEFailed;
        }
    }
    else
    {
        CHX_LOG_ERROR("Faild to get ThreadManager %p feature %s", m_pThreadManager, m_pFeatureName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::DebugDataCopy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::DebugDataCopy(
    ChiFeature2RequestObject*   pRequestObject,
    ChiMetadata*                pInputMetadata
    ) const
{
    CDKResult                           result              = CDKResultSuccess;
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj  = NULL;
    DebugData*                          pOfflineDebugData   = NULL;
    UINT                                indexDebugData      = 0;
    BOOL                                allocDone           = FALSE;

    pUsecaseRequestObj = pRequestObject->GetUsecaseRequestObject();
    if (NULL != pUsecaseRequestObj)
    {
        pOfflineDebugData = pUsecaseRequestObj->GetOfflineDebugData();
    }

    if ((NULL != pOfflineDebugData) &&
        (TRUE == ChxUtils::IsVendorTagPresent(pInputMetadata, VendorTag::DebugDataTag)))
    {
        CHAR* pData = NULL;

        ChxUtils::GetVendorTagValue(pInputMetadata, VendorTag::DebugDataTag, (VOID**)&pData);
        if (NULL != pData)
        {
            DebugData* pDebug               = reinterpret_cast<DebugData*>(pData);

            if ((NULL != pDebug->pData) && (0 < pDebug->size))
            {
                // We have input debug-data available, now lets check if need to allocated offline data
                for (UINT i = 0; DebugDataMaxOfflineFrames > i; i++)
                {
                    if ((TRUE == allocDone) || (CDKResultSuccess != result))
                    {
                        // Done looking or fail to allocate
                        break;
                    }

                    if (pOfflineDebugData[i].pData == pDebug->pData)
                    {
                        // Skip new allocation and copy, use same buffer
                        allocDone       = TRUE;
                        indexDebugData  = i;
                    }
                    else if (NULL == pOfflineDebugData[i].pData)
                    {
                        // Allocate data for offline processing
                        result = ChxUtils::DebugDataAllocBuffer(&pOfflineDebugData[i]);
                        if (CDKResultSuccess == result)
                        {
                            allocDone       = TRUE;
                            indexDebugData  = i;
                        }
                        else
                        {
                            CHX_LOG_WARN("DebugDataAll: Fail to allocate offline debug-data (non-fatal error)");
                            allocDone       = FALSE;
                            indexDebugData  = i;
                        }
                    }
                }

                if ((TRUE           == allocDone) &&
                    (NULL           != pOfflineDebugData[indexDebugData].pData) &&
                    (pDebug->size   == pOfflineDebugData[indexDebugData].size))
                {
                    // Copy from source to offline debug-data buffer, but skip copy if already offline data
                    if (pOfflineDebugData[indexDebugData].pData != pDebug->pData)
                    {
                        CHX_LOG_INFO("DebugDataAll: Offline debug-data: %p, copy data source: %p",
                                     pOfflineDebugData[indexDebugData].pData,
                                     pDebug->pData);
                        ChxUtils::Memcpy(pOfflineDebugData[indexDebugData].pData, pDebug->pData, pDebug->size);
                    }

                    // Update vendor tag always to avoid free of real-time debug-data memory
                    result = ChxUtils::SetVendorTagValue(pInputMetadata,
                                                         VendorTag::DebugDataTag,
                                                         sizeof(DebugData),
                                                         &pOfflineDebugData[indexDebugData]);
                    if (CDKResultSuccess != result)
                    {
                        // Non-fatal error
                        CHX_LOG_WARN("DebugDataAll: fail to set DebugDataTag");
                    }
                }
                else
                {
                    // Non-fatal error
                    CHX_LOG_WARN("DebugDataAll: buffer not available or size does not match");
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::DumpDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::DumpDebugData(
    const VOID*                 pDebugData,
    const SIZE_T                sizeDebugData,
    const UINT                  requestNumber,
    const UINT                  frameNumber,
    const CHAR*                 pPipelineName,
    const ChiFeature2StageInfo* pStageInfo)
{
    CHAR                    dumpFilename[256];
    CHAR                    suffix[]            = "bin";
    const CHAR              parserString[19]    = "QTI Debug Metadata";
    DebugDataStartHeader    metadataHeader;
    CHAR                    timeBuf[128]        = { 0 };
    time_t                  currentTime;

    time(&currentTime);
    struct tm* pTimeInfo = localtime(&currentTime);
    if (NULL != pTimeInfo)
    {
        strftime(timeBuf, sizeof(timeBuf), "%Y%m%d_%H%M%S", pTimeInfo);
    }

    CdkUtils::SNPrintF(dumpFilename, sizeof(dumpFilename),
                       "%s/%s_debug-data_URO[%u]_frame[%u]_%s_stageId[%u]_seq[%u].%s",
                       FileDumpPath, timeBuf,
                       requestNumber, frameNumber, pPipelineName, pStageInfo->stageId, pStageInfo->stageSequenceId, suffix);

    metadataHeader.dataSize                = sizeof(parserString) + sizeof(metadataHeader) + sizeDebugData;
    metadataHeader.majorRevision           = 1;
    metadataHeader.minorRevision           = 1;
    metadataHeader.patchRevision           = 0;
    metadataHeader.SWMajorRevision         = 1;
    metadataHeader.SWMinorRevision         = 0;
    metadataHeader.SWPatchRevision         = 0;
    metadataHeader.featureDesignator[0]    = 'R';
    metadataHeader.featureDesignator[1]    = 'C';

    CHX_LOG_INFO("DebugDataAll: dumpFilename: %s, pDebugData: %p, sizeDebugData: %zu, sizeMeta: %u [0x%x]",
                 dumpFilename, pDebugData, sizeDebugData, metadataHeader.dataSize, metadataHeader.dataSize);

    FILE* pFile = CdkUtils::FOpen(dumpFilename, "wb");
    if (NULL != pFile)
    {
        CdkUtils::FWrite(parserString, sizeof(parserString), 1, pFile);
        CdkUtils::FWrite(&metadataHeader, sizeof(metadataHeader), 1, pFile);
        CdkUtils::FWrite(pDebugData, sizeDebugData, 1, pFile);
        CdkUtils::FClose(pFile);
    }
    else
    {
        CHX_LOG_WARN("DebugDataAll: Debug data failed to open for writing: %s", dumpFilename);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Base::ProcessDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::ProcessDebugData(
    ChiFeature2RequestObject*       pRequestObject,
    const ChiFeature2StageInfo*     pStageInfo,
    const ChiFeature2Identifier*    pPortIdentifier,
    ChiMetadata*                    pMetadata,
    UINT32                          frameNumber)
{
    CDKResult                           result              = CDKResultSuccess;
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj  = NULL;
    ChiFeaturePipelineData*             pPipelineData       = NULL;

    if ((NULL == pRequestObject) || (NULL == pStageInfo) ||
        (NULL == pPortIdentifier) || (NULL == pMetadata))
    {
        CHX_LOG_WARN("Invalid Argument pRequestObject = %p pStageInfo = %p"
                      "pPortIdentifier = %p pStreamBuffer = %p",
                      pRequestObject, pStageInfo, pPortIdentifier, pMetadata);
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        pPipelineData       = GetPipelineData(pPortIdentifier);
        pUsecaseRequestObj  = pRequestObject->GetUsecaseRequestObject();

        if ((NULL != pPipelineData) && (TRUE == ChxUtils::IsVendorTagPresent(pMetadata, VendorTag::DebugDataTag)))
        {
            CHAR* pData = NULL;
            ChxUtils::GetVendorTagValue(pMetadata, VendorTag::DebugDataTag, (VOID**)&pData);

            if (NULL != pData)
            {
                DebugData* pDebug = reinterpret_cast<DebugData*>(pData);
                if ((NULL != pDebug->pData) && (0 < pDebug->size))
                {
                    DumpDebugData(pDebug->pData,
                                  pDebug->size,
                                  pUsecaseRequestObj->GetAppFrameNumber(),
                                  frameNumber,
                                  pPipelineData->pPipelineName,
                                  pStageInfo);
                }
            }
            else
            {
                CHX_LOG_WARN("%s: Null DebugData tag",
                             pRequestObject->IdentifierString());
            }
        }
        else
        {
            CHX_LOG_WARN("%s: Missing DebugData",
                         pRequestObject->IdentifierString());
        }
    }

    if (CDKResultSuccess != result)
    {
        CHX_LOG_WARN("%s: Fail to process debug-data",
                     pRequestObject->IdentifierString());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetNumDependencyListsFromStageDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 ChiFeature2Base::GetNumDependencyListsFromStageDescriptor(
    const ChiFeature2StageDescriptor * pStageDescriptor,
    UINT8 sessionIndex,
    UINT8 pipelineIndex
    ) const
{
    UINT8 numDependencyLists = 0;

    if (NULL != pStageDescriptor)
    {
        if (sessionIndex < pStageDescriptor->numDependencyConfigDescriptor)
        {
            ChiFeature2SessionInfo sessionInfo = pStageDescriptor->pDependencyConfigDescriptor[sessionIndex];

            if (pipelineIndex < sessionInfo.numPipelines)
            {
                ChiFeature2PipelineInfo                 pipelineInfo = sessionInfo.pPipelineInfo[pipelineIndex];
                ChiFeature2DependencyConfigDescriptor*  pDescriptorList = reinterpret_cast
                    <ChiFeature2DependencyConfigDescriptor*>(pipelineInfo.handle);

                numDependencyLists = pDescriptorList->numInputDependency;
            }
            else
            {
                CHX_LOG_ERROR("Pipeline index %d is greater than total number of pipelines %d", pipelineIndex,
                    sessionInfo.numPipelines);
            }
        }
        else
        {
            CHX_LOG_ERROR("Session index %d is greater than total number of sessions %d", sessionIndex,
                pStageDescriptor->numDependencyConfigDescriptor);
        }
    }
    else
    {
        CHX_LOG_ERROR("Stage descriptor is NULL!");
    }
    return numDependencyLists;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PopulateDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::PopulateDependency (
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    result = OnPopulateDependency(pRequestObject);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PopulateDependencyPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::PopulateDependencyPorts(
    ChiFeature2RequestObject*         pRequestObject,
    UINT8                             dependencyIndex,
    const ChiFeature2InputDependency* pInputDependency
    ) const
{
    CDKResult     result            = CDKResultSuccess;
    ChiMetadata*  pFeatureSettings  = NULL;
    UINT8         requestId         = pRequestObject->GetCurRequestId();


    if (CDKResultSuccess == result)
    {
        for (UINT8 portIndex = 0; portIndex < pInputDependency->numInputDependency; ++portIndex)
        {
            ChiFeature2PortDescriptor portDescriptor = pInputDependency->pInputDependency[portIndex];
            ChiFeature2Identifier     portidentifier = portDescriptor.globalId;

            result = pRequestObject->SetPortDescriptor(ChiFeature2SequenceOrder::Next,
                ChiFeature2RequestObjectOpsType::InputDependency,
                &portidentifier, &portDescriptor, requestId, dependencyIndex);

            CHX_LOG_INFO("%s: Set dependency on port %s, requestId %d, dependencyIndex %d", pRequestObject->IdentifierString(),
                     portDescriptor.pPortName, requestId,
                     dependencyIndex);

            if (portidentifier.portType == ChiFeature2PortType::MetaData)
            {
                pFeatureSettings = ChiMetadata::Create(NULL, 0, true);
                if (NULL != pFeatureSettings)
                {
                    result = PopulateDependencySettings(pRequestObject, dependencyIndex, &portidentifier, pFeatureSettings);
                }
                else
                {
                    CHX_LOG_ERROR("Alloc memory failed");
                    result = CDKResultENoMemory;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PopulateDependencySettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::PopulateDependencySettings(
    ChiFeature2RequestObject*     pRequestObject,
    UINT8                         dependencyIndex,
    const ChiFeature2Identifier*  pSettingPortId,
    ChiMetadata*                  pFeatureSettings
    ) const
{
    CDKResult result     = CDKResultSuccess;
    UINT8     requestId  = pRequestObject->GetCurRequestId();

    result = OnPopulateDependencySettings(pRequestObject, dependencyIndex, pSettingPortId, pFeatureSettings);

    if (CDKResultSuccess == result)
    {
        CHIMETAHANDLE  hSetting = pFeatureSettings->GetHandle();
        if (NULL != hSetting)
        {
            CHX_LOG_INFO("Add Dependency: %s: %d %p %d, pSetting:%p ", m_pFeatureName, dependencyIndex,
                hSetting, pSettingPortId->port, pFeatureSettings);
            pRequestObject->SetRequestInputInfo(
                ChiFeature2SequenceOrder::Next, pSettingPortId,
                hSetting, dependencyIndex, requestId);
        }
        else
        {
            CHX_LOG_WARN("%s, hSetting is NULL", pRequestObject->IdentifierString());
        }
    }
    else
    {
        CHX_LOG_WARN("%s OnPopulateDependencySettings failed!", pRequestObject->IdentifierString());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PrepareZSLQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::PrepareZSLQueue(
    ChiFeature2RequestObject*     pRequestObject,
    const ChiFeature2PortIdList*  pList,
    const ChiFeature2Identifier*  pSyncPort,
    const ChiFeature2Identifier*  pSettingsPort
    ) const
{
    CDKResult result = CDKResultSuccess;

    ChiFeatureRequestContext* pRequestContext = NULL;

    ChiFeaturePortData*       pPortData       = GetPortData(pSyncPort);

    if ((NULL != pPortData) && (NULL != pPortData->pOutputBufferTbm))
    {
        CHITargetBufferManager* pManager = pPortData->pOutputBufferTbm;

        std::vector<UINT32> consumerlist = pManager->GetAllSequenceId(SearchOption::SearchConsumerList);

        for (UINT32 index = 0; index < consumerlist.size(); ++index)
        {
            CHX_LOG_INFO("Current Consumer list %d", consumerlist.at(index));
        }

        std::vector<UINT32> producerList = pManager->GetAllSequenceId(SearchOption::SearchProducerList);

        for (UINT32 index = 0; index < producerList.size(); ++index)
        {
            CHX_LOG_INFO("Current Producer list %d", producerList.at(index));
        }

        std::vector<INT32> offsets = GetZSLOffsets(pRequestObject, producerList, consumerlist, pSettingsPort);

        std::sort(offsets.begin(), offsets.end());

        std::queue<UINT32> finalList;
        for (UINT8 index = 0; index < offsets.size(); ++index)
        {
            INT32 offset = offsets.at(index);
            // Negative offsets are picked from consumer list
            if (offset < 0)
            {
                finalList.push(consumerlist.at(static_cast<INT32>(consumerlist.size()) + offset));
            }
            else
            {
                finalList.push(producerList.at(offset));
            }
            CHX_LOG_INFO("Pushing element to ZSL Queue %d", finalList.back());
        }

        pRequestContext = static_cast<ChiFeatureRequestContext*>(pRequestObject->GetPrivContext());

        for (UINT8 portIndex = 0; portIndex < pList->numPorts; ++ portIndex)
        {
            PortZSLQueueData* pPortZSLQueue = CHX_NEW(PortZSLQueueData);

            if (NULL != pPortZSLQueue)
            {
                ChiFeature2InFlightCallbackData* pCallbackData = &pPortZSLQueue->inflightCallbackData;

                pCallbackData->pRequestObject   = pRequestObject;
                pCallbackData->pFeatureInstance = this;
                pCallbackData->portId           = pList->pPorts[portIndex];

                pPortZSLQueue->globalId     = pList->pPorts[portIndex];
                pPortZSLQueue->frameNumbers = finalList;

                CHX_LOG_INFO("Assigning final list for port %d %p", pList->pPorts[portIndex].port, &finalList);
                pRequestContext->pPortZSLQueues.push_back(pPortZSLQueue);
            }
            else
            {
                CHX_LOG_ERROR("Out of memory");
            }

        }
        consumerlist.clear();
        producerList.clear();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnPopulateDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnPopulateDependency (
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult             result      = CDKResultSuccess;
    ChiFeature2StageInfo  stageInfo   = { 0 };
    UINT8                 requestId   = 0;

    UINT8                                numRequestOutputs  = 0;
    const ChiFeature2RequestOutputInfo*  pRequestOutputInfo = NULL;
    const ChiFeature2StageDescriptor*    pStageDescriptor   = NULL;

    requestId       = pRequestObject->GetCurRequestId();
    pRequestObject->GetNextStageInfo(&stageInfo, requestId);
    pRequestObject->GetExternalRequestOutput(&numRequestOutputs, &pRequestOutputInfo, requestId);

    if (InvalidStageId != stageInfo.stageId)
    {
        pStageDescriptor = GetStageDescriptor(stageInfo.stageId);
    }

    if (NULL != pStageDescriptor)
    {
        for (UINT8 sessionIdx = 0; sessionIdx < pStageDescriptor->numDependencyConfigDescriptor; sessionIdx++)
        {
            const ChiFeature2SessionInfo* pSessionInfo = &pStageDescriptor->pDependencyConfigDescriptor[sessionIdx];

            for (UINT8 pipelineId = 0; pipelineId < pSessionInfo->numPipelines; pipelineId++)
            {
                const ChiFeature2PipelineInfo*          pPipelineInfo    = &pSessionInfo->pPipelineInfo[pipelineId];
                ChiFeature2DependencyConfigDescriptor*  pDescriptorList  =
                    reinterpret_cast<ChiFeature2DependencyConfigDescriptor*>(pPipelineInfo->handle);

                if (NULL != pDescriptorList)
                {
                    for (UINT8 listIndex = 0; listIndex < pDescriptorList->numInputDependency; ++listIndex)
                    {
                        const ChiFeature2InputDependency* pDependency =
                            GetDependencyListFromStageDescriptor(pStageDescriptor,
                                sessionIdx, pipelineId, listIndex);

                        if (NULL != pDependency)
                        {
                            result = PopulateDependencyPorts(pRequestObject, listIndex, pDependency);
                        }
                    }
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetExternalInputSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata* ChiFeature2Base::GetExternalInputSettings(
    ChiFeature2RequestObject*     pRequestObject,
    const ChiFeature2Identifier*  pMetadataPortId,
    const UINT8                   requestId
    ) const
{
    CDKResult    result             = CDKResultSuccess;
    ChiMetadata* pSrcFeatureSetting = NULL;

    if ((NULL == pRequestObject) || (NULL == pMetadataPortId))
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        UINT8                                numRequestOutputs  = 0;
        const ChiFeature2RequestOutputInfo*  pRequestOutputInfo = NULL;

        result = pRequestObject->GetExternalRequestOutput(&numRequestOutputs,
            &pRequestOutputInfo, requestId);

        for (UINT32 index = 0; index < numRequestOutputs; index++)
        {
            if (*pMetadataPortId == pRequestOutputInfo[index].pPortDescriptor->globalId)
            {
                pSrcFeatureSetting = GetMetadataManager()->GetMetadataFromHandle(
                    pRequestOutputInfo[index].bufferMetadataInfo.hMetadata);
            }
        }
    }
    return pSrcFeatureSetting;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnPopulateDependencySettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnPopulateDependencySettings(
    ChiFeature2RequestObject*     pRequestObject,
    UINT8                         dependencyIndex,
    const ChiFeature2Identifier*  pSettingPortId,
    ChiMetadata*                  pFeatureSettings
    ) const
{
    // Base class implementation merges all down stream dependency settings.
    // Derived class should override otherwise

    CDK_UNREFERENCED_PARAM(dependencyIndex);
    CDK_UNREFERENCED_PARAM(pSettingPortId);
    CDKResult  result     = CDKResultSuccess;
    UINT8      requestId  = 0;

    UINT8                                numRequestOutputs  = 0;
    const ChiFeature2RequestOutputInfo*  pRequestOutputInfo = NULL;
    ChiMetadata*                         pSrcFeatureSetting = NULL;

    if (NULL == pFeatureSettings || NULL == pRequestObject)
    {
        CHX_LOG_ERROR("pFeatureSettings=%p, pRequestObject=%p",
            pFeatureSettings, pRequestObject);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        requestId  = pRequestObject->GetCurRequestId();
        result     = pRequestObject->GetExternalRequestOutput(&numRequestOutputs,
            &pRequestOutputInfo, requestId);

        for (UINT32 index = 0; index < numRequestOutputs; index++)
        {
            if (ChiFeature2PortType::MetaData ==
                pRequestOutputInfo[index].pPortDescriptor->globalId.portType)
            {
                pSrcFeatureSetting = GetExternalInputSettings(pRequestObject,
                    &pRequestOutputInfo[index].pPortDescriptor->globalId, pRequestObject->GetCurRequestId());

                if (NULL != pSrcFeatureSetting)
                {
                    result = pFeatureSettings->Copy(*pSrcFeatureSetting, true);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnProcessingDependenciesComplete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::OnProcessingDependenciesComplete(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId)
{
    // If all of our ports are done processing and our request is the last request (applicable to batched requests)
    // then send the notification
    if (FALSE == pRequestObject->GetOutputPortsProcessingCompleteNotified())
    {
        if ((TRUE == pRequestObject->AllOutputPortsProcessingComplete()) &&
            ((pRequestObject->GetNumRequests() - 1) == requestId))
        {
            CHX_LOG_INFO("%s All ports are done processing; generating processing dependencies complete feature message",
                            pRequestObject->IdentifierString());

            ChiFeature2MessageDescriptor featurePortMessage;

            ChxUtils::Memset(&featurePortMessage, 0, sizeof(ChiFeature2MessageDescriptor));
            featurePortMessage.messageType  = ChiFeature2MessageType::ProcessingDependenciesComplete;
            featurePortMessage.pPrivData    = pRequestObject;
            pRequestObject->SetCurRequestId(requestId);
            ProcessFeatureMessage(&featurePortMessage);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::MergeAppSettings()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::MergeAppSettings(
    ChiFeature2RequestObject* pRequestObject,
    ChiMetadata**             ppResultMetadata,
    BOOL                      isDisjoint
    ) const
{
    CDKResult result = CDKResultSuccess;

    // Merge all output ports' feature settings with App meta for RT feature
    ChiMetadata*  pResultMetadata = *ppResultMetadata;

    if (NULL == pResultMetadata)
    {
        CHX_LOG_ERROR("Get result metadata from setting TBM failed");
        result = CDKResultEInvalidPointer;
    }
    else
    {
        // Merge APP metadata disjoint
        ChiMetadata* pAppMeta = NULL;
        pRequestObject->GetAppRequestSetting(&pAppMeta);
        if (NULL == pAppMeta)
        {
            CHX_LOG_ERROR("APP metadata is NULL");
            result = CDKResultEInvalidPointer;
        }
        else
        {
            pResultMetadata->Merge(*pAppMeta, isDisjoint);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::PopulateConfigurationSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::PopulateConfigurationSettings(
    ChiFeature2RequestObject*     pRequestObject,
    const ChiFeature2Identifier*  pMetadataPortId,
    ChiMetadata*                  pInputMetadata
    ) const
{
    CDKResult result     = CDKResultSuccess;
    UINT8     requestId  = pRequestObject->GetCurRequestId();

    ChiFeatureSequenceData*  pRequestData = static_cast<ChiFeatureSequenceData*>(
        pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current, requestId));

    result = OnPopulateConfigurationSettings(pRequestObject, pMetadataPortId, pInputMetadata);

    if (NULL != pRequestData)
    {
        pRequestData->pInputMetadata = pInputMetadata;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnPopulateConfigurationSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnPopulateConfigurationSettings(
    ChiFeature2RequestObject*     pRequestObject,
    const ChiFeature2Identifier*  pMetadataPortId,
    ChiMetadata*                  pInputMetadata
    ) const
{
    CDKResult                       result          = CDKResultSuccess;
    ChiModeUsecaseSubModeType       usecaseMode     = pRequestObject->GetFeatureUsecaseMode();
    UINT8                           requestId       = pRequestObject->GetCurRequestId();
    UINT32                          sensorModeIndex;

    ChiMetadata* pResultMetadata = GetExternalOutputSettings(pRequestObject,
        pMetadataPortId, requestId, 0);

    if (NULL != pResultMetadata)
    {
        pInputMetadata->Copy(*pResultMetadata, true);
    }

    sensorModeIndex = GetSensorModeIndex(pMetadataPortId);
    ChxUtils::FillDefaultTuningMetadata(pInputMetadata,
        usecaseMode,
        sensorModeIndex);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetExternalOutputSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiMetadata* ChiFeature2Base::GetExternalOutputSettings(
    ChiFeature2RequestObject*     pRequestObject,
    const ChiFeature2Identifier*  pMetadataPortId,
    UINT8                         requestId,
    UINT8                         dependencyIndex
    ) const
{
    ChiMetadata* pResultMetadata = NULL;

    ChiFeatureSequenceData*  pRequestData = static_cast<ChiFeatureSequenceData*>(
        pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current, pRequestObject->GetCurRequestId()));

    if ((ChiFeature2PortDirectionType::InternalInput == pMetadataPortId->portDirectionType) ||
        (ChiFeature2PortDirectionType::ExternalInput == pMetadataPortId->portDirectionType))
    {
        ChiFeature2BufferMetadataInfo inputBufferMetaInfo = { 0 };
        pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
            pMetadataPortId, &inputBufferMetaInfo.hBuffer, &inputBufferMetaInfo.key,
            requestId, dependencyIndex);

        if (NULL != inputBufferMetaInfo.hBuffer)
        {
            CHIMETAHANDLE hMetadataBuffer = NULL;
            GetMetadataBuffer(inputBufferMetaInfo.hBuffer,
                              inputBufferMetaInfo.key,
                              &hMetadataBuffer);

            if (NULL != hMetadataBuffer)
            {
                pResultMetadata =
                    GetMetadataManager()->GetMetadataFromHandle(hMetadataBuffer);
            }
        }
    }
    return pResultMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnSelectFlowToExecuteRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RequestFlowType ChiFeature2Base::OnSelectFlowToExecuteRequest(
    ChiFeature2RequestObject*       pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);

    return ChiFeature2RequestFlowType::Invalid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::OnExecuteProcessRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    ChiFeature2RequestFlowType flowtype = ChiFeature2RequestFlowType::Invalid;
    CDKResult                  result   = CDKResultSuccess;

    flowtype = OnSelectFlowToExecuteRequest(pRequestObject);

    CHX_LOG_INFO("%s Executing base request flow %d", pRequestObject->IdentifierString(), flowtype);

    ExecuteBaseRequestFlow(pRequestObject, flowtype);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::DeActivatePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Base::DeActivatePipeline(
    const ChiFeature2Identifier* pKey,
    CHIDEACTIVATEPIPELINEMODE    modeBitmask)
{
    CDKResult   result                = CDKResultSuccess;
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    if ((NULL != pKey) && (NULL != pExtensionModule))
    {
        ChiFeatureSessionData*  pSessionData    = GetSessionData(pKey);
        ChiFeaturePipelineData* pPipelineData   = GetPipelineData(pKey);

        if ((NULL != pSessionData) && (NULL != pPipelineData))
        {
            Session*   pSession  = pSessionData->pSession;
            Pipeline*  pPipeline = pPipelineData->pPipeline;
            if ((NULL != pSession) && (NULL != pPipeline))
            {
                CHX_LOG_INFO("DeActivatePipeline name %s", pPipeline->GetPipelineName());
                result = pExtensionModule->DeactivatePipeline(pSession->GetSessionHandle(),
                    pPipeline->GetPipelineHandle(),
                    modeBitmask);
                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Deactivate %s failed!", pPipeline->GetPipelineName());
                }
            }
            else
            {
                CHX_LOG_ERROR("Invalid session or pipeline handles");
                result = CDKResultEFailed;
            }
        }
        else
        {
            CHX_LOG_ERROR("Invalid session or pipeline data");
            result = CDKResultEFailed;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Argument ChiFeature2Identifier");
        result = CDKResultEFailed;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::Deactivate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::Deactivate()
{
    ExtensionModule* pExtensionModule = ExtensionModule::GetInstance();

    // Destroy pipeline and session
    for (auto pSessionData : m_pSessionData)
    {
        Session* pSession = pSessionData->pSession;
        if (NULL != pSession)
        {
            for (auto pPipelineData : pSessionData->pPipelineData)
            {
                Pipeline* pPipeline = pPipelineData->pPipeline;
                if (NULL != pPipeline)
                {
                    pExtensionModule->DeactivatePipeline(pSession->GetSessionHandle(),
                        pPipeline->GetPipelineHandle(), CHIDeactivateModeUnlinkPipeline);
                }
            }

        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnBufferResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Base::OnBufferResult(
    ChiFeature2RequestObject*  pRequestObject,
    UINT8                      resultId,
    ChiFeature2StageInfo*      pStageInfo,
    ChiFeature2Identifier*     pPortIdentifier,
    const CHISTREAMBUFFER*     pStreamBuffer,
    UINT32                     frameNumber,
    VOID*                      pPrivateData)
{
    ChiFeaturePortData*         pPortData       = NULL;
    BOOL                        sendToGraph     = FALSE;
    CDKResult                   result          = CDKResultSuccess;
    BOOL                        resultStatus    = TRUE;
    ChiTargetStatus             bufferStatus    = ChiTargetStatus::Ready;

    if ((NULL == pRequestObject) || (NULL == pStageInfo) ||
        (NULL == pPortIdentifier) || (NULL == pStreamBuffer))
    {
        CHX_LOG_ERROR("Invalid Argument pRequestObject = %p pStageInfo = %p"
            "pPortIdentifier = %p pStreamBuffer = %p",
            pRequestObject, pStageInfo, pPortIdentifier, pStreamBuffer);
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        pPortData = GetPortData(pPortIdentifier);
        if (NULL == pPortData)
        {
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        ChiFeatureSequenceData* pRequestData = static_cast<ChiFeatureSequenceData*>(pPrivateData);
        if (NULL != pRequestData)
        {
            resultStatus = (FALSE == pRequestData->skipSequence) ?
                TRUE : FALSE;
            bufferStatus = (FALSE == resultStatus) ? ChiTargetStatus::Error :
                ChiTargetStatus::Ready;
        }
    }

    if (CDKResultSuccess == result)
    {
        CHITargetBufferManager*     pBufferManager  = pPortData->pOutputBufferTbm;

        CHX_LOG_INFO("%s Result from Request:%d for port:%s stageId %d stageSequenceId %d stream width  height format "
                     "%d %d %d ",
                     pRequestObject->IdentifierString(),
                     frameNumber,
                     pPortData->pPortName,
                     pStageInfo->stageId,
                     pStageInfo->stageSequenceId,
                     pStreamBuffer->pStream->width,
                     pStreamBuffer->pStream->height,
                     pStreamBuffer->pStream->format);

        if (NULL != pBufferManager)
        {
            CHITARGETBUFFERINFOHANDLE* phBuffer = NULL;

            if (ChiFeature2PortDirectionType::ExternalOutput ==
                pPortData->globalId.portDirectionType)
            {
                BOOL isOutputGeneratedForPort = pRequestObject->GetOutputGeneratedForPort(pPortData->globalId, resultId);

                ChiFeature2BufferMetadataInfo* pBufferMetaInfo  = NULL;

                pRequestObject->GetFinalBufferMetadataInfo(pPortData->globalId,
                                                           &pBufferMetaInfo, resultId);

                if ((NULL != pBufferMetaInfo) && (FALSE == isOutputGeneratedForPort) &&
                    (TRUE == resultStatus))
                {
                    phBuffer              = &pBufferMetaInfo->hBuffer;
                    pBufferMetaInfo->key  = reinterpret_cast<UINT64>(
                            pPortData->pTarget->pChiStream);
                    sendToGraph           = TRUE;
                }
            }

            result = pBufferManager->UpdateTarget(frameNumber,
                                         reinterpret_cast<UINT64>(pPortData->pTarget->pChiStream),
                                         pStreamBuffer->bufferInfo.phBuffer,
                                         bufferStatus,
                                         phBuffer);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Unable to get target buffer for port %s", pPortData->pPortName);
                sendToGraph = FALSE;
            }
        }

    }
    return sendToGraph;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnBufferError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::OnBufferError(
    ChiFeature2RequestObject* pRequestObject,
    ChiFeature2Identifier*    pPortIdentifier,
    UINT8                     resultId
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    CDK_UNUSED_PARAM(pPortIdentifier);
    CDK_UNUSED_PARAM(resultId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnPartialMetadataResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Base::OnPartialMetadataResult(
    ChiFeature2RequestObject*  pFeatureReqObj,
    UINT8                      resultId,
    ChiFeature2StageInfo*      pStageInfo,
    ChiFeature2Identifier*     pPortIdentifier,
    ChiMetadata*               pMetadata,
    UINT32                     frameNumber,
    VOID*                      pPrivateData)
{
    ChiFeaturePipelineData*  pPipelineData   = NULL;
    BOOL                     sendToGraph     = FALSE;
    CDKResult                result          = CDKResultSuccess;

    if ((NULL == pFeatureReqObj) || (NULL == pStageInfo) ||
        (NULL == pPortIdentifier) || (NULL == pMetadata))
    {
        CHX_LOG_ERROR("Invalid Argument pRequestObject = %p pStageInfo = %p"
            "pPortIdentifier = %p pStreamBuffer = %p",
            pFeatureReqObj, pStageInfo, pPortIdentifier, pMetadata);
        result = CDKResultEFailed;
    }

    m_pProcessResultLock->Lock();

    if (CDKResultSuccess == result)
    {
        pPipelineData = GetPipelineData(pPortIdentifier);
        if (NULL == pPipelineData)
        {
            result = CDKResultEFailed;
        }
    }

    if ((CDKResultSuccess == result) && (NULL != pPipelineData))
    {
        ChiFeatureSequenceData* pRequestData = NULL;
        BOOL                    resultStatus = TRUE;

        CHX_LOG_INFO("%s : Request:%d for pipeline:%s stageId %d stageSequenceId %d pMetadata %p ",
            pFeatureReqObj->IdentifierString(),
            frameNumber,
            pPipelineData->pPipelineName,
            pStageInfo->stageId,
            pStageInfo->stageSequenceId,
            pMetadata);

        pRequestData = static_cast<ChiFeatureSequenceData*>(pPrivateData);
        if (NULL != pRequestData)
        {
            resultStatus = (FALSE == pRequestData->skipSequence) ?
                TRUE : FALSE;
        }

        if (ChiFeature2PortDirectionType::ExternalOutput == pPortIdentifier->portDirectionType)
        {
            ChiFeature2BufferMetadataInfo* pBufferMetaInfo  = NULL;

            pFeatureReqObj->GetFinalBufferMetadataInfo(*pPortIdentifier,
                &pBufferMetaInfo, resultId);

            BOOL isOutputGeneratedForPort = pFeatureReqObj->GetOutputGeneratedForPort(
                *pPortIdentifier, resultId);

            if ((NULL != pBufferMetaInfo) &&
                (FALSE == isOutputGeneratedForPort) &&
                (TRUE == resultStatus))
            {
                CHITargetBufferManager*     pBufferManager = pPipelineData->pOutputMetaTbm;
                CHITARGETBUFFERINFOHANDLE   hBuffer        = NULL;

                if (NULL != pBufferManager)
                {
                    pBufferManager->GetCHIHandle(frameNumber,
                        pPipelineData->metadataClientId, &hBuffer);

                    if (NULL == pBufferMetaInfo->hBuffer)
                    {
                        pBufferMetaInfo->hBuffer = hBuffer;
                        pBufferMetaInfo->key     = pPipelineData->metadataClientId;
                        sendToGraph = TRUE;
                    }
                    else
                    {
                        CHX_LOG_INFO("%s Metadata already generated for frame %d",
                            pFeatureReqObj->IdentifierString(),
                            frameNumber);
                        sendToGraph = FALSE;
                    }

                }
                else
                {
                    CHX_LOG_ERROR("Unable to get target buffer for pipeline %s",
                        pPipelineData->pPipelineName);
                }
            }
        }
    }
    m_pProcessResultLock->Unlock();

    return sendToGraph;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnMetadataResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Base::OnMetadataResult(
    ChiFeature2RequestObject*  pRequestObject,
    UINT8                      resultId,
    ChiFeature2StageInfo*      pStageInfo,
    ChiFeature2Identifier*     pPortIdentifier,
    ChiMetadata*               pMetadata,
    UINT32                     frameNumber,
    VOID*                      pPrivateData)
{
    ChiFeaturePipelineData* pPipelineData = NULL;
    BOOL                    sendToGraph   = FALSE;
    CDKResult               result        = CDKResultSuccess;

    if ((NULL == pRequestObject) || (NULL == pStageInfo) ||
        (NULL == pPortIdentifier) || (NULL == pMetadata))
    {
        CHX_LOG_ERROR("Invalid Argument pRequestObject = %p pStageInfo = %p"
            "pPortIdentifier = %p pStreamBuffer = %p",
            pRequestObject, pStageInfo, pPortIdentifier, pMetadata);
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        pPipelineData = GetPipelineData(pPortIdentifier);
        if (NULL == pPipelineData)
        {
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result && NULL != pPipelineData)
    {
        CHITargetBufferManager*    pBufferManager  = pPipelineData->pOutputMetaTbm;
        CHITARGETBUFFERINFOHANDLE  hBuffer[1]      = { 0 };
        BOOL                       resultStatus    = TRUE;
        ChiTargetStatus            bufferStatus    = ChiTargetStatus::Ready;

        ChiFeatureSequenceData* pRequestData = static_cast<ChiFeatureSequenceData*>(pPrivateData);
        if (NULL != pRequestData)
        {
            resultStatus = (FALSE == pRequestData->skipSequence) ?
                TRUE : FALSE;
            bufferStatus = (FALSE == resultStatus) ? ChiTargetStatus::Error :
                ChiTargetStatus::Ready;
        }

        CHX_LOG_INFO("%s Result from Request:%d for pipeline:%s stageId %d stageSequenceId %d pMetadata %p ",
                 pRequestObject->IdentifierString(),
                 frameNumber,
                 pPipelineData->pPipelineName,
                 pStageInfo->stageId,
                 pStageInfo->stageSequenceId,
                 pMetadata);

        if (NULL != pBufferManager)
        {
            CHITARGETBUFFERINFOHANDLE* phBuffer = NULL;

            if ((ChiFeature2PortDirectionType::ExternalOutput == pPortIdentifier->portDirectionType) &&
                (ChiTargetStatus::Ready == bufferStatus))
            {
                BOOL isOutputGeneratedForPort = pRequestObject->GetOutputGeneratedForPort(*pPortIdentifier, resultId);

                ChiFeature2BufferMetadataInfo* pBufferMetaInfo  = NULL;
                pRequestObject->GetFinalBufferMetadataInfo(*pPortIdentifier,
                    &pBufferMetaInfo, resultId);

                if ((NULL != pBufferMetaInfo) && (FALSE == isOutputGeneratedForPort) &&
                    (TRUE == resultStatus))
                {
                    phBuffer              = &pBufferMetaInfo->hBuffer;
                    pBufferMetaInfo->key  = pPipelineData->metadataClientId;

                    pRequestObject->SetOutputGeneratedForPort(*pPortIdentifier, resultId);
                    sendToGraph = TRUE;
                }
            }

            result = pBufferManager->UpdateTarget(frameNumber, pPipelineData->metadataClientId,
                pMetadata, bufferStatus, phBuffer);

            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Unable to get target buffer for pipeline %s", pPipelineData->pPipelineName);
                sendToGraph = FALSE;
            }
        }

    }

    return sendToGraph;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::OnReleaseDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Base::OnReleaseDependencies(
    const ChiFeature2Identifier*      pIdentifier,
    UINT8                             dependencyIndex,
    ChiFeature2StageInfo*             pStageInfo,
    ChiFeature2RequestObject*         pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pIdentifier);
    CDK_UNUSED_PARAM(dependencyIndex);
    CDK_UNUSED_PARAM(pRequestObject);
    CDK_UNUSED_PARAM(pStageInfo);

    BOOL release = TRUE;

    return release;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::DumpInputMetaBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::DumpInputMetaBuffer(
    ChiFeature2BufferMetadataInfo* pInputMetaInfo,
    CHAR*                          pBaseName,
    UINT                           index
    ) const
{
    CDKResult               result       = CDKResultSuccess;
    ChiMetadata*            pMetadata    = NULL;
    CHITargetBufferManager* pMetadataTBM = NULL;

    if (NULL == pInputMetaInfo)
    {
        result = CDKResultEInvalidArg;
    }

    // Dump metadata to file
    if (CDKResultSuccess == result)
    {
        pMetadataTBM = CHITargetBufferManager::GetTargetBufferManager(pInputMetaInfo->hBuffer);
    }

    if (NULL != pMetadataTBM)
    {
        pMetadata = static_cast<ChiMetadata*>(pMetadataTBM->GetTarget(
            pInputMetaInfo->hBuffer, pInputMetaInfo->key));
    }
    else
    {
        result = CDKResultEFailed;
    }

    if ((CDKResultSuccess == result) && (NULL != pMetadata))
    {
        CHAR metaName[256] = { 0 };

        // Determine the metadata dump file name
        CdkUtils::SNPrintF(metaName, sizeof(metaName), "%s_%d.bin", pBaseName, index);

        DumpMetadata(pMetadata, metaName);
    }

    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("DumpInputMetaBuffer failed!");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::DumpInputImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::DumpInputImageBuffer(
    ChiFeature2BufferMetadataInfo* pInputBufferInfo,
    CHAR*                          pBaseName,
    UINT                           index
    ) const
{
    CDKResult               result        = CDKResultSuccess;
    CHITargetBufferManager* pImageTBM     = NULL;
    CHISTREAMBUFFER*        pStreamBuffer = NULL;

    if (NULL == pInputBufferInfo)
    {
        result = CDKResultEInvalidArg;
    }

    // Dump raw image to file
    if (CDKResultSuccess == result)
    {
        pImageTBM = CHITargetBufferManager::GetTargetBufferManager(pInputBufferInfo->hBuffer);
    }

    if (NULL != pImageTBM)
    {
        pStreamBuffer = static_cast<CHISTREAMBUFFER*>(pImageTBM->GetTarget(
            pInputBufferInfo->hBuffer, pInputBufferInfo->key));
    }
    else
    {
        result = CDKResultEFailed;
    }

    if ((CDKResultSuccess == result) && (NULL != pStreamBuffer))
    {
        CHX_LOG_INFO("Dump image format=%d, WxH=%dx%d, planeStride=%d, sliceHeight=%d",
                        pStreamBuffer->pStream->format,
                        pStreamBuffer->pStream->width,
                        pStreamBuffer->pStream->height,
                        pStreamBuffer->pStream->streamParams.planeStride,
                        pStreamBuffer->pStream->streamParams.sliceHeight);

        CHAR imageName[256] = { 0 };

        // Determine the image file dump name
        CdkUtils::SNPrintF(imageName, sizeof(imageName),
                           "/data/vendor/camera/%s_%d.raw",
                           pBaseName, index);

        DumpRawImage(pStreamBuffer, imageName);
    }

    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("DumpInputImageBuffer failed!");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetUseCaseString
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::GetUseCaseString(
    ChiModeUsecaseSubModeType useCase,
    std::string&              output
    ) const
{
    switch (useCase)
    {
        case ChiModeUsecaseSubModeType::Preview:
        {
            output = "P";

            break;
        }

        case ChiModeUsecaseSubModeType::Snapshot:
        {
            output = "S";

            break;
        }

        case ChiModeUsecaseSubModeType::Video:
        {
            output = "V";

            break;
        }

        case ChiModeUsecaseSubModeType::ZSL:
        {
            output = "Z";

            break;
        }

        case ChiModeUsecaseSubModeType::Liveshot:
        {
            output = "L";

            break;
        }

        default:
        {
            output = "ERR";

            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Base::GetDumpFileBaseName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Base::GetDumpFileBaseName(
    ChiFeature2RequestObject* pRequestObject,
    CHAR*                     pDumpFileBaseName,
    UINT                      size
    ) const
{
    CHIDateTime dateTime              = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    ChiModeUsecaseSubModeType useCase = pRequestObject->GetUsecaseRequestObject()->GetUsecaseMode();
    std::string useCaseString         = "ERR";
    UINT32 requestID                  = pRequestObject->GetUsecaseRequestObject()->GetAppFrameNumber();

    // Get the current data and time
    CdkUtils::GetDateTime(&dateTime);

    // Get the string associated with the current use case
    GetUseCaseString(useCase, useCaseString);

    CdkUtils::SNPrintF(pDumpFileBaseName, sizeof(CHAR) * size, "Cam%d_L%dI%d_%s_RQ%d_%04d%02d%02d%02d%02d%02d",
                       m_pCameraInfo->ppDeviceInfo[m_pInstanceProps->cameraId]->cameraId, m_pCameraInfo->cameraId,
                       m_pInstanceProps->cameraId, useCaseString.c_str(), requestID,
                       dateTime.year + 1900, dateTime.month + 1, dateTime.dayOfMonth, dateTime.hours,
                       dateTime.minutes, dateTime.seconds);
}
