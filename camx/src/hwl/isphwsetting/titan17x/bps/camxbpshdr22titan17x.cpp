////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpshdr22titan17x.cpp
/// @brief CAMXBPSHDR22TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpshdr22titan17x.h"
#include "hdr22setting.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief HDR Reconstruct register Configuration
struct BPSHDR22ReconstructConfig
{
    BPS_BPS_0_CLC_HDR_RECON_HDR_0_CFG   config0;             ///< HDR reconstruct config 0
    BPS_BPS_0_CLC_HDR_RECON_HDR_1_CFG   config1;             ///< HDR reconstruct config 1
    BPS_BPS_0_CLC_HDR_RECON_HDR_2_CFG   config2;             ///< HDR reconstruct config 2
    BPS_BPS_0_CLC_HDR_RECON_HDR_3_CFG   config3;             ///< HDR reconstruct config 3
    BPS_BPS_0_CLC_HDR_RECON_IHDR_0_CFG  interlacedConfig0;   ///< interlaced HDR reconstruct config 0
    BPS_BPS_0_CLC_HDR_RECON_IHDR_1_CFG  interlacedConfig1;   ///< interlaced HDR reconstruct config 1
    BPS_BPS_0_CLC_HDR_RECON_IHDR_2_CFG  interlacedConfig2;   ///< interlaced HDR reconstruct config 2
    BPS_BPS_0_CLC_HDR_RECON_ZHDR_0_CFG  zigzagConfig0;       ///< zigzag HDR reconstruct config 0
    BPS_BPS_0_CLC_HDR_RECON_ZHDR_1_CFG  zigzagConfig1;       ///< zigzag HDR reconstruct config 1
} CAMX_PACKED;

/// @brief HDR MAC register Configuration
struct BPSHDR22MACConfig
{
    BPS_BPS_0_CLC_HDR_MAC_HDR_MAC_0_CFG config0;  ///< HDR reconstruct config 0
    BPS_BPS_0_CLC_HDR_MAC_HDR_MAC_1_CFG config1;  ///< HDR reconstruct config 1
    BPS_BPS_0_CLC_HDR_MAC_HDR_MAC_2_CFG config2;  ///< HDR reconstruct config 2
    BPS_BPS_0_CLC_HDR_MAC_HDR_MAC_3_CFG config3;  ///< HDR reconstruct config 3
    BPS_BPS_0_CLC_HDR_MAC_HDR_MAC_4_CFG config4;  ///< interlaced HDR reconstruct config 0
    BPS_BPS_0_CLC_HDR_MAC_HDR_MAC_5_CFG config5;  ///< interlaced HDR reconstruct config 1
    BPS_BPS_0_CLC_HDR_MAC_HDR_MAC_6_CFG config6;  ///< interlaced HDR reconstruct config 2
} CAMX_PACKED;

/// @brief HDR Reconstruct and MAC module Configuration
struct BPSHDR22ModuleConfig
{
    BPS_BPS_0_CLC_HDR_MAC_MODULE_CFG    macModuleConfig;    ///< MAC Module Config
    BPS_BPS_0_CLC_HDR_RECON_MODULE_CFG  reconModuleConfig;  ///< Recon Module Config
} CAMX_PACKED;

CAMX_END_PACKED

/// @brief HDR Reconstruct and MAC register Configuration
struct BPSHDR22RegCmd
{
    BPSHDR22ReconstructConfig reconstructConfig;  ///< HDR reconstruct config
    BPSHDR22MACConfig         MACConfig;          ///< HDR MAC config
};

