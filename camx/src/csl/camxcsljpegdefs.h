////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcsljpegdefs.h
/// @brief JPEG Hardware Interface Definiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCSLJPEGDEFS_H
#define CAMXCSLJPEGDEFS_H

#include "camxcsl.h"
#include "camxcslresourcedefs.h"
#include "camxdefs.h"

CAMX_BEGIN_PACKED

/// @brief JPEG hardware version information
struct CSLJPEGDeviceVersion
{
    UINT32      size;       ///< Reserved field to force size as a multiple of 64bits
    CSLVersion  version;    ///< Device version
    UINT32      deviceType; ///< Device type for the cap info(CSLJPEGDeviceType)
} CAMX_PACKED;

/// @brief JPEG device capability information
///         Contains information about firmware and its API version along with number of Encoder and DMA resources available
struct CSLJPEGQueryCapabilityCmd
{
    CSLVersion           APIVersion;                                         ///< API version info
    UINT32               numENC;                                             ///< Number of encoders
    UINT32               numDMA;                                             ///< Number of DMA
    CSLJPEGDeviceVersion deviceVersion[CSLJPEGResourceIDMaxNumResources];    ///< Returned device capability array

} CAMX_PACKED;

/// @brief JPEG resource characteristics (dimension and format) information
struct CSLJPEGResourceInfo
{
    UINT32      format;   ///< Image format
    UINT32      width;    ///< Image width
    UINT32      height;   ///< Image height
    UINT32      FPS;      ///< Stream frame per second
} CAMX_PACKED;

/// @brief JPEG acquire device information
struct CSLJPEGAcquireDeviceInfo
{
    UINT32                resourceType;     ///< Resource type (Encoder decoder dma)
    CSLJPEGResourceInfo   inputResource;    ///< Input resource info struct
    CSLJPEGResourceInfo   outputResource;   ///< Input resource info struct
} CAMX_PACKED;

/// @brief JPEG Config Input Params
struct CSLJPEGConfigInputInfo
{
    INT32   clk_index;     ///< jpeg hw clock index to be selected (-1 default)
    INT32   output_size;   ///< jpeg encode/dma output size.
} CAMX_PACKED_ALIGN_N(8);

CAMX_END_PACKED

#endif // CAMXCSLJPEGDEFS_H
