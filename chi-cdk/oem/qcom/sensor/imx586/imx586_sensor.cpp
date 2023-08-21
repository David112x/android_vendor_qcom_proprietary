////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxsensordriverapi.h"
#include "chistatsinterfacedefs.h"
// NOWHINE ENTIRE FILE
#include <android/log.h>
#include <math.h>

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

static const UINT   IMX586CITLShiftRegister      = 0x3100;                                ///< CIT left shift register
static const UINT   IMX586MaxLinecount           = 0xFFFF;                                ///< Maximum linecount
static const UINT   IMX586MaxCITLShift           = 7;                                     ///< Maximum CIT left shift value

static const UINT   IMX586MinRegisterGain        = 0;                                     ///< Minimum analog register gain
static const UINT   IMX586MaxRegisterGain        = 992;                                   ///< Maximum analog register gain
static const DOUBLE IMX586MinRealGain            = (1024 / (1024 - IMX586MinRegisterGain)); ///< Minimum analog real gain (1X)
static       DOUBLE IMX586MaxRealGain            = (1024 / (1024 - IMX586MaxRegisterGain)); ///< Maximum analog real gain (16X)
static const UINT   IMX586MinDigitalRegisterGain = 256;                                   ///< Minimum digital register gain
static const UINT   IMX586MaxDigitalRegisterGain = 256;                                   ///< Maximum digital register gain
static const UINT   IMX586DigitalGainDecimator   = 256;                                   /// < Digital gain decimator factor
static const DOUBLE IMX586MinDigitalRealGain     = (IMX586MinDigitalRegisterGain / IMX586DigitalGainDecimator);
                                                                                          ///< Minimum digital real gain (1X)
static const DOUBLE IMX586MaxDigitalRealGain     = (IMX586MaxDigitalRegisterGain/ IMX586DigitalGainDecimator);
                                                                                          ///< Maximum digital real gain (16X)
                                                                                          ///< Make Digtal gain to 1X to fix AECScan issues
