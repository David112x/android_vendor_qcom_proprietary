// NOWHINE NC009 <- Shared file with system team so uses non-CamX file naming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  upscale20setting.cpp
/// @brief IPE Upscale20 setting calculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "upscale20setting.h"

static const UINT32 FilterMethodEdYUVRGB        = 0;
static const UINT32 FilterMethodCircularYUVRGB  = 1;
static const UINT32 FilterMethodSeparableYUVRGB = 2;
static const UINT32 ScalerPhaseAccumQBits       = 21;
static const FLOAT  HorizontalScaleFactor       = 0.5f;  // (input/output) less than 1 -> upscale
static const FLOAT  VerticalScaleFactor         = 0.5f;  // (input/output) less than 1 -> upscale

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Upscale20Setting::CalculateHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Upscale20Setting::CalculateHWSetting(
    const Upscale20InputData*                                     pInput,
    upscale_2_0_0::upscale20_rgn_dataType*                        pData,
    upscale_2_0_0::chromatix_upscale20_reserveType*               pReserveData,
    upscale_2_0_0::chromatix_upscale20Type::enable_sectionStruct* pModuleEnable,
    VOID*                                                         pOutput)
{
    BOOL result = FALSE;

    if ((NULL != pInput)        &&
        (NULL != pData)         &&
        (NULL != pReserveData)  &&
        (NULL != pModuleEnable) &&
        (NULL != pOutput))
    {
        Upscale20UnpackedField* pUnpackedField = static_cast<Upscale20UnpackedField*>(pOutput);
        CAMX_ASSERT(NULL != pUnpackedField);

        pUnpackedField->enable = static_cast<UINT16>(pModuleEnable->upscale_enable);

        result = SetUpscaleSwRegistry(pInput, pReserveData, pUnpackedField);
        if (TRUE == result)
        {
            result = SetUpscaleHwRegistry(pUnpackedField);
        }
    }

    if (FALSE == result)
    {
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Upscale20Setting::CalculateChromaUpHWSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Upscale20Setting::CalculateChromaUpHWSetting(
    const Upscale20InputData* pInputData,
    ChromaUp20UnpackedField*  pUnpackedField)
{
    BOOL  result = TRUE;

    if ((NULL != pInputData) && (NULL != pUnpackedField))
    {
        // @todo (CAMX-2205) Map IPE pipeline input image height / width from ISP
        UINT32 ch1Height    = pInputData->ch1InputHeight;
        UINT32 ch1Width     = pInputData->ch1InputWidth;
        UINT32 ch2Height    = pInputData->ch2InputHeight;
        UINT32 ch2Width     = pInputData->ch2InputWidth;
        UINT32 ch1WidthOut  = ch1Width;
        UINT32 ch1HeightOut = ch1Height;
        UINT32 ch2WidthOut  = ch2Width;
        UINT32 ch2HeightOut = ch2Height;

        // @todo (CAMX-2298) Need to map the cosited / even_odd values from ISP
        pUnpackedField->cosited          = pInputData->cosited;
        pUnpackedField->evenOdd          = pInputData->evenOdd;

        pUnpackedField->enable           = static_cast<UINT16>(pInputData->pChromatix->enable_section.upscale_enable);
        pUnpackedField->enableHorizontal = pInputData->enableHorizontal;
        pUnpackedField->enableVertical   = pInputData->enableVertical;

        // Chroma upscaler registers are Not Initialized:
        // Output dimension as decided by enable bits
        if (pUnpackedField->enableHorizontal)
        {
            ch1WidthOut = pInputData->ch0InputWidth;
            ch2WidthOut = pInputData->ch0InputWidth;
        }  // else output width equals input width

        if (pUnpackedField->enableVertical)
        {
            ch1HeightOut = pInputData->ch0InputHeight;
            ch2HeightOut = pInputData->ch0InputHeight;
        }  // else output height equals input height

        // Setup HW registers by SW calculations on defaults
        result = ChromaUpsampleSW(ch1Width, ch1Height, ch1WidthOut, ch1HeightOut, pUnpackedField);

        if (TRUE == result)
        {
            if (ch1WidthOut > 1 && ch1HeightOut > 1)
            {
                pUnpackedField->outputHorizontal = static_cast<UINT16>(ch1WidthOut - 1);
                pUnpackedField->outputVertical   = static_cast<UINT16>(ch1HeightOut - 1);
            }
            else
            {
                result = FALSE;
                /// @todo (CAMX-1812) Need to add logging for Common library
            }
        }
        else
        {
            result = FALSE;
            /// @todo (CAMX-1812) Need to add logging for Common library
        }
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Upscale20Setting::ChromaUpsampleSW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Upscale20Setting::ChromaUpsampleSW(
    UINT32                   inputWidth,
    UINT32                   inputHeight,
    UINT32                   outputWidth,
    UINT32                   outputHeight,
    ChromaUp20UnpackedField* pUnpackedRegData)
{
    BOOL  result                  = TRUE;

    float horizontalInitialOffset = 0;
    float verticalInitialOffset   = 0;
    float initialPhase;

    if (inputWidth <= outputWidth && inputHeight <= outputHeight)
    {
        if ((TRUE == pUnpackedRegData->enableHorizontal) && (inputWidth * 2 != outputWidth))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "InputWidth [0x%d], OutputWidth [0x%d]", inputWidth, outputWidth);
        }

        if ((TRUE == pUnpackedRegData->enableVertical) && (inputHeight * 2 != outputHeight))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "InputHeight [0x%d], OutputHeight [0x%d]", inputHeight, outputHeight);
        }

        // Output dimension registers
        if (TRUE == pUnpackedRegData->enableHorizontal)
        {
            pUnpackedRegData->outputHorizontal = static_cast<UINT16>(outputWidth - 1);
        }
        else
        {
            pUnpackedRegData->outputHorizontal = static_cast<UINT16>(inputWidth - 1);  // Bypassed
        }

        if (TRUE == pUnpackedRegData->enableVertical)
        {
            pUnpackedRegData->outputVertical   = static_cast<UINT16>(outputHeight - 1);
        }
        else
        {
            pUnpackedRegData->outputVertical   = static_cast<UINT16>(inputHeight - 1);  // Bypassed
        }

        // Phase adjustment
        if (pUnpackedRegData->cosited == INTERSTITIAL)
        {
            // Interstitial
            horizontalInitialOffset = 0.5f * HorizontalScaleFactor - 0.5f;  // -0.25
            verticalInitialOffset   = 0.5f * VerticalScaleFactor - 0.5f;    // -0.25
        }
        else if (pUnpackedRegData->cosited == COSITED && pUnpackedRegData->evenOdd == EVEN)
        {
            // Even cosite
            horizontalInitialOffset = 0.5f * HorizontalScaleFactor - 0.25f;  // 0
            verticalInitialOffset   = 0.5f * VerticalScaleFactor - 0.25f;    // 0
        }
        else if (pUnpackedRegData->cosited == COSITED && pUnpackedRegData->evenOdd == ODD)
        {
            // Odd cosite
            horizontalInitialOffset = 0.5f * HorizontalScaleFactor - 0.75f;  // -0.5
            verticalInitialOffset   = 0.5f * VerticalScaleFactor - 0.75f;    // -0.5
        }
        else
        {
            result = FALSE;
            /// @todo (CAMX-1812) Need to add logging for Common library
        }

        // Left pixel padding; default: {-1, 0, -1}
        pUnpackedRegData->indexOffsetHorizontal  =
            static_cast<INT16>(floor(horizontalInitialOffset));
        // [0, 1.0); default: {0.75, 0, 0.5}
        initialPhase                             =
            horizontalInitialOffset - static_cast<FLOAT>(pUnpackedRegData->indexOffsetHorizontal);
        // Default: {1572864, 0, 1048576}
        pUnpackedRegData->initialPhaseHorizontal =
            static_cast<UINT32>(floor(initialPhase * (1 << ScalerPhaseAccumQBits)));
        pUnpackedRegData->roundingPatternHorizontal = 1;

        // Top line padding; default: {-1, 0, -1}
        pUnpackedRegData->indexOffsetVertical    =
            static_cast<INT16>(floor(verticalInitialOffset));
        // [0, 1.0); default: {0.75, 0, 0.5}
        initialPhase                             =
            verticalInitialOffset - static_cast<FLOAT>(pUnpackedRegData->indexOffsetVertical);
        // Default: {1572864, 0, 1048576}
        pUnpackedRegData->initialPhaseVertical   =
            static_cast<UINT32>(floor(initialPhase * (1 << ScalerPhaseAccumQBits)));
        pUnpackedRegData->roundingPatternVertical   = 2;
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Upscale20Setting::SetUpscaleSwRegistry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Upscale20Setting::SetUpscaleSwRegistry(
    const Upscale20InputData*  pInput,
    upscale_2_0_0::chromatix_upscale20_reserveType* pReserveData,
    Upscale20UnpackedField*    pUnpackedField)
{
    BOOL  result = TRUE;

    if (NULL != pInput && NULL != pUnpackedField)
    {

        pUnpackedField->upscaleSwConfig.compROIInWidth[0]      = pInput->ch0InputWidth;
        pUnpackedField->upscaleSwConfig.compROIInHeight[0]     = pInput->ch0InputHeight;
        pUnpackedField->upscaleSwConfig.compROIInWidth[1]      = pInput->ch1InputWidth  << 1;  // YUV444 Input
        pUnpackedField->upscaleSwConfig.compROIInHeight[1]     = pInput->ch1InputHeight << 1;
        pUnpackedField->upscaleSwConfig.compROIInWidth[2]      = pInput->ch2InputWidth  << 1;
        pUnpackedField->upscaleSwConfig.compROIInHeight[2]     = pInput->ch2InputHeight << 1;

        pUnpackedField->upscaleSwConfig.outWidth               = static_cast<UINT16>(pInput->outWidth);
        pUnpackedField->upscaleSwConfig.outHeight              = static_cast<UINT16>(pInput->outHeight);

        pUnpackedField->upscaleSwConfig.bitPrecisionIn         = 10; // Napali will have 10 bpp
        pUnpackedField->upscaleSwConfig.bitPrecisionOut        = 10;

        // Hardware tied constants to registers
        pUnpackedField->upscaleHwConfig.opMode.alphaEnable     = 0;  // YUV444 only, ARGB removed
        pUnpackedField->upscaleHwConfig.opMode.aConfig         = 0;  // YUV444 only, ARGB removed

        pUnpackedField->upscaleSwConfig.scaleEnable            = static_cast<UINT32>(pUnpackedField->enable);

        pUnpackedField->upscaleSwConfig.detailEnhanceEnable    = static_cast<UINT8>(pReserveData->enable);
        pUnpackedField->upscaleSwConfig.blendFilter            = static_cast<UINT8>(pReserveData->blend_filter);
        pUnpackedField->upscaleSwConfig.sharpeningStrength1    = static_cast<UINT32>(pReserveData->sharpening_strength1);
        pUnpackedField->upscaleSwConfig.sharpeningStrength2    = static_cast<UINT32>(pReserveData->sharpening_strength2);
        pUnpackedField->upscaleSwConfig.detailEnhanceLimiter   = static_cast<UINT8>(pReserveData->delimiter);
        pUnpackedField->upscaleSwConfig.detailEnhanceClipShift = static_cast<UINT8>(pReserveData->de_clip_shift);
        pUnpackedField->upscaleSwConfig.detailEnhanceCurveT1   = static_cast<UINT16>(pReserveData->decurvet1);
        pUnpackedField->upscaleSwConfig.detailEnhanceCurveT2   = static_cast<UINT16>(pReserveData->decurvet2);
        pUnpackedField->upscaleSwConfig.quietZoneThreshold     = static_cast<UINT16>(pReserveData->tquiet);
        pUnpackedField->upscaleSwConfig.curveRange             = static_cast<UINT16>(pReserveData->curverange);
        pUnpackedField->upscaleSwConfig.comp0FilterMethod      = static_cast<UINT32>(pReserveData->comp0_filter_method);
        pUnpackedField->upscaleSwConfig.comp1and2FilterMethod  = static_cast<UINT32>(pReserveData->comp1_2_filter_method);
    }
    else
    {
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Upscale20Setting::SetUpscaleHwRegistry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Upscale20Setting::SetUpscaleHwRegistry(
    Upscale20UnpackedField*    pUnpackedField)
{
    BOOL result = TRUE;

    UINT32 compROIInWidth[MaxYuvChannels];
    UINT32 compROIInHeight[MaxYuvChannels];
    UINT32 compOutWidth[MaxYuvChannels];
    UINT32 compOutHeight[MaxYuvChannels];
    UINT32 compPhaseStepX[MaxYorUVChannels];
    UINT32 compPhaseStepY[MaxYorUVChannels];
    INT32  compInitialPhaseX[MaxYorUVChannels];
    INT32  compInitialPhaseY[MaxYorUVChannels];
    INT32  compNumLeftExtendedPixels[MaxYorUVChannels];
    INT32  compNumTopExtendedPixels[MaxYorUVChannels];
    INT32  compNumRightExtendedPixels[MaxYorUVChannels];
    INT32  compNumBottomExtendedPixels[MaxYorUVChannels];
    INT32  i32InitPhaseX[MaxYorUVChannels];
    INT32  i32InitPhaseY[MaxYorUVChannels];

    UINT32 chromaSubsampleXFlag = 0;
    UINT32 chromaSubsampleYFlag = 0;

    for (UINT i = 0; i < MaxYuvChannels; i++)
    {
        compROIInWidth[i]  = pUnpackedField->upscaleSwConfig.compROIInWidth[i];
        compROIInHeight[i] = pUnpackedField->upscaleSwConfig.compROIInHeight[i];
        compOutWidth[i]    = pUnpackedField->upscaleSwConfig.outWidth;
        compOutHeight[i]   = pUnpackedField->upscaleSwConfig.outHeight;
    }

    if (compROIInWidth[0] == (compROIInWidth[1] << 1))
    {
        chromaSubsampleXFlag = 1;
    }

    if (compROIInHeight[0] == (compROIInHeight[1] << 1))
    {
        chromaSubsampleYFlag = 1;
    }

    TwoDUpscaleMISCS::cal_phase_step_for_qseed3(compROIInWidth, compROIInHeight,
                                                compOutWidth, compOutHeight,
                                                compPhaseStepX, compPhaseStepY);

    pUnpackedField->upscaleHwConfig.horizontalPhaseStepY  = compPhaseStepX[0];
    pUnpackedField->upscaleHwConfig.horizontalPhaseStepUV = compPhaseStepX[1];
    pUnpackedField->upscaleHwConfig.verticalPhaseStepY    = compPhaseStepY[0];
    pUnpackedField->upscaleHwConfig.verticalPhaseStepUV   = compPhaseStepY[1];

    if ((pUnpackedField->upscaleHwConfig.opMode.enable > 0)                           &&
        (pUnpackedField->upscaleHwConfig.horizontalPhaseStepY > FixPixelUnitScale) &&
        (pUnpackedField->upscaleHwConfig.opMode.directionEnable == 1))
    {
        // DIR_EN needs to be disabled (set as 0) for horizontal downscaling cases
        result = FALSE;
        /// @todo (CAMX-1812) Need to add logging for Common library
    }

    UINT32 inputCompOffset[MaxYorUVChannels]  = { 0, 0 };
    UINT32 outputCompOffset[MaxYorUVChannels] = { 0, 0 };

    TwoDUpscaleMISCS::cal_start_phase_common_qseed3(
        chromaSubsampleXFlag,  chromaSubsampleYFlag,  // the ones after rot90
        // the original, not take rot90 and flip into account
        static_cast<MDP_Chroma_Sample_Site>(pUnpackedField->upscaleSwConfig.chromaSite),
        compPhaseStepX, compPhaseStepY,
        inputCompOffset, outputCompOffset,
        compInitialPhaseX, compInitialPhaseY);

    TwoDUpscaleMISCS::cal_num_extended_pels_for_qseed3(
        pUnpackedField->upscaleSwConfig.scaleEnable,
        compROIInWidth, compROIInHeight,
        compOutWidth, compOutHeight,
        compPhaseStepX, compPhaseStepY,
        compInitialPhaseX, compInitialPhaseY,
        compNumLeftExtendedPixels, compNumRightExtendedPixels,
        compNumTopExtendedPixels, compNumBottomExtendedPixels);

    if ((pUnpackedField->upscaleHwConfig.horizontalPhaseStepY == FixPixelUnitScale) &&
        (pUnpackedField->upscaleHwConfig.verticalPhaseStepY   == FixPixelUnitScale) &&
        ((compROIInWidth[0] > compOutWidth[0]) || (compROIInHeight[0] > compOutHeight[0])))
    {
        // QSEED3 is configurated as a cropper
        pUnpackedField->upscaleHwConfig.phaseInit.horizontalPhaseInitY  = 0;
        pUnpackedField->upscaleHwConfig.phaseInit.horizontalPhaseInitUV = 0;

        pUnpackedField->upscaleHwConfig.phaseInit.verticalPhaseInitY    = 0;
        pUnpackedField->upscaleHwConfig.phaseInit.verticalPhaseInitUV   = 0;
    }
    else
    {
        TwoDUpscaleMISCS::derive_init_phase_for_qseed3(
            compInitialPhaseX, compInitialPhaseY, i32InitPhaseX, i32InitPhaseY);

        pUnpackedField->upscaleHwConfig.phaseInit.horizontalPhaseInitY  = static_cast<UINT32>(i32InitPhaseX[0]);
        pUnpackedField->upscaleHwConfig.phaseInit.horizontalPhaseInitUV = static_cast<UINT32>(i32InitPhaseX[1]);

        pUnpackedField->upscaleHwConfig.phaseInit.verticalPhaseInitY    = static_cast<UINT32>(i32InitPhaseY[0]);
        pUnpackedField->upscaleHwConfig.phaseInit.verticalPhaseInitUV   = static_cast<UINT32>(i32InitPhaseY[1]);
    }

    UINT8  horizontalPreloadY;
    UINT8  verticalPreloadY;
    UINT8  horizontalPreloadUV;
    UINT8  verticalPreloadUV;
    UINT32 inputROIWidth[MaxYorUVChannels];
    UINT32 inputROIHeight[MaxYorUVChannels];
    UINT32 inputROIHorizontalOffset[MaxYorUVChannels];
    UINT32 inputROIVerticalOffset[MaxYorUVChannels];
    UINT8  repeatOnly;

    repeatOnly = 0xF; // repeatonly is true for all four boundaries.

    UINT32 compInHorizontalOffset[MaxYorUVChannels] = { 0, 0 };
    UINT32 compInVerticalOffset[MaxYorUVChannels]   = { 0, 0 };

    TwoDUpscaleMISCS::cal_preload_pels_for_qseed3(
        compNumLeftExtendedPixels,
        compNumRightExtendedPixels,
        compNumTopExtendedPixels,
        compNumBottomExtendedPixels,
        pUnpackedField->upscaleHwConfig.horizontalPhaseStepY,
        pUnpackedField->upscaleHwConfig.verticalPhaseStepY,
        repeatOnly,
        compInHorizontalOffset,
        compInVerticalOffset,
        &(pUnpackedField->upscaleSwConfig.compROIInWidth[0]),
        &(pUnpackedField->upscaleSwConfig.compROIInHeight[0]),
        pUnpackedField->upscaleSwConfig.compROIInWidth,
        pUnpackedField->upscaleSwConfig.compROIInHeight,
        compOutWidth[0],
        compOutHeight[0],
        &horizontalPreloadY,
        &verticalPreloadY,
        &horizontalPreloadUV,
        &verticalPreloadUV,
        inputROIWidth,
        inputROIHeight,
        inputROIHorizontalOffset,
        inputROIVerticalOffset);

    pUnpackedField->upscaleHwConfig.preload.horizontalPreloadY  = horizontalPreloadY; // alpha will be horizontalPreloadY - 2
    pUnpackedField->upscaleHwConfig.preload.verticalPreloadY    = verticalPreloadY; // alpha will be verticalPreloadY - 1

    pUnpackedField->upscaleHwConfig.preload.horizontalPreloadUV = horizontalPreloadUV;
    pUnpackedField->upscaleHwConfig.preload.verticalPreloadUV   = verticalPreloadUV;

    pUnpackedField->upscaleHwConfig.opMode.enable               =
        static_cast<UINT8>(pUnpackedField->upscaleSwConfig.scaleEnable);
    pUnpackedField->upscaleHwConfig.opMode.detailEnhanceEnable  =
        static_cast<UINT8>(pUnpackedField->upscaleSwConfig.detailEnhanceEnable);

    if ((compPhaseStepX[0]                                 <= FixPixelUnitScale) &&
        (pUnpackedField->upscaleSwConfig.comp0FilterMethod == FilterMethodEdYUVRGB))
    {
        pUnpackedField->upscaleHwConfig.opMode.directionEnable = 1;
    }
    else
    {
        // turned off for downscaling cases
        pUnpackedField->upscaleHwConfig.opMode.directionEnable = 0;
    }

    pUnpackedField->upscaleHwConfig.opMode.weightGain = 0;
    pUnpackedField->upscaleHwConfig.opMode.colorSpace = 1;
    pUnpackedField->upscaleHwConfig.opMode.yConfig    = static_cast<UINT8>(
        pUnpackedField->upscaleSwConfig.comp0FilterMethod);

    if (((pUnpackedField->upscaleHwConfig.horizontalPhaseStepY > FixPixelUnitScale) ||
        (pUnpackedField->upscaleHwConfig.verticalPhaseStepY > FixPixelUnitScale))   &&
        (pUnpackedField->upscaleHwConfig.opMode.yConfig == FilterMethodEdYUVRGB))
    {
        // Edge directed method can't be used for downscaling cases, yConfig is updated to separable method
        pUnpackedField->upscaleHwConfig.opMode.yConfig = FilterMethodSeparableYUVRGB;
    }

    pUnpackedField->upscaleHwConfig.opMode.uvConfig = static_cast<UINT8>(
        pUnpackedField->upscaleSwConfig.comp1and2FilterMethod);

    if (((pUnpackedField->upscaleHwConfig.verticalPhaseStepY  != pUnpackedField->upscaleHwConfig.verticalPhaseStepUV)    ||
        (pUnpackedField->upscaleHwConfig.horizontalPhaseStepY != pUnpackedField->upscaleHwConfig.horizontalPhaseStepUV)) &&
        (pUnpackedField->upscaleHwConfig.opMode.yConfig       == FilterMethodEdYUVRGB)                               &&
        (pUnpackedField->upscaleHwConfig.opMode.uvConfig      == FilterMethodEdYUVRGB))
    {
        // Edge directed method can't be used for YUV420/422 formats, uvConfig is updated to separable method for
        // downscaling cases and to circular method for upscaling or unity scaling cases!
        if ((pUnpackedField->upscaleHwConfig.horizontalPhaseStepUV > FixPixelUnitScale) ||
            (pUnpackedField->upscaleHwConfig.verticalPhaseStepUV   > FixPixelUnitScale))
        {
            pUnpackedField->upscaleHwConfig.opMode.uvConfig = FilterMethodSeparableYUVRGB;
        }
        else
        {
            pUnpackedField->upscaleHwConfig.opMode.uvConfig = FilterMethodCircularYUVRGB;
        }
    }

    pUnpackedField->upscaleHwConfig.opMode.bitWidth               = static_cast<UINT8>(
        (pUnpackedField->upscaleSwConfig.bitPrecisionIn == 8) ? 1 : 0);
    pUnpackedField->upscaleHwConfig.opMode.blendConfig            = pUnpackedField->upscaleSwConfig.blendFilter;
    pUnpackedField->upscaleHwConfig.sharpenStrength.sharpenLevel1 = static_cast<INT16>(
        pUnpackedField->upscaleSwConfig.sharpeningStrength1);
    pUnpackedField->upscaleHwConfig.sharpenStrength.sharpenLevel2 = static_cast<INT16>(
        pUnpackedField->upscaleSwConfig.sharpeningStrength2);

    pUnpackedField->upscaleHwConfig.detailEnhanceControl.detailEnhanceLimit =
        pUnpackedField->upscaleSwConfig.detailEnhanceLimiter;
    pUnpackedField->upscaleHwConfig.detailEnhanceControl.detailEnhanceClip  =
        pUnpackedField->upscaleSwConfig.detailEnhanceClipShift;
    pUnpackedField->upscaleHwConfig.detailEnhanceThreshold.thresholdLow     =
        pUnpackedField->upscaleSwConfig.detailEnhanceCurveT1;
    pUnpackedField->upscaleHwConfig.detailEnhanceThreshold.thresholdHigh    =
        pUnpackedField->upscaleSwConfig.detailEnhanceCurveT2;
    pUnpackedField->upscaleHwConfig.curveShape.thresholdQuiet               =
        pUnpackedField->upscaleSwConfig.quietZoneThreshold;
    pUnpackedField->upscaleHwConfig.curveShape.thresholdDieout              =
        pUnpackedField->upscaleSwConfig.curveRange;

    if (pUnpackedField->upscaleSwConfig.detailEnhanceEnable)
    {
        UINT8 precisionBitNum;
        INT16 curveA0;
        INT16 curveB0;
        INT16 curveC0;
        INT16 curveA1;
        INT16 curveB1;
        INT16 curveC1;
        INT16 curveA2;
        INT16 curveB2;
        INT16 curveC2;

        // curve section 1: A0*x^2 + B0*x + C0=y;  passing dots(0,0) and(T1,T1);
        // curve section 2: A1*x^2 + B1*x + C1=y;  passing dots (T1,T1) (T2,T2)
        // curve section 3: A2*x^2 + B2*x + C2=y;  passing dots (T2,T2) (1023, 0)
        TwoDUpscaleMISCS::cal_de_curve_for_qseed3(
            pUnpackedField->upscaleSwConfig.detailEnhanceCurveT1,
            pUnpackedField->upscaleSwConfig.detailEnhanceCurveT2,
            pUnpackedField->upscaleSwConfig.quietZoneThreshold,
            pUnpackedField->upscaleSwConfig.curveRange,
            &precisionBitNum,
            &curveA0,
            &curveB0,
            &curveC0,
            &curveA1,
            &curveB1,
            &curveC1,
            &curveA2,
            &curveB2,
            &curveC2);

        // pass [0, 4] instead of [8, 12] to save some bits.
        pUnpackedField->upscaleHwConfig.detailEnhanceControl.detailEnhancePrecision = static_cast<UINT8>(precisionBitNum - 8);

        pUnpackedField->upscaleHwConfig.curveAs.adjustA0 = curveA0;
        pUnpackedField->upscaleHwConfig.curveBs.adjustB0 = curveB0;
        pUnpackedField->upscaleHwConfig.curveCs.adjustC0 = curveC0;
        pUnpackedField->upscaleHwConfig.curveAs.adjustA1 = curveA1;
        pUnpackedField->upscaleHwConfig.curveBs.adjustB1 = curveB1;
        pUnpackedField->upscaleHwConfig.curveCs.adjustC1 = curveC1;
        pUnpackedField->upscaleHwConfig.curveAs.adjustA2 = curveA2;
        pUnpackedField->upscaleHwConfig.curveBs.adjustB2 = curveB2;
        pUnpackedField->upscaleHwConfig.curveCs.adjustC2 = curveC2;
    }

    UpdateIn  inParam;
    UpdateOut outParam;

    inParam.isInitProgram = true;
    inParam.curYCfg       = pUnpackedField->upscaleHwConfig.opMode.yConfig;
    inParam.curUVCfg      = pUnpackedField->upscaleHwConfig.opMode.uvConfig;
    inParam.curBlndCfg    = pUnpackedField->upscaleHwConfig.opMode.blendConfig;
    inParam.curColorSpace = pUnpackedField->upscaleHwConfig.opMode.colorSpace;
    inParam.blurry_level  = 0;

    inParam.pTwoDFilterA = &(pUnpackedField->upscaleHwConfig.twoDFilterA[0][0]);
    inParam.pTwoDFilterB = &(pUnpackedField->upscaleHwConfig.twoDFilterB[0][0]);
    inParam.pTwoDFilterC = &(pUnpackedField->upscaleHwConfig.twoDFilterC[0][0]);
    inParam.pTwoDFilterD = &(pUnpackedField->upscaleHwConfig.twoDFilterD[0][0]);

    // input: horizontal phase step size [horizontalPhaseStepY, horizontalPhaseStepUV]
    inParam.curPhaseStepHor[0] = pUnpackedField->upscaleHwConfig.horizontalPhaseStepY;
    inParam.curPhaseStepHor[1] = pUnpackedField->upscaleHwConfig.horizontalPhaseStepUV;

    inParam.curPhaseStepVer[0] = pUnpackedField->upscaleHwConfig.verticalPhaseStepY;
    inParam.curPhaseStepVer[1] = pUnpackedField->upscaleHwConfig.verticalPhaseStepUV;

    TwoDUpscaleMISCS::CoeffLUTsUpdate(&inParam, &outParam);

    // data_path setting up
    for (UINT i = 0; i < MaxYuvChannels; i++)
    {
        pUnpackedField->dataPath.inputHeight[i]  = pUnpackedField->upscaleSwConfig.compROIInHeight[i];
        pUnpackedField->dataPath.inputWidth[i]   = pUnpackedField->upscaleSwConfig.compROIInWidth[i];
        pUnpackedField->dataPath.outputHeight[i] = pUnpackedField->upscaleSwConfig.outHeight;
        pUnpackedField->dataPath.outputWidth[i]  = pUnpackedField->upscaleSwConfig.outWidth;
    }

    return result;
}
