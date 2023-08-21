////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpslsc34titan17x.cpp
/// @brief CAMXBPSLSC34TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpslsc34titan17x.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief LSC register Configuration
struct BPSLSC34RegConfig
{
    BPS_BPS_0_CLC_LENS_ROLLOFF_LENS_ROLLOFF_0_CFG   config0;    ///< Lens Rolloff config 0
    BPS_BPS_0_CLC_LENS_ROLLOFF_LENS_ROLLOFF_1_CFG   config1;    ///< Lens Rolloff config 1
    BPS_BPS_0_CLC_LENS_ROLLOFF_LENS_ROLLOFF_2_CFG   config2;    ///< Lens Rolloff config 2
    BPS_BPS_0_CLC_LENS_ROLLOFF_LENS_ROLLOFF_3_CFG   config3;    ///< Lens Rolloff config 3
    BPS_BPS_0_CLC_LENS_ROLLOFF_LENS_ROLLOFF_4_CFG   config4;    ///< Lens Rolloff config 4
    BPS_BPS_0_CLC_LENS_ROLLOFF_LENS_ROLLOFF_5_CFG   config5;    ///< Lens Rolloff config 5
    BPS_BPS_0_CLC_LENS_ROLLOFF_LENS_ROLLOFF_6_CFG   config6;    ///< Lens Rolloff config 6
} CAMX_PACKED;

/// @brief LSC register Configuration
struct BPSLSC34ModuleConfig
{
    BPS_BPS_0_CLC_LENS_ROLLOFF_MODULE_CFG  moduleConfig;    ///< Module configuration
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 RedLUT                  = BPS_BPS_0_CLC_LENS_ROLLOFF_DMI_LUT_CFG_LUT_SEL_RED_LUT;
static const UINT32 BlueLUT                 = BPS_BPS_0_CLC_LENS_ROLLOFF_DMI_LUT_CFG_LUT_SEL_BLUE_LUT;

static const UINT32 BPSLSC34RegLengthDWord  = sizeof(BPSLSC34RegConfig) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34Titan17x::BPSLSC34Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLSC34Titan17x::BPSLSC34Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSLSC34Titan17x* pHWSetting = CAMX_NEW BPSLSC34Titan17x;

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
/// BPSLSC34Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pModuleConfig = CAMX_CALLOC(sizeof(BPSLSC34ModuleConfig));

