// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  lenr10setting.h
/// @brief LENR10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LENR10SETTING_H
#define LENR10SETTING_H

#include "lenr_1_0_0.h"
#include "iqcommondefs.h"
#include "iqsettingutil.h"

// LENR global parameters and size constants
static const INT32 LENR_V10_FNR_ARR_NUM            = 17;
static const INT32 LENR_V10_FNR_ARR_INTVAL         = 8;
static const INT32 LENR_V10_FNR_AC_ARR_NUM         = 17;
static const INT32 LENR_V10_FNR_AC_ARR_INTVAL      = 16;
static const INT32 LENR_V10_RNR_ARR_NUM            = 6;
static const INT32 LENR_V10_MAX_FACE_NUM           = 5;
static const INT32 LENR_V10_SNR_ARR_NUM            = 17;
static const INT32 LENR_V10_LCE_KERNEL_ROW         = 9;
static const INT32 LENR_V10_LCE_KERNEL_COL         = 4;

// Const Min Max values for LENR10
static const INT32 LENR10_LNR_GAIN_LUT_QFACTOR    = 5;
static const INT32 LENR10_LNR_GAIN_LUT_MIN        = 0;
static const INT32 LENR10_LNR_GAIN_LUT_MAX        = 63;

static const INT32 LENR10_LNR_SHIFT_MIN           = 0;
static const INT32 LENR10_LNR_SHIFT_MAX           = 3;

static const INT32 LENR10_CNR_GAIN_LUT_QFACTOR    = 5;
static const INT32 LENR10_CNR_GAIN_LUT_MIN        = 0;
static const INT32 LENR10_CNR_GAIN_LUT_MAX        = 63;

static const INT32 LENR10_CNR_LOW_THRD_U_MIN      = -128;
static const INT32 LENR10_CNR_LOW_THRD_U_MAX      = 127;
static const INT32 LENR10_CNR_THRD_GAP_U_MIN      = 0;
static const INT32 LENR10_CNR_THRD_GAP_U_MAX      = 7;
static const INT32 LENR10_CNR_LOW_THRD_V_MIN      = -128;
static const INT32 LENR10_CNR_LOW_THRD_V_MAX      = 127;
static const INT32 LENR10_CNR_THRD_GAP_V_MIN      = 0;
static const INT32 LENR10_CNR_THRD_GAP_V_MAX      = 7;
static const INT32 LENR10_CNR_ADJ_GAIN_MIN        = 0;
static const INT32 LENR10_CNR_ADJ_GAIN_MAX        = 63;
static const INT32 LENR10_CNR_SCALE_MIN           = 0;
static const INT32 LENR10_CNR_SCALE_MAX           = 15;

static const INT32  LENR10_FNR_ACTHR_LUT_MIN      = 0;
static const INT32  LENR10_FNR_ACTHR_LUT_MAX      = 63;
static const INT32  LENR10_FNR_GAINARR_MIN        = 0;
static const INT32  LENR10_FNR_GAINARR_MAX        = 63;
static const UINT32 LENR10_FNR_GAINCLAMPARR_MIN   = 0;
static const UINT32 LENR10_FNR_GAINCLAMPARR_MAX   = 255;
static const INT32  LENR10_FNR_GAIN_SHIFT_IN_ARR  = 16;
static const INT32  LENR10_FNR_AC_SHFT_MIN        = 0;
static const INT32  LENR10_FNR_AC_SHFT_MAX        = 3;
static const INT32  LENR10_FNR_GAINARR_LUT_MIN    = 0;
static const INT32  LENR10_FNR_GAINARR_LUT_MAX    = 63;
static const INT32  LENR10_ABS_AMP_SHFT_MIN       = 0;
static const INT32  LENR10_ABS_AMP_SHFT_MAX       = 3;

static const INT32 LENR10_LPF3_PERCENT_MIN        = 0;
static const INT32 LENR10_LPF3_PERCENT_MAX        = 255;
static const INT32 LENR10_LPF3_OFFSET_MIN         = 0;
static const INT32 LENR10_LPF3_OFFSET_MAX         = 255;
static const INT32 LENR10_LPF3_STRENGTH_MIN       = 0;
static const INT32 LENR10_LPF3_STRENGTH_MAX       = 5;

static const INT32 LENR10_BLEND_CNR_ADJ_GAIN_MIN  = 0;
static const INT32 LENR10_BLEND_CNR_ADJ_GAIN_MAX  = 63;
static const INT32 LENR10_BLEND_SNR_GAINARR_MIN   = 0;
static const INT32 LENR10_BLEND_SNR_GAINARR_MAX   = 31;
static const INT32 LENR10_BLEND_LNR_GAINARR_MIN   = 0;
static const INT32 LENR10_BLEND_LNR_GAINARR_MAX   = 255;

static const INT16  LENR10_RNR_GAIN_ARR_MAX       = 1023;
static const INT16  LENR10_RNR_GAIN_ARR_MIN       = -1023;
static const INT16  LENR10_RNR_THRD_ARR_MIN       = 0;
static const INT16  LENR10_RNR_THRD_ARR_MAX       = static_cast<INT16>(IQSettingUtils::MAXINT32BITFIELD(11));
static const INT32  LENR10_RNR_BX_MIN             = IQSettingUtils::MININT32BITFIELD(14);
static const INT32  LENR10_RNR_BX_MAX             = IQSettingUtils::MAXINT32BITFIELD(14);
static const INT32  LENR10_RNR_BY_MIN             = IQSettingUtils::MININT32BITFIELD(14);
static const INT32  LENR10_RNR_BY_MAX             = IQSettingUtils::MAXINT32BITFIELD(14);
static const UINT32 LENR10_RNR_R_SQUARE_INIT_MIN  = IQSettingUtils::MINUINTBITFIELD(28);
static const UINT32 LENR10_RNR_R_SQUARE_INIT_MAX  = IQSettingUtils::MAXUINTBITFIELD(28);
static const UINT32 LENR10_RNR_MIN_SHIFT          = 0;
static const UINT32 LENR10_RNR_MAX_SHIFT          = 15;
static const UINT32 LENR10_RNR_MIN_R_SQUARE_SCALE = 0;
static const UINT32 LENR10_RNR_MAX_R_SQUARE_SCALE = 127;
static const UINT32 LENR10_RNR_MIN_ANCHOR         = 0;
static const UINT32 LENR10_RNR_MAX_ANCHOR         = 1023;
static const UINT32 LENR10_RNR_MIN_BASE           = 0;
static const UINT32 LENR10_RNR_MAX_BASE           = 1023;
static const UINT32 LENR10_SKIN_HUE_MIN_MIN       = 0;
static const UINT32 LENR10_SKIN_HUE_MIN_MAX       = 1023;
static const UINT32 LENR10_SKIN_HUE_MAX_MIN       = 0;
static const UINT32 LENR10_SKIN_HUE_MAX_MAX       = 255;
static const UINT32 LENR10_SNR_QSTEP_MIN          = 0;
static const UINT32 LENR10_SNR_QSTEP_MAX          = 128;
static const UINT32 LENR10_SNR_GAIN_MIN           = 1;
static const UINT32 LENR10_SNR_GAIN_MAX           = 32;
static const INT    LENR_Q_BITS_NUMBER            = 8;
static const INT16  LENR10_R_SQUARE_SHFT_MAX      = 16;
static const INT16  LENR10_MAX_RSQUARE_VAL        = 8191;
static const FLOAT  LENR10_MIN_FACE_BOUNDARY      = 0.0f;
static const FLOAT  LENR10_MAX_FACE_BOUNDARY      = 8.0f;
static const FLOAT  LENR10_MIN_FACE_TRANSITION    = 0.0f;
static const FLOAT  LENR10_MAX_FACE_TRANSITION    = 8.0f;

