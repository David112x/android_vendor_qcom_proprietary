////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxeepromdata.cpp
/// @brief Implements EEPROMData methods. This will have EEPROM library data structure signatures
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcmdbuffer.h"
#include "camxcmdbuffermanager.h"
#include "camxcslsensordefs.h"
#include "camxdebug.h"
#include "camxhwdefs.h"
#include "camxhwenvironment.h"
#include "camximagebuffer.h"
#include "camximagesensorutils.h"
#include "camxmem.h"
#include "camxosutils.h"
#include "camxpacket.h"
#include "camxpacketdefs.h"
#include "camxstaticcaps.h"
#include "camxutils.h"
#include "camxeepromdata.h"
#include "camximagesensormoduledatamanager.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Static Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::EEPROMData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EEPROMData::EEPROMData(
    EEPROMDriverData*       pEEPROMDriverData,
    HwSensorInfo*           pSensorInfoTable,
    const HwDeviceTypeInfo* pDeviceInfo,
    CSLHandle               hCSL)
{
    CamxResult      result               = CamxResultEFailed;
    ResourceParams  packetResourceParams = {0};

    m_pEEPROMInitReadPacket         = NULL;
    m_pEEPROMDriverData             = pEEPROMDriverData;
    m_pSensorInfoTable              = pSensorInfoTable;
    m_EEPROMDeviceIndex             = -1;
    m_hEEPROMSessionHandle          = 0;
    m_isCSLOpenByEEPROM             = FALSE;
    m_hEEPROMDevice                 = 0;
    m_numberOfMemoryBlocks          = 0;
    m_pImageBufferManager           = NULL;
    m_pImage                        = NULL;
    m_pOTPData                      = NULL;
    m_OTPDataSize                   = 0;
    m_deviceAcquired                = FALSE;
    m_phEEPROMLibHandle             = NULL;
    m_EEPROMLibraryAPI.size         = 0;
    m_EEPROMLibraryAPI.majorVersion = 0;
    m_EEPROMLibraryAPI.minorVersion = 0;

    if (0 != hCSL)
    {
        m_hEEPROMSessionHandle = hCSL;
    }

    packetResourceParams.usageFlags.packet                = 1;
    // one for power sequence and one for memory map info
    packetResourceParams.packetParams.maxNumCmdBuffers    = 2;
    packetResourceParams.packetParams.maxNumIOConfigs     = 1;
    packetResourceParams.resourceSize                     = Packet::CalculatePacketSize(&packetResourceParams.packetParams);
    packetResourceParams.poolSize                         = packetResourceParams.resourceSize;
    packetResourceParams.alignment                        = CamxPacketAlignmentInBytes;
    packetResourceParams.pDeviceIndices                   = NULL;
    packetResourceParams.numDevices                       = 0;
    packetResourceParams.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

    result = CmdBufferManager::Create("EEPROMPacketManager", &packetResourceParams, &m_pPacketManager);

    if (CamxResultSuccess == result)
    {
        ParseMemoryMapData();

        result = InitializeCSL(m_pSensorInfoTable->CSLCapability.EEPROMSlotId, pDeviceInfo);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain CSL session for: %s", pEEPROMDriverData->slaveInfo.EEPROMName);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create packet manager");
    }

    if (CamxResultSuccess == result)
    {
        result = ReadEEPROMDevice();
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "Data read success for: %s", m_pEEPROMDriverData->slaveInfo.EEPROMName);
            m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData.pRawData =
                static_cast<BYTE*>(CAMX_CALLOC(m_OTPDataSize));
            if (NULL != m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData.pRawData)
            {
                Utils::Memcpy(m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData.pRawData, m_pOTPData, m_OTPDataSize);
                m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData.rawDataSize = m_OTPDataSize;
            }
            else
            {
                /// Not returning error as format data can still proceed without raw data available for other modules
                Utils::Memset(&(m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData),
                              0,
                              sizeof(m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData));
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to allocate memory for OTP raw data");
            }

            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                             "EEPROM rawData: %p, size: %d",
                             m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData.pRawData,
                             m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData.rawDataSize);

            LoadEEPROMLibrary();

            // @todo (CAMX-1996) - Implement library mechanism for OTP data parsing/formating.
            FormatAFData();
            FormatWBData();
            FormatLSCData();
            FormatSPCData();
            FormatQSCData();
            FormatOISData();
            FormatDualCameraData();
            FormatPDAFDCCData();
            FormatPDAF2DData();
            FormatLensData();
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "EEPROM read failed for: %s", m_pEEPROMDriverData->slaveInfo.EEPROMName);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::~EEPROMData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EEPROMData::~EEPROMData()
{
    if (0 != m_hEEPROMSessionHandle)
    {
        UnInititialize();
    }

    if (NULL != m_pPacketManager)
    {
        if (NULL != m_pEEPROMInitReadPacket)
        {
            m_pPacketManager->Recycle(m_pEEPROMInitReadPacket);
        }

        CAMX_DELETE m_pPacketManager;
        m_pPacketManager = NULL;
    }

    if (NULL != m_phEEPROMLibHandle)
    {
        OsUtils::LibUnmap(m_phEEPROMLibHandle);
        m_phEEPROMLibHandle = NULL;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::ParseMemoryMapData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::ParseMemoryMapData()
{
    INT16 memMapBlockIndex = -1;

    if ((NULL != m_pEEPROMDriverData->memoryMap.regSetting) && (0 < m_pEEPROMDriverData->memoryMap.regSettingCount))
    {
        for (UINT16 memoryMapIndex = 0; memoryMapIndex < m_pEEPROMDriverData->memoryMap.regSettingCount; memoryMapIndex++)
        {
            if (0 == memoryMapIndex)
            {
                /// if the slave address of the first setting is zero, then assign it as the address
                /// in slaveInfo section.
                if (0 == m_pEEPROMDriverData->memoryMap.regSetting[memoryMapIndex].slaveAddr)
                {
                    m_pEEPROMDriverData->memoryMap.regSetting[memoryMapIndex].slaveAddr =
                        m_pEEPROMDriverData->slaveInfo.slaveAddress;
                }

                memMapBlockIndex++;
                m_memoryMapParsedData[memMapBlockIndex].memoryBlockStartingIndex = memoryMapIndex;
            }
            // If the settings size of current block is bigger than PerBlock's threshold, move to next block.
            else if (MaximumNumberOfSettingsPerBlock == m_memoryMapParsedData[memMapBlockIndex].numberOfMemorySettings)
            {
                if (0 == m_pEEPROMDriverData->memoryMap.regSetting[memoryMapIndex].slaveAddr)
                {
                    m_pEEPROMDriverData->memoryMap.regSetting[memoryMapIndex].slaveAddr =
                        m_pEEPROMDriverData->slaveInfo.slaveAddress;
                }

                memMapBlockIndex++;
                m_memoryMapParsedData[memMapBlockIndex].memoryBlockStartingIndex = memoryMapIndex;
            }
            /// If the non-zero slave address is different with the slave address of previous
            /// setting, then move to next block to create a new I2CInfoCmd.
            else if (0 != m_pEEPROMDriverData->memoryMap.regSetting[memoryMapIndex].slaveAddr &&
                    (m_pEEPROMDriverData->memoryMap.regSetting[memoryMapIndex].slaveAddr !=
                    m_pEEPROMDriverData->memoryMap.regSetting[memoryMapIndex - 1].slaveAddr))
            {
                memMapBlockIndex++;
                m_memoryMapParsedData[memMapBlockIndex].memoryBlockStartingIndex = memoryMapIndex;
            }

            m_memoryMapParsedData[memMapBlockIndex].numberOfMemorySettings++;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to parse memory map");
    }

    m_numberOfMemoryBlocks = memMapBlockIndex + 1;
    CAMX_LOG_INFO(CamxLogGroupSensor, "Number of memory blocks: %d", m_numberOfMemoryBlocks);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::GetEEPROMCSLDeviceIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEPROMData::GetEEPROMCSLDeviceIndex(
    UINT32                  EEPROMSlotID,
    const HwDeviceTypeInfo* pDeviceInfo)
{
    UINT                    deviceIndex         = 0;
    CamxResult              result              = CamxResultEFailed;
    CSLEEPROMCapability     EEPROMCSLCapability = { 0 };

    // Find the index of the EEPROM devices
    do
    {
        result = CSLQueryDeviceCapabilities(pDeviceInfo->deviceIndex[deviceIndex],
                                            static_cast<VOID*>(&(EEPROMCSLCapability)),
                                            sizeof(EEPROMCSLCapability));

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                           "QueryCap for deviceindex: %d: slotInfo: %u, multiModule: %u",
                           pDeviceInfo->deviceIndex[deviceIndex],
                           EEPROMCSLCapability.slotInfo,
                           EEPROMCSLCapability.multiModule);
            // Compare the slot id and exit the loop when matching slot id is found
            if (EEPROMSlotID ==  EEPROMCSLCapability.slotInfo)
            {
                m_EEPROMDeviceIndex = pDeviceInfo->deviceIndex[deviceIndex];
                CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                                 "Found matching EEPROMSlotId:%d, EEPROM device Index: %d",
                                 EEPROMCSLCapability.slotInfo,
                                 m_EEPROMDeviceIndex);
                break;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupHWL, "QueryCap failed for EEPROM Device");
        }

        deviceIndex++;
    } while ((CamxResultSuccess == result) && (deviceIndex < pDeviceInfo->deviceIndexCount));

    if (-1 == m_EEPROMDeviceIndex)
    {
        result = CamxResultENoSuch;
        CAMX_LOG_ERROR(CamxLogGroupHWL,
                       "Failed to obtain EEPROM device index for %s",
                       m_pEEPROMDriverData->slaveInfo.EEPROMName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::InitializeCSL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEPROMData::InitializeCSL(
    UINT32                  EEPROMSlotID,
    const HwDeviceTypeInfo* pDeviceInfo)
{
    CamxResult result = CamxResultEFailed;

    result = GetEEPROMCSLDeviceIndex(EEPROMSlotID, pDeviceInfo);
    if (CamxResultSuccess == result)
    {
        if (0 == m_hEEPROMSessionHandle)
        {
            /// Need to create a new CSL session as EEPROM data can be read before the actual CSL session is created
            /// This session will be closed once the data is read based on m_isCSLOpenByEEPROM variable.
            result = CSLOpen(&m_hEEPROMSessionHandle);
            if (CamxResultSuccess == result)
            {
                m_isCSLOpenByEEPROM = TRUE;
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to open CSL session");
                return result;
            }
        }

        result = CSLAcquireDevice(m_hEEPROMSessionHandle,
                                  &m_hEEPROMDevice,
                                  m_EEPROMDeviceIndex,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  "EEPROM");
        if (CamxResultSuccess == result)
        {
            m_deviceAcquired = TRUE;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "AcquireDevice on EEPROM failed!");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "GetCSLDeviceIndex on EEPROM failed!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::UnInititialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEPROMData::UnInititialize()
{
    CamxResult result = CamxResultSuccess;

    if (TRUE == m_deviceAcquired)
    {
        result = CSLReleaseDevice(m_hEEPROMSessionHandle, m_hEEPROMDevice);

        if (CamxResultSuccess == result)
        {
            m_deviceAcquired = FALSE;
            CAMX_LOG_INFO(CamxLogGroupSensor, "ReleaseDevice on EEPROM succeed!");
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "ReleaseDevice on EEPROM failed!");
        }
    }

    if (TRUE == m_isCSLOpenByEEPROM)
    {
        result = CSLClose(m_hEEPROMSessionHandle);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "CSL Close on EEPROM failed, error: %d", result);
        }
    }

    if ((NULL != m_pImageBufferManager) && (NULL != m_pImage))
    {
        m_pImageBufferManager->ReleaseReference(m_pImage);
        m_pImage = NULL;

        m_pImageBufferManager->Destroy();
        m_pImageBufferManager = NULL;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROMData::GetPowerSequenceCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT EEPROMData::GetPowerSequenceCmdSize(
    BOOL isPowerUp)
{
    UINT            powerSequenceSize = 0;
    PowerSetting*   pPowerSettings    = NULL;

    if (TRUE == isPowerUp)
    {
        powerSequenceSize   = m_pEEPROMDriverData->slaveInfo.powerUpSequence.powerSettingCount;
        pPowerSettings      = m_pEEPROMDriverData->slaveInfo.powerUpSequence.powerSetting;
    }
    else
    {
        powerSequenceSize   = m_pEEPROMDriverData->slaveInfo.powerDownSequence.powerSettingCount;
        pPowerSettings      = m_pEEPROMDriverData->slaveInfo.powerDownSequence.powerSetting;
    }

    CAMX_ASSERT_MESSAGE((NULL != pPowerSettings), "Invalid power settings");

    return ImageSensorUtils::GetPowerSequenceCmdSize(powerSequenceSize, pPowerSettings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROMData::CreatePowerSequenceCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEPROMData::CreatePowerSequenceCmd(
    BOOL    isPowerUp,
    VOID*   pCmdBuffer)

{
    UINT            powerSequenceSize   = 0;
    PowerSetting*   pPowerSettings      = NULL;

    if (TRUE == isPowerUp)
    {
        powerSequenceSize   = m_pEEPROMDriverData->slaveInfo.powerUpSequence.powerSettingCount;
        pPowerSettings      = m_pEEPROMDriverData->slaveInfo.powerUpSequence.powerSetting;
    }
    else
    {
        powerSequenceSize   = m_pEEPROMDriverData->slaveInfo.powerDownSequence.powerSettingCount;
        pPowerSettings      = m_pEEPROMDriverData->slaveInfo.powerDownSequence.powerSetting;
    }

    CAMX_ASSERT_MESSAGE((NULL != pPowerSettings), "Invalid power settings");

    return ImageSensorUtils::CreatePowerSequenceCmd(pCmdBuffer, isPowerUp, powerSequenceSize, pPowerSettings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROMData::CreateI2CInfoCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEPROMData::CreateI2CInfoCmd(
    CSLSensorI2CInfo*   pI2CInfoCmd,
    UINT16              memMapBlockIndex)
{

    UINT16 slaveAddress = ((m_pEEPROMDriverData->memoryMap.regSetting) +
                           (m_memoryMapParsedData[memMapBlockIndex].memoryBlockStartingIndex))->slaveAddr;

    return ImageSensorUtils::CreateI2CInfoCmd(pI2CInfoCmd,
                                              slaveAddress,
                                              m_pEEPROMDriverData->slaveInfo.i2cFrequencyMode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROMData::GetMemoryMapCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT EEPROMData::GetMemoryMapCmdSize(
    UINT memMapBlockIndex)
{
    UINT                memMapCmdSize       = 0;
    UINT                memMapSettingsCount = 0;
    RegisterSetting*    pMemMapSettings     = NULL;
    UINT                regSettingIndex     = 0;
    memMapSettingsCount = m_memoryMapParsedData[memMapBlockIndex].numberOfMemorySettings;
    pMemMapSettings     = (m_pEEPROMDriverData->memoryMap.regSetting) +
                          (m_memoryMapParsedData[memMapBlockIndex].memoryBlockStartingIndex);
    memMapCmdSize       = ImageSensorUtils::GetRegisterSettingsCmdSize(memMapSettingsCount,
                                            pMemMapSettings, &regSettingIndex);

    return memMapCmdSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROMData::GetTotalMemoryMapCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT EEPROMData::GetTotalMemoryMapCmdSize()
{
    UINT totalSize = 0;

    for (UINT memMapBlockIndex = 0; memMapBlockIndex < m_numberOfMemoryBlocks; memMapBlockIndex++)
    {
        totalSize        = totalSize + GetMemoryMapCmdSize(memMapBlockIndex);
    }
    CAMX_LOG_INFO(CamxLogGroupSensor, "memory map commands totalSize =%d", totalSize);

    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROMData::CreateMemoryMapCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEPROMData::CreateMemoryMapCmd(
    VOID*   pCmdBuffer,
    UINT    memMapBlockIndex)
{
    CamxResult          result              = CamxResultSuccess;
    UINT                memMapSettingsCount = 0;
    RegisterSetting*    pMemMapSettings     = NULL;

    memMapSettingsCount = m_memoryMapParsedData[memMapBlockIndex].numberOfMemorySettings;
    pMemMapSettings     = (m_pEEPROMDriverData->memoryMap.regSetting) +
                          (m_memoryMapParsedData[memMapBlockIndex].memoryBlockStartingIndex);

    result    = ImageSensorUtils::CreateRegisterSettingsCmd(pCmdBuffer,
                                                            memMapSettingsCount,
                                                            pMemMapSettings,
                                                            0,
                                                            m_pEEPROMDriverData->slaveInfo.i2cFrequencyMode);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROMData::GetMemorySizeBytes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T EEPROMData::GetMemorySizeBytes()
{
    SIZE_T maxMemMapSize = 0;

    for (UINT count = 0; count < m_pEEPROMDriverData->memoryMap.regSettingCount; count++)
    {
        if (OperationType::READ == (m_pEEPROMDriverData->memoryMap.regSetting + count)->operation)
        {
            maxMemMapSize += (m_pEEPROMDriverData->memoryMap.regSetting + count)->registerData[0];
        }
    }

    CAMX_LOG_INFO(CamxLogGroupSensor, "maxMemmapSize = %d", maxMemMapSize);
    return maxMemMapSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// EEPROMData::GetCommandBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEPROMData::GetCommandBuffer(
    CmdBufferManager*   pCmdBufferManager,
    PacketResource**    ppPacketResource)
{
    CamxResult result = pCmdBufferManager->GetBuffer(ppPacketResource);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get buffer: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::ReadEEPROMDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEPROMData::ReadEEPROMDevice()
{
    CamxResult        result                   = CamxResultSuccess;
    CmdBufferManager* pCmdManagerPowerSequence = NULL;
    CmdBufferManager* pCmdManagerMemoryMapInfo = NULL;
    PacketResource*   pPacketResource          = NULL;
    CmdBuffer*        pPowerSequenceCmd        = NULL;
    CmdBuffer*        pMemoryMapCmd            = NULL;
    UINT              powerUpCmdSize           = GetPowerSequenceCmdSize(TRUE);
    UINT              powerDownCmdSize         = GetPowerSequenceCmdSize(FALSE);
    UINT              powerSequenceSize        = (powerUpCmdSize + powerDownCmdSize);
    UINT              I2CInfoCmdSize           = sizeof(CSLSensorI2CInfo);
    UINT              memoryMapCmdSize         = GetTotalMemoryMapCmdSize();
    UINT              memoryMapSize            = (I2CInfoCmdSize * m_numberOfMemoryBlocks) + memoryMapCmdSize;
    UINT32            cmdBufferIndex           = 0;

    // Step1: Initialize the EEPROM init read packet
    if (NULL != m_pPacketManager)
    {
        result = GetCommandBuffer(m_pPacketManager, &pPacketResource);
    }
    else
    {
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);
        // pResource points to a Packet so we may static_cast
        m_pEEPROMInitReadPacket = static_cast<Packet*>(pPacketResource);
    }

    // Step2: Initialize and add power sequence commnds to the packet
    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams = {0};

        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.resourceSize         = powerSequenceSize;
        cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
        cmdResourceParams.cmdParams.type       = CmdType::Generic;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CmdBufferManager::Create("EEPROMCmdManagerPowerSequence", &cmdResourceParams, &pCmdManagerPowerSequence);

        if (CamxResultSuccess == result)
        {
            result = GetCommandBuffer(pCmdManagerPowerSequence, &pPacketResource);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create Power Sequence cmdbuffermanager");
        }

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
            pPowerSequenceCmd = static_cast<CmdBuffer*>(pPacketResource);

            // step2a: Commit power up command
            VOID* pCmdPowerUp = pPowerSequenceCmd->BeginCommands(powerUpCmdSize / sizeof(UINT32));
            if (NULL != pCmdPowerUp)
            {
                if (CamxResultSuccess == CreatePowerSequenceCmd(TRUE, pCmdPowerUp))
                {
                    result = pPowerSequenceCmd->CommitCommands();
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create power up command");
                    result = CamxResultEFailed;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve powerup command area in command buffer");
                result = CamxResultEFailed;
            }

            // step2b: Commit power down command
            if (CamxResultSuccess == result)
            {
                VOID* pCmdPowerDown = pPowerSequenceCmd->BeginCommands(powerDownCmdSize / sizeof(UINT32));
                if (NULL != pCmdPowerDown)
                {
                    if (CamxResultSuccess == CreatePowerSequenceCmd(FALSE, pCmdPowerDown))
                    {
                        result = pPowerSequenceCmd->CommitCommands();
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create power down command");
                        result = CamxResultEFailed;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve powerdown command area in command buffer");
                    result = CamxResultEFailed;
                }
            }

            // step2c: Add power sequence (power up + power down) command buffer to packet
            if (CamxResultSuccess == result)
            {
                result = m_pEEPROMInitReadPacket->AddCmdBufferReference(pPowerSequenceCmd, &cmdBufferIndex);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain power command buffer");
        }
    }

    // Step3: Initialize and add memory map commnds(i2c info and memory details) to the packet
    if (CamxResultSuccess == result)
    {
        ResourceParams cmdResourceParams = {0};

        cmdResourceParams.usageFlags.cmdBuffer = 1;
        cmdResourceParams.resourceSize         = memoryMapSize;
        cmdResourceParams.poolSize             = cmdResourceParams.resourceSize;
        cmdResourceParams.cmdParams.type       = CmdType::I2C;
        cmdResourceParams.alignment            = CamxCommandBufferAlignmentInBytes;
        cmdResourceParams.pDeviceIndices       = NULL;
        cmdResourceParams.numDevices           = 0;
        cmdResourceParams.memFlags             = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CmdBufferManager::Create("EEPROMCmdManagerMemoryMapInfo", &cmdResourceParams, &pCmdManagerMemoryMapInfo);

        if (CamxResultSuccess == result)
        {
            result = GetCommandBuffer(pCmdManagerMemoryMapInfo, &pPacketResource);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create Memory Map Info cmdbuffermanager");
        }

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
            pMemoryMapCmd = static_cast<CmdBuffer*>(pPacketResource);

            for (UINT16 memMapBlockIndex = 0; memMapBlockIndex < m_numberOfMemoryBlocks; memMapBlockIndex++)
            {
                // Step3a: Commit the I2cInfo command specific to the memory block
                CSLSensorI2CInfo* pCmdI2CInfo =
                    reinterpret_cast<CSLSensorI2CInfo*>(pMemoryMapCmd->BeginCommands(I2CInfoCmdSize / sizeof(UINT32)));
                if (NULL != pCmdI2CInfo)
                {
                    if (CamxResultSuccess == CreateI2CInfoCmd(pCmdI2CInfo, memMapBlockIndex))
                    {
                        result = pMemoryMapCmd->CommitCommands();
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create i2c command");
                        result = CamxResultEFailed;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve i2c info command area in command buffer");
                    result = CamxResultEFailed;
                }

                // Step3b: Commit the memorymap command specific to the memory block
                if (CamxResultSuccess == result)
                {
                    VOID* pCmdMemoryMapInfo =
                        pMemoryMapCmd->BeginCommands(GetMemoryMapCmdSize(memMapBlockIndex) / sizeof(UINT32));
                    if (NULL != pCmdMemoryMapInfo)
                    {
                        if (CamxResultSuccess == CreateMemoryMapCmd(pCmdMemoryMapInfo, memMapBlockIndex))
                        {
                            result = pMemoryMapCmd->CommitCommands();
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create memory map command");
                            result = CamxResultEFailed;
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to reserve memory map command area in command buffer");
                        result = CamxResultEFailed;
                    }
                }
            }

            // step3c: Add memory map (I2C info + memory info) command buffer to packet
            if (CamxResultSuccess == result)
            {
                // Not associated with any request. Won't be recycled.
                m_pEEPROMInitReadPacket->SetRequestId(CamxInvalidRequestId);
                result = m_pEEPROMInitReadPacket->AddCmdBufferReference(pMemoryMapCmd, &cmdBufferIndex);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain memory map command buffer");
        }
    }

    // Step4: Initialize and add read data commnds to the packet
    if (CamxResultSuccess == result)
    {
        BufferManagerCreateData createData  = { };
        ImageFormat*            pFormat     = &createData.bufferProperties.imageFormat;
        UINT32                  IOConfigIndex;

        m_OTPDataSize    = static_cast<UINT32>(GetMemorySizeBytes());
        pFormat->width     = m_OTPDataSize;
        pFormat->height    = 1;
        pFormat->format    = Format::Blob;
        pFormat->alignment = 8;

        createData.maxBufferCount               = 1;
        createData.immediateAllocBufferCount    = 1;
        createData.bufferProperties.memFlags    = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
        createData.allocateBufferMemory         = TRUE;
        createData.numBatchedFrames             = 1;
        createData.bufferProperties.bufferHeap  = CSLBufferHeapIon;
        createData.bufferManagerType            = BufferManagerType::CamxBufferManager;

        result = ImageBufferManager::Create("EEPROM_OTP", &createData, &m_pImageBufferManager);

        if (CamxResultSuccess == result)
        {
            m_pImage = m_pImageBufferManager->GetImageBuffer();

            if (NULL == m_pImage)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to obtain image buffer");
                result = CamxResultEFailed;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to obtain image buffer manager");
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            result = m_pEEPROMInitReadPacket->AddIOConfig(m_pImage,
                                                          0,
                                                          CSLIODirection::CSLIODirectionOutput,
                                                          NULL,
                                                          0,
                                                          &IOConfigIndex,
                                                          NULL,
                                                          0);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to add IO config =%d", result);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to allocate image buffer");
        }
    }

    // Step5: Add opcode and commit the packet
    if (CamxResultSuccess == result)
    {
        m_pEEPROMInitReadPacket->SetOpcode(CSLDeviceTypeEEPROM, CSLPacketOpcodesEEPROMInitialConfig);
        result = m_pEEPROMInitReadPacket->CommitPacket();
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to commit packet =%d", result);
        }
    }

    // Step6: Submit the prepared packet to CSL/KMD
    if (CamxResultSuccess == result)
    {
        result = CSLSubmit(m_hEEPROMSessionHandle,
                           m_hEEPROMDevice,
                           m_pEEPROMInitReadPacket->GetMemHandle(),
                           m_pEEPROMInitReadPacket->GetOffset());
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "failed to submit packet =%d", result);
        }
    }

    // Step6: get the buffer to which OTP data copied from CSL
    // Note: This logic works fine as the specifc IOCTL is synchronous but will use the fence logic to comply with design.
    // @todo (CAMX-1996) - Use fence to read the buffer in order to comply with the design.
    if (CamxResultSuccess == result)
    {
        m_pOTPData = m_pImage->GetPlaneVirtualAddr(0, 0);
        if (NULL == m_pOTPData)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to obtain OTP memory buffer, result =%d", result);
        }
    }

    // Clean up the resources
    if (NULL != pCmdManagerPowerSequence)
    {
        pCmdManagerPowerSequence->Recycle(pPowerSequenceCmd);
        CAMX_DELETE pCmdManagerPowerSequence;
    }

    if (NULL != pCmdManagerMemoryMapInfo)
    {
        pCmdManagerMemoryMapInfo->Recycle(pMemoryMapCmd);
        CAMX_DELETE pCmdManagerMemoryMapInfo;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::LoadEEPROMLibrary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult EEPROMData::LoadEEPROMLibrary()
{
    CamxResult  result                                      = CamxResultSuccess;
    CHAR        symbolName[]                                = "GetEEPROMLibraryAPIs";
    UINT16      fileCount                                   = 0;
    CHAR        libFiles[MaxSensorModules][FILENAME_MAX]    = {""};

    fileCount = OsUtils::GetFilesFromPath(SensorModulesPath,
                                          FILENAME_MAX,
                                          &libFiles[0][0],
                                          "*",
                                          "eeprom",
                                          m_pEEPROMDriverData->slaveInfo.EEPROMName,
                                          "*",
                                          SharedLibraryExtension);

    if (0 != fileCount)
    {
        CAMX_ASSERT(1 == fileCount);

        m_phEEPROMLibHandle = OsUtils::LibMap(&libFiles[0][0]);

        if (NULL != m_phEEPROMLibHandle)
        {
            typedef VOID(*GetEEPROMLibraryAPIs)(EEPROMLibraryAPI* pEEPROMLibraryAPI);

            GetEEPROMLibraryAPIs pfnGetEEPROMLibraryAPI =
                reinterpret_cast<GetEEPROMLibraryAPIs>(OsUtils::LibGetAddr(m_phEEPROMLibHandle, symbolName));
            if (NULL != pfnGetEEPROMLibraryAPI)
            {
                pfnGetEEPROMLibraryAPI(&m_EEPROMLibraryAPI);
                CAMX_LOG_INFO(CamxLogGroupSensor,
                              "EEPROM library loaded for %s, version Major: %d, Minor: %d",
                              m_pEEPROMDriverData->slaveInfo.EEPROMName,
                              m_EEPROMLibraryAPI.majorVersion,
                              m_EEPROMLibraryAPI.minorVersion);
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Couldn't find symbol: %s in %s", symbolName, &libFiles[0][0]);
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Unable to open EEPROM library: %s", &libFiles[0][0]);
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "EEPROM library not present for %s", m_pEEPROMDriverData->slaveInfo.EEPROMName);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::MaskLengthInBits
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 EEPROMData::MaskLengthInBits(
    UINT mask)
{
    UINT32 lastBitSetIndex = 0;

    // Mask is filled with 1's to indicate desired/valid bits in the read OTP data.
    // so find the position of last bit set from MSB
    CamX::Utils::BitScanReverse(mask, &lastBitSetIndex);

    // Add +1 to result as the above util function returns the 0 based index.
    return lastBitSetIndex + 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::MaskLengthInBytes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 EEPROMData::MaskLengthInBytes(
    UINT mask)
{
    UINT32 numberOfBits = 0;

    numberOfBits = MaskLengthInBits(mask);
    return (((numberOfBits % BitsPerByte) != 0) ? ((numberOfBits / BitsPerByte) + 1) : (numberOfBits / BitsPerByte));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatDataTypeByte
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 EEPROMData::FormatDataTypeByte(
    MemoryInfo* pMemoryInfo)
{
    UINT8   returnValue             = 0;
    UINT32  mask                    = pMemoryInfo->mask;
    UINT32  setBitIndex             = 0;

    // Find the index(Index starts from 0) of the first 1 bit from LSB
    CamX::Utils::BitScanForward(mask, &setBitIndex);

    // Get the data byte from OTP data and shift to actual data using the mask and data postion
    returnValue = (((m_pOTPData[pMemoryInfo->offset]) & (pMemoryInfo->mask)) >> (setBitIndex));

    return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatDataTypeInteger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 EEPROMData::FormatDataTypeInteger(
    MemoryInfo* pMemoryInfo,
    EndianType  endian)
{
    INT32     returnValue     = 0;
    UINT32    numberOfBytes   = MaskLengthInBytes(pMemoryInfo->mask);
    UINT32    numberOfBits    = MaskLengthInBits(pMemoryInfo->mask);

    switch (endian)
    {
        case EndianType::BIG:
            // Read MSB which is at low address in the buffer and left shift by 8 and so on until all the number of bytes
            // specifiied by mask are read to store the Big endian data from buff to integer.
            for (UINT16 index = 0; index < numberOfBytes; index++)
            {
                returnValue = (returnValue << BitsPerByte) | (m_pOTPData[pMemoryInfo->offset + index]);
            }
            break;
        case EndianType::LITTLE:
            // Read MSB which is at high address in the buffer and left shift by 8 and so on until all the number
            // of bytes specifiied by mask are read to store the Little endian data from buff to integer.
            for (; numberOfBytes > 0; numberOfBytes--)
            {
                returnValue = (returnValue << BitsPerByte) | (m_pOTPData[pMemoryInfo->offset + (numberOfBytes - 1)]);
            }
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid format\n");
            break;
    }

    if (SignType::SIGNED == pMemoryInfo->signedness)
    {
        // Bit shifts done to preserve the sign bit information
        numberOfBytes = MaskLengthInBytes(pMemoryInfo->mask);
        returnValue   = ((returnValue) << (BitsPerInteger - (numberOfBytes * (BitsPerByte)))) >>
                        (BitsPerInteger - numberOfBits);
    }

    return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatDataTypeFloat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FLOAT EEPROMData::FormatDataTypeFloat(
    MemoryInfo* pMemoryInfo)
{
    FLOAT  returnValue = 0.0;

    /// Float type use IEEE 754 standard store the data, it does not specify endianness.
    /// Copy the individual bytes of data to float variable directly.
    memcpy(&returnValue, &m_pOTPData[pMemoryInfo->offset], sizeof(FLOAT));

    return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatAFData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::FormatAFData()
{
    AFInfo*             pAFFormatInfo   = &(m_pEEPROMDriverData->formatInfo.AF);
    AFCalibrationData*  pAFData         = &(m_pSensorInfoTable->moduleCaps.OTPData.AFCalibration);

    pAFData->isAvailable                = pAFFormatInfo->autoFocusData.isAvailable;

    if (TRUE == pAFFormatInfo->autoFocusData.isAvailable)
    {
        pAFData->macroDAC               =
            static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->macro), pAFFormatInfo->autoFocusData.endianness));
        pAFData->infinityDAC            =
            static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->infinity), pAFFormatInfo->autoFocusData.endianness));

        /// If AF infinity and macro positions are calibrated with lans sag,
        /// then apply the gravity offset to the otp/eeprom calibrated infinity and macro position.
        ///   DEFAULT    : Deg 90, Default, faced forward -- no need to apply gravity offset to the original value
        ///   DEG0_OTP   : Faced up, OTP/EEPROM calibrated
        ///   DEG0_AVG   : Faced up, Average value
        ///                In case of faced up : new infinity/macro = infinity/macro - gravity offset
        ///   DEG180_OTP : Faced down, OTP/EEPROM calibrated
        ///   DEG180_AVG : Faced down, Average value
        ///                In case of faced down : new infinity/macro = infinity/macro + gravity offset
        AfLensSagType lensSag;
        INT16         gravityOfs;
        /// Check for gravity offset for macro calibration
        lensSag = pAFFormatInfo->lensSagCalMac;
        if (AfLensSagType::DEFAULT != lensSag)
        {
            gravityOfs = 0;
            if ((AfLensSagType::DEG0_OTP == lensSag) && (pAFFormatInfo->otpGravityOfs0to90.mask))
            {
                gravityOfs        =
                    static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->otpGravityOfs0to90),
                                        pAFFormatInfo->autoFocusData.endianness));
                gravityOfs        = -gravityOfs;
            }
            else if ((AfLensSagType::DEG180_OTP == lensSag) && (pAFFormatInfo->otpGravityOfs90to180.mask))
            {
                gravityOfs        =
                    static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->otpGravityOfs90to180),
                                        pAFFormatInfo->autoFocusData.endianness));
            }
            else if (AfLensSagType::DEG0_AVG == lensSag)
            {
                gravityOfs        = -pAFFormatInfo->avgGravityOfs0to90;
            }
            else if (AfLensSagType::DEG180_AVG == lensSag)
            {
                gravityOfs        = pAFFormatInfo->avgGravityOfs90to180;
            }
            // apply gravity offset to macro cal position
            pAFData->macroDAC += gravityOfs;
        }
        /// Check for gravity offset for infinity calibration
        lensSag = pAFFormatInfo->lensSagCalInf;
        if (AfLensSagType::DEFAULT != lensSag)
        {
            gravityOfs = 0;
            if ((AfLensSagType::DEG0_OTP == lensSag) && (pAFFormatInfo->otpGravityOfs0to90.mask))
            {
                gravityOfs        =
                    static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->otpGravityOfs0to90),
                                        pAFFormatInfo->autoFocusData.endianness));
                gravityOfs        = -gravityOfs;
            }
            else if ((AfLensSagType::DEG180_OTP == lensSag) && (pAFFormatInfo->otpGravityOfs90to180.mask))
            {
                gravityOfs        =
                    static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->otpGravityOfs90to180),
                                        pAFFormatInfo->autoFocusData.endianness));
            }
            else if (AfLensSagType::DEG0_AVG == lensSag)
            {
                gravityOfs        = -pAFFormatInfo->avgGravityOfs0to90;
            }
            else if (lensSag == AfLensSagType::DEG180_AVG)
            {
                gravityOfs        = pAFFormatInfo->avgGravityOfs90to180;
            }
            // apply gravity offset to infinity cal position
            pAFData->infinityDAC += gravityOfs;
        }

        INT16 horizontalMacroDAC        =
            static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->horizontalMacro),
                               pAFFormatInfo->autoFocusData.endianness));
        INT16 horizontalInfinityDAC     =
            static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->horizontalInfinity),
                               pAFFormatInfo->autoFocusData.endianness));
        INT16 verticalMacroDAC          =
            static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->verticalMacro),
                               pAFFormatInfo->autoFocusData.endianness));
        INT16 vertcalInfinityDAC        =
            static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->verticalInfinity),
                               pAFFormatInfo->autoFocusData.endianness));

        /// If there are valid horizontal and vertical macros, assign the largest of those two as macro DAC to
        /// accommodate gravity effects
        if (pAFFormatInfo->horizontalMacro.mask && pAFFormatInfo->verticalMacro.mask)
        {
            pAFData->macroDAC           =
                (horizontalMacroDAC > verticalMacroDAC) ? horizontalMacroDAC : verticalMacroDAC;
        }

        /// If there are valid horizontal and vertical infinity, assign the smallest of those two as infinity DAC to
        /// accommodate gravity effects
        if (pAFFormatInfo->horizontalInfinity.mask && pAFFormatInfo->verticalInfinity.mask)
        {
            pAFData->infinityDAC        =
                (horizontalInfinityDAC < vertcalInfinityDAC) ? horizontalInfinityDAC : vertcalInfinityDAC;
        }

        if ((0 < pAFFormatInfo->calibrationInfoCount) && (MaxAFCalibrationDistances >= pAFFormatInfo->calibrationInfoCount))
        {
            pAFData->numberOfDistances = pAFFormatInfo->calibrationInfoCount;
            for (UINT32 count = 0; count < pAFFormatInfo->calibrationInfoCount; count++)
            {
                pAFData->calibrationInfo[count].chartDistanceCM =
                    FormatDataTypeInteger(&(pAFFormatInfo->calibrationInfo[count].chartDistanceCM),
                                          pAFFormatInfo->autoFocusData.endianness);
                pAFData->calibrationInfo[count].DACValue        =
                    static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->calibrationInfo[count].DACValue),
                                          pAFFormatInfo->autoFocusData.endianness));
            }
        }

        UINT8 hallOffset            =
            static_cast<UINT8>(FormatDataTypeInteger(&(pAFFormatInfo->hall), pAFFormatInfo->autoFocusData.endianness));
        UINT8 hallBias              =
            static_cast<UINT8>(FormatDataTypeInteger(&(pAFFormatInfo->hallBias), pAFFormatInfo->autoFocusData.endianness));
        pAFData->hallOffsetBias     = (hallOffset << 8) | (hallBias);
        pAFData->hallRegisterAddr   = pAFFormatInfo->hallRegisterAddr;
        pAFData->infinityMargin     = pAFFormatInfo->infinityMargin;
        pAFData->macroMargin        = pAFFormatInfo->macroMargin;

        // Read lens sag offsets
        if (pAFFormatInfo->otpGravityOfs0to90.mask != 0)
        {
            pAFData->gravityOffset0to90 =
                static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->otpGravityOfs0to90),
                                    pAFFormatInfo->autoFocusData.endianness));
        }
        else
        {
            pAFData->gravityOffset0to90 = pAFFormatInfo->avgGravityOfs0to90;
        }

        if (pAFFormatInfo->otpGravityOfs90to180.mask != 0)
        {
            pAFData->gravityOffset90to180 =
                static_cast<INT16>(FormatDataTypeInteger(&(pAFFormatInfo->otpGravityOfs90to180),
                                    pAFFormatInfo->autoFocusData.endianness));
        }
        else
        {
            pAFData->gravityOffset90to180 = pAFFormatInfo->avgGravityOfs90to180;
        }

        if (TRUE == pAFFormatInfo->actuatorIDExists)
        {
            pAFData->actuatorID = static_cast<UINT32>(FormatDataTypeInteger(&(pAFFormatInfo->actuatorID),
                                                                            pAFFormatInfo->autoFocusData.endianness));
        }

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatWBData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::FormatWBData()
{
    WBInfo*             pWBFormatInfo   = &(m_pEEPROMDriverData->formatInfo.WB);
    WBCalibrationData*  pWBData         = &(m_pSensorInfoTable->moduleCaps.OTPData.WBCalibration[0]);
    RegisterSetting*    pRegSetting     = NULL;
    RegisterData*       pRegData        = NULL;

    if (0.0f == pWBFormatInfo->qValue)
    {
        pWBFormatInfo->qValue = 1.0f;
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "incorrect qValue configured, so defaulting it to 1");
    }

    if ((FALSE == pWBFormatInfo->WBData.isAvailable) && (NULL != m_EEPROMLibraryAPI.pFormatWBSettings))
    {
        m_EEPROMLibraryAPI.pFormatWBSettings(m_pOTPData, m_OTPDataSize,
            reinterpret_cast<WBCalibration*>(pWBData));

        if (0 != pWBData->settings.regSettingCount)
        {
            pRegSetting =
            static_cast<RegisterSetting*>(CAMX_CALLOC((sizeof(RegisterSetting)) * (pWBData->settings.regSettingCount)));

            if (NULL != pRegSetting)
            {
                UINT32 registerDataCount = 0;
                for (UINT32 i = 0; i < pWBData->settings.regSettingCount; i++)
                {
                    registerDataCount += pWBData->settings.regSetting[i].registerDataCount;
                }

                if (0 != registerDataCount)
                {
                    pRegData =
                    static_cast<RegisterData*>(CAMX_CALLOC(sizeof(RegisterData) * registerDataCount));
                }

                /// copy to local structures because life time of settings
                if (NULL != pRegData)
                {
                    RegisterData temp = 0;

                    Utils::Memcpy(pRegSetting, pWBData->settings.regSetting,
                        (sizeof(RegisterSetting)) * (pWBData->settings.regSettingCount));

                    pWBData->settings.regSetting = pRegSetting;
                    for (UINT32 i = 0; i < pWBData->settings.regSettingCount; i++)
                    {
                        temp = pWBData->settings.regSetting[i].registerData[0];
                        pWBData->settings.regSetting[i].registerData = &pRegData[i];
                        pWBData->settings.regSetting[i].registerData[0] = temp;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "copy to local failed");
                    CAMX_FREE(pRegSetting);
                    pWBData->settings.regSetting = NULL;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "regSetting alloc failed");
            }
        }
    }
    else if ((TRUE == pWBFormatInfo->WBData.isAvailable) && (WBType::RATIO == pWBFormatInfo->datatype))
    /// If the OTP data contains the ratio values
    {
        UINT8 mirror                    =
            static_cast<UINT8>(FormatDataTypeInteger(&(pWBFormatInfo->mirror), pWBFormatInfo->WBData.endianness));

        /// Copying the OTP data in the same order as its configured in xml as per the AWB algorithm requirement.
        for (UINT8 count = 0; count < pWBFormatInfo->lightInfoCount; count++)
        {
            pWBData[count].isAvailable  = pWBFormatInfo->WBData.isAvailable;
            pWBData[count].illuminant   = pWBFormatInfo->lightInfo[count].illuminantType;
            pWBData[count].bOverG       =
                static_cast<FLOAT>(FormatDataTypeInteger(&(pWBFormatInfo->lightInfo + count)->bOverGValue,
                                                         pWBFormatInfo->WBData.endianness)) / (pWBFormatInfo->qValue);
            pWBData[count].rOverG       =
                static_cast<FLOAT>(FormatDataTypeInteger(&(pWBFormatInfo->lightInfo + count)->rOverGValue,
                                                         pWBFormatInfo->WBData.endianness)) / (pWBFormatInfo->qValue);
            pWBData[count].grOverGB     =
                static_cast<FLOAT>(FormatDataTypeInteger(&(pWBFormatInfo->lightInfo + count)->grOverGBValue,
                                                         pWBFormatInfo->WBData.endianness)) / (pWBFormatInfo->qValue);

            /// This logic needs to be re-visited
            if (( 0 != mirror) && (TRUE == pWBFormatInfo->isInvertGROverGB))
            {
                pWBData[count].grOverGB = 1.0f / pWBData[count].grOverGB;
            }
        }
    }
    else if ((TRUE == pWBFormatInfo->WBData.isAvailable) && (WBType::INDIVIDUAL == pWBFormatInfo->datatype))
    {
        // If the OTP data contains the individual values, read them and compute the ratio.
        UINT8 mirror                    =
            static_cast<UINT8>(FormatDataTypeInteger(&(pWBFormatInfo->mirror), pWBFormatInfo->WBData.endianness));

        /// Copying the WB data in the same order as its configured in xml/OTP as per the AWB algorithm requirement.
        for (UINT8 count = 0; count < pWBFormatInfo->lightInfoCount; count++)
        {
            pWBData[count].isAvailable  = pWBFormatInfo->WBData.isAvailable;
            pWBData[count].illuminant   = pWBFormatInfo->lightInfo[count].illuminantType;

            INT32 rValue  = FormatDataTypeInteger(&(pWBFormatInfo->lightInfo + count)->rValue,
                                                  pWBFormatInfo->WBData.endianness);
            INT32 grValue = FormatDataTypeInteger(&(pWBFormatInfo->lightInfo + count)->grValue,
                                                  pWBFormatInfo->WBData.endianness);
            INT32 gbValue = FormatDataTypeInteger(&(pWBFormatInfo->lightInfo + count)->gbValue,
                                                  pWBFormatInfo->WBData.endianness);
            INT32 bValue  = FormatDataTypeInteger(&(pWBFormatInfo->lightInfo + count)->bValue,
                                                  pWBFormatInfo->WBData.endianness);

            if (0 != gbValue)
            {
                pWBData[count].rOverG      = ((static_cast<FLOAT>(rValue) / gbValue) / pWBFormatInfo->qValue);
                pWBData[count].bOverG      = ((static_cast<FLOAT>(bValue) / gbValue) / pWBFormatInfo->qValue);
                pWBData[count].grOverGB    = ((static_cast<FLOAT>(grValue) / gbValue) / pWBFormatInfo->qValue);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid gbValue in the OTP data");
            }

            if (( 0 != mirror) && (TRUE == pWBFormatInfo->isInvertGROverGB) && (0 != pWBData[count].grOverGB))
            {
                pWBData[count].grOverGB = 1.0f / pWBData[count].grOverGB;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatLSCData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::FormatLSCData()
{
    LSCInfo*            pLSCFormatInfo  = &(m_pEEPROMDriverData->formatInfo.LSC);
    LSClightInfo*       pLSCLightInfo   = NULL;
    LSCCalibrationData* pLSCData        = &(m_pSensorInfoTable->moduleCaps.OTPData.LSCCalibration[0]);
    UINT8               valueLSB        = 0;
    UINT8               valueMSB        = 0;
    RegisterSetting*    pRegSetting     = NULL;
    RegisterData*       pRegData        = NULL;

    if ((FALSE == pLSCFormatInfo->LSCData.isAvailable) && (NULL != m_EEPROMLibraryAPI.pFormatLSCSettings))
    {
        m_EEPROMLibraryAPI.pFormatLSCSettings(m_pOTPData, m_OTPDataSize,
        reinterpret_cast<LSCCalibration*>(pLSCData));

        if (0 != pLSCData->settings.regSettingCount)
        {
            pRegSetting =
            static_cast<RegisterSetting*>(CAMX_CALLOC((sizeof(RegisterSetting)) * (pLSCData->settings.regSettingCount)));

            if (NULL != pRegSetting)
            {
                UINT32 registerDataCount = 0;
                for (UINT32 i = 0; i < pLSCData->settings.regSettingCount; i++)
                {
                    registerDataCount += pLSCData->settings.regSetting[i].registerDataCount;
                }

                if (0 != registerDataCount)
                {
                    pRegData =
                    static_cast<RegisterData*>(CAMX_CALLOC(sizeof(RegisterData) * registerDataCount));
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "pRegData alloc %d", registerDataCount);
                }

                /// copy to local structures because life time of settings
                if (NULL != pRegData)
                {
                    RegisterData temp = 0;

                    Utils::Memcpy(pRegSetting, pLSCData->settings.regSetting,
                        (sizeof(RegisterSetting)) * (pLSCData->settings.regSettingCount));

                    pLSCData->settings.regSetting = pRegSetting;
                    for (UINT32 i = 0; i < pLSCData->settings.regSettingCount; i++)
                    {
                        temp = pLSCData->settings.regSetting[i].registerData[0];
                        pLSCData->settings.regSetting[i].registerData = &pRegData[i];
                        pLSCData->settings.regSetting[i].registerData[0] = temp;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupSensor, "copy to local failed");
                    CAMX_FREE(pRegSetting);
                    pLSCData->settings.regSetting = NULL;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "regSetting alloc failed");
            }
        }
    }
    else if (TRUE == pLSCFormatInfo->LSCData.isAvailable)
    {
        CAMX_ASSERT((NULL != pLSCFormatInfo->lightInfo) && (0 != pLSCFormatInfo->lightInfoCount));

        if (HWRollOffTableSize < pLSCFormatInfo->meshHWRollOffSize)
        {
            CAMX_LOG_INFO(CamxLogGroupSensor, "meshHWRollOffSize out of bound");
            pLSCFormatInfo->meshHWRollOffSize = HWRollOffTableSize;
        }

        /// copy to local structures so that original offset values wont be modified because of increments
        pLSCLightInfo = static_cast<LSClightInfo*>(CAMX_CALLOC(sizeof(LSClightInfo) * (pLSCFormatInfo->lightInfoCount)));
        if (NULL != pLSCLightInfo)
        {
            Utils::Memcpy(pLSCLightInfo, pLSCFormatInfo->lightInfo, (sizeof(LSClightInfo) * pLSCFormatInfo->lightInfoCount));
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Memory allocation for LSC failed");
            return;
        }

        for (UINT8 count = 0; count < pLSCFormatInfo->lightInfoCount; count++)
        {
            pLSCData[count].isAvailable        = pLSCFormatInfo->LSCData.isAvailable;
            pLSCData[count].meshHWRollOffSize  = pLSCFormatInfo->meshHWRollOffSize;
            pLSCData[count].illuminant         = pLSCFormatInfo->lightInfo[count].illuminantType;

            for (UINT16 size = 0; size < pLSCFormatInfo->meshHWRollOffSize; size++)
            {
                // R Gain
                valueLSB                      = FormatDataTypeByte(&(pLSCLightInfo + count)->rGainLSB);
                valueMSB                      = FormatDataTypeByte(&(pLSCLightInfo + count)->rGainMSB);
                pLSCData[count].rGain[size]   = static_cast<FLOAT>(valueMSB << BitsPerByte | valueLSB);
                // GR Gain
                valueLSB                      = FormatDataTypeByte(&(pLSCLightInfo + count)->grGainLSB);
                valueMSB                      = FormatDataTypeByte(&(pLSCLightInfo + count)->grGainMSB);
                pLSCData[count].grGain[size]  = static_cast<FLOAT>(valueMSB << BitsPerByte | valueLSB);
                // GB Gain
                valueLSB                      = FormatDataTypeByte(&(pLSCLightInfo + count)->gbGainLSB);
                valueMSB                      = FormatDataTypeByte(&(pLSCLightInfo + count)->gbGainMSB);
                pLSCData[count].gbGain[size]  = static_cast<FLOAT>(valueMSB << BitsPerByte | valueLSB);
                // B Gain
                valueLSB                      = FormatDataTypeByte(&(pLSCLightInfo + count)->bGainLSB);
                valueMSB                      = FormatDataTypeByte(&(pLSCLightInfo + count)->bGainMSB);
                pLSCData[count].bGain[size]   = static_cast<FLOAT>(valueMSB << BitsPerByte | valueLSB);

                /// R increment specifies the offset value to be incremented to read the next R gain value
                (pLSCLightInfo + count)->rGainMSB.offset  += pLSCFormatInfo->rIncrement;
                (pLSCLightInfo + count)->rGainLSB.offset  += pLSCFormatInfo->rIncrement;
                // GR increment specifies the offset value to be incremented to read the next GR gain value
                (pLSCLightInfo + count)->grGainMSB.offset += pLSCFormatInfo->grIncrement;
                (pLSCLightInfo + count)->grGainLSB.offset += pLSCFormatInfo->grIncrement;
                // GB increment specifies the offset value to be incremented to read the next GR gain value
                (pLSCLightInfo + count)->gbGainMSB.offset += pLSCFormatInfo->gbIncrement;
                (pLSCLightInfo + count)->gbGainLSB.offset += pLSCFormatInfo->gbIncrement;
                // B increment specifies the offset value to be incremented to read the next GB gain value
                (pLSCLightInfo + count)->bGainMSB.offset  += pLSCFormatInfo->bIncrement;
                (pLSCLightInfo + count)->bGainLSB.offset  += pLSCFormatInfo->bIncrement;
            }
        }

        CAMX_FREE(pLSCLightInfo);
        pLSCLightInfo = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatDualCameraData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::FormatDualCameraData()
{
    DualCameraInfo*             pDualCameraFormatInfo   = &(m_pEEPROMDriverData->formatInfo.dualCamera);
    DualCameraCalibrationData*  pDualCameraData         = &(m_pSensorInfoTable->moduleCaps.OTPData.dualCameraCalibration);
    MemoryInfo                  dataOffset              = { 0 };
    FLOAT                       qValue                  = QValueDefault;
    EndianType                  endian                  = pDualCameraFormatInfo->DualCameraData.endianness;

    pDualCameraData->isAvailable                        = pDualCameraFormatInfo->DualCameraData.isAvailable;

    if (TRUE == pDualCameraFormatInfo->DualCameraData.isAvailable)
    {
        pDualCameraData->dualCameraOffset = pDualCameraFormatInfo->offset;
        pDualCameraData->dualCameraSize   = pDualCameraFormatInfo->settingsize;

        /// Format the main camera data
        pDualCameraData->masterCalibrationData.focalLength                  =
            FormatDataTypeFloat(&(pDualCameraFormatInfo->masterInfo.focalLength));
        pDualCameraData->masterCalibrationData.focalLengthRatio             =
            FormatDataTypeFloat(&(pDualCameraFormatInfo->masterInfo.focalLengthRatio));
        pDualCameraData->masterCalibrationData.nativeSensorResolutionWidth  =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->masterInfo.nativeSensorResolutionWidth),
                                                      pDualCameraFormatInfo->DualCameraData.endianness));
        pDualCameraData->masterCalibrationData.nativeSensorResolutionHeight =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->masterInfo.nativeSensorResolutionHeight),
                                                      pDualCameraFormatInfo->DualCameraData.endianness));
        pDualCameraData->masterCalibrationData.calibrationResolutionWidth   =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->masterInfo.calibrationResolutionWidth),
                                                      pDualCameraFormatInfo->DualCameraData.endianness));
        pDualCameraData->masterCalibrationData.calibrationResolutionHeight  =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->masterInfo.calibrationResolutionHeight),
                                                      pDualCameraFormatInfo->DualCameraData.endianness));

        UINT32 AFSyncCount = pDualCameraFormatInfo->masterInfo.AFSyncInfoCount;

        if ((0 < AFSyncCount) && (MaxAFCalibrationDistances >= AFSyncCount))
        {
            AFLensData*          pAFSyncData = pDualCameraData->masterCalibrationData.AFSyncData;
            AFCalibrationInfo*   pAFSyncInfo = pDualCameraFormatInfo->masterInfo.AFSyncInfo;

            pDualCameraData->masterCalibrationData.numberOfDistances = AFSyncCount;
            for (UINT32 count = 0; count < AFSyncCount; count++)
            {
                pAFSyncData[count].chartDistanceCM  = FormatDataTypeInteger(&(pAFSyncInfo[count].chartDistanceCM), endian);
                pAFSyncData[count].DACValue         = static_cast<INT16>(FormatDataTypeInteger(&(pAFSyncInfo[count].DACValue),
                                                                                                endian));
            }
        }

        /// Format the aux camera data
        pDualCameraData->auxCalibrationData.focalLength                     =
            FormatDataTypeFloat(&(pDualCameraFormatInfo->auxInfo.focalLength));
        pDualCameraData->auxCalibrationData.focalLengthRatio                =
            FormatDataTypeFloat(&(pDualCameraFormatInfo->auxInfo.focalLengthRatio));
        pDualCameraData->auxCalibrationData.nativeSensorResolutionWidth     =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->auxInfo.nativeSensorResolutionWidth), endian));
        pDualCameraData->auxCalibrationData.nativeSensorResolutionHeight    =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->auxInfo.nativeSensorResolutionHeight), endian));
        pDualCameraData->auxCalibrationData.calibrationResolutionWidth      =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->auxInfo.calibrationResolutionWidth), endian));
        pDualCameraData->auxCalibrationData.calibrationResolutionHeight     =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->auxInfo.calibrationResolutionHeight), endian));

        AFSyncCount = pDualCameraFormatInfo->auxInfo.AFSyncInfoCount;

        if ((0 < AFSyncCount) && (MaxAFCalibrationDistances >= AFSyncCount))
        {
            AFLensData*         pAFSyncData = pDualCameraData->auxCalibrationData.AFSyncData;
            AFCalibrationInfo*  pAFSyncInfo = pDualCameraFormatInfo->auxInfo.AFSyncInfo;

            pDualCameraData->auxCalibrationData.numberOfDistances = AFSyncCount;
            for (UINT32 count = 0; count < AFSyncCount; count++)
            {
                pAFSyncData[count].chartDistanceCM  = FormatDataTypeInteger(&(pAFSyncInfo[count].chartDistanceCM), endian);
                pAFSyncData[count].DACValue         = static_cast<INT16>(FormatDataTypeInteger(&(pAFSyncInfo[count].DACValue),
                                                                                                endian));
            }
        }

        /// Format the system data
        pDualCameraData->systemCalibrationData.calibrationFormatVersion                         =
            FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.calibrationFormatVersion), endian);

        UINT32 dataSize = MaskLengthInBytes(pDualCameraFormatInfo->systemInfo.relativeRotationMatrixOffset.mask);
        Utils::Memcpy(&dataOffset, &pDualCameraFormatInfo->systemInfo.relativeRotationMatrixOffset, sizeof(MemoryInfo));
        for (UINT16 index = 0; index < pDualCameraFormatInfo->systemInfo.rotationMatrixSize; index++)
        {
            pDualCameraData->systemCalibrationData.relativeRotationMatrix[index] = FormatDataTypeFloat(&(dataOffset));
            dataOffset.offset                                                   += static_cast<UINT16>(dataSize);
        }

        dataSize = MaskLengthInBytes(pDualCameraFormatInfo->systemInfo.relativeGeometricSurfaceParametersOffset.mask);
        /// copy to local structures so that original offset values wont be modified because of increments
        Utils::Memcpy(&dataOffset,
                      &pDualCameraFormatInfo->systemInfo.relativeGeometricSurfaceParametersOffset,
                      sizeof(MemoryInfo));
        for (UINT16 index = 0; index < pDualCameraFormatInfo->systemInfo.geometricMatrixSize; index++)
        {
            pDualCameraData->systemCalibrationData.relativeGeometricSurfaceParameters[index] =
                FormatDataTypeFloat(&(dataOffset));
            dataOffset.offset                                                               += static_cast<UINT16>(dataSize);
        }

        pDualCameraData->systemCalibrationData.relativePrinciplePointXOfffset   =
            FormatDataTypeFloat(&(pDualCameraFormatInfo->systemInfo.relativePrinciplePointX));
        pDualCameraData->systemCalibrationData.relativePrinciplePointYOffset    =
            FormatDataTypeFloat(&(pDualCameraFormatInfo->systemInfo.relativePrinciplePointY));
        pDualCameraData->systemCalibrationData.relativePositionFlag             =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.relativePositionFlag),
                                pDualCameraFormatInfo->DualCameraData.endianness));
        pDualCameraData->systemCalibrationData.relativeBaselineDistance         =
            FormatDataTypeFloat(&(pDualCameraFormatInfo->systemInfo.relativeBaselineDistance));
        pDualCameraData->systemCalibrationData.masterSensorMirrorFlipSetting    =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.masterSensorMirrorFlipSetting),
                                                      endian));
        pDualCameraData->systemCalibrationData.auxSensorMirrorFlipSetting       =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.auxSensorMirrorFlipSetting),
                                                      endian));
        pDualCameraData->systemCalibrationData.moduleOrientationFlag            =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.moduleOrientationFlag), endian));
        pDualCameraData->systemCalibrationData.rotationFlag                     =
            static_cast<UINT16>(FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.rotationFlag), endian));

        if (TRUE == pDualCameraFormatInfo->systemInfo.qValueExists)
        {
            qValue = pDualCameraFormatInfo->systemInfo.qValue;

            if (0.0 == qValue)
            {
                qValue = QValueDefault;
            }
        }

        EndianType AECDataEndian = pDualCameraFormatInfo->DualCameraData.endianness;

        for (UINT index = 0; index < m_pEEPROMDriverData->customInfoCount; index ++)
        {
            if (0 == OsUtils::StrCmp("DualCameraAECDataEndian", m_pEEPROMDriverData->customInfo[index].name))
            {
                AECDataEndian = static_cast<EndianType>(m_pEEPROMDriverData->customInfo[index].value);
                CAMX_LOG_INFO(CamxLogGroupSensor, "DCData Endian = %d, DCAECDataEndian: %d", endian, AECDataEndian);
            }
        }

        if (TRUE == pDualCameraFormatInfo->systemInfo.brightnessRatioExists)
        {
            pDualCameraData->systemCalibrationData.brightnessRatio =
                static_cast<FLOAT>((FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.brightnessRatio),
                                    AECDataEndian))/qValue);
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "brightnessRatio =%f qValue = %f",
                                pDualCameraData->systemCalibrationData.brightnessRatio, qValue);
        }
        else
        {
            pDualCameraData->systemCalibrationData.brightnessRatio = DualAecDefaultValue;
        }

        if (TRUE == pDualCameraFormatInfo->systemInfo.referenceSlaveGainExists)
        {
            pDualCameraData->systemCalibrationData.referenceSlaveGain =
                static_cast<FLOAT>((FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.referenceSlaveGain),
                                    AECDataEndian))/qValue);
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "referenceSlaveGain =%f",
                                pDualCameraData->systemCalibrationData.referenceSlaveGain);
        }
        else
        {
            pDualCameraData->systemCalibrationData.referenceSlaveGain = DualAecDefaultValue;
        }

        if (TRUE == pDualCameraFormatInfo->systemInfo.referenceSlaveExpTimeExists)
        {
            pDualCameraData->systemCalibrationData.referenceSlaveExpTime =
                static_cast<FLOAT>(FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.referenceSlaveExpTime),
                                    AECDataEndian));
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "referenceSlaveExpTime =%f",
                                pDualCameraData->systemCalibrationData.referenceSlaveExpTime);
        }
        else
        {
            pDualCameraData->systemCalibrationData.referenceSlaveExpTime = DualAecDefaultValue;
        }

        if (TRUE == pDualCameraFormatInfo->systemInfo.referenceMasterGainExists)
        {
            pDualCameraData->systemCalibrationData.referenceMasterGain =
                static_cast<FLOAT>((FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.referenceMasterGain),
                                    AECDataEndian))/qValue);
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "referenceMasterGain =%f",
                                pDualCameraData->systemCalibrationData.referenceMasterGain);

        }
        else
        {
            pDualCameraData->systemCalibrationData.referenceMasterGain = DualAecDefaultValue;
        }

        if (TRUE == pDualCameraFormatInfo->systemInfo.referenceMasterExpTimeExists)
        {
            pDualCameraData->systemCalibrationData.referenceMasterExpTime =
                static_cast<FLOAT>(FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.referenceMasterExpTime),
                                    AECDataEndian));
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "referenceMasterExpTime =%f",
                                pDualCameraData->systemCalibrationData.referenceMasterExpTime);
        }
        else
        {
            pDualCameraData->systemCalibrationData.referenceMasterExpTime = DualAecDefaultValue;
        }

        if (TRUE == pDualCameraFormatInfo->systemInfo.referenceMasterColorTempExists)
        {
            pDualCameraData->systemCalibrationData.referenceMasterColorTemperature =
                static_cast<UINT32>(FormatDataTypeInteger(&(pDualCameraFormatInfo->systemInfo.referenceMasterColorTemp),
                                    AECDataEndian));
        }
        else
        {
            pDualCameraData->systemCalibrationData.referenceMasterColorTemperature = DualDefaultColorTemp;
        }

        if (TRUE == pDualCameraFormatInfo->systemInfo.absoluteMethodAECSyncInfoExists)
        {
            pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.version =
                static_cast<UINT16>(FormatDataTypeInteger(&(
                    pDualCameraFormatInfo->systemInfo.absoluteMethodAECSyncInfo.version), AECDataEndian));
            pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.averageLuma =
                static_cast<UINT16>(FormatDataTypeInteger(&(
                    pDualCameraFormatInfo->systemInfo.absoluteMethodAECSyncInfo.averageLuma), AECDataEndian));
            pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.gain =
                static_cast<FLOAT>((FormatDataTypeInteger(&(
                    pDualCameraFormatInfo->systemInfo.absoluteMethodAECSyncInfo.gain), AECDataEndian)) / qValue);
            pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.exposureTimeUs =
                static_cast<UINT16>(FormatDataTypeInteger(&(
                    pDualCameraFormatInfo->systemInfo.absoluteMethodAECSyncInfo.exposureTimeUs), AECDataEndian));
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Absolute Method AEC Sync data not present in OTP");
            pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.version        = 0;
            pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.averageLuma    = 0;
            pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.gain           = 0;
            pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.exposureTimeUs = 0;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatSPCData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::FormatSPCData()
{
    SPCInfo*             pSPCFormatInfo = &(m_pEEPROMDriverData->formatInfo.SPC);
    SPCCalibrationData*  pSPCData       = &(m_pSensorInfoTable->moduleCaps.OTPData.SPCCalibration);
    MemoryInfo           dataOffset     = { 0 };

    pSPCData->isAvailable = pSPCFormatInfo->SPCData.isAvailable;

    if (TRUE == pSPCFormatInfo->SPCData.isAvailable)
    {
        UINT32  totalRegCount = 0;
        for (UINT16 i = 0; i < pSPCFormatInfo->SPCSettingsCount; ++i)
        {
            totalRegCount += pSPCFormatInfo->SPCSettings[i].settingsSize;
        }

        pSPCData->settings.regSettingCount = totalRegCount;
        pSPCData->settings.regSetting      = static_cast<RegisterSetting*>(
                                                         CAMX_CALLOC(sizeof(RegisterSetting) * totalRegCount));

        if (NULL != pSPCData->settings.regSetting)
        {
            RegisterData* pRegData = static_cast<RegisterData*>(CAMX_CALLOC(sizeof(RegisterData) * totalRegCount));

            if (NULL != pRegData)
            {
                RegisterSetting* pCurRegOffset = pSPCData->settings.regSetting;

                for (UINT16 i = 0; i < pSPCFormatInfo->SPCSettingsCount; ++i)
                {
                    /// copy to local structures so that original offset values wont be modified because of increments
                    Utils::Memcpy(&dataOffset, &pSPCFormatInfo->SPCSettings[i].dataOffset, sizeof(MemoryInfo));

                    UINT32 dataSize = MaskLengthInBytes(dataOffset.mask);
                    if (NULL != pSPCData->settings.regSetting)
                    {
                        for (UINT16 index = 0; index < pSPCFormatInfo->SPCSettings[i].settingsSize; index++, pCurRegOffset++)
                        {
                            (pCurRegOffset)->registerAddr    = pSPCFormatInfo->SPCSettings[i].SPCAddress + index;
                            (pCurRegOffset)->registerData    = pRegData++;
                            (pCurRegOffset)->registerData[0] =
                                FormatDataTypeInteger((&dataOffset), pSPCFormatInfo->SPCData.endianness);
                            dataOffset.offset                += static_cast<UINT16>(dataSize);
                            (pCurRegOffset)->regAddrType     = pSPCFormatInfo->addressType;
                            (pCurRegOffset)->regDataType     = pSPCFormatInfo->dataType;
                            (pCurRegOffset)->operation       = OperationType::WRITE;

                            if (0 != pSPCFormatInfo->delay)
                            {
                                (pCurRegOffset)->delayUsExists = TRUE;
                                (pCurRegOffset)->delayUsID     = 0;
                                (pCurRegOffset)->delayUs       = pSPCFormatInfo->delay;
                            }
                        }
                    }
                }

            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Calloc failed");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Calloc failed");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatQSCData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::FormatQSCData()
{
    QSCInfo*             pQSCFormatInfo = &(m_pEEPROMDriverData->formatInfo.QSC);
    QSCCalibrationData*  pQSCData       = &(m_pSensorInfoTable->moduleCaps.OTPData.QSCCalibration);
    MemoryInfo           dataOffset     = { 0 };

    pQSCData->isAvailable = pQSCFormatInfo->QSCData.isAvailable;

    if (TRUE == pQSCFormatInfo->QSCData.isAvailable)
    {
        UINT32  totalRegCount = 0;
        for (UINT16 i = 0; i < pQSCFormatInfo->QSCSettingsCount; ++i)
        {
            totalRegCount += pQSCFormatInfo->QSCSettings[i].settingsSize;
        }

        pQSCData->settings.regSettingCount = totalRegCount;
        pQSCData->settings.regSetting      = static_cast<RegisterSetting*>(
                                                         CAMX_CALLOC(sizeof(RegisterSetting) * totalRegCount));

        if (NULL != pQSCData->settings.regSetting)
        {
            RegisterData* pRegData = static_cast<RegisterData*>(CAMX_CALLOC(sizeof(RegisterData) * totalRegCount));

            if (NULL != pRegData)
            {
                RegisterSetting* pCurRegOffset = pQSCData->settings.regSetting;

                for (UINT16 i = 0; i < pQSCFormatInfo->QSCSettingsCount; ++i)
                {
                    /// copy to local structures so that original offset values wont be modified because of increments
                    Utils::Memcpy(&dataOffset, &pQSCFormatInfo->QSCSettings[i].dataOffset, sizeof(MemoryInfo));

                    UINT32 dataSize = MaskLengthInBytes(dataOffset.mask);
                    if (NULL != pQSCData->settings.regSetting)
                    {
                        for (UINT16 index = 0; index < pQSCFormatInfo->QSCSettings[i].settingsSize; index++, pCurRegOffset++)
                        {
                            (pCurRegOffset)->registerAddr    = pQSCFormatInfo->QSCSettings[i].QSCAddress + index;
                            (pCurRegOffset)->registerData    = pRegData++;
                            (pCurRegOffset)->registerData[0] =
                                FormatDataTypeInteger((&dataOffset), pQSCFormatInfo->QSCData.endianness);
                            dataOffset.offset                += static_cast<UINT16>(dataSize);
                            (pCurRegOffset)->regAddrType     = pQSCFormatInfo->addressType;
                            (pCurRegOffset)->regDataType     = pQSCFormatInfo->dataType;
                            (pCurRegOffset)->operation       = OperationType::WRITE;

                            if (0 != pQSCFormatInfo->delay)
                            {
                                (pCurRegOffset)->delayUsExists = TRUE;
                                (pCurRegOffset)->delayUsID     = 0;
                                (pCurRegOffset)->delayUs       = pQSCFormatInfo->delay;
                            }
                        }
                    }
                }

            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Calloc failed");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Calloc failed");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatOISData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::FormatOISData()
{
    OISInfo*             pOISFormatInfo     = &(m_pEEPROMDriverData->formatInfo.OIS);
    OISCalibrationData*  pOISData           = &(m_pSensorInfoTable->moduleCaps.OTPData.OISCalibration);
    MemoryInfo           dataOffset         = { 0 };

    pOISData->isAvailable                   = pOISFormatInfo->OISData.isAvailable;

    if (TRUE == pOISFormatInfo->OISData.isAvailable)
    {
        /// copy to local structures so that original offset values wont be modified because of increments
        Utils::Memcpy(&dataOffset, &pOISFormatInfo->dataOffset, sizeof(MemoryInfo));

        pOISData->settings.regSettingCount = pOISFormatInfo->settingsSize;
        pOISData->settings.regSetting      =
            static_cast<RegisterSetting*>(CAMX_CALLOC((sizeof(RegisterSetting)) * (pOISFormatInfo->settingsSize)));

        RegisterData* pRegData = static_cast<RegisterData*>(CAMX_CALLOC(sizeof(RegisterData) * pOISFormatInfo->settingsSize));

        UINT32 dataSize = MaskLengthInBytes(dataOffset.mask);
        if (NULL!=pOISData->settings.regSetting)
        {
            for (UINT16 index = 0; index < pOISFormatInfo->settingsSize; index++)
            {
                pOISData->settings.regSetting[index].registerAddr     = pOISFormatInfo->OISAddressArray[index];
                pOISData->settings.regSetting[index].registerData     = &pRegData[index];

                pOISData->settings.regSetting[index].registerData[0]  =
                        FormatDataTypeInteger((&dataOffset), pOISFormatInfo->OISData.endianness);
                dataOffset.offset                                    += static_cast<UINT16>(dataSize);
                pOISData->settings.regSetting[index].regAddrType      = pOISFormatInfo->addressType;
                pOISData->settings.regSetting[index].regDataType      = pOISFormatInfo->dataType;
                pOISData->settings.regSetting[index].operation        = OperationType::WRITE;

                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "OisRegAddr = 0x%x OisRegData = 0x%x",
                    pOISData->settings.regSetting[index].registerAddr,
                    pOISData->settings.regSetting[index].registerData[0]);

                if (0 != pOISFormatInfo->delay)
                {
                    pOISData->settings.regSetting[index].delayUsExists  = TRUE;
                    pOISData->settings.regSetting[index].delayUsID      = 0;
                    pOISData->settings.regSetting[index].delayUs        = pOISFormatInfo->delay;
                }
            }
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatPDAFDCCData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::FormatPDAFDCCData()
{
    PDAFDCCInfo*             pPDAFDCCFormatInfo     = &(m_pEEPROMDriverData->formatInfo.PDAFDCC);
    PDAFDCCCalibrationData*  pPDAFDCCData           = &(m_pSensorInfoTable->moduleCaps.OTPData.PDAFDCCCalibration);
    pPDAFDCCData->isAvailable                       = pPDAFDCCFormatInfo->DCCData.isAvailable;

    if (0 == pPDAFDCCFormatInfo->qValue)
    {
        pPDAFDCCFormatInfo->qValue = 1;
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "incorrect qValue configured, so defaulting it to 1");
    }

    if (TRUE == pPDAFDCCFormatInfo->DCCData.isAvailable)
    {
        pPDAFDCCData->knotXCount    = pPDAFDCCFormatInfo->knotX;
        pPDAFDCCData->knotYCount    = pPDAFDCCFormatInfo->knotY;

        UINT32     slopeDataSize    = MaskLengthInBytes(pPDAFDCCFormatInfo->slopeDataOffset.mask);
        UINT32     offsetDataSize   = MaskLengthInBytes(pPDAFDCCFormatInfo->offsetDataOffset.mask);
        MemoryInfo slopeDataOffset  = { 0 };
        MemoryInfo offsetDataOffset = { 0 };

        /// copy to local structures so that original offset values wont be modified because of increments
        Utils::Memcpy(&slopeDataOffset, &pPDAFDCCFormatInfo->slopeDataOffset, sizeof(MemoryInfo));
        Utils::Memcpy(&offsetDataOffset, &pPDAFDCCFormatInfo->offsetDataOffset, sizeof(MemoryInfo));
        for (UINT16 index = 0; index < ((pPDAFDCCFormatInfo->knotX) * (pPDAFDCCFormatInfo->knotY)); index++)
        {
            pPDAFDCCData->slopeData[index]  = static_cast<FLOAT>(FormatDataTypeInteger(&(slopeDataOffset),
                                                                pPDAFDCCFormatInfo->DCCData.endianness)) /
                                                (pPDAFDCCFormatInfo->qValue);
            slopeDataOffset.offset         += static_cast<UINT16>(slopeDataSize);

            pPDAFDCCData->offsetData[index] = static_cast<FLOAT>(FormatDataTypeInteger(&(offsetDataOffset),
                                                                 pPDAFDCCFormatInfo->DCCData.endianness)) /
                                                (pPDAFDCCFormatInfo->qValue);
            offsetDataOffset.offset        += static_cast<UINT16>(offsetDataSize);
        }

        for (UINT16 index = 0; index < pPDAFDCCFormatInfo->knotX; index++)
        {
            pPDAFDCCData->slopeOffsetAddressX[index]    =
                (pPDAFDCCFormatInfo->offsetX) + ((pPDAFDCCFormatInfo->areaX) * (index));
        }

        for (UINT16 index = 0; index < pPDAFDCCFormatInfo->knotY; index++)
        {
            pPDAFDCCData->slopeOffsetAddressY[index]    =
                (pPDAFDCCFormatInfo->offsetY) + ((pPDAFDCCFormatInfo->areaY) * (index));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatPDAF2DData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::FormatPDAF2DData()
{
    PDAF2DInfo*             pPDAF2DFormatInfo   = &(m_pEEPROMDriverData->formatInfo.PDAF2D);
    PDAF2DCalibrationData*  pPDAF2DData         = &(m_pSensorInfoTable->moduleCaps.OTPData.PDAF2DCalibration);
    LSCCalibrationData*     pLSCData            = &(m_pSensorInfoTable->moduleCaps.OTPData.LSCCalibration[0]);
    MemoryInfo              offsetCopy          = { 0 };

    pPDAF2DData->isAvailable = pPDAF2DFormatInfo->PDAF2DData.isAvailable;

    if (TRUE == pPDAF2DFormatInfo->PDAF2DData.isAvailable)
    {
        pPDAF2DData->versionNumber                  =
            static_cast<UINT16>(FormatDataTypeInteger((&pPDAF2DFormatInfo->version), pPDAF2DFormatInfo->PDAF2DData.endianness));
        pPDAF2DData->mapWidth                       =
            static_cast<UINT16>(FormatDataTypeInteger((&pPDAF2DFormatInfo->mapWidth),
                                                      pPDAF2DFormatInfo->PDAF2DData.endianness));
        pPDAF2DData->mapHeight                      =
            static_cast<UINT16>(FormatDataTypeInteger((&pPDAF2DFormatInfo->mapHeight),
                                                       pPDAF2DFormatInfo->PDAF2DData.endianness));

        UINT32 leftGainSize = MaskLengthInBytes(pPDAF2DFormatInfo->leftGainMap.mask);
        /// copy to local structures so that original offset values wont be modified because of increments
        Utils::Memcpy(&offsetCopy, &pPDAF2DFormatInfo->leftGainMap, sizeof(MemoryInfo));
        for (UINT16 index = 0; index < ((pPDAF2DFormatInfo->gainWidth) * (pPDAF2DFormatInfo->gainHeight)); index++)
        {
            pPDAF2DData->leftGainMap[index] =
                static_cast<UINT16>(FormatDataTypeInteger(&offsetCopy, pPDAF2DFormatInfo->PDAF2DData.endianness));
            offsetCopy.offset              += static_cast<UINT16>(leftGainSize);
        }

        UINT32 rightGainSize = MaskLengthInBytes(pPDAF2DFormatInfo->rightGainMap.mask);
        /// copy to local structures so that original offset values wont be modified because of increments
        Utils::Memcpy(&offsetCopy, &pPDAF2DFormatInfo->rightGainMap, sizeof(MemoryInfo));
        for (UINT16 index = 0; index < ((pPDAF2DFormatInfo->gainWidth) * (pPDAF2DFormatInfo->gainHeight)); index++)
        {
            pPDAF2DData->rightGainMap[index]  =
                static_cast<UINT16>(FormatDataTypeInteger(&offsetCopy, pPDAF2DFormatInfo->PDAF2DData.endianness));
            offsetCopy.offset                += static_cast<UINT16>(rightGainSize);
        }

        if (TRUE == pPDAF2DFormatInfo->upGainMapExists)
        {
            UINT32 upGainSize = MaskLengthInBytes(pPDAF2DFormatInfo->upGainMap.mask);
            /// copy to local structures so that original offset values wont be modified because of increments
            Utils::Memcpy(&offsetCopy, &pPDAF2DFormatInfo->upGainMap, sizeof(MemoryInfo));
            for (UINT16 index = 0; index < ((pPDAF2DFormatInfo->gainWidth) * (pPDAF2DFormatInfo->gainHeight)); index++)
            {
                pPDAF2DData->upGainMap[index] =
                    static_cast<UINT16>(FormatDataTypeInteger(&offsetCopy, pPDAF2DFormatInfo->PDAF2DData.endianness));
                offsetCopy.offset += static_cast<UINT16>(upGainSize);
            }
        }

        if (TRUE == pPDAF2DFormatInfo->downGainMapExists)
        {
            UINT32 downGainSize = MaskLengthInBytes(pPDAF2DFormatInfo->downGainMap.mask);
            /// copy to local structures so that original offset values wont be modified because of increments
            Utils::Memcpy(&offsetCopy, &pPDAF2DFormatInfo->downGainMap, sizeof(MemoryInfo));
            for (UINT16 index = 0; index < ((pPDAF2DFormatInfo->gainWidth) * (pPDAF2DFormatInfo->gainHeight)); index++)
            {
                pPDAF2DData->upGainMap[index] =
                    static_cast<UINT16>(FormatDataTypeInteger(&offsetCopy, pPDAF2DFormatInfo->PDAF2DData.endianness));
                offsetCopy.offset += static_cast<UINT16>(downGainSize);
            }
        }

        UINT32 conversionCoefficientLength = MaskLengthInBytes(pPDAF2DFormatInfo->conversionCoefficient.mask);
        /// copy to local structures so that original offset values wont be modified because of increments
        Utils::Memcpy(&offsetCopy, &pPDAF2DFormatInfo->conversionCoefficient, sizeof(MemoryInfo));
        for (UINT16 index = 0; index < pPDAF2DFormatInfo->conversionCoefficientCount; index++)
        {
            pPDAF2DData->conversionCoefficientPD[index] =
                static_cast<INT16>(FormatDataTypeInteger(&offsetCopy, pPDAF2DFormatInfo->PDAF2DData.endianness));
            offsetCopy.offset                          += static_cast<UINT16>(conversionCoefficientLength);
        }

        if ((TRUE == pPDAF2DFormatInfo->DCCMapHeightMemoryInfoExists) &&
            (TRUE == pPDAF2DFormatInfo->DCCMapWidthMemoryInfoExists) &&
            (TRUE == pPDAF2DFormatInfo->DCCQFormatMemoryInfoExists))
        {
            pPDAF2DData->qFactorDCC = static_cast<UINT16>(FormatDataTypeInteger(
                                                         (&pPDAF2DFormatInfo->DCCQFormatMemoryInfo),
                                                          pPDAF2DFormatInfo->PDAF2DData.endianness));
            pPDAF2DData->mapWidthDCC = static_cast<UINT16>(FormatDataTypeInteger(
                                                         (&pPDAF2DFormatInfo->DCCMapWidthMemoryInfo),
                                                          pPDAF2DFormatInfo->PDAF2DData.endianness));
            pPDAF2DData->mapHeightDCC = static_cast<UINT16>(FormatDataTypeInteger(
                                                         (&pPDAF2DFormatInfo->DCCMapHeightMemoryInfo),
                                                          pPDAF2DFormatInfo->PDAF2DData.endianness));
        }
        else
        {
            pPDAF2DData->qFactorDCC   = pPDAF2DFormatInfo->DCCQFormat;
            pPDAF2DData->mapWidthDCC  = pPDAF2DFormatInfo->DCCMapWidth;
            pPDAF2DData->mapHeightDCC = pPDAF2DFormatInfo->DCCMapHeight;
        }

        /// Center gainmap is required for advanced sparse pd algorithm, calculate from LSC table
        if (TRUE == pLSCData->isAvailable)
        {
            CAMX_ASSERT(pPDAF2DData->mapWidth * pPDAF2DData->mapHeight == pLSCData->meshHWRollOffSize);
            FLOAT maxGain = pLSCData->grGain[0];
            for (UINT16 index = 0; index < pLSCData->meshHWRollOffSize; index++)
            {
                if (pLSCData->grGain[index] > maxGain)
                {
                    maxGain = pLSCData->grGain[index];
                }
            }
            for (UINT16 index = 0; index < pLSCData->meshHWRollOffSize; index++)
            {
                pPDAF2DData->centerGainMap[index] =
                    static_cast<UINT16>(maxGain / pLSCData->grGain[index] * GainMapPrecision);
            }
        }
        else
        {
            for (UINT16 index = 0; index < pLSCData->meshHWRollOffSize; index++)
            {
                pPDAF2DData->centerGainMap[index] = static_cast<UINT16>(1.0 * GainMapPrecision);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMData::FormatLensData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMData::FormatLensData()
{
    LensDistortionInfo*   pLensFormatInfo       = &(m_pEEPROMDriverData->formatInfo.Lens);
    LensData*             pLensData             = &(m_pSensorInfoTable->moduleCaps.OTPData.lensDataCalibration);

    pLensData->isAvailable = pLensFormatInfo->lensData.isAvailable;

    if (TRUE == pLensData->isAvailable)
    {
        BYTE*  pOTPData               = m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData.pRawData;

        if (pOTPData != NULL)
        {
            pLensData->horizontalFocalLength = FormatDataTypeFloat((&pLensFormatInfo->horizontalFocalLength));
            pLensData->verticalFocalLength   = FormatDataTypeFloat((&pLensFormatInfo->verticalFocalLength));
            pLensData->opticalAxisX          = FormatDataTypeFloat((&pLensFormatInfo->opticalAxisX));
            pLensData->opticalAxisY          = FormatDataTypeFloat((&pLensFormatInfo->opticalAxisY));
            pLensData->kappa0                = FormatDataTypeFloat((&pLensFormatInfo->kappa0));
            pLensData->kappa1                = FormatDataTypeFloat((&pLensFormatInfo->kappa1));
            pLensData->kappa2                = FormatDataTypeFloat((&pLensFormatInfo->kappa2));
            pLensData->kappa3                = FormatDataTypeFloat((&pLensFormatInfo->kappa3));
            pLensData->kappa4                = FormatDataTypeFloat((&pLensFormatInfo->kappa4));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMDataDump::EEPROMDataDump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EEPROMDataDump::EEPROMDataDump(
    EEPROMDriverData*       pEEPROMDriverData,
    HwSensorInfo*           pSensorInfoTable)
{
    m_pEEPROMDriverData     = pEEPROMDriverData;
    m_pSensorInfoTable      = pSensorInfoTable;

    if (NULL != m_pEEPROMDriverData &&
        NULL != m_pSensorInfoTable)
    {
        DumpEEPROMData();
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "No EEPROM data to dump!");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMDataDump::~EEPROMDataDump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
EEPROMDataDump::~EEPROMDataDump()
{
    if (NULL != m_pEEPROMDriverData &&
        NULL != m_pSensorInfoTable)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Dumped EEPROM data for %s.", m_pEEPROMDriverData->slaveInfo.EEPROMName);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMDataDump::PrintLightType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMDataDump::PrintLightType(
    FILE*                pDumpFile,
    EEPROMIlluminantType illuminant)
{
    switch (illuminant)
    {
        case EEPROMIlluminantType::D65:
            OsUtils::FPrintF(pDumpFile, "\nLight Type: D65 \n");
            break;
        case EEPROMIlluminantType::TL84:
            OsUtils::FPrintF(pDumpFile, "\nLight Type: TL84 \n");
            break;
        case EEPROMIlluminantType::A:
            OsUtils::FPrintF(pDumpFile, "\nLight Type: A \n");
            break;
        case EEPROMIlluminantType::D50:
            OsUtils::FPrintF(pDumpFile, "\nLight Type: D50 \n");
            break;
        case EEPROMIlluminantType::H:
            OsUtils::FPrintF(pDumpFile, "\nLight Type: H \n");
            break;
        default:
            OsUtils::FPrintF(pDumpFile, "\nLight Type: Invalid\n");
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Invalid light type")
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMDataDump::DumpAFData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMDataDump::DumpAFData()
{
    FILE*              pAFDumpFile            = NULL;
    CHAR               fileName[FILENAME_MAX] = { 0 };
    AFCalibrationData* pAFData                = &(m_pSensorInfoTable->moduleCaps.OTPData.AFCalibration);

    if (TRUE == pAFData->isAvailable)
    {
        OsUtils::SNPrintF(fileName, sizeof(fileName), "%s/%s_%s_%s", ConfigFileDirectory,
                          m_pEEPROMDriverData->slaveInfo.EEPROMName, "autofocus", "OTP.txt");
        pAFDumpFile = OsUtils::FOpen(fileName, "w");
    }

    if (pAFDumpFile != NULL)
    {
        OsUtils::FPrintF(pAFDumpFile, "Autofocus data dump for EEPROM %s\n",
                         m_pEEPROMDriverData->slaveInfo.EEPROMName);
        OsUtils::FPrintF(pAFDumpFile, "\nMacro dac = %d\n", pAFData->macroDAC);
        OsUtils::FPrintF(pAFDumpFile, "Infinity dac = %d\n", pAFData->infinityDAC);
        OsUtils::FPrintF(pAFDumpFile, "Macro margin = %f\n", pAFData->macroMargin);
        OsUtils::FPrintF(pAFDumpFile, "Infinity margin = %f\n", pAFData->infinityMargin);
        OsUtils::FPrintF(pAFDumpFile, "Hall offset bias = %d\n", pAFData->hallOffsetBias);
        OsUtils::FPrintF(pAFDumpFile, "Hall register addr = %d\n", pAFData->hallRegisterAddr);
        OsUtils::FPrintF(pAFDumpFile, "Lens Calibration Data: numberOfDistances: %d\n", pAFData->numberOfDistances);
        if (0 < pAFData->numberOfDistances)
        {
            for (UINT16 index = 0; index < pAFData->numberOfDistances; index++)
            {
                OsUtils::FPrintF(pAFDumpFile,
                                 "    Distance(CM) = %d : DAC  = %d\n",
                                 pAFData->calibrationInfo[index].chartDistanceCM,
                                 pAFData->calibrationInfo[index].DACValue);
            }
        }

        OsUtils::FClose(pAFDumpFile);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMDataDump::DumpWBData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMDataDump::DumpWBData()
{
    FILE*              pWBDumpFile            = NULL;
    CHAR               fileName[FILENAME_MAX] = { 0 };
    WBCalibrationData* pWBData                = &(m_pSensorInfoTable->moduleCaps.OTPData.WBCalibration[0]);
    BOOL               isDataAvailable        = FALSE;

    for (UINT8 index = 0; index < MaxLightTypes; index ++)
    {
        if (TRUE == pWBData[index].isAvailable)
        {
            // If the data is available in any of the index then set it to TRUE
            isDataAvailable = TRUE;
            break;
        }
    }

    if (TRUE == isDataAvailable)
    {
        INT lengthWritten = OsUtils::SNPrintF(fileName,
                                              sizeof(fileName),
                                              "%s/%s_%s_%s",
                                              ConfigFileDirectory,
                                              m_pEEPROMDriverData->slaveInfo.EEPROMName,
                                              "wb",
                                              "OTP.txt");
        if (0 < lengthWritten)
        {
            pWBDumpFile = OsUtils::FOpen(fileName, "w");
            if (pWBDumpFile != NULL)
            {
                OsUtils::FPrintF(pWBDumpFile, "Whitebalance data dump for EEPROM %s\n",
                                 m_pEEPROMDriverData->slaveInfo.EEPROMName);
                for (UINT8 index = 0; index < MaxLightTypes; index ++)
                {
                    if (TRUE == pWBData[index].isAvailable)
                    {
                        PrintLightType(pWBDumpFile, pWBData[index].illuminant);
                        OsUtils::FPrintF(pWBDumpFile, "  rOverG   = %f\n", pWBData[index].rOverG);
                        OsUtils::FPrintF(pWBDumpFile, "  bOverG   = %f\n", pWBData[index].bOverG);
                        OsUtils::FPrintF(pWBDumpFile, "  grOverGB = %f\n", pWBData[index].grOverGB);
                    }
                }

                OsUtils::FClose(pWBDumpFile);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create file");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get file name");
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "WB OTP data not available");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMDataDump::DumpLSCData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMDataDump::DumpLSCData()
{
    FILE*               pLSCDumpFile             = NULL;
    CHAR                fileName[FILENAME_MAX]   = { 0 };
    LSCCalibrationData* pLSCData                 = &(m_pSensorInfoTable->moduleCaps.OTPData.LSCCalibration[0]);
    BOOL                isDataAvailable          = FALSE;

    for (UINT8 index = 0; index < MaxLightTypes; index ++)
    {
        if (TRUE == pLSCData[index].isAvailable)
        {
            // If the data is available in any of the index then set it to TRUE
            isDataAvailable = TRUE;
            break;
        }
    }

    if (TRUE == isDataAvailable)
    {
        INT lengthWritten = OsUtils::SNPrintF(fileName,
                                              sizeof(fileName),
                                              "%s/%s_%s_%s",
                                              ConfigFileDirectory,
                                              m_pEEPROMDriverData->slaveInfo.EEPROMName,
                                              "lsc",
                                              "OTP.txt");
        if (0 < lengthWritten)
        {
            pLSCDumpFile = OsUtils::FOpen(fileName, "w");
            if (pLSCDumpFile != NULL)
            {
                OsUtils::FPrintF(pLSCDumpFile, "Lens shading correction data dump for EEPROM %s\n",
                                 m_pEEPROMDriverData->slaveInfo.EEPROMName);
                for (UINT16 index = 0; index < MaxLightTypes; index ++)
                {
                    if (TRUE == pLSCData[index].isAvailable)
                    {
                        PrintLightType(pLSCDumpFile, pLSCData[index].illuminant);
                        OsUtils::FPrintF(pLSCDumpFile, "MeshHWRollOffSize=%u\n", pLSCData[index].meshHWRollOffSize);

                        OsUtils::FPrintF(pLSCDumpFile, "\nr_gain:\n");
                        for (UINT8 row = 0; row < HWRollOffTableRowSize; row ++)
                        {
                            for (UINT8 col = 0; col < HWRollOffTableColSize; col ++)
                            {
                                OsUtils::FPrintF(pLSCDumpFile,
                                                 "%f  ",
                                                 pLSCData[index].rGain[row * HWRollOffTableColSize + col]);
                            }
                            OsUtils::FPrintF(pLSCDumpFile, "\n");
                        }

                        OsUtils::FPrintF(pLSCDumpFile, "\ngr_gain:\n");
                        for (UINT8 row = 0; row < HWRollOffTableRowSize; row ++)
                        {
                            for (UINT8 col = 0; col < HWRollOffTableColSize; col ++)
                            {
                                OsUtils::FPrintF(pLSCDumpFile,
                                                 "%f  ",
                                                 pLSCData[index].grGain[row * HWRollOffTableColSize + col]);
                            }
                            OsUtils::FPrintF(pLSCDumpFile, "\n");
                        }

                        OsUtils::FPrintF(pLSCDumpFile, "\ngb_gain:\n");
                        for (UINT8 row = 0; row < HWRollOffTableRowSize; row ++)
                        {
                            for (UINT8 col = 0; col < HWRollOffTableColSize; col ++)
                            {
                                OsUtils::FPrintF(pLSCDumpFile,
                                                 "%f  ",
                                                 pLSCData[index].gbGain[row * HWRollOffTableColSize + col]);
                            }
                            OsUtils::FPrintF(pLSCDumpFile, "\n");
                        }

                        OsUtils::FPrintF(pLSCDumpFile, "\nb_gain:\n");
                        for (UINT8 row = 0; row < HWRollOffTableRowSize; row ++)
                        {
                            for (UINT8 col = 0; col < HWRollOffTableColSize; col ++)
                            {
                                OsUtils::FPrintF(pLSCDumpFile,
                                                 "%f  ",
                                                 pLSCData[index].bGain[row * HWRollOffTableColSize + col]);
                            }
                            OsUtils::FPrintF(pLSCDumpFile, "\n");
                        }
                    }
                }

                OsUtils::FClose(pLSCDumpFile);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to create file");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupSensor, "Failed to get file name");
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "LSC OTP data not available");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMDataDump::DumpDualCameraData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMDataDump::DumpDualCameraData()
{
    FILE*                      pDualCameraDumpFile    = NULL;
    CHAR                       fileName[FILENAME_MAX] = { 0 };
    DualCameraCalibrationData* pDualCameraData        = &(m_pSensorInfoTable->moduleCaps.OTPData.dualCameraCalibration);

    if (TRUE == pDualCameraData->isAvailable)
    {
        OsUtils::SNPrintF(fileName, sizeof(fileName), "%s/%s_%s_%s", ConfigFileDirectory,
                          m_pEEPROMDriverData->slaveInfo.EEPROMName, "dualcamera", "OTP.txt");
        pDualCameraDumpFile = OsUtils::FOpen(fileName, "w");
    }

    if (pDualCameraDumpFile != NULL)
    {
        OsUtils::FPrintF(pDualCameraDumpFile, "Dual camera calibation data dump for EEPROM %s\n",
                         m_pEEPROMDriverData->slaveInfo.EEPROMName);

        OsUtils::FPrintF(pDualCameraDumpFile, "\nMASTER SENSOR CALIB:\n");
        OsUtils::FPrintF(pDualCameraDumpFile, "Focal length = %f\n",
                         pDualCameraData->masterCalibrationData.focalLength);
        OsUtils::FPrintF(pDualCameraDumpFile, "Native sensor resolution width = %u\n",
                         pDualCameraData->masterCalibrationData.nativeSensorResolutionWidth);
        OsUtils::FPrintF(pDualCameraDumpFile, "Native sensor resolution height = %u\n",
                         pDualCameraData->masterCalibrationData.nativeSensorResolutionHeight);
        OsUtils::FPrintF(pDualCameraDumpFile, "Calibration resolution width = %u\n",
                         pDualCameraData->masterCalibrationData.calibrationResolutionWidth);
        OsUtils::FPrintF(pDualCameraDumpFile, "Calibration resolution height = %u\n",
                         pDualCameraData->masterCalibrationData.calibrationResolutionHeight);
        OsUtils::FPrintF(pDualCameraDumpFile, "Focal length ratio = %f\n",
                         pDualCameraData->masterCalibrationData.focalLengthRatio);
        OsUtils::FPrintF(pDualCameraDumpFile,
                         "AF Sync Calibration Data: numberOfDistances: %d\n",
                         pDualCameraData->masterCalibrationData.numberOfDistances);
        if (0 < pDualCameraData->masterCalibrationData.numberOfDistances)
        {
            for (UINT16 index = 0; index < pDualCameraData->masterCalibrationData.numberOfDistances; index++)
            {
                OsUtils::FPrintF(pDualCameraDumpFile,
                                 "    Distance(CM) = %d : DAC = %d\n",
                                 pDualCameraData->masterCalibrationData.AFSyncData[index].chartDistanceCM,
                                 pDualCameraData->masterCalibrationData.AFSyncData[index].DACValue);
            }
        }

        OsUtils::FPrintF(pDualCameraDumpFile, "\nAUXILIARY SENSOR CALIB:\n");
        OsUtils::FPrintF(pDualCameraDumpFile, "Focal length = %f\n",
                         pDualCameraData->auxCalibrationData.focalLength);
        OsUtils::FPrintF(pDualCameraDumpFile, "Native sensor resolution width = %u\n",
                         pDualCameraData->auxCalibrationData.nativeSensorResolutionWidth);
        OsUtils::FPrintF(pDualCameraDumpFile, "Native sensor resolution height = %u\n",
                         pDualCameraData->auxCalibrationData.nativeSensorResolutionHeight);
        OsUtils::FPrintF(pDualCameraDumpFile, "Calibration resolution width = %u\n",
                         pDualCameraData->auxCalibrationData.calibrationResolutionWidth);
        OsUtils::FPrintF(pDualCameraDumpFile, "Calibration resolution height = %u\n",
                         pDualCameraData->auxCalibrationData.calibrationResolutionHeight);
        OsUtils::FPrintF(pDualCameraDumpFile, "Focal length ratio = %f\n",
                         pDualCameraData->auxCalibrationData.focalLengthRatio);
        OsUtils::FPrintF(pDualCameraDumpFile,
                                 "AF Sync Calibration Data: numberOfDistances: %d\n",
                                 pDualCameraData->auxCalibrationData.numberOfDistances);
        if (0 < pDualCameraData->auxCalibrationData.numberOfDistances)
        {
            for (UINT16 index = 0; index < pDualCameraData->auxCalibrationData.numberOfDistances; index++)
            {
                OsUtils::FPrintF(pDualCameraDumpFile,
                                 "    Distance(CM) = %d : DAC = %d\n",
                                 pDualCameraData->auxCalibrationData.AFSyncData[index].chartDistanceCM,
                                 pDualCameraData->auxCalibrationData.AFSyncData[index].DACValue);
            }
        }

        OsUtils::FPrintF(pDualCameraDumpFile, "\nSYSTEM CALIB:\n");
        OsUtils::FPrintF(pDualCameraDumpFile, "Calibration version format = %u\n",
                         pDualCameraData->systemCalibrationData.calibrationFormatVersion);
        OsUtils::FPrintF(pDualCameraDumpFile, "Relative principle point X offset = %f\n",
                         pDualCameraData->systemCalibrationData.relativePrinciplePointXOfffset);
        OsUtils::FPrintF(pDualCameraDumpFile, "Relative principle point Y offset = %f\n",
                         pDualCameraData->systemCalibrationData.relativePrinciplePointYOffset);
        OsUtils::FPrintF(pDualCameraDumpFile, "Relative position flag = %u\n",
                         pDualCameraData->systemCalibrationData.relativePositionFlag);
        OsUtils::FPrintF(pDualCameraDumpFile, "Relative baseline distance = %f\n",
                         pDualCameraData->systemCalibrationData.relativeBaselineDistance);

        for (UINT8 index = 0; index < RelativeRotationMatrixMax; index ++)
        {
            OsUtils::FPrintF(pDualCameraDumpFile, "Relative rotation matrix[%d] = %f\n", index,
                             pDualCameraData->systemCalibrationData.relativeRotationMatrix[index]);
        }

        for (UINT8 index = 0; index < RelativeGeometricSurfaceParamsMax; index ++)
        {
            OsUtils::FPrintF(pDualCameraDumpFile, "Relative geometric surface parameters[%d] = %f\n", index,
                             pDualCameraData->systemCalibrationData.relativeGeometricSurfaceParameters[index]);
        }

        OsUtils::FPrintF(pDualCameraDumpFile, "Master sensor mirror and flip setting = %d\n",
                         pDualCameraData->systemCalibrationData.masterSensorMirrorFlipSetting);
        OsUtils::FPrintF(pDualCameraDumpFile, "Auxiliary sensor mirror and flip setting = %d\n",
                         pDualCameraData->systemCalibrationData.auxSensorMirrorFlipSetting);
        OsUtils::FPrintF(pDualCameraDumpFile, "Module orientation flag = %d\n",
                         pDualCameraData->systemCalibrationData.moduleOrientationFlag);
        OsUtils::FPrintF(pDualCameraDumpFile, "Rotation flag = %d\n",
                         pDualCameraData->systemCalibrationData.rotationFlag);

        OsUtils::FPrintF(pDualCameraDumpFile, "\n3A CALIB: \n");
        OsUtils::FPrintF(pDualCameraDumpFile, "Brightness ratio = %f\n",
                         pDualCameraData->systemCalibrationData.brightnessRatio);
        OsUtils::FPrintF(pDualCameraDumpFile, "Reference slave gain = %f\n",
                         pDualCameraData->systemCalibrationData.referenceSlaveGain);
        OsUtils::FPrintF(pDualCameraDumpFile, "Reference slave exposureTime = %f ms\n",
                         pDualCameraData->systemCalibrationData.referenceSlaveExpTime);
        OsUtils::FPrintF(pDualCameraDumpFile, "Reference master gain = %f\n",
                         pDualCameraData->systemCalibrationData.referenceMasterGain);
        OsUtils::FPrintF(pDualCameraDumpFile, "Reference master exposureTime = %f ms\n",
                         pDualCameraData->systemCalibrationData.referenceMasterExpTime);
        OsUtils::FPrintF(pDualCameraDumpFile, "Reference master color temperature = %d\n",
                         pDualCameraData->systemCalibrationData.referenceMasterColorTemperature);

        OsUtils::FPrintF(pDualCameraDumpFile, "\nAbsolute AEC SYNC CALIB: \n");
        OsUtils::FPrintF(pDualCameraDumpFile, "version = %d\n",
                         pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.version);
        OsUtils::FPrintF(pDualCameraDumpFile, "averageLuma = %d\n",
                         pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.averageLuma);
        OsUtils::FPrintF(pDualCameraDumpFile, "gain = %f\n",
                         pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.gain);
        OsUtils::FPrintF(pDualCameraDumpFile, "exposureTimeUs = %d\n",
                         pDualCameraData->systemCalibrationData.absoluteMethodAECSyncData.exposureTimeUs);

        OsUtils::FClose(pDualCameraDumpFile);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMDataDump::DumpPDAFData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMDataDump::DumpPDAFData()
{
    FILE*                   pPDAFDumpFile          = NULL;
    CHAR                    fileName[FILENAME_MAX] = { 0 };
    PDAF2DCalibrationData*  pPDAF2DData            = &(m_pSensorInfoTable->moduleCaps.OTPData.PDAF2DCalibration);
    PDAFDCCCalibrationData* pPDAFDCCData           = &(m_pSensorInfoTable->moduleCaps.OTPData.PDAFDCCCalibration);

    if (TRUE == pPDAFDCCData->isAvailable || TRUE == pPDAF2DData->isAvailable)
    {
        OsUtils::SNPrintF(fileName, sizeof(fileName), "%s/%s_%s_%s", ConfigFileDirectory,
                          m_pEEPROMDriverData->slaveInfo.EEPROMName, "pdaf", "OTP.txt");
        pPDAFDumpFile = OsUtils::FOpen(fileName, "w");
    }

    if (pPDAFDumpFile != NULL)
    {
        OsUtils::FPrintF(pPDAFDumpFile, "PDAF data dump for EEPROM %s\n",
                         m_pEEPROMDriverData->slaveInfo.EEPROMName);

        OsUtils::FPrintF(pPDAFDumpFile, "\n-------------PDAF T1 data DUMP------------\n");
        for (UINT16 index = 0; index < MaxPDAFKnotX * MaxPDAFKnotY; index ++)
        {
            OsUtils::FPrintF(pPDAFDumpFile, "SlopeData[%d]: %f\n", index, pPDAFDCCData->slopeData[index]);
        }

        OsUtils::FPrintF(pPDAFDumpFile, "\n-------------PDAF T2/3 data DUMP----------\n");
        OsUtils::FPrintF(pPDAFDumpFile, "VersionNum: %d, knotXCount %d, knotYCount %d\n",
                         pPDAF2DData->versionNumber, pPDAFDCCData->knotXCount, pPDAFDCCData->knotYCount);
        OsUtils::FPrintF(pPDAFDumpFile, "MapWidth %d, MapHeight %d, PD_conversion_coeff[0] %d\n",
                         pPDAF2DData->mapWidth, pPDAF2DData->mapHeight, pPDAF2DData->conversionCoefficientPD[0]);

        OsUtils::FPrintF(pPDAFDumpFile, "-----------Left GainMap----------------\n");
        for (UINT16 i = 0; i < pPDAF2DData->mapWidth * pPDAF2DData->mapHeight; i += pPDAF2DData->mapWidth)
        {
            for (UINT16 j = 0; j < pPDAF2DData->mapWidth; j ++)
            {
                OsUtils::FPrintF(pPDAFDumpFile, "%d, ", pPDAF2DData->leftGainMap[i + j]);
            }
            OsUtils::FPrintF(pPDAFDumpFile, "\n");
        }

        OsUtils::FPrintF(pPDAFDumpFile, "-----------Right GainMap----------------\n");
        for (UINT16 i = 0; i < pPDAF2DData->mapWidth * pPDAF2DData->mapHeight; i += pPDAF2DData->mapWidth)
        {
            for (UINT16 j = 0; j < pPDAF2DData->mapWidth; j ++)
            {
                OsUtils::FPrintF(pPDAFDumpFile, "%d, ", pPDAF2DData->rightGainMap[i + j]);
            }
            OsUtils::FPrintF(pPDAFDumpFile, "\n");
        }

        OsUtils::FPrintF(pPDAFDumpFile, "-------------------DCC------------------\n");
        OsUtils::FPrintF(pPDAFDumpFile, "conversionCoefficientCount: %d, DCCMapWidth %d, DCCMapHeight %d, Q factor %d\n",
                         pPDAF2DData->mapWidthDCC * pPDAF2DData->mapHeightDCC,
                         pPDAF2DData->mapWidthDCC, pPDAF2DData->mapHeightDCC, pPDAF2DData->qFactorDCC);
        for (UINT16 i = 0; i < pPDAF2DData->mapWidthDCC * pPDAF2DData->mapHeightDCC; i += pPDAF2DData->mapWidthDCC)
        {
            for (UINT16 j = 0; j < pPDAF2DData->mapWidthDCC; j ++)
            {
                OsUtils::FPrintF(pPDAFDumpFile, "%d, ", pPDAF2DData->conversionCoefficientPD[i + j]);
            }
            OsUtils::FPrintF(pPDAFDumpFile, "\n");
        }

        OsUtils::FClose(pPDAFDumpFile);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMDataDump::DumpKernelBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMDataDump::DumpKernelBuffer()
{
    FILE*  pKernelDumpFile        = NULL;
    CHAR   fileName[FILENAME_MAX] = { 0 };
    BYTE*  pOTPData               = m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData.pRawData;
    UINT32 memorySize             = m_pSensorInfoTable->moduleCaps.OTPData.EEPROMInfo.rawOTPData.rawDataSize;

    if (pOTPData != NULL)
    {
        OsUtils::SNPrintF(fileName, sizeof(fileName), "%s/%s_%s_%s", ConfigFileDirectory,
                          m_pEEPROMDriverData->slaveInfo.EEPROMName, "kbuffer", "OTP.txt");
        pKernelDumpFile = OsUtils::FOpen(fileName, "w");
    }

    if (pKernelDumpFile != NULL)
    {
        OsUtils::FPrintF(pKernelDumpFile, "Kernel buffer data dump for EEPROM %s\n",
                         m_pEEPROMDriverData->slaveInfo.EEPROMName);
        OsUtils::FPrintF(pKernelDumpFile, "\nNumber of bytes: %u\n", memorySize);
        for (UINT32 index = 0; index < memorySize; index ++)
        {
            OsUtils::FPrintF(pKernelDumpFile, "0x%02x\n", *(pOTPData + index));
        }

        OsUtils::FClose(pKernelDumpFile);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// EEPROMDataDump::DumpEEPROMData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID EEPROMDataDump::DumpEEPROMData()
{
    DumpAFData();
    DumpWBData();
    DumpLSCData();
    DumpDualCameraData();
    DumpPDAFData();
    DumpKernelBuffer();
}

CAMX_NAMESPACE_END
