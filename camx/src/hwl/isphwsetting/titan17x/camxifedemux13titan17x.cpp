////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedemux13titan17x.cpp
/// @brief CAMXIFEDEMEMUX13TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifedemux13titan17x.h"
#include "demux13setting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 IFEDemux13RegLengthDWord = sizeof(IFEDemux13RegCmd) / sizeof(UINT32);
CAMX_STATIC_ASSERT((7 * 4) == sizeof(IFEDemux13RegCmd));

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan17x::IFEDemux13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemux13Titan17x::IFEDemux13Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEDemux13RegLengthDWord));

    // Hardcode initial value for all the registers
    m_regCmd.demuxConfig.u32All     = 0x00000001;
    m_regCmd.demuxGain0.u32All      = 0x04430444;
    m_regCmd.demuxGain1.u32All      = 0x04440444;
    m_regCmd.demuxRightGain0.u32All = 0x04430444;
    m_regCmd.demuxRightGain1.u32All = 0x04440444;
    m_regCmd.demuxEvenConfig.u32All = 0x000000c9;
    m_regCmd.demuxOddConfig.u32All  = 0x000000ac;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13Titan17x::CreateCmdList(
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
                                                  regIFE_IFE_0_VFE_DEMUX_CFG,
                                                  IFEDemux13RegLengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEDemux13RegCmd) <= sizeof(pIFETuningMetadata->metadata17x.IFEDemuxData.demuxConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEDemuxData.demuxConfig, &m_regCmd, sizeof(IFEDemux13RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult                result = CamxResultSuccess;
    Demux13UnpackedField*     pData  = static_cast<Demux13UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd.demuxConfig.bitfields.PERIOD                   = pData->period;
        m_regCmd.demuxConfig.bitfields.BLK_OUT                  = pData->blackLevelOut;
        m_regCmd.demuxConfig.bitfields.BLK_IN                   = pData->blackLevelIn;
        m_regCmd.demuxGain0.bitfields.CH0_GAIN_EVEN             = pData->gainChannel0EevenLeft;
        m_regCmd.demuxGain0.bitfields.CH0_GAIN_ODD              = pData->gainChannel0OddLeft;
        m_regCmd.demuxGain1.bitfields.CH1_GAIN                  = pData->gainChannel1Left;
        m_regCmd.demuxGain1.bitfields.CH2_GAIN                  = pData->gainChannel2Left;
        m_regCmd.demuxRightGain0.bitfields.CH0_GAIN_EVEN        = pData->gainChannel0EvenRight;
        m_regCmd.demuxRightGain0.bitfields.CH0_GAIN_ODD         = pData->gainChannel0OddRight;
        m_regCmd.demuxRightGain1.bitfields.CH1_GAIN             = pData->gainChannel1Right;
        m_regCmd.demuxRightGain1.bitfields.CH2_GAIN             = pData->gainChannel2Right;
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
// IFEDemux13Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemux13Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_ERROR(CamxLogGroupIQMod, "Not Implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan17x::~IFEDemux13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemux13Titan17x::~IFEDemux13Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemux13Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDemux13Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux moduleConfig       [0x%x]", m_regCmd.demuxConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxGain0         [0x%x]", m_regCmd.demuxGain0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxGain1         [0x%x]", m_regCmd.demuxGain1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxRightGain0    [0x%x]", m_regCmd.demuxRightGain0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxRightGain1    [0x%x]", m_regCmd.demuxRightGain1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxEvenConfig    [0x%x]", m_regCmd.demuxEvenConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demux demuxOddConfig     [0x%x]", m_regCmd.demuxOddConfig.u32All);
}

CAMX_NAMESPACE_END
