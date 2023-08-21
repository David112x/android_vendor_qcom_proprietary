// NOWHINE ENTIRE FILE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  lsc34setting.h
/// @brief lsc34 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LSC34SETTING_H
#define LSC34SETTING_H

#include "camxutils.h"
#include "chitintlessinterface.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "lsc_3_4_0.h"
#include "tintless_2_0_0.h"

static const UINT32 ROLLOFF_MESH_H_V34                  = 16;  // Note: VFE4 HW is 16
static const UINT32 ROLLOFF_MESH_V_V34                  = 12;  // Note: VFE4 HW is 12
static const UINT32 ROLLOFF_MESH_PT_H_V34               = 17;  // MESH_PT_H = MESH_H + 1
static const UINT32 ROLLOFF_MESH_PT_V_V34               = 13;  // MESH_PT_V = MESH_V + 1
static const UINT32 MESH_ROLLOFF_SIZE                   = ROLLOFF_MESH_PT_H_V34 * ROLLOFF_MESH_PT_V_V34;
static const UINT32 LSC34_Q_FACTOR                      = 10;
static const UINT32 LSC34_GAIN_MIN                      = 1024;
static const UINT32 LSC34_GAIN_MAX                      = 8191;
static const UINT32 LSC34_MESH_ROLLOFF_HORIZONTAL_GRIDS = 8;
static const UINT32 LSC34_MESH_ROLLOFF_VERTICAL_GRIDS   = 6;
static const UINT32 LSC34_BWIDTH_MIN                    = 8;
static const UINT32 LSC34_BWIDTH_MAX                    = 2047; // 8~2047 in HPG
static const UINT32 LSC34_BHEIGHT_MIN                   = 8;
static const UINT32 LSC34_BHEIGHT_MAX                   = 1023; // 8~1023 in HPG
static const UINT32 LSC34_MESHGRIDBWIDTH_MIN            = 17;
static const UINT32 LSC34_MESHGRIDBWIDTH_MAX            = 2047;
static const UINT32 LSC34_MESHGRIDBHEIGHT_MIN           = 8;
static const UINT32 LSC34_MESHGRIDBHEIGHT_MAX           = 1023;
static const UINT32 LSC34_INV_SUBBLOCK_WIDTH_MIN        = 512;
static const UINT32 LSC34_INV_SUBBLOCK_WIDTH_MAX        = 116508;
static const UINT32 LSC34_INV_SUBBLOCK_HEIGHT_MIN       = 1024;
static const UINT32 LSC34_INV_SUBBLOCK_HEIGHT_MAX       = 116508;
static const UINT32 LSC34_DELTA_Q_FACTOR                = 20;
static const UINT32 LSC34_DELTA_MIN                     = 0;
static const UINT32 LSC34_DELTA_MAX                     = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 LSC34_MIN_BLOCK_X                   = 0;
static const UINT32 LSC34_MAX_BLOCK_X                   = IQSettingUtils::MAXUINTBITFIELD(6);
static const UINT32 LSC34_MIN_BLOCK_Y                   = 0;
static const UINT32 LSC34_MAX_BLOCK_Y                   = IQSettingUtils::MAXUINTBITFIELD(6);
static const UINT32 LSC34_MIN_SUBGRID_X                 = 0;
static const UINT32 LSC34_MAX_SUBGRID_X                 = IQSettingUtils::MAXUINTBITFIELD(3);
static const UINT32 LSC34_MIN_SUBGRID_Y                 = 0;
static const UINT32 LSC34_MAX_SUBGRID_Y                 = IQSettingUtils::MAXUINTBITFIELD(3);
static const UINT32 LSC34_MIN_PIXEL_INDEX_X             = 0;
static const UINT32 LSC34_MAX_PIXEL_INDEX_X             = IQSettingUtils::MAXUINTBITFIELD(11);
static const UINT32 LSC34_MIN_PIXEL_INDEX_Y             = 0;
static const UINT32 LSC34_MAX_PIXEL_INDEX_Y             = IQSettingUtils::MAXUINTBITFIELD(10);
static const UINT32 LSC34_MIN_E_INIT                    = 0;
static const UINT32 LSC34_MAX_E_INIT                    = IQSettingUtils::MAXUINTBITFIELD(20);
static const FLOAT  LSC34_MIN_MESH_VAL                  = 1.0f;
static const FLOAT  LSC34_MAX_MESH_VAL                  = 7.999f;

/// @brief LSC34 module unpacked data
// NOWHINE NC004c: Share code with system team
struct LSC34UnpackedField
{
    // VFE4 and above: Mesh table sets with 3D support
    UINT16 enable;                   ///< Module enable flag, 1u
    UINT32 pixel_pattern;            ///< Pixel pattern, 2u, 0: RGGB, 1: GRBG, 2: BGGR, 3: GBRG
    UINT16 enable_3d;                ///<
    UINT16 left_img_wd;              ///<
    UINT16 bank_sel;                 ///< RAM bank 0 or 1 (~ double buffer)
    UINT32 pixel_offset;             ///< Pixel offset at output, (6+e)u

