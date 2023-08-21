// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  demosaic36setting.h
/// @brief Demosaic36 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DEMOSAIC36SETTING_H
#define DEMOSAIC36SETTING_H

#include "demosaic_3_6_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

// NOWHINE NC004c: Share code with system team
struct Demosaic36UnpackedField
{
    UINT16 enable;               ///< Module enable flag
    UINT16 cositedRGB;           ///< cositeRGB
    UINT16 enable3D;             ///< Enable 3D flag
    UINT16 leftImageWD;          ///< leftImageWD
    INT16  wk;                   ///< WK field
    UINT16 ak;                   ///< AK field
    UINT16 enDemosaicV4;         ///< Switch between v3/v4 interpolation filters
    UINT16 disableDirectionalG;  ///< Disable directional G interpolation
    UINT16 disableDirectionalRB; ///< Disable directional RB interpolation
    UINT16 enDynamicClampG;      ///< Enable G dynamic clamping
    UINT16 enDynamicClampRB;     ///< Enable RB Dynamic clamping
    UINT16 lambdaG;              ///< Lambda G channel
    UINT16 lambdaRB;             ///< Lambda RB channel
};

static const UINT32 Demosaic36AKMin                   = 0;
static const UINT32 Demosaic36AKMax                   = 400;
static const INT32  Demosaic36WKMin                   = -512;
static const INT32  Demosaic36WKMax                   = 511;
static const UINT32 Demosaic36DisDirectionalGMin      = 0;
static const UINT32 Demosaic36DisDirectionalGMax      = 255;
static const UINT32 Demosaic36DisDirectionalRBMin     = 0;
static const UINT32 Demosaic36DisDirectionalRBMax     = 255;
static const UINT32 Demosaic36LowFreqWeightGMin       = 0;
static const UINT32 Demosaic36LowFreqWeightGMax       = 255;
static const UINT32 Demosaic36LowFreqWeightRBMin      = 0;
static const UINT32 Demosaic36LowFreqWeightRBMax      = 255;
static const UINT32 Demosaic36EnableDynamicClampGMin  = 0;
static const UINT32 Demosaic36EnableDynamicClampGMax  = 1;
static const UINT32 Demosaic36EnableDynamicClampRBMin = 0;
static const UINT32 Demosaic36EnableDynamicClampRBMax = 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Demosaic36 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Demosaic36Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to the Input data to the demosaic Module
    /// @param  pData         Pointer to the interpolation data
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the unpacked data set of the demosaic module
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const Demosaic36InputData*                                      pInput,
        demosaic_3_6_0::demosaic36_rgn_dataType*                        pData,
        demosaic_3_6_0::chromatix_demosaic36Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                           pRegCmd);
};

#endif // DEMOSAIC36SETTING_H
