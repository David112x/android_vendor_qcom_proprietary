// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  wb13setting.cpp
/// @brief wb13 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "wb13setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WB13Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL WB13Setting::CalculateHWSetting(
    const WB13InputData*             pInput,
    globalelements::enable_flag_type moduleEnable,
    VOID*                            pOutput)
{
    BOOL result = TRUE;

    if ((NULL != pInput) && (NULL != pOutput))
    {
        WB13UnpackedField* pUnpackedField = static_cast<WB13UnpackedField*>(pOutput);

        pUnpackedField->enable    = static_cast<UINT16>(moduleEnable);
        pUnpackedField->leftImgWd = static_cast<UINT16>(static_cast<FLOAT>(pInput->inputWidth) / 2.0f);

        // Left 3D image
        // green channel gain, 12uQ7
        pUnpackedField->gainChannel0Left     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                             pInput->leftGGainWB * pInput->predictiveGain, QNumber_10U),
                                                             WB13GainTableMinValue,
                                                             WB13GainTableMaxValue));

        // blue channel gain, 12uQ7
        pUnpackedField->gainChannel1Left     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                             pInput->leftBGainWB * pInput->predictiveGain, QNumber_10U),
                                                             WB13GainTableMinValue,
                                                             WB13GainTableMaxValue));

        // red channel gain, 12uQ7
        pUnpackedField->gainChannel2Left     =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber(
                                                             pInput->leftRGainWB * pInput->predictiveGain, QNumber_10U),
                                                             WB13GainTableMinValue,
                                                             WB13GainTableMaxValue));

        pUnpackedField->offsetChannel0Left   = 0;
        pUnpackedField->offsetChannel1Left   = 0;
        pUnpackedField->offsetChannel2Left   = 0;
    }
    else
    {
        result = FALSE;
    }

    return result;
}
