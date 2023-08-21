////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifegtm10titan17x.cpp
/// @brief CAMXIFEGTM10TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifegtm10titan17x.h"
#include "gtm10setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan17x::IFEGTM10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGTM10Titan17x::IFEGTM10Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEGTM10RegLengthDword) +
                     PacketBuilder::RequiredWriteDMISizeInDwords());
    Set64bitDMILength(IFEGTM10DMILengthDword);
    m_bankSelect = GTMLUTRawBank0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10Titan17x::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;
    CmdBuffer*    pDMIBuffer    = NULL;
    UINT32        offset        = *pDMIBufferOffset * sizeof(UINT32);
    UINT32        lengthInByte  = IFEGTM10DMILengthDword * sizeof(UINT32);

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p64bitDMIBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        pDMIBuffer = pInputData->p64bitDMIBuffer;

        if (TRUE == pInputData->bankUpdate.isValid)
        {
            m_bankSelect = (pInputData->bankUpdate.GTMBank == 0) ? GTMLUTRawBank0 : GTMLUTRawBank1;
        }

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_GTM_CFG,
                                              IFEGTM10RegLengthDword,
                                              reinterpret_cast<UINT32*>(&m_regCmd));

        CAMX_ASSERT(CamxResultSuccess == result);

        result = PacketBuilder::WriteDMI(pCmdBuffer,
                                         regIFE_IFE_0_VFE_DMI_CFG,
                                         m_bankSelect,
                                         pDMIBuffer,
                                         offset,
                                         lengthInByte);

        CAMX_ASSERT(CamxResultSuccess == result);

        // Switch LUT Bank select immediately after writing
        m_bankSelect = (m_bankSelect == GTMLUTRawBank0) ? GTMLUTRawBank1 : GTMLUTRawBank0;
        m_regCmd.configRegister.bitfields.LUT_BANK_SEL = (m_bankSelect == GTMLUTRawBank0) ? 0x0 : 0x1;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEGTM10RegCmd) == sizeof(pIFETuningMetadata->metadata17x.IFEGTMData.GTMConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEGTMData.GTMConfig, &m_regCmd, sizeof(IFEGTM10RegCmd));

        if (NULL != m_pGTMLUTPtr)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata17x.IFEDMIPacked.GTM.LUT,
                          m_pGTMLUTPtr,
                          IFEGTM10DMILengthDword * sizeof(UINT32));
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
// IFEGTM10Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result        = CamxResultSuccess;
    GTM10UnpackedField* pData         = static_cast<GTM10UnpackedField*>(pInput);
    GTM10OutputData*    pOutputData   = static_cast<GTM10OutputData*>(pOutput);

    if ((NULL != pOutputData) && (NULL != pData) && (NULL != pOutputData->regCmd.IFE.pDMIDataPtr))
    {
        // pOutputData->regCmd.IFE.pRegCmd->configRegister.bitfields.LUT_BANK_SEL = pData->tableSel;

        for (UINT32 LUTEntry = 0; LUTEntry < GTM10LUTSize; LUTEntry++)
        {
            pOutputData->regCmd.IFE.pDMIDataPtr[LUTEntry] =
                (static_cast<UINT64>(pData->YratioBase[pData->tableSel][LUTEntry]) & Utils::AllOnes64(18)) |
                ((static_cast<UINT64>(pData->YratioSlope[pData->tableSel][LUTEntry]) & Utils::AllOnes64(26)) << 32);
        }
        if (TRUE != pOutputData->registerBETEn)
        {
            const HwEnvironment* pHwEnvironment = HwEnvironment::GetInstance();
            if ((pHwEnvironment->IsHWBugWorkaroundEnabled(Titan17xWorkarounds::Titan17xWorkaroundsCDMDMI64EndiannessBug)) &&
                ((FALSE == pHwEnvironment->GetStaticSettings()->ifeSWCDMEnable)))
            {
                for (UINT32 LUTEntry = 0; LUTEntry < GTM10LUTSize; LUTEntry++)
                {
                    pOutputData->regCmd.IFE.pDMIDataPtr[LUTEntry] =
                                ((pOutputData->regCmd.IFE.pDMIDataPtr[LUTEntry] & CamX::Utils::AllOnes64(32)) << 32) |
                                ((pOutputData->regCmd.IFE.pDMIDataPtr[LUTEntry] >> 32) & CamX::Utils::AllOnes64(32));
                }
            }
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
// IFEGTM10Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGTM10Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan17x::~IFEGTM10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGTM10Titan17x::~IFEGTM10Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGTM10Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEGTM10Titan17x::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
