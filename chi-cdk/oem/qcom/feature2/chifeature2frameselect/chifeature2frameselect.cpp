////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2frameselect.cpp
/// @brief FrameSelect feature implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2frameselect.h"
#include "chifeature2featurepool.h"
#include "chifeature2utils.h"

// NOWHINE FILE CP006:  Used standard libraries for performance improvements

static const UINT32 Feature2FrameSelectMajorVersion = 0;
static const UINT32 Feature2FrameSelectMinorVersion = 0;
static const CHAR*  VendorName                      = "QTI";
static const UINT8  FrameCollectionStage            = 0;
static const UINT8  InitialFROIndex                 = 0;
static const UINT8  RDIPortId                       = 0;
static const UINT8  MetadataPortId                  = 1;
static const CHAR*  Feature2FrameSelectCaps[]       =
{
    "FrameSelect",
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2FrameSelect * ChiFeature2FrameSelect::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{

    CDKResult               result   = CDKResultSuccess;
    ChiFeature2FrameSelect* pFeature = NULL;

    if (NULL != pCreateInputInfo)
    {
        pFeature = CHX_NEW(ChiFeature2FrameSelect);

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
// ChiFeature2FrameSelect::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2FrameSelect::Destroy()
{
    if (NULL != m_pFrameSelectThreadMutex)
    {
        m_pFrameSelectThreadMutex->Destroy();
        m_pFrameSelectThreadMutex = NULL;
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::OnInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::OnInitialize(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    CDKResult                    result           = CDKResultSuccess;
    const ChiFeature2Descriptor* pFeature2Desc    = pCreateInputInfo->pFeatureDescriptor;
    ChiMetadataManager*          pMetadataManager = pCreateInputInfo->pMetadataManager;
    if (NULL != pFeature2Desc)
    {
        ChiFeature2Identifier globalId;
        for (UINT8 sessionIdx = 0; sessionIdx < pFeature2Desc->numSessions; ++sessionIdx)
        {
            const ChiFeature2SessionDescriptor* pSessionDesc = &(pFeature2Desc->pSession[sessionIdx]);
            globalId.session = pSessionDesc->sessionId;

            for (UINT8 pipelineIdx = 0; pipelineIdx < pSessionDesc->numPipelines; ++pipelineIdx)
            {
                const ChiFeature2PipelineDescriptor* pPipelineDesc =
                    &(pFeature2Desc->pSession[sessionIdx].pPipeline[pipelineIdx]);

                globalId.pipeline                     = pPipelineDesc->pipelineId;
                ChiFeaturePipelineData* pPipelineData = GetPipelineData(&globalId);

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
                                  pPipelineData,
                                  pMetadataManager);
                    result = CDKResultEInvalidArg;
                    break;
                }

                for (UINT8 portIdx = 0; portIdx < pPipelineDesc->numInputPorts; ++portIdx)
                {
                    // only assign stream for buffer port
                    if (pPipelineDesc->pInputPortDescriptor[portIdx].globalId.portType == ChiFeature2PortType::ImageBuffer)
                    {
                        for (UINT8 streamIdx = 0; streamIdx < pCreateInputInfo->pStreamConfig->numStreams; ++streamIdx)
                        {
                            if (ChiStreamTypeInput == pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx]->streamType)
                            {
                                ChiFeature2Identifier key   = pPipelineDesc->pInputPortDescriptor[portIdx].globalId;
                                ChiFeaturePortData*   pPort = GetPortData(&key);
                                if (NULL != pPort)
                                {
                                    pPort->pChiStream = pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx];
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
                    for (UINT8 streamIdx = 0; streamIdx < pCreateInputInfo->pStreamConfig->numStreams; ++streamIdx)
                    {
                        // only assign stream for buffer port
                        if (pPipelineDesc->pOutputPortDescriptor[portIdx].globalId.portType == ChiFeature2PortType::ImageBuffer)
                        {
                            if (ChiStreamTypeOutput == pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx]->streamType)
                            {
                                ChiFeature2Identifier key   = pPipelineDesc->pOutputPortDescriptor[portIdx].globalId;
                                ChiFeaturePortData*   pPort = GetPortData(&key);
                                if (NULL != pPort)
                                {
                                    pPort->pChiStream = pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx];
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
            }
        }
        m_pFrameSelectThreadMutex = Mutex::Create();
    }
    else
    {
        CHX_LOG_ERROR("pFeature2Desc is NULL pointer");
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::OnPrepareRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;
    if (NULL != pRequestObject)
    {
        ChiFeature2StageInfo stageInfo = { 0 };
        result = GetCurrentStageInfo(pRequestObject, &stageInfo);


        m_pFrameSelectThreadMutex->Lock();
        VOID* pPrivateData = pRequestObject->GetInterFeatureRequestPrivateData(m_featureId,
                                                                               m_pCameraInfo->cameraId,
                                                                               m_pInstanceProps->instanceId);

        if (NULL == pPrivateData)
        {
            ChiFeature2FrameSelectInputInfo* pFrameSelectInputInfo =
                static_cast<ChiFeature2FrameSelectInputInfo*>(CHX_CALLOC(sizeof(ChiFeature2FrameSelectInputInfo)));

            if (NULL != pFrameSelectInputInfo)
            {
                // 2 input ports: RDI port and metadata port
                pFrameSelectInputInfo->maxInputPorts = MaxInputPortsPerCamera;
                for (UINT8 portIndex = 0; portIndex < pFrameSelectInputInfo->maxInputPorts; portIndex++)
                {
                    pFrameSelectInputInfo->inputBufferArray[portIndex].reserve(pRequestObject->GetFeatureHint()->numFrames);
                }

                ChiFeature2PortBufferInfo inputBufferInfo = {};
                ChxUtils::Memset(&inputBufferInfo, 0, sizeof(inputBufferInfo));

                for (UINT8 frameIndex = 0; frameIndex < pRequestObject->GetFeatureHint()->numFrames; frameIndex++)
                {
                    for (UINT8 portIndex = 0; portIndex < pFrameSelectInputInfo->maxInputPorts; portIndex++)
                    {
                        pFrameSelectInputInfo->inputBufferArray[portIndex].push_back(inputBufferInfo);
                        pFrameSelectInputInfo->inputBufferArray[portIndex][frameIndex].pRequestObject = pRequestObject;
                    }
                }

                pFrameSelectInputInfo->currentIndex      = 0;
                pFrameSelectInputInfo->numFramesSent     = 0;
                pFrameSelectInputInfo->releaseStartIndex = 0;
                pFrameSelectInputInfo->releaseEndIndex   = 0;
                pFrameSelectInputInfo->numRequests       = pRequestObject->GetNumRequests();
                pFrameSelectInputInfo->pInitialFRO       = pRequestObject;
                pFrameSelectInputInfo->pCurrentFRO       = pRequestObject;

                pRequestObject->SetInterFeatureRequestPrivateData(m_featureId,
                                                                  m_pCameraInfo->cameraId,
                                                                  m_pInstanceProps->instanceId,
                                                                  pFrameSelectInputInfo);
            }
            else
            {
                CHX_LOG_ERROR("NO memory to allocate for pFrameSelectInputInfo");
                result = CDKResultENoMemory;
            }
        }
        else
        {
            ChiFeature2FrameSelectInputInfo* pFrameSelectInputInfo =
                static_cast<ChiFeature2FrameSelectInputInfo*>(pRequestObject->
                    GetInterFeatureRequestPrivateData(m_featureId,
                                                      m_pCameraInfo->cameraId,
                                                      m_pInstanceProps->instanceId));

            if (NULL != pFrameSelectInputInfo)
            {
                if (pRequestObject != pFrameSelectInputInfo->pCurrentFRO)
                {
                    pFrameSelectInputInfo->numRequests += pRequestObject->GetNumRequests();
                    pFrameSelectInputInfo->pCurrentFRO  = pRequestObject;

                    UINT8 numFrames    = pRequestObject->GetFeatureHint()->numFrames;
                    UINT8 nextFROStart = pFrameSelectInputInfo->releaseStartIndex;
                    for (UINT8 portIndex = 0; portIndex < pFrameSelectInputInfo->maxInputPorts; portIndex++)
                    {
                        for (UINT8 queueIndex = nextFROStart; queueIndex < numFrames; queueIndex++)
                        {
                            pFrameSelectInputInfo->inputBufferArray[portIndex][queueIndex].pRequestObject = pRequestObject;
                        }
                    }
                }

                pRequestObject->SetInterFeatureRequestPrivateData(m_featureId,
                                                                  m_pCameraInfo->cameraId,
                                                                  m_pInstanceProps->instanceId,
                                                                  pFrameSelectInputInfo);
            }
        }
        m_pFrameSelectThreadMutex->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::HandleCollectInputInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::HandleCollectInputInfo(
    ChiFeature2RequestObject*           pRequestObject,
    ChiFeature2FrameSelectInputInfo*    pFrameSelectInputInfo,
    UINT8                               stageId
    ) const
{
    CDKResult             result         = CDKResultSuccess;
    UINT8                 curRequestId   = 0;
    ChiFeature2PortIdList inputPortList  = {0};
    UINT8                 hintFrames     = pRequestObject->GetFeatureHint()->numFrames;
    BOOL                  breakOuterLoop = FALSE;

    if ((NULL == pRequestObject) || (NULL == pFrameSelectInputInfo))
    {
        CHX_LOG_ERROR("Invalid Arg! pRequestObject=%p, pFrameSelectInputInfo=%p",
            pRequestObject, pFrameSelectInputInfo);
        result = CDKResultEInvalidArg;
    }
    else
    {
        curRequestId = pRequestObject->GetCurRequestId();

        for (UINT8 batchIdx = pFrameSelectInputInfo->currentIndex; batchIdx < hintFrames; batchIdx++)
        {
            // If we have already received frame info for this batchIdx, continue to next batchIdx to check for frame info
            if ((NULL != pFrameSelectInputInfo->
                    inputBufferArray[RDIPortId][batchIdx].Buffer.streamBuffer.pStream) &&
                (NULL != static_cast<VOID*>(pFrameSelectInputInfo->
                    inputBufferArray[MetadataPortId][batchIdx].Buffer.hMetaHandle)))
            {
                continue;
            }

            if (curRequestId < hintFrames)
            {
                result = GetInputPortsForStage(stageId, &inputPortList);
                ChiFeaturePortData*    pInputPortData = NULL;
                ChiFeature2Identifier* pPorts         = static_cast<ChiFeature2Identifier*>
                    (ChxUtils::Calloc(sizeof(ChiFeature2Identifier) * inputPortList.numPorts));
                if (NULL != pPorts)
                {
                    for (UINT8 inputIdx = 0; inputIdx < inputPortList.numPorts; ++inputIdx)
                    {
                        ChiFeature2BufferMetadataInfo  inputBufferMetaInfo = { 0 };
                        ChiFeature2PortBufferInfo      inputBufferInfo;

                        pPorts[inputIdx] = inputPortList.pPorts[inputIdx];
                        pInputPortData   = GetPortData(&(pPorts[inputIdx]));

                        if (NULL != pInputPortData)
                        {
                            pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                                                          &(pPorts[inputIdx]),
                                                          &inputBufferMetaInfo.hBuffer,
                                                          &inputBufferMetaInfo.key,
                                                          curRequestId,
                                                          batchIdx);

                            CHITargetBufferManager* pTargetBufferManager =
                                CHITargetBufferManager::GetTargetBufferManager(inputBufferMetaInfo.hBuffer);

                            if (NULL != pTargetBufferManager)
                            {

                                if (pPorts[inputIdx].portType == ChiFeature2PortType::ImageBuffer)
                                {
                                    inputBufferInfo.bufferType = ChiFeature2PortType::ImageBuffer;
                                    result                     = GetStreamBuffer(inputBufferMetaInfo.hBuffer,
                                                                                 inputBufferMetaInfo.key,
                                                                                 &(inputBufferInfo.Buffer.streamBuffer));

                                    if (CDKResultSuccess == result)
                                    {
                                        CHX_LOG_INFO("SetBufferInfo:%s, targetHandle:%p, bufferHandle:%p",
                                                      pInputPortData->pPortName,
                                                      inputBufferMetaInfo.hBuffer,
                                                      inputBufferInfo.Buffer.streamBuffer.bufferInfo.phBuffer);
                                    }
                                    else
                                    {
                                        CHX_LOG_INFO("GetStreamBuffer failed! hBuffer=%p", inputBufferMetaInfo.hBuffer);
                                    }
                                }
                                else
                                {
                                    inputBufferInfo.bufferType = ChiFeature2PortType::MetaData;
                                    result                     = GetMetadataBuffer(inputBufferMetaInfo.hBuffer,
                                                                                  inputBufferMetaInfo.key,
                                                                                  &(inputBufferInfo.Buffer.hMetaHandle));

                                    if (CDKResultSuccess == result)
                                    {
                                        CHX_LOG_INFO("SetBufferInfo:%s, targetHandle:%p, metaHandle:%p",
                                                     pInputPortData->pPortName,
                                                     inputBufferMetaInfo.hBuffer,
                                                     inputBufferInfo.Buffer.hMetaHandle);
                                    }
                                    else
                                    {
                                        CHX_LOG_ERROR("GetMetadataBuffer failed! hBuffer=%p", inputBufferMetaInfo.hBuffer);
                                    }
                                }

                                if (CDKResultSuccess == result)
                                {
                                    if (inputBufferInfo.bufferType == ChiFeature2PortType::ImageBuffer)
                                    {
                                        CHX_LOG_INFO("ImageBuffer: Receiving [%u][%u]", inputIdx, batchIdx);
                                        pFrameSelectInputInfo->inputBufferArray[inputIdx][batchIdx].bufferType =
                                            inputBufferInfo.bufferType;

                                        pFrameSelectInputInfo->inputBufferArray[inputIdx][batchIdx].Buffer     =
                                            inputBufferInfo.Buffer;
                                    }
                                    else if (inputBufferInfo.bufferType == ChiFeature2PortType::MetaData)
                                    {
                                        CHX_LOG_INFO("MetaData:    Receiving [%u][%u]", inputIdx, batchIdx);
                                        pFrameSelectInputInfo->inputBufferArray[inputIdx][batchIdx].bufferType =
                                            inputBufferInfo.bufferType;

                                        pFrameSelectInputInfo->inputBufferArray[inputIdx][batchIdx].Buffer     =
                                            inputBufferInfo.Buffer;
                                    }

                                    if ((NULL != pFrameSelectInputInfo->
                                        inputBufferArray[RDIPortId][batchIdx].Buffer.streamBuffer.pStream) &&
                                        (NULL != static_cast<VOID*>(pFrameSelectInputInfo->
                                            inputBufferArray[MetadataPortId][batchIdx].Buffer.hMetaHandle)) &&
                                        (FALSE == pFrameSelectInputInfo->inputBufferArray[RDIPortId][batchIdx].isValid))
                                    {
                                        pFrameSelectInputInfo->currentIndex = batchIdx;
                                        IsFrameValid(pRequestObject, pFrameSelectInputInfo);
                                    }
                                }
                            }
                            else
                            {
                                // If there is no buffer ready for this batchIdx, there will not be a buffer ready for the
                                // next batchIdx. Break from inner loop and set breakOuterLoop to return from function.
                                CHX_LOG_WARN("Invalid target handle! Next Frame Not Ready inputBufferMetaInfo.hBuffer:%p",
                                              inputBufferMetaInfo.hBuffer);
                                result         = CDKResultEInvalidArg;
                                breakOuterLoop = TRUE;
                                break;
                            }
                        }
                        else
                        {
                            CHX_LOG_ERROR("GetPortData failed! portKey[%d,%d,%d,%d,%d]",
                                          pPorts[inputIdx].session,
                                          pPorts[inputIdx].pipeline,
                                          pPorts[inputIdx].port,
                                          pPorts[inputIdx].portDirectionType,
                                          pPorts[inputIdx].portType);
                            result = CDKResultEFailed;
                        }
                    }
                    if ((TRUE == breakOuterLoop) ||
                        ((pFrameSelectInputInfo->validCount - pFrameSelectInputInfo->numFramesSent)
                            >= pRequestObject->GetNumRequests()))
                    {
                        break;
                    }
                }
                else
                {
                    CHX_LOG_ERROR("No memory! Calloc failed!");
                    result = CDKResultENoMemory;
                }
                if (NULL != pPorts)
                {
                    ChxUtils::Free(pPorts);
                }
            }
            else
            {
                CHX_LOG_ERROR("curRequestId=%d,NumOfRequest=%d", curRequestId, pRequestObject->GetNumRequests());
                result = CDKResultEInvalidArg;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::DoFrameSelect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::DoFrameSelect(
    ChiFeature2RequestObject*           pRequestObject,
    ChiFeature2FrameSelectInputInfo*    pFrameSelectInputInfo,
    ChiFeature2FrameSelectOutputInfo*   pFrameSelectOutputInfo
    ) const
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pFrameSelectInputInfo) || (NULL == pFrameSelectOutputInfo))
    {
        CHX_LOG_ERROR("Invalid Arg! pFrameSelectInputInfo=%p, pFrameSelectOutputInfo=%p",
            pFrameSelectInputInfo, pFrameSelectOutputInfo);
        result = CDKResultEInvalidArg;
    }
    else
    {
        UINT8 numRequests   = pFrameSelectInputInfo->numRequests +
            ((pFrameSelectInputInfo->currentIndex + 1) - pFrameSelectInputInfo->validCount);
        UINT8 numFramesSent = pFrameSelectInputInfo->numFramesSent +
            ((pFrameSelectInputInfo->currentIndex + 1) - pFrameSelectInputInfo->validCount);

        pFrameSelectOutputInfo->outputOrder.reserve(pRequestObject->GetNumRequests());

        for (UINT8 bufferIdx = numFramesSent; bufferIdx < numRequests; ++bufferIdx)
        {
            if (pFrameSelectInputInfo->inputBufferArray[RDIPortId][bufferIdx].isValid == TRUE)
            {
                pFrameSelectOutputInfo->outputOrder.push_back(bufferIdx);
                CHX_LOG_INFO("FrameSelectOutputOrder:%d", bufferIdx);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::IsFrameValid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::IsFrameValid(
    ChiFeature2RequestObject*           pRequestObject,
    ChiFeature2FrameSelectInputInfo*    pFrameSelectInputInfo
    ) const
{
    CDKResult result = CDKResultSuccess;
    CDK_UNUSED_PARAM(pRequestObject);

    UINT8 currentIndex = pFrameSelectInputInfo->currentIndex;
    BOOL isBufferReady = TRUE;

    for (UINT8 portIndex = 0; portIndex < pFrameSelectInputInfo->maxInputPorts; portIndex++)
    {
        if ((NULL == pFrameSelectInputInfo->inputBufferArray[RDIPortId][currentIndex].Buffer.streamBuffer.pStream) ||
            (NULL == static_cast<VOID*>(pFrameSelectInputInfo->
                inputBufferArray[MetadataPortId][currentIndex].Buffer.hMetaHandle)))
        {
            isBufferReady = FALSE;
        }

        if ((TRUE == isBufferReady) &&
            (pRequestObject->GetFeatureHint()->numFrames > currentIndex) &&
            (pFrameSelectInputInfo->inputBufferArray[portIndex][currentIndex].isValid != TRUE))
        {
            pFrameSelectInputInfo->inputBufferArray[portIndex][currentIndex].isValid = TRUE;
            if (RDIPortId == portIndex)
            {
                pFrameSelectInputInfo->validCount++;
            }
        }
    }

    // if not marked valid release!
    if (FALSE == pFrameSelectInputInfo->inputBufferArray[RDIPortId][currentIndex].isValid && TRUE == isBufferReady)
    {
        ReleaseInputDependencyResource(pFrameSelectInputInfo->pInitialFRO, currentIndex);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::BuildAndSubmitOutputInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::BuildAndSubmitOutputInfo(
    ChiFeature2RequestObject*         pRequestObject,
    ChiFeature2FrameSelectInputInfo*  pFrameSelectInputInfo,
    ChiFeature2FrameSelectOutputInfo* pFrameSelectOutputInfo,
    UINT8                             stageId
    ) const
{
    CDKResult result = CDKResultSuccess;
    if ((NULL == pRequestObject) || (NULL == pFrameSelectInputInfo) || (NULL == pFrameSelectOutputInfo))
    {
        CHX_LOG_ERROR("Invalid Arg! pRequestObject=%p, pFrameSelectInputInfo=%p, pFrameSelectOutputInfo=%p",
            pRequestObject, pFrameSelectInputInfo, pFrameSelectOutputInfo);
        result = CDKResultEInvalidArg;
    }
    else
    {
        ChiFeature2PortIdList          outputPortList   = { 0 };
        CHIMETADATAHANDLE              hMetaHandle      = { 0 };
        ChiFeature2BufferMetadataInfo* pFinalBufferMeta = NULL;
        ChiFeature2PortBufferInfo*     pInputBufferInfo = NULL;
        UINT8                          filteredIdx      = 0;
        ChiFeature2Identifier          outputPortId     = { 0 };
        ChiFeaturePortData*            pOutputPortData  = NULL;
        ChiFeaturePipelineData*        pPipelineData    = NULL;
        CHITargetBufferManager*        pOutputTBM       = NULL;
        UINT64                         targetKey        = 0xFFFFFFFF;
        VOID*                          pTarget          = NULL;

        result = GetOutputPortsForStage(stageId, &outputPortList);

        // need to handle outToInput map gracefully
        std::vector<UINT8> outToInputMap;

        /**************** inputPort to outputPort map ************/
        /*    inputPort(index)              outputPort(index)
        *     RDI_0  (0)                    RDI_0   (0)
        *     Meta_0 (1)                    Meta_0  (1)
        *     RDI_1  (2)                    RDI_1   (2)
        *     Meta_1 (3)                    Meta_1  (3)
        */
        outToInputMap.reserve(4);
        outToInputMap.push_back(0); // inputPort_RDI->outputPort_RDI
        outToInputMap.push_back(1); // inputPort_Meta->outputPort_meta
        outToInputMap.push_back(2);
        outToInputMap.push_back(3);

        if (CDKResultSuccess == result)
        {
            for (UINT8 requestId = 0; requestId < pRequestObject->GetNumRequests(); ++requestId)
            {
                filteredIdx = pFrameSelectOutputInfo->outputOrder.at(requestId);

                std::vector<ChiFeature2Identifier> extPorts;
                extPorts.reserve(outputPortList.numPorts);

                for (UINT8 outputIdx = 0; outputIdx < outputPortList.numPorts; ++outputIdx)
                {
                    outputPortId = outputPortList.pPorts[outputIdx];

                    // Get buffer and metadata by reorder index
                    pInputBufferInfo = &(pFrameSelectInputInfo->inputBufferArray[outToInputMap.at(outputIdx)].at(filteredIdx));

                    CHX_LOG_INFO("%s Input[%d,%d]->Output[%d,%d]", pRequestObject->IdentifierString(),
                        outToInputMap.at(outputIdx), filteredIdx, outputIdx, requestId);

                    if (outputPortId.portType == ChiFeature2PortType::ImageBuffer)
                    {
                        pOutputPortData = GetPortData(&outputPortId);

                        if (NULL != pOutputPortData)
                        {
                            pOutputTBM = pOutputPortData->pOutputBufferTbm;

                            targetKey = reinterpret_cast<UINT64>(pOutputPortData->pChiStream);

                            pTarget = &(pInputBufferInfo->Buffer.streamBuffer);
                        }
                        else
                        {
                            CHX_LOG_ERROR("GetPortData failed! portKey[%d,%d,%d,%d,%d]",
                                          outputPortId.session,
                                          outputPortId.pipeline,
                                          outputPortId.port,
                                          outputPortId.portDirectionType,
                                          outputPortId.portType);
                            result = CDKResultEFailed;
                        }
                    }
                    else if (outputPortId.portType == ChiFeature2PortType::MetaData)
                    {
                        pPipelineData = GetPipelineData(&outputPortId);

                        if (NULL != pPipelineData)
                        {
                            pOutputTBM = pPipelineData->pOutputMetaTbm;

                            targetKey  = pPipelineData->metadataClientId;

                            pTarget    = GetMetadataManager()->GetMetadataFromHandle(pInputBufferInfo->Buffer.hMetaHandle);
                        }
                        else
                        {
                            CHX_LOG_ERROR("GetPipelineData failed! pipelineKey[%d,%d]",
                                          outputPortId.session,
                                          outputPortId.pipeline);
                            result = CDKResultEFailed;
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("Invalid PortType!");
                        result = CDKResultEFailed;
                    }

                    if (CDKResultSuccess == result)
                    {
                        CHITARGETBUFFERINFOHANDLE* phBuffer = NULL;

                        // Import input port buffer to output port Tbm
                        pOutputTBM->ImportExternalTargetBuffer(GetFrameNumber(pRequestObject, requestId), targetKey, pTarget);

                        // Setup Output buffer Tbm
                        pOutputTBM->SetupTargetBuffer(GetFrameNumber(pRequestObject, requestId));

                        pFinalBufferMeta = NULL;
                        // Update final output
                        if (ChiFeature2PortDirectionType::ExternalOutput == outputPortId.portDirectionType)
                        {
                            pRequestObject->GetFinalBufferMetadataInfo(outputPortId, &pFinalBufferMeta, requestId);

                            if (NULL != pFinalBufferMeta)
                            {
                                pFinalBufferMeta->key     = targetKey;
                                phBuffer                  = &pFinalBufferMeta->hBuffer;

                                if (outputPortId.portType == ChiFeature2PortType::MetaData)
                                {
                                    GetMetadataBuffer(pFinalBufferMeta->hBuffer, pFinalBufferMeta->key, &hMetaHandle);
                                }
                            }

                            extPorts.push_back(outputPortId);
                        }

                        // Update target
                        if (outputPortId.portType == ChiFeature2PortType::ImageBuffer)
                        {
                            result = pOutputTBM->UpdateTarget(GetFrameNumber(pRequestObject, requestId),
                                                              targetKey,
                                                              pInputBufferInfo->Buffer.streamBuffer.bufferInfo.phBuffer,
                                                              ChiTargetStatus::Ready,
                                                              phBuffer);
                        }
                        else
                        {
                            result = pOutputTBM->UpdateTarget(GetFrameNumber(pRequestObject, requestId),
                                                              targetKey,
                                                              pTarget,
                                                              ChiTargetStatus::Ready,
                                                              phBuffer);
                        }

                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("Unable to get target buffer");
                        }
                    }
                }

                if (CDKResultSuccess == result)
                {
                    CHX_LOG_INFO("Send buffer and metadata message:requestId:%d,", requestId);

                    // Send metadata result message
                    ChiFeature2MessageDescriptor metadataResultMsg;
                    ChxUtils::Memset(&metadataResultMsg, 0, sizeof(metadataResultMsg));
                    metadataResultMsg.messageType                = ChiFeature2MessageType::MetadataNotification;
                    metadataResultMsg.message.result.numPorts    = extPorts.size();
                    metadataResultMsg.message.result.pPorts      = extPorts.data();
                    metadataResultMsg.message.result.resultIndex = requestId;
                    metadataResultMsg.pPrivData                  = pRequestObject;
                    ProcessFeatureMessage(&metadataResultMsg);

                    // Send buffer result message
                    ChiFeature2MessageDescriptor bufferResultMsg;
                    ChxUtils::Memset(&bufferResultMsg, 0, sizeof(bufferResultMsg));
                    bufferResultMsg.messageType                = ChiFeature2MessageType::ResultNotification;
                    bufferResultMsg.message.result.numPorts    = extPorts.size();
                    bufferResultMsg.message.result.pPorts      = extPorts.data();
                    bufferResultMsg.message.result.resultIndex = requestId;
                    bufferResultMsg.pPrivData                  = pRequestObject;
                    ProcessFeatureMessage(&bufferResultMsg);
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::OnExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::OnExecuteProcessRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult result          = CDKResultSuccess;
    UINT8     stageId         = InvalidStageId;
    UINT8     nextStageId     = InvalidStageId;
    UINT8     stageSequenceId = InvalidStageSequenceId;

    const ChiFeature2StageDescriptor*   pStageDescriptor   = NULL;
    ChiFeature2StageInfo                stageInfo          = { 0 };
    ChiFeature2PortIdList               outputList         = { 0 };
    ChiFeature2PortIdList               inputList          = { 0 };
    UINT8                               numDependencyLists = 0;
    UINT8                               curRequestId       = 0;
    UINT8                               hintNumframes      = pRequestObject->GetFeatureHint()->numFrames;

    result = GetCurrentStageInfo(pRequestObject, &stageInfo);

    if (CDKResultSuccess == result)
    {
        stageId         = stageInfo.stageId;
        stageSequenceId = stageInfo.stageSequenceId;
        nextStageId     = stageId + 1;
        curRequestId    = pRequestObject->GetCurRequestId();
        switch (stageId)
        {
            case InvalidStageId:
            {
                // Setting this tells the request object how many overall sequences we will run
                SetConfigInfo(pRequestObject, 2);
                break;
            }
            case FrameCollectionStage:
            {
                m_pFrameSelectThreadMutex->Lock();
                ChiFeature2FrameSelectInputInfo* pFrameSelectInputInfo =
                    static_cast<ChiFeature2FrameSelectInputInfo*>(pRequestObject->
                        GetInterFeatureRequestPrivateData(m_featureId,
                                                          m_pCameraInfo->cameraId,
                                                          m_pInstanceProps->instanceId));
                if (NULL != pFrameSelectInputInfo)
                {
                    UINT8 numRequests   = pRequestObject->GetNumRequests();
                    UINT8 validCount    = pFrameSelectInputInfo->validCount;
                    UINT8 numFramesSent = pFrameSelectInputInfo->numFramesSent;

                    if ((validCount - numFramesSent) >= (numRequests) &&
                        ((pRequestObject->GetCurRequestId()) == (numRequests - 1)))
                    {
                        ChiFeature2StageInfo currentStageInfo = { 0 };
                        result = GetCurrentStageInfo(pFrameSelectInputInfo->pInitialFRO, &currentStageInfo);
                        SendValidFramesDownstream(pFrameSelectInputInfo, currentStageInfo);
                    }

                    if (curRequestId == 0 && pRequestObject == pFrameSelectInputInfo->pInitialFRO)
                    {
                        pRequestObject->SetCurRequestState(ChiFeature2RequestState::OutputNotificationPending, curRequestId);

                        pRequestObject->SetCurRequestState(ChiFeature2RequestState::Complete, curRequestId);

                        if (FALSE == pRequestObject->GetOutputPortsProcessingCompleteNotified())
                        {
                            if (TRUE == pRequestObject->AllOutputPortsProcessingComplete())
                            {
                                UINT8 releaseStartIndex = pFrameSelectInputInfo->releaseStartIndex;
                                for (UINT8 batchIndex = releaseStartIndex; batchIndex < hintNumframes; batchIndex++)
                                {
                                    ReleaseInputDependencyResource(pFrameSelectInputInfo->pInitialFRO, batchIndex);
                                }

                                for (UINT8 i = 0; i < pFrameSelectInputInfo->maxInputPorts; i++)
                                {
                                    pFrameSelectInputInfo->inputBufferArray[i].clear();
                                    pFrameSelectInputInfo->inputBufferArray[i].shrink_to_fit();
                                }

                                VOID* pRequestContext = pRequestObject->GetPrivContext();
                                if (NULL != pRequestContext)
                                {
                                    CHX_FREE(pRequestContext);
                                    pRequestContext = NULL;
                                    pRequestObject->SetPrivContext(NULL);
                                }

                                ChiFeature2MessageDescriptor featurePortMessage = {};
                                featurePortMessage.messageType = ChiFeature2MessageType::ProcessingDependenciesComplete;
                                featurePortMessage.pPrivData   = pRequestObject;
                                ProcessFeatureMessage(&featurePortMessage);

                                pRequestObject->RemoveInterFeatureRequestPrivateData(m_featureId,
                                                                                     m_pCameraInfo->cameraId,
                                                                                     m_pInstanceProps->instanceId);

                                ChxUtils::Free(pFrameSelectInputInfo);
                                pFrameSelectInputInfo = NULL;
                            }
                        }
                    }
                }
                m_pFrameSelectThreadMutex->Unlock();

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
                m_pFrameSelectThreadMutex->Lock();
                ChiFeature2FrameSelectInputInfo* pFrameSelectInputInfo =
                    static_cast<ChiFeature2FrameSelectInputInfo*>(pRequestObject->
                        GetInterFeatureRequestPrivateData(m_featureId,
                                                          m_pCameraInfo->cameraId,
                                                          m_pInstanceProps->instanceId));

                if (NULL != pFrameSelectInputInfo && 0 == curRequestId && pFrameSelectInputInfo->pInitialFRO == pRequestObject)
                {
                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, hintNumframes);
                    const ChiFeature2InputDependency* pInputDependency =
                        GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);

                    if (NULL != pInputDependency)
                    {
                        for (UINT8 bufferIndex = 0; bufferIndex < hintNumframes; ++bufferIndex)
                        {
                            result = PopulateDependencyPorts(pRequestObject, bufferIndex, pInputDependency);
                        }
                    }
                }
                else
                {
                    SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, 0);
                }
                m_pFrameSelectThreadMutex->Unlock();
            }
        }
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::OnInputResourcePending
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::OnInputResourcePending(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult            result    = CDKResultSuccess;
    ChiFeature2StageInfo stageInfo = { 0 };

    result = GetCurrentStageInfo(pRequestObject, &stageInfo);

    m_pFrameSelectThreadMutex->Lock();
    ChiFeature2FrameSelectInputInfo* pFrameSelectInputInfo =
        static_cast<ChiFeature2FrameSelectInputInfo*>(pRequestObject->
            GetInterFeatureRequestPrivateData(m_featureId,
                                              m_pCameraInfo->cameraId,
                                              m_pInstanceProps->instanceId));

    if (NULL != pFrameSelectInputInfo)
    {
        HandleCollectInputInfo(pRequestObject, pFrameSelectInputInfo, stageInfo.stageId);

        ChiFeature2RequestObject* pMappedFRO =
            pFrameSelectInputInfo->inputBufferArray[RDIPortId][pFrameSelectInputInfo->currentIndex].pRequestObject;

        if ((pFrameSelectInputInfo->validCount - pFrameSelectInputInfo->numFramesSent) >= (pMappedFRO->GetNumRequests()))
        {
            SendValidFramesDownstream(pFrameSelectInputInfo, stageInfo);
        }
    }
    m_pFrameSelectThreadMutex->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::SendValidFramesDownstream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::SendValidFramesDownstream(
    ChiFeature2FrameSelectInputInfo*    pFrameSelectInputInfo,
    ChiFeature2StageInfo                stageInfo
    ) const
{
    CDKResult                 result     = CDKResultSuccess;
    ChiFeature2RequestObject* pMappedFRO =
        pFrameSelectInputInfo->inputBufferArray[RDIPortId][pFrameSelectInputInfo->currentIndex].pRequestObject;

    if ((NULL != pMappedFRO) &&
        (pFrameSelectInputInfo->numFramesSent < pFrameSelectInputInfo->numRequests))
    {
        ChiFeature2FrameSelectOutputInfo frameSelectOutputInfo;
        DoFrameSelect(pMappedFRO, pFrameSelectInputInfo, &frameSelectOutputInfo);
        BuildAndSubmitOutputInfo(pMappedFRO,
                                 pFrameSelectInputInfo,
                                 &frameSelectOutputInfo,
                                 stageInfo.stageId);

        pFrameSelectInputInfo->releaseEndIndex = pFrameSelectInputInfo->numRequests +
            ((pFrameSelectInputInfo->currentIndex + 1) - pFrameSelectInputInfo->validCount);

        for (UINT curRequestId = 0; curRequestId < pMappedFRO->GetNumRequests(); curRequestId++)
        {
            pMappedFRO->SetCurRequestState(ChiFeature2RequestState::OutputNotificationPending, curRequestId);
        }

        pFrameSelectInputInfo->numFramesSent += pMappedFRO->GetNumRequests();

        pMappedFRO->SetInterFeatureRequestPrivateData(m_featureId,
                                                      m_pCameraInfo->cameraId,
                                                      m_pInstanceProps->instanceId,
                                                      pFrameSelectInputInfo);
    }
    else
    {
        result = CDKResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::OnReleaseInputDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::OnReleaseInputDependency(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId
    ) const
{
    CDK_UNUSED_PARAM(requestId);
    CDKResult result = CDKResultSuccess;

    BOOL  allPortsReleased  = FALSE;
    UINT8 numRequestOutputs = 0;
    UINT8 numFramesSent     = 0;
    UINT8 releaseStartIndex = 0;
    UINT8 releaseEndIndex   = 0;

    const ChiFeature2RequestOutputInfo* pRequestOutputInfo = NULL;

    m_pFrameSelectThreadMutex->Lock();
    ChiFeature2FrameSelectInputInfo* pFrameSelectInputInfo =
        static_cast<ChiFeature2FrameSelectInputInfo*>(pRequestObject->
            GetInterFeatureRequestPrivateData(m_featureId,
                                              m_pCameraInfo->cameraId,
                                              m_pInstanceProps->instanceId));
    if (NULL != pFrameSelectInputInfo)
    {
        numFramesSent     = pFrameSelectInputInfo->numFramesSent;
        releaseStartIndex = pFrameSelectInputInfo->releaseStartIndex;
        releaseEndIndex   = pFrameSelectInputInfo->releaseEndIndex;
    }
    else
    {
        result = CDKResultEInvalidPointer;
    }
    m_pFrameSelectThreadMutex->Unlock();

    if (CDKResultSuccess == result)
    {
        UINT8 numRequests   = pRequestObject->GetNumRequests();

        result = pRequestObject->GetExternalRequestOutput(&numRequestOutputs, &pRequestOutputInfo, InitialFROIndex);

        if (CDKResultSuccess == result)
        {
            for (UINT8 portIndex = 0; portIndex < numRequestOutputs; ++portIndex)
            {
                const ChiFeature2PortDescriptor*       pPortDescriptor = pRequestOutputInfo[portIndex].pPortDescriptor;
                const ChiFeature2BufferMetadataInfo*   pBufferMetaInfo = &pRequestOutputInfo[portIndex].bufferMetadataInfo;

                if (NULL != pPortDescriptor && NULL != pBufferMetaInfo)
                {
                    BOOL    isPortNotified =
                        pRequestObject->GetOutputNotifiedForPort(pPortDescriptor->globalId, InitialFROIndex);

                    BOOL    isPortReleased =
                        pRequestObject->GetReleaseAcknowledgedForPort(pPortDescriptor->globalId, InitialFROIndex);

                    if (TRUE == isPortNotified && FALSE == isPortReleased)
                    {
                        // Release buffers
                        if (pRequestObject == pFrameSelectInputInfo->pInitialFRO)
                        {
                            CHITargetBufferManager* pManager = CHITargetBufferManager::GetTargetBufferManager(
                                pBufferMetaInfo->hBuffer);
                            if (NULL != pBufferMetaInfo->hBuffer)
                            {
                                result = pManager->ReleaseTargetBuffer(pBufferMetaInfo->hBuffer);
                            }
                            pRequestObject->SetReleaseAcknowledgedForPort(pPortDescriptor->globalId, InitialFROIndex);
                        }

                        for (UINT8 batchIndex = releaseStartIndex; batchIndex < releaseEndIndex; batchIndex++)
                        {
                            if (pFrameSelectInputInfo->inputBufferArray[RDIPortId][batchIndex].isValid == TRUE)
                            {
                                ReleaseInputDependencyResource(pFrameSelectInputInfo->pInitialFRO, batchIndex);
                            }
                        }

                        m_pFrameSelectThreadMutex->Lock();

                        pFrameSelectInputInfo->releaseStartIndex = pFrameSelectInputInfo->releaseEndIndex;
                        pRequestObject->SetInterFeatureRequestPrivateData(m_featureId,
                                                                          m_pCameraInfo->cameraId,
                                                                          m_pInstanceProps->instanceId,
                                                                          pFrameSelectInputInfo);
                        m_pFrameSelectThreadMutex->Unlock();

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
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::OnProcessingDependenciesComplete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2FrameSelect::OnProcessingDependenciesComplete(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId)
{
    // If all of our ports are done processing and our request is the last request (applicable to batched requests)
    // then send the notification
    if (FALSE == pRequestObject->GetOutputPortsProcessingCompleteNotified())
    {
        if ((TRUE == pRequestObject->AllOutputPortsProcessingComplete()))
        {
            m_pFrameSelectThreadMutex->Lock();
            ChiFeature2FrameSelectInputInfo* pFrameSelectInputInfo =
                static_cast<ChiFeature2FrameSelectInputInfo*>(
                    pRequestObject->GetInterFeatureRequestPrivateData(m_featureId,
                                                                      m_pCameraInfo->cameraId,
                                                                      m_pInstanceProps->instanceId));
            if (NULL != pFrameSelectInputInfo)
            {
                if (pFrameSelectInputInfo->pInitialFRO->GetCurRequestState(InitialFROIndex) ==
                    ChiFeature2RequestState::Complete)
                {
                    UINT8 hintFrames        = pRequestObject->GetFeatureHint()->numFrames;
                    UINT8 releaseStartIndex = pFrameSelectInputInfo->releaseStartIndex;

                    for (UINT8 batchIndex = releaseStartIndex; batchIndex < hintFrames; batchIndex++)
                    {
                        ReleaseInputDependencyResource(pFrameSelectInputInfo->pInitialFRO, batchIndex);
                    }

                    for (UINT8 i = 0; i < pFrameSelectInputInfo->maxInputPorts; i++)
                    {
                        pFrameSelectInputInfo->inputBufferArray[i].clear();
                        pFrameSelectInputInfo->inputBufferArray[i].shrink_to_fit();
                    }

                    VOID* pRequestContext = pFrameSelectInputInfo->pInitialFRO->GetPrivContext();
                    if (NULL != pRequestContext)
                    {
                        CHX_FREE(pRequestContext);
                        pRequestContext = NULL;
                        pFrameSelectInputInfo->pInitialFRO->SetPrivContext(NULL);
                    }

                    ChiFeature2MessageDescriptor featurePortMessage = {};
                    featurePortMessage.messageType = ChiFeature2MessageType::ProcessingDependenciesComplete;
                    featurePortMessage.pPrivData   = pRequestObject;
                    pRequestObject->SetCurRequestId(requestId);
                    ProcessFeatureMessage(&featurePortMessage);

                    pRequestObject->RemoveInterFeatureRequestPrivateData(m_featureId,
                                                                         m_pCameraInfo->cameraId,
                                                                         m_pInstanceProps->instanceId);

                    ChxUtils::Free(pFrameSelectInputInfo);
                    pFrameSelectInputInfo = NULL;

                }
            }
            m_pFrameSelectThreadMutex->Unlock();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::ReleaseInputDependencyResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::ReleaseInputDependencyResource(
    ChiFeature2RequestObject * pRequestObject,
    UINT8                      dependencyIndex
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pRequestObject)
    {
        ChiFeature2PortIdList inputPortList = { 0 };

        result = GetInputPortsForStage(FrameCollectionStage, &inputPortList);
        ChiFeaturePortData*    pInputPortData = NULL;
        ChiFeature2Identifier* pPorts         = static_cast<ChiFeature2Identifier*>(ChxUtils::Calloc(
            sizeof(ChiFeature2Identifier) * inputPortList.numPorts));

        if (NULL != pPorts)
        {
            for (UINT8 inputIdx = 0; inputIdx < inputPortList.numPorts; ++inputIdx)
            {
                pPorts[inputIdx] = inputPortList.pPorts[inputIdx];
            }

            CHX_LOG_INFO("Release dependencyIndex:%d resource!", dependencyIndex);
            // Release input dependency for upstream feature
            ChiFeature2MessageDescriptor releaseInputMsg;
            releaseInputMsg.messageType = ChiFeature2MessageType::ReleaseInputDependency;
            releaseInputMsg.message.releaseDependencyData.batchIndex                     = 0;
            releaseInputMsg.message.releaseDependencyData.numDependencies                = 1;
            releaseInputMsg.message.releaseDependencyData.pDependencies                  = static_cast<ChiFeature2Dependency*>(
                ChxUtils::Calloc(sizeof(ChiFeature2Dependency) * 1));
            releaseInputMsg.message.releaseDependencyData.pDependencies->dependencyIndex = dependencyIndex;
            releaseInputMsg.message.releaseDependencyData.pDependencies->numPorts        = inputPortList.numPorts;
            releaseInputMsg.message.releaseDependencyData.pDependencies->pPorts          = pPorts;
            releaseInputMsg.pPrivData                                                    = pRequestObject;

            ProcessFeatureMessage(&releaseInputMsg);

            ChxUtils::Free(releaseInputMsg.message.releaseDependencyData.pDependencies);
            ChxUtils::Free(pPorts);
        }
        else
        {
            result = CDKResultENoMemory;
            CHX_LOG_ERROR("No memory! Calloc failed!");
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Arg! pRequestObject is NULL!");
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::DoCleanupRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);

    CDKResult result = CDKResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2FrameSelect::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2FrameSelect::DoFlush()
{
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2FrameSelect* pFeatureFrameSelect = ChiFeature2FrameSelect::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeatureFrameSelect);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoQueryCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DoQueryCaps(
    VOID*                   pConfig,
    ChiFeature2QueryInfo*   pQueryInfo)
{
    CDK_UNUSED_PARAM(pConfig);
    CDKResult result = CDKResultSuccess;

    if (NULL != pQueryInfo)
    {
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2FrameSelectCaps);
        pQueryInfo->ppCapabilities = &Feature2FrameSelectCaps[0];
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
        auto CloneStream = [&pNegotiationOutput](CHISTREAM* pSrcCameraStream) -> CHISTREAM* {
            ChiStream* pStream = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
            ChxUtils::Memcpy(pStream, pSrcCameraStream, sizeof(CHISTREAM));
            pNegotiationOutput->pOwnedStreams->push_back(pStream);
            return pStream;
        };

        ChiFeature2Type featureId    = static_cast<ChiFeature2Type>(pNegotiationInfo->pFeatureInstanceId->featureId);
        const UINT      numStreams   = pNegotiationInfo->pFwkStreamConfig->numStreams;
        CHISTREAM**     ppChiStreams = pNegotiationInfo->pFwkStreamConfig->pChiStreams;

        GetSensorOutputDimension(pNegotiationInfo->pFeatureInstanceId->cameraId,
                                 pNegotiationInfo->pFwkStreamConfig,
                                 pNegotiationInfo->pFeatureInstanceId->flags,
                                 &RDIStream.width,
                                 &RDIStream.height);

        UINT32 blobWidth     = 0;
        UINT32 blobHeight    = 0;
        UINT32 physicalCamId = pNegotiationInfo->pFeatureInstanceId->cameraId;

        CHISTREAM* pRDIInputStream  = NULL;
        CHISTREAM* pRDIOutputStream = NULL;

        // Get correct physical camera ID blob resolution
        GetBlobResolution(pNegotiationInfo->pFwkStreamConfig, &blobWidth, &blobHeight, physicalCamId, isMultiCamera);

        pNegotiationOutput->pStreams->clear();

        // Configure input stream(RDI stream)
        pRDIInputStream = CloneStream(&Bayer2YuvStreamsInput);
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

        pRDIOutputStream = CloneStream(&Bayer2YuvStreamsOutput);

        if (NULL != pRDIOutputStream)
        {
            // configure output stream
            pRDIOutputStream->width  = blobWidth;
            pRDIOutputStream->height = blobHeight;
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
        pChiFeature2Ops->size               = sizeof(CHIFEATURE2OPS);
        pChiFeature2Ops->majorVersion       = Feature2FrameSelectMajorVersion;
        pChiFeature2Ops->minorVersion       = Feature2FrameSelectMinorVersion;
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
