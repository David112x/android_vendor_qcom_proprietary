////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxiferoundclamp11titan480.cpp
/// @brief CAMXIFEROUNDCLAMP11TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxiferoundclamp11titan480.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::IFERoundClamp11Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFERoundClamp11Titan480::IFERoundClamp11Titan480()
{
    Utils::Memset(&m_regCmd, 0, sizeof(m_regCmd));
    SetCommandLength(
        PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFECRC11RegCmd) / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERoundClamp11Titan480::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CamxResult result        = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer = NULL;

    UINT32     register1;
    UINT32     numberOfValues1;
    UINT32*    pValues1;
    UINT32     register2;
    UINT32     numberOfValues2;
    UINT32*    pValues2;
    UINT32     register3;
    UINT32     numberOfValues3;
    UINT32*    pValues3;
    UINT32     register4;
    UINT32     numberOfValues4;
    UINT32*    pValues4;

    if (NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer   = pInputData->pCmdBuffer;
        m_modulePath = pInputData->modulePath;
        switch (m_modulePath)
        {
            case IFEPipelinePath::FDPath:
                register1       = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_MODULE_CFG;
                numberOfValues1 = 1;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.FDLuma.moduleConfig);

                register2       = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_FD_OUT_CH0_CLAMP_CFG;
                numberOfValues2 = 6;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.FDLuma.ch0ClampConfig);

                register3       = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_MODULE_CFG;
                numberOfValues3 = 1;
                pValues3        = reinterpret_cast<UINT32*>(&m_regCmd.FDChroma.moduleConfig);

                register4       = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_FD_OUT_CH0_CLAMP_CFG;
                numberOfValues4 = 6;
                pValues4        = reinterpret_cast<UINT32*>(&m_regCmd.FDChroma.ch0ClampConfig);
                break;

            case IFEPipelinePath::VideoFullPath:
                register1       = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_MODULE_CFG;
                numberOfValues1 = 1;
                pValues1        = reinterpret_cast<UINT32*>(&m_regCmd.fullLuma.moduleConfig);

                register2       = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_VID_OUT_CH0_CLAMP_CFG;
                numberOfValues2 = 6;
                pValues2        = reinterpret_cast<UINT32*>(&m_regCmd.fullLuma.ch0ClampConfig);

                register3       = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_MODULE_CFG;
                numberOfValues3 = 1;
                pValues3        = reinterpret_cast<UINT32*>(&m_regCmd.fullChroma.moduleConfig);

                register4       = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_VID_OUT_CH0_CLAMP_CFG;
                numberOfValues4 = 6;
                pValues4        = reinterpret_cast<UINT32*>(&m_regCmd.fullChroma.ch0ClampConfig);
                break;

            case IFEPipelinePath::VideoDS4Path:
                register1 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_MODULE_CFG;
                numberOfValues1 = 1;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.DSXLuma.moduleConfig);

                register2 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_Y_VID_OUT_CH0_CLAMP_CFG;
                numberOfValues2 = 6;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.DSXLuma.ch0ClampConfig);

                register3 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_MODULE_CFG;
                numberOfValues3 = 1;
                pValues3 = reinterpret_cast<UINT32*>(&m_regCmd.DSXChroma.moduleConfig);

                register4 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DSX_C_VID_OUT_CH0_CLAMP_CFG;
                numberOfValues4 = 6;
                pValues4 = reinterpret_cast<UINT32*>(&m_regCmd.DSXChroma.ch0ClampConfig);
                break;

            case IFEPipelinePath::VideoDS16Path:
                register1 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_MODULE_CFG;
                numberOfValues1 = 1;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.DS16Luma.moduleConfig);

                register2 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CH0_CLAMP_CFG;
                numberOfValues2 = 6;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.DS16Luma.ch0ClampConfig);

                register3 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_MODULE_CFG;
                numberOfValues3 = 1;
                pValues3 = reinterpret_cast<UINT32*>(&m_regCmd.DS16Chroma.moduleConfig);

                register4 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_VID_DS16_OUT_CH0_CLAMP_CFG;
                numberOfValues4 = 6;
                pValues4 = reinterpret_cast<UINT32*>(&m_regCmd.DS16Chroma.ch0ClampConfig);
                break;
                break;

            case IFEPipelinePath::DisplayFullPath:
                register1 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_MODULE_CFG;
                numberOfValues1 = 1;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.displayLuma.moduleConfig);

                register2 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_Y_DISP_OUT_CH0_CLAMP_CFG;
                numberOfValues2 = 6;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.displayLuma.ch0ClampConfig);

                register3 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_MODULE_CFG;
                numberOfValues3 = 1;
                pValues3 = reinterpret_cast<UINT32*>(&m_regCmd.displayChroma.moduleConfig);

                register4 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_MN_C_DISP_OUT_CH0_CLAMP_CFG;
                numberOfValues4 = 6;
                pValues4 = reinterpret_cast<UINT32*>(&m_regCmd.displayChroma.ch0ClampConfig);
                break;

            case IFEPipelinePath::DisplayDS4Path:
                register1 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_MODULE_CFG;
                numberOfValues1 = 1;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Luma.moduleConfig);

                register2 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CH0_CLAMP_CFG;
                numberOfValues2 = 6;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Luma.ch0ClampConfig);

                register3 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_MODULE_CFG;
                numberOfValues3 = 1;
                pValues3 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Chroma.moduleConfig);

                register4 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CH0_CLAMP_CFG;
                numberOfValues4 = 6;
                pValues4 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Chroma.ch0ClampConfig);
                break;

            case IFEPipelinePath::DisplayDS16Path:
                register1 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_MODULE_CFG;
                numberOfValues1 = 1;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Luma.moduleConfig);

                register2 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CH0_CLAMP_CFG;
                numberOfValues2 = 6;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Luma.ch0ClampConfig);

                register3 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_MODULE_CFG;
                numberOfValues3 = 1;
                pValues3 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Chroma.moduleConfig);

                register4 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_POST_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CH0_CLAMP_CFG;
                numberOfValues4 = 6;
                pValues4 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Chroma.ch0ClampConfig);
                break;

            case IFEPipelinePath::PixelRawDumpPath:
                register1 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_MODULE_CFG;
                numberOfValues1 = 1;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.pixelRaw.moduleConfig);

                register2 = regIFE_IFE_0_PP_CLC_CROP_RND_CLAMP_PIXEL_RAW_OUT_CH0_CLAMP_CFG;
                numberOfValues2 = 6;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.pixelRaw.ch0ClampConfig);

                register3 = 0;
                register4 = 0;
                break;

            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("We would never runinto this case");
                return CamxResultEInvalidState;
                break;
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer, register1, numberOfValues1, pValues1);

            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer, register2, numberOfValues2, pValues2);
            }

            if ((CamxResultSuccess == result) && (0 != register3))
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer, register3, numberOfValues3, pValues3);
            }

            if ((CamxResultSuccess == result) && (0 != register4))
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer, register4, numberOfValues4, pValues4);
            }

            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Failed to configure Crop FD path Register");
            }
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::ConfigureFDPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan480::ConfigureFDPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFECRC11FDLumaReg*   pFDLuma   = &m_regCmd.FDLuma;
    IFECRC11FDChromaReg* pFDChroma = &m_regCmd.FDChroma;

    UINT32 roundOffBits = 0;

    if (BitWidthTen >= m_bitWidth)
    {
        roundOffBits = BitWidthTen - m_bitWidth;
    }

    // For Luma channel only channel0 is valid
    pFDLuma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pFDLuma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pFDLuma->moduleConfig.bitfields.CH1_ROUND_EN = 0;
    pFDLuma->moduleConfig.bitfields.CH1_CLAMP_EN = 0;
    pFDLuma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pFDLuma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    // For Chroma channel channel0 and channel 1 is valid
    pFDChroma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pFDChroma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pFDChroma->moduleConfig.bitfields.CH1_ROUND_EN = 1;
    pFDChroma->moduleConfig.bitfields.CH1_CLAMP_EN = 1;
    pFDChroma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pFDChroma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pFDLuma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN          = minValue;
    pFDLuma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX          = maxValue;
    pFDLuma->ch0RoundConfig.bitfields.CH0_INTERLEAVED        = 0;
    pFDLuma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS     = roundOffBits;
    pFDLuma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
    pFDChroma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN        = minValue;
    pFDChroma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX        = maxValue;
    pFDChroma->ch0RoundConfig.bitfields.CH0_INTERLEAVED      = 1;
    pFDChroma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS   = roundOffBits;
    pFDChroma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN = 0x3;

    pFDLuma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN          = minValue;
    pFDLuma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX          = maxValue;
    pFDLuma->ch1RoundConfig.bitfields.CH1_INTERLEAVED        = 0;
    pFDLuma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS     = roundOffBits;
    pFDLuma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN   = 0x3;
    pFDChroma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN        = minValue;
    pFDChroma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX        = maxValue;
    pFDChroma->ch1RoundConfig.bitfields.CH1_INTERLEAVED      = 1;
    pFDChroma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS   = roundOffBits;
    pFDChroma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::ConfigureFullPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan480::ConfigureFullPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFECRC11FullLumaReg*   pFullLuma   = &m_regCmd.fullLuma;
    IFECRC11FullChromaReg* pFullChroma = &m_regCmd.fullChroma;

    UINT32 roundOffBits = 0;

    if (BitWidthTen >= m_bitWidth)
    {
        roundOffBits = BitWidthTen - m_bitWidth;
    }

    pFullLuma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pFullLuma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pFullLuma->moduleConfig.bitfields.CH1_ROUND_EN = 0;
    pFullLuma->moduleConfig.bitfields.CH1_CLAMP_EN = 0;
    pFullLuma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pFullLuma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pFullChroma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pFullChroma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pFullChroma->moduleConfig.bitfields.CH1_ROUND_EN = 1;
    pFullChroma->moduleConfig.bitfields.CH1_CLAMP_EN = 1;
    pFullChroma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pFullChroma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pFullLuma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN          = minValue;
    pFullLuma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX          = maxValue;
    pFullLuma->ch0RoundConfig.bitfields.CH0_INTERLEAVED        = 0;
    pFullLuma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS     = roundOffBits;
    pFullLuma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
    pFullChroma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN        = minValue;
    pFullChroma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX        = maxValue;
    pFullChroma->ch0RoundConfig.bitfields.CH0_INTERLEAVED      = 1;
    pFullChroma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS   = roundOffBits;
    pFullChroma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN = 0x3;

    pFullLuma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN          = minValue;
    pFullLuma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX          = maxValue;
    pFullLuma->ch1RoundConfig.bitfields.CH1_INTERLEAVED        = 0;
    pFullLuma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS     = roundOffBits;
    pFullLuma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN   = 0x3;
    pFullChroma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN        = minValue;
    pFullChroma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX        = maxValue;
    pFullChroma->ch1RoundConfig.bitfields.CH1_INTERLEAVED      = 1;
    pFullChroma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS   = roundOffBits;
    pFullChroma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::ConfigureDSXPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan480::ConfigureDSXPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFECRC11DSXLumaReg*   pDSXLuma   = &m_regCmd.DSXLuma;
    IFECRC11DSXChromaReg* pDSXChroma = &m_regCmd.DSXChroma;

    UINT32 roundOffBits = 0;

    if (BitWidthTen >= m_bitWidth)
    {
        roundOffBits = BitWidthTen - m_bitWidth;
    }

    pDSXLuma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pDSXLuma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pDSXLuma->moduleConfig.bitfields.CH1_ROUND_EN = 0;
    pDSXLuma->moduleConfig.bitfields.CH1_CLAMP_EN = 0;
    pDSXLuma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pDSXLuma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pDSXChroma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pDSXChroma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pDSXChroma->moduleConfig.bitfields.CH1_ROUND_EN = 1;
    pDSXChroma->moduleConfig.bitfields.CH1_CLAMP_EN = 1;
    pDSXChroma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pDSXChroma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pDSXLuma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN          = minValue;
    pDSXLuma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX          = maxValue;
    pDSXLuma->ch0RoundConfig.bitfields.CH0_INTERLEAVED        = 0;
    pDSXLuma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS     = roundOffBits;
    pDSXLuma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
    pDSXChroma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN        = minValue;
    pDSXChroma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX        = maxValue;
    pDSXChroma->ch0RoundConfig.bitfields.CH0_INTERLEAVED      = 1;
    pDSXChroma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS   = roundOffBits;
    pDSXChroma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN = 0x3;

    pDSXLuma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN          = minValue;
    pDSXLuma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX          = maxValue;
    pDSXLuma->ch1RoundConfig.bitfields.CH1_INTERLEAVED        = 0;
    pDSXLuma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS     = roundOffBits;
    pDSXLuma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN   = 0x3;
    pDSXChroma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN        = minValue;
    pDSXChroma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX        = maxValue;
    pDSXChroma->ch1RoundConfig.bitfields.CH1_INTERLEAVED      = 1;
    pDSXChroma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS   = roundOffBits;
    pDSXChroma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::ConfigureDS16PathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan480::ConfigureDS16PathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFECRC11DS16LumaReg*   pDS16Luma   = &m_regCmd.DS16Luma;
    IFECRC11DS16ChromaReg* pDS16Chroma = &m_regCmd.DS16Chroma;

    UINT32 roundOffBits = 0;

    if (BitWidthTen >= m_bitWidth)
    {
        roundOffBits = BitWidthTen - m_bitWidth;
    }

    pDS16Luma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pDS16Luma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pDS16Luma->moduleConfig.bitfields.CH1_ROUND_EN = 0;
    pDS16Luma->moduleConfig.bitfields.CH1_CLAMP_EN = 0;
    pDS16Luma->moduleConfig.bitfields.CH2_ROUND_EN = 0;
    pDS16Luma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;

    pDS16Chroma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pDS16Chroma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pDS16Chroma->moduleConfig.bitfields.CH1_ROUND_EN = 1;
    pDS16Chroma->moduleConfig.bitfields.CH1_CLAMP_EN = 1;
    pDS16Chroma->moduleConfig.bitfields.CH2_ROUND_EN = 0;
    pDS16Chroma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;

    pDS16Luma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN          = minValue;
    pDS16Luma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX          = maxValue;
    pDS16Luma->ch0RoundConfig.bitfields.CH0_INTERLEAVED        = 0;
    pDS16Luma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS     = roundOffBits;
    pDS16Luma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
    pDS16Chroma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN        = minValue;
    pDS16Chroma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX        = maxValue;
    pDS16Chroma->ch0RoundConfig.bitfields.CH0_INTERLEAVED      = 1;
    pDS16Chroma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS   = roundOffBits;
    pDS16Chroma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN = 0x3;

    pDS16Luma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN          = minValue;
    pDS16Luma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX          = maxValue;
    pDS16Luma->ch1RoundConfig.bitfields.CH1_INTERLEAVED        = 0;
    pDS16Luma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS     = roundOffBits;
    pDS16Luma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN   = 0x3;
    pDS16Chroma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN        = minValue;
    pDS16Chroma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX        = maxValue;
    pDS16Chroma->ch1RoundConfig.bitfields.CH1_INTERLEAVED      = 1;
    pDS16Chroma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS   = roundOffBits;
    pDS16Chroma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::ConfigureDisplayPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan480::ConfigureDisplayPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFECRC11DisplayLumaReg*   pFullLuma   = &m_regCmd.displayLuma;
    IFECRC11DisplayChromaReg* pFullChroma = &m_regCmd.displayChroma;

    UINT32 roundOffBits = 0;

    if (BitWidthTen >= m_bitWidth)
    {
        roundOffBits = BitWidthTen - m_bitWidth;
    }

    pFullLuma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pFullLuma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pFullLuma->moduleConfig.bitfields.CH1_ROUND_EN = 0;
    pFullLuma->moduleConfig.bitfields.CH1_CLAMP_EN = 0;
    pFullLuma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pFullLuma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pFullChroma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pFullChroma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pFullChroma->moduleConfig.bitfields.CH1_ROUND_EN = 1;
    pFullChroma->moduleConfig.bitfields.CH1_CLAMP_EN = 1;
    pFullChroma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pFullChroma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pFullLuma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN          = minValue;
    pFullLuma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX          = maxValue;
    pFullLuma->ch0RoundConfig.bitfields.CH0_INTERLEAVED        = 0;
    pFullLuma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS     = roundOffBits;
    pFullLuma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
    pFullChroma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN        = minValue;
    pFullChroma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX        = maxValue;
    pFullChroma->ch0RoundConfig.bitfields.CH0_INTERLEAVED      = 1;
    pFullChroma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS   = roundOffBits;
    pFullChroma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN = 0x3;

    pFullLuma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN          = minValue;
    pFullLuma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX          = maxValue;
    pFullLuma->ch1RoundConfig.bitfields.CH1_INTERLEAVED        = 0;
    pFullLuma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS     = roundOffBits;
    pFullLuma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN   = 0x3;
    pFullChroma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN        = minValue;
    pFullChroma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX        = maxValue;
    pFullChroma->ch1RoundConfig.bitfields.CH1_INTERLEAVED      = 1;
    pFullChroma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS   = roundOffBits;
    pFullChroma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::ConfigureDS4DisplayPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan480::ConfigureDS4DisplayPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFECRC11DisplayDS4LumaReg*   pDS4Luma   = &m_regCmd.displayDS4Luma;
    IFECRC11DisplayDS4ChromaReg* pDS4Chroma = &m_regCmd.displayDS4Chroma;

    UINT32 roundOffBits = 0;

    if (BitWidthTen >= m_bitWidth)
    {
        roundOffBits = BitWidthTen - m_bitWidth;
    }

    pDS4Luma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pDS4Luma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pDS4Luma->moduleConfig.bitfields.CH1_ROUND_EN = 0;
    pDS4Luma->moduleConfig.bitfields.CH1_CLAMP_EN = 0;
    pDS4Luma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pDS4Luma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pDS4Chroma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pDS4Chroma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pDS4Chroma->moduleConfig.bitfields.CH1_ROUND_EN = 1;
    pDS4Chroma->moduleConfig.bitfields.CH1_CLAMP_EN = 1;
    pDS4Chroma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pDS4Chroma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pDS4Luma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN          = minValue;
    pDS4Luma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX          = maxValue;
    pDS4Luma->ch0RoundConfig.bitfields.CH0_INTERLEAVED        = 0;
    pDS4Luma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS     = roundOffBits;
    pDS4Luma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
    pDS4Chroma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN        = minValue;
    pDS4Chroma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX        = maxValue;
    pDS4Chroma->ch0RoundConfig.bitfields.CH0_INTERLEAVED      = 1;
    pDS4Chroma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS   = roundOffBits;
    pDS4Chroma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN = 0x3;

    pDS4Luma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN          = minValue;
    pDS4Luma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX          = maxValue;
    pDS4Luma->ch1RoundConfig.bitfields.CH1_INTERLEAVED        = 0;
    pDS4Luma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS     = roundOffBits;
    pDS4Luma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN   = 0x3;
    pDS4Chroma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN        = minValue;
    pDS4Chroma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX        = maxValue;
    pDS4Chroma->ch1RoundConfig.bitfields.CH1_INTERLEAVED      = 1;
    pDS4Chroma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS   = roundOffBits;
    pDS4Chroma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::ConfigureDS16DisplayPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan480::ConfigureDS16DisplayPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFECRC11displayDS16LumaReg*   pDS16Luma   = &m_regCmd.displayDS16Luma;
    IFECRC11displayDS16ChromaReg* pDS16Chroma = &m_regCmd.displayDS16Chroma;

    UINT32 roundOffBits = 0;

    if (BitWidthTen >= m_bitWidth)
    {
        roundOffBits = BitWidthTen - m_bitWidth;
    }

    pDS16Luma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pDS16Luma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pDS16Luma->moduleConfig.bitfields.CH1_ROUND_EN = 0;
    pDS16Luma->moduleConfig.bitfields.CH1_CLAMP_EN = 0;
    pDS16Luma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pDS16Luma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pDS16Chroma->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pDS16Chroma->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pDS16Chroma->moduleConfig.bitfields.CH1_ROUND_EN = 1;
    pDS16Chroma->moduleConfig.bitfields.CH1_CLAMP_EN = 1;
    pDS16Chroma->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pDS16Chroma->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pDS16Luma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN          = minValue;
    pDS16Luma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX          = maxValue;
    pDS16Luma->ch0RoundConfig.bitfields.CH0_INTERLEAVED        = 0;
    pDS16Luma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS     = roundOffBits;
    pDS16Luma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
    pDS16Chroma->ch0ClampConfig.bitfields.CH0_CLAMP_MIN        = minValue;
    pDS16Chroma->ch0ClampConfig.bitfields.CH0_CLAMP_MAX        = maxValue;
    pDS16Chroma->ch0RoundConfig.bitfields.CH0_INTERLEAVED      = 1;
    pDS16Chroma->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS   = roundOffBits;
    pDS16Chroma->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN = 0x3;

    pDS16Luma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN          = minValue;
    pDS16Luma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX          = maxValue;
    pDS16Luma->ch1RoundConfig.bitfields.CH1_INTERLEAVED        = 0;
    pDS16Luma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS     = roundOffBits;
    pDS16Luma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN   = 0x3;
    pDS16Chroma->ch1ClampConfig.bitfields.CH1_CLAMP_MIN        = minValue;
    pDS16Chroma->ch1ClampConfig.bitfields.CH1_CLAMP_MAX        = maxValue;
    pDS16Chroma->ch1RoundConfig.bitfields.CH1_INTERLEAVED      = 1;
    pDS16Chroma->ch1RoundConfig.bitfields.CH1_ROUND_OFF_BITS   = roundOffBits;
    pDS16Chroma->ch1RoundConfig.bitfields.CH1_ROUNDING_PATTERN = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::ConfigurePixelRawPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan480::ConfigurePixelRawPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFECRC11PixelRawReg* pPxielRaw = &m_regCmd.pixelRaw;
    UINT32 roundOffBits = 0;

    if (IFEPipelineBitWidth >= m_bitWidth)
    {
        roundOffBits = IFEPipelineBitWidth - m_bitWidth;
    }

    pPxielRaw->moduleConfig.bitfields.CH0_CLAMP_EN = 1;
    pPxielRaw->moduleConfig.bitfields.CH0_ROUND_EN = 1;
    pPxielRaw->moduleConfig.bitfields.CH1_ROUND_EN = 0;
    pPxielRaw->moduleConfig.bitfields.CH1_CLAMP_EN = 0;
    pPxielRaw->moduleConfig.bitfields.CH2_CLAMP_EN = 0;
    pPxielRaw->moduleConfig.bitfields.CH2_ROUND_EN = 0;

    pPxielRaw->ch0ClampConfig.bitfields.CH0_CLAMP_MIN        = minValue;
    pPxielRaw->ch0ClampConfig.bitfields.CH0_CLAMP_MAX        = maxValue;
    pPxielRaw->ch0RoundConfig.bitfields.CH0_INTERLEAVED      = 0;
    pPxielRaw->ch0RoundConfig.bitfields.CH0_ROUND_OFF_BITS   = roundOffBits;
    pPxielRaw->ch0RoundConfig.bitfields.CH0_ROUNDING_PATTERN = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERoundClamp11Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    ISPInputData* pInputData        = static_cast<ISPInputData*>(pInput);
    CamxResult          result      = CamxResultSuccess;
    UINT32              maxValue    = 0;
    UINT32              minValue    = 0;

    m_bitWidth                      = pInputData->bitWidth;
    m_modulePath                    = pInputData->modulePath;

    if (BitWidthEight == m_bitWidth)
    {
        maxValue = Max8BitValue;
    }
    else if (BitWidthTen == m_bitWidth)
    {
        maxValue = Max10BitValue;
    }
    else if (BitWidthFourteen == m_bitWidth)
    {
        maxValue = Max14BitValue;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Bad bitWidth %d, defaulting to 8 bits", m_bitWidth);
        maxValue = Max8BitValue;
    }

    /// @todo (CAMX-666) Configure registers specific to path based on object type
    // Full output  path configuration
    if (IFEPipelinePath::VideoFullPath == m_modulePath)
    {
        ConfigureFullPathRegisters(minValue, maxValue);
        m_regCmd.fullLuma.moduleConfig.bitfields.CROP_RND_CLAMP_EN = 1;
        m_regCmd.fullLuma.moduleConfig.bitfields.CROP_EN =
            pInputData->pCalculatedData->moduleEnable.videoProcessingModules.CropEnable;
        m_regCmd.fullChroma.moduleConfig.bitfields.CROP_RND_CLAMP_EN = 1;
        m_regCmd.fullChroma.moduleConfig.bitfields.CROP_EN =
            pInputData->pCalculatedData->moduleEnable.videoProcessingModules.CropEnable;
    }

    // FD output path configuration
    if (IFEPipelinePath::FDPath == m_modulePath)
    {
        ConfigureFDPathRegisters(minValue, maxValue);
        m_regCmd.FDLuma.moduleConfig.bitfields.CROP_RND_CLAMP_EN   = 1;
        m_regCmd.FDLuma.moduleConfig.bitfields.CROP_EN             =
            pInputData->pCalculatedData->moduleEnable.FDprocessingModules.CropEnable;
        m_regCmd.FDChroma.moduleConfig.bitfields.CROP_RND_CLAMP_EN = 1;
        m_regCmd.FDChroma.moduleConfig.bitfields.CROP_EN           =
            pInputData->pCalculatedData->moduleEnable.FDprocessingModules.CropEnable;
    }

    // DS4 output path configuration
    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        ConfigureDSXPathRegisters(minValue, maxValue);
        m_regCmd.DSXLuma.moduleConfig.bitfields.CROP_RND_CLAMP_EN   = 1;
        m_regCmd.DSXLuma.moduleConfig.bitfields.CROP_EN             =
            pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4PostCropEnable;
        m_regCmd.DSXChroma.moduleConfig.bitfields.CROP_RND_CLAMP_EN = 1;
        m_regCmd.DSXChroma.moduleConfig.bitfields.CROP_EN           =
            pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS4PostCropEnable;
    }

    // DS16 output path configuration
    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        ConfigureDS16PathRegisters(minValue, maxValue);
        m_regCmd.DS16Luma.moduleConfig.bitfields.CROP_RND_CLAMP_EN   = 1;
        m_regCmd.DS16Luma.moduleConfig.bitfields.CROP_EN             =
            pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16PostCropEnable;
        m_regCmd.DS16Chroma.moduleConfig.bitfields.CROP_RND_CLAMP_EN = 1;
        m_regCmd.DS16Chroma.moduleConfig.bitfields.CROP_EN           =
            pInputData->pCalculatedData->moduleEnable.videoProcessingModules.DS16PostCropEnable;
    }

    // Disp output  path configuration
    if (IFEPipelinePath::DisplayFullPath == m_modulePath)
    {
        ConfigureDisplayPathRegisters(minValue, maxValue);
        m_regCmd.displayLuma.moduleConfig.bitfields.CROP_RND_CLAMP_EN   = 1;
        m_regCmd.displayLuma.moduleConfig.bitfields.CROP_EN             =
            pInputData->pCalculatedData->moduleEnable.displayProcessingModules.CropEnable;
        m_regCmd.displayChroma.moduleConfig.bitfields.CROP_RND_CLAMP_EN = 1;
        m_regCmd.displayChroma.moduleConfig.bitfields.CROP_EN           =
            pInputData->pCalculatedData->moduleEnable.displayProcessingModules.CropEnable;
    }

    // DS4 output path configuration
    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        ConfigureDS4DisplayPathRegisters(minValue, maxValue);
        m_regCmd.displayDS4Luma.moduleConfig.bitfields.CROP_RND_CLAMP_EN   = 1;
        m_regCmd.displayDS4Luma.moduleConfig.bitfields.CROP_EN             =
            pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4PostCropEnable;
        m_regCmd.displayDS4Chroma.moduleConfig.bitfields.CROP_RND_CLAMP_EN = 1;
        m_regCmd.displayDS4Chroma.moduleConfig.bitfields.CROP_EN           =
            pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS4PostCropEnable;
    }

    // DS16 output path configuration
    if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        ConfigureDS16DisplayPathRegisters(minValue, maxValue);
        m_regCmd.displayDS16Luma.moduleConfig.bitfields.CROP_RND_CLAMP_EN   = 1;
        m_regCmd.displayDS16Luma.moduleConfig.bitfields.CROP_EN             =
            pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16PostCropEnable;
        m_regCmd.displayDS16Chroma.moduleConfig.bitfields.CROP_RND_CLAMP_EN = 1;
        m_regCmd.displayDS16Chroma.moduleConfig.bitfields.CROP_EN           =
            pInputData->pCalculatedData->moduleEnable.displayProcessingModules.DS16PostCropEnable;
    }

    // Pixel Raw Path
    if (IFEPipelinePath::PixelRawDumpPath == m_modulePath)
    {
        ConfigurePixelRawPathRegisters(minValue, maxValue);
        m_regCmd.pixelRaw.moduleConfig.bitfields.CROP_RND_CLAMP_EN = 1;
        m_regCmd.pixelRaw.moduleConfig.bitfields.CROP_EN =
            pInputData->pCalculatedData->moduleEnable.IQModules.pixelRawPathEnable;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERoundClamp11Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFERoundClamp11Titan480::CopyRegCmd(
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
// IFERoundClamp11Titan480::~IFERoundClamp11Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFERoundClamp11Titan480::~IFERoundClamp11Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan480::DumpRegConfig()
{
    if (IFEPipelinePath::FDPath == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Luma Module config        [0x%x]", m_regCmd.FDLuma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Luma Ch0 Clamp config     [0x%x]", m_regCmd.FDLuma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Luma Ch0 Round config     [0x%x]", m_regCmd.FDLuma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Luma Ch1 Clamp config     [0x%x]", m_regCmd.FDLuma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Luma Ch1 Round config     [0x%x]", m_regCmd.FDLuma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Luma Ch2 Clamp config     [0x%x]", m_regCmd.FDLuma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Luma Ch2 Round config     [0x%x]", m_regCmd.FDLuma.ch2RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Chroma Module config      [0x%x]", m_regCmd.FDChroma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Chroma Ch0 Clamp config   [0x%x]", m_regCmd.FDChroma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Chroma Ch0 Round config   [0x%x]", m_regCmd.FDChroma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Chroma Ch1 Clamp config   [0x%x]", m_regCmd.FDChroma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Chroma Ch1 Round config   [0x%x]", m_regCmd.FDChroma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Chroma Ch2 Clamp config   [0x%x]", m_regCmd.FDChroma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Chroma Ch2 Round config   [0x%x]", m_regCmd.FDChroma.ch2RoundConfig);
    }

    if (IFEPipelinePath::VideoFullPath == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Luma Module config        [0x%x]", m_regCmd.fullLuma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Luma Ch0 Clamp config     [0x%x]", m_regCmd.fullLuma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Luma Ch0 Round config     [0x%x]", m_regCmd.fullLuma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Luma Ch1 Clamp config     [0x%x]", m_regCmd.fullLuma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Luma Ch1 Round config     [0x%x]", m_regCmd.fullLuma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Luma Ch2 Clamp config     [0x%x]", m_regCmd.fullLuma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Luma Ch2 Round config     [0x%x]", m_regCmd.fullLuma.ch2RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Chroma Module config      [0x%x]", m_regCmd.fullChroma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Chroma Ch0 Clamp config   [0x%x]", m_regCmd.fullChroma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Chroma Ch0 Round config   [0x%x]", m_regCmd.fullChroma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Chroma Ch1 Clamp config   [0x%x]", m_regCmd.fullChroma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Chroma Ch1 Round config   [0x%x]", m_regCmd.fullChroma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Chroma Ch2 Clamp config   [0x%x]", m_regCmd.fullChroma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "VID Chroma Ch2 Round config   [0x%x]", m_regCmd.fullChroma.ch2RoundConfig);
    }

    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Module config        [0x%x]", m_regCmd.DSXLuma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Ch0 Clamp config     [0x%x]", m_regCmd.DSXLuma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Ch0 Round config     [0x%x]", m_regCmd.DSXLuma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Ch1 Clamp config     [0x%x]", m_regCmd.DSXLuma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Ch1 Round config     [0x%x]", m_regCmd.DSXLuma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Ch2 Clamp config     [0x%x]", m_regCmd.DSXLuma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Luma Ch2 Round config     [0x%x]", m_regCmd.DSXLuma.ch2RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Chroma Module config      [0x%x]", m_regCmd.DSXChroma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Chroma Ch0 Clamp config   [0x%x]", m_regCmd.DSXChroma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Chroma Ch0 Round config   [0x%x]", m_regCmd.DSXChroma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Chroma Ch1 Clamp config   [0x%x]", m_regCmd.DSXChroma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Chroma Ch1 Round config   [0x%x]", m_regCmd.DSXChroma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Chroma Ch2 Clamp config   [0x%x]", m_regCmd.DSXChroma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DSX Chroma Ch2 Round config   [0x%x]", m_regCmd.DSXChroma.ch2RoundConfig);
    }

    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma Module config        [0x%x]", m_regCmd.DS16Luma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma Ch0 Clamp config     [0x%x]", m_regCmd.DS16Luma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma Ch0 Round config     [0x%x]", m_regCmd.DS16Luma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma Ch1 Clamp config     [0x%x]", m_regCmd.DS16Luma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma Ch1 Round config     [0x%x]", m_regCmd.DS16Luma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma Ch2 Clamp config     [0x%x]", m_regCmd.DS16Luma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma Ch2 Round config     [0x%x]", m_regCmd.DS16Luma.ch2RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Module config      [0x%x]", m_regCmd.DS16Chroma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Ch0 Clamp config   [0x%x]", m_regCmd.DS16Chroma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Ch0 Round config   [0x%x]", m_regCmd.DS16Chroma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Ch1 Clamp config   [0x%x]", m_regCmd.DS16Chroma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Ch1 Round config   [0x%x]", m_regCmd.DS16Chroma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Ch2 Clamp config   [0x%x]", m_regCmd.DS16Chroma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Ch2 Round config   [0x%x]", m_regCmd.DS16Chroma.ch2RoundConfig);
    }

    if (IFEPipelinePath::DisplayFullPath == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Luma Module config        [0x%x]", m_regCmd.displayLuma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Luma Ch0 Clamp config     [0x%x]", m_regCmd.displayLuma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Luma Ch0 Round config     [0x%x]", m_regCmd.displayLuma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Luma Ch1 Clamp config     [0x%x]", m_regCmd.displayLuma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Luma Ch1 Round config     [0x%x]", m_regCmd.displayLuma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Luma Ch2 Clamp config     [0x%x]", m_regCmd.displayLuma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Luma Ch2 Round config     [0x%x]", m_regCmd.displayLuma.ch2RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Chroma Module config      [0x%x]", m_regCmd.displayChroma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Chroma Ch0 Clamp config   [0x%x]", m_regCmd.displayChroma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Chroma Ch0 Round config   [0x%x]", m_regCmd.displayChroma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Chroma Ch1 Clamp config   [0x%x]", m_regCmd.displayChroma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Chroma Ch1 Round config   [0x%x]", m_regCmd.displayChroma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Chroma Ch2 Clamp config   [0x%x]", m_regCmd.displayChroma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS VID Chroma Ch2 Round config   [0x%x]", m_regCmd.displayChroma.ch2RoundConfig);
    }

    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Luma Module config        [0x%x]",
                         m_regCmd.displayDS4Luma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Luma Ch0 Clamp config     [0x%x]",
                         m_regCmd.displayDS4Luma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Luma Ch0 Round config     [0x%x]",
                         m_regCmd.displayDS4Luma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Luma Ch1 Clamp config     [0x%x]",
                         m_regCmd.displayDS4Luma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Luma Ch1 Round config     [0x%x]",
                         m_regCmd.displayDS4Luma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Luma Ch2 Clamp config     [0x%x]",
                         m_regCmd.displayDS4Luma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Luma Ch2 Round config     [0x%x]",
                         m_regCmd.displayDS4Luma.ch2RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Chroma Module config      [0x%x]",
                         m_regCmd.displayDS4Chroma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Chroma Ch0 Clamp config   [0x%x]",
                         m_regCmd.displayDS4Chroma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Chroma Ch0 Round config   [0x%x]",
                         m_regCmd.displayDS4Chroma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Chroma Ch1 Clamp config   [0x%x]",
                         m_regCmd.displayDS4Chroma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Chroma Ch1 Round config   [0x%x]",
                         m_regCmd.displayDS4Chroma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Chroma Ch2 Clamp config   [0x%x]",
                         m_regCmd.displayDS4Chroma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS4 Chroma Ch2 Round config   [0x%x]",
                         m_regCmd.displayDS4Chroma.ch2RoundConfig);
    }

    if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Luma Module config        [0x%x]",
            m_regCmd.displayDS16Luma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Luma Ch0 Clamp config     [0x%x]",
            m_regCmd.displayDS16Luma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Luma Ch0 Round config     [0x%x]",
            m_regCmd.displayDS16Luma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Luma Ch1 Clamp config     [0x%x]",
            m_regCmd.displayDS16Luma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Luma Ch1 Round config     [0x%x]",
            m_regCmd.displayDS16Luma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Luma Ch2 Clamp config     [0x%x]",
            m_regCmd.displayDS16Luma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Luma Ch2 Round config     [0x%x]",
            m_regCmd.displayDS16Luma.ch2RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Chroma Module config      [0x%x]",
            m_regCmd.displayDS16Chroma.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Chroma Ch0 Clamp config   [0x%x]",
            m_regCmd.displayDS16Chroma.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Chroma Ch0 Round config   [0x%x]",
            m_regCmd.displayDS16Chroma.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Chroma Ch1 Clamp config   [0x%x]",
            m_regCmd.displayDS16Chroma.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Chroma Ch1 Round config   [0x%x]",
            m_regCmd.displayDS16Chroma.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Chroma Ch2 Clamp config   [0x%x]",
            m_regCmd.displayDS16Chroma.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DIS DS16 Chroma Ch2 Round config   [0x%x]",
            m_regCmd.displayDS16Chroma.ch2RoundConfig);
    }

    if (IFEPipelinePath::PixelRawDumpPath == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel Raw Module config        [0x%x]", m_regCmd.pixelRaw.moduleConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel Raw Ch0 Clamp config     [0x%x]", m_regCmd.pixelRaw.ch0ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel Raw Ch0 Round config     [0x%x]", m_regCmd.pixelRaw.ch0RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel Raw Ch1 Clamp config     [0x%x]", m_regCmd.pixelRaw.ch1ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel Raw Ch1 Round config     [0x%x]", m_regCmd.pixelRaw.ch1RoundConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel Raw Ch2 Clamp config     [0x%x]", m_regCmd.pixelRaw.ch2ClampConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Pixel Raw Ch2 Round config     [0x%x]", m_regCmd.pixelRaw.ch2RoundConfig);
    }
}

CAMX_NAMESPACE_END
