// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  gra10setting.h
/// @brief GRA10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef GRA10SETTING_H
#define GRA10SETTING_H
#include "gra_1_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT32 GRA10LUTEntrySize                = sizeof(UINT32);
static const UINT32 GRA10LUTNumEntriesPerChannel     = 32;
static const UINT32 GRA10LUTNumEntriesPerChannelSize = GRA10LUTNumEntriesPerChannel * GRA10LUTEntrySize;
static const UINT   MaxGRA10LUTNumEntries            = 96; ///< Total Entries 96 (Sum of all entries from all LUT curves)
static const UINT   BITPERPIXEL                      = 8;

struct CamxLutGrainAdderInStruct_V10
{
    UINT32* pGRATable;      ///< Gamma Table for one Channel (Y/Cb/Cr)
    UINT16  numEntries;     ///< number of entries in the GRA Table
};

/// @brief: This enumerator is for gra look up tables
enum IPEGRALUT
{
    GRALUTChannel0 = 0,   ///< Y channel
    GRALUTChannel1 = 1,   ///< CB channel
    GRALUTChannel2 = 2,   ///< Cr channel
    GRALUTMax      = 3,   ///< Max LUTs
};

// NOWHINE NC004c: Share code with system team
struct GRA10UnpackedField
{
    UINT16 enable;                         ///< 1u, Module enable flag
    UINT16 enable_dithering_Y;             ///< 1u,
    UINT16 enable_dithering_C;             ///< 1u,
    UINT32 grain_seed;                     ///< 28u,
    UINT32 mcg_a;                          ///< 28u,
    UINT32 skip_ahead_a_jump;              ///< 28u,
    UINT16 grain_strength;                 ///< 3u,
    UINT16 r;                              ///< 3u,
    CamxLutGrainAdderInStruct_V10 ch0_LUT; ///< Y channel LUT
    CamxLutGrainAdderInStruct_V10 ch1_LUT; ///< Cb channel LUT
    CamxLutGrainAdderInStruct_V10 ch2_LUT; ///< Cr channel LUT
};

// NOWHINE NC004c: Share code with system team
struct GRA10DebugBuffer
{
    gra_1_0_0::gra10_rgn_dataType interpolationResult; ///< Interpolated chromatix data
    GRA10UnpackedField            unpackedData;        ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements GRA10 module tuning data interpolation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class GRA10Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to iput data
    /// @param  pData         Pointer to the intepolation result
    /// @param  pReserveData  Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const GRA10IQInput*                                   pInput,
        gra_1_0_0::gra10_rgn_dataType*                        pData,
        gra_1_0_0::chromatix_gra10_reserveType*               pReserveData,
        gra_1_0_0::chromatix_gra10Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);
};
#endif // GRA10SETTING_H