    if (NULL == m_pModuleConfig)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for module configuration!");
    }
    else
    {
        m_pRegCmd = CAMX_CALLOC(sizeof(BPSLSC34RegConfig));

        if (NULL != m_pRegCmd)
        {
            SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSLSC34RegLengthDWord));
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
// BPSLSC34Titan17x::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34Titan17x::WriteLUTtoDMI(
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
                                     regBPS_BPS_0_CLC_LENS_ROLLOFF_DMI_CFG,
                                     RedLUT,
                                     pInputData->p64bitDMIBuffer,
                                     LUTOffset,
                                     BPSLSC34LUTTableSize);
    CAMX_ASSERT(CamxResultSuccess == result);

    LUTOffset += BPSLSC34LUTTableSize;

    result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                     regBPS_BPS_0_CLC_LENS_ROLLOFF_DMI_CFG,
                                     BlueLUT,
                                     pInputData->p64bitDMIBuffer,
                                     LUTOffset,
                                     BPSLSC34LUTTableSize);
    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34Titan17x::CreateCmdList(
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
                                              regBPS_BPS_0_CLC_LENS_ROLLOFF_LENS_ROLLOFF_0_CFG,
                                              BPSLSC34RegLengthDWord,
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
// BPSLSC34Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    BPSLSC34RegConfig*    pRegCmd    = static_cast<BPSLSC34RegConfig*>(m_pRegCmd);
    BPSLSC34ModuleConfig* pModuleCfg = static_cast<BPSLSC34ModuleConfig*>(m_pModuleConfig);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->lensRollOffParameters.moduleCfg.EN                     = moduleEnable;

    pBPSIQSettings->lensRollOffParameters.moduleCfg.NUM_SUBBLOCKS          = pModuleCfg->moduleConfig.bitfields.NUM_SUBBLOCKS;
    pBPSIQSettings->lensRollOffParameters.subgridWidth                     = pRegCmd->config2.bitfields.SUBBLOCK_WIDTH;
    pBPSIQSettings->lensRollOffParameters.meshGridBwidth                   = pRegCmd->config1.bitfields.BLOCK_WIDTH;
    pBPSIQSettings->lensRollOffParameters.startBlockIndex                  = pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_X;
    pBPSIQSettings->lensRollOffParameters.startSubgridIndex                = pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_X;
    pBPSIQSettings->lensRollOffParameters.topLeftCoordinate                = pRegCmd->config4.bitfields.INIT_PIXEL_X;
    pBPSIQSettings->lensRollOffParameters.numHorizontalMeshGains           = pRegCmd->config0.bitfields.NUM_BLOCKS_X;
    pBPSIQSettings->lensRollOffParameters.initBlockY                       = pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_Y;
    pBPSIQSettings->lensRollOffParameters.initSubblockY                    = pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_Y;
    pBPSIQSettings->lensRollOffParameters.initPixelY                       = pRegCmd->config4.bitfields.INIT_PIXEL_Y;

    pInputData->pCalculatedData->lensShadingInfo.lensShadingMapSize.width  = BPSRolloffMeshPtHV34;
    pInputData->pCalculatedData->lensShadingInfo.lensShadingMapSize.height = BPSRolloffMeshPtVV34;

    if (NULL != pInputData->pOEMIQSetting)
    {
        pInputData->pCalculatedData->lensShadingInfo.shadingMode           = ShadingModeFast;
        pInputData->pCalculatedData->lensShadingInfo.lensShadingMapMode    = StatisticsLensShadingMapModeOff;
    }
    else
    {
        if (FALSE == moduleEnable)
        {
            pInputData->pCalculatedData->lensShadingInfo.shadingMode    = ShadingModeOff;
        }
        else
        {
            pInputData->pCalculatedData->lensShadingInfo.shadingMode    = pInputData->pHALTagsData->shadingMode;
        }
        pInputData->pCalculatedData->lensShadingInfo.lensShadingMapMode =
            pInputData->pHALTagsData->statisticsLensShadingMapMode;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSLSC34RegConfig* pRegCmd            = static_cast<BPSLSC34RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSLSC34RegConfig) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSLSCData.rolloffConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSLSCData.rolloffConfig,
                        pRegCmd,
                        sizeof(BPSLSC34RegConfig));

        if (TRUE == pBPSIQSettings->lensRollOffParameters.moduleCfg.EN)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSLSC34Register,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata17x.BPSLSCData.rolloffConfig),
                &pBPSTuningMetadata->BPSTuningMetadata17x.BPSLSCData.rolloffConfig,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSLSCData.rolloffConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSLSC34Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result      = CamxResultSuccess;
    LSC34UnpackedField* pData       = static_cast<LSC34UnpackedField*>(pInput);
    LSC34OutputData*    pOutputData = static_cast<LSC34OutputData*>(pOutput);

    BPSLSC34RegConfig*    pRegCmd    = static_cast<BPSLSC34RegConfig*>(m_pRegCmd);
    BPSLSC34ModuleConfig* pModuleCfg = static_cast<BPSLSC34ModuleConfig*>(m_pModuleConfig);

    UINT32 totalHorMesh = 0;
    UINT32 toatlVerMesh = 0;
    UINT32 dmiCount = 0;
    UINT32 horMeshNum;
    UINT32 verMeshNum;

    if ((NULL != pOutputData) && (NULL != pData))
    {
        pModuleCfg->moduleConfig.bitfields.NUM_SUBBLOCKS            = pData->intp_factor_l;
        pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_X             = pData->lx_start_l;
        pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_X          = pData->bx_start_l;
        pModuleCfg->moduleConfig.bitfields.INIT_BLOCK_Y             = pData->ly_start_l;
        pModuleCfg->moduleConfig.bitfields.INIT_SUBBLOCK_Y          = pData->by_start_l;

        pRegCmd->config0.bitfields.NUM_BLOCKS_X                     = pData->num_meshgain_h;
        pRegCmd->config0.bitfields.NUM_BLOCKS_Y                     = pData->num_meshgain_v;
        pRegCmd->config1.bitfields.BLOCK_HEIGHT                     = pData->meshGridBheight_l;
        pRegCmd->config1.bitfields.BLOCK_WIDTH                      = pData->meshGridBwidth_l;
        pRegCmd->config2.bitfields.INV_SUBBLOCK_WIDTH               = pData->x_delta_l;
        pRegCmd->config2.bitfields.SUBBLOCK_WIDTH                   = pData->bwidth_l;
        pRegCmd->config3.bitfields.INV_SUBBLOCK_HEIGHT              = pData->y_delta_l;
        pRegCmd->config3.bitfields.SUBBLOCK_HEIGHT                  = pData->bheight_l;
        pRegCmd->config4.bitfields.INIT_PIXEL_X                     = pData->bx_d1_l;
        pRegCmd->config4.bitfields.INIT_PIXEL_Y                     = pData->by_e1_l;
        pRegCmd->config5.bitfields.PIXEL_OFFSET                     = pData->pixel_offset;
        pRegCmd->config6.bitfields.INIT_YDELTA                      = pData->by_init_e1_l;


        // (NUM_MESHGAIN_H+2) * (NUM_MESHGAIN_V+2) LUT entries for a frame.
        totalHorMesh = pData->num_meshgain_h + 2;
        toatlVerMesh = pData->num_meshgain_v + 2;

        for (verMeshNum = 0; verMeshNum < toatlVerMesh; verMeshNum++)
        {
            for (horMeshNum = 0; horMeshNum < totalHorMesh; horMeshNum++)
            {
                pOutputData->pGRRLUTDMIBuffer[dmiCount] =
                    ((pData->mesh_table_l[pData->bank_sel][0][verMeshNum][horMeshNum] & Utils::AllOnes32(13)) |
                    ((pData->mesh_table_l[pData->bank_sel][1][verMeshNum][horMeshNum]) << 13));

                pOutputData->pGBBLUTDMIBuffer[dmiCount] =
                    ((pData->mesh_table_l[pData->bank_sel][3][verMeshNum][horMeshNum] & Utils::AllOnes32(13)) |
                    ((pData->mesh_table_l[pData->bank_sel][2][verMeshNum][horMeshNum]) << 13));
                dmiCount++;
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Data is NULL %p %p", pOutputData, pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34Titan17x::~BPSLSC34Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSLSC34Titan17x::~BPSLSC34Titan17x()
{
    CAMX_FREE(m_pModuleConfig);
    m_pModuleConfig = NULL;

    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSLSC34Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSLSC34Titan17x::DumpRegConfig()
{
    BPSLSC34RegConfig* pRegCmd = static_cast<BPSLSC34RegConfig*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 0 = %x\n", pRegCmd->config0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 1 = %x\n", pRegCmd->config1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 2 = %x\n", pRegCmd->config2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 3 = %x\n", pRegCmd->config3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 4 = %x\n", pRegCmd->config4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 5 = %x\n", pRegCmd->config5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Config 6 = %x\n", pRegCmd->config6);
    }
}


CAMX_NAMESPACE_END