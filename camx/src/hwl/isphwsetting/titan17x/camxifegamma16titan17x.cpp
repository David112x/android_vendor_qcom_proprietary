////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifegamma16titan17x.cpp
/// @brief CAMXIFEGAMMA16TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifegamma16titan17x.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan17x::IFEGamma16Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGamma16Titan17x::IFEGamma16Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEGamma16RegLengthDword) +
                     PacketBuilder::RequiredWriteDMISizeInDwords() * IFEGamma16NumDMITables);
    Set32bitDMILength(IFEGamma16DMILengthDword);

    // By default, Bank Selection set to 0
    m_channelGLUTBankSelect = GammaGLUTBank0;
    m_channelBLUTBankSelect = GammaBLUTBank0;
    m_channelRLUTBankSelect = GammaRLUTBank0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16Titan17x::CreateCmdList(
    VOID* pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    UINT32        offset        = *pDMIBufferOffset * sizeof(UINT32);
    UINT32        lengthInByte  = Gamma16NumberOfEntriesPerLUT * sizeof(UINT32);
    UINT8         bankSelect    = 0;

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p32bitDMIBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;
        CmdBuffer* pDMIBuffer = pInputData->p32bitDMIBuffer;

        if (TRUE == pInputData->bankUpdate.isValid)
        {
            m_channelGLUTBankSelect = (pInputData->bankUpdate.gammaBank == 0) ? GammaGLUTBank0 : GammaGLUTBank1;
            m_channelBLUTBankSelect = (pInputData->bankUpdate.gammaBank == 0) ? GammaBLUTBank0 : GammaBLUTBank1;
            m_channelRLUTBankSelect = (pInputData->bankUpdate.gammaBank == 0) ? GammaRLUTBank0 : GammaRLUTBank1;
        }

        // Can't pass &m_regCmd.rgb_lut_cfg directly since taking the
        // address of a packed struct member can cause alignment errors
        IFE_IFE_0_VFE_RGB_LUT_CFG rgb_lut_cfg = m_regCmd.rgb_lut_cfg;
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                regIFE_IFE_0_VFE_RGB_LUT_CFG,
                IFEGamma16RegLengthDword,
                reinterpret_cast<UINT32*>(&rgb_lut_cfg));
        m_regCmd.rgb_lut_cfg = rgb_lut_cfg;

        result = PacketBuilder::WriteDMI(pCmdBuffer,
                regIFE_IFE_0_VFE_DMI_CFG,
                m_channelGLUTBankSelect,
                pDMIBuffer,
                offset,
                lengthInByte);
        CAMX_ASSERT(CamxResultSuccess == result);

        offset   += lengthInByte;
        result = PacketBuilder::WriteDMI(pCmdBuffer,
                regIFE_IFE_0_VFE_DMI_CFG,
                m_channelBLUTBankSelect,
                pDMIBuffer,
                offset,
                lengthInByte);
        CAMX_ASSERT(CamxResultSuccess == result);

        offset   += lengthInByte;
        result = PacketBuilder::WriteDMI(pCmdBuffer,
                regIFE_IFE_0_VFE_DMI_CFG,
                m_channelRLUTBankSelect,
                pDMIBuffer,
                offset,
                lengthInByte);
        CAMX_ASSERT(CamxResultSuccess == result);

        m_channelGLUTBankSelect = (m_channelGLUTBankSelect == GammaGLUTBank0) ? GammaGLUTBank1 : GammaGLUTBank0;
        m_channelBLUTBankSelect = (m_channelBLUTBankSelect == GammaBLUTBank0) ? GammaBLUTBank1 : GammaBLUTBank0;
        m_channelRLUTBankSelect = (m_channelRLUTBankSelect == GammaRLUTBank0) ? GammaRLUTBank1 : GammaRLUTBank0;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEGamma16RegCmd) == sizeof(pIFETuningMetadata->metadata17x.IFEGammaData.gammaConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEGammaData.gammaConfig, &m_regCmd, sizeof(IFEGamma16RegCmd));

        if (NULL != m_pGammaGLUT)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata17x.IFEDMIPacked.gamma[GammaLUTChannelG].curve,
                          m_pGammaGLUT,
                          Gamma16NumberOfEntriesPerLUT * sizeof(UINT32));
        }

        if (NULL != m_pGammaBLUT)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata17x.IFEDMIPacked.gamma[GammaLUTChannelB].curve,
                          m_pGammaBLUT,
                          Gamma16NumberOfEntriesPerLUT * sizeof(UINT32));
        }

        if (NULL != m_pGammaRLUT)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata17x.IFEDMIPacked.gamma[GammaLUTChannelR].curve,
                          m_pGammaRLUT,
                          Gamma16NumberOfEntriesPerLUT * sizeof(UINT32));
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
// IFEGamma16Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult            result        = CamxResultSuccess;
    Gamma16UnpackedField* pData         = static_cast<Gamma16UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd.rgb_lut_cfg.bitfields.CH0_BANK_SEL = pData->r_lut_in_cfg.tableSelect;
        m_regCmd.rgb_lut_cfg.bitfields.CH1_BANK_SEL = pData->g_lut_in_cfg.tableSelect;
        m_regCmd.rgb_lut_cfg.bitfields.CH2_BANK_SEL = pData->b_lut_in_cfg.tableSelect;

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
// IFEGamma16Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEGamma16Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan17x::~IFEGamma16Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEGamma16Titan17x::~IFEGamma16Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEGamma16Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEGamma16Titan17x::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
