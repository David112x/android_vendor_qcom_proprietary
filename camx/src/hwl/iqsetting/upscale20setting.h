// NOWHINE NC009 <- Shared file with system team so uses non-Camx file nameing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  upscale20setting.h
/// @brief Upscale20 module IQ settings calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef UPSCALE20SETTING_H
#define UPSCALE20SETTING_H

#include "iqcommondefs.h"
#include "iqsettingutil.h"
#include "upscale20setting_misc.h"
#include "upscale_2_0_0.h"

static const INT16 CoeffLUTSetSize = 4;
static const INT16 MaxYuvChannels  = 3;

/// @brief Chroma Upsample mode
enum ChromaUpMode
{
    INTERSTITIAL = 0, ///< interstitial mode
    COSITED      = 1  ///< cosited mode
};

/// @brief cosite mode
enum CositeMode
{
    EVEN = 0, ///< even
    ODD  = 1  ///< odd
};

/// @brief status for qseed3 calculations
enum qseed3Status
{
    MDP_QSEED3_STATUS_FAIL = 0, ///< For zero memory
    MDP_QSEED3_STATUS_OK        ///< For enough memory
};

/// @brief qseed3 operation mode
// NOWHINE NC004c: Share code with system team
struct Qseed3OpMode
{
    UINT8 enable;              ///< enable upscale
    UINT8 directionEnable;     ///< enable directional upscale
    UINT8 detailEnhanceEnable; ///< detail enhancement enable
    UINT8 colorSpace;          ///< 0:RGB 1:YUV
    UINT8 yConfig;             ///< Y channel config
    UINT8 uvConfig;            ///< UV channel config
    UINT8 aConfig;             ///< A config
    UINT8 alphaEnable;         ///< 1:enable alpha channel processing. 0: disable alpha channel processing
    UINT8 bitWidth;            ///< 0: 10-bit, 1: 8-bit.
    UINT8 blendConfig;         ///< 0: circular 1:separable
    UINT8 weightGain;          ///< Weight Gain
};

/// @brief qseed3 initial phase data
// NOWHINE NC004c: Share code with system team
struct Qseed3PhaseInit
{
    UINT32 horizontalPhaseInitY;  ///< Horizontal phase init for channel Y
    UINT32 verticalPhaseInitY;    ///< Vertical phase init for channel Y
    UINT32 horizontalPhaseInitUV; ///< Horizontal phase init for channel UV
    UINT32 verticalPhaseInitUV;   ///< Vertical phase init for channel UV
};

/// @brief qseed3 preload data
// NOWHINE NC004c: Share code with system team
struct Qseed3Preload
{
    UINT8 horizontalPreloadY;  ///< 0:69, Horizontal preload for Y channel
    UINT8 verticalPreloadY;    ///< 0:67, Vertical preload for Y channel
    UINT8 horizontalPreloadUV; ///< 0:69, Horizontal preload for UV channel
    UINT8 verticalPreloadUV;   ///< 0:67, Vertical preload for UV channel
};

/// @brief qseed3 detail enhance sharpening levels
// NOWHINE NC004c: Share code with system team
struct Qseed3DetailEnhanceSharpen
{
    INT16 sharpenLevel1; ///< first level of sharpening
    INT16 sharpenLevel2; ///< second level of sharpening
};

/// @brief qseed3 detail enhance sharpening control
// NOWHINE NC004c: Share code with system team
struct Qseed3DetailEnhanceSharpenControl
{
    UINT8 detailEnhancePrecision; ///< range 8 to 12. interface would pass [0,4] to save some bits.
                                  ///< HW would add 8 when using it.
    UINT8 detailEnhanceLimit;     ///< detail enhancelimit range 0 to 8
    UINT8 detailEnhanceClip;      ///< local dynamic clipping parameter within Detail Ehancement
};

/// @brief qseed3 detail enhance shape threshold
// NOWHINE NC004c: Share code with system team
struct Qseed3DetailEnhanceShapeControl
{
    UINT16 thresholdQuiet;  ///< quite threshold
    UINT16 thresholdDieout; ///< dieout threshold
};

/// @brief qseed3 detail enhance threshold
// NOWHINE NC004c: Share code with system team
struct Qseed3DetailEnhanceThreshold
{
    UINT16 thresholdLow;  ///< low threshold
    UINT16 thresholdHigh; ///< high threshold
};

/// @brief qseed3 detail enhance Adjust Data 0
// NOWHINE NC004c: Share code with system team
struct Qseed3DetailEnhanceAdjustData0
{
    INT16 adjustA0; ///< Curve A0
    INT16 adjustA1; ///< Curve A1
    INT16 adjustA2; ///< Curve A2
};

/// @brief qseed3 detail enhance Adjust Data 1
// NOWHINE NC004c: Share code with system team
struct Qseed3DetailEnhanceAdjustData1
{
    INT16 adjustB0; ///< Curve B0
    INT16 adjustB1; ///< Curve B1
    INT16 adjustB2; ///< Curve B2
};

