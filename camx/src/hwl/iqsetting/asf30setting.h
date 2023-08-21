// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  asf30setting.h
/// @brief ASF30 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ASF30SETTING_H
#define ASF30SETTING_H

#include "iqcommondefs.h"
#include "asf_3_0_0.h"
#include "iqsettingutil.h"

static const UINT32 LUT_ACTIVITY_NORM_Q_BITS     = 8;
static const UINT32 LUT_WEIGHT_MOD_Q_BITS        = 8;
static const UINT32 LUT_GAIN_POSNEG_Q_BITS       = 5;
static const UINT32 LUT_GAIN_WEIGHT_Q_BITS       = 8;
static const UINT32 LUT_GAIN_CONTRAST_Q_BITS     = 8;
static const UINT32 LUT_GAIN_CHROMA_Q_BITS       = 8;
static const UINT32 LUT_SKIN_Q_BITS              = 8;
static const UINT32 RADIAL_CF_Q_BITS             = 8;
static const UINT32 FACE_Q_BITS                  = 8;
static const UINT32 NORM_SCALE_Q_BITS            = 6;
static const UINT32 GAIN_CAP_Q_BITS              = 5;
static const UINT32 SP_Q_BITS                    = 4;
static const UINT32 MEDIAN_BLEND_OFFSET_Q_BITS   = 4;
static const UINT32 CORNER_THRESHOLD_Q_BITS      = 10;
static const UINT32 SMOOTHING_STRENGTH_Q_BITS    = 10;
static const UINT32 NUM_OF_NZ_ENTRIES            = 8;
static const UINT32 LUT_LAYER1_FILTER_SIZE       = 10;
static const UINT32 LUT_ACTIVITY_BPF_SIZE        = 6;
static const UINT32 LUT_LAYER2_FILTER_SIZE       = 6;
static const UINT32 LUT_ACTIVITY_NORM_SIZE       = 64;
static const UINT32 LUT_WEIGHT_MOD_SIZE          = 64;
static const UINT32 LUT_SOFT_THRESHOLD_SIZE      = 64;
static const UINT32 LUT_GAINPOSNEG_SIZE          = 64;
static const UINT32 LUT_GAINWEIGHT_SIZE          = 64;
static const UINT32 LUT_GAINCONTRAST_SIZE        = 32;
static const UINT32 LUT_GAINCHROMA_SIZE          = 32;
static const UINT32 LUT_SKIN_SIZE                = 17;
static const UINT32 R_SQAURE_Q_FACTOR            = 12;
static const UINT32 R_SQUARE_SCALE_FACTOR        = 6;
static const UINT32 RADIAL_GAIN_SLOPE_Q_FACTOR   = 11;
static const UINT32 RADIAL_ACT_SLOPE_Q_FACTOR    = 11;
static const UINT32 SKIN_SATURATIN_Q_FACTOR      = 8;
static const UINT32 NUM_LUMA_R_SQUARE_ENTRIES    = 4;
static const UINT32 NUM_ASF_RADIAL_ENTRIES       = 4;
static const UINT32 EA_CONV_FACTOR               = 25;
static const UINT32 FLAT_THR_MIN                 = 0;
static const UINT32 FLAT_THR_MAX                 = (1 << 13) - 1;
static const UINT32 MAX_SMOOTH_MIN               = 0;
static const UINT32 MAX_SMOOTH_MAX               = (1 << 8) - 1;
static const UINT32 CORNER_THR_MIN               = 0;
static const UINT32 CORNER_THR_MAX               = (1 << 16) - 1;
static const UINT32 SMOOTH_STR_MIN               = 0;
static const UINT32 SMOOTH_STR_MAX               = (1 << 10) - 1;
static const UINT32 MEDIAN_BLEND_OFFSET_MIN      = 0;
static const UINT32 MEDIAN_BLEND_OFFSET_MAX      = 15;
static const UINT32 DMI_GAINPOSNEG_SIZE          = 256;
static const UINT32 DMI_GAINWEIGHT_SIZE          = 256;
static const UINT32 DMI_SOFT_SIZE                = 256;
static const UINT32 DMI_ACT_NORM_SIZE            = 256;
static const UINT32 DMI_WEIGHT_MOD_SIZE          = 256;
static const UINT32 DMI_GAINCHROMA_SIZE          = 256;
static const UINT32 DMI_GAINCONTRAST_SIZE        = 256;
static const UINT32 DMI_SKIN_SIZE                = 17;
static const UINT32 RNR_CONTROL_POINTS           = 3;
static const UINT32 SKIN_Q_NONQ_MIN              = 0;
static const UINT32 SKIN_Q_NONQ_MAX              = 128;
static const UINT32 MAX_FRACTION_WITH_Q          = 2048;
static const FLOAT  SP_FLOAT_MIN                 = 0.0f;
static const FLOAT  SP_FLOAT_MAX                 = 1.0f;

static const INT16 L1_CORNER_FILTER_MIN[LUT_LAYER1_FILTER_SIZE]  =
{
    MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT11, MIN_INT12
};

static const INT16 L1_CORNER_FILTER_MAX[LUT_LAYER1_FILTER_SIZE]  =
{
    MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT11, MAX_INT12
};

static const INT16 L1_LOWPASS_FILTER_MIN[LUT_LAYER1_FILTER_SIZE] =
{
    MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT10, MIN_INT11, MIN_INT12
};

static const INT16 L1_LOWPASS_FILTER_MAX[LUT_LAYER1_FILTER_SIZE] =
{
    MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT10, MAX_INT11, MAX_INT12
};

static const INT16 L1_ACTIVITY_BPF_MIN[LUT_ACTIVITY_BPF_SIZE]    =
{
    MIN_INT8, MIN_INT8, MIN_INT8, MIN_INT8, MIN_INT9, MIN_INT10
};

static const INT16 L1_ACTIVITY_BPF_MAX[LUT_ACTIVITY_BPF_SIZE]    =
{
    MAX_INT8, MAX_INT8, MAX_INT8, MAX_INT8, MAX_INT9, MAX_INT10
};

static const INT16 L2_CORNER_FILTER_MIN[LUT_LAYER2_FILTER_SIZE]  =
{
    MIN_INT8, MIN_INT8, MIN_INT8, MIN_INT8, MIN_INT9, MIN_INT10
};

static const INT16 L2_CORNER_FILTER_MAX[LUT_LAYER2_FILTER_SIZE]  =
{
    MAX_INT8, MAX_INT8, MAX_INT8, MAX_INT8, MAX_INT9, MAX_INT10
};

static const INT16 L2_LOWPASS_FILTER_MIN[LUT_LAYER2_FILTER_SIZE] =
{
    MIN_INT8, MIN_INT8, MIN_INT8, MIN_INT8, MIN_INT9, MIN_INT10
};

static const INT16 L2_LOWPASS_FILTER_MAX[LUT_LAYER2_FILTER_SIZE] =
{
    MAX_INT8, MAX_INT8, MAX_INT8, MAX_INT8, MAX_INT9, MAX_INT10
};

static const INT16 L2_ACTIVITY_BPF_MIN[LUT_ACTIVITY_BPF_SIZE]    =
{
    MIN_INT8, MIN_INT8, MIN_INT8, MIN_INT8, MIN_INT9, MIN_INT10
};

static const INT16 L2_ACTIVITY_BPF_MAX[LUT_ACTIVITY_BPF_SIZE]    =
{
    MAX_INT8, MAX_INT8, MAX_INT8, MAX_INT8, MAX_INT9, MAX_INT10
};

/// @brief ASF30 module unpacked data
// NOWHINE NC004c: Share code with system team
struct ASF30UnpackedField
{
    // enable flags
    UINT8  enable;                                             ///< module enable flag
    UINT8  smoothPercentage;                                   ///< layer1 3*3 median filter smoothing percentage
    UINT8  specialEffectEnable;                                ///< special effect enable flag
    UINT8  specialEffectAbsoluteEnable;                        ///< 0 for Emboss and 1 for Sketch or Neon depends on negAbsY1
    UINT8  negateAbsoluteY1;                                   ///< 1 for Sketch and 1 for Neon when sp_eff_abs_en is 1
    INT16  nonZero[NUM_OF_NZ_ENTRIES];                         ///< negation & zero to alter kernel symmetric

    // filters
    INT16  layer1LowPassFilter[LUT_LAYER1_FILTER_SIZE];        ///< layer1 Low Pass Filter symmteric coeff
    INT16  layer1CornerFilter[LUT_LAYER1_FILTER_SIZE];         ///< layer1 High Pass Filter symmteric coeff
    INT16  layer1ActivityBPF[LUT_ACTIVITY_BPF_SIZE];           ///< layer1 activity band pass coeff
    INT16  layer2LowPassFilter[LUT_LAYER2_FILTER_SIZE];        ///< layer2 Low Pass Filter symmteric coeff
    INT16  layer2CornerFilter[LUT_LAYER2_FILTER_SIZE];         ///< layer2 High Pass Filter symmteric coeff
    INT16  layer2ActivityBPF[LUT_ACTIVITY_BPF_SIZE];           ///< layer2 activity band pass coeff

    // clamping high/low levels
    INT16  layer1RegTH;                                        ///< Manual fixed layer1 positive clamping level
    INT16  layer1RegTL;                                        ///< Manual fixed layer1 negetive clamping level
    INT16  layer2RegTH;                                        ///< Manual fixed layer2 positive clamping level
    INT16  layer2RegTL;                                        ///< Manual fixed layer2 negetive clamping level

    UINT8  layer1ActivityClampThreshold;                       ///< layer1 static clamp of 5*5 band pass filter output
    UINT8  layer2ActivityClampThreshold;                       ///< layer2 static clamp of 5*5 band pass filter output
    UINT8  layer1NormScale;                                    ///< layer1 scale factor of 5*5 band pass filter output
    UINT8  layer2NormScale;                                    ///< layer2 scale factor of 5*5 band pass filter output
    UINT8  layer1L2NormEn;                                     ///< layer1 enable L2 norm of 5*5 band pass filter output
    UINT8  layer2L2NormEn;                                     ///< layer2 enable L2 norm of 5*5 band pass filter output

    UINT8  gammaCorrectedLumaTarget;                           ///< luma target after gamma to control level based sharpening.
    UINT8  layer1GainPositiveLut[DMI_GAINPOSNEG_SIZE];         ///< level-based sharpening gain LUT for positive halos
    UINT8  layer2GainPositiveLut[DMI_GAINPOSNEG_SIZE];         ///< level-based sharpening gain LUT for positive halos
    UINT8  layer1GainNegativeLut[DMI_GAINPOSNEG_SIZE];         ///< level-based sharpening gain LUT for negative halos
    UINT8  layer2GainNegativeLut[DMI_GAINPOSNEG_SIZE];         ///< level-based sharpening gain LUT for negative halos
    UINT8  layer1GainWeightLut[DMI_GAINWEIGHT_SIZE];           ///< normalized activity-based sharpening gain LUT
    UINT8  layer2GainWeightLut[DMI_GAINWEIGHT_SIZE];           ///< normalized activity-based sharpening gain LUT
    UINT8  layer1SoftThresholdLut[DMI_SOFT_SIZE];              ///< level-based soft-thresholding LUT to determine the
                                                               ///< final sharpened value
    UINT8  layer1SoftThresholdWeightLut[DMI_SOFT_SIZE];        ///< normalized activity-based LUT to control the slope when
                                                               ///< sharpened value magnitude is less than the soft threshold
    UINT8  layer2SoftThresholdLut[DMI_SOFT_SIZE];              ///< level-based soft-thresholding LUT to determine the
                                                               ///< final sharpened value
    UINT8  layer2SoftThresholdWeightLut[DMI_SOFT_SIZE];        ///< normalized activity-based LUT to control the slope when
                                                               ///< sharpened value magnitude is less than the soft threshold
    UINT8  layer1ActivityNormalizationLut[DMI_ACT_NORM_SIZE];  ///< level-based normalization LUT to calculate normalized
                                                               ///< activity
    UINT8  layer2ActivityNormalizationLut[DMI_ACT_NORM_SIZE];  ///< level-based normalization LUT to calculate normalized
                                                               ///< activity

    UINT8  gainCap;                                            ///< upper cap of sharpening gain
    UINT8  medianBlendUpperOffset;                             ///< upper offset of median blend
    UINT8  medianBlendLowerOffset;                             ///< lower offset of median blend

