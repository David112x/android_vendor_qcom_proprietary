////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxsensordriverapi.h"
#include "chistatsinterfacedefs.h"
// NOWHINE ENTIRE FILE
#include <android/log.h>
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

static const UINT   IMX563MinRegisterGain        = 0;                                     ///< Minimum analog register gain
static const UINT   IMX563MaxRegisterGain        = 896;                                   ///< Maximum analog register gain
static const DOUBLE IMX563MinRealGain            = (1024 / (1024 - IMX563MinRegisterGain)); ///< Minimum analog real gain (1X)
static const DOUBLE IMX563MaxRealGain            = (1024 / (1024 - IMX563MaxRegisterGain)); ///< Maximum analog real gain (16X)
static const UINT   IMX563MinDigitalRegisterGain = 256;                                   ///< Minimum digital register gain
static const UINT   IMX563MaxDigitalRegisterGain = 4096;                                  ///< Maximum digital register gain
static const FLOAT   IMX563DigitalGainDecimator  = 256.0f;                                /// < Digital gain decimator factor
static const DOUBLE IMX563MinDigitalRealGain     = (IMX563MinDigitalRegisterGain / IMX563DigitalGainDecimator);
                                                                                          ///< Minimum digital real gain (1X)
static const DOUBLE IMX563MaxDigitalRealGain     = (IMX563MaxDigitalRegisterGain/ IMX563DigitalGainDecimator);
                                                                                          ///< Maximum digital real gain (16X)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToRealGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DOUBLE RegisterToRealGain(
    UINT registerGain)
{
    if(IMX563MaxRegisterGain < registerGain)
    {
        registerGain = IMX563MaxRegisterGain;
    }

    return  (1024.0 / (1024.0 - registerGain));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealToRegisterGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT RealToRegisterGain(
    DOUBLE realGain)
{
    if (IMX563MinRealGain > realGain)
    {
        realGain = IMX563MinRealGain;
    }
    else if (IMX563MaxRealGain < realGain)
    {
        realGain = IMX563MaxRealGain;
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
    UINT32 digitalMSB = 0, digitalLSB = 0, digitalGain = 0;

    float digitalRealGain = IMX563MinDigitalRealGain;

    // digital gain upper byte = integer part of DigGain
    // digital gain lower bye  = fractional part *256

    if (IMX563MaxRealGain < totalRealGain)
    {
        digitalRealGain = totalRealGain / sensorRealGain;
    }
    else
    {
        digitalRealGain = IMX563MinDigitalRealGain;
    }

    if (IMX563MaxDigitalRealGain < digitalRealGain)
    {
        digitalRealGain = IMX563MaxDigitalRealGain;
    }

    return static_cast<UINT>(digitalRealGain * IMX563DigitalGainDecimator);

}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateExposure
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
        pExposureInfo->digitalRegisterGain  = CalculateDigitalGain(pCalculateExposureData->realGain, pExposureInfo->analogRealGain);
        pExposureInfo->digitalRealGain      = static_cast<FLOAT>(pExposureInfo->digitalRegisterGain) / IMX563DigitalGainDecimator;
        pExposureInfo->ISPDigitalGain       =
            pCalculateExposureData->realGain /(pExposureInfo->analogRealGain * pExposureInfo->digitalRealGain);
        pExposureInfo->lineCount            = pCalculateExposureData->lineCount;
        pExposureInfo->shortRegisterGain    = RealToRegisterGain(pCalculateExposureData->shortRealGain);
        pExposureInfo->shortLinecount       = pCalculateExposureData->shortLinecount;
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
        pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->lineCount & 0xFF);
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
    pSensorLibraryAPI->majorVersion                  = 1;
    pSensorLibraryAPI->minorVersion                  = 0;
    pSensorLibraryAPI->pCalculateExposure            = CalculateExposure;
    pSensorLibraryAPI->pFillExposureSettings         = FillExposureSettings;
    pSensorLibraryAPI->pStatsParse                   = NULL;
    pSensorLibraryAPI->pFillAutoWhiteBalanceSettings = NULL;
}

#ifdef __cplusplus
} // CamX Namespace
#endif // __cplusplus
