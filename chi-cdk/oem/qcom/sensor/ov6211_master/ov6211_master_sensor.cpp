////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxsensordriverapi.h"
// NOWHINE ENTIRE FILE

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// the following values are taken from the sensor XML file for quarter resolution.
// TODO: check these numbers, they look wrong, full res is 1752@30fps
const UINT32 FrameRate            = 30;
const UINT32 FrameLengthLines     = 1904;

// the following value was measured from /STROBE going low to 100% intensity of the LED
// TODO: check these numbers, was this measured on Trinity??
const UINT32 LEDRampTimeNs        = 900000; // 0.9ms

const UINT32 NanoSecondsPerSecond = 1000000000;
const UINT32 LineTimeNs           = (((NanoSecondsPerSecond / FrameRate)) / FrameLengthLines);
const UINT32 ExtraLines           = (LEDRampTimeNs / LineTimeNs);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToRealGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DOUBLE RegisterToRealGain(
    UINT registerGain)
{
    DOUBLE real_gain;
    real_gain = (DOUBLE) (((DOUBLE)(registerGain))/16.0);
    return real_gain;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealToRegisterGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT RealToRegisterGain(
    DOUBLE realGain)
{
    UINT reg_gain = 0;
    realGain = realGain*16.0;
    reg_gain = (UINT)realGain;
    return reg_gain;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateDigitalGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT CalculateDigitalGain(
    FLOAT totalRealGain,
    FLOAT sensorRealGain)
{
    (void) totalRealGain;
    (void) sensorRealGain;
    return 1;
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
    pExposureInfo->digitalRealGain      = 1.0;
    pExposureInfo->ISPDigitalGain       =
        pCalculateExposureData->realGain /(pExposureInfo->analogRealGain * pExposureInfo->digitalRealGain);
    pExposureInfo->lineCount            = pCalculateExposureData->lineCount;
    pExposureInfo->shortRegisterGain    = RealToRegisterGain(pCalculateExposureData->shortRealGain);
    pExposureInfo->shortLinecount       = pCalculateExposureData->shortLinecount;
    //ALOGE("CalculateExposure(), ov6211 lineCount:%d realGain:%f\n", lineCount, realGain);

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
    INT32   offset    = 0;
    UINT16  regCount  = 0;

    if ((NULL == pRegSettingsInfo) || (NULL == pExposureData))
    {
        return FALSE;
    }

    // strobeStart = frameLengthLines (aka VTS) - exposure - 9 - strobe_offset (aka Tnegative)
    UINT32 Tnegative   = ExtraLines;
    UINT32 strobeStart = pExposureData->frameLengthLines - pExposureData->lineCount - 9 - Tnegative;
    UINT32 strobeSpan  = pExposureData->lineCount + Tnegative;

    // group hold on (start of register settings group)
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
        pRegSettingsInfo->regSetting[regCount].operation=IOOperationTypeWrite;
        regCount++;
    }
    offset = pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount;

    // configure exposure settings
    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->frameLengthLinesAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->frameLengthLines & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->frameLengthLinesAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->frameLengthLines & 0xFF);
    regCount++;

    // exposure in lines: max-value =  min(frame-length-in-lines - vertical-offset, 434)
    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr - 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = ((pExposureData->lineCount >> 12) & 0x0F);
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = ((pExposureData->lineCount >> 4) & 0xFF);
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = ((pExposureData->lineCount << 4) & 0xF0);
    regCount++;

    // gain: 1.0 to ~6000 where 1.0 = 0x10
    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->analogRegisterGain & 0x300) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr+1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (pExposureData->analogRegisterGain & 0xFF);
    regCount++;

    // strobe-frame-span in line-counts: bits [15:8]
    pRegSettingsInfo->regSetting[regCount].registerAddr  = 0x3b8e;
    pRegSettingsInfo->regSetting[regCount].registerData  = ((strobeSpan >> 8)  & 0xFF);
    regCount++;

    // strobe-frame-span in line-counts: bits [7:0]
    pRegSettingsInfo->regSetting[regCount].registerAddr  = 0x3b8f;
    pRegSettingsInfo->regSetting[regCount].registerData  = ((strobeSpan)       & 0xFF);
    regCount++;

    // strobe-start-point in line-counts: bits [15:8]
    pRegSettingsInfo->regSetting[regCount].registerAddr  = 0x3b90;
    pRegSettingsInfo->regSetting[regCount].registerData  = ((strobeStart >> 8) & 0xFF);
    regCount++;

    // strobe-start-point in line-counts: bits [7:0]
    pRegSettingsInfo->regSetting[regCount].registerAddr  = 0x3b91;
    pRegSettingsInfo->regSetting[regCount].registerData  = ((strobeStart)      & 0xFF);
    regCount++;

    for (index = offset; index < regCount; index++)
    {
        pRegSettingsInfo->regSetting[index].regAddrType  = I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[index].regDataType  = I2CRegAddressDataTypeByte;
        pRegSettingsInfo->regSetting[index].delayUs      = 0;
        pRegSettingsInfo->regSetting[index].operation    = IOOperationTypeWrite;
    }

    // group hold off (end of register settings group)
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
        pRegSettingsInfo->regSetting[regCount].delayUs      =
            pExposureData->pRegInfo->groupHoldOffSettings.regSetting[index].delayUs;
        pRegSettingsInfo->regSetting[regCount].operation=IOOperationTypeWrite;
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
    pSensorLibraryAPI->majorVersion          = 2;
    pSensorLibraryAPI->minorVersion          = 0;
    pSensorLibraryAPI->pCalculateExposure    = CalculateExposure;
    pSensorLibraryAPI->pFillExposureSettings = FillExposureSettings;
}

#ifdef __cplusplus
} // CamX Namespace
#endif // __cplusplus
