////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebls12titan480.cpp
/// @brief CAMXIFEBLS12TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifebls12titan480.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan480::IFEBLS12Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBLS12Titan480::IFEBLS12Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEBLS12Titan480RegLengthDWord1) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEBLS12Titan480RegLengthDWord2));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBLS12Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer = NULL;

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
                                                  regIFE_IFE_0_PP_CLC_BLS_MODULE_CFG,
                                                  IFEBLS12Titan480RegLengthDWord1,
                                                  reinterpret_cast<UINT32*>(&m_regCmd1));

        result     = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_BLS_BLACK_LEVEL_CFG_0,
                                                  IFEBLS12Titan480RegLengthDWord2,
                                                  reinterpret_cast<UINT32*>(&m_regCmd2));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBLS12Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEBLS12Titan480RegCmd1) <= sizeof(pIFETuningMetadata->metadata480.IFEBLSData.BLSConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEBLSData.BLSConfig1, &m_regCmd1, sizeof(IFEBLS12Titan480RegCmd1));

        CAMX_STATIC_ASSERT(sizeof(IFEBLS12Titan480RegCmd2) <= sizeof(pIFETuningMetadata->metadata480.IFEBLSData.BLSConfig2));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEBLSData.BLSConfig2, &m_regCmd2, sizeof(IFEBLS12Titan480RegCmd2));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBLS12Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result      = CamxResultSuccess;
    BLS12UnpackedField* pData       = static_cast<BLS12UnpackedField*>(pInput);
    BLS12OutputData*    pOutputData = static_cast<BLS12OutputData*>(pOutput);

    if ((NULL != pData) && (NULL != pOutputData))
    {
        m_regCmd1.mdouleConfig.bitfields.EN                 = 1;
        m_regCmd2.configRegister0.bitfields.OFFSET          = pData->offset;
        m_regCmd2.configRegister1.bitfields.SCALE           = pData->scale;
        m_regCmd2.thresholdRegister0.bitfields.GB_THRESHOLD = pData->thresholdGB;
        m_regCmd2.thresholdRegister0.bitfields.GR_THRESHOLD = pData->thresholdGR;
        m_regCmd2.thresholdRegister1.bitfields.B_THRESHOLD  = pData->thresholdB;
        m_regCmd2.thresholdRegister1.bitfields.R_THRESHOLD  = pData->thresholdR;
        pOutputData->blackLevelOffset                       = pData->offset;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "data is pData %p, pOutputData %p", pData, pOutputData);
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBLS12Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan480::~IFEBLS12Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBLS12Titan480::~IFEBLS12Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBLS12Titan480::DumpRegConfig()
{
    // Print regster config
}

CAMX_NAMESPACE_END