                                     // MSM8084 update to MLRO
    UINT32 num_meshgain_h;           ///< Number of horizontal mesh gains
    UINT32 num_meshgain_v;           ///< Number of vertical mesh gains

                                     ///< Left image
    /// @todo(CAMX-1771) Optimization this array structure
    UINT16 mesh_table_l[2][4][ROLLOFF_MESH_PT_V_V34][ROLLOFF_MESH_PT_H_V34];
    // Mesh gain table, 13uQ10, double buffered in HW
    // for 4 channels: 0->R, 1->Gr, 2->Gb, 3->B
    UINT32 intp_factor_l;            ///< Bicubic interpolation factor for left image, 0~3 (1/2/4/8 subgrids)
    UINT32 bwidth_l;                 ///< Subgrid width, 9u, 2*Bwidth is the real width
    UINT32 bheight_l;                ///< Subgrid height, 9u, 2*Bheight is the real height
    UINT32 meshGridBwidth_l;         ///< Meshgrid width, 9u, 2*MeshGridBwidth is the real width
                                     ///< not used in rolloff implementation, only as HW counters
    UINT32 meshGridBheight_l;        ///< Meshgrid height, 9u, 2*MeshGridBheight is the real height
                                     ///< not used in rolloff implementation, only as HW counters
    UINT32 x_delta_l;                ///< 1/Bwidth, 17uQ20
    UINT32 y_delta_l;                ///< 1/Bheight, 17uQ20
                                     ///< Right image
    UINT16 mesh_table_r[2][4][ROLLOFF_MESH_PT_V_V34][ROLLOFF_MESH_PT_H_V34];
    // Mesh gain table, 13uQ10, double buffered in HW
    // for 4 channels: 0->R, 1->Gr, 2->Gb, 3->B
    UINT32 intp_factor_r;            ///< Bicubic interpolation factor for right image, 0~3 (1/2/4/8 subgrids)
    UINT32 bwidth_r;                 ///< Subgrid width, 9u, 2*Bwidth is the real width
    UINT32 bheight_r;                ///< Subgrid height, 9u, 2*Bheight is the real height
    UINT32 meshGridBwidth_r;         ///< Meshgrid width, 9u, 2*MeshGridBwidth is the real width
                                     // not used in rolloff implementation, only as HW counters
    UINT32 meshGridBheight_r;        ///< Meshgrid height, 9u, 2*MeshGridBheight is the real height
                                     ///< not used in rolloff implementation, only as HW counters
    UINT32 x_delta_r;                ///< 1/Bwidth, 17uQ20
    UINT32 y_delta_r;                ///< 1/Bheight, 17uQ20
                                     ///< Stripe processing is not supported on 3D mode, so these are needed for left image
                                     ///< (as single image default) only
                                     ///< Left plane
    UINT32 lx_start_l;               ///< Start block x index, 6u
    UINT32 ly_start_l;               ///< Start block y index, 6u
    UINT32 bx_start_l;               ///< Start subgrid x index within start block, 3u
    UINT32 by_start_l;               ///< Start subgrid y index within start block, 3u
    UINT32 bx_d1_l;                  ///< x coordinate of top left pixel in start block/subgrid, 9u
    UINT32 by_e1_l;                  ///< y coordinate of top left pixel in start block/subgrid, 9u
    UINT32 by_init_e1_l;             ///< e1 * y_delta, 12uQ12 for <= VFE3.1 / 13uQ13 for >= VFE3.2
                                     ///< Right plane
    UINT32 lx_start_r;               ///< Start block x index, 6u
    UINT32 ly_start_r;               ///< Start block y index, 6u
    UINT32 bx_start_r;               ///< Start subgrid x index within start block, 3u
    UINT32 by_start_r;               ///< Start subgrid y index within start block, 3u
    UINT32 bx_d1_r;                  ///< x coordinate of top left pixel in start block/subgrid, 9u
    UINT32 by_e1_r;                  ///< y coordinate of top left pixel in start block/subgrid, 9u
    UINT32 by_init_e1_r;             ///< e1 * y_delta, 12uQ12 for <= VFE3.1 / 13uQ13 for >= VFE3.2
};

