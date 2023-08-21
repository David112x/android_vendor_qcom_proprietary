////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpslsc40titan480.cpp
/// @brief CAMXBPSLSC40TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpslsc40titan480.h"
#include "camxiqinterface.h"
#include "titan480_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief LSC register Configuration
struct BPSLSC40RegConfig
{
    BPS_BPS_0_CLC_LSC_LSC_0_CFG     config0;    ///< Lens Rolloff config 0
    BPS_BPS_0_CLC_LSC_LSC_1_CFG     config1;    ///< Lens Rolloff config 1
    BPS_BPS_0_CLC_LSC_LSC_2_CFG     config2;    ///< Lens Rolloff config 2
    BPS_BPS_0_CLC_LSC_LSC_3_CFG     config3;    ///< Lens Rolloff config 3
    BPS_BPS_0_CLC_LSC_LSC_4_CFG     config4;    ///< Lens Rolloff config 4
    BPS_BPS_0_CLC_LSC_LSC_5_CFG     config5;    ///< Lens Rolloff config 5
    BPS_BPS_0_CLC_LSC_LSC_6_CFG     config6;    ///< Lens Rolloff config 6
    BPS_BPS_0_CLC_LSC_LSC_7_CFG     config7;    ///< Lens Rolloff config 7
    BPS_BPS_0_CLC_LSC_LSC_8_CFG     config8;    ///< Lens Rolloff config 8
    BPS_BPS_0_CLC_LSC_LSC_9_CFG     config9;    ///< Lens Rolloff config 9
    BPS_BPS_0_CLC_LSC_LSC_10_CFG    config10;   ///< Lens Rolloff config 10
} CAMX_PACKED;

struct BPSLSC40ModuleConfig
{
    BPS_BPS_0_CLC_LSC_MODULE_LUT_BANK_CFG       moduleLUTConfig; ///< Module LUT configuration
    BPS_BPS_0_CLC_LSC_MODULE_CFG                moduleConfig;    ///< Module configuration
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSLSC40RegLengthDword  = sizeof(BPSLSC40RegConfig) / sizeof(UINT32);
static const UINT32 BPSLSC40DMISetSizeDword = MESH_ROLLOFF40_SIZE;
static const UINT32 BPSLSC40LUTTableSize    = (BPSLSC40DMISetSizeDword * sizeof(UINT32));
static const UINT32 BPSLSC40DMILengthDword  = (BPSLSC40DMISetSizeDword * LSC40NumDMITables);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40Titan480::BPSLSC40Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLSC40Titan480::BPSLSC40Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40Titan480::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40Titan480::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSLSC40Titan480* pHWSetting = CAMX_NEW BPSLSC40Titan480;

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
/// BPSLSC40Titan480::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40Titan480::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pModuleConfig = CAMX_CALLOC(sizeof(BPSLSC40ModuleConfig));

