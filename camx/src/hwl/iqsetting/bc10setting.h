// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  bc10setting.h
/// @brief BC10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef BC10SETTING_H
#define BC10SETTING_H

#include "bincorr_1_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// @brief Unpacked BC10 output from common IQ library
// NOWHINE NC004c: Share code with system team
struct BC10UnpackedField
{
    UINT16 enable;       ///< BC enable
    UINT16 binCorrVerW1; ///< Vertical width 1
    UINT16 binCorrVerW2; ///< Vertical width 2
    UINT16 binCorrHorW1; ///< Horizontal width 1
    UINT16 binCorrHorW2; ///< Horizontal width 2
};

/// @brief Debug BC10 output from common IQ library
// NOWHINE NC004c: Share code with system team
struct BC10DebugBuffer
{
    bincorr_1_0_0::bincorr10_rgn_dataType interpolationResult;
    BC10UnpackedField                     unpackedData;          ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements BC10 module setting calculation5
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class BC10Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pData          Pointer to the interpolation result
    /// @param  pModuleEnable  Pointer to control variable that enables the module
    /// @param  pUnpackedField Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const BC10InputData*                                          pInput,
        bincorr_1_0_0::bincorr10_rgn_dataType*                        pData,
        bincorr_1_0_0::chromatix_bincorr10Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                         pUnpackedField);
};
#endif // BC10SETTING_H
