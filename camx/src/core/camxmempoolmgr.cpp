////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxmempoolmgr.cpp
///
/// @brief CamX Memory Pool Mgr definitions. MemPoolMgr is used to allocate memory for buffers.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxmempoolmgr.h"
#include "camxmempoolgroup.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Structure containing MemPool Buffer Manager handle data. This contains the Group in which client Buffer Manager is
///        is attached to and unique MemPoolBufferManager pointer corresponding to this Buffer Manager in the Group.
struct MemPoolBufMgrHandleData
{
    MemPoolGroup*           pMemPoolGroup;  ///< Memory Pool Group to which this Buffer Manager is attached
    MemPoolBufferManager*   pMemPoolBufMgr; ///< Pointer to corresponding Memory Pool Buffer Manager in MemPoolGroup
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// This is a flag indicating whether the MemPoolMgr singleton is in a valid state
BOOL MemPoolMgr::s_isValid = FALSE;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::IsSupportedBufferHeap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MemPoolMgr::IsSupportedBufferHeap(
    CSLBufferHeap bufferHeap)
{
    BOOL bIsSupported;

    switch (bufferHeap)
    {
        case CSLBufferHeapIon :
        case CSLBufferHeapEGL :
            bIsSupported = TRUE;
            break;
        default :
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Unsupported buffer heap type %d", bufferHeap);
            bIsSupported = FALSE;
            break;
    }

    return bIsSupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::IsValidBufferConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MemPoolMgr::IsValidBufferConfiguration(
    const BufferManagerCreateData* pCreateData)
{
    BOOL bIsValidConfiguration = TRUE;
    UINT numberOfDevices       = pCreateData->deviceCount;

    if (FALSE == IsSupportedBufferHeap(pCreateData->bufferProperties.bufferHeap))
    {
        bIsValidConfiguration = FALSE;
    }

    if (TRUE == bIsValidConfiguration)
    {
        if ((0 != (pCreateData->bufferProperties.memFlags & CSLMemFlagHw)) &&
            (0 == numberOfDevices))
        {
            bIsValidConfiguration = FALSE;
        }
    }

    return bIsValidConfiguration;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::RegisterBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemPoolBufMgrHandle MemPoolMgr::RegisterBufferManager(
    const CHAR*                     pBufferManagerName,
    const BufferManagerCreateData*  pCreateData)
{
    MemPoolMgr*                 pMemPoolMgr         = NULL;
    MemPoolBufMgrHandleData*    pBufMgrHandleData   = NULL;
    MemPoolGroup*               pMemPoolGroup       = NULL;
    MemPoolBufferManager*       pMemPoolBufMgr      = NULL;
    CamxResult                  result              = CamxResultSuccess;

    if ((NULL == pBufferManagerName) || (NULL == pCreateData))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input args %p %p", pBufferManagerName, pCreateData);
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) &&
        (FALSE             == MemPoolMgr::IsValidBufferConfiguration(pCreateData)))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Buffer heap not supported %d", pCreateData->bufferProperties.bufferHeap);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        pBufMgrHandleData = static_cast<MemPoolBufMgrHandleData*>(CAMX_CALLOC(sizeof(MemPoolBufMgrHandleData)));
        if (NULL == pBufMgrHandleData)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Insufficient memory");
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolMgrEntry : RegisterBufferManager BufMgr[%s] heap=%d",
                         pBufferManagerName, pCreateData->bufferProperties.bufferHeap);

