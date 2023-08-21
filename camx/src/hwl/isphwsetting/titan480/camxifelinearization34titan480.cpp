////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifelinearization34titan480.cpp
/// @brief CAMXIFELINEARIZATION34TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifelinearization34titan480.h"
#include "linearization34setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34Titan480::IFELinearization34Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELinearization34Titan480::IFELinearization34Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFELinearization34Titan480RegLengthDWord1) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFELinearization34Titan480RegLengthDWord2) +
                     PacketBuilder::RequiredWriteDMISizeInDwords());

    Set32bitDMILength(IFELinearization34Titan480DMILengthDWord);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization34Titan480::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;
    CmdBuffer*    pDMIBuffer    = NULL;
    UINT32        offset        = *pDMIBufferOffset * sizeof(UINT32);
    UINT32        lengthInByte  = IFELinearization34Titan480DMILengthDWord * sizeof(UINT32);

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p32bitDMIBuffer &&
        NULL != pInputData->p32bitDMIBufferAddr)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        pDMIBuffer = pInputData->p32bitDMIBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_LINEARIZATION_DMI_LUT_BANK_CFG,
                                              IFELinearization34Titan480RegLengthDWord1,
                                              reinterpret_cast<UINT32*>(&m_regCmd1));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_LINEARIZATION_KNEEPOINT_R_0_CFG,
                                                  IFELinearization34Titan480RegLengthDWord2,
                                                  reinterpret_cast<UINT32*>(&m_regCmd2));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_LINEARIZATION_DMI_CFG,
                                             LinearizationTitan480LUT,
                                             pDMIBuffer,
                                             offset,
                                             lengthInByte);
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Data");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization34Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFELinearization34Titan480RegCmd1) <=
            sizeof(pIFETuningMetadata->metadata480.IFELinearizationData.linearizationConfig1));
        Utils::Memcpy(pIFETuningMetadata->metadata480.IFELinearizationData.linearizationConfig1,
                      &m_regCmd1,
                      sizeof(IFELinearization34Titan480RegCmd1));

        CAMX_STATIC_ASSERT(sizeof(IFELinearization34Titan480RegCmd2) <=
            sizeof(pIFETuningMetadata->metadata480.IFELinearizationData.linearizationConfig2));
        Utils::Memcpy(pIFETuningMetadata->metadata480.IFELinearizationData.linearizationConfig2,
                      &m_regCmd2,
                      sizeof(IFELinearization34Titan480RegCmd2));

        if (NULL != m_pLUTDMIBuffer)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata480.IFEDMIPacked.linearizationLUT.linearizationLUT,
                          m_pLUTDMIBuffer,
                          IFELinearization34Titan480DMILengthDWord * sizeof(UINT32));
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
// IFELinearization34Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization34Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult                    result        = CamxResultSuccess;
    Linearization34UnpackedField* pData         = static_cast<Linearization34UnpackedField*>(pInput);
    Linearization34OutputData*    pOutputData   = static_cast<Linearization34OutputData*>(pOutput);
    UINT32                        DMILength     = 0;

    if ((NULL != pOutputData) && (NULL != pData) && (NULL != pOutputData->pDMIDataPtr))
    {
        m_regCmd1.configRegister.bitfields.EN                = 1;
        m_regCmd1.lutBankConfigRegister.bitfields.BANK_SEL   = pData->LUTbankSelection;
        m_regCmd1.dmiBankConfigRegister.bitfields.BANK_SEL   = pData->LUTbankSelection;

        /// Left plane
        m_regCmd2.kneeB0Register.bitfields.P0  = pData->bLUTkneePointL[0];
        m_regCmd2.kneeB0Register.bitfields.P1  = pData->bLUTkneePointL[1];
        m_regCmd2.kneeB1Register.bitfields.P2  = pData->bLUTkneePointL[2];
        m_regCmd2.kneeB1Register.bitfields.P3  = pData->bLUTkneePointL[3];
        m_regCmd2.kneeB2Register.bitfields.P4  = pData->bLUTkneePointL[4];
        m_regCmd2.kneeB2Register.bitfields.P5  = pData->bLUTkneePointL[5];
        m_regCmd2.kneeB3Register.bitfields.P6  = pData->bLUTkneePointL[6];
        m_regCmd2.kneeB3Register.bitfields.P7  = pData->bLUTkneePointL[7];

        m_regCmd2.kneeR0Register.bitfields.P0  = pData->rLUTkneePointL[0];
        m_regCmd2.kneeR0Register.bitfields.P1  = pData->rLUTkneePointL[1];
        m_regCmd2.kneeR1Register.bitfields.P2  = pData->rLUTkneePointL[2];
        m_regCmd2.kneeR1Register.bitfields.P3  = pData->rLUTkneePointL[3];
        m_regCmd2.kneeR2Register.bitfields.P4  = pData->rLUTkneePointL[4];
        m_regCmd2.kneeR2Register.bitfields.P5  = pData->rLUTkneePointL[5];
        m_regCmd2.kneeR3Register.bitfields.P6  = pData->rLUTkneePointL[6];
        m_regCmd2.kneeR3Register.bitfields.P7  = pData->rLUTkneePointL[7];

        m_regCmd2.kneeGB0Register.bitfields.P0 = pData->gbLUTkneePointL[0];
        m_regCmd2.kneeGB0Register.bitfields.P1 = pData->gbLUTkneePointL[1];
        m_regCmd2.kneeGB1Register.bitfields.P2 = pData->gbLUTkneePointL[2];
        m_regCmd2.kneeGB1Register.bitfields.P3 = pData->gbLUTkneePointL[3];
        m_regCmd2.kneeGB2Register.bitfields.P4 = pData->gbLUTkneePointL[4];
        m_regCmd2.kneeGB2Register.bitfields.P5 = pData->gbLUTkneePointL[5];
        m_regCmd2.kneeGB3Register.bitfields.P6 = pData->gbLUTkneePointL[6];
        m_regCmd2.kneeGB3Register.bitfields.P7 = pData->gbLUTkneePointL[7];

        m_regCmd2.kneeGR0Register.bitfields.P0 = pData->grLUTkneePointL[0];
        m_regCmd2.kneeGR0Register.bitfields.P1 = pData->grLUTkneePointL[1];
        m_regCmd2.kneeGR1Register.bitfields.P2 = pData->grLUTkneePointL[2];
        m_regCmd2.kneeGR1Register.bitfields.P3 = pData->grLUTkneePointL[3];
        m_regCmd2.kneeGR2Register.bitfields.P4 = pData->grLUTkneePointL[4];
        m_regCmd2.kneeGR2Register.bitfields.P5 = pData->grLUTkneePointL[5];
        m_regCmd2.kneeGR3Register.bitfields.P6 = pData->grLUTkneePointL[6];
        m_regCmd2.kneeGR3Register.bitfields.P7 = pData->grLUTkneePointL[7];

        pOutputData->dynamicBlackLevel[0]      = m_regCmd2.kneeR0Register.bitfields.P0;
        pOutputData->dynamicBlackLevel[1]      = m_regCmd2.kneeGR0Register.bitfields.P0;
        pOutputData->dynamicBlackLevel[2]      = m_regCmd2.kneeGB0Register.bitfields.P0;
        pOutputData->dynamicBlackLevel[3]      = m_regCmd2.kneeB0Register.bitfields.P0;

        ///< packing the DMI table
        for (UINT32 count = 0; count < MAX_SLOPE; count++, DMILength += 4)
        {
            pOutputData->pDMIDataPtr[DMILength] =
                 ((static_cast<UINT32>(pData->rLUTbaseL[pData->LUTbankSelection][count])) & Utils::AllOnes32(14)) |
                 ((static_cast<UINT32>(pData->rLUTdeltaL[pData->LUTbankSelection][count]) & Utils::AllOnes32(14)) << 14);

            pOutputData->pDMIDataPtr[DMILength + 1] =
                 ((static_cast<UINT32>(pData->grLUTbaseL[pData->LUTbankSelection][count])) & Utils::AllOnes32(14)) |
                 ((static_cast<UINT32>(pData->grLUTdeltaL[pData->LUTbankSelection][count]) & Utils::AllOnes32(14)) << 14);

            pOutputData->pDMIDataPtr[DMILength + 2] =
                 ((static_cast<UINT32>(pData->gbLUTbaseL[pData->LUTbankSelection][count])) & Utils::AllOnes32(14)) |
                 ((static_cast<UINT32>(pData->gbLUTdeltaL[pData->LUTbankSelection][count]) & Utils::AllOnes32(14)) << 14);

            pOutputData->pDMIDataPtr[DMILength + 3] =
                 ((static_cast<UINT32>(pData->bLUTbaseL[pData->LUTbankSelection][count])) & Utils::AllOnes32(14)) |
                 ((static_cast<UINT32>(pData->bLUTdeltaL[pData->LUTbankSelection][count]) & Utils::AllOnes32(14)) << 14);
        }

        m_pLUTDMIBuffer = pOutputData->pDMIDataPtr;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input params are NULL pData=0x%p pOutputData=0x%p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization34Titan480::CreateSubCmdList(
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
            regIFE_IFE_0_PP_CLC_LINEARIZATION_MODULE_CFG,
            sizeof(m_regCmd1.configRegister) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd1.configRegister));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFELinearization34Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd1.configRegister.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34Titan480::~IFELinearization34Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFELinearization34Titan480::~IFELinearization34Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFELinearization34Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFELinearization34Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 LUT Bank Config   0x%x", m_regCmd1.lutBankConfigRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 Module Config     0x%x", m_regCmd1.configRegister.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee R0  Register 0x%x", m_regCmd2.kneeR0Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee R1  Register 0x%x", m_regCmd2.kneeR1Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee R2  Register 0x%x", m_regCmd2.kneeR2Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee R3  Register 0x%x", m_regCmd2.kneeR3Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee GR0 Register 0x%x", m_regCmd2.kneeGR0Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee GR1 Register 0x%x", m_regCmd2.kneeGR1Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee GR2 Register 0x%x", m_regCmd2.kneeGR2Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee GR3 Register 0x%x", m_regCmd2.kneeGR3Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee B0  Register 0x%x", m_regCmd2.kneeB0Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee B1  Register 0x%x", m_regCmd2.kneeB1Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee B2  Register 0x%x", m_regCmd2.kneeB2Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee B3  Register 0x%x", m_regCmd2.kneeB3Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee GB0 Register 0x%x", m_regCmd2.kneeGB0Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee GB1 Register 0x%x", m_regCmd2.kneeGB1Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee GB2 Register 0x%x", m_regCmd2.kneeGB2Register.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Linearization34 knee GB3 Register 0x%x", m_regCmd2.kneeGB3Register.u32All);
}

CAMX_NAMESPACE_END
