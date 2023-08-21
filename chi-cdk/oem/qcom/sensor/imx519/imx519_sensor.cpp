////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxsensordriverapi.h"
// NOWHINE ENTIRE FILE
#include <android/log.h>
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

#undef IMX519_DBG
#ifdef IMX519_DBG_ENABLE
#define IMX519_DBG(fmt, args,...) __android_log_print(ANDROID_LOG_INFO, "IMX519",fmt, ##args)
#else
#define IMX519_DBG(fmt, args,...)
#endif



static const UINT   IMX519MinRegisterGain        = 0;                                     ///< Minimum analog register gain
static const UINT   IMX519MaxRegisterGain        = 960;                                   ///< Maximum analog register gain
static const UINT   IMX519MinRegisterGainHFR480  = 299;                                   ///< Min analog register gain 480fps
static const UINT   IMX519MaxRegisterGainHFR480  = 896;                                   ///< Max analog register gain 480fps
static const DOUBLE IMX519MinRealGain            = (1024 / (1024 - IMX519MinRegisterGain)); ///< Minimum analog real gain (1X)
static const DOUBLE IMX519MaxRealGain            = (1024 / (1024 - IMX519MaxRegisterGain)); ///< Maximum analog real gain (16X)
static const DOUBLE IMX519MinRealGainHFR480      = (1024.0 / (1024.0 - IMX519MinRegisterGainHFR480));
                                                                                       ///< Min analog real gain 480fps (1.41X)
static const DOUBLE IMX519MaxRealGainHFR480      = (1024.0 / (1024.0 - IMX519MaxRegisterGainHFR480));
                                                                                        ///< Max analog real gain 480fps (8X)
static const UINT   IMX519MinDigitalRegisterGain = 256;                                    ///< Minimum digital register gain
static const UINT   IMX519MaxDigitalRegisterGain = 4096;                                   ///< Maximum digital register gain
static const FLOAT  IMX519DigitalGainDecimator   = 256.0f;                                 /// < Digital gain decimator factor
static const DOUBLE IMX519MinDigitalRealGain     = (IMX519MinDigitalRegisterGain / IMX519DigitalGainDecimator);
                                                                                          ///< Minimum digital real gain (1X)
