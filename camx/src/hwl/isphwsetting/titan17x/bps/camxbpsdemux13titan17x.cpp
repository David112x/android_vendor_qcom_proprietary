////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsdemux13titan17x.cpp
/// @brief CAMXBPSDEMUX13TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpsdemux13titan17x.h"
#include "demux13setting.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief Demux register Configuration
struct BPSDemux13RegCmd
{
    BPS_BPS_0_CLC_BPC_PDPC_DEMUX_CFG             config;                ///< Demux Config
    BPS_BPS_0_CLC_BPC_PDPC_DEMUX_GAIN_CH0        gainChannel0;          ///< Demux Channel 0
    BPS_BPS_0_CLC_BPC_PDPC_DEMUX_GAIN_CH12       gainChannel12;         ///< Demux Channel 12
    BPS_BPS_0_CLC_BPC_PDPC_DEMUX_GAIN_RIGHT_CH0  gainRightChannel0;     ///< Demux right plane channel 0
    BPS_BPS_0_CLC_BPC_PDPC_DEMUX_GAIN_RIGHT_CH12 gainRightChannel12;    ///< Demux right plane channel 12
    BPS_BPS_0_CLC_BPC_PDPC_DEMUX_EVEN_CFG        evenConfig;            ///< Demux Even config
    BPS_BPS_0_CLC_BPC_PDPC_DEMUX_ODD_CFG         oddConfig;             ///< Demux Odd Config
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSDemux13RegLengthDWord = sizeof(BPSDemux13RegCmd) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13Titan17x::BPSDemux13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSDemux13Titan17x::BPSDemux13Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemux13Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSDemux13Titan17x* pHWSetting = CAMX_NEW BPSDemux13Titan17x;

    if (NULL != pHWSetting)
    {
        result = pHWSetting->Initialize();
        if (CamxResultSuccess == result)
        {
            (*ppHWSetting) = pHWSetting;
        }
        else
        {
            CAMX_DELETE pHWSetting;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to initialize in %s, no memory", __FUNCTION__);
        }
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "%s failed, no memory", __FUNCTION__);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSDemux13Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemux13Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    BPSDemux13RegCmd* pRegCmd = static_cast<BPSDemux13RegCmd*>(CAMX_CALLOC(sizeof(BPSDemux13RegCmd)));

    if (NULL != pRegCmd)
    {
        m_pRegCmd = pRegCmd;

        SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSDemux13RegLengthDWord));

        pRegCmd->config.u32All              = 0x00000001;
        pRegCmd->gainChannel0.u32All        = 0x044c044d;
        pRegCmd->gainChannel12.u32All       = 0x044d044c;
        pRegCmd->gainRightChannel0.u32All   = 0x044c044d;
        pRegCmd->gainRightChannel12.u32All  = 0x044d044c;
        pRegCmd->oddConfig.u32All           = 0x000000ac;
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for register configuration!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemux13Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    if (NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_BPC_PDPC_DEMUX_CFG,
                                              BPSDemux13RegLengthDWord,
                                              static_cast<UINT32*>(m_pRegCmd));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemux13Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    if (NULL != pBPSIQSettings)
    {
        // Demux is in pdpc hardware
        pBPSIQSettings->pdpcParameters.moduleCfg.EN                 |= moduleEnable;
        pBPSIQSettings->pdpcParameters.moduleCfg.CHANNEL_BALANCE_EN  = moduleEnable;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "pBPSIQSettings is NULL");
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemux13Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BPSDemux13RegCmd*  pRegCmd            = static_cast<BPSDemux13RegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSDemux13RegCmd) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSDemuxData.demuxConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSDemuxData.demuxConfig,
                        pRegCmd,
                        sizeof(BPSDemux13RegCmd));

        result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
            DebugDataTagID::TuningBPSDemuxRegister,
            DebugDataTagType::UInt32,
            CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata17x.BPSDemuxData.demuxConfig),
            &pBPSTuningMetadata->BPSTuningMetadata17x.BPSDemuxData.demuxConfig,
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSDemuxData.demuxConfig));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemux13Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult            result  = CamxResultSuccess;
    Demux13UnpackedField* pData   = static_cast<Demux13UnpackedField*>(pInput);
    BPSDemux13RegCmd*     pRegCmd = static_cast<BPSDemux13RegCmd*>(m_pRegCmd);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        pRegCmd->config.bitfields.PERIOD                   = pData->period;
        pRegCmd->config.bitfields.BLK_OUT                  = pData->blackLevelOut;
        pRegCmd->config.bitfields.BLK_IN                   = pData->blackLevelIn;
        pRegCmd->gainChannel0.bitfields.CH0_GAIN_EVEN      = pData->gainChannel0EevenLeft;
        pRegCmd->gainChannel0.bitfields.CH0_GAIN_ODD       = pData->gainChannel0OddLeft;
        pRegCmd->gainChannel12.bitfields.CH1_GAIN          = pData->gainChannel1Left;
        pRegCmd->gainChannel12.bitfields.CH2_GAIN          = pData->gainChannel2Left;
        pRegCmd->gainRightChannel0.bitfields.CH0_GAIN_EVEN = pData->gainChannel0EvenRight;
        pRegCmd->gainRightChannel0.bitfields.CH0_GAIN_ODD  = pData->gainChannel0OddRight;
        pRegCmd->gainRightChannel12.bitfields.CH1_GAIN     = pData->gainChannel1Right;
        pRegCmd->gainRightChannel12.bitfields.CH2_GAIN     = pData->gainChannel2Right;
        pRegCmd->evenConfig.bitfields.EVEN_LINE_PATTERN    = pData->evenConfig;
        pRegCmd->oddConfig.bitfields.ODD_LINE_PATTERN      = pData->oddConfig;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input Data is pData %p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13Titan17x::~BPSDemux13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSDemux13Titan17x::~BPSDemux13Titan17x()
{
    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemux13Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSDemux13Titan17x::DumpRegConfig()
{
    BPSDemux13RegCmd* pRegCmd = static_cast<BPSDemux13RegCmd*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Module Config         = %x\n", pRegCmd->config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Channel 0 gain        = %x\n", pRegCmd->gainChannel0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Channel 12 gain       = %x\n", pRegCmd->gainChannel12);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Right Channel 0 gain  = %x\n", pRegCmd->gainRightChannel0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Right Channel 12 gain = %x\n", pRegCmd->gainRightChannel12);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "odd Config            = %x\n", pRegCmd->oddConfig);
    }
}

CAMX_NAMESPACE_END
