////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxswprocessalgo.h
/// @brief camxswprocessalgo interface
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXSWPROCESSALGO_H
#define CAMXSWPROCESSALGO_H

#include "camxdefs.h"

CAMX_NAMESPACE_BEGIN
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// Max Image Planes
static const UINT32 SWProcMaxPlaneCount = 3;

/// @brief Image Plane Type
enum SWProcPlaneType
{
    SWProcPlaneType_Y = 0,  ///< Luma plane
    SWProcPlaneType_CB_CR,  ///< CbCr plane
    SWProcPlaneType_CR_CB   ///< CrCb plane
};

/// @brief Image Plane
struct SWProcImagePlane
{
    SWProcPlaneType type;       ///< Plane type
    UINT8*          pBuffer;    ///< Plane buffer location
    UINT32          stride;     ///< Stride of Image Plane
    UINT32          scanline;   ///< Scanline of image plane
};

/// @brief Image Type
struct SWProcImage
{
    UINT32              width;                          ///< Image width
    UINT32              height;                         ///< Image height
    UINT32              planeCount;                     ///< Number of planes
    SWProcImagePlane    planes[SWProcMaxPlaneCount];    ///< Plane information
};

/// @brief SW Processing GTM parameters
struct SWProcGTMParams
{
    BOOL            bIsInlineProcessing;    ///< Whether to process inline or not
    SWProcImage     image;                  ///< Input frame for sw processing, becomes output as well if inline processing
    SWProcImage     outputImage;            ///< Output frame for sw processing, not used if inline processing
    BOOL            bIsAdaptiveGTM;         ///< Whether or not Adaptive GTM processing
    BOOL            bIsFixedGTM;            ///< Whether to use Fixed GTM for tone mapping or not
    FLOAT           gammaFactor;            ///< Gamma value to be applied globally. Not used for Adaptive GTM processing
    FLOAT           gainFactor;             ///< Gain value to be applied globally. Not used for Adaptive GTM processing
    FLOAT           darkKnee;               ///< Knee point (0-1) to define end of dark region.
                                            ///  Not used for Adaptive GTM processing
    FLOAT           darkGainFactor;         ///< Gain factor to applied on dark region (0 - darkKnee).
                                            ///  Not used for Adaptive GTM processing
};

/// @brief SW Processing LTM parameters
struct SWProcLTMParams
{
    BOOL            bIsInlineProcessing;    ///< Whether to process inline or not
    SWProcImage     image;                  ///< Input frame for sw processing, becomes output as well if inline processing
    SWProcImage     outputImage;            ///< Output frame for sw processing, not used if inline processing
    FLOAT           contrastFactor;         ///< GTM param to stretch contrast. 10 results in 2.5 times contrast increase
    FLOAT           normalizedGrayPoint;    ///< Normalization gray point for GTM contrast stretching
    FLOAT           clipLimit;              ///< Clip threshold for limiting gain
    INT32           gridX;                  ///< Grid dimension in the width direction for LTM
    INT32           gridY;                  ///< Grid dimension in the height direction for LTM
};

/// @brief Identifies the type of configuration to SWProc algorithm
enum SWProcAlgoType
{
    SWProcAlgoInvalid   = -1,           ///< Invalid type
    SWProcTypeGTM,                      ///< GTM Processing. Payload type: SWProcGTMParams
    SWProcTypeLTM,                      ///< LTM Processing. Payload type: SWProcLTMParams
    SWProcAlgoMax       = 0x7FFFFFFF    ///< Anchor to indicate the last item in the defines
};

/// @brief Represents one single SWProc inputs to the SWProc algorithm
struct SWProcAlgoProcessParams
{
    VOID*           pSWProcData;        ///< Pointer to the payload. See SWProcAlgoType for details.
    UINT32          sizeOfSWProcData;   ///< Size of payload
    SWProcAlgoType  type;               ///< Type of the payload
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SWAlgoProcess
///
/// @brief  SW Algo Processing function
///
/// @param  pProcessParams  Pointer to sw processing parameters
///
/// @return CamxResultSuccess if successful
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef CamxResult (*SWAlgoProcessType)(
    SWProcAlgoProcessParams* pProcessParams);

#ifdef __cplusplus
}
#endif // __cplusplus

CAMX_NAMESPACE_END

#endif // CAMXSWPROCESSALGO_H
