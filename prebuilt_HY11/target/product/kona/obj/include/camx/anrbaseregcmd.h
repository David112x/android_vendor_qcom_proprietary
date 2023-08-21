// NOWHINE ENTIRE FILE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  anrbaseregcmd.h
/// @brief IPEANRBaseRegCmd structures declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: do not include this file directly. Use anr10regcmd.h or anr20regcmd.h instead.

#ifndef ANRBASEREGCMD_H
#define ANRBASEREGCMD_H

#include "camxdefs.h"
// This file is common for titan 170 and 480, so cannot include titanX-ipe.h here.


CAMX_BEGIN_PACKED

struct IPEANRDCBlend1RegCmd
{
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND1_BYPASS   bypass; ///< DCBlend1 Bypass
} CAMX_PACKED;

struct IPEANRRNFRegCmd
{
    IPE_IPE_0_NPS_CLC_ANR_RNF_BYPASS                        bypass;             ///< Bypass register for the whole RNF block
    IPE_IPE_0_NPS_CLC_ANR_RNF_BYPASS_SUB                    subBypass;          ///< Inner bypass controls for logics inside RNF
    IPE_IPE_0_NPS_CLC_ANR_RNF_DEBUG                         debug;              ///< Debug register for RNF
    IPE_IPE_0_NPS_CLC_ANR_RNF_INDRANK_Y_CFG                 indRankConfigYCFG;  ///< indRankConfigYCFG
    IPE_IPE_0_NPS_CLC_ANR_RNF_INDRANK_Y_HV                  indRankYHV;         ///< indRankYHV
    IPE_IPE_0_NPS_CLC_ANR_RNF_INDRANK_Y_DG                  indRankYDG;         ///< indRankYDG
    IPE_IPE_0_NPS_CLC_ANR_RNF_INDRANK_C_CFG                 indRankCCFG;        ///< indRankCCFG
    IPE_IPE_0_NPS_CLC_ANR_RNF_INDRANK_C_HV                  indRankCHV;         ///< indRankCHV
    IPE_IPE_0_NPS_CLC_ANR_RNF_INDRANK_C_DG                  indRankCDG;         ///< indRankCDG
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_CFG_Y                    peakConfigY;        ///< peakConfigY
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_CFG_C                    peakConfigc;        ///< peakConfigc
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_IMGCONDS0_Y              peakImgCondS0Y;     ///< peakImgCondS0Y
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_IMGCONDS1_Y              peakImgCondS1Y;     ///< peakImgCondS1Y
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_IMGCONDS0_C              peakImgCondS0C;     ///< peakImgCondS0C
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_IMGCONDS1_C              peakImgCondS1C;     ///< peakImgCondS1C
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_DCCONDS0_Y               peakDCCondS0Y;      ///< peakDCCondS0Y
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_DCCONDS1_Y               peakDCCondS1Y;      ///< peakDCCondS1Y
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_DCCONDS0_C               peakDCCondS0C;      ///< peakDCCondS0C
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_DCCONDS1_C               peakDCCondS1C;      ///< peakDCCondS1C
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_LUMA_ISOTROPIC_THRS      peakLumaIsoThr;     ///< peakLumaIsoThr
    IPE_IPE_0_NPS_CLC_ANR_RNF_PEAK_CHROMA_ISOTROPIC_THRS    peakChromaIsoThr;   ///< peakChromaIsoThr
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_CFG_Y                     firConfigY;         ///< firConfigY
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_CFG_C                     firConfigC;         ///< firConfigC
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_DIRECTIONAL_THR_Y         firDirThrY;         ///< firDirThrY
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_DIRECTIONAL_THR_C         firDirThrC;         ///< firDirThrC
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_TRANS_Y_CNT_0             firTransY0;         ///< firTransY0
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_TRANS_Y_CNT_1             firTransY1;         ///< firTransY1
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_TRANS_Y_CNT_2             firTransY2;         ///< firTransY2
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_TRANS_C_CNT_0             firTransC0;         ///< firTransC0
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_TRANS_C_CNT_1             firTransC1;         ///< firTransC1
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_TRANS_C_CNT_2             firTransC2;         ///< firTransC2
    IPE_IPE_0_NPS_CLC_ANR_RNF_FIR_TRANS_C_CNT_3             firTransC3;         ///< firTransC3
} CAMX_PACKED;

struct IPEANRDCUSRegCmd
{
    IPE_IPE_0_NPS_CLC_ANR_DCUS_BYPASS   bypass; ///< DCUS Bypass
} CAMX_PACKED;

struct IPEANRCFilter2RegCmd
{
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_BYPASS              byPass;             ///< byPass
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_DEBUG               debug;              ///< debug
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_BYPASS_SUB          subBypass;          ///< subBypass
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_CHROMAFILTERING     chromaFiltering;    ///< chromaFiltering
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_CHROMAIND_THR9      chromaIndThr9;      ///< chromaIndThr9
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_CHROMAIND_THR11     chromaIndThr11;     ///< chromaIndThr11
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_GREYIND_THR9        greyIndThr9;        ///< greyIndThr9
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_GREYIND_THR11       greyIndThr11;       ///< greyIndThr11
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_CHROMA              chroma;             ///< chroma
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_MEDIAN_CNT1THR      medianThr;          ///< medianThr
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_MEDIAN_CNT1THRHV    medianThrHV;        ///< medianThrHV
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_FILTER2_MEDIAN_CNT1THRDG12  medianThrDG12;      ///< medianThrDG12
} CAMX_PACKED;

struct IPEANRDCBlend2RegCmd
{
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_BYPASS_LUMA      lumaBypass;     ///< Bypass & mode register for the Luma DCBlend2 block
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_BYPASS_CHROMA    chromaBypass;   ///< Bypass & mode register for the Chroma DCBlend2 block
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_Y_DEBUG          yDebug;         ///< debug mode for luma channel
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_C_DEBUG          cDebug;         ///< debug mode for chroma channel
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_LUMA_CFG         lumaConfig;     ///< Configuration register for the DCBlend2 block
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_CHROMA_CFG       chromaConfig;   ///< Configuration register for the DCBlend2 block
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_YBLEND_1X1       yBlend1X1;      ///< Luma Ydc blend factors for 1X1 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_YBLEND_3X3       yBlend3X3;      ///< Luma Ydc blend factors for 3X3 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_YBLEND_5X5       yBlend5X5;      ///< Luma Ydc blend factors for 5X5 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_YBLEND_7X7       yBlend7X7;      ///< Luma Ydc blend factors for 7X7 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_YBLEND_9X9       yBlend9X9;      ///< Luma Ydc blend factors for 9X9 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_YBLEND_1X1_ALT   yBlend1X1Alt;   ///< Luma Yalt blend factors for 1X1 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_YBLEND_3X3_ALT   yBlend3X3Alt;   ///< Luma Yalt blend factors for 3X3 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_YBLEND_5X5_ALT   yBlend5X5Alt;   ///< Luma Yalt blend factors for 5X5 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_YBLEND_7X7_ALT   yBlend7X7Alt;   ///< Luma Yalt blend factors for 7X7 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_YBLEND_9X9_ALT   yBlend9X9Alt;   ///< Luma Yalt blend factors for 9X9 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_CBLEND_1X1       cBlend1X1;      ///< Chroma blend factor for 1X1 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_CBLEND_3X3       cBlend3X3;      ///< Chroma blend factor for 3X3 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_CBLEND_5X5       cBlend5X5;      ///< Chroma blend factor for 5X5 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_CBLEND_7X7       cBlend7X7;      ///< Chroma blend factor for 7X7 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_CBLEND_9X9       cBlend9X9;      ///< Chroma blend factor for 9X9 dc pixels
    IPE_IPE_0_NPS_CLC_ANR_DCBLEND2_CBLEND_11X11     cblend11X11;    ///< Chroma blend factor for 11X11 dc pixels
} CAMX_PACKED;

struct IPEANRCYLPFPreLensGainRegCmd
{
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_BYPASS          bypass;     ///< Bypass register for the whole CYLPF block
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_BYPASS_SUB      subBypass;  ///< Bypass register for subblock
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_DEBUG           debug;      ///< Debug register for CYLPF
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFG             config;     ///< Configuration register for the entire CYLPF block
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_TCF_CFG         tcfConfig;  ///< TCF control register
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_TCF_RADIUS_THR  tcfRadius;  ///< TCF Radius related parameters register

    IPE_IPE_0_NPS_CLC_ANR_CYLPF_TCF_TRANSITION_THR_LOW_LUT_0    tcfLowThrLUT0;      ///< Transition low threshold function LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_TCF_TRANSITION_THR_LOW_LUT_1    tcfLowThrLUT1;      ///< Transition low threshold function LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_TCF_TRANSITION_THR_LOW_LUT_2    tcfLowThrLUT2;      ///< Transition low threshold function LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_TCF_TRANSITION_THR_HIGH_LUT_0   tcfHighThrLUT0;     ///< Transition high threshold function LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_TCF_TRANSITION_THR_HIGH_LUT_1   tcfHighThrLUT1;     ///< Transition high threshold function LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_TCF_TRANSITION_THR_HIGH_LUT_2   tcfHighThrLUT2;     ///< Transition high threshold function LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_TCF_TRANSITION_THR_LOW_LIMITS   tcfLowThrLimit;     ///< Transition threshold low function
                                                                                    ///< limits
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_TCF_TRANSITION_THR_HIGH_LIMITS  tcfHighThrLimit;    ///< Transition threshold high function
                                                                                    ///< limits
} CAMX_PACKED;

struct IPEANRCYLPFLensGainRegCmd
{
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_Y_0  lgYFilterLUTThrY0;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_Y_1  lgYFilterLUTThrY1;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_Y_2  lgYFilterLUTThrY2;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_Y_3  lgYFilterLUTThrY3;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_Y_4  lgYFilterLUTThrY4;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_Y_5  lgYFilterLUTThrY5;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_Y_6  lgYFilterLUTThrY6;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_Y_7  lgYFilterLUTThrY7;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_Y_8  lgYFilterLUTThrY8;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_C_0  lgYFilterLUTThrC0;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_C_1  lgYFilterLUTThrC1;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_C_2  lgYFilterLUTThrC2;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_C_3  lgYFilterLUTThrC3;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_C_4  lgYFilterLUTThrC4;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_C_5  lgYFilterLUTThrC5;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_C_6  lgYFilterLUTThrC6;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_C_7  lgYFilterLUTThrC7;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_YFILTER_LUT_THR_C_8  lgYFilterLUTThrC8;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_Y_0  lgCFilterLUTThrY0;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_Y_1  lgCFilterLUTThrY1;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_Y_2  lgCFilterLUTThrY2;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_Y_3  lgCFilterLUTThrY3;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_Y_4  lgCFilterLUTThrY4;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_Y_5  lgCFilterLUTThrY5;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_Y_6  lgCFilterLUTThrY6;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_Y_7  lgCFilterLUTThrY7;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_Y_8  lgCFilterLUTThrY8;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_C_0  lgCFilterLUTThrC0;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_C_1  lgCFilterLUTThrC1;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_C_2  lgCFilterLUTThrC2;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_C_3  lgCFilterLUTThrC3;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_C_4  lgCFilterLUTThrC4;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_C_5  lgCFilterLUTThrC5;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_C_6  lgCFilterLUTThrC6;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_C_7  lgCFilterLUTThrC7;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_CFILTER_LUT_THR_C_8  lgCFilterLUTThrC8;  ///< Lens gain threshold LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_LUT_BLEND_0          lgLUTBlend0;        ///< Lens gain blend factor LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_LUT_BLEND_1          lgLUTBlend1;        ///< Lens gain blend factor LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_LUT_BLEND_2          lgLUTBlend2;        ///< Lens gain blend factor LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_LUT_BLEND_3          lgLUTBlend3;        ///< Lens gain blend factor LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_LUT_BLEND_4          lgLUTBlend4;        ///< Lens gain blend factor LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_LUT_BLEND_5          lgLUTBlend5;        ///< Lens gain blend factor LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_LUT_BLEND_6          lgLUTBlend6;        ///< Lens gain blend factor LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_LUT_BLEND_7          lgLUTBlend7;        ///< Lens gain blend factor LUT
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_LG_LUT_BLEND_8          lgLUTBlend8;        ///< Lens gain blend factor LUT
} CAMX_PACKED;