    // RNR parameters
    UINT32 rSquareTable[RNR_CONTROL_POINTS];                   ///< RNR control points
    UINT8  rSquareShift;                                       ///< RNR radial anchor points shift
    INT16  radialActivitySlopeTable[RNR_CONTROL_POINTS];       ///< RNR slope between control points
    INT16  radialActivityCFTable[RNR_CONTROL_POINTS];          ///< Correction factor for each control point
    INT16  radialGainSlopeTable[RNR_CONTROL_POINTS];           ///< RNR slope between control points
    INT16  radialGainCFTable[RNR_CONTROL_POINTS];              ///< Correction factor for each control point
    INT16  initialOffsetMinusCenterHorizontal;                 ///< top left corner X coord relative to center
    INT16  initialOffsetMinusCenterVertical;                   ///< top left corner Y coord relative to center
    UINT16 rSquareScale;                                       ///< parameter to scale radius^2
    UINT32 initialRSquare;                                     ///< Initial radius^2 value for the top left corner of the stripe
    UINT8  radialGainCFShift[RNR_CONTROL_POINTS];              ///< shift values for slope
    UINT8  radialActivityCFShift[RNR_CONTROL_POINTS];          ///< shift values for slope
    UINT8  radialEnable;                                       ///< RNR enable flag

    // Skin detection
    UINT32 minHue;                                             ///< Min hue value for skin detection.
    UINT32 maxHue;                                             ///< Max hue value for skin detection.
    UINT32 minY;                                               ///< Minimum Y value for skin detection.
    UINT32 maxY;                                               ///< Maximum Y value for skin detection.
    UINT32 minShY;                                             ///< Minimum saturation value for skin when Y=maxY
    UINT32 maxShY;                                             ///< Maximum saturation value for skin when Y=maxY
    UINT32 paraSmin;                                           ///< skin saturation parameter
    UINT32 paraSmax;                                           ///< skin saturation parameter

    // Skin level Q4 qualtization
    UINT16 boundaryProbability;                                ///< Denoting the skin tone probability at the boundary
    UINT16 skinQ;                                              ///< used to calculate skin_nonskin_to_skin_qratio
    UINT16 nonskinQ;                                           ///< used to calculate skin_nonskin_to_skin_qratio

    UINT8  gainContrastNegativeLut[DMI_GAINCONTRAST_SIZE];     ///< Contrast-based negative halo sharpening gain
    UINT8  gainContrastPositiveLut[DMI_GAINCONTRAST_SIZE];     ///< Contrast-based positive halo sharpening gain
    UINT8  chromaGradientPositiveLut[DMI_GAINCHROMA_SIZE];     ///< Chroma gradient-based positive halo sharpening gain
    UINT8  chromaGradientNegativeLut[DMI_GAINCHROMA_SIZE];     ///< Chroma gradient-based negative halo sharpening gain
    UINT8  skinGainLut[DMI_SKIN_SIZE];                         ///< Skin-detection-based sharpening gain
    UINT8  skinActivityLut[DMI_SKIN_SIZE];                     ///< Skin-detection-based activity gain

    UINT8  layer1Enable;                                       ///< layer 1 7x7 ASF enable flag
    UINT8  layer2Enable;                                       ///< layer 2 13x13 ASF enable flag
    UINT8  contrastEnable;                                     ///< contrast enable flag of skin tone range.
    UINT8  chromaGradientEnable;                               ///< enable bit of chroma gradient statistics
    UINT8  skinEnable;                                         ///< enable bit of skin color based noise reduction
    UINT8  edgeAlignmentEnable;                                ///< enable edge alignment

    UINT32 flatThreshold;                                      ///< Apply edge smoothing only when maximum AAD >= FlatThreshold
    UINT32 maxSmoothingClamp;                                  ///< Edge smoothing value change is clamped by Texture Threshold
    UINT32 cornerThreshold;                                    ///< Edge_smoothing value is applied only when max_SAD >=
                                                               ///< min_SAD * similarituyThreshold
    UINT16 smoothingStrength;                                  ///< Blending factor between original pixel and edge smoothed
                                                               ///< pixel

