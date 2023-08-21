////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpshdr30titan480.cpp
/// @brief CAMXBPSHDR30TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpshdr30titan480.h"
#include "hdr30setting.h"
#include "camxiqinterface.h"
#include "titan480_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED
struct BPSHDR30RegCmd
{
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_0_CFG       configRegister0;        ///< HDR Config Register0
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_1_CFG       configRegister1;        ///< HDR Config Register1
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_2_CFG       configRegister2;        ///< HDR Config Register2
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_3_CFG       configRegister3;        ///< HDR Config Register3
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_4_CFG       configRegister4;        ///< HDR Config Register4
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_5_CFG       configRegister5;        ///< HDR Config Register5
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_REC_0_CFG   reconstructionConfig0;  ///< HDR reconstruction Config0
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_REC_1_CFG   reconstructionConfig1;  ///< HDR reconstruction Config1
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_REC_2_CFG   reconstructionConfig2;  ///< HDR reconstruction Config2
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_REC_3_CFG   reconstructionConfig3;  ///< HDR reconstruction Config3
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_REC_4_CFG   reconstructionConfig4;  ///< HDR reconstruction Config4
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_MAC_0_CFG   macConfigRegister0;     ///< HDR MAC Config0
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_MAC_1_CFG   macConfigRegister1;     ///< HDR MAC Config1
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_MAC_2_CFG   macConfigRegister2;     ///< HDR MAC Config2
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_MAC_3_CFG   macConfigRegister3;     ///< HDR MAC Config3
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_MAC_4_CFG   macConfigRegister4;     ///< HDR MAC Config4
    BPS_BPS_0_CLC_HDR_BINCORRECT_HDR_MAC_5_CFG   macConfigRegister5;     ///< HDR MAC Config5
    BPS_BPS_0_CLC_HDR_BINCORRECT_BIN_CORR_0_CFG  bcconfigRegister0;      ///< BC correction Config0
    BPS_BPS_0_CLC_HDR_BINCORRECT_BIN_CORR_1_CFG  bcconfigRegister1;      ///< BC correction Config1
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSHDR30RegLengthDWord = sizeof(BPSHDR30RegCmd) / sizeof(UINT32);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30Titan480::BPSHDR30Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHDR30Titan480::BPSHDR30Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30Titan480::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30Titan480::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSHDR30Titan480* pHWSetting = CAMX_NEW BPSHDR30Titan480;

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
/// BPSHDR30Titan480::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30Titan480::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pRegCmd = CAMX_CALLOC(sizeof(BPSHDR30RegCmd));

