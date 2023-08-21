////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternaljpeg.cpp
///
/// @brief CamxCSL HW Internal implements for JPEG hw csl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <media/cam_defs.h>
#include <media/cam_jpeg.h>

#include "camxcslhwinternal.h"
#include "camxmem.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwJPEGKmdQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwJPEGKmdQueryCap(
    CSLHandle   hDevice)
{
    CSLHwDevice*             pDevice        = &g_CSLHwInstance.CSLInternalKMDDevices[hDevice];
    CamxResult               result         = CamxResultSuccess;
    struct cam_control       ioctlCmd;
    struct cam_query_cap_cmd queryCapsCmd;

    if (NULL == pDevice)
    {
        return CamxResultEInvalidArg;
    }

    if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData =
            static_cast<struct cam_jpeg_query_cap_cmd*>(CAMX_CALLOC(sizeof(struct cam_jpeg_query_cap_cmd)));
        if (NULL == pDevice->pKMDDeviceData)
        {
            return CamxResultENoMemory;
        }

        queryCapsCmd.size        = sizeof(struct cam_jpeg_query_cap_cmd);
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
            CAMX_LOG_ERROR(CamxLogGroupCSL, "ioctl failed for fd=%d hDevice %d", pDevice->fd, hDevice);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d hDevice %d", pDevice->fd, hDevice);

            pDevice->hMapIOMMU.hNonSecureIOMMU    =
                (static_cast<struct cam_jpeg_query_cap_cmd*>(pDevice->pKMDDeviceData))->dev_iommu_handle.non_secure;
            pDevice->hMapIOMMU.hSecureIOMMU       =
                (static_cast<struct cam_jpeg_query_cap_cmd*>(pDevice->pKMDDeviceData))->dev_iommu_handle.secure;
            pDevice->hMapCDMIOMMU.hNonSecureIOMMU =
                (static_cast<struct cam_jpeg_query_cap_cmd *>(pDevice->pKMDDeviceData))->cdm_iommu_handle.non_secure;
            pDevice->hMapCDMIOMMU.hSecureIOMMU    =
                (static_cast<struct cam_jpeg_query_cap_cmd *>(pDevice->pKMDDeviceData))->cdm_iommu_handle.secure;
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "JPEG IOMMU handle nsecure=%d secure %d",
               pDevice->hMapIOMMU.hNonSecureIOMMU, pDevice->hMapIOMMU.hSecureIOMMU);

            /// @todo (CAMX-1263): For now no one in UMD interested in KMD data. Later update to the UMD data type properly.
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwJPEGKMDAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwJPEGKMDAcquire(
    CSLHandle          hCSL,
    CSLDeviceHandle*   phDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest,
    SIZE_T             numDeviceResources)
{
    struct cam_acquire_dev_cmd  acquireCmd;
    SIZE_T                      loop          = 0;
    struct cam_control          ioctlCmd;
    CSLHwDevice*                pDevice       = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*               pSession      = NULL;
    CSLHandle                   hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    CamxResult                  result        = CamxResultSuccess;

    CAMX_ASSERT(CSLHwMaxNumSessions > hCSLHwSession);
    pSession      = &g_CSLHwInstance.sessionList[hCSLHwSession];

    if (NULL == pDevice)
    {
        return CamxResultEInvalidArg;
    }

    acquireCmd.session_handle = pSession->hSession;
    acquireCmd.num_resources  = numDeviceResources;
    acquireCmd.handle_type    = CAM_HANDLE_USER_POINTER;
    acquireCmd.resource_hdl   = CamX::Utils::VoidPtrToUINT64(pDeviceResourceRequest[loop].pDeviceResourceParam);

    ioctlCmd.op_code          = CAM_ACQUIRE_DEV;
    ioctlCmd.size             = sizeof(cam_acquire_dev_cmd);
    ioctlCmd.handle_type      = CAM_HANDLE_USER_POINTER;
    ioctlCmd.reserved         = 0;
    ioctlCmd.handle           = CamX::Utils::VoidPtrToUINT64(&acquireCmd);

    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering ioctl for fd=%d, index %d", pDevice->fd, deviceIndex);

    result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "ioctl failed for fd=%d, index %d", pDevice->fd, deviceIndex);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d, index %d %d",
            pDevice->fd, deviceIndex, acquireCmd.dev_handle);
        *phDevice = acquireCmd.dev_handle;
    }

    return result;
}

CSLHwDeviceOps g_CSLHwDeviceJPEGOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwJPEGKmdQueryCap,
    CSLHwInternalDefaultUMDQueryCap,
    CSLHwJPEGKMDAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
