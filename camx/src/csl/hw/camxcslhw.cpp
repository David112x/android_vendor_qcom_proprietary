////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhw.cpp
///
/// @brief CamxCSL Hw Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <ctime>
#include "camxcsl.h"
#include "camxcsljumptable.h"
#include "camxcslhwinternal.h"
#include "camxhal3types.h"
#include "camxincs.h"
#include "camxlist.h"
#include "camxmem.h"
#include "camxpacketdefs.h"

CSLHwInstance g_CSLHwInstance;
CSLFenceInfo  g_CSLFenceInfo[CSLMaxFences]; //< CSL Fence Information

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwCheckCallerAgainstOwner
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BOOL CSLHwCheckCallerAgainstOwner()
{
    return (g_CSLHwInstance.acquiredPID == CamX::OsUtils::GetProcessID());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLInitializeHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLInitializeHW()
{
    CamxResult result                          = CamxResultEFailed;
    CHAR       syncDeviceName[CSLHwMaxDevName] = {0};

    if (FALSE == CSLHwIsHwInstanceValid())
    {
        if (TRUE == CSLHwEnumerateAndAddCSLHwDevice(CSLInternalHwVideodevice, CAM_VNODE_DEVICE_TYPE))
        {
            if (TRUE == CSLHwEnumerateAndAddCSLHwDevice(CSLInternalHwVideoSubdevice, CAM_CPAS_DEVICE_TYPE))
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Platform family=%d, version=%d.%d.%d, cpas version=%d.%d.%d",
                    g_CSLHwInstance.pCameraPlatform.family,
                    g_CSLHwInstance.pCameraPlatform.platformVersion.majorVersion,
                    g_CSLHwInstance.pCameraPlatform.platformVersion.minorVersion,
                    g_CSLHwInstance.pCameraPlatform.platformVersion.revVersion,
                    g_CSLHwInstance.pCameraPlatform.CPASVersion.majorVersion,
                    g_CSLHwInstance.pCameraPlatform.CPASVersion.minorVersion,
                    g_CSLHwInstance.pCameraPlatform.CPASVersion.revVersion);

                if (FALSE == CSLHwEnumerateAndAddCSLHwDevice(CSLInternalHwVideoSubdeviceAll, 0))
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "No KMD devices found");
                }
                else
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Total KMD subdevices found =%d", g_CSLHwInstance.kmdDeviceCount);
                }
                // Init the memory manager data structures here
                CamX::Utils::Memset(g_CSLHwInstance.memManager.bufferInfo, 0, sizeof(g_CSLHwInstance.memManager.bufferInfo));
                // Init the sync manager here
                g_CSLHwInstance.lock->Lock();
                g_CSLHwInstance.pSyncFW = CamX::SyncManager::GetInstance();
                if (NULL != g_CSLHwInstance.pSyncFW)
                {
                    CSLHwGetSyncHwDevice(syncDeviceName, CSLHwMaxDevName);
                    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Sync device found = %s", syncDeviceName);
                    result = g_CSLHwInstance.pSyncFW->Initialize(syncDeviceName);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "CSL failed to initialize SyncFW");
                        result = g_CSLHwInstance.pSyncFW->Destroy();
                        g_CSLHwInstance.pSyncFW = NULL;
                    }
                }
                g_CSLHwInstance.lock->Unlock();
                CSLHwInstanceSetState(CSLHwValidState);
                result = CamxResultSuccess;
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Successfully acquired requestManager");
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Failed to acquire CPAS");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Failed to acquire requestManager invalid");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CSL in Invalid State");
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLUninitializeHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLUninitializeHW()
{
    CamxResult result = CamxResultEFailed;

    if ((TRUE == CSLHwInstanceGetRefCount()) && (TRUE == CSLHwCheckCallerAgainstOwner()))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Started CSL Uninitialize");
        CSLHwInstanceSetState(CSLHwDestroyingState);
        result = CamxResultSuccess;
        // Make sure the tearing down the devices are in the opposite order of initialize
        CSLHwRemoveHwDevice(CSLInternalHwVideoSubdeviceAll, 0);
        CSLHwRemoveHwDevice(CSLInternalHwVideoSubdevice, CAM_CPAS_DEVICE_TYPE);
        if (NULL != g_CSLHwInstance.pSyncFW)
        {
            result = g_CSLHwInstance.pSyncFW->Uninitialize();
            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("CSL failed to uninitialize SyncFW");
            }
            else
            {
                result = g_CSLHwInstance.pSyncFW->Destroy();
                g_CSLHwInstance.pSyncFW = NULL;
            }
        }
        CSLHwInstancePutRefCount();
        CSLHwRemoveHwDevice(CSLInternalHwVideodevice, CAM_VNODE_DEVICE_TYPE);
        // UnSubscribe for Cam Req Mngr events
        // Cleanup needs to be done here
        CSLHwInstanceSetState(CSLHwInvalidState);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Invalid operation state=%d owner=%d",
                         CSLHwInstanceGetState(), g_CSLHwInstance.acquiredPID);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLLogAcquiredDevices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT CSLLogAcquiredDevices(
    CSLHwsession* pSession)
{
    // WARNING! Assumes CSLHwInstanceGetRefCount() has been set correctly by caller!
    UINT count = 0;

    if (NULL != pSession)
    {
        for (SIZE_T deviceIndex = 0; deviceIndex < CSLHwMaxNumAcquiredDevices; deviceIndex++)
        {
            CSLHwAcquiredDevice* pDevice = &pSession->sessionDevices[deviceIndex];
            if (CSLInvalidHandle != pDevice->hAcquired)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "deviceName=%s, %s, index=%u hAcquired=%d aState=%s refCount=%u",
                              CSLHwInternalDeviceTypeStrings[pDevice->deviceType], pDevice->nodeName, deviceIndex,
                              pDevice->hAcquired, CSLHwInternalDeviceStateStrings[pDevice->aState], pDevice->refCount);
                count++;
            }
        }

        if (0 < count)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "%u devices orphaned", count);
        }

        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "%u devices acquired", count);

    }

    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLOpenHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLOpenHW(
    CSLHandle* phCSL)
{
    CamxResult result = CamxResultEFailed;

    if (NULL != phCSL)
    {
        // The Instance refcount is released only in the CSLClose if everything in this API is success
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            *phCSL = CSLInvalidHandle;
            result = CSLHwInternalCreateSession(phCSL);
            if ((CamxResultSuccess == result) && (CSLInvalidHandle != *phCSL))
            {
                CSLHwsession* pSession      = NULL;
                CSLHandle     hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(*phCSL);
                if (hCSLHwSession < CSLHwMaxNumSessions)
                {
                    pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

                    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "CSL Hw session handle=%x Num session = %d with handle index=%d",
                                     *phCSL, g_CSLHwInstance.numSessions, hCSLHwSession);

                    if (CSLHwInvalidState != CSLHwGetSessionState(pSession))
                    {
                        // Some thing bad lets destroy the session and return error
                        result = CSLHwInternalDestroySession(*phCSL);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCSL, "CSL CSLHwInternalDestroySession failed");
                        }
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "CSLHwSession in invalid state =%d",
                                       CSLHwGetSessionState(pSession));
                        CSLHwInstancePutRefCount();
                        *phCSL = CSLInvalidHandle;
                        result = CamxResultEFailed;
                    }
                    else
                    {
                        g_CSLHwInstance.numSessions++;
                        pSession->lock                       = CamX::Mutex::Create("CSLSession");
                        pSession->destroyCondition           = CamX::Condition::Create("CSLSession Destroy");
                        pSession->lock->Lock();
                        CamX::Utils::Memset(&pSession->sessionDevices, 0, (sizeof(pSession->sessionDevices)));
                        CamX::Utils::Memset(&pSession->linkInfo, 0, (sizeof(pSession->linkInfo)));
                        pSession->linkCount                  = 0;
                        pSession->hMasterLink                = CSLInvalidHandle;
                        pSession->index                      = hCSLHwSession;
                        pSession->messageHandler             = NULL;
                        pSession->sessionMessageHandler      = NULL;
                        pSession->pMessageData               = NULL;
                        pSession->pSessionMessageData        = NULL;
                        pSession->hSession                   = *phCSL;
                        pSession->clientRefCount             = 1;
                        CSLHwSetSessionState(pSession, CSLHwValidState);
                        pSession->lock->Unlock();
                        pSession->acquiredPID = CamX::OsUtils::GetProcessID();
                        pSession->acquiredTID = CamX::OsUtils::GetThreadID();
                    }
                }
                else
                {
                    result = CamxResultEOutOfBounds;
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
                    result = CSLHwInternalDestroySession(*phCSL);
                    *phCSL = CSLInvalidHandle;
                    CSLHwInstancePutRefCount();
                }
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupCSL, "CSL CSLHwInternalCreateSession failed");
                *phCSL = CSLInvalidHandle;
                CSLHwInstancePutRefCount();
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLCloseHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCloseHW(
    CSLHandle hCSL)
{
    CamxResult result           = CamxResultEFailed;
    UINT       staleHandleCount = 0;
    if (CSLInvalidHandle != hCSL)
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            CSLHwsession* pSession  = NULL;
            CSLHandle hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
            if (hCSLHwSession < CSLHwMaxNumSessions)
            {
                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                if ((CSLHwInvalidState != CSLHwGetSessionState(pSession)) && (hCSL == pSession->hSession))
                {
                    INT clientRefCount = CamX::CamxAtomicDec(&pSession->clientRefCount);

                    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Session=0x%x : Updated clientRefCount=%d",
                                     hCSL, pSession->clientRefCount);

                    if (0 == pSession->clientRefCount)
                    {
                        // No more active clients, but wait for any pending ops to complete. We really should never have to
                        // wait here, but let's keep the safety check.
                        if (TRUE == CSLHwSessionWaitForRefcountZero(pSession))
                        {
                            CAMX_LOG_INFO(CamxLogGroupCSL, "Dump any orphaned and acquired devices after close CSLHandle=%d",
                                           hCSL);
                            staleHandleCount = CSLLogAcquiredDevices(pSession);

                            if (1 < staleHandleCount)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCSL, "Stale device handles on Close: count=%u", staleHandleCount);
                                CamX::OsUtils::RaiseSignalAbort();
                            }
                            result = CSLHwInternalDestroySession(hCSL);
                            if (CamxResultSuccess == result)
                            {
                                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Destroying the session = %x Success", hCSL);
                                // The Instance refcount is released for the CSLOpen
                                CSLHwInstancePutRefCount();
                                pSession->lock->Lock();
                                g_CSLHwInstance.numSessions--;
                                // Check if we need to call the handler
                                pSession->messageHandler        = NULL;
                                pSession->pMessageData          = NULL;
                                pSession->sessionMessageHandler = NULL;
                                pSession->pSessionMessageData   = NULL;
                                CamX::Utils::Memset(&pSession->sessionDevices, 0, (sizeof(pSession->sessionDevices)));
                                pSession->lock->Unlock();
                                pSession->lock->Destroy();
                                pSession->destroyCondition->Destroy();
                                CSLHwSetSessionState(pSession, CSLHwInvalidState);
                                CamX::Utils::Memset(pSession, 0, (sizeof(CSLHwsession)));
                            }
                            else
                            {
                                CAMX_ASSERT_ALWAYS();
                            }
                        }
                        else
                        {
                            CAMX_ASSERT_ALWAYS_MESSAGE("Destroying the session = %d failed", hCSL);
                        }
                    }
                    else if (0 > clientRefCount)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "Session=0x%x : Negative clientRefCount=%d!",
                                       hCSL, pSession->clientRefCount);
                    }
                    else
                    {
                        CAMX_LOG_INFO(CamxLogGroupCSL, "Session=0x%x : clientRefCount=%d resetting the state in close",
                                       hCSL, pSession->clientRefCount);
                        CSLHwSetSessionState(pSession, CSLHwValidState);
                    }
                }
                else
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Destroying the session = %d failed", hCSL);
                }
            }
            else
            {
                result = CamxResultEOutOfBounds;
                CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
            }

            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLAddReferenceHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAddReferenceHW(
    CSLHandle hCSL)
{
    CamxResult result = CamxResultEInvalidArg;

    if (CSLInvalidHandle != hCSL)
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            CSLHandle hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
            if (CSLHwMaxNumSessions > hCSLHwSession)
            {
                CSLHwsession* pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

                if ((CSLHwInvalidState != CSLHwGetSessionState(pSession)) && (hCSL == pSession->hSession))
                {
                    INT clientRefCount = CamX::CamxAtomicInc(&pSession->clientRefCount);

                    if (1 >= clientRefCount)
                    {
                        CAMX_ASSERT_ALWAYS();
                    }

                    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Session=0x%x : Updated clientRefCount=%d",
                                     hCSL, pSession->clientRefCount);

                    result = CamxResultSuccess;
                }
                else
                {
                    result = CamxResultEFailed;
                    CAMX_ASSERT_ALWAYS_MESSAGE("Session=0x%x in invalid state", hCSL);
                }
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Session=0x%x is CSLInvalidHandle", hCSL);
            }

            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Session=0x%x is CSLInvalidHandle", hCSL);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLQueryCameraPlatformHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLQueryCameraPlatformHW(
    CSLCameraPlatform* pCameraPlatform)
{
    CamxResult result = CamxResultEFailed;

    if (NULL != pCameraPlatform)
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            CamX::Utils::Memcpy(pCameraPlatform, &g_CSLHwInstance.pCameraPlatform, sizeof(CSLCameraPlatform));
            result = CamxResultSuccess;
            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLImageSensorProbeHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLImageSensorProbeHW(
    CSLMemHandle                hPacket,
    SIZE_T                      offset,
    CSLImageSensorProbeResult*  pProbeResult)
{

    CamxResult result = CamxResultEFailed;

    if ((NULL != pProbeResult) && (CSLInvalidHandle != hPacket))
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            INT32   deviceIndex;

            result = CSLHwInternalProbeSensorHW(hPacket, offset, &deviceIndex);
            if (CamxResultSuccess == result)
            {
                pProbeResult->detected    = TRUE;
                pProbeResult->deviceIndex = deviceIndex;
            }
            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLSetDebugBufferHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSetDebugBufferHW(
    CSLHandle               hCSL,
    CSLDebugBufferType      type,
    CSLMemHandle            hBuffer,
    SIZE_T                  offset,
    SIZE_T                  length,
    CSLDebugBufferResult*   pDebugBufferResult)
{
    CamxResult      result    = CamxResultEFailed;
#if CAM_REQ_MGR_SET_KMD_BUF_DEBUG
    CSLHwDeviceOps* pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;

    // Obtain a CSL instance
    if (TRUE == CSLHwInstanceGetRefCount())
    {
        CSLHwsession* pSession = &g_CSLHwInstance.sessionList[CAM_REQ_MGR_GET_HDL_IDX(hCSL)];
        if (TRUE == CSLHwSessionGetRefCount(pSession))
        {
            struct cam_mem_set_kmd_buf  setDebugBufferCmd = {};
            setDebugBufferCmd.session_hdl   = pSession->hSession;
            setDebugBufferCmd.buf_handle    = hBuffer;
            setDebugBufferCmd.buf_offset    = offset;
            setDebugBufferCmd.buf_len       = length;

            switch(type)
            {
                case CSLDebugBufferTypeSCOPE:
                    result = CamxResultSuccess;
                    setDebugBufferCmd.buf_type = CAM_BUF_DEBUG_TYPE_SCOPE;
                    break;
                default:
                    // Unknown debug buffer type
                    result = CamxResultEInvalidArg;
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid debug buffer type %d.", type);
                    break;
            }

            if (CamxResultSuccess == result)
            {
                result = pDeviceOp->Ioctl2(&g_CSLHwInstance.requestManager, CAM_REQ_MGR_SET_KMD_BUF_DEBUG, &setDebugBufferCmd,
                                           0, sizeof(setDebugBufferCmd));

                if (CamxResultSuccess == result)
                {
                    if (NULL != pDebugBufferResult)
                    {
                        pDebugBufferResult->hHandle         = setDebugBufferCmd.out.buf_handle;
                        pDebugBufferResult->offset          = setDebugBufferCmd.out.buf_offset;
                        pDebugBufferResult->bytesWritten    = setDebugBufferCmd.out.bytes_used;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "Set debug buffer (type = %d) failed for handle = %u",
                                    type, hBuffer);
                }
            }

            CSLHwSessionPutRefCount(pSession);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Unable to acquire session");
            result = CamxResultEInvalidState;
        }

        // Release CSL
        CSLHwInstancePutRefCount();
    }
#else
    CAMX_UNREFERENCED_PARAM(hCSL);
    CAMX_UNREFERENCED_PARAM(type);
    CAMX_UNREFERENCED_PARAM(hBuffer);
    CAMX_UNREFERENCED_PARAM(offset);
    CAMX_UNREFERENCED_PARAM(length);
    CAMX_UNREFERENCED_PARAM(pDebugBufferResult);
#endif // CAM_REQ_MGR_SET_KMD_BUF_DEBUG

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLEnumerateDevicesHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLEnumerateDevicesHW(
    CSLDeviceDescriptor* pDeviceDescriptor)
{
    CamxResult result = CamxResultEFailed;

    if (NULL == pDeviceDescriptor)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument (NULL pointer).");
    }
    else if (pDeviceDescriptor->deviceIndex < 0)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument (device index is negative).");
    }
    else if (pDeviceDescriptor->deviceIndex >= CSLHwMaxKMDNumDevices)
    {
        result = CamxResultENoMore;
        CAMX_LOG_INFO(CamxLogGroupCSL, "No more devices available (index: %d).", pDeviceDescriptor->deviceIndex);
    }
    else
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            g_CSLHwInstance.lock->Lock();
            if ((pDeviceDescriptor->deviceIndex) <= g_CSLHwInstance.kmdDeviceCount)
            {
                CSLHwDevice* pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[pDeviceDescriptor->deviceIndex];

                // Update this from pDevice->DeviceDescriptor instead of individually
                pDeviceDescriptor->deviceType    = CSLHwGetDeviceTypeFromInternal(pDevice->deviceType);
                pDeviceDescriptor->driverVersion = pDevice->driverVersion;
                pDeviceDescriptor->hwVersion     = pDevice->hwVersion;
                result = CamxResultSuccess;
            }
            else
            {
                result = CamxResultENoMore;
                CAMX_LOG_INFO(CamxLogGroupCSL, "No more devices available (index: %d).", pDeviceDescriptor->deviceIndex);
            }
            g_CSLHwInstance.lock->Unlock();
            CSLHwInstancePutRefCount();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLQueryDeviceCapabilitiesHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLQueryDeviceCapabilitiesHW(
    INT32   deviceIndex,
    VOID*   pDeviceData,
    SIZE_T  deviceDataSize)
{
    CamxResult result = CamxResultEFailed;

    if ((NULL != pDeviceData) && (0 <= deviceIndex) && (0 < deviceDataSize) &&
       (CSLHwMaxKMDNumDevices > deviceIndex))
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            CSLHwDevice* pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];

            if (NULL != pDevice->deviceOp.UMDQueryCap)
            {
                result = pDevice->deviceOp.UMDQueryCap(deviceIndex, pDeviceData, deviceDataSize);
            }
            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLAcquireDeviceHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAcquireDeviceHW(
    CSLHandle           hCSL,
    CSLDeviceHandle*    phDevice,
    INT32               deviceIndex,
    CSLDeviceResource*  pDeviceResourceRequest,
    SIZE_T              numDeviceResources,
    CSLDeviceAttribute* pDeviceAttribute,
    SIZE_T              numDeviceAttributes,
    const CHAR*         pDeviceName)
{
    CamxResult result = CamxResultEFailed;

    if ((CSLInvalidHandle != hCSL) && (NULL != phDevice) && (0 <= deviceIndex) && ((deviceIndex) < CSLHwMaxKMDNumDevices))
        // looks like sensor has no data for now so changing this to allow null and size zero data
        // && pDeviceResourceRequest && (deviceResourceSize != 0))
    {
        CSLHandle hTempindex = CSLInvalidHandle;

        hTempindex = CAM_REQ_MGR_GET_HDL_IDX(hCSL);

        if (hTempindex < CSLHwMaxNumSessions)
        {
            *phDevice = CSLInvalidHandle;
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                g_CSLHwInstance.lock->Lock();
                CSLHwsession* pSession = NULL;
                CSLHwDevice* pHWDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
                pSession               = &g_CSLHwInstance.sessionList[hTempindex];

                g_CSLHwInstance.lock->Unlock();

                if ((hCSL == pSession->hSession) &&
                    (NULL != pHWDevice->deviceOp.Acquire))
                {
                    // The Instance refcount is released only in the CSLReleaseDeviceHW if everything in this API is success
                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        result = pHWDevice->deviceOp.Acquire(hCSL,
                                                             phDevice,
                                                             deviceIndex,
                                                             pDeviceResourceRequest,
                                                             numDeviceResources);

                        if (CamxResultSuccess == result)
                        {
                            if (CSLInvalidHandle != *phDevice)
                            {
                                CSLHwAcquiredDevice* pAcquireddevice = NULL;

                                CAMX_LOG_CONFIG(CamxLogGroupCSL, "DeviceAcquired: Name=%s phDevice=%d name:%s",
                                                CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType],
                                                *phDevice, pHWDevice->devName);

                                g_CSLHwInstance.lock->Lock();
                                hTempindex = CAM_REQ_MGR_GET_HDL_IDX(*phDevice);
                                g_CSLHwInstance.lock->Unlock();

                                if (hTempindex < CSLHwMaxNumAcquiredDevices)
                                {
                                    g_CSLHwInstance.lock->Lock();
                                    pAcquireddevice = &pSession->sessionDevices[hTempindex];
                                    CSLHwKMDDeviceGetRefcount(pHWDevice);
                                    pHWDevice->hAcquired[hTempindex] = *phDevice;
                                    CSLHwParseDeviceAttribute(pHWDevice, pDeviceAttribute, numDeviceAttributes);
                                    g_CSLHwInstance.lock->Unlock();

                                    pAcquireddevice->lock             = CamX::Mutex::Create("HWDevice");
                                    pAcquireddevice->destroyCondition = CamX::Condition::Create("HWDevice Destroy");
                                    pAcquireddevice->lock->Lock();

                                    CamX::OsUtils::StrLCpy(pAcquireddevice->nodeName, pDeviceName, MaxStringLength256);

                                    pAcquireddevice->refCount        = 0;
                                    pAcquireddevice->hAcquired       = *phDevice;
                                    pAcquireddevice->hSession        = hCSL;
                                    pAcquireddevice->cslDeviceIndex  = deviceIndex;
                                    pAcquireddevice->deviceType      = pHWDevice->deviceType;
                                    pAcquireddevice->fd              = -1;
                                    pAcquireddevice->mode            = pHWDevice->mode;
                                    pAcquireddevice->operationMode   = pHWDevice->operationMode;
                                    CamX::CamxAtomicStore32(&pAcquireddevice->aState, CSLHwValidState);
                                    CSLHwGetHwDeviceOrder(pHWDevice->deviceType, &pAcquireddevice->order);
                                    CSLHwGetHwDeviceStreamOffOrder(pHWDevice->deviceType, &pAcquireddevice->orderoff);
                                    pAcquireddevice->lock->Unlock();

                                    CamX::LDLLNode* pNode = static_cast<CamX::LDLLNode*>(CAMX_CALLOC(sizeof(CamX::LDLLNode)));

                                    if (NULL != pNode)
                                    {
                                        pNode->pData = pAcquireddevice;
                                        pSession->lock->Lock();
                                        if (CSLRealtimeOperation == pHWDevice->operationMode)
                                        {
                                            pSession->rtList.InsertToTail(pNode);
                                        }
                                        else
                                        {
                                            pSession->nrtList.InsertToTail(pNode);
                                        }
                                        pSession->lock->Unlock();
                                    }
                                    else
                                    {
                                        result = CamxResultENoMemory;
                                    }
                                }
                                else
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempindex >= CSLHwMaxNumAcquiredDevices");
                                    result = CamxResultEOutOfBounds;
                                }
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCSL, "phDevice is invalid for deviceName=%s, %s, index=%d,"
                                           "aState=%s , name:%s",
                                            CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType],
                                            pDeviceName, deviceIndex,
                                            CSLHwInternalDeviceStateStrings[pHWDevice->aState],
                                            pHWDevice->devName);
                                result = CamxResultEFailed;
                            }
                        }
                        else
                        {
                            CAMX_LOG_ERROR(
                                           CamxLogGroupCSL,
                                           "Acquire failed for deviceName=%s, %s, index=%d, aState=%s, name:%s",
                                            CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType],
                                            pDeviceName, deviceIndex,
                                            CSLHwInternalDeviceStateStrings[pHWDevice->aState],
                                            pHWDevice->devName);
                            CAMX_ASSERT_ALWAYS();
                        }
                        CSLHwSessionPutRefCount(pSession);
                    }
                }

                if (CamxResultSuccess != result)
                {
                    CSLLogAcquiredDevices(pSession);
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempindex >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess != result)
    {
        CSLHwDevice* pHWDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
        CAMX_LOG_ERROR(CamxLogGroupCSL,
                       "Acquire Device failure: hCSL: 0x%x, deviceIndex: %d, phDevice: 0x%x, Name=%s, result=%u",
                       hCSL,
                       deviceIndex,
                       phDevice,
                       CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType],
                       result);
        CamX::OsUtils::RaiseSignalAbort();
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLReleaseDeviceHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseDeviceHW(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice)
{
    CamxResult result = CamxResultEFailed;

    g_CSLHwInstance.lock->Lock();
    if ((CSLInvalidHandle != hCSL) && (CSLInvalidHandle != hDevice))
    {
        CSLHandle hTempIndex = CSLInvalidHandle;

        hTempIndex = CAM_REQ_MGR_GET_HDL_IDX(hCSL);

        if (hTempIndex < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hTempIndex];
                if (TRUE == CSLHwSessionGetRefCount(pSession))
                {
                    if (hCSL == pSession->hSession)
                    {
                        CSLHwAcquiredDevice*               pReleaseDevice = NULL;
                        CSLHwDevice*                       pHWDevice      = NULL;
                        CamX::LightweightDoublyLinkedList* pList          = NULL;

                        hTempIndex = CAM_REQ_MGR_GET_HDL_IDX(hDevice);
                        if (hTempIndex < CSLHwMaxNumAcquiredDevices)
                        {
                            pReleaseDevice = &pSession->sessionDevices[hTempIndex];
                            CAMX_ASSERT(hDevice == pReleaseDevice->hAcquired);
                            pHWDevice = &g_CSLHwInstance.CSLInternalKMDDevices[pReleaseDevice->cslDeviceIndex];
                            pHWDevice->hAcquired[hTempIndex] = 0;
                            if (NULL != pReleaseDevice->lock)
                            {
                                pReleaseDevice->lock->Lock();
                            }

                            if (NULL != (pHWDevice->deviceOp.Release))
                            {
                                result = pHWDevice->deviceOp.Release(hCSL, pReleaseDevice->cslDeviceIndex, hDevice);
                                if (CamxResultSuccess != result)
                                {
                                    CAMX_ASSERT_ALWAYS();
                                    CAMX_LOG_ERROR(
                                        CamxLogGroupCSL,
                                        "hCSL release failed: %d, cslDeviceIndex %d oishandle, hDevice %d name:%s",
                                         hCSL, pReleaseDevice->cslDeviceIndex, hDevice,
                                         CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType]);
                                }
                                else
                                {
                                    CAMX_LOG_CONFIG(
                                       CamxLogGroupCSL,
                                        "hCSL release success: %d, cslDeviceIndex %d oishandle, hDevice %d name:%s",
                                        hCSL, pReleaseDevice->cslDeviceIndex, hDevice,
                                        CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType]);
                                }
                                pSession->lock->Lock();
                                if (CSLRealtimeOperation == pReleaseDevice->operationMode)
                                {
                                    pList = &pSession->rtList;
                                }
                                else
                                {
                                    pList = &pSession->nrtList;
                                }

                                CamX::LDLLNode* pNode = pList->Tail();

                                while (NULL != pNode)
                                {
                                    if (((static_cast<CSLHwAcquiredDevice*>(pNode->pData))->hAcquired) == hDevice)
                                    {
                                        break;
                                    }

                                    pNode = pNode->pPrev;
                                }

                                if (NULL != pNode)
                                {
                                    pList->RemoveNode(pNode);
                                    CAMX_FREE(pNode);
                                    pNode = NULL;
                                }
                                else
                                {
                                    CAMX_ASSERT_ALWAYS();
                                }
                                pSession->lock->Unlock();
                                // Need to check on the ref count here
                                pReleaseDevice->hAcquired = 0;
                                pReleaseDevice->cslDeviceIndex = 0;
                                pReleaseDevice->fd = -1;
                                CamX::CamxAtomicStore32(&pReleaseDevice->aState, CSLHwInvalidState);
                                if (NULL != pReleaseDevice->lock)
                                {
                                    pReleaseDevice->lock->Unlock();
                                    pReleaseDevice->lock->Destroy();
                                }
                                if (NULL != pReleaseDevice->destroyCondition)
                                {
                                    pReleaseDevice->destroyCondition->Destroy();
                                }
                                CamX::Utils::Memset(pReleaseDevice, 0, (sizeof(CSLHwAcquiredDevice)));
                                CSLHwKMDDevicePutRefcount(pHWDevice);
                                result = CamxResultSuccess;
                            }
                            else
                            {
                                if (NULL != pReleaseDevice->lock)
                                {
                                    pReleaseDevice->lock->Unlock();
                                }
                                CAMX_ASSERT_ALWAYS();
                            }
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempIndex >= CSLHwMaxNumAcquiredDevices");
                            result = CamxResultEOutOfBounds;
                        }
                    }
                    else
                    {
                        CAMX_ASSERT_ALWAYS();
                    }
                    // Release the session refcount for current API
                    CSLHwSessionPutRefCount(pSession);
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempIndex >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    g_CSLHwInstance.lock->Unlock();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLAcquireHardwareHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAcquireHardwareHW(
    CSLHandle           hCSL,
    CSLDeviceHandle     hDevice,
    CSLDeviceResource*  pDeviceResourceRequest,
    UINT32              version)
{
    CamxResult result = CamxResultEFailed;

    g_CSLHwInstance.lock->Lock();
    if ((CSLInvalidHandle != hCSL) && (CSLInvalidHandle != hDevice) && (NULL != pDeviceResourceRequest))
    {
        CSLHandle hTempIndex    = CSLInvalidHandle;
        hTempIndex              = CAM_REQ_MGR_GET_HDL_IDX(hCSL);

        if ((hTempIndex < CSLHwMaxNumSessions) && (TRUE == CSLHwInstanceGetRefCount()))
        {
            CSLHwsession* pSession  = NULL;
            pSession                = &g_CSLHwInstance.sessionList[hTempIndex];

            if ((hCSL == pSession->hSession) && (TRUE == CSLHwSessionGetRefCount(pSession)))
            {
                CSLHwAcquiredDevice* pAcquiredDevice    = NULL;
                CSLHwDevice*         pHWDevice          = NULL;

                hTempIndex = CAM_REQ_MGR_GET_HDL_IDX(hDevice);
                if (hTempIndex < CSLHwMaxNumAcquiredDevices)
                {
                    pAcquiredDevice = &pSession->sessionDevices[hTempIndex];
                    pHWDevice       = &g_CSLHwInstance.CSLInternalKMDDevices[pAcquiredDevice->cslDeviceIndex];
                    if (NULL != pAcquiredDevice->lock)
                    {
                        pAcquiredDevice->lock->Lock();
                    }

                    if ((version == CSLAcquiredDeviceVersion2) && (NULL != (pHWDevice->deviceOp.AcquireHardware)))
                    {
                        result = pHWDevice->deviceOp.AcquireHardwareV2(hCSL,
                                                                       hDevice,
                                                                       pAcquiredDevice->cslDeviceIndex,
                                                                       pDeviceResourceRequest);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %d, cslDeviceIndex %d, hDevice %d version %d",
                                           hCSL, pAcquiredDevice->cslDeviceIndex, hDevice, version);
                        }

                    }
                    else if (NULL != (pHWDevice->deviceOp.AcquireHardware))
                    {
                        result = pHWDevice->deviceOp.AcquireHardware(hCSL,
                                                                     hDevice,
                                                                     pAcquiredDevice->cslDeviceIndex,
                                                                     pDeviceResourceRequest);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_ASSERT_ALWAYS();
                            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %d, cslDeviceIndex %d, hDevice %d version %d",
                                           hCSL, pAcquiredDevice->cslDeviceIndex, hDevice, version);
                        }
                    }

                    CAMX_LOG_CONFIG(CamxLogGroupCSL, "DeviceAcquired: Name=%s phDevice=%p",
                                  CSLHwInternalDeviceTypeStrings[pHWDevice->deviceType], hDevice);

                    if (NULL != pAcquiredDevice->lock)
                    {
                        pAcquiredDevice->lock->Unlock();
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempIndex %d CSLHwMaxNumAcquiredDevices %d",
                                   hTempIndex, CSLHwMaxNumAcquiredDevices);
                    result = CamxResultEOutOfBounds;
                }

                // Release the session refcount for current API
                CSLHwSessionPutRefCount(pSession);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL %d pSession->hSession %d", hCSL, pSession->hSession);
                result = CamxResultEInvalidArg;
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Device dump on failure: result=%u", result);
                CSLLogAcquiredDevices(pSession);
            }

            CSLHwInstancePutRefCount();
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempIndex %d CSLHwMaxNumSessions %d", hTempIndex, CSLHwMaxNumSessions);
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Acquire failure: result=%u", result);
        CamX::OsUtils::RaiseSignalAbort();
    }

    g_CSLHwInstance.lock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLReleaseHardwareHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseHardwareHW(
    CSLHandle           hCSL,
    CSLDeviceHandle     hDevice)
{
    CamxResult result = CamxResultEFailed;

    g_CSLHwInstance.lock->Lock();
    if ((CSLInvalidHandle != hCSL) && (CSLInvalidHandle != hDevice))
    {
        CSLHandle hTempIndex    = CSLInvalidHandle;
        hTempIndex              = CAM_REQ_MGR_GET_HDL_IDX(hCSL);

        if ((hTempIndex < CSLHwMaxNumSessions) && (TRUE == CSLHwInstanceGetRefCount()))
        {
            CSLHwsession* pSession  = NULL;
            pSession                = &g_CSLHwInstance.sessionList[hTempIndex];

            if ((hCSL == pSession->hSession) && (TRUE == CSLHwSessionGetRefCount(pSession)))
            {
                CSLHwAcquiredDevice* pAcquiredDevice    = NULL;
                CSLHwDevice*         pHWDevice          = NULL;

                hTempIndex = CAM_REQ_MGR_GET_HDL_IDX(hDevice);
                if (hTempIndex < CSLHwMaxNumAcquiredDevices)
                {
                    pAcquiredDevice = &pSession->sessionDevices[hTempIndex];
                    pHWDevice       = &g_CSLHwInstance.CSLInternalKMDDevices[pAcquiredDevice->cslDeviceIndex];
                    if (NULL != pAcquiredDevice->lock)
                    {
                        pAcquiredDevice->lock->Lock();
                    }

                    if (NULL != (pHWDevice->deviceOp.ReleaseHardware))
                    {
                        result = pHWDevice->deviceOp.ReleaseHardware(hCSL,
                                                                     hDevice,
                                                                     pAcquiredDevice->cslDeviceIndex);
                        if (CamxResultSuccess != result)
                        {
                            CAMX_ASSERT_ALWAYS();
                            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %d, cslDeviceIndex %d, hDevice %d",
                                           hCSL, pAcquiredDevice->cslDeviceIndex, hDevice);
                        }
                    }

                    if (NULL != pAcquiredDevice->lock)
                    {
                        pAcquiredDevice->lock->Unlock();
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempIndex %d CSLHwMaxNumAcquiredDevices %d",
                                   hTempIndex, CSLHwMaxNumAcquiredDevices);
                    result = CamxResultEOutOfBounds;
                }

                // Release the session refcount for current API
                CSLHwSessionPutRefCount(pSession);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL %d pSession->hSession %d", hCSL, pSession->hSession);
                result = CamxResultEInvalidArg;
            }
            CSLHwInstancePutRefCount();
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempIndex %d CSLHwMaxNumSessions %d", hTempIndex, CSLHwMaxNumSessions);
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    g_CSLHwInstance.lock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLLinkHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLLinkHW(
    CSLHandle         hCSL,
    CSLDeviceHandle*  phDevices,
    UINT              handleCount,
    CSLLinkHandle*    phLink)
{
    CamxResult result = CamxResultEFailed;
    CAMX_UNREFERENCED_PARAM(handleCount);

    if ((NULL != phDevices) &&
        (NULL != phLink) &&
        (CSLInvalidHandle != hCSL))
    {
        CSLHandle hTempindex = CSLInvalidHandle;

        hTempindex = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hTempindex < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hTempindex];

                if (hCSL == pSession->hSession)
                {
                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        CamX::LightweightDoublyLinkedList* pList                                      = NULL;
                        CSLHwAcquiredDevice*               pAcquireddevice                            = NULL;
                        UINT                               numHandles                                 = 0;
                        UINT                               realtimeDeviceCount                        = 0;
                        CSLDeviceHandle                    hDeviceHandles[CSLHwMaxNumAcquiredDevices] = {0};
                        CSLDeviceHandle                    hDevicesRT[CSLHwMaxNumAcquiredDevices]     = {0};

                        result = CamxResultSuccess;

                        pSession->lock->Lock();
                        // First trigger a real time devices stream on
                        pList = &pSession->rtList;
                        // First sort the real time devices in order
                        pList->Sort(CSLHwRtDeviceOderCompare);

                        // Links are owned by pipelines, need to make sure we are not exceeding maxlinks
                        // when creating a new one for a new pipeline that's streaming on
                        if (CSLHwMaxLinksArray > pSession->linkCount)
                        {
                            CamX::LDLLNode* pNode = pList->Head();

                            while (NULL != pNode)
                            {
                                pAcquireddevice = (static_cast<CSLHwAcquiredDevice*>(pNode->pData));
                                // Build up a list of ONLY the pipeline's devices to link
                                for (UINT i = 0; i < CamxMaxDeviceIndex; i++)
                                {
                                    if ((CSLHwCSIPHY != pAcquireddevice->deviceType) &&
                                        (phDevices[i] == pAcquireddevice->hAcquired))
                                    {
                                        hDeviceHandles[numHandles] = pAcquireddevice->hAcquired;
                                        numHandles++;
                                    }
                                }
                                pNode = CamX::LightweightDoublyLinkedList::NextNode(pNode);
                            }

                            if ((0 != numHandles) && (CSLHwMaxKMDNumDevices > numHandles))
                            {
                                // numHandles is for the number of hDeviceHandles (need to know how many devices to find the one
                                // the particular pipeline is trying to link)
                                for (UINT i = 0; i < numHandles; i++)
                                {
                                    UINT hTempDevIndex = CAM_REQ_MGR_GET_HDL_IDX(hDeviceHandles[i]);
                                    if (hTempDevIndex < CSLHwMaxNumAcquiredDevices)
                                    {
                                        pAcquireddevice = &pSession->sessionDevices[hTempDevIndex];

                                        // Should only link RealTime devices (not including CSIPHY)
                                        if ((CSLRealtimeOperation == pAcquireddevice->operationMode) &&
                                            (CSLHwCSIPHY != pAcquireddevice->deviceType))
                                        {
                                            hDevicesRT[realtimeDeviceCount++] = hDeviceHandles[i];
                                        }
                                    }
                                }

                                if ((hCSL == pSession->hSession) && (CSLHwMaxLinksArray > pSession->linkCount))
                                {
                                    result = CSLHwLinkKMDHardwares(pSession, hDevicesRT, realtimeDeviceCount, phLink);
                                    if (CamxResultSuccess == result)
                                    {
                                        hTempindex = CAM_REQ_MGR_GET_HDL_IDX(*phLink);
                                        if (hTempindex < CSLHwMaxLinksArray)
                                        {
                                            pSession->linkInfo[hTempindex].hLinkIdentifier = *phLink;
                                            pSession->linkInfo[hTempindex].num_devices = realtimeDeviceCount;
                                            CamX::Utils::Memcpy(&pSession->linkInfo[hTempindex].hDevice, hDevicesRT,
                                                (realtimeDeviceCount * sizeof(CSLDeviceHandle)));
                                            pSession->linkCount++;

                                            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "CSL Link success with handle =%x", *phLink);
                                            pSession->hMasterLink = *phLink;
                                        }
                                        else
                                        {
                                            CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempindex >= CSLHwMaxLinksArray");
                                            result = CamxResultEOutOfBounds;
                                        }
                                    }
                                }
                                else
                                {
                                    result = CamxResultEOutOfBounds;
                                    CAMX_LOG_ERROR(CamxLogGroupCSL, "Failed to link HW linkCount: %d greater than max: %d",
                                                   pSession->linkCount, CSLHwMaxLinksArray);
                                }
                            }
                            else
                            {
                                result = CamxResultEResource;
                                CAMX_LOG_ERROR(CamxLogGroupCSL,
                                               "Didn't match any devices, pipeline has nothing to StreamOn for session %p",
                                               hCSL);
                            }
                        }
                        pSession->lock->Unlock();
                        CSLHwSessionPutRefCount(pSession);
                    }
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempindex >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLUnlinkHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLUnlinkHW(
    CSLHandle      hCSL,
    CSLLinkHandle* phLink)
{
    CamxResult result = CamxResultEFailed;

    if ((CSLInvalidHandle != hCSL) && (CSLInvalidHandle != *phLink))
    {
        CSLHandle hTempindex = CSLInvalidHandle;

        hTempindex = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hTempindex < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hTempindex];
                if (TRUE == CSLHwSessionGetRefCount(pSession))
                {
                    if ((hCSL == pSession->hSession) && (0 != pSession->linkCount))
                    {
                        CSLHwLinks* pLink;

                        CSLHandle hTempLinkindex = CAM_REQ_MGR_GET_HDL_IDX(*phLink);
                        CAMX_ASSERT(CSLHwMaxLinksArray > hTempLinkindex);
                        if (CSLHwMaxLinksArray > hTempLinkindex)
                        {
                            pLink = &pSession->linkInfo[hTempLinkindex];
                            result = CSLHwUnLinkKMDHardwares(pSession, pLink->hLinkIdentifier);
                            if (CamxResultSuccess == result)
                            {
                                for (UINT32 index = 0; index < CSLHwMaxNumSessions; index++)
                                {
                                    if (pSession->CSLLinkHandleData[index].hCSLLinkHandle)
                                    {
                                        if (pSession->CSLLinkHandleData[index].hCSLLinkHandle == *phLink)
                                        {
                                            pSession->CSLLinkHandleData[index].hCSLLinkHandle = 0;
                                        }
                                    }
                                }
                                pSession->linkCount--;
                                CamX::Utils::Memset(pLink, 0, (sizeof(CSLHwLinks)));

                                if (pLink->hLinkIdentifier == pSession->hMasterLink)
                                {
                                    pSession->hMasterLink      = 0;
                                }
                                *phLink = CSLInvalidHandle;
                            }
                            else
                            {
                                CAMX_ASSERT_ALWAYS_MESSAGE("CSL unlink failed for LinkInfo=0x%x",
                                                           pSession->linkInfo[hTempLinkindex].hLinkIdentifier);
                            }
                        }
                    }
                    CSLHwSessionPutRefCount(pSession);
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempindex >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLSyncLinksHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSyncLinksHW(
    CSLHandle       hCSL,
    CSLLinkHandle*  phLink,
    UINT            handleCount,
    CSLLinkHandle   hMasterLink,
    CSLSyncLinkMode syncMode)
{
    CamxResult result = CamxResultEFailed;

    if ((NULL != phLink) && (CSLInvalidHandle != hMasterLink) && (0 != handleCount) &&
        (CSLHwMaxKMDNumDevices > handleCount))
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hCSLHwSession < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                pSession->hMasterLink = hMasterLink;
                if (hCSL == pSession->hSession)
                {
                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        result = CSLHwSyncLinkKMDHardwares(pSession, phLink, handleCount, hMasterLink, syncMode);
                        CSLHwSessionPutRefCount(pSession);
                    }
                }
                else
                {
                    CAMX_ASSERT_ALWAYS();
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLOpenRequestHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CSLOpenRequestHW(
    CSLHandle          hCSL,
    CSLLinkHandle      hLink,
    UINT64             requestId,
    BOOL               bubble,
    CSLSyncLinkMode    syncMode,
    UINT32             expectedExposureTime)
{
    CamxResult result = CamxResultEFailed;
    if (CSLInvalidHandle != hCSL)
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            CSLHwsession* pSession      = NULL;
            CSLHandle     hCSLHwSession = CSLInvalidHandle;

            hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
            if (hCSLHwSession < CSLHwMaxNumSessions)
            {
                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                if ((CSLHwValidState == CSLHwGetSessionState(pSession)) && (hCSL == pSession->hSession))
                {
                    if (TRUE == pSession->bInFlush)
                    {
                        if ((TRUE == pSession->bInFlush) &&
                            ((NULL == pSession->flushInfo.phDevices) &&
                            (NULL == pSession->flushInfo.phSyncLink)))
                        {
                            result = CamxResultECancelledRequest;
                        }
                        else if ((TRUE == pSession->bInFlush) &&
                            ((NULL != pSession->flushInfo.phSyncLink) &&
                                 (hLink == *(pSession->flushInfo.phSyncLink))))
                        {
                            result = CamxResultECancelledRequest;
                        }
                        else
                        {
                            if ((TRUE == pSession->bInFlush) &&
                                (requestId <= pSession->flushInfo.lastCSLSyncId) &&
                                (CSLSyncLinkModeSync == syncMode))
                            {
                                syncMode = CSLSyncLinkModeNoSync;
                                CAMX_LOG_INFO(CamxLogGroupCSL, "hLink 0x%x sync mode update to FALSE for CSLSyncID: %llu",
                                    hLink, requestId);
                            }

                            result = CSLHwScheduleRequest(pSession, hLink, requestId, bubble, syncMode, expectedExposureTime);
                        }
                    }
                    else
                    {
                        result = CSLHwScheduleRequest(pSession, hLink, requestId, bubble, syncMode, expectedExposureTime);
                    }
                }
                else if (CSLHwFlushState == CSLHwGetSessionState(pSession))
                {
                    result = CamxResultECancelledRequest;
                }
                else if ((CSLHwValidState != CSLHwGetSessionState(pSession)) &&
                         (CSLHwFlushState != CSLHwGetSessionState(pSession)))
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Open request for session = %d failed!", hCSL);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
                result = CamxResultEOutOfBounds;
            }
            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLCancelRequestHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CSLCancelRequestHW(
    CSLHandle           hCSL,
    const CSLFlushInfo& rCSLFlushInfo)
{
    CamxResult result = CamxResultEFailed;

    if (CSLInvalidHandle != hCSL)
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hCSLHwSession < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                if (TRUE == CSLHwSessionGetRefCount(pSession))
                {
                    if (hCSL == pSession->hSession)
                    {
                        result = CSLHwCancelRequest(pSession, rCSLFlushInfo);
                    }
                    else
                    {
                        CAMX_ASSERT_ALWAYS();
                    }
                    CSLHwSessionPutRefCount(pSession);
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLStreamOnHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLStreamOnHW(
    CSLHandle hCSL,
    CSLLinkHandle* phLink,
    CSLDeviceHandle* phDevices)
{
    CamxResult result = CamxResultEFailed;
    CAMX_UNREFERENCED_PARAM(phLink);

    if (CSLInvalidHandle != hCSL)
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hCSLHwSession < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                if (hCSL == pSession->hSession)
                {
                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        CamX::LightweightDoublyLinkedList* pList = NULL;
                        CSLDeactivateMode                  mode  = CSLDeativateModeDefault;
                        UINT32                             index = 0;
                        result = CamxResultSuccess;

                        for (index = 0; index < CSLHwMaxLinksArray; index++)
                        {
                            if (pSession->linkInfo[index].hLinkIdentifier == *phLink)
                            {
                                mode = pSession->linkInfo[index].currentMode;
                                break;
                            }
                        }

                        pSession->lock->Lock();

                        if ((CSLInvalidHandle != *phLink) && (CSLHwMaxLinksArray > index))
                        {
                            // Enable CRM Link control before stream on, will be disabled during stream off
                            if (CamxResultSuccess == result)
                            {
                                result = CSLHwLinkControl(pSession, *phLink, CAM_REQ_MGR_LINK_ACTIVATE);
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCSL, "CRM stream on failed!");
                            }

                            if (CamxResultSuccess == result)
                            {
                                pSession->linkInfo[index].isActive = TRUE;
                                CAMX_LOG_INFO(CamxLogGroupCSL, "mode=%d, index=%d, isActive=%d, link=%d",
                                    mode, index, pSession->linkInfo[index].isActive, *phLink);
                            }

                            // First trigger a real time devices stream on
                            pList = &pSession->rtList;

                            // First sort the real time devices in order
                            pList->Sort(CSLHwRtDeviceOderCompare);

                            if (CamxResultSuccess == result)
                            {
                                result = CSLHwStreamOnKMDHardwares(hCSLHwSession, pList, phDevices, mode);
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCSL, "StreamOn call dropped due to error!");
                            }
                        }

                        if (CamxResultSuccess == result)
                        {
                            pList = &pSession->nrtList;

                            // Now trigger Non realtime devices stream on but move this to a job as they are not inter dependent
                            // Actually first schedule job for NRT devices stream on and then start RT devices as optimization.
                            // Check if the nrt list has devices that are acquired before calling stream on
                            BOOL                  callStreamOn       = FALSE;
                            CSLHwAcquiredDevice*  pAcquiredDevice    = NULL;
                            CamX::LDLLNode*       pNode              = pList->Head();

                            while (NULL != pNode)
                            {
                                pAcquiredDevice = (static_cast<CSLHwAcquiredDevice*>(pNode->pData));
                                for (UINT i = 0; i < CamxMaxDeviceIndex; i++)
                                {
                                    if (phDevices[i] == pAcquiredDevice->hAcquired)
                                    {
                                        callStreamOn = TRUE;
                                        break;
                                    }
                                }

                                if (TRUE == callStreamOn)
                                {
                                    break;
                                }
                                pNode = CamX::LightweightDoublyLinkedList::NextNode(pNode);
                            }

                            if (TRUE == callStreamOn)
                            {
                                result = CSLHwStreamOnKMDHardwares(hCSLHwSession, pList, phDevices, mode);

                                if (CamxResultSuccess == result)
                                {
                                    pSession->streamOn = TRUE;
                                }
                                else
                                {
                                    // Now stream off realtime devices too
                                    result = CSLHwStreamOffKMDHardwares(hCSLHwSession, &pSession->rtList, phDevices, mode);
                                    CSLHwLinkControl(pSession, *phLink, CAM_REQ_MGR_LINK_DEACTIVATE);
                                    pSession->streamOn = FALSE;
                                }
                            }
                        }
                        else
                        {
                            CSLHwLinkControl(pSession, *phLink, CAM_REQ_MGR_LINK_DEACTIVATE);
                            pSession->streamOn = FALSE;
                        }

                        pSession->lock->Unlock();
                        CSLHwSessionPutRefCount(pSession);
                    }
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLStreamOffHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLStreamOffHW(
    CSLHandle         hCSL,
    CSLLinkHandle*    phLink,
    CSLDeviceHandle*  phDevices,
    CSLDeactivateMode mode)
{
    CamxResult result        = CamxResultEFailed;
    CamxResult resultRetCode = CamxResultSuccess;

    if (CSLInvalidHandle != hCSL)
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hCSLHwSession < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

                if (hCSL == pSession->hSession)
                {
                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        pSession->lock->Lock();
                        CamX::LightweightDoublyLinkedList* pList = NULL;
                        result            = CamxResultSuccess;

                        if (CSLInvalidHandle != *phLink)
                        {
                            // First trigger a real time devices stream on
                            pList = &pSession->rtList;
                            // First sort the real time devices in order
                            pList->Sort(CSLHwRtDeviceStreamoffOrderCompare);
                            result = CSLHwStreamOffKMDHardwares(hCSLHwSession, pList, phDevices, mode);

                            // Disable CRM link control on stream off
                            if (CamxResultSuccess == result)
                            {
                                result = CSLHwLinkControl(pSession, *phLink, CAM_REQ_MGR_LINK_DEACTIVATE);
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupCSL, "CRM stream off failed!");

                                // Deactivate the link anyway even though stream off has failed
                                CSLHwLinkControl(pSession, *phLink, CAM_REQ_MGR_LINK_DEACTIVATE);
                                resultRetCode = result;
                            }
                        }

                        pList  = &pSession->nrtList;
                        result = CSLHwStreamOffKMDHardwares(hCSLHwSession, pList, phDevices, mode);

                        pSession->streamOn = FALSE;
                        pSession->lock->Unlock();
                        CSLHwSessionPutRefCount(pSession);

                        for (UINT i = 0; i < CSLHwMaxLinksArray; i++)
                        {
                            if (pSession->linkInfo[i].hLinkIdentifier == *phLink)
                            {
                                CAMX_LOG_INFO(CamxLogGroupCSL, "mode=%d, index=%d, isActive=%d, link=%d",
                                    mode, i, pSession->linkInfo[i].isActive, *phLink);
                                pSession->linkInfo[i].currentMode = mode;
                                pSession->linkInfo[i].isActive    = FALSE;
                                break;
                            }
                        }
                    }
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CSLStreamOffHw failed. result: %d");
        resultRetCode = result;
    }

    return resultRetCode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLSingleDeviceStreamOnHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSingleDeviceStreamOnHW(
    CSLHandle           hCSL,
    INT32               deviceIndex,
    CSLDeviceHandle*    phDevice)
{
    CamxResult result = CamxResultEFailed;

    if (CSLInvalidHandle != hCSL)
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hCSLHwSession < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                if (hCSL == pSession->hSession)
                {
                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        CSLDeactivateMode mode = CSLDeativateModeDefault;
                        result = CSLHwSingleDeviceStreamOnKMDHardwares(hCSLHwSession,
                            deviceIndex,
                            phDevice,
                            mode);
                        CSLHwSessionPutRefCount(pSession);
                    }
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLSingleDeviceStreamOffHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSingleDeviceStreamOffHW(
    CSLHandle           hCSL,
    INT32               deviceIndex,
    CSLDeviceHandle*    phDevice)
{
    CamxResult result = CamxResultEFailed;

    if (CSLInvalidHandle != hCSL)
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hCSLHwSession < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

                if (hCSL == pSession->hSession)
                {
                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        CSLDeactivateMode mode = CSLDeativateModeDefault;
                        result = CSLHwSingleDeviceStreamOffKMDHardwares(hCSLHwSession,
                            deviceIndex,
                            phDevice,
                            mode);
                        CSLHwSessionPutRefCount(pSession);
                    }
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLSubmitHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLSubmitHW(
    CSLHandle       hCSL,
    CSLDeviceHandle hDevice,
    CSLMemHandle    hPacket,
    SIZE_T          offset)
{
    CamxResult result = CamxResultEFailed;

    if ((CSLInvalidHandle != hCSL) && (CSLInvalidHandle != hDevice))
    {
        CSLHandle hTempIndex = CSLInvalidHandle;

        hTempIndex = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hTempIndex < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hTempIndex];

                if (TRUE == pSession->bInFlush)
                {
                    if ((NULL == pSession->flushInfo.phDevices) &&
                        (NULL == pSession->flushInfo.phSyncLink))
                    {
                        CAMX_LOG_INFO(CamxLogGroupCSL,
                                      "SUBMIT failed as Flush is in Progress - Session: %d, Device: %d, Packet: %d",
                                      hCSL, hDevice, hPacket);
                        result = CamxResultECancelledRequest;
                    }
                    else if (NULL != pSession->flushInfo.phDevices)
                    {
                        INT32            numDevices = pSession->flushInfo.numDevices - 1;
                        CSLDeviceHandle* phDevices  = pSession->flushInfo.phDevices;

                        while (0 <= numDevices)
                        {
                            if (hDevice == phDevices[numDevices])
                            {
                                CAMX_LOG_INFO(CamxLogGroupCSL,
                                      "SUBMIT failed as Flush is in Progress - Session: %d, Device: %d, Packet: %d",
                                      hCSL, hDevice, hPacket);
                                result = CamxResultECancelledRequest;
                                break;
                            }
                            numDevices--;
                        }
                    }
                }

                if ((CamxResultECancelledRequest != result) && (TRUE == CSLHwSessionGetRefCount(pSession)))
                {
                    if (hCSL == (pSession->hSession))
                    {
                        CSLHwAcquiredDevice* pSubmitDevice = NULL;
                        CSLHwDevice*         pHWDevice     = NULL;

                        hTempIndex = CAM_REQ_MGR_GET_HDL_IDX(hDevice);
                        if (hTempIndex < CSLHwMaxNumAcquiredDevices)
                        {
                            pSubmitDevice = &pSession->sessionDevices[hTempIndex];
                            if (hDevice == pSubmitDevice->hAcquired)
                            {
                                pSubmitDevice->lock->Lock();
                                pHWDevice = &g_CSLHwInstance.CSLInternalKMDDevices[pSubmitDevice->cslDeviceIndex];
                                if (TRUE == CSLHwKMDDeviceGetRefcount(pHWDevice))
                                {
                                    if (NULL != pHWDevice->deviceOp.Submit)
                                    {
                                        result = pHWDevice->deviceOp.Submit(hCSL, hDevice, hPacket, offset,
                                                                            pSubmitDevice->cslDeviceIndex);
                                    }
                                    CSLHwKMDDevicePutRefcount(pHWDevice);
                                }
                                pSubmitDevice->lock->Unlock();
                            }
                            else
                            {
                                result = CamxResultEInvalidArg;
                                CAMX_ASSERT_ALWAYS();
                            }
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempIndex >= CSLHwMaxNumAcquiredDevices");
                            result = CamxResultEOutOfBounds;
                        }
                    }
                    else
                    {
                        result = CamxResultEInvalidArg;
                    }
                    // Release the session refcount for current API
                    CSLHwSessionPutRefCount(pSession);
                }

                if (CamxResultEBusy != result)
                {
                    CSLHwInstancePutRefCount();
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hTempIndex >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLFlushLockHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFlushLockHW(
    CSLHandle           hCSL,
    const CSLFlushInfo& rCSLFlushInfo)
{
    CamxResult result = CamxResultEInvalidArg;

    if (CSLInvalidHandle != hCSL)
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (CSLHwMaxNumSessions > hCSLHwSession)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                if (hCSL == pSession->hSession)
                {
                    pSession->bInFlush = TRUE;
                    CamX::Utils::Memcpy(&pSession->flushInfo, &rCSLFlushInfo, sizeof(CSLFlushInfo));

                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        result = CSLHwCancelRequest(pSession, rCSLFlushInfo);
                        CSLHwSessionPutRefCount(pSession);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %p != pSession->hSession: %p", hCSL, pSession->hSession);
                    result = CamxResultEInvalidArg;
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLFlushUnlockHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFlushUnlockHW(
    CSLHandle hCSL)
{
    CamxResult result = CamxResultEInvalidArg;

    if (CSLInvalidHandle != hCSL)
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (CSLHwMaxNumSessions > hCSLHwSession)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                if (hCSL == pSession->hSession)
                {
                    pSession->bInFlush = FALSE;
                    CamX::Utils::Memset(&pSession->flushInfo, 0, sizeof(CSLFlushInfo));
                    result = CamxResultSuccess;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %p != pSession->hSession: %p", hCSL, pSession->hSession);
                    result = CamxResultEInvalidArg;
                }

                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLRegisterMessageHandlerHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLRegisterMessageHandlerHW(
    CSLHandle           hCSL,
   CSLLinkHandle        hCSLLinkHandle,
    CSLMessageHandler   messageHandler,
    VOID*               pUserData)
{
    CamxResult result = CamxResultEFailed;

    if ((CSLInvalidHandle != hCSL) && (NULL != messageHandler) && (NULL != pUserData))
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hCSLHwSession < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                if (hCSL == pSession->hSession)
                {
                    // The Instance refcount is released only in the CSLReleaseDeviceHW if everything in this API is success
                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        for (UINT32 index = 0; index < CSLHwMaxNumSessions; index++)
                        {
                            if (pSession->CSLLinkHandleData[index].hCSLLinkHandle == 0)
                            {
                                pSession->CSLLinkHandleData[index].messageHandler = messageHandler;
                                pSession->CSLLinkHandleData[index].pMessageData = pUserData;
                                pSession->CSLLinkHandleData[index].hCSLLinkHandle = hCSLLinkHandle;
                                result                   = CamxResultSuccess;
                                break;
                            }
                        }
                        CSLHwSessionPutRefCount(pSession);
                    }
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLRegisterSessionMessageHandlerHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLRegisterSessionMessageHandlerHW(
    CSLHandle                  hCSL,
    CSLSessionMessageHandler   sesMessageHandler,
    VOID*                      pUserData)
{
    CamxResult result = CamxResultEFailed;

    if ((CSLInvalidHandle != hCSL) && (NULL != sesMessageHandler) && (NULL != pUserData))
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (hCSLHwSession < CSLHwMaxNumSessions)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                if (hCSL == pSession->hSession)
                {
                    // The Instance refcount is released only in the CSLReleaseDeviceHW if everything in this API is success
                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        pSession->sessionMessageHandler = sesMessageHandler;
                        pSession->pSessionMessageData   = pUserData;
                        result                          = CamxResultSuccess;
                        CSLHwSessionPutRefCount(pSession);
                    }
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLAllocHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLAllocHW(
    const CHAR*     pStr,
    CSLBufferInfo*  pBufferInfo,
    SIZE_T          bufferSize,
    SIZE_T          alignment,
    UINT32          CSLFlags,
    const INT32*    pDeviceIndices,
    UINT            deviceCount)
{

    CamxResult      result    = CamxResultSuccess;
    CSLHwDeviceOps* pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;

    if ((NULL != pBufferInfo) && (0 != bufferSize))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Passed arg validation!");
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            struct cam_mem_mgr_alloc_cmd allocCmd = {};
            allocCmd.len   = bufferSize;
            allocCmd.align = alignment;
            CAMX_LOG_VERBOSE(CamxLogGroupCSL,
                             "Allocate buffer: caller=%s size=%zu flags=%X UMD=%d KMD=%d CMD=%d HW=%d PKT=%d CACHE=%d PROT=%d",
                             pStr,
                             bufferSize,
                             CSLFlags,
                             (CSLFlags & CSLMemFlagUMDAccess),
                             (CSLFlags & CSLMemFlagKMDAccess),
                             (CSLFlags & CSLMemFlagCmdBuffer),
                             (CSLFlags & CSLMemFlagHw),
                             (CSLFlags & CSLMemFlagPacketBuffer),
                             (CSLFlags & CSLMemFlagCache),
                             (CSLFlags & CSLMemFlagProtected));
            /// @todo (CAMX-671): Add support for packet buffers
            if (CSLFlags & CSLMemFlagPacketBuffer)
            {
                CSLFlags &= ~CSLMemFlagPacketBuffer;
                CSLFlags |= CSLMemFlagCmdBuffer;
            }

            if ((CSLFlags & CSLMemFlagHw) ||
                (CSLFlags & CSLMemFlagProtected))
            {
                if ((NULL != pDeviceIndices) && (0 != deviceCount))
                {
                    result = CSLHwPopulateMMUHandles(allocCmd.mmu_hdls,
                                                     &allocCmd.num_hdl,
                                                     pDeviceIndices,
                                                     deviceCount,
                                                     CSLFlags);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL,
                                   "Invalid arg for HW read/write buffer: pDeviceIndices=%p, deviceCount=%u",
                                   pDeviceIndices, deviceCount);
                    result = CamxResultEInvalidArg;
                }
            }

            if (CSLFlags & CSLMemFlagCmdBuffer)
            {
                if ((NULL != pDeviceIndices) && (0 != deviceCount))
                {
                    UINT CDMDeviceCount = allocCmd.num_hdl;
                    result = CSLHwPopulateCDMMMUHandles(allocCmd.mmu_hdls,
                                                        &allocCmd.num_hdl,
                                                        pDeviceIndices,
                                                        deviceCount,
                                                        CSLFlags);
                    if (CDMDeviceCount < allocCmd.num_hdl)
                    {
                        allocCmd.flags |= CAM_MEM_FLAG_HW_READ_WRITE;
                    }
                }
            }
            if (CamxResultSuccess == result)
            {
                result = CSLHwMapCSLAllocFlagsToKMD(CSLFlags, &allocCmd.flags);
            }
            if (CamxResultSuccess == result)
            {
                // Call the actual IOCTL here
                g_CSLHwInstance.allocLock->Lock();
                result = pDeviceOp->Ioctl2(&g_CSLHwInstance.requestManager, CAM_REQ_MGR_ALLOC_BUF, &allocCmd,
                                           0, sizeof(allocCmd));
                g_CSLHwInstance.allocLock->Unlock();

                if (CamxResultSuccess == result)
                {
                    CSLBufferInfo* pLocalBuffInfo = NULL;

                    // This API adds an entry into CSL's internal data structures
                    // Release the memory if Add fails
                    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Got handle from kernel = %x fd %d len %d",
                                    allocCmd.out.buf_handle, allocCmd.out.fd, allocCmd.len);

                    g_CSLHwInstance.allocLock->Lock();
                    result = CSLHwAddBuffer(allocCmd.out.buf_handle,
                        allocCmd.out.fd,
                        allocCmd.len,
                        &pLocalBuffInfo,
                        CSLFlags,
                        FALSE);
                    g_CSLHwInstance.allocLock->Unlock();

                    // Now we do a deep copy
                    if (CamxResultSuccess == result)
                    {
                        CamX::Utils::Memcpy(pBufferInfo, pLocalBuffInfo, sizeof(*pLocalBuffInfo));
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "CSLHwAddBuffer() failed for hdl = %x",
                                        allocCmd.out.buf_handle);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "Allocating CSL Buffer failed for len = %llu", allocCmd.len);
                }
            }
            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLMapBufferHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMapBufferHW(
    CSLBufferInfo*  pBufferInfo,
    INT             bufferFD,
    SIZE_T          offset,
    SIZE_T          bufferLength,
    UINT32          CSLFlags,
    const INT32*    pDeviceIndices,
    UINT            deviceCount)
{
    CAMX_UNREFERENCED_PARAM(offset);

    CamxResult                  result = CamxResultSuccess;
    CSLHwDeviceOps*             pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;

    if ((0 != CSLFlags) && (NULL != pBufferInfo) && (0 != bufferLength) && (bufferFD >= 0))
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            struct cam_mem_mgr_map_cmd  mapCmd = {};
            if ((CSLFlags & CSLMemFlagHw) || (CSLFlags & CSLMemFlagProtected))
            {
                if ((NULL != pDeviceIndices) && (0 != deviceCount))
                {
                    result = CSLHwPopulateMMUHandles(mapCmd.mmu_hdls,
                                                     &mapCmd.num_hdl,
                                                     pDeviceIndices,
                                                     deviceCount,
                                                     CSLFlags);
                }
            }
            if (CSLFlags & CSLMemFlagCmdBuffer)
            {
                if ((NULL != pDeviceIndices) && (0 != deviceCount))
                {
                    UINT CDMDeviceCount = mapCmd.num_hdl;
                    result = CSLHwPopulateCDMMMUHandles(mapCmd.mmu_hdls,
                                                        &mapCmd.num_hdl,
                                                        pDeviceIndices,
                                                        deviceCount,
                                                        CSLFlags);
                    if (CDMDeviceCount < mapCmd.num_hdl)
                    {
                        mapCmd.flags |= CAM_MEM_FLAG_HW_READ_WRITE;
                    }
                }
            }
            if (CamxResultSuccess == result)
            {
                result = CSLHwMapCSLAllocFlagsToKMD(CSLFlags, &mapCmd.flags);
            }
            if (CamxResultSuccess == result)
            {
                CSLBufferInfo* pLocalBuffInfo = NULL;

                g_CSLHwInstance.allocLock->Lock();
                // Call the actual IOCTL here
                mapCmd.fd = bufferFD;
                result    = pDeviceOp->Ioctl2(&g_CSLHwInstance.requestManager, CAM_REQ_MGR_MAP_BUF,
                                                  &mapCmd, 0, sizeof(mapCmd));

                if (CamxResultSuccess == result)
                {
                    // This API adds an entry into CSL's internal data structures
                    result = CSLHwAddBuffer(mapCmd.out.buf_handle,
                        bufferFD,
                        bufferLength,
                        &pLocalBuffInfo,
                        CSLFlags,
                        TRUE);

                    // Now we do a deep copy
                    if (CamxResultSuccess == result)
                    {
                        CamX::Utils::Memcpy(pBufferInfo, pLocalBuffInfo, sizeof(*pLocalBuffInfo));
                        UINT idx = CAM_MEM_MGR_GET_HDL_IDX(mapCmd.out.buf_handle);
                        g_CSLHwInstance.memManager.bufferInfo[idx].refcount++;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "CSLHwAddBuffer failed, result = %d", result);
                    }
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupCSL, "Mapped buffer twice fd = %d", bufferFD);
                    UINT idx;
                    for (idx = 0; idx < CAM_MEM_BUFQ_MAX; idx++)
                    {
                        if ((NULL       != g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo)      &&
                            (bufferFD   == g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo->fd)  &&
                            (TRUE       == g_CSLHwInstance.memManager.bufferInfo[idx].isImported))
                        {
                            CAMX_LOG_INFO(CamxLogGroupCSL, "Returning existing buffer idx = %d", bufferFD);
                            pLocalBuffInfo = g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo;
                            break;
                        }
                    }
                    if (NULL != pLocalBuffInfo)
                    {
                        CamX::Utils::Memcpy(pBufferInfo, pLocalBuffInfo, sizeof(*pLocalBuffInfo));
                        g_CSLHwInstance.memManager.bufferInfo[idx].refcount++;
                        result = CamxResultSuccess;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "Mapping CSL Buffer failed for fd = %d", bufferFD);
                    }
                }
                g_CSLHwInstance.allocLock->Unlock();
            }
            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid Argument, CSLFlags=0x%x, pBufferInfo=%p, bufferLength=%d, bufferFD=%d",
                       CSLFlags, pBufferInfo, bufferLength, bufferFD);
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLMapNativeBufferHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMapNativeBufferHW(
    CSLBufferInfo*          pBufferInfo,
    const CSLNativeHandle*  phNativeBuffer,
    SIZE_T                  offset,
    SIZE_T                  bufferLength,
    UINT32                  CSLFlags,
    const INT32*            pDeviceIndices,
    UINT                    deviceCount)
{
    CamxResult                  result          = CamxResultSuccess;
    const CamX::NativeHandle*   phNativeHandle  = reinterpret_cast<const CamX::NativeHandle*>(phNativeBuffer);

    if (NULL == phNativeHandle)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid arg: native buffer = %p", phNativeHandle);
    }
    else
    {
        result = CSLMapBufferHW(pBufferInfo,
                                phNativeHandle->data[0],
                                offset,
                                bufferLength,
                                CSLFlags,
                                pDeviceIndices,
                                deviceCount);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLGetBufferInfoHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLGetBufferInfoHW(
    CSLMemHandle   hBuffer,
    CSLBufferInfo* pBufferInfo)
{
    INT32          idx;
    CamxResult     result     = CamxResultSuccess;

    if ((NULL != pBufferInfo) && (CSLInvalidHandle != hBuffer))
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            idx = CAM_MEM_MGR_GET_HDL_IDX(hBuffer);
            if ((idx >= 0) && (idx < CAM_MEM_BUFQ_MAX))
            {
                if ((CamxResultSuccess == result) &&
                    (NULL == g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo))
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "No buffer at idx = %d", idx);
                    result = CamxResultENoSuch;
                }
                else
                {
                    *pBufferInfo = *g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid idx = %d", idx);
                result = CamxResultEOutOfBounds;
            }
            CSLHwInstancePutRefCount();
        }
        else
        {
            result = CamxResultEFailed;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLBufferCacheOpHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLBufferCacheOpHW(
    CSLMemHandle hBuffer,
    BOOL         invalidate,
    BOOL         clean)
{
    CamxResult                   result    = CamxResultSuccess;
    CSLHwDeviceOps*              pDeviceOp = &g_CSLHwInstance.requestManager.deviceOp;

    if (TRUE == CSLHwInstanceGetRefCount())
    {
        struct cam_mem_cache_ops_cmd cacheCmd = {};
        cacheCmd.buf_handle = hBuffer;

        if ((TRUE == invalidate) && (TRUE == clean))
        {
            cacheCmd.mem_cache_ops = CAM_MEM_CLEAN_INV_CACHE;
        }

        if ((TRUE == invalidate) && (FALSE == clean))
        {
            cacheCmd.mem_cache_ops = CAM_MEM_INV_CACHE;
        }

        if ((FALSE == invalidate) && (TRUE == clean))
        {
            cacheCmd.mem_cache_ops = CAM_MEM_CLEAN_CACHE;
        }

        if ((FALSE == invalidate) && (FALSE == clean))
        {
            result = CamxResultEInvalidArg;
        }

        if (CamxResultSuccess == result)
        {
            result = pDeviceOp->Ioctl2(&g_CSLHwInstance.requestManager, CAM_REQ_MGR_CACHE_OPS, &cacheCmd,
                                       0, sizeof(cacheCmd));

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Cache ops (Inv = %d, Clean = %d) successful for handle = %u",
                                 invalidate, clean, hBuffer);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Cache ops (Inv = %d, Clean = %d) failed for handle = %u",
                               invalidate, clean, hBuffer);
            }
        }
        CSLHwInstancePutRefCount();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLReleaseBufferForcedHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseBufferForcedHW(
    CSLMemHandle hBuffer)
{
    INT32 idx;
    CSLHwMemBufferInfo* pBufferInfo = NULL;

    g_CSLHwInstance.lock->Lock();
    // free up that index from CSL internal tracking data structure
    idx = CAM_MEM_MGR_GET_HDL_IDX(hBuffer);

    if ((idx < 0) || (idx >= CAM_MEM_BUFQ_MAX))
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "index for CSL data structure invalid %d", idx);
    }
    else
    {
        // Call munmap() first for UMD access case
        pBufferInfo = &g_CSLHwInstance.memManager.bufferInfo[idx];
        if (NULL != pBufferInfo && 0 < pBufferInfo->refcount)
        {
            pBufferInfo->refcount = 0;
        }
    }
    g_CSLHwInstance.lock->Unlock();

    return CSLReleaseBuffer(hBuffer);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLReleaseBufferHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseBufferHW(
    CSLMemHandle hBuffer)
{
    CamxResult                     result = CamxResultSuccess;
    INT                            returnMemUnmap;
    CSLHwMemBufferInfo* pBufferInfo = NULL;

    if (TRUE == CSLHwInstanceGetRefCount())
    {
        g_CSLHwInstance.allocLock->Lock();
        INT32 idx;

        // free up that index from CSL internal tracking data structure
        idx = CAM_MEM_MGR_GET_HDL_IDX(hBuffer);

        if ((idx < 0) || (idx >= CAM_MEM_BUFQ_MAX))
        {
            result = CamxResultEOutOfBounds;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "index for CSL data structure invalid idx=%d, hBuffer=%d", idx, hBuffer);
        }
        else
        {
            // Call munmap() first for UMD access case
            pBufferInfo = &g_CSLHwInstance.memManager.bufferInfo[idx];
            if (NULL != pBufferInfo && 0 < pBufferInfo->refcount)
            {
                pBufferInfo->refcount--;
            }
            if ((NULL != pBufferInfo) && (0 >= pBufferInfo->refcount) && (NULL != pBufferInfo->pBufferInfo))
            {
                CAMX_LOG_INFO(CamxLogGroupCSL, "idx=%d, handle=%d, refCount=%d, fd=%d",
                              idx, hBuffer, pBufferInfo->refcount,
                              pBufferInfo->pBufferInfo->fd);

                if ((CSLMemFlagUMDAccess & g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo->flags) &&
                    (!(CSLMemFlagProtected & g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo->flags) ||
                    (!(CSLMemFlagProtectedUMDAccess & g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo->flags))))
                {
                    returnMemUnmap = CamX::OsUtils::MemUnmap(
                        g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo->pVirtualAddr,
                        g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo->size);
                    if (returnMemUnmap < 0)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCSL,
                                       "MemUnmap()failed for buffer with hdl = %u at idx = %d, ptr = %p with size = %zu",
                                       hBuffer,
                                       idx,
                                       g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo->pVirtualAddr,
                                       g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo->size);
                    }
                    else
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Successful MemUnmap!");
                    }
                }
                // Close the fd, but only if it was not imported
                if (FALSE == g_CSLHwInstance.memManager.bufferInfo[idx].isImported)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Not imported fd=%d, close file desc",
                                     g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo->fd);
                    close(g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo->fd);
                }
                CAMX_FREE(g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo);
                g_CSLHwInstance.memManager.bufferInfo[idx].pBufferInfo = NULL;
                g_CSLHwInstance.memManager.bufferInfo[idx].isImported  = FALSE;
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupCSL, "Still pending reference for buffer idx = %d ref cnt = %d",
                    idx, (NULL != pBufferInfo) ? pBufferInfo->refcount : -1);
            }
        }

        if ((NULL != pBufferInfo) && (0 >= pBufferInfo->refcount))
        {
            result = CSLHwReleaseBufferInKernel(hBuffer);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Releasing CSL Buffer failed for handle = %u", hBuffer);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Released CSL Buffer Successfully for handle = %u", hBuffer);
            }
        }
        g_CSLHwInstance.allocLock->Unlock();
        CSLHwInstancePutRefCount();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLCreatePrivateFenceHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCreatePrivateFenceHW(
    const CHAR* pName,
    CSLFence*   phFenceOut)
{
    CamxResult result = CamxResultEFailed;

    if (NULL != phFenceOut)
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            result = g_CSLHwInstance.pSyncFW->CreateSync(pName, phFenceOut);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Sync Creation failed: pName = %s", pName);
                for (UINT32 index = 0; index < CSLMaxFences; index++)
                {
                    CAMX_LOG_DUMP(CamxLogGroupCSL, "Valid Fence Table Info");
                    if (TRUE == g_CSLFenceInfo[index].isValid)
                    {
                        CAMX_LOG_DUMP(CamxLogGroupCSL, "Fence [%d] [%s]", index, g_CSLFenceInfo[index].fenceName);
                    }
                }
                *phFenceOut = CSLInvalidHandle;
                CamX::OsUtils::RaiseSignalAbort();
            }
            else
            {
                if (CSLMaxFences >*phFenceOut)
                {
                    g_CSLFenceInfo[*phFenceOut].isValid = TRUE;
                    CamX::OsUtils::SNPrintF(g_CSLFenceInfo[*phFenceOut].fenceName,
                        sizeof(g_CSLFenceInfo[*phFenceOut].fenceName), "%s",
                        pName);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "Fence ID outside normal bounds pName = %s fence ID:%d", pName, phFenceOut);
                }
            }
            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument(s): phFenceOut = %p, pName = %p", phFenceOut, pName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLCreateNativeFenceHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLCreateNativeFenceHW(
    CSLNativeFenceCreateDataPtr pCreateData,
    CSLFence*                   phFenceOut)
{
    CAMX_UNREFERENCED_PARAM(pCreateData);
    CAMX_UNREFERENCED_PARAM(phFenceOut);

    CAMX_NOT_IMPLEMENTED();
    return CamxResultEUnsupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLMergeFencesHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLMergeFencesHW(
    CSLFence* phFences,
    SIZE_T    fenceCount,
    CSLFence* phFenceOut)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != phFences) && (NULL != phFenceOut))
    {
        // Detect duplicate fences in the array passed from UMD
        for (UINT32 i = 0; i < fenceCount; i++)
        {
            for (UINT32 j = i + 1; j < fenceCount; j++)
            {
                if (phFences[i] == phFences[j])
                {
                    result = CamxResultEInvalidArg;
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument(s): duplicates detected in fence array");
                }
            }
        }
        if (CamxResultSuccess == result)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                result = g_CSLHwInstance.pSyncFW->Merge(phFences, fenceCount, phFenceOut);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "Sync Merge failed");
                    *phFenceOut = CSLInvalidHandle;
                }
                CSLHwInstancePutRefCount();
            }
            else
            {
                result = CamxResultEFailed;
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument(s): phFences = %p, phFenceOut = %p", phFences, phFenceOut);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLGetFenceAttribHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLGetFenceAttribHW(
    CSLFence hFence,
    UINT32   attrib,
    VOID*    pAttribVal,
    UINT32   valSize)
{
    CAMX_UNREFERENCED_PARAM(hFence);
    CAMX_UNREFERENCED_PARAM(attrib);
    CAMX_UNREFERENCED_PARAM(pAttribVal);
    CAMX_UNREFERENCED_PARAM(valSize);

    CAMX_NOT_IMPLEMENTED();
    return CamxResultEUnsupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLFenceWaitHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceWaitHW(
    CSLFence    hFence,
    UINT64      timeout)
{
    CamxResult result = CamxResultSuccess;

    if (TRUE == CSLHwInstanceGetRefCount())
    {
        result = g_CSLHwInstance.pSyncFW->Wait(hFence, timeout);
        CSLHwInstancePutRefCount();
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLFenceWaitMultipleHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceWaitMultipleHW(
    CSLFence*   phFences,
    BOOL*       pFenceSignaled,
    SIZE_T      fenceCount,
    UINT64      timeout)
{
    CAMX_UNREFERENCED_PARAM(phFences);
    CAMX_UNREFERENCED_PARAM(pFenceSignaled);
    CAMX_UNREFERENCED_PARAM(fenceCount);
    CAMX_UNREFERENCED_PARAM(timeout);

    CAMX_NOT_IMPLEMENTED();
    return CamxResultEUnsupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLFenceAsyncWaitHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceAsyncWaitHW(
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData)
{
    CamxResult result = CamxResultEFailed;

    if ((NULL != pUserData) && (NULL != handler) && (CSLInvalidHandle != hFence))
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            result = g_CSLHwInstance.pSyncFW->RegisterCallback(hFence, handler, pUserData);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Sync RegisterCallback() failed");
            }
            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument(s): pUserData %p handler %p hFence %d",
                       pUserData, handler, hFence);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLFenceAsyncCancelHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceAsyncCancelHW(
    CSLFence        hFence,
    CSLFenceHandler handler,
    VOID*           pUserData)
{

    CamxResult result = CamxResultEFailed;

    if ((NULL != pUserData) && (NULL != handler) && (CSLInvalidHandle != hFence))
    {
        if (TRUE == CSLHwInstanceGetRefCount())
        {
            result = g_CSLHwInstance.pSyncFW->DeregisterCallback(hFence, handler, pUserData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Sync DeregisterCallback() failed");
            }
            CSLHwInstancePutRefCount();
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid argument(s): pUserData %p handler %p hFence %d",
                       pUserData, handler, hFence);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLFenceSignalHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLFenceSignalHW(
    CSLFence        hFence,
    CSLFenceResult  fenceResult)
{
    CamxResult result = CamxResultEFailed;
    CamX::SyncSignalResult signalResult;

    if (TRUE == CSLHwInstanceGetRefCount())
    {
        if (CSLFenceResultSuccess == fenceResult)
        {
            signalResult = CamX::SyncSignalSuccess;
        }
        else
        {
            signalResult = CamX::SyncSignalFailed;
        }

        result = g_CSLHwInstance.pSyncFW->Signal(hFence, signalResult);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Sync Signal() failed: hFence:%d result:%d", hFence, result);
        }
        CSLHwInstancePutRefCount();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLDumpRequestHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLDumpRequestHW(
    CSLHandle              hCSL,
    CSLDumpRequestInfo*    pDumpRequestInfo,
    SIZE_T*                pFilledLength)
{
    CamxResult result = CamxResultEInvalidArg;

    if ((CSLInvalidHandle != hCSL) && (NULL != pDumpRequestInfo) && (NULL != pFilledLength))
    {
        CSLHandle hCSLHwSession = CSLInvalidHandle;

        hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
        if (CSLHwMaxNumSessions > hCSLHwSession)
        {
            if (TRUE == CSLHwInstanceGetRefCount())
            {
                CSLHwsession* pSession = NULL;

                pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];
                if (hCSL == pSession->hSession)
                {
                    if (TRUE == CSLHwSessionGetRefCount(pSession))
                    {
                        result = CSLHwDumpRequest(pSession, pDumpRequestInfo, pFilledLength);
                        CSLHwSessionPutRefCount(pSession);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSL: %p != pSession->hSession: %p", hCSL, pSession->hSession);
                }
                CSLHwInstancePutRefCount();
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "hCSLHwSession >= CSLHwMaxNumSessions");
            result = CamxResultEOutOfBounds;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid parameters: hCSL: %u pDumpRequestInfo %p pFilledLength %p",
            hCSL, pDumpRequestInfo, pFilledLength);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLReleaseFenceHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLReleaseFenceHW(
    CSLFence hFence)
{
    CamxResult result = CamxResultEFailed;

    if (TRUE == CSLHwInstanceGetRefCount())
    {
        result = g_CSLHwInstance.pSyncFW->DestroySync(hFence);
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Sync Destroy requested: hFence:%d result:%d", hFence, result);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Sync Destroy failed: hFence:%d result:%d", hFence, result);
        }
        else
        {
            if (CSLMaxFences > hFence)
            {
                g_CSLFenceInfo[hFence].isValid = FALSE;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "Fence ID outside normal bounds fence ID:%d", hFence);
            }
        }

        CSLHwInstancePutRefCount();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Jump table initialization section
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief  Define the jump table for CSL IFH mode
CSLJumpTable g_CSLjumpTableHW =
{
    CSLInitializeHW,
    CSLUninitializeHW,
    CSLOpenHW,
    CSLCloseHW,
    CSLAddReferenceHW,
    CSLQueryCameraPlatformHW,
    CSLImageSensorProbeHW,
    CSLSetDebugBufferHW,
    CSLEnumerateDevicesHW,
    CSLQueryDeviceCapabilitiesHW,
    CSLAcquireDeviceHW,
    CSLReleaseDeviceHW,
    CSLLinkHW,
    CSLUnlinkHW,
    CSLSyncLinksHW,
    CSLOpenRequestHW,
    CSLCancelRequestHW,
    CSLStreamOnHW,
    CSLStreamOffHW,
    CSLSingleDeviceStreamOnHW,
    CSLSingleDeviceStreamOffHW,
    CSLSubmitHW,
    CSLFlushLockHW,
    CSLFlushUnlockHW,
    CSLRegisterMessageHandlerHW,
    CSLRegisterSessionMessageHandlerHW,
    CSLAllocHW,
    CSLMapBufferHW,
    CSLMapNativeBufferHW,
    CSLGetBufferInfoHW,
    CSLBufferCacheOpHW,
    CSLReleaseBufferHW,
    CSLReleaseBufferForcedHW,
    CSLCreatePrivateFenceHW,
    CSLCreateNativeFenceHW,
    CSLMergeFencesHW,
    CSLGetFenceAttribHW,
    CSLFenceWaitHW,
    CSLFenceWaitMultipleHW,
    CSLFenceAsyncWaitHW,
    CSLFenceAsyncCancelHW,
    CSLFenceSignalHW,
    CSLReleaseFenceHW,
    CSLAcquireHardwareHW,
    CSLReleaseHardwareHW,
    CSLDumpRequestHW
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetCSLJumpTableHw
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSLJumpTable* GetCSLJumpTableHw()
{
    return &g_CSLjumpTableHW;
}

#endif // ANDROID
