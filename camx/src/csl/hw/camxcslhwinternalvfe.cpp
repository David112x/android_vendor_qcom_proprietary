////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternalvfe.cpp
///
/// @brief CamxCSL HW Internal implements for VFE hw csl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <media/cam_defs.h>
#include <media/cam_isp.h>
#include <media/cam_isp_vfe.h>

#include "camxcslhwinternal.h"
#include "camxmem.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwVFEKmdQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwVFEKmdQueryCap(
    CSLHandle   hDevice)
{
    CSLHwDevice*       pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[hDevice];
    INT32              status  = -1;
    CamxResult         result  = CamxResultEFailed;
    struct cam_control ioctlCmd;

    if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData = static_cast<struct cam_isp_query_cap_cmd *>
            (CAMX_CALLOC(sizeof(struct cam_isp_query_cap_cmd)));
        ioctlCmd.op_code        = CAM_QUERY_CAP;
        ioctlCmd.size           = sizeof(struct cam_isp_query_cap_cmd);
        ioctlCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved       = 0;
        ioctlCmd.handle         = CamX::Utils::VoidPtrToUINT64(pDevice->pKMDDeviceData);

        status = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (0 > status)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl failed for fd=%d hDevice %d", pDevice->fd, hDevice);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d hDevice %d", pDevice->fd, hDevice);
            pDevice->hMapIOMMU.hNonSecureIOMMU =
                (static_cast<struct cam_isp_query_cap_cmd *>(pDevice->pKMDDeviceData))->device_iommu.non_secure;
            pDevice->hMapIOMMU.hSecureIOMMU    =
                (static_cast<struct cam_isp_query_cap_cmd *>(pDevice->pKMDDeviceData))->device_iommu.secure;
            pDevice->hMapCDMIOMMU.hNonSecureIOMMU   = -1;
            pDevice->hMapCDMIOMMU.hSecureIOMMU      = -1;
            // For now no one in UMD interested in KMD data, So later update to the UMD data type properly
            result                             = CamxResultSuccess;
        }
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwVFEKmdAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwVFEKmdAcquire(
    CSLHandle          hCSL,
    CSLDeviceHandle*   phDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest,
    SIZE_T             numDeviceResources)
{
    struct cam_acquire_dev_cmd acquireCmd;
    SIZE_T                     loop;
    struct cam_control         ioctlCmd;
    CSLHwDevice*               pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult                 result  = CamxResultEFailed;
    struct cam_isp_resource*   pRes    = NULL;

    if (0 < numDeviceResources)
    {
        pRes = static_cast<struct cam_isp_resource*>(CAMX_CALLOC(numDeviceResources * sizeof(struct cam_isp_resource)));
        for (loop = 0; loop < numDeviceResources; loop++)
        {
            pRes[loop].resource_id = pDeviceResourceRequest[loop].resourceID;
            pRes[loop].handle_type = CAM_HANDLE_USER_POINTER;
            pRes[loop].res_hdl     = CamX::Utils::VoidPtrToUINT64(pDeviceResourceRequest[loop].pDeviceResourceParam);
            pRes[loop].length      = pDeviceResourceRequest[loop].deviceResourceParamSize;
        }
        // For now i am adding the acquire structure with only one cmd so later add as per UMD info
        acquireCmd.session_handle = hCSL;
        acquireCmd.num_resources  = numDeviceResources;
        acquireCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        acquireCmd.resource_hdl   = CamX::Utils::VoidPtrToUINT64(pRes);

        ioctlCmd.op_code     = CAM_ACQUIRE_DEV;
        ioctlCmd.size        = sizeof(struct cam_acquire_dev_cmd);
        ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved    = 0;
        ioctlCmd.handle      = CamX::Utils::VoidPtrToUINT64(&acquireCmd);
        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "Entering ioctl for fd=%d index %d", pDevice->fd, deviceIndex);
        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);

        if (CamxResultEFailed == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d index %d", pDevice->fd, deviceIndex);
            *phDevice = acquireCmd.dev_handle;
        }
        CAMX_FREE(pRes);
        pRes = NULL;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    return result;

}


CSLHwDeviceOps g_CSLHwDeviceVFEOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwVFEKmdQueryCap,
    CSLHwInternalDefaultUMDQueryCap,
    CSLHwVFEKmdAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
