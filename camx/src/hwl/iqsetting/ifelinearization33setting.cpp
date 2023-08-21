// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifelinearization33setting.cpp
/// @brief IFE Linearization33 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifelinearization33setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELinearization33Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFELinearization33Setting::CalculateHWSetting(
    const Linearization33InputData*                    pInput,
    linearization_3_3_0::linearization33_rgn_dataType* pData,
    VOID*                                              pOutput)
{
    BOOL  result = TRUE;
    FLOAT rLUTDelta[MaxSlope];
    FLOAT grLUTDelta[MaxSlope];
    FLOAT bLUTDelta[MaxSlope];
    FLOAT gbLUTDelta[MaxSlope];

    UINT32 count = 0;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pOutput))
    {
        Linearization33UnpackedField* pUnpackedField = static_cast<Linearization33UnpackedField*>(pOutput);

        pUnpackedField->lut_bank_sel = pInput->lutBankSel;

        /// @todo (CAMX-1812): add one more input as IFEPipelineCfg
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

        for ( count = 0; count < MaxKneepoints; count++ )
        {
            pUnpackedField->r_lut_p_l[count]  =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->r_lut_p_tab.r_lut_p[count]),
                                                               LINEARIZATION33_LUT_MIN,
                                                               LINEARIZATION33_LUT_MAX));

            pUnpackedField->r_lut_p_r[count]  =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->r_lut_p_tab.r_lut_p[count]),
                                                               LINEARIZATION33_LUT_MIN,
                                                               LINEARIZATION33_LUT_MAX));

            pUnpackedField->gr_lut_p_l[count] =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gr_lut_p_tab.gr_lut_p[count]),
                                                               LINEARIZATION33_LUT_MIN,
                                                               LINEARIZATION33_LUT_MAX));

            pUnpackedField->gr_lut_p_r[count] =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gr_lut_p_tab.gr_lut_p[count]),
                                                               LINEARIZATION33_LUT_MIN,
                                                               LINEARIZATION33_LUT_MAX));

            pUnpackedField->gb_lut_p_l[count] =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gb_lut_p_tab.gb_lut_p[count]),
                                                               LINEARIZATION33_LUT_MIN,
                                                               LINEARIZATION33_LUT_MAX));

            pUnpackedField->gb_lut_p_r[count] =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gb_lut_p_tab.gb_lut_p[count]),
                                                               LINEARIZATION33_LUT_MIN,
                                                               LINEARIZATION33_LUT_MAX));

            pUnpackedField->b_lut_p_l[count]  =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->b_lut_p_tab.b_lut_p[count]),
                                                               LINEARIZATION33_LUT_MIN,
                                                               LINEARIZATION33_LUT_MAX));

            pUnpackedField->b_lut_p_r[count]  =
                static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->b_lut_p_tab.b_lut_p[count]),
                                                               LINEARIZATION33_LUT_MIN,
                                                               LINEARIZATION33_LUT_MAX));
        }

        // Program the base levels and slopes to HW
        for ( count = 0; count < MaxSlope; count++ )
        {
            pUnpackedField->r_lut_base_l[pInput->lutBankSel][count]   = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->r_lut_base_tab.r_lut_base[count]),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->r_lut_base_r[pInput->lutBankSel][count]   = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->r_lut_base_tab.r_lut_base[count]),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->gr_lut_base_l[pInput->lutBankSel][count]  = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gr_lut_base_tab.gr_lut_base[count]),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->gr_lut_base_r[pInput->lutBankSel][count]  = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gr_lut_base_tab.gr_lut_base[count]),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->gb_lut_base_l[pInput->lutBankSel][count]  = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gb_lut_base_tab.gb_lut_base[count]),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->gb_lut_base_r[pInput->lutBankSel][count]  = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->gb_lut_base_tab.gb_lut_base[count]),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->b_lut_base_l[pInput->lutBankSel][count]   = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->b_lut_base_tab.b_lut_base[count]),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->b_lut_base_r[pInput->lutBankSel][count]   = static_cast<UINT16>(
                IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->b_lut_base_tab.b_lut_base[count]),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->r_lut_delta_l[pInput->lutBankSel][count]  = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(rLUTDelta[count], LINEARIZATION33_Q_FACTOR_SLOPE),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->r_lut_delta_r[pInput->lutBankSel][count]  = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(rLUTDelta[count], LINEARIZATION33_Q_FACTOR_SLOPE),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->gr_lut_delta_l[pInput->lutBankSel][count] = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(grLUTDelta[count], LINEARIZATION33_Q_FACTOR_SLOPE),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->gr_lut_delta_r[pInput->lutBankSel][count] = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(grLUTDelta[count], LINEARIZATION33_Q_FACTOR_SLOPE),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->gb_lut_delta_l[pInput->lutBankSel][count] = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(gbLUTDelta[count], LINEARIZATION33_Q_FACTOR_SLOPE),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->gb_lut_delta_r[pInput->lutBankSel][count] = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(gbLUTDelta[count], LINEARIZATION33_Q_FACTOR_SLOPE),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->b_lut_delta_l[pInput->lutBankSel][count]  = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(bLUTDelta[count], LINEARIZATION33_Q_FACTOR_SLOPE),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));

            pUnpackedField->b_lut_delta_r[pInput->lutBankSel][count]  = static_cast<UINT32>(
                IQSettingUtils::ClampINT32(IQSettingUtils::FloatToQNumber(bLUTDelta[count], LINEARIZATION33_Q_FACTOR_SLOPE),
                                           LINEARIZATION33_LUT_MIN,
                                           LINEARIZATION33_LUT_MAX));
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFELinearization33Setting::CalculateDelta
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELinearization33Setting::CalculateDelta(
    FLOAT* pLUTP,
    FLOAT* pLUTBase,
    FLOAT* pLUTDelta,
    BOOL   pedestalEnable)
{
    UINT count        = 0;
    /// @todo (CAMX-1812) chromatix_L_out->last_region_unity_slope_en == 0
    BOOL  unitySlopeEn = FALSE;
    FLOAT maxLutValue  = static_cast<FLOAT>(LINEARIZATION33_LUT_MAX);

    if (TRUE == pedestalEnable)
    {
        for (count = 1; count < MaxKneepoints; count++)
        {
            pLUTP[count] -= pLUTP[0];
        }
        pLUTP[0] = 0.0f;
    }

    // solve divided-by-zero case
    if (TRUE == IQSettingUtils::FEqual(pLUTP[0] , 0.0f))
    {
        pLUTDelta[0] = 1.0f;
    }
    else
    {
        pLUTDelta[0] = (pLUTBase[1] - pLUTBase[0]) / (pLUTP[0]);
    }

    for (count = 1; count < MaxKneepoints; count++)
    {
        // solve divided-by-zero case
        if (IQSettingUtils::FEqual(pLUTP[count] , pLUTP[count - 1]))
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
        if (pLUTP[MaxKneepoints - 1] < maxLutValue)
        {
            pLUTDelta[MaxSlope - 1] = (maxLutValue - pLUTBase[MaxSlope - 1]) /
                                      (maxLutValue - pLUTP[MaxKneepoints - 1]);
        }
        else
        {
            pLUTDelta[MaxSlope - 1] = 0.0f;
        }
    }
    else
    {
        pLUTDelta[MaxSlope - 1] = 1.0f;
    }
}
