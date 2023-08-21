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

static const UINT   IMX481CITLShiftRegister      = 0x3100;                                  ///< CIT left shift register
static const UINT   IMX481MaxLinecount           = 0xFFFF;                                  ///< Maximum linecount
static const UINT   IMX481MaxCITLShift           = 7;                                       ///< Maximum CIT left shift value

static const UINT   IMX481MinRegisterGain        = 0;                                       ///< Minimum analog register gain
static const UINT   IMX481MaxRegisterGain        = 960;                                     ///< Maximum analog register gain
static const DOUBLE IMX481MinRealGain            = (1024 / (1024 - IMX481MinRegisterGain)); ///< Minimum analog real gain (1X)
static const DOUBLE IMX481MaxRealGain            = (1024 / (1024 - IMX481MaxRegisterGain)); ///< Maximum analog real gain (16X)
static const UINT   IMX481MinDigitalRegisterGain = 256;                                     ///< Minimum digital register gain
static const UINT   IMX481MaxDigitalRegisterGain = 256;                                     ///< Maximum digital register gain
static const UINT   IMX481DigitalGainDecimator   = 256;                                     ///< Digital gain decimator factor
static const DOUBLE IMX481MinDigitalRealGain     = (IMX481MinDigitalRegisterGain / IMX481DigitalGainDecimator);
                                                                                            ///< Minimum digital real gain (1X)
static const DOUBLE IMX481MaxDigitalRealGain     = (IMX481MaxDigitalRegisterGain/ IMX481DigitalGainDecimator);
                                                                                            ///< Maximum digital real gain (16X)
