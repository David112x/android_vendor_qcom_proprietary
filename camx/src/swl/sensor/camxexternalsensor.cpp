////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxexternalsensor.cpp
/// @brief Sensor Module class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxexternalsensor.h"

CAMX_NAMESPACE_BEGIN

static const UINT SensorMaxCmdBufferManagerCount = 40;                       ///< Number of max command buffer managers
static const UINT InitialConfigCmdCount          = 1;                        ///< Number of command buffers in config command
static const UINT I2CInfoCmdCount                = 1;                        ///< Number of command buffers in I2C command
static const UINT UpdateCmdCount                 = 40;                       ///< max request depth for Update command

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::Initialize(
    UINT32      cameraId,
    CSLHandle   hCSLSession,
    HwContext*  pHwContext,
    UINT32      currentSensorMode)
{
    CamxResult              result              = CamxResultSuccess;
    CSLCSIPHYCapability     CSIPHYCapability    = { 0 };
    const CHAR*             pNameAndInstanceId  = "ExternalSensor";

    m_cameraId                  = cameraId;
    m_deviceIndexCount          = 0;
    m_pHwContext                = pHwContext;
    m_currentResolutionIndex    = currentSensorMode;
    m_hCSLSession               = hCSLSession;


    // Must initialize before creating command buffers
    result = InitializeCmdBufferManagerList(SensorMaxCmdBufferManagerCount);

    if (CamxResultSuccess == result)
    {
        // get sensor module indexes
        // NOWHINE CP036a: Need exception here
        m_pSensorModuleData = const_cast<ImageSensorModuleData*>(m_pHwContext->GetImageSensorModuleData(m_cameraId));
        CAMX_ASSERT(NULL != m_pSensorModuleData);

        if (NULL != m_pSensorModuleData)
        {
            result = GetSensorModuleIndexes(&CSIPHYCapability);
        }
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Got Sensor Module Indexes");
        // Acquire Sensor device
        SubDeviceProperty sensorDevice = m_pSensorSubDevicesCache->GetSubDevice(cameraId, SensorHandle);

        if (FALSE == sensorDevice.isAcquired)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Acquiring sensor device handle");
            result = CSLAcquireDevice(m_hCSLSession,
                                      &m_hSensorDevice,
                                      sensorDevice.deviceIndex,
                                      NULL,
                                      0,
                                      NULL,
                                      0,
                                      pNameAndInstanceId);
        }
        else
        {
            CAMX_ASSERT(sensorDevice.hDevice != CSLInvalidHandle);
            m_hSensorDevice = sensorDevice.hDevice;

            CAMX_LOG_INFO(CamxLogGroupSensor,
                          "Reusing sensor device handle: %p for camerId: %d",
                          m_hSensorDevice,
                          m_cameraId);

            result = CamxResultSuccess;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to Get Sensor Module Indexes");
    }

    if (CamxResultSuccess == result)
    {
        m_sensorState = SensorState::SensorAcquired;

        CAMX_LOG_INFO(CamxLogGroupSensor,
                      "Acquired sensor device handle: %p for camerId: %d",
                      m_hSensorDevice,
                      cameraId);

        m_pSensorSubDevicesCache->SetSubDeviceHandle(m_hCSLSession, cameraId, m_hSensorDevice, SensorHandle);
        AddCSLDeviceHandle(m_hSensorDevice);
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "AcquireDevice on Sensor successful!");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "AcquireDevice on Sensor failed: %d sensor device handle: %p for camerId: %d",
                       Utils::CamxResultToString(result),
                       m_hSensorDevice,
                       cameraId);
    }

    if (CamxResultSuccess == result)
    {
        result = LoadSensorInitConfigCmd();
    }

    if (CamxResultSuccess == result)
    {
        result = LoadSensorConfigCmds();
    }

    if (CamxResultSuccess == result)
    {
        result = CreateSensorSubmodules();
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::Uninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::Uninitialize()
{
    CamxResult result = CamxResultSuccess;

    if (SensorState::SensorUninitialized != m_sensorState)
    {
        if (NULL != m_pHwContext)
        {
            if (TRUE == SensorSubDevicesCache::GetInstance()->MustRelease(m_cameraId, SensorHandle))
            {
                SensorSubDevicesCache::GetInstance()->ReleaseOneSubDevice(m_cameraId, SensorHandle);
                m_sensorState = SensorState::SensorReleased;
            }

            if (TRUE == SensorSubDevicesCache::GetInstance()->MustRelease(m_cameraId, CSIPHYHandle))
            {
                SensorSubDevicesCache::GetInstance()->ReleaseOneSubDevice(m_cameraId, CSIPHYHandle);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "CSIPHYHandle Release failed: cameraId=%u m_pHwContext=%p",
                           m_cameraId, m_pHwContext);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::StreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::StreamOn()
{
    CamxResult        result        = CamxResultSuccess;
    SubDeviceProperty sensorDevice  = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, SensorHandle);
    SubDeviceProperty csiphyDevice  = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, CSIPHYHandle);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Step1: on Stream on : Update Startup exposure");
    UpdateStartupExposureSettings();

    UINT regSettingIdx  = 0;
    UINT AECCmdSize     = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::AEC,
                                                               m_pRegSettings,
                                                               0,
                                                               &regSettingIdx);

    CAMX_ASSERT(0 == (AECCmdSize % sizeof(UINT32)));

    result = CreateAndSubmitCommand(AECCmdSize,
                                    I2CRegSettingType::AEC,
                                    CSLPacketOpcodesSensorConfig,
                                    m_currentResolutionIndex,
                                    regSettingIdx);

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Step2: on Stream on : Streamon Phy");
        result = CSLSingleDeviceStreamOn(m_hCSLSession,
                                         csiphyDevice.deviceIndex,
                                         &csiphyDevice.hDevice);
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Step3: on Stream on : Streamon Sensor");
        result = CSLSingleDeviceStreamOn(m_hCSLSession,
                                         sensorDevice.deviceIndex,
                                         &m_hSensorDevice);
    }

    if (CamxResultSuccess == result)
    {
        m_sensorState = SensorState::SensorStreamedOn;
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Stream on Successful");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Stream on Failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::StreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::StreamOff()
{
    CamxResult result = CamxResultSuccess;
    if (SensorState::SensorStreamedOn == m_sensorState)
    {
        SubDeviceProperty sensorDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, SensorHandle);
        SubDeviceProperty csiphyDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, CSIPHYHandle);


        if (CamxResultSuccess == result)
        {
            if (TRUE == sensorDevice.isAcquired && NULL != m_pHwContext)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Step1: on Stream off : Streamoff Sensor");
                result = CSLSingleDeviceStreamOff(m_hCSLSession,
                                                  sensorDevice.deviceIndex,
                                                  &m_hSensorDevice);
            }
        }


        if (CamxResultSuccess == result)
        {
            if (TRUE == csiphyDevice.isAcquired && NULL != m_pHwContext)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Step2: on Stream off : Streamoff Phy");
                result = CSLSingleDeviceStreamOff(m_hCSLSession,
                                                  csiphyDevice.deviceIndex,
                                                  &csiphyDevice.hDevice);
            }
        }

        if (CamxResultSuccess == result)
        {
            m_sensorState = SensorState::SensorStreamedOff;
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Stream off Successful");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Stream off Failed");
        }
    }
    else
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Stream off Failed due to invalid state");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ExternalSensor* ExternalSensor::Create()
{
    ExternalSensor* pSensor = CAMX_NEW ExternalSensor;
    return pSensor;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExternalSensor::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::ExternalSensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ExternalSensor::ExternalSensor()
    : m_hSensorDevice(CSLInvalidHandle)
    , m_currentResolutionIndex(1)
    , m_currentSyncMode(NoSync)
{
    m_pSensorSubDevicesCache = SensorSubDevicesCache::GetInstance();
    m_sensorState = SensorState::SensorUninitialized;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::~ExternalSensor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ExternalSensor::~ExternalSensor()
{
    Uninitialize();

    // destroy all created objects
    if (NULL != m_pCSIPHY)
    {
        m_pCSIPHY->Destroy();
        m_pCSIPHY = NULL;
    }

    if ((0 != m_numCmdBufferManagers) && (NULL != m_ppCmdBufferManagers))
    {
        for (UINT i = 0; i < m_numCmdBufferManagers; i++)
        {
            CAMX_ASSERT(NULL != m_ppCmdBufferManagers[i]);
            if (NULL != m_ppCmdBufferManagers[i])
            {
                CAMX_DELETE m_ppCmdBufferManagers[i];
                m_ppCmdBufferManagers[i] = NULL;
            }
        }
    }

    if (NULL != m_ppCmdBufferManagers)
    {
        CAMX_FREE(m_ppCmdBufferManagers);
        m_numCmdBufferManagers      = 0;
        m_maxNumCmdBufferManagers   = 0;
        m_ppCmdBufferManagers       = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::GetSensorModuleIndexes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::GetSensorModuleIndexes(
    CSLCSIPHYCapability*     pCSIPHYCapability)
{
    CDKResult result = CDKResultEFailed;
    // Get the slot info for this camera
    CSLSensorCapability sensorCap    = { 0 };
    SubDeviceProperty sensorDevice   = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, SensorHandle);
    SubDeviceProperty CSIPHYDevice   = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, CSIPHYHandle);


    INT32 deviceIndices[MaxNumImageSensors] = { 0 };
    UINT  actualNumIndices = 0;

    if (FALSE == sensorDevice.isAcquired)
    {
        result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeImageSensor,
                                                                &deviceIndices[0],
                                                                MaxNumImageSensors,
                                                                &actualNumIndices);

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(m_cameraId < actualNumIndices);
            m_pSensorSubDevicesCache->SetSubDeviceIndex(m_cameraId, deviceIndices[m_cameraId], SensorHandle);
            sensorDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, SensorHandle); // to get the updated value
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get device index for Sensor");
            return result;
        }
    }

    if (CamxResultSuccess == AddDeviceIndex(sensorDevice.deviceIndex))
    {

        result = CSLQueryDeviceCapabilities(sensorDevice.deviceIndex,
                                            &sensorCap,
                                            sizeof(CSLSensorCapability));

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                             "SensorDeviceIndex[%d] Cap={slotInfo:%d,CSIPHY:%d}",
                             sensorDevice.deviceIndex,
                             sensorCap.slotInfo,
                             sensorCap.CSIPHYSlotId);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to query sensor capabilites");
            return result;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Failed to add device result: %d, sensor index: %d, actual indices: %d",
                       Utils::CamxResultToString(result),
                       sensorDevice.deviceIndex,
                       actualNumIndices);

        return result;
    }

    if (FALSE == CSIPHYDevice.isAcquired)
    {
        result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeCSIPHY,
                                                                &deviceIndices[0],
                                                                MaxNumImageSensors,
                                                                &actualNumIndices);

        if (CamxResultSuccess == result)
        {
            // Find CSIPHY index match sensor slot info
            for (UINT i = 0; i < actualNumIndices; i++)
            {
                if ((CamxResultSuccess == CSLQueryDeviceCapabilities(deviceIndices[i],
                                                                     pCSIPHYCapability,
                                                                     sizeof(CSLCSIPHYCapability))) &&
                    (pCSIPHYCapability->slotInfo == sensorCap.CSIPHYSlotId))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                        "Found matched CSIPHY[%d] deviceIndices %d slotInfo %d",
                        i,
                        deviceIndices[i],
                        pCSIPHYCapability->slotInfo);

                    m_pSensorSubDevicesCache->SetSubDeviceIndex(m_cameraId, deviceIndices[i], CSIPHYHandle);
                    CSIPHYDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, CSIPHYHandle); // to get the updated value
                    break;
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get device indices for CSIPHY");
            return result;
        }
    }
    else
    {
        if ((CamxResultSuccess != CSLQueryDeviceCapabilities(CSIPHYDevice.deviceIndex,
                                                             pCSIPHYCapability,
                                                             sizeof(CSLCSIPHYCapability))) ||
            (pCSIPHYCapability->slotInfo != sensorCap.CSIPHYSlotId))
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Cached CSIPHY device index: %d is not valid for given sensor",
                CSIPHYDevice.deviceIndex);
            return CamxResultEFailed;
        }
    }

    result = AddDeviceIndex(CSIPHYDevice.deviceIndex);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Failed to add device result: %d, CSIPHY index: %d, actual indices: %d",
                       Utils::CamxResultToString(result),
                       CSIPHYDevice.deviceIndex,
                       actualNumIndices);

        return result;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::AddDeviceIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::AddDeviceIndex(
    INT32 deviceIndex)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((m_deviceIndexCount + 1) <= CamxMaxDeviceIndex);

    if ((m_deviceIndexCount + 1) <= CamxMaxDeviceIndex)
    {
        m_deviceIndices[m_deviceIndexCount++] = deviceIndex;
        // Add the added device index to all output ports.
        for (UINT portIndex = 0; portIndex < m_outputPortsData.numPorts; portIndex++)
        {
            UINT portID = m_outputPortsData.pOutputPorts[portIndex].portId;
            AddOutputDeviceIndices(portID, &deviceIndex, 1);
        }
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::AddOutputDeviceIndices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExternalSensor::AddOutputDeviceIndices(
    UINT         portId,
    const INT32* pDeviceIndices,
    UINT         deviceIndexCount)
{
    UINT        portIndex = OutputPortIndex(portId);
    OutputPort* pPort = &(m_outputPortsData.pOutputPorts[portIndex]);

    for (UINT i = 0; i < deviceIndexCount; i++)
    {
        BOOL alreadyInList = FALSE;

        for (UINT j = 0; j < pPort->deviceCount; j++)
        {
            if (pDeviceIndices[i] == pPort->deviceIndices[j])
            {
                alreadyInList = TRUE;
                break;
            }
        }

        if (FALSE == alreadyInList)
        {
            pPort->deviceIndices[pPort->deviceCount++] = pDeviceIndices[i];
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::OutputPortIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ExternalSensor::OutputPortIndex(
    UINT portId)
{
    UINT portIndex = 0;
    UINT port = 0;

    for (port = 0; port < m_outputPortsData.numPorts; port++)
    {
        if (m_outputPortsData.pOutputPorts[port].portId == portId)
        {
            portIndex = port;
            break;
        }
    }

    CAMX_ASSERT(port < m_outputPortsData.numPorts);

    return portIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::InitializeCmdBufferManagerList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::InitializeCmdBufferManagerList(
    UINT maxNumCmdBufferManagers)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((NULL == m_ppCmdBufferManagers) && (0 != maxNumCmdBufferManagers));

    m_maxNumCmdBufferManagers = 0;
    m_ppCmdBufferManagers =
        static_cast<CmdBufferManager**>(CAMX_CALLOC(sizeof(CmdBufferManager*) * maxNumCmdBufferManagers));

    if (NULL == m_ppCmdBufferManagers)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("InitializeCmdBufferManagerList: CAMX_CALLOC failed");
        result = CamxResultENoMemory;
    }
    else
    {
        m_maxNumCmdBufferManagers = maxNumCmdBufferManagers;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExternalSensor::LoadSensorInitConfigCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::LoadSensorInitConfigCmd()
{
    CamxResult  result = CamxResultSuccess;

    UINT initializeCmdSize  = 0;
    UINT WBCmdSize          = 0;
    UINT LSCCmdSize         = 0;
    UINT regSettingIdx      = 0;
    do
    {
        UINT curRegSettingIdx = regSettingIdx;
        // NOWHINE NC011: Asking for exception, sensor commands are well known as - init, res, AEC, start, stop etc
        initializeCmdSize = GetSensorDataObject()->GetI2CCmdSize(I2CRegSettingType::Init,
                                                                 NULL,
                                                                 0,
                                                                 &regSettingIdx);

        // All sizes should be a multiple of dword
        CAMX_ASSERT(0 == (initializeCmdSize % sizeof(UINT32)));

        CAMX_LOG_INFO(CamxLogGroupSensor, "Sensor initializeCmdSize: %d", initializeCmdSize);


        result = CreateAndSubmitCommand(initializeCmdSize,
            // NOWHINE NC011: Asking for exception, sensor commands are well known as - init
                                        I2CRegSettingType::Init,
                                        CSLPacketOpcodesSensorInitialConfig,
                                        0,
                                        curRegSettingIdx);

        CAMX_LOG_INFO(CamxLogGroupSensor, "Sensor initialized successfully");
    } while (regSettingIdx != 0);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExternalSensor::CreateAndSubmitCommand
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::CreateAndSubmitCommand(
    UINT cmdSize,
    I2CRegSettingType cmdType,
    CSLPacketOpcodesSensor opCode,
    UINT32 currentResolutionIndex,
    UINT regSettingIdx)

{
    CamxResult  result = CamxResultSuccess;

    // Create configuration packet/command buffers
    ResourceParams resourceParams                   = { 0 };
    resourceParams.usageFlags.packet                = 1;
    resourceParams.packetParams.maxNumCmdBuffers    = 1;
    resourceParams.packetParams.maxNumIOConfigs     = 0;
    resourceParams.resourceSize                     = Packet::CalculatePacketSize(&resourceParams.packetParams);
    resourceParams.poolSize                         = resourceParams.resourceSize;
    resourceParams.alignment                        = CamxPacketAlignmentInBytes;
    resourceParams.pDeviceIndices                   = NULL;
    resourceParams.numDevices                       = 0;
    resourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    result = CreateCmdBufferManager(&resourceParams, &m_pConfigPacketManager);
    if (CamxResultSuccess == result)
    {
        ResourceParams params       = { 0 };
        params.resourceSize         = cmdSize;
        params.poolSize             = InitialConfigCmdCount * cmdSize;
        params.usageFlags.cmdBuffer = 1;
        params.cmdParams.type       = CmdType::I2C;
        params.alignment            = CamxCommandBufferAlignmentInBytes;
        params.pDeviceIndices       = NULL;
        params.numDevices           = 0;
        params.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager(&params, &m_pConfigCmdBufferManager);
    }

    if ((CamxResultSuccess == result) && (NULL != m_pConfigCmdBufferManager) && (NULL != m_pConfigPacketManager))
    {
        m_pConfigCmdBuffer = GetCmdBuffer(m_pConfigCmdBufferManager);
        m_pConfigPacket = GetPacket(m_pConfigPacketManager);

        if ((NULL == m_pConfigPacket) || (NULL == m_pConfigCmdBuffer))
        {
            result = CamxResultENoMemory;
        }
        else
        {
            ImageSensorData* pSensorData = GetSensorDataObject();
            CAMX_ASSERT(NULL != pSensorData);

            VOID* pCmd = m_pConfigCmdBuffer->BeginCommands(cmdSize / sizeof(UINT32));
            if (NULL != pCmd)
            {
                if (I2CRegSettingType::AEC == cmdType)
                {
                    result = pSensorData->CreateI2CCmd(cmdType,
                                                       pCmd,
                                                       m_pRegSettings,
                                                       currentResolutionIndex,
                                                       regSettingIdx);
                }
                else if (I2CRegSettingType::SPC == cmdType)
                {
                    result = pSensorData->CreateI2CCmd(cmdType,
                                                       pCmd,
                                                       static_cast<const VOID*>(&m_pOTPData->SPCCalibration.settings),
                                                       currentResolutionIndex,
                                                       regSettingIdx);
                }
                else if (I2CRegSettingType::AWBOTP == cmdType)
                {
                    result = pSensorData->CreateI2CCmd(cmdType,
                                                       pCmd,
                                                       static_cast<const VOID*>(&m_pOTPData->WBCalibration[0].settings),
                                                       currentResolutionIndex,
                                                       regSettingIdx);
                }
                else if (I2CRegSettingType::LSC == cmdType)
                {
                    result = pSensorData->CreateI2CCmd(cmdType,
                                                       pCmd,
                                                       static_cast<const VOID*>(&m_pOTPData->LSCCalibration[0].settings),
                                                       currentResolutionIndex,
                                                       regSettingIdx);
                }
                else
                {
                    result = pSensorData->CreateI2CCmd(cmdType, pCmd, NULL, currentResolutionIndex, regSettingIdx);
                }
                if (CamxResultSuccess == result)
                {
                    result = m_pConfigCmdBuffer->CommitCommands();
                }
            }
            if (CamxResultSuccess == result)
            {
                // Not associated with any request. Won't be recycled.
                m_pConfigCmdBuffer->SetRequestId(CamxInvalidRequestId);

                // Not associated with any request. Won't be recycled.
                m_pConfigPacket->SetRequestId(CamxInvalidRequestId);

                m_pConfigPacket->SetOpcode(CSLDeviceTypeInvalidDevice, opCode);

                result = m_pConfigPacket->AddCmdBufferReference(m_pConfigCmdBuffer, NULL);
            }

            if (CamxResultSuccess == result)
            {
                result = m_pConfigPacket->CommitPacket();
            }
            if (CamxResultSuccess == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "IN Submit!");
                // Send Sensor  configuration
                result = m_pHwContext->Submit(m_hCSLSession, m_hSensorDevice, m_pConfigPacket);
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "out Submit! result %d", result);
            }
            if (NULL != m_pConfigPacketManager)
            {
                m_pConfigPacketManager->Recycle(m_pConfigPacket);
                m_pConfigPacket = NULL;
            }

            if (NULL != m_pConfigCmdBufferManager)
            {
                m_pConfigCmdBufferManager->Recycle(m_pConfigCmdBuffer);
                m_pConfigCmdBuffer = NULL;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::CreateCmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::CreateCmdBufferManager(
    const ResourceParams* pParams,
    CmdBufferManager**    ppCmdBufferManager)
{
    CAMX_ASSERT(m_numCmdBufferManagers < m_maxNumCmdBufferManagers);

    CamxResult result = CmdBufferManager::Create("ExternalSensorCmdBufferManager", pParams, ppCmdBufferManager);

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(NULL != *ppCmdBufferManager);
        m_ppCmdBufferManagers[m_numCmdBufferManagers++] = *ppCmdBufferManager;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::GetCmdBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBuffer* ExternalSensor::GetCmdBuffer(
    CmdBufferManager* pCmdBufferManager)
{
    PacketResource* pPacketResource = NULL;

    CAMX_ASSERT(NULL != pCmdBufferManager);

    if (CamxResultSuccess == pCmdBufferManager->GetBuffer(&pPacketResource))
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
    }

    return static_cast<CmdBuffer*>(pPacketResource);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::GetPacket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Packet* ExternalSensor::GetPacket(
    CmdBufferManager* pCmdBufferManager)
{
    PacketResource* pPacketResource = NULL;

    CAMX_ASSERT(NULL != pCmdBufferManager);

    if (CamxResultSuccess == pCmdBufferManager->GetBuffer(&pPacketResource))
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);
    }

    return static_cast<Packet*>(pPacketResource);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::LoadSensorConfigCmds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::LoadSensorConfigCmds()
{
    CamxResult       result         = CamxResultEFailed;
    ImageSensorData* pSensorData    = GetSensorDataObject();

    CAMX_ASSERT(NULL != pSensorData);

    UINT regSettingIdx  = 0; // using common index as only init reg setting uses this param
    UINT resCmdSize     = pSensorData->GetI2CCmdSize(I2CRegSettingType::Res, NULL, m_currentResolutionIndex, &regSettingIdx);
    UINT startCmdSize   = pSensorData->GetI2CCmdSize(I2CRegSettingType::Start, NULL, 0, &regSettingIdx);
    UINT stopCmdSize    = pSensorData->GetI2CCmdSize(I2CRegSettingType::Stop, NULL, 0, &regSettingIdx);

    UINT SPCCmdSize                  = 0;
    UINT syncCmdSize                 = 0;

    I2CRegSettingType syncI2CCmdType = I2CRegSettingType::Master;

    if (MasterMode == m_currentSyncMode)
    {
        syncCmdSize    = pSensorData->GetI2CCmdSize(I2CRegSettingType::Master, NULL, 0, &regSettingIdx);
        syncI2CCmdType = I2CRegSettingType::Master;
    }
    else if (SlaveMode == m_currentSyncMode)
    {
        syncCmdSize    = pSensorData->GetI2CCmdSize(I2CRegSettingType::Slave, NULL, 0, &regSettingIdx);
        syncI2CCmdType = I2CRegSettingType::Slave;
    }

    if ((NULL != m_pOTPData) && (TRUE == m_pOTPData->SPCCalibration.isAvailable))
    {
        SPCCmdSize = pSensorData->GetI2CCmdSize(I2CRegSettingType::SPC,
                                                static_cast<const VOID*>(&m_pOTPData->SPCCalibration.settings),
                                                0, &regSettingIdx);
    }

    CAMX_ASSERT_MESSAGE((resCmdSize != 0) && (startCmdSize != 0) && (stopCmdSize != 0),
        "Command size(res: %d, start: %d stop: %d)", resCmdSize, startCmdSize, stopCmdSize);

    // All sizes should be a multiple of dword
    CAMX_ASSERT(0 == (resCmdSize % sizeof(UINT32)));
    CAMX_ASSERT(0 == (startCmdSize % sizeof(UINT32)));
    CAMX_ASSERT(0 == (stopCmdSize % sizeof(UINT32)));

    if ((NULL != m_pOTPData) && (TRUE == m_pOTPData->SPCCalibration.isAvailable))
    {
        CAMX_ASSERT(0 == (SPCCmdSize % sizeof(UINT32)));
    }

    CAMX_LOG_INFO(CamxLogGroupSensor,
        "resCmdSize: %d, startCmdSize: %d, stopCmdSize:= %d, syncCmdSize: %d, SPCCmdSize: %d",
        resCmdSize, startCmdSize, stopCmdSize, syncCmdSize, SPCCmdSize);

    result = CreateAndSubmitCommand(
        resCmdSize, I2CRegSettingType::Res, CSLPacketOpcodesSensorConfig,
        m_currentResolutionIndex, regSettingIdx);

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Res configuration successful!");

        if (NoSync != m_currentSyncMode)
        {
            if (0 == syncCmdSize)
            {
                CAMX_LOG_WARN(CamxLogGroupSensor,
                    "Seems no sensor sync setting in sensor driver, can't apply sync setting!");
            }
            else
            {
                result = CreateAndSubmitCommand(syncCmdSize, syncI2CCmdType,
                            CSLPacketOpcodesSensorConfig, 0, regSettingIdx);

                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Sensor hardware sync configure successful!");
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor hardware sync configure failed!");
                }
            }
        }

        if ((CamxResultSuccess == result) && (0 != SPCCmdSize))
        {
            result = CreateAndSubmitCommand(SPCCmdSize, I2CRegSettingType::SPC,
                        CSLPacketOpcodesSensorConfig, 0, regSettingIdx);
            if (CamxResultSuccess == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "SpcSetting configuration successful!");
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "SpcSetting configuration failed!");
            }
        }

        if (CamxResultSuccess == result)
        {
            result = CreateAndSubmitCommand(startCmdSize, I2CRegSettingType::Start,
                        CSLPacketOpcodesSensorStreamOn, 0, regSettingIdx);

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Streamon configuration successful!");

                result = CreateAndSubmitCommand(stopCmdSize, I2CRegSettingType::Stop,
                            CSLPacketOpcodesSensorStreamOff, 0, regSettingIdx);

                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "StreamOFF configuration successful!");
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor Streamoff configuration failed!");
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor StreamOn configuration failed!");
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor Res configuration failed!");
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Sensor configuration successful");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor configuration Failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::CreateSensorSubmodules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::CreateSensorSubmodules()
{
    CamxResult      result = CamxResultSuccess;
    StatsCameraInfo cameraInfo;

    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupSensor, SCOPEEventSubModuleAcquire);

    result = CreateSubModules();

    if (CamxResultSuccess == result)
    {
        result = CreatePerFrameUpdateResources();
    }

    if (CamxResultSuccess == result)
    {
        result = GetInitialCameraInfo(&cameraInfo);
    }

    if (CamxResultSuccess == result)
    {
        const CSIInformation*   pCSIInfo = m_pSensorModuleData->GetCSIInfo();
        CSLSensorCSIPHYInfo     cmdCSIPHYConfig = { 0 };
        if (NULL != pCSIInfo)
        {
            GetSensorDataObject()->CreateCSIPHYConfig(&cmdCSIPHYConfig, pCSIInfo->laneAssign,
                pCSIInfo->isComboMode, m_currentResolutionIndex);
            cmdCSIPHYConfig.secureMode = static_cast<UINT8>(0);
            result = m_pCSIPHY->Configure(m_hCSLSession, &cmdCSIPHYConfig);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "CSI Configuration failed");
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Sensor Submodule creation successful");
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Sensor Submodule creation failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExternalSensor::CreateSubModules
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::CreateSubModules()
{
    CamxResult        result       = CamxResultSuccess;
    SubDeviceProperty CSIPHYDevice = m_pSensorSubDevicesCache->GetSubDevice(m_cameraId, CSIPHYHandle);

    result = CSIPHYSubmodule::Create(m_pHwContext, m_hCSLSession, &m_pCSIPHY, CSIPHYDevice.deviceIndex, m_cameraId,
                                     "ExternalCSIPHYSensor");

    if (result != CamxResultSuccess)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "CSIPhy Submodule creation failed");
        return result;
    }

    if (NULL == m_pCSIPHY)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "CSIPhy Pointer is NULL");
        return CamxResultEInvalidPointer;
    }

    AddCSLDeviceHandle(m_pCSIPHY->GetDeviceHandle());
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExternalSensor::GetInitialCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::GetInitialCameraInfo(
    StatsCameraInfo* pCameraInfo)
{
    CamxResult result   = CamxResultSuccess;
    UINT cameraPosition = 0;

    pCameraInfo->cameraId = m_cameraId;
    result                = m_pSensorModuleData->GetCameraPosition(&cameraPosition);

    pCameraInfo->algoRole   = StatsAlgoRoleDefault;

    CAMX_LOG_VERBOSE(CamxLogGroupStats, "Sensor initial camera info: ID:%d role:%d",
                     pCameraInfo->cameraId,
                     pCameraInfo->algoRole);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ExternalSensor::AddCSLDeviceHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::AddCSLDeviceHandle(
    CSLDeviceHandle hCslDeiveHandle)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(m_cslDeviceCount < CamxMaxDeviceIndex);

    if (m_cslDeviceCount < CamxMaxDeviceIndex)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Added CSL Handle: %p at index: %d", hCslDeiveHandle, m_cslDeviceCount);
        m_hCSLDeviceHandles[m_cslDeviceCount++] = hCslDeiveHandle;
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::PrepareStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::PrepareStreamOff()
{
    CamxResult result = CamxResultSuccess;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::CreatePerFrameUpdateResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ExternalSensor::CreatePerFrameUpdateResources()
{
    CamxResult result = CamxResultSuccess;

    if (NULL == m_pExposureInfo)
    {
        m_pExposureInfo = static_cast<VOID*>(GetSensorDataObject()->Allocate(SensorAllocateType::ExposureInfo));
    }

    if (NULL == m_pRegSettings)
    {
        m_pRegSettings = static_cast<VOID*>(GetSensorDataObject()->Allocate(SensorAllocateType::I2CRegSetting));
    }

    if (NULL == m_pExposureRegAddressInfo)
    {
        m_pExposureRegAddressInfo =
            static_cast<VOID*>(GetSensorDataObject()->Allocate(SensorAllocateType::ExposureRegAddressInfo));

        if (NULL != m_pExposureRegAddressInfo)
        {
            GetSensorDataObject()->UpdateExposureRegAddressInfo(m_pExposureRegAddressInfo);
        }
    }

    if ((NULL == m_pExposureInfo) || (NULL == m_pRegSettings) ||
        (NULL == m_pExposureRegAddressInfo))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Failed to allocate buffer ExposureInfo: %p, RegSettings: %p, RegAddressInfo: %p PDAFsettings =%p",
                       m_pExposureInfo,
                       m_pRegSettings,
                       m_pExposureRegAddressInfo);

        return CamxResultEInvalidPointer;
    }

    UINT maxUpdateCmdSize        = GetSensorDataObject()->GetI2CCmdMaxSize(I2CRegSettingType::AEC, m_pRegSettings);
    UINT maxMasterSettingSize    = GetSensorDataObject()->GetI2CCmdMaxSize(I2CRegSettingType::Master, NULL);
    UINT maxSlaveSettingSize     = GetSensorDataObject()->GetI2CCmdMaxSize(I2CRegSettingType::Slave, NULL);


    maxUpdateCmdSize            += maxMasterSettingSize > maxSlaveSettingSize ? maxMasterSettingSize : maxSlaveSettingSize;


    CAMX_LOG_INFO(CamxLogGroupSensor, "maxUpdateCmdSize: %d", maxUpdateCmdSize);

    CAMX_ASSERT(maxUpdateCmdSize != 0);
    CAMX_ASSERT(0 == (maxUpdateCmdSize % sizeof(UINT32)));

    // Create the update packet manager, command manager, packet and commands

    ResourceParams resourceParams                   = { 0 };
    resourceParams.usageFlags.packet                = 1;
    resourceParams.packetParams.maxNumCmdBuffers    = 1;
    resourceParams.packetParams.maxNumIOConfigs     = 0;
    resourceParams.resourceSize                     = Packet::CalculatePacketSize(&resourceParams.packetParams);
    resourceParams.poolSize                         = UpdateCmdCount * resourceParams.resourceSize;
    resourceParams.alignment                        = CamxPacketAlignmentInBytes;
    resourceParams.pDeviceIndices                   = NULL;
    resourceParams.numDevices                       = 0;
    resourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    if (CamxResultSuccess == CreateCmdBufferManager(&resourceParams, &m_pUpdatePacketManager))
    {
        ResourceParams params       = { 0 };
        params.resourceSize         = maxUpdateCmdSize;
        params.poolSize             = UpdateCmdCount * maxUpdateCmdSize;
        params.usageFlags.cmdBuffer = 1;
        params.cmdParams.type       = CmdType::I2C;
        params.alignment            = CamxCommandBufferAlignmentInBytes;
        params.pDeviceIndices       = NULL;
        params.numDevices           = 0;
        params.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager(&params, &m_pUpdateCmdBufferManager);
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::UpdateStartupExposureSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExternalSensor::UpdateStartupExposureSettings() {

    SensorParam updateParam  = { 0 };
    UINT64      exposureTime = m_pHwContext->GetStaticSettings()->sensorExposureTime;
    FLOAT       gain         = m_pHwContext->GetStaticSettings()->gain;

    updateParam.currentExposure         = exposureTime;
    updateParam.currentShortExposure    = exposureTime;
    updateParam.currentGain             = gain;
    updateParam.currentShortGain        = gain;
    updateParam.pRegControlData         = NULL;

    // Second argument can be NULL because we do not need to update slave exposure settings based on the master
    // sensor in the first frame
    PrepareSensorUpdate(&updateParam);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                     "Sensor: Update Startup Exposure: Gain = %f ExpTime = %ld, Short Gain = %f Short ExpTime = %ld FLL = %u",
                     updateParam.currentGain,
                     updateParam.currentExposure,
                     updateParam.currentShortGain,
                     updateParam.currentShortExposure,
                     updateParam.currentFrameLengthLines);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ExternalSensor::PrepareSensorUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ExternalSensor::PrepareSensorUpdate(
    SensorParam* pSensorParam)
{
    VOID*                             pRegControlInfo      = NULL;
    const ExposureContorlInformation* pExposureControlInfo = GetSensorDataObject()->GetExposureControlInfo();
    SensorSeamlessType                currentSeamlessType  = SensorSeamlessType::None;
    UINT64                            minExposureTime      =
        static_cast<UINT64>(GetSensorDataObject()->GetLineReadoutTime(m_currentResolutionIndex, currentSeamlessType));

    CAMX_ASSERT(NULL != pExposureControlInfo);

    if ((NULL == m_pExposureInfo) || (NULL == m_pRegSettings) || (NULL == m_pExposureRegAddressInfo))
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor,
                       "Invalid inputs: m_pExposureInfo: %p, m_pRegSettings: %p, RegAddressInfo: %p",
                       m_pExposureInfo,
                       m_pRegSettings,
                       m_pExposureRegAddressInfo);

        return;
    }

    if (pSensorParam->currentExposure < minExposureTime)
    {
        pSensorParam->currentExposure = minExposureTime;
    }
    if (pSensorParam->currentShortExposure < minExposureTime)
    {
        pSensorParam->currentShortExposure = minExposureTime;
    }

    pSensorParam->currentLineCount = GetSensorDataObject()->ExposureToLineCount(pSensorParam->currentExposure,
                                                                                m_currentResolutionIndex,
                                                                                currentSeamlessType);

    pSensorParam->currentShortLineCount = GetSensorDataObject()->ExposureToLineCount(pSensorParam->currentShortExposure,
                                                                                     m_currentResolutionIndex,
                                                                                     currentSeamlessType);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "AppliedExpTime=%llu", pSensorParam->currentExposure);

    if (pSensorParam->currentLineCount > pExposureControlInfo->maxLineCount)
    {
        pSensorParam->currentLineCount = pExposureControlInfo->maxLineCount;
        /// Updating the actual exposure in case if line count is capped to maxLineCount.
        pSensorParam->currentExposure = GetSensorDataObject()->LineCountToExposure(pSensorParam->currentLineCount,
                                                                                   m_currentResolutionIndex,
                                                                                   currentSeamlessType);
    }

    if (pSensorParam->currentShortLineCount > pExposureControlInfo->maxLineCount)
    {
        pSensorParam->currentShortLineCount = pExposureControlInfo->maxLineCount;
        pSensorParam->currentShortExposure = GetSensorDataObject()->LineCountToExposure(pSensorParam->currentShortLineCount,
                                                                                    m_currentResolutionIndex,
                                                                                    currentSeamlessType);
    }

    UINT32 frameLengthLines = pSensorParam->currentFrameLengthLines;
    frameLengthLines = static_cast<UINT32>(Utils::RoundDOUBLE(frameLengthLines * 1)); // GetFPSDiv()

    /// Make sure that minumum of vertical offset is maintained between line count and frame length lines
    if (pSensorParam->currentLineCount > (frameLengthLines - pExposureControlInfo->verticalOffset))
    {
        frameLengthLines = pSensorParam->currentLineCount + pExposureControlInfo->verticalOffset;
    }
    pSensorParam->currentFrameLengthLines = frameLengthLines;

    GetSensorDataObject()->CalculateExposure(pSensorParam->currentGain,
                                             pSensorParam->currentLineCount,
                                             pSensorParam->currentMiddleGain,
                                             pSensorParam->currentMiddleLineCount,
                                             pSensorParam->currentShortGain,
                                             pSensorParam->currentShortLineCount,
                                             m_currentResolutionIndex,
                                             m_pExposureInfo);

    if (NULL != pSensorParam->pRegControlData)
    {
        pRegControlInfo = reinterpret_cast<VOID*>(pSensorParam->pRegControlData->sensorControl.registerControl);
    }

    GetSensorDataObject()->FillExposureArray(m_pExposureInfo,
                                             frameLengthLines,
                                             pRegControlInfo,
                                             m_pExposureRegAddressInfo,
                                             m_pRegSettings,
                                             GetSensorDataObject()->IsHDRMode(m_currentResolutionIndex),
                                             GetSensorDataObject()->Is3ExposureHDRMode(m_currentResolutionIndex),
                                             m_currentResolutionIndex);
}
CAMX_NAMESPACE_END
