////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2demux.cpp
/// @brief Demux feature implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2demux.h"
#include "chifeature2base.h"
#include "chifeature2featurepool.h"
#include "chifeature2utils.h"

// NOWHINE FILE CP006:  Used standard libraries for performance improvements

static const UINT32 Feature2DemuxMajorVersion = 0;
static const UINT32 Feature2DemuxMinorVersion = 0;
static const CHAR*  VendorName                = "QTI";
static const CHAR*  Feature2DemuxCaps[]       =
{
    "Demux",
};

static const UINT8 BuildAndSubmitStage       = 0;
static const UINT8 RDIPortId                 = 0;
static const UINT8 MetadataPortId            = 1;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Demux::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Demux * ChiFeature2Demux::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    CDKResult         result   = CDKResultSuccess;
    ChiFeature2Demux* pFeature = NULL;

    if (NULL != pCreateInputInfo)
    {
        pFeature = CHX_NEW(ChiFeature2Demux);

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
// ChiFeature2Demux::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Demux::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Demux::OnInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Demux::OnInitialize(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    CDKResult                    result           = CDKResultSuccess;
    const ChiFeature2Descriptor* pFeature2Desc    = pCreateInputInfo->pFeatureDescriptor;
    ChiMetadataManager*          pMetadataManager = pCreateInputInfo->pMetadataManager;
    if (NULL != pFeature2Desc)
    {
        ChiFeature2Identifier globalId;
        for (UINT8 sessionIdx = 0; sessionIdx < pFeature2Desc->numSessions ; ++sessionIdx)
        {
            const ChiFeature2SessionDescriptor* pSessionDesc = &(pFeature2Desc->pSession[sessionIdx]);
            globalId.session                                 = pSessionDesc->sessionId;

            for (UINT8 pipelineIdx = 0; pipelineIdx < pSessionDesc->numPipelines; ++pipelineIdx)
            {
                const ChiFeature2PipelineDescriptor* pPipelineDesc =
                    &(pFeature2Desc->pSession[sessionIdx].pPipeline[pipelineIdx]);

                globalId.pipeline                                  = pPipelineDesc->pipelineId;
                ChiFeaturePipelineData* pPipelineData              = GetPipelineData(&globalId);

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
// ChiFeature2Demux::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Demux::OnPrepareRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;
    if (NULL != pRequestObject)
    {
        ChiFeature2DemuxPrivateInfo* pPrivContext =
            static_cast<ChiFeature2DemuxPrivateInfo*>(GetFeaturePrivContext(pRequestObject));
        if (NULL == pPrivContext)
        {
            pPrivContext = CHX_NEW ChiFeature2DemuxPrivateInfo;
            if (NULL != pPrivContext)
            {
                pPrivContext->numInputDependency = pRequestObject->GetNumRequests();
                pPrivContext->numTotalDependency = 0;
                SetFeaturePrivContext(pRequestObject, pPrivContext);
            }
            else
            {
                CHX_LOG_ERROR("Out of memory! Allocate pPrivContext failed!");
                result = CDKResultENoMemory;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Demux::GetNumOfInputDependencys
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChiFeature2Demux::GetNumOfInputDependency(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    UINT32                         numOfInputRequired = 0;
    ChiFeature2PortIdList          outputPortList     = { 0 };
    CDKResult                      result             = CDKResultSuccess;
    ChiFeature2Identifier          outputPortId       = { 0 };
    ChiFeaturePortData*            pOutputPortData    = NULL;
    ChiFeature2BufferMetadataInfo* pFinalBufferMeta   = NULL;
    const ChiFeature2RequestOutputInfo* pRequestOutputInfo = NULL;

    result = GetOutputPortsForStage(0, &outputPortList);

    if (CDKResultSuccess == result)
    {
        ChiFeature2DemuxPrivateInfo* pPrivContext =
            static_cast<ChiFeature2DemuxPrivateInfo*>(GetFeaturePrivContext(pRequestObject));
        if (NULL != pPrivContext)
        {
            // There are 2 output ports(buffer and metadata) for every downstream feature
            for (UINT8 outportIdx = 0; outportIdx < outputPortList.numPorts; outportIdx += 2)
            {
                for (UINT8 requestId = 0 ; requestId < pRequestObject->GetNumRequests(); ++requestId)
                {
                    for (UINT8 bufferMetadataIdx = 0; bufferMetadataIdx < 2; ++bufferMetadataIdx)
                    {
                        UINT8 outportPortIdx = outportIdx + bufferMetadataIdx;

                        outputPortId        = outputPortList.pPorts[outportPortIdx];
                        pFinalBufferMeta    = NULL;
                        if ((ChiFeature2PortDirectionType::ExternalOutput == outputPortId.portDirectionType) &&
                            (outputPortId.portType == ChiFeature2PortType::ImageBuffer))
                        {
                            // if no buffer and metadata for this request
                            pRequestObject->GetFinalBufferMetadataInfo(outputPortId,
                                    &pFinalBufferMeta, requestId);

                            if (NULL != pFinalBufferMeta)
                            {
                                CHX_LOG_INFO("Store Buffer port: %d requestId: %d to dependency index map:%d",
                                    outportPortIdx, requestId, pPrivContext->numTotalDependency);
                                numOfInputRequired++;
                                pPrivContext->portMap.push_back(outportPortIdx);
                                pPrivContext->requestIdMap.push_back(requestId);
                                pPrivContext->numTotalDependency++;
                            }
                        }

                        // Get input setting from downstream feature
                        if ((ChiFeature2PortDirectionType::ExternalOutput == outputPortId.portDirectionType) &&
                            (outputPortId.portType == ChiFeature2PortType::MetaData))
                        {
                            // if no buffer and metadata for this request
                            pRequestObject->GetFinalBufferMetadataInfo(outputPortId,
                                    &pFinalBufferMeta, requestId);

                            if (NULL != pFinalBufferMeta)
                            {
                                CHX_LOG_INFO("Store setting port: %d requestId: %d to dependency index map:%d",
                                    outportPortIdx, requestId, pPrivContext->numTotalDependency);
                                pPrivContext->settingMap.push_back(
                                    GetExternalInputSettings(pRequestObject, &outputPortId, requestId));
                                pPrivContext->portMap.push_back(outportPortIdx);
                                pPrivContext->requestIdMap.push_back(requestId);
                                pPrivContext->numTotalDependency++;
                            }
                        }
                    }
                }
            }

            pPrivContext->numInputDependency = numOfInputRequired;
        }
    }

    return numOfInputRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Demux::OnPopulateDependencySettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Demux::OnPopulateDependencySettings(
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

    CDKResult       result              = CDKResultSuccess;
    ChiMetadata*    pSrcFeatureSetting  = NULL;
    if (NULL == pFeatureSettings || NULL == pRequestObject)
    {
        CHX_LOG_ERROR("pFeatureSettings=%p, pRequestObject=%p",
            pFeatureSettings, pRequestObject);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DemuxPrivateInfo* pPrivContext =
            static_cast<ChiFeature2DemuxPrivateInfo*>(GetFeaturePrivContext(pRequestObject));
        if (NULL != pPrivContext)
        {
            pSrcFeatureSetting = static_cast<ChiMetadata*>(pPrivContext->settingMap[dependencyIndex]);

            if (NULL != pSrcFeatureSetting)
            {
                result = pFeatureSettings->Copy(*pSrcFeatureSetting, true);

                VOID* pEV = pSrcFeatureSetting->GetTag(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION);
                if (NULL != pEV)
                {
                    CHX_LOG_INFO("Populate dependency setting:%d, EV:%d, psrcSetting:%p",
                        dependencyIndex, *(static_cast<UINT32*>(pEV)),
                        pSrcFeatureSetting);
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Demux::GetOutputBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Demux::GetOutputBuffer(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId,
    ChiFeature2Identifier     outputId,
    VOID*                     pTarget
    ) const
{
    CDKResult                       result              = CDKResultEFailed;
    ChiFeature2PortIdList           inputPortList       = { 0 };
    ChiFeature2BufferMetadataInfo   inputBufferMetaInfo = { 0 };
    result = GetInputPortsForStage(0, &inputPortList);
    if (CDKResultSuccess == result)
    {
        ChiFeature2DemuxPrivateInfo* pPrivContext =
            static_cast<ChiFeature2DemuxPrivateInfo*>(GetFeaturePrivContext(pRequestObject));
        if (NULL != pPrivContext)
        {
            for (UINT8 dependencyIdx = 0 ; dependencyIdx < pPrivContext->numTotalDependency; dependencyIdx++)
            {
                if ((pPrivContext->portMap[dependencyIdx] == outputId.port) &&
                    (pPrivContext->requestIdMap[dependencyIdx] == requestId))
                {
                    if (outputId.portType == ChiFeature2PortType::ImageBuffer)
                    {
                        pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                            &(inputPortList.pPorts[RDIPortId]), &inputBufferMetaInfo.hBuffer,
                            &inputBufferMetaInfo.key, 0, (dependencyIdx >> 1));

                        CHITargetBufferManager* pTargetBufferManager =
                            CHITargetBufferManager::GetTargetBufferManager(inputBufferMetaInfo.hBuffer);

                        result = GetStreamBuffer(inputBufferMetaInfo.hBuffer, inputBufferMetaInfo.key,
                            static_cast<CHISTREAMBUFFER*>(pTarget));

                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("dependencyIdx:%d GetStreamBuffer failed! hBuffer=%p",
                                dependencyIdx,
                                inputBufferMetaInfo.hBuffer);
                        }

                    }
                    else if (outputId.portType == ChiFeature2PortType::MetaData)
                    {
                        pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                            &(inputPortList.pPorts[MetadataPortId]), &inputBufferMetaInfo.hBuffer,
                            &inputBufferMetaInfo.key, 0, (dependencyIdx >> 1));

                        CHITargetBufferManager* pTargetBufferManager =
                            CHITargetBufferManager::GetTargetBufferManager(inputBufferMetaInfo.hBuffer);
                        result = GetMetadataBuffer(inputBufferMetaInfo.hBuffer, inputBufferMetaInfo.key,
                            static_cast<CHIMETAHANDLE*>(pTarget));

                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("dependencyIdx:%d GetMetadataBuffer failed! hBuffer=%p",
                                dependencyIdx,
                                inputBufferMetaInfo.hBuffer);
                        }
                    }
                    break;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Demux::BuildAndSubmitOutputInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Demux::BuildAndSubmitOutputInfo(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;
    if (NULL != pRequestObject)
    {
        ChiFeature2PortIdList          outputPortList    = { 0 };
        ChiFeature2Identifier          outputPortId      = { 0 };
        ChiFeature2BufferMetadataInfo* pFinalBufferMeta  = NULL;
        ChiFeaturePortData*            pOutputPortData   = NULL;
        CHITargetBufferManager*        pOutputTBM        = NULL;
        UINT64                         targetKey         = 0xFFFFFFFF;
        VOID*                          pTarget           = NULL;
        CHISTREAMBUFFER                streamBuffer      = { 0 };
        CHIMETAHANDLE                  hMetaHandle       = NULL;
        result = GetOutputPortsForStage(0, &outputPortList);
        if (CDKResultSuccess == result)
        {
            for (UINT8 requestId = 0; requestId < pRequestObject->GetNumRequests(); requestId++)
            {
                std::vector<ChiFeature2Identifier> extPorts;
                extPorts.reserve(outputPortList.numPorts);
                for (UINT8 outputIdx = 0; outputIdx < outputPortList.numPorts; outputIdx++)
                {
                    outputPortId         = outputPortList.pPorts[outputIdx];
                    pOutputPortData      = GetPortData(&outputPortId);
                    if (NULL == pOutputPortData)
                    {
                        CHX_LOG_ERROR("pOutputPortData GID[Session:%d Pipeline:%d Port:%d Type:%d] is NULL",
                                      outputPortId.session, outputPortId.pipeline, outputPortId.port, outputPortId.portType);
                        result = CDKResultEInvalidArg;
                        break;
                    }

                    pFinalBufferMeta = NULL;
                    if (ChiFeature2PortDirectionType::ExternalOutput == outputPortId.portDirectionType)
                    {
                        // if no buffer and metadata for this request
                        pRequestObject->GetFinalBufferMetadataInfo(outputPortId,
                                &pFinalBufferMeta, requestId);
                        if (NULL == pFinalBufferMeta)
                        {
                            CHX_LOG_INFO("Skip output port requestId: %d, outputIdx: %d",
                                         requestId, outputIdx);
                            continue;
                        }
                    }

                    pOutputTBM = pOutputPortData->pOutputBufferTbm;
                    if (NULL != pOutputTBM)
                    {
                        if (outputPortId.portType == ChiFeature2PortType::ImageBuffer)
                        {
                            targetKey = reinterpret_cast<UINT64>(pOutputPortData->pChiStream);
                            result    = GetOutputBuffer(pRequestObject, requestId, outputPortId, &streamBuffer);
                            if (CDKResultSuccess == result)
                            {
                                pTarget = &streamBuffer;
                            }
                            else
                            {
                                CHX_LOG_ERROR("GetOutputBuffer failed: request id:%d, outputid:%d",
                                    requestId, outputIdx);
                                break;
                            }
                        }
                        else if (outputPortId.portType == ChiFeature2PortType::MetaData)
                        {
                            targetKey = pOutputPortData->metadataClientId;
                            result    = GetOutputBuffer(pRequestObject, requestId, outputPortId, &hMetaHandle);
                            if (CDKResultSuccess == result)
                            {
                                pTarget = GetMetadataManager()->GetMetadataFromHandle(hMetaHandle);
                            }
                            else
                            {
                                CHX_LOG_ERROR("GetOutputBuffer failed: request id:%d, outputid:%d",
                                    requestId, outputIdx);
                                break;
                            }
                        }
                        else
                        {
                            CHX_LOG_ERROR("Invalid PortType!");
                            result = CDKResultEFailed;
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

                            if (outputPortId.portType == ChiFeature2PortType::ImageBuffer)
                            {
                                pSrcBufferTarget = streamBuffer.bufferInfo.phBuffer;
                            }
                            else
                            {
                                pSrcBufferTarget = pTarget;
                            }

                            // Update final output
                            if (ChiFeature2PortDirectionType::ExternalOutput == outputPortId.portDirectionType)
                            {
                                CHITARGETBUFFERINFOHANDLE   hBuffer;
                                // add reference for output port
                                result = pOutputTBM->UpdateTarget(GetFrameNumber(pRequestObject, requestId),
                                                         targetKey, pSrcBufferTarget, ChiTargetStatus::Ready,
                                                         &hBuffer);

                                if (CDKResultSuccess == result)
                                {
                                    pFinalBufferMeta->key     = targetKey;
                                    pFinalBufferMeta->hBuffer = hBuffer;
                                    extPorts.push_back(outputPortId);
                                }
                                else
                                {
                                    CHX_LOG_INFO("Failed to get Buffer of Port = %d", outputPortId.port);
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
                    else
                    {
                        CHX_LOG_ERROR("GetOutputTBM failed! outputIdx:%d", outputIdx);
                        result = CDKResultEFailed;
                    }
                }

                if (CDKResultSuccess == result)
                {
                    CHX_LOG_INFO("%s Send buffer and metadata message:requestId:%d,",
                                 pRequestObject->IdentifierString(), requestId);

                    // Send buffer result message
                    ChiFeature2MessageDescriptor bufferResultMsg;
                    ChxUtils::Memset(&bufferResultMsg, 0, sizeof(bufferResultMsg));
                    bufferResultMsg.messageType                = ChiFeature2MessageType::ResultNotification;
                    bufferResultMsg.message.result.numPorts    = extPorts.size();
                    bufferResultMsg.message.result.pPorts      = extPorts.data();
                    bufferResultMsg.message.result.resultIndex = requestId;
                    bufferResultMsg.pPrivData                  = pRequestObject;
                    ProcessFeatureMessage(&bufferResultMsg);

                    // Set output notification as this index of request is done
                    pRequestObject->SetCurRequestState(ChiFeature2RequestState::OutputNotificationPending,
                        requestId);
                }
                else
                {
                    CHX_LOG_ERROR("%s failed! result:%d", pRequestObject->IdentifierString(), result);
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Demux::OnExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Demux::OnExecuteProcessRequest(
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
        CHX_LOG_INFO("%s curRequestId:%d, stageId:%d, stageSequenceId:%d, nextStageId:%d",
                      pRequestObject->IdentifierString(), curRequestId, stageId, stageSequenceId, nextStageId);
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
            case BuildAndSubmitStage:
            {
                if (curRequestId == 0)
                {
                    result = BuildAndSubmitOutputInfo(pRequestObject);
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
                    numDependencyLists = GetNumOfInputDependency(pRequestObject);
                    CHX_LOG_INFO("%s numDependencyLists:%d", pRequestObject->IdentifierString(), numDependencyLists);
                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0,
                        numDependencyLists);
                    const ChiFeature2InputDependency* pInputDependency =
                        GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);
                    if (NULL != pInputDependency)
                    {
                        for (UINT8 bufferIndex = 0; bufferIndex < numDependencyLists; ++bufferIndex)
                        {
                            PopulateDependencyPorts(pRequestObject, bufferIndex, pInputDependency);
                        }
                    }
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
// ChiFeature2Demux::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Demux::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pRequestObject)
    {
        ChiFeature2DemuxPrivateInfo* pPrivContext =
            static_cast<ChiFeature2DemuxPrivateInfo*>(GetFeaturePrivContext(pRequestObject));
        if (NULL != pPrivContext)
        {
            ChiFeature2PortIdList inputPortList = { 0 };

            result = GetInputPortsForStage(BuildAndSubmitStage, &inputPortList);
            ChiFeature2Identifier* pPorts =
                static_cast<ChiFeature2Identifier*>(ChxUtils::Calloc(sizeof(ChiFeature2Identifier) * inputPortList.numPorts));

            if (NULL != pPorts)
            {
                UINT8 numActivePorts = 0;
                for (UINT8 inputIdx = 0; inputIdx < inputPortList.numPorts; ++inputIdx)
                {
                    pPorts[numActivePorts] = inputPortList.pPorts[inputIdx];
                    numActivePorts++;
                }

                // Release input dependency for upstream feature
                ChiFeature2MessageDescriptor releaseInputMsg;
                releaseInputMsg.messageType = ChiFeature2MessageType::ReleaseInputDependency;
                releaseInputMsg.message.releaseDependencyData.batchIndex      = 0;
                releaseInputMsg.message.releaseDependencyData.numDependencies = pPrivContext->numInputDependency;
                releaseInputMsg.message.releaseDependencyData.pDependencies   = static_cast<ChiFeature2Dependency*>(
                                                                    ChxUtils::Calloc(sizeof(ChiFeature2Dependency) *
                                                                    pPrivContext->numInputDependency));

                if (NULL != releaseInputMsg.message.releaseDependencyData.pDependencies)
                {
                    for (UINT8 dependencyIdx = 0 ; dependencyIdx < pPrivContext->numInputDependency; ++dependencyIdx)
                    {
                        releaseInputMsg.message.releaseDependencyData.pDependencies[dependencyIdx].dependencyIndex =
                            dependencyIdx;
                        releaseInputMsg.message.releaseDependencyData.pDependencies[dependencyIdx].numPorts        =
                            numActivePorts;
                        releaseInputMsg.message.releaseDependencyData.pDependencies[dependencyIdx].pPorts          =
                            pPorts;
                    }

                    releaseInputMsg.pPrivData = pRequestObject;
                    ProcessFeatureMessage(&releaseInputMsg);
                }
                else
                {
                    CHX_LOG_ERROR("Out of memory! Allocate pDependencies failed");
                    result = CDKResultENoMemory;
                }

                if (NULL != releaseInputMsg.message.releaseDependencyData.pDependencies)
                {
                    ChxUtils::Free(releaseInputMsg.message.releaseDependencyData.pDependencies);
                    releaseInputMsg.message.releaseDependencyData.pDependencies = NULL;
                }

                ChxUtils::Free(pPorts);
                pPorts = NULL;
            }
            else
            {
                CHX_LOG_ERROR("Out of memory! Allocate pPorts failed!");
                result = CDKResultENoMemory;
            }

            CHX_DELETE pPrivContext;
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
// ChiFeature2Demux::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Demux::DoFlush()
{
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2Demux* pFeatureDemux = ChiFeature2Demux::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeatureDemux);
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
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2DemuxCaps);
        pQueryInfo->ppCapabilities = &Feature2DemuxCaps[0];
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
    CDKResult result        = CDKResultSuccess;
    BOOL      isMultiCamera = (pNegotiationInfo->pLogicalCameraInfo->numPhysicalCameras <= 1) ? FALSE : TRUE;

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

        const UINT   numStreams   = pNegotiationInfo->pFwkStreamConfig->numStreams;
        CHISTREAM**  ppChiStreams = pNegotiationInfo->pFwkStreamConfig->pChiStreams;
        pNegotiationOutput->pStreams->clear();

        UINT32     physicalCamIdx   = pNegotiationInfo->pFeatureInstanceId->cameraId;
        UINT32     physicalCamId    = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[physicalCamIdx]->cameraId;
        CHISTREAM* pRDIInputStream  = NULL;
        CHISTREAM* pRDIOutputStream = NULL;

        GetSensorOutputDimension(physicalCamId,
                                pNegotiationInfo->pFwkStreamConfig,
                                pNegotiationInfo->pFeatureInstanceId->flags,
                                &RDIStream.width,
                                &RDIStream.height);

        // Configure input stream(RDI/FD stream)
        pRDIInputStream  = CloneStream(&Bayer2YuvStreamsInput);
        // Configure input RDI stream
        if (NULL != pRDIInputStream)
        {
            // configure input stream
            pRDIInputStream->width  = RDIStream.width;
            pRDIInputStream->height = RDIStream.height;
            pNegotiationOutput->pStreams->push_back(pRDIInputStream);
        }
        else
        {
            CHX_LOG_ERROR("Clone RDI stream failed!");
            result = CDKResultENoMemory;
        }

        pRDIOutputStream = CloneStream(&Bayer2YuvStreamsInput);

        if (NULL != pRDIOutputStream)
        {
            // configure output stream
            pRDIOutputStream->width  = RDIStream.width;
            pRDIOutputStream->height = RDIStream.height;
            pRDIOutputStream->streamType = ChiStreamTypeOutput;
            pNegotiationOutput->pStreams->push_back(pRDIOutputStream);
        }
        else
        {
            CHX_LOG_ERROR("Clone RDI output stream failed!");
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
        pChiFeature2Ops->majorVersion           = Feature2DemuxMajorVersion;
        pChiFeature2Ops->minorVersion           = Feature2DemuxMinorVersion;
        pChiFeature2Ops->pVendorName            = VendorName;
        pChiFeature2Ops->pCreate                = CreateFeature;
        pChiFeature2Ops->pQueryCaps             = DoQueryCaps;
        pChiFeature2Ops->pGetVendorTags         = GetVendorTags;
        pChiFeature2Ops->pStreamNegotiation     = DoStreamNegotiation;
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus
