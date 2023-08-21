// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  demux13setting.h
/// @brief Demux13 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef DEMUX13SETTING_H
#define DEMUX13SETTING_H

#include "demux_1_3_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

// NOWHINE NC004c: Share code with system team
struct Demux13UnpackedField
{
    UINT16 enable;                   ///< Module enable flag
    UINT16 period;                   ///< period
    UINT32 oddConfig;                ///< Odd Channel Config
    UINT32 evenConfig;               ///< Even Channel Config
    UINT16 gainChannel0EevenLeft;    ///< Gain of Channel 0
    UINT16 gainChannel0OddLeft;      ///< Gain of Channel 0
    UINT16 gainChannel1Left;         ///< gainChannel1Left
    UINT16 gainChannel2Left;         ///< gainChannel2Left
    UINT16 gainChannel0EvenRight;    ///< gainChannel0EvenRight
    UINT16 gainChannel0OddRight;     ///< gainChannel0OddRight
    UINT16 gainChannel1Right;        ///< gainChannel1Right
    UINT16 gainChannel2Right;        ///< gainChannel2Right
    UINT16 blackLevelIn;             ///< blaockLevelIn
    UINT16 blackLevelOut;            ///< blackLevelOut
};

static const UINT32 IFEDemux13QNumber       = 10;
static const UINT32 IFEDemux13MaxValue      = ((1 << 15) - 1);
static const FLOAT  MAX_DEMUX13_GAIN        = 31.999f;
static const UINT32 DEMUX13_BLK_LVL_IN_MIN  = 0;
static const UINT32 DEMUX13_BLK_LVL_IN_MAX  = ((1 << 12) - 1);
static const UINT32 DEMUX13_BLK_LVL_OUT_MIN = 0;
static const UINT32 DEMUX13_BLK_LVL_OUT_MAX = ((1 << 12) - 1);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Demux13 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Demux13Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to the input data to the Demux13 interpolation module
    /// @param  pReserveType  Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the unpacked field
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const Demux13InputData*                                   pInput,
        const demux_1_3_0::chromatix_demux13_reserveType*         pReserveType,
        demux_1_3_0::chromatix_demux13Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                     pRegCmd);
};
#endif // DEMUX13SETTING_H
