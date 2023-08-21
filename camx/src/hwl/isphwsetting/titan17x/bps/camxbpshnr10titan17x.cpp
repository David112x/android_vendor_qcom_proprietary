////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpshnr10titan17x.cpp
/// @brief CAMXBPSHNR10TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpshnr10titan17x.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief HNR10 register Configuration
struct BPSHNR10RegConfig
{
    BPS_BPS_0_CLC_HNR_NR_GAIN_TABLE_0            nrGainCoefficient0;        ///< HNR NR GAIN REGISTER0
    BPS_BPS_0_CLC_HNR_NR_GAIN_TABLE_1            nrGainCoefficient1;        ///< HNR NR GAIN REGISTER1
    BPS_BPS_0_CLC_HNR_NR_GAIN_TABLE_2            nrGainCoefficient2;        ///< HNR NR GAIN REGISTER2
    BPS_BPS_0_CLC_HNR_NR_GAIN_TABLE_3            nrGainCoefficient3;        ///< HNR NR GAIN REGISTER3
    BPS_BPS_0_CLC_HNR_NR_GAIN_TABLE_4            nrGainCoefficient4;        ///< HNR NR GAIN REGISTER4
    BPS_BPS_0_CLC_HNR_NR_GAIN_TABLE_5            nrGainCoefficient5;        ///< HNR NR GAIN REGISTER5
    BPS_BPS_0_CLC_HNR_NR_GAIN_TABLE_6            nrGainCoefficient6;        ///< HNR NR GAIN REGISTER6
    BPS_BPS_0_CLC_HNR_NR_GAIN_TABLE_7            nrGainCoefficient7;        ///< HNR NR GAIN REGISTER7
    BPS_BPS_0_CLC_HNR_NR_GAIN_TABLE_8            nrGainCoefficient8;        ///< HNR NR GAIN REGISTER8
    BPS_BPS_0_CLC_HNR_CNR_CFG_0                  cnrCFGCoefficient0;        ///< HNR CNR CONFIG REGISTER0
    BPS_BPS_0_CLC_HNR_CNR_CFG_1                  cnrCFGCoefficient1;        ///< HNR CNR CONFIG REGISTER1
    BPS_BPS_0_CLC_HNR_CNR_GAIN_TABLE_0           cnrGaninCoeffincient0;     ///< HNR CNR GAIN REGISTER0
    BPS_BPS_0_CLC_HNR_CNR_GAIN_TABLE_1           cnrGaninCoeffincient1;     ///< HNR CNR GAIN REGISTER1
    BPS_BPS_0_CLC_HNR_CNR_GAIN_TABLE_2           cnrGaninCoeffincient2;     ///< HNR CNR GAIN REGISTER2
    BPS_BPS_0_CLC_HNR_CNR_GAIN_TABLE_3           cnrGaninCoeffincient3;     ///< HNR CNR GAIN REGISTER3
    BPS_BPS_0_CLC_HNR_CNR_GAIN_TABLE_4           cnrGaninCoeffincient4;     ///< HNR CNR GAIN REGISTER4
    BPS_BPS_0_CLC_HNR_SNR_CFG_0                  snrCoefficient0;           ///< HNR SNR CONFIG REGISTER0
    BPS_BPS_0_CLC_HNR_SNR_CFG_1                  snrCoefficient1;           ///< HNR SNR CONFIG REGISTER1
    BPS_BPS_0_CLC_HNR_SNR_CFG_2                  snrCoefficient2;           ///< HNR SNR CONFIG REGISTER2
    BPS_BPS_0_CLC_HNR_FACE_CFG                   faceConfigCoefficient;     ///< HNR FACE CONFIG REGISTER
    BPS_BPS_0_CLC_HNR_FACE_OFFSET_CFG            faceOffsetCoefficient;     ///< HNR FACE OFFSET CONFIG REGISTER
    BPS_BPS_0_CLC_HNR_FACE_0_CENTER_CFG          faceCoefficient0;          ///< HNR FACE CENTER CONFIG REGISTER0
    BPS_BPS_0_CLC_HNR_FACE_1_CENTER_CFG          faceCoefficient1;          ///< HNR FACE CENTER CONFIG REGISTER1
    BPS_BPS_0_CLC_HNR_FACE_2_CENTER_CFG          faceCoefficient2;          ///< HNR FACE CENTER CONFIG REGISTER2
    BPS_BPS_0_CLC_HNR_FACE_3_CENTER_CFG          faceCoefficient3;          ///< HNR FACE CENTER CONFIG REGISTER3
    BPS_BPS_0_CLC_HNR_FACE_4_CENTER_CFG          faceCoefficient4;          ///< HNR FACE CENTER CONFIG REGISTER4
    BPS_BPS_0_CLC_HNR_FACE_0_RADIUS_CFG          radiusCoefficient0;        ///< HNR FACE RADIUS CONFIG REGISTER0
    BPS_BPS_0_CLC_HNR_FACE_1_RADIUS_CFG          radiusCoefficient1;        ///< HNR FACE RADIUS CONFIG REGISTER0
    BPS_BPS_0_CLC_HNR_FACE_2_RADIUS_CFG          radiusCoefficient2;        ///< HNR FACE RADIUS CONFIG REGISTER0
    BPS_BPS_0_CLC_HNR_FACE_3_RADIUS_CFG          radiusCoefficient3;        ///< HNR FACE RADIUS CONFIG REGISTER0
    BPS_BPS_0_CLC_HNR_FACE_4_RADIUS_CFG          radiusCoefficient4;        ///< HNR FACE RADIUS CONFIG REGISTER0
    BPS_BPS_0_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_0 rnrAnchorCoefficient0;     ///< RNR ANCHOR BASE SETTING REGISTER0
    BPS_BPS_0_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_1 rnrAnchorCoefficient1;     ///< RNR ANCHOR BASE SETTING REGISTER1
    BPS_BPS_0_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_2 rnrAnchorCoefficient2;     ///< RNR ANCHOR BASE SETTING REGISTER2
    BPS_BPS_0_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_3 rnrAnchorCoefficient3;     ///< RNR ANCHOR BASE SETTING REGISTER3
    BPS_BPS_0_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_4 rnrAnchorCoefficient4;     ///< RNR ANCHOR BASE SETTING REGISTER4
    BPS_BPS_0_CLC_HNR_RNR_ANCHOR_BASE_SETTINGS_5 rnrAnchorCoefficient5;     ///< RNR ANCHOR BASE SETTING REGISTER5
    BPS_BPS_0_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_0 rnrSlopeShiftCoefficient0; ///< RNR SLOPE SHIFT SETTING REGISTER0
    BPS_BPS_0_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_1 rnrSlopeShiftCoefficient1; ///< RNR SLOPE SHIFT SETTING REGISTER1
    BPS_BPS_0_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_2 rnrSlopeShiftCoefficient2; ///< RNR SLOPE SHIFT SETTING REGISTER2
    BPS_BPS_0_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_3 rnrSlopeShiftCoefficient3; ///< RNR SLOPE SHIFT SETTING REGISTER3
    BPS_BPS_0_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_4 rnrSlopeShiftCoefficient4; ///< RNR SLOPE SHIFT SETTING REGISTER4
    BPS_BPS_0_CLC_HNR_RNR_SLOPE_SHIFT_SETTINGS_5 rnrSlopeShiftCoefficient5; ///< RNR SLOPE SHIFT SETTING REGISTER5
    BPS_BPS_0_CLC_HNR_RNR_INIT_HV_OFFSET         hnrCoefficient1;           ///< RNR INIT HV OFFSET REGISTER
    BPS_BPS_0_CLC_HNR_RNR_R_SQUARE_INIT          rnr_r_squareCoefficient;   ///< RNR R SQUARE INIT REGISTER
    BPS_BPS_0_CLC_HNR_RNR_R_SCALE_SHIFT          rnr_r_ScaleCoefficient;    ///< RNR R SCALE SHIFT REGISTER
    BPS_BPS_0_CLC_HNR_LPF3_CFG                   lpf3ConfigCoefficient;     ///< HNR LPF3 CONFIG REGISTER
    BPS_BPS_0_CLC_HNR_MISC_CFG                   miscConfigCoefficient;     ///< HNR MIS CONFIG REGISTER
} CAMX_PACKED;

