////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxmetadatapool.cpp
/// @brief Implements camx metadata slot and pool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxatomic.h"
#include "camxhal3metadatautil.h"
#include "camxhwenvironment.h"
#include "camxmem.h"
#include "camxmetadatapool.h"
#include "camxtrace.h"
#include "camxvendortags.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Namespaces
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constant Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT32 MetaBufferCamUsageMask = 0x1<<30;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::MetadataPoolThreadCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* MetadataPool::MetadataPoolThreadCb(
    VOID* pArg)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pArg)
    {
        MetadataPool* pLocalInstance = static_cast<MetadataPool*>(pArg);

        result = pLocalInstance->Initialize();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "NULL argument in MetadataPoolThreadCb");
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataPool* MetadataPool::Create(
    PoolType        poolType,
    UINT            pipelineId,
    ThreadManager*  pThreadManager,
    UINT            numSlots,
    const CHAR*     pPipelineName,
    UINT            numPrePublishedTags)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupMeta, SCOPEEventMetadataPoolCreate);

    CamxResult      result          = CamxResultSuccess;
    MetadataPool*   pLocalInstance  = NULL;

    pLocalInstance =
        CAMX_NEW MetadataPool(poolType, pipelineId, pThreadManager, numSlots, pPipelineName, numPrePublishedTags);
    if (NULL != pLocalInstance)
    {
        if (NULL != pThreadManager)
        {
            result = pLocalInstance->m_pThreadManager->PostJob(pLocalInstance->m_hThread,
                                                               NULL,
                                                               reinterpret_cast<VOID**>(&pLocalInstance),
                                                               FALSE,
                                                               FALSE);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "[%s] PostJob failed.", pLocalInstance->GetPoolIdentifier());
                // PostJob failed. We are not parallelizing this pool creation.
                result = pLocalInstance->Initialize();
            }
        }
        else
        {
            // pThreadManager is NULL, we are not parallelizing this pool creation.
            result = pLocalInstance->Initialize();
        }

        if (CamxResultSuccess != result)
        {
            CAMX_DELETE pLocalInstance;
            pLocalInstance = NULL;
        }
    }

    return pLocalInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::Destroy()
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupMeta, SCOPEEventMetadataPoolDestroy);

    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::MetadataPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataPool::MetadataPool(
    PoolType        poolType,
    UINT            pipelineId,
    ThreadManager*  pThreadManager,
    UINT            numSlots,
    const CHAR*     pPipelineName,
    UINT            numPrePublishedTags)
    : m_poolType(poolType)
    , m_numSlots(numSlots)
    , m_pipelineId(pipelineId)
    , m_pipelineName(pPipelineName)
    , m_numPrePublishedTags(numPrePublishedTags)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(((PoolType::Static    == m_poolType)  && (1 == m_numSlots)) ||
                ((PoolType::PerUsecase == m_poolType) && (1 == m_numSlots)) ||
                (1 < m_numSlots));

    OsUtils::SNPrintF(m_poolIdentifier,
                      MaxTraceStringLength,
                      "Metadata Type: %s P:%p",
                      PoolTypeStrings[PoolTypeIndex()],
                      this);

    m_pThreadManager            = pThreadManager;
    m_pMetadataPoolCreateWait   = Condition::Create("MetadataPoolCreateWait");
    m_pMetadataPoolCreateLock   = Mutex::Create("MetadataPoolCreateLock");
    m_poolStatus                = PoolStatus::Uninitialized;

    if (NULL != m_pThreadManager)
    {
        result = m_pThreadManager->RegisterJobFamily(MetadataPoolThreadCb,
                                                     GetPoolIdentifier(),
                                                     NULL,
                                                     JobPriority::Normal,
                                                     TRUE,
                                                     &m_hThread);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMeta, "[%s] Register MetadaPoolThreadManager failed!",
                           GetPoolIdentifier());
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::~MetadataPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataPool::~MetadataPool()
{
    // Unregister jobs first, we can not handle any call during destroy
    if ((InvalidJobHandle != m_hThread) && (NULL != m_pThreadManager))
    {
        m_pThreadManager->UnregisterJobFamily(MetadataPoolThreadCb,
                                              GetPoolIdentifier(),
                                              m_hThread);
    }

    for (UINT32 slotIndex = 0; slotIndex < m_numSlots; ++slotIndex)
    {
        if (NULL != m_pSlots[slotIndex])
        {
            m_pSlots[slotIndex]->Destroy();
        }
    }

    for (UINT32 clientIndex = 0; clientIndex < MaxMetadataTags; clientIndex++)
    {
        m_pMetadataClients[clientIndex].clear();
    }

    if (NULL != m_pPoolLock)
    {
        m_pPoolLock->Destroy();
        m_pPoolLock = NULL;
    }

    if (NULL != m_pMetadataPoolCreateLock)
    {
        m_pMetadataPoolCreateLock->Destroy();
        m_pMetadataPoolCreateLock = NULL;
    }

    if (NULL != m_pMetadataPoolCreateWait)
    {
        m_pMetadataPoolCreateWait->Destroy();
        m_pMetadataPoolCreateWait = NULL;
    }

    if (NULL != m_pStickyMetaBuffer)
    {
        m_pStickyMetaBuffer->Destroy();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataPool::Initialize()
{
    CAMX_ENTRYEXIT_NAME(CamxLogGroupMeta, "MetadataPoolInit");

    CamxResult  result = CamxResultSuccess;

    // calculate size
    switch (m_poolType)
    {
        case PoolType::Static:
            HAL3MetadataUtil::CalculateSizeStaticMeta(&m_slotMetadataEntryCount, &m_slotMetadataDataSize,
                TagSectionVisibility::TagSectionVisibleToAll);
            break;

        case PoolType::PerFrameInternal:
            m_slotMetadataDataSize = sizeof(InternalPropertyBlob);
            break;

        case PoolType::PerFrameDebugData:
            m_slotMetadataDataSize = sizeof(DebugDataPropertyBlob);
            break;

        case PoolType::PerUsecase:
            // update the publish list
            for (UINT32 propertyID = PropertyIDUsecaseBegin; propertyID <= PropertyIDUsecaseEnd; ++propertyID)
            {
                m_publishTagSet.insert(propertyID);
            }
            break;

        default:
            break;
    }

    for (UINT32 slotIndex = 0; slotIndex < m_numSlots; ++slotIndex)
    {
        m_pSlots[slotIndex] = MetadataSlot::Create(m_slotMetadataEntryCount, m_slotMetadataDataSize, this);

        CAMX_ASSERT_MESSAGE(NULL != m_pSlots[slotIndex], "Failed to create slot %d", slotIndex);

        if (NULL == m_pSlots[slotIndex])
        {
            result = CamxResultEFailed;
            break;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (PoolType::PerFrameResult == m_poolType)
        {
            m_pStickyMetaBuffer = MetaBuffer::Create(NULL);
            if (NULL == m_pStickyMetaBuffer)
            {
                result = CamxResultENoMemory;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        m_pPoolLock = Mutex::Create(m_poolIdentifier);
        if (NULL == m_pPoolLock)
        {
            CAMX_ASSERT(NULL != m_pPoolLock);
            result = CamxResultENoMemory;
        }
    }

    m_pMetadataPoolCreateLock->Lock();

    if (CamxResultSuccess == result)
    {
        m_poolStatus = PoolStatus::Initialized;
        CAMX_LOG_INFO(CamxLogGroupMeta, "[%s] Metadata pool initialized.", GetPoolIdentifier());
    }
    else
    {
        m_poolStatus = PoolStatus::Error;
        CAMX_LOG_ERROR(CamxLogGroupMeta, "[%s] Metadata pool initialize failed.", GetPoolIdentifier());
    }

    m_pMetadataPoolCreateWait->Signal();

    m_pMetadataPoolCreateLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::Subscribe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataPool::Subscribe(
    const Subscription*     pSubscribe,
    UINT32                  tagCount,
    IPropertyPoolObserver*  pClient,
    const CHAR*             pClientName)
{
    CAMX_ASSERT((PoolType::PerFrameInput != m_poolType) && (PoolType::PerFrameDebugData != m_poolType));

    CamxResult  result  = CamxResultSuccess;

    if ((PoolType::PerFrameInput        == m_poolType)     ||
        (PoolType::PerFrameDebugData    == m_poolType))
    {
        result = CamxResultEUnsupported;
    }
    else if ((MaxMetadataTags < tagCount)   ||
        (NULL == pSubscribe)                ||
        (NULL == pClient))
    {
        result = CamxResultEInvalidArg;
    }
    else
    {
        for (UINT32 index = 0; index < tagCount; ++index)
        {
            UINT32 tagIndex = HAL3MetadataUtil::GetUniqueIndexByTag(pSubscribe[index].unitId);
            if (tagIndex < MaxMetadataTags)
            {
                m_pMetadataClients[tagIndex].push_back({ pClient, pClientName });
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "InvalidTag subscribed %d.", pSubscribe[index].unitId);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::SubscribeAll
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataPool::SubscribeAll(
    IPropertyPoolObserver*  pClient,
    const CHAR*             pClientName)
{
    CAMX_ASSERT((PoolType::PerFrameInput != m_poolType) && (PoolType::PerFrameDebugData != m_poolType));

    UINT client;

    for (client = 0; client < MaxSubscribers; client++)
    {
        if (NULL == m_subscribeAllClients[client].pClient)
        {
            m_subscribeAllClients[client] = { pClient, pClientName };
            break;
        }
    }

    return (MaxSubscribers > client) ? CamxResultSuccess : CamxResultENoMemory;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::UnsubscribeAll
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::UnsubscribeAll(
    IPropertyPoolObserver* pClient)
{
    CAMX_ASSERT((PoolType::PerFrameInput != m_poolType) && (PoolType::PerFrameDebugData != m_poolType));

    for (UINT client = 0; client < MaxSubscribers; client ++)
    {
        if (m_subscribeAllClients[client].pClient == pClient)
        {
            m_subscribeAllClients[client] = { NULL, NULL };
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::NotifyClient
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::NotifyClient(
    IPropertyPoolObserver* pObserver,
    UINT32                 unitId,
    UINT64                 slotRequestId,
    BOOL                   isSuccess)
{
    if (HAL3MetadataUtil::IsProperty(unitId))
    {
        if (TRUE == isSuccess)
        {
            pObserver->OnPropertyUpdate(unitId, slotRequestId, m_pipelineId);
        }
        else
        {
            pObserver->OnPropertyFailure(unitId, slotRequestId, m_pipelineId);
        }
    }
    else
    {
        if (TRUE == isSuccess)
        {
            pObserver->OnMetadataUpdate(unitId, slotRequestId, m_pipelineId);
        }
        else
        {
            pObserver->OnMetadataFailure(unitId, slotRequestId, m_pipelineId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::NotifyImmediate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::NotifyImmediate(
    UINT32      unitId,
    UINT32      tagIndex,
    UINT64      slotRequestId,
    BOOL        isSuccess)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);
    CAMX_ASSERT((PoolType::PerFrameInput != m_poolType) && (PoolType::PerFrameDebugData != m_poolType));

    /// update per entry clients
    for (auto clientIterator = m_pMetadataClients[tagIndex].begin();
         clientIterator != m_pMetadataClients[tagIndex].end();
         ++clientIterator)
    {
        CAMX_ASSERT(NULL != clientIterator->pClient);
        CAMX_ASSERT(NULL != clientIterator->pClientName);

        CAMX_TRACE_MESSAGE_F(CamxLogGroupMeta,
                             "Notifying %s of %s for requestId %llu, Metadata %08x",
                             clientIterator->pClientName,
                             (isSuccess == TRUE ? "Success" : "Failure"), slotRequestId, unitId);

        NotifyClient(clientIterator->pClient, unitId, slotRequestId, isSuccess);
    }

    /// update global clients
    for (UINT32 clientIndex = 0; clientIndex < MaxSubscribers; clientIndex++)
    {
        SubscriptionEntry* pEntry = &m_subscribeAllClients[clientIndex];
        if (NULL != pEntry->pClient)
        {
            CAMX_ASSERT(NULL != pEntry->pClientName);

            CAMX_TRACE_MESSAGE_F(CamxLogGroupMeta,
                                 "Notifying %s of %s for requestId %llu, Metadata %x",
                                 pEntry->pClientName,
                                 (isSuccess == TRUE ? "Success" : "Failure"), slotRequestId, unitId);

            CAMX_LOG_VERBOSE(CamxLogGroupMeta,
                             "Notifying %s of %s for requestId %llu, Metadata %x",
                             pEntry->pClientName,
                             (isSuccess == TRUE ? "Success" : "Failure"), slotRequestId, unitId);

            NotifyClient(pEntry->pClient, unitId, slotRequestId, isSuccess);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::Flush(
    UINT64 lastValidRequestId,
    UINT64 flushRequestId)
{
    CAMX_ASSERT(lastValidRequestId <= flushRequestId);

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "MetadataPool::Flush lastValidRequestId: %llu "
                                       "m_prevlastValidRequestId: %llu "
                                       "m_lastFlushRequestId: %llu ",
                                       lastValidRequestId,
                                       m_prevlastValidRequestId,
                                       m_lastFlushRequestId);
    m_pPoolLock->Lock();

    if ((0 == m_prevlastValidRequestId) ||
        (lastValidRequestId != m_prevlastValidRequestId))
    {
        MetaBuffer*   pMetaBuffer = NULL;
        MetadataSlot* pCurrentSlot = GetSlot(lastValidRequestId);

        if (PoolType::PerFrameResult == m_poolType)
        {
            pCurrentSlot->GetMetabuffer(&pMetaBuffer);

            m_pStickyMetaBuffer->Invalidate();

            pCurrentSlot->ReadLock();
            if (NULL != pMetaBuffer)
            {
                m_pStickyMetaBuffer->Copy(pMetaBuffer, FALSE, TRUE);
            }
            pCurrentSlot->Unlock();

            for (UINT32 slotIndex = (lastValidRequestId % m_numSlots); slotIndex <= (flushRequestId % m_numSlots); slotIndex++)
            {
                m_pSlots[slotIndex]->DetachMetabuffer(&pMetaBuffer);
                m_pSlots[slotIndex]->Invalidate();
            }
        }
        else if (PoolType::PerFrameInput == m_poolType)
        {
            for (UINT32 slotIndex = (lastValidRequestId % m_numSlots); slotIndex <= (flushRequestId % m_numSlots); slotIndex++)
            {
                m_pSlots[slotIndex]->DetachMetabuffer(&pMetaBuffer);
                m_pSlots[slotIndex]->Invalidate();
            }
        }
        m_prevlastValidRequestId = lastValidRequestId;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Did not update StickyMetadata");
    }

    m_lastFlushRequestId = flushRequestId;

    m_pPoolLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::Invalidate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::Invalidate(
    UINT64 requestId)
{
    // We only slide the window once all of the clients waiting on the current frame have been notified

    m_pPoolLock->Lock();

    if (PoolStatus::Initialized == m_poolStatus)
    {
        MetadataSlot* pCurrentSlot = GetSlot(requestId);
        pCurrentSlot->Invalidate();
    }

    m_pPoolLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::UpdateRequestId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::UpdateRequestId(
    UINT64 requestId)
{
    // We only update the next request ID when a process capture request is called into the HAL
    m_pPoolLock->Lock();

    GetSlot(requestId)->SetSlotRequestId(requestId);

    m_pPoolLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::LockMetadataSubscribers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::LockMetadataSubscribers(
    UINT32 tagIndex)
{
    for (auto clientIterator = m_pMetadataClients[tagIndex].begin();
        clientIterator != m_pMetadataClients[tagIndex].end(); ++clientIterator)
    {
        clientIterator->pClient->LockForPublish();
    }

    /// update global clients
    for (UINT32 clientIndex = 0; clientIndex < MaxSubscribers; clientIndex++)
    {
        SubscriptionEntry* pEntry = &m_subscribeAllClients[clientIndex];
        if (NULL != pEntry->pClient)
        {
            pEntry->pClient->LockForPublish();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::UnlockMetadataSubscribers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::UnlockMetadataSubscribers(
    UINT32 tagIndex)
{
    for (auto clientIterator = m_pMetadataClients[tagIndex].begin();
        clientIterator != m_pMetadataClients[tagIndex].end(); ++clientIterator)
    {
        clientIterator->pClient->UnlockAfterPublish();
    }

    /// update global clients
    for (UINT32 clientIndex = 0; clientIndex < MaxSubscribers; clientIndex++)
    {
        SubscriptionEntry* pEntry = &m_subscribeAllClients[clientIndex];
        if (NULL != pEntry->pClient)
        {
            pEntry->pClient->UnlockAfterPublish();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::Lock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::Lock() const
{
    m_pPoolLock->Lock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::Unlock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::Unlock() const
{
    m_pPoolLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::WaitForMetadataPoolCreation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataPool::WaitForMetadataPoolCreation()
{
    CamxResult  result = CamxResultSuccess;

    m_pMetadataPoolCreateLock->Lock();

    if (PoolStatus::Uninitialized == m_poolStatus)
    {
        result = m_pMetadataPoolCreateWait->TimedWait(m_pMetadataPoolCreateLock->GetNativeHandle(), TimeoutMilliseconds);
    }

    m_pMetadataPoolCreateLock->Unlock();

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Wait for metadata pool [%s] creation failed! This should never happen!",
                       GetPoolIdentifier());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataPool::UpdatePublishSet
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataPool::UpdatePublishSet(
    const std::unordered_set<UINT32>& pPublishList)
{
    CamxResult result = CamxResultSuccess;
    if (PoolType::PerFrameResult == GetPoolType())
    {
        std::unordered_set<UINT32>::const_iterator setIterator;
        for (setIterator = pPublishList.cbegin(); setIterator != pPublishList.cend(); ++setIterator)
        {
            m_publishTagSet.insert(*setIterator);
        }
    }
    else
    {
        result = CamxResultEUnsupported;
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Update publist set is only allowed for result pools",
                       GetPoolIdentifier());
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataSlot* MetadataSlot::Create(
    SIZE_T        entryCapacity,
    SIZE_T        dataSize,
    MetadataPool* pPool)
{
    CamxResult      result          = CamxResultEFailed;
    MetadataSlot*   pLocalInstance  = NULL;

    pLocalInstance = CAMX_NEW MetadataSlot(entryCapacity, dataSize, pPool);

    if (NULL != pLocalInstance)
    {
        result = pLocalInstance->Initialize();

        if (CamxResultSuccess != result)
        {
            CAMX_DELETE pLocalInstance;
            pLocalInstance = NULL;
        }
    }

    return pLocalInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataSlot::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::MetadataSlot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataSlot::MetadataSlot(
    SIZE_T        entryCapacity,
    SIZE_T        dataSize,
    MetadataPool* pPool)
    : m_slotRequestId(0)
    , m_dataSize(dataSize)
    , m_entryCapacity(entryCapacity)
    , m_pMetadata(NULL)
    , m_pPool(pPool)
    , m_pRWLock(NULL)
    , m_pMetaBuffer(NULL)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::~MetadataSlot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataSlot::~MetadataSlot()
{
    if (NULL != m_pMetadata)
    {
        switch (m_pPool->GetPoolType())
        {
            case PoolType::Static:
                HAL3MetadataUtil::FreeMetadata(m_pMetadata);
                break;

            case PoolType::PerFrameInternal:
            case PoolType::PerFrameDebugData:
                CAMX_FREE(m_pMetadata);
                break;

            default:
                break;
        }

        m_pMetadata = NULL;
    }

    if ((PoolType::PerUsecase == m_pPool->GetPoolType()) && (NULL != m_pMetaBuffer))
    {
        m_pMetaBuffer->Destroy();
        m_pMetaBuffer = NULL;
    }

    if (NULL != m_pRWLock)
    {
        m_pRWLock->Destroy();
        m_pRWLock = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::InitializeAndroidMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::InitializeAndroidMetadata()
{
    CamxResult result = CamxResultSuccess;

    m_pMetadata = HAL3MetadataUtil::CreateMetadata(m_entryCapacity, m_dataSize);

    if (NULL == m_pMetadata)
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Couldn't allocate memory for metadata slot because of invalid pool type: ",
                       static_cast<UINT>(m_pPool->GetPoolType()));

        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::InitializeInternalMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::InitializeInternalMetadata()
{
    CamxResult result = CamxResultSuccess;

    m_pMetadata = CAMX_CALLOC(m_dataSize);

    if (NULL == m_pMetadata)
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Couldn't allocate memory for metadata slot for pool %d ",
                       static_cast<UINT>(m_pPool->GetPoolType()));

        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::InitializeMetaBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::InitializeMetaBuffers()
{
    CamxResult result = CamxResultSuccess;
    m_pMetaBuffer = MetaBuffer::Create(NULL);
    if (NULL != m_pMetaBuffer)
    {
        const unordered_set<UINT32>& publishSet = m_pPool->GetPublishSet();
        if (!publishSet.empty())
        {
            result = m_pMetaBuffer->AllocateBuffer(publishSet);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Couldn't allocate memory for metadata slot because of invalid pool type: ",
                       static_cast<UINT>(m_pPool->GetPoolType()));

        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::Initialize()
{
    CamxResult result   = CamxResultSuccess;
    PoolType   poolType = m_pPool->GetPoolType();

    switch (poolType)
    {
        case PoolType::Static:
            result = InitializeAndroidMetadata();
            if (CamxResultSuccess == result)
            {
                m_isValid = TRUE;
            }
            break;

        case PoolType::PerUsecase:
            result = InitializeMetaBuffers();
            if (CamxResultSuccess == result)
            {
                m_isValid = TRUE;
                ResetPublishList();
            }
            break;

        case PoolType::PerFrameInternal:
        case PoolType::PerFrameDebugData:
            result = InitializeInternalMetadata();
            break;

        default:
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_pRWLock = ReadWriteLock::Create(m_pPool->GetPoolIdentifier());
        if (NULL == m_pRWLock)
        {
            result = CamxResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::AttachMetabuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::AttachMetabuffer(
    MetaBuffer* pMetabuffer)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pMetabuffer);
    CAMX_ASSERT(NULL == m_pMetaBuffer);

    if (NULL != pMetabuffer)
    {
        m_pMetaBuffer = pMetabuffer;
        m_pMetaBuffer->AddReference(MetaBufferCamUsageMask | static_cast<UINT32>(m_slotRequestId), FALSE);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::GetMetabuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::GetMetabuffer(
    MetaBuffer** ppMetabuffer)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != ppMetabuffer);

    if (m_pPool->UseMetaBuffers() && (NULL != m_pMetaBuffer))
    {
        *ppMetabuffer = m_pMetaBuffer;
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupMeta, "Could not get metabuffer");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::DetachMetabuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::DetachMetabuffer(
    MetaBuffer** ppMetabuffer)
{
    m_pRWLock->WriteLock();
    CamxResult result = CamxResultSuccess;

    if (NULL != m_pMetaBuffer)
    {
        *ppMetabuffer = m_pMetaBuffer;
        m_pMetaBuffer->ReleaseReference(MetaBufferCamUsageMask | static_cast<UINT32>(m_slotRequestId), FALSE);
        m_pMetaBuffer = NULL;
    }
    else
    {
        *ppMetabuffer = NULL;
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "Metabuffer already detached");
        result = CamxResultENoSuch;
    }
    m_pRWLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::GetMetadataByTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* MetadataSlot::GetMetadataByTag(
    UINT32      tag,
    const CHAR* pName)
{
    VOID* pData      = NULL;

    CAMX_ENTRYEXIT(CamxLogGroupMeta);

    const CHAR*         pTagName      = "";
    UINT32              size          = 0;
    BOOL                isPublished   = FALSE;

    if ((TRUE == IsSlotValid()) && (TRUE == m_pPool->UseMetaBuffers()))
    {
        const MetadataInfo* pMetadataInfo = HAL3MetadataUtil::GetMetadataInfoByTag(tag);
        CAMX_ASSERT(NULL != pMetadataInfo);

        pTagName    = pMetadataInfo->tagName;
        size        = pMetadataInfo->size;
        isPublished = IsMetadataPublishedByTagIndex(pMetadataInfo->index);

        if (NULL != m_pMetaBuffer)
        {
            pData = m_pMetaBuffer->GetTag(tag, TRUE);
        }

        if (NULL == pData)
        {
            if ((TRUE == GetTagFromStickyMeta()) && (TRUE == isPublished))
            {
                pData = m_pPool->m_pStickyMetaBuffer->GetTag(tag, TRUE);
            }
            else if (PoolType::PerFrameInput == m_pPool->GetPoolType() && (NULL != m_pMetaBuffer))
            {
                /// @todo (CAMX-4175) Remove memset input pool data
                UINT memsetInputMeta = HwEnvironment::GetInstance()->GetStaticSettings()->memsetInputMeta;

                if ((0 < memsetInputMeta) &&
                    ((size <= MetaBuffer::MaxInplaceTagSize) || (1 < memsetInputMeta) ||
                     (TRUE == HAL3MetadataUtil::IsProperty(tag))))
                {
                    VOID* pTagPayload = CAMX_CALLOC(size);

                    if (NULL != pTagPayload)
                    {
                        m_pMetaBuffer->SetTag(tag, pTagPayload, pMetadataInfo->count, size);

                        pData = m_pMetaBuffer->GetTag(tag, TRUE);

                        CAMX_FREE(pTagPayload);
                    }
                }
            }
        }
    }
    else if (!m_pPool->UseMetaBuffers() && (NULL != m_pMetadata))
    {
        isPublished = TRUE;
        HAL3MetadataUtil::GetMetadata(m_pMetadata, tag, &pData);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta,
                         "Invalid Slot/Metadata to get a metadata from pool %d tag %x reqId %llu name %s client %s",
                         m_pPool->GetPoolType(), tag, m_slotRequestId,
                         pTagName, pName);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMeta,
        "data %p pool %d tagID %x published %d reqId %llu name %s client Node::%s using memset data %d %p pipeline %s",
        pData, m_pPool->GetPoolType(), tag, isPublished, m_slotRequestId, pTagName, pName,
        HwEnvironment::GetInstance()->GetStaticSettings()->memsetInputMeta,
        m_pMetaBuffer, m_pPool->m_pipelineName);

    return pData;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::GetMetadataByCameraId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* MetadataSlot::GetMetadataByCameraId(
    UINT32      tag,
    UINT32      cameraId,
    const CHAR* pName)
{
    VOID* pData      = NULL;

    CAMX_ENTRYEXIT(CamxLogGroupMeta);

    const CHAR*         pTagName      = "";
    UINT32              size          = 0;
    BOOL                isPublished   = FALSE;

    if ((TRUE == m_isValid) &&
        ((PoolType::PerFrameInput == m_pPool->m_poolType) ||
        (PoolType::PerFrameResult == m_pPool->m_poolType)))
    {
        const MetadataInfo* pMetadataInfo = HAL3MetadataUtil::GetMetadataInfoByTag(tag);
        CAMX_ASSERT(NULL != pMetadataInfo);

        pTagName    = pMetadataInfo->tagName;
        size        = pMetadataInfo->size;
        isPublished = IsMetadataPublishedByTagIndex(pMetadataInfo->index);

        if (NULL != m_pMetaBuffer)
        {
            pData = m_pMetaBuffer->GetTagByCameraId(tag, cameraId, TRUE);
        }

        if ((NULL == pData) && (TRUE == GetTagFromStickyMeta()) && (TRUE == isPublished))
        {
            pData = m_pPool->m_pStickyMetaBuffer->GetTag(tag, TRUE);
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta,
                         "Invalid Slot to get a metadata from pool %d tag %x camId %d reqId %llu name %s client %s",
                         m_pPool->GetPoolType(), tag, cameraId, m_slotRequestId, pTagName, pName);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMeta,
        "data %p pool %d tagID %x cameraId %d published %d reqId %llu name %s client Node::%s memset %d %p pipeline %s",
        pData, m_pPool->GetPoolType(), tag, cameraId, isPublished, m_slotRequestId, pTagName, pName,
        HwEnvironment::GetInstance()->GetStaticSettings()->memsetInputMeta,
        m_pMetaBuffer, m_pPool->m_pipelineName);

    return pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::GetMetadataCountByTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T MetadataSlot::GetMetadataCountByTag(
    UINT32  tag,
    BOOL    allowSticky)
{
    CAMX_UNREFERENCED_PARAM(allowSticky);
    CAMX_ENTRYEXIT(CamxLogGroupMeta);
    UINT32 count = 0;

    const MetadataInfo* pMetadataInfo = HAL3MetadataUtil::GetMetadataInfoByTag(tag);

    CAMX_ASSERT((!m_pPool->UseMetaBuffers() && (NULL != m_pMetadata)) ||
        (m_pPool->UseMetaBuffers() && (NULL != m_pMetaBuffer)) ||
        (TRUE == IsSlotValid()));


    if ((TRUE == m_isValid) && (PoolType::PerFrameInternal != m_pPool->GetPoolType()))
    {
        if (!m_pPool->UseMetaBuffers())
        {
            // It is expected the caller has already taken the lock by calling ReadLock() before calling this function
            count = static_cast<UINT32>(HAL3MetadataUtil::GetMetadataCount(m_pMetadata, tag));
            // It is expected that the caller will release the lock by calling Unlock() after this function returns
        }
        else
        {
            const VOID* pTagData = NULL;
            if ((PoolType::PerFrameInput == m_pPool->GetPoolType()) ||
                IsMetadataPublishedByTagIndex(pMetadataInfo->index))
            {
                pTagData = m_pMetaBuffer->GetTag(tag, count, TRUE);
            }

            CAMX_LOG_VERBOSE(CamxLogGroupMeta,
                             "%u count %p for data - pool %d tagID %x reqId %llu name %s",
                             count, pTagData, m_pPool->GetPoolType(), tag, m_slotRequestId,
                             pMetadataInfo->tagName);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid Slot to get a metadata from");
    }

    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::SetMetadataByTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::SetMetadataByTag(
    UINT32      tag,
    const VOID* pData,
    SIZE_T      count,
    const CHAR* pName)
{
    CamxResult          result        = CamxResultSuccess;
    const CHAR*         pTagName      = "";

    if ((NULL != pData)                     &&
        (0 < count)                         &&
        (TRUE == m_pPool->UseMetaBuffers()) &&
        (PoolType::PerFrameInput != m_pPool->GetPoolType()))
    {
        UINT32 tagType;
        UINT32 tagSize;
        UINT32 tagUnitSize;

        const MetadataInfo* pMetadataInfo = HAL3MetadataUtil::GetMetadataInfoByTag(tag);
        CAMX_ASSERT( NULL != pMetadataInfo );

        UINT32 maxTagSize = pMetadataInfo->size;
        UINT32 tagCount   = static_cast<UINT32>(count);

        pTagName = pMetadataInfo ? pMetadataInfo->tagName : "";

        if (HAL3MetadataUtil::IsProperty(tag))
        {
            tagType  = TYPE_BYTE;
            tagCount = maxTagSize;
        }
        else
        {
            tagType = pMetadataInfo->type;
        }

        tagUnitSize = TagSizeByType[tagType];
        tagSize     = tagUnitSize * tagCount;

        // try auto correction
        if ((tagSize > maxTagSize) && (tagCount == maxTagSize))
        {
            tagCount = 1;
            tagSize  = tagUnitSize;
            CAMX_LOG_VERBOSE(CamxLogGroupMeta,
                             "Invalid count set for tag %x type %d maxSize %u unitsize %u count %u pool %d"
                             " tagName %s client %s pipeline %s",
                             tag, tagType, maxTagSize, tagUnitSize, count, m_pPool->GetPoolType(),
                             pTagName, pName, m_pPool->m_pipelineName);
        }

        if (tagSize <= maxTagSize)
        {
            MetaBuffer* pMetaBuffer = (NULL != m_pMetaBuffer) ? m_pMetaBuffer : m_pPool->m_pStickyMetaBuffer;
            if (NULL != pMetaBuffer)
            {
                result = pMetaBuffer->SetTag(tag, pData, tagCount, tagUnitSize * tagCount, TRUE, pMetadataInfo);
            }

            if (CamxResultSuccess == result)
            {
                m_metadataPublishCount[pMetadataInfo->index] = TRUE;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta,
                               "Cannot set tag %x type %d maxSize %u unitsize %u count %u pool %d"
                               " tagName %s client %s buffer %p pipeline %s",
                               tag, tagType, maxTagSize, tagUnitSize, count, m_pPool->GetPoolType(),
                               pTagName, pName, pMetaBuffer, m_pPool->m_pipelineName);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMeta,
                           "Invalid size for tag %x type %d maxSize %u unitsize %u count %u"
                           " pool %d tagName %s client %s pipeline %s",
                           tag, tagType, maxTagSize, tagUnitSize, count, m_pPool->GetPoolType(),
                           pTagName, pName, m_pPool->m_pipelineName);
        }
    }
    else if ((NULL == pData) || (0 == count))
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Invalid input %p count %d pooltype %d tagName %s client %s reqId %llu pipeline %s",
                       pData, count, m_pPool->GetPoolType(),
                       pTagName, pName, m_slotRequestId,
                       m_pPool->m_pipelineName);

        result = CamxResultEInvalidArg;
    }
    else if (FALSE == IsSlotValid())
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Invalid slot to set for tag %x reqId %llu pool %d tagName %s client %s pipeline %s",
                       tag, m_slotRequestId, m_pPool->GetPoolType(), pTagName, pName, m_pPool->m_pipelineName);

        result = CamxResultEInvalidState;
    }
    else if (PoolType::PerFrameInput == m_pPool->GetPoolType())
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Invalid reqId %llu pool %d cannot set metadata tag %x tagName %s client %s pipeline %s",
                       m_slotRequestId, m_pPool->GetPoolType(), tag, pTagName, pName, m_pPool->m_pipelineName);

        result = CamxResultEUnsupported;
    }
    else
    {
        m_pRWLock->WriteLock();
        if (!HAL3MetadataUtil::IsProperty(tag))
        {
            result = HAL3MetadataUtil::UpdateMetadata(m_pMetadata, tag, pData, count, true);
        }
        else
        {
            result = CamxResultEInvalidArg;

            CAMX_LOG_ERROR(CamxLogGroupMeta,
                           "Cannot update property pool %d cannot set metadata tag %x tagName %s client %s pipeline %s",
                           m_pPool->GetPoolType(), tag, pTagName, pName, m_pPool->m_pipelineName);
        }
        m_pRWLock->Unlock();
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMeta,
                     "Setting metadata %x data %p count %d pooltype %d tagName %s client %s reqId %llu pipeline %s",
                     tag, pData, count, m_pPool->GetPoolType(), pTagName, pName, m_slotRequestId,
                     m_pPool->m_pipelineName);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::GetPropertyBlob
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::GetPropertyBlob(
    VOID** ppBlob)
{
    CamxResult result   = CamxResultSuccess;
    PoolType   poolType = m_pPool->GetPoolType();

    if (NULL == ppBlob)
    {
        result = CamxResultEInvalidArg;
    }
    else if (NULL == m_pMetadata)
    {
        result = CamxResultEUnsupported;

        CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid pooltype %d", poolType);
    }
    else
    {
        *ppBlob = m_pMetadata;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::PublishMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::PublishMetadata(
    UINT32      tag,
    const CHAR* pClientName)
{
    CAMX_ASSERT(PoolType::PerFrameInput != m_pPool->GetPoolType());

    CamxResult  result      = CamxResultSuccess;

    const MetadataInfo* pMetadataInfo = HAL3MetadataUtil::GetMetadataInfoByTag(tag);

    UINT32 tagIndex = (NULL != pMetadataInfo) ? pMetadataInfo->index : MaxMetadataTags;

    if (MaxMetadataTags > tagIndex)
    {
        m_metadataPublishCount[tagIndex] = TRUE;

    //  CAMX_LOG_VERBOSE(CamxLogGroupMeta,
    //                 "Publish tag %x tagName %s reqId %llu pool %u client %s",
    //                 tag, pMetadataInfo ? pMetadataInfo->tagName : "",
    //                 m_slotRequestId, m_pPool->GetPoolType(), pClientName);

        m_pPool->LockMetadataSubscribers(tagIndex);
        m_pPool->NotifyImmediate(tag, tagIndex, m_slotRequestId, TRUE);
        m_pPool->UnlockMetadataSubscribers(tagIndex);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Invalid tag %x to publish pool %d client %s",
                       tag, m_pPool->GetPoolType(), pClientName);

        result = CamxResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::GetMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Metadata* MetadataSlot::GetMetadata()
{
    return m_pMetadata;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::DumpMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataSlot::DumpMetadata()
{
    if (NULL != m_pMetaBuffer)
    {
        m_pMetaBuffer->PrintDetails();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::PublishMetadataList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MetadataSlot::PublishMetadataList(
    UINT32*     pTagList,
    UINT32      numTags,
    const CHAR* pClientName)
{
    CamxResult resultFinal = CamxResultSuccess;

    CAMX_ASSERT(PoolType::PerFrameInput     != m_pPool->GetPoolType());
    CAMX_ASSERT(PoolType::PerFrameInternal  != m_pPool->GetPoolType());
    CAMX_ASSERT(PoolType::Static            != m_pPool->GetPoolType());
    CAMX_ASSERT(NULL != pTagList);
    CAMX_ASSERT(0 < numTags);

    if (m_pPool->UseMetaBuffers())
    {
        for (UINT32 index = 0; index < numTags; index++)
        {
            CamxResult resultLocal = PublishMetadata(pTagList[index], pClientName);

            // We don't abort if a single publish failed
            if (CamxResultSuccess != resultLocal)
            {
                resultFinal = resultLocal;
            }
        }
    }
    else if (FALSE == m_isValid)
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Invalid slot, cannot publish metadata list pool %d reqId %d name %s",
                       m_pPool->GetPoolType(), m_slotRequestId, pClientName);

        resultFinal = CamxResultEInvalidState;
    }
    else if (NULL == pTagList)
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Invalid metadata, cannot publish metadata list pool %d reqId %d name %s",
                       m_pPool->GetPoolType(), m_slotRequestId, pClientName);

        resultFinal = CamxResultEInvalidArg;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta,
                       "Invalid pool, cannot publish metadata list pool %d reqId %d name %s",
                       m_pPool->GetPoolType(), m_slotRequestId, pClientName);

        resultFinal = CamxResultEInvalidState;
    }

    return resultFinal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::Invalidate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataSlot::Invalidate()
{
    m_pRWLock->WriteLock();

    m_slotRequestId  = 0;
    m_isValid        = FALSE;

    m_pRWLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::MergeMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataSlot::MergeMetadata(
    const Metadata* pSrcMetadata)
{
    if (m_pMetadata == NULL)
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "m_pMetadata == NULL");
    }
    else if (pSrcMetadata == NULL)
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "pSrcMetadata == NULL");
    }
    else if (m_pPool->UseMetaBuffers())
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot merged invalid pool type %d", static_cast<INT32>(m_pPool->GetPoolType() ));
    }
    else
    {
        HAL3MetadataUtil::MergeMetadata(m_pMetadata, pSrcMetadata);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::IsMetadataPublishedByTagIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MetadataSlot::IsMetadataPublishedByTagIndex(
    UINT32  tagIndex)
{
    return m_metadataPublishCount[tagIndex];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::IsPublished
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MetadataSlot::IsPublished(
    UnitType    type,
    UINT32      unitId)
{
    BOOL     published = FALSE;
    PoolType poolType  = m_pPool->GetPoolType();

    if ((PoolType::PerFrameInternal  == poolType) ||
        (PoolType::PerFrameResult    == poolType) ||
        (PoolType::PerUsecase        == poolType))
    {
        published = IsMetadataPublished(unitId);
    }
    else if (PoolType::PerFrameInput == poolType)
    {
        published = TRUE;
    }
    else if (FALSE == m_isValid)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta,
                         "Slot not yet initialized, slotPtr = %08x, requestID = %llu, pool = %d, type = %d, id = %08x",
                         this, poolType, m_slotRequestId, type, unitId);

        published = FALSE;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta,
                         "Invalid pool type requestID = %llu, pool = %d, type = %d, id = %08x",
                         m_pPool->GetPoolType(), m_slotRequestId, type, unitId);
    }

    return published;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::ResetPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataSlot::ResetPublishList()
{
    for (UINT32 index = 0; index < m_metadataPublishCount.size(); ++index)
    {
        m_metadataPublishCount[index] = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::SetSlotRequestId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataSlot::SetSlotRequestId(
    UINT64 requestId)
{
    if ((PoolType::Static != m_pPool->GetPoolType()) && (PoolType::PerUsecase != m_pPool->GetPoolType()))
    {
        CAMX_ASSERT_MESSAGE((0 != requestId), "ERROR pooltype %d id %d", m_pPool->GetPoolType(), requestId);
    }

    m_slotRequestId = requestId;
    m_isValid       = TRUE;

    ResetPublishList();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::GetSlotRequestId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 MetadataSlot::GetSlotRequestId() const
{
    CAMX_ASSERT_MESSAGE((TRUE == m_isValid), "Invalid Slot");

    return m_slotRequestId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::ReadLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataSlot::ReadLock() const
{
    m_pRWLock->ReadLock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::WriteLock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataSlot::WriteLock() const
{
    m_pRWLock->WriteLock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MetadataSlot::Unlock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataSlot::Unlock() const
{
    m_pRWLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MetadataSlot::PrintTagData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MetadataSlot::PrintTagData(
    UINT32      tag)
{
    if (!m_pPool->UseMetaBuffers())
    {
        HAL3MetadataUtil::PrintTagData(m_pMetadata, tag);
    }
    else
    {
        CAMX_ASSERT(NULL != m_pMetaBuffer);
    }
}


CAMX_NAMESPACE_END