static const DOUBLE IMX519MaxDigitalRealGain     = (IMX519MaxDigitalRegisterGain/ IMX519DigitalGainDecimator);
                                                                                          ///< Maximum digital real gain (16X)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToRealGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DOUBLE RegisterToRealGain(
    UINT registerGain)
{
    if(IMX519MaxRegisterGain < registerGain)
    {
        registerGain = IMX519MaxRegisterGain;
    }

    return  (1024.0 / (1024.0 - registerGain));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealToRegisterGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT RealToRegisterGain(
    DOUBLE realGain)
{
    if (IMX519MinRealGain > realGain)
    {
        realGain = IMX519MinRealGain;
    }
    else if (IMX519MaxRealGain < realGain)
    {
        realGain = IMX519MaxRealGain;
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
    float digitalRealGain = totalRealGain / sensorRealGain;

    if (IMX519MaxDigitalRealGain < digitalRealGain)
    {
        digitalRealGain = IMX519MaxDigitalRealGain;
    }
    else if (IMX519MinDigitalRealGain > digitalRealGain)
    {
        digitalRealGain = IMX519MinDigitalRealGain;
    }

    return static_cast<UINT>(digitalRealGain * IMX519DigitalGainDecimator);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// VerifyAnalogGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID VerifyAnalogGain(
    SensorExposureInfo*          pExposureInfo,
    SensorCalculateExposureData* pCalculateExposureData)
{
    pExposureInfo->lineCount = pCalculateExposureData->lineCount;
    switch (pCalculateExposureData->sensorResolutionIndex)
    {
        /* For 480fps HFR mode IMX519 only accepts real analog gains
        in the range of 3dB to 18dB. Please refer sensor specifications
        for more details
        */
        case 8:
            if (IMX519MaxRegisterGainHFR480 < pExposureInfo->analogRegisterGain)
            {
                pExposureInfo->analogRegisterGain   = IMX519MaxRegisterGainHFR480;
                pExposureInfo->analogRealGain       = IMX519MaxRealGainHFR480;
            }
            else if (IMX519MinRegisterGainHFR480 > pExposureInfo->analogRegisterGain)
            {
                pExposureInfo->analogRegisterGain   = IMX519MinRegisterGainHFR480;
                pExposureInfo->analogRealGain       = IMX519MinRealGainHFR480;
                pExposureInfo->lineCount            =
                    (pCalculateExposureData->lineCount) * (pCalculateExposureData->realGain) / IMX519MinRealGainHFR480;
            }
            else
            {
                pExposureInfo->analogRealGain = static_cast<FLOAT>(RegisterToRealGain(pExposureInfo->analogRegisterGain));
            }
            break;

        default:
            pExposureInfo->analogRealGain = static_cast<FLOAT>(RegisterToRealGain(pExposureInfo->analogRegisterGain));
            /*
            Remaining resolution modes do not have any restrictions.
            */
            break;
    }
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
    VerifyAnalogGain(pExposureInfo, pCalculateExposureData);
    pExposureInfo->digitalRegisterGain  = CalculateDigitalGain(pCalculateExposureData->realGain, pExposureInfo->analogRealGain);
    pExposureInfo->digitalRealGain      = static_cast<FLOAT>(pExposureInfo->digitalRegisterGain) / IMX519DigitalGainDecimator;
    pExposureInfo->ISPDigitalGain       =
        pCalculateExposureData->realGain /(pExposureInfo->analogRealGain * pExposureInfo->digitalRealGain);
    if (pExposureInfo->ISPDigitalGain < 1.0f)
    {
        pExposureInfo->ISPDigitalGain = 1.0f;
    }
    // Since isp channel gain block is common to both short and long, remove isp digital gain from short sensitivity.
    pCalculateExposureData->shortRealGain = pCalculateExposureData->shortRealGain / pExposureInfo->ISPDigitalGain;

    if (pCalculateExposureData->shortRealGain < 1.0f)
    {
        // Considering isp digital gain in exposuretime, since gain exhausted.
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
// NormalizeLineCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT NormalizeLineCount(
    UINT inputLineCount)
{
    UINT normalizedLineCount = inputLineCount;

    /*
    For resolutions with H:1/2 and V:1/2 binning w/o PDAF following restriction apply on linecount:
    If CIT <= 14, acceptable linecount follows 4N+2 rule. That implies N can only be {0, 1, 2, 3}.
    If CIT > 14 , acceptable linecount is only even numbered values.
    */

    if (inputLineCount > 14)
    {
        if (inputLineCount & 0x1)
        {
            ++normalizedLineCount;
        }
    }
    else if (inputLineCount > 10)
    {
        normalizedLineCount = 14;
    }
    else if (inputLineCount > 6)
    {
        normalizedLineCount = 10;
    }
    else if (inputLineCount > 2)
    {
        normalizedLineCount = 6;
    }
    else
    {
        normalizedLineCount = 2;
    }

    return normalizedLineCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetLineCount
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT GetLineCount(
    UINT    inputLineCount,
    UINT32  resolutionIndex)
{
    UINT lineCount = inputLineCount;

    switch (resolutionIndex)
    {
        /* For full res modes IMX519 only accepts odd line count
           when the CIT is less than or equal to 16. Please refer
           Software Reference Manual IMX519 page - 94 Table 5-24 for
           more details
        */
        case 0:
        case 4:
        case 9:
        case 10:
        case 12:
        {
            if ((inputLineCount <= 16) && !(inputLineCount & 0x1))
            {
                ++lineCount;
            }
        }
        break;
        /* For modes without PDAF and bin 2, linecount needs to be normalized based on
           certain rules described in NormalizeLineCount. Please refer
           Software Reference Manual IMX519 page - 94 Table 5-24 for
           more details
        */
        case 2:
        case 3:
        case 6:
        case 7:
        case 8:
        {
            lineCount = NormalizeLineCount(inputLineCount);
        }
        break;
        default:
            /*
            Remaining resolution modes do not have any restrictions.
            */
            break;
    }

    return lineCount;
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

    UINT lineCount = GetLineCount(pExposureData->lineCount, pExposureData->sensorResolutionIndex);
    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr;
    pRegSettingsInfo->regSetting[regCount].registerData  = (lineCount & 0xFF00) >> 8;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr + 1;
    pRegSettingsInfo->regSetting[regCount].registerData  = (lineCount & 0xFF);
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
