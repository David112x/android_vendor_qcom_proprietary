////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcmdbuffermanager.cpp
/// @brief CmdBufferManager class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcmdbuffermanager.h"
#include "camxcmdbuffer.h"
#include "camxincs.h"
#include "camxlist.h"
#include "camxmem.h"
#include "camxpacket.h"
#include "camxpacketdefs.h"
#include "camxpacketresource.h"
#include "camxhwenvironment.h"
#include "camxsettingsmanager.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constant Definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT MaxNumAllocations = 100;  ///< Max number of allocations

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Mutex*                  CmdBufferManager::s_pStatsLock  = NULL;
CmdBufferManagerStats   CmdBufferManager::s_stats       = { };


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CmdBufferManager::CreateParentManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBufferManager* CmdBufferManager::CreateParentManager(
    BOOL                   bDisableOverrideSettings,
    CmdBufferManagerParam* pParams,
    UINT32                 numberOfCmdBufferManagers)
{
    CmdBufferManager* pBufferManger          = NULL;
    BOOL              bIsSharedAccess        = FALSE;
    BOOL              bIsPacket              = FALSE;
    BOOL              bIsValidCombination    = FALSE;
    BOOL              bIgnoreAlignment       = FALSE;
    UINT32            flags                  = 0;
    ResourceParams    params;
    UINT32            paddedSize             = 0;
    UINT32            numberOfResources      = 0;
    UINT32            allocationSize         = 0;
    CamxResult        result                 = CamxResultSuccess;
    UINT32            maxNestedBuffers       = 0;
    BOOL              bEnableAddressPatching = FALSE;
    CHAR              bufferManagerName[MaxStringLength256];

    // Validate whether requested buffer managers can be merged
    // Either all Buffer Managers needs to be Shared Access or Non Shared Access
    bIsSharedAccess = (CmdType::FW == pParams[0].pParams->cmdParams.type) ? TRUE : FALSE;
    bIsPacket       = (1 == pParams[0].pParams->usageFlags.packet) ? TRUE : FALSE;

    bIsValidCombination = TRUE;
    for (UINT32 count = 0; count < numberOfCmdBufferManagers; count++)
    {
        if (((TRUE == bIsSharedAccess) && (CmdType::FW != pParams[count].pParams->cmdParams.type))    ||
            ((FALSE == bIsSharedAccess) && (CmdType::FW == pParams[count].pParams->cmdParams.type)))
        {
            bIsValidCombination = FALSE;
            CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                           "Shared and Non Shared command buffer managers cant be merged, %s Shared Access %d",
                           pParams[count].pBufferManagerName, bIsSharedAccess);
            break;
        }

        if (((TRUE == bIsPacket) && (1 != pParams[count].pParams->usageFlags.packet))    ||
            ((FALSE == bIsPacket) && (1 == pParams[count].pParams->usageFlags.packet)))
        {
            bIsValidCombination = FALSE;
            CAMX_LOG_ERROR(CamxLogGroupMemMgr,
            "Packet and Non packet command buffer managers can't be merged, %s Packet %d",
            pParams[count].pBufferManagerName, bIsPacket);
            break;
        }


        paddedSize  = GetPaddedSize(pParams[count].pParams->resourceSize,
                                    pParams[count].pParams->alignment);

        numberOfResources = pParams[count].pParams->poolSize / pParams[count].pParams->resourceSize;
        allocationSize   += numberOfResources * paddedSize;

        if (1 == pParams[count].pParams->cmdParams.enableAddrPatching)
        {
            maxNestedBuffers       = 1;
            bEnableAddressPatching = TRUE;
        }
        flags |=  pParams[count].pParams->memFlags;
    }

    if (TRUE == bIsSharedAccess)
    {
        allocationSize   = GetFWBufferAlignedSize(allocationSize);
        bIgnoreAlignment = TRUE;
    }

    if (TRUE == bIsValidCombination)
    {
        Utils::Memcpy(&params, pParams[0].pParams, sizeof(ResourceParams));

        params.poolSize                     = allocationSize;
        params.resourceSize                 = allocationSize;
        params.cmdParams.maxNumNestedAddrs  = maxNestedBuffers;
        params.cmdParams.enableAddrPatching = bEnableAddressPatching;
        params.memFlags                     = flags;
        params.alignment                    = 1;
        pBufferManger = CAMX_NEW CmdBufferManager(bDisableOverrideSettings, NULL, TRUE);

        if (NULL != pBufferManger)
        {
            OsUtils::SNPrintF(bufferManagerName,
                              sizeof(bufferManagerName),
                              "%s__Parent",
                              pParams[0].pBufferManagerName);
            result = pBufferManger->Initialize(bufferManagerName, &params);
            if (CamxResultSuccess != result)
            {
                CAMX_DELETE pBufferManger;
                pBufferManger = NULL;
            }
        }
    }

    return pBufferManger;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CmdBufferManager::CreateMultiManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBufferManager::CreateMultiManager(
    BOOL                   bDisableOverrideSettings,
    CmdBufferManagerParam* pParams,
    UINT32                 numberOfCmdBufferManagers)
{
    CamxResult        result         = CamxResultSuccess;
    CmdBufferManager* pBufferManager = NULL;

    if ((NULL != pParams) && (0 < numberOfCmdBufferManagers))
    {
        CmdBufferManager* pParentCmdBufferManager = NULL;

        pParentCmdBufferManager = CreateParentManager(bDisableOverrideSettings, pParams, numberOfCmdBufferManagers);

        if (NULL != pParentCmdBufferManager)
        {
            for (UINT32 count = 0; count < numberOfCmdBufferManagers; count++)
            {
                if ((NULL != pParams[count].pBufferManagerName) &&
                    (NULL != pParams[count].pParams) &&
                    (NULL != pParams[count].ppCmdBufferManager))
                {
                    CmdBufferManager* pCmdBufferManager = NULL;
                    pCmdBufferManager = CAMX_NEW CmdBufferManager(bDisableOverrideSettings, pParentCmdBufferManager, TRUE);
                    if (NULL != pCmdBufferManager)
                    {
                        result = pCmdBufferManager->Initialize(pParams[count].pBufferManagerName,
                                                               pParams[count].pParams);
                        if (CamxResultSuccess == result)
                        {
                            pParentCmdBufferManager->IncrementChildRefCount();
                            *pParams[count].ppCmdBufferManager = pCmdBufferManager;
                        }
                        else
                        {
                            result = CamxResultEFailed;
                            CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                                           "%s command Buffer manager Initialization failed",
                                           pParams[count].pBufferManagerName);
                            break;
                        }
                    }
                    else
                    {
                        result = CamxResultENoMemory;
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Out Of Memory");
                        break;
                    }
                }
            }
        }

        if (CamxResultSuccess != result)
        {
            for (UINT32 count = 0; count < numberOfCmdBufferManagers; count++)
            {
                CmdBufferManager* pBufferManger = *pParams[count].ppCmdBufferManager;
                if (NULL != pBufferManger)
                {
                    CAMX_DELETE pBufferManager;
                    pBufferManager = NULL;
                }
            }

            if (NULL != pParentCmdBufferManager)
            {
                CAMX_DELETE pParentCmdBufferManager;
                pParentCmdBufferManager = NULL;
            }
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                       "Invalid Params %p numberofBufferMangers %d",
                       pParams,
                       numberOfCmdBufferManagers);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CmdBufferManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBufferManager::Create(
    const CHAR*           pBufferManagerName,
    const ResourceParams* pParams,
    CmdBufferManager**    ppCmdBufferManager)
{
    CamxResult        result            = CamxResultSuccess;
    CmdBufferManager* pCmdBufferManager = NULL;

    CAMX_ASSERT((NULL != pParams) && (NULL != ppCmdBufferManager));

    if ((NULL != pBufferManagerName) && (NULL != pParams) && (NULL != ppCmdBufferManager))
    {
        pCmdBufferManager = CAMX_NEW CmdBufferManager(FALSE);
        if (NULL != pCmdBufferManager)
        {
            result = pCmdBufferManager->Initialize(pBufferManagerName, pParams);

            if (CamxResultSuccess == result)
            {
                *ppCmdBufferManager = pCmdBufferManager;
            }
        }
        else
        {
            result = CamxResultENoMemory;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess != result)
    {
        if (NULL != pCmdBufferManager)
        {
            CAMX_DELETE pCmdBufferManager;
            pCmdBufferManager = NULL;
        }
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CmdBufferManager::CmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBufferManager::CmdBufferManager()
{
    m_bEnableMemoryStats     = HwEnvironment::GetInstance()->GetStaticSettings()->enableMemoryStats;
    m_pParentBufferManager   = NULL;
    m_bIgnoreBufferAlignment = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CmdBufferManager::CmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBufferManager::CmdBufferManager(
    BOOL    bDisableOverrideSettings)
{
    if (TRUE == bDisableOverrideSettings)
    {
        m_bEnableMemoryStats = FALSE;
    }
    else
    {
        m_bEnableMemoryStats = HwEnvironment::GetInstance()->GetStaticSettings()->enableMemoryStats;
    }

    m_pParentBufferManager   = NULL;
    m_bIgnoreBufferAlignment = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CmdBufferManager::CmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBufferManager::CmdBufferManager(
    BOOL              bDisableOverrideSettings,
    CmdBufferManager* pParentCmdBufferManager,
    BOOL              bIgnoreAlignment)
{
    if (TRUE == bDisableOverrideSettings)
    {
        m_bEnableMemoryStats = FALSE;
    }
    else
    {
        m_bEnableMemoryStats = HwEnvironment::GetInstance()->GetStaticSettings()->enableMemoryStats;
    }

    m_pParentBufferManager   = pParentCmdBufferManager;
    m_bIgnoreBufferAlignment = bIgnoreAlignment;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::InitializePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBufferManager::InitializePool()
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((0 != m_params.resourceSize) && (0 == (m_params.poolSize % m_params.resourceSize)));

    if ((0 == m_params.resourceSize) || (0 != (m_params.poolSize % m_params.resourceSize)))
    {
        result = CamxResultEInvalidArg;
    }
    else
    {
        // Only allocate one big memory and break it up into various resources.
        // We don't have to do it this way because KMD ensures the allocations of a particular type are
        // mapped in one area; however, it seems it's easier to manage and debug if all the allocations of a
        // manager use the same handle.
        SIZE_T  allocationSize  = 0;
        UINT32  flags           = m_params.memFlags;
        m_numResources          = m_params.poolSize / m_params.resourceSize;

        CAMX_ASSERT(m_params.usageFlags.cmdBuffer | m_params.usageFlags.packet);

        if (1 == m_params.usageFlags.packet)
        {
            flags |= CSLMemFlagPacketBuffer | CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
        }
        else
        {
            flags |= CSLMemFlagCmdBuffer;

            if (1 == m_params.cmdParams.enableAddrPatching)
            {
                flags |= CSLMemFlagKMDAccess;
            }

            if ((CmdType::CDMDirect     == m_params.cmdParams.type) ||
                (CmdType::CDMIndirect   == m_params.cmdParams.type) ||
                (CmdType::CDMDMI        == m_params.cmdParams.type) ||
                (CmdType::CDMDMI32      == m_params.cmdParams.type) ||
                (CmdType::CDMDMI64      == m_params.cmdParams.type))
            {
                flags |= CSLMemFlagHw;
            }

            if (CmdType::FW == m_params.cmdParams.type)
            {
                flags |= CSLMemFlagHw | CSLMemFlagSharedAccess;
            }
        }

        // Enable cache if the buffer is not shared with HW device
        if (0 == (flags & CSLMemFlagHw))
        {
            flags |= CSLMemFlagCache;
        }

        m_flags = flags;

        // We need to allocate one extra DWORD for overrun check purposes
        UINT32 paddedSize = GetPaddedSize(m_params.resourceSize, m_params.alignment);
        if (TRUE != m_bIgnoreBufferAlignment)
        {
            allocationSize    = m_numResources * paddedSize;
        }
        else
        {
            // Shared FW Buffers are pre calcuated for alignement, and they need to be exactly aligned as requested
            allocationSize = m_params.poolSize;
        }

        CAMX_ASSERT_MESSAGE(0 != allocationSize, "Num of resource: %d, padded size: %d",
                            m_numResources, paddedSize);

        CSLBufferInfo*  pBufferInfo = static_cast<CSLBufferInfo*>(CAMX_CALLOC(sizeof(CSLBufferInfo)));

        if (NULL == m_pParentBufferManager)
        {
            if (NULL != pBufferInfo)
            {
                result = CSLAlloc(m_pBufferManagerName,
                                  pBufferInfo,
                                  allocationSize,
                                  m_params.alignment,
                                  flags,
                                  m_params.pDeviceIndices,
                                  m_params.numDevices);
            }
            if ((CamxResultSuccess == result) && (NULL != pBufferInfo))
            {
                m_pBufferInfo = pBufferInfo;
                SetBufferOffset(pBufferInfo->offset);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Buffer Info %p, CamxResult: %d", pBufferInfo, result);
            }
        }
        else
        {
            if ((NULL != pBufferInfo) && (NULL != m_pParentBufferManager->GetCSLBufferInfo()))
            {
                Utils::Memcpy(pBufferInfo, m_pParentBufferManager->GetCSLBufferInfo(), sizeof(CSLBufferInfo));
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Buffer Info %p Parent Buffer Info %p",
                               pBufferInfo, m_pParentBufferManager->GetCSLBufferInfo());
            }
        }

        if ((CamxResultSuccess != result) || (NULL == pBufferInfo) || (CSLInvalidHandle == pBufferInfo->hHandle))
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory");

            result = CamxResultENoMemory;
        }
        else
        {
            if (TRUE == m_bEnableMemoryStats)
            {
                s_pStatsLock->Lock();

                s_stats.numBuffersAllocated++;
                s_stats.peakNumBuffersAllocated      = Utils::MaxUINT(s_stats.peakNumBuffersAllocated,
                                                                      s_stats.numBuffersAllocated);

                s_stats.sizeOfMemoryAllocated       += allocationSize;
                s_stats.peakSizeOfMemoryAllocated    = Utils::MaxSIZET(s_stats.peakSizeOfMemoryAllocated,
                                                                       s_stats.sizeOfMemoryAllocated);
                s_pStatsLock->Unlock();

                CAMX_LOG_INFO(CamxLogGroupMemMgr,
                              "[%s] : CreateParams [numResources=%d, resourceSize=%d (padded=%d), poolSize=%d, "
                              "alignment=%d, usageFlags=%d, , memFlags=0x%x, numDevices=%d, allocationSize=%zu], "
                              "CSLAlloc returned [hHandle=%d, fd=%d, size=%d] "
                              "Static Data : [numAllocations=%d, peakAllocations=%d, currentSize=%d, peakSize=%d]",
                              m_pBufferManagerName,
                              m_numResources,
                              m_params.resourceSize,
                              paddedSize,
                              m_params.poolSize,
                              m_params.alignment,
                              m_params.usageFlags,
                              m_params.memFlags,
                              m_params.numDevices,
                              allocationSize,
                              pBufferInfo->hHandle,
                              pBufferInfo->fd,
                              pBufferInfo->size,
                              s_stats.numBuffersAllocated,
                              s_stats.peakNumBuffersAllocated,
                              s_stats.sizeOfMemoryAllocated,
                              s_stats.peakSizeOfMemoryAllocated);
            }

            SIZE_T  resourceOffset = 0;
            UINT32  resourceSize   = paddedSize;

            if (NULL != m_pParentBufferManager)
            {
                resourceOffset = m_pParentBufferManager->GetBufferOffset();
            }

            // Create the actual command buffer resource objects and add to the corresponding bucket.
            for (UINT j = 0; j < m_numResources; j++)
            {
                // We pass the client-requested size as the size of the resource not the aligned size.
                PacketResource* pResource = NULL;

                result = CreateResource(pBufferInfo, resourceOffset, &m_params, &pResource);

                if (CamxResultSuccess == result)
                {
                    CAMX_ASSERT(NULL != pResource);

                    LDLLNode* pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));

                    if (NULL != pNode)
                    {
                        pNode->pData = pResource;

                        m_freePool.InsertToTail(pNode);
                        resourceOffset += resourceSize;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory");
                        result = CamxResultENoMemory;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupUtils, "Failed to create resource.");
                    break;
                }
            }

            if (NULL != m_pParentBufferManager)
            {
                m_pParentBufferManager->SetBufferOffset(resourceOffset);
            }

            if (CamxResultSuccess == result)
            {
                if (NULL == m_pParentBufferManager)
                {
                    LDLLNode* pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));

                    if (pNode != NULL)
                    {
                        pNode->pData = pBufferInfo;
                        m_CSLAllocations.InsertToTail(pNode);
                    }
                    else
                    {
                        result = CamxResultENoMemory;
                    }
                }
                else
                {
                    CAMX_FREE(pBufferInfo);
                    pBufferInfo = NULL;
                }
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupUtils, "Pool initialization failed: releasing resources...");
                UninitializePools();
                if (NULL == m_pParentBufferManager)
                {
                    CSLReleaseBuffer(pBufferInfo->hHandle);
                }
                CAMX_FREE(pBufferInfo);
                pBufferInfo = NULL;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::UninitializePools
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CmdBufferManager::UninitializePools()
{
    m_pLock->Lock();
    LDLLNode* pNode = m_freePool.RemoveFromHead();

    while (NULL != pNode)
    {
        PacketResource* pResource = static_cast<PacketResource*>(pNode->pData);
        pResource->Destroy();

        CAMX_FREE(pNode);

        pNode = m_freePool.RemoveFromHead();
    }

    pNode = m_busyPool.RemoveFromHead();

    while (NULL != pNode)
    {
        PacketResource* pResource = static_cast<PacketResource*>(pNode->pData);
        m_peakSizeUsedWithinAResource = Utils::MaxUINT(m_peakSizeUsedWithinAResource, pResource->GetResourceUsedDwords());
        pResource->Destroy();

        CAMX_FREE(pNode);

        pNode = m_busyPool.RemoveFromHead();
    }
    m_pLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::CreateResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBufferManager::CreateResource(
    CSLBufferInfo*      pBufferInfo,
    SIZE_T              offset,
    ResourceParams*     pParams,
    PacketResource**    ppResource)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL == ppResource) || (0 == pParams->resourceSize))
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
        result = CamxResultEInvalidArg;
    }
    else
    {
        if (TRUE == pParams->usageFlags.cmdBuffer)
        {
            result = CmdBuffer::Create(&(pParams->cmdParams),
                                       pBufferInfo,
                                       offset,
                                       pParams->resourceSize,
                                       reinterpret_cast<CmdBuffer**>(ppResource));
        }
        else if (TRUE == pParams->usageFlags.packet)
        {
            result = Packet::Create(&(pParams->packetParams),
                                    pBufferInfo,
                                    offset,
                                    pParams->resourceSize,
                                    reinterpret_cast<Packet**>(ppResource));
        }

        if (CamxResultSuccess == result)
        {
            (*ppResource)->SetUsageFlags(pParams->usageFlags);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Failed to process create request  cmd flag: %d, packet flag: %d, result: %d",
                           pParams->usageFlags.cmdBuffer, pParams->usageFlags.packet, result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBufferManager::Initialize(
    const CHAR*             pBufferManagerName,
    const ResourceParams*   pParams)
{
    CamxResult result = CamxResultSuccess;

    if (TRUE == m_initialized)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid state");
        result = CamxResultEInvalidState;
    }
    else if (NULL == pParams)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Invalid args");
        result = CamxResultEInvalidArg;
    }
    else
    {

        CAMX_ASSERT(NULL != pBufferManagerName);
        OsUtils::StrLCpy(m_pBufferManagerName, pBufferManagerName, sizeof(m_pBufferManagerName));

        m_params         = *pParams;

        if ((TRUE == m_bEnableMemoryStats) && (NULL == s_pStatsLock))
        {
            s_pStatsLock = Mutex::Create("CmdBufferStatsLock");

            if (NULL == s_pStatsLock)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed to create mutex lock");
                result = CamxResultEFailed;
            }
        }

        result = InitializePool();
    }

    m_pLock = Mutex::Create("CmdBufferManager");

    if (NULL == m_pLock)
    {
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        m_initialized = TRUE;
    }
    else
    {
        FreeResources();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::Uninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CmdBufferManager::Uninitialize()
{
    if (TRUE == m_initialized)
    {
        FreeResources();
        m_initialized = FALSE;

        if (NULL != m_pLock)
        {
            m_pLock->Destroy();
            m_pLock = NULL;
        }

        if (NULL != m_pBufferInfo)
        {
            CAMX_FREE(m_pBufferInfo);
            m_pBufferInfo = NULL;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::~CmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBufferManager::~CmdBufferManager()
{
    Uninitialize();

    if (TRUE == m_bEnableMemoryStats)
    {
        CAMX_LOG_INFO(CamxLogGroupMemMgr,
                      "CmdBufferMgrStats : CreateParams [numResources=%d, resourceSize=%d poolSize=%d "
                      "Flags(HW=%s, Protected=%s, CmdBuffer=%s, UMD=%s, Cache=%s, Packet=%s, KMD=%s, Shared=%s)] "
                      "ActualUsage [PeakNumResourcesUsed=%d, PeakSizeused=%d], "
                      "StaticData [num=%d, peakNum=%d, size=%zu, peakSize=%zu], Name[%s]",
                      m_numResources, m_params.resourceSize, m_params.poolSize,
                      (m_flags & CSLMemFlagHw)           ? "TRUE " : "FALSE",
                      (m_flags & CSLMemFlagProtected)    ? "TRUE " : "FALSE",
                      (m_flags & CSLMemFlagCmdBuffer)    ? "TRUE " : "FALSE",
                      (m_flags & CSLMemFlagUMDAccess)    ? "TRUE " : "FALSE",
                      (m_flags & CSLMemFlagCache)        ? "TRUE " : "FALSE",
                      (m_flags & CSLMemFlagPacketBuffer) ? "TRUE " : "FALSE",
                      (m_flags & CSLMemFlagKMDAccess)    ? "TRUE " : "FALSE",
                      (m_flags & CSLMemFlagSharedAccess) ? "TRUE " : "FALSE",
                      m_peakNumResourcesUsed,
                      m_peakSizeUsedWithinAResource * sizeof(UINT),
                      s_stats.numBuffersAllocated,   s_stats.peakNumBuffersAllocated,
                      s_stats.sizeOfMemoryAllocated, s_stats.peakSizeOfMemoryAllocated,
                      m_pBufferManagerName);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::GetPaddedSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CmdBufferManager::GetPaddedSize(
    UINT32 sizeInBytes,
    UINT32 alignmentInBytes)
{
    return static_cast<UINT32>(Utils::ByteAlign(sizeInBytes + sizeof(CamxCanary), alignmentInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::FreeMemHandles
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CmdBufferManager::FreeMemHandles()
{
    LDLLNode* pNode = m_CSLAllocations.RemoveFromHead();

    while (NULL != pNode)
    {
        CSLBufferInfo* pBufferInfo = static_cast<CSLBufferInfo*>(pNode->pData);
        CAMX_ASSERT(NULL != pBufferInfo);

        if (TRUE == m_bEnableMemoryStats)
        {
            s_pStatsLock->Lock();
            s_stats.numBuffersAllocated--;
            s_stats.sizeOfMemoryAllocated -= pBufferInfo->size;
            s_pStatsLock->Unlock();
        }

        CSLReleaseBuffer(pBufferInfo->hHandle);
        CAMX_FREE(pBufferInfo);
        pBufferInfo   = NULL;
        m_pBufferInfo = NULL;

        CAMX_FREE(pNode);
        pNode = m_CSLAllocations.RemoveFromHead();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::FreeResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CmdBufferManager::FreeResources()
{
    // free what's still in the pool
    UninitializePools();

    if (NULL == m_pParentBufferManager)
    {
        FreeMemHandles();
    }
    else
    {
        if (0 == m_pParentBufferManager->DecrementChildRefCount())
        {
            CAMX_DELETE m_pParentBufferManager;
            m_pParentBufferManager = NULL;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::Recycle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CmdBufferManager::Recycle(
    PacketResource* pResource)
{
    CAMX_ASSERT(TRUE == m_initialized);
    CAMX_ASSERT(NULL != pResource);

    m_pLock->Lock();

    if (NULL != pResource)
    {
        LDLLNode* pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));
        if (NULL != pNode)
        {
            pNode->pData = pResource;

            m_busyPool.RemoveByValue(pResource);
            m_peakSizeUsedWithinAResource = Utils::MaxUINT(m_peakSizeUsedWithinAResource, pResource->GetResourceUsedDwords());
            m_freePool.InsertToTail(pNode);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory");
        }
    }

    m_pLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::RecycleAll
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CmdBufferManager::RecycleAll(
    UINT64 requestId)
{
    CAMX_ASSERT(TRUE == m_initialized);

    m_pLock->Lock();

    LDLLNode* pNode = m_busyPool.Head();

    while (NULL != pNode)
    {
        PacketResource* pResource = static_cast<PacketResource*>(pNode->pData);
        CAMX_ASSERT(NULL != pResource);

        LDLLNode* pNext = LightweightDoublyLinkedList::NextNode(pNode);

        // If the resource's timestamp has retired reuse it.
        /// @note The assumption is we don't worry about wrap-around of requestIds (64bit)
        if (requestId == pResource->GetRequestId())
        {
            m_busyPool.RemoveNode(pNode);
            m_peakSizeUsedWithinAResource = Utils::MaxUINT(m_peakSizeUsedWithinAResource, pResource->GetResourceUsedDwords());
            m_freePool.InsertToTail(pNode);
        }

        pNode = pNext;
    }

    m_pLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::RecycleAllRequests
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CmdBufferManager::RecycleAllRequests()
{
    CAMX_ASSERT(TRUE == m_initialized);

    m_pLock->Lock();

    LDLLNode* pNode = m_busyPool.Head();

    while (NULL != pNode)
    {
        PacketResource* pResource = static_cast<PacketResource*>(pNode->pData);
        CAMX_ASSERT(NULL != pResource);

        LDLLNode* pNext = LightweightDoublyLinkedList::NextNode(pNode);

        m_busyPool.RemoveNode(pNode);
        m_peakSizeUsedWithinAResource = Utils::MaxUINT(m_peakSizeUsedWithinAResource, pResource->GetResourceUsedDwords());
        m_freePool.InsertToTail(pNode);

        pNode = pNext;
    }

    m_pLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::GetBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBufferManager::GetBuffer(
    PacketResource** ppBuffer)
{
    CAMX_ASSERT(NULL != ppBuffer);
    CAMX_ASSERT(TRUE == m_initialized);

    CamxResult result = CamxResultSuccess;

    if (NULL == ppBuffer)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "ppBuffer NULL");
        return CamxResultEFailed;
    }
    if (TRUE != m_initialized)
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "m_initialized is FALSE");
        return CamxResultEFailed;
    }

    m_pLock->Lock();

    LDLLNode* pNode = m_freePool.RemoveFromHead();

    if (NULL != pNode)
    {
        *ppBuffer = static_cast<PacketResource*>(pNode->pData);

        if (NULL == *ppBuffer)
        {
            result = CamxResultEInvalidState;
        }
        else
        {
            (*ppBuffer)->Reset();
            (*ppBuffer)->SetRequestId(CamxInvalidRequestId);
        }

        CAMX_FREE(pNode);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupUtils, "Out of memory");
        result = CamxResultENoMemory;
    }

    m_pLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::GetBufferForRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBufferManager::GetBufferForRequest(
    UINT64           requestId,
    PacketResource** ppBuffer)
{
    CamxResult result = GetBuffer(ppBuffer);

    m_pLock->Lock();

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(NULL != *ppBuffer);

        (*ppBuffer)->SetRequestId(requestId);

        LDLLNode* pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));

        if (NULL != pNode)
        {
            pNode->pData = *ppBuffer;

            m_busyPool.InsertToTail(pNode);

            m_peakNumResourcesUsed = Utils::MaxUINT(m_peakNumResourcesUsed, m_busyPool.NumNodes());
        }
        else
        {
            result = CamxResultENoMemory;
        }
    }

    m_pLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CmdBufferManager::CheckBufferWithRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CmdBufferManager::CheckBufferWithRequest(
    UINT64           requestId,
    PacketResource** ppBuffer)
{
    m_pLock->Lock();

    CamxResult result = CamxResultEFailed;
    LDLLNode*  pNode  = m_busyPool.Head();

    // This code assumes there is only one buffer that matches
    while (NULL != pNode)
    {
        PacketResource* pResource = static_cast<PacketResource*>(pNode->pData);
        CAMX_ASSERT(NULL != pResource);

        if ((NULL != pResource) && (requestId == pResource->GetRequestId()))
        {
            result    = CamxResultSuccess;
            *ppBuffer = pResource;
            break;
        }

        pNode = LightweightDoublyLinkedList::NextNode(pNode);
    }

    m_pLock->Unlock();

    return result;
}

CAMX_NAMESPACE_END
