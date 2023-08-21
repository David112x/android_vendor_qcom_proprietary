////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipelenr10titan480.cpp
/// @brief CAMXIPELENR10TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxipelenr10.h"
#include "camxipelenr10titan480.h"
#include "titan480_ipe.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief LENR10 register Configuration
struct IPELENR10RegConfig
{
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_NZ_CTRL                   dn4BltrNzCtrl;       ///< dn4BltrNzCtrl
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_FNR_GAIN_0                dn4BltrFnrGain0;     ///< dn4BltrFnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_FNR_GAIN_1                dn4BltrFnrGain1;     ///< dn4BltrFnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_FNR_GAIN_2                dn4BltrFnrGain2;     ///< dn4BltrFnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_FNR_GAIN_3                dn4BltrFnrGain3;     ///< dn4BltrFnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_FNR_GAIN_4                dn4BltrFnrGain4;     ///< dn4BltrFnrGain4
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_FNR_GAIN_5                dn4BltrFnrGain5;     ///< dn4BltrFnrGain5
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_FNR_GAIN_6                dn4BltrFnrGain6;     ///< dn4BltrFnrGain6
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_FNR_GAIN_7                dn4BltrFnrGain7;     ///< dn4BltrFnrGain7
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_FNR_GAIN_8                dn4BltrFnrGain8;     ///< dn4BltrFnrGain8
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_SNR_GAIN_0                dn4BltrSnrGain0;     ///< dn4BltrSnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_SNR_GAIN_1                dn4BltrSnrGain1;     ///< dn4BltrSnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_SNR_GAIN_2                dn4BltrSnrGain2;     ///< dn4BltrSnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_SNR_GAIN_3                dn4BltrSnrGain3;     ///< dn4BltrSnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_NZ_CTRL                   dn8BltrNzCtrl;       ///< dn8BltrNzCtrl
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_FNR_GAIN_0                dn8BltrFnrGain0;     ///< dn8BltrFnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_FNR_GAIN_1                dn8BltrFnrGain1;     ///< dn8BltrFnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_FNR_GAIN_2                dn8BltrFnrGain2;     ///< dn8BltrFnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_FNR_GAIN_3                dn8BltrFnrGain3;     ///< dn8BltrFnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_FNR_GAIN_4                dn8BltrFnrGain4;     ///< dn8BltrFnrGain4
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_FNR_GAIN_5                dn8BltrFnrGain5;     ///< dn8BltrFnrGain5
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_FNR_GAIN_6                dn8BltrFnrGain6;     ///< dn8BltrFnrGain6
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_FNR_GAIN_7                dn8BltrFnrGain7;     ///< dn8BltrFnrGain7
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_FNR_GAIN_8                dn8BltrFnrGain8;     ///< dn8BltrFnrGain8
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_SNR_GAIN_0                dn8BltrSnrGain0;     ///< dn8BltrSnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_SNR_GAIN_1                dn8BltrSnrGain1;     ///< dn8BltrSnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_SNR_GAIN_2                dn8BltrSnrGain2;     ///< dn8BltrSnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN8_BLTR_SNR_GAIN_3                dn8BltrSnrGain3;     ///< dn8BltrSnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_NZ_CTRL                  dn16BltrNzCtrl;      ///< dn16BltrNzCtrl
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_FNR_GAIN_0               dn16BltrFnrGain0;    ///< dn16BltrFnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_FNR_GAIN_1               dn16BltrFnrGain1;    ///< dn16BltrFnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_FNR_GAIN_2               dn16BltrFnrGain2;    ///< dn16BltrFnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_FNR_GAIN_3               dn16BltrFnrGain3;    ///< dn16BltrFnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_FNR_GAIN_4               dn16BltrFnrGain4;    ///< dn16BltrFnrGain4
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_FNR_GAIN_5               dn16BltrFnrGain5;    ///< dn16BltrFnrGain5
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_FNR_GAIN_6               dn16BltrFnrGain6;    ///< dn16BltrFnrGain6
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_FNR_GAIN_7               dn16BltrFnrGain7;    ///< dn16BltrFnrGain7
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_FNR_GAIN_8               dn16BltrFnrGain8;    ///< dn16BltrFnrGain8
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_SNR_GAIN_0               dn16BltrSnrGain0;    ///< dn16BltrSnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_SNR_GAIN_1               dn16BltrSnrGain1;    ///< dn16BltrSnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_SNR_GAIN_2               dn16BltrSnrGain2;    ///< dn16BltrSnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN16_BLTR_SNR_GAIN_3               dn16BltrSnrGain3;    ///< dn16BltrSnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_HPF_POS                    dn4LceHpfPos;        ///< dn4LceHpfPos
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_HPF_NEG                    dn4LceHpfNeg;        ///< dn4LceHpfNeg
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_FNR_GAIN_0                 dn4LceFnrGain0;      ///< dn4LceFnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_FNR_GAIN_1                 dn4LceFnrGain1;      ///< dn4LceFnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_FNR_GAIN_2                 dn4LceFnrGain2;      ///< dn4LceFnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_FNR_GAIN_3                 dn4LceFnrGain3;      ///< dn4LceFnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_FNR_GAIN_4                 dn4LceFnrGain4;      ///< dn4LceFnrGain4
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_FNR_GAIN_5                 dn4LceFnrGain5;      ///< dn4LceFnrGain5
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_FNR_GAIN_6                 dn4LceFnrGain6;      ///< dn4LceFnrGain6
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_FNR_GAIN_7                 dn4LceFnrGain7;      ///< dn4LceFnrGain7
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_FNR_GAIN_8                 dn4LceFnrGain8;      ///< dn4LceFnrGain8
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_SNR_GAIN_0                 dn4LceSnrGain0;      ///< dn4LceSnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_SNR_GAIN_1                 dn4LceSnrGain1;      ///< dn4LceSnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_SNR_GAIN_2                 dn4LceSnrGain2;      ///< dn4LceSnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN4_LCE_SNR_GAIN_3                 dn4LceSnrGain3;      ///< dn4LceSnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_HPF_POS                    dn8LceHpfPos;        ///< dn8LceHpfPos
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_HPF_NEG                    dn8LceHpfNeg;        ///< dn8LceHpfNeg
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_FNR_GAIN_0                 dn8LceFnrGain0;      ///< dn8LceFnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_FNR_GAIN_1                 dn8LceFnrGain1;      ///< dn8LceFnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_FNR_GAIN_2                 dn8LceFnrGain2;      ///< dn8LceFnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_FNR_GAIN_3                 dn8LceFnrGain3;      ///< dn8LceFnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_FNR_GAIN_4                 dn8LceFnrGain4;      ///< dn8LceFnrGain4
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_FNR_GAIN_5                 dn8LceFnrGain5;      ///< dn8LceFnrGain5
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_FNR_GAIN_6                 dn8LceFnrGain6;      ///< dn8LceFnrGain6
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_FNR_GAIN_7                 dn8LceFnrGain7;      ///< dn8LceFnrGain7
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_FNR_GAIN_8                 dn8LceFnrGain8;      ///< dn8LceFnrGain8
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_SNR_GAIN_0                 dn8LceSnrGain0;      ///< dn8LceSnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_SNR_GAIN_1                 dn8LceSnrGain1;      ///< dn8LceSnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_SNR_GAIN_2                 dn8LceSnrGain2;      ///< dn8LceSnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN8_LCE_SNR_GAIN_3                 dn8LceSnrGain3;      ///< dn8LceSnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_HPF_POS                   dn16LceHpfPos;       ///< dn16LceHpfPos
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_HPF_NEG                   dn16LceHpfNeg;       ///< dn16LceHpfNeg
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_FNR_GAIN_0                dn16LceFnrGain0;     ///< dn16LceFnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_FNR_GAIN_1                dn16LceFnrGain1;     ///< dn16LceFnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_FNR_GAIN_2                dn16LceFnrGain2;     ///< dn16LceFnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_FNR_GAIN_3                dn16LceFnrGain3;     ///< dn16LceFnrGain3
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_FNR_GAIN_4                dn16LceFnrGain4;     ///< dn16LceFnrGain4
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_FNR_GAIN_5                dn16LceFnrGain5;     ///< dn16LceFnrGain5
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_FNR_GAIN_6                dn16LceFnrGain6;     ///< dn16LceFnrGain6
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_FNR_GAIN_7                dn16LceFnrGain7;     ///< dn16LceFnrGain7
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_FNR_GAIN_8                dn16LceFnrGain8;     ///< dn16LceFnrGain8
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_SNR_GAIN_0                dn16LceSnrGain0;     ///< dn16LceSnrGain0
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_SNR_GAIN_1                dn16LceSnrGain1;     ///< dn16LceSnrGain1
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_SNR_GAIN_2                dn16LceSnrGain2;     ///< dn16LceSnrGain2
    IPE_IPE_0_PPS_CLC_LENR_DN16_LCE_SNR_GAIN_3                dn16LceSnrGain3;     ///< dn16LceSnrGain3
    IPE_IPE_0_PPS_CLC_LENR_ALL_LCE_KERNEL_0                   allLceKernel0;       ///< allLceKernel0
    IPE_IPE_0_PPS_CLC_LENR_ALL_LCE_KERNEL_1                   allLceKernel1;       ///< allLceKernel1
    IPE_IPE_0_PPS_CLC_LENR_ALL_LCE_KERNEL_2                   allLceKernel2;       ///< allLceKernel2
    IPE_IPE_0_PPS_CLC_LENR_ALL_LCE_KERNEL_3                   allLceKernel3;       ///< allLceKernel3
    IPE_IPE_0_PPS_CLC_LENR_ALL_LCE_KERNEL_4                   allLceKernel4;       ///< allLceKernel4
    IPE_IPE_0_PPS_CLC_LENR_ALL_CMN_SETTING                    allCmnSetting;       ///< allCmnSetting
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_ANCHOR_BASE_SETTINGS_0    bltrRnrAnchorBaseSetting0;    ///< bltrRnrAnchorBaseSetting0
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_SLOPE_SHIFT_SETTINGS_0    bltrRnrSlopeShiftSetting0;    ///< bltrRnrSlopeShiftSetting0
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_ANCHOR_BASE_SETTINGS_1    bltrRnrAnchorBaseSetting1;    ///< bltrRnrAnchorBaseSetting1
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_SLOPE_SHIFT_SETTINGS_1    bltrRnrSlopeShiftSetting1;    ///< bltrRnrSlopeShiftSetting1
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_ANCHOR_BASE_SETTINGS_2    bltrRnrAnchorBaseSetting2;    ///< bltrRnrAnchorBaseSetting2
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_SLOPE_SHIFT_SETTINGS_2    bltrRnrSlopeShiftSetting2;    ///< bltrRnrSlopeShiftSetting2
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_ANCHOR_BASE_SETTINGS_3    bltrRnrAnchorBaseSetting3;    ///< bltrRnrAnchorBaseSetting3
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_SLOPE_SHIFT_SETTINGS_3    bltrRnrSlopeShiftSetting3;    ///< bltrRnrSlopeShiftSetting3
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_ANCHOR_BASE_SETTINGS_4    bltrRnrAnchorBaseSetting4;    ///< bltrRnrAnchorBaseSetting4
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_SLOPE_SHIFT_SETTINGS_4    bltrRnrSlopeShiftSetting4;    ///< bltrRnrSlopeShiftSetting4
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_ANCHOR_BASE_SETTINGS_5    bltrRnrAnchorBaseSetting5;    ///< bltrRnrAnchorBaseSetting5
    IPE_IPE_0_PPS_CLC_LENR_BLTR_RNR_SLOPE_SHIFT_SETTINGS_5    bltrRnrSlopeShiftSetting5;    ///< bltrRnrSlopeShiftSetting5
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_ANCHOR_BASE_SETTINGS_0     lceRnrAnchorBaseSetting0;     ///< lceRnrAnchorBaseSetting0
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_SLOPE_SHIFT_SETTINGS_0     lceRnrSlopeShiftSetting0;     ///< lceRnrSlopeShiftSetting0
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_ANCHOR_BASE_SETTINGS_1     lceRnrAnchorBaseSetting1;     ///< lceRnrAnchorBaseSetting1
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_SLOPE_SHIFT_SETTINGS_1     lceRnrSlopeShiftSetting1;     ///< lceRnrSlopeShiftSetting1
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_ANCHOR_BASE_SETTINGS_2     lceRnrAnchorBaseSetting2;     ///< lceRnrAnchorBaseSetting2
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_SLOPE_SHIFT_SETTINGS_2     lceRnrSlopeShiftSetting2;     ///< lceRnrSlopeShiftSetting2
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_ANCHOR_BASE_SETTINGS_3     lceRnrAnchorBaseSetting3;     ///< lceRnrAnchorBaseSetting3
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_SLOPE_SHIFT_SETTINGS_3     lceRnrSlopeShiftSetting3;     ///< lceRnrSlopeShiftSetting3
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_ANCHOR_BASE_SETTINGS_4     lceRnrAnchorBaseSetting4;     ///< lceRnrAnchorBaseSetting4
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_SLOPE_SHIFT_SETTINGS_4     lceRnrSlopeShiftSetting4;     ///< lceRnrSlopeShiftSetting4
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_ANCHOR_BASE_SETTINGS_5     lceRnrAnchorBaseSetting5;     ///< lceRnrAnchorBaseSetting5
    IPE_IPE_0_PPS_CLC_LENR_LCE_RNR_SLOPE_SHIFT_SETTINGS_5     lceRnrSlopeShiftSetting5;     ///< lceRnrSlopeShiftSetting5
    IPE_IPE_0_PPS_CLC_LENR_DN4_RNR_INIT_HV_OFFSET             dn4RnrInitHvOffset;           ///< dn4RnrInitHvOffset
    IPE_IPE_0_PPS_CLC_LENR_DN4_RNR_R_SQUARE_INIT              dn4RnrRSquareInit;            ///< dn4RnrRSquareInit
    IPE_IPE_0_PPS_CLC_LENR_DN4_RNR_SQUARE_SCALE_SHIFT         dn4RnrRSquareScaleShift;      ///< dn4RnrRSquareScaleShift
    IPE_IPE_0_PPS_CLC_LENR_DN8_RNR_INIT_HV_OFFSET             dn8RnrInitHvOffset;           ///< dn8RnrInitHvOffset
    IPE_IPE_0_PPS_CLC_LENR_DN8_RNR_R_SQUARE_INIT              dn8RnrRSquareInit;            ///< dn8RnrRSquareInit
    IPE_IPE_0_PPS_CLC_LENR_DN8_RNR_SQUARE_SCALE_SHIFT         dn8RnrRSquareScaleShift;      ///< dn8RnrRSquareScaleShift
    IPE_IPE_0_PPS_CLC_LENR_DN16_RNR_INIT_HV_OFFSET            dn16RnrInitHvOffset;          ///< dn16RnrInitHvOffset
    IPE_IPE_0_PPS_CLC_LENR_DN16_RNR_R_SQUARE_INIT             dn16RnrRSquareInit;           ///< dn16RnrRSquareInit
    IPE_IPE_0_PPS_CLC_LENR_DN16_RNR_SQUARE_SCALE_SHIFT        dn16RnrRSquareScaleShift;     ///< dn16RnrRSquareScaleShift
    IPE_IPE_0_PPS_CLC_LENR_SNR_CFG_0                          snrCfg0;                      ///< snrCfg0
    IPE_IPE_0_PPS_CLC_LENR_SNR_CFG_1                          snrCfg1;                      ///< snrCfg1
    IPE_IPE_0_PPS_CLC_LENR_SNR_CFG_2                          snrCfg2;                      ///< snrCfg2
    IPE_IPE_0_PPS_CLC_LENR_FACE_CFG                           faceCfg;                      ///< faceCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_OFFSET_CFG                    faceOffsetCfg;                ///< faceOffsetCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_1_CENTER_CFG                  face1CenterCfg;               ///< face1CenterCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_1_RADIUS_CFG                  face1RadiusCfg;               ///< face1RadiusCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_2_CENTER_CFG                  face2CenterCfg;               ///< face2CenterCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_2_RADIUS_CFG                  face2RadiusCfg;               ///< face2RadiusCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_3_CENTER_CFG                  face3CenterCfg;               ///< face3CenterCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_3_RADIUS_CFG                  face3RadiusCfg;               ///< face3RadiusCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_4_CENTER_CFG                  face4CenterCfg;               ///< face4CenterCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_4_RADIUS_CFG                  face4RadiusCfg;               ///< face4RadiusCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_5_CENTER_CFG                  face5CenterCfg;               ///< face5CenterCfg
    IPE_IPE_0_PPS_CLC_LENR_FACE_5_RADIUS_CFG                  face5RadiusCfg;               ///< face5RadiusCfg
    IPE_IPE_0_PPS_CLC_LENR_US4_INIT_PHASE                     us4InitPhase;                 ///< us4InitPhase
    IPE_IPE_0_PPS_CLC_LENR_DN4_WEIGHT_CURVE                   dn4WeightCurve;               ///< dn4WeightCurve
    IPE_IPE_0_PPS_CLC_LENR_DN4_CLAMP                          dn4Clamp;                     ///< dn4Clamp
    IPE_IPE_0_PPS_CLC_LENR_DN8_WEIGHT_CURVE                   dn8WeightCurve;               ///< dn8WeightCurve
    IPE_IPE_0_PPS_CLC_LENR_DN8_CLAMP                          dn8Clamp;                     ///< dn8Clamp
    IPE_IPE_0_PPS_CLC_LENR_DN16_WEIGHT_CURVE                  dn16WeightCurve;              ///< dn16WeightCurve
    IPE_IPE_0_PPS_CLC_LENR_DN16_CLAMP                         dn16Clamp;                    ///< dn16Clamp
    IPE_IPE_0_PPS_CLC_LENR_FE_CROP                            feCrop;                       ///< feCrop
    IPE_IPE_0_PPS_CLC_LENR_BE_CROP_H                          beCropH;                      ///< beCropH
    IPE_IPE_0_PPS_CLC_LENR_BE_CROP_V                          beCropV;                      ///< beCropV
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPELENR10RegLength = sizeof(IPELENR10RegConfig) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10Titan480::IPELENR10Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPELENR10Titan480::IPELENR10Titan480()
{
    m_pRegCmd = CAMX_CALLOC(sizeof(IPELENR10RegConfig));

    if (NULL == m_pRegCmd)
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to allocate memory for Cmd Buffer");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10Titan480::WriteConfigCmds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELENR10Titan480::WriteConfigCmds(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData) && (NULL != m_pRegCmd) && (NULL != pInputData->pipelineIPEData.ppIPECmdBuffer))
    {
        IPELENR10RegConfig* pRegCmd    = static_cast<IPELENR10RegConfig*>(m_pRegCmd);
        CmdBuffer*          pCmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferPreLTM];

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIPE_IPE_0_PPS_CLC_LENR_DN4_BLTR_NZ_CTRL,
                                              IPELENR10RegLength,
                                              reinterpret_cast<UINT32*>(pRegCmd));

