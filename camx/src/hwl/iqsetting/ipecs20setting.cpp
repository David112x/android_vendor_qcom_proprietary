// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ipecs20setting.cpp
/// @brief IPE CS20 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ipecs20setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPECS20Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IPECS20Setting::CalculateHWSetting(
    const CS20InputData*                                pInput,
    cs_2_0_0::cs20_rgn_dataType*                        pData,
    cs_2_0_0::chromatix_cs20_reserveType*               pReserveType,
    cs_2_0_0::chromatix_cs20Type::enable_sectionStruct* pModuleEnable,
    VOID*                                               pOutput)
{
    BOOL   result           = TRUE;
    UINT   i;
    /// @todo (CAMX-2108) check Bit depth
    UINT16 modInputBitDepth = 10;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pOutput))
    {
        CS20UnpackedField* pUnpackedField = static_cast<CS20UnpackedField*>(pOutput);

        pUnpackedField->CS_enable = static_cast<UINT8>(pModuleEnable->chroma_suppression_enable);

        /// @todo (CAMX-2108) need to find out where these values should be from
        // from CSIM code, it seems that default values for CS_Q_RES_CHROMA_CFG and CS_Q_RES_LUMA_CFG are 0s
        pUnpackedField->chroma_q = static_cast<UINT8>(IQSettingUtils::ClampUINT32(pReserveType->chroma_q, 0, 3));
        pUnpackedField->luma_q   = static_cast<UINT8>(IQSettingUtils::ClampUINT32(pReserveType->luma_q, 0, 3));

        // calculate shift bit
        INT bitshift = modInputBitDepth - 8;

        if (bitshift < 0)
        {
            bitshift = 0;
        }

        INT qFactorChroma = 10 + bitshift + pUnpackedField->chroma_q;
        INT qFactorLuma   = 10 + bitshift + pUnpackedField->luma_q;

        FLOAT tempValue  = 0.0f;

        pData->knee_point_lut_tab.knee_point_lut[0]                        = 0;

        for (i = 0; i < CHROMA_SUPP_LUT_SIZE - 1; i++)
        {
            pUnpackedField->knee_point_lut[i] =
                IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->knee_point_lut_tab.knee_point_lut[i] * (1 << bitshift)),
                                            0,
                                            NMAX10 -1);
            if (pData->knee_point_lut_tab.knee_point_lut[i + 1] == pData->knee_point_lut_tab.knee_point_lut[i])
            {
                tempValue = 0.0f;
            }
            else
            {
                tempValue = static_cast<FLOAT>(
                    1.0f / (pData->knee_point_lut_tab.knee_point_lut[i + 1] - pData->knee_point_lut_tab.knee_point_lut[i]));
            }
            pUnpackedField->knee_point_inverse_lut[i] = static_cast<UINT32>(ceil(tempValue * (1 << qFactorLuma)));
            pUnpackedField->knee_point_inverse_lut[i] =
                IQSettingUtils::ClampUINT32(pUnpackedField->knee_point_inverse_lut[i], 0, MAX_UINT12);
        }
        pUnpackedField->knee_point_lut[CHROMA_SUPP_LUT_SIZE - 1] = IQSettingUtils::MAXUINTBITFIELD(10);

        tempValue = static_cast<FLOAT>(
            1.0f / static_cast<FLOAT>(1024 - pUnpackedField->knee_point_lut[CHROMA_SUPP_LUT_SIZE - 1]));

        pUnpackedField->knee_point_inverse_lut[CHROMA_SUPP_LUT_SIZE - 1] =
            static_cast<UINT32>(tempValue * (1 << qFactorLuma));
        pUnpackedField->knee_point_inverse_lut[CHROMA_SUPP_LUT_SIZE - 1] =
            IQSettingUtils::ClampUINT32(pUnpackedField->knee_point_inverse_lut[CHROMA_SUPP_LUT_SIZE - 1], 0, MAX_UINT12);

        /// @todo (CAMX-2108) FIXME: Check whether 0 or not
        pUnpackedField->knee_point_inverse_lut[CHROMA_SUPP_LUT_SIZE - 1] = 0;

        for (i = 0; i < CHROMA_SUPP_LUT_SIZE; i++)
        {
            // lower threshold
            pUnpackedField->c_thr_lut[i] = static_cast<UINT16>(
                IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->c_thr1_lut_tab.c_thr1_lut[i] * (1 << bitshift)),
                                           CS20_C_THR_LUT_MIN,
                                           CS20_C_THR_LUT_MAX));
            // upper threshold
            UINT16 c_thr2_lut = static_cast<UINT16>(pData->c_thr2_lut_tab.c_thr2_lut[i] * (1 << bitshift));

            pUnpackedField->c_slope_lut[i] =
                static_cast<UINT16>((1.0 / (c_thr2_lut - pUnpackedField->c_thr_lut[i])) * (1ULL << qFactorChroma));
            pUnpackedField->c_slope_lut[i] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->c_slope_lut[i], 0, MAX_UINT12));
        }

        for (i = 0; i < CHROMA_SUPP_LUT_SIZE; i++)
        {
            /// @todo (CAMX-2108) FIXME: Need to check it out
            pUnpackedField->y_weight_lut[i] = static_cast<UINT16>(
                (pData->y_weight_lut_tab.y_weight_lut[i] * (1 << (8 + bitshift))));
            pUnpackedField->y_weight_lut[i] =
                static_cast<UINT16>(IQSettingUtils::ClampUINT32(pUnpackedField->y_weight_lut[i], 0, MAX_UINT11));
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }

    return result;
}
