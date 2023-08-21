////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifeds411titan480.cpp
/// @brief CAMXIFEDS411TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifeds411titan480.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411Titan480::IFEDS411Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDS411Titan480::IFEDS411Titan480()
{
    SetCommandLength(
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS4DisplayLumaReg)        / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS4DisplayLumaCropReg)    / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS4DisplayChromaReg)      / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS4DisplayChromaCropReg)  / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS16DisplayLumaReg)       / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS16DisplayLumaCropReg)   / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS16DisplayChromaReg)     / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS16DisplayChromaCropReg) / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS16VideoLumaReg)         / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS16VideoLumaCropReg)     / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS16VideoChromaReg)       / RegisterWidthInBytes) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEDS16VideoChromaCropReg)   / RegisterWidthInBytes));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411Titan480::ConfigureDS4Registers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS411Titan480::ConfigureDS4Registers(
    DS411InputData* pData)
{
    CAMX_UNREFERENCED_PARAM(pData);

    m_regCmd.displayDS4.luma.lumaModuleConfig.bitfields.EN                  = 1;
    m_regCmd.displayDS4.luma.lumaModuleConfig.bitfields.CROP_EN             = 1;
    m_regCmd.displayDS4.luma.lumaModuleConfig.bitfields.SCALE_EN            = 1;
    m_regCmd.displayDS4.luma.lumaModuleConfig.bitfields.FLUSH_PACE_CNT      = 3;
    m_regCmd.displayDS4.luma.coefficients.bitfields.COEFF_07                = 125;
    m_regCmd.displayDS4.luma.coefficients.bitfields.COEFF_16                = 91;
    m_regCmd.displayDS4.luma.coefficients.bitfields.COEFF_25                = 144;

    //  DS4 pre-corp config is same as MNDS post crop, when pre-crop enabled in DS4 module
    m_regCmd.displayDS4.lumaCrop.pixelConfig.bitfields.FIRST_PIXEL = m_pState->preCropInfo.YCrop.firstPixel;
    m_regCmd.displayDS4.lumaCrop.pixelConfig.bitfields.LAST_PIXEL  = m_pState->preCropInfo.YCrop.lastPixel;
    m_regCmd.displayDS4.lumaCrop.lineConfig.bitfields.FIRST_LINE   = m_pState->preCropInfo.YCrop.firstLine;
    m_regCmd.displayDS4.lumaCrop.lineConfig.bitfields.LAST_LINE    = m_pState->preCropInfo.YCrop.lastLine;


    m_regCmd.displayDS4.chroma.chromaModuleConfig.bitfields.EN              = 1;
    m_regCmd.displayDS4.chroma.chromaModuleConfig.bitfields.CROP_EN         = 1;
    m_regCmd.displayDS4.chroma.chromaModuleConfig.bitfields.SCALE_EN        = 1;
    m_regCmd.displayDS4.chroma.chromaModuleConfig.bitfields.FLUSH_PACE_CNT  = 3;

    //  DS4 pre-corp config is same as MNDS post crop, when pre-crop enabled in DS4 module
    m_regCmd.displayDS4.chromaCrop.pixelConfig.bitfields.FIRST_PIXEL = m_pState->preCropInfo.CrCrop.firstPixel;
    m_regCmd.displayDS4.chromaCrop.pixelConfig.bitfields.LAST_PIXEL  = m_pState->preCropInfo.CrCrop.lastPixel;
    m_regCmd.displayDS4.chromaCrop.lineConfig.bitfields.FIRST_LINE   = m_pState->preCropInfo.CrCrop.firstLine;
    m_regCmd.displayDS4.chromaCrop.lineConfig.bitfields.LAST_LINE    = m_pState->preCropInfo.CrCrop.lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411Titan480::ConfigureDS16Registers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS411Titan480::ConfigureDS16Registers(
    DS411InputData* pData)
{
    CAMX_UNREFERENCED_PARAM(pData);

    m_regCmd.displayDS16.luma.lumaModuleConfig.bitfields.EN                    = 1;
    m_regCmd.displayDS16.luma.lumaModuleConfig.bitfields.CROP_EN               = 1;
    m_regCmd.displayDS16.luma.lumaModuleConfig.bitfields.SCALE_EN              = 1;
    m_regCmd.displayDS16.luma.lumaModuleConfig.bitfields.FLUSH_PACE_CNT        = 0xf;
    m_regCmd.displayDS16.luma.coefficients.bitfields.COEFF_07                  = 125;
    m_regCmd.displayDS16.luma.coefficients.bitfields.COEFF_16                  = 91;
    m_regCmd.displayDS16.luma.coefficients.bitfields.COEFF_25                  = 144;

    // DS16 pre-corp config is same as DS4 post crop, when pre-crop enabled in DS16 module
    m_regCmd.displayDS16.lumaCrop.pixelConfig.bitfields.FIRST_PIXEL = m_pState->preCropInfo.YCrop.firstPixel;
    m_regCmd.displayDS16.lumaCrop.pixelConfig.bitfields.LAST_PIXEL  = m_pState->preCropInfo.YCrop.lastPixel;
    m_regCmd.displayDS16.lumaCrop.lineConfig.bitfields.FIRST_LINE   = m_pState->preCropInfo.YCrop.firstLine;
    m_regCmd.displayDS16.lumaCrop.lineConfig.bitfields.LAST_LINE    = m_pState->preCropInfo.YCrop.lastLine;

    m_regCmd.displayDS16.chroma.chromaModuleConfig.bitfields.EN                = 1;
    m_regCmd.displayDS16.chroma.chromaModuleConfig.bitfields.CROP_EN           = 1;
    m_regCmd.displayDS16.chroma.chromaModuleConfig.bitfields.SCALE_EN          = 1;
    m_regCmd.displayDS16.chroma.chromaModuleConfig.bitfields.FLUSH_PACE_CNT    = 0xf;

    // DS16 pre-corp config is same as DS4 post crop, when pre-crop enabled in DS16 module
    m_regCmd.displayDS16.chromaCrop.pixelConfig.bitfields.FIRST_PIXEL = m_pState->preCropInfo.CrCrop.firstPixel;
    m_regCmd.displayDS16.chromaCrop.pixelConfig.bitfields.LAST_PIXEL  = m_pState->preCropInfo.CrCrop.lastPixel;
    m_regCmd.displayDS16.chromaCrop.lineConfig.bitfields.FIRST_LINE   = m_pState->preCropInfo.CrCrop.firstLine;
    m_regCmd.displayDS16.chromaCrop.lineConfig.bitfields.LAST_LINE    = m_pState->preCropInfo.CrCrop.lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411Titan480::ConfigureVideoDS16Registers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS411Titan480::ConfigureVideoDS16Registers(
    DS411InputData* pData)
{
    CAMX_UNREFERENCED_PARAM(pData);

    m_regCmd.videoDS16.luma.lumaModuleConfig.bitfields.EN             = 1;
    m_regCmd.videoDS16.luma.lumaModuleConfig.bitfields.CROP_EN        = 1;
    m_regCmd.videoDS16.luma.lumaModuleConfig.bitfields.SCALE_EN       = 1;
    m_regCmd.videoDS16.luma.lumaModuleConfig.bitfields.FLUSH_PACE_CNT = 0xf;
    m_regCmd.videoDS16.luma.coefficients.bitfields.COEFF_07           = 125;
    m_regCmd.videoDS16.luma.coefficients.bitfields.COEFF_16           = 91;
    m_regCmd.videoDS16.luma.coefficients.bitfields.COEFF_25           = 144;

    // Video DS16 pre-corp config is same as Video DS4 post crop, when pre-crop enabled in video DS16 module
    m_regCmd.videoDS16.lumaCrop.pixelConfig.bitfields.FIRST_PIXEL = m_pState->preCropInfo.YCrop.firstPixel;
    m_regCmd.videoDS16.lumaCrop.pixelConfig.bitfields.LAST_PIXEL  = m_pState->preCropInfo.YCrop.lastPixel;
    m_regCmd.videoDS16.lumaCrop.lineConfig.bitfields.FIRST_LINE   = m_pState->preCropInfo.YCrop.firstLine;
    m_regCmd.videoDS16.lumaCrop.lineConfig.bitfields.LAST_LINE    = m_pState->preCropInfo.YCrop.lastLine;

    m_regCmd.videoDS16.chroma.chromaModuleConfig.bitfields.EN             = 1;
    m_regCmd.videoDS16.chroma.chromaModuleConfig.bitfields.CROP_EN        = 1;
    m_regCmd.videoDS16.chroma.chromaModuleConfig.bitfields.SCALE_EN       = 1;
    m_regCmd.videoDS16.chroma.chromaModuleConfig.bitfields.FLUSH_PACE_CNT = 0xf;

    // Video DS16 pre-corp config is same as Video DS4 post crop, when pre-crop enabled in video DS16 module
    m_regCmd.videoDS16.chromaCrop.pixelConfig.bitfields.FIRST_PIXEL = m_pState->preCropInfo.CrCrop.firstPixel;
    m_regCmd.videoDS16.chromaCrop.pixelConfig.bitfields.LAST_PIXEL  = m_pState->preCropInfo.CrCrop.lastPixel;
    m_regCmd.videoDS16.chromaCrop.lineConfig.bitfields.FIRST_LINE   = m_pState->preCropInfo.CrCrop.firstLine;
    m_regCmd.videoDS16.chromaCrop.lineConfig.bitfields.LAST_LINE    = m_pState->preCropInfo.CrCrop.lastLine;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS411Titan480::CreateCmdList(
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
        if ((IFEPipelinePath::DisplayDS4Path == m_modulePath))
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_MODULE_CFG,
                                                  (sizeof(IFEDS4DisplayLumaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.displayDS4.luma));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure display DS4 path Y register");
            }

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS4_OUT_CROP_LINE_CFG,
                                                  (sizeof(IFEDS4DisplayLumaCropReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.displayDS4.lumaCrop));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure display DS4 path Y crop register");
            }

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS4_OUT_MODULE_CFG,
                                                  (sizeof(IFEDS4DisplayChromaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.displayDS4.chroma));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure display DS4 path C register");
            }

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS4_OUT_CROP_LINE_CFG,
                                                  (sizeof(IFEDS4DisplayChromaCropReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.displayDS4.chromaCrop));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure display DS4 path crop register");
            }
        }

        // Write DS16 output path registers
        if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_MODULE_CFG,
                (sizeof(IFEDS16DisplayLumaReg) / RegisterWidthInBytes),
                reinterpret_cast<UINT32*>(&m_regCmd.displayDS16.luma));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure display DS16 path Y register");
            }

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_DISP_DS16_OUT_CROP_LINE_CFG,
                (sizeof(IFEDS16DisplayLumaCropReg) / RegisterWidthInBytes),
                reinterpret_cast<UINT32*>(&m_regCmd.displayDS16.lumaCrop));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure display DS16 path Y crop register");
            }

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS16_OUT_MODULE_CFG,
                (sizeof(IFEDS16DisplayChromaReg) / RegisterWidthInBytes),
                reinterpret_cast<UINT32*>(&m_regCmd.displayDS16.chroma));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure display DS16 path C register");
            }

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_DISP_DS16_OUT_CROP_LINE_CFG,
                (sizeof(IFEDS16DisplayChromaCropReg) / RegisterWidthInBytes),
                reinterpret_cast<UINT32*>(&m_regCmd.displayDS16.chromaCrop));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure display DS16 path C crop register");
            }
        }

        if (IFEPipelinePath::VideoDS16Path == m_modulePath)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_VID_DS16_OUT_MODULE_CFG,
                                                  (sizeof(IFEDS16VideoLumaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.videoDS16.luma));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure video DS16 path Y register");
            }

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_Y_VID_DS16_OUT_CROP_LINE_CFG,
                                                  (sizeof(IFEDS16VideoLumaCropReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.videoDS16.lumaCrop));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure video DS16 path Y crop register");
            }

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_VID_DS16_OUT_MODULE_CFG,
                                                  (sizeof(IFEDS16VideoChromaReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.videoDS16.chroma));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure video DS16 path C register");
            }

            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DOWNSCALE_4TO1_C_VID_DS16_OUT_CROP_LINE_CFG,
                                                  (sizeof(IFEDS16VideoChromaCropReg) / RegisterWidthInBytes),
                                                  reinterpret_cast<UINT32*>(&m_regCmd.videoDS16.chromaCrop));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to configure video DS16 path C crop register");
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
// IFEDS411Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS411Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult      result = CamxResultSuccess;
    DS411InputData* pData  = static_cast<DS411InputData*>(pInput);
    m_pState               = static_cast<DSState*>(pOutput);

    if ((NULL != pData) && (NULL != m_pState))
    {
        m_modulePath = static_cast<CamX::IFEPipelinePath>(pData->modulePath);

        if (IFEPipelinePath::DisplayDS4Path  == m_modulePath)
        {
            ConfigureDS4Registers(pData);
        }

        if (IFEPipelinePath::DisplayDS16Path  == m_modulePath)
        {
            ConfigureDS16Registers(pData);
        }

        if (IFEPipelinePath::VideoDS16Path == m_modulePath)
        {
            ConfigureVideoDS16Registers(pData);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDS411Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_ERROR(CamxLogGroupIQMod, "Not Implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEDS411Titan480::CopyRegCmd(
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
// IFEDS411Titan480::~IFEDS411Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDS411Titan480::~IFEDS411Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDS411Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDS411Titan480::DumpRegConfig()
{
    if (IFEPipelinePath::DisplayDS4Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 lumaConfig                 [0x%x]",
            m_regCmd.displayDS4.luma.lumaModuleConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Filter coefficients        [0x%x]",
            m_regCmd.displayDS4.luma.coefficients.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4.luma.Crop line config      [0x%x]",
            m_regCmd.displayDS4.lumaCrop.lineConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4.luma.Crop pixel config     [0x%x]",
            m_regCmd.displayDS4.lumaCrop.pixelConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 chromaConfig               [0x%x]",
            m_regCmd.displayDS4.chroma.chromaModuleConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Chroma Crop Line config    [0x%x]",
            m_regCmd.displayDS4.chromaCrop.lineConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS4 Chroma Crop pixel config   [0x%x]",
            m_regCmd.displayDS4.chromaCrop.pixelConfig.u32All);
    }

    if (IFEPipelinePath::DisplayDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 lumaConfig                [0x%x]",
            m_regCmd.displayDS16.luma.lumaModuleConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Filter coefficients       [0x%x]",
            m_regCmd.displayDS16.luma.coefficients.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16.luma.Crop line config     [0x%x]",
            m_regCmd.displayDS16.lumaCrop.lineConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16.luma.Crop pixel config    [0x%x]",
            m_regCmd.displayDS16.lumaCrop.pixelConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 chromaConfig              [0x%x]",
            m_regCmd.displayDS16.chroma.chromaModuleConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Crop Line config   [0x%x]",
            m_regCmd.displayDS16.chromaCrop.lineConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "DS16 Chroma Crop pixel config  [0x%x]",
            m_regCmd.displayDS16.chromaCrop.pixelConfig.u32All);
    }

    if (IFEPipelinePath::VideoDS16Path == m_modulePath)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "videoDS16  lumaConfig                [0x%x]",
            m_regCmd.videoDS16.luma.lumaModuleConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "videoDS16 Filter coefficients       [0x%x]",
            m_regCmd.videoDS16.luma.coefficients.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "videoDS16.luma.Crop line config     [0x%x]",
            m_regCmd.videoDS16.lumaCrop.lineConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "videoDS16.luma.Crop pixel config    [0x%x]",
            m_regCmd.videoDS16.lumaCrop.pixelConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "videoDS16 chromaConfig              [0x%x]",
            m_regCmd.videoDS16.chroma.chromaModuleConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "videoDS16 Chroma Crop Line config   [0x%x]",
            m_regCmd.videoDS16.chromaCrop.lineConfig.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "videoDS16 Chroma Crop pixel config  [0x%x]",
            m_regCmd.videoDS16.chromaCrop.pixelConfig.u32All);
    }
}
CAMX_NAMESPACE_END
