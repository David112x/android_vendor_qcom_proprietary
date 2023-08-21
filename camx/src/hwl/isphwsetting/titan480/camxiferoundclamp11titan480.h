////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxiferoundclamp11titan480.h
/// @brief IFE ROUNDCLAMP11 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXIFEROUNDCLAMP11TITAN480_H
#define CAMXIFEROUNDCLAMP11TITAN480_H

#include "titan480_ife.h"
#include "camxdefs.h"
#include "camxisphwsetting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

struct IFECRC11FullLumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch2 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11FullChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11FDLumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11FDChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11DSXLumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11DSXChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11DS16LumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11DS16ChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11DisplayLumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11DisplayChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11DisplayDS4LumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11DisplayDS4ChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11displayDS16LumaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11displayDS16ChromaReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

struct IFECRC11PixelRawReg
{
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_MODULE_CFG       moduleConfig;    ///< Module Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_CH0_CLAMP_CFG    ch0ClampConfig;  ///< Ch0 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_CH0_ROUNDING_CFG ch0RoundConfig;  ///< Ch0 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_CH1_CLAMP_CFG    ch1ClampConfig;  ///< Ch1 Clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_CH1_ROUNDING_CFG ch1RoundConfig;  ///< Ch1 Round Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_CH2_CLAMP_CFG    ch2ClampConfig;  ///< Ch2 clamp Config
    IFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_CH2_ROUNDING_CFG ch2RoundConfig;  ///< Ch2 Round Config
} CAMX_PACKED;

CAMX_END_PACKED

struct IFECRC11RegCmd
{
    IFECRC11FullLumaReg          fullLuma;          ///< FD path Luma config
    IFECRC11FullChromaReg        fullChroma;        ///< FD path Chroma config
    IFECRC11FDLumaReg            FDLuma;            ///< Full path Luma config
    IFECRC11FDChromaReg          FDChroma;          ///< Full path Luma config
    IFECRC11DSXLumaReg           DSXLuma;           ///< DSX path Luma config
    IFECRC11DSXChromaReg         DSXChroma;         ///< DSX path Luma config
    IFECRC11DS16LumaReg          DS16Luma;          ///< DS16 path Luma config
    IFECRC11DS16ChromaReg        DS16Chroma;        ///< DS16 path Luma config
    IFECRC11DisplayLumaReg       displayLuma;       ///< Disp path Luma config
    IFECRC11DisplayChromaReg     displayChroma;     ///< Disp path Chroma config
    IFECRC11DisplayDS4LumaReg    displayDS4Luma;    ///< DS4 Disp path Luma config
    IFECRC11DisplayDS4ChromaReg  displayDS4Chroma;  ///< DS4 Disp path Luma config
    IFECRC11displayDS16LumaReg   displayDS16Luma;   ///< DS16 Disp path Luma config
    IFECRC11displayDS16ChromaReg displayDS16Chroma; ///< DS16 Disp path Luma config
    IFECRC11PixelRawReg          pixelRaw;          ///< Pixel Raw Config
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief IFE ROUNDCLAMP11 register setting for Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IFERoundClamp11Titan480 final : public ISPHWSetting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCmdList
    ///
    /// @brief  Generate the Command List
    ///
    /// @param  pInputData       Pointer to the Inputdata
    /// @param  pDMIBufferOffset Pointer for DMI Buffer
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult CreateCmdList(
        VOID*   pInputData,
        UINT32* pDMIBufferOffset);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PackIQRegisterSetting
    ///
    /// @brief  Calculate register settings based on common library result
    ///
    /// @param  pInput  Pointer to the Input data to the module for calculation
    /// @param  pOutput Pointer to the Output data to the module for DMI buffer
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PackIQRegisterSetting(
        VOID* pInput,
        VOID* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupRegisterSetting
    ///
    /// @brief  Setup register value based on CamX Input
    ///
    /// @param  pInput Pointer to the Input data to the module for calculation
    ///
    /// @return Return CamxResult.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SetupRegisterSetting(
        VOID*  pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IFERoundClamp11Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IFERoundClamp11Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IFERoundClamp11Titan480
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    IFERoundClamp11Titan480();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpRegConfig
    ///
    /// @brief  Print register configuration of Crop module for debug
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpRegConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CopyRegCmd
    ///
    /// @brief  Copy register settings to the input buffer
    ///
    /// @param  pData  Pointer to the Input data buffer
    ///
    /// @return Number of bytes copied
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 CopyRegCmd(
        VOID* pData);

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFDPathRegisters
    ///
    /// @brief  Configure RoundClamp11 FD output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFDPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFullPathRegisters
    ///
    /// @brief  Configure RoundClamp11 Full output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureFullPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDSXPathRegisters
    ///
    /// @brief  Configure RoundClamp11 DSX output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDSXPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS16PathRegisters
    ///
    /// @brief  Configure RoundClamp11 DS16 output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS16PathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDisplayPathRegisters
    ///
    /// @brief  Configure RoundClamp11 Display output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDisplayPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS4DisplayPathRegisters
    ///
    /// @brief  Configure RoundClamp11 DS4 Display output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS4DisplayPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureDS16DisplayPathRegisters
    ///
    /// @brief  Configure RoundClamp11 DS16 Display output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigureDS16DisplayPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigurePixelRawPathRegisters
    ///
    /// @brief  Configure RoundClamp11 Pixel raw output path registers
    ///
    /// @param  minValue   Minimum threshold to clamp
    /// @param  maxValue   Maximum threshold to clamp
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ConfigurePixelRawPathRegisters(
        UINT32 minValue,
        UINT32 maxValue);

    IFECRC11RegCmd            m_regCmd;       ///< Register List of this Module
    IFEPipelinePath           m_modulePath;   ///< IFE pipeline path for module
    UINT32                    m_bitWidth;     ///< ISP output bit width based on ISP output format

    IFERoundClamp11Titan480(const IFERoundClamp11Titan480&)            = delete; ///< Disallow the copy constructor
    IFERoundClamp11Titan480& operator=(const IFERoundClamp11Titan480&) = delete; ///< Disallow assignment operator
};

CAMX_NAMESPACE_END

#endif // CAMXIFEROUNDCLAMP11TITAN480_H
