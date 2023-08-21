// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ipecs20setting.h
/// @brief CS20 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IPECS20SETTING_H
#define IPECS20SETTING_H

#include "cs_2_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT32 CHROMA_SUPP_LUT_SIZE   = 16;
static const INT32  CS20_C_THR_LUT_MIN      = static_cast<INT32>(IQSettingUtils::MINUINTBITFIELD(9));
static const INT32  CS20_C_THR_LUT_MAX      = static_cast<INT32>(IQSettingUtils::MAXUINTBITFIELD(9));

/// @brief CS20 Module Unpacked Data Field
// NOWHINE NC004c: Share code with system team
struct CS20UnpackedField
{
    UINT16 CS_enable;                                    ///< Chromasuppression20 Enable
    UINT32 knee_point_lut[CHROMA_SUPP_LUT_SIZE];         ///< Luma base point lut
    UINT32 knee_point_inverse_lut[CHROMA_SUPP_LUT_SIZE]; ///< Luma base point inverse lut
                                                         ///< 1/(knee[i] - knee[i-1])
    UINT16 y_weight_lut[CHROMA_SUPP_LUT_SIZE];           ///< Luma suppression ratio
    UINT16 c_thr_lut[CHROMA_SUPP_LUT_SIZE];              ///< Chroma thr1 lut
    UINT16 c_slope_lut[CHROMA_SUPP_LUT_SIZE];            ///< 1/(c_thr2-c_thr1)lut
    UINT8  chroma_q;                                     ///< Q bits for chroma (reg value + 1)
    UINT8  luma_q;                                       ///< Q bits for luma (reg value + 1)
};

/// @brief CS20 Module DebugBuffer
// NOWHINE NC004c: Share code with system team
struct CS20DebugBuffer
{
    cs_2_0_0::cs20_rgn_dataType interpolationResult; ///< Interpolated chromatix data
    CS20UnpackedField           unpackedData;        ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements CS20 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IPECS20Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pData          Pointer to the interpolation result
    /// @param  pReserveType   Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable  Pointer to the variable(s) to enable this module
    /// @param  pUnpackedField Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const CS20InputData*                                pInput,
        cs_2_0_0::cs20_rgn_dataType*                        pData,
        cs_2_0_0::chromatix_cs20_reserveType*               pReserveType,
        cs_2_0_0::chromatix_cs20Type::enable_sectionStruct* pModuleEnable,
        VOID*                                               pUnpackedField);
};
#endif // IPECS20SETTING_H
