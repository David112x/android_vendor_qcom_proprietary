// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @file  cac22setting.h
// @brief CAC22 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAC22SETTING_H
#define CAC22SETTING_H

#include "cac_2_2_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

// NOWHINE NC004c: Share code with system team
struct CAC22UnpackedField
{
    UINT32 enableCAC2;              ///< CAC2 enable flag
    UINT32 enableSNR;               ///< SNR enable flag
    UINT32 resolution;              ///< 1u 0 - 420 proessing 1 - 422 processing
    UINT32 ySpotThreshold;          ///< 6u (Q6) THD1
    UINT32 ySaturationThreshold;    ///< 8u (Q7) THD2
    UINT32 cSpotThreshold;          ///< 0u (Q7) THD4
    UINT32 cSaturationThreshold;    ///< 7u (Q7) THD3
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements CAC22 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class CAC22Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput  Pointer to input data of CAC22 Module
    /// @param  pData   Pointer to output of the interpolation algorithem
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd Pointer to ouput register value
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const CAC22InputData*                                 pInput,
        cac_2_2_0::cac22_rgn_dataType*                        pData,
        cac_2_2_0::chromatix_cac22Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);
};
#endif // CAC22SETTING_H
