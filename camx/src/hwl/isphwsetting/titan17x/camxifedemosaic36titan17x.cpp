////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedemosaic36titan17x.cpp
/// @brief CAMXIFEDEMOSAIC36TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifedemosaic36titan17x.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan17x::IFEDemosaic36Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemosaic36Titan17x::IFEDemosaic36Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEDemosaic36RegLengthDWord));

    // Hardcode initial value for all the registers
    m_regCmd.demosaic36RegCmd1.moduleConfig.u32All             = 0x00000100;
    m_regCmd.demosaic36RegCmd2.interpolationCoeffConfig.u32All = 0x00008000;
    m_regCmd.demosaic36RegCmd2.interpolationClassifier0.u32All = 0x08000066;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic36Titan17x::CreateCmdList(
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
                                              regIFE_IFE_0_VFE_DEMO_CFG,
                                              IFEDemosaic36RegLengthDWord1,
                                              reinterpret_cast<UINT32*>(&m_regCmd.demosaic36RegCmd1));

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_DEMO_INTERP_COEFF_CFG,
                                                  IFEDemosaic36RegLengthDWord2,
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
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic36Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEDemosaic36RegCmd) <=
                           sizeof(pIFETuningMetadata->metadata17x.IFEDemosaicData.demosaicConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEDemosaicData.demosaicConfig,
                      &m_regCmd,
                      sizeof(IFEDemosaic36RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic36Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult                result = CamxResultSuccess;
    Demosaic36UnpackedField*  pData  = static_cast<Demosaic36UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.DIR_G_INTERP_DIS  = pData->disableDirectionalG;
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.DIR_RB_INTERP_DIS = pData->disableDirectionalRB;
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.COSITED_RGB_EN    = pData->cositedRGB;
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.DYN_G_CLAMP_EN    = pData->enDynamicClampG;
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.DYN_RB_CLAMP_EN   = pData->enDynamicClampRB;
        m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.LB_ONLY_EN        = !pData->enable; ///< Disable LB if module enable

        m_regCmd.demosaic36RegCmd2.interpolationCoeffConfig.bitfields.LAMDA_RB = pData->lambdaRB;
        m_regCmd.demosaic36RegCmd2.interpolationCoeffConfig.bitfields.LAMDA_G  = pData->lambdaG;

        m_regCmd.demosaic36RegCmd2.interpolationClassifier0.bitfields.A_N = pData->ak;
        m_regCmd.demosaic36RegCmd2.interpolationClassifier0.bitfields.W_N = pData->wk;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input data is pData %p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic36Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    m_regCmd.demosaic36RegCmd1.moduleConfig.bitfields.LB_ONLY_EN = 1;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan17x::~IFEDemosaic36Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemosaic36Titan17x::~IFEDemosaic36Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic36Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDemosaic36Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demo moduleConfig                [0x%x]",
        m_regCmd.demosaic36RegCmd1.moduleConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demo interpolationCoeffConfig    [0x%x]",
        m_regCmd.demosaic36RegCmd2.interpolationCoeffConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demo interpolationClassifier0    [0x%x]",
        m_regCmd.demosaic36RegCmd2.interpolationClassifier0.u32All);
}

CAMX_NAMESPACE_END
