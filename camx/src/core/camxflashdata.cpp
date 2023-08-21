////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxflashdata.cpp
/// @brief Implements FlashData methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxmem.h"
#include "camxcsl.h"
#include "camxcslsensordefs.h"
#include "camxhwenvironment.h"
#include "camximagesensorutils.h"
#include "camxpacketdefs.h"
#include "camxflashdata.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FlashData::FlashData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FlashData::FlashData(
    FlashDriverData*        pFlashDriverData,
    HwSensorInfo*           pSensorInfoTable,
    const HwDeviceTypeInfo* pDeviceInfo)
{
    CSLFlashQueryCapability flashCapability = { 0 };
    CamxResult              result          = CamxResultEFailed;

    m_pFlashDriverData = pFlashDriverData;
    m_hFlashDeviceIndex = -1;

    for (UINT i = 0; i < pDeviceInfo->deviceIndexCount; i++)
    {
        result = CSLQueryDeviceCapabilities(pDeviceInfo->deviceIndex[i], &flashCapability, sizeof(CSLFlashQueryCapability));
        if (CamxResultSuccess == result)
        {
            if (pSensorInfoTable->CSLCapability.flashSlotId == flashCapability.slotInfo)
            {
                CAMX_LOG_INFO(CamxLogGroupSensor,
                              "Found flash driver for SensorInfoTable: %d index: %u deviceIndex: %d: slotInfo: %u",
                              pSensorInfoTable->deviceIndex,
                              i,
                              pDeviceInfo->deviceIndex[i],
                              flashCapability.slotInfo);

                m_hFlashDeviceIndex = pDeviceInfo->deviceIndex[i];
                result              = CamxResultSuccess;
                break;
            }
        }
    }
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "FlashData create failed deviceIndexCount %d", pDeviceInfo->deviceIndexCount)
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FlashData::~FlashData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FlashData::~FlashData()
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FlashData::GetPowerSequenceCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT FlashData::GetPowerSequenceCmdSize(
    BOOL isPowerUp)
{
    UINT            powerSequenceSize = 0;
    UINT            powerSequenceCommandSize = 0;
    PowerSetting*   pPowerSettings = NULL;

    if (TRUE == isPowerUp)
    {
        powerSequenceSize = m_pFlashDriverData->powerUpSequence.powerSettingCount;
        pPowerSettings = m_pFlashDriverData->powerUpSequence.powerSetting;
    }
    else
    {
        powerSequenceSize = m_pFlashDriverData->powerDownSequence.powerSettingCount;
        pPowerSettings = m_pFlashDriverData->powerDownSequence.powerSetting;
    }

    if ((0 != powerSequenceSize) && (NULL != pPowerSettings))
    {
        powerSequenceCommandSize = ImageSensorUtils::GetPowerSequenceCmdSize(powerSequenceSize, pPowerSettings);
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "Power settings are not valid. Will use default settings");
    }

    return powerSequenceCommandSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FlashData::CreatePowerSequenceCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FlashData::CreatePowerSequenceCmd(
    BOOL    isPowerUp,
    VOID*   pCmdBuffer)
{
    UINT            powerSequenceSize = 0;
    PowerSetting*   pPowerSettings = NULL;

    if (TRUE == isPowerUp)
    {
        powerSequenceSize = m_pFlashDriverData->powerUpSequence.powerSettingCount;
        pPowerSettings = m_pFlashDriverData->powerUpSequence.powerSetting;
    }
    else
    {
        powerSequenceSize = m_pFlashDriverData->powerDownSequence.powerSettingCount;
        pPowerSettings = m_pFlashDriverData->powerDownSequence.powerSetting;
    }

    CAMX_ASSERT_MESSAGE((NULL != pPowerSettings), "Invalid power settings");

    return ImageSensorUtils::CreatePowerSequenceCmd(pCmdBuffer, isPowerUp, powerSequenceSize, pPowerSettings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FlashData::CreateI2CInfoCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FlashData::CreateI2CInfoCmd(
    CSLSensorI2CInfo* pI2CInfoCmd)
{
    CamxResult result = CamxResultSuccess;

    if (m_pFlashDriverData->i2cInfoExists)
    {
        result = ImageSensorUtils::CreateI2CInfoCmd(pI2CInfoCmd,
                                                    m_pFlashDriverData->i2cInfo.slaveAddress,
                                                    m_pFlashDriverData->i2cInfo.i2cFrequencyMode);
    }

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FlashData::GetI2CInitializeCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT FlashData::GetI2CInitializeCmdSize()
{
    UINT             totalSize       = 0;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;
    UINT             regSettingIndex = 0;

    if (m_pFlashDriverData->i2cInfoExists)
    {
        settingsCount = m_pFlashDriverData->i2cInfo.flashInitSettings.regSettingCount;
        pSettings     = m_pFlashDriverData->i2cInfo.flashInitSettings.regSetting;

        totalSize = ImageSensorUtils::GetRegisterSettingsCmdSize(settingsCount, pSettings, &regSettingIndex);
    }

    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FlashData::GetI2CFireCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT FlashData::GetI2CFireCmdSize(
    FlashOperation operation)
{
    UINT             totalSize       = 0;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;
    UINT             regSettingIndex = 0;

    if (m_pFlashDriverData->i2cInfoExists)
    {
        switch (operation)
        {
            case FlashOperation::Off:
                settingsCount = m_pFlashDriverData->i2cInfo.flashOffSettings.regSettingCount;
                pSettings = m_pFlashDriverData->i2cInfo.flashOffSettings.regSetting;
                break;

            case FlashOperation::Low:
                settingsCount = m_pFlashDriverData->i2cInfo.flashLowSettings.regSettingCount;
                pSettings = m_pFlashDriverData->i2cInfo.flashLowSettings.regSetting;
                break;

            case FlashOperation::High:
                settingsCount = m_pFlashDriverData->i2cInfo.flashHighSettings.regSettingCount;
                pSettings = m_pFlashDriverData->i2cInfo.flashHighSettings.regSetting;
                break;

            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported Flash Operation type %d", operation);
                break;
        }

        totalSize = ImageSensorUtils::GetRegisterSettingsCmdSize(settingsCount, pSettings, &regSettingIndex);
    }
    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FlashData::GetI2CFireMaxCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UINT FlashData::GetI2CFireMaxCmdSize()
{
    UINT maxSize = 0;
    UINT getSize = 0;

    for (UINT i = static_cast<UINT>(FlashOperation::Off); i <= static_cast<UINT>(FlashOperation::High); i++)
    {
        getSize = GetI2CFireCmdSize(static_cast<FlashOperation>(i));
        maxSize = getSize > maxSize ? getSize : maxSize;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "maxSize = %d", maxSize);
    return maxSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FlashData::CreateI2CInitializeCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FlashData::CreateI2CInitializeCmd(
    VOID*   pCmdBuffer)
{
    CamxResult       result          = CamxResultSuccess;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;

    settingsCount = m_pFlashDriverData->i2cInfo.flashInitSettings.regSettingCount;
    pSettings     = m_pFlashDriverData->i2cInfo.flashInitSettings.regSetting;

    result = ImageSensorUtils::CreateRegisterSettingsCmd(pCmdBuffer, settingsCount, pSettings, 0,
                                    m_pFlashDriverData->i2cInfo.i2cFrequencyMode);

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FlashData::CreateI2CFireCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FlashData::CreateI2CFireCmd(
    VOID*   pCmdBuffer,
    FlashOperation operation)
{
    CamxResult       result          = CamxResultSuccess;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;

    switch (operation)
    {
        case FlashOperation::Off:
            settingsCount = m_pFlashDriverData->i2cInfo.flashOffSettings.regSettingCount;
            pSettings     = m_pFlashDriverData->i2cInfo.flashOffSettings.regSetting;
            break;

        case FlashOperation::Low:
            settingsCount = m_pFlashDriverData->i2cInfo.flashLowSettings.regSettingCount;
            pSettings     = m_pFlashDriverData->i2cInfo.flashLowSettings.regSetting;
            break;

        case FlashOperation::High:
            settingsCount = m_pFlashDriverData->i2cInfo.flashHighSettings.regSettingCount;
            pSettings     = m_pFlashDriverData->i2cInfo.flashHighSettings.regSetting;
            break;

        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported Flash Operation type %d", operation);
            break;

    }

    result = ImageSensorUtils::CreateRegisterSettingsCmd(pCmdBuffer, settingsCount, pSettings, 0,
                                                            m_pFlashDriverData->i2cInfo.i2cFrequencyMode);

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FlashData::CreateInitializeCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID FlashData::CreateInitializeCmd(
    CSLFlashInfoCmd* pInitCmd)
{
    pInitCmd->flashType = static_cast<UINT32>(m_pFlashDriverData->flashDriverType);
    pInitCmd->cmdType   = CSLSensorCmdTypeFlashInit;
}

CAMX_NAMESPACE_END
