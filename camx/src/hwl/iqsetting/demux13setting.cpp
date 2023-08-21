// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  demux13setting.cpp
/// @brief IFE Demux13 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "demux13setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Demux13Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Demux13Setting::CalculateHWSetting(
    const Demux13InputData*                                   pInput,
    const demux_1_3_0::chromatix_demux13_reserveType*         pReserveType,
    demux_1_3_0::chromatix_demux13Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                     pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pReserveType)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        Demux13UnpackedField* pUnpackedField = static_cast<Demux13UnpackedField*>(pOutput);
        FLOAT                 redGain        = pReserveType->red_gain;
        FLOAT                 greenGainEven  = pReserveType->green_gain_even;
        FLOAT                 greenGainOdd   = pReserveType->green_gain_odd;
        FLOAT                 blueGain       = pReserveType->blue_gain;
        FLOAT                 rGainTotal     = 0;
        FLOAT                 grGainTotal    = 0;
        FLOAT                 gbGainTotal    = 0;
        FLOAT                 bGainTotal     = 0;
        FLOAT                 maxGain        = 0;
        FLOAT                 allChannelGain = 0;
        UINT32                qNumber        = 10;
        UINT32                maxValue       = ((1 << 15) - 1);
        UINT32                minValue       = 0;

        redGain       *= pInput->digitalGain * pInput->stretchGainRed;
        greenGainEven *= pInput->digitalGain * pInput->stretchGainGreenEven;
        greenGainOdd  *= pInput->digitalGain * pInput->stretchGainGreenOdd;
        blueGain      *= pInput->digitalGain * pInput->stretchGainBlue;

        switch (pInput->bayerPattern)
        {
            case RGGB_PATTERN:
                pUnpackedField->period = 1;
                pUnpackedField->evenConfig = 0xC9;    // 0x41
                pUnpackedField->oddConfig  = 0xAC;    // 0x24
                break;

            case GRBG_PATTERN:
                pUnpackedField->period = 1;
                pUnpackedField->evenConfig = 0x9C;    // 0x14
                pUnpackedField->oddConfig  = 0xCA;    // 0x42
                break;

            case BGGR_PATTERN:
                pUnpackedField->period = 1;
                pUnpackedField->evenConfig = 0xCA;    // 0x42
                pUnpackedField->oddConfig  = 0x9C;    // 0x14
                break;

            case GBRG_PATTERN:
                pUnpackedField->period = 1;
                pUnpackedField->evenConfig = 0xAC;    // 0x24
                pUnpackedField->oddConfig  = 0xC9;    // 0x41
                break;

            case YCBYCR422_PATTERN:
                pUnpackedField->period = 3;
                pUnpackedField->evenConfig = 0x9CAC;    // 0x24
                pUnpackedField->oddConfig  = 0x9CAC;    // 0x41
                break;

            case YCRYCB422_PATTERN:
                pUnpackedField->period = 3;
                pUnpackedField->evenConfig = 0xAC9C;    // 0x24
                pUnpackedField->oddConfig  = 0xAC9C;    // 0x41
                break;

            case CBYCRY422_PATTERN:
                pUnpackedField->period = 3;
                pUnpackedField->evenConfig = 0xC9CA;    // 0x24
                pUnpackedField->oddConfig  = 0xC9CA;    // 0x41
                break;

            case CRYCBY422_PATTERN:
                pUnpackedField->period = 3;
                pUnpackedField->evenConfig = 0xCAC9;    // 0x24
                pUnpackedField->oddConfig  = 0xCAC9;    // 0x41
                break;

            case Y_PATTERN:
                pUnpackedField->period = 0;
                pUnpackedField->evenConfig = 0x0C;      // 0x0C
                pUnpackedField->oddConfig  = 0x0C;      // 0x0C
                break;

            default:
                break;
        }

        /// @todo (CAMX-561) Hard code Config Period and Config Config for now.
        // pUnpackedField->period = static_cast<UINT16>(pInput->demuxInConfigPeriod);
        pUnpackedField->enable = static_cast<UINT16>(pModuleEnable->demux_enable);

        // rggb
        if (pInput->bayerPattern == RGGB_PATTERN || pInput->bayerPattern == GRBG_PATTERN)
        {
            rGainTotal  = redGain;
            grGainTotal = greenGainEven;
            gbGainTotal = greenGainOdd;
            bGainTotal  = blueGain;
        }
        // grbg
        else if (pInput->bayerPattern == BGGR_PATTERN || pInput->bayerPattern == GBRG_PATTERN)
        {
            rGainTotal  = redGain;
            grGainTotal = greenGainOdd;
            gbGainTotal = greenGainEven;
            bGainTotal  = blueGain;
        }

        maxGain = IQSettingUtils::MaxFLOAT(IQSettingUtils::MaxFLOAT(rGainTotal, grGainTotal),
                                       IQSettingUtils::MaxFLOAT(gbGainTotal, bGainTotal));

        if (maxGain > MAX_DEMUX13_GAIN)
        {
            allChannelGain = static_cast<FLOAT>(MAX_DEMUX13_GAIN/maxGain);
            rGainTotal     = (rGainTotal  * allChannelGain);
            bGainTotal     = (bGainTotal  * allChannelGain);
            grGainTotal    = (grGainTotal * allChannelGain);
            gbGainTotal    = (gbGainTotal * allChannelGain);
        }

        // rggb
        if ((RGGB_PATTERN == pInput->bayerPattern) || (GRBG_PATTERN == pInput->bayerPattern))
        {
            pUnpackedField->gainChannel0OddLeft   = IQSettingUtils::DataConversion((gbGainTotal * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel0OddRight  = IQSettingUtils::DataConversion((gbGainTotal * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel0EevenLeft = IQSettingUtils::DataConversion((grGainTotal * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel0EvenRight = IQSettingUtils::DataConversion((grGainTotal * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel1Left      = IQSettingUtils::DataConversion((bGainTotal  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel1Right     = IQSettingUtils::DataConversion((bGainTotal  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel2Left      = IQSettingUtils::DataConversion((rGainTotal  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel2Right     = IQSettingUtils::DataConversion((rGainTotal  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
        }
        // grbg
        else if ((pInput->bayerPattern == BGGR_PATTERN) || (pInput->bayerPattern == GBRG_PATTERN))
        {
            pUnpackedField->gainChannel0OddLeft   = IQSettingUtils::DataConversion((grGainTotal * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel0OddRight  = IQSettingUtils::DataConversion((grGainTotal * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel0EevenLeft = IQSettingUtils::DataConversion((gbGainTotal * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel0EvenRight = IQSettingUtils::DataConversion((gbGainTotal * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel1Left      = IQSettingUtils::DataConversion((bGainTotal  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel1Right     = IQSettingUtils::DataConversion((bGainTotal  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel2Left      = IQSettingUtils::DataConversion((rGainTotal  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel2Right     = IQSettingUtils::DataConversion((rGainTotal  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
        }
        else
        {
            pUnpackedField->gainChannel0OddLeft   = IQSettingUtils::DataConversion((1.0f * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel0OddRight  = IQSettingUtils::DataConversion((1.0f * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel0EevenLeft = IQSettingUtils::DataConversion((1.0f * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel0EvenRight = IQSettingUtils::DataConversion((1.0f * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel1Left      = IQSettingUtils::DataConversion((1.0f  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel1Right     = IQSettingUtils::DataConversion((1.0f  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel2Left      = IQSettingUtils::DataConversion((1.0f  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
            pUnpackedField->gainChannel2Right     = IQSettingUtils::DataConversion((1.0f  * (1 << qNumber)),
                                                                                   minValue,
                                                                                   maxValue);
        }

        pUnpackedField->blackLevelIn  = IQSettingUtils::DataConversion(static_cast<FLOAT>(pInput->blackLevelOffset),
                                                                       DEMUX13_BLK_LVL_IN_MIN,
                                                                       DEMUX13_BLK_LVL_IN_MAX);
        pUnpackedField->blackLevelOut = IQSettingUtils::DataConversion(static_cast<FLOAT>(pInput->blackLevelOffset),
                                                                       DEMUX13_BLK_LVL_IN_MIN,
                                                                       DEMUX13_BLK_LVL_OUT_MAX);
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}