    if (NULL != m_pRegCmd)
    {
        SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSHDR30RegLengthDWord));
    }
    else
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for register configuration!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result        = CamxResultSuccess;
    ISPInputData* pInputData    = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*    pCmdBuffer    = NULL;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_HDR_BINCORRECT_HDR_0_CFG,
                                              BPSHDR30RegLengthDWord,
                                              static_cast<UINT32*>(m_pRegCmd));

        CAMX_ASSERT(CamxResultSuccess == result);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30Titan480::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30Titan480::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*   pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings*  pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSHDR30RegCmd* pRegCmd        = static_cast<BPSHDR30RegCmd*>(m_pRegCmd);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    // Store HDR enable state for PDPC
    pInputData->triggerData.zzHDRModeEnable                       = pRegCmd->configRegister0.bitfields.HDR_ENABLE;


    pBPSIQSettings->hdrBinCorrectParameters_480.hdrExpRatio       = pRegCmd->configRegister0.bitfields.HDR_EXP_RATIO;
    pBPSIQSettings->hdrBinCorrectParameters_480.hdrZrecFirstRbExp = pRegCmd->configRegister0.bitfields.HDR_ZREC_FIRST_RB_EXP;
    pBPSIQSettings->hdrBinCorrectParameters_480.hdrZrecPattern    = pRegCmd->configRegister0.bitfields.HDR_ZREC_PATTERN;
    pBPSIQSettings->hdrBinCorrectParameters_480.hdrLsbAligh       = pRegCmd->configRegister0.bitfields.HDR_LSB_ALIGN;
    pBPSIQSettings->hdrBinCorrectParameters_480.hdrLinearMode     = pRegCmd->configRegister0.bitfields.HDR_LINEAR_MODE;
    pBPSIQSettings->hdrBinCorrectParameters_480.hdrEnable         = pRegCmd->configRegister0.bitfields.HDR_ENABLE;
    pBPSIQSettings->hdrBinCorrectParameters_480.hdrMacDilation    = pRegCmd->macConfigRegister4.bitfields.HDR_MAC_DILATION;
    pBPSIQSettings->hdrBinCorrectParameters_480.hdrMacLinearMode  = pRegCmd->configRegister0.bitfields.HDR_LINEAR_MODE;
    pBPSIQSettings->hdrBinCorrectParameters_480.binCorrEn         = pRegCmd->bcconfigRegister0.bitfields.BIN_CORR_EN;

    pBPSIQSettings->hdrBinCorrectParameters_480.moduleCfg.EN =
        pRegCmd->configRegister0.bitfields.HDR_ENABLE | pRegCmd->bcconfigRegister0.bitfields.BIN_CORR_EN;

    // Based on the HPG, the HDR and BC are mutually exclusive. here we just pick HDR as the higher prority one
    if (1 == pRegCmd->configRegister0.bitfields.HDR_ENABLE && 1 == pRegCmd->bcconfigRegister0.bitfields.BIN_CORR_EN)
    {
        CAMX_LOG_WARN(CamxLogGroupBPS, "Both HDR and BC is enabled. we need to disable the BC");
        pBPSIQSettings->hdrBinCorrectParameters_480.binCorrEn = 0;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "HDR30:%d BC10:%d moduleEnable:%d",
        pBPSIQSettings->hdrBinCorrectParameters_480.hdrEnable,
        pBPSIQSettings->hdrBinCorrectParameters_480.binCorrEn,
        pBPSIQSettings->hdrBinCorrectParameters_480.moduleCfg.EN);

    CAMX_UNREFERENCED_PARAM(moduleEnable);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSHDR30RegCmd*    pRegCmd            = static_cast<BPSHDR30RegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSHDR30RegCmd) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSHDRData.HDRConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata480.BPSHDRData.HDRConfig, pRegCmd, sizeof(BPSHDR30RegCmd));

        if ((TRUE == pBPSIQSettings->hdrBinCorrectParameters_480.moduleCfg.EN) &&
            (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSHDR30Register,
                DebugDataTagType::UInt32,
                CAMX_ARRAY_SIZE(pBPSTuningMetadata->BPSTuningMetadata480.BPSHDRData.HDRConfig),
                &pBPSTuningMetadata->BPSTuningMetadata480.BPSHDRData.HDRConfig,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSHDRData.HDRConfig));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR30Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult              result      = CamxResultSuccess;
    HDR30BC10UnpackedField* pData       = static_cast<HDR30BC10UnpackedField*>(pInput);
    BPSHDR30RegCmd*         pRegCmd     = static_cast<BPSHDR30RegCmd*>(m_pRegCmd);
    HDR30UnpackedField*     pDataHdr30  = NULL;
    BC10UnpackedField*      pDataBc10   = NULL;

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        pDataHdr30 = pData->pUnpackedRegisterDataHDR30;
        pDataBc10  = pData->pUnpackedRegisterDataBC10;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Invalid pointer");
    }

    if (NULL != pRegCmd)
    {
        if (NULL != pDataHdr30)
        {
            pRegCmd->configRegister0.bitfields.HDR_ENABLE = pDataHdr30->enable;
            pRegCmd->configRegister0.bitfields.HDR_EXP_RATIO = pDataHdr30->hdr_exp_ratio;
            pRegCmd->configRegister0.bitfields.HDR_LINEAR_MODE = pDataHdr30->hdr_linear_mode;
            pRegCmd->configRegister0.bitfields.HDR_LSB_ALIGN = pDataHdr30->hdr_lsb_align;
            pRegCmd->configRegister0.bitfields.HDR_ZREC_FIRST_RB_EXP = pDataHdr30->hdr_zrec_first_rb_exp;
            pRegCmd->configRegister0.bitfields.HDR_ZREC_PATTERN = pDataHdr30->hdr_zrec_pattern;
            pRegCmd->configRegister1.bitfields.HDR_BG_WB_GAIN_RATIO = pDataHdr30->hdr_bg_wb_gain_ratio;
            pRegCmd->configRegister1.bitfields.HDR_RG_WB_GAIN_RATIO = pDataHdr30->hdr_rg_wb_gain_ratio;
            pRegCmd->configRegister2.bitfields.HDR_GR_WB_GAIN_RATIO = pDataHdr30->hdr_gr_wb_gain_ratio;
            pRegCmd->configRegister3.bitfields.HDR_GB_WB_GAIN_RATIO = pDataHdr30->hdr_gb_wb_gain_ratio;
            pRegCmd->configRegister4.bitfields.HDR_BLACK_IN = pDataHdr30->hdr_black_in;
            pRegCmd->configRegister4.bitfields.HDR_LONG_EXP_BLACK_IN = pDataHdr30->hdr_long_exp_black_in;
            pRegCmd->configRegister5.bitfields.HDR_EXP_RATIO_RECIP = pDataHdr30->hdr_exp_ratio_recip;
            pRegCmd->configRegister5.bitfields.HDR_LONG_EXP_SATURATION = pDataHdr30->hdr_long_exp_saturation;
            pRegCmd->reconstructionConfig0.bitfields.HDR_REC_G_GRAD_DTH_LOG2 = pDataHdr30->hdr_rec_g_grad_dth_log2;
            pRegCmd->reconstructionConfig0.bitfields.HDR_REC_G_GRAD_TH1 = pDataHdr30->hdr_rec_g_grad_th1;
            pRegCmd->reconstructionConfig0.bitfields.HDR_REC_RB_GRAD_DTH_LOG2 = pDataHdr30->hdr_rec_rb_grad_dth_log2;
            pRegCmd->reconstructionConfig0.bitfields.HDR_REC_RB_GRAD_TH1 = pDataHdr30->hdr_rec_rb_grad_th1;
            pRegCmd->reconstructionConfig1.bitfields.HDR_REC_HL_DETAIL_NEGATIVE_W = pDataHdr30->hdr_rec_hl_detail_negative_w;
            pRegCmd->reconstructionConfig1.bitfields.HDR_REC_HL_DETAIL_POSITIVE_W = pDataHdr30->hdr_rec_hl_detail_positive_w;
            pRegCmd->reconstructionConfig1.bitfields.HDR_REC_HL_DETAIL_TH_W = pDataHdr30->hdr_rec_hl_detail_th_w;
            pRegCmd->reconstructionConfig1.bitfields.HDR_REC_HL_MOTION_TH_LOG2 = pDataHdr30->hdr_rec_hl_motion_th_log2;
            pRegCmd->reconstructionConfig2.bitfields.HDR_REC_EDGE_SCALE = pDataHdr30->hdr_rec_edge_scale;
            pRegCmd->reconstructionConfig2.bitfields.HDR_REC_EDGE_SHORT_EXP_W = pDataHdr30->hdr_rec_edge_short_exp_w;
            pRegCmd->reconstructionConfig3.bitfields.HDR_REC_BLENDING_W_MAX = pDataHdr30->hdr_rec_blending_w_max;
            pRegCmd->reconstructionConfig3.bitfields.HDR_REC_BLENDING_W_MIN = pDataHdr30->hdr_rec_blending_w_min;
            pRegCmd->reconstructionConfig3.bitfields.HDR_REC_EDGE_DTH_LOG2 = pDataHdr30->hdr_rec_edge_dth_log2;
            pRegCmd->reconstructionConfig3.bitfields.HDR_REC_EDGE_TH1 = pDataHdr30->hdr_rec_edge_th1;
            pRegCmd->reconstructionConfig4.bitfields.HDR_REC_DETAIL_CORING_DTH_LOG2 =
                pDataHdr30->hdr_rec_detail_coring_dth_log2;
            pRegCmd->reconstructionConfig4.bitfields.HDR_REC_DETAIL_CORING_STRENGTH =
                pDataHdr30->hdr_rec_detail_coring_strength;
            pRegCmd->reconstructionConfig4.bitfields.HDR_REC_DETAIL_CORING_TH2 = pDataHdr30->hdr_rec_detail_coring_th2;
            pRegCmd->reconstructionConfig4.bitfields.HDR_REC_DETAIL_TH_W = pDataHdr30->hdr_rec_detail_th_w;
            pRegCmd->macConfigRegister0.bitfields.HDR_MAC_HFS_ACT_DTH_LOG2 = pDataHdr30->hdr_mac_hfs_act_dth_log2;
            pRegCmd->macConfigRegister0.bitfields.HDR_MAC_HFS_ACT_TH2 = pDataHdr30->hdr_mac_hfs_act_th2;
            pRegCmd->macConfigRegister0.bitfields.HDR_MAC_HFS_ACT_WEIGHT = pDataHdr30->hdr_mac_hfs_act_weight;
            pRegCmd->macConfigRegister1.bitfields.HDR_MAC_STATIC_DTH_LOG2 = pDataHdr30->hdr_mac_static_dth_log2;
            pRegCmd->macConfigRegister1.bitfields.HDR_MAC_STATIC_TH2 = pDataHdr30->hdr_mac_static_th2;
            pRegCmd->macConfigRegister2.bitfields.HDR_MAC_HIGHLIGHT_DTH_LOG2 = pDataHdr30->hdr_mac_highlight_dth_log2;
            pRegCmd->macConfigRegister2.bitfields.HDR_MAC_HIGHLIGHT_STRENGTH = pDataHdr30->hdr_mac_highlight_strength;
            pRegCmd->macConfigRegister3.bitfields.HDR_MAC_DARK_TH2 = pDataHdr30->hdr_mac_dark_th2;
            pRegCmd->macConfigRegister3.bitfields.HDR_MAC_LOWLIGHT_DTH_LOG2 = pDataHdr30->hdr_mac_lowlight_dth_log2;
            pRegCmd->macConfigRegister3.bitfields.HDR_MAC_LOWLIGHT_STRENGTH = pDataHdr30->hdr_mac_lowlight_strength;
            pRegCmd->macConfigRegister4.bitfields.HDR_MAC_DILATION = pDataHdr30->hdr_mac_dilation;
            pRegCmd->macConfigRegister4.bitfields.HDR_MAC_MOTION_DT0 = pDataHdr30->hdr_mac_motion_dt0;
            pRegCmd->macConfigRegister4.bitfields.HDR_MAC_MOTION_SCALE = pDataHdr30->hdr_mac_motion_scale;
            pRegCmd->macConfigRegister4.bitfields.HDR_MAC_MOTION_STRICTNESS = pDataHdr30->hdr_mac_motion_strictness;
            pRegCmd->macConfigRegister5.bitfields.HDR_MAC_MOTION_DTH_LOG2 = pDataHdr30->hdr_mac_motion_dth_log2;
            pRegCmd->macConfigRegister5.bitfields.HDR_MAC_MOTION_G2_W_MAX = pDataHdr30->hdr_mac_motion_g2_w_max;
            pRegCmd->macConfigRegister5.bitfields.HDR_MAC_MOTION_G2_W_MIN = pDataHdr30->hdr_mac_motion_g2_w_min;
            pRegCmd->macConfigRegister5.bitfields.HDR_MAC_MOTION_TH1 = pDataHdr30->hdr_mac_motion_th1;
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input HDR30 param is NULL");
        }

        if (NULL != pDataBc10)
        {
            pRegCmd->bcconfigRegister0.bitfields.BIN_CORR_EN = pDataBc10->enable;
            pRegCmd->bcconfigRegister0.bitfields.VER_BIN_CORR_W1 = pDataBc10->binCorrVerW1;
            pRegCmd->bcconfigRegister0.bitfields.VER_BIN_CORR_W2 = pDataBc10->binCorrVerW2;
            pRegCmd->bcconfigRegister1.bitfields.HOR_BIN_CORR_W1 = pDataBc10->binCorrHorW1;
            pRegCmd->bcconfigRegister1.bitfields.HOR_BIN_CORR_W2 = pDataBc10->binCorrHorW2;
        }
        else
        {
            result = CamxResultEInvalidArg;
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input BC10 param is NULL");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input RegCmd param is NULL");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30Titan480::~BPSHDR30Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHDR30Titan480::~BPSHDR30Titan480()
{
    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR30Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHDR30Titan480::DumpRegConfig()
{
    BPSHDR30RegCmd* pRegCmd = static_cast<BPSHDR30RegCmd*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister0       0x%x", pRegCmd->configRegister0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister1       0x%x", pRegCmd->configRegister1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister2       0x%x", pRegCmd->configRegister2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister3       0x%x", pRegCmd->configRegister3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister4       0x%x", pRegCmd->configRegister4.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister5       0x%x", pRegCmd->configRegister5.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 reconstructionConfig0 0x%x", pRegCmd->reconstructionConfig0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 reconstructionConfig1 0x%x", pRegCmd->reconstructionConfig1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 reconstructionConfig2 0x%x", pRegCmd->reconstructionConfig2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 reconstructionConfig3 0x%x", pRegCmd->reconstructionConfig3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 reconstructionConfig4 0x%x", pRegCmd->reconstructionConfig4.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister0    0x%x", pRegCmd->macConfigRegister0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister1    0x%x", pRegCmd->macConfigRegister1.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister2    0x%x", pRegCmd->macConfigRegister2.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister3    0x%x", pRegCmd->macConfigRegister3.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister4    0x%x", pRegCmd->macConfigRegister4.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister5    0x%x", pRegCmd->macConfigRegister5.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "BC10 configRegister0 0x%x", pRegCmd->configRegister0.u32All);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "BC10 configRegister1 0x%x", pRegCmd->configRegister1.u32All);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input param is NULL pRegCmd=0x%p", pRegCmd);
    }
}

CAMX_NAMESPACE_END
