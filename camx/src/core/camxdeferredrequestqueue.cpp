////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxdeferredrequestqueue.cpp
/// @brief Deferred request queue implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdeferredrequestqueue.h"
#include "camxhal3metadatautil.h"
#include "camxincs.h"
#include "camxmem.h"
#include "camxnode.h"
#include "camxthreadmanager.h"
#include "camxsettingsmanager.h"

CAMX_NAMESPACE_BEGIN

/// @todo (CAMX-1797) Not sure if we need anything fancy, quickly rethink
UINT DeferredRequestQueue::s_numInstances = 0;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PipelineRequest
/// a pair of request id and pipeline id to keep track of where the request errord out
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PipelineRequest
{
    UINT   pipelineId;  ///< The pipeline id for this pair
    UINT64 requestId;   ///< The request id for the pair.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DeferredRequestQueue* DeferredRequestQueue::Create(
    DeferredRequestQueueCreateData* pCreateData)
{
    CamxResult              result                  = CamxResultEFailed;
    DeferredRequestQueue*   pDeferredRequestQueue   = NULL;

    pDeferredRequestQueue = CAMX_NEW DeferredRequestQueue();

    if (NULL != pDeferredRequestQueue)
    {
        result = pDeferredRequestQueue->Initialize(pCreateData);

        if (CamxResultSuccess != result)
        {
            CAMX_DELETE pDeferredRequestQueue;
            pDeferredRequestQueue = NULL;
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Out of memory");
        result = CamxResultENoMemory;
    }

    return pDeferredRequestQueue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::DeferredRequestQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DeferredRequestQueue::DeferredRequestQueue()
    : m_hDeferredWorker(InvalidJobHandle)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::~DeferredRequestQueue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DeferredRequestQueue::~DeferredRequestQueue()
{
    if (NULL != m_pThreadManager)
    {
        if (InvalidJobHandle != m_hDeferredWorker)
        {
            CHAR wrapperName[FILENAME_MAX];
            OsUtils::SNPrintF(&wrapperName[0], sizeof(wrapperName), "DeferredWorkerWrapper%p", m_pSession);
            m_pThreadManager->UnregisterJobFamily(DeferredWorkerWrapper, wrapperName, m_hDeferredWorker);
        }
    }

    // Empty hashmap
    if (NULL != m_pDependencyMap)
    {
        m_pDependencyMap->Foreach(FreeDependencyMapListData, TRUE);
        m_pDependencyMap->Destroy();
        m_pDependencyMap = NULL;
    }

    if (NULL != m_pFenceRequestMap)
    {
        m_pFenceRequestMap->Destroy();
        m_pFenceRequestMap = NULL;
    }

    // Free all remaining entries in m_errorRequests, referenced data are only numbers, not pointers
    m_pDeferredQueueLock->Lock();
    LightweightDoublyLinkedListNode* pNode = m_errorRequests.Head();
    while (NULL != pNode)
    {
        LightweightDoublyLinkedListNode* pNext = LightweightDoublyLinkedList::NextNode(pNode);
        m_errorRequests.RemoveNode(pNode);
        if (NULL != pNode->pData)
        {
            CAMX_FREE(pNode->pData);
            pNode->pData = NULL;
        }
        CAMX_FREE(pNode);

        pNode = pNext;
    }

    // Free all remaining entries in m_deferredNodes, and the data they reference
    m_deferredNodes.FreeAllNodesAndTheirClientData();
    m_pDeferredQueueLock->Unlock();

    // Free all the entries in ready queue
    m_pReadyQueueLock->Lock();
    m_readyNodes.FreeAllNodesAndTheirClientData();
    m_pReadyQueueLock->Unlock();

    if (NULL != m_pDeferredQueueLock)
    {
        m_pDeferredQueueLock->Destroy();
        m_pDeferredQueueLock = NULL;
    }

    if (NULL != m_pReadyQueueLock)
    {
        m_pReadyQueueLock->Destroy();
        m_pReadyQueueLock = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::FreeDependencyMapListData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::FreeDependencyMapListData(
    VOID* pData)
{
    if ((NULL != pData) && (NULL != *static_cast<VOID**>(pData)))
    {
        LightweightDoublyLinkedList* pList = *static_cast<LightweightDoublyLinkedList**>(pData);

        if (NULL != pList)
        {
            // Empty list
            LightweightDoublyLinkedListNode* pNode = pList->Head();

            while (NULL != pNode)
            {
                LightweightDoublyLinkedListNode* pNext = LightweightDoublyLinkedList::NextNode(pNode);
                pList->RemoveNode(pNode);
                CAMX_FREE(pNode);
                pNode = pNext;
            }

            CAMX_DELETE pList;
            pList = NULL;
        }

        *static_cast<VOID**>(pData) = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DeferredRequestQueue::Initialize(
    DeferredRequestQueueCreateData* pCreateData)
{
    CamxResult result = CamxResultEFailed;
    CHAR       wrapperName[FILENAME_MAX];

    m_logEnabled     = HwEnvironment::GetInstance()->GetStaticSettings()->logDRQEnable;
    m_pThreadManager = pCreateData->pThreadManager;
    m_pSession       = pCreateData->pSession;
    m_numPipelines   = pCreateData->numPipelines;

    for (UINT32 i = 0; i < m_numPipelines; i++)
    {
        m_pMainPools[i]     = pCreateData->pMainPools[i];
    }

    CAMX_ASSERT(NULL != m_pThreadManager);

    OsUtils::SNPrintF(&wrapperName[0], sizeof(wrapperName), "DeferredWorkerWrapper%p", m_pSession);

    result = m_pThreadManager->RegisterJobFamily(DeferredWorkerWrapper,
                                                 wrapperName,
                                                 NULL,
                                                 JobPriority::Normal,
                                                 FALSE,
                                                 &m_hDeferredWorker);

    if (CamxResultSuccess == result)
    {
        HashmapParams   hashMapParams   = { 0 };

        hashMapParams.keySize       = sizeof(DependencyKey);
        hashMapParams.valSize       = sizeof(LightweightDoublyLinkedList*);
        hashMapParams.maxNumBuckets = MaxNodeType * pCreateData->requestQueueDepth;
        hashMapParams.multiMap      = 0;
        m_pDependencyMap            = Hashmap::Create(&hashMapParams);

        if (NULL == m_pDependencyMap)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Out of memory");
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        HashmapParams   hashMapParams   = { 0 };

        hashMapParams.keySize       = sizeof(UINT64);
        hashMapParams.valSize       = sizeof(LightweightDoublyLinkedList*);
        hashMapParams.maxNumBuckets = MaxNodeType * pCreateData->requestQueueDepth;
        hashMapParams.multiMap      = 0;
        m_pFenceRequestMap          = Hashmap::Create(&hashMapParams);

        if (NULL == m_pFenceRequestMap)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Out of memory");
            result = CamxResultENoMemory;
        }
    }

    for (UINT32 i = 0; i < m_numPipelines; i++)
    {
        if ((CamxResultSuccess == result) && (NULL != m_pMainPools[i]))
        {
            ///@ todo (CAMX-325) - We are going with subscribe all model for now.
            // If there is a performance bottleneck found, we will revisit selective, runtime subscription again
            result = m_pMainPools[i]->SubscribeAll(static_cast<IPropertyPoolObserver*>(this), "DeferredRequestQueue");
        }
    }

    if (CamxResultSuccess == result)
    {
        m_preemptDependency.isPreemptDependencyEnabled = FALSE;
        m_preemptDependency.pipelineDepenency          = FALSE;

        CamxAtomicStoreU(&m_numErrorRequests, 0);

        m_pDeferredQueueLock = Mutex::Create("DeferredRequestQueue");
        m_pReadyQueueLock    = Mutex::Create("DRQReadyQueue");

        if ((NULL == m_pDeferredQueueLock) || (NULL == m_pReadyQueueLock))
        {
            result = CamxResultENoMemory;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::DeferredWorkerCore
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DeferredRequestQueue::DeferredWorkerCore(
    Dependency* pDependency)
{
    CAMX_ASSERT(pDependency->pNode != NULL);

    CamxResult             result         = CamxResultSuccess;
    NodeProcessRequestData processRequest = { 0 };
    Node*                  pNode          = pDependency->pNode;

    processRequest.processSequenceId    = pDependency->processSequenceId;
    processRequest.bindIOBuffers        = pDependency->bindIOBuffers;
    processRequest.isSequenceIdInternal = pDependency->isInternalDependency;

    if (NULL != pNode)
    {
        CAMX_LOG_DRQ("DRQ dispatching node=%d Node::%s, request=%llu, seqId=%d, bindIOBuffers=%d",
                     pNode->Type(),
                     pNode->NodeIdentifierString(),
                     pDependency->requestId,
                     pDependency->processSequenceId,
                     pDependency->bindIOBuffers);

        result = pNode->ProcessRequest(&processRequest, pDependency->requestId);

        CAMX_LOG_DRQ("DRQ execute complete node=%d Node::%s, request=%llu, seqId=%d",
                     pNode->Type(),
                     pNode->NodeIdentifierString(),
                     pDependency->requestId,
                     pDependency->processSequenceId);

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT_MESSAGE(processRequest.numDependencyLists <= MaxDependencies,
                                "Number of Dependency: %d is greater Max: %d",
                                processRequest.numDependencyLists, MaxDependencies);

            for (UINT index = 0; index < processRequest.numDependencyLists; index++)
            {
                DependencyUnit* pDependencyInfo = &processRequest.dependencyInfo[index];

                // Nodes should have an actual dependency if they report a dependency count. Any that doesnt may have messed up
                // filling in the dependencies. e.g. set dependencies in index 1, expecting dependencies in index 0, but only
                // incrementing numDependencyLists by one, for which adding the deferred node would skip the actual dependencies
                // Loop below checks that nodes didnt set dependencies above the count as well
                // Disabling this assert by default as nodes are now allowed to report a dependency without setting a mask bit
                // CAMX_ASSERT_MESSAGE(TRUE == Node::HasAnyDependency(pDependencyInfo),
                //                    "Node %s reported dependency %u without actual dependency", pNode->Name(), index);

                result = AddDeferredNode(pDependency->requestId, pNode, pDependencyInfo);
            }

#if ASSERTS_ENABLED
            for (UINT index = processRequest.numDependencyLists; index < MaxDependencies; index++)
            {
                // Any dependencies set in entries outside of numDependencyLists will not be considered, will be dropped, and
                // processing expected to occurred will not
                CAMX_ASSERT(FALSE == Node::HasAnyDependency(pDependencyInfo));
            }
#endif // ASSERTS_ENABLED
        }
        else if (CamxResultECancelledRequest == result)
        {
            // returning success as request is intended to be dropped for flush call
            result = CamxResultSuccess;
        }
    }
    else
    {
        CAMX_LOG_PERF_INFO(CamxLogGroupCore, "DRQ dispatching Chi fence callback");

        // suport only one chi fence for now
        CAMX_ASSERT(1 == pDependency->chiFenceCount);

        pDependency->pChiFenceCallback(pDependency->pChiFences[0]->hChiFence, pDependency->pUserData);
    }

    // Consider any nodes ready immediately
    DispatchReadyNodes();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::DeferredWorkerWrapperr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* DeferredRequestQueue::DeferredWorkerWrapper(
    VOID* pData)
{
    CamxResult  result      = CamxResultEFailed;
    Dependency* pDependency = reinterpret_cast<Dependency*>(pData);

    if ((NULL != pDependency) && (NULL != pDependency->pInstance))
    {
        DeferredRequestQueue* pDeferredQueue = pDependency->pInstance;

        result = pDeferredQueue->DeferredWorkerCore(pDependency);

        pDeferredQueue->LockForPublish();
        CAMX_FREE(pDependency);
        pDependency = NULL;
        pDeferredQueue->UnlockAfterPublish();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "DeferredWorker failed with result %s", Utils::CamxResultToString(result));
        }
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::AddDependencyEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DeferredRequestQueue::AddDependencyEntry(
    Dependency*      pDependency)
{
    CamxResult result    = CamxResultSuccess;
    UINT64     requestId = pDependency->requestId;

    LightweightDoublyLinkedListNode* pNode =
        reinterpret_cast<LightweightDoublyLinkedListNode*>(CAMX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

    if (NULL != pNode)
    {
        CAMX_LOG_DRQ("Adding dependencies for node: %d Node: %s request: %llu seqId: %d bindIOBuffers: %d",
                    pDependency->pNode->Type(),
                    pDependency->pNode->NodeIdentifierString(),
                    pDependency->requestId,
                    pDependency->processSequenceId,
                    pDependency->bindIOBuffers);

        UINT64 id = reinterpret_cast<UINT64>(pDependency);
        CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupDRQ, id, "Deferred Node %s:%d",
                         pDependency->pNode->Name(), pDependency->pNode->InstanceID());

        pNode->pData = pDependency;


        if ((0 == pDependency->propertyCount) &&
            (0 == pDependency->fenceCount)    &&
            (0 == pDependency->chiFenceCount))
        {
            // Node doesn't have any dependencies so it should be ready.
            m_pReadyQueueLock->Lock();
            m_readyNodes.InsertToTail(pNode);
            m_pReadyQueueLock->Unlock();
        }
        else
        {
            m_deferredNodes.InsertToTail(pNode);
        }

        // Add dependencies for all noted properties at requestId
        for (UINT i = 0; i < pDependency->propertyCount; i++)
        {
            LightweightDoublyLinkedList* pList   = NULL;

            INT64         offset  = (TRUE == pDependency->negate[i]) ? (-1 * static_cast<INT64>(pDependency->offsets[i]))
                                                                     : pDependency->offsets[i];
            INT64         intReq  = static_cast<INT64>(requestId);
            UINT64        request = (intReq - offset <= 0) ? FirstValidRequestId : (intReq - offset);
            DependencyKey mapKey  = {request, pDependency->pipelineIds[i], pDependency->properties[i], NULL, NULL};

            if (TRUE == m_logEnabled)
            {
                CHAR propertyName[128] = { 0 };
                HAL3MetadataUtil::GetNameByTag(propertyName, sizeof(propertyName), pDependency->properties[i]);

                CAMX_LOG_DRQ("Dependency Node: %s request: %llu seqId: %d -> "
                             "property[%d] = %08x %s pipeline[%d] = %d request = %llu bindIOBuffers = %d",
                             pDependency->pNode->NodeIdentifierString(),
                             pDependency->requestId,
                             pDependency->processSequenceId,
                             i,
                             pDependency->properties[i],
                             propertyName,
                             i,
                             pDependency->pipelineIds[i],
                             request,
                             pDependency->bindIOBuffers);
            }

            // Check if property already has a list
            if (CamxResultSuccess != m_pDependencyMap->Get(&mapKey, reinterpret_cast<VOID**>(&pList)))
            {
                CAMX_ASSERT(NULL == pList);
                // allocate and put in map
                pList = CAMX_NEW LightweightDoublyLinkedList();

                if (NULL != pList)
                {
                    m_pDependencyMap->Put(&mapKey, &pList);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                    result = CamxResultENoMemory;
                }
            }

            if (CamxResultSuccess == result)
            {
                // Allocate new node representing that the node has a dependency on prop i from requeustId
                pNode =
                    reinterpret_cast<LightweightDoublyLinkedListNode*>(CAMX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

                CAMX_ASSERT(NULL != pNode);

                if (NULL != pNode)
                {
                    pNode->pData = pDependency;

                    pList->InsertToTail(pNode);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                    result = CamxResultENoMemory;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                result = CamxResultENoMemory;
            }
        }

        // Add dependencies for all noted buffer fences at requestId
        for (UINT i = 0; i < pDependency->fenceCount; i++)
        {
            DependencyKey mapKey = {0, 0, PropertyIDInvalid, pDependency->phFences[i], NULL};

            LightweightDoublyLinkedList* pList  = NULL;

            CAMX_LOG_DRQ("Dependency Node: %s request: %llu seqId: %d -> "
                         "bindIOBuffers[%d] fence[%d] = %08x(%08x)",
                         pDependency->pNode->NodeIdentifierString(),
                         pDependency->requestId,
                         pDependency->processSequenceId,
                         pDependency->bindIOBuffers,
                         i,
                         pDependency->phFences[i],
                         *pDependency->phFences[i]);

            // Check if fence already has a list
            if (CamxResultSuccess != m_pDependencyMap->Get(&mapKey, reinterpret_cast<VOID**>(&pList)))
            {
                CAMX_ASSERT(NULL == pList);
                // allocate and put in map
                pList = CAMX_NEW LightweightDoublyLinkedList();
                if (NULL != pList)
                {
                    m_pDependencyMap->Put(&mapKey, &pList);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                    result = CamxResultENoMemory;
                }
            }

            CAMX_ASSERT(NULL != pList);

            if (CamxResultSuccess == result)
            {
                // Allocate new node representing that the node has a dependency on fence i from requeustId
                pNode =
                    reinterpret_cast<LightweightDoublyLinkedListNode*>(CAMX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

                CAMX_ASSERT(NULL != pNode);

                if (NULL != pNode)
                {
                    pNode->pData = pDependency;
                    pList->InsertToTail(pNode);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                    result = CamxResultENoMemory;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                result = CamxResultENoMemory;
            }
        }

        // Add dependencies for all noted Chi Fences at requestId
        for (UINT i = 0; i < pDependency->chiFenceCount; i++)
        {
            DependencyKey mapKey = {0, 0, PropertyIDInvalid, NULL, pDependency->pChiFences[i]};

            LightweightDoublyLinkedList* pList  = NULL;

            CAMX_LOG_DRQ("Chi fence[%d] = %08x(%08x)", i, pDependency->pChiFences[i], *pDependency->pChiFences[i]);

            // Check if fence already has a list
            if (CamxResultSuccess != m_pDependencyMap->Get(&mapKey, reinterpret_cast<VOID**>(&pList)))
            {
                CAMX_ASSERT(NULL == pList);
                // allocate and put in map
                pList = CAMX_NEW LightweightDoublyLinkedList();
                if (NULL != pList)
                {
                    m_pDependencyMap->Put(&mapKey, &pList);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                    result = CamxResultENoMemory;
                }
            }

            CAMX_ASSERT(NULL != pList);

            if (CamxResultSuccess == result)
            {
                // Allocate new node representing that the node has a dependency on fence i from requeustId
                pNode =
                    reinterpret_cast<LightweightDoublyLinkedListNode*>(CAMX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

                CAMX_ASSERT(NULL != pNode);

                if (NULL != pNode)
                {
                    pNode->pData = pDependency;
                    pList->InsertToTail(pNode);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                    result = CamxResultENoMemory;
                }
            }
            else
            {
                result = CamxResultENoMemory;
            }

            if (ChiFenceTypeInternal == pDependency->pChiFences[i]->type)
            {
                DeferredFenceCallbackData* pData =
                    reinterpret_cast<DeferredFenceCallbackData*>(CAMX_CALLOC(sizeof(DeferredFenceCallbackData)));

                if (NULL != pData)
                {
                    pData->pDeferredRequestQueue = this;
                    pData->pChiFence             = pDependency->pChiFences[i];
                    pData->requestId             = pDependency->requestId;

                    result = CSLFenceAsyncWait(pDependency->pChiFences[i]->hFence,
                                               &this->DependencyFenceCallbackCSL,
                                               pData);
                }
                else
                {
                    result = CamxResultENoMemory;
                    CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                }
            }
            else
            {
                CAMX_NOT_IMPLEMENTED();
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::AddDeferredNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DeferredRequestQueue::AddDeferredNode(
    UINT64           requestId,
    Node*            pNode,
    DependencyUnit*  pDependencyUnit)
{
    CamxResult  result      = CamxResultSuccess;

    // Freed by DeferredWorkerCore when all dependencies have been satisfied
    Dependency* pDependency = reinterpret_cast<Dependency*>(CAMX_CALLOC(sizeof(Dependency)));

    if (NULL != pDependency)
    {
        pDependency->pInstance = this;
        pDependency->pNode     = pNode;
        pDependency->requestId = requestId;

        pNode->SetRequestStatus(requestId, PerRequestNodeStatus::Deferred);

        m_pDeferredQueueLock->Lock();
        if (NULL != pDependencyUnit)
        {
            if (TRUE == pDependencyUnit->dependencyFlags.hasPropertyDependency)
            {
                GetUnpublishedList(pDependencyUnit, pDependency, requestId);
            }

            if (TRUE == pDependencyUnit->dependencyFlags.hasInputBuffersReadyDependency)
            {
                for (UINT i = 0; i < pDependencyUnit->bufferDependency.fenceCount; i++)
                {
                    if (0 == CamxAtomicLoadU(pDependencyUnit->bufferDependency.pIsFenceSignaled[i]))
                    {
                        pDependency->phFences[pDependency->fenceCount++] = pDependencyUnit->bufferDependency.phFences[i];
                    }
                }
            }

            if (TRUE == pDependencyUnit->dependencyFlags.hasFenceDependency)
            {
                for (UINT i = 0; i < pDependencyUnit->chiFenceDependency.chiFenceCount; i++)
                {
                    pDependency->pChiFences[pDependency->chiFenceCount++] =
                        pDependencyUnit->chiFenceDependency.pChiFences[i];
                }
                pDependency->pChiFenceCallback = pDependencyUnit->chiFenceDependency.pChiFenceCallback;
                pDependency->pUserData         = pDependencyUnit->chiFenceDependency.pUserData;
            }

            pDependency->preemptable        = ((TRUE == pDependencyUnit->dependencyFlags.isPreemptable) ||
                                               (TRUE == pNode->CanDRQPreemptOnStopRecording())) ? TRUE : FALSE;
            pDependency->processSequenceId    = pDependencyUnit->processSequenceId;
            pDependency->bindIOBuffers        = pDependencyUnit->dependencyFlags.hasIOBufferAvailabilityDependency;
            pDependency->isInternalDependency = pDependencyUnit->dependencyFlags.isInternalDependency;
        }

        result = AddDependencyEntry(pDependency);

        m_pDeferredQueueLock->Unlock();
    }
    else
    {
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::WaitForFenceDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DeferredRequestQueue::WaitForFenceDependency(
    ChiFence**          ppChiFences,
    UINT32              numFences,
    PFNCHIFENCECALLBACK pCallback,
    VOID*               pUserData)
{
    CamxResult  result      = CamxResultSuccess;
    // Freed by DeferredWorkerCore when all dependencies have been satisfied
    Dependency* pDependency = reinterpret_cast<Dependency*>(CAMX_CALLOC(sizeof(Dependency)));

    if (NULL != pDependency)
    {
        // Currently only handle 1 fence per wait
        CAMX_ASSERT(1 == numFences);

        m_pDeferredQueueLock->Lock();

        for (UINT i = 0; i < numFences; i++)
        {
            pDependency->pChiFences[i] = ppChiFences[i];
        }

        pDependency->pInstance          = this;
        pDependency->chiFenceCount      = numFences;
        pDependency->pChiFenceCallback  = pCallback;
        pDependency->pUserData          = pUserData;

        CAMX_LOG_DRQ("Adding Chi fence dependency: num fences: %d", numFences);

        // Don't care about node, request, active stream parameters
        result = AddChiDependencyEntry(pDependency);

        for (UINT i = 0; i < numFences; i++)
        {
            CAMX_LOG_DRQ("Chi fence[%d] = %08x", i, pDependency->pChiFences[i]);

            if (ChiFenceTypeInternal == pDependency->pChiFences[i]->type)
            {
                DeferredFenceCallbackData* pData =
                    reinterpret_cast<DeferredFenceCallbackData*>(CAMX_CALLOC(sizeof(DeferredFenceCallbackData)));

                if (NULL != pData)
                {
                    pData->pDeferredRequestQueue    = this;
                    pData->pChiFence                = pDependency->pChiFences[i];

                    result = CSLFenceAsyncWait(pDependency->pChiFences[i]->hFence,
                                               &this->FenceCallbackCSL,
                                               pData);
                }
                else
                {
                    result = CamxResultENoMemory;
                    CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                }
            }
            else
            {
                CAMX_NOT_IMPLEMENTED();
            }
        }
        m_pDeferredQueueLock->Unlock();
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::HandleFenceError
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::HandleFenceError(
    UINT       pipelineId,
    UINT64     requestId,
    BOOL       isFlush)
{
    m_pDeferredQueueLock->Lock();

    if (FALSE == RequestInErrorState(requestId, pipelineId))
    {
        LightweightDoublyLinkedListNode* pNode =
            reinterpret_cast<LightweightDoublyLinkedListNode*>(CAMX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

        PipelineRequest* pPipelineRequest =
            reinterpret_cast<PipelineRequest*>(CAMX_CALLOC(sizeof(PipelineRequest)));

        if ((NULL != pPipelineRequest) && (NULL != pNode))
        {
            pPipelineRequest->requestId = requestId;
            pPipelineRequest->pipelineId = pipelineId;
            pNode->pData = reinterpret_cast<VOID*>(pPipelineRequest);
            m_errorRequests.InsertToTail(pNode);
            CamxAtomicIncU(&m_numErrorRequests);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Out of memory");
        }

        // Assuming that the total across all nodes is less than the max for a single node, may need adjustment
        UINT props[MaxProperties];
        UINT numProps = 0;
        // Loop through dependencies and signal any for the request in error state
        LightweightDoublyLinkedListNode* pDeferred = m_deferredNodes.Head();
        while (NULL != pDeferred)
        {
            Dependency* pDependency = static_cast<Dependency*>(pDeferred->pData);

            CAMX_ASSERT(NULL != pDependency);

            if (NULL != pDependency)
            {
                // Step through each dependency and queue
                for (UINT i = 0; i < pDependency->propertyCount; i++)
                {
                    UINT64 dependencyRequest = pDependency->requestId;

                    if (TRUE == pDependency->negate[i])
                    {
                        dependencyRequest += pDependency->offsets[i];
                    }
                    else if (pDependency->offsets[i] >= dependencyRequest)
                    {
                        dependencyRequest = FirstValidRequestId;
                    }
                    else
                    {
                        dependencyRequest -= pDependency->offsets[i];
                    }

                    if ((dependencyRequest == requestId) && (pDependency->pipelineIds[i] == pipelineId))
                    {
                        if (MaxProperties > numProps)
                        {
                            props[numProps] = pDependency->properties[i];
                        }
                        numProps++;
                    }
                }

                if (MaxProperties < numProps)
                {
                    CAMX_ASSERT_ALWAYS();
                    CAMX_LOG_ERROR(CamxLogGroupCore,
                                    "Failed to signal all properties on error. Expect hang");
                }
            }
            pDeferred = LightweightDoublyLinkedList::NextNode(pDeferred);
        }

        numProps = CamX::Utils::MinUINT32(MaxProperties, numProps);

        for (UINT propIndex = 0; propIndex < numProps; propIndex++)
        {
            UpdateDependency(static_cast<PropertyID>(props[propIndex]), NULL, NULL, requestId, pipelineId, FALSE, isFlush);
        }

        // Consider any nodes now ready
        DispatchReadyNodes();
    }

    m_pDeferredQueueLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::RequestInErrorState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL DeferredRequestQueue::RequestInErrorState(
    UINT64  requestId,
    UINT    pipelineId)
{
    BOOL error = FALSE;

    if (0 < CamxAtomicLoadU(&m_numErrorRequests))
    {
        m_pDeferredQueueLock->Lock();
        LightweightDoublyLinkedListNode* pNode     = m_errorRequests.Head();
        while (NULL != pNode)
        {
            PipelineRequest* pPiplineRequest = reinterpret_cast<PipelineRequest*>(pNode->pData);
            if ((requestId == pPiplineRequest->requestId) && (pipelineId == pPiplineRequest->pipelineId))
            {
                error = TRUE;
                break;
            }

            pNode = LightweightDoublyLinkedList::NextNode(pNode);
        }
        m_pDeferredQueueLock->Unlock();
    }
    return error;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::UpdateDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::UpdateDependency(
    PropertyID  propertyId,
    CSLFence*   phFence,
    ChiFence*   pChiFence,
    UINT64      requestId,
    UINT        pipelineId,
    BOOL        isSuccess,
    BOOL        isFlush)
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "DeferredRequestQueue::UpdateDependency");

    CHAR propertyName[128] = { 0 };

    if (TRUE == m_logEnabled)
    {
        HAL3MetadataUtil::GetNameByTag(propertyName, sizeof(propertyName), propertyId);
    }

    m_pDeferredQueueLock->Lock();
    if (FALSE == isSuccess)
    {
        if (FALSE == isFlush)
        {
            CAMX_LOG_ERROR(CamxLogGroupDRQ,
                "Failure to update: property: %08x %s, fence: %08x(%08x), request: %llu...result will"
                "be an error",
                 propertyId,
                 propertyName,
                 phFence,
                 (NULL == phFence) ? 0 : *phFence,
                 requestId);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupDRQ,
                "Failure to update during flush: property: %08x %s, fence: %08x(%08x), request: %llu...result will"
                "be an error",
                 propertyId,
                 propertyName,
                 phFence,
                 (NULL == phFence) ? 0 : *phFence,
                 requestId);
        }

        // Let the code process the dependency as if it were satisfied.  The Node base class will return a previously
        // successful property/buffer to allow execution to continue.  The result will be returned to the FW as an error
    }
    else if (PropertyIDInvalid != propertyId) // Avoid clearing error list in case of fenceCb
    {
        m_pDeferredQueueLock->Lock();
        LightweightDoublyLinkedListNode* pNode = m_errorRequests.Head();

        // clean old requests out of the error list
        while ((NULL != pNode) && (NULL != pNode->pData))
        {
            /// @todo (CAMX-4287): Use (requestQueueDepth + 1)
            PipelineRequest* pPipelineRequest = reinterpret_cast<PipelineRequest*>(pNode->pData);
            if ((pPipelineRequest->pipelineId == pipelineId) &&
                (pPipelineRequest->requestId + RequestQueueDepth < requestId))
            {
                LightweightDoublyLinkedListNode* pNext = pNode->pNext;
                CAMX_LOG_INFO(CamxLogGroupDRQ, "Clearing old error: %llu", pPipelineRequest->requestId);
                m_errorRequests.RemoveNode(pNode);
                CamxAtomicDecU(&m_numErrorRequests);
                CAMX_FREE(pNode->pData);
                pNode->pData = NULL;
                CAMX_FREE(pNode);
                pNode = pNext;
            }
            else
            {
                pNode = LightweightDoublyLinkedList::NextNode(pNode);
            }
        }
        m_pDeferredQueueLock->Unlock();
    }

    CAMX_LOG_DRQ("Update Dependency - property: %08x %s, fence: %08x(%d), request: %llu, pipeline: %d",
                 propertyId,
                 propertyName,
                 phFence,
                 (NULL == phFence) ? 0 : *phFence,
                 requestId,
                 pipelineId);


    DependencyKey mapKey =
    {
        (propertyId == PropertyIDInvalid) ? 0  : requestId,
                                                 pipelineId,
                                                 propertyId,
                                                 phFence,
                                                 pChiFence
    };

    UpdateOrRemoveDependency(&mapKey, NULL);

    m_pDeferredQueueLock->Unlock();

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::DispatchReadyNodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::DispatchReadyNodes()
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "DeferredRequestQueue::DispatchReadyNodes");

    // If preempt dependency enabled and dependency registered for preemption,
    // clear node dependency in deferred list first.
    if (TRUE == m_preemptDependency.isPreemptDependencyEnabled)
    {
        m_pDeferredQueueLock->Lock();
        LightweightDoublyLinkedListNode* pDeferred = m_deferredNodes.Head();
        while (NULL != pDeferred)
        {
            Dependency* pDependency = static_cast<Dependency*>(pDeferred->pData);
            if ((NULL != pDependency)  &&
                (((TRUE == pDependency->preemptable) &&
                (FALSE == pDependency->isInternalDependency) &&
                (FALSE == m_preemptDependency.pipelineDepenency)) ||
                ((TRUE == m_preemptDependency.pipelineDepenency) &&
                (m_preemptDependency.pipelineIndex == pDependency->pNode->GetPipelineId()))))
            {
                CAMX_LOG_DRQ("Remove dependencies for Node: %s, request: %llu, seqID %d",
                                pDependency->pNode->NodeIdentifierString(),
                                pDependency->requestId,
                                pDependency->processSequenceId);

                RemoveAllDependencies(pDependency);
            }

            if (NULL != (m_deferredNodes.FindByValue(pDependency)))
            {
                pDeferred = LightweightDoublyLinkedList::NextNode(pDeferred);
            }
            else
            {
                // pDeferred is removed from deferred list to ready list
                pDeferred = NULL;
            }
        }
        m_pDeferredQueueLock->Unlock();
    }

    while (0 < m_readyNodes.NumNodes())
    {
        LightweightDoublyLinkedListNode* pReady      = NULL;
        Dependency*                      pDependency = NULL;

        m_pReadyQueueLock->Lock();
        if (0 < m_readyNodes.NumNodes())
        {
            pReady      = m_readyNodes.Head();
            pDependency = static_cast<Dependency*>(pReady->pData);
            m_readyNodes.RemoveNode(pReady);
        }
        m_pReadyQueueLock->Unlock();

        // Dispatch and remove all completed subscribers from the deferred node subscription list
        if (NULL != pReady)
        {
            CAMX_FREE(pReady);
            CAMX_ASSERT(NULL != pDependency);

            if (NULL != pDependency)
            {
                if (NULL != pDependency->pChiFenceCallback)
                {
                    pDependency->pChiFenceCallback(pDependency->pChiFences[0]->hChiFence, pDependency->pUserData);
                }

                UINT64 id = reinterpret_cast<UINT64>(pDependency);
                CAMX_TRACE_ASYNC_END_F(CamxLogGroupDRQ, id, "Deferred Node %s", pDependency->pNode->NodeIdentifierString());

                CAMX_LOG_DRQ("Posting job for Node %s, request %llu seqID %d",
                                 pDependency->pNode->NodeIdentifierString(),
                                 pDependency->requestId,
                                 pDependency->processSequenceId);
                // Fire the deferred node processing
                VOID* pData[] = {pDependency, NULL};
                CamxResult result = m_pThreadManager->PostJob(m_hDeferredWorker, NULL, &pData[0], FALSE, FALSE);

                if (CamxResultSuccess != result)
                {
                    if (NULL != pDependency)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to post ProcessRequest job for Node %s, request %llu seqID %d",
                                    pDependency->pNode->NodeIdentifierString(),
                                    pDependency->requestId, pDependency->processSequenceId);
                    }
                }

            }
            else
            {
                // No data for the entry to exist to track
                CAMX_LOG_ERROR(CamxLogGroupDRQ, "No dependencies");
            }
        }
    }

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::GetUnpublishedList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::GetUnpublishedList(
    DependencyUnit* pInput,
    Dependency*     pDependency,
    UINT64          requestId)
{
    UINT32* pSrcList        = pInput->propertyDependency.properties;
    UINT*   pSrcPipelineIds = pInput->propertyDependency.pipelineIds;
    UINT64* pSrcOffsetList  = pInput->propertyDependency.offsets;
    BOOL*   pSrcNegateList  = pInput->propertyDependency.negate;
    UINT32  dstCount        = 0;

    for (UINT i = 0; i < pInput->propertyDependency.count; i++)
    {
        CAMX_ASSERT(m_numPipelines > pSrcPipelineIds[i]);

        UnitType      type         = (0 == (pSrcList[i] & DriverInternalGroupMask)) ? UnitType::Metadata : UnitType::Property;
        INT64         offset       = (TRUE == pSrcNegateList[i]) ?
                                     (-1 * static_cast<INT64>(pSrcOffsetList[i])) : pSrcOffsetList[i];
        INT64         intReq       = static_cast<INT64>(requestId);
        UINT64        request      = (intReq - offset <= 0) ? FirstValidRequestId : (intReq - offset);

        // Would generally lock around this to ensure the pools didnt publish, but the caller holds the DRQ lock, and because
        // all pools lock their subscribers, of which DRQ is one, to publish, published bits wont change while we're here
        MetadataSlot* pCurrentSlot = m_pMainPools[pSrcPipelineIds[i]]->GetSlot(request);

        if ((FALSE == RequestInErrorState(request, pSrcPipelineIds[i])) &&
            ((request != pCurrentSlot->GetSlotRequestId()) || (FALSE == pCurrentSlot->IsPublished(type, pSrcList[i]))))
        {
            pDependency->properties[dstCount]  = pSrcList[i];
            pDependency->offsets[dstCount]     = pSrcOffsetList[i];
            pDependency->negate[dstCount]      = pSrcNegateList[i];
            pDependency->pipelineIds[dstCount] = pSrcPipelineIds[i];

            CAMX_LOG_DRQ("Unpublished list [%d] for node %s property: %x, request %d pipeline: %d",
                         dstCount,
                         pDependency->pNode->Name(),
                         pSrcList[i],
                         requestId,
                         pSrcPipelineIds[i]);
            dstCount++;
        }
    }

    pDependency->propertyCount = dstCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::FenceErrorSignaledCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::FenceErrorSignaledCallback(
    UINT       pipelineId,
    CSLFence*  phFence,
    UINT64     requestId,
    BOOL       isFlush)
{
    // Topology will call in here with the current request Id
    HandleFenceError(pipelineId, requestId, isFlush);
    UpdateDependency(PropertyIDInvalid, phFence, NULL, requestId, 0, FALSE, isFlush);
    DispatchReadyNodes();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::FenceSignaledCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::FenceSignaledCallback(
    CSLFence* phFence,
    UINT64    requestId)
{
    // Topology will call in here with the current request Id
    UpdateDependency(PropertyIDInvalid, phFence, NULL, requestId, 0, TRUE, FALSE);
    DispatchReadyNodes();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::ChiFenceSignaledCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::ChiFenceSignaledCallback(
    ChiFence* pChiFence,
    UINT64    requestId)
{
    // Waiting Chi Fence Callback will call in here
    UpdateDependency(PropertyIDInvalid, NULL, pChiFence, requestId, 0, TRUE, FALSE);
    DispatchReadyNodes();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::OnPropertyUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::OnPropertyUpdate(
    PropertyID  id,
    UINT64      requestId,
    UINT        pipelineId)
{
    // Data Pool will call in here with the current request Id
    UpdateDependency(id, NULL, NULL, requestId, pipelineId, TRUE, FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::OnMetadataUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::OnMetadataUpdate(
    UINT32 tag,
    UINT64 requestId,
    UINT   pipelineId)
{
    // Data Pool will call in here with the current request Id
    UpdateDependency(static_cast<PropertyID>(tag), NULL, NULL, requestId, pipelineId, TRUE, FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::OnPropertyFailure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::OnPropertyFailure(
    PropertyID  id,
    UINT64      requestId,
    UINT        pipelineId)
{
    // Data Pool will call in here with the current request Id
    UpdateDependency(id, NULL, NULL, requestId, pipelineId, FALSE, FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::OnMetadataFailure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::OnMetadataFailure(
    UINT32 tag,
    UINT64 requestId,
    UINT   pipelineId)
{
    // Data Pool will call in here with the current request Id
    UpdateDependency(static_cast<PropertyID>(tag), NULL, NULL, requestId, pipelineId, FALSE, FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::DumpDebugInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::DumpDebugInfo(
    UINT64 id,
    BOOL   forceDump)
{
    if ((TRUE == m_logEnabled) || (TRUE == forceDump))
    {
        CHAR propertyName[128] = { 0 };

        CamxResult result = m_pDeferredQueueLock->TryLock();

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupCore, "WARNING: DRQ Lock failed with status: %d.  Stopping dump",
                          result);
            return;
        }

        LightweightDoublyLinkedListNode* pDeferred = m_deferredNodes.Head();

        // Remove all completed subscribers from the deferred node subscription list
        while (NULL != pDeferred)
        {
            Dependency*                      pDependency    = static_cast<Dependency*>(pDeferred->pData);
            LightweightDoublyLinkedListNode* pNext          = LightweightDoublyLinkedList::NextNode(pDeferred);

            if (NULL != pDependency)
            {
                if (pDependency->requestId == id)
                {
                    CAMX_LOG_DUMP(CamxLogGroupDRQ, "+----------------------------+");
                    CAMX_LOG_DUMP(CamxLogGroupDRQ, "+    DRQ - req %08d      +", id);
                    CAMX_LOG_DUMP(CamxLogGroupDRQ, "+   Node: %s, Request: %llu, SeqId: %d,"
                                                   " signaledCount: %d, publishedCount: %d bindIOBuffers: %d",
                                                   pDependency->pNode->NodeIdentifierString(),
                                                   pDependency->requestId,
                                                   pDependency->processSequenceId,
                                                   pDependency->signaledCount,
                                                   pDependency->publishedCount,
                                                   pDependency->bindIOBuffers);

                    for (UINT propertyIndex = 0; propertyIndex < pDependency->propertyCount; propertyIndex++)
                    {
                        PropertyID    propertyId    = pDependency->properties[propertyIndex];
                        UINT          srcPipelineId = pDependency->pipelineIds[propertyIndex];
                        UINT64        srcOffset     = pDependency->offsets[propertyIndex];
                        BOOL          srcNegate     = pDependency->negate[propertyIndex];
                        INT64         offset        = (TRUE == srcNegate) ? (-1 * static_cast<INT64>(srcOffset)) : srcOffset;

                        INT64         intReq        = static_cast<INT64>(pDependency->requestId);
                        UINT64        request       = (intReq - offset <= 0) ? FirstValidRequestId : (intReq - offset);
                        MetadataSlot* pCurrentSlot  = m_pMainPools[srcPipelineId]->GetSlot(request);
                        UnitType      type          = (0 == (propertyId & DriverInternalGroupMask)) ?
                                                         UnitType::Metadata :
                                                         UnitType::Property;

                        BOOL isPublished = ((TRUE == RequestInErrorState(request, srcPipelineId)) ||
                                            ((request == pCurrentSlot->GetSlotRequestId()) &&
                                             (TRUE == pCurrentSlot->IsPublished(type, propertyId))));

                        if (FALSE == isPublished)
                        {
                            HAL3MetadataUtil::GetNameByTag(propertyName,
                                                           sizeof(propertyName), pDependency->properties[propertyIndex]);
                            CAMX_LOG_DUMP(CamxLogGroupDRQ, "+    Property[%d] = 0x%08x %s on Pipeline = %d is not published",
                                          propertyIndex,
                                          propertyId,
                                          propertyName,
                                          srcPipelineId);
                        }
                    }
                    for (UINT i = 0; i < pDependency->fenceCount; i++)
                    {
                        CAMX_LOG_DUMP(CamxLogGroupDRQ, "+    Fence[%d] = 0x%08x(%08x)",
                            i, pDependency->phFences[i], *pDependency->phFences[i]);
                    }
                }
            }
            pDeferred = pNext;
        }

        pDeferred = m_CHIFenceDependencies.Head();
        // Remove all completed subscribers from the deferred node subscription list
        while (NULL != pDeferred)
        {
            Dependency*                      pDependency = static_cast<Dependency*>(pDeferred->pData);
            LightweightDoublyLinkedListNode* pNext       = LightweightDoublyLinkedList::NextNode(pDeferred);

            if (NULL != pDependency)
            {
                if (pDependency->requestId == id)
                {
                    CAMX_LOG_DUMP(CamxLogGroupDRQ, "+   UserData: %p, Callback: %p",
                                   pDependency->pUserData,
                                   pDependency->pChiFenceCallback);
                    CAMX_LOG_DUMP(CamxLogGroupDRQ, "+   signaledCount: %d", pDependency->chiSignaledCount);

                    for (UINT i = 0; i < pDependency->chiFenceCount; i++)
                    {
                        CAMX_LOG_DUMP(CamxLogGroupDRQ, "+   Fence[%d] = 0x%08x", i, *pDependency->pChiFences[i]);
                    }
                }
                CAMX_LOG_DUMP(CamxLogGroupDRQ, "+----------------------------+");
            }
            pDeferred = pNext;
        }
        m_pDeferredQueueLock->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::FenceCallbackCSL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::FenceCallbackCSL(
    VOID*           pUserData,
    CSLFence        hSyncFence,
    CSLFenceResult  result)
{
    DeferredFenceCallbackData* pData = reinterpret_cast<DeferredFenceCallbackData*>(pUserData);

    CAMX_ASSERT(NULL != pData);
    CAMX_ASSERT (hSyncFence == pData->pChiFence->hFence);

    if (NULL != pData)
    {
        pData->pDeferredRequestQueue->UpdateChiFenceDependency(pData->pChiFence, CSLFenceResultSuccess == result);

        CAMX_FREE(pData);
        pData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::DependencyFenceCallbackCSL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::DependencyFenceCallbackCSL(
    VOID*           pUserData,
    CSLFence        hSyncFence,
    CSLFenceResult  result)
{
    DeferredFenceCallbackData* pData = reinterpret_cast<DeferredFenceCallbackData*>(pUserData);

    CAMX_ASSERT(NULL != pData);
    CAMX_ASSERT (hSyncFence == pData->pChiFence->hFence);

    if (NULL != pData)
    {
        if (CSLFenceResultSuccess == result)
        {
            pData->pDeferredRequestQueue->ChiFenceSignaledCallback(pData->pChiFence, pData->requestId);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupDRQ, "Fence failure with result %d, fence callback will not be called" , result);
        }
        CAMX_FREE(pData);
        pData = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::AddChiDependencyEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult DeferredRequestQueue::AddChiDependencyEntry(
    Dependency* pDependency)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(pDependency->chiFenceCount > 0);

    LightweightDoublyLinkedListNode* pNode =
        reinterpret_cast<LightweightDoublyLinkedListNode*>(CAMX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

    if (NULL != pNode)
    {
        CAMX_LOG_DRQ("Adding dependencies for user data: %p, callback: %p",
                     pDependency->pUserData,
                     pDependency->pChiFenceCallback);

        pNode->pData = pDependency;
        m_CHIFenceDependencies.InsertToTail(pNode);
        // Add dependencies for all noted fences at requestId
        for (UINT i = 0; i < pDependency->chiFenceCount; i++)
        {
            DependencyKey mapKey =
            {
                0,
                0,
                PropertyIDInvalid,
                NULL,
                pDependency->pChiFences[i]
            };
            LightweightDoublyLinkedList* pList  = NULL;

            CAMX_LOG_DRQ("Chi fence[%d] = %08x", i, pDependency->pChiFences[i]);

            // Check if fence already has a list
            if (CamxResultSuccess != m_pDependencyMap->Get(&mapKey, reinterpret_cast<VOID**>(&pList)))
            {
                CAMX_ASSERT(NULL == pList);
                // allocate and put in map
                pList = CAMX_NEW LightweightDoublyLinkedList();
                if (NULL != pList)
                {
                    m_pDependencyMap->Put(&mapKey, &pList);
                    CamxAtomicInc(&pDependency->pChiFences[i]->aRefCount);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                    result = CamxResultENoMemory;
                }
            }

            CAMX_ASSERT(NULL != pList);

            if (CamxResultSuccess == result)
            {
                // Allocate new node representing that the node has a dependency on fence i
                pNode =
                    reinterpret_cast<LightweightDoublyLinkedListNode*>(CAMX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

                CAMX_ASSERT(NULL != pNode);

                if (NULL != pNode)
                {
                    pNode->pData = pDependency;
                    pList->InsertToTail(pNode);
                }
                else
                {
                    result = CamxResultENoMemory;
                    CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
                }
            }
            else
            {
                result = CamxResultENoMemory;
                CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
            }
        }
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupDRQ, "Out of memory");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::UpdateChiFenceDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::UpdateChiFenceDependency(
    ChiFence*   pChiFence,
    BOOL        isSuccess)
{
    CamxResult result = CamxResultEFailed;
    BOOL       update = FALSE;

    if (FALSE == isSuccess)
    {
        ///@ todo (CAMX-325) - Handle update failure
        return;
    }

    CAMX_LOG_DRQ("Update Dependency - Chi fence: %08x", pChiFence);

    m_pDeferredQueueLock->Lock();

    DependencyKey                mapKey = {0, 0, PropertyIDInvalid, NULL, pChiFence};
    LightweightDoublyLinkedList* pList  = NULL;

    m_pDependencyMap->Get(&mapKey, reinterpret_cast<VOID**>(&pList));

    if (NULL != pList)
    {
        LightweightDoublyLinkedListNode* pNode = pList->Head();

        while (NULL != pNode)
        {
            Dependency* pDependency = static_cast<Dependency*>(pNode->pData);

            CAMX_ASSERT(NULL != pDependency);

            if (NULL != pDependency)
            {
                if (NULL != pChiFence)
                {
                    pDependency->chiSignaledCount++;
                    CAMX_LOG_DRQ("Chi Fence %08x signaled", pChiFence);
                }

                if (pDependency->chiFenceCount == pDependency->chiSignaledCount)
                {
                    update = TRUE;
                }
            }

            // Move to next subscriber and remove the node from this subscription
            LightweightDoublyLinkedListNode* pNext = LightweightDoublyLinkedList::NextNode(pNode);
            pList->RemoveNode(pNode);

            // No longer need the linked list entry. The data is associated finally with m_CHIFenceDependencies
            CAMX_FREE(pNode);

            pNode = pNext;
        }

        // If list empty delete it and the hashmap entry. Keeps from growing unbounded
        if (0 == pList->NumNodes())
        {
            CAMX_DELETE pList;
            pList = NULL;
            m_pDependencyMap->Remove(&mapKey);
            CamxAtomicDec(&pChiFence->aRefCount);
        }
    }

    // if at least one deferred node was signalled process the list and remove processed dependencies references
    if (TRUE == update)
    {
        LightweightDoublyLinkedListNode* pDependencyRef = m_CHIFenceDependencies.Head();

        // Remove all completed subscribers from the subscription list
        while (NULL != pDependencyRef)
        {
            Dependency*                      pDependency = static_cast<Dependency*>(pDependencyRef->pData);
            LightweightDoublyLinkedListNode* pNext       = LightweightDoublyLinkedList::NextNode(pDependencyRef);

            CAMX_ASSERT(NULL != pDependency);

            if (NULL != pDependency)
            {
                if (pDependency->chiFenceCount == pDependency->chiSignaledCount)
                {
                    CAMX_LOG_DRQ("All chi dependencies satisfied for pUserData (%p), callback (%p)",
                                 pDependency->pUserData, pDependency->pChiFenceCallback);

                    // Fire the deferred node processing
                    VOID* pData[]   = {pDependency, NULL};
                    result          = m_pThreadManager->PostJob(m_hDeferredWorker, NULL, &pData[0], FALSE, FALSE);

                    m_CHIFenceDependencies.RemoveNode(pDependencyRef);
                    CAMX_FREE(pDependencyRef);  // Free the linked list entry
                }
            }
            else
            {
                // No data for the entry to exist to track
                m_CHIFenceDependencies.RemoveNode(pDependencyRef);
                CAMX_FREE(pDependencyRef);
            }
            pDependencyRef = pNext;
        }
    }

    m_pDeferredQueueLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::DumpState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::DumpState(
    INT     fd,
    UINT32  indent)
{
    /// @note Accessing with a TryLock since this is intended to be a post-mortem log.  If we try to enforce the lock, there's a
    ///       reasonable chance the post-mortem will deadlock. Failed locks will be noted.
    CamxResult result = m_pDeferredQueueLock->TryLock();

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_TO_FILE(fd, indent, "WARNING: Lock failed with status: %s.  Stopping dump",
                         CamxResultStrings[result]);
        return;
    }

    CAMX_LOG_TO_FILE(fd, indent, "Num deferred nodes     = %u", m_deferredNodes.NumNodes());
    CAMX_LOG_TO_FILE(fd, indent, "Num Chi deferred nodes = %u", m_CHIFenceDependencies.NumNodes());

    LightweightDoublyLinkedListNode* pDeferred = m_deferredNodes.Head();
    CAMX_LOG_TO_FILE(fd, indent, "-----------------------");
    CAMX_LOG_TO_FILE(fd, indent, "Internal dependencies: ");
    // Loop over all completed subscribers from the deferred node subscription list
    while (NULL != pDeferred)
    {
        Dependency*                      pDependency = static_cast<Dependency*>(pDeferred->pData);
        LightweightDoublyLinkedListNode* pNext       = LightweightDoublyLinkedList::NextNode(pDeferred);

        if (NULL != pDependency)
        {
            CAMX_LOG_TO_FILE(fd, indent + 2, "-----------------------");
            CAMX_LOG_TO_FILE(fd, indent + 2,
                             "Dependency %p - RequestId = %llu, Node::%s",
                             pDependency,
                             pDependency->requestId,
                             pDependency->pNode->NodeIdentifierString(),
                             pDependency->processSequenceId,
                             Utils::BoolToString(pDependency->bindIOBuffers));

            CAMX_LOG_TO_FILE(fd, indent + 4,
                             "ProcessSequenceId = %d, bindIOBuffers = %s, preemptable = %s, isInternalDependency = %s",
                             pDependency->processSequenceId,
                             Utils::BoolToString(pDependency->bindIOBuffers),
                             Utils::BoolToString(pDependency->preemptable),
                             Utils::BoolToString(pDependency->isInternalDependency));

            // Print Properties that we are depending on
            CAMX_LOG_TO_FILE(fd, indent + 4, "Properties = { count = %u, published = %u }",
                             pDependency->propertyCount,
                             pDependency->publishedCount);

            CHAR propertyName[128] = { 0 };
            for (UINT propertyIndex = 0; propertyIndex < pDependency->propertyCount; propertyIndex++)
            {
                PropertyID propertyId    = pDependency->properties[propertyIndex];
                UINT       srcPipelineId = pDependency->pipelineIds[propertyIndex];
                UINT64     srcOffset     = pDependency->offsets[propertyIndex];
                BOOL       srcNegate     = pDependency->negate[propertyIndex];

                UnitType type    = (0 == (propertyId & DriverInternalGroupMask)) ? UnitType::Metadata : UnitType::Property;
                INT64    offset  = (TRUE == srcNegate) ? (-1 * static_cast<INT64>(srcOffset)) : srcOffset;
                INT64    intReq  = static_cast<INT64>(pDependency->requestId);
                UINT64   request = (intReq - offset <= 0) ? FirstValidRequestId : (intReq - offset);

                MetadataSlot* pCurrentSlot = m_pMainPools[srcPipelineId]->GetSlot(request);
                BOOL          isPublished  = ((TRUE == RequestInErrorState(request, srcPipelineId)) ||
                                              ((request == pCurrentSlot->GetSlotRequestId()) &&
                                               (TRUE  == pCurrentSlot->IsPublished(type, propertyId))));

                if (FALSE == isPublished)
                {
                    HAL3MetadataUtil::GetNameByTag(propertyName, sizeof(propertyName), pDependency->properties[propertyIndex]);
                    CAMX_LOG_TO_FILE(fd, indent + 6,
                                     "Property_%u = { Id = 0x%08X, Name = %s, srcPipelineId = %u} is Unpublished",
                                     propertyIndex,
                                     propertyId,
                                     propertyName,
                                     srcPipelineId);
                }
            }

            // Print CSL Fences that we are depending on
            CAMX_LOG_TO_FILE(fd, indent + 4, "CSLFences = { count = %u, published = %u }",
                             pDependency->fenceCount,
                             pDependency->signaledCount);

            for (UINT i = 0; i < pDependency->fenceCount; i++)
            {
                CAMX_LOG_TO_FILE(fd, indent + 6, "CSLFence_%u = { value = 0x%08x, address = %p}",
                                 i,
                                 *pDependency->phFences[i],
                                 pDependency->phFences[i]);
            }
        }
        pDeferred = pNext;
    }

    pDeferred = m_CHIFenceDependencies.Head();

    CAMX_LOG_TO_FILE(fd, indent, "-----------------------");
    CAMX_LOG_TO_FILE(fd, indent, "Chi dependencies: ");
    // Remove all completed subscribers from the external subscription list
    while (NULL != pDeferred)
    {
        Dependency*                      pDependency = static_cast<Dependency*>(pDeferred->pData);
        LightweightDoublyLinkedListNode* pNext       = LightweightDoublyLinkedList::NextNode(pDeferred);

        if (NULL != pDependency)
        {
            CAMX_LOG_TO_FILE(fd, indent + 2,
                             "ChiDependency %p - UserData: %p, Callback: %p",
                             pDependency,
                             pDependency->pUserData,
                             pDependency->pChiFenceCallback);

            // Print Chi Fences that we are depending on
            CAMX_LOG_TO_FILE(fd, indent + 4, "ChiFences = {count = %u, published = %u}",
                             pDependency->chiFenceCount,
                             pDependency->chiSignaledCount);

            for (UINT i = 0; i < pDependency->chiFenceCount; i++)
            {
                CAMX_LOG_TO_FILE(fd, indent + 6, "ChiFence_%u = { value = 0x%08x, address = %p}",
                                 i,
                                 *pDependency->pChiFences[i],
                                 pDependency->pChiFences[i]);
            }
        }
        pDeferred = pNext;
    }

    if (CamxResultSuccess == result)
    {
        m_pDeferredQueueLock->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::SetPreemptDependencyFlag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::SetPreemptDependencyFlag(
    BOOL isPreemptDependencyEnabled)
{
    m_preemptDependency.isPreemptDependencyEnabled = isPreemptDependencyEnabled;
    m_preemptDependency.pipelineDepenency          = FALSE;
    CAMX_LOG_DRQ("Preempt Dependencies: %d, pipelineDependency: %d", isPreemptDependencyEnabled,
                                                                     m_preemptDependency.pipelineDepenency);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// DeferredRequestQueue::SetPreemptPipelineDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::SetPreemptPipelineDependency(
    BOOL   isPreemptDependencyEnabled,
    UINT32 pipelineIndex)
{
    m_preemptDependency.isPreemptDependencyEnabled = isPreemptDependencyEnabled;
    m_preemptDependency.pipelineIndex              = pipelineIndex;
    m_preemptDependency.pipelineDepenency          = TRUE;
    CAMX_LOG_DRQ("Preempt Dependencies: %d, pipelineIndex: %u, pipelineDependency: %d",
                 isPreemptDependencyEnabled,
                 pipelineIndex,
                 m_preemptDependency.pipelineDepenency);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::UpdateOrRemoveDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::UpdateOrRemoveDependency(
    DependencyKey* pMapKey,
    Dependency*    pDependencyToRemove)
{
    LightweightDoublyLinkedList* pList = NULL;

    CAMX_ASSERT(NULL != pMapKey);
    m_pDependencyMap->Get(pMapKey, reinterpret_cast<VOID**>(&pList));
    if (NULL != pList)
    {
        LightweightDoublyLinkedListNode* pNode = pList->Head();
        while (NULL != pNode)
        {
            LightweightDoublyLinkedListNode* pNext       = LightweightDoublyLinkedList::NextNode(pNode);
            Dependency*                      pDependency = static_cast<Dependency*>(pNode->pData);

            CAMX_ASSERT(NULL != pDependency);
            if (NULL != pDependency)
            {
                if ((NULL == pDependencyToRemove) || (pDependencyToRemove == pDependency))
                {
                    if (PropertyIDInvalid != pMapKey->dataId)
                    {
                        pDependency->publishedCount++;

                        if (TRUE == m_logEnabled)
                        {
                            CHAR propertyName[128] = { 0 };
                            HAL3MetadataUtil::GetNameByTag(propertyName, sizeof(propertyName), pMapKey->dataId);

                            CAMX_LOG_DRQ("Property %08x %s published for Node: %s request: %llu "
                                        "seqId: %d bindIOBuffers: %d",
                                        pMapKey->dataId,
                                        propertyName,
                                        pDependency->pNode->NodeIdentifierString(),
                                        pDependency->requestId,
                                        pDependency->processSequenceId,
                                        pDependency->bindIOBuffers);
                        }
                    }
                    else if (NULL != pMapKey->pFence)
                    {
                        pDependency->signaledCount++;
                        CAMX_LOG_DRQ("Fence %08x(%08x) signaled for Node: %s request: %llu "
                                     "seqId: %d bindIOBuffers: %d",
                                     pMapKey->pFence,
                                     (NULL == pMapKey->pFence) ? 0 : *reinterpret_cast<CSLFence *>(pMapKey->pFence),
                                     pDependency->pNode->NodeIdentifierString(),
                                     pDependency->requestId,
                                     pDependency->processSequenceId,
                                     pDependency->bindIOBuffers);
                    }
                    else if (NULL != pMapKey->pChiFence)
                    {
                        pDependency->chiSignaledCount++;
                        CAMX_LOG_DRQ("Chi Fence %08x(%08x) signaled for node: %s",
                                   pMapKey->pChiFence,
                                   (NULL == pMapKey->pChiFence) ? 0 : reinterpret_cast<ChiFence *>(pMapKey->pChiFence)->hFence,
                                   pDependency->pNode->Name());
                    }

                    if ((pDependency->propertyCount == pDependency->publishedCount) &&
                        (pDependency->fenceCount    == pDependency->signaledCount)  &&
                        (pDependency->chiFenceCount == pDependency->chiSignaledCount))
                    {
                        CAMX_LOG_DRQ("Node: %s - all satisfied. request %llu",
                                     pDependency->pNode->NodeIdentifierString(),
                                     pDependency->requestId);

                        // Move the node to the ready queue
                        LightweightDoublyLinkedListNode* pDeferred = m_deferredNodes.FindByValue(pDependency);
                        m_deferredNodes.RemoveNode(pDeferred);

                        m_pReadyQueueLock->Lock();
                        m_readyNodes.InsertToTail(pDeferred);
                        m_pReadyQueueLock->Unlock();
                    }

                    pList->RemoveNode(pNode);
                    // No longer need the linked list entry. The data is associated finally with m_deferrednodes
                    CAMX_FREE(pNode);
                }
            }
            pNode = pNext;
        }

        // If list empty, delete it and the hashmap entry.
        if (0 == pList->NumNodes())
        {
            CAMX_DELETE pList;
            pList = NULL;
            m_pDependencyMap->Remove(pMapKey);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::RemoveAllDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::RemoveAllDependencies(
    Dependency* pDependency)
{
    m_pDeferredQueueLock->Lock();

    // Remove property dependency
    for (UINT i = 0; i < pDependency->propertyCount; i++)
    {
        INT64         offset   = (TRUE == pDependency->negate[i]) ?
                                 (-1 * static_cast<INT64>(pDependency->offsets[i])) : pDependency->offsets[i];
        INT64         intReq   = static_cast<INT64>(pDependency->requestId);
        UINT64        request  = (intReq - offset <= 0) ? FirstValidRequestId : (intReq - offset);
        DependencyKey mapKey   = { request, pDependency->pipelineIds[i], pDependency->properties[i], NULL, NULL };
        UpdateOrRemoveDependency(&mapKey, pDependency);
    }

    // Remove fence dependency
    for (UINT i = 0; i < pDependency->fenceCount; i++)
    {
        DependencyKey mapKey = { 0, 0, PropertyIDInvalid, pDependency->phFences[i], NULL };
        UpdateOrRemoveDependency(&mapKey, pDependency);
    }

    // Remove chi fence dependency
    for (UINT i = 0; i < pDependency->chiFenceCount; i++)
    {
        DependencyKey  mapKey = { 0, 0, PropertyIDInvalid, NULL, pDependency->pChiFences[i] };
        UpdateOrRemoveDependency(&mapKey, pDependency);
    }

    // Set process sequence id to -1 to indicate preemption
    pDependency->processSequenceId  = -1;
    pDependency->bindIOBuffers      = FALSE;
    m_pDeferredQueueLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DeferredRequestQueue::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID DeferredRequestQueue::Flush()
{
    CAMX_LOG_VERBOSE(CamxLogGroupCore, "DRQ Flush starting for session");
    m_pDeferredQueueLock->Lock();

    // Empty the hashmap
    if (NULL != m_pDependencyMap)
    {
        m_pDependencyMap->Foreach(FreeDependencyMapListData, TRUE);
    }

    // Free all remaining entries in m_errorRequests, referenced data are only numbers, not pointers
    LightweightDoublyLinkedListNode* pNode = m_errorRequests.Head();
    while (NULL != pNode)
    {
        LightweightDoublyLinkedListNode* pNext = LightweightDoublyLinkedList::NextNode(pNode);
        m_errorRequests.RemoveNode(pNode);
        if (NULL != pNode->pData)
        {
            CAMX_FREE(pNode->pData);
            pNode->pData = NULL;
        }
        CAMX_FREE(pNode);

        pNode = pNext;
    }

    CamxAtomicStoreU(&m_numErrorRequests, 0);

    // Free all remaining entries in m_deferredNodes, and the data they reference
    m_deferredNodes.FreeAllNodesAndTheirClientData();

    // Free all the entries in ready queue
    m_pReadyQueueLock->Lock();
    m_readyNodes.FreeAllNodesAndTheirClientData();
    m_pReadyQueueLock->Unlock();

    m_pDeferredQueueLock->Unlock();
    CAMX_LOG_VERBOSE(CamxLogGroupCore, "DRQ Flush is done.");
}

CAMX_NAMESPACE_END
