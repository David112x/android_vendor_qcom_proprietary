// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gra10setting.cpp
/// @brief GRA10 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gra10setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GRA10Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL GRA10Setting::CalculateHWSetting(
    const GRA10IQInput*                                   pInput,
    gra_1_0_0::gra10_rgn_dataType*                        pData,
    gra_1_0_0::chromatix_gra10_reserveType*               pReserveData,
    gra_1_0_0::chromatix_gra10Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    BOOL   result                                          = TRUE;

    /// @todo (CAMX-1812) Need to read Bit perpixel info from ISP o/p
    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pReserveData)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        GRA10UnpackedField*                     pUnpackedField = static_cast<GRA10UnpackedField*>(pOutput);
        UINT16 outBPP                                          = 10;
        UINT32 frameNum                                        = pInput->frameNum;

        pUnpackedField->enable             = static_cast<UINT16>(pModuleEnable->gra_enable);
        pUnpackedField->enable_dithering_Y = pReserveData->enable_dithering_y & 1;
        pUnpackedField->enable_dithering_C = pReserveData->enable_dithering_c & 1;
        pUnpackedField->mcg_a              = pReserveData->mcg_a;
        pUnpackedField->skip_ahead_a_jump  = pReserveData->skip_ahead_a_jump;
        pUnpackedField->grain_strength     = static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->grain_strength));

        if (BITPERPIXEL == outBPP)
        {
            pUnpackedField->r = 2;
        }
        else
        {
            pUnpackedField->r = 0;
        }

        // Seed is odd number for algorithm , 6 is suggested by system team to generate random number
        pUnpackedField->grain_seed = (frameNum * 6) + 1;

        for (UINT32 index = 0; index < GRA10LUTNumEntriesPerChannel; index++)
        {
            pUnpackedField->ch0_LUT.pGRATable[index] =
                static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->y_weight_lut_tab.y_weight_lut[index]));
            pUnpackedField->ch1_LUT.pGRATable[index] =
                static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->cb_weight_lut_tab.cb_weight_lut[index]));
            pUnpackedField->ch2_LUT.pGRATable[index] =
                static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->cr_weight_lut_tab.cr_weight_lut[index]));
        }
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}