struct IPEANRCYLPFPostLensGainRegCmd
{
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UVLIMIT             yFilterUVLmit;          ///< limit value for UV-Average input for
                                                                                    ///< Luma base threshold caluclation
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_CNRBASEBLENDFAR_Y   yFilterBaseBlendFarY;   ///< Blend factor for CNR gain level for
                                                                                    ///< the Far base Luma thresholds
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_CNRBASEBLENDFAR_C   yFilterBaseBlendFarC;   ///< Blend factor for CNR gain level for
                                                                                    ///< the Far base Chroma thresholds
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YYTB_0              yFilterYYTB0;           ///< yFilterYYTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YYTB_1              yFilterYYTB1;           ///< yFilterYYTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YYTB_2              yFilterYYTB2;           ///< yFilterYYTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YYTB_3              yFilterYYTB3;           ///< yFilterYYTB3
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YYTB_4              yFilterYYTB4;           ///< yFilterYYTB4
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YYTB_5              yFilterYYTB5;           ///< yFilterYYTB5
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YCTB_0              yFilterYCTB0;           ///< yFilterYCTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YCTB_1              yFilterYCTB1;           ///< yFilterYCTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YCTB_2              yFilterYCTB2;           ///< yFilterYCTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YCTB_3              yFilterYCTB3;           ///< yFilterYCTB3
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YTB_LIMITS          yFilterYTBLimit;        ///< Yfilter: Top limit and bottom limit of
                                                                                    ///< Y threshold calculation result
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UYTB_0              yFilterUYTB0;           ///< yFilterUYTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UYTB_1              yFilterUYTB1;           ///< yFilterUYTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UYTB_2              yFilterUYTB2;           ///< yFilterUYTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UYTB_3              yFilterUYTB3;           ///< yFilterUYTB3
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UYTB_4              yFilterUYTB4;           ///< yFilterUYTB4
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UCTB_0              yFilterUCTB0;           ///< yFilterUCTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UCTB_1              yFilterUCTB1;           ///< yFilterUCTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UCTB_2              yFilterUCTB2;           ///< yFilterUCTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UTB_LIMITS          yFilterUTBLimit;        ///< Yfilter: Top limit and bottom limit of
                                                                                    ///< U threshold calculation result
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VYTB_0              yFilterVYTB0;           ///< yFilterVYTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VYTB_1              yFilterVYTB1;           ///< yFilterVYTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VYTB_2              yFilterVYTB2;           ///< yFilterVYTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VYTB_3              yFilterVYTB3;           ///< yFilterVYTB3
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VYTB_4              yFilterVYTB4;           ///< yFilterVYTB4
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VCTB_0              yFilterVCTB0;           ///< yFilterVCTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VCTB_1              yFilterVCTB1;           ///< yFilterVCTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VCTB_2              yFilterVCTB2;           ///< yFilterVCTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VTB_LIMITS          yFilterVTBLimit;        ///< Yfilter: Top limit and bottom limit of
                                                                                    ///< V threshold calculation result

    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UVLIMIT             cFilterUVLimit;         ///< cFilterUVLimit
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_CNRBASEBLENDFAR_Y   cFilterBaseBlendFarY;   ///< Blend factor for CNR gain level for
                                                                                    ///< the Far base Luma thresholds
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_CNRBASEBLENDFAR_C   cFilterBaseBlendFarC;   ///< Blend factor for CNR gain level for
                                                                                    ///<  the Far base Chroma thresholds
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YYTB_0              cFilterYYTB0;           ///< cFilterYYTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YYTB_1              cFilterYYTB1;           ///< cFilterYYTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YYTB_2              cFilterYYTB2;           ///< cFilterYYTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YYTB_3              cFilterYYTB3;           ///< cFilterYYTB3
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YYTB_4              cFilterYYTB4;           ///< cFilterYYTB4
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YYTB_5              cFilterYYTB5;           ///< cFilterYYTB5
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YCTB_0              cFilterYCTB0;           ///< cFilterYCTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YCTB_1              cFilterYCTB1;           ///< cFilterYCTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YCTB_2              cFilterYCTB2;           ///< cFilterYCTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YCTB_3              cFilterYCTB3;           ///< cFilterYCTB3
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YTB_LIMITS          cFilterYTBLimit;        ///< Cfilter: Top limit and bottom limit of
                                                                                    ///< Y primary threshold calculation result
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UYTB_0              cFilterUYTB0;           ///< cFilterUYTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UYTB_1              cFilterUYTB1;           ///< cFilterUYTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UYTB_2              cFilterUYTB2;           ///< cFilterUYTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UYTB_3              cFilterUYTB3;           ///< cFilterUYTB3
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UYTB_4              cFilterUYTB4;           ///< cFilterUYTB4
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UCTB_0              cFilterUCTB0;           ///< cFilterUCTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UCTB_1              cFilterUCTB1;           ///< cFilterUCTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UCTB_2              cFilterUCTB2;           ///< cFilterUCTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UTB_LIMITS          cFilterUTBLimit;        ///< Cfilter: Top limit and bottom limit of
                                                                                    ///< Y primary threshold calculation result
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VYTB_0              cFilterVYTB0;           ///< cFilterVYTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VYTB_1              cFilterVYTB1;           ///< cFilterVYTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VYTB_2              cFilterVYTB2;           ///< cFilterVYTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VYTB_3              cFilterVYTB3;           ///< cFilterVYTB3
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VYTB_4              cFilterVYTB4;           ///< cFilterVYTB4
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VCTB_0              cFilterVCTB0;           ///< cFilterVCTB0
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VCTB_1              cFilterVCTB1;           ///< cFilterVCTB1
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VCTB_2              cFilterVCTB2;           ///< cFilterVCTB2
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VTB_LIMITS          cFilterVTBLimit;        ///< Cfilter: Top limit and bottom limit of
                                                                                    ///< V primary threshold calculation result

    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_CFG                 yFilterConfig;                  ///< yFilterConfig
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_KERNEL              yFilterKernel;                  ///< Filtering kernel of non-edge
                                                                                            ///< luma pixels
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_ALTKERNEL           yFilterAltKernel;               ///< Filtering kernel of edge luma
                                                                                            ///<  pixels
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_DCINDDERFLAGS       yFilterDCIndFlags;              ///< Condition flags to calculate
                                                                                            ///< the luma dc indication
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_FILTDERFLAGS        yFilterDerFlags;                ///< Condition flags to calculate
                                                                                            ///< the luma filtering flags
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_DER2FLAGS           yFilterDer2Flags;               ///< Der2 condition flags to
                                                                                            ///< calculate the luma filtering
                                                                                            ///< flags required for TCF
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_DIRECTIONALCFG      yFilterDirConfig;               ///< Specifies YLPF masks for
                                                                                            ///< directional filtering
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_DIRECTIONALCFGX     yFilterExternalDirConfig;       ///< Specifies YLPF masks for
                                                                                            ///< external directional filtering
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YTHRCLOSE3MOD       yFilterYThrehClose3Mod;         ///< 3x3 luma derivative
                                                                                            ///< scale/offset factors
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YTHRDER2CLOSE3MOD   yFilterYThreshDer2Close3Mod;    ///< 3x3 luma second order
                                                                                            ///< derivative scale/offset factors
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YTHRFARMOD          yFilterYThrFarMod;              ///< 7x7/9x9 luma derivative
                                                                                            ///< scale/offset factors
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UTHRFARMOD          yFilterUThrFarMod;              ///< 5x5 U derivative scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VTHRFARMOD          yFilterVThrFarMod;              ///< 5x5 V derivative scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YTHRCLOSEXMOD       yFilterYThrCloseMod;            ///< 5x5 external luma derivative
                                                                                            ///< scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_YTHRFARXMOD         yFilterYThrFarExMod;            ///< 9x9 external luma derivative
                                                                                            ///< scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_UTHRFARXMOD         yFilterUThrFarExMod;            ///< 5x5 external U derivative
                                                                                            ///< scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YFILTER_VTHRFARXMOD         yFilterVThrFarExMod;            ///< 5x5 external V derivative
                                                                                            ///< scale/offset

    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YDCINDY_GAIN_DER_3_0    yDCIndYGain30;      ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YDCINDY_GAIN_DER_5_0    yDCIndYGain50;      ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YDCINDY_GAIN_DER_7_0    yDCIndYGain70;      ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YDCINDY_GAIN_DER_9_0    yDCIndYGain90;      ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YDCINDY_GAIN_DER_5X_0   yDCIndYGain5X0;     ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YDCINDY_GAIN_DER_9X_0   yDCIndYGain9X0;     ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YDCINDUV_GAIN_DER_3_0   yDCIndUVGain30;     ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YDCINDUV_GAIN_DER_5_0   yDCIndUVGain50;     ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_YDCINDUV_GAIN_DER_5X_0  yDCIndUVGain5X0;    ///< Threshold modifiers depending on DC indications

    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_CFG                     cFilterConfig;              ///< cFilterConfig
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_DETAIL_COND             cFilterDetailCond;          ///< Contains chromaticity threshold
                                                                                            ///< and y derivative threshold used
                                                                                            ///< to identify details
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CTHRESHOLDS                     cThreshold;                 ///< Threshold for chroma detail
                                                                                            ///< detection
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_GREY_YDERFNC            cFilterGreyYDer;            ///< Describes the slope and offset
                                                                                            ///< of the grey factor luma
                                                                                            ///<     derivative function
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_GREY_CHFNC              cFilterGreyChromaticity;    ///< Describes the slope and offset
                                                                                            ///< of the grey factor
                                                                                            ///< chromaticity function
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_GREY_LIMITS             cFilterGreyLimits;          ///< Describes the top and bottom
                                                                                            ///< limits of the grey factor
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_GREY_BLENDSCALE         cFilterGreyBlendScale;      ///< cFilterGreyBlendScale
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_GREY_BLENDOFFSET        cFilterGreyBlendOffset;     ///< cFilterGreyBlendOffset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_GREYSIZETHR_3_5         cFilterGreySizeThr35;       ///< Thresholds used to calculate
                                                                                            ///< the grey dc size output
                                                                                            ///< indication
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_GREYSIZETHR_7_9         cFilterGreySizeThr79;       ///< Thresholds used to calculate
                                                                                            ///< the grey dc size output
                                                                                            ///< indication
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_GREY_THRMODVAL_CLOSE    cFilterGreyThrClose;        ///< Target thresholds for the 3x3
                                                                                            ///< and 5x5 thresholds;
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_GREY_THRMODVAL_FAR      cFilterGreyThrFar;          ///< Target thresholds for the 7x7
                                                                                            ///< and 9x9 thresholds;
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_DCINDDERFLAGS           cFilterDCIndFlags;          ///< Condition flags to calculate
                                                                                            ///< the chroma dc indication flags
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_FILTDERFLAGS            cFilterDerFlags;            ///< Condition flags to calculate
                                                                                            ///< the chroma filtering flags
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_DIRECTIONALCFG          cFilterDirConfig;           ///< Specifies CLPF masks for
                                                                                            ///< directional filtering.
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_DIRECTIONALCFGX         cFilterDirConfigEx;         ///< Specifies CLPF masks for
                                                                                            ///< external directional filtering
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YTHRFARMOD              cFilterYThrFarMod;          ///< 7x7/9x9 luma derivative
                                                                                            ///< scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UTHRFARMOD              cFilterUThrFarMod;          ///< 5x5 U derivative scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VTHRFARMOD              cFilterVThrFarMod;          ///< 5x5 V derivative scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UTHRDISTMOD             cFilterUThrDistMod;         ///< 7x7 U derivative scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VTHRDISTMOD             cFilterVThrDisMod;          ///< 7x7 V derivative scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YTHRCLOSEXMOD           cFilterYThrCloseMod;        ///< 5x5 external luma derivative
                                                                                            ///< scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_YTHRFARXMOD             cFilterYThrFarExMod;        ///< 9x9 external luma derivative
                                                                                            ///< scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_UTHRFARXMOD             cFilterUThrFarExMod;        ///< 5x5 external U derivative
                                                                                            ///< scale/offset
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CFILTER_VTHRFARXMOD             cFilterVThrFarExMod;        ///< 5x5 external V derivative
                                                                                            ///< scale/offset

    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CDCINDY_GAIN_DER_3_0    cDCIndYGain30;      ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CDCINDY_GAIN_DER_5_0    cDCIndYGain50;      ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CDCINDY_GAIN_DER_7_0    cDCIndYGain70;      ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CDCINDY_GAIN_DER_9_0    cDCIndYGain90;      ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CDCINDY_GAIN_DER_5X_0   cDCIndYGain5X0;     ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CDCINDY_GAIN_DER_9X_0   cDCIndYGain9X0;     ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CDCINDUV_GAIN_DER_3_0   cDCIndUVGain30;     ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CDCINDUV_GAIN_DER_5_0   cDCIndUVGain50;     ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CDCINDUV_GAIN_DER_7_0   cDCIndUVGain70;     ///< Threshold modifiers depending on DC indications
    IPE_IPE_0_NPS_CLC_ANR_CYLPF_CDCINDUV_GAIN_DER_5X_0  cDCIndUVGain5X0;    ///< Threshold modifiers depending on DC indications
} CAMX_PACKED;

