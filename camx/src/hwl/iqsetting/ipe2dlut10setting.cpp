// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ipe2dlut10setting.cpp
/// @brief IPE TDL10 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ipe2dlut10setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPETDL10Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPETDL10Setting::CalculateHWSetting(
    const TDL10InputData*                                 pInput,
    tdl_1_0_0::tdl10_rgn_dataType*                        pData,
    tdl_1_0_0::chromatix_tdl10_reserveType*               pReserveType,
    tdl_1_0_0::chromatix_tdl10Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    UINT                index          = 0;
    BOOL                result         = TRUE;
    TDL10UnpackedField* pUnpackedField = NULL;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pOutput))
    {
        pUnpackedField         = static_cast<TDL10UnpackedField*>(pOutput);

        pUnpackedField->enable = static_cast<UINT8>(pModuleEnable->twodlut_enable);

        for (index = 0; index < H_GRID; index++)
        {
            pUnpackedField->hs_lut.lut_1d_h[index] = static_cast<UINT16>(
                IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                (pReserveType->lut_1d_h_tab.lut_1d_h[index] / 60.0f),
                                                QNumber_11U),
                    IQSettingUtils::MINUINTBITFIELD(14),
                    LUT1D_H));
        }

        for (index = 0; index < S_GRID; index++)
        {
            pUnpackedField->hs_lut.lut_1d_s[index] = static_cast<UINT16>(
                IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pReserveType->lut_1d_s_tab.lut_1d_s[index],
                                                                           QNumber_11U),
                    IQSettingUtils::MINUINTBITFIELD(12),
                    LUT1D_S));
        }

        pUnpackedField->k_b_integer = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pReserveType->k_b_integer, QNumber_9U),
                                        KINTEGERMIN,
                                        KINTEGERMAX));

        pUnpackedField->k_r_integer = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pReserveType->k_r_integer, QNumber_9U),
                                        KINTEGERMIN,
                                        KINTEGERMAX));

        pUnpackedField->h_shift = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->h_shift, 0, 5));
        pUnpackedField->s_shift = static_cast<UINT16>(IQSettingUtils::ClampUINT32(pReserveType->s_shift, 0, 3));

        for (index = 0; index < ((H_GRID - 1) * S_GRID); index++)
        {
            pUnpackedField->hs_lut.lut_2d_h[index] = static_cast<UINT32>(
                IQSettingUtils::RoundFLOAT(((pData->lut_2d_h_tab.lut_2d_h[index] / 60.0f)*(1 << 11)) /
                                           (1 << pUnpackedField->h_shift)));

            pUnpackedField->hs_lut.lut_2d_h[index] =
                IQSettingUtils::ClampINT32(pUnpackedField->hs_lut.lut_2d_h[index],
                                            LUT2D_MIN,
                                            LUT2D_MAX);

            pUnpackedField->hs_lut.lut_2d_s[index] = static_cast<UINT32>(
                IQSettingUtils::RoundFLOAT((pData->lut_2d_s_tab.lut_2d_s[index] * (1 << 11)) /
                                           (1 << pUnpackedField->s_shift)));

            pUnpackedField->hs_lut.lut_2d_s[index] =
                IQSettingUtils::ClampINT32(pUnpackedField->hs_lut.lut_2d_s[index],
                                           LUT2D_MIN,
                                           LUT2D_MAX);
        }

        for (index = 0; index < (S_GRID); index++)
        {
            pUnpackedField->hs_lut.lut_2d_h[((H_GRID - 1)*S_GRID) + index] =
                pUnpackedField->hs_lut.lut_2d_h[index];
            pUnpackedField->hs_lut.lut_2d_s[((H_GRID - 1)*S_GRID) + index] =
                pUnpackedField->hs_lut.lut_2d_s[index];
        }

        for (index = 1; index < (H_GRID); index++)
        {
            pUnpackedField->hs_lut.lut_1d_h_inv[index - 1] = static_cast<UINT32>(
                IQSettingUtils::RoundDOUBLE(ceil(1.0f*(1 << H_inverse_q_factor) /
                    (pUnpackedField->hs_lut.lut_1d_h[index] - pUnpackedField->hs_lut.lut_1d_h[index - 1]))));

            pUnpackedField->hs_lut.lut_1d_h_inv[index - 1] =
                IQSettingUtils::ClampUINT32(pUnpackedField->hs_lut.lut_1d_h_inv[index - 1],
                                            IQSettingUtils::MINUINTBITFIELD(14),
                                            IQSettingUtils::MAXUINTBITFIELD(14));
        }

        for (index = 1; index < (S_GRID); index++)
        {
            pUnpackedField->hs_lut.lut_1d_s_inv[index - 1] = static_cast<UINT32>(
                IQSettingUtils::RoundDOUBLE(ceil(1.0f*(1 << S_inverse_q_factor) /
                    (pUnpackedField->hs_lut.lut_1d_s[index] - pUnpackedField->hs_lut.lut_1d_s[index - 1]))));

            pUnpackedField->hs_lut.lut_1d_s_inv[index - 1] =
                IQSettingUtils::ClampUINT32(pUnpackedField->hs_lut.lut_1d_s_inv[index - 1],
                                            IQSettingUtils::MINUINTBITFIELD(11),
                                            IQSettingUtils::MAXUINTBITFIELD(11));
        }

        pUnpackedField->l_boundary_start_A = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pData->l_boundary_start_a, QNumber_11U),
                                        IQSettingUtils::MINUINTBITFIELD(11),
                                        IQSettingUtils::MAXUINTBITFIELD(11)));

        pUnpackedField->l_boundary_start_B = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pData->l_boundary_start_b, QNumber_11U),
                                        IQSettingUtils::MINUINTBITFIELD(11),
                                        IQSettingUtils::MAXUINTBITFIELD(11)));

        pUnpackedField->l_boundary_end_A = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pData->l_boundary_end_a, QNumber_11U),
                                        IQSettingUtils::MINUINTBITFIELD(11),
                                        IQSettingUtils::MAXUINTBITFIELD(11)));

        pUnpackedField->l_boundary_end_B = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pData->l_boundary_end_b, QNumber_11U),
                                        IQSettingUtils::MINUINTBITFIELD(11),
                                        IQSettingUtils::MAXUINTBITFIELD(11)));

        pUnpackedField->y_blend_factor_integer = static_cast<UINT16>(
            IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(pData->y_blend_factor_integer, QNumber_4U),
                                        Y_BLEND_FACTOR_INTEGER_MIN,
                                        Y_BLEND_FACTOR_INTEGER_MAX));

        if (pUnpackedField->l_boundary_start_B > pUnpackedField->l_boundary_start_A)
        {
            pUnpackedField->l_boundary_start_inv =
                IQSettingUtils::ClampUINT16(static_cast<UINT16>(IQSettingUtils::RoundDOUBLE(
                                                                    ceil(1.0f * (1 << L_inverse_q_factor)/
                                                                    (pUnpackedField->l_boundary_start_B -
                                                                        pUnpackedField->l_boundary_start_A)))),
                                            L_BOUNDARY_MIN,
                                            L_BOUNDARY_MAX);
        }
        else
        {
            pUnpackedField->l_boundary_start_inv = 1;
        }

        if (pUnpackedField->l_boundary_end_B > pUnpackedField->l_boundary_end_A)
        {
            pUnpackedField->l_boundary_end_inv = static_cast<UINT16>(
                IQSettingUtils::RoundDOUBLE(ceil(1.0f * (1 << L_inverse_q_factor) /
                    (pUnpackedField->l_boundary_end_B - pUnpackedField->l_boundary_end_A))));

            pUnpackedField->l_boundary_end_inv =
                IQSettingUtils::ClampUINT16(pUnpackedField->l_boundary_end_inv,
                                            L_BOUNDARY_MIN,
                                            L_BOUNDARY_MAX);
        }
        else
        {
            pUnpackedField->l_boundary_end_inv = 1;
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}
