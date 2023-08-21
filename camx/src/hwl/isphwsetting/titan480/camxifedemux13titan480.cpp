////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedemux13titan480.cpp
/// @brief CAMXIFEDEMEMUX13TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifedemux13titan480.h"
#include "demux13setting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan480::IFEDemux13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemux13Titan480::IFEDemux13Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEDemux13Titan480RegLengthDWord));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL == pInputData->pCmdBuffer)
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input data or command buffer");
    }
    else
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_BPC_PDPC_DEMUX_CFG,
                                                  IFEDemux13Titan480RegLengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEDemux13Titan480RegCmd) <=
            sizeof(pIFETuningMetadata->metadata480.IFEDemuxData.demuxConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEDemuxData.demuxConfig, &m_regCmd, sizeof(IFEDemux13Titan480RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult                    result            = CamxResultSuccess;
    Demux13UnpackedField*         pData             = static_cast<Demux13UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd.demuxConfig.bitfields.PERIOD                   = pData->period;
        m_regCmd.demuxConfig.bitfields.BLK_OUT                  = pData->blackLevelOut;
        m_regCmd.demuxConfig.bitfields.BLK_IN                   = pData->blackLevelIn;
        m_regCmd.demuxGainCH0.bitfields.CH0_GAIN_EVEN           = pData->gainChannel0EevenLeft;
        m_regCmd.demuxGainCH0.bitfields.CH0_GAIN_ODD            = pData->gainChannel0OddLeft;
        m_regCmd.demuxGainCH12.bitfields.CH1_GAIN               = pData->gainChannel1Left;
        m_regCmd.demuxGainCH12.bitfields.CH2_GAIN               = pData->gainChannel2Left;
        m_regCmd.demuxRightGainCH0.bitfields.CH0_GAIN_EVEN      = pData->gainChannel0EvenRight;
        m_regCmd.demuxRightGainCH0.bitfields.CH0_GAIN_ODD       = pData->gainChannel0OddRight;
        m_regCmd.demuxRightGainCH12.bitfields.CH1_GAIN          = pData->gainChannel1Right;
        m_regCmd.demuxRightGainCH12.bitfields.CH2_GAIN          = pData->gainChannel2Right;
        m_regCmd.demuxEvenConfig.bitfields.EVEN_LINE_PATTERN    = pData->evenConfig;
        m_regCmd.demuxOddConfig.bitfields.ODD_LINE_PATTERN      = pData->oddConfig;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input Data pData %p", pData);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_ERROR(CamxLogGroupIQMod, "Not Implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan480::~IFEDemux13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemux13Titan480::~IFEDemux13Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDemux13Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux moduleConfig       [0x%x]", m_regCmd.demuxConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxGainCH0       [0x%x]", m_regCmd.demuxGainCH0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxGainCH12      [0x%x]", m_regCmd.demuxGainCH12.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxRightGainCH0  [0x%x]", m_regCmd.demuxRightGainCH0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxRightGainCH12 [0x%x]", m_regCmd.demuxRightGainCH12.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxEvenConfig    [0x%x]", m_regCmd.demuxEvenConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxOddConfig     [0x%x]", m_regCmd.demuxOddConfig.u32All);
}

CAMX_NAMESPACE_END
