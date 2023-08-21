////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2serializer.cpp
/// @brief Serializer feature implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2serializer.h"
#include "chifeature2base.h"
#include "chifeature2featurepool.h"
#include "chifeature2utils.h"

// NOWHINE FILE CP006:  Used standard libraries for performance improvements

static const UINT32 Feature2SerializerMajorVersion  = 0;
static const UINT32 Feature2SerializerMinorVersion  = 0;
static const CHAR*  VendorName                      = "QTI";
static const CHAR*  Feature2SerializerCaps[]        =
{
    "Serializer",
};

static const UINT8 OutputMappingStage        = 0;
static const UINT8 RDIPortId                 = 0;
static const UINT8 MetadataPortId            = 1;
extern const ChiFeature2PortDescriptor SerializerInputPortDescriptors[];
extern const ChiFeature2PortDescriptor SerializerOutputPortDescriptors[];

// Dependency & input / output port mapping. Derived Serializer can customize this map.
struct ChiFeature2SerializerMapInfo MapInfo[] =
{
    {
        0,
        0,
        SerializerInputPortDescriptors[0],
        SerializerOutputPortDescriptors[0],
    },
    {
        0,
        0,
        SerializerInputPortDescriptors[1],
        SerializerOutputPortDescriptors[1],
    },
    {
        1,
        0,
        SerializerInputPortDescriptors[2],
        SerializerOutputPortDescriptors[0],
    },
    {
        1,
        0,
        SerializerInputPortDescriptors[3],
        SerializerOutputPortDescriptors[1],
    },
    {
        2,
        1,
        SerializerInputPortDescriptors[2],
        SerializerOutputPortDescriptors[0],
    },
    {
        2,
        1,
        SerializerInputPortDescriptors[3],
        SerializerOutputPortDescriptors[1],
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Serializer* ChiFeature2Serializer::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{

    CDKResult              result   = CDKResultSuccess;
    ChiFeature2Serializer* pFeature = NULL;

    if (NULL != pCreateInputInfo)
    {
        pFeature = CHX_NEW(ChiFeature2Serializer);

        if (NULL != pFeature)
        {
            result = pFeature->Initialize(pCreateInputInfo);

            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Create Failed!");
                pFeature->Destroy();
                pFeature = NULL;
            }
        }
        else
        {
            CHX_LOG_ERROR("No Memory! New Feature failed!");
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Arg! pCreateInputInfo is NULL!");
    }

    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Serializer::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::OnInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Serializer::OnInitialize(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    CDKResult                    result           = CDKResultSuccess;
    const ChiFeature2Descriptor* pFeature2Desc    = pCreateInputInfo->pFeatureDescriptor;
    ChiMetadataManager*          pMetadataManager = pCreateInputInfo->pMetadataManager;
    if (NULL != pFeature2Desc)
    {
        ChiFeature2Identifier globalId;
        for (UINT8 sessionIdx = 0 ; sessionIdx < pFeature2Desc->numSessions ; ++sessionIdx)
        {
            const ChiFeature2SessionDescriptor* pSessionDesc = &(pFeature2Desc->pSession[sessionIdx]);
            globalId.session                                 = pSessionDesc->sessionId;

            for (UINT8 pipelineIdx = 0; pipelineIdx < pSessionDesc->numPipelines; ++pipelineIdx)
            {
                const ChiFeature2PipelineDescriptor* pPipelineDesc =
                    &(pFeature2Desc->pSession[sessionIdx].pPipeline[pipelineIdx]);

                globalId.pipeline                       = pPipelineDesc->pipelineId;
                ChiFeaturePipelineData* pPipelineData   = GetPipelineData(&globalId);

                if ((NULL != pPipelineData) && (NULL != pMetadataManager))
                {
                    UINT32 metadataClientId = pMetadataManager->RegisterClient(FALSE,
                                                                               NULL,
                                                                               0,
                                                                               0,
                                                                               1,
                                                                               ChiMetadataUsage::OfflineOutput);
                    SetMetadataClientId(pPipelineData, metadataClientId);
                }
                else
                {
                    CHX_LOG_ERROR("RegisterMetadataClient failed! pPipelineData=%p, pMetadataManager=%p",
                        pPipelineData, pMetadataManager);
                    result = CDKResultEInvalidArg;
                    break;
                }

                CHX_LOG_INFO("numOfinputPorts:%d, numOfOutputPorts:%d",
                    pPipelineDesc->numInputPorts, pPipelineDesc->numOutputPorts);

                for (UINT8 portIdx = 0; portIdx < pPipelineDesc->numInputPorts; ++portIdx)
                {
                    // only assign stream for buffer port
                    if (pPipelineDesc->pInputPortDescriptor[portIdx].globalId.portType ==
                        ChiFeature2PortType::ImageBuffer)
                    {
                        for (UINT8 streamIdx = 0; streamIdx < pCreateInputInfo->pStreamConfig->numStreams; ++streamIdx)
                        {
                            if (ChiStreamTypeInput ==
                                pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx]->streamType)
                            {
                                ChiFeature2Identifier key = pPipelineDesc->pInputPortDescriptor[portIdx].globalId;
                                ChiFeaturePortData* pPort = GetPortData(&key);
                                if (NULL != pPort)
                                {
                                    pPort->pChiStream = pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx];
                                    CHX_LOG_INFO("Assign stream:Inputport:%d->streamindex:%d", portIdx, streamIdx);
                                }
                                else
                                {
                                    CHX_LOG_ERROR("GetPortData failed! portKey[%d,%d,%d,%d,%d]",
                                        key.session,
                                        key.pipeline,
                                        key.port,
                                        key.portDirectionType,
                                        key.portType);
                                }
                            }
                        }
                    }
                }

                for (UINT8 portIdx = 0; portIdx < pPipelineDesc->numOutputPorts; ++portIdx)
                {
                    ChiFeature2Identifier key = pPipelineDesc->pOutputPortDescriptor[portIdx].globalId;
                    ChiFeaturePortData* pPort = GetPortData(&key);

                    if (NULL == pPort)
                    {
                        CHX_LOG_ERROR("GetPortData failed! portKey[%d,%d,%d,%d,%d]",
                            key.session,
                            key.pipeline,
                            key.port,
                            key.portDirectionType,
                            key.portType);
                        result = CDKResultEFailed;
                        break;
                    }

                    // only assign stream for buffer port
                    if (pPipelineDesc->pOutputPortDescriptor[portIdx].globalId.portType ==
                        ChiFeature2PortType::ImageBuffer)
                    {
                        for (UINT8 streamIdx = 0; streamIdx < pCreateInputInfo->pStreamConfig->numStreams; ++streamIdx)
                        {
                            if (ChiStreamTypeOutput ==
                                pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx]->streamType)
                            {

                                pPort->pChiStream =
                                    pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx];
                                CHX_LOG_INFO("Assign stream:Outputport:%d->streamindex:%d", portIdx, streamIdx);
                            }
                        }
                    }
                    else
                    {
                        UINT32 metadataClientId = pMetadataManager->RegisterClient(FALSE,
                                                                                   NULL,
                                                                                   0,
                                                                                   0,
                                                                                   1,
                                                                                   ChiMetadataUsage::OfflineOutput);

                        pPort->metadataClientId = metadataClientId;
                    }
                }
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("pFeature2Desc is NULL pointer");
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Serializer::OnPrepareRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;
    if (NULL != pRequestObject)
    {
        ChiFeature2SerializerPrivateInfo* pPrivContext =
            static_cast<ChiFeature2SerializerPrivateInfo*>(GetFeaturePrivContext(pRequestObject));

        if ((NULL == pPrivContext) && (NULL != m_pCameraInfo))
        {
            pPrivContext = static_cast<ChiFeature2SerializerPrivateInfo*>(CHX_CALLOC(sizeof(ChiFeature2SerializerPrivateInfo)));

            if (NULL != pPrivContext)
            {
                pPrivContext->numInputDependency    = 6;
                pPrivContext->pPortMap              = &MapInfo[0];

                SetFeaturePrivContext(pRequestObject, pPrivContext);
            }
            else
            {
                CHX_LOG_ERROR("Allocate pPrivContext failed!");
                result = CDKResultENoMemory;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::GetInputFromDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Serializer::GetInputFromDependency(
    ChiFeature2RequestObject*   pRequestObject,
    UINT8                       inputBatchIdx,
    UINT8                       dependencyIdx,
    ChiFeature2Identifier*      pInputPortId,
    VOID*                       pTarget
    ) const
{
    CDKResult                       result              = CDKResultEFailed;
    ChiFeature2BufferMetadataInfo   inputBufferMetaInfo = { 0 };

    if (pInputPortId->portType == ChiFeature2PortType::ImageBuffer)
    {
        pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
            pInputPortId, &inputBufferMetaInfo.hBuffer,
            &inputBufferMetaInfo.key, inputBatchIdx, dependencyIdx);

        CHITargetBufferManager* pTargetBufferManager =
            CHITargetBufferManager::GetTargetBufferManager(inputBufferMetaInfo.hBuffer);

        result = GetStreamBuffer(inputBufferMetaInfo.hBuffer, inputBufferMetaInfo.key,
            static_cast<CHISTREAMBUFFER*>(pTarget));

        if (CDKResultSuccess == result)
        {
            CHX_LOG_VERBOSE("DependencyIdx:%d targetHandle:%p",
                dependencyIdx,
                inputBufferMetaInfo.hBuffer);
        }
        else
        {
            CHX_LOG_ERROR("DependencyIdx:%d GetStreamBuffer failed! hBuffer=%p",
                dependencyIdx,
                inputBufferMetaInfo.hBuffer);
        }

    }
    else if (pInputPortId->portType == ChiFeature2PortType::MetaData)
    {
        pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
            pInputPortId, &inputBufferMetaInfo.hBuffer,
            &inputBufferMetaInfo.key, 0, dependencyIdx);

        CHITargetBufferManager* pTargetBufferManager =
            CHITargetBufferManager::GetTargetBufferManager(inputBufferMetaInfo.hBuffer);

        result = GetMetadataBuffer(inputBufferMetaInfo.hBuffer, inputBufferMetaInfo.key,
            static_cast<CHIMETAHANDLE*>(pTarget));

        if (CDKResultSuccess == result)
        {
            CHX_LOG_VERBOSE("InputIdx:%d targetHandle:%p",
                dependencyIdx,
                inputBufferMetaInfo.hBuffer);
        }
        else
        {
            CHX_LOG_ERROR("InputIdx:%d GetMetadataBuffer failed! hBuffer=%p",
                dependencyIdx,
                inputBufferMetaInfo.hBuffer);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::HandleDependencyMapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Serializer::HandleDependencyMapping(
    ChiFeature2RequestObject*   pRequestObject
    ) const
{
    CDKResult                           result              = CDKResultSuccess;
    ChiMetadata*                        pFeatureSettings    = NULL;
    ChiFeature2SerializerPrivateInfo*   pPrivContext        = static_cast<ChiFeature2SerializerPrivateInfo*>(
                                                                GetFeaturePrivContext(pRequestObject));

    UINT8                               requestId           = 0;
    UINT8                               dependencyIdx       = 0;
    ChiFeature2PortDescriptor           inputPort;
    ChiFeature2PortDescriptor           outputPort;



    if (NULL == pPrivContext)
    {
        CHX_LOG_ERROR("Invalid arg pPrivContext is NULL!");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT8 inputIdx = 0; inputIdx < pPrivContext->numInputDependency; inputIdx++)
        {
            inputPort       = pPrivContext->pPortMap[inputIdx].inputPort;
            outputPort      = pPrivContext->pPortMap[inputIdx].outputPort;
            requestId       = pPrivContext->pPortMap[inputIdx].requestId;
            dependencyIdx   = pPrivContext->pPortMap[inputIdx].dependencyIdx;

            result = pRequestObject->SetPortDescriptor(ChiFeature2SequenceOrder::Next,
                ChiFeature2RequestObjectOpsType::InputDependency,
                &inputPort.globalId, &inputPort, pRequestObject->GetCurRequestId(), dependencyIdx);

            CHX_LOG_INFO("%s: Set dependency inputIdx %d on port %s, requestId %d, dependencyIndex %d",
                         pRequestObject->IdentifierString(),
                         inputIdx,
                         inputPort.pPortName,
                         pRequestObject->GetCurRequestId(),
                         dependencyIdx);

            if (inputPort.globalId.portType == ChiFeature2PortType::MetaData)
            {
                ChiMetadata* pSrcMeta = GetExternalInputSettings(pRequestObject, &outputPort.globalId, requestId);
                if (NULL != pSrcMeta)
                {
                    pFeatureSettings = ChiMetadata::Create(NULL, 0, true);
                    if (NULL != pFeatureSettings)
                    {

                        VOID* pEV = pSrcMeta->GetTag(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
                        if (NULL != pEV)
                        {
                            CHX_LOG_INFO("Populate dependency setting:%d, EV:%d, pSrcMeta:%p, portid:%d",
                                requestId, *(static_cast<UINT32*>(pEV)),
                                pSrcMeta,
                                inputPort.globalId.port);
                        }
                        pFeatureSettings->Copy(*pSrcMeta, true);

                        result =
                            PopulateDependencySettings(pRequestObject, dependencyIdx, &(inputPort.globalId), pFeatureSettings);
                    }
                    else
                    {
                        CHX_LOG_ERROR("Alloc memory failed");
                        result = CDKResultENoMemory;
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Getting External input settings failed");
                    result = CDKResultEFailed;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::HandleOutputMapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Serializer::HandleOutputMapping(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultEInvalidArg;
    if (NULL != pRequestObject)
    {
        ChiFeature2PortIdList               outputPortList      = { 0 };
        ChiFeature2BufferMetadataInfo*      pFinalBufferMeta    = NULL;
        ChiFeaturePortData*                 pOutputPortData     = NULL;
        CHITargetBufferManager*             pOutputTBM          = NULL;
        ChiFeature2SerializerPrivateInfo*   pPrivContext        = static_cast<ChiFeature2SerializerPrivateInfo*>(
                                                                    GetFeaturePrivContext(pRequestObject));
        UINT64                              targetKey           = 0xFFFFFFFF;
        VOID*                               pTarget             = NULL;
        CHISTREAMBUFFER                     streamBuffer        = { 0 };
        CHIMETAHANDLE                       hMetaHandle         = NULL;
        UINT8                               requestId           = 0;
        UINT8                               dependencyIdx       = 0;
        ChiFeature2PortDescriptor           inputPort;
        ChiFeature2PortDescriptor           outputPort;

        result = GetOutputPortsForStage(0, &outputPortList);

        if ((NULL != pPrivContext) && (CDKResultSuccess == result))
        {
            std::vector<std::vector<ChiFeature2Identifier>> extPorts(pRequestObject->GetNumRequests());

            for (UINT8 numRequest = 0; numRequest < pRequestObject->GetNumRequests(); numRequest++)
            {
                extPorts[numRequest].reserve(outputPortList.numPorts);
            }

            for (UINT8 inputIdx = 0; inputIdx < pPrivContext->numInputDependency; inputIdx++)
            {
                inputPort       = pPrivContext->pPortMap[inputIdx].inputPort;
                outputPort      = pPrivContext->pPortMap[inputIdx].outputPort;
                requestId       = pPrivContext->pPortMap[inputIdx].requestId;
                dependencyIdx   = pPrivContext->pPortMap[inputIdx].dependencyIdx;

                pOutputPortData = GetPortData(&outputPort.globalId);
                if (NULL == pOutputPortData)
                {
                    result = CDKResultEInvalidArg;
                    break;
                }

                pFinalBufferMeta = NULL;
                if (ChiFeature2PortDirectionType::ExternalOutput == outputPort.globalId.portDirectionType)
                {
                    // if no buffer and metadata for this request
                    pRequestObject->GetFinalBufferMetadataInfo(outputPort.globalId,
                            &pFinalBufferMeta, requestId);
                    if (NULL == pFinalBufferMeta)
                    {
                        CHX_LOG_INFO("Skip output port requestId: %d, outputIdx: %d",
                                     requestId, outputPort.globalId.port);
                        continue;
                    }
                }

                if (inputPort.globalId.portType == ChiFeature2PortType::ImageBuffer)
                {
                    targetKey   = reinterpret_cast<UINT64>(pOutputPortData->pChiStream);
                    result      = GetInputFromDependency(pRequestObject,
                                                         0,
                                                         dependencyIdx,
                                                         &inputPort.globalId,
                                                         &streamBuffer);
                    if (CDKResultSuccess == result)
                    {
                        pTarget = &streamBuffer;
                    }
                    else
                    {
                        CHX_LOG_ERROR("GetInputFromDependency failed: inputIdx:%d, requestId:%d, dependencyIdx:%d",
                            inputIdx, 0, dependencyIdx);
                        break;
                    }
                }
                else if (inputPort.globalId.portType == ChiFeature2PortType::MetaData)
                {
                    targetKey   = pOutputPortData->metadataClientId;
                    result      = GetInputFromDependency(pRequestObject,
                                                         0,
                                                         dependencyIdx,
                                                         &inputPort.globalId,
                                                         &hMetaHandle);
                    if (CDKResultSuccess == result)
                    {
                        pTarget = GetMetadataManager()->GetMetadataFromHandle(hMetaHandle);
                    }
                    else
                    {
                        CHX_LOG_ERROR("GetInputFromDependency failed: inputIdx:%d, requestId:%d, dependencyIdx:%d",
                            inputIdx, 0, dependencyIdx);
                        break;
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Invalid PortType!");
                    result = CDKResultEFailed;
                }

                pOutputTBM = pOutputPortData->pOutputBufferTbm;
                if (NULL == pOutputTBM)
                {
                    CHX_LOG_ERROR("GetOutputTBM failed! outputIdx:%d", inputIdx);
                    result = CDKResultEFailed;
                    break;
                }

                if (CDKResultSuccess == result)
                {
                    VOID* pSrcBufferTarget = NULL;
                    // Import input port buffer to output port Tbm
                    pOutputTBM->ImportExternalTargetBuffer(
                        GetFrameNumber(pRequestObject, requestId),
                        targetKey,
                        pTarget);

                    // Setup Output buffer Tbm
                    pOutputTBM->SetupTargetBuffer(
                        GetFrameNumber(pRequestObject, requestId));

                    if (outputPort.globalId.portType == ChiFeature2PortType::ImageBuffer)
                    {
                        pSrcBufferTarget = streamBuffer.bufferInfo.phBuffer;
                    }
                    else
                    {
                        pSrcBufferTarget = pTarget;
                    }

                    // Update final output
                    if (ChiFeature2PortDirectionType::ExternalOutput == outputPort.globalId.portDirectionType)
                    {
                        CHITARGETBUFFERINFOHANDLE hBuffer;

                        result = pOutputTBM->UpdateTarget(GetFrameNumber(pRequestObject, requestId),
                                                 targetKey, pSrcBufferTarget, ChiTargetStatus::Ready,
                                                 &hBuffer);

                        if (CDKResultSuccess == result)
                        {
                            pFinalBufferMeta->key     = targetKey;
                            pFinalBufferMeta->hBuffer = hBuffer;
                            extPorts[requestId].push_back(outputPort.globalId);
                        }
                        else
                        {
                            CHX_LOG_INFO("Failed to get Buffer of Port = %d", outputPort.globalId.port);
                        }
                    }
                    else
                    {
                        pOutputTBM->UpdateTarget(GetFrameNumber(pRequestObject, requestId),
                                                 targetKey, pSrcBufferTarget,
                                                 ChiTargetStatus::Ready, NULL);
                    }

                }
            }

            if (CDKResultSuccess == result)
            {
                for (UINT8 numRequest = 0; numRequest < pRequestObject->GetNumRequests(); numRequest++)
                {
                    CHX_LOG_INFO("Send buffer and metadata message:requestId:%d,", numRequest);
                    // Send buffer result message
                    ChiFeature2MessageDescriptor bufferResultMsg;
                    ChxUtils::Memset(&bufferResultMsg, 0, sizeof(bufferResultMsg));
                    bufferResultMsg.messageType                = ChiFeature2MessageType::ResultNotification;
                    bufferResultMsg.message.result.numPorts    = extPorts[numRequest].size();
                    bufferResultMsg.message.result.pPorts      = extPorts[numRequest].data();
                    bufferResultMsg.message.result.resultIndex = numRequest;
                    bufferResultMsg.pPrivData                  = pRequestObject;
                    ProcessFeatureMessage(&bufferResultMsg);

                    // Set output notification as this index of request is done
                    pRequestObject->SetCurRequestState(ChiFeature2RequestState::OutputNotificationPending,
                        numRequest);
                }

            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::OnExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Serializer::OnExecuteProcessRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult result            = CDKResultSuccess;
    UINT8     stageId           = InvalidStageId;
    UINT8     nextStageId       = InvalidStageId;
    UINT8     stageSequenceId   = InvalidStageSequenceId;

    const ChiFeature2StageDescriptor*   pStageDescriptor    = NULL;
    ChiFeature2StageInfo                stageInfo           = { 0 };
    ChiFeature2PortIdList               outputList          = { 0 };
    ChiFeature2PortIdList               inputList           = { 0 };
    UINT8                               numDependencyLists  = 0;
    BOOL                                postDependency      = FALSE;
    UINT8                               curRequestId        = 0;

    result = GetCurrentStageInfo(pRequestObject, &stageInfo);

    if (CDKResultSuccess == result)
    {
        stageId         = stageInfo.stageId;
        stageSequenceId = stageInfo.stageSequenceId;
        nextStageId     = stageId + 1;
        curRequestId    = pRequestObject->GetCurRequestId();
        switch(stageId)
        {
            case InvalidStageId:
            {
                // Setting this tells the request object how many overall sequences we will run
                SetConfigInfo(pRequestObject, 2);
                if (curRequestId == 0)
                {
                    postDependency = TRUE;
                }
                break;
            }
            case OutputMappingStage:
            {
                if (curRequestId == 0)
                {
                    result = HandleOutputMapping(pRequestObject);
                }
                break;
            }
            default:
            {
                CHX_LOG_ERROR("Invalid stage ID!");
                break;
            }
        }

        if (nextStageId < GetNumStages())
        {
            pStageDescriptor = GetStageDescriptor(nextStageId);

            if (NULL != pStageDescriptor)
            {
                if (TRUE == postDependency)
                {
                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, 2);
                    HandleDependencyMapping(pRequestObject);
                }
                else
                {
                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, 0);
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Serializer::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pRequestObject)
    {
        ChiFeature2SerializerPrivateInfo* pPrivContext = static_cast<ChiFeature2SerializerPrivateInfo*>(
                                                            GetFeaturePrivContext(pRequestObject));

        if (NULL != pPrivContext)
        {
            ChiFeature2MessageDescriptor    releaseInputMsg;

            releaseInputMsg.messageType                                     = ChiFeature2MessageType::ReleaseInputDependency;
            releaseInputMsg.message.releaseDependencyData.batchIndex        = 0;
            releaseInputMsg.message.releaseDependencyData.numDependencies   = pPrivContext->numInputDependency;
            releaseInputMsg.message.releaseDependencyData.pDependencies     = static_cast<ChiFeature2Dependency*>(
                                                                                ChxUtils::Calloc(sizeof(ChiFeature2Dependency) *
                                                                                    pPrivContext->numInputDependency));

            if (NULL != releaseInputMsg.message.releaseDependencyData.pDependencies)
            {
                for (UINT8 inputIdx = 0 ; inputIdx < pPrivContext->numInputDependency; ++inputIdx)
                {
                    releaseInputMsg.message.releaseDependencyData.pDependencies[inputIdx].dependencyIndex =
                        pPrivContext->pPortMap[inputIdx].dependencyIdx;
                    releaseInputMsg.message.releaseDependencyData.pDependencies[inputIdx].numPorts        = 1;
                    releaseInputMsg.message.releaseDependencyData.pDependencies[inputIdx].pPorts          =
                        &pPrivContext->pPortMap[inputIdx].inputPort.globalId;
                }

                releaseInputMsg.pPrivData = pRequestObject;
                ProcessFeatureMessage(&releaseInputMsg);
            }
            else
            {
                CHX_LOG_INFO("Calloc releaseInputMsg.message.releaseDependencyData.pDependencies failed!");
                result = CDKResultENoMemory;
            }

            if (NULL != releaseInputMsg.message.releaseDependencyData.pDependencies)
            {
                ChxUtils::Free(releaseInputMsg.message.releaseDependencyData.pDependencies);
                releaseInputMsg.message.releaseDependencyData.pDependencies = NULL;
            }

            CHX_FREE(pPrivContext);
            pPrivContext = NULL;
            SetFeaturePrivContext(pRequestObject, pPrivContext);
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Arg! pRequestObject == NULL!");
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Serializer::DoFlush()
{
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2Serializer::CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2Serializer* pFeatureSerializer = ChiFeature2Serializer::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeatureSerializer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::DoQueryCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Serializer::DoQueryCaps(
    VOID*                 pConfig,
    ChiFeature2QueryInfo* pQueryInfo)
{
    CDK_UNUSED_PARAM(pConfig);
    CDKResult result = CDKResultSuccess;

    if (NULL != pQueryInfo)
    {
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2SerializerCaps);
        pQueryInfo->ppCapabilities = &Feature2SerializerCaps[0];
    }
    else
    {
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::GetVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Serializer::GetVendorTags(
    VOID* pVendorTags)
{
    CDK_UNUSED_PARAM(pVendorTags);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Serializer::DoStreamNegotiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Serializer::DoStreamNegotiation(
    StreamNegotiationInfo*      pNegotiationInfo,
    StreamNegotiationOutput*    pNegotiationOutput)
{
    CDKResult   result  = CDKResultSuccess;
    UINT32      width   = 0;
    UINT32      height  = 0;

    if ((NULL == pNegotiationInfo) || (pNegotiationOutput == NULL))
    {
        CHX_LOG_ERROR("Invalid Arg! pNegotiation=%p, pDesiredStreamConfig=%p",
            pNegotiationInfo, pNegotiationOutput);

        result = CDKResultEInvalidArg;
    }
    else
    {
        // Clone a stream and put it into the list of streams that will be freed by the feature
        auto CloneStream = [&pNegotiationOutput](CHISTREAM* pSrcCameraStream) -> CHISTREAM * {
            ChiStream* pStream = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
            ChxUtils::Memcpy(pStream, pSrcCameraStream, sizeof(CHISTREAM));
            pNegotiationOutput->pOwnedStreams->push_back(pStream);
            return pStream;
        };

        UINT32      physicalCamIdx  = pNegotiationInfo->pFeatureInstanceId->cameraId;
        UINT32      physicalCamId   = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[physicalCamIdx]->cameraId;
        CHISTREAM*  pInputStream    = NULL;
        CHISTREAM*  pOutputStream   = NULL;

        pNegotiationOutput->pStreams->clear();

        GetSensorOutputDimension(physicalCamId,
                                 pNegotiationInfo->pFwkStreamConfig,
                                 pNegotiationInfo->pFeatureInstanceId->flags,
                                 &width,
                                 &height);

        // Consider instance based stream selection untill interfeature negotiation is established.
        // RDI for stream configuration.
        pInputStream  = CloneStream(&HDRT1StreamsInput1);
        // Configure input stream
        if (NULL != pInputStream)
        {
            // configure input stream
            pInputStream->width  = width;
            pInputStream->height = height;
            pNegotiationOutput->pStreams->push_back(pInputStream);
        }
        else
        {
            CHX_LOG_ERROR("Clone input stream failed!");
            result = CDKResultENoMemory;
        }

        pOutputStream = CloneStream(&HDRT1StreamsInput1);

        if (NULL != pOutputStream)
        {
            // configure output stream
            pOutputStream->width        = width;
            pOutputStream->height       = height;
            pOutputStream->streamType   = ChiStreamTypeOutput;
            pNegotiationOutput->pStreams->push_back(pOutputStream);
        }
        else
        {
            CHX_LOG_ERROR("Clone output stream failed!");
            result = CDKResultENoMemory;
        }

        // configure desired stream
        pNegotiationOutput->pDesiredStreamConfig->numStreams       = pNegotiationOutput->pStreams->size();
        pNegotiationOutput->pDesiredStreamConfig->operationMode    =
            pNegotiationInfo->pFwkStreamConfig->operationMode;
        pNegotiationOutput->pDesiredStreamConfig->pChiStreams      = &(*(pNegotiationOutput->pStreams))[0];
        pNegotiationOutput->pDesiredStreamConfig->pSessionSettings = NULL;

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
        pChiFeature2Ops->size                   = sizeof(CHIFEATURE2OPS);
        pChiFeature2Ops->majorVersion           = Feature2SerializerMajorVersion;
        pChiFeature2Ops->minorVersion           = Feature2SerializerMinorVersion;
        pChiFeature2Ops->pVendorName            = VendorName;
        pChiFeature2Ops->pCreate                = ChiFeature2Serializer::CreateFeature;
        pChiFeature2Ops->pQueryCaps             = ChiFeature2Serializer::DoQueryCaps;
        pChiFeature2Ops->pGetVendorTags         = ChiFeature2Serializer::GetVendorTags;
        pChiFeature2Ops->pStreamNegotiation     = ChiFeature2Serializer::DoStreamNegotiation;
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus
