////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedemosaic36titan480.cpp
/// @brief CAMXIFEDEMOSAIC36TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifedemosaic36titan480.h"
#include "demosaic36setting.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan480::IFEDemosaic36Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemosaic36Titan480::IFEDemosaic36Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEDemosaic36Titan480RegLengthDWord));

    // Hardcode initial value for all the registers
    m_regCmd.demosaic36RegCmd1.moduleConfig.u32All              = 0x00000001;
    m_regCmd.demosaic36RegCmd2.interpCoeffConfig.u32All         = 0x00000080;
    m_regCmd.demosaic36RegCmd2.interpClassifierConfig.u32All    = 0x00200066;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic36Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_DEMOSAIC_MODULE_CFG,
                                              IFEDemosaic36Titan480RegLengthDWord1,
                                              reinterpret_cast<UINT32*>(&m_regCmd.demosaic36RegCmd1));

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_DEMOSAIC_INTERP_COEFF_CFG,
                                                  IFEDemosaic36Titan480RegLengthDWord2,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.demosaic36RegCmd2));
        }

        if (CamxResultSuccess != result)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEFailed;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic36Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEDemosaic36Titan480RegCmd) <=
                           sizeof(pIFETuningMetadata->metadata480.IFEDemosaicData.demosaicConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEDemosaicData.demosaicConfig,
                      &m_regCmd,
                      sizeof(IFEDemosaic36Titan480RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic36Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult                result = CamxResultSuccess;
    Demosaic36UnpackedField*  pData  = static_cast<Demosaic36UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        // Need to determine how to program this register
        // m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.STRIPE_AUTO_CROP_DIS =
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.EN                   = pData->enable;
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.COSITED_RGB_EN       = pData->cositedRGB;
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.DIR_G_INTERP_DIS     = pData->disableDirectionalG;
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.DIR_RB_INTERP_DIS    = pData->disableDirectionalRB;
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.DYN_G_CLAMP_EN       = pData->enDynamicClampG;
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.DYN_RB_CLAMP_EN      = pData->enDynamicClampRB;

        m_regCmd.demosaic36RegCmd2.interpCoeffConfig.bitfields.LAMDA_G  = pData->lambdaG;
        m_regCmd.demosaic36RegCmd2.interpCoeffConfig.bitfields.LAMDA_RB = pData->lambdaRB;

        m_regCmd.demosaic36RegCmd2.interpClassifierConfig.bitfields.W_K = pData->wk;
        m_regCmd.demosaic36RegCmd2.interpClassifierConfig.bitfields.A_K = pData->ak;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input data is pData %p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic36Titan480::CreateSubCmdList(
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
            regIFE_IFE_0_PP_CLC_DEMOSAIC_MODULE_CFG,
            sizeof(m_regCmd.demosaic36RegCmd1.moduleConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.demosaic36RegCmd1.moduleConfig));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic36Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan480::~IFEDemosaic36Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemosaic36Titan480::~IFEDemosaic36Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDemosaic36Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demo moduleConfig                [0x%x]",
        m_regCmd.demosaic36RegCmd1.moduleConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demo interpolationCoeffConfig    [0x%x]",
        m_regCmd.demosaic36RegCmd2.interpCoeffConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demo interpolationClassifier0    [0x%x]",
        m_regCmd.demosaic36RegCmd2.interpClassifierConfig.u32All);
}

CAMX_NAMESPACE_END
