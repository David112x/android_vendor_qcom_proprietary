// NOWHINE NC009 <- Shared file with system team so uses non-Camx file nameing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  upscale12setting.h
/// @brief Upscale12 module IQ settings calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef UPSCALE12SETTING_H
#define UPSCALE12SETTING_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"

/// @brief Unpacked Data for Upsample v12 Module
// NOWHINE NC004c: Share code with system team
struct Upscale12UnpackedField
{
    UINT16 lumaVScaleFirAlgorithm;        ///< Vertical scale FIR algo
    UINT16 lumaHScaleFirAlgorithm;        ///< Horizontal scale FIR algo
    UINT16 lumaInputDitheringDisable;     ///< Input Dithering disable
    UINT16 lumaInputDitheringMode;        ///< Input Dithering Mode
    UINT16 chromaVScaleFirAlgorithm;      ///< Vertical scale FIR algo
    UINT16 chromaHScaleFirAlgorithm;      ///< Horizontal scale FIR algo
    UINT16 chromaInputDitheringDisable;   ///< Input Dithering disable
    UINT16 chromaInputDitheringMode;      ///< Input Dithering Mode
    UINT16 chromaRoundingModeV;           ///< Chroma rounding mode vertical
    UINT16 chromaRoundingModeH;           ///< Chroma rounding mode horzontal
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Upscale12 module IQ settings calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Upscale12Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked Upscale12 register value
    ///
    /// @param  pInput        Input data to the Upscale12 Module
    /// @param  pOutput       Pointer to Upscale12 Unpacked Register Data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const Upscale12InputData* pInput,
        VOID*                     pOutput);
};

#endif // UPSCALE12SETTING_H
