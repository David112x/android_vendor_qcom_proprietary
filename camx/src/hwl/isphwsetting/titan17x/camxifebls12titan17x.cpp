////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebls12titan17x.cpp
/// @brief CAMXIFEBLS12TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifebls12titan17x.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan17x::IFEBLS12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBLS12Titan17x::IFEBLS12Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEBLS12RegLengthDWord));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBLS12Titan17x::CreateCmdList(
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
                                                  regIFE_IFE_0_VFE_BLACK_LEVEL_CFG,
                                                  IFEBLS12RegLengthDWord,
                                                  reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBLS12Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEBLS12RegCmd) <= sizeof(pIFETuningMetadata->metadata17x.IFEBLSData.BLSConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEBLSData.BLSConfig, &m_regCmd, sizeof(IFEBLS12RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBLS12Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result      = CamxResultSuccess;
    BLS12UnpackedField* pData       = static_cast<BLS12UnpackedField*>(pInput);
    BLS12OutputData*    pOutputData = static_cast<BLS12OutputData*>(pOutput);

    if ((NULL != pData) && (NULL != pOutputData))
    {
        m_regCmd.configRegister.bitfields.OFFSET           = pData->offset;
        m_regCmd.configRegister.bitfields.SCALE            = pData->scale;
        m_regCmd.thresholdRegister0.bitfields.GB_THRESHOLD = pData->thresholdGB;
        m_regCmd.thresholdRegister0.bitfields.GR_THRESHOLD = pData->thresholdGR;
        m_regCmd.thresholdRegister1.bitfields.B_THRESHOLD  = pData->thresholdB;
        m_regCmd.thresholdRegister1.bitfields.R_THRESHOLD  = pData->thresholdR;

        pOutputData->blackLevelOffset                      = pData->offset;
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("data is pData %p, pOutputData %p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBLS12Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan17x::~IFEBLS12Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBLS12Titan17x::~IFEBLS12Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBLS12Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBLS12Titan17x::DumpRegConfig()
{
    // Print regster config
}

CAMX_NAMESPACE_END
