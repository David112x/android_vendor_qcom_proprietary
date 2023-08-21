////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsabf40titan17x.cpp
/// @brief CAMXBPSABF40TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpsabf40titan17x.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief ABF register Configuration
struct BPSABF40RegConfig
{
    BPS_BPS_0_CLC_ABF_ABF_0_CFG   config0;      ///< ABF Config 0
    BPS_BPS_0_CLC_ABF_ABF_1_CFG   config1;      ///< ABF Config 1
    BPS_BPS_0_CLC_ABF_ABF_2_CFG   config2;      ///< ABF Config 2
    BPS_BPS_0_CLC_ABF_ABF_3_CFG   config3;      ///< ABF Config 3
    BPS_BPS_0_CLC_ABF_ABF_4_CFG   config4;      ///< ABF Config 4
    BPS_BPS_0_CLC_ABF_ABF_5_CFG   config5;      ///< ABF Config 5
    BPS_BPS_0_CLC_ABF_ABF_6_CFG   config6;      ///< ABF Config 6
    BPS_BPS_0_CLC_ABF_ABF_7_CFG   config7;      ///< ABF Config 7
    BPS_BPS_0_CLC_ABF_ABF_8_CFG   config8;      ///< ABF Config 8
    BPS_BPS_0_CLC_ABF_ABF_9_CFG   config9;      ///< ABF Config 9
    BPS_BPS_0_CLC_ABF_ABF_10_CFG  config10;     ///< ABF Config 10
    BPS_BPS_0_CLC_ABF_ABF_11_CFG  config11;     ///< ABF Config 11
    BPS_BPS_0_CLC_ABF_ABF_12_CFG  config12;     ///< ABF Config 12
    BPS_BPS_0_CLC_ABF_ABF_13_CFG  config13;     ///< ABF Config 13
    BPS_BPS_0_CLC_ABF_ABF_14_CFG  config14;     ///< ABF Config 14
    BPS_BPS_0_CLC_ABF_ABF_15_CFG  config15;     ///< ABF Config 15
    BPS_BPS_0_CLC_ABF_ABF_16_CFG  config16;     ///< ABF Config 16
    BPS_BPS_0_CLC_ABF_ABF_17_CFG  config17;     ///< ABF Config 17
    BPS_BPS_0_CLC_ABF_ABF_18_CFG  config18;     ///< ABF Config 18
    BPS_BPS_0_CLC_ABF_ABF_19_CFG  config19;     ///< ABF Config 19
    BPS_BPS_0_CLC_ABF_ABF_20_CFG  config20;     ///< ABF Config 20
    BPS_BPS_0_CLC_ABF_ABF_21_CFG  config21;     ///< ABF Config 21
    BPS_BPS_0_CLC_ABF_ABF_22_CFG  config22;     ///< ABF Config 22
    BPS_BPS_0_CLC_ABF_ABF_23_CFG  config23;     ///< ABF Config 23
    BPS_BPS_0_CLC_ABF_ABF_24_CFG  config24;     ///< ABF Config 24
    BPS_BPS_0_CLC_ABF_ABF_25_CFG  config25;     ///< ABF Config 25
    BPS_BPS_0_CLC_ABF_ABF_26_CFG  config26;     ///< ABF Config 26
    BPS_BPS_0_CLC_ABF_ABF_27_CFG  config27;     ///< ABF Config 27
    BPS_BPS_0_CLC_ABF_ABF_28_CFG  config28;     ///< ABF Config 28
    BPS_BPS_0_CLC_ABF_ABF_29_CFG  config29;     ///< ABF Config 29
    BPS_BPS_0_CLC_ABF_ABF_30_CFG  config30;     ///< ABF Config 30
    BPS_BPS_0_CLC_ABF_ABF_31_CFG  config31;     ///< ABF Config 31
    BPS_BPS_0_CLC_ABF_ABF_32_CFG  config32;     ///< ABF Config 32
    BPS_BPS_0_CLC_ABF_ABF_33_CFG  config33;     ///< ABF Config 33
    BPS_BPS_0_CLC_ABF_ABF_34_CFG  config34;     ///< ABF Config 34
    BPS_BPS_0_CLC_ABF_ABF_35_CFG  config35;     ///< ABF Config 35
    BPS_BPS_0_CLC_ABF_ABF_36_CFG  config36;     ///< ABF Config 36
    BPS_BPS_0_CLC_ABF_ABF_37_CFG  config37;     ///< ABF Config 37
    BPS_BPS_0_CLC_ABF_ABF_38_CFG  config38;     ///< ABF Config 38
    BPS_BPS_0_CLC_ABF_ABF_39_CFG  config39;     ///< ABF Config 39
    BPS_BPS_0_CLC_ABF_ABF_40_CFG  config40;     ///< ABF Config 40
    BPS_BPS_0_CLC_ABF_ABF_41_CFG  config41;     ///< ABF Config 41
    BPS_BPS_0_CLC_ABF_ABF_42_CFG  config42;     ///< ABF Config 42
    BPS_BPS_0_CLC_ABF_ABF_43_CFG  config43;     ///< ABF Config 43
    BPS_BPS_0_CLC_ABF_ABF_44_CFG  config44;     ///< ABF Config 44
    BPS_BPS_0_CLC_ABF_ABF_45_CFG  config45;     ///< ABF Config 45
} CAMX_PACKED;

