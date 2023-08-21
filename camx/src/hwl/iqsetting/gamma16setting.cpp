// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gamma16setting.cpp
/// @brief Gamma16 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "gamma16setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Gamma16Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Gamma16Setting::CalculateHWSetting(
    const Gamma16InputData*                                   pInput,
    gamma_1_6_0::mod_gamma16_channel_dataType*                pChannelData,
    gamma_1_6_0::chromatix_gamma16Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                     pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pChannelData)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        Gamma16UnpackedField* pUnpackedField = static_cast<Gamma16UnpackedField*>(pOutput);

        pUnpackedField->enable = static_cast<UINT16>(pModuleEnable->gamma_enable);

        pUnpackedField->r_lut_in_cfg.tableSelect = pInput->LUTBankSel;
        pUnpackedField->g_lut_in_cfg.tableSelect = pInput->LUTBankSel;
        pUnpackedField->b_lut_in_cfg.tableSelect = pInput->LUTBankSel;

        /// @todo (CAMX-1828) For now using the same table for all three channels
        GenerateGammaLUT(pChannelData[GammaLUTChannel0].gamma16_rgn_data.table, pUnpackedField->g_lut_in_cfg.pGammaTable);
        GenerateGammaLUT(pChannelData[GammaLUTChannel1].gamma16_rgn_data.table, pUnpackedField->b_lut_in_cfg.pGammaTable);
        GenerateGammaLUT(pChannelData[GammaLUTChannel2].gamma16_rgn_data.table, pUnpackedField->r_lut_in_cfg.pGammaTable);
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Gamma16Setting::GenerateGammaLUT
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Gamma16Setting::GenerateGammaLUT(
    FLOAT*  pGamma_table,
    UINT32* pLut)
{
    INT32  lut_delta  = 0;
    UINT16 x          = 0;
    UINT16 y          = 0;
    UINT32 i          = 0;

    const UINT16 mask = NMAX10 - 1;

    /// Gamma table in Chromatix has 65 entries and the HW LUT has 64. The 65th chromatix entry is used to
    /// compute the delta of last HW LUT entry.
    for (i = 0; i < DMIRAM_RGB_CH0_LENGTH_RGBLUT_V16; i++)
    {
        x = IQSettingUtils::ClampUINT16(static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pGamma_table[i])), 0, (NMAX10 - 1));
        if (i < DMIRAM_RGB_CH0_LENGTH_RGBLUT_V16 - 1)
        {
            y = IQSettingUtils::ClampUINT16(
                static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pGamma_table[i + 1])), 0, (NMAX10 - 1));
        }
        else
        {
            y = IQSettingUtils::ClampUINT16(static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pGamma_table[i + 1])), 0, NMAX10);
        }

        lut_delta = y - x;
        lut_delta = IQSettingUtils::ClampINT32(lut_delta, MIN_INT10, MAX_INT10);
        lut_delta = (lut_delta & mask);  // bpp
        /// Each Gamma LUT entry has 10 bit LSB base value and 10 bit MSB delta
        pLut[i]   = (x) | (lut_delta << GAMMA16_HW_PACK_BIT);
    }

    return;
}
