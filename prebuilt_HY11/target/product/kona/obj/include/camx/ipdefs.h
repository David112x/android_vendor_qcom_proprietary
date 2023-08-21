//-------------------------------------------------------------------------
// Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// @file  ipdefs.h
// @brief Common IP data type definitions
//------------------------------------------------------------------------

//------------------------------------------------------------------------
// @Edit History
// when              who           what,where,why
// -----             ---           -------------
// Sep/21/2015       shirb        Initial version
//------------------------------------------------------------------------

#ifndef _IPDEFS_H_
#define _IPDEFS_H_

#include "icpcomdef.h"

//------------------------------------------------------------------------
// Constant Definitions
//------------------------------------------------------------------------

// For YUV 420, there are two plains: one for Y and one for UV
#define MAX_NUM_OF_IMAGE_PLANES         2

//------------------------------------------------------------------------
// Type Declarations
//------------------------------------------------------------------------
typedef enum _ImageFormat
{
    IMAGE_FORMAT_INVALID = 0,   // Default is INVALID.
    IMAGE_FORMAT_MIPI_8,
    IMAGE_FORMAT_MIPI_10,
    IMAGE_FORMAT_MIPI_12,
    IMAGE_FORMAT_MIPI_14,
    IMAGE_FORMAT_BAYER_8,
    IMAGE_FORMAT_BAYER_10,
    IMAGE_FORMAT_BAYER_12,
    IMAGE_FORMAT_BAYER_14,
    IMAGE_FORMAT_PDI_10,
    IMAGE_FORMAT_PD_10,
    IMAGE_FORMAT_LINEAR_PLAIN_8, // was IMAGE_FORMAT_RESERVED_1, for PD_8 format, but re-purposed for PLAIN_8
    IMAGE_FORMAT_INDICATIONS,
    IMAGE_FORMAT_REFINEMENT,
    IMAGE_FORMAT_UBWC_TP_10,
    IMAGE_FORMAT_UBWC_NV_12,
    IMAGE_FORMAT_UBWC_NV12_4R,
    IMAGE_FORMAT_UBWC_P010,
    IMAGE_FORMAT_LINEAR_TP_10,
    IMAGE_FORMAT_LINEAR_P010,
    IMAGE_FORMAT_LINEAR_NV12,
    IMAGE_FORMAT_LINEAR_PLAIN_16,
    IMAGE_FORMAT_YUV422_8,
    IMAGE_FORMAT_YUV422_10,
    IMAGE_FORMAT_STATISTICS_BAYER_GRID,
    IMAGE_FORMAT_STATISTICS_BAYER_HISTOGRAM,
    IMAGE_FORMAT_ARGB_8_8_8_8,
    IMAGE_FORMAT_ARGB_2_10_10_10,
    IMAGE_FORMAT_ARGB_16_16_16_16,
    IMAGE_FORMAT_LMC,
    IMAGE_FORMAT_BAYER_16,
    IMAGE_FORMAT_BAYER_20,
    IMAGE_FORMAT_MIPI_20,
    IMAGE_FORMAT_STATISTICS_HDR10PLUS,
    IMAGE_FORMAT_MAX    // This must be the last member
} ImageFormat;

typedef enum _PlaneFormat
{
    PLANE_FORMAT_INVALID = 0,    // Default is INVALID.

    PLANE_FORMAT_MIPI_8,
    PLANE_FORMAT_MIPI_10,
    PLANE_FORMAT_MIPI_12,
    PLANE_FORMAT_MIPI_14,
    PLANE_FORMAT_BAYER_8,
    PLANE_FORMAT_BAYER_10,
    PLANE_FORMAT_BAYER_12,
    PLANE_FORMAT_BAYER_14,
    PLANE_FORMAT_PDI_10,
    PLANE_FORMAT_PD_10,    // 2x2 block: 4 luma samples, followed by Cb and Cr (10 bps).
    PLANE_FORMAT_RESERVED_1,    // PLANE_FORMAT_PD_8, 2x2 block: 4 luma samples, followed by Cb and Cr (8 bps).
    PLANE_FORMAT_INDICATIONS,
    PLANE_FORMAT_REFINEMENT,
    PLANE_FORMAT_UBWC_TP_10_Y,
    PLANE_FORMAT_UBWC_TP_10_C,
    PLANE_FORMAT_UBWC_NV_12_Y,
    PLANE_FORMAT_UBWC_NV_12_C,
    PLANE_FORMAT_UBWC_NV_12_4R_Y,
    PLANE_FORMAT_UBWC_NV_12_4R_C,
    PLANE_FORMAT_UBWC_P010_Y,
    PLANE_FORMAT_UBWC_P010_C,
    PLANE_FORMAT_LINEAR_TP_10_Y,
    PLANE_FORMAT_LINEAR_TP_10_C,
    PLANE_FORMAT_LINEAR_P010_Y,
    PLANE_FORMAT_LINEAR_P010_C,
    PLANE_FORMAT_LINEAR_NV12_Y,
    PLANE_FORMAT_LINEAR_NV12_C,
    PLANE_FORMAT_LINEAR_PLAIN_16_Y,
    PLANE_FORMAT_LINEAR_PLAIN_16_C,
    PLANE_FORMAT_YUV422_8,
    PLANE_FORMAT_YUV422_10,
    PLANE_FORMAT_STATISTICS_BAYER_GRID,
    PLANE_FORMAT_STATISTICS_BAYER_HISTOGRAM,
    PLANE_FORMAT_ARGB_8_8_8_8,
    PLANE_FORMAT_ARGB_2_10_10_10,
    PLANE_FORMAT_ARGB_16_16_16_16,
    PLANE_FORMAT_LMC,
    PLANE_FORMAT_BAYER_16,
    PLANE_FORMAT_BAYER_20,
    PLANE_FORMAT_MIPI_20,
    PLANE_FORAMT_STATISTICS_HDR10PLUS,
    PLANE_FORMAT_MAX    // This must be the last member
} PlaneFormat;

typedef enum _BayerPixelOrder
{
    FIRST_PIXEL_R,
    FIRST_PIXEL_GR,
    FIRST_PIXEL_B,
    FIRST_PIXEL_GB,
    FIRST_PIXEL_MAX
} BayerPixelOrder;

typedef enum _PixelPackingAlignment
{
    PIXEL_LSB_ALIGNED,
    PIXEL_MSB_ALIGNED
} PixelPackingAlignment;

typedef enum _Yuv422PixelOrder
{
    PIXEL_ORDER_Y_U_Y_V,
    PIXEL_ORDER_Y_V_Y_U,
    PIXEL_ORDER_U_Y_V_Y,
    PIXEL_ORDER_V_Y_U_Y,
    PIXEL_ORDER_YUV422_MAX
} Yuv422PixelOrder;

typedef enum _UbwcWriteClient
{
    IPE_UBWC_STATS_VIDEO_Y = 0,
    IPE_UBWC_STATS_VIDEO_C,
    IPE_UBWC_STATS_DISPLAY_Y,
    IPE_UBWC_STATS_DISPLAY_C,
    IPE_UBWC_STATS_FULL_REF_Y,
    IPE_UBWC_STATS_FULL_REF_C,
    IPE_UBWC_STATS_MAX    // This must be the last member
} UbwcWriteClient;

typedef enum _BPSUbwcWriteClient
{
    BPS_UBWC_STATS_FULL_Y = 0,
    BPS_UBWC_STATS_FULL_C,
    BPS_UBWC_STATS_MAX    // This must be the last member
} BPSUbwcWriteClient;

#pragma pack(push)
#pragma pack(1)

typedef struct _UbwcStats
{
    uint32_t UBWC_COMPRESSED_32B;
    uint32_t UBWC_COMPRESSED_64B;
    uint32_t UBWC_COMPRESSED_96B;
    uint32_t UBWC_COMPRESSED_128B;
    uint32_t UBWC_COMPRESSED_160B;
    uint32_t UBWC_COMPRESSED_192B;
    uint32_t UBWC_COMPRESSED_256B;
    uint32_t totalTiles;
} UbwcStats;

typedef struct _IPEUBWCStatsBuffer
{
    uint32_t padding0[16];
    UbwcStats UbwcStatsBuffer[IPE_UBWC_STATS_MAX];
    uint32_t padding1[16];
} IPEUbwcStatsBuffer;

typedef IPEUbwcStatsBuffer UbwcStatsBuffer;

typedef struct _BPSUBWCStatsBuffer
{
    uint32_t padding0[16];
    UbwcStats UbwcStatsBuffer[BPS_UBWC_STATS_MAX];
    uint32_t padding1[16];
} BPSUbwcStatsBuffer;

typedef struct _ImageDimensions
{
    uint32_t    widthPixels;
    uint32_t    heightLines;
} ImageDimensions;

