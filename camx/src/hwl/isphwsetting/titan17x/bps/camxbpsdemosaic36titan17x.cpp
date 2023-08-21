////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsdemosaic36titan17x.cpp
/// @brief CAMXBPSDEMOSAIC36TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpsdemosaic36titan17x.h"
#include "demosaic36setting.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN
CAMX_BEGIN_PACKED


/// @brief Demosaic register Configuration
struct BPSDemosaic36RegConfig
{
    BPS_BPS_0_CLC_DEMOSAIC_INTERP_COEFF_CFG      interpolationCoefficientConfig; ///< interpolation coefficent config
    BPS_BPS_0_CLC_DEMOSAIC_INTERP_CLASSIFIER_CFG interpolationClassifierConfig;  ///< interpolation classifier config
} CAMX_PACKED;

/// @brief Demosaic module Configuration
struct BPSDemosaic36ModuleConfig
{
    BPS_BPS_0_CLC_DEMOSAIC_MODULE_CFG  moduleConfig;    ///< Module configuration
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSDemosaic36RegLengthDWord = sizeof(BPSDemosaic36RegConfig) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36Titan17x::BPSDemosaic36Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSDemosaic36Titan17x::BPSDemosaic36Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSDemosaic36Titan17x* pHWSetting = CAMX_NEW BPSDemosaic36Titan17x;

    if (NULL != pHWSetting)
    {
        result = pHWSetting->Initialize();
        if (CamxResultSuccess == result)
        {
            (*ppHWSetting) = pHWSetting;
        }
        else
        {
            CAMX_DELETE pHWSetting;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to initialize in %s, no memory", __FUNCTION__);
        }
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "%s failed, no memory", __FUNCTION__);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSDemosaic36Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pModuleConfig = CAMX_CALLOC(sizeof(BPSDemosaic36ModuleConfig));
    if (NULL == m_pModuleConfig)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for module configuration!");
    }
    else
    {
        BPSDemosaic36RegConfig* pRegCmd = static_cast<BPSDemosaic36RegConfig*>(CAMX_CALLOC(sizeof(BPSDemosaic36RegConfig)));

        if (NULL != pRegCmd)
        {
            m_pRegCmd = pRegCmd;
            SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSDemosaic36RegLengthDWord));

            // Hardcode initial value for all the registers
            pRegCmd->interpolationCoefficientConfig.u32All = 0x00000080;
            pRegCmd->interpolationClassifierConfig.u32All  = 0x00800066;
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for register configuration!");

            CAMX_FREE(m_pModuleConfig);
            m_pModuleConfig = NULL;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData->pCmdBuffer)
    {
        pCmdBuffer = pInputData->pCmdBuffer;
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_DEMOSAIC_INTERP_COEFF_CFG,
                                              BPSDemosaic36RegLengthDWord,
                                              static_cast<UINT32*>(m_pRegCmd));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input data or command buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*              pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings*             pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSDemosaic36ModuleConfig* pModuleCfg     = static_cast<BPSDemosaic36ModuleConfig*>(m_pModuleConfig);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->demosaicParameters.moduleCfg.EN = moduleEnable;

    pBPSIQSettings->demosaicParameters.moduleCfg.COSITED_RGB_EN    = pModuleCfg->moduleConfig.bitfields.COSITED_RGB_EN;
    pBPSIQSettings->demosaicParameters.moduleCfg.DIR_G_INTERP_DIS  = pModuleCfg->moduleConfig.bitfields.DIR_G_INTERP_DIS;
    pBPSIQSettings->demosaicParameters.moduleCfg.DIR_RB_INTERP_DIS = pModuleCfg->moduleConfig.bitfields.DIR_RB_INTERP_DIS;
    pBPSIQSettings->demosaicParameters.moduleCfg.DYN_G_CLAMP_EN    = pModuleCfg->moduleConfig.bitfields.DYN_G_CLAMP_EN;
    pBPSIQSettings->demosaicParameters.moduleCfg.DYN_RB_CLAMP_EN   = pModuleCfg->moduleConfig.bitfields.DYN_RB_CLAMP_EN;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult              result             = CamxResultSuccess;
    ISPInputData*           pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata*      pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*          pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSDemosaic36RegConfig* pRegCmd            = static_cast<BPSDemosaic36RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSDemosaic36RegConfig) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSDemosaicData.demosaicConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSDemosaicData.demosaicConfig,
                        pRegCmd,
                        sizeof(BPSDemosaic36RegConfig));

        if (TRUE == pBPSIQSettings->demosaicParameters.moduleCfg.EN)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSDemosaicRegister,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata17x.BPSDemosaicData.demosaicConfig),
                &pBPSTuningMetadata->BPSTuningMetadata17x.BPSDemosaicData.demosaicConfig,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSDemosaicData.demosaicConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSDemosaic36Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult               result  = CamxResultSuccess;
    Demosaic36UnpackedField* pData   = static_cast<Demosaic36UnpackedField*>(pInput);

    BPSDemosaic36RegConfig*    pRegCmd    = static_cast<BPSDemosaic36RegConfig*>(m_pRegCmd);
    BPSDemosaic36ModuleConfig* pModuleCfg = static_cast<BPSDemosaic36ModuleConfig*>(m_pModuleConfig);

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        pRegCmd->interpolationCoefficientConfig.bitfields.LAMDA_RB = pData->lambdaRB;
        pRegCmd->interpolationCoefficientConfig.bitfields.LAMDA_G  = pData->lambdaG;
        pRegCmd->interpolationClassifierConfig.bitfields.A_K       = pData->ak;
        pRegCmd->interpolationClassifierConfig.bitfields.W_K       = pData->wk;

        pModuleCfg->moduleConfig.bitfields.COSITED_RGB_EN       = pData->cositedRGB;
        pModuleCfg->moduleConfig.bitfields.DIR_G_INTERP_DIS     = pData->disableDirectionalG;
        pModuleCfg->moduleConfig.bitfields.DIR_RB_INTERP_DIS    = pData->disableDirectionalRB;
        pModuleCfg->moduleConfig.bitfields.DYN_G_CLAMP_EN       = pData->enDynamicClampG;
        pModuleCfg->moduleConfig.bitfields.DYN_RB_CLAMP_EN      = pData->enDynamicClampRB;

    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input data is pData %p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36Titan17x::~BPSDemosaic36Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSDemosaic36Titan17x::~BPSDemosaic36Titan17x()
{
    CAMX_FREE(m_pModuleConfig);
    m_pModuleConfig = NULL;

    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSDemosaic36Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSDemosaic36Titan17x::DumpRegConfig()
{
    BPSDemosaic36RegConfig* pRegCmd = static_cast<BPSDemosaic36RegConfig*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "interpolation Coefficient Config = %x\n", pRegCmd->interpolationCoefficientConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "interpolation Classifier Config  = %x\n", pRegCmd->interpolationClassifierConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "interpolation Coefficient Config = %x\n", pRegCmd->interpolationCoefficientConfig);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "interpolation Classifier Config  = %x\n", pRegCmd->interpolationClassifierConfig);
    }
}

CAMX_NAMESPACE_END
