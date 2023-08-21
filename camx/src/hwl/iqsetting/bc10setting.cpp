// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bc10setting.cpp
/// @brief bc10 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "bc10setting.h"

static const UINT16 BC10_W1_W2_MIN = 0;
static const UINT16 BC10_W1_W2_MAX = 128;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BC10Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BC10Setting::CalculateHWSetting(
    const BC10InputData*                                          pInput,
    bincorr_1_0_0::bincorr10_rgn_dataType*                        pData,
    bincorr_1_0_0::chromatix_bincorr10Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                         pOutput)
{
    BOOL  result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        BC10UnpackedField* pUnpackedField = static_cast<BC10UnpackedField*>(pOutput);

        pUnpackedField->enable       = static_cast<UINT16>(pModuleEnable->bincorr_enable);
        pUnpackedField->binCorrVerW1 = IQSettingUtils::ClampUINT16(pData->ver_bin_corr_w1, BC10_W1_W2_MIN, BC10_W1_W2_MAX);
        pUnpackedField->binCorrVerW2 = IQSettingUtils::ClampUINT16(pData->ver_bin_corr_w2, BC10_W1_W2_MIN, BC10_W1_W2_MAX);
        pUnpackedField->binCorrHorW1 = IQSettingUtils::ClampUINT16(pData->hor_bin_corr_w1, BC10_W1_W2_MIN, BC10_W1_W2_MAX);
        pUnpackedField->binCorrHorW2 = IQSettingUtils::ClampUINT16(pData->hor_bin_corr_w2, BC10_W1_W2_MIN, BC10_W1_W2_MAX);
    }
    else
    {
        result = FALSE;
    }

    return result;
}
