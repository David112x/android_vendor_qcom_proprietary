////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2wrapper.cpp
/// @brief CHI feature2 wrapper class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2wrapper.h"

// NOWHINE FILE CP006:  used standard libraries for performance improvements

using namespace std;

// Numbers of RDI buffer will be cached for MC RDI queue
static const UINT32 MaxFrameCntNeededForFeature2 = 8;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Feature2Wrapper* Feature2Wrapper::Create(
    Feature2WrapperCreateInputInfo* pCreateData,
    UINT32                          physicalCameraIndex)
{
    Feature2Wrapper* pFeature = NULL;

    if (NULL != pCreateData)
    {
        pFeature = CHX_NEW Feature2Wrapper;

        if (NULL != pFeature)
        {
            if (CDKResultSuccess != pFeature->Initialize(pCreateData))
            {
                pFeature->Destroy(FALSE);
                pFeature = NULL;
            }
            else
            {
                pFeature->SetPhysicalCameraIndex(physicalCameraIndex);
            }
        }
        else
        {
            CHX_LOG_ERROR("Create Feature2Wrapper failed!");
        }
    }
    else
    {
        CHX_LOG_ERROR("pCreateData is NULL!");
    }

    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2Wrapper::Initialize(
    Feature2WrapperCreateInputInfo* pCreateData)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pCreateData->pUsecaseBase ||
        NULL == pCreateData->pMetadataManager ||
        NULL == pCreateData->pFrameworkStreamConfig)
    {
        CHX_LOG_ERROR("Invalid input! %p %p %p", pCreateData->pUsecaseBase,
            pCreateData->pMetadataManager, pCreateData->pFrameworkStreamConfig);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        m_pUROMapLock          = Mutex::Create();
        m_pFGMLock             = Mutex::Create();
        m_pPartialMetadataLock = Mutex::Create();

        if ((NULL == m_pUROMapLock) || (NULL == m_pFGMLock) || (NULL == m_pPartialMetadataLock))
        {
            result = CDKResultENoMemory;
            CHX_LOG_ERROR("Cannot allocate URO lock %p, FGM lock %p or PartialMetadata Lock %p",
                          m_pUROMapLock,
                          m_pFGMLock,
                          m_pPartialMetadataLock);
        }
    }

    if (CDKResultSuccess == result)
    {
        m_metaClientId              = 0;
        m_pUsecaseBase              = pCreateData->pUsecaseBase;
        m_pMetadataManager          = pCreateData->pMetadataManager;
        m_pFrameworkStreamConfig    = pCreateData->pFrameworkStreamConfig;
        m_inputOutputType           = pCreateData->inputOutputType;
        m_lastZSLFrameNumber        = 0;

        m_internalInputStreamMap.clear();

        for (UINT32 i = 0; i < pCreateData->internalInputStreams.size(); i++)
        {
            m_internalInputStreamMap[pCreateData->internalInputStreams[i]] =
                    static_cast<ChiStream*>(CHX_CALLOC(sizeof(ChiStream)));
            ChxUtils::Memcpy(m_internalInputStreamMap[pCreateData->internalInputStreams[i]],
                             pCreateData->internalInputStreams[i], sizeof(ChiStream));
            m_internalInputStreamMap[pCreateData->internalInputStreams[i]]->streamType = ChiStreamTypeInput;
        }
    }

    ExtensionModule::GetInstance()->GetVendorTagOps(&m_vendorTagOps);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::Destroy(
    BOOL isForced)
{
    CAMX_UNREFERENCED_PARAM(isForced);

    if (NULL != m_pChiFeatureGraphManager)
    {
        m_pChiFeatureGraphManager->Flush();
        m_pChiFeatureGraphManager->Destroy();
        m_pChiFeatureGraphManager = NULL;
    }

    for (auto const& it : m_usecaseRequestObjectMap)
    {
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObject = it.second;
        if (NULL != pUsecaseRequestObject)
        {
            m_pMetadataManager->Release(pUsecaseRequestObject->GetAppSettings());
            pUsecaseRequestObject->Destroy();
            pUsecaseRequestObject = NULL;
        }
    }
    m_usecaseRequestObjectMap.clear();

    for (auto const& it : m_frameworkTargetBufferManagers)
    {
        CHITargetBufferManager* pTargetBufferManager = it.second;
        if (NULL != pTargetBufferManager)
        {
            pTargetBufferManager->Destroy();
            pTargetBufferManager = NULL;
        }
    }
    m_frameworkTargetBufferManagers.clear();

    for (auto const& it : m_internalInputStreamMap)
    {
        ChiStream* pStream = it.second;
        if (NULL != pStream)
        {
            CHX_FREE(pStream);
            pStream = NULL;
        }
    }
    m_internalInputStreamMap.clear();

    for (auto pTargetBufferManager : m_pInputMetadataTBMData)
    {
        if (NULL != pTargetBufferManager)
        {
            pTargetBufferManager->Destroy();
            pTargetBufferManager = NULL;
        }
    }
    m_pInputMetadataTBMData.clear();

    if (NULL != m_pUROMapLock)
    {
        m_pUROMapLock->Destroy();
        m_pUROMapLock = NULL;
    }

    if (NULL != m_pFGMLock)
    {
        m_pFGMLock->Destroy();
        m_pFGMLock = NULL;
    }

    if (NULL != m_pPartialMetadataLock)
    {
        m_pPartialMetadataLock->Destroy();
        m_pPartialMetadataLock = NULL;
    }

    CHX_DELETE(this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::Dump(
    INT fd)
{
    m_pUROMapLock->Lock();
    for (auto Mapiterator = m_usecaseRequestObjectMap.cbegin(); Mapiterator != m_usecaseRequestObjectMap.cend(); ++Mapiterator)
    {
        UINT                             frameNum           = Mapiterator->first;
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObj = Mapiterator->second;

        if ((NULL != pUsecaseRequestObj) &&
            (ChiFeature2UsecaseRequestObjectState::Invalid != pUsecaseRequestObj->GetRequestState()))
        {
            if (NULL != m_pChiFeatureGraphManager)
            {
                m_pChiFeatureGraphManager->Dump(fd, pUsecaseRequestObj);
            }
        }
    }
    m_pUROMapLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::Pause
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::Pause(
    BOOL isForced)
{
    CAMX_UNREFERENCED_PARAM(isForced);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::OverrideUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiUsecase* Feature2Wrapper::OverrideUsecase(
    LogicalCameraInfo*              pCameraInfo,
    camera3_stream_configuration_t* pStreamConfig)
{
    CAMX_UNREFERENCED_PARAM(pStreamConfig);

    m_pLogicalCameraInfo = pCameraInfo;

    m_pChiUsecase = m_pUsecaseBase->GetChiUseCase();

    return m_pChiUsecase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::PipelineCreated
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::PipelineCreated(
    UINT32 sessionId,
    UINT32 pipelineIndex)
{
    CAMX_UNREFERENCED_PARAM(sessionId);
    CAMX_UNREFERENCED_PARAM(pipelineIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::PostUsecaseCreated
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::PostUsecaseCreated()
{
    if (NULL == m_pChiFeatureGraphManager)
    {
        CreateFeatureGraphManager();
    }

    if (0 == m_frameworkTargetBufferManagers.size())
    {
        CreateTargetBufferManagers();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::GetRequestInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2Wrapper::GetRequestInfo(
    camera3_capture_request_t*  pRequest,
    FeatureRequestInfo*         pOutputRequests,
    FeatureRequestType          requestType)
{
    CAMX_UNREFERENCED_PARAM(pRequest);
    CAMX_UNREFERENCED_PARAM(pOutputRequests);
    CAMX_UNREFERENCED_PARAM(requestType);

    CDKResult                           result                  = CDKResultSuccess;
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObject   = GetUROForRequest(pRequest);

    result = ProcessURO(pUsecaseRequestObject, pRequest);

    if (CDKResultSuccess == result)
    {
        std::vector<ChiFeature2UsecaseRequestObjectExtSrcStreamData> inputStreamsInfo =
            pUsecaseRequestObject->GetSelectedInputSrcStreamsInfo();

        for (UINT32 index = 0; index < inputStreamsInfo.size(); ++index)
        {
            result = UpdateFeatureRequestInfo(pRequest, &inputStreamsInfo[index], pOutputRequests);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::GetUROForRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2UsecaseRequestObject* Feature2Wrapper::GetUROForRequest(
    camera3_capture_request_t* pRequest)
{
    CDKResult                        result                = CDKResultSuccess;
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObject = NULL;

    m_pUROMapLock->Lock();
    if (m_usecaseRequestObjectMap.find(pRequest->frame_number) != m_usecaseRequestObjectMap.end())
    {
        CHX_LOG_ERROR("Usecase request object exists for frame number %d",
                pRequest->frame_number);

        result = CDKResultEExists;
    }
    m_pUROMapLock->Unlock();

    if (CDKResultSuccess == result)
    {
        pUsecaseRequestObject = CreateUsecaseRequestObject(pRequest);

        if (NULL == pUsecaseRequestObject)
        {
            CHX_LOG_ERROR("Usecase request object creation failed");
            result = CDKResultEFailed;
        }
    }

    return pUsecaseRequestObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::ProcessURO
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2Wrapper::ProcessURO(
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObject,
    camera3_capture_request_t* pRequest)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pUsecaseRequestObject)
    {
        m_pUROMapLock->Lock();
        m_usecaseRequestObjectMap[pRequest->frame_number] = pUsecaseRequestObject;
        m_pUROMapLock->Unlock();

        m_pFGMLock->Lock();
        result = m_pChiFeatureGraphManager->ExecuteProcessRequest(pUsecaseRequestObject);
        m_pFGMLock->Unlock();

        if ((CDKResultSuccess != result) ||
            (ChiFeature2UsecaseRequestObjectState::InputConfigPending != pUsecaseRequestObject->GetRequestState()))
        {
            CHX_LOG_ERROR("FeatureGraphManager ExecuteProcessRequest failed! "
                          "frameNumber=%d pUsecaseRequestObject=%p UROState=%d",
                          pRequest->frame_number, pUsecaseRequestObject,
                          pUsecaseRequestObject->GetRequestState());
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
/// Feature2Wrapper::GetRequestInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2Wrapper::GetRequestInfo(
    camera3_capture_request_t*               pRequest,
    std::vector<FeatureRequestInfoForStream> &pOutputRequests,
    FeatureRequestType                       requestType)
{
    CAMX_UNREFERENCED_PARAM(requestType);

    CDKResult                           result                  = CDKResultSuccess;
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObject   = GetUROForRequest(pRequest);

    result = ProcessURO(pUsecaseRequestObject, pRequest);

    if (CDKResultSuccess == result)
    {
        std::vector<ChiFeature2UsecaseRequestObjectExtSrcStreamData> inputStreamsInfo =
                pUsecaseRequestObject->GetSelectedInputSrcStreamsInfo();

        for (UINT32 index = 0; index < inputStreamsInfo.size(); ++index)
        {
            if (NULL != inputStreamsInfo[index].pChiStream)
            {
                FeatureRequestInfoForStream                      outputRequestInfo = { 0 };
                ChiFeature2UsecaseRequestObjectExtSrcStreamData* pInputStreamInfo  = &inputStreamsInfo[index];

                if (NULL != pInputStreamInfo)
                {
                    outputRequestInfo.pStream = pInputStreamInfo->pChiStream;
                    result = UpdateFeatureRequestInfo(pRequest, pInputStreamInfo, &outputRequestInfo.featureRequestInfo);
                }

                if (CDKResultSuccess == result)
                {
                    pOutputRequests.push_back(outputRequestInfo);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::UpdateFeatureRequestInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2Wrapper::UpdateFeatureRequestInfo(
    camera3_capture_request_t*                       pRequest,
    ChiFeature2UsecaseRequestObjectExtSrcStreamData* pInputStreamData,
    FeatureRequestInfo*                              pOutputRequestInfo)
{
    CDKResult result = CDKResultEFailed;
    if (NULL != pInputStreamData->pChiStream)
    {
        CHIMETAHANDLE* phSettings = NULL;

        switch (pInputStreamData->pChiStream->format)
        {
            case ChiStreamFormat::ChiStreamFormatRawOpaque:
            case ChiStreamFormat::ChiStreamFormatRaw16:
            case ChiStreamFormat::ChiStreamFormatRaw10:
                pOutputRequestInfo->numOfRequest = pInputStreamData->numInputs;
                phSettings                       = pInputStreamData->inputSettings.data();
                CHX_LOG_INFO("NumOfRequest %d", pOutputRequestInfo->numOfRequest);

                for (UINT32 i = 0; i < pOutputRequestInfo->numOfRequest; ++i)
                {
                    if (0 == i)
                    {
                        pOutputRequestInfo->isReturnResult[i] = TRUE;
                    }
                    else
                    {
                        pOutputRequestInfo->isReturnResult[i] = FALSE;
                    }

                    ChxUtils::Memcpy(&pOutputRequestInfo->request[i], pRequest, sizeof(camera3_capture_request));
                    if ((NULL != phSettings) && (NULL != phSettings[i]))
                    {
                        pOutputRequestInfo->metadataInfo[i].pInputMetadata =
                                m_pMetadataManager->GetMetadataFromHandle(phSettings[i]);
                    }
                    else
                    {
                        pOutputRequestInfo->metadataInfo[i].pInputMetadata = NULL;
                    }
                    pOutputRequestInfo->metadataInfo[i].pOutputMetadata = NULL;
                }
                result = CDKResultSuccess;
                CHX_LOG_INFO("raw opaque stream is selected");
                break;
            case ChiStreamFormatYCbCr420_888:
                if (TRUE == IsFDStream(pInputStreamData->pChiStream))
                {
                    pOutputRequestInfo->isFdStreamRequired = TRUE;
                    CHX_LOG_INFO("FD stream is selected");
                }
                result = CDKResultSuccess;
                break;
            default:
                result = CDKResultEInvalidArg;
                CHX_LOG_ERROR("unsupported stream %d", pInputStreamData->pChiStream->format);
                break;
        }
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2Wrapper::ExecuteProcessRequest(
    camera3_capture_request_t* pRequest)
{
    CAMX_UNREFERENCED_PARAM(pRequest);

    CHX_LOG_INFO("ExecuteProcessRequest frameNumber %d", pRequest->frame_number);
    CDKResult                           result                  = CDKResultSuccess;
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObject   = NULL;
    UINT32                              fwkFrameNum             = pRequest->frame_number;
    UINT32                              fwkFrameIdx             = (fwkFrameNum % MaxOutstandingRequests);

    // UsecaseMC calls GetRequestInfo first which creats usecaes request object, and then calls ExecuteProcessRequest.
    // Advancedcamerauescase calls ExecuteProcessRequest only, so the uro should be created here.
    if (FALSE == m_pUsecaseBase->IsMultiCameraUsecase())
    {
        m_pUROMapLock->Lock();
        if (m_usecaseRequestObjectMap.find(pRequest->frame_number) == m_usecaseRequestObjectMap.end())
        {
            // Create usecase object is expensive call. Release the lock and reacquire only for insertion
            m_pUROMapLock->Unlock();
            pUsecaseRequestObject = CreateUsecaseRequestObject(pRequest);
            m_pUROMapLock->Lock();
            m_usecaseRequestObjectMap[pRequest->frame_number] = pUsecaseRequestObject;
        }
        else
        {
            CHX_LOG_ERROR("Usecase request object exists for frame number %d", pRequest->frame_number);
            result = CDKResultEExists;
        }
        m_pUROMapLock->Unlock();
    }

    if (CDKResultSuccess == result)
    {
        m_pUROMapLock->Lock();
        pUsecaseRequestObject = m_usecaseRequestObjectMap[pRequest->frame_number];
        m_pUROMapLock->Unlock();
        if (NULL == pUsecaseRequestObject)
        {
            CHX_LOG_ERROR("Usecase request object does not exist for frame number %d", pRequest->frame_number);
            result = CDKResultEFailed;
        }
    }

    // Usecasemc will handle message process
    if (CDKResultSuccess == result &&
        FALSE == m_pUsecaseBase->IsMultiCameraUsecase())
    {
        m_pUsecaseBase->SetShutterTimestamp(pRequest->frame_number, 0);
        if (0 != m_lastZSLFrameNumber)
        {
            pUsecaseRequestObject->SetLastZSLFrameNumber(m_lastZSLFrameNumber);
        }
    }

    // For usecaseMC, get input RDI and FD buffers from usecase and populate into URO
    if ((CDKResultSuccess == result) &&
        (TRUE == m_pUsecaseBase->IsMultiCameraUsecase()))
    {
        UINT32  internalFrameNumber = m_pUsecaseBase->GetFeature2InputFrame(pRequest->frame_number);
        UINT32  masterCameraId      = pUsecaseRequestObject->GetMCCResult()->masterCameraId;

        CHISTREAMBUFFER inputBuffer;
        ChxUtils::Memset(&m_offlinePrivData[fwkFrameIdx], 0, sizeof(CHIPRIVDATA));
        m_offlinePrivData[fwkFrameIdx].featureType      = Feature2;
        // This will get overwritten while sending result back per camera index.
        m_offlinePrivData[fwkFrameIdx].streamIndex      = m_pUsecaseBase->GetPipelineIdFromCamId(masterCameraId);

        std::vector<ChiFeature2UsecaseRequestObjectExtSrcStreamData> inputStreamsInfo =
            pUsecaseRequestObject->GetSelectedInputSrcStreamsInfo();

        // Set streamIndex 0 if it is fusion stream
        if (pRequest->output_buffers->stream->physical_camera_id == NULL)
        {
            m_offlinePrivData[fwkFrameIdx].streamIndex = 0;
        }

        for (UINT32 index = 0; index < inputStreamsInfo.size(); ++index)
        {
            if (NULL != inputStreamsInfo[index].pChiStream)
            {
                switch(inputStreamsInfo[index].pChiStream->format)
                {
                    case ChiStreamFormat::ChiStreamFormatRawOpaque:
                    case ChiStreamFormat::ChiStreamFormatYCbCr420_888:
                    case ChiStreamFormat::ChiStreamFormatRaw16:
                    case ChiStreamFormat::ChiStreamFormatRaw10:
                        result = FillInputBufferMeta(&inputStreamsInfo[index],
                                                     internalFrameNumber,
                                                     fwkFrameNum);
                        CHX_LOG_INFO("%d stream is selected numInputs = %d",
                            inputStreamsInfo[index].pChiStream->format, inputStreamsInfo[index].numInputs);
                        break;
                    default:
                        CHX_LOG_ERROR("unsupported stream format %d", inputStreamsInfo[index].pChiStream->format);
                        break;
                }
            }

            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Fill input buffer and meta failed");
                break;
            }
        }
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObj = NULL;

        m_pUROMapLock->Lock();
        vector<ChiFeature2UsecaseRequestObject*> UROCleanupList;
        for (auto Mapiterator = m_usecaseRequestObjectMap.cbegin(); Mapiterator != m_usecaseRequestObjectMap.cend(); )
        {
            ChiFeature2UsecaseRequestObject* pUsecaseRequestObj = Mapiterator->second;
            UINT                             frameNum           = Mapiterator->first;

            if (frameNum < (pRequest->frame_number))
            {
                if ((NULL != pUsecaseRequestObj) &&
                    (ChiFeature2UsecaseRequestObjectState::Complete == pUsecaseRequestObj->GetRequestState()))
                {
                    pUsecaseRequestObj->ReleaseInputTargetBufferHandle();
                    // Move the URO to invalid, giving one additional EPR call's time for all graph operations to complete
                    pUsecaseRequestObj->SetRequestState(ChiFeature2UsecaseRequestObjectState::Invalid);
                    ++Mapiterator;
                }
                else if ((NULL != pUsecaseRequestObj) &&
                         (ChiFeature2UsecaseRequestObjectState::Invalid == pUsecaseRequestObj->GetRequestState()))
                {
                    UROCleanupList.push_back(pUsecaseRequestObj);
                    Mapiterator = m_usecaseRequestObjectMap.erase(Mapiterator);
                }
                else
                {
                    ++Mapiterator;
                }
            }
            else
            {
                ++Mapiterator;
            }
        }
        m_pUROMapLock->Unlock();

        for (auto pURO : UROCleanupList)
        {
            m_pFGMLock->Lock();
            m_pChiFeatureGraphManager->OnFeatureObjComplete(pURO);
            m_pFGMLock->Unlock();

            m_pMetadataManager->Release(pURO->GetAppSettings());
            pURO->Destroy();
        }

        m_pFGMLock->Lock();
        result = m_pChiFeatureGraphManager->ExecuteProcessRequest(pUsecaseRequestObject);
        m_pFGMLock->Unlock();

        // if it is app reprocess then call EPR again after populating the URO with input buffers
        if (((CDKResultSuccess == result)) && (TRUE == pUsecaseRequestObject->IsAppReprocessRequest()))
        {
            ChiCaptureRequest* pRequest = pUsecaseRequestObject->GetChiCaptureRequest();
            SetupInputConfig(pRequest, pUsecaseRequestObject);

            m_pFGMLock->Lock();
            result = m_pChiFeatureGraphManager->ExecuteProcessRequest(pUsecaseRequestObject);
            m_pFGMLock->Unlock();
        }

        PassSkipPreviewFlagToUsecase(pUsecaseRequestObject, fwkFrameIdx);

        if ((ChiFeature2UsecaseRequestObjectState::OutputPending != pUsecaseRequestObject->GetRequestState()) &&
            (ChiFeature2UsecaseRequestObjectState::Complete != pUsecaseRequestObject->GetRequestState()))
        {
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("FGM ExecuteProcessRequest failed with result:%d! frameNumber=%d pUsecaseRequestObject=%p "
                          "UROState=%d",
                          result,
                          pRequest->frame_number,
                          pUsecaseRequestObject,
                          pUsecaseRequestObject->GetRequestState());
        }
    }

    if (CDKResultSuccess == result)
    {
        CHX_LOG_INFO("ExecuteProcessRequest frameNumber %d SUCCESSFUL!!!!", pRequest->frame_number);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::FillInputBufferMeta
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2Wrapper::FillInputBufferMeta(
    ChiFeature2UsecaseRequestObjectExtSrcStreamData* pExtSrcStreamData,
    UINT32                                           internalFrameNumber,
    UINT32                                           fwkFrameNum)
{
    CDKResult                        result                = CDKResultSuccess;
    UINT32                           fwkFrameIdx           = (fwkFrameNum % MaxOutstandingRequests);
    UINT32                           numPrivDataBufferCnt  = m_offlinePrivData[fwkFrameIdx].numInputBuffers;
    UINT32                           cameraId              = ChxUtils::GetCameraIdFromStream(pExtSrcStreamData->pChiStream);
    UINT32                           pipelineId            = m_pUsecaseBase->GetPipelineIdFromCamId(cameraId);
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObject = NULL;

    m_pUROMapLock->Lock();
    pUsecaseRequestObject = m_usecaseRequestObjectMap[fwkFrameNum];
    m_pUROMapLock->Unlock();

    if (pipelineId >= MaxRealTimePipelines)
    {
        CHX_LOG_ERROR("invalid pipeline id");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < pExtSrcStreamData->numInputs; i++)
        {
            CHISTREAMBUFFER   inputBuffer;
            ChiMetadata*      pSrcMetadata   = NULL;
            CHIBufferManager* pBufferManager = NULL;
            UINT32            rdiInputSeqId  = m_pUsecaseBase->GetRDIInputFrame(internalFrameNumber);
            switch(pExtSrcStreamData->pChiStream->format)
            {
                case ChiStreamFormatRaw10:
                case ChiStreamFormatRaw16:
                case ChiStreamFormatRawOpaque :
                    result = m_pUsecaseBase->GetInputBufferFromRDIQueue(internalFrameNumber,
                                                                        pipelineId,
                                                                        i,
                                                                        &inputBuffer,
                                                                        &pSrcMetadata,
                                                                        TRUE);
                    pBufferManager = m_pUsecaseBase->GetBufferManager(pipelineId);
                    break;
                case ChiStreamFormatYCbCr420_888:
                    result = m_pUsecaseBase->GetInputBufferFromFDQueue(internalFrameNumber,
                                                                       pipelineId,
                                                                       i,
                                                                       &inputBuffer,
                                                                       TRUE);
                    pBufferManager = m_pUsecaseBase->GetFDBufferManager(pipelineId);
                    break;
                default:
                    result = CDKResultEInvalidArg;
                    CHX_LOG_ERROR("Unsupported stream %d", pExtSrcStreamData->pChiStream->format);
                    break;

            }

            if (CDKResultSuccess == result)
            {
                ChiStream*                  pUsecaseStream              = inputBuffer.pStream;
                ChiStream*                  pInternalStream             = m_internalInputStreamMap[pUsecaseStream];
                CHITargetBufferManager*     pRequestInternalInputTBM    = m_frameworkTargetBufferManagers[pInternalStream];
                CHITargetBufferManager*     pRequestInputMetadataTBM    = m_pInputMetadataTBMData[pipelineId];

                CHITARGETBUFFERINFOHANDLE   hOutputTBH                  = NULL;
                CHITARGETBUFFERINFOHANDLE   hMetadataTBH                = NULL;
                UINT32                      seqId                       = rdiInputSeqId - i;
                // Import buffer handle
                result = pRequestInternalInputTBM->ImportExternalTargetBuffer(
                    seqId, reinterpret_cast<UINT64>(pInternalStream), &inputBuffer);

                // Setup Buffer TBH
                hOutputTBH = pRequestInternalInputTBM->SetupTargetBuffer(seqId);

                // Update status to ready
                pRequestInternalInputTBM->UpdateTarget(seqId,
                                                       reinterpret_cast<UINT64>(pInternalStream),
                                                       inputBuffer.bufferInfo.phBuffer,
                                                       ChiTargetStatus::Ready,
                                                       &hOutputTBH);

                CHX_LOG_INFO("Got rdi buffer(seqId=%d) %p TBH %p, pipelineindex:%d", seqId,
                    inputBuffer.bufferInfo.phBuffer, hOutputTBH, pipelineId);

                if (NULL != pSrcMetadata)
                {
                    // Import metadata handle
                    result = pRequestInputMetadataTBM->ImportExternalTargetBuffer(
                        seqId, m_metaClientId, pSrcMetadata);

                    // Setup Metadata TBH
                    hMetadataTBH = pRequestInputMetadataTBM->SetupTargetBuffer(seqId);

                    // Update status to ready
                    pRequestInputMetadataTBM->UpdateTarget(seqId,
                                                           m_metaClientId,
                                                           pSrcMetadata,
                                                           ChiTargetStatus::Ready,
                                                           &hMetadataTBH);

                    CHX_LOG_INFO("Got rdi buffer(seqId=%d) %p meta %p,hmeta:%p pipelineindex:%d", seqId,
                        inputBuffer.bufferInfo.phBuffer, hMetadataTBH, pSrcMetadata->GetHandle(), pipelineId);
                }

                if (TRUE == pUsecaseRequestObject->IsReprocessRequest())
                {
                    result = pUsecaseRequestObject->SetInputTargetBufferHandle(pInternalStream, hMetadataTBH, hOutputTBH);
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("Set input target handle failure");
                    }
                }

                // Saving first input buffer
                if (0 == numPrivDataBufferCnt)
                {
                    m_offlinePrivData[fwkFrameIdx].bufferManagers[i] = pBufferManager;
                    m_offlinePrivData[fwkFrameIdx].inputBuffers[i]   = inputBuffer.bufferInfo;
                }
                else
                {
                    m_offlinePrivData[fwkFrameIdx].bufferManagers[i+ numPrivDataBufferCnt]  = pBufferManager;
                    m_offlinePrivData[fwkFrameIdx].inputBuffers[i + numPrivDataBufferCnt]   = inputBuffer.bufferInfo;

                }

                CHX_LOG_INFO("fwkFrameIdx = %d, priv data = %p buffer = %p for index = %d, pBufferManager=%p",
                             fwkFrameIdx,
                             &m_offlinePrivData[fwkFrameIdx],
                             inputBuffer.bufferInfo.phBuffer, i, pBufferManager);
            }
        }
        m_offlinePrivData[fwkFrameIdx].numInputBuffers  += pExtSrcStreamData->numInputs;;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::ProcessCaptureResultCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::ProcessCaptureResultCb(
    CHICAPTURERESULT*           pResult,
    VOID*                       pPrivateCallbackData)
{
    CAMX_UNREFERENCED_PARAM(pResult);
    CAMX_UNREFERENCED_PARAM(pPrivateCallbackData);

    Feature2Wrapper* pFeature2Wrapper = reinterpret_cast<Feature2Wrapper*>(pPrivateCallbackData);

    pFeature2Wrapper->ProcessResult(pResult, pPrivateCallbackData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::ProcessResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::ProcessResult(
    CHICAPTURERESULT*           pResult,
    VOID*                       pPrivateCallbackData)
{
    CAMX_UNREFERENCED_PARAM(pPrivateCallbackData);

    UINT32                              resultFrameNum          = pResult->frameworkFrameNum;
    UINT32                              resultFrameIndex        = resultFrameNum % MaxOutstandingRequests;
    camera3_capture_result_t*           pUsecaseResult          = m_pUsecaseBase->GetCaptureResult(resultFrameIndex);
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObject   = NULL;

    m_pUROMapLock->Lock();
    pUsecaseRequestObject = m_usecaseRequestObjectMap[resultFrameNum];
    m_pUROMapLock->Unlock();

    CHX_LOG_INFO("Frame %d: outputMetadata: %p, numOutputBuffers %d",
        resultFrameNum, pResult->pOutputMetadata, pResult->numOutputBuffers);

    if (NULL != pUsecaseRequestObject)
    {
        // Handle metadata result
        if ((NULL != pResult->pOutputMetadata) && (FALSE == m_pUsecaseBase->IsMetadataSent(resultFrameIndex)))
        {
            ChiMetadata* pChiOutputMetadata = m_pMetadataManager->GetMetadataFromHandle(pResult->pOutputMetadata);

            pUsecaseResult->partial_result = pResult->numPartialMetadata;
            UINT32* pCameraId = static_cast<UINT32*>(pChiOutputMetadata->GetTag(
                "com.qti.chi.metadataOwnerInfo", "MetadataOwner"));

            if (NULL != pCameraId)
            {
                UINT32 pipelineId = m_pUsecaseBase->GetPipelineIdFromCamId(*pCameraId);
                m_offlinePrivData[resultFrameIndex].streamIndex = pipelineId;
            }

            CHX_LOG_INFO("inputMeta:%p,outputMeta:%p",
                pResult->pInputMetadata, pResult->pOutputMetadata);
            if (FALSE == m_pUsecaseBase->IsMultiCameraUsecase())
            {
                m_pUsecaseBase->SetMetadataAvailable(resultFrameIndex);
                ChxUtils::UpdateTimeStamp(pChiOutputMetadata,
                    m_pUsecaseBase->GetShutterTimestamp(resultFrameNum), resultFrameNum);

                m_pUsecaseBase->UpdateAppResultMetadata(pChiOutputMetadata,
                                                        resultFrameIndex,
                                                        pChiOutputMetadata->GetClientId());

            }
            else
            {
                pResult->pPrivData = &m_offlinePrivData[resultFrameIndex];
                m_pUsecaseBase->ProcessFeatureDone(pResult->frameworkFrameNum, this, pResult);
            }

        }

        // Handle buffer result
        for (UINT32 j = 0; j < pResult->numOutputBuffers; j++)
        {
            ChiStream*              pResultOutputStream = reinterpret_cast<ChiStream*>(pResult->pOutputBuffers[j].pStream);
            CHITargetBufferManager* pResultOutputFwkTBM = m_frameworkTargetBufferManagers[pResultOutputStream];

            CHX_LOG_INFO("Output buffer %d: phBuffer %p, bufferStatus %d, stream %p, format %d, tbm %p", j,
                pResult->pOutputBuffers[j].bufferInfo.phBuffer, pResult->pOutputBuffers[j].bufferStatus, pResultOutputStream,
                pResult->pOutputBuffers[j].pStream->format, pResultOutputFwkTBM);

            if (pResult->pOutputBuffers[j].bufferStatus == 0)
            {
                pResultOutputFwkTBM->UpdateTarget(resultFrameNum,
                                                  reinterpret_cast<UINT64>(pResultOutputStream),
                                                  pResult->pOutputBuffers[j].bufferInfo.phBuffer,
                                                  ChiTargetStatus::Ready,
                                                  NULL);
            }
            else
            {
                pResultOutputFwkTBM->UpdateTarget(resultFrameNum,
                                                  reinterpret_cast<UINT64>(pResultOutputStream),
                                                  pResult->pOutputBuffers[j].bufferInfo.phBuffer,
                                                  ChiTargetStatus::Error,
                                                  NULL);
            }

            if ((pResult->pOutputBuffers[j].pStream->format == ChiStreamFormatYCbCr420_888) &&
                (TRUE == m_pUsecaseBase->IsMultiCameraUsecase()))
            {
                UINT32 numSrcBufs         = pUsecaseRequestObject->GetInputSrcNumBuffersNeeded(NULL);
                BOOL   isFDStreamRequired = pUsecaseRequestObject->IsFDStreamInSelectedSrcStreams();
                CHX_LOG_INFO("Got mc yuv buffer back %d resultFrameIndex  physicalCamIdx = %d, numOfSrcBufs=%d",
                    resultFrameIndex, m_physicalCameraIndex, numSrcBufs);

                UINT32 cameraId   = pUsecaseRequestObject->GetMCCResult()->masterCameraId;
                UINT32 pipelineId = m_pUsecaseBase->GetPipelineIdFromCamId(cameraId);

                if (FALSE == pUsecaseRequestObject->GetMCCResult()->isSnapshotFusion)
                {
                    m_offlinePrivData[resultFrameIndex].streamIndex = pipelineId;
                    m_offlinePrivData[resultFrameIndex].readyMask = ChxUtils::BitSet(
                        m_offlinePrivData[resultFrameIndex].readyMask, pipelineId);
                    CHX_LOG_VERBOSE("got mc %d buffer readyMask:%x",
                        resultFrameNum, m_offlinePrivData[resultFrameIndex].readyMask);
                }
                else
                {
                    m_offlinePrivData[resultFrameIndex].streamIndex = 0;
                }

                pResult->pPrivData = &m_offlinePrivData[resultFrameIndex];

                CHX_LOG_VERBOSE("processresult yuv buffer %p PrivData %p StreamIndex = %d",
                    pResult->pOutputBuffers[0].bufferInfo.phBuffer, pResult->pPrivData,
                    m_offlinePrivData[resultFrameIndex].streamIndex);

                m_pUsecaseBase->ProcessFeatureDone(pResult->frameworkFrameNum, this, pResult);
                if ((TRUE == pUsecaseRequestObject->GetMCCResult()->isSnapshotFusion) &&
                    (static_cast<UINT32>(InputOutputType::YUV_OUT) == m_inputOutputType))
                {
                    // Only release input buffer when all result are back.
                    if (pUsecaseRequestObject->GetMCCResult()->activeMap ==
                        m_offlinePrivData[resultFrameIndex].readyMask)
                    {
                        m_pUsecaseBase->ReleaseReferenceToInputBuffers(static_cast<CHIPRIVDATA*>(pResult->pPrivData));
                        m_offlinePrivData[resultFrameIndex].readyMask = 0;
                        CHX_LOG_VERBOSE("release input buffer for %d", resultFrameNum);
                    }

                    // Update valid buffer length for all active pipeline
                    for (UINT32 i = 0; i < m_pLogicalCameraInfo->numPhysicalCameras; i++)
                    {
                        if (ChxUtils::IsBitSet(pUsecaseRequestObject->GetMCCResult()->activeMap, i))
                        {
                            m_pUsecaseBase->UpdateValidRDIBufferLength(i, numSrcBufs);
                            if (TRUE == isFDStreamRequired)
                            {
                                m_pUsecaseBase->UpdateValidFDBufferLength(i, numSrcBufs);
                            }
                        }
                    }
                }
                else
                {
                    m_pUsecaseBase->ReleaseReferenceToInputBuffers(static_cast<CHIPRIVDATA*>(pResult->pPrivData));
                    m_offlinePrivData[resultFrameIndex].readyMask = 0;

                    if (FALSE == pUsecaseRequestObject->GetMCCResult()->isSnapshotFusion)
                    {
                        if (pipelineId > MaxRealTimePipelines)
                        {
                            CHX_LOG_ERROR("Invalid pipeline id");
                        }
                        else
                        {
                            m_pUsecaseBase->UpdateValidRDIBufferLength(pipelineId, numSrcBufs);
                            if (TRUE == isFDStreamRequired)
                            {
                                m_pUsecaseBase->UpdateValidFDBufferLength(pipelineId, numSrcBufs);
                            }
                        }
                    }
                    else
                    {
                        m_offlinePrivData[resultFrameIndex].streamIndex = 0;
                        for (UINT32 i = 0; i < m_pLogicalCameraInfo->numPhysicalCameras; i++)
                        {
                            if (ChxUtils::IsBitSet(pUsecaseRequestObject->GetMCCResult()->activeMap, i))
                            {
                                m_pUsecaseBase->UpdateValidRDIBufferLength(i, numSrcBufs);
                                if (TRUE == isFDStreamRequired)
                                {
                                    m_pUsecaseBase->UpdateValidFDBufferLength(i, numSrcBufs);
                                }
                            }
                        }
                    }
                    CHX_LOG_VERBOSE("release input buffer for %d", resultFrameNum);
                }
            }
            else if ((pResult->pOutputBuffers[j].pStream->format == ChiStreamFormatImplDefined)  ||
                     (pResult->pOutputBuffers[j].pStream->format == ChiStreamFormatBlob)         ||
                     (pResult->pOutputBuffers[j].pStream->format == ChiStreamFormatYCbCr420_888) ||
                     (pResult->pOutputBuffers[j].pStream->format == ChiStreamFormatRaw10)        ||
                     (pResult->pOutputBuffers[j].pStream->format == ChiStreamFormatRaw16))
            {
                m_pUsecaseBase->GetAppResultMutex()->Lock();
                camera3_stream_buffer_t* pResultBuffer =
                    (camera3_stream_buffer_t*)&pUsecaseResult->output_buffers[pUsecaseResult->num_output_buffers++];

                ChxUtils::PopulateChiToHALStreamBuffer(&pResult->pOutputBuffers[j], pResultBuffer);
                CheckAndSetPreviewBufferSkip(pResultBuffer, pResult->pOutputBuffers[j].pStream->format, resultFrameIndex);
                m_pUsecaseBase->GetAppResultMutex()->Unlock();
            }
        }

        if (FALSE == m_pUsecaseBase->IsMultiCameraUsecase())
        {
            m_pUsecaseBase->ProcessAndReturnFinishedResults();
        }
    }
    else
    {
        CHX_LOG_ERROR("Process result for frame number %d failed! Not able to find usecsase request object", resultFrameNum);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::ProcessMessageCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2Wrapper::ProcessMessageCb(
    const ChiFeature2Messages*  pMessageDescriptor,
    VOID*                       pPrivateCallbackData)
{
    CDKResult result = CDKResultSuccess;
    Feature2Wrapper* pFeature2Wrapper = reinterpret_cast<Feature2Wrapper*>(pPrivateCallbackData);
    if (NULL != pMessageDescriptor->chiNotification.pChiMessages)
    {
        pFeature2Wrapper->ProcessMessage(pMessageDescriptor->chiNotification.pChiMessages, pPrivateCallbackData);
    }
    else if (NULL != pMessageDescriptor->pFeatureMessages)
    {
        if (pMessageDescriptor->pFeatureMessages->messageType == ChiFeature2MessageType::MessageNotification)
        {
            pFeature2Wrapper->ProcessFeatureToChiMessage(pMessageDescriptor, pPrivateCallbackData);
        }
        else if (pMessageDescriptor->pFeatureMessages->messageType == ChiFeature2MessageType::SubmitRequestNotification)
        {
            ChiPipelineRequest submitRequest = pMessageDescriptor->pFeatureMessages->message.submitRequest;
            result = pFeature2Wrapper->m_pUsecaseBase->SubmitRequest(&submitRequest);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::ProcessFeatureToChiMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::ProcessFeatureToChiMessage(
    const ChiFeature2Messages* pMessageDescriptor,
    VOID*                      pPrivateCallbackData)
{
    CAMX_UNREFERENCED_PARAM(pPrivateCallbackData);
    ChiFeature2ChiMessageNotification notificationData = pMessageDescriptor->pFeatureMessages->message.notificationData;

    if (notificationData.messageType == ChiFeature2ChiMessageType::AnchorPick)
    {
        UINT anchorFrameIdx = notificationData.message.anchorData.anchorFrameIdx;

        if (TRUE == m_pUsecaseBase->IsMultiCameraUsecase())
        {
            m_pUsecaseBase->ProcessFeatureDataNotify(0, this, &anchorFrameIdx);
        }
    }
    if (notificationData.messageType == ChiFeature2ChiMessageType::ZSL)
    {
        UINT32 frameNumber = notificationData.message.ZSLData.lastFrameNumber;

        CHX_LOG_INFO("ZSL Last timestamp picked %d", frameNumber);

        m_lastZSLFrameNumber = frameNumber;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::ProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::ProcessMessage(
    const ChiMessageDescriptor* pMessageDescriptor,
    VOID*                       pPrivateCallbackData)
{
    CAMX_UNREFERENCED_PARAM(pPrivateCallbackData);

    if (NULL != pMessageDescriptor)
    {
        if (ChiMessageTypeSof == pMessageDescriptor->messageType)
        {
            CHX_LOG("SOF: frameNum %d fwkFrameNumber %d timestamp %" PRIu64,
                    pMessageDescriptor->message.sofMessage.sofId,
                    pMessageDescriptor->message.sofMessage.frameworkFrameNum,
                    pMessageDescriptor->message.sofMessage.timestamp);
        }
        else if (ChiMessageTypeMetaBufferDone == pMessageDescriptor->messageType)
        {
            CHX_LOG("MetaBufferDone: fwkFrameNumber %u i/p metadata %p o/p metadata %p",
                    pMessageDescriptor->message.metaBufferDoneMessage.frameworkFrameNum,
                    pMessageDescriptor->message.metaBufferDoneMessage.inputMetabuffer,
                    pMessageDescriptor->message.metaBufferDoneMessage.outputMetabuffer);
        }
        else if (ChiMessageTypeShutter == pMessageDescriptor->messageType)
        {
            if (FALSE == m_pUsecaseBase->IsMultiCameraUsecase())
            {
                if (0 == m_pUsecaseBase->GetShutterTimestamp(pMessageDescriptor->message.shutterMessage.frameworkFrameNum))
                {
                    m_pUsecaseBase->SetShutterTimestamp(pMessageDescriptor->message.shutterMessage.frameworkFrameNum,
                        pMessageDescriptor->message.shutterMessage.timestamp);
                }

                CHX_LOG("Shutter: fwkFrameNumber %d timestamp:%" PRIu64,
                        pMessageDescriptor->message.shutterMessage.frameworkFrameNum,
                        pMessageDescriptor->message.shutterMessage.timestamp);

                m_pUsecaseBase->ReturnFrameworkMessage(
                    // NOWHINE CP036a:
                    (reinterpret_cast<camera3_notify_msg_t*>(const_cast<ChiMessageDescriptor *>(pMessageDescriptor))),
                    m_pUsecaseBase->GetCameraId());
            }
        }
        else if (ChiMessageTypeError == pMessageDescriptor->messageType)
        {
            if (FALSE == m_isFlush)
            {
                CHX_LOG_ERROR("Error: fwkFrameNumber %d error message type:%s",
                    pMessageDescriptor->message.errorMessage.frameworkFrameNum,
                    ChxUtils::ErrorMessageCodeToString(pMessageDescriptor->message.errorMessage.errorMessageCode));
            }
            switch (pMessageDescriptor->message.errorMessage.errorMessageCode)
            {
                case MessageCodeDevice:
                case MessageCodeTriggerRecovery:
                case MessageCodeRequest:
                case MessageCodeBuffer:
                case MessageCodeResult:
                    m_pUsecaseBase->ProcessErrorMessage(pMessageDescriptor);
                    break;
                default:
                    CHX_LOG_WARN("Unhandled error type:%d", pMessageDescriptor->message.errorMessage.errorMessageCode);
                    break;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::ProcessPartialCaptureResultCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::ProcessPartialCaptureResultCb(
    ChiPartialCaptureResult*    pResult,
    VOID*                       pPrivateCallbackData)
{
    Feature2Wrapper* pFeature2Wrapper = reinterpret_cast<Feature2Wrapper*>(pPrivateCallbackData);

    pFeature2Wrapper->ProcessDriverPartialCaptureResult(pResult, pPrivateCallbackData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::ProcessDriverPartialCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::ProcessDriverPartialCaptureResult(
    CHIPARTIALCAPTURERESULT*    pResult,
    VOID*                       pPrivateCallbackData)
{
    CAMX_UNREFERENCED_PARAM(pPrivateCallbackData);

    UINT32 resultFrameNum   = pResult->frameworkFrameNum;
    UINT32 resultFrameIndex = resultFrameNum % MaxOutstandingRequests;

    // Handle partial metadata
    if (NULL != pResult->pPartialResultMetadata)
    {
        ChiMetadata* pPartialResultMetadata = m_pMetadataManager->GetMetadataFromHandle(pResult->pPartialResultMetadata);

        PartialResultSender sender = PartialResultSender::DriverPartialData;

        m_pPartialMetadataLock->Lock();
        if (TRUE == m_pUsecaseBase->CheckIfPartialDataCanBeSent(sender, resultFrameIndex))
        {
            m_pUsecaseBase->UpdateAppPartialResultMetadataFromDriver(pPartialResultMetadata,
                                                                     resultFrameIndex,
                                                                     resultFrameNum,
                                                                     pPartialResultMetadata->GetClientId());

            m_pUsecaseBase->ProcessAndReturnPartialMetadataFinishedResults(sender);
        }
        m_pPartialMetadataLock->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::ProcessCHIPartialData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::ProcessCHIPartialData(
    UINT32    frameNum,
    UINT32    sessionId)
{
    CAMX_UNREFERENCED_PARAM(frameNum);
    CAMX_UNREFERENCED_PARAM(sessionId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::CreateFeatureGraphManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2Wrapper::CreateFeatureGraphManager()
{
    CDKResult                       result                  = CDKResultSuccess;
    ChiFeature2GraphManager*        pChiFeatureGraphManager = NULL;
    UINT32                          streamCt = 0;
    UINT32                          numStreams              = m_pFrameworkStreamConfig->numStreams +
                                                              m_internalInputStreamMap.size();
    ChiStream*                      pStreams[numStreams];
    ChiStreamConfigInfo             streamConfig;
    FeatureGraphManagerConfig       fgmConfig;
    ChiFeature2WrapperCallbacks     featureWrapperCallbacks;

    // Internal streams
    for (auto const& it : m_internalInputStreamMap)
    {
        pStreams[streamCt] = it.second;
        CHX_LOG("Internal input stream[%d]=%p", streamCt, pStreams[streamCt]);
        streamCt++;
    }

    // Framework streams
    for (UINT32 i = 0; i < m_pFrameworkStreamConfig->numStreams; i++)
    {
        pStreams[streamCt] = m_pFrameworkStreamConfig->pChiStreams[i];
        CHX_LOG("framework stream[%d]=%p", streamCt, pStreams[streamCt]);
        streamCt++;
    }

    streamConfig.numStreams       = numStreams;
    streamConfig.operationMode    = m_pFrameworkStreamConfig->operationMode;
    streamConfig.pChiStreams      = &pStreams[0];
    streamConfig.pSessionSettings = NULL;
    streamConfig.pSessionSettings = m_pFrameworkStreamConfig->pSessionSettings;

    fgmConfig.pCameraStreamConfig = &streamConfig;
    fgmConfig.pCameraInfo         = m_pLogicalCameraInfo;
    fgmConfig.pSessionParams      = 0;
    fgmConfig.pMetadataManager    = m_pMetadataManager;

    featureWrapperCallbacks.ProcessCaptureResultCbToUsecase         = ProcessCaptureResultCb;
    featureWrapperCallbacks.NotifyMessageToUsecase                  = ProcessMessageCb;
    featureWrapperCallbacks.ProcessPartialCaptureResultCbToUsecase  = ProcessPartialCaptureResultCb;
    featureWrapperCallbacks.pPrivateCallbackData                    = this;

    fgmConfig.pFeatureWrapperCallbacks  = &featureWrapperCallbacks;
    fgmConfig.pUsecaseDescriptor        = m_pChiUsecase;
    fgmConfig.inputOutputType           = m_inputOutputType;

    m_pChiFeatureGraphManager = ChiFeature2GraphManager::Create(&fgmConfig);

    CHX_LOG_INFO("m_pChiFeatureGraphManager=%p", m_pChiFeatureGraphManager);

    if (NULL == m_pChiFeatureGraphManager)
    {
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::CreateTargetBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature2Wrapper::CreateTargetBufferManagers()
{
    CDKResult                           result      = CDKResultSuccess;
    ChiTargetBufferManagerCreateData    tbmCreateData;
    CHAR                                tbmName[MaxStringLength256];

    for (auto const& it : m_internalInputStreamMap)
    {
        ChiStream* pChiStream = it.second;

        tbmCreateData = { 0 };
        CdkUtils::SNPrintF(tbmName, sizeof(tbmName), "InternalIn_%p_TBM", pChiStream);

        tbmCreateData.pTargetBufferName            = tbmName;
        tbmCreateData.numOfMetadataBuffers         = 0;
        tbmCreateData.numOfInternalStreamBuffers   = 0;
        tbmCreateData.numOfExternalStreamBuffers   = 1;
        tbmCreateData.minExternalBufferCount       = 1;
        tbmCreateData.maxExternalBufferCount       = pChiStream->maxNumBuffers;
        tbmCreateData.externalStreamIds[0]         = reinterpret_cast<UINT64>(pChiStream);
        tbmCreateData.isChiFenceEnabled            = ExtensionModule::GetInstance()->EnableTBMChiFence();

        CHITargetBufferManager* pChiTargetBufferManager = CHITargetBufferManager::Create(&tbmCreateData);

        if (NULL != pChiTargetBufferManager)
        {
            m_frameworkTargetBufferManagers[pChiStream] = pChiTargetBufferManager;

            CHX_LOG("Internal input Target Buffer Manager [%s][%p] created. stream=%p type=%d format=%d",
                tbmName, pChiTargetBufferManager, pChiStream, pChiStream->streamType, pChiStream->format);
        }
        else
        {
            CHX_LOG_ERROR("Create framework target buffer manager failed!");
            result = CDKResultEFailed;
            break;
        }
    }

    for (UINT32 i = 0; i < m_pFrameworkStreamConfig->numStreams; i++)
    {
        tbmCreateData = { 0 };

        CdkUtils::SNPrintF(tbmName, sizeof(tbmName), "FWK_%d_TBM", m_pFrameworkStreamConfig->pChiStreams[i]->format);

        tbmCreateData.pTargetBufferName            = tbmName;
        tbmCreateData.numOfMetadataBuffers         = 0;
        tbmCreateData.numOfInternalStreamBuffers   = 0;
        tbmCreateData.numOfExternalStreamBuffers   = 1;
        tbmCreateData.maxExternalBufferCount       = m_pFrameworkStreamConfig->pChiStreams[i]->maxNumBuffers;
        tbmCreateData.externalStreamIds[0]         =
            reinterpret_cast<UINT64>(m_pFrameworkStreamConfig->pChiStreams[i]);
        tbmCreateData.isChiFenceEnabled            = ExtensionModule::GetInstance()->EnableTBMChiFence();

        CHITargetBufferManager* pChiTargetBufferManager = CHITargetBufferManager::Create(&tbmCreateData);

        if (NULL != pChiTargetBufferManager)
        {
            m_frameworkTargetBufferManagers[m_pFrameworkStreamConfig->pChiStreams[i]] = pChiTargetBufferManager;

            CHX_LOG("Framework Target Buffer Manager [%s][%p] created. stream=%p type=%d format=%d",
                tbmName, pChiTargetBufferManager,
                m_pFrameworkStreamConfig->pChiStreams[i],
                m_pFrameworkStreamConfig->pChiStreams[i]->streamType,
                m_pFrameworkStreamConfig->pChiStreams[i]->format);
        }
        else
        {
            CHX_LOG_ERROR("Create framework target buffer manager failed!");
            result = CDKResultEFailed;
            break;
        }
    }

    // Create a metadata manager to import RDI metadata into

    CHITargetBufferManager* pTargetBufferManager = NULL;

    ChiTargetBufferManagerCreateData    targetBufferCreateData      = { 0 };
    CHAR                                metadataManagerName[100]    = { 0 };

    CdkUtils::SNPrintF(metadataManagerName, sizeof(metadataManagerName), "Feature2WrapperInputMetadata");

    // No client registered for metadata, using default value of 0 for metaclient Id as this will
    // always use imported buffers
    for (UINT8 physicalCamIdx = 0 ; physicalCamIdx < m_pLogicalCameraInfo->numPhysicalCameras; ++physicalCamIdx)
    {
        targetBufferCreateData.pTargetBufferName                    = metadataManagerName;
        targetBufferCreateData.numOfMetadataBuffers                 = 1;
        targetBufferCreateData.metadataIds[0]                       = m_metaClientId;
        targetBufferCreateData.pChiMetadataManager                  = m_pMetadataManager;
        targetBufferCreateData.numOfInternalStreamBuffers           = 0;
        targetBufferCreateData.numOfExternalStreamBuffers           = 0;
        targetBufferCreateData.minMetaBufferCount                   = 1;
        targetBufferCreateData.maxMetaBufferCount                   = 16;
        targetBufferCreateData.isChiFenceEnabled                    = ExtensionModule::GetInstance()->EnableTBMChiFence();

        pTargetBufferManager = CHITargetBufferManager::Create(&targetBufferCreateData);

        if (NULL != pTargetBufferManager)
        {
            CHX_LOG_INFO("Wrapper meta target buffer created %p ,physicalCamIdx:%d",
                pTargetBufferManager, physicalCamIdx);
            m_pInputMetadataTBMData.push_back(pTargetBufferManager);
        }
        else
        {
            CHX_LOG_ERROR("Failed to create metadata target buffer manager");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::CreateUsecaseRequestObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2UsecaseRequestObject* Feature2Wrapper::CreateUsecaseRequestObject(
    camera3_capture_request_t* pRequest)
{
    CDKResult                                       result = CDKResultSuccess;
    ChiFeature2UsecaseRequestObjectCreateInputInfo  pCreateInputInfo;
    ChiFeature2UsecaseRequestObject*                pUsecaseRequestObject = NULL;

    if (pRequest != NULL)
    {
        pCreateInputInfo.pRequest           = pRequest;

        if ((TRUE == m_pUsecaseBase->IsMultiCameraUsecase()))
        {
            pCreateInputInfo.reprocessFlag    = TRUE;
            pCreateInputInfo.appReprocessFlag = FALSE;
        }
        else if (NULL != pRequest->input_buffer)
        {
            pCreateInputInfo.appReprocessFlag = TRUE;
        }
        else
        {
            pCreateInputInfo.reprocessFlag    = FALSE;
            pCreateInputInfo.appReprocessFlag = FALSE;
        }

        if (TRUE == m_pUsecaseBase->IsMultiCameraUsecase())
        {
            pCreateInputInfo.pAppSettings = m_pMetadataManager->Get(
                m_pUsecaseBase->GetGenericMetadataClientId(), pRequest->frame_number);
            if (NULL != pCreateInputInfo.pAppSettings)
            {
                pCreateInputInfo.pAppSettings->SetAndroidMetadata(pRequest->settings);
            }
            else
            {
                CHX_LOG_WARN("Get Application setting failed!");
            }
        }
        else
        {
            if (TRUE == pCreateInputInfo.reprocessFlag)
            {
                pCreateInputInfo.pAppSettings = m_pMetadataManager->GetInput(
                    pRequest->settings, pRequest->frame_number, FALSE, FALSE);
            }
            else
            {
                pCreateInputInfo.pAppSettings = m_pMetadataManager->GetInput(
                    pRequest->settings, pRequest->frame_number);
            }
        }

        if (TRUE == pCreateInputInfo.reprocessFlag)
        {
            pCreateInputInfo.pStatusMetadata = m_pMetadataManager->GetMetadataFromHandle(
                m_pUsecaseBase->GetAvialableResultMetadata());
        }
        else
        {
            pCreateInputInfo.pStatusMetadata = NULL;
        }

        for (auto const& it : m_internalInputStreamMap)
        {
            pCreateInputInfo.inputSrcStreams.push_back(it.second);
        }

        pCreateInputInfo.MCCResult = GetMCCResult(&m_vendorTagOps, pRequest->settings, m_pLogicalCameraInfo);

        pUsecaseRequestObject = ChiFeature2UsecaseRequestObject::Create(&pCreateInputInfo);

        if (NULL != pUsecaseRequestObject)
        {
            // @todo (CAMX-1234) handle pRequest->input_buffer

            for (UINT32 i = 0; i < pRequest->num_output_buffers; i++)
            {
                ChiStream*                  pRequestOutputStream = reinterpret_cast<ChiStream*>(
                    pRequest->output_buffers[i].stream);
                CHITargetBufferManager*     pRequestOutputFwkTBM = m_frameworkTargetBufferManagers[pRequestOutputStream];
                CHISTREAMBUFFER             outputChiStreamBuffer;
                CHITARGETBUFFERINFOHANDLE   hOutputTBH;

                CHX_LOG_VERBOSE("bufferIdx:%d, pStream:%p, cameraId:%s",
                    i, pRequestOutputStream, pRequestOutputStream->physicalCameraId);


                if (NULL == pRequestOutputFwkTBM)
                {
                    CHX_LOG_ERROR("Not able to find target buffer manager for stream %p!", pRequestOutputStream);
                    result = CDKResultEFailed;
                    break;
                }

                ChxUtils::PopulateHALToChiStreamBuffer(&pRequest->output_buffers[i], &outputChiStreamBuffer);

                if ((TRUE == m_pUsecaseBase->IsMultiCameraUsecase()) &&
                    (static_cast<UINT32>(InputOutputType::YUV_OUT) != m_inputOutputType))
                {
                    // Overwrite the buffer type because the yuv buffer manager in usecaseMC provides
                    // the YUV output buffer of the ChiNative type.
                    outputChiStreamBuffer.bufferInfo.bufferType = ChiNative;
                }

                result = pRequestOutputFwkTBM->ImportExternalTargetBuffer(
                    pRequest->frame_number, reinterpret_cast<UINT64>(pRequestOutputStream), &outputChiStreamBuffer);
                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Import fwk output buffer of stream %p into TBM %p failed!",
                                  pRequestOutputStream, pRequestOutputFwkTBM);
                    break;
                }

                hOutputTBH = pRequestOutputFwkTBM->SetupTargetBuffer(pRequest->frame_number);
                if (NULL == hOutputTBH)
                {
                    CHX_LOG_ERROR("Setup target buffer for frame number %d on TBM %p failed!",
                                  pRequest->frame_number, pRequestOutputFwkTBM);
                    result = CDKResultEFailed;
                    break;
                }

                result = pUsecaseRequestObject->SetOutputTargetBufferHandle(pRequestOutputStream, hOutputTBH);
                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("Set output target buffer handle %p to usecase request object %p failed!",
                                  hOutputTBH, pUsecaseRequestObject);
                    break;
                }
            }

            if (CDKResultSuccess == result)
            {
                CHX_LOG_INFO("frame_number=%d pUsecaseRequestObject=%p",
                             pRequest->frame_number, pUsecaseRequestObject);
            }

        }
        else
        {
            CHX_LOG_ERROR("Create usecase request object failed! FrameNumber=%d", pRequest->frame_number);
        }

    }

    return pUsecaseRequestObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::SetupInputConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::SetupInputConfig(
    ChiCaptureRequest*               pRequest,
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObject)
{
    CDKResult result = CDKResultSuccess;

    // for input buffer in the request
    ChiStream*                  pRequestInputStream  = pRequest->pInputBuffers->pStream;
    CHITargetBufferManager*     pRequestInputFwkTBM  = m_frameworkTargetBufferManagers[pRequestInputStream];
    CHISTREAMBUFFER             inputChiStreamBuffer = pRequest->pInputBuffers[0];
    CHITARGETBUFFERINFOHANDLE   hInputTBH            = NULL;
    CHITARGETBUFFERINFOHANDLE   hMetadataTBH         = NULL;

    CHX_LOG_INFO("pStream:%p, cameraId:%s",
                 pRequestInputStream, pRequestInputStream->physicalCameraId);

    if (NULL == pRequestInputFwkTBM)
    {
        CHX_LOG_ERROR("Not able to find target buffer manager for stream %p!", pRequestInputStream);
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        result = pRequestInputFwkTBM->ImportExternalTargetBuffer(
            pRequest->frameNumber, reinterpret_cast<UINT64>(pRequestInputStream), &inputChiStreamBuffer);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Import fwk output buffer of stream %p into TBM %p failed!",
                          pRequestInputStream, pRequestInputFwkTBM);
        }
    }

    if (CDKResultSuccess == result)
    {
        hInputTBH = pRequestInputFwkTBM->SetupTargetBuffer(pRequest->frameNumber);
        if (NULL == hInputTBH)
        {
            CHX_LOG_ERROR("Setup target buffer for frame number %" PRIu64 " on TBM %p failed!",
                          pRequest->frameNumber, pRequestInputFwkTBM);
            result = CDKResultEFailed;
        }
        else
        {
            // Update status to ready
            pRequestInputFwkTBM->UpdateTarget(
                pRequest->frameNumber,
                reinterpret_cast<UINT64>(pRequestInputStream),
                inputChiStreamBuffer.bufferInfo.phBuffer,
                ChiTargetStatus::Ready,
                NULL);
        }
    }

    if (CDKResultSuccess == result)
    {
        UINT32 cameraId = m_pLogicalCameraInfo->ppDeviceInfo[0]->cameraId;

        UINT32 pipelineId = m_pUsecaseBase->GetPipelineIdFromCamId(cameraId);

        CHITargetBufferManager* pRequestInputMetadataTBM = m_pInputMetadataTBMData[pipelineId];

        // Import metadata handle
        result = pRequestInputMetadataTBM->ImportExternalTargetBuffer(
            pRequest->frameNumber, m_metaClientId, pUsecaseRequestObject->GetAppSettings());

        // Setup Metadata TBH
        hMetadataTBH = pRequestInputMetadataTBM->SetupTargetBuffer(pRequest->frameNumber);

        // Update status to ready
        pRequestInputMetadataTBM->UpdateTarget(pRequest->frameNumber,
            m_metaClientId, pUsecaseRequestObject->GetAppSettings(), ChiTargetStatus::Ready, NULL);

        result = pUsecaseRequestObject->SetInputTargetBufferHandle(pRequestInputStream, hMetadataTBH, hInputTBH);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Set output target buffer handle %p to usecase request object %p failed!",
                          hInputTBH, pUsecaseRequestObject);
        }
    }

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::Flush()
{
    if (NULL != m_pChiFeatureGraphManager)
    {
        m_isFlush = TRUE;
        m_pChiFeatureGraphManager->Flush();
        m_isFlush = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::GetMaxRequiredFramesForSnapshot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Feature2Wrapper::GetMaxRequiredFramesForSnapshot(
    const camera_metadata_t* pMetadata)
{
    CDK_UNREFERENCED_PARAM(pMetadata);

    UINT numOfHwmfFrames = ExtensionModule::GetInstance()->ForceHWMFFixedNumOfFrames();
    UINT numOfSwmfFrames = ExtensionModule::GetInstance()->ForceSWMFFixedNumOfFrames();
    UINT numOfFrames     = 3;

    if ((0 != numOfHwmfFrames) && (0 != numOfSwmfFrames))
    {
        UINT numOfForceFrames     = ChxUtils::MaxUINT(numOfHwmfFrames, numOfSwmfFrames);
        if (numOfForceFrames > 3)
        {
            // Max Buffers are required in HWMFNR/swmfnr + HDR.
            numOfFrames = numOfForceFrames + 2;
        }
        else
        {
            // Max Buffers are required in HWMFNR/SWMF + HDR.
            // Minimum Multiframe is used 3 for HWMFNR/SWMF.
            numOfFrames = numOfFrames + 2;
        }
    }
    else
    {
        // Max Buffers are required in HWMFNR + HDR which is 8+2.
        numOfFrames = MaxFrameCntNeededForFeature2 + 2;
    }

    return numOfFrames;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::PassSkipPreviewFlagToUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::PassSkipPreviewFlagToUsecase(
    ChiFeature2UsecaseRequestObject*    pRequestObject,
    UINT32                              resultFrameIndex)
{
    if ((NULL != pRequestObject) &&
        (TRUE == pRequestObject->GetSkipPreviewFlag()))
    {
        m_pUsecaseBase->SetSkipPreviewFlagInRequestMapInfo(resultFrameIndex, TRUE);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Feature2Wrapper::CheckAndSetPreviewBufferSkip
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature2Wrapper::CheckAndSetPreviewBufferSkip(
    camera3_stream_buffer_t*    pCamera3StreamBuffer,
    UINT64                      format,
    UINT32                      resultFrameIndex)
{
    if ((TRUE == m_pUsecaseBase->GetSkipPreviewFlagInRequestMapInfo(resultFrameIndex)) &&
        (TRUE == IsPreviewStreamFormat(format)) &&
        (NULL != pCamera3StreamBuffer))
    {
        ChxUtils::SkipFrame(pCamera3StreamBuffer);
        m_pUsecaseBase->SetSkipPreviewFlagInRequestMapInfo(resultFrameIndex, FALSE);

        CHX_LOG_INFO("Skip the preview frame");
    }

}
