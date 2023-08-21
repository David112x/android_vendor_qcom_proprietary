// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  registration.h
/// @brief registration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE DC002 :
// NOWHINE FILE GR017 : intrinsic type
// NOWHINE FILE CF003 :
// NOWHINE FILE CP006 :
// NOWHINE FILE CP010 :
// NOWHINE FILE CP021 :


// NOWHINE FILE NC003 :
#ifndef REGISTRATION_H
#define REGISTRATION_H

#include <stdint.h>

// NOWHINE PR007:

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
struct RegImage
{
    void* pData;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t bpp;
    uint32_t have2bps;
};

struct RegPostprocessing
{
    uint32_t resizeToWidth;  // = 0
    uint32_t resizeToHeight; // = 0
    uint32_t newOriginX;     // = 0
    uint32_t newOriginY;     // = 0
    double resizeFactorX;    // = 1
    double resizeFactorY;    // = 1
};

// NOWHINE NC010:
int register_mf(RegImage* anchor, RegImage* ref, RegPostprocessing* postprocessing, float gmv[9], uint32_t* pConfidence);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // REGISTRATION_H
