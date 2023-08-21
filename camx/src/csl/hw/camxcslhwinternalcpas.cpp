////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternalcpas.cpp
///
/// @brief CamxCSL HW Internal implemention for CPAS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <media/cam_defs.h>
#include <media/cam_cpas.h>

#include "camxcslhwinternal.h"
#include "camxmem.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwKMDFamilyToCSLFamilyType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult CSLHwKMDFamilyToCSLFamilyType(
    UINT32            KMDFamilyId,
    CSLCameraFamily*  pFamilyType)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pFamilyType);

    switch (KMDFamilyId)
    {
        case CAM_FAMILY_CAMERA_SS:
            *pFamilyType = CSLCameraFamilyCameraSS;
            break;
        case CAM_FAMILY_CPAS_SS:
            *pFamilyType = CSLCameraFamilyTitan;
            break;
        default :
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupCSL, "Got an invalid KMDFamilyId=%d", KMDFamilyId);
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwCPASKMDQueryCap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwCPASKMDQueryCap(
    CSLHandle   hIndex)
{
    CamxResult         result   = CamxResultSuccess;
    CSLHwDevice*       pDevice  = &g_CSLHwInstance.CPASDevice;
    INT32              status   = -1;
    struct cam_control ioctlCmd;

    if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData = static_cast<struct cam_cpas_query_cap *>(CAMX_CALLOC(sizeof(struct cam_cpas_query_cap)));
        ioctlCmd.op_code        = CAM_QUERY_CAP;
        ioctlCmd.size           = sizeof(struct cam_cpas_query_cap);
        ioctlCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved       = 0;
        ioctlCmd.handle         = CamX::Utils::VoidPtrToUINT64(pDevice->pKMDDeviceData);

        status = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (0 > status)
        {
            CAMX_LOG_ERROR(CamxLogGroupCSL, "ioctl failed for fd=%d hIndex %d", pDevice->fd, hIndex);
            result = CamxResultEFailed;
        }
        else
        {
            struct cam_cpas_query_cap* pCaps = static_cast<struct cam_cpas_query_cap *>(pDevice->pKMDDeviceData);
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "kmd query cap family=%d, platform version=%d.%d.%d, cpas version=%d.%d.%d",
                             pCaps->camera_family,
                             pCaps->camera_version.major, pCaps->camera_version.minor, pCaps->camera_version.incr,
                             pCaps->cpas_version.major, pCaps->cpas_version.minor, pCaps->cpas_version.incr);

            result = CSLHwKMDFamilyToCSLFamilyType(pCaps->camera_family, &g_CSLHwInstance.pCameraPlatform.family);
            if (CamxResultEFailed == result)
            {
                CAMX_LOG_ERROR(CamxLogGroupCSL, "failed to get family type for familyId=%d", pCaps->camera_family);
            }
            else
            {
                INT32              socFd;
                CHAR               buf[10]  = {0};
                g_CSLHwInstance.pCameraPlatform.platformVersion.majorVersion = pCaps->camera_version.major;
                g_CSLHwInstance.pCameraPlatform.platformVersion.minorVersion = pCaps->camera_version.minor;
                g_CSLHwInstance.pCameraPlatform.platformVersion.revVersion   = pCaps->camera_version.incr;
                g_CSLHwInstance.pCameraPlatform.CPASVersion.majorVersion     = pCaps->cpas_version.major;
                g_CSLHwInstance.pCameraPlatform.CPASVersion.minorVersion     = pCaps->cpas_version.minor;
                g_CSLHwInstance.pCameraPlatform.CPASVersion.revVersion       = pCaps->cpas_version.incr;
                g_CSLHwInstance.pCameraPlatform.CPASHardwareCaps             = pCaps->reserved;
                socFd = pDevice->deviceOp.Open("/sys/devices/soc0/soc_id", O_RDONLY);
                if (socFd > 0)
                {
                    if (read(socFd, buf, sizeof(buf) - 1) == -1)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCSL, "Unable to read soc_id");
                        result = CamxResultEFailed;
                    }
                    else
                    {
                        g_CSLHwInstance.pCameraPlatform.socId = static_cast<CSLCameraFamilySoc>(atoi(buf));
                    }
                    pDevice->deviceOp.Close(socFd);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCSL, "Unable to open soc_id");
                    result = CamxResultEFailed;
                }
            }
        }
    }

    return result;
}

CSLHwDeviceOps g_CSLHwDeviceCPASOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwCPASKMDQueryCap,
    CSLHwInternalDefaultUMDQueryCap,
    NULL,
    NULL,
    CSLHwInternalKMDStreamOn,
    CSLHwInternalKMDStreamOff,
    NULL,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
