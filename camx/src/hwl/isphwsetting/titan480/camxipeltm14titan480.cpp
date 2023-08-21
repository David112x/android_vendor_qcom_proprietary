////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxipeltm14titan480.cpp
/// @brief IPELTM14Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "titan480_ipe.h"
#include "ltm14setting.h"
#include "camxutils.h"
#include "camxipeltm14titan480.h"
#include "camxipeltm14.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// @brief LTM14 register Configuration
struct IPELTM14RegConfig
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

/// @brief LTM14 Module Configuration
struct IPELTM14ModuleConfig
{
    IPE_IPE_0_PPS_CLC_LTM_MODULE_CFG moduleConfig;   ///< IPELTM14 Module config
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 IPELTM14RegLengthDWord = (sizeof(IPELTM14RegConfig) / sizeof(UINT32));

static const UINT32 LTM14WeightLUT      = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_WEIGHT_LUT;
static const UINT32 LTM14LaLUT          = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_LA_LUT;
static const UINT32 LTM14CurveLUT       = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_CURVE_LUT;
static const UINT32 LTM14ScaleLUT       = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_SCALE_LUT;
static const UINT32 LTM14MaskLUT        = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_MASK_LUT;
static const UINT32 LTM14LCEPositiveLUT = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_LCE_POS_SCALE_LUT;
static const UINT32 LTM14LCENegativeLUT = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_LCE_NEG_SCALE_LUT;
static const UINT32 LTM14RGammaLUT      = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_RGAMMA_LUT;
static const UINT32 LTM14AverageLUT     = IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_AVG_PLUT;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM14Titan480::IPELTM14Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPELTM14Titan480::IPELTM14Titan480()
{
    m_pModuleConfig = NULL;
    m_pRegCmd       = CAMX_CALLOC(sizeof(IPELTM14RegConfig));

    if (NULL == m_pRegCmd)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for Cmd Buffer");
    }
    else
    {
        m_pModuleConfig = CAMX_CALLOC(sizeof(IPELTM14ModuleConfig));

        if (NULL == m_pModuleConfig)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to allocate memory for module config");
            CAMX_FREE(m_pRegCmd);
            m_pRegCmd = NULL;
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM14Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM14Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pLTMHWSettingParams)
{
    CamxResult            result                 = CamxResultSuccess;
    ISPInputData*         pInputData             = static_cast<ISPInputData*>(pSettingData);
    LTM14HWSettingParams* pModuleHWSettingParams = reinterpret_cast<LTM14HWSettingParams*>(pLTMHWSettingParams);

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
// IPELTM14Titan480::WriteLUTInDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM14Titan480::WriteLUTInDMI(
    ISPInputData* pInputData)
{
    CamxResult  result        = CamxResultSuccess;
    CmdBuffer*  pDMICmdBuffer = pInputData->pipelineIPEData.ppIPECmdBuffer[CmdBufferDMIHeader];

    if ((NULL != pDMICmdBuffer) && (NULL != m_pLUTCmdBuffer))
    {
        result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                         regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                         IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_WEIGHT_LUT,
                                         m_pLUTCmdBuffer,
                                         m_pOffsetLUTCmdBuffer[LTM14IndexWeight],
                                         IPELTM14LUTNumEntries[LTM14IndexWeight] * sizeof(UINT32));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write LTM14IndexWeight");
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                             IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_LA_LUT,
                                             m_pLUTCmdBuffer,
                                             m_pOffsetLUTCmdBuffer[LTM14IndexLA],
                                             IPELTM14LUTNumEntries[LTM14IndexLA] * sizeof(UINT32));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write LTM14IndexLA");
            }
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                             IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_CURVE_LUT,
                                             m_pLUTCmdBuffer,
                                             m_pOffsetLUTCmdBuffer[LTM14IndexCurve],
                                             IPELTM14LUTNumEntries[LTM14IndexCurve] * sizeof(UINT32));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write LTM14IndexCurve");
            }
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                             IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_SCALE_LUT,
                                             m_pLUTCmdBuffer,
                                             m_pOffsetLUTCmdBuffer[LTM14IndexScale],
                                             IPELTM14LUTNumEntries[LTM14IndexScale] * sizeof(UINT32));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write LTM14IndexScale");
            }
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                             IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_MASK_LUT,
                                             m_pLUTCmdBuffer,
                                             m_pOffsetLUTCmdBuffer[LTM14IndexMask],
                                             IPELTM14LUTNumEntries[LTM14IndexMask] * sizeof(UINT32));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write LTM14IndexMask");
            }
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                             IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_LCE_POS_SCALE_LUT,
                                             m_pLUTCmdBuffer,
                                             m_pOffsetLUTCmdBuffer[LTM14IndexLCEPositive],
                                             IPELTM14LUTNumEntries[LTM14IndexLCEPositive] * sizeof(UINT32));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write LTM14IndexLCEPositive");
            }
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                             IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_LCE_NEG_SCALE_LUT,
                                             m_pLUTCmdBuffer,
                                             m_pOffsetLUTCmdBuffer[LTM14IndexLCENegative],
                                             IPELTM14LUTNumEntries[LTM14IndexLCENegative] * sizeof(UINT32));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write LTM14IndexLCENegative");
            }
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteDMI(pDMICmdBuffer,
                                             regIPE_IPE_0_PPS_CLC_LTM_DMI_CFG,
                                             IPE_IPE_0_PPS_CLC_LTM_DMI_LUT_CFG_LUT_SEL_RGAMMA_LUT,
                                             m_pLUTCmdBuffer,
                                             m_pOffsetLUTCmdBuffer[LTM14IndexRGamma],
                                             IPELTM14LUTNumEntries[LTM14IndexRGamma] * sizeof(UINT32));
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupPProc, "Failed to write LTM14IndexRGamma");
            }
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
// IPELTM14Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM14Titan480::PackIQRegisterSetting(
    VOID*  pInput,
    VOID*  pOutputVal)