/// @brief Unpacked LENR10 output from common IQ library
// NOWHINE NC004c: Share code with system team
struct LENR10UnpackedField
{
    INT16 enable;                       ///< Enable bit 1u
    INT16 LENRBltrEn;                   ///< LENR BLTR EN 1u
    INT16 LENRLceEn;                    ///< LENR LCE EN 1u
    INT16 LENRBltrLayer1Only;           ///< LENR BLTR EN layer 1 1u
    INT16 LENRBlendEn;                  ///< LENR Blend EN 1u
    INT16 rnrEn;                        ///< RNR EN 1u
    INT16 snrEn;                        ///< SNR EN 1u
    INT16 fnrEn;                        ///< FNR EN 1u
    INT16 fdSnrEn;                      ///< FD SNR EN 1u

    INT16 LENRDn4BltrTh;                ///< lenrDn4BltrTh
    INT16 LENRDn4BltrGap;               ///< lenrDn4BltrGap
    INT16 LENRDn4BltrCtrlTh;            ///< LENRDn4BltrCtrlTh
    INT16 LENRDn4BltrCtrlW;             ///< LENRDn4BltrCtrlW
    INT16 LENRDn4BltrClampEn;           ///< LENRDn4BltrClampEn

    UINT16 LENRDn4BltrFnrGainArr[LENR_V10_FNR_AC_ARR_NUM];    ///< LENRDn4BltrFnrGain Array
    UINT16 LENRDn4BltrSnrGainArr[LENR_V10_SNR_ARR_NUM];       ///< LENRDn4BltrSnrGain Array
    INT16 LENRDn4BltrClampP;                                  ///< LENRDn4BltrClampP
    INT16 LENRDn4BltrClampN;                                  ///< LENRDn4BltrClampN

    INT16 LENRDn8BltrTh;                                      ///< LENRDn8BltrTh
    INT16 LENRDn8BltrGap;                                     ///< LENRDn8BltrGap
    INT16 LENRDn8BltrCtrlTh;                                  ///< LENRDn8BltrCtrlTh
    INT16 LENRDn8BltrCtrlW;                                   ///< LENRDn8BltrCtrlW
    INT16 LENRDn8BltrClampEn;                                 ///< LENRDn8BltrClampEn
    UINT16 LENRDn8BltrFnrGainArr[LENR_V10_FNR_AC_ARR_NUM];    ///< LENRDn8BltrFnrGain Array
    UINT16 LENRDn8BltrSnrGainArr[LENR_V10_SNR_ARR_NUM];       ///< LENRDn8BltrSnrGainArr
    INT16 LENRDn8BltrClampP;                                  ///< LENRDn8BltrClampP
    INT16 LENRDn8BltrClampN;                                  ///< LENRDn8BltrClampN

    INT16 LENRDn16BltrTh;                                     ///< LENRDn16BltrTh
    INT16 LENRDn16BltrGap;                                    ///< LENRDn16BltrGap
    INT16 LENRDn16BltrCtrlTh;                                 ///< LENRDn16BltrCtrlTh
    INT16 LENRDn16BltrCtrlW;                                  ///< LENRDn16BltrCtrlW
    INT16 LENRDn16BltrClampEn;                                ///< LENRDn16BltrClampEn
    UINT16 LENRDn16BltrFnrGainArr[LENR_V10_FNR_AC_ARR_NUM];   ///< LENRDn16BltrFnrGain Array
    UINT16 LENRDn16BltrSnrGainArr[LENR_V10_SNR_ARR_NUM];      ///< LENRDn16BltrSnrGain Array
    INT16 LENRDn16BltrClampP;                                 ///< LENRDn16BltrClampP
    INT16 LENRDn16BltrClampN;                                 ///< LENRDn16BltrClampN

    UINT16 LENRAllBltrRnrAnchor[LENR_V10_RNR_ARR_NUM];        ///< LENRAllBltrRnrAnchor Array 10u
    UINT16 LENRAllBltrRnrBase[LENR_V10_RNR_ARR_NUM];          ///< LENRAllBltrRnrBase Array 10u, 6 anchors
    INT16  LENRAllBltrRnrSlope[LENR_V10_RNR_ARR_NUM];         ///< LENRAllBltrRnrSlope Array 11s
    UINT16 LENRAllBltrRnrShift[LENR_V10_RNR_ARR_NUM];         ///< LENRAllBltrRnrShift Array 4u

    INT16 LENRDn4LceCoreP;                                    ///< LENRDn4LceCoreP
    INT16 LENRDn4LceCoreN;                                    ///< LENRDn4LceCoreN
    INT16 LENRDn4LceScaleP;                                   ///< LENRDn4LceScaleP
    INT16 LENRDn4LceScaleN;                                   ///< LENRDn4LceScaleN
    INT16 LENRDn4LceClampP;                                   ///< LENRDn4LceClampP
    INT16 LENRDn4LceClampN;                                   ///< LENRDn4LceClampN
    UINT16 LENRDn4LceFnrGainArr[LENR_V10_FNR_AC_ARR_NUM];     ///< LENRDdn4LceFnrGain Array
    UINT16 LENRDn4LceSnrGainArr[LENR_V10_SNR_ARR_NUM];        ///< LENRDn4LceSnrGain Array

    INT16 LENRDn8LceCoreP;                                    ///< LENRDn8LceCoreP
    INT16 LENRDn8LceCoreN;                                    ///< LENRDn8LceCoreN
    INT16 LENRDn8LceScaleP;                                   ///< LENRDn8LceScaleP
    INT16 LENRDn8LceScaleN;                                   ///< LENRDn8LceScaleN
    INT16 LENRDn8LceClampP;                                   ///< LENRDn8LceClampP
    INT16 LENRDn8LceClampN;                                   ///< LENRDn8LceClampN
    UINT16 LENRDn8LceFnrGainArr[LENR_V10_FNR_AC_ARR_NUM];     ///< LENRDn8LceFnrGain Array
    UINT16 LENRDn8LceSnrGainArr[LENR_V10_SNR_ARR_NUM];        ///< LENRDn8LceSnrGain Array

    INT16 LENRDn16LceCoreP;                                   ///< LENRDn16LceCoreP
    INT16 LENRDn16LceCoreN;                                   ///< LENRDn16LceCoreN
    INT16 LENRDn16LceScaleP;                                  ///< LENRDn16LceScaleP
    INT16 LENRDn16LceScaleN;                                  ///< LENRDn16LceScaleN
    INT16 LENRDn16LceClampP;                                  ///< LENRDn16LceClampP
    INT16 LENRDn16LceClampN;                                  ///< LENRDn16LceClampN
    UINT16 LENRDn16LceFnrGainArr[LENR_V10_FNR_AC_ARR_NUM];    ///< LENRDn16LceFnrGain Array
    UINT16 LENRDn16LceSnrGainArr[LENR_V10_SNR_ARR_NUM];       ///< LENRDn16LceSnrGain Array

    UINT16 LENRAllLceRnrAnchor[LENR_V10_RNR_ARR_NUM];         ///< LENRAllLceRnrAnchor array 10u
    UINT16 LENRAllLceRnrBase[LENR_V10_RNR_ARR_NUM];           ///< LENRAllLceRnrBase Array 10u, 6 anchors
    INT16  LENRAllLceRnrSlope[LENR_V10_RNR_ARR_NUM];          ///< LENRAllLceRnrSlope Array 11s
    UINT16 LENRAllLceRnrShift[LENR_V10_RNR_ARR_NUM];          ///< LENRAllLceRnrShift Array 4u

    INT16 LENRAllLceKernel[LENR_V10_LCE_KERNEL_ROW][LENR_V10_LCE_KERNEL_COL];    ///<  LENRAllLceKernel Array
    INT16 LENRAllLceKernelCenter;                             ///< LENRAllLceKernelCenter

    // BLTR/LCE shared
    INT16 LENRAllFnrShift;                                    ///< LENRAllFnrShift