/// @brief ABF Module Configuration
struct BPSABF40ModuleConfig
{
    BPS_BPS_0_CLC_ABF_MODULE_CFG  moduleConfig;  ///< Module configuration
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 NoiseLUT0   = BPS_BPS_0_CLC_ABF_DMI_LUT_CFG_LUT_SEL_NOISE_LUT0;
static const UINT32 NoiseLUT1   = BPS_BPS_0_CLC_ABF_DMI_LUT_CFG_LUT_SEL_NOISE_LUT1;
static const UINT32 ActivityLUT = BPS_BPS_0_CLC_ABF_DMI_LUT_CFG_LUT_SEL_ACTIVITY_LUT;
static const UINT32 DarkLUT     = BPS_BPS_0_CLC_ABF_DMI_LUT_CFG_LUT_SEL_DARK_LUT;
static const UINT32 ABFLUTBank0 = BPS_BPS_0_CLC_ABF_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT32 ABFLUTBank1 = BPS_BPS_0_CLC_ABF_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK1;

static const UINT32 BPSABF40RegConfigLengthDWord = sizeof(BPSABF40RegConfig) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40Titan17x::BPSABF40Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSABF40Titan17x::BPSABF40Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSABF40Titan17x* pHWSetting = CAMX_NEW BPSABF40Titan17x;

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
/// BPSABF40Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pModuleConfig = CAMX_CALLOC(sizeof(BPSABF40ModuleConfig));