{
    CamxResult          result      = CamxResultSuccess;
    LTM14UnpackedField* pData       = static_cast<LTM14UnpackedField*>(pInput);
    UINT32              offset      = 0;
    UINT32*             pDMIDataPtr = static_cast<UINT32*>(pOutputVal);

    if ((NULL != pData) && (NULL != m_pRegCmd) && (NULL != m_pModuleConfig) && (NULL != pDMIDataPtr))
    {
        IPELTM14RegConfig*    pRegCmd       = static_cast<IPELTM14RegConfig*>(m_pRegCmd);
        IPELTM14ModuleConfig* pModuleConfig = static_cast<IPELTM14ModuleConfig*>(m_pModuleConfig);

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

        offset += IPELTM14LUTNumEntries[LTM14IndexWeight];
        Utils::Memcpy((pDMIDataPtr + offset),
            pData->la_curve.pLUTTable,
            IPELTM14LUTNumEntries[LTM14IndexLA] * sizeof(INT32));
        offset += IPELTM14LUTNumEntries[LTM14IndexLA];
        offset += IPELTM14LUTNumEntries[LTM14IndexCurve];
        offset += IPELTM14LUTNumEntries[LTM14IndexScale];
        offset += IPELTM14LUTNumEntries[LTM14IndexMask];
        offset += IPELTM14LUTNumEntries[LTM14IndexLCEPositive];
        offset += IPELTM14LUTNumEntries[LTM14IndexLCENegative];
        Utils::Memcpy((pDMIDataPtr + offset),
                      pData->igamma64.pLUTTable,
                      IPELTM14LUTNumEntries[LTM14IndexRGamma] * sizeof(INT32));
        offset += IPELTM14LUTNumEntries[LTM14IndexRGamma];
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid data is NULL pData %p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM14Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IPELTM14Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pInput);

    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM14Titan480::GetRegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT IPELTM14Titan480::GetRegSize()
{
    return sizeof(IPELTM14RegConfig);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM14Titan480::SetupInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE CamxResult IPELTM14Titan480::SetupInternalData(
    VOID* pData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pData) && (NULL != m_pRegCmd) && (NULL != m_pModuleConfig))
    {
        IpeIQSettings*        pIPEIQSettings = static_cast<IpeIQSettings*>(pData);
        IPELTM14ModuleConfig* pModuleConfig  = static_cast<IPELTM14ModuleConfig*>(m_pModuleConfig);
        IPELTM14RegConfig*    pRegCmd        = static_cast<IPELTM14RegConfig*>(m_pRegCmd);

        pIPEIQSettings->ltmParameters.moduleCfg.RGAMMA_EN       = pModuleConfig->moduleConfig.bitfields.RGAMMA_EN;

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
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "Input data is pData %p m_pRegCmd %p", pData, m_pRegCmd);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IPELTM14Titan480::~IPELTM14Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPELTM14Titan480::~IPELTM14Titan480()
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
// IPELTM14Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IPELTM14Titan480::DumpRegConfig()
{
    CAMX_LOG_ERROR(CamxLogGroupPProc, "Function not implemented");
}

CAMX_NAMESPACE_END
