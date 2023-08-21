////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2anchorsync.cpp
/// @brief AnchorSync feature implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2anchorsync.h"
#include "chifeature2base.h"
#include "chifeature2featurepool.h"
#include "chifeature2utils.h"

// NOWHINE FILE CP006:  Used standard libraries for performance improvements
// NOWHINE FILE GR017:  Need intrinsic type for sort


/// @brief anchorsync min buffer count to set in Meta TBM
static const UINT32 FEATURE2_ANCHORSYNC_MIN_METADATA_BUFFER_COUNT = 1;
static const UINT32 Feature2AnchorSyncMajorVersion                = 0;
static const UINT32 Feature2AnchorSyncMinorVersion                = 0;
static const CHAR*  VendorName                                    = "QTI";
static const UINT8  FrameCollectionStage                          = 0;
static const CHAR*  Feature2AnchorSyncCaps[]       =
{
    "AnchorSync",
};

static const UINT8 RDIPortId             = 0;
static const UINT8 FDPortId              = 1;
static const UINT8 DepthRDIPortId        = 6;
static const UINT8 DepthMetaPortId       = 7;
// Maximum active ports: Two RDI buffer port, Two FD buffer port and Two input metadata port
static const UINT8 MaxActiveInputPorts   = 6;
static const UINT8 MaxInputPerCamera     = 3;
static const UINT8 MaxOutputPerCamera    = 2;
// INPUT RDI/INPUT FD/OUTPUT RDI
static const UINT8 MaxStreamPerCamera    = 3;
static const UINT8 InputStreamOffset     = 0;
static const UINT8 OutputStreamOffset    = 2;

/// @brief Invalid Port Index.
static const UINT8 InvalidPortIndex      = 0xFF;

/**************** inputPort to outputPort map ************/
/*    inputPort(index) ----->   outputPort(index)
*     RDI_0  (0)                RDI_0   (0)
*     FD_0   (1)                   x
*     Meta_0 (2)                Meta_0  (1)
*/
/**************** inputPort to outputPort map ************/

