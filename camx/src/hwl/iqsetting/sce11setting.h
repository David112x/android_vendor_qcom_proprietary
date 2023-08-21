// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  sce11setting.h
/// @brief SCE11 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef SCE11SETTING_H
#define SCE11SETTING_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "sce_1_1_0.h"

/// @brief Unpacked SCE11 output from common IQ library
// NOWHINE NC004c: Share code with system team
struct SCE11UnpackedField
{
    UINT16 enable;  ///< Module enable flag

    // Triangle vertices, 5 sets
    UINT32 t0_Cr1;  ///< (8+e)s
    UINT32 t0_Cb1;  ///< (8+e)s
    UINT32 t0_Cr2;  ///< (8+e)s
    UINT32 t0_Cb2;  ///< (8+e)s
    UINT32 t0_Cr3;  ///< (8+e)s
    UINT32 t0_Cb3;  ///< (8+e)s
    UINT32 t1_Cr1;  ///< (8+e)s
    UINT32 t1_Cb1;  ///< (8+e)s
    UINT32 t1_Cr2;  ///< (8+e)s
    UINT32 t1_Cb2;  ///< (8+e)s
    UINT32 t1_Cr3;  ///< (8+e)s
    UINT32 t1_Cb3;  ///< (8+e)s
    UINT32 t2_Cr1;  ///< (8+e)s
    UINT32 t2_Cb1;  ///< (8+e)s
    UINT32 t2_Cr2;  ///< (8+e)s
    UINT32 t2_Cb2;  ///< (8+e)s
    UINT32 t2_Cr3;  ///< (8+e)s
    UINT32 t2_Cb3;  ///< (8+e)s
    UINT32 t3_Cr1;  ///< (8+e)s
    UINT32 t3_Cb1;  ///< (8+e)s
    UINT32 t3_Cr2;  ///< (8+e)s
    UINT32 t3_Cb2;  ///< (8+e)s
    UINT32 t3_Cr3;  ///< (8+e)s
    UINT32 t3_Cb3;  ///< (8+e)s
    UINT32 t4_Cr1;  ///< (8+e)s
    UINT32 t4_Cb1;  ///< (8+e)s
    UINT32 t4_Cr2;  ///< (8+e)s
    UINT32 t4_Cb2;  ///< (8+e)s
    UINT32 t4_Cr3;  ///< (8+e)s
    UINT32 t4_Cb3;  ///< (8+e)s

    // Affine transform coefficients, 6 sets
    INT32  t0_A;    ///< 12s
    INT32  t0_B;    ///< 12s
    INT32  t0_C;    ///< 17s
    INT32  t0_D;    ///< 12s
    INT32  t0_E;    ///< 12s
    INT32  t0_F;    ///< 17s
    UINT16 t0_Q1;   ///< 4u
    UINT16 t0_Q2;   ///< 4u
    INT32  t1_A;    ///< 12s
    INT32  t1_B;    ///< 12s
    INT32  t1_C;    ///< 17s
    INT32  t1_D;    ///< 12s
    INT32  t1_E;    ///< 12s
    INT32  t1_F;    ///< 12s
    UINT16 t1_Q1;   ///< 4u
    UINT16 t1_Q2;   ///< 4u
    INT32  t2_A;    ///< 12s
    INT32  t2_B;    ///< 12s
    INT32  t2_C;    ///< 17s
    INT32  t2_D;    ///< 12s
    INT32  t2_E;    ///< 12s
    INT32  t2_F;    ///< 17s
    UINT16 t2_Q1;   ///< 4u
    UINT16 t2_Q2;   ///< 4u
    INT32  t3_A;    ///< 12s
    INT32  t3_B;    ///< 12s
    INT32  t3_C;    ///< 17s
    INT32  t3_D;    ///< 12s
    INT32  t3_E;    ///< 12s
    INT32  t3_F;    ///< 17s
    UINT16 t3_Q1;   ///< 4u
    UINT16 t3_Q2;   ///< 4u
    INT32  t4_A;    ///< 12s
    INT32  t4_B;    ///< 12s
    INT32  t4_C;    ///< 17s
    INT32  t4_D;    ///< 12s
    INT32  t4_E;    ///< 12s
    INT32  t4_F;    ///< 17s
    UINT16 t4_Q1;   ///< 4u
    UINT16 t4_Q2;   ///< 4u
    INT32  t5_A;    ///< 12s
    INT32  t5_B;    ///< 12s
    INT32  t5_C;    ///< 17s
    INT32  t5_D;    ///< 12s
    INT32  t5_E;    ///< 12s
    INT32  t5_F;    ///< 17s
    UINT16 t5_Q1;   ///< 4u
    UINT16 t5_Q2;   ///< 4u
};

// NOWHINE NC004c: Share code with system team
struct Float_cr_cb_point
{
    float cr; ///< cr data
    float cb; ///< cb data
};

// NOWHINE NC004c: Share code with system team
struct Triangle
{
    Float_cr_cb_point point[3]; ///< CrCb Points
};

// NOWHINE NC004c: Share code with system team
struct SCE11DebugBuffer
{
    sce_1_1_0::sce11_rgn_dataType interpolationResult; ///< Interpolated chromatix data
    SCE11UnpackedField            unpackedData;        ///< Calculated unpacked data
};

static const INT32 SCE11_LEVEL_MIN = -2047;
static const INT32 SCE11_LEVEL_MAX = 2047;

static const UINT32 SCE11MaxTriangleIdx      = 3;
static const UINT32 SCE11MaxTriangleRow      = 3;
static const UINT32 SCE11MaxTriangleCol      = 3;
static const UINT32 SCE11MaxAffineTriangeSet = 6;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements SCE11 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class SCE11Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pData         Pointer to the interpolation data
    /// @param  pReserveData  Reserved data for SCE module.
    /// @param  pReserveType  Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the unpacked data set of the module
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const SCE11InputData*                                 pInput,
        sce_1_1_0::sce11_rgn_dataType*                        pData,
        sce_1_1_0::chromatix_sce11_reserveType*               pReserveType,
        sce_1_1_0::chromatix_sce11Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RearrangeTriangle
    ///
    /// @brief  Rearranging Triangle
    ///
    /// @param  tInput pointer to the Input data to the Module
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL RearrangeTriangle(
        sce_1_1_0::cr_cb_triangle* pInput);
};
#endif // SCE11SETTING_H
