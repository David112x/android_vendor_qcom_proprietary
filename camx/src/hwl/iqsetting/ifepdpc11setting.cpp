// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifepdpc11setting.cpp
/// @brief IFE PDPC11 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifepdpc11setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEPDPC11Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEPDPC11Setting::CalculateHWSetting(
    const PDPC11InputData*                                  pInput,
    pdpc_1_1_0::pdpc11_rgn_dataType*                        pData,
    pdpc_1_1_0::chromatix_pdpc11Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                   pOutput)
{
    BOOL   result            = TRUE;
    UINT16 pipeBitAdjFactor  = 1;
    UINT16 blackResOffset    = 0;
    UINT32 HDRExpRatio       = 1024;
    UINT32 HDRExpRatioRecip  = 256;
    UINT32 blockWidth        = 64;
    UINT32 blockHeight       = 64;
    UINT32 numX              = 0;
    UINT32 numY              = 0;
    INT32  offsetX;
    INT32  offsetY;
    UINT32 streamInWidth;
    UINT32 streamInHeight;
    UINT32 x;
    UINT32 y;
    UINT32 pixCount;

    if ((NULL != pInput) && (NULL != pData) && (NULL != pOutput))
    {
        PDPC11UnpackedField* pUnpackedField = static_cast<PDPC11UnpackedField*>(pOutput);

        streamInWidth  = pInput->imageWidth;
        streamInHeight = pInput->imageHeight;
        blockWidth     = pInput->PDAFBlockWidth;
        blockHeight    = pInput->PDAFBlockHeight;
        blackResOffset = pInput->blackLevelOffset;
        HDRExpRatio    = IQSettingUtils::FloatToQNumber(pInput->AECSensitivity, QNumber_10U);

        if (FALSE == IQSettingUtils::FEqual(pInput->AECSensitivity, 0.0f))
        {
            HDRExpRatioRecip = IQSettingUtils::FloatToQNumber((1.0f / pInput->AECSensitivity),
                                                   QNumber_8U);
        }
        else
        {
            HDRExpRatioRecip = static_cast<UINT16>(IQSettingUtils::FloatToQNumber(1.0f, QNumber_8U));
        }

        pUnpackedField->enable          =
            static_cast<UINT16>(pModuleEnable->pdpc_enable);
        pUnpackedField->pdaf_pdpc_en    =
            static_cast<UINT16>(pModuleEnable->pdpc_enable && (pInput->PDAFPixelCount > 0));
        pUnpackedField->pdaf_dsbpc_en   =
            static_cast<UINT16>(pModuleEnable->dsbpc_enable);

        pUnpackedField->pdaf_blacklevel =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(blackResOffset * pipeBitAdjFactor,
                                                            PDPC11_PDAF_BLACKLEVEL_MIN,
                                                            PDPC11_PDAF_BLACKLEVEL_MAX));

        if (FALSE == IQSettingUtils::FEqual(pInput->leftGGainWB, 0.0f))
        {
            // for PDAF
            pUnpackedField->rg_wb_gain =
                IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber((pInput->leftRGainWB / pInput->leftGGainWB),
                                            QNumber_12U),
                                            PDPC11_RG_WB_GAIN_MIN,
                                            PDPC11_RG_WB_GAIN_MAX);  // 17uQ12

            pUnpackedField->bg_wb_gain =
                IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber((pInput->leftBGainWB / pInput->leftGGainWB),
                                            QNumber_12U),
                                            PDPC11_BG_WB_GAIN_MIN,
                                            PDPC11_BG_WB_GAIN_MAX);  // 17uQ12
        }
        else
        {
            pUnpackedField->rg_wb_gain = PDPC11_RG_WB_GAIN_MIN;
            pUnpackedField->bg_wb_gain = PDPC11_BG_WB_GAIN_MIN;
        }

        if (FALSE == IQSettingUtils::FEqual(pInput->leftRGainWB, 0.0f))
        {
            pUnpackedField->gr_wb_gain =
                IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber((pInput->leftGGainWB / pInput->leftRGainWB),
                                            QNumber_12U),
                                            PDPC11_GR_WB_GAIN_MIN,
                                            PDPC11_GR_WB_GAIN_MAX);  // 17uQ12
        }
        else
        {
            pUnpackedField->gr_wb_gain = PDPC11_GR_WB_GAIN_MIN;
        }

        if (FALSE == IQSettingUtils::FEqual(pInput->leftBGainWB, 0.0f))
        {
            pUnpackedField->gb_wb_gain =
                IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber((pInput->leftGGainWB / pInput->leftBGainWB),
                                            QNumber_12U),
                                            PDPC11_GB_WB_GAIN_MIN,
                                            PDPC11_GB_WB_GAIN_MAX);  // 17uQ12
        }
        else
        {
            pUnpackedField->gb_wb_gain = PDPC11_GB_WB_GAIN_MIN;
        }
        // for DBPC
        pUnpackedField->fmax_pixel_Q6         =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->fmax_pixel_q6),
                                                            PDPC11_FMAX_PIXEL_MIN,
                                                            PDPC11_FMAX_PIXEL_MAX));    // 8uQ6
        pUnpackedField->fmin_pixel_Q6         =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->fmin_pixel_q6),
                                                            PDPC11_FMIN_PIXEL_MIN,
                                                            PDPC11_FMIN_PIXEL_MAX));    // 8uQ6
        pUnpackedField->bp_offset_g_pixel     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->bp_offset_g_pixel),
                                                            PDPC11_BP_OFFSET_PIXEL_MIN,
                                                            PDPC11_BP_OFFSET_PIXEL_MAX));    // 15u
        pUnpackedField->bp_offset_rb_pixel    =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->bp_offset_rb_pixel),
                                                            PDPC11_BP_OFFSET_PIXEL_MIN,
                                                            PDPC11_BP_OFFSET_PIXEL_MAX));    // 15u
        pUnpackedField->t2_bp_offset_g_pixel  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->t2_bp_offset_g_pixel),
                                                            PDPC11_T2_BP_OFFSET_PIXEL_MIN,
                                                            PDPC11_T2_BP_OFFSET_PIXEL_MAX)); // 15u
        pUnpackedField->t2_bp_offset_rb_pixel =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->t2_bp_offset_rb_pixel),
                                                            PDPC11_T2_BP_OFFSET_PIXEL_MIN,
                                                            PDPC11_T2_BP_OFFSET_PIXEL_MAX)); // 15u

        if (HDR_ISP_ON == pInput->HDRMode)
        {
            pUnpackedField->pdaf_HDR_selection = 0x4 + pInput->zzHDRPattern;
        }
        else
        {
            pUnpackedField->pdaf_HDR_selection = 0;
        }

        pUnpackedField->pdaf_zzHDR_first_rb_exp  = pInput->zzHDRFirstRBEXP;

        // For PDPC
        pUnpackedField->pdaf_HDR_selection       = IQSettingUtils::ClampUINT16(pUnpackedField->pdaf_HDR_selection,
                                                                               PDPC11_PDAF_HDR_SELECTION_MIN,
                                                                               PDPC11_PDAF_HDR_SELECTION_MAX);
        pUnpackedField->pdaf_zzHDR_first_rb_exp  = IQSettingUtils::ClampUINT16(pUnpackedField->pdaf_zzHDR_first_rb_exp, 0, 1);
        pUnpackedField->pdaf_hdr_exp_ratio       = static_cast<UINT16>(IQSettingUtils::ClampUINT32(HDRExpRatio,
                                                                               PDPC11_PDAF_HDR_EXP_RATIO_MIN,
                                                                               PDPC11_PDAF_HDR_EXP_RATIO_MAX));
        pUnpackedField->pdaf_hdr_exp_ratio_recip = static_cast<UINT16>(IQSettingUtils::ClampUINT32(HDRExpRatioRecip,
                                                                               PDPC11_PDAF_HDR_EXP_RATIO_RECIP_MIN,
                                                                               PDPC11_PDAF_HDR_EXP_RATIO_RECIP_MAX));
        pUnpackedField->pdaf_global_offset_x     = IQSettingUtils::ClampUINT16(pInput->PDAFGlobaloffsetX,
                                                                               PDPC11_PDAF_MIN_GLOBAL_OFFSET,
                                                                               PDPC11_PDAF_MAX_GLOBAL_OFFSET);
        pUnpackedField->pdaf_global_offset_y     = IQSettingUtils::ClampUINT16(pInput->PDAFGlobaloffsetY,
                                                                               PDPC11_PDAF_MIN_GLOBAL_OFFSET,
                                                                               PDPC11_PDAF_MAX_GLOBAL_OFFSET);
        pUnpackedField->pdaf_x_end               = static_cast<UINT16>(IQSettingUtils::ClampUINT32(streamInWidth,
                                                                                                   PDPC11_PDAF_X_END_MIN,
                                                                                                   PDPC11_PDAF_X_END_MAX));
        pUnpackedField->pdaf_y_end               = static_cast<UINT16>(IQSettingUtils::ClampUINT32(streamInHeight,
                                                                                                   PDPC11_PDAF_Y_END_MIN,
                                                                                                   PDPC11_PDAF_Y_END_MAX));


        IQSettingUtils::Memset(pUnpackedField->PDAF_PD_Mask, 0, DMIRAM_PDAF_LUT_LENGTH * sizeof(UINT32));

        if (pUnpackedField->pdaf_pdpc_en)
        {
            numX = DMIRAM_PDAF_LUT_LENGTH / blockWidth;
            numY = DMIRAM_PDAF_LUT_LENGTH / blockHeight;
            for (y = 0; y < numY; y++)
            {
                if (y < 1) // block_height x 64
                {
                    for (x = 0; x < numX; x++)
                    {
                        for (pixCount = 0; pixCount < pInput->PDAFPixelCount; pixCount++)
                        {
                            offsetX =
                                pInput->PDAFPixelCoords[pixCount].PDXCoordinate - pUnpackedField->pdaf_global_offset_x +
                                    (blockWidth * x);
                            offsetY = pInput->PDAFPixelCoords[pixCount].PDYCoordinate - pUnpackedField->pdaf_global_offset_y;

                            offsetX = IQSettingUtils::ClampINT32(offsetX, 0, (DMIRAM_PDAF_LUT_LENGTH - 1));
                            offsetY = IQSettingUtils::ClampINT32(offsetY, 0, (DMIRAM_PDAF_LUT_LENGTH - 1));

                            pUnpackedField->PDAF_PD_Mask[offsetY] =
                                pUnpackedField->PDAF_PD_Mask[offsetY] | (1 << (PDPCBitMask - static_cast<INT32>(offsetX / 2)));
                        }
                    }
                }
                else // copy from the block above
                {
                    for (pixCount = 0; pixCount < pInput->PDAFPixelCount; pixCount++)
                    {
                        offsetY =
                            pInput->PDAFPixelCoords[pixCount].PDYCoordinate - pUnpackedField->pdaf_global_offset_y +
                                (blockHeight * y);

                        pUnpackedField->PDAF_PD_Mask[offsetY] = pUnpackedField->PDAF_PD_Mask[offsetY - blockHeight];
                    }
                }
            }
        }
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }
    return result;
}
