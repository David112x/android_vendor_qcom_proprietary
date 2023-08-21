////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxhal3queue.cpp
/// @brief Implements HAL3 Blocking Queue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdebugprint.h"
#include "camxmem.h"
#include "camxhal3queue.h"
#include "camxutils.h"

CAMX_NAMESPACE_BEGIN

// @brief Get the start address of the StreamBufferInfo portion of memory for the given Slot descriptor
#define STREAMBUFFERINFO_ADDR(pDesc) \
    ((pDesc)->pData + sizeof(SessionCaptureRequest))

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief A metadata slot descriptor for an element in the queue
struct SlotDescriptor
{
    BOOL    inUse;      ///< is this queue slot in use by the client (i.e. cannot reuse until it's released by the client)
    UINT32  index;      ///< position of this slot inside m_pDataBlob
    UINT32  signature;  ///< match signature when pData comes back in release
    BYTE*   pData;      ///< actual client data of this slot
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3Queue* HAL3Queue::Create(
    UINT32      maxElements,
    UINT32      numStreamBufferInfo,
    CreatedAs   createType)
{
    CamxResult result          = CamxResultEFailed;
    HAL3Queue* pLocalInstance  = NULL;

    CAMX_ASSERT(0 != maxElements);
    CAMX_ASSERT(0 != numStreamBufferInfo);

    if ((0 != maxElements) && (0 != numStreamBufferInfo))
    {
        pLocalInstance = CAMX_NEW HAL3Queue(maxElements, numStreamBufferInfo);

        if (NULL != pLocalInstance)
        {
            result = pLocalInstance->Initialize(createType);

            if (CamxResultSuccess != result)
            {
                CAMX_DELETE pLocalInstance;
                pLocalInstance = NULL;
            }
        }
    }

    return pLocalInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Queue::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::HAL3Queue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3Queue::HAL3Queue(
    UINT32  maxElements,
    UINT32  numStreamBufferInfo)
    : m_maxSlots(maxElements)
    , m_numStreamBufferInfo(numStreamBufferInfo)
{
    // Reserve space for SessionCaptureRequest and also the StreamBufferInfo[] needed by each
    // SessionCaptureRequest.requests[].pStreamBuffers[] at the end of each Slot.
    m_perSlotDataSize = sizeof(SessionCaptureRequest) +
                        (sizeof(StreamBufferInfo) * numStreamBufferInfo * MaxPipelinesPerSession);
    m_perSlotSize     = m_perSlotDataSize + sizeof(SlotDescriptor);

    CAMX_LOG_VERBOSE(CamxLogGroupCore,
                     "sizeof(SessionCaptureRequest)=%u sizeof(StreamBufferInfo)=%u numStreamBufferInfo=%u "
                     "MaxPipelinesPerSession=%u m_perSlotDataSize=%u",
                     sizeof(SessionCaptureRequest), sizeof(StreamBufferInfo), numStreamBufferInfo, MaxPipelinesPerSession,
                     m_perSlotDataSize);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::~HAL3Queue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HAL3Queue::~HAL3Queue()
{
    if (NULL != m_pDataBlob)
    {
        CAMX_FREE(m_pDataBlob);
        m_pDataBlob = NULL;
    }

    if (NULL != m_pWaitFlush)
    {
        m_pWaitFlush->Destroy();
        m_pWaitFlush = NULL;
    }

    if (NULL != m_pWaitEmpty)
    {
        m_pWaitEmpty->Destroy();
        m_pWaitEmpty = NULL;
    }

    if (NULL != m_pWaitFull)
    {
        m_pWaitFull->Destroy();
        m_pWaitFull = NULL;
    }

    if (NULL != m_pQueueLock)
    {
        m_pQueueLock->Destroy();
        m_pQueueLock = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3Queue::Initialize(
    CreatedAs createType)
{
    CamxResult      result    = CamxResultEFailed;
    UINT            slotIndex = 0;
    SIZE_T          size      = m_perSlotSize * m_maxSlots;
    SlotDescriptor* pDesc     = NULL;

    m_pDataBlob = static_cast<BYTE*>(CAMX_CALLOC_ALIGNED(size, 4));
    CAMX_LOG_VERBOSE(CamxLogGroupCore, "m_pDataBlob=%p TotalSize=%u m_maxSlots=%u m_perSlotSize=%u",
                     m_pDataBlob, size, m_maxSlots, m_perSlotSize);

    if (NULL != m_pDataBlob)
    {
        result = CamxResultSuccess;

        BOOL slotInUse = FALSE;

        switch (createType)
        {
            case CreatedAs::Full:   // Q is created full
                slotInUse = TRUE;
                break;

            case CreatedAs::Empty:  // Q is created empty
            default:
                break;
        }

        for (slotIndex = 0; slotIndex < m_maxSlots; slotIndex++)
        {
            pDesc = GetSlotDescriptor(slotIndex);

            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
            ///                     pDesc
            ///                       | pDesc->pData      Start of StreamBufferInfo data for all pDesc->pData->requests[]
            ///                       | |                 |
            ///  __________ _ ________V_V________ _ ______v_ _ ________ _ ________ _ _________
            /// | |      !!| |      !!| |      !!| |      !!| |      !!| |      !!| |
            /// | |      !!| |      !!| |      !!| |      !!| |      !!| |      !!| |
            /// | |      !!| |      !!| |      !!| |      !!| |      !!| |      !!| |
            /// |_|______!!|_|______!!|_|______!!|_|______!!|_|______!!|_|______!!|_|_________
            /// {_ _    _ _}           ^                  {}
            ///      \ /               |                   |
            ///       |                |                   |
            /// m_perSlotSize   struct SlotDescriptor  sizeof(StreamBufferInfo)*MaxPipelinesPerSession*m_numStreamBufferInfo
            ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            pDesc->pData     = m_pDataBlob + (slotIndex * m_perSlotSize) + sizeof(SlotDescriptor);
            pDesc->inUse     = slotInUse;
            pDesc->index     = slotIndex;
            pDesc->signature = CamxCanary;

            SessionCaptureRequest* pSessionRequest   = reinterpret_cast<SessionCaptureRequest*>(pDesc->pData);
            StreamBufferInfo*      pStreamBuffInBlob = reinterpret_cast<StreamBufferInfo*>(STREAMBUFFERINFO_ADDR(pDesc));

            // Point each pSessionRequest.request[].pStreamBuffers to the space we've reserved at the end of the slot
            for (UINT32 captureIndex = 0; captureIndex < MaxPipelinesPerSession; captureIndex++)
            {
                pSessionRequest->requests[captureIndex].pStreamBuffers =
                    &pStreamBuffInBlob[captureIndex * m_numStreamBufferInfo];
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        m_pQueueLock  = Mutex::Create("Hal3Queue");
        m_pWaitEmpty  = Condition::Create("Hal3Queue Empty");
        m_pWaitFull   = Condition::Create("Hal3Queue Full");
        m_pWaitFlush  = Condition::Create("Hal3Queue Flush");

        if ((NULL == m_pQueueLock) || (NULL == m_pWaitEmpty) || (NULL == m_pWaitFull) || (NULL == m_pWaitFlush))
        {
            result = CamxResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::CanEnqueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HAL3Queue::CanEnqueue() const
{
    // m_tail always points to the slot where the inordered enqueue is to happen next. So if m_tail points to a slot which is
    // currently in use, it means we cannot enqueue anymore - and thats because we enforce an inordered enqueue/dequeue
    BOOL canEnqueue = ((TRUE == GetSlotDescriptor(m_tail)->inUse) ? FALSE : TRUE);

    return canEnqueue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::IsEmpty
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HAL3Queue::IsEmpty() const
{
    // m_head always points to the slot that will be dequeued and m_tail points to a slot in which the next enqueue will happen
    // So if both of them point to slot that is currently not-in-use, it means the queue is empty
    BOOL isEmpty = (((m_head == m_tail) && (FALSE == IsSlotInUse(m_head))) ? TRUE : FALSE);

    return isEmpty;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::ReleaseCore
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Queue::ReleaseCore(
    VOID* pData)
{
    SlotDescriptor* pReleasedDesc = reinterpret_cast<SlotDescriptor*>(static_cast<BYTE*>(pData) - sizeof(SlotDescriptor));

    CAMX_ASSERT(CamxCanary == pReleasedDesc->signature);

    pReleasedDesc->inUse = FALSE;

    // After a release, if we can enqueue now, signal the waiting Enqueues
    if (TRUE == CanEnqueue())
    {
        m_pWaitFull->Signal();
    }

    // After a release, If queue becomes empty, signal anyone waiting for the queue to be empty
    if (TRUE == IsEmpty())
    {
        m_pWaitEmpty->Signal();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::EnqueueCore
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3Queue::EnqueueCore(
    VOID* pData)
{
    CamxResult result = CamxResultEFailed;

    if (TRUE == CanEnqueue())
    {
        SlotDescriptor* pDesc = NULL;
        // Always enqueue at tail
        pDesc        = GetSlotDescriptor(m_tail);

        SessionCaptureRequest* pSessionRequestSrc = reinterpret_cast<SessionCaptureRequest*>(pData);
        SessionCaptureRequest* pSessionRequestDst = reinterpret_cast<SessionCaptureRequest*>(pDesc->pData);

        pSessionRequestDst->numRequests           = pSessionRequestSrc->numRequests;

        StreamBufferInfo* pStreamBuffInBlob       = reinterpret_cast<StreamBufferInfo*>(STREAMBUFFERINFO_ADDR(pDesc));

        result = CamxResultSuccess;
        for (UINT32 i = 0; i < pSessionRequestSrc->numRequests; i++)
        {
            if (pSessionRequestDst->requests[i].pStreamBuffers == pStreamBuffInBlob)
            {
                Utils::Memcpy(&pSessionRequestDst->requests[i], &pSessionRequestSrc->requests[i], sizeof(CaptureRequest));
                pSessionRequestDst->requests[i].pStreamBuffers = pStreamBuffInBlob;

                UINT numBatchedFrames =
                    pSessionRequestSrc->requests[i].GetBatchedHALOutputNum(&pSessionRequestSrc->requests[i]);
                if ((numBatchedFrames <= m_numStreamBufferInfo) &&
                    (NULL != pStreamBuffInBlob) && (NULL != pSessionRequestSrc->requests[i].pStreamBuffers))
                {
                    Utils::Memcpy(pStreamBuffInBlob,
                                  pSessionRequestSrc->requests[i].pStreamBuffers,
                                  (sizeof(StreamBufferInfo) * numBatchedFrames));
                }
                else
                {
                    // This means someone did a direct memcpy/memset/write into our blob instead of using Enqueue()
                    CAMX_LOG_ERROR(CamxLogGroupCore,
                                   "Invalid params: Src->numBatchedFrames=%u m_numStreamBufferInfo=%u"
                                   "pStreamBuffInBlob=%p pSessionRequestSrc->requests[%u].pStreamBuffers=%p",
                                   pSessionRequestSrc->requests[i].numBatchedFrames, m_numStreamBufferInfo,
                                   pStreamBuffInBlob, i, pSessionRequestSrc->requests[i].pStreamBuffers);
                    result = CamxResultEInvalidArg;
                    break;
                }
                pStreamBuffInBlob++;
            }
            else
            {
                // This means someone did a direct memcpy/memset/write into our blob instead of using Enqueue()
                CAMX_LOG_ERROR(CamxLogGroupCore, "Corrupted pointer pStreamBuffers[%u]=%p pStreamBuffInBlob=%p!",
                               i, pSessionRequestDst->requests[i].pStreamBuffers, pStreamBuffInBlob);
                result = CamxResultEInvalidPointer;
                break;
            }
        }

        if (CamxResultSuccess == result)
        {
            pDesc->inUse = TRUE;
            m_tail = ((m_tail + 1) % m_maxSlots);

            // Enqueue always signals the waiting Dequeues
            m_pWaitEmpty->Signal();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::DequeueCore
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* HAL3Queue::DequeueCore()
{
    VOID* pReturnedData = NULL;

    if (FALSE == IsEmpty())
    {
        // Always dequeue from the head
        SlotDescriptor* pDesc = GetSlotDescriptor(m_head);

        pReturnedData = pDesc->pData;

        SessionCaptureRequest* pSessionRequestDst = reinterpret_cast<SessionCaptureRequest*>(pDesc->pData);
        StreamBufferInfo*      pStreamBuffInBlob  = reinterpret_cast<StreamBufferInfo*>(STREAMBUFFERINFO_ADDR(pDesc));

        for (UINT32 i = 0; i < pSessionRequestDst->numRequests; i++)
        {
            if (pSessionRequestDst->requests[i].pStreamBuffers != pStreamBuffInBlob)
            {
                // This means someone did a direct memcpy/memset/write into our blob instead of using Enqueue()
                // or we have some memory corruption somewhere.
                CAMX_LOG_ERROR(CamxLogGroupCore, "pStreamBuffers[%u]=%p pStreamBuffInBlob=%p corrupted!",
                               i, pSessionRequestDst->requests[i].pStreamBuffers, pStreamBuffInBlob);

                pReturnedData = NULL;
                break;
            }
            pStreamBuffInBlob++;
        }

        if (NULL != pReturnedData)
        {
            m_head = ((m_head + 1) % m_maxSlots);

            CAMX_ASSERT(TRUE == pDesc->inUse);
        }
    }

    return pReturnedData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::Enqueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3Queue::Enqueue(
    VOID* pData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pData);

    if (NULL == pData)
    {
        return CamxResultEFailed;
    }

    m_pQueueLock->Lock();

    result = EnqueueCore(pData);

    m_pQueueLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::Dequeue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* HAL3Queue::Dequeue()
{
    VOID* pData = NULL;

    m_pQueueLock->Lock();

    pData = DequeueCore();

    if (TRUE == IsEmpty())
    {
        m_pQueueLock->Unlock();
        m_pWaitFlush->Signal();
    }
    else
    {
        m_pQueueLock->Unlock();
    }

    return pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::Release
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Queue::Release(
    VOID* pData)
{
    CAMX_ASSERT(NULL != pData);

    if (NULL != pData)
    {
        m_pQueueLock->Lock();
        ReleaseCore(pData);
        m_pQueueLock->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::EnqueueWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HAL3Queue::EnqueueWait(
    VOID* pData)
{
    CamxResult result = CamxResultEFailed;

    CAMX_ASSERT(NULL != pData);

    m_pQueueLock->Lock();

    if (NULL != pData)
    {
        do
        {
            while ((FALSE == m_cancelFullWait) && (FALSE == CanEnqueue()))
            {
                m_pWaitFull->Wait(m_pQueueLock->GetNativeHandle());
            }

            if (FALSE == m_cancelFullWait)
            {
                result = EnqueueCore(pData);
            }
            // We currently only use enqueue wait in Session::ProcessCaptureRequest, if recovery is in progress,
            // we cancel the HAL queue wait and we want to return timeout instead of failure
            else if (TRUE == m_recoveryInProgress)
            {
                CAMX_LOG_INFO(CamxLogGroupHAL, "Recovery in progress, returning timeout");
                result = CamxResultETimeout;
                break;
            }
            else
            {
                break;
            }
        } while (result != CamxResultSuccess);
    }

    m_pQueueLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::DequeueWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* HAL3Queue::DequeueWait()
{
    VOID* pElement = NULL;

    m_pQueueLock->Lock();

    do
    {
        while ((FALSE == m_cancelEmptyWait) && (TRUE == IsEmpty()))
        {
            m_pWaitEmpty->Wait(m_pQueueLock->GetNativeHandle());
        }

        if (FALSE == m_cancelEmptyWait)
        {
            pElement = DequeueCore();
        }
        else
        {
            break;
        }
    } while (NULL == pElement);

    m_pQueueLock->Unlock();

    return pElement;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::WaitEmpty
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Queue::WaitEmpty()
{
    m_pQueueLock->Lock();

    while (FALSE == IsEmpty())
    {
        m_pWaitFlush->Wait(m_pQueueLock->GetNativeHandle());
    }

    m_pQueueLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::CancelWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Queue::CancelWait()
{
    m_pQueueLock->Lock();

    // First cancel all enqueue-s waiting
    m_cancelFullWait = TRUE;
    m_pWaitFull->Broadcast();

    // Then cancel all dequeue-s waiting
    m_cancelEmptyWait = TRUE;
    m_pWaitEmpty->Broadcast();

    m_pQueueLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::EnableWait
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Queue::EnableWait()
{
    // Re-enable enqueue wait
    m_cancelFullWait = FALSE;

    // Re-enable dequeue wait
    m_cancelEmptyWait = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HAL3Queue::IsSlotInUse
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HAL3Queue::IsSlotInUse(
    UINT32 slotIndex
    ) const
{
    CAMX_ASSERT(slotIndex < m_maxSlots);

    return GetSlotDescriptor(slotIndex)->inUse;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HAL3Queue::DumpState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HAL3Queue::DumpState(
    INT     fd,
    UINT32  indent)
{
    /// @note Accessing with a TryLock since this is intended to be a post-mortem log.  If we try to enforce the lock, there's a
    ///       reasonable chance the post-mortem will deadlock. Failed locks will be noted.
    CamxResult result = m_pQueueLock->TryLock();
    CAMX_LOG_TO_FILE(fd, indent, "m_pQueueLockAcquireStatus = \"%s\"", CamxResultStrings[result]);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_TO_FILE(fd, indent, "WARNING: Lock failed with status: %s.  Stopping dump", CamxResultStrings[result]);
        return;
    }

    if (FALSE == IsEmpty())
    {
        UINT32          headSlot    = m_head;
        SlotDescriptor* pDesc                           = GetSlotDescriptor(headSlot);
        // Total hack...Session is the only place that uses the HAL3Queue, and SessionCaptureRequest is the structure stored.
        // if more things use this, should store a callback function pointer for the structured dump on creation (or dump a
        // blob and post process the log).
        SessionCaptureRequest* pSessionRequest          = reinterpret_cast<SessionCaptureRequest*>(pDesc->pData);

        while (headSlot != m_tail)
        {
            CAMX_LOG_TO_FILE(fd, indent, "Slot = %u,  inUse = %s",
                             pDesc->index,
                             Utils::BoolToString(pDesc->inUse));

            for (UINT requestIndex = 0; requestIndex < pSessionRequest->numRequests; requestIndex++)
            {
                CaptureRequest* pReq = &(pSessionRequest->requests[requestIndex]);
                CAMX_LOG_TO_FILE(fd, indent + 2, "requestId = %llu, CSLSyncID = %llu, numBatchedFrames: %u, pipelineIndex: %u, "
                                " isMultiRequest: %s",
                                 pReq->requestId,
                                 pReq->CSLSyncID,
                                 pReq->numBatchedFrames,
                                 pReq->pipelineIndex,
                                 Utils::BoolToString(pReq->pMultiRequestData->currReq.isMultiRequest));

                UINT numBatchedFrames = pReq->GetBatchedHALOutputNum(pReq);
                for (UINT i = 0; i < numBatchedFrames; i++)
                {
                    CAMX_LOG_TO_FILE(fd, indent + 4, "streamBuffer_%u, sequenceId = %u, originalFrameworkNumber = %llu",
                                     i,
                                     pReq->pStreamBuffers[i].sequenceId,
                                     pReq->pStreamBuffers[i].originalFrameworkNumber);

                    for (UINT j = 0; j < pReq->pStreamBuffers[i].numOutputBuffers; j++)
                    {
                        const ChiStreamBuffer* pOutputBuffer = &(pReq->pStreamBuffers[i].outputBuffers[j]);

                        ///< @todo (CAMX-4114) Do not assume incoming CHI buffer is always Native Handle
                        CAMX_LOG_TO_FILE(fd, indent + 6, "outputBuffer_%u", j);
                        CAMX_LOG_TO_FILE(fd, indent + 8,
                                         "stream = %p,  buffer = { type = %u. address = %p, fd = %d, bufferStatus = %d }",
                                         pOutputBuffer->pStream,
                                         pOutputBuffer->bufferInfo.bufferType,
                                         pOutputBuffer->bufferInfo.phBuffer,
                                         (reinterpret_cast<NativeHandle*> (pOutputBuffer->bufferInfo.phBuffer))->data[0],
                                         pOutputBuffer->bufferStatus);

                        CAMX_LOG_TO_FILE(fd, indent + 8,
                                         "acquireFence = { valid = %s, type = %u, nativeFence = %d, chiFence = %p }"
                                         "releaseFence = { valid = %s, type = %u, nativeFence = %d, chiFence = %p }",
                                         Utils::BoolToString(pOutputBuffer->acquireFence.valid),
                                         pOutputBuffer->acquireFence.type,
                                         pOutputBuffer->acquireFence.nativeFenceFD,
                                         pOutputBuffer->acquireFence.hChiFence,
                                         Utils::BoolToString(pOutputBuffer->releaseFence.valid),
                                         pOutputBuffer->releaseFence.type,
                                         pOutputBuffer->releaseFence.nativeFenceFD,
                                         pOutputBuffer->releaseFence.hChiFence);
                    }

                    for (UINT j = 0; j < pReq->pStreamBuffers[i].numInputBuffers; j++)
                    {
                        const StreamInputBufferInfo* pInputBufferInfo = &(pReq->pStreamBuffers[i].inputBufferInfo[j]);

                        CAMX_LOG_TO_FILE(fd, indent + 6, "inputBufferInfo_%u", j);
                        CAMX_LOG_TO_FILE(fd, indent + 8, "CSLFence = 0x%08X, port = %u, isChiFence = %s",
                                         pInputBufferInfo->fence,
                                         pInputBufferInfo->portId,
                                         pInputBufferInfo->isChiFence);
                        CAMX_LOG_TO_FILE(fd, indent + 8,
                                         "stream: %p, buffer = { type %u, address = %p, fd: %d. bufferStatus = %d }",
                                         pInputBufferInfo->inputBuffer.pStream,
                                         pInputBufferInfo->inputBuffer.bufferInfo.bufferType,
                                         pInputBufferInfo->inputBuffer.bufferInfo.phBuffer,
                                         reinterpret_cast<NativeHandle*>(
                                                                    pInputBufferInfo->inputBuffer.bufferInfo.phBuffer)->data[0],
                                         pInputBufferInfo->inputBuffer.bufferStatus);
                        CAMX_LOG_TO_FILE(fd, indent + 8,
                                         "acquireFence = { valid = %s, type = %u, nativeFence = %d, chiFence = %p }"
                                         "releaseFence = { valid = %s, type = %u, nativeFence = %d, chiFence = %p }",
                                          Utils::BoolToString(pInputBufferInfo->inputBuffer.acquireFence.valid),
                                          pInputBufferInfo->inputBuffer.acquireFence.type,
                                          pInputBufferInfo->inputBuffer.acquireFence.nativeFenceFD,
                                          pInputBufferInfo->inputBuffer.acquireFence.hChiFence,
                                          Utils::BoolToString(pInputBufferInfo->inputBuffer.releaseFence.valid),
                                          pInputBufferInfo->inputBuffer.releaseFence.type,
                                          pInputBufferInfo->inputBuffer.releaseFence.nativeFenceFD,
                                          pInputBufferInfo->inputBuffer.releaseFence.hChiFence);
                    }
                    if (TRUE == pReq->pMultiRequestData->currReq.isMultiRequest)
                    {
                        CAMX_LOG_TO_FILE(fd, indent + 6, "MultiRequestData syncSequenceId = %llu",
                                         pReq->pMultiRequestData->syncSequenceId);

                        for (UINT j = 0; j < MaxPipelinesPerSession; j++)
                        {
                            if (TRUE == pReq->pMultiRequestData->currReq.isActive[j])
                            {
                                CAMX_LOG_TO_FILE(fd, indent + 8, "Pipeline_%u RequestId= %llu",
                                                 j,
                                                 pReq->pMultiRequestData->currReq.requestID[j]);
                            }
                        }
                    }
                }
            }

            headSlot = ((headSlot + 1) % m_maxSlots);
            pDesc = GetSlotDescriptor(headSlot);
        }
    }
    else
    {
        CAMX_LOG_TO_FILE(fd, indent + 3, "Queue is currently empty...m_head: %d  m_tail: %d", m_head, m_tail);
    }

    if (CamxResultSuccess == result)
    {
        m_pQueueLock->Unlock();
    }
}

CAMX_NAMESPACE_END
