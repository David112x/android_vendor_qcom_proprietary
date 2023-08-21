////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeds410titan17x.cpp
/// @brief CAMXIFEDS410TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifeds410titan17x.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410Titan17x::IFEDS410Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDS410Titan17x::IFEDS410Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS4Reg)  / RegisterWidthInBytes) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS16Reg) / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS410Titan17x::CreateCmdList(
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
        // Write DS4 output path registers
        if ((IFEPipelinePath::VideoDS4Path == m_modulePath))
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_DS_4TO1_Y_1ST_CFG,
                                                  (sizeof(IFEDS4Reg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.DS4));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure (Video) DS4 path Register");
            }
        }

        // Write DS16 output path registers
        if (IFEPipelinePath::VideoDS16Path == m_modulePath)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_DS_4TO1_Y_2ND_CFG,
                                                  (sizeof(IFEDS16Reg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.DS16));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure (Video) DS16 path Register");
            }
        }

        // Write Display DS4 output path registers
        if ((IFEPipelinePath::DisplayDS4Path == m_modulePath))
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_DISP_DS_4TO1_Y_1ST_CFG,
                                                  (sizeof(IFEDisplayDS4Reg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.displayDS4));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure Display DS4 path Register");
            }
        }

        // Write Display DS16 output path registers
        if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_DISP_DS_4TO1_Y_2ND_CFG,
                                                  (sizeof(IFEDisplayDS16Reg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.displayDS16));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure Display DS16 path Register");
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410Titan17x::ConfigureDS4Registers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS410Titan17x::ConfigureDS4Registers(
    DS410InputData* pData)
{
    /// @todo (CAMX-919) Replace hardcoded filter coefficients from Chromatix
    // Flush count is updated as per HLD guidelines
    m_regCmd.DS4.lumaConfig.bitfields.FLUSH_PACE_CNT   = 3;

    m_regCmd.DS4.lumaConfig.bitfields.HEIGHT           = Utils::AlignGeneric32(pData->stream_height, 2);
    m_regCmd.DS4.coefficients.bitfields.COEFF_07       =
        pData->pDS4Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_07;
    m_regCmd.DS4.coefficients.bitfields.COEFF_16       =
        pData->pDS4Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_16;
    m_regCmd.DS4.coefficients.bitfields.COEFF_25       =
        pData->pDS4Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_25;
    m_regCmd.DS4.chromaConfig.bitfields.FLUSH_PACE_CNT = 3;
    // Input to this module is YUV420
    m_regCmd.DS4.chromaConfig.bitfields.HEIGHT         = m_regCmd.DS4.lumaConfig.bitfields.HEIGHT / YUV420SubsampleFactor;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "<Scaler> DS4 path Y_Height %d Cr_Height %d",
                     m_regCmd.DS4.lumaConfig.bitfields.HEIGHT,
                     m_regCmd.DS4.chromaConfig.bitfields.HEIGHT);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410Titan17x::ConfigureDS16Registers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS410Titan17x::ConfigureDS16Registers(
    DS410InputData* pData)
{
    /// @todo (CAMX-919) Replace hardcoded filter coefficients from Chromatix
    // Flush count is updated as per HLD guidelines
    m_regCmd.DS16.lumaConfig.bitfields.FLUSH_PACE_CNT   = 0xf;

    // Input to DS16 is downsampled by DS4 by 1:4
    m_regCmd.DS16.lumaConfig.bitfields.HEIGHT           = Utils::AlignGeneric32(pData->stream_height, 2);

    m_regCmd.DS16.coefficients.bitfields.COEFF_07       =
        pData->pDS16Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_07;
    m_regCmd.DS16.coefficients.bitfields.COEFF_16       =
        pData->pDS16Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_16;
    m_regCmd.DS16.coefficients.bitfields.COEFF_25       =
        pData->pDS16Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_25;
    m_regCmd.DS16.chromaConfig.bitfields.FLUSH_PACE_CNT = 0xf;
    // Input to this module is YUV420 and need to account for downscale by DS4
    m_regCmd.DS16.chromaConfig.bitfields.HEIGHT         = m_regCmd.DS16.lumaConfig.bitfields.HEIGHT / YUV420SubsampleFactor;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "<Scaler> DS16 path Y_Height %d Cr_Height %d",
                     m_regCmd.DS16.lumaConfig.bitfields.HEIGHT,
                     m_regCmd.DS16.chromaConfig.bitfields.HEIGHT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410Titan17x::ConfigureDisplayDS4Registers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS410Titan17x::ConfigureDisplayDS4Registers(
    DS410InputData* pData)
{
    /// @todo (CAMX-919) Replace hardcoded filter coefficients from Chromatix
    // Flush count is updated as per HLD guidelines
    m_regCmd.displayDS4.lumaConfig.bitfields.FLUSH_PACE_CNT   = 3;

    m_regCmd.displayDS4.lumaConfig.bitfields.HEIGHT           = Utils::AlignGeneric32(pData->stream_height, 2);

    /// @todo (CAMX-4027) Changes for new IFE display o/p. Is chromatix the same ?
    m_regCmd.displayDS4.coefficients.bitfields.COEFF_07 =
        pData->pDS4Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_07;
    m_regCmd.displayDS4.coefficients.bitfields.COEFF_16 =
        pData->pDS4Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_16;
    m_regCmd.displayDS4.coefficients.bitfields.COEFF_25 =
        pData->pDS4Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_25;
    m_regCmd.displayDS4.chromaConfig.bitfields.FLUSH_PACE_CNT = 3;
    // Input to this module is YUV420
    m_regCmd.displayDS4.chromaConfig.bitfields.HEIGHT         = m_regCmd.displayDS4.lumaConfig.bitfields.HEIGHT /
                                                                YUV420SubsampleFactor;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "<Scaler> Display DS4 path Y_Height %d Cr_Height %d",
        m_regCmd.displayDS4.lumaConfig.bitfields.HEIGHT,
        m_regCmd.displayDS4.chromaConfig.bitfields.HEIGHT);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410Titan17x::ConfigureDisplayDS16Registers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS410Titan17x::ConfigureDisplayDS16Registers(
    DS410InputData* pData)
{
    /// @todo (CAMX-919) Replace hardcoded filter coefficients from Chromatix
    // Flush count is updated as per HLD guidelines
    m_regCmd.displayDS16.lumaConfig.bitfields.FLUSH_PACE_CNT   = 0xf;

    // Input to DS16Disp is downsampled by DS4 by 1:4
    m_regCmd.displayDS16.lumaConfig.bitfields.HEIGHT           = Utils::AlignGeneric32(pData->stream_height, 2);

    /// @todo (CAMX-4027) Changes for new IFE display o/p. Is chromatix the same ?
    m_regCmd.displayDS16.coefficients.bitfields.COEFF_07        =
        pData->pDS16Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_07;
    m_regCmd.displayDS16.coefficients.bitfields.COEFF_16        =
        pData->pDS16Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_16;
    m_regCmd.displayDS16.coefficients.bitfields.COEFF_25        =
        pData->pDS16Chromatix->chromatix_ds4to1v10_reserve.mod_ds4to1v10_pass_reserve_data->pass_data.coeff_25;
    m_regCmd.displayDS16.chromaConfig.bitfields.FLUSH_PACE_CNT  = 0xf;
    // Input to this module is YUV420 and need to account for downscale by DS4
    m_regCmd.displayDS16.chromaConfig.bitfields.HEIGHT          = m_regCmd.displayDS16.lumaConfig.bitfields.HEIGHT /
                                                                  YUV420SubsampleFactor;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "<Scaler> Display DS16 path Y_Height %d Cr_Height %d",
        m_regCmd.displayDS16.lumaConfig.bitfields.HEIGHT,
        m_regCmd.displayDS16.chromaConfig.bitfields.HEIGHT);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS410Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result  = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pInput)
    {
        DS410InputData* pData = static_cast<DS410InputData*>(pInput);

        m_modulePath = static_cast<CamX::IFEPipelinePath>(pData->modulePath);

        switch (m_modulePath)
        {
            case IFEPipelinePath::VideoDS4Path:
                ConfigureDS4Registers(pData);
                break;
            case IFEPipelinePath::VideoDS16Path:
                ConfigureDS16Registers(pData);
                break;
            case IFEPipelinePath::DisplayDS4Path:
                ConfigureDisplayDS4Registers(pData);
                break;
            case IFEPipelinePath::DisplayDS16Path:
                ConfigureDisplayDS16Registers(pData);
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupISP, "No pipe line path");
                result = CamxResultEInvalidArg;
                break;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS410Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_ERROR(CamxLogGroupIQMod, "Not Implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410Titan17x::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEDS410Titan17x::CopyRegCmd(
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
// IFEDS410Titan17x::~IFEDS410Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDS410Titan17x::~IFEDS410Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS410Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS410Titan17x::DumpRegConfig()
{
    if (IFEPipelinePath::VideoDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 lumaConfig          [0x%x] ", m_regCmd.DS4.lumaConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Filter coefficients [0x%x] ", m_regCmd.DS4.coefficients);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 chromaConfig        [0x%x] ", m_regCmd.DS4.chromaConfig);
    }

    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 lumaConfig          [0x%x] ", m_regCmd.DS16.lumaConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Filter coefficients [0x%x] ", m_regCmd.DS16.coefficients);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 chromaConfig        [0x%x] ", m_regCmd.DS16.chromaConfig);
    }

    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS4 lumaConfig          [0x%x] ", m_regCmd.displayDS4.lumaConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS4 Filter coefficients [0x%x] ", m_regCmd.displayDS4.coefficients);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS4 chromaConfig        [0x%x] ", m_regCmd.displayDS4.chromaConfig);
    }

    if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS16 lumaConfig          [0x%x] ", m_regCmd.displayDS16.lumaConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS16 Filter coefficients [0x%x] ", m_regCmd.displayDS16.coefficients);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Display DS16 chromaConfig        [0x%x] ", m_regCmd.displayDS16.chromaConfig);
    }
}

CAMX_NAMESPACE_END
