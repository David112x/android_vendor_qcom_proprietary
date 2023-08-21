////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 - 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxsensordriverapi.h"
// NOWHINE ENTIRE FILE

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

static const UINT  OV12A10MinRegisterGain        = 0x0080;             ///< Minimum analog register gain
static const UINT  OV12A10MaxRegisterGain        = 0x07F0;             ///< Maximum analog register gain
static const UINT  OV12A10MinDigitalGain         = 1;                  ///< Minimum digital gain
static const UINT  OV12A10MaxDigitalGain         = 4;                  ///< Maximum digital gain
static const FLOAT OV12A10MinRealGain            = 1.0f;               ///< Minimum analog real gain(1X)
static const FLOAT OV12A10MaxRealGain            = 15.5f;              ///< Maximum analog real gain(15.5X)
static const UINT  OV12A10DigitalGainDecimator   = 1024;               ///< Digital gain decimator factor
                                                                       ///< Dig gain 1x = Reg gain 0x0400
static const UINT  OV12A10RegisterGainDecimator  = 128;                ///< Register gain decimator factor

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToRealGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static FLOAT RegisterToRealGain(
    UINT registerGain)
{
    FLOAT tempRegisterGain = static_cast<FLOAT>(registerGain);

    if (OV12A10MaxRegisterGain < tempRegisterGain)
    {
        tempRegisterGain = OV12A10RegisterGainDecimator * OV12A10MaxRealGain;
    }
    else if (OV12A10MinRegisterGain > tempRegisterGain)
    {
        tempRegisterGain = OV12A10RegisterGainDecimator * OV12A10MinRealGain;
    }

    return  static_cast<FLOAT>(tempRegisterGain / OV12A10RegisterGainDecimator);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealToRegisterGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT RealToRegisterGain(
    FLOAT realGain)
{
    FLOAT tempRealGain = realGain;
    UINT  registerGain = OV12A10MinRegisterGain;

    if (OV12A10MinRealGain > realGain)
    {
        tempRealGain = OV12A10MinRealGain;
    }

    if (2.0f >= realGain)
    {
        tempRealGain = ((int)(realGain * 16.0f)) / 16.0f;
    }
    else if (4.0f >= realGain)
    {
        tempRealGain = ((int)(realGain * 8.0f)) / 8.0f;
    }
    else if (8.0f >= realGain)
    {
        tempRealGain = ((int)(realGain * 4.0f)) / 4.0f;
    }
    else
    {
        tempRealGain = ((int)(realGain * 2.0f)) / 2.0f;
    }

    if (OV12A10MinRealGain > tempRealGain)
    {
        tempRealGain = OV12A10MinRealGain;
    }
    else if (OV12A10MaxRealGain < tempRealGain)
    {
        tempRealGain = OV12A10MaxRealGain;
    }

    registerGain = static_cast<UINT>(tempRealGain * OV12A10RegisterGainDecimator);
    return registerGain;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateDigitalGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT CalculateDigitalGain(
    FLOAT realGain,
    FLOAT analogRealGain)
{
    FLOAT digitalRealGain = 0.0;
    digitalRealGain = realGain/analogRealGain;

    if (OV12A10MinDigitalGain > digitalRealGain)
    {
        digitalRealGain = OV12A10MinDigitalGain;
    }
    else if (OV12A10MaxDigitalGain < digitalRealGain)
    {
        digitalRealGain = OV12A10MaxDigitalGain;
    }

    return static_cast<UINT>(digitalRealGain * OV12A10DigitalGainDecimator);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FillExposureSettings
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
    pExposureInfo->digitalRealGain      = static_cast<FLOAT>(pExposureInfo->digitalRegisterGain) /
        static_cast<FLOAT>(OV12A10DigitalGainDecimator);
    pExposureInfo->ISPDigitalGain       =
        pCalculateExposureData->realGain / (pExposureInfo->analogRealGain *  pExposureInfo->digitalRealGain);
    pExposureInfo->lineCount            = pCalculateExposureData->lineCount;

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
    UINT    regCount  = 0;

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
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->lineCount >> 12) & 0xFF;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->lineCount >> 4) & 0xFF;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr = pExposureData->pRegInfo->coarseIntgTimeAddr + 2;
    pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->lineCount << 4) & 0xFF;
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

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->GlobalDigitalGainAddr+ 1;
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
