////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxiferoundclamp11titan17x.cpp
/// @brief CAMXIFEROUNDCLAMP11TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxiferoundclamp11titan17x.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::IFERoundClamp11Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFERoundClamp11Titan17x::IFERoundClamp11Titan17x()
{
    Utils::Memset(&m_regCmd, 0, sizeof(m_regCmd));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERoundClamp11Titan17x::CreateCmdList(
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

    if (NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer   = pInputData->pCmdBuffer;
        m_modulePath = pInputData->modulePath;
        switch (m_modulePath)
        {
            case IFEPipelinePath::FDPath:
                register1 = regIFE_IFE_0_VFE_FD_OUT_Y_CH0_CLAMP_CFG;
                numberOfValues1 = sizeof(IFERoundClamp11FDLumaReg) / RegisterWidthInBytes;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.FDLuma);

                register2 = regIFE_IFE_0_VFE_FD_OUT_C_CH0_CLAMP_CFG;
                numberOfValues2 = sizeof(IFERoundClamp11FDChromaReg) / RegisterWidthInBytes;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.FDChroma);
                break;

            case IFEPipelinePath::VideoFullPath:
                register1 = regIFE_IFE_0_VFE_FULL_OUT_Y_CH0_CLAMP_CFG;
                numberOfValues1 = (sizeof(IFERoundClamp11FullLumaReg) / RegisterWidthInBytes);
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.fullLuma);

                register2 = regIFE_IFE_0_VFE_FULL_OUT_C_CH0_CLAMP_CFG;
                numberOfValues2 = sizeof(IFERoundClamp11FullChromaReg) / RegisterWidthInBytes;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.fullChroma);
                break;

            case IFEPipelinePath::VideoDS4Path:
                register1 = regIFE_IFE_0_VFE_DS4_Y_CH0_CLAMP_CFG;
                numberOfValues1 = sizeof(IFERoundClamp11DS4LumaReg) / RegisterWidthInBytes;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.DS4Luma);

                register2 = regIFE_IFE_0_VFE_DS4_C_CH0_CLAMP_CFG;
                numberOfValues2 = sizeof(IFERoundClamp11DS4ChromaReg) / RegisterWidthInBytes;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.DS4Chroma);;
                break;

            case IFEPipelinePath::VideoDS16Path:
                register1 = regIFE_IFE_0_VFE_DS16_Y_CH0_CLAMP_CFG;
                numberOfValues1 = sizeof(IFERoundClamp11DS16LumaReg) / RegisterWidthInBytes;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.DS16Luma);

                register2 = regIFE_IFE_0_VFE_DS16_C_CH0_CLAMP_CFG;
                numberOfValues2 = sizeof(IFERoundClamp11DS16ChromaReg) / RegisterWidthInBytes;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.DS16Chroma);
                break;

            case IFEPipelinePath::DisplayFullPath:
                register1 = regIFE_IFE_0_VFE_DISP_OUT_Y_CH0_CLAMP_CFG;
                numberOfValues1 = (sizeof(IFERoundClamp11DisplayLumaReg) / RegisterWidthInBytes);
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.displayLuma);

                register2 = regIFE_IFE_0_VFE_DISP_OUT_C_CH0_CLAMP_CFG;
                numberOfValues2 = sizeof(IFERoundClamp11DisplayChromaReg) / RegisterWidthInBytes;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.displayChroma);
                break;

            case IFEPipelinePath::DisplayDS4Path:
                register1 = regIFE_IFE_0_VFE_DISP_DS4_Y_CH0_CLAMP_CFG;
                numberOfValues1 = sizeof(IFERoundClamp11DisplayDS4LumaReg) / RegisterWidthInBytes;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Luma);

                register2 = regIFE_IFE_0_VFE_DISP_DS4_C_CH0_CLAMP_CFG;
                numberOfValues2 = sizeof(IFERoundClamp11DisplayDS4ChromaReg) / RegisterWidthInBytes;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS4Chroma);;
                break;

            case IFEPipelinePath::DisplayDS16Path:
                register1 = regIFE_IFE_0_VFE_DISP_DS16_Y_CH0_CLAMP_CFG;
                numberOfValues1 = sizeof(IFERoundClamp11displayDS16LumaReg) / RegisterWidthInBytes;
                pValues1 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Luma);

                register2 = regIFE_IFE_0_VFE_DISP_DS16_C_CH0_CLAMP_CFG;
                numberOfValues2 = sizeof(IFERoundClamp11displayDS16ChromaReg) / RegisterWidthInBytes;
                pValues2 = reinterpret_cast<UINT32*>(&m_regCmd.displayDS16Chroma);
                break;

            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("We would never runinto this case");
                return CamxResultEInvalidState;
                break;
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer, register1, numberOfValues1, pValues1);

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer, register2, numberOfValues2, pValues2);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to configure Crop FD path Register");
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
// IFERoundClamp11Titan17x::ConfigureFDPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan17x::ConfigureFDPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFERoundClamp11FDLumaReg*   pFDLuma             = &m_regCmd.FDLuma;
    IFERoundClamp11FDChromaReg* pFDChroma           = &m_regCmd.FDChroma;

    pFDLuma->clamp.bitfields.CH0_CLAMP_MIN          = minValue;
    pFDLuma->clamp.bitfields.CH0_CLAMP_MAX          = maxValue;
    pFDLuma->round.bitfields.CH0_INTERLEAVED        = 0;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pFDLuma->round.bitfields.CH0_ROUND_OFF_BITS     = BitWidthTen - m_bitWidth;
    /// @todo (CAMX-666) Update rounding pattren from Chromatix
    pFDLuma->round.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
    pFDChroma->clamp.bitfields.CH0_CLAMP_MIN        = minValue;
    pFDChroma->clamp.bitfields.CH0_CLAMP_MAX        = maxValue;
    pFDChroma->round.bitfields.CH0_INTERLEAVED      = 1;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pFDChroma->round.bitfields.CH0_ROUND_OFF_BITS   = BitWidthTen - m_bitWidth;
    /// @todo (CAMX-666) Update rounding pattren from Chromatix
    pFDChroma->round.bitfields.CH0_ROUNDING_PATTERN = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::ConfigureFullPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan17x::ConfigureFullPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFERoundClamp11FullLumaReg*   pFullLuma             = &m_regCmd.fullLuma;
    IFERoundClamp11FullChromaReg* pFullChroma           = &m_regCmd.fullChroma;

    pFullLuma->clamp.bitfields.CH0_CLAMP_MIN            = minValue;
    pFullLuma->clamp.bitfields.CH0_CLAMP_MAX            = maxValue;
    pFullLuma->round.bitfields.CH0_INTERLEAVED          = 0;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pFullLuma->round.bitfields.CH0_ROUND_OFF_BITS       = BitWidthTen - m_bitWidth;
    /// @todo (CAMX-666) Update rounding pattren from Chromatix
    pFullLuma->round.bitfields.CH0_ROUNDING_PATTERN     = 0x3;
    pFullChroma->clamp.bitfields.CH0_CLAMP_MIN          = minValue;
    pFullChroma->clamp.bitfields.CH0_CLAMP_MAX          = maxValue;
    pFullChroma->round.bitfields.CH0_INTERLEAVED        = 1;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pFullChroma->round.bitfields.CH0_ROUND_OFF_BITS     = BitWidthTen - m_bitWidth;
    /// @todo (CAMX-666) Update rounding pattren from Chromatix
    pFullChroma->round.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::ConfigureDS4PathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan17x::ConfigureDS4PathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFERoundClamp11DS4LumaReg*   pDS4Luma               = &m_regCmd.DS4Luma;
    IFERoundClamp11DS4ChromaReg* pDS4Chroma             = &m_regCmd.DS4Chroma;

    pDS4Luma->clamp.bitfields.CH0_CLAMP_MIN             = minValue;
    pDS4Luma->clamp.bitfields.CH0_CLAMP_MAX             = maxValue;
    pDS4Luma->round.bitfields.CH0_INTERLEAVED           = 0;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pDS4Luma->round.bitfields.CH0_ROUND_OFF_BITS        = BitWidthTen - m_bitWidth;
    pDS4Luma->round.bitfields.CH0_ROUNDING_PATTERN      = 0x3;
    pDS4Chroma->clamp.bitfields.CH0_CLAMP_MIN           = minValue;
    pDS4Chroma->clamp.bitfields.CH0_CLAMP_MAX           = maxValue;
    pDS4Chroma->round.bitfields.CH0_INTERLEAVED         = 1;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pDS4Chroma->round.bitfields.CH0_ROUND_OFF_BITS      = BitWidthTen - m_bitWidth;
    /// @todo (CAMX-666) Update rounding pattren from Chromatix
    pDS4Chroma->round.bitfields.CH0_ROUNDING_PATTERN    = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::ConfigureDS16PathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan17x::ConfigureDS16PathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFERoundClamp11DS16LumaReg*   pDS16Luma             = &m_regCmd.DS16Luma;
    IFERoundClamp11DS16ChromaReg* pDS16Chroma           = &m_regCmd.DS16Chroma;

    pDS16Luma->clamp.bitfields.CH0_CLAMP_MIN            = minValue;
    pDS16Luma->clamp.bitfields.CH0_CLAMP_MAX            = maxValue;
    pDS16Luma->round.bitfields.CH0_INTERLEAVED          = 0;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pDS16Luma->round.bitfields.CH0_ROUND_OFF_BITS       = BitWidthTen - m_bitWidth;
    pDS16Luma->round.bitfields.CH0_ROUNDING_PATTERN     = 0x3;
    pDS16Chroma->clamp.bitfields.CH0_CLAMP_MIN          = minValue;
    pDS16Chroma->clamp.bitfields.CH0_CLAMP_MAX          = maxValue;
    pDS16Chroma->round.bitfields.CH0_INTERLEAVED        = 1;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pDS16Chroma->round.bitfields.CH0_ROUND_OFF_BITS     = BitWidthTen - m_bitWidth;
    /// @todo (CAMX-666) Update rounding pattren from Chromatix
    pDS16Chroma->round.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::ConfigureDisplayPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan17x::ConfigureDisplayPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFERoundClamp11DisplayLumaReg*   pFullLuma          = &m_regCmd.displayLuma;
    IFERoundClamp11DisplayChromaReg* pFullChroma        = &m_regCmd.displayChroma;

    pFullLuma->clamp.bitfields.CH0_CLAMP_MIN            = minValue;
    pFullLuma->clamp.bitfields.CH0_CLAMP_MAX            = maxValue;
    pFullLuma->round.bitfields.CH0_INTERLEAVED          = 0;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pFullLuma->round.bitfields.CH0_ROUND_OFF_BITS       = BitWidthTen - m_bitWidth;
    /// @todo (CAMX-666) Update rounding pattren from Chromatix
    pFullLuma->round.bitfields.CH0_ROUNDING_PATTERN     = 0x3;
    pFullChroma->clamp.bitfields.CH0_CLAMP_MIN          = minValue;
    pFullChroma->clamp.bitfields.CH0_CLAMP_MAX          = maxValue;
    pFullChroma->round.bitfields.CH0_INTERLEAVED        = 1;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pFullChroma->round.bitfields.CH0_ROUND_OFF_BITS     = BitWidthTen - m_bitWidth;
    /// @todo (CAMX-666) Update rounding pattren from Chromatix
    pFullChroma->round.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::ConfigureDS4DisplayPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan17x::ConfigureDS4DisplayPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFERoundClamp11DisplayDS4LumaReg*   pDS4Luma        = &m_regCmd.displayDS4Luma;
    IFERoundClamp11DisplayDS4ChromaReg* pDS4Chroma      = &m_regCmd.displayDS4Chroma;

    pDS4Luma->clamp.bitfields.CH0_CLAMP_MIN             = minValue;
    pDS4Luma->clamp.bitfields.CH0_CLAMP_MAX             = maxValue;
    pDS4Luma->round.bitfields.CH0_INTERLEAVED           = 0;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pDS4Luma->round.bitfields.CH0_ROUND_OFF_BITS        = BitWidthTen - m_bitWidth;
    pDS4Luma->round.bitfields.CH0_ROUNDING_PATTERN      = 0x3;
    pDS4Chroma->clamp.bitfields.CH0_CLAMP_MIN           = minValue;
    pDS4Chroma->clamp.bitfields.CH0_CLAMP_MAX           = maxValue;
    pDS4Chroma->round.bitfields.CH0_INTERLEAVED         = 1;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pDS4Chroma->round.bitfields.CH0_ROUND_OFF_BITS      = BitWidthTen - m_bitWidth;
    /// @todo (CAMX-666) Update rounding pattren from Chromatix
    pDS4Chroma->round.bitfields.CH0_ROUNDING_PATTERN    = 0x3;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::ConfigureDS16DisplayPathRegisters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan17x::ConfigureDS16DisplayPathRegisters(
    UINT32        minValue,
    UINT32        maxValue)
{
    IFERoundClamp11displayDS16LumaReg*   pDS16Luma      = &m_regCmd.displayDS16Luma;
    IFERoundClamp11displayDS16ChromaReg* pDS16Chroma    = &m_regCmd.displayDS16Chroma;

    pDS16Luma->clamp.bitfields.CH0_CLAMP_MIN            = minValue;
    pDS16Luma->clamp.bitfields.CH0_CLAMP_MAX            = maxValue;
    pDS16Luma->round.bitfields.CH0_INTERLEAVED          = 0;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pDS16Luma->round.bitfields.CH0_ROUND_OFF_BITS       = BitWidthTen - m_bitWidth;
    pDS16Luma->round.bitfields.CH0_ROUNDING_PATTERN     = 0x3;
    pDS16Chroma->clamp.bitfields.CH0_CLAMP_MIN          = minValue;
    pDS16Chroma->clamp.bitfields.CH0_CLAMP_MAX          = maxValue;
    pDS16Chroma->round.bitfields.CH0_INTERLEAVED        = 1;
    /// @todo (CAMX-666) Update round off bits based on input and output bits per pixel
    pDS16Chroma->round.bitfields.CH0_ROUND_OFF_BITS     = BitWidthTen - m_bitWidth;
    /// @todo (CAMX-666) Update rounding pattren from Chromatix
    pDS16Chroma->round.bitfields.CH0_ROUNDING_PATTERN   = 0x3;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERoundClamp11Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);
    ISPInputData* pInputData        = static_cast<ISPInputData*>(pInput);
    CamxResult          result      = CamxResultSuccess;
    UINT32              maxValue    = 0;
    UINT32              minValue    = 0;

    m_bitWidth                      = pInputData->bitWidth;

    CAMX_ASSERT((m_bitWidth == BitWidthEight) || (m_bitWidth == BitWidthTen));

    if (BitWidthEight == m_bitWidth)
    {
        maxValue = Max8BitValue;
    }
    else if (BitWidthTen == m_bitWidth)
    {
        maxValue = Max10BitValue;
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
    }

    // FD output path configuration
    if (IFEPipelinePath::FDPath == m_modulePath)
    {
        ConfigureFDPathRegisters(minValue, maxValue);
    }

    // DS4 output path configuration
    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        ConfigureDS4PathRegisters(minValue, maxValue);
    }

    // DS16 output path configuration
    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        ConfigureDS16PathRegisters(minValue, maxValue);
    }

    // Disp output  path configuration
    if (IFEPipelinePath::DisplayFullPath == m_modulePath)
    {
        ConfigureDisplayPathRegisters(minValue, maxValue);
    }

    // DS4 output path configuration
    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        ConfigureDS4DisplayPathRegisters(minValue, maxValue);
    }

    // DS16 output path configuration
    if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        ConfigureDS16DisplayPathRegisters(minValue, maxValue);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFERoundClamp11Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFERoundClamp11Titan17x::CopyRegCmd(
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
// IFERoundClamp11Titan17x::~IFERoundClamp11Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFERoundClamp11Titan17x::~IFERoundClamp11Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFERoundClamp11Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFERoundClamp11Titan17x::DumpRegConfig()
{
    if (IFEPipelinePath::FDPath == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Luma Clamp config   [0x%x]", m_regCmd.FDLuma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Luma Round config   [0x%x]", m_regCmd.FDLuma.round);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Chroma Clamp config [0x%x]", m_regCmd.FDChroma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "FD Chroma Round config [0x%x]", m_regCmd.FDChroma.round);
    }

    if (IFEPipelinePath::VideoFullPath == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Full Luma Clamp config   [0x%x]", m_regCmd.fullLuma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Full Luma Round config   [0x%x]", m_regCmd.fullLuma.round);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Full Chroma Clamp config [0x%x]", m_regCmd.fullChroma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Full Chroma Round config [0x%x]", m_regCmd.fullChroma.round);
    }

    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Luma Clamp config   [0x%x]", m_regCmd.DS4Luma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Luma Round config   [0x%x]", m_regCmd.DS4Luma.round);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Chroma Clamp config [0x%x]", m_regCmd.DS4Chroma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Chroma Round config [0x%x]", m_regCmd.DS4Chroma.round);
    }

    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma Clamp config   [0x%x]", m_regCmd.DS16Luma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Luma Round config   [0x%x]", m_regCmd.DS16Luma.round);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Clamp config [0x%x]", m_regCmd.DS16Chroma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Round config [0x%x]", m_regCmd.DS16Chroma.round);
    }

    if (IFEPipelinePath::DisplayFullPath == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display Luma Clamp config   [0x%x]", m_regCmd.displayLuma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display Luma Round config   [0x%x]", m_regCmd.displayLuma.round);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display Chroma Clamp config [0x%x]", m_regCmd.displayChroma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display Chroma Round config [0x%x]", m_regCmd.displayChroma.round);
    }

    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS4 Luma Clamp config   [0x%x]", m_regCmd.displayDS4Luma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS4 Luma Round config   [0x%x]", m_regCmd.displayDS4Luma.round);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS4 Chroma Clamp config [0x%x]", m_regCmd.displayDS4Chroma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS4 Chroma Round config [0x%x]", m_regCmd.displayDS4Chroma.round);
    }

    if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS16 Luma Clamp config   [0x%x]", m_regCmd.displayDS16Luma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS16 Luma Round config   [0x%x]", m_regCmd.displayDS16Luma.round);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS16 Chroma Clamp config [0x%x]", m_regCmd.displayDS16Chroma.clamp);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS16 Chroma Round config [0x%x]", m_regCmd.displayDS16Chroma.round);
    }
}

CAMX_NAMESPACE_END