/// @brief HNR10 module Configuration
struct BPSHNR10ModuleConfig
{
    BPS_BPS_0_CLC_HNR_MODULE_CFG  moduleConfig;    ///< Module Configuration
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 LNRGainLUT                      = BPS_BPS_0_CLC_HNR_DMI_LUT_CFG_LUT_SEL_LNR_GAIN_ARR;
static const UINT32 MergedFNRGainAndGainClampLUT    = BPS_BPS_0_CLC_HNR_DMI_LUT_CFG_LUT_SEL_MERGED_FNR_GAIN_ARR_GAIN_CLAMP_ARR;
static const UINT32 FNRAcLUT                        = BPS_BPS_0_CLC_HNR_DMI_LUT_CFG_LUT_SEL_FNR_AC_TH_ARR;
static const UINT32 SNRGainLUT                      = BPS_BPS_0_CLC_HNR_DMI_LUT_CFG_LUT_SEL_SNR_GAIN_ARR;
static const UINT32 BlendLNRGainLUT                 = BPS_BPS_0_CLC_HNR_DMI_LUT_CFG_LUT_SEL_BLEND_LNR_GAIN_ARR;
static const UINT32 BlendSNRGainLUT                 = BPS_BPS_0_CLC_HNR_DMI_LUT_CFG_LUT_SEL_BLEND_SNR_GAIN_ARR;

static const UINT32 BPSHNR10RegConfigLengthDWord = sizeof(BPSHNR10RegConfig) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10Titan17x::BPSHNR10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHNR10Titan17x::BPSHNR10Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSHNR10Titan17x* pHWSetting = CAMX_NEW BPSHNR10Titan17x;

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
/// BPSHNR10Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pModuleConfig = CAMX_CALLOC(sizeof(BPSHNR10ModuleConfig));

