// NOWHINE ENTIRE FILE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  lsc40setting.h
/// @brief lsc40 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LSC40SETTING_H
#define LSC40SETTING_H

#include "camxutils.h"
#include "chitintlessinterface.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "lsc_4_0_0.h"
#include "tintless_2_0_0.h"

struct ALSCHelperOutput;

static const UINT32 LSC_MESH_H_V40                  = 16;  // Note: VFE4 HW is 16
static const UINT32 LSC_MESH_V_V40                  = 12;  // Note: VFE4 HW is 12
static const UINT32 LSC_MESH_PT_H_V40               = 17;  // MESH_PT_H = MESH_H + 1
static const UINT32 LSC_MESH_PT_V_V40               = 13;  // MESH_PT_V = MESH_V + 1
static const UINT32 MESH_ROLLOFF40_SIZE             = LSC_MESH_PT_H_V40 * LSC_MESH_PT_V_V40;

static const UINT16 LSC40_BPP                           = 14;
static const UINT32 LSC40_Q_FACTOR                      = 10;
static const UINT32 LSC40_GAIN_MIN                      = 1024;
static const UINT32 LSC40_GAIN_MAX                      = 8191;
static const UINT32 LSC40_MESH_ROLLOFF_HORIZONTAL_GRIDS = 8;
static const UINT32 LSC40_MESH_ROLLOFF_VERTICAL_GRIDS   = 6;
static const UINT32 LSC40_BWIDTH_MIN                    = 8;
static const UINT32 LSC40_BWIDTH_MAX                    = 2047; // 8~2047 in HPG
static const UINT32 LSC40_BHEIGHT_MIN                   = 8;
static const UINT32 LSC40_BHEIGHT_MAX                   = 1023; // 8~1023 in HPG
static const UINT32 LSC40_MESHGRIDBWIDTH_MIN            = 17;
static const UINT32 LSC40_MESHGRIDBWIDTH_MAX            = 2047;
static const UINT32 LSC40_MESHGRIDBHEIGHT_MIN           = 8;
static const UINT32 LSC40_MESHGRIDBHEIGHT_MAX           = 1023;
static const UINT32 LSC40_INV_SUBBLOCK_WIDTH_MIN        = 512;
static const UINT32 LSC40_INV_SUBBLOCK_WIDTH_MAX        = 116508;
static const UINT32 LSC40_INV_SUBBLOCK_HEIGHT_MIN       = 1024;
static const UINT32 LSC40_INV_SUBBLOCK_HEIGHT_MAX       = 116508;
static const UINT32 LSC40_DELTA_Q_FACTOR                = 20;
static const UINT32 LSC40_DELTA_MIN                     = 0;
static const UINT32 LSC40_DELTA_MAX                     = IQSettingUtils::MAXUINTBITFIELD(17);
static const UINT32 LSC40_MIN_BLOCK_X                   = 0;
static const UINT32 LSC40_MAX_BLOCK_X                   = 15;
static const UINT32 LSC40_MIN_BLOCK_Y                   = 0;
static const UINT32 LSC40_MAX_BLOCK_Y                   = 11;
static const UINT32 LSC40_MIN_SUBGRID_X                 = 0;
static const UINT32 LSC40_MAX_SUBGRID_X                 = IQSettingUtils::MAXUINTBITFIELD(3);
static const UINT32 LSC40_MIN_SUBGRID_Y                 = 0;
static const UINT32 LSC40_MAX_SUBGRID_Y                 = IQSettingUtils::MAXUINTBITFIELD(3);
static const UINT32 LSC40_MIN_PIXEL_INDEX_X             = 0;
static const UINT32 LSC40_MAX_PIXEL_INDEX_X             = 2029;
static const UINT32 LSC40_MIN_PIXEL_INDEX_Y             = 0;
static const UINT32 LSC40_MAX_PIXEL_INDEX_Y             = 1005;
static const UINT32 LSC40_MIN_E_INIT                    = 0;
static const UINT32 LSC40_MAX_E_INIT                    = 932064;
static const FLOAT  LSC40_MIN_MESH_VAL                  = 1.0f;
static const FLOAT  LSC40_MAX_MESH_VAL                  = 7.999f;
static const UINT16 LSC40_MIN_LUMA_WEIGHT_BASE_SCALE    = 0;
static const UINT16 LSC40_MAX_LUMA_WEIGHT_BASE_SCALE    = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 LSC40_MIN_LUMA_WEIGHT_BASE_MIN      = 0;
static const UINT16 LSC40_MAX_LUMA_WEIGHT_BASE_MIN      = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(LSC40_BPP));
static const UINT16 LSC40_MIN_LUMA_WEIGHT_MIN           = 0;
static const UINT16 LSC40_MAX_LUMA_WEIGHT_MIN           = 128;
static const UINT16 LSC40_GRID_MEAN_MAX                 = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 LSC40_GRID_MEAN_MIN                 = 0;
static const UINT16 LSC40_GRID_GAIN_MAX                 = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(12));
static const UINT16 LSC40_GRID_GAIN_MIN                 = 0;
static const UINT16 LSC40_LUMA_WEIGHT_BASE_SCALE_Q_VALUE = 2048;
static const UINT16 LSC40_THRESHOLD_HIGHLIGHT_MIN        = 0;
static const UINT16 LSC40_THRESHOLD_HIGHLIGHT_MAX        = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 LSC40_THRESHOLD_LOWLIGHT_MIN         = 0;
static const UINT16 LSC40_THRESHOLD_LOWLIGHT_MAX         = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(14));
static const UINT16 LSC40_ADAPTIVE_GAIN_LOW_MIN          = 1024;
static const UINT16 LSC40_ADAPTIVE_GAIN_LOW_MAX          = 2048;
static const UINT16 LSC40_ADAPTIVE_GAIN_HIGH_MIN         = 0;
static const UINT16 LSC40_ADAPTIVE_GAIN_HIGH_MAX         = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(10));
static const UINT16 LSC40_LOWLIGHT_GAIN_STRENGTH_MIN     = 0;
static const UINT16 LSC40_LOWLIGHT_GAIN_STRENGTH_MAX     = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(4));
static const UINT16 LSC40_HIGHLIGHT_GAIN_STRENGETH_MIN   = 0;
static const UINT16 LSC40_HIGHLIGHT_GAIN_STRENGTH_MAX    = static_cast<UINT16>(IQSettingUtils::MAXUINTBITFIELD(4));
static const UINT32 ALSC_BG_GRID_H                       = 64;
static const UINT32 ALSC_BG_GRID_V                       = 48;
static const UINT32 ALSC_SCRATCH_BUFFER_SIZE_IN_DWORD    = (ALSC_BG_GRID_H * ALSC_BG_GRID_V * ALSC_BUFFER_LIST_SIZE);

