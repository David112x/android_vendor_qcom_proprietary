////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxsession.cpp
/// @brief Definitions for Session class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-2499): Will move this to the OsUtil library later
#define USE_COLOR_METADATA


#if (!defined(LE_CAMERA))
#include "qdMetaData.h"
#endif // !LE_CAMERA
#include "camxincs.h"
#include "camxmem.h"
#include "camxmemspy.h"
#include "camxthreadmanager.h"
#include "camxdeferredrequestqueue.h"
#include "camxhal3defaultrequest.h"
#include "camxhal3stream.h"
#include "camxhal3metadatautil.h"
#include "camxhal3queue.h"
#include "camxhwcontext.h"
#include "camxchi.h"
#include "camxpipeline.h"
#include "camxnode.h"
#include "camxsession.h"
#include "camxsettingsmanager.h"
#include "camxvendortags.h"
#include "chi.h"
#include "chibinarylog.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constant Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT32 MaxContentLightLevel                = 1000;         ///< Maximum content light level
static const UINT32 MaxFrameAverageLightLevel           = 200;          ///< Maximum frame average light level
static const UINT32 MaxDisplayLuminance                 = 1000;         ///< Maximum Luminance in cd/m^2
static const UINT32 MinDisplayLuminance                 = 50;           ///< Minimum Luminance in 1/10000 cd/m^2
static const UINT32 PrimariesRGB[3][2]                  = {{34000, 16000}, {13250, 34500}, {7500, 3000}};
static const UINT32 PrimariesWhitePoint[2]              = {15635, 16450};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::~Session
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Session::~Session()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::WaitTillAllResultsAvailable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::WaitTillAllResultsAvailable(
    UINT overrideTime)
{
    CamxResult result   = CamxResultSuccess;

    m_pWaitForResultsLock->Lock();
    if (FALSE == IsDoneProcessing())
    {
        m_pResultHolderListLock->Lock();
        if (0 != m_resultHolderList.NumNodes())
        {
            SessionResultHolder* pSessionResultHolderHead =
                reinterpret_cast<SessionResultHolder*>(m_resultHolderList.Head()->pData);
            SessionResultHolder* pSessionResultHolderTail =
                reinterpret_cast<SessionResultHolder*>(m_resultHolderList.Tail()->pData);
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Waiting for all results minRequestIdPending: %u  maxRequestIdPending: %u "
                "minSequenceIdPending: %u  maxSequenceIdPending: %u",
                pSessionResultHolderHead->resultHolders[0].requestId,
                pSessionResultHolderTail->resultHolders[pSessionResultHolderTail->numResults - 1].requestId,
                pSessionResultHolderHead->resultHolders[0].sequenceId,
                pSessionResultHolderTail->resultHolders[pSessionResultHolderTail->numResults - 1].sequenceId);
        }
        m_pResultHolderListLock->Unlock();

        UINT waitTime;
        if (0 == overrideTime)
        {
            waitTime = static_cast<UINT>(GetFlushResponseTime());
        }
        else
        {
            waitTime = overrideTime;
        }
        CAMX_LOG_INFO(CamxLogGroupCore, "Session is not done processing, Waiting for %u", waitTime);

        m_waitforResults = TRUE;
        result = m_pWaitAllResultsAvailable->TimedWait(m_pWaitForResultsLock->GetNativeHandle(), waitTime);
        m_waitforResults = FALSE;
        m_pWaitForResultsLock->Unlock();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                           "TimedWait for results timed out at %u ms with error %s! Pending results: ",
                           waitTime, Utils::CamxResultToString(result));

            // Dumping during recovery creates vulnerability to access destroyed pointers
            if (FALSE == GetSessionTriggeringRecovery())
            {
                DumpSessionState(SessionDumpFlag::Flush);
            }
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "TimedWait returned with success");
        }
    }
    else
    {
        m_pWaitForResultsLock->Unlock();
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::WaitTillFlushResultsAvailable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::WaitTillFlushResultsAvailable()
{
    CamxResult result   = CamxResultSuccess;

    m_pWaitForResultsLock->Lock();
    if (FALSE == IsDoneProcessing())
    {

        UINT waitTime;
        waitTime = static_cast<UINT>(GetFlushResponseTime());

        CAMX_LOG_INFO(CamxLogGroupCore, "Session is not done processing, Waiting for %u", waitTime);
        CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupCore, 0, "WaitTillFlushResultsAvailable");
        m_waitForPipelineFlushResults = TRUE;
        result = m_pWaitFlushResultsAvailable->TimedWait(m_pWaitForResultsLock->GetNativeHandle(), waitTime);
        m_waitForPipelineFlushResults = FALSE;
        m_pWaitForResultsLock->Unlock();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                           "TimedWait for results timed out at %u ms with error %s! Pending results: ",
                           waitTime, Utils::CamxResultToString(result));

            // Dumping during recovery creates vulnerability to access destroyed pointers
            if (FALSE == GetSessionTriggeringRecovery())
            {
                DumpSessionState(SessionDumpFlag::Flush);
            }
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "TimedWait returned with success");
        }
    }
    else
    {
        m_pWaitForResultsLock->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::FallbackFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::FallbackFlush(
    BOOL isSessionFlush)
{
    CamxResult result = CamxResultSuccess;
    UINT fallbackWaitTime = HwEnvironment::GetInstance()->GetStaticSettings()->sessionFallbackWaitTime;

    CAMX_LOG_INFO(CamxLogGroupCore, "Flush timed out, Giving the nodes %u additional fallback wait time.",
                  fallbackWaitTime);

    m_pWaitForResultsLock->Lock();
    if (FALSE == IsDoneProcessing())
    {
        Condition* waitCondition = (TRUE == isSessionFlush) ? m_pWaitAllResultsAvailable : m_pWaitFlushResultsAvailable;

        if (TRUE == isSessionFlush)
        {
            m_waitforResults = TRUE;
        }
        else
        {
            m_waitForPipelineFlushResults = TRUE;
        }
        result = waitCondition->TimedWait(m_pWaitForResultsLock->GetNativeHandle(), fallbackWaitTime);
        if (TRUE == isSessionFlush)
        {
            m_waitforResults = FALSE;
        }
        else
        {
            m_waitForPipelineFlushResults = FALSE;
        }
    }
    m_pWaitForResultsLock->Unlock();

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore,
            "Fallback TimedWait for results timed out at %u ms with error %s!",
            fallbackWaitTime, Utils::CamxResultToString(result));
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "Fallback TimedWait returned with success");
    }

    if (CamxResultSuccess != result)
    {
        UINT32 numPipelines = (TRUE == isSessionFlush) ? m_numPipelines : m_sessionFlushInfo.numPipelines;
        for (UINT32 index = 0; index < numPipelines; index++)
        {
            UINT pipelineIndex = index;
            if (FALSE == isSessionFlush)
            {
                pipelineIndex = GetPipelineIndex(m_sessionFlushInfo.pPipelineFlushInfo[index].hPipelineHandle);
            }

            if (PipelineStatus::STREAM_ON == m_pipelineData[pipelineIndex].pPipeline->GetPipelineStatus())
            {
                m_pipelineData[pipelineIndex].pPipeline->FlushPendingNodes();
            }
        }

        m_pWaitForResultsLock->Lock();
        if (FALSE == IsDoneProcessing())
        {
            m_waitforResults = TRUE;
            result = m_pWaitAllResultsAvailable->TimedWait(m_pWaitForResultsLock->GetNativeHandle(), fallbackWaitTime);
            m_waitforResults = FALSE;
        }
        m_pWaitForResultsLock->Unlock();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Fallback forced flush call timed out at %u ms with error %s!",
                fallbackWaitTime, Utils::CamxResultToString(result));
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "Fallback Forced flush returned with success");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetFlushStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::GetFlushStatus()
{
    BOOL      pipelineFlushStatus = TRUE;

    for (UINT index = 0; index < m_numPipelines; index++)
    {
        pipelineFlushStatus = ((pipelineFlushStatus) && (GetPipelineFlushStatus(index)));
    }

    return pipelineFlushStatus;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetPipelineFlushStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BOOL Session::GetPipelineFlushStatus(
    UINT32 pipelineIndex)
{
    Pipeline* pPipeline = m_pipelineData[pipelineIndex].pPipeline;
    return static_cast<BOOL>(pPipeline->GetPipelineFlushState());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::PreFlushLockOperations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::PreFlushLockOperations(
    UINT32* pPipelineIndexes,
    UINT32  numPipelinesToFlush)
{
    CamxResult result = CamxResultSuccess;

    for (UINT index = 0; index < numPipelinesToFlush; index++)
    {
        // input pipelineIndex not really match the index recorded by Session, so use GetPipelineIndex
        // to get corresponding pipeline index.
        pPipelineIndexes[index] = GetPipelineIndex(m_sessionFlushInfo.pPipelineFlushInfo[index].hPipelineHandle);

        CAMX_LOG_INFO(CamxLogGroupCore, "Pipeline %d: %s (%p) : PipelineStatus: %d FlushType: %d NumPipelines: %d",
            pPipelineIndexes[index],
            m_pipelineData[pPipelineIndexes[index]].pPipeline->GetPipelineIdentifierString(),
            this,
            m_pipelineData[pPipelineIndexes[index]].pPipeline->GetPipelineStatus(),
            m_sessionFlushInfo.pPipelineFlushInfo[index].flushType,
            numPipelinesToFlush);
    }

    for (UINT index = 0; index < numPipelinesToFlush; index++)
    {
        Pipeline* pPipeline = m_pipelineData[pPipelineIndexes[index]].pPipeline;
        pPipeline->UpdatePipelineFlushState(TRUE);
    }

    // print nodes still processing, save last valid requestId
    for (UINT32 index = 0; index < numPipelinesToFlush; index++)
    {
        UINT64    pipelineIndex = pPipelineIndexes[index];
        Pipeline* pPipeline = m_pipelineData[pipelineIndex].pPipeline;

        if (PipelineStatus::STREAM_ON == pPipeline->GetPipelineStatus())
        {
            pPipeline->SaveLastValidRequestId();
            pPipeline->LogPendingNodes();
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::PostFlushLockOperations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::PostFlushLockOperations(
    UINT32* pPipelineIndexes,
    UINT32  numPipelinesToFlush)
{
    CamxResult result = CamxResultSuccess;

    for (UINT index = 0; index < numPipelinesToFlush; index++)
    {
        Pipeline* pPipeline = m_pipelineData[pPipelineIndexes[index]].pPipeline;
        pPipeline->UpdatePipelineFlushState(FALSE);
    }

    m_qtimerErrIndicated = 0;

    CAMX_TRACE_SYNC_BEGIN(CamxLogGroupCore, "Set AE Lock Range");
    if (FALSE == m_sessionFlushInfo.isDestroyInProgress)
    {
        for (UINT32 index = 0; index < numPipelinesToFlush; index++)
        {
            UINT pipelineIndex = pPipelineIndexes[index];

            if (PipelineStatus::STREAM_ON == m_pipelineData[pipelineIndex].pPipeline->GetPipelineStatus())
            {
                // Disable AE lock if already set
                if (TRUE == m_pipelineData[pipelineIndex].pPipeline->IsRealTime())
                {
                    SetAELockRange(pipelineIndex, 0, 0);
                }
            }
        }
    }
    CAMX_TRACE_SYNC_END(CamxLogGroupCore);

    for (UINT32 index = 0; index < numPipelinesToFlush; index++)
    {
        UINT32 pipelineIndex = pPipelineIndexes[index];
        Pipeline* pPipeline = m_pipelineData[pipelineIndex].pPipeline;
        if (PipelineStatus::STREAM_ON == pPipeline->GetPipelineStatus())
        {
            pPipeline->ResetLastSubmittedShutterRequestId();
        }
    }

    m_sessionFlushInfo = { 0 };

    m_pLivePendingRequestsLock->Lock();
    DispatchResultsForInflightRequests();
    // Send Live pending request signal to unlock request and
    // pending request would be cleared after the lock released.
    // Otherwise thread will be released after destroy
    CAMX_LOG_VERBOSE(CamxLogGroupCore, "Sending signal to m_pWaitLivePendingRequests");
    m_pWaitLivePendingRequests->Signal();
    m_pLivePendingRequestsLock->Unlock();

    m_pFlushDoneLock->Lock();
    m_pWaitForFlushDone->Signal();
    m_pFlushDoneLock->Unlock();

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::PipelineFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::PipelineFlush(
    const CHISESSIONFLUSHINFO* pSessionFlushInfo)
{
    CAMX_TRACE_SYNC_BEGIN(CamxLogGroupCore, "PipelineFlush");
    CamxResult result                                  = CamxResultSuccess;
    UINT       maxRequestIdPending                     = 0;
    UINT32     numPipelinesToFlush                     = 0;
    BOOL       isSessionFlush                          = FALSE;
    UINT32     pipelineIndexes[MaxPipelinesPerSession] = {};

    CamxAtomicStore32(&m_isPipelineFlush, TRUE);

    // pSessionFlushInfo comes in with a NULL sanity check
    Utils::Memcpy(&m_sessionFlushInfo, pSessionFlushInfo, sizeof(ChiSessionFlushInfo));

    numPipelinesToFlush = m_sessionFlushInfo.numPipelines;

    // Add shared code between PipelineFlush and SessionFlush before CSLFlushLock here
    PreFlushLockOperations(pipelineIndexes, numPipelinesToFlush);

    // Prepare Flush Payload
    CSLFlushInfo cslFlushInfo = {};

    for (UINT32 index = 0; index < numPipelinesToFlush; index++)
    {
        UINT32    pipelineIndex = pipelineIndexes[index];
        Pipeline* pPipeline = m_pipelineData[pipelineIndex].pPipeline;

        if (PipelineStatus::STREAM_ON == pPipeline->GetPipelineStatus())
        {
            cslFlushInfo.flushType     = CSLFlushAll;
            cslFlushInfo.lastCSLSyncId = m_lastCSLSyncId;
            cslFlushInfo.lastRequestId = pPipeline->GetLastSubmittedRequestId();
            cslFlushInfo.phSyncLink    = pPipeline->GetCSLLink();
            cslFlushInfo.numSyncLinks  = 1;
            cslFlushInfo.phDevices     = pPipeline->GetCSLDevices();
            cslFlushInfo.numDevices    = CamxMaxDeviceIndex;

            m_pDeferredRequestQueue->SetPreemptPipelineDependency(TRUE, pipelineIndex);
            m_pChiContext->GetHwContext()->FlushLock(GetCSLSession(), cslFlushInfo);
        }
    }

    // Wait for Pipeline Flush to complete
    result = WaitTillFlushResultsAvailable();

    if (CamxResultSuccess != result)
    {
        if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->raisesigabrt)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: Flush could not clean up the requests in time for");
            for (UINT32 index = 0; index < numPipelinesToFlush; index++)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline Index: %d, ",
                               pipelineIndexes[index]);
            }
            CAMX_LOG_ERROR(CamxLogGroupCore, "in session %p", this);
            OsUtils::RaiseSignalAbort();
        }
        else
        {
            result = FallbackFlush(isSessionFlush);
        }
    }

    CAMX_TRACE_SYNC_BEGIN(CamxLogGroupCore, "Clear Pending Res/FlushMetadata");
    for (UINT32 index = 0; index < numPipelinesToFlush; index++)
    {
        UINT32    pipelineIndex = pipelineIndexes[index];
        Pipeline* pPipeline = m_pipelineData[pipelineIndex].pPipeline;
        if (PipelineStatus::STREAM_ON == pPipeline->GetPipelineStatus())
        {
            pPipeline->ClearPendingResources();
            if (FALSE == m_sessionFlushInfo.isDestroyInProgress)
            {
                pPipeline->FlushMetadata();
            }
        }
        pPipeline->SaveFlushInfo(cslFlushInfo.flushType);
    }
    CAMX_TRACE_SYNC_END(CamxLogGroupCore);

    // Call unlock after all the results are out
    m_pChiContext->GetHwContext()->FlushUnlock(GetCSLSession());

    CAMX_TRACE_SYNC_BEGIN(CamxLogGroupCore, "Set Preempt Pipeline Dependency");
    for (UINT32 index = 0; index < numPipelinesToFlush; index++)
    {
        UINT32 pipelineIndex = pipelineIndexes[index];
        m_pDeferredRequestQueue->SetPreemptPipelineDependency(FALSE, pipelineIndex);
    }
    CAMX_TRACE_SYNC_END(CamxLogGroupCore);

    // Add shared code between PipelineFlush and SessionFlush after CSLFlushUnlock here
    PostFlushLockOperations(pipelineIndexes, numPipelinesToFlush);

    // Reset the pipeline Flush State
    CamX::Utils::Memset(m_pipelineFlushResultsAv, FALSE, sizeof(BOOL)*MaxPipelinesPerSession);

    CamxAtomicStoreU8(&m_aSessionDumpComplete, FALSE);
    CamxAtomicStore32(&m_isPipelineFlush, FALSE);

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::SessionFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::SessionFlush(
    const CHISESSIONFLUSHINFO* pSessionFlushInfo)
{
    CamxResult  result                 = CamxResultSuccess;
    UINT64      lastSubmittedRequestId = 0;
    UINT32      numPipelinesToFlush    = 0;
    UINT32      pipelineIndexes[MaxPipelinesPerSession] = {};

    // pSessionFlushInfo comes in with a NULL sanity check
    Utils::Memcpy(&m_sessionFlushInfo, pSessionFlushInfo, sizeof(ChiSessionFlushInfo));

    // Print pipeline flush info provided by the use case for all pipelines
    numPipelinesToFlush = m_sessionFlushInfo.numPipelines;

    CAMX_ASSERT(numPipelinesToFlush == m_numPipelines);

    // Add shared code between PipelineFlush and SessionFlush before CSLFlushLock here
    PreFlushLockOperations(pipelineIndexes, numPipelinesToFlush);

    // print nodes still processing, save last valid requestId
    for (UINT32 index = 0; index < m_numPipelines; index++)
    {
        Pipeline* pPipeline = m_pipelineData[index].pPipeline;
        if (PipelineStatus::STREAM_ON == pPipeline->GetPipelineStatus())
        {
            lastSubmittedRequestId = Utils::MaxUINT64(pPipeline->GetLastSubmittedRequestId(), lastSubmittedRequestId);
        }
    }

    // Prepare Flush Payload
    m_pDeferredRequestQueue->SetPreemptDependencyFlag(TRUE);

    CSLFlushInfo cslFlushInfo = {};
    cslFlushInfo.flushType = CSLFlushAll;
    cslFlushInfo.lastRequestId = lastSubmittedRequestId;
    cslFlushInfo.lastCSLSyncId = m_lastCSLSyncId;

    // Block submissions to the CSL.
    m_pChiContext->GetHwContext()->FlushLock(GetCSLSession(), cslFlushInfo);

    // Wait for all the results. if times out report error
    if (CamxResultSuccess != WaitTillAllResultsAvailable())
    {
        if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->raisesigabrt)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: Flush could not clean up the requests in time");
            OsUtils::RaiseSignalAbort();
        }
        else
        {
            BOOL isSessionFlush = TRUE;
            result = FallbackFlush(isSessionFlush);
        }

    }

    CAMX_TRACE_SYNC_BEGIN(CamxLogGroupCore, "Clear Pending Res/FlushMetadata");
    for (UINT32 index = 0; index < m_numPipelines; index++)
    {
        UINT32    pipelineIndex = pipelineIndexes[index];
        Pipeline* pPipeline = m_pipelineData[pipelineIndex].pPipeline;
        if (PipelineStatus::STREAM_ON == pPipeline->GetPipelineStatus())
        {
            pPipeline->ClearPendingResources();
            if (FALSE == m_sessionFlushInfo.isDestroyInProgress)
            {
                pPipeline->FlushMetadata();
            }
        }
        pPipeline->SaveFlushInfo(cslFlushInfo.flushType);
    }
    CAMX_TRACE_SYNC_END(CamxLogGroupCore);

    // Call unlock after all the results are out
    m_pChiContext->GetHwContext()->FlushUnlock(GetCSLSession());

    ClearResultHolderList();

    // Empty the Hal3Queue
    FlushHALQueue();

    m_pDeferredRequestQueue->SetPreemptDependencyFlag(FALSE);

    // Add shared code between PipelineFlush and SessionFlush after CSLFlushUnlock here
    PostFlushLockOperations(pipelineIndexes, numPipelinesToFlush);

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "Flush is done for session %p", this);
    m_additionalWaitTimeForLivePending = TRUE;
    CamxAtomicStoreU8(&m_aSessionDumpComplete, FALSE);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::ClearResultHolderList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::ClearResultHolderList()
{
    ResultHolder*           pResultHolder                = NULL;
    SessionResultHolder*    pSessionResultHolder         = NULL;

    m_pResultHolderListLock->Lock();

    LightweightDoublyLinkedListNode* pNode = m_resultHolderList.Head();

    while (NULL != pNode)
    {
        if (NULL != pNode->pData)
        {
            pSessionResultHolder = reinterpret_cast<SessionResultHolder*>(pNode->pData);

            for (UINT32 index = 0; index < pSessionResultHolder->numResults; index++)
            {
                pResultHolder = &(pSessionResultHolder->resultHolders[index]);
                if (NULL != pResultHolder)
                {
                    for (UINT32 bufIndex = 0; bufIndex < MaxNumOutputBuffers; bufIndex++)
                    {
                        ResultHolder::BufferResult* pBufferResult = &pResultHolder->bufferHolder[bufIndex];

                        if (NULL  != pBufferResult->pStream)
                        {
                            ChiStreamWrapper* pChiStreamWrapper =
                                    static_cast<ChiStreamWrapper*>(pBufferResult->pStream->pPrivateInfo);

                            CAMX_LOG_VERBOSE(CamxLogGroupCore, "MoveToNextExpectedResultFrame from %d",
                                         reinterpret_cast<ResultHolder*>(pNode->pData)->sequenceId);
                            pChiStreamWrapper->MoveToNextExpectedResultFrame();
                        }
                    }
                }
            }

            CAMX_FREE(pNode->pData);
            pNode->pData = NULL;
        }

        m_resultHolderList.RemoveNode(pNode);
        CAMX_FREE(pNode);
        pNode    = NULL;

        pNode    = m_resultHolderList.Head();
    }

    m_pResultHolderListLock->Unlock();

    // setting livePending to default.
    m_pLivePendingRequestsLock->Lock();
    if (0 < m_livePendingRequests)
    {
        m_livePendingRequests = 0;

        m_pWaitLivePendingRequests->Signal();
    }
    m_pLivePendingRequestsLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::Flush(
    const CHISESSIONFLUSHINFO* pSessionFlushInfo)
{
    CamxResult result                 = CamxResultSuccess;
    UINT       maxRequestIdPending    = 0;
    BOOL       enablePipelineFlush    = HwEnvironment::GetInstance()->GetStaticSettings()->enablePipelineFlush;
    BOOL       destroyInProgress      = FALSE;

    if (FALSE == m_sesssionInitComplete)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Flush is called before session %p is initialized", this);
        result = CamxResultEInvalidState;
    }
    else
    {
        UINT32     flushStartTime;
        UINT32     flushEndTime;
        CamxTime   pTime;

        OsUtils::GetTime(&pTime);
        flushStartTime = OsUtils::CamxTimeToMillis(&pTime);
        m_pFlushLock->Lock();

        if ((NULL != pSessionFlushInfo) &&
            (TRUE == pSessionFlushInfo->isDestroyInProgress))
        {
            destroyInProgress = TRUE;
        }
        else if (TRUE == m_sessionFlushInfo.isDestroyInProgress)
        {
            destroyInProgress = TRUE;
        }

        for (UINT32 index = 0; index < m_numPipelines; index++)
        {
            Pipeline* pPipeline = m_pipelineData[index].pPipeline;
            if (NULL == pPipeline)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Flush called failed for session %p with pipelines %s", this, m_pipelineNames);
                result = CamxResultEInvalidPointer;
                break;
            }

            if  ((FALSE == destroyInProgress) &&
                (TRUE == pPipeline->IsRealTime()) &&
                (PipelineStatus::STREAM_ON != pPipeline->GetPipelineStatus()) &&
                (0 < pPipeline->GetLivePendingRequest()))
            {
                CAMX_LOG_INFO(CamxLogGroupCore,
                    "Flush call of session %p is waiting for device stream on of pipeline %s, current livependingReq %d",
                    this, pPipeline->GetPipelineName(), pPipeline->GetLivePendingRequest());
                result = pPipeline->WaitUntilStreamOnDone();
                if (CamxResultSuccess != result)
                {
                    result = CamxResultSuccess;
                    break;
                }
            }

            // signal config done if session destroy is in progress
            if ((TRUE == destroyInProgress) && (TRUE == pPipeline->IsRealTime()) &&
                (PipelineStatus::STREAM_ON != pPipeline->GetPipelineStatus()))
            {
                pPipeline->AbortConfigDone();
            }
        }

        if ((CamxResultSuccess == result) && (TRUE == IsAnyPendingRequest()))
        {
            if ((TRUE == enablePipelineFlush) &&
                (NULL != pSessionFlushInfo))
            {
                CAMX_LOG_INFO(CamxLogGroupCore, "Pipeline Flush Selected");
                result = PipelineFlush(pSessionFlushInfo);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupCore, "Session Flush Selected");

                // Prepare Session Flush Payload
                CHISESSIONFLUSHINFO  sessionFlushInfo = { 0 };
                CHIPIPELINEFLUSHINFO pipelineFlushInfo[MaxPipelinesPerSession];

                for (UINT pipelineIndex = 0; pipelineIndex < m_numPipelines; pipelineIndex++)
                {
                    pipelineFlushInfo[pipelineIndex].flushType = FlushAll;
                    pipelineFlushInfo[pipelineIndex].hPipelineHandle = reinterpret_cast<CHIPIPELINEHANDLE>(
                        m_pipelineData[pipelineIndex].pPipelineDescriptor);
                }

                sessionFlushInfo.numPipelines = m_numPipelines;
                sessionFlushInfo.pSessionHandle = NULL;
                sessionFlushInfo.pPipelineFlushInfo = pipelineFlushInfo;

                result = SessionFlush(&sessionFlushInfo);
            }
        }

        m_pFlushLock->Unlock();
        OsUtils::GetTime(&pTime);
        flushEndTime = OsUtils::CamxTimeToMillis(&pTime);

        CAMX_LOG_CONFIG(CamxLogGroupCore, "Flush took %u ms. Session (%p)",
            (flushEndTime - flushStartTime),
            this);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::Destroy()
{
    CAMX_ENTRYEXIT(CamxLogGroupCore);

    CAMX_LOG_INFO(CamxLogGroupCore, "Destroying session %p", this);

    ClearPendingMBDoneQueue();

    m_pFlushLock->Lock();
    // Using lock to control atomicity instead of atomic store since this member var is controlled
    // by FlushLock
    m_sessionFlushInfo.isDestroyInProgress = TRUE;
    m_pFlushLock->Unlock();

    Flush();

    this->FlushThreadJobCallback();

    // Due to drain logic we had better destroy anything that has a job registered first (including DRQ)
    // But - as the Nodes already have m_hNodeJobHandle with them, they may continue posting jobs (can happen if Flush timedout)
    // If we Unregister here and Nodes postJob using the same m_hNodeJobHandle - there can be 2 cases :
    // 1. This JobHandle slot is reset while Unregister, so PostJob will fail as the JobHandle slot validation fails because it
    //    is invalid - PostJob after Unregister is handled with error, no issues.
    // 2. This JobHandle slot is reset while Unregister, but say if Register from a different client comes immediately
    //    and the same JobHandle slot is given to that client (i.e now this slot of JobHandle becomes valid with that client's
    //    callback function, etc), and then Nodes post their jobs using the same handle, but they are actually going into the
    //    queue of different client which just acquired the same JobHandle slot. So whatever jobs that the Nodes are posting
    //    would be called into the callback function of different client - causing issues.
    // So, just do Flush here, so that we will flush all pending jobs and also do not allow any further jobs from Nodes
    // because, flush status is set to Flushed (so further PostJob will fail)
    // And do actual UnregisterJobFamily later after all Nodes are destroyed.

    if (InvalidJobHandle != m_hNodeJobHandle)
    {
        m_pThreadManager->FlushJobFamily(m_hNodeJobHandle, m_pThreadManager, TRUE);
    }

    m_pStreamOnOffLock->Lock();

    /// @todo (CAMX-1797) Temporary workaround - Need to figure out the right place
    for (UINT i = 0; i < m_numPipelines; i++)
    {
        if (NULL != m_pipelineData[i].pPipeline)
        {
            // Disable AE lock if already set
            if (TRUE == m_pipelineData[i].pPipeline->IsRealTime())
            {
                SetAELockRange(i, 0, 0);
            }
            m_pipelineData[i].pPipeline->StreamOff(CHIDeactivateModeDefault);
            m_pipelineData[i].pPipeline->Unlink();
        }
    }

    m_pStreamOnOffLock->Unlock();

    m_pSessionDumpLock->Lock();

    if (NULL != m_pDeferredRequestQueue)
    {
        m_pDeferredRequestQueue->Destroy();
        m_pDeferredRequestQueue = NULL;
    }

    // We should have no more result waiting for this session, so we are good to move all the nodes
    // we allocated

    LightweightDoublyLinkedListNode* pNode = m_resultHolderList.RemoveFromHead();
    while (NULL != pNode)
    {
        CAMX_ASSERT(NULL != pNode->pData);
        if (NULL != pNode->pData)
        {
            CAMX_FREE(pNode->pData);
            pNode->pData = NULL;
        }
        CAMX_FREE(pNode);
        pNode = m_resultHolderList.RemoveFromHead();
    }


    if (NULL != m_pLivePendingRequestsLock)
    {
        m_pLivePendingRequestsLock->Destroy();
        m_pLivePendingRequestsLock = NULL;
    }

    if (NULL != m_pWaitLivePendingRequests)
    {
        m_pWaitLivePendingRequests->Destroy();
        m_pWaitLivePendingRequests = NULL;
    }

    for (UINT i = 0; i < m_numPipelines; i++)
    {
        if (NULL != m_pipelineData[i].pPipeline)
        {
            // unregister to resource manager before destroying the pipeline
            if (TRUE == UsingResourceManager(i))
            {
                ResourceID resourceId = static_cast<ResourceID>(ResourceType::RealtimePipeline);

                m_pChiContext->GetResourceManager()->UnregisterResourceWithClient(
                    resourceId, static_cast<VOID*>(m_pipelineData[i].pPipelineDescriptor));
            }

            m_pipelineData[i].pPipeline->Destroy();
            m_pipelineData[i].pPipeline = NULL;
        }
    }

    this->UnregisterThreadJobCallback();

    // m_hNodeJobHandle is passed to Nodes while FinalizePipeline, so they have this handle within them, so they may still
    // continue posting jobs. So make sure to unregister this only after all the Nodes are destroyed.
    // In case we do not want to allow Nodes to PostJobs, set Flush status to Flushed by calling FlushJobFamily earlier
    if (InvalidJobHandle != m_hNodeJobHandle)
    {
        CHAR wrapperName[FILENAME_MAX];
        OsUtils::SNPrintF(&wrapperName[0], sizeof(wrapperName), "NodeCommonThreadJobFamily%p", this);
        m_pThreadManager->UnregisterJobFamily(NodeThreadJobFamilySessionCb, wrapperName, m_hNodeJobHandle);
        m_hNodeJobHandle = InvalidJobHandle;
    }

    UINT32 numOutputBuffers =
        m_requestQueueDepth * m_numPipelines * GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined);
    for (UINT32 i = 0; i < numOutputBuffers; i++)
    {
        if (NULL != m_pCaptureResult[i].pOutputBuffers)
        {
            // NOWHINE CP036a: exception
            CAMX_FREE(const_cast<ChiStreamBuffer*>(m_pCaptureResult[i].pOutputBuffers));
            m_pCaptureResult[i].pOutputBuffers = NULL;
        }
    }

    if (NULL != m_pCaptureResult)
    {
        CAMX_FREE(m_pCaptureResult);
        m_pCaptureResult = NULL;
    }

    if (NULL != m_pRequestQueue)
    {
        m_pRequestQueue->Destroy();
        m_pRequestQueue = NULL;
    }

    if (NULL != m_pResultLock)
    {
        m_pResultLock->Destroy();
        m_pResultLock = NULL;
    }

    if (NULL != m_pWaitForResultsLock)
    {
        m_pWaitForResultsLock->Destroy();
        m_pWaitForResultsLock = NULL;
    }

    if (NULL != m_pRequestLock)
    {
        m_pRequestLock->Destroy();
        m_pRequestLock = NULL;
    }

    if (NULL != m_pFlushLock)
    {
        m_pFlushLock->Destroy();
        m_pFlushLock = NULL;
    }

    if (NULL != m_pFlushDoneLock)
    {
        m_pFlushDoneLock->Destroy();
        m_pFlushDoneLock = NULL;
    }

    if (NULL != m_pWaitForFlushDone)
    {
        m_pWaitForFlushDone->Destroy();
        m_pWaitForFlushDone = NULL;
    }

    if (NULL != m_pStreamOnOffLock)
    {
        m_pStreamOnOffLock->Destroy();
        m_pStreamOnOffLock = NULL;
    }

    if (NULL != m_ppPerFrameDebugDataPool)
    {
        for (UINT i = 0; i < m_numPipelines; i++)
        {
            if (NULL != m_ppPerFrameDebugDataPool[i])
            {
                m_ppPerFrameDebugDataPool[i]->Destroy();
                m_ppPerFrameDebugDataPool[i] = NULL;
            }
        }
        CAMX_FREE(m_ppPerFrameDebugDataPool);
        m_ppPerFrameDebugDataPool = NULL;
    }

    if (NULL != m_pDebugDataBuffer)
    {
        CAMX_FREE(m_pDebugDataBuffer);
        m_pDebugDataBuffer = NULL;
    }

    if (NULL != m_pStreamBufferInfo)
    {
        CAMX_FREE(m_pStreamBufferInfo);
        m_pStreamBufferInfo = NULL;
    }

    if (NULL != m_pWaitAllResultsAvailable)
    {
        m_pWaitAllResultsAvailable->Destroy();
        m_pWaitAllResultsAvailable = NULL;
    }

    if (NULL != m_pWaitFlushResultsAvailable)
    {
        m_pWaitFlushResultsAvailable->Destroy();
        m_pWaitFlushResultsAvailable = NULL;
    }

    if (NULL != m_pResultHolderListLock)
    {
        m_pResultHolderListLock->Destroy();
        m_pResultHolderListLock = NULL;
    }

    if (NULL != m_pPendingMBQueueLock)
    {
        m_pPendingMBQueueLock->Destroy();
        m_pPendingMBQueueLock = NULL;
    }

    if (NULL != m_resultStreamBuffers.pLock)
    {
        m_resultStreamBuffers.pLock->Destroy();
        m_resultStreamBuffers.pLock = NULL;
    }

    if (NULL != m_notifyMessages.pLock)
    {
        m_notifyMessages.pLock->Destroy();
        m_notifyMessages.pLock = NULL;
    }

    if (NULL != m_PartialDataMessages.pLock)
    {
        m_PartialDataMessages.pLock->Destroy();
        m_PartialDataMessages.pLock = NULL;
    }

    if (NULL != m_notifyMessages.pNotifyMessage)
    {
        CAMX_FREE(m_notifyMessages.pNotifyMessage);
        m_notifyMessages.pNotifyMessage = NULL;
    }

    if (NULL != m_PartialDataMessages.pPartialCaptureMessage)
    {
        CAMX_FREE(m_PartialDataMessages.pPartialCaptureMessage);
        m_PartialDataMessages.pPartialCaptureMessage = NULL;
    }

    if (NULL != m_pCaptureResultForInflightRequests)
    {
        for (UINT8 requestIdx = 0; requestIdx < m_numInflightRequests; requestIdx++)
        {
            if (NULL != m_pCaptureResultForInflightRequests[requestIdx].pInputBuffer)
            {
                // NOWHINE CP036a: exception
                CAMX_FREE(const_cast<ChiStreamBuffer*>(m_pCaptureResultForInflightRequests[requestIdx].pInputBuffer));
                m_pCaptureResultForInflightRequests[requestIdx].pInputBuffer = NULL;
            }
            if (NULL != m_pCaptureResultForInflightRequests[requestIdx].pOutputBuffers)
            {
                // NOWHINE CP036a: exception
                CAMX_FREE(const_cast<ChiStreamBuffer*>(m_pCaptureResultForInflightRequests[requestIdx].pOutputBuffers));
                m_pCaptureResultForInflightRequests[requestIdx].pOutputBuffers = NULL;
            }
        }

        CAMX_FREE(m_pCaptureResultForInflightRequests);
        m_pCaptureResultForInflightRequests = NULL;
        m_numInflightRequests = 0;
    }

    m_pSessionDumpLock->Unlock();

    if (NULL != m_pSessionDumpLock)
    {
        m_pSessionDumpLock->Destroy();
        m_pSessionDumpLock = NULL;
    }

    m_sesssionInitComplete = FALSE;

    CAMX_LOG_INFO(CamxLogGroupCore, "%s: CSLClose(0x%x)", m_pipelineNames, GetCSLSession());
    m_pChiContext->GetHwContext()->CloseSession(GetCSLSession());

    CAMX_LOG_CONFIG(CamxLogGroupCore, "Session (%p) Destroy", this);
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::NotifyPipelinesOfTriggeringRecovery
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::NotifyPipelinesOfTriggeringRecovery(
    BOOL isRecovery)
{
    for (UINT32 index = 0; index < m_numPipelines; index++)
    {
        Pipeline* pPipeline = m_pipelineData[index].pPipeline;
        if (NULL == pPipeline)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, " Pipeline cannot be NULL");
            continue;
        }
        pPipeline->SetPipelineTriggeringRecovery(isRecovery);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::FinalizeDeferPipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::FinalizeDeferPipeline(
    UINT32 pipelineIndex)
{
    CAMX_ENTRYEXIT(CamxLogGroupCore);
    CamxResult result = CamxResultSuccess;
    CAMX_ASSERT(pipelineIndex < m_numPipelines);

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    m_requestBatchId[pipelineIndex]           = CamxInvalidRequestId;

    FinalizeInitializationData finalizeInitializationData = { 0 };

    finalizeInitializationData.pHwContext                 = m_pChiContext->GetHwContext();
    finalizeInitializationData.pDeferredRequestQueue      = m_pDeferredRequestQueue;
    finalizeInitializationData.pDebugDataPool             = m_ppPerFrameDebugDataPool[pipelineIndex];
    finalizeInitializationData.pSession                   = this;
    finalizeInitializationData.pThreadManager             = m_pThreadManager;
    finalizeInitializationData.usecaseNumBatchedFrames    = m_usecaseNumBatchedFrames;
    finalizeInitializationData.HALOutputBufferCombined    = m_HALOutputBufferCombined;
    finalizeInitializationData.enableQTimer               = pStaticSettings->enableQTimer;
    finalizeInitializationData.numSessionPipelines        = m_numPipelines;
    finalizeInitializationData.pSensorModeInfo            =
        &(m_pipelineData[pipelineIndex].pPipelineDescriptor->inputData[0].sensorInfo.sensorMode);
    finalizeInitializationData.resourcePolicy             =
        m_pipelineData[pipelineIndex].pipelineFinalizeData.pipelineResourcePolicy;

    if (InvalidJobHandle == m_hNodeJobHandle)
    {
        CHAR wrapperName[FILENAME_MAX];
        OsUtils::SNPrintF(&wrapperName[0], sizeof(wrapperName), "NodeCommonThreadJobFamily%p", this);
        result = m_pThreadManager->RegisterJobFamily(NodeThreadJobFamilySessionCb,
                                                     wrapperName,
                                                     NULL,
                                                     JobPriority::Normal,
                                                     TRUE,
                                                     &m_hNodeJobHandle);
    }

    if (CamxResultSuccess == result)
    {
        finalizeInitializationData.hThreadJobFamilyHandle     = m_hNodeJobHandle;

        result = m_pipelineData[pipelineIndex].pPipeline->FinalizePipeline(&finalizeInitializationData);

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::FinalizePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::FinalizePipeline(
    SessionCreateData* pCreateData,
    UINT32             pipelineIndex,
    BIT                enableQTimer)
{
    CAMX_ENTRYEXIT_NAME(CamxLogGroupCore, "SessionFinalizePipeline");
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCreateData);

    if (FALSE == pCreateData->pPipelineInfo[pipelineIndex].isDeferFinalizeNeeded)
    {
        FinalizeInitializationData finalizeInitializationData = { 0 };

        finalizeInitializationData.pHwContext                 = pCreateData->pHwContext;
        finalizeInitializationData.pDeferredRequestQueue      = m_pDeferredRequestQueue;
        finalizeInitializationData.pDebugDataPool             = m_ppPerFrameDebugDataPool[pipelineIndex];
        finalizeInitializationData.pSession                   = this;
        finalizeInitializationData.pThreadManager             = m_pThreadManager;
        finalizeInitializationData.usecaseNumBatchedFrames    = m_usecaseNumBatchedFrames;
        finalizeInitializationData.HALOutputBufferCombined    = m_HALOutputBufferCombined;
        finalizeInitializationData.enableQTimer               = enableQTimer;
        finalizeInitializationData.numSessionPipelines        = pCreateData->numPipelines;
        finalizeInitializationData.pSensorModeInfo            =
            pCreateData->pPipelineInfo[pipelineIndex].pipelineInputInfo.sensorInfo.pSensorModeInfo;
        finalizeInitializationData.resourcePolicy             =
            pCreateData->pPipelineInfo[pipelineIndex].pipelineResourcePolicy;

        if (InvalidJobHandle == m_hNodeJobHandle)
        {
            CHAR wrapperName[FILENAME_MAX];
            OsUtils::SNPrintF(&wrapperName[0], sizeof(wrapperName), "NodeCommonThreadJobFamily%p", this);
            result = m_pThreadManager->RegisterJobFamily(NodeThreadJobFamilySessionCb,
                                                         wrapperName,
                                                         NULL,
                                                         JobPriority::Normal,
                                                         TRUE,
                                                         &m_hNodeJobHandle);
        }

        if (CamxResultSuccess == result)
        {
            finalizeInitializationData.hThreadJobFamilyHandle     = m_hNodeJobHandle;
            CAMX_LOG_INFO(CamxLogGroupCore, "Finalize for pipeline %d, %s !",
                          pipelineIndex, m_pipelineData[pipelineIndex].pPipeline->GetPipelineName());
            result = m_pipelineData[pipelineIndex].pPipeline->FinalizePipeline(&finalizeInitializationData);
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "deferred pipeline %d, %s finalization!",
                      pipelineIndex, m_pipelineData[pipelineIndex].pPipeline->GetPipelineName());
        m_pipelineData[pipelineIndex].pipelineFinalizeData.pipelineResourcePolicy =
            pCreateData->pPipelineInfo[pipelineIndex].pipelineResourcePolicy;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetNumInputSensors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Session::GetNumInputSensors(
    SessionCreateData* pSessionCreateData)
{
    UINT32 numOfInputSensors = 0;
    for (UINT i = 0; i < pSessionCreateData->numPipelines; i++)
    {
        ChiPipelineInfo*      pPipelineInfo       = &pSessionCreateData->pPipelineInfo[i];
        ChiPipelineInputInfo* pPipelineInput      = &pPipelineInfo->pipelineInputInfo;
        if (TRUE == pPipelineInput->isInputSensor)
        {
            numOfInputSensors++;
        }
    }
    return numOfInputSensors;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetSensorSyncMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorSyncMode Session::GetSensorSyncMode(
    PipelineDescriptor* pPipelineDescriptor)

{
    SensorSyncMode syncMode  = NoSync;
    CamxResult result        = CamxResultSuccess;

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    if ((MultiCameraHWSyncDisabled != pStaticSettings->multiCameraHWSyncMask) && (m_numInputSensors > 1))
    {
        UINT32  metaTag                           = 0;
        VOID*   pSyncMode                         = NULL;
        SensorSyncModeMetadata* pSensorSyncConfig = NULL;
        MetaBuffer* pMetaBuffer                   = pPipelineDescriptor->pSessionMetadata;

        result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerasensorconfig",
            "sensorsyncmodeconfig", &metaTag);

        if ((CamxResultSuccess == result) && (NULL != pMetaBuffer))
        {
            pSensorSyncConfig = static_cast<SensorSyncModeMetadata*>(pMetaBuffer->GetTag(metaTag, TRUE));
            if ((NULL != pSensorSyncConfig) && (TRUE == pSensorSyncConfig->isValid))
            {
                syncMode = pSensorSyncConfig->sensorSyncMode;
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupCore, "Get Multi Camera hardware sync metadata failed %p", pSensorSyncConfig);
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupCore, "No sensor sync tag found!");
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "Disable sensor hardware sync for CameraId:%d",
            pPipelineDescriptor->cameraId);
    }

    return syncMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::NodeThreadJobFamilySessionCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* Session::NodeThreadJobFamilySessionCb(
    VOID* pCbData)
{
    CAMX_ASSERT(NULL != pCbData);

    return Node::NodeThreadJobFamilyCb(pCbData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::BuildSessionName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::BuildSessionName(
    SessionCreateData* pCreateData)
{
    m_pipelineNames[0] = '\0';
    for (UINT i = 0; i < pCreateData->numPipelines; i++)
    {
        ChiPipelineInfo*      pPipelineInfo       = &pCreateData->pPipelineInfo[i];
        PipelineDescriptor*   pPipelineDescriptor = reinterpret_cast<PipelineDescriptor*>(pPipelineInfo->hPipelineDescriptor);
        if (0 < i)
        {
            OsUtils::StrLCat(m_pipelineNames, ", ", sizeof(m_pipelineNames));
        }
        OsUtils::StrLCat(m_pipelineNames, pPipelineDescriptor->pipelineName, sizeof(m_pipelineNames));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::Initialize(
    SessionCreateData* pCreateData)
{
    CamxResult           result                    = CamxResultSuccess;
    UINT32               additionalNeededRequests  = 0;
    UINT32               numMetadataSlots          = DefaultPerFramePoolWindowSize;

    BOOL requireExtraHalRequest = FALSE;

    CAMX_ASSERT(NULL != pCreateData);
    CAMX_ASSERT(NULL != pCreateData->pThreadManager);

    Utils::Memcpy(&m_chiCallBacks, pCreateData->pChiAppCallBacks, sizeof(ChiCallBacks));

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    m_pThreadManager                   = pCreateData->pThreadManager;
    m_pChiContext                      = pCreateData->pChiContext;
    m_pPrivateCbData                   = pCreateData->pPrivateCbData;
    m_HALOutputBufferCombined          = pCreateData->HALOutputBufferCombined;
    m_linkSyncMode                     = CSLSyncLinkModeNoSync;
    m_numStreamedOnRealtimePipelines   = 0;
    m_sequenceId                       = 0;
    m_syncSequenceId                   = 1;
    m_numRealtimePipelines             = 0;
    m_numMetadataResults               = 1;
    m_ppPerFrameDebugDataPool          = NULL;
    m_pDebugDataBuffer                 = NULL;
    m_debugDataSlots                   = 0;
    m_debugDataBufferIndex             = 0;

    BOOL isRealtimePipeline = SetRealtimePipeline(pCreateData);
    BOOL isActiveSensor     = CheckActiveSensor(pCreateData);

    BuildSessionName(pCreateData);

    // Checking to see if there is an active CSLSession for the camera
    if ((TRUE == isRealtimePipeline) && (TRUE == isActiveSensor))
    {
        result = GetActiveCSLSession(pCreateData, &m_hCSLSession);

        if (CamxResultSuccess == result)
        {
            result = CSLAddReference(GetCSLSession());
            CAMX_LOG_INFO(CamxLogGroupCore, "%s: CSLAddReference(0x%x)", m_pipelineNames, m_hCSLSession);
        }
    }
    else
    {
        result = m_pChiContext->GetHwContext()->OpenSession(&m_hCSLSession);
        CAMX_LOG_INFO(CamxLogGroupCore, "%s: CSLOpen(0x%x)", m_pipelineNames, m_hCSLSession);
    }

    if (CamxResultSuccess != result)
    {
        m_hCSLSession = CSLInvalidHandle;
        CAMX_LOG_ERROR(CamxLogGroupCore, "Could not obtain CSL Session handle.");
        return result;
    }

    m_currExposureTimeUseBySensor           = 0;
    m_hNodeJobHandle                        = InvalidJobHandle;
    m_numInputSensors                       = GetNumInputSensors(pCreateData);
    m_numPipelines                          = pCreateData->numPipelines;
    m_recordingEndOfStreamTagId             = 0;
    m_setVideoPerfModeFlag                  = FALSE;
    m_sesssionInitComplete                  = FALSE;
    m_recordingEndOfStreamRequestIdTagId    = 0;
    m_lastFPSCountTime                      = 0;
    m_numLinksSynced                        = 0;
    CamxAtomicStoreU32(&m_aTotalLongExposureTimeout, 0);

    Utils::Memset(m_hLinkHandles, 0, sizeof(m_hLinkHandles));

    m_additionalWaitTimeForLivePending = TRUE;

    if (1 == pCreateData->flags.u.IsFastAecSession)
    {
        m_statsOperationMode = StatsOperationModeFastConvergence;
    }
    else
    {
        m_statsOperationMode = StatsOperationModeNormal;
    }

    for (UINT i = 0; i < pCreateData->numPipelines; i++)
    {
        ChiPipelineInfo*      pPipelineInfo       = &pCreateData->pPipelineInfo[i];
        PipelineDescriptor*   pPipelineDescriptor = reinterpret_cast<PipelineDescriptor*>(pPipelineInfo->hPipelineDescriptor);
        ChiPipelineInputInfo* pPipelineInput      = &pPipelineInfo->pipelineInputInfo;

        m_pipelineData[i].pPipelineDescriptor = pPipelineDescriptor;
        /// @todo (CAMX-3119) remove IsTorchWidgetSession setting below and handle this in generic way.
        m_isTorchWidgetSession                |= pPipelineDescriptor->flags.isTorchWidget;

        // Consume the input buffer info for offline pipelines and update the pipeline descriptor with that information
        if (FALSE == pPipelineInput->isInputSensor)
        {
            ChiInputBufferInfo* pChiInputBufferInfo = &pPipelineInput->inputBufferInfo;
            GrallocProperties   grallocProperties;
            Format              selectedFormat;

            for (UINT input = 0; input < pChiInputBufferInfo->numInputBuffers; input++)
            {
                const ChiPortBufferDescriptor* pBufferDescriptor = &pChiInputBufferInfo->pInputBufferDescriptors[input];
                ChiStream*                     pChiStream        = pBufferDescriptor->pStream;
                if ((NULL != pChiStream) && (ChiStreamTypeInput == pChiStream->streamType))
                {
                    pChiStream->pHalStream = NULL;
                }

                Camera3Stream*                 pHAL3Stream       = reinterpret_cast<Camera3Stream*>(pChiStream);
                ChiStreamWrapper*              pChiStreamWrapper =
                    reinterpret_cast<ChiStreamWrapper*>(pChiStream->pPrivateInfo);

                grallocProperties.colorSpace         = static_cast<ColorSpace>(pChiStream->dataspace);
                grallocProperties.pixelFormat        = pChiStream->format;
                grallocProperties.grallocUsage       = m_pChiContext->GetGrallocUsage(pChiStream);
                grallocProperties.isInternalBuffer   = TRUE;
                grallocProperties.isRawFormat        = FALSE;
                grallocProperties.staticFormat       = HwEnvironment::GetInstance()->GetStaticSettings()->outputFormat;
                grallocProperties.isMultiLayerFormat = FALSE;

                if (ChiStreamTypeInput == pChiStream->streamType)
                {
                    grallocProperties.isRawFormat = pBufferDescriptor->bIsOverrideImplDefinedWithRaw;
                    result                        = ImageFormatUtils::GetFormat(grallocProperties, selectedFormat);

                    if (CamxResultSuccess == result)
                    {
                        pChiStreamWrapper = CAMX_NEW ChiStreamWrapper(pHAL3Stream, input, selectedFormat);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore,
                            "GetFormat failed, Input %d, portId %d pixelFormat %d, outputFormat %d, usage %llu",
                            input, pBufferDescriptor->pNodePort[0].nodePortId,
                            grallocProperties.pixelFormat, grallocProperties.staticFormat,
                            grallocProperties.grallocUsage);
                        break;
                    }

                    if (NULL != pChiStreamWrapper)
                    {
                        /// @todo (CAMX-1512) Session can contain all the created Wrappers that it can clean up when destroyed
                        // The wrapper is created by this session
                        BOOL isOwner = TRUE;

                        m_pChiContext->SetChiStreamInfo(pChiStreamWrapper, pPipelineDescriptor->numBatchedFrames, FALSE);

                        pChiStream->pPrivateInfo                                  = pChiStreamWrapper;

                        result = m_pChiContext->SetPipelineDescriptorInputStream(pPipelineDescriptor, pBufferDescriptor,
                            isOwner);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCore, "Input stream setting for pipeline descriptor failed!!"
                                " for input: %d of pipeline %s result = %s", input, pPipelineDescriptor->pipelineName,
                                Utils::CamxResultToString(result));
                            break;
                        }
                        /// @todo (CAMX-4925) add support for multiple port per stream.
                        pChiStreamWrapper->SetPortId(pBufferDescriptor->pNodePort[0].nodePortId);
                        pChiStreamWrapper->SetNumberOfPortId(pBufferDescriptor->numNodePorts);
                        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Input %d, portId %d wd ht %d x %d wrapper %x stream %x",
                            input, pBufferDescriptor->pNodePort[0].nodePortId, pChiStream->width, pChiStream->height,
                            pChiStreamWrapper, pChiStream);
                    }
                    else
                    {
                        result = CamxResultENoMemory;
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory!");
                        break;
                    }
                }
                else
                {
                    /// @todo (CAMX-1512) Session can contain all the created Wrappers that it can clean up when destroyed
                    // The wrapper was created by some other session and this session is simply using it as an input
                    BOOL isOwner = FALSE;

                    if ((TRUE == pCreateData->isNativeChi) && (NULL == pChiStreamWrapper))
                    {
                        grallocProperties.isRawFormat = pBufferDescriptor->bIsOverrideImplDefinedWithRaw;
                        result                        = ImageFormatUtils::GetFormat(grallocProperties, selectedFormat);

                        if (CamxResultSuccess == result)
                        {
                            pChiStreamWrapper        = CAMX_NEW ChiStreamWrapper(pHAL3Stream, input, selectedFormat);
                            pChiStream->pPrivateInfo = pChiStreamWrapper;
                            isOwner                  = TRUE;
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCore,
                                "GetFormat failed, Input %d, portId %d pixelFormat %d, outputFormat %d, usage %llu",
                                input, pBufferDescriptor->pNodePort[0].nodePortId,
                                grallocProperties.pixelFormat, grallocProperties.staticFormat,
                                grallocProperties.grallocUsage);
                            break;
                        }
                    }

                    if (NULL == pChiStreamWrapper)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "ChiStreamWrapper cannot be NULL!");
                        result = CamxResultEFailed;
                        break;
                    }

                    result = m_pChiContext->SetPipelineDescriptorInputStream(pPipelineDescriptor, pBufferDescriptor, isOwner);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Input stream setting for pipeline descriptor failed!!"
                            " for input: %d of pipeline %s result = %s", input, pPipelineDescriptor->pipelineName,
                            Utils::CamxResultToString(result));
                        break;
                    }

                    // Need to save only input port information
                    if (ChiStreamTypeBidirectional == pChiStream->streamType)
                    {
                        /// @todo (CAMX-4925) add support for multiple port per stream.
                        pChiStreamWrapper->SetPortId(pBufferDescriptor->pNodePort[0].nodePortId);
                        pChiStreamWrapper->SetNumberOfPortId(pBufferDescriptor->numNodePorts);
                        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Bidirectional %d, portId %d wd ht %d x %d wrapper %x stream %x",
                            input, pBufferDescriptor->pNodePort[0].nodePortId, pChiStream->width, pChiStream->height,
                            pChiStreamWrapper, pChiStream);
                    }
                }
            }
        }
        else
        {
            SensorInfo* pSensorInfo = &pPipelineDescriptor->inputData[0].sensorInfo;

            pSensorInfo->cameraId = pPipelineInput->sensorInfo.cameraId;
            Utils::Memcpy(&pSensorInfo->sensorMode, pPipelineInput->sensorInfo.pSensorModeInfo, sizeof(ChiSensorModeInfo));
            if ((60 <= (pSensorInfo->sensorMode.frameRate / pSensorInfo->sensorMode.batchedFrames)) ||
                (8 <= pSensorInfo->sensorMode.batchedFrames))
            {
                // Extra HAL request is needed
                // 1. when effective frame rate is 60FPS or more.
                // 2. when batch size is 8 or moe
                requireExtraHalRequest = TRUE;
            }
        }
    }

    DeferredRequestQueueCreateData pDeferredCreateData;
    pDeferredCreateData.numPipelines   = pCreateData->numPipelines;
    pDeferredCreateData.pThreadManager = m_pThreadManager;

    for (UINT i = 0; ((i < pCreateData->numPipelines) && (result == CamxResultSuccess)); i++)
    {
        ChiPipelineInfo*    pPipelineInfo       = &pCreateData->pPipelineInfo[i];
        PipelineDescriptor* pPipelineDescriptor = reinterpret_cast<PipelineDescriptor*>(pPipelineInfo->hPipelineDescriptor);

        m_pipelineData[i].pPipelineDescriptor             = pPipelineDescriptor;
        m_pipelineData[i].pPipeline                       = m_pChiContext->CreatePipelineFromDesc(pPipelineDescriptor, i);

        if (NULL == m_pipelineData[i].pPipeline)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline creation failed for %d pipeline", i);
            result = CamxResultEFailed;
            break;
        }

        pPipelineInfo->pipelineOutputInfo.hPipelineHandle = m_pipelineData[i].pPipeline;

        ChiPipelineInputInfo* pPipelineInput      = &pPipelineInfo->pipelineInputInfo;

        for (UINT j = 0; j < pPipelineDescriptor->pipelineInfo.numNodes; j++)
        {
            if (pPipelineDescriptor->pipelineInfo.pNodeInfo[j].nodeId == ExtSensorNodeId)
            {
                CAMX_LOG_CONFIG(CamxLogGroupCore, "Storing CSL session %p", GetCSLSession());
                HwEnvironment* pHWEnvironment = HwEnvironment::GetInstance();

                if (NULL != pHWEnvironment)
                {
                    if (CSLInvalidHandle != GetCSLSession())
                    {
                        pHWEnvironment->InitializeSensorHwDeviceCache(pPipelineInput->sensorInfo.cameraId, NULL,
                                                                      GetCSLSession(), 1, NULL, NULL);
                    }
                }
            }
        }

        UINT32 frameDelay = m_pipelineData[i].pPipeline->DetermineFrameDelay();
        UINT32 extraFrameworkBuffers = m_pipelineData[i].pPipeline->DetermineExtrabuffersNeeded();

        if (0 != frameDelay)
        {
            for (UINT output = 0; output < pPipelineDescriptor->numOutputs; output++)
            {
                ChiStreamWrapper* pChiStreamWrapper = pPipelineDescriptor->outputData[output].pOutputStreamWrapper;
                // After successfully creating the pipeline, need to set the maximum num of native buffers for the stream
                if (TRUE == pChiStreamWrapper->IsVideoStream())
                {
                    UINT32 maxNumBuffers = (pStaticSettings->maxHalRequests + frameDelay) *
                                                GetBatchedHALOutputNum(pPipelineDescriptor->numBatchedFrames,
                                                                       pPipelineDescriptor->HALOutputBufferCombined);
                    pChiStreamWrapper->SetNativeMaxNumBuffers(maxNumBuffers);
                }
                else if ((extraFrameworkBuffers > 0) && (TRUE == pChiStreamWrapper->IsPreviewStream()))
                {
                    // Need extra buffers in EIS usecase and dewarp chinode node will publish the required count
                    // If the dewarp node is not present in pipeline extraFrameworkBuffers is 0
                    UINT32 maxNumBuffers = pStaticSettings->maxHalRequests + extraFrameworkBuffers;
                    pChiStreamWrapper->SetNativeMaxNumBuffers(maxNumBuffers);
                }
            }
        }

        if (frameDelay > additionalNeededRequests)
        {
            additionalNeededRequests = frameDelay;
        }
    }

    // Read from session meta to determine use resource manager or not
    if ((CamxResultSuccess == result) &&
        (NULL != m_pChiContext->GetResourceManager()))
    {
        for (UINT i = 0; i < pCreateData->numPipelines; i++)
        {
            if (TRUE == m_pipelineData[i].pPipeline->IsRealTime())
            {
                UINT32*      pUseResManager = NULL;
                UINT32       resManagerTag  = 0;

                VendorTagManager::QueryVendorTagLocation("org.quic.camera.resource", "enable", &resManagerTag);

                MetaBuffer* pSessionMeta = m_pipelineData[i].pPipelineDescriptor->pSessionMetadata;

                if ((0 != resManagerTag) && (NULL != pSessionMeta))
                {
                    pUseResManager = static_cast<UINT32*>(pSessionMeta->GetTag(resManagerTag));
                }

                if ((NULL != pUseResManager) && (TRUE == *pUseResManager))
                {
                    m_useResourceManager[i] = TRUE;
                    CAMX_LOG_CONFIG(CamxLogGroupCore, "Using resource manager for pipeline[%d]: %s",
                        i, m_pipelineData[i].pPipeline->GetPipelineName());
                }
            }

            if (TRUE == UsingResourceManager(i))
            {
                ResourceMgrCallbackOps resMgrCbs         = { 0 };
                ClientInputInfo        clientInputInfo   = { 0 };
                ResourceInputInfo      resourceInputInfo = { 0 };
                ResourceManager*       pResourceManager  =  m_pChiContext->GetResourceManager();

                resourceInputInfo.pResourceName          = "RealtimePipeline";
                resourceInputInfo.resourceId             = static_cast<ResourceID>(ResourceType::RealtimePipeline);
                resourceInputInfo.totalAvailableResource = 100;

                resMgrCbs.AcquireResourceFunc            = Session::ProcessAcquireResource;
                resMgrCbs.ReleaseResourceFunc            = Session::ProcessReleaseResource;

                clientInputInfo.pClientName              = m_pipelineData[i].pPipeline->GetPipelineName();
                clientInputInfo.maxResourceCost          = 100;
                clientInputInfo.minResourceCost          = clientInputInfo.maxResourceCost;
                clientInputInfo.hClient                  = static_cast<VOID*>(m_pipelineData[i].pPipelineDescriptor);
                clientInputInfo.pCallbackOps             = &resMgrCbs;
                clientInputInfo.pPrivateData             = static_cast<VOID*>(this);

                pResourceManager->RegisterResourceWithClient(&resourceInputInfo, &clientInputInfo);
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        numMetadataSlots = DefaultPerFramePoolWindowSize + (additionalNeededRequests * 2);

        if (GetBatchedHALOutputNum(pCreateData->usecaseNumBatchedFrames, pCreateData->HALOutputBufferCombined) > 1)
        {
            numMetadataSlots = DefaultPerFramePoolWindowSize * 2;
        }
        CAMX_ASSERT(numMetadataSlots <= MaxPerFramePoolWindowSize);

        for (UINT i = 0; i < pCreateData->numPipelines; i++)
        {
            ChiPipelineInfo* pPipelineInfo = &pCreateData->pPipelineInfo[i];

            if ((TRUE == m_pipelineData[i].pPipeline->IsRealTime()) ||
                (TRUE == m_pipelineData[i].pPipeline->IsDelayedPipeline()))
            {
                m_pipelineData[i].pPipeline->InitializeMetadataPools(numMetadataSlots);
            }
            else
            {
                m_pipelineData[i].pPipeline->InitializeMetadataPools(DefaultRequestQueueDepth);
            }

            if (pPipelineInfo->pipelineInputInfo.isInputSensor)
            {
                PipelineDescriptor* pPipelineDescriptor = GetPipelineDescriptor(pPipelineInfo->hPipelineDescriptor);

                m_pipelineData[i].pPipeline->SetSensorSyncMode(GetSensorSyncMode(pPipelineDescriptor));
            }

            pDeferredCreateData.pMainPools[i]     = m_pipelineData[i].pPipeline->GetPerFramePool(PoolType::PerFrameResult);

            if (TRUE == pPipelineInfo->pipelineInputInfo.isInputSensor)
            {
                MetadataPool* pUsecasePool    = m_pipelineData[i].pPipeline->GetPerFramePool(PoolType::PerUsecase);
                MetadataSlot* pSlot           = pUsecasePool->GetSlot(0);

                StatsStreamInitConfig statsStreamInitConfig = { };
                statsStreamInitConfig.operationMode         = m_statsOperationMode;

                pSlot->SetMetadataByTag(PropertyIDUsecaseStatsStreamInitConfig, &statsStreamInitConfig,
                                        sizeof(statsStreamInitConfig),
                                        "camx_session");

                pSlot->PublishMetadata(PropertyIDUsecaseStatsStreamInitConfig);

                m_numRealtimePipelines++;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        m_ppPerFrameDebugDataPool = static_cast<MetadataPool**>(CAMX_CALLOC(pCreateData->numPipelines * sizeof(MetadataPool*)));
        if (NULL != m_ppPerFrameDebugDataPool)
        {
            UINT pipelineIndex = 0;
            for (pipelineIndex = 0; pipelineIndex < pCreateData->numPipelines; pipelineIndex++)
            {
                m_ppPerFrameDebugDataPool[pipelineIndex] = MetadataPool::Create(PoolType::PerFrameDebugData,
                                                                                UINT_MAX,
                                                                                m_pThreadManager,
                                                                                numMetadataSlots,
                                                                                "Session",
                                                                                0);

            }

            for (pipelineIndex = 0; pipelineIndex < pCreateData->numPipelines; pipelineIndex++)
            {
                if (NULL == m_ppPerFrameDebugDataPool[pipelineIndex])
                {
                    result = CamxResultENoMemory;
                    break;
                }
            }

            if (CamxResultSuccess == result)
            {
                // Allocate internal memory for debug-data pool. This is the memory where the nodes write the actual data.
                if (TRUE == UseInternalDebugDataMemory())
                {
                    CAMX_LOG_CONFIG(CamxLogGroupDebugData,
                                    "DebugDataAll: RT: %u, DD_slots: %u to allocate. 3A enable: %u (%s), Tuning enable: %u "
                                    "Per-module sizes: AEC= %u AWB= %u AF= %u IFE= %u IPE= %u BPS= %u",
                                    m_isRealTime,
                                    numMetadataSlots,
                                    ((TRUE == pStaticSettings->enable3ADebugData) ||
                                     (TRUE == pStaticSettings->enableConcise3ADebugData)),
                                    ((TRUE == pStaticSettings->enable3ADebugData) ? "Default"
                                             : ((TRUE == pStaticSettings->enableConcise3ADebugData) ? "Concise" : "None")),
                                    pStaticSettings->enableTuningMetadata,
                                    ((TRUE == pStaticSettings->enable3ADebugData) ? pStaticSettings->debugDataSizeAEC
                                                                                  : pStaticSettings->conciseDebugDataSizeAEC),
                                    ((TRUE == pStaticSettings->enable3ADebugData) ? pStaticSettings->debugDataSizeAWB
                                                                                  : pStaticSettings->conciseDebugDataSizeAWB),
                                    ((TRUE == pStaticSettings->enable3ADebugData) ? pStaticSettings->debugDataSizeAF
                                                                                  : pStaticSettings->conciseDebugDataSizeAF),
                                    pStaticSettings->tuningDumpDataSizeIFE,
                                    pStaticSettings->tuningDumpDataSizeIPE,
                                    pStaticSettings->tuningDumpDataSizeBPS);
                    AllocateDebugDataPool(&m_pDebugDataBuffer, numMetadataSlots * pCreateData->numPipelines);
                    if (NULL == m_pDebugDataBuffer)
                    {
                        CAMX_LOG_WARN(CamxLogGroupDebugData, "Fail to allocate debug-data pool memory");
                    }
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupDebugData, "RT: %u, 3A Debug-Data & Tuning-metadata: disable",
                                  m_isRealTime);
                }
            }
        }
        else
        {
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        UINT32 currentRequestDepth                  = DefaultRequestQueueDepth + additionalNeededRequests;
        pDeferredCreateData.requestQueueDepth       = currentRequestDepth;

        m_usecaseNumBatchedFrames                   = pCreateData->usecaseNumBatchedFrames;
        m_notificationListSize                      = NotificationListSize * m_usecaseNumBatchedFrames;
        m_partialMetadataListSize                   = m_notificationListSize;
        m_notifyMessages.pNotifyMessage             = static_cast<ChiMessageDescriptor*>(CAMX_CALLOC(m_notificationListSize *
                                         sizeof(ChiMessageDescriptor)));

        if (NULL == m_notifyMessages.pNotifyMessage)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory and the Notificationlistsize is %d",
                                         m_notificationListSize);
            result = CamxResultENoMemory;
        }

        m_PartialDataMessages.pPartialCaptureMessage = static_cast<ChiPartialCaptureResult*>
                                         (CAMX_CALLOC(m_partialMetadataListSize *  sizeof(ChiPartialCaptureResult)));

        if (NULL == m_PartialDataMessages.pPartialCaptureMessage)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory and the PartialMetadatalistsize is %d",
                                         m_partialMetadataListSize);
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            m_requestQueueDepth                          = currentRequestDepth *
                                         GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined);

            /// @todo (CAMX-2876) The 8 limit is artificial, and based on the frame number remapping array (m_fwFrameNumberMap)
            CAMX_ASSERT(m_usecaseNumBatchedFrames <= 16);

            m_numPipelines             = pCreateData->numPipelines;

            m_pResultLock                       = Mutex::Create("SessionResultLock");
            m_pWaitForResultsLock               = Mutex::Create("WaitForResultsLock");
            m_pRequestLock                      = Mutex::Create("SessionRequestLock");
            m_pFlushLock                        = Mutex::Create("SessionFlushLock");
            m_pStreamOnOffLock                  = Mutex::Create("SessionStreamOnOffLock");
            m_pSessionDumpLock                  = Mutex::Create("SessionDumpLock");
            m_pResultHolderListLock             = Mutex::Create("ResultHolderListLock");
            m_resultStreamBuffers.pLock         = Mutex::Create("ResultStreamBuffers.pLock");
            m_notifyMessages.pLock              = Mutex::Create("NotifyMessages.pLock");
            m_PartialDataMessages.pLock         = Mutex::Create("PartialDataMessages.pLock");
            m_pPendingMBQueueLock               = Mutex::Create("PendingMBDoneLock");

            m_pDeferredRequestQueue             = DeferredRequestQueue::Create(&pDeferredCreateData);
            m_pWaitAllResultsAvailable          = Condition::Create("SessionWaitAllResultsAvailable");
            m_pWaitFlushResultsAvailable        = Condition::Create("SessionWaitFlushResultsAvailable");
            m_pCaptureResult                    = static_cast<ChiCaptureResult*>(
                                                        CAMX_CALLOC(m_requestQueueDepth *
                                                        GetBatchedHALOutputNum(m_usecaseNumBatchedFrames,
                                                                               m_HALOutputBufferCombined) *
                                                        m_numPipelines * sizeof(ChiCaptureResult)));

            m_pWaitLivePendingRequests     = Condition::Create("WaitInFlightRequests");
            m_pLivePendingRequestsLock     = Mutex::Create("InFlightRequests");
            m_livePendingRequests = 0;

            m_pFlushDoneLock    = Mutex::Create("SessionFlushDoneLock");
            m_pWaitForFlushDone = Condition::Create("WaitForFlushDone");

            if (TRUE == requireExtraHalRequest)
            {
                additionalNeededRequests += 1;
            }

            m_maxLivePendingRequests = (pStaticSettings->maxHalRequests + additionalNeededRequests) *
                GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined);
            m_defaultMaxLivePendingRequests = (pStaticSettings->maxHalRequests + additionalNeededRequests);

            CAMX_LOG_INFO(CamxLogGroupCore, "%s: m_requestQueueDepth=%u m_usecaseNumBatchedFrames=%u",
                          m_pipelineNames, m_requestQueueDepth, m_usecaseNumBatchedFrames);
            m_pRequestQueue = HAL3Queue::Create(m_requestQueueDepth,
                                                GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined),
                                                CreatedAs::Empty);

            if (NULL != m_pCaptureResult)
            {
                UINT32 numOutputBuffers = m_requestQueueDepth * m_numPipelines *
                    GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined);
                for (UINT32 i = 0; i < numOutputBuffers; i++)
                {
                    m_pCaptureResult[i].pOutputBuffers =
                        static_cast<ChiStreamBuffer*>(CAMX_CALLOC(MaxNumOutputBuffers * sizeof(ChiStreamBuffer)));

                    if (NULL == m_pCaptureResult[i].pOutputBuffers)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory");
                        result = CamxResultENoMemory;
                        break;
                    }
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            m_pStreamBufferInfo = static_cast<StreamBufferInfo*>(
                CAMX_CALLOC(sizeof(StreamBufferInfo) *
                            MaxPipelinesPerSession *
                            GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined)));

            if (NULL != m_pStreamBufferInfo)
            {
                for (UINT32 i = 0; i < MaxPipelinesPerSession; i++)
                {
                    m_captureRequest.requests[i].pStreamBuffers =
                        &m_pStreamBufferInfo[i *
                            GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined)];
                }
            }
            else
            {
                result = CamxResultENoMemory;
            }
        }

        if (CamxResultSuccess == result)
        {
            if ((NULL == m_pRequestQueue)                   ||
                (NULL == m_pResultLock)                     ||
                (NULL == m_pWaitForResultsLock)             ||
                (NULL == m_pResultHolderListLock)           ||
                (NULL == m_resultStreamBuffers.pLock)       ||
                (NULL == m_notifyMessages.pLock )           ||
                (NULL == m_PartialDataMessages.pLock)       ||
                (NULL == m_pRequestLock)                    ||
                (NULL == m_pStreamBufferInfo)               ||
                (NULL == m_pStreamOnOffLock)                ||
                (NULL == m_pDeferredRequestQueue)           ||
                (NULL == m_pCaptureResult)                  ||
                (NULL == m_pWaitLivePendingRequests)        ||
                (NULL == m_pLivePendingRequestsLock)        ||
                (NULL == m_pPendingMBQueueLock)             ||
                (NULL == m_pWaitFlushResultsAvailable)      ||
                (NULL == m_pWaitAllResultsAvailable))
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory");
                result = CamxResultENoMemory;
            }
            else
            {
                if (CamxResultSuccess == result)
                {
                    // Publish ChiSensorModeInfo structure to Vendor tags
                    UINT32             metaTag         = 0;
                    MetadataPool*      pPool           = NULL;
                    MetadataSlot*      pSlot           = NULL;
                    ChiSensorModeInfo* pSensorModeInfo = NULL;
                    result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sensor_meta_data",
                                                                      "sensor_mode_info",
                                                                      &metaTag);
                    CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "Failed to get vendor tag: sensor_mode_info");

                    for (UINT i = 0; i < m_numPipelines; i++)
                    {
                        pPool = m_pipelineData[i].pPipeline->GetPerFramePool(PoolType::PerUsecase);
                        pSlot = pPool->GetSlot(0);
                        pSensorModeInfo = &m_pipelineData[i].pPipelineDescriptor->inputData[0].sensorInfo.sensorMode;

                        pSlot->SetMetadataByTag(metaTag,
                                                static_cast<VOID*>(pSensorModeInfo),
                                                sizeof(ChiSensorModeInfo),
                                                "camx_session");

                        pSlot->PublishMetadataList(&metaTag, 1);

                    }
                }
                result = InitializeNewPipelines(pCreateData);
            }
        }
    }

    CamxResult resultCode = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sensor_meta_data",
                                                                     "sensor_mode_index",
                                                                     &m_vendorTagSensorModeIndex);
    if (CamxResultSuccess != resultCode)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore,
                        "Failed to find org.codeaurora.qcamera3.sensor_meta_data.sensor_mode_index, resultCode=%s",
                       Utils::CamxResultToString(resultCode));
    }

    resultCode = VendorTagManager::QueryVendorTagLocation("org.quic.camera.qtimer", "timestamp", &m_vendorTagIndexTimestamp);
    if (CamxResultSuccess != resultCode)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore,
                       "Failed to find org.quic.camera.qtimer.timestamp, encoder will fallback to system time, resultCode=%s",
                       Utils::CamxResultToString(resultCode));
    }

    resultCode = VendorTagManager::QueryVendorTagLocation("org.quic.camera.cvpMetaData", "CVPMetaData", &m_indexCVPMetaData);
    if (CamxResultSuccess != resultCode)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore,
                       "Failed to find org.quic.camera.cvpMetaData, resultCode=%s",
                       Utils::CamxResultToString(resultCode));
    }

    resultCode = VendorTagManager::QueryVendorTagLocation("org.quic.camera.recording", "endOfStream",
                                                          &m_recordingEndOfStreamTagId);
    if (CamxResultSuccess != resultCode)
    {
        m_recordingEndOfStreamTagId = 0;
        CAMX_LOG_ERROR(CamxLogGroupCore,
                       "Failed to find org.quic.camera.recording.endOfStream, resultCode=%s",
                       Utils::CamxResultToString(resultCode));
    }

    resultCode = VendorTagManager::QueryVendorTagLocation("org.quic.camera.recording", "endOfStreamRequestId",
                                                          &m_recordingEndOfStreamRequestIdTagId);
    if (CamxResultSuccess != resultCode)
    {
        m_recordingEndOfStreamRequestIdTagId = 0;
        CAMX_LOG_ERROR(CamxLogGroupCore,
                       "Failed to find org.quic.camera.recording.endOfStreamRequestId, resultCode=%s",
                       Utils::CamxResultToString(resultCode));
    }

    resultCode = VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainfo", "SyncMode",
        &m_sessionSync.syncModeTagID);
    if (CamxResultSuccess != resultCode)
    {
        m_sessionSync.syncModeTagID = 0;
        CAMX_LOG_ERROR(CamxLogGroupCore,
            "Failed to find com.qti.chi.multicamerainfo.SyncMode, resultCode=%s",
            Utils::CamxResultToString(resultCode));
    }

    resultCode = VendorTagManager::QueryVendorTagLocation("org.quic.camera.streamTypePresent", "preview",
                                                          &m_previewStreamPresentTagId);
    if (CamxResultSuccess != resultCode)
    {
        m_previewStreamPresentTagId = 0;
        CAMX_LOG_ERROR(CamxLogGroupCore,
                       "Failed to find org.quic.camera.streamTypePresent.preview, resultCode=%s",
                       CamxResultStrings[resultCode]);
    }

    resultCode = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.iso_exp_priority", "select_priority",
                                                          &m_exposurePriorityModeTagId);
    if (CamxResultSuccess != resultCode)
    {
        m_exposurePriorityModeTagId = 0;
        CAMX_LOG_ERROR(CamxLogGroupCore,
                       "Failed to find org.codeaurora.qcamera3.iso_exp_priority.select_priority, resultCode=%s",
                       CamxResultStrings[resultCode]);
    }

    if (CamxResultSuccess == result)
    {
        // Reset the pending queue
        ResetMetabufferPendingQueue();
        CAMX_LOG_CONFIG(CamxLogGroupCore, "Session (%p) Initialized", this);
        m_sesssionInitComplete = TRUE;
    }
    else
    {
        // Free if initialization fails
        if (NULL != m_pDebugDataBuffer)
        {
            CAMX_FREE(m_pDebugDataBuffer);
            m_pDebugDataBuffer = NULL;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::CSLSessionMessageHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::CSLSessionMessageHandler(
    VOID*        pUserData,
    CSLMessage*  pMessage)
{
    if ((NULL == pUserData) || (NULL == pMessage))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Unexpected CSL Session or Msghandler data !!");
        return;
    }

    Pipeline* pPipeline = NULL;
    Session*  pSession  = static_cast<Session*>(pUserData);

    for (UINT i = 0; i < pSession->m_numPipelines; i++)
    {
        pPipeline = pSession->m_pipelineData[i].pPipeline;
        switch (pMessage->type)
        {
            case CSLMessageTypeError:
                pPipeline->SendErrorNotification(&(pMessage->message.errorMessage));
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupCore, "Unexpected CSL message type");
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::InitializeNewPipelines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::InitializeNewPipelines(
    SessionCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCreateData);
    CAMX_ASSERT(m_usecaseNumBatchedFrames == pCreateData->usecaseNumBatchedFrames);

    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    for (UINT32 i = 0; i < m_numPipelines; i++)
    {
        m_requestBatchId[i] = CamxInvalidRequestId;
    }

    for (UINT i = 0; i < pCreateData->numPipelines; i++)
    {
        if ((TRUE == m_pipelineData[i].pPipeline->IsRealTime()) || (TRUE == IsTorchWidgetSession()))
        {
            result = FinalizePipeline(pCreateData,
                                      i,
                                      pStaticSettings->enableQTimer);
        }
        else
        {
            result = m_pipelineData[i].pPipeline->CheckOfflinePipelineInputBufferRequirements();
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "FinalizePipeline(%d) failed!", i);
            break;
        }
    }

    for (UINT i = 0; i < pCreateData->numPipelines; i++)
    {
        if (TRUE == m_pipelineData[i].pPipeline->CheckIPERTPipeline())
        {
            SetIPERTPipeline(TRUE);
        }
    }

    CamxResult resultCode = CSLRegisterSessionMessageHandler(GetCSLSession(),
                                CSLSessionMessageHandler, static_cast<VOID*>(this));
    if (CamxResultSuccess != resultCode)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to register Offline message handler");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::NotifyResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::NotifyResult(
    ResultsData* pResultsData)
{
    CAMX_ASSERT(NULL != pResultsData);
    switch (pResultsData->type)
    {
        case CbType::Error:
            HandleErrorCb(&pResultsData->cbPayload.error, pResultsData->pipelineIndex, pResultsData->pPrivData);
            break;

        case CbType::Async:
            HandleAsyncCb(&pResultsData->cbPayload.async, pResultsData->pPrivData);
            break;

        case CbType::Metadata:
            HandleMetadataCb(&pResultsData->cbPayload.metadata,
                             pResultsData->pPrivData,
                             pResultsData->pipelineIndex);
            break;

        case CbType::PartialMetadata:
            HandlePartialMetadataCb(&pResultsData->cbPayload.partialMetadata,
                pResultsData->pPrivData,
                pResultsData->pipelineIndex);
            break;

        case CbType::EarlyMetadata:
            HandleEarlyMetadataCb(&pResultsData->cbPayload.metadata, pResultsData->pipelineIndex, pResultsData->pPrivData);
            break;

        case CbType::Buffer:
            HandleBufferCb(&pResultsData->cbPayload.buffer, pResultsData->pipelineIndex,
                           pResultsData->pPrivData);
            break;

        case CbType::SOF:
            HandleSOFCb(&pResultsData->cbPayload.sof);
            break;

        case CbType::MetaBufferDone:
            HandleMetaBufferDoneCb(&pResultsData->cbPayload.metabufferDone);
            break;

        default:
            break;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::StreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::StreamOn(
    CHIPIPELINEHANDLE hPipelineDescriptor)
{
    UINT32     index  = 0;
    CamxResult result = CamxResultSuccess;

    // input pipelineIndex not really match the index recorded by Session, so use Descriptor to find it.
    for (index = 0; index < m_numPipelines; index++)
    {
        if (hPipelineDescriptor == m_pipelineData[index].pPipelineDescriptor)
        {
            // found corresponding pipeline can use index to get to it
            break;
        }
    }

    CAMX_ASSERT(index < m_numPipelines);

    Pipeline* pPipeline = m_pipelineData[index].pPipeline;

    m_pStreamOnOffLock->Lock();

    if ((NULL != pPipeline) && (PipelineStatus::STREAM_ON != pPipeline->GetPipelineStatus()))
    {
        PipelineStatus pipelineStatus = pPipeline->GetPipelineStatus();

        if (PipelineStatus::FINALIZED > pipelineStatus)
        {
            result = FinalizeDeferPipeline(index);
            pipelineStatus = pPipeline->GetPipelineStatus();
            CAMX_LOG_INFO(CamxLogGroupCore, "FinalizeDeferPipeline result: %d pipelineStatus: %d",
                result, pipelineStatus);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "FinalizeDeferPipeline() unsuccessful, Session StreamOn() is failed !!");
            pPipeline->ReleaseResources();
        }
        else
        {
            if (PipelineStatus::FINALIZED <= pipelineStatus)
            {
                result = pPipeline->StreamOn();

                if (CamxResultSuccess == result)
                {
                    if (TRUE == pPipeline->IsRealTime())
                    {
                        m_numStreamedOnRealtimePipelines++;

                        CheckAndSyncLinks();
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline %s failed to stream on.",
                        pPipeline->GetPipelineName());
                }
            }
        }
    }

    m_pStreamOnOffLock->Unlock();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::StreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::StreamOff(
    CHIPIPELINEHANDLE           hPipelineDescriptor,
    CHIDEACTIVATEPIPELINEMODE   modeBitmask)
{
    UINT32     index  = 0;
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != hPipelineDescriptor);
    if (NULL != hPipelineDescriptor)
    {
        // Input pipelineIndex not really match the index recorded by Session, so use Descriptor to find it.
        for (index = 0;
             ((index < m_numPipelines) && (hPipelineDescriptor != m_pipelineData[index].pPipelineDescriptor));
             index++);

        CAMX_ASSERT(index < m_numPipelines);
        if (index < m_numPipelines)
        {
            // Found corresponding pipeline
            Pipeline* pPipeline = m_pipelineData[index].pPipeline;

            if (NULL != pPipeline)
            {
                // if it is multicamera LPM or release buffer for offline pipeline, don't wait result available,
                // result waiting is handled in chi layer.
                if (FALSE == ((modeBitmask & CHIDeactivateModeSensorStandby) ||
                    (modeBitmask & CHIDeactivateModeReleaseBuffer)))
                {
                    result = pPipeline->WaitForAllNodesRequest();
                }

                if (CamxResultSuccess == result)
                {
                    m_pStreamOnOffLock->Lock();

                    // Disable AE lock if already set
                    if (TRUE == pPipeline->IsRealTime())
                    {
                        SetAELockRange(index, 0, 0);
                    }

                    if (PipelineStatus::STREAM_ON == pPipeline->GetPipelineStatus())
                    {
                        pPipeline->ClearPendingResources();
                    }

                    result = pPipeline->StreamOff(modeBitmask);

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline %s failed to stream off. result: %d",
                            pPipeline->GetPipelineName(), result);
                    }

                    if (TRUE == pPipeline->IsRealTime())
                    {
                        if (m_numStreamedOnRealtimePipelines > 0)
                        {
                            m_numStreamedOnRealtimePipelines--;
                        }
                        CheckAndSyncLinks();
                    }

                    m_pStreamOnOffLock->Unlock();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "pPipeline: %p", pPipeline);
                result = CamxResultEFailed;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Couldn't locate Pipeline (Handle/Descriptor: %p)", hPipelineDescriptor);
            result = CamxResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Pipeline Handle/Descriptor: %p", hPipelineDescriptor);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::ProcessCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::ProcessCaptureRequest(
    const ChiPipelineRequest* pPipelineRequests)
{
    CamxResult  result      = CamxResultSuccess;

    UINT        numRequests = pPipelineRequests->numRequests;
    UINT32      pipelineIndexes[MaxPipelinesPerSession];

    const StaticSettings* pStaticSettings = m_pChiContext->GetStaticSettings();

    CAMX_ASSERT(NULL != pPipelineRequests);
    CAMX_ASSERT(NULL != pStaticSettings);

    // Prepare info for each request on each pipeline
    for (UINT requestIndex = 0; requestIndex < numRequests; requestIndex++)
    {
        // input pipelineIndex not really match the index recorded by Session, so use GetPipelineIndex to get corresponding
        // pipeline index.
        pipelineIndexes[requestIndex] = GetPipelineIndex(pPipelineRequests->pCaptureRequests[requestIndex].hPipelineHandle);
        CAMX_LOG_VERBOSE(CamxLogGroupCore,
            "Received(%d/%d) for framework framenumber %llu, num outputs %d on %s: PipelineStatus:%d",
            requestIndex+1,
            numRequests,
            pPipelineRequests->pCaptureRequests[requestIndex].frameNumber,
            pPipelineRequests->pCaptureRequests[requestIndex].numOutputs,
            m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->GetPipelineIdentifierString(),
            m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->GetPipelineStatus());
    }

    if (CamxResultSuccess != m_pFlushLock->TryLock())
    {
        // Prepare capture result with request error for the pipeline requests but don't
        // dispatch it immediately as it would be dispatched at the end of flush call. This will
        // ensure that all capture results of the cancelled requests are dispatched first before
        // returning the control to the caller.
        // Decrementing live pending and sending processing done notification to allow flush to
        // process this inflight request's result after handling all enqueued requests
        m_pLivePendingRequestsLock->Lock();
        PrepareChiRequestErrorForInflightRequests(pPipelineRequests);
        m_pLivePendingRequestsLock->Unlock();

        NotifyProcessingDone();

        m_pFlushDoneLock->Lock();
        while (TRUE == GetFlushStatus())
        {
            // Block the thread and send result after flush is done
            result = m_pWaitForFlushDone->TimedWait(m_pFlushDoneLock->GetNativeHandle(), MaxWaitTimeForFlush);
        }
        m_pFlushDoneLock->Unlock();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupCore, "Flush done timed out for session: %p, but returing success as results"
                " should be processed!!", this);
            // returning success as result of this request should have already
            // been dispatched at the end of flush
            result = CamxResultSuccess;
        }

        return result;
    }

    m_pFlushLock->Unlock();

    m_pLivePendingRequestsLock->Lock();

    CAMX_ASSERT(m_maxLivePendingRequests > 0);

    while (m_livePendingRequests >= m_maxLivePendingRequests - 1)
    {
        if (TRUE == m_aDeviceInError)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Device in error state, returning failure for session:%p", this);
            m_pLivePendingRequestsLock->Unlock();
            return CamxResultEFailed;
        }

        if (TRUE == GetSessionTriggeringRecovery())
        {
            CAMX_LOG_WARN(CamxLogGroupCore, "Session %p triggering recovery, cancelling new PCR", this);
            m_pLivePendingRequestsLock->Unlock();
            return CamxResultECancelledRequest;
        }

        UINT waitTime = LivePendingRequestTimeoutDefault;

        if (m_sequenceId < m_maxLivePendingRequests * 2)
        {
            waitTime = LivePendingRequestTimeoutDefault + (m_maxLivePendingRequests * LivePendingRequestTimeOutExtendor);
        }

        UINT32 additionalExposureTime = CamxAtomicLoadU32(&m_aTotalLongExposureTimeout);
        if (TRUE == m_additionalWaitTimeForLivePending)
        {
            // after flush if current exposureTime used by sensor is more than the requested exposure time
            // then need to wait according to current exposureTime use by sensor. Because sensor doesnot
            // use requested exposure time upto first 3 frame(pipeline delay).
            if (additionalExposureTime < m_currExposureTimeUseBySensor)
            {
                additionalExposureTime = m_currExposureTimeUseBySensor;
            }
            additionalExposureTime = additionalExposureTime * 3;
            m_additionalWaitTimeForLivePending = FALSE;
        }
        waitTime = static_cast<UINT>(additionalExposureTime) + waitTime;

        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                         "Timed Wait Live Pending Requests(%u) "
                         "Sequence Id %u "
                         "Live Pending Requests %u "
                         "Max Live Pending Requests %u "
                         "Live Pending Request TimeOut Extendor %u",
                         waitTime,
                         m_sequenceId,
                         m_livePendingRequests,
                         m_maxLivePendingRequests,
                         LivePendingRequestTimeOutExtendor);

        result = m_pWaitLivePendingRequests->TimedWait(m_pLivePendingRequestsLock->GetNativeHandle(), waitTime);

        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                      "Timed Wait Live Pending Requests(%u) ...DONE result %s",
                       waitTime,
                       CamxResultStrings[result]);

        if (CamxResultSuccess != result)
        {
            break;
        }
    }

    if (CamxResultSuccess != result)
    {
        m_pLivePendingRequestsLock->Unlock();
        if (TRUE == pStaticSettings->enableRecovery)
        {
            if (TRUE == pStaticSettings->raiserecoverysigabrt)
            {
                DumpSessionState(SessionDumpFlag::ResetRecovery);
                CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: Raise SigAbort to debug the root cause of HAL recovery");
                OsUtils::RaiseSignalAbort();
            }
            else
            {
                CAMX_LOG_CONFIG(CamxLogGroupCore, "Lets do a Reset:UMD");
                // Set recovery status to TRUE
                SetSessionTriggeringRecovery(TRUE);

                NotifyPipelinesOfTriggeringRecovery(TRUE);

                DumpSessionState(SessionDumpFlag::ResetUMD);
                return CamxResultETimeout;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "HAL Recovery is disabled, cannot trigger");
            return CamxResultEFailed;
        }
    }

    for (UINT requestIndex = 0; requestIndex < numRequests; requestIndex++)
    {
        m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->IncrementLivePendingRequest();
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Framework frame number: %llu Pipeline: %s LivePendingRequests: %d",
            pPipelineRequests->pCaptureRequests[requestIndex].frameNumber,
            m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->GetPipelineIdentifierString(),
            m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->GetLivePendingRequest());
    }

    m_livePendingRequests++;
    m_pLivePendingRequestsLock->Unlock();

    if (CamxResultSuccess != m_pFlushLock->TryLock())
    {
        // Prepare capture result with request error for the pipeline requests but don't
        // dispatch it immediately as it would be dispatched at the end of flush call. This will
        // ensure that all capture results of the cancelled requests are dispatched first before
        // returning the control to the caller.
        // Decrementing live pending and sending processing done notification to allow flush to
        // process this inflight request's result after handling all enqueued requests
        m_pLivePendingRequestsLock->Lock();
        PrepareChiRequestErrorForInflightRequests(pPipelineRequests);
        PipelinesInflightRequestsNotification(pPipelineRequests);
        m_livePendingRequests--;
        m_pLivePendingRequestsLock->Unlock();

        NotifyProcessingDone();

        m_pFlushDoneLock->Lock();
        while (TRUE == GetFlushStatus())
        {
            // Block the thread and send result after flush is done
            result = m_pWaitForFlushDone->TimedWait(m_pFlushDoneLock->GetNativeHandle(), MaxWaitTimeForFlush);
        }
        m_pFlushDoneLock->Unlock();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupCore, "Flush done timed out for session: %p, but returing success as results"
                " should be processed!!", this);
            // returning success as result of this request should have already
            // been dispatched at the end of flush
            result = CamxResultSuccess;
        }

        return result;
    }

    // If it reaches here flush lock should already be taken

    // Block process request while stream on in progress
    m_pStreamOnOffLock->Lock();

    ChiCaptureRequest requests[MaxPipelinesPerSession];
    m_captureRequest.numRequests = numRequests;

    if (MaxRealTimePipelines > m_numRealtimePipelines)
    {
        // In single camera use case, one CHI request should have only one request per pipeline so that incoming requests will
        // not be more than m_requestQueueDepth and the only exception is in Dual Camera use case to have two requests
        if (2 <= numRequests)
        {
            CAMX_LOG_WARN(CamxLogGroupCore, "In batch mode, number of pipeline requests are more than 1");
        }
    }

    SyncProcessCaptureRequest(pPipelineRequests, pipelineIndexes);

    for (UINT requestIndex = 0; requestIndex < numRequests; requestIndex++)
    {
        // check resource availability before enqueue to requestQueue
        if ((CamxResultSuccess == result) &&
            (TRUE == UsingResourceManager(pipelineIndexes[requestIndex])))
        {
            ResourceID resourceId = static_cast<ResourceID>(ResourceType::RealtimePipeline);

            m_pChiContext->GetResourceManager()->AddResourceReference(resourceId,
                static_cast<VOID*>(m_pipelineData[pipelineIndexes[requestIndex]].pPipelineDescriptor), 0);
        }
    }

    for (UINT requestIndex = 0; requestIndex < numRequests; requestIndex++)
    {
        const ChiCaptureRequest* pCaptureRequest    = &(pPipelineRequests->pCaptureRequests[requestIndex]);
        UINT32             pipelineIndex            = pipelineIndexes[requestIndex];
        Pipeline*          pPipeline                = m_pipelineData[pipelineIndex].pPipeline;
        MetadataPool*      pPerFrameInputPool       = NULL;
        MetadataPool*      pPerFrameResultPool      = NULL;
        MetadataPool*      pPerFrameInternalPool    = NULL;
        MetadataPool*      pPerFrameEarlyResultPool = NULL;
        MetadataPool*      pPerUsecasePool          = NULL;
        MetaBuffer*        pInputMetabuffer         = NULL;
        MetaBuffer*        pOutputMetabuffer        = NULL;

        if (NULL == pPipeline)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "pPipeline is NULL, pipelineIndex %u requestIndex %u",
                pipelineIndex, requestIndex);
            result = CamxResultEFailed;
        }
        else if (PipelineStatus::FINALIZED > pPipeline->GetPipelineStatus())
        {
            result = FinalizeDeferPipeline(pipelineIndex);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "%s: FinalizeDeferPipeline failed pipelineIndex %u PipelineName: %s"
                    "result: %s", m_pipelineNames, pipelineIndex, pPipeline->GetPipelineName(),
                    CamxResultStrings[result]);
                pPipeline->ReleaseResources();
                break;
            }
        }

        if (CamxResultSuccess == result)
        {
            pPerFrameInputPool       = pPipeline->GetPerFramePool(PoolType::PerFrameInput);
            pPerFrameResultPool      = pPipeline->GetPerFramePool(PoolType::PerFrameResult);
            pPerFrameInternalPool    = pPipeline->GetPerFramePool(PoolType::PerFrameInternal);
            pPerFrameEarlyResultPool = pPipeline->GetPerFramePool(PoolType::PerFrameResultEarly);
            pPerUsecasePool          = pPipeline->GetPerFramePool(PoolType::PerUsecase);
        }

        if ((NULL != pPerFrameEarlyResultPool) &&
            (NULL != pPerFrameInputPool)       &&
            (NULL != pPerFrameResultPool)      &&
            (NULL != pPerFrameInternalPool)    &&
            (NULL != pPerUsecasePool))
        {
            // Replace the incoming frameNumber with m_sequenceId to protect against sparse input frameNumbers
            CamX::Utils::Memcpy(&requests[requestIndex], pCaptureRequest, sizeof(ChiCaptureRequest));

            pInputMetabuffer  = reinterpret_cast<MetaBuffer*>(requests[requestIndex].pInputMetadata);
            pOutputMetabuffer = reinterpret_cast<MetaBuffer*>(requests[requestIndex].pOutputMetadata);

            requests[requestIndex].frameNumber = m_sequenceId;
            m_sequenceId++;

            result = CanRequestProceed(&requests[requestIndex]);

            if (CamxResultSuccess == result)
            {
                result = WaitOnAcquireFence(&requests[requestIndex]);

                if (CamxResultSuccess == result)
                {
                    // Finally copy and enqueue the request and fire the threadpool

                    // Set the expected exposure time to the current default
                    UINT32 expectedExposureTime = 0;

                    // m_batchedFrameIndex of respective pipelines should be less than m_usecaseNumBatchedFrames
                    CAMX_ASSERT(m_batchedFrameIndex[pipelineIndex] < m_usecaseNumBatchedFrames);

                    // m_batchedFrameIndex 0 implies a new requestId must be generated - irrespective of batching
                    // ON/OFF status
                    if (0 == m_batchedFrameIndex[pipelineIndex])
                    {
                        m_requestBatchId[pipelineIndex]++;

                        CAMX_ASSERT(m_usecaseNumBatchedFrames >= m_captureRequest.requests[requestIndex].numBatchedFrames);
                        CaptureRequest::PartialClearData(&m_captureRequest.requests[requestIndex]);

                        m_captureRequest.requests[requestIndex].requestId = m_requestBatchId[pipelineIndex];
                        m_captureRequest.requests[requestIndex].pMultiRequestData =
                            &m_requestSyncData[(m_syncSequenceId) % MaxQueueDepth];
                        CAMX_LOG_VERBOSE(CamxLogGroupCore, "m_syncSequenceId:%d", m_syncSequenceId);
                        CAMX_LOG_VERBOSE(CamxLogGroupCore, "%s is handling RequestID:%llu whose PeerRequestID:%llu"
                            " m_syncSequenceId:%llu",
                            pPipeline->GetPipelineIdentifierString(),
                            m_captureRequest.requests[requestIndex].requestId,
                            m_requestSyncData[(m_syncSequenceId) % MaxQueueDepth],
                            m_syncSequenceId);

                        pPerFrameInputPool->Invalidate(m_requestBatchId[pipelineIndex]);
                        pPerFrameResultPool->Invalidate(m_requestBatchId[pipelineIndex]);
                        pPerFrameEarlyResultPool->Invalidate(m_requestBatchId[pipelineIndex]);
                        pPerFrameInternalPool->Invalidate(m_requestBatchId[pipelineIndex]);

                        pPerFrameInputPool->UpdateRequestId(m_requestBatchId[pipelineIndex]);
                        pPerFrameResultPool->UpdateRequestId(m_requestBatchId[pipelineIndex]);
                        pPerFrameEarlyResultPool->UpdateRequestId(m_requestBatchId[pipelineIndex]);
                        pPerFrameInternalPool->UpdateRequestId(m_requestBatchId[pipelineIndex]);
                        m_ppPerFrameDebugDataPool[pipelineIndex]->UpdateRequestId(m_requestBatchId[pipelineIndex]);

                        if (TRUE == UseInternalDebugDataMemory())
                        {
                            // Assign debug-data memory to the next request
                            VOID*           pSlotDebugData      = NULL;
                            VOID*           pBlobDebugData      = NULL;
                            MetadataSlot*   pDebugDataPoolSlot  =
                                m_ppPerFrameDebugDataPool[pipelineIndex]->GetSlot(m_requestBatchId[pipelineIndex]);

                            result = GetDebugDataForSlot(&pSlotDebugData);
                            if (CamxResultSuccess == result)
                            {
                                result = pDebugDataPoolSlot->GetPropertyBlob(&pBlobDebugData);
                            }
                            if (CamxResultSuccess == result)
                            {
                                CAMX_LOG_VERBOSE(CamxLogGroupDebugData,
                                                 "Assigning DebugData for request: %llu, debug-data: %p, pBlobDebugData: %p",
                                                 m_requestBatchId[pipelineIndex], pSlotDebugData, pBlobDebugData);
                                result = InitDebugDataSlot(pBlobDebugData, pSlotDebugData);
                            }

                            if (CamxResultSuccess != result)
                            {
                                // Debug-Data framework failures are non-fatal
                                CAMX_LOG_WARN(CamxLogGroupDebugData, "Fail to add debug-data to slot");
                                result = CamxResultSuccess;
                            }

                        }
                        else
                        {
                            // Assign debug-data memory to the next request
                            VOID*           pBlobDebugData      = NULL;
                            MetadataSlot*   pDebugDataPoolSlot  =
                                m_ppPerFrameDebugDataPool[pipelineIndex]->GetSlot(m_requestBatchId[pipelineIndex]);

                            pDebugDataPoolSlot->GetPropertyBlob(&pBlobDebugData);
                            CAMX_LOG_VERBOSE(CamxLogGroupDebugData,
                                             "Not setting debug-data: RT: %u request[%u]: %llu pBlob: %p : pSlot: %p",
                                             m_isRealTime,
                                             pipelineIndex,
                                             m_requestBatchId[pipelineIndex],
                                             pBlobDebugData);
                        }

                        if (TRUE == pStaticSettings->logMetaEnable)
                        {
                            CAMX_LOG_META("+----------------------------------------------------");
                            CAMX_LOG_META("| Input metadata for request: %lld", m_requestBatchId[pipelineIndex]);
                            CAMX_LOG_META("|     %d entries", pInputMetabuffer->Count());
                            CAMX_LOG_META("+----------------------------------------------------");

                            pInputMetabuffer->PrintDetails();
                        }


                        MetadataSlot* pMetadataSlot       = pPerFrameInputPool->GetSlot(m_requestBatchId[pipelineIndex]);
                        MetadataSlot* pResultMetadataSlot = pPerFrameResultPool->GetSlot(m_requestBatchId[pipelineIndex]);
                        MetadataSlot* pUsecasePoolSlot    = pPerUsecasePool->GetSlot(0);

                        if (pMetadataSlot != NULL)
                        {
                            result = pMetadataSlot->AttachMetabuffer(pInputMetabuffer);

                            if (CamxResultSuccess == result)
                            {
                                result = pResultMetadataSlot->AttachMetabuffer(pOutputMetabuffer);

                                if (CamxResultSuccess == result)
                                {
                                    UINT dumpMetadata = HwEnvironment::GetInstance()->GetStaticSettings()->dumpMetadata;

                                    CHAR metadataFileName[FILENAME_MAX];

                                    dumpMetadata &= (TRUE == pPipeline->IsRealTime()) ?  RealTimeMetadataDumpMask
                                        : OfflineMetadataDumpMask;

                                    if (0 != (dumpMetadata & 0x3))
                                    {
                                        OsUtils::SNPrintF(metadataFileName, FILENAME_MAX, "inputMetadata_%s_%5d.txt",
                                            pPipeline->GetPipelineIdentifierString(),
                                            m_requestBatchId[pipelineIndex]);

                                        pInputMetabuffer->DumpDetailsToFile(metadataFileName);
                                    }
                                    else if (0 != ((dumpMetadata>>2) & 0x3))
                                    {
                                        OsUtils::SNPrintF(metadataFileName, FILENAME_MAX, "inputMetadata_%s_%5d.bin",
                                            pPipeline->GetPipelineIdentifierString(),
                                            m_requestBatchId[pipelineIndex]);

                                        pInputMetabuffer->BinaryDump(metadataFileName);
                                    }
                                }
                                else
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupCore, "Error attach output failed for slot %d",
                                                   m_requestBatchId[pipelineIndex]);
                                }
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCore, "Error attach input failed for slot %d",
                                               m_requestBatchId[pipelineIndex]);
                            }
                            CAMX_LOG_INFO(CamxLogGroupCore, "AttachMetabuffer in pipeline %s InputMetaBuffer %p "
                                                            "OutputMetaBuffer %p reqId %llu",
                                          pPipeline->GetPipelineIdentifierString(),
                                          pInputMetabuffer,
                                          pOutputMetabuffer,
                                          m_requestBatchId[pipelineIndex]);

                            if (CamxResultSuccess == result)
                            {

                                INT64*   pExposurePriority        = NULL;
                                UINT64*  pSensorExposureTime      = static_cast<UINT64*>(pMetadataSlot->GetMetadataByTag(
                                                                               SensorExposureTime));

                                UINT64*  pSensorFrameDurationTime = static_cast<UINT64*>(pMetadataSlot->GetMetadataByTag(
                                                                               SensorFrameDuration));

                                INT32*   pExposurePriorityMode    = static_cast<INT32*>(pMetadataSlot->GetMetadataByTag(
                                                                               m_exposurePriorityModeTagId));

                                UINT8*   pAEMode                  = static_cast<UINT8*>(pMetadataSlot->GetMetadataByTag(
                                                                               ControlAEMode));
                                UINT8*   pControlMode             = static_cast<UINT8*>(pMetadataSlot->GetMetadataByTag(
                                                                               ControlMode));

                                UINT32    exposurePriorityTagId   = 0;
                                CDKResult resultCode              = VendorTagManager::QueryVendorTagLocation(
                                                                                "org.codeaurora.qcamera3.iso_exp_priority",
                                                                                "use_iso_exp_priority",
                                                                                &exposurePriorityTagId);

                                ControlAEModeValues AEMode       = ControlAEModeValues::ControlAEModeEnd;
                                ControlModeValues   controlMode  = ControlModeValues::ControlModeEnd;

                                if (CDKResultSuccess == resultCode)
                                {
                                    pExposurePriority = static_cast<INT64*>(pMetadataSlot->GetMetadataByTag(
                                                                            exposurePriorityTagId));
                                }

                                if ((NULL != pControlMode) && (NULL != pAEMode))
                                {
                                    AEMode      = *(reinterpret_cast<ControlAEModeValues*>(pAEMode));
                                    controlMode = *(reinterpret_cast<ControlModeValues*>(pControlMode));
                                }

                                INT32  exposureProrityMode    = *pExposurePriorityMode;

                                if ((1 == exposureProrityMode) && (NULL != pExposurePriority))
                                {
                                    expectedExposureTime = static_cast<UINT32>((*pExposurePriority) /
                                                                                static_cast<UINT64>(1000000));
                                }
                                else if (NULL != pSensorExposureTime)
                                {
                                    UINT32 exposureTimeInMs =
                                        static_cast<UINT32>((*pSensorExposureTime) / static_cast<UINT64>(1000000));
                                    if ( (exposureTimeInMs > expectedExposureTime) &&
                                         (TRUE == m_isRealTime) &&
                                         (TRUE == pStaticSettings->extendedTimeForLongExposure) &&
                                         ((ControlModeValues::ControlModeOff == controlMode) ||
                                          (ControlAEModeValues::ControlAEModeOff == AEMode)))
                                    {
                                        expectedExposureTime = exposureTimeInMs;
                                    }

                                    if ((NULL != pSensorFrameDurationTime) && (NULL != pExposurePriorityMode))
                                    {
                                        UINT32 sensorDurationTimeInMs =
                                            static_cast<UINT32>((*pSensorFrameDurationTime) / static_cast<UINT64>(1000000));

                                        if ((1 == exposureProrityMode) && (ControlAEModeValues::ControlAEModeOn == AEMode))
                                        {
                                            expectedExposureTime =
                                                    CamX::Utils::MaxUINT32(sensorDurationTimeInMs, expectedExposureTime);
                                        }
                                    }
                                }

                                // m_batchedFrameIndex of 0 implies batching may be switched ON/OFF starting from this frame
                                if (TRUE == IsUsecaseBatchingEnabled())
                                {
                                    RangeINT32* pFPSRange = static_cast<RangeINT32*>(pMetadataSlot->GetMetadataByTag(
                                                                                     ControlAETargetFpsRange));

                                    // Must have been filled by GetMetadataByTag()
                                    CAMX_ASSERT(NULL != pFPSRange);

                                    BOOL hasBatchingModeChanged = FALSE;

                                    if ((NULL != pFPSRange) && (pFPSRange->min == pFPSRange->max))
                                    {
                                        if (FALSE == m_isRequestBatchingOn)
                                        {
                                            hasBatchingModeChanged = TRUE;
                                        }

                                        m_isRequestBatchingOn = TRUE;
                                    }
                                    else
                                    {
                                        if (TRUE == m_isRequestBatchingOn)
                                        {
                                            hasBatchingModeChanged = TRUE;
                                        }

                                        m_isRequestBatchingOn = FALSE;
                                    }

                                    // If batching mode changes from ON to OFF or OFF to ON we need to dynamically adjust
                                    // m_requestQueueDepth - because m_requestQueueDepth is different with batching ON or
                                    // OFF With batching OFF it is RequestQueueDepth and with ON it is
                                    // "RequestQueueDepth * usecaseNumBatchedFrames"
                                    if (TRUE == hasBatchingModeChanged)
                                    {
                                        // Before changing m_requestQueueDepth, we need to make sure:
                                        // 1. All the current pending requests are processed by the Pipeline
                                        // 2. All the results for all those processed requests are sent back to the
                                        //    framework
                                        //
                                        // (1) is done by waiting for the request queue to become empty
                                        // (2) is done by waiting on a condition variable that is signaled when all results
                                        //     are sent back to the framework
                                        m_pRequestQueue->WaitEmpty();

                                        m_pLivePendingRequestsLock->Lock();
                                        m_livePendingRequests--;
                                        m_pLivePendingRequestsLock->Unlock();

                                        if (CamxResultSuccess != WaitTillAllResultsAvailable())
                                        {
                                            CAMX_LOG_WARN(CamxLogGroupCore,
                                                          "Failed to drain on batching mode change, calling flush");
                                            Flush();
                                        }

                                        m_pLivePendingRequestsLock->Lock();
                                        m_livePendingRequests++;
                                        m_pLivePendingRequestsLock->Unlock();

                                        // The request and result queues are completely empty at this point, and this
                                        // function is the only thing that can add to the request queue.  Safe to change
                                        // m_requestQueueDepth at this point
                                        if (TRUE == m_isRequestBatchingOn)
                                        {
                                            m_requestQueueDepth      =
                                                DefaultRequestQueueDepth *
                                                GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined);
                                            m_maxLivePendingRequests =
                                                m_defaultMaxLivePendingRequests *
                                                GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined);
                                        }
                                        else
                                        {
                                            m_requestQueueDepth      = DefaultRequestQueueDepth;
                                            m_maxLivePendingRequests = m_defaultMaxLivePendingRequests;
                                        }
                                    }
                                    else
                                    {
                                        // Need to set default value if batch mode is enabled but request batching is off.
                                        // In this case we have only preview reqest.
                                        if (FALSE == m_isRequestBatchingOn)
                                        {
                                            m_requestQueueDepth      = DefaultRequestQueueDepth;
                                            m_maxLivePendingRequests = m_defaultMaxLivePendingRequests;
                                        }
                                    }
                                }

                                if ((0 != m_recordingEndOfStreamTagId) && (0 != m_recordingEndOfStreamRequestIdTagId))
                                {
                                    UINT8* pRecordingEndOfStream = static_cast<UINT8*>(pMetadataSlot->GetMetadataByTag(
                                        m_recordingEndOfStreamTagId));

                                    if ((FALSE == pStaticSettings->disableDRQPreemptionOnStopRecord) &&
                                        ((NULL != pRecordingEndOfStream) && (0 != *pRecordingEndOfStream)))
                                    {
                                        UINT64 requestId = m_requestBatchId[pipelineIndex];
                                        CAMX_LOG_INFO(CamxLogGroupCore, "Recording stopped on reqId %llu", requestId);

                                        UINT32 endOfStreamRequestIdTag = m_recordingEndOfStreamRequestIdTagId;

                                        pUsecasePoolSlot->SetMetadataByTag(endOfStreamRequestIdTag,
                                                                           static_cast<VOID*>(&requestId),
                                                                           sizeof(requestId),
                                                                           "camx_session");

                                        pUsecasePoolSlot->PublishMetadataList(&endOfStreamRequestIdTag, 1);

                                        m_setVideoPerfModeFlag = TRUE;
                                        m_pDeferredRequestQueue->SetPreemptDependencyFlag(TRUE);
                                        m_pDeferredRequestQueue->DispatchReadyNodes();
                                    }
                                    else
                                    {
                                        m_setVideoPerfModeFlag = FALSE;
                                        m_pDeferredRequestQueue->SetPreemptDependencyFlag(FALSE);
                                    }
                                }
                                else
                                {
                                    CAMX_LOG_INFO(CamxLogGroupCore, "No stop recording vendor tags");
                                }

                                ControlCaptureIntentValues* pCaptureIntent = static_cast<ControlCaptureIntentValues*>(
                                    pMetadataSlot->GetMetadataByTag(ControlCaptureIntent));

                                // Update dynamic pipeline depth metadata which is required in capture result.
                                pResultMetadataSlot->SetMetadataByTag(RequestPipelineDepth,
                                                                      static_cast<VOID*>(&(m_requestQueueDepth)),
                                                                      1,
                                                                      "camx_session");

                                if (NULL != pCaptureIntent)
                                {
                                    // Copy Intent to result
                                    result = pResultMetadataSlot->SetMetadataByTag(ControlCaptureIntent, pCaptureIntent, 1,
                                                                                   "camx_session");
                                }
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCore, "Couldn't copy request metadata!");
                            }
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCore,
                                            "Couldn't get metadata slot for request id: %d",
                                            requests[requestIndex].frameNumber);

                            result = CamxResultEFailed;
                        }

                        // Get the per frame sensor mode index
                        UINT* pSensorModeIndex  = NULL;

                        if (m_vendorTagSensorModeIndex > 0)
                        {
                            if (NULL != pMetadataSlot)
                            {
                                pSensorModeIndex = static_cast<UINT*>(pMetadataSlot->GetMetadataByTag(
                                    m_vendorTagSensorModeIndex));
                            }

                            if (NULL != pSensorModeIndex)
                            {
                                pResultMetadataSlot->WriteLock();

                                pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();

                                if (TRUE == pStaticSettings->perFrameSensorMode)
                                {
                                    pResultMetadataSlot->SetMetadataByTag(PropertyIDSensorCurrentMode, pSensorModeIndex, 1,
                                                                          "camx_session");
                                    pResultMetadataSlot->PublishMetadata(PropertyIDSensorCurrentMode);
                                }

                                pResultMetadataSlot->Unlock();
                            }
                        }

                        // Check and update, if the preview stream is present in this request
                        if (m_previewStreamPresentTagId > 0)
                        {
                            BOOL isPreviewPresent = FALSE;
                            for (UINT32 i = 0; i < pCaptureRequest->numOutputs; i++)
                            {
                                CHISTREAM*        pStream    = pCaptureRequest->pOutputBuffers[i].pStream;
                                ChiStreamWrapper* pChiStream = static_cast<ChiStreamWrapper*>(pStream->pPrivateInfo);
                                if ((NULL != pChiStream) && (TRUE == pChiStream->IsPreviewStream()))
                                {
                                    isPreviewPresent = TRUE;
                                    break;
                                }
                            }
                            // Update the metadata tag.
                            pResultMetadataSlot->WriteLock();
                            pResultMetadataSlot->SetMetadataByTag(m_previewStreamPresentTagId,
                                                                  static_cast<VOID*>(&(isPreviewPresent)),
                                                                  1,
                                                                  "camx_session");
                            pResultMetadataSlot->PublishMetadata(m_previewStreamPresentTagId);
                            pResultMetadataSlot->Unlock();
                        }

                    }

                    if (CamxResultSuccess == result)
                    {
                        /// Adding 1 to avoid 0 as 0 is flagged as invalid
                        UINT64            cslsyncid         = pCaptureRequest->frameNumber + 1;
                        CaptureRequest*   pRequest          = &(m_captureRequest.requests[requestIndex]);
                        UINT              batchedFrameIndex = m_batchedFrameIndex[pipelineIndex];
                        ChiStreamWrapper* pChiStreamWrapper = NULL;
                        ChiStream*        pChiStream        = NULL;

                        m_lastCSLSyncId                                                     = cslsyncid;
                        pRequest->CSLSyncID                                                 = cslsyncid;
                        pRequest->expectedExposureTime                                      = expectedExposureTime;
                        pRequest->pPrivData                                                 = pCaptureRequest->pPrivData;
                        pRequest->pStreamBuffers[batchedFrameIndex].originalFrameworkNumber = pCaptureRequest->frameNumber;
                        pRequest->pStreamBuffers[batchedFrameIndex].numInputBuffers         = requests[requestIndex].numInputs;

                        pRequest->pStreamBuffers[batchedFrameIndex].sequenceId =
                            static_cast<UINT32>(requests[requestIndex].frameNumber);

                        for (UINT i = 0; i < requests[requestIndex].numInputs; i++)
                        {
                            /// @todo (CAMX-1015): Avoid this memcpy.
                            Utils::Memcpy(&pRequest->pStreamBuffers[batchedFrameIndex].inputBufferInfo[i].inputBuffer,
                                            &requests[requestIndex].pInputBuffers[i],
                                            sizeof(ChiStreamBuffer));

                            pChiStream = reinterpret_cast<ChiStream*>(
                                pRequest->pStreamBuffers[batchedFrameIndex].inputBufferInfo[i].inputBuffer.pStream);

                            if (NULL != pChiStream)
                            {
                                pChiStreamWrapper = reinterpret_cast<ChiStreamWrapper*>(pChiStream->pPrivateInfo);

                                CamxAtomicStoreU(&pRequest->pStreamBuffers[batchedFrameIndex].inputBufferInfo[i].fenceRefCount,
                                pChiStreamWrapper->GetNumberOfPortId());

                                if (0 == pRequest->pStreamBuffers[batchedFrameIndex].inputBufferInfo[i].fenceRefCount)
                                {
                                    CAMX_LOG_WARN(CamxLogGroupCore, "fenceRefCount shouldn't be zero for input buffer");

                                    // Assigning fence count to 1 for input buffer if it is 0.
                                    // this is to avoid any regressions due to source port sharing.
                                    pRequest->pStreamBuffers[batchedFrameIndex].inputBufferInfo[i].fenceRefCount = 1;
                                }

                                // Below check is ideally not required but to avoid
                                // regressions making it applicable to only MFNR/MFSR
                                if (requests[requestIndex].numInputs > 1)
                                {
                                    pRequest->pStreamBuffers[batchedFrameIndex].inputBufferInfo[i].portId =
                                    pChiStreamWrapper->GetPortId();
                                    CAMX_LOG_VERBOSE(CamxLogGroupCore,
                                    "input buffers #%d, port %d, dim %d x %d wrapper %x, stream %x fenceRefCount %d",
                                    i, pRequest->pStreamBuffers[batchedFrameIndex].inputBufferInfo[i].portId,
                                    pChiStream->width, pChiStream->height, pChiStreamWrapper, pChiStream,
                                    pRequest->pStreamBuffers[batchedFrameIndex].inputBufferInfo[i].fenceRefCount);
                                }
                            }
                        }

                        if (CamxResultSuccess == result)
                        {
                            /// @todo (CAMX-1797) Delete this
                            pRequest->pipelineIndex    = pipelineIndex;

                            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                                             "Submit to pipeline index: %d / number of pipelines: %d batched index %d",
                                             pRequest->pipelineIndex, m_numPipelines, m_batchedFrameIndex[pipelineIndex]);

                            CAMX_ASSERT(requests[requestIndex].numOutputs <= MaxOutputBuffers);

                            pRequest->pStreamBuffers[m_batchedFrameIndex[pipelineIndex]].numOutputBuffers =
                                requests[requestIndex].numOutputs;

                            for (UINT i = 0; i < requests[requestIndex].numOutputs; i++)
                            {
                                /// @todo (CAMX-1015): Avoid this memcpy.
                                Utils::Memcpy(&pRequest->pStreamBuffers[m_batchedFrameIndex[pipelineIndex]].outputBuffers[i],
                                              &requests[requestIndex].pOutputBuffers[i],
                                              sizeof(ChiStreamBuffer));
                            }

                            // Increment batch index only if batch mode is on
                            if (TRUE == m_isRequestBatchingOn)
                            {
                                m_batchedFrameIndex[pipelineIndex]++;
                                pRequest->numBatchedFrames          = m_usecaseNumBatchedFrames;
                                pRequest->HALOutputBufferCombined   = m_HALOutputBufferCombined;
                            }
                            else
                            {
                                m_batchedFrameIndex[pipelineIndex]  = 0;
                                pRequest->numBatchedFrames          = 1;
                                pRequest->HALOutputBufferCombined   = FALSE;
                            }

                        }
                    }

                    if (CamxResultSuccess == result)
                    {
                        // Fill Color Metadata for output buffer
                        result = SetPerStreamColorMetadata(pCaptureRequest, pPerFrameInputPool,
                                                           m_requestBatchId[pipelineIndex]);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Acquire fence failed for request");
                }
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupCore, "Session unable to process request because of device state");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "PerFrame MetadataPool is NULL");
            result = CamxResultEInvalidPointer;
        }
    }

    // Update multi request sync data
    if ((CamxResultSuccess == result) && (TRUE == IsMultiCamera()))
    {
        UpdateMultiRequestSyncData(pPipelineRequests);
    }

    if (CamxResultSuccess == result)
    {
        // Once we batch all the frames according to usecaseNumBatchedFrames we enqueue the capture request.
        // For non-batch mode m_usecaseNumBatchedFrames is 1 so we enqueue every request. If batching is ON
        // we enqueue the batched capture request only after m_usecaseBatchSize number of requests have been
        // received
        BOOL batchFrameReady = TRUE;
        for (UINT requestIndex = 0; requestIndex < numRequests; requestIndex++)
        {
            UINT32 pipelineIndex = pipelineIndexes[requestIndex];

            if (m_batchedFrameIndex[pipelineIndex] !=
                GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined))
            {
                batchFrameReady = FALSE;
                break; // batch frame number must be same for all the pipelines in same session
            }
        }

        if ((FALSE == m_isRequestBatchingOn) || (TRUE == batchFrameReady))
        {
            result = m_pRequestQueue->EnqueueWait(&m_captureRequest);

            if (CamxResultSuccess == result)
            {
                // Check for good conditions once more, if enqueue had to wait
                for (UINT requestIndex = 0; requestIndex < numRequests; requestIndex++)
                {
                    result = CanRequestProceed(&requests[requestIndex]);
                    if (CamxResultSuccess != result)
                    {
                        break;
                    }
                }
            }

            if (CamxResultSuccess == result)
            {
                for (UINT requestIndex = 0; requestIndex < numRequests; requestIndex++)
                {
                    const ChiCaptureRequest* pCaptureRequest = &(pPipelineRequests->pCaptureRequests[requestIndex]);
                    CAMX_LOG_CONFIG(CamxLogGroupCore,
                        "Pipeline: %s Added Sequence ID %lld CHI framenumber %lld to request queue and launched job "
                        "with request id %llu",
                        m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->GetPipelineIdentifierString(),
                        requests[requestIndex].frameNumber, pCaptureRequest->frameNumber,
                        m_requestBatchId[pipelineIndexes[requestIndex]]);
                }

                VOID* pData[] = {this, NULL};
                result        = m_pThreadManager->PostJob(m_hJobFamilyHandle,
                                                          NULL,
                                                          &pData[0],
                                                          FALSE,
                                                          FALSE);
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupCore, "Session unable to process request because of device state");
            }

            for (UINT requestIndex = 0; requestIndex < numRequests; requestIndex++)
            {
                m_batchedFrameIndex[pipelineIndexes[requestIndex]] = 0;
            }
        }
    }

    m_pStreamOnOffLock->Unlock();
    m_pFlushLock->Unlock();
    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::ProcessResultEarlyMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::ProcessResultEarlyMetadata(
    ResultHolder* pResultHolder,
    UINT*         pNumResults)
{
    UINT currentResult = *pNumResults;

    BOOL metadataAvailable = FALSE;

    // Dispatch metadata error, and no more metadata will be delivered for this frame
    if ((0 != pResultHolder->pendingMetadataCount) && (NULL != pResultHolder->pMetadataError))
    {
        CAMX_LOG_ERROR(CamxLogGroupHAL, "Metadata error for request: %d", pResultHolder->sequenceId);
        pResultHolder->pMetadataError->pPrivData = pResultHolder->pPrivData;
        DispatchNotify(pResultHolder->pMetadataError);
        pResultHolder->pendingMetadataCount = 0;
    }

    // There was no metadata error, concatenate and send all metadata results together in FIFO order
    if ((NULL == pResultHolder->pMetadataError) && (TRUE == MetadataReadyToFly(pResultHolder->sequenceId)) &&
        (FALSE == pResultHolder->isCancelled))
    {
        // The early metadata is always in slot [2].
        if (NULL != pResultHolder->pMetaBuffer[2])
        {
            // This change will be overridden once partial metadata is handled
            m_pCaptureResult[currentResult].pOutputMetadata = pResultHolder->pMetaBuffer[2];

            pResultHolder->pendingMetadataCount--;

            m_pCaptureResult[currentResult].numPartialMetadata = 1;
            m_pCaptureResult[currentResult].frameworkFrameNum = GetFrameworkFrameNumber(pResultHolder->sequenceId);

            pResultHolder->pMetaBuffer[1] = NULL;
            metadataAvailable = TRUE;

            CAMX_LOG_INFO(CamxLogGroupHAL,
                          "Finalized early metadata result for Sequence ID %d mapped to framework id %d",
                          pResultHolder->sequenceId,
                          m_pCaptureResult[currentResult].frameworkFrameNum);
        }
    }

    return metadataAvailable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::ProcessResultMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::ProcessResultMetadata(
    ResultHolder* pResultHolder,
    UINT*         pNumResults)
{
    UINT currentResult = *pNumResults;

    BOOL metadataAvailable = FALSE;

    UINT totalBuffersSent =
              (NULL != pResultHolder) ? (pResultHolder->numErrorBuffersSent + pResultHolder->numOkBuffersSent) : 0;

    if (NULL != pResultHolder)
    {
        // Dispatch metadata error, and no more metadata will be delivered for this frame
        if ((0 != pResultHolder->pendingMetadataCount) && (NULL != pResultHolder->pMetadataError))
        {
            if (FALSE == GetFlushStatus())
            {
                CAMX_LOG_ERROR(CamxLogGroupHAL, "Metadata error for sequenceId: %d", pResultHolder->sequenceId);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupHAL, "Metadata NULL during flush for sequenceId: %d", pResultHolder->sequenceId);
            }

            UINT32 queueIndex = pResultHolder->sequenceId % MaxQueueDepth;

            m_pendingMetabufferDoneQueue.pendingMask[queueIndex]   = (MetaBufferDoneMetaReady|MetaBufferDoneBufferReady);
            m_pendingMetabufferDoneQueue.pipelineIndex[queueIndex] = pResultHolder->pipelineIndex;
            m_pendingMetabufferDoneQueue.requestId[queueIndex]     = pResultHolder->requestId;
            if (pResultHolder->sequenceId > m_pendingMetabufferDoneQueue.latestSequenceId)
            {
                m_pendingMetabufferDoneQueue.latestSequenceId = pResultHolder->sequenceId;
            }

            pResultHolder->pMetadataError->pPrivData = pResultHolder->pPrivData;
            DispatchNotify(pResultHolder->pMetadataError);
            pResultHolder->pendingMetadataCount = 0;
        }
        // To handle the corner case when pending metadata is not received or metadata error is not received even
        // after metadata by the pipeline but all buffers are ready for dispatch. So, it is better to sending
        // metadata error notification here in order to avoid missing the opportunity to dispatch the result.
        else if ((pResultHolder->numOutBuffers == totalBuffersSent) && (pResultHolder->numErrorBuffersSent > 0) &&
                 (FALSE == pResultHolder->isCancelled) && (pResultHolder->pendingMetadataCount > 0))
        {
            CAMX_LOG_INFO(CamxLogGroupCore,
            "RequestId: %llu SequenceId: %llu Framenumber: %llu - All results were injected with buffer error,"
            " but an error notify has not been dispatched, dispatching now.",
            pResultHolder->requestId, pResultHolder->sequenceId,
            GetFrameworkFrameNumber(pResultHolder->sequenceId));

            ChiMessageDescriptor* pNotify = GetNotifyMessageDescriptor();

            if (NULL != pNotify)
            {
                pNotify->messageType                            = ChiMessageTypeError;
                pNotify->pPrivData                              = pResultHolder->pPrivData;
                pNotify->message.errorMessage.errorMessageCode  = static_cast<ChiErrorMessageCode>(MessageCodeResult);
                pNotify->message.errorMessage.frameworkFrameNum = GetFrameworkFrameNumber(pResultHolder->sequenceId);
                pNotify->message.errorMessage.pErrorStream      = NULL; // No stream applicable

                DispatchNotify(pNotify);
                pResultHolder->isCancelled = TRUE;
                pResultHolder->pendingMetadataCount = 0;

                UINT32 queueIndex = pResultHolder->sequenceId % MaxQueueDepth;

                m_pendingMetabufferDoneQueue.pendingMask[queueIndex]   = (MetaBufferDoneMetaReady|MetaBufferDoneBufferReady);
                m_pendingMetabufferDoneQueue.pipelineIndex[queueIndex] = pResultHolder->pipelineIndex;
                m_pendingMetabufferDoneQueue.requestId[queueIndex]     = pResultHolder->requestId;

                if (pResultHolder->sequenceId > m_pendingMetabufferDoneQueue.latestSequenceId)
                {
                    m_pendingMetabufferDoneQueue.latestSequenceId = pResultHolder->sequenceId;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Notify msg descriptor is NULL, could not dispatch notify");
            }
        }

        // There was no metadata error, concatenate and send all metadata results together in FIFO order
        if ((NULL == pResultHolder->pMetadataError) && (TRUE == MetadataReadyToFly(pResultHolder->sequenceId)))
        {
            // The main metadata is always in slot [0] and [1].
            if ((NULL != pResultHolder->pMetaBuffer[0]) && (NULL != pResultHolder->pMetaBuffer[1]))
            {
                /// @todo (CAMX-271) - Handle more than one (>1) partial metadata in pipeline/HAL -
                ///                    When we handle metadata in pipeline, we need to decide how we
                ///                    want to break the slot metadata into multiple result metadata
                ///                    components, as per the contract in MaxPartialMetadataHAL
                ///                    (i.e. android.request.partialResultCount)

                m_pCaptureResult[currentResult].pInputMetadata  = pResultHolder->pMetaBuffer[0];
                m_pCaptureResult[currentResult].pOutputMetadata = pResultHolder->pMetaBuffer[1];

                pResultHolder->pendingMetadataCount--;

                m_pCaptureResult[currentResult].numPartialMetadata = m_numMetadataResults;
                m_pCaptureResult[currentResult].frameworkFrameNum = GetFrameworkFrameNumber(pResultHolder->sequenceId);

                pResultHolder->pMetaBuffer[0] = NULL;
                pResultHolder->pMetaBuffer[1] = NULL;
                metadataAvailable = TRUE;

                UINT32 queueIndex = pResultHolder->sequenceId % MaxQueueDepth;

                m_pendingMetabufferDoneQueue.pendingMask[queueIndex]  |= MetaBufferDoneMetaReady;
                m_pendingMetabufferDoneQueue.pipelineIndex[queueIndex] = pResultHolder->pipelineIndex;
                m_pendingMetabufferDoneQueue.requestId[queueIndex]     = pResultHolder->requestId;

                if (pResultHolder->sequenceId > m_pendingMetabufferDoneQueue.latestSequenceId)
                {
                    m_pendingMetabufferDoneQueue.latestSequenceId = pResultHolder->sequenceId;
                }

                CAMX_LOG_INFO(CamxLogGroupHAL,
                              "Finalized metadata result for Sequence ID %d mapped to framework id %d pipeline %d reqId %u",
                              pResultHolder->sequenceId,
                              m_pCaptureResult[currentResult].frameworkFrameNum,
                              pResultHolder->pipelineIndex,
                              pResultHolder->requestId);
            }
        }
    }
    return metadataAvailable;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::ProcessResultBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::ProcessResultBuffers(
    ResultHolder* pResultHolder,
    BOOL          metadataAvailable,
    UINT*         pNumResults)
{
    BOOL hasResult        = FALSE;
    UINT currentResult    = *pNumResults;
    BOOL metadataReady    = (TRUE == metadataAvailable) || (0 == pResultHolder->pendingMetadataCount);

    // Dispatch buffers with both OK and Error status
    UINT numBuffersOut = 0;
    for (UINT32 bufIndex = 0; bufIndex < MaxNumOutputBuffers; bufIndex++)
    {
        ResultHolder::BufferResult* pBufferResult = &pResultHolder->bufferHolder[bufIndex];

        if (NULL != pBufferResult->pBuffer)
        {
            const BOOL bufferReady = (TRUE == pBufferResult->valid) && (TRUE == metadataReady);
            const BOOL bufferError = (NULL != pBufferResult->pBufferError);
            const BOOL bufferSent  = (NULL == pBufferResult->pStream); // If stream is NULL, then we've returned this buffer

            ChiCaptureResult* pResult = &m_pCaptureResult[currentResult];

            /// @todo (CAMX-3119) remove IsTorchWidgetSession check below and handle this in generic way.
            if (TRUE == IsTorchWidgetSession())
            {
                // Invalidate the pStream for torch, as there is no actual buffer with torch.
                pBufferResult->pStream = NULL;
            }

            if ((TRUE == bufferReady) || (TRUE == bufferError))
            {
                if ((NULL  != pBufferResult->pStream) &&
                    (TRUE == BufferReadyToFly(pResultHolder->sequenceId, pBufferResult->pStream)))
                {
                    ChiStreamWrapper* pChiStreamWrapper = static_cast<ChiStreamWrapper*>(pBufferResult->pStream->pPrivateInfo);

                    /// @todo (CAMX-1797) Rethink this way of keeping track of which is the next expected frame number
                    ///                   for the stream
                    pChiStreamWrapper->MoveToNextExpectedResultFrame();

                    ChiStreamBuffer* pStreamBuffer =
                        // NOWHINE CP036a: Google API requires const type
                        const_cast<ChiStreamBuffer*>(&pResult->pOutputBuffers[pResult->numOutputBuffers]);

                    Utils::Memcpy(pStreamBuffer, pBufferResult->pBuffer, sizeof(ChiStreamBuffer));

                    pResult->numOutputBuffers++;
                    hasResult = TRUE;

                    // Need to use this local since buffers may not all come back right away
                    numBuffersOut++;

                    if (TRUE == bufferError)
                    {
                        // Got a buffer in an error state so dispatch error

                        pBufferResult->error = TRUE;
                        pBufferResult->valid = FALSE;
                        pBufferResult->pBufferError->pPrivData = pResultHolder->pPrivData;
                        pBufferResult->pBuffer->bufferStatus   = BufferStatusError;

                        CAMX_LOG_INFO(CamxLogGroupCore, "Result buffer for sequence ID %u framenumber %u in error state.\n"
                                     "Result buffer %p\n"
                                     "     error:   %d\n"
                                     "     valid:   %d\n"
                                     "     buffer: %p\n"
                                     "          pStream:%p\n"
                                     "     Message: %p\n"
                                     "          messageType: %d\n"
                                     "          messageUnion: %p\n"
                                     "               errorMessageCode: %d\n"
                                     "               frameworkFrameNum: %u\n"
                                     "               pErrorStream: %p",
                                     pResultHolder->sequenceId,
                                     GetFrameworkFrameNumber(pResultHolder->sequenceId), pBufferResult,
                                     pBufferResult->error, pBufferResult->valid, pBufferResult->pBuffer,
                                     pBufferResult->pBuffer->pStream, pBufferResult->pBufferError,
                                     pBufferResult->pBufferError->messageType,
                                     pBufferResult->pBufferError->message,
                                     pBufferResult->pBufferError->message.errorMessage.errorMessageCode,
                                     pBufferResult->pBufferError->message.errorMessage.frameworkFrameNum,
                                     pBufferResult->pBufferError->message.errorMessage.pErrorStream);

                        CAMX_ASSERT(pBufferResult->pBufferError->message.errorMessage.errorMessageCode ==
                            ChiErrorMessageCode::MessageCodeBuffer);

                        DispatchNotify(pBufferResult->pBufferError);
                    }

                    // Invalidate the stream, will help in determining FIFO order for next result
                    pBufferResult->pStream = NULL;
                    pBufferResult->valid   = FALSE;
                }
            }
            else if (TRUE == bufferSent)
            {
                numBuffersOut++; // Account for buffers already sent
            }

            if (numBuffersOut == pResultHolder->numOutBuffers)
            {
                // We got the number of output buffers that we were expecting
                // For the input buffer, we should release input buffer fence
                // once we have all reporocess outputs buffers.

                // While CamX <=> CHI allows for more than 1 Input Buffer per request,
                // The   CHI  <=> HAL <=> App Framework layers do not.
                // The input buffer holder only can only properly represent Camera3 Input Streams
                // The maximum number of Camera3 input buffers supported is 1
                CAMX_STATIC_ASSERT(MaxNumInputBuffers == 1);
                CAMX_STATIC_ASSERT(CAMX_ARRAY_SIZE(pResultHolder->inputbufferHolder) == 1);

                if ((0 != pResultHolder->numInBuffers) && (NULL != pResultHolder->inputbufferHolder[0].pBuffer))
                {
                    pResult->pInputBuffer = pResultHolder->inputbufferHolder[0].pBuffer;

                    // NOWHINE CP036a: Google API requires const type
                    ChiStreamBuffer* pStreamInputBuffer = const_cast<ChiStreamBuffer*>(pResult->pInputBuffer);

                    // Driver no longer owns this and app will take ownership
                    pStreamInputBuffer->releaseFence.valid = FALSE;
                }
                break;
            }
        }
    }

    return hasResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::ProcessResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::ProcessResults()
{
    CamxResult              result                  = CamxResultSuccess;
    UINT32                  i                       = 0;
    UINT32                  numResults              = 0;
    ResultHolder*           pResultHolder           = NULL;
    SessionResultHolder*    pSessionResultHolder    = NULL;

    if (!GetFlushStatus())
    {
        result = m_pResultLock->TryLock();
        if (CamxResultSuccess != result)
        {
            // This could happen if InjectResult is called around the same time as the request/result processing thread
            // is running.  If it happens, skip result processing and try again
            CAMX_LOG_PERF_WARN(CamxLogGroupCore, "m_pResultLock failed, schedule this thread for reprocessing");
            CamxAtomicStore32(&m_aCheckResults, TRUE);

            // If it's a final result of the last request then no more PostJob is called and CHI/framework waits
            // indefinitely for the result
            VOID* pData[] = { this, NULL };
            result        = m_pThreadManager->PostJob(m_hJobFamilyHandle, NULL, &pData[0], FALSE, FALSE);
            return CamxResultSuccess;
        }
    }
    // Waiting in Flush status to avoid skip the last frame
    else
    {
        m_pResultLock->Lock();
    }

    // Reset the essential fields of framework results, so that not to be taken in stale
    for (UINT32 index = 0; index < (m_requestQueueDepth * m_numPipelines); index++)
    {
        m_pCaptureResult[index].frameworkFrameNum  = 0;
        m_pCaptureResult[index].numOutputBuffers   = 0;
        m_pCaptureResult[index].numPartialMetadata = 0;
        m_pCaptureResult[index].pInputMetadata     = NULL;
        m_pCaptureResult[index].pOutputMetadata    = NULL;
        m_pCaptureResult[index].pInputBuffer       = NULL;
        m_pCaptureResult[index].pResultMetadata    = NULL;
    }

    // m_pResultHolderListLock should only locks the code that updates the result holder and
    // it should be in the inner lock if it is used with other lock such as m_pResultLock, m_pFlushLock.
    m_pResultHolderListLock->Lock();

    LightweightDoublyLinkedListNode* pNode = m_resultHolderList.Head();

    while (NULL != pNode)
    {
        BOOL earlyMetadataReady    = FALSE;
        BOOL metadataReady         = FALSE;
        BOOL bufferReady           = FALSE;

        CAMX_ASSERT(NULL != pNode->pData);

        if (NULL != pNode->pData)
        {
            pSessionResultHolder = reinterpret_cast<SessionResultHolder*>(pNode->pData);

            for (UINT32 index = 0; index < pSessionResultHolder->numResults; index++)
            {
                pResultHolder = &(pSessionResultHolder->resultHolders[index]);
                if (NULL != pResultHolder)
                {
                    if (FALSE == pResultHolder->isCancelled)
                    {
                        // Only do partial metadata processing when the setting has defined the number of results greater than 1
                        if (1 < m_numMetadataResults)
                        {
                            earlyMetadataReady = ProcessResultEarlyMetadata(pResultHolder, &numResults);
                        }

                        // If we ever have early metadata for a given result before anything else. Stop processing the rest
                        // and just make sure we send back the early metadata.
                        if ((FALSE == earlyMetadataReady))
                        {
                            metadataReady = ProcessResultMetadata(pResultHolder, &numResults);
                            bufferReady   = ProcessResultBuffers(pResultHolder, metadataReady, &numResults);
                        }
                    }
                    else
                    {
                        // process results without regards to metadata if the request was cancelled
                        bufferReady = ProcessResultBuffers(pResultHolder, metadataReady, &numResults);
                    }

                    UINT totalBuffersSent =
                        (NULL != pResultHolder) ? pResultHolder->numErrorBuffersSent + pResultHolder->numOkBuffersSent : 0;

                    if ((TRUE == bufferReady) &&
                        (TRUE == m_isRealTime) &&
                        (pResultHolder->numOutBuffers == totalBuffersSent))
                    {
                        UINT32 expTime = CamxAtomicDecU32(&m_aTotalLongExposureTimeout, pResultHolder->expectedExposureTime);
                        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                            "Session %p - exposureTimeout after processing request with sequenceID %llu = %u",
                            this, pResultHolder->sequenceId, expTime);
                    }

                    UINT32 sequenceId  = pResultHolder->sequenceId;
                    UINT32 fwkFrameNum = GetFrameworkFrameNumber(pResultHolder->sequenceId);
                    BOOL   isCancelled = pResultHolder->isCancelled;
                    BINARY_LOG(LogEvent::Session_ProcessResult, sequenceId, fwkFrameNum, earlyMetadataReady, metadataReady,
                        bufferReady, isCancelled, this);
                    CAMX_LOG_INFO(CamxLogGroupCore,
                        "Processing Result - SequenceId %u Framenumber %u - earlyMetadata %d metadataReady %d "
                        "bufferReady %d isCancelled %d",
                        pResultHolder->sequenceId, GetFrameworkFrameNumber(pResultHolder->sequenceId), earlyMetadataReady,
                        metadataReady, bufferReady, pResultHolder->isCancelled);

                    if ((TRUE == metadataReady) || (TRUE == bufferReady) || (TRUE == earlyMetadataReady) ||
                        (TRUE == pResultHolder->isCancelled))
                    {
                        CAMX_LOG_CONFIG(CamxLogGroupCore,
                        "Processing Result - SequenceId %u Framenumber %llu - earlyMetadata %d metadataReady %d "
                        "bufferReady %d isCancelled %d",
                        pResultHolder->sequenceId, GetFrameworkFrameNumber(pResultHolder->sequenceId), earlyMetadataReady,
                        metadataReady, bufferReady, pResultHolder->isCancelled);
                        m_pCaptureResult[numResults].pPrivData         = pResultHolder->pPrivData;
                        m_pCaptureResult[numResults].frameworkFrameNum = GetFrameworkFrameNumber(pResultHolder->sequenceId);

                        numResults++;
                        bufferReady      = FALSE;
                        metadataReady    = FALSE;
                    }


                    if ((NULL != pResultHolder) &&
                        (pResultHolder->numOutBuffers == totalBuffersSent) && (pResultHolder->numErrorBuffersSent > 0) &&
                        (FALSE == pResultHolder->isCancelled) && (pResultHolder->pendingMetadataCount > 0))
                    {
                        CAMX_LOG_INFO(CamxLogGroupCore,
                            "RequestId: %llu SequenceId: %llu Framenumber: %llu - All results were injected with buffer error,"
                            " but an error notify has not been dispatched, dispatching now.",
                            pResultHolder->requestId, pResultHolder->sequenceId,
                            GetFrameworkFrameNumber(pResultHolder->sequenceId));

                        ChiMessageDescriptor* pNotify = GetNotifyMessageDescriptor();

                        pNotify->messageType                            = ChiMessageTypeError;
                        pNotify->pPrivData                              = pResultHolder->pPrivData;
                        pNotify->message.errorMessage.errorMessageCode  = static_cast<ChiErrorMessageCode>(MessageCodeResult);
                        pNotify->message.errorMessage.frameworkFrameNum = GetFrameworkFrameNumber(pResultHolder->sequenceId);
                        pNotify->message.errorMessage.pErrorStream      = NULL; // No stream applicable

                        DispatchNotify(pNotify);
                        pResultHolder->isCancelled          = TRUE;
                        pResultHolder->pendingMetadataCount = 0;

                        UINT32 queueIndex = pResultHolder->sequenceId % MaxQueueDepth;

                        m_pendingMetabufferDoneQueue.pendingMask[queueIndex]   =
                            (MetaBufferDoneMetaReady | MetaBufferDoneBufferReady);
                        m_pendingMetabufferDoneQueue.pipelineIndex[queueIndex] = pResultHolder->pipelineIndex;
                        m_pendingMetabufferDoneQueue.requestId[queueIndex]     = pResultHolder->requestId;

                        if (pResultHolder->sequenceId > m_pendingMetabufferDoneQueue.latestSequenceId)
                        {
                            m_pendingMetabufferDoneQueue.latestSequenceId = pResultHolder->sequenceId;
                        }
                    }

                    // If the request is complete, update the buffer to set processing end time
                    if ((NULL != pResultHolder) && (pResultHolder->numOutBuffers == totalBuffersSent) &&
                        (FALSE == pResultHolder->isCancelled) && (pResultHolder->pendingMetadataCount == 0))
                    {
                        UpdateSessionRequestTimingBuffer(pResultHolder);
                    }
                }
            }
        }

        // Get the next result holder and see what's going on with it
        pNode = m_resultHolderList.NextNode(pNode);
    }

    m_pResultHolderListLock->Unlock();

    if (0 < numResults)
    {
        // Finally dispatch all the results to the Framework
        DispatchResults(&m_pCaptureResult[0], numResults);
    }

    if ((TRUE == IsPipelineFlush()) &&
        (TRUE == m_waitForPipelineFlushResults))
    {
        PollPipelineFlushStatus();
    }

    // Error results can result in the minimum being advanced without dispatching results
    AdvanceMinExpectedResult();

    // process metabuffers
    ProcessMetabufferPendingQueue();

    m_pResultLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::ProcessRequest()
{
    CamxResult              result          = CamxResultSuccess;
    SessionCaptureRequest*  pSessionRequest = NULL;

    // This should only ever be called from threadpool, should never be reentrant, and nothing else  grabs the request lock.
    // If there is contention on this lock something very bad happened.
    result = m_pRequestLock->TryLock();
    if (CamxResultSuccess != result)
    {
        // Should never happen...return control back to the threadpool and this will eventually get called again
        CAMX_LOG_ERROR(CamxLogGroupCore, "Could not grab m_pRequestLock...undefined behavior possible");

        return CamxResultETryAgain;
    }

    // Initialize a result holder expected for the result coming out of this request
    // This information will be used in the result notification path

    pSessionRequest = static_cast<SessionCaptureRequest*>(m_pRequestQueue->Dequeue());

    if (NULL != pSessionRequest)
    {
        // If session request contain multiple pipeline request, it means pipelines need to be sync
        // and the batch frame number must be same.
        UINT32 numBatchedFrames = pSessionRequest->requests[0].GetBatchedHALOutputNum(&pSessionRequest->requests[0]);
        for (UINT requestIndex = 1; requestIndex < pSessionRequest->numRequests; requestIndex++)
        {
            if (numBatchedFrames !=
                pSessionRequest->requests[requestIndex].GetBatchedHALOutputNum(&pSessionRequest->requests[requestIndex]))
            {
                CAMX_LOG_ERROR(CamxLogGroupCore,
                    "batch frame number are different in different pipline request");
                m_pRequestLock->Unlock();
                return CamxResultEInvalidArg;
            }
        }

        const SettingsManager* pSettingManager = HwEnvironment::GetInstance()->GetSettingsManager();

        if (TRUE == pSettingManager->GetStaticSettings()->dynamicPropertiesEnabled)
        {
            // NOWHINE CP036a: We're actually poking into updating the settings dynamically so we do want to do this
            const_cast<SettingsManager*>(pSettingManager)->UpdateOverrideProperties();
        }

        LightweightDoublyLinkedListNode** ppResultNodes         = NULL;
        SessionResultHolder**             ppSessionResultHolder = NULL;

        for (UINT requestIndex = 0; requestIndex < pSessionRequest->numRequests; requestIndex++)
        {
            CaptureRequest& rRequest = pSessionRequest->requests[requestIndex];
            CAMX_ASSERT(rRequest.numBatchedFrames > 0);

            if (NULL == ppResultNodes)
            {
                ppResultNodes = reinterpret_cast<LightweightDoublyLinkedListNode**>(
                    CAMX_CALLOC(numBatchedFrames * sizeof(LightweightDoublyLinkedListNode*)));

                if (NULL == ppResultNodes)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "memory allocation failed for ppResultNodes for request %llu",
                        rRequest.requestId);
                    result = CamxResultENoMemory;
                    break;
                }
            }

            if (NULL == ppSessionResultHolder)
            {
                ppSessionResultHolder = reinterpret_cast<SessionResultHolder**>(
                    CAMX_CALLOC(numBatchedFrames * sizeof(SessionResultHolder*)));
                if (NULL == ppSessionResultHolder)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "memory allocation failed for ppSessionResultHolder for request %llu",
                        rRequest.requestId);
                    result = CamxResultENoMemory;
                    break;
                }
            }

            if ((NULL != ppResultNodes) && (NULL != ppSessionResultHolder))
            {
                // Add sequence id to framework frame number mapping after CheckRequestProcessingRate() = TRUE
                // This is to make sure new process request do not override old result has not sent to framework yet.
                for (UINT32 batchIndex = 0; batchIndex < rRequest.GetBatchedHALOutputNum(&rRequest); batchIndex++)
                {
                    Pipeline*         pPipeline      = m_pipelineData[rRequest.pipelineIndex].pPipeline;
                    StreamBufferInfo& rStreamBuffer  = rRequest.pStreamBuffers[batchIndex];
                    UINT64            chiFrameNumber = rStreamBuffer.originalFrameworkNumber;
                    UINT64            requestId      = rRequest.requestId;
                    UINT32            sequenceId     = rStreamBuffer.sequenceId;
                    UINT64            CSLSyncID      = rRequest.CSLSyncID;
                    auto              hPipeline      = pPipeline->GetPipelineDescriptor();
                    m_fwFrameNumberMap[sequenceId % MaxQueueDepth] = chiFrameNumber;

                    CAMX_LOG_REQMAP(CamxLogGroupCore,
                        "chiFrameNum: %llu  <==>  requestId: %llu  <==>  sequenceId: %u  <==> CSLSyncId: %llu -- %s",
                        chiFrameNumber, requestId, sequenceId, CSLSyncID,
                        pPipeline->GetPipelineIdentifierString());

                    BINARY_LOG(LogEvent::ReqMap_CamXInfo, chiFrameNumber, requestId, sequenceId,
                        CSLSyncID, hPipeline, this);
                }

                for (UINT batchIndex = 0; batchIndex < rRequest.GetBatchedHALOutputNum(&rRequest); batchIndex++)
                {
                    UINT32 sequenceId = rRequest.pStreamBuffers[batchIndex].sequenceId;

                    CAMX_TRACE_MESSAGE_F(CamxLogGroupCore, "ProcessRequest: RequestId: %llu sequenceId: %u",
                        rRequest.requestId, sequenceId);

                    LightweightDoublyLinkedListNode* pNode = ppResultNodes[batchIndex];
                    if (NULL == pNode)
                    {
                        pNode = reinterpret_cast<LightweightDoublyLinkedListNode*>
                            (CAMX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));
                        ppResultNodes[batchIndex] = pNode;
                    }

                    SessionResultHolder* pSessionResultHolder = ppSessionResultHolder[batchIndex];
                    if (NULL == pSessionResultHolder)
                    {
                        pSessionResultHolder = reinterpret_cast<SessionResultHolder*>
                            (CAMX_CALLOC(sizeof(SessionResultHolder)));
                        ppSessionResultHolder[batchIndex] = pSessionResultHolder;
                    }

                    if ((NULL == pNode) ||
                        (NULL == pSessionResultHolder))
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory pNode=%p pSessionResultHolder=%p",
                            pNode, pSessionResultHolder);
                        result = CamxResultENoMemory;

                        if (NULL != pNode)
                        {
                            CAMX_FREE(pNode);
                            pNode = NULL;
                        }

                        if (NULL != pSessionResultHolder)
                        {
                            CAMX_FREE(pSessionResultHolder);
                            pSessionResultHolder = NULL;
                        }
                    }

                    if (CamxResultSuccess == result)
                    {
                        ResultHolder* pHolder = &(pSessionResultHolder->resultHolders[requestIndex]);
                        Utils::Memset(pHolder, 0x0, sizeof(ResultHolder));
                        pHolder->sequenceId           = sequenceId;
                        pHolder->numOutBuffers        = rRequest.pStreamBuffers[batchIndex].numOutputBuffers;
                        pHolder->numInBuffers         = rRequest.pStreamBuffers[batchIndex].numInputBuffers;
                        pHolder->pendingMetadataCount = m_numMetadataResults;
                        pHolder->pPrivData            = rRequest.pPrivData;
                        pHolder->requestId            = static_cast<UINT32>(rRequest.requestId);
                        pHolder->expectedExposureTime = static_cast<UINT32>(rRequest.expectedExposureTime);

                        // We may not get a result metadata for reprocess requests
                        // This logic may need to be expanded for multi-camera CHI override scenarios,
                        // as to designate what pipelines are exactly offline
                        if (rRequest.pipelineIndex > 0)
                        {
                            pHolder->tentativeMetadata = TRUE;
                        }

                        for (UINT32 buffer = 0; buffer < pHolder->numOutBuffers; buffer++)
                        {
                            UINT32 streamIndex = GetStreamIndex(reinterpret_cast<ChiStream*>(
                                rRequest.pStreamBuffers[batchIndex].outputBuffers[buffer].pStream));

                            if (streamIndex < MaxNumOutputBuffers)
                            {
                                pHolder->bufferHolder[streamIndex].pBuffer = GetResultStreamBuffer();

                                Utils::Memcpy(pHolder->bufferHolder[streamIndex].pBuffer,
                                              &(rRequest.pStreamBuffers[batchIndex].outputBuffers[buffer]),
                                              sizeof(ChiStreamBuffer));

                                pHolder->bufferHolder[streamIndex].valid = FALSE;

                                pHolder->bufferHolder[streamIndex].pStream = reinterpret_cast<ChiStream*>(
                                    rRequest.pStreamBuffers[batchIndex].outputBuffers[buffer].pStream);

                                ChiStreamWrapper* pChiStreamWrapper = static_cast<ChiStreamWrapper*>(
                                    rRequest.pStreamBuffers[batchIndex].outputBuffers[buffer].pStream->pPrivateInfo);

                                pChiStreamWrapper->AddEnabledInFrame(rRequest.pStreamBuffers[batchIndex].sequenceId);
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCore, "stream index = %d < MaxNumOutputBuffers = %d",
                                               streamIndex, MaxNumOutputBuffers);
                            }
                        }

                        // Create internal private input buffer fences and relase them (below), so that
                        // input fence trigger mechanism would work same way that when the input fences
                        // released from previous/parent node output buffer

                        for (UINT32 buffer = 0; buffer < pHolder->numInBuffers; buffer++)
                        {
                            StreamInputBufferInfo* pInputBufferInfo   = rRequest.pStreamBuffers[batchIndex].inputBufferInfo;
                            ChiStreamBuffer*       pInputBuffer       = &(pInputBufferInfo[buffer].inputBuffer);
                            CSLFence*              phCSLFence         = NULL;
                            CHIFENCEHANDLE*        phAcquireFence     = NULL;
                            UINT32                 streamIndex        = 0;
                            ChiStream*             pInputBufferStream = reinterpret_cast<ChiStream*>(pInputBuffer->pStream);

                            /// @todo (CAMX-1797) Kernel currently requires us to pass a fence always even if we dont need it.
                            ///                   Fix that and also need to handle input fence mechanism
                            phCSLFence = &(pInputBufferInfo[buffer].fence);

                            if ((TRUE == pInputBuffer->acquireFence.valid) &&
                                (ChiFenceTypeInternal == pInputBuffer->acquireFence.type))
                            {
                                phAcquireFence = &(pInputBuffer->acquireFence.hChiFence);
                            }

                            if (FALSE == IsValidCHIFence(phAcquireFence))
                            {
                                result = CSLCreatePrivateFence("InputBufferFence_session", phCSLFence);
                                CAMX_ASSERT(CamxResultSuccess == result);

                                if (CamxResultSuccess != result)
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupCore, "process request failed : result %d", result);
                                    break;
                                }
                                else
                                {
                                    CAMX_LOG_VERBOSE(CamxLogGroupCore, "CSLCreatePrivateFence:%d Used", *phCSLFence);
                                }
                                pInputBufferInfo[buffer].isChiFence = FALSE;
                            }
                            else
                            {
                                *phCSLFence = reinterpret_cast<ChiFence*>(*phAcquireFence)->hFence;
                                pInputBufferInfo[buffer].isChiFence = TRUE;
                                CAMX_LOG_VERBOSE(CamxLogGroupCore, "AcquireFence:%d Used", *phCSLFence);
                            }

                            // While CamX <=> CHI allows for more than 1 Input Buffer per request,
                            // The   CHI  <=> HAL <=> App Framework layers do not.
                            // The input buffer holder only can only properly represent Camera3 Input Streams
                            // The maximum number of Camera3 input buffers supported is 1
                            CAMX_STATIC_ASSERT(MaxNumInputBuffers == 1);
                            CAMX_STATIC_ASSERT(CAMX_ARRAY_SIZE(pHolder->inputbufferHolder) == 1);
                            streamIndex = 0;
                            if (streamIndex < MaxNumInputBuffers)
                            {
                                ChiStreamBuffer* pChiResultStreamBuffer = GetResultStreamBuffer();

                                Utils::Memcpy(pChiResultStreamBuffer, pInputBuffer, sizeof(ChiStreamBuffer));
                                pHolder->inputbufferHolder[streamIndex].pBuffer = pChiResultStreamBuffer;
                                pHolder->inputbufferHolder[streamIndex].pStream = pInputBufferStream;
                            }
                        }
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }

        if ((NULL != ppResultNodes) && (NULL != ppSessionResultHolder))
        {
            // Lets start accounting for this request's exposure time
            UINT32 totalResultExposureTime = 0;

            // Now add the result holder to the linked list
            for (UINT batchIndex = 0; batchIndex < numBatchedFrames; batchIndex++)
            {
                LightweightDoublyLinkedListNode* pNode                  = ppResultNodes[batchIndex];
                SessionResultHolder*             pSessionResultHolder   = ppSessionResultHolder[batchIndex];
                pSessionResultHolder->numResults = pSessionRequest->numRequests;
                pNode->pData = pSessionResultHolder;
                m_pResultHolderListLock->Lock();
                m_resultHolderList.InsertToTail(pNode);
                m_pResultHolderListLock->Unlock();

                for (UINT idx = 0; idx < pSessionResultHolder->numResults; idx++)
                {
                    totalResultExposureTime += pSessionResultHolder->resultHolders[idx].expectedExposureTime;
                }
            }

            UINT32 totalExposureTime = CamxAtomicIncU32(&m_aTotalLongExposureTimeout, totalResultExposureTime);
            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                "Session %p - exposureTimeout after accounting for %u requests starting with requestID %llu = %u",
                this,
                pSessionRequest->numRequests,
                pSessionRequest->requests->requestId,
                totalExposureTime);
        }

        // De-acllocate the array of ppResultNodes and ppSessionResultHolder.
        // The actual node and session result holder will be free in processResult
        if (NULL != ppResultNodes)
        {
            CAMX_FREE(ppResultNodes);
            ppResultNodes = NULL;
        }
        if (NULL != ppSessionResultHolder)
        {
            CAMX_FREE(ppSessionResultHolder);
            ppSessionResultHolder = NULL;
        }
    }
    m_pRequestLock->Unlock();

    if (NULL != pSessionRequest)
    {
        BOOL isSyncMode = TRUE;

        if ((pSessionRequest->numRequests <= 1) ||
            (CSLSyncLinkModeNoSync == m_linkSyncMode))
        {
            isSyncMode = FALSE;
        }

        for (UINT requestIndex = 0; requestIndex < pSessionRequest->numRequests; requestIndex++)
        {
            CaptureRequest& rRequest = pSessionRequest->requests[requestIndex];

            result = m_pipelineData[rRequest.pipelineIndex].pPipeline->OpenRequest(rRequest.requestId,
                rRequest.CSLSyncID, isSyncMode, rRequest.expectedExposureTime);

            CAMX_LOG_INFO(CamxLogGroupCore,
                "pipeline[%d] OpenRequest called for request id = %llu withCSLSyncID %llu",
                rRequest.pipelineIndex,
                rRequest.requestId,
                rRequest.CSLSyncID);

            if ((CamxResultSuccess != result) && (CamxResultECancelledRequest != result))
            {
                CAMX_LOG_ERROR(CamxLogGroupCore,
                    "pipeline[%d] OpenRequest failed for request id = %llu withCSLSyncID %llu result = %s",
                    rRequest.pipelineIndex,
                    rRequest.requestId,
                    rRequest.CSLSyncID,
                    Utils::CamxResultToString(result));
            }
            else if (CamxResultECancelledRequest == result)
            {
                CAMX_LOG_INFO(CamxLogGroupCore, "Session: %p is in Flush state, Canceling OpenRequest for pipeline[%d] "
                    "for request id = %llu", this, rRequest.pipelineIndex, rRequest.requestId);

                result = CamxResultSuccess;
            }
        }

        if (CamxResultSuccess == result)
        {
            for (UINT requestIndex = 0; requestIndex < pSessionRequest->numRequests; requestIndex++)
            {
                CaptureRequest&            rRequest                   = pSessionRequest->requests[requestIndex];
                PipelineProcessRequestData pipelineProcessRequestData = {};

                result = SetupRequestData(&rRequest, &pipelineProcessRequestData);

                // Set timestamp for start of request processing
                PopulateSessionRequestTimingBuffer(&rRequest);

                if (CamxResultSuccess == result)
                {
                    result = m_pipelineData[rRequest.pipelineIndex].pPipeline->ProcessRequest(&pipelineProcessRequestData);
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "pipeline[%u] ProcessRequest failed for request %llu - %s",
                                   rRequest.pipelineIndex,
                                   rRequest.requestId,
                                   Utils::CamxResultToString(result));
                }

                if (NULL != pipelineProcessRequestData.pPerBatchedFrameInfo)
                {
                    CAMX_FREE(pipelineProcessRequestData.pPerBatchedFrameInfo);
                    pipelineProcessRequestData.pPerBatchedFrameInfo = NULL;
                }
            }
        }
        m_pRequestQueue->Release(pSessionRequest);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::PopulateSessionRequestTimingBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::PopulateSessionRequestTimingBuffer(
    CaptureRequest* pRequest)
{
    const StaticSettings* pStaticSettings = m_pChiContext->GetStaticSettings();

    if (TRUE == pStaticSettings->dumpSessionProcessingInfo)
    {
        CamxTime                pTime;
        PerResultHolderInfo*    pRequestInfo = GetSessionRequestData(pRequest->requestId, FALSE);

        OsUtils::GetTime(&pTime);
        pRequestInfo->startTime = OsUtils::CamxTimeToMillis(&pTime);
        pRequestInfo->requestId = pRequest->requestId;
        pRequestInfo->CSLSyncId = pRequest->CSLSyncID;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::UpdateSessionRequestTimingBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::UpdateSessionRequestTimingBuffer(
    ResultHolder* pResultHolder)
{
    const StaticSettings* pStaticSettings = m_pChiContext->GetStaticSettings();

    if (TRUE == pStaticSettings->dumpSessionProcessingInfo)
    {
        CamxTime                pTime;
        PerResultHolderInfo*    pRequestInfo = GetSessionRequestData(pResultHolder->requestId, TRUE);

        OsUtils::GetTime(&pTime);
        pRequestInfo->endTime = OsUtils::CamxTimeToMillis(&pTime);

        // Dump if we have populated the entire buffer
        if (0 == (pResultHolder->requestId % MaxSessionPerRequestInfo))
        {
            DumpSessionRequestProcessingTime();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::DumpSessionRequestProcessingTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::DumpSessionRequestProcessingTime()
{
    UINT   latestSessionRequestIndex    = m_perRequestInfo.lastSessionRequestIndex;
    UINT32 count                        = MaxSessionPerRequestInfo;
    UINT32 minProcessingTime            = UINT32_MAX;
    UINT32 maxProcessingTime            = 0;
    UINT32 currProcessingTime           = 0;
    UINT32 averageProcessingTime        = 0;
    UINT32 divisor                      = MaxSessionPerRequestInfo;

    const StaticSettings* pStaticSettings = m_pChiContext->GetStaticSettings();

    if (TRUE == pStaticSettings->dumpSessionProcessingInfo)
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "+------------------------------------------------------------------+");
        CAMX_LOG_INFO(CamxLogGroupCore, "+       DUMPING SESSION:%p REQUEST PROCESSING TIMES      +", this);

        while (count)
        {
            count--;

            // Check if session request entry is valid
            if ((0 != m_perRequestInfo.requestInfo[latestSessionRequestIndex].requestId)    &&
                (0 != m_perRequestInfo.requestInfo[latestSessionRequestIndex].endTime)      &&
                (0 != m_perRequestInfo.requestInfo[latestSessionRequestIndex].startTime)    &&
                (m_perRequestInfo.requestInfo[latestSessionRequestIndex].endTime            >=
                 m_perRequestInfo.requestInfo[latestSessionRequestIndex].startTime))
            {
                currProcessingTime      = (m_perRequestInfo.requestInfo[latestSessionRequestIndex].endTime -
                                            m_perRequestInfo.requestInfo[latestSessionRequestIndex].startTime);
                averageProcessingTime   += currProcessingTime;
                maxProcessingTime       = Utils::MaxUINT32(maxProcessingTime, currProcessingTime);
                minProcessingTime       = Utils::MinUINT32(minProcessingTime, currProcessingTime);

                CAMX_LOG_INFO(CamxLogGroupCore, "+   ReqId[%llu], SyncId[%llu], Start[%d], End[%d], Total[%d ms]",
                              m_perRequestInfo.requestInfo[latestSessionRequestIndex].requestId,
                              m_perRequestInfo.requestInfo[latestSessionRequestIndex].CSLSyncId,
                              m_perRequestInfo.requestInfo[latestSessionRequestIndex].startTime,
                              m_perRequestInfo.requestInfo[latestSessionRequestIndex].endTime,
                              currProcessingTime);
            }
            else
            {
                // If data wasn't valid, adjust how we calculate the average
                divisor--;
            }

            // Update the running index to reset correctly
            if (0 == latestSessionRequestIndex)
            {
                latestSessionRequestIndex = (MaxSessionPerRequestInfo - 1);
            }
            else
            {
                latestSessionRequestIndex--;
            }
        }

        averageProcessingTime = Utils::DivideAndCeil(averageProcessingTime, divisor);

        CAMX_LOG_INFO(CamxLogGroupCore, "+            AVG[%d ms],   MIN[%d ms],   MAX[%d ms]             +",
                      averageProcessingTime, minProcessingTime, maxProcessingTime);
        CAMX_LOG_INFO(CamxLogGroupCore, "+------------------------------------------------------------------+");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::HandleErrorCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::HandleErrorCb(
    CbPayloadError* pError,
    UINT            pipelineIndex,
    VOID*           pPrivData)
{
    // This should contain the number of output buffers and also the ChiMesssageDescriptor
    VOID*                 pPayloads[MaxPipelineOutputs+1] = {0};
    ChiMessageDescriptor* pNotify                         = GetNotifyMessageDescriptor();
    UINT                  streamId                        = 0;
    PipelineOutputData*   pPipelineOutputData             = NULL;
    ChiStreamBuffer*      pOutBuffer                      = NULL;

    CAMX_ASSERT(NULL != pNotify);
    CAMX_ASSERT(NULL != pError);

    pNotify->pPrivData                              = pPrivData;
    pNotify->messageType                            = ChiMessageTypeError;
    pNotify->message.errorMessage.frameworkFrameNum = GetFrameworkFrameNumber(pError->sequenceId);
    pNotify->message.errorMessage.errorMessageCode  = static_cast<ChiErrorMessageCode>(pError->code);

    switch (pError->code)
    {
        case MessageCodeDevice:
            /// @todo (CAMX-3266) Finalize error fence callbacks for on the flight results, we would depend on flush
            ///                implementation under normal conditions.
            ///                Yet, a device in error state might not be responsive; decide how to handle
            CAMX_LOG_ERROR(CamxLogGroupCore, "Device is in error condition!");
            // Set error state (block processing of capture requests and results), block Callbacks including torch
            // notify that would cause a segfault if device is resetting, etc.
            SetDeviceInError(TRUE);
            HAL3Module::GetInstance()->SetDropCallbacks();

            // Signal wait condition; Device error will cause camera to shut down, if we are waiting on this condition
            // after the close, we will hit NULL pointer when signaling destroyed condition after timed wait
            CAMX_LOG_CONFIG(CamxLogGroupCore, "Signaling m_pWaitLivePendingRequests condition");
            m_pWaitLivePendingRequests->Signal();

            pNotify->message.errorMessage.pErrorStream = NULL; // No stream applicable

            // Dispatch it immediately
            DispatchNotify(pNotify);

            // Trigger if caller was blocked on ProcessCaptureRequest
            m_pRequestQueue->CancelWait();

            // We expect close to be called at this point
            break;

        case MessageCodeRequest:
            // Dispatch it immediately

            pNotify->message.errorMessage.pErrorStream = NULL; // No stream applicable
            DispatchNotify(pNotify);

            pPayloads[0] = pNotify;
            InjectResult(ResultType::RequestError, pPayloads, pError->sequenceId, NULL, pipelineIndex);
            break;

        case MessageCodeResult:
            pNotify->message.errorMessage.pErrorStream = NULL; // No stream applicable

            // Notification will be dispatched along with other results
            InjectResult(ResultType::MetadataError, pNotify, pError->sequenceId, pPrivData, pipelineIndex);
            break;

        case MessageCodeBuffer:
            pOutBuffer = GetResultStreamBuffer();

            CAMX_ASSERT(NULL != pOutBuffer);

            streamId            = pError->streamId;
            pPipelineOutputData = &m_pipelineData[pipelineIndex].pPipelineDescriptor->outputData[streamId];

            pOutBuffer->size                = sizeof(ChiStreamBuffer);
            ///< @todo (CAMX-4113) Decouple CHISTREAM and camera3_stream
            pOutBuffer->pStream             =
                reinterpret_cast<ChiStream*>(pPipelineOutputData->pOutputStreamWrapper->GetNativeStream());
            pOutBuffer->bufferInfo          = pError->bufferInfo;
            pOutBuffer->bufferStatus        = BufferStatusError;

            if ((TRUE == isVideoStream(pPipelineOutputData->pOutputStreamWrapper->GetGrallocUsage())) &&
                (TRUE == CamX::IsGrallocBuffer(&pError->bufferInfo)))
            {
                BufferHandle* phNativeHandle = CamX::GetBufferHandleFromBufferInfo(&(pError->bufferInfo));

                if (NULL != phNativeHandle)
                {
                    SetPerFrameVTTimestampMetadata(*phNativeHandle,
                        GetIntraPipelinePerFramePool(PoolType::PerFrameResult, pipelineIndex),
                        pError->sequenceId + 1); // request ID starts from 1
                }
            }

            pNotify->message.errorMessage.pErrorStream =
                reinterpret_cast<ChiStream*>(pPipelineOutputData->pOutputStreamWrapper->GetNativeStream());

            pPayloads[0] = pOutBuffer;
            pPayloads[1] = pNotify;
            // Notification will be dispatched along with other results
            InjectResult(ResultType::BufferError, pPayloads, pError->sequenceId, pPrivData, pipelineIndex);
            break;
        case MessageCodeSOFFreeze:
        case MessageCodeRecovery:
            // If we aren't flushing (aka draining) accept a new request
            if (FALSE == CamxAtomicLoadU8(&m_aFlushingPipeline))
            {
                if (TRUE == m_pChiContext->GetStaticSettings()->enableRecovery)
                {
                    if (TRUE == m_pChiContext->GetStaticSettings()->raiserecoverysigabrt)
                    {
                        DumpSessionState(SessionDumpFlag::ResetRecovery);
                        CAMX_LOG_ERROR(CamxLogGroupCore, "FATAL ERROR: Raise SigAbort to debug"
                            " the root cause of UMD recovery");
                        OsUtils::RaiseSignalAbort();
                    }
                    else
                    {
                        if (FALSE == GetSessionTriggeringRecovery())
                        {
                            // Tell HAL queue we are recovering to handle enqueuing new requests properly during recovery
                            m_pRequestQueue->SetRecoveryStatus(TRUE);

                            // Set recovery status to block any PCRs from coming in
                            SetSessionTriggeringRecovery(TRUE);
                            NotifyPipelinesOfTriggeringRecovery(TRUE);

                            // Do not hold onto wait condition
                            CAMX_LOG_CONFIG(CamxLogGroupCore, "Signaling m_pWaitLivePendingRequests condition");
                            m_pWaitLivePendingRequests->Signal();

                            CAMX_LOG_CONFIG(CamxLogGroupCore, "Sending recovery for frameworkFrameNum=%d",
                                pNotify->message.errorMessage.frameworkFrameNum);

                            DumpSessionState(SessionDumpFlag::ResetRecovery);
                            DispatchNotify(pNotify);

                            // Trigger if caller was blocked on ProcessCaptureRequest
                            m_pRequestQueue->CancelWait();
                        }
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "UMD Recovery is disabled, cannot trigger");
                }
            }

            break;

        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::HandleAsyncCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::HandleAsyncCb(
    CbPayloadAsync* pAsync,
    VOID*           pPrivData)
{
    ChiMessageDescriptor* pNotify = GetNotifyMessageDescriptor();

    CAMX_ASSERT(NULL != pNotify);

    // We have to assume that any async callback has to be shutter message
    pNotify->messageType                              = ChiMessageTypeShutter;
    pNotify->message.shutterMessage.frameworkFrameNum = GetFrameworkFrameNumber(pAsync->sequenceId);
    pNotify->message.shutterMessage.timestamp         = pAsync->timestamp;
    pNotify->pPrivData                                = pPrivData;

    // Dispatch it immediately
    DispatchNotify(pNotify);

    m_pResultHolderListLock->Lock();
    // Mark the we sent out the shutter
    ResultHolder* pResHolder     = GetResultHolderBySequenceId(pAsync->sequenceId);
    if (NULL != pResHolder)
    {
        pResHolder->isShutterSentOut = TRUE;
    }
    m_pResultHolderListLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::HandleSOFCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::HandleSOFCb(
    CbPayloadSof* pSof)
{
    ChiMessageDescriptor* pNotify = GetNotifyMessageDescriptor();

    CAMX_ASSERT(NULL != pNotify);
    CAMX_ASSERT(NULL != pSof);

    // Send SOF event as a shutter message
    pNotify->messageType                                   = ChiMessageTypeSof;
    pNotify->message.sofMessage.timestamp                  = pSof->timestamp;
    pNotify->message.sofMessage.sofId                      = pSof->frameNum;
    pNotify->message.sofMessage.bIsFrameworkFrameNumValid  = pSof->bIsSequenceIdValid;
    if (pSof->bIsSequenceIdValid)
    {
        pNotify->message.sofMessage.frameworkFrameNum = GetFrameworkFrameNumber(pSof->sequenceId);
    }

    // Dispatch it immediately
    DispatchNotify(pNotify);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::HandleMetaBufferDoneCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::HandleMetaBufferDoneCb(
    CbPayloadMetaBufferDone* pMetaBufferCbPayload)
{
    ChiMessageDescriptor notify = { };

    if ((NULL != pMetaBufferCbPayload->pInputMetabuffer) ||
         (NULL != pMetaBufferCbPayload->pOutputMetabuffer))
    {
        // Send MetaBufferDone event
        notify.messageType                                     = ChiMessageTypeMetaBufferDone;
        notify.message.metaBufferDoneMessage.inputMetabuffer   = pMetaBufferCbPayload->pInputMetabuffer;
        notify.message.metaBufferDoneMessage.outputMetabuffer  = pMetaBufferCbPayload->pOutputMetabuffer;
        notify.message.metaBufferDoneMessage.frameworkFrameNum = GetFrameworkFrameNumber(pMetaBufferCbPayload->sequenceId);

        // Dispatch it immediately
        DispatchNotify(&notify);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot issue metabuffer done callback input %p output %p",
                      pMetaBufferCbPayload->pInputMetabuffer,
                      pMetaBufferCbPayload->pOutputMetabuffer);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::UpdateCurrExpTimeUseBySensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::UpdateCurrExpTimeUseBySensor(
    UINT64 currExposureTimeUseBySensor)
{
    m_currExposureTimeUseBySensor = static_cast<UINT32>(currExposureTimeUseBySensor / static_cast<UINT64>(1000000));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::HandleMetadataCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::HandleMetadataCb(
    CbPayloadMetadata* pPayload,
    VOID*              pPrivData,
    UINT32             pipelineIndex)
{
    // We will use the pMetadata->pMetadata all the way upto the framework
    InjectResult(ResultType::MetadataOK, &pPayload->metaPayload, pPayload->sequenceId, pPrivData, pipelineIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Session::HandlePartialMetadataCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::HandlePartialMetadataCb(
    CbPayloadPartialMetadata* pPayload,
    VOID*                     pPrivData,
    UINT32                    pipelineIndex)
{
    CAMX_UNREFERENCED_PARAM(pipelineIndex);

    ChiPartialCaptureResult* pPartialCaptureResult  = GetPartialMetadataMessageDescriptor();
    pPartialCaptureResult->frameworkFrameNum        = GetFrameworkFrameNumber(pPayload->sequenceId);
    pPartialCaptureResult->pPartialResultMetadata   = pPayload->partialMetaPayload.pMetaBuffer;
    pPartialCaptureResult->pPrivData                = pPrivData;
    // Dispatch it immediately
    DispatchPartialMetadata(pPartialCaptureResult);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::HandleEarlyMetadataCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::HandleEarlyMetadataCb(
    CbPayloadMetadata* pPayload,
    UINT               pipelineIndex,
    VOID*              pPrivData)
{
    // We will use the pMetadata->pMetadata all the way upto the framework
    InjectResult(ResultType::EarlyMetadataOK, &pPayload->metaPayload, pPayload->sequenceId, pPrivData, pipelineIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::HandleBufferCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::HandleBufferCb(
    CbPayloadBuffer* pPayload,
    UINT             pipelineIndex,
    VOID*            pPrivData)
{
    ChiStreamBuffer outBuffer = { 0 };

    UINT                streamId            = pPayload->streamId;
    PipelineOutputData* pPipelineOutputData = &m_pipelineData[pipelineIndex].pPipelineDescriptor->outputData[streamId];

    outBuffer.size                  = sizeof(ChiStreamBuffer);
    outBuffer.pStream               = reinterpret_cast<ChiStream*>(
                                      pPipelineOutputData->pOutputStreamWrapper->GetNativeStream());
    outBuffer.bufferInfo            = pPayload->bufferInfo;
    outBuffer.bufferStatus          = BufferStatusOK;
    outBuffer.releaseFence.valid    = FALSE; // For the moment

    if ((TRUE == isVideoStream(pPipelineOutputData->pOutputStreamWrapper->GetGrallocUsage())) &&
        (TRUE == CamX::IsGrallocBuffer(&pPayload->bufferInfo)))
    {
        BufferHandle* phNativeHandle = CamX::GetBufferHandleFromBufferInfo(&(pPayload->bufferInfo));

        if (NULL != phNativeHandle)
        {
            SetPerFrameVTTimestampMetadata(*phNativeHandle,
                GetIntraPipelinePerFramePool(PoolType::PerFrameResult, pipelineIndex),
                pPayload->sequenceId + 1); // request ID starts from 1

            SetPerFrameCVPMetadata(*phNativeHandle, pipelineIndex, pPayload->sequenceId);

            if ((FALSE == m_pChiContext->GetStaticSettings()->disableVideoPerfModeSetting) &&
               (TRUE == m_setVideoPerfModeFlag))
            {
                SetPerFrameVideoPerfModeMetadata(*phNativeHandle);
            }
        }
    }

    InjectResult(ResultType::BufferOK, &outBuffer, pPayload->sequenceId, pPrivData, pipelineIndex);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::InjectResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::InjectResult(
    ResultType  resultType,
    VOID*       pPayload,
    UINT32      sequenceId,
    VOID*       pPrivData,
    INT32       pipelineIndex)
{
    CamxResult result = CamxResultSuccess;

    m_pResultHolderListLock->Lock();

    ResultHolder* pHolder = GetResultHolderBySequenceId(sequenceId);

    if (NULL == pHolder)
    {
        CAMX_LOG_WARN(CamxLogGroupCore,
                       "Result holder NULL for seqId: %d, this request may be flushed out already.",
                       sequenceId);

        m_pResultHolderListLock->Unlock();

        return CamxResultSuccess;
    }

    CAMX_ASSERT(FALSE == GetDeviceInError());

    pHolder->pipelineIndex = pipelineIndex;

    // Bail out if device/request is in error
    if ((TRUE == GetDeviceInError()))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Cannot inject results");
        m_pResultHolderListLock->Unlock();

        return CamxResultEFailed;
    }

    if (ResultType::RequestError == resultType)
    {
        pHolder->isCancelled         = TRUE;

        // Update the buffer error to be the notify passed so ProcessResultsBuffer can know to clean up accordingly.
        for (UINT bufIndex = 0; bufIndex < MaxNumOutputBuffers; bufIndex++)
        {
            if (NULL != pHolder->bufferHolder[bufIndex].pStream)
            {
                pHolder->bufferHolder[bufIndex].pBufferError          = static_cast<ChiMessageDescriptor*>(pPayload);
            }
        }
        pHolder->pPrivData = pPrivData;
    }
    else if (ResultType::MetadataError == resultType)
    {
        pHolder->pMetadataError = static_cast<ChiMessageDescriptor*>(pPayload);
        pHolder->pPrivData      = pPrivData;
    }
    else if (ResultType::MetadataOK == resultType)
    {
        /// @todo (CAMX-271) - Handle more than one (>1) partial metadata in pipeline/HAL -
        ///                    When we handle metadata in pipeline, we need to decide how we
        ///                    want to break the slot metadata into multiple result metadata
        ///                    components, as per the contract in MaxPartialMetadataHAL
        ///                    (i.e. android.request.partialResultCount)
        CAMX_LOG_INFO(CamxLogGroupCore, "Inject result added metadata in result holder for sequence ID : %d", sequenceId);

        MetadataPayload* pMetaPayload = static_cast<MetadataPayload*>(pPayload);

        pHolder->pMetaBuffer[0] = pMetaPayload->pMetaBuffer[0];
        pHolder->pMetaBuffer[1] = pMetaPayload->pMetaBuffer[1];
        pHolder->requestId      = pMetaPayload->requestId;
        pHolder->pPrivData      = pPrivData;

        pHolder->metadataCbIndex++;
    }
    else if (ResultType::EarlyMetadataOK == resultType)
    {
        CAMX_LOG_INFO(CamxLogGroupHAL, "Inject result added early metadata in result holder for sequence ID : %d", sequenceId);

        MetadataPayload* pMetaPayload = static_cast<MetadataPayload*>(pPayload);

        // incase of partial metadata only one buffer is sent from the pipeline
        // partial metadata is kept in buffer index 2
        pHolder->pMetaBuffer[2]       = pMetaPayload->pMetaBuffer[0];
        pHolder->pPrivData            = pPrivData;

        pHolder->metadataCbIndex++;
    }
    else if (ResultType::BufferError == resultType)
    {
        VOID**           ppPayloads = static_cast<VOID**>(pPayload);
        ChiStreamBuffer* pBuffer    = static_cast<ChiStreamBuffer*>(ppPayloads[0]);
        ChiStream*       pStream    = pBuffer->pStream;

        UINT32 streamIndex = GetStreamIndex(pStream);

        if (FALSE == GetPipelineFlushStatus(pipelineIndex))
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                           "Reporting a buffer error to the framework for streamIndex %u SeqId: %d <-> ReqId: %d",
                           streamIndex,
                           sequenceId,
                           pHolder->requestId);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupCore,
               "Reporting a buffer error during flush to the framework for streamIndex %u SeqId: %d <-> ReqId: %d",
               streamIndex,
               sequenceId,
               pHolder->requestId);
        }

        if (MaxNumOutputBuffers != streamIndex)
        {
            pHolder->bufferHolder[streamIndex].error        = TRUE;
            pHolder->bufferHolder[streamIndex].pBuffer      = pBuffer;
            pHolder->bufferHolder[streamIndex].pBufferError = static_cast<ChiMessageDescriptor*>(ppPayloads[1]);
        }
        pHolder->pPrivData = pPrivData;
        pHolder->numErrorBuffersSent++;
    }
    else if (ResultType::BufferOK == resultType)
    {
        ChiStreamBuffer* pBuffer = static_cast<ChiStreamBuffer*>(pPayload);
        ChiStream*       pStream = pBuffer->pStream;

        UINT32 streamIndex = GetStreamIndex(pStream);

        if (MaxNumOutputBuffers != streamIndex)
        {
            ChiStreamBuffer* pHoldBuffer = pHolder->bufferHolder[streamIndex].pBuffer;

            CAMX_LOG_INFO(CamxLogGroupCore,
                          "Inject result added request %d output buffer in result holder:Stream %p Fmt: %d W: %d H: %d",
                          sequenceId,
                          pStream,
                          pStream->format,
                          pStream->width,
                          pStream->height);


            if (NULL != pHoldBuffer)
            {
                if ((pHoldBuffer->pStream             == pStream) &&
                        (pHoldBuffer->bufferInfo.phBuffer == pBuffer->bufferInfo.phBuffer))
                {
                    Utils::Memcpy(pHoldBuffer, pBuffer, sizeof(ChiStreamBuffer));
                    pHolder->bufferHolder[streamIndex].valid = TRUE;
                    pHolder->pPrivData                       = pPrivData;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore,
                            "Session %p: Sequence ID %d: Result bufferHolder[%d] pStream = %p, bufferType = %d phBuffer = %p"
                            " does not match buffer Cb pStream = %p, bufferType = %d phBuffer = %p",
                            this,
                            sequenceId,
                            streamIndex,
                            pHoldBuffer->pStream,
                            pHoldBuffer->bufferInfo.bufferType,
                            pHoldBuffer->bufferInfo.phBuffer,
                            pStream,
                            pBuffer->bufferInfo.bufferType,
                            pBuffer->bufferInfo.phBuffer);
                }
            }
        }

        pHolder->numOkBuffersSent++;
    }

    // Make the result holder slot live, it's ok to write it more than once
    pHolder->isAlive = TRUE;

    m_pResultHolderListLock->Unlock();

    if (TRUE == MeetFrameworkNotifyCriteria(pHolder))
    {
        // Worker thread needs to check results
        CamxAtomicStore32(&m_aCheckResults, TRUE);


        if ((m_pThreadManager->GetJobCount(m_hJobFamilyHandle) <= 1) || (TRUE == m_pRequestQueue->IsEmpty()))
        {
            // Check current HAL worker thread job count. If count is 0 or 1, it means we are in below two situations.
            // 1. Possible last request comes in and no job will posted by request anymore.
            // 2. Request is processed too fast and paused.
            // Both cases means there might not be any HALWorker jobs pending to consume the result.
            // And since we have a result that needs to be processed by the HALWorker, it needs to post a job now.

            VOID* pData[] = { this, NULL };
            result        = m_pThreadManager->PostJob(m_hJobFamilyHandle, NULL, &pData[0], FALSE, FALSE);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::ClearAllPendingItems
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::ClearAllPendingItems(
    ResultHolder* pHolder
    ) const
{
    // Request was in error, no result for this frame will be deemed valid but next result can proceed

    pHolder->pendingMetadataCount = 0;

    for (UINT32 i = 0; i < MaxNumOutputBuffers; i++)
    {
        pHolder->bufferHolder[i].pStream = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::MeetFrameworkNotifyCriteria
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::MeetFrameworkNotifyCriteria(
    ResultHolder* pHolder)
{
    CAMX_UNREFERENCED_PARAM(pHolder);

    BOOL metCriteria = FALSE;

    // To start with, we ensure that we have got at least as many metadata as HAL is expected to send in a result
    /// @todo (CAMX-1797) For offline pipelines this is not necessarily valid
    metCriteria = TRUE;

    // We leave this space open for optimization ---
    // We may want to wake the worker up only when we have a batch of results, OR
    // We have output buffers from specific streams in the result, OR
    // Move the burden of checking FIFO eligibility from ProcessResults to here

    return metCriteria;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::MetadataReadyToFly
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::MetadataReadyToFly(
    UINT32 sequenceId)
{
    // Nothing pending, good to go
    SessionResultHolder* pSessionResultHolder =
            reinterpret_cast<SessionResultHolder*>(m_resultHolderList.Head()->pData);
    CamxResult result = CamxResultEFailed;
    BOOL canDispatch = TRUE;
    for (UINT32 i = 0; i < pSessionResultHolder->numResults; i++)
    {
        ResultHolder* pHolder = &pSessionResultHolder->resultHolders[i];

        if (sequenceId == pHolder->sequenceId)
        {
            // Make sure the meta data is updated
            canDispatch = (pHolder->metadataCbIndex == 0) ? FALSE : TRUE;
            result = CamxResultSuccess;
            break;
        }
    }

    if (CamxResultSuccess != result)
    {
        m_pResultHolderListLock->Lock();
        ResultHolder* pLastFrameHolder = GetResultHolderBySequenceId(sequenceId - 1);

        // If we find a previous result AND its metadata count is the same as num partials, then we likely
        // still have processing to do. If pendingMetadataCount != numPartial, it means we've gotten some metadata backs
        if ((NULL != pLastFrameHolder) && (pLastFrameHolder->pendingMetadataCount == m_numMetadataResults))
        {
            canDispatch = FALSE;
        }
        m_pResultHolderListLock->Unlock();
    }

    return canDispatch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::BufferReadyToFly
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::BufferReadyToFly(
    UINT32      sequenceId,
    ChiStream*  pStream)
{
    BOOL isReadyToFly = FALSE;

    ChiStreamWrapper* pChiStreamWrapper = static_cast<ChiStreamWrapper*>(pStream->pPrivateInfo);

    if (TRUE == pChiStreamWrapper->IsNextExpectedResultFrame(sequenceId))
    {
        isReadyToFly = TRUE;
    }

    return isReadyToFly;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::CalculateResultFPS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::CalculateResultFPS()
{
    UINT64 currentTime = OsUtils::GetNanoSeconds();
    UINT64 elapsedTime;

    if (0 == m_lastFPSCountTime)
    {
        m_lastFPSCountTime  = currentTime;
        m_currentFrameCount = 0;
    }
    else
    {
        m_currentFrameCount++;
    }
    elapsedTime = currentTime - m_lastFPSCountTime;

    // Update FPS after 10 secs
    if (elapsedTime > (10 * NanoSecondsPerSecond))
    {
        FLOAT fps = (m_currentFrameCount * NanoSecondsPerSecond / static_cast<FLOAT> (elapsedTime));
        CAMX_LOG_PERF_INFO(CamxLogGroupCore, "FPS: %0.2f", fps);

        m_currentFrameCount = 0;
        m_lastFPSCountTime  = currentTime;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::AdvanceMinExpectedResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::AdvanceMinExpectedResult()
{
    /// @note This function can only be called from Session::ProcessRequest with m_pResultLock taken.  If it is called from
    ///       anywhere else, a deadlock is possible
    BOOL    canAdvance      = TRUE;
    BOOL    moveNext        = TRUE;
    UINT32  pipelineIndex   = InvalidPipelineIndex;

    m_pResultHolderListLock->Lock();

    LightweightDoublyLinkedListNode* pNode = m_resultHolderList.Head();

    if ((NULL == pNode) && (0 < m_resultHolderList.NumNodes()))
    {
        CAMX_LOG_WARN(CamxLogGroupCore,
                "Session %p: Warning: Result Holder HEAD is NULL with size %d",
                this, m_resultHolderList.NumNodes());
    }

    while (NULL != pNode)
    {
        SessionResultHolder* pSessionResultHolder = reinterpret_cast<SessionResultHolder*>(pNode->pData);

        CAMX_ASSERT(NULL != pSessionResultHolder);


        // If there are multiple pipeline result map to same framework number
        // e.g. In dual camera both wide and tele pipeline need to generate output for same framework request
        // all the pipeline result need to be sent before it can advance.
        for (UINT resultIndex = 0; resultIndex < pSessionResultHolder->numResults; resultIndex++)
        {
            ResultHolder* pHolder = &pSessionResultHolder->resultHolders[resultIndex];

            if (NULL != pHolder)
            {

                if ((TRUE == canAdvance) && (FALSE == pHolder->isCancelled))
                {
                    if (pHolder->pendingMetadataCount > 0)
                    {
                        CAMX_LOG_INFO(CamxLogGroupCore,
                            "Session %p: Can't advance because of pending metadata for sequence id: %d, Pipeline Index: %d",
                            this,
                            reinterpret_cast<ResultHolder*>(pNode->pData)->sequenceId, resultIndex);
                        canAdvance = FALSE;
                    }
                }

                // Make sure all of the pStreams have been consumed from the result bufferHolder, if any are NOT NULL it means
                // we haven't finished with the request and still have more results to return
                if (TRUE == canAdvance)
                {
                    for (UINT32 bufferIndex = 0; bufferIndex < MaxNumOutputBuffers; bufferIndex++)
                    {
                        if (NULL != pHolder->bufferHolder[bufferIndex].pStream)
                        {
                            CAMX_LOG_INFO(CamxLogGroupCore,
                                "Session %p: Can't advance because of buffer[%d] for sequence Id: %d, Pipeline Index: %d",
                                this,
                                bufferIndex,
                                reinterpret_cast<ResultHolder*>(pNode->pData)->sequenceId, resultIndex);
                            canAdvance      = FALSE;
                            break;
                        }
                    }
                }

                if (FALSE == canAdvance)
                {
                    break;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Session %p: pHolder is NULL while trying to advance min result", this);
            }
        }

        if (TRUE == canAdvance)
        {
            for (UINT resultIndex = 0; resultIndex < pSessionResultHolder->numResults; resultIndex++)
            {
                ResultHolder* pHolder = &pSessionResultHolder->resultHolders[resultIndex];
                // Making it this far for a given pHolder means it's ready to be popped off the
                // list and we can move forward.

                pipelineIndex = pHolder->pipelineIndex;

                // Now we can release the reference to resousce manager
                if (TRUE == UsingResourceManager(pipelineIndex))
                {
                    ResourceID resourceId = static_cast<ResourceID>(ResourceType::RealtimePipeline);

                    m_pResultHolderListLock->Unlock();
                    m_pChiContext->GetResourceManager()->ReleaseResourceReference(resourceId,
                        static_cast<VOID*>(m_pipelineData[pipelineIndex].pPipelineDescriptor), TRUE);
                    m_pResultHolderListLock->Lock();
                }

                m_pipelineData[pHolder->pipelineIndex].pPipeline->DecrementLivePendingRequest();

                // all buffers are done. update metabuffer done queue
                UINT32 index = pHolder->sequenceId % MaxQueueDepth;
                m_pendingMetabufferDoneQueue.pendingMask[index] |= MetaBufferDoneBufferReady;

                if (TRUE == m_pChiContext->GetStaticSettings()->enableFPSLog)
                {
                    CalculateResultFPS();
                }

                // If DRQ debugging is enabled dump it out.
                m_pDeferredRequestQueue->DumpDebugInfo(pHolder->requestId, FALSE);

                // Add some trace events
                CAMX_TRACE_ASYNC_END_F(CamxLogGroupCore, pHolder->sequenceId, "HAL3: RequestTrace");
                CAMX_LOG_INFO(CamxLogGroupCore,
                              "Session %p: Results processed for sequenceId: %d",
                              this, pHolder->sequenceId);
            }

            CAMX_FREE(pNode->pData);
            pNode->pData = NULL;

            // Since we've finished the requeset, remove the node from the list
            m_resultHolderList.RemoveNode(pNode);

            CAMX_FREE(pNode);
            pNode    = NULL;
            moveNext = FALSE;

            m_pResultHolderListLock->Unlock();

            // If we've gotten any buffer ready for this result then we can accept another
            m_pLivePendingRequestsLock->Lock();

            if (0 < m_livePendingRequests)
            {
                m_livePendingRequests--;

                m_pWaitLivePendingRequests->Signal();
            }

            m_pLivePendingRequestsLock->Unlock();

            NotifyProcessingDone();
            m_pResultHolderListLock->Lock();
        }
        else
        {
            break;
        }

        if (TRUE == moveNext)
        {
            pNode = m_resultHolderList.NextNode(pNode);
        }
        else
        {
            pNode = m_resultHolderList.Head();
        }
    }

    if ((0 != m_resultHolderList.NumNodes()) &&
        ((NULL != m_resultHolderList.Head()) &&
         (NULL != m_resultHolderList.Tail())))
    {
        SessionResultHolder* pSessionResultHolderHead =
            reinterpret_cast<SessionResultHolder*>(m_resultHolderList.Head()->pData);
        SessionResultHolder* pSessionResultHolderTail =
            reinterpret_cast<SessionResultHolder*>(m_resultHolderList.Tail()->pData);
        if ((NULL != pSessionResultHolderHead) && (NULL != pSessionResultHolderTail))
        {
            CAMX_LOG_INFO(CamxLogGroupCore,
                          "Session %p: Results processed, current queue state: minResultIdExpected = %u, "
                          "maxResultIdExpected= %u, minSequenceIdExpected: %u, maxSequenceIdExpected: %u, "
                          "and numResultsPending: %d",
                          this,
                          pSessionResultHolderHead->resultHolders[0].requestId,
                          pSessionResultHolderTail->resultHolders[pSessionResultHolderTail->numResults - 1].requestId,
                          pSessionResultHolderHead->resultHolders[0].sequenceId,
                          pSessionResultHolderTail->resultHolders[pSessionResultHolderTail->numResults - 1].sequenceId,
                          m_resultHolderList.NumNodes());
        }
        m_pResultHolderListLock->Unlock();
    }
    else
    {
        m_pResultHolderListLock->Unlock();

        CAMX_LOG_INFO(CamxLogGroupCore, "Session %p: All results processed, current queue state is empty.", this);

        // Now that we've emptied the queue, signal saying we're ready for a new request if they exist
        m_pWaitLivePendingRequests->Signal();

        // If flush and process_capture_request were racing, it's possible that we accepted the job
        // and added it to the HALQueue right as we were starting flush, in which case we've got a request
        // hanging out in the HALQueue that now needs to be processed, so this kicks the queue
        if (FALSE == m_pRequestQueue->IsEmpty())
        {
            VOID* pData[] = { this, NULL };
            m_pThreadManager->PostJob(m_hJobFamilyHandle, NULL, &pData[0], FALSE, FALSE);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::AllPipelinesFlushed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::AllPipelinesFlushed()
{
    UINT64 flushCount = 0;

    for (UINT index = 0; index < m_sessionFlushInfo.numPipelines; index++)
    {
        if (TRUE == m_pipelineFlushResultsAv[index])
        {
            flushCount += 1;
        }
    }

    return static_cast<BOOL>(m_sessionFlushInfo.numPipelines == flushCount);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::PollPipelineFlushStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::PollPipelineFlushStatus()
{
    /// @note This function can only be called from Session::ProcessRequest with m_pResultLock taken.  If it is called from
    ///       anywhere else, a deadlock is possible

    m_pResultHolderListLock->Lock();

    for (UINT index = 0; index < m_sessionFlushInfo.numPipelines; index++)
    {
        BOOL   canAdvance                      = TRUE;
        UINT32 flushPipelineIndex              = GetPipelineIndex(m_sessionFlushInfo.pPipelineFlushInfo[index].hPipelineHandle);
        LightweightDoublyLinkedListNode* pNode = m_resultHolderList.Head();

        if (InvalidPipelineIndex != flushPipelineIndex)
        {
            if ((NULL == pNode) && (0 < m_resultHolderList.NumNodes()))
            {
                CAMX_LOG_WARN(CamxLogGroupCore,
                    "Session %p: Warning: Result Holder HEAD is NULL with size %d",
                    this, m_resultHolderList.NumNodes());
            }

            while (NULL != pNode)
            {
                SessionResultHolder* pSessionResultHolder = reinterpret_cast<SessionResultHolder*>(pNode->pData);

                CAMX_ASSERT(NULL != pSessionResultHolder);

                // If there are multiple pipeline result map to same framework number
                // e.g. In dual camera both wide and tele pipeline need to generate output for same framework request
                // check if the pipeline we are trying to flush has returned "valid" results
                for (UINT resultIndex = 0; resultIndex < pSessionResultHolder->numResults; resultIndex++)
                {
                    ResultHolder* pHolder = &pSessionResultHolder->resultHolders[resultIndex];
                    if (NULL != pHolder && (flushPipelineIndex == pHolder->pipelineIndex))
                    {
                        if ((pHolder->numOutBuffers != (pHolder->numOkBuffersSent + pHolder->numErrorBuffersSent)) &&
                            (FALSE == pHolder->isCancelled) &&
                            (0 != pHolder->pendingMetadataCount))
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                                                "Pipeline:%s flush in Session %p: Can't advance because only %d "
                                                "buffers out of a total of %d buffers returned for sequence Id: %d",
                                                m_pipelineData[flushPipelineIndex].pPipelineDescriptor->pipelineName,
                                                this,
                                                (pHolder->numOkBuffersSent + pHolder->numErrorBuffersSent),
                                                pHolder->numOutBuffers,
                                                reinterpret_cast<ResultHolder*>(pNode->pData)->sequenceId);
                            canAdvance = FALSE;
                            break;
                        }
                    }
                }
                pNode = m_resultHolderList.NextNode(pNode);
            }
            m_pipelineFlushResultsAv[index] = canAdvance;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid Pipeline Index");
        }
    }

    m_pResultHolderListLock->Unlock();

    if (TRUE == AllPipelinesFlushed())
    {
        NotifyPipelineProcessingDone();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DetermineActiveStreams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DetermineActiveStreams(
    PipelineProcessRequestData*  pPipelineProcessRequestData)
{
    const CaptureRequest* pCaptureRequest = pPipelineProcessRequestData->pCaptureRequest;

    for (UINT frameIndex = 0;
         frameIndex < pCaptureRequest->GetBatchedHALOutputNum(pCaptureRequest);
         frameIndex++)
    {
        PerBatchedFrameInfo* pTopologyPerFrameInfo = &pPipelineProcessRequestData->pPerBatchedFrameInfo[frameIndex];

        pTopologyPerFrameInfo->activeStreamIdMask  = 0;
        pTopologyPerFrameInfo->sequenceId          = pCaptureRequest->pStreamBuffers[frameIndex].sequenceId;

        for (UINT i = 0; i < pCaptureRequest->pStreamBuffers[frameIndex].numOutputBuffers; i++)
        {
            const ChiStreamBuffer*  pOutputBuffer  =
                reinterpret_cast<const ChiStreamBuffer*>(&pCaptureRequest->pStreamBuffers[frameIndex].outputBuffers[i]);
            const ChiStreamWrapper* pStreamWrapper = static_cast<ChiStreamWrapper*>(pOutputBuffer->pStream->pPrivateInfo);

            UINT streamId          = pStreamWrapper->GetStreamIndex();
            UINT currentStreamMask = pTopologyPerFrameInfo->activeStreamIdMask;

            CAMX_ASSERT(TRUE == CamX::IsValidChiBuffer(&pOutputBuffer->bufferInfo));

            pTopologyPerFrameInfo->bufferInfo[streamId]  = pOutputBuffer->bufferInfo;
            pTopologyPerFrameInfo->activeStreamIdMask   |= Utils::BitSet(currentStreamMask, streamId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::UpdateMultiRequestSyncData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::UpdateMultiRequestSyncData(
    const ChiPipelineRequest* pRequest)
{
    UINT        index           = 0;
    UINT32      pipelineIndex   = InvalidPipelineIndex;
    UINT64      comboID         = m_syncSequenceId;
    UINT32      comboIndex      = comboID % MaxQueueDepth;
    UINT32      lastComboIndex  = (comboID - 1) % MaxQueueDepth;

    // just multiple realtime pipeline request in one session
    if (m_numInputSensors >= 2)
    {
        /// refresh request data;
        for (index = 0; index < m_numPipelines; index++)
        {
            m_requestSyncData[comboIndex].prevReq.requestID[index] = m_requestSyncData[lastComboIndex].currReq.requestID[index];
            m_requestSyncData[comboIndex].prevReq.isActive[index]  = m_requestSyncData[lastComboIndex].currReq.isActive[index];
            m_requestSyncData[comboIndex].prevReq.isMaster[index]  = m_requestSyncData[lastComboIndex].currReq.isMaster[index];
            m_requestSyncData[comboIndex].prevReq.isMultiRequest   = m_requestSyncData[lastComboIndex].currReq.isMultiRequest;

            m_requestSyncData[comboIndex].currReq.requestID[index] = m_requestSyncData[comboIndex].prevReq.requestID[index];
            m_requestSyncData[comboIndex].currReq.isActive[index]  = FALSE;
            m_requestSyncData[comboIndex].currReq.isMaster[index]  = FALSE;
            m_requestSyncData[comboIndex].currReq.isMultiRequest   = FALSE;
        }

        m_requestSyncData[comboIndex].syncSequenceId    = m_syncSequenceId;
        m_requestSyncData[comboIndex].numPipelines      = m_numPipelines;

        UINT32     metaTag = 0;
        CamxResult result  = VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainfo", "MasterCamera", &metaTag);
        for (index = 0; index < pRequest->numRequests; index++)
        {
            if (0 != pRequest->pCaptureRequests[index].hPipelineHandle)
            {
                pipelineIndex = GetPipelineIndex(pRequest->pCaptureRequests[index].hPipelineHandle);

                if (pipelineIndex != InvalidPipelineIndex)
                {
                    m_requestSyncData[comboIndex].currReq.requestID[pipelineIndex]
                        = m_requestSyncData[lastComboIndex].currReq.requestID[pipelineIndex] + 1;
                    m_requestSyncData[comboIndex].currReq.isActive[pipelineIndex] = TRUE;

                    if (CamxResultSuccess == result)
                    {
                        Pipeline*       pPipeline   = m_pipelineData[pipelineIndex].pPipeline;
                        MetadataPool*   pInputPool  = pPipeline->GetPerFramePool(PoolType::PerFrameInput);
                        MetadataSlot*   pSlot       =
                            pInputPool->GetSlot(m_requestSyncData[comboIndex].currReq.requestID[pipelineIndex]);

                        BOOL* pMetaValue = reinterpret_cast<BOOL*>(pSlot->GetMetadataByTag(metaTag));
                        if (NULL != pMetaValue)
                        {
                            m_requestSyncData[comboIndex].currReq.isMaster[pipelineIndex] = *pMetaValue;
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupHAL, "Error when retrieving vendor tag: NULL pointer");
                            m_requestSyncData[comboIndex].currReq.isMaster[pipelineIndex] = FALSE;
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupHAL, "Error when querying vendor tag");
                        m_requestSyncData[comboIndex].currReq.isMaster[pipelineIndex] = FALSE;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "unkown pipeline handle request!");
                }
            }
        }

        // just number of ruquest is bigger than 2, it needs 3A sync.
        if (pRequest->numRequests >= 2)
        {
            m_requestSyncData[comboIndex].currReq.isMultiRequest = TRUE;
        }
        else
        {
            m_requestSyncData[comboIndex].currReq.isMultiRequest = FALSE;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupStats, "MultiReq: syncSequenceId:%llu comboIndex:%d Num:%d Multi:%d",
                         m_syncSequenceId, comboIndex,
                         m_requestSyncData[comboIndex].numPipelines,
                         m_requestSyncData[comboIndex].currReq.isMultiRequest);
        for (index = 0; index < m_requestSyncData[comboIndex].numPipelines; index++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "MultiReq: isActive[%d]:%d, Req[%d]:%lu, Master[%d]:%d",
                             index, m_requestSyncData[comboIndex].currReq.isActive[index],
                             index, m_requestSyncData[comboIndex].currReq.requestID[index],
                             index, m_requestSyncData[comboIndex].currReq.isMaster[index]);
        }

        for (index = 0; index < m_requestSyncData[comboIndex].numPipelines; index++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupHAL, "MultiReq_Prev: isActive[%d]:%d, Req[%d]:%lu, Master[%d]:%d",
                             index, m_requestSyncData[comboIndex].prevReq.isActive[index],
                             index, m_requestSyncData[comboIndex].prevReq.requestID[index],
                             index, m_requestSyncData[comboIndex].prevReq.isMaster[index]);
        }

        m_syncSequenceId++;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::CanRequestProceed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::CanRequestProceed(
    const ChiCaptureRequest* pRequest)
{
    CAMX_UNREFERENCED_PARAM(pRequest);

    CamxResult result = CamxResultSuccess;

    if (TRUE == static_cast<BOOL>(CamxAtomicLoad32(&m_aDeviceInError)))
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::CheckValidChiStreamBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::CheckValidChiStreamBuffer(
    const ChiStreamBuffer* pStreamBuffer
    ) const
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pStreamBuffer)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid input");
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        if (sizeof(CHISTREAMBUFFER) != pStreamBuffer->size)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid stream buffer - expected size %u incoming size %u",
                           static_cast<UINT32>(sizeof(CHISTREAMBUFFER)), pStreamBuffer->size);
            result = CamxResultEInvalidArg;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (FALSE == CamX::IsValidChiBuffer(&pStreamBuffer->bufferInfo))
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid stream buffer - bufferType = %d, phBuffer = %p",
                           pStreamBuffer->bufferInfo.bufferType,
                           pStreamBuffer->bufferInfo.phBuffer);
            result = CamxResultEInvalidArg;
        }
    }

    // Print ChiStreamBuffer info - helps in debugging
    if (CamxResultSuccess == result)
    {
        CHISTREAM* pStream = pStreamBuffer->pStream;

        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                         "ChiStreamBuffer : size=%d, pStream=%p, bufferType=%d, phBuffer=%p, "
                         "bufferStatus=%d, acquireFence=(valid=%d type=%d nativeFence=%d chiFence=%p), "
                         "releaseFence=(valid=%d type=%d nativeFence=%d chiFence=%p)",
                         pStreamBuffer->size,
                         pStreamBuffer->pStream,
                         pStreamBuffer->bufferInfo.bufferType,
                         pStreamBuffer->bufferInfo.phBuffer,
                         pStreamBuffer->bufferStatus,
                         pStreamBuffer->acquireFence.valid,
                         pStreamBuffer->acquireFence.type,
                         pStreamBuffer->acquireFence.nativeFenceFD,
                         pStreamBuffer->acquireFence.hChiFence,
                         pStreamBuffer->releaseFence.valid,
                         pStreamBuffer->releaseFence.type,
                         pStreamBuffer->releaseFence.nativeFenceFD,
                         pStreamBuffer->releaseFence.hChiFence);

        if (NULL != pStream)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                             "ChiStream : streamType=%d, width=%d, height=%d, format=%d, grallocUsage=%d, "
                             "maxNumBuffers=%d, pPrivateInfo=%p, dataspace=%d, rotation=%d, planeStride=%d, sliceHeight=%d",
                             pStream->streamType,
                             pStream->width,
                             pStream->height,
                             pStream->format,
                             pStream->grallocUsage,
                             pStream->maxNumBuffers,
                             pStream->pPrivateInfo,
                             pStream->dataspace,
                             pStream->rotation,
                             pStream->streamParams.planeStride,
                             pStream->streamParams.sliceHeight);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::CheckValidInputRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::CheckValidInputRequest(
    const ChiCaptureRequest* pCaptureRequest
    ) const
{
    //// HAL interface requires -EINVAL (EInvalidArg) if the input request is malformed
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCaptureRequest);

    // Validate input ChiStreamBuffer
    if ((0 < pCaptureRequest->numInputs) && (NULL != pCaptureRequest->pInputBuffers))
    {
        for (UINT32 bufferIndex = 0; bufferIndex < pCaptureRequest->numInputs; bufferIndex++)
        {
            result = CheckValidChiStreamBuffer(&pCaptureRequest->pInputBuffers[bufferIndex]);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid input stream buffer bufferIndex=%d, result = %s",
                               bufferIndex, Utils::CamxResultToString(result));
                break;
            }
        }
    }

    // Validate output ChiStreamBuffer
    if ((CamxResultSuccess == result) &&
        (0 < pCaptureRequest->numOutputs) && (NULL != pCaptureRequest->pOutputBuffers))
    {
        for (UINT32 bufferIndex = 0; bufferIndex < pCaptureRequest->numOutputs; bufferIndex++)
        {
            result = CheckValidChiStreamBuffer(&pCaptureRequest->pOutputBuffers[bufferIndex]);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid output stream buffer bufferIndex=%d, result = %s",
                               bufferIndex, Utils::CamxResultToString(result));
                break;
            }
        }
    }

    // Validate metadata
    if ((NULL == pCaptureRequest->pInputMetadata) ||
        (NULL == pCaptureRequest->pOutputMetadata) ||
        (pCaptureRequest->pInputMetadata == pCaptureRequest->pOutputMetadata) ||
        (FALSE == MetaBuffer::IsValid(static_cast<MetaBuffer*>(pCaptureRequest->pInputMetadata))) ||
        (FALSE == MetaBuffer::IsValid(static_cast<MetaBuffer*>(pCaptureRequest->pOutputMetadata))))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupHAL, "ERROR Invalid metadata provided input %p output %p",
                       pCaptureRequest->pInputMetadata,
                       pCaptureRequest->pOutputMetadata);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::WaitOnAcquireFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::WaitOnAcquireFence(
    const ChiCaptureRequest* pRequest)
{
    CamxResult result = CamxResultSuccess;
#if ANDROID
    for (UINT i = 0; i < pRequest->numOutputs; i++)
    {
        if ((TRUE               == pRequest->pOutputBuffers[i].acquireFence.valid) &&
            (ChiFenceTypeNative == pRequest->pOutputBuffers[i].acquireFence.type)  &&
            (InvalidNativeFence != pRequest->pOutputBuffers[i].acquireFence.nativeFenceFD))
        {
            NativeFence acquireFence = pRequest->pOutputBuffers[i].acquireFence.nativeFenceFD;

            CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupSync, "Waiting on Acquire Fence");
            /// @todo (CAMX-2491) Define constant for HalFenceTimeout
            result = OsUtils::NativeFenceWait(acquireFence, 5000);
            CAMX_TRACE_SYNC_END(CamxLogGroupSync);

            if (CamxResultSuccess != result)
            {
                break;
            }

            OsUtils::Close(acquireFence);
        }
    }
#else // ANDROID
    CAMX_UNREFERENCED_PARAM(pRequest);
#endif // ANDROID

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetIntraPipelinePerFramePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataPool* Session::GetIntraPipelinePerFramePool(
    PoolType poolType,
    UINT     pipelineId)
{
    CAMX_ASSERT(pipelineId < m_numPipelines);
    return m_pipelineData[pipelineId].pPipeline->GetPerFramePool(poolType);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetStreamIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE UINT32 Session::GetStreamIndex(
    ChiStream* pStream
    ) const
{
    ChiStreamWrapper* pChiStreamWrapper = static_cast<ChiStreamWrapper*>(pStream->pPrivateInfo);

    return pChiStreamWrapper->GetStreamIndex();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::UnregisterThreadJobCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::UnregisterThreadJobCallback()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::FlushThreadJobCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::FlushThreadJobCallback()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetCurrentSequenceId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Session::GetCurrentSequenceId()
{
    return m_sequenceId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetCurrentPipelineRequestId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 Session::GetCurrentPipelineRequestId(
    UINT pipelineIndex)
{
    CAMX_ASSERT(pipelineIndex < m_numPipelines);
    return m_requestBatchId[pipelineIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::IsResultHolderEmpty
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::IsResultHolderEmpty()
{
    BOOL result     = FALSE;
    UINT numNodes   = 0;

    m_pResultHolderListLock->Lock();
    numNodes = m_resultHolderList.NumNodes();
    m_pResultHolderListLock->Unlock();

    if (0 == numNodes)
    {
        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::IsPipelineRealTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::IsPipelineRealTime(
    CHIPIPELINEHANDLE hPipelineDescriptor)
{
    UINT32     index = 0;

    // input pipelineIndex not really match the index recorded by Session, so use Descriptor to find it.
    for (index = 0; index < m_numPipelines; index++)
    {
        if (hPipelineDescriptor == m_pipelineData[index].pPipelineDescriptor)
        {
            // found corresponding pipeline can use index to get to it
            break;
        }
    }

    CAMX_ASSERT(index < m_numPipelines);

    Pipeline* pPipeline = m_pipelineData[index].pPipeline;
    CAMX_ASSERT(NULL != pPipeline);

    return pPipeline->IsRealTime();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::QueryStreamHDRMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::QueryStreamHDRMode(
    HAL3Stream*              pStream,
    MetadataPool*            pInputPool,
    UINT64                   requestId)
{
    CamxResult                result = CamxResultSuccess;

    if (HDRModeMax == pStream->GetHDRMode())
    {
        // HDR mode can be only changed once when the first capture request given.
        UINT32 metaTag        = 0;
        UINT8  currentHDRMode = HDRModeNone;
        Format format         = pStream->GetInternalFormat();

        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.streamconfigs", "HDRVideoMode", &metaTag);

        if (CamxResultSuccess == result)
        {
            MetadataSlot* pMetaDataSlot = pInputPool->GetSlot(requestId);
            UINT8* pMetaValue = static_cast<UINT8*>(pMetaDataSlot->GetMetadataByTag(metaTag));

            if (NULL != pMetaValue)
            {
                currentHDRMode = *pMetaValue;

                if (currentHDRMode >= HDRModeMax)
                {
                    CAMX_LOG_ERROR(CamxLogGroupHAL, "Invalid HDR mode :%d sent", currentHDRMode);
                    currentHDRMode = HDRModeNone;
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHAL, "query HDR mode vendor tag failed");
        }

        switch (format)
        {
            case Format::UBWCTP10:
            case Format::P010:
                pStream->SetHDRMode(currentHDRMode);
                break;

            default:
                pStream->SetHDRMode(HDRModeNone);
                break;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Set stream HDR mode %d", pStream->GetHDRMode());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::SetPerStreamColorMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::SetPerStreamColorMetadata(
    const ChiCaptureRequest* pRequest,
    MetadataPool*            pInputPool,
    UINT64                   requestId)
{
    CamxResult                result         = CamxResultSuccess;
#if (!defined(LE_CAMERA))
    ColorMetaData             bufferMetadata   = {};
    CHISTREAM*                pStream        = NULL;
    struct private_handle_t*  phBufferHandle = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Request with outbuffer num %d", pRequest->numOutputs);

    for (UINT32 i = 0; i < pRequest->numOutputs; i++)
    {
        pStream = pRequest->pOutputBuffers[i].pStream;

        HAL3Stream*  pHAL3Stream = static_cast<HAL3Stream*>(pStream->pPrivateInfo);

        CAMX_ASSERT(pHAL3Stream != NULL);
        QueryStreamHDRMode(pHAL3Stream, pInputPool, requestId);

        switch (pHAL3Stream->GetHDRMode())
        {
            case HDRModeHLG:
                CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Set HDR mode HLG for req %llu", requestId);

                // In HLG mode, UBWC 10bit-TP
                bufferMetadata.colorPrimaries           = ColorPrimaries_BT2020;
                bufferMetadata.range                    = Range_Full;
                bufferMetadata.transfer                 = Transfer_HLG;
                bufferMetadata.matrixCoefficients       = MatrixCoEff_BT2020;

                bufferMetadata.contentLightLevel.lightLevelSEIEnabled    = TRUE;
                bufferMetadata.contentLightLevel.maxContentLightLevel    = MaxContentLightLevel;
                bufferMetadata.contentLightLevel.minPicAverageLightLevel = MaxFrameAverageLightLevel;

                bufferMetadata.masteringDisplayInfo.colorVolumeSEIEnabled   = TRUE;
                bufferMetadata.masteringDisplayInfo.maxDisplayLuminance     = MaxDisplayLuminance;
                bufferMetadata.masteringDisplayInfo.minDisplayLuminance     = MinDisplayLuminance;

                Utils::Memcpy(&bufferMetadata.masteringDisplayInfo.primaries.rgbPrimaries, PrimariesRGB,
                    sizeof(bufferMetadata.masteringDisplayInfo.primaries.rgbPrimaries));
                Utils::Memcpy(&bufferMetadata.masteringDisplayInfo.primaries.whitePoint, PrimariesWhitePoint,
                    sizeof(bufferMetadata.masteringDisplayInfo.primaries.whitePoint));
                break;
            case HDRModeHDR10:
                CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Set HDR mode HDR10 for req %llu", requestId);

                bufferMetadata.colorPrimaries       = ColorPrimaries_BT2020;
                bufferMetadata.range                = Range_Full;
                bufferMetadata.transfer             = Transfer_SMPTE_ST2084;
                bufferMetadata.matrixCoefficients   = MatrixCoEff_BT2020;

                bufferMetadata.contentLightLevel.lightLevelSEIEnabled    = TRUE;
                bufferMetadata.contentLightLevel.maxContentLightLevel    = MaxContentLightLevel;
                bufferMetadata.contentLightLevel.minPicAverageLightLevel = MaxFrameAverageLightLevel;

                bufferMetadata.masteringDisplayInfo.colorVolumeSEIEnabled   = TRUE;
                bufferMetadata.masteringDisplayInfo.maxDisplayLuminance     = MaxDisplayLuminance;
                bufferMetadata.masteringDisplayInfo.minDisplayLuminance     = MinDisplayLuminance;

                Utils::Memcpy(&bufferMetadata.masteringDisplayInfo.primaries.rgbPrimaries, PrimariesRGB,
                    sizeof(bufferMetadata.masteringDisplayInfo.primaries.rgbPrimaries));
                Utils::Memcpy(&bufferMetadata.masteringDisplayInfo.primaries.whitePoint, PrimariesWhitePoint,
                    sizeof(bufferMetadata.masteringDisplayInfo.primaries.whitePoint));
                break;
            default:
                CAMX_LOG_VERBOSE(CamxLogGroupHAL, "Set HDR mode default for req %llu", requestId);

                // default Color Metadata
                bufferMetadata.colorPrimaries        = ColorPrimaries_BT601_6_625;
                bufferMetadata.range                 = Range_Full;
                bufferMetadata.transfer              = Transfer_SMPTE_170M;
                bufferMetadata.matrixCoefficients    = MatrixCoEff_BT601_6_625;
                break;
        }

        /// @todo (CAMX-2499): Decouple the camx core dependency from the android display API.
        ChiStreamBuffer* pBuffers = pRequest->pOutputBuffers;

        if (FALSE == CamX::IsValidChiBuffer(&pBuffers[i].bufferInfo))
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Output Buffer[%d] : unsupported buffer type %d",
                           i, pBuffers[i].bufferInfo.bufferType);

            result = CamxResultEInvalidArg;
            break;
        }
        else if (FALSE == IsChiNativeBufferType(&pBuffers[i].bufferInfo))
        {
            BufferHandle* phNativeHandle = CamX::GetBufferHandleFromBufferInfo(&pBuffers[i].bufferInfo);

            if (NULL != phNativeHandle)
            {
                // NOWHINE CP036a: exception
                phBufferHandle = const_cast<struct private_handle_t*>(
                                 reinterpret_cast<const struct private_handle_t*>(*phNativeHandle));

                setMetaData(phBufferHandle, COLOR_METADATA, &bufferMetadata);
            }
        }
    }
#endif // !LE_CAMERA
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::SetPerFrameVTTimestampMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::SetPerFrameVTTimestampMetadata(
    const NativeHandle* phNativeBufferHandle,
    MetadataPool*       pPool,
    UINT64              requestId)
{
    UINT64*                   pTimestamp        = NULL;
    struct private_handle_t*  phBufferHandle    = NULL;

    CAMX_ASSERT(NULL != pPool);
    MetadataSlot* pMetadataSlot = pPool->GetSlot(requestId);
    CAMX_ASSERT(NULL != pMetadataSlot);
    pTimestamp = static_cast<UINT64*>(pMetadataSlot->GetMetadataByTag(m_vendorTagIndexTimestamp));

    if (NULL != pTimestamp)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupHAL, "PerFrame Metadata VT timestamp=%llu", *pTimestamp);

        // NOWHINE CP036a: Google API requires const types
        phBufferHandle = const_cast<struct private_handle_t*>(
                reinterpret_cast<const struct private_handle_t*>(phNativeBufferHandle));
#if (!defined(LE_CAMERA))
        setMetaData(phBufferHandle, SET_VT_TIMESTAMP, pTimestamp);
#endif // !LE_CAMERA
        m_qtimerErrIndicated = 0;
    }
    else
    {
        const UINT32 ErrLogThreshold = 20; // Stop spewing the error after this threshold

        if (m_qtimerErrIndicated < ErrLogThreshold)
        {
            m_qtimerErrIndicated++;
            CAMX_LOG_WARN(CamxLogGroupHAL,
                            "Failed to retrieve VT timestamp for requestId: %llu, encoder will fallback to system time",
                            requestId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::SetPerFrameVideoPerfModeMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::SetPerFrameVideoPerfModeMetadata(
    const NativeHandle* phNativeBufferHandle)
{
    struct private_handle_t*  phBufferHandle = NULL;
    UINT32                    videoPerfMode  = 1;

    // NOWHINE CP036a: Google API requires const types
    phBufferHandle = const_cast<struct private_handle_t*>(
        reinterpret_cast<const struct private_handle_t*>(phNativeBufferHandle));

    CAMX_LOG_INFO(CamxLogGroupCore, "Set Video Perf mode %d", videoPerfMode);
#if (!defined(LE_CAMERA))
    setMetaData(phBufferHandle, SET_VIDEO_PERF_MODE, &videoPerfMode);
#endif // !LE_CAMERA
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::SetPerFrameCVPMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::SetPerFrameCVPMetadata(
    const NativeHandle* phNativeBufferHandle,
    UINT32              pipelineIndex,
    UINT64              sequenceId)
{
#if !defined(_MSC_VER) && CVPENABLED
    CVPMetaDataInternal*        pCVPMetaData         = NULL;
    struct private_handle_t*    phBufferHandle       = NULL;
    MetadataPool*               pPool                = NULL;
    MetadataSlot*               pMetadataSlot        = NULL;
    UINT                        captureFps           = m_pipelineData[pipelineIndex].pPipeline->GetFPSValue();
    CVPMetadata                 cvpMetaData;
    Utils::Memset(&cvpMetaData, 0x0, sizeof(CVPMetadata));
    UINT   batchedHALOutputNum  = GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined);
    UINT64 requestId            = (sequenceId / batchedHALOutputNum) + 1; // RequestId starts from 1
    BOOL   repeatMeta           = (0 != (sequenceId % batchedHALOutputNum));

    pPool = GetIntraPipelinePerFramePool(PoolType::PerFrameResult, pipelineIndex);
    if (NULL != pPool)
    {
        pMetadataSlot = pPool->GetSlot(requestId);
    }

    if (NULL != pMetadataSlot)
    {
        pCVPMetaData = static_cast<CVPMetaDataInternal*>(pMetadataSlot->GetMetadataByTag(m_indexCVPMetaData));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "pMetadataSlot is NULL");
    }

    if (NULL != pCVPMetaData)
    {

        CAMX_LOG_VERBOSE(CamxLogGroupCore, "reqid %llu, confidence %d, size %d, payload size %d,"
                         " capturefps %d, batch = %d, repeat %d, valid %x",
                         requestId, pCVPMetaData->nTransformConfidence, sizeof(CVPMetaDataInternal),
                         sizeof(cvpMetaData.payload), captureFps, m_usecaseNumBatchedFrames, repeatMeta,
                         pCVPMetaData->nIsValid);

        // Fill CVP video metadata
        cvpMetaData.size = sizeof(cvpMetaData.payload);
        Utils::Memcpy(&cvpMetaData.payload, pCVPMetaData, sizeof(CVPMetaDataInternal));
        // need to set frame rate in Q16 format
        cvpMetaData.capture_frame_rate = static_cast<UINT32>(Utils::FloatToQNumber(
            static_cast<FLOAT>(captureFps), 1 << 16));
        cvpMetaData.cvp_frame_rate     = static_cast<UINT32>(Utils::FloatToQNumber(
            static_cast<FLOAT>(captureFps) / m_usecaseNumBatchedFrames, 1 << 16));

        if (TRUE == repeatMeta)
        {
            cvpMetaData.flags = CVP_METADATA_FLAG_REPEAT;
        }
        else
        {
            cvpMetaData.flags = CVP_METADATA_FLAG_NONE;
        }
        // NOWHINE CP036a: Google API requires const types
        phBufferHandle = const_cast<struct private_handle_t*>(
                reinterpret_cast<const struct private_handle_t*>(phNativeBufferHandle));

#if (!defined(LE_CAMERA))
        setMetaData(phBufferHandle, SET_CVP_METADATA, &cvpMetaData);
#endif // !LE_CAMERA
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "pCVPMetaData is NULL for %llu, captureFps %d, batch %d",
            requestId, captureFps, m_usecaseNumBatchedFrames);
    }
#else // !defined(_MSC_VER) && CVPENABLED
    CAMX_UNREFERENCED_PARAM(phNativeBufferHandle);
    CAMX_UNREFERENCED_PARAM(pipelineIndex);
    CAMX_UNREFERENCED_PARAM(sequenceId);
#endif // !defined(_MSC_VER) && CVPENABLED
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::DumpResultState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::DumpResultState(
    INT     fd,
    UINT32  indent)
{
    /// @note Accessing with a TryLock since this is intended to be a post-mortem log.  If we try to enforce the lock, theres a
    ///       reasonable chance the post-mortem will deadlock. Failed locks will be noted.
    CamxResult result = m_pResultLock->TryLock();

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_TO_FILE(fd, indent, "WARNING: Lock failed with status: %s.  Stopping dump",
                         CamxResultStrings[result]);
        return;
    }

    LightweightDoublyLinkedListNode* pNode                = m_resultHolderList.Head();
    ResultHolder*                    pResultHolder        = NULL;
    SessionResultHolder*             pSessionResultHolder = NULL;

    while (NULL != pNode)
    {
        if (NULL != pNode->pData)
        {
            pSessionResultHolder = reinterpret_cast<SessionResultHolder*>(pNode->pData);
            CAMX_LOG_TO_FILE(fd, indent + 2,
                             "---------------------------------------------------------------------------------------");
            CAMX_LOG_TO_FILE(fd, indent + 2, "+ Session Result Holder %p; numResults = %u",
                             pSessionResultHolder,
                             pSessionResultHolder->numResults);

            for (UINT resHolderIdx = 0; resHolderIdx < pSessionResultHolder->numResults; resHolderIdx++)
            {
                pResultHolder = &pSessionResultHolder->resultHolders[resHolderIdx];
                if (NULL != pResultHolder)
                {
                    CAMX_LOG_TO_FILE(fd, indent + 2, "----------------------------------------------------");
                    CAMX_LOG_TO_FILE(fd, indent + 4,
                                    "+ Result Holder %u - RequestId = %u, SequenceId = %u, numInBuffers = %u, "
                                     "numOutBuffers = %u",
                                     resHolderIdx,
                                     pResultHolder->requestId,
                                     pResultHolder->sequenceId,
                                     pResultHolder->numInBuffers,
                                     pResultHolder->numOutBuffers);
                    CAMX_LOG_TO_FILE(fd, indent + 4, "--------------------------------");
                    CAMX_LOG_TO_FILE(fd, indent + 6, "numOkBuffersSent     = %u",   pResultHolder->numOkBuffersSent);
                    CAMX_LOG_TO_FILE(fd, indent + 6, "numErrorBuffersSent  = %u",   pResultHolder->numErrorBuffersSent);
                    CAMX_LOG_TO_FILE(fd, indent + 6, "pendingMetadataCount = %u",   pResultHolder->pendingMetadataCount);
                    CAMX_LOG_TO_FILE(fd, indent + 6, "isShutterSentOut     = %s\n",
                                     Utils::BoolToString(pResultHolder->isShutterSentOut));

                    for (UINT i = 0; i < MaxNumOutputBuffers; i++)
                    {
                        if ((NULL != pResultHolder->bufferHolder[i].pStream) ||
                            (NULL != pResultHolder->bufferHolder[i].pBuffer))
                        {
                            CAMX_LOG_TO_FILE(fd, indent+ 6, "bufferHolder[%u] - pStream = %p  pBuffer = %p  error = %d",
                                             i,
                                             pResultHolder->bufferHolder[i].pStream,
                                             pResultHolder->bufferHolder[i].pBuffer,
                                             pResultHolder->bufferHolder[i].error);
                        }
                    }
                    for (UINT i = 0; i < MaxNumInputBuffers; i++)
                    {
                        if ((NULL != pResultHolder->inputbufferHolder[i].pStream) ||
                            (NULL != pResultHolder->inputbufferHolder[i].pBuffer))
                        {
                            CAMX_LOG_TO_FILE(fd, indent + 6, "inputbufferHolder[%u] - pStream = %p  pBuffer = %p ",
                                             i,
                                             pResultHolder->inputbufferHolder[i].pStream,
                                             pResultHolder->inputbufferHolder[i].pBuffer);
                        }
                    }
                }
            }
        }

        // Get the next result holder and see what's going on with it
        pNode = m_resultHolderList.NextNode(pNode);
    }

    if (CamxResultSuccess == result)
    {
        m_pResultLock->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::DumpState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::DumpState(
    INT     fd,
    UINT32  indent,
    SessionDumpFlag flag)
{
    BOOL isFlushing = GetFlushStatus();
// Read from settings text file
    BOOL isPartialMetadataEnabled               = m_numMetadataResults > 1;

    static const CHAR* SessionDumpFlagStrings[] =
    { "Flush", "SigAbort", "ResetKMD", "ResetUMD", "Reset-Recovery", "ChiContextDump" };

    CAMX_LOG_TO_FILE(fd, indent, "+-----------------------------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "+ SESSION %p DUMP FOR %s", this, SessionDumpFlagStrings[static_cast<UINT>(flag)]);
    CAMX_LOG_TO_FILE(fd, indent, "+-----------------------------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "+ numPipelines            = %u",     m_numPipelines);
    CAMX_LOG_TO_FILE(fd, indent, "+ livePendingRequests     = %u",     m_livePendingRequests);
    CAMX_LOG_TO_FILE(fd, indent, "+ maxLivePendingRequests  = %u",     m_maxLivePendingRequests);
    CAMX_LOG_TO_FILE(fd, indent, "+ requestQueueDepth       = %u",     m_requestQueueDepth);
    CAMX_LOG_TO_FILE(fd, indent, "+ FlushStatus             = %s",     Utils::BoolToString(isFlushing));
    CAMX_LOG_TO_FILE(fd, indent, "+ usecaseNumBatchedFrames = %u",     m_usecaseNumBatchedFrames);
    CAMX_LOG_TO_FILE(fd, indent, "+ numRealtimePipelines    = %u",     m_numRealtimePipelines);
    CAMX_LOG_TO_FILE(fd, indent, "+ partialMetadataEnabled  = %s",     Utils::BoolToString(isPartialMetadataEnabled));
    CAMX_LOG_TO_FILE(fd, indent, "+ aDeviceInError          = %s",
                     Utils::BoolToString(static_cast<BOOL>(CamxAtomicLoad32(&m_aDeviceInError))));
    CAMX_LOG_TO_FILE(fd, indent, "+ isTorchWidgetSession    = %s",     Utils::BoolToString(m_isTorchWidgetSession));
    CAMX_LOG_TO_FILE(fd, indent, "+ isRequestBatchingOn     = %s",     Utils::BoolToString(m_isRequestBatchingOn));
    CAMX_LOG_TO_FILE(fd, indent, "+ sequenceId              = %u",     m_sequenceId);
    CAMX_LOG_TO_FILE(fd, indent, "+ syncSequenceId          = %llu\n", m_syncSequenceId);

    CAMX_LOG_TO_FILE(fd, indent + 2, "-----------------------");
    CAMX_LOG_TO_FILE(fd, indent + 2, "+ Pipeline(s) overview:");
    CAMX_LOG_TO_FILE(fd, indent + 2, "-----------------------");
    for (UINT32 i = 0; i < m_numPipelines; i++)
    {
        CAMX_LOG_TO_FILE(fd, indent+ 2,
                         "  Pipeline_%s { cameraId = %u, requestBatchId = %llu, batchedFrameIndex = %u}",
                         m_pipelineData[i].pPipeline->GetPipelineIdentifierString(),
                         m_pipelineData[i].pPipelineDescriptor->cameraId,
                         m_requestBatchId[i],
                         m_batchedFrameIndex[i]);
    }
    CAMX_LOG_TO_FILE(fd, indent, "");

    if ((NULL != m_resultHolderList.Head()) && (NULL != m_resultHolderList.Tail()))
    {
        SessionResultHolder* pSessionResultHolderHead =
            reinterpret_cast<SessionResultHolder*>(m_resultHolderList.Head()->pData);
        SessionResultHolder* pSessionResultHolderTail =
            reinterpret_cast<SessionResultHolder*>(m_resultHolderList.Tail()->pData);

        if ((NULL != pSessionResultHolderHead) && (NULL != pSessionResultHolderTail))
        {
            CAMX_LOG_TO_FILE(fd, indent + 2, " -------------------------------------------------------");
            CAMX_LOG_TO_FILE(fd, indent + 2, "| Waiting for all results  minResult:%llu  maxRequest:%llu |",
                          pSessionResultHolderHead->resultHolders[0].sequenceId,
                          pSessionResultHolderTail->resultHolders[pSessionResultHolderTail->numResults - 1].sequenceId);
            for (UINT j = 0; j < pSessionResultHolderHead->numResults; j++)
            {
                CAMX_LOG_TO_FILE(fd, indent + 2, "| Potentially stuck on Sequence Id:%llu Request Id:%llu     |",
                                 pSessionResultHolderHead->resultHolders[j].sequenceId,
                                 pSessionResultHolderHead->resultHolders[j].requestId);
            }
            CAMX_LOG_TO_FILE(fd, indent + 2, " -------------------------------------------------------\n");
        }
    }

    // Dump the result holder list
    CAMX_LOG_TO_FILE(fd, indent, "+-----------------------------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "+ RESULT HOLDER DUMP, %u PENDING ENTRIES", m_resultHolderList.NumNodes());
    DumpResultState(fd, indent + 2);

    // Dump pipeline info for this session
    CAMX_LOG_TO_FILE(fd, indent, "\n+---------------------------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "+ PIPELINE DEBUG INFO:");
    CAMX_LOG_TO_FILE(fd, indent, "+---------------------------------------------------------------------------------------+");
    for (UINT i = 0; i < m_numPipelines; i++)
    {
        m_pipelineData[i].pPipeline->DumpState(fd, indent + 2);
    }

    // Dump the session's request queue
    CAMX_LOG_TO_FILE(fd, indent, "\n+---------------------------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "+ HAL3 REQUEST QUEUE");
    CAMX_LOG_TO_FILE(fd, indent, "+---------------------------------------------------------------------------------------+");
    m_pRequestQueue->DumpState(fd, indent + 2);

    // Dump this session's DRQ
    CAMX_LOG_TO_FILE(fd, indent, "\n+---------------------------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "+ DEFERRED REQUEST QUEUE");
    CAMX_LOG_TO_FILE(fd, indent, "+---------------------------------------------------------------------------------------+");
    m_pDeferredRequestQueue->DumpState(fd, indent + 2);

    // Dump Job Family State
    CAMX_LOG_TO_FILE(fd, indent, "\n+---------------------------------------------------------------------------------------+");
    CAMX_LOG_TO_FILE(fd, indent, "+ THREAD MANAGER");
    CAMX_LOG_TO_FILE(fd, indent, "+---------------------------------------------------------------------------------------+");
    m_pThreadManager->DumpStateToFile(fd, indent + 2);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::DumpDebugInfo()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::DumpDebugInfo(
    SessionDumpFlag flag)
{
    CAMX_LOG_DUMP(CamxLogGroupCore, "+----------------------------------------------------------+");

    switch (flag)
    {
        case SessionDumpFlag::Flush:
            CAMX_LOG_DUMP(CamxLogGroupCore, "+      CALLING SESSION DUMP %p FOR FLUSH         +", this);
            break;
        case SessionDumpFlag::SigAbort:
            CAMX_LOG_DUMP(CamxLogGroupCore, "+      CALLING SESSION DUMP %p FOR SIGABORT      +", this);
            break;
        case SessionDumpFlag::ResetKMD:
            CAMX_LOG_DUMP(CamxLogGroupCore, "+      CALLING SESSION DUMP %p FOR RESET-KMD     +", this);
            break;
        case SessionDumpFlag::ResetUMD:
            CAMX_LOG_DUMP(CamxLogGroupCore, "+      CALLING SESSION DUMP %p FOR RESET-UMD     +", this);
            break;
        case SessionDumpFlag::ResetRecovery:
            CAMX_LOG_DUMP(CamxLogGroupCore, "+      CALLING SESSION DUMP %p FOR RESET-Recovery     +", this);
            break;
        default:
            CAMX_LOG_DUMP(CamxLogGroupCore, "+            CALLING SESSION DUMP %p             +", this);
            break;
    }

    CAMX_LOG_DUMP(CamxLogGroupCore, "+ numPipelines:             %d", m_numPipelines);
    CAMX_LOG_DUMP(CamxLogGroupCore, "+ livePendingRequests:      %d", m_livePendingRequests);
    CAMX_LOG_DUMP(CamxLogGroupCore, "+ maxLivePendingRequests:   %d", m_maxLivePendingRequests);
    CAMX_LOG_DUMP(CamxLogGroupCore, "+ requestQueueDepth:        %d", m_requestQueueDepth);

    for (UINT32 pipelineIndex= 0; pipelineIndex< m_numPipelines; pipelineIndex++)
    {
        CAMX_LOG_DUMP(CamxLogGroupCore, "+ Pipeline[%u] Flush Status: %d",
                      pipelineIndex,
                      GetPipelineFlushStatus(pipelineIndex));
    }

    CAMX_LOG_DUMP(CamxLogGroupCore, "+ usecaseNumBatchedFrames:  %d", m_usecaseNumBatchedFrames);

    CamxResult result = m_pResultHolderListLock->TryLock();

    if (CamxResultSuccess == result)
    {
        if ((NULL != m_resultHolderList.Head()) && (NULL != m_resultHolderList.Tail()))
        {
            SessionResultHolder* pSessionResultHolderHead =
                reinterpret_cast<SessionResultHolder*>(m_resultHolderList.Head()->pData);
            SessionResultHolder* pSessionResultHolderTail =
                reinterpret_cast<SessionResultHolder*>(m_resultHolderList.Tail()->pData);

            if ((NULL != pSessionResultHolderHead) && (NULL != pSessionResultHolderTail))
            {
                CAMX_LOG_DUMP(CamxLogGroupCore, "+------------------------------------------------------------------+");
                CAMX_LOG_DUMP(CamxLogGroupCore, "+ Waiting for all results min Sequence Id:%d  max Sequence Id:%d",
                    pSessionResultHolderHead->resultHolders[0].sequenceId,
                    pSessionResultHolderTail->resultHolders[pSessionResultHolderTail->numResults - 1].sequenceId);
                CAMX_LOG_DUMP(CamxLogGroupCore, "+------------------------------------------------------------------+");
                CAMX_LOG_DUMP(CamxLogGroupCore, "+ Stuck on Sequence Id: %d Request Id: %d",
                    pSessionResultHolderHead->resultHolders[0].sequenceId,
                    pSessionResultHolderHead->resultHolders[0].requestId);
            }
        }
        m_pResultHolderListLock->Unlock();
    }
    else
    {
        CAMX_LOG_DUMP(CamxLogGroupCore, "WARNING: Lock failed with status: %d. but continuing session dump", result);
    }

    // dumping below info without locking
    for (UINT i = 0; i < m_numPipelines; i++)
    {
        if (NULL != m_pipelineData[i].pPipeline)
        {
            m_pipelineData[i].pPipeline->DumpDebugInfo();
        }
    }

    result = m_pResultHolderListLock->TryLock();

    if (CamxResultSuccess == result)
    {
        LightweightDoublyLinkedListNode*    pNode = m_resultHolderList.Head();
        ResultHolder*                       pResultHolder = NULL;

        while (NULL != pNode)
        {
            if (NULL != pNode->pData)
            {
                SessionResultHolder* pSessionResultHolder = reinterpret_cast<SessionResultHolder*>(pNode->pData);

                for (UINT32 i = 0; i < pSessionResultHolder->numResults; i++)
                {
                    pResultHolder = &pSessionResultHolder->resultHolders[i];
                    m_pDeferredRequestQueue->DumpDebugInfo(pResultHolder->requestId, TRUE);
                }
            }

            // Get the next result holder and see what's going on with it
            pNode = m_resultHolderList.NextNode(pNode);
        }
        m_pResultHolderListLock->Unlock();
    }
    else
    {
        CAMX_LOG_DUMP(CamxLogGroupCore, "WARNING: Lock failed with status: %d. skip DRQ dump", result);
    }

    if (NULL != m_pThreadManager)
    {
        // Dump Job Family State
        m_pThreadManager->DumpStateToLog();
    }

    HwEnvironment::GetInstance()->GetSettingsManager()->DumpOverrideSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::PrepareAndDispatchRequestError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::PrepareAndDispatchRequestError(
    SessionCaptureRequest*  pSessionRequests)
{
    CAMX_ASSERT(NULL != pSessionRequests);

    UINT maxNumBatchedFrames = 0;
    UINT maxNumInputBuffers  = 0;
    UINT maxNumOutputBuffers = 0;

    for (UINT32 request = 0; request < pSessionRequests->numRequests; request++)
    {
        CaptureRequest* pRequest            = static_cast<CaptureRequest*>(&pSessionRequests->requests[request]);
        UINT32          numBatchedFrames    = pRequest->GetBatchedHALOutputNum(pRequest);
        if (NULL != pRequest)
        {
            maxNumBatchedFrames = Utils::MaxUINT(maxNumBatchedFrames, numBatchedFrames);
            for (UINT32 index = 0; index < numBatchedFrames; index++)
            {
                maxNumInputBuffers = Utils::MaxUINT(maxNumInputBuffers, pRequest->pStreamBuffers[index].numInputBuffers);
                maxNumOutputBuffers = Utils::MaxUINT(maxNumOutputBuffers, pRequest->pStreamBuffers[index].numOutputBuffers);
            }
        }
    }

    // Allocate memory to be able to return the result
    ChiCaptureResult* pCaptureResults =
        static_cast<ChiCaptureResult*>(CAMX_CALLOC(maxNumBatchedFrames * sizeof(ChiCaptureResult)));

    ChiStreamBuffer* pInputBuffers  = static_cast<ChiStreamBuffer*>(CAMX_CALLOC(maxNumInputBuffers * sizeof(ChiStreamBuffer)));
    ChiStreamBuffer* pOutputBuffers = static_cast<ChiStreamBuffer*>(CAMX_CALLOC(maxNumOutputBuffers * sizeof(ChiStreamBuffer)));

    if ((NULL  == pCaptureResults) ||
        ((NULL == pInputBuffers)   && (0 < maxNumInputBuffers)) ||
        ((NULL == pOutputBuffers)  && (0 < maxNumOutputBuffers)))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore,
                       "Failed to allocate enough memory. Allocated the following pointers - pCaptureResults: "
                       " %p pInputBuffers %p pOutputBuffers %p",  pCaptureResults, pInputBuffers, pOutputBuffers);
    }
    else
    {
        for (UINT32 request = 0; request < pSessionRequests->numRequests; request++)
        {
            CaptureRequest* pRequest         = static_cast<CaptureRequest*>(&pSessionRequests->requests[request]);

            if (NULL != pRequest)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCore, "Dispatching Request Error for request id %llu",
                    pRequest->pStreamBuffers[request].originalFrameworkNumber);

                for (UINT32 index = 0; index < pRequest->GetBatchedHALOutputNum(pRequest); index++)
                {
                    pCaptureResults[index].frameworkFrameNum  = 0;
                    pCaptureResults[index].numOutputBuffers   = 0;
                    pCaptureResults[index].numPartialMetadata = 0;
                    pCaptureResults[index].pInputMetadata     = NULL;
                    pCaptureResults[index].pOutputMetadata    = NULL;
                    pCaptureResults[index].pInputBuffer       = pInputBuffers;
                    pCaptureResults[index].pOutputBuffers     = pOutputBuffers;
                }

                for (UINT32 index = 0; index < pRequest->GetBatchedHALOutputNum(pRequest); index++)
                {

                    // Prepare and Dispatch Error immediately
                    ChiMessageDescriptor* pNotify = GetNotifyMessageDescriptor();
                    pNotify->messageType = ChiMessageTypeError;
                    pNotify->message.errorMessage.errorMessageCode = static_cast<ChiErrorMessageCode>(MessageCodeRequest);
                    pNotify->pPrivData = pRequest->pPrivData;
                    pNotify->message.errorMessage.pErrorStream = NULL;
                    pNotify->message.errorMessage.frameworkFrameNum =
                        static_cast<UINT32>(pRequest->pStreamBuffers[index].originalFrameworkNumber);

                    DispatchNotify(pNotify);

                    for (UINT32 buffer = 0; buffer < pRequest->pStreamBuffers[index].numOutputBuffers; buffer++)
                    {
                        CAMX_LOG_INFO(CamxLogGroupCore, "Returning error for pending  output buffers - %llu",
                            pRequest->pStreamBuffers[index].originalFrameworkNumber);

                        ChiStreamBuffer* pOutputBuffer = &pRequest->pStreamBuffers[index].outputBuffers[buffer];
                        pOutputBuffer->size = sizeof(ChiStreamBuffer);
                        pOutputBuffer->bufferStatus = BufferStatusError;
                        pOutputBuffer->releaseFence.valid = FALSE;

                        // Copy the stream to be delivered to the framework
                        ChiStreamBuffer* pStreamBuffer =
                            // NOWHINE CP036a: Google API requires const type
                            const_cast<ChiStreamBuffer*>(&pCaptureResults[index].pOutputBuffers[buffer]);
                        Utils::Memcpy(pStreamBuffer, pOutputBuffer, sizeof(ChiStreamBuffer));
                    }

                    for (UINT32 buffer = 0; buffer < pRequest->pStreamBuffers[index].numInputBuffers; buffer++)
                    {
                        CAMX_LOG_INFO(CamxLogGroupCore, "Returning error for pending input buffers - %llu",
                            pRequest->pStreamBuffers[index].originalFrameworkNumber);

                        ChiStreamBuffer* pInputBuffer = &pRequest->pStreamBuffers[index].inputBufferInfo[buffer].inputBuffer;
                        pInputBuffer->size = sizeof(ChiStreamBuffer);
                        pInputBuffer->releaseFence.valid = FALSE;
                        pInputBuffer->bufferStatus = BufferStatusError;

                        ChiStreamBuffer* pStreamBuffer =
                            // NOWHINE CP036a: Google API requires const type
                            const_cast<ChiStreamBuffer*>(&pCaptureResults[index].pInputBuffer[buffer]);
                        Utils::Memcpy(pStreamBuffer, pInputBuffer, sizeof(ChiStreamBuffer));
                    }

                    pCaptureResults[index].numOutputBuffers   = pRequest->pStreamBuffers[index].numOutputBuffers;
                    pCaptureResults[index].pResultMetadata    = NULL;
                    pCaptureResults[index].numPartialMetadata = 1;
                    pCaptureResults[index].pPrivData          = pRequest->pPrivData;
                    pCaptureResults[index].frameworkFrameNum  =
                        static_cast<UINT32>(pRequest->pStreamBuffers[index].originalFrameworkNumber);
                }

                Pipeline* pPipeline = m_pipelineData[pRequest->pipelineIndex].pPipeline;
                pPipeline->UpdateLastFlushedRequestId(pRequest->requestId);
                DispatchResults(pCaptureResults, 1);
            }
        }
    }

    // If we've gotten any buffer ready for this result then we can accept another
    m_pLivePendingRequestsLock->Lock();
    if (0 < m_livePendingRequests)
    {
        m_livePendingRequests--;
        m_pWaitLivePendingRequests->Signal();
    }
    m_pLivePendingRequestsLock->Unlock();

    CAMX_FREE(pCaptureResults);
    CAMX_FREE(pInputBuffers);
    CAMX_FREE(pOutputBuffers);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::FlushHALQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::FlushHALQueue()
{
    m_pRequestLock->Lock();

    while (FALSE == m_pRequestQueue->IsEmpty())
    {
        SessionCaptureRequest* pSessionRequests = static_cast<SessionCaptureRequest*>(m_pRequestQueue->Dequeue());

        if (NULL != pSessionRequests)
        {
            PrepareAndDispatchRequestError(pSessionRequests);
            m_pRequestQueue->Release(pSessionRequests);
        }
    }

    m_pRequestLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::QueryMetadataInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::QueryMetadataInfo(
    const CHIPIPELINEHANDLE hPipelineDescriptor,
    const UINT32            maxPublishTagArraySize,
    UINT32*                 pPublishTagArray,
    UINT32*                 pPublishTagCount,
    UINT32*                 pPartialPublishTagCount,
    UINT32*                 pMaxNumMetaBuffers)
{
    UINT32     index = 0;
    CamxResult result = CamxResultEInvalidArg;

    // input pipelineIndex not really match the index recorded by Session, so use Descriptor to find it.
    for (index = 0; index < m_numPipelines; index++)
    {
        if (hPipelineDescriptor == m_pipelineData[index].pPipelineDescriptor)
        {
            Pipeline* pPipeline = m_pipelineData[index].pPipeline;

            if (NULL != pPipeline)
            {
                result = pPipeline->QueryMetadataInfo(
                    maxPublishTagArraySize,
                    pPublishTagArray,
                    pPublishTagCount,
                    pPartialPublishTagCount,
                    pMaxNumMetaBuffers);
            }
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::ResetMetabufferPendingQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::ResetMetabufferPendingQueue()
{
    Utils::Memset(&m_pendingMetabufferDoneQueue, 0x0, sizeof(m_pendingMetabufferDoneQueue));

    for (UINT32 index = 0; index < m_numPipelines; ++index)
    {
        PipelineMetaBufferDoneEntry* pEntry = &m_pendingMetabufferDoneQueue.pipelineEntry[index];
        Pipeline*                    pPipeline = m_pipelineData[index].pPipeline;

        pEntry->pPipeline = pPipeline;
        // the delay in bacth needs to be updated considering skip
        pEntry->maxDelay =
            (1 == GetBatchedHALOutputNum(m_usecaseNumBatchedFrames, m_HALOutputBufferCombined)) ?
            pPipeline->GetMetaBufferDelay() : pPipeline->GetMetaBufferDelay()* 2;
        pEntry->oldestRequestId = 1;
        pEntry->latestRequestId = 1;

        CAMX_LOG_INFO(CamxLogGroupHAL, "MetaBuffer done Maximum delay for %s is %u",
            pEntry->pPipeline->GetPipelineIdentifierString(),
            pEntry->maxDelay);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::ProcessMetabufferPendingQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::ProcessMetabufferPendingQueue()
{
    m_pPendingMBQueueLock->Lock();

    for (UINT32 sequenceId = m_pendingMetabufferDoneQueue.oldestSequenceId;
        sequenceId <= m_pendingMetabufferDoneQueue.latestSequenceId;
        ++sequenceId)
    {
        UINT32 index = sequenceId % MaxQueueDepth;

        if ((MetaBufferDoneMetaReady | MetaBufferDoneBufferReady) == m_pendingMetabufferDoneQueue.pendingMask[index])
        {
            UINT32 pipelineIndex = m_pendingMetabufferDoneQueue.pipelineIndex[index];
            UINT32 curRequestId = m_pendingMetabufferDoneQueue.requestId[index];

            PipelineMetaBufferDoneEntry* pEntry = &(m_pendingMetabufferDoneQueue.pipelineEntry[pipelineIndex]);

            pEntry->isReady[curRequestId % MaxQueueDepth] = TRUE;

            // update latest request Id
            if (curRequestId > pEntry->latestRequestId)
            {
                pEntry->latestRequestId = curRequestId;
            }

            // Sending all the ready results for the pipeline
            CAMX_LOG_INFO(CamxLogGroupHAL, "Pending Queue pipeline %s request %u old = %u last %u delay %u",
                pEntry->pPipeline->GetPipelineIdentifierString(),
                curRequestId,
                pEntry->oldestRequestId,
                pEntry->latestRequestId,
                pEntry->maxDelay);

            for (UINT32 requestId = pEntry->oldestRequestId;
                requestId + pEntry->maxDelay <= pEntry->latestRequestId; ++requestId)
            {
                BOOL& rIsReady = pEntry->isReady[requestId % MaxQueueDepth];

                if (TRUE == rIsReady)
                {
                    pEntry->pPipeline->ProcessMetadataBufferDone(requestId);
                    rIsReady = FALSE;
                }

                if (requestId == pEntry->oldestRequestId)
                {
                    pEntry->oldestRequestId++;
                }
            }

            if (sequenceId == m_pendingMetabufferDoneQueue.oldestSequenceId)
            {
                m_pendingMetabufferDoneQueue.oldestSequenceId++;
            }

            m_pendingMetabufferDoneQueue.pendingMask[index] = 0;
        }
    }
    m_pPendingMBQueueLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::DumpDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::DumpDebugData(
    SessionDumpFlag flag)
{
    CHAR         dumpFilename[256];
    CamxDateTime systemDateTime;
    FILE*        pFile = NULL;

    OsUtils::GetDateTime(&systemDateTime);
    CamX::OsUtils::SNPrintF(dumpFilename, sizeof(dumpFilename), "%s/%04d%02d%02d_%02d%02d%02d%_session_debug_dump.txt",
        ConfigFileDirectory,
        systemDateTime.year + 1900,
        systemDateTime.month + 1,
        systemDateTime.dayOfMonth,
        systemDateTime.hours,
        systemDateTime.minutes,
        systemDateTime.seconds);

    pFile = CamX::OsUtils::FOpen(dumpFilename, "w");

    if (NULL != pFile)
    {
        INT fileDesc = CamX::OsUtils::FileNo(pFile);

        CAMX_LOG_DUMP(CamxLogGroupCore,
            "Session dumped, please check the session dump at %s",
            dumpFilename);

        DumpState(fileDesc, 0, flag);
        CamX::OsUtils::FClose(pFile);
    }
    else
    {
        CAMX_LOG_DUMP(CamxLogGroupCore, "File failed to open, could not dump session");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::DumpSessionState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::DumpSessionState(
    SessionDumpFlag flag)
{
    UINT numResults = 0;

    if ((NULL == m_pSessionDumpLock) ||
        ((NULL != m_pSessionDumpLock) && (CamxResultSuccess != m_pSessionDumpLock->TryLock())))
    {
        CAMX_LOG_INFO(CamxLogGroupHAL, "Unable to Get Dump Lock, no session dump happening");
    }
    else
    {
        if (TRUE == m_pChiContext->GetStaticSettings()->enableSignal35Tombstone)
        {
            // collect threadcallstacks through tombstone file by raising signal 35
            OsUtils::DumpCallStacksForAllThreads();
        }

        if ((NULL != m_pResultHolderListLock) && (CamxResultSuccess == m_pResultHolderListLock->TryLock()))
        {
            numResults = m_resultHolderList.NumNodes();
            m_pResultHolderListLock->Unlock();
        }

        if (0 != numResults)
        {
            if (static_cast<BOOL>(CamxAtomicLoadU8(&m_aSessionDumpComplete) != TRUE))
            {
                CamxAtomicStoreU8(&m_aSessionDumpComplete, TRUE);
                const StaticSettings* pStaticSettings = m_pChiContext->GetStaticSettings();

                if ((TRUE == pStaticSettings->sessionDumpToFile) || (SessionDumpFlag::SigAbort == flag))
                {
                    DumpDebugData(flag);
                }

                if (TRUE == pStaticSettings->sessionDumpToLog)
                {
                    if ((SessionDumpFlag::Flush == flag) && (TRUE == pStaticSettings->sessionDumpForFlush))
                    {
                        DumpDebugInfo(flag);
                    }
                    else if ((SessionDumpFlag::ResetRecovery == flag) && (TRUE == pStaticSettings->sessionDumpForRecovery))
                    {
                        DumpDebugInfo(flag);
                    }
                    else
                    {
                        DumpDebugInfo(flag);
                    }
                }

                DumpKMDInfo(flag);
                CAMX_LOG_INFO(CamxLogGroupHAL, "Session Dump complete");
            }
        }

        m_pSessionDumpLock->Unlock();

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::SetRealtimePipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::SetRealtimePipeline(
    SessionCreateData* pCreateData)
{
    m_isRealTime = FALSE;

    for (UINT i = 0; i < pCreateData->numPipelines; i++)
    {
        ChiPipelineInfo*      pPipelineInfo       = &pCreateData->pPipelineInfo[i];
        PipelineDescriptor*   pPipelineDescriptor = reinterpret_cast<PipelineDescriptor*>(pPipelineInfo->hPipelineDescriptor);
        if (TRUE == pPipelineDescriptor->flags.isRealTime)
        {
            m_isRealTime = TRUE;
            CAMX_LOG_CONFIG(CamxLogGroupCore, "Session %p Pipeline[%s] Selected Sensor Mode W=%d  H=%d",
                            this,
                            pPipelineDescriptor->pipelineName,
                            pPipelineInfo->pipelineInputInfo.sensorInfo.pSensorModeInfo->frameDimension.width,
                            pPipelineInfo->pipelineInputInfo.sensorInfo.pSensorModeInfo->frameDimension.height);
            break;
        }
    }

    return m_isRealTime;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::CheckActiveSensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::CheckActiveSensor(
    SessionCreateData* pCreateData)
{
    BOOL         isActiveSensor = FALSE;
    CSLHandle    hCSLHandle     = CSLInvalidHandle;
    CamxResult   result;

    result = GetActiveCSLSession(pCreateData, &hCSLHandle);

    if (CSLInvalidHandle != hCSLHandle)
    {
        isActiveSensor = TRUE;
    }

    return isActiveSensor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetActiveCSLSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::GetActiveCSLSession(
    SessionCreateData* pCreateData,
    CSLHandle*         phCSLSession)
{
    CamxResult           result         = CamxResultEFailed;
    const HwEnvironment* pHWEnvironment = HwEnvironment::GetInstance();

    *phCSLSession = CSLInvalidHandle;

    CAMX_ASSERT(NULL != pCreateData);
    CAMX_ASSERT(NULL != pHWEnvironment);

    if (NULL != pHWEnvironment)
    {
        UINT32 cameraID = 0;
        for (UINT i = 0; i < pCreateData->numPipelines; i++)
        {
            ChiPipelineInfo*    pPipelineInfo = &pCreateData->pPipelineInfo[i];
            PipelineDescriptor* pPipelineDescriptor = reinterpret_cast<PipelineDescriptor*>(pPipelineInfo->hPipelineDescriptor);
            HwCameraInfo        cameraInfo;

            cameraID = pPipelineDescriptor->cameraId;
            result = pHWEnvironment->GetCameraInfo(cameraID, &cameraInfo);

            if ((CamxResultSuccess == result) && (1 != pCreateData->flags.u.IsSSM) &&
                (CSLInvalidHandle != cameraInfo.pSensorCaps->hCSLSession[0]))
            {
                CAMX_LOG_CONFIG(CamxLogGroupCore, "cameraId=%d, session=%p, cslsession=%p", cameraID, this,
                                cameraInfo.pSensorCaps->hCSLSession[0]);
                *phCSLSession = cameraInfo.pSensorCaps->hCSLSession[0];
                break;
            }
            else if ((CamxResultSuccess == result) && (1 == pCreateData->flags.u.IsSSM) &&
                     (CSLInvalidHandle != cameraInfo.pSensorCaps->hCSLSession[1]))
            {
                CAMX_LOG_CONFIG(CamxLogGroupCore, "cameraId=%d, session=%p, cslsession=%p", cameraID, this,
                                cameraInfo.pSensorCaps->hCSLSession[1]);
                *phCSLSession = cameraInfo.pSensorCaps->hCSLSession[1];
                break;
            }
        }
        if (CSLInvalidHandle == *phCSLSession)
        {
            CAMX_LOG_WARN(CamxLogGroupCore, "Was not able to retrieve camera info for cameraId=%d, session=%p", cameraID, this);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetFlushResponseTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 Session::GetFlushResponseTime()
{
    UINT64 pipelineWaittime = 0;
    UINT64 totalWaitTime    = 0;
    UINT64 paddingTime      = m_pChiContext->GetStaticSettings()->sessionResponseTimePadding;
    UINT64 maxWaitTime      = m_pChiContext->GetStaticSettings()->sessionMaxFlushWaitTime;

    paddingTime = Utils::MaxUINT64(paddingTime, paddingTime*m_livePendingRequests);

    for (UINT index = 0; index < m_numPipelines; index++)
    {
        pipelineWaittime = m_pipelineData[index].pPipeline->GetFlushResponseTimeInMs();
        totalWaitTime    = Utils::MaxUINT64(pipelineWaittime, totalWaitTime);
    }

    return Utils::MinUINT64(maxWaitTime, totalWaitTime + paddingTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SetupRequestData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SetupRequestData(
    CaptureRequest* pRequest,
    PipelineProcessRequestData* pOutRequestData)
{
    // Pipeline to process this Request
    CamxResult result                     = CamxResultSuccess;
    pOutRequestData->pCaptureRequest      = pRequest;
    UINT numBatchedFrames                 = pRequest->GetBatchedHALOutputNum(pRequest);
    pOutRequestData->pPerBatchedFrameInfo =
        static_cast<PerBatchedFrameInfo*>(CAMX_CALLOC(sizeof(PerBatchedFrameInfo) * numBatchedFrames));

    if (NULL == pOutRequestData->pPerBatchedFrameInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to allocate memory for pPerBatchedFrameInfo!");
        result = CamxResultENoMemory;
    }
    else
    {
        CAMX_ASSERT(NULL != pOutRequestData->pPerBatchedFrameInfo);

        DetermineActiveStreams(pOutRequestData);

        for (UINT batchIndex = 0; batchIndex < numBatchedFrames; batchIndex++)
        {
            CAMX_ASSERT(pOutRequestData->pPerBatchedFrameInfo[batchIndex].activeStreamIdMask > 0);
        }

        // Trigger the fences on the input buffers
        for (UINT batchIndex = 0; batchIndex < numBatchedFrames; batchIndex++)
        {
            UINT32 numInputBuffers = pRequest->pStreamBuffers[batchIndex].numInputBuffers;

            for (UINT32 buffer = 0; buffer < numInputBuffers; buffer++)
            {
                StreamInputBufferInfo* pInputBufferInfo =
                    &(pRequest->pStreamBuffers[batchIndex].inputBufferInfo[buffer]);
                CSLFence               hInternalCSLFence = pInputBufferInfo->fence;
                CSLFence               hExternalCSLFence = CSLInvalidHandle;

                if ((TRUE == pInputBufferInfo->inputBuffer.acquireFence.valid) &&
                    (ChiFenceTypeInternal == pInputBufferInfo->inputBuffer.acquireFence.type))
                {
                    hExternalCSLFence =
                        reinterpret_cast<ChiFence*>(pInputBufferInfo->inputBuffer.acquireFence.hChiFence)->hFence;
                }

                if ((FALSE == IsValidCSLFence(hExternalCSLFence)) &&
                    (TRUE == IsValidCSLFence(hInternalCSLFence)))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupCore,
                        "pRequest->streamBuffers[%u].inputBufferInfo[%u].inputBuffer.phBuffer:%p "
                        "hInternalCSLFence:%llu Signalled",
                        batchIndex,
                        buffer,
                        pRequest->pStreamBuffers[batchIndex].inputBufferInfo[buffer].inputBuffer.bufferInfo.phBuffer,
                        hInternalCSLFence);

                    CSLFenceSignal(hInternalCSLFence, CSLFenceResultSuccess);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::IsDoneProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Session::IsDoneProcessing()
{
    BOOL isDone         = TRUE;
    BOOL isSessionFlush = FALSE;

    UINT32 numPipelines;
    UINT32 pipelineIndexes[MaxPipelinesPerSession] = {};

    if (IsPipelineFlush())
    {
        numPipelines = m_sessionFlushInfo.numPipelines;

        for (UINT index = 0; index < numPipelines; index++)
        {
            pipelineIndexes[index] = GetPipelineIndex(m_sessionFlushInfo.pPipelineFlushInfo[index].hPipelineHandle);
        }
    }
    else
    {
        isSessionFlush = TRUE;
        numPipelines   = m_numPipelines; // All the pipelines in this session
    }

    // make sure all the pipelines have caught up their processing with the session (completed requests we need them to)
    for (UINT32 pipelineIndex = 0; pipelineIndex < numPipelines; pipelineIndex++)
    {
        UINT32 pindex = pipelineIndex;
        if (FALSE == isSessionFlush)
        {
            pindex = pipelineIndexes[pipelineIndex];
        }

        Pipeline* pPipeline = m_pipelineData[pindex].pPipeline;

        if ((pPipeline->GetPipelineStatus() == PipelineStatus::STREAM_ON) &&
            (FALSE == pPipeline->AreAllNodesDone()))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Session %p is not done processing, Waiting for pipeline: %s:%u",
                             this, m_pipelineData[pindex].pPipelineDescriptor->pipelineName,
                             m_pipelineData[pindex].pPipeline->GetPipelineId());

            isDone = FALSE;
            break;
        }
    }

    if (TRUE == isDone)
    {
        if ((TRUE == isSessionFlush))
        {
            m_pLivePendingRequestsLock->Lock();
            if (m_livePendingRequests != 0)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCore, "Session %p is not done processing, Waiting for m_livePendingRequests",
                    this);
                isDone = FALSE;
            }
            CAMX_LOG_INFO(CamxLogGroupCore, "Session Flush: Livepending: %d", m_livePendingRequests);
            m_pLivePendingRequestsLock->Unlock();
        }
        else
        {
            for (UINT32 pipelineIndex = 0; pipelineIndex < numPipelines; pipelineIndex++)
            {
                UINT32    pindex    = pipelineIndexes[pipelineIndex];
                Pipeline* pPipeline = m_pipelineData[pindex].pPipeline;
                if (0 < pPipeline->GetLivePendingRequest())
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupCore, "Pipeline %s is not done processing, waiting for"
                        " livePendingRequests: %d", pPipeline->GetPipelineName(), pPipeline->GetLivePendingRequest());
                    isDone = FALSE;
                    break;
                }
            }
        }
    }

    if (TRUE == isDone)
    {
        if (TRUE == isSessionFlush)
        {
            m_pResultHolderListLock->Lock();
            UINT numNodesInResHolder = m_resultHolderList.NumNodes();
            m_pResultHolderListLock->Unlock();

            if (0 != numNodesInResHolder)
            {
                isDone = FALSE;
                CAMX_LOG_VERBOSE(CamxLogGroupCore,
                    "Session Flush: %p is not done processing, Waiting for results. m_resultHolderList.NumNodes: %u",
                    this, m_resultHolderList.NumNodes());
            }
        }
        else if (TRUE != AllPipelinesFlushed())
        {
            isDone = FALSE;
            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                "Session %p is not done with pipeline flush, Waiting for results.", this);
        }
    }

    return isDone;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::NotifyProcessingDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::NotifyProcessingDone()
{

    m_pWaitForResultsLock->Lock();
    if (TRUE == m_waitforResults)
    {
        // Check if we can tell the flush thread that it can continue
        if (TRUE == IsDoneProcessing())
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "Session %p is done processing Waking up flush thread", this);
            m_pWaitAllResultsAvailable->Signal();
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "Session %p is not done processing", this);
        }
    }
    m_pWaitForResultsLock->Unlock();

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::NotifyPipelineProcessingDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::NotifyPipelineProcessingDone()
{

    m_pWaitForResultsLock->Lock();
    // Check if we can tell the flush thread that it can continue
    if (TRUE == IsDoneProcessing())
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "Session %p is done processing Waking up flush thread", this);
        m_pWaitFlushResultsAvailable->Signal();
        CAMX_TRACE_ASYNC_END_F(CamxLogGroupCore, 0, "WaitTillFlushResultsAvailable");
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "Session %p is not done processing", this);
    }
    m_pWaitForResultsLock->Unlock();

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::CheckAndSyncLinks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::CheckAndSyncLinks()
{
    CamxResult      result    = CamxResultSuccess;

    // 1. Unsync the links if previous mode is sync mode
    if (CSLSyncLinkModeSync == m_linkSyncMode)
    {
        result = CSLSyncLinks(GetCSLSession(),
            m_hLinkHandles, m_numLinksSynced,
            m_hLinkHandles[0], CSLSyncLinkModeNoSync);

        m_numLinksSynced = 0;
        m_linkSyncMode   = CSLSyncLinkModeNoSync;
        Utils::Memset(m_hLinkHandles, 0, sizeof(m_hLinkHandles));

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "Unsync CSL links Success!");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Unsync CSL links failed!");
        }
    }

    // 2. Sync all the streamed on realtime pipelines
    if ((CamxResultSuccess == result) && (m_numStreamedOnRealtimePipelines > 1) &&
        (m_numStreamedOnRealtimePipelines <= MaxSyncLinkCount))
    {
        // 2.1 Get all the streamed on realtime pipeline handles
        for (UINT i = 0; i < m_numPipelines; i++)
        {
            Pipeline* pPipelineObject = m_pipelineData[i].pPipeline;
            if (NULL != pPipelineObject)
            {
                if ((TRUE == pPipelineObject->IsRealTime()) && (TRUE == pPipelineObject->IsStreamedOn()))
                {
                    // Sync link only both the handles are valid
                    if (CSLInvalidHandle != *pPipelineObject->GetCSLLink())
                    {
                        m_hLinkHandles[m_numLinksSynced++] = *pPipelineObject->GetCSLLink();
                    }
                    else
                    {
                        result = CamxResultEFailed;
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Link is not created for pipeline %s[%d] yet!",
                                         pPipelineObject->GetPipelineName(),
                                         pPipelineObject->GetPipelineId());
                        break;
                    }
                }
            }
        }

        // 2.2 Sync the links
        if ((CamxResultSuccess == result) && (m_numStreamedOnRealtimePipelines == m_numLinksSynced))
        {
            result = CSLSyncLinks(GetCSLSession(),
                                  m_hLinkHandles, m_numLinksSynced,
                                  m_hLinkHandles[0], CSLSyncLinkModeSync);

            if (CamxResultSuccess == result)
            {
                m_linkSyncMode = CSLSyncLinkModeSync;
                CAMX_LOG_INFO(CamxLogGroupCore, "Sync CSL links Success!");
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Sync CSL links Failed!");
            }
        }

        // Reset m_hLinkHandles of all pipelines and m_numLinksSynced to zero, if something fails while syncing.
        if (CamxResultSuccess != result)
        {
            m_numLinksSynced = 0;
            Utils::Memset(m_hLinkHandles, 0, sizeof(m_hLinkHandles));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::SyncProcessCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::SyncProcessCaptureRequest(
    const ChiPipelineRequest* pPipelineRequests,
    UINT32*                   pPipelineIndexes)
{
    CamxResult result                                           = CamxResultSuccess;
    UINT       numOfRealtimePipelines                           = 0;
    UINT       realTimePipelineIndex[MaxPipelinesPerSession]    = {};

    if ((TRUE == IsRealtimeSession()) &&
        (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->multiCameraFrameSync))
    {
        // Get the number of real time request
        for (UINT i = 0; i < pPipelineRequests->numRequests; i++)
        {
            UINT pipelineIndex = pPipelineIndexes[i];
            if (TRUE == m_pipelineData[pipelineIndex].pPipeline->IsRealTime())
            {
                realTimePipelineIndex[numOfRealtimePipelines++] = pipelineIndex;
            }
        }

        const ChiCaptureRequest* pCaptureRequest;
        StreamStatus             streamStatus = StreamStatus::NON_SYNC_STREAMING;
        if (1 == numOfRealtimePipelines)
        {
            if (PipelineStatus::STREAM_ON == m_pipelineData[realTimePipelineIndex[0]].pPipeline->GetPipelineStatus())
            {
                // If there is only one real time pipeline request then we should not be doing any sync
                pCaptureRequest = &(pPipelineRequests->pCaptureRequests[0]);
                // NOWHINE CP036a: exception as we explictly need to modify the request to add the addiotonal info
                SetMultiCameraSync(const_cast<ChiCaptureRequest*>(pCaptureRequest), FALSE);
                SetSyncStreamStatus(realTimePipelineIndex[0], StreamStatus::NON_SYNC_STREAMING);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupCore, "Pipeline is Not streamed on");
            }
        }
        else if (2 == numOfRealtimePipelines)
        {
            BOOL bSync              = FALSE;
            BOOL enableAELock       = FALSE;
            UINT pipelineIndex      = 0;

            // If there are two real time requests and both pipelines has been streamed on then we should do sync
            if ((PipelineStatus::STREAM_ON == m_pipelineData[realTimePipelineIndex[0]].pPipeline->GetPipelineStatus()) &&
                (PipelineStatus::STREAM_ON == m_pipelineData[realTimePipelineIndex[1]].pPipeline->GetPipelineStatus()))
            {
                streamStatus    = StreamStatus::SYNC_STREAMING;
                bSync           = TRUE;
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupCore, "Both Pipelines are not Streamed on and we are in Non-Sync Streaming mode");
                streamStatus    = StreamStatus::NON_SYNC_STREAMING;
                enableAELock    = TRUE;
            }

            if (TRUE == enableAELock)
            {
                UINT   aeLockFrames     = HwEnvironment::GetInstance()->GetStaticSettings()->numberOfAELockFrames;
                UINT64 startRequestID   = 0;
                UINT64 stopRequestID    = 0;

                for (UINT i = 0; i < numOfRealtimePipelines; i++)
                {
                    pipelineIndex   = realTimePipelineIndex[i];
                    startRequestID  = m_requestBatchId[pipelineIndex];
                    stopRequestID   = startRequestID + aeLockFrames;
                    if (startRequestID == 0)
                    {
                        // Request ID has not been initialized yet and request ID always starts from 1
                        startRequestID++;
                    }
                    SetAELockRange(pipelineIndex, startRequestID, stopRequestID);
                }
            }

            for (UINT i = 0; i < numOfRealtimePipelines; i++)
            {
                pipelineIndex   = realTimePipelineIndex[i];
                pCaptureRequest = &(pPipelineRequests->pCaptureRequests[i]);
                // NOWHINE CP036a: exception as we explictly need to modify the request to add the addiotonal info
                SetMultiCameraSync(const_cast<ChiCaptureRequest*>(pCaptureRequest), bSync);
                SetSyncStreamStatus(pipelineIndex, streamStatus);
            }
        }
        else if (3 <= numOfRealtimePipelines)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "More than 2 real time pipeline request. How to handle?");
            result = CamxResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::SetMultiCameraSync
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::SetMultiCameraSync(
    ChiCaptureRequest* pCaptureRequest,
    BOOL enable)
{
    CamxResult      result                      = CamxResultSuccess;
    MetaBuffer*     pInputMetabuffer            = reinterpret_cast<MetaBuffer*>(pCaptureRequest->pInputMetadata);
    SyncModeInfo    modeInfo;

    modeInfo.isSyncModeEnabled  = enable;
    result                      = pInputMetabuffer->SetTag(m_sessionSync.syncModeTagID, &modeInfo, 1, sizeof(SyncModeInfo));
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to set Sync");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::SetAELockRange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::SetAELockRange(
    UINT pipelineIndex,
    UINT64 startRequestID,
    UINT64 stopRequestID)
{
    MetadataPool*   pPerUsecasePool      = m_pipelineData[pipelineIndex].pPipeline->GetPerFramePool(PoolType::PerUsecase);
    MetadataSlot*   pUsecasePoolSlot     = pPerUsecasePool->GetSlot(0);
    CamxResult      result;

    result = pUsecasePoolSlot->SetMetadataByTag(PropertyIDUsecaseAESyncStartLockTagID, &startRequestID, 1, "camx_session");

    if (result == CamxResultSuccess)
    {
        result = pUsecasePoolSlot->SetMetadataByTag(PropertyIDUsecaseAESyncStopLockTagID, &stopRequestID, 1, "camx_session");
    }

    if (result == CamxResultSuccess)
    {
        result = pUsecasePoolSlot->PublishMetadata(PropertyIDUsecaseAESyncStartLockTagID);
    }

    if (result == CamxResultSuccess)
    {
        result = pUsecasePoolSlot->PublishMetadata(PropertyIDUsecaseAESyncStopLockTagID);
    }

    if (result != CamxResultSuccess)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to set AE Lock Range");
    }
    else
    {
        CAMX_LOG_CONFIG(CamxLogGroupCore, "Set AE lock Range from %llu - %llu for PipelineIndex:%d",
            startRequestID,
            stopRequestID,
            pipelineIndex);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::AllocateDebugDataPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::AllocateDebugDataPool(
    VOID** ppDebugDataBuffer,
    UINT numSlots)
{
    CamxResult      result          = CamxResultSuccess;
    DebugData*      pDebugData      = {0};
    MetadataSlot*   pSlot           = NULL;
    VOID*           pBlob           = NULL;
    VOID*           pSlotDebugData  = NULL;

    if (NULL == *ppDebugDataBuffer)
    {
        *ppDebugDataBuffer  =
            CAMX_CALLOC_ALIGNED(numSlots * HAL3MetadataUtil::DebugDataSize(DebugDataType::AllTypes), alignof(MAXALIGN_T));

        if (NULL == *ppDebugDataBuffer)
        {
            CAMX_LOG_WARN(CamxLogGroupDebugData, "Fail to allocate debug-data memory size: %zu",
                          (numSlots * HAL3MetadataUtil::DebugDataSize(DebugDataType::AllTypes)));
            result = CamxResultENoMemory;
        }
        else
        {
            m_debugDataSlots = numSlots;
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupDebugData,
                      "Already available memory: slots: %u single slot size: %zu: main debugData allocation: %p, size %zu",
                      numSlots,
                      HAL3MetadataUtil::DebugDataSize(DebugDataType::AllTypes),
                      *ppDebugDataBuffer,
                      (numSlots * HAL3MetadataUtil::DebugDataSize(DebugDataType::AllTypes)));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::GetDebugDataForSlot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::GetDebugDataForSlot(
    VOID** ppSlotDebugData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == m_pDebugDataBuffer)
    {
        *ppSlotDebugData = NULL;
        result = CamxResultENoSuch;

        CAMX_LOG_WARN(CamxLogGroupDebugData, "Debug-data memory not available");
    }

    if (CamxResultSuccess == result)
    {
        // Assign new circular buffer index. This function is no thread safe, so must be run under external locks.
        m_debugDataBufferIndex++;
        m_debugDataBufferIndex = m_debugDataBufferIndex % m_debugDataSlots;

        // Set pointer to corresponding debug-data slot
        *ppSlotDebugData =
            Utils::VoidPtrInc(m_pDebugDataBuffer,
                              (m_debugDataBufferIndex * HAL3MetadataUtil::DebugDataSize(DebugDataType::AllTypes)));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::InitDebugDataSlot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Session::InitDebugDataSlot(
    VOID* pBlob,
    VOID* pDebugDataBase)
{
    CamxResult      result          = CamxResultSuccess;
    DebugData*      pDebugData      = {0};

    // Update of all pointers in blob
    pDebugData = reinterpret_cast<DebugData*>(
        Utils::VoidPtrInc(pBlob, DebugDataPropertyOffsets[PropertyIDDebugDataAll & ~DriverInternalGroupMask]));
    pDebugData->size    = HAL3MetadataUtil::DebugDataSize(DebugDataType::AllTypes);
    pDebugData->pData   = pDebugDataBase;

    pDebugData =
        reinterpret_cast<DebugData*>(
            Utils::VoidPtrInc(pBlob, DebugDataPropertyOffsets[PropertyIDDebugDataAEC & ~DriverInternalGroupMask]));
    if (0 != HAL3MetadataUtil::DebugDataSize(DebugDataType::AEC))
    {
        pDebugData->size    = HAL3MetadataUtil::DebugDataSize(DebugDataType::AEC);
        pDebugData->pData   = Utils::VoidPtrInc(pDebugDataBase, HAL3MetadataUtil::DebugDataOffset(DebugDataType::AEC));
    }
    else
    {
        pDebugData->size    = 0;
        pDebugData->pData   = NULL;
    }

    pDebugData =
        reinterpret_cast<DebugData*>(
            Utils::VoidPtrInc(pBlob, DebugDataPropertyOffsets[PropertyIDDebugDataAWB & ~DriverInternalGroupMask]));
    if (0 != HAL3MetadataUtil::DebugDataSize(DebugDataType::AWB))
    {
        pDebugData->size    = HAL3MetadataUtil::DebugDataSize(DebugDataType::AWB);
        pDebugData->pData   = Utils::VoidPtrInc(pDebugDataBase, HAL3MetadataUtil::DebugDataOffset(DebugDataType::AWB));
    }
    else
    {
        pDebugData->size    = 0;
        pDebugData->pData   = NULL;
    }

    pDebugData =
        reinterpret_cast<DebugData*>(
            Utils::VoidPtrInc(pBlob, DebugDataPropertyOffsets[PropertyIDDebugDataAF  & ~DriverInternalGroupMask]));
    if (0 != HAL3MetadataUtil::DebugDataSize(DebugDataType::AF))
    {
        pDebugData->size    = HAL3MetadataUtil::DebugDataSize(DebugDataType::AF);
        pDebugData->pData   = Utils::VoidPtrInc(pDebugDataBase, HAL3MetadataUtil::DebugDataOffset(DebugDataType::AF));
    }
    else
    {
        pDebugData->size    = 0;
        pDebugData->pData   = NULL;
    }

    pDebugData =
        reinterpret_cast<DebugData*>(
            Utils::VoidPtrInc(pBlob,
                              DebugDataPropertyOffsets[PropertyIDTuningDataIFE  & ~DriverInternalGroupMask]));
    if (0 != HAL3MetadataUtil::DebugDataSize(DebugDataType::IFETuning))
    {
        pDebugData->size    = HAL3MetadataUtil::DebugDataSize(DebugDataType::IFETuning);
        pDebugData->pData   =
            Utils::VoidPtrInc(pDebugDataBase, HAL3MetadataUtil::DebugDataOffset(DebugDataType::IFETuning));
    }
    else
    {
        pDebugData->size    = 0;
        pDebugData->pData   = NULL;
    }

    pDebugData =
        reinterpret_cast<DebugData*>(
            Utils::VoidPtrInc(pBlob, DebugDataPropertyOffsets[PropertyIDTuningDataIPE  & ~DriverInternalGroupMask]));
    if (0 != HAL3MetadataUtil::DebugDataSize(DebugDataType::IPETuning))
    {
        pDebugData->size = HAL3MetadataUtil::DebugDataSize(DebugDataType::IPETuning);
        pDebugData->pData =
            Utils::VoidPtrInc(pDebugDataBase, HAL3MetadataUtil::DebugDataOffset(DebugDataType::IPETuning));
    }
    else
    {
        pDebugData->size = 0;
        pDebugData->pData = NULL;
    }

    pDebugData =
        reinterpret_cast<DebugData*>(
            Utils::VoidPtrInc(pBlob, DebugDataPropertyOffsets[PropertyIDTuningDataBPS  & ~DriverInternalGroupMask]));
    if (0 != HAL3MetadataUtil::DebugDataSize(DebugDataType::BPSTuning))
    {
        pDebugData->size = HAL3MetadataUtil::DebugDataSize(DebugDataType::BPSTuning);
        pDebugData->pData =
            Utils::VoidPtrInc(pDebugDataBase, HAL3MetadataUtil::DebugDataOffset(DebugDataType::BPSTuning));
    }
    else
    {
        pDebugData->size = 0;
        pDebugData->pData = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::PrepareChiRequestErrorForInflightRequests
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::PrepareChiRequestErrorForInflightRequests(
    const ChiPipelineRequest* pPipelineRequests)
{
    if ((NULL == m_pCaptureResultForInflightRequests) && (0 < pPipelineRequests->numRequests))
    {
        m_pCaptureResultForInflightRequests = static_cast<ChiCaptureResult*>(
            CAMX_CALLOC(pPipelineRequests->numRequests * sizeof(ChiCaptureResult)));

        m_numInflightRequests = pPipelineRequests->numRequests;

        if (NULL != m_pCaptureResultForInflightRequests)
        {
            for (UINT requestIndex = 0; requestIndex < pPipelineRequests->numRequests; requestIndex++)
            {
                ChiCaptureResult*        pCaptureResult  = &m_pCaptureResultForInflightRequests[requestIndex];
                const CHICAPTUREREQUEST* pCaptureRequest = &pPipelineRequests->pCaptureRequests[requestIndex];

                ChiStreamBuffer* pInputBuffers  = NULL;
                ChiStreamBuffer* pOutputBuffers = NULL;

                if (0 < pCaptureRequest->numInputs)
                {
                    pInputBuffers = static_cast<ChiStreamBuffer*>
                        (CAMX_CALLOC(pCaptureRequest->numInputs * sizeof(ChiStreamBuffer)));
                }

                if (0 < pCaptureRequest->numOutputs)
                {
                    pOutputBuffers = static_cast<ChiStreamBuffer*>
                        (CAMX_CALLOC(pCaptureRequest->numOutputs * sizeof(ChiStreamBuffer)));
                }

                if (NULL != pInputBuffers)
                {
                    for (UINT8 bufIdx = 0; bufIdx < pCaptureRequest->numInputs; bufIdx++)
                    {
                        Utils::Memcpy(&pInputBuffers[bufIdx],
                            &pCaptureRequest->pInputBuffers[bufIdx],
                            sizeof(ChiStreamBuffer));
                    }
                }

                if (NULL != pOutputBuffers)
                {
                    for (UINT8 bufIdx = 0; bufIdx < pCaptureRequest->numOutputs; bufIdx++)
                    {
                        Utils::Memcpy(&pOutputBuffers[bufIdx],
                            &pCaptureRequest->pOutputBuffers[bufIdx],
                            sizeof(ChiStreamBuffer));
                    }
                }

                CAMX_LOG_INFO(CamxLogGroupCore, "Rejecting capture request with framenum %llu, as session is in flush",
                    pCaptureRequest->frameNumber);


                // Prepare the results and dispatch them
                pCaptureResult->frameworkFrameNum  = static_cast<UINT32>(pCaptureRequest->frameNumber);
                pCaptureResult->numOutputBuffers   = pCaptureRequest->numOutputs;
                pCaptureResult->pResultMetadata    = NULL;
                pCaptureResult->numPartialMetadata = 0;
                pCaptureResult->pPrivData          = reinterpret_cast<CHIPRIVDATA*>(pCaptureRequest->pPrivData);
                pCaptureResult->pInputBuffer       = pInputBuffers;
                pCaptureResult->pOutputBuffers     = pOutputBuffers;

                for (UINT32 result = 0; result < pCaptureRequest->numOutputs; result++)
                {
                    pCaptureRequest->pOutputBuffers[result].size = sizeof(ChiStreamBuffer);
                    pCaptureRequest->pOutputBuffers[result].releaseFence.valid = FALSE;
                    pCaptureRequest->pOutputBuffers[result].bufferStatus = BufferStatusError;
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Allocation failed for session: %p!!", this);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to prepare Chi request error for session: %p, Num Pipeline requests %u ",
            this,
            "m_pCaptureResultForInflightRequests = %p ",
            pPipelineRequests->numRequests,
            m_pCaptureResultForInflightRequests);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::DispatchResultsForInflightRequests
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::DispatchResultsForInflightRequests()
{
    if (NULL != m_pCaptureResultForInflightRequests)
    {
        for (UINT8 requestIdx = 0; requestIdx < m_numInflightRequests; requestIdx++)
        {
            ChiCaptureResult*     pCaptureResult             = &m_pCaptureResultForInflightRequests[requestIdx];
            ChiMessageDescriptor* pNotify                    = GetNotifyMessageDescriptor();
            pNotify->messageType                             = ChiMessageTypeError;
            pNotify->pPrivData                               = static_cast<CHIPRIVDATA*>(pCaptureResult->pPrivData);
            pNotify->message.errorMessage.errorMessageCode   = static_cast<ChiErrorMessageCode>(MessageCodeRequest);
            pNotify->message.errorMessage.frameworkFrameNum  = static_cast<UINT32>(pCaptureResult->frameworkFrameNum);
            pNotify->message.errorMessage.pErrorStream       = NULL; // No stream applicable
            DispatchNotify(pNotify);
        }

        DispatchResults(m_pCaptureResultForInflightRequests, m_numInflightRequests);

        for (UINT8 requestIdx = 0; requestIdx < m_numInflightRequests; requestIdx++)
        {
            if (NULL != m_pCaptureResultForInflightRequests[requestIdx].pInputBuffer)
            {
                // NOWHINE CP036a: exception
                CAMX_FREE(const_cast<ChiStreamBuffer*>(m_pCaptureResultForInflightRequests[requestIdx].pInputBuffer));
                m_pCaptureResultForInflightRequests[requestIdx].pInputBuffer = NULL;
            }
            if (NULL != m_pCaptureResultForInflightRequests[requestIdx].pOutputBuffers)
            {
                // NOWHINE CP036a: exception
                CAMX_FREE(const_cast<ChiStreamBuffer*>(m_pCaptureResultForInflightRequests[requestIdx].pOutputBuffers));
                m_pCaptureResultForInflightRequests[requestIdx].pOutputBuffers = NULL;
            }
        }

        CAMX_FREE(m_pCaptureResultForInflightRequests);
        m_pCaptureResultForInflightRequests = NULL;
        m_numInflightRequests               = 0;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::PipelinesInflightRequestsNotification
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::PipelinesInflightRequestsNotification(
    const ChiPipelineRequest* pPipelineRequests)
{
    UINT   numPipelines                            = pPipelineRequests->numRequests;
    UINT32 pipelineIndexes[MaxPipelinesPerSession] = {};

    for (UINT requestIndex = 0; requestIndex < numPipelines; requestIndex++)
    {
        MetadataPool* pPerUsecasePool        = NULL;
        UINT32        pNumPCRsBeforeStreamOn = 0;
        UINT32*       pTagValue              = NULL;
        UINT32        pTagLocation           = 0;
        CamxResult    result                 = CamxResultSuccess;

        pipelineIndexes[requestIndex]        = GetPipelineIndex(
            pPipelineRequests->pCaptureRequests[requestIndex].hPipelineHandle);
        m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->DecrementLivePendingRequest();

        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Framework frame number: %llu Pipeline: %s LivePendingRequests: %d",
            pPipelineRequests->pCaptureRequests[requestIndex].frameNumber,
            m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->GetPipelineIdentifierString(),
            m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->GetLivePendingRequest());

        pPerUsecasePool = m_pipelineData[pipelineIndexes[requestIndex]].
            pPipeline->GetPerFramePool(PoolType::PerUsecase);

        if (NULL != pPerUsecasePool)
        {
            MetadataSlot* pUsecasePoolSlot = pPerUsecasePool->GetSlot(0);
            result = VendorTagManager::QueryVendorTagLocation("org.codeaurora.qcamera3.sessionParameters",
                                                              "numPCRsBeforeStreamOn",
                                                              &pTagLocation);
            if (CamxResultSuccess == result)
            {
                pTagValue = static_cast<UINT32*>(pUsecasePoolSlot->GetMetadataByTag(pTagLocation));
                if (NULL != pTagValue)
                {
                    pNumPCRsBeforeStreamOn = *pTagValue;
                }
            }
        }

        // Signal flush thread waiting for streamon as pipeline streamon will not come in this case
        if ((m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->IsRealTime()) &&
            (PipelineStatus::STREAM_ON != m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->GetPipelineStatus()) &&
            (pNumPCRsBeforeStreamOn > m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->GetLivePendingRequest()))
        {
            m_pipelineData[pipelineIndexes[requestIndex]].pPipeline->NotifyStreamOnWait();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Session::DumpKMDInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Session::DumpKMDInfo(
    SessionDumpFlag flag)
{
    UINT64             lastSubmittedReqId        = CamxInvalidRequestId;
    UINT64             firstPendingReqId         = ULLONG_MAX;
    UINT64             firstPendingCSLSyncId     = ULLONG_MAX;
    SIZE_T             filledLength              = 0;
    CamxResult         result                    = CamxResultSuccess;
    BOOL               atleastOnepipelinStreamon = FALSE;
    CSLBufferInfo      filledBufferInfo;
    CSLDumpRequestInfo inputKMDDumpInfo;
    CHAR               filename[256];

    for (UINT32 index = 0; index < m_numPipelines; index++)
    {
        Pipeline* pPipeline = m_pipelineData[index].pPipeline;
        if (PipelineStatus::STREAM_ON == pPipeline->GetPipelineStatus())
        {
            pPipeline->CalculateFirstPendingRequest();
            lastSubmittedReqId        = Utils::MaxUINT64(pPipeline->GetLastSubmittedRequestId(), lastSubmittedReqId);
            firstPendingReqId         = Utils::MinUINT64(pPipeline->GetFirstPendingRequestId(), firstPendingReqId);
            firstPendingCSLSyncId     = Utils::MinUINT64(pPipeline->GetFirstPendingCSLSyncId(), firstPendingCSLSyncId);
            atleastOnepipelinStreamon = TRUE;
            CAMX_LOG_DUMP(CamxLogGroupCore,
                "Session: %p flag: %d pipeline: %s firstPendingReqId: %lld firstPendingCSLSyncId: %lld"
                " lastSubmittedRequestId: %lld lastCSLSyncId: %lld", this, flag, pPipeline->GetPipelineIdentifierString(),
                firstPendingReqId, firstPendingCSLSyncId, lastSubmittedReqId, m_lastCSLSyncId);
        }
        else
        {
            CAMX_LOG_DUMP(CamxLogGroupCore,
                "Stream on is not yet done. Session: %p flag: %d pipeline: %s firstPendingReqId: %lld firstPendingCSLSyncId:"
                " %lld lastSubmittedRequestId: %lld lastCSLSyncId: %lld", this, flag, pPipeline->GetPipelineIdentifierString(),
                firstPendingReqId, firstPendingCSLSyncId, lastSubmittedReqId, m_lastCSLSyncId);
        }
    }

    if ((TRUE == atleastOnepipelinStreamon) && ((SessionDumpFlag::ResetRecovery == flag) ||
        (SessionDumpFlag::ResetKMD == flag) || (SessionDumpFlag::ResetUMD == flag)) &&
        (CamxInvalidRequestId != firstPendingReqId))
    {
        result = CSLAlloc(
            "CamxSession",
            &filledBufferInfo,
            KmdBufferSize,
            0,
            CSLMemFlagUMDAccess | CSLMemFlagKMDAccess,
            NULL,
            0);

        if (CamxResultSuccess == result)
        {
            inputKMDDumpInfo.issueRequestId = firstPendingReqId;
            inputKMDDumpInfo.issueSyncId    = firstPendingCSLSyncId;
            inputKMDDumpInfo.lastRequestId  = lastSubmittedReqId;
            inputKMDDumpInfo.lastSyncId     = m_lastCSLSyncId;
            inputKMDDumpInfo.offset         = 0;
            inputKMDDumpInfo.errorType      = 0;
            inputKMDDumpInfo.hBuf           = filledBufferInfo.hHandle;

            result = CSLDumpRequest(
                         GetCSLSession(),
                         &inputKMDDumpInfo,
                         &filledLength);
            if (CamxResultSuccess == result)
            {
                CAMX_LOG_DUMP(CamxLogGroupCore, "buffer fd: %d, address: %p"
                    "filled length: %zu", filledBufferInfo.fd, filledBufferInfo.pVirtualAddr, filledLength);

                if (0 < filledLength)
                {
                    CamX::OsUtils::SNPrintF(filename, sizeof(filename), "%s/KMDSessionDumpData.bin",
                        FileDumpPath);
                    CAMX_LOG_DUMP(CamxLogGroupCore, "Dumping KMD data for session: %p to file: %s", this, filename);
                    FILE* pFile = CamX::OsUtils::FOpen(filename, "wb");
                    if ((NULL != pFile) && (NULL != filledBufferInfo.pVirtualAddr))
                    {
                        CamX::OsUtils::FWrite(filledBufferInfo.pVirtualAddr, filledLength, 1, pFile);
                        CamX::OsUtils::FClose(pFile);
                    }
                }
                else
                {
                    CAMX_LOG_DUMP(CamxLogGroupCore, "No KMD debug info for session: %p", this);
                }
            }
            else
            {
                CAMX_LOG_DUMP(CamxLogGroupCore, "CSLDumpRequest failed");
            }
            CSLReleaseBuffer(filledBufferInfo.hHandle);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                "[%p] Allocation failed, size=%d", this, KmdBufferSize);
        }
    }
}

CAMX_NAMESPACE_END
