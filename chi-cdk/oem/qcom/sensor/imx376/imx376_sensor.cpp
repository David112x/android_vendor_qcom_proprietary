////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE ENTIRE FILE
#include "camxsensordriverapi.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

static const UINT   IMX376MinRegisterGain        = 0;                                     ///< Minimum analog register gain
static const UINT   IMX376MaxRegisterGain        = 960;                                   ///< Maximum analog register gain
static const DOUBLE IMX376MinRealGain            = (1024 / (1024 - IMX376MinRegisterGain)); ///< Minimum analog real gain (1X)
static const DOUBLE IMX376MaxRealGain            = (1024 / (1024 - IMX376MaxRegisterGain)); ///< Maximum analog real gain (16X)
static const UINT   IMX376MinDigitalRegisterGain = 256;                                   ///< Minimum digital register gain
static const UINT   IMX376MaxDigitalRegisterGain = 256;                                   ///< Maximum digital register gain
static const UINT   IMX376DigitalGainDecimator   = 256;                                   /// < Digital gain decimator factor
static const DOUBLE IMX376MinDigitalRealGain     = (IMX376MinDigitalRegisterGain / IMX376DigitalGainDecimator);
                                                                                          ///< Minimum digital real gain (1X)
static const DOUBLE IMX376MaxDigitalRealGain     = (IMX376MaxDigitalRegisterGain/ IMX376DigitalGainDecimator);
                                                                                          ///< Maximum digital real gain (1X)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToRealGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DOUBLE RegisterToRealGain(
    UINT registerGain)
{
    if (IMX376MaxRegisterGain < registerGain)
    {
        registerGain = IMX376MaxRegisterGain;
    }

    return  (1024.0 / (1024.0 - registerGain));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealToRegisterGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT RealToRegisterGain(
    DOUBLE realGain)
{
    if (IMX376MinRealGain > realGain)
    {
        realGain = IMX376MinRealGain;
    }
    else if (IMX376MaxRealGain < realGain)
    {
        realGain = IMX376MaxRealGain;
    }

    return static_cast<UINT>(1024.0 - (1024.0 / realGain));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateDigitalGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT CalculateDigitalGain(
    FLOAT totalRealGain,
    FLOAT sensorRealGain)
{
    float digitalRealGain = IMX376MinDigitalRealGain;

    if (IMX376MaxRealGain < totalRealGain)
    {
        digitalRealGain = totalRealGain / sensorRealGain;
    }
    else
    {
        digitalRealGain = IMX376MinDigitalRealGain;
    }

    if (IMX376MaxDigitalRealGain < digitalRealGain)
    {
        digitalRealGain = IMX376MaxDigitalRealGain;
    }

    return static_cast<UINT>(digitalRealGain * IMX376DigitalGainDecimator);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateExposure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CalculateExposure(
    SensorExposureInfo*          pExposureInfo,
    SensorCalculateExposureData* pCalculateExposureData)
{
    if (NULL == pExposureInfo || NULL == pCalculateExposureData)
    {
        return FALSE;
    }

    pExposureInfo->analogRegisterGain   = RealToRegisterGain(pCalculateExposureData->realGain);
    pExposureInfo->analogRealGain       = static_cast<FLOAT>(RegisterToRealGain(pExposureInfo->analogRegisterGain));
    pExposureInfo->digitalRegisterGain  = CalculateDigitalGain(pCalculateExposureData->realGain, pExposureInfo->analogRealGain);
    pExposureInfo->digitalRealGain      = static_cast<FLOAT>(pExposureInfo->digitalRegisterGain / IMX376DigitalGainDecimator);
    pExposureInfo->ISPDigitalGain       =
        pCalculateExposureData->realGain /(pExposureInfo->analogRealGain * pExposureInfo->digitalRealGain);
    pExposureInfo->lineCount            = pCalculateExposureData->lineCount;

    // Since isp channel gain block is common to both short and long, remove isp digital gain from short sensitivity.
    pCalculateExposureData->shortRealGain = pCalculateExposureData->shortRealGain / pExposureInfo->ISPDigitalGain;

    if (pCalculateExposureData->shortRealGain < 1.0f)
    {
        // Considering isp digital gain in exposuretime, since gain exausted.
        pCalculateExposureData->shortLinecount =
            static_cast<UINT>(pCalculateExposureData->shortRealGain * pCalculateExposureData->shortLinecount);
        // Making linecount 1 if it falls less then 1
        pCalculateExposureData->shortLinecount =
            pCalculateExposureData->shortLinecount < 1 ? 1 : pCalculateExposureData->shortLinecount;
        pCalculateExposureData->shortRealGain = 1.0f;
    }
    pExposureInfo->shortRegisterGain    = RealToRegisterGain(pCalculateExposureData->shortRealGain);
    pExposureInfo->shortLinecount       = pCalculateExposureData->shortLinecount;

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FillExposureSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->analogRegisterGain & 0xFF);
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->digitalGainBlueAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->digitalGainBlueAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF);
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->digitalGainGreenBlueAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->digitalGainGreenBlueAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF);
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->digitalGainGreenRedAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->digitalGainGreenRedAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF);
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->digitalGainRedAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->digitalGainRedAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF);
    regCount++;

    if (TRUE == pExposureData->applyShortExposure)
    {
        pRegSettingsInfo->regSetting[regCount].registerAddr = pExposureData->pRegInfo->shortCoarseIntgTimeAddr;
        pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->shortLineCount & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = pExposureData->pRegInfo->shortCoarseIntgTimeAddr + 1;
        pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->shortLineCount & 0xFF);
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = pExposureData->pRegInfo->shortGlobalAnalogGainAddr;
        pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->shortRegisterGain & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = pExposureData->pRegInfo->shortGlobalAnalogGainAddr + 1;
        pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->shortRegisterGain & 0xFF);
        regCount++;
    }

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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetSensorLibraryAPIs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
