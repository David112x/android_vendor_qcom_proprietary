////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxsensorsubmodulebase.cpp
/// @brief Implements sub module methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxsensorsubmodulebase.h"
#include "camxhal3module.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubModuleBase::SensorSubModuleBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorSubModuleBase::SensorSubModuleBase(
    HwContext*  pHwContext)
{
    m_pHwContext        = pHwContext;
    m_hCSLDeviceHandle  = CSLInvalidHandle;
    m_pPacketManager    = NULL;
    m_pCmdBufferManager = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubModuleBase::~SensorSubModuleBase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorSubModuleBase::~SensorSubModuleBase()
{
    if (m_pCmdBufferManager != NULL)
    {
        m_pCmdBufferManager->Uninitialize();
        CAMX_DELETE m_pCmdBufferManager;
        m_pCmdBufferManager = NULL;
    }

    if (m_pPacketManager != NULL)
    {
        m_pPacketManager->Uninitialize();
        CAMX_DELETE m_pPacketManager;
        m_pPacketManager = NULL;
    }

    if (m_CSLDeviceType == CSLDeviceTypeCSIPHY)
    {
        if (CSLInvalidHandle != m_hCSLDeviceHandle)
        {
            if (TRUE == SensorSubDevicesCache::GetInstance()->MustRelease(m_cameraId, CSIPHYHandle))
            {
                SensorSubDevicesCache::GetInstance()->ReleaseOneSubDevice(m_cameraId, CSIPHYHandle);
                m_deviceAcquired = FALSE;
            }
        }
    }
    else
    {
        CSLReleaseDevice(m_hCSLSession, m_hCSLDeviceHandle);
        m_deviceAcquired = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorSubModuleBase::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorSubModuleBase::Initialize(
    CSLHandle           hCSLSession,
    INT32               CSLDeviceIndex,
    CSLDeviceResource*  pDeviceResourceRequest,
    UINT32              cameraId,
    const CHAR*         pDeviceName)
{
    CamxResult result = CamxResultSuccess;

    m_hCSLSession = hCSLSession;

    if (m_CSLDeviceType == CSLDeviceTypeCSIPHY)
    {
        SubDeviceProperty CSIPHYDevice = SensorSubDevicesCache::GetInstance()->GetSubDevice(cameraId, CSIPHYHandle);

        if (FALSE == CSIPHYDevice.isAcquired)
        {
            result = CSLAcquireDevice(m_hCSLSession,
                                      &m_hCSLDeviceHandle,
                                      CSLDeviceIndex,
                                      pDeviceResourceRequest,
                                      0,
                                      NULL,
                                      0,
                                      pDeviceName);

            if (CamxResultSuccess == result)
            {
                SensorSubDevicesCache::GetInstance()->SetSubDeviceHandle(m_hCSLSession,
                                                                         cameraId,
                                                                         m_hCSLDeviceHandle,
                                                                         CSIPHYHandle);
            }
        }
        else
        {
            CAMX_ASSERT(CSIPHYDevice.hDevice != CSLInvalidHandle);
            m_hCSLDeviceHandle = CSIPHYDevice.hDevice;
            CAMX_LOG_INFO(CamxLogGroupSensor, "Reusing CSIPHY device handle: %p for camerId: %d",
                                               CSIPHYDevice.hDevice, cameraId);
            result = CamxResultSuccess;
        }
    }
    else
    {
        CHAR devName[15];
        OsUtils::SNPrintF(devName, sizeof(devName), "SensorSubDev%d", CSLDeviceIndex);

        CAMX_LOG_INFO(CamxLogGroupSensor, "Not caching CameraId=%u, subDevice=%s", cameraId, devName);

        // Acquire the CSL device handle
        result = CSLAcquireDevice(m_hCSLSession,
                                  &m_hCSLDeviceHandle,
                                  CSLDeviceIndex,
                                  pDeviceResourceRequest,
                                  0,
                                  NULL,
                                  0,
                                  devName);
    }

    if (CamxResultSuccess == result)
    {
        m_deviceAcquired = TRUE;
        m_cameraId       = cameraId;

        // Create packet manager
        m_pPacketManager = CAMX_NEW CmdBufferManager(FALSE);
        if (m_pPacketManager == NULL)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Cannot create CmdBufferManager. No memory");
            result = CamxResultENoMemory;
        }
        else
        {
            ResourceParams resourceParams                  = { 0 };
            resourceParams.usageFlags.packet               = 1;
            resourceParams.packetParams.maxNumCmdBuffers   = 1;
            resourceParams.packetParams.maxNumIOConfigs    = 0;
            resourceParams.resourceSize                    = Packet::CalculatePacketSize(&resourceParams.packetParams);
            resourceParams.poolSize                        = resourceParams.resourceSize;
            resourceParams.alignment                       = CamxPacketAlignmentInBytes;
            resourceParams.pDeviceIndices                  = NULL;
            resourceParams.numDevices                      = 0;
            resourceParams.memFlags                        = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

            result = m_pPacketManager->Initialize("SensorModuleBasePacketManager", &resourceParams);
        }
    }

    if (CamxResultSuccess == result)
    {
        // Create command buffer manager
        m_pCmdBufferManager = CAMX_NEW CmdBufferManager(FALSE);
        if (m_pCmdBufferManager == NULL)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Cannot create CmdBufferManager. No memory");
            result = CamxResultENoMemory;
        }
        else
        {
            ResourceParams params       = { 0 };
            params.resourceSize         = m_resourseSize;
            params.poolSize             = m_poolSize;
            params.usageFlags.cmdBuffer = 1;
            params.cmdParams.type       = CmdType::Generic;
            params.alignment            = CamxCommandBufferAlignmentInBytes;
            params.pDeviceIndices       = NULL;
            params.numDevices           = 0;
            params.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

            result = m_pCmdBufferManager->Initialize("SensorModuleBaseCmdBufferManager", &params);
        }
    }

    return result;
}

CAMX_NAMESPACE_END
