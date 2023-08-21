// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------


#ifndef __DS4TO1_CHROMATIX_H__
#define __DS4TO1_CHROMATIX_H__

#include "CommonDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DS4TO1_GENERAL_TAG
{
    // 8-Tap coefficient for pixels at locations 0 and 7.
    // For both Horizontal and vertical filters.
    // Default value (for 8-Tap typical) = 125
    // Typical value for binning option = 0
    // Coefficients values must comply with:
    // coeff_xx <= 256  (xx can be 07, 16 or 25)
    // coeff_07+coeff_16+coeff_25 <= 512
    // coeff_07+coeff_16+coeff_25 >= 256
    // format: 9u
    PARAM_UINT coeff_07;

    // 8-Tap coefficient for pixels at locations 1 and 6. For both Horizontal and vertical filters.
    // Default value (for 8-Tap typical) = 91
    // Typical value for binning option = 0
    // Value must comply same rules as in coeff_07
    // format: 9u
    PARAM_UINT coeff_16;

    // 8-Tap coefficient for pixels at locations 2 and 5.
    // For both Horizontal and vertical filters.
    // Default value (for 8-Tap typical) = 144
    // Typical value for binning option = 256
    // Value must comply same rules as in coeff_07
    // format: 9u
    PARAM_UINT coeff_25;

} DS4TO1_GENERAL;

typedef struct DS4to1_Chromatix_TAG
{
    DS4TO1_GENERAL general;
} DS4to1_Chromatix;

// ############ Functions ############
int32_t Validate_DS4to1_Chromatix( DS4to1_Chromatix* regStruct );
void SetDefaultVal_DS4to1_Chromatix( DS4to1_Chromatix* regStruct );
// ###################################
#ifdef __cplusplus
}
#endif


#endif //__DS4TO1_CHROMATIX_H__

