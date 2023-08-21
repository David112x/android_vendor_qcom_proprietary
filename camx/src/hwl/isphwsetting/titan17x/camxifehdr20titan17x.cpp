////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdr20titan17x.cpp
/// @brief CAMXIFEHDR20TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifehdr20titan17x.h"
#include "ifehdr20setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR20Titan17x::IFEHDR20Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDR20Titan17x::IFEHDR20Titan17x()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEHDR20RegLengthDWord1) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEHDR20RegLengthDWord2) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEHDR20RegLengthDWord3));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR20Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR20Titan17x::CreateCmdList(
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
                                              regIFE_IFE_0_VFE_HDR_CFG_0,
                                              IFEHDR20RegLengthDWord1,
                                              reinterpret_cast<UINT32*>(&m_regCmd.m_regCmd1));
        CAMX_ASSERT(CamxResultSuccess == result);

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_HDR_CFG_5,
                                              IFEHDR20RegLengthDWord2,
                                              reinterpret_cast<UINT32*>(&m_regCmd.m_regCmd2));
        CAMX_ASSERT(CamxResultSuccess == result);

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_HDR_RECON_CFG_5,
                                              IFEHDR20RegLengthDWord3,
                                              reinterpret_cast<UINT32*>(&m_regCmd.m_regCmd3));
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
// IFEHDR20Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR20Titan17x::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEHDR20RegCmd1) == sizeof(pIFETuningMetadata->metadata17x.IFEHDRData.HDRConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEHDRData.HDRConfig1,
                      &m_regCmd.m_regCmd1,
                      sizeof(IFEHDR20RegCmd1));

        CAMX_STATIC_ASSERT(sizeof(IFEHDR20RegCmd2) == sizeof(pIFETuningMetadata->metadata17x.IFEHDRData.HDRConfig2));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEHDRData.HDRConfig2,
                      &m_regCmd.m_regCmd2,
                      sizeof(IFEHDR20RegCmd2));

        CAMX_STATIC_ASSERT(sizeof(IFEHDR20RegCmd3) == sizeof(pIFETuningMetadata->metadata17x.IFEHDRData.HDRConfig3));
        Utils::Memcpy(&pIFETuningMetadata->metadata17x.IFEHDRData.HDRConfig3,
                      &m_regCmd.m_regCmd3,
                      sizeof(IFEHDR20RegCmd3));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR20Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR20Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult          result    = CamxResultSuccess;
    HDR20UnpackedField* pData     = static_cast<HDR20UnpackedField*>(pInput);
    IFEHDR20RegCmd1*    pRegCmd1  = NULL;
    IFEHDR20RegCmd2*    pRegCmd2  = NULL;
    IFEHDR20RegCmd3*    pRegCmd3  = NULL;

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        pRegCmd1 = &(m_regCmd.m_regCmd1);
        pRegCmd2 = &(m_regCmd.m_regCmd2);
        pRegCmd3 = &(m_regCmd.m_regCmd3);

        pRegCmd1->configRegister0.bitfields.EXP_RATIO                    = pData->hdr_exp_ratio;
        pRegCmd1->configRegister0.bitfields.RECON_FIRST_FIELD            = pData->hdr_first_field;

        pRegCmd1->configRegister1.bitfields.RG_WB_GAIN_RATIO             = pData->hdr_rg_wb_gain_ratio;
        pRegCmd1->configRegister2.bitfields.BG_WB_GAIN_RATIO             = pData->hdr_bg_wb_gain_ratio;
        pRegCmd1->configRegister3.bitfields.GR_WB_GAIN_RATIO             = pData->hdr_gr_wb_gain_ratio;
        pRegCmd1->configRegister4.bitfields.GB_WB_GAIN_RATIO             = pData->hdr_gb_wb_gain_ratio;

        pRegCmd1->macConfigRegister0.bitfields.MAC_MOTION_0_TH1          = pData->hdr_mac_motion0_th1;
        pRegCmd1->macConfigRegister0.bitfields.MAC_MOTION_0_TH2          = pData->hdr_mac_motion0_th2;
        pRegCmd1->macConfigRegister0.bitfields.R_MAC_MOTION_0_TH1        = pData->hdr_mac_motion0_th1;

        pRegCmd1->macConfigRegister1.bitfields.MAC_DILATION               = pData->hdr_mac_dilation;
        pRegCmd1->macConfigRegister1.bitfields.MAC_SQRT_ANALOG_GAIN       = pData->hdr_mac_sqrt_analog_gain;
        pRegCmd1->macConfigRegister1.bitfields.R_MAC_MOTION_0_TH2         = pData->hdr_mac_motion0_th2;
        pRegCmd1->macConfigRegister1.bitfields.R_MAC_SQRT_ANALOG_GAIN     = pData->hdr_mac_sqrt_analog_gain;

        pRegCmd1->macConfigRegister2.bitfields.MAC_MOTION_0_DT0           = pData->hdr_mac_motion0_dt0;
        pRegCmd1->macConfigRegister2.bitfields.MAC_MOTION_STRENGTH        = pData->hdr_mac_motion_strength;
        pRegCmd1->macConfigRegister2.bitfields.R_MAC_MOTION_0_DT0         = pData->hdr_mac_motion0_dt0;
        pRegCmd1->macConfigRegister2.bitfields.R_MAC_MOTION_STRENGTH      = pData->hdr_mac_motion_strength;

        pRegCmd1->macConfigRegister3.bitfields.MAC_LOW_LIGHT_TH1          = pData->hdr_mac_lowlight_th1;
        pRegCmd1->macConfigRegister3.bitfields.R_MAC_LOW_LIGHT_TH1        = pData->hdr_mac_lowlight_th1;
        pRegCmd1->macConfigRegister4.bitfields.MAC_HIGH_LIGHT_DTH_LOG2    = pData->hdr_mac_hilight_dth_log2;
        pRegCmd1->macConfigRegister4.bitfields.MAC_LOW_LIGHT_DTH_LOG2     = pData->hdr_mac_lowlight_dth_log2;
        pRegCmd1->macConfigRegister4.bitfields.MAC_LOW_LIGHT_STRENGTH     = pData->hdr_mac_lowlight_strength;
        pRegCmd1->macConfigRegister4.bitfields.R_MAC_HIGH_LIGHT_DTH_LOG2  = pData->hdr_mac_hilight_dth_log2;
        pRegCmd1->macConfigRegister4.bitfields.R_MAC_LOW_LIGHT_DTH_LOG2   = pData->hdr_mac_lowlight_dth_log2;
        pRegCmd1->macConfigRegister4.bitfields.R_MAC_LOW_LIGHT_STRENGTH   = pData->hdr_mac_lowlight_strength;

        pRegCmd1->macConfigRegister5.bitfields.MAC_HIGH_LIGHT_TH1         = pData->hdr_mac_hilight_th1;
        pRegCmd1->macConfigRegister5.bitfields.R_MAC_HIGH_LIGHT_TH1       = pData->hdr_mac_hilight_th1;

        pRegCmd1->macConfigRegister6.bitfields.MAC_SMOOTH_DTH_LOG2        = pData->hdr_mac_smooth_dth_log2;
        pRegCmd1->macConfigRegister6.bitfields.MAC_SMOOTH_ENABLE          = pData->hdr_mac_smooth_filter_en;
        pRegCmd1->macConfigRegister6.bitfields.MAC_SMOOTH_TH1             = pData->hdr_mac_smooth_th1;
        pRegCmd1->macConfigRegister6.bitfields.R_MAC_SMOOTH_DTH_LOG2      = pData->hdr_mac_smooth_dth_log2;
        pRegCmd1->macConfigRegister6.bitfields.R_MAC_SMOOTH_TH1           = pData->hdr_mac_smooth_th1;

        pRegCmd1->macConfigRegister7.bitfields.EXP_RATIO_RECIP            = pData->hdr_exp_ratio_recip;
        pRegCmd1->macConfigRegister7.bitfields.MAC_LINEAR_MODE            = pData->hdr_mac_linear_mode;
        pRegCmd1->macConfigRegister7.bitfields.MAC_SMOOTH_TAP0            = pData->hdr_mac_smooth_tap0;
        pRegCmd1->macConfigRegister7.bitfields.MSB_ALIGNED                = pData->hdr_MSB_align;
        pRegCmd1->macConfigRegister7.bitfields.R_MAC_SMOOTH_TAP0          = pData->hdr_mac_smooth_tap0;

        pRegCmd1->reconstructionConfig0.bitfields.RECON_H_EDGE_DTH_LOG2   = pData->hdr_rec_h_edge_dth_log2;
        pRegCmd1->reconstructionConfig0.bitfields.RECON_H_EDGE_TH1        = pData->hdr_rec_h_edge_th1;
        pRegCmd1->reconstructionConfig0.bitfields.RECON_MOTION_DTH_LOG2   = pData->hdr_rec_motion_dth_log2;
        pRegCmd1->reconstructionConfig0.bitfields.RECON_MOTION_TH1        = pData->hdr_rec_motion_th1;

        pRegCmd1->reconstructionConfig1.bitfields.RECON_DARK_DTH_LOG2     = pData->hdr_rec_dark_dth_log2;
        pRegCmd1->reconstructionConfig1.bitfields.RECON_DARK_TH1          = pData->hdr_rec_dark_th1;
        pRegCmd1->reconstructionConfig1.bitfields.RECON_EDGE_LPF_TAP0     = pData->hdr_rec_edge_lpf_tap0;
        pRegCmd1->reconstructionConfig1.bitfields.RECON_FLAT_REGION_TH    = pData->hdr_rec_flat_region_th;

        pRegCmd1->reconstructionConfig2.bitfields.R_RECON_H_EDGE_DTH_LOG2 = pData->hdr_rec_h_edge_dth_log2;
        pRegCmd1->reconstructionConfig2.bitfields.R_RECON_H_EDGE_TH1      = pData->hdr_rec_h_edge_th1;
        pRegCmd1->reconstructionConfig2.bitfields.R_RECON_MOTION_DTH_LOG2 = pData->hdr_rec_motion_dth_log2;
        pRegCmd1->reconstructionConfig2.bitfields.R_RECON_MOTION_TH1      = pData->hdr_rec_motion_th1;

        pRegCmd1->reconstructionConfig3.bitfields.RECON_LINEAR_MODE       = pData->hdr_rec_linear_mode;
        pRegCmd1->reconstructionConfig3.bitfields.RECON_MIN_FACTOR        = pData->hdr_rec_min_factor;
        pRegCmd1->reconstructionConfig3.bitfields.R_RECON_DARK_DTH_LOG2   = pData->hdr_rec_dark_dth_log2;
        pRegCmd1->reconstructionConfig3.bitfields.R_RECON_DARK_TH1        = pData->hdr_rec_dark_th1;
        pRegCmd1->reconstructionConfig3.bitfields.R_RECON_MIN_FACTOR      = pData->hdr_rec_min_factor;

        pRegCmd1->reconstructionConfig4.bitfields.R_RECON_FLAT_REGION_TH  = pData->hdr_rec_flat_region_th;

        pRegCmd2->configRegister5.bitfields.BLK_IN                        = pData->hdr_black_in;
        pRegCmd2->configRegister5.bitfields.BLK_OUT                       = pData->hdr_black_out;

        pRegCmd3->reconstructionConfig5.bitfields.ZREC_ENABLE             = pData->hdr_recon_en;
        pRegCmd3->reconstructionConfig5.bitfields.ZREC_FIRST_RB_EXP       = pData->hdr_first_field;
        pRegCmd3->reconstructionConfig5.bitfields.ZREC_PATTERN            = pData->hdr_zrec_pattern;
        pRegCmd3->reconstructionConfig5.bitfields.ZREC_PREFILT_TAP0       = pData->hdr_zrec_prefilt_tap0;

        pRegCmd3->reconstructionConfig6.bitfields.ZREC_G_GRAD_TH1         = pData->hdr_zrec_g_grad_th1;
        pRegCmd3->reconstructionConfig6.bitfields.ZREC_G_DTH_LOG2         = pData->hdr_zrec_g_grad_dth_log2;
        pRegCmd3->reconstructionConfig6.bitfields.ZREC_RB_GRAD_TH1        = pData->hdr_zrec_rb_grad_th1;
        pRegCmd3->reconstructionConfig6.bitfields.ZREC_RB_DTH_LOG2        = pData->hdr_zrec_rb_grad_dth_log2;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR20Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR20Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR20Titan17x::~IFEHDR20Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDR20Titan17x::~IFEHDR20Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR20Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDR20Titan17x::DumpRegConfig()
{
}

CAMX_NAMESPACE_END
