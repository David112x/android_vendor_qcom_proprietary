// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bpspdpc20setting.cpp
/// @brief BPS pdpc20 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bpspdpc20setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSPDPC20Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSPDPC20Setting::CalculateHWSetting(
    const PDPC20IQInput*                                    pInput,
    pdpc_2_0_0::pdpc20_rgn_dataType*                        pData,
    pdpc_2_0_0::chromatix_pdpc20Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                   pOutput)
{
    BOOL   result           = TRUE;
    UINT16 pipeBitAdjFactor = 1;
    UINT16 blackResOffset   = 0;
    UINT32 blockWidth       = 64;
    UINT32 blockHeight      = 64;
    UINT16 HDRExpRatio      = 1024;
    UINT16 HDRExpRatioRecip = 256;
    UINT32 numX             = 0;
    UINT32 numY             = 0;
    UINT32 streamInWidth;
    UINT32 streamInHeight;
    INT32  offsetX;
    INT32  offsetY;
    UINT32 x;
    UINT32 y;
    UINT32 pixelCount;

    if ((NULL != pInput)           &&
        (NULL != pData)            &&
        (NULL != pModuleEnable)    &&
        (NULL != pOutput))
    {
        PDPC20UnpackedField* pUnpackedField = static_cast<PDPC20UnpackedField*>(pOutput);

        streamInWidth  = pInput->imageWidth;
        streamInHeight = pInput->imageHeight;
        blockWidth     = pInput->PDAFBlockWidth;
        blockHeight    = pInput->PDAFBlockHeight;
        blackResOffset = static_cast<UINT16>(pInput->blackLevelOffset);
        HDRExpRatio    = static_cast<UINT16>(IQSettingUtils::FloatToQNumber(pInput->AECSensitivity, QNumber_10U));

        if (FALSE == IQSettingUtils::FEqual(pInput->AECSensitivity, 0.0f))
        {
            HDRExpRatioRecip                       = static_cast<UINT16>(IQSettingUtils::FloatToQNumber(
                                                        (1.0f / pInput->AECSensitivity), QNumber_8U));
        }
        else
        {
            HDRExpRatioRecip                       = static_cast<UINT16>(IQSettingUtils::FloatToQNumber(1.0f, QNumber_8U));
        }

        pUnpackedField->enable                     = static_cast<UINT16>(pModuleEnable->pdpc_enable)  ||
                                                     static_cast<UINT16>(pModuleEnable->dsbpc_enable);
        pUnpackedField->PDAFPDPCEnable             = static_cast<UINT16>((pModuleEnable->pdpc_enable) &&
                                                                         (pInput->PDAFPixelCount > 0));
        pUnpackedField->PDAFDSBPCEnable            = static_cast<UINT16>(pModuleEnable->dsbpc_enable);
        pUnpackedField->LUTBankSelection           = pInput->LUTBankSel;
        pUnpackedField->bayerPattern               = static_cast<UINT16>(pInput->bayerPattern);
        pUnpackedField->blackLevel                 = static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                                                        (blackResOffset * pipeBitAdjFactor),
                                                        PDPC20_BLACK_LEVEL_MIN,
                                                        PDPC20_BLACK_LEVEL_MAX));
        // for PDAF pixels
        if (FALSE == IQSettingUtils::FEqual(pInput->leftGGainWB, 0.0f))
        {
            // for PDAF
            pUnpackedField->rgWbGain4096           = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                        (pInput->leftRGainWB / pInput->leftGGainWB), QNumber_12U),
                                                        PDPC20_RGB_WB_GAIN_MIN,
                                                        PDPC20_RGB_WB_GAIN_MAX);  // 17uQ12
            pUnpackedField->bgWbGain4096           = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                        (pInput->leftBGainWB / pInput->leftGGainWB), QNumber_12U),
                                                        PDPC20_RGB_WB_GAIN_MIN,
                                                        PDPC20_RGB_WB_GAIN_MAX);  // 17uQ12
        }
        else
        {
            pUnpackedField->rgWbGain4096           = PDPC20_RGB_WB_GAIN_MIN;
            pUnpackedField->bgWbGain4096           = PDPC20_RGB_WB_GAIN_MIN;
        }

        if (FALSE == IQSettingUtils::FEqual(pInput->leftRGainWB, 0.0f))
        {
            pUnpackedField->grWbGain4096           = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                        (pInput->leftGGainWB / pInput->leftRGainWB), QNumber_12U),
                                                        PDPC20_RGB_WB_GAIN_MIN,
                                                        PDPC20_RGB_WB_GAIN_MAX);  // 17uQ12
        }
        else
        {
            pUnpackedField->grWbGain4096           = PDPC20_RGB_WB_GAIN_MIN;
        }

        if (FALSE == IQSettingUtils::FEqual(pInput->leftBGainWB, 0.0f))
        {
            pUnpackedField->gbWbGain4096           = IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                        (pInput->leftGGainWB / pInput->leftBGainWB), QNumber_12U),
                                                        PDPC20_RGB_WB_GAIN_MIN,
                                                        PDPC20_RGB_WB_GAIN_MAX);  // 17uQ12
        }
        else
        {
            pUnpackedField->gbWbGain4096           = PDPC20_RGB_WB_GAIN_MIN;
        }

        // for DBPC
        pUnpackedField->fmaxPixelQ6                = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->fmax_pixel_q6)),
                                                        PDPC20_FMAXPIXEL_MIN,
                                                        PDPC20_FMAXPIXEL_MAX);
        pUnpackedField->fminPixelQ6                = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->fmin_pixel_q6)),
                                                        PDPC20_FMINPIXEL_MIN,
                                                        PDPC20_FMINPIXEL_MAX);
        pUnpackedField->hotPixelCorrectionDisable  = IQSettingUtils::ClampUINT32(
                                                        static_cast<UINT32>(pData->hot_pixel_correction_disable),
                                                        PDPC20_HOT_PIXEL_CORRECTION_DISABLE_MIN,
                                                        PDPC20_HOT_PIXEL_CORRECTION_DISABLE_MAX);
        pUnpackedField->coldPixelCorrectionDisable = IQSettingUtils::ClampUINT32(
                                                        static_cast<UINT32>(pData->cold_pixel_correction_disable),
                                                        PDPC20_COLD_PIXEL_CORRECTION_DISABLE_MIN,
                                                        PDPC20_COLD_PIXEL_CORRECTION_DISABLE_MAX);
        pUnpackedField->bpcOffset                  = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->bpc_offset)),
                                                        PDPC20_BPC_OFFSET_MIN,
                                                        PDPC20_BPC_OFFSET_MAX);
        pUnpackedField->bccOffset                  = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->bcc_offset)),
                                                        PDPC20_BCC_OFFSET_MIN,
                                                        PDPC20_BCC_OFFSET_MAX);
        pUnpackedField->bpcOffsetT2                = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->bpc_offset_t2)),
                                                        PDPC20_BPC_OFFSET_T2_MIN,
                                                        PDPC20_BPC_OFFSET_T2_MAX);
        pUnpackedField->bccOffsetT2                = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(IQSettingUtils::RoundFLOAT(pData->bcc_offset_t2)),
                                                        PDPC20_BCC_OFFSET_T2_MIN,
                                                        PDPC20_BCC_OFFSET_T2_MAX);
        pUnpackedField->correctionThreshold        = IQSettingUtils::ClampUINT32(
                                                        static_cast<UINT32>(IQSettingUtils::RoundFLOAT(
                                                            pData->correction_threshold)),
                                                        PDPC20_CORRECTION_THRES_MIN,
                                                        PDPC20_BCC_CORRECTION_THRES_MAX);
        pUnpackedField->removeAlongEdge            = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(pData->remove_along_edge),
                                                        PDPC20_REMOVE_ALONG_EDGE_MIN,
                                                        PDPC20_REMOVE_ALONG_EDGE_MAX);
        pUnpackedField->usingCrossChannel          = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(pData->using_cross_channel),
                                                        PDPC20_USING_CROSS_CHANNEL_MIN,
                                                        PDPC20_USING_CROSS_CHANNEL_MAX);
        pUnpackedField->saturationThreshold        = IQSettingUtils::ClampUINT16(
                                                        static_cast<UINT16>(
                                                            IQSettingUtils::RoundFLOAT(pData->saturation_threshold)),
                                                        PDPC20_SATURATION_THRES_MIN,
                                                        PDPC20_SATURATION_THRES_MAX);

        // For HDR
        if (BPS_HDR_ISP_OFF == static_cast<UINT8>(pInput->zzHDRModeEnable))
        {
            pUnpackedField->PDAFHDRSelection = 0;
        }
        else
        {
            // case HDR_ZZREC_Pattern == 0: PDAFHDRSelection = (hdr_first_field == 0) ? 1: 2;
            // HDR_ZZREC_Pattern is 1 (hardcoded in HDR22)
            // case ZZHDRPattern == 0: PDAFHDRSelection = 4;
            // case ZZHDRPattern == 1: PDAFHDRSelection = 5;
            // case ZZHDRPattern == 2: PDAFHDRSelection = 6;
            // case ZZHDRPattern == 3: PDAFHDRSelection = 7;
            pUnpackedField->PDAFHDRSelection = pInput->ZZHDRPattern + 4;
        }

        pUnpackedField->PDAFHDRSelection           = IQSettingUtils::ClampUINT16(pUnpackedField->PDAFHDRSelection,
                                                        PDPC20_PDAF_HDR_SELECTION_MIN,
                                                        PDPC20_PDAF_HDR_SELECTION_MAX);
        pUnpackedField->PDAFzzHDRFirstrbExposure   = IQSettingUtils::ClampUINT16(pInput->zzHDRFirstRBEXP, 0, 1);

        pUnpackedField->PDAFHDRExposureRatio       = IQSettingUtils::ClampUINT16(HDRExpRatio,
                                                        PDPC20_HDR_EXP_RATIO_MIN,
                                                        PDPC20_HDR_EXP_RATIO_MAX);
        pUnpackedField->PDAFHDRExposureRatioRecip  = IQSettingUtils::ClampUINT16(HDRExpRatioRecip,
                                                        PDPC20_HDR_EXP_RATIO__RECIP_MIN,
                                                        PDPC20_HDR_EXP_RATIO_RECIP_MAX);
        // For PDAF pixels
        pUnpackedField->PDAFGlobalOffsetX          = IQSettingUtils::ClampUINT16(pInput->PDAFGlobaloffsetX,
                                                        PDPC20_GLOBAL_OFFSET_X_MIN,
                                                        PDPC20_GLOBAL_OFFSET_X_MAX);
        pUnpackedField->PDAFGlobalOffsetY          = IQSettingUtils::ClampUINT16(pInput->PDAFGlobaloffsetY,
                                                        PDPC20_GLOBAL_OFFSET_Y_MIN,
                                                        PDPC20_GLOBAL_OFFSET_Y_MAX);
        pUnpackedField->PDAFXend                   = static_cast<UINT16>(IQSettingUtils::ClampUINT32(streamInWidth,
                                                        PDPC20_PDAF_X_END_MIN,
                                                        PDPC20_PDAF_X_END_MAX));
        pUnpackedField->PDAFYend                   = static_cast<UINT16>(IQSettingUtils::ClampUINT32(streamInHeight,
                                                        PDPC20_PDAF_Y_END_MIN,
                                                        PDPC20_PDAF_Y_END_MAX));
        pUnpackedField->PDAFTableXOffset           = 0;
        pUnpackedField->PDAFTableYOffset           = 0;

        if (0 != blockWidth)
        {
            numX = BPSPDPC20DMILengthDword / blockWidth;
        }

        if (0 != blockHeight)
        {
            numY = BPSPDPC20DMILengthDword / blockHeight;
        }

        IQSettingUtils::Memset(pUnpackedField->PDAFPDMask, 0, BPSPDPC20DMILengthDword * sizeof(UINT32));

        if (pUnpackedField->PDAFPDPCEnable)
        {
            for (y = 0; y < numY; y++)
            {
                if (y < 1) // block_height x 64
                {
                    for (x = 0; x < numX; x++)
                    {
                        for (pixelCount = 0; pixelCount < pInput->PDAFPixelCount; pixelCount++)
                        {
                            offsetX = pInput->PDAFPixelCoords[pixelCount].PDXCoordinate - pUnpackedField->PDAFGlobalOffsetX +
                                        blockWidth * x;
                            offsetY = pInput->PDAFPixelCoords[pixelCount].PDYCoordinate - pUnpackedField->PDAFGlobalOffsetY;

                            offsetX = IQSettingUtils::ClampINT32(offsetX, 0, (BPSPDPC20DMILengthDword - 1));
                            offsetY = IQSettingUtils::ClampINT32(offsetY, 0, (BPSPDPC20DMILengthDword - 1));

                            pUnpackedField->PDAFPDMask[pUnpackedField->LUTBankSelection][offsetY] =
                            pUnpackedField->PDAFPDMask[pUnpackedField->LUTBankSelection][offsetY] |
                                          (1 << (PDPCBitMask - static_cast<INT32> (offsetX / 2)));
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
