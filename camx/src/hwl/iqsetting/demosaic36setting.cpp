// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  demosaic36setting.cpp
/// @brief Demosaic36 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "demosaic36setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Demosaic36Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Demosaic36Setting::CalculateHWSetting(
    const Demosaic36InputData*                                      pInput,
    demosaic_3_6_0::demosaic36_rgn_dataType*                        pData,
    demosaic_3_6_0::chromatix_demosaic36Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                           pOutput)
{
    BOOL  result = TRUE;

    CAMX_UNREFERENCED_PARAM(pInput);

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        Demosaic36UnpackedField* pUnpackedField = static_cast<Demosaic36UnpackedField*>(pOutput);

        pUnpackedField->enable               = static_cast<UINT16>(pModuleEnable->demosaic_enable);
        pUnpackedField->cositedRGB           = 0;
        pUnpackedField->enable3D             = 0;
        pUnpackedField->enDemosaicV4         = static_cast<UINT16>(pModuleEnable->demosaic_enable);
        pUnpackedField->leftImageWD          = static_cast<UINT16>(pInput->imageWidth / 2);
        pUnpackedField->ak                   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->edge_det_noise_offset),
                                                            Demosaic36AKMin,
                                                            Demosaic36AKMax));
        pUnpackedField->wk                   =
            static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->edge_det_weight*(1 << 10)),
                                                           Demosaic36WKMin,
                                                           Demosaic36WKMax));

        pUnpackedField->disableDirectionalG  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->dis_directional_g),
                                                            Demosaic36DisDirectionalGMin,
                                                            Demosaic36DisDirectionalGMax));

        pUnpackedField->enDynamicClampG      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->en_dyna_clamp_g),
                                                            Demosaic36EnableDynamicClampGMin,
                                                            Demosaic36EnableDynamicClampGMax));

        pUnpackedField->disableDirectionalRB =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->dis_directional_rb),
                                                            Demosaic36DisDirectionalRBMin,
                                                            Demosaic36DisDirectionalRBMax));

        pUnpackedField->enDynamicClampRB     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->en_dyna_clamp_rb),
                                                            Demosaic36EnableDynamicClampRBMin,
                                                            Demosaic36EnableDynamicClampRBMax));

        pUnpackedField->lambdaG              =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(pData->lambda_g)),
                                            Demosaic36LowFreqWeightGMin,
                                            Demosaic36LowFreqWeightGMax));

        pUnpackedField->lambdaRB             =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(pData->lambda_rb)),
                                            Demosaic36LowFreqWeightRBMin,
                                            Demosaic36LowFreqWeightRBMax));
    }
    else
    {
        result = FALSE;
    }

    return result;
}
