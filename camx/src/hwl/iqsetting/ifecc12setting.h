// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  ifecc12setting.h
/// @brief CC1_2 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef IFECC12SETTING_H
#define IFECC12SETTING_H
#include "cc_1_2_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

static const UINT32 CC12_Q_FACTOR = 7;

// NOWHINE NC004c: Share code with system team
struct CC12UnpackedField
{
    INT16 c0_l;       ///< c0_1, left plane setting
    INT16 c1_l;       ///< c1_1, left plane setting
    INT16 c2_l;       ///< c2_1, left plane setting
    INT16 c3_l;       ///< c3_1, left plane setting
    INT16 c4_l;       ///< c4_1, left plane setting
    INT16 c5_l;       ///< c5_1, left plane setting
    INT16 c6_l;       ///< c6_1, left plane setting
    INT16 c7_l;       ///< c7_1, left plane setting
    INT16 c8_l;       ///< c8_1, left plane setting
    INT16 k0_l;       ///< k0_1, left plane setting
    INT16 k1_l;       ///< k1_1, left plane setting
    INT16 k2_l;       ///< k2_1, left plane setting
    UINT16 qfactor_l; ///< qfactor of left plane, left plane setting
    INT16 c0_r;       ///< c0_r, right plane setting
    INT16 c1_r;       ///< c1_r, right plane setting
    INT16 c2_r;       ///< c2_r, right plane setting
    INT16 c3_r;       ///< c3_r, right plane setting
    INT16 c4_r;       ///< c4_r, right plane setting
    INT16 c5_r;       ///< c5_r, right plane setting
    INT16 c6_r;       ///< c6_r, right plane setting
    INT16 c7_r;       ///< c7_r, right plane setting
    INT16 c8_r;       ///< c8_r, right plane setting
    INT16 k0_r;       ///< k0_r, right plane setting
    INT16 k1_r;       ///< k1_r, right plane setting
    INT16 k2_r;       ///< k2_r, right plane setting
    UINT16 qfactor_r; ///< qfactor_r, right plane setting
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements CC12 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class IFECC12Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the input data
    /// @param  pData          Pointer to the intepolation result
    /// @param  pReserveType   Pointer to the Chromatix ReserveType Field
    /// @param  pUnpackedField Pointer to the Unpacked Data Field
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const CC12InputData*                  pInput,
        cc_1_2_0::cc12_rgn_dataType*          pData,
        cc_1_2_0::chromatix_cc12_reserveType* pReserveType,
        VOID*                                 pUnpackedField);
};
#endif // IFECC12SETTING_H
