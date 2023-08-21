////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <cutils/properties.h>
#include <stdlib.h>

#include "camxsensordriverapi.h"
// NOWHINE ENTIRE FILE

#include <string.h>
#include "chistatsinterfacedefs.h"
#include "s5k2x5sp_stats_parser.h"
#include "s5k2x5sp_dbglog.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

// sensor support short again only.
// long
#define LONG_COARSE_INT_ADDR   0x0226
#define LONG_DGAIN_ADDR        0x0230 // gr

// medium
#define MEDIUM_COARSE_INT_ADDR 0x022C
#define MEDIUM_DGAIN_ADDR      0x0238 // gr

// short
#define SHORT_COARSE_INT_ADDR  0x0202
#define SHORT_AGAIN_ADDR       0x0204
#define SHORT_DGAIN_ADDR       0x020E // gr

// need to check
static const UINT   S5K2X5MinRegisterGain        = 0x0020;  ///< Minimum analog register gain
static const UINT   S5K2X5MaxRegisterGain        = 0x0200;  ///< Maximum analog register gain
static const DOUBLE S5K2X5MinRealGain            = 1.0;     ///< Minimum analog real gain (1X)
static const DOUBLE S5K2X5MaxRealGain            = 16.0;    ///< Maximum analog real gain (16X)
static const UINT   S5K2X5MinDigitalRegisterGain = 0x0100;  ///< Minimum digital register gain
static const UINT   S5K2X5MaxDigitalRegisterGain = 0x1000;  ///< Maximum digital register gain
static const UINT   S5K2X5DigitalGainDecimator   = 0x0100;  ///< Digital gain decimator factor
static const DOUBLE S5K2X5MinDigitalRealGain     = 1.0;     ///< Minimum digital real gain (1X)
static const DOUBLE S5K2X5MaxDigitalRealGain     = 1.0;     ///< Maximum digital real gain (1X)
static const DOUBLE S5K2X5HDRMinDigitalRealGain  = 1.0;     ///< HDR mode minimum digital real gain (1X)
static const DOUBLE S5K2X5HDRMaxDigitalRealGain  = 16.0;    ///< HDR mode maximum digital real gain (16X)


