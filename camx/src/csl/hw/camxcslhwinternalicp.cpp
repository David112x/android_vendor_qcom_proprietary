////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternalicp.cpp
///
/// @brief CamxCSL HW Internal implements for ICP hw csl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <ctime>

#include <media/cam_icp.h>

#include "camxcsl.h"
#include "camxcslhwinternal.h"
#include "camxcslicpdefs.h"
#include "camxcsljumptable.h"
#include "camxincs.h"
#include "camxlist.h"
#include "camxmem.h"
#include "camxpacketdefs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHWICPKMDQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHWICPKMDQueryCap(
    CSLHandle hDevice)
{
    CamxResult                  result          = CamxResultSuccess;
    CSLHwDevice*                pDevice         = &g_CSLHwInstance.CSLInternalKMDDevices[hDevice];
    struct cam_control          ioctlCmd;
    struct cam_query_cap_cmd    queryCapsCmd;

    if (NULL == pDevice->pKMDDeviceData)
    {
        /// @todo  (CAMX-1278): Add for NULL check after this allocation and other places throughout
        pDevice->pKMDDeviceData =
            static_cast<struct cam_icp_query_cap_cmd*>(CAMX_CALLOC(sizeof(struct cam_icp_query_cap_cmd)));

        queryCapsCmd.size           = sizeof(struct cam_icp_query_cap_cmd);
        queryCapsCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        queryCapsCmd.caps_handle    = CamX::Utils::VoidPtrToUINT64(pDevice->pKMDDeviceData);

        ioctlCmd.op_code        = CAM_QUERY_CAP;
        ioctlCmd.size           = sizeof(cam_query_cap_cmd);
        ioctlCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved       = 0;
        ioctlCmd.handle         = CamX::Utils::VoidPtrToUINT64(&queryCapsCmd);

        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "ioctl failed for fd=%d, hDevice %d", pDevice->fd, hDevice);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d, hDevice %d", pDevice->fd, hDevice);

            pDevice->hMapIOMMU.hNonSecureIOMMU =
                (static_cast<struct cam_icp_query_cap_cmd*>(pDevice->pKMDDeviceData))->dev_iommu_handle.non_secure;
            pDevice->hMapIOMMU.hSecureIOMMU    =
                (static_cast<struct cam_icp_query_cap_cmd*>(pDevice->pKMDDeviceData))->dev_iommu_handle.secure;
            pDevice->hMapCDMIOMMU.hNonSecureIOMMU = -1;
            pDevice->hMapCDMIOMMU.hSecureIOMMU    = -1;
            // For now no one in UMD interested in KMD data, So later update to the UMD data type properly
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "IFE IOMMU handle nsecure=%d, secure %d",
                pDevice->hMapIOMMU.hNonSecureIOMMU, pDevice->hMapIOMMU.hSecureIOMMU);
        }
    }

    CAMX_LOG_VERBOSE(
        CamxLogGroupCSL, "IOMMU: %x %x\n",
        pDevice->hMapIOMMU.hNonSecureIOMMU,
        pDevice->hMapIOMMU.hSecureIOMMU);

    CAMX_LOG_VERBOSE(CamxLogGroupCSL, "CSLHWICPKMDQueryCap exit: %d", result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHWICPKMDAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHWICPKMDAcquire(
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

    if (hCSLHwSession < CSLHwMaxNumSessions)
    {
        pSession      = &g_CSLHwInstance.sessionList[hCSLHwSession];

        acquireCmd.session_handle = pSession->hSession;
        acquireCmd.num_resources  = numDeviceResources;
        acquireCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        acquireCmd.resource_hdl   = CamX::Utils::VoidPtrToUINT64(pDeviceResourceRequest[loop].pDeviceResourceParam);

        ioctlCmd.op_code          = CAM_ACQUIRE_DEV;
        ioctlCmd.size             = sizeof(cam_acquire_dev_cmd);
        ioctlCmd.handle_type      = CAM_HANDLE_USER_POINTER;
        /// @todo  (CAMX-1278): Initialize structures with 0 and then remove explicit assignments of 0
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
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultEOutOfBounds: hCSLHwSession = %d", hCSLHwSession);
        result = CamxResultEOutOfBounds;
    }

    return result;
}

CSLHwDeviceOps g_CSLHwDeviceICPOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    CSLHwInternalDefaultIoctl2,
    CSLHWICPKMDQueryCap,
    CSLHwInternalDefaultUMDQueryCap,
    CSLHWICPKMDAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
