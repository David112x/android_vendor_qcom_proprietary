////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifemnds21titan480.cpp
/// @brief CAMXIFEMNDS21TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifemnds21titan480.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::IFEMNDS21Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEMNDS21Titan480::IFEMNDS21Titan480()
{
    SetCommandLength(
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEMNDS21FDLumaReg)      / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEMNDS21FDChromaReg)    / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEMNDS21VideoLumaReg)   / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEMNDS21VideoChromaReg) / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEMNDS21DispLumaReg)    / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEMNDS21DispChromaReg)  / RegisterWidthInBytes));

    // Hardcode initial value for all the registers
    m_regCmd.FDLuma.module.u32All                  = 0x00000101;
    m_regCmd.FDLuma.config.u32All                  = 0x00000600;
    m_regCmd.FDLuma.imageSize.u32All               = 0x077f0437;
    m_regCmd.FDLuma.horizontalPhase.u32All         = 0xc0200000;
    m_regCmd.FDLuma.horizontalStripe.u32All        = 0x00000000;
    m_regCmd.FDLuma.verticalPhase.u32All           = 0xc0200000;
    m_regCmd.FDLuma.verticalStripe.u32All          = 0x00000000;
    m_regCmd.FDLuma.cropLine.u32All                = 0x00000437;
    m_regCmd.FDLuma.cropPixel.u32All               = 0x0000077f;

    m_regCmd.FDChroma.module.u32All                = 0x00000101;
    m_regCmd.FDChroma.config.u32All                = 0x00000600;
    m_regCmd.FDChroma.imageSize.u32All             = 0x077f0437;
    m_regCmd.FDChroma.horizontalPhase.u32All       = 0xc0400000;
    m_regCmd.FDChroma.horizontalStripe.u32All      = 0x00000000;
    m_regCmd.FDChroma.verticalPhase.u32All         = 0xc0400000;
    m_regCmd.FDChroma.verticalStripe.u32All        = 0x00000000;
    m_regCmd.FDChroma.cropLine.u32All              = 0x00000437;
    m_regCmd.FDChroma.cropPixel.u32All             = 0x0000077f;

    m_regCmd.dispLuma.module.u32All                = 0x00000101;
    m_regCmd.dispLuma.config.u32All                = 0x00000600;
    m_regCmd.dispLuma.imageSize.u32All             = 0x077f0437;
    m_regCmd.dispLuma.horizontalPhase.u32All       = 0xc0200000;
    m_regCmd.dispLuma.horizontalStripe.u32All      = 0x00000000;
    m_regCmd.dispLuma.verticalPhase.u32All         = 0xc0200000;
    m_regCmd.dispLuma.verticalStripe.u32All        = 0x00000000;
    m_regCmd.dispLuma.cropLine.u32All              = 0x00000437;
    m_regCmd.dispLuma.cropPixel.u32All             = 0x0000077f;

    m_regCmd.dispChroma.module.u32All              = 0x00000101;
    m_regCmd.dispChroma.config.u32All              = 0x00009600;
    m_regCmd.dispChroma.imageSize.u32All           = 0x077f0437;
    m_regCmd.dispChroma.horizontalPhase.u32All     = 0xc0400000;
    m_regCmd.dispChroma.horizontalStripe.u32All    = 0x00000000;
    m_regCmd.dispChroma.verticalPhase.u32All       = 0xc0400000;
    m_regCmd.dispChroma.verticalStripe.u32All      = 0x00000000;
    m_regCmd.dispChroma.cropLine.u32All            = 0x00000437;
    m_regCmd.dispChroma.cropPixel.u32All           = 0x0000077f;

    m_regCmd.videoLuma.module.u32All               = 0x00000101;
    m_regCmd.videoLuma.config.u32All               = 0x00000600;
    m_regCmd.videoLuma.imageSize.u32All            = 0x077f0437;
    m_regCmd.videoLuma.horizontalPhase.u32All      = 0xc0200000;
    m_regCmd.videoLuma.horizontalStripe.u32All     = 0x00000000;
    m_regCmd.videoLuma.verticalPhase.u32All        = 0xc0200000;
    m_regCmd.videoLuma.verticalStripe.u32All       = 0x00000000;
    m_regCmd.videoLuma.cropLine.u32All             = 0x00000437;
    m_regCmd.videoLuma.cropPixel.u32All            = 0x0000077f;

    m_regCmd.videoChroma.module.u32All             = 0x00000101;
    m_regCmd.videoChroma.config.u32All             = 0x00000600;
    m_regCmd.videoChroma.imageSize.u32All          = 0x077f0437;
    m_regCmd.videoChroma.horizontalPhase.u32All    = 0xc0400000;
    m_regCmd.videoChroma.horizontalStripe.u32All   = 0x00000000;
    m_regCmd.videoChroma.verticalPhase.u32All      = 0xc0400000;
    m_regCmd.videoChroma.verticalStripe.u32All     = 0x00000000;
    m_regCmd.videoChroma.cropLine.u32All           = 0x00000437;
    m_regCmd.videoChroma.cropPixel.u32All          = 0x0000077f;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS21Titan480::CreateCmdList(
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
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_FD_OUT_MODULE_CFG,
                                                  (sizeof(IFEMNDS21FDLumaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.FDLuma));
            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_FD_OUT_MODULE_CFG,
                                                      (sizeof(IFEMNDS21FDChromaReg) / RegisterWidthInBytes),
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
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_VID_OUT_MODULE_CFG,
                                                  (sizeof(IFEMNDS21VideoLumaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.videoLuma));
            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_VID_OUT_MODULE_CFG,
                                                      (sizeof(IFEMNDS21VideoChromaReg) / RegisterWidthInBytes),
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
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_Y_DISP_OUT_MODULE_CFG,
                                                  (sizeof(IFEMNDS21DispLumaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.dispLuma));
            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regIFE_IFE_0_PP_CLC_DOWNSCALE_MN_C_DISP_OUT_MODULE_CFG,
                                                      (sizeof(IFEMNDS21DispChromaReg) / RegisterWidthInBytes),
                                                      reinterpret_cast<UINT32*>(&m_regCmd.dispChroma));
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
// IFEMNDS21Titan480::CalculateInterpolationResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS21Titan480::CalculateInterpolationResolution(
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
            CAMX_LOG_WARN(CamxLogGroupIQMod, "Scaler output larger than sensor out, Scale factor clamped to 1x");
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
        else if (scaleFactorHorizontal < MNDS21MaxScaleFactor)
        {
            *pHorizontalInterpolationResolution = 0;
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "scaleFactorHorizontal %d is greater than max supported ratio %d",
                                       scaleFactorHorizontal, MNDS21MaxScaleFactor);
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
            else if (scaleFactorVertical < MNDS21MaxScaleFactor)
            {
                *pVerticalInterpolationResolution = 0;
            }
            else
            {
                result = CamxResultEFailed;
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "scaleFactorVertical %d is greater than max supported ratio %d",
                                           scaleFactorVertical, MNDS21MaxScaleFactor);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input Null pointer");
        result = CamxResultEInvalidArg;
    }

    return  result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::GetChromaSubsampleFactor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS21Titan480::GetChromaSubsampleFactor(
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
            case Format::P010:
            case Format::PD10:
            case Format::Y8:
            case Format::Y16:
                *pHorizontalSubSampleFactor = 2.0f;
                *pVerticalSubsampleFactor   = 2.0f;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Incompatible Format: %d",  m_pixelFormat);
                *pHorizontalSubSampleFactor = 1.0f;
                *pVerticalSubsampleFactor   = 1.0f;
                result = CamxResultEInvalidArg;
                break;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input Null pointer");
        result = CamxResultEInvalidPointer;
    }

    return  result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::ConfigureFDLumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS21Titan480::ConfigureFDLumaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution)
{
    IFEMNDS21FDLumaReg* pFDLuma = &m_regCmd.FDLuma;
    UINT32 scale_in_cfg_input_h;
    UINT32 scale_in_cfg_input_v;
    UINT32 scale_in_cfg_output_h;
    UINT32 scale_in_cfg_output_v;
    FLOAT horizontal_scale_factor;
    FLOAT vertical_scale_factor;

    CAMX_UNREFERENCED_PARAM(horizontalInterpolationResolution);
    CAMX_UNREFERENCED_PARAM(verticalInterpolationResolution);

    scale_in_cfg_output_h   = m_pState->MNDSOutput.dimension.width;
    scale_in_cfg_output_v   = m_pState->MNDSOutput.dimension.height;
    scale_in_cfg_input_h    = m_pState->inputWidth;
    scale_in_cfg_input_v    = m_pState->inputHeight;
    horizontal_scale_factor = static_cast<FLOAT>(scale_in_cfg_input_h) / static_cast<FLOAT>(scale_in_cfg_output_h);
    vertical_scale_factor   = static_cast<FLOAT>(scale_in_cfg_input_v) / static_cast<FLOAT>(scale_in_cfg_output_v);

    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.width, "Invalid MNDS output width");
    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.height, "Invalid MNDS output height");

    // pFDLuma->config.bitfields.HORIZONTAL_SCALE_EN = TRUE;
    // pFDLuma->config.bitfields.VERTICAL_SCALE_EN = TRUE;

    pFDLuma->module.bitfields.CROP_EN                     = 0; // currently crop is not enabled

    pFDLuma->cropLine.bitfields.FIRST_LINE                = 0;
    pFDLuma->cropLine.bitfields.LAST_LINE                 = 0;
    pFDLuma->cropPixel.bitfields.FIRST_PIXEL              = 0;
    pFDLuma->cropPixel.bitfields.LAST_PIXEL               = 0;

    pFDLuma->config.bitfields.DROP_FIRST_OUTPUT_LINE      = 0;
    pFDLuma->config.bitfields.DROP_FIRST_OUTPUT_PIXEL     = 0;
    pFDLuma->config.bitfields.HORIZONTAL_ROUNDING         = 0;
    pFDLuma->config.bitfields.HORIZONTAL_TERMINATION_EN   = 0;
    pFDLuma->config.bitfields.VERTICAL_ROUNDING           = 0;
    pFDLuma->config.bitfields.VERTICAL_TERMINATION_EN     = 0;

    pFDLuma->horizontalPhase.bitfields.PHASE_STEP_H  = UINT32(floor(horizontal_scale_factor * (1 << 21)));
    pFDLuma->imageSize.bitfields.INPUT_HEIGHT        = m_pState->inputHeight - 1;
    pFDLuma->imageSize.bitfields.INPUT_WIDTH         = m_pState->inputWidth - 1;
    pFDLuma->horizontalStripe.bitfields.PHASE_INIT_H = 0;
    pFDLuma->verticalPhase.bitfields.PHASE_STEP_V    = UINT32(floor(vertical_scale_factor * (1 << 21)));
    pFDLuma->verticalStripe.bitfields.PHASE_INIT_V   = 0;

    pFDLuma->config.bitfields.HORIZONTAL_SCALE_EN =
        (scale_in_cfg_output_h < pFDLuma->imageSize.bitfields.INPUT_WIDTH + 1)  ? 1 : 0;
    pFDLuma->config.bitfields.VERTICAL_SCALE_EN   =
        (scale_in_cfg_output_v < pFDLuma->imageSize.bitfields.INPUT_HEIGHT + 1) ? 1 : 0;
    pFDLuma->module.bitfields.EN = (pFDLuma->config.bitfields.HORIZONTAL_SCALE_EN ||
                                    pFDLuma->config.bitfields.VERTICAL_SCALE_EN);
    if (horizontal_scale_factor > 64)
    {
        pFDLuma->horizontalPhase.bitfields.H_INTERP_RESO = 0;
    }
    else if (horizontal_scale_factor > 32)
    {
        pFDLuma->horizontalPhase.bitfields.H_INTERP_RESO = 1;
    }
    else if (horizontal_scale_factor > 16)
    {
        pFDLuma->horizontalPhase.bitfields.H_INTERP_RESO = 2;
    }
    else
    {
        pFDLuma->horizontalPhase.bitfields.H_INTERP_RESO = 3;
    }

    if (vertical_scale_factor > 64)
    {
        pFDLuma->verticalPhase.bitfields.V_INTERP_RESO = 0;
    }
    else if (vertical_scale_factor > 32)
    {
        pFDLuma->verticalPhase.bitfields.V_INTERP_RESO = 1;
    }
    else if (vertical_scale_factor > 16)
    {
        pFDLuma->verticalPhase.bitfields.V_INTERP_RESO = 2;
    }
    else
    {
        pFDLuma->verticalPhase.bitfields.V_INTERP_RESO = 3;
    }

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        // Overwrite horizontal only. Use the above vertical settings from already configured
        pFDLuma->config.bitfields.HORIZONTAL_ROUNDING     = m_pState->MNDSOutput.version20.mnds_config_y.roundingOptionHor;
        pFDLuma->imageSize.bitfields.INPUT_WIDTH          = m_pState->MNDSOutput.version20.mnds_config_y.input - 1;
        pFDLuma->horizontalPhase.bitfields.PHASE_STEP_H   = m_pState->MNDSOutput.version20.mnds_config_y.phaseStep;
        pFDLuma->horizontalPhase.bitfields.H_INTERP_RESO  = m_pState->MNDSOutput.version20.mnds_config_y.interpReso;
        pFDLuma->horizontalStripe.bitfields.PHASE_INIT_H  = m_pState->MNDSOutput.version20.mnds_config_y.phaseInit;
        // If the striping library enabled pre MNDS crop, enable crop and configure accordingly.
        if (TRUE == m_pState->MNDSOutput.version20.preMndsCrop_y.enable)
        {
            pFDLuma->module.bitfields.CROP_EN = 1;
            pFDLuma->cropPixel.bitfields.FIRST_PIXEL  = m_pState->MNDSOutput.version20.preMndsCrop_y.firstOut;
            pFDLuma->cropPixel.bitfields.LAST_PIXEL   = m_pState->MNDSOutput.version20.preMndsCrop_y.lastOut;
            pFDLuma->cropLine.bitfields.FIRST_LINE    = 0;
            pFDLuma->cropLine.bitfields.LAST_LINE     = m_pState->inputHeight - 1;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::ConfigureFullLumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS21Titan480::ConfigureFullLumaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution)
{
    IFEMNDS21VideoLumaReg* pVidLuma = &m_regCmd.videoLuma;
    UINT32 scale_in_cfg_input_h;
    UINT32 scale_in_cfg_input_v;
    UINT32 scale_in_cfg_output_h;
    UINT32 scale_in_cfg_output_v;
    FLOAT horizontal_scale_factor;
    FLOAT vertical_scale_factor;

    CAMX_UNREFERENCED_PARAM(horizontalInterpolationResolution);
    CAMX_UNREFERENCED_PARAM(verticalInterpolationResolution);

    scale_in_cfg_output_h = m_pState->MNDSOutput.dimension.width;
    scale_in_cfg_output_v = m_pState->MNDSOutput.dimension.height;
    scale_in_cfg_input_h  = m_pState->inputWidth;
    scale_in_cfg_input_v  = m_pState->inputHeight;

    horizontal_scale_factor = static_cast<FLOAT>(scale_in_cfg_input_h) / static_cast<FLOAT>(scale_in_cfg_output_h);
    vertical_scale_factor   = static_cast<FLOAT>(scale_in_cfg_input_v) / static_cast<FLOAT>(scale_in_cfg_output_v);

    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.width, "Invalid MNDS output width");
    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.height, "Invalid MNDS output height");

    // pVidLuma->config.bitfields.HORIZONTAL_SCALE_EN = TRUE;
    // pVidLuma->config.bitfields.VERTICAL_SCALE_EN = TRUE;

    pVidLuma->module.bitfields.CROP_EN          = 0; // currently crop is not enabled

    pVidLuma->cropLine.bitfields.FIRST_LINE     = 0;
    pVidLuma->cropLine.bitfields.LAST_LINE      = 0;
    pVidLuma->cropPixel.bitfields.FIRST_PIXEL   = 0;
    pVidLuma->cropPixel.bitfields.LAST_PIXEL    = 0;

    pVidLuma->config.bitfields.DROP_FIRST_OUTPUT_LINE       = 0;
    pVidLuma->config.bitfields.DROP_FIRST_OUTPUT_PIXEL      = 0;
    pVidLuma->config.bitfields.HORIZONTAL_ROUNDING          = 0;
    pVidLuma->config.bitfields.HORIZONTAL_TERMINATION_EN    = 0;
    pVidLuma->config.bitfields.VERTICAL_ROUNDING            = 0;
    pVidLuma->config.bitfields.VERTICAL_TERMINATION_EN      = 0;

    pVidLuma->horizontalPhase.bitfields.PHASE_STEP_H    = UINT32(floor(horizontal_scale_factor * (1 << 21)));
    pVidLuma->imageSize.bitfields.INPUT_HEIGHT          = m_pState->inputHeight - 1;
    pVidLuma->imageSize.bitfields.INPUT_WIDTH           = m_pState->inputWidth - 1;
    pVidLuma->horizontalStripe.bitfields.PHASE_INIT_H   = 0;
    pVidLuma->verticalPhase.bitfields.PHASE_STEP_V      = UINT32(floor(vertical_scale_factor * (1 << 21)));
    pVidLuma->verticalStripe.bitfields.PHASE_INIT_V     = 0;

    ///< Scaler need to be enabled always , if the corresponding output path is enabled - This is HW workaround
    pVidLuma->config.bitfields.HORIZONTAL_SCALE_EN  = 1;
    pVidLuma->config.bitfields.VERTICAL_SCALE_EN    = 1;
    pVidLuma->module.bitfields.EN = (pVidLuma->config.bitfields.HORIZONTAL_SCALE_EN ||
                                     pVidLuma->config.bitfields.VERTICAL_SCALE_EN);
    if (horizontal_scale_factor > 64)
    {
        pVidLuma->horizontalPhase.bitfields.H_INTERP_RESO = 0;
    }
    else if (horizontal_scale_factor > 32)
    {
        pVidLuma->horizontalPhase.bitfields.H_INTERP_RESO = 1;
    }
    else if (horizontal_scale_factor > 16)
    {
        pVidLuma->horizontalPhase.bitfields.H_INTERP_RESO = 2;
    }
    else
    {
        pVidLuma->horizontalPhase.bitfields.H_INTERP_RESO = 3;
    }

    if (vertical_scale_factor > 64)
    {
        pVidLuma->verticalPhase.bitfields.V_INTERP_RESO = 0;
    }
    else if (vertical_scale_factor > 32)
    {
        pVidLuma->verticalPhase.bitfields.V_INTERP_RESO = 1;
    }
    else if (vertical_scale_factor > 16)
    {
        pVidLuma->verticalPhase.bitfields.V_INTERP_RESO = 2;
    }
    else
    {
        pVidLuma->verticalPhase.bitfields.V_INTERP_RESO = 3;
    }

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        // Overwrite horizontal only. Use the above vertical settings from already configured
        pVidLuma->config.bitfields.HORIZONTAL_ROUNDING      = m_pState->MNDSOutput.version20.mnds_config_y.roundingOptionHor;
        pVidLuma->imageSize.bitfields.INPUT_WIDTH           = m_pState->MNDSOutput.version20.mnds_config_y.input - 1;
        pVidLuma->horizontalPhase.bitfields.PHASE_STEP_H    = m_pState->MNDSOutput.version20.mnds_config_y.phaseStep;
        pVidLuma->horizontalPhase.bitfields.H_INTERP_RESO   = m_pState->MNDSOutput.version20.mnds_config_y.interpReso;
        pVidLuma->horizontalStripe.bitfields.PHASE_INIT_H   = m_pState->MNDSOutput.version20.mnds_config_y.phaseInit;
        // If the striping library enabled pre MNDS crop, enable crop and configure accordingly.
        if (TRUE == m_pState->MNDSOutput.version20.preMndsCrop_y.enable)
        {
            pVidLuma->module.bitfields.CROP_EN          = 1;
            pVidLuma->cropPixel.bitfields.FIRST_PIXEL   = m_pState->MNDSOutput.version20.preMndsCrop_y.firstOut;
            pVidLuma->cropPixel.bitfields.LAST_PIXEL    = m_pState->MNDSOutput.version20.preMndsCrop_y.lastOut;
            pVidLuma->cropLine.bitfields.FIRST_LINE     = 0;
            pVidLuma->cropLine.bitfields.LAST_LINE      = m_pState->inputHeight - 1;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::ConfigureDispLumaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS21Titan480::ConfigureDispLumaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution)
{
    IFEMNDS21DispLumaReg* pDispLuma = &m_regCmd.dispLuma;
    UINT32 scale_in_cfg_input_h;
    UINT32 scale_in_cfg_input_v;
    UINT32 scale_in_cfg_output_h;
    UINT32 scale_in_cfg_output_v;
    FLOAT horizontal_scale_factor;
    FLOAT vertical_scale_factor;

    CAMX_UNREFERENCED_PARAM(horizontalInterpolationResolution);
    CAMX_UNREFERENCED_PARAM(verticalInterpolationResolution);

    scale_in_cfg_output_h   = m_pState->MNDSOutput.dimension.width;
    scale_in_cfg_output_v   = m_pState->MNDSOutput.dimension.height;
    scale_in_cfg_input_h    = m_pState->inputWidth;
    scale_in_cfg_input_v    = m_pState->inputHeight;

    horizontal_scale_factor = static_cast<FLOAT>(scale_in_cfg_input_h) / static_cast<FLOAT>(scale_in_cfg_output_h);
    vertical_scale_factor   = static_cast<FLOAT>(scale_in_cfg_input_v) / static_cast<FLOAT>(scale_in_cfg_output_v);

    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.width, "Invalid MNDS output width");
    CAMX_ASSERT_MESSAGE(0 != m_pState->MNDSOutput.dimension.height, "Invalid MNDS output height");

    // pVidLuma->config.bitfields.HORIZONTAL_SCALE_EN = TRUE;
    // pVidLuma->config.bitfields.VERTICAL_SCALE_EN = TRUE;

    pDispLuma->module.bitfields.CROP_EN         = 0; // currently crop is not enabled

    pDispLuma->cropLine.bitfields.FIRST_LINE    = 0;
    pDispLuma->cropLine.bitfields.LAST_LINE     = 0;
    pDispLuma->cropPixel.bitfields.FIRST_PIXEL  = 0;
    pDispLuma->cropPixel.bitfields.LAST_PIXEL   = 0;

    pDispLuma->config.bitfields.DROP_FIRST_OUTPUT_LINE      = 0;
    pDispLuma->config.bitfields.DROP_FIRST_OUTPUT_PIXEL     = 0;
    pDispLuma->config.bitfields.HORIZONTAL_ROUNDING         = 0;
    pDispLuma->config.bitfields.HORIZONTAL_TERMINATION_EN   = 0;
    pDispLuma->config.bitfields.VERTICAL_ROUNDING           = 0;
    pDispLuma->config.bitfields.VERTICAL_TERMINATION_EN     = 0;

    pDispLuma->horizontalPhase.bitfields.PHASE_STEP_H   = UINT32(floor(horizontal_scale_factor * (1 << 21)));
    pDispLuma->imageSize.bitfields.INPUT_HEIGHT         = m_pState->inputHeight - 1;
    pDispLuma->imageSize.bitfields.INPUT_WIDTH          = m_pState->inputWidth - 1;
    pDispLuma->horizontalStripe.bitfields.PHASE_INIT_H  = 0;
    pDispLuma->verticalPhase.bitfields.PHASE_STEP_V     = UINT32(floor(vertical_scale_factor * (1 << 21)));
    pDispLuma->verticalStripe.bitfields.PHASE_INIT_V    = 0;

    ///< Scaler need to be enabled always , if the corresponding output path is enabled-  This is HW workaround
    pDispLuma->config.bitfields.HORIZONTAL_SCALE_EN = 1;
    pDispLuma->config.bitfields.VERTICAL_SCALE_EN   = 1;

    pDispLuma->module.bitfields.EN = (pDispLuma->config.bitfields.HORIZONTAL_SCALE_EN ||
                                      pDispLuma->config.bitfields.VERTICAL_SCALE_EN);
    if (horizontal_scale_factor > 64)
    {
        pDispLuma->horizontalPhase.bitfields.H_INTERP_RESO = 0;
    }
    else if (horizontal_scale_factor > 32)
    {
        pDispLuma->horizontalPhase.bitfields.H_INTERP_RESO = 1;
    }
    else if (horizontal_scale_factor > 16)
    {
        pDispLuma->horizontalPhase.bitfields.H_INTERP_RESO = 2;
    }
    else
    {
        pDispLuma->horizontalPhase.bitfields.H_INTERP_RESO = 3;
    }

    if (vertical_scale_factor > 64)
    {
        pDispLuma->verticalPhase.bitfields.V_INTERP_RESO = 0;
    }
    else if (vertical_scale_factor > 32)
    {
        pDispLuma->verticalPhase.bitfields.V_INTERP_RESO = 1;
    }
    else if (vertical_scale_factor > 16)
    {
        pDispLuma->verticalPhase.bitfields.V_INTERP_RESO = 2;
    }
    else
    {
        pDispLuma->verticalPhase.bitfields.V_INTERP_RESO = 3;
    }

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        // Overwrite horizontal only. Use the above vertical settings from already configured
        pDispLuma->config.bitfields.HORIZONTAL_ROUNDING     = m_pState->MNDSOutput.version20.mnds_config_y.roundingOptionHor;
        pDispLuma->imageSize.bitfields.INPUT_WIDTH          = m_pState->MNDSOutput.version20.mnds_config_y.input - 1;
        pDispLuma->horizontalPhase.bitfields.PHASE_STEP_H   = m_pState->MNDSOutput.version20.mnds_config_y.phaseStep;
        pDispLuma->horizontalPhase.bitfields.H_INTERP_RESO  = m_pState->MNDSOutput.version20.mnds_config_y.interpReso;
        pDispLuma->horizontalStripe.bitfields.PHASE_INIT_H  = m_pState->MNDSOutput.version20.mnds_config_y.phaseInit;
        // If the striping library enabled pre MNDS crop, enable crop and configure accordingly.
        if (TRUE == m_pState->MNDSOutput.version20.preMndsCrop_y.enable)
        {
            pDispLuma->module.bitfields.CROP_EN = 1;
            pDispLuma->cropPixel.bitfields.FIRST_PIXEL  = m_pState->MNDSOutput.version20.preMndsCrop_y.firstOut;
            pDispLuma->cropPixel.bitfields.LAST_PIXEL   = m_pState->MNDSOutput.version20.preMndsCrop_y.lastOut;
            pDispLuma->cropLine.bitfields.FIRST_LINE    = 0;
            pDispLuma->cropLine.bitfields.LAST_LINE     = m_pState->inputHeight - 1;
        }
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::ConfigureFDChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS21Titan480::ConfigureFDChromaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution,
    FLOAT  horizontalSubSampleFactor,
    FLOAT  verticalSubsampleFactor)
{
    IFEMNDS21FDChromaReg* pFDChroma = &m_regCmd.FDChroma;
    UINT32 scale_in_cfg_input_h;
    UINT32 scale_in_cfg_input_v;
    UINT32 scale_in_cfg_output_h;
    UINT32 scale_in_cfg_output_v;
    FLOAT horizontal_scale_factor;
    FLOAT vertical_scale_factor;

    CAMX_UNREFERENCED_PARAM(horizontalInterpolationResolution);
    CAMX_UNREFERENCED_PARAM(verticalInterpolationResolution);

    scale_in_cfg_output_h = m_pState->MNDSOutput.dimension.width / static_cast<UINT32>(horizontalSubSampleFactor);
    scale_in_cfg_output_v = m_pState->MNDSOutput.dimension.height / static_cast<UINT32>(verticalSubsampleFactor);
    scale_in_cfg_input_h  = m_pState->inputWidth;
    scale_in_cfg_input_v  = m_pState->inputHeight;

    horizontal_scale_factor = static_cast<FLOAT>(scale_in_cfg_input_h) / static_cast<FLOAT>(scale_in_cfg_output_h);
    vertical_scale_factor   = static_cast<FLOAT>(scale_in_cfg_input_v) / static_cast<FLOAT>(scale_in_cfg_output_v);

    // pFDLuma->config.bitfields.HORIZONTAL_SCALE_EN = TRUE;
    // pFDLuma->config.bitfields.VERTICAL_SCALE_EN = TRUE;

    pFDChroma->module.bitfields.EN              = 1;
    pFDChroma->module.bitfields.CROP_EN         = 0; // currently crop is not enabled
    pFDChroma->cropLine.bitfields.FIRST_LINE    = 0;
    pFDChroma->cropLine.bitfields.LAST_LINE     = 0;
    pFDChroma->cropPixel.bitfields.FIRST_PIXEL  = 0;
    pFDChroma->cropPixel.bitfields.LAST_PIXEL   = 0;

    pFDChroma->config.bitfields.HORIZONTAL_ROUNDING         = 0; // 20 has 1?
    pFDChroma->config.bitfields.HORIZONTAL_TERMINATION_EN   = 0;
    pFDChroma->config.bitfields.VERTICAL_ROUNDING           = 0; // 20 has 2?
    pFDChroma->config.bitfields.VERTICAL_TERMINATION_EN     = 0;

    pFDChroma->horizontalPhase.bitfields.PHASE_STEP_H       = UINT32(floor(horizontal_scale_factor * (1 << 21)));
    pFDChroma->imageSize.bitfields.INPUT_HEIGHT             = m_pState->inputHeight - 1;
    pFDChroma->imageSize.bitfields.INPUT_WIDTH              = m_pState->inputWidth - 1;
    pFDChroma->horizontalStripe.bitfields.PHASE_INIT_H      = 0;
    pFDChroma->verticalPhase.bitfields.PHASE_STEP_V         = UINT32(floor(vertical_scale_factor * (1 << 21)));
    pFDChroma->verticalStripe.bitfields.PHASE_INIT_V        = 0;

    pFDChroma->config.bitfields.HORIZONTAL_SCALE_EN         =
        (scale_in_cfg_output_h < pFDChroma->imageSize.bitfields.INPUT_WIDTH + 1) ? 1 : 0;
    pFDChroma->config.bitfields.VERTICAL_SCALE_EN           =
        (scale_in_cfg_output_v < pFDChroma->imageSize.bitfields.INPUT_HEIGHT + 1) ? 1 : 0;
    pFDChroma->module.bitfields.EN = (pFDChroma->config.bitfields.HORIZONTAL_SCALE_EN ||
                                      pFDChroma->config.bitfields.VERTICAL_SCALE_EN);
    if (horizontal_scale_factor > 64)
    {
        pFDChroma->horizontalPhase.bitfields.H_INTERP_RESO = 0;
    }
    else if (horizontal_scale_factor > 32)
    {
        pFDChroma->horizontalPhase.bitfields.H_INTERP_RESO = 1;
    }
    else if (horizontal_scale_factor > 16)
    {
        pFDChroma->horizontalPhase.bitfields.H_INTERP_RESO = 2;
    }
    else
    {
        pFDChroma->horizontalPhase.bitfields.H_INTERP_RESO = 3;
    }

    if (vertical_scale_factor > 64)
    {
        pFDChroma->verticalPhase.bitfields.V_INTERP_RESO = 0;
    }
    else if (vertical_scale_factor > 32)
    {
        pFDChroma->verticalPhase.bitfields.V_INTERP_RESO = 1;
    }
    else if (vertical_scale_factor > 16)
    {
        pFDChroma->verticalPhase.bitfields.V_INTERP_RESO = 2;
    }
    else
    {
        pFDChroma->verticalPhase.bitfields.V_INTERP_RESO = 3;
    }

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        // Overwrite horizontal only. Use the above vertical settings from already configured
        pFDChroma->config.bitfields.HORIZONTAL_ROUNDING    = m_pState->MNDSOutput.version20.mnds_config_c.roundingOptionHor;
        pFDChroma->imageSize.bitfields.INPUT_WIDTH         = m_pState->MNDSOutput.version20.mnds_config_c.input - 1;
        pFDChroma->horizontalPhase.bitfields.PHASE_STEP_H  = m_pState->MNDSOutput.version20.mnds_config_c.phaseStep;
        pFDChroma->horizontalPhase.bitfields.H_INTERP_RESO = m_pState->MNDSOutput.version20.mnds_config_c.interpReso;
        pFDChroma->horizontalStripe.bitfields.PHASE_INIT_H = m_pState->MNDSOutput.version20.mnds_config_c.phaseInit;
        // If the striping library enabled pre MNDS crop, enable crop and configure accordingly.
        if (TRUE == m_pState->MNDSOutput.version20.preMndsCrop_c.enable)
        {
            pFDChroma->module.bitfields.CROP_EN          = 1;
            pFDChroma->cropPixel.bitfields.FIRST_PIXEL   = m_pState->MNDSOutput.version20.preMndsCrop_c.firstOut;
            pFDChroma->cropPixel.bitfields.LAST_PIXEL    = m_pState->MNDSOutput.version20.preMndsCrop_c.lastOut;
            pFDChroma->cropLine.bitfields.FIRST_LINE     = 0;
            pFDChroma->cropLine.bitfields.LAST_LINE      = m_pState->inputHeight - 1;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::ConfigureFullChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS21Titan480::ConfigureFullChromaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution,
    FLOAT  horizontalSubSampleFactor,
    FLOAT  verticalSubsampleFactor)
{
    IFEMNDS21VideoChromaReg* pVidChroma = &m_regCmd.videoChroma;
    UINT32 scale_in_cfg_input_h;
    UINT32 scale_in_cfg_input_v;
    UINT32 scale_in_cfg_output_h;
    UINT32 scale_in_cfg_output_v;
    FLOAT horizontal_scale_factor;
    FLOAT vertical_scale_factor;

    CAMX_UNREFERENCED_PARAM(horizontalInterpolationResolution);
    CAMX_UNREFERENCED_PARAM(verticalInterpolationResolution);

    scale_in_cfg_output_h = m_pState->MNDSOutput.dimension.width / static_cast<UINT32>(horizontalSubSampleFactor);
    scale_in_cfg_output_v = m_pState->MNDSOutput.dimension.height / static_cast<UINT32>(verticalSubsampleFactor);
    scale_in_cfg_input_h  = m_pState->inputWidth;
    scale_in_cfg_input_v  = m_pState->inputHeight;

    horizontal_scale_factor = static_cast<FLOAT>(scale_in_cfg_input_h) / static_cast<FLOAT>(scale_in_cfg_output_h);
    vertical_scale_factor   = static_cast<FLOAT>(scale_in_cfg_input_v) / static_cast<FLOAT>(scale_in_cfg_output_v);

    pVidChroma->module.bitfields.EN             = 1;
    pVidChroma->module.bitfields.CROP_EN        = 0; // currently crop is not enabled

    pVidChroma->cropLine.bitfields.FIRST_LINE   = 0;
    pVidChroma->cropLine.bitfields.LAST_LINE    = 0;
    pVidChroma->cropPixel.bitfields.FIRST_PIXEL = 0;
    pVidChroma->cropPixel.bitfields.LAST_PIXEL  = 0;

    pVidChroma->config.bitfields.HORIZONTAL_ROUNDING        = 0; // 20 has 1?
    pVidChroma->config.bitfields.HORIZONTAL_TERMINATION_EN  = 0;
    pVidChroma->config.bitfields.VERTICAL_ROUNDING          = 0; // 20 has 2?
    pVidChroma->config.bitfields.VERTICAL_TERMINATION_EN    = 0;
    pVidChroma->horizontalPhase.bitfields.PHASE_STEP_H      = UINT32(floor(horizontal_scale_factor * (1 << 21)));
    pVidChroma->imageSize.bitfields.INPUT_HEIGHT            = m_pState->inputHeight - 1;
    pVidChroma->imageSize.bitfields.INPUT_WIDTH             = m_pState->inputWidth - 1;
    pVidChroma->horizontalStripe.bitfields.PHASE_INIT_H     = 0;
    pVidChroma->verticalPhase.bitfields.PHASE_STEP_V        = UINT32(floor(vertical_scale_factor * (1 << 21)));
    pVidChroma->verticalStripe.bitfields.PHASE_INIT_V       = 0;

    ///< Scaler need to be enabled always , if the corresponding output path is enabled
    pVidChroma->config.bitfields.HORIZONTAL_SCALE_EN = 1;

    pVidChroma->config.bitfields.VERTICAL_SCALE_EN   = 1;

    pVidChroma->module.bitfields.EN = (pVidChroma->config.bitfields.HORIZONTAL_SCALE_EN ||
                                       pVidChroma->config.bitfields.VERTICAL_SCALE_EN);
    if (horizontal_scale_factor > 64)
    {
        pVidChroma->horizontalPhase.bitfields.H_INTERP_RESO = 0;
    }
    else if (horizontal_scale_factor > 32)
    {
        pVidChroma->horizontalPhase.bitfields.H_INTERP_RESO = 1;
    }
    else if (horizontal_scale_factor > 16)
    {
        pVidChroma->horizontalPhase.bitfields.H_INTERP_RESO = 2;
    }
    else
    {
        pVidChroma->horizontalPhase.bitfields.H_INTERP_RESO = 3;
    }

    if (vertical_scale_factor > 64)
    {
        pVidChroma->verticalPhase.bitfields.V_INTERP_RESO = 0;
    }
    else if (vertical_scale_factor > 32)
    {
        pVidChroma->verticalPhase.bitfields.V_INTERP_RESO = 1;
    }
    else if (vertical_scale_factor > 16)
    {
        pVidChroma->verticalPhase.bitfields.V_INTERP_RESO = 2;
    }
    else
    {
        pVidChroma->verticalPhase.bitfields.V_INTERP_RESO = 3;
    }

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        // Overwrite horizontal only. Use the above vertical settings from already configured
        pVidChroma->config.bitfields.HORIZONTAL_ROUNDING    = m_pState->MNDSOutput.version20.mnds_config_c.roundingOptionHor;
        pVidChroma->imageSize.bitfields.INPUT_WIDTH         = m_pState->MNDSOutput.version20.mnds_config_c.input - 1;
        pVidChroma->horizontalPhase.bitfields.PHASE_STEP_H  = m_pState->MNDSOutput.version20.mnds_config_c.phaseStep;
        pVidChroma->horizontalPhase.bitfields.H_INTERP_RESO = m_pState->MNDSOutput.version20.mnds_config_c.interpReso;
        pVidChroma->horizontalStripe.bitfields.PHASE_INIT_H = m_pState->MNDSOutput.version20.mnds_config_c.phaseInit;
        // If the striping library enabled pre MNDS crop, enable crop and configure accordingly.
        if (TRUE == m_pState->MNDSOutput.version20.preMndsCrop_c.enable)
        {
            pVidChroma->module.bitfields.CROP_EN          = 1;
            pVidChroma->cropPixel.bitfields.FIRST_PIXEL   = m_pState->MNDSOutput.version20.preMndsCrop_c.firstOut;
            pVidChroma->cropPixel.bitfields.LAST_PIXEL    = m_pState->MNDSOutput.version20.preMndsCrop_c.lastOut;
            pVidChroma->cropLine.bitfields.FIRST_LINE     = 0;
            pVidChroma->cropLine.bitfields.LAST_LINE      = m_pState->inputHeight - 1;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::ConfigureDispChromaRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS21Titan480::ConfigureDispChromaRegisters(
    UINT32 horizontalInterpolationResolution,
    UINT32 verticalInterpolationResolution,
    FLOAT  horizontalSubSampleFactor,
    FLOAT  verticalSubsampleFactor)
{
    IFEMNDS21DispChromaReg* pDispChroma = &m_regCmd.dispChroma;
    UINT32 scale_in_cfg_input_h;
    UINT32 scale_in_cfg_input_v;
    UINT32 scale_in_cfg_output_h;
    UINT32 scale_in_cfg_output_v;
    FLOAT horizontal_scale_factor;
    FLOAT vertical_scale_factor;

    CAMX_UNREFERENCED_PARAM(horizontalInterpolationResolution);
    CAMX_UNREFERENCED_PARAM(verticalInterpolationResolution);

    scale_in_cfg_output_h = m_pState->MNDSOutput.dimension.width / static_cast<UINT32>(horizontalSubSampleFactor);
    scale_in_cfg_output_v = m_pState->MNDSOutput.dimension.height / static_cast<UINT32>(verticalSubsampleFactor);
    scale_in_cfg_input_h  = m_pState->inputWidth;
    scale_in_cfg_input_v  = m_pState->inputHeight;

    horizontal_scale_factor = static_cast<FLOAT>(scale_in_cfg_input_h) / static_cast<FLOAT>(scale_in_cfg_output_h);
    vertical_scale_factor   = static_cast<FLOAT>(scale_in_cfg_input_v) / static_cast<FLOAT>(scale_in_cfg_output_v);

    // pDispChroma->config.bitfields.HORIZONTAL_SCALE_EN = TRUE;
    // pDispChroma->config.bitfields.VERTICAL_SCALE_EN = TRUE;

    pDispChroma->module.bitfields.EN                = 1;
    pDispChroma->module.bitfields.CROP_EN           = 0; // currently crop is not enabled

    pDispChroma->cropLine.bitfields.FIRST_LINE      = 0;
    pDispChroma->cropLine.bitfields.LAST_LINE       = 0;
    pDispChroma->cropPixel.bitfields.FIRST_PIXEL    = 0;
    pDispChroma->cropPixel.bitfields.LAST_PIXEL     = 0;

    pDispChroma->config.bitfields.HORIZONTAL_ROUNDING       = 0; // 20 has 1?
    pDispChroma->config.bitfields.HORIZONTAL_TERMINATION_EN = 0;
    pDispChroma->config.bitfields.VERTICAL_ROUNDING         = 0; // 20 has 2?
    pDispChroma->config.bitfields.VERTICAL_TERMINATION_EN   = 0;

    pDispChroma->horizontalPhase.bitfields.PHASE_STEP_H     = UINT32(floor(horizontal_scale_factor * (1 << 21)));
    pDispChroma->imageSize.bitfields.INPUT_HEIGHT           = m_pState->inputHeight - 1;
    pDispChroma->imageSize.bitfields.INPUT_WIDTH            = m_pState->inputWidth - 1;
    pDispChroma->horizontalStripe.bitfields.PHASE_INIT_H    = 0;
    pDispChroma->verticalPhase.bitfields.PHASE_STEP_V       = UINT32(floor(vertical_scale_factor * (1 << 21)));
    pDispChroma->verticalStripe.bitfields.PHASE_INIT_V      = 0;

    ///< Scaler need to be enabled always , if the corresponding output path is enabled--  This is HW workaround
    pDispChroma->config.bitfields.HORIZONTAL_SCALE_EN       = 1;
    pDispChroma->config.bitfields.VERTICAL_SCALE_EN         = 1;

    pDispChroma->module.bitfields.EN = (pDispChroma->config.bitfields.HORIZONTAL_SCALE_EN ||
                                        pDispChroma->config.bitfields.VERTICAL_SCALE_EN);
    if (horizontal_scale_factor > 64)
    {
        pDispChroma->horizontalPhase.bitfields.H_INTERP_RESO = 0;
    }
    else if (horizontal_scale_factor > 32)
    {
        pDispChroma->horizontalPhase.bitfields.H_INTERP_RESO = 1;
    }
    else if (horizontal_scale_factor > 16)
    {
        pDispChroma->horizontalPhase.bitfields.H_INTERP_RESO = 2;
    }
    else
    {
        pDispChroma->horizontalPhase.bitfields.H_INTERP_RESO = 3;
    }

    if (vertical_scale_factor > 64)
    {
        pDispChroma->verticalPhase.bitfields.V_INTERP_RESO = 0;
    }
    else if (vertical_scale_factor > 32)
    {
        pDispChroma->verticalPhase.bitfields.V_INTERP_RESO = 1;
    }
    else if (vertical_scale_factor > 16)
    {
        pDispChroma->verticalPhase.bitfields.V_INTERP_RESO = 2;
    }
    else
    {
        pDispChroma->verticalPhase.bitfields.V_INTERP_RESO = 3;
    }

    if (TRUE == m_pInputData->pStripeConfig->overwriteStripes)
    {
        // Overwrite horizontal only. Use the above vertical settings from already configured
        pDispChroma->config.bitfields.HORIZONTAL_ROUNDING    = m_pState->MNDSOutput.version20.mnds_config_c.roundingOptionHor;
        pDispChroma->imageSize.bitfields.INPUT_WIDTH         = m_pState->MNDSOutput.version20.mnds_config_c.input - 1;
        pDispChroma->horizontalPhase.bitfields.PHASE_STEP_H  = m_pState->MNDSOutput.version20.mnds_config_c.phaseStep;
        pDispChroma->horizontalPhase.bitfields.H_INTERP_RESO = m_pState->MNDSOutput.version20.mnds_config_c.interpReso;
        pDispChroma->horizontalStripe.bitfields.PHASE_INIT_H = m_pState->MNDSOutput.version20.mnds_config_c.phaseInit;
        // If the striping library enabled pre MNDS crop, enable crop and configure accordingly.
        if (TRUE == m_pState->MNDSOutput.version20.preMndsCrop_c.enable)
        {
            pDispChroma->module.bitfields.CROP_EN           = 1;
            pDispChroma->cropPixel.bitfields.FIRST_PIXEL    = m_pState->MNDSOutput.version20.preMndsCrop_c.firstOut;
            pDispChroma->cropPixel.bitfields.LAST_PIXEL     = m_pState->MNDSOutput.version20.preMndsCrop_c.lastOut;
            pDispChroma->cropLine.bitfields.FIRST_LINE      = 0;
            pDispChroma->cropLine.bitfields.LAST_LINE       = m_pState->inputHeight - 1;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS21Titan480::PackIQRegisterSetting(
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

        // Calculate the interpolation resolution based on the scaling ratio
        scaleFactorHorizontal = m_pState->inputWidth / m_pState->MNDSOutput.dimension.width;
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
            else if (FullOutput == m_output)
            {
                ConfigureFullLumaRegisters(horizontalInterpolationResolution, verticalInterpolationResolution);
            }
            else // display output
            {
                ConfigureDispLumaRegisters(horizontalInterpolationResolution, verticalInterpolationResolution);
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
            else if (FullOutput == m_output)
            {
                ConfigureFullChromaRegisters(horizontalInterpolationResolution,
                                             verticalInterpolationResolution,
                                             horizontalSubSampleFactor,
                                             verticalSubsampleFactor);
            }
            else // display output
            {
                ConfigureDispChromaRegisters(horizontalInterpolationResolution,
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
// IFEMNDS21Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEMNDS21Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_ERROR(CamxLogGroupIQMod, "Not Implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEMNDS21Titan480::CopyRegCmd(
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
// IFEMNDS21Titan480::~IFEMNDS21Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEMNDS21Titan480::~IFEMNDS21Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEMNDS21Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEMNDS21Titan480::DumpRegConfig()
{
    if (FDOutput == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma module            [0x%x] ", m_regCmd.FDLuma.module);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma config            [0x%x] ", m_regCmd.FDLuma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma imageSize         [0x%x] ", m_regCmd.FDLuma.imageSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma horizontalPhase   [0x%x] ", m_regCmd.FDLuma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma horizontalStripe  [0x%x] ", m_regCmd.FDLuma.horizontalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma verticalPhase     [0x%x] ", m_regCmd.FDLuma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma verticalStripe    [0x%x] ", m_regCmd.FDLuma.verticalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma cropLine          [0x%x] ", m_regCmd.FDLuma.cropLine);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Luma cropPixel         [0x%x] ", m_regCmd.FDLuma.cropPixel);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma module            [0x%x] ", m_regCmd.FDChroma.module);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma config            [0x%x] ", m_regCmd.FDChroma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma imageSize         [0x%x] ", m_regCmd.FDChroma.imageSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma horizontalPhase   [0x%x] ", m_regCmd.FDChroma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma horizontalStripe  [0x%x] ", m_regCmd.FDChroma.horizontalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma verticalPhase     [0x%x] ", m_regCmd.FDChroma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma verticalStripe    [0x%x] ", m_regCmd.FDChroma.verticalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma cropLine          [0x%x] ", m_regCmd.FDChroma.cropLine);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS FD Chroma cropPixel         [0x%x] ", m_regCmd.FDChroma.cropPixel);
    }

    if (FullOutput == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma module            [0x%x] ", m_regCmd.videoLuma.module);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma config            [0x%x] ", m_regCmd.videoLuma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma imageSize         [0x%x] ", m_regCmd.videoLuma.imageSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma horizontalPhase   [0x%x] ", m_regCmd.videoLuma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma horizontalStripe  [0x%x] ", m_regCmd.videoLuma.horizontalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma verticalPhase     [0x%x] ", m_regCmd.videoLuma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma verticalStripe    [0x%x] ", m_regCmd.videoLuma.verticalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma cropLine          [0x%x] ", m_regCmd.videoLuma.cropLine);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma cropPixel         [0x%x] ", m_regCmd.videoLuma.cropPixel);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma module            [0x%x] ", m_regCmd.videoChroma.module);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma config            [0x%x] ", m_regCmd.videoChroma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma imageSize         [0x%x] ", m_regCmd.videoChroma.imageSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma horizontalPhase   [0x%x] ", m_regCmd.videoChroma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma horizontalStripe  [0x%x] ",
            m_regCmd.videoChroma.horizontalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma verticalPhase     [0x%x] ",
            m_regCmd.videoChroma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma verticalStripe    [0x%x] ", m_regCmd.videoChroma.verticalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma cropLine          [0x%x] ", m_regCmd.videoChroma.cropLine);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma cropPixel         [0x%x] ", m_regCmd.videoChroma.cropPixel);
    }


    if (DisplayFullOutput == m_output)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma module            [0x%x] ", m_regCmd.dispLuma.module);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma config            [0x%x] ", m_regCmd.dispLuma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma imageSize         [0x%x] ", m_regCmd.dispLuma.imageSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma horizontalPhase   [0x%x] ", m_regCmd.dispLuma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma horizontalStripe  [0x%x] ", m_regCmd.dispLuma.horizontalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma verticalPhase     [0x%x] ", m_regCmd.dispLuma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma verticalStripe    [0x%x] ", m_regCmd.dispLuma.verticalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma cropLine          [0x%x] ", m_regCmd.dispLuma.cropLine);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Luma cropPixel         [0x%x] ", m_regCmd.dispLuma.cropPixel);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma module            [0x%x] ", m_regCmd.dispChroma.module);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma config            [0x%x] ", m_regCmd.dispChroma.config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma imageSize         [0x%x] ", m_regCmd.dispChroma.imageSize);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma horizontalPhase   [0x%x] ", m_regCmd.dispChroma.horizontalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma horizontalStripe  [0x%x] ",
            m_regCmd.dispChroma.horizontalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma verticalPhase     [0x%x] ",
            m_regCmd.dispChroma.verticalPhase);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma verticalStripe    [0x%x] ", m_regCmd.dispChroma.verticalStripe);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma cropLine          [0x%x] ", m_regCmd.dispChroma.cropLine);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MNDS Full Chroma cropPixel         [0x%x] ", m_regCmd.dispChroma.cropPixel);
    }
}

CAMX_NAMESPACE_END