/// @brief qseed3 detail enhance Adjust Data 2
// NOWHINE NC004c: Share code with system team
struct Qseed3DetailEnhanceAdjustData2
{
    INT16 adjustC0; ///< Curve C0
    INT16 adjustC1; ///< Curve C1
    INT16 adjustC2; ///< Curve C2
};

/// @brief MDP input / output data dimensions
// NOWHINE NC004c: Share code with system team
struct MdpDataPath
{
    UINT32 inputWidth[MaxYuvChannels];   ///< input width for channel Y/U/V
    UINT32 inputHeight[MaxYuvChannels];  ///< input height for channel Y/U/V
    UINT32 outputWidth[MaxYuvChannels];  ///< output width for channel Y/U/V
    UINT32 outputHeight[MaxYuvChannels]; ///< output height for channel Y/U/V
};

/// @brief Upscale Software Configuration
// NOWHINE NC004c: Share code with system team
struct UpscaleSwConfig
{
    UINT32 scaleEnable;                       ///< Upscale enable flag
    UINT8  detailEnhanceEnable;               ///< detail enhancement enable flag
    UINT8  detailEnhanceClipShift;            ///< 1:enabled, 0: disabled for clipping shift
    UINT32 chromaSite;                        ///< chroma site io
    UINT32 comp0FilterMethod;                 ///< scaling method that is applied to Y component of YUV format
    UINT32 comp1and2FilterMethod;             ///< combined with directional filter to generate filter coefficients
                                              ///< for edge-directed interpolation for CbCr channel
    UINT32 compROIInWidth[MaxYuvChannels];  ///< Compute Region of Interest in Width
    UINT32 compROIInHeight[MaxYuvChannels]; ///< Compute Region of Interest in Height
    UINT16 outWidth;                          ///< module output width
    UINT16 outHeight;                         ///< module output height
    UINT8  bitPrecisionIn;                    ///< module input bit precision. In Napali, it is 10 bpp
    UINT8  bitPrecisionOut;                   ///< module output bit precision. In Napali, it is 10 bpp
    UINT8  blendFilter;                       ///< 0 (circular filter), 1 (separable filter)
    UINT32 sharpeningStrength1;               ///< sharpening strength level when delta out is no more than dECurveT1
    UINT32 sharpeningStrength2;               ///< sharpening strength level when delta out is more than dECurveT1
    UINT16 quietZoneThreshold;                ///< Quiet zone threshold
    UINT16 curveRange;                        ///< Detail Enhancement Curve Range
    UINT16 detailEnhanceCurveT1;              ///< Detail Enhancement Curve Threshold low
    UINT16 detailEnhanceCurveT2;              ///< Detail Enhancement Curve Threshold high
    UINT8  detailEnhanceLimiter;              ///< Detail Enhancement Limit Value
};

/// @brief Upscale Hardware Configuration
// NOWHINE NC004c: Share code with system team
struct  UpScaleHwConfig
{
    Qseed3OpMode                      opMode;                 ///< upscale operation mode
    Qseed3PhaseInit                   phaseInit;              ///< qseed3 initial phase data
    Qseed3Preload                     preload;                ///< qseed3 preload data
    Qseed3DetailEnhanceSharpen        sharpenStrength;        ///< sharpen strength
    Qseed3DetailEnhanceSharpenControl detailEnhanceControl;   ///< sharpen control for detail enhancement
    Qseed3DetailEnhanceShapeControl   curveShape;             ///< shape threshold
    Qseed3DetailEnhanceThreshold      detailEnhanceThreshold; ///< detail enhancement threshold
    Qseed3DetailEnhanceAdjustData0    curveAs;                ///< A curves
    Qseed3DetailEnhanceAdjustData1    curveBs;                ///< B curves
    Qseed3DetailEnhanceAdjustData2    curveCs;                ///< C curves

    UINT32 horizontalPhaseStepY;                              ///< Horizontal phase step for channel Y
    UINT32 verticalPhaseStepY;                                ///< Vertical phase step for channel Y
    UINT32 horizontalPhaseStepUV;                             ///< Horizontal phase step for channel UV
    UINT32 verticalPhaseStepUV;                               ///< Vertical phase step for channel UV
    UINT32 twoDFilterA[CoeffLUTSetSize][CoeffLUTSizeA]; ///< 2D LUT filter A
    UINT32 twoDFilterB[CoeffLUTSetSize][CoeffLUTSizeB]; ///< 2D LUT filter B
    UINT32 twoDFilterC[CoeffLUTSetSize][CoeffLUTSizeC]; ///< 2D LUT filter C
    UINT32 twoDFilterD[CoeffLUTSetSize][CoeffLUTSizeD]; ///< 2D LUT filter D
};

