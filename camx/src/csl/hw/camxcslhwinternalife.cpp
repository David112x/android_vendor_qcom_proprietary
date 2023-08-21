////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternalife.cpp
///
/// @brief CamxCSL HW Internal implements for IFE hw csl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <media/cam_defs.h>
#include <media/cam_isp.h>
#include <media/cam_isp_ife.h>

#include "camxcslhwinternal.h"
#include "camxmem.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwIFEKmdQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwIFEKmdQueryCap(
    CSLHandle   hDevice)
{
    CSLHwDevice*             pDevice        = &g_CSLHwInstance.CSLInternalKMDDevices[hDevice];
    CamxResult               result         = CamxResultSuccess;
    struct cam_control       ioctlCmd;
    struct cam_query_cap_cmd queryCapsCmd;

    if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData =
            static_cast<struct cam_isp_query_cap_cmd*>(CAMX_CALLOC(sizeof(struct cam_isp_query_cap_cmd)));

        queryCapsCmd.size        = sizeof(struct cam_isp_query_cap_cmd);
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
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d hDevice %d", pDevice->fd, hDevice);

            pDevice->hMapIOMMU.hNonSecureIOMMU    =
                (static_cast<struct cam_isp_query_cap_cmd*>(pDevice->pKMDDeviceData))->device_iommu.non_secure;
            pDevice->hMapIOMMU.hSecureIOMMU       =
                (static_cast<struct cam_isp_query_cap_cmd*>(pDevice->pKMDDeviceData))->device_iommu.secure;
            pDevice->hMapCDMIOMMU.hNonSecureIOMMU =
                (static_cast<struct cam_isp_query_cap_cmd *>(pDevice->pKMDDeviceData))->cdm_iommu.non_secure;
            pDevice->hMapCDMIOMMU.hSecureIOMMU    =
                (static_cast<struct cam_isp_query_cap_cmd *>(pDevice->pKMDDeviceData))->cdm_iommu.secure;
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "IFE IOMMU handle nsecure=%d secure %d",
               pDevice->hMapIOMMU.hNonSecureIOMMU, pDevice->hMapIOMMU.hSecureIOMMU);

            /// @todo (CAMX-1263): For now no one in UMD interested in KMD data. Later update to the UMD data type properly.
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwIFEKmdAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwIFEKmdAcquire(
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
    struct cam_isp_resource*   pISPResource     = NULL;
    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering IFE Acquire for fd=%d index %d", pDevice->fd, deviceIndex);

    CAMX_UNREFERENCED_PARAM(pDeviceResourceRequest);
    CAMX_UNREFERENCED_PARAM(numDeviceResources);

    acquireCmd.session_handle  = hCSL;
    // num of resources will be CAM_API_COMPAT_CONSTANT to let kernel know to skip the hw resources acquiring
    acquireCmd.num_resources   = CAM_API_COMPAT_CONSTANT;
    acquireCmd.handle_type     = CAM_HANDLE_USER_POINTER;
    acquireCmd.resource_hdl    = CamX::Utils::VoidPtrToUINT64(pISPResource);

    ioctlCmd.op_code            = CAM_ACQUIRE_DEV;
    ioctlCmd.size               = sizeof(struct cam_acquire_dev_cmd);
    ioctlCmd.handle_type        = CAM_HANDLE_USER_POINTER;
    ioctlCmd.reserved           = 0;
    ioctlCmd.handle             = CamX::Utils::VoidPtrToUINT64(&acquireCmd);

    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering ioctl for fd=%d index %d", pDevice->fd, deviceIndex);
    result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d index %d", pDevice->fd, deviceIndex);
        *phDevice = acquireCmd.dev_handle;
    }

    return result;
}

CSLHwDeviceOps g_CSLHwDeviceIFEOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwIFEKmdQueryCap,
    CSLHwInternalDefaultUMDQueryCap,
    CSLHwIFEKmdAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    CSLHwInternalKMDAcquireHardwareV2,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
