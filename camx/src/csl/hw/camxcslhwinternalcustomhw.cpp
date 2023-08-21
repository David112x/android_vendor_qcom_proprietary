////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternalcustomhw.cpp
///
/// @brief CamxCSL HW Internal implements for Custom hw csl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

// NOWHINE FILE PR007b: Whiner incorrectly concludes as non-library files
#include <media/cam_defs.h>
#include <media/cam_custom.h>

#include "camxcslhwinternal.h"
#include "camxmem.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwCustomHWKmdQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwCustomHWKmdQueryCap(
    CSLHandle   hDevice)
{
    CSLHwDevice*             pDevice        = &g_CSLHwInstance.CSLInternalKMDDevices[hDevice];
    CamxResult               result         = CamxResultSuccess;
    struct cam_control       ioctlCmd;
    struct cam_query_cap_cmd queryCapsCmd;

    if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData =
           static_cast<struct cam_custom_query_cap_cmd*>(CAMX_CALLOC(sizeof(struct cam_custom_query_cap_cmd)));

        queryCapsCmd.size        = sizeof(struct cam_custom_query_cap_cmd);
        queryCapsCmd.handle_type = CAM_HANDLE_USER_POINTER;
        queryCapsCmd.caps_handle = CamX::Utils::VoidPtrToUINT64(pDevice->pKMDDeviceData);

        ioctlCmd.op_code     = CAM_QUERY_CAP;
        ioctlCmd.size        = sizeof(struct cam_query_cap_cmd);
        ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved    = 0;
        ioctlCmd.handle      = CamX::Utils::VoidPtrToUINT64(&queryCapsCmd);

        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl failed for fd=%d hDevice %d", pDevice->fd, hDevice);
        }
        else
        {

            pDevice->hMapIOMMU.hNonSecureIOMMU    =
                (static_cast<struct cam_custom_query_cap_cmd*>(pDevice->pKMDDeviceData))->device_iommu.non_secure;
            pDevice->hMapIOMMU.hSecureIOMMU       =
                (static_cast<struct cam_custom_query_cap_cmd*>(pDevice->pKMDDeviceData))->device_iommu.secure;
            pDevice->hMapCDMIOMMU.hNonSecureIOMMU = -1;
            pDevice->hMapCDMIOMMU.hSecureIOMMU    = -1;

            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "CustomHW IOMMU handle nsecure=%d secure %d",
               pDevice->hMapIOMMU.hNonSecureIOMMU, pDevice->hMapIOMMU.hSecureIOMMU);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwCustomHWKmdAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwCustomHWKmdAcquire(
    CSLHandle          hCSL,
    CSLDeviceHandle*   phDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest,
    SIZE_T             numDeviceResources)
{
    struct cam_acquire_dev_cmd acquireCmd;
    SIZE_T                     loop;
    struct cam_control         ioctlCmd;
    CSLHwDevice*               pDevice          = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult                 result           = CamxResultSuccess;
    struct cam_custom_resource* pCustomResource = NULL;
    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering CustomHW Acquire for fd=%d index %d", pDevice->fd, deviceIndex);

    CAMX_UNREFERENCED_PARAM(pDeviceResourceRequest);
    CAMX_UNREFERENCED_PARAM(numDeviceResources);

    acquireCmd.session_handle  = hCSL;
    // num of resources will be CAM_API_COMPAT_CONSTANT to let kernel know to skip the hw resources acquiring
    acquireCmd.num_resources   = numDeviceResources;
    acquireCmd.handle_type     = CAM_HANDLE_USER_POINTER;
    acquireCmd.resource_hdl    = CamX::Utils::VoidPtrToUINT64(pCustomResource);

    ioctlCmd.op_code           = CAM_ACQUIRE_DEV;
    ioctlCmd.size              = sizeof(struct cam_acquire_dev_cmd);
    ioctlCmd.handle_type       = CAM_HANDLE_USER_POINTER;
    ioctlCmd.reserved          = 0;
    ioctlCmd.handle            = CamX::Utils::VoidPtrToUINT64(&acquireCmd);

    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering ioctl for fd=%d index %d", pDevice->fd, deviceIndex);
    result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, " ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
    }
    else
    {
        *phDevice = acquireCmd.dev_handle;
    }

    return result;
}

CSLHwDeviceOps g_CSLHwDeviceCustomHWOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwCustomHWKmdQueryCap,
    CSLHwInternalDefaultUMDQueryCap,
    CSLHwCustomHWKmdAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