        // Get singleton instance
        pMemPoolMgr = GetInstanceLocked();
    }

    if (NULL != pMemPoolMgr)
    {
        BOOL bMatchingGroupFound = FALSE;

        if (0 == pMemPoolMgr->m_groupList.NumNodes())
        {
            // Do not change this log format, few scripts are written based on this
            CAMX_LOG_INFO(CamxLogGroupMemMgr, "MPM : Number of groups is 0, register first buffer manager");
        }

        if (FreeingPoolState::FreeingInProgress == pMemPoolMgr->GetCurMgrFreeingPoolState())
        {
            pMemPoolMgr->WaitOnFreeingStateComplete();
        }

        // register buffer manager with exisiting mem pool group if matches
        result = pMemPoolMgr->RegisterIfExistingGroupMatches(pBufferManagerName, pCreateData, &pMemPoolGroup, &pMemPoolBufMgr);

        if (CamxResultSuccess == result)
        {
            bMatchingGroupFound = TRUE;
        }
        else if (CamxResultENoSuch == result)
        {
            if (FreeingPoolState::FreeingInProgress == pMemPoolMgr->GetCurMgrFreeingPoolState())
            {
                pMemPoolMgr->WaitOnFreeingStateComplete();
            }

            // create and register new mem pool group as no matching group found
            result = pMemPoolMgr->RegisterNewMemPoolGroup(pBufferManagerName, pCreateData, &pMemPoolGroup, &pMemPoolBufMgr);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Matching MemPoolGroup[%s] found, but BufMgr[%s] Registration failed %s",
                           pMemPoolGroup->GetMemPoolGroupName(), pBufferManagerName, Utils::CamxResultToString(result));
        }

        if (CamxResultSuccess == result)
        {
            pBufMgrHandleData->pMemPoolGroup    = pMemPoolGroup;
            pBufMgrHandleData->pMemPoolBufMgr   = pMemPoolBufMgr;

            if (FALSE == pMemPoolMgr->m_monitorThreadStart)
            {
                pMemPoolMgr->m_monitorThreadStart = TRUE;
                pMemPoolMgr->m_pMonitorThreadCond->Signal();
            }
        }
        else
        {
            if (NULL != pBufMgrHandleData)
            {
                CAMX_FREE(pBufMgrHandleData);
                pBufMgrHandleData = NULL;
            }
        }

        pMemPoolMgr->Unlock();
    }

    return static_cast<MemPoolBufMgrHandle>(pBufMgrHandleData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::UnregisterBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolMgr::UnregisterBufferManager(
    MemPoolBufMgrHandle hMemPoolBufMgrHandle)
{
    MemPoolBufMgrHandleData*    pBufMgrHandleData   = static_cast<MemPoolBufMgrHandleData*>(hMemPoolBufMgrHandle);
    MemPoolMgr*                 pMemPoolMgr         = NULL;
    CamxResult                  result              = CamxResultSuccess;

    if ((NULL == pBufMgrHandleData)                 ||
        (NULL == pBufMgrHandleData->pMemPoolGroup)  ||
        (NULL == pBufMgrHandleData->pMemPoolBufMgr))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Buffer Manager Handle");
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        // Get singleton instance and lock mutex
        pMemPoolMgr = GetInstanceLocked();
    }

    if (NULL != pMemPoolMgr)
    {
        MemPoolGroup* pMemPoolGroup = pBufMgrHandleData->pMemPoolGroup;

        if (FALSE == pMemPoolMgr->MemPoolGroupExists(pMemPoolGroup))
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Group handle %p", pMemPoolGroup);
            result = CamxResultEInvalidArg;
        }

        if (CamxResultSuccess == result)
        {
            result = pMemPoolGroup->UnregisterBufferManager(pBufMgrHandleData->pMemPoolBufMgr);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in UnregisterBufferManager result=%s",
                               Utils::CamxResultToString(result));
            }
        }

        if (CamxResultSuccess == result)
        {
            BOOL bCanDestroyMemPoolGroup = pMemPoolGroup->CanDestroyMemPoolGroup();

            if (TRUE == bCanDestroyMemPoolGroup)
            {
                // First remove the group from List
                result = pMemPoolMgr->RemoveMemPoolGroupFromList(pMemPoolGroup);

                if (CamxResultSuccess != result)
                {
                    // We should never come here as we checked this group is already present in list
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in in removing, result=%s", Utils::CamxResultToString(result));
                }

                CAMX_DELETE pMemPoolGroup;
                pMemPoolGroup = NULL;

                if (0 == pMemPoolMgr->m_groupList.NumNodes())
                {
                    if (TRUE == pMemPoolMgr->m_monitorThreadStart)
                    {
                        pMemPoolMgr->m_monitorThreadStart = FALSE;
                        pMemPoolMgr->m_pMonitorThreadCond->Signal();
                    }

                    pMemPoolMgr->m_groupNameCounter = 0;

                    MemPoolGroup::PrintMemoryPoolManagerStats(FALSE);
                    MemPoolGroup::ResetMemoryPoolManagerStats();

                    // Do not change this log format, few scripts are written based on this
                    CAMX_LOG_INFO(CamxLogGroupMemMgr, "MPM : Number of groups is 0 now, unregistered last buffer manager");
                }
            }
        }

        CAMX_FREE(pBufMgrHandleData);
        pBufMgrHandleData = NULL;

        pMemPoolMgr->Unlock();
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::ActivateBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolMgr::ActivateBufferManager(
    MemPoolBufMgrHandle hMemPoolBufMgrHandle)
{
    MemPoolBufMgrHandleData*    pBufMgrHandleData   = static_cast<MemPoolBufMgrHandleData*>(hMemPoolBufMgrHandle);
    CamxResult                  result              = CamxResultSuccess;

    if ((NULL == pBufMgrHandleData)                 ||
        (NULL == pBufMgrHandleData->pMemPoolGroup)  ||
        (NULL == pBufMgrHandleData->pMemPoolBufMgr))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Buffer Manager Handle %p", pBufMgrHandleData);
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) && (TRUE == GetStaticSettings()->MPMValidateMemPoolGroup))
    {
        // Get singleton instance and lock mutex
        MemPoolMgr* pMemPoolMgr = GetInstanceLocked();

        if (NULL != pMemPoolMgr)
        {
            if (FALSE == pMemPoolMgr->MemPoolGroupExists(pBufMgrHandleData->pMemPoolGroup))
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Group handle %p", pBufMgrHandleData->pMemPoolGroup);
                result = CamxResultEInvalidArg;
            }

            pMemPoolMgr->Unlock();
        }
    }

    if (CamxResultSuccess == result)
    {
        MemPoolMgr* pMemPoolMgr = GetInstanceLocked();
        if (NULL != pMemPoolMgr)
        {
            // Do not call to ActivateBufferManager if freeing of buffers is in progress
            if (FreeingPoolState::FreeingInProgress == pMemPoolMgr->GetCurMgrFreeingPoolState())
            {
                pMemPoolMgr->WaitOnFreeingStateComplete();
            }

            pMemPoolMgr->Unlock();
        }

        MemPoolGroup* pMemPoolGroup = pBufMgrHandleData->pMemPoolGroup;

        result = pMemPoolGroup->ActivateBufferManager(pBufMgrHandleData->pMemPoolBufMgr);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in Activate buffer manager %s", Utils::CamxResultToString(result));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::DeactivateBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolMgr::DeactivateBufferManager(
    MemPoolBufMgrHandle hMemPoolBufMgrHandle)
{
    MemPoolBufMgrHandleData*    pBufMgrHandleData   = static_cast<MemPoolBufMgrHandleData*>(hMemPoolBufMgrHandle);
    CamxResult                  result              = CamxResultSuccess;
    MemPoolGroup*               pMemPoolGroup;

    if ((NULL == pBufMgrHandleData)                 ||
        (NULL == pBufMgrHandleData->pMemPoolGroup)  ||
        (NULL == pBufMgrHandleData->pMemPoolBufMgr))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Buffer Manager Handle %p", pBufMgrHandleData);
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) && (TRUE == GetStaticSettings()->MPMValidateMemPoolGroup))
    {
        // Get singleton instance and lock mutex
        MemPoolMgr* pMemPoolMgr = GetInstanceLocked();

        if (NULL != pMemPoolMgr)
        {
            if (FALSE == pMemPoolMgr->MemPoolGroupExists(pBufMgrHandleData->pMemPoolGroup))
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Group handle %p", pBufMgrHandleData->pMemPoolGroup);
                result = CamxResultEInvalidArg;
            }

            pMemPoolMgr->Unlock();
        }
    }

    if (CamxResultSuccess == result)
    {
        pMemPoolGroup   = pBufMgrHandleData->pMemPoolGroup;

        result = pMemPoolGroup->DeactivateBufferManager(pBufMgrHandleData->pMemPoolBufMgr);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in Deactivate Buffer manager %s", Utils::CamxResultToString(result));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::GetBufferFromPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemPoolBufferHandle MemPoolMgr::GetBufferFromPool(
    MemPoolBufMgrHandle hMemPoolBufMgrHandle,
    CSLBufferInfo*      pCSLBufferInfo,
    BufferHandle*       phGrallocBuffer)
{
    MemPoolBufMgrHandleData*    pBufMgrHandleData   = static_cast<MemPoolBufMgrHandleData*>(hMemPoolBufMgrHandle);
    MemPoolGroup*               pMemPoolGroup;
    CamxResult                  result              = CamxResultSuccess;
    MemPoolBufferHandle         hBufferHandle       = NULL;
    MemPoolGetBufferResult      getBufferPoolResult = { 0 };

    if ((NULL == pBufMgrHandleData)                 ||
        (NULL == pBufMgrHandleData->pMemPoolGroup)  ||
        (NULL == pBufMgrHandleData->pMemPoolBufMgr))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Buffer Manager Handle %p", pBufMgrHandleData);
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) && (TRUE == GetStaticSettings()->MPMValidateMemPoolGroup))
    {
        // Get singleton instance and lock mutex
        MemPoolMgr* pMemPoolMgr = GetInstanceLocked();

        if (NULL != pMemPoolMgr)
        {
            if (FALSE == pMemPoolMgr->MemPoolGroupExists(pBufMgrHandleData->pMemPoolGroup))
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Group handle %p", pBufMgrHandleData->pMemPoolGroup);
                result = CamxResultEInvalidArg;
            }

            pMemPoolMgr->Unlock();
        }
    }

    if (CamxResultSuccess == result)
    {
        MemPoolMgr* pMemPoolMgr = GetInstanceLocked();

        if (NULL != pMemPoolMgr)
        {
            // Do not call to GetBufferFromPool if freeing of buffers is in progress
            if (FreeingPoolState::FreeingInProgress == pMemPoolMgr->GetCurMgrFreeingPoolState())
            {
                pMemPoolMgr->WaitOnFreeingStateComplete();
            }

            pMemPoolMgr->Unlock();

            pMemPoolGroup       = pBufMgrHandleData->pMemPoolGroup;
            getBufferPoolResult = pMemPoolGroup->GetBufferFromPool(pBufMgrHandleData->pMemPoolBufMgr,
                                                                    pCSLBufferInfo, phGrallocBuffer);
            hBufferHandle       = getBufferPoolResult.hBufferHandle;
            result              = getBufferPoolResult.result;

            // If we failed to allocate the buffer, retry allocation through
            // clearing all the memory pools' free lists
            if (CamxResultENoMemory == result)
            {
                CAMX_LOG_WARN(CamxLogGroupMemMgr, "Failed in getting buffer from pool result=%s, "
                              "attempting to free memory and try again",
                              Utils::CamxResultToString(result));

                // Print the state of Mem Pool Manaager - Prints state of all Groups
                MemPoolMgr::PrintMemPoolMgrState(TRUE);

                pMemPoolMgr->ClearAllMemPoolFreeList();

                getBufferPoolResult = pMemPoolGroup->GetBufferFromPool(pBufMgrHandleData->pMemPoolBufMgr,
                                                                       pCSLBufferInfo, phGrallocBuffer);
                hBufferHandle       = getBufferPoolResult.hBufferHandle;
                result              = getBufferPoolResult.result;

                // If we failed allocating memory again and we are currently in freeing not in
                // progress state, try again. This means at the time of clearing the free lists
                // we were in FreeingComplete state, so buffers were cleared at some point but
                // we don't know how long ago. If buffer allocation fails, we should retry clearing
                // lists again as this failure could mean we freed too long ago for allocation
                // to succeed on first "blind" retry
                if (CamxResultENoMemory == result)
                {
                    CAMX_LOG_WARN(CamxLogGroupMemMgr, "Failed in getting buffer from pool result=%s, "
                                  "2nd attempt to free memory and try again",
                                  Utils::CamxResultToString(result));
                    pMemPoolMgr->ClearAllMemPoolFreeList();
                    getBufferPoolResult = pMemPoolGroup->GetBufferFromPool(pBufMgrHandleData->pMemPoolBufMgr,
                                                                           pCSLBufferInfo, phGrallocBuffer);
                    hBufferHandle       = getBufferPoolResult.hBufferHandle;
                    result              = getBufferPoolResult.result;
                }

                // If result is not success at this point we have completely failed
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in getting buffer from pool result=%s",
                                   Utils::CamxResultToString(result));
                }
            }
        }
    }

    return hBufferHandle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::ReleaseBufferToPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolMgr::ReleaseBufferToPool(
    MemPoolBufMgrHandle hMemPoolBufMgrHandle,
    MemPoolBufferHandle hMemPoolBufferHandle,
    BOOL                bufferCached)
{
    MemPoolBufMgrHandleData*    pBufMgrHandleData   = static_cast<MemPoolBufMgrHandleData*>(hMemPoolBufMgrHandle);
    MemPoolGroup*               pMemPoolGroup;
    CamxResult                  result              = CamxResultSuccess;

    if ((NULL == pBufMgrHandleData)                 ||
        (NULL == pBufMgrHandleData->pMemPoolGroup)  ||
        (NULL == pBufMgrHandleData->pMemPoolBufMgr) ||
        (NULL == hMemPoolBufferHandle))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Buffer Manager Handle %p %p", pBufMgrHandleData, hMemPoolBufferHandle);
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) && (TRUE == GetStaticSettings()->MPMValidateMemPoolGroup))
    {
        // Get singleton instance and lock mutex
        MemPoolMgr* pMemPoolMgr = GetInstanceLocked();

        if (NULL != pMemPoolMgr)
        {
            if (FALSE == pMemPoolMgr->MemPoolGroupExists(pBufMgrHandleData->pMemPoolGroup))
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Group handle %p", pBufMgrHandleData->pMemPoolGroup);
                result = CamxResultEInvalidArg;
            }

            pMemPoolMgr->Unlock();
        }
    }

    if (CamxResultSuccess == result)
    {
        pMemPoolGroup = pBufMgrHandleData->pMemPoolGroup;

        result = pMemPoolGroup->ReleaseBufferToPool(pBufMgrHandleData->pMemPoolBufMgr, hMemPoolBufferHandle, bufferCached);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in Release Buffer to Group result=%s",
                           Utils::CamxResultToString(result));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::MapBufferToDevices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolMgr::MapBufferToDevices(
    MemPoolBufMgrHandle hMemPoolBufMgrHandle,
    MemPoolBufferHandle hMemPoolBufferHandle,
    SIZE_T              offset,
    SIZE_T              size,
    UINT32              flags,
    const INT32*        pDeviceIndices,
    UINT                deviceCount)
{
    MemPoolBufMgrHandleData*    pBufMgrHandleData   = static_cast<MemPoolBufMgrHandleData*>(hMemPoolBufMgrHandle);
    MemPoolGroup*               pMemPoolGroup;
    CamxResult                  result              = CamxResultSuccess;

    if ((NULL == pBufMgrHandleData)                 ||
        (NULL == pBufMgrHandleData->pMemPoolGroup)  ||
        (NULL == pBufMgrHandleData->pMemPoolBufMgr) ||
        (NULL == hMemPoolBufferHandle))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Buffer Manager Handle %p %p", pBufMgrHandleData, hMemPoolBufferHandle);
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) && (TRUE == GetStaticSettings()->MPMValidateMemPoolGroup))
    {
        // Get singleton instance and lock mutex
        MemPoolMgr* pMemPoolMgr = GetInstanceLocked();

        if (NULL != pMemPoolMgr)
        {
            if (FALSE == pMemPoolMgr->MemPoolGroupExists(pBufMgrHandleData->pMemPoolGroup))
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Group handle");
                result = CamxResultEInvalidArg;
            }

            pMemPoolMgr->Unlock();
        }
    }

    if (CamxResultSuccess == result)
    {
        pMemPoolGroup   = pBufMgrHandleData->pMemPoolGroup;

        result = pMemPoolGroup->MapBufferToDevices(pBufMgrHandleData->pMemPoolBufMgr, hMemPoolBufferHandle,
                                                   offset, size, flags, pDeviceIndices, deviceCount);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in MapBufferToDevices, result=%s", Utils::CamxResultToString(result));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::GetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolMgr::GetBufferInfo(
    MemPoolBufMgrHandle hMemPoolBufMgrHandle,
    MemPoolBufferHandle hMemPoolBufferHandle,
    CSLBufferInfo*      pCSLBufferInfo,
    BufferHandle*       phGrallocBuffer)
{
    MemPoolBufMgrHandleData*    pBufMgrHandleData   = static_cast<MemPoolBufMgrHandleData*>(hMemPoolBufMgrHandle);
    MemPoolGroup*               pMemPoolGroup;
    CamxResult                  result          = CamxResultSuccess;

    if ((NULL == pBufMgrHandleData)                 ||
        (NULL == pBufMgrHandleData->pMemPoolGroup)  ||
        (NULL == pBufMgrHandleData->pMemPoolBufMgr) ||
        (NULL == hMemPoolBufferHandle))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Buffer Manager Handle BufMgrHandle=%p BufferHandle=%p",
                       pBufMgrHandleData, hMemPoolBufferHandle);
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) && (TRUE == GetStaticSettings()->MPMValidateMemPoolGroup))
    {
        // Get singleton instance and lock mutex
        MemPoolMgr* pMemPoolMgr = GetInstanceLocked();

        if (NULL != pMemPoolMgr)
        {
            pMemPoolGroup = pBufMgrHandleData->pMemPoolGroup;

            if (FALSE == pMemPoolMgr->MemPoolGroupExists(pMemPoolGroup))
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid Group handle %p", pMemPoolGroup);
                result = CamxResultEInvalidArg;
            }

            pMemPoolMgr->Unlock();
        }
    }

    if (CamxResultSuccess == result)
    {
        pMemPoolGroup   = pBufMgrHandleData->pMemPoolGroup;

        result = pMemPoolGroup->GetBufferInfo(pBufMgrHandleData->pMemPoolBufMgr,
                                              hMemPoolBufferHandle,
                                              pCSLBufferInfo,
                                              phGrallocBuffer);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in getting buffer information result=%s",
                           Utils::CamxResultToString(result));
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::PrintMemPoolMgrState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolMgr::PrintMemPoolMgrState(
    BOOL forceTrigger)
{
    if ((TRUE == forceTrigger) || (TRUE == GetStaticSettings()->enableMPMStatelogging))
    {
        MemPoolMgr*     pMemPoolMgr;
        MemPoolGroup*   pMemPoolGroup;
        LDLLNode*       pNode;

        // Get singleton instance and lock mutex
        pMemPoolMgr = GetInstanceLocked();

        if (NULL != pMemPoolMgr)
        {
            CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "MemPoolMgr[%p] : Printing State============================================",
                            pMemPoolMgr);

            CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "s_isValid=%d, m_groupNameCounter=%d, Number of Groups=%d",
                            pMemPoolMgr->s_isValid, pMemPoolMgr->m_groupNameCounter, pMemPoolMgr->m_groupList.NumNodes());

            MemPoolGroup::PrintMemoryPoolManagerStats(forceTrigger);

            pNode = pMemPoolMgr->m_groupList.Head();

            // Iterate through existing groups and print state of each Group
            while (NULL != pNode)
            {
                pMemPoolGroup = static_cast<MemPoolGroup*>(pNode->pData);

                if (NULL != pMemPoolGroup)
                {
                    pMemPoolGroup->PrintMemPoolGroupState(forceTrigger);
                }

                pNode = LightweightDoublyLinkedList::NextNode(pNode);
            }

            CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "MemPoolMgr[%p] : Printing State Done=======================================",
                            pMemPoolMgr);

            pMemPoolMgr->Unlock();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::GetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemPoolMgr* MemPoolMgr::GetInstance()
{
    static MemPoolMgr s_memPoolMgrSingleton;

    MemPoolMgr* pMemPoolMgrSingleton = NULL;

    if (TRUE == s_isValid)
    {
        pMemPoolMgrSingleton =  &s_memPoolMgrSingleton;
    }

    return pMemPoolMgrSingleton;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::MonitorThread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* MemPoolMgr::MonitorThread(
    VOID* pArg)
{
    ThreadConfig* pThreadConfig = NULL;

    pThreadConfig = reinterpret_cast<ThreadConfig*>(pArg);

    CAMX_ASSERT(NULL != pThreadConfig);

    MemPoolMgr* pMemPoolMgr = reinterpret_cast<MemPoolMgr*>(pThreadConfig->pContext);
    pMemPoolMgr->Monitoring();

    return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::Monitoring
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* MemPoolMgr::Monitoring()
{
    UINT32          monitorTimeInterval = GetStaticSettings()->MPMMonitorTimeInterval;
    MemPoolMgr*     pMemPoolMgr;
    MemPoolGroup*   pMemPoolGroup;
    LDLLNode*       pNode;

    // The MPM monitor thread wakes up periodically to perform certain tasks.
    // One task is to free up idle buffers for each MemPoolGroup:
    //  Buffers will be allocated for offline pipeline when taking snapshots and remain in
    //  idle state once done processing and no more incoming snapshot requests. Freeing those
    //  idle buffers will significantly reduce memory footprint.
    // Other similar routines can be added to this monitor thread as well.
    while (TRUE == s_isValid)
    {
        pMemPoolMgr = GetInstanceLocked();
        if (NULL != pMemPoolMgr)
        {
            // Monitor starts when the first buffer manager is registered,
            // and stops when the last buffer manager is unregistered.
            // If m_monitorThreadStart is false, the thread waits indefinitly for signal from RegisterBufferManager call.
            if (FALSE == pMemPoolMgr->m_monitorThreadStart)
            {
                CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolMgr[%p] : wait on signal to start monitoring", pMemPoolMgr);
                if ((NULL != pMemPoolMgr->m_pMonitorThreadCond) && (NULL != pMemPoolMgr->m_pLock))
                {
                    pMemPoolMgr->m_pMonitorThreadCond->Wait(pMemPoolMgr->m_pLock->GetNativeHandle());

                    if (TRUE == pMemPoolMgr->m_monitorThreadStart)
                    {
                        CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolMgr[%p] : start monitoring, interval=%d",
                                      pMemPoolMgr, monitorTimeInterval);
                    }
                }
            }

            if (TRUE == pMemPoolMgr->m_monitorThreadStart)
            {
                // Use TimedWait instead of sleep in order to stop monitoring as soon as
                // receiving signal from UnregisterBufferManager
                if ((NULL != pMemPoolMgr->m_pMonitorThreadCond) && (NULL != pMemPoolMgr->m_pLock))
                {
                    pMemPoolMgr->m_pMonitorThreadCond->TimedWait(pMemPoolMgr->m_pLock->GetNativeHandle(), monitorTimeInterval);
                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                     "MemPoolMgr[%p] : Came out from wait - %d (0 - indefinite wait, 1 - self shrinking",
                                     pMemPoolMgr, pMemPoolMgr->m_monitorThreadStart);
                }

                if (TRUE == pMemPoolMgr->m_monitorThreadStart)
                {
                    pNode = pMemPoolMgr->m_groupList.Head();

                    // Iterate through existing groups and free idle buffers
                    while (NULL != pNode)
                    {
                        pMemPoolGroup = static_cast<MemPoolGroup*>(pNode->pData);

                        CAMX_ASSERT(NULL != pMemPoolGroup);

                        // Within one monitor interval, MemPoolGroup keeps track of the idle buffers count by updating
                        // m_numBuffersIdle in each GetBufferFromPool call:
                        //      m_numBuffersIdle = min(m_numBuffersIdle, m_freeBufferList.NumNodes())

                        // At the beginning of current monitor interval, we want to free up buffers that were idle
                        // from last interval, and reset m_numBuffersIdle for the current interval:
                        //      m_numBuffersIdle = m_freeBufferList.NumNodes()
                        pMemPoolGroup->FreeIdleBuffers();

                        pNode = LightweightDoublyLinkedList::NextNode(pNode);
                    }
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolMgr[%p] : end monitoring, goto indefinite wait state",
                                  pMemPoolMgr);
                }
            }

            pMemPoolMgr->Unlock();
        }
    }

    CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolMgr : terminate monitoring");

    return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::MemPoolMgr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemPoolMgr::MemPoolMgr()
    : m_pLock(NULL)
    , m_pMonitorThreadCond(NULL)
{
    m_pLock                           = Mutex::Create("MemPoolMgr");
    m_mpmMonitorThread.threadId       = 0;
    m_mpmMonitorThread.workThreadFunc = MonitorThread;
    m_mpmMonitorThread.pContext       = reinterpret_cast<VOID*>(this);
    m_pMonitorThreadCond              = Condition::Create("MPMMonitorThreadCond");
    m_monitorThreadStart              = FALSE;
    m_mpmFreeingPoolState             = FreeingPoolState::FreeingNotInProgress;
    m_pFreeingPoolStateCond           = Condition::Create("MPMFreeingPoolStateCond");

    CamxResult result = CamxResultSuccess;

    if ((NULL == m_pLock) || (NULL == m_pMonitorThreadCond) || (NULL == m_pFreeingPoolStateCond))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed to create resources! m_pLock=%p m_pMonitorThreadCond=%p",
                       m_pLock, m_pMonitorThreadCond);
        result = CamxResultENoMemory;
    }
    else
    {
        if (TRUE == GetStaticSettings()->enableMPMMonitorThread)
        {
            result = OsUtils::ThreadCreate(m_mpmMonitorThread.workThreadFunc,
                                           &m_mpmMonitorThread,
                                           &m_mpmMonitorThread.hWorkThread);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Couldn't create monitor thread! result=%s",
                               Utils::CamxResultToString(result));
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "Monitor thread created. hWorkThread=%p", m_mpmMonitorThread.hWorkThread);
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        if (NULL != m_pMonitorThreadCond)
        {
            m_pMonitorThreadCond->Destroy();
            m_pMonitorThreadCond = NULL;
        }

        if (NULL != m_pLock)
        {
            m_pLock->Destroy();
            m_pLock = NULL;
        }

        if (NULL != m_pFreeingPoolStateCond)
        {
            m_pFreeingPoolStateCond->Destroy();
            m_pFreeingPoolStateCond = NULL;
        }

        s_isValid = FALSE;
    }
    else
    {
        s_isValid = TRUE;
    }

    m_groupNameCounter = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::~MemPoolMgr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemPoolMgr::~MemPoolMgr()
{
    PrintMemPoolMgrState(FALSE);

    if (NULL != m_pLock)
    {
        m_pLock->Lock();
    }

    MemPoolGroup*   pMemPoolGroup;
    LDLLNode*       pNode = m_groupList.Head();

    // Iterate through existing groups and remove them
    while (NULL != pNode)
    {
        pMemPoolGroup = static_cast<MemPoolGroup*>(pNode->pData);

        // First remove the group from List
        RemoveMemPoolGroupFromList(pMemPoolGroup);

        CAMX_DELETE pMemPoolGroup;

        pNode = LightweightDoublyLinkedList::NextNode(pNode);
    }

    m_groupNameCounter  = 0;

    if (TRUE == s_isValid)
    {
        s_isValid = FALSE;

        // Stop the monitor thread
        m_monitorThreadStart = FALSE;
        m_pMonitorThreadCond->Signal();
        m_pFreeingPoolStateCond->Signal();

        if (NULL != m_pLock)
        {
            m_pLock->Unlock();
        }

        CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolMgr[%p] : Trigger Stop monitoring, waiting...", this);

        if (TRUE == GetStaticSettings()->enableMPMMonitorThread)
        {
            OsUtils::ThreadWait(m_mpmMonitorThread.hWorkThread);
        }

        CAMX_LOG_INFO(CamxLogGroupMemMgr, "MemPoolMgr[%p] : Thread stopped", this);

        if (NULL != m_pFreeingPoolStateCond)
        {
            m_pFreeingPoolStateCond->Destroy();
            m_pFreeingPoolStateCond = NULL;
        }

        if (NULL != m_pMonitorThreadCond)
        {
            m_pMonitorThreadCond->Destroy();
            m_pMonitorThreadCond = NULL;
        }

        // Destroy created objects in reverse order of creation
        if (NULL != m_pLock)
        {
            m_pLock->Destroy();
            m_pLock = NULL;
        }
    }
    else
    {
        if (NULL != m_pLock)
        {
            m_pLock->Unlock();
            m_pLock->Destroy();
            m_pLock = NULL;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::GetInstanceLocked
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MemPoolMgr* MemPoolMgr::GetInstanceLocked()
{
    MemPoolMgr* pMemPoolMgr;

    // Get singleton instance
    pMemPoolMgr = GetInstance();

    if (NULL != pMemPoolMgr)
    {
        if (NULL != pMemPoolMgr->m_pLock)
        {
            pMemPoolMgr->m_pLock->Lock();
        }
    }

    // Check again to make sure we have a valid instance in case the lock had been held by the destructor.
    pMemPoolMgr = GetInstance();

    return pMemPoolMgr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::Unlock
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolMgr::Unlock()
{
    if (NULL != m_pLock)
    {
        m_pLock->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::MemPoolGroupExists
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL MemPoolMgr::MemPoolGroupExists(
    MemPoolGroup*   pMemPoolGroup
    ) const
{
    BOOL        bGroupFound = FALSE;
    LDLLNode*   pNode       = m_groupList.Head();

    while (NULL != pNode)
    {
        if (pNode->pData == pMemPoolGroup)
        {
            bGroupFound = TRUE;
            break;
        }

        pNode = LightweightDoublyLinkedList::NextNode(pNode);
    }

    return bGroupFound;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::RegisterNewMemPoolGroup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolMgr::RegisterNewMemPoolGroup(
    const CHAR*                     pBufferManagerName,
    const BufferManagerCreateData*  pCreateData,
    MemPoolGroup**                  ppMemPoolGroup,
    MemPoolBufferManager**          ppMemPoolBufMgr)
{
    CamxResult      result          = CamxResultEFailed;
    MemPoolGroup*   pMemPoolGroup   = NULL;

    // create new mem pool group
    pMemPoolGroup = CAMX_NEW MemPoolGroup();

    if (NULL != pMemPoolGroup)
    {
        result = pMemPoolGroup->Initialize(m_groupNameCounter++,
                                           this,
                                           pCreateData->bufferProperties.bufferHeap,
                                           pCreateData->bNeedDedicatedBuffers,
                                           pCreateData->bDisableSelfShrinking);

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                             "MemPoolBufMgr[%s] : New MemPoolGroup[%s][%p] created",
                             pBufferManagerName, pMemPoolGroup->GetMemPoolGroupName(), pMemPoolGroup);

            // add new mem pool group to list
            result = AddMemPoolGroupToList(pMemPoolGroup);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in adding MemPoolGroup[%s] to list, result=%s",
                           pMemPoolGroup->GetMemPoolGroupName(), Utils::CamxResultToString(result));
            CAMX_DELETE pMemPoolGroup;
            pMemPoolGroup   = NULL;
            result          = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(NULL != pMemPoolGroup);

            *ppMemPoolBufMgr = pMemPoolGroup->RegisterBufferManager(pBufferManagerName, pCreateData, &result);

            if (NULL == *ppMemPoolBufMgr)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Failed in Register Buffer Manager");
                // registration failed, remove added mem pool group from active list
                RemoveMemPoolGroupFromList(pMemPoolGroup);

                CAMX_DELETE pMemPoolGroup;
                pMemPoolGroup = NULL;

                // update result status
                result = CamxResultEFailed;
            }
        }

         // update mem pool group handle
        *ppMemPoolGroup = pMemPoolGroup;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolBufMgr[%s] Ran out of memory to allocate", pBufferManagerName);
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::RegisterIfExistingGroupMatches
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolMgr::RegisterIfExistingGroupMatches(
    const CHAR*                     pBufferManagerName,
    const BufferManagerCreateData*  pCreateData,
    MemPoolGroup**                  ppMemPoolGroup,
    MemPoolBufferManager**          ppMemPoolBufMgr)
{
    CamxResult     result              = CamxResultENoSuch;
    LDLLNode*      pNode               = NULL;
    MemPoolGroup*  pMemPoolGroup       = NULL;
    BOOL           bIsMatchingGroup    = FALSE;

    // Try to find a matching group if DoNotGroup setting is FALSE. If DoNotGroup is enabled, each BufferManager
    // will have its own dedicated Group.
    // We can try to find a matching group only if this Buffer manager is fine with sharing buffers.
    // i.e If bNeedDedicatedBuffers is set to TRUE, create a dedicated group for this buffer manager.
    // Also don't group this buffer manager if self shrinking is disabled or its heap type is EGL.
    if ((FALSE            == GetStaticSettings()->MPMDoNotGroupBufferManagers) &&
        (FALSE            == pCreateData->bNeedDedicatedBuffers)               &&
        (FALSE            == pCreateData->bDisableSelfShrinking)               &&
        (CSLBufferHeapEGL != pCreateData->bufferProperties.bufferHeap))
    {
        pNode = m_groupList.Head();

        while (NULL != pNode)
        {
            pMemPoolGroup = static_cast<MemPoolGroup*>(pNode->pData);

            CAMX_ASSERT(NULL != pMemPoolGroup);

             // Checks if MemPoolGroup of active list matches to register the incoming Buffer Manager to it.
             // If MemPoolGroup doesn't match, result is updated with CamxResultENoSuch and iterates through
             // next MemPoolGroup in list. If MemPoolGroup matches and registration fails, then NULL MemPoolBufferManager
             // will be returned and results in functionality failure.
            *ppMemPoolBufMgr = pMemPoolGroup->RegisterBufferManager(pBufferManagerName, pCreateData, &result);

            if ((CamxResultSuccess == result) && (NULL != *ppMemPoolBufMgr))
            {
                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s] matched for BufferManager[%s] in Active Group",
                                 pMemPoolGroup->GetMemPoolGroupName(), pBufferManagerName);
                // updated memory pool group handle with the matching group in active list
                *ppMemPoolGroup = pMemPoolGroup;
                break;
            }
            else if ((CamxResultENoSuch != result) && (NULL == *ppMemPoolBufMgr))
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolGroup[%s] of Active Group and BufMgr[%s] Registration failed %s",
                               pMemPoolGroup->GetMemPoolGroupName(), pBufferManagerName, Utils::CamxResultToString(result));
                break;
            }

            pNode = LightweightDoublyLinkedList::NextNode(pNode);
        }
    }

    // if result is CamxResultENoSuch at this stage, it means no matching MemPoolGroup found,
    // and a new MemPoolGroup has to be created.

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::AddMemPoolGroupToList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolMgr::AddMemPoolGroupToList(
    MemPoolGroup*   pMemPoolGroup)
{
    LDLLNode*   pNode;
    CamxResult  result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "Number of MemPool groups in list before adding %d", m_groupList.NumNodes());

    pNode = static_cast<LDLLNode*>(CAMX_CALLOC(sizeof(LDLLNode)));

    if (NULL != pNode)
    {
        pNode->pData = pMemPoolGroup;
        m_groupList.InsertToTail(pNode);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Insufficient Memory");
        result = CamxResultENoMemory;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::RemoveMemPoolGroupFromList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MemPoolMgr::RemoveMemPoolGroupFromList(
    MemPoolGroup*   pMemPoolGroup)
{
    LDLLNode*   pNode;
    CamxResult  result      = CamxResultSuccess;
    BOOL        bGroupFound = FALSE;

    pNode = m_groupList.Head();

    while (NULL != pNode)
    {
        if (pNode->pData == pMemPoolGroup)
        {
            bGroupFound = TRUE;
            break;
        }

        pNode = LightweightDoublyLinkedList::NextNode(pNode);
    }

    if (TRUE == bGroupFound)
    {
        m_groupList.RemoveNode(pNode);

        CAMX_FREE(pNode);
        pNode = NULL;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "Number of MemPool groups in list after removing %d", m_groupList.NumNodes());

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MemPoolMgr::ClearAllMemPoolFreeList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MemPoolMgr::ClearAllMemPoolFreeList()
{
    UINT            numNodesToFree  = 0;
    UINT32          numNodesFreed   = 0;
    BOOL            clearMemory;
    MemPoolMgr*     pMemPoolMgr;
    MemPoolGroup*   pMemPoolGroup;
    LDLLNode*       pNode;

    // Get singleton instance
    pMemPoolMgr = GetInstanceLocked();

    if (NULL != pMemPoolMgr)
    {
        FreeingPoolState curFreeingPoolState = GetCurMgrFreeingPoolState();
        CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "MemPoolMgr[%p] Current freeing state:%d", pMemPoolMgr, curFreeingPoolState);

        switch (curFreeingPoolState)
        {
            case FreeingPoolState::FreeingNotInProgress:
                // We should clear memory if freeing is not in progress
                clearMemory = TRUE;
                break;

            case FreeingPoolState::FreeingInProgress:
                // If freeing is in progress wait here until we hit freeing complete,
                // then don't free the memory list again
                pMemPoolMgr->WaitOnFreeingStateComplete();
                clearMemory = FALSE;
                break;

            case FreeingPoolState::FreeingComplete:
                // If freeing is complete it means we recently freed memory so adequate memory
                // most likely exists. Don't clear all the free lists, allow CSLAlloc to
                // retry again. On second failure, we will come again and free the lists
                SetCurMgrFreeingPoolState(FreeingPoolState::FreeingNotInProgress);
                clearMemory = FALSE;
                break;

            default:
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "MemPoolMgr[%p] Called with unknown FreeingPoolState:%d, "
                               "not freeing any memory",
                               pMemPoolMgr,
                               curFreeingPoolState);
                clearMemory = FALSE;
                break;
        }

        // Iterate through existing groups and clear all buffers in each group's free list
        if (TRUE == clearMemory)
        {
            SetCurMgrFreeingPoolState(FreeingPoolState::FreeingInProgress);
            CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "MemPoolMgr[%p], FreeingInProgress; freeing %u mem pool groups free list",
                            pMemPoolMgr, pMemPoolMgr->m_groupList.NumNodes());

            pNode = pMemPoolMgr->m_groupList.Head();
            while (NULL != pNode)
            {
                pMemPoolGroup = static_cast<MemPoolGroup*>(pNode->pData);
                if (NULL != pMemPoolGroup)
                {
                    pMemPoolGroup->FreeAllFreeListBuffers();
                }

                pNode = LightweightDoublyLinkedList::NextNode(pNode);
            }

            SetCurMgrFreeingPoolState(FreeingPoolState::FreeingComplete);
            CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "MemPoolMgr[%p], FreeingComplete", pMemPoolMgr);
        }
        pMemPoolMgr->Unlock();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "pMemPoolManager is NULL! Cannot free mem pool groups buffers!");
    }
}

CAMX_NAMESPACE_END
