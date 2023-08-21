// NOWHINE ENTIRE FILE
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  tfbaseregcmd.h
/// @brief IPETFRegCmd structure definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOTE: do not include this file directly. Use tf10regcmd17x.h or tf20regcmd480.h instead.

#ifndef TFBASEREGCMD_H
#define TFBASEREGCMD_H

#include "camxdefs.h"
// This file is common for titan 170 and 480, so cannot include titanX-ipe.h here.

CAMX_BEGIN_PACKED

struct IPETFRegCmd
{
    IPE_IPE_0_NPS_CLC_TF_TF_CONFIG_0                      config0;                ///< operation modes of TF
    IPE_IPE_0_NPS_CLC_TF_TF_CONFIG_1                      config1;                ///< operation modes of TF
    IPE_IPE_0_NPS_CLC_TF_TF_IM_ERODE_CFG                  erodeConfig;            ///< operation modes of TF_IM (erosion)
    IPE_IPE_0_NPS_CLC_TF_TF_IM_DILATE_CFG                 dilateConfig;           ///< operation modes of TF_IM (dilation)
    IPE_IPE_0_NPS_CLC_TF_TF_DATA_CROPINHORIZSTART         cropInHorizStart;       ///< horizontal cropping start location
    IPE_IPE_0_NPS_CLC_TF_TF_DATA_CROPINHORIZEND           cropInHorizEnd;         ///< horizontal cropping end location
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNRSTARTIDXH            lnrStartIDXH;           ///< initial value of the horizontal part
                                                                                  ///< of the distance to the optical center
    IPE_IPE_0_NPS_CLC_TF_TF_US_CROP                       usCrop;                 ///< TF_US parameters
    IPE_IPE_0_NPS_CLC_TF_TF_IND_CROPOUT_CFG               indCropConfig;          ///< configure cropping output IND11
    IPE_IPE_0_NPS_CLC_TF_TF_PRNGSEED                      prngSeed;               ///< seed for TF_PRNG
    IPE_IPE_0_NPS_CLC_TF_TF_REF_CFG_0                     refCfg0;                ///< configure refinement
    IPE_IPE_0_NPS_CLC_TF_TF_REF_CFG_1                     refCfg1;                ///< configure refinement
    IPE_IPE_0_NPS_CLC_TF_TF_REF_CFG_2                     refCfg2;                ///< configure refinement
    IPE_IPE_0_NPS_CLC_TF_TF_DATA_CROPINVERTSTART          cropInvertStart;        ///< vertical cropping start location
    IPE_IPE_0_NPS_CLC_TF_TF_DATA_CROPINVERTEND            cropInvertEnd;          ///< vertical cropping end location
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNRSTARTIDXV            lnrStartIDXV;           ///< initial value of the vertical part of
                                                                                  ///< the distance to the optical center
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNRSCALE                lnrScale;               ///< horizontal and vertical steps between
                                                                                  ///< neighbor pixels
    IPE_IPE_0_NPS_CLC_TF_TF_IND_CROPOUTVERT               cropOutVert;            ///< vertical crop out parameters
    IPE_IPE_0_NPS_CLC_TF_TF_REF_Y_CFG                     refYCfg;                ///< Y refinement params
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_0          tdNtNpYContrib0;        ///< noiseParams{Y,Cb,Cr}_YTb(0)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_1          tdNtNpYContrib1;        ///< noiseParams{Y,Cb,Cr}_YTb(1)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_2          tdNtNpYContrib2;        ///< noiseParams{Y,Cb,Cr}_YTb(2)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_3          tdNtNpYContrib3;        ///< noiseParams{Y,Cb,Cr}_YTb(3)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_4          tdNtNpYContrib4;        ///< noiseParams{Y,Cb,Cr}_YTb(4)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_5          tdNtNpYContrib5;        ///< noiseParams{Y,Cb,Cr}_YTb(5)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_6          tdNtNpYContrib6;        ///< noiseParams{Y,Cb,Cr}_YTb(6)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_7          tdNtNpYContrib7;        ///< noiseParams{Y,Cb,Cr}_YTb(7)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_8          tdNtNpYContrib8;        ///< noiseParams{Y,Cb,Cr}_YTb(8)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_9          tdNtNpYContrib9;        ///< noiseParams{Y,Cb,Cr}_YTb(9)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_10         tdNtNpYContrib10;       ///< noiseParams{Y,Cb,Cr}_YTb(10)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_11         tdNtNpYContrib11;       ///< noiseParams{Y,Cb,Cr}_YTb(11)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_12         tdNtNpYContrib12;       ///< noiseParams{Y,Cb,Cr}_YTb(12)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_13         tdNtNpYContrib13;       ///< noiseParams{Y,Cb,Cr}_YTb(13)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_14         tdNtNpYContrib14;       ///< noiseParams{Y,Cb,Cr}_YTb(14)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_15         tdNtNpYContrib15;       ///< noiseParams{Y,Cb,Cr}_YTb(15)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_Y_CONTRIB_16         tdNtNpYContrib16;       ///< noiseParams{Y,Cb,Cr}_YTb(16)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_Y_0        tdNtNpCContribY0;       ///< noiseParamsY_CTb(0)noiseParamsY_CTb(1)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_Y_1        tdNtNpCContribY1;       ///< noiseParamsY_CTb(2)noiseParamsY_CTb(3)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_Y_2        tdNtNpCContribY2;       ///< noiseParamsY_CTb(4)noiseParamsY_CTb(5)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_Y_3        tdNtNpCContribY3;       ///< noiseParamsY_CTb(6)noiseParamsY_CTb(7)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_CB_0       tdNtNpCContribCB0;      ///< noiseParamsCb_CTb(0)noiseParamsCb_CTb(1)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_CB_1       tdNtNpCContribCB1;      ///< noiseParamsCb_CTb(2)noiseParamsCb_CTb(3)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_CB_2       tdNtNpCContribCB2;      ///< noiseParamsCb_CTb(4)noiseParamsCb_CTb(5)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_CB_3       tdNtNpCContribCB3;      ///< noiseParamsCb_CTb(6)noiseParamsCb_CTb(7)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_CR_0       tdNtNpCContribCR0;      ///< noiseParamsCr_CTb(0)noiseParamsCr_CTb(1)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_CR_1       tdNtNpCContribCR1;      ///< noiseParamsCr_CTb(2)noiseParamsCr_CTb(3)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_CR_2       tdNtNpCContribCR2;      ///< noiseParamsCr_CTb(4)noiseParamsCr_CTb(5)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_C_CONTRIB_CR_3       tdNtNpCContribCR3;      ///< noiseParamsCr_CTb(6)noiseParamsCr_CTb(7)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_UVLIMIT              tdNtNpUVLimit;          ///< UVLimit for Y,Cb,Cr noise thresholds
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_TOPLIM               tdNtNpTopLimit;         ///< upper limit for Y,Cb,Cr noise thresholds
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_NP_BOTLIM               tdNtNpBottomLimit;      ///< lower limit for Y,Cb,Cr noise thresholds
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNR_LUT_Y_0             tdNtLnrLutY0;           ///< LnrLutY(0..3)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNR_LUT_Y_1             tdNtLnrLutY1;           ///< LnrLutY(4..7)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNR_LUT_Y_2             tdNtLnrLutY2;           ///< LnrLutY(8..11)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNR_LUT_Y_3             tdNtLnrLutY3;           ///< LnrLutY(12..15)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNR_LUT_C_0             tdNtLnrLutC0;           ///< LnrLutC(0..3)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNR_LUT_C_1             tdNtLnrLutC1;           ///< LnrLutC(4..7)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNR_LUT_C_2             tdNtLnrLutC2;           ///< LnrLutC(8..11)
    IPE_IPE_0_NPS_CLC_TF_TF_TD_NT_LNR_LUT_C_3             tdNtLnrLutC3;           ///< LnrLutC(12..15)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSY_0        fsDecisionParamsY0;     ///< fsDecisionParamsY_c{1,2,3,4}(0)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSY_1        fsDecisionParamsY1;     ///< fsDecisionParamsY_c{1,2,3,4}(1)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSY_2        fsDecisionParamsY2;     ///< fsDecisionParamsY_c{1,2,3,4}(2)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSY_3        fsDecisionParamsY3;     ///< fsDecisionParamsY_c{1,2,3,4}(3)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSY_4        fsDecisionParamsY4;     ///< fsDecisionParamsY_c{1,2,3,4}(4)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSY_5        fsDecisionParamsY5;     ///< fsDecisionParamsY_c{1,2,3,4}(5)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSY_6        fsDecisionParamsY6;     ///< fsDecisionParamsY_c{1,2,3,4}(6)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSY_7        fsDecisionParamsY7;     ///< fsDecisionParamsY_c{1,2,3,4}(7)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSY_8        fsDecisionParamsY8;     ///< fsDecisionParamsY_c{1,2,3,4}(8)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSOOFY       fsDecisionParamsOOFY;   ///< c1,c2,c3,c4 parameters in case of
                                                                                  ///< out-of-frame Luma pixel
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSC_0        fsDecisionParamsC0;     ///< fsDecisionParamsC_c{1,2,3,4}(0)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSC_1        fsDecisionParamsC1;     ///< fsDecisionParamsC_c{1,2,3,4}(1)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSC_2        fsDecisionParamsC2;     ///< fsDecisionParamsC_c{1,2,3,4}(2)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSC_3        fsDecisionParamsC3;     ///< fsDecisionParamsC_c{1,2,3,4}(3)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSC_4        fsDecisionParamsC4;     ///< fsDecisionParamsC_c{1,2,3,4}(4)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSC_5        fsDecisionParamsC5;     ///< fsDecisionParamsC_c{1,2,3,4}(5)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSC_6        fsDecisionParamsC6;     ///< fsDecisionParamsC_c{1,2,3,4}(6)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSC_7        fsDecisionParamsC7;     ///< fsDecisionParamsC_c{1,2,3,4}(7)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSC_8        fsDecisionParamsC8;     ///< fsDecisionParamsC_c{1,2,3,4}(8)
    IPE_IPE_0_NPS_CLC_TF_TF_SD_FSDECISIONPARAMSOOFC       fsDecisionParamsOOFC;   ///< c1,c2,c3,c4 parameters in case of
                                                                                  ///< out-of-frame Chroma pixel
    IPE_IPE_0_NPS_CLC_TF_TF_SD_A3T1T2_SCALE               a3T1T2Scale;            ///< a3T1Scale{Y,C} and a3T2Scale{Y,C} param
    IPE_IPE_0_NPS_CLC_TF_TF_SD_A3T1_OFFS                  a3T1OFFS;               ///< a3T1Offs{Y,C} parameters
    IPE_IPE_0_NPS_CLC_TF_TF_SD_A3T2_OFFS                  a3T2OFFS;               ///< a3T1Offs{Y,C} and a3T2Offs{Y,C} param
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_A2_MIN_MAX              a2MinMax;               ///< A2Min{Y,C} and A2Max{Y,C} parameters
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_A2_SLOPE                a2Slope;                ///< a2Slope{Y,C} parameters
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_FSTOA1A4MAP_0           fsToA1A4Map0;           ///< fsToA1Map{Y,C}(0) and fsToA4Map{Y,C}(0)
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_FSTOA1A4MAP_1           fsToA1A4Map1;           ///< fsToA1Map{Y,C}(1) and fsToA4Map{Y,C}(1)
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_FSTOA1A4MAP_2           fsToA1A4Map2;           ///< fsToA1Map{Y,C}(2) and fsToA4Map{Y,C}(2)
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_FSTOA1A4MAP_3           fsToA1A4Map3;           ///< fsToA1Map{Y,C}(3) and fsToA4Map{Y,C}(3)
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_FSTOA1A4MAP_4           fsToA1A4Map4;           ///< fsToA1Map{Y,C}(4) and fsToA4Map{Y,C}(4)
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_FSTOA1A4MAP_5           fsToA1A4Map5;           ///< fsToA1Map{Y,C}(5) and fsToA4Map{Y,C}(5)
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_FSTOA1A4MAP_6           fsToA1A4Map6;           ///< fsToA1Map{Y,C}(6) and fsToA4Map{Y,C}(6)
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_FSTOA1A4MAP_7           fsToA1A4Map7;           ///< fsToA1Map{Y,C}(7) and fsToA4Map{Y,C}(7)
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_FSTOA1A4MAP_8           fsToA1A4Map8;           ///< fsToA1Map{Y,C}(8) and fsToA4Map{Y,C}(8)
    IPE_IPE_0_NPS_CLC_TF_TF_BLEND_CONSTANTBLENDINGFACTOR  constantBlendingFactor; ///< constantBlendingFactor{Y,C} parameters
} CAMX_PACKED;

CAMX_END_PACKED

#endif // TFBASEREGCMD_H
