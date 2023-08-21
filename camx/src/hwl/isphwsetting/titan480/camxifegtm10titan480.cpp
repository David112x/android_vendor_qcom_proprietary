////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifegtm10titan480.cpp
/// @brief IFEGTM10TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifegtm10titan480.h"
#include "gtm10setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan480::IFEGTM10Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGTM10Titan480::IFEGTM10Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEGTM10Titan480RegLengthDword) +
                     PacketBuilder::RequiredWriteDMISizeInDwords());
    Set32bitDMILength(IFEGTM10Titan480DMILengthDword);

    m_bankSelect = GTMTitan480LUTRawBank0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10Titan480::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;
    CmdBuffer*    pDMIBuffer    = NULL;
    UINT32        offset        = *pDMIBufferOffset * sizeof(UINT32);
    UINT32        lengthInByte  = IFEGTM10Titan480DMILengthDword * sizeof(UINT32);

    if ((NULL != pInputData)                  &&
        (NULL != pInputData->pCmdBuffer)      &&
        (NULL != pInputData->p32bitDMIBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        pDMIBuffer = pInputData->p32bitDMIBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_GTM_DMI_LUT_BANK_CFG,
                                              IFEGTM10Titan480RegLengthDword,
                                              reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_GTM_DMI_CFG,
                                             IFE_IFE_0_PP_CLC_GTM_DMI_LUT_CFG_LUT_SEL_GTM_LUT0,
                                             pDMIBuffer,
                                             offset,
                                             lengthInByte);
        }

        if (CamxResultSuccess == result)
        {
            // Switch LUT Bank select immediately after writing
            m_bankSelect = (m_bankSelect == GTMTitan480LUTRawBank0) ? GTMTitan480LUTRawBank1 : GTMTitan480LUTRawBank0;
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
// IFEGTM10Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10Titan480::CreateSubCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData &&
        NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_GTM_MODULE_CFG,
                                              sizeof(m_regCmd.moduleConfig) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&m_regCmd.moduleConfig));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEGTM10Titan480RegCmd) == sizeof(pIFETuningMetadata->metadata480.IFEGTMData.GTMConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEGTMData.GTMConfig, &m_regCmd, sizeof(IFEGTM10Titan480RegCmd));

        if (NULL != m_pGTMLUTPtr)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata480.IFEDMIPacked.GTM.LUT,
                          m_pGTMLUTPtr,
                          IFEGTM10Titan480DMILengthDword * sizeof(UINT32));
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result        = CamxResultSuccess;
    GTM10UnpackedField* pData         = static_cast<GTM10UnpackedField*>(pInput);
    GTM10OutputData*    pOutputData   = static_cast<GTM10OutputData*>(pOutput);


    if ((NULL != pOutputData) && (TRUE == pOutputData->bIsBankUpdateValid))
    {
        m_bankSelect = pOutputData->bankSelect;
    }
    m_regCmd.DMILUTBankConfig.bitfields.BANK_SEL = m_bankSelect;
    m_regCmd.LUTBankConfig.bitfields.BANK_SEL    = m_bankSelect;

    if ((NULL != pOutputData) && (NULL != pData) && (NULL != pOutputData->regCmd.IFE.pDMIDataPtr))
    {
        m_regCmd.moduleConfig.bitfields.EN           = pData->enable;

        for (UINT32 LUTEntry = 0; LUTEntry < GTM10LUTSize; LUTEntry++)
        {
            pOutputData->regCmd.IFE.pDMIDataPtr[LUTEntry] =
                ((static_cast<UINT64>(pData->YratioBase[pData->tableSel][LUTEntry]) & Utils::AllOnes64(18)) |
                 ((static_cast<UINT64>(pData->YratioSlope[pData->tableSel][LUTEntry]) & Utils::AllOnes64(26)) << 18));
        }
        m_pGTMLUTPtr = pOutputData->regCmd.IFE.pDMIDataPtr;
    }
    else
    {
        m_pGTMLUTPtr = NULL;
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p pOutputData=0x%p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd.moduleConfig.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan480::~IFEGTM10Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGTM10Titan480::~IFEGTM10Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEGTM10Titan480::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
