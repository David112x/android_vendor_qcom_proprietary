////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxsensordriverapi.h"
// NOWHINE ENTIRE FILE
#include <android/log.h>
#include <math.h>
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#define IMX334_DBG(fmt, args...) __android_log_print(ANDROID_LOG_INFO, "IMX334",fmt, ##args)

static const INT   IMX334MinRegisterGain        = 0;                                     ///< Minimum analog register gain
static const INT   IMX334MaxRegisterGain        = 240;                                   ///< Maximum analog register gain
/* TODO: Check for DGAIN CALC
static const UINT   IMX318MinDigitalRegisterGain = 1;                                   ///< Minimum digital register gain
static const UINT   IMX318MaxDigitalRegisterGain = 140;                                   ///< Maximum digital register gain
static const DOUBLE IMX334MinDigitalRealGain     = ((3 * IMX318MinDigitalRegisterGain) / 10);
                                                                                 ///< Minimum digital real gain (1X)
static const DOUBLE IMX334MaxDigitalRealGain     = ((3 * IMX318MaxDigitalRegisterGain) / 10);
                                                                                          ///< Maximum digital real gain (1X)
*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToRealGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DOUBLE RegisterToRealGain(
    UINT registerGain)
{
    if(IMX334MaxRegisterGain < registerGain)
    {
        registerGain = IMX334MaxRegisterGain;
    }

    return powf(10, (((static_cast<FLOAT>(registerGain))*0.3)/20.0));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealToRegisterGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static INT RealToRegisterGain(
    DOUBLE realGain)
{
    INT regGain = (20.0 * log10(realGain) *10.0)/3.0;

    if (IMX334MinRegisterGain > regGain)
    {
        realGain = IMX334MinRegisterGain;
    }
    else if (IMX334MaxRegisterGain < regGain)
    {
        regGain = IMX334MaxRegisterGain;
    }

    return regGain;
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
    pExposureInfo->digitalRegisterGain  = 1; // TODO: Check for DGAIN CAL
    pExposureInfo->digitalRealGain      = 1;
    pExposureInfo->ISPDigitalGain       =
        pCalculateExposureData->realGain /(pExposureInfo->analogRealGain * pExposureInfo->digitalRealGain);
    pExposureInfo->lineCount            = pCalculateExposureData->lineCount;
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
    INT     shr_lef = 0;
    INT     shr_sef = 0;
    INT     rhs2 = 0;

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
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->frameLengthLines & 0xFF);
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->frameLengthLinesAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->frameLengthLines & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->frameLengthLinesAddr + 2;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->frameLengthLines & 0xF0000) >> 16;
    regCount++;
    if (FALSE == pExposureData->applyShortExposure)
    {
        pExposureData->lineCount = pExposureData->frameLengthLines - pExposureData->lineCount;

        pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr;
        pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->lineCount & 0xFF);
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr + 1;
        pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->lineCount & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = pExposureData->pRegInfo->coarseIntgTimeAddr + 2;
        pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->lineCount & 0xF0000) >> 16;
        regCount++;
    }
    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->analogRegisterGain & 0xFF);
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr+ 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->analogRegisterGain & 0xFF00) >> 8;
    regCount++;

    /* TODO: Need to check for DGAIN CALC
    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->GlobalDigitalGainAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->GlobalDigitalGainAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->digitalRegisterGain & 0xFF);
    regCount++;
    */
    if (TRUE == pExposureData->applyShortExposure)
    {
        // for long exp SHR 3058 and short exp SHR 305C
        shr_lef = 5000 - pExposureData->lineCount;
        shr_sef = 309 - pExposureData->shortLineCount;

        if (shr_sef < 9) {
            shr_sef = 9;
        }
        else if (shr_sef > 309) {
            shr_sef = 309;
        }

        if (shr_lef < 618) {
            shr_lef = 618;
        }
        else if (shr_lef > (INT)(pExposureData->frameLengthLines - 2)) {
            shr_lef = pExposureData->frameLengthLines - 2;
        }

        pRegSettingsInfo->regSetting[regCount].registerAddr = 0x305C;
        pRegSettingsInfo->regSetting[regCount].registerData = (shr_sef & 0xFF);
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = 0x305D;
        pRegSettingsInfo->regSetting[regCount].registerData = (shr_sef & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = 0x3058;
        pRegSettingsInfo->regSetting[regCount].registerData = (shr_lef & 0xFF);
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = 0x3059;
        pRegSettingsInfo->regSetting[regCount].registerData = (shr_lef & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = pExposureData->pRegInfo->shortGlobalAnalogGainAddr;
        pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->shortRegisterGain & 0xFF);
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = pExposureData->pRegInfo->shortGlobalAnalogGainAddr + 1;
        pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->shortRegisterGain & 0xFF00) >> 8;
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