static const UINT32 BPSHDR22RegReconstructConfigLengthDWord = sizeof(BPSHDR22ReconstructConfig) / RegisterWidthInBytes;
static const UINT32 BPSHDR22RegMacConfigLengthDWord         = sizeof(BPSHDR22MACConfig) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22Titan17x::BPSHDR22Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHDR22Titan17x::BPSHDR22Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSHDR22Titan17x* pHWSetting = CAMX_NEW BPSHDR22Titan17x;

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
/// BPSHDR22Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pModuleConfig = CAMX_CALLOC(sizeof(BPSHDR22ModuleConfig));

    if (NULL == m_pModuleConfig)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for module configuration!");
    }
    else
    {
        m_pRegCmd = CAMX_CALLOC(sizeof(BPSHDR22RegCmd));

        if (NULL != m_pRegCmd)
        {
            SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSHDR22RegReconstructConfigLengthDWord) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSHDR22RegMacConfigLengthDWord));
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
// BPSHDR22Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CamxResult      result          = CamxResultSuccess;
    ISPInputData*   pInputData      = static_cast<ISPInputData*>(pSettingData);
    CmdBuffer*      pCmdBuffer      = NULL;
    BPSHDR22RegCmd* pRegCmd         = static_cast<BPSHDR22RegCmd*>(m_pRegCmd);

    if ((NULL != pInputData) && (NULL != pInputData->pCmdBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_HDR_RECON_HDR_0_CFG,
                                              BPSHDR22RegReconstructConfigLengthDWord,
                                              reinterpret_cast<UINT32*>(&pRegCmd->reconstructConfig));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regBPS_BPS_0_CLC_HDR_MAC_HDR_MAC_0_CFG,
                                                  BPSHDR22RegMacConfigLengthDWord,
                                                  reinterpret_cast<UINT32*>(&pRegCmd->MACConfig));
        }
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
// BPSHDR22Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    BPSHDR22ModuleConfig* pModuleCfg = static_cast<BPSHDR22ModuleConfig*>(m_pModuleConfig);
    BPSHDR22RegCmd*       pRegCmd    = static_cast<BPSHDR22RegCmd*>(m_pRegCmd);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    // Store HDR enable state for PDPC
    pInputData->triggerData.zzHDRModeEnable = moduleEnable;

    pBPSIQSettings->hdrMacParameters.moduleCfg.EN        = pModuleCfg->macModuleConfig.bitfields.EN;
    pBPSIQSettings->hdrMacParameters.dilation            = pRegCmd->MACConfig.config4.bitfields.MAC_DILATION;
    pBPSIQSettings->hdrMacParameters.smoothFilterEnable  = pRegCmd->MACConfig.config6.bitfields.MAC_SMOOTH_ENABLE;
    pBPSIQSettings->hdrMacParameters.linearMode          = pRegCmd->MACConfig.config6.bitfields.MAC_LINEAR_MODE;

    pBPSIQSettings->hdrReconParameters.moduleCfg.EN      = pModuleCfg->reconModuleConfig.bitfields.EN;
    pBPSIQSettings->hdrReconParameters.hdrMode           = 1;
    pBPSIQSettings->hdrReconParameters.zzHdrShift        = pRegCmd->reconstructConfig.zigzagConfig0.bitfields.ZREC_FIRST_RB_EXP;
    pBPSIQSettings->hdrReconParameters.zzHdrPattern      = pRegCmd->reconstructConfig.zigzagConfig0.bitfields.ZREC_PATTERN;
    pBPSIQSettings->hdrReconParameters.zzHdrPrefilterTap = pRegCmd->reconstructConfig.zigzagConfig0.bitfields.ZREC_PREFILT_TAP0;
    pBPSIQSettings->hdrReconParameters.linearMode        = pRegCmd->reconstructConfig.config0.bitfields.RECON_LINEAR_MODE;
    pBPSIQSettings->hdrReconParameters.hdrExpRatio       = pRegCmd->reconstructConfig.config0.bitfields.EXP_RATIO;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BPSHDR22RegCmd*    pRegCmd            = static_cast<BPSHDR22RegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSHDR22ReconstructConfig) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSHDRData.HDRReconConfig));

        CAMX_STATIC_ASSERT(sizeof(BPSHDR22MACConfig) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSHDRData.HDRMACConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSHDRData.HDRReconConfig,
                        &pRegCmd->reconstructConfig,
                        sizeof(BPSHDR22ReconstructConfig));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSHDRData.HDRMACConfig,
                        &pRegCmd->MACConfig,
                        sizeof(BPSHDR22MACConfig));

        if (TRUE == pInputData->triggerData.zzHDRModeEnable)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSHDR22Register,
                DebugDataTagType::TuningBPSHDR22Config,
                1,
                &pBPSTuningMetadata->BPSTuningMetadata17x.BPSHDRData,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSHDRData));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDR22Titan17x::PackIQRegisterSetting(
    VOID*  pInputData,
    VOID*  pOutputData)
{
    CamxResult                    result            = CamxResultSuccess;
    HDR22UnpackedField*           pData             = static_cast<HDR22UnpackedField*>(pInputData);
    BPSHDR22RegCmd*               pRegCmd           = static_cast<BPSHDR22RegCmd*>(m_pRegCmd);
    BPSHDR22ModuleConfig*         pModuleCfg        = static_cast<BPSHDR22ModuleConfig*>(m_pModuleConfig);

    CAMX_UNREFERENCED_PARAM(pOutputData);

    if (NULL != pData)
    {
        pRegCmd->MACConfig.config0.bitfields.EXP_RATIO                  = pData->hdr_exp_ratio;
        pRegCmd->MACConfig.config0.bitfields.EXP_RATIO_RECIP            = pData->hdr_exp_ratio_recip;
        pRegCmd->MACConfig.config1.bitfields.BLK_IN                     = pData->hdr_black_in;
        pRegCmd->MACConfig.config1.bitfields.GB_WB_GAIN_RATIO           = pData->hdr_gb_wb_gain_ratio;
        pRegCmd->MACConfig.config2.bitfields.BLK_OUT                    = pData->hdr_black_out;
        pRegCmd->MACConfig.config2.bitfields.GR_WB_GAIN_RATIO           = pData->hdr_gr_wb_gain_ratio;
        pRegCmd->MACConfig.config3.bitfields.MAC_MOTION_0_TH1           = pData->hdr_mac_motion0_th1;
        pRegCmd->MACConfig.config3.bitfields.MAC_MOTION_0_TH2           = pData->hdr_mac_motion0_th2;
        pRegCmd->MACConfig.config3.bitfields.MAC_SQRT_ANALOG_GAIN       = pData->hdr_mac_sqrt_analog_gain;
        pRegCmd->MACConfig.config4.bitfields.MAC_DILATION               = pData->hdr_mac_dilation;
        pRegCmd->MACConfig.config4.bitfields.MAC_LOW_LIGHT_DTH_LOG2     = pData->hdr_mac_lowlight_dth_log2;
        pRegCmd->MACConfig.config4.bitfields.MAC_LOW_LIGHT_STRENGTH     = pData->hdr_mac_lowlight_strength;
        pRegCmd->MACConfig.config4.bitfields.MAC_MOTION_0_DT0           = pData->hdr_mac_motion0_dt0;
        pRegCmd->MACConfig.config4.bitfields.MAC_MOTION_STRENGTH        = pData->hdr_mac_motion_strength;
        pRegCmd->MACConfig.config5.bitfields.MAC_HIGH_LIGHT_TH1         = pData->hdr_mac_hilight_th1;
        pRegCmd->MACConfig.config5.bitfields.MAC_LOW_LIGHT_TH1          = pData->hdr_mac_lowlight_th1;
        pRegCmd->MACConfig.config6.bitfields.MAC_HIGH_LIGHT_DTH_LOG2    = pData->hdr_mac_hilight_dth_log2;
        pRegCmd->MACConfig.config6.bitfields.MAC_LINEAR_MODE            = pData->hdr_mac_linear_mode;
        pRegCmd->MACConfig.config6.bitfields.MAC_SMOOTH_DTH_LOG2        = pData->hdr_mac_smooth_dth_log2;
        pRegCmd->MACConfig.config6.bitfields.MAC_SMOOTH_ENABLE          = pData->hdr_mac_smooth_filter_en;
        pRegCmd->MACConfig.config6.bitfields.MAC_SMOOTH_TAP0            = pData->hdr_mac_smooth_tap0;
        pRegCmd->MACConfig.config6.bitfields.MAC_SMOOTH_TH1             = pData->hdr_mac_smooth_th1;
        pRegCmd->MACConfig.config6.bitfields.MSB_ALIGNED                = pData->hdr_MSB_align;

        pRegCmd->reconstructConfig.config0.bitfields.EXP_RATIO              = pData->hdr_exp_ratio;
        pRegCmd->reconstructConfig.config0.bitfields.RECON_LINEAR_MODE      = pData->hdr_rec_linear_mode;
        pRegCmd->reconstructConfig.config0.bitfields.ZREC_ENABLE            = pData->hdr_zrec_en;
        pRegCmd->reconstructConfig.config1.bitfields.RG_WB_GAIN_RATIO       = pData->hdr_rg_wb_gain_ratio;
        pRegCmd->reconstructConfig.config2.bitfields.BG_WB_GAIN_RATIO       = pData->hdr_bg_wb_gain_ratio;
        pRegCmd->reconstructConfig.config3.bitfields.BLK_IN                 = pData->hdr_black_in;

        pRegCmd->reconstructConfig.interlacedConfig0.bitfields.RECON_H_EDGE_DTH_LOG2    = pData->hdr_rec_h_edge_dth_log2;
        pRegCmd->reconstructConfig.interlacedConfig0.bitfields.RECON_H_EDGE_TH1         = pData->hdr_rec_h_edge_th1;
        pRegCmd->reconstructConfig.interlacedConfig0.bitfields.RECON_MOTION_DTH_LOG2    = pData->hdr_rec_motion_dth_log2;
        pRegCmd->reconstructConfig.interlacedConfig0.bitfields.RECON_MOTION_TH1         = pData->hdr_rec_motion_th1;
        pRegCmd->reconstructConfig.interlacedConfig1.bitfields.RECON_DARK_DTH_LOG2      = pData->hdr_rec_dark_dth_log2;
        pRegCmd->reconstructConfig.interlacedConfig1.bitfields.RECON_DARK_TH1           = pData->hdr_rec_dark_th1;
        pRegCmd->reconstructConfig.interlacedConfig1.bitfields.RECON_EDGE_LPF_TAP0      = pData->hdr_rec_edge_lpf_tap0;
        pRegCmd->reconstructConfig.interlacedConfig1.bitfields.RECON_FIRST_FIELD        = pData->hdr_first_field;
        pRegCmd->reconstructConfig.interlacedConfig1.bitfields.RECON_FLAT_REGION_TH     = pData->hdr_rec_flat_region_th;
        pRegCmd->reconstructConfig.interlacedConfig2.bitfields.RECON_MIN_FACTOR         = pData->hdr_rec_min_factor;
        pRegCmd->reconstructConfig.zigzagConfig0.bitfields.ZREC_FIRST_RB_EXP            = pData->hdr_zrec_first_rb_exp;
        pRegCmd->reconstructConfig.zigzagConfig0.bitfields.ZREC_PATTERN                 = pData->hdr_zrec_pattern;
        pRegCmd->reconstructConfig.zigzagConfig0.bitfields.ZREC_PREFILT_TAP0            = pData->hdr_zrec_prefilt_tap0;
        pRegCmd->reconstructConfig.zigzagConfig1.bitfields.ZREC_G_DTH_LOG2              = pData->hdr_zrec_g_grad_dth_log2;
        pRegCmd->reconstructConfig.zigzagConfig1.bitfields.ZREC_G_GRAD_TH1              = pData->hdr_zrec_g_grad_th1;
        pRegCmd->reconstructConfig.zigzagConfig1.bitfields.ZREC_RB_DTH_LOG2             = pData->hdr_zrec_rb_grad_dth_log2;
        pRegCmd->reconstructConfig.zigzagConfig1.bitfields.ZREC_RB_GRAD_TH1             = pData->hdr_zrec_rb_grad_th1;

        pModuleCfg->macModuleConfig.bitfields.EN   = pData->hdr_mac_en;
        pModuleCfg->reconModuleConfig.bitfields.EN = pData->hdr_recon_en;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "%s: Input data is NULL ", __FUNCTION__);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22Titan17x::~BPSHDR22Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHDR22Titan17x::~BPSHDR22Titan17x()
{
    CAMX_FREE(m_pModuleConfig);
    m_pModuleConfig = NULL;

    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDR22Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHDR22Titan17x::DumpRegConfig()
{
    BPSHDR22RegCmd* pRegCmd = static_cast<BPSHDR22RegCmd*>(m_pRegCmd);

    if (NULL != pRegCmd)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "reconstructConfig.config0 = %x \n", pRegCmd->reconstructConfig.config0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "reconstructConfig.config1 = %x \n", pRegCmd->reconstructConfig.config1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "reconstructConfig.config2 = %x \n", pRegCmd->reconstructConfig.config2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "reconstructConfig.config3 = %x \n", pRegCmd->reconstructConfig.config3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "interlacedConfig0         = %x \n", pRegCmd->reconstructConfig.interlacedConfig0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "interlacedConfig1         = %x \n", pRegCmd->reconstructConfig.interlacedConfig1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "interlacedConfig2         = %x \n", pRegCmd->reconstructConfig.interlacedConfig2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "zigzagConfig0             = %x \n", pRegCmd->reconstructConfig.zigzagConfig0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "zigzagConfig1             = %x \n", pRegCmd->reconstructConfig.zigzagConfig1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MACConfig.config0         = %x \n", pRegCmd->MACConfig.config0);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MACConfig.config1         = %x \n", pRegCmd->MACConfig.config1);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MACConfig.config2         = %x \n", pRegCmd->MACConfig.config2);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MACConfig.config3         = %x \n", pRegCmd->MACConfig.config3);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MACConfig.config4         = %x \n", pRegCmd->MACConfig.config4);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MACConfig.config5         = %x \n", pRegCmd->MACConfig.config5);
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "MACConfig.config6         = %x \n", pRegCmd->MACConfig.config6);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input param is NULL pRegCmd=0x%p", pRegCmd);
    }
}

CAMX_NAMESPACE_END
