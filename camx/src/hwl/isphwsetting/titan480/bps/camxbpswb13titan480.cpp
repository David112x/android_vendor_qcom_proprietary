////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpswb13titan480.cpp
/// @brief CAMXBPSWB13TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpswb13titan480.h"
#include "wb13setting.h"
#include "camxiqinterface.h"
#include "titan480_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief WB register Configuration
struct BPSWB13RegCmd
{
    BPS_BPS_0_CLC_DEMOSAIC_WB_GAIN_CFG_0    whiteBalanceConfig0;    ///< white balance config 0
    BPS_BPS_0_CLC_DEMOSAIC_WB_GAIN_CFG_1    whiteBalanceConfig1;    ///< white balance config 1
    BPS_BPS_0_CLC_DEMOSAIC_WB_OFFSET_CFG_0  whiteBalaneOffset0;     ///< white balance offset 0
    BPS_BPS_0_CLC_DEMOSAIC_WB_OFFSET_CFG_1  whiteBalaneOffset1;     ///< white balance offset 1
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSWB13RegLengthDWord = sizeof(BPSWB13RegCmd) / sizeof(UINT32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13Titan480::BPSWB13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSWB13Titan480::BPSWB13Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13Titan480::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSWB13Titan480::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSWB13Titan480* pHWSetting = CAMX_NEW BPSWB13Titan480;

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
/// BPSWB13Titan480::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSWB13Titan480::Initialize()
{
    CamxResult result = CamxResultSuccess;

    BPSWB13RegCmd* pRegCmd = static_cast<BPSWB13RegCmd*>(CAMX_CALLOC(sizeof(BPSWB13RegCmd)));

    if (NULL != pRegCmd)
    {
        m_pRegCmd = pRegCmd;
        SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSWB13RegLengthDWord));

        pRegCmd->whiteBalanceConfig0.bitfields.G_GAIN  = 0x00000400;
        pRegCmd->whiteBalanceConfig0.bitfields.B_GAIN  = 0x00000827;
        pRegCmd->whiteBalanceConfig1.bitfields.R_GAIN  = 0x00000512;
        pRegCmd->whiteBalaneOffset0.bitfields.G_OFFSET = 0x00000000;
        pRegCmd->whiteBalaneOffset0.bitfields.B_OFFSET = 0x00000000;
        pRegCmd->whiteBalaneOffset1.bitfields.R_OFFSET = 0x00000000;
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for register configuration!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSWB13Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CamxResult     result     = CamxResultSuccess;
    ISPInputData*  pInputData = static_cast<ISPInputData*>(pSettingData);
    BPSWB13RegCmd* pRegCmd    = static_cast<BPSWB13RegCmd*>(m_pRegCmd);

    if (NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_DEMOSAIC_WB_GAIN_CFG_0,
                                              BPSWB13RegLengthDWord,
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

    if (NULL != pInputData->pCalculatedData)
    {
        pInputData->pCalculatedData->colorCorrectionGains.blue =
            static_cast<FLOAT>(pRegCmd->whiteBalanceConfig0.bitfields.B_GAIN) / (1 << 10);
        pInputData->pCalculatedData->colorCorrectionGains.greenEven =
            static_cast<FLOAT>(pRegCmd->whiteBalanceConfig0.bitfields.G_GAIN) / (1 << 10);
        pInputData->pCalculatedData->colorCorrectionGains.greenOdd =
            static_cast<FLOAT>(pRegCmd->whiteBalanceConfig0.bitfields.G_GAIN) / (1 << 10);
        pInputData->pCalculatedData->colorCorrectionGains.red =
            static_cast<FLOAT>(pRegCmd->whiteBalanceConfig1.bitfields.R_GAIN) / (1 << 10);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSWB13Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BPSWB13RegCmd*     pRegCmd            = static_cast<BPSWB13RegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSWB13RegCmd) <= sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSWBData.WBConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata480.BPSWBData.WBConfig, pRegCmd, sizeof(BPSWB13RegCmd));

        if (NULL != pInputData->pipelineBPSData.pDebugDataWriter)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSWBRegister,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata480.BPSWBData.WBConfig),
                &pBPSTuningMetadata->BPSTuningMetadata480.BPSWBData.WBConfig,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSWBData.WBConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSWB13Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult         result      = CamxResultSuccess;
    WB13UnpackedField* pData       = static_cast<WB13UnpackedField*>(pInput);
    WB13OutputData*    pOutputData = static_cast<WB13OutputData*>(pOutput);
    BPSWB13RegCmd*     pRegCmd     = static_cast<BPSWB13RegCmd*>(m_pRegCmd);

    if ((NULL != pOutputData) && (NULL != pData))
    {
        pRegCmd->whiteBalanceConfig0.bitfields.G_GAIN  = pData->gainChannel0Left;
        pRegCmd->whiteBalanceConfig0.bitfields.B_GAIN  = pData->gainChannel1Left;
        pRegCmd->whiteBalanceConfig1.bitfields.R_GAIN  = pData->gainChannel2Left;
        pRegCmd->whiteBalaneOffset0.bitfields.G_OFFSET = pData->offsetChannel0Left;
        pRegCmd->whiteBalaneOffset0.bitfields.B_OFFSET = pData->offsetChannel1Left;
        pRegCmd->whiteBalaneOffset1.bitfields.R_OFFSET = pData->offsetChannel2Left;

        if (TRUE == pOutputData->manualGainOverride)
        {
            pRegCmd->whiteBalanceConfig0.bitfields.G_GAIN = Utils::FloatToQNumber(pOutputData->manualControl.gGain, 1 << 10);
            pRegCmd->whiteBalanceConfig0.bitfields.B_GAIN = Utils::FloatToQNumber(pOutputData->manualControl.bGain, 1 << 10);
            pRegCmd->whiteBalanceConfig1.bitfields.R_GAIN = Utils::FloatToQNumber(pOutputData->manualControl.rGain, 1 << 10);

            pRegCmd->whiteBalaneOffset0.bitfields.G_OFFSET = 0;
            pRegCmd->whiteBalaneOffset0.bitfields.B_OFFSET = 0;
            pRegCmd->whiteBalaneOffset1.bitfields.R_OFFSET = 0;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Data: %p or Output: %p is NULL", pData, pOutput);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13Titan480::~BPSWB13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSWB13Titan480::~BPSWB13Titan480()
{
    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSWB13Titan480::DumpRegConfig()
{
    BPSWB13RegCmd* pRegCmd = static_cast<BPSWB13RegCmd*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "* WB13 CFG [HEX] \n");
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "G_Gain   = %x\n", pRegCmd->whiteBalanceConfig0.bitfields.G_GAIN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "B_Gain   = %x\n", pRegCmd->whiteBalanceConfig0.bitfields.B_GAIN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "R_Gain   = %x\n", pRegCmd->whiteBalanceConfig1.bitfields.R_GAIN);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "G_OFFSET = %x\n", pRegCmd->whiteBalaneOffset0.bitfields.G_OFFSET);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "B_OFFSET = %x\n", pRegCmd->whiteBalaneOffset0.bitfields.B_OFFSET);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "R_OFFSET = %x\n", pRegCmd->whiteBalaneOffset1.bitfields.R_OFFSET);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "\n\n");
    }
}

CAMX_NAMESPACE_END
