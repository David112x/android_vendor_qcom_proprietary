////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternalflash.cpp
///
/// @brief CamxCSL HW Internal implements for VFE hw csl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <media/cam_defs.h>
#include <media/cam_sensor.h>

#include "camxcslhwinternal.h"
#include "camxmem.h"
#include "camxpacketdefs.h"
#include "camxcslsensordefs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwFlashKMDQueryCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwFlashKMDQueryCapability(
    CSLHandle   hIndex)
{
    CSLHwDevice*        pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[hIndex];
    CamxResult          result = CamxResultEFailed;
    struct cam_control  ioctlCmd;

    if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData = static_cast<cam_flash_query_cap_info *>(CAMX_CALLOC(sizeof(cam_flash_query_cap_info)));
        ioctlCmd.op_code        = CAM_QUERY_CAP;
        ioctlCmd.size           = sizeof(cam_flash_query_cap_info);
        ioctlCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved       = 0;
        ioctlCmd.handle         = CamX::Utils::VoidPtrToUINT64(pDevice->pKMDDeviceData);

        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (CamxResultEFailed == result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "CAM_QUERY_CAP failed for fd=%d hIndex %d", pDevice->fd, hIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "CAM_QUERY_CAP success for fd=%d hIndex %d", pDevice->fd, hIndex);
            // Flash doesn't support iommu handles so defaulting it to -1 always.
            pDevice->hMapIOMMU.hNonSecureIOMMU    = -1;
            pDevice->hMapIOMMU.hSecureIOMMU       = -1;
            pDevice->hMapCDMIOMMU.hNonSecureIOMMU = -1;
            pDevice->hMapCDMIOMMU.hSecureIOMMU    = -1;
            // For now no one in UMD interested in KMD data, So later update to the UMD data type properly
            result = CamxResultSuccess;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwFlashKMDAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwFlashKMDAcquire(
    CSLHandle          hCSL,
    CSLDeviceHandle*   phDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest,
    SIZE_T             numDeviceResources)
{
    CAMX_UNREFERENCED_PARAM(pDeviceResourceRequest);
    CAMX_UNREFERENCED_PARAM(numDeviceResources);

    cam_sensor_acquire_dev*  pAcquireCmd   = NULL;
    struct cam_control       ioctlCmd;
    CSLHwDevice*             pDevice       = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*            pSession      = NULL;
    CSLHandle                hCSLHwSession = CSLInvalidHandle;
    CamxResult               result        = CamxResultEFailed;

    hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    if (hCSLHwSession < CSLHwMaxNumSessions)
    {
        pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

        pAcquireCmd = reinterpret_cast<cam_sensor_acquire_dev *>(CAMX_CALLOC(sizeof(cam_sensor_acquire_dev)));
        if (NULL != pAcquireCmd)
        {
            pAcquireCmd->session_handle = hCSL;
            pAcquireCmd->device_handle  = CSLInvalidHandle;
            pAcquireCmd->reserved       = 0;
            pAcquireCmd->handle_type    = CAM_HANDLE_USER_POINTER;
            pAcquireCmd->info_handle    = 0;

            ioctlCmd.op_code     = CAM_ACQUIRE_DEV;
            ioctlCmd.size        = sizeof(cam_sensor_acquire_dev);
            ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
            ioctlCmd.reserved    = 0;
            ioctlCmd.handle      = CamX::Utils::VoidPtrToUINT64(pAcquireCmd);

            result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
            if (CamxResultEFailed == result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d index %d", pDevice->fd, deviceIndex);
                *phDevice = pAcquireCmd->device_handle;
            }
            CAMX_FREE(pAcquireCmd);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "pAcquireCmd is NULL");
            result = CamxResultEInvalidPointer;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultEOutOfBounds: sessionIndex = %d", hCSLHwSession);
        result = CamxResultEOutOfBounds;
    }

    return result;


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalFlashUMDQueryCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalFlashUMDQueryCapability(
    INT32   deviceIndex,
    VOID*   pDeviceData,
    SIZE_T  deviceDataSize)
{
    CSLHwDevice* pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result = CamxResultSuccess;

    if ((sizeof(CSLFlashQueryCapability) != deviceDataSize) || (NULL == pDevice->pKMDDeviceData))
    {
        result = CamxResultEFailed;
    }
    else
    {
        CamX::Utils::Memcpy(pDeviceData, pDevice->pKMDDeviceData, deviceDataSize);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalFlashKMDStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalFlashKMDStreamOn(
    CSLHandle           hCSLHwSession,
    CSLDeviceHandle     hDevice,
    INT32               deviceIndex,
    CSLDeactivateMode   mode)
{
    CSLHwDevice* pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result  = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(mode);

    if ((FALSE == (CSLDeativateModeSensorStandBy & mode)) &&
        (FALSE == (CSLDeativateModeSensorRealtimeDevices & mode)))
    {
        CAMX_LOG_INFO(CamxLogGroupCSL, "Try streaming on Sensor device index %d", deviceIndex);
        result = CSLHwInternalKMDStreamOp(hCSLHwSession, hDevice, deviceIndex, TRUE);
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupCSL, "Device fd %d stream on success index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Device stream on failed device fd %d index %d", pDevice->fd, deviceIndex);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalFlashKMDStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalFlashKMDStreamOff(
    CSLHandle           hCSLHwSession,
    CSLDeviceHandle     hDevice,
    INT32               deviceIndex,
    CSLDeactivateMode   mode)
{
    CSLHwDevice* pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result  = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(mode);

    if ((FALSE == (CSLDeativateModeSensorStandBy & mode)) &&
        (FALSE == (CSLDeativateModeSensorRealtimeDevices & mode)))
    {
        CAMX_LOG_INFO(CamxLogGroupCSL, "Try streaming off Sensor device index %d", deviceIndex);
        result = CSLHwInternalKMDStreamOp(hCSLHwSession, hDevice, deviceIndex, FALSE);
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupCSL, "Device fd %d stream off success index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Device stream off failed device fd %d index %d", pDevice->fd, deviceIndex);
        }
    }
    return result;
}

CSLHwDeviceOps g_CSLHwDeviceFlashOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwFlashKMDQueryCapability,
    CSLHwInternalFlashUMDQueryCapability,
    CSLHwFlashKMDAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalFlashKMDStreamOn,
    CSLHwInternalFlashKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
