// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bls12setting.cpp
/// @brief bls12 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bls12setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BLS12Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BLS12Setting::CalculateHWSetting(
    const BLS12InputData*                                 pInput,
    bls_1_2_0::bls12_rgn_dataType*                        pData,
    bls_1_2_0::chromatix_bls12Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pOutput)
{
    BOOL  result = TRUE;
    FLOAT scale  = 1.0f;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        BLS12UnpackedField* pUnpackedField = static_cast<BLS12UnpackedField*>(pOutput);

        pUnpackedField->enable       = static_cast<UINT16>(pModuleEnable->bls_enable);
        pUnpackedField->bayerPattern = pInput->bayerPattern;

        if (((1 << PIPELINE_BITWIDTH) - pData->offset) != 0)
        {
            scale = (static_cast<FLOAT>((1 << PIPELINE_BITWIDTH)) /
                (static_cast<FLOAT>(1 << PIPELINE_BITWIDTH) - IQSettingUtils::RoundFLOAT(pData->offset)));
        }

        pUnpackedField->scale         =
            static_cast<UINT16>(IQSettingUtils::ClampINT32(static_cast<INT32> (scale * (1 << BLS12_SCALE_Q_FACTOR)),
                                                           BLS12_SCALE_MIN,
                                                           BLS12_SCALE_MAX));
        pUnpackedField->offset       =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::RoundFLOAT(pData->offset),
                                                            BLS12_OFFSET_MIN,
                                                            BLS12_OFFSET_MAX));
        pUnpackedField->thresholdR   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->threshold_r),
                                                            BLS12_THR_R_MIN,
                                                            BLS12_THR_R_MAX));
        pUnpackedField->thresholdGR  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->threshold_gr),
                                                            BLS12_THR_GR_MIN,
                                                            BLS12_THR_GR_MAX));
        pUnpackedField->thresholdGB  =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->threshold_gb),
                                                            BLS12_THR_GB_MIN,
                                                            BLS12_THR_GB_MAX));
        pUnpackedField->thresholdB   =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->threshold_b),
                                                            BLS12_THR_B_MIN,
                                                            BLS12_THR_B_MAX));
    }
    else
    {
        /// @todo (CAMX-1460) Need to add logging for Common library
        result = FALSE;
    }
    return result;
}
