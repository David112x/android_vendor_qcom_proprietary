////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpspedestal13titan17x.cpp
/// @brief CAMXBPSPEDESTAL13TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpspedestal13titan17x.h"
#include "pedestal13setting.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief Pedestal register Configuration
struct BPSPedestal13RegConfig
{
    BPS_BPS_0_CLC_PEDESTAL_MODULE_1_CFG module1Config;    ///< Pedestal module 1 config
    BPS_BPS_0_CLC_PEDESTAL_MODULE_2_CFG module2Config;    ///< Pedestal module 2 config
    BPS_BPS_0_CLC_PEDESTAL_MODULE_3_CFG module3Config;    ///< Pedestal module 3 config
    BPS_BPS_0_CLC_PEDESTAL_MODULE_4_CFG module4Config;    ///< Pedestal module 4 config
    BPS_BPS_0_CLC_PEDESTAL_MODULE_5_CFG module5Config;    ///< Pedestal module 5 config
} CAMX_PACKED;

/// @brief Pedestal module Configuration
struct BPSPedestal13ModuleConfig
{
    BPS_BPS_0_CLC_PEDESTAL_MODULE_CFG moduleConfig;    ///< Module configuration
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSPedestal13RegLengthDword = sizeof(BPSPedestal13RegConfig) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13Titan17x::BPSPedestal13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSPedestal13Titan17x::BPSPedestal13Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSPedestal13Titan17x* pHWSetting = CAMX_NEW BPSPedestal13Titan17x;

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
/// BPSPedestal13Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pModuleConfig = CAMX_CALLOC(sizeof(BPSPedestal13ModuleConfig));