/// @brief LSC40 module unpacked data
// NOWHINE NC004c: Share code with system team
struct LSC40UnpackedField
{

    UINT16 enable;             // 1u,
    UINT32 pixel_pattern;      // Bayer pattern, 2u, 0: RGGB, 1: GRBG, 2: BGGR, 3: GBRG
    UINT16 bank_sel;           // RAM bank 0 or 1 (~ double buffer)
    UINT32 pixel_offset;       // Pixel offset at output, (6+e)u

    // MSM8084 update to MLRO
    UINT32 num_meshgain_h;     // Number of horizontal mesh gains, n mean n+2
    UINT32 num_meshgain_v;     // Number of vertical mesh gains, n mean n+2

    // Left image
    UINT16 mesh_table[2][4][LSC_MESH_PT_V_V40][LSC_MESH_PT_H_V40];
    // Mesh gain table, 13uQ10, double buffered in HW
    // for 4 channels: 0->R, 1->Gr, 2->Gb, 3->B
    UINT32 intp_factor;        // Bicubic interpolation factor for left image, 0~3 (1/2/4/8 subgrids)
    UINT32 Bwidth;             // Subgrid width, 9u, 2*Bwidth is the real width, n mean n+1
    UINT32 Bheight;            // Subgrid height, 9u, 2*Bheight is the real height, n mean n+1
    UINT32 MeshGridBwidth;     // Meshgrid width, 9u, 2*MeshGridBwidth is the real width, n mean n+1
    // not used in rolloff implementation, only as HW counters
    UINT32 MeshGridBheight;    // Meshgrid height, 9u, 2*MeshGridBheight is the real height, n mean n+1
    // not used in rolloff implementation, only as HW counters
    UINT32 x_delta;            // Actual value 1/Bwidth, 17uQ20
    UINT32 y_delta;            // Actual value 1/Bheight, 17uQ20

    // (as single image default) only
    // Left plane
    UINT32 Lx_start;           // Start block x index, 6u
    UINT32 Ly_start;           // Start block y index, 6u
    UINT32 Bx_start;           // Start subgrid x index within start block, 3u
    UINT32 By_start;           // Start subgrid y index within start block, 3u
    UINT32 Bx_d1;              // x coordinate of top left pixel in start block/subgrid, 9u
    UINT32 By_e1;              // y coordinate of top left pixel in start block/subgrid, 9u
    UINT32 By_init_e1;         // e1 * y_delta, 12uQ12 for <= VFE3.1 / 13uQ13 for >= VFE3.2

    // ALSC: adaptive roll-off
    UINT16 ALSC_enable;

    UINT16 grids_mean[2][LSC_MESH_PT_V_V40][LSC_MESH_PT_H_V40];
    // grid mean table, 14u, double buffered in HW

