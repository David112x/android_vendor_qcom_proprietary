////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxmempoolgroup.cpp
///
/// @brief CamX Memory Pool Group definitions. MemPoolGroup manages the buffers and sharing among Buffer Managers.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE PR007b: Whiner incorrectly concludes as non-library files
#include <limits>

#include "camxmempoolmgr.h"
#include "camxmempoolgroup.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants needed for type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT32 RealTimeImmediateAllocBufferCount = 2;
static const UINT32 IFENodeImmediateAllocBufferCount  = 4;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const UINT MemPoolInvalidBufferCount = 0xFFFFFFF;

// MemPoolGroup Class static members
Mutex*              MemPoolGroup::s_pMPGStatsLock   = NULL;
MemPoolManagerStats MemPoolGroup::s_memPoolMgrStats = { };


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::MemPoolGroup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemPoolGroup::MemPoolGroup()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::~MemPoolGroup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemPoolGroup::~MemPoolGroup()
{
    UINT numBufMgrs = m_bufferManagerList.NumNodes();

    CAMX_ASSERT(0 == numBufMgrs);
    if (0 != numBufMgrs)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolGroup[%s] : Number of Buffer Managers still attached to this group %d",
                       m_pGroupName, numBufMgrs);
    }

    CAMX_ASSERT(0 == m_numBuffersAllocated);
    if (0 != m_numBuffersAllocated)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolGroup[%s] : Number of Buffers still allocated in this group %d",
                       m_pGroupName, m_numBuffersAllocated);
    }

    // Do not change this log format, offline scripts are written based on this
    CAMX_LOG_INFO(CamxLogGroupMemMgr,
                  "MemPoolGroup[%s][%p] : Buffers : Allocated=%d, Inuse=%d, Free=%d PeakAllocated=%d, PeakUsed=%d",
                  m_pGroupName,                 this,
                  m_numBuffersAllocated,        (m_numBuffersAllocated - m_freeBufferList.NumNodes()),
                  m_freeBufferList.NumNodes(),  m_peakNumBuffersAllocated,
                  m_peakNumBuffersUsed);

    if (NULL != m_pLock)
    {
        m_pLock->Destroy();
        m_pLock = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolGroup::Initialize(
    UINT32          groupNumber,
    MemPoolMgr*     pMemPoolMgr,
    CSLBufferHeap   bufferHeap,
    BOOL            bDedicatedGroup,
    BOOL            bDisableSelfShrinking)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pMemPoolMgr)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input MemPoolMgr pointer");
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) && (TRUE == GetStaticSettings()->enableMemoryStats))
    {
        if (NULL == s_pMPGStatsLock)
        {
            Utils::Memset(&s_memPoolMgrStats, 0x0, sizeof(s_memPoolMgrStats));

            s_pMPGStatsLock = Mutex::Create("MPGStatsLock");

            if (NULL == s_pMPGStatsLock)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed to create mutex lock", m_pGroupName);
                result = CamxResultEFailed;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        OsUtils::SNPrintF(m_pGroupName, MaxStringLength64, "MPG_%d", groupNumber);

        m_pLock = Mutex::Create(m_pGroupName);

        if (NULL == m_pLock)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : Failed to create mutex lock", m_pGroupName);
            result = CamxResultEFailed;
        }
    }

    if (CamxResultSuccess == result)
    {
        m_pMemPoolMgr           = pMemPoolMgr;
        m_bufferHeap            = bufferHeap;
        m_bDisableSelfShrinking = bDisableSelfShrinking;

        // If self shrinking is disabled or heap type is EGL for this group, its better to have it as a dedicated
        // group for this Buffer Manager set m_bDedicatedGroup to TRUE in such case, so that IsMatchingGroup for
        // this group would fail for any future Buffer Manager registers
        m_bDedicatedGroup       = ((TRUE             == m_bDisableSelfShrinking) ||
                                   (CSLBufferHeapEGL == m_bufferHeap))            ? TRUE : bDedicatedGroup;

        m_registeredBufferCount.immediateAllocBufferCount.min = MemPoolInvalidBufferCount;
        m_registeredBufferCount.maxBufferCount.min            = MemPoolInvalidBufferCount;
        m_activatedBufferCount.immediateAllocBufferCount.min  = MemPoolInvalidBufferCount;
        m_activatedBufferCount.maxBufferCount.min             = MemPoolInvalidBufferCount;

        // Lets start with address alignment which satisfies all HW's requirements - something like 4K.
        // If a Buffer Manager needs a different alignment not dividiable by the default value, we can anyway update it
        // in UpdateBufferAllocProperties - which makes alignment as LCM(4096, alignment)
        // This help to avoid going to a different group just because of different address alignment requirements.
        m_bufferAllocProperties.alignment = GetStaticSettings()->MPMBufferAddressAlignment;

        // Have UMD access flags always if dumping enabled
        if ((TRUE == GetStaticSettings()->autoImageDump)    ||
            (TRUE == GetStaticSettings()->dynamicImageDump) ||
            (TRUE == GetStaticSettings()->watermarkImage))
        {
            m_bufferAllocProperties.flags = CSLMemFlagUMDAccess;
        }

        // Disable Delayed Unmap optimization
        if (TRUE == GetStaticSettings()->MPMForceDisableDelayedUnmap)
        {
            m_bufferAllocProperties.flags |= CSLMemFlagDisableDelayedUnmap;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s] : Group Created Heap=%d, Dedicated=%d",
                         m_pGroupName, bufferHeap, bDedicatedGroup);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::IsMatchingGroup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MemPoolGroup::IsMatchingGroup(
    const CHAR*                     pBufferManagerName,
    const BufferManagerCreateData*  pCreateData)
{
    BOOL bCanCombineIntoThisGroup   = FALSE;
    UINT numBufMgrs;

    CAMX_ASSERT(NULL != pBufferManagerName);
    CAMX_ASSERT(NULL != pCreateData);

    m_pLock->Lock();

    numBufMgrs = m_bufferManagerList.NumNodes();

    if ((TRUE == GetStaticSettings()->MPMDoNotGroupBufferManagers) || (TRUE == m_bDedicatedGroup))
    {
        // 1. If MPMDoNotGroupBufferManagers setting is enabled, any group would have max only one Buffer Manager attached to it
        // 2. A group is tagged as DedicatedGroup if this group is created for an BufferManager which needs dedicated buffers.
        //    So, do not allow any other BufferManagers being attached to this group.
        CAMX_ASSERT(1 >= numBufMgrs);

        bCanCombineIntoThisGroup = FALSE;

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Dedicated=%d", m_pGroupName, this, m_bDedicatedGroup);
    }
    else
    {
        BOOL bBuffersAllocated  = (m_numBuffersAllocated > 0) ? TRUE : FALSE;
        BOOL bSizeMatch         = FALSE;
        BOOL bAlignmentMatch    = FALSE;
        BOOL bFlagsMatch        = FALSE;
        BOOL bGrallocFlagsMatch = FALSE;
        BOOL bDeviceMatch       = FALSE;
        BOOL bCompatibleHeap    = FALSE;
        BOOL bIsFromSameNode    = FALSE;

        LDLLNode* pNode = m_bufferManagerList.Head();

#undef max
        SIZE_T smallestSizeInGroup  = std::numeric_limits<SIZE_T>::max();

        while (NULL != pNode)
        {
            MemPoolBufferManager* pMemPoolBufMgr = static_cast<MemPoolBufferManager*>(pNode->pData);

            if (NULL != pMemPoolBufMgr)
            {
                if ((NULL                              != pMemPoolBufMgr->createData.linkProperties.pNode) &&
                    (NULL                              != pCreateData->linkProperties.pNode)               &&
                    (pCreateData->linkProperties.pNode == pMemPoolBufMgr->createData.linkProperties.pNode))
                {
                    bIsFromSameNode = TRUE;
                    break;
                }

                smallestSizeInGroup = ((pMemPoolBufMgr->sizeRequired < smallestSizeInGroup) ?
                                        pMemPoolBufMgr->sizeRequired : smallestSizeInGroup);
            }

            pNode = LightweightDoublyLinkedList::NextNode(pNode);
        }

        // If there are no buffers allocated currently in this group : we can be little liberal
        // If already allocated : we can't accept Buffer managers with more size, alignment, etc.. more restrictive rules
        if (bIsFromSameNode == FALSE)
        {
            bCompatibleHeap = IsCompatibleHeap(bBuffersAllocated, m_bufferHeap, pCreateData->bufferProperties.bufferHeap);
        }

        if (TRUE == bCompatibleHeap)
        {
            SIZE_T currentSize = ImageFormatUtils::GetTotalSize(&pCreateData->bufferProperties.imageFormat) *
                                 pCreateData->numBatchedFrames;

            bSizeMatch = SizeWithinRange((m_numBuffersAllocated > 0) ? TRUE : FALSE,
                                         m_bufferAllocProperties.totalBufferSizeBytes,
                                         currentSize,
                                         smallestSizeInGroup);

            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                             "MemPoolGroup[%s][%p] BufMgr[%s] Group Alloc size=%d, BufMgr alloc size=%d, bSizeMatch=%d",
                             m_pGroupName, this, pBufferManagerName, m_bufferAllocProperties.totalBufferSizeBytes,
                             currentSize, bSizeMatch);

            if (TRUE == bSizeMatch)
            {
                bAlignmentMatch = AlignmentMatch((m_numBuffersAllocated > 0) ? TRUE : FALSE,
                                                 m_bufferAllocProperties.alignment,
                                                 pCreateData->bufferProperties.imageFormat.alignment);

                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] BufMgr[%s] "
                                 "Group alignment=%d, BufMgr alignment=%d, bAlignmentMatch=%d",
                                 m_pGroupName, this, pBufferManagerName, m_bufferAllocProperties.alignment,
                                 pCreateData->bufferProperties.imageFormat.alignment, bAlignmentMatch);
            }

            if (TRUE == bAlignmentMatch)
            {
                bFlagsMatch = CSLMemFlagsMatch((m_numBuffersAllocated > 0) ? TRUE : FALSE,
                                               m_bufferAllocProperties.flags,
                                               pCreateData->bufferProperties.memFlags);

                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                 "MemPoolGroup[%s][%p] BufMgr[%s] Group flags=0x%x, BufMgr flags=0x%x, bFlagsMatch=%d",
                                 m_pGroupName, this, pBufferManagerName, m_bufferAllocProperties.flags,
                                 pCreateData->bufferProperties.memFlags, bFlagsMatch);
            }

            if (TRUE == bFlagsMatch)
            {
                if (CSLBufferHeapEGL == pCreateData->bufferProperties.bufferHeap)
                {
                    bGrallocFlagsMatch = GrallocMemFlagsMatch((m_numBuffersAllocated > 0) ? TRUE : FALSE,
                                                              m_bufferAllocProperties.producerUsageFlags,
                                                              m_bufferAllocProperties.consumerUsageFlags,
                                                              pCreateData->bufferProperties.producerFlags,
                                                              pCreateData->bufferProperties.consumerFlags);
                }
                else
                {
                    // set this to TRUE to continue checking others
                    bGrallocFlagsMatch = TRUE;
                }
            }

            if (TRUE == bGrallocFlagsMatch)
            {
                bDeviceMatch = DevicesMatch((m_numBuffersAllocated > 0) ? TRUE : FALSE,
                                            m_bufferAllocProperties.deviceIndices,
                                            m_bufferAllocProperties.deviceCount,
                                            pCreateData->deviceIndices,
                                            pCreateData->deviceCount);

                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                 "MemPoolGroup[%s][%p] : BufMgr[%s] "
                                 "Group Alloc Devices(%d)=%d %d %d %d %d %d %d, "
                                 "BufMgr alloc Devices(%d)=%d %d %d %d %d %d %d, bDeviceMatch=%d",
                                 m_pGroupName, this, pBufferManagerName,
                                 m_bufferAllocProperties.deviceCount,
                                 m_bufferAllocProperties.deviceIndices[0],
                                 m_bufferAllocProperties.deviceIndices[1],
                                 m_bufferAllocProperties.deviceIndices[2],
                                 m_bufferAllocProperties.deviceIndices[3],
                                 m_bufferAllocProperties.deviceIndices[4],
                                 m_bufferAllocProperties.deviceIndices[5],
                                 m_bufferAllocProperties.deviceIndices[6],
                                 pCreateData->deviceCount,
                                 pCreateData->deviceIndices[0],
                                 pCreateData->deviceIndices[1],
                                 pCreateData->deviceIndices[2],
                                 pCreateData->deviceIndices[3],
                                 pCreateData->deviceIndices[4],
                                 pCreateData->deviceIndices[5],
                                 pCreateData->deviceIndices[6],
                                 bDeviceMatch);
            }

            if ((TRUE == bSizeMatch) && (TRUE == bAlignmentMatch) && (TRUE == bFlagsMatch) && (TRUE == bGrallocFlagsMatch) &&
                (TRUE == bDeviceMatch))
            {
                bCanCombineIntoThisGroup= TRUE;
            }
        }
    }

    m_pLock->Unlock();

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : BufMgr[%s] Matching Group = %d",
                     m_pGroupName, this, pBufferManagerName, bCanCombineIntoThisGroup);

    return bCanCombineIntoThisGroup;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::RegisterBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemPoolBufferManager* MemPoolGroup::RegisterBufferManager(
    const CHAR*                     pBufferManagerName,
    const BufferManagerCreateData*  pCreateData,
    CamxResult*                     pIsMatchingResult)
{
    CamxResult              result              = CamxResultSuccess;
    MemPoolBufferManager*   pMemPoolBufMgr      = NULL;
    LDLLNode*               pNode               = NULL;
    BOOL                    bMatchingGroupFound = FALSE;

    if ((NULL == pBufferManagerName) || (NULL == pCreateData) || (NULL == pIsMatchingResult))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input args %p %p %p", pBufferManagerName, pCreateData, pIsMatchingResult);
        result = CamxResultEInvalidArg;
    }

    m_pLock->Lock();

    if ((CamxResultSuccess == result) && (TRUE == m_bufferAllocProperties.valid))
    {
        bMatchingGroupFound = IsMatchingGroup(pBufferManagerName, pCreateData);
        if (FALSE == bMatchingGroupFound)
        {

            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] cannot reuse for BufMgr[%s]",
                             m_pGroupName, this, pBufferManagerName);
            result = CamxResultENoSuch;
        }
    }

    if (CamxResultSuccess == result)
    {
        pNode           = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));
        pMemPoolBufMgr  = static_cast<MemPoolBufferManager*>(CAMX_CALLOC(sizeof(MemPoolBufferManager)));

        if ((NULL != pMemPoolBufMgr) && (NULL != pNode))
        {
            OsUtils::StrLCpy(pMemPoolBufMgr->name, pBufferManagerName, MaxStringLength256);

            pMemPoolBufMgr->pMemPoolGroup   = this;
            pMemPoolBufMgr->pNode           = pNode;
            pMemPoolBufMgr->createData      = *pCreateData;
            pMemPoolBufMgr->bActivated      = FALSE;
            pMemPoolBufMgr->peakBuffersUsed = 0;

            SelfTuneImmediateCount(pMemPoolBufMgr);

            pNode->pData = pMemPoolBufMgr;

            // Add the buffer manager client to the list
            m_bufferManagerList.InsertToTail(pNode);

            // Even if the heap is set Ion while creating the group, if we are attaching a buffer manager that requires Gralloc,
            // change the heap type to Gralloc. We allow such grouping only if no buffers allocated in this group currently
            if (CSLBufferHeapEGL == pCreateData->bufferProperties.bufferHeap)
            {
                if (CSLBufferHeapIon == m_bufferHeap)
                {
                    CAMX_ASSERT(0 == m_numBuffersAllocated);
                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                     "MemPoolGroup[%s]-->MemPoolBufMgr[%s] : Switching to Gralloc heap, numBuffersAllocated=%d",
                                     m_pGroupName, pMemPoolBufMgr->name, m_numBuffersAllocated);
                }

                m_bufferHeap = CSLBufferHeapEGL;
            }

            // Update buffer counts and alloc properties always in Register, so any new allocation from now onwards will
            // obey the requirements of this buffer manager as well.

            // Update Registered Buffer count info
            UpdateMemPoolGroupBufferCounts(pMemPoolBufMgr, TRUE, FALSE);

            // If there are buffers already allocated in this group, this generally adds new devices, flags but size wont change
            // IsMatchingGroup takes care of not allowing Buffer Managers needing more size in such cases.
            // If no buffers currently, this may increase the buffer size as well as IsMatchingGroup might allow Buffer Managers
            // needing more size (within fixed range) as well.
            UpdateBufferAllocProperties(pMemPoolBufMgr);

            // Save consolidated group link property information. Useful in determining while allcoation, free of buffers
            UpdateMemPoolGroupLinkProperties(pMemPoolBufMgr);

            // Do not change this log format, offline scripts are written based on this
            CAMX_LOG_CONFIG(CamxLogGroupMemMgr,
                             "[%s]-->[%s] : NumBufMgrs=%d, type=%s, heap=%d, "
                             "Original Immediate=%d, Max=%d, SelfTuned Immediate=%d, Max=%d, "
                             "needDedicatedBuffers=%d, lateBinding=%d, numBatched=%d, "
                             "deviceCount=%d, devices=%d, %d, flags=0x%x width=%d, height=%d, format=%d, size=%zu",
                             m_pGroupName, pMemPoolBufMgr->name, m_bufferManagerList.NumNodes(),
                             (BufferManagerType::CamxBufferManager == pCreateData->bufferManagerType) ? "CamX" : "CHI",
                             pCreateData->bufferProperties.bufferHeap,
                             pCreateData->immediateAllocBufferCount, pCreateData->maxBufferCount,
                             pMemPoolBufMgr->createData.immediateAllocBufferCount, pMemPoolBufMgr->createData.maxBufferCount,
                             pCreateData->bNeedDedicatedBuffers,
                             pCreateData->bEnableLateBinding,
                             pCreateData->numBatchedFrames,
                             pCreateData->deviceCount,
                             pCreateData->deviceIndices[0],
                             pCreateData->deviceIndices[1],
                             pCreateData->bufferProperties.memFlags,
                             pCreateData->bufferProperties.imageFormat.width,
                             pCreateData->bufferProperties.imageFormat.height,
                             pCreateData->bufferProperties.imageFormat.format,
                             pMemPoolBufMgr->sizeRequired);

            // Enabling MPMAllocateBuffersOnRegister will cause to allocate buffers on Pipeline creation itself. This may
            // unnecessarily pre-allocate buffers for offline pipelines even though really not required.
            // Also, by allocating buffers here, we may loose advantage of sharing buffers with Buffer Managers having slightly
            // different requirements, exa - with small size difference

            if (TRUE == GetStaticSettings()->MPMAllocateBuffersOnRegister)
            {
                UINT32 buffersToAllocate = DetermineBufferAllocationRequired(pMemPoolBufMgr);

                if (0 < buffersToAllocate)
                {
                    UINT32 numBuffersAllocated = AllocateBuffers(buffersToAllocate);

                    if (buffersToAllocate != numBuffersAllocated)
                    {
                        CAMX_LOG_WARN(CamxLogGroupMemMgr, "Failed in Allocating buffers heap=%d, Required=1, Allocated=%d",
                                       m_bufferHeap, buffersToAllocate, numBuffersAllocated);

                        // No need to set error here, as this is just pre-allocating. If the buffers still required,
                        // we will try again in GetBufferFromPool anyway, we can have fatal error there
                    }
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Insufficient memory %p %p", pMemPoolBufMgr, pNode);

            if (NULL != pNode)
            {
                CAMX_FREE(pNode);
                pNode = NULL;
            }

            if (NULL != pMemPoolBufMgr)
            {
                CAMX_FREE(pMemPoolBufMgr);
                pMemPoolBufMgr = NULL;
            }
        }
    }

    if (NULL != pIsMatchingResult)
    {
        // update matching group result
        *pIsMatchingResult = result;
    }

    m_pLock->Unlock();

    return pMemPoolBufMgr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::UnregisterBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolGroup::UnregisterBufferManager(
    MemPoolBufferManager* pMemPoolBufMgr)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL == pMemPoolBufMgr) || (NULL == pMemPoolBufMgr->pNode))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input %p", pMemPoolBufMgr);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        PrintMemPoolBufMgrState(pMemPoolBufMgr, FALSE);

        result = DeactivateBufferManager(pMemPoolBufMgr);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : MemPoolBufMgr[%s][%p] Failed in Deactivating, result=%s",
                           m_pGroupName, this, pMemPoolBufMgr->name, pMemPoolBufMgr, Utils::CamxResultToString(result));
        }
    }

    if (CamxResultSuccess == result)
    {
        m_pLock->Lock();

        // Error check - Ideally, Deactivate should make sure all buffers assigned to this BufferManager are released
        // to Free list, even if they were not released explicitly
        if (TRUE == IsBufMgrHoldingBuffers(pMemPoolBufMgr))
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                           "MemPoolGroup[%s][%p]-->MemPoolBufMgr[%s] has buffers in use, shouldn't unregister",
                           m_pGroupName, this, pMemPoolBufMgr->name);
        }

        LDLLNode* pNode = pMemPoolBufMgr->pNode;

        m_bufferManagerList.RemoveNode(pNode);

        SetupMemPoolGroupBufferCounts(TRUE, FALSE);

        SetupBufferAllocProperties();

        SetupMemPoolGroupLinkProperties();

        // Now see if we can free any buffers that are not being used
        UINT numBuffersToFree = DetermineBufferFreeRequired(pMemPoolBufMgr, TRUE);

        if (0 < numBuffersToFree)
        {
            FreeBuffers(numBuffersToFree);
        }

        // Do not change this log format, offline scripts are written based on this
        CAMX_LOG_INFO(CamxLogGroupMemMgr,
                      "[%s]-->[%s] : Stats : CreateData : (type=%s, Immediate=%d, Max=%d), "
                      "Actual Peak=%d, sizeRequired=%zu (allocated in range of %zu to %zu), activated=%d",
                      m_pGroupName, pMemPoolBufMgr->name,
                      (BufferManagerType::CamxBufferManager == pMemPoolBufMgr->createData.bufferManagerType) ? "CamX" : "CHI",
                      pMemPoolBufMgr->createData.immediateAllocBufferCount,
                      pMemPoolBufMgr->createData.maxBufferCount,
                      pMemPoolBufMgr->peakBuffersUsed,
                      pMemPoolBufMgr->sizeRequired,
                      m_leastBufferSizeAllocated,
                      m_maxBufferSizeAllocated,
                      pMemPoolBufMgr->bEverActivated);

        m_pLock->Unlock();

        CAMX_FREE(pNode);
        CAMX_FREE(pMemPoolBufMgr);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::ActivateBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolGroup::ActivateBufferManager(
    MemPoolBufferManager* pMemPoolBufMgr)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pMemPoolBufMgr)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input %p", pMemPoolBufMgr);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        m_pLock->Lock();

        if (TRUE == pMemPoolBufMgr->bActivated)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p]-->MemPoolBufMgr[%s][%p] : in Active state already",
                             m_pGroupName, this, pMemPoolBufMgr->name, pMemPoolBufMgr);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p]-->MemPoolBufMgr[%s][%p] : Activating",
                          m_pGroupName, this, pMemPoolBufMgr->name, pMemPoolBufMgr);

            SetMemPoolBufMgrActivated(pMemPoolBufMgr, TRUE);

            // Update Active Buffer count info
            UpdateMemPoolGroupBufferCounts(pMemPoolBufMgr, FALSE, TRUE);

            UINT buffersToAllocate  = DetermineBufferAllocationRequired(pMemPoolBufMgr);

            if (0 < buffersToAllocate)
            {
                UINT32 numBuffersAllocated = AllocateBuffers(buffersToAllocate);

                if (buffersToAllocate != numBuffersAllocated)
                {
                    CAMX_LOG_WARN(CamxLogGroupMemMgr, "Failed in Allocating buffers heap=%d, Required=%d, Allocated=%d",
                                  m_bufferHeap, buffersToAllocate, numBuffersAllocated);

                    // Allocation failed for some reason, we can still continue, do not set error
                    // We can allocate buffers when client Buffer Manager actually asks for buffer, we can fail there
                    // if we couldn't allocate
                }
            }
        }

        m_pLock->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::DeactivateBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolGroup::DeactivateBufferManager(
    MemPoolBufferManager* pMemPoolBufMgr)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pMemPoolBufMgr)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input");
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        m_pLock->Lock();

        if (FALSE == pMemPoolBufMgr->bActivated)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p]-->MemPoolBufMgr[%s][%p] : in Deactive state already",
                             m_pGroupName, this, pMemPoolBufMgr->name, pMemPoolBufMgr);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p]-->MemPoolBufMgr[%s][%p] : Deactivating",
                          m_pGroupName, this, pMemPoolBufMgr->name, pMemPoolBufMgr);

            SetMemPoolBufMgrActivated(pMemPoolBufMgr, FALSE);

            SetupMemPoolGroupBufferCounts(FALSE, TRUE);

            // Release any buffers that are still assigned to this Buffer Manager. This shouldn't happen ideally as we expect
            // client Buffer Managers to release all buffers they acquired before. Lets print a warning to pop this up.
            LDLLNode* pNode = pMemPoolBufMgr->memPoolBufferList.RemoveFromHead();

            while (NULL != pNode)
            {
                MemPoolBuffer* pBuffer = static_cast<MemPoolBuffer*>(pNode->pData);

                CAMX_ASSERT(NULL != pBuffer);

                CAMX_LOG_WARN(CamxLogGroupMemMgr, "MemPoolGroup[%s]-->MemPoolBufMgr[%s] : MemPoolBuffer[%s] was not released",
                              m_pGroupName, pMemPoolBufMgr->name, pBuffer->name);

                pBuffer->pMemPoolBufMgr     = NULL;
                pBuffer->bCurrentlyCached   = TRUE; // since we dont know the state, set cached, we might take an invalidate
                                                    // hit, but still better than corruption

                // Add the buffer to the free buffer list
                m_freeBufferList.InsertToTail(pNode);

                pNode = pMemPoolBufMgr->memPoolBufferList.RemoveFromHead();
            }

            // Now see if we can free any buffers that are not being used
            UINT numBuffersToFree = DetermineBufferFreeRequired(pMemPoolBufMgr, FALSE);

            FreeBuffers(numBuffersToFree);
        }

        m_pLock->Unlock();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::GetBufferFromPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemPoolGetBufferResult MemPoolGroup::GetBufferFromPool(
    MemPoolBufferManager*   pMemPoolBufMgr,
    CSLBufferInfo*          pCSLBufferInfo,
    BufferHandle*           phGrallocBuffer)
{
    CamxResult              result              = CamxResultSuccess;
    MemPoolBuffer*          pBuffer             = NULL;
    MemPoolGetBufferResult  getBufferPoolResult = { 0 };

    if (FALSE == pMemPoolBufMgr->bActivated)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p]-->MemPoolBuffer[%s][%p] calling Activate internally",
                         m_pGroupName, this, pMemPoolBufMgr->name, pMemPoolBufMgr);

        // Make sure this Buffer Manager is activated
        result = ActivateBufferManager(pMemPoolBufMgr);
    }

    m_pLock->Lock();

    if ((CamxResultSuccess == result) &&
        (pMemPoolBufMgr->memPoolBufferList.NumNodes() == pMemPoolBufMgr->createData.maxBufferCount))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : MemPoolBuffer[%s][%p] exceeding max Buffers %d",
                       m_pGroupName, this, pMemPoolBufMgr->name, pMemPoolBufMgr, pMemPoolBufMgr->createData.maxBufferCount);

        result = CamxResultENoMore;
    }

    if (CamxResultSuccess == result)
    {
        /// @todo (CAMX-4157) Wait for the offline thread to finish, if in progress
        // Make sure to wait if the offline thread is in the process of allocating a buffer after wait is done, do below

        if (0 == m_freeBufferList.NumNodes())
        {
            // No free buffers, grow Pool.
            UINT32 numBuffersAllocated = AllocateBuffers(1);

            if (1 != numBuffersAllocated)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                               "MemPoolGroup[%s] Failed in Allocating buffers heap=%d, Required=1, Allocated=%d",
                               m_pGroupName, m_bufferHeap, numBuffersAllocated);

                result = CamxResultENoMemory;
            }
        }

        if (CamxResultSuccess == result)
        {
            // Check the free List for an available buffer
            LDLLNode* pNode = m_freeBufferList.RemoveFromHead();

            if (NULL != pNode)
            {
                pBuffer = static_cast<MemPoolBuffer*>(pNode->pData);

                CAMX_ASSERT(NULL != pBuffer);

                // We got a buffer to give back.. but make sure this is mapped to the devices that this Buffer manager requires.
                // This can happen if - this MemPool Group allocated some buffers before this Buffer manager attached to this
                // group (based on size). In that case, buffer may not have mapped to the devices that listed in this
                // buffer manager.
                // So, check if array of devices listed in this Buffer Manager create data are present in the array of devices
                // listed in MemPoolBuffer device list.
                result = CheckAndMapBufferToDevices(pBuffer,
                                                    pMemPoolBufMgr->createData.deviceIndices,
                                                    pMemPoolBufMgr->createData.deviceCount,
                                                    pMemPoolBufMgr->createData.bufferProperties.memFlags);

                if (CamxResultSuccess == result)
                {
                    pBuffer->pMemPoolBufMgr = pMemPoolBufMgr;

                    pMemPoolBufMgr->memPoolBufferList.InsertToTail(pNode);

                    pMemPoolBufMgr->peakBuffersUsed = Utils::MaxUINT(pMemPoolBufMgr->peakBuffersUsed,
                                                                     pMemPoolBufMgr->memPoolBufferList.NumNodes());

                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                     "MemPoolGroup[%s][%p] : MemPoolBuffer[%s][%p] Moved from Free to MemPoolBufMgr[%s][%p]",
                                     m_pGroupName, this, pBuffer->name, pBuffer, pMemPoolBufMgr->name, pMemPoolBufMgr);

                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                     "MemPoolGroup[%s][%p]-->MemPoolBufMgr[%s][%p] now has %d in use buffers, %d peak buffers",
                                     m_pGroupName, this, pMemPoolBufMgr->name, pMemPoolBufMgr,
                                     pMemPoolBufMgr->memPoolBufferList.NumNodes(), pMemPoolBufMgr->peakBuffersUsed);

                    m_peakNumBuffersUsed = Utils::MaxUINT(m_peakNumBuffersUsed,
                                                          m_numBuffersAllocated - m_freeBufferList.NumNodes());
                    m_numBuffersIdle     = Utils::MinUINT(m_freeBufferList.NumNodes(), m_numBuffersIdle);

                    if (NULL != pCSLBufferInfo)
                    {
                        *pCSLBufferInfo = pBuffer->bufferInfo;
                    }

                    if (NULL != phGrallocBuffer)
                    {
                        *phGrallocBuffer = pBuffer->hGrallocBuffer;
                    }
                }
                else
                {
                    // Even though we have a free buffer, we failed in mapping the buffer to all required devices.
                    // so this buffer doesn't meet requirements, its a fatal now. Add this buffer back to free list
                    m_freeBufferList.InsertToTail(pNode);
                }
            }
        }

        if (m_freeBufferList.NumNodes() < GetStaticSettings()->MPMKeepMinNumFreeBuffers)
        {
            /// @todo (CAMX-4157) Trigger offline thread to allocate buffers in parallel

            // No more buffers available, keep minimum number of buffers extra always
            // Allocate : GetStaticSettings()->MPMKeepMinNumFreeBuffers - m_freeBufferList.NumNodes()
            // Trigger thread to allocate the buffer offline
        }
    }

    getBufferPoolResult.result          = result;
    getBufferPoolResult.hBufferHandle   = static_cast<MemPoolBufferHandle>(pBuffer);

    m_pLock->Unlock();

    return getBufferPoolResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::ReleaseBufferToPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolGroup::ReleaseBufferToPool(
    MemPoolBufferManager*   pMemPoolBufMgr,
    MemPoolBufferHandle     hMemPoolBufferHandle,
    BOOL                    bufferCached)
{
    CamxResult      result  = CamxResultSuccess;
    MemPoolBuffer*  pBuffer = static_cast<MemPoolBuffer*>(hMemPoolBufferHandle);

    m_pLock->Lock();

    if ((NULL == pMemPoolBufMgr) || (NULL == pBuffer) || (pMemPoolBufMgr != pBuffer->pMemPoolBufMgr))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                       "MemPoolGroup[%s][%p] Invalid inputs pMemPoolBufMgr=%p pBuffer=%p pBuffer->pMemPoolBufMgr=%p",
                       m_pGroupName, this, pMemPoolBufMgr, pBuffer, (NULL != pBuffer) ? pBuffer->pMemPoolBufMgr : NULL);

        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        LDLLNode* pCurrNode = NULL;

        // Check if the incoming buffer manager exists in MemPool Buffer Manager list
        if (NULL == m_bufferManagerList.FindByValue(pMemPoolBufMgr))
        {
            result = CamxResultEInvalidArg;

            CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                           "MemPoolGroup[%s][%p] Invalid input buffer manager, MemPoolBufMgr=[%p] pBuffer=%p",
                           m_pGroupName, this, pMemPoolBufMgr, pBuffer);
        }

        // Check if the incoming buffer exists in MemPool Buffer Manager's in-use list
        if (CamxResultSuccess == result)
        {
            pCurrNode = pMemPoolBufMgr->memPoolBufferList.FindByValue(pBuffer);

            if ((NULL      == pCurrNode) ||
                (pCurrNode != pBuffer->pNode))
            {
                result = CamxResultEInvalidArg;

                CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                               "MemPoolGroup[%s][%p] Invalid input buffer, doesn't exist in in-use list, "
                               "MemPoolBufMgr=[%p][%s] pBuffer=%p, pNode=%p",
                               m_pGroupName, this, pMemPoolBufMgr, pMemPoolBufMgr->name, pBuffer, pCurrNode);

                if (NULL != m_freeBufferList.FindByValue(pBuffer))
                {
                    result = CamxResultEExists;

                    CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                                   "MemPoolGroup[%s][%p] MemPoolBufMgr[%p][%s] MemPoolBuffer[%p][%s] "
                                   "Buffer is already in Group's free list",
                                   m_pGroupName, this, pMemPoolBufMgr, pMemPoolBufMgr->name, pBuffer, pBuffer->name);
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            LDLLNode* pNode = pBuffer->pNode;

            pMemPoolBufMgr->memPoolBufferList.RemoveNode(pNode);

            pBuffer->pMemPoolBufMgr     = NULL;
            pBuffer->bCurrentlyCached   = bufferCached;

            // Add the buffer to the free buffer list
            m_freeBufferList.InsertToTail(pNode);

            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                             "MemPoolGroup[%s][%p] : MemPoolBuffer[%s][%p] Moved from MemPoolBufMgr[%s][%p] to FreeList",
                             m_pGroupName, this, pBuffer->name, pBuffer, pMemPoolBufMgr->name, pMemPoolBufMgr);

            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                             "MemPoolGroup[%s][%p]-->MemPoolBufMgr[%s][%p] now has %d in use buffers, %d peak buffers",
                             m_pGroupName, this, pMemPoolBufMgr->name, pMemPoolBufMgr,
                             pMemPoolBufMgr->memPoolBufferList.NumNodes(), pMemPoolBufMgr->peakBuffersUsed);
        }

        if (CamxResultEExists == result)
        {
            // overwrite as the buffer is already in free list
            result = CamxResultSuccess;
        }
    }

    m_pLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::MapBufferToDevices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolGroup::MapBufferToDevices(
    MemPoolBufferManager*   pMemPoolBufMgr,
    MemPoolBufferHandle     hMemPoolBufferHandle,
    SIZE_T                  offset,
    SIZE_T                  size,
    UINT32                  flags,
    const INT32*            pDeviceIndices,
    UINT                    deviceCount)
{
    CamxResult      result  = CamxResultSuccess;
    MemPoolBuffer*  pBuffer = static_cast<MemPoolBuffer*>(hMemPoolBufferHandle);

    CAMX_UNREFERENCED_PARAM(offset);
    CAMX_UNREFERENCED_PARAM(size);

    if ((NULL == pMemPoolBufMgr) || (NULL == pBuffer) || (pMemPoolBufMgr != pBuffer->pMemPoolBufMgr))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                       "MemPoolGroup[%s][%p] Invalid inputs pMemPoolBufMgr=%p pBuffer=%p pBuffer->pMemPoolBufMgr=%p",
                       m_pGroupName, this, pMemPoolBufMgr, pBuffer, (NULL != pBuffer) ? pBuffer->pMemPoolBufMgr : NULL);

        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        m_pLock->Lock();

        result = CheckAndMapBufferToDevices(pBuffer, pDeviceIndices, deviceCount, flags);

        m_pLock->Unlock();
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::GetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolGroup::GetBufferInfo(
    MemPoolBufferManager*   pMemPoolBufMgr,
    MemPoolBufferHandle     hMemPoolBufferHandle,
    CSLBufferInfo*          pCSLBufferInfo,
    BufferHandle*           phGrallocBuffer
    ) const
{
    CamxResult      result  = CamxResultSuccess;
    MemPoolBuffer*  pBuffer = static_cast<MemPoolBuffer*>(hMemPoolBufferHandle);

    if ((NULL           == pMemPoolBufMgr) ||
        (NULL           == pBuffer)        ||
        (pMemPoolBufMgr != pBuffer->pMemPoolBufMgr))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] Invalid inputs %p %p",
                       m_pGroupName, this, pMemPoolBufMgr, pBuffer);

        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        if (NULL != pCSLBufferInfo)
        {
            *pCSLBufferInfo = pBuffer->bufferInfo;
        }

        if (NULL != phGrallocBuffer)
        {
            *phGrallocBuffer = pBuffer->hGrallocBuffer;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::CanDestroyMemPoolGroup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolGroup::CanDestroyMemPoolGroup() const
{
    BOOL bDestroyMemPoolGroup   = FALSE;
    UINT numBufMgrs;

    m_pLock->Lock();

    PrintMemPoolGroupState(FALSE);

    numBufMgrs = m_bufferManagerList.NumNodes();

    if ((0 == numBufMgrs) && (0 == m_numBuffersAllocated))
    {
        bDestroyMemPoolGroup    = TRUE;
    }

    m_pLock->Unlock();

    return bDestroyMemPoolGroup;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::UpdateBufferAllocProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::UpdateBufferAllocProperties(
    MemPoolBufferManager* pMemPoolBufMgr)
{
    BufferManagerCreateData*    pCreateData     = &pMemPoolBufMgr->createData;
    ImageFormat*                pImageFormat    = &pCreateData->bufferProperties.imageFormat;
    SIZE_T                      currentSize     = ImageFormatUtils::GetTotalSize(pImageFormat) * pCreateData->numBatchedFrames;

    pMemPoolBufMgr->sizeRequired = currentSize;

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "MemPoolGroup[%s][%p] : GroupCurrentAllocProperties : totalBufferSizeBytes=%d, alignment=%d, flags=0x%x, "
                     "stride=%d, scanline=%d, grallocFormat=0x%x, producerFlags=0x%llx, consumerFlags=0x%llx, "
                     "deviceCount=%d, Devices %d %d %d %d %d %d %d %d",
                     m_pGroupName, this,
                     m_bufferAllocProperties.totalBufferSizeBytes,
                     m_bufferAllocProperties.alignment,
                     m_bufferAllocProperties.flags,
                     m_bufferAllocProperties.planeStride,
                     m_bufferAllocProperties.sliceHeight,
                     m_bufferAllocProperties.grallocFormat,
                     m_bufferAllocProperties.producerUsageFlags,
                     m_bufferAllocProperties.consumerUsageFlags,
                     m_bufferAllocProperties.deviceCount,
                     m_bufferAllocProperties.deviceIndices[0],
                     m_bufferAllocProperties.deviceIndices[1],
                     m_bufferAllocProperties.deviceIndices[2],
                     m_bufferAllocProperties.deviceIndices[3],
                     m_bufferAllocProperties.deviceIndices[4],
                     m_bufferAllocProperties.deviceIndices[5],
                     m_bufferAllocProperties.deviceIndices[6],
                     m_bufferAllocProperties.deviceIndices[7]);

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "MemPoolGroup[%s][%p] : BufferManagerAllocProperties : totalBufferSizeBytes=%d, alignment=%d, flags=0x%x, "
                     "stride=%d, scanline=%d, grallocFormat=0x%x, producerFlags=0x%llx, consumerFlags=0x%llx, "
                     "deviceCount=%d, Devices %d %d %d %d %d %d %d %d",
                     m_pGroupName, this,
                     currentSize,
                     pImageFormat->alignment,
                     pCreateData->bufferProperties.memFlags,
                     pImageFormat->formatParams.yuvFormat[0].planeStride,
                     pImageFormat->formatParams.yuvFormat[0].sliceHeight,
                     pCreateData->bufferProperties.grallocFormat,
                     pCreateData->bufferProperties.producerFlags,
                     pCreateData->bufferProperties.consumerFlags,
                     pCreateData->deviceCount,
                     pCreateData->deviceIndices[0],
                     pCreateData->deviceIndices[1],
                     pCreateData->deviceIndices[2],
                     pCreateData->deviceIndices[3],
                     pCreateData->deviceIndices[4],
                     pCreateData->deviceIndices[5],
                     pCreateData->deviceIndices[6],
                     pCreateData->deviceIndices[7]);

    m_bufferAllocProperties.valid = TRUE;

    if (CSLBufferHeapEGL != m_bufferAllocProperties.bufferHeap)
    {
        // If current setting is Gralloc, lets not change it. We can use Gralloc buffers for CSL heap as well.
        m_bufferAllocProperties.bufferHeap = m_bufferHeap;
    }

    m_bufferAllocProperties.totalBufferSizeBytes = Utils::MaxSIZET(currentSize, m_bufferAllocProperties.totalBufferSizeBytes);
    m_bufferAllocProperties.alignment            = Utils::CalculateLCM(static_cast<INT32>(pImageFormat->alignment),
                                                                       static_cast<INT32>(m_bufferAllocProperties.alignment));
    m_bufferAllocProperties.flags               |= pCreateData->bufferProperties.memFlags;

    UINT numDevicesAdded = Utils::AppendArray1ToArray2(pCreateData->deviceIndices,
                                                       pCreateData->deviceCount,
                                                       m_bufferAllocProperties.deviceIndices,
                                                       m_bufferAllocProperties.deviceCount,
                                                       CamxMaxDeviceIndex);

    m_bufferAllocProperties.deviceCount += numDevicesAdded;

    // Used only if allocating using Gralloc
    if (CSLBufferHeapEGL == pCreateData->bufferProperties.bufferHeap)
    {
        UINT32 stride;
        UINT32 scanline;

        if (TRUE == ImageFormatUtils::IsYUV(pImageFormat))
        {
            stride      = pImageFormat->formatParams.yuvFormat[0].planeStride;
            scanline    = pImageFormat->formatParams.yuvFormat[0].sliceHeight;
        }
        else
        {
            stride      = pImageFormat->width;
            scanline    = pImageFormat->height;
        }

        m_bufferAllocProperties.planeStride         = Utils::MaxUINT32(stride, m_bufferAllocProperties.planeStride);
        m_bufferAllocProperties.sliceHeight         = Utils::MaxUINT32(scanline, m_bufferAllocProperties.sliceHeight);

        // Even if we allow grouping, we dont really need to change format. We group only if format is same or both formats
        // have similar stride/scanline requirements. So, we can allocate with any one format and re-use buffer for the other.
        m_bufferAllocProperties.format              = pImageFormat->format;
        m_bufferAllocProperties.grallocFormat       = pCreateData->bufferProperties.grallocFormat;

        m_bufferAllocProperties.producerUsageFlags |= pCreateData->bufferProperties.producerFlags;
        m_bufferAllocProperties.consumerUsageFlags |= pCreateData->bufferProperties.consumerFlags;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "MemPoolGroup[%s][%p] : GroupConsolidatedAllocProperties : totalBufferSizeBytes=%d, alignment=%d, "
                     "flags=0x%x, stride=%d, scanline=%d, grallocFormat=0x%x, producerFlags=0x%llx, consumerFlags=0x%llx",
                     m_pGroupName, this,
                     m_bufferAllocProperties.totalBufferSizeBytes,
                     m_bufferAllocProperties.alignment,
                     m_bufferAllocProperties.flags,
                     m_bufferAllocProperties.planeStride,
                     m_bufferAllocProperties.sliceHeight,
                     m_bufferAllocProperties.grallocFormat,
                     m_bufferAllocProperties.producerUsageFlags,
                     m_bufferAllocProperties.consumerUsageFlags);

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "MemPoolGroup[%s][%p] : GroupConsolidatedAllocProperties : devices=%d, Devices %d %d %d %d %d %d %d %d",
                     m_pGroupName, this,
                     m_bufferAllocProperties.deviceCount,
                     m_bufferAllocProperties.deviceIndices[0],
                     m_bufferAllocProperties.deviceIndices[1],
                     m_bufferAllocProperties.deviceIndices[2],
                     m_bufferAllocProperties.deviceIndices[3],
                     m_bufferAllocProperties.deviceIndices[4],
                     m_bufferAllocProperties.deviceIndices[5],
                     m_bufferAllocProperties.deviceIndices[6],
                     m_bufferAllocProperties.deviceIndices[7]);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::SetupBufferAllocProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::SetupBufferAllocProperties()
{
    // We have multiple Buffer Managers attached to this group.. we could probably find alloc properties to satisy all.
    // This logic should be inline with IsMatchingGroup() logic
    // i.e if IsMatchingGroup allows to bind buffer managers with nearly similar sizes (not exact size),
    // we should allocate max of those size here to satisfy all Buffer Managers.

    // Iterate through all buffer managers attached to this group and populate

    Utils::Memset(&m_bufferAllocProperties, 0x0, sizeof(m_bufferAllocProperties));

    // Lets start with address alignment which satisfies all HW's requirements - something like 4K.
    // If a Buffer Manager needs a different alignment not dividiable by the default value, we can anyway update it
    // in UpdateBufferAllocProperties - which makes alignment as LCM(4K, alignment)
    // This help to avoid going to a different group just because of different alignment alignement requirements.
    m_bufferAllocProperties.alignment = GetStaticSettings()->MPMBufferAddressAlignment;

    if ((TRUE == GetStaticSettings()->autoImageDump)    ||
        (TRUE == GetStaticSettings()->dynamicImageDump) ||
        (TRUE == GetStaticSettings()->watermarkImage))
    {
        m_bufferAllocProperties.flags |= CSLMemFlagUMDAccess;
    }

    LDLLNode* pNode = m_bufferManagerList.Head();

    while (NULL != pNode)
    {
        MemPoolBufferManager* pMemPoolBufMgr = static_cast<MemPoolBufferManager*>(pNode->pData);

        CAMX_ASSERT(NULL != pMemPoolBufMgr);

        UpdateBufferAllocProperties(pMemPoolBufMgr);

        pNode = LightweightDoublyLinkedList::NextNode(pNode);

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::AllocateBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 MemPoolGroup::AllocateBuffers(
    UINT32 numBuffersToAllocate)
{
    CamxResult  result              = CamxResultSuccess;
    UINT32      numBuffersAllocated = 0;

    CAMX_ASSERT(TRUE                        == m_bufferAllocProperties.valid);
    CAMX_ASSERT(MemPoolInvalidBufferCount   != numBuffersToAllocate);

    if (MemPoolInvalidBufferCount == numBuffersToAllocate)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolGroup[%s] : Incorrect number of buffers to allocate %d",
                       m_pGroupName, numBuffersToAllocate);
        result = CamxResultEFailed;
    }

    for (UINT32 i = 0; ((i < numBuffersToAllocate) && (CamxResultSuccess == result)); i++)
    {
        MemPoolBuffer*  pBuffer = static_cast<MemPoolBuffer*>(CAMX_CALLOC(sizeof(MemPoolBuffer)));
        LDLLNode*       pNode   = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));

        if ((NULL == pBuffer) || (NULL == pNode))
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Insufficient memory %p %p", pBuffer, pNode);
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            // set to -1 as 0 is a valid fd value
            pBuffer->bufferInfo.fd = -1;

            OsUtils::SNPrintF(pBuffer->name, MaxStringLength64, "%s_Buffer%d", m_pGroupName, ++m_bufferNameCounter);

            if ((CSLBufferHeapIon == m_bufferAllocProperties.bufferHeap))
            {
                /// @todo (CAMX-954) Does alignment need to be handled separately for numBatchedFrames > 1?
                result = CSLAlloc(pBuffer->name,
                                  &pBuffer->bufferInfo,
                                  m_bufferAllocProperties.totalBufferSizeBytes,
                                  m_bufferAllocProperties.alignment,
                                  m_bufferAllocProperties.flags,
                                  m_bufferAllocProperties.deviceIndices,
                                  m_bufferAllocProperties.deviceCount);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in CSL Alloc %d %d %d, result=%s",
                                   m_bufferAllocProperties.totalBufferSizeBytes,
                                   m_bufferAllocProperties.alignment,
                                   m_bufferAllocProperties.flags,
                                   Utils::CamxResultToString(result));
                }
            }
            else
            {
                Gralloc* pGralloc = Gralloc::GetInstance();

                pBuffer->hGrallocBuffer = NULL;

                if (NULL != pGralloc)
                {
                    // Due to Gralloc limitation, buffers with HAL_PIXEL_FORMAT_BLOB must have a height of 1.
                    // Width is modified(width*height) to maintain the size of buffer
                    if (HALPixelFormatBlob == m_bufferAllocProperties.grallocFormat)
                    {
                        UINT32 planeStride = m_bufferAllocProperties.planeStride * m_bufferAllocProperties.sliceHeight;
                        UINT32 sliceHeight = 1;

                        result = pGralloc->AllocateGrallocBuffer(planeStride,
                                                                 sliceHeight,
                                                                 m_bufferAllocProperties.grallocFormat,
                                                                 &pBuffer->hGrallocBuffer,
                                                                 &pBuffer->allocatedStride,
                                                                 m_bufferAllocProperties.producerUsageFlags,
                                                                 m_bufferAllocProperties.consumerUsageFlags);
                    }
                    else
                    {
                        result = pGralloc->AllocateGrallocBuffer(m_bufferAllocProperties.planeStride,
                                                                 m_bufferAllocProperties.sliceHeight,
                                                                 m_bufferAllocProperties.grallocFormat,
                                                                 &pBuffer->hGrallocBuffer,
                                                                 &pBuffer->allocatedStride,
                                                                 m_bufferAllocProperties.producerUsageFlags,
                                                                 m_bufferAllocProperties.consumerUsageFlags);
                    }

                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                     "MemPoolBuffer[%s][%p] : Allocating widthxheight %dx%d stride=%d, hGrallocBuffer=%p, "
                                     "BufferSizeBytes=%d, CSLFlags=0x%x, producerUsageFlags=0x%x, consumerUsageFlags=0x%x, "
                                     "deviceCount=%d",
                                     pBuffer->name, pBuffer,
                                     m_bufferAllocProperties.planeStride,
                                     m_bufferAllocProperties.sliceHeight,
                                     pBuffer->allocatedStride,
                                     pBuffer->hGrallocBuffer,
                                     m_bufferAllocProperties.totalBufferSizeBytes,
                                     m_bufferAllocProperties.flags,
                                     m_bufferAllocProperties.producerUsageFlags,
                                     m_bufferAllocProperties.consumerUsageFlags,
                                     m_bufferAllocProperties.deviceCount);

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                                       "MemPoolGroup[%s] : Error in allocating Gralloc Buffer, width=%d, height=%d, format=0x%x"
                                       ", producerUsageFlags=0x%llx, consumerUsageFlags=0x%llx",
                                       m_pGroupName, m_bufferAllocProperties.planeStride, m_bufferAllocProperties.sliceHeight,
                                       m_bufferAllocProperties.grallocFormat, m_bufferAllocProperties.producerUsageFlags,
                                       m_bufferAllocProperties.consumerUsageFlags);
                    }

                    // If no devices or no cpu access flags, nothing to CSLMap, CLients work on Gralloc handle
                    if ((CamxResultSuccess == result) &&
                        (0 != m_bufferAllocProperties.totalBufferSizeBytes) &&
                        ((0 != m_bufferAllocProperties.flags) || (0 != m_bufferAllocProperties.deviceCount)))
                    {
                        BufferHandle*       phNativeHandle  = reinterpret_cast<BufferHandle*>(&pBuffer->hGrallocBuffer);
                        const NativeHandle* phNativeBuffer  = *phNativeHandle;

                        result = CSLMapNativeBuffer(&pBuffer->bufferInfo,
                                                    reinterpret_cast<const CSLNativeHandle*>(phNativeBuffer),
                                                    0, // offset
                                                    m_bufferAllocProperties.totalBufferSizeBytes,
                                                    m_bufferAllocProperties.flags,
                                                    m_bufferAllocProperties.deviceIndices,
                                                    m_bufferAllocProperties.deviceCount);

                        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolBuffer[%s] : For [%dx%d] size=%d :: "
                                         "Gralloc handle=%p, stride=%d, hHandle=%d, pVirtualAddr=%p, deviceAddr=0x%x, fd=%d, "
                                         "offset=%d, size=%d, flags=0x%x",
                                         pBuffer->name,
                                         m_bufferAllocProperties.planeStride,
                                         m_bufferAllocProperties.sliceHeight,
                                         m_bufferAllocProperties.totalBufferSizeBytes,
                                         pBuffer->hGrallocBuffer,
                                         pBuffer->allocatedStride,
                                         pBuffer->bufferInfo.hHandle,
                                         pBuffer->bufferInfo.pVirtualAddr,
                                         pBuffer->bufferInfo.deviceAddr,
                                         pBuffer->bufferInfo.fd,
                                         pBuffer->bufferInfo.offset,
                                         pBuffer->bufferInfo.size,
                                         pBuffer->bufferInfo.flags);

                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in CSLMapNative, result=%s",
                                           Utils::CamxResultToString(result));

                            if (CamxResultSuccess == pGralloc->Destroy(pBuffer->hGrallocBuffer))
                            {
                                pBuffer->hGrallocBuffer = NULL;
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "ERROR: Release called on an invalid handle");
                            }
                        }
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "ERROR: Unable to get gralloc handle");
                    result = CamxResultEInvalidPointer;
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            numBuffersAllocated++;

            // Save buffer allocation properties in MemPoolBuffer holder

            pBuffer->deviceCount = m_bufferAllocProperties.deviceCount;

            if (0 < pBuffer->deviceCount)
            {
                Utils::Memcpy(pBuffer->deviceIndices, m_bufferAllocProperties.deviceIndices,
                              (sizeof(pBuffer->deviceIndices[0]) * pBuffer->deviceCount));
            }

            pBuffer->bufferHeap     = m_bufferAllocProperties.bufferHeap;
            pBuffer->pMemPoolGroup  = this;
            pBuffer->allocatedSize  = m_bufferAllocProperties.totalBufferSizeBytes;

            pNode->pData    = pBuffer;
            pBuffer->pNode  = pNode;

            m_freeBufferList.InsertToTail(pNode);
            m_numBuffersAllocated++;

            if (0 == m_peakNumBuffersAllocated)
            {
                // First allocation in this group
                m_leastBufferSizeAllocated  = m_bufferAllocProperties.totalBufferSizeBytes;
                m_maxBufferSizeAllocated    = m_bufferAllocProperties.totalBufferSizeBytes;
            }
            else
            {
                m_leastBufferSizeAllocated  = Utils::MinSIZET(m_leastBufferSizeAllocated,
                                                              m_bufferAllocProperties.totalBufferSizeBytes);
                m_maxBufferSizeAllocated    = Utils::MaxSIZET(m_maxBufferSizeAllocated,
                                                              m_bufferAllocProperties.totalBufferSizeBytes);
            }

            m_peakNumBuffersAllocated = Utils::MaxUINT(m_peakNumBuffersAllocated, m_numBuffersAllocated);

            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                             "MemPoolGroup[%s] MemPoolBuffer[%s][%p] Buffers Allocated : Total=%d Free= %d Busy= %d",
                             m_pGroupName, pBuffer->name, pBuffer,
                             m_numBuffersAllocated,
                             m_freeBufferList.NumNodes(),
                             (m_numBuffersAllocated - m_freeBufferList.NumNodes()));

            if (TRUE == GetStaticSettings()->enableMemoryStats)
            {
                s_pMPGStatsLock->Lock();

                s_memPoolMgrStats.numBuffersAllocated++;
                s_memPoolMgrStats.peakNumBuffersAllocated      = Utils::MaxUINT(s_memPoolMgrStats.peakNumBuffersAllocated,
                                                                                s_memPoolMgrStats.numBuffersAllocated);

                s_memPoolMgrStats.sizeOfMemoryAllocated       += pBuffer->allocatedSize;
                s_memPoolMgrStats.peakSizeOfMemoryAllocated    = Utils::MaxSIZET(s_memPoolMgrStats.peakSizeOfMemoryAllocated,
                                                                                 s_memPoolMgrStats.sizeOfMemoryAllocated);

                CAMX_LOG_INFO(CamxLogGroupMemMgr,
                              "MemPoolGroup[%s]-->MemPoolBuffer[%s] : MemPoolMgr Group Stats : "
                              "CurrNum=%d, PeakNum=%d, SizePerBuffer=%zu",
                              m_pGroupName, pBuffer->name,
                              m_numBuffersAllocated,   m_peakNumBuffersAllocated, pBuffer->allocatedSize);

                CAMX_LOG_INFO(CamxLogGroupMemMgr,
                              "MemPoolGroup[%s]-->MemPoolBuffer[%s] : MemPoolMgr Overall Stats : "
                              "CurrNum=%d, PeakNum=%d, CurrSize=%zu, PeakSize=%zu",
                              m_pGroupName, pBuffer->name,
                              s_memPoolMgrStats.numBuffersAllocated,   s_memPoolMgrStats.peakNumBuffersAllocated,
                              s_memPoolMgrStats.sizeOfMemoryAllocated, s_memPoolMgrStats.peakSizeOfMemoryAllocated);

                if (0 != (m_bufferAllocProperties.flags & CSLMemFlagUMDAccess))
                {
                    // we mapped the allocated buffer to CPU, Devices
                    s_memPoolMgrStats.numBuffersMappedToCPU++;
                    s_memPoolMgrStats.peakNumBuffersMappedToCPU    =
                        Utils::MaxUINT(s_memPoolMgrStats.peakNumBuffersMappedToCPU, s_memPoolMgrStats.numBuffersMappedToCPU);

                    s_memPoolMgrStats.sizeOfMemoryMappedToCPU     += pBuffer->allocatedSize;
                    s_memPoolMgrStats.peakSizeOfMemoryMappedToCPU  =
                        Utils::MaxSIZET(s_memPoolMgrStats.peakSizeOfMemoryMappedToCPU,
                        s_memPoolMgrStats.sizeOfMemoryMappedToCPU);
                }

                if (0 != m_bufferAllocProperties.deviceCount)
                {
                    // we mapped the allocated buffer to hw device(s)
                    // Note - we are saving stats by combining all devices, not per-device stats
                    // we mapped the allocated buffer to CPU, Devices
                    s_memPoolMgrStats.numBuffersMappedToDevices++;
                    s_memPoolMgrStats.peakNumBuffersMappedToDevices    = Utils::MaxUINT(
                                                                         s_memPoolMgrStats.peakNumBuffersMappedToDevices,
                                                                         s_memPoolMgrStats.numBuffersMappedToDevices);

                    s_memPoolMgrStats.sizeOfMemoryMappedToDevices     += pBuffer->allocatedSize;
                    s_memPoolMgrStats.peakSizeOfMemoryMappedToDevices  = Utils::MaxSIZET(
                                                                         s_memPoolMgrStats.peakSizeOfMemoryMappedToDevices,
                                                                         s_memPoolMgrStats.sizeOfMemoryMappedToDevices);
                }

                s_pMPGStatsLock->Unlock();
            }
        }

        if (CamxResultSuccess != result)
        {
            if (NULL != pBuffer)
            {
                CAMX_FREE(pBuffer);
                pBuffer = NULL;
            }

            if (NULL != pNode)
            {
                CAMX_FREE(pNode);
                pNode = NULL;
            }
        }
    }

    return numBuffersAllocated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::FreeBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 MemPoolGroup::FreeBuffers(
    UINT32 numBuffersToFree)
{
    MemPoolBuffer*  pBuffer;
    UINT32          numBuffersFreed = 0;
    CamxResult      result          = CamxResultSuccess;

    CAMX_ASSERT(MemPoolInvalidBufferCount != numBuffersToFree);

    if (0 < numBuffersToFree)
    {
        while (numBuffersFreed != numBuffersToFree)
        {
            LDLLNode* pNode = m_freeBufferList.RemoveFromHead();

            if (NULL == pNode)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Empty Free list, freed=%d, requested=%d",
                                 m_pGroupName, this, numBuffersFreed, numBuffersToFree);
                break;
            }

            if (NULL == pNode->pData)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : NULL data in Node[%p], freed=%d, requested=%d",
                               m_pGroupName, this, pNode, numBuffersFreed, numBuffersToFree);
            }
            else
            {
                pBuffer = static_cast<MemPoolBuffer*>(pNode->pData);

                CAMX_ASSERT(NULL != pBuffer);

                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                 "MemPoolGroup[%s] MemPoolBuffer[%s][%p] Freeing Buffer : Total=%d Free= %d Busy= %d",
                                 m_pGroupName, pBuffer->name, pBuffer,
                                 m_numBuffersAllocated,
                                 m_freeBufferList.NumNodes(),
                                 (m_numBuffersAllocated - m_freeBufferList.NumNodes()));

                PrintMemPoolBufferState(pBuffer, FALSE);

                // If we have done any incremental mappings, release all of them first.
                // This can happen if, the buffer is initiall mapped to one device while allocating and then later
                // mapped to a 2nd device.
                for (UINT i = 0; i < MaxIncrBufMappings; i++)
                {
                    if (0 != pBuffer->incrBufferInfo[i].hHandle)
                    {
                        result = CSLReleaseBuffer(pBuffer->incrBufferInfo[i].hHandle);

                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                                           "MemPoolGroup[%s][%p] : Failed in CSLReleaseBuffer[%d] handle=%d, result=%s",
                                           m_pGroupName, this, i, pBuffer->incrBufferInfo[i].hHandle,
                                           Utils::CamxResultToString(result));
                        }

                        if (TRUE == GetStaticSettings()->enableMemoryStats)
                        {
                            s_pMPGStatsLock->Lock();

                            if (0 != (pBuffer->incrBufferInfo[i].flags & CSLMemFlagUMDAccess))
                            {
                                // we mapped the allocated buffer to CPU, Devices
                                s_memPoolMgrStats.numBuffersMappedToCPU--;
                                s_memPoolMgrStats.sizeOfMemoryMappedToCPU -= pBuffer->allocatedSize;
                            }

                            if (0 != pBuffer->deviceCount)
                            {
                                // we mapped the allocated buffer to hw device(s)
                                // Note - we are saving stats by combining all devices, not per-device stats
                                // we mapped the allocated buffer to CPU, Devices
                                if (s_memPoolMgrStats.numBuffersMappedToDevices > 0)
                                {
                                    s_memPoolMgrStats.numBuffersMappedToDevices--;
                                }
                                if (s_memPoolMgrStats.sizeOfMemoryMappedToDevices > pBuffer->allocatedSize)
                                {
                                    s_memPoolMgrStats.sizeOfMemoryMappedToDevices -= pBuffer->allocatedSize;
                                }
                            }

                            s_pMPGStatsLock->Unlock();
                        }
                    }
                }

                if (0 != pBuffer->bufferInfo.hHandle)
                {
                    result = CSLReleaseBuffer(pBuffer->bufferInfo.hHandle);

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                                       "MemPoolGroup[%s][%p] : Failed in CSLReleaseBuffer handle=%d, result=%s",
                                       m_pGroupName, this, pBuffer->bufferInfo.hHandle, Utils::CamxResultToString(result));
                    }

                    if (TRUE == GetStaticSettings()->enableMemoryStats)
                    {
                        s_pMPGStatsLock->Lock();

                        if (0 != (pBuffer->bufferInfo.flags & CSLMemFlagUMDAccess))
                        {
                            // we mapped the allocated buffer to CPU, Devices
                            s_memPoolMgrStats.numBuffersMappedToCPU--;
                            s_memPoolMgrStats.sizeOfMemoryMappedToCPU -= pBuffer->allocatedSize;
                        }

                        if (0 != pBuffer->deviceCount)
                        {
                            // we mapped the allocated buffer to hw device(s)
                            // Note - we are saving stats by combining all devices, not per-device stats
                            // we mapped the allocated buffer to CPU, Devices
                            if (s_memPoolMgrStats.numBuffersMappedToDevices > 0)
                            {
                                s_memPoolMgrStats.numBuffersMappedToDevices--;
                            }
                            if (s_memPoolMgrStats.sizeOfMemoryMappedToDevices > pBuffer->allocatedSize)
                            {
                                s_memPoolMgrStats.sizeOfMemoryMappedToDevices -= pBuffer->allocatedSize;
                            }
                        }

                        s_pMPGStatsLock->Unlock();
                    }
                }

                if ((CSLBufferHeapEGL == m_bufferHeap) && (NULL != pBuffer->hGrallocBuffer))
                {
                    Gralloc* pGralloc = Gralloc::GetInstance();

                    if (NULL != pGralloc)
                    {
                        if (CamxResultSuccess != pGralloc->Destroy(pBuffer->hGrallocBuffer))
                        {
                            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolBuffer[%s] : Failed in destroying Gralloc handle",
                                           pBuffer->name);
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolBuffer[%s] : Invalid Gralloc instance, can't release handle",
                                       pBuffer->name);
                    }
                }

                m_numBuffersAllocated--;
                numBuffersFreed++;

                if (TRUE == GetStaticSettings()->enableMemoryStats)
                {
                    s_pMPGStatsLock->Lock();
                    s_memPoolMgrStats.numBuffersAllocated--;
                    s_memPoolMgrStats.sizeOfMemoryAllocated -= pBuffer->allocatedSize;

                    CAMX_LOG_INFO(CamxLogGroupMemMgr,
                                  "MemPoolGroup[%s]-->MemPoolBuffer[%s] : MemPoolMgr Group Stats : "
                                  "CurrNum=%d, PeakNum=%d, SizePerBuffer=%zu",
                                  m_pGroupName, pBuffer->name,
                                  m_numBuffersAllocated, m_peakNumBuffersAllocated, pBuffer->allocatedSize);

                    CAMX_LOG_INFO(CamxLogGroupMemMgr,
                                  "MemPoolGroup[%s]-->MemPoolBuffer[%s] : MemPoolMgr Overall Stats : "
                                  "CurrNum=%d, PeakNum=%d, CurrSize=%zu, PeakSize=%zu",
                                  m_pGroupName, pBuffer->name,
                                  s_memPoolMgrStats.numBuffersAllocated, s_memPoolMgrStats.peakNumBuffersAllocated,
                                  s_memPoolMgrStats.sizeOfMemoryAllocated, s_memPoolMgrStats.peakSizeOfMemoryAllocated);

                    s_pMPGStatsLock->Unlock();
                }

                CAMX_FREE(pBuffer);
                pBuffer = NULL;

                CAMX_FREE(pNode);
                pNode = NULL;
            }
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "MemPoolGroup[%s][%p] : Freed %d buffers (requested=%d), Remaining : total=%d, InUse=%d, Free=%d",
                     m_pGroupName, this, numBuffersFreed, numBuffersToFree, m_numBuffersAllocated,
                     (m_numBuffersAllocated - m_freeBufferList.NumNodes()), m_freeBufferList.NumNodes());

    if (numBuffersFreed != numBuffersToFree)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Insufficient buffers to free :Freed %d buffers "
                         "(requested=%d), Remaining : total=%d, InUse=%d, Free=%d",
                         m_pGroupName, this, numBuffersFreed, numBuffersToFree,
                         m_numBuffersAllocated, (m_numBuffersAllocated - m_freeBufferList.NumNodes()),
                         m_freeBufferList.NumNodes());
    }

    return numBuffersFreed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::PrintMemPoolBufMgrState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::PrintMemPoolBufMgrState(
    MemPoolBufferManager*   pMemPoolBufMgr,
    BOOL                    forceTrigger
    ) const
{
    if ((TRUE == forceTrigger) || (TRUE == GetStaticSettings()->enableMPMStatelogging))
    {
        CAMX_LOG_CONFIG(CamxLogGroupMemMgr,
                        "MemPoolGroup[%s]-->MemPoolBufMgr[%s] : Activated=%d, bEverActivated=%d InUse=%d, PeakUsed=%d",
                        m_pGroupName, pMemPoolBufMgr->name, pMemPoolBufMgr->bActivated, pMemPoolBufMgr->bEverActivated,
                        pMemPoolBufMgr->memPoolBufferList.NumNodes(), pMemPoolBufMgr->peakBuffersUsed);

        LDLLNode* pNode = pMemPoolBufMgr->memPoolBufferList.Head();

        while (NULL != pNode)
        {
            MemPoolBuffer* pMemPoolBuffer = static_cast<MemPoolBuffer*>(pNode->pData);

            CAMX_ASSERT(NULL != pMemPoolBuffer);

            PrintMemPoolBufferState(pMemPoolBuffer, forceTrigger);

            pNode = LightweightDoublyLinkedList::NextNode(pNode);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::PrintMemPoolBufferState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::PrintMemPoolBufferState(
    MemPoolBuffer*  pMemPoolBuffer,
    BOOL            forceTrigger
    ) const
{
    if ((TRUE == forceTrigger) || (TRUE == GetStaticSettings()->enableMPMStatelogging))
    {
        BOOL bBuffersInUse = FALSE;

        CAMX_LOG_INFO(CamxLogGroupMemMgr,
                      "MemPoolGroup[%s]-->MemPoolBuffer[%s] : heap=%d, currBufMgr=%s, cached=%d, "
                      "deviceCount=%d, Devices %d %d %d %d %d",
                      m_pGroupName,
                      pMemPoolBuffer->name,
                      pMemPoolBuffer->bufferHeap,
                      (NULL == pMemPoolBuffer->pMemPoolBufMgr) ? "None" : pMemPoolBuffer->pMemPoolBufMgr->name,
                      pMemPoolBuffer->bCurrentlyCached,
                      pMemPoolBuffer->deviceCount,
                      pMemPoolBuffer->deviceIndices[0],
                      pMemPoolBuffer->deviceIndices[1],
                      pMemPoolBuffer->deviceIndices[2],
                      pMemPoolBuffer->deviceIndices[3],
                      pMemPoolBuffer->deviceIndices[4],
                      pMemPoolBuffer->deviceIndices[5]);

        CAMX_LOG_INFO(CamxLogGroupMemMgr,
                      "MemPoolGroup[%s]-->MemPoolBuffer[%s] : BufferInfo : hHandle=%d, (%d, %d %d), pVirtualAddr=%p, fd=%d, "
                      "offset=%d, size=%d, flags=%d",
                      m_pGroupName,
                      pMemPoolBuffer->name,
                      pMemPoolBuffer->bufferInfo.hHandle,
                      pMemPoolBuffer->incrBufferInfo[0].hHandle,
                      pMemPoolBuffer->incrBufferInfo[1].hHandle,
                      pMemPoolBuffer->incrBufferInfo[2].hHandle,
                      pMemPoolBuffer->bufferInfo.pVirtualAddr,
                      pMemPoolBuffer->bufferInfo.fd,
                      pMemPoolBuffer->bufferInfo.offset,
                      pMemPoolBuffer->bufferInfo.size,
                      pMemPoolBuffer->bufferInfo.flags);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::PrintMemPoolGroupState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::PrintMemPoolGroupState(
    BOOL    forceTrigger
    ) const
{
    if ((TRUE == forceTrigger) || (TRUE == GetStaticSettings()->enableMPMStatelogging))
    {
        CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Printing state------------------------------",
                        m_pGroupName, this);

        CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Consolidated Registered Buffer count : "
                      "immediateAllocBufferCount(min=%d, max=%d, sum=%d), maxBufferCount(min=%d, max=%d, sum=%d)",
                      m_pGroupName, this,
                      m_registeredBufferCount.immediateAllocBufferCount.min,
                      m_registeredBufferCount.immediateAllocBufferCount.max,
                      m_registeredBufferCount.immediateAllocBufferCount.sum,
                      m_registeredBufferCount.maxBufferCount.min,
                      m_registeredBufferCount.maxBufferCount.max,
                      m_registeredBufferCount.maxBufferCount.sum);

        CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Consolidated Activated Buffer count : "
                      "immediateAllocBufferCount(min=%d, max=%d, sum=%d), maxBufferCount(min=%d, max=%d, sum=%d)",
                      m_pGroupName, this,
                      m_activatedBufferCount.immediateAllocBufferCount.min,
                      m_activatedBufferCount.immediateAllocBufferCount.max,
                      m_activatedBufferCount.immediateAllocBufferCount.sum,
                      m_activatedBufferCount.maxBufferCount.min,
                      m_activatedBufferCount.maxBufferCount.max,
                      m_activatedBufferCount.maxBufferCount.sum);

        CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : BufferAllocProps : size=%zu, flags=0x%x, dvices=%d",
                        m_pGroupName, this, m_bufferAllocProperties.totalBufferSizeBytes, m_bufferAllocProperties.flags,
                        m_bufferAllocProperties.deviceCount);

        CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : --------------Buffer Managers : Registered=%d, Active=%d",
                        m_pGroupName, this, m_bufferManagerList.NumNodes(), m_numActiveBufMgrs);

        LDLLNode* pNode = m_bufferManagerList.Head();

        while (NULL != pNode)
        {
            MemPoolBufferManager* pMemPoolBufMgr = static_cast<MemPoolBufferManager*>(pNode->pData);

            CAMX_ASSERT(NULL != pMemPoolBufMgr);

            PrintMemPoolBufMgrState(pMemPoolBufMgr, forceTrigger);

            pNode = LightweightDoublyLinkedList::NextNode(pNode);
        }

        CAMX_LOG_CONFIG(CamxLogGroupMemMgr,
                        "MemPoolGroup[%s][%p] : Buffers : Allocated=%d, Inuse=%d, Free=%d "
                        "PeakAllocated=%d, PeakUsed=%d, idle=%d",
                        m_pGroupName,                   this,
                        m_numBuffersAllocated,          (m_numBuffersAllocated - m_freeBufferList.NumNodes()),
                        m_freeBufferList.NumNodes(),    m_peakNumBuffersAllocated,
                        m_peakNumBuffersUsed,           m_numBuffersIdle);

        pNode = m_freeBufferList.Head();

        while (NULL != pNode)
        {
            MemPoolBuffer* pMemPoolBuffer = static_cast<MemPoolBuffer*>(pNode->pData);

            CAMX_ASSERT(NULL != pMemPoolBuffer);

            PrintMemPoolBufferState(pMemPoolBuffer, forceTrigger);

            pNode = LightweightDoublyLinkedList::NextNode(pNode);
        }

        CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Printing state Done---------------------",
                      m_pGroupName, this);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::ResetMemoryPoolManagerStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::ResetMemoryPoolManagerStats()
{
    if (TRUE == GetStaticSettings()->enableMemoryStats)
    {
        Utils::Memset(&s_memPoolMgrStats, 0x0, sizeof(s_memPoolMgrStats));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::PrintMemoryPoolManagerStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::PrintMemoryPoolManagerStats(
    BOOL    forceTrigger)
{
    if ((TRUE == forceTrigger) || (TRUE == GetStaticSettings()->enableMemoryStats))
    {
        CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "MemPoolMgrStats : Allocation : num=%d, peakNum=%d, size=%zu, peakSize=%zu",
                        s_memPoolMgrStats.numBuffersAllocated,   s_memPoolMgrStats.peakNumBuffersAllocated,
                        s_memPoolMgrStats.sizeOfMemoryAllocated, s_memPoolMgrStats.peakSizeOfMemoryAllocated);

        CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "MemPoolMgrStats : MapsCPU : num=%d, peakNum=%d, size=%zu, peakSize=%zu; "
                        "MapsDevices : num=%d, peakNum=%d, size=%zu, peakSize=%zu",
                        s_memPoolMgrStats.numBuffersMappedToCPU,       s_memPoolMgrStats.peakNumBuffersMappedToCPU,
                        s_memPoolMgrStats.sizeOfMemoryMappedToCPU,     s_memPoolMgrStats.peakSizeOfMemoryMappedToCPU,
                        s_memPoolMgrStats.numBuffersMappedToDevices,   s_memPoolMgrStats.peakNumBuffersMappedToDevices,
                        s_memPoolMgrStats.sizeOfMemoryMappedToDevices, s_memPoolMgrStats.peakSizeOfMemoryMappedToDevices);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::SetupMemPoolGroupBufferCounts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::SetupMemPoolGroupBufferCounts(
    BOOL bRegisterBufferCount,
    BOOL bActivatedBufferCount)
{
    if (TRUE == bRegisterBufferCount)
    {
        Utils::Memset(&m_registeredBufferCount, 0x0, sizeof(m_registeredBufferCount));
        m_registeredBufferCount.immediateAllocBufferCount.min = MemPoolInvalidBufferCount;
        m_registeredBufferCount.maxBufferCount.min            = MemPoolInvalidBufferCount;
    }

    if (TRUE == bActivatedBufferCount)
    {
        Utils::Memset(&m_activatedBufferCount, 0x0, sizeof(m_registeredBufferCount));
        m_activatedBufferCount.immediateAllocBufferCount.min = MemPoolInvalidBufferCount;
        m_activatedBufferCount.maxBufferCount.min            = MemPoolInvalidBufferCount;
    }

    LDLLNode* pNode = m_bufferManagerList.Head();

    while (NULL != pNode)
    {
        MemPoolBufferManager* pMemPoolBufMgr = static_cast<MemPoolBufferManager*>(pNode->pData);

        CAMX_ASSERT(NULL != pMemPoolBufMgr);

        UpdateMemPoolGroupBufferCounts(pMemPoolBufMgr, bRegisterBufferCount, bActivatedBufferCount);

        pNode = LightweightDoublyLinkedList::NextNode(pNode);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Consolidated Registered Buffer count : "
                     "immediateAllocBufferCount(min=%d, max=%d, sum=%d), maxBufferCount(min=%d, max=%d, sum=%d)",
                     m_pGroupName, this,
                     m_registeredBufferCount.immediateAllocBufferCount.min,
                     m_registeredBufferCount.immediateAllocBufferCount.max,
                     m_registeredBufferCount.immediateAllocBufferCount.sum,
                     m_registeredBufferCount.maxBufferCount.min,
                     m_registeredBufferCount.maxBufferCount.max,
                     m_registeredBufferCount.maxBufferCount.sum);

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Consolidated Activated Buffer count : "
                     "immediateAllocBufferCount(min=%d, max=%d, sum=%d), maxBufferCount(min=%d, max=%d, sum=%d)",
                     m_pGroupName, this,
                     m_activatedBufferCount.immediateAllocBufferCount.min,
                     m_activatedBufferCount.immediateAllocBufferCount.max,
                     m_activatedBufferCount.immediateAllocBufferCount.sum,
                     m_activatedBufferCount.maxBufferCount.min,
                     m_activatedBufferCount.maxBufferCount.max,
                     m_activatedBufferCount.maxBufferCount.sum);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::UpdateMemPoolGroupBufferCounts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::UpdateMemPoolGroupBufferCounts(
    MemPoolBufferManager*   pMemPoolBufMgr,
    BOOL                    bRegisterBufferCount,
    BOOL                    bActivatedBufferCount)
{
    if (TRUE == bRegisterBufferCount)
    {
        m_registeredBufferCount.immediateAllocBufferCount.min  = Utils::MinUINT(
                                                                 m_registeredBufferCount.immediateAllocBufferCount.min,
                                                                 pMemPoolBufMgr->createData.immediateAllocBufferCount);
        m_registeredBufferCount.immediateAllocBufferCount.max  = Utils::MaxUINT(
                                                                 m_registeredBufferCount.immediateAllocBufferCount.max,
                                                                 pMemPoolBufMgr->createData.immediateAllocBufferCount);
        m_registeredBufferCount.immediateAllocBufferCount.sum += pMemPoolBufMgr->createData.immediateAllocBufferCount;

        m_registeredBufferCount.maxBufferCount.min             = Utils::MinUINT(
                                                                 m_registeredBufferCount.maxBufferCount.min,
                                                                 pMemPoolBufMgr->createData.maxBufferCount);
        m_registeredBufferCount.maxBufferCount.max             = Utils::MaxUINT(
                                                                 m_registeredBufferCount.maxBufferCount.max,
                                                                 pMemPoolBufMgr->createData.maxBufferCount);
        m_registeredBufferCount.maxBufferCount.sum            += pMemPoolBufMgr->createData.maxBufferCount;
    }

    if ((TRUE == bActivatedBufferCount) && (TRUE == pMemPoolBufMgr->bActivated))
    {
        m_activatedBufferCount.immediateAllocBufferCount.min  = Utils::MinUINT(
                                                                m_activatedBufferCount.immediateAllocBufferCount.min,
                                                                pMemPoolBufMgr->createData.immediateAllocBufferCount);
        m_activatedBufferCount.immediateAllocBufferCount.max  = Utils::MaxUINT(
                                                                m_activatedBufferCount.immediateAllocBufferCount.max,
                                                                pMemPoolBufMgr->createData.immediateAllocBufferCount);
        m_activatedBufferCount.immediateAllocBufferCount.sum += pMemPoolBufMgr->createData.immediateAllocBufferCount;

        m_activatedBufferCount.maxBufferCount.min             = Utils::MinUINT(
                                                                m_activatedBufferCount.maxBufferCount.min,
                                                                pMemPoolBufMgr->createData.maxBufferCount);
        m_activatedBufferCount.maxBufferCount.max             = Utils::MaxUINT(
                                                                m_activatedBufferCount.maxBufferCount.max,
                                                                pMemPoolBufMgr->createData.maxBufferCount);
        m_activatedBufferCount.maxBufferCount.sum            += pMemPoolBufMgr->createData.maxBufferCount;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Consolidated Registered Buffer count : "
                     "immediateAllocBufferCount(min=%d, max=%d, sum=%d), maxBufferCount(min=%d, max=%d, sum=%d)",
                     m_pGroupName, this,
                     m_registeredBufferCount.immediateAllocBufferCount.min,
                     m_registeredBufferCount.immediateAllocBufferCount.max,
                     m_registeredBufferCount.immediateAllocBufferCount.sum,
                     m_registeredBufferCount.maxBufferCount.min,
                     m_registeredBufferCount.maxBufferCount.max,
                     m_registeredBufferCount.maxBufferCount.sum);

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Consolidated Activated Buffer count : "
                     "immediateAllocBufferCount(min=%d, max=%d, sum=%d), maxBufferCount(min=%d, max=%d, sum=%d)",
                     m_pGroupName, this,
                     m_activatedBufferCount.immediateAllocBufferCount.min,
                     m_activatedBufferCount.immediateAllocBufferCount.max,
                     m_activatedBufferCount.immediateAllocBufferCount.sum,
                     m_activatedBufferCount.maxBufferCount.min,
                     m_activatedBufferCount.maxBufferCount.max,
                     m_activatedBufferCount.maxBufferCount.sum);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::SetupMemPoolGroupLinkProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::SetupMemPoolGroupLinkProperties()
{
    Utils::Memset(&m_linkProperties, 0x0, sizeof(m_linkProperties));

    LDLLNode* pNode = m_bufferManagerList.Head();

    while (NULL != pNode)
    {
        MemPoolBufferManager* pMemPoolBufMgr = static_cast<MemPoolBufferManager*>(pNode->pData);

        CAMX_ASSERT(NULL != pMemPoolBufMgr);

        UpdateMemPoolGroupLinkProperties(pMemPoolBufMgr);

        pNode = LightweightDoublyLinkedList::NextNode(pNode);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Consolidated Registered Buffer count : "
                     "immediateAllocBufferCount(min=%d, max=%d, sum=%d), maxBufferCount(min=%d, max=%d, sum=%d)",
                     m_pGroupName, this,
                     m_linkProperties.isFromIFENode,
                     m_linkProperties.isPartOfRealTimePipeline,
                     m_linkProperties.isPartOfPreviewVideoPipeline,
                     m_linkProperties.isPartOfSnapshotPipeline);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::UpdateMemPoolGroupLinkProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::UpdateMemPoolGroupLinkProperties(
    MemPoolBufferManager*   pMemPoolBufMgr)
{
    if (BufferManagerType::CamxBufferManager == pMemPoolBufMgr->createData.bufferManagerType)
    {
        m_linkProperties.isFromIFENode                  |= pMemPoolBufMgr->createData.linkProperties.isFromIFENode;
        m_linkProperties.isPartOfRealTimePipeline       |= pMemPoolBufMgr->createData.linkProperties.isPartOfRealTimePipeline;
        m_linkProperties.isPartOfPreviewVideoPipeline   |=
            pMemPoolBufMgr->createData.linkProperties.isPartOfPreviewVideoPipeline;
        m_linkProperties.isPartOfSnapshotPipeline       |= pMemPoolBufMgr->createData.linkProperties.isPartOfSnapshotPipeline;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s][%p] : Consolidated Registered Buffer count : "
                     "immediateAllocBufferCount(min=%d, max=%d, sum=%d), maxBufferCount(min=%d, max=%d, sum=%d)",
                     m_pGroupName, this,
                     m_linkProperties.isFromIFENode,
                     m_linkProperties.isPartOfRealTimePipeline,
                     m_linkProperties.isPartOfPreviewVideoPipeline,
                     m_linkProperties.isPartOfSnapshotPipeline);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::DetermineBufferAllocationRequired
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT MemPoolGroup::DetermineBufferAllocationRequired(
    MemPoolBufferManager* pMemPoolBufMgr)
{
    UINT numBuffersToAllocate   = 0;
    BOOL bInvalidState          = FALSE;

    CAMX_UNREFERENCED_PARAM(pMemPoolBufMgr);

    if (m_numBuffersAllocated < pMemPoolBufMgr->createData.immediateAllocBufferCount)
    {
        numBuffersToAllocate = pMemPoolBufMgr->createData.immediateAllocBufferCount - m_numBuffersAllocated;
    }

    if ((GetStaticSettings()->MPMKeepMinNumFreeBuffers > m_freeBufferList.NumNodes()) &&
        (m_activatedBufferCount.maxBufferCount.sum     > m_numBuffersAllocated) &&
        ((TRUE == m_linkProperties.isPartOfRealTimePipeline) ||
         (TRUE == m_linkProperties.isPartOfPreviewVideoPipeline)))
    {
        UINT minBuffersToAllocate = GetStaticSettings()->MPMKeepMinNumFreeBuffers - m_freeBufferList.NumNodes();

        numBuffersToAllocate = Utils::MaxUINT(numBuffersToAllocate, minBuffersToAllocate);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s]-->MemPoolBufMgr[%s] : Buffers to allocate = %d, "
                     "Consolidated RegisterImmediate(min=%d, max=%d, sum=%d), ActiveImmediate(min=%d, max=%d, sum=%d)",
                     m_pGroupName, pMemPoolBufMgr->name, numBuffersToAllocate,
                     m_registeredBufferCount.immediateAllocBufferCount.min,
                     m_registeredBufferCount.immediateAllocBufferCount.max,
                     m_registeredBufferCount.immediateAllocBufferCount.sum,
                     m_activatedBufferCount.immediateAllocBufferCount.min,
                     m_activatedBufferCount.immediateAllocBufferCount.max,
                     m_activatedBufferCount.immediateAllocBufferCount.sum);

    return numBuffersToAllocate;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::DetermineBufferFreeRequired
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT MemPoolGroup::DetermineBufferFreeRequired(
    MemPoolBufferManager*   pMemPoolBufMgr,
    BOOL                    bOnUnregister)
{
    UINT numBuffersToFree = 0;

    CAMX_UNREFERENCED_PARAM(pMemPoolBufMgr);

    if ((FALSE == bOnUnregister) && (1 < m_bufferManagerList.NumNodes()))
    {
        // If there are no BufferManagers that are active currently, we can free those buffers now.
        // There could be a BufferManager being activated immediately.. so do not free if there are any other Buffer managers
        // attached to this group.
        // If no other BufMgr is getting activated in next few seconds, Self timer thread will free them anyway

        // If Buffer size in this group is more than threshold, we should better free them right away and take a hit in
        // re-allocating if there will be another Buffer manager that needs a buffer immediately.
        // If there is no other GetBuffer from this group, we could save some memory by doing here
        // instead of waiting until idle timeout.
        // Do not do this if this group provides some buffers to IFE
        if ((GetStaticSettings()->MPMMinSizeToFreeOnDeactivate <  m_bufferAllocProperties.totalBufferSizeBytes)         &&
            (FALSE                                             == m_linkProperties.isFromIFENode)                       &&
            (m_numBuffersAllocated                             >  m_activatedBufferCount.immediateAllocBufferCount.sum))
        {
            // just keep one buffer in free list to serve one immediate upcoming GetBuffer
            if (1 < m_freeBufferList.NumNodes())
            {
                numBuffersToFree = Utils::MinUINT(m_numBuffersAllocated - m_activatedBufferCount.immediateAllocBufferCount.sum,
                                                  m_freeBufferList.NumNodes() - 1);
                numBuffersToFree = Utils::MinUINT(numBuffersToFree, pMemPoolBufMgr->peakBuffersUsed);
            }
        }
    }
    else if (FALSE == bOnUnregister)
    {
        if (m_numBuffersAllocated > m_activatedBufferCount.immediateAllocBufferCount.sum)
        {
            numBuffersToFree = m_numBuffersAllocated - m_activatedBufferCount.immediateAllocBufferCount.sum;
        }
    }
    else
    {
        if (m_numBuffersAllocated > m_registeredBufferCount.immediateAllocBufferCount.sum)
        {
            numBuffersToFree = m_numBuffersAllocated - m_registeredBufferCount.immediateAllocBufferCount.sum;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "MemPoolGroup[%s]-->MemPoolBufMgr[%s] : bOnUnregister=%d, Buffers allocated=%d, to free = %d, "
                     "in free list = %d, buffer size = %d, "
                     "Consolidated RegisterImmediate(min=%d, max=%d, sum=%d), ActiveImmediate(min=%d, max=%d, sum=%d)",
                     m_pGroupName, pMemPoolBufMgr->name, bOnUnregister, m_numBuffersAllocated, numBuffersToFree,
                     m_freeBufferList.NumNodes(),
                     m_bufferAllocProperties.totalBufferSizeBytes,
                     m_registeredBufferCount.immediateAllocBufferCount.min,
                     m_registeredBufferCount.immediateAllocBufferCount.max,
                     m_registeredBufferCount.immediateAllocBufferCount.sum,
                     m_activatedBufferCount.immediateAllocBufferCount.min,
                     m_activatedBufferCount.immediateAllocBufferCount.max,
                     m_activatedBufferCount.immediateAllocBufferCount.sum);

    return numBuffersToFree;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::IsCompatibleHeap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MemPoolGroup::IsCompatibleHeap(
    BOOL            bAlreadyAllocated,
    CSLBufferHeap   groupHeap,
    CSLBufferHeap   bufMgrHeap
    ) const
{
    BOOL bCompatibleHeap = FALSE;

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "MemPoolGroup[%s][%p] : Group heapType=%d, current BufMgr heapType=%d",
                     m_pGroupName, this, m_bufferHeap, bufMgrHeap);

    if (groupHeap == bufMgrHeap)
    {
        bCompatibleHeap = TRUE;
    }
    else
    {
        if (FALSE == bAlreadyAllocated)
        {
            // If we havn't allocated any buffers yet, we can switch to allocate from either Gralloc or CSL
            bCompatibleHeap = TRUE;
        }
        else if ((CSLBufferHeapEGL == groupHeap) &&
                 (CSLBufferHeapIon == bufMgrHeap))
        {
            // We can use Gralloc memory for CSL based Buffer managers as well.
            // But reverse can't. i.e If this goup is based on CSL allocation, we can't use it for Buffer Managers which
            // need Gralloc handle (we can't get native_handle from CSL Allocs)
            bCompatibleHeap = TRUE;
        }
        else
        {
            bCompatibleHeap = FALSE;
        }
    }

    return bCompatibleHeap;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::SizeWithinRange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MemPoolGroup::SizeWithinRange(
    BOOL   bAlreadyAllocated,
    SIZE_T groupSize,
    SIZE_T bufMgrSize,
    SIZE_T minGroupSize
    ) const
{
    BOOL    bSizeMatch      = FALSE;
    UINT    sizeDiffAllowed = GetStaticSettings()->MPMSizeDiffAllowedToGroup;

    // If the group size is more than the defined large group size, override the sizeDiffAllowed to be the
    // sizeDiffAllowed override value
    if (groupSize >= GetStaticSettings()->MPMLargeGroupSizeDefinition)
    {
        sizeDiffAllowed = GetStaticSettings()->MPMSizeDiffAllowedToGroupOverride;
    }

    if (GetStaticSettings()->MPMMinSizeForSharing > bufMgrSize)
    {
        // If the size is less than limit, lets have a separate group to avoid unnecessary grouping and allocating more
        // memory
        bSizeMatch = FALSE;
    }
    else if (TRUE == bAlreadyAllocated)
    {
        // If we already allocated some buffers, we can't allow Buffer Managers needing more memory size
        // Only allow with less sizes
        if ((groupSize >= bufMgrSize) && ((groupSize - bufMgrSize) <= sizeDiffAllowed))
        {
            bSizeMatch = TRUE;
        }
        else
        {
            bSizeMatch = FALSE;
        }
    }
    else
    {
        // If no buffers allocated till now, we can allow BufferManagers with size within range
        if (((groupSize >= bufMgrSize) && ((groupSize - bufMgrSize) <= sizeDiffAllowed)) ||
            ((bufMgrSize >= groupSize) && ((bufMgrSize - groupSize) <= sizeDiffAllowed)))
        {
            // If the buffer is within the current min and max of the group, let it join group
            if ((bufMgrSize <= groupSize) && (bufMgrSize >= minGroupSize))
            {
                bSizeMatch = TRUE;
            }
            // If the buffer is not within the current min and max of the group, check that adding it
            // to the group won't make the total size range of the group more than our allowed size range
            else
            {
                SIZE_T sizeDiffWithNewBuffer = ((bufMgrSize > groupSize) ?
                    (bufMgrSize - minGroupSize) : (groupSize - bufMgrSize));
                if (sizeDiffWithNewBuffer <= sizeDiffAllowed)
                {
                    bSizeMatch = TRUE;
                }
                else
                {
                    bSizeMatch = FALSE;
                }
            }
        }
        else
        {
            bSizeMatch = FALSE;
        }
    }

    return bSizeMatch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::AlignmentMatch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MemPoolGroup::AlignmentMatch(
    BOOL   bAlreadyAllocated,
    SIZE_T groupAlignment,
    SIZE_T bufMgrAlignment
    ) const
{
    BOOL bAlignmentMatch = FALSE;

    // For now, lets make alignment as 4096 always.. to avoid going to a different group only because of alignment.
    // This will be set in UpdateCSLBufferAllocProperties.
    // So if any Buffer Manager has alignment that is 4K divisible, we are fine irrespective of whether buffers are already
    // allocated or not.

    CAMX_ASSERT(0 == (groupAlignment % GetStaticSettings()->MPMBufferAddressAlignment));

    if (TRUE == bAlreadyAllocated)
    {
        // If we already allocated some buffers, we can't allow Buffer Managers needing a new alignment requirement.
        // If group's current alignment satisfies this Buffer manager, we can allow
        // Only allow with less sizes
        if ((groupAlignment % bufMgrAlignment) == 0)
        {
            bAlignmentMatch = TRUE;
        }
        else
        {
            bAlignmentMatch = FALSE;
        }
    }
    else
    {
        // If there are no buffers currently allocated in this group, we can allow any alignment
        bAlignmentMatch = TRUE;
    }

    return bAlignmentMatch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::CSLMemFlagsMatch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MemPoolGroup::CSLMemFlagsMatch(
    BOOL bAlreadyAllocated,
    UINT groupFlags,
    UINT bufMgrFlags
    ) const
{
    BOOL bFlagsMatch = TRUE;

    CAMX_UNREFERENCED_PARAM(bAlreadyAllocated);

    // We can generally obey any flags except PROTECTED
    // i.e can't match secure - non sucure flags in to the same group

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "groupFlags flags=%X UMD=%d KMD=%d CMD=%d HW=%d PKT=%d CACHE=%d PROT=%d DSPSecure=%d",
                     groupFlags,
                     (groupFlags & CSLMemFlagUMDAccess),
                     (groupFlags & CSLMemFlagKMDAccess),
                     (groupFlags & CSLMemFlagCmdBuffer),
                     (groupFlags & CSLMemFlagHw),
                     (groupFlags & CSLMemFlagPacketBuffer),
                     (groupFlags & CSLMemFlagCache),
                     (groupFlags & CSLMemFlagProtected),
                     (groupFlags & CSLMemFlagDSPSecureAccess));

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "bufMgrFlags flags=%X UMD=%d KMD=%d CMD=%d HW=%d PKT=%d CACHE=%d PROT=%d DSPSecure=%d",
                     bufMgrFlags,
                     (bufMgrFlags & CSLMemFlagUMDAccess),
                     (bufMgrFlags & CSLMemFlagKMDAccess),
                     (bufMgrFlags & CSLMemFlagCmdBuffer),
                     (bufMgrFlags & CSLMemFlagHw),
                     (bufMgrFlags & CSLMemFlagPacketBuffer),
                     (bufMgrFlags & CSLMemFlagCache),
                     (bufMgrFlags & CSLMemFlagProtected),
                     (bufMgrFlags & CSLMemFlagDSPSecureAccess));

    /// @todo (CAMX-4162) Check if we have any other restrictions on mem flags

    if (((groupFlags & CSLMemFlagProtected)       != (bufMgrFlags & CSLMemFlagProtected)) ||
        ((groupFlags & CSLMemFlagDSPSecureAccess) != (bufMgrFlags & CSLMemFlagDSPSecureAccess)))
    {
        bFlagsMatch = FALSE;
    }

    if (TRUE == bAlreadyAllocated)
    {
        // If buffers are already allocated with non-cache, and if incoming buffer manager needs cache - do not match
        if ((0 == (groupFlags & CSLMemFlagCache)) && (0 != (bufMgrFlags & CSLMemFlagCache)))
        {
            bFlagsMatch = FALSE;
        }
    }

    return bFlagsMatch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::GrallocMemFlagsMatch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MemPoolGroup::GrallocMemFlagsMatch(
    BOOL    bAlreadyAllocated,
    UINT64  groupProducerFlags,
    UINT64  groupConsumerFlags,
    UINT64  bufMgrProducerFlags,
    UINT64  bufMgrConsumerFlags
    ) const
{
    UINT64 groupFlags   = groupProducerFlags  | groupConsumerFlags;
    UINT64 bufMgrFlags  = bufMgrProducerFlags | bufMgrConsumerFlags;
    BOOL   bFlagsMatch  = TRUE;

    CAMX_UNREFERENCED_PARAM(bAlreadyAllocated);

    // We can generally obey any flags except Protected
    // i.e can't match secure - non sucure flags in to the same group

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "groupProducerFlags=0x%llx, groupConsumerFlags=0x%llx, "
                     "bufMgrProducerFlags=0x%llx, bufMgrConsumerFlags=0x%llx",
                     groupProducerFlags, groupConsumerFlags, bufMgrProducerFlags, bufMgrConsumerFlags);

    /// @todo (CAMX-4162) Check if we have any other restrictions on mem flags

    if ((0 != groupFlags) && (groupFlags & GrallocUsageProtected) != (bufMgrFlags & GrallocUsageProtected))
    {
        bFlagsMatch = FALSE;
    }

    return bFlagsMatch;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::DevicesMatch
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MemPoolGroup::DevicesMatch(
    BOOL            bAlreadyAllocated,
    INT32*          pGroupDeviceIndices,
    UINT            groupDeviceCount,
    const INT32*    pBufMgrDeviceIndices,
    UINT            bufMgrDeviceCount
    ) const
{
    BOOL bDeviceMatch = FALSE;

    CAMX_UNREFERENCED_PARAM(bAlreadyAllocated);

    if (FALSE == GetStaticSettings()->MPMGroupIfExactDeviceMatch)
    {
        // We can always allow Buf Managers even though buffers need to be mapped to additional/different devices.
        // (even if we have already allocated some buffers)
        // If not already allocated or for any new allocations - we map the buffers to all HWs while allocation itself
        // If already allocated buffer needs to be given - we do incremental map, i.e map to the additional devices
        //     while giving the buffer to BufferManager, in GetBufferFromPool call.
        // We always save devices info (what all devices the buffer is currently mapped to) in MemPoolBuffer structure, so
        //     we know when to do incremental mapping.
        bDeviceMatch = TRUE;
    }
    else
    {
        // If exact matching is enabled - for exa - by avoiding unnecessary mapping of buffers to all HWs to save SMMU
        // space - we should only group Buffer managers with exact devices or we can map Buffer managers with no devices

        if (0 == bufMgrDeviceCount)
        {
            bDeviceMatch = TRUE;
        }
        else
        {
            bDeviceMatch = Utils::IsArray1SubsetOfArray2(pBufMgrDeviceIndices, bufMgrDeviceCount,
                                                         pGroupDeviceIndices, groupDeviceCount);
        }
    }

    return bDeviceMatch;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::CheckAndMapBufferToDevices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolGroup::CheckAndMapBufferToDevices(
    MemPoolBuffer*  pBuffer,
    const INT32*    pDeviceIndices,
    UINT            deviceCountt,
    UINT            flags)
{
    CamxResult  result          = CamxResultSuccess;
    UINT        numDevicesToMap = 0;
    INT32       deviceIndices[CamxMaxDeviceIndex];

    // Find devices in Buffer Manager devices to which this buffer is not yet mapped
    numDevicesToMap = Utils::FindElementsInArray1NotPresentInArray2(pDeviceIndices,
                                                                    deviceCountt,
                                                                    pBuffer->deviceIndices,
                                                                    pBuffer->deviceCount,
                                                                    deviceIndices,
                                                                    CamxMaxDeviceIndex);

    // If no new devices to map, remove the HwFlags. Incoming flags might have Hw+UMD and so we may go and call CSLMapBuffer
    // below because of UMD flags, but it tries to map to HW but with deviceCount 0, which will fail.
    if (0 == numDevicesToMap)
    {
        flags &= ~CSLMemFlagHw;
    }

    // Also check if we need to map to CPU (if not already mapped)
    if (0 != (flags & CSLMemFlagUMDAccess))
    {
        // Need UMD access, so check if already mapped or not
        if (NULL != pBuffer->bufferInfo.pVirtualAddr)
        {
            flags &= ~CSLMemFlagUMDAccess;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolBuffer[%s][%p] Mapping : flags=0x%x, numDevicesToMap=%d, list=%d %d %d %d %d",
                     pBuffer->name, pBuffer, flags, numDevicesToMap, deviceIndices[0], deviceIndices[1], deviceIndices[2],
                     deviceIndices[3], deviceIndices[4]);

    // If we have found some devices that are not being mapped, map to those devices now
    // Or If UMDAccess is needed now
    if ((0 < numDevicesToMap) || (0 != (flags & CSLMemFlagUMDAccess)))
    {
        if (MaxIncrBufMappings > pBuffer->numIncrMappings)
        {
            CSLBufferInfo   bufferInfo;


            // Disable Delayed Unmap optimization
            if (TRUE == GetStaticSettings()->MPMForceDisableDelayedUnmap)
            {
                flags |= CSLMemFlagDisableDelayedUnmap;
            }

            if (-1 != pBuffer->bufferInfo.fd)
            {
                result = CSLMapBuffer(&bufferInfo,
                                      pBuffer->bufferInfo.fd,
                                      pBuffer->bufferInfo.offset,
                                      pBuffer->bufferInfo.size,
                                      flags,
                                      deviceIndices,
                                      numDevicesToMap);
            }
            else if (NULL != pBuffer->hGrallocBuffer)
            {
                BufferHandle*       phNativeHandle  = reinterpret_cast<BufferHandle*>(&pBuffer->hGrallocBuffer);
                const NativeHandle* phNativeBuffer  = *phNativeHandle;

                result = CSLMapNativeBuffer(&bufferInfo,
                                            reinterpret_cast<const CSLNativeHandle*>(phNativeBuffer),
                                            0, // offset
                                            m_bufferAllocProperties.totalBufferSizeBytes,
                                            flags,
                                            deviceIndices,
                                            numDevicesToMap);
            }
            else
            {
                result = CamxResultEInvalidState;

                CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                               "MemPoolBuffer[%s][%p] : Cannot map, invalid buffer handles, fd=%d, hGrallocBuffer=%p",
                               pBuffer->name, pBuffer, pBuffer->bufferInfo.fd, pBuffer->hGrallocBuffer);
            }

            /// @todo (CAMX-4162) Do we need to save any additional info from bufferInfo or flags ?

            if (CamxResultSuccess == result)
            {
                if (-1 == pBuffer->bufferInfo.fd)
                {
                    // This is the first time map on this buffer which was allocated using Gralloc
                    // Save Buffer information in main bufferInfo
                    pBuffer->bufferInfo = bufferInfo;
                }
                else
                {
                    // This is an incremental mapping to map buffer to additional devices.
                    // Save Buffer information in incremental bufferInfo
                    pBuffer->incrBufferInfo[pBuffer->numIncrMappings] = bufferInfo;

                    pBuffer->numIncrMappings++;

                    // If we have mapped this buffer to UMD now, save pVirtualAddr into pBuffer->bufferInfo
                    // So that getCPUAddress will return valid address and also we will not map to UMD again in future
                    // incremental maps
                    if (0 != (flags & CSLMemFlagUMDAccess))
                    {
                        pBuffer->bufferInfo.pVirtualAddr = bufferInfo.pVirtualAddr;
                    }
                }

                // Append pBuffer->deviceList so that we know, from now, this buffer is mapped to these new devices
                UINT numDevicesAdded = Utils::AppendArray1ToArray2(deviceIndices,
                                                                   numDevicesToMap,
                                                                   pBuffer->deviceIndices,
                                                                   pBuffer->deviceCount,
                                                                   CamxMaxDeviceIndex);

                CAMX_ASSERT(numDevicesAdded == numDevicesToMap);
                if (numDevicesAdded != numDevicesToMap)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolBuffer[%s][%p] : current=%d, add=%d but added only %d",
                                   pBuffer->name, pBuffer, pBuffer->deviceCount, numDevicesToMap, numDevicesAdded);
                }

                pBuffer->deviceCount += numDevicesAdded;

                if (TRUE == GetStaticSettings()->enableMemoryStats)
                {
                    s_pMPGStatsLock->Lock();

                    if (0 != (flags & CSLMemFlagUMDAccess))
                    {
                        // we mapped the allocated buffer to CPU, Devices
                        s_memPoolMgrStats.numBuffersMappedToCPU++;
                        s_memPoolMgrStats.peakNumBuffersMappedToCPU    = Utils::MaxUINT(
                                                                         s_memPoolMgrStats.peakNumBuffersMappedToCPU,
                                                                         s_memPoolMgrStats.numBuffersMappedToCPU);

                        s_memPoolMgrStats.sizeOfMemoryMappedToCPU     += pBuffer->allocatedSize;
                        s_memPoolMgrStats.peakSizeOfMemoryMappedToCPU  = Utils::MaxSIZET(
                                                                         s_memPoolMgrStats.peakSizeOfMemoryMappedToCPU,
                                                                         s_memPoolMgrStats.sizeOfMemoryMappedToCPU);
                    }

                    if (0 < numDevicesToMap)
                    {
                        // we mapped the allocated buffer to hw device(s)
                        // Note - we are saving stats by combining all devices, not per-device stats
                        // we mapped the allocated buffer to CPU, Devices
                        s_memPoolMgrStats.numBuffersMappedToDevices++;
                        s_memPoolMgrStats.peakNumBuffersMappedToDevices    = Utils::MaxUINT(
                                                                             s_memPoolMgrStats.peakNumBuffersMappedToDevices,
                                                                             s_memPoolMgrStats.numBuffersMappedToDevices);

                        s_memPoolMgrStats.sizeOfMemoryMappedToDevices     += pBuffer->allocatedSize;
                        s_memPoolMgrStats.peakSizeOfMemoryMappedToDevices  = Utils::MaxSIZET(
                                                                             s_memPoolMgrStats.peakSizeOfMemoryMappedToDevices,
                                                                             s_memPoolMgrStats.sizeOfMemoryMappedToDevices);
                    }

                    s_pMPGStatsLock->Unlock();
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                               "MemPoolBuffer[%s][%p] : Failed in mapping to devices[%d] = %d %d %d %d %d, result=%s",
                               pBuffer->name, pBuffer, numDevicesToMap, deviceIndices[0], deviceIndices[1],
                               deviceIndices[2], deviceIndices[3], deviceIndices[4],
                               Utils::CamxResultToString(result));
            }
        }
        else
        {
            result = CamxResultEFailed;

            CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                           "MemPoolBuffer[%s][%p] : Cannot map numIncrMappings= %d devices[%d] = %d %d %d %d %d",
                           pBuffer->name, pBuffer, pBuffer->numIncrMappings, numDevicesToMap,
                           deviceIndices[0], deviceIndices[1], deviceIndices[2], deviceIndices[3], deviceIndices[4]);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolGroup::SelfTuneImmediateCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolGroup::SelfTuneImmediateCount(
    MemPoolBufferManager*   pMemPoolBufMgr)
{
    if (TRUE == GetStaticSettings()->MPMSelfTuneImmediateCount)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                         "[%s]-->[%s] : Before Self Tune : Type=%s, Immediate=%d, Max=%d, "
                         "isIFE=%d, isRealtime=%d, isPreviewVideo=%d, isSnapshot=%d",
                         m_pGroupName, pMemPoolBufMgr->name,
                         (BufferManagerType::CamxBufferManager == pMemPoolBufMgr->createData.bufferManagerType) ?
                         "CamX" : "CHI",
                         pMemPoolBufMgr->createData.immediateAllocBufferCount,
                         pMemPoolBufMgr->createData.maxBufferCount,
                         pMemPoolBufMgr->createData.linkProperties.isFromIFENode,
                         pMemPoolBufMgr->createData.linkProperties.isPartOfRealTimePipeline,
                         pMemPoolBufMgr->createData.linkProperties.isPartOfPreviewVideoPipeline,
                         pMemPoolBufMgr->createData.linkProperties.isPartOfSnapshotPipeline);

        if (BufferManagerType::CamxBufferManager == pMemPoolBufMgr->createData.bufferManagerType)
        {
            // Overwrite immediate counts based on link properties
            if (TRUE == pMemPoolBufMgr->createData.linkProperties.isFromIFENode)
            {
                pMemPoolBufMgr->createData.immediateAllocBufferCount = Utils::MinUINT(IFENodeImmediateAllocBufferCount,
                    pMemPoolBufMgr->createData.immediateAllocBufferCount);
            }
            else if (TRUE == pMemPoolBufMgr->createData.linkProperties.isPartOfRealTimePipeline)
            {
                pMemPoolBufMgr->createData.immediateAllocBufferCount =
                    Utils::MinUINT(RealTimeImmediateAllocBufferCount,
                        pMemPoolBufMgr->createData.immediateAllocBufferCount);
            }
            else
            {
                pMemPoolBufMgr->createData.immediateAllocBufferCount = 0;
            }
        }
        else
        {
            // ChiBufferManager type - We dont have enough information to self tune. Use whatever passed.
        }

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                         "[%s]-->[%s] : After Self Tune : Type=%s, Immediate=%d, Max=%d, "
                         "isIFE=%d, isRealtime=%d, isPreviewVideo=%d, isSnapshot=%d",
                         m_pGroupName, pMemPoolBufMgr->name,
                         (BufferManagerType::CamxBufferManager == pMemPoolBufMgr->createData.bufferManagerType) ?
                         "CamX" : "CHI",
                         pMemPoolBufMgr->createData.immediateAllocBufferCount,
                         pMemPoolBufMgr->createData.maxBufferCount,
                         pMemPoolBufMgr->createData.linkProperties.isFromIFENode,
                         pMemPoolBufMgr->createData.linkProperties.isPartOfRealTimePipeline,
                         pMemPoolBufMgr->createData.linkProperties.isPartOfPreviewVideoPipeline,
                         pMemPoolBufMgr->createData.linkProperties.isPartOfSnapshotPipeline);
    }
}

CAMX_NAMESPACE_END
