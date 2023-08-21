// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  wb12setting.cpp
/// @brief wb12 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "wb12setting.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WB12Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL WB12Setting::CalculateHWSetting(
    const WB12InputData* pInput,
    VOID*                pOutput)
{
    BOOL  result = TRUE;

    if ((NULL != pInput) && (NULL != pOutput))
    {
        WB12UnpackedField*   pUnpackedField = static_cast<WB12UnpackedField*>(pOutput);

        // Left 3D image
        // green channel gain, 12uQ7
        pUnpackedField->gainChannel0Left    =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber
                                                            (pInput->leftGGainWB*pInput->predictiveGain, QNumber_7U),
                                                            WB12GainTableMinValue,
                                                            WB12GainTableMaxValue));

        // blue channel gain, 12uQ7
        pUnpackedField->gainChannel1Left    =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber
                                                            (pInput->leftBGainWB*pInput->predictiveGain, QNumber_7U),
                                                            WB12GainTableMinValue,
                                                            WB12GainTableMaxValue));

        // red channel gain, 12uQ7
        pUnpackedField->gainChannel2Left    =
            static_cast<UINT16>(IQSettingUtils::ClampUINT32(IQSettingUtils::FloatToQNumber
                                                            (pInput->leftRGainWB*pInput->predictiveGain, QNumber_7U),
                                                            WB12GainTableMinValue,
                                                            WB12GainTableMaxValue));
        pUnpackedField->offsetChannel0Left  = 0;
        pUnpackedField->offsetChannel1Left  = 0;
        pUnpackedField->offsetChannel2Left  = 0;
    }
    else
    {
        result = FALSE;
    }

    return result;
}
