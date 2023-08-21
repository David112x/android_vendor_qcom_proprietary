// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  demosaic37setting.h
/// @brief Demosaic37 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DEMOSAIC37SETTING_H
#define DEMOSAIC37SETTING_H

#include "demosaic_3_7_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

// NOWHINE NC004c: Share code with system team
struct Demosaic37UnpackedField
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

static const UINT32 Demosaic37AKMin                   = 0;
static const UINT32 Demosaic37AKMax                   = 400;
static const INT32  Demosaic37WKMin                   = -512;
static const INT32  Demosaic37WKMax                   = 511;
static const UINT32 Demosaic37DisDirectionalGMin      = 0;
static const UINT32 Demosaic37DisDirectionalGMax      = 255;
static const UINT32 Demosaic37DisDirectionalRBMin     = 0;
static const UINT32 Demosaic37DisDirectionalRBMax     = 255;
static const UINT32 Demosaic37LowFreqWeightGMin       = 0;
static const UINT32 Demosaic37LowFreqWeightGMax       = 255;
static const UINT32 Demosaic37LowFreqWeightRBMin      = 0;
static const UINT32 Demosaic37LowFreqWeightRBMax      = 255;
static const UINT32 Demosaic37EnableDynamicClampGMin  = 0;
static const UINT32 Demosaic37EnableDynamicClampGMax  = 1;
static const UINT32 Demosaic37EnableDynamicClampRBMin = 0;
static const UINT32 Demosaic37EnableDynamicClampRBMax = 1;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Demosaic37 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Demosaic37Setting
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
        const Demosaic37InputData*                                      pInput,
        demosaic_3_7_0::demosaic37_rgn_dataType*                        pData,
        demosaic_3_7_0::chromatix_demosaic37Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                           pRegCmd);
};

#endif // DEMOSAIC37SETTING_H
