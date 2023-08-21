////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeltm13titan17x.cpp
/// @brief IPELTM13Titan17x class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan170_ipe.h"
#include "ltm13setting.h"
#include "camxutils.h"
#include "camxipeltm13titan17x.h"
#include "camxipeltm13.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief LTM13 register Configuration
struct IPELTM13RegConfig
{
    IPE_IPE_0_PPS_CLC_LTM_MODULE_CFG                    LTMModuleConfig;                 ///< LTM module config
    IPE_IPE_0_PPS_CLC_LTM_DC_CFG_0                      LTMDataCollectionConfig0;        ///< LTM Data collector config 0
    IPE_IPE_0_PPS_CLC_LTM_DC_CFG_1                      LTMDataCollectionConfig1;        ///< LTM Data collector config 1
    IPE_IPE_0_PPS_CLC_LTM_DC_CFG_2                      LTMDataCollectionConfig2;        ///< LTM Data collector config 2
    IPE_IPE_0_PPS_CLC_LTM_RGB2Y_CFG                     LTMRGB2YConfig;                  ///< LTM RGB2Y config
    IPE_IPE_0_PPS_CLC_LTM_IP_0_CFG                      LTMImageProcessingConfig0;       ///< LTM Image processing config 0
    IPE_IPE_0_PPS_CLC_LTM_IP_1_CFG                      LTMImageProcessingConfig1;       ///< LTM Image processing config 1
    IPE_IPE_0_PPS_CLC_LTM_IP_2_CFG                      LTMImageProcessingConfig2;       ///< LTM Image processing config 2
    IPE_IPE_0_PPS_CLC_LTM_IP_3_CFG                      LTMImageProcessingConfig3;       ///< LTM Image processing config 3
    IPE_IPE_0_PPS_CLC_LTM_IP_4_CFG                      LTMImageProcessingConfig4;       ///< LTM Image processing config 4
    IPE_IPE_0_PPS_CLC_LTM_IP_5_CFG                      LTMImageProcessingConfig5;       ///< LTM Image processing config 5
    IPE_IPE_0_PPS_CLC_LTM_IP_6_CFG                      LTMImageProcessingConfig6;       ///< LTM Image processing config 6
    IPE_IPE_0_PPS_CLC_LTM_MASK_0_CFG                    LTMMaskFilterCoefficientConfig0; ///< LTM Mask config 0
    IPE_IPE_0_PPS_CLC_LTM_MASK_1_CFG                    LTMMaskFilterCoefficientConfig1; ///< LTM Mask config 1
    IPE_IPE_0_PPS_CLC_LTM_DOWNSCALE_MN_Y_CFG            LTMDownScaleMNYConfig;           ///< LTM downscale MN Y config
    IPE_IPE_0_PPS_CLC_LTM_DOWNSCALE_MN_Y_IMAGE_SIZE_CFG LTMDownScaleMNYimageSizeConfig;  ///< LTM downscale MN Y image
                                                                                         ///< size config
    IPE_IPE_0_PPS_CLC_LTM_DOWNSCALE_MN_Y_H_CFG          LTMDownScaleMNYHConfig;          ///< LTM downscale MN Y H config
    IPE_IPE_0_PPS_CLC_LTM_DOWNSCALE_MN_Y_H_PHASE_CFG    LTMDownScaleMNYHPhaseConfig;     ///< LTM downscale MN Y config
    IPE_IPE_0_PPS_CLC_LTM_DOWNSCALE_MN_Y_V_CFG          LTMDownScaleMNYVConfig;          ///< LTM downscale MN Y config
    IPE_IPE_0_PPS_CLC_LTM_DOWNSCALE_MN_Y_V_PHASE_CFG    LTMDownScaleMNYVPhaseConfig;     ///< LTM downscale MN Y config
} CAMX_PACKED;