        if (result != CamxResultSuccess)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid pointers pInputData: %p, m_pRegCmd: %p, ppIPECmdBuffer: %p",
                       pInputData,
                       m_pRegCmd,
                       (NULL != pInputData) ? pInputData->pipelineIPEData.ppIPECmdBuffer : NULL);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELENR10Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pLENRHWSettingParams)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pLENRHWSettingParams);

    if (CamxResultSuccess == result)
    {
        result = WriteConfigCmds(pInputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPELENR10Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELENR10Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult          result             = CamxResultSuccess;
    ISPInputData*       pInputData         = static_cast<ISPInputData*>(pSettingData);
    IPETuningMetadata*  pIPETuningMetadata = static_cast<IPETuningMetadata*>(pInputData->pIPETuningMetadata);
    IpeIQSettings*      pIPEIQSettings     = static_cast<IpeIQSettings*>(pInputData->pipelineIPEData.pIPEIQSettings);
    IPELENR10RegConfig* pRegCmd            = static_cast<IPELENR10RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if ((NULL != pInputData->pIPETuningMetadata) && (NULL != pRegCmd))
    {
        CAMX_STATIC_ASSERT(sizeof(IPELENR10RegConfig) ==
            sizeof(pIPETuningMetadata->IPETuningMetadata480.IPELENRData.LENRConfig));

        Utils::Memcpy(&pIPETuningMetadata->IPETuningMetadata480.IPELENRData.LENRConfig,
                      pRegCmd,
                      sizeof(IPELENR10RegConfig));

        if (TRUE == pIPEIQSettings->lenrParameters.moduleCfg.EN)
        {
            DebugDataTagID dataTagID;
            if (TRUE == pInputData->pipelineIPEData.realtimeFlag)
            {
                dataTagID = DebugDataTagID::TuningIPELENR10Register;
            }
            else
            {
                dataTagID = DebugDataTagID::TuningIPELENR10RegisterOffline;
            }

            result = pInputData->pipelineIPEData.pDebugDataWriter->AddDataTag(
                dataTagID,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pIPETuningMetadata->IPETuningMetadata480.IPELENRData.LENRConfig),
                &pIPETuningMetadata->IPETuningMetadata480.IPELENRData.LENRConfig,
                sizeof(pIPETuningMetadata->IPETuningMetadata480.IPELENRData.LENRConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupPProc, "AddDataTag failed with error: %d", result);
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELENR10Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult           result          = CamxResultSuccess;
    LenrParameters*      pLENRParameters = NULL;
    UINT                 i               = 0;
    LENR10UnpackedField* pData           = static_cast<LENR10UnpackedField*>(pInput);
    LENR10OutputData*    pOutputData     = static_cast<LENR10OutputData*>(pOutput);
    IPELENR10RegConfig*  pRegCmd         = static_cast<IPELENR10RegConfig*>(m_pRegCmd);

    if ((NULL != pData) && (NULL != pOutputData) && (NULL != pOutputData->pLENRParams))
    {
        pLENRParameters = pOutputData->pLENRParams;

        pRegCmd->dn4BltrNzCtrl.bitfields.CTRL_TH = pData->LENRDn4BltrCtrlTh;
        pRegCmd->dn4BltrNzCtrl.bitfields.CTRL_W  = pData->LENRDn4BltrCtrlW;

        pRegCmd->dn4BltrFnrGain0.bitfields.GAIN_0  = pData->LENRDn4BltrFnrGainArr[0];
        pRegCmd->dn4BltrFnrGain0.bitfields.GAIN_1  = pData->LENRDn4BltrFnrGainArr[1];
        pRegCmd->dn4BltrFnrGain1.bitfields.GAIN_2  = pData->LENRDn4BltrFnrGainArr[2];
        pRegCmd->dn4BltrFnrGain1.bitfields.GAIN_3  = pData->LENRDn4BltrFnrGainArr[3];
        pRegCmd->dn4BltrFnrGain2.bitfields.GAIN_4  = pData->LENRDn4BltrFnrGainArr[4];
        pRegCmd->dn4BltrFnrGain2.bitfields.GAIN_5  = pData->LENRDn4BltrFnrGainArr[5];
        pRegCmd->dn4BltrFnrGain3.bitfields.GAIN_6  = pData->LENRDn4BltrFnrGainArr[6];
        pRegCmd->dn4BltrFnrGain3.bitfields.GAIN_7  = pData->LENRDn4BltrFnrGainArr[7];
        pRegCmd->dn4BltrFnrGain4.bitfields.GAIN_8  = pData->LENRDn4BltrFnrGainArr[8];
        pRegCmd->dn4BltrFnrGain4.bitfields.GAIN_9  = pData->LENRDn4BltrFnrGainArr[9];
        pRegCmd->dn4BltrFnrGain5.bitfields.GAIN_10 = pData->LENRDn4BltrFnrGainArr[10];
        pRegCmd->dn4BltrFnrGain5.bitfields.GAIN_11 = pData->LENRDn4BltrFnrGainArr[11];
        pRegCmd->dn4BltrFnrGain6.bitfields.GAIN_12 = pData->LENRDn4BltrFnrGainArr[12];
        pRegCmd->dn4BltrFnrGain6.bitfields.GAIN_13 = pData->LENRDn4BltrFnrGainArr[13];
        pRegCmd->dn4BltrFnrGain7.bitfields.GAIN_14 = pData->LENRDn4BltrFnrGainArr[14];
        pRegCmd->dn4BltrFnrGain7.bitfields.GAIN_15 = pData->LENRDn4BltrFnrGainArr[15];
        pRegCmd->dn4BltrFnrGain8.bitfields.GAIN_16 = pData->LENRDn4BltrFnrGainArr[16];


        pRegCmd->dn4BltrSnrGain0.bitfields.IDX_0  = pData->LENRDn4BltrSnrGainArr[0];
        pRegCmd->dn4BltrSnrGain0.bitfields.IDX_1  = pData->LENRDn4BltrSnrGainArr[1];
        pRegCmd->dn4BltrSnrGain0.bitfields.IDX_2  = pData->LENRDn4BltrSnrGainArr[2];
        pRegCmd->dn4BltrSnrGain0.bitfields.IDX_3  = pData->LENRDn4BltrSnrGainArr[3];
        pRegCmd->dn4BltrSnrGain1.bitfields.IDX_0  = pData->LENRDn4BltrSnrGainArr[4];
        pRegCmd->dn4BltrSnrGain1.bitfields.IDX_1  = pData->LENRDn4BltrSnrGainArr[5];
        pRegCmd->dn4BltrSnrGain1.bitfields.IDX_2  = pData->LENRDn4BltrSnrGainArr[6];
        pRegCmd->dn4BltrSnrGain1.bitfields.IDX_3  = pData->LENRDn4BltrSnrGainArr[7];
        pRegCmd->dn4BltrSnrGain2.bitfields.IDX_0  = pData->LENRDn4BltrSnrGainArr[8];
        pRegCmd->dn4BltrSnrGain2.bitfields.IDX_1  = pData->LENRDn4BltrSnrGainArr[9];
        pRegCmd->dn4BltrSnrGain2.bitfields.IDX_2  = pData->LENRDn4BltrSnrGainArr[10];
        pRegCmd->dn4BltrSnrGain2.bitfields.IDX_3  = pData->LENRDn4BltrSnrGainArr[11];
        pRegCmd->dn4BltrSnrGain3.bitfields.IDX_0  = pData->LENRDn4BltrSnrGainArr[12];
        pRegCmd->dn4BltrSnrGain3.bitfields.IDX_1  = pData->LENRDn4BltrSnrGainArr[13];
        pRegCmd->dn4BltrSnrGain3.bitfields.IDX_2  = pData->LENRDn4BltrSnrGainArr[14];
        pRegCmd->dn4BltrSnrGain3.bitfields.IDX_3  = pData->LENRDn4BltrSnrGainArr[15];
        pRegCmd->dn4BltrSnrGain3.bitfields.IDX_4  = pData->LENRDn4BltrSnrGainArr[16];

        pRegCmd->dn8BltrNzCtrl.bitfields.CTRL_TH  = pData->LENRDn8BltrCtrlTh;
        pRegCmd->dn8BltrNzCtrl.bitfields.CTRL_W   = pData->LENRDn8BltrCtrlW;

        pRegCmd->dn8BltrFnrGain0.bitfields.GAIN_0  = pData->LENRDn8BltrFnrGainArr[0];
        pRegCmd->dn8BltrFnrGain0.bitfields.GAIN_1  = pData->LENRDn8BltrFnrGainArr[1];
        pRegCmd->dn8BltrFnrGain1.bitfields.GAIN_2  = pData->LENRDn8BltrFnrGainArr[2];
        pRegCmd->dn8BltrFnrGain1.bitfields.GAIN_3  = pData->LENRDn8BltrFnrGainArr[3];
        pRegCmd->dn8BltrFnrGain2.bitfields.GAIN_4  = pData->LENRDn8BltrFnrGainArr[4];
        pRegCmd->dn8BltrFnrGain2.bitfields.GAIN_5  = pData->LENRDn8BltrFnrGainArr[5];
        pRegCmd->dn8BltrFnrGain3.bitfields.GAIN_6  = pData->LENRDn8BltrFnrGainArr[6];
        pRegCmd->dn8BltrFnrGain3.bitfields.GAIN_7  = pData->LENRDn8BltrFnrGainArr[7];
        pRegCmd->dn8BltrFnrGain4.bitfields.GAIN_8  = pData->LENRDn8BltrFnrGainArr[8];
        pRegCmd->dn8BltrFnrGain4.bitfields.GAIN_9  = pData->LENRDn8BltrFnrGainArr[9];
        pRegCmd->dn8BltrFnrGain5.bitfields.GAIN_10 = pData->LENRDn8BltrFnrGainArr[10];
        pRegCmd->dn8BltrFnrGain5.bitfields.GAIN_11 = pData->LENRDn8BltrFnrGainArr[11];
        pRegCmd->dn8BltrFnrGain6.bitfields.GAIN_12 = pData->LENRDn8BltrFnrGainArr[12];
        pRegCmd->dn8BltrFnrGain6.bitfields.GAIN_13 = pData->LENRDn8BltrFnrGainArr[13];
        pRegCmd->dn8BltrFnrGain7.bitfields.GAIN_14 = pData->LENRDn8BltrFnrGainArr[14];
        pRegCmd->dn8BltrFnrGain7.bitfields.GAIN_15 = pData->LENRDn8BltrFnrGainArr[15];
        pRegCmd->dn8BltrFnrGain8.bitfields.GAIN_16 = pData->LENRDn8BltrFnrGainArr[16];


        pRegCmd->dn8BltrSnrGain0.bitfields.IDX_0   = pData->LENRDn8BltrSnrGainArr[0];
        pRegCmd->dn8BltrSnrGain0.bitfields.IDX_1   = pData->LENRDn8BltrSnrGainArr[1];
        pRegCmd->dn8BltrSnrGain0.bitfields.IDX_2   = pData->LENRDn8BltrSnrGainArr[2];
        pRegCmd->dn8BltrSnrGain0.bitfields.IDX_3   = pData->LENRDn8BltrSnrGainArr[3];
        pRegCmd->dn8BltrSnrGain1.bitfields.IDX_0   = pData->LENRDn8BltrSnrGainArr[4];
        pRegCmd->dn8BltrSnrGain1.bitfields.IDX_1   = pData->LENRDn8BltrSnrGainArr[5];
        pRegCmd->dn8BltrSnrGain1.bitfields.IDX_2   = pData->LENRDn8BltrSnrGainArr[6];
        pRegCmd->dn8BltrSnrGain1.bitfields.IDX_3   = pData->LENRDn8BltrSnrGainArr[7];
        pRegCmd->dn8BltrSnrGain2.bitfields.IDX_0   = pData->LENRDn8BltrSnrGainArr[8];
        pRegCmd->dn8BltrSnrGain2.bitfields.IDX_1   = pData->LENRDn8BltrSnrGainArr[9];
        pRegCmd->dn8BltrSnrGain2.bitfields.IDX_2   = pData->LENRDn8BltrSnrGainArr[10];
        pRegCmd->dn8BltrSnrGain2.bitfields.IDX_3   = pData->LENRDn8BltrSnrGainArr[11];
        pRegCmd->dn8BltrSnrGain3.bitfields.IDX_0   = pData->LENRDn8BltrSnrGainArr[12];
        pRegCmd->dn8BltrSnrGain3.bitfields.IDX_1   = pData->LENRDn8BltrSnrGainArr[13];
        pRegCmd->dn8BltrSnrGain3.bitfields.IDX_2   = pData->LENRDn8BltrSnrGainArr[14];
        pRegCmd->dn8BltrSnrGain3.bitfields.IDX_3   = pData->LENRDn8BltrSnrGainArr[15];
        pRegCmd->dn8BltrSnrGain3.bitfields.IDX_4   = pData->LENRDn8BltrSnrGainArr[16];

        pRegCmd->dn16BltrNzCtrl.bitfields.CTRL_TH   = pData->LENRDn16BltrCtrlTh;
        pRegCmd->dn16BltrNzCtrl.bitfields.CTRL_W    = pData->LENRDn16BltrCtrlW;

        pRegCmd->dn16BltrFnrGain0.bitfields.GAIN_0  = pData->LENRDn16BltrFnrGainArr[0];
        pRegCmd->dn16BltrFnrGain0.bitfields.GAIN_1  = pData->LENRDn16BltrFnrGainArr[1];
        pRegCmd->dn16BltrFnrGain1.bitfields.GAIN_2  = pData->LENRDn16BltrFnrGainArr[2];
        pRegCmd->dn16BltrFnrGain1.bitfields.GAIN_3  = pData->LENRDn16BltrFnrGainArr[3];
        pRegCmd->dn16BltrFnrGain2.bitfields.GAIN_4  = pData->LENRDn16BltrFnrGainArr[4];
        pRegCmd->dn16BltrFnrGain2.bitfields.GAIN_5  = pData->LENRDn16BltrFnrGainArr[5];
        pRegCmd->dn16BltrFnrGain3.bitfields.GAIN_6  = pData->LENRDn16BltrFnrGainArr[6];
        pRegCmd->dn16BltrFnrGain3.bitfields.GAIN_7  = pData->LENRDn16BltrFnrGainArr[7];
        pRegCmd->dn16BltrFnrGain4.bitfields.GAIN_8  = pData->LENRDn16BltrFnrGainArr[8];
        pRegCmd->dn16BltrFnrGain4.bitfields.GAIN_9  = pData->LENRDn16BltrFnrGainArr[9];
        pRegCmd->dn16BltrFnrGain5.bitfields.GAIN_10 = pData->LENRDn16BltrFnrGainArr[10];
        pRegCmd->dn16BltrFnrGain5.bitfields.GAIN_11 = pData->LENRDn16BltrFnrGainArr[11];
        pRegCmd->dn16BltrFnrGain6.bitfields.GAIN_12 = pData->LENRDn16BltrFnrGainArr[12];
        pRegCmd->dn16BltrFnrGain6.bitfields.GAIN_13 = pData->LENRDn16BltrFnrGainArr[13];
        pRegCmd->dn16BltrFnrGain7.bitfields.GAIN_14 = pData->LENRDn16BltrFnrGainArr[14];
        pRegCmd->dn16BltrFnrGain7.bitfields.GAIN_15 = pData->LENRDn16BltrFnrGainArr[15];
        pRegCmd->dn16BltrFnrGain8.bitfields.GAIN_16 = pData->LENRDn16BltrFnrGainArr[16];

        pRegCmd->dn16BltrSnrGain0.bitfields.IDX_0   = pData->LENRDn16BltrSnrGainArr[0];
        pRegCmd->dn16BltrSnrGain0.bitfields.IDX_1   = pData->LENRDn16BltrSnrGainArr[1];
        pRegCmd->dn16BltrSnrGain0.bitfields.IDX_2   = pData->LENRDn16BltrSnrGainArr[2];
        pRegCmd->dn16BltrSnrGain0.bitfields.IDX_3   = pData->LENRDn16BltrSnrGainArr[3];
        pRegCmd->dn16BltrSnrGain1.bitfields.IDX_0   = pData->LENRDn16BltrSnrGainArr[4];
        pRegCmd->dn16BltrSnrGain1.bitfields.IDX_1   = pData->LENRDn16BltrSnrGainArr[5];
        pRegCmd->dn16BltrSnrGain1.bitfields.IDX_2   = pData->LENRDn16BltrSnrGainArr[6];
        pRegCmd->dn16BltrSnrGain1.bitfields.IDX_3   = pData->LENRDn16BltrSnrGainArr[7];
        pRegCmd->dn16BltrSnrGain2.bitfields.IDX_0   = pData->LENRDn16BltrSnrGainArr[8];
        pRegCmd->dn16BltrSnrGain2.bitfields.IDX_1   = pData->LENRDn16BltrSnrGainArr[9];
        pRegCmd->dn16BltrSnrGain2.bitfields.IDX_2   = pData->LENRDn16BltrSnrGainArr[10];
        pRegCmd->dn16BltrSnrGain2.bitfields.IDX_3   = pData->LENRDn16BltrSnrGainArr[11];
        pRegCmd->dn16BltrSnrGain3.bitfields.IDX_0   = pData->LENRDn16BltrSnrGainArr[12];
        pRegCmd->dn16BltrSnrGain3.bitfields.IDX_1   = pData->LENRDn16BltrSnrGainArr[13];
        pRegCmd->dn16BltrSnrGain3.bitfields.IDX_2   = pData->LENRDn16BltrSnrGainArr[14];
        pRegCmd->dn16BltrSnrGain3.bitfields.IDX_3   = pData->LENRDn16BltrSnrGainArr[15];
        pRegCmd->dn16BltrSnrGain3.bitfields.IDX_4   = pData->LENRDn16BltrSnrGainArr[16];

        pRegCmd->dn4LceHpfPos.bitfields.CLAMP  = pData->LENRDn4LceClampP;
        pRegCmd->dn4LceHpfPos.bitfields.CORE   = pData->LENRDn4LceCoreP;
        pRegCmd->dn4LceHpfPos.bitfields.SCALE  = pData->LENRDn4LceScaleP;

        pRegCmd->dn4LceHpfNeg.bitfields.CLAMP  = pData->LENRDn4LceClampN;
        pRegCmd->dn4LceHpfNeg.bitfields.CORE   = pData->LENRDn4LceCoreN;
        pRegCmd->dn4LceHpfNeg.bitfields.SCALE  = pData->LENRDn4LceScaleN;

        pRegCmd->dn4LceFnrGain0.bitfields.LSB_0 = pData->LENRDn4LceFnrGainArr[0];
        pRegCmd->dn4LceFnrGain0.bitfields.LSB_1 = pData->LENRDn4LceFnrGainArr[1];
        pRegCmd->dn4LceFnrGain1.bitfields.LSB_0 = pData->LENRDn4LceFnrGainArr[2];
        pRegCmd->dn4LceFnrGain1.bitfields.LSB_1 = pData->LENRDn4LceFnrGainArr[3];
        pRegCmd->dn4LceFnrGain2.bitfields.LSB_0 = pData->LENRDn4LceFnrGainArr[4];
        pRegCmd->dn4LceFnrGain2.bitfields.LSB_1 = pData->LENRDn4LceFnrGainArr[5];
        pRegCmd->dn4LceFnrGain3.bitfields.LSB_0 = pData->LENRDn4LceFnrGainArr[6];
        pRegCmd->dn4LceFnrGain3.bitfields.LSB_1 = pData->LENRDn4LceFnrGainArr[7];
        pRegCmd->dn4LceFnrGain4.bitfields.LSB_0 = pData->LENRDn4LceFnrGainArr[8];
        pRegCmd->dn4LceFnrGain4.bitfields.LSB_1 = pData->LENRDn4LceFnrGainArr[9];
        pRegCmd->dn4LceFnrGain5.bitfields.LSB_0 = pData->LENRDn4LceFnrGainArr[10];
        pRegCmd->dn4LceFnrGain5.bitfields.LSB_1 = pData->LENRDn4LceFnrGainArr[11];
        pRegCmd->dn4LceFnrGain6.bitfields.LSB_0 = pData->LENRDn4LceFnrGainArr[12];
        pRegCmd->dn4LceFnrGain6.bitfields.LSB_1 = pData->LENRDn4LceFnrGainArr[13];
        pRegCmd->dn4LceFnrGain7.bitfields.LSB_0 = pData->LENRDn4LceFnrGainArr[14];
        pRegCmd->dn4LceFnrGain7.bitfields.LSB_1 = pData->LENRDn4LceFnrGainArr[15];
        pRegCmd->dn4LceFnrGain8.bitfields.LSB_0 = pData->LENRDn4LceFnrGainArr[16];


        pRegCmd->dn4LceSnrGain0.bitfields.IDX_0 = pData->LENRDn4LceSnrGainArr[0];
        pRegCmd->dn4LceSnrGain0.bitfields.IDX_1 = pData->LENRDn4LceSnrGainArr[1];
        pRegCmd->dn4LceSnrGain0.bitfields.IDX_2 = pData->LENRDn4LceSnrGainArr[2];
        pRegCmd->dn4LceSnrGain0.bitfields.IDX_3 = pData->LENRDn4LceSnrGainArr[3];
        pRegCmd->dn4LceSnrGain1.bitfields.IDX_0 = pData->LENRDn4LceSnrGainArr[4];
        pRegCmd->dn4LceSnrGain1.bitfields.IDX_1 = pData->LENRDn4LceSnrGainArr[5];
        pRegCmd->dn4LceSnrGain1.bitfields.IDX_2 = pData->LENRDn4LceSnrGainArr[6];
        pRegCmd->dn4LceSnrGain1.bitfields.IDX_3 = pData->LENRDn4LceSnrGainArr[7];
        pRegCmd->dn4LceSnrGain2.bitfields.IDX_0 = pData->LENRDn4LceSnrGainArr[8];
        pRegCmd->dn4LceSnrGain2.bitfields.IDX_1 = pData->LENRDn4LceSnrGainArr[9];
        pRegCmd->dn4LceSnrGain2.bitfields.IDX_2 = pData->LENRDn4LceSnrGainArr[10];
        pRegCmd->dn4LceSnrGain2.bitfields.IDX_3 = pData->LENRDn4LceSnrGainArr[11];
        pRegCmd->dn4LceSnrGain3.bitfields.IDX_0 = pData->LENRDn4LceSnrGainArr[12];
        pRegCmd->dn4LceSnrGain3.bitfields.IDX_1 = pData->LENRDn4LceSnrGainArr[13];
        pRegCmd->dn4LceSnrGain3.bitfields.IDX_2 = pData->LENRDn4LceSnrGainArr[14];
        pRegCmd->dn4LceSnrGain3.bitfields.IDX_3 = pData->LENRDn4LceSnrGainArr[15];
        pRegCmd->dn4LceSnrGain3.bitfields.IDX_4 = pData->LENRDn4LceSnrGainArr[16];

        pRegCmd->dn8LceHpfPos.bitfields.CLAMP  = pData->LENRDn8LceClampP;
        pRegCmd->dn8LceHpfPos.bitfields.CORE   = pData->LENRDn8LceCoreP;
        pRegCmd->dn8LceHpfPos.bitfields.SCALE  = pData->LENRDn8LceScaleP;

        pRegCmd->dn8LceHpfNeg.bitfields.CLAMP = pData->LENRDn8LceClampN;
        pRegCmd->dn8LceHpfNeg.bitfields.CORE  = pData->LENRDn8LceCoreN;
        pRegCmd->dn8LceHpfNeg.bitfields.SCALE = pData->LENRDn8LceScaleN;

        pRegCmd->dn8LceFnrGain0.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[0];
        pRegCmd->dn8LceFnrGain0.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[1];
        pRegCmd->dn8LceFnrGain1.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[2];
        pRegCmd->dn8LceFnrGain1.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[3];
        pRegCmd->dn8LceFnrGain2.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[4];
        pRegCmd->dn8LceFnrGain2.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[5];
        pRegCmd->dn8LceFnrGain3.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[6];
        pRegCmd->dn8LceFnrGain3.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[7];
        pRegCmd->dn8LceFnrGain4.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[8];
        pRegCmd->dn8LceFnrGain4.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[9];
        pRegCmd->dn8LceFnrGain5.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[10];
        pRegCmd->dn8LceFnrGain5.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[11];
        pRegCmd->dn8LceFnrGain6.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[12];
        pRegCmd->dn8LceFnrGain6.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[13];
        pRegCmd->dn8LceFnrGain7.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[14];
        pRegCmd->dn8LceFnrGain7.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[15];
        pRegCmd->dn8LceFnrGain8.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[16];


        pRegCmd->dn8LceSnrGain0.bitfields.IDX_0 = pData->LENRDn8LceSnrGainArr[0];
        pRegCmd->dn8LceSnrGain0.bitfields.IDX_1 = pData->LENRDn8LceSnrGainArr[1];
        pRegCmd->dn8LceSnrGain0.bitfields.IDX_2 = pData->LENRDn8LceSnrGainArr[2];
        pRegCmd->dn8LceSnrGain0.bitfields.IDX_3 = pData->LENRDn8LceSnrGainArr[3];
        pRegCmd->dn8LceSnrGain1.bitfields.IDX_0 = pData->LENRDn8LceSnrGainArr[4];
        pRegCmd->dn8LceSnrGain1.bitfields.IDX_1 = pData->LENRDn8LceSnrGainArr[5];
        pRegCmd->dn8LceSnrGain1.bitfields.IDX_2 = pData->LENRDn8LceSnrGainArr[6];
        pRegCmd->dn8LceSnrGain1.bitfields.IDX_3 = pData->LENRDn8LceSnrGainArr[7];
        pRegCmd->dn8LceSnrGain2.bitfields.IDX_0 = pData->LENRDn8LceSnrGainArr[8];
        pRegCmd->dn8LceSnrGain2.bitfields.IDX_1 = pData->LENRDn8LceSnrGainArr[9];
        pRegCmd->dn8LceSnrGain2.bitfields.IDX_2 = pData->LENRDn8LceSnrGainArr[10];
        pRegCmd->dn8LceSnrGain2.bitfields.IDX_3 = pData->LENRDn8LceSnrGainArr[11];
        pRegCmd->dn8LceSnrGain3.bitfields.IDX_0 = pData->LENRDn8LceSnrGainArr[12];
        pRegCmd->dn8LceSnrGain3.bitfields.IDX_1 = pData->LENRDn8LceSnrGainArr[13];
        pRegCmd->dn8LceSnrGain3.bitfields.IDX_2 = pData->LENRDn8LceSnrGainArr[14];
        pRegCmd->dn8LceSnrGain3.bitfields.IDX_3 = pData->LENRDn8LceSnrGainArr[15];
        pRegCmd->dn8LceSnrGain3.bitfields.IDX_4 = pData->LENRDn8LceSnrGainArr[16];


        pRegCmd->dn16LceHpfPos.bitfields.CLAMP = pData->LENRDn16LceClampP;
        pRegCmd->dn16LceHpfPos.bitfields.CORE  = pData->LENRDn16LceCoreP;
        pRegCmd->dn16LceHpfPos.bitfields.SCALE = pData->LENRDn16LceScaleP;

        pRegCmd->dn16LceHpfNeg.bitfields.CLAMP = pData->LENRDn16LceClampN;
        pRegCmd->dn16LceHpfNeg.bitfields.CORE  = pData->LENRDn16LceCoreN;
        pRegCmd->dn16LceHpfNeg.bitfields.SCALE = pData->LENRDn16LceScaleN;

        pRegCmd->dn8LceFnrGain0.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[0];
        pRegCmd->dn8LceFnrGain0.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[1];
        pRegCmd->dn8LceFnrGain1.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[2];
        pRegCmd->dn8LceFnrGain1.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[3];
        pRegCmd->dn8LceFnrGain2.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[4];
        pRegCmd->dn8LceFnrGain2.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[5];
        pRegCmd->dn8LceFnrGain3.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[6];
        pRegCmd->dn8LceFnrGain3.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[7];
        pRegCmd->dn8LceFnrGain4.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[8];
        pRegCmd->dn8LceFnrGain4.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[9];
        pRegCmd->dn8LceFnrGain5.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[10];
        pRegCmd->dn8LceFnrGain5.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[11];
        pRegCmd->dn8LceFnrGain6.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[12];
        pRegCmd->dn8LceFnrGain6.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[13];
        pRegCmd->dn8LceFnrGain7.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[14];
        pRegCmd->dn8LceFnrGain7.bitfields.LSB_1 = pData->LENRDn8LceFnrGainArr[15];
        pRegCmd->dn8LceFnrGain8.bitfields.LSB_0 = pData->LENRDn8LceFnrGainArr[16];

        pRegCmd->dn16LceFnrGain0.bitfields.LSB_0 = pData->LENRDn16LceFnrGainArr[0];
        pRegCmd->dn16LceFnrGain0.bitfields.LSB_1 = pData->LENRDn16LceFnrGainArr[1];
        pRegCmd->dn16LceFnrGain1.bitfields.LSB_0 = pData->LENRDn16LceFnrGainArr[2];
        pRegCmd->dn16LceFnrGain1.bitfields.LSB_1 = pData->LENRDn16LceFnrGainArr[3];
        pRegCmd->dn16LceFnrGain2.bitfields.LSB_0 = pData->LENRDn16LceFnrGainArr[4];
        pRegCmd->dn16LceFnrGain2.bitfields.LSB_1 = pData->LENRDn16LceFnrGainArr[5];
        pRegCmd->dn16LceFnrGain3.bitfields.LSB_0 = pData->LENRDn16LceFnrGainArr[6];
        pRegCmd->dn16LceFnrGain3.bitfields.LSB_1 = pData->LENRDn16LceFnrGainArr[7];
        pRegCmd->dn16LceFnrGain4.bitfields.LSB_0 = pData->LENRDn16LceFnrGainArr[8];
        pRegCmd->dn16LceFnrGain4.bitfields.LSB_1 = pData->LENRDn16LceFnrGainArr[9];
        pRegCmd->dn16LceFnrGain5.bitfields.LSB_0 = pData->LENRDn16LceFnrGainArr[10];
        pRegCmd->dn16LceFnrGain5.bitfields.LSB_1 = pData->LENRDn16LceFnrGainArr[11];
        pRegCmd->dn16LceFnrGain6.bitfields.LSB_0 = pData->LENRDn16LceFnrGainArr[12];
        pRegCmd->dn16LceFnrGain6.bitfields.LSB_1 = pData->LENRDn16LceFnrGainArr[13];
        pRegCmd->dn16LceFnrGain7.bitfields.LSB_0 = pData->LENRDn16LceFnrGainArr[14];
        pRegCmd->dn16LceFnrGain7.bitfields.LSB_1 = pData->LENRDn16LceFnrGainArr[15];
        pRegCmd->dn16LceFnrGain8.bitfields.LSB_0 = pData->LENRDn16LceFnrGainArr[16];

        pRegCmd->dn16LceSnrGain0.bitfields.IDX_0 = pData->LENRDn16LceSnrGainArr[0];
        pRegCmd->dn16LceSnrGain0.bitfields.IDX_1 = pData->LENRDn16LceSnrGainArr[1];
        pRegCmd->dn16LceSnrGain0.bitfields.IDX_2 = pData->LENRDn16LceSnrGainArr[2];
        pRegCmd->dn16LceSnrGain0.bitfields.IDX_3 = pData->LENRDn16LceSnrGainArr[3];
        pRegCmd->dn16LceSnrGain1.bitfields.IDX_0 = pData->LENRDn16LceSnrGainArr[4];
        pRegCmd->dn16LceSnrGain1.bitfields.IDX_1 = pData->LENRDn16LceSnrGainArr[5];
        pRegCmd->dn16LceSnrGain1.bitfields.IDX_2 = pData->LENRDn16LceSnrGainArr[6];
        pRegCmd->dn16LceSnrGain1.bitfields.IDX_3 = pData->LENRDn16LceSnrGainArr[7];
        pRegCmd->dn16LceSnrGain2.bitfields.IDX_0 = pData->LENRDn16LceSnrGainArr[8];
        pRegCmd->dn16LceSnrGain2.bitfields.IDX_1 = pData->LENRDn16LceSnrGainArr[9];
        pRegCmd->dn16LceSnrGain2.bitfields.IDX_2 = pData->LENRDn16LceSnrGainArr[10];
        pRegCmd->dn16LceSnrGain2.bitfields.IDX_3 = pData->LENRDn16LceSnrGainArr[11];
        pRegCmd->dn16LceSnrGain3.bitfields.IDX_0 = pData->LENRDn16LceSnrGainArr[12];
        pRegCmd->dn16LceSnrGain3.bitfields.IDX_1 = pData->LENRDn16LceSnrGainArr[13];
        pRegCmd->dn16LceSnrGain3.bitfields.IDX_2 = pData->LENRDn16LceSnrGainArr[14];
        pRegCmd->dn16LceSnrGain3.bitfields.IDX_3 = pData->LENRDn16LceSnrGainArr[15];
        pRegCmd->dn16LceSnrGain3.bitfields.IDX_4 = pData->LENRDn16LceSnrGainArr[16];

        pRegCmd->allLceKernel0.bitfields.COEF_0_F0 = pData->LENRAllLceKernel[0][0];
        pRegCmd->allLceKernel0.bitfields.COEF_0_F1 = pData->LENRAllLceKernel[0][1];
        pRegCmd->allLceKernel0.bitfields.COEF_0_F2 = pData->LENRAllLceKernel[0][2];
        pRegCmd->allLceKernel0.bitfields.COEF_0_F3 = pData->LENRAllLceKernel[0][3];

        pRegCmd->allLceKernel0.bitfields.COEF_1_F0 = pData->LENRAllLceKernel[1][0];
        pRegCmd->allLceKernel0.bitfields.COEF_1_F1 = pData->LENRAllLceKernel[1][1];
        pRegCmd->allLceKernel0.bitfields.COEF_1_F2 = pData->LENRAllLceKernel[1][2];
        pRegCmd->allLceKernel0.bitfields.COEF_1_F3 = pData->LENRAllLceKernel[1][3];

        pRegCmd->allLceKernel1.bitfields.COEF_2_F0 = pData->LENRAllLceKernel[2][0];
        pRegCmd->allLceKernel1.bitfields.COEF_2_F1 = pData->LENRAllLceKernel[2][1];
        pRegCmd->allLceKernel1.bitfields.COEF_2_F2 = pData->LENRAllLceKernel[2][2];
        pRegCmd->allLceKernel1.bitfields.COEF_2_F3 = pData->LENRAllLceKernel[2][3];

        pRegCmd->allLceKernel1.bitfields.COEF_3_F0 = pData->LENRAllLceKernel[3][0];
        pRegCmd->allLceKernel1.bitfields.COEF_3_F1 = pData->LENRAllLceKernel[3][1];
        pRegCmd->allLceKernel1.bitfields.COEF_3_F2 = pData->LENRAllLceKernel[3][2];
        pRegCmd->allLceKernel1.bitfields.COEF_3_F3 = pData->LENRAllLceKernel[3][3];

        pRegCmd->allLceKernel2.bitfields.COEF_4_F0 = pData->LENRAllLceKernel[4][0];
        pRegCmd->allLceKernel2.bitfields.COEF_4_F1 = pData->LENRAllLceKernel[4][1];
        pRegCmd->allLceKernel2.bitfields.COEF_4_F2 = pData->LENRAllLceKernel[4][2];
        pRegCmd->allLceKernel2.bitfields.COEF_4_F3 = pData->LENRAllLceKernel[4][3];

        pRegCmd->allLceKernel2.bitfields.COEF_5_F0 = pData->LENRAllLceKernel[5][0];
        pRegCmd->allLceKernel2.bitfields.COEF_5_F1 = pData->LENRAllLceKernel[5][1];
        pRegCmd->allLceKernel2.bitfields.COEF_5_F2 = pData->LENRAllLceKernel[5][2];
        pRegCmd->allLceKernel2.bitfields.COEF_5_F3 = pData->LENRAllLceKernel[5][3];

        pRegCmd->allLceKernel3.bitfields.COEF_6_F0 = pData->LENRAllLceKernel[6][0];
        pRegCmd->allLceKernel3.bitfields.COEF_6_F1 = pData->LENRAllLceKernel[6][1];
        pRegCmd->allLceKernel3.bitfields.COEF_6_F2 = pData->LENRAllLceKernel[6][2];
        pRegCmd->allLceKernel3.bitfields.COEF_6_F3 = pData->LENRAllLceKernel[6][3];

        pRegCmd->allLceKernel3.bitfields.COEF_7_F0 = pData->LENRAllLceKernel[7][0];
        pRegCmd->allLceKernel3.bitfields.COEF_7_F1 = pData->LENRAllLceKernel[7][1];
        pRegCmd->allLceKernel3.bitfields.COEF_7_F2 = pData->LENRAllLceKernel[7][2];
        pRegCmd->allLceKernel3.bitfields.COEF_7_F3 = pData->LENRAllLceKernel[7][3];

        pRegCmd->allLceKernel4.bitfields.COEF_8_F0 = pData->LENRAllLceKernel[8][0];
        pRegCmd->allLceKernel4.bitfields.COEF_8_F1 = pData->LENRAllLceKernel[8][1];
        pRegCmd->allLceKernel4.bitfields.COEF_8_F2 = pData->LENRAllLceKernel[8][2];
        pRegCmd->allLceKernel4.bitfields.COEF_8_F3 = pData->LENRAllLceKernel[8][3];

        pRegCmd->allCmnSetting.bitfields.CNR_SCALE         = pData->cnrScale;
        pRegCmd->allCmnSetting.bitfields.FNR_SHIFT         = pData->LENRAllFnrShift;
        pRegCmd->allCmnSetting.bitfields.LCE_KERNEL_CENTER = pData->LENRAllLceKernelCenter;

        pRegCmd->bltrRnrAnchorBaseSetting0.bitfields.ANCHOR = pData->LENRAllBltrRnrAnchor[0];
        pRegCmd->bltrRnrAnchorBaseSetting0.bitfields.BASE   = pData->LENRAllBltrRnrBase[0];
        pRegCmd->bltrRnrAnchorBaseSetting1.bitfields.ANCHOR = pData->LENRAllBltrRnrAnchor[1];
        pRegCmd->bltrRnrAnchorBaseSetting1.bitfields.BASE   = pData->LENRAllBltrRnrBase[1];
        pRegCmd->bltrRnrAnchorBaseSetting2.bitfields.ANCHOR = pData->LENRAllBltrRnrAnchor[2];
        pRegCmd->bltrRnrAnchorBaseSetting2.bitfields.BASE   = pData->LENRAllBltrRnrBase[2];
        pRegCmd->bltrRnrAnchorBaseSetting3.bitfields.ANCHOR = pData->LENRAllBltrRnrAnchor[3];
        pRegCmd->bltrRnrAnchorBaseSetting3.bitfields.BASE   = pData->LENRAllBltrRnrBase[3];
        pRegCmd->bltrRnrAnchorBaseSetting4.bitfields.ANCHOR = pData->LENRAllBltrRnrAnchor[4];
        pRegCmd->bltrRnrAnchorBaseSetting4.bitfields.BASE   = pData->LENRAllBltrRnrBase[4];
        pRegCmd->bltrRnrAnchorBaseSetting5.bitfields.ANCHOR = pData->LENRAllBltrRnrAnchor[5];
        pRegCmd->bltrRnrAnchorBaseSetting5.bitfields.BASE   = pData->LENRAllBltrRnrBase[5];

        pRegCmd->bltrRnrSlopeShiftSetting0.bitfields.SHIFT = pData->LENRAllBltrRnrShift[0];
        pRegCmd->bltrRnrSlopeShiftSetting0.bitfields.SLOPE = pData->LENRAllBltrRnrSlope[0];
        pRegCmd->bltrRnrSlopeShiftSetting1.bitfields.SHIFT = pData->LENRAllBltrRnrShift[1];
        pRegCmd->bltrRnrSlopeShiftSetting1.bitfields.SLOPE = pData->LENRAllBltrRnrSlope[1];
        pRegCmd->bltrRnrSlopeShiftSetting2.bitfields.SHIFT = pData->LENRAllBltrRnrShift[2];
        pRegCmd->bltrRnrSlopeShiftSetting2.bitfields.SLOPE = pData->LENRAllBltrRnrSlope[2];
        pRegCmd->bltrRnrSlopeShiftSetting3.bitfields.SHIFT = pData->LENRAllBltrRnrShift[3];
        pRegCmd->bltrRnrSlopeShiftSetting3.bitfields.SLOPE = pData->LENRAllBltrRnrSlope[3];
        pRegCmd->bltrRnrSlopeShiftSetting4.bitfields.SHIFT = pData->LENRAllBltrRnrShift[4];
        pRegCmd->bltrRnrSlopeShiftSetting4.bitfields.SLOPE = pData->LENRAllBltrRnrSlope[4];
        pRegCmd->bltrRnrSlopeShiftSetting5.bitfields.SHIFT = pData->LENRAllBltrRnrShift[5];
        pRegCmd->bltrRnrSlopeShiftSetting5.bitfields.SLOPE = pData->LENRAllBltrRnrSlope[5];

        pRegCmd->lceRnrAnchorBaseSetting0.bitfields.ANCHOR = pData->LENRAllLceRnrAnchor[0];
        pRegCmd->lceRnrAnchorBaseSetting0.bitfields.BASE   = pData->LENRAllLceRnrBase[0];
        pRegCmd->lceRnrAnchorBaseSetting1.bitfields.ANCHOR = pData->LENRAllLceRnrAnchor[1];
        pRegCmd->lceRnrAnchorBaseSetting1.bitfields.BASE   = pData->LENRAllLceRnrBase[1];
        pRegCmd->lceRnrAnchorBaseSetting2.bitfields.ANCHOR = pData->LENRAllLceRnrAnchor[2];
        pRegCmd->lceRnrAnchorBaseSetting2.bitfields.BASE   = pData->LENRAllLceRnrBase[2];
        pRegCmd->lceRnrAnchorBaseSetting3.bitfields.ANCHOR = pData->LENRAllLceRnrAnchor[3];
        pRegCmd->lceRnrAnchorBaseSetting3.bitfields.BASE   = pData->LENRAllLceRnrBase[3];
        pRegCmd->lceRnrAnchorBaseSetting4.bitfields.ANCHOR = pData->LENRAllLceRnrAnchor[4];
        pRegCmd->lceRnrAnchorBaseSetting4.bitfields.BASE   = pData->LENRAllLceRnrBase[4];
        pRegCmd->lceRnrAnchorBaseSetting5.bitfields.ANCHOR = pData->LENRAllLceRnrAnchor[5];
        pRegCmd->lceRnrAnchorBaseSetting5.bitfields.BASE   = pData->LENRAllLceRnrBase[5];

        pRegCmd->lceRnrSlopeShiftSetting0.bitfields.SHIFT = pData->LENRAllLceRnrShift[0];
        pRegCmd->lceRnrSlopeShiftSetting0.bitfields.SLOPE = pData->LENRAllLceRnrSlope[0];
        pRegCmd->lceRnrSlopeShiftSetting1.bitfields.SHIFT = pData->LENRAllLceRnrShift[1];
        pRegCmd->lceRnrSlopeShiftSetting1.bitfields.SLOPE = pData->LENRAllLceRnrSlope[1];
        pRegCmd->lceRnrSlopeShiftSetting2.bitfields.SHIFT = pData->LENRAllLceRnrShift[2];
        pRegCmd->lceRnrSlopeShiftSetting2.bitfields.SLOPE = pData->LENRAllLceRnrSlope[2];
        pRegCmd->lceRnrSlopeShiftSetting3.bitfields.SHIFT = pData->LENRAllLceRnrShift[3];
        pRegCmd->lceRnrSlopeShiftSetting3.bitfields.SLOPE = pData->LENRAllLceRnrSlope[3];
        pRegCmd->lceRnrSlopeShiftSetting4.bitfields.SHIFT = pData->LENRAllLceRnrShift[4];
        pRegCmd->lceRnrSlopeShiftSetting4.bitfields.SLOPE = pData->LENRAllLceRnrSlope[4];
        pRegCmd->lceRnrSlopeShiftSetting5.bitfields.SHIFT = pData->LENRAllLceRnrShift[5];
        pRegCmd->lceRnrSlopeShiftSetting5.bitfields.SLOPE = pData->LENRAllLceRnrSlope[5];

        pRegCmd->dn4RnrInitHvOffset.bitfields.BX                  = pData->LENRDn4RnrBx;
        pRegCmd->dn4RnrInitHvOffset.bitfields.BY                  = pData->LENRDn4RnrBy;
        pRegCmd->dn4RnrRSquareInit.bitfields.R_SQUARE_INIT        = pData->LENRDn4RnrRSquareInit;
        pRegCmd->dn4RnrRSquareScaleShift.bitfields.R_SQUARE_SCALE = pData->LENRDn4RnrRSquareScale;
        pRegCmd->dn4RnrRSquareScaleShift.bitfields.R_SQUARE_SHIFT = pData->LENRDn4RnrRSquareShift;

        pRegCmd->dn8RnrInitHvOffset.bitfields.BX                  = pData->LENRDn8RnrBx;
        pRegCmd->dn8RnrInitHvOffset.bitfields.BY                  = pData->LENRDn8RnrBy;
        pRegCmd->dn8RnrRSquareInit.bitfields.R_SQUARE_INIT        = pData->LENRDn8RnrRSquareInit;
        pRegCmd->dn8RnrRSquareScaleShift.bitfields.R_SQUARE_SCALE = pData->LENRDn8RnrRSquareScale;
        pRegCmd->dn8RnrRSquareScaleShift.bitfields.R_SQUARE_SHIFT = pData->LENRDn8RnrRSquareShift;

        pRegCmd->dn16RnrInitHvOffset.bitfields.BX                  = pData->LENRDn16RnrBx;
        pRegCmd->dn16RnrInitHvOffset.bitfields.BY                  = pData->LENRDn16RnrBy;
        pRegCmd->dn16RnrRSquareInit.bitfields.R_SQUARE_INIT        = pData->LENRDn16RnrRSquareInit;
        pRegCmd->dn16RnrRSquareScaleShift.bitfields.R_SQUARE_SCALE = pData->LENRDn16RnrRSquareScale;
        pRegCmd->dn16RnrRSquareScaleShift.bitfields.R_SQUARE_SHIFT = pData->LENRDn16RnrRSquareShift;

        pRegCmd->snrCfg0.bitfields.H_MAX = pData->snrSkinHueMax;
        pRegCmd->snrCfg0.bitfields.H_MIN = pData->snrSkinHueMin;
        pRegCmd->snrCfg0.bitfields.Y_MIN = pData->snrSkinYMin;
        pRegCmd->snrCfg1.bitfields.Y_MAX = pData->snrSkinYMax;
        pRegCmd->snrCfg1.bitfields.Q_SNR = pData->snrQstepSkin;
        pRegCmd->snrCfg1.bitfields.BDRY_PROB = pData->snrBoundaryProbability;
        pRegCmd->snrCfg1.bitfields.Q_NON_SNR = pData->snrQstepNonskin;
        pRegCmd->snrCfg2.bitfields.MAX_SLOPE = pData->snrSatMaxSlope;
        pRegCmd->snrCfg2.bitfields.MIN_SLOPE = pData->snrSatMinSlope;
        pRegCmd->snrCfg2.bitfields.SAT_MAX   = pData->snrSkinYmaxSatMax;
        pRegCmd->snrCfg2.bitfields.SAT_MIN   = pData->snrSkinYmaxSatMin;

        pRegCmd->faceCfg.bitfields.FACE_NUM = pData->faceNum;
        pRegCmd->faceOffsetCfg.bitfields.FACE_HORIZONTAL_OFFSET = pData->faceHorizontalOffset;
        pRegCmd->faceOffsetCfg.bitfields.FACE_VERTICAL_OFFSET   = pData->faceVerticalOffset;

        pRegCmd->face1CenterCfg.bitfields.FACE_CENTER_HORIZONTAL = pData->faceCenterHorizontal[0];
        pRegCmd->face2CenterCfg.bitfields.FACE_CENTER_HORIZONTAL = pData->faceCenterHorizontal[1];
        pRegCmd->face3CenterCfg.bitfields.FACE_CENTER_HORIZONTAL = pData->faceCenterHorizontal[2];
        pRegCmd->face4CenterCfg.bitfields.FACE_CENTER_HORIZONTAL = pData->faceCenterHorizontal[3];
        pRegCmd->face5CenterCfg.bitfields.FACE_CENTER_HORIZONTAL = pData->faceCenterHorizontal[4];

        pRegCmd->face1CenterCfg.bitfields.FACE_CENTER_VERTICAL   = pData->faceCenterVertical[0];
        pRegCmd->face2CenterCfg.bitfields.FACE_CENTER_VERTICAL   = pData->faceCenterVertical[1];
        pRegCmd->face3CenterCfg.bitfields.FACE_CENTER_VERTICAL   = pData->faceCenterVertical[2];
        pRegCmd->face4CenterCfg.bitfields.FACE_CENTER_VERTICAL   = pData->faceCenterVertical[3];
        pRegCmd->face5CenterCfg.bitfields.FACE_CENTER_VERTICAL   = pData->faceCenterVertical[4];
        pRegCmd->face1RadiusCfg.bitfields.FACE_RADIUS_BOUNDARY = pData->faceRadiusBoundary[0];
        pRegCmd->face1RadiusCfg.bitfields.FACE_RADIUS_SHIFT    = pData->faceRadiusShift[0];
        pRegCmd->face1RadiusCfg.bitfields.FACE_RADIUS_SLOPE    = pData->faceRadiusSlope[0];
        pRegCmd->face1RadiusCfg.bitfields.FACE_SLOPE_SHIFT     = pData->faceSlopeShift[0];

        pRegCmd->face2RadiusCfg.bitfields.FACE_RADIUS_BOUNDARY = pData->faceRadiusBoundary[1];
        pRegCmd->face2RadiusCfg.bitfields.FACE_RADIUS_SHIFT    = pData->faceRadiusShift[1];
        pRegCmd->face2RadiusCfg.bitfields.FACE_RADIUS_SLOPE    = pData->faceRadiusSlope[1];
        pRegCmd->face2RadiusCfg.bitfields.FACE_SLOPE_SHIFT     = pData->faceSlopeShift[1];

        pRegCmd->face3RadiusCfg.bitfields.FACE_RADIUS_BOUNDARY = pData->faceRadiusBoundary[2];
        pRegCmd->face3RadiusCfg.bitfields.FACE_RADIUS_SHIFT    = pData->faceRadiusShift[2];
        pRegCmd->face3RadiusCfg.bitfields.FACE_RADIUS_SLOPE    = pData->faceRadiusSlope[2];
        pRegCmd->face3RadiusCfg.bitfields.FACE_SLOPE_SHIFT     = pData->faceSlopeShift[2];

        pRegCmd->face4RadiusCfg.bitfields.FACE_RADIUS_BOUNDARY = pData->faceRadiusBoundary[3];
        pRegCmd->face4RadiusCfg.bitfields.FACE_RADIUS_SHIFT    = pData->faceRadiusShift[3];
        pRegCmd->face4RadiusCfg.bitfields.FACE_RADIUS_SLOPE    = pData->faceRadiusSlope[3];
        pRegCmd->face4RadiusCfg.bitfields.FACE_SLOPE_SHIFT     = pData->faceSlopeShift[3];

        pRegCmd->face5RadiusCfg.bitfields.FACE_RADIUS_BOUNDARY = pData->faceRadiusBoundary[4];
        pRegCmd->face5RadiusCfg.bitfields.FACE_RADIUS_SHIFT    = pData->faceRadiusShift[4];
        pRegCmd->face5RadiusCfg.bitfields.FACE_RADIUS_SLOPE    = pData->faceRadiusSlope[4];
        pRegCmd->face5RadiusCfg.bitfields.FACE_SLOPE_SHIFT     = pData->faceSlopeShift[4];

        pRegCmd->us4InitPhase.bitfields.PHH = pData->us4InitPhH;
        pRegCmd->us4InitPhase.bitfields.PHV = pData->us4InitPhV;

        pRegCmd->dn4WeightCurve.bitfields.GAP  = pData->LENRDn4BltrGap;
        pRegCmd->dn4WeightCurve.bitfields.TH   = pData->LENRDn4BltrTh;

        pRegCmd->dn8WeightCurve.bitfields.GAP  = pData->LENRDn8BltrGap;
        pRegCmd->dn8WeightCurve.bitfields.TH   = pData->LENRDn8BltrTh;

        pRegCmd->dn16WeightCurve.bitfields.GAP = pData->LENRDn16BltrGap;
        pRegCmd->dn16WeightCurve.bitfields.TH  = pData->LENRDn16BltrTh;

        pRegCmd->dn4Clamp.bitfields.NEG  = pData->LENRDn4BltrClampN;
        pRegCmd->dn4Clamp.bitfields.POS  = pData->LENRDn4BltrClampP;
        pRegCmd->dn8Clamp.bitfields.NEG  = pData->LENRDn8BltrClampN;
        pRegCmd->dn8Clamp.bitfields.POS  = pData->LENRDn8BltrClampP;
        pRegCmd->dn16Clamp.bitfields.NEG = pData->LENRDn16BltrClampN;
        pRegCmd->dn16Clamp.bitfields.POS = pData->LENRDn16BltrClampP;

        // CHECK
        // pRegCmd->feCrop.bitfields.ALIGN_EN = pData->lenr_blend_en;
        pRegCmd->feCrop.bitfields.LEFT   = pData->cropStripeToImageBoundaryDistLeft;
        pRegCmd->feCrop.bitfields.RIGHT  = pData->cropFrameBoundaryPixAlignRight;

        pRegCmd->beCropH.bitfields.LEFT  = pData->postCropFirstPixel;
        pRegCmd->beCropH.bitfields.RIGHT = pData->postCropLastPixel;
        pRegCmd->beCropV.bitfields.UP    = pData->postCropFirstLine;
        pRegCmd->beCropV.bitfields.DOWN  = pData->postCropLastLine;


        pLENRParameters->moduleCfg.DN4_BLTR_CLAMP_EN  = pData->LENRDn4BltrClampEn;
        pLENRParameters->moduleCfg.DN8_BLTR_CLAMP_EN  = pData->LENRDn8BltrClampEn;
        pLENRParameters->moduleCfg.DN16_BLTR_CLAMP_EN = pData->LENRDn16BltrClampEn;
        pLENRParameters->moduleCfg.BLTR_EN            = pData->LENRBltrEn;
        pLENRParameters->moduleCfg.EN                 = pData->enable;
        pLENRParameters->moduleCfg.FD_SNR_EN          = pData->fdSnrEn;
        pLENRParameters->moduleCfg.FNR_EN             = pData->fnrEn;
        pLENRParameters->moduleCfg.LAYER_1_ONLY       = pData->LENRBltrLayer1Only;
        pLENRParameters->moduleCfg.LCE_EN             = pData->LENRLceEn;
        pLENRParameters->moduleCfg.RNR_EN             = pData->rnrEn;
        pLENRParameters->moduleCfg.SNR_EN             = pData->snrEn;
        pLENRParameters->moduleCfg.POST_CROP_EN       = pData->postCropEnable;
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "4clamp %d, 8clamp %d, 16 clamp %d, BLTR_EN %d, EN %d, FD_SNR_EN %d"
                         "FNR_EN %d, LAYER_1_ONLY %d, LCE_EN %d, RNR_EN %d, SNR_EN %d, POST_CROP_EN %d",
                         pLENRParameters->moduleCfg.DN4_BLTR_CLAMP_EN,
                         pLENRParameters->moduleCfg.DN8_BLTR_CLAMP_EN,
                         pLENRParameters->moduleCfg.DN16_BLTR_CLAMP_EN,
                         pLENRParameters->moduleCfg.BLTR_EN,
                         pLENRParameters->moduleCfg.EN,
                         pLENRParameters->moduleCfg.FD_SNR_EN,
                         pLENRParameters->moduleCfg.FNR_EN,
                         pLENRParameters->moduleCfg.LAYER_1_ONLY,
                         pLENRParameters->moduleCfg.LCE_EN,
                         pLENRParameters->moduleCfg.RNR_EN,
                         pLENRParameters->moduleCfg.SNR_EN ,
                         pLENRParameters->moduleCfg.POST_CROP_EN);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELENR10Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_ERROR(CamxLogGroupPProc, "%s Not implemented ", __FUNCTION__);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10Titan480::~IPELENR10Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPELENR10Titan480::~IPELENR10Titan480()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPELENR10Titan480::DumpRegConfig()
{
    if (NULL != m_pRegCmd)
    {
        IPELENR10RegConfig* pRegCmd = static_cast<IPELENR10RegConfig*>(m_pRegCmd);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "******** IPE LENR10 [HEX] ********\n");

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrNzCtrl             = %x\n", pRegCmd->dn4BltrNzCtrl);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrFnrGain0           = %x\n", pRegCmd->dn4BltrFnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrFnrGain1           = %x\n", pRegCmd->dn4BltrFnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrFnrGain2           = %x\n", pRegCmd->dn4BltrFnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrFnrGain3           = %x\n", pRegCmd->dn4BltrFnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrFnrGain4           = %x\n", pRegCmd->dn4BltrFnrGain4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrFnrGain5           = %x\n", pRegCmd->dn4BltrFnrGain5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrFnrGain6           = %x\n", pRegCmd->dn4BltrFnrGain6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrFnrGain7           = %x\n", pRegCmd->dn4BltrFnrGain7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrFnrGain8           = %x\n", pRegCmd->dn4BltrFnrGain8);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrSnrGain0           = %x\n", pRegCmd->dn4BltrSnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrSnrGain1           = %x\n", pRegCmd->dn4BltrSnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrSnrGain2           = %x\n", pRegCmd->dn4BltrSnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4BltrSnrGain3           = %x\n", pRegCmd->dn4BltrSnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrNzCtrl             = %x\n", pRegCmd->dn8BltrNzCtrl);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrFnrGain0           = %x\n", pRegCmd->dn8BltrFnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrFnrGain1           = %x\n", pRegCmd->dn8BltrFnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrFnrGain2           = %x\n", pRegCmd->dn8BltrFnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrFnrGain3           = %x\n", pRegCmd->dn8BltrFnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrFnrGain4           = %x\n", pRegCmd->dn8BltrFnrGain4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrFnrGain5           = %x\n", pRegCmd->dn8BltrFnrGain5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrFnrGain6           = %x\n", pRegCmd->dn8BltrFnrGain6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrFnrGain7           = %x\n", pRegCmd->dn8BltrFnrGain7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrFnrGain8           = %x\n", pRegCmd->dn8BltrFnrGain8);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrSnrGain0           = %x\n", pRegCmd->dn8BltrSnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrSnrGain1           = %x\n", pRegCmd->dn8BltrSnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrSnrGain2           = %x\n", pRegCmd->dn8BltrSnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8BltrSnrGain3           = %x\n", pRegCmd->dn8BltrSnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrNzCtrl            = %x\n", pRegCmd->dn16BltrNzCtrl);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrFnrGain0          = %x\n", pRegCmd->dn16BltrFnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrFnrGain1          = %x\n", pRegCmd->dn16BltrFnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrFnrGain2          = %x\n", pRegCmd->dn16BltrFnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrFnrGain3          = %x\n", pRegCmd->dn16BltrFnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrFnrGain4          = %x\n", pRegCmd->dn16BltrFnrGain4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrFnrGain5          = %x\n", pRegCmd->dn16BltrFnrGain5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrFnrGain6          = %x\n", pRegCmd->dn16BltrFnrGain6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrFnrGain7          = %x\n", pRegCmd->dn16BltrFnrGain7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrFnrGain8          = %x\n", pRegCmd->dn16BltrFnrGain8);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrSnrGain0          = %x\n", pRegCmd->dn16BltrSnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrSnrGain1          = %x\n", pRegCmd->dn16BltrSnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrSnrGain2          = %x\n", pRegCmd->dn16BltrSnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16BltrSnrGain3          = %x\n", pRegCmd->dn16BltrSnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceHpfPos              = %x\n", pRegCmd->dn4LceHpfPos);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceHpfNeg              = %x\n", pRegCmd->dn4LceHpfNeg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceFnrGain0            = %x\n", pRegCmd->dn4LceFnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceFnrGain1            = %x\n", pRegCmd->dn4LceFnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceFnrGain2            = %x\n", pRegCmd->dn4LceFnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceFnrGain3            = %x\n", pRegCmd->dn4LceFnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceFnrGain4            = %x\n", pRegCmd->dn4LceFnrGain4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceFnrGain5            = %x\n", pRegCmd->dn4LceFnrGain5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceFnrGain6            = %x\n", pRegCmd->dn4LceFnrGain6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceFnrGain7            = %x\n", pRegCmd->dn4LceFnrGain7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceFnrGain8            = %x\n", pRegCmd->dn4LceFnrGain8);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceSnrGain0            = %x\n", pRegCmd->dn4LceSnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceSnrGain1            = %x\n", pRegCmd->dn4LceSnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceSnrGain2            = %x\n", pRegCmd->dn4LceSnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4LceSnrGain3            = %x\n", pRegCmd->dn4LceSnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceHpfPos              = %x\n", pRegCmd->dn8LceHpfPos);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceHpfNeg              = %x\n", pRegCmd->dn8LceHpfNeg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceFnrGain0            = %x\n", pRegCmd->dn8LceFnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceFnrGain1            = %x\n", pRegCmd->dn8LceFnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceFnrGain2            = %x\n", pRegCmd->dn8LceFnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceFnrGain3            = %x\n", pRegCmd->dn8LceFnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceFnrGain4            = %x\n", pRegCmd->dn8LceFnrGain4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceFnrGain5            = %x\n", pRegCmd->dn8LceFnrGain5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceFnrGain6            = %x\n", pRegCmd->dn8LceFnrGain6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceFnrGain7            = %x\n", pRegCmd->dn8LceFnrGain7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceFnrGain8            = %x\n", pRegCmd->dn8LceFnrGain8);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceSnrGain0            = %x\n", pRegCmd->dn8LceSnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceSnrGain1            = %x\n", pRegCmd->dn8LceSnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceSnrGain2            = %x\n", pRegCmd->dn8LceSnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8LceSnrGain3            = %x\n", pRegCmd->dn8LceSnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceHpfPos             = %x\n", pRegCmd->dn16LceHpfPos);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceHpfNeg             = %x\n", pRegCmd->dn16LceHpfNeg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceFnrGain0           = %x\n", pRegCmd->dn16LceFnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceFnrGain1           = %x\n", pRegCmd->dn16LceFnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceFnrGain2           = %x\n", pRegCmd->dn16LceFnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceFnrGain3           = %x\n", pRegCmd->dn16LceFnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceFnrGain4           = %x\n", pRegCmd->dn16LceFnrGain4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceFnrGain5           = %x\n", pRegCmd->dn16LceFnrGain5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceFnrGain6           = %x\n", pRegCmd->dn16LceFnrGain6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceFnrGain7           = %x\n", pRegCmd->dn16LceFnrGain7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceFnrGain8           = %x\n", pRegCmd->dn16LceFnrGain8);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceSnrGain0           = %x\n", pRegCmd->dn16LceSnrGain0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceSnrGain1           = %x\n", pRegCmd->dn16LceSnrGain1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceSnrGain2           = %x\n", pRegCmd->dn16LceSnrGain2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16LceSnrGain3           = %x\n", pRegCmd->dn16LceSnrGain3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "allLceKernel0             = %x\n", pRegCmd->allLceKernel0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "allLceKernel1             = %x\n", pRegCmd->allLceKernel1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "allLceKernel2             = %x\n", pRegCmd->allLceKernel2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "allLceKernel3             = %x\n", pRegCmd->allLceKernel3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "allLceKernel4             = %x\n", pRegCmd->allLceKernel4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "allCmnSetting             = %x\n", pRegCmd->allCmnSetting);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrAnchorBaseSetting0 = %x\n", pRegCmd->bltrRnrAnchorBaseSetting0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrSlopeShiftSetting0 = %x\n", pRegCmd->bltrRnrSlopeShiftSetting0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrAnchorBaseSetting1 = %x\n", pRegCmd->bltrRnrAnchorBaseSetting1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrSlopeShiftSetting1 = %x\n", pRegCmd->bltrRnrSlopeShiftSetting1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrAnchorBaseSetting2 = %x\n", pRegCmd->bltrRnrAnchorBaseSetting2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrSlopeShiftSetting2 = %x\n", pRegCmd->bltrRnrSlopeShiftSetting2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrAnchorBaseSetting3 = %x\n", pRegCmd->bltrRnrAnchorBaseSetting3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrSlopeShiftSetting3 = %x\n", pRegCmd->bltrRnrSlopeShiftSetting3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrAnchorBaseSetting4 = %x\n", pRegCmd->bltrRnrAnchorBaseSetting4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrSlopeShiftSetting4 = %x\n", pRegCmd->bltrRnrSlopeShiftSetting4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrAnchorBaseSetting5 = %x\n", pRegCmd->bltrRnrAnchorBaseSetting5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "bltrRnrSlopeShiftSetting5 = %x\n", pRegCmd->bltrRnrSlopeShiftSetting5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrAnchorBaseSetting0  = %x\n", pRegCmd->lceRnrAnchorBaseSetting0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrSlopeShiftSetting0  = %x\n", pRegCmd->lceRnrSlopeShiftSetting0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrAnchorBaseSetting1  = %x\n", pRegCmd->lceRnrAnchorBaseSetting1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrSlopeShiftSetting1  = %x\n", pRegCmd->lceRnrSlopeShiftSetting1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrAnchorBaseSetting2  = %x\n", pRegCmd->lceRnrAnchorBaseSetting2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrSlopeShiftSetting2  = %x\n", pRegCmd->lceRnrSlopeShiftSetting2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrAnchorBaseSetting3  = %x\n", pRegCmd->lceRnrAnchorBaseSetting3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrSlopeShiftSetting3  = %x\n", pRegCmd->lceRnrSlopeShiftSetting3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrAnchorBaseSetting4  = %x\n", pRegCmd->lceRnrAnchorBaseSetting4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrSlopeShiftSetting4  = %x\n", pRegCmd->lceRnrSlopeShiftSetting4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrAnchorBaseSetting5  = %x\n", pRegCmd->lceRnrAnchorBaseSetting5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lceRnrSlopeShiftSetting5  = %x\n", pRegCmd->lceRnrSlopeShiftSetting5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4RnrInitHvOffset        = %x\n", pRegCmd->dn4RnrInitHvOffset);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4RnrRSquareInit         = %x\n", pRegCmd->dn4RnrRSquareInit);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4RnrRSquareScaleShift   = %x\n", pRegCmd->dn4RnrRSquareScaleShift);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8RnrInitHvOffset        = %x\n", pRegCmd->dn8RnrInitHvOffset);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8RnrRSquareInit         = %x\n", pRegCmd->dn8RnrRSquareInit);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8RnrRSquareScaleShift   = %x\n", pRegCmd->dn8RnrRSquareScaleShift);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16RnrInitHvOffset       = %x\n", pRegCmd->dn16RnrInitHvOffset);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16RnrRSquareInit        = %x\n", pRegCmd->dn16RnrRSquareInit);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16RnrRSquareScaleShift  = %x\n", pRegCmd->dn16RnrRSquareScaleShift);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "snrCfg0                   = %x\n", pRegCmd->snrCfg0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "snrCfg1                   = %x\n", pRegCmd->snrCfg1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "snrCfg2                   = %x\n", pRegCmd->snrCfg2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "faceCfg                   = %x\n", pRegCmd->faceCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "faceOffsetCfg             = %x\n", pRegCmd->faceOffsetCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "face1CenterCfg            = %x\n", pRegCmd->face1CenterCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "face1RadiusCfg            = %x\n", pRegCmd->face1RadiusCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "face2CenterCfg            = %x\n", pRegCmd->face2CenterCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "face2RadiusCfg            = %x\n", pRegCmd->face2RadiusCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "face3CenterCfg            = %x\n", pRegCmd->face3CenterCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "face3RadiusCfg            = %x\n", pRegCmd->face3RadiusCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "face4CenterCfg            = %x\n", pRegCmd->face4CenterCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "face4RadiusCfg            = %x\n", pRegCmd->face4RadiusCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "face5CenterCfg            = %x\n", pRegCmd->face5CenterCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "face5RadiusCfg            = %x\n", pRegCmd->face5RadiusCfg);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "us4InitPhase              = %x\n", pRegCmd->us4InitPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4WeightCurve            = %x\n", pRegCmd->dn4WeightCurve);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn4Clamp                  = %x\n", pRegCmd->dn4Clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8WeightCurve            = %x\n", pRegCmd->dn8WeightCurve);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn8Clamp                  = %x\n", pRegCmd->dn8Clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16WeightCurve           = %x\n", pRegCmd->dn16WeightCurve);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "dn16Clamp                 = %x\n", pRegCmd->dn16Clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "feCrop                    = %x\n", pRegCmd->feCrop);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "beCropH                   = %x\n", pRegCmd->beCropH);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "beCropV                   = %x\n", pRegCmd->beCropV);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELENR10Titan480::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPELENR10Titan480::GetRegSize()
{
    return sizeof(IPELENR10RegConfig);
}


CAMX_NAMESPACE_END