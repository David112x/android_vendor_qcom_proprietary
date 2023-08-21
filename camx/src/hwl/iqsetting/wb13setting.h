// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  wb13setting.h
/// @brief WB13 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WB13SETTING_H
#define WB13SETTING_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"

// Const Min Max values for WB13
static const INT32  WB13GainTableMinValue = 0;
static const INT32  WB13GainTableMaxValue = ((1 << 15) - 1);

/// @brief Unpacked Data for WB13 Module
// NOWHINE NC004c: Share code with system team
struct WB13UnpackedField
{
    UINT16 enable;              ///< Module enable flag
    ///< Left 3D image
    UINT16 leftImgWd;           ///< Left Image Width
    UINT16 gainChannel0Left;    ///< green channel gain, 12uQ7
    UINT16 gainChannel1Left;    ///< blue channel gain, 12uQ7
    UINT16 gainChannel2Left;    ///< red channel gain, 12uQ7
    UINT16 offsetChannel0Left;  ///< green channel offset, 13s/15s
    UINT16 offsetChannel1Left;  ///< blue channel offset, 13s/15s
    UINT16 offsetChannel2Left;  ///< red channel offset, 13s/15s
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements WB13module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class WB13Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput       Pointer to the input data
    /// @param  moduleEnable Control variable to enable the module
    /// @param  pRegCmd      Pointer to the unpacked data
    ///
    /// @return true for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const WB13InputData*             pInput,
        globalelements::enable_flag_type moduleEnable,
        VOID*                            pRegCmd);
};
#endif // WB13_H
