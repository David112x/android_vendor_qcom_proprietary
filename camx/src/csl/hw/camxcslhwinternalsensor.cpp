////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcslhwinternalsensor.cpp
///
/// @brief CamxCSL HW Internal implements for sensor hw csl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if ANDROID

#include <media/cam_defs.h>
#include <media/cam_sensor.h>

#include "camxcslhwinternal.h"
#include "camxmem.h"
#include "camxpacketdefs.h"
#include "camxcslsensordefs.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSensorKmdQueryCapabilityOperation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult CSLHwSensorKmdQueryCapabilityOperation(
    CSLHandle hIndex,
    BOOL      op)
{
    CSLHwDevice*       pDevice  = NULL;
    CamxResult         result   = CamxResultEFailed;
    struct cam_control ioctlCmd;


    if (TRUE == op)
    {
        pDevice  = &g_CSLHwInstance.CSLHwSensorSlotDevices[hIndex];
    }
    else
    {
        pDevice  = &g_CSLHwInstance.CSLInternalKMDDevices[hIndex];
    }


    if (NULL == pDevice->pKMDDeviceData)
    {
        pDevice->pKMDDeviceData = static_cast<cam_sensor_query_cap *>(CAMX_CALLOC(sizeof(cam_sensor_query_cap)));
        ioctlCmd.op_code     = CAM_QUERY_CAP;
        ioctlCmd.size        = sizeof(cam_sensor_query_cap);
        ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved    = 0;
        ioctlCmd.handle      = CamX::Utils::VoidPtrToUINT64(pDevice->pKMDDeviceData);
        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);
        if (result != CamxResultSuccess)
        {
            CAMX_LOG_WARN(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, hIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL,
                             "ioctl success for fd=%d index %d DeviceData %x slot id %d",
                             pDevice->fd,
                             hIndex,
                             pDevice->pKMDDeviceData,
                             static_cast<cam_sensor_query_cap*>(pDevice->pKMDDeviceData)->slot_info);
            // Sensor doesn't support iommu handles so defaulting it to -1 always.
            pDevice->hMapIOMMU.hNonSecureIOMMU      = -1;
            pDevice->hMapIOMMU.hSecureIOMMU         = -1;
            pDevice->hMapCDMIOMMU.hNonSecureIOMMU   = -1;
            pDevice->hMapCDMIOMMU.hSecureIOMMU      = -1;
            // For now no one in UMD interested in KMD data, So later update to the UMD data type properly
        }
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSensorSlotKmdQueryCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSensorSlotKmdQueryCapability(
    CSLHandle hIndex)
{
    return CSLHwSensorKmdQueryCapabilityOperation(hIndex, TRUE);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSensorKmdQueryCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSensorKmdQueryCapability(
    CSLHandle   hIndex)
{
    return CSLHwSensorKmdQueryCapabilityOperation(hIndex, FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSensorFindSlot
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CamxResult CSLHwSensorFindSlot(
    CSLHandle* phIndex,
    UINT16     slot)
{
    CSLHwDevice* pDevice   = NULL;
    UINT         i         = 0;
    CamxResult   result    = CamxResultEFailed;

    for (i = 0; i < g_CSLHwInstance.sensorSlotDeviceCount ; i++)
    {
        pDevice = &g_CSLHwInstance.CSLHwSensorSlotDevices[i];
        if ((pDevice != NULL) && (pDevice->pKMDDeviceData != NULL))
        {
            if (slot == static_cast<cam_sensor_query_cap *>(pDevice->pKMDDeviceData)->slot_info)
            {
                *phIndex = i;
                result = CamxResultSuccess;
                break;
            }
        }
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalProbeSensorHW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalProbeSensorHW(
    CSLMemHandle hPacket,
    SIZE_T       offset,
    INT32*       pDeviceIndex)
{
    CSLBufferInfo  probeDataBuffer;
    CSLPacket*     pPacket;
    CSLCmdMemDesc* pCmdDescs;
    CamxResult     result = CamxResultEFailed;

    result = CSLGetBufferInfoHW(hPacket, &probeDataBuffer);
    if (CamxResultSuccess == result)
    {
        cam_cmd_probe* pSlaveInfo;
        CSLHandle      hIndex  = CSLInvalidHandle;
        CSLHwDevice*   pLoophw = NULL;

        pPacket    = reinterpret_cast<CSLPacket*>(CamX::Utils::VoidPtrInc(probeDataBuffer.pVirtualAddr, offset));
        pCmdDescs  = reinterpret_cast<CSLCmdMemDesc*>(CamX::Utils::VoidPtrInc(&pPacket->data, pPacket->cmdBuffersOffset));
        result     = CSLGetBufferInfoHW(pCmdDescs->hMem, &probeDataBuffer);
        if (CamxResultSuccess == result)
        {
            pSlaveInfo = reinterpret_cast<cam_cmd_probe*>(CamX::Utils::VoidPtrInc(probeDataBuffer.pVirtualAddr,
                                                                                  sizeof(cam_cmd_i2c_info)));
            result     = CSLHwSensorFindSlot(&hIndex, pSlaveInfo->camera_id);

            if (CamxResultSuccess == result)
            {
                pLoophw = &g_CSLHwInstance.CSLHwSensorSlotDevices[hIndex];
                pLoophw->lock->Lock();
                if (NULL != pLoophw->deviceOp.Ioctl)
                {
                    struct cam_control ioctlCmd;

                    ioctlCmd.op_code     = CAM_SENSOR_PROBE_CMD;
                    ioctlCmd.size        = sizeof(ioctlCmd.handle);
                    ioctlCmd.handle_type = CAM_HANDLE_MEM_HANDLE;
                    ioctlCmd.reserved    = 0;
                    ioctlCmd.handle      = hPacket;

                    result = pLoophw->deviceOp.Ioctl(pLoophw, VIDIOC_CAM_CONTROL, &ioctlCmd);

                    if (result != CamxResultSuccess)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl failed for fd=%d", pLoophw->fd);
                    }
                    else
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d", pLoophw->fd);
                    }
                }
                pLoophw->lock->Unlock();
            }

            // Now move this node to the CSL Instance devices list if successful sensor probe
            if (CamxResultSuccess == result)
            {
                CHAR   device_name[CSLHwMaxDevName];
                UINT32 groupId;
                INT    deviceFd;

                pLoophw->lock->Lock();
                CamX::OsUtils::SNPrintF(device_name, sizeof(device_name), "%s", pLoophw->devName);
                groupId  = pLoophw->kmdGroupId;
                deviceFd = pLoophw->fd;
                pLoophw->lock->Unlock();

                CSLHwRemoveSensorSlotDeviceFromInstance(pLoophw);
                CSLHwAddKMDDeviceToInstance(device_name, groupId, pDeviceIndex, deviceFd);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "No reference to command buffer in sensor probe packet");
        }
    }
    return result;

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwSensorKmdAcquire
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwSensorKmdAcquire(
    CSLHandle          hCSL,
    CSLDeviceHandle*   phDevice,
    INT32              deviceIndex,
    CSLDeviceResource* pDeviceResourceRequest,
    SIZE_T             numDeviceResources)
{
    CAMX_UNREFERENCED_PARAM(pDeviceResourceRequest);
    CAMX_UNREFERENCED_PARAM(numDeviceResources);

    cam_sensor_acquire_dev acquireCmd;
    struct cam_control     ioctlCmd;
    CSLHwDevice*           pDevice       = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CSLHwsession*          pSession      = NULL;
    CSLHandle              hCSLHwSession = CSLInvalidHandle;
    CamxResult             result        = CamxResultEFailed;

    hCSLHwSession = CAM_REQ_MGR_GET_HDL_IDX(hCSL);
    if (hCSLHwSession < CSLHwMaxNumSessions)
    {
        pSession = &g_CSLHwInstance.sessionList[hCSLHwSession];

        acquireCmd.session_handle = hCSL;
        acquireCmd.device_handle  = CSLInvalidHandle;
        acquireCmd.reserved       = 0;
        acquireCmd.handle_type    = CAM_HANDLE_USER_POINTER;
        acquireCmd.info_handle    = 0;

        ioctlCmd.op_code     = CAM_ACQUIRE_DEV;
        ioctlCmd.size        = sizeof(cam_sensor_acquire_dev);
        ioctlCmd.handle_type = CAM_HANDLE_USER_POINTER;
        ioctlCmd.reserved    = 0;
        ioctlCmd.handle      = CamX::Utils::VoidPtrToUINT64(&acquireCmd);

        result = pDevice->deviceOp.Ioctl(pDevice, VIDIOC_CAM_CONTROL, &ioctlCmd);

        if (CamxResultEFailed == result)
        {
            CAMX_LOG_WARN(CamxLogGroupCSL, "ioctl failed for fd=%d index %d", pDevice->fd, deviceIndex);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCSL, "ioctl success for fd=%d index %d", pDevice->fd, deviceIndex);
            *phDevice = acquireCmd.device_handle;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "CamxResultEOutOfBounds: hCSLHwSession = %d", hCSLHwSession);
        result = CamxResultEOutOfBounds;
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalSensorUMDQueryCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalSensorUMDQueryCapability(
    INT32   deviceIndex,
    VOID*   pDeviceData,
    SIZE_T  deviceDataSize)
{
    CSLHwDevice* pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result  = CamxResultSuccess;

    if ((sizeof(CSLSensorCapability) != deviceDataSize) || (NULL == pDevice->pKMDDeviceData))
    {
        result = CamxResultEFailed;
    }
    else
    {
        CamX::Utils::Memcpy(pDeviceData, pDevice->pKMDDeviceData, deviceDataSize);
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalSensorKMDStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalSensorKMDStreamOn(
    CSLHandle           hCSLHwSession,
    CSLDeviceHandle     hDevice,
    INT32               deviceIndex,
    CSLDeactivateMode   mode)
{
    CSLHwDevice* pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result  = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(mode);

    CAMX_LOG_INFO(CamxLogGroupCSL, "Try streaming on Sensor device index %d", deviceIndex);
    result = CSLHwInternalKMDStreamOp(hCSLHwSession, hDevice, deviceIndex, TRUE);
    if (CamxResultSuccess == result)
    {
        CAMX_LOG_INFO(CamxLogGroupCSL, "Device fd %d stream on success index %d", pDevice->fd, deviceIndex);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Device stream on failed device fd %d index %d", pDevice->fd, deviceIndex);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSLHwInternalSensorKMDStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSLHwInternalSensorKMDStreamOff(
    CSLHandle           hCSLHwSession,
    CSLDeviceHandle     hDevice,
    INT32               deviceIndex,
    CSLDeactivateMode   mode)
{
    CSLHwDevice* pDevice = &g_CSLHwInstance.CSLInternalKMDDevices[deviceIndex];
    CamxResult   result  = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(mode);

    CAMX_LOG_INFO(CamxLogGroupCSL, "Try streaming off Sensor device index %d", deviceIndex);
    result = CSLHwInternalKMDStreamOp(hCSLHwSession, hDevice, deviceIndex, FALSE);
    if (CamxResultSuccess == result)
    {
        CAMX_LOG_INFO(CamxLogGroupCSL, "Device fd %d stream off success index %d", pDevice->fd, deviceIndex);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCSL, "Device stream off failed device fd %d index %d", pDevice->fd, deviceIndex);
    }

    return result;
}

CSLHwDeviceOps g_CSLHwDeviceSensorOps =
{
    CSLHwInternalDefaultOpen,
    CSLHwInternalDefaultClose,
    CSLHwInternalDefaultIoctl,
    NULL,
    CSLHwSensorKmdQueryCapability,
    CSLHwInternalSensorUMDQueryCapability,
    CSLHwSensorKmdAcquire,
    CSLHwInternalKMDRelease,
    CSLHwInternalSensorKMDStreamOn,
    CSLHwInternalSensorKMDStreamOff,
    CSLHwInternalDefaultSubmit,
    CSLHwInternalKMDAcquireHardware,
    NULL,
    CSLHwInternalKMDReleaseHardware
};

#endif // ANDROID