    UINT16 grids_gain[2][LSC_MESH_PT_V_V40][LSC_MESH_PT_H_V40];
    // grid gain table, 8uQ6, double buffered in HW

    UINT16 luma_weight_base_scale;      // luma weigting base for pixel value 2048
    UINT16 luma_weight_base_min;        // minimum value for luma weight base value

    UINT16 luma_weight_min;             // minimum luma weight

    // Crop
    UINT16 crop_enable;
    UINT16 first_pixel;
    UINT16 last_pixel;
    UINT16 first_line;
    UINT16 last_line;
};

struct LSC40UnpackedData
{
    BOOL                             bIsDataReady;        ///< status of unpackedData
    LSC40UnpackedField               unpackedData;        ///< Calculated unpacked data
};

/// @brief LSC40 Debug Data
// NOWHINE NC004c: Share code with system team
struct LSC40DebugBuffer
{
    lsc_4_0_0::lsc40_rgn_dataType    interpolationResult; ///< Interpolated chromatix data
    LSC40UnpackedField               unpackedData;        ///< Calculated unpacked data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements LSC40 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class LSC40Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput         Pointer to the Input data
    /// @param  pData          Pointer to the interpolatin result
    /// @param  pReserveType   Pointer to the reserveType of this module
    /// @param  pModuleEnable  Pointer to the variable(s) to enable this module
    /// @param  pTintlessData  Pointer to the tintless interpolatin result
    /// @param  pUnpackedField Pointer to the unpacked value
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        LSC40InputData*                                       pInput,
        lsc_4_0_0::lsc40_rgn_dataType*                        pData,
        lsc_4_0_0::chromatix_lsc40_reserveType*               pReserveType,
        lsc_4_0_0::chromatix_lsc40Type::enable_sectionStruct* pModuleEnable,
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
        LSC40InputData*                          pInput,
        lsc_4_0_0::lsc40_rgn_dataType*           pData,
        tintless_2_0_0::tintless20_rgn_dataType* pTintlessData,
        lsc_4_0_0::lsc40_rgn_dataType*           pOutput,
        UINT32                                   subGridWidth,
        UINT32                                   subGridHeight,
        UINT32                                   interpolateFactor,
        UINT32                                   meshHorizontalOffset,
        UINT32                                   meshVerticalOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateALSC Setting
    ///
    /// @brief  Calculate the ALSC grid gain and output
    /// @param  pInput               Pointer to the Input data
    ///
    /// @return  returns TRUE if Success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateALSCSetting(
        LSC40InputData*                                       pInput,
        lsc_4_0_0::chromatix_lsc40Type::enable_sectionStruct* pModuleEnable,
        lsc_4_0_0::lsc40_rgn_dataType*                        pData,
        ALSCHelperOutput*                                     gainOutput);

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
        const LSC40InputData*          pInput,
        lsc_4_0_0::lsc40_rgn_dataType* pData);

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
    /// calculateGridsMean
    ///
    /// @brief  Calculate Grids Mean
    ///
    /// @param  img_in              Input Image
    /// @param  bayer_pattern       Bayer Pattern
    /// @param  rGain               R Channel Gain
    /// @param  gGain               G Channel Gain
    /// @param  bGain               B Channel Gain
    /// @param  mesh_gains          Mesh Gain Table
    /// @param  number_cell_column  Cell Height
    /// @param  number_cell_row     Cell Row
    /// @param  width               Image Width
    /// @param  height              Image Height
    /// @param  grids_mean          Output Grids Mean Table
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID calculateGridsMean(
        UINT16 *img_in,
        UINT32 bayer_pattern,
        UINT16 rGain,
        UINT16 gGain,
        UINT16 bGain,
        UINT16 mesh_gains[][LSC_MESH_PT_H_V40],
        UINT16 number_cell_column,
        UINT16 number_cell_row,
        UINT32 width,
        UINT32 height,
        UINT32 *grids_mean);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// calculateGridsGain
    ///
    /// @brief  Calculate Grids Gain
    ///
    /// @param  grids_mean  Input Grids Mean Table
    /// @param  number_cell_column  Cell Height
    /// @param  number_cell_row     Cell Row
    /// @param  grids_gain  Output Grids Gain Table
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID calculateGridsGain(
        UINT32 *grids_mean,
        UINT16 number_cell_column,
        UINT16 number_cell_row,
        UINT32 *grids_gain);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// m_meshHorizontal Number of horizontal mesh
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT32 s_meshHorizontal;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// m_meshVertical Number of Vertical mesh
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static INT32 s_meshVertical;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// s_validPrevTintlessOutput The previous frame tintless output valid flag
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL  s_validPrevTintlessOutput;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// s_preTintlessOutputTable The previous frame tintless output table
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static lsc_4_0_0::lsc40_rgn_dataType s_preTintlessOutputTable;
};
#endif // LSC40SETTING_H