    if (NULL == m_pModuleConfig)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for module configuration!");
    }
    else
    {
        m_pRegCmd = CAMX_CALLOC(sizeof(BPSLSC40RegConfig));

        if (NULL != m_pRegCmd)
        {
            SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSLSC40RegLengthDword));
            Set64bitDMILength(0);
            Set32bitDMILength(BPSLSC40DMILengthDword);
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
// BPSLSC40Titan480::~BPSLSC40Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLSC40Titan480::~BPSLSC40Titan480()
{
    CAMX_FREE(m_pModuleConfig);
    m_pModuleConfig = NULL;

    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40Titan480::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40Titan480::WriteLUTtoDMI(
    VOID* pInput)
{
    CamxResult      result      = CamxResultSuccess;
    ISPInputData*   pInputData  = static_cast<ISPInputData*>(pInput);
    CmdBuffer*      pDMIBuffer  = pInputData->pDMICmdBuffer;
    UINT32          LUTOffset;

    CAMX_ASSERT(NULL != pDMIBuffer);

    // CDM pack the DMI buffer and patch the Red and Blue LUT DMI buffer into CDM pack
    LUTOffset = 0;
    result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                     regBPS_BPS_0_CLC_LSC_DMI_CFG,
                                     BPS_BPS_0_CLC_LSC_DMI_LUT_CFG_LUT_SEL_RED_LUT,
                                     pInputData->p32bitDMIBuffer,
                                     LUTOffset,
                                     BPSLSC40LUTTableSize);
    if (CamxResultSuccess == result)
    {
        LUTOffset += BPSLSC40LUTTableSize;

        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_LSC_DMI_CFG,
                                         BPS_BPS_0_CLC_LSC_DMI_LUT_CFG_LUT_SEL_BLUE_LUT,
                                         pInputData->p32bitDMIBuffer,
                                         LUTOffset,
                                         BPSLSC40LUTTableSize);
        if (CamxResultSuccess == result)
        {
            LUTOffset += BPSLSC40LUTTableSize;

            result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                             regBPS_BPS_0_CLC_LSC_DMI_CFG,
                                             BPS_BPS_0_CLC_LSC_DMI_LUT_CFG_LUT_SEL_GRID_LUT,
                                             pInputData->p32bitDMIBuffer,
                                             LUTOffset,
                                             BPSLSC40LUTTableSize);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Write Grid LUT failed");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Write BLUE LUT failed");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Write RED LUT failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CamxResult    result     = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    result = WriteLUTtoDMI(pInputData);
    if ((CamxResultSuccess == result) && (NULL != pInputData->pCmdBuffer))
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_LSC_LSC_0_CFG,
                                              BPSLSC40RegLengthDword,
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
// BPSLSC40Titan480::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40Titan480::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    BPSLSC40ModuleConfig* pModuleCfg = static_cast<BPSLSC40ModuleConfig*>(m_pModuleConfig);
    BPSLSC40RegConfig*    pRegCmd    = static_cast<BPSLSC40RegConfig*>(m_pRegCmd);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    // post any LSC information in case down stream module requires
    // BPS configuration for FW
    pBPSIQSettings->lensRollOffParameters_480.moduleCfg.EN             = moduleEnable;

    pBPSIQSettings->lensRollOffParameters_480.moduleCfg.ALSC_EN        = pModuleCfg->moduleConfig.bitfields.ALSC_EN;
    pBPSIQSettings->lensRollOffParameters_480.moduleCfg.NUM_SUBBLOCKS  = pModuleCfg->moduleConfig.bitfields.NUM_SUBBLOCKS;

    pBPSIQSettings->lensRollOffParameters_480.subgridWidth             = pRegCmd->config2.bitfields.SUBBLOCK_WIDTH;
    pBPSIQSettings->lensRollOffParameters_480.meshGridBwidth           = pRegCmd->config1.bitfields.BLOCK_WIDTH;
    pBPSIQSettings->lensRollOffParameters_480.startBlockIndex          = pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_X;
    pBPSIQSettings->lensRollOffParameters_480.startSubgridIndex        = pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_X;
    pBPSIQSettings->lensRollOffParameters_480.topLeftCoordinate        = pRegCmd->config4.bitfields.INIT_PIXEL_X;
    pBPSIQSettings->lensRollOffParameters_480.numHorizontalMeshGains   = pRegCmd->config0.bitfields.NUM_BLOCKS_X;

    pBPSIQSettings->lensRollOffParameters_480.initBlockY               = pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_Y;
    pBPSIQSettings->lensRollOffParameters_480.initSubblockY            = pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_Y;
    pBPSIQSettings->lensRollOffParameters_480.initPixelY               = pRegCmd->config4.bitfields.INIT_PIXEL_Y;

    pInputData->pCalculatedData->lensShadingInfo.lensShadingMapSize.width  = LSC_MESH_PT_H_V40;
    pInputData->pCalculatedData->lensShadingInfo.lensShadingMapSize.height = LSC_MESH_PT_V_V40;

    if (FALSE == pModuleCfg->moduleConfig.bitfields.EN)
    {
        pInputData->pCalculatedData->lensShadingInfo.shadingMode    = ShadingModeOff;
    }
    else
    {
        pInputData->pCalculatedData->lensShadingInfo.shadingMode    = pInputData->pHALTagsData->shadingMode;
    }
    pInputData->pCalculatedData->lensShadingInfo.lensShadingMapMode = pInputData->pHALTagsData->statisticsLensShadingMapMode;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSLSC40RegConfig* pRegCmd            = static_cast<BPSLSC40RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSLSC40RegConfig) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSLSCData.rolloffConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata480.BPSLSCData.rolloffConfig,
                        pRegCmd,
                        sizeof(BPSLSC40RegConfig));

        if ((TRUE == pBPSIQSettings->lensRollOffParameters_480.moduleCfg.EN) &&
            (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSLSC40Register,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata480.BPSLSCData.rolloffConfig),
                &pBPSTuningMetadata->BPSTuningMetadata480.BPSLSCData.rolloffConfig,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSLSCData.rolloffConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC40Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult              result          = CamxResultSuccess;
    LSC40UnpackedField*     pData           = static_cast<LSC40UnpackedField*>(pInput);
    LSC40OutputData*        pOutputData     = static_cast<LSC40OutputData*>(pOutput);
    BPSLSC40RegConfig*      pRegCmd         = static_cast<BPSLSC40RegConfig*>(m_pRegCmd);
    BPSLSC40ModuleConfig*   pModuleCfg      = static_cast<BPSLSC40ModuleConfig*>(m_pModuleConfig);

    UINT32 totalHorMesh = 0;
    UINT32 totalVerMesh = 0;
    UINT32 dmiCount     = 0;
    UINT32 horMeshNum;
    UINT32 verMeshNum;

    if ((NULL != pOutputData) && (NULL != pData))
    {
        pModuleCfg->moduleConfig.bitfields.ALSC_EN          = pData->ALSC_enable;
        pModuleCfg->moduleConfig.bitfields.CROP_EN          = pData->crop_enable;

        pModuleCfg->moduleConfig.bitfields.NUM_SUBBLOCKS    = pData->intp_factor;
        pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_X     = pData->Lx_start;
        pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_Y     = pData->Ly_start;
        pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_X  = pData->Bx_start;
        pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_Y  = pData->By_start;

        pModuleCfg->moduleLUTConfig.bitfields.BANK_SEL      = pData->bank_sel;

        pRegCmd->config0.bitfields.NUM_BLOCKS_X             = pData->num_meshgain_h;
        pRegCmd->config0.bitfields.NUM_BLOCKS_Y             = pData->num_meshgain_v;
        pRegCmd->config1.bitfields.BLOCK_HEIGHT             = pData->MeshGridBheight;
        pRegCmd->config1.bitfields.BLOCK_WIDTH              = pData->MeshGridBwidth;
        pRegCmd->config2.bitfields.INV_SUBBLOCK_WIDTH       = pData->x_delta;
        pRegCmd->config2.bitfields.SUBBLOCK_WIDTH           = pData->Bwidth;
        pRegCmd->config3.bitfields.INV_SUBBLOCK_HEIGHT      = pData->y_delta;
        pRegCmd->config3.bitfields.SUBBLOCK_HEIGHT          = pData->Bheight;
        pRegCmd->config4.bitfields.INIT_PIXEL_X             = pData->Bx_d1;
        pRegCmd->config4.bitfields.INIT_PIXEL_Y             = pData->By_e1;
        pRegCmd->config5.bitfields.PIXEL_OFFSET             = pData->pixel_offset;
        pRegCmd->config6.bitfields.INIT_YDELTA              = pData->By_init_e1;
        pRegCmd->config7.bitfields.LUMA_WEIGHT_BASE_MIN     = pData->luma_weight_base_min;
        pRegCmd->config7.bitfields.LUMA_WEIGHT_BASE_SCALE   = pData->luma_weight_base_scale;
        pRegCmd->config8.bitfields.LUMA_WEIGHT_MIN          = pData->luma_weight_min;
        pRegCmd->config9.bitfields.LAST_PIXEL               = pData->last_pixel;
        pRegCmd->config9.bitfields.FIRST_PIXEL              = pData->first_pixel;
        pRegCmd->config10.bitfields.FIRST_LINE              = pData->first_line;
        pRegCmd->config10.bitfields.LAST_LINE               = pData->last_line;

        // (NUM_MESHGAIN_H+2) * (NUM_MESHGAIN_V+2) LUT entries for a frame.
        totalHorMesh = pData->num_meshgain_h + 2;
        totalVerMesh = pData->num_meshgain_v + 2;

        if (totalHorMesh > LSC_MESH_PT_H_V40 || totalVerMesh > LSC_MESH_PT_V_V40)
        {
            CAMX_LOG_WARN(CamxLogGroupPProc, "LUT table size is overlimit, need to truncate it");
            totalHorMesh = LSC_MESH_PT_H_V40;
            totalVerMesh = LSC_MESH_PT_V_V40;
        }

        for (verMeshNum = 0; verMeshNum < totalVerMesh; verMeshNum++)
        {
            for (horMeshNum = 0; horMeshNum < totalHorMesh; horMeshNum++)
            {
                pOutputData->pGRRLUTDMIBuffer[dmiCount] =
                    ((pData->mesh_table[pData->bank_sel][0][verMeshNum][horMeshNum] & Utils::AllOnes32(13)) |
                    (((pData->mesh_table[pData->bank_sel][1][verMeshNum][horMeshNum]) & Utils::AllOnes32(13)) << 13));

                pOutputData->pGBBLUTDMIBuffer[dmiCount] =
                    ((pData->mesh_table[pData->bank_sel][3][verMeshNum][horMeshNum] & Utils::AllOnes32(13)) |
                    (((pData->mesh_table[pData->bank_sel][2][verMeshNum][horMeshNum]) & Utils::AllOnes32(13)) << 13));

                pOutputData->pGridLUTDMIBuffer[dmiCount] =
                    ((pData->grids_gain[pData->bank_sel][verMeshNum][horMeshNum] & Utils::AllOnes32(12)) |
                    (((pData->grids_mean[pData->bank_sel][verMeshNum][horMeshNum]) & Utils::AllOnes32(14)) << 12));

                dmiCount++;
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Data is NULL %p %p", pOutputData, pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC40Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLSC40Titan480::DumpRegConfig()
{
    BPSLSC40RegConfig* pRegCmd = static_cast<BPSLSC40RegConfig*>(m_pRegCmd);

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "============== BPS LSC40 ==================");
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 0 = %x\n", pRegCmd->config0);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 1 = %x\n", pRegCmd->config1);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 2 = %x\n", pRegCmd->config2);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 3 = %x\n", pRegCmd->config3);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 4 = %x\n", pRegCmd->config4);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 5 = %x\n", pRegCmd->config5);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 6 = %x\n", pRegCmd->config6);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 7 = %x\n", pRegCmd->config7);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 8 = %x\n", pRegCmd->config8);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 9 = %x\n", pRegCmd->config9);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 10 = %x\n", pRegCmd->config10);
}


CAMX_NAMESPACE_END