    if (NULL == m_pModuleConfig)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for module configuration!");
    }
    else
    {
        m_pRegCmd = CAMX_CALLOC(sizeof(BPSHNR10RegConfig));

        if (NULL != m_pRegCmd)
        {
            SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSHNR10RegConfigLengthDWord));
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
// BPSHNR10Titan17x::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10Titan17x::WriteLUTtoDMI(
    VOID* pInput)
{
    CamxResult      result     = CamxResultSuccess;
    ISPInputData*   pInputData = static_cast<ISPInputData*>(pInput);

    // CDM pack the DMI buffer and patch the LUT DMI buffer into CDM pack
    if ((NULL != pInputData) && (NULL != pInputData->pDMICmdBuffer))
    {
        UINT32 LUTOffset = 0;
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_HNR_DMI_CFG,
                                         LNRGainLUT,
                                         pInputData->p64bitDMIBuffer,
                                         LUTOffset,
                                         BPSLNRLutBufferSize);
        CAMX_ASSERT(CamxResultSuccess == result);

        LUTOffset += BPSLNRLutBufferSize;
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_HNR_DMI_CFG,
                                         MergedFNRGainAndGainClampLUT,
                                         pInputData->p64bitDMIBuffer,
                                         LUTOffset,
                                         BPSFNRAndGAinLutBufferSize);
        CAMX_ASSERT(CamxResultSuccess == result);

        LUTOffset += BPSFNRAndGAinLutBufferSize;
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_HNR_DMI_CFG,
                                         FNRAcLUT,
                                         pInputData->p64bitDMIBuffer,
                                         LUTOffset,
                                         BPSFNRAcLutBufferSize);
        CAMX_ASSERT(CamxResultSuccess == result);

        LUTOffset += BPSFNRAcLutBufferSize;
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_HNR_DMI_CFG,
                                         SNRGainLUT,
                                         pInputData->p64bitDMIBuffer,
                                         LUTOffset,
                                         BPSSNRLutBufferSize);
        CAMX_ASSERT(CamxResultSuccess == result);

        LUTOffset += BPSSNRLutBufferSize;
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_HNR_DMI_CFG,
                                         BlendLNRGainLUT,
                                         pInputData->p64bitDMIBuffer,
                                         LUTOffset,
                                         BPSBlendLNRLutBufferSize);
        CAMX_ASSERT(CamxResultSuccess == result);

        LUTOffset += BPSBlendLNRLutBufferSize;
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_HNR_DMI_CFG,
                                         BlendSNRGainLUT,
                                         pInputData->p64bitDMIBuffer,
                                         LUTOffset,
                                         BPSBlendSNRLutBufferSize);
        CAMX_ASSERT(CamxResultSuccess == result);
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10Titan17x::CreateCmdList(
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
                                              regBPS_BPS_0_CLC_HNR_NR_GAIN_TABLE_0,
                                              BPSHNR10RegConfigLengthDWord,
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
// BPSHNR10Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    BPSHNR10ModuleConfig* pModuleCfg = static_cast<BPSHNR10ModuleConfig*>(m_pModuleConfig);
    BPSHNR10RegConfig*    pRegCmd    = static_cast<BPSHNR10RegConfig*>(m_pRegCmd);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pInputData->pCalculatedData->noiseReductionMode = pInputData->pHALTagsData->noiseReductionMode;

    pBPSIQSettings->hnrParameters.moduleCfg.EN           = moduleEnable;

    pBPSIQSettings->hnrParameters.moduleCfg.BLEND_SNR_EN = pModuleCfg->moduleConfig.bitfields.BLEND_SNR_EN;
    pBPSIQSettings->hnrParameters.moduleCfg.BLEND_CNR_EN = pModuleCfg->moduleConfig.bitfields.BLEND_CNR_EN;
    pBPSIQSettings->hnrParameters.moduleCfg.BLEND_ENABLE = pModuleCfg->moduleConfig.bitfields.BLEND_ENABLE;
    pBPSIQSettings->hnrParameters.moduleCfg.LPF3_EN      = pModuleCfg->moduleConfig.bitfields.LPF3_EN;
    pBPSIQSettings->hnrParameters.moduleCfg.FNR_EN       = pModuleCfg->moduleConfig.bitfields.FNR_EN;
    pBPSIQSettings->hnrParameters.moduleCfg.FD_SNR_EN    = pModuleCfg->moduleConfig.bitfields.FD_SNR_EN;
    pBPSIQSettings->hnrParameters.moduleCfg.SNR_EN       = pModuleCfg->moduleConfig.bitfields.SNR_EN;
    pBPSIQSettings->hnrParameters.moduleCfg.CNR_EN       = pModuleCfg->moduleConfig.bitfields.CNR_EN;
    pBPSIQSettings->hnrParameters.moduleCfg.RNR_EN       = pModuleCfg->moduleConfig.bitfields.RNR_EN;
    pBPSIQSettings->hnrParameters.moduleCfg.LNR_EN       = pModuleCfg->moduleConfig.bitfields.LNR_EN;
    pBPSIQSettings->hnrParameters.snrSkinSmoothingStr    = pRegCmd->snrCoefficient0.bitfields.SNR_SKIN_SMOOTHING_STR;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSHNR10RegConfig* pRegCmd            = static_cast<BPSHNR10RegConfig*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSHNR10RegConfig) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSHNRData.HNRConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSHNRData.HNRConfig,
                        pRegCmd,
                        sizeof(BPSHNR10RegConfig));

        if (TRUE == pBPSIQSettings->hnrParameters.moduleCfg.EN)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSHNR10Register,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata17x.BPSHNRData.HNRConfig),
                &pBPSTuningMetadata->BPSTuningMetadata17x.BPSHNRData.HNRConfig,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSHNRData.HNRConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHNR10Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result       = CamxResultSuccess;
    UINT                i            = 0;
    HNR10UnpackedField* pData        = static_cast<HNR10UnpackedField*>(pInput);
    HNR10OutputData*    pOutputData  = static_cast<HNR10OutputData*>(pOutput);

    BPSHNR10RegConfig*    pRegCmd    = static_cast<BPSHNR10RegConfig*>(m_pRegCmd);
    BPSHNR10ModuleConfig* pModuleCfg = static_cast<BPSHNR10ModuleConfig*>(m_pModuleConfig);

    if ((NULL != pData) && (NULL != pOutputData))
    {
        pRegCmd->faceCoefficient0.bitfields.FACE_CENTER_HORIZONTAL      = pData->face_center_horizontal[0];
        pRegCmd->faceCoefficient0.bitfields.FACE_CENTER_VERTICAL        = pData->face_center_vertical[0];
        pRegCmd->faceCoefficient1.bitfields.FACE_CENTER_HORIZONTAL      = pData->face_center_horizontal[1];
        pRegCmd->faceCoefficient1.bitfields.FACE_CENTER_VERTICAL        = pData->face_center_vertical[1];
        pRegCmd->faceCoefficient2.bitfields.FACE_CENTER_HORIZONTAL      = pData->face_center_horizontal[2];
        pRegCmd->faceCoefficient2.bitfields.FACE_CENTER_VERTICAL        = pData->face_center_vertical[2];
        pRegCmd->faceCoefficient3.bitfields.FACE_CENTER_HORIZONTAL      = pData->face_center_horizontal[3];
        pRegCmd->faceCoefficient3.bitfields.FACE_CENTER_VERTICAL        = pData->face_center_vertical[3];
        pRegCmd->faceCoefficient4.bitfields.FACE_CENTER_HORIZONTAL      = pData->face_center_horizontal[4];
        pRegCmd->faceCoefficient4.bitfields.FACE_CENTER_VERTICAL        = pData->face_center_vertical[4];

        pRegCmd->miscConfigCoefficient.bitfields.ABS_AMP_SHIFT          = pData->abs_amp_shift;
        pRegCmd->miscConfigCoefficient.bitfields.BLEND_CNR_ADJ_GAIN     = pData->blend_cnr_adj_gain;
        pRegCmd->miscConfigCoefficient.bitfields.FNR_AC_SHIFT           = pData->fnr_ac_shift;
        pRegCmd->miscConfigCoefficient.bitfields.LNR_SHIFT              = pData->lnr_shift;

        pRegCmd->lpf3ConfigCoefficient.bitfields.LPF3_OFFSET            = pData->lpf3_offset;
        pRegCmd->lpf3ConfigCoefficient.bitfields.LPF3_PERCENT           = pData->lpf3_percent;
        pRegCmd->lpf3ConfigCoefficient.bitfields.LPF3_STRENGTH          = pData->lpf3_strength;

        pRegCmd->rnr_r_ScaleCoefficient.bitfields.RNR_R_SQUARE_SCALE    = pData->rnr_r_square_scale;
        pRegCmd->rnr_r_ScaleCoefficient.bitfields.RNR_R_SQUARE_SHIFT    = pData->rnr_r_square_shift;
        pRegCmd->rnr_r_squareCoefficient.bitfields.RNR_R_SQUARE_INIT    = pData->rnr_r_square_init;

        pRegCmd->hnrCoefficient1.bitfields.RNR_BX = pData->rnr_bx;
        pRegCmd->hnrCoefficient1.bitfields.RNR_BY = pData->rnr_by;

        pRegCmd->rnrSlopeShiftCoefficient0.bitfields.RNR_SHIFT_0    = pData->rnr_shift[0];
        pRegCmd->rnrSlopeShiftCoefficient0.bitfields.RNR_SLOPE_0    = pData->rnr_slope[0];
        pRegCmd->rnrSlopeShiftCoefficient1.bitfields.RNR_SHIFT_1    = pData->rnr_shift[1];
        pRegCmd->rnrSlopeShiftCoefficient1.bitfields.RNR_SLOPE_1    = pData->rnr_slope[1];
        pRegCmd->rnrSlopeShiftCoefficient2.bitfields.RNR_SHIFT_2    = pData->rnr_shift[2];
        pRegCmd->rnrSlopeShiftCoefficient2.bitfields.RNR_SLOPE_2    = pData->rnr_slope[2];
        pRegCmd->rnrSlopeShiftCoefficient3.bitfields.RNR_SHIFT_3    = pData->rnr_shift[3];
        pRegCmd->rnrSlopeShiftCoefficient3.bitfields.RNR_SLOPE_3    = pData->rnr_slope[3];
        pRegCmd->rnrSlopeShiftCoefficient4.bitfields.RNR_SHIFT_4    = pData->rnr_shift[4];
        pRegCmd->rnrSlopeShiftCoefficient4.bitfields.RNR_SLOPE_4    = pData->rnr_slope[4];
        pRegCmd->rnrSlopeShiftCoefficient5.bitfields.RNR_SHIFT_5    = pData->rnr_shift[5];
        pRegCmd->rnrSlopeShiftCoefficient5.bitfields.RNR_SLOPE_5    = pData->rnr_slope[5];

        pRegCmd->rnrAnchorCoefficient0.bitfields.RNR_ANCHOR_0       = pData->rnr_anchor[0];
        pRegCmd->rnrAnchorCoefficient0.bitfields.RNR_BASE_0         = pData->rnr_base[0];
        pRegCmd->rnrAnchorCoefficient1.bitfields.RNR_ANCHOR_1       = pData->rnr_anchor[1];
        pRegCmd->rnrAnchorCoefficient1.bitfields.RNR_BASE_1         = pData->rnr_base[1];
        pRegCmd->rnrAnchorCoefficient2.bitfields.RNR_ANCHOR_2       = pData->rnr_anchor[2];
        pRegCmd->rnrAnchorCoefficient2.bitfields.RNR_BASE_2         = pData->rnr_base[2];
        pRegCmd->rnrAnchorCoefficient3.bitfields.RNR_ANCHOR_3       = pData->rnr_anchor[3];
        pRegCmd->rnrAnchorCoefficient3.bitfields.RNR_BASE_3         = pData->rnr_base[3];
        pRegCmd->rnrAnchorCoefficient4.bitfields.RNR_ANCHOR_4       = pData->rnr_anchor[4];
        pRegCmd->rnrAnchorCoefficient4.bitfields.RNR_BASE_4         = pData->rnr_base[4];
        pRegCmd->rnrAnchorCoefficient5.bitfields.RNR_ANCHOR_5       = pData->rnr_anchor[5];
        pRegCmd->rnrAnchorCoefficient5.bitfields.RNR_BASE_5         = pData->rnr_base[5];

        pRegCmd->radiusCoefficient0.bitfields.FACE_RADIUS_BOUNDARY  = pData->face_radius_boundary[0];
        pRegCmd->radiusCoefficient0.bitfields.FACE_RADIUS_SHIFT     = pData->face_radius_shift[0];
        pRegCmd->radiusCoefficient0.bitfields.FACE_RADIUS_SLOPE     = pData->face_radius_slope[0];
        pRegCmd->radiusCoefficient0.bitfields.FACE_SLOPE_SHIFT      = pData->face_slope_shift[0];


        pRegCmd->radiusCoefficient1.bitfields.FACE_RADIUS_BOUNDARY  = pData->face_radius_boundary[1];
        pRegCmd->radiusCoefficient1.bitfields.FACE_RADIUS_SHIFT     = pData->face_radius_shift[1];
        pRegCmd->radiusCoefficient1.bitfields.FACE_RADIUS_SLOPE     = pData->face_radius_slope[1];
        pRegCmd->radiusCoefficient1.bitfields.FACE_SLOPE_SHIFT      = pData->face_slope_shift[1];

        pRegCmd->radiusCoefficient2.bitfields.FACE_RADIUS_BOUNDARY  = pData->face_radius_boundary[2];
        pRegCmd->radiusCoefficient2.bitfields.FACE_RADIUS_SHIFT     = pData->face_radius_shift[2];
        pRegCmd->radiusCoefficient2.bitfields.FACE_RADIUS_SLOPE     = pData->face_radius_slope[2];
        pRegCmd->radiusCoefficient2.bitfields.FACE_SLOPE_SHIFT      = pData->face_slope_shift[2];

        pRegCmd->radiusCoefficient3.bitfields.FACE_RADIUS_BOUNDARY  = pData->face_radius_boundary[3];
        pRegCmd->radiusCoefficient3.bitfields.FACE_RADIUS_SHIFT     = pData->face_radius_shift[3];
        pRegCmd->radiusCoefficient3.bitfields.FACE_RADIUS_SLOPE     = pData->face_radius_slope[3];
        pRegCmd->radiusCoefficient3.bitfields.FACE_SLOPE_SHIFT      = pData->face_slope_shift[3];

        pRegCmd->radiusCoefficient4.bitfields.FACE_RADIUS_BOUNDARY  = pData->face_radius_boundary[4];
        pRegCmd->radiusCoefficient4.bitfields.FACE_RADIUS_SHIFT     = pData->face_radius_shift[4];
        pRegCmd->radiusCoefficient4.bitfields.FACE_RADIUS_SLOPE     = pData->face_radius_slope[4];
        pRegCmd->radiusCoefficient4.bitfields.FACE_SLOPE_SHIFT      = pData->face_slope_shift[4];

        pRegCmd->nrGainCoefficient0.bitfields.FILTERING_NR_GAIN_ARR_0  = pData->filtering_nr_gain_arr[0];
        pRegCmd->nrGainCoefficient0.bitfields.FILTERING_NR_GAIN_ARR_1  = pData->filtering_nr_gain_arr[1];
        pRegCmd->nrGainCoefficient0.bitfields.FILTERING_NR_GAIN_ARR_2  = pData->filtering_nr_gain_arr[2];
        pRegCmd->nrGainCoefficient0.bitfields.FILTERING_NR_GAIN_ARR_3  = pData->filtering_nr_gain_arr[3];
        pRegCmd->nrGainCoefficient1.bitfields.FILTERING_NR_GAIN_ARR_4  = pData->filtering_nr_gain_arr[4];
        pRegCmd->nrGainCoefficient1.bitfields.FILTERING_NR_GAIN_ARR_5  = pData->filtering_nr_gain_arr[5];
        pRegCmd->nrGainCoefficient1.bitfields.FILTERING_NR_GAIN_ARR_6  = pData->filtering_nr_gain_arr[6];
        pRegCmd->nrGainCoefficient1.bitfields.FILTERING_NR_GAIN_ARR_7  = pData->filtering_nr_gain_arr[7];
        pRegCmd->nrGainCoefficient2.bitfields.FILTERING_NR_GAIN_ARR_8  = pData->filtering_nr_gain_arr[8];
        pRegCmd->nrGainCoefficient2.bitfields.FILTERING_NR_GAIN_ARR_9  = pData->filtering_nr_gain_arr[9];
        pRegCmd->nrGainCoefficient2.bitfields.FILTERING_NR_GAIN_ARR_10 = pData->filtering_nr_gain_arr[10];
        pRegCmd->nrGainCoefficient2.bitfields.FILTERING_NR_GAIN_ARR_11 = pData->filtering_nr_gain_arr[11];
        pRegCmd->nrGainCoefficient3.bitfields.FILTERING_NR_GAIN_ARR_12 = pData->filtering_nr_gain_arr[12];
        pRegCmd->nrGainCoefficient3.bitfields.FILTERING_NR_GAIN_ARR_13 = pData->filtering_nr_gain_arr[13];
        pRegCmd->nrGainCoefficient3.bitfields.FILTERING_NR_GAIN_ARR_14 = pData->filtering_nr_gain_arr[14];
        pRegCmd->nrGainCoefficient3.bitfields.FILTERING_NR_GAIN_ARR_15 = pData->filtering_nr_gain_arr[15];
        pRegCmd->nrGainCoefficient4.bitfields.FILTERING_NR_GAIN_ARR_16 = pData->filtering_nr_gain_arr[16];
        pRegCmd->nrGainCoefficient4.bitfields.FILTERING_NR_GAIN_ARR_17 = pData->filtering_nr_gain_arr[17];
        pRegCmd->nrGainCoefficient4.bitfields.FILTERING_NR_GAIN_ARR_18 = pData->filtering_nr_gain_arr[18];
        pRegCmd->nrGainCoefficient4.bitfields.FILTERING_NR_GAIN_ARR_19 = pData->filtering_nr_gain_arr[19];
        pRegCmd->nrGainCoefficient5.bitfields.FILTERING_NR_GAIN_ARR_20 = pData->filtering_nr_gain_arr[20];
        pRegCmd->nrGainCoefficient5.bitfields.FILTERING_NR_GAIN_ARR_21 = pData->filtering_nr_gain_arr[21];
        pRegCmd->nrGainCoefficient5.bitfields.FILTERING_NR_GAIN_ARR_22 = pData->filtering_nr_gain_arr[22];
        pRegCmd->nrGainCoefficient5.bitfields.FILTERING_NR_GAIN_ARR_23 = pData->filtering_nr_gain_arr[23];
        pRegCmd->nrGainCoefficient6.bitfields.FILTERING_NR_GAIN_ARR_24 = pData->filtering_nr_gain_arr[24];
        pRegCmd->nrGainCoefficient6.bitfields.FILTERING_NR_GAIN_ARR_25 = pData->filtering_nr_gain_arr[25];
        pRegCmd->nrGainCoefficient6.bitfields.FILTERING_NR_GAIN_ARR_26 = pData->filtering_nr_gain_arr[26];
        pRegCmd->nrGainCoefficient6.bitfields.FILTERING_NR_GAIN_ARR_27 = pData->filtering_nr_gain_arr[27];
        pRegCmd->nrGainCoefficient7.bitfields.FILTERING_NR_GAIN_ARR_28 = pData->filtering_nr_gain_arr[28];
        pRegCmd->nrGainCoefficient7.bitfields.FILTERING_NR_GAIN_ARR_29 = pData->filtering_nr_gain_arr[29];
        pRegCmd->nrGainCoefficient7.bitfields.FILTERING_NR_GAIN_ARR_30 = pData->filtering_nr_gain_arr[30];
        pRegCmd->nrGainCoefficient7.bitfields.FILTERING_NR_GAIN_ARR_31 = pData->filtering_nr_gain_arr[31];
        pRegCmd->nrGainCoefficient8.bitfields.FILTERING_NR_GAIN_ARR_32 = pData->filtering_nr_gain_arr[32];

        pRegCmd->cnrCFGCoefficient0.bitfields.CNR_LOW_THRD_U           = pData->cnr_low_thrd_u;
        pRegCmd->cnrCFGCoefficient0.bitfields.CNR_LOW_THRD_V           = pData->cnr_low_thrd_v;
        pRegCmd->cnrCFGCoefficient0.bitfields.CNR_THRD_GAP_U           = pData->cnr_thrd_gap_u;
        pRegCmd->cnrCFGCoefficient0.bitfields.CNR_THRD_GAP_V           = pData->cnr_thrd_gap_v;
        pRegCmd->cnrCFGCoefficient1.bitfields.CNR_ADJ_GAIN             = pData->cnr_adj_gain;
        pRegCmd->cnrCFGCoefficient1.bitfields.CNR_SCALE                = pData->cnr_scale;

        pRegCmd->cnrGaninCoeffincient0.bitfields.CNR_GAIN_ARR_0     = pData->cnr_gain_arr[0];
        pRegCmd->cnrGaninCoeffincient0.bitfields.CNR_GAIN_ARR_1     = pData->cnr_gain_arr[1];
        pRegCmd->cnrGaninCoeffincient0.bitfields.CNR_GAIN_ARR_2     = pData->cnr_gain_arr[2];
        pRegCmd->cnrGaninCoeffincient0.bitfields.CNR_GAIN_ARR_3     = pData->cnr_gain_arr[3];
        pRegCmd->cnrGaninCoeffincient0.bitfields.CNR_GAIN_ARR_4     = pData->cnr_gain_arr[4];
        pRegCmd->cnrGaninCoeffincient1.bitfields.CNR_GAIN_ARR_5     = pData->cnr_gain_arr[5];
        pRegCmd->cnrGaninCoeffincient1.bitfields.CNR_GAIN_ARR_6     = pData->cnr_gain_arr[6];
        pRegCmd->cnrGaninCoeffincient1.bitfields.CNR_GAIN_ARR_7     = pData->cnr_gain_arr[7];
        pRegCmd->cnrGaninCoeffincient1.bitfields.CNR_GAIN_ARR_8     = pData->cnr_gain_arr[8];
        pRegCmd->cnrGaninCoeffincient1.bitfields.CNR_GAIN_ARR_9     = pData->cnr_gain_arr[9];
        pRegCmd->cnrGaninCoeffincient2.bitfields.CNR_GAIN_ARR_10    = pData->cnr_gain_arr[10];
        pRegCmd->cnrGaninCoeffincient2.bitfields.CNR_GAIN_ARR_11    = pData->cnr_gain_arr[11];
        pRegCmd->cnrGaninCoeffincient2.bitfields.CNR_GAIN_ARR_12    = pData->cnr_gain_arr[12];
        pRegCmd->cnrGaninCoeffincient2.bitfields.CNR_GAIN_ARR_13    = pData->cnr_gain_arr[13];
        pRegCmd->cnrGaninCoeffincient2.bitfields.CNR_GAIN_ARR_14    = pData->cnr_gain_arr[14];
        pRegCmd->cnrGaninCoeffincient3.bitfields.CNR_GAIN_ARR_15    = pData->cnr_gain_arr[15];
        pRegCmd->cnrGaninCoeffincient3.bitfields.CNR_GAIN_ARR_16    = pData->cnr_gain_arr[16];
        pRegCmd->cnrGaninCoeffincient3.bitfields.CNR_GAIN_ARR_17    = pData->cnr_gain_arr[17];
        pRegCmd->cnrGaninCoeffincient3.bitfields.CNR_GAIN_ARR_18    = pData->cnr_gain_arr[18];
        pRegCmd->cnrGaninCoeffincient3.bitfields.CNR_GAIN_ARR_19    = pData->cnr_gain_arr[19];
        pRegCmd->cnrGaninCoeffincient4.bitfields.CNR_GAIN_ARR_20    = pData->cnr_gain_arr[20];
        pRegCmd->cnrGaninCoeffincient4.bitfields.CNR_GAIN_ARR_21    = pData->cnr_gain_arr[21];
        pRegCmd->cnrGaninCoeffincient4.bitfields.CNR_GAIN_ARR_22    = pData->cnr_gain_arr[22];
        pRegCmd->cnrGaninCoeffincient4.bitfields.CNR_GAIN_ARR_23    = pData->cnr_gain_arr[23];
        pRegCmd->cnrGaninCoeffincient4.bitfields.CNR_GAIN_ARR_24    = pData->cnr_gain_arr[24];

        pRegCmd->snrCoefficient0.bitfields.SNR_SKIN_HUE_MAX         = pData->snr_skin_hue_max;
        pRegCmd->snrCoefficient0.bitfields.SNR_SKIN_HUE_MIN         = pData->snr_skin_hue_min;
        pRegCmd->snrCoefficient0.bitfields.SNR_SKIN_SMOOTHING_STR   = pData->snr_skin_smoothing_str;
        pRegCmd->snrCoefficient0.bitfields.SNR_SKIN_Y_MIN           = pData->snr_skin_y_min;

        pRegCmd->snrCoefficient1.bitfields.SNR_SKIN_Y_MAX           = pData->snr_skin_y_max;
        pRegCmd->snrCoefficient1.bitfields.SNR_BOUNDARY_PROBABILITY = pData->snr_boundary_probability;
        pRegCmd->snrCoefficient1.bitfields.SNR_QSTEP_NONSKIN        = pData->snr_qstep_nonskin;
        pRegCmd->snrCoefficient1.bitfields.SNR_QSTEP_SKIN           = pData->snr_qstep_skin;
        pRegCmd->snrCoefficient2.bitfields.SNR_SAT_MAX_SLOPE        = pData->snr_sat_max_slope;
        pRegCmd->snrCoefficient2.bitfields.SNR_SAT_MIN_SLOPE        = pData->snr_sat_min_slope;
        pRegCmd->snrCoefficient2.bitfields.SNR_SKIN_YMAX_SAT_MAX    = pData->snr_skin_ymax_sat_max;
        pRegCmd->snrCoefficient2.bitfields.SNR_SKIN_YMAX_SAT_MIN    = pData->snr_skin_ymax_sat_min;

        pRegCmd->faceConfigCoefficient.bitfields.FACE_NUM               = pData->face_num;
        pRegCmd->faceOffsetCoefficient.bitfields.FACE_HORIZONTAL_OFFSET = pData->face_horizontal_offset;
        pRegCmd->faceOffsetCoefficient.bitfields.FACE_VERTICAL_OFFSET   = pData->face_vertical_offset;

        pModuleCfg->moduleConfig.bitfields.BLEND_SNR_EN         = pData->blend_snr_en;
        pModuleCfg->moduleConfig.bitfields.BLEND_CNR_EN         = pData->blend_cnr_en;
        pModuleCfg->moduleConfig.bitfields.BLEND_ENABLE         = pData->blend_enable;
        pModuleCfg->moduleConfig.bitfields.LPF3_EN              = pData->lpf3_en;
        pModuleCfg->moduleConfig.bitfields.FNR_EN               = pData->fnr_en;
        pModuleCfg->moduleConfig.bitfields.FD_SNR_EN            = pData->fd_snr_en;
        pModuleCfg->moduleConfig.bitfields.SNR_EN               = pData->snr_en;
        pModuleCfg->moduleConfig.bitfields.RNR_EN               = pData->rnr_en;
        pModuleCfg->moduleConfig.bitfields.LNR_EN               = pData->lnr_en;

        for (i = 0; i < HNR_V10_LNR_ARR_NUM; i++)
        {
            pOutputData->pLNRDMIBuffer[i] =
                static_cast<UINT32>(pData->lnr_gain_arr[pData->lut_bank_sel][i]) & Utils::AllOnes32(12);
        }

        for (i = 0; i < HNR_V10_FNR_ARR_NUM; i++)
        {
            pOutputData->pFNRAndClampDMIBuffer[i] = pData->merged_fnr_gain_arr_gain_clamp_arr[pData->lut_bank_sel][i] &
                Utils::AllOnes32(28);
            pOutputData->pFNRAcDMIBuffer[i] = static_cast<UINT32>(pData->fnr_ac_th_arr[pData->lut_bank_sel][i]) &
                Utils::AllOnes32(12);
        }

        for (i = 0; i < HNR_V10_SNR_ARR_NUM; i++)
        {
            pOutputData->pSNRDMIBuffer[i] = static_cast<UINT32>(pData->snr_gain_arr[pData->lut_bank_sel][i]) &
                Utils::AllOnes32(10);
        }

        for (i = 0; i < HNR_V10_BLEND_LNR_ARR_NUM; i++)
        {
            pOutputData->pBlendLNRDMIBuffer[i] = static_cast<UINT32>(pData->blend_lnr_gain_arr[pData->lut_bank_sel][i]) &
                Utils::AllOnes32(16);
        }

        for (i = 0; i < HNR_V10_BLEND_SNR_ARR_NUM; i++)
        {
            pOutputData->pBlendSNRDMIBuffer[i] = static_cast<UINT32>(pData->blend_snr_gain_arr[pData->lut_bank_sel][i]) &
                Utils::AllOnes32(10);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid pointer: pData: %p, pOutputData: %p", pData, pOutputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10Titan17x::~BPSHNR10Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHNR10Titan17x::~BPSHNR10Titan17x()
{
    CAMX_FREE(m_pModuleConfig);
    m_pModuleConfig = NULL;

    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHNR10Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHNR10Titan17x::DumpRegConfig()
{
    BPSHNR10RegConfig* pRegCmd    = static_cast<BPSHNR10RegConfig*>(m_pRegCmd);
    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "nrGainCoefficient0         = %x\n", pRegCmd->nrGainCoefficient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "nrGainCoefficient1         = %x\n", pRegCmd->nrGainCoefficient1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "nrGainCoefficient2         = %x\n", pRegCmd->nrGainCoefficient2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "nrGainCoefficient3         = %x\n", pRegCmd->nrGainCoefficient3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "nrGainCoefficient4         = %x\n", pRegCmd->nrGainCoefficient4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "nrGainCoefficient5         = %x\n", pRegCmd->nrGainCoefficient5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "nrGainCoefficient6         = %x\n", pRegCmd->nrGainCoefficient6);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "nrGainCoefficient7         = %x\n", pRegCmd->nrGainCoefficient7);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "nrGainCoefficient8         = %x\n", pRegCmd->nrGainCoefficient8);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "cnrCFGCoefficient0         = %x\n", pRegCmd->cnrCFGCoefficient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "cnrCFGCoefficient1         = %x\n", pRegCmd->cnrCFGCoefficient1);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "cnrGaninCoeffincient0      = %x\n", pRegCmd->cnrGaninCoeffincient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "cnrGaninCoeffincient1      = %x\n", pRegCmd->cnrGaninCoeffincient1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "cnrGaninCoeffincient2      = %x\n", pRegCmd->cnrGaninCoeffincient2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "cnrGaninCoeffincient3      = %x\n", pRegCmd->cnrGaninCoeffincient3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "cnrGaninCoeffincient4      = %x\n", pRegCmd->cnrGaninCoeffincient4);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "snrCoefficient0            = %x\n", pRegCmd->snrCoefficient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "snrCoefficient1            = %x\n", pRegCmd->snrCoefficient1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "snrCoefficient2            = %x\n", pRegCmd->snrCoefficient2);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "faceConfigCoefficient      = %x\n", pRegCmd->faceConfigCoefficient);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "faceOffsetCoefficient      = %x\n", pRegCmd->faceOffsetCoefficient);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "faceCoefficient0           = %x\n", pRegCmd->faceCoefficient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "faceCoefficient1           = %x\n", pRegCmd->faceCoefficient1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "faceCoefficient2           = %x\n", pRegCmd->faceCoefficient2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "faceCoefficient3           = %x\n", pRegCmd->faceCoefficient3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "faceCoefficient4           = %x\n", pRegCmd->faceCoefficient4);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "radiusCoefficient0         = %x\n", pRegCmd->radiusCoefficient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "radiusCoefficient1         = %x\n", pRegCmd->radiusCoefficient1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "radiusCoefficient2         = %x\n", pRegCmd->radiusCoefficient2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "radiusCoefficient3         = %x\n", pRegCmd->radiusCoefficient3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "radiusCoefficient4         = %x\n", pRegCmd->radiusCoefficient4);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrAnchorCoefficient0      = %x\n", pRegCmd->rnrAnchorCoefficient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrAnchorCoefficient1      = %x\n", pRegCmd->rnrAnchorCoefficient1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrAnchorCoefficient2      = %x\n", pRegCmd->rnrAnchorCoefficient2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrAnchorCoefficient3      = %x\n", pRegCmd->rnrAnchorCoefficient3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrAnchorCoefficient4      = %x\n", pRegCmd->rnrAnchorCoefficient4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrAnchorCoefficient5      = %x\n", pRegCmd->rnrAnchorCoefficient5);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrSlopeShiftCoefficient0  = %x\n", pRegCmd->rnrSlopeShiftCoefficient0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrSlopeShiftCoefficient1  = %x\n", pRegCmd->rnrSlopeShiftCoefficient1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrSlopeShiftCoefficient2  = %x\n", pRegCmd->rnrSlopeShiftCoefficient2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrSlopeShiftCoefficient3  = %x\n", pRegCmd->rnrSlopeShiftCoefficient3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrSlopeShiftCoefficient4  = %x\n", pRegCmd->rnrSlopeShiftCoefficient4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnrSlopeShiftCoefficient5  = %x\n", pRegCmd->rnrSlopeShiftCoefficient5);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "hnrCoefficient1            = %x\n", pRegCmd->hnrCoefficient1);

        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnr_r_squareCoefficient    = %x\n", pRegCmd->rnr_r_squareCoefficient);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "rnr_r_ScaleCoefficient     = %x\n", pRegCmd->rnr_r_ScaleCoefficient);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "lpf3ConfigCoefficient      = %x\n", pRegCmd->lpf3ConfigCoefficient);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "miscConfigCoefficient      = %x\n", pRegCmd->miscConfigCoefficient);
    }
}


CAMX_NAMESPACE_END