// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cv12setting.cpp
/// @brief IPE CV12 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "cv12setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CV12Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CV12Setting::CalculateHWSetting(
    const CV12InputData*                                pInput,
    cv_1_2_0::cv12_rgn_dataType*                        pData,
    cv_1_2_0::chromatix_cv12Type::enable_sectionStruct* pModuleEnable,
    VOID*                                               pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        CV12UnpackedField* pUnpackedField = static_cast<CV12UnpackedField*>(pOutput);

        pUnpackedField->enable  = static_cast<UINT16>(pModuleEnable->cv_enable);

        pUnpackedField->rToY    = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->r_to_y * pInput->stretch_factor,
                                      CV12_RGB2Y_DATA_Q_FACTOR),
                                      CV12_RGB2Y_RTOY_MIN,
                                      CV12_RGB2Y_RTOY_MAX));
        pUnpackedField->gToY    = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->g_to_y * pInput->stretch_factor,
                                      CV12_RGB2Y_DATA_Q_FACTOR),
                                      CV12_RGB2Y_GTOY_MIN,
                                      CV12_RGB2Y_GTOY_MAX));
        pUnpackedField->bToY    = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->b_to_y * pInput->stretch_factor,
                                      CV12_RGB2Y_DATA_Q_FACTOR),
                                      CV12_RGB2Y_BTOY_MIN,
                                      CV12_RGB2Y_BTOY_MAX));
        pUnpackedField->yOffset = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::RoundFLOAT((pData->y_offset - pInput->clamp) * pInput->stretch_factor),
                                      CV12_RGB2Y_OFFSET_MIN,
                                      CV12_RGB2Y_OFFSET_MAX));

        pUnpackedField->ap      = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->ap, CV12_CHROMAPROC_DATA_Q_FACTOR),
                                      CV12_CHROMAPROC_AP_MIN,
                                      CV12_CHROMAPROC_AP_MAX));
        pUnpackedField->am      = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->am, CV12_CHROMAPROC_DATA_Q_FACTOR),
                                      CV12_CHROMAPROC_AM_MIN,
                                      CV12_CHROMAPROC_AM_MAX));
        pUnpackedField->bp      = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->bp, CV12_CHROMAPROC_DATA_Q_FACTOR),
                                      CV12_CHROMAPROC_BP_MIN,
                                      CV12_CHROMAPROC_BP_MAX));
        pUnpackedField->bm      = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->bm, CV12_CHROMAPROC_DATA_Q_FACTOR),
                                      CV12_CHROMAPROC_BM_MIN,
                                      CV12_CHROMAPROC_BM_MAX));
        pUnpackedField->cp      = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->cp, CV12_CHROMAPROC_DATA_Q_FACTOR),
                                      CV12_CHROMAPROC_CP_MIN,
                                      CV12_CHROMAPROC_CP_MAX));
        pUnpackedField->cm      = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->cm, CV12_CHROMAPROC_DATA_Q_FACTOR),
                                      CV12_CHROMAPROC_CM_MIN,
                                      CV12_CHROMAPROC_CM_MAX));
        pUnpackedField->dp      = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->dp, CV12_CHROMAPROC_DATA_Q_FACTOR),
                                      CV12_CHROMAPROC_DP_MIN,
                                      CV12_CHROMAPROC_DP_MAX));
        pUnpackedField->dm      = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::FloatToQNumber(pData->dm, CV12_CHROMAPROC_DATA_Q_FACTOR),
                                      CV12_CHROMAPROC_DM_MIN,
                                      CV12_CHROMAPROC_DM_MAX));
        pUnpackedField->kcb     = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::RoundFLOAT(pData->kcb),
                                      CV12_CHROMAPROC_KCB_MIN,
                                      CV12_CHROMAPROC_KCB_MAX));
        pUnpackedField->kcr     = static_cast<FLOAT>(IQSettingUtils::ClampINT32(
                                      IQSettingUtils::RoundFLOAT(pData->kcr),
                                      CV12_CHROMAPROC_KCR_MIN,
                                      CV12_CHROMAPROC_KCR_MAX));
    }
    else
    {
        result = FALSE;
    }

    return result;
}
