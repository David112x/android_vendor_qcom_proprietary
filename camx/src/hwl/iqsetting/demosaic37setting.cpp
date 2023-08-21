// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  demosaic37setting.cpp
/// @brief Demosaic37 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "demosaic37setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Demosaic37Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Demosaic37Setting::CalculateHWSetting(
    const Demosaic37InputData*                                      pInput,
    demosaic_3_7_0::demosaic37_rgn_dataType*                        pData,
    demosaic_3_7_0::chromatix_demosaic37Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                           pOutput)
{
    BOOL  result = TRUE;

    CAMX_UNREFERENCED_PARAM(pInput);

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        Demosaic37UnpackedField* pUnpackedField = static_cast<Demosaic37UnpackedField*>(pOutput);

        pUnpackedField->enable               = static_cast<UINT16>(pModuleEnable->demosaic_enable);
        pUnpackedField->cositedRGB           = 0;
        pUnpackedField->enable3D             = 0;
        pUnpackedField->enDemosaicV4         = static_cast<UINT16>(pModuleEnable->demosaic_enable);
        pUnpackedField->leftImageWD          = static_cast<UINT16>(pInput->imageWidth / 2);
        pUnpackedField->ak                   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->edge_det_noise_offset),
                                                            Demosaic37AKMin,
                                                            Demosaic37AKMax));
        pUnpackedField->wk                   =
            static_cast<UINT16>(IQSettingUtils::ClampINT32(IQSettingUtils::RoundFLOAT(pData->edge_det_weight*(1 << 10)),
                                                           Demosaic37WKMin,
                                                           Demosaic37WKMax));

        pUnpackedField->disableDirectionalG  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->dis_directional_g),
                                                            Demosaic37DisDirectionalGMin,
                                                            Demosaic37DisDirectionalGMax));

        pUnpackedField->enDynamicClampG      =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->en_dyna_clamp_g),
                                                            Demosaic37EnableDynamicClampGMin,
                                                            Demosaic37EnableDynamicClampGMax));

        pUnpackedField->disableDirectionalRB =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->dis_directional_rb),
                                                            Demosaic37DisDirectionalRBMin,
                                                            Demosaic37DisDirectionalRBMax));

        pUnpackedField->enDynamicClampRB     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->en_dyna_clamp_rb),
                                                            Demosaic37EnableDynamicClampRBMin,
                                                            Demosaic37EnableDynamicClampRBMax));

        pUnpackedField->lambdaG              =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(pData->lambda_g)),
                                            Demosaic37LowFreqWeightGMin,
                                            Demosaic37LowFreqWeightGMax));

        pUnpackedField->lambdaRB             =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(
                IQSettingUtils::AbsoluteINT(IQSettingUtils::RoundFLOAT(pData->lambda_rb)),
                                            Demosaic37LowFreqWeightRBMin,
                                            Demosaic37LowFreqWeightRBMax));
    }
    else
    {
        result = FALSE;
    }

    return result;
}
