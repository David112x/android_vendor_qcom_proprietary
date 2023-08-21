// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gamma16setting.h
/// @brief Gamma16 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAMMA16SETTING_H
#define GAMMA16SETTING_H

#include "gamma_1_6_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT32 GAMMA16_TABLE_LENGTH             = 65;
static const UINT32 DMIRAM_RGB_CH0_LENGTH_RGBLUT_V16 = 64;
static const UINT16 GAMMA16_HW_PACK_BIT              = 10;

/// @brief Unpacked Gamma16 DMI table entries
// NOWHINE NC004c: Share code with system team
struct GammaLUTInStructV16
{
    UINT16  tableSelect;    ///< table sel module
    UINT32* pGammaTable;    ///< used for RGB LUT
};

/// @brief Unpacked Gamma16 output from common IQ library
// NOWHINE NC004c : Share code with system team
struct Gamma16UnpackedField
{
    UINT16              enable;          ///< Module enable flag
    GammaLUTInStructV16 r_lut_in_cfg;    ///< Module R DMI LUT config
    GammaLUTInStructV16 g_lut_in_cfg;    ///< Module G DMI LUT config
    GammaLUTInStructV16 b_lut_in_cfg;    ///< Module B DMI LUT config
};

/// @brief Debug Gamma16 output from common IQ library
// NOWHINE NC004c: Share code with system team
struct Gamma16DebugBuffer
{
    gamma_1_6_0::mod_gamma16_channel_dataType interpolationResult[GammaLUTMax];   ///< Interpolated chromatix data
    Gamma16UnpackedField                      unpackedData;                       ///< Calculated unpacked data
};

enum CHANNEL_TYPE
{
    CHANNEL_TYPE_G,   ///< CHANNEL_TYPE_G
    CHANNEL_TYPE_B,   ///< CHANNEL_TYPE_B
    CHANNEL_TYPE_R    ///< CHANNEL_TYPE_R
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Gamma16 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Gamma16Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pChannelData   Pointer to the channel data of gamma type
    /// @param  pModuleEnable  Pointer to the variable(s) to enable this module
    /// @param  pUnpackedField Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const Gamma16InputData*                                   pInput,
        gamma_1_6_0::mod_gamma16_channel_dataType*                pChannelData,
        gamma_1_6_0::chromatix_gamma16Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                     pUnpackedField);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateGammaLUT
    ///
    /// @brief  Generate the Gamma LUT
    ///
    /// @param  pGamma_table  Pointer to the input gamma table
    /// @param  pLut          Pointer to the output gamma table
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GenerateGammaLUT(
        FLOAT*  pGamma_table,
        UINT32* pLut);
};
#endif // Gamma16SETTING_H