struct IPEANRCNRRegCmd
{
    IPE_IPE_0_NPS_CLC_ANR_CNR_BYPASS                    bypass;                 ///< Bypass CNR
    IPE_IPE_0_NPS_CLC_ANR_CNR_CFG                       config;                 ///< CNR configuration register
    IPE_IPE_0_NPS_CLC_ANR_CNR_DEBUG                     debug;                  ///< CNR debug mode
    IPE_IPE_0_NPS_CLC_ANR_CNR_MODIFIERS_FILTER_MODE     filterMode;             ///< filterMode
    IPE_IPE_0_NPS_CLC_ANR_CNR_COLOR0_SPECIAL_PARAMS     specialParams;          ///< Color0 adiitional parameters
    IPE_IPE_0_NPS_CLC_ANR_CNR_C_MODE_0                  cMode;                  ///< cMode
    IPE_IPE_0_NPS_CLC_ANR_CNR_AMIN_0                    angleMin0;              ///< angleMin0
    IPE_IPE_0_NPS_CLC_ANR_CNR_AMIN_1                    angleMin1;              ///< angleMin1
    IPE_IPE_0_NPS_CLC_ANR_CNR_AMIN_2                    angleMin2;              ///< angleMin2
    IPE_IPE_0_NPS_CLC_ANR_CNR_AMAX_0                    angleMax0;              ///< angleMax0
    IPE_IPE_0_NPS_CLC_ANR_CNR_AMAX_1                    angleMax1;              ///< angleMax1
    IPE_IPE_0_NPS_CLC_ANR_CNR_AMAX_2                    angleMax2;              ///< angleMax2
    IPE_IPE_0_NPS_CLC_ANR_CNR_AINTERNAL_0               angleInternal;          ///< Angle internal/external to range treatment
    IPE_IPE_0_NPS_CLC_ANR_CNR_CSMIN_0                   colorSatMin0;           ///< Color saturation low limit
    IPE_IPE_0_NPS_CLC_ANR_CNR_CSMIN_1                   colorSatMin1;           ///< Color saturation low limit
    IPE_IPE_0_NPS_CLC_ANR_CNR_CSMIN_2                   colorSatMin2;           ///< Color saturation low limit
    IPE_IPE_0_NPS_CLC_ANR_CNR_CSMAX_0                   colorSatMax0;           ///< Color saturation high limit
    IPE_IPE_0_NPS_CLC_ANR_CNR_CSMAX_1                   colorSatMax1;           ///< Color saturation high limit
    IPE_IPE_0_NPS_CLC_ANR_CNR_CSMAX_2                   colorSatMax2;           ///< Color saturation high limit
    IPE_IPE_0_NPS_CLC_ANR_CNR_YMIN_0                    yCompMin0;              ///< Y component low limit
    IPE_IPE_0_NPS_CLC_ANR_CNR_YMIN_1                    yCompMin1;              ///< Y component low limit
    IPE_IPE_0_NPS_CLC_ANR_CNR_YMAX_0                    yCompMax0;              ///< Y component high limit
    IPE_IPE_0_NPS_CLC_ANR_CNR_YMAX_1                    yCompMax1;              ///< Y component high limit
    IPE_IPE_0_NPS_CLC_ANR_CNR_BOUNDARY_PROBABILITY_0    boundaryProbability;    ///< Weight value at Color Range Borders
    IPE_IPE_0_NPS_CLC_ANR_CNR_Q_0                       q0;                     ///< 1/slope of smooth range transition
                                                                                ///< function of each color region
    IPE_IPE_0_NPS_CLC_ANR_CNR_Q_1                       q1;                     ///< 1/slope of smooth range transition
                                                                                ///< function of each color region

    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_Y_Y_0      gainFactorYY0;      ///< Max/Min Gain factor YLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_Y_Y_1      gainFactorYY1;      ///< Max/Min Gain factor YLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_Y_Y_2      gainFactorYY2;      ///< Max/Min Gain factor YLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_UV_Y_0     gainFactorUVY0;     ///< Max/Min Gain factor for YLPF U/V-Base
                                                                        ///< Thresholds
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_UV_Y_1     gainFactorUVY1;     ///< Max/Min Gain factor for YLPF U/V-Base
                                                                        ///< Thresholds
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_UV_Y_2     gainFactorUVY2;     ///< Max/Min Gain factor for YLPF U/V-Bas
                                                                        ///< Thresholds
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_Y_Y_0          offsetYY0;          ///< Offset for YLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_Y_Y_1          offsetYY1;          ///< Offset for YLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_Y_Y_2          offsetYY2;          ///< Offset for YLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_U_Y_0          offsetUY0;          ///< Offset for YLPF U-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_U_Y_1          offsetUY1;          ///< Offset for YLPF U-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_V_Y_0          offsetVY0;          ///< Offset for YLPF V-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_V_Y_1          offsetVY1;          ///< Offset for YLPF V-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_Y_C_0      gainFactorYC0;      ///< Max/Min Gain factor for CLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_Y_C_1      gainFactorYC1;      ///< Max/Min Gain factor for CLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_Y_C_2      gainFactorYC2;      ///< Max/Min Gain factor for CLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_UV_C_0     gainFactorUVC0;     ///< Max/Min Gain factor for CLPF U/V- Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_UV_C_1     gainFactorUVC1;     ///< Max/Min Gain factor for CLPF U/V- Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_GAINFACTOR_UV_C_2     gainFactorUVC2;     ///< Max/Min Gain factor for CLPF U/V- Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_Y_C_0          offsetYC0;          ///< Offset for CLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_Y_C_1          offsetYC1;          ///< Offset for CLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_Y_C_2          offsetYC2;          ///< Offset for CLPF Y-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_U_C_0          offsetUC0;          ///< Offset for CLPF U-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_U_C_1          offsetUC1;          ///< Offset for CLPF U-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_V_C_0          offsetVC0;          ///< Offset for CLPF V-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_OFFSET_V_C_1          offsetVC1;          ///< Offset for CLPF V-Base Threshold
    IPE_IPE_0_NPS_CLC_ANR_CNR_BLENDFACTOR_Y_DC_0    blendFactorYDC0;    ///< Factor for DCBlend2 Y_Dc blend factor modification
    IPE_IPE_0_NPS_CLC_ANR_CNR_BLENDFACTOR_Y_DC_1    blendFactorYDC1;    ///< Factor for DCBlend2 Y_Dc blend factor modification
    IPE_IPE_0_NPS_CLC_ANR_CNR_BLENDFACTOR_Y_ALT_0   blendFactorYALT0;   ///< Factor for DCBlend2 Y_Alt blend factor modification
    IPE_IPE_0_NPS_CLC_ANR_CNR_BLENDFACTOR_Y_ALT_1   blendFactorYALT1;   ///< Factor for DCBlend2 Y_Alt blend factor modification
    IPE_IPE_0_NPS_CLC_ANR_CNR_BLENDFACTOR_C_0       blendFactorC0;      ///< Factor for DCBlend2 Chroma blend factor
                                                                        ///< modification
    IPE_IPE_0_NPS_CLC_ANR_CNR_BLENDFACTOR_C_1       blendFactorC1;      ///< Factor for DCBlend2 Chroma blend factor
                                                                        ///< modification

    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_OFFSET                     fdOffset;           ///< FD Offset
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_XCENTER_0          xCenter0;           ///< Horizontal center of face cyclic region
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_XCENTER_1          xCenter1;           ///< Horizontal center of face cyclic region
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_XCENTER_2          xCenter2;           ///< Horizontal center of face cyclic region
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_YCENTER_0          yCenter0;           ///< Vertical center of face cyclic region
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_YCENTER_1          yCenter1;           ///< Vertical center of face cyclic region
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_YCENTER_2          yCenter2;           ///< Vertical center of face cyclic region
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_RBOUNDARY_0        rBoundary0;         ///< Represents Radius Boundary
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_RBOUNDARY_1        rBoundary1;         ///< Represents Radius Boundary
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_RBOUNDARY_2        rBoundary2;         ///< Represents Radius Boundary
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_RBOUNDARY_SHIFT_0  rBoundaryShift0;    ///< RBoubndary shift size
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_RSLOPE_0           rSlope0;            ///< The transition area slope
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_RSLOPE_1           rSlope1;            ///< The transition area slope
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_RSLOPE_2           rSlope2;            ///< The transition area slope
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_REGIONS_RSLOPE_SHIFT_0     rSlopeShift0;       ///< Slope shift size
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_COMBINE_0                  combine0;           ///< Defines for each color whether it's
                                                                                ///< Color_W should be combined with FD_W
    IPE_IPE_0_NPS_CLC_ANR_CNR_FD_CFG                        fdConfig;           ///< FD config
} CAMX_PACKED;


struct IPEANRBaseRegCmd
{
    IPEANRDCBlend1RegCmd            dcBlend1Cmd;            ///< ANR DCBlend1 reg cmds
    IPEANRRNFRegCmd                 rnfCmd;                 ///< ANR RNF reg cmds
    IPEANRDCUSRegCmd                dcusCmd;                ///< ANR DCUS reg cmds
    IPEANRCFilter2RegCmd            filter2Cmd;             ///< ANR Filter2 cmds
    IPEANRDCBlend2RegCmd            dcBlend2Cmd;            ///< ANR DCBlend2 reg cmds
    IPEANRCYLPFPreLensGainRegCmd    cylpfPreLensGainCmd;    ///< ANR CYLPF pre Lens Gain reg cmds
    IPEANRCYLPFLensGainRegCmd       cylpfLensGainCmd;       ///< ANR CYLPF Lens Gain reg cmds
    IPEANRCYLPFPostLensGainRegCmd   cylpfPostLensGainCmd;   ///< ANR CYLPF post Lens Gain cmds
    IPEANRCNRRegCmd                 cnrCmd;                 ///< ANR CNR reg cmds
} CAMX_PACKED;

CAMX_END_PACKED

#endif // ANRBASEREGCMD_H