/* @high-level stat data after parsing */
st_ae_stat_t ae_stat;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RegisterToRealGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static DOUBLE RegisterToRealGain(
    UINT registerGain)
{
    if(S5K2X5MaxRegisterGain < registerGain)
    {
        registerGain = S5K2X5MaxRegisterGain;
    }

    return  registerGain / 32.0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RealToRegisterGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT RealToRegisterGain(
    DOUBLE realGain)
{
    if (S5K2X5MinRealGain > realGain)
    {
        realGain = S5K2X5MinRealGain;
    }
    else if (S5K2X5MaxRealGain < realGain)
    {
        realGain = S5K2X5MaxRealGain;
    }

    return static_cast<UINT>(realGain * 32.0);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CalculateDigitalGain
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT CalculateDigitalGain(
    FLOAT totalRealGain,
    FLOAT sensorRealGain,
    BOOL  HDREnabled)

{
    float digitalRealGain = S5K2X5MinDigitalRealGain;

    if (TRUE == HDREnabled)
    {
        digitalRealGain = totalRealGain / sensorRealGain;

        if (S5K2X5HDRMaxDigitalRealGain < digitalRealGain)
        {
            digitalRealGain = S5K2X5HDRMaxDigitalRealGain;
        }
    }
    else
    {
        if (S5K2X5MaxRealGain < totalRealGain)
        {
            digitalRealGain = totalRealGain / sensorRealGain;
        }
        else
        {
            digitalRealGain = S5K2X5MinDigitalRealGain;
        }

        if (S5K2X5MaxDigitalRealGain < digitalRealGain)
        {
            digitalRealGain = S5K2X5MaxDigitalRealGain;
        }
    }

    return static_cast<UINT>(digitalRealGain * S5K2X5DigitalGainDecimator);
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

    if ((pCalculateExposureData->middleRealGain == 0.0) || (pCalculateExposureData->middleLinecount == 0))
    {
        pExposureInfo->analogRegisterGain   = RealToRegisterGain(pCalculateExposureData->realGain);
        pExposureInfo->analogRealGain       = static_cast<FLOAT>(RegisterToRealGain(pExposureInfo->analogRegisterGain));
        pExposureInfo->digitalRegisterGain  =
            CalculateDigitalGain(pCalculateExposureData->realGain, pExposureInfo->analogRealGain, FALSE);
        pExposureInfo->digitalRealGain      = static_cast<FLOAT>(pExposureInfo->digitalRegisterGain / S5K2X5DigitalGainDecimator);
        pExposureInfo->ISPDigitalGain       =
            pCalculateExposureData->realGain /(pExposureInfo->analogRealGain * pExposureInfo->digitalRealGain);
        pExposureInfo->lineCount            = pCalculateExposureData->lineCount;
    }
    else
    {
        // Change from AGain to DGain Control.
        // AGain = Short Gain until under x16
        // Long Dgain = Long Gain / AGain
        // Middle Dgain = Middle Gain / AGain
        // Short Dgain = Short Gain / AGain = 1

        // Short
        // This will be common AGain.
        pExposureInfo->shortRegisterGain         = RealToRegisterGain(pCalculateExposureData->shortRealGain);
        pExposureInfo->shortAnalogRealGain       = static_cast<FLOAT>(RegisterToRealGain(pExposureInfo->shortRegisterGain));
        pExposureInfo->shortDigitalRegisterGain  =
            CalculateDigitalGain(pCalculateExposureData->shortRealGain, pExposureInfo->shortAnalogRealGain, TRUE);
        pExposureInfo->shortDigitalRealGain      =
            static_cast<FLOAT>(pExposureInfo->shortDigitalRegisterGain / S5K2X5DigitalGainDecimator);
        pExposureInfo->shortLinecount            = pCalculateExposureData->shortLinecount;

        // Middle
        pExposureInfo->middleRegisterGain         = pExposureInfo->shortRegisterGain;
        pExposureInfo->middleAnalogRealGain       = pExposureInfo->shortAnalogRealGain;
        pExposureInfo->middleDigitalRegisterGain  =
            CalculateDigitalGain(pCalculateExposureData->middleRealGain, pExposureInfo->middleAnalogRealGain, TRUE);
        pExposureInfo->middleDigitalRealGain      =
            static_cast<FLOAT>(pExposureInfo->middleDigitalRegisterGain / S5K2X5DigitalGainDecimator);
        pExposureInfo->middleLinecount            = pCalculateExposureData->middleLinecount;

        // Long
        pExposureInfo->analogRegisterGain         = pExposureInfo->shortRegisterGain;
        pExposureInfo->analogRealGain             = pExposureInfo->shortAnalogRealGain;
        pExposureInfo->digitalRegisterGain        =
            CalculateDigitalGain(pCalculateExposureData->realGain, pExposureInfo->analogRealGain, TRUE);
        pExposureInfo->digitalRealGain            =
            static_cast<FLOAT>(pExposureInfo->digitalRegisterGain / S5K2X5DigitalGainDecimator);
        pExposureInfo->lineCount                  = pCalculateExposureData->lineCount;

        pExposureInfo->ISPDigitalGain             = 1.0f;
    }

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

    // minimum coarse integration time
    if (pExposureData->lineCount < 4) {
        pExposureData->lineCount = 4;
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
    pRegSettingsInfo->regSetting[regCount].registerData  = pExposureData->frameLengthLines;
    regCount++;

    if (TRUE == pExposureData->applyShortExposure && TRUE == pExposureData->applyMiddleExposure)
    {
        // short
        pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr;
        pRegSettingsInfo->regSetting[regCount].registerData  = pExposureData->shortLineCount;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr;
        pRegSettingsInfo->regSetting[regCount].registerData  = pExposureData->shortRegisterGain;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->GlobalDigitalGainAddr;
        pRegSettingsInfo->regSetting[regCount].registerData  = pExposureData->shortDigitalRegisterGain;
        regCount++;

        // middle
        pRegSettingsInfo->regSetting[regCount].registerAddr = MEDIUM_COARSE_INT_ADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = pExposureData->middleLineCount;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = MEDIUM_DGAIN_ADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = pExposureData->middleDigitalRegisterGain;
        regCount++;

        // long
        pRegSettingsInfo->regSetting[regCount].registerAddr = LONG_COARSE_INT_ADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = pExposureData->lineCount;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr = LONG_DGAIN_ADDR;
        pRegSettingsInfo->regSetting[regCount].registerData = pExposureData->digitalRegisterGain;
        regCount++;
    }
    else
    {
        // linecount need divide by 2 for 2x5 sensor binning mode
        if(pExposureData->sensitivityCorrectionFactor == 4)
        {
            pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr;
            pRegSettingsInfo->regSetting[regCount].registerData  = pExposureData->lineCount / 2;
            regCount++;
        }
        else
        {
            pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->coarseIntgTimeAddr;
            pRegSettingsInfo->regSetting[regCount].registerData  = pExposureData->lineCount;
            regCount++;
        }

        pRegSettingsInfo->regSetting[regCount].registerAddr  = pExposureData->pRegInfo->globalAnalogGainAddr;
        pRegSettingsInfo->regSetting[regCount].registerData  = pExposureData->analogRegisterGain;
        regCount++;

        pRegSettingsInfo->regSetting[regCount].registerAddr  = SHORT_DGAIN_ADDR;
        pRegSettingsInfo->regSetting[regCount].registerData  = pExposureData->digitalRegisterGain;
        regCount++;
    }

    for (index = 0; (pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index) < regCount; index++)
    {
        pRegSettingsInfo->regSetting[pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index].regAddrType  =
            I2CRegAddressDataTypeWord;
        pRegSettingsInfo->regSetting[pExposureData->pRegInfo->groupHoldOnSettings.regSettingCount + index].regDataType  =
            I2CRegAddressDataTypeWord;
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


BOOL FillAutoWhiteBalanceSettings(
    RegSettingsInfo*    pRegSettingsInfo,
    FLOAT               AWBGainRed,
    FLOAT               AWBGainGreen,
    FLOAT               AWBGainBlue)
{
    UINT32  index     = 0;
    UINT16  regCount  = 0;
    UINT32  rGain = static_cast<UINT32>(AWBGainRed * 256);
    UINT32  gGain = static_cast<UINT32>(AWBGainGreen * 256);
    UINT32  bGain = static_cast<UINT32>(AWBGainBlue * 256);

    if (NULL == pRegSettingsInfo)
    {
        return FALSE;
    }

    pRegSettingsInfo->regSetting[regCount].registerAddr  = 0x0D12;
    pRegSettingsInfo->regSetting[regCount].registerData  = rGain;
    pRegSettingsInfo->regSetting[regCount].regAddrType   = I2CRegAddressDataTypeWord;
    pRegSettingsInfo->regSetting[regCount].regDataType   = I2CRegAddressDataTypeWord;
    pRegSettingsInfo->regSetting[regCount].delayUs       = 0;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = 0x0D14;
    pRegSettingsInfo->regSetting[regCount].registerData  = gGain;
    pRegSettingsInfo->regSetting[regCount].regAddrType   = I2CRegAddressDataTypeWord;
    pRegSettingsInfo->regSetting[regCount].regDataType   = I2CRegAddressDataTypeWord;
    pRegSettingsInfo->regSetting[regCount].delayUs       = 0;
    regCount++;

    pRegSettingsInfo->regSetting[regCount].registerAddr  = 0x0D16;
    pRegSettingsInfo->regSetting[regCount].registerData  = bGain;
    pRegSettingsInfo->regSetting[regCount].regAddrType   = I2CRegAddressDataTypeWord;
    pRegSettingsInfo->regSetting[regCount].regDataType   = I2CRegAddressDataTypeWord;
    pRegSettingsInfo->regSetting[regCount].delayUs       = 0;
    regCount++;

    pRegSettingsInfo->regSettingCount = regCount;

    if (MAX_REG_SETTINGS <= regCount)
    {
        return FALSE;
    }
    else
    {
        return TRUE;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RDIHDRStatsParse
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RDIHDRStatsParse(UINT8* pInputData, VOID* pOutputData, UINT reserved1, UINT reserved2)
{
    StatsHDR3ExposureDataType* pDataParse = reinterpret_cast<StatsHDR3ExposureDataType *>(pOutputData);
    CDK_UNUSED_PARAM(reserved1);
    CDK_UNUSED_PARAM(reserved2);
    INT32 i = 0;
    memset(&ae_stat,0,sizeof(ae_stat));
    if (isValidStat(pInputData)) {

        // split histogram
        if (parseAEStatHistSplit(pInputData, &ae_stat.split_hist)) {
            pDataParse->splitBayerHist.mergedType        = HDR3ExpSplitLong;
            pDataParse->splitBayerHist.measurementRegion = GlobalRegion;
            pDataParse->splitBayerHist.binCount          = 256;
            pDataParse->splitHistchannelCount            = 0;
            // Y
            if(ae_stat.split_hist.split_y_hist_valid == true){
                // pDataParse->splitBayerHist.channelCount++;
                pDataParse->splitHistchannelCount++;
                for(i = 0; i < ae_stat.split_hist.split_y_hist.num_of_active_mem; i++){
                    pDataParse->splitBayerHist.histDataType[0] = StatsColorChannelY;
                    memcpy(pDataParse->splitBayerHist.pHistData[0], ae_stat.split_hist.split_y_hist.hist_data[i],
                        sizeof(UINT32) * ae_stat.split_hist.split_y_hist.num_bins[i]);
                }
            }
            // R
            if(ae_stat.split_hist.split_r_hist_valid == true){
                // pDataParse->splitBayerHist.channelCount++;
                pDataParse->splitHistchannelCount++;
                for(i = 0; i < ae_stat.split_hist.split_r_hist.num_of_active_mem; i++){
                    pDataParse->splitBayerHist.histDataType[1] = StatsColorChannelR;
                    memcpy(pDataParse->splitBayerHist.pHistData[1], ae_stat.split_hist.split_r_hist.hist_data[i],
                        sizeof(UINT32) * ae_stat.split_hist.split_r_hist.num_bins[i]);
                }
            }
            // B
            if(ae_stat.split_hist.split_b_hist_valid == true){
                // pDataParse->splitBayerHist.channelCount++;
                pDataParse->splitHistchannelCount++;
                for(i = 0; i < ae_stat.split_hist.split_b_hist.num_of_active_mem; i++){
                    pDataParse->splitBayerHist.histDataType[2] = StatsColorChannelB;
                    memcpy(pDataParse->splitBayerHist.pHistData[2], ae_stat.split_hist.split_b_hist.hist_data[i],
                        sizeof(UINT32) * ae_stat.split_hist.split_b_hist.num_bins[i]);
                }
            }
            // G
            if(ae_stat.split_hist.split_g_hist_valid == true){
                // pDataParse->splitBayerHist.channelCount++;
                pDataParse->splitHistchannelCount++;
                for(i = 0; i < ae_stat.split_hist.split_g_hist.num_of_active_mem; i++){
                    pDataParse->splitBayerHist.histDataType[3] = StatsColorChannelG;
                    memcpy(pDataParse->splitBayerHist.pHistData[3], ae_stat.split_hist.split_g_hist.hist_data[i],
                        sizeof(UINT32) * ae_stat.split_hist.split_g_hist.num_bins[i]);
                }
            }
            pDataParse->splitBayerHist.totalHistCount             = 2973696;
            pDataParse->splitBayerHist.histWeightUnit             = 1;
        }

        // merged histogram
        if (parseAEStatHistMerged(pInputData, &ae_stat.merged_hist)) {
            pDataParse->mergedBayerHist.mergedType        = HDR3ExpMerged;
            pDataParse->mergedBayerHist.measurementRegion = GlobalRegion;
            pDataParse->mergedBayerHist.binCount          = 256;
            pDataParse->mergedHistchannelCount            = 0;
            //Y
            if(ae_stat.merged_hist.merged_y_hist_valid == true){
                // pDataParse->mergedBayerHist.channelCount++;
                pDataParse->mergedHistchannelCount++;
                for(i = 0; i < ae_stat.merged_hist.merged_y_hist.num_of_active_mem; i++){
                    pDataParse->mergedBayerHist.histDataType[0] = StatsColorChannelY;
                    memcpy(pDataParse->mergedBayerHist.pHistData[0], ae_stat.merged_hist.merged_y_hist.hist_data[i],
                        sizeof(UINT32) * 256);
                }
            }
            // R
            if(ae_stat.merged_hist.merged_r_hist_valid == true){
                // pDataParse->mergedBayerHist.channelCount++;
                pDataParse->mergedHistchannelCount++;
                for(i = 0; i < ae_stat.merged_hist.merged_r_hist.num_of_active_mem; i++){
                    pDataParse->mergedBayerHist.histDataType[1] = StatsColorChannelR;
                    memcpy(pDataParse->mergedBayerHist.pHistData[1], ae_stat.merged_hist.merged_r_hist.hist_data[i],
                        sizeof(UINT32) * 256);
                }
            }
            // B
            if(ae_stat.merged_hist.merged_b_hist_valid == true){
                // pDataParse->mergedBayerHist.channelCount++;
                pDataParse->mergedHistchannelCount++;
                for(i = 0; i < ae_stat.merged_hist.merged_b_hist.num_of_active_mem; i++){
                    pDataParse->mergedBayerHist.histDataType[2] = StatsColorChannelB;
                    memcpy(pDataParse->mergedBayerHist.pHistData[2], ae_stat.merged_hist.merged_b_hist.hist_data[i],
                        sizeof(UINT32) * 256);
                }
            }
            // G
            if(ae_stat.merged_hist.merged_g_hist_valid == true){
                // pDataParse->mergedBayerHist.channelCount++;
                pDataParse->mergedHistchannelCount++;
                for(i = 0; i < ae_stat.merged_hist.merged_g_hist.num_of_active_mem; i++){
                    pDataParse->mergedBayerHist.histDataType[3] = StatsColorChannelG;
                    memcpy(pDataParse->mergedBayerHist.pHistData[3], ae_stat.merged_hist.merged_g_hist.hist_data[i],
                        sizeof(UINT32) * 256);
                }
            }
            pDataParse->mergedBayerHist.totalHistCount             = 2973696;
            pDataParse->mergedBayerHist.histWeightUnit             = 1;
        }

        // thumb nail(grid) stat
        if (parseAEStatThumbnail(pInputData, &ae_stat.thumb_nail)) {
            pDataParse->gridBayerStats.horizontalRegionCount = ae_stat.thumb_nail.region_h_num;
            pDataParse->gridBayerStats.verticalRegionCount   = ae_stat.thumb_nail.region_v_num;
            pDataParse->gridBayerStats.totalRegionCount      = ae_stat.thumb_nail.num_of_patch;
            pDataParse->gridBayerStats.regionWidth           = ae_stat.thumb_nail.region_width;
            pDataParse->gridBayerStats.regionHeight          = ae_stat.thumb_nail.region_height;
            pDataParse->gridBayerStats.bitDepth              = 12;
            memcpy(pDataParse->gridBayerStats.pGridData[0], ae_stat.thumb_nail.thumb_nail_data.r_avg,
                sizeof(UINT16) * ae_stat.thumb_nail.num_of_patch);
            memcpy(pDataParse->gridBayerStats.pGridData[1], ae_stat.thumb_nail.thumb_nail_data.g_avg,
                sizeof(UINT16) * ae_stat.thumb_nail.num_of_patch);
            memcpy(pDataParse->gridBayerStats.pGridData[2], ae_stat.thumb_nail.thumb_nail_data.b_avg,
                sizeof(UINT16) * ae_stat.thumb_nail.num_of_patch);
            memcpy(pDataParse->gridBayerStats.pGridData[3], ae_stat.thumb_nail.thumb_nail_data.y_avg,
                sizeof(UINT16) * ae_stat.thumb_nail.num_of_patch);
        }
    }
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
    pSensorLibraryAPI->pFillAutoWhiteBalanceSettings = FillAutoWhiteBalanceSettings;
    pSensorLibraryAPI->pStatsParse                   = RDIHDRStatsParse;
}

#ifdef __cplusplus
} // CamX Namespace
#endif // __cplusplus
