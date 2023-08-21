////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternalfd.cpp
///
/// @brief CamxCSL Hw Internal implements for FD Hw CSL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <media/cam_defs.h>
#include <media/cam_fd.h>

#include "camxcslhwinternal.h"
#include "camxmem.h"
#include "camxcslfddefs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLFDHWCaps definitions comparison with KMD definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_STATIC_ASSERT(sizeof(CSLFDHWCaps)                             == sizeof(struct cam_fd_hw_caps));
CAMX_STATIC_ASSERT(offsetof(CSLFDHWCaps, coreVesion)               == offsetof(struct cam_fd_hw_caps, core_version));
CAMX_STATIC_ASSERT(offsetof(CSLFDHWCaps, wrapperVersion)           == offsetof(struct cam_fd_hw_caps, wrapper_version));
CAMX_STATIC_ASSERT(offsetof(CSLFDHWCaps, rawResultsAvailable)      == offsetof(struct cam_fd_hw_caps, raw_results_available));
CAMX_STATIC_ASSERT(offsetof(CSLFDHWCaps, supportedModes)           == offsetof(struct cam_fd_hw_caps, supported_modes));


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwFDKMDQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwFDKMDQueryCap(
    CSLHandle   hDevice)
{
    CSLHwDevice*             pDevice        = &g_CSLHwInstance.CSLInternalKMDDevices[hDevice];
    CamxResult               result         = CamxResultSuccess;
    struct cam_control       ioctlCmd;
    struct cam_query_cap_cmd queryCapsCmd;

    if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData =
            static_cast<struct cam_fd_query_cap_cmd*>(CAMX_CALLOC(sizeof(struct cam_fd_query_cap_cmd)));
        if (NULL == pDevice->pKMDDeviceData)
        {
            result = CamxResultENoMemory;
        }

        if (CamxResultSuccess == result)
        {
            queryCapsCmd.size           = sizeof(struct cam_fd_query_cap_cmd);
            queryCapsCmd.handle_type    = CAM_HANDLE_USER_POINTER;

            queryCapsCmd.caps_handle =
                reinterpret_cast<__u64>(static_cast<struct cam_fd_query_cap_cmd*>(pDevice->pKMDDeviceData));

            ioctlCmd.op_code     = CAM_QUERY_CAP;
            ioctlCmd.size        = sizeof(struct cam_query_cap_cmd);
            ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
            ioctlCmd.reserved    = 0;
            ioctlCmd.handle      = reinterpret_cast<__u64>(&queryCapsCmd);

            result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "ioctl failed for fd=%d hDevice %d", pDevice->fd, hDevice);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d hDevice %d", pDevice->fd, hDevice);

                pDevice->hMapIOMMU.hNonSecureIOMMU    =
                    (static_cast<struct cam_fd_query_cap_cmd*>(pDevice->pKMDDeviceData))->device_iommu.non_secure;
                pDevice->hMapIOMMU.hSecureIOMMU       =
                    (static_cast<struct cam_fd_query_cap_cmd*>(pDevice->pKMDDeviceData))->device_iommu.secure;
                pDevice->hMapCDMIOMMU.hNonSecureIOMMU =
                    (static_cast<struct cam_fd_query_cap_cmd *>(pDevice->pKMDDeviceData))->cdm_iommu.non_secure;
                pDevice->hMapCDMIOMMU.hSecureIOMMU    =
                    (static_cast<struct cam_fd_query_cap_cmd *>(pDevice->pKMDDeviceData))->cdm_iommu.secure;

                CAMX_LOG_VERBOSE(CamxLogGroupCSL, "FD IOMMU handle nsecure=%d secure %d",
                    pDevice->hMapIOMMU.hNonSecureIOMMU, pDevice->hMapIOMMU.hSecureIOMMU);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CSLHwFDKMDAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwFDKMDAcquire(
    CSLHandle          hCSL,
    CSLDeviceHandle*   phDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest,
    SIZE_T             numDeviceResources)
{
    struct cam_acquire_dev_cmd  acquireCmd    = {};
    SIZE_T                      loop          = 0;
    struct cam_control          ioctlCmd      = {};
    CSLHwDevice*                pDevice       = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*               pSession      = NULL;
    CSLHandle                   hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    CamxResult                  result        = CamxResultSuccess;

    if (CSLHwMaxNumSessions <= hCSLHwSession)
    {
        CAMX_LOG_ERROR(CamxLogGroupNCS, "Invalid hCSLHwSession");
        return CamxResultEInvalidArg;
    }
    pSession      = &g_CSLHwInstance.sessionList[hCSLHwSession];

    acquireCmd.session_handle = pSession->hSession;
    acquireCmd.num_resources  = numDeviceResources;
    acquireCmd.handle_type    = CAM_HANDLE_USER_POINTER;
    acquireCmd.resource_hdl   = reinterpret_cast<uint64_t>(pDeviceResourceRequest[loop].pDeviceResourceParam);

    ioctlCmd.op_code          = CAM_ACQUIRE_DEV;
    ioctlCmd.size             = sizeof(cam_acquire_dev_cmd);
    ioctlCmd.handle_type      = CAM_HANDLE_USER_POINTER;
    ioctlCmd.handle           = reinterpret_cast<__u64>(&acquireCmd);

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwFDUMDQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwFDUMDQueryCap(
    INT32   deviceIndex,
    VOID*   pDeviceData,
    SIZE_T  deviceDataSize)
{
    CSLHwDevice* pDevice  = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result   = CamxResultSuccess;

    if ((sizeof(CSLFDHWCaps) != deviceDataSize) || (NULL == pDevice->pKMDDeviceData))
    {
        result = CamxResultEFailed;
    }
    else
    {
        struct cam_fd_query_cap_cmd* pQueryCap = static_cast<struct cam_fd_query_cap_cmd*>(pDevice->pKMDDeviceData);

        CamX::Utils::Memcpy(pDeviceData, &pQueryCap->hw_caps, deviceDataSize);
    }

    return result;

}

CSLHwDeviceOps g_CSLHwDeviceFDOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwFDKMDQueryCap,
    CSLHwFDUMDQueryCap,
    CSLHwFDKMDAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
