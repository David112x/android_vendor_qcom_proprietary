////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifedemosaic37titan17x.cpp
/// @brief CAMXIFEDEMOSAIC37TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifedemosaic37titan17x.h"
#include "camxispiqmodule.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 IFEDemosaic37RegLengthDWord  = sizeof(IFEDemosaic37RegCmd)  / sizeof(UINT32);
static const UINT32 IFEDemosaic37RegLengthDWord1 = sizeof(IFEDemosaic37RegCmd1) / sizeof(UINT32);
static const UINT32 IFEDemosaic37RegLengthDWord2 = sizeof(IFEDemosaic37RegCmd2) / sizeof(UINT32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37Titan17x::IFEDemosaic37Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemosaic37Titan17x::IFEDemosaic37Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEDemosaic37RegLengthDWord)  +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEDemosaic37RegLengthDWord1) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEDemosaic37RegLengthDWord2));

    // Hardcode initial value for all the registers
    m_regCmd.demosaic37RegCmd1.moduleConfig.u32All             = 0x00000100;
    m_regCmd.demosaic37RegCmd2.interpolationCoeffConfig.u32All = 0x00008000;
    m_regCmd.demosaic37RegCmd2.interpolationClassifier0.u32All = 0x08000066;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic37Titan17x::CreateCmdList(
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
                                              IFEDemosaic37RegLengthDWord1,
                                              reinterpret_cast<UINT32*>(&m_regCmd.demosaic37RegCmd1));

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_DEMO_INTERP_COEFF_CFG,
                                                  IFEDemosaic37RegLengthDWord2,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.demosaic37RegCmd2));
        }

        if (CamxResultSuccess != result)
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic37Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEDemosaic37RegCmd) <=
                           sizeof(pIFETuningMetadata->metadata17x.IFEDemosaicData.demosaicConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEDemosaicData.demosaicConfig, &m_regCmd, sizeof(IFEDemosaic37RegCmd));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic37Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutput)
{
    CamxResult                result = CamxResultSuccess;
    Demosaic37UnpackedField*  pData  = static_cast<Demosaic37UnpackedField*>(pInput);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        m_regCmd.demosaic37RegCmd1.moduleConfig.bitfields.DIR_G_INTERP_DIS     = pData->disableDirectionalG;
        m_regCmd.demosaic37RegCmd1.moduleConfig.bitfields.DIR_RB_INTERP_DIS    = pData->disableDirectionalRB;
        m_regCmd.demosaic37RegCmd1.moduleConfig.bitfields.COSITED_RGB_EN       = pData->cositedRGB;
        m_regCmd.demosaic37RegCmd1.moduleConfig.bitfields.DYN_G_CLAMP_EN       = pData->enDynamicClampG;
        m_regCmd.demosaic37RegCmd1.moduleConfig.bitfields.DYN_RB_CLAMP_EN      = pData->enDynamicClampRB;

        // Disable LB if module enable
        m_regCmd.demosaic37RegCmd1.moduleConfig.bitfields.LB_ONLY_EN           = !pData->enable;

        m_regCmd.demosaic37RegCmd2.interpolationCoeffConfig.bitfields.LAMDA_RB = pData->lambdaRB;
        m_regCmd.demosaic37RegCmd2.interpolationCoeffConfig.bitfields.LAMDA_G  = pData->lambdaG;

        m_regCmd.demosaic37RegCmd2.interpolationClassifier0.bitfields.A_N      = pData->ak;
        m_regCmd.demosaic37RegCmd2.interpolationClassifier0.bitfields.W_N      = pData->wk;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Input data is pData %p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEDemosaic37Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    m_regCmd.demosaic37RegCmd1.moduleConfig.bitfields.LB_ONLY_EN = 1;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37Titan17x::~IFEDemosaic37Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEDemosaic37Titan17x::~IFEDemosaic37Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEDemosaic37Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEDemosaic37Titan17x::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demo moduleConfig                [0x%x]",
        m_regCmd.demosaic37RegCmd1.moduleConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demo interpolationCoeffConfig    [0x%x]",
        m_regCmd.demosaic37RegCmd2.interpolationCoeffConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Demo interpolationClassifier0    [0x%x]",
        m_regCmd.demosaic37RegCmd2.interpolationClassifier0.u32All);
}

CAMX_NAMESPACE_END
