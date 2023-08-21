////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternaleeprom.cpp
///
/// @brief CamxCSL HW Internal implementation for EEPROM hw csl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <media/cam_defs.h>
#include <media/cam_sensor.h>

#include "camxcslhwinternal.h"
#include "camxcslsensordefs.h"
#include "camxmem.h"
#include "camxpacketdefs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwEEPROMKMDQueryCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwEEPROMKMDQueryCapability(
    CSLHandle   hIndex)
{
    CSLHwDevice*        pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[hIndex];
    CamxResult          result  = CamxResultEFailed;
    struct cam_control  ioctlCmd;

    if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData = static_cast<cam_eeprom_query_cap_t *>(CAMX_CALLOC(sizeof(cam_eeprom_query_cap_t)));
        ioctlCmd.op_code        = CAM_QUERY_CAP;
        ioctlCmd.size           = sizeof(cam_eeprom_query_cap_t);
        ioctlCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved       = 0;
        ioctlCmd.handle         = (reinterpret_cast<__u64>(static_cast<cam_eeprom_query_cap_t*>(pDevice->pKMDDeviceData)));

        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (CamxResultEFailed == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "CAM_QUERY_CAP failed for fd=%d hIndex %d", pDevice->fd, hIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "CAM_QUERY_CAP success for fd=%d hIndex %d", pDevice->fd, hIndex);
            // Sensor doesn't support iommu handles so defaulting it to -1 always.
            pDevice->hMapIOMMU.hNonSecureIOMMU      = -1;
            pDevice->hMapIOMMU.hSecureIOMMU         = -1;
            pDevice->hMapCDMIOMMU.hNonSecureIOMMU   = -1;
            pDevice->hMapCDMIOMMU.hSecureIOMMU      = -1;
            // For now no one in UMD interested in KMD data, So later update to the UMD data type properly
            result = CamxResultSuccess;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwEepromKmdAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwEepromKmdAcquire(
    CSLHandle          hCSL,
    CSLDeviceHandle*   phDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest,
    SIZE_T             numDeviceResources)
{
    CAMX_UNREFERENCED_PARAM(pDeviceResourceRequest);
    CAMX_UNREFERENCED_PARAM(numDeviceResources);

    cam_sensor_acquire_dev* pAcquireCmd = NULL;
    CSLHwDevice*            pDevice     = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*           pSession    = NULL;
    CSLHandle               hTempIndex  = CSLInvalidHandle;
    CamxResult              result      = CamxResultEFailed;
    struct cam_control      ioctlCmd;

    hTempIndex = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    CAMX_ASSERT(CSLHwMaxNumSessions > hTempIndex);
    if (CSLHwMaxNumSessions <= hTempIndex)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid hTempIndex");
        return CamxResultEInvalidArg;
    }
    pSession = &g_CSLHwInstance.sessionList[hTempIndex];

    pAcquireCmd                 = reinterpret_cast<cam_sensor_acquire_dev *>(CAMX_CALLOC(sizeof(cam_sensor_acquire_dev)));
    if (NULL != pAcquireCmd)
    {
        pAcquireCmd->session_handle = hCSL;
        pAcquireCmd->device_handle  = CSLInvalidHandle;
        pAcquireCmd->reserved       = 0;
        pAcquireCmd->handle_type    = CAM_HANDLE_USER_POINTER;
        pAcquireCmd->info_handle    = 0;

        ioctlCmd.op_code        = CAM_ACQUIRE_DEV;
        ioctlCmd.size           = sizeof(cam_csiphy_acquire_dev_info);
        ioctlCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved       = 0;
        ioctlCmd.handle         = (reinterpret_cast<__u64>(pAcquireCmd));

        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (CamxResultEFailed == result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "CAM_ACQUIRE_DEV failed for fd=%d Index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "CAM_ACQUIRE_DEV success for fd=%d Index %d", pDevice->fd, deviceIndex);
            *phDevice = pAcquireCmd->device_handle;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "in eeprom acquire device_handle = %d", *phDevice);

        CAMX_FREE(pAcquireCmd);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalEEPROMUMDQueryCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 CamxResult CSLHwInternalEEPROMUMDQueryCapability(
    INT32   deviceIndex,
    VOID*   pDeviceData,
    SIZE_T  deviceDataSize)
{
    CSLHwDevice* pDevice    = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result     = CamxResultSuccess;

    if ((sizeof(CSLEEPROMCapability) != deviceDataSize) || (NULL == pDevice->pKMDDeviceData))
    {
        result = CamxResultEFailed;
    }
    else
    {
        CamX::Utils::Memcpy(pDeviceData, pDevice->pKMDDeviceData, deviceDataSize);
    }
    return result;
}

CSLHwDeviceOps g_CSLHwDeviceEepromOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwEEPROMKMDQueryCapability,
    CSLHwInternalEEPROMUMDQueryCapability,
    CSLHwEepromKmdAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
