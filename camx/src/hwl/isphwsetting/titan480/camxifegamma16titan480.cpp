////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifegamma16titan480.cpp
/// @brief CAMXIFEGAMMA16TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifegamma16titan480.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan480::IFEGamma16Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGamma16Titan480::IFEGamma16Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEGamma16Titan480RegLengthDword) +
                     PacketBuilder::RequiredWriteDMISizeInDwords() * IFEGamma16Titan480NumDMITables);
    Set32bitDMILength(IFEGamma16Titan480DMILengthDword);

    // By default, Bank Selection set to 0
    m_channelLUTBankSelect = Gamma16Titan480LUTBank0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    UINT32        offset        = *pDMIBufferOffset * sizeof(UINT32);
    UINT32        lengthInByte  = Gamma16Titan480NumberOfEntriesPerLUT * sizeof(UINT32);
    UINT8         bankSelect    = 0;

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p32bitDMIBuffer)
    {
        CmdBuffer* pCmdBuffer   = pInputData->pCmdBuffer;
        CmdBuffer* pDMIBuffer   = pInputData->p32bitDMIBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_GLUT_DMI_LUT_BANK_CFG,
                                              sizeof(m_regCmd) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&m_regCmd));
        if (CamxResultSuccess == result)
        {
            // "lengthInByte" is same for all three DMI.
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_GLUT_DMI_CFG,
                                              Gamma16Titan480Channel0LUT,
                                              pDMIBuffer,
                                              offset,
                                              lengthInByte);
            CAMX_ASSERT(CamxResultSuccess == result);

            offset   += lengthInByte;
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_GLUT_DMI_CFG,
                                             Gamma16Titan480Channel1LUT,
                                             pDMIBuffer,
                                             offset,
                                             lengthInByte);
            CAMX_ASSERT(CamxResultSuccess == result);

            offset   += lengthInByte;
            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_GLUT_DMI_CFG,
                                             Gamma16Titan480Channel2LUT,
                                             pDMIBuffer,
                                             offset,
                                             lengthInByte);
            CAMX_ASSERT(CamxResultSuccess == result);
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
// IFEGamma16Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16Titan480::CreateSubCmdList(
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
                                              regIFE_IFE_0_PP_CLC_GLUT_MODULE_CFG,
                                              sizeof(m_regCmd.config) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&m_regCmd.config));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEGamma16Titan480RegCmd) ==
                           sizeof(pIFETuningMetadata->metadata480.IFEGammaData.gammaConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEGammaData.gammaConfig, &m_regCmd, sizeof(IFEGamma16Titan480RegCmd));

        if (NULL != m_pGammaGLUT)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata480.IFEDMIPacked.gamma[GammaLUTChannelG].curve,
                          m_pGammaGLUT,
                          Gamma16Titan480NumberOfEntriesPerLUT * sizeof(UINT32));
        }

        if (NULL != m_pGammaBLUT)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata480.IFEDMIPacked.gamma[GammaLUTChannelB].curve,
                          m_pGammaBLUT,
                          Gamma16Titan480NumberOfEntriesPerLUT * sizeof(UINT32));
        }

        if (NULL != m_pGammaRLUT)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata480.IFEDMIPacked.gamma[GammaLUTChannelR].curve,
                          m_pGammaRLUT,
                          Gamma16Titan480NumberOfEntriesPerLUT * sizeof(UINT32));
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
// IFEGamma16Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult            result = CamxResultSuccess;
    Gamma16UnpackedField* pData  = static_cast<Gamma16UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd.config.bitfields.EN                    = pData->enable;
        m_regCmd.DMILUTBankConfig.bitfields.BANK_SEL    = pData->r_lut_in_cfg.tableSelect; // Using the same bank
        m_regCmd.moduleLUTBankConfig.bitfields.BANK_SEL = pData->r_lut_in_cfg.tableSelect; // Using the same bank

        // Store DMI pointers
        m_pGammaRLUT = pData->r_lut_in_cfg.pGammaTable;
        m_pGammaGLUT = pData->g_lut_in_cfg.pGammaTable;
        m_pGammaBLUT = pData->b_lut_in_cfg.pGammaTable;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd.config.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan480::~IFEGamma16Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGamma16Titan480::~IFEGamma16Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEGamma16Titan480::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
