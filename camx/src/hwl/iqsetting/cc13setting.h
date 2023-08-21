// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  cc13setting.h
/// @brief CC1_3 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CC13SETTING_H
#define CC13SETTING_H
#include "cc_1_3_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT32 CC13_Q_FACTOR = 7;

// NOWHINE NC004c: Share code with system team
struct CC13UnpackedField
{
    UINT16 enable;    ///< Module enable flag
    INT16  c0;        ///< 12s bits
    INT16  c1;        ///< 12s bits
    INT16  c2;        ///< 12s bits
    INT16  c3;        ///< 12s bits
    INT16  c4;        ///< 12s bits
    INT16  c5;        ///< 12s bits
    INT16  c6;        ///< 12s bits
    INT16  c7;        ///< 12s bits
    INT16  c8;        ///< 12s bits
    INT16  k0;        ///< (7+e)s bits
    INT16  k1;        ///< (7+e)s bits
    INT16  k2;        ///< (7+e)s bits
    UINT16 qfactor;   ///< 2u bits
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements CC13 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class CC13Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pData          Pointer to the interpolation result
    /// @param  pReserveType   Pointer to the reserveType of this module
    /// @param  pModuleEnable  Pointer to the variable(s) to enable this module
    /// @param  pOutput        Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const CC13InputData*                                pInput,
        cc_1_3_0::cc13_rgn_dataType*                        pData,
        cc_1_3_0::chromatix_cc13_reserveType*               pReserveType,
        cc_1_3_0::chromatix_cc13Type::enable_sectionStruct* pModuleEnable,
        VOID*                                               pOutput);
};
#endif // IFECC13SETTING_H
