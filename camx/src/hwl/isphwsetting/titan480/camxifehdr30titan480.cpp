////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehdr30titan480.cpp
/// @brief CAMXIFEHDR30TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifehdr30titan480.h"
#include "hdr30setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30Titan480::IFEHDR30Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDR30Titan480::IFEHDR30Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEHDR30RegLengthDWord1) +
                     PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEHDR30RegLengthDWord2));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30Titan480::CreateCmdList(
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
                                              regIFE_IFE_0_PP_CLC_HDR_BINCORRECT_MODULE_CFG,
                                              IFEHDR30RegLengthDWord1,
                                              reinterpret_cast<UINT32*>(&m_regCmd1));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_HDR_BINCORRECT_HDR_0_CFG,
                                                  IFEHDR30RegLengthDWord2,
                                                  reinterpret_cast<UINT32*>(&m_regCmd2));
        }

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
// IFEHDR30Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEHDR30RegCmd1) == sizeof(pIFETuningMetadata->metadata480.IFEHDRData.HDRConfig1));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEHDRData.HDRConfig1, &m_regCmd1, sizeof(IFEHDR30RegCmd1));

        CAMX_STATIC_ASSERT(sizeof(IFEHDR30RegCmd2) == sizeof(pIFETuningMetadata->metadata480.IFEHDRData.HDRConfig2));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEHDRData.HDRConfig2, &m_regCmd2, sizeof(IFEHDR30RegCmd2));
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult              result     = CamxResultSuccess;
    HDR30BC10UnpackedField* pData = static_cast<HDR30BC10UnpackedField*>(pInput);
    HDR30UnpackedField*     pDataHDR30 = NULL;
    BC10UnpackedField*      pDataBC10  = NULL;

    CAMX_UNREFERENCED_PARAM(pOutput);

    if (NULL != pData)
    {
        pDataHDR30                                                               = pData->pUnpackedRegisterDataHDR30;
        pDataBC10                                                                = pData->pUnpackedRegisterDataBC10;
        m_regCmd1.configModule.bitfields.EN                                      = pDataHDR30->enable | pDataBC10->enable;
        if (TRUE == pDataHDR30->enable && TRUE == pDataBC10->enable)
        {
            CAMX_LOG_WARN(CamxLogGroupIQMod, "Either HDR or Binning correction should be enabled, BC will be disabled");
            pDataBC10->enable = FALSE;
        }

        m_regCmd1.configModule.bitfields.STRIPE_AUTO_CROP_DIS                    = 1;
        m_regCmd2.configRegister0.bitfields.HDR_ENABLE                           = pDataHDR30->enable;
        m_regCmd2.configRegister0.bitfields.HDR_EXP_RATIO                        = pDataHDR30->hdr_exp_ratio;
        m_regCmd2.configRegister0.bitfields.HDR_LINEAR_MODE                      = pDataHDR30->hdr_linear_mode;
        m_regCmd2.configRegister0.bitfields.HDR_LSB_ALIGN                        = pDataHDR30->hdr_lsb_align;
        m_regCmd2.configRegister0.bitfields.HDR_ZREC_FIRST_RB_EXP                = pDataHDR30->hdr_zrec_first_rb_exp;
        m_regCmd2.configRegister0.bitfields.HDR_ZREC_PATTERN                     = pDataHDR30->hdr_zrec_pattern;
        m_regCmd2.configRegister1.bitfields.HDR_BG_WB_GAIN_RATIO                 = pDataHDR30->hdr_bg_wb_gain_ratio;
        m_regCmd2.configRegister1.bitfields.HDR_RG_WB_GAIN_RATIO                 = pDataHDR30->hdr_rg_wb_gain_ratio;
        m_regCmd2.configRegister2.bitfields.HDR_GR_WB_GAIN_RATIO                 = pDataHDR30->hdr_gr_wb_gain_ratio;
        m_regCmd2.configRegister3.bitfields.HDR_GB_WB_GAIN_RATIO                 = pDataHDR30->hdr_gb_wb_gain_ratio;
        m_regCmd2.configRegister4.bitfields.HDR_BLACK_IN                         = pDataHDR30->hdr_black_in;
        m_regCmd2.configRegister4.bitfields.HDR_LONG_EXP_BLACK_IN                = pDataHDR30->hdr_long_exp_black_in;
        m_regCmd2.configRegister5.bitfields.HDR_EXP_RATIO_RECIP                  = pDataHDR30->hdr_exp_ratio_recip;
        m_regCmd2.configRegister5.bitfields.HDR_LONG_EXP_SATURATION              = pDataHDR30->hdr_long_exp_saturation;
        m_regCmd2.reconstructionConfig0.bitfields.HDR_REC_G_GRAD_DTH_LOG2        = pDataHDR30->hdr_rec_g_grad_dth_log2;
        m_regCmd2.reconstructionConfig0.bitfields.HDR_REC_G_GRAD_TH1             = pDataHDR30->hdr_rec_g_grad_th1;
        m_regCmd2.reconstructionConfig0.bitfields.HDR_REC_RB_GRAD_DTH_LOG2       = pDataHDR30->hdr_rec_rb_grad_dth_log2;
        m_regCmd2.reconstructionConfig0.bitfields.HDR_REC_RB_GRAD_TH1            = pDataHDR30->hdr_rec_rb_grad_th1;
        m_regCmd2.reconstructionConfig1.bitfields.HDR_REC_HL_DETAIL_NEGATIVE_W   = pDataHDR30->hdr_rec_hl_detail_negative_w;
        m_regCmd2.reconstructionConfig1.bitfields.HDR_REC_HL_DETAIL_POSITIVE_W   = pDataHDR30->hdr_rec_hl_detail_positive_w;
        m_regCmd2.reconstructionConfig1.bitfields.HDR_REC_HL_DETAIL_TH_W         = pDataHDR30->hdr_rec_hl_detail_th_w;
        m_regCmd2.reconstructionConfig1.bitfields.HDR_REC_HL_MOTION_TH_LOG2      = pDataHDR30->hdr_rec_hl_motion_th_log2;
        m_regCmd2.reconstructionConfig2.bitfields.HDR_REC_EDGE_SCALE             = pDataHDR30->hdr_rec_edge_scale;
        m_regCmd2.reconstructionConfig2.bitfields.HDR_REC_EDGE_SHORT_EXP_W       = pDataHDR30->hdr_rec_edge_short_exp_w;
        m_regCmd2.reconstructionConfig3.bitfields.HDR_REC_BLENDING_W_MAX         = pDataHDR30->hdr_rec_blending_w_max;
        m_regCmd2.reconstructionConfig3.bitfields.HDR_REC_BLENDING_W_MIN         = pDataHDR30->hdr_rec_blending_w_min;
        m_regCmd2.reconstructionConfig3.bitfields.HDR_REC_EDGE_DTH_LOG2          = pDataHDR30->hdr_rec_edge_dth_log2;
        m_regCmd2.reconstructionConfig3.bitfields.HDR_REC_EDGE_TH1               = pDataHDR30->hdr_rec_edge_th1;
        m_regCmd2.reconstructionConfig4.bitfields.HDR_REC_DETAIL_CORING_DTH_LOG2 = pDataHDR30->hdr_rec_detail_coring_dth_log2;
        m_regCmd2.reconstructionConfig4.bitfields.HDR_REC_DETAIL_CORING_STRENGTH = pDataHDR30->hdr_rec_detail_coring_strength;
        m_regCmd2.reconstructionConfig4.bitfields.HDR_REC_DETAIL_CORING_TH2      = pDataHDR30->hdr_rec_detail_coring_th2;
        m_regCmd2.reconstructionConfig4.bitfields.HDR_REC_DETAIL_TH_W            = pDataHDR30->hdr_rec_detail_th_w;
        m_regCmd2.macConfigRegister0.bitfields.HDR_MAC_HFS_ACT_DTH_LOG2          = pDataHDR30->hdr_mac_hfs_act_dth_log2;
        m_regCmd2.macConfigRegister0.bitfields.HDR_MAC_HFS_ACT_TH2               = pDataHDR30->hdr_mac_hfs_act_th2;
        m_regCmd2.macConfigRegister0.bitfields.HDR_MAC_HFS_ACT_WEIGHT            = pDataHDR30->hdr_mac_hfs_act_weight;
        m_regCmd2.macConfigRegister1.bitfields.HDR_MAC_STATIC_DTH_LOG2           = pDataHDR30->hdr_mac_static_dth_log2;
        m_regCmd2.macConfigRegister1.bitfields.HDR_MAC_STATIC_TH2                = pDataHDR30->hdr_mac_static_th2;
        m_regCmd2.macConfigRegister2.bitfields.HDR_MAC_HIGHLIGHT_DTH_LOG2        = pDataHDR30->hdr_mac_highlight_dth_log2;
        m_regCmd2.macConfigRegister2.bitfields.HDR_MAC_HIGHLIGHT_STRENGTH        = pDataHDR30->hdr_mac_highlight_strength;
        m_regCmd2.macConfigRegister3.bitfields.HDR_MAC_DARK_TH2                  = pDataHDR30->hdr_mac_dark_th2;
        m_regCmd2.macConfigRegister3.bitfields.HDR_MAC_LOWLIGHT_DTH_LOG2         = pDataHDR30->hdr_mac_lowlight_dth_log2;
        m_regCmd2.macConfigRegister3.bitfields.HDR_MAC_LOWLIGHT_STRENGTH         = pDataHDR30->hdr_mac_lowlight_strength;
        m_regCmd2.macConfigRegister4.bitfields.HDR_MAC_DILATION                  = pDataHDR30->hdr_mac_dilation;
        m_regCmd2.macConfigRegister4.bitfields.HDR_MAC_MOTION_DT0                = pDataHDR30->hdr_mac_motion_dt0;
        m_regCmd2.macConfigRegister4.bitfields.HDR_MAC_MOTION_SCALE              = pDataHDR30->hdr_mac_motion_scale;
        m_regCmd2.macConfigRegister4.bitfields.HDR_MAC_MOTION_STRICTNESS         = pDataHDR30->hdr_mac_motion_strictness;
        m_regCmd2.macConfigRegister5.bitfields.HDR_MAC_MOTION_DTH_LOG2           = pDataHDR30->hdr_mac_motion_dth_log2;
        m_regCmd2.macConfigRegister5.bitfields.HDR_MAC_MOTION_G2_W_MAX           = pDataHDR30->hdr_mac_motion_g2_w_max;
        m_regCmd2.macConfigRegister5.bitfields.HDR_MAC_MOTION_G2_W_MIN           = pDataHDR30->hdr_mac_motion_g2_w_min;
        m_regCmd2.macConfigRegister5.bitfields.HDR_MAC_MOTION_TH1                = pDataHDR30->hdr_mac_motion_th1;
        m_regCmd2.bcConfigRegister0.bitfields.BIN_CORR_EN                        = pDataBC10->enable;
        m_regCmd2.bcConfigRegister0.bitfields.VER_BIN_CORR_W1                    = pDataBC10->binCorrHorW1;
        m_regCmd2.bcConfigRegister0.bitfields.VER_BIN_CORR_W2                    = pDataBC10->binCorrHorW2;
        m_regCmd2.bcConfigRegister1.bitfields.HOR_BIN_CORR_W1                    = pDataBC10->binCorrVerW1;
        m_regCmd2.bcConfigRegister1.bitfields.HOR_BIN_CORR_W2                    = pDataBC10->binCorrVerW2;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30Titan480::CreateSubCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result = CamxResultSuccess;
    ISPInputData* pInputData = static_cast<ISPInputData*>(pSettingData);

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    if (NULL != pInputData &&
        NULL != pInputData->pCmdBuffer)
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
            regIFE_IFE_0_PP_CLC_HDR_BINCORRECT_MODULE_CFG,
            sizeof(m_regCmd1.configModule) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd1.configModule));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHDR30Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd1.configModule.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30Titan480::~IFEHDR30Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHDR30Titan480::~IFEHDR30Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHDR30Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEHDR30Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configModule          0x%x", m_regCmd1.configModule.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister0       0x%x", m_regCmd2.configRegister0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister1       0x%x", m_regCmd2.configRegister1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister2       0x%x", m_regCmd2.configRegister2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister3       0x%x", m_regCmd2.configRegister3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister4       0x%x", m_regCmd2.configRegister4.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 configRegister5       0x%x", m_regCmd2.configRegister5.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 reconstructionConfig0 0x%x", m_regCmd2.reconstructionConfig0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 reconstructionConfig1 0x%x", m_regCmd2.reconstructionConfig1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 reconstructionConfig2 0x%x", m_regCmd2.reconstructionConfig2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 reconstructionConfig3 0x%x", m_regCmd2.reconstructionConfig3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 reconstructionConfig4 0x%x", m_regCmd2.reconstructionConfig4.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister0    0x%x", m_regCmd2.macConfigRegister0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister1    0x%x", m_regCmd2.macConfigRegister1.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister2    0x%x", m_regCmd2.macConfigRegister2.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister3    0x%x", m_regCmd2.macConfigRegister3.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister4    0x%x", m_regCmd2.macConfigRegister4.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 macConfigRegister5    0x%x", m_regCmd2.macConfigRegister5.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 bcConfigRegister0     0x%x", m_regCmd2.bcConfigRegister0.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR30 bcConfigRegister1     0x%x", m_regCmd2.bcConfigRegister1.u32All);
}

CAMX_NAMESPACE_END