/// @brief Unpacked Data for Chroma Upsample v20 Module
// NOWHINE NC004c: Share code with system team
struct ChromaUp20UnpackedField
{
    UINT16 enable;                    ///< Chroma Upsample Module enable
    UINT16 enableHorizontal;          ///< Horizontal enable
    UINT16 outputHorizontal;          ///< Output right crop, n means n+1
    UINT32 initialPhaseHorizontal;    ///< 21uQ21
    INT16  indexOffsetHorizontal;     ///< 2s for left padding, -1: pad 2x, 0: pad 1x, 1: no pad
    UINT16 roundingPatternHorizontal; ///< 2u: 0x0:AAAA; 0x1:ABBA; 0x2:BAAB; 0x3:0000; A=256, B=255
    UINT16 enableVertical;            ///< Vertical enable
    UINT16 outputVertical;            ///< Output bottom crop, n mean n+1
    UINT32 initialPhaseVertical;      ///< 21uQ21
    INT16  indexOffsetVertical;       ///< 2s for top padding, -1: pad 2x, 0: pad 1x, 1: no pad
    UINT16 roundingPatternVertical;   ///< 2u: 0x0:AAAA; 0x1:ABBA; 0x2:BAAB; 0x3:0000; A=256, B=255
    UINT16 cosited;                   ///< 0: interstitial; 1: cosited
    UINT16 evenOdd;                   ///< 0: even; 1: odd
};

/// @brief Unpacked Data for Upscale v20 Module
// NOWHINE NC004c: Share code with system team
struct Upscale20UnpackedField
{
    UINT16            enable;          ///< Module enable flag
    UpScaleHwConfig   upscaleHwConfig; ///< Hardware parameters
    UpscaleSwConfig   upscaleSwConfig; ///< Software parameters
    MdpDataPath       dataPath;        ///< input / output dimensions for Y/U/V channels
    qseed3Status      status;          ///< status of QSeed3 calculation
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements Upscale20 module IQ settings calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE NC004c: Share code with system team
class Upscale20Setting
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateHWSetting
    ///
    /// @brief  Calculate the unpacked Upscale20 register value
    ///
    /// @param  pInput        Input data to the Upscale20 Module
    /// @param  pData         The output of the interpolation algorithem
    /// @param  pReserveData  Pointer to the Chromatix ReserveType field
    /// @param  pModuleEnable Pointer to the variable(s) to enable this module
    /// @param  pOutput       Pointer to Upscale20 Unpacked Register Data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateHWSetting(
        const Upscale20InputData*                                     pInput,
        upscale_2_0_0::upscale20_rgn_dataType*                        pData,
        upscale_2_0_0::chromatix_upscale20_reserveType*               pReserveData,
        upscale_2_0_0::chromatix_upscale20Type::enable_sectionStruct* pModuleEnable,
        VOID*                                                         pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CalculateChromaUpHWSetting
    ///
    /// @brief  Calculate the unpacked Chroma Upsammple register value
    ///
    /// @param  pInput    Input data to the Upscale20 Module
    /// @param  pRegCmd   Pointer to Chroma Upsample Unpacked Register Data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL CalculateChromaUpHWSetting(
        const Upscale20InputData* pInput,
        ChromaUp20UnpackedField*  pRegCmd);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChromaUpsampleSW
    ///
    /// @brief  Chroma Upsample SW module calculations
    ///
    /// @param  inputWidth         Input image width
    /// @param  inputHeight        Input image height
    /// @param  outputWidth        Output image width
    /// @param  outputHeight       Output image height
    /// @param  pUnpackedRegData   Pointer to unpacked chroma upsample register data
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL ChromaUpsampleSW(
        UINT32                   inputWidth,
        UINT32                   inputHeight,
        UINT32                   outputWidth,
        UINT32                   outputHeight,
        ChromaUp20UnpackedField* pUnpackedRegData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUpscaleHwRegistry
    ///
    /// @brief  Upscale Hardware Register calculations
    ///
    /// @param  pUnpackedField    Input|Output: Pointer to unpacked Upscale register data.
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL SetUpscaleHwRegistry(
        Upscale20UnpackedField* pUnpackedField);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUpscaleSwRegistry
    ///
    /// @brief  Upscale Software Register calculations
    ///
    /// @param  pInput             Input:  Pointer to the Upscale20 Module input data
    /// @param  pReserveData       Input:  Pointer to reserved data
    /// @param  pUnpackedField     Output: Pointer to unpacked Upscale register data.
    ///
    /// @return TRUE for success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL SetUpscaleSwRegistry(
        const Upscale20InputData*                       pInput,
        upscale_2_0_0::chromatix_upscale20_reserveType* pReserveData,
        Upscale20UnpackedField*                         pUnpackedField);
};

#endif // UPSCALE20SETTING_H
