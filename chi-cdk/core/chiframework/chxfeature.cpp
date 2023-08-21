////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "chxfeature.h"
#include "chxadvancedcamerausecase.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

static const UINT TIMEOUT_FEATURE_READY = 200;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Feature::InitializePrivateResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature::InitializePrivateResources()
{
    m_pStateLock      = Mutex::Create();
    m_pReadyCondition = Condition::Create();

    SetFeatureStatus(FeatureStatus::NOTINIT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Feature::DestroyPrivateResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature::DestroyPrivateResources()
{
    if (NULL != m_pStateLock)
    {
        m_pStateLock->Destroy();
        m_pStateLock = NULL;
    }

    if (NULL != m_pReadyCondition)
    {
        m_pReadyCondition->Destroy();
        m_pReadyCondition = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Feature::ReleaseInputReferences
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature::ReleaseInputReferences(CHIPIPELINEREQUEST* pSubmitRequest)
{
    for (UINT requestIdx = 0; requestIdx < pSubmitRequest->numRequests; requestIdx++)
    {
        const CHICAPTUREREQUEST* captureRequest = &pSubmitRequest->pCaptureRequests[requestIdx];
        CHIPRIVDATA*       pPrivData            = static_cast<CHIPRIVDATA*>(captureRequest->pPrivData);
        m_pUsecase->ReleaseReferenceToInputBuffers(pPrivData);

        if (NULL != captureRequest->pInputMetadata)
        {
            m_pMetadataManager->Release(m_pMetadataManager->GetMetadataFromHandle(captureRequest->pInputMetadata));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Feature::ReleaseOutputReferences
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature::ReleaseOutputReferences(CHIPIPELINEREQUEST* pSubmitRequest)
{
    for (UINT requestIdx = 0; requestIdx < pSubmitRequest->numRequests; requestIdx++)
    {
        const CHICAPTUREREQUEST* pRequest = &pSubmitRequest->pCaptureRequests[requestIdx];
        for (UINT bufferIndex = 0; bufferIndex < pRequest->numOutputs; bufferIndex++)
        {
            CHISTREAMBUFFER*  pOutputBuffer  = &pRequest->pOutputBuffers[bufferIndex];
            if (NULL != pOutputBuffer)
            {
                CHIBufferManager* pBufferManager = GetOutputBufferManager(pOutputBuffer);
                if (NULL != pBufferManager)
                {
                    pBufferManager->ReleaseReference(&pOutputBuffer->bufferInfo);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Feature::InvalidateRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature::InvalidateRequest(CHIPIPELINEREQUEST* pSubmitRequest)
{
    ReleaseInputReferences(pSubmitRequest);
    ReleaseOutputReferences(pSubmitRequest);

    const UINT& sessionId = m_pUsecase->GetSessionId(pSubmitRequest->pSessionHandle);

    for (UINT requestIdx = 0; requestIdx < pSubmitRequest->numRequests; requestIdx++)
    {
        const CHICAPTUREREQUEST* const pRequest       = &pSubmitRequest->pCaptureRequests[requestIdx];
        if (NULL != pRequest)
        {
            UINT                sequenceIndex  = pRequest->frameNumber % MaxOutstandingRequests;
            const UINT          pipelineId     = m_pUsecase->GetPipelineId(sessionId, pRequest->hPipelineHandle);
            const PipelineData* pPipelineData  = m_pUsecase->GetPipelineData(sessionId, pipelineId);

            if (NULL != pPipelineData)
            {
                const UINT&         chiFrameNumber = pPipelineData->seqIdToFrameNum[sequenceIndex];

                CHX_LOG("Setup Early Termination of Frame: %u", chiFrameNumber);
                m_pUsecase->InvalidateFrameNumber(chiFrameNumber);
            }
            else
            {
                CHX_LOG_ERROR("Invalid Pipeline Data");
            }
        }
    }

    SetFeatureStatus(FeatureStatus::READY);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Feature::SetFeatureStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature::SetFeatureStatus(FeatureStatus currentState)
{
    if (currentState == GetFeatureStatus())
    {
        return;
    }

    ChxUtils::AtomicStoreU32(&m_aFeatureStatus, static_cast<UINT32>(currentState));
    switch (currentState)
    {
        case FeatureStatus::READY:
            m_pStateLock->Lock();
            m_pReadyCondition->Broadcast();
            m_pStateLock->Unlock();
            break;
        case FeatureStatus::BUSY:
        case FeatureStatus::NOTINIT:
        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Feature::WaitUntilReady
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Feature::WaitUntilReady(UINT timeoutTime)
{
    CDKResult result = CDKResultSuccess;

    m_pStateLock->Lock();

    while (FeatureStatus::READY != GetFeatureStatus())
    {
        result = m_pReadyCondition->TimedWait(m_pStateLock->GetNativeHandle(), timeoutTime);
        if (result != CDKResultSuccess)
        {
            CHX_LOG_ERROR("Error: %u - TimeoutTime: %u", result, timeoutTime);
            break;
        }
    }

    m_pStateLock->Unlock();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Feature::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Feature::Flush()
{
    CDKResult result = CDKResultSuccess;

    if (FeatureStatus::NOTINIT != GetFeatureStatus())
    {
        result = WaitUntilReady(TIMEOUT_FEATURE_READY);
    }
    else
    {
        CHX_LOG_WARN("Flush called on not initialized feature!");
        result = CDKResultEInvalidState;
    }

    if (CDKResultSuccess != result)
    {
        CHX_LOG_WARN("Flush failed with reason: %u", result);
    }
}