static const UINT8 outputMapToInput[MaxOutputPerCamera] =
{
    0,
    2
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2AnchorSync * ChiFeature2AnchorSync::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{

    CDKResult              result   = CDKResultSuccess;
    ChiFeature2AnchorSync* pFeature = NULL;

    if (NULL != pCreateInputInfo)
    {
        pFeature = CHX_NEW(ChiFeature2AnchorSync);

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
// ChiFeature2AnchorSync::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2AnchorSync::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::OnInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::OnInitialize(
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
                    pPipelineData->minMetaBufferCount = FEATURE2_ANCHORSYNC_MIN_METADATA_BUFFER_COUNT;
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
                        UINT8 camIdx    = portIdx / MaxInputPerCamera;
                        UINT8 streamIdx = (portIdx % MaxInputPerCamera) + (camIdx * MaxStreamPerCamera);
                        if (camIdx >= pCreateInputInfo->pCameraInfo->numPhysicalCameras)
                        {
                            break;
                        }

                        if (ChiStreamTypeInput ==
                            pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx]->streamType)
                        {
                            ChiFeature2Identifier key = pPipelineDesc->pInputPortDescriptor[portIdx].globalId;
                            ChiFeaturePortData* pPort = GetPortData(&key);
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
                        else
                        {
                            CHX_LOG_ERROR("Wrong stream mapping! portIdx=%d", portIdx);
                            result = CDKResultEFailed;
                        }
                    }
                }

                for (UINT8 portIdx = 0; portIdx < pPipelineDesc->numOutputPorts; ++portIdx)
                {
                    UINT8 camIdx    = portIdx / MaxOutputPerCamera;
                    UINT8 streamIdx = OutputStreamOffset + (portIdx % MaxOutputPerCamera) +
                        (camIdx * MaxStreamPerCamera);

                    if (camIdx >= pCreateInputInfo->pCameraInfo->numPhysicalCameras)
                    {
                        break;
                    }

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
                        if (ChiStreamTypeOutput ==
                            pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx]->streamType)
                        {

                            pPort->pChiStream =
                                pCreateInputInfo->pStreamConfig->pChiStreams[streamIdx];
                        }
                        else
                        {
                            CHX_LOG_ERROR("Wrong stream mapping! portIdx=%d", portIdx);
                            result = CDKResultEFailed;
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
                        pPort->minBufferCount   = FEATURE2_ANCHORSYNC_MIN_METADATA_BUFFER_COUNT;
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
// ChiFeature2AnchorSync::ReleasePrivateInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::ReleasePrivateInfo(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult result                                       = CDKResultSuccess;
    ChiFeature2AnchorPickInputInfo** ppAnchorPickInputInfo = static_cast<ChiFeature2AnchorPickInputInfo**>(
        GetFeaturePrivContext(pRequestObject));

    if (NULL != ppAnchorPickInputInfo)
    {
        if (NULL != m_pCameraInfo)
        {
            for (UINT8 camIdx = 0 ; camIdx < m_pCameraInfo->numPhysicalCameras; camIdx++)
            {
                if (NULL != ppAnchorPickInputInfo[camIdx])
                {
                    for (UINT8 i = 0 ; i < ppAnchorPickInputInfo[camIdx]->maxInputPorts; i++)
                    {
                        ppAnchorPickInputInfo[camIdx]->inputBufferArray[i].clear();
                        ppAnchorPickInputInfo[camIdx]->inputBufferArray[i].shrink_to_fit();
                    }
                    CHX_DELETE ppAnchorPickInputInfo[camIdx];
                    ppAnchorPickInputInfo[camIdx] = NULL;
                }
            }
            ChxUtils::Free(ppAnchorPickInputInfo);
            ppAnchorPickInputInfo = NULL;
            SetFeaturePrivContext(pRequestObject, NULL);
        }
    }
    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2AnchorSync::FillDefaultAnchorParamsData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::FillDefaultAnchorParamsData(
    ChiFeature2AnchorPickInputInfo* pAnchorPickInputInfo
    ) const
{
    CDKResult result = CDKResultSuccess;

    /// OEM's Can Choose Anchor selection Mode, by default we will use sharpness
    pAnchorPickInputInfo->anchorFrameSelectionData.anchorFrameSelectionMode =
        static_cast<ChiFeature2AnchorFrameSelectionMode>(
            ExtensionModule::GetInstance()->EnableMFNRAnchorSelectionAlgorithmType());
    pAnchorPickInputInfo->anchorFrameSelectionData.numImagesAllowedAsAnchor = 3;
    pAnchorPickInputInfo->anchorFrameSelectionData.minHistrogramBin         = 0;
    pAnchorPickInputInfo->anchorFrameSelectionData.maxHistrogramBin         = 255;
    pAnchorPickInputInfo->anchorFrameSelectionData.numOfImagesToBlend       = 0;
    pAnchorPickInputInfo->anchorFrameSelectionData.desiredAnchorFrameIndex  = 0;
    pAnchorPickInputInfo->anchorFrameSelectionData.anchorFrameTimeRange     = 200000000;
    pAnchorPickInputInfo->anchorFrameSelectionData.brightnessTolerance      = 3;
    pAnchorPickInputInfo->anchorFrameSelectionData.removeExpectedBadImages  = FALSE;
    pAnchorPickInputInfo->anchorFrameSelectionData.numSharpnessImages       = 3;
    pAnchorPickInputInfo->anchorFrameSelectionData.sharpnessBlockSize       = 16;
    pAnchorPickInputInfo->anchorFrameSelectionData.sharpnessRankValue       = 0.9375;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::OnPrepareRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result   = CDKResultSuccess;
    VOID* pPrivContext = GetFeaturePrivContext(pRequestObject);

    if ((NULL == pPrivContext) && (NULL != pRequestObject))
    {
        if (NULL != m_pCameraInfo)
        {
            ChiFeature2AnchorPickInputInfo** ppAnchorPickInputInfo = static_cast<ChiFeature2AnchorPickInputInfo**>(CHX_CALLOC(
                sizeof(ChiFeature2AnchorPickInputInfo*) * m_pCameraInfo->numPhysicalCameras));
            if (NULL != ppAnchorPickInputInfo)
            {
                for (UINT8 camIdx = 0 ; camIdx < m_pCameraInfo->numPhysicalCameras; camIdx++)
                {
                    ChiFeature2AnchorPickInputInfo* pAnchorPickInputInfo = CHX_NEW ChiFeature2AnchorPickInputInfo;

                    if (NULL != pAnchorPickInputInfo)
                    {
                        // 3 input ports: RDI port, FD port and metadata
                        pAnchorPickInputInfo->maxInputPorts = MaxInputPortsPerCamera;
                        for (UINT8 i = 0 ; i < pAnchorPickInputInfo->maxInputPorts ; i++)
                        {
                            pAnchorPickInputInfo->inputBufferArray[i].reserve(pRequestObject->GetNumRequests());
                            FillDefaultAnchorParamsData(pAnchorPickInputInfo);
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("NO memory to allocate for pAnchorPickInputInfo");
                        result = CDKResultENoMemory;
                        break;
                    }
                    ppAnchorPickInputInfo[camIdx] = pAnchorPickInputInfo;
                }

                SetFeaturePrivContext(pRequestObject, ppAnchorPickInputInfo);

                if (CDKResultSuccess != result)
                {
                    ReleasePrivateInfo(pRequestObject);
                }
            }
            else
            {
                CHX_LOG_ERROR("NO memory to allocate for ppAnchorPickInputInfo");
                result = CDKResultENoMemory;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::OnPortCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::OnPortCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    result = ChiFeature2Base::OnPortCreate(pKey);

    if (NULL != pKey)
    {
        ChiFeaturePortData* pPortData = GetPortData(pKey);

        if ((NULL != pPortData) && (ChiFeature2PortType::MetaData == pPortData->globalId.portType))
        {
            pPortData->minBufferCount = FEATURE2_ANCHORSYNC_MIN_METADATA_BUFFER_COUNT;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Key");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::OnPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::OnPipelineCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pKey)
    {
        ChiFeaturePipelineData* pPipelineData = GetPipelineData(pKey);
        if (NULL != pPipelineData)
        {
            pPipelineData->minMetaBufferCount = FEATURE2_ANCHORSYNC_MIN_METADATA_BUFFER_COUNT;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid key");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::HandleCollectInputInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::HandleCollectInputInfo(
    ChiFeature2RequestObject*        pRequestObject,
    ChiFeature2AnchorPickInputInfo** ppAnchorPickInputInfo,
    UINT8                            stageId
    ) const
{
    CDKResult result = CDKResultSuccess;

    UINT8                 curRequestId  = 0;
    ChiFeature2PortIdList inputPortList = { 0 };

    if ((NULL == pRequestObject) || (NULL == ppAnchorPickInputInfo))
    {
        CHX_LOG_ERROR("Invalid Arg! pRequestObject=%p, pAnchorPickInputInfo=%p",
            pRequestObject, ppAnchorPickInputInfo);
        result = CDKResultEInvalidArg;
    }
    else
    {
        curRequestId = pRequestObject->GetCurRequestId();

        CHX_LOG_INFO("CurRequestId:%d, numOfRequests:%d", curRequestId, pRequestObject->GetNumRequests());

        if (curRequestId < pRequestObject->GetNumRequests())
        {
            result = GetInputPortsForStage(stageId, &inputPortList);
            ChiFeaturePortData*    pInputPortData = NULL;
            ChiFeature2Identifier* pPorts         = static_cast<ChiFeature2Identifier*>
                (ChxUtils::Calloc(sizeof(ChiFeature2Identifier) * inputPortList.numPorts));

            ChiFeature2AnchorPickInputInfo* pAnchorPickInputInfo = NULL;
            const Feature2ControllerResult* pMCCResult           =
                pRequestObject->GetUsecaseRequestObject()->GetMCCResult();

            if (NULL != pPorts)
            {
                for (UINT8 inputIdx = 0; inputIdx < inputPortList.numPorts; ++inputIdx)
                {
                    UINT8 camIdx = inputIdx / MaxInputPerCamera;
                    if (camIdx >= m_pCameraInfo->numPhysicalCameras)
                    {
                        break;
                    }

                    pAnchorPickInputInfo = ppAnchorPickInputInfo[camIdx];
                    if (NULL == pAnchorPickInputInfo)
                    {
                        CHX_LOG_ERROR("inputIdx=%d camIdx=%d pAnchorPickInputInfo is NULL!", inputIdx, camIdx);
                        result = CDKResultEFailed;
                        break;
                    }

                    if (FALSE == IsPortActive(pMCCResult, camIdx))
                    {
                        continue;
                    }

                    if ((FDPortId == inputIdx % MaxInputPerCamera) &&
                        (FALSE == IsFDBufferRequired(pRequestObject)))
                    {
                        CHX_LOG_INFO("FD buffer is not available, skip FD port");
                        continue;
                    }

                    ChiFeature2BufferMetadataInfo  inputBufferMetaInfo = { 0 };
                    ChiFeature2PortBufferInfo      inputBufferInfo ;

                    pPorts[inputIdx] = inputPortList.pPorts[inputIdx];
                    pInputPortData   = GetPortData(&(pPorts[inputIdx]));

                    if (NULL != pInputPortData)
                    {
                        pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                            &(pPorts[inputIdx]), &inputBufferMetaInfo.hBuffer, &inputBufferMetaInfo.key, curRequestId, 0);

                        CHITargetBufferManager* pTargetBufferManager =
                            CHITargetBufferManager::GetTargetBufferManager(inputBufferMetaInfo.hBuffer);

                        if (NULL != pTargetBufferManager)
                        {

                            if (pPorts[inputIdx].portType == ChiFeature2PortType::ImageBuffer)
                            {
                                inputBufferInfo.bufferType = ChiFeature2PortType::ImageBuffer;
                                result = GetStreamBuffer(inputBufferMetaInfo.hBuffer, inputBufferMetaInfo.key,
                                    &(inputBufferInfo.Buffer.streamBuffer));

                                if (CDKResultSuccess == result)
                                {
                                    CHX_LOG_INFO("InputIdx:%d GetStreamBuffer:%s, targetHandle:%p, bufferHandle:%p",
                                        inputIdx,
                                        pInputPortData->pPortName,
                                        inputBufferMetaInfo.hBuffer,
                                        inputBufferInfo.Buffer.streamBuffer.bufferInfo.phBuffer);
                                }
                                else
                                {
                                    CHX_LOG_INFO("InputIdx:%d GetStreamBuffer failed! hBuffer=%p",
                                        inputIdx,
                                        inputBufferMetaInfo.hBuffer);
                                }
                            }
                            else
                            {
                                inputBufferInfo.bufferType = ChiFeature2PortType::MetaData;
                                result = GetMetadataBuffer(inputBufferMetaInfo.hBuffer, inputBufferMetaInfo.key,
                                    &(inputBufferInfo.Buffer.hMetaHandle));

                                if (CDKResultSuccess == result)
                                {
                                    CHX_LOG_INFO("InputIdx:%d GetMetadataBuffer:%s, targetHandle:%p, metaHandle:%p",
                                        inputIdx,
                                        pInputPortData->pPortName,
                                        inputBufferMetaInfo.hBuffer,
                                        inputBufferInfo.Buffer.hMetaHandle);
                                }
                                else
                                {
                                    CHX_LOG_ERROR("InputIdx:%d GetMetadataBuffer failed! hBuffer=%p",
                                        inputIdx,
                                        inputBufferMetaInfo.hBuffer);
                                }
                            }

                            if (CDKResultSuccess == result)
                            {
                                pAnchorPickInputInfo->inputBufferArray[inputIdx % MaxInputPerCamera].push_back(
                                    inputBufferInfo);
                            }
                        }
                        else
                        {
                            CHX_LOG_ERROR("InputIdx=%d, Invalid target handle! inputBufferMetaInfo.hBuffer:%p",
                                inputIdx,
                                inputBufferMetaInfo.hBuffer);
                            result = CDKResultEInvalidArg;
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

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::DoAnchorPick
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::DoAnchorPick(
    ChiFeature2RequestObject*        pRequestObject,
    ChiFeature2AnchorPickInputInfo*  pAnchorPickInputInfo,
    ChiFeature2AnchorPickOutputInfo* pAnchorPickOutputInfo
    ) const
{
    CDKResult result                                                  = CDKResultSuccess;
    ChiFeature2AnchorFrameSelectionAlgorithm anchorSelectionAlgorithm = ChiFeature2AnchorFrameSelectionAlgorithm::None;
    ChiFeature2Hint* pFeatureHint = pRequestObject->GetFeatureHint();

    if ((NULL == pAnchorPickInputInfo) || (NULL == pAnchorPickOutputInfo))
    {
        CHX_LOG_ERROR("Invalid Arg! pAnchorPickInputInfo=%p, pAnchorPickOutputInfo=%p",
            pAnchorPickInputInfo, pAnchorPickOutputInfo);
        result = CDKResultEInvalidArg;
    }
    else
    {
        UINT8 numOfSequence = pAnchorPickInputInfo->inputBufferArray[0].size();

        pAnchorPickOutputInfo->outputOrder.reserve(numOfSequence);

        // Set default initial processing order
        for (UINT32 i = 0; i < numOfSequence; i++)
        {
            pAnchorPickOutputInfo->outputOrder.push_back(i);
        }

        anchorSelectionAlgorithm = static_cast<ChiFeature2AnchorFrameSelectionAlgorithm>(
            ExtensionModule::GetInstance()->EnableMFNRAnchorSelectionAlgorithm());

        switch (anchorSelectionAlgorithm)
        {
            case ChiFeature2AnchorFrameSelectionAlgorithm::NonFixed:
            {
                switch (pAnchorPickInputInfo->anchorFrameSelectionData.anchorFrameSelectionMode)
                {
                    case ChiFeature2AnchorFrameSelectionMode::Sharpness:
                    {
                        if (TRUE == IsFDBufferAvailable())
                        {
                            AnchorPickSharpnessBased(pAnchorPickInputInfo, pAnchorPickOutputInfo);
                        }
                        else
                        {
                            CHX_LOG_WARN("FD buffer is not available, skip sharpness based anchor pick");
                        }
                        break;
                    }

                    case ChiFeature2AnchorFrameSelectionMode::TimeStamp:
                    default:
                    {
                        if ((NULL != pFeatureHint) && (FALSE == pFeatureHint->captureMode.u.LowLight))
                        {
                            std::reverse(pAnchorPickOutputInfo->outputOrder.begin(),
                                pAnchorPickOutputInfo->outputOrder.end());
                        }
                        break;
                    }
                }
                break;
            }

            case ChiFeature2AnchorFrameSelectionAlgorithm::None:
            {
                // Keep initial processing order
                break;
            }

            default:
            {
                CHX_LOG_ERROR("Unknown/Invalid anchor frame selection and frame order option");
                // Keep default processing order, so that lets not break the request
                break;
            }
        }

        CHX_LOG_INFO("AnchorOutput numbers:%d", numOfSequence);

        for (auto bufferIdx: pAnchorPickOutputInfo->outputOrder)
        {
            CHX_LOG_INFO("AnchorOutputOrder:%d", bufferIdx);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::IsPortActive
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2AnchorSync::IsPortActive(
    const Feature2ControllerResult* pMCCResult,
    UINT32                          cameraIndex
    ) const
{
    BOOL requireEnabled = FALSE;
    if ((NULL == pMCCResult) || (cameraIndex >= m_pCameraInfo->numPhysicalCameras))
    {
        CHX_LOG_ERROR("Invalid Arg! pMCCResult = %p, cameraIndex = %d, numPhysicalCameras=%d",
            pMCCResult, cameraIndex, m_pCameraInfo->numPhysicalCameras);
    }
    else
    {
        if (TRUE == pMCCResult->isSnapshotFusion)
        {
            requireEnabled = ChxUtils::IsBitSet(pMCCResult->activeMap, cameraIndex);
        }
        else
        {
            requireEnabled = ((m_pCameraInfo->ppDeviceInfo[cameraIndex]->cameraId == pMCCResult->masterCameraId) ?
                TRUE : FALSE);
        }
    }
    return requireEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::GetOutputInputMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 ChiFeature2AnchorSync::GetOutputInputMap(
    UINT8 outputPortIdx
    ) const
{
    UINT8 inputPortIdx = InvalidPortIndex;

    if (InvalidPortIndex != outputPortIdx)
    {
        UINT8 outIdx = outputPortIdx % MaxOutputPerCamera;
        inputPortIdx = outputMapToInput[outIdx];
    }
    else
    {
        CHX_LOG_ERROR("Invalid output index!");
    }

    return inputPortIdx;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2AnchorSync::AnchorPickSharpnessBased
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::AnchorPickSharpnessBased(
    ChiFeature2AnchorPickInputInfo*  pAnchorPickInputInfo,
    ChiFeature2AnchorPickOutputInfo* pAnchorPickOutputInfo
    ) const
{
    CDKResult result = CDKResultSuccess;

    float sharpness[BufferQueueDepth]           = { 0 };
    UINT8 numOfSequence                         = pAnchorPickInputInfo->inputBufferArray[0].size();
    ChiFeature2PortBufferInfo* pInputBufferInfo = NULL;

    pAnchorPickInputInfo->anchorFrameSelectionData.numOfImagesToBlend       = numOfSequence - 1;
    pAnchorPickInputInfo->anchorFrameSelectionData.desiredAnchorFrameIndex  = numOfSequence - 1;

    for (UINT32 i = 0; i < pAnchorPickInputInfo->anchorFrameSelectionData.numSharpnessImages; i++)
    {

        pInputBufferInfo = &(pAnchorPickInputInfo->inputBufferArray[FDPortId].at(i));
        sharpness[i] = CalculateSharpness(&(pInputBufferInfo->Buffer.streamBuffer),
            pAnchorPickInputInfo->anchorFrameSelectionData.sharpnessBlockSize,
            pAnchorPickInputInfo->anchorFrameSelectionData.sharpnessRankValue);
    }

    // Choose the sharpest image
    INT32 anchorIndex = 0;
    for (UINT i = 0; i < numOfSequence; ++i)
    {
        if (IsCandidateForAnchor(
            pAnchorPickInputInfo->anchorFrameSelectionData.numImagesAllowedAsAnchor, i) &&
            sharpness[i] > sharpness[anchorIndex])
        {
            anchorIndex = i;
        }
    }
    pAnchorPickOutputInfo->outputOrder[0]           = anchorIndex;
    pAnchorPickOutputInfo->outputOrder[anchorIndex] = 0;


    // Sort the remaining images by sharpness
    auto compare = [&](int i1, int i2)
    {
        bool cand1 = IsCandidateForAnchor(
            pAnchorPickInputInfo->anchorFrameSelectionData.numImagesAllowedAsAnchor, i1);
        bool cand2 = IsCandidateForAnchor(
            pAnchorPickInputInfo->anchorFrameSelectionData.numImagesAllowedAsAnchor, i2);
        if (cand1 != cand2)
        {
            return cand1;
        }
        else
        {
            return sharpness[i1] > sharpness[i2];
        }
    };
    std::sort(pAnchorPickOutputInfo->outputOrder.begin() + 1,
        pAnchorPickOutputInfo->outputOrder.end() - 1, compare);

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2AnchorSync::CalculateSharpness
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT ChiFeature2AnchorSync::CalculateSharpness(
    CHISTREAMBUFFER* pInputFDBuffer,
    UINT32           sharpnessBlockSize,
    float            sharpnessRankValue
    ) const
{
    CDKResult   result              = CDKResultSuccess;
    VOID*       pHostptr            = NULL;
    INT         bufferLength        = (pInputFDBuffer->pStream->width * pInputFDBuffer->pStream->height * 3) / 2;
    FLOAT       sharpnessresult     = 0.0f;
    CHISTREAM   fdStream            = UsecaseSelector::GetFDOutStream();
    UINT32      fdStreamWidth       = fdStream.width;
    UINT32      fdStreamHeight      = fdStream.height;
    UINT32      numOfBlocksX        = (fdStreamWidth - 1) / sharpnessBlockSize;
    UINT32      numOfBlocksY        = (fdStreamHeight - 1) / sharpnessBlockSize;
    UINT32      numOfBlocks         = numOfBlocksX * numOfBlocksY;
    UINT32      downscaledStride    = pInputFDBuffer->pStream->width;

    pHostptr = CHIBufferManager::GetCPUAddress(&pInputFDBuffer->bufferInfo, bufferLength);

    CHX_LOG("Type=%d, phBuffer=%p, pHostptr=%p",
        pInputFDBuffer->bufferInfo.bufferType, pInputFDBuffer->bufferInfo.phBuffer, pHostptr);

    if ((CDKResultSuccess == result) && (pHostptr != NULL))
    {
        const UINT8* pImageLine = reinterpret_cast<const UINT8*>(pHostptr) +
            (fdStreamHeight - numOfBlocksY * sharpnessBlockSize) / 2 * downscaledStride +
            (fdStreamWidth - numOfBlocksX * sharpnessBlockSize) / 8 * 4;
        UINT16* pBlockSharpnessBuffer = static_cast<UINT16*>(CHX_CALLOC(numOfBlocks * sizeof(UINT16)));

        if (NULL != pBlockSharpnessBuffer)
        {
            UINT16* pSharpnessLine = pBlockSharpnessBuffer;

            for (UINT32 blockCounterY = 0; blockCounterY < numOfBlocksY; ++blockCounterY)
            {
                for (UINT32 pixelCounterY = 0; pixelCounterY < sharpnessBlockSize; ++pixelCounterY)
                {
                    const UINT8* pImage = pImageLine;
                    UINT16* pSharpness = pSharpnessLine;
                    for (UINT32 blockCounterX = 0; blockCounterX < numOfBlocksX; ++blockCounterX)
                    {
                        UINT32 sharpness = *pSharpness;
                        for (UINT32 pixelCounterX = 0; pixelCounterX < sharpnessBlockSize; ++pixelCounterX)
                        {
                            UINT8 derivX = abs(pImage[1] - pImage[0]);
                            UINT8 derivY = abs(pImage[downscaledStride] - pImage[0]);
                            sharpness += derivX;
                            sharpness += derivY;
                            ++pImage;
                        }
                        *pSharpness++ = static_cast<UINT16>(std::min<UINT32>(sharpness, 0xffff));
                    }
                    pImageLine += downscaledStride;
                }
                pSharpnessLine += numOfBlocksX;
            }

            UINT32 index = UINT32(numOfBlocks * sharpnessRankValue);
            std::nth_element(pBlockSharpnessBuffer, pBlockSharpnessBuffer + index, pBlockSharpnessBuffer + numOfBlocks);
            sharpnessresult = pBlockSharpnessBuffer[index];
            sharpnessresult /= sharpnessBlockSize * sharpnessBlockSize * 2;

            CHX_FREE(pBlockSharpnessBuffer);
        }
        else
        {
            CHX_LOG_ERROR("Calloc for pBlockSharpnessBuffer failed!");
            result = CDKResultENoMemory;
        }

        CHIBufferManager::PutCPUAddress(&pInputFDBuffer->bufferInfo, bufferLength, pHostptr);
    }
    else
    {
        CHX_LOG_ERROR("Invalid ChiStreamBuffer Type=%d, phBuffer=%p, pHostptr=%p",
            pInputFDBuffer->bufferInfo.bufferType, pInputFDBuffer->bufferInfo.phBuffer, pHostptr);
    }

    return sharpnessresult;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::BuildAndSubmitOutputInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::BuildAndSubmitOutputInfo(
    ChiFeature2RequestObject*        pRequestObject,
    ChiFeature2AnchorPickInputInfo** ppAnchorPickInputInfo,
    ChiFeature2AnchorPickOutputInfo* pAnchorPickOutputInfo,
    UINT8                            stageId
    ) const
{
    CDKResult result = CDKResultSuccess;
    if ((NULL == pRequestObject) || (NULL == ppAnchorPickInputInfo) || (NULL == pAnchorPickOutputInfo))
    {
        CHX_LOG_ERROR("Invalid Arg! pRequestObject=%p, pAnchorPickInputInfo=%p, pAnchorPickOutputInfo=%p",
            pRequestObject, ppAnchorPickInputInfo, pAnchorPickOutputInfo);
        result = CDKResultEInvalidArg;
    }
    else
    {
        ChiFeature2PortIdList          outputPortList    = { 0 };
        CHIMETADATAHANDLE              hMetaHandle       = { 0 };
        ChiFeature2BufferMetadataInfo* pFinalBufferMeta  = NULL;
        ChiFeature2PortBufferInfo*     pInputBufferInfo  = NULL;
        UINT8                          reorderedIdx      = 0;
        UINT8                          anchorFrameOrder  = 0;
        ChiFeature2Identifier          outputPortId      = { 0 };
        ChiFeaturePortData*            pOutputPortData   = NULL;
        ChiFeaturePipelineData*        pPipelineData     = NULL;
        CHITargetBufferManager*        pOutputTBM        = NULL;
        UINT64                         targetKey         = 0xFFFFFFFF;
        VOID*                          pTarget           = NULL;
        const Feature2ControllerResult* pMCCResult       = pRequestObject->GetUsecaseRequestObject()->GetMCCResult();

        result = GetOutputPortsForStage(stageId, &outputPortList);

        if (CDKResultSuccess == result)
        {
            ChiFeature2AnchorPickInputInfo* pAnchorPickInputInfo = NULL;
            for (UINT8 requestId = 0 ; requestId < pRequestObject->GetNumRequests(); ++requestId)
            {
                reorderedIdx = pAnchorPickOutputInfo->outputOrder.at(requestId);

                std::vector<ChiFeature2Identifier> extPorts;
                extPorts.reserve(outputPortList.numPorts);

                for (UINT8 outputIdx = 0; outputIdx < outputPortList.numPorts; ++outputIdx)
                {
                    UINT8 camIdx = outputIdx / MaxOutputPerCamera;
                    if ((DepthRDIPortId == outputIdx) || (DepthMetaPortId == outputIdx))
                    {
                        camIdx = GetMasterCameraIdx(pRequestObject->GetUsecaseRequestObject()->GetMCCResult(),
                            m_pCameraInfo);
                    }

                    if (camIdx >= m_pCameraInfo->numPhysicalCameras)
                    {
                        break;
                    }
                    UINT8 inputIdxMap    = GetOutputInputMap(outputIdx);
                    pAnchorPickInputInfo = ppAnchorPickInputInfo[camIdx];
                    outputPortId         = outputPortList.pPorts[outputIdx];
                    pOutputPortData      = GetPortData(&outputPortId);

                    if ((NULL == pAnchorPickInputInfo)     ||
                        (inputIdxMap >= MaxInputPerCamera) ||
                        (NULL == pOutputPortData))
                    {
                        CHX_LOG_ERROR("Invalid Arg! outputIdx=%d, requestid=%d pAnchorPickInputInfo=%p,"
                                      "inputIdMap=%d, pOutPortData=%p",
                                      outputIdx,
                                      requestId,
                                      pAnchorPickInputInfo,
                                      inputIdxMap,
                                      pOutputPortData);
                        result = CDKResultEFailed;
                        break;
                    }

                    if (FALSE == IsPortActive(pMCCResult, camIdx))
                    {
                        continue;
                    }

                    pFinalBufferMeta = NULL;
                    if (ChiFeature2PortDirectionType::ExternalOutput == outputPortId.portDirectionType)
                    {
                        // if no buffer and metadata for this request
                        pRequestObject->GetFinalBufferMetadataInfo(outputPortId,
                                &pFinalBufferMeta, requestId);
                        if (NULL == pFinalBufferMeta)
                        {
                            continue;
                        }
                    }

                    // Get buffer and metadata by reorder index
                    if ((DepthRDIPortId == outputIdx) || (DepthMetaPortId == outputIdx))
                    {
                        anchorFrameOrder = 0;
                        pInputBufferInfo = &(pAnchorPickInputInfo->inputBufferArray[inputIdxMap].at(anchorFrameOrder));
                    }
                    else
                    {
                        pInputBufferInfo = &(pAnchorPickInputInfo->inputBufferArray[inputIdxMap].at(reorderedIdx));
                    }

                    CHX_LOG_INFO("camIdx:%d Input[%d,%d]->Output[%d,%d]",
                        camIdx, (inputIdxMap + (MaxInputPerCamera * camIdx)), reorderedIdx, outputIdx, requestId);

                    pOutputTBM = pOutputPortData->pOutputBufferTbm;
                    if (outputPortId.portType == ChiFeature2PortType::ImageBuffer)
                    {
                        targetKey       = reinterpret_cast<UINT64>(pOutputPortData->pChiStream);
                        pTarget         = &(pInputBufferInfo->Buffer.streamBuffer);
                    }
                    else if (outputPortId.portType == ChiFeature2PortType::MetaData)
                    {
                        targetKey     = pOutputPortData->metadataClientId;
                        pTarget       = GetMetadataManager()->GetMetadataFromHandle(pInputBufferInfo->Buffer.hMetaHandle);
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
                            pSrcBufferTarget = pInputBufferInfo->Buffer.streamBuffer.bufferInfo.phBuffer;
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
                                pFinalBufferMeta->key       = targetKey;
                                pFinalBufferMeta->hBuffer   = hBuffer;
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

                if (CDKResultSuccess == result)
                {
                    CHX_LOG_INFO("Send buffer and metadata message:requestId:%d,", requestId);

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
// ChiFeature2AnchorSync::IsAnchorPickRequired
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2AnchorSync::IsAnchorPickRequired(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    BOOL isAnchorPickAlgoRequired = FALSE;

    if (NULL != pRequestObject)
    {
        for (UINT8 camIdx = 0; camIdx < m_pCameraInfo->numPhysicalCameras; ++camIdx)
        {
            VOID* pHintNumFrame = pRequestObject->GetInterFeatureRequestPrivateData(
                                    static_cast<UINT>(ChiFeature2Type::MFNR), camIdx, camIdx);
            if (NULL == pHintNumFrame)
            {
                pHintNumFrame = pRequestObject->GetInterFeatureRequestPrivateData(
                                    static_cast<UINT>(ChiFeature2Type::MFSR), camIdx, camIdx);
            }

            if (NULL != pHintNumFrame)
            {
                UINT hintNumFrame = *(static_cast<UINT*>(pHintNumFrame));
                if (hintNumFrame > 1)
                {
                    isAnchorPickAlgoRequired = TRUE;
                    break;
                }
            }
        }
    }

    return isAnchorPickAlgoRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::IsFDBufferRequired
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2AnchorSync::IsFDBufferRequired(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    BOOL isFDBufferRequired = TRUE;

    if ((FALSE == IsFDBufferAvailable()) ||
        (FALSE == IsAnchorPickRequired(pRequestObject)))
    {
        isFDBufferRequired = FALSE;
    }
    return isFDBufferRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::OnExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::OnExecuteProcessRequest(
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
    BOOL                                readyToGoNextStage  = TRUE;
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
                break;
            }
            case FrameCollectionStage:
            {
                ChiFeature2AnchorPickInputInfo** ppAnchorPickInputInfo =
                    static_cast<ChiFeature2AnchorPickInputInfo**>(GetFeaturePrivContext(pRequestObject));

                if (NULL != ppAnchorPickInputInfo)
                {
                    result = HandleCollectInputInfo(pRequestObject, ppAnchorPickInputInfo, stageId);

                    // Set output notification as this index of request is done
                    pRequestObject->SetCurRequestState(ChiFeature2RequestState::OutputNotificationPending,
                        curRequestId);

                    if (CDKResultSuccess == result)
                    {
                        // only All of input buffers are ready, that can go on.
                        if (curRequestId < (pRequestObject->GetNumRequests() - 1))
                        {
                            readyToGoNextStage = FALSE;
                        }

                        // when call information collected, we can go for anchor pick or other stuff
                        if (TRUE == readyToGoNextStage)
                        {
                            ChiFeature2AnchorPickOutputInfo anchorPickOutputInfo;

                            if (TRUE == IsAnchorPickRequired(pRequestObject))
                            {
                                // Search master camera and do anchor pick for master camera
                                const Feature2ControllerResult* pMCCResult =
                                            pRequestObject->GetUsecaseRequestObject()->GetMCCResult();

                                for (UINT8 camIdx = 0 ; camIdx < m_pCameraInfo->numPhysicalCameras; camIdx++)
                                {
                                    if (pMCCResult->masterCameraId == m_pCameraInfo->ppDeviceInfo[camIdx]->cameraId)
                                    {
                                        DoAnchorPick(pRequestObject,
                                            ppAnchorPickInputInfo[camIdx],
                                            &anchorPickOutputInfo);
                                    }
                                }
                                for (UINT8 index = anchorPickOutputInfo.outputOrder.size();
                                    index < pRequestObject->GetNumRequests();
                                    index++)
                                {
                                    anchorPickOutputInfo.outputOrder.push_back(index);
                                }
                            }
                            else
                            {
                                anchorPickOutputInfo.outputOrder.reserve(pRequestObject->GetNumRequests());
                                for (UINT8 i = 0 ; i < pRequestObject->GetNumRequests(); i++)
                                {
                                    anchorPickOutputInfo.outputOrder.push_back(i);
                                }
                            }

                            // Send anchor picked frame as message
                            ChiFeature2MessageDescriptor anchorFrameMsg;
                            ChxUtils::Memset(&anchorFrameMsg, 0, sizeof(anchorFrameMsg));
                            anchorFrameMsg.messageType                          = ChiFeature2MessageType::MessageNotification;
                            anchorFrameMsg.message.notificationData.messageType = ChiFeature2ChiMessageType::AnchorPick;
                            ChiFeature2AnchorSyncData  anchorData;

                            anchorData.anchorFrameIdx = anchorPickOutputInfo.outputOrder[0];
                            anchorData.numberOfFrames = anchorPickOutputInfo.outputOrder.size();

                            anchorFrameMsg.message.notificationData.message.anchorData = anchorData;
                            anchorFrameMsg.pPrivData                                   = pRequestObject;
                            ProcessFeatureMessage(&anchorFrameMsg);

                            BuildAndSubmitOutputInfo(pRequestObject, ppAnchorPickInputInfo,
                                &anchorPickOutputInfo, stageId);
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("pAnchorPickInputInfo == NULL! no private data populated!");
                    result = CDKResultEInvalidArg;
                }
                break;
            }

            default:
            {
                CHX_LOG_ERROR("Invalid stage ID!");
                break;
            }
        }

        if ((nextStageId < GetNumStages()) && (TRUE == readyToGoNextStage))
        {
            pStageDescriptor = GetStageDescriptor(nextStageId);

            if (NULL != pStageDescriptor)
            {
                numDependencyLists = GetNumDependencyListsFromStageDescriptor(pStageDescriptor, 0, 0);
                SetNextStageInfoFromStageDescriptor(pRequestObject, pStageDescriptor, 0, numDependencyLists);
                result = PopulateDependency(pRequestObject);
            }
        }
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::ReleaseInputDependencyResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::ReleaseInputDependencyResource(
    ChiFeature2RequestObject * pRequestObject,
    UINT8                      requestId
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
        UINT8 numActivePorts = 0;
        if (NULL != pPorts)
        {
            const Feature2ControllerResult* pMCCResult = pRequestObject->GetUsecaseRequestObject()->GetMCCResult();
            for (UINT8 inputIdx = 0; inputIdx < inputPortList.numPorts; ++inputIdx)
            {
                UINT8 camIdx         = inputIdx / MaxInputPerCamera;

                if (camIdx >= m_pCameraInfo->numPhysicalCameras)
                {
                    break;
                }

                if (FALSE == IsPortActive(pMCCResult, camIdx))
                {
                    continue;
                }

                if ((FDPortId == inputIdx % MaxInputPerCamera) &&
                    (FALSE == IsFDBufferRequired(pRequestObject)))
                {
                    CHX_LOG_INFO("FD buffer is not available, skip FD port");
                    continue;
                }

                pPorts[numActivePorts] = inputPortList.pPorts[inputIdx];
                numActivePorts++;
            }

            CHX_LOG_INFO("Release requestIdx:%d resource!", requestId);

            // Release input dependency for upstream feature
            ChiFeature2MessageDescriptor releaseInputMsg;
            releaseInputMsg.messageType = ChiFeature2MessageType::ReleaseInputDependency;
            releaseInputMsg.message.releaseDependencyData.batchIndex         = requestId;
            releaseInputMsg.message.releaseDependencyData.numDependencies    = 1;
            releaseInputMsg.message.releaseDependencyData.pDependencies      = static_cast<ChiFeature2Dependency*>(
                                                                ChxUtils::Calloc(sizeof(ChiFeature2Dependency) * 1));
            releaseInputMsg.message.releaseDependencyData.pDependencies->dependencyIndex = 0;
            releaseInputMsg.message.releaseDependencyData.pDependencies->numPorts        = numActivePorts;
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
// ChiFeature2AnchorSync::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult                       result               = CDKResultSuccess;
    ChiFeature2AnchorPickInputInfo* pAnchorPickInputInfo = NULL;

    if (NULL != pRequestObject)
    {
        for (UINT8 requestId = 0; requestId < pRequestObject->GetNumRequests(); ++ requestId)
        {
            CHX_LOG_INFO("Clean up resource for %s_%d numRequests %d", pRequestObject->IdentifierString(), requestId,
                pRequestObject->GetNumRequests());

            // Since DoCleanUp is called when all requests are complete, we need to release all together
            // Ideal solution would be to maintain an output to input map so when an output is complete
            // we can release corresponding input
            ReleaseInputDependencyResource(pRequestObject, requestId);
        }
        ReleasePrivateInfo(pRequestObject);
    }
    else
    {
        CHX_LOG_ERROR("Invalid Arg! pRequestObject == NULL!");
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::DoFlush()
{
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::PopulateDependencyPortsBasedOnMCC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::PopulateDependencyPortsBasedOnMCC(
    ChiFeature2RequestObject*         pRequestObject,
    UINT8                             dependencyIndex,
    const ChiFeature2InputDependency* pInputDependency
    ) const
{
    CDKResult     result            = CDKResultSuccess;
    ChiMetadata*  pFeatureSettings  = NULL;
    UINT8         requestId         = pRequestObject->GetCurRequestId();

    const Feature2ControllerResult* pMCCResult = pRequestObject->GetUsecaseRequestObject()->GetMCCResult();

    for (UINT8 portIndex = 0; portIndex < pInputDependency->numInputDependency; ++portIndex)
    {
        UINT8 camIdx                             = portIndex / MaxInputPerCamera;

        if (camIdx >= m_pCameraInfo->numPhysicalCameras)
        {
            break;
        }

        ChiFeature2PortDescriptor portDescriptor = pInputDependency->pInputDependency[portIndex];
        ChiFeature2Identifier     portidentifier = portDescriptor.globalId;
        BOOL                      isPortActive   = IsPortActive(pMCCResult, camIdx);
        CHX_LOG_INFO("portIndex:%d, camIdx:%d,isActive:%d",
            portIndex, camIdx, isPortActive);

        if ((FDPortId == portIndex % MaxInputPerCamera) &&
            (FALSE == IsFDBufferRequired(pRequestObject)))
        {
            // skip fd port
            CHX_LOG_INFO("FD buffer is not available, skip input dependency on FD port");
            continue;
        }

        if (TRUE == isPortActive)
        {
            result = pRequestObject->SetPortDescriptor(ChiFeature2SequenceOrder::Next,
                ChiFeature2RequestObjectOpsType::InputDependency,
                &portidentifier, &portDescriptor, requestId, dependencyIndex);

            CHX_LOG_INFO("%s: Set dependency on port %s, requestId %d, dependencyIndex %d",
                pRequestObject->IdentifierString(),
                portDescriptor.pPortName,
                requestId,
                dependencyIndex);

            if (portidentifier.portType == ChiFeature2PortType::MetaData)
            {
                pFeatureSettings = ChiMetadata::Create(NULL, 0, true);
                if (NULL == pFeatureSettings)
                {
                    CHX_LOG_ERROR("Alloc memory failed");
                    result = CDKResultENoMemory;
                }

                if (CDKResultSuccess == result)
                {
                    result = PopulateDependencySettings(pRequestObject, dependencyIndex, &portidentifier, pFeatureSettings);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2AnchorSync::OnPopulateDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2AnchorSync::OnPopulateDependency (
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
                            PopulateDependencyPortsBasedOnMCC(pRequestObject, listIndex, pDependency);
                        }
                    }
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2AnchorSync* pFeatureAnchorSync = ChiFeature2AnchorSync::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeatureAnchorSync);
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
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2AnchorSyncCaps);
        pQueryInfo->ppCapabilities = &Feature2AnchorSyncCaps[0];
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
        auto CloneStream = [&pNegotiationOutput](CHISTREAM* pSrcCameraStream) -> CHISTREAM* {
            ChiStream* pStream = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
            ChxUtils::Memcpy(pStream, pSrcCameraStream, sizeof(CHISTREAM));
            pNegotiationOutput->pOwnedStreams->push_back(pStream);
            return pStream;
        };

        ChiFeature2Type featureId = static_cast<ChiFeature2Type>(pNegotiationInfo->pFeatureInstanceId->featureId);
        const UINT   numStreams   = pNegotiationInfo->pFwkStreamConfig->numStreams;
        CHISTREAM**  ppChiStreams = pNegotiationInfo->pFwkStreamConfig->pChiStreams;
        pNegotiationOutput->pStreams->clear();

        UINT32     blobWidth        = 0;
        UINT32     blobHeight       = 0;
        UINT32     physicalCamId    = pNegotiationInfo->pFeatureInstanceId->cameraId;
        CHISTREAM* pRDIInputStream  = NULL;
        CHISTREAM* pFDInputStream   = NULL;
        CHISTREAM* pRDIOutputStream = NULL;


        if (TRUE == isMultiCamera)
        {
            for (UINT8 streamIdx = 0 ; streamIdx < pNegotiationInfo->pFwkStreamConfig->numStreams; ++streamIdx)
            {
                camera3_stream_t* pStream =
                    reinterpret_cast<camera3_stream_t*>(pNegotiationInfo->pFwkStreamConfig->pChiStreams[streamIdx]);

                if ((TRUE == UsecaseSelector::IsYUVInStream(pStream)) &&
                    (0 != pStream->height))
                {
                    FLOAT yuvAspectRatio  = static_cast<FLOAT>(pStream->width) / pStream->height;

                    UsecaseSelector::UpdateFDStream(yuvAspectRatio);
                    break;
                }
            }
        }

        for (UINT8 camIdx = 0 ; camIdx < pNegotiationInfo->pLogicalCameraInfo->numPhysicalCameras; camIdx++)
        {
            physicalCamId = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[camIdx]->cameraId;

            GetSensorOutputDimension(physicalCamId,
                                     pNegotiationInfo->pFwkStreamConfig,
                                     pNegotiationInfo->pFeatureInstanceId->flags,
                                     &RDIStream.width,
                                     &RDIStream.height);

            // Get correct physical camera ID blob resolution
            GetBlobResolution(pNegotiationInfo->pFwkStreamConfig,
                &blobWidth, &blobHeight, physicalCamId, isMultiCamera);

            // Configure input stream(RDI/FD stream)
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

            CHISTREAM fdStream = UsecaseSelector::GetFDOutStream();
            pFDInputStream = CloneStream(&fdStream);
            if (NULL != pFDInputStream)
            {
                pFDInputStream->streamType = ChiStreamTypeInput;
                pNegotiationOutput->pStreams->push_back(pFDInputStream);
            }
            else
            {
                CHX_LOG_ERROR("Clone FD stream failed!");
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

            for (UINT8 i = 0 ; i < MaxInputPerCamera; i++)
            {
                UINT8 portId = i + (MaxInputPerCamera * camIdx);
                pNegotiationOutput->pInputPortMap->push_back(std::pair<UINT8, UINT8>(portId, camIdx));
            }
            for (UINT8 i = 0 ; i < MaxOutputPerCamera; i++)
            {
                UINT8 portId = i + (MaxOutputPerCamera * camIdx);
                pNegotiationOutput->pOutputPortMap->push_back(std::pair<UINT8, UINT8>(portId, camIdx));
            }

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
        pChiFeature2Ops->majorVersion           = Feature2AnchorSyncMajorVersion;
        pChiFeature2Ops->minorVersion           = Feature2AnchorSyncMinorVersion;
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
