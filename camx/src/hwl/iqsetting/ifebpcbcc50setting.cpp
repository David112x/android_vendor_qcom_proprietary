// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifebpcbcc50setting.cpp
/// @brief IFE BPCBCC50 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "ifebpcbcc50setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEBPCBCC50Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEBPCBCC50Setting::CalculateHWSetting(
    const BPCBCC50InputData*              pInput,
    bpcbcc_5_0_0::bpcbcc50_rgn_dataType*  pData,
    globalelements::enable_flag_type      moduleEnable,
    VOID*                                 pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput)         &&
        (NULL != pData)          &&
        (NULL != pOutput)        &&
        (NULL != pInput->pChromatix))
    {
        BPCBCC50UnpackedField* pUnpackedField = static_cast<BPCBCC50UnpackedField*>(pOutput);

        pUnpackedField->enable                      = moduleEnable;

        pUnpackedField->hotPixelCorrectionDisable   = IQSettingUtils::ClampUINT32(
                                                          static_cast<UINT32>(pData->hot_pixel_correction_disable),
                                                          BPCBCC50_PIXEL_CORRECTION_DISABLE_MIN,
                                                          BPCBCC50_PIXEL_CORRECTION_DISABLE_MAX);
        pUnpackedField->coldPixelCorrectionDisable  = IQSettingUtils::ClampUINT32(
                                                          static_cast<UINT32>(pData->cold_pixel_correction_disable),
                                                          BPCBCC50_PIXEL_CORRECTION_DISABLE_MIN,
                                                          BPCBCC50_PIXEL_CORRECTION_DISABLE_MAX);
        pUnpackedField->sameChannelRecovery         = IQSettingUtils::ClampUINT32(
                                                          static_cast<UINT32>(pData->same_channel_recovery),
                                                          BPCBCC50_SAME_CHANNEL_RECOVERY_MIN,
                                                          BPCBCC50_SAME_CHANNEL_RECOVERY_MAX);
        pUnpackedField->fmax                        = IQSettingUtils::ClampUINT32(
                                                          IQSettingUtils::RoundFLOAT(pData->fmax),
                                                          BPCBCC50_FMAX_MIN,
                                                          BPCBCC50_FMAX_MAX);
        pUnpackedField->fmin                        = IQSettingUtils::ClampUINT32(
                                                          IQSettingUtils::RoundFLOAT(pData->fmin),
                                                          BPCBCC50_FMIN_MIN,
                                                          BPCBCC50_FMIN_MAX);
        pUnpackedField->bpcOffset                   = IQSettingUtils::ClampUINT32(
                                                          IQSettingUtils::RoundFLOAT(pData->bpc_offset),
                                                          BPCBCC50_BPC_OFFSET_MIN,
                                                          BPCBCC50_BPC_OFFSET_MAX);
        pUnpackedField->bccOffset                   = IQSettingUtils::ClampUINT32(
                                                          IQSettingUtils::RoundFLOAT(pData->bcc_offset),
                                                          BPCBCC50_BCC_OFFSET_MIN,
                                                          BPCBCC50_BCC_OFFSET_MAX);
        pUnpackedField->correctionThreshold         = IQSettingUtils::ClampUINT32(
                                                          IQSettingUtils::RoundFLOAT(pData->correction_threshold),
                                                          BPCBCC50_CORRECTION_THRESHOLD_MIN,
                                                          BPCBCC50_CORRECTION_THRESHOLD_MAX);
        pUnpackedField->black_level                 = IQSettingUtils::ClampUINT32(
                                                          IQSettingUtils::RoundFLOAT(pInput->blackLevelOffset *
                                                              pInput->nonHdrMultFactor),
                                                          BPCBCC50_BLACKLEVEL_MIN,
                                                          BPCBCC50_BLACKLEVEL_MAX); // 12u

        if ((FALSE == IQSettingUtils::FEqual(pInput->leftRGainWB, 0.0f)) &&
            (FALSE == IQSettingUtils::FEqual(pInput->leftGGainWB, 0.0f)) &&
            (FALSE == IQSettingUtils::FEqual(pInput->leftBGainWB, 0.0f)))
        {
            pUnpackedField->bg_wb_gain_ratio            = IQSettingUtils::ClampUINT32(
                                                              IQSettingUtils::FloatToQNumber(
                                                                  (pInput->leftBGainWB / pInput->leftGGainWB),
                                                                  QNumber_12U),
                                                              BPCBCC50_BG_WB_GAIN_RATIO_MIN,
                                                              BPCBCC50_BG_WB_GAIN_RATIO_MAX);
            pUnpackedField->gr_wb_gain_ratio            = IQSettingUtils::ClampUINT32(
                                                              IQSettingUtils::FloatToQNumber(
                                                                  (pInput->leftGGainWB / pInput->leftRGainWB),
                                                                  QNumber_12U),
                                                              BPCBCC50_GR_WB_GAIN_RATIO_MIN,
                                                              BPCBCC50_GR_WB_GAIN_RATIO_MAX);
            pUnpackedField->gb_wb_gain_ratio            = IQSettingUtils::ClampUINT32(
                                                              IQSettingUtils::FloatToQNumber(
                                                                  (pInput->leftGGainWB / pInput->leftBGainWB),
                                                                  QNumber_12U),
                                                              BPCBCC50_GB_WB_GAIN_RATIO_MIN,
                                                              BPCBCC50_GB_WB_GAIN_RATIO_MAX);
            pUnpackedField->rg_wb_gain_ratio            = IQSettingUtils::ClampUINT32(
                                                              IQSettingUtils::FloatToQNumber(
                                                                  (pInput->leftRGainWB / pInput->leftGGainWB),
                                                                  QNumber_12U),
                                                              BPCBCC50_RG_WB_GAIN_RATIO_MIN,
                                                              BPCBCC50_RG_WB_GAIN_RATIO_MAX);
        }
        else
        {
            /// @todo (CAMX-1812) Need to add logging for Common library
            result = FALSE;
        }

        pUnpackedField->hot_bad_pix_cnt  = 0;
        pUnpackedField->cold_bad_pix_cnt = 0;
    }
    else
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
        result = FALSE;
    }
    return result;
}
