////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chitargetbuffermanager.cpp
/// @brief CHX CHITargetBufferManager class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE CP006:  Need vector for returning list

#include "chitargetbuffermanager.h"

// NOWHINE FILE CP006:  Need whiner update: std::string allowed in exceptional cases

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT32 ListSize = 3;
static const UINT32 WAIT_TARGET_BUFFER = 3000;
#define MAX_TBM_REF_COUNT 15 ///< Maximum nuber of references for a TBM

typedef INT32 CSLFence;

/// @brief Chi fence state
enum ChiFenceState
{
    ChiFenceIntialized = 0,         ///< Fence initialized
    ChiFenceSuccess,                ///< Fence signaled with success
    ChiFenceFailed,                 ///< Fence signaled with failure
    ChiFenceInvalid                 ///< Fence invalid state
};

/// @brief Chi fence structure
struct ChiFence
{
    CHIFENCEHANDLE  hChiFence;      ///< Handle to this Chi fence instance
    CHIFENCETYPE    type;           ///< Chi fence type
    INT             aRefCount;      ///< Reference count
    CSLFence        hFence;         ///< CSL fence representing this Chi fence
    ChiFenceState   resultState;    ///< Fence signal result state
    union
    {
        UINT64      EGLSync;        ///< EGL sync object (need to cast to EGLSyncKHR)
        INT         nativeFenceFD;  ///< Native fence file descriptor
    };
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHITargetBufferManager::CHITargetBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHITargetBufferManager::CHITargetBufferManager()
{
    ChxUtils::Memset(m_pChiStreams,     0, (sizeof(ChiStream*) * MaxChiStreams));
    ChxUtils::Memset(m_pBufferManagers, 0, (sizeof(CHIBufferManager*) * MaxChiStreams));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHITargetBufferManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHITargetBufferManager* CHITargetBufferManager::Create(
    ChiTargetBufferManagerCreateData* pCreateData)
{
    CDKResult           result          = CDKResultSuccess;
    CHITargetBufferManager*    pTargetBuffer   = NULL;

    if (NULL != pCreateData)
    {
        pTargetBuffer = CHX_NEW CHITargetBufferManager;
        if (NULL != pTargetBuffer)
        {
            result = pTargetBuffer->Initialize(pCreateData);

            if (CDKResultSuccess != result)
            {
                pTargetBuffer->Destroy();
                pTargetBuffer = NULL;
            }
        }
        else
        {
            CHX_LOG_ERROR("pTargetBuffer is NULL!");
        }
    }
    else
    {
        CHX_LOG_ERROR("pCreateData is NULL!");
    }

    return pTargetBuffer;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHITargetBufferManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHITargetBufferManager::Destroy()
{
    LightweightDoublyLinkedList*     pLists[ListSize] = {m_pProducerList, m_pConsumerList, m_pImportList};
    LightweightDoublyLinkedListNode* pNode            = NULL;
    UINT32                           seqId            = INVALIDSEQUENCEID;

    for (UINT i = 0; i < ListSize; i++)
    {
        if (NULL != pLists[i])
        {
            pNode = pLists[i]->Head();

            while (NULL != pNode)
            {
                seqId = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData)->seqId;
                pNode = pLists[i]->NextNode(pNode);

                if (INVALIDSEQUENCEID != seqId)
                {
                    RemoveTargetBufferPrivate(seqId);
                }
            }
        }
    }

    if (NULL != m_pProducerList)
    {
        CHX_DELETE m_pProducerList;
        m_pProducerList = NULL;
    }

    if (NULL != m_pConsumerList)
    {
        CHX_DELETE m_pConsumerList;
        m_pConsumerList = NULL;
    }

    if (NULL != m_pImportList)
    {
        CHX_DELETE m_pImportList;
        m_pImportList = NULL;
    }

    for (UINT i = 0; i < m_numOfTargets; i++)
    {
        if (NULL != m_pBufferManagers[i])
        {
            m_pBufferManagers[i]->Destroy();
            m_pBufferManagers[i] = NULL;
        }
    }

    if (NULL != m_pLock)
    {
        m_pLock->Destroy();
        m_pLock = NULL;
    }

    if (NULL != m_pCondition)
    {
        m_pCondition->Destroy();
        m_pCondition = NULL;
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHITargetBufferManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHITargetBufferManager::Initialize(
    ChiTargetBufferManagerCreateData* pCreateData)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pCreateData) ||
        (NULL == pCreateData->pTargetBufferName) ||
        (0 == pCreateData->numOfMetadataBuffers &&
         0 == pCreateData->numOfInternalStreamBuffers &&
         0 == pCreateData->numOfExternalStreamBuffers) ||
        (MaxChiStreams < pCreateData->numOfMetadataBuffers +
                         pCreateData->numOfInternalStreamBuffers +
                         pCreateData->numOfExternalStreamBuffers))
    {
        CHX_LOG_ERROR("Invalid args! pCreateData=%p, pTargetBufferName=%p",
                      pCreateData,
                      (pCreateData != NULL) ? pCreateData->pTargetBufferName : NULL);
        result = CDKResultEInvalidArg;
    }

    // Initialize class members
    if (CDKResultSuccess == result)
    {
        CdkUtils::StrLCpy(m_targetBufferName, pCreateData->pTargetBufferName, sizeof(m_targetBufferName));
        m_pProducerList             = CHX_NEW LightweightDoublyLinkedList();
        m_pConsumerList             = CHX_NEW LightweightDoublyLinkedList();
        m_pImportList               = CHX_NEW LightweightDoublyLinkedList();
        m_pLock                     = Mutex::Create();
        m_pCondition                = Condition::Create();
        m_numOfTargets              = 0;
        m_minNumOfBuffers           = 0;
        m_maxNumOfBuffers           = 0;
        m_isChiFenceEnabled         = pCreateData->isChiFenceEnabled;

        if ((NULL == m_pProducerList) ||
            (NULL == m_pConsumerList) ||
            (NULL == m_pImportList) ||
            (NULL == m_pLock) ||
            (NULL == m_pCondition))
        {
            CHX_LOG_ERROR("[%s] Error creating resources! "
                "m_pProducerList=%p, m_pConsumerList=%p, m_pImportList=%p, m_pLock=%p, m_pCondition=%p",
                m_targetBufferName, m_pProducerList, m_pConsumerList, m_pImportList, m_pLock, m_pCondition);
            result = CDKResultEResource;
        }
    }

    // Initialize metadata target related class members
    if (CDKResultSuccess == result)
    {
        m_pMetadataManager = pCreateData->pChiMetadataManager;
        for (UINT i = 0; i < pCreateData->numOfMetadataBuffers; i++)
        {
            // Make sure no duplicate target ids
            if ((InvalidIndex == LookupTargetIndex(pCreateData->metadataIds[i])) &&
                (NULL != m_pMetadataManager))
            {
                m_targetIds[m_numOfTargets]  = pCreateData->metadataIds[i];
                m_targetType[m_numOfTargets] = ChiTargetType::Metadata;
                m_minNumOfBuffers            = pCreateData->minMetaBufferCount;
                m_numOfTargets++;
            }
            else
            {
                CHX_LOG_ERROR("[%s] Invalid args! metadataIds[%d]=%" PRIu64 ", m_pMetadataManager=%p",
                              m_targetBufferName,
                              i,
                              static_cast<UINT64>(pCreateData->metadataIds[i]),
                              m_pMetadataManager);
                result = CDKResultEInvalidArg;
            }
        }
    }

    // Initialize internal stream buffer target related class members
    if (CDKResultSuccess == result)
    {
        for (UINT i = 0; i < pCreateData->numOfInternalStreamBuffers; i++)
        {
            // Make sure no duplicate target ids
            if (InvalidIndex != LookupTargetIndex(pCreateData->internalStreamIds[i]))
            {
                CHX_LOG_ERROR("[%s] Invalid args! internalStreamIds[%d]=%" PRIu64 "",
                    m_targetBufferName, i, static_cast<UINT64>(pCreateData->internalStreamIds[i]));
                result = CDKResultEInvalidArg;
                break;
            }

            m_pBufferManagers[m_numOfTargets] = CHIBufferManager::Create(pCreateData->pBufferManagerNames[i],
                                                                         pCreateData->pBufferManagerCreateData[i]);

            if (NULL == m_pBufferManagers[m_numOfTargets])
            {
                CHX_LOG_ERROR("[%s] Error creating resources! m_pBufferManagers[%d]=0x0", m_targetBufferName, m_numOfTargets);
                result = CDKResultEResource;
                break;
            }

            // Every buffer manager should have the same maxBufferCount within one target buffer object
            if ((m_maxNumOfBuffers != 0) &&
                (pCreateData->pBufferManagerCreateData[i]->maxBufferCount != m_maxNumOfBuffers))
            {
                CHX_LOG_ERROR("[%s] maxBufferCount mismatch! m_maxNumOfBuffers=%d maxBufferCount=%d",
                    m_targetBufferName, m_maxNumOfBuffers, pCreateData->pBufferManagerCreateData[i]->maxBufferCount);
                result = CDKResultEInvalidArg;
                break;
            }

            m_pChiStreams[m_numOfTargets]   = pCreateData->pBufferManagerCreateData[i]->pChiStream;
            m_minNumOfBuffers               = pCreateData->pBufferManagerCreateData[i]->minBufferCount;
            m_maxNumOfBuffers               = pCreateData->pBufferManagerCreateData[i]->maxBufferCount;

            CHX_LOG_INFO("[%s]m_maxNumOfBuffers=%d maxBufferCount=%d",
                          m_targetBufferName, m_maxNumOfBuffers, pCreateData->pBufferManagerCreateData[i]->maxBufferCount);
            m_targetIds[m_numOfTargets]     = pCreateData->internalStreamIds[i];
            m_targetType[m_numOfTargets]    = ChiTargetType::InternalBuffer;
            m_numOfTargets++;
        }
    }

    // Initialize external stream buffer target related members
    if (CDKResultSuccess == result)
    {
        for (UINT i = 0; i < pCreateData->numOfExternalStreamBuffers; i++)
        {
            // Make sure no duplicate target ids
            if (InvalidIndex != LookupTargetIndex(pCreateData->externalStreamIds[i]))
            {
                CHX_LOG_ERROR("[%s] Invalid args! internalStreamIds[%d]=%" PRIu64 "",
                              m_targetBufferName,
                              i,
                              static_cast<UINT64>(pCreateData->externalStreamIds[i]));
                result = CDKResultEInvalidArg;
                break;
            }

            m_targetIds[m_numOfTargets]     = pCreateData->externalStreamIds[i];
            m_targetType[m_numOfTargets]    = ChiTargetType::ExternalBuffer;
            m_maxNumOfBuffers               = pCreateData->maxExternalBufferCount;
            m_minNumOfBuffers               = pCreateData->minExternalBufferCount;
            if (0 == m_minNumOfBuffers)
            {
                m_minNumOfBuffers = m_maxNumOfBuffers;
            }
            m_numOfTargets++;
        }
    }

    // m_maxNumOfBuffers should have been set if there's internal stream buffer target.
    // If the target buffer object is managing metadata or external stream buffer targets,
    // set m_maxNumOfBuffers to a default valud.
    if (0 == m_maxNumOfBuffers)
    {
        if (0 == m_minNumOfBuffers)
        {
            m_minNumOfBuffers = DefaultMaxNode;
        }
        m_maxNumOfBuffers = DefaultMaxNode;
    }

    if (m_minNumOfBuffers > m_maxNumOfBuffers)
    {
        m_maxNumOfBuffers = m_minNumOfBuffers;
    }

    CHX_LOG_INFO("[%s] m_minNumOfBuffers:%d, m_maxNumOfBuffers:%d",
                 m_targetBufferName, m_minNumOfBuffers, m_maxNumOfBuffers);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::SetupTargetBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHITARGETBUFFERINFOHANDLE CHITargetBufferManager::SetupTargetBuffer(
    UINT32 seqId)
{
    CDKResult                           result              = CDKResultSuccess;
    CHITARGETBUFFERINFOHANDLE           handle              = NULL;
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;

    m_pLock->Lock();

    // Setup target buffer should be done incrementally r.w.t. the seqId.
    // Calling this API with random order of seqId or setup target buffer on the same seqId more than once is not allowed.
    pNode = m_pProducerList->Tail();
    if (NULL != pNode)
    {
        ChiTargetBufferInfo* pTemp = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
        if (pTemp->seqId >= seqId)
        {
            CHX_LOG_ERROR("[%s] Setup seqId=%d failed, last seqId=%d", m_targetBufferName, seqId, pTemp->seqId);
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        // Before setting up the target buffer for this new sequence Id,
        // we try to recycle some target buffers from consumer list to save memory footprint.
        RecycleTargetBuffersFromConsumerList();

        if ((m_pProducerList->NumNodes() + m_pConsumerList->NumNodes() < DefaultMaxNode) ||
            (m_pProducerList->NumNodes() + m_pConsumerList->NumNodes() <= m_maxNumOfBuffers))
        {
            // Check import list if target buffer is imported for this sequence id.
            // Node should already present in m_pImportedList if imported.
            // Client calls ImportExternalTargetBuffer ahead of time.
            pNode = SearchList(seqId, SearchOption::SearchImportList);

            if (NULL != pNode)
            {
                pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
                m_pImportList->RemoveNode(pNode);
            }
            else
            {
                // Allocate a new node if target is not imported
                pNode = AllocateTargetBufferInfoNode();
                if (NULL != pNode)
                {
                    pTargetBufferInfo                = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
                    pTargetBufferInfo->seqId         = seqId;
                }
                else
                {
                    result = CDKResultENoMemory;
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("[%s] Max number of nodes reached %d! m_pProducerList size=%d, m_pConsumerList size=%d",
                m_targetBufferName, m_maxNumOfBuffers, m_pProducerList->NumNodes(), m_pConsumerList->NumNodes());
            result = CDKResultENoMore;
        }
    }


    // Populate pTargetBufferInfo and finalize the initialization
    if (CDKResultSuccess == result)
    {
        // Populate pTargetBufferInfo doesn't need protection as it is an independently node and only accessed
        // by the current thread. Besides, GetImageBufferInfo may also get blocked due to waiting for a free buffer.
        // Therefore, unlock the mutex to prevent deadlock.
        m_pLock->Unlock();

        pTargetBufferInfo->seqId    = seqId;
        pTargetBufferInfo->refCount = 1;

        for (UINT i = 0; i < m_numOfTargets; i++)
        {
            pTargetBufferInfo->targetStatus[i] = ChiTargetStatus::NotReady;

            if (m_targetType[i] == ChiTargetType::Metadata)
            {
                // If pMetadata is NOT NULL, it may have been populated by import.
                if (NULL == pTargetBufferInfo->pMetadata[i])
                {
                    pTargetBufferInfo->pMetadata[i] = m_pMetadataManager->Get(m_targetIds[i], seqId);
                }

                if (NULL == pTargetBufferInfo->pMetadata[i])
                {
                    CHX_LOG_ERROR("[%s] seqId=%d setup metadata failed!", m_targetBufferName, pTargetBufferInfo->seqId);
                    result = CDKResultEFailed;
                    break;
                }
            }
            else if (m_targetType[i] == ChiTargetType::InternalBuffer)
            {
                // An Internal stream buffer may have already been populated at this point.
                // For example, client import a framework raw buffer into this target on internal RDI target id.
                if (NULL == pTargetBufferInfo->chiStreamBuffer[i].bufferInfo.phBuffer)
                {
                    // NOWHINE CP036a: for const_cast<>
                    pTargetBufferInfo->chiStreamBuffer[i].pStream       = const_cast<CHISTREAM*>(m_pChiStreams[i]);
                    pTargetBufferInfo->chiStreamBuffer[i].size          = sizeof(CHISTREAMBUFFER);
                    pTargetBufferInfo->chiStreamBuffer[i].bufferInfo    = m_pBufferManagers[i]->GetImageBufferInfo();

                    pTargetBufferInfo->chiStreamBuffer[i].acquireFence.valid = FALSE;
                    pTargetBufferInfo->chiStreamBuffer[i].releaseFence.valid = FALSE;
                }

                if (NULL == pTargetBufferInfo->chiStreamBuffer[i].bufferInfo.phBuffer)
                {
                    CHX_LOG_ERROR("[%s] seqId=%d setup internal stream buffer failed!",
                        m_targetBufferName, pTargetBufferInfo->seqId);
                    result = CDKResultEFailed;
                    break;
                }
            }
            else if (m_targetType[i] == ChiTargetType::ExternalBuffer)
            {
                if (NULL == pTargetBufferInfo->chiStreamBuffer[i].bufferInfo.phBuffer)
                {
                    CHX_LOG_ERROR("[%s] seqId=%d setup external stream buffer failed!",
                        m_targetBufferName, pTargetBufferInfo->seqId);
                    result = CDKResultEFailed;
                    break;
                }
            }

            // Setup release fence for ChiStreamBuffer
            if ((m_isChiFenceEnabled == TRUE) &&
                ((m_targetType[i] == ChiTargetType::InternalBuffer) ||
                 (m_targetType[i] == ChiTargetType::ExternalBuffer)) &&
                 (pTargetBufferInfo->chiStreamBuffer[i].releaseFence.valid == FALSE))
            {
                // Setup release fence and leave acquire fence as invalid. Acquire fence will be setup
                // whenever a client needs to consume this target buffer (i.e. in GetTargetBuffers API).
                CHIFENCEINFO*           pReleaseFence  = &pTargetBufferInfo->chiStreamBuffer[i].releaseFence;
                CHIFENCECREATEPARAMS    chiFenceParams =
                {
                    sizeof(CHIFENCECREATEPARAMS),
                    ChiFenceTypeInternal,
                    m_targetBufferName
                };

                result = ExtensionModule::GetInstance()->CreateChiFence(&chiFenceParams, &pReleaseFence->hChiFence);
                CSLFence hNewFence = reinterpret_cast<ChiFence*>(pReleaseFence->hChiFence)->hFence;


                if (CDKResultSuccess == result)
                {
                    pReleaseFence->valid = TRUE;
                    pReleaseFence->type = ChiFenceTypeInternal;

                    CHIFENCEINFO* pAcquireFence = &pTargetBufferInfo->chiStreamBuffer[i].acquireFence;
                    ChxUtils::Memcpy(&pTargetBufferInfo->chiStreamBuffer[i].acquireFence,
                                     &pTargetBufferInfo->chiStreamBuffer[i].releaseFence,
                                     sizeof(CHIFENCEINFO));

                    CSLFence hAcqNewFence = reinterpret_cast<ChiFence*>(pAcquireFence->hChiFence)->hFence;

                    CHX_LOG_INFO("CreatePrivateFence TBM::%s,\t fence: %p(%d), acqFence:%d reqId: %d, streamBuffer:%p,"
                                 "ImgBuf: %p",
                                 m_targetBufferName,
                                 pReleaseFence->hChiFence,
                                 hNewFence,
                                 hAcqNewFence,
                                 seqId,
                                 &pTargetBufferInfo->chiStreamBuffer[i],
                                 pTargetBufferInfo->chiStreamBuffer[i].bufferInfo.phBuffer);
                }
                else
                {
                    CHX_LOG_ERROR("[%s] seqId=%d create release fence failed!",
                                  m_targetBufferName, pTargetBufferInfo->seqId);
                    break;
                }
            }

            CHX_LOG_INFO("[%s]_TBM Info: MinCnt:%d MaxCnt:%d ProdQ:%d ConsumerQ:%d"
                "[%s]_TargetBuffer Info: seqId:%d pTargetBufferInfo:%p targetIndex:%d type:%d"
                "[%s]_Buffer Info: metadata:%p phBuffer:%p releaseFence:%p(%d) %d refCount=%u",
                m_targetBufferName, m_minNumOfBuffers, m_maxNumOfBuffers,
                m_pProducerList->NumNodes(), m_pConsumerList->NumNodes(),
                m_targetBufferName,
                pTargetBufferInfo->seqId, pTargetBufferInfo, i, m_targetType[i],
                m_targetBufferName,
                pTargetBufferInfo->pMetadata[i],
                pTargetBufferInfo->chiStreamBuffer[i].bufferInfo.phBuffer,
                pTargetBufferInfo->chiStreamBuffer[i].releaseFence.hChiFence,
                pTargetBufferInfo->chiStreamBuffer[i].releaseFence.valid == TRUE ?
                    reinterpret_cast<ChiFence*>(pTargetBufferInfo->chiStreamBuffer[i].releaseFence.hChiFence)->hFence : 0,
                pTargetBufferInfo->chiStreamBuffer[i].releaseFence.valid,
                pTargetBufferInfo->refCount);
        }

        m_pLock->Lock();

        // Insert the node to producer list regardless setup successful or not.
        // RemoveTargetBufferPrivate will take place to clean up references and remove node on failure case.
        if (NULL != pNode)
        {
            m_pProducerList->InsertToTail(pNode);
        }

        if (CDKResultSuccess == result)
        {
            pTargetBufferInfo->pCHITargetBufferManager = this;
            handle = pTargetBufferInfo;
        }
        else
        {
            RemoveTargetBufferPrivate(seqId);
        }

        if (CHX_IS_VERBOSE_ENABLED())
        {
            CHAR bufferManagerName[MaxStringLength64];

            CdkUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "C_%s", m_targetBufferName);
            ATRACE_INT(bufferManagerName, m_pConsumerList->NumNodes());

            CdkUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "P_%s", m_targetBufferName);
            ATRACE_INT(bufferManagerName, m_pProducerList->NumNodes());
        }

    }

    m_pLock->Unlock();

    return handle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::GetTarget
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CHITargetBufferManager::GetTarget(
    CHITARGETBUFFERINFOHANDLE   hTargetBufferInfo,
    UINT64                      targetId)
{
    ChiTargetBufferInfo*    pTargetBufferInfo   = NULL;
    UINT32                  targetIdx           = LookupTargetIndex(targetId);
    VOID*                   pTarget             = NULL;

    if ((NULL != hTargetBufferInfo) && (InvalidIndex != targetIdx))
    {
        pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(hTargetBufferInfo);

        if (m_targetType[targetIdx] == ChiTargetType::Metadata)
        {
            if (NULL != pTargetBufferInfo->pMetadata[targetIdx])
            {
                pTarget = pTargetBufferInfo->pMetadata[targetIdx];
            }
            else
            {
                CHX_LOG_ERROR("[%s] No metadata for hTargetBufferInfo=%p, targetIdx=%d",
                              m_targetBufferName, hTargetBufferInfo, targetIdx);
            }
        }
        else
        {
            if (NULL != pTargetBufferInfo->chiStreamBuffer[targetIdx].bufferInfo.phBuffer)
            {
                pTarget = &pTargetBufferInfo->chiStreamBuffer[targetIdx];
            }
            else
            {
                CHX_LOG_ERROR("[%s] No ChiStreamBuffer for hTargetBufferInfo=%p, targetIdx=%d",
                              m_targetBufferName, hTargetBufferInfo, targetIdx);
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("[%s] hTargetBufferInfo=%p, targetIdx=%d", m_targetBufferName, hTargetBufferInfo, targetIdx);
    }

    return pTarget;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::UpdateTarget
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHITargetBufferManager::UpdateTarget(
    UINT32                     seqId,
    UINT64                     targetId,
    VOID*                      pSrcTarget,
    ChiTargetStatus            status,
    CHITARGETBUFFERINFOHANDLE* phTargetBufferInfo)
{
    CDKResult                           result              = CDKResultSuccess;
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;
    UINT32                              targetIdx           = LookupTargetIndex(targetId);

    if ((MaxChiStreams <= targetIdx) ||
        (MaxChiStreams <= m_numOfTargets))
    {
        CHX_LOG_ERROR("[%s] Invalid args! seqId=%d, pSrcTarget=%p, targetIdx=%d",
                      m_targetBufferName, seqId, pSrcTarget, targetIdx);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        m_pLock->Lock();

        pNode = SearchList(seqId, SearchOption::SearchProducerList);

        if (NULL != pNode)
        {
            pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);

            if (NULL != pSrcTarget)
            {
                if (m_targetType[targetIdx] == ChiTargetType::Metadata)
                {
                    ChiMetadata* pSrcMetadata = reinterpret_cast<ChiMetadata*>(pSrcTarget);
                    if (pSrcMetadata == pTargetBufferInfo->pMetadata[targetIdx])
                    {
                        pTargetBufferInfo->targetStatus[targetIdx] = status;

                        CHX_LOG("[%s] seqId=%d targetIdx=%d pMetadata=%p status updated to %d refCount=%u",
                            m_targetBufferName, seqId, targetIdx, pSrcMetadata, status, pTargetBufferInfo->refCount);
                    }
                    else
                    {
                        CHX_LOG_ERROR("[%s] UpdateMetadata failed! seqId=%d targetIdx=%d InternalMeta=%p InputMeta=%p",
                            m_targetBufferName, seqId, targetIdx, pTargetBufferInfo->pMetadata[targetIdx], pSrcMetadata);
                        result = CDKResultEInvalidArg;
                    }
                }
                else
                {
                    CHIBUFFERHANDLE hBuffer = reinterpret_cast<CHIBUFFERHANDLE>(pSrcTarget);
                    if (hBuffer == pTargetBufferInfo->chiStreamBuffer[targetIdx].bufferInfo.phBuffer)
                    {
                        pTargetBufferInfo->targetStatus[targetIdx] = status;

                        CHX_LOG("[%s] seqId=%d targetIdx=%d phBuffer=%p status updated to %d refCount=%u",
                                m_targetBufferName, seqId, targetIdx, hBuffer, status, pTargetBufferInfo->refCount);
                    }
                    else
                    {
                        CHX_LOG_ERROR("[%s] UpdateBuffer failed! seqId %d targetIdx %d Internal phBuffer=%p Input phBuffer=%p",
                                      m_targetBufferName, seqId, targetIdx,
                                      pTargetBufferInfo->chiStreamBuffer[targetIdx].bufferInfo.phBuffer, hBuffer);
                        result = CDKResultEInvalidArg;
                    }
                }
            }
            else
            {
                // If pSrcTarget is NULL, it is expected that target is in error state.
                if (ChiTargetStatus::Error == status)
                {
                    pTargetBufferInfo->targetStatus[targetIdx] = status;
                }
                else
                {
                    CHX_LOG_ERROR("[%s] Expect target error status, but received status=%d", m_targetBufferName, status);
                    result = CDKResultEInvalidArg;
                }
            }

            if (CDKResultSuccess == result)
            {
                TryMoveToConsumerList(seqId);
            }
        }
        else
        {
            CHX_LOG_WARN("[%s] Target buffer of seqId %d not found in m_pProducerList", m_targetBufferName, seqId);
        }

        if ((CDKResultSuccess == result) && (NULL != phTargetBufferInfo))
        {
            pNode   = SearchList(seqId, SearchOption::SearchConsumerList);

            if ((NULL != pNode) && (NULL != pNode->pData))
            {
                pTargetBufferInfo    = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
                *phTargetBufferInfo  = static_cast<CHITARGETBUFFERINFOHANDLE>(pTargetBufferInfo);

                if (NULL != *phTargetBufferInfo)
                {
                    pTargetBufferInfo->refCount++;
                    CHX_LOG("[%s] Get target buffer %d succeed, hTargetBufferInfo=%p ref =%d pTargetBufferInfo=%p",
                        m_targetBufferName, seqId, *phTargetBufferInfo, pTargetBufferInfo->refCount, pTargetBufferInfo);

                    // Setup acquire fence
                    for (UINT i = 0; i < m_numOfTargets; i++)
                    {
                        if ((m_isChiFenceEnabled == TRUE) &&
                            ((m_targetType[i] == ChiTargetType::InternalBuffer) ||
                             (m_targetType[i] == ChiTargetType::ExternalBuffer)) &&
                            (pTargetBufferInfo->chiStreamBuffer[i].releaseFence.valid == TRUE))
                        {
                            // Use the release fence as acquire fence. Once CamX pipeline produces the target, it will
                            // signal the release fence so that the consumer who is waiting on the same acquire fence
                            // gets unblocked.
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("[%s] Get buffer %d failed! Buffer Error", m_targetBufferName, seqId);
                    result = CDKResultENoSuch;
                }
            }
            else
            {
                CHX_LOG_ERROR("[%s] Get buffer %d failed! Buffer Error", m_targetBufferName, seqId);
                result = CDKResultENoSuch;
            }
        }
        m_pLock->Unlock();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::GetCHIHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHITargetBufferManager::GetCHIHandle(
    UINT32                      seqId,
    UINT64                      targetId,
    CHITARGETBUFFERINFOHANDLE*  phTargetBufferInfo)
{
    CDKResult                           result            = CDKResultSuccess;
    LightweightDoublyLinkedListNode*    pNode             = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo = NULL;
    UINT32                              targetIdx         = LookupTargetIndex(targetId);

    if ((MaxChiStreams <= targetIdx) || (MaxChiStreams <= m_numOfTargets))
    {
        CHX_LOG_ERROR("[%s] Invalid args! seqId=%d, targetIdx=%d",
            m_targetBufferName, seqId, targetIdx);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        m_pLock->Lock();

        pNode = SearchList(seqId, SearchOption::SearchProducerList);

        if (NULL != pNode)
        {
            pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);

            if (NULL != phTargetBufferInfo)
            {
                phTargetBufferInfo[0] = static_cast<CHITARGETBUFFERINFOHANDLE>(pTargetBufferInfo);
            }
        }
        else
        {
            result = CDKResultEFailed;
        }

        m_pLock->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::GetTargetBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHITargetBufferManager::GetTargetBuffers(
    UINT32                          startSeqId,
    UINT32                          numBuffers,
    CHITARGETBUFFERINFOHANDLE*      phTargetBufferInfos,
    BOOL                            waitFlag,
    ChiTargetBufferCallbackData*    pCallbackData)
{
    waitFlag = TRUE;
    CDKResult                           result              = CDKResultSuccess;
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    LightweightDoublyLinkedListNode*    pNext               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;
    BOOL                                foundMatch          = TRUE;
    UINT32                              seqId               = INVALIDSEQUENCEID;
    UINT32                              firstSeqId          = INVALIDSEQUENCEID;
    UINT32                              lastSeqId           = INVALIDSEQUENCEID;
    UINT32                              newStartSeqId       = INVALIDSEQUENCEID;

    m_pLock->Lock();

    // ProducerList     ConsumerList    firstSeqId      lastSeqId
    // empty            empty           invalid         invalid
    // empty            non-empty       consumerHead    consumerTail
    // non-empty        empty           producerHead    producerTail
    // non-empty        non-empty       consumerHead    producerTail
    if (NULL != m_pConsumerList->Head())
    {
        firstSeqId = reinterpret_cast<ChiTargetBufferInfo*>(m_pConsumerList->Head()->pData)->seqId;
    }
    else if (NULL != m_pProducerList->Head())
    {
        firstSeqId = reinterpret_cast<ChiTargetBufferInfo*>(m_pProducerList->Head()->pData)->seqId;
    }

    if (NULL != m_pProducerList->Tail())
    {
        lastSeqId = reinterpret_cast<ChiTargetBufferInfo*>(m_pProducerList->Tail()->pData)->seqId;
    }
    else if (NULL != m_pConsumerList->Tail())
    {
        lastSeqId = reinterpret_cast<ChiTargetBufferInfo*>(m_pConsumerList->Tail()->pData)->seqId;
    }

    if ((NULL == phTargetBufferInfos) ||
        (numBuffers > m_maxNumOfBuffers) ||
        (INVALIDSEQUENCEID == firstSeqId) ||
        (INVALIDSEQUENCEID == lastSeqId))
    {
        CHX_LOG_ERROR("[%s] Failed to get target buffers! startSeqId=%d, numBuffers=%d, firstSeqId=%d, lastSeqId=%d",
                      m_targetBufferName, startSeqId, numBuffers, firstSeqId, lastSeqId);
        result = CDKResultEFailed;
    }

    if (CDKResultSuccess == result)
    {
        // First try to find exact matching buffers
        newStartSeqId = startSeqId;
        for (UINT i = 0; i < numBuffers; i++)
        {
            pNode = SearchList(startSeqId + i, SearchOption::SearchProducerAndConsumerList);
            if (NULL == pNode)
            {
                foundMatch = FALSE;
                break;
            }
        }

        // If can't find exact macthing buffers, re-caculate the startSeqId based on the current
        // status of the producer and consumer lists, and try to find N contigious buffers.
        if (FALSE == foundMatch)
        {
            newStartSeqId = startSeqId + 1;

            for (seqId = newStartSeqId; seqId <= lastSeqId; seqId++)
            {
                pNode = SearchList(seqId, SearchOption::SearchProducerAndConsumerList);
                if (NULL == pNode)
                {
                    newStartSeqId = seqId + 1;
                }
                else if ((seqId - newStartSeqId + 1) == numBuffers)
                {
                    foundMatch = TRUE;
                    break;
                }
            }
        }

        if (FALSE == foundMatch)
        {
            CHX_LOG_ERROR("[%s] Failed to get target buffers! startSeqId=%d, numBuffers=%d, firstSeqId=%d, lastSeqId=%d",
                          m_targetBufferName, startSeqId, numBuffers, firstSeqId, lastSeqId);
            result = CDKResultEFailed;
        }
    }

    if (CDKResultSuccess == result)
    {
        // Now ready to get target buffers

        if (startSeqId == newStartSeqId)
        {
            CHX_LOG_INFO("[%s] Get %d target buffer(s) with start seqId %d (Exact match)",
                         m_targetBufferName, numBuffers, startSeqId);
        }
        else
        {
            CHX_LOG_WARN("[%s] Get %d target buffer(s) with start seqId %d (Suboptimal - Input startSeqId %d)",
                         m_targetBufferName, numBuffers, newStartSeqId, startSeqId);
        }

        // First put reference on all target buffers
        for (UINT i = 0; i < numBuffers; i++)
        {
            seqId               = newStartSeqId + i;
            pNode               = SearchList(seqId, SearchOption::SearchProducerAndConsumerList);
            pTargetBufferInfo   = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
            pTargetBufferInfo->refCount++;

            std::string targetBufferName = m_targetBufferName;
            if ((targetBufferName.find("Blend") != std::string::npos) &&
                (TRUE == m_isChiFenceEnabled))
            {
                waitFlag = FALSE;
                CHX_LOG_INFO("[%s] waitflag =%d", m_targetBufferName, waitFlag);
            }

            // Setup acquire fence
            for (UINT i = 0; i < m_numOfTargets; i++)
            {
                if ((m_isChiFenceEnabled == TRUE) &&
                    ((m_targetType[i] == ChiTargetType::InternalBuffer) ||
                     (m_targetType[i] == ChiTargetType::ExternalBuffer)) &&
                    (pTargetBufferInfo->chiStreamBuffer[i].releaseFence.valid == TRUE))
                {
                    // Use the release fence as acquire fence. Once CamX pipeline produces the target, it will
                    // signal the release fence so that the consumer who is waiting on the same acquire fence
                    // gets unblocked.
                }
                else
                {
                    // If the target is metadata or the release fence is invalid, we have to block this call
                    // and return target buffer handles only when they are ready.
                    waitFlag = TRUE;
                }
            }
        }

        if (TRUE == waitFlag && NULL == pCallbackData)
        {
            // Wait for the last seqId to be ready
            seqId = newStartSeqId + numBuffers - 1;
            while ((NULL == SearchList(seqId, SearchOption::SearchConsumerList)) &&
                   (NULL != SearchList(seqId, SearchOption::SearchProducerList)))
            {
                CHX_LOG("[%s] Wait for target buffer of seqId %d start..", m_targetBufferName, seqId);
                result = m_pCondition->TimedWait(m_pLock->GetNativeHandle(), WAIT_TARGET_BUFFER);

                if (result != CDKResultSuccess)
                {
                    CHX_LOG_ERROR("[%s] Wait for target buffer of seqId %d timedout!", m_targetBufferName, seqId);
                    break;
                }
                else
                {
                    // Moving the log in else condition as in timeout scenario if the consumer list is empty then it will crash
                    CHX_LOG("[%s] Wait for target buffer of seqId %d end.. result=%d lastReadySeqId=%d",
                            m_targetBufferName, seqId, result,
                            reinterpret_cast<ChiTargetBufferInfo*>(m_pConsumerList->Tail()->pData)->seqId);
                }

                CHX_LOG("[%s] Wait for target buffer of seqId %d end.. result=%d lastReadySeqId=%d",
                    m_targetBufferName, seqId, result,
                    reinterpret_cast<ChiTargetBufferInfo*>(m_pConsumerList->Tail()->pData)->seqId);
            }
        }

        if (CDKResultSuccess == result)
        {
            // Grab all target buffers
            for (UINT i = 0; i < numBuffers; i++)
            {
                seqId   = newStartSeqId + i;
                if (waitFlag == TRUE)
                {
                    pNode = SearchList(seqId, SearchOption::SearchConsumerList);
                }
                else
                {
                    pNode = SearchList(seqId, SearchOption::SearchProducerList);
                    if (NULL == pNode)
                    {
                        pNode = SearchList(seqId, SearchOption::SearchConsumerList);
                    }
                }

                if (NULL != pNode)
                {
                    pTargetBufferInfo       = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
                    phTargetBufferInfos[i]  = static_cast<CHITARGETBUFFERINFOHANDLE>(pTargetBufferInfo);
                    CHX_LOG("[%s] Get target buffer %d succeed, hTargetBufferInfo=%p",
                        m_targetBufferName, seqId, phTargetBufferInfos[i]);
                }
                else
                {
                    pNode = SearchList(seqId, SearchOption::SearchProducerList);
                    {
                        if (NULL != pNode)
                        {
                            if (NULL != pCallbackData)
                            {
                                pTargetBufferInfo       = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
                                phTargetBufferInfos[i]  = NULL;

                                ChxUtils::Memcpy(&pTargetBufferInfo->callbackData, pCallbackData,
                                    sizeof(ChiTargetBufferCallbackData));
                                CHX_LOG_INFO("[%s] Registered callback for seqId %d", m_targetBufferName, seqId);
                            }
                            else
                            {
                                // Should not get here
                                CHX_LOG_ERROR("[%s] SeqId %d in Producer list but no callback provided", m_targetBufferName,
                                seqId);
                            }
                        }
                        else
                        {
                            CHX_LOG_ERROR("[%s] Get target buffer %d failed! Not in producer or consumer list",
                                m_targetBufferName, seqId);
                            result = CDKResultENoSuch;
                        }
                    }
                }
            }
        }

        if (result != CDKResultSuccess)
        {
            // clean up
            for (UINT i = 0; i < numBuffers; i++)
            {
                seqId   = newStartSeqId + i;
                pNode   = SearchList(seqId, SearchOption::SearchProducerAndConsumerList);

                if (NULL != pNode)
                {
                    pTargetBufferInfo   = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
                    pTargetBufferInfo->refCount--;
                }

                phTargetBufferInfos[i] = NULL;
            }
        }
    }

    m_pLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::ReleaseTargetBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHITargetBufferManager::ReleaseTargetBuffer(
    CHITARGETBUFFERINFOHANDLE   hTargetBufferInfo)
{
    CDKResult               result              = CDKResultSuccess;
    ChiTargetBufferInfo*    pTargetBufferInfo   = NULL;
    UINT                    refCount            = MAX_TBM_REF_COUNT;

    if (NULL != hTargetBufferInfo)
    {
        m_pLock->Lock();
        pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(hTargetBufferInfo);

        if (NULL != pTargetBufferInfo)
        {
            if (pTargetBufferInfo->refCount > 0)
            {
                pTargetBufferInfo->refCount--;
                CHX_LOG("[%s] Release reference to hTargetBufferInfo=%p seqId=%d refCount=%d",
                    m_targetBufferName, hTargetBufferInfo, pTargetBufferInfo->seqId, pTargetBufferInfo->refCount);
            }
            else
            {
                CHX_LOG_WARN("[%s] hTargetBufferInfo=%p seqId=%d refCount is already 0!",
                              m_targetBufferName, hTargetBufferInfo, pTargetBufferInfo->seqId);
                result = CDKResultEFailed;
            }

            if (0 == pTargetBufferInfo->refCount)
            {
                // If Consumer queue has more than requested buffer start shrinking the count
                CHX_LOG_INFO("[%s]_TBM Info: MinCnt:%d MaxCnt:%d ProdQ:%d ConsumerQ:%d"
                             "[%s]_TargetBuffer Info: seqId:%d pTargetBufferInfo:%p targetIndex:%d type:%d"
                             "[%s]_Buffer Info: metadata:%p phBuffer:%p releaseFence:%p(%d) %d",
                             m_targetBufferName, m_minNumOfBuffers, m_maxNumOfBuffers,
                             m_pProducerList->NumNodes(), m_pConsumerList->NumNodes(),
                             m_targetBufferName,
                             pTargetBufferInfo->seqId, pTargetBufferInfo, 0, m_targetType[0],
                             m_targetBufferName,
                             pTargetBufferInfo->pMetadata[0],
                             pTargetBufferInfo->chiStreamBuffer[0].bufferInfo.phBuffer,
                             pTargetBufferInfo->chiStreamBuffer[0].releaseFence.hChiFence,
                             pTargetBufferInfo->chiStreamBuffer[0].releaseFence.valid == TRUE ?
                             reinterpret_cast<ChiFence*>(pTargetBufferInfo->chiStreamBuffer[0].releaseFence.hChiFence)->
                             hFence : 0,
                             pTargetBufferInfo->chiStreamBuffer[0].releaseFence.valid);
            }
            refCount = pTargetBufferInfo->refCount;
        }
        m_pLock->Unlock();

        // If Consumer queue has more than requested buffer start shrinking the count
        if (0 == refCount)
        {
            RecycleTargetBuffersFromConsumerList();
        }
    }
    else
    {
        CHX_LOG_ERROR("[%s] hTargetBufferInfo is NULL", m_targetBufferName);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::RemoveCallbackFromTarget
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHITargetBufferManager::RemoveCallbackFromTarget(
    UINT32 seqId)
{
    CDKResult                           result              = CDKResultSuccess;
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;
    m_pLock->Lock();

    pNode = SearchList(seqId, SearchOption::SearchProducerList);
    if (NULL == pNode)
    {
        pNode = SearchList(seqId, SearchOption::SearchConsumerList);
    }

    if (NULL != pNode)
    {
        pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
        ChxUtils::Memset(&pTargetBufferInfo->callbackData, 0, sizeof(ChiTargetBufferCallbackData));
    }
    m_pLock->Unlock();

 }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::ImportExternalTargetBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHITargetBufferManager::ImportExternalTargetBuffer(
    UINT32              seqId,
    UINT64              targetId,
    VOID*               pTarget)
{
    // This API imports an external buffer (e.g fwk buffer) into the target buffer objcet.
    // It creates a node in m_pImportList, which will be later moved m_pProducerList and
    // finalize initlaization when SetupTargetBuffer is called.

    /// The API was originally designed for importing external framework buffers only
    /// so that the reference count of the buffer is not a concern. However, the API is
    /// extended to allow importing internal target from one TBM to another. Hence the
    /// reference counting should be properly taken caren of.

    CDKResult                           result              = CDKResultSuccess;
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;
    UINT32                              targetIdx           = LookupTargetIndex(targetId);

    if ((NULL == pTarget) || (InvalidIndex == targetIdx))
    {
        CHX_LOG_ERROR("[%s] Importing external buffer failed! seqId=%d, pTarget=%p, targetIdx=%d",
                      m_targetBufferName, seqId, pTarget, targetIdx);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        m_pLock->Lock();

        pNode = SearchList(seqId, SearchOption::SearchImportList);

        if (NULL == pNode)
        {
            // Create a node and insert to m_pImportList
            pNode = AllocateTargetBufferInfoNode();
            if (NULL != pNode)
            {
                pTargetBufferInfo           = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
                pTargetBufferInfo->seqId    = seqId;
                m_pImportList->InsertToTail(pNode);
            }
            else
            {
                result = CDKResultENoMemory;
            }
        }
        else
        {
            // Node of this seqId already presents in m_pImportList (ImportExternalBuffer has been called on the same seqId).
            pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
        }

        if (CDKResultSuccess == result)
        {
            pTargetBufferInfo->isImported = TRUE;

            if (m_targetType[targetIdx] == ChiTargetType::Metadata)
            {
                // Importing metadata
                if (NULL == pTargetBufferInfo->pMetadata[targetIdx])
                {
                    pTargetBufferInfo->pMetadata[targetIdx] = reinterpret_cast<ChiMetadata*>(pTarget);
                }
                else
                {
                    CHX_LOG_ERROR("Meta (pMetadata=%p, seqId=%d) has already been imported!",
                        pTargetBufferInfo->pMetadata[targetIdx],
                        seqId);

                    result = CDKResultEFailed;
                }
            }
            else
            {
                // Importing ChiStreamBuffer
                CHISTREAMBUFFER* pChiStreamBuffer = reinterpret_cast<CHISTREAMBUFFER*>(pTarget);

                // There should be no ChiStreamBuffer for this stream of this seqId.
                // Importing more than once on the same stream and seqId is not allowed.
                if ((NULL == pTargetBufferInfo->chiStreamBuffer[targetIdx].bufferInfo.phBuffer) &&
                    (NULL != pChiStreamBuffer->bufferInfo.phBuffer))
                {
                    ChxUtils::Memcpy(&pTargetBufferInfo->chiStreamBuffer[targetIdx], pChiStreamBuffer, sizeof(CHISTREAMBUFFER));

                    // Do not import ChiFence to avoid the fence being destroyed by this this TBM.
                    pTargetBufferInfo->chiStreamBuffer[targetIdx].releaseFence.valid = FALSE;
                }
                else
                {
                    CHX_LOG_ERROR("Buffer (phBuffer=%p, seqId=%d) has already been imported! To be imported phBuffer=%p",
                        pTargetBufferInfo->chiStreamBuffer[targetIdx].bufferInfo.phBuffer,
                        seqId,
                        pChiStreamBuffer->bufferInfo.phBuffer);
                    result = CDKResultEFailed;
                }
            }
        }

        if (CDKResultSuccess == result)
        {
            CHX_LOG("[%s] Import external target succeed for seqId=%d, targetIdx=%d, pTarget=%p",
                m_targetBufferName, seqId, targetIdx, pTarget);
        }
        else
        {
            CHX_LOG_ERROR("[%s] Import external target failed! seqId=%d, targetIdx=%d pTarget=%p",
                m_targetBufferName, seqId, targetIdx, pTarget);
        }

        m_pLock->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::RemoveTargetBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHITargetBufferManager::RemoveTargetBuffer(
    UINT32 seqId)
{
    CDKResult result = CDKResultSuccess;

    m_pLock->Lock();
    result = RemoveTargetBufferPrivate(seqId);
    m_pLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::GetFirstReadySequenceID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CHITargetBufferManager::GetFirstReadySequenceID()
{
    UINT32                              seqId               = INVALIDSEQUENCEID;
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;

    m_pLock->Lock();

    pNode = m_pConsumerList->Head();
    if (NULL != pNode)
    {
        pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
        seqId = pTargetBufferInfo->seqId;
    }

    m_pLock->Unlock();

    return seqId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::GetLastReadySequenceID
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CHITargetBufferManager::GetLastReadySequenceID()
{
    UINT32                              seqId               = INVALIDSEQUENCEID;
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;

    m_pLock->Lock();

    pNode = m_pConsumerList->Tail();
    if (NULL != pNode)
    {
        pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
        seqId = pTargetBufferInfo->seqId;
    }

    m_pLock->Unlock();

    return seqId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::GetTargetBufferStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiTargetStatus CHITargetBufferManager::GetTargetBufferStatus(
    ChiTargetBufferInfo* pTargetBufferInfo)
{
    // This API returns the status of the ChiTargetBufferInfo node.
    //
    // If one of the targets is in 'NotReady' status, pTargetBufferInfo is considered as not ready. This TBM expects
    // client to call UpdateTarget API with either 'Ready' or 'Error' status for the remaining targets.
    //
    // If all the targets' status have been updated but any of them is 'Error', then this target buffer node is
    // considered in error status.
    //
    // Otherwise, pTargetBufferInfo is ready.

    ChiTargetStatus status = ChiTargetStatus::Ready;

    if (NULL != pTargetBufferInfo)
    {
        for (UINT i = 0; i < m_numOfTargets; i++)
        {
            if (pTargetBufferInfo->targetStatus[i] == ChiTargetStatus::NotReady)
            {
                status = ChiTargetStatus::NotReady;
                break;
            }
            else if (pTargetBufferInfo->targetStatus[i] == ChiTargetStatus::Error)
            {
                status = ChiTargetStatus::Error;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("[%s] pTargetBufferInfo is NULL", m_targetBufferName);
        status = ChiTargetStatus::Error;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::GetTargetBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHITargetBufferManager* CHITargetBufferManager::GetTargetBufferManager(
    CHITARGETBUFFERINFOHANDLE   hTargetBufferInfo)
{
    CHITargetBufferManager* pCHITargetBufferManager = NULL;
    ChiTargetBufferInfo*    pTargetBufferInfo       = reinterpret_cast<ChiTargetBufferInfo*>(hTargetBufferInfo);

    if (NULL != pTargetBufferInfo)
    {
        pCHITargetBufferManager = pTargetBufferInfo->pCHITargetBufferManager;
    }
    else
    {
        CHX_LOG_ERROR("pTargetBufferInfo is NULL");
    }

    return pCHITargetBufferManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::GetSequenceId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CHITargetBufferManager::GetSequenceId(
    CHITARGETBUFFERINFOHANDLE   hTargetBufferInfo)
{
    UINT32                  sequenceId          = INVALIDSEQUENCEID;
    ChiTargetBufferInfo*    pTargetBufferInfo   = reinterpret_cast<ChiTargetBufferInfo*>(hTargetBufferInfo);

    if (NULL != pTargetBufferInfo)
    {
        sequenceId = pTargetBufferInfo->seqId;
    }
    else
    {
        CHX_LOG_ERROR("pTargetBufferInfo is NULL");
    }

    return sequenceId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::GetAllSequenceId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<UINT32> CHITargetBufferManager::GetAllSequenceId(
    SearchOption option)
{
    std::vector<UINT32> readyList;

    LightweightDoublyLinkedList*     pList             = NULL;
    LightweightDoublyLinkedListNode* pNode             = NULL;
    ChiTargetBufferInfo*             pTargetBufferInfo = NULL;

    if (SearchOption::SearchConsumerList == option)
    {
        pList = m_pConsumerList;
    }
    else if (SearchOption::SearchProducerList == option)
    {
        pList = m_pProducerList;
    }
    else
    {
        CHX_LOG_ERROR("Unsupported option %d", option);
    }

    m_pLock->Lock();

    if (NULL != pList)
    {
        pNode = pList->Head();

        while (NULL != pNode)
        {
            pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);

            if (INVALIDSEQUENCEID != pTargetBufferInfo->seqId)
            {
                readyList.push_back(pTargetBufferInfo->seqId);
            }
            pNode = pList->NextNode(pNode);
        }
    }

    m_pLock->Unlock();

    return readyList;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::AllocateTargetBufferInfoNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LightweightDoublyLinkedListNode* CHITargetBufferManager::AllocateTargetBufferInfoNode()
{
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;

    pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(CHX_CALLOC(sizeof(ChiTargetBufferInfo)));
    pNode             = reinterpret_cast<LightweightDoublyLinkedListNode*>(
                            CHX_CALLOC(sizeof(LightweightDoublyLinkedListNode)));

    if ((NULL != pTargetBufferInfo) && (NULL != pNode))
    {
        pNode->pData = pTargetBufferInfo;
    }
    else
    {
        CHX_LOG_ERROR("[%s] Out of memory! pTargetBufferInfo=%p, pNode=%p", m_targetBufferName, pTargetBufferInfo, pNode);

        if (NULL != pTargetBufferInfo)
        {
            CHX_FREE(pTargetBufferInfo);
            pTargetBufferInfo = NULL;
        }

        if (NULL != pNode)
        {
            CHX_FREE(pNode);
            pNode = NULL;
        }
    }

    return pNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::LookupTargetIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CHITargetBufferManager::LookupTargetIndex(
    UINT64 targetId)
{
    UINT32 index = InvalidIndex;

    for (UINT i =0; i < m_numOfTargets; i++)
    {
        if (m_targetIds[i] == targetId)
        {
            index = i;
            break;
        }
    }

    return index;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::TryMoveToConsumerList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHITargetBufferManager::TryMoveToConsumerList(
    UINT32 seqId)
{
    // This API moves the target buffer from producer to consumer list if ready,
    // or remove the target buffer if it is in error status.
    // Then notify client who is waiting for the target buffer.
    // Make sure m_pLock is aquired before calling this API.

    LightweightDoublyLinkedListNode*    pNode               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;
    BOOL                                isTargetBufferReady = TRUE;
    ChiTargetStatus                     status;

    pNode = SearchList(seqId, SearchOption::SearchProducerList);

    if (NULL != pNode)
    {
        pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);

        status = GetTargetBufferStatus(pTargetBufferInfo);

        // SetTargetBuffer adds a node with refCount=1 to the producer list, in normal case when
        // the node becomes ready, the refCount should remain 1. So we decrement the ref count and
        // move the node to consumer list.
        //
        // In some cases, client may remove the reference while the node is in producer list,
        // for example, when flush is triggered which removes all references so that
        // ref count is 0 when the node becomes ready. Hence remove this node instead of moving to consumer list.
        //
        // In case of target buffer error, remove the node.

        if ((ChiTargetStatus::Ready == status) && (0 < pTargetBufferInfo->refCount))
        {
            CHX_LOG("[%s] target buffer of seqId=%d is ready, move to consumer list. refCount=%u",
                    m_targetBufferName, seqId, pTargetBufferInfo->refCount);

            pTargetBufferInfo->refCount--;
            m_pProducerList->RemoveNode(pNode);
            m_pConsumerList->InsertToTail(pNode);

            // Signal the release fences
            for (UINT targetIdx = 0; targetIdx < m_numOfTargets; targetIdx++)
            {
                if ((m_isChiFenceEnabled == TRUE) &&
                    ((m_targetType[targetIdx] == ChiTargetType::InternalBuffer) ||
                    (m_targetType[targetIdx] == ChiTargetType::ExternalBuffer)) &&
                     (pTargetBufferInfo->chiStreamBuffer[targetIdx].releaseFence.valid == TRUE))
                {
                    CHIFENCEINFO* pReleaseFence = &pTargetBufferInfo->chiStreamBuffer[targetIdx].releaseFence;

                    CDKResult result;
                    ExtensionModule::GetInstance()->GetChiFenceStatus(pReleaseFence->hChiFence, &result);

                    CSLFence hNewFence = reinterpret_cast<ChiFence*>(pReleaseFence->hChiFence)->hFence;
                    CHX_LOG_INFO("Chifence status %d for fence %d", result, hNewFence);
                    if (CDKResultSuccess == result)
                    {
                        CHX_LOG_WARN("Fence already signaled TBM::%s,\t fence: %p(%d), reqId: %d, streamBuf: %p ImgBuf: %p",
                                     m_targetBufferName,
                                     pReleaseFence->hChiFence,
                                     hNewFence,
                                     seqId,
                                     &pTargetBufferInfo->chiStreamBuffer[targetIdx],
                                     pTargetBufferInfo->chiStreamBuffer[targetIdx].bufferInfo.phBuffer);
                    }
                    else
                    {
                        ExtensionModule::GetInstance()->SignalChiFence(pReleaseFence->hChiFence, CDKResultSuccess);
                        CHX_LOG_INFO("Signal Fence TBM::%s, fence: %p(%d), reqId: %d, streamBuf:%p ImgBuf: %p",
                                     m_targetBufferName,
                                     pTargetBufferInfo->chiStreamBuffer[targetIdx].releaseFence.hChiFence,
                                     hNewFence,
                                     seqId,
                                     &pTargetBufferInfo->chiStreamBuffer[targetIdx],
                                     pTargetBufferInfo->chiStreamBuffer[targetIdx].bufferInfo.phBuffer);
                    }

                }
            }

            m_pCondition->Signal();

            if (NULL != pTargetBufferInfo->callbackData.pCallback)
            {
                CHX_LOG_INFO("[%s] target buffer of seqId=%d is ready callback. refCount=%u",
                                m_targetBufferName, seqId, pTargetBufferInfo->refCount);
                CHITARGETBUFFERINFOHANDLE hBuffer = static_cast<CHITARGETBUFFERINFOHANDLE>(pTargetBufferInfo);
                pTargetBufferInfo->callbackData.pCallback(hBuffer, pTargetBufferInfo->callbackData.pPrivateData);
            }
        }
        else if ((ChiTargetStatus::Ready == status) && (0 == pTargetBufferInfo->refCount))
        {
            CHX_LOG("[%s] target buffer of seqId=%d is ready but refCt=0, remove it. refCount=%u",
                          m_targetBufferName, seqId, pTargetBufferInfo->refCount);

            m_pProducerList->RemoveNode(pNode);
            m_pConsumerList->InsertToTail(pNode);
            m_pCondition->Signal();
        }
        else if (ChiTargetStatus::Error == status)
        {
            CHX_LOG("[%s] target buffer of seqId=%d is in error status, remove it. refCount=%u",
                       m_targetBufferName, seqId, pTargetBufferInfo->refCount);
            pTargetBufferInfo->refCount--;
            m_pProducerList->RemoveNode(pNode);
            m_pConsumerList->InsertToTail(pNode);
            // RemoveTargetBufferPrivate(seqId);
            m_pCondition->Signal();
        }
    }
    else
    {
        CHX_LOG_ERROR("[%s] seqId %d not in m_pProducerList", m_targetBufferName, seqId);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::SearchList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LightweightDoublyLinkedListNode* CHITargetBufferManager::SearchList(
    UINT32          seqId,
    SearchOption    option)
{
    LightweightDoublyLinkedListNode*    pNodeReturn         = NULL;
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;

    if (SearchOption::SearchImportList == option)
    {
        pNode = m_pImportList->Head();
        while (NULL != pNode)
        {
            pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
            if (seqId == pTargetBufferInfo->seqId)
            {
                pNodeReturn = pNode;
                break;
            }
            pNode = m_pImportList->NextNode(pNode);
        }
    }
    else
    {
        if ((SearchOption::SearchProducerList == option) ||
            (SearchOption::SearchProducerAndConsumerList == option))
        {
            pNode = m_pProducerList->Head();
            while (NULL != pNode)
            {
                pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
                if (seqId == pTargetBufferInfo->seqId)
                {
                    pNodeReturn = pNode;
                    break;
                }
                pNode = m_pProducerList->NextNode(pNode);
            }
        }

        if ((NULL == pNodeReturn) &&
            ((SearchOption::SearchConsumerList == option) ||
             (SearchOption::SearchProducerAndConsumerList == option)))
        {
            pNode = m_pConsumerList->Head();
            while (NULL != pNode)
            {
                pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
                if (seqId == pTargetBufferInfo->seqId)
                {
                    pNodeReturn = pNode;
                    break;
                }
                pNode = m_pConsumerList->NextNode(pNode);
            }
        }
    }

    return pNodeReturn;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::RemoveTargetBufferPrivate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult CHITargetBufferManager::RemoveTargetBufferPrivate(
    UINT32 seqId)
{
    // Remove the target buffer from the list.
    // Release references to image buffer and metadata and free memory resources of the node.
    // Note caller needs to aquire mutex lock before calling this API.

    CDKResult                           result              = CDKResultSuccess;
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    LightweightDoublyLinkedList*        pList               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;

    pNode = SearchList(seqId, SearchOption::SearchProducerList);
    if (NULL != pNode)
    {
        pList = m_pProducerList;
    }
    else
    {
        pNode = SearchList(seqId, SearchOption::SearchConsumerList);
        pList = m_pConsumerList;
    }

    if (NULL != pNode)
    {
        pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);

        if (0 < pTargetBufferInfo->refCount)
        {
            CHX_LOG_WARN("[%s] Target buffer seqId=%d is removed with refCount=%u",
                         m_targetBufferName, pTargetBufferInfo->seqId, pTargetBufferInfo->refCount);
        }

        for (UINT i = 0; i < m_numOfTargets; i++)
        {
            if (m_targetType[i] == ChiTargetType::Metadata)
            {
                if (NULL != pTargetBufferInfo->pMetadata[i])
                {
                    CHX_LOG("[%s] Release seqId %d metadata=%p, isImported:%d. refCount=%u",
                        m_targetBufferName, pTargetBufferInfo->seqId, pTargetBufferInfo->pMetadata[i],
                        pTargetBufferInfo->isImported, pTargetBufferInfo->refCount);
                    if (FALSE == pTargetBufferInfo->isImported)
                    {
                        m_pMetadataManager->Release(pTargetBufferInfo->pMetadata[i]);
                    }
                    pTargetBufferInfo->pMetadata[i] = NULL;
                }
            }
            else if (m_targetType[i] == ChiTargetType::InternalBuffer)
            {
                if (NULL != pTargetBufferInfo->chiStreamBuffer[i].bufferInfo.phBuffer)
                {
                    CHX_LOG("[%s] Release seqId %d phBuffer=%p, isImported:%d. refCount=%u",
                        m_targetBufferName, pTargetBufferInfo->seqId,
                        pTargetBufferInfo->chiStreamBuffer[i].bufferInfo.phBuffer,
                        pTargetBufferInfo->isImported, pTargetBufferInfo->refCount);
                    if (FALSE == pTargetBufferInfo->isImported)
                    {
                        m_pBufferManagers[i]->ReleaseReference(&pTargetBufferInfo->chiStreamBuffer[i].bufferInfo);
                    }
                    pTargetBufferInfo->chiStreamBuffer[i].bufferInfo.phBuffer = NULL;
                }
            }

            if ((m_isChiFenceEnabled == TRUE) &&
                ((m_targetType[i] == ChiTargetType::InternalBuffer) ||
                 (m_targetType[i] == ChiTargetType::ExternalBuffer)) &&
                (pTargetBufferInfo->chiStreamBuffer[i].releaseFence.valid == TRUE) &&
                (pTargetBufferInfo->chiStreamBuffer[i].releaseFence.type == ChiFenceTypeInternal))
            {
                CDKResult result;
                CHIFENCEHANDLE hChiFence = pTargetBufferInfo->chiStreamBuffer[i].releaseFence.hChiFence;
                ExtensionModule::GetInstance()->GetChiFenceStatus(hChiFence, &result);

                CSLFence hNewFence = reinterpret_cast<ChiFence*>(hChiFence)->hFence;
                CHX_LOG_INFO("Chifence status %d for fence %d", result, hNewFence);
                if (0 == hNewFence)
                {
                    CHX_LOG_WARN("Fence already released TBM::%s,\t fence: %p(%d), reqId: %d, streamBuf: %p ImgBuf: %p",
                                 m_targetBufferName,
                                 hChiFence,
                                 hNewFence,
                                 seqId,
                                 &pTargetBufferInfo->chiStreamBuffer[i],
                                 pTargetBufferInfo->chiStreamBuffer[i].bufferInfo.phBuffer);
                }
                else
                {

                    ExtensionModule::GetInstance()->ReleaseChiFence(hChiFence);

                    CHX_LOG_INFO("ReleaseFence TBM::%s, \t fence: %p(%d), reqId: %d, StreamBuf:%p ImgBuf: %p",
                                 m_targetBufferName,
                                 pTargetBufferInfo->chiStreamBuffer[i].releaseFence.hChiFence,
                                 hNewFence,
                                 seqId,
                                 &pTargetBufferInfo->chiStreamBuffer[i],
                                 pTargetBufferInfo->chiStreamBuffer[i].bufferInfo.phBuffer);

                    ChxUtils::Memset(&pTargetBufferInfo->chiStreamBuffer[i].releaseFence, 0, sizeof(CHIFENCEINFO));
                    ChxUtils::Memset(&pTargetBufferInfo->chiStreamBuffer[i].acquireFence, 0, sizeof(CHIFENCEINFO));
                }
            }
        }

        CHX_LOG("[%s] Target Buffer seqId=%d is being removed. refCount=%u",
                                  m_targetBufferName, seqId, pTargetBufferInfo->refCount);

        pList->RemoveNode(pNode);

        pTargetBufferInfo->pCHITargetBufferManager = NULL;
        pTargetBufferInfo->callbackData.pPrivateData = NULL;
        CHX_FREE(pTargetBufferInfo);
        pTargetBufferInfo = NULL;

        CHX_FREE(pNode);
        pNode = NULL;

        CHX_LOG("[%s] Target Buffer seqId=%d is removed", m_targetBufferName, seqId);
    }
    else
    {
        CHX_LOG_WARN("[%s] Failed to remove target buffer! Didn't find seqId %d from lists", m_targetBufferName, seqId);
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CHITargetBufferManager::RecycleTargetBuffersFromConsumerList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CHITargetBufferManager::RecycleTargetBuffersFromConsumerList()
{
    LightweightDoublyLinkedListNode*    pNode               = NULL;
    ChiTargetBufferInfo*                pTargetBufferInfo   = NULL;
    // Current logic releases the oldest target buffer without reference from m_pConsumerList if maxNumOfNodes is reached
    // More sophisticated recycling logic could be implemented.
    if (m_pProducerList->NumNodes() + m_pConsumerList->NumNodes() >= m_minNumOfBuffers)
    {
        m_pLock->Lock();
        pNode = m_pConsumerList->Head();
        while (NULL != pNode)
        {
            pTargetBufferInfo = reinterpret_cast<ChiTargetBufferInfo*>(pNode->pData);
            if (0 == pTargetBufferInfo->refCount)
            {
                RemoveTargetBufferPrivate(pTargetBufferInfo->seqId);
                break;
            }
            pNode = m_pConsumerList->NextNode(pNode);
        }
        m_pLock->Unlock();
    }
}
