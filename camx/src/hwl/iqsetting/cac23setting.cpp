// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @file  cac23setting.cpp
// @brief CAC23 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "cac23setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAC23Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAC23Setting::CalculateHWSetting(
    const CAC23InputData*                                 pInput,
    cac_2_3_0::cac23_rgn_dataType*                        pData,
    cac_2_3_0::chromatix_cac23Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                 pRegCmd)
{
    BOOL result = TRUE;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pModuleEnable) &&
        (NULL != pRegCmd))
    {
        CAC23UnpackedField* pUnpackedField   = static_cast<CAC23UnpackedField*>(pRegCmd);

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
