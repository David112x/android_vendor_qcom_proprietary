////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternal.cpp
///
/// @brief CamxCSL Hw Internal utils implementation for hw csl variant
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <ctime>
#include <errno.h>
#include "camxtypes.h"
#include "camxcsl.h"
#include "camxcsljumptable.h"
#include "camxcslhwinternal.h"
#include "camxincs.h"
#include "camxlist.h"
#include "camxmem.h"
#include "camxpacketdefs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwIsHwInstanceValid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwIsHwInstanceValid()
{
    BOOL returnCode = FALSE;

    switch (CamX::CamxAtomicLoad32(&g_CSLHwInstance.aState))
    {
        case CSLHwValidState:
            returnCode = TRUE;
            break;
        default:
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwIsHwInstanceState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwIsHwInstanceState(
    CSLHwInternalState state)
{
    BOOL returnCode = FALSE;

    if ((CamX::CamxAtomicLoad32(&g_CSLHwInstance.aState)) == state)
    {
        returnCode = TRUE;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwKmdGroupidToCslDevicetype
///
/// @brief  This api converts KMD groupid to CSL internal devicetype
///
/// @param  groupId     KMD groupid
/// @param  pType       pointer to update the CSL internal devicetype
///
/// @return boolean TRUE on success or FALSE on failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult CSLHwKmdGroupidToCslDevicetype(
    UINT32                   groupId,
    CSLHwInternalDeviceType* pType)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pType);

    switch (groupId)
    {
        case CAM_FD_DEVICE_TYPE:
            *pType = CSLHwFD;
            break;
        case CAM_LRME_DEVICE_TYPE:
            *pType = CSLHwLRME;
            break;
        case CAM_IFE_DEVICE_TYPE:
            *pType = CSLHwIFE;
            break;
        case CAM_JPEG_DEVICE_TYPE:
            *pType = CSLHwJPEGE;
            break;
        case CAM_ICP_DEVICE_TYPE:
            *pType = CSLHwICP;
            break;
        case CAM_VNODE_DEVICE_TYPE:
            *pType = CSLHwRequestManager;
            break;
        case CAM_CPAS_DEVICE_TYPE:
            *pType = CSLHwCPAS_TOP;
            break;
        case CAM_SENSOR_DEVICE_TYPE:
            *pType = CSLHwImageSensor;
            break;
        case CAM_CSIPHY_DEVICE_TYPE:
            *pType = CSLHwCSIPHY;
            break;
        case CAM_ACTUATOR_DEVICE_TYPE:
            *pType = CSLHwLensActuator;
            break;
        case CAM_EEPROM_DEVICE_TYPE:
            *pType = CSLHwEEPROM;
            break;
        case CAM_FLASH_DEVICE_TYPE:
            *pType = CSLHwFlash;
            break;
        case CAM_OIS_DEVICE_TYPE:
            *pType = CSLHwOIS;
            break;
        case CAM_CUSTOM_DEVICE_TYPE:
            *pType = CSLHwCustom;
            break;
        default :
            *pType = CSLHwInvalidDevice;
            result = CamxResultEFailed;
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Got a invalid groupId=%d", groupId);
            break;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwGetKmdSyncTypeFromCSLSyncType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 CSLHwGetKmdSyncTypeFromCSLSyncType(
    CSLSyncLinkMode syncMode)
{
    INT32 syncType;

    switch (syncMode)
    {
        case CSLSyncLinkModeNoSync:
            syncType = CAM_REQ_MGR_SYNC_MODE_NO_SYNC;
            break;
        case CSLSyncLinkModeSync:
            syncType = CAM_REQ_MGR_SYNC_MODE_SYNC;
            break;
        default:
            syncType = CAM_REQ_MGR_SYNC_MODE_NO_SYNC;
            break;
    }

    return syncType;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwGetDeviceTypeFromInternal
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLDeviceType CSLHwGetDeviceTypeFromInternal(
    CSLHwInternalDeviceType  internalType)
{
    CSLDeviceType result = CSLDeviceTypeInvalidDevice;

    switch (internalType)
    {
        case CSLHwImageSensor:
            result = CSLDeviceTypeImageSensor;
            break;
        case CSLHwLensActuator:
            result = CSLDeviceTypeLensActuator;
            break;
        case CSLHwCompanion:
            result = CSLDeviceTypeCompanion;
            break;
        case CSLHwEEPROM:
            result = CSLDeviceTypeEEPROM;
            break;
        case CSLHwCSIPHY:
            result = CSLDeviceTypeCSIPHY;
            break;
        case CSLHwOIS:
            result = CSLDeviceTypeOIS;
            break;
        case CSLHwFlash:
            result = CSLDeviceTypeFlash;
            break;
        case CSLHwFD:
            result = CSLDeviceTypeFD;
            break;
        case CSLHwJPEGE:
            result = CSLDeviceTypeJPEGE;
            break;
        case CSLHwJPEGD:
            result = CSLDeviceTypeJPEGD;
            break;
        case CSLHwVFE:
            result = CSLDeviceTypeVFE;
            break;
        case CSLHwCPP:
            result = CSLDeviceTypeCPP;
            break;
        case CSLHwCSID:
            result = CSLDeviceTypeCSID;
            break;
        case CSLHwISPIF:
            result = CSLDeviceTypeISPIF;
            break;
        case CSLHwIFE:
            result = CSLDeviceTypeIFE;
            break;
        case CSLHwICP:
            result = CSLDeviceTypeICP;
            break;
        case CSLHwLRME:
            result = CSLDeviceTypeLRME;
            break;
        case CSLHwCustom:
            result = CSLDeviceTypeCustom;
            break;
        case CSLHwMAXDEVICE:
            result = CSLDeviceTypeMaxDevice;
            break;
        default:
            break;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwIsHwRealtimeDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwIsHwRealtimeDevice(
    CSLHwInternalDeviceType  type)
{
    BOOL returnCode = FALSE;

    switch (type)
    {
        case CSLHwImageSensor:
        case CSLHwCompanion:
        case CSLHwCSIPHY:
        case CSLHwVFE:
        case CSLHwCSID:
        case CSLHwISPIF:
        case CSLHwIFE:
        case CSLHwFlash:
        case CSLHwLensActuator:
        case CSLHwCustom:
            returnCode =TRUE;
            break;
        case CSLHwEEPROM:
        case CSLHwOIS:
        case CSLHwFD:
        case CSLHwJPEGE:
        case CSLHwJPEGD:
        case CSLHwCPP:
        case CSLHwICP:
        case CSLHwLRME:
        default:
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwRtDeviceOderCompare
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CSLHwRtDeviceOderCompare(
    const VOID* p1,
    const VOID* p2)
{
    return ((static_cast<const CSLHwAcquiredDevice*>(p1))->order) - ((static_cast<const CSLHwAcquiredDevice*>(p2))->order);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwRtDeviceStreamoffOrderCompare
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CSLHwRtDeviceStreamoffOrderCompare(
    const VOID* p1,
    const VOID* p2)
{
    return ((static_cast<const CSLHwAcquiredDevice *>(p1))->orderoff)
        - ((static_cast<const CSLHwAcquiredDevice *>(p2))->orderoff);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwGetHwDeviceOrder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwGetHwDeviceOrder(
    CSLHwInternalDeviceType type,
    INT*                    pOrder)
{
    BOOL returnCode = FALSE;

    /// @todo (CAMX-2582): CSL magic number cleanup
    switch (static_cast<uint32_t>(type))
    {
        case CSLHwCSIPHY:
            *pOrder    = 2;
            returnCode =TRUE;
            break;
        case CSLHwImageSensor:
        case CSLHwCompanion:
        case CSLHwFlash:
        case static_cast<uint32_t>(CSLDeviceTypeLensActuator):
            *pOrder    = 3;
            returnCode =TRUE;
            break;
        case CSLHwVFE:
        case CSLHwCSID:
        case CSLHwISPIF:
        case CSLHwIFE:
            *pOrder    = 1;
            returnCode =TRUE;
            break;
        default:
            *pOrder = -1;
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwGetHwDeviceStreamOffOrder
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwGetHwDeviceStreamOffOrder(
    CSLHwInternalDeviceType type,
    INT*                    pOrder)
{
    BOOL returnCode = FALSE;

    /// @todo (CAMX-2582): CSL magic number cleanup
    switch (type)
    {
        case CSLHwImageSensor:
        case CSLHwCompanion:
            *pOrder    = 2;
            returnCode =TRUE;
            break;

        case CSLHwCSIPHY:
        case CSLHwFlash:
            *pOrder    = 3;
            returnCode =TRUE;
            break;
        case CSLHwVFE:
        case CSLHwCSID:
        case CSLHwISPIF:
        case CSLHwIFE:
            *pOrder    = 1;
            returnCode =TRUE;
            break;
        default:
            *pOrder = -1;
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwIsKmdGroupidCslInternalDevicetype
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwIsKmdGroupidCslInternalDevicetype(
    UINT32 groupId)
{
    BOOL returnCode = FALSE;

    switch (groupId)
    {
        case CAM_VNODE_DEVICE_TYPE:
        case CAM_CPAS_DEVICE_TYPE:
            returnCode = TRUE;
            break;
        default :
            break;
    }
    return returnCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInstanceGetState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 CSLHwInstanceGetState()
{
    return CamX::CamxAtomicLoad32(&g_CSLHwInstance.aState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInstanceSetState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInstanceSetState(
    CSLHwInternalState state)
{
    CamxResult result       = CamxResultEFailed;
    INT32      currentstate = CamX::CamxAtomicLoad32(&g_CSLHwInstance.aState);

    switch (state)
    {
        case CSLHwInvalidState:
            result = CamxResultSuccess;
            break;
        case CSLHwValidState:
            if (CSLHwInvalidState == currentstate)
            {
                result = CamxResultSuccess;
            }
            break;
        case CSLHwDestroyingState:
            if (CSLHwValidState == currentstate)
            {
                result = CamxResultSuccess;
            }
            break;
        case CSLHwErrorState:
            result = CamxResultSuccess;
            break;
        default:
            break;
    }
    if (CamxResultSuccess == result)
    {
        CamX::CamxAtomicStore32(&g_CSLHwInstance.aState, state);
    }

    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Setting Invalid CSLHwInstanceSetState state as %d from = %d",
                         state, currentstate);

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInstanceGetRefCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInstanceGetRefCount()
{
    BOOL  returnCode = FALSE;
    INT32 state      = CamX::CamxAtomicLoad32(&g_CSLHwInstance.aState);

    switch (state)
    {
        case CSLHwValidState:
            g_CSLHwInstance.lock->Lock();
            g_CSLHwInstance.refcount++;
            g_CSLHwInstance.lock->Unlock();
            returnCode = TRUE;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid get for CSLHwInstanceGetRefCount state = %d", state);
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInstancePutRefCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwInstancePutRefCount()
{
    INT32 state = CamX::CamxAtomicLoad32(&g_CSLHwInstance.aState);

    switch (state)
    {
        case CSLHwValidState:
        case CSLHwDestroyingState:
            g_CSLHwInstance.lock->Lock();
            g_CSLHwInstance.refcount--;
            if ((0 == g_CSLHwInstance.refcount) && (CSLHwDestroyingState == state))
            {
                g_CSLHwInstance.destroyCondition->Signal();
            }
            g_CSLHwInstance.lock->Unlock();
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid put for CSLHwInstancePutRefCount state=%d", state);
            break;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInstanceWaitForRefcountZero
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwInstanceWaitForRefcountZero()
{
    BOOL       returnCode   = FALSE;
    CamxResult result       = CamxResultEFailed;

    switch (CamX::CamxAtomicLoad32(&g_CSLHwInstance.aState))
    {
        case CSLHwValidState:
            g_CSLHwInstance.lock->Lock();
            CSLHwInstanceSetState(CSLHwDestroyingState);
            if (0 == g_CSLHwInstance.refcount)
            {
                returnCode = TRUE;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Current ref count %d", g_CSLHwInstance.refcount);
                CAMX_ASSERT_ALWAYS_MESSAGE("CSL HW instance being destroyed before all references were released\n");
                result = g_CSLHwInstance.destroyCondition->Wait(
                    g_CSLHwInstance.lock->GetNativeHandle());
                if (CamxResultSuccess == result)
                {
                    // Now make sure all the sessions are teared down
                    returnCode = TRUE;
                }
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "After Wait ref count %d", g_CSLHwInstance.refcount);
            }
            g_CSLHwInstance.lock->Unlock();
            break;
        case CSLHwDestroyingState:
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Already in destroying state");
            break;
        default:
            break;
    }
    return returnCode;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwIsSessionStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwIsSessionStreamOn(
    CSLHwsession* pSession)
{
    BOOL  returnCode = FALSE;
    INT32 state      = CamX::CamxAtomicLoad32(&pSession->aState);

    switch (state)
    {
        case CSLHwValidState:
            pSession->lock->Lock();
            returnCode = pSession->streamOn;
            pSession->lock->Unlock();
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid get for CSLHwIsSessionStreamOn state=%d", state);
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwGetSessionState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 CSLHwGetSessionState(
    CSLHwsession* pSession)
{
    return CamX::CamxAtomicLoad32(&pSession->aState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSetSessionState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSetSessionState(
    CSLHwsession*      pSession,
    CSLHwInternalState state)
{
    CamxResult result       = CamxResultEFailed;
    INT32      currentstate = CamX::CamxAtomicLoad32(&pSession->aState);

    switch (state)
    {
        case CSLHwInvalidState:
            result = CamxResultSuccess;
            break;
        case CSLHwValidState:
            if ((CSLHwInvalidState == currentstate) || (CSLHwFlushState == currentstate))
            {
                result = CamxResultSuccess;
            }
            break;
        case CSLHwDestroyingState:
            if ((CSLHwValidState == currentstate) || (CSLHwFlushState == currentstate) ||
                (CSLHwErrorState == currentstate))
            {
                result = CamxResultSuccess;
            }
            break;
        case CSLHwErrorState:
            result = CamxResultSuccess;
            break;
        case CSLHwFlushState:
            result = CamxResultSuccess;
            break;
        default:
            break;
    }
    if (CamxResultSuccess == result)
    {
        CamX::CamxAtomicStore32(&pSession->aState, state);
    }

    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Setting Invalid CSLHwSetSessionState state as %d from = %d",
                        state, currentstate);

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSessionGetRefCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwSessionGetRefCount(
    CSLHwsession* pSession)
{
    BOOL  returnCode = FALSE;
    INT32 state      = CamX::CamxAtomicLoad32(&pSession->aState);

    switch (state)
    {
        case CSLHwValidState:
        case CSLHwFlushState:
            pSession->lock->Lock();
            pSession->refCount++;
            pSession->lock->Unlock();
            returnCode = TRUE;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid get for CSLHwSessionGetRefCount state=%d", state);
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSessionPutRefCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwSessionPutRefCount(
    CSLHwsession* pSession)
{
    INT32 state = CamX::CamxAtomicLoad32(&pSession->aState);

    switch (state)
    {
        case CSLHwValidState:
        case CSLHwDestroyingState:
        case CSLHwFlushState:
            pSession->lock->Lock();
            pSession->refCount--;
            if ((0 == pSession->refCount) && (CSLHwDestroyingState == state))
            {
                pSession->destroyCondition->Signal();
            }
            pSession->lock->Unlock();
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid put for CSLHwInstance state=%d", state);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSessionWaitForRefcountZero
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwSessionWaitForRefcountZero(
    CSLHwsession* pSession)
{
    BOOL       returnCode   = FALSE;
    CamxResult result       = CamxResultEFailed;

    switch (CamX::CamxAtomicLoad32(&pSession->aState))
    {
        case CSLHwValidState:
            pSession->lock->Lock();
            CSLHwSetSessionState(pSession, CSLHwDestroyingState);
            if (0 == pSession->refCount)
            {
                returnCode = TRUE;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Current ref count %d", pSession->refCount);
                CAMX_ASSERT_ALWAYS_MESSAGE("CSL HW Session being destroyed before all references were released\n");
                result = pSession->destroyCondition->Wait(pSession->lock->GetNativeHandle());
                if (CamxResultSuccess == result)
                {
                    // Now make sure the streamoff is done and see if any acquired hardwares needs to be released
                    returnCode = TRUE;
                }
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "After Wait ref count %d", pSession->refCount);
            }
            pSession->lock->Unlock();
            break;
        case CSLHwDestroyingState:
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Already in destroying state");
            break;
        default:
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwAcquiredHWGetRefcount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwAcquiredHWGetRefcount(
    CSLHwAcquiredDevice* pAcquiredHw)
{
    BOOL  returnCode = FALSE;
    INT32 state      = CamX::CamxAtomicLoad32(&pAcquiredHw->aState);

    switch (state)
    {
        case CSLHwValidState:
            pAcquiredHw->lock->Lock();
            pAcquiredHw->refCount++;
            pAcquiredHw->lock->Unlock();
            returnCode = TRUE;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid get for CSLHwAcquiredHWGetRefcount state=%d", state);
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwAcquiredHWPutRefcount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwAcquiredHWPutRefcount(
    CSLHwAcquiredDevice* pAcquiredHw)
{
    BOOL  returnCode = FALSE;
    INT32 state      = CamX::CamxAtomicLoad32(&pAcquiredHw->aState);

    switch (state)
    {
        case CSLHwValidState:
        case CSLHwDestroyingState:
            pAcquiredHw->lock->Lock();
            pAcquiredHw->refCount--;
            if ((0 == pAcquiredHw->refCount) && (CSLHwDestroyingState == state))
            {
                pAcquiredHw->destroyCondition->Signal();
            }
            pAcquiredHw->lock->Unlock();
            returnCode = TRUE;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid put for CSLHwInstance state=%d", state);
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwAcquiredHWWaitForRefcountZero
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwAcquiredHWWaitForRefcountZero(
    CSLHwAcquiredDevice* pAcquiredHw)
{
    BOOL       returnCode   = FALSE;
    CamxResult result       = CamxResultEFailed;

    switch (CamX::CamxAtomicLoad32(&pAcquiredHw->aState))
    {
        case CSLHwValidState:
            pAcquiredHw->lock->Lock();
            if (0 == pAcquiredHw->refCount)
            {
                pAcquiredHw->aState = CSLHwDestroyingState;
                returnCode = TRUE;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Current ref count %d", pAcquiredHw->refCount);
                CAMX_ASSERT_ALWAYS_MESSAGE("This should not happen if UMD clean properly\n");
                result = pAcquiredHw->destroyCondition->Wait(pAcquiredHw->lock->GetNativeHandle());
                if (CamxResultSuccess == result)
                {
                    // Now make sure the streamoff is done and see if any acquired hardwares needs to be released
                    returnCode = TRUE;
                }
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "After Wait ref count %d", pAcquiredHw->refCount);
            }
            pAcquiredHw->lock->Unlock();
            break;
        case CSLHwDestroyingState:
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Already in destroying state");
            break;
        default:
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwKMDDeviceGetState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 CSLHwKMDDeviceGetState(
    CSLHwDevice* pHWDevice)
{
    return CamX::CamxAtomicLoad32(&pHWDevice->aState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwKMDDeviceSetState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwKMDDeviceSetState(
    CSLHwDevice*       pHWDevice,
    CSLHwInternalState state)
{
    CamxResult result       = CamxResultEFailed;
    INT32      currentstate = CamX::CamxAtomicLoad32(&pHWDevice->aState);

    switch (state)
    {
        case CSLHwInvalidState:
            result = CamxResultSuccess;
            break;
        case CSLHwValidState:
            if (CSLHwInvalidState == currentstate)
            {
                result = CamxResultSuccess;
            }
            break;
        case CSLHwDestroyingState:
            if (CSLHwValidState == currentstate)
            {
                result = CamxResultSuccess;
            }
            break;
        case CSLHwErrorState:
            result = CamxResultSuccess;
            break;
        default:
            break;
    }
    if (CamxResultSuccess == result)
    {
        CamX::CamxAtomicStore32(&pHWDevice->aState, state);
    }

    CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Setting Invalid CSLHwKMDDeviceSetState state as %d from = %d",
                        state, currentstate);

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwKMDDeviceGetRefcount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwKMDDeviceGetRefcount(
    CSLHwDevice* pHWDevice)
{
    BOOL  returnCode = FALSE;
    INT32 state      = CamX::CamxAtomicLoad32(&pHWDevice->aState);

    switch (state)
    {
        case CSLHwValidState:
            pHWDevice->lock->Lock();
            pHWDevice->refCount++;
            pHWDevice->lock->Unlock();
            returnCode = TRUE;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid get for CSLHwKMDDeviceGetRefcount state=%d", state);
            break;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwKMDDevicePutRefcount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwKMDDevicePutRefcount(
    CSLHwDevice* pHWDevice)
{
    INT32 state = CamX::CamxAtomicLoad32(&pHWDevice->aState);

    switch (state)
    {
        case CSLHwValidState:
        case CSLHwDestroyingState:
            pHWDevice->lock->Lock();
            pHWDevice->refCount--;
            if ((0 == pHWDevice->refCount) && (CSLHwDestroyingState == state))
            {
                pHWDevice->destroyCondition->Signal();
            }
            pHWDevice->lock->Unlock();
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid put for CSLHwInstance state=%d", state);
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwKMDDeviceWaitForRefcountZero
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwKMDDeviceWaitForRefcountZero(
    CSLHwDevice* pHWDevice)
{
    BOOL       returnCode   = FALSE;
    CamxResult result       = CamxResultEFailed;

    switch (CamX::CamxAtomicLoad32(&pHWDevice->aState))
    {
        case CSLHwValidState:
            pHWDevice->lock->Lock();
            if (0 == pHWDevice->refCount)
            {
                pHWDevice->aState = CSLHwDestroyingState;
                returnCode        = TRUE;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Current ref count %d", pHWDevice->refCount);
                CAMX_ASSERT_ALWAYS_MESSAGE("This should not happen if UMD clean properly");
                result = pHWDevice->destroyCondition->Wait(pHWDevice->lock->GetNativeHandle());
                if (CamxResultSuccess == result)
                {
                    returnCode = TRUE;
                }
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "After Wait ref count %d", pHWDevice->refCount);
            }
            pHWDevice->lock->Unlock();
            break;
        case CSLHwDestroyingState:
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Already in destroying state");
            break;
        default:
            break;
    }
    return returnCode;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwAddFdToPoll
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwAddFdToPoll(
    INT fd)
{
    CSLInternalHwPipeMessage data;
    INT                      result     = 0;
    BOOL                     returnCode = TRUE;

    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Adding fd to poll =%d and Pipe write fd is %d", fd, g_CSLHwInstance.pipeFd[1]);
    data.type = CSLHwPipeMessageAddFd;
    data.fd   = fd;
    result    = write(g_CSLHwInstance.pipeFd[1], &data, sizeof(CSLInternalHwPipeMessage));
    if (result != sizeof(CSLInternalHwPipeMessage))
    {
        CHAR errnoStr[100] = {0};
        CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
        CAMX_ASSERT_ALWAYS_MESSAGE("Write failed for Add fd to pipe with reason = %s", errnoStr);
        returnCode = FALSE;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwRemoveFromPoll
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwRemoveFromPoll(
    INT fd)
{
    CSLInternalHwPipeMessage data;
    INT                      result     = 0;
    BOOL                     returnCode = TRUE;

    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Removing fd from poll =%d and Pipe write fd is %d", fd, g_CSLHwInstance.pipeFd[1]);
    data.type = CSLHwPipeMessageDeleteFd;
    data.fd   = fd;
    result    = write(g_CSLHwInstance.pipeFd[1], &data, sizeof(CSLInternalHwPipeMessage));
    if (result != sizeof(CSLInternalHwPipeMessage))
    {
        CHAR errnoStr[100] = {0};
        CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
        CAMX_ASSERT_ALWAYS_MESSAGE("Write failed for Add fd to pipe with reason = %s", errnoStr);
        returnCode = FALSE;
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalDefaultSubscribeEvents
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalDefaultSubscribeEvents(
    CSLHwDevice*    pDevice,
    UINT32          id,
    UINT32          type)
{
    struct v4l2_event_subscription sub;

    sub.id   = id;
    sub.type = type;

    return CSLHwInternalDefaultIoctl(pDevice, VIDIOC_SUBSCRIBE_EVENT, &sub);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSendExitToPoll
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwSendExitToPoll()
{
    CSLInternalHwPipeMessage data;
    INT                      result     = 0;
    BOOL                     returnCode = TRUE;

    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Sending exit to poll thread");
    data.type  = CSLHwPipeMessageExitThread;
    result     = write(g_CSLHwInstance.pipeFd[1], &data, sizeof(CSLInternalHwPipeMessage));
    if (result != sizeof(CSLInternalHwPipeMessage))
    {
        CHAR errnoStr[100] = {0};
        CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
        CAMX_ASSERT_ALWAYS_MESSAGE("Write failed for sending Exit to pipe with reason = %s", errnoStr);
        returnCode = FALSE;
    }
    return returnCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalDQEvent
///
/// @brief  This API dequeues subscribed V4L2 events
///
/// @param  fileDes    File descriptor identifying device from which to dequeue event
/// @param  pEvent     pointer to V4L2 Event
///
/// @return Status     CamxResultSuccess on success, failure otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult CSLHwInternalDQEvent(
    INT                fileDes,
    struct v4l2_event* pEvent)
{
    INT returnCode       = -1;
    CamxResult    result = CamxResultEFailed;

    returnCode = ioctl(fileDes, VIDIOC_DQEVENT, pEvent);

    if (returnCode < 0)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "DQEVENT failed for File Descriptor %d", fileDes);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "DQEVENT Success for File Descriptor %d", fileDes);
        result = CamxResultSuccess;
    }

    return returnCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwFindPipelineIndex
///
/// @brief  This function returns the pipeline index for which the message handler was registered
///
/// @param  pMessage    Message received from kernel
/// @param  type        Type of event received
/// @param  pIndex      pointer to index to matching link handle
///
/// @return UINT32      CamxResultSuccess on success, failure otherwise
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 CSLHwFindPipelineIndex(
    struct cam_req_mgr_message* pMessage, UINT32 type, UINT32* pIndex)
{
    CSLHandle     hSession;
    CSLHwsession* pSession = NULL;
    CamxResult    result = CamxResultEFailed;

    if (NULL == pMessage)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("pEvent is NULL");
    }
    else
    {
        hSession = pMessage->session_hdl;

        if (CSLInvalidHandle == hSession)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid session handle");
        }
        else
        {
            UINT sessionIndex = CAM_REQ_MGR_GET_HDL_IDX(hSession);
            if (sessionIndex < CSLHwMaxNumSessions)
            {
                pSession = &g_CSLHwInstance.sessionList[sessionIndex];
                CAMX_ASSERT(hSession == pSession->hSession);
                if ((V4L_EVENT_CAM_REQ_MGR_SOF == type) || (V4L_EVENT_CAM_REQ_MGR_SOF_BOOT_TS == type))
                {
                    for (UINT32 i = 0; i < CSLHwMaxNumSessions; i++)
                    {
                        if (pSession->CSLLinkHandleData[i].hCSLLinkHandle == pMessage->u.frame_msg.link_hdl)
                        {
                            *pIndex = i;
                            result = CamxResultSuccess;
                            break;
                        }
                    }
                }
                else if (V4L_EVENT_CAM_REQ_MGR_ERROR == type)
                {
                    for (UINT32 i = 0; i < CSLHwMaxNumSessions; i++)
                    {
                        if (pSession->CSLLinkHandleData[i].hCSLLinkHandle == pMessage->u.err_msg.link_hdl)
                        {
                            *pIndex = i;
                            result = CamxResultSuccess;
                            break;
                        }
                    }
                }
            }
        }
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwInternalSendRequestManagerEvent
///
/// @brief  This API sends event messages to the clients subscribed for events
///
/// @param  pEvent     pointer to V4L2 Event
///
/// @return boolean TRUE on success or FALSE on failure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult CSLHwInternalSendRequestManagerEvent(
    struct v4l2_event* pEvent)
{
    CamxResult                  result   = CamxResultEFailed;
    CSLHandle                   hSession;
    CSLHwsession*               pSession = NULL;
    struct cam_req_mgr_message* pMessage = NULL;
    CSLMessage                  newCSLMessage;
    UINT32                      index = 0;

    if (NULL == pEvent)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("pEvent is NULL");
        return CamxResultEInvalidArg;
    }

    pMessage = reinterpret_cast<struct cam_req_mgr_message*>(pEvent->u.data);
    hSession = pMessage->session_hdl;

    if ((0 == pMessage->u.frame_msg.request_id) &&
       ((pEvent->id != V4L_EVENT_CAM_REQ_MGR_SOF) &&
       (pEvent->id != V4L_EVENT_CAM_REQ_MGR_ERROR))) // Ignore event if request_id 0 for multi camera single zone
    {
        return CamxResultSuccess;
    }
    result = CSLHwFindPipelineIndex(pMessage, pEvent->id, &index);

    if (CamxResultSuccess != result)
    {
        if (CSLInvalidHandle == hSession)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid session handle");
            result = CamxResultEFailed;
        }
        else
        {
            UINT sessionIndex = CAM_REQ_MGR_GET_HDL_IDX(hSession);
            if (sessionIndex < CSLHwMaxNumSessions)
            {
                pSession = &g_CSLHwInstance.sessionList[sessionIndex];
                if (NULL != pSession->sessionMessageHandler)
                {
                    switch(pEvent->id)
                    {
                        case V4L_EVENT_CAM_REQ_MGR_ERROR:
                            newCSLMessage.type                               = CSLMessageTypeError;
                            newCSLMessage.message.errorMessage.errorType     =
                                static_cast<CSLErrorMessageCode>(pMessage->u.err_msg.error_type);
                            newCSLMessage.message.errorMessage.requestID     = pMessage->u.err_msg.request_id;
                            newCSLMessage.message.errorMessage.hDevice       = pMessage->u.err_msg.device_hdl;
                            newCSLMessage.message.errorMessage.resourceIndex = pMessage->u.err_msg.resource_size;
                            CAMX_LOG_ERROR(CamxLogGroupCSL, "frame error: type %d, requestID %llu, device hdl %d",
                                newCSLMessage.message.errorMessage.errorType,
                                newCSLMessage.message.errorMessage.requestID,
                                newCSLMessage.message.errorMessage.hDevice);
                            result                                           = CamxResultSuccess;
                            break;
                        default:
                            CAMX_LOG_ERROR(CamxLogGroupCSL, "unexpected Event ID of type %d", pEvent->id);
                            result = CamxResultEInvalidArg;
                            break;
                    }
                    //  Propagating Error notification to Session Directly.
                    if (CamxResultSuccess == result)
                    {
                        pSession->sessionMessageHandler(pSession->pSessionMessageData, &newCSLMessage);
                    }
                }
            }
        }
        return result;
    }

    if (CSLInvalidHandle == hSession)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid session handle");
        result = CamxResultEFailed;
    }
    else
    {
        UINT sessionIndex = CAM_REQ_MGR_GET_HDL_IDX(hSession);
        if (sessionIndex < CSLHwMaxNumSessions)
        {
            pSession = &g_CSLHwInstance.sessionList[sessionIndex];
            CAMX_ASSERT(hSession == pSession->hSession);

            if (pSession->CSLLinkHandleData[index].messageHandler)
            {
                switch(pEvent->id)
                {
                    case V4L_EVENT_CAM_REQ_MGR_SOF_BOOT_TS:
                        newCSLMessage.type                                  = CSLMessageTypeFrame;
                        newCSLMessage.message.frameMessage.requestID        = pMessage->u.frame_msg.request_id;
                        newCSLMessage.message.frameMessage.frameCount       = pMessage->u.frame_msg.frame_id;
                        newCSLMessage.message.frameMessage.timestamp        = pMessage->u.frame_msg.timestamp;
                        newCSLMessage.message.frameMessage.timestampType    = CSLTimestampTypeMono;
                        newCSLMessage.message.frameMessage.link_hdl         = pMessage->u.frame_msg.link_hdl;
                        newCSLMessage.message.frameMessage.bNotify          = TRUE;
                        CAMX_LOG_INFO(CamxLogGroupCSL, "EVENT_CAM_REQ_MGR_SOF_BOOT_TS Index %d requestid %llu framecount %llu",
                            index, newCSLMessage.message.frameMessage.requestID, pMessage->u.frame_msg.frame_id);
                        result                                              = CamxResultSuccess;
                        break;
                    case V4L_EVENT_CAM_REQ_MGR_SOF:
                        newCSLMessage.type                               = CSLMessageTypeFrame;
                        newCSLMessage.message.frameMessage.requestID     = pMessage->u.frame_msg.request_id;
                        newCSLMessage.message.frameMessage.frameCount    = pMessage->u.frame_msg.frame_id;
                        newCSLMessage.message.frameMessage.timestamp     = pMessage->u.frame_msg.timestamp;
                        newCSLMessage.message.frameMessage.link_hdl      = pMessage->u.frame_msg.link_hdl;
                        newCSLMessage.message.frameMessage.timestampType = CSLTimestampTypeQtimer;
                        newCSLMessage.message.frameMessage.bNotify       = FALSE;
                        CAMX_LOG_INFO(CamxLogGroupCSL, "V4L_EVENT_CAM_REQ_MGR_SOF Index %d request id %llu frame count %llu",
                            index, newCSLMessage.message.frameMessage.requestID, pMessage->u.frame_msg.frame_id);
                        result                                          = CamxResultSuccess;
                        break;
                    case V4L_EVENT_CAM_REQ_MGR_ERROR:
                        newCSLMessage.type                               = CSLMessageTypeError;
                        newCSLMessage.message.errorMessage.errorType     =
                            static_cast<CSLErrorMessageCode>(pMessage->u.err_msg.error_type);
                        newCSLMessage.message.errorMessage.requestID     = pMessage->u.err_msg.request_id;
                        newCSLMessage.message.errorMessage.hDevice       = pMessage->u.err_msg.device_hdl;
                        newCSLMessage.message.errorMessage.resourceIndex = pMessage->u.err_msg.resource_size;
                        result                                           = CamxResultSuccess;
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "frame error: type %d, requestID %llu, device hdl %d",
                                       newCSLMessage.message.errorMessage.errorType,
                                       newCSLMessage.message.errorMessage.requestID,
                                       newCSLMessage.message.errorMessage.hDevice);
                        break;
                    default:
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "unexpected Event ID");
                        result = CamxResultEInvalidArg;
                        break;
                }
                CAMX_ASSERT(CamxResultSuccess == result);
                pSession->CSLLinkHandleData[index].messageHandler(pSession->CSLLinkHandleData[index].pMessageData,
                    &newCSLMessage);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "messageHandler is NULL");
                result = CamxResultEFailed;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultEOutOfBounds: sessionIndex = %d", sessionIndex);
            result = CamxResultEOutOfBounds;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInstancePollThreadFunc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static VOID* CSLHwInstancePollThreadFunc(
    VOID* pData)
{
    CAMX_UNREFERENCED_PARAM(pData);

    INT32             pollStatus      = -1;
    INT32             exit_thread = 1;
    CamxResult        result;

    while (exit_thread)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering Polling with Num of fds = %d", g_CSLHwInstance.pollNumFds);
        pollStatus = poll(g_CSLHwInstance.pollFds, g_CSLHwInstance.pollNumFds, -1);
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "After Polling with status=%d num fd=%d", pollStatus, g_CSLHwInstance.pollNumFds);
        if (0 < pollStatus)
        {
            if ((POLLIN == (g_CSLHwInstance.pollFds[0].revents & POLLIN)) &&
                (POLLRDNORM == (g_CSLHwInstance.pollFds[0].revents & POLLRDNORM)))
            {
                ssize_t                  readLength;
                CSLInternalHwPipeMessage data;

                readLength = read(g_CSLHwInstance.pipeFd[0], &data, sizeof(CSLInternalHwPipeMessage));
                if (sizeof(CSLInternalHwPipeMessage) != readLength)
                {
                    CHAR errnoStr[100] = {0};
                    CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "Received a invalid data in Pipe len = %d with error reason %s",
                                   readLength, errnoStr);
                }
                else
                {
                    switch (data.type)
                    {
                        case CSLHwPipeMessageExitThread:
                            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Poll thread exit received");
                            exit_thread = 0;
                            break;
                        case CSLHwPipeMessageAddFd:
                            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Poll thread received Add new fd=%d numfds=%d",
                                             data.fd, g_CSLHwInstance.pollNumFds);
                            g_CSLHwInstance.pollLock->Lock();
                            if (CSLHwMaxKMDNumDevices <= g_CSLHwInstance.pollNumFds)
                            {
                                CAMX_ASSERT_ALWAYS();
                            }
                            else
                            {
                                g_CSLHwInstance.pollFds[g_CSLHwInstance.pollNumFds].fd     = data.fd;
                                g_CSLHwInstance.pollFds[g_CSLHwInstance.pollNumFds].events = POLLIN|POLLRDNORM|POLLPRI;
                                g_CSLHwInstance.pollNumFds++;
                            }
                            g_CSLHwInstance.pollLock->Unlock();
                            break;
                        case CSLHwPipeMessageDeleteFd:
                            g_CSLHwInstance.pollLock->Lock();
                            g_CSLHwInstance.pollNumFds--;
                            // For now assuming adding and removal of FD are always sequential if anything else assert for now
                            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Poll thread received Delete fd=%d matchingfd=%d",
                                             data.fd,
                                             g_CSLHwInstance.pollFds[g_CSLHwInstance.pollNumFds].fd);
                            if ((CSLHwMaxKMDNumDevices <= g_CSLHwInstance.pollNumFds) ||
                                (g_CSLHwInstance.pollFds[g_CSLHwInstance.pollNumFds].fd != data.fd))
                            {
                                CAMX_ASSERT_ALWAYS_MESSAGE("Assert here");
                            }
                            else
                            {
                                g_CSLHwInstance.pollFds[g_CSLHwInstance.pollNumFds].fd     = -1;
                                g_CSLHwInstance.pollFds[g_CSLHwInstance.pollNumFds].events = 0;
                            }
                            g_CSLHwInstance.pollLock->Unlock();
                            break;
                        default:
                            break;
                    }
                }
            }
            if (0 < pollStatus)
            {
                UINT  i;
                for (i = 1; i < g_CSLHwInstance.pollNumFds; i++)
                {
                    if ((POLLPRI == (g_CSLHwInstance.pollFds[i].revents & POLLPRI)) ||
                        (POLLIN == (g_CSLHwInstance.pollFds[i].revents & POLLIN)) ||
                        (POLLRDNORM == (g_CSLHwInstance.pollFds[i].revents & POLLRDNORM)))
                    {
                        struct v4l2_event* pV4L2_event =
                            static_cast<struct v4l2_event *>(CAMX_CALLOC(sizeof(struct v4l2_event)));
                        CAMX_ASSERT(NULL != pV4L2_event);

                        if (NULL != pV4L2_event)
                        {
                            do
                            {
                                result = CSLHwInternalDQEvent(g_CSLHwInstance.pollFds[i].fd, pV4L2_event);

                                if (CamxResultSuccess != result)
                                {
                                    CAMX_LOG_VERBOSE(CamxLogGroupCSL,
                                                     "End of VIDIOC_DQEVENT for fd=%d",
                                                     g_CSLHwInstance.pollFds[i].fd);
                                    break;
                                }
                                else
                                {
                                    if (V4L_EVENT_CAM_REQ_MGR_EVENT == pV4L2_event->type)
                                    {
                                        result = CSLHwInternalSendRequestManagerEvent(pV4L2_event);
                                        if (CamxResultSuccess != result)
                                        {
                                            // CAMX_ASSERT_ALWAYS_MESSAGE("Failed to send request manager event");
                                        }
                                    }
                                }
                            } while (CamxResultSuccess == result);

                            CAMX_FREE(pV4L2_event);
                            pV4L2_event = NULL;
                        }
                    }
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "In the else case");
        }
    }
    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "CSL HWInstance poll thread exiting");
    return NULL;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalGetKMDDeviceQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult CSLHwInternalGetKMDDeviceQueryCap(
    CSLHwDevice* pKMDhw)
{
    CamxResult result = CamxResultEFailed;

    if (NULL != pKMDhw->deviceOp.KMDQueryCap)
    {
        switch (pKMDhw->deviceType)
        {
            case CSLHwImageSensor:
            case CSLHwLensActuator:
            case CSLHwCompanion:
            case CSLHwEEPROM:
            case CSLHwCSIPHY:
            case CSLHwOIS:
            case CSLHwFlash:
            case CSLHwFD:
            case CSLHwJPEGE:
            case CSLHwJPEGD:
            case CSLHwVFE:
            case CSLHwCPP:
            case CSLHwCSID:
            case CSLHwISPIF:
            case CSLHwIFE:
            case CSLHwICP:
            case CSLHwLRME:
            case CSLHwCPAS_TOP:
            case CSLHwCustom:
                result = pKMDhw->deviceOp.KMDQueryCap(pKMDhw->deviceIndex);
                break;
            case CSLHwMAXDEVICE:
            default:
                break;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalAssignDeviceOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult CSLHwInternalAssignDeviceOps(
    CSLHwDevice* pKMDhw)
{
    CamxResult result = CamxResultEFailed;

    switch (pKMDhw->deviceType)
    {
        case CSLHwFD:
            pKMDhw->deviceOp  = g_CSLHwDeviceFDOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwVFE:
            pKMDhw->deviceOp  = g_CSLHwDeviceVFEOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwIFE:
            pKMDhw->deviceOp  = g_CSLHwDeviceIFEOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwJPEGE:
            pKMDhw->deviceOp = g_CSLHwDeviceJPEGOps;
            result = CamxResultSuccess;
            break;
        case CSLHwLRME:
            pKMDhw->deviceOp  = g_CSLHwDeviceLRMEOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwImageSensor:
            pKMDhw->deviceOp  = g_CSLHwDeviceSensorOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwCSIPHY:
            pKMDhw->deviceOp  = g_CSLHwDeviceCsiPhyOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwICP:
            pKMDhw->deviceOp  = g_CSLHwDeviceICPOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwFlash:
            pKMDhw->deviceOp  = g_CSLHwDeviceFlashOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwLensActuator:
            pKMDhw->deviceOp  = g_CSLHwDeviceActuatorOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwEEPROM:
            pKMDhw->deviceOp  = g_CSLHwDeviceEepromOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwOIS:
            pKMDhw->deviceOp  = g_CSLHwDeviceOisOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwCompanion:
        case CSLHwJPEGD:
        case CSLHwCPP:
        case CSLHwCSID:
        case CSLHwISPIF:
        case CSLHwRequestManager:
            pKMDhw->deviceOp = g_CSLHwDeviceDefaultOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwCPAS_TOP:
            pKMDhw->deviceOp = g_CSLHwDeviceCPASOps;
            result           = CamxResultSuccess;
            break;
        case CSLHwCustom:
            pKMDhw->deviceOp  = g_CSLHwDeviceCustomHWOps;
            result            = CamxResultSuccess;
            break;
        case CSLHwMAXDEVICE:
        default:
            break;
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwAddKMDPrivateDeviceToInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL CSLHwAddKMDPrivateDeviceToInstance(
    CHAR* pDeviceName, UINT32 groupId)
{
    INT        fd         = -1;
    INT        status     = -1;
    BOOL       returnCode = FALSE;
    INT        openStatus = EBUSY_CODE;
    INT        pollCount  = 1;
    CamxResult result     = CamxResultEFailed;

    // If previous close operation is still in progress, try open operation for CSLHwMaxDevOpenPolls times until
    // open is successful
    while ((EBUSY_CODE == openStatus) && (pollCount <= CSLHwMaxDevOpenPolls))
    {
        fd = CSLHwInternalDefaultOpen(pDeviceName, O_RDWR | O_NONBLOCK);
        if (fd < 0)
        {
            CHAR errnoStr[100] = {0};
            CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
            openStatus = errno;

            if (EBUSY_CODE == openStatus)
            {
                CAMX_LOG_INFO(CamxLogGroupCSL, "Open failed for CSL Private device %s with group id=%d, errno: %d, "
                    "error reason %s, open poll count: %d, max poll: %d. Retry open again after %d usec....",
                    pDeviceName, groupId, errno, errnoStr, pollCount, CSLHwMaxDevOpenPolls, CSLHwMaxSleepTime);
                usleep(CSLHwMaxSleepTime);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Open failed for CSL Private device %s with group id=%d, errno: %d, "
                    "error reason %s, open poll count: %d, max poll: %d",
                    pDeviceName, groupId, errno, errnoStr, pollCount, CSLHwMaxDevOpenPolls);
                break;
            }
        }
        else
        {
            result = CamxResultSuccess;
            break;
        }
        pollCount++;
    }

    if (CamxResultSuccess == result)
    {
        switch (groupId)
        {
            case CAM_VNODE_DEVICE_TYPE:
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "FD = %d for device = %s", fd, pDeviceName);
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Open Success for Camera Device Manager after %d device poll attempts",
                    pollCount);
                // Init any data structures needed here
                g_CSLHwInstance.lock = CamX::Mutex::Create("CSLInstance");
                g_CSLHwInstance.allocLock = CamX::Mutex::Create("CSLAlloc");
                g_CSLHwInstance.lock->Lock();
                g_CSLHwInstance.numSessions         = 0;
                g_CSLHwInstance.destroyCondition    = CamX::Condition::Create("CSLInstance");
                g_CSLHwInstance.acquiredPID         = CamX::OsUtils::GetProcessID();
                g_CSLHwInstance.acquiredTID         = CamX::OsUtils::GetThreadID();
                g_CSLHwInstance.refcount            = 0;
                g_CSLHwInstance.requestManager.lock = CamX::Mutex::Create("CSLReqManager");
                g_CSLHwInstance.requestManager.lock->Lock();
                g_CSLHwInstance.requestManager.deviceIndex = -1;
                g_CSLHwInstance.requestManager.deviceType  = CSLHwRequestManager;
                g_CSLHwInstance.requestManager.mode        = CSLHwDeviceSingleton;
                g_CSLHwInstance.requestManager.fd          = fd;
                g_CSLHwInstance.requestManager.hMapIOMMU   = {-1, -1};
                g_CSLHwInstance.requestManager.deviceOrder = -1;
                g_CSLHwInstance.requestManager.kmdGroupId  = groupId;
                CSLHwKmdGroupidToCslDevicetype(groupId, &g_CSLHwInstance.requestManager.deviceType);
                CSLHwInternalAssignDeviceOps(&g_CSLHwInstance.requestManager);
                g_CSLHwInstance.kmdDeviceCount        = 0;
                g_CSLHwInstance.sensorSlotDeviceCount = 0;
                CamX::Utils::Memset(&g_CSLHwInstance.sessionList, 0, (sizeof(g_CSLHwInstance.sessionList)));
                CamX::Utils::Memcpy(&g_CSLHwInstance.requestManager.devName, pDeviceName,
                                    sizeof(g_CSLHwInstance.requestManager.devName));
                CAMX_LOG_INFO(CamxLogGroupCSL, "CSL Device name =%s", pDeviceName);
                status = pipe(g_CSLHwInstance.pipeFd);
                if (0 > status)
                {
                    CHAR errnoStr[100] = {0};
                    CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
                    CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create pipe with error reason %s", errnoStr);
                }

                result = CSLHwInternalDefaultSubscribeEvents(&g_CSLHwInstance.requestManager,
                                                             V4L_EVENT_CAM_REQ_MGR_SOF,
                                                             V4L_EVENT_CAM_REQ_MGR_EVENT);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "SOF event subscription failed");
                }
                else
                {
                    result = CSLHwInternalDefaultSubscribeEvents(&g_CSLHwInstance.requestManager,
                                                                 V4L_EVENT_CAM_REQ_MGR_SOF_BOOT_TS,
                                                                 V4L_EVENT_CAM_REQ_MGR_EVENT);
                    CAMX_LOG_INFO(CamxLogGroupCSL, "SOF Boot TS result: %d", result);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "SOF boot TS subscription failed");
                    }


                    result = CSLHwInternalDefaultSubscribeEvents(&g_CSLHwInstance.requestManager,
                                                                 V4L_EVENT_CAM_REQ_MGR_ERROR,
                                                                 V4L_EVENT_CAM_REQ_MGR_EVENT);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "Error event subscription failed");
                    }
                }

                g_CSLHwInstance.pollLock = CamX::Mutex::Create("CSLPoll");
                g_CSLHwInstance.pollLock->Lock();
                // Dont think we need condition on poll
                g_CSLHwInstance.pollFds[0].fd     = g_CSLHwInstance.pipeFd[0];
                g_CSLHwInstance.pollFds[0].events = POLLIN|POLLRDNORM;
                g_CSLHwInstance.pollNumFds        = 1;
                g_CSLHwInstance.pollFds[1].fd     = fd;
                g_CSLHwInstance.pollFds[1].events = POLLIN|POLLRDNORM|POLLPRI;
                g_CSLHwInstance.pollNumFds        = 2;
                g_CSLHwInstance.pollLock->Unlock();
                // Create a poll thread with bridge
                result = CamX::OsUtils::ThreadCreate(CSLHwInstancePollThreadFunc,
                                                     NULL, &g_CSLHwInstance.pollThreadHandle);
                if (CamxResultSuccess != result)
                {
                    CHAR errnoStr[100] = {0};
                    CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
                    CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create poll thread with error reason %s", errnoStr);
                }
                // Subscribe for Cam Req Mngr events
                CamX::CamxAtomicStore32(&g_CSLHwInstance.requestManager.aState, CSLHwValidState);
                g_CSLHwInstance.requestManager.lock->Unlock();
                g_CSLHwInstance.lock->Unlock();
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "CSL is in Valid state");
                if (CamxResultSuccess == result)
                {
                    returnCode = TRUE;
                }
                break;
            case CAM_CPAS_DEVICE_TYPE:
                g_CSLHwInstance.lock->Lock();
                g_CSLHwInstance.CPASDevice.lock = CamX::Mutex::Create("CSLCPAS");
                g_CSLHwInstance.CPASDevice.lock->Lock();
                CamX::Utils::Memcpy(&g_CSLHwInstance.CPASDevice.devName,
                                    pDeviceName,
                                    sizeof(g_CSLHwInstance.CPASDevice.devName));
                g_CSLHwInstance.CPASDevice.fd = fd;
                CAMX_LOG_INFO(CamxLogGroupCSL, "CSL Device name =%s", pDeviceName);
                CSLHwAddFdToPoll(fd);
                g_CSLHwInstance.CPASDevice.deviceIndex = -1;
                g_CSLHwInstance.CPASDevice.deviceOrder = -1;
                CamX::CamxAtomicStore32(&g_CSLHwInstance.CPASDevice.aState, CSLHwValidState);
                g_CSLHwInstance.CPASDevice.mode        = CSLHwDeviceSingleton;
                g_CSLHwInstance.CPASDevice.hMapIOMMU   = {-1, -1};
                g_CSLHwInstance.CPASDevice.kmdGroupId  = groupId;
                CSLHwKmdGroupidToCslDevicetype(groupId, &g_CSLHwInstance.CPASDevice.deviceType);
                CSLHwInternalAssignDeviceOps(&g_CSLHwInstance.CPASDevice);
                // Now trigger the device Query cap and book keep all the infromation in CSL
                CSLHwInternalGetKMDDeviceQueryCap(&g_CSLHwInstance.CPASDevice);
                g_CSLHwInstance.CPASDevice.lock->Unlock();
                g_CSLHwInstance.lock->Unlock();
                returnCode = TRUE;
                break;
            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Private device %s with group id=%d", pDeviceName, groupId);
                break;
        }
    }
    else if (EBUSY_CODE == openStatus)
    {
        CHAR errnoStr[100] = {0};
        CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Open failed for CSL Private device %s with group id=%d, errno: %d, "
            "error reason %s, Exhausted maximum(%d) number of device open attempts",
            pDeviceName, groupId, errno, errnoStr, CSLHwMaxDevOpenPolls);
    }

    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwRemoveKMDPrivateDeviceFromInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwRemoveKMDPrivateDeviceFromInstance(
    UINT32 groupId)
{
    switch (groupId)
    {
        case CAM_VNODE_DEVICE_TYPE:
            if (CSLHwValidState == g_CSLHwInstance.requestManager.aState)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Remove for Camera Device Manager");
                g_CSLHwInstance.lock->Lock();
                if (g_CSLHwInstance.refcount != 0)
                {
                    CAMX_ASSERT_ALWAYS();
                }
                g_CSLHwInstance.requestManager.lock->Lock();
                CSLHwRemoveFromPoll(g_CSLHwInstance.requestManager.fd);
                close(g_CSLHwInstance.requestManager.fd);
                g_CSLHwInstance.requestManager.deviceIndex = -1;
                CSLHwSendExitToPoll();
                CamX::OsUtils::ThreadWait(g_CSLHwInstance.pollThreadHandle);
                g_CSLHwInstance.pollLock->Lock();
                close(g_CSLHwInstance.pipeFd[0]);
                close(g_CSLHwInstance.pipeFd[1]);
                g_CSLHwInstance.pollLock->Unlock();
                g_CSLHwInstance.pollLock->Destroy();
                g_CSLHwInstance.requestManager.lock->Unlock();
                g_CSLHwInstance.requestManager.lock->Destroy();
                g_CSLHwInstance.lock->Unlock();
                g_CSLHwInstance.lock->Destroy();
                g_CSLHwInstance.allocLock->Unlock();
                g_CSLHwInstance.allocLock->Destroy();
                g_CSLHwInstance.destroyCondition->Destroy();
                CamX::Utils::Memset(&g_CSLHwInstance, 0, (sizeof(g_CSLHwInstance)));
                CamX::Utils::Memset(&g_CSLHwInstance.pollFds, 0, (sizeof(g_CSLHwInstance.pollFds)));
            }
            break;
        case CAM_CPAS_DEVICE_TYPE:
            if (CSLHwValidState == g_CSLHwInstance.CPASDevice.aState)
            {
                g_CSLHwInstance.CPASDevice.lock->Lock();
                CSLHwRemoveFromPoll(g_CSLHwInstance.CPASDevice.fd);
                close(g_CSLHwInstance.CPASDevice.fd);
                g_CSLHwInstance.CPASDevice.deviceIndex = -1;
                g_CSLHwInstance.CPASDevice.lock->Unlock();
                g_CSLHwInstance.CPASDevice.lock->Destroy();

                if (NULL != g_CSLHwInstance.CPASDevice.pKMDDeviceData)
                {
                    CAMX_FREE(g_CSLHwInstance.CPASDevice.pKMDDeviceData);
                    g_CSLHwInstance.CPASDevice.pKMDDeviceData = NULL;
                }
                CamX::Utils::Memset(&g_CSLHwInstance.CPASDevice, 0, (sizeof(g_CSLHwInstance.CPASDevice)));
            }
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Private device with group id=%d", groupId);
            break;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwAddKMDDeviceToInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwAddKMDDeviceToInstance(
    CHAR*  pDeviceName,
    UINT32 groupId,
    INT32* pDeviceIndex,
    INT    deviceFd)
{
    CSLHwInternalDeviceType type       = CSLHwInvalidDevice;
    INT                     fd         = -1;
    BOOL                    returnCode = FALSE;

    if (NULL != pDeviceName)
    {
        CamxResult result = CamxResultEFailed;

        // In case of successfully probed sensors, fd would already be known and opened.
        // Therefore, already opened fd should be used.
        // In all other cases, fd is unknown and needs to be opened.
        if (-1 == deviceFd)
        {
            fd = CSLHwInternalDefaultOpen(pDeviceName, O_RDWR | O_NONBLOCK);
        }
        else
        {
            fd = deviceFd;
        }

        if (fd >= 0)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, " Input Args DevName= %s groupId %d DeviceCount=%d",
               pDeviceName, groupId, g_CSLHwInstance.kmdDeviceCount);

            result = CSLHwKmdGroupidToCslDevicetype(groupId, &type);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupCSL, "Ignoring these devices groupId=%d\n", groupId);
                CSLHwInternalDefaultClose(fd);
                returnCode = TRUE;
            }
            else
            {
                CSLHwDevice* pLoophw;

                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Valid CSL devices and type =%d", type);
                g_CSLHwInstance.lock->Lock();
                pLoophw = &g_CSLHwInstance.CSLInternalKMDDevices[g_CSLHwInstance.kmdDeviceCount];
                pLoophw->lock = CamX::Mutex::Create("KMDLoop");
                pLoophw->destroyCondition = CamX::Condition::Create("KMDLoop Destroy");
                pLoophw->lock->Lock();
                pLoophw->deviceIndex = g_CSLHwInstance.kmdDeviceCount;
                pLoophw->deviceType  = type;
                CamX::OsUtils::SNPrintF(pLoophw->devName, sizeof(pLoophw->devName), "%s", pDeviceName);
                CAMX_LOG_INFO(CamxLogGroupCSL, "CSL Device name =%s", pDeviceName);
                pLoophw->deviceType = type;
                result = CSLHwInternalAssignDeviceOps(pLoophw);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_WARN(CamxLogGroupCSL, "Assigning ops failed for groupId=%d", groupId);
                    pLoophw->deviceOp = g_CSLHwDeviceDefaultOps;
                }
                pLoophw->fd         = fd;
                pLoophw->kmdGroupId = groupId;
                CSLHwAddFdToPoll(fd);
                // Now trigger the device Query cap and book keep all the infromation in CSL
                // Update the IOMMU handles of the devices here
                result = CSLHwInternalGetKMDDeviceQueryCap(pLoophw);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_WARN(CamxLogGroupCSL, "KMD querycaps failed for groupId=%d", groupId);
                    // Some device like LRME has not implemented KMD querycaps so Ok for Now.
                    // CAMX_ASSERT_ALWAYS();
                }
                CamX::Utils::Memset(&pLoophw->hAcquired, 0, (sizeof(pLoophw->hAcquired)));
                pLoophw->mode = CSLHwDeviceSingleton;
                result = CSLHwKMDDeviceSetState(pLoophw, CSLHwValidState);
                if (CamxResultSuccess != result)
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Setting device state failed for groupId=%d", groupId);
                }
                pLoophw->deviceOrder = -1;
                pLoophw->refCount = 0;
                pLoophw->lock->Unlock();
                *pDeviceIndex = pLoophw->deviceIndex;
                g_CSLHwInstance.kmdDeviceCount++;
                g_CSLHwInstance.lock->Unlock();
                returnCode = TRUE;
            }
        }
        else
        {
            CHAR errnoStr[100] = {0};
            CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Open failed for CSL KMD device %s with group id=%d with error reason %s",
                           pDeviceName,
                           groupId,
                           errnoStr);
            returnCode = FALSE;
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Invalid args or state %p %d", pDeviceName, groupId);
    }

    return returnCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwAddSensorSlotDeviceToInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL CSLHwAddSensorSlotDeviceToInstance(
    CHAR*  pDeviceName,
    UINT32 groupId,
    INT32* pDeviceIndex)
{
    CAMX_UNREFERENCED_PARAM(pDeviceIndex);

    CSLHwInternalDeviceType type;
    INT                     fd         = -1;
    BOOL                    returnCode = FALSE;

    if (NULL != pDeviceName)
    {
        CamxResult result = CamxResultEFailed;

        fd = CSLHwInternalDefaultOpen(pDeviceName, O_RDWR | O_NONBLOCK);
        if (fd >= 0)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Input Args DevName= %s groupId %d DeviceCount=%d",
                             pDeviceName, groupId, g_CSLHwInstance.sensorSlotDeviceCount);
            result = CSLHwKmdGroupidToCslDevicetype(groupId, &type);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupCSL, "Ignoring these devices groupId=%d", groupId);
                CSLHwInternalDefaultClose(fd);
                returnCode = TRUE;
            }
            else
            {
                CSLHwDevice* pLoophw;

                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Valid CSL devices and type =%d", type);
                g_CSLHwInstance.lock->Lock();
                pLoophw                   = &g_CSLHwInstance.CSLHwSensorSlotDevices[g_CSLHwInstance.sensorSlotDeviceCount];
                pLoophw->lock             = CamX::Mutex::Create("SensorLoop");
                pLoophw->destroyCondition = CamX::Condition::Create("SensorLoop Destroy");
                pLoophw->lock->Lock();
                pLoophw->deviceIndex = g_CSLHwInstance.sensorSlotDeviceCount;
                pLoophw->deviceType  = type;
                CamX::OsUtils::SNPrintF(pLoophw->devName, sizeof(pLoophw->devName), "%s", pDeviceName);
                CAMX_LOG_INFO(CamxLogGroupCSL, "CSL Device name =%s", pDeviceName);
                result = CSLHwInternalAssignDeviceOps(pLoophw);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_WARN(CamxLogGroupCSL, "Assigning ops failed for groupId=%d", groupId);
                    pLoophw->deviceOp = g_CSLHwDeviceDefaultOps;
                }
                pLoophw->fd         = fd;
                pLoophw->kmdGroupId = groupId;

                // Now trigger the device Query cap and book keep all the infromation in CSL
                // Update the IOMMU handles of the devices here
                result = CSLHwSensorSlotKmdQueryCapability(pLoophw->deviceIndex);
                if (CamxResultSuccess != result)
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("KMD querycaps failed for groupId=%d", groupId);
                }
                CamX::Utils::Memset(&pLoophw->hAcquired, 0, (sizeof(pLoophw->hAcquired)));
                pLoophw->mode = CSLHwDeviceSingleton;
                result        = CSLHwKMDDeviceSetState(pLoophw, CSLHwValidState);
                if (CamxResultSuccess != result)
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Setting device state failed for groupId=%d", groupId);
                }
                pLoophw->deviceOrder = -1;
                pLoophw->refCount    = 0;
                pLoophw->lock->Unlock();
                g_CSLHwInstance.sensorSlotDeviceCount++;
                g_CSLHwInstance.lock->Unlock();
                returnCode = TRUE;
            }
        }
        else
        {
            CHAR errnoStr[100] = {0};
            CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Open failed for CSL KMD device %s with group id=%d with error reason %s",
                           pDeviceName, groupId, errnoStr);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid args or state %p %d", pDeviceName, groupId);
    }
    return returnCode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwRemoveALLKMDDevicesFromInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwRemoveALLKMDDevicesFromInstance()
{
    CSLHwDevice* pLoophw;

    g_CSLHwInstance.lock->Lock();
    while (g_CSLHwInstance.kmdDeviceCount)
    {
        g_CSLHwInstance.kmdDeviceCount--;
        pLoophw = &g_CSLHwInstance.CSLInternalKMDDevices[g_CSLHwInstance.kmdDeviceCount];
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Removing KMD device groupId =%d id=%d",
                         pLoophw->kmdGroupId, g_CSLHwInstance.kmdDeviceCount);
        CamX::CamxAtomicStore32(&pLoophw->aState, CSLHwInvalidState);
        pLoophw->lock->Lock();
        CSLHwRemoveFromPoll(pLoophw->fd);
        // Now check all acquired are releases here if not release now and for now i am asserting
        if (0 != pLoophw->refCount)
        {
            CAMX_ASSERT_ALWAYS();
        }
        close(pLoophw->fd);
        pLoophw->fd = -1;
        // Makesure to release memory for UMD/KMD device data
        pLoophw->lock->Unlock();
        pLoophw->lock->Destroy();
        pLoophw->destroyCondition->Destroy();

        if (NULL != pLoophw->pKMDDeviceData)
        {
            CAMX_FREE(pLoophw->pKMDDeviceData);
            pLoophw->pKMDDeviceData = NULL;
        }

        CamX::Utils::Memset(pLoophw, 0, sizeof(CSLHwDevice));
    }
    g_CSLHwInstance.lock->Unlock();
    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Done removing all devices");

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwRemoveSensorSlotDeviceFromInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwRemoveSensorSlotDeviceFromInstance(
    CSLHwDevice* pHWDevice)
{
    g_CSLHwInstance.lock->Lock();
    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Removing Sensor slot KMD device groupId =%d id=%d",
                     pHWDevice->kmdGroupId, g_CSLHwInstance.sensorSlotDeviceCount);
    pHWDevice->lock->Destroy();
    pHWDevice->destroyCondition->Destroy();
    CamX::Utils::Memset(pHWDevice, 0, sizeof(CSLHwDevice));
    g_CSLHwInstance.lock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwEnumerateAndAddCSLHwDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSLHwEnumerateAndAddCSLHwDevice(
    CSLHwInternalHwEnumeration deviceType,
    UINT32                     deviceClass)
{
    struct media_device_info mediadevInfo;
    CHAR                     mediaName[CSLHwMaxDevName];
    INT32                    numMediaDevices = 0;
    INT32                    mediaFd         = 0;
    CamxResult               result          = 0;
    BOOL                     returnCode      = FALSE;
    INT32                    numEntities     = 0;
    INT                      deviceFd        = -1;

    while (1)
    {
        CamX::OsUtils::SNPrintF(mediaName, sizeof(mediaName), "/dev/media%d", numMediaDevices++);
        mediaFd = open(mediaName, O_RDWR | O_NONBLOCK);
        if (mediaFd < 0)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "No more media devices found on %s", mediaName);
            break;
        }
        result = CSLHwInternalFDIoctl(mediaFd, MEDIA_IOC_DEVICE_INFO, &mediadevInfo);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "media device get device info failed");
            close(mediaFd);
            continue;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "media devices found on %s and name = %s", mediaName, mediadevInfo.model);
        if (0 != (strncmp(mediadevInfo.model, CAM_REQ_MGR_VNODE_NAME, sizeof(mediadevInfo.model))))
        {
            close(mediaFd);
            continue;
        }
        numEntities = 1;
        while (1)
        {
            struct media_entity_desc entity = {};
            CHAR                     deviceName[CSLHwMaxDevName] = {0};
            CHAR                     subdeviceName[CSLHwMaxDevName] = {0};

            entity.id = numEntities;
            result = CSLHwInternalFDIoctl(mediaFd, MEDIA_IOC_ENUM_ENTITIES, &entity);
            if (result != CamxResultSuccess)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "media device enumerate entities failed for %s (id:%d)",
                                 entity.name, entity.id);
                break;
            }
            numEntities = entity.id | MEDIA_ENT_ID_FLAG_NEXT;

            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Found entity name:%s, type:%d, id:%d, device_class:%d deviceType:%d",
                             entity.name, entity.type, entity.id, deviceClass, deviceType);
            if (CSLInternalHwVideodevice == deviceType)
            {
                if (entity.type == deviceClass)
                {
                    CamX::OsUtils::SNPrintF(deviceName, sizeof(deviceName), "/dev/%s", entity.name);
                    returnCode = CSLHwAddKMDPrivateDeviceToInstance(deviceName, entity.type);
                    close(mediaFd);
                    return returnCode;
                }
            }
            else if (CSLInternalHwVideoSubdevice == deviceType)
            {
                if (entity.type == deviceClass)
                {
                    CamX::OsUtils::SNPrintF(subdeviceName, sizeof(subdeviceName), "/dev/%s", entity.name);
                    returnCode = CSLHwAddKMDPrivateDeviceToInstance(subdeviceName, entity.type);
                    close(mediaFd);
                    return returnCode;
                }
            }
            else if (CSLInternalHwVideoSubdeviceAll == deviceType)
            {
                CSLHwInternalDeviceType type;
                INT32                   deviceIndex = -1;

                CSLHwKmdGroupidToCslDevicetype(entity.type, &type);

                if ((FALSE == CSLHwIsKmdGroupidCslInternalDevicetype(entity.type)) &&
                    (CamxResultSuccess == CSLHwKmdGroupidToCslDevicetype(entity.type, &type)))
                {
                    CamX::OsUtils::SNPrintF(subdeviceName, sizeof(subdeviceName), "/dev/%s", entity.name);
                    if (CAM_SENSOR_DEVICE_TYPE == entity.type)
                    {
                        CSLHwAddSensorSlotDeviceToInstance(subdeviceName, entity.type, &deviceIndex);
                    }
                    else
                    {
                        // Non-sensor devices will be opened and then added to the KMD device list
                        // Therefore, fd is not known at this point of time and should be -1
                        CSLHwAddKMDDeviceToInstance(subdeviceName, entity.type, &deviceIndex, deviceFd);
                    }
                    returnCode = TRUE;
                }
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Invalid device type %d", deviceType);
                break;
            }
        }
        close(mediaFd);
        return returnCode;
    }
    return returnCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwRemoveHwDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSLHwRemoveHwDevice(
    CSLHwInternalHwEnumeration deviceType,
    UINT32                     deviceClass)
{
    CAMX_UNREFERENCED_PARAM(deviceClass);

    if (CSLInternalHwVideodevice == deviceType)
    {
        CSLHwRemoveKMDPrivateDeviceFromInstance(deviceClass);
    }
    else if (CSLInternalHwVideoSubdevice == deviceType)
    {
        CSLHwRemoveKMDPrivateDeviceFromInstance(deviceClass);
    }
    else if (CSLInternalHwVideoSubdeviceAll == deviceType)
    {
        CSLHwRemoveALLKMDDevicesFromInstance();
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Invalid device type %d", deviceType);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwLinkControl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwLinkControl(
    CSLHwsession* pSession,
    CSLLinkHandle hLink,
    UINT32        opMode)
{
    struct cam_control               cmd;
    struct cam_req_mgr_link_control  linkControl = {0};
    CamxResult                       result     = CamxResultEFailed;
    CSLHwDeviceOps*                  pDeviceOp  = &g_CSLHwInstance.requestManager.deviceOp;

    cmd.op_code     = CAM_REQ_MGR_LINK_CONTROL;
    cmd.size        = sizeof(linkControl);
    cmd.handle      = CamX::Utils::VoidPtrToUINT64(&linkControl);
    cmd.handle_type = CAM_HANDLE_USER_POINTER;
    cmd.reserved    = 0;

    linkControl.ops             = opMode;
    linkControl.session_hdl     = pSession->hSession;
    linkControl.link_hdls[0]    = hLink;
    linkControl.num_links       = 1;
    linkControl.reserved        = -1;

    result = pDeviceOp->Ioctl(&g_CSLHwInstance.requestManager, VIDIOC_CAM_CONTROL, &cmd);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Ioctl Failed with session = %d, result = %d", pSession->hSession, result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwStreamOnKMDHardwares
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwStreamOnKMDHardwares(
    CSLHandle                          Sessionindex,
    CamX::LightweightDoublyLinkedList* pList,
    CSLDeviceHandle*                   phDevices,
    CSLDeactivateMode                  mode)
{
    CSLHwsession*        pSession        = NULL;
    CSLHwDevice*         pHWDevice       = NULL;
    CSLHwAcquiredDevice* pAcquireddevice = NULL;
    CamxResult           result          = CamxResultSuccess;
    CamX::LDLLNode*      pNode           = pList->Head();

    UINT i_temp = 0;

    pSession = &g_CSLHwInstance.sessionList[Sessionindex];
    if (0 == pList->NumNodes())
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "No Hardware connected so returning Success");
    }

    // pList contains all of the devices for the CSL session
    while (NULL != pNode)
    {
        pAcquireddevice     = (static_cast<CSLHwAcquiredDevice*>(pNode->pData));
        pHWDevice           = &g_CSLHwInstance.CSLInternalKMDDevices[pAcquireddevice->cslDeviceIndex];
        BOOL turnOnDevice   = FALSE;

        if (NULL == phDevices)
        {
            turnOnDevice = TRUE;
        }
        else
        {
            // The session stores a list of ALL devices that have ever been turned on
            // need to find the specific devices that the camxpipeline has requested to turn on
            for (UINT i = 0; i < CamxMaxDeviceIndex; i++)
            {
                if (phDevices[i] == pAcquireddevice->hAcquired)
                {
                    CAMX_LOG_CONFIG(CamxLogGroupCSL,
                                     "Turning on DevH: 0x%x, hAc: 0x%x type: %d, idx: %d name:%s",
                                      phDevices[i],
                                      pAcquireddevice->hAcquired,
                                      pAcquireddevice->deviceType,
                                      pAcquireddevice->cslDeviceIndex,
                                      CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType]);
                    i_temp = i;
                    turnOnDevice = TRUE;
                    break;
                }
            }
        }

        if (TRUE == turnOnDevice)
        {
            if (NULL != pHWDevice->deviceOp.StreamOn)
            {
                CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCSL, "KMDStreamON: DevType%u", pAcquireddevice->deviceType);

                // Stream on a device
                result = pHWDevice->deviceOp.StreamOn(Sessionindex, pAcquireddevice->hAcquired,
                                                      pAcquireddevice->cslDeviceIndex, mode);

                CAMX_TRACE_SYNC_END(CamxLogGroupCSL);

                // If we failed to stream on the current device need to walk backwards up the list
                // of all devices we turned on and turn them off.
                if (CamxResultSuccess != result)
                {
                    pNode = CamX::LightweightDoublyLinkedList::PrevNode(pNode);

                    CAMX_LOG_ERROR(CamxLogGroupCSL,
                        "Stream On Failed hAc: 0x%x type: %d, idx: %d name:%s",
                        pAcquireddevice->hAcquired,
                        pAcquireddevice->deviceType,
                        pAcquireddevice->cslDeviceIndex,
                        CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType]);

                    // now make sure previous streamedON devices are streamedOFF
                    while (NULL != pNode)
                    {
                        pAcquireddevice     = (static_cast<CSLHwAcquiredDevice*>(pNode->pData));
                        pHWDevice->deviceOp.StreamOff(Sessionindex, pAcquireddevice->hAcquired,
                                                      pAcquireddevice->cslDeviceIndex, mode);
                        pNode = CamX::LightweightDoublyLinkedList::PrevNode(pNode);
                    }
                    break;
                }
                else
                {
                    pHWDevice->isActive = TRUE;
                }
            }
            else
            {
                result = CamxResultEFailed;
                pNode = CamX::LightweightDoublyLinkedList::PrevNode(pNode);
                // If we failed to stream on the current device need to walk backwards up the list
                // of all devices we turned on and turn them off.
                while (NULL != pNode)
                {
                    pAcquireddevice     = (static_cast<CSLHwAcquiredDevice*>(pNode->pData));
                    pHWDevice->deviceOp.StreamOff(Sessionindex, pAcquireddevice->hAcquired,
                                                    pAcquireddevice->cslDeviceIndex, mode);
                    pNode = CamX::LightweightDoublyLinkedList::PrevNode(pNode);
                }
                break;
                CAMX_ASSERT_ALWAYS();
            }
        }
        pNode = CamX::LightweightDoublyLinkedList::NextNode(pNode);
    }

    CAMX_LOG_INFO(CamxLogGroupCSL, "Stream on Done = %d", result);
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSingleDeviceStreamOnKMDHardwares
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSingleDeviceStreamOnKMDHardwares(
    CSLHandle                          hSessionindex,
    INT32                              deviceIndex,
    CSLDeviceHandle*                   phDevice,
    CSLDeactivateMode                  mode)
{
    CSLHwsession*        pSession        = NULL;
    CSLHwDevice*         pHWDevice       = NULL;
    CSLHwAcquiredDevice* pAcquireddevice = NULL;
    CamxResult           result          = CamxResultSuccess;

    pSession    = &g_CSLHwInstance.sessionList[hSessionindex];
    pHWDevice   = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];

    BOOL turnOnDevice   = FALSE;

    if (NULL != pHWDevice->deviceOp.StreamOn)
    {
        // Stream on a device
        result = pHWDevice->deviceOp.StreamOn(hSessionindex,
            *phDevice,
            deviceIndex,
            mode);
        // If we failed to stream on the current device need to walk backwards up the list
        // of all devices we turned on and turn them off.
        if (CamxResultSuccess != result)
        {
            CAMX_ASSERT_ALWAYS();
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_ASSERT_ALWAYS();
    }
    CAMX_LOG_INFO(CamxLogGroupCSL, "Stream on Done = %d", result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwStreamOffKMDHardwares
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwStreamOffKMDHardwares(
    CSLHandle                           Sessionindex,
    CamX::LightweightDoublyLinkedList*  pList,
    CSLDeviceHandle*                    phDevices,
    CSLDeactivateMode                   mode)
{
    CSLHwsession*        pSession           = NULL;
    CSLHwDevice*         pHWDevice          = NULL;
    CSLHwAcquiredDevice* pAcquireddevice    = NULL;
    CamxResult           result             = CamxResultSuccess;
    CamxResult           resultRetCode      = CamxResultSuccess;
    CamX::LDLLNode*      pNode              = pList->Head();

    UINT i_temp = 0;

    pSession = &g_CSLHwInstance.sessionList[Sessionindex];
    if (0 == pList->NumNodes())
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "No Hardware connected so returning Success");
    }

    while (NULL != pNode)
    {
        pAcquireddevice     = (static_cast<CSLHwAcquiredDevice*>(pNode->pData));
        pHWDevice           = &g_CSLHwInstance.CSLInternalKMDDevices[pAcquireddevice->cslDeviceIndex];
        BOOL turnOffDevice  = FALSE;
        if (NULL == phDevices)
        {
            turnOffDevice = TRUE;
        }
        else
        {
            // The session stores a list of ALL devices that have ever been turned on
            // need to find the specific devices that the camxpipeline has requested to turn on
            for (UINT i = 0; i < CamxMaxDeviceIndex; i++)
            {
                if ((phDevices[i]                    == pAcquireddevice->hAcquired) ||
                    ((pAcquireddevice->deviceType    == CSLHwCSIPHY) &&
                    (static_cast<UINT>(phDevices[i]) == pAcquireddevice->cslDeviceIndex)))
                {
                    CAMX_LOG_CONFIG(CamxLogGroupCSL,
                        "Turning off DevH: 0x%x, hAc: 0x%x type: %d, idx: %d, name = %s",
                        phDevices[i],
                        pAcquireddevice->hAcquired,
                        pAcquireddevice->deviceType,
                        pAcquireddevice->cslDeviceIndex,
                        CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType]);
                    i_temp = i;
                    turnOffDevice = TRUE;
                    break;
                }
            }
        }

        if ((NULL != pHWDevice->deviceOp.StreamOff) && (TRUE == turnOffDevice))
        {
            CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCSL, "KMDStreamOFF: DevType%u", pAcquireddevice->deviceType);
            result = pHWDevice->deviceOp.StreamOff(Sessionindex, pAcquireddevice->hAcquired,
                                                   pAcquireddevice->cslDeviceIndex, mode);
            CAMX_TRACE_SYNC_END(CamxLogGroupCSL);

            if (CamxResultSuccess != result)
            {
                resultRetCode = result;
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Stream off failed: hAc: 0x%x type: %d, idx: %d, name = %s, result = %d",
                    pAcquireddevice->hAcquired,
                    pAcquireddevice->deviceType,
                    pAcquireddevice->cslDeviceIndex,
                    CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType],
                    result);
            }
            else
            {
                pHWDevice->isActive = FALSE;
            }
        }
        pNode = CamX::LightweightDoublyLinkedList::NextNode(pNode);
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Stream off failed. mode %d result = %d", mode, result);
        resultRetCode = result;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Stream off mode %d result = %d", mode, resultRetCode);

    return resultRetCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSingleDeviceStreamOffKMDHardwares
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSingleDeviceStreamOffKMDHardwares(
    CSLHandle                          hSessionindex,
    INT32                              deviceIndex,
    CSLDeviceHandle*                   phDevice,
    CSLDeactivateMode                  mode)
{
    CSLHwsession*        pSession = NULL;
    CSLHwDevice*         pHWDevice = NULL;
    CSLHwAcquiredDevice* pAcquireddevice = NULL;
    CamxResult           result = CamxResultSuccess;

    pSession = &g_CSLHwInstance.sessionList[hSessionindex];
    pHWDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];

    if ((NULL != pHWDevice->deviceOp.StreamOff))
    {
        CAMX_LOG_INFO(CamxLogGroupCSL, "Stream off start");
        //   stream off a device
        result = pHWDevice->deviceOp.StreamOff(hSessionindex,
            *phDevice,
            deviceIndex,
            mode);
        CAMX_TRACE_SYNC_END(CamxLogGroupCSL);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Fail to stream off device, result = %d", result);
        }
    }

    CAMX_LOG_INFO(CamxLogGroupCSL, "Stream off Done = %d", result);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalCreateSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalCreateSession(
    CSLHandle* phCSL)
{
    struct cam_control              cmd;
    struct cam_req_mgr_session_info create_session;
    CamxResult                      result    = CamxResultEFailed;
    CSLHwDeviceOps*                 pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;
    CSLHwDevice                     hwDevice;

    cmd.op_code     = CAM_REQ_MGR_CREATE_SESSION;
    cmd.handle_type = CAM_HANDLE_USER_POINTER;
    cmd.size        = sizeof(create_session);
    cmd.handle      = CamX::Utils::VoidPtrToUINT64(&create_session);
    cmd.reserved    = 0;

    hwDevice = g_CSLHwInstance.requestManager;
    result = pDeviceOp->Ioctl(&hwDevice, VIDIOC_CAM_CONTROL, &cmd);
    if (CamxResultSuccess != result)
    {
        *phCSL = CSLInvalidHandle;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Ioctl Failed for create session result = %d", result);
    }
    else
    {
        if (0 > create_session.session_hdl)
        {
            struct cam_req_mgr_session_info destroy_session;

            cmd.op_code     = CAM_REQ_MGR_DESTROY_SESSION;
            cmd.size        = sizeof(destroy_session);
            cmd.handle      = CamX::Utils::VoidPtrToUINT64(&destroy_session);
            cmd.handle_type = CAM_HANDLE_USER_POINTER;
            cmd.reserved    = 0;

            destroy_session.session_hdl = create_session.session_hdl;
            g_CSLHwInstance.requestManager.deviceOp.Ioctl(
                &hwDevice, VIDIOC_CAM_CONTROL, &cmd);
            *phCSL = CSLInvalidHandle;
        }
        else
        {
            *phCSL = create_session.session_hdl;
        }
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalDestroySession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalDestroySession(
    CSLHandle hCSL)
{
    struct cam_control              cmd;
    struct cam_req_mgr_session_info destroy_session;
    CamxResult                      result    = CamxResultEFailed;
    CSLHwDeviceOps*                 pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;
    CSLHwDevice                     hwDevice;

    cmd.op_code     = CAM_REQ_MGR_DESTROY_SESSION;
    cmd.size        = sizeof(destroy_session);
    cmd.handle      = CamX::Utils::VoidPtrToUINT64(&destroy_session);
    cmd.handle_type = CAM_HANDLE_USER_POINTER;
    cmd.reserved    = 0;

    destroy_session.session_hdl = hCSL;

    hwDevice = g_CSLHwInstance.requestManager;

    result = pDeviceOp->Ioctl(&hwDevice, VIDIOC_CAM_CONTROL, &cmd);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Ioctl Failed for Destroy session = %d, result = %d",
                       destroy_session.session_hdl, result);
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwLinkKMDHardwares
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwLinkKMDHardwares(
    CSLHwsession*    pSession,
    CSLDeviceHandle* phDevices,
    UINT             handleCount,
    CSLLinkHandle*   phLink)
{
    struct cam_control           cmd;
    struct cam_req_mgr_link_info_v2 linkHw;
    CamxResult                   result    = CamxResultEFailed;
    CSLHwDeviceOps*              pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;
    CSLHwDevice                  hwDevice;

    cmd.op_code     = CAM_REQ_MGR_LINK_V2;
    cmd.size        = sizeof(linkHw);
    cmd.handle      = CamX::Utils::VoidPtrToUINT64(&linkHw);
    cmd.handle_type = CAM_HANDLE_USER_POINTER;
    cmd.reserved    = 0;

    linkHw.session_hdl = pSession->hSession;
    linkHw.num_devices = handleCount;
    CamX::Utils::Memcpy(&linkHw.dev_hdls, phDevices,
        (handleCount * sizeof(CSLDeviceHandle)));

    hwDevice = g_CSLHwInstance.requestManager;

    result = pDeviceOp->Ioctl(&hwDevice, VIDIOC_CAM_CONTROL, &cmd);
    if (CamxResultSuccess != result)
    {
        *phLink = CSLInvalidHandle;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Ioctl Failed with session = %d, result = %d", pSession->hSession, result);
    }
    else
    {
        *phLink = linkHw.link_hdl;
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwUnLinkKMDHardwares
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwUnLinkKMDHardwares(
    CSLHwsession* pSession,
    CSLLinkHandle hLink)
{
    struct cam_control             cmd;
    struct cam_req_mgr_unlink_info unlinkHw;
    CamxResult                     result    = CamxResultEFailed;
    CSLHwDeviceOps*                pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;
    CSLHwDevice                    hwDevice;

    cmd.op_code     = CAM_REQ_MGR_UNLINK;
    cmd.size        = sizeof(unlinkHw);
    cmd.handle      = CamX::Utils::VoidPtrToUINT64(&unlinkHw);
    cmd.handle_type = CAM_HANDLE_USER_POINTER;
    cmd.reserved    = 0;

    unlinkHw.session_hdl = pSession->hSession;
    unlinkHw.link_hdl    = hLink;

    hwDevice = g_CSLHwInstance.requestManager;

    result = pDeviceOp->Ioctl(&hwDevice, VIDIOC_CAM_CONTROL, &cmd);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Ioctl Failed with session = %d for link = %d", pSession->hSession, hLink);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSyncLinkKMDHardwares
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSyncLinkKMDHardwares(
    CSLHwsession*   pSession,
    CSLLinkHandle*  phLink,
    UINT            handleCount,
    CSLLinkHandle   hMasterLink,
    CSLSyncLinkMode syncMode)
{
    struct cam_control           cmd;
    struct cam_req_mgr_sync_mode syncLinks = {};
    CamxResult                   result    = CamxResultEFailed;
    CSLHwDeviceOps*              pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;
    CSLHwDevice                  hwDevice;

    cmd.op_code     = CAM_REQ_MGR_SYNC_MODE;
    cmd.size        = sizeof(syncLinks);
    cmd.handle      = CamX::Utils::VoidPtrToUINT64(&syncLinks);
    cmd.handle_type = CAM_HANDLE_USER_POINTER;
    cmd.reserved    = 0;

    syncLinks.session_hdl     = pSession->hSession;
    syncLinks.num_links       = handleCount;
    syncLinks.master_link_hdl = hMasterLink;
    syncLinks.sync_mode       = CSLHwGetKmdSyncTypeFromCSLSyncType(syncMode);
    CamX::Utils::Memcpy(&syncLinks.link_hdls, phLink, (handleCount * sizeof(CSLLinkHandle)));

    hwDevice = g_CSLHwInstance.requestManager;

    result = pDeviceOp->Ioctl(&hwDevice, VIDIOC_CAM_CONTROL, &cmd);
    if (CamxResultSuccess != result)
    {
        *phLink = CSLInvalidHandle;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Ioctl Failed with session = %d, result = %d", pSession->hSession, result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwScheduleRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwScheduleRequest(
    CSLHwsession*   pSession,
    CSLLinkHandle   hLink,
    UINT64          requestid,
    BOOL            bubble,
    CSLSyncLinkMode syncMode,
    UINT32          expectedExposureTime)
{
    CSLHwDevice                      hwDevice    = g_CSLHwInstance.requestManager;
    struct cam_control               cmd         = {};
    struct cam_req_mgr_sched_request openRequest = {};
    CamxResult                       result      = CamxResultEFailed;
    CSLHwDeviceOps*                  pDeviceOp   = &g_CSLHwInstance.requestManager.deviceOp;

    cmd.op_code     = CAM_REQ_MGR_SCHED_REQ;
    cmd.size        = sizeof(openRequest);
    cmd.handle      = CamX::Utils::VoidPtrToUINT64(&openRequest);
    cmd.handle_type = CAM_HANDLE_USER_POINTER;
    cmd.reserved    = 0;

    openRequest.req_id               = requestid;
    openRequest.sync_mode            = CSLHwGetKmdSyncTypeFromCSLSyncType(syncMode);
    openRequest.session_hdl          = pSession->hSession;
    openRequest.bubble_enable        = bubble;
    openRequest.link_hdl             = hLink;
    openRequest.additional_timeout   = expectedExposureTime;


    result = pDeviceOp->Ioctl(&hwDevice, VIDIOC_CAM_CONTROL, &cmd);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Ioctl Failed with session = %d, result = %d", pSession->hSession, result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwCancelRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwCancelRequest(
    CSLHwsession*       pSession,
    const CSLFlushInfo& rCSLFlushInfo)
{
    CamxResult result = CamxResultSuccess;
    struct cam_control       cmd;
    struct cam_flush_dev_cmd flushDeviceCmd;
    BOOL                     bFlushAllDevices = FALSE;
    UINT                     numDevices;

    if (NULL == pSession)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "pSession: %p is NULL Invalid Args!!",  pSession);
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) && (rCSLFlushInfo.numSyncLinks > 1))
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "rCSLFlushInfo.numSyncLinks: %d is greater than 1", rCSLFlushInfo.numSyncLinks);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        struct cam_req_mgr_flush_info flushSession;
        CSLHwDevice                   hwDevice;
        CSLHwDeviceOps*               pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;
        UINT16                        flushType = rCSLFlushInfo.flushType;

        // Set up Ioctl cmd for realtime devs
        cmd.op_code     = CAM_REQ_MGR_FLUSH_REQ;
        cmd.size        = sizeof(flushSession);
        cmd.handle      = CamX::Utils::VoidPtrToUINT64(&flushSession);
        cmd.handle_type = CAM_HANDLE_USER_POINTER;
        cmd.reserved    = 0;

        // Setup ioctl arg for real time dev
        flushSession.session_hdl = pSession->hSession;
        hwDevice                 = g_CSLHwInstance.requestManager;

        // Setup ioctl arg for nrt devs
        flushDeviceCmd.version        = 0;
        flushDeviceCmd.session_handle = pSession->hSession;
        flushDeviceCmd.reserved       = 0;

        // Setup for Flush on non realtime devices
        CSLHwAcquiredDevice* pAcquireddevice = NULL;
        BOOL                 bFlushAll       = FALSE;

        switch (flushType)
        {
            case CSLFlushAll:
                // Session Flush - flush all requests
                flushSession.flush_type   = CAM_REQ_MGR_FLUSH_TYPE_ALL;
                flushSession.req_id       = rCSLFlushInfo.lastCSLSyncId;
                flushDeviceCmd.flush_type = CAM_FLUSH_TYPE_ALL;
                flushDeviceCmd.req_id     = rCSLFlushInfo.lastRequestId;
                break;
            case CSLFlushRequest:
                // Request flush for all the requests
                flushSession.flush_type   = CAM_REQ_MGR_FLUSH_TYPE_CANCEL_REQ;
                flushSession.req_id       = rCSLFlushInfo.lastCSLSyncId;
                flushDeviceCmd.flush_type = CAM_FLUSH_TYPE_REQ;
                flushDeviceCmd.req_id     = rCSLFlushInfo.lastRequestId;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid Flush Type supplied");
                result = CamxResultEInvalidArg;
        }

        if (CamxResultSuccess == result)
        {
            // On this condition, flush all devices
            if ((NULL == rCSLFlushInfo.phDevices) &&
                (NULL == rCSLFlushInfo.phSyncLink))
            {
                bFlushAll        = TRUE;
                bFlushAllDevices = TRUE;
                numDevices       = 1;
                CAMX_LOG_INFO(CamxLogGroupCSL, "Flushing all requests. "
                              "rCSLFlushInfo.phDevices: %p, "
                              "rCSLFlushInfo.phSyncLinks: %p"
                              " with %s",
                              rCSLFlushInfo.phDevices,
                              rCSLFlushInfo.phSyncLink,
                              CamxFlushTypeStrings[flushType]);
            }
            else
            {
                numDevices = rCSLFlushInfo.numDevices;
            }

            if ((TRUE == bFlushAll) || (NULL != rCSLFlushInfo.phSyncLink))
            {
                // Flush the rt devices in the provided links
                for (UINT linkIndex = 0; linkIndex < CSLHwMaxLinksArray; linkIndex++)
                {
                    flushSession.link_hdl = pSession->linkInfo[linkIndex].hLinkIdentifier;

                    if ((TRUE  == bFlushAll) || ((FALSE == bFlushAll) &&
                        (flushSession.link_hdl == *(rCSLFlushInfo.phSyncLink))))
                    {
                        if ((CSLInvalidHandle != flushSession.link_hdl) &&
                            (TRUE == pSession->linkInfo[linkIndex].isActive))
                        {
                            CAMX_LOG_INFO(CamxLogGroupCSL, "link_hdl(%d), hCSLLinkHandle(%d) isActive getting flushed",
                                          flushSession.link_hdl, pSession->CSLLinkHandleData[linkIndex].hCSLLinkHandle);
                            result = pDeviceOp->Ioctl(&hwDevice, VIDIOC_CAM_CONTROL, &cmd);
                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCSL,
                                               "Ioctl Failed for cancel/flush request with session = %p , Flush Type = %s[%d]"
                                               " for CSL Sync Id = %llu , result = %s [%d]",
                                               pSession->hSession,
                                               CamxFlushTypeStrings[flushType],
                                               flushType,
                                               rCSLFlushInfo.lastCSLSyncId,
                                               CamxResultStrings[result],
                                               result);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    // If condition fails, do not flush non-realtime devices
    if ((CamxResultSuccess == result          ) &&
        ((TRUE             == bFlushAllDevices) ||
         (NULL             != rCSLFlushInfo.phDevices)))
    {

        CamX::LightweightDoublyLinkedList* pList           = &pSession->nrtList;
        CSLHwAcquiredDevice*               pAcquireddevice = NULL;
        CSLHwDevice*                       pHWDevice       = NULL;
        UINT16                             flushType       = rCSLFlushInfo.flushType;

        // update cmd for nrt devices
        cmd.op_code = CAM_FLUSH_REQ;
        cmd.size    = sizeof(flushDeviceCmd);
        cmd.handle  = CamX::Utils::VoidPtrToUINT64(&flushDeviceCmd);

        if (NULL == pList->Head())
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "No NRT Hardware connected so returning success");
        }
        else
        {
            for (UINT flushDeviceIndex = 0; flushDeviceIndex < numDevices; flushDeviceIndex++)
            {
                CamX::LDLLNode* pNode = pList->Head();

                // pList contains all of the devices for the CSL session
                while (NULL != pNode)
                {
                    pAcquireddevice = (static_cast<CSLHwAcquiredDevice*>(pNode->pData));
                    pHWDevice = &g_CSLHwInstance.CSLInternalKMDDevices[pAcquireddevice->cslDeviceIndex];

                    if ((TRUE == bFlushAllDevices) || ((FALSE == bFlushAllDevices) &&
                        (rCSLFlushInfo.phDevices[flushDeviceIndex] == pAcquireddevice->hAcquired)))
                    {

                        flushDeviceCmd.dev_handle = pAcquireddevice->hAcquired;

                        if ((NULL != pHWDevice->deviceOp.Ioctl) && (TRUE == pHWDevice->isActive))
                        {

                            result = pHWDevice->deviceOp.Ioctl(pHWDevice, VIDIOC_CAM_CONTROL, &cmd);
                            if (CamxResultSuccess != result)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCSL,
                                               "Ioctl Failed for cancel/flush request with session = %p "
                                               "device = %s[%p] Flush Type = %s[%d], for requestID = %llu "
                                               "result = %s[%d]",
                                    pSession->hSession,
                                    pAcquireddevice->nodeName,
                                    pAcquireddevice->hAcquired,
                                    CamxFlushTypeStrings[flushType],
                                    flushType,
                                    rCSLFlushInfo.lastRequestId,
                                    CamxResultStrings[result],
                                    result);
                            }
                            else
                            {

                                CAMX_LOG_VERBOSE(CamxLogGroupCSL,
                                               "Ioctl success! for cancel/flush request with session = %p "
                                               "device = %s[%p] Flush Type = %s[%d], for requestID = %llu "
                                               "result = %s[%d]",
                                               pSession->hSession,
                                               pAcquireddevice->nodeName,
                                               pAcquireddevice->hAcquired,
                                               CamxFlushTypeStrings[flushType],
                                               flushType,
                                               rCSLFlushInfo.lastRequestId,
                                               CamxResultStrings[result],
                                               result);
                            }
                        }
                    }
                    pNode = CamX::LightweightDoublyLinkedList::NextNode(pNode);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwGetSyncHwDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwGetSyncHwDevice(
    CHAR* pSyncDeviceName,
    INT32 deviceNameLen)
{
    struct media_device_info mediaDeviceInfo;
    CHAR                     mediaName[CSLHwMaxDevName];
    INT32                    numMediaDevices   = 0;
    INT32                    mediaFd           = 0;
    CamxResult               result            = CamxResultSuccess;
    INT32                    numEntities       = 0;

    while (1)
    {
        CamX::OsUtils::SNPrintF(mediaName, sizeof(mediaName), "/dev/media%d", numMediaDevices++);
        mediaFd = open(mediaName, O_RDWR | O_NONBLOCK);
        if (mediaFd < 0)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "No more media devices found on %s", mediaName);
            result = CamxResultEFailed;
            break;
        }
        result = CSLHwInternalFDIoctl(mediaFd, MEDIA_IOC_DEVICE_INFO, &mediaDeviceInfo);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "media device get device info failed");
            close(mediaFd);
            continue;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "media devices found on %s and name = %s", mediaName, mediaDeviceInfo.model);
        if (0 != (strncmp(mediaDeviceInfo.model, CAM_SYNC_DEVICE_NAME, sizeof(mediaDeviceInfo.model))))
        {
            close(mediaFd);
            continue;
        }
        numEntities = 1;
        while (1)
        {
            struct media_entity_desc entity                      = {};
            CHAR                     deviceName[CSLHwMaxDevName] = {};

            entity.id = numEntities;
            result = CSLHwInternalFDIoctl(mediaFd, MEDIA_IOC_ENUM_ENTITIES, &entity);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "media device enumerate entities failed");
                break;
            }
            numEntities = entity.id | MEDIA_ENT_ID_FLAG_NEXT;
            if (entity.type == CAM_SYNC_DEVICE_TYPE)
            {
                CamX::OsUtils::SNPrintF(pSyncDeviceName, deviceNameLen, "/dev/%s", entity.name);
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "deviceName=%s", deviceName);
                close(mediaFd);
                break;
            }
        }
        close(mediaFd);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalDefaultOpen
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT CSLHwInternalDefaultOpen(
    const CHAR* pDeviceName,
    INT         flags)
{
    INT fd = -1;

    if (NULL != pDeviceName)
    {
        fd = open(pDeviceName, flags);
        if (fd < 0)
        {
            CHAR errnoStr[100] = {0};
            CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Open failed for Device %s with error reason %s", pDeviceName, errnoStr);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupCSL, "Open Success for Device %s", pDeviceName);
        }
    }

    return fd;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalDefaultClose
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalDefaultClose(
    INT fd)
{
    INT        returnCode    = -1;
    CamxResult result = CamxResultEFailed;

    returnCode = close(fd);
    if (returnCode < 0)
    {
        CHAR errnoStr[100] = {0};
        CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Close failed for File Descriptor %d with error reason %s", fd, errnoStr);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Close Success for File Descriptor %d", fd);
        result = CamxResultSuccess;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalFDIoctl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalFDIoctl(
    INT           fd,
    // NOWHINE GR017: The ioctl type uses unsigned long
    unsigned long request,
    VOID*         pArg)
{
    INT        returnCode = -1;
    CamxResult result     = CamxResultEFailed;

    returnCode = ioctl(fd, request, pArg);
    if (returnCode < 0)
    {
        CHAR errnoStr[100] = {0};
        CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
        CAMX_LOG_WARN(CamxLogGroupCSL, "Ioctl failed for FD:%d with error reason %s", fd, errnoStr);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Ioctl succeeded for FD:%d", fd);
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalDefaultIoctl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalDefaultIoctl(
    const CSLHwDevice*  pDevice,
    // NOWHINE GR017: The ioctl type uses unsigned long
    unsigned long       request,
    VOID*               pArg)
{
    INT        returnCode = -1;
    CamxResult result     = CamxResultEFailed;

    returnCode = ioctl(pDevice->fd, request, pArg);
    if (returnCode < 0)
    {
        CHAR errnoStr[100] = {0};
        CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
        if (CSLHwImageSensor == pDevice->deviceType)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Ioctl failed for device %s (Type:%s, FD:%d, Index:%d) with error reason %s",
                           pDevice->devName, CSLHwInternalDeviceTypeStrings[pDevice->deviceType],
                           pDevice->fd, pDevice->deviceIndex, errnoStr);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Ioctl failed for device %s (Type:%s, FD:%d, Index:%d) with error reason %s",
               pDevice->devName, CSLHwInternalDeviceTypeStrings[pDevice->deviceType],
               pDevice->fd, pDevice->deviceIndex, errnoStr);
        }

        // Kernel cancel request map to CamxResultECancelledRequest
        if (EBADR == errno)
        {
            result = CamxResultECancelledRequest;
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Ioctl succeeded for device %s (Type:%s, FD:%d, Index:%d)",
                         pDevice->devName, CSLHwInternalDeviceTypeStrings[pDevice->deviceType],
                         pDevice->fd, pDevice->deviceIndex);
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalDefaultIoctl2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalDefaultIoctl2(
    const CSLHwDevice*  pDevice,
    UINT                opcode,
    VOID*               pArg,
    UINT32              hType,
    UINT32              size)
{
    /// @todo  (CAMX-1278): Can we just use Ioctl instead of having Ioctl2?
    CAMX_UNREFERENCED_PARAM(hType);

    INT                ioctlResult = -1;
    CamxResult         result      = CamxResultEFailed;
    struct cam_control ioctlCmd    = {};

    ioctlCmd.op_code = opcode;
    ioctlCmd.size    = size;
    ioctlCmd.handle  = CamX::Utils::VoidPtrToUINT64(pArg);

    ioctlResult = ioctl(pDevice->fd, VIDIOC_CAM_CONTROL, &ioctlCmd);
    if (ioctlResult < 0)
    {
        CHAR errnoStr[100] = {0};
        CamX::OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Ioctl returned %d, failed for device %s (Type:%s, FD:%d, Index:%d)"
                                        "with error reason %s",
                       ioctlResult, pDevice->devName, CSLHwInternalDeviceTypeStrings[pDevice->deviceType],
                       pDevice->fd, pDevice->deviceIndex, errnoStr);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Ioctl Success for Device %s (Type:%s, FD:%d, Index:%d)",
                         pDevice->devName, CSLHwInternalDeviceTypeStrings[pDevice->deviceType],
                         pDevice->fd, pDevice->deviceIndex);
        result = CamxResultSuccess;

        if (opcode == CAM_REQ_MGR_ALLOC_BUF)
        {
            INT32 handle = static_cast<struct cam_mem_mgr_alloc_cmd *>(pArg)->out.buf_handle;
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Got handle as: %x", handle);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalDefaultUMDQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalDefaultUMDQueryCap(
    INT32  deviceIndex,
    VOID*  pDeviceData,
    SIZE_T deviceDataSize)
{
    CSLHwDevice* pDevice       = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLVersion*  pUMDhwversion = reinterpret_cast<CSLVersion*>(pDeviceData);
    CamxResult   result        = CamxResultEFailed;

    if (sizeof(CSLVersion) == deviceDataSize)
    {
        *pUMDhwversion = pDevice->hwVersion;
        result = CamxResultSuccess;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalDefaultSubmit
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalDefaultSubmit(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice,
    CSLMemHandle    hPacket,
    SIZE_T          offset,
    INT32           deviceIndex)
{
    struct cam_control        ioctlCmd;
    struct cam_config_dev_cmd submitCmd;
    CSLHwDevice*              pDevice   = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult                result    = CamxResultSuccess;

    ioctlCmd.op_code         = CAM_CONFIG_DEV;
    ioctlCmd.size            = sizeof(struct cam_config_dev_cmd);
    ioctlCmd.handle_type     = CAM_HANDLE_USER_POINTER;
    ioctlCmd.reserved        = 0;
    ioctlCmd.handle          = CamX::Utils::VoidPtrToUINT64(&submitCmd);
    submitCmd.session_handle = hCSL;
    submitCmd.dev_handle     = hDevice;
    submitCmd.offset         = offset;
    submitCmd.packet_handle  = hPacket;

    result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d index %d", pDevice->fd, deviceIndex);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalKMDStreamOp
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalKMDStreamOp(
    CSLHandle       hCSLHwSession,
    CSLDeviceHandle hDevice,
    INT32           deviceIndex,
    BOOL            ops)
{
    CSLHwDevice*                  pDevice  = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*                 pSession = NULL;
    CamxResult                    result   = CamxResultSuccess;
    struct cam_control            ioctlCmd;
    struct cam_start_stop_dev_cmd streamCmd;

    pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

    if (TRUE == ops)
    {
        ioctlCmd.op_code = CAM_START_DEV;
    }
    else
    {
        ioctlCmd.op_code = CAM_STOP_DEV;
    }
    ioctlCmd.size            = sizeof(struct cam_start_stop_dev_cmd);
    ioctlCmd.handle_type     = CAM_HANDLE_USER_POINTER;
    ioctlCmd.reserved        = 0;
    ioctlCmd.handle          = CamX::Utils::VoidPtrToUINT64(&streamCmd);
    streamCmd.session_handle = pSession->hSession;
    streamCmd.dev_handle     = hDevice;

    result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_WARN(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d Device= %s [%d] index %d StreamOn = %d",
            pDevice->fd,
            CSLHwInternalDeviceTypeStrings[pDevice->deviceType],
            pDevice->deviceType,
            deviceIndex,
            ops);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalKMDStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalKMDStreamOn(
    CSLHandle           hCSLHwSession,
    CSLDeviceHandle     hDevice,
    INT32               deviceIndex,
    CSLDeactivateMode   mode)
{
    CSLHwDevice* pDevice    = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result     = CamxResultSuccess;

    if (FALSE == (CSLDeativateModeSensorStandBy & mode))
    {
        result = CSLHwInternalKMDStreamOp(hCSLHwSession, hDevice, deviceIndex, TRUE);
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupCSL, "Device fd %d stream on success index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Device fd %d stream on failed index %d", pDevice->fd, deviceIndex);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalKMDStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalKMDStreamOff(
    CSLHandle           hCSLHwSession,
    CSLDeviceHandle     hDevice,
    INT32               deviceIndex,
    CSLDeactivateMode   mode)
{
    CSLHwDevice* pDevice    = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result     = CamxResultSuccess;

    if (FALSE == (CSLDeativateModeSensorStandBy & mode))
    {
        result = CSLHwInternalKMDStreamOp(hCSLHwSession, hDevice, deviceIndex, FALSE);
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupCSL, "Device fd %d stream off success index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Device fd %d stream off failed index %d", pDevice->fd, deviceIndex);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalKMDRelease
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalKMDRelease(
    CSLHandle       hCSL,
    INT32           deviceIndex,
    CSLDeviceHandle hDevice)
{
    struct cam_release_dev_cmd releaseCmd;
    struct cam_control         ioctlCmd;
    CSLHwDevice*               pDevice       = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*              pSession      = NULL;
    CSLHandle                  hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    CamxResult                 result        = CamxResultSuccess;

    if (hCSLHwSession < CSLHwMaxNumSessions)
    {
        pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

        ioctlCmd.op_code          = CAM_RELEASE_DEV;
        ioctlCmd.size             = sizeof(struct cam_release_dev_cmd);
        ioctlCmd.handle_type      = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved         = 0;
        ioctlCmd.handle           = CamX::Utils::VoidPtrToUINT64(&releaseCmd);
        releaseCmd.session_handle = pSession->hSession;
        releaseCmd.dev_handle     = hDevice;

        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultEOutOfBounds: hCSLHwSession = %d", hCSLHwSession);
        result = CamxResultEOutOfBounds;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalKMDAcquireHardware
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalKMDAcquireHardware(
    CSLHandle          hCSL,
    CSLDeviceHandle    hDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest)
{
    struct cam_acquire_hw_cmd_v1    acquireHWCmd;
    struct cam_control              ioctlCmd;
    CSLHwDevice*                    pDevice         = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*                   pSession        = NULL;
    CSLHandle                       hCSLHwSession   = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    CamxResult                      result          = CamxResultSuccess;
    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering Acquire HW for fd=%d index %d", pDevice->fd, deviceIndex);

    if (hCSLHwSession < CSLHwMaxNumSessions)
    {
        pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

        acquireHWCmd.struct_version = CAM_ACQUIRE_HW_STRUCT_VERSION_1;
        acquireHWCmd.session_handle = pSession->hSession;;
        acquireHWCmd.dev_handle     = hDevice;
        acquireHWCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        acquireHWCmd.data_size      = pDeviceResourceRequest->deviceResourceParamSize;
        acquireHWCmd.resource_hdl   = CamX::Utils::VoidPtrToUINT64(pDeviceResourceRequest->pDeviceResourceParam);

        ioctlCmd.op_code        = CAM_ACQUIRE_HW;
        ioctlCmd.size           = sizeof(struct cam_acquire_hw_cmd_v1);
        ioctlCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved       = 0;
        ioctlCmd.handle         = CamX::Utils::VoidPtrToUINT64(&acquireHWCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering ioctl for fd=%d index %d", pDevice->fd, deviceIndex);
        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d index %d", pDevice->fd, deviceIndex);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultEOutOfBounds: hCSLHwSession = %d", hCSLHwSession);
        result = CamxResultEOutOfBounds;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalKMDAcquireHardwareV2
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalKMDAcquireHardwareV2(
    CSLHandle          hCSL,
    CSLDeviceHandle    hDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest)
{
    struct cam_acquire_hw_cmd_v2    acquireHWCmd;
    struct cam_control              ioctlCmd;
    CSLHwDevice*                    pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*                   pSession = NULL;
    CSLHandle                       hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    CamxResult                      result = CamxResultSuccess;
    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering Acquire HW for fd=%d index %d", pDevice->fd, deviceIndex);

    if (hCSLHwSession < CSLHwMaxNumSessions)
    {
        pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

        acquireHWCmd.struct_version = CAM_ACQUIRE_HW_STRUCT_VERSION_2;
        acquireHWCmd.session_handle = pSession->hSession;;
        acquireHWCmd.dev_handle     = hDevice;
        acquireHWCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        acquireHWCmd.data_size      = pDeviceResourceRequest->deviceResourceParamSize;
        acquireHWCmd.resource_hdl   = CamX::Utils::VoidPtrToUINT64(pDeviceResourceRequest->pDeviceResourceParam);

        ioctlCmd.op_code     = CAM_ACQUIRE_HW;
        ioctlCmd.size        = sizeof(struct cam_acquire_hw_cmd_v2);
        ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved    = 0;
        ioctlCmd.handle      = CamX::Utils::VoidPtrToUINT64(&acquireHWCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering ioctl for fd=%d index %d", pDevice->fd, deviceIndex);
        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CamX::Utils::Memcpy(pDeviceResourceRequest->pReturnParams,
                                &acquireHWCmd.hw_info,
                                sizeof(struct cam_acquired_hw_info));

            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d index %d", pDevice->fd, deviceIndex);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultEOutOfBounds: hCSLHwSession = %d", hCSLHwSession);
        result = CamxResultEOutOfBounds;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalKMDReleaseHardware
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalKMDReleaseHardware(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice,
    INT32           deviceIndex)
{
    struct cam_release_hw_cmd_v1    releaseHWCmd;
    struct cam_control              ioctlCmd;
    CSLHwDevice*                    pDevice         = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*                   pSession        = NULL;
    CSLHandle                       hCSLHwSession   = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    CamxResult                      result          = CamxResultSuccess;
    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering Release HW for fd=%d index %d", pDevice->fd, deviceIndex);

    if (hCSLHwSession < CSLHwMaxNumSessions)
    {
        pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

        releaseHWCmd.struct_version = CAM_ACQUIRE_HW_STRUCT_VERSION_1;
        releaseHWCmd.session_handle = pSession->hSession;
        releaseHWCmd.dev_handle     = hDevice;

        ioctlCmd.op_code        = CAM_RELEASE_HW;
        ioctlCmd.size           = sizeof(struct cam_release_hw_cmd_v1);
        ioctlCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved       = 0;
        ioctlCmd.handle         = CamX::Utils::VoidPtrToUINT64(&releaseHWCmd);

        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultEOutOfBounds: hCSLHwSession = %d", hCSLHwSession);
        result = CamxResultEOutOfBounds;
    }
    return result;
}

CSLHwDeviceOps g_CSLHwDeviceDefaultOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    CSLHwInternalDefaultIoctl2,
    NULL,
    CSLHwInternalDefaultUMDQueryCap,
    NULL,
    NULL,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    NULL,
    CSLHwInternalKMDAcquireHardware,
    CSLHwInternalKMDAcquireHardwareV2,
    CSLHwInternalKMDReleaseHardware
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwMapCSLAllocFlagsToKMD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwMapCSLAllocFlagsToKMD(
    UINT32  CSLFlags,
    UINT32* pKMDFlags)
{
    CamxResult result = CamxResultSuccess;

    if (FALSE == (CSLFlags & CSLMemFlagPacketBuffer))
    {
        if (CSLFlags & CSLMemFlagHw)
        {
            // For now we are mapping all HW access as Read/Write
            // because CSL flags are not as granular as their KMD counterparts
            *pKMDFlags |= CAM_MEM_FLAG_HW_READ_WRITE;
        }

        if (CSLFlags & CSLMemFlagProtected)
        {
            *pKMDFlags |= CAM_MEM_FLAG_PROTECTED_MODE;
        }

        if (CSLFlags & CSLMemFlagCmdBuffer)
        {
            *pKMDFlags |= CAM_MEM_FLAG_CMD_BUF_TYPE;
        }

        // CSLMemFlagProtectedUMDAccess - KMD will anyway ignore UMD_ACCESS, so re-use UMD_ACCESS
        // as a place holder for ProtectedUMDAccess
        if ((CSLFlags & CSLMemFlagUMDAccess) ||
            (CSLFlags & CSLMemFlagProtectedUMDAccess))
        {
            *pKMDFlags |= CAM_MEM_FLAG_UMD_ACCESS;
        }

        if (CSLFlags & CSLMemFlagKMDAccess)
        {
            *pKMDFlags |= CAM_MEM_FLAG_KMD_ACCESS;
        }

        if (CSLFlags & CSLMemFlagCache)
        {
            *pKMDFlags |= CAM_MEM_FLAG_CACHE;
        }

        if (CSLFlags & CSLMemFlagSharedAccess)
        {
            *pKMDFlags |= CAM_MEM_FLAG_HW_SHARED_ACCESS;
        }

        if (CSLFlags & CSLMemFlagDSPSecureAccess)
        {
            *pKMDFlags |= CAM_MEM_FLAG_CDSP_OUTPUT;
        }

        if (CSLFlags & CSLMemFlagDisableDelayedUnmap)
        {
            *pKMDFlags |= CAM_MEM_FLAG_DISABLE_DELAYED_UNMAP;
        }
    }
    else
    {
        result = CamxResultEUnsupported;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwPopulateMMUHandles
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwPopulateMMUHandles(
    INT32*       pMMUHandles,
    UINT32*      pNumHandles,
    const INT32* pDeviceIndices,
    UINT         deviceCount,
    UINT32       CSLFlags)
{
    CSLHwDevice* pDevice      = NULL;
    CamxResult   result       = CamxResultEFailed;
    INT32        totalDevices;
    UINT         i;
    UINT         count = 0;
    INT32        idx;
    UINT32       deviceFilled = 0;

    *pNumHandles = deviceCount;
    pDevice      = g_CSLHwInstance.CSLInternalKMDDevices;
    totalDevices = g_CSLHwInstance.kmdDeviceCount;

    for (i = 0; i < deviceCount; i++)
    {
        idx = pDeviceIndices[i];

        if (idx < 0 || idx >= totalDevices)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid Input CSL indices = %d", idx);
            result = CamxResultEInvalidArg;
            break;
        }

        if (0 != (deviceFilled & (1 << idx)))
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "duplicate device handle %d, existing devices %x", idx, deviceFilled);
            continue;
        }

        if (CSLFlags & CSLMemFlagProtected)
        {

            if (-1 == pDevice[idx].hMapIOMMU.hSecureIOMMU)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid Secure IOMMU Hdl = %u", pDevice[idx].hMapIOMMU.hSecureIOMMU);
                result = CamxResultEInvalidArg;
                break;
            }
            else
            {
                pMMUHandles[count++] = pDevice[idx].hMapIOMMU.hSecureIOMMU;
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Secure IOMMU Hdl = %X, idx = %d", pMMUHandles[i], i);
            }
        }
        else
        {
            if (-1 == pDevice[idx].hMapIOMMU.hNonSecureIOMMU)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid IOMMU Hdl = %u for idx=%d type=%d count %d",
                               pDevice[idx].hMapIOMMU.hNonSecureIOMMU, idx, pDevice[idx].deviceType, deviceCount);
                result = CamxResultEInvalidArg;
                break;
            }
            else
            {
                pMMUHandles[count++] = pDevice[idx].hMapIOMMU.hNonSecureIOMMU;
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Non-secure IOMMU Hdl = %X, idx = %d", pMMUHandles[i], i);
            }
        }
        result = CamxResultSuccess;
        deviceFilled = deviceFilled | (1 << idx);
    }

    *pNumHandles = count;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwPopulateCDMMMUHandles
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwPopulateCDMMMUHandles(
    INT32*       pMMUHandles,
    UINT32*      pNumHandles,
    const INT32* pDeviceIndices,
    UINT         deviceCount,
    UINT32       CSLFlags)
{
    CSLHwDevice* pDevice        = NULL;
    CamxResult   result         = CamxResultEFailed;
    INT32        MMUHandleCount = *pNumHandles;
    INT32        totalDevices;
    UINT         i;
    INT32        idx;
    UINT32       deviceFilled = 0;

    pDevice      = g_CSLHwInstance.CSLInternalKMDDevices;
    totalDevices = g_CSLHwInstance.kmdDeviceCount;

    for (i = 0; i < deviceCount; i++)
    {
        idx = pDeviceIndices[i];
        if (idx < 0 || idx >= totalDevices)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid Input CSL indices = %d", idx);
            result = CamxResultEInvalidArg;
            break;
        }

        if ((deviceFilled & (1 << idx)) != 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "duplicate device handle %d, existing devices %x", idx, deviceFilled);
            continue;
        }

        if (CSLFlags & CSLMemFlagProtected)
        {
            if (-1 == pDevice[idx].hMapCDMIOMMU.hSecureIOMMU)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid Secure CDM IOMMU Hdl = %u for idx=%d type=%d count %d",
                               pDevice[idx].hMapCDMIOMMU.hSecureIOMMU, idx, pDevice[idx].deviceType, deviceCount);
                result = CamxResultEInvalidArg;
                break;
            }
            else
            {
                pMMUHandles[MMUHandleCount] = pDevice[idx].hMapCDMIOMMU.hSecureIOMMU;
                MMUHandleCount++;
            }
        }
        else
        {
            if (-1 == pDevice[idx].hMapCDMIOMMU.hNonSecureIOMMU)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Invalid CDM IOMMU Hdl = %u for idx=%d type=%d count %d",
                                 pDevice[idx].hMapCDMIOMMU.hNonSecureIOMMU, idx, pDevice[idx].deviceType, deviceCount);
            }
            else
            {
                pMMUHandles[MMUHandleCount] = pDevice[idx].hMapCDMIOMMU.hNonSecureIOMMU;
                MMUHandleCount++;
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Got CDM IOMMU Hdl = %u for idx=%d type=%d count %d",
                                 pDevice[idx].hMapCDMIOMMU.hNonSecureIOMMU, idx, pDevice[idx].deviceType, deviceCount);
            }
        }
        result = CamxResultSuccess;
        deviceFilled = deviceFilled | (1 << idx);
    }
    *pNumHandles = MMUHandleCount;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwAddBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwAddBuffer(
    CSLMemHandle    hMem,
    INT             fd,
    UINT64          length,
    CSLBufferInfo** ppCSLBufferInfo,
    UINT32          CSLFlags,
    BOOL            isImported)
{
    INT32               idx;
    CSLHwMemBufferInfo* pLocalBufferInfo  = NULL;
    CSLBufferInfo*      pLocalBuffer      = NULL;
    VOID*               pLocalVirtualAddr = NULL;
    CamxResult          result            = CamxResultSuccess;

    idx = CAM_MEM_MGR_GET_HDL_IDX(hMem);
    if ((0 > idx) || (CAM_MEM_BUFQ_MAX <= idx))
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultEOutOfBounds: idx = %d", idx);
        CSLHwReleaseBufferInKernel(hMem);
        result = CamxResultEOutOfBounds;
    }

    if (CamxResultSuccess == result)
    {
        pLocalBufferInfo = &g_CSLHwInstance.memManager.bufferInfo[idx];
        pLocalBufferInfo->pBufferInfo = static_cast<CSLBufferInfo*>(CAMX_CALLOC(sizeof(CSLBufferInfo)));
        if (NULL == pLocalBufferInfo->pBufferInfo)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultENoMemory");
            CSLHwReleaseBufferInKernel(hMem);
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            // Now do an mmap() if UMD access flag is set but only in non secure case
            if (CSLFlags & CSLMemFlagUMDAccess)
            {
                if (!(CSLFlags & CSLMemFlagProtected) &&
                    (!(CSLFlags & CSLMemFlagProtectedUMDAccess)))
                {
                    pLocalVirtualAddr = CamX::OsUtils::MemMap(fd, length, 0);
                    if (NULL == pLocalVirtualAddr)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "mmap() failed!");
                        CSLHwReleaseBufferInKernel(hMem);
                        result = CamxResultEFailed;
                    }
                }
                else
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Skipping mmap() for secure buffer");
                }
            }

            if (CamxResultSuccess == result)
            {
                // Now update internal structure since mmap was a success
                pLocalBufferInfo->isImported  = isImported;
                pLocalBuffer                  = pLocalBufferInfo->pBufferInfo;
                pLocalBuffer->hHandle         = hMem;
                pLocalBuffer->pVirtualAddr    = pLocalVirtualAddr;
                pLocalBuffer->deviceAddr      = 0;
                pLocalBuffer->fd              = fd;
                pLocalBuffer->offset          = 0;
                pLocalBuffer->size            = length;
                pLocalBuffer->flags           = CSLFlags;

                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Buffer added into CSLMemMgr table idx=%d  fd=%d, hHandle=%d, size=%d, "
                                 "flags=0x%x, isImported=%d, refCount=%d",
                                 idx, pLocalBuffer->fd, pLocalBuffer->hHandle, pLocalBuffer->size, pLocalBuffer->flags,
                                 pLocalBufferInfo->isImported, pLocalBufferInfo->refcount);
            }
        }
    }
    else
    {
        if (NULL != pLocalBuffer)
        {
            pLocalBuffer->pVirtualAddr = NULL;
        }
    }

    *ppCSLBufferInfo = pLocalBuffer;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwReleaseBufferInKernel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwReleaseBufferInKernel(
    CSLMemHandle hBuffer)
{
    struct cam_mem_mgr_release_cmd releaseCmd = {};
    CamxResult                     result     = CamxResultEFailed;
    CSLHwDeviceOps*                pDeviceOp  = &g_CSLHwInstance.requestManager.deviceOp;

    releaseCmd.buf_handle = hBuffer;

    result = pDeviceOp->Ioctl2(&g_CSLHwInstance.requestManager, CAM_REQ_MGR_RELEASE_BUF, &releaseCmd, 0, sizeof(releaseCmd));
    if (result != CamxResultSuccess)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Release Buff IOCTL failed!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwParseDeviceAttribute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwParseDeviceAttribute(
    CSLHwDevice*        pHWDevice,
    CSLDeviceAttribute* pDeviceAttribute,
    SIZE_T              numDeviceAttributes)
{
    CamxResult   result        = CamxResultSuccess;

    /// operationMode
    /// Set default operationMode value to predefined value base on deviceType
    if (TRUE == CSLHwIsHwRealtimeDevice(pHWDevice->deviceType))
    {
        pHWDevice->operationMode = CSLRealtimeOperation;
    }
    else
    {
        pHWDevice->operationMode = CSLNonRealtimeOperation;
    }

    if (NULL != pDeviceAttribute)
    {
        /// Parse the input value and update
        for (UINT i = 0; i < numDeviceAttributes; i++)
        {
            switch (pDeviceAttribute[i].attributeID)
            {
                case CSLDeviceAttributeRealtimeOperation:
                    pHWDevice->operationMode = CSLRealtimeOperation;
                    break;
                case CSLDeviceAttributeNonRealtimeOperation:
                    pHWDevice->operationMode = CSLNonRealtimeOperation;
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "Undefined device attributeID %x", pDeviceAttribute[i].attributeID)
            }
        }
    }
    else
    {
        // Some devices like EEPROM will have NULL pDeviceAttribute, but log anyways in case someone unintentionally passing
        // in NULL.
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "pDeviceAttribute==NULL for pHWDevice=%p", pHWDevice);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwDumpRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwDumpRequest(
    CSLHwsession*          pSession,
    CSLDumpRequestInfo*    pDumpRequestInfo,
    SIZE_T*                pFilledLength)
{
    struct cam_control            cmd;
    struct cam_dump_req_cmd       dumpRequestCmd;
    CamxResult                    result    = CamxResultEFailed;
    CSLHwDeviceOps*               pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;
    CSLHwDevice                   hwDevice;
    SIZE_T                        pre_offset;

    cmd.op_code                       = CAM_REQ_MGR_REQUEST_DUMP;
    cmd.size                          = sizeof(dumpRequestCmd);
    cmd.handle                        = CamX::Utils::VoidPtrToUINT64(&dumpRequestCmd);
    cmd.handle_type                   = CAM_HANDLE_USER_POINTER;
    cmd.reserved                      = 0;

    pre_offset                        = pDumpRequestInfo->offset;
    dumpRequestCmd.session_handle     = pSession->hSession;
    hwDevice                          = g_CSLHwInstance.requestManager;

    // for RT devices
    dumpRequestCmd.issue_req_id       = pDumpRequestInfo->issueSyncId;
    dumpRequestCmd.buf_handle         = pDumpRequestInfo->hBuf;
    dumpRequestCmd.offset             = pDumpRequestInfo->offset;
    dumpRequestCmd.error_type         = pDumpRequestInfo->errorType;

    for (UINT linkIndex = 0; linkIndex < CSLHwMaxLinksArray; linkIndex++)
    {
        dumpRequestCmd.link_hdl = pSession->linkInfo[linkIndex].hLinkIdentifier;
        CAMX_LOG_INFO(CamxLogGroupCSL, "link_hdl=%d, hCSLLinkHandle=%d",
            dumpRequestCmd.link_hdl, pSession->CSLLinkHandleData[linkIndex].hCSLLinkHandle);

        if ((CSLInvalidHandle != dumpRequestCmd.link_hdl) &&
            (TRUE == pSession->linkInfo[linkIndex].isActive))
        {
            CAMX_LOG_INFO(CamxLogGroupCSL, "link_hdl(%d) isActive dumping info",
                dumpRequestCmd.link_hdl);

            result = pDeviceOp->Ioctl(&hwDevice, VIDIOC_CAM_CONTROL, &cmd);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL,
                    "Ioctl Failed for pre reset with session = %d"
                     "issue req id = %llu bufhandle = %u"
                     "issueSyncId = %llu lastRequestId = %llu"
                     "lastSyncId = %llu result = %d",
                    pSession->hSession,
                    pDumpRequestInfo->issueRequestId,
                    dumpRequestCmd.buf_handle,
                    pDumpRequestInfo->issueSyncId,
                    pDumpRequestInfo->lastRequestId,
                    pDumpRequestInfo->lastSyncId,
                    result);
                break;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        // Dump non real time devices
        CSLHwDevice*                       pHWDevice       = NULL;
        CSLHwAcquiredDevice*               pAcquireddevice = NULL;
        CamX::LightweightDoublyLinkedList* pList           = NULL;

        pList = &pSession->nrtList;

        CamX::LDLLNode* pNode = pList->Head();

        // pList contains all of the devices for the CSL session

        while (NULL != pNode)
        {
            pAcquireddevice = (static_cast<CSLHwAcquiredDevice*>(pNode->pData));
            pHWDevice       = &g_CSLHwInstance.CSLInternalKMDDevices[pAcquireddevice->cslDeviceIndex];

            if ((NULL != pHWDevice->deviceOp.Ioctl) && (TRUE == pHWDevice->isActive))
            {
                cmd.op_code                     = CAM_DUMP_REQ;
                cmd.size                        = sizeof(dumpRequestCmd);
                cmd.handle                      = CamX::Utils::VoidPtrToUINT64(&dumpRequestCmd);
                cmd.handle_type                 = CAM_HANDLE_USER_POINTER;
                cmd.reserved                    = 0;

                dumpRequestCmd.dev_handle       = pAcquireddevice->hAcquired;
                dumpRequestCmd.issue_req_id     = pDumpRequestInfo->issueRequestId;

                result = pHWDevice->deviceOp.Ioctl(pHWDevice, VIDIOC_CAM_CONTROL, &cmd);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL,
                        "Ioctl Failed for dump request with session = %d device = %d "
                        "for requestID = %llu syncID = %llu dumped_offset = %zu "
                        "pre_offset = %zu handle = %u result = %d",
                        pSession->hSession,
                        pAcquireddevice->hAcquired,
                        pDumpRequestInfo->issueRequestId,
                        dumpRequestCmd.offset,
                        pre_offset,
                        dumpRequestCmd.dev_handle,
                        result);
                    break;
                }
            }
            pNode = CamX::LightweightDoublyLinkedList::NextNode(pNode);
        }
    }
    *pFilledLength = dumpRequestCmd.offset - pre_offset;
    CAMX_LOG_INFO(CamxLogGroupCSL, "requestId %lld  syncId %lld filledLength %zu",
        pDumpRequestInfo->issueRequestId,
        pDumpRequestInfo->issueSyncId, *pFilledLength);

    return result;
}
#endif // ANDROID