    if (NULL == m_pModuleConfig)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for module configuration!");
    }
    else
    {
        m_pRegCmd = CAMX_CALLOC(sizeof(BPSABF40RegConfig));

        if (NULL != m_pRegCmd)
        {
            SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSABF40RegConfigLengthDWord));
            Set32bitDMILength(((BPSABF40NoiseLUTSizeDword * 2) + BPSABF40ActivityLUTSizeDword + BPSABF40DarkLUTSizeDword));
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
// BPSABF40Titan17x::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40Titan17x::WriteLUTtoDMI(
    VOID*   pInput)
{
    CamxResult      result      = CamxResultSuccess;
    ISPInputData*   pInputData  = static_cast<ISPInputData*>(pInput);

    // CDM pack the DMI buffer and patch the Red and Blue LUT DMI buffer into CDM pack
    if ((NULL != pInputData) && (NULL != pInputData->pDMICmdBuffer))
    {
        UINT32 LUTOffset = 0;
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                        regBPS_BPS_0_CLC_ABF_DMI_CFG,
                                        NoiseLUT0,
                                        pInputData->p64bitDMIBuffer,
                                        LUTOffset,
                                        BPSABF40NoiseLUTSize);
        if (CamxResultSuccess == result)
        {
            LUTOffset += BPSABF40NoiseLUTSize;
            result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                            regBPS_BPS_0_CLC_ABF_DMI_CFG,
                                            NoiseLUT1,
                                            pInputData->p64bitDMIBuffer,
                                            LUTOffset,
                                            BPSABF40NoiseLUTSize);
        }

        if (CamxResultSuccess == result)
        {
            LUTOffset += BPSABF40NoiseLUTSize;
            result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                            regBPS_BPS_0_CLC_ABF_DMI_CFG,
                                            ActivityLUT,
                                            pInputData->p64bitDMIBuffer,
                                            LUTOffset,
                                            BPSABF40ActivityLUTSize);
        }

        if (CamxResultSuccess == result)
        {
            LUTOffset += BPSABF40ActivityLUTSize;
            result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                            regBPS_BPS_0_CLC_ABF_DMI_CFG,
                                            DarkLUT,
                                            pInputData->p64bitDMIBuffer,
                                            LUTOffset,
                                            BPSABF40DarkLUTSize);
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
// BPSABF40Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CamxResult         result     = CamxResultSuccess;
    ISPInputData*      pInputData = static_cast<ISPInputData*>(pSettingData);

    result = WriteLUTtoDMI(pInputData);

    if ((CamxResultSuccess == result) && (NULL != pInputData->pCmdBuffer))
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_ABF_ABF_0_CFG,
                                              BPSABF40RegConfigLengthDWord,
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
// BPSABF40Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    BPSABF40RegConfig*    pRegCmd    = static_cast<BPSABF40RegConfig*>(m_pRegCmd);
    BPSABF40ModuleConfig* pModuleCfg = static_cast<BPSABF40ModuleConfig*>(m_pModuleConfig);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->abfParameters.moduleCfg.EN                     = moduleEnable;

    pBPSIQSettings->abfParameters.moduleCfg.ACT_ADJ_EN             = pModuleCfg->moduleConfig.bitfields.ACT_ADJ_EN;
    pBPSIQSettings->abfParameters.moduleCfg.DARK_SMOOTH_EN         = pModuleCfg->moduleConfig.bitfields.DARK_SMOOTH_EN;
    pBPSIQSettings->abfParameters.moduleCfg.DARK_DESAT_EN          = pModuleCfg->moduleConfig.bitfields.DARK_DESAT_EN;
    pBPSIQSettings->abfParameters.moduleCfg.DIR_SMOOTH_EN          = pModuleCfg->moduleConfig.bitfields.DIR_SMOOTH_EN;
    pBPSIQSettings->abfParameters.moduleCfg.MINMAX_EN              = pModuleCfg->moduleConfig.bitfields.MINMAX_EN;
    pBPSIQSettings->abfParameters.moduleCfg.CROSS_PLANE_EN         = pModuleCfg->moduleConfig.bitfields.CROSS_PLANE_EN;
    pBPSIQSettings->abfParameters.moduleCfg.BLS_EN                 = pModuleCfg->moduleConfig.bitfields.BLS_EN;
    pBPSIQSettings->abfParameters.moduleCfg.PIX_MATCH_LEVEL_RB     = pModuleCfg->moduleConfig.bitfields.PIX_MATCH_LEVEL_RB;
    pBPSIQSettings->abfParameters.moduleCfg.PIX_MATCH_LEVEL_G      = pModuleCfg->moduleConfig.bitfields.PIX_MATCH_LEVEL_G;
    pBPSIQSettings->abfParameters.moduleCfg.BLOCK_MATCH_PATTERN_RB = pModuleCfg->moduleConfig.bitfields.BLOCK_MATCH_PATTERN_RB;

    if (NULL != pInputData->pOEMIQSetting)
    {
        pInputData->pCalculatedData->noiseReductionMode = NoiseReductionModeFast;
    }
    else
    {
        pInputData->pCalculatedData->noiseReductionMode = pInputData->pHALTagsData->noiseReductionMode;
    }

    if (NoiseReductionModeOff == pInputData->pCalculatedData->noiseReductionMode)
    {
        pBPSIQSettings->abfParameters.moduleCfg.FILTER_EN = 0;
    }
    else
    {
        pBPSIQSettings->abfParameters.moduleCfg.FILTER_EN = pModuleCfg->moduleConfig.bitfields.FILTER_EN;
    }

    pInputData->pCalculatedMetadata->BLSblackLevelOffset = pRegCmd->config44.bitfields.BLS_OFFSET;
    pInputData->triggerData.blackLevelOffset             = pRegCmd->config44.bitfields.BLS_OFFSET;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSABF40RegConfig* pRegCmd            = static_cast<BPSABF40RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSABF40RegConfig) <= sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSABFData.config));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSABFData.config, pRegCmd, sizeof(BPSABF40RegConfig));

        if (TRUE == pBPSIQSettings->abfParameters.moduleCfg.EN)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSABFRegister,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata17x.BPSABFData.config),
                &pBPSTuningMetadata->BPSTuningMetadata17x.BPSABFData.config,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSABFData.config));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSABF40Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult               result  = CamxResultSuccess;
    ABF40BLS12UnpackedField* pData   = static_cast<ABF40BLS12UnpackedField*>(pInput);

    BPSABF40RegConfig*    pRegCmd    = static_cast<BPSABF40RegConfig*>(m_pRegCmd);
    BPSABF40ModuleConfig* pModuleCfg = static_cast<BPSABF40ModuleConfig*>(m_pModuleConfig);

    ABF40UnpackedField* pDataABF = pData->pUnpackedRegisterDataABF;
    BLS12UnpackedField* pDataBLS = pData->pUnpackedRegisterDataBLS;
    ABF40OutputData*    pCmd     = static_cast<ABF40OutputData*>(pOutput);

    if ((NULL != pDataABF) && (NULL != pDataBLS) && (NULL != pCmd))
    {
        pRegCmd->config0.bitfields.EDGE_SOFTNESS_GR       = pDataABF->edgeSoftness[1]; ///< [4] =>[R,GR,GB,B]
        pRegCmd->config0.bitfields.EDGE_SOFTNESS_GB       = pDataABF->edgeSoftness[2];
        pRegCmd->config1.bitfields.EDGE_SOFTNESS_R        = pDataABF->edgeSoftness[0];
        pRegCmd->config1.bitfields.EDGE_SOFTNESS_B        = pDataABF->edgeSoftness[3];
        pRegCmd->config2.bitfields.DISTANCE_LEVEL0_GRGB_0 = pDataABF->distanceLevel[0][3];
        pRegCmd->config2.bitfields.DISTANCE_LEVEL0_GRGB_1 = pDataABF->distanceLevel[0][4];
        pRegCmd->config2.bitfields.DISTANCE_LEVEL0_GRGB_2 = pDataABF->distanceLevel[0][5];
        pRegCmd->config2.bitfields.DISTANCE_LEVEL1_GRGB_0 = pDataABF->distanceLevel[1][3];
        pRegCmd->config2.bitfields.DISTANCE_LEVEL1_GRGB_1 = pDataABF->distanceLevel[1][4];
        pRegCmd->config2.bitfields.DISTANCE_LEVEL1_GRGB_2 = pDataABF->distanceLevel[1][5];
        pRegCmd->config2.bitfields.DISTANCE_LEVEL2_GRGB_0 = pDataABF->distanceLevel[2][3];
        pRegCmd->config2.bitfields.DISTANCE_LEVEL2_GRGB_1 = pDataABF->distanceLevel[2][4];
        pRegCmd->config2.bitfields.DISTANCE_LEVEL2_GRGB_2 = pDataABF->distanceLevel[2][5];
        pRegCmd->config3.bitfields.DISTANCE_LEVEL0_RB_0   = pDataABF->distanceLevel[0][0];
        pRegCmd->config3.bitfields.DISTANCE_LEVEL0_RB_1   = pDataABF->distanceLevel[0][1];
        pRegCmd->config3.bitfields.DISTANCE_LEVEL0_RB_2   = pDataABF->distanceLevel[0][2];
        pRegCmd->config3.bitfields.DISTANCE_LEVEL1_RB_0   = pDataABF->distanceLevel[1][0];
        pRegCmd->config3.bitfields.DISTANCE_LEVEL1_RB_1   = pDataABF->distanceLevel[1][1];
        pRegCmd->config3.bitfields.DISTANCE_LEVEL1_RB_2   = pDataABF->distanceLevel[1][2];
        pRegCmd->config3.bitfields.DISTANCE_LEVEL2_RB_0   = pDataABF->distanceLevel[2][0];
        pRegCmd->config3.bitfields.DISTANCE_LEVEL2_RB_1   = pDataABF->distanceLevel[2][1];
        pRegCmd->config3.bitfields.DISTANCE_LEVEL2_RB_2   = pDataABF->distanceLevel[2][2];

        ///< curve offse[4] =>[R,GR,GB,B]
        pRegCmd->config4.bitfields.CURVE_OFFSET_R               = pDataABF->curveOffset[0];
        pRegCmd->config4.bitfields.CURVE_OFFSET_GR              = pDataABF->curveOffset[1];
        pRegCmd->config4.bitfields.CURVE_OFFSET_GB              = pDataABF->curveOffset[2];
        pRegCmd->config4.bitfields.CURVE_OFFSET_B               = pDataABF->curveOffset[3];
        pRegCmd->config5.bitfields.FILTER_STRENGTH_R            = pDataABF->filterStrength[0];
        pRegCmd->config5.bitfields.FILTER_STRENGTH_GR           = pDataABF->filterStrength[1];
        pRegCmd->config6.bitfields.FILTER_STRENGTH_GB           = pDataABF->filterStrength[2];
        pRegCmd->config6.bitfields.FILTER_STRENGTH_B            = pDataABF->filterStrength[3];
        pRegCmd->config7.bitfields.MINMAX_BLS                   = pDataABF->minmaxBLS;
        pRegCmd->config7.bitfields.MINMAX_MAX_SHIFT             = pDataABF->minmaxMaxShift;
        pRegCmd->config7.bitfields.MINMAX_MIN_SHIFT             = pDataABF->minmaxMinShift;
        pRegCmd->config7.bitfields.MINMAX_OFFSET                = pDataABF->minmaxOffset;
        pRegCmd->config8.bitfields.RNR_BX                       = pDataABF->bx;
        pRegCmd->config8.bitfields.RNR_BY                       = pDataABF->by;
        pRegCmd->config9.bitfields.RNR_INIT_RSQUARE             = pDataABF->rSquareInit;
        pRegCmd->config10.bitfields.RNR_RSQUARE_SCALE           = pDataABF->rSquareScale;
        pRegCmd->config10.bitfields.RNR_RSQUARE_SHIFT           = pDataABF->rSquareShift;
        pRegCmd->config11.bitfields.RNR_ANCHOR_0                = pDataABF->RNRAnchor[0];
        pRegCmd->config11.bitfields.RNR_ANCHOR_1                = pDataABF->RNRAnchor[1];
        pRegCmd->config12.bitfields.RNR_ANCHOR_2                = pDataABF->RNRAnchor[2];
        pRegCmd->config12.bitfields.RNR_ANCHOR_3                = pDataABF->RNRAnchor[3];
        pRegCmd->config13.bitfields.RNR_NOISE_BASE_0            = pDataABF->RNRBase0[0];
        pRegCmd->config13.bitfields.RNR_NOISE_BASE_1            = pDataABF->RNRBase0[1];
        pRegCmd->config14.bitfields.RNR_NOISE_BASE_2            = pDataABF->RNRBase0[2];
        pRegCmd->config14.bitfields.RNR_NOISE_BASE_3            = pDataABF->RNRBase0[3];
        pRegCmd->config15.bitfields.RNR_NOISE_SLOPE_0           = pDataABF->RNRSlope0[0];
        pRegCmd->config15.bitfields.RNR_NOISE_SLOPE_1           = pDataABF->RNRSlope0[1];
        pRegCmd->config16.bitfields.RNR_NOISE_SLOPE_2           = pDataABF->RNRSlope0[2];
        pRegCmd->config16.bitfields.RNR_NOISE_SLOPE_3           = pDataABF->RNRSlope0[3];
        pRegCmd->config17.bitfields.RNR_NOISE_SHIFT_0           = pDataABF->RNRShift0[0];
        pRegCmd->config17.bitfields.RNR_NOISE_SHIFT_1           = pDataABF->RNRShift0[1];
        pRegCmd->config17.bitfields.RNR_NOISE_SHIFT_2           = pDataABF->RNRShift0[2];
        pRegCmd->config17.bitfields.RNR_NOISE_SHIFT_3           = pDataABF->RNRShift0[3];
        pRegCmd->config17.bitfields.RNR_THRESH_SHIFT_0          = pDataABF->RNRShift1[0];
        pRegCmd->config17.bitfields.RNR_THRESH_SHIFT_1          = pDataABF->RNRShift1[1];
        pRegCmd->config17.bitfields.RNR_THRESH_SHIFT_2          = pDataABF->RNRShift1[2];
        pRegCmd->config17.bitfields.RNR_THRESH_SHIFT_3          = pDataABF->RNRShift1[3];
        pRegCmd->config18.bitfields.RNR_THRESH_BASE_0           = pDataABF->RNRBase1[0];
        pRegCmd->config18.bitfields.RNR_THRESH_BASE_1           = pDataABF->RNRBase1[1];
        pRegCmd->config19.bitfields.RNR_THRESH_BASE_2           = pDataABF->RNRBase1[2];
        pRegCmd->config19.bitfields.RNR_THRESH_BASE_3           = pDataABF->RNRBase1[3];
        pRegCmd->config20.bitfields.RNR_THRESH_SLOPE_0          = pDataABF->RNRSlope1[0];
        pRegCmd->config20.bitfields.RNR_THRESH_SLOPE_1          = pDataABF->RNRSlope1[1];
        pRegCmd->config21.bitfields.RNR_THRESH_SLOPE_2          = pDataABF->RNRSlope1[2];
        pRegCmd->config21.bitfields.RNR_THRESH_SLOPE_3          = pDataABF->RNRSlope1[3];
        pRegCmd->config22.bitfields.NP_ANCHOR_0                 = pDataABF->nprsvAnchor[0];
        pRegCmd->config22.bitfields.NP_ANCHOR_1                 = pDataABF->nprsvAnchor[1];
        pRegCmd->config23.bitfields.NP_ANCHOR_2                 = pDataABF->nprsvAnchor[2];
        pRegCmd->config23.bitfields.NP_ANCHOR_3                 = pDataABF->nprsvAnchor[3];
        pRegCmd->config24.bitfields.NP_BASE_RB_0                = pDataABF->nprsvBase[0][0];
        pRegCmd->config24.bitfields.NP_BASE_RB_1                = pDataABF->nprsvBase[0][1];
        pRegCmd->config25.bitfields.NP_BASE_RB_2                = pDataABF->nprsvBase[0][2];
        pRegCmd->config25.bitfields.NP_BASE_RB_3                = pDataABF->nprsvBase[0][3];
        pRegCmd->config26.bitfields.NP_SLOPE_RB_0               = pDataABF->nprsvSlope[0][0];
        pRegCmd->config26.bitfields.NP_SLOPE_RB_1               = pDataABF->nprsvSlope[0][1];
        pRegCmd->config27.bitfields.NP_SLOPE_RB_2               = pDataABF->nprsvSlope[0][2];
        pRegCmd->config27.bitfields.NP_SLOPE_RB_3               = pDataABF->nprsvSlope[0][3];
        pRegCmd->config28.bitfields.NP_SHIFT_GRGB_0             = pDataABF->nprsvShift[1][0];
        pRegCmd->config28.bitfields.NP_SHIFT_GRGB_1             = pDataABF->nprsvShift[1][1];
        pRegCmd->config28.bitfields.NP_SHIFT_GRGB_2             = pDataABF->nprsvShift[1][2];
        pRegCmd->config28.bitfields.NP_SHIFT_GRGB_3             = pDataABF->nprsvShift[1][3];
        pRegCmd->config28.bitfields.NP_SHIFT_RB_0               = pDataABF->nprsvShift[0][0];
        pRegCmd->config28.bitfields.NP_SHIFT_RB_1               = pDataABF->nprsvShift[0][1];
        pRegCmd->config28.bitfields.NP_SHIFT_RB_2               = pDataABF->nprsvShift[0][2];
        pRegCmd->config28.bitfields.NP_SHIFT_RB_3               = pDataABF->nprsvShift[0][3];
        pRegCmd->config29.bitfields.NP_BASE_GRGB_0              = pDataABF->nprsvBase[1][0];
        pRegCmd->config29.bitfields.NP_BASE_GRGB_1              = pDataABF->nprsvBase[1][1];
        pRegCmd->config30.bitfields.NP_BASE_GRGB_2              = pDataABF->nprsvBase[1][2];
        pRegCmd->config30.bitfields.NP_BASE_GRGB_3              = pDataABF->nprsvBase[1][3];
        pRegCmd->config31.bitfields.NP_SLOPE_GRGB_0             = pDataABF->nprsvSlope[1][0];
        pRegCmd->config31.bitfields.NP_SLOPE_GRGB_1             = pDataABF->nprsvSlope[1][1];
        pRegCmd->config32.bitfields.NP_SLOPE_GRGB_2             = pDataABF->nprsvSlope[1][2];
        pRegCmd->config32.bitfields.NP_SLOPE_GRGB_3             = pDataABF->nprsvSlope[1][3];
        pRegCmd->config33.bitfields.ACT_FAC0                    = pDataABF->activityFactor0;
        pRegCmd->config33.bitfields.ACT_FAC1                    = pDataABF->activityFactor1;
        pRegCmd->config34.bitfields.ACT_THD0                    = pDataABF->activityThreshold0;
        pRegCmd->config34.bitfields.ACT_THD1                    = pDataABF->activityThreshold1;
        pRegCmd->config35.bitfields.ACT_SMOOTH_THD0             = pDataABF->activitySmoothThreshold0;
        pRegCmd->config35.bitfields.ACT_SMOOTH_THD1             = pDataABF->activitySmoothThreshold1;
        pRegCmd->config35.bitfields.DARK_THD                    = pDataABF->darkThreshold;
        pRegCmd->config36.bitfields.GR_RATIO                    = pDataABF->grRatio;
        pRegCmd->config36.bitfields.RG_RATIO                    = pDataABF->rgRatio;
        pRegCmd->config37.bitfields.BG_RATIO                    = pDataABF->bgRatio;
        pRegCmd->config37.bitfields.GB_RATIO                    = pDataABF->gbRatio;
        pRegCmd->config38.bitfields.BR_RATIO                    = pDataABF->brRatio;
        pRegCmd->config38.bitfields.RB_RATIO                    = pDataABF->rbRatio;
        pRegCmd->config39.bitfields.EDGE_COUNT_THD              = pDataABF->edgeCountLow;
        pRegCmd->config39.bitfields.EDGE_DETECT_THD             = pDataABF->edgeDetectThreshold;
        pRegCmd->config39.bitfields.EDGE_DETECT_NOISE_SCALAR    = pDataABF->edgeDetectNoiseScalar;
        pRegCmd->config39.bitfields.EDGE_SMOOTH_STRENGTH        = pDataABF->edgeSmoothStrength;
        pRegCmd->config40.bitfields.EDGE_SMOOTH_NOISE_SCALAR_R  = pDataABF->edgeSmoothNoiseScalar[0];
        pRegCmd->config40.bitfields.EDGE_SMOOTH_NOISE_SCALAR_GR = pDataABF->edgeSmoothNoiseScalar[1];
        pRegCmd->config41.bitfields.EDGE_SMOOTH_NOISE_SCALAR_GB = pDataABF->edgeSmoothNoiseScalar[2];
        pRegCmd->config41.bitfields.EDGE_SMOOTH_NOISE_SCALAR_B  = pDataABF->edgeSmoothNoiseScalar[3];

        pRegCmd->config42.bitfields.BLS_THRESH_GR = pDataBLS->thresholdGR;
        pRegCmd->config42.bitfields.BLS_THRESH_R  = pDataBLS->thresholdR;
        pRegCmd->config43.bitfields.BLS_THRESH_B  = pDataBLS->thresholdB;
        pRegCmd->config43.bitfields.BLS_THRESH_GB = pDataBLS->thresholdGB;
        pRegCmd->config44.bitfields.BLS_OFFSET    = pDataBLS->offset;
        pRegCmd->config45.bitfields.BLS_SCALE     = pDataBLS->scale;

        /// Updating Data to fill in DMI Buffers
        for (UINT32 index = 0; index < DMIRAM_ABF40_NOISESTD_LENGTH; index++)
        {
            pCmd->pNoiseLUT[index] = pDataABF->noiseStdLUT[pDataABF->LUTBankSel][index];
            pCmd->pNoiseLUT1[index] = pDataABF->noiseStdLUT[pDataABF->LUTBankSel][index];
        }

        for (UINT32 index = 0; index < DMIRAM_ABF40_ACTIVITY_LENGTH; index++)
        {
            pCmd->pActivityLUT[index] = pDataABF->activityFactorLUT[pDataABF->LUTBankSel][index];
        }

        for (UINT32 index = 0; index < DMIRAM_ABF40_DARK_LENGTH; index++)
        {
            pCmd->pDarkLUT[index] = pDataABF->darkFactorLUT[pDataABF->LUTBankSel][index];
        }

        pModuleCfg->moduleConfig.bitfields.FILTER_EN              = pDataABF->bilateralEnable;
        pModuleCfg->moduleConfig.bitfields.ACT_ADJ_EN             = pDataABF->actAdjEnable;
        pModuleCfg->moduleConfig.bitfields.DARK_SMOOTH_EN         = pDataABF->darkSmoothEnable;
        pModuleCfg->moduleConfig.bitfields.DARK_DESAT_EN          = pDataABF->darkDesatEnable;
        pModuleCfg->moduleConfig.bitfields.DIR_SMOOTH_EN          = pDataABF->directSmoothEnable;
        pModuleCfg->moduleConfig.bitfields.MINMAX_EN              = pDataABF->minmaxEnable;
        pModuleCfg->moduleConfig.bitfields.CROSS_PLANE_EN         = pDataABF->crossProcessEnable;
        pModuleCfg->moduleConfig.bitfields.BLS_EN                 = pDataBLS->enable;
        pModuleCfg->moduleConfig.bitfields.PIX_MATCH_LEVEL_RB     = pDataABF->blockPixLevel[0];
        pModuleCfg->moduleConfig.bitfields.PIX_MATCH_LEVEL_G      = pDataABF->blockPixLevel[1];
        pModuleCfg->moduleConfig.bitfields.BLOCK_MATCH_PATTERN_RB = pDataABF->blockOpt;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input data is pdataABF %p pDataBLS %p pCmd %p ", pDataABF, pDataBLS, pCmd);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40Titan17x::~BPSABF40Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSABF40Titan17x::~BPSABF40Titan17x()
{
    CAMX_FREE(m_pModuleConfig);
    m_pModuleConfig = NULL;

    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSABF40Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSABF40Titan17x::DumpRegConfig()
{
    BPSABF40RegConfig* pRegCmd = static_cast<BPSABF40RegConfig*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        /// @brief Local debug dump register structure
        struct DumpInfo
        {
            UINT32  startRegAddr;    ///< Start address of the register of range
            UINT32  numRegs;         ///< The number of registers to be programmed.
            UINT32* pRegRangeAddr;   ///< The pointer to the structure in memory or a single varaible.
        };

        DumpInfo dumpRegInfoArray[] =
        {
            {
                regBPS_BPS_0_CLC_ABF_ABF_0_CFG,
                BPSABF40RegConfigLengthDWord,
                reinterpret_cast<UINT32*>(pRegCmd)
            }
        };

        for (UINT i = 0; i < CAMX_ARRAY_SIZE(dumpRegInfoArray); i++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SECTION[%d]: %08x: %08x",
                i, dumpRegInfoArray[i].startRegAddr,
                dumpRegInfoArray[i].startRegAddr + (dumpRegInfoArray[i].numRegs - 1) * RegisterWidthInBytes);

            for (UINT j = 0; j < dumpRegInfoArray[i].numRegs; j++)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "%08x: %08x",
                    dumpRegInfoArray[i].startRegAddr + j * 4, *(dumpRegInfoArray[i].pRegRangeAddr + j));
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Error pRegCmd is NULL");
    }
}

CAMX_NAMESPACE_END
