////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternallrme.cpp
///
/// @brief CamxCSL HW Internal utils implements for LRME hw csl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <media/cam_defs.h>
#include <media/cam_lrme.h>

#include "camxcslhwinternal.h"

#include "camxmem.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwLRMEKmdQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwLRMEKmdQueryCap(
    CSLHandle   hDevice)
{
    CSLHwDevice*             pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[hDevice];
    CamxResult               result = CamxResultSuccess;
    struct cam_control       ioctlCmd;
    struct cam_query_cap_cmd queryCapsCmd;

    if (NULL == pDevice)
    {
        result = CamxResultEInvalidArg;
    }
    else if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData =
            static_cast<struct cam_lrme_query_cap_cmd*>(CAMX_CALLOC(sizeof(struct cam_lrme_query_cap_cmd)));

        if (NULL == pDevice->pKMDDeviceData)
        {
            result = CamxResultENoMemory;
        }
        else
        {
            memset(pDevice->pKMDDeviceData, 0, sizeof(struct cam_lrme_query_cap_cmd));
            queryCapsCmd.size = sizeof(struct cam_lrme_query_cap_cmd);
            queryCapsCmd.handle_type = CAM_HANDLE_USER_POINTER;
            queryCapsCmd.caps_handle = CamX::Utils::VoidPtrToUINT64(pDevice->pKMDDeviceData);

            ioctlCmd.op_code = CAM_QUERY_CAP;
            ioctlCmd.size = sizeof(struct cam_query_cap_cmd);
            ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
            ioctlCmd.reserved = 0;
            ioctlCmd.handle = CamX::Utils::VoidPtrToUINT64(&queryCapsCmd);

            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "LRME caps handle %p, %llx",
                pDevice->pKMDDeviceData, queryCapsCmd.caps_handle);

            result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "ioctl failed for fd=%d hDevice %d", pDevice->fd, hDevice);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d hDevice %d type %d",
                    pDevice->fd, hDevice, pDevice->deviceType);

                pDevice->hMapIOMMU.hNonSecureIOMMU =
                    (static_cast<struct cam_lrme_query_cap_cmd*>(pDevice->pKMDDeviceData))->device_iommu.non_secure;
                pDevice->hMapCDMIOMMU.hNonSecureIOMMU =
                    (static_cast<struct cam_lrme_query_cap_cmd*>(pDevice->pKMDDeviceData))->cdm_iommu.non_secure;

                CAMX_LOG_INFO(CamxLogGroupCSL, "LRME IOMMU handle nsecure=%x secure %x",
                    pDevice->hMapIOMMU.hNonSecureIOMMU, pDevice->hMapIOMMU.hSecureIOMMU);
                pDevice->hwVersion.majorVersion =
                    (static_cast<struct cam_lrme_query_cap_cmd *>(pDevice->pKMDDeviceData))->dev_caps[0].top_hw_version.rev;
                pDevice->hwVersion.minorVersion =
                    (static_cast<struct cam_lrme_query_cap_cmd *>(pDevice->pKMDDeviceData))->dev_caps[0].top_hw_version.gen;

                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "LRME hw version %d/%d",
                    pDevice->hwVersion.majorVersion, pDevice->hwVersion.minorVersion);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwLRMEKMDAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwLRMEKMDAcquire(
    CSLHandle          hCSL,
    CSLDeviceHandle*   phDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest,
    SIZE_T             numDeviceResources)
{
    struct cam_acquire_dev_cmd  acquireCmd;
    SIZE_T                      loop = 0;
    struct cam_control          ioctlCmd;
    CSLHwDevice*                pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*               pSession = NULL;
    CSLHandle                   hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    CamxResult                  result = CamxResultSuccess;

    if (CSLHwMaxNumSessions <= hCSLHwSession)
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Invalid hCSLHwSession");
        return CamxResultEInvalidArg;
    }
    pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

    if (NULL == pDevice)
    {
        return CamxResultEInvalidArg;
    }

    acquireCmd.session_handle = pSession->hSession;
    acquireCmd.num_resources = numDeviceResources;
    acquireCmd.handle_type = CAM_HANDLE_USER_POINTER;
    acquireCmd.resource_hdl = CamX::Utils::VoidPtrToUINT64(pDeviceResourceRequest[loop].pDeviceResourceParam);

    ioctlCmd.op_code = CAM_ACQUIRE_DEV;
    ioctlCmd.size = sizeof(cam_acquire_dev_cmd);
    ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
    ioctlCmd.reserved = 0;
    ioctlCmd.handle = CamX::Utils::VoidPtrToUINT64(&acquireCmd);

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

CSLHwDeviceOps g_CSLHwDeviceLRMEOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwLRMEKmdQueryCap,
    CSLHwInternalDefaultUMDQueryCap,
    CSLHwLRMEKMDAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
