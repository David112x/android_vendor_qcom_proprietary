// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @file  cac22setting.cpp
// @brief CAC22 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "cac22setting.h"

static const UINT32 CACEnableMin            = 0;
static const UINT32 CACEnableMax            = 1;
static const UINT32 SpotYThresholdMin       = 0;
static const UINT32 SpotYThresholdMax       = 63;
static const UINT32 SaturationYThresholdMin = 0;
static const UINT32 SaturationYThresholdMax = 1023;
static const UINT32 SpotCThresholdMin       = 0;
static const UINT32 SpotCThresholdMax       = 1023;
static const UINT32 SaturationCThresholdMin = 0;
static const UINT32 SaturationCThresholdMax = 1023;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAC22Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAC22Setting::CalculateHWSetting(
    const CAC22InputData*                                 pInput,
    cac_2_2_0::cac22_rgn_dataType*                        pData,
    cac_2_2_0::chromatix_cac22Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pRegCmd)
{
    BOOL result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pRegCmd))
    {
        CAC22UnpackedField* pUnpackedField   = static_cast<CAC22UnpackedField*>(pRegCmd);

        pUnpackedField->enableCAC2           =
            IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->cac_en), CACEnableMin, CACEnableMax);

        pUnpackedField->enableSNR            = pInput->enableSNR;
        pUnpackedField->resolution           = pInput->resolution;

        pUnpackedField->ySpotThreshold       =
            IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->y_spot_thr), SpotYThresholdMin, SpotYThresholdMax);
        pUnpackedField->ySaturationThreshold =
            IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->y_saturation_thr),
                                        SaturationYThresholdMin,
                                        SaturationYThresholdMax);
        pUnpackedField->cSpotThreshold       =
            IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->c_spot_thr),
                                        SpotCThresholdMin,
                                        SpotCThresholdMax);
        pUnpackedField->cSaturationThreshold =
            IQSettingUtils::ClampUINT32(static_cast<UINT32>(pData->c_saturation_thr),
                                        SaturationCThresholdMin,
                                        SaturationCThresholdMax);
    }
    else
    {
        result = FALSE;
        // @todo (CAMX-1812) Need to add logging for Common library
    }
    return result;
}