    UINT16 faceCenterHorizontal[MAX_FACE_NUM];                 ///< face center horizontal pos
    UINT16 faceCenterVertical[MAX_FACE_NUM];                   ///< face center vertical pos
    UINT16 faceNum;                                            ///< total face number configured
    UINT16 faceRadiusBoundary[MAX_FACE_NUM];                   ///< face boundary: face_radius_boundary << face_radius_shift
    UINT16 faceRadiusSlope[MAX_FACE_NUM];                      ///< transition area slope:
                                                               ///< face_radius_slope/(2^face_slope_shift)
    UINT16 faceRadiusShift[MAX_FACE_NUM];                      ///< face radius shift
    UINT16 faceSlopeShift[MAX_FACE_NUM];                       ///< slope shift values
    UINT16 faceHorizontalOffset;                               ///< face region horizontal offset
    UINT16 faceVerticalOffset;                                 ///< face region vertical offset
    UINT8  faceEnable;                                         ///< face detection enable
    UINT32 textureThr;                                         ///< texture threshold or max edge smoothing change
    UINT32 similarityThr;                                      ///< xml check  added new variable corner from edge
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements ASF30 module setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class ASF30Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked register value
    ///
    /// @param  pInput        Input data to the ASF30 Module
    /// @param  pData         Interpolated Data
    /// @param  pReserveType  Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pRegCmd       Pointer to the Unpacked Register Data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const ASF30InputData*                                 pInput,
        asf_3_0_0::asf30_rgn_dataType*                        pData,
        asf_3_0_0::chromatix_asf30_reserveType*               pReserveType,
        asf_3_0_0::chromatix_asf30Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                 pRegCmd);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateFilterSetting
    ///
    /// @brief  Calculate the unpacked register value for ASF filters
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateFilterSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateActNormLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Activity
    ///         Normalization LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateActNormLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateWeightLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Weight
    ///         Modulation and Soft Threshold Weight LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateWeightLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateSoftThresholdLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Layer1/Layer2
    ///         Soft Thresold LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateSoftThresholdLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateGainPositiveLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Layer1/Layer2
    ///         Gain Positive LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateGainPositiveLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd,
        FLOAT                               sharpnessVal);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateGainNegativeLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Layer1/Layer2
    ///         Gain Negative LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateGainNegativeLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd,
        FLOAT                               sharpnessVal);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateGainWeightLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Layer1/Layer2
    ///         Gain Weight LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateGainWeightLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateGainContrastNegLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Gain
    ///         Contrast Negative LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateGainContrastNegLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateGainContrastPosLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Gain
    ///         Contrast Positive LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateGainContrastPosLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateGainChromaNegLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Gain
    ///         Chroma Negative LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateGainChromaNegLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateGainChromaPosLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Gain
    ///         Chroma Positive LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateGainChromaPosLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateSkinGainActLUTSetting
    ///
    /// @brief  Calculate the unpacked register value for Skin Gain
    ///         and Skin Activity LUT tables
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateSkinGainActLUTSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateRNRSetting
    ///
    /// @brief  Calculate the unpacked register value for Radial Noise
    ///         Reduction tables and parameters
    ///
    /// @param  pInput     Input data to the ASF30 Module
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateRNRSetting(
        const ASF30InputData*                   pInput,
        asf_3_0_0::asf30_rgn_dataType*          pData,
        asf_3_0_0::chromatix_asf30_reserveType* pReserveType,
        ASF30UnpackedField*                     pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateSkinSetting
    ///
    /// @brief  Calculate the unpacked register value for Skin
    ///         parameters
    ///
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateSkinSetting(
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateFaceSetting
    ///
    /// @brief  Calculate the unpacked register value for Face
    ///         parameters
    ///
    /// @param  pInput     Input data to the ASF30 Module
    /// @param  pData      Interpolated Data
    /// @param  pRegCmd    Pointer to the Unpacked Register Data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID CalculateFaceSetting(
        const ASF30InputData*               pInput,
        asf_3_0_0::asf30_rgn_dataType*      pData,
        ASF30UnpackedField*                 pRegCmd);
};

#endif // IPEASF30SETTING_H
