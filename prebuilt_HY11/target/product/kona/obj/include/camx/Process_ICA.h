// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

#ifndef __PROCESS_ICA_H__
#define __PROCESS_ICA_H__

#include "ICA_Chromatix.h"
#include "ICA_Registers_v30.h"
#include "NcLibContext.h"
#include "NcLibWarpCommonDef.h"
#include "ipe_data.h"

struct GeoLibIcaPassMapping;

#ifdef __cplusplus
extern "C" {
#endif

// RTODO: remove when added to FW API
typedef struct _IcaGeoPassParameters
{
    uint32_t INPUT_FRAME_WIDTH_MINUS_1;         // 14u
    uint32_t INPUT_FRAME_HEIGHT_MINUS_1;        // 14u
    uint32_t OUTPUT_STRIP_HEIGHT_MINUS_1;       // 14u
    uint32_t CONTROLLER_VALID_WIDTH_MINUS_1;    // 19uQ5 as 0.14.5
    uint32_t CONTROLLER_VALID_HEIGHT_MINUS_1;   // 19uQ5 as 0.14.5
    uint32_t CTC_HALF_OUTPUT_FRAME_WIDTH;       // 14u
    uint32_t CTC_HALF_OUTPUT_FRAME_HEIGHT;      // 14u
    uint32_t CTC_O2V_SCALE_FACTOR_X;            // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_O2V_SCALE_FACTOR_Y;            // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_O2V_OFFSET_X;                  // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_O2V_OFFSET_Y;                  // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_V2I_INV_SCALE_FACTOR_X;        // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_V2I_INV_SCALE_FACTOR_Y;        // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_V2I_OFFSET_X;                  // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_V2I_OFFSET_Y;                  // exponenta-mantissa. T480: 6s-18s. T17x: 6s-16s
    uint32_t CTC_INPUT_COORD_PRECISION;         // 3u

    uint32_t outputWidthPixels;
    uint32_t outputHeightPixels;
    uint32_t forceWarpOn;                       // if 1 must set MODE register to WARP_ON (0), CTC cannot be bypassed
} IcaGeoPassParameters;

typedef struct _IcaGeoParameters
{
    IcaGeoPassParameters ica1[PASS_NAME_MAX];
    IcaGeoPassParameters ica2[PASS_NAME_MAX];
} IcaGeoParameters;

/**
*  @brief   Calculate ICA registers and FW struct which are set directly by Chromatix.
*
*  @param [in]      chromatixStruct             pointer to the pass Chromatix struct
*  @param [in]      icaIsGridEnabledByFlow      indication if Grid is set by Chromatix or SW.
*                                               if it is set to 0, it is taken from Chromatix
*                                               else it is set by SW (ICA_ProcessNonChromatixParams).
*  @param [out]     regStruct                   pointer to the pass register struct
*  @param [out]     fwStruct                    FW struct that is filled by the function.
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t ICA_ProcessNcLib(
    const ICA_Chromatix* chromatixStruct,
    uint8_t icaIsGridEnabledByFlow,
    ICA_REG_v30* regStruct,
    IcaParameters* fwStruct);

/**
*  @brief   Calculate ICA non Chromatix registers and FW structs .
*
*  @param [in]      inGridFlow                  pointer to grid. Has to be grid of exact size of ICA grid :
*                                                 Napali (ICA v1.0): ICA_GRID_TRANSFORM_WIDTH_V10 x ICA_V10_GRID_TRANSFORM_HEIGHT
*                                                 Hana (ICA v2.0): ICA_V20_GRID_TRANSFORM_WIDTH x ICA_V20_GRID_TRANSFORM_HEIGHT
*                                                 Kona (ICA v3.0) - one of:
*                                                   ICA_V30_GRID_TRANSFORM_LOW_RES_WIDTH x ICA_V30_GRID_TRANSFORM_LOW_RES_HEIGHT
*                                                   ICA_V30_GRID_TRANSFORM_HI_RES_WIDTH x ICA_V30_GRID_TRANSFORM_HI_RES_HEIGHT
*  @param [in]      persp                       pointer to perspective transform matrices.
*                                               There are up to be max 9 matrices.
*  @param [in]      geo                         pointer to Multi-pass ICA geometrical mapping struct.
*  @param [in]      icaIsGridEnabledByFlow      indication if Grid is set by Chromatix or SW.
*                                               if it is set to 0, it is taken from Chromatix (ICA_ProcessNcLib)
*                                               else it is set by SW (this function).
*  @param [in]      icaIsRefinementEnabled      indication if Refinement is enabled.
*  @param [in]      icaVersion                  ICA HW version: 10 for Napali, 20 for Hana, 20 or 30 for Kona.
*  @param [out]     regStruct                   pointer to the pass register struct
*  @param [out]     fwStruct                    FW struct that is filled by the function.
*  @param [out]     fwRegStruct                 FW Registers structs for each pass that is filled by the function.
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t ICA_ProcessNonChromatixParams(
    const NcLibWarpGrid* inGridFlow,
    const NcLibWarpMatrices* persp,
    const GeoLibIcaPassMapping* geo,
    uint8_t icaIsGridEnabledByFlow,
    uint32_t icaIsRefinementEnabled,
    uint32_t icaVersion,
    ICA_REG_v30* regStruct,
    IcaParameters* fwStruct,
    IcaGeoPassParameters fwRegStruct[]);

/**
*  @brief   Gets the inputs of ICA_ProcessNonChromatixParams and return true or false if they are valid for ICA
*
*  @param [in]      inGridFlow                  pointer to grid. Has to be grid of exact size of ICA grid :
*                                                 Napali (ICA v1.0): ICA_GRID_TRANSFORM_WIDTH_V10 x ICA_V10_GRID_TRANSFORM_HEIGHT
*                                                 Hana (ICA v2.0): ICA_V20_GRID_TRANSFORM_WIDTH x ICA_V20_GRID_TRANSFORM_HEIGHT
*                                                 Kona (ICA v3.0) - one of:
*                                                   ICA_V30_GRID_TRANSFORM_LOW_RES_WIDTH x ICA_V30_GRID_TRANSFORM_LOW_RES_HEIGHT
*                                                   ICA_V30_GRID_TRANSFORM_HI_RES_WIDTH x ICA_V30_GRID_TRANSFORM_HI_RES_HEIGHT
*  @param [in]      persp                       pointer to perspective transform matrices.
*                                               There are up to be max 9 matrices.
*  @param [in]      icaIsGridEnabledByFlow      indication if Grid is set by Chromatix or SW.
*                                               if it is set to 0, it is taken from Chromatix (ICA_ProcessNcLib)
*                                               else it is set by SW (this function).
*  @param [in]      icaIsRefinementEnabled      indication if Refinement is enabled.
*  @param [in]      icaVersion                  ICA HW version: 10 for Napali, 20 for Hana, 20 or 30 for Kona.
*
*  @return NC_LIB_SUCCESS in case of success, otherwise failed.
*/
int32_t ValidateIcaNonChromatixParams(
    const NcLibWarpGrid* inGridFlow,
    const NcLibWarpMatrices* persp,
    uint8_t  icaIsGridEnabledByFlow,
    uint32_t icaIsRefinementEnabled,
    uint32_t icaVersion);


/**
*  @brief   Set default values for ICA FW struct
*
*  @param [out]     fwStruct               The FW struct that is set.
*  @param [in]      enable                 Whether to enable invalidPixelModeInterpolation and Perspective
*
*  @return void
*/
void SetDefaultsForIcaStruct(IcaParameters* fwStruct, bool enable = true);

/**
*  @brief   Get ICA version typical for current chip
*
*  @return 10 for Napali, 20 for Hana, 30 for Kona.
*/
uint32_t NcLibGetIpeIcaVersion();

#ifdef __cplusplus
}
#endif


#endif //__PROCESS_ICA_H__