/// @brief LSC34 module unpacked data
// NOWHINE NC004c: Share code with system team
struct LSC34UnpackedData
{
    BOOL                             bIsDataReady;        ///< status of unpackedData
    LSC34UnpackedField               unpackedData;        ///< Calculated unpacked data
};
/// @brief LSC34 Debug Data
// NOWHINE NC004c: Share code with system team
struct LSC34DebugBuffer
{
    lsc_3_4_0::lsc34_rgn_dataType    interpolationResult; ///< Interpolated chromatix data
    LSC34UnpackedField               unpackedData;        ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements LSC34 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class LSC34Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the Input data
    /// @param  pData          Pointer to the interpolatin result
    /// @param  pModuleEnable  Pointer to the variable(s) to enable this module
    /// @param  pTintlessData  Pointer to the tintless interpolatin result
    /// @param  pUnpackedField Pointer to the unpacked value
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        LSC34InputData*                                       pInput,
        lsc_3_4_0::lsc34_rgn_dataType*                        pData,
        lsc_3_4_0::chromatix_lsc34Type::enable_sectionStruct* pModuleEnable,
        tintless_2_0_0::tintless20_rgn_dataType*              pTintlessData,
        VOID*                                                 pUnpackedField);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateTintlessSetting
    ///
    /// @brief  Calculate the Tintless output table
    ///
    /// @param  pInput               Pointer to the Input data
    /// @param  pData                Pointer to the interpolatin result
    /// @param  pTintlessData        Pointer to the Tintless Interpolation Data
    /// @param  POutput              Pointer to the Tintless Algo Output
    /// @param  subGridWidth         Width of Mesh Sub grid
    /// @param  subGridHeight        Height of Mesh Sub grid
    /// @param  interpolateFactor    InterpolateFactor
    /// @param  meshHorizontalOffset Horozontal Offset of Mesh Grid
    /// @param  meshVerticalOffset   Vertical Offset of Mesh Subgrid
    ///
    /// @return  returns TRUE if Success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateTintlessSetting(
        LSC34InputData*                          pInput,
        lsc_3_4_0::lsc34_rgn_dataType*           pData,
        tintless_2_0_0::tintless20_rgn_dataType* pTintlessData,
        lsc_3_4_0::lsc34_rgn_dataType*           pOutput,
        UINT32                                   subGridWidth,
        UINT32                                   subGridHeight,
        UINT32                                   interpolateFactor,
        UINT32                                   meshHorizontalOffset,
        UINT32                                   meshVerticalOffset);

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
    /// @param  pNH         Channel Hor
    /// @param  pNV         Channel Ver
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID InterpGridOptimization(
        INT rawWidth,
        INT rawHeight,
        INT* pScaleCubic,
        INT* pDeltaH,
        INT* pDeltaV,
        INT* pSubgridH,
        INT* pSubgridV,
        INT* pNH,
        INT* pNV);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MeshRolloffScaleRolloff
    ///
    /// @brief  This function is used to resample the ideal Rolloff table from the full-resolution sensor to the
    ///         (CAMIF) output size, which is decided by the sensor mode
    ///
    /// @param  pMeshIn      input ideal Rolloff table (13x17)at the full sensor
    /// @param  pMeshOut     output rolloff table (13x17) at the current output resolution
    /// @param  fullWidth    full-resolution width
    /// @param  fullHeight   full-resolution height
    /// @param  outputWidth  output width
    /// @param  outputHeight output height
    /// @param  offsetX      x-index of the top-left corner of output image on the full-resolution sensor
    /// @param  offsetY      y-index of the top-left corner of output image on the full-resolution sensor
    /// @param  scaleX       sensor scaling factor in X direction (=binning_factor * digal_sampling_factor)
    /// @param  scaleY       sensor scaling factor in Y direction (=binning_factor * digal_sampling_factor)
    /// @param  pMeshH       pMeshH Horizontal Mesh
    /// @param  pMeshV       pMeshV Vertical Mesh
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID MeshRolloffScaleRolloff(
        const FLOAT* pMeshIn,
        FLOAT*       pMeshOut,
        INT          fullWidth,
        INT          fullHeight,
        INT          outputWidth,
        INT          outputHeight,
        INT          offsetX,
        INT          offsetY,
        INT          scaleX,
        INT          scaleY,
        INT*         pMeshH,
        INT*         pMeshV);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// scaleRolloffTable
    ///
    /// @brief  Calculate scaleRolloffTable
    ///
    /// @param  pInputData  Pointer to input
    /// @param  pData       Pointer to the intepolation result
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID scaleRolloffTable(
        const LSC34InputData*          pInput,
        lsc_3_4_0::lsc34_rgn_dataType* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MeshExtend1block
    ///
    /// @brief  Calculate MeshExtend1block
    ///
    /// @param  pMeshIn  Input Mesh Table
    /// @param  pMeshOut Output Mesh Table
    /// @param  nx       perChannel width
    /// @param  ny       perChannel Height
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID MeshExtend1block(
        const FLOAT* pMeshIn,
        FLOAT*       pMeshOut,
        INT          nx,
        INT          ny);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BicubicF
    ///
    /// @brief  Calculate BicubicF
    ///
    /// @param  fs    Input Value
    /// @param  pFc0  Output Cofficient 0
    /// @param  pFc1  Output Cofficient 1
    /// @param  pFc2  Output Cofficient 2
    /// @param  pFc3  Output Cofficient 3
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID BicubicF(
        FLOAT  fs,
        FLOAT* pFc0,
        FLOAT* pFc1,
        FLOAT* pFc2,
        FLOAT* pFc3);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// m_meshHorizontal Number of horizontal mesh
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT32 s_meshHorizontal;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// m_meshVertical Number of Vertical mesh
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT32 s_meshVertical;
};
#endif // LSC34SETTING_H