    if (NULL == m_pModuleConfig)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for module configuration!");
    }
    else
    {
        m_pRegCmd = CAMX_CALLOC(sizeof(BPSPedestal13RegConfig));

        if (NULL != m_pRegCmd)
        {
            SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSPedestal13RegLengthDword));
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
// CamxResult BPSPedestal13Titan17x::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13Titan17x::WriteLUTtoDMI(
    VOID* pInput)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pInput);

    // CDM pack the DMI buffer and patch the Red and Blue LUT DMI buffer into CDM pack
    if ((NULL != pInputData) && (NULL != pInputData->pDMICmdBuffer))
    {
        UINT32 LUTOffset = 0;
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_PEDESTAL_DMI_CFG,
                                         RedLUTPedestal,
                                         pInputData->p64bitDMIBuffer,
                                         LUTOffset,
                                         BPSPedestal13DMISetSizeDword * sizeof(UINT32));
        if (CamxResultSuccess == result)
        {
            LUTOffset = (BPSPedestal13DMISetSizeDword * sizeof(UINT32));
            result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                             regBPS_BPS_0_CLC_PEDESTAL_DMI_CFG,
                                             BlueLUTPedestal,
                                             pInputData->p64bitDMIBuffer,
                                             LUTOffset,
                                             BPSPedestal13DMISetSizeDword * sizeof(UINT32));
        }
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write DMI data");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer = pInputData->pCmdBuffer;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    result = WriteLUTtoDMI(pInputData);

    if ((CamxResultSuccess == result) && (NULL != pInputData->pCmdBuffer))
    {
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_PEDESTAL_MODULE_1_CFG,
                                              BPSPedestal13RegLengthDword,
                                              static_cast<UINT32*>(m_pRegCmd));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write command buffer");
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    BPSPedestal13RegConfig*    pRegCmd    = static_cast<BPSPedestal13RegConfig*>(m_pRegCmd);
    BPSPedestal13ModuleConfig* pModuleCfg = static_cast<BPSPedestal13ModuleConfig*>(m_pModuleConfig);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->pedestalParameters.moduleCfg.EN            = moduleEnable;

    pBPSIQSettings->pedestalParameters.moduleCfg.SCALE_BYPASS  = pModuleCfg->moduleConfig.bitfields.SCALE_BYPASS;
    pBPSIQSettings->pedestalParameters.moduleCfg.NUM_SUBBLOCKS = pModuleCfg->moduleConfig.bitfields.NUM_SUBBLOCKS;
    pBPSIQSettings->pedestalParameters.subgridWidth            = pRegCmd->module2Config.bitfields.SUBBLOCK_WIDTH;
    pBPSIQSettings->pedestalParameters.meshGridBwidth          = pRegCmd->module1Config.bitfields.BLOCK_WIDTH;
    pBPSIQSettings->pedestalParameters.startBlockIndex         = pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_X;
    pBPSIQSettings->pedestalParameters.startSubgridIndex       = pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_X;
    pBPSIQSettings->pedestalParameters.topLeftCoordinate       = pRegCmd->module4Config.bitfields.INIT_PIXEL_X;

    pBPSIQSettings->pedestalParameters.initBlockY              = pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_Y;
    pBPSIQSettings->pedestalParameters.initSubblockY           = pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_Y;
    pBPSIQSettings->pedestalParameters.initPixelY              = pRegCmd->module4Config.bitfields.INIT_PIXEL_X;

    pInputData->pCalculatedData->blackLevelLock = pInputData->pHALTagsData->blackLevelLock;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult              result             = CamxResultSuccess;
    ISPInputData*           pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata*      pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*          pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSPedestal13RegConfig* pRegCmd            = static_cast<BPSPedestal13RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSPedestal13RegConfig) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSPedestalData.pedestalConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSPedestalData.pedestalConfig,
            pRegCmd,
            sizeof(BPSPedestal13RegConfig));

        if (TRUE == pBPSIQSettings->pedestalParameters.moduleCfg.EN)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSPedestalRegister,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata17x.BPSPedestalData.pedestalConfig),
                &pBPSTuningMetadata->BPSTuningMetadata17x.BPSPedestalData.pedestalConfig,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSPedestalData.pedestalConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPedestal13Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult               result = CamxResultSuccess;
    Pedestal13UnpackedField* pData  = static_cast<Pedestal13UnpackedField*>(pInput);
    Pedestal13OutputData*    pCmd   = static_cast<Pedestal13OutputData*>(pOutput);

    BPSPedestal13RegConfig*    pRegCmd    = static_cast<BPSPedestal13RegConfig*>(m_pRegCmd);
    BPSPedestal13ModuleConfig* pModuleCfg = static_cast<BPSPedestal13ModuleConfig*>(m_pModuleConfig);

    UINT32 DMICnt = 0;

    if ((NULL != pData) && (NULL != pCmd))
    {

       // Need to check on how to get these values
        pModuleCfg->moduleConfig.bitfields.SCALE_BYPASS      = pData->scaleBypass;
        pModuleCfg->moduleConfig.bitfields.NUM_SUBBLOCKS     = pData->intpFactorL;
        pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_X      = pData->lxStartL;
        pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_X   = pData->bxStartL;
        pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_Y      = pData->lyStartL;
        pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_Y   = pData->byStartL;

        pRegCmd->module1Config.bitfields.BLOCK_HEIGHT        = pData->meshGridbHeightL;
        pRegCmd->module1Config.bitfields.BLOCK_WIDTH         = pData->meshGridbWidthL;
        pRegCmd->module2Config.bitfields.INV_SUBBLOCK_WIDTH  = pData->xDeltaL;
        pRegCmd->module2Config.bitfields.SUBBLOCK_WIDTH      = pData->bWidthL;
        pRegCmd->module3Config.bitfields.INV_SUBBLOCK_HEIGHT = pData->yDeltaL;
        pRegCmd->module3Config.bitfields.SUBBLOCK_HEIGHT     = pData->bHeightL;
        pRegCmd->module4Config.bitfields.INIT_PIXEL_X        = pData->bxD1L;
        pRegCmd->module4Config.bitfields.INIT_PIXEL_Y        = pData->byE1L;
        pRegCmd->module5Config.bitfields.INIT_YDELTA         = pData->byInitE1L;

        for (UINT32 verMeshNum = 0; verMeshNum < PED_MESH_PT_V_V13; verMeshNum++)
        {
            for (UINT32 horMeshNum = 0; horMeshNum < PED_MESH_PT_H_V13; horMeshNum++)
            {
                // 0->R, 1->Gr, 2->Gb, 3->B
                pCmd->pGRRLUTDMIBuffer[DMICnt] =
                    ((pData->meshTblT1L[pData->bankSel][0][verMeshNum][horMeshNum] & Utils::AllOnes32(12)) |
                    ((pData->meshTblT1L[pData->bankSel][1][verMeshNum][horMeshNum] & Utils::AllOnes32(12)) << 12));
                pCmd->pGBBLUTDMIBuffer[DMICnt] =
                    ((pData->meshTblT1L[pData->bankSel][3][verMeshNum][horMeshNum] & Utils::AllOnes32(12)) |
                    ((pData->meshTblT1L[pData->bankSel][2][verMeshNum][horMeshNum] & Utils::AllOnes32(12)) << 12));
                DMICnt++;
            }
        }
        CAMX_ASSERT(PED_LUT_LENGTH == DMICnt);
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "data is  pData %p pCmd %p ", pData, pCmd);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13Titan17x::~BPSPedestal13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSPedestal13Titan17x::~BPSPedestal13Titan17x()
{
    CAMX_FREE(m_pModuleConfig);
    m_pModuleConfig = NULL;

    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPedestal13Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSPedestal13Titan17x::DumpRegConfig()
{
    BPSPedestal13RegConfig* pRegCmd = static_cast<BPSPedestal13RegConfig*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "module 1 Config = %x\n", pRegCmd->module1Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "module 2 Config = %x\n", pRegCmd->module2Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "module 3 Config = %x\n", pRegCmd->module3Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "module 4 Config = %x\n", pRegCmd->module4Config);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "module 5 Config = %x\n", pRegCmd->module5Config);
    }
}

CAMX_NAMESPACE_END