    INT16  LENRDn4RnrBx;                                     ///<  LENRDn4RnrBx 14s, init_h_offset-h_center
    INT16  LENRDn4RnrBy;                                     ///<  LENRDn4RnrBy 14s, init_v_offset-v_center
    UINT32 LENRDn4RnrRSquareInit;                            ///<  LENRDn4RnrRSquareInit 28u,
                                                             ///  (init_h_offset-h_center)^2 + (init_v_offset-v_center)^2
    UINT16 LENRDn4RnrRSquareScale;                           ///<  LENRDn4RnrRSquareScale 7u; range[0, 127]
    UINT16 LENRDn4RnrRSquareShift;                           ///<  LENRDn4RnrRSquareShift 4u  range[0, 15]

    INT16  LENRDn8RnrBx;                                     ///< LENRDn8RnrBx 14s, init_h_offset-h_center
    INT16  LENRDn8RnrBy;                                     ///< LENRDn8RnrBy 14s, init_v_offset-v_center
    UINT32 LENRDn8RnrRSquareInit;                            ///< LENRDn8RnrRSquareInit 28u,
                                                             ///  (init_h_offset-h_center)^2 + (init_v_offset-v_center)^2
    UINT16 LENRDn8RnrRSquareScale;                           ///< LENRDn8RnrRSquareScale 7u; range[0, 127]
    UINT16 LENRDn8RnrRSquareShift;                           ///< LENRDn8RnrRSquareShift 4u  range[0, 15]

    INT16  LENRDn16RnrBx;                                    ///< LENRDn16RnrBx 14s, init_h_offset-h_center
    INT16  LENRDn16RnrBy;                                    ///< LENRDn16RnrBy 14s, init_v_offset-v_center
    UINT32 LENRDn16RnrRSquareInit;                           ///< LENRDn16RnrRSquareInit 28u,
                                                             /// (init_h_offset-h_center)^2 + (init_v_offset-v_center)^2
    UINT16 LENRDn16RnrRSquareScale;                          ///< LENRDn16RnrRSquareScale 7u; range[0, 127]
    UINT16 LENRDn16RnrRSquareShift;                          ///< LENRDn16RnrRSquareShift 4u  range[0, 15]

    // SNR common
    UINT16 snrSkinHueMin;                                    ///< snrSkinHueMin 10u
    UINT16 snrSkinHueMax;                                    ///< snrSkinHueMax 8u
    UINT16 snrSkinYMin;                                      ///< snrSkinYMin 8u
    UINT16 snrSkinYMax;                                      ///< snrSkinYmaxSatMin 8u
    UINT16 snrSkinYmaxSatMin;                                ///< snrSkinYmaxSatMin 8u
    UINT16 snrSkinYmaxSatMax;                                ///< snrSkinYmaxSatMax 8u
    UINT16 snrSatMinSlope;                                   ///< snrSatMinSlope 8u
    UINT16 snrSatMaxSlope;                                   ///< snrSatMaxSlope 8u
    UINT16 snrBoundaryProbability;                           ///< snrBoundaryProbability 4u
    UINT16 snrQstepSkin;                                     ///< snrQstepSkin 8uQ8
    UINT16 snrQstepNonskin;                                  ///< snrQstepNonskin 8uQ8
    //   Word16u snr_skin_smoothing_str;                     ///< 2u, [0, 2]

    UINT16 faceNum;                                          ///< faceNum 3u, [0, 4], n means n+1
    UINT16 faceCenterHorizontal[LENR_V10_MAX_FACE_NUM];      ///< faceCenterHorizontal 16u
    UINT16 faceCenterVertical[LENR_V10_MAX_FACE_NUM];        ///< faceCenterVertical 16u
    UINT16 faceRadiusBoundary[LENR_V10_MAX_FACE_NUM];        ///< faceRadiusBoundary 8u
    UINT16 faceRadiusSlope[LENR_V10_MAX_FACE_NUM];           ///< faceRadiusSlope 8u
    UINT16 faceRadiusShift[LENR_V10_MAX_FACE_NUM];           ///< faceRadiusShift4u
    UINT16 faceSlopeShift[LENR_V10_MAX_FACE_NUM];            ///< faceSlopeShift 3u
    UINT16 faceHorizontalOffset;                             ///< faceHorizontalOffset16u
    UINT16 faceVerticalOffset;                               ///< faceVerticalOffset 16u
    UINT16 cnrScale;                                         ///< cnrScale

