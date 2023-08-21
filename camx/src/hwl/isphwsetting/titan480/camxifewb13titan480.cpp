////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifewb13titan480.cpp
/// @brief CAMXIFEWB13TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifewb13titan480.h"
#include "wb13setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13Titan480::IFEWB13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEWB13Titan480::IFEWB13Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEWB13Titan480RegLengthDWord));

    // Hardcode initial value for all the registers
    m_regCmd.WBGainConfig0.u32All   = 0x04000400;
    m_regCmd.WBGainConfig1.u32All   = 0x00000400;
    m_regCmd.WBOffsetConfig0.u32All = 0x00000000;
    m_regCmd.WBOffsetConfig1.u32All = 0x00000000;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB13Titan480::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DEMOSAIC_WB_GAIN_CFG_0,
                                                  IFEWB13Titan480RegLengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
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
// IFEWB13Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB13Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEWB13Titan480RegCmd) <= sizeof(pIFETuningMetadata->metadata480.IFEWBData.WBconfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEWBData.WBconfig, &m_regCmd, sizeof(IFEWB13Titan480RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB13Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult         result        = CamxResultSuccess;
    WB13UnpackedField* pData         = static_cast<WB13UnpackedField*>(pInput);
    WB13OutputData*    pOutputData   = static_cast<WB13OutputData*>(pOutput);

    if ((NULL != pOutputData) && (NULL != pData))
    {
        m_regCmd.WBGainConfig0.bitfields.G_GAIN     = pData->gainChannel0Left;
        m_regCmd.WBGainConfig0.bitfields.B_GAIN     = pData->gainChannel1Left;
        m_regCmd.WBGainConfig1.bitfields.R_GAIN     = pData->gainChannel2Left;
        m_regCmd.WBOffsetConfig0.bitfields.G_OFFSET = pData->offsetChannel0Left;
        m_regCmd.WBOffsetConfig0.bitfields.B_OFFSET = pData->offsetChannel1Left;
        m_regCmd.WBOffsetConfig1.bitfields.R_OFFSET = pData->offsetChannel2Left;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p pOutputData=0x%p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13Titan480::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB13Titan480::UpdateIFEInternalData(
    VOID*  pInput)
{
    CamxResult result           = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pInput);

    if (NULL != pInputData->pCalculatedData)
    {
        pInputData->pCalculatedData->colorCorrectionGains.blue      =
            IQSettingUtils::QNumberToFloat(m_regCmd.WBGainConfig0.bitfields.B_GAIN, QNumber_10U);
        pInputData->pCalculatedData->colorCorrectionGains.greenEven =
            IQSettingUtils::QNumberToFloat(m_regCmd.WBGainConfig0.bitfields.G_GAIN, QNumber_10U);
        pInputData->pCalculatedData->colorCorrectionGains.greenOdd  =
            IQSettingUtils::QNumberToFloat(m_regCmd.WBGainConfig0.bitfields.G_GAIN, QNumber_10U);
        pInputData->pCalculatedData->colorCorrectionGains.red       =
            IQSettingUtils::QNumberToFloat(m_regCmd.WBGainConfig1.bitfields.R_GAIN, QNumber_10U);

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Reporting Gains [%f, %f, %f, %f]",
                pInputData->pCalculatedData->colorCorrectionGains.red,
                pInputData->pCalculatedData->colorCorrectionGains.greenEven,
                pInputData->pCalculatedData->colorCorrectionGains.greenOdd,
                pInputData->pCalculatedData->colorCorrectionGains.red);
    }

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIFETuningMetadata)
    {
        if (CamxResultSuccess != UpdateTuningMetadata(pInputData->pIFETuningMetadata))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "UpdateTuningMetadata failed.");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13Titan480::~IFEWB13Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEWB13Titan480::~IFEWB13Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEWB13Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "G_Gain   [0x%x]", m_regCmd.WBGainConfig0.bitfields.G_GAIN);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "B_Gain   [0x%x]", m_regCmd.WBGainConfig0.bitfields.B_GAIN);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "R_Gain   [0x%x]", m_regCmd.WBGainConfig1.bitfields.R_GAIN);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "G_OFFSET [0x%x]", m_regCmd.WBOffsetConfig0.bitfields.G_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "B_OFFSET [0x%x]", m_regCmd.WBOffsetConfig0.bitfields.B_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "R_OFFSET [0x%x]", m_regCmd.WBOffsetConfig1.bitfields.R_OFFSET);
}

CAMX_NAMESPACE_END
