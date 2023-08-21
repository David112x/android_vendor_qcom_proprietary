// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------


#ifndef __LRME_REGISTERS_H__
#define __LRME_REGISTERS_H__

#include "CommonDefs.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LRME_REG_TAG
{

    // Enable bit for the unit (part of CLC interface)
    // format: 1u
    FIELD_UINT en;

    // Whether to expect valid data on Ref input.
    // format: 1u
    FIELD_UINT isRefValid;

    // Whether downscaled Tar image should be used. ‘0’ means no downscaling is performed, in which case no output is expected to DS2 client.
    // format: 1u
    FIELD_UINT doDownscaling;

    // 0 = Linear PD10; 1 = Linear PD8; 2 = Y-only 8bps (for example, NV12-Y will appear this way); 3 = Y-only 10bps (for example,  PLAIN16_10_LSB and PLAIN16_10_MSB will appear this way)
    // format: 2u
    FIELD_UINT tarDataFormat;

    // 0 = Linear PD10; 1 = Linear PD8; 2 = Y-only 8bps (for example, NV12-Y will appear this way); 3 = Y-only 10bps (for example,  PLAIN16_10_LSB and PLAIN16_10_MSB will appear this way)
    // format: 2u
    FIELD_UINT refDataFormat;

    // Horizontal search range to be used. The number of horizontal search locations is 2*searchRangeX + 1.  Search range is restricted to 2 options only: +/-12 x +/-8 or +/-8 x +/-6
    // format: 4u
    FIELD_UINT searchRangeX;

    // Vertical search range to be used. The number of vertical search locations is 2*searchRangeY + 1. Search range is restricted to 2 options only: +/-12 x +/-8 or +/-8 x +/-6
    // format: 4u
    FIELD_UINT searchRangeY;

    // Horizontal step (in pixels) between consecutive blocks to be processed in Tar image.
    // format: 4u
    FIELD_UINT stepX;

    // Vertical step (in pixels) between consecutive blocks to be processed in Tar image.
    // format: 4u
    FIELD_UINT stepY;

    // Offset used to determine zeroMvX - the location around which the search is performed in Ref frame. Should be bigger than -searchRangeX. Note: configuration of LRME unit by SW should be such that for every block there will be at least one valid search location (within the search range).
    // format: 5s
    FIELD_INT offsetX;

    // Offset used to determine zeroMvY - the location around which the search is performed in Ref frame. Should be bigger than -searchRangeY
    // format: 5s
    FIELD_INT offsetY;

    // Whether to calculate NSAD (if this flag is 0 - calculate regular SAD)
    // format: 1u
    FIELD_UINT isNormalizedSad;

    // Whether sub-pixel ME (Optical Flow) is enabled
    // format: 1u
    FIELD_UINT subPelSearchEnable;

    // Output format sent to Res client: 0 = short format; 1 = long format
    // format: 1u
    FIELD_UINT resultsFormat;

    // Whether to swap Tar (after possible downscaling and crop) and Ref inputs
    // format: 1u
    FIELD_UINT swapInputs;

    // Maximum allowed NSAD (or SAD, depending on isNormalizedSad parameter). If exceeded this threshold (problem in ME) the block will be marked invalid.
    // format: 15u
    FIELD_UINT maxAllowedSad;

    // Minimum allowed tarMAD value. If below this value (non-informative block) - the block will be marked invalid.
    // format: 10u
    FIELD_UINT minAllowedTarMad;

    // Locations with regular SAD difference below this threshold are considered to have exactly the same similarity measure (even if normalized SAD difference is big).
    // format: 10u
    FIELD_UINT meaningfulSadDiff;

    // Determines the lower limit of denominator used in normalized difference of SAD calculation to avoid overflow.
    // format: 10u
    FIELD_UINT minSadDiffDenom;

    // Height of Tar frame after possible downscale and crop (in pixels)
    // format: 10u
    FIELD_UINT tarHeight;

    // Height of Ref frame (in pixels)
    // format: 10u
    FIELD_UINT refHeight;

    // Whether input cropper is enabled. If disabled - no need to configure crop parameters.
    // format: 1u
    FIELD_UINT dsCropEnable;

    // Horizontal cropping start location. This is the index of first valid output pixel.
    // format: 9u
    FIELD_UINT dsCropHorizStart;

    // Horizontal cropping end location. This is the index of last valid output pixel.
    // format: 10u
    FIELD_UINT dsCropHorizEnd;

    // Vertical cropping start location. This is the index of first valid output row.
    // format: 10u
    FIELD_UINT dsCropVertStart;

    // Vertical cropping end location. This is the index of last valid output row.
    // format: 14u
    FIELD_UINT dsCropVertEnd;

    // Relevant only for PD format. Defines where the valid data starts in chunks of 128 bits coming from Tar FE. Meaning of the value is inferred based on tarDataFormat
    // format: 3u
    FIELD_UINT tarPdUnpacker_Phase;

    // Relevant only for PD format. Defines the width of image coming from Tar FE (in pixels)
    // format: 11u
    FIELD_UINT tarPdUnpacker_LineWidth;

    // Relevant only for PD format. Defines where the valid data starts inside the 128 bits coming from Ref FE. Meaning of the value is inferred based on refDataFormat
    // format: 3u
    FIELD_UINT refPdUnpacker_Phase;

    // Relevant only for PD format. Defines the width of image coming from Ref FE (in pixels)
    // format: 9u
    FIELD_UINT refPdUnpacker_LineWidth;

    // Provides a requirement on minimal normalized difference of SADs at distance 1-9 pixels (for big distances the expected differences should be big). Default values: 0, 128, 128, 128, 128, 128, 128, 128 (no penalty to single pixel distance; afterwards same penalty for all other distances)
    // format: 9u
    FIELD_UINT robustnessMeasureDistMap[8];

} LRME_REG;

// ############ Functions ############
int32_t Validate_LRME_REG( LRME_REG* regStruct );
void SetDefaultVal_LRME_REG( LRME_REG* regStruct );
// ###################################
#ifdef __cplusplus
}
#endif


#endif //__LRME_REGISTERS_H__

