////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2usecaserequestobject.cpp
/// @brief CHI feature2 request object class definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chibinarylog.h"
#include "chifeature2base.h"
#include "chifeature2usecaserequestobject.h"
#include "chifeature2utils.h"
#include "chitargetbuffermanager.h"

// NOWHINE FILE CP006:  used standard libraries for performance improvements

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2UsecaseRequestObject* ChiFeature2UsecaseRequestObject::Create(
    const ChiFeature2UsecaseRequestObjectCreateInputInfo* pCreateInputInfo)
{
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObject = NULL;

    if ((NULL != pCreateInputInfo) &&
        (NULL != pCreateInputInfo->pRequest) &&
        (NULL != pCreateInputInfo->pAppSettings))
    {
        pUsecaseRequestObject = CHX_NEW ChiFeature2UsecaseRequestObject();
        if (NULL != pUsecaseRequestObject)
        {
            CDKResult result = pUsecaseRequestObject->Initialize(pCreateInputInfo);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("pUsecaseRequestObject->Initialize() failed with result %d", result);
                pUsecaseRequestObject->Destroy();
                pUsecaseRequestObject = NULL;
            }
        }
        else
        {
            CHX_LOG_ERROR("Could not create ChiFeature2Graph");
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid pCreateInputInfo %p", pCreateInputInfo);
    }

    return pUsecaseRequestObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2UsecaseRequestObject::Destroy()
{
    ReleaseInputTargetBufferHandle();

    if (NULL != m_chiCaptureRequest.pInputBuffers)
    {
        CHX_FREE(m_chiCaptureRequest.pInputBuffers);
        m_chiCaptureRequest.pInputBuffers = NULL;
    }

    if (NULL != m_chiCaptureRequest.pOutputBuffers)
    {
        CHX_FREE(m_chiCaptureRequest.pOutputBuffers);
        m_chiCaptureRequest.pOutputBuffers = NULL;
    }

    if (NULL != m_captureRequest.output_buffers)
    {
        // NOWHINE CP036a: allow const cast
        CHX_FREE(const_cast<VOID*>(static_cast<const VOID*>(m_captureRequest.output_buffers)));
        m_captureRequest.output_buffers = NULL;
    }

    for (UINT i = 0; DebugDataMaxOfflineFrames > i; i++)
    {
        if (NULL != m_offlineDebugData[i].pData)
        {
            CHX_FREE(m_offlineDebugData[i].pData);
            m_offlineDebugData[i].pData    = NULL;
            m_offlineDebugData[i].size     = 0;
        }
    }

    CHX_LOG_INFO("URO:%d_Destroyed", m_appFrameNumber);
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2UsecaseRequestObject::Dump(
    INT fd
    ) const
{
    CDK_UNREFERENCED_PARAM(fd);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::SetInterFeatureRequestPrivateData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2UsecaseRequestObject::SetInterFeatureRequestPrivateData(
    UINT32  featureId,
    UINT32  cameraId,
    UINT32  instanceId,
    VOID*   pPrivateData)
{
    CDKResult result             = CDKResultSuccess;
    BOOL      isPrivateDataFound = FALSE;

    std::list<ChiFeature2InterFeatureRequestPrivateData>::iterator it;

    for (it = m_interFeatureRequestPrivateData.begin(); it != m_interFeatureRequestPrivateData.end(); ++it)
    {
        if (featureId == it->featureId)
        {
            if (cameraId == it->cameraId)
            {
                if (instanceId == it->instanceId)
                {
                    it->featureId      = featureId;
                    it->cameraId       = cameraId;
                    it->instanceId     = instanceId;
                    it->pPrivateData   = pPrivateData;
                    isPrivateDataFound = TRUE;
                    break;
                }
            }
        }
    }

    if (FALSE == isPrivateDataFound)
    {
        ChiFeature2InterFeatureRequestPrivateData interFeatureRequestPrivateData = { 0 };
        interFeatureRequestPrivateData.featureId    = featureId;
        interFeatureRequestPrivateData.cameraId     = cameraId;
        interFeatureRequestPrivateData.instanceId   = instanceId;
        interFeatureRequestPrivateData.pPrivateData = pPrivateData;

        m_interFeatureRequestPrivateData.push_back(interFeatureRequestPrivateData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::GetInterFeatureRequestPrivateData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2UsecaseRequestObject::GetInterFeatureRequestPrivateData(
    UINT32  featureId,
    UINT32  cameraId,
    UINT32  instanceId)
{
    CDKResult result = CDKResultSuccess;
    VOID* pPrivateData = NULL;

    std::list<ChiFeature2InterFeatureRequestPrivateData>::iterator it;

    for (it = m_interFeatureRequestPrivateData.begin(); it != m_interFeatureRequestPrivateData.end(); ++it)
    {
        if (featureId == it->featureId)
        {
            if (cameraId == it->cameraId)
            {
                if (instanceId == it->instanceId)
                {
                    pPrivateData = it->pPrivateData;
                    break;
                }
            }
        }
    }

    return pPrivateData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::RemoveInterFeatureRequestPrivateData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2UsecaseRequestObject::RemoveInterFeatureRequestPrivateData(
    UINT32  featureId,
    UINT32  cameraId,
    UINT32  instanceId)
{
    CDKResult result = CDKResultSuccess;

    std::list<ChiFeature2InterFeatureRequestPrivateData>::iterator it;

    for (it = m_interFeatureRequestPrivateData.begin(); it != m_interFeatureRequestPrivateData.end(); ++it)
    {
        if (featureId == it->featureId)
        {
            if (cameraId == it->cameraId)
            {
                if (instanceId == it->instanceId)
                {
                    m_interFeatureRequestPrivateData.erase(it);
                    break;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::~ChiFeature2UsecaseRequestObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2UsecaseRequestObject::~ChiFeature2UsecaseRequestObject()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2UsecaseRequestObject::Initialize(
    const ChiFeature2UsecaseRequestObjectCreateInputInfo* pCreateInputInfo)
{
    CDK_UNREFERENCED_PARAM(pCreateInputInfo);

    CDKResult     result  = CDKResultSuccess;
    VOID*         pData   = NULL;
    StreamHDRMode HDRMode = StreamHDRMode::HDRModeNone;

    // Convert framework camera3_capture_request to internel ChiCaptureRequest
    m_chiCaptureRequest.frameNumber     = pCreateInputInfo->pRequest->frame_number;
    m_chiCaptureRequest.hPipelineHandle = NULL;
    m_chiCaptureRequest.pInputMetadata  = NULL;
    m_chiCaptureRequest.numInputs       = 0;
    m_chiCaptureRequest.pInputBuffers   = NULL;
    m_chiCaptureRequest.pOutputMetadata = NULL;
    m_chiCaptureRequest.numOutputs      = pCreateInputInfo->pRequest->num_output_buffers;
    m_chiCaptureRequest.pMetadata       = NULL;
    m_chiCaptureRequest.pPrivData       = NULL;
    m_appFrameNumber                    = pCreateInputInfo->pRequest->frame_number;
    m_shutterTimeStamp                  = 0;
    m_lastZSLFrameNumber                = 0;
    m_captureRequest                    = { 0 };
    m_MCCResult                         = pCreateInputInfo->MCCResult;
    m_releaseInput                      = FALSE;

    ChxUtils::GetVendorTagValue((pCreateInputInfo->pAppSettings),
        VendorTag::VideoHDR10Mode,
        reinterpret_cast<VOID**>(&pData));

    if (NULL != pData)
    {
        HDRMode = *(static_cast<StreamHDRMode*>(pData));
        if (StreamHDRMode::HDRModeHDR10 == HDRMode)
        {
            m_tuningFeature2Value = static_cast<UINT32>(ChiModeFeature2SubModeType::HDR10);
        }
        else if (StreamHDRMode::HDRModeHLG == HDRMode)
        {
            m_tuningFeature2Value = static_cast<UINT32>(ChiModeFeature2SubModeType::HLG);
        }
        else
        {
            m_tuningFeature2Value = 0;
        }
    }

    if (NULL != pCreateInputInfo->pRequest->input_buffer)
    {
        m_chiCaptureRequest.numInputs       = 1;
        m_chiCaptureRequest.pInputBuffers   = reinterpret_cast<CHISTREAMBUFFER*>(CHX_CALLOC(sizeof(CHISTREAMBUFFER)));

        if (NULL != m_chiCaptureRequest.pInputBuffers)
        {
            ChxUtils::PopulateHALToChiStreamBuffer(&pCreateInputInfo->pRequest->input_buffer[0],
                                                   &m_chiCaptureRequest.pInputBuffers[0]);
        }
        else
        {
            CHX_LOG_ERROR("Failed to allocate memory for input buffers!!");
            result = CDKResultENoMemory;
        }
    }

    if ((CDKResultSuccess == result) && (NULL != pCreateInputInfo->pRequest->output_buffers))
    {
        m_chiCaptureRequest.pOutputBuffers = reinterpret_cast<CHISTREAMBUFFER*>(
            CHX_CALLOC(sizeof(CHISTREAMBUFFER) * m_chiCaptureRequest.numOutputs));

        if (NULL != m_chiCaptureRequest.pOutputBuffers)
        {
            for (UINT32 i = 0; i < pCreateInputInfo->pRequest->num_output_buffers; i++)
            {
                camera3_stream_t* pStream = pCreateInputInfo->pRequest->output_buffers[i].stream;
                ChxUtils::PopulateHALToChiStreamBuffer(&pCreateInputInfo->pRequest->output_buffers[i],
                                                       &m_chiCaptureRequest.pOutputBuffers[i]);
                m_outputSinkStreams.push_back(reinterpret_cast<ChiStream*>(pStream));

                if (UsecaseSelector::IsVideoStream(pStream) || UsecaseSelector::IsPreviewStream(pStream))
                {
                    m_fwkRealtimeStreamData.pFwkStream.push_back(reinterpret_cast<ChiStream*>(pStream));
                    m_fwkRealtimeStreamData.numOutputNotified = 0;
                    m_fwkRealtimeStreamData.numOutputs++;
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("Failed to allocate memory for output buffers!!");
            result = CDKResultENoMemory;
        }
    }

    if (CDKResultSuccess == result)
    {
        if (NULL == m_captureRequest.output_buffers)
        {
            m_captureRequest.output_buffers = static_cast<camera3_stream_buffer_t*>(
                CHX_CALLOC(sizeof(camera3_stream_buffer_t) * pCreateInputInfo->pRequest->num_output_buffers));
        }

        m_pAppSettings      = pCreateInputInfo->pAppSettings;
        m_pMetaSettings     = pCreateInputInfo->pRequest->settings;
        m_pStatusMetadata   = pCreateInputInfo->pStatusMetadata;
        m_reprocessFlag     = pCreateInputInfo->reprocessFlag;
        m_appReprocessFlag  = pCreateInputInfo->appReprocessFlag;
        m_inputSrcStreams   = pCreateInputInfo->inputSrcStreams;
        m_requestState      = ChiFeature2UsecaseRequestObjectState::Initialized;
        m_usecaseMode       = ChxUtils::GetUsecaseMode(pCreateInputInfo->pRequest);
        UINT32 frameNumber  = m_appFrameNumber;
        ChxUtils::DeepCopyCamera3CaptureRequest(pCreateInputInfo->pRequest, &m_captureRequest);
        CHX_LOG_INFO("URO:%d_Created for usecasemode:%d", frameNumber, m_usecaseMode);
        BINARY_LOG(LogEvent::FT2_URO_Init, frameNumber);
    }

    if (CDKResultSuccess == result)
    {
        if (ChiModeUsecaseSubModeType::Snapshot == m_usecaseMode)
        {
            ChxUtils::Memset(&m_offlineDebugData, 0x0, sizeof(m_offlineDebugData));
        }
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::SetInputConfigurations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2UsecaseRequestObject::SetInputConfigurations(
    std::vector<ChiFeature2UsecaseRequestObjectExtSrcStreamData>  selectedInputSrcStreams)
{
    CDKResult result = CDKResultSuccess;

    m_selectedInputSrcStreamsData   = selectedInputSrcStreams;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::GetInputSrcNumBuffersNeeded
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ChiFeature2UsecaseRequestObject::GetInputSrcNumBuffersNeeded(
    ChiStream* pChiStream
    ) const
{
    CDK_UNREFERENCED_PARAM(pChiStream);
    UINT numBuffers = 0;

    // Assume the number of input buffers for all the stream are same. If there is special requirements for it, we can
    // change it
    if (m_selectedInputSrcStreamsData.size() > 0)
    {
        numBuffers = m_selectedInputSrcStreamsData[0].numInputs;
    }

    return numBuffers;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::IsFDStreamInSelectedSrcStreams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2UsecaseRequestObject::IsFDStreamInSelectedSrcStreams()
{
    BOOL isFDRequired = FALSE;
    for (UINT32 i = 0; i < m_selectedInputSrcStreamsData.size(); i++)
    {
        if (TRUE == IsFDStream(m_selectedInputSrcStreamsData[i].pChiStream))
        {
            isFDRequired = TRUE;
            break;
        }
    }
    return isFDRequired;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::SetInputTargetBufferHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2UsecaseRequestObject::SetInputTargetBufferHandle(
    ChiStream*                   pInternalStream,
    CHITARGETBUFFERINFOHANDLE    hMetadataTBH,
    CHITARGETBUFFERINFOHANDLE    hOutputTBH)
{
    CDKResult result                  = CDKResultSuccess;
    BOOL      isInputMetadataTBHFound = FALSE;
    BOOL      isInputBufferTBHFound   = FALSE;

    // Set MetadataTBHs
    if (NULL != hMetadataTBH)
    {
        CHITargetBufferManager* pTBM = CHITargetBufferManager::GetTargetBufferManager(hMetadataTBH);
        for (auto item = m_inputMetadataTBHs.begin(); item != m_inputMetadataTBHs.end(); ++item)
        {
            if (pInternalStream == item->pInternalStream)
            {
                isInputMetadataTBHFound = TRUE;
                item->bufferMetadataTBHs.push_back(hMetadataTBH);
                break;
            }
        }
        if (FALSE == isInputMetadataTBHFound)
        {
            ChiFeature2UsecaseRequestObjectInputStreamBufferMetadataTBHs streamMetadataTBHs;
            std::vector<CHITARGETBUFFERINFOHANDLE>                       metadataTBH;
            metadataTBH.push_back(hMetadataTBH);
            streamMetadataTBHs.pInternalStream    = pInternalStream;
            streamMetadataTBHs.bufferMetadataTBHs = metadataTBH;
            m_inputMetadataTBHs.push_back(streamMetadataTBHs);
        }
    }

    // Set input BufferTBHs
    if (NULL != hOutputTBH)
    {
        for (auto item = m_inputBufferTBHs.begin(); item != m_inputBufferTBHs.end(); ++item)
        {
            if (pInternalStream == item->pInternalStream)
            {
                isInputBufferTBHFound = TRUE;
                item->bufferMetadataTBHs.push_back(hOutputTBH);
                break;
            }
        }
        if (FALSE == isInputBufferTBHFound)
        {
            ChiFeature2UsecaseRequestObjectInputStreamBufferMetadataTBHs streamBufferTBHs;
            std::vector<CHITARGETBUFFERINFOHANDLE>                       bufferTBH;
            bufferTBH.push_back(hOutputTBH);
            streamBufferTBHs.pInternalStream    = pInternalStream;
            streamBufferTBHs.bufferMetadataTBHs = bufferTBH;
            m_inputBufferTBHs.push_back(streamBufferTBHs);
        }
    }

    if (m_inputBufferTBHs.size() != 0 || m_inputMetadataTBHs.size() != 0)
    {
        m_releaseInput = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::ReleaseInputTargetBufferHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2UsecaseRequestObject::ReleaseInputTargetBufferHandle()
{
    if (m_releaseInput == FALSE)
    {
        return;
    }

    // release input buffer TBH
    for (auto inputStreamBufferTBHs : m_inputBufferTBHs)
    {
        for (auto inputBufferTBHs : inputStreamBufferTBHs.bufferMetadataTBHs)
        {
            CHITargetBufferManager* pTBM = CHITargetBufferManager::GetTargetBufferManager(inputBufferTBHs);
            if (NULL != pTBM)
            {
                CHX_LOG_INFO("Input buffer TBH:%p", inputBufferTBHs);
                pTBM->ReleaseTargetBuffer(inputBufferTBHs);
            }
        }
    }

    // For reprocess request, Meta TBM will be handled by pipeline
    if (FALSE == IsAppReprocessRequest())
    {
        // release input buffer TBH
        for (auto inputStreamMetadataTBHs : m_inputMetadataTBHs)
        {
            for (auto inputMetadataTBHs : inputStreamMetadataTBHs.bufferMetadataTBHs)
            {
                CHITargetBufferManager* pTBM = CHITargetBufferManager::GetTargetBufferManager(inputMetadataTBHs);
                if (NULL != pTBM)
                {
                    CHX_LOG_INFO("Input metadata TBH:%p IsAppReprocessRequest=%d", inputMetadataTBHs, IsAppReprocessRequest());
                    pTBM->ReleaseTargetBuffer(inputMetadataTBHs);
                }
            }
        }
    }
    else
    {
        CHX_LOG_WARN("App reprocess, not releasing metadata");
    }

    m_releaseInput = FALSE;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::SetOutputTargetBufferHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2UsecaseRequestObject::SetOutputTargetBufferHandle(
    ChiStream*                  pChiStream,
    CHITARGETBUFFERINFOHANDLE   hTargetBufferInfoHandle)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pChiStream) || (NULL == hTargetBufferInfoHandle))
    {
        CHX_LOG_ERROR("Invalid args! pChiStream=%p hTargetBufferInfoHandle=%p", pChiStream, hTargetBufferInfoHandle);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        if (m_outputSinkStreams.end() == std::find(m_outputSinkStreams.begin(), m_outputSinkStreams.end(), pChiStream))
        {
            CHX_LOG_ERROR("pChiStream %p is not output sink stream of this URO!", pChiStream);
            result = CDKResultEInvalidArg;
        }
    }

    if (CDKResultSuccess == result)
    {
        if (m_outputBufferTBHs.find(pChiStream) != m_outputBufferTBHs.end())
        {
            CHX_LOG_ERROR("Output target buffer handle is already set for ChiStream %p", pChiStream);
            result = CDKResultEInvalidArg;
        }
    }

    if (CDKResultSuccess == result)
    {
        m_outputBufferTBHs[pChiStream] = hTargetBufferInfoHandle;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2UsecaseRequestObject::GetOutputTargetBufferHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHITARGETBUFFERINFOHANDLE ChiFeature2UsecaseRequestObject::GetOutputTargetBufferHandle(
    ChiStream* pChiStream)
{
    CDKResult                   result          = CDKResultSuccess;
    CHITARGETBUFFERINFOHANDLE   hTargetBuffer    = NULL;

    if (NULL == pChiStream)
    {
        CHX_LOG_ERROR("Invalid args! pChiStream=%p", pChiStream);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        if (m_outputSinkStreams.end() == std::find(m_outputSinkStreams.begin(), m_outputSinkStreams.end(), pChiStream))
        {
            CHX_LOG_ERROR("pChiStream %p is not output sink stream of this URO!", pChiStream);
            result = CDKResultEInvalidArg;
        }
    }

    if (CDKResultSuccess == result)
    {
        if (m_outputBufferTBHs.find(pChiStream) == m_outputBufferTBHs.end())
        {
            CHX_LOG_ERROR("Output target buffer handle is not set for ChiStream %p", pChiStream);
            result = CDKResultEInvalidArg;
        }
    }

    if (CDKResultSuccess == result)
    {
        hTargetBuffer = m_outputBufferTBHs[pChiStream];

        if (NULL == hTargetBuffer)
        {
            CHX_LOG_ERROR("m_outputBufferTBHs[%p] is NULL!", pChiStream);
        }
    }

    return hTargetBuffer;
}
