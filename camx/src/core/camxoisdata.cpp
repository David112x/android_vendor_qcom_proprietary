////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxoisdata.cpp
/// @brief Implements OisData methods.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxmem.h"
#include "camxpacketdefs.h"
#include "camximagesensorutils.h"
#include "camxoisdata.h"


CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OISData::OISData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OISData::OISData(
    OISDriverData* pOISDriverData)
{
    m_pOisDriverData = pOISDriverData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// OISData::~OISData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
OISData::~OISData()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::CalibrateOISDriverData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OISData::CalibrateOISDriverData() const
{
    CamxResult result = CamxResultSuccess;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::CreateI2CInfoCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OISData::CreateI2CInfoCmd(
    CSLOISI2CInfo* pI2CInfoCmd)
{
    CamxResult result = CamxResultSuccess;

    pI2CInfoCmd->slaveAddr          = static_cast<UINT32>(m_pOisDriverData->slaveInfo.slaveAddress);
    pI2CInfoCmd->I2CFrequencyMode   = static_cast<UINT8>(m_pOisDriverData->slaveInfo.i2cFrequencyMode);
    pI2CInfoCmd->OISFwFlag          = static_cast<UINT8>(m_pOisDriverData->slaveInfo.fwflag);
    pI2CInfoCmd->isOISCalib         = static_cast<UINT8>(m_pOisDriverData->slaveInfo.oiscalib);
    pI2CInfoCmd->opcode.coeff       = m_pOisDriverData->opcode.coeff;
    pI2CInfoCmd->opcode.pheripheral = m_pOisDriverData->opcode.pheripheral;
    pI2CInfoCmd->opcode.prog        = m_pOisDriverData->opcode.prog;
    pI2CInfoCmd->opcode.memory      = m_pOisDriverData->opcode.memory;
    pI2CInfoCmd->cmdType            = CSLSensorCmdTypeI2CInfo;

    OsUtils::StrLCpy(pI2CInfoCmd->OISName, m_pOisDriverData->slaveInfo.OISName, sizeof(pI2CInfoCmd->OISName));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::GetPowerSequenceCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT OISData::GetPowerSequenceCmdSize(
    BOOL isPowerUp)
{
    UINT            powerSequenceSize        = 0;
    UINT            powerSequenceCommandSize = 0;
    PowerSetting*   pPowerSettings           = NULL;

    if (TRUE == isPowerUp)
    {
        powerSequenceSize   = m_pOisDriverData->slaveInfo.powerUpSequence.powerSettingCount;
        pPowerSettings      = m_pOisDriverData->slaveInfo.powerUpSequence.powerSetting;
    }
    else
    {
        powerSequenceSize   = m_pOisDriverData->slaveInfo.powerDownSequence.powerSettingCount;
        pPowerSettings      = m_pOisDriverData->slaveInfo.powerDownSequence.powerSetting;
    }

    if ((0 != powerSequenceSize) && (NULL != pPowerSettings))
    {
        powerSequenceCommandSize =  ImageSensorUtils::GetPowerSequenceCmdSize(powerSequenceSize, pPowerSettings);
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupSensor, "Power settings are not valid. Will use default settings");
    }

    return powerSequenceCommandSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::CreatePowerSequenceCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OISData::CreatePowerSequenceCmd(
    BOOL    isPowerUp,
    VOID*   pCmdBuffer)
{
    UINT            powerSequenceSize   = 0;
    PowerSetting*   pPowerSettings      = NULL;

    if (TRUE == isPowerUp)
    {
        powerSequenceSize   = m_pOisDriverData->slaveInfo.powerUpSequence.powerSettingCount;
        pPowerSettings      = m_pOisDriverData->slaveInfo.powerUpSequence.powerSetting;
    }
    else
    {
        powerSequenceSize   = m_pOisDriverData->slaveInfo.powerDownSequence.powerSettingCount;
        pPowerSettings      = m_pOisDriverData->slaveInfo.powerDownSequence.powerSetting;
    }

    CAMX_ASSERT_MESSAGE((NULL != pPowerSettings), "Invalid power settings");

    return ImageSensorUtils::CreatePowerSequenceCmd(pCmdBuffer, isPowerUp, powerSequenceSize, pPowerSettings);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::GetInitializeCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT OISData::GetInitializeCmdSize()
{
    UINT             totalSize       = 0;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;
    UINT             regSettingIndex = 0;

    settingsCount = m_pOisDriverData->oisinitSettings.regSettingCount;
    pSettings     = m_pOisDriverData->oisinitSettings.regSetting;

    totalSize = ImageSensorUtils::GetRegisterSettingsCmdSize(settingsCount, pSettings, &regSettingIndex);

    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::CreateInitializeCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OISData::CreateInitializeCmd(
    VOID*   pCmdBuffer)
{
    CamxResult       result          = CamxResultSuccess;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;

    settingsCount = m_pOisDriverData->oisinitSettings.regSettingCount;
    pSettings     = m_pOisDriverData->oisinitSettings.regSetting;

    result = ImageSensorUtils::CreateRegisterSettingsCmd(pCmdBuffer,
                                                         settingsCount,
                                                         pSettings,
                                                         0,
                                                         m_pOisDriverData->slaveInfo.i2cFrequencyMode);

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::GetCalibrationCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT OISData::GetCalibrationCmdSize()
{
    UINT             totalSize       = 0;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;
    UINT             regSettingIndex = 0;

    settingsCount = m_pCalibSettings->regSettingCount;
    pSettings     = m_pCalibSettings->regSetting;

    totalSize = ImageSensorUtils::GetRegisterSettingsCmdSize(settingsCount, pSettings, &regSettingIndex);

    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::CreateCalibrationCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OISData::CreateCalibrationCmd(
    VOID*   pCmdBuffer)
{
    CamxResult       result          = CamxResultSuccess;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;

    settingsCount = m_pCalibSettings->regSettingCount;
    pSettings     = m_pCalibSettings->regSetting;

    result = ImageSensorUtils::CreateRegisterSettingsCmd(pCmdBuffer,
                                                         settingsCount,
                                                         pSettings,
                                                         0,
                                                         m_pOisDriverData->slaveInfo.i2cFrequencyMode);

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::GetOISModeMaxCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UINT OISData::GetOISModeMaxCmdSize()
{
    UINT maxSize = 0;
    UINT getSize = 0;

    for (UINT i = static_cast<UINT>(OISMode::EnableOIS); i < static_cast<UINT>(OISMode::ModeEnd); i++)
    {
        getSize = GetOISModeCmdSize(static_cast<OISMode>(i));
        maxSize = getSize > maxSize ? getSize : maxSize;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "maxSize = %d", maxSize);
    return maxSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::GetOISModeCmdSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT OISData::GetOISModeCmdSize(
    OISMode mode)
{
    UINT             totalSize       = 0;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;
    UINT             regSettingIndex = 0;

    switch (mode)
    {
        case OISMode::EnableOIS:
            settingsCount = m_pOisDriverData->enableOisSettings.regSettingCount;
            pSettings     = m_pOisDriverData->enableOisSettings.regSetting;
            break;

        case OISMode::DisableOIS:
            settingsCount = m_pOisDriverData->disableOisSettings.regSettingCount;
            pSettings     = m_pOisDriverData->disableOisSettings.regSetting;
            break;

        case OISMode::Movie:
            settingsCount = m_pOisDriverData->movieModeSettings.regSettingCount;
            pSettings     = m_pOisDriverData->movieModeSettings.regSetting;
            break;

        case OISMode::Still:
            settingsCount = m_pOisDriverData->stillModeSettings.regSettingCount;
            pSettings     = m_pOisDriverData->stillModeSettings.regSetting;
            break;

        case OISMode::EnterDownLoadMode:
            settingsCount = m_pOisDriverData->enterDownloadModeSettings.regSettingCount;
            pSettings     = m_pOisDriverData->enterDownloadModeSettings.regSetting;
            break;

        case OISMode::CenteringOn:
            settingsCount = m_pOisDriverData->centeringOnSettings.regSettingCount;
            pSettings     = m_pOisDriverData->centeringOnSettings.regSetting;
            break;

        case OISMode::CenteringOff:
            settingsCount = m_pOisDriverData->centeringOffSettings.regSettingCount;
            pSettings     = m_pOisDriverData->centeringOffSettings.regSetting;
            break;

        case OISMode::Pantilt:
            settingsCount = m_pOisDriverData->pantiltOnSettings.regSettingCount;
            pSettings     = m_pOisDriverData->pantiltOnSettings.regSetting;
            break;

        case OISMode::Scene:
            settingsCount = m_pOisDriverData->sceneOisSettings.regSettingCount;
            pSettings     = m_pOisDriverData->sceneOisSettings.regSetting;
            break;

        case OISMode::SceneFilterOn:
            settingsCount = m_pOisDriverData->sceneFilterOnSettings.regSettingCount;
            pSettings     = m_pOisDriverData->sceneFilterOnSettings.regSetting;
            break;

        case OISMode::SceneFilterOff:
            settingsCount = m_pOisDriverData->sceneFilterOffSettings.regSettingCount;
            pSettings     = m_pOisDriverData->sceneFilterOffSettings.regSetting;
            break;

        case OISMode::SceneRangeOn:
            settingsCount = m_pOisDriverData->sceneRangeOnSettings.regSettingCount;
            pSettings     = m_pOisDriverData->sceneRangeOnSettings.regSetting;
            break;

        case OISMode::SceneRangeOff:
            settingsCount = m_pOisDriverData->sceneFilterOffSettings.regSettingCount;
            pSettings     = m_pOisDriverData->sceneFilterOffSettings.regSetting;
            break;

        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported OIS mode type %d", mode);
            break;

    }

    totalSize = ImageSensorUtils::GetRegisterSettingsCmdSize(settingsCount, pSettings, &regSettingIndex);

    return totalSize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OISData::CreateOISModeCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult OISData::CreateOISModeCmd(
    VOID*   pCmdBuffer,
    OISMode mode)

{
    CamxResult       result          = CamxResultSuccess;
    UINT             settingsCount   = 0;
    RegisterSetting* pSettings       = NULL;

    switch (mode)
    {
        case OISMode::EnableOIS:
            settingsCount = m_pOisDriverData->enableOisSettings.regSettingCount;
            pSettings     = m_pOisDriverData->enableOisSettings.regSetting;
            break;

        case OISMode::DisableOIS:
            settingsCount = m_pOisDriverData->disableOisSettings.regSettingCount;
            pSettings     = m_pOisDriverData->disableOisSettings.regSetting;
            break;

        case OISMode::Movie:
            settingsCount = m_pOisDriverData->movieModeSettings.regSettingCount;
            pSettings     = m_pOisDriverData->movieModeSettings.regSetting;
            break;

        case OISMode::Still:
            settingsCount = m_pOisDriverData->stillModeSettings.regSettingCount;
            pSettings     = m_pOisDriverData->stillModeSettings.regSetting;
            break;

        case OISMode::EnterDownLoadMode:
            settingsCount = m_pOisDriverData->enterDownloadModeSettings.regSettingCount;
            pSettings     = m_pOisDriverData->enterDownloadModeSettings.regSetting;
            break;

        case OISMode::CenteringOn:
            settingsCount = m_pOisDriverData->centeringOnSettings.regSettingCount;
            pSettings     = m_pOisDriverData->centeringOnSettings.regSetting;
            break;

        case OISMode::CenteringOff:
            settingsCount = m_pOisDriverData->centeringOffSettings.regSettingCount;
            pSettings     = m_pOisDriverData->centeringOffSettings.regSetting;
            break;

        case OISMode::Pantilt:
            settingsCount = m_pOisDriverData->pantiltOnSettings.regSettingCount;
            pSettings     = m_pOisDriverData->pantiltOnSettings.regSetting;
            break;

        case OISMode::Scene:
            settingsCount = m_pOisDriverData->sceneOisSettings.regSettingCount;
            pSettings     = m_pOisDriverData->sceneOisSettings.regSetting;
            break;

        case OISMode::SceneFilterOn:
            settingsCount = m_pOisDriverData->sceneFilterOnSettings.regSettingCount;
            pSettings     = m_pOisDriverData->sceneFilterOnSettings.regSetting;
            break;

        case OISMode::SceneFilterOff:
            settingsCount = m_pOisDriverData->sceneFilterOffSettings.regSettingCount;
            pSettings     = m_pOisDriverData->sceneFilterOffSettings.regSetting;
            break;

        case OISMode::SceneRangeOn:
            settingsCount = m_pOisDriverData->sceneRangeOnSettings.regSettingCount;
            pSettings     = m_pOisDriverData->sceneRangeOnSettings.regSetting;
            break;

        case OISMode::SceneRangeOff:
            settingsCount = m_pOisDriverData->sceneFilterOffSettings.regSettingCount;
            pSettings     = m_pOisDriverData->sceneFilterOffSettings.regSetting;
            break;

        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Unsupported OIS mode type %d", mode);
            break;

    }

    result = ImageSensorUtils::CreateRegisterSettingsCmd(pCmdBuffer,
                                                         settingsCount,
                                                         pSettings,
                                                         0,
                                                         m_pOisDriverData->slaveInfo.i2cFrequencyMode);

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

CAMX_NAMESPACE_END