static const UINT   FlexArea0StartAddress        = 0x38b4;                                  ///< Flex area start address

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToRealGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DOUBLE RegisterToRealGain(
    UINT registerGain)
{
    if(IMX481MaxRegisterGain < registerGain)
    {
        registerGain = IMX481MaxRegisterGain;
    }

    return  (1024.0 / (1024.0 - registerGain));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealToRegisterGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT RealToRegisterGain(
    DOUBLE realGain)
{
    if (IMX481MinRealGain > realGain)
    {
        realGain = IMX481MinRealGain;
    }
    else if (IMX481MaxRealGain < realGain)
    {
        realGain = IMX481MaxRealGain;
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

    float digitalRealGain = IMX481MinDigitalRealGain;

    if (IMX481MaxRealGain < totalRealGain)
    {
        digitalRealGain = totalRealGain / sensorRealGain;
    }
    else
    {
        digitalRealGain = IMX481MinDigitalRealGain;
    }

    if (IMX481MaxDigitalRealGain < digitalRealGain)
    {
        digitalRealGain = IMX481MaxDigitalRealGain;
    }

    digitalMSB = static_cast<UINT32>(digitalRealGain);
    digitalLSB = static_cast<UINT32>(((digitalRealGain - digitalMSB) * IMX481DigitalGainDecimator));

    digitalGain = ((digitalMSB & 0x0F) << 8) + digitalLSB;

    return static_cast<UINT>(digitalGain);
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
        pExposureInfo->digitalRegisterGain  = CalculateDigitalGain(pCalculateExposureData->realGain, pExposureInfo->analogRealGain);
        FLOAT digitalRealGainInt            = static_cast<FLOAT>((pExposureInfo->digitalRegisterGain >> 8) & 0x0F);
        FLOAT digitalRealGainFrac           = static_cast<FLOAT>((pExposureInfo->digitalRegisterGain & 0xFF) /
                                                                  IMX481DigitalGainDecimator);
        pExposureInfo->digitalRealGain      = digitalRealGainInt + digitalRealGainFrac;
        pExposureInfo->ISPDigitalGain       = pCalculateExposureData->realGain / (pExposureInfo->analogRealGain *
                                              pExposureInfo->digitalRealGain);
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
    UINT32  index     = 0;
    UINT16  regCount  = 0;
    BOOL    result    = FALSE;
    UINT    lineCount = 0;
    UINT    citLshift = 0;

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

        lineCount = pExposureData->lineCount;

        for (; lineCount > IMX481MaxLinecount;)
        {
            lineCount = pExposureData->lineCount / static_cast<UINT>(pow(2, ++citLshift));
        }

        if ((lineCount > IMX481MaxLinecount) || (citLshift > IMX481MaxCITLShift))
        {
            citLshift = IMX481MaxCITLShift;
            lineCount = IMX481MaxLinecount;
        }

        pRegSettingsInfo->regSetting[regCount].registerAddr  = IMX481CITLShiftRegister;
        pRegSettingsInfo->regSetting[regCount].registerData  = citLshift;
        regCount++;

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
        result = TRUE;
    }

    if ((TRUE == result) && (MAX_REG_SETTINGS <= regCount))
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FillPDAFSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FillPDAFSettings(
        RegSettingsInfo*        pRegSettingsInfo,
        SensorFillPDAFData*     pPDAFData)
{
    UINT16  regCount     = 0;
    UINT16  winNumber    = 0;
    UINT    flexAreaADDR = 0;
    BOOL    result       = FALSE;

    if ((NULL == pRegSettingsInfo) || (NULL == pPDAFData))
    {
        return result;
    }

    winNumber    = static_cast<UINT16>(pPDAFData->horizontalWindowCount * pPDAFData->verticalWindowCount);
    flexAreaADDR = FlexArea0StartAddress;

    if (winNumber == 1)
    {
        pRegSettingsInfo->regSetting[regCount].registerAddr = flexAreaADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = ((pPDAFData->PDAFstartX) & 0xFF00)>> 8;
        pRegSettingsInfo->regSetting[regCount].regAddrType  = I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[regCount].regDataType  = I2CRegAddressDataTypeByte;
        pRegSettingsInfo->regSetting[regCount].delayUs      = 0;
        regCount++;
        flexAreaADDR++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = flexAreaADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = (pPDAFData->PDAFstartX) & 0xFF;
        pRegSettingsInfo->regSetting[regCount].regAddrType  = I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[regCount].regDataType  = I2CRegAddressDataTypeByte;
        pRegSettingsInfo->regSetting[regCount].delayUs      = 0;
        regCount++;
        flexAreaADDR++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = flexAreaADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = (pPDAFData->PDAFstartY & 0xFF00)>> 8;
        pRegSettingsInfo->regSetting[regCount].regAddrType  = I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[regCount].regDataType  = I2CRegAddressDataTypeByte;
        pRegSettingsInfo->regSetting[regCount].delayUs      = 0;
        regCount++;
        flexAreaADDR++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = flexAreaADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = pPDAFData->PDAFstartY & 0xFF;
        pRegSettingsInfo->regSetting[regCount].regAddrType  = I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[regCount].regDataType  = I2CRegAddressDataTypeByte;
        pRegSettingsInfo->regSetting[regCount].delayUs      = 0;
        regCount++;
        flexAreaADDR++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = flexAreaADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = (pPDAFData->PDAFendX & 0xFF00)>> 8;
        pRegSettingsInfo->regSetting[regCount].regAddrType  = I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[regCount].regDataType  = I2CRegAddressDataTypeByte;
        pRegSettingsInfo->regSetting[regCount].delayUs      = 0;
        regCount++;
        flexAreaADDR++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = flexAreaADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = pPDAFData->PDAFendX & 0xFF;
        pRegSettingsInfo->regSetting[regCount].regAddrType  = I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[regCount].regDataType  = I2CRegAddressDataTypeByte;
        pRegSettingsInfo->regSetting[regCount].delayUs      = 0;
        regCount++;
        flexAreaADDR++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = flexAreaADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = (pPDAFData->PDAFendY & 0xFF00)>> 8;
        pRegSettingsInfo->regSetting[regCount].regAddrType  = I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[regCount].regDataType  = I2CRegAddressDataTypeByte;
        pRegSettingsInfo->regSetting[regCount].delayUs      = 0;
        regCount++;
        flexAreaADDR++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = flexAreaADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = pPDAFData->PDAFendY & 0xFF;
        pRegSettingsInfo->regSetting[regCount].regAddrType  = I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[regCount].regDataType  = I2CRegAddressDataTypeByte;
        pRegSettingsInfo->regSetting[regCount].delayUs      = 0;
        regCount++;
        flexAreaADDR++;

        result = TRUE;
    }

    pRegSettingsInfo->regSettingCount = regCount;

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
    pSensorLibraryAPI->pFillPDAFSettings     = FillPDAFSettings;
}

#ifdef __cplusplus
} // CamX Namespace
#endif // __cplusplus