/// @brief LTM13 Module Configuration
struct IPELTM13ModuleConfig
{
    IPE_IPE_0_PPS_CLC_LTM_MODULE_CFG moduleConfig;   ///< IPELTM13 Module config
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPELTM13RegLengthDWord = (sizeof(IPELTM13RegConfig) / sizeof(UINT32));

static const UINT32 LTMWeightLUT      = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_WEIGHT_LUT;
static const UINT32 LTMLaLUT0         = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_LA_LUT0;
static const UINT32 LTMLaLUT1         = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_LA_LUT1;
static const UINT32 LTMCurveLUT       = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_CURVE_LUT;
static const UINT32 LTMScaleLUT       = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_SCALE_LUT;
static const UINT32 LTMMaskLUT        = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_MASK_LUT;
static const UINT32 LTMLCEPositiveLUT = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_LCE_POS_SCALE_LUT;
static const UINT32 LTMLCENegativeLUT = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_LCE_NEG_SCALE_LUT;
static const UINT32 LTMRGammaLUT0     = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_RGAMMA_LUT0;
static const UINT32 LTMRGammaLUT1     = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_RGAMMA_LUT1;
static const UINT32 LTMRGammaLUT2     = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_RGAMMA_LUT2;
static const UINT32 LTMRGammaLUT3     = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_RGAMMA_LUT3;
static const UINT32 LTMRGammaLUT4     = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_RGAMMA_LUT4;
static const UINT32 LTMAverageLUT     = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_AVG_LUT;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13Titan17x::IPELTM13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPELTM13Titan17x::IPELTM13Titan17x()
{
    m_pModuleConfig = NULL;
    m_pRegCmd       = CAMX_CALLOC(sizeof(IPELTM13RegConfig));

    if (NULL == m_pRegCmd)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for Cmd Buffer");
    }
    else
    {
        m_pModuleConfig = CAMX_CALLOC(sizeof(IPELTM13ModuleConfig));

        if (NULL == m_pModuleConfig)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for module config");
            CAMX_FREE(m_pRegCmd);
            m_pRegCmd = NULL;
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pLTMHWSettingParams)
{
    CamxResult          result                 = CamxResultSuccess;
    ISPInputData*       pInputData             = static_cast<ISPInputData*>(pSettingData);
    LTMHWSettingParams* pModuleHWSettingParams = reinterpret_cast<LTMHWSettingParams*>(pLTMHWSettingParams);

    m_pLUTCmdBuffer       = pModuleHWSettingParams->pLUTCmdBuffer;
    m_pOffsetLUTCmdBuffer = pModuleHWSettingParams->pOffsetLUTCmdBuffer;

    result = WriteLUTInDMI(pInputData);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to update DMI buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13Titan17x::WriteLUTInDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13Titan17x::WriteLUTInDMI(
    ISPInputData* pInputData)
{
    CamxResult  result        = CamxResultSuccess;
    CmdBuffer*  pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];

    if ((NULL != pDMICmdBuffer) && (NULL != m_pLUTCmdBuffer))
    {
        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexWeight + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexWeight],
                                         IPELTMLUTNumEntries[LTMIndexWeight] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexWeight, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexLA0 + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexLA0],
                                         IPELTMLUTNumEntries[LTMIndexLA0] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexLA0, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexLA1 + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexLA1],
                                         IPELTMLUTNumEntries[LTMIndexLA1] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexLA1, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexCurve + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexCurve],
                                         IPELTMLUTNumEntries[LTMIndexCurve] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexCurve, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexScale + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexScale],
                                         IPELTMLUTNumEntries[LTMIndexScale] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexScale, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexMask + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexMask],
                                         IPELTMLUTNumEntries[LTMIndexMask] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexMask, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexLCEPositive + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexLCEPositive],
                                         IPELTMLUTNumEntries[LTMIndexLCEPositive] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexLCEPositive, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexLCENegative + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexLCENegative],
                                         IPELTMLUTNumEntries[LTMIndexLCENegative] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexLCENegative, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexRGamma0 + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexRGamma0],
                                         IPELTMLUTNumEntries[LTMIndexRGamma0] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexRGamma0, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexRGamma1 + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexRGamma1],
                                         IPELTMLUTNumEntries[LTMIndexRGamma1] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexRGamma1, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexRGamma2 + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexRGamma2],
                                         IPELTMLUTNumEntries[LTMIndexRGamma2] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexRGamma2, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexRGamma3 + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexRGamma3],
                                         IPELTMLUTNumEntries[LTMIndexRGamma3] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexRGamma3, result: %d", result);
        }

        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         LTMIndexRGamma4 + 1,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTMIndexRGamma4],
                                         IPELTMLUTNumEntries[LTMIndexRGamma4] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Faile to write LTMIndexRGamma4, result: %d", result);
        }

    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13Titan17x::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)