    INT16 us4InitPhV;                                        ///< us4InitPhV
    INT16 us4InitPhH;                                        ///< us4InitPhH

    // Crop
    UINT16 cropStripeToImageBoundaryDistLeft;                ///< cropStripeToImageBoundaryDistLeft 14u
    UINT16 cropOutputStripeWidth;                            ///< cropOutputStripeWidth 14u
    UINT16 cropFrameBoundaryPixAlignRight;                   ///< cropFrameBoundaryPixAlignRight 1u

    UINT16 postCropEnable;                                   ///< postCropEnable 1u
    UINT16 postCropFirstPixel;                               ///< postCropFirstPixel 14u
    UINT16 postCropLastPixel;                                ///< postCropLastPixel 14u
    UINT16 postCropFirstLine;                                ///< postCropFirstLine 14u
    UINT16 postCropLastLine;                                 ///< postCropLastLine 14u
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements LENR10 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class LENR10Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Pointer to the input data
    /// @param  pData         Pointer to the intepolation result
    /// @param  pReserveType  Pointer to the reserveType of this module
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the unpacked data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const LENR10InputData*                                  pInput,
        lenr_1_0_0::lenr10_rgn_dataType*                        pData,
        lenr_1_0_0::chromatix_lenr10_reserveType*               pReserveType,
        lenr_1_0_0::chromatix_lenr10Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                   pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetRNRAdj
    ///
    /// @brief  Set RNR Adjustment
    ///
    /// @param  pRnrThrdArr    Pointer to the RNR Thrd
    /// @param  pRnrGainArr    Pointer to the RNR Gain
    /// @param  pRnrAnchor     Pointer to the RNR Gain
    /// @param  pRnrBase       Pointer to the RNR base
    /// @param  pRnrShift      Pointer to the RNR Shift
    /// @param  pRnrSlope      Pointer to the RNR Slope
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void SetRNRAdj(
        FLOAT*  pRnrThrdArr,
        FLOAT*  pRnrGainArr,
        UINT16* pRnrAnchor,
        UINT16* pRnrBase,
        UINT16* pRnrShift,
        INT16*  pRnrSlope);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDNFNRArr
    ///
    /// @brief  Set FNR parameter
    ///
    /// @param  pDnFnrArr          Pointer to FNR
    /// @param  pUnpackDnFnrArr    Pointer to FNR unpack
    /// @param  size               size parameter
    /// @param  clampMin           clamp min parameter
    /// @param  clampMax           clamp max parameter
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void SetDNFNRArr(
        UINT32* pDnFnrArr,
        UINT16* pUnpackDnFnrArr,
        INT32   size,
        INT32   clampMin,
        INT32   clampMax);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDNSNRArr
    ///
    /// @brief  Set SNR parameter
    ///
    /// @param  pDnSnrArr          Pointer to SNR
    /// @param  pUnpacknSnrArr     Pointer to SNR unpack
    /// @param  size               size parameter
    /// @param  clampMin           clamp min parameter
    /// @param  clampMax           clamp max parameter
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void SetDNSNRArr(
        FLOAT*  pDnSnrArr,
        UINT16* pUnpacknSnrArr,
        INT32   size,
        INT32   clampMin,
        INT32   clampMax);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetRNRParam
    ///
    /// @brief  Set RNR Parameters
    ///
    /// @param  width               Width of image
    /// @param  height              Height of image
    /// @param  pRnrBx              Rnr X pointer
    /// @param  pRnrBy              RNR Y pointer
    /// @param  pRnrRSquareInit     RNR square init pointer
    /// @param  pRnrRSquareShift    pointer to Rsquare shift
    /// @param  pRnrRSquareScale    pointer to rsquare scale
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static void SetRNRParam(
        INT32   width,
        INT32   height,
        INT16*  pRnrBx,
        INT16*  pRnrBy,
        UINT32* pRnrRSquareInit,
        UINT16* pRnrRSquareShift,
        UINT16* pRnrRSquareScale);
};
#endif // LENR10SETTING_H
