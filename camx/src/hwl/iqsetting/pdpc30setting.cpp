// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  pdpc30setting.cpp
/// @brief pdpc30 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pdpc30setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PDPC30Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL PDPC30Setting::CalculateHWSetting(
    const PDPC30IQInput*                                    pInput,
    pdpc_3_0_0::pdpc30_rgn_dataType*                        pData,
    pdpc_3_0_0::chromatix_pdpc30Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                   pOutput)
{
    BOOL   result           = TRUE;
    UINT32 blackResOffset;
    UINT32 blockWidth;
    UINT32 blockHeight;
    UINT16 HDRExpRatio;
    UINT16 HDRExpRatioReciprocal;
    UINT32 numX = 0;
    UINT32 numY = 0;
    UINT32 streamInWidth;
    UINT32 streamInHeight;
    INT32  offsetX;
    INT32  offsetY;
    UINT32 x;
    UINT32 y;
    UINT32 pixelCount;
    UINT16 noiseStdLut[PDPC30_NOISESTD_LENGTH + 1];

    if ((NULL != pInput)           &&
        (NULL != pData)            &&
        (NULL != pModuleEnable)    &&
        (NULL != pOutput))
    {
        PDPC30UnpackedField* pUnpackedField = static_cast<PDPC30UnpackedField*>(pOutput);

        streamInWidth  = pInput->imageWidth;
        streamInHeight = pInput->imageHeight;
        blockWidth     = pInput->PDAFBlockWidth;
        blockHeight    = pInput->PDAFBlockHeight;
        blackResOffset = static_cast<UINT32>(pInput->blackLevelOffset);
        HDRExpRatio    = static_cast<UINT16>(IQSettingUtils::FloatToQNumber(pInput->AECSensitivity, QNumber_8U));

        if (FALSE == IQSettingUtils::FEqual(pInput->AECSensitivity, 0.0f))
        {
            HDRExpRatioReciprocal               = static_cast<UINT16>(IQSettingUtils::FloatToQNumber(
                                                                     (1.0f / pInput->AECSensitivity), QNumber_12U));
        }
        else
        {
            HDRExpRatioReciprocal               = static_cast<UINT16>(IQSettingUtils::FloatToQNumber(1.0f, QNumber_12U));
        }

        pUnpackedField->enable                  = static_cast<UINT16>(pInput->moduleEnable);
        pUnpackedField->PDAFPDPCEnable          = static_cast<UINT16>((pInput->pdpcEnable) &&
                                                                      (pInput->PDAFPixelCount > 0));
        pUnpackedField->PDAFBPCEnable           = static_cast<UINT16>(pInput->bpcEnable);
        pUnpackedField->PDAFGICEnable           = static_cast<UINT16>(pInput->gicEnable);
        pUnpackedField->LUTBankSelection        = pInput->LUTBankSel;
        pUnpackedField->bayerPattern            = static_cast<UINT16>(pInput->bayerPattern);
        pUnpackedField->blackLevel              = static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                                                                        (blackResOffset),
                                                                        PDPC30_BLACK_LEVEL_MIN,
                                                                        PDPC30_BLACK_LEVEL_MAX));
        // for PDAF pixels
        if (FALSE == IQSettingUtils::FEqual(pInput->leftGGainWB, 0.0f))
        {
            // for PDAF
            pUnpackedField->rgWbGain4096        = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                        (pInput->leftRGainWB / pInput->leftGGainWB), QNumber_12U),
                                                        PDPC30_RGB_WB_GAIN_MIN,
                                                        PDPC30_RGB_WB_GAIN_MAX);  // 17uQ12
            pUnpackedField->bgWbGain4096        = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                        (pInput->leftBGainWB / pInput->leftGGainWB), QNumber_12U),
                                                        PDPC30_RGB_WB_GAIN_MIN,
                                                        PDPC30_RGB_WB_GAIN_MAX);  // 17uQ12
        }
        else
        {
            pUnpackedField->rgWbGain4096        = PDPC30_RGB_WB_GAIN_MIN;
            pUnpackedField->bgWbGain4096        = PDPC30_RGB_WB_GAIN_MIN;
        }

        if (FALSE == IQSettingUtils::FEqual(pInput->leftRGainWB, 0.0f))
        {
            pUnpackedField->grWbGain4096        = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                        (pInput->leftGGainWB / pInput->leftRGainWB), QNumber_12U),
                                                        PDPC30_RGB_WB_GAIN_MIN,
                                                        PDPC30_RGB_WB_GAIN_MAX);  // 17uQ12
        }
        else
        {
            pUnpackedField->grWbGain4096        = PDPC30_RGB_WB_GAIN_MIN;
        }

        if (FALSE == IQSettingUtils::FEqual(pInput->leftBGainWB, 0.0f))
        {
            pUnpackedField->gbWbGain4096        = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                        (pInput->leftGGainWB / pInput->leftBGainWB), QNumber_12U),
                                                        PDPC30_RGB_WB_GAIN_MIN,
                                                        PDPC30_RGB_WB_GAIN_MAX);  // 17uQ12
        }
        else
        {
            pUnpackedField->gbWbGain4096        = PDPC30_RGB_WB_GAIN_MIN;
        }

        // for DBPC
        pUnpackedField->fmaxFlat                = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->fmax_flat)),
                                                        PDPC30_FMAXPIXEL_MIN,
                                                        PDPC30_FMAXPIXEL_MAX);
        pUnpackedField->fminFlat                = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->fmin_flat)),
                                                        PDPC30_FMINPIXEL_MIN,
                                                        PDPC30_FMINPIXEL_MAX);
        pUnpackedField->fmax                    = IQSettingUtils::ClampUINT32(
                                                        static_cast<UINT32>(pData->fmax),
                                                        PDPC30_FMAXPIXEL_MIN,
                                                        PDPC30_FMAXPIXEL_MAX);
        pUnpackedField->fmin                    = IQSettingUtils::ClampUINT32(
                                                        static_cast<UINT32>(pData->fmin),
                                                        PDPC30_FMINPIXEL_MIN,
                                                        PDPC30_FMINPIXEL_MAX);
        pUnpackedField->bpcOffset               = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->bpc_offset)),
                                                        PDPC30_BPC_OFFSET_MIN,
                                                        PDPC30_BPC_OFFSET_MAX);
        pUnpackedField->bccOffset               = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->bcc_offset)),
                                                        PDPC30_BCC_OFFSET_MIN,
                                                        PDPC30_BCC_OFFSET_MAX);

        pUnpackedField->bpcOffsetFlat           = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->bpc_offset_flat)),
                                                        PDPD30_BPC_OFFSET_FLAT_MIN,
                                                        PDPD30_BPC_OFFSET_FLAT_MAX);

        pUnpackedField->bccOffsetFlat           = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->bcc_offset_flat)),
                                                        PDPC30_BCC_OFFSET_FLAT_MIN,
                                                        PDPC30_BCC_OFFSET_FLAT_MAX);

        pUnpackedField->flatDetectionEn         = static_cast<UINT32>(pData->flat_detection_en);

        pUnpackedField->flatThRecip             = static_cast<UINT16>(pData->flat_th_recip);

        pUnpackedField->useSameChannelOnly      = static_cast<UINT16>(pData->use_same_channel_only);
        pUnpackedField->singleBPCOnly           = static_cast<UINT16>(pData->single_bpc_only);

        pUnpackedField->directionalBPCEnable    = static_cast<UINT16>(pData->directional_bpc_en &&
                                                  pInput->directionalBPCEnable);
        pUnpackedField->dirTk                   = static_cast<UINT16>(pData->dir_tk);
        pUnpackedField->dirOffset               = static_cast<UINT16>(pData->dir_offset);
        pUnpackedField->fmaxGIC                 = static_cast<UINT16>(pData->fmax_gic);
        pUnpackedField->bpcOffsetGIC            = static_cast<UINT16>(pData->bpc_offset_gic);
        pUnpackedField->gicThinLineNoiseOffset  = static_cast<UINT16>(pData->gic_thin_line_noise_offset);
        pUnpackedField->gicFilterStrength       = static_cast<UINT16>(pData->gic_filter_strength);

        pUnpackedField->saturationThreshold     = IQSettingUtils::ClampUINT16(
                                                  static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->saturation_threshold)),
                                                  PDPC30_SATURATION_THRES_MIN,
                                                  PDPC30_SATURATION_THRES_MAX);

        for (UINT32 i = 0; i < PDPC30_NOISESTD_LENGTH + 1; i++)
        {
            noiseStdLut[i] = static_cast<UINT16>(pData->noise_std_lut_tab.noise_std_lut[i]);
        }

        for (UINT32 i = 0; i < PDPC30_NOISESTD_LENGTH; i++)
        {
            UINT32 diff0 = noiseStdLut[i + 1] - noiseStdLut[i];
            pUnpackedField->noiseStdLUTLevel0[0][i] = pUnpackedField->noiseStdLUTLevel0[1][i] =
                                                        static_cast<UINT32>((diff0 << PDPC30_BIT_WIDTH) |
                                                        (noiseStdLut[i] & ((1 << PDPC30_BIT_WIDTH) - 1)));
        }

        // For HDR
        if (HDR30_ISP_OFF == static_cast<UINT8>(pInput->zzHDRModeEnable))
        {
            pUnpackedField->PDAFHDRSelection = 0;
        }
        else
        {
            pUnpackedField->PDAFHDRSelection = pInput->ZZHDRPattern + 4;
        }

        pUnpackedField->PDAFzzHDRFirstrbExposure   = pInput->zzHDRFirstRBEXP;
        pUnpackedField->PDAFHDRSelection           = IQSettingUtils::ClampUINT16(pUnpackedField->PDAFHDRSelection,
                                                        PDPC30_PDAF_HDR_SELECTION_MIN,
                                                        PDPC30_PDAF_HDR_SELECTION_MAX);
        pUnpackedField->PDAFzzHDRFirstrbExposure  = IQSettingUtils::ClampUINT16(pUnpackedField->PDAFzzHDRFirstrbExposure, 0, 1);
        pUnpackedField->PDAFHDRExposureRatio       = IQSettingUtils::ClampUINT16(HDRExpRatio,
                                                        PDPC30_HDR_EXP_RATIO_MIN,
                                                        PDPC30_HDR_EXP_RATIO_MAX);
        pUnpackedField->PDAFHDRExposureRatioRecip  = IQSettingUtils::ClampUINT16(HDRExpRatioReciprocal,
                                                        PDPC30_HDR_EXP_RATIO__RECIP_MIN,
                                                        PDPC30_HDR_EXP_RATIO_RECIP_MAX);
        // For PDAF pixels
        pUnpackedField->PDAFGlobalOffsetX          = IQSettingUtils::ClampUINT16(pInput->PDAFGlobaloffsetX,
                                                        PDPC30_GLOBAL_OFFSET_X_MIN,
                                                        PDPC30_GLOBAL_OFFSET_X_MAX);
        pUnpackedField->PDAFGlobalOffsetY          = IQSettingUtils::ClampUINT16(pInput->PDAFGlobaloffsetY,
                                                        PDPC30_GLOBAL_OFFSET_Y_MIN,
                                                        PDPC30_GLOBAL_OFFSET_Y_MAX);
        pUnpackedField->PDAFXend                   = static_cast<UINT16>(IQSettingUtils::ClampUINT32(streamInWidth,
                                                        PDPC30_PDAF_X_END_MIN,
                                                        PDPC30_PDAF_X_END_MAX));
        pUnpackedField->PDAFYend                   = static_cast<UINT16>(IQSettingUtils::ClampUINT32(streamInHeight,
                                                        PDPC30_PDAF_Y_END_MIN,
                                                        PDPC30_PDAF_Y_END_MAX));
        pUnpackedField->PDAFTableXOffset           = 0;
        pUnpackedField->PDAFTableYOffset           = 0;

        IQSettingUtils::Memset(pUnpackedField->PDAFPDMask, 0, 2 * PDPC30DMILengthDword * sizeof(UINT64));

        if (pUnpackedField->PDAFPDPCEnable)
        {

            if (0 != blockWidth)
            {
                numX = BPSPDPC20DMILengthDword / blockWidth;
            }

            if (0 != blockHeight)
            {
                numY = BPSPDPC20DMILengthDword / blockHeight;
            }

            for (y = 0; y < numY; y++)
            {
                if (y < 1) // block_height x 64
                {
                    for (x = 0; x < numX; x++)
                    {
                        for (pixelCount = 0; pixelCount < pInput->PDAFPixelCount; pixelCount++)
                        {
                            offsetX = pInput->PDAFPixelCoords[pixelCount].PDXCoordinate -
                                      pUnpackedField->PDAFGlobalOffsetX + blockWidth * x;
                            offsetY = pInput->PDAFPixelCoords[pixelCount].PDYCoordinate - pUnpackedField->PDAFGlobalOffsetY;
                            offsetX = IQSettingUtils::ClampINT32(offsetX, 0, (2 * PDPC30DMILengthDword - 1));
                            offsetY = IQSettingUtils::ClampINT32(offsetY, 0, (2 * PDPC30DMILengthDword - 1));
                            pUnpackedField->PDAFPDMask[pUnpackedField->LUTBankSelection][offsetY] =
                                pUnpackedField->PDAFPDMask[pUnpackedField->LUTBankSelection][offsetY] |
                                (static_cast<UINT64>(1) << (static_cast<UINT64>(63) - static_cast<UINT64>((offsetX))));
                        }
                    }
                }
                else // copy from the block above
                {
                    for (pixelCount = 0; pixelCount < pInput->PDAFPixelCount; pixelCount++)
                    {
                        offsetY = pInput->PDAFPixelCoords[pixelCount].PDYCoordinate - pUnpackedField->PDAFGlobalOffsetY +
                                  blockHeight * y;
                        pUnpackedField->PDAFPDMask[pUnpackedField->LUTBankSelection][offsetY] =
                            pUnpackedField->PDAFPDMask[pUnpackedField->LUTBankSelection][offsetY - blockHeight];
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