static const UINT   IMX586HDRMaxDigitalRegisterGain = 4096;                               ///< HDR Maximum digital register gain
static const DOUBLE IMX586HDRMaxDigitalRealGain  = (IMX586HDRMaxDigitalRegisterGain / IMX586DigitalGainDecimator);
                                                                                          ///< HDR Maximum digital real gain(16X)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToRealGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DOUBLE RegisterToRealGain(
    UINT registerGain)
{
    if(IMX586MaxRegisterGain < registerGain)
    {
        registerGain = IMX586MaxRegisterGain;
    }

    return  (1024.0 / (1024.0 - registerGain));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealToRegisterGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT RealToRegisterGain(
    DOUBLE realGain)
{
    if (IMX586MinRealGain > realGain)
    {
        realGain = IMX586MinRealGain;
    }
    else if (IMX586MaxRealGain < realGain)
    {
        realGain = IMX586MaxRealGain;
    }

    return static_cast<UINT>(1024.0 - (1024.0 / realGain));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateDigitalGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT CalculateDigitalGain(
    FLOAT totalRealGain,
    FLOAT sensorRealGain,
    BOOL  HDREnabled)
{
    UINT32 digitalMSB = 0, digitalLSB = 0, digitalGain = 0;

    float digitalRealGain = IMX586MinDigitalRealGain;

    if (TRUE == HDREnabled)
    {
        digitalRealGain = totalRealGain / sensorRealGain;

        if (IMX586HDRMaxDigitalRealGain < digitalRealGain)
        {
            digitalRealGain = IMX586HDRMaxDigitalRealGain;
        }
    }
    else
    {
        if (IMX586MaxRealGain < totalRealGain)
        {
            digitalRealGain = totalRealGain / sensorRealGain;
        }
        else
        {
            digitalRealGain = IMX586MinDigitalRealGain;
        }

        if (IMX586MaxDigitalRealGain < digitalRealGain)
        {
            digitalRealGain = IMX586MaxDigitalRealGain;
        }
    }

    digitalMSB = static_cast<UINT32>(digitalRealGain);
    digitalLSB = static_cast<UINT32>(((digitalRealGain - digitalMSB)*IMX586DigitalGainDecimator));

    digitalGain = ((digitalMSB & 0x0F) << 8) + digitalLSB;

    return static_cast<UINT>(digitalGain);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateExposure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CalculateExposure(
    SensorExposureInfo* pExposureInfo,
    SensorCalculateExposureData* pCalculateExposureData)
{
    BOOL result = FALSE;

    if ((NULL != pExposureInfo) && (NULL != pCalculateExposureData))
    {
        FLOAT realGain            = pCalculateExposureData->realGain;
        UINT  lineCount           = pCalculateExposureData->lineCount;
        FLOAT shortRealGain       = pCalculateExposureData->shortRealGain;
        UINT  shortLinecount      = pCalculateExposureData->shortLinecount;
        FLOAT middleRealGain      = pCalculateExposureData->middleRealGain;
        UINT  middleLinecount     = pCalculateExposureData->middleLinecount;
        FLOAT digitalRealGainInt  = 0.0f;
        FLOAT digitalRealGainFrac = 0.0f;

        if ((middleRealGain == 0.0) || (middleLinecount == 0))
        {
            pExposureInfo->analogRegisterGain   = RealToRegisterGain(realGain);
            pExposureInfo->analogRealGain       = static_cast<FLOAT>(RegisterToRealGain(pExposureInfo->analogRegisterGain));
            pExposureInfo->digitalRegisterGain  = CalculateDigitalGain(realGain, pExposureInfo->analogRealGain, FALSE);
            digitalRealGainInt                  = static_cast<FLOAT>((pExposureInfo->digitalRegisterGain >> 8) & 0x0F);
            digitalRealGainFrac                 = static_cast<FLOAT>(pExposureInfo->digitalRegisterGain & 0xFF) /
                                                                                     IMX586DigitalGainDecimator;
            pExposureInfo->digitalRealGain      = digitalRealGainInt + digitalRealGainFrac;
            pExposureInfo->ISPDigitalGain       = realGain /(pExposureInfo->analogRealGain * pExposureInfo->digitalRealGain);
            pExposureInfo->lineCount            = lineCount;

            shortRealGain = shortRealGain / pExposureInfo->ISPDigitalGain;

            if (shortRealGain < 1.0f)
            {
                shortLinecount = static_cast<UINT>(shortRealGain * shortLinecount);
                shortLinecount = shortLinecount < 1 ? 1 : shortLinecount;
                shortRealGain = 1.0f;
            }
            pExposureInfo->shortRegisterGain    = RealToRegisterGain(shortRealGain);
            pExposureInfo->shortLinecount       = shortLinecount;
        }
        else
        {
            pExposureInfo->analogRegisterGain         = RealToRegisterGain(realGain);
            pExposureInfo->analogRealGain             = static_cast<FLOAT>(RegisterToRealGain(pExposureInfo->analogRegisterGain));
            pExposureInfo->digitalRegisterGain        = CalculateDigitalGain(realGain, pExposureInfo->analogRealGain, TRUE);
            digitalRealGainInt                        = static_cast<FLOAT>((pExposureInfo->digitalRegisterGain >> 8) & 0x0F);
            digitalRealGainFrac                       = static_cast<FLOAT>(pExposureInfo->digitalRegisterGain & 0xFF) /
                                                                                                 IMX586DigitalGainDecimator;
            pExposureInfo->digitalRealGain            = digitalRealGainInt + digitalRealGainFrac;;
            pExposureInfo->lineCount                  = lineCount;

            pExposureInfo->shortRegisterGain          = RealToRegisterGain(shortRealGain);
            pExposureInfo->shortAnalogRealGain        = static_cast<FLOAT>(RegisterToRealGain(pExposureInfo->shortRegisterGain));
            pExposureInfo->shortDigitalRegisterGain   = CalculateDigitalGain(shortRealGain, pExposureInfo->shortAnalogRealGain, TRUE);
            digitalRealGainInt                        = static_cast<FLOAT>((pExposureInfo->shortDigitalRegisterGain >> 8) & 0x0F);
            digitalRealGainFrac                       = static_cast<FLOAT>(pExposureInfo->shortDigitalRegisterGain & 0xFF) /
                                                                                                 IMX586DigitalGainDecimator;
            pExposureInfo->shortDigitalRealGain       = digitalRealGainInt + digitalRealGainFrac;;
            pExposureInfo->shortLinecount             = shortLinecount;

            pExposureInfo->middleRegisterGain         = RealToRegisterGain(middleRealGain);
            pExposureInfo->middleAnalogRealGain       = static_cast<FLOAT>(RegisterToRealGain(pExposureInfo->middleRegisterGain));
            pExposureInfo->middleDigitalRegisterGain  = CalculateDigitalGain(middleRealGain, pExposureInfo->middleAnalogRealGain, TRUE);
            digitalRealGainInt                        = static_cast<FLOAT>((pExposureInfo->middleDigitalRegisterGain >> 8) & 0x0F);
            digitalRealGainFrac                       = static_cast<FLOAT>(pExposureInfo->middleDigitalRegisterGain & 0xFF) /
                                                                                                 IMX586DigitalGainDecimator;
            pExposureInfo->middleDigitalRealGain      = digitalRealGainInt + digitalRealGainFrac;;
            pExposureInfo->middleLinecount            = middleLinecount;

            pExposureInfo->ISPDigitalGain             = 1.0f;
        }
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
    BOOL   result    = FALSE;
    UINT32 index     = 0;
    UINT16 regCount  = 0;
    UINT   lineCount = 0;
    UINT   citLshift = 0;

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

        for (; lineCount > IMX586MaxLinecount;)
        {
            lineCount = pExposureData->lineCount / static_cast<UINT>(pow(2, ++citLshift));
        }

        if ((lineCount > IMX586MaxLinecount) || (citLshift > IMX586MaxCITLShift))
        {
            citLshift = IMX586MaxCITLShift;
            lineCount = IMX586MaxLinecount;
        }

        pRegSettingsInfo->regSetting[regCount].registerAddr  = IMX586CITLShiftRegister;
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

            pRegSettingsInfo->regSetting[regCount].registerAddr = 0x218;
            pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->shortDigitalRegisterGain & 0xFF00) >> 8;
            regCount++;

            if (TRUE == pExposureData->applyMiddleExposure)
            {
                pRegSettingsInfo->regSetting[regCount].registerAddr = 0x218 + 1;
                pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->shortDigitalRegisterGain & 0xFF);
                regCount++;

                pRegSettingsInfo->regSetting[regCount].registerAddr = 0x3FE0;
                pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->middleLineCount & 0xFF00) >> 8;
                regCount++;

                pRegSettingsInfo->regSetting[regCount].registerAddr = 0x3FE0 + 1;
                pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->middleLineCount & 0xFF);
                regCount++;

                pRegSettingsInfo->regSetting[regCount].registerAddr = 0x3FE2;
                pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->middleRegisterGain & 0xFF00) >> 8;
                regCount++;

                pRegSettingsInfo->regSetting[regCount].registerAddr = 0x3FE2 + 1;
                pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->middleRegisterGain & 0xFF);
                regCount++;

                pRegSettingsInfo->regSetting[regCount].registerAddr = 0x3FE4;
                pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->middleDigitalRegisterGain & 0xFF00) >> 8;
                regCount++;

                pRegSettingsInfo->regSetting[regCount].registerAddr = 0x3FE4 + 1;
                pRegSettingsInfo->regSetting[regCount].registerData = (pExposureData->middleDigitalRegisterGain & 0xFF);
                regCount++;
           }
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
// StatsParse
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsParse(
    UINT8*             pInputData,
    VOID*              pOutputData,
    UINT               reserved1,
    UINT               reserved2)
{
    (void)reserved1;
    (void)reserved2;
    StatsHDR3ExposureDataType* pDataParse = reinterpret_cast<StatsHDR3ExposureDataType *>(pOutputData);
    UINT8* pInput = pInputData;

    pDataParse->splitHistchannelCount = 1;
    for (UINT32 channelCount = 0; channelCount < pDataParse->splitHistchannelCount; channelCount++)
    {
        for (UINT32 index = 0; index < 128; index++)
        {
            pDataParse->splitBayerHist.pHistData[channelCount][index] =
                (pInput[index * 5] << 16) + (pInput[index * 5 + 1] << 8) + (pInput[index * 5 + 2]);
        }
        for (UINT32 index = 0; index < 8; index++)
        {
            pDataParse->splitBayerHist.pHistData[channelCount][126 - index]
                = pDataParse->splitBayerHist.pHistData[channelCount][126 - index] +
                  pDataParse->splitBayerHist.pHistData[channelCount][127 - index];
            pDataParse->splitBayerHist.pHistData[channelCount][127 - index] = 0;
        }
        pDataParse->splitBayerHist.histDataType[channelCount] = StatsColorChannelY;
        pDataParse->splitBayerHist.binCount                   = 128;
        pDataParse->splitBayerHist.totalHistCount             = 12000000;
        pDataParse->splitBayerHist.histWeightUnit             = 1;
        pDataParse->splitBayerHist.measurementRegion          = MeasurementRegionType::GlobalRegion;
        pDataParse->splitBayerHist.mergedType                 = HDR3ExposureMergedType::HDR3ExpSplitLong;
    }

    pDataParse->mergedHistchannelCount = 1;
    pInput                             = pInputData + 640;
    for (UINT32 channelCount = 0; channelCount < pDataParse->mergedHistchannelCount; channelCount++)
    {
        for (UINT32 index = 0; index < 16; index++)
        {
            pDataParse->mergedBayerHist.pHistData[channelCount][index] =
                (pInput[index * 5] << 16) + (pInput[index * 5 + 1] << 8) + (pInput[index * 5 + 2]);
        }
        pDataParse->mergedBayerHist.histDataType[channelCount] = StatsColorChannelY;
        pDataParse->mergedBayerHist.binCount                   = 16;
        pDataParse->mergedBayerHist.totalHistCount             = 12000000;
        pDataParse->mergedBayerHist.histWeightUnit             = 1;
        pDataParse->mergedBayerHist.measurementRegion          = MeasurementRegionType::GlobalRegion;
        pDataParse->mergedBayerHist.mergedType                 = HDR3ExposureMergedType::HDR3ExpMerged;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FillAutoWhiteBalanceSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FillAutoWhiteBalanceSettings(
    RegSettingsInfo*    pRegSettingsInfo,
    FLOAT               AWBGainRed,
    FLOAT               AWBGainGreen,
    FLOAT               AWBGainBlue)
{
    BOOL   result     = FALSE;
    UINT16 regCount   = 0;
    UINT32 rGain      = static_cast<UINT32>(AWBGainRed   * 256);
    UINT32 gGain      = static_cast<UINT32>(AWBGainGreen * 256);
    UINT32 bGain      = static_cast<UINT32>(AWBGainBlue  * 256);
    UINT   wbGainAddr = 0x0B8E;

    if (NULL != pRegSettingsInfo)
    {
        pRegSettingsInfo->regSetting[regCount].registerData = (gGain & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerData = gGain & 0x00FF;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerData = (rGain & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerData = rGain & 0x00FF;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerData = (bGain & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerData = bGain & 0xFF;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerData = (gGain & 0xFF00) >> 8;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerData = gGain & 0x00FF;
        regCount++;

        pRegSettingsInfo->regSettingCount = regCount;

        for (UINT32 index = 0; index < regCount; index++)
        {
            pRegSettingsInfo->regSetting[index].registerAddr = wbGainAddr++;
            pRegSettingsInfo->regSetting[index].regAddrType  = I2CRegAddressDataTypeWord;
            pRegSettingsInfo->regSetting[index].regDataType  = I2CRegAddressDataTypeByte;
            pRegSettingsInfo->regSetting[index].delayUs      = 0;
        }

        result = TRUE;
    }

    if ((TRUE == result) && (MAX_REG_SETTINGS <= regCount))
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UpdateMaxAnalogGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID UpdateMaxAnalogGain(
    DOUBLE  inputMaxAnalogGain)
{
    IMX586MaxRealGain = inputMaxAnalogGain;
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
    pSensorLibraryAPI->pStatsParse                   = StatsParse;
    pSensorLibraryAPI->pFillAutoWhiteBalanceSettings = FillAutoWhiteBalanceSettings;
    pSensorLibraryAPI->pUpdateMaxAnalogGain          = UpdateMaxAnalogGain;
}

#ifdef __cplusplus
} // CamX Namespace
#endif // __cplusplus