typedef struct _WindowRegion
{
    uint32_t    fullWidth;
    uint32_t    fullHeight;
    int32_t     windowLeft;
    int32_t     windowTop;
    uint32_t    windowWidth;
    uint32_t    windowHeight;
} WindowRegion;

typedef enum _UbwcVersion
{
    UBWC_VERSION_2 = 0,
    UBWC_VERSION_3 = 1,
    UBWC_VERSION_4 = 2,
} UbwcVersion;

typedef enum _UbwcLossyMode
{
    UBWC_LOSSLESS = 0,
    UBWC_LOSSY = 1
} UbwcLossyMode;

typedef struct _ImageInfo
{
    ImageFormat             format;
    ImageDimensions         dimensions;
    BayerPixelOrder         bayerOrder;
    PixelPackingAlignment   pixelPackingAlignment;
    Yuv422PixelOrder        yuv422Order;
    uint32_t                enableByteSwap;      // an indication whether the Bytes should be swapped
    UbwcVersion             ubwcVersion;         // Relevant for T175 and beyond
                                                 // 0 - UBWC 2.0
                                                 // 1 - UBWC 3.0
                                                 // 2 - UBWC 4.0
                                                 // For Kona and beyond, it is a static control shared by
                                                 // all bus write clients; provide 1 value for all clients
    UbwcLossyMode           ubwcLossyMode;       // Relevant for T175 and beyond
    uint32_t                ubwcBwLimit;         // Relevant for T175 and beyond
    uint32_t                ubwcThreshLossy0;    // Relevant for T175 and beyond
                                                 // Used for programming CLIENT_X_UBWC_THRESHOLD_LOSSY_0
    uint32_t                ubwcThreshLossy1;    // Relevant for T175 and beyond
                                                 // Used for programming CLIENT_X_UBWC_THRESHOLD_LOSSY_0
    uint32_t                argbAlpha;           // Relevent for T175 and beyond
    uint32_t                ubwcOffsetsVarLossy; // Relevant for T175 and beyond
                                                 // Used for programming CLIENT_X_UBWC_OFFSETS_VARIANCE_LOSSY
    uint32_t                fromDownscale;       // an indication whether the image is from firmware downscaling
} ImageInfo;

typedef struct _BufferLayout
{
    uint32_t bufferStride;    // bytes
    uint32_t bufferHeight;    // lines
} BufferLayout;

typedef struct _ImageDescriptor
{
    ImageInfo       info;
    BufferLayout    bufferLayout[MAX_NUM_OF_IMAGE_PLANES];
    BufferLayout    metadataBufferLayout[MAX_NUM_OF_IMAGE_PLANES];
} ImageDescriptor;

typedef UINTPTR32 FrameBufferPtr;

typedef struct _FrameBuffers
{
    // Raw pointers to allocated buffers.
    FrameBufferPtr  bufferPtr[MAX_NUM_OF_IMAGE_PLANES];
    FrameBufferPtr  metadataBufferPtr[MAX_NUM_OF_IMAGE_PLANES];
} FrameBuffers;

typedef struct _HNR_MODULE_CFG
{
    uint32_t  EN;           /* 0:0 */
    uint32_t  BLEND_SNR_EN; /* 8:8 */
    uint32_t  BLEND_CNR_EN; /* 9:9 */
    uint32_t  BLEND_ENABLE; /* 10:10 */
    uint32_t  LPF3_EN;      /* 11:11 */
    uint32_t  FNR_EN;       /* 12:12 */
    uint32_t  FD_SNR_EN;    /* 13:13 */
    uint32_t  SNR_EN;       /* 14:14 */
    uint32_t  CNR_EN;       /* 15:15 */
    uint32_t  RNR_EN;       /* 16:16 */
    uint32_t  LNR_EN;       /* 17:17 */
} HNR_MODULE_CFG;

typedef struct _HnrParameters
{
    HNR_MODULE_CFG moduleCfg;
    int32_t snrSkinSmoothingStr;
} HnrParameters;

typedef struct _GTM_MODULE_CFG
{
    uint32_t  EN;    /* 0:0 */
} GTM_MODULE_CFG;

typedef struct _GtmParameters
{
    GTM_MODULE_CFG moduleCfg;
} GtmParameters;

typedef struct _UVG_MODULE_CFG
{
    uint32_t  EN; /* 0:0 */
} UVG_MODULE_CFG;
typedef struct _UVGammaParameters
{
    UVG_MODULE_CFG moduleCfg;
} UVGammaParameters;


#pragma pack(pop)

#endif // _IPDEFS_H_
