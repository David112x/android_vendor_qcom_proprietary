// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  linearization34setting.cpp
/// @brief Linearization34 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "linearization34setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Linearization34Setting::CalculateDelta
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Linearization34Setting::CalculateDelta(
    FLOAT* pLUTP,
    FLOAT* pLUTBase,
    FLOAT* pLUTDelta,
    BOOL   pedestalEnable)
{
    UINT32 count;
    // @todo (CAMX-1399) chromatix_L_out->last_region_unity_slope_en == 0
    BOOL   unitySlopeEn = FALSE;
    FLOAT  maxLUTValue  = static_cast<FLOAT>(LINEARIZATION34_LUT_MAX);

    if (TRUE == pedestalEnable)
    {
        for (count = 1; count < MAX_KNEE_POINTS; count++)
        {
            pLUTP[count] -= pLUTP[0];
        }
        pLUTP[0] = 0.0f;
    }

    // solve divided-by-zero case
    if (TRUE == IQSettingUtils::FEqual(pLUTP[0], 0.0f))
    {
        pLUTDelta[0] = 1.0f;
    }
    else
    {
        pLUTDelta[0] = (pLUTBase[1] - pLUTBase[0]) / (pLUTP[0]);
    }

    for (count = 1; count < MAX_KNEE_POINTS; count++)
    {
        if (IQSettingUtils::FEqual(pLUTP[count], pLUTP[count - 1]))
        {
            pLUTDelta[count] = 0.0f;
        }
        else
        {
            pLUTDelta[count] = (pLUTBase[count + 1] - pLUTBase[count]) / (pLUTP[count] - pLUTP[count - 1]);
        }
    }

    if (FALSE == unitySlopeEn)
    {
        if (pLUTP[MAX_KNEE_POINTS - 1] < maxLUTValue)
        {
            pLUTDelta[MAX_SLOPE - 1] = (maxLUTValue - pLUTBase[MAX_SLOPE - 1]) /
                (maxLUTValue - pLUTP[MAX_KNEE_POINTS - 1]);
        }
        else
        {
            pLUTDelta[MAX_SLOPE - 1] = 0.0f;
        }
    }
    else
    {
        pLUTDelta[MAX_SLOPE - 1] = 1.0f;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Linearization34Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Linearization34Setting::CalculateHWSetting(
    const Linearization34IQInput*                                             pInput,
    linearization_3_4_0::linearization34_rgn_dataType*                        pData,
    linearization_3_4_0::chromatix_linearization34Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                                     pOutput)
{
    BOOL   result = TRUE;
    UINT32 count  = 0;
    FLOAT  rLUTDelta[MAX_SLOPE];
    FLOAT  grLUTDelta[MAX_SLOPE];
    FLOAT  bLUTDelta[MAX_SLOPE];
    FLOAT  gbLUTDelta[MAX_SLOPE];

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        Linearization34UnpackedField* pUnpackedField = static_cast<Linearization34UnpackedField*>(pOutput);

        pUnpackedField->enable           = static_cast<UINT16>(pModuleEnable->linearization_enable);
        pUnpackedField->bayerPattern     = static_cast<UINT16>(pInput->bayerPattern);
        pUnpackedField->LUTbankSelection = pInput->LUTBankSel;

        CalculateDelta(pData->r_lut_p_tab.r_lut_p,
                       pData->r_lut_base_tab.r_lut_base,
                       rLUTDelta,
                       pInput->pedestalEnable);
        CalculateDelta(pData->gr_lut_p_tab.gr_lut_p,
                       pData->gr_lut_base_tab.gr_lut_base,
                       grLUTDelta,
                       pInput->pedestalEnable);
        CalculateDelta(pData->gb_lut_p_tab.gb_lut_p,
                       pData->gb_lut_base_tab.gb_lut_base,
                       gbLUTDelta,
                       pInput->pedestalEnable);
        CalculateDelta(pData->b_lut_p_tab.b_lut_p,
                       pData->b_lut_base_tab.b_lut_base,
                       bLUTDelta,
                       pInput->pedestalEnable);

        for (count = 0; count < MAX_KNEE_POINTS; count++)
        {
            pUnpackedField->rLUTkneePointL[count]  =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->r_lut_p_tab.r_lut_p[count]),
                                    LINEARIZATION34_LUT_MIN,
                                    LINEARIZATION34_LUT_MAX));

            pUnpackedField->grLUTkneePointL[count] =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gr_lut_p_tab.gr_lut_p[count]),
                                    LINEARIZATION34_LUT_MIN,
                                    LINEARIZATION34_LUT_MAX));

            pUnpackedField->gbLUTkneePointL[count] =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gb_lut_p_tab.gb_lut_p[count]),
                                    LINEARIZATION34_LUT_MIN,
                                    LINEARIZATION34_LUT_MAX));

            pUnpackedField->bLUTkneePointL[count]  =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->b_lut_p_tab.b_lut_p[count]),
                                    LINEARIZATION34_LUT_MIN,
                                    LINEARIZATION34_LUT_MAX));

        }

        // Program the base levels and slopes to HW
        for (count = 0; count < MAX_SLOPE; count++)
        {
            pUnpackedField->rLUTbaseL[pInput->LUTBankSel][count]   = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->r_lut_base_tab.r_lut_base[count]),
                                           LINEARIZATION34_LUT_MIN,
                                           LINEARIZATION34_LUT_MAX));

            pUnpackedField->grLUTbaseL[pInput->LUTBankSel][count]  = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gr_lut_base_tab.gr_lut_base[count]),
                                           LINEARIZATION34_LUT_MIN,
                                           LINEARIZATION34_LUT_MAX));

            pUnpackedField->gbLUTbaseL[pInput->LUTBankSel][count]  = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gb_lut_base_tab.gb_lut_base[count]),
                                           LINEARIZATION34_LUT_MIN,
                                           LINEARIZATION34_LUT_MAX));

            pUnpackedField->bLUTbaseL[pInput->LUTBankSel][count]   = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->b_lut_base_tab.b_lut_base[count]),
                                           LINEARIZATION34_LUT_MIN,
                                           LINEARIZATION34_LUT_MAX));

            pUnpackedField->rLUTdeltaL[pInput->LUTBankSel][count]  = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(rLUTDelta[count], LINEARIZATION34_Q_FACTOR_SLOPE),
                                           LINEARIZATION34_LUT_MIN,
                                           LINEARIZATION34_LUT_MAX));

            pUnpackedField->grLUTdeltaL[pInput->LUTBankSel][count] = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(grLUTDelta[count], LINEARIZATION34_Q_FACTOR_SLOPE),
                                           LINEARIZATION34_LUT_MIN,
                                           LINEARIZATION34_LUT_MAX));

            pUnpackedField->gbLUTdeltaL[pInput->LUTBankSel][count] = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(gbLUTDelta[count], LINEARIZATION34_Q_FACTOR_SLOPE),
                                           LINEARIZATION34_LUT_MIN,
                                           LINEARIZATION34_LUT_MAX));

            pUnpackedField->bLUTdeltaL[pInput->LUTBankSel][count]  = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(bLUTDelta[count], LINEARIZATION34_Q_FACTOR_SLOPE),
                                           LINEARIZATION34_LUT_MIN,
                                           LINEARIZATION34_LUT_MAX));
        }
    }
    else
    {
        result = FALSE;
    }

    return result;
}
