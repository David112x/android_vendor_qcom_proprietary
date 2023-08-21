////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2stub.cpp
/// @brief Stub CHI feature2 implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2stub.h"
#include "chifeature2utils.h"
#include "chifeature2featurepool.h"

static const UINT32 Feature2MajorVersion = 0;
static const UINT32 Feature2MinorVersion = 0;
static const UINT8  MaxBufferCount       = 16;
static const UINT   ThreadSleepTime      = 25000;
static const CHAR*  VendorName           = "QTI";

static const CHAR*  Feature2StubCaps[] =
{
    "Stub",
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Stub* ChiFeature2Stub::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    CDKResult        result   = CDKResultSuccess;
    ChiFeature2Stub* pFeature = CHX_NEW(ChiFeature2Stub);

    result = pFeature->Initialize(pCreateInputInfo);

    if (CDKResultSuccess != result)
    {
        CHX_DELETE pFeature;
        pFeature = NULL;
    }

    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Stub::Destroy()
{
    if (NULL != m_pStubThreadMutex)
    {
        m_pStubThreadMutex->Lock();
        m_stubRequestProcessTerminate = TRUE;
        m_pStubRequestAvailable->Signal();
        m_pStubThreadMutex->Unlock();
    }

    if (NULL != m_stubRequestProcessThread.pPrivateData)
    {
        ChxUtils::ThreadTerminate(m_stubRequestProcessThread.hThreadHandle);
        m_stubRequestProcessThread = { 0 };
    }

    if (NULL != m_pStubThreadMutex)
    {
        m_pStubThreadMutex->Destroy();
        m_pStubThreadMutex = NULL;
    }

    if (NULL != m_pStubRequestAvailable)
    {
        m_pStubRequestAvailable->Destroy();
        m_pStubRequestAvailable = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::OnInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Stub::OnInitialize(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    const ChiFeature2Descriptor* pFeatureDescriptor = pCreateInputInfo->pFeatureDescriptor;
    for (UINT32 seesionIdx = 0; seesionIdx < pFeatureDescriptor->numSessions; seesionIdx++)
    {
        for (UINT32 pipelineIdx = 0; pipelineIdx < pFeatureDescriptor->pSession[seesionIdx].numPipelines; pipelineIdx++)
        {
            const ChiFeature2PipelineDescriptor Pipeline = pFeatureDescriptor->pSession[seesionIdx].pPipeline[pipelineIdx];
            for (UINT32 numInput = 0; numInput < Pipeline.numInputPorts; numInput++)
            {
                for (UINT32 numStream = 0; numStream < pCreateInputInfo->pStreamConfig->numStreams; numStream++)
                {
                    ChiFeature2Identifier pKey  = Pipeline.pInputPortDescriptor[numInput].globalId;
                    ChiFeaturePortData*   pPort = GetPortData(&pKey);
                    CHISTREAM*            pTemp = CreateStream();
                    pTemp                       = pCreateInputInfo->pStreamConfig->pChiStreams[numStream];

                    if (NULL != pPort)
                    {
                        pPort->maxBufferCount = MaxBufferCount;
                        pPort->pChiStream     = pTemp;
                    }
                }
            }
        }
    }

    for (UINT32 seesionIdx = 0; seesionIdx < pFeatureDescriptor->numSessions; seesionIdx++)
    {
        for (UINT32 pipelineIdx = 0; pipelineIdx < pFeatureDescriptor->pSession[seesionIdx].numPipelines; pipelineIdx++)
        {
            const ChiFeature2PipelineDescriptor Pipeline = pFeatureDescriptor->pSession[seesionIdx].pPipeline[pipelineIdx];
            for (UINT32 numOutput = 0; numOutput < Pipeline.numOutputPorts; numOutput++)
            {
                for (UINT32 numStream = 0; numStream < pCreateInputInfo->pStreamConfig->numStreams; numStream++)
                {
                    ChiFeature2Identifier pKey  = Pipeline.pOutputPortDescriptor[numOutput].globalId;
                    ChiFeaturePortData*   pPort = GetPortData(&pKey);
                    CHISTREAM*            pTemp = CreateStream();
                    pTemp                       = pCreateInputInfo->pStreamConfig->pChiStreams[numStream];

                    if (NULL != pPort)
                    {
                        pPort->maxBufferCount = MaxBufferCount;
                        pPort->pChiStream     = pTemp;
                    }
                }
            }
        }
    }

    m_pStubThreadMutex                      = Mutex::Create();
    m_pStubRequestAvailable                 = Condition::Create();
    m_stubRequestProcessTerminate           = FALSE;
    m_stubRequestProcessThread.pPrivateData = this;

    CDKResult result = CDKResultSuccess;
    result           = ChxUtils::ThreadCreate(ChiFeature2Stub::RequestThread,
                                              &m_stubRequestProcessThread,
                                              &m_stubRequestProcessThread.hThreadHandle);

    if (CDKResultSuccess == result)
    {
        CHX_LOG_INFO("StubThread Created");
    }
    else
    {
        result = CDKResultEFailed;
        CHX_LOG_ERROR("StubThread Create Failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::DoPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Stub::DoPrepareRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::OnExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Stub::OnExecuteProcessRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult result             = CDKResultSuccess;
    UINT8     stageId            = InvalidStageId;
    UINT8     nextStageId        = InvalidStageId;
    UINT8     numDependencyLists = 0;
    UINT8     numStages          = GetNumStages();



    const ChiFeature2StageDescriptor* pStageDescriptor = NULL;
    ChiFeature2StageInfo              stageInfo        = { 0 };
    ChiFeature2PortIdList             outputList       = { 0 };
    ChiFeature2PortIdList             inputList        = { 0 };

    if (NULL != pRequestObject)
    {
        result = GetCurrentStageInfo(pRequestObject, &stageInfo);
    }
    else
    {
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        stageId         = stageInfo.stageId;
        nextStageId     = stageId + 1;

        if (InvalidStageId == stageId)
        {
            // Setting this tells the request object how many overall sequences we will run
            SetConfigInfo(pRequestObject, numStages);
            pStageDescriptor = GetStageDescriptor(nextStageId);

            if (NULL != pStageDescriptor)
            {
                numDependencyLists = GetNumDependencyListsFromStageDescriptor(pStageDescriptor, 0, 0);
                SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, numDependencyLists);
                PopulateDependency(pRequestObject);
            }
        }
        else
        {
            result = GetOutputPortsForStage(stageId, &outputList);

            if (CDKResultSuccess == result)
            {
                result = GetInputPortsForStage(stageId, &inputList);
            }

            if (CDKResultSuccess == result)
            {
                result = PopulatePortConfiguration(pRequestObject, &inputList, &outputList);
            }

            if (CDKResultSuccess == result)
            {
                m_pStubThreadMutex->Lock();
                CHX_LOG_INFO("StubThread[%s] pRequestObject: %p EnQueued, thread signaled",
                             pRequestObject->IdentifierString(),
                             pRequestObject);
                // EnQueue(pRequestObject);
                m_pStubRequestAvailable->Signal();
                m_pStubThreadMutex->Unlock();
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::RequestThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2Stub::RequestThread(
    VOID* pThreadData)
{
    PerThreadData*   pPerThreadData   = reinterpret_cast<PerThreadData*>(pThreadData);

    ChiFeature2Stub* pChiFeature2Stub = reinterpret_cast<ChiFeature2Stub*>(pPerThreadData->pPrivateData);

    pChiFeature2Stub->StubThreadHandler();

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::StubThreadHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Stub::StubThreadHandler()
{
    while (TRUE)
    {
        m_pStubThreadMutex->Lock();
        ChiFeature2RequestObject* pRequestObject = NULL;

        if (Empty())
        {
            CHX_LOG_INFO("StubThread Wait");
            m_pStubRequestAvailable->Wait(m_pStubThreadMutex->GetNativeHandle());
            CHX_LOG_INFO("StubThread Go");
        }

        if (!Empty())
        {
            pRequestObject = DeQueue();
            CHX_LOG_INFO("[%s]Begin StubThread DeQueue pRequestObject: %p",
                          pRequestObject->IdentifierString(),
                          pRequestObject);
        }

        m_pStubThreadMutex->Unlock();

        if (m_stubRequestProcessTerminate == TRUE)
        {
            break;
        }

        ChxUtils::SleepMicroseconds(ThreadSleepTime);

        ExecuteProcessRequestHelper(pRequestObject);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::ExecuteProcessRequestHelper
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Stub::ExecuteProcessRequestHelper(
    ChiFeature2RequestObject* pRequestObject)
{
    CDKResult result             = CDKResultSuccess;
    UINT8     stageId            = InvalidStageId;
    UINT8     nextStageId        = InvalidStageId;
    UINT8     numDependencyLists = 0;


    const ChiFeature2StageDescriptor* pStageDescriptor = NULL;
    ChiFeature2StageInfo              stageInfo        = { 0 };
    ChiFeature2PortIdList             outputList       = { 0 };
    ChiFeature2PortIdList             inputList        = { 0 };

    if (NULL != pRequestObject)
    {
        CHX_LOG_INFO("StubThread[%s] Execute for pRequestObject: %p",
                     pRequestObject->IdentifierString(),
                     pRequestObject);
        result = GetCurrentStageInfo(pRequestObject, &stageInfo);
    }
    else
    {
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        stageId         = stageInfo.stageId;
        nextStageId     = stageId + 1;

        result = GetOutputPortsForStage(stageId, &outputList);

        if (CDKResultSuccess == result)
        {
            result = GetInputPortsForStage(stageId, &inputList);
        }

        if (CDKResultSuccess == result)
        {
            ChiFeatureSequenceData*  pRequestData = static_cast<ChiFeatureSequenceData*>(
                pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current, pRequestObject->GetCurRequestId()));

            UINT64 timeStamp   = 0;
            UINT64 frameNumber = 0;

            if (NULL != pRequestData)
            {
                frameNumber = pRequestData->frameNumber;
                timeStamp   = frameNumber + 1;
            }

            for (UINT32 outputIdx = 0; outputIdx < outputList.numPorts; outputIdx++)
            {
                // Handle Metadata Buffer
                ChiFeature2BufferMetadataInfo outputBufferMetaInfo = { 0 };
                ChiFeature2Identifier         portidentifier       = outputList.pPorts[outputIdx];
                ChiFeaturePipelineData*       pPipelineData        = GetPipelineData(&portidentifier);
                ChiFeaturePortData*           pPortData            = GetPortData(&portidentifier);
                ChiFeatureSequenceData*       pSequenceData        = static_cast<ChiFeatureSequenceData*>(
                    pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current, pRequestObject->GetCurRequestId()));

                pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::OutputConfiguration,
                                              &outputList.pPorts[outputIdx],
                                              &outputBufferMetaInfo.hBuffer,
                                              &outputBufferMetaInfo.key,
                                              pRequestObject->GetCurRequestId(),
                                              0);

                ChiFeature2BufferMetadataInfo* pBufferMetaInfo = NULL;

                pRequestObject->GetFinalBufferMetadataInfo(outputList.pPorts[outputIdx],
                                                           &pBufferMetaInfo,
                                                           pRequestObject->GetCurRequestId());

                if (NULL != pBufferMetaInfo)
                {
                    pBufferMetaInfo->hBuffer = outputBufferMetaInfo.hBuffer;
                    pBufferMetaInfo->key     = outputBufferMetaInfo.key;
                }

                if (NULL != pPortData)
                {
                    if (ChiFeature2PortType::ImageBuffer == pPortData->globalId.portType)
                    {
                        CHITargetBufferManager* pTargetBufferManager =
                            CHITargetBufferManager::GetTargetBufferManager(outputBufferMetaInfo.hBuffer);

                        if (NULL != pTargetBufferManager)
                        {
                            CHISTREAMBUFFER* pChiStreamBuffer = reinterpret_cast<CHISTREAMBUFFER*>
                            (pTargetBufferManager->GetTarget(outputBufferMetaInfo.hBuffer,
                                                             reinterpret_cast<UINT64>(pPortData->pChiStream)));

                            if (NULL != pChiStreamBuffer)
                            {
                                CHITARGETBUFFERINFOHANDLE hBuffer = NULL;
                                result = pTargetBufferManager->UpdateTarget(frameNumber,
                                                                   reinterpret_cast<UINT64>(pPortData->pChiStream),
                                                                   pChiStreamBuffer->bufferInfo.phBuffer,
                                                                   ChiTargetStatus::Ready,
                                                                   &hBuffer);
                            }
                        }
                    }

                    if (ChiFeature2PortType::MetaData == pPortData->globalId.portType)
                    {
                        if (NULL != pPipelineData && NULL != pPipelineData->pOutputMetaTbm)
                        {
                            ChiMetadata* pChiMetadata = reinterpret_cast<ChiMetadata*>
                                (pPipelineData->pOutputMetaTbm->GetTarget(outputBufferMetaInfo.hBuffer,
                                                                          pPipelineData->metadataClientId));

                            if (NULL != pChiMetadata)
                            {
                                CHITARGETBUFFERINFOHANDLE hBuffer[1] = { 0 };
                                result = pPipelineData->pOutputMetaTbm->UpdateTarget(frameNumber,
                                                                            pPipelineData->metadataClientId,
                                                                            pChiMetadata,
                                                                            ChiTargetStatus::Ready,
                                                                            &hBuffer[0]);

                                pChiMetadata->SetTag(ANDROID_SENSOR_TIMESTAMP, &timeStamp, 1);
                            }
                        }
                    }
                }
            }

            // Populate ProcessMessage Data
            CHIMESSAGEDESCRIPTOR*        pMessageDescriptor = CHX_NEW CHIMESSAGEDESCRIPTOR;
            ChiFeatureFrameCallbackData* pFrameCallback     = CHX_NEW ChiFeatureFrameCallbackData;

            pFrameCallback->pRequestObj   = pRequestObject;
            pMessageDescriptor->pPrivData = static_cast<VOID*>(pFrameCallback);
            pMessageDescriptor->message.shutterMessage.frameworkFrameNum = frameNumber;
            pMessageDescriptor->message.shutterMessage.timestamp         = timeStamp;

            // Send Shutter
            pMessageDescriptor->messageType = ChiMessageTypeShutter;
            ProcessMessage(pMessageDescriptor, static_cast<VOID*>(this));

            // Send SOF
            pMessageDescriptor->messageType = ChiMessageTypeSof;
            ProcessMessage(pMessageDescriptor, static_cast<VOID*>(this));

            CHX_DELETE pMessageDescriptor;
            CHX_DELETE pFrameCallback;
            pMessageDescriptor = NULL;
            pFrameCallback     = NULL;

            // Set State to OutputNotificationPending before featureMessage
            pRequestObject->SetCurRequestState(ChiFeature2RequestState::OutputNotificationPending,
                                               pRequestObject->GetCurRequestId());

            // Send ProcessFeatureMessage for Buffer
            ChiFeature2MessageDescriptor featureMessage;
            ChxUtils::Memset(&featureMessage, 0, sizeof(ChiFeature2MessageDescriptor));
            featureMessage.messageType             = ChiFeature2MessageType::ResultNotification;
            featureMessage.message.result.numPorts = outputList.numPorts;
            featureMessage.message.result.pPorts   = outputList.pPorts;
            featureMessage.pPrivData               = pRequestObject;
            ProcessFeatureMessage(&featureMessage);

            ProcessReleaseDependency(pRequestObject, 0, 0, NULL);

            if (nextStageId < GetNumStages())
            {
                pStageDescriptor = GetStageDescriptor(nextStageId);

                if (NULL != pStageDescriptor)
                {
                    numDependencyLists = GetNumDependencyListsFromStageDescriptor(pStageDescriptor, 0, 0);
                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, 1);
                    PopulateDependency(pRequestObject);
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("Unable to get port information for stage %d", stageId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::DeQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RequestObject* ChiFeature2Stub::DeQueue()
{
    UINT32 idx = m_stubThreadQueueRear;

    if (StubThreadQueueSize > m_stubThreadQueueRear)
    {
        m_stubThreadQueueRear++;
    }
    else
    {
        m_stubThreadQueueRear = 0;
    }

    return m_pStubThreadQueue[idx];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::EnQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Stub::EnQueue(
    ChiFeature2RequestObject* pRequestObject)
{
    m_pStubThreadQueue[m_stubThreadQueueFront] = pRequestObject;

    if (StubThreadQueueSize > m_stubThreadQueueFront)
    {
        m_stubThreadQueueFront++;
    }
    else
    {
        m_stubThreadQueueFront = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::Empty
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Stub::Empty()
{
    BOOL isEmpty = FALSE;

    if (m_stubThreadQueueFront == m_stubThreadQueueRear)
    {
        isEmpty = TRUE;
    }

    return isEmpty;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::DoProcessResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Stub::DoProcessResult(
    CHICAPTURERESULT * pResult,
    VOID * pPrivateCallbackData)
{
    CDK_UNUSED_PARAM(pResult);
    CDK_UNUSED_PARAM(pPrivateCallbackData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::DoProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Stub::DoProcessMessage(
    const CHIMESSAGEDESCRIPTOR * pMessageDescriptor,
    VOID * pPrivateCallbackData)
{
    CDK_UNUSED_PARAM(pMessageDescriptor);
    CDK_UNUSED_PARAM(pPrivateCallbackData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::DoProcessPartialResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Stub::DoProcessPartialResult(
    CHIPARTIALCAPTURERESULT* pCaptureResult,
    VOID*                    pPrivateCallbackData)
{
    CDK_UNUSED_PARAM(pCaptureResult);
    CDK_UNUSED_PARAM(pPrivateCallbackData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Stub::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Stub::DoFlush()
{
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Stub::DoDestroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Stub::DoDestroy()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2Stub* pFeatureStub = ChiFeature2Stub::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeatureStub);;
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
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2StubCaps);
        pQueryInfo->ppCapabilities = &Feature2StubCaps[0];
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
    StreamNegotiationInfo*      pNegotiationInfo,
    StreamNegotiationOutput*    pNegotiationOutput)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pNegotiationInfo) || (NULL == pNegotiationOutput))
    {
        CHX_LOG_ERROR("Invalid Arg! pNegotiation=%p, pDesiredStreamConfig=%p", pNegotiationInfo, pNegotiationOutput);
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

        ChiFeature2Type featureId = static_cast<ChiFeature2Type>(pNegotiationInfo->pFeatureInstanceId->featureId);

        switch (featureId)
        {
            case ChiFeature2Type::STUB_RT:
            case ChiFeature2Type::STUB_B2Y:
            {
                pNegotiationOutput->pDesiredStreamConfig->numStreams       = pNegotiationInfo->pFwkStreamConfig->numStreams;
                pNegotiationOutput->pDesiredStreamConfig->pChiStreams      = pNegotiationInfo->pFwkStreamConfig->pChiStreams;
                pNegotiationOutput->pDesiredStreamConfig->operationMode    = pNegotiationInfo->pFwkStreamConfig->operationMode;
                pNegotiationOutput->pDesiredStreamConfig->pSessionSettings =
                                                                          pNegotiationInfo->pFwkStreamConfig->pSessionSettings;
            }
            break;

            default:
                CHX_LOG_ERROR("Invalid featureID! featureId:%d", featureId);
                result = CDKResultEInvalidArg;
                break;
        }
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
    CHIFEATURE2OPS* pChiFeature2StubOps)
{
    if (NULL != pChiFeature2StubOps)
    {
        pChiFeature2StubOps->size               = sizeof(CHIFEATURE2OPS);
        pChiFeature2StubOps->majorVersion       = Feature2MajorVersion;
        pChiFeature2StubOps->minorVersion       = Feature2MinorVersion;
        pChiFeature2StubOps->pVendorName        = VendorName;
        pChiFeature2StubOps->pCreate            = CreateFeature;
        pChiFeature2StubOps->pQueryCaps         = DoQueryCaps;
        pChiFeature2StubOps->pGetVendorTags     = GetVendorTags;
        pChiFeature2StubOps->pStreamNegotiation = DoStreamNegotiation;
    }
}
#ifdef __cplusplus
}
#endif // __cplusplus
