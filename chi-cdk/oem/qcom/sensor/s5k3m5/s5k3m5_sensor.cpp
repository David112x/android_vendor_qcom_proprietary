////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxsensordriverapi.h"
// NOWHINE ENTIRE FILE

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

static const UINT   S5K3M5MinRegisterGain        = 32;                                    ///< Minimum analog register gain
static const UINT   S5K3M5MaxRegisterGain        = 512;                                   ///< Maximum analog register gain
static const DOUBLE S5K3M5MinRealGain            = (S5K3M5MinRegisterGain/32);            ///< Minimum analog real gain (1X)
static const DOUBLE S5K3M5MaxRealGain            = (S5K3M5MaxRegisterGain/32);            ///< Maximum analog real gain (16X)
static const UINT   S5K3M5MinDigitalRegisterGain = 256;                                   ///< Minimum digital register gain
static const UINT   S5K3M5MaxDigitalRegisterGain = 256;                                   ///< Maximum digital register gain
static const UINT   S5K3M5DigitalGainDecimator   = 256;                                   /// < Digital gain decimator factor
static const DOUBLE S5K3M5MinDigitalRealGain     = (S5K3M5MinDigitalRegisterGain / S5K3M5DigitalGainDecimator);
                                                                                          ///< Minimum digital real gain (1X)
static const DOUBLE S5K3M5MaxDigitalRealGain     = (S5K3M5MaxDigitalRegisterGain/ S5K3M5DigitalGainDecimator);
                                                                                          ///< Maximum digital real gain (1X)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToRealGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DOUBLE RegisterToRealGain(
    UINT registerGain)
{
    if(S5K3M5MaxRegisterGain < registerGain)
    {
        registerGain = S5K3M5MaxRegisterGain;
    }

    return ((float)registerGain/32);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealToRegisterGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT RealToRegisterGain(
    DOUBLE realGain)
{
    if (S5K3M5MinRealGain > realGain)
    {
        realGain = S5K3M5MinRealGain;
    }
    else if (S5K3M5MaxRealGain < realGain)
    {
        realGain = S5K3M5MaxRealGain;
    }

    return static_cast<UINT>(realGain*32);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateDigitalGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT CalculateDigitalGain(
    FLOAT totalRealGain,
    FLOAT sensorRealGain)
{
    float digitalRealGain = S5K3M5MinDigitalRealGain;

    if (S5K3M5MaxRealGain < totalRealGain)
    {
        digitalRealGain = totalRealGain / sensorRealGain;
    }
    else
    {
        digitalRealGain = S5K3M5MinDigitalRealGain;
    }

    if (S5K3M5MaxDigitalRealGain < digitalRealGain)
    {
        digitalRealGain = S5K3M5MaxDigitalRealGain;
    }

    return static_cast<UINT>(digitalRealGain * S5K3M5DigitalGainDecimator);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FillExposureSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CalculateExposure(
    SensorExposureInfo*          pExposureInfo,
    SensorCalculateExposureData* pCalculateExposureData)
{
    BOOL result = FALSE;

    if ((NULL != pExposureInfo) && (NULL != pCalculateExposureData))
    {
        pExposureInfo->analogRegisterGain   = RealToRegisterGain(pCalculateExposureData->realGain);
        pExposureInfo->analogRealGain       = static_cast<FLOAT>(RegisterToRealGain(pExposureInfo->analogRegisterGain));
        pExposureInfo->digitalRegisterGain  = CalculateDigitalGain(pCalculateExposureData->realGain,
                                                                   pExposureInfo->analogRealGain);
        pExposureInfo->digitalRealGain      = static_cast<FLOAT>(pExposureInfo->digitalRegisterGain /
                                                                 S5K3M5DigitalGainDecimator);
        pExposureInfo->ISPDigitalGain       = pCalculateExposureData->realGain /
                                              (pExposureInfo->analogRealGain * pExposureInfo->digitalRealGain);
        pExposureInfo->lineCount            = pCalculateExposureData->lineCount;

        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FillExposureSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FillExposureSettings(
    RegSettingsInfo*        pRegSettingsInfo,
    SensorFillExposureData* pExposureData)
{
    BOOL   result   = FALSE;
    UINT32 index    = 0;
    UINT16 regCount = 0;

    if ((NULL != pRegSettingsInfo) && (NULL != pExposureData))
    {
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
        pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->lineCount & 0x00FF);
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr;
        pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->analogRegisterGain & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr+ 1;
        pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->analogRegisterGain & 0xFF);
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->GlobalDigitalGainAddr;
        pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->GlobalDigitalGainAddr + 1;
        pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF);
        regCount++;

        for (index = 0; (pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index) < regCount; index++)
        {
            pRegSettingsInfo->regSetting[pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index].regAddrType =
                I2CRegAddressDataTypeWord;
            pRegSettingsInfo->regSetting[pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index].regDataType =
                I2CRegAddressDataTypeByte;
            pRegSettingsInfo->regSetting[pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index].delayUs     =
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

        result = TRUE;
    }

    if ((TRUE == result) && (MAX_REG_SETTINGS <= regCount))
    {
        result = FALSE;
    }

    return result;
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
