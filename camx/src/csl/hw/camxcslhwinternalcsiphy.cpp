////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternalcsiphy.cpp
///
/// @brief CamxCSL HW Internal implements for csiphy hw csl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <media/cam_defs.h>
#include <media/cam_sensor.h>

#include "camxcslhwinternal.h"
#include "camxcslsensordefs.h"
#include "camxmem.h"
#include "camxpacketdefs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwCSIPhyKmdQueryCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwCSIPhyKmdQueryCapability(
    CSLHandle   hIndex)
{
    CSLHwDevice*       pDevice  = &g_CSLHwInstance.CSLInternalKMDDevices[hIndex];
    INT32              status   = -1;
    CamxResult         result   = CamxResultEFailed;
    struct cam_control ioctlCmd;

    if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData = static_cast<cam_csiphy_query_cap *>(CAMX_CALLOC(sizeof(cam_csiphy_query_cap)));
        ioctlCmd.op_code     = CAM_QUERY_CAP;
        ioctlCmd.size        = sizeof(cam_csiphy_query_cap);
        ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved    = 0;
        ioctlCmd.handle      = CamX::Utils::VoidPtrToUINT64(pDevice->pKMDDeviceData);

        status = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (0 > status)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl failed for fd=%d hIndex %d", pDevice->fd, hIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL,
                             "ioctl success for fd=%d hIndex %d slot_info %d",
                             pDevice->fd,
                             hIndex,
                             static_cast<cam_csiphy_query_cap*>(pDevice->pKMDDeviceData)->slot_info);
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
// CSLHwCsiPhyFindSlot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwCsiPhyFindSlot(
    CSLHandle*    phIndex,
    UINT16        slot)
{
    CSLHwDevice* pDevice = NULL;
    UINT         i       = 0;
    CamxResult   result  = CamxResultEFailed;

    for (i = 0; i < g_CSLHwInstance.sensorSlotDeviceCount ; i++)
    {
        pDevice = &g_CSLHwInstance.CSLHwSensorSlotDevices[i];
        if (slot == (static_cast<cam_csiphy_query_cap *>(pDevice->pKMDDeviceData))->slot_info)
        {
            *phIndex = i;
            result   = CamxResultSuccess;
            break;
        }
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwCsiPhyKmdAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwCsiPhyKmdAcquire(
    CSLHandle          hCSL,
    CSLDeviceHandle*   phDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest,
    SIZE_T             numDeviceResources)
{
    CAMX_UNREFERENCED_PARAM(numDeviceResources);

    struct cam_sensor_acquire_dev*      pAcquireCmd = NULL;
    struct cam_control                  ioctlCmd;
    CSLHwDevice*                        pDevice    = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*                       pSession   = NULL;
    CSLHandle                           hTempIndex = CSLInvalidHandle;
    CamxResult                          result     = CamxResultEFailed;

    hTempIndex = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    if (hTempIndex < CSLHwMaxNumSessions)
    {
        pSession = &g_CSLHwInstance.sessionList[hTempIndex];

        pAcquireCmd                 = reinterpret_cast<struct cam_sensor_acquire_dev *>
                                        (CAMX_CALLOC(sizeof(struct cam_sensor_acquire_dev)));
        if (NULL != pAcquireCmd)
        {
            pAcquireCmd->session_handle = hCSL;
            pAcquireCmd->device_handle  = CSLInvalidHandle;
            pAcquireCmd->reserved       = 0;
            pAcquireCmd->handle_type    = CAM_HANDLE_USER_POINTER;
            pAcquireCmd->info_handle    = CamX::Utils::VoidPtrToUINT64(pDeviceResourceRequest->pDeviceResourceParam);

            ioctlCmd.op_code     = CAM_ACQUIRE_DEV;
            ioctlCmd.size        = sizeof(struct cam_sensor_acquire_dev);
            ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
            ioctlCmd.reserved    = 0;
            ioctlCmd.handle      = CamX::Utils::VoidPtrToUINT64(pAcquireCmd);

            result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
            if (CamxResultEFailed == result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "ioctl failed for fd=%d Index %d", pDevice->fd, deviceIndex);
            }
            else
            {
                *phDevice = pAcquireCmd->device_handle;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultEOutOfBounds: hTempIndex = %d", hTempIndex);
            result = CamxResultEOutOfBounds;
        }

        CAMX_FREE(pAcquireCmd);
    }
    else
    {
        result = CamxResultEFailed;
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalCSIPhyUMDQueryCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalCSIPhyUMDQueryCapability(
    INT32   deviceIndex,
    VOID*   pDeviceData,
    SIZE_T  deviceDataSize)
{
    CSLHwDevice* pDevice  = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result   = CamxResultSuccess;

    if ((sizeof(CSLCSIPHYCapability) != deviceDataSize) || (NULL == pDevice->pKMDDeviceData))
    {
        result = CamxResultEFailed;
    }
    else
    {
        CamX::Utils::Memcpy(pDeviceData, pDevice->pKMDDeviceData, deviceDataSize);
    }
    return result;

}

CSLHwDeviceOps g_CSLHwDeviceCsiPhyOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwCSIPhyKmdQueryCapability,
    CSLHwInternalCSIPhyUMDQueryCapability,
    CSLHwCsiPhyKmdAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
