// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  pedestal13setting.h
/// @brief Pedestal13 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef PEDESTAL13SETTING_H
#define PEDESTAL13SETTING_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "pedestal_1_3_0.h"

static const UINT32 PED_LUT_LENGTH    = 130;
static const UINT32 PED_MESH_H_V13    = 12;  // Note: VFE4 HW is 12
static const UINT32 PED_MESH_V_V13    = 9;   // Note: VFE4 HW is 9
static const UINT32 PED_MESH_PT_H_V13 = 13;  // PED_MESH_PT_H_V13 = PED_MESH_H_V13 + 1
static const UINT32 PED_MESH_PT_V_V13 = 10;
static const UINT32 PEDESTAL13_MIN    = 0;
static const UINT32 PEDESTAL13_MAX    = 4095;

/// @brief Unpacked Pedestal13 output from common IQ library
// NOWHINE NC004c: Share code with system team
struct Pedestal13UnpackedField
{
    UINT16 enable;                ///< Module enable flag
    UINT16 enable3D;              ///< Indicate 3D enabled
    UINT16 HDRen;                 ///< Indicate HDR enabled
    UINT16 leftImgWd;             ///< Left image width
    UINT16 bankSel;               ///< RAM bank 0 or 1 (~ double buffer)
    UINT16 scaleBypass;           ///< Whether to bypass scaling pedestal difference to full range
                                  ///< Left image
    UINT16 meshTblT1L[2][4][PED_MESH_PT_V_V13][PED_MESH_PT_H_V13];
    ///< Mesh gain table for T1 lines in left plane, 12u,
    ///< double buffered in HW for 4 channels: 0->R, 1->Gr, 2->Gb, 3->B
    UINT32 intpFactorL;           ///< Bicubic interpolation factor for left image, 0~3 (1/2/4/8 subgrids)
    UINT32 bWidthL;               ///< Subgrid width, 11u, 2*Bwidth is the real width
    UINT32 bHeightL;              ///< Subgrid height, 10u, 2*Bheight is the real height
    UINT32 meshGridbWidthL;       ///< Meshgrid width, 11u, 2*MeshGridBwidth is the real width
                                  ///< not used in pedestal implementation, only as HW counters
    UINT32 meshGridbHeightL;      ///< Meshgrid height, 10u, 2*MeshGridBheight is the real height
                                  ///< not used in pedestal implementation, only as HW counters
    UINT32 xDeltaL;               ///< 1/Bwidth, 17uQ20
    UINT32 yDeltaL;               ///< 1/Bheight, 17uQ20
                                  ///< Right image
    UINT16 meshTblT1R[2][4][PED_MESH_PT_V_V13][PED_MESH_PT_H_V13];
    ///< Mesh gain table for T1 lines in right plane, 12u,
    ///< double buffered in HW for 4 channels: 0->R, 1->Gr, 2->Gb, 3->B
    UINT32 intpFactorR;           ///< Bicubic interpolation factor for right image, 0~3 (1/2/4/8 subgrids)
    UINT32 bWidthR;               ///< Subgrid width, 11u, 2*Bwidth is the real width
    UINT32 bHeightR;              ///< Subgrid height, 10u, 2*Bheight is the real height
    UINT32 meshGridbWidthR;       ///< Meshgrid width, 11u, 2*MeshGridBwidth is the real width
                                  ///< not used in pedestal implementation, only as HW counters
    UINT32 meshGridbHeightR;      ///< Meshgrid height, 10u, 2*MeshGridBheight is the real height
                                  ///< not used in pedestal implementation, only as HW counters
    UINT32 xDeltaR;               ///< 1/Bwidth, 17uQ20
    UINT32 yDeltaR;               ///< 1/Bheight, 17uQ20
                                  ///< Stripe processing is not supported on 3D mode, so these are needed for left image
                                  ///< (as single image default) only
    UINT32 bayerPattern;          ///< Bayer pattern, 2u, 0: RGGB, 1: GRBG, 2: BGGR, 3: GBRG
    UINT32 lxStartL;              ///< Start block x index, 4u
    UINT32 lyStartL;              ///< Start block y index, 4u
    UINT32 bxStartL;              ///< Start subgrid x index within start block, 3u
    UINT32 byStartL;              ///< Start subgrid y index within start block, 3u
    UINT32 bxD1L;                 ///< x coordinate of top left pixel in start block/subgrid, 11u
    UINT32 byE1L;                 ///< y coordinate of top left pixel in start block/subgrid, 10u
    UINT32 byInitE1L;             ///< e1 * y_delta, 20uQ20
    UINT32 lxStartR;              ///< Start block x index, 4u
    UINT32 lyStartR;              ///< Start block y index, 4u
    UINT32 bxStartR;              ///< Start subgrid x index within start block, 3u
    UINT32 byStartR;              ///< Start subgrid y index within start block, 3u
    UINT32 bxD1R;                 ///< x coordinate of top left pixel in start block/subgrid, 11u
    UINT32 byE1R;                 ///< y coordinate of top left pixel in start block/subgrid, 10u
    UINT32 byInitE1R;             ///< e1 * y_delta, 20uQ20
};

/// @brief Debug Pedestal13 output from common IQ library
// NOWHINE NC004c: Share code with system team
struct Pedestal13DebugBuffer
{
    pedestal_1_3_0::pedestal13_rgn_dataType interpolationResult; ///< Interpolated chromatix data
    Pedestal13UnpackedField                 unpackedData;        ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Pedestal13 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Pedestal13Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to the input data
    /// @param  pData         Pointer to the intepolation result
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const Pedestal13InputData*                                      pInput,
        pedestal_1_3_0::pedestal13_rgn_dataType*                        pData,
        pedestal_1_3_0::chromatix_pedestal13Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                           pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InterpGridOptimization
    ///
    /// @brief  Calculate interp Grid Optimization
    ///
    /// @param  rawWidth    Sensor raw width
    /// @param  rawHeight   Sensor raw Height
    /// @param  pScaleCubic Pointer Cubic
    /// @param  pDeltaH     Pointer to Horizontal Delta
    /// @param  pDeltaV     Pointer to Vertical Delta
    /// @param  pSubgridH   Pointer to Subgrid Horizontal
    /// @param  pSubgridV   Pointer to Subgrid Vertical
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID InterpGridOptimization(
        INT32  rawWidth,
        INT32  rawHeight,
        INT32* pScaleCubic,
        INT32* pDeltaH,
        INT32* pDeltaV,
        INT32* pSubgridH,
        INT32* pSubgridV);
};
#endif // PEDESTAL13ETTING_H
