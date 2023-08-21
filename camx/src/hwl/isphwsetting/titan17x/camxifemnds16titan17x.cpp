////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifemnds16titan17x.cpp
/// @brief CAMXIFEMNDS16TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifemnds16titan17x.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::IFEMNDS16Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEMNDS16Titan17x::IFEMNDS16Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEMNDS16FDLumaReg) / RegisterWidthInBytes)    +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEMNDS16FDChromaReg) / RegisterWidthInBytes)  +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEMNDS16VideoLumaReg) / RegisterWidthInBytes) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEMNDS16VideoChromaReg) / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        // Write FD output path registers
        if (FDOutput == m_output)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_SCALE_FD_Y_CFG,
                                                  (sizeof(IFEMNDS16FDLumaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.FDLuma));
            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_VFE_SCALE_FD_CBCR_CFG,
                                                      (sizeof(IFEMNDS16FDChromaReg) / RegisterWidthInBytes),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.FDChroma));
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure Scaler FD path Register");
            }
        }

        // Write Video(Full) output path registers
        if (FullOutput == m_output)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_SCALE_VID_Y_CFG,
                                                  (sizeof(IFEMNDS16VideoLumaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.videoLuma));
            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_VFE_SCALE_VID_CBCR_CFG,
                                                      (sizeof(IFEMNDS16VideoChromaReg) / RegisterWidthInBytes),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.videoChroma));

            }
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure Scaler Video(Full) path Register");
            }
        }

        // Write Display(Full) output path registers
        if (DisplayFullOutput == m_output)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_SCALE_DISP_Y_CFG,
                                                  (sizeof(IFEMNDS16DisplayLumaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.displayLuma));
            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_VFE_SCALE_DISP_CBCR_CFG,
                                                      (sizeof(IFEMNDS16DisplayChromaReg) / RegisterWidthInBytes),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.displayChroma));
            }
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure Scaler Display(Full) path Register");
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::CalculateInterpolationResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16Titan17x::CalculateInterpolationResolution(
    UINT32  scaleFactorHorizontal,
    UINT32  scaleFactorVertical,
    UINT32* pHorizontalInterpolationResolution,
    UINT32* pVerticalInterpolationResolution)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pHorizontalInterpolationResolution) &&
        (NULL != pVerticalInterpolationResolution))
    {
        if ((scaleFactorHorizontal < 1) || (scaleFactorVertical < 1))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "Scaler output larger than sensor out, Scale factor clamped to 1x");
            scaleFactorHorizontal = 1;
            scaleFactorVertical   = 1;
        }

        // Constrains from SWI document
        if (scaleFactorHorizontal < 4)
        {
            *pHorizontalInterpolationResolution = 3;
        }
        else if (scaleFactorHorizontal < 8)
        {
            *pHorizontalInterpolationResolution = 2;
        }
        else if (scaleFactorHorizontal < 16)
        {
            *pHorizontalInterpolationResolution = 1;
        }
        else if (scaleFactorHorizontal < MNDS16MaxScaleFactor)
        {
            *pHorizontalInterpolationResolution = 0;
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "scaleFactorHorizontal %d is greater than max supported ratio %d",
                                       scaleFactorHorizontal, MNDS16MaxScaleFactor);
        }

        if (CamxResultSuccess == result)
        {
            // Constrains from SWI document
            if (scaleFactorVertical < 4)
            {
                *pVerticalInterpolationResolution = 3;
            }
            else if (scaleFactorVertical < 8)
            {
                *pVerticalInterpolationResolution = 2;
            }
            else if (scaleFactorVertical < 16)
            {
                *pVerticalInterpolationResolution = 1;
            }
            else if (scaleFactorVertical < MNDS16MaxScaleFactor)
            {
                *pVerticalInterpolationResolution = 0;
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "scaleFactorVertical %d is greater than max supported ratio %d",
                                           scaleFactorVertical, MNDS16MaxScaleFactor);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Null pointer");
        result = CamxResultEInvalidArg;
    }

    return  result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::GetChromaSubsampleFactor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16Titan17x::GetChromaSubsampleFactor(
    FLOAT* pHorizontalSubSampleFactor,
    FLOAT* pVerticalSubsampleFactor)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pHorizontalSubSampleFactor) &&
        (NULL != pVerticalSubsampleFactor))
    {
        switch (m_pixelFormat)
        {
            /// @todo (CAMX-526) Check for all the formats and subsample factors
            case Format::YUV420NV12TP10:
            case Format::YUV420NV21TP10:
            case Format::YUV422NV16TP10:
            case Format::YUV420NV12:
            case Format::YUV420NV21:
            case Format::YUV422NV16:
            case Format::UBWCNV124R:
            case Format::UBWCTP10:
            case Format::PD10:
            case Format::P010:
                *pHorizontalSubSampleFactor = 2.0f;
                *pVerticalSubsampleFactor   = 2.0f;
                break;

            case Format::Y8:
            case Format::Y16:
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupISP, "Incompatible Format: %d",  m_pixelFormat);
                *pHorizontalSubSampleFactor = 1.0f;
                *pVerticalSubsampleFactor   = 1.0f;
                result = CamxResultEInvalidArg;
                break;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Null pointer");
        result = CamxResultEInvalidPointer;
    }

    return  result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::ConfigureFDLumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS16Titan17x::ConfigureFDLumaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution)
{
    IFEMNDS16FDLumaReg* pFDLuma = &m_regCmd.FDLuma;

    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.width, "Invalid MNDS output width");
    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.height, "Invalid MNDS output height");

    pFDLuma->config.bitfields.H_MN_EN                     = TRUE;
    pFDLuma->config.bitfields.V_MN_EN                     = TRUE;
    pFDLuma->horizontalPadding.bitfields.H_SKIP_CNT       = 0;
    pFDLuma->horizontalPadding.bitfields.RIGHT_PAD_EN = 0;
    pFDLuma->horizontalPadding.bitfields.ROUNDING_PATTERN = 0;
     // Programming n means n+1 to the Hw, for all the width and height registers
    pFDLuma->horizontalPadding.bitfields.SCALE_Y_IN_WIDTH = m_pState->inputWidth - 1;
    pFDLuma->horizontalPhase.bitfields.H_INTERP_RESO      = horizontalInterpolationResolution;
    pFDLuma->horizontalPhase.bitfields.H_PHASE_MULT       = (m_pState->inputWidth <<
        (horizontalInterpolationResolution + MNDS16InterpolationShift)) / m_pState->MNDSOutput.dimension.width;

    pFDLuma->horizontalSize.bitfields.H_IN                = m_pState->inputWidth - 1;
    pFDLuma->horizontalSize.bitfields.H_OUT               = m_pState->MNDSOutput.dimension.width - 1;
    pFDLuma->horizontalStripe0.bitfields.H_MN_INIT        = 0;
    pFDLuma->horizontalStripe1.bitfields.H_PHASE_INIT     = 0;

    pFDLuma->verticalPadding.bitfields.BOTTOM_PAD_EN      = 0;
    pFDLuma->verticalPadding.bitfields.ROUNDING_PATTERN   = 0;
    pFDLuma->verticalPadding.bitfields.SCALE_Y_IN_HEIGHT  = m_pState->inputHeight - 1;
    pFDLuma->verticalPadding.bitfields.V_SKIP_CNT         = 0;
    pFDLuma->verticalPhase.bitfields.V_INTERP_RESO        = verticalInterpolationResolution;
    pFDLuma->verticalPhase.bitfields.V_PHASE_MULT         = (m_pState->inputHeight <<
        (verticalInterpolationResolution + MNDS16InterpolationShift)) / m_pState->MNDSOutput.dimension.height;

    pFDLuma->verticalSize.bitfields.V_IN                  = m_pState->inputHeight - 1;
    pFDLuma->verticalSize.bitfields.V_OUT                 = m_pState->MNDSOutput.dimension.height -1;
    pFDLuma->verticalStripe0.bitfields.V_MN_INIT          = 0;
    pFDLuma->verticalStripe1.bitfields.V_PHASE_INIT       = 0;

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {

        pFDLuma->horizontalPadding.bitfields.RIGHT_PAD_EN   = m_pState->MNDSOutput.version16.mnds_config_y.rightPadEnable;
        pFDLuma->horizontalPadding.bitfields.H_SKIP_CNT     = m_pState->MNDSOutput.version16.mnds_config_y.pixelOffset;
        pFDLuma->horizontalStripe0.bitfields.H_MN_INIT      = m_pState->MNDSOutput.version16.mnds_config_y.cntInit;
        pFDLuma->horizontalStripe1.bitfields.H_PHASE_INIT   = m_pState->MNDSOutput.version16.mnds_config_y.phaseInit;
        pFDLuma->horizontalSize.bitfields.H_IN              = m_pState->MNDSOutput.version16.mnds_config_y.input - 1;
        pFDLuma->horizontalSize.bitfields.H_OUT             = m_pState->MNDSOutput.version16.mnds_config_y.output - 1;
        pFDLuma->horizontalPhase.bitfields.H_PHASE_MULT     = m_pState->MNDSOutput.version16.mnds_config_y.phaseStep;
        pFDLuma->horizontalPhase.bitfields.H_INTERP_RESO    = m_pState->MNDSOutput.version16.mnds_config_y.interpReso;

        pFDLuma->horizontalPadding.bitfields.SCALE_Y_IN_WIDTH =
            m_pState->MNDSOutput.version16.mnds_config_y.inputProcessedLength - 1;

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::ConfigureFullLumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS16Titan17x::ConfigureFullLumaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution)
{
    IFEMNDS16VideoLumaReg* pFullLuma = &m_regCmd.videoLuma;

    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.width, "Invalid MNDS output width");
    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.height, "Invalid MNDS output height");

    pFullLuma->config.bitfields.H_MN_EN                     = TRUE;
    pFullLuma->config.bitfields.V_MN_EN                     = TRUE;
    pFullLuma->horizontalPadding.bitfields.H_SKIP_CNT       = 0;
    pFullLuma->horizontalPadding.bitfields.RIGHT_PAD_EN = 0;
    pFullLuma->horizontalPadding.bitfields.ROUNDING_PATTERN = 0;
    // Programming n means n+1 to the Hw, for all the width and height registers
    pFullLuma->horizontalPadding.bitfields.SCALE_Y_IN_WIDTH = m_pState->inputWidth - 1;
    pFullLuma->horizontalPhase.bitfields.H_INTERP_RESO      = horizontalInterpolationResolution;
    pFullLuma->horizontalPhase.bitfields.H_PHASE_MULT       = (m_pState->inputWidth <<
        (horizontalInterpolationResolution + MNDS16InterpolationShift)) / m_pState->MNDSOutput.dimension.width;

    pFullLuma->horizontalSize.bitfields.H_IN                = m_pState->inputWidth - 1;
    pFullLuma->horizontalSize.bitfields.H_OUT               = m_pState->MNDSOutput.dimension.width - 1;
    pFullLuma->horizontalStripe0.bitfields.H_MN_INIT        = 0;
    pFullLuma->horizontalStripe1.bitfields.H_PHASE_INIT     = 0;

    pFullLuma->verticalPadding.bitfields.BOTTOM_PAD_EN      = 0;
    pFullLuma->verticalPadding.bitfields.ROUNDING_PATTERN   = 0;
    pFullLuma->verticalPadding.bitfields.SCALE_Y_IN_HEIGHT  = m_pState->inputHeight - 1;
    pFullLuma->verticalPadding.bitfields.V_SKIP_CNT         = 0;
    pFullLuma->verticalPhase.bitfields.V_INTERP_RESO        = verticalInterpolationResolution;
    pFullLuma->verticalPhase.bitfields.V_PHASE_MULT         = (m_pState->inputHeight <<
        (verticalInterpolationResolution + MNDS16InterpolationShift)) / m_pState->MNDSOutput.dimension.height;

    pFullLuma->verticalSize.bitfields.V_IN                  = m_pState->inputHeight - 1;
    pFullLuma->verticalSize.bitfields.V_OUT                 = m_pState->MNDSOutput.dimension.height - 1;
    pFullLuma->verticalStripe0.bitfields.V_MN_INIT          = 0;
    pFullLuma->verticalStripe1.bitfields.V_PHASE_INIT       = 0;

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {

        pFullLuma->horizontalPadding.bitfields.RIGHT_PAD_EN = m_pState->MNDSOutput.version16.mnds_config_y.rightPadEnable;
        pFullLuma->horizontalPadding.bitfields.H_SKIP_CNT   = m_pState->MNDSOutput.version16.mnds_config_y.pixelOffset;
        pFullLuma->horizontalStripe0.bitfields.H_MN_INIT    = m_pState->MNDSOutput.version16.mnds_config_y.cntInit;
        pFullLuma->horizontalStripe1.bitfields.H_PHASE_INIT = m_pState->MNDSOutput.version16.mnds_config_y.phaseInit;
        pFullLuma->horizontalSize.bitfields.H_IN            = m_pState->MNDSOutput.version16.mnds_config_y.input - 1;
        pFullLuma->horizontalSize.bitfields.H_OUT           = m_pState->MNDSOutput.version16.mnds_config_y.output - 1;
        pFullLuma->horizontalPhase.bitfields.H_PHASE_MULT   = m_pState->MNDSOutput.version16.mnds_config_y.phaseStep;
        pFullLuma->horizontalPhase.bitfields.H_INTERP_RESO  = m_pState->MNDSOutput.version16.mnds_config_y.interpReso;

        pFullLuma->horizontalPadding.bitfields.SCALE_Y_IN_WIDTH =
            m_pState->MNDSOutput.version16.mnds_config_y.inputProcessedLength - 1;

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::ConfigureDisplayLumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS16Titan17x::ConfigureDisplayLumaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution)
{
    IFEMNDS16DisplayLumaReg* pFullLuma = &m_regCmd.displayLuma;

    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.width, "Invalid MNDS output width");
    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.height, "Invalid MNDS output height");

    pFullLuma->config.bitfields.H_MN_EN                     = TRUE;
    pFullLuma->config.bitfields.V_MN_EN                     = TRUE;
    pFullLuma->horizontalPadding.bitfields.H_SKIP_CNT       = 0;
    pFullLuma->horizontalPadding.bitfields.RIGHT_PAD_EN = 0;
    pFullLuma->horizontalPadding.bitfields.ROUNDING_PATTERN = 0;
    // Programming n means n+1 to the Hw, for all the width and height registers
    pFullLuma->horizontalPadding.bitfields.SCALE_Y_IN_WIDTH = m_pState->inputWidth - 1;
    pFullLuma->horizontalPhase.bitfields.H_INTERP_RESO      = horizontalInterpolationResolution;
    pFullLuma->horizontalPhase.bitfields.H_PHASE_MULT       = (m_pState->inputWidth <<
        (horizontalInterpolationResolution + MNDS16InterpolationShift)) / m_pState->MNDSOutput.dimension.width;

    pFullLuma->horizontalSize.bitfields.H_IN                = m_pState->inputWidth - 1;
    pFullLuma->horizontalSize.bitfields.H_OUT               = m_pState->MNDSOutput.dimension.width - 1;
    pFullLuma->horizontalStripe0.bitfields.H_MN_INIT        = 0;
    pFullLuma->horizontalStripe1.bitfields.H_PHASE_INIT     = 0;

    pFullLuma->verticalPadding.bitfields.BOTTOM_PAD_EN      = 0;
    pFullLuma->verticalPadding.bitfields.ROUNDING_PATTERN   = 0;
    pFullLuma->verticalPadding.bitfields.SCALE_Y_IN_HEIGHT  = m_pState->inputHeight - 1;
    pFullLuma->verticalPadding.bitfields.V_SKIP_CNT         = 0;
    pFullLuma->verticalPhase.bitfields.V_INTERP_RESO        = verticalInterpolationResolution;
    pFullLuma->verticalPhase.bitfields.V_PHASE_MULT         = (m_pState->inputHeight <<
        (verticalInterpolationResolution + MNDS16InterpolationShift)) / m_pState->MNDSOutput.dimension.height;

    pFullLuma->verticalSize.bitfields.V_IN                  = m_pState->inputHeight - 1;
    pFullLuma->verticalSize.bitfields.V_OUT                 = m_pState->MNDSOutput.dimension.height - 1;
    pFullLuma->verticalStripe0.bitfields.V_MN_INIT          = 0;
    pFullLuma->verticalStripe1.bitfields.V_PHASE_INIT       = 0;


    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        pFullLuma->horizontalPadding.bitfields.RIGHT_PAD_EN = m_pState->MNDSOutput.version16.mnds_config_y.rightPadEnable;
        pFullLuma->horizontalPadding.bitfields.H_SKIP_CNT   = m_pState->MNDSOutput.version16.mnds_config_y.pixelOffset;
        pFullLuma->horizontalStripe0.bitfields.H_MN_INIT    = m_pState->MNDSOutput.version16.mnds_config_y.cntInit;
        pFullLuma->horizontalStripe1.bitfields.H_PHASE_INIT = m_pState->MNDSOutput.version16.mnds_config_y.phaseInit;
        pFullLuma->horizontalSize.bitfields.H_IN            = m_pState->MNDSOutput.version16.mnds_config_y.input - 1;
        pFullLuma->horizontalSize.bitfields.H_OUT           = m_pState->MNDSOutput.version16.mnds_config_y.output - 1;
        pFullLuma->horizontalPhase.bitfields.H_PHASE_MULT   = m_pState->MNDSOutput.version16.mnds_config_y.phaseStep;
        pFullLuma->horizontalPhase.bitfields.H_INTERP_RESO  = m_pState->MNDSOutput.version16.mnds_config_y.interpReso;

        pFullLuma->horizontalPadding.bitfields.SCALE_Y_IN_WIDTH =
            m_pState->MNDSOutput.version16.mnds_config_y.inputProcessedLength - 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::ConfigureFDChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS16Titan17x::ConfigureFDChromaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution,
    FLOAT  horizontalSubSampleFactor,
    FLOAT  verticalSubsampleFactor)
{
    IFEMNDS16FDChromaReg* pFDChroma = &m_regCmd.FDChroma;

    CAMX_ASSERT_MESSAGE(0 != horizontalSubSampleFactor, "Invalid horizontalSubSampleFactor");
    CAMX_ASSERT_MESSAGE(0 != verticalSubsampleFactor, "Invalid verticalSubsampleFactor");

    pFDChroma->config.bitfields.H_MN_EN                        = TRUE;
    pFDChroma->config.bitfields.V_MN_EN                        = TRUE;
    pFDChroma->horizontalPadding.bitfields.H_SKIP_CNT          = 0;
    pFDChroma->horizontalPadding.bitfields.RIGHT_PAD_EN = 0;
    pFDChroma->horizontalPadding.bitfields.ROUNDING_PATTERN    = 0;
    // Programming n means n+1 to the Hw, for all the width and height registers
    pFDChroma->horizontalPadding.bitfields.SCALE_CBCR_IN_WIDTH = m_pState->inputWidth - 1;
    pFDChroma->horizontalSize.bitfields.H_IN                   = m_pState->inputWidth - 1;
    pFDChroma->horizontalSize.bitfields.H_OUT                  =
        static_cast<UINT32> (m_pState->MNDSOutput.dimension.width / horizontalSubSampleFactor) - 1;
    pFDChroma->horizontalPhase.bitfields.H_INTERP_RESO         = horizontalInterpolationResolution;

    // Add 1 to H_OUT as values of hIn, hOut, vIn and vOut are decreased by 1, Value of n means n+1 in hardware
    pFDChroma->horizontalPhase.bitfields.H_PHASE_MULT          =
        (m_pState->inputWidth << (horizontalInterpolationResolution + MNDS16InterpolationShift)) /
        (pFDChroma->horizontalSize.bitfields.H_OUT + 1);

    pFDChroma->horizontalStripe0.bitfields.H_MN_INIT           = 0;
    pFDChroma->horizontalStripe1.bitfields.H_PHASE_INIT        = 0;
    pFDChroma->verticalPadding.bitfields.BOTTOM_PAD_EN         = 0;
    pFDChroma->verticalPadding.bitfields.ROUNDING_PATTERN      = 0;
    pFDChroma->verticalPadding.bitfields.SCALE_CBCR_IN_HEIGHT  = m_pState->inputHeight - 1;
    pFDChroma->verticalPadding.bitfields.V_SKIP_CNT            = 0;
    pFDChroma->verticalSize.bitfields.V_IN                     = m_pState->inputHeight -1;
    pFDChroma->verticalSize.bitfields.V_OUT                    =
        static_cast<UINT32>(m_pState->MNDSOutput.dimension.height / verticalSubsampleFactor) -1;
    pFDChroma->verticalPhase.bitfields.V_INTERP_RESO           = verticalInterpolationResolution;

    // Add 1 to V_OUT as values of hIn, hOut, vIn and vOut are decreased by 1, Value of n means n+1 in hardware
    pFDChroma->verticalPhase.bitfields.V_PHASE_MULT            =
        (m_pState->inputHeight << (verticalInterpolationResolution + MNDS16InterpolationShift)) /
        (m_regCmd.FDChroma.verticalSize.bitfields.V_OUT + 1);
    pFDChroma->verticalStripe0.bitfields.V_MN_INIT             = 0;
    pFDChroma->verticalStripe1.bitfields.V_PHASE_INIT          = 0;

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        pFDChroma->horizontalPadding.bitfields.RIGHT_PAD_EN = m_pState->MNDSOutput.version16.mnds_config_c.rightPadEnable;
        pFDChroma->horizontalPadding.bitfields.H_SKIP_CNT   = m_pState->MNDSOutput.version16.mnds_config_c.pixelOffset;
        pFDChroma->horizontalStripe0.bitfields.H_MN_INIT    = m_pState->MNDSOutput.version16.mnds_config_c.cntInit;
        pFDChroma->horizontalStripe1.bitfields.H_PHASE_INIT = m_pState->MNDSOutput.version16.mnds_config_c.phaseInit;
        pFDChroma->horizontalSize.bitfields.H_IN            = m_pState->MNDSOutput.version16.mnds_config_c.input - 1;
        pFDChroma->horizontalSize.bitfields.H_OUT           =
            static_cast<UINT32> (m_pState->MNDSOutput.version16.mnds_config_c.output) - 1;
        // Add 1 to H_OUT as values of hIn, hOut, vIn and vOut are decreased by 1, Value of n means n+1 in hardware
        pFDChroma->horizontalPhase.bitfields.H_PHASE_MULT   = m_pState->MNDSOutput.version16.mnds_config_c.phaseStep;
        pFDChroma->horizontalPhase.bitfields.H_INTERP_RESO  = m_pState->MNDSOutput.version16.mnds_config_c.interpReso;

        pFDChroma->horizontalPadding.bitfields.SCALE_CBCR_IN_WIDTH    =
            m_pState->MNDSOutput.version16.mnds_config_c.inputProcessedLength - 1;

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::ConfigureFullChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS16Titan17x::ConfigureFullChromaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution,
    FLOAT  horizontalSubSampleFactor,
    FLOAT  verticalSubsampleFactor)
{
    IFEMNDS16VideoChromaReg* pFullChroma = &m_regCmd.videoChroma;

    CAMX_ASSERT_MESSAGE(0 != horizontalSubSampleFactor, "Invalid horizontalSubSampleFactor");
    CAMX_ASSERT_MESSAGE(0 != verticalSubsampleFactor, "Invalid verticalSubsampleFactor");

    pFullChroma->config.bitfields.H_MN_EN                           = TRUE;
    pFullChroma->config.bitfields.V_MN_EN                           = TRUE;
    pFullChroma->horizontalPadding.bitfields.H_SKIP_CNT             = 0;
    pFullChroma->horizontalPadding.bitfields.RIGHT_PAD_EN = 0;
    pFullChroma->horizontalPadding.bitfields.ROUNDING_PATTERN       = 0;
    // Programming n means n+1 to the Hw, for all the width and height registers
    pFullChroma->horizontalPadding.bitfields.SCALE_CBCR_IN_WIDTH    = m_pState->inputWidth - 1;
    pFullChroma->horizontalSize.bitfields.H_IN                      = m_pState->inputWidth - 1;
    pFullChroma->horizontalSize.bitfields.H_OUT                     =
        static_cast<UINT32> (m_pState->MNDSOutput.dimension.width / horizontalSubSampleFactor) - 1;
    pFullChroma->horizontalPhase.bitfields.H_INTERP_RESO            = horizontalInterpolationResolution;

    // Add 1 to H_OUT as values of hIn, hOut, vIn and vOut are decreased by 1, Value of n means n+1 in hardware
    pFullChroma->horizontalPhase.bitfields.H_PHASE_MULT             =
        (m_pState->inputWidth << (horizontalInterpolationResolution + MNDS16InterpolationShift)) /
        (pFullChroma->horizontalSize.bitfields.H_OUT + 1);

    pFullChroma->horizontalStripe0.bitfields.H_MN_INIT              = 0;
    pFullChroma->horizontalStripe1.bitfields.H_PHASE_INIT           = 0;
    pFullChroma->verticalPadding.bitfields.BOTTOM_PAD_EN            = 0;
    pFullChroma->verticalPadding.bitfields.ROUNDING_PATTERN         = 0;
    pFullChroma->verticalPadding.bitfields.SCALE_CBCR_IN_HEIGHT     = m_pState->inputHeight - 1;
    pFullChroma->verticalPadding.bitfields.V_SKIP_CNT               = 0;
    pFullChroma->verticalSize.bitfields.V_IN                        = m_pState->inputHeight -1;
    pFullChroma->verticalSize.bitfields.V_OUT                       =
        static_cast<UINT32>(m_pState->MNDSOutput.dimension.height / verticalSubsampleFactor) -1;
    pFullChroma->verticalPhase.bitfields.V_INTERP_RESO              = verticalInterpolationResolution;

    // Add 1 to V_OUT as values of hIn, hOut, vIn and vOut are decreased by 1, Value of n means n+1 in hardware
    pFullChroma->verticalPhase.bitfields.V_PHASE_MULT               =
        (m_pState->inputHeight << (verticalInterpolationResolution + MNDS16InterpolationShift)) /
        (m_regCmd.videoChroma.verticalSize.bitfields.V_OUT + 1);
    pFullChroma->verticalStripe0.bitfields.V_MN_INIT                = 0;
    pFullChroma->verticalStripe1.bitfields.V_PHASE_INIT             = 0;

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        pFullChroma->horizontalPadding.bitfields.RIGHT_PAD_EN = m_pState->MNDSOutput.version16.mnds_config_c.rightPadEnable;
        pFullChroma->horizontalPadding.bitfields.H_SKIP_CNT     = m_pState->MNDSOutput.version16.mnds_config_c.pixelOffset;
        pFullChroma->horizontalStripe0.bitfields.H_MN_INIT      = m_pState->MNDSOutput.version16.mnds_config_c.cntInit;
        pFullChroma->horizontalStripe1.bitfields.H_PHASE_INIT   = m_pState->MNDSOutput.version16.mnds_config_c.phaseInit;
        pFullChroma->horizontalSize.bitfields.H_IN              = m_pState->MNDSOutput.version16.mnds_config_c.input - 1;
        pFullChroma->horizontalSize.bitfields.H_OUT             =
            static_cast<UINT32>(m_pState->MNDSOutput.version16.mnds_config_c.output)- 1;
        // Add 1 to H_OUT as values of hIn, hOut, vIn and vOut are decreased by 1, Value of n means n+1 in hardware
        pFullChroma->horizontalPhase.bitfields.H_PHASE_MULT     = m_pState->MNDSOutput.version16.mnds_config_c.phaseStep;
        pFullChroma->horizontalPhase.bitfields.H_INTERP_RESO    = m_pState->MNDSOutput.version16.mnds_config_c.interpReso;

        pFullChroma->horizontalPadding.bitfields.SCALE_CBCR_IN_WIDTH =
            m_pState->MNDSOutput.version16.mnds_config_c.inputProcessedLength - 1;

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::ConfigureDisplayChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS16Titan17x::ConfigureDisplayChromaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution,
    FLOAT  horizontalSubSampleFactor,
    FLOAT  verticalSubsampleFactor)
{
    IFEMNDS16DisplayChromaReg* pFullChroma = &m_regCmd.displayChroma;

    CAMX_ASSERT_MESSAGE(0 != horizontalSubSampleFactor, "Invalid horizontalSubSampleFactor");
    CAMX_ASSERT_MESSAGE(0 != verticalSubsampleFactor, "Invalid verticalSubsampleFactor");

    pFullChroma->config.bitfields.H_MN_EN                           = TRUE;
    pFullChroma->config.bitfields.V_MN_EN                           = TRUE;
    pFullChroma->horizontalPadding.bitfields.H_SKIP_CNT             = 0;
    pFullChroma->horizontalPadding.bitfields.RIGHT_PAD_EN = 0;
    pFullChroma->horizontalPadding.bitfields.ROUNDING_PATTERN       = 0;
    // Programming n means n+1 to the Hw, for all the width and height registers
    pFullChroma->horizontalPadding.bitfields.SCALE_CBCR_IN_WIDTH    = m_pState->inputWidth - 1;
    pFullChroma->horizontalSize.bitfields.H_IN                      = m_pState->inputWidth - 1;
    pFullChroma->horizontalSize.bitfields.H_OUT                     =
        static_cast<UINT32> (m_pState->MNDSOutput.dimension.width / horizontalSubSampleFactor) - 1;
    pFullChroma->horizontalPhase.bitfields.H_INTERP_RESO            = horizontalInterpolationResolution;

    // Add 1 to H_OUT as values of hIn, hOut, vIn and vOut are decreased by 1, Value of n means n+1 in hardware
    pFullChroma->horizontalPhase.bitfields.H_PHASE_MULT             =
        (m_pState->inputWidth << (horizontalInterpolationResolution + MNDS16InterpolationShift)) /
        (pFullChroma->horizontalSize.bitfields.H_OUT + 1);

    pFullChroma->horizontalStripe0.bitfields.H_MN_INIT              = 0;
    pFullChroma->horizontalStripe1.bitfields.H_PHASE_INIT           = 0;
    pFullChroma->verticalPadding.bitfields.BOTTOM_PAD_EN            = 0;
    pFullChroma->verticalPadding.bitfields.ROUNDING_PATTERN         = 0;
    pFullChroma->verticalPadding.bitfields.SCALE_CBCR_IN_HEIGHT     = m_pState->inputHeight - 1;
    pFullChroma->verticalPadding.bitfields.V_SKIP_CNT               = 0;
    pFullChroma->verticalSize.bitfields.V_IN                        = m_pState->inputHeight -1;
    pFullChroma->verticalSize.bitfields.V_OUT                       =
        static_cast<UINT32>(m_pState->MNDSOutput.dimension.height / verticalSubsampleFactor) -1;
    pFullChroma->verticalPhase.bitfields.V_INTERP_RESO              = verticalInterpolationResolution;

    // Add 1 to V_OUT as values of hIn, hOut, vIn and vOut are decreased by 1, Value of n means n+1 in hardware
    pFullChroma->verticalPhase.bitfields.V_PHASE_MULT               =
        (m_pState->inputHeight << (verticalInterpolationResolution + MNDS16InterpolationShift)) /
        (m_regCmd.displayChroma.verticalSize.bitfields.V_OUT + 1);
    pFullChroma->verticalStripe0.bitfields.V_MN_INIT                = 0;
    pFullChroma->verticalStripe1.bitfields.V_PHASE_INIT             = 0;

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        pFullChroma->horizontalPadding.bitfields.RIGHT_PAD_EN   = m_pState->MNDSOutput.version16.mnds_config_c.rightPadEnable;
        pFullChroma->horizontalPadding.bitfields.H_SKIP_CNT     = m_pState->MNDSOutput.version16.mnds_config_c.pixelOffset;
        pFullChroma->horizontalStripe0.bitfields.H_MN_INIT      = m_pState->MNDSOutput.version16.mnds_config_c.cntInit;
        pFullChroma->horizontalStripe1.bitfields.H_PHASE_INIT   = m_pState->MNDSOutput.version16.mnds_config_c.phaseInit;
        pFullChroma->horizontalSize.bitfields.H_IN              = m_pState->MNDSOutput.version16.mnds_config_c.input - 1;
        pFullChroma->horizontalSize.bitfields.H_OUT             =
            static_cast<UINT32> (m_pState->MNDSOutput.version16.mnds_config_c.output)- 1;
        // Add 1 to H_OUT as values of hIn, hOut, vIn and vOut are decreased by 1, Value of n means n+1 in hardware
        pFullChroma->horizontalPhase.bitfields.H_PHASE_MULT     = m_pState->MNDSOutput.version16.mnds_config_c.phaseStep;
        pFullChroma->horizontalPhase.bitfields.H_INTERP_RESO    = m_pState->MNDSOutput.version16.mnds_config_c.interpReso;

        pFullChroma->horizontalPadding.bitfields.SCALE_CBCR_IN_WIDTH =
            m_pState->MNDSOutput.version16.mnds_config_c.inputProcessedLength - 1;

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result        = CamxResultSuccess;
    ISPInputData*       pData         = static_cast<ISPInputData*>(pInput);
    MNDSOutputData*     pOutputData   = static_cast<MNDSOutputData*>(pOutput);

    UINT32  scaleFactorHorizontal;                    // Horizontal scale factor
    UINT32  scaleFactorVertical;                      // vertical scale factor
    UINT32  horizontalInterpolationResolution = 1;    // horizontal interpolation resolution
    UINT32  verticalInterpolationResolution   = 1;    // vertical interpolation resolution
    FLOAT   horizontalSubSampleFactor         = 1.0f; // Horizontal chroma subsampling factor
    FLOAT   verticalSubsampleFactor           = 1.0f; // Verical chroma subsampling factor

    if ((NULL != pOutputData) && (NULL != pData))
    {
        m_pState      = pOutputData->pMNDSState;
        m_output      = pOutputData->ifeOutputPath;
        m_pixelFormat = pOutputData->pixelFormat;
        m_pInputData  = pData;

        CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.width, "Invalid MNDS output width");
        CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.height, "Invalid MNDS output height");

        // Calculate the interpolation resolution based on the scaling ratio
        scaleFactorHorizontal = m_pState->inputWidth  / m_pState->MNDSOutput.dimension.width;
        scaleFactorVertical   = m_pState->inputHeight / m_pState->MNDSOutput.dimension.height;

        result = CalculateInterpolationResolution(scaleFactorHorizontal,
                                                  scaleFactorVertical,
                                                  &horizontalInterpolationResolution,
                                                  &verticalInterpolationResolution);

        // Luma register configuration
        if (CamxResultSuccess == result)
        {
            if (FDOutput == m_output)
            {
                ConfigureFDLumaRegisters(horizontalInterpolationResolution, verticalInterpolationResolution);
            }
            else if (DisplayFullOutput == m_output)
            {
                ConfigureDisplayLumaRegisters(horizontalInterpolationResolution, verticalInterpolationResolution);
            }
            else
            {
                ConfigureFullLumaRegisters(horizontalInterpolationResolution, verticalInterpolationResolution);
            }
        }

        // Chroma register configuration
        if (CamxResultSuccess == result)
        {
            // Get the chroma channel subsample factor
            result = GetChromaSubsampleFactor(&horizontalSubSampleFactor, &verticalSubsampleFactor);

            if (CamxResultSuccess == result)
            {
                // Calculate the interpolation resolution based on the Chroma scaling ratio
                scaleFactorHorizontal = static_cast<UINT32>((m_pState->inputWidth * horizontalSubSampleFactor) /
                                                             m_pState->MNDSOutput.dimension.width);
                scaleFactorVertical   = static_cast<UINT32>((m_pState->inputHeight * verticalSubsampleFactor) /
                                                             m_pState->MNDSOutput.dimension.height);
                result = CalculateInterpolationResolution(scaleFactorHorizontal,
                                                          scaleFactorVertical,
                                                          &horizontalInterpolationResolution,
                                                          &verticalInterpolationResolution);
            }
        }

        if (CamxResultSuccess == result)
        {
            if (FDOutput == m_output)
            {
                ConfigureFDChromaRegisters(horizontalInterpolationResolution,
                        verticalInterpolationResolution,
                        horizontalSubSampleFactor,
                        verticalSubsampleFactor);
            }
            else if (DisplayFullOutput == m_output)
            {
                ConfigureDisplayChromaRegisters(horizontalInterpolationResolution,
                        verticalInterpolationResolution,
                        horizontalSubSampleFactor,
                        verticalSubsampleFactor);
            }
            else
            {
                ConfigureFullChromaRegisters(horizontalInterpolationResolution,
                        verticalInterpolationResolution,
                        horizontalSubSampleFactor,
                        verticalSubsampleFactor);
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p pOutputData=0x%p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS16Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEMNDS16Titan17x::CopyRegCmd(
    VOID* pData)
{
    UINT32 dataCopied = 0;

    if (NULL != pData)
    {
        Utils::Memcpy(pData, &m_regCmd, sizeof(m_regCmd));
        dataCopied = sizeof(m_regCmd);
    }

    return dataCopied;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::~IFEMNDS16Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEMNDS16Titan17x::~IFEMNDS16Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS16Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS16Titan17x::DumpRegConfig()
{
    if (FDOutput == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma PAD_EN        [%d] ",
            m_regCmd.FDLuma.horizontalPadding.bitfields.RIGHT_PAD_EN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma H_SKIP_CNT    [%d] ",
            m_regCmd.FDLuma.horizontalPadding.bitfields.H_SKIP_CNT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma H_MN_INIT     [%d] ",
            m_regCmd.FDLuma.horizontalStripe0.bitfields.H_MN_INIT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma H_PHASE_INIT  [%d] ",
            m_regCmd.FDLuma.horizontalStripe1.bitfields.H_PHASE_INIT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma H_IN          [%d] ",
            m_regCmd.FDLuma.horizontalSize.bitfields.H_IN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma H_OUT         [%d] ",
            m_regCmd.FDLuma.horizontalSize.bitfields.H_OUT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma V_IN          [%d] ",
            m_regCmd.FDLuma.verticalSize.bitfields.V_IN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma V_OUT         [%d] ",
            m_regCmd.FDLuma.verticalSize.bitfields.V_OUT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma SCALE_Y_IN_WIDTH      [%d] ",
            m_regCmd.FDLuma.horizontalPadding.bitfields.SCALE_Y_IN_WIDTH);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma H_PHASE_MULT  [%d] ",
            m_regCmd.FDLuma.horizontalPhase.bitfields.H_PHASE_MULT);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma Config            [0x%x] ", m_regCmd.FDLuma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma horizontalSize    [0x%x] ", m_regCmd.FDLuma.horizontalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma horizontalPhase   [0x%x] ", m_regCmd.FDLuma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma horizontalStripe0 [0x%x] ", m_regCmd.FDLuma.horizontalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma horizontalStripe1 [0x%x] ", m_regCmd.FDLuma.horizontalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma horizontalPadding [0x%x] ", m_regCmd.FDLuma.horizontalPadding);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma verticalSize      [0x%x] ", m_regCmd.FDLuma.verticalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma verticalPhase     [0x%x] ", m_regCmd.FDLuma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma verticalStripe0   [0x%x] ", m_regCmd.FDLuma.verticalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma verticalStripe1   [0x%x] ", m_regCmd.FDLuma.verticalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma verticalPadding   [0x%x] ", m_regCmd.FDLuma.verticalPadding);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma Config            [0x%x] ", m_regCmd.FDChroma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma horizontalSize    [0x%x] ", m_regCmd.FDChroma.horizontalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma horizontalPhase   [0x%x] ", m_regCmd.FDChroma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma horizontalStripe0 [0x%x] ", m_regCmd.FDChroma.horizontalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma horizontalStripe1 [0x%x] ", m_regCmd.FDChroma.horizontalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma horizontalPadding [0x%x] ", m_regCmd.FDChroma.horizontalPadding);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma verticalSize      [0x%x] ", m_regCmd.FDChroma.verticalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma verticalPhase     [0x%x] ", m_regCmd.FDChroma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma verticalStripe0   [0x%x] ", m_regCmd.FDChroma.verticalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma verticalStripe1   [0x%x] ", m_regCmd.FDChroma.verticalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma verticalPadding   [0x%x] ", m_regCmd.FDChroma.verticalPadding);
    }

    if (FullOutput == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma PAD_EN        [%d] ",
            m_regCmd.videoLuma.horizontalPadding.bitfields.RIGHT_PAD_EN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma H_SKIP_CNT    [%d] ",
            m_regCmd.videoLuma.horizontalPadding.bitfields.H_SKIP_CNT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma H_MN_INIT     [%d] ",
            m_regCmd.videoLuma.horizontalStripe0.bitfields.H_MN_INIT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma H_PHASE_INIT  [%d] ",
            m_regCmd.videoLuma.horizontalStripe1.bitfields.H_PHASE_INIT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma H_IN          [%d] ",
            m_regCmd.videoLuma.horizontalSize.bitfields.H_IN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma H_OUT         [%d] ",
            m_regCmd.videoLuma.horizontalSize.bitfields.H_OUT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma V_IN          [%d] ",
            m_regCmd.videoLuma.verticalSize.bitfields.V_IN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma V_OUT         [%d] ",
            m_regCmd.videoLuma.verticalSize.bitfields.V_OUT);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma SCALE_Y_IN_WIDTH      [%d] ",
            m_regCmd.videoLuma.horizontalPadding.bitfields.SCALE_Y_IN_WIDTH);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma H_PHASE_MULT  [%d] ",
            m_regCmd.videoLuma.horizontalPhase.bitfields.H_PHASE_MULT);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma Config            [0x%x] ", m_regCmd.videoLuma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma horizontalSize    [0x%x] ", m_regCmd.videoLuma.horizontalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma horizontalPhase   [0x%x] ", m_regCmd.videoLuma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma horizontalStripe0 [0x%x] ", m_regCmd.videoLuma.horizontalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma horizontalStripe1 [0x%x] ", m_regCmd.videoLuma.horizontalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma horizontalPadding [0x%x] ", m_regCmd.videoLuma.horizontalPadding);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma verticalSize      [0x%x] ", m_regCmd.videoLuma.verticalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma verticalPhase     [0x%x] ", m_regCmd.videoLuma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma verticalStripe0   [0x%x] ", m_regCmd.videoLuma.verticalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma verticalStripe1   [0x%x] ", m_regCmd.videoLuma.verticalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma verticalPadding   [0x%x] ", m_regCmd.videoLuma.verticalPadding);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma Config            [0x%x] ", m_regCmd.videoChroma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma horizontalSize    [0x%x] ", m_regCmd.videoChroma.horizontalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma horizontalPhase   [0x%x] ", m_regCmd.videoChroma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
            "MNDS Full Chroma horizontalStripe0 [0x%x] ", m_regCmd.videoChroma.horizontalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
            "MNDS Full Chroma horizontalStripe1 [0x%x] ", m_regCmd.videoChroma.horizontalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod,
            "MNDS Full Chroma horizontalPadding [0x%x] ", m_regCmd.videoChroma.horizontalPadding);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma verticalSize      [0x%x] ", m_regCmd.videoChroma.verticalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma verticalPhase     [0x%x] ", m_regCmd.videoChroma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma verticalStripe0   [0x%x] ", m_regCmd.videoChroma.verticalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma verticalStripe1   [0x%x] ", m_regCmd.videoChroma.verticalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma verticalPadding   [0x%x] ", m_regCmd.videoChroma.verticalPadding);
    }

    if (DisplayFullOutput == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma PAD_EN        [%d] ",
            m_regCmd.displayLuma.horizontalPadding.bitfields.RIGHT_PAD_EN);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma H_SKIP_CNT    [%d] ",
            m_regCmd.displayLuma.horizontalPadding.bitfields.H_SKIP_CNT);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma H_MN_INIT     [%d] ",
            m_regCmd.displayLuma.horizontalStripe0.bitfields.H_MN_INIT);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma H_PHASE_INIT  [%d] ",
            m_regCmd.displayLuma.horizontalStripe1.bitfields.H_PHASE_INIT);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma H_IN          [%d] ",
            m_regCmd.displayLuma.horizontalSize.bitfields.H_IN);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma H_OUT         [%d] ",
            m_regCmd.displayLuma.horizontalSize.bitfields.H_OUT);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma V_IN          [%d] ",
            m_regCmd.displayLuma.verticalSize.bitfields.V_IN);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma V_OUT         [%d] ",
            m_regCmd.displayLuma.verticalSize.bitfields.V_OUT);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma SCALE_Y_IN_WIDTH      [%d] ",
            m_regCmd.displayLuma.horizontalPadding.bitfields.SCALE_Y_IN_WIDTH);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma H_PHASE_MULT  [%d] ",
            m_regCmd.displayLuma.horizontalPhase.bitfields.H_PHASE_MULT);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma Config            [0x%x]", m_regCmd.displayLuma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma horizontalSize    [0x%x]", m_regCmd.displayLuma.horizontalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma horizontalPhase   [0x%x]", m_regCmd.displayLuma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma horizontalStripe0 [0x%x]", m_regCmd.displayLuma.horizontalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma horizontalStripe1 [0x%x]", m_regCmd.displayLuma.horizontalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma horizontalPadding [0x%x]", m_regCmd.displayLuma.horizontalPadding);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma verticalSize      [0x%x]", m_regCmd.displayLuma.verticalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma verticalPhase     [0x%x]", m_regCmd.displayLuma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma verticalStripe0   [0x%x]", m_regCmd.displayLuma.verticalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma verticalStripe1   [0x%x]", m_regCmd.displayLuma.verticalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "MNDS Disp Luma verticalPadding   [0x%x]", m_regCmd.displayLuma.verticalPadding);

        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma Config            [0x%x]", m_regCmd.displayChroma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma horizontalSize    [0x%x]", m_regCmd.displayChroma.horizontalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma horizontalPhase   [0x%x]", m_regCmd.displayChroma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma horizontalStripe0 [0x%x]", m_regCmd.displayChroma.horizontalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma horizontalStripe1 [0x%x]", m_regCmd.displayChroma.horizontalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma horizontalPadding [0x%x]", m_regCmd.displayChroma.horizontalPadding);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma verticalSize      [0x%x]", m_regCmd.displayChroma.verticalSize);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma verticalPhase     [0x%x]", m_regCmd.displayChroma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma verticalStripe0   [0x%x]", m_regCmd.displayChroma.verticalStripe0);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma verticalStripe1   [0x%x]", m_regCmd.displayChroma.verticalStripe1);
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "MNDS Display Chroma verticalPadding   [0x%x]", m_regCmd.displayChroma.verticalPadding);
    }
}

CAMX_NAMESPACE_END