{
    CamxResult          result      = CamxResultSuccess;
    LTM13UnpackedField* pData       = static_cast<LTM13UnpackedField*>(pInput);
    UINT32              offset      = 0;
    UINT32*             pDMIDataPtr = static_cast<UINT32*>(pOutputVal);

    if ((NULL != pData) && (NULL != m_pRegCmd) && (NULL != m_pModuleConfig) && (NULL != pDMIDataPtr))
    {
        IPELTM13RegConfig*    pRegCmd       = static_cast<IPELTM13RegConfig*>(m_pRegCmd);
        IPELTM13ModuleConfig* pModuleConfig = static_cast<IPELTM13ModuleConfig*>(m_pModuleConfig);

        pRegCmd->LTMDataCollectionConfig0.bitfields.BIN_INIT_CNT           = pData->bin_init_cnt;
        pRegCmd->LTMRGB2YConfig.bitfields.C1                               = pData->c1;
        pRegCmd->LTMRGB2YConfig.bitfields.C2                               = pData->c2;
        pRegCmd->LTMRGB2YConfig.bitfields.C3                               = pData->c3;
        pRegCmd->LTMRGB2YConfig.bitfields.C4                               = pData->c4;
        pRegCmd->LTMImageProcessingConfig6.bitfields.LCE_THD               = pData->lce_thd;
        pRegCmd->LTMImageProcessingConfig6.bitfields.YRATIO_MAX            = pData->y_ratio_max;
        pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D0              = pData->mask_filter_kernel[0];
        pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D1              = pData->mask_filter_kernel[1];
        pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D2              = pData->mask_filter_kernel[2];
        pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D3              = pData->mask_filter_kernel[3];
        pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D4              = pData->mask_filter_kernel[4];
        pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D5              = pData->mask_filter_kernel[5];
        pRegCmd->LTMMaskFilterCoefficientConfig1.bitfields.SCALE           = pData->mask_filter_scale;
        pRegCmd->LTMMaskFilterCoefficientConfig1.bitfields.SHIFT           = pData->mask_filter_shift;
        pRegCmd->LTMDownScaleMNYConfig.bitfields.HORIZONTAL_TERMINATION_EN = pData->scale_in_cfg.early_termination_h;
        pRegCmd->LTMDownScaleMNYConfig.bitfields.VERTICAL_TERMINATION_EN   = pData->scale_in_cfg.early_termination_v;

        pModuleConfig->moduleConfig.bitfields.DC_AVG_BANK_SEL              = pData->dc_3dtable_avg_pong_sel;
        pModuleConfig->moduleConfig.bitfields.DC_EN                        = pData->data_collect_en;
        pModuleConfig->moduleConfig.bitfields.EN                           = pData->enable;
        pModuleConfig->moduleConfig.bitfields.IP_AVG_BANK_SEL              = pData->ip_3dtable_avg_pong_sel;
        pModuleConfig->moduleConfig.bitfields.IP_DEBUG_SEL                 = pData->debug_out_sel;
        pModuleConfig->moduleConfig.bitfields.IP_EN                        = pData->img_process_en;
        pModuleConfig->moduleConfig.bitfields.LA_EN                        = pData->la_en;
        pModuleConfig->moduleConfig.bitfields.MASK_EN                      = pData->mask_filter_en;
        pModuleConfig->moduleConfig.bitfields.MN_Y_EN                      = pData->scale_in_cfg.enable;
        pModuleConfig->moduleConfig.bitfields.RGAMMA_EN                    = pData->igamma_en;

        offset += IPELTMLUTNumEntries[LTMIndexWeight];
        offset += IPELTMLUTNumEntries[LTMIndexLA0];
        Utils::Memcpy((pDMIDataPtr + offset),
                      pData->la_curve.pLUTTable,
                      IPELTMLUTNumEntries[LTMIndexLA1] * sizeof(INT32));
        offset += IPELTMLUTNumEntries[LTMIndexLA1];
        offset += IPELTMLUTNumEntries[LTMIndexCurve];
        offset += IPELTMLUTNumEntries[LTMIndexScale];
        offset += IPELTMLUTNumEntries[LTMIndexMask];
        offset += IPELTMLUTNumEntries[LTMIndexLCEPositive];
        offset += IPELTMLUTNumEntries[LTMIndexLCENegative];
        offset += IPELTMLUTNumEntries[LTMIndexRGamma0];

        Utils::Memcpy((pDMIDataPtr + offset),
                      pData->igamma64.pLUTTable,
                      IPELTMLUTNumEntries[LTMIndexRGamma1] * sizeof(INT32));
        offset += IPELTMLUTNumEntries[LTMIndexRGamma1];

        Utils::Memcpy((pDMIDataPtr + offset),
                      pData->igamma64.pLUTTable,
                      IPELTMLUTNumEntries[LTMIndexRGamma2] * sizeof(INT32));
        offset += IPELTMLUTNumEntries[LTMIndexRGamma2];

        Utils::Memcpy((pDMIDataPtr + offset),
                      pData->igamma64.pLUTTable,
                      IPELTMLUTNumEntries[LTMIndexRGamma3] * sizeof(INT32));
        offset += IPELTMLUTNumEntries[LTMIndexRGamma3];

        Utils::Memcpy((pDMIDataPtr + offset),
                      pData->igamma64.pLUTTable,
                      IPELTMLUTNumEntries[LTMIndexRGamma4] * sizeof(INT32));
        offset += IPELTMLUTNumEntries[LTMIndexRGamma4];
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid input pData %p, pOutputVal %p", pData, pOutputVal);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13Titan17x::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13Titan17x::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13Titan17x::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPELTM13Titan17x::GetRegSize()
{
    return sizeof(IPELTM13RegConfig);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13Titan17x::SetupInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM13Titan17x::SetupInternalData(
    VOID* pData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pData) && (NULL != m_pRegCmd) && (NULL != m_pModuleConfig))
    {
        IpeIQSettings*        pIPEIQSettings = static_cast<IpeIQSettings*>(pData);
        IPELTM13ModuleConfig* pModuleConfig  = static_cast<IPELTM13ModuleConfig*>(m_pModuleConfig);
        IPELTM13RegConfig*    pRegCmd        = static_cast<IPELTM13RegConfig*>(m_pRegCmd);

        pIPEIQSettings->ltmParameters.moduleCfg.LA_EN           = pModuleConfig->moduleConfig.bitfields.LA_EN;
        pIPEIQSettings->ltmParameters.moduleCfg.MASK_EN         = pModuleConfig->moduleConfig.bitfields.MASK_EN;

        pIPEIQSettings->ltmParameters.dcBinInitCnt              = pRegCmd->LTMDataCollectionConfig0.bitfields.BIN_INIT_CNT;
        pIPEIQSettings->ltmParameters.rgb2yC1                   = pRegCmd->LTMRGB2YConfig.bitfields.C1;
        pIPEIQSettings->ltmParameters.rgb2yC2                   = pRegCmd->LTMRGB2YConfig.bitfields.C2;
        pIPEIQSettings->ltmParameters.rgb2yC3                   = pRegCmd->LTMRGB2YConfig.bitfields.C3;
        pIPEIQSettings->ltmParameters.rgb2yC4                   = pRegCmd->LTMRGB2YConfig.bitfields.C4;

        pIPEIQSettings->ltmParameters.ipLceThd                  = pRegCmd->LTMImageProcessingConfig6.bitfields.LCE_THD;
        pIPEIQSettings->ltmParameters.ipYRatioMax               = pRegCmd->LTMImageProcessingConfig6.bitfields.YRATIO_MAX;

        pIPEIQSettings->ltmParameters.maskD0                    = pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D0;
        pIPEIQSettings->ltmParameters.maskD1                    = pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D1;
        pIPEIQSettings->ltmParameters.maskD2                    = pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D2;
        pIPEIQSettings->ltmParameters.maskD3                    = pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D3;
        pIPEIQSettings->ltmParameters.maskD4                    = pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D4;
        pIPEIQSettings->ltmParameters.maskD5                    = pRegCmd->LTMMaskFilterCoefficientConfig0.bitfields.D5;

        pIPEIQSettings->ltmParameters.maskShift                 = pRegCmd->LTMMaskFilterCoefficientConfig1.bitfields.SHIFT;
        pIPEIQSettings->ltmParameters.maskScale                 = pRegCmd->LTMMaskFilterCoefficientConfig1.bitfields.SCALE;

        pIPEIQSettings->ltmParameters.downscaleMNHorTermination =
            pRegCmd->LTMDownScaleMNYConfig.bitfields.HORIZONTAL_TERMINATION_EN;
        pIPEIQSettings->ltmParameters.downscaleMNVerTermination =
            pRegCmd->LTMDownScaleMNYConfig.bitfields.VERTICAL_TERMINATION_EN;
        pIPEIQSettings->ltmParameters.dataCollectionEnabled     = pModuleConfig->moduleConfig.bitfields.DC_EN;
        pIPEIQSettings->ltmParameters.ltmImgProcessEn           = pModuleConfig->moduleConfig.bitfields.IP_EN;

        pIPEIQSettings->ltmParameters.moduleCfg.RGAMMA_EN       = pModuleConfig->moduleConfig.bitfields.RGAMMA_EN;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input data is pData %p m_pRegCmd %p", pData, m_pRegCmd);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13Titan17x::~IPELTM13Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPELTM13Titan17x::~IPELTM13Titan17x()
{
    if (NULL != m_pRegCmd)
    {
        CAMX_FREE(m_pRegCmd);
        m_pRegCmd = NULL;
    }

    if (NULL != m_pModuleConfig)
    {
        CAMX_FREE(m_pModuleConfig);
        m_pModuleConfig = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM13Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPELTM13Titan17x::DumpRegConfig()
{
    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");
}

CAMX_NAMESPACE_END
