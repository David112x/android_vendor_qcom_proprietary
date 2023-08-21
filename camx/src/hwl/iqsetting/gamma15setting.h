// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gamma15setting.h
/// @brief GAMMA1_5 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GAMMA15SETTING_H
#define GAMMA15SETTING_H

#include "gamma_1_5_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT32 Gamma15LUTNumEntriesPerChannel = 256;
static const UINT8  Gamma15DefaultContrastLevel    = 5;

// NOWHINE NC004c: Share code with system team
struct Gamma15LUTInStruct
{
    UINT32*  pGammaTable; ///< Gamma Table for one Channel (R/G/B)
    UINT16   numEntries;  ///< number of entries in the Gamma Table
};

/// @brief: Gamma15 unpacked field
// NOWHINE NC004c: Share code with system team
struct Gamma15UnpackedField
{
    UINT16              enable;       ///< Module enable flag
    Gamma15LUTInStruct  rLUTInConfig; ///< Module rLUTInConfig flag
    Gamma15LUTInStruct  gLUTInConfig; ///< Module gLUTInConfig flag
    Gamma15LUTInStruct  bLUTInConfig; ///< Module bLUTInConfig flag
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Gamma15 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Gamma15Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pData          Pointer to the intepolation result
    /// @param  pModuleEnable  Pointer to the variable(s) to enable this module
    /// @param  pUnpackedField Pointer to the unpacked register result
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const Gamma15InputData*                                   pInput,
        gamma_1_5_0::mod_gamma15_cct_dataType::cct_dataStruct*    pData,
        gamma_1_5_0::chromatix_gamma15Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                     pUnpackedField);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GenerateGammaLUT
    ///
    /// @brief  Generate the Gamma LUT of 256 entries for one Channel
    ///
    /// @param  pGammaTable  Pointer to the Gamma Table (R/G/B)
    /// @param  pLUT         Pointer to the output LUT Table of 256 UINT32 data (R/G/B)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID GenerateGammaLUT(
        FLOAT*  pGammaTable,
        UINT32* pLUT);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ApplyManualContrastToGammaCurve
    ///
    /// @brief  Apply Manual Contrast to Gamma Curve for one Channel
    ///
    /// @param  contrastLevel   level of contrast need to apply to gamma curve
    /// @param  pGammaTable     Pointer to the Gamma Table (R/G/B)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ApplyManualContrastToGammaCurve(
        const UINT8 contrastLevel,
        FLOAT*      pGammaTable);
};

#endif // GAMMA15SETTING_H
