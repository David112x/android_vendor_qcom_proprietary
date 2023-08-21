////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camximagebuffermanager.cpp
///
/// @brief Image buffer manager implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camximagebuffermanager.h"
#include "camximageformatutils.h"
#include "camxmem.h"
#include "camxmempoolmgr.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBufferManager::Create(
    const CHAR*              pBufferManagerName,
    BufferManagerCreateData* pCreateData,
    ImageBufferManager**     ppImageBufferManager)
{
    CamxResult          result              = CamxResultSuccess;
    ImageBufferManager* pImageBufferManager = NULL;

    if ((NULL == pCreateData) || (NULL == pBufferManagerName) || (NULL == ppImageBufferManager))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid arg %p %p %p", pCreateData, pBufferManagerName, ppImageBufferManager);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        *ppImageBufferManager = NULL;

        pImageBufferManager = CAMX_NEW ImageBufferManager();

        if (NULL == pImageBufferManager)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed in creating ImageBufferManager object", pBufferManagerName);
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        result = pImageBufferManager->Initialize(pBufferManagerName, pCreateData);

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupMemMgr, "[%s] : allocRequired=%d, immediateAllocBufferCount=%d, maxBufferCount=%d",
                          pBufferManagerName, pCreateData->allocateBufferMemory, pCreateData->immediateAllocBufferCount,
                          pCreateData->maxBufferCount);

            result = pImageBufferManager->InitializeBuffers(pCreateData);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed in InitializeBuffers, result=%s",
                               pBufferManagerName, Utils::CamxResultToString(result));
            }
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed in Initialize, result=%s",
                           pBufferManagerName, Utils::CamxResultToString(result));
            pImageBufferManager->Destroy();
            pImageBufferManager = NULL;
        }
    }

    if (CamxResultSuccess == result)
    {
        *ppImageBufferManager = pImageBufferManager;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBufferManager::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::ImageBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageBufferManager::ImageBufferManager()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::~ImageBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageBufferManager::~ImageBufferManager()
{
    SIZE_T size = 0;

    if (TRUE == m_createData.allocateBufferMemory)
    {
        size = ImageFormatUtils::GetTotalSize(&m_createData.bufferProperties.imageFormat);
    }

    CAMX_LOG_INFO(CamxLogGroupMemMgr,
                  "[%s] : ImageBufferManager stats : CreateData : (Type=%s, heap=%d, Immediate=%d, Max=%d), Actual Peak=%d, "
                  "AllocRequired=%d, size=%d, numBatches=%d",
                  GetBufferManagerName(),
                  (BufferManagerType::CamxBufferManager == m_createData.bufferManagerType) ? "CamX" : "CHI",
                  m_createData.bufferProperties.bufferHeap, m_immediateAllocBufferCount, m_maxBufferCount, m_peakBufferHolders,
                  m_createData.allocateBufferMemory, size, m_createData.numBatchedFrames);

    // Clean up buffers and allocated resources no matter what.

    m_pLock->Lock();
    FreeBuffers(FALSE, TRUE);
    m_pLock->Unlock();

    FreeResources();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBufferManager::Initialize(
    const CHAR*                 pBufferManagerName,
    BufferManagerCreateData*    pCreateData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pCreateData);

    CAMX_ASSERT(NULL != pBufferManagerName);
    OsUtils::StrLCpy(m_pBufferManagerName, pBufferManagerName, MaxStringLength256);

    m_pLock = Mutex::Create("ImageBufferManager");
    CAMX_ASSERT(NULL != m_pLock);

    m_pWaitFreeBuffer = Condition::Create("ImageBuffer available");
    CAMX_ASSERT(NULL != m_pWaitFreeBuffer);

    if ((NULL == m_pLock) || (NULL == m_pWaitFreeBuffer))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed in creating lock/condition, m_pLock=%p, m_pWaitFreeBuffer=%p",
                       pBufferManagerName, m_pLock, m_pWaitFreeBuffer);
        result = CamxResultEFailed;
    }

    if ((TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->MPMEnable) &&
        (TRUE == pCreateData->allocateBufferMemory))
    {
        // We need to go to Memory Pool only if this Buffer Manager needs memory.
        // For holder Buffer managers (exa - HAL ports), we do not need to register with Memory Pool
        m_hMemPoolBufMgrHandle = MemPoolMgr::RegisterBufferManager(pBufferManagerName, pCreateData);

        if (NULL == m_hMemPoolBufMgrHandle)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] MemPoolMgr Register failed", pBufferManagerName);

            FreeResources();
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] : MemPoolMgr Register success handle=%p",
                             pBufferManagerName, m_hMemPoolBufMgrHandle);
        }
    }
    else
    {
        m_hMemPoolBufMgrHandle = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::InitializeBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBufferManager::InitializeBuffers(
    BufferManagerCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCreateData);
    CAMX_ASSERT(pCreateData->immediateAllocBufferCount <= pCreateData->maxBufferCount);

    if (NULL == pCreateData)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Invalid input pCreateData", m_pBufferManagerName);
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) &&
        (TRUE              == pCreateData->allocateBufferMemory) &&
        (0                 == ImageFormatUtils::GetTotalSize(&pCreateData->bufferProperties.imageFormat)))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Invalid size 0, width=%d, height=%d, format=%d",
                       m_pBufferManagerName,
                       pCreateData->bufferProperties.imageFormat.width,
                       pCreateData->bufferProperties.imageFormat.height,
                       pCreateData->bufferProperties.imageFormat.format);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        // Store the inputs provided to the buffer manager as this information is common to all
        // the buffers allocated
        m_createData                = *pCreateData;
        m_currentFormat             = pCreateData->bufferProperties.imageFormat;
        m_maxBufferCount            = pCreateData->maxBufferCount;
        m_immediateAllocBufferCount = pCreateData->immediateAllocBufferCount;

        // If the total number of buffers to allocate is less than the minimum number of buffers to be allocated,
        // the number of buffers to be immediately allocated is updated accordingly
        if (m_maxBufferCount < m_immediateAllocBufferCount)
        {
            m_immediateAllocBufferCount = m_maxBufferCount;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] Max buffers = %d Immediately allocated buffers = %d",
                         GetBufferManagerName(), m_maxBufferCount, m_immediateAllocBufferCount);

        m_pLock->Lock();

        // Allocate the minimum number of buffers needed immediately serially
        for (UINT i = 0; i < m_immediateAllocBufferCount; i++)
        {
            ImageBuffer* pBuffer = CAMX_NEW ImageBuffer();

            CAMX_ASSERT(NULL != pBuffer);

            if (NULL != pBuffer)
            {
                result = pBuffer->Initialize(m_pBufferManagerName, &m_createData, &m_currentFormat, this,
                                             m_hMemPoolBufMgrHandle);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed in initializing ImageBuffer object, result=%s",
                                   GetBufferManagerName(), Utils::CamxResultToString(result));
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed in allocating ImageBuffer object", GetBufferManagerName());
                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess == result)
            {
                if ((TRUE  == m_createData.allocateBufferMemory) &&
                    (FALSE == m_createData.bEnableLateBinding))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] [buf.camx] [%d/%d] allocating buffer, size:%d ",
                                     GetBufferManagerName(),
                                     i,
                                     m_immediateAllocBufferCount,
                                     ImageFormatUtils::GetTotalSize(&m_createData.bufferProperties.imageFormat));

                    // Allocate the buffer
                    result = pBuffer->Allocate();

                    CAMX_ASSERT(CamxResultSuccess == result);
                }

                if (CamxResultSuccess == result)
                {
                    LDLLNode* pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));

                    if (NULL != pNode)
                    {
                        pNode->pData = pBuffer;

                        // Add the buffer to the free buffer list
                        m_freeBufferList.InsertToTail(pNode);

                        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] ImageBuffer %p", GetBufferManagerName(), pBuffer);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed in allocating Node", GetBufferManagerName());
                        result = CamxResultENoMemory;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed in Buffer allocate", GetBufferManagerName());
                }
            }

            if (CamxResultSuccess != result)
            {
                if (NULL != pBuffer)
                {
                    CAMX_DELETE pBuffer;
                    pBuffer = NULL;
                }

                // There was an error, break out and clean up.
                break;
            }
        }

        if (CamxResultSuccess != result)
        {
            FreeBuffers(FALSE, FALSE);
        }

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] Free buffers = %d Busy buffers = %d",
                         GetBufferManagerName(), m_freeBufferList.NumNodes(), m_busyBufferList.NumNodes());

        m_pLock->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::GetImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageBuffer* ImageBufferManager::GetImageBuffer()
{
    m_pLock->Lock();

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] Free buffers = %d Busy buffers = %d",
                     GetBufferManagerName(), m_freeBufferList.NumNodes(), m_busyBufferList.NumNodes());

    LDLLNode*    pNode   = m_freeBufferList.RemoveFromHead();
    ImageBuffer* pBuffer = NULL;

    // Check the free List for an available buffer
    if (NULL != pNode)
    {
        pBuffer = static_cast<ImageBuffer*>(pNode->pData);

        CAMX_FREE(pNode);
        pNode = NULL;

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] Returning a buffer from the free list", GetBufferManagerName());
    }

    // If no buffer in the free list, search the entire busy list for buffers with reference count 0. If more than one
    // buffer is found with a reference count of 0, the additional buffers will be returned to the free list.
    if (NULL == pBuffer)
    {
        pNode = m_busyBufferList.Head();

        while (NULL != pNode)
        {
            ImageBuffer* pBusyBuffer = static_cast<ImageBuffer*>(pNode->pData);
            LDLLNode*    pNext       = LightweightDoublyLinkedList::NextNode(pNode);

            if (0 == pBusyBuffer->GetReferenceCount())
            {
                m_busyBufferList.RemoveNode(pNode);

                if (NULL == pBuffer)
                {
                    // We will keep this buffer
                    pBuffer = pBusyBuffer;
                    CAMX_FREE(pNode);
                    pNode = NULL;
                }
                else
                {
                    // We already found a buffer, but we can move this unreferenced busy buffer to the free list.
                    m_freeBufferList.InsertToTail(pNode);
                }
            }
            pNode = pNext;
        }

        if (NULL != pBuffer)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] Returning a buffer from the busy list", GetBufferManagerName());
        }
    }

    if (NULL == pBuffer)
    {
        UINT numOfFreeBuffers = m_freeBufferList.NumNodes();
        UINT numOfBusyBuffers = m_busyBufferList.NumNodes();

        // If no free buffers were found either in the free list or the busy list, we check to see if an additional buffer
        // can be allocated immediately
        if ((numOfFreeBuffers + numOfBusyBuffers) < m_maxBufferCount)
        {
            CamxResult result = CamxResultSuccess;

            pBuffer = CAMX_NEW ImageBuffer();

            CAMX_ASSERT(NULL != pBuffer);

            if (NULL != pBuffer)
            {
                result = pBuffer->Initialize(m_pBufferManagerName, &m_createData, &m_currentFormat, this,
                                             m_hMemPoolBufMgrHandle);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Failed in initializing ImageBuffer object, result=%s",
                                   GetBufferManagerName(), Utils::CamxResultToString(result));
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Ran out of memory to allocate", GetBufferManagerName());

                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess == result)
            {
                if ((TRUE == m_createData.allocateBufferMemory) &&
                    (FALSE == m_createData.bEnableLateBinding))
                {
                    // Allocate the buffer
                    result = pBuffer->Allocate();

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] ImageBuffer allocation failed with result = %s",
                                       GetBufferManagerName(), Utils::CamxResultToString(result));
                    }
                }

                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] ImageBuffer %p", GetBufferManagerName(), pBuffer);
            }
            else
            {
                if (NULL != pBuffer)
                {
                    CAMX_DELETE pBuffer;
                    pBuffer = NULL;
                }
            }
        }
    }

    if (NULL == pBuffer)
    {
        CamxResult            result                    = CamxResultSuccess;
        const StaticSettings* pStaticSettings           = HwEnvironment::GetInstance()->GetStaticSettings();
        static const UINT     BufferTimeoutMilliseconds = pStaticSettings->imageBufferWaitTime;
        static const UINT     MaxTimeoutCount           = pStaticSettings->maxImageBufferTimeoutCount;
        UINT                  timeoutCount              = 0;

        CAMX_LOG_INFO(CamxLogGroupMemMgr, "[%s] About to wait for a free buffer: Free buffers = %d Busy buffers = %d",
                      GetBufferManagerName(), m_freeBufferList.NumNodes(), m_busyBufferList.NumNodes());

        // If no free buffers were found either in the free list or the busy list and no more buffers can be allocated,
        // we wait until a busy buffer becomes free

        pNode = NULL;

        while (NULL == pNode)
        {
            result = m_pWaitFreeBuffer->TimedWait(m_pLock->GetNativeHandle(), BufferTimeoutMilliseconds);

            pNode = m_freeBufferList.RemoveFromHead();

            if (timeoutCount >= MaxTimeoutCount)
            {
                // we waited the max number of times we can wait for, time to bail out
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Waited %ux%u times and failed to get buffer, returning a null buffer",
                               GetBufferManagerName(), timeoutCount, BufferTimeoutMilliseconds);
                break;
            }
            else if (CamxResultETimeout == result)
            {
                // timeout happened, print an error log and continue waiting
                timeoutCount++;

                CAMX_LOG_WARN(CamxLogGroupMemMgr, "[%s] Waiting %ux%d ms timedout, there might be a leak or performance issue",
                               GetBufferManagerName(), timeoutCount, BufferTimeoutMilliseconds);
            }
            else if (CamxResultSuccess != result)
            {
                // something wrong while waiting on TimedWait, do not continue, break the loop and raise an error
                timeoutCount++;
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Waiting %dx%d ms failed for unknown reason, result=%s",
                               GetBufferManagerName(), timeoutCount, BufferTimeoutMilliseconds,
                               Utils::CamxResultToString(result));
                break;
            }
        }

        // Check the free List for an available buffer
        if (NULL != pNode)
        {
            pBuffer = static_cast<ImageBuffer*>(pNode->pData);
            CAMX_FREE(pNode);
            pNode   = NULL;

            CAMX_LOG_INFO(CamxLogGroupMemMgr, "[%s] Returning a buffer [%p] from the free list after wait ended",
                          GetBufferManagerName(), pBuffer);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Unable to find a buffer in the free buffer list after wait ended",
                           GetBufferManagerName());
        }
    }

    // Found a buffer. Increment it's reference count and add to the busy list.
    if (NULL != pBuffer)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] GetImageBuffer is returning ImageBuffer %p",
                         GetBufferManagerName(), pBuffer);

        m_pLock->Unlock();
        pBuffer->AddImageReference();
        m_pLock->Lock();

        pBuffer->SetBusyState(TRUE);

        pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));

        if (NULL != pNode)
        {
            pNode->pData = pBuffer;
            m_busyBufferList.InsertToTail(pNode);

            m_peakBufferHolders = Utils::MaxUINT(m_peakBufferHolders, m_busyBufferList.NumNodes());
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] ? Leaking an image buffer: %p", GetBufferManagerName(), pBuffer);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Unable to allocate free buffers and no more free buffers available",
                       GetBufferManagerName());
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] Free buffers = %d Busy buffers = %d",
                     GetBufferManagerName(), m_freeBufferList.NumNodes(), m_busyBufferList.NumNodes());

    m_pLock->Unlock();

    return pBuffer;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::AddReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ImageBufferManager::AddReference(
    ImageBuffer* pImageBuffer)
{
    BOOL      doneFlag = FALSE;
    UINT      count    = 0;
    LDLLNode* pNode;

    m_pLock->Lock();

    pNode = m_freeBufferList.Head();

    while (NULL != pNode)
    {
        // Found the image buffer in free list
        if (pImageBuffer == static_cast<ImageBuffer*>(pNode->pData))
        {
            // Add reference to the image buffer
            count = pImageBuffer->AddImageReference();

            // Move if from free list to busy list
            m_freeBufferList.RemoveNode(pNode);
            pImageBuffer->SetBusyState(TRUE);
            m_busyBufferList.InsertToTail(pNode);

            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] Image buffer %p in free list, reference count after adding = %d",
                             GetBufferManagerName(), pImageBuffer, pImageBuffer->GetReferenceCount());

            doneFlag = TRUE;
            break;
        }
        pNode = LightweightDoublyLinkedList::NextNode(pNode);
    }

    if (FALSE == doneFlag)
    {
        pNode = m_busyBufferList.Head();

        // Didn't find the image buffer in free list, then search busy list
        while (NULL != pNode)
        {
            // Found the image buffer in busy list
            if (pImageBuffer == static_cast<ImageBuffer*>(pNode->pData))
            {
                // Add reference to the image buffer
                count = pImageBuffer->AddImageReference();

                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] Image buffer %p in busy list, reference count after adding = %d",
                                 GetBufferManagerName(), pImageBuffer, pImageBuffer->GetReferenceCount());

                doneFlag = TRUE;
                break;
            }
            pNode = LightweightDoublyLinkedList::NextNode(pNode);
        }
    }

    if (FALSE == doneFlag)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Add reference to image buffer %p failed.",
                       GetBufferManagerName(), pImageBuffer);
    }

    m_pLock->Unlock();

    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::ReleaseReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ImageBufferManager::ReleaseReference(
    ImageBuffer* pImageBuffer)
{
    BOOL doneFlag = FALSE;
    UINT count    = 0;

    if (NULL == m_pLock)
    {
        return count;
    }

    m_pLock->Lock();

    LDLLNode* pNode = m_busyBufferList.Head();

    while (NULL != pNode)
    {
        // Found the image buffer in busy list
        if (pImageBuffer == static_cast<ImageBuffer*>(pNode->pData))
        {
            if (0 < pImageBuffer->GetReferenceCount())
            {
                // Release reference to the image buffer
                count = pImageBuffer->ReleaseImageReference();

                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] Image buffer %p in busy list, reference count after releasing = %d",
                                 GetBufferManagerName(), pImageBuffer, count);

                if (0 == count)
                {
                    // Move the buffer to the free list
                    m_busyBufferList.RemoveNode(pNode);

                    if ((TRUE == m_createData.bEnableLateBinding)   &&
                        (TRUE == m_createData.allocateBufferMemory) &&
                        (TRUE == pImageBuffer->HasBackingBuffer()))
                    {
                        // If late binding is enabled, release the buffer and just add the holder ImageBuffer object into
                        // free list. When this ImageBuffer is acquired, explicit BindBuffer on this ImageBuffer object will be
                        // called to allocate or bind the buffer back to this ImageBuffer
                        pImageBuffer->Release(FALSE);
                    }

                    pImageBuffer->SetBusyState(FALSE);
                    m_freeBufferList.InsertToTail(pNode);

                    // Signal any threads waiting for a free buffer
                    m_pWaitFreeBuffer->Signal();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                               "[%s] Release reference to image buffer %p failed. Reference count is already 0",
                               GetBufferManagerName(), pImageBuffer);
            }

            doneFlag = TRUE;
            break;
        }

        pNode = LightweightDoublyLinkedList::NextNode(pNode);
    }

    if (FALSE == doneFlag)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Didn't find the image buffer %p in busy list.",
                       GetBufferManagerName(), pImageBuffer);
    }

    m_pLock->Unlock();

    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::BindBufferManagerImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBufferManager::BindBufferManagerImageBuffer(
    ImageBuffer* pImageBuffer)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pImageBuffer)
    {
        m_pLock->Lock();

        if (TRUE == pImageBuffer->IsInValidUsageState())
        {
            if (FALSE == pImageBuffer->HasBackingBuffer())
            {
                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] : Binding Buffer to ImageBuffer with allocate=%d",
                                 GetBufferManagerName(), m_createData.allocateBufferMemory);

                if (TRUE == m_createData.allocateBufferMemory)
                {
                    // This is an ImageBuffer which allocates memory, i.e internal link manager ImageBuffer object.
                    // Allocate buffer now.
                    result = pImageBuffer->Allocate();
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Failed in allocating buffer, result=%s",
                                       GetBufferManagerName(), Utils::CamxResultToString(result));
                    }
                }
                else
                {
                    // This is a holder ImageBuffer object which is importing ChiNative buffer (different ImageBuffer object)
                    // For holder ImageBuffers that are importing HAL buffers or ChiGralloc buffers, we would have already
                    // imported buffer and so HasBackingBuffer would have been TRUE, so we will not come here for those
                    // ImageBuffer objects.
                    // We will come here only if this holder ImageBuffer object is importing a ChiNative object which has
                    // LateBinding enabled. So, we need to allocate buffer on that ImageBuffer object being imported. And then
                    // import the buffer information.

                    result = pImageBuffer->BindImportedBuffer();
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Failed in binding imported buffer, result=%s",
                                       GetBufferManagerName(), Utils::CamxResultToString(result));
                    }
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] ImageBuffer is in release state (in free list), can't bind buffer now",
                           GetBufferManagerName(), pImageBuffer);
        }

        m_pLock->Unlock();
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::Activate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBufferManager::Activate()
{
    CamxResult result = CamxResultSuccess;

    m_pLock->Lock();

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "Activate Buffer Manager %s", GetBufferManagerName());

    if (NULL != m_hMemPoolBufMgrHandle)
    {
        result = MemPoolMgr::ActivateBufferManager(m_hMemPoolBufMgrHandle);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] MemPoolMgr activate failed for handle=%p, result=%s",
                           GetBufferManagerName(), m_hMemPoolBufMgrHandle, Utils::CamxResultToString(result));
        }
    }

    m_pLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::Deactivate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBufferManager::Deactivate(
    BOOL isPartialRelease)
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "Deactivate Buffer Manager %s, isPartialRelease=%d",
                     GetBufferManagerName(), isPartialRelease);

    m_pLock->Lock();

    FreeBuffers(isPartialRelease, FALSE);

    if (NULL != m_hMemPoolBufMgrHandle)
    {
        result = MemPoolMgr::DeactivateBufferManager(m_hMemPoolBufMgrHandle);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] MemPoolMgr Deactivate failed for handle=%p, result=%s",
                           GetBufferManagerName(), m_hMemPoolBufMgrHandle, Utils::CamxResultToString(result));
        }
    }

    m_pLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::GetBufferManagerName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHAR* ImageBufferManager::GetBufferManagerName()
{
    return m_pBufferManagerName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::IsBuffersListPartiallyCleared
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageBufferManager::IsBuffersListPartiallyCleared(
    BOOL isPartialRelease)
{
    BOOL listPartiallyCleared = FALSE;

    if (TRUE == isPartialRelease)
    {
        listPartiallyCleared = ((m_freeBufferList.NumNodes() + m_busyBufferList.NumNodes()) <= m_immediateAllocBufferCount);
    }

    return listPartiallyCleared;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::FreeBufferFromList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageBufferManager::FreeBufferFromList(
    LightweightDoublyLinkedList* pBufferList,
    BOOL                         isForced,
    const CHAR*                  pBufferListDescription)
{
    BOOL      emptyList = FALSE;
    LDLLNode* pListNode = NULL;

    if ((NULL != pBufferList) &&
        (NULL != (pListNode = pBufferList->RemoveFromHead())))
    {
        ImageBuffer* pBuffer        = static_cast<ImageBuffer*>(pListNode->pData);
        UINT         bufferRefCount = pBuffer->GetReferenceCount();

        if (NULL != pBuffer)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupUtils, "[%s] Releasing ImageBuffer %x from %s with reference count of %d",
                GetBufferManagerName(), pBuffer, pBufferListDescription, pBuffer->GetReferenceCount());

            pBuffer->Release(isForced);

            CAMX_DELETE pBuffer;
            pBuffer = NULL;
        }

        CAMX_FREE(pListNode);
        pListNode = NULL;
    }
    else
    {
        CAMX_ASSERT_MESSAGE((NULL != pBufferList), "pBufferList %p", pBufferList);
        emptyList = TRUE;
    }

    return emptyList;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::ClearFreeListBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBufferManager::ClearFreeListBuffers(
    BOOL isPartialRelease,
    BOOL isForced)
{
    while (TRUE)
    {
        BOOL listPartiallyCleared = IsBuffersListPartiallyCleared(isPartialRelease);

        if (TRUE == listPartiallyCleared)
        {
            break;
        }

        BOOL emptyList = FreeBufferFromList(&m_freeBufferList, isForced, "FreeList");

        if (TRUE == emptyList)
        {
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::ClearBusyListBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBufferManager::ClearBusyListBuffers(
    BOOL isPartialRelease,
    BOOL isForced)
{
    while (TRUE)
    {
        BOOL listPartiallyCleared = IsBuffersListPartiallyCleared(isPartialRelease);

        if (TRUE == listPartiallyCleared)
        {
            break;
        }

        BOOL emptyList = FreeBufferFromList(&m_busyBufferList, isForced, "BusyList");

        if (TRUE == emptyList)
        {
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::FreeBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBufferManager::FreeBuffers(
    BOOL isPartialRelease,
    BOOL isForced)
{
    ClearFreeListBuffers(isPartialRelease, isForced);
    ClearBusyListBuffers(isPartialRelease, isForced);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::FreeResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBufferManager::FreeResources()
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "Free Buffer Manager %s", GetBufferManagerName());

    if (NULL != m_hMemPoolBufMgrHandle)
    {
        result = MemPoolMgr::UnregisterBufferManager(m_hMemPoolBufMgrHandle);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] MemPoolMgr Unregister failed for handle=%p, result=%s",
                           GetBufferManagerName(), m_hMemPoolBufMgrHandle, Utils::CamxResultToString(result));
        }
        else
        {
            m_hMemPoolBufMgrHandle = NULL;
        }
    }

    if (NULL != m_pWaitFreeBuffer)
    {
        m_pWaitFreeBuffer->Destroy();
        m_pWaitFreeBuffer = NULL;
    }

    if (NULL != m_pLock)
    {
        m_pLock->Destroy();
        m_pLock = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::UpdateImageFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBufferManager::UpdateImageFormat(
    const ImageFormat* pFormat)
{
    CamxResult result = CamxResultSuccess;

    m_pLock->Lock();

    if (NULL == pFormat)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Invalid input arg", GetBufferManagerName());
        result = CamxResultEInvalidArg;
    }
    else if (0 != m_busyBufferList.NumNodes())
    {
        // Double check :
        // What do we expect the state of this ImageBufferManager has to be at this time?
        // can have Image Buffers in busy list?
        // No problem in updating format properties for ImageBuffers in Free list, but how do we handle that are in busy list

        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Can't update format properties while some of the ImageBuffers are in use",
                       GetBufferManagerName());
        result = CamxResultEUnsupported;
    }
    else if (ImageFormatUtils::GetTotalSize(&m_createData.bufferProperties.imageFormat) <
             ImageFormatUtils::GetTotalSize(pFormat))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Can not update format, allocated size %d can't be less than updated %d",
                       GetBufferManagerName(),
                       ImageFormatUtils::GetTotalSize(&m_createData.bufferProperties.imageFormat),
                       ImageFormatUtils::GetTotalSize(pFormat));
        result = CamxResultENeedMore;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] : Update Image Format for Buffer Manager", GetBufferManagerName());

        LDLLNode*       pNode                   = m_freeBufferList.Head();
        UINT            numImageBuffersUpdated  = 0;
        ImageBuffer*    pImageBuffer;

        while (NULL != pNode)
        {
            pImageBuffer = static_cast<ImageBuffer*>(pNode->pData);

            CAMX_ASSERT(NULL != pImageBuffer);

            result = pImageBuffer->UpdateImageFormat(pFormat);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed in updating format %d", GetBufferManagerName(), result);
                break;
            }

            numImageBuffersUpdated++;

            pNode = LightweightDoublyLinkedList::NextNode(pNode);
        }

        if (CamxResultSuccess == result)
        {
            // First update import/current format for this ImageBufferManager
            m_currentFormat = *pFormat;
        }
        else
        {
            // If failed to update, lets set the format back to original one.
            UINT i = 0;

            pNode = m_freeBufferList.Head();

            while ((NULL != pNode) && (i < numImageBuffersUpdated))
            {
                pImageBuffer = static_cast<ImageBuffer*>(pNode->pData);

                CAMX_ASSERT(NULL != pImageBuffer);

                pImageBuffer->UpdateImageFormat(&m_currentFormat);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed in updating format %d, result=%s",
                                   GetBufferManagerName(), i, Utils::CamxResultToString(result));
                    break;
                }

                i++;

                pNode = LightweightDoublyLinkedList::NextNode(pNode);
            }
        }
    }

    m_pLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBufferManager::DumpState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBufferManager::DumpState(
    INT     fd,
    UINT32  indent)
{
    auto bufMgrTypeToString = [](BufferManagerType t) -> const CHAR*
    {
        switch (t)
        {
            case BufferManagerType::CamxBufferManager: return "CamxBufferManager";
            case BufferManagerType::ChiBufferManager:  return "ChiBufferManager";
            default:                                   return "Undefined";
        }
    };

    CAMX_LOG_TO_FILE(fd, indent, "ImageBufferManager_%s", m_pBufferManagerName);
    CAMX_LOG_TO_FILE(fd, indent + 2, "address                = %p", this);
    CAMX_LOG_TO_FILE(fd, indent + 2, "allocateBufferMemory   = %d", m_createData.allocateBufferMemory);
    CAMX_LOG_TO_FILE(fd, indent + 2, "enableLateBinding      = %d", m_createData.bEnableLateBinding);
    CAMX_LOG_TO_FILE(fd, indent + 2, "disableSelfShrinking   = %d", m_createData.bDisableSelfShrinking);
    CAMX_LOG_TO_FILE(fd, indent + 2, "NeedDedicatedBuffers   = %d", m_createData.bNeedDedicatedBuffers);
    CAMX_LOG_TO_FILE(fd, indent + 2, "deviceCount            = %u", m_createData.deviceCount);
    CAMX_LOG_TO_FILE(fd, indent + 2, "deviceIndices          = %d", m_createData.deviceIndices);
    CAMX_LOG_TO_FILE(fd, indent + 2, "immediateAllocBufCount = %u", m_createData.immediateAllocBufferCount);
    CAMX_LOG_TO_FILE(fd, indent + 2, "numBatchedFrames       = %u", m_createData.numBatchedFrames);
    CAMX_LOG_TO_FILE(fd, indent + 2, "maxBufferCount         = %u", m_createData.maxBufferCount);
    CAMX_LOG_TO_FILE(fd, indent + 2, "bufferManagerType      = \"%s\"", bufMgrTypeToString(m_createData.bufferManagerType));

    CamxResult result = m_pLock->TryLock();
    if (CamxResultSuccess == result)
    {
        CAMX_LOG_TO_FILE(fd, indent + 2, "freeBuffersCount = %u", m_freeBufferList.NumNodes());
        CAMX_LOG_TO_FILE(fd, indent + 2, "busyBufferCount  = %u", m_busyBufferList.NumNodes());

        CAMX_LOG_TO_FILE(fd, indent + 2, "BusyBuffers");
        LightweightDoublyLinkedListNode* pNode = m_busyBufferList.Head();
        while (NULL != pNode)
        {
            static_cast<ImageBuffer*>(pNode->pData)->DumpState(fd, indent + 4);
            pNode = LightweightDoublyLinkedList::NextNode(pNode);
        }
        m_pLock->Unlock();
    }
    else
    {
        CAMX_LOG_TO_FILE(fd, indent + 2, "WARNING: Lock failed with status: %s. No Per Image Buffer Info",
                         CamxResultStrings[result]);
    }
}

CAMX_NAMESPACE_END
