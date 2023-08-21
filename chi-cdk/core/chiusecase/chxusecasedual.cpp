////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecasedual.cpp
/// @brief Usecases class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <vector>
#include <cstdlib>
#include <cstring>

#include "chxusecase.h"
#include "chxusecasedual.h"
#include "chistatspropertydefines.h"
#include "chxutils.h"
#include "chxmulticamcontroller.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::~UsecaseDualCamera
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseDualCamera::~UsecaseDualCamera()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UsecaseDualCamera* UsecaseDualCamera::Create(
    LogicalCameraInfo*              cameraInfo,    ///< Camera info
    camera3_stream_configuration_t* pStreamConfig) ///< Stream configuration
{
    CDKResult             result                = CDKResultSuccess;
    UsecaseDualCamera* pUsecaseDualCamera = CHX_NEW UsecaseDualCamera;

    if (NULL != pUsecaseDualCamera)
    {
        result = pUsecaseDualCamera->Initialize(cameraInfo, pStreamConfig);

        if (CDKResultSuccess != result)
        {
            pUsecaseDualCamera->Destroy(FALSE);
            pUsecaseDualCamera = NULL;
        }
    }
    else
    {
        result = CDKResultEFailed;
    }

    return pUsecaseDualCamera;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseDualCamera::Destroy(BOOL isForced)
{
    for (UINT i = 0 ; i < MaxSessions; i++)
    {
        if (NULL != m_pSession[i])
        {
            m_pSession[i]->Destroy(isForced);
            m_pSession[i] = NULL;
        }
    }

    for (UINT i = 0 ; i < MaxPipelinesPerSession; i++)
    {
        if (NULL != m_pPipeline[i])
        {
            m_pPipeline[i]->Destroy();
            m_pPipeline[i] = NULL;
        }
    }

    for (UINT i = 0 ; i < MAX_REALTIME_PIPELINE ; i++)
    {
        for (UINT j = 0; j < MAX_NUM_YUV_STREAMS; j++) {
            if (NULL != m_pDummyYuvStream[i][j])
            {
                ChxUtils::Free(m_pDummyYuvStream[i][j]);
                m_pDummyYuvStream[i][j] = NULL;
            }
        }
        if (NULL != m_pDummyRawStream[i])
        {
            ChxUtils::Free(m_pDummyRawStream[i]);
            m_pDummyRawStream[i] = NULL;
        }
        if (NULL != m_pDummyJpegStream[i])
        {
            ChxUtils::Free(m_pDummyJpegStream[i]);
            m_pDummyJpegStream[i] = NULL;
        }

        for (UINT j = 0; j < m_pInternalStreams[i].size(); j++) {
            ChxUtils::Free(m_pInternalStreams[i][j]);
            m_pInternalStreams[i][j] = NULL;

        }
        m_pInternalStreams[i].clear();

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::SessionCbNotifyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseDualCamera::SessionCbNotifyMessage(
    const ChiMessageDescriptor* pMessageDescriptor,
    VOID*                       pPrivateCallbackData)
{
    SessionPrivateData* pSessionPrivateData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
    UsecaseDualCamera* pUsecase            = static_cast<UsecaseDualCamera*>(pSessionPrivateData->pUsecase);

    if (NULL == pMessageDescriptor || ChiMessageTypeSof == pMessageDescriptor->messageType)
    {
        // SOF notifications are not sent to the HAL3 application and so ignore
        return;
    }

    // Shutter message from logical camera pipeline of realtime session
    if (ChiMessageTypeShutter == pMessageDescriptor->messageType)
    {
        auto& pendingChiResults = pUsecase->mPendingChiResults;
        UINT32 frameNumber = pMessageDescriptor->message.shutterMessage.frameworkFrameNum;
        PendingChiResult& pendingResult = pendingChiResults.at(frameNumber);

        std::unique_lock<std::mutex> lock(pUsecase->mPendingResultLock);
        CHIPRIVDATA* pPrivData = static_cast<CHIPRIVDATA*>(pMessageDescriptor->pPrivData);
        if (pendingResult.logicalPipelineId == pPrivData->streamIndex)
        {
            pUsecase->ReturnFrameworkMessage((camera3_notify_msg_t*)pMessageDescriptor,
                    pUsecase->m_cameraId);
        }
    } else {
        // Error message should propagate regardless.
        pUsecase->ReturnFrameworkMessage((camera3_notify_msg_t*)pMessageDescriptor,
                pUsecase->m_cameraId);
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::SessionCbCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseDualCamera::SessionCbCaptureResult(
    ChiCaptureResult* pResult,
    VOID*             pPrivateCallbackData)
{
    SessionPrivateData* pSessionPrivateData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
    UsecaseDualCamera*  pUsecase            = static_cast<UsecaseDualCamera*>(pSessionPrivateData->pUsecase);
    auto&               pendingChiResults   = pUsecase->mPendingChiResults;
    UINT32              pipelineIndex;
    camera_metadata_t*  pPhyFrameworkOutput[2] = { NULL };

    if (NULL != pResult->pPrivData)
    {
        pipelineIndex = static_cast<CHIPRIVDATA*>(pResult->pPrivData)->streamIndex;
    }
    else
    {
        CHX_LOG("Result: Cannot Accept NULL private data here for = %d", pResult->frameworkFrameNum);
        return;
    }

    CHX_LOG_ERROR("Result Session: %d Pipeline: %d FrameNum: %d o/p buffers: %d, ResultMeta: %p, PartialMeta: %d",
        pSessionPrivateData->sessionID, pipelineIndex,
        pResult->frameworkFrameNum, pResult->numOutputBuffers,
        pResult->pOutputMetadata, pResult->numPartialMetadata);

    if (CDKResultSuccess != pUsecase->ModifyCaptureResultStream(pResult)) {
        CHX_LOG_ERROR("Result: Failed to modify capture result stream for logical stream");
        return;

    }

    UINT32 halCameraId = pUsecase->m_pCameraInfo->ppDeviceInfo[pipelineIndex]->cameraId;
    UINT32 frameworkCameraId = ExtensionModule::GetInstance()->RemapCameraId(
            halCameraId, IdReverseMapCamera);
    std::string frameworkCameraIdStr = std::to_string(frameworkCameraId);

    // release input
    if (NULL != pResult->pInputMetadata)
    {
        pUsecase->m_pMetadataManager->Release(
            pUsecase->m_pMetadataManager->GetMetadataFromHandle(pResult->pInputMetadata));
    }

    if (pResult->pOutputMetadata == NULL) {
        CHX_ASSERT(pResult->numPartialMetadata == 0);
        camera3_capture_result_t *result = pUsecase->GetCaptureResult(pResult->frameworkFrameNum);;
        result->frame_number = pResult->frameworkFrameNum;
        result->num_output_buffers = pResult->numOutputBuffers;
        camera3_stream_buffer_t *pAppResultBuffer         =
            (camera3_stream_buffer_t *)ChxUtils::Calloc(sizeof(camera3_stream_buffer_t) * pResult->numOutputBuffers);

        if ((NULL == pAppResultBuffer) || (result->num_output_buffers == 0))
        {
           CHX_LOG_ERROR("ERROR: no memory! or number of output buffers are zero");
           return;
        }

        for (UINT32 j = 0; j < result->num_output_buffers; j++)
        {
            ChxUtils::PopulateChiToHALStreamBuffer(&pResult->pOutputBuffers[j], &pAppResultBuffer[j]);
        }
        result->output_buffers = reinterpret_cast<camera3_stream_buffer_t *>(pAppResultBuffer);
        result->input_buffer = NULL;
        result->partial_result = 0;
        result->result = NULL;
        result->num_physcam_metadata = 0;
        pUsecase->ReturnFrameworkResult(
                reinterpret_cast<const camera3_capture_result_t*>(result),
                pUsecase->m_cameraId);
        free(pAppResultBuffer);
        result->output_buffers = NULL;
    } else {
        CHX_ASSERT(pResult->numPartialMetadata == 1);
        std::unique_lock<std::mutex> lock(pUsecase->mPendingResultLock);
        PendingChiResult& pendingResult = pendingChiResults.at(pResult->frameworkFrameNum);

        // Accumulate capture results from individual pipelines, and send out
        // once all are ready.
        BOOL finalResultReady =
                pendingResult.handleCaptureResult(pUsecase, *pResult, frameworkCameraIdStr);
        if (finalResultReady) {

            if (CDKResultSuccess != pUsecase->RemapLogicalResultMetadata(
                    static_cast<ChiMetadata *>(pendingResult.logicalMetadata),
                    pendingResult.logicalPipelineId))
            {
                CHX_LOG_ERROR("Error: Failed to remap logical result metadata");
                return;
            }

            camera_metadata_t* pFrameworkOutput = pUsecase->m_pMetadataManager->GetAndroidFrameworkOutputMetadata();
            if (NULL != pFrameworkOutput)
            {
                (reinterpret_cast<ChiMetadata *>(pendingResult.logicalMetadata))->TranslateToCameraMetadata(
                    pFrameworkOutput);
            }

            camera3_capture_result_t    *result           = pUsecase->GetCaptureResult(pResult->frameworkFrameNum);
            result->frame_number                          = pResult->frameworkFrameNum;
            result->result                                = pFrameworkOutput;
            result->num_output_buffers                    = pendingResult.outputBuffers.size();
            result->output_buffers                        = NULL;
            camera3_stream_buffer_t     *pAppResultBuffer = NULL;
            if (result->num_output_buffers != 0)
            {
                CHISTREAMBUFFER *pResultBuffer         =
                        reinterpret_cast<CHISTREAMBUFFER *>(
                        pendingResult.outputBuffers.data());
                camera3_stream_buffer_t *pAppResultBuffer         =
                    (camera3_stream_buffer_t *)ChxUtils::Calloc(sizeof(camera3_stream_buffer_t) * result->num_output_buffers);
                if ((NULL != pAppResultBuffer) && (result->num_output_buffers != 0))
                {
                    for (UINT32 j = 0; j < result->num_output_buffers; j++)
                    {
                        ChxUtils::PopulateChiToHALStreamBuffer(&pResultBuffer[j], &pAppResultBuffer[j]);
                    }
                    result->output_buffers = reinterpret_cast<camera3_stream_buffer_t *>(pAppResultBuffer);
                }
                else
                {
                    CHX_LOG_ERROR("Error: pAppResultBuffer is NULL. Result output buffers not set. FrameNum: %u",
                        pResult->frameworkFrameNum);
                }
            }
            result->input_buffer = (pendingResult.inputBuffer.size() == 0) ? NULL :
                    reinterpret_cast<camera3_stream_buffer_t *>(pendingResult.inputBuffer.data());
            result->partial_result = 2;

            result->num_physcam_metadata = pendingResult.physicalMetadata.size();
            if (result->num_physcam_metadata) {
                result->physcam_ids = (const char**) new const char *[result->num_physcam_metadata];
                if (result->physcam_ids)
                {
                    for (UINT32 i = 0; i < result->num_physcam_metadata; i++)
                    {
                        result->physcam_ids[i] = pendingResult.physicalIds[i].c_str();
                    }
                    result->physcam_metadata = const_cast<const camera_metadata_t **>(
                    reinterpret_cast<camera_metadata_t **>(pendingResult.physicalMetadata.data()));
                    for (UINT32 i = 0; i < pendingResult.physicalMetadata.size(); i++)
                    {
                        pPhyFrameworkOutput[i] = reinterpret_cast<camera_metadata_t *>(pendingResult.physicalMetadata[i]);
                    }
                }
            }

            CHX_LOG_ERROR("Result Session: frameNumber %d, num_physcam_metadata %d output = %d",
                          result->frame_number, result->num_physcam_metadata, result->num_output_buffers);
            pUsecase->ReturnFrameworkResult(result, pUsecase->m_cameraId);

            if (result->physcam_ids) {
                delete[] result->physcam_ids;
            }
            // Remove the final result from the map now that all physical
            // results are available.
            if (pendingResult.logicalMetadata != NULL) {
                pendingResult.logicalMetadata = NULL;
            }

            for (UINT32 i = 0; i < result->num_physcam_metadata; i++) {
                if (pendingResult.physicalMetadata[i] != NULL)
                {
                    pendingResult.physicalMetadata[i] = NULL;
                }
            }
            pendingChiResults.erase(pResult->frameworkFrameNum);
            if (NULL != pAppResultBuffer)
            {
                free(pAppResultBuffer);
                result->output_buffers = NULL;
            }

            // Release Framework outputs
            if (NULL != pFrameworkOutput)
            {
                pUsecase->m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(pFrameworkOutput);
            }
            if (NULL != pPhyFrameworkOutput[0])
            {
                pUsecase->m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(pPhyFrameworkOutput[0]);
            }
            if (NULL != pPhyFrameworkOutput[1])
            {
                pUsecase->m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(pPhyFrameworkOutput[1]);
            }
        }
        pUsecase->m_pMetadataManager->Release(
            pUsecase->m_pMetadataManager->GetMetadataFromHandle(pResult->pOutputMetadata));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::ProcessAndReturnPartialMetadataFinishedResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseDualCamera::ProcessAndReturnPartialMetadataFinishedResults(PartialResultSender partialResultSender)
{
    camera3_capture_result_t *pPartialResult        = NULL;
    camera3_capture_result_t *pPartialResultTest    = NULL;
    CDKResult                 result                = CDKResultSuccess;

    if (PartialResultSender::DriverMetaData == partialResultSender)
    {
        result = CDKResultEFailed;
    }

    if ((PartialResultSender::CHIPartialData == partialResultSender) &&
        (PartialMetaSupport::CombinedPartialMeta == ExtensionModule::GetInstance()->EnableCHIPartialData()))
    {
        result = CDKResultEFailed;
    }

    if (result == CDKResultSuccess)
    {
        m_pAppResultMutex->Lock();

        for (INT64 resultFrame = m_nextAppResultFrame; resultFrame <= m_lastAppRequestFrame; resultFrame++)
        {
            UINT frameIndex = ChxUtils::GetResultFrameIndexChi(resultFrame);

            pPartialResult  = (PartialResultSender::DriverPartialData == partialResultSender) ?
                &m_driverPartialCaptureResult[frameIndex] : &m_chiPartialCaptureResult[frameIndex];

            BOOL metadataAvailable  = ((NULL != pPartialResult->result) &&
                (0 != pPartialResult->partial_result)) ? TRUE : FALSE;

            if (TRUE == metadataAvailable)
            {
                if ((PartialResultSender::DriverPartialData == partialResultSender) &&
                    (PartialMetaSupport::CombinedPartialMeta == ExtensionModule::GetInstance()->EnableCHIPartialData()))
                {
                    // We need to merge the meta data
                    camera3_capture_result_t *pCHIPartialResult     = &m_chiPartialCaptureResult[frameIndex];
                    camera_metadata_t        *pCHIResultMetadata    = const_cast<camera_metadata_t*>(pCHIPartialResult->result);
                    camera_metadata_t        *pResultMetadata       = const_cast<camera_metadata_t*>(pPartialResult->result);

                    ChxUtils::AndroidMetadata::MergeMetadata(static_cast<VOID*>(pCHIResultMetadata),
                                            static_cast<VOID*>(pResultMetadata));

                    CHX_LOG("Partial Result is merged");
                }

                camera3_capture_result_t result     = { 0 };
                result.frame_number                 = pPartialResult->frame_number;
                result.result                       = pPartialResult->result;
                result.partial_result               =
                    static_cast<int>(ChxUtils::GetPartialResultCount(partialResultSender));

                CHX_LOG("Frame %" PRIu64 ": Returning Partial metadata result %d "
                    "for app frame %d m_nextAppResultFrame=%" PRIu64 " m_lastAppRequestFrame=%" PRIu64 ,
                    resultFrame,
                    result.partial_result,
                    result.frame_number,
                    m_nextAppResultFrame,
                    m_lastAppRequestFrame);

                Usecase::ReturnFrameworkResult(&result, m_cameraId);

                if (NULL != m_driverPartialCaptureResult[frameIndex].result)
                {
                    m_pMetadataManager->ReleaseAndroidFrameworkOutputMetadata(
                        m_driverPartialCaptureResult[frameIndex].result);
                }

                // Reset the partial meta related info in corresponding partial capture result
                pPartialResult->result           = NULL;
                pPartialResult->partial_result   = 0;

            }
        }
        m_pAppResultMutex->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::ProcessDriverPartialCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseDualCamera::ProcessDriverPartialCaptureResult(
    CHIPARTIALCAPTURERESULT*    pResult)
{
    UINT32       pipelineIndex;
    UINT32       internalFrameNum       = pResult->frameworkFrameNum;
    UINT32       internalFrameNumIndex  = internalFrameNum %  MaxOutstandingRequests;
    ChiMetadata* pChiOutputMetadata     = NULL;
    CDKResult    result                 = CDKResultSuccess;

    PartialResultSender  sender         = PartialResultSender::DriverPartialData;

    if (NULL == pResult->pPrivData)
    {
        CHX_LOG_ERROR("Result: Cannot Accept NULL private data here for = %d", pResult->frameworkFrameNum);
        // Early return
        result = CDKResultEFailed;
    }
    else
    {
        pipelineIndex = static_cast<CHIPRIVDATA*>(pResult->pPrivData)->streamIndex;
    }

    if (result == CDKResultSuccess)
    {
        CHX_LOG_ERROR("PMDDEBUG:pipelineindex:%d masterpipelinelindex:%d",
            pipelineIndex,
            REALTIMEPIPELINEMAIN);

        if (pipelineIndex == REALTIMEPIPELINEMAIN)
        {
            pChiOutputMetadata = m_pMetadataManager->GetMetadataFromHandle(pResult->pPartialResultMetadata);
            // This check is to ensure that we have not sent earlier
            if (TRUE == CheckIfPartialDataCanBeSent(sender, internalFrameNumIndex))
            {
                CHX_LOG_ERROR("PMDDEBUG:Driver partial meta data can be sent");
                if (PartialMetaSupport::CombinedPartialMeta == ExtensionModule::GetInstance()->EnableCHIPartialData())
                {
                    ProcessCHIPartialData(pResult->frameworkFrameNum,0);
                }
                UpdateAppPartialResultMetadataFromDriver(pChiOutputMetadata,
                                                         internalFrameNumIndex,
                                                         internalFrameNum,
                                                         m_pPipeline[pipelineIndex]->GetMetadataClientId());
                ProcessAndReturnPartialMetadataFinishedResults(sender);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::RealtimeCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseDualCamera::ProcessCHIPartialData(
    UINT32    frameNum,
    UINT32    sessionId)
{
    CAMX_UNREFERENCED_PARAM(frameNum);
    CAMX_UNREFERENCED_PARAM(sessionId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::SessionCbCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseDualCamera::SessionCbPartialCaptureResult(
    ChiPartialCaptureResult* pPartialCaptureResult,
    VOID*                    pPrivateCallbackData)
{
    SessionPrivateData* pSessionPrivateData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
    UsecaseDualCamera*  pUsecase            = static_cast<UsecaseDualCamera*>(pSessionPrivateData->pUsecase);
    UINT32 pipelineIndex;

    if (NULL == pPartialCaptureResult->pPrivData)
    {
        CHX_LOG_ERROR("Result: Cannot Accept NULL private data here for = %d", pPartialCaptureResult->frameworkFrameNum);
        // Early return
        return;
    }
    else
    {
        pipelineIndex = static_cast<CHIPRIVDATA*>(pPartialCaptureResult->pPrivData)->streamIndex;
    }

    if (REALTIMESESSIONIDX == pSessionPrivateData->sessionID)
    {
        CHX_LOG_ERROR("PMDDEBUG: Real time partial capture result coming for sessionid:%d", pSessionPrivateData->sessionID);
        pUsecase->ProcessDriverPartialCaptureResult(pPartialCaptureResult);
    }
    else
    {
        CHX_LOG_ERROR("PMDDEBUG:NOT HANDLING Real time partial capture result coming for sessionid:%d",
            pSessionPrivateData->sessionID);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseDualCamera::Initialize(
    LogicalCameraInfo*              pCameraInfo,   ///< Camera info
    camera3_stream_configuration_t* pStreamConfig) ///< Stream configuration
{
    CDKResult result        = CDKResultSuccess;
    ChiUsecase* pChiUsecase = NULL;
    m_pCameraInfo = pCameraInfo;


    ChiPortBufferDescriptor pipelineOutputBuffer[MaxChiStreams];
    ChiPortBufferDescriptor pipelineInputBuffer[MaxChiStreams];

    ChxUtils::Memset(pipelineOutputBuffer, 0, sizeof(pipelineOutputBuffer));
    ChxUtils::Memset(pipelineInputBuffer, 0, sizeof(pipelineInputBuffer));
    m_usecaseId = UsecaseId::MultiCamera;
    m_cameraId = pCameraInfo->cameraId;
    m_HasPhysicalStream = FALSE;

    m_LogicalCameraPipelineId = REALTIMEPIPELINEMAIN;
    for (UINT i = 0; i < MAX_REALTIME_PIPELINE; i++)
    {
        m_IsFirstRequest[i] = TRUE;
        m_effectModeValue[i] = ANDROID_CONTROL_EFFECT_MODE_OFF;
        m_sceneModeValue[i] = ANDROID_CONTROL_SCENE_MODE_DISABLED;
        m_tuningFeature1Value[i] = 0;
        m_tuningFeature2Value[i] = 0;
    }

    result      = Usecase::Initialize();
    if (CDKResultSuccess == result)
    {
        result = CreateMetadataManager(m_cameraId, false, NULL, true);
    }

    m_shutterFrameNum = -1;

    // Construct MultiCamControllerManager instance to be able to use vendor tag
    // ops.
    MultiCamControllerManager::GetInstance();

    pChiUsecase   = &(Usecases8Target[UsecaseDualCameraId]);
    if (NULL != pChiUsecase)
    {
        CHX_LOG("USECASE Name: %s, pipelines: %d, numTargets: %d",
            pChiUsecase->pUsecaseName, pChiUsecase->numPipelines, pChiUsecase->numTargets);

        for (UINT i = 0; i < pChiUsecase->numPipelines; i++)
        {
            for (UINT j = 0; j < MAX_NUM_YUV_STREAMS; j++)
            {
                m_pDummyYuvStream[i][j] = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
                m_pDummyYuvStream[i][j]->streamType = ChiStreamTypeOutput;
                m_pDummyYuvStream[i][j]->width = DUMMYWIDTH;
                m_pDummyYuvStream[i][j]->height = DUMMYHEIGHT;
                m_pDummyYuvStream[i][j]->format = ChiStreamFormatYCrCb420_SP;
                m_pDummyYuvStream[i][j]->grallocUsage = GrallocUsageSwReadOften;
                m_pDummyYuvStream[i][j]->maxNumBuffers = 1;
                m_pDummyYuvStream[i][j]->pPrivateInfo = NULL;
                m_pDummyYuvStream[i][j]->physicalCameraId = NULL;
            }

            m_pDummyRawStream[i] = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
            m_pDummyRawStream[i]->streamType = ChiStreamTypeOutput;
            m_pDummyRawStream[i]->width = pCameraInfo->m_cameraCaps.sensorCaps.activeArray.width;
            m_pDummyRawStream[i]->height = pCameraInfo->m_cameraCaps.sensorCaps.activeArray.height;
            m_pDummyRawStream[i]->format = ChiStreamFormatRaw10;
            m_pDummyRawStream[i]->grallocUsage = GrallocUsageSwReadOften;
            m_pDummyRawStream[i]->maxNumBuffers = 1;
            m_pDummyRawStream[i]->pPrivateInfo = NULL;
            m_pDummyRawStream[i]->physicalCameraId = NULL;

            m_pDummyJpegStream[i] = static_cast<CHISTREAM*>(ChxUtils::Calloc(sizeof(CHISTREAM)));
            m_pDummyJpegStream[i]->streamType = ChiStreamTypeOutput;
            m_pDummyJpegStream[i]->width = DUMMYWIDTH;
            m_pDummyJpegStream[i]->height = DUMMYHEIGHT;
            m_pDummyJpegStream[i]->format = ChiStreamFormatBlob;
            m_pDummyJpegStream[i]->grallocUsage = GrallocUsageSwReadOften;
            m_pDummyJpegStream[i]->maxNumBuffers = 1;
            m_pDummyJpegStream[i]->pPrivateInfo = NULL;
            m_pDummyJpegStream[i]->physicalCameraId = NULL;

            CHX_LOG("PIPELINE[%d] Name: %s, Source Target num: %d, Sink Target Num: %d",
                i,
                pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName,
                pChiUsecase->pPipelineTargetCreateDesc[i].sourceTarget.numTargets,
                pChiUsecase->pPipelineTargetCreateDesc[i].sinkTarget.numTargets);
        }

        for (UINT stream = 0; stream < pStreamConfig->num_streams; stream++)
        {
            CHX_LOG("stream = %p streamType = %d streamFormat = %d streamWidth = %d streamHeight = %d physicalcameraid %s",
                pStreamConfig->streams[stream],
                pStreamConfig->streams[stream]->stream_type,
                pStreamConfig->streams[stream]->format,
                pStreamConfig->streams[stream]->width,
                pStreamConfig->streams[stream]->height,
                pStreamConfig->streams[stream]->physical_camera_id);
        }

        result = MatchUsecaseChiStreams(pChiUsecase, pStreamConfig);
        if (CDKResultSuccess != result)
        {
            return result;
        }

        for (UINT i = 0; i < pChiUsecase->numPipelines; i++)
        {
            ChiTargetPortDescriptorInfo* pSinkTarget = &pChiUsecase->pPipelineTargetCreateDesc[i].sinkTarget;
            ChiTargetPortDescriptorInfo* pSrcTarget  = &pChiUsecase->pPipelineTargetCreateDesc[i].sourceTarget;
            CHX_LOG("pipeline Index:%d,targetNum:%d,sourceNums:%d",i,pSinkTarget->numTargets,
                pSrcTarget->numTargets);

            // Remap pipeline Index from usecase.
            UINT32 remappedPipelineIndex = RemapPipelineIndex(i);
            for (UINT sinkIdx = 0; sinkIdx < pSinkTarget->numTargets; sinkIdx++)
            {
                ChiTargetPortDescriptor* pSinkTargetDesc   = &pSinkTarget->pTargetPortDesc[sinkIdx];
                pipelineOutputBuffer[sinkIdx].pStream      = pSinkTargetDesc->pTarget->pChiStream;
                pipelineOutputBuffer[sinkIdx].pNodePort    = pSinkTargetDesc->pNodePort;
                pipelineOutputBuffer[sinkIdx].numNodePorts = pSinkTargetDesc->numNodePorts;

                if (pSinkTargetDesc->pTarget->pChiStream->physicalCameraId != NULL &&
                        pSinkTargetDesc->pTarget->pChiStream->physicalCameraId[0] != '\0') {
                    m_HasPhysicalStream = TRUE;
                }
            }

            for (UINT sourceIdx = 0; sourceIdx < pSrcTarget->numTargets; sourceIdx++)
            {
                ChiTargetPortDescriptor* pSrcTargetDesc     = &pSrcTarget->pTargetPortDesc[sourceIdx];
                pipelineInputBuffer[sourceIdx].pStream      = pSrcTargetDesc->pTarget->pChiStream;
                pipelineInputBuffer[sourceIdx].pNodePort    = pSrcTargetDesc->pNodePort;
                pipelineInputBuffer[sourceIdx].numNodePorts = pSrcTargetDesc->numNodePorts;
            }

            m_pPipeline[remappedPipelineIndex] = Pipeline::Create(
                    pCameraInfo->ppDeviceInfo[remappedPipelineIndex]->cameraId, PipelineType::Default);

            if (NULL != m_pPipeline[remappedPipelineIndex])
            {
                m_pPipeline[remappedPipelineIndex]->SetOutputBuffers(pSinkTarget->numTargets, &pipelineOutputBuffer[0]);
                m_pPipeline[remappedPipelineIndex]->SetInputBuffers(pSrcTarget->numTargets, &pipelineInputBuffer[0]);
                m_pPipeline[remappedPipelineIndex]->SetPipelineNodePorts(
                    &pChiUsecase->pPipelineTargetCreateDesc[i].pipelineCreateDesc);
                m_pPipeline[remappedPipelineIndex]->SetPipelineName(pChiUsecase->pPipelineTargetCreateDesc[i].pPipelineName);

                result = m_pPipeline[remappedPipelineIndex]->CreateDescriptor();
            }
        }
    }
    else
    {
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        ChiCallBacks callbacks = { 0 };
        ///< create realtime session
        callbacks.ChiNotify                        = SessionCbNotifyMessage;
        callbacks.ChiProcessCaptureResult          = SessionCbCaptureResult;
        callbacks.ChiProcessPartialCaptureResult   = SessionCbPartialCaptureResult;

        m_perSessionPvtData[REALTIMESESSIONIDX].sessionID    = REALTIMESESSIONIDX;
        m_perSessionPvtData[REALTIMESESSIONIDX].pUsecase     = this;
        m_pSession[REALTIMESESSIONIDX] = Session::Create(&m_pPipeline[0], MAX_REALTIME_PIPELINE, &callbacks,
        &m_perSessionPvtData[REALTIMESESSIONIDX]);

        if (NULL == m_pSession[REALTIMESESSIONIDX])
        {
            CHX_LOG("Failed to creat realtime Session with multiple pipeline");
            result = CDKResultEFailed;
        }
        if (!this->m_pMetadataManager)
        {
            CHX_LOG("Failed to create metadata manager");
            result = CDKResultEFailed;
        }
        else
        {
            UINT32 clientid = m_pMetadataManager->RegisterClient(
            m_pPipeline[REALTIMEPIPELINEMAIN]->IsRealTime(),
            m_pPipeline[REALTIMEPIPELINEMAIN]->GetTagList(),
            m_pPipeline[REALTIMEPIPELINEMAIN]->GetTagCount(),
            m_pPipeline[REALTIMEPIPELINEMAIN]->GetPartialTagCount(),
            m_pPipeline[REALTIMEPIPELINEMAIN]->GetMetadataBufferCount() + BufferQueueDepth,
            ChiMetadataUsage::RealtimeOutput);
            m_pPipeline[REALTIMEPIPELINEMAIN]->SetMetadataClientId(clientid);
            clientid = m_pMetadataManager->RegisterClient(
            m_pPipeline[REALTIMEPIPELINEAUX]->IsRealTime(),
            m_pPipeline[REALTIMEPIPELINEAUX]->GetTagList(),
            m_pPipeline[REALTIMEPIPELINEAUX]->GetTagCount(),
            m_pPipeline[REALTIMEPIPELINEAUX]->GetPartialTagCount(),
            m_pPipeline[REALTIMEPIPELINEAUX]->GetMetadataBufferCount() + BufferQueueDepth,
            ChiMetadataUsage::RealtimeOutput);
            m_pPipeline[REALTIMEPIPELINEAUX]->SetMetadataClientId(clientid);
            UINT32 frameworkBufferCount = m_pPipeline[REALTIMEPIPELINEMAIN]->GetMetadataBufferCount() +
            m_pPipeline[REALTIMEPIPELINEAUX]->GetMetadataBufferCount() + 2 * BufferQueueDepth;
            m_pMetadataManager->InitializeFrameworkInputClient(frameworkBufferCount, true);
        }
    }

    // Internal streams' grallocUsage and maxNumBuffers are populated by CamX
    // during session creation. So we need to assign the values back to
    // framework camera3_stream after session creation.
    if (CDKResultSuccess == result)
    {
        for (UINT streamId = 0; streamId < pStreamConfig->num_streams; streamId++)
        {
            ChiStream* pStream = reinterpret_cast<ChiStream*>(pStreamConfig->streams[streamId]);
            UINT32 pipelineId = GetUsecasePipelineId(pStream->physicalCameraId);

            if (pipelineId == REALTIMEPIPELINELOGICAL)
            {
                auto remappedStream = m_IncomingStreamMap.find(pStream);
                if (remappedStream == m_IncomingStreamMap.end())
                {
                    CHX_LOG_ERROR("Failed to find internal stream for streamId %d", streamId);
                    result = CDKResultEFailed;
                    break;
                }
                // Assumption is that matching pair of internal streams have the same
                // grallocUsage and maxNumBuffers.
                pStream->grallocUsage =
                    m_pInternalStreams[0][remappedStream->second]->grallocUsage;
                pStream->maxNumBuffers =
                    m_pInternalStreams[0][remappedStream->second]->maxNumBuffers;
            }
            CHX_LOG("stream[%d]: width %d, height %d, usage 0x%x, format 0x%x, max_buffers %d,"
                 " physical_id %s", streamId, pStream->width, pStream->height,
                pStream->grallocUsage, pStream->format, pStream->maxNumBuffers,
                pStream->physicalCameraId);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::ExecuteCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseDualCamera::ExecuteCaptureRequest(
    camera3_capture_request_t* pRequest)
{
    CDKResult result = CDKResultSuccess;
    UINT32 frameIndex = (pRequest->frame_number % MaxOutstandingRequests);
    m_shutterTimestamp[frameIndex]                 = 0;

    // Request to generate preview for now
    result = GenerateRealtimeRequest(pRequest);

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::ProcessResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UsecaseDualCamera::ProcessResults()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::RemapPipelineIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 UsecaseDualCamera::RemapPipelineIndex(
    UINT32 pipelineIndexFromUsecase)
{
    UINT32 pipelineIndex = 0;
    switch (pipelineIndexFromUsecase)
    {
        case Realtime0:
            pipelineIndex = REALTIMEPIPELINEMAIN;
            break;
        case Realtime1:
            pipelineIndex = REALTIMEPIPELINEAUX;
            break;
        default:
            CHX_LOG_ERROR("ERROR: Pipeline index %d is not expected", pipelineIndexFromUsecase);
            break;
    }
    return pipelineIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::GetUsecasePipelineId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 UsecaseDualCamera::GetUsecasePipelineId(
    const char* physicalCameraIdStr)
{
    errno = 0;
    char* endPtr;
    UINT32 usecasePipelineId = REALTIMEPIPELINELOGICAL;

    // Convert physical camera id from string to integer
    if (!physicalCameraIdStr)
    {
        CHX_LOG_ERROR("physical_camera_id is NULL");
        return usecasePipelineId;
    }
    if (strlen(physicalCameraIdStr) == 0)
    {
        // This is a logic camera id, use logical camera pipeline id
        return usecasePipelineId;
    }
    UINT32 physicalCameraId = strtol(physicalCameraIdStr, &endPtr, 10);
    if (errno || (endPtr == physicalCameraIdStr))
    {
        CHX_LOG_ERROR("physical_camera_id '%s' is malformed", physicalCameraIdStr);
        return usecasePipelineId;
    }

    // Map framework camera id to HAL internal camera id, and find
    // MAIN/AUX/Logical pipeline id
    UINT32 halCameraId = ExtensionModule::GetInstance()->RemapCameraId(
            physicalCameraId, IdRemapCamera);
    for (UINT32 pipelineId = 0; pipelineId < m_pCameraInfo->numPhysicalCameras; pipelineId++)
    {
        if (m_pCameraInfo->ppDeviceInfo[pipelineId]->cameraId == halCameraId) {
            return pipelineId;
        }
    }

    CHX_LOG_ERROR("Error: Cannot find pipeline id for physical camera %s", physicalCameraIdStr);
    return usecasePipelineId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::MatchUsecaseChiStreams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseDualCamera::MatchUsecaseChiStreams(
        const ChiUsecase* pChiUsecase,
        const camera3_stream_configuration_t* pStreamConfig)
{
    UINT numStreams = pStreamConfig->num_streams;
    UINT numYuvStreams[MAX_REALTIME_PIPELINE] = {0, 0};
    UINT numRawStreams[MAX_REALTIME_PIPELINE] = {0, 0};

    // Reset pChiStream to NULL for all targets
    for (UINT i = 0; i < pChiUsecase->numPipelines; i++)
    {
        ChiTargetPortDescriptorInfo* pSinkTarget =
                &pChiUsecase->pPipelineTargetCreateDesc[i].sinkTarget;
        for (UINT sinkIdx = 0; sinkIdx < pSinkTarget->numTargets; sinkIdx++)
        {
            ChiTarget* pTargetInfo = pSinkTarget->pTargetPortDesc[sinkIdx].pTarget;
            pTargetInfo->pChiStream = NULL;
        }
    }

    // Match HAL configured streams with topology
    for (UINT streamId = 0; streamId < numStreams; streamId++)
    {
        ChiStream* pStream = reinterpret_cast<ChiStream*>(pStreamConfig->streams[streamId]);
        UINT streamType = pStream->streamType;
        UINT32 streamWidth  = pStream->width;
        UINT32 streamHeight = pStream->height;
        UINT32 pipelineId  = GetUsecasePipelineId(pStream->physicalCameraId);

        for (UINT i = 0; i < pChiUsecase->numPipelines; i++)
        {
            if (pipelineId != i && pipelineId != REALTIMEPIPELINELOGICAL)
            {
                // Current stream doesn't request on this pipeline.
                continue;
            }

            // Fail the stream combinations not yet supported by DualCamera topology
            if (UsecaseSelector::IsPreviewStream(pStreamConfig->streams[streamId]) ||
                    UsecaseSelector::IsVideoStream(pStreamConfig->streams[streamId]) ||
                    UsecaseSelector::IsYUVSnapshotStream(pStreamConfig->streams[streamId]))
            {
                numYuvStreams[i]++;
            }
            else if (UsecaseSelector::IsRawStream(pStreamConfig->streams[streamId]))
            {
                numRawStreams[i]++;
            }
            if (numYuvStreams[i] > MAX_NUM_YUV_STREAMS || numRawStreams[i] > 1)
            {
                CHX_LOG_ERROR("Only support %d (%d) YUV streams and 1 (%d) Raw streams per pipeline",
                        numYuvStreams[i], MAX_NUM_YUV_STREAMS, numRawStreams[i]);
                return CDKResultEFailed;
            }

            UINT32 ri = RemapPipelineIndex(i);
            ChiTargetPortDescriptorInfo* pSinkTarget =
                    &pChiUsecase->pPipelineTargetCreateDesc[ri].sinkTarget;
            BOOL formatMatched = FALSE;
            for (UINT sinkIdx = 0; sinkIdx < pSinkTarget->numTargets; sinkIdx++)
            {
               ChiTarget* pTargetInfo = pSinkTarget->pTargetPortDesc[sinkIdx].pTarget;
               if (NULL != pTargetInfo->pChiStream)
               {
                   // This target has been taken.
                   continue;
               }
               formatMatched = UsecaseSelector::IsMatchingFormat(
                       pStream,
                       pTargetInfo->numFormats,
                       pTargetInfo->pBufferFormats);
               if (TRUE == formatMatched)
               {
                   formatMatched = ((streamType == static_cast<UINT>(pTargetInfo->direction)) ? TRUE : FALSE);
               }

               if (TRUE == formatMatched)
               {
                   BufferDimension* pRange = &pTargetInfo->dimension;

                   if ((streamWidth  >= pRange->minWidth)  &&
                           (streamWidth  <= pRange->maxWidth)  &&
                           (streamHeight >= pRange->minHeight) &&
                           (streamHeight <= pRange->maxHeight))
                   {
                       formatMatched = TRUE;
                   }
                   else
                   {
                       formatMatched = FALSE;
                   }
               }

               if (TRUE == formatMatched)
               {
                   if (pipelineId == REALTIMEPIPELINELOGICAL) {
                       CHISTREAM* chiStream = static_cast<CHISTREAM*>(
                           ChxUtils::Calloc(sizeof(CHISTREAM)));
                       ChxUtils::Memcpy(chiStream, pStream, sizeof(CHISTREAM));
                       m_IncomingStreamMap[pStream] = m_pInternalStreams[i].size();
                       m_OutgoingStreamMap[chiStream] = pStream;

                       m_pInternalStreams[i].push_back(chiStream);
                       pTargetInfo->pChiStream = chiStream;
                   }
                   else {
                       pTargetInfo->pChiStream = pStream;

                   }

                   CHX_LOG_INFO("Matched stream id %d with pipeline %d, target id %d",
                           streamId, i, sinkIdx);
                   break;
               }
            }
            if (FALSE == formatMatched)
            {
                CHX_LOG_ERROR("Fatal: Cannot find matching sinkTargets combination");
                return CDKResultEFailed;
            }
        }
    }

    // Use dummy stream to fill out rest of the targets
    for (UINT i = 0; i < pChiUsecase->numPipelines; i++)
    {
        ChiTargetPortDescriptorInfo* pSinkTarget =
                &pChiUsecase->pPipelineTargetCreateDesc[i].sinkTarget;
        UINT numDummyYuvStreams = 0;
        for (UINT sinkIdx = 0; sinkIdx < pSinkTarget->numTargets; sinkIdx++)
        {
            ChiTarget* pTargetInfo = pSinkTarget->pTargetPortDesc[sinkIdx].pTarget;
            if (NULL == pTargetInfo->pChiStream)
            {
                if (pTargetInfo->numFormats >= 1 &&
                        ((pTargetInfo->pBufferFormats[0] == ChiFormatRawMIPI) ||
                         (pTargetInfo->pBufferFormats[0] == ChiFormatRawPlain16)))
                {
                    pTargetInfo->pChiStream = m_pDummyRawStream[i];
                } else if (pTargetInfo->numFormats >= 1 &&
                        (pTargetInfo->pBufferFormats[0] == ChiFormatJpeg ||
                         pTargetInfo->pBufferFormats[0] == ChiFormatBlob)) {
                    pTargetInfo->pChiStream = m_pDummyJpegStream[i];
                } else {
                    pTargetInfo->pChiStream = m_pDummyYuvStream[i][numDummyYuvStreams];
                    numDummyYuvStreams++;
                }
            }
        }
    }

    return CDKResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::GenerateInternalRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CDKResult UsecaseDualCamera::GenerateInternalRequest(
    UINT32 sessionId, UINT numRequest, CHICAPTUREREQUEST* pRequest)
{
    CDKResult         result = CDKResultSuccess;

    CHIPIPELINEREQUEST submitRequest;

    submitRequest.pSessionHandle   = reinterpret_cast<CHIHANDLE>(m_pSession[sessionId]->GetSessionHandle());
    submitRequest.numRequests      = numRequest;
    submitRequest.pCaptureRequests = pRequest;
    result = SubmitRequest(&submitRequest);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::GenerateRealtimeRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseDualCamera::GenerateRealtimeRequest(
    camera3_capture_request_t* pRequest)
{
    CDKResult         result     = CDKResultSuccess;

    UINT8 cnt_outbuffer0                        = 0;
    UINT8 cnt_outbuffer1                        = 0;
    ChiMetadata* pChiRTMainInputMetadata        = NULL;
    ChiMetadata* pChiMainOutputMetadata         = NULL;
    ChiMetadata* pChiRTAuxInputMetadata         = NULL;
    ChiMetadata* pChiAuxOutputMetadata          = NULL;
    int metadataOutput[MAX_REALTIME_PIPELINE]   = { MetadataOutputNone, MetadataOutputNone };
    UINT32  camID;
    std::unordered_set<UINT32> requestedPipelineIds;

    std::vector<CHICAPTUREREQUEST> chiRequests;
    CHICAPTUREREQUEST *request = NULL;
    const camera_metadata_t* physicalSettings[MAX_REALTIME_PIPELINE] = {};

    // Validate dual camera related fields in camera3_capture_request_t
    for (uint32_t i = 0; i < pRequest->num_physcam_settings; i++) {
        if ((pRequest->settings == nullptr && pRequest->physcam_settings[i] != nullptr) ||
                (pRequest->settings != nullptr && pRequest->physcam_settings[i] == nullptr)) {
            CHX_LOG_ERROR("Request : frameNum :%d logical setting and physical settings must be"
                    " null/non-null at the same time", pRequest->frame_number);
            result = CDKResultEInvalidArg;
            return result;
        }

        if (pRequest->physcam_id[i] == nullptr) {
            CHX_LOG_ERROR("Request : frameNum :%d physcam_id[%d] is null",
                    pRequest->frame_number, i);
            result = CDKResultEInvalidArg;
            return result;
        }
        if (pRequest->physcam_settings[i] == nullptr) {
            continue;
        }
        if (get_camera_metadata_entry_count(pRequest->physcam_settings[i]) == 0) {
            CHX_LOG_ERROR("Request : frameNum :%d physcam_settings[%d] is invalid",
                    pRequest->frame_number, i);
            result = CDKResultEInvalidArg;
            return result;
        }

        UINT32 physicalCameraId = atoi(pRequest->physcam_id[i]);
        UINT32 halPhysicalCameraId = ExtensionModule::GetInstance()->RemapCameraId(
                physicalCameraId, IdRemapCamera);
        UINT32 j = 0;
        for (j = 0; j < m_pCameraInfo->numPhysicalCameras; j++) {
           if (halPhysicalCameraId == m_pCameraInfo->ppDeviceInfo[j]->cameraId) {
               physicalSettings[j] = pRequest->physcam_settings[i];
               break;
           }
        }
        if (j == m_pCameraInfo->numPhysicalCameras) {
             CHX_LOG_ERROR("Request : frameNum :%d physcam_id[%d] is invalid",
                    pRequest->frame_number, i);
            result = CDKResultEInvalidArg;
            return result;
        }
    }
    // Figure out the physical camera id the logical camera maps to based on
    // focal length.
    if (pRequest->settings != 0) {
        camera_metadata_ro_entry_t entry;
        if (0 == find_camera_metadata_ro_entry(pRequest->settings,
                ANDROID_LENS_FOCAL_LENGTH, &entry)) {
            CHX_LOG("ANDROID_LENS_FOCAL_LENGTH is set to %f", entry.data.f[0]);
            for (UINT32 i = 0; i < MAX_REALTIME_PIPELINE; i++) {
                if (entry.data.f[0] ==
                        m_pCameraInfo->ppDeviceInfo[i]->m_pDeviceCaps->lensCaps.focalLength) {
                    m_LogicalCameraPipelineId = i;
                    break;
                }
            }
        }
    }
    metadataOutput[m_LogicalCameraPipelineId] |= MetadataOutputLogical;

    for (uint32_t i = 0; i < pRequest->num_output_buffers; i++) {
        CHX_LOG("Request : frameNum :%d acquireFence: %d , ReleaseFence: %d Buffer: %p, status: %d, physical id %s",
                pRequest->frame_number, pRequest->output_buffers[i].acquire_fence,
                pRequest->output_buffers[i].release_fence, pRequest->output_buffers[i].buffer,
                pRequest->output_buffers[i].status, pRequest->output_buffers[i].stream->physical_camera_id);

        UINT32 pipelineId = GetUsecasePipelineId(
                pRequest->output_buffers[i].stream->physical_camera_id);

        if (pipelineId == REALTIMEPIPELINELOGICAL) {
            if (m_LogicalCameraPipelineId == REALTIMEPIPELINEMAIN) {
                outputBuffers0[cnt_outbuffer0].size    = sizeof(CHISTREAMBUFFER);
                outputBuffers0[cnt_outbuffer0].pStream = (CHISTREAM *)pRequest->output_buffers[i].stream;
                memcpy(&outputBuffers0[cnt_outbuffer0].acquireFence, &pRequest->output_buffers[i].acquire_fence,
                    sizeof(pRequest->output_buffers[i].acquire_fence));
                memcpy(&outputBuffers0[cnt_outbuffer0].releaseFence, &pRequest->output_buffers[i].release_fence,
                    sizeof(pRequest->output_buffers[i].acquire_fence));
                outputBuffers0[cnt_outbuffer0].bufferStatus = pRequest->output_buffers[i].status;
                outputBuffers0[cnt_outbuffer0].bufferInfo.phBuffer = pRequest->output_buffers[i].buffer;
                cnt_outbuffer0++;
            } else if (m_LogicalCameraPipelineId == REALTIMEPIPELINEAUX) {
                outputBuffers1[cnt_outbuffer1].size    = sizeof(CHISTREAMBUFFER);
                outputBuffers1[cnt_outbuffer1].pStream = (CHISTREAM *)pRequest->output_buffers[i].stream;
                memcpy(&outputBuffers1[cnt_outbuffer1].acquireFence, &pRequest->output_buffers[i].acquire_fence,
                    sizeof(pRequest->output_buffers[i].acquire_fence));
                memcpy(&outputBuffers1[cnt_outbuffer1].releaseFence, &pRequest->output_buffers[i].release_fence,
                    sizeof(pRequest->output_buffers[i].acquire_fence));
                outputBuffers1[cnt_outbuffer1].bufferStatus = pRequest->output_buffers[i].status;
                outputBuffers1[cnt_outbuffer1].bufferInfo.phBuffer = pRequest->output_buffers[i].buffer;
                cnt_outbuffer1++;
            }
        } else if (pipelineId == REALTIMEPIPELINEMAIN) {
            metadataOutput[0] |= MetadataOutputPhysical;
            outputBuffers0[cnt_outbuffer0].size    = sizeof(CHISTREAMBUFFER);
            outputBuffers0[cnt_outbuffer0].pStream = (CHISTREAM *)pRequest->output_buffers[i].stream;
            memcpy(&outputBuffers0[cnt_outbuffer0].acquireFence, &pRequest->output_buffers[i].acquire_fence,
                    sizeof(pRequest->output_buffers[i].acquire_fence));
            memcpy(&outputBuffers0[cnt_outbuffer0].releaseFence, &pRequest->output_buffers[i].release_fence,
                    sizeof(pRequest->output_buffers[i].acquire_fence));
            outputBuffers0[cnt_outbuffer0].bufferStatus = pRequest->output_buffers[i].status;
            outputBuffers0[cnt_outbuffer0].bufferInfo.phBuffer = pRequest->output_buffers[i].buffer;
            cnt_outbuffer0++;
            requestedPipelineIds.insert(pipelineId);
        } else if (pipelineId == REALTIMEPIPELINEAUX) {
            metadataOutput[1] |= MetadataOutputPhysical;
            outputBuffers1[cnt_outbuffer1].size    = sizeof(CHISTREAMBUFFER);
            outputBuffers1[cnt_outbuffer1].pStream = (CHISTREAM *)pRequest->output_buffers[i].stream;
            memcpy(&outputBuffers1[cnt_outbuffer1].acquireFence, &pRequest->output_buffers[i].acquire_fence,
                    sizeof(pRequest->output_buffers[i].acquire_fence));
            memcpy(&outputBuffers1[cnt_outbuffer1].releaseFence, &pRequest->output_buffers[i].release_fence,
                    sizeof(pRequest->output_buffers[i].acquire_fence));
            outputBuffers1[cnt_outbuffer1].bufferStatus = pRequest->output_buffers[i].status;
            outputBuffers1[cnt_outbuffer1].bufferInfo.phBuffer = pRequest->output_buffers[i].buffer;
            cnt_outbuffer1++;
            requestedPipelineIds.insert(pipelineId);
        } else {
            CHX_LOG_ERROR("Request : frameNum :%d pipelineId %d is not valid",
                    pRequest->frame_number, pipelineId);
            result = CDKResultEInvalidArg;
            return result;
        }
    }

    if (pRequest->settings != NULL)
    {
        BOOL mainPipelineActive  = (m_HasPhysicalStream ||
                                   m_LogicalCameraPipelineId == REALTIMEPIPELINEMAIN);
        BOOL auxPipelineActive   = (m_HasPhysicalStream ||
                                   m_LogicalCameraPipelineId == REALTIMEPIPELINEAUX);
        BOOL bothPipelinesActive = (mainPipelineActive && auxPipelineActive);

        // Set SyncInfo
        //TODO: Kernel frame sync isn't verified with LPM yet.
        BOOL isSyncActive = FALSE;
        result = MultiCamControllerManager::s_vendorTagOps.pSetMetaData(
                const_cast<camera_metadata_t*>(pRequest->settings),
                MultiCamControllerManager::m_vendorTagSyncInfo, &isSyncActive,
                sizeof(SyncModeInfo));
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Request: frameNum :%d failed to set SyncActive tag %d",
                    pRequest->frame_number, isSyncActive);
            return result;
        }

        // Set low power mode info
        LowPowerModeInfo lowPowerMode;
        lowPowerMode.isLPMEnabled               = !bothPipelinesActive;
        lowPowerMode.isSlaveOperational         = bothPipelinesActive;
        lowPowerMode.lowPowerMode[0].isEnabled  = !mainPipelineActive;
        lowPowerMode.lowPowerMode[1].isEnabled  = !auxPipelineActive;
        result = MultiCamControllerManager::s_vendorTagOps.pSetMetaData(
                const_cast<camera_metadata_t*>(pRequest->settings),
                MultiCamControllerManager::m_vendorTagLPMInfo,
                &lowPowerMode, sizeof(LowPowerModeInfo));
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Request: frameNum :%d failed to set LPMInfo tag",
                    pRequest->frame_number);
            return result;
        }
    }

    if (cnt_outbuffer0 > 0)
    {
        pChiRTMainInputMetadata = m_pMetadataManager->GetInput(
            pRequest->settings, pRequest->frame_number, true, false);

        pChiMainOutputMetadata = m_pMetadataManager->Get(
            m_pSession[REALTIMESESSIONIDX]->GetMetadataClientId(REALTIMEPIPELINEMAIN),
            pRequest->frame_number);
        if (!pChiRTMainInputMetadata || !pChiMainOutputMetadata)
        {
            CHX_LOG_ERROR("[CMB_ERROR] cannot generate real time request metadata main (%p %p)",
                          pChiRTMainInputMetadata,
                          pChiMainOutputMetadata);
            return CDKResultEFailed;
        }
    }

    if (cnt_outbuffer1 > 0)
    {
        pChiRTAuxInputMetadata = m_pMetadataManager->GetInput(
            pRequest->settings, pRequest->frame_number, true, false);

        pChiAuxOutputMetadata = m_pMetadataManager->Get(
            m_pSession[REALTIMESESSIONIDX]->GetMetadataClientId(REALTIMEPIPELINEAUX),
            pRequest->frame_number );
        if (!pChiRTAuxInputMetadata || !pChiAuxOutputMetadata)
        {
            CHX_LOG_ERROR("[CMB_ERROR] cannot generate real time request metadata main (%p %p)",
                          pChiRTAuxInputMetadata,
                          pChiAuxOutputMetadata);
            return CDKResultEFailed;
        }
    }


    ///< generate request for realtime0
    if (cnt_outbuffer0 > 0) {
        CHICAPTUREREQUEST chiRequest     = {0};

        if (pChiRTMainInputMetadata != NULL) {
            result = UpdateVendorTags(pChiRTMainInputMetadata, REALTIMEPIPELINEMAIN);
            if (CDKResultSuccess != result) {
                CHX_LOG_ERROR("Request: pipeline %d frameNumber %d UpdateVendorTags failure: %d",
                        REALTIMEPIPELINEMAIN, pRequest->frame_number, result);
                return result;
            }
        }
        camID                            = REALTIMEPIPELINEMAIN;
        request                          = &chiRequest;
        memset(request, 0, sizeof(chiRequest));
        request->frameNumber             = pRequest->frame_number;
        request->hPipelineHandle         = reinterpret_cast<CHIPIPELINEHANDLE>(
                                           m_pSession[REALTIMESESSIONIDX]->GetPipelineHandle(camID));
        request->numOutputs              = cnt_outbuffer0;
        request->pOutputBuffers          = (CHISTREAMBUFFER*)(&outputBuffers0[0]);
        request->pInputMetadata          = pChiRTMainInputMetadata->GetHandle();
        request->pOutputMetadata         = pChiMainOutputMetadata->GetHandle();
        CHIPRIVDATA* pPrivData           = &privRealTime1[pRequest->frame_number % MaxOutstandingRequests];
        request->pPrivData               = pPrivData;
        pPrivData->streamIndex           = REALTIMEPIPELINEMAIN;

        result = ModifyCaptureRequestStream(chiRequest, REALTIMEPIPELINEMAIN);
        if (result != CDKResultSuccess) {
            CHX_LOG_ERROR("Request: pipeline %d failed to modify capture request stream: %d, result %d",
                REALTIMEPIPELINEMAIN, pRequest->frame_number, result);
            return result;
        }


        // Fill tuning metadata for request
        if (NULL != m_pSession[REALTIMESESSIONIDX]->GetSensorModeInfo(camID))
        {
            UINT32 sensorModeIndex = m_pSession[REALTIMESESSIONIDX]->GetSensorModeInfo(camID)->modeIndex;
            ChxUtils::FillTuningModeData(pChiRTMainInputMetadata,
                pRequest, sensorModeIndex, &m_effectModeValue[camID], &m_sceneModeValue[camID],
                &m_tuningFeature1Value[camID], &m_tuningFeature2Value[camID]);

            chiRequests.push_back(chiRequest);
        }
        else
        {
            CHX_LOG_ERROR("Sensor mode info is Null");
        }
    }

    ///< generate request for realtime1
    if (cnt_outbuffer1 > 0) {
        CHICAPTUREREQUEST chiRequest     = {0};

        result = UpdateVendorTags(pChiRTAuxInputMetadata, REALTIMEPIPELINEAUX);
        if (CDKResultSuccess != result) {
            CHX_LOG_ERROR("Request: pipeline %d frameNumber %d UpdateVendorTags failure: %d",
                    REALTIMEPIPELINEAUX, pRequest->frame_number, result);
            return result;
        }

        result = RemapLogicalSettings(pChiRTAuxInputMetadata, REALTIMEPIPELINEAUX);
        if (CDKResultSuccess != result) {
            CHX_LOG_ERROR("Request: pipeline %d frameNumber %d RemapLogicalSettings failure: %d",
                    REALTIMEPIPELINEAUX, pRequest->frame_number, result);
            return result;
        }

        camID                            = REALTIMEPIPELINEAUX;
        request                          = &chiRequest;
        request->frameNumber             = pRequest->frame_number;
        request->hPipelineHandle         = reinterpret_cast<CHIPIPELINEHANDLE>(
                                           m_pSession[REALTIMESESSIONIDX]->GetPipelineHandle(camID));
        request->numOutputs              = cnt_outbuffer1;
        request->pOutputBuffers          = (CHISTREAMBUFFER*)(&outputBuffers1[0]);
        request->pInputMetadata          = pChiRTAuxInputMetadata->GetHandle();
        request->pOutputMetadata         = pChiAuxOutputMetadata->GetHandle();
        CHIPRIVDATA*  pPrivData          = &privRealTime2[pRequest->frame_number % MaxOutstandingRequests];
        request->pPrivData               = pPrivData;
        pPrivData->streamIndex           = REALTIMEPIPELINEAUX;

        result = ModifyCaptureRequestStream(chiRequest, REALTIMEPIPELINEAUX);
        if (result != CDKResultSuccess) {
            CHX_LOG_ERROR("Request: pipeline %d failed to modify capture request stream: %d, result %d",
                REALTIMEPIPELINEAUX, pRequest->frame_number, result);
            return result;

        }

        // Fill tuning metadata for request
        if (NULL != m_pSession[REALTIMESESSIONIDX]->GetSensorModeInfo(camID))
        {
            UINT32 sensorModeIndex = m_pSession[REALTIMESESSIONIDX]->GetSensorModeInfo(camID)->modeIndex;
            ChxUtils::FillTuningModeData(pChiRTAuxInputMetadata,
                pRequest, sensorModeIndex, &m_effectModeValue[camID], &m_sceneModeValue[camID],
                &m_tuningFeature1Value[camID], &m_tuningFeature2Value[camID]);

            chiRequests.push_back(chiRequest);
        }
        else
        {
            CHX_LOG_ERROR("Sensor mode info is Null");
        }
    }

    {
        std::unique_lock<std::mutex> lock(mPendingResultLock);
        mPendingChiResults.emplace(pRequest->frame_number,
                PendingChiResult(metadataOutput[0], metadataOutput[1], requestedPipelineIds));
    }

    for (UINT32 i = 0; i <= REALTIMEPIPELINEAUX; i++)
    {
        // Activate standby pipeline if incoming request requires it
        if (FALSE == m_pSession[REALTIMESESSIONIDX]->IsPipelineActive(i) && metadataOutput[i])
        {
            result =
                ExtensionModule::GetInstance()->ActivatePipeline(m_pSession[REALTIMESESSIONIDX]->GetSessionHandle(),
                                                                 m_pSession[REALTIMESESSIONIDX]->GetPipelineHandle(i));

            CHX_LOG_INFO("ActivatePipeline : result = %d", result);
            if (CDKResultSuccess == result)
            {
                m_pSession[REALTIMESESSIONIDX]->SetPipelineActivateFlag(i);
            }
        }
    }

    ///< send two request together
    result = GenerateInternalRequest(REALTIMESESSIONIDX, chiRequests.size(), chiRequests.data());
    CHX_LOG_INFO("GenerateInternalRequest : result = %d", result);

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::RemapLogicalSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseDualCamera::RemapLogicalSettings(ChiMetadata *metadata,
    UINT32 pipelineId)
{
    if (REALTIMEPIPELINEMAIN == pipelineId) {
        return CDKResultSuccess;
    }

    const CHICAMERAINFO* pChiMainCameraInfo =
    m_pCameraInfo->ppDeviceInfo[REALTIMEPIPELINEMAIN]->m_pDeviceCaps;
    camera_info_t* pMainCameraInfo = static_cast<camera_info_t *>(pChiMainCameraInfo->pLegacy);
    const camera_metadata_t* pMainMetadata = pMainCameraInfo->static_camera_characteristics;

    const CHICAMERAINFO* pChiAuxCameraInfo =
    m_pCameraInfo->ppDeviceInfo[REALTIMEPIPELINEAUX]->m_pDeviceCaps;
    camera_info_t* pAuxCameraInfo = static_cast<camera_info_t *>(pChiAuxCameraInfo->pLegacy);
    const camera_metadata_t* pAuxMetadata = pAuxCameraInfo->static_camera_characteristics;

    camera_metadata_entry_t entryMain = { 0 };
    camera_metadata_entry_t entryAux = { 0 };
    // Map sensor sensitivity from logical camera (MAIN) to physical pipeline id
    // (AUX). Assume that the sensitivity range of the physical cameras are identical
    // (may be shifted due to different f-number/lens).
    if ((0 == find_camera_metadata_entry(const_cast<camera_metadata_t*>(pMainMetadata),
    ANDROID_SENSOR_INFO_SENSITIVITY_RANGE, &entryMain)) &&
    (0 == find_camera_metadata_entry(const_cast<camera_metadata_t*>(pAuxMetadata),
        +ANDROID_SENSOR_INFO_SENSITIVITY_RANGE, &entryAux))
    )
    {
    camera_metadata_entry_t entry = { 0 };
    if (CDKResultSuccess == metadata->FindTag(ANDROID_SENSOR_SENSITIVITY, &entry))
    {
        int32_t mainSens = entry.data.i32[0];
        int32_t auxMinSens = entryAux.data.i32[0];
        int32_t auxMaxSens = entryAux.data.i32[1];
        int32_t mainMinSens = entryMain.data.i32[0];
        int32_t mainMaxSens = entryMain.data.i32[1];
        int32_t auxSens = static_cast<int32_t>((1.0 * (auxMaxSens - auxMinSens) /
            (mainMaxSens - mainMinSens)) * (mainSens - mainMinSens) + auxMinSens);
        auxSens = std::max(mainMinSens, std::min(mainMaxSens, mainSens));
        if (CDKResultSuccess != metadata->SetTag(ANDROID_SENSOR_SENSITIVITY,&auxSens, 1))
        {
            CHX_LOG_ERROR("Error: Failed to update ANDROID_SENSOR_SENSITIVITY in settings metadata");
            return CDKResultEFailed;
        }
    }
    }
   return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::UpdateVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseDualCamera::UpdateVendorTags(ChiMetadata *settings, UINT pipelineId)
{
   // Set MultiCameraId
   if (m_IsFirstRequest[pipelineId])
   {
        MultiCameraIds inputMetadata  = {};
        inputMetadata.currentCameraId = m_pCameraInfo->ppDeviceInfo[pipelineId]->cameraId;
        inputMetadata.logicalCameraId = m_cameraId;
        inputMetadata.masterCameraId  = m_pCameraInfo->ppDeviceInfo[pipelineId]->cameraId;
        CDKResult result = settings->SetTag(MultiCamControllerManager::m_vendorTagMultiCameraId,
            &inputMetadata, sizeof(MultiCameraIds));
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Request: Failed to set MultiCameraRole for pipeline %u", pipelineId);
            return result;
        }
        m_IsFirstRequest[pipelineId] = FALSE;
    }

    // Set MasterInfo
    BOOL isMaster = (m_LogicalCameraPipelineId == pipelineId);
    CDKResult result = settings->SetTag(MultiCamControllerManager::m_vendorTagMasterInfo,
        &isMaster, sizeof(BOOL));
    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("Request: Failed %d to set MasterInfo isMaster %d for pipeline %u",
            result, isMaster, pipelineId);
        return result;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::RemapLogicalResultMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseDualCamera::RemapLogicalResultMetadata(ChiMetadata *metadata,
        UINT32 pipelineId)
{
    if (REALTIMEPIPELINEMAIN == pipelineId) {
        return CDKResultSuccess;
    }

    const CHICAMERAINFO* pChiMainCameraInfo =
            m_pCameraInfo->ppDeviceInfo[REALTIMEPIPELINEMAIN]->m_pDeviceCaps;
    camera_info_t* pMainCameraInfo = static_cast<camera_info_t *>(pChiMainCameraInfo->pLegacy);
    const camera_metadata_t* pMainMetadata = pMainCameraInfo->static_camera_characteristics;

    const CHICAMERAINFO* pChiAuxCameraInfo =
            m_pCameraInfo->ppDeviceInfo[REALTIMEPIPELINEAUX]->m_pDeviceCaps;
    camera_info_t* pAuxCameraInfo = static_cast<camera_info_t *>(pChiAuxCameraInfo->pLegacy);
    const camera_metadata_t* pAuxMetadata = pAuxCameraInfo->static_camera_characteristics;

    camera_metadata_entry_t entryMain = { 0 };
    camera_metadata_entry_t entryAux  = { 0 };
    // Map sensor sensitivity from physical pipeline id to logical pipeline id.
    // Assume that the sensitivity range of the physical cameras are identical
    // (may be shifted due to different f-number/lens).
    if ((0 == find_camera_metadata_entry(const_cast<camera_metadata_t*>(pMainMetadata),
            ANDROID_SENSOR_INFO_SENSITIVITY_RANGE, &entryMain)) &&
        (0 == find_camera_metadata_entry(const_cast<camera_metadata_t*>(pAuxMetadata),
            ANDROID_SENSOR_INFO_SENSITIVITY_RANGE, &entryAux))
        )
    {
        camera_metadata_entry_t entry = { 0 };
        if (CDKResultSuccess == metadata->FindTag(ANDROID_SENSOR_SENSITIVITY, &entry))
        {
            int32_t auxSens = entry.data.i32[0];
            int32_t auxMinSens = entryAux.data.i32[0];
            int32_t auxMaxSens = entryAux.data.i32[1];
            int32_t mainMinSens = entryMain.data.i32[0];
            int32_t mainMaxSens = entryMain.data.i32[1];
            int32_t mainSens = static_cast<int32_t>((1.0 * (mainMaxSens - mainMinSens) /
                    (auxMaxSens - auxMinSens)) * (auxSens - auxMinSens) + mainMinSens);
            mainSens = std::max(mainMinSens, std::min(mainMaxSens, mainSens));
            if (CDKResultSuccess != metadata->SetTag(ANDROID_SENSOR_SENSITIVITY, &mainSens, 1))
            {
                CHX_LOG_ERROR("Error: Failed to update ANDROID_SENSOR_SENSITIVITY in result metadata");
                return CDKResultEFailed;
            }
        }
    }
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::PendingChiResult::handleCaptureResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseDualCamera::PendingChiResult::handleCaptureResult(
        UsecaseDualCamera* usecase, const ChiCaptureResult& captureResult, std::string physicalId)
{
    if (captureResult.pOutputMetadata == nullptr) {
        CHX_LOG_ERROR("Error: result metadata for the capture result must be valid");
        return FALSE;
    }

    UINT32 pipelineIndex = static_cast<CHIPRIVDATA*>(captureResult.pPrivData)->streamIndex;
    auto currentPipeline = requestedPipelineIds.find(pipelineIndex);
    if (currentPipeline == requestedPipelineIds.end() &&
            pipelineIndex != logicalPipelineId) {
        CHX_LOG_ERROR("Error: pipeline %d was not requested by request %d",
                pipelineIndex, captureResult.frameworkFrameNum);
        return FALSE;
    }

    // Save newly received capture result->
    if (captureResult.numOutputBuffers > 0) {
        outputBuffers.insert(outputBuffers.end(), captureResult.pOutputBuffers,
                captureResult.pOutputBuffers + captureResult.numOutputBuffers);
    }
    if (captureResult.pInputBuffer != NULL) {
        if (inputBuffer.size() > 0) {
            CHX_LOG_ERROR("Error: more than 1 pipeline returns input buffer");
            return FALSE;
        }
        inputBuffer.push_back(*captureResult.pInputBuffer);
    }

    // Current pipeline generates logical capture result
    if (pipelineIndex == logicalPipelineId) {
        if (logicalMetadata != nullptr) {
            CHX_LOG_ERROR("Error: More than more pipeline generates logical result");
            return FALSE;
        }
        logicalMetadata = static_cast<VOID*>(
            usecase->m_pMetadataManager->GetMetadataFromHandle(captureResult.pOutputMetadata));
    }

    // Current pipeline generates physical capture result
    if (currentPipeline != requestedPipelineIds.end())
    {
        camera_metadata_t* pFrameworkOutput = usecase->m_pMetadataManager->GetAndroidFrameworkOutputMetadata();
        if (NULL != pFrameworkOutput)
        {
            (usecase->m_pMetadataManager->GetMetadataFromHandle(
                captureResult.pOutputMetadata))->TranslateToCameraMetadata(pFrameworkOutput);
        }
        VOID* metadata = static_cast<VOID*>(pFrameworkOutput);
        physicalMetadata.push_back(metadata);
        physicalIds.push_back(physicalId);

        requestedPipelineIds.erase(currentPipeline);
    }

    // If both the logical result and requested physical results are
    // populated, return TRUE. Otherwise, return FALSE;
    if (requestedPipelineIds.size() == 0 && logicalMetadata != nullptr) {
        return TRUE;
    } else {
        return FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::ModifyCaptureRequestStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseDualCamera::ModifyCaptureRequestStream(CHICAPTUREREQUEST& chiRequest,
    UINT32 pipelineId)
 {
    if (pipelineId >= MAX_REALTIME_PIPELINE) {
        CHX_LOG_ERROR("Request: frameNumber %" PRId64 " invalid pipelineId %d",
            chiRequest.frameNumber, pipelineId);
        return CDKResultEInvalidArg;
    }

    for (UINT i = 0; i < chiRequest.numOutputs; i++)
    {
        const CHISTREAM* chiStream = chiRequest.pOutputBuffers[i].pStream;
        BOOL isLogicalStream = strlen(chiStream->physicalCameraId) == 0;
        if (!isLogicalStream) {
            continue;
        }

        auto internalStream = m_IncomingStreamMap.find(chiStream);
        if (internalStream == m_IncomingStreamMap.end())
        {
            CHX_LOG_ERROR("Request: frameNumber %" PRId64 " failed to find internal stream",
            chiRequest.frameNumber);
            return CDKResultEFailed;
        }

        chiRequest.pOutputBuffers[i].pStream =
            m_pInternalStreams[pipelineId][internalStream->second];
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UsecaseDualCamera::ModifyCaptureResultStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult UsecaseDualCamera::ModifyCaptureResultStream(ChiCaptureResult* chiResult)
{
    if (chiResult == NULL) {
        CHX_LOG_ERROR("Result: invalid chiResult %p", chiResult);
        return CDKResultEInvalidArg;
    }
    // Remap from internal stream structure to external stream structure
    for (UINT i = 0; i < chiResult->numOutputBuffers; i++) {
        const CHISTREAM* chiStream = chiResult->pOutputBuffers[i].pStream;
        BOOL isLogicalStream = strlen(chiStream->physicalCameraId) == 0;
        if (!isLogicalStream) {
            continue;
        }

        auto externalStream = m_OutgoingStreamMap.find(chiStream);
        if (externalStream == m_OutgoingStreamMap.end()) {
            CHX_LOG_ERROR("Request: frameNumber %d failed to find external stream",
                chiResult->frameworkFrameNum);
            return CDKResultEFailed;
        }
        CHISTREAMBUFFER* chiBuffer = const_cast<CHISTREAMBUFFER*>(&chiResult->pOutputBuffers[i]);
        chiBuffer->pStream = const_cast<CHISTREAM*>(externalStream->second);
    }

    return CDKResultSuccess;
}




