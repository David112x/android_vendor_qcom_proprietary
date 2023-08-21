////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxsensordriverapi.h"
// NOWHINE ENTIRE FILE
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

static const DOUBLE MinAnalogGain           = 1.0;                                      // minimum analog gain (1X)
static const DOUBLE MaxAnalogGain           = 16.0;                                     // maximum analog gain (16X)
static const UINT   AnalogGainDecimator     = 32;                                       // analog gain decimator factor
static const UINT   MinAnalogGainRegister   = MinAnalogGain * AnalogGainDecimator;      // minimum analog gain register value
static const UINT   MaxAnalogGainRegister   = MaxAnalogGain * AnalogGainDecimator;      // maximum analog gain register value
static const DOUBLE MinDigitalGain          = 1.0;                                      // minimum digital gain (1X)
static const DOUBLE MaxDigitalGain          = 1.0;                                      // maximum digital gain (1X), we don't use sensor digital gain currently
static const UINT   DigitalGainDecimator    = 256;                                      // digital gain decimator factor
static const UINT   MinDigitalGainRegister  = MinDigitalGain * DigitalGainDecimator;    // minimum digital gain register value
static const UINT   MaxDigitalGainRegister  = MaxDigitalGain * DigitalGainDecimator;    // maximum digital gain register value



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToAnalogGain
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DOUBLE RegisterToAnalogGain(UINT registerValue)
{
    if (MinAnalogGainRegister > registerValue) // should not happen
    {
        registerValue = MinAnalogGainRegister;
    }
    if (MaxAnalogGainRegister < registerValue)
    {
        registerValue = MaxAnalogGainRegister;
    }

    return static_cast<DOUBLE>(registerValue) / AnalogGainDecimator;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AnalogGainToRegister
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT AnalogGainToRegister(DOUBLE analogGain)
{
    if (MinAnalogGain > analogGain)
    {
        analogGain = MinAnalogGain;
    }
    else if (MaxAnalogGain < analogGain)
    {
        analogGain = MaxAnalogGain;
    }

    return static_cast<UINT>(analogGain * AnalogGainDecimator);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateDigitalGain
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT CalculateDigitalGain(
    DOUBLE totalGain,
    DOUBLE analogGain)
{
    DOUBLE digitalGain = MinDigitalGain;

    if (MaxAnalogGain < totalGain)
    {
        digitalGain = totalGain / analogGain;
    }
    else
    {
        digitalGain = MinDigitalGain;
    }

    if (MaxDigitalGain < digitalGain)
    {
        digitalGain = MaxDigitalGain;
    }

    return static_cast<UINT>(digitalGain * DigitalGainDecimator);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateExposure
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CalculateExposure(
    SensorExposureInfo*          pExposureInfo,
    SensorCalculateExposureData* pCalculateExposureData)
{
    if (NULL == pExposureInfo || NULL == pCalculateExposureData)
    {
        return FALSE;
    }

    pExposureInfo->analogRegisterGain   = AnalogGainToRegister(pCalculateExposureData->realGain);
    pExposureInfo->analogRealGain       = static_cast<FLOAT>(RegisterToAnalogGain(pExposureInfo->analogRegisterGain));
    pExposureInfo->digitalRegisterGain  =
        CalculateDigitalGain(pCalculateExposureData->realGain, pExposureInfo->analogRealGain);
    pExposureInfo->digitalRealGain      = static_cast<FLOAT>(pExposureInfo->digitalRegisterGain) / DigitalGainDecimator;
    pExposureInfo->ISPDigitalGain       =
        pCalculateExposureData->realGain / (pExposureInfo->analogRealGain * pExposureInfo->digitalRealGain);
    pExposureInfo->lineCount            = pCalculateExposureData->lineCount;

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FillExposureSettings
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FillExposureSettings(
    RegSettingsInfo*        pRegSettingsInfo,
    SensorFillExposureData* pExposureData)
{
    UINT32  index     = 0;
    UINT16  regCount  = 0;

    if ((NULL == pRegSettingsInfo) || (NULL == pExposureData))
    {
        return FALSE;
    }

    for (index = 0; index < pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount; index++)
    {
        pRegSettingsInfo->regSetting[regCount].registerAddr =
            pExposureData->pRegInfo->groupHoldOnSettings.regSetting[index].registerAddr;
        pRegSettingsInfo->regSetting[regCount].registerData =
            pExposureData->pRegInfo->groupHoldOnSettings.regSetting[index].registerData;
        pRegSettingsInfo->regSetting[regCount].regAddrType  =
            pExposureData->pRegInfo->groupHoldOnSettings.regSetting[index].regAddrType;
        pRegSettingsInfo->regSetting[regCount].regDataType  =
            pExposureData->pRegInfo->groupHoldOnSettings.regSetting[index].regDataType;
        pRegSettingsInfo->regSetting[regCount].delayUs      =
            pExposureData->pRegInfo->groupHoldOnSettings.regSetting[index].delayUs;
        regCount++;
    }

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->frameLengthLinesAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->frameLengthLines & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->frameLengthLinesAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->frameLengthLines & 0xFF);
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->lineCount & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->lineCount & 0xFF);
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->analogRegisterGain & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr+ 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->analogRegisterGain & 0xFF);
    regCount++;

    for (index = 0; (pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index) < regCount; index++)
    {
        pRegSettingsInfo->regSetting[pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index].regAddrType  =
            I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index].regDataType  =
            I2CRegAddressDataTypeByte;
        pRegSettingsInfo->regSetting[pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index].delayUs      =
            0;
    }

    for (index = 0; index < pExposureData->pRegInfo->groupHoldOffSettings.regSettingCount; index++)
    {
        pRegSettingsInfo->regSetting[regCount].registerAddr  =
            pExposureData->pRegInfo->groupHoldOffSettings.regSetting[index].registerAddr;
        pRegSettingsInfo->regSetting[regCount].registerData  =
            pExposureData->pRegInfo->groupHoldOffSettings.regSetting[index].registerData;
        pRegSettingsInfo->regSetting[regCount].regAddrType  =
            pExposureData->pRegInfo->groupHoldOffSettings.regSetting[index].regAddrType;
        pRegSettingsInfo->regSetting[regCount].regDataType=
            pExposureData->pRegInfo->groupHoldOffSettings.regSetting[index].regDataType;
        regCount++;
    }

    pRegSettingsInfo->regSettingCount = regCount;

    if (MAX_REG_SETTINGS <= regCount)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetSensorLibraryAPIs
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetSensorLibraryAPIs(
    SensorLibraryAPI* pSensorLibraryAPI)
{
    pSensorLibraryAPI->majorVersion          = 1;
    pSensorLibraryAPI->minorVersion          = 0;
    pSensorLibraryAPI->pCalculateExposure    = CalculateExposure;
    pSensorLibraryAPI->pFillExposureSettings = FillExposureSettings;
}

#ifdef __cplusplus
} // CamX Namespace
#endif // __cplusplus
