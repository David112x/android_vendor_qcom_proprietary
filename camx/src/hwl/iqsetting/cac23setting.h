// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// @file  cac23setting.h
// @brief CAC23 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAC23SETTING_H
#define CAC23SETTING_H

#include "cac_2_3_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT32 CACEnableMin = 0;
static const UINT32 CACEnableMax = 1;
static const UINT32 SpotYThresholdMin = 0;
static const UINT32 SpotYThresholdMax = 63;
static const UINT32 SaturationYThresholdMin = 0;
static const UINT32 SaturationYThresholdMax = 1023;
static const UINT32 SpotCThresholdMin = 0;
static const UINT32 SpotCThresholdMax = 1023;
static const UINT32 SaturationCThresholdMin = 0;
static const UINT32 SaturationCThresholdMax = 511;

// NOWHINE NC004c: Share code with system team
struct CAC23UnpackedField
{
    UINT32 enableCAC2;              ///< CAC2 enable flag
    UINT32 enableSNR;               ///< SNR enable flag
    UINT32 resolution;              ///< 1u 0 - 420 proessing 1 - 423 processing
    UINT32 ySpotThreshold;          ///< 6u (Q6) THD1
    UINT32 ySaturationThreshold;    ///< 8u (Q7) THD2
    UINT32 cSpotThreshold;          ///< 0u (Q7) THD4
    UINT32 cSaturationThreshold;    ///< 7u (Q7) THD3
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements CAC23 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class CAC23Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput  Pointer to input data of CAC23 Module
    /// @param  pData   Pointer to output of the interpolation algorithem
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd Pointer to ouput register value
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const CAC23InputData*                                 pInput,
        cac_2_3_0::cac23_rgn_dataType*                        pData,
        cac_2_3_0::chromatix_cac23Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);
};
#endif // CAC23SETTING_H
