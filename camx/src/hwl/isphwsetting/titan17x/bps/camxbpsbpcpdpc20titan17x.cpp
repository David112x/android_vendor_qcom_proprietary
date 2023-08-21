////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpsbpcpdpc20titan17x.cpp
/// @brief CAMXBPSBPCPDPC20TITAN17X class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpsbpcpdpc20titan17x.h"
#include "bpspdpc20setting.h"
#include "camxiqinterface.h"
#include "titan170_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC register Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BPSBPCPDPC20RegConfig0
{
    BPS_BPS_0_CLC_BPC_PDPC_PDPC_BLACK_LEVEL      PDPCBlackLevel;            ///< PDPC Black level
    BPS_BPS_0_CLC_BPC_PDPC_PDAF_HDR_EXP_RATIO    HDRExposureRatio;          ///< HDR exposure ratio
    BPS_BPS_0_CLC_BPC_PDPC_BAD_PIXEL_THRESHOLDS  BPCPixelThreshold;         ///< BPC Pixel threshold
    BPS_BPS_0_CLC_BPC_PDPC_BAD_PIXEL_DET_OFFSET  badPixelDetectionOffset;   ///< BPC/BCC detection offset
    BPS_BPS_0_CLC_BPC_PDPC_PDAF_RG_WB_GAIN       PDAFRGWhiteBalanceGain;    ///< PDAF R/G white balance gain
    BPS_BPS_0_CLC_BPC_PDPC_PDAF_BG_WB_GAIN       PDAFBGWhiteBalanceGain;    ///< PDAF B/G white balance gain
    BPS_BPS_0_CLC_BPC_PDPC_PDAF_GR_WB_GAIN       PDAFGRWhiteBalanceGain;    ///< PDAF G/R white balance gain
    BPS_BPS_0_CLC_BPC_PDPC_PDAF_GB_WB_GAIN       PDAFGBWhiteBalanceGain;    ///< PDAF G/B white balance gain
    BPS_BPS_0_CLC_BPC_PDPC_PDAF_LOC_OFFSET_CFG   PDAFLocationOffsetConfig;  ///< PDAF location offset config
    BPS_BPS_0_CLC_BPC_PDPC_PDAF_LOC_END_CFG      PDAFLocationEndConfig;     ///< PDAF location end config
} CAMX_PACKED;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC register Configuration 1
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BPSBPCPDPC20RegConfig1
{
    BPS_BPS_0_CLC_BPC_PDPC_BAD_PIXEL_DET_OFFSET_T2  badPixelDetectionOffsetT2;  ///< BPC/BCC detection offset for exposure T2
    BPS_BPS_0_CLC_BPC_PDPC_SATURATION_THRESHOLD     PDPCSaturationThreshold;    ///< PDPC saturation threshold
    BPS_BPS_0_CLC_BPC_PDPC_PDAF_TAB_OFFSET_CFG      PDAFTabOffsetConfig;        ///< PDAF tab offset config
} CAMX_PACKED;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC register Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BPSBPCPDPC20RegCmd
{
    BPSBPCPDPC20RegConfig0 config0;    ///< BPC/PDPC Config 0
    BPSBPCPDPC20RegConfig1 config1;    ///< BPC/PDPC Config 1
} CAMX_PACKED;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC module Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BPSBPCPDPC20ModuleConfig
{
    BPS_BPS_0_CLC_BPC_PDPC_MODULE_CFG  moduleConfig;   ///< BPCPDPC20 Module config
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 PDPCLUTBank0 = BPS_BPS_0_CLC_BPC_PDPC_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT32 PDPCLUTBank1 = BPS_BPS_0_CLC_BPC_PDPC_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK1;
static const UINT32 PDPCLUT      = BPS_BPS_0_CLC_BPC_PDPC_DMI_LUT_CFG_LUT_SEL_PDAF_LUT;

static const UINT32 BPSBPCPDPC20RegConfig0LengthDword = sizeof(BPSBPCPDPC20RegConfig0) / RegisterWidthInBytes;
static const UINT32 BPSBPCPDPC20RegConfig1LengthDword = sizeof(BPSBPCPDPC20RegConfig1) / RegisterWidthInBytes;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20Titan17x::BPSBPCPDPC20Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSBPCPDPC20Titan17x::BPSBPCPDPC20Titan17x()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20Titan17x::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20Titan17x::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSBPCPDPC20Titan17x* pHWSetting = CAMX_NEW BPSBPCPDPC20Titan17x;

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
/// BPSBPCPDPC20Titan17x::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20Titan17x::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pModuleConfig = CAMX_CALLOC(sizeof(BPSBPCPDPC20ModuleConfig));
    if (NULL == m_pModuleConfig)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for module configuration!");
    }
    else
    {
        m_pRegCmd = CAMX_CALLOC(sizeof(BPSBPCPDPC20RegCmd));
        if (NULL != m_pRegCmd)
        {
            SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSBPCPDPC20RegConfig0LengthDword) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSBPCPDPC20RegConfig1LengthDword));
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
// BPSBPCPDPC20Titan17x::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20Titan17x::WriteLUTtoDMI(
    VOID*   pInput)
{
    CamxResult      result      = CamxResultSuccess;
    ISPInputData*   pInputData  = static_cast<ISPInputData*>(pInput);

    UINT32     lengthInByte = BPSPDPC20DMILengthDword * sizeof(UINT32);

    // CDM pack the DMI buffer and patch the Red and Blue LUT DMI buffer into CDM pack
    if ((NULL != pInputData) && (NULL != pInputData->pDMICmdBuffer))
    {
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_BPC_PDPC_DMI_CFG,
                                         PDPCLUT,
                                         pInputData->p64bitDMIBuffer,
                                         0,
                                         lengthInByte);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write DMI data");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20Titan17x::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20Titan17x::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CamxResult      result     = CamxResultSuccess;
    ISPInputData*   pInputData = static_cast<ISPInputData*>(pSettingData);

    BPSBPCPDPC20ModuleConfig* pModuleCfg = static_cast<BPSBPCPDPC20ModuleConfig*>(m_pModuleConfig);
    BPSBPCPDPC20RegCmd*       pRegCmd    = static_cast<BPSBPCPDPC20RegCmd*>(m_pRegCmd);

    if (0 != pModuleCfg->moduleConfig.bitfields.PDAF_PDPC_EN)
    {
        result = WriteLUTtoDMI(pInputData);
    }

    if ((CamxResultSuccess == result) && (NULL != pInputData->pCmdBuffer))
    {
        CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regBPS_BPS_0_CLC_BPC_PDPC_PDPC_BLACK_LEVEL,
                                              BPSBPCPDPC20RegConfig0LengthDword,
                                              reinterpret_cast<UINT32*>(&pRegCmd->config0));

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regBPS_BPS_0_CLC_BPC_PDPC_BAD_PIXEL_DET_OFFSET_T2,
                                                  BPSBPCPDPC20RegConfig1LengthDword,
                                                  reinterpret_cast<UINT32*>(&pRegCmd->config1));

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
// BPSBPCPDPC20Titan17x::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20Titan17x::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    BPSBPCPDPC20ModuleConfig* pModuleCfg = static_cast<BPSBPCPDPC20ModuleConfig*>(m_pModuleConfig);
    BPSBPCPDPC20RegCmd*       pRegCmd    = static_cast<BPSBPCPDPC20RegCmd*>(m_pRegCmd);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->pdpcParameters.moduleCfg.EN                |= moduleEnable;

    pBPSIQSettings->pdpcParameters.moduleCfg.BPC_EN             = pModuleCfg->moduleConfig.bitfields.BPC_EN;
    pBPSIQSettings->pdpcParameters.moduleCfg.PDAF_PDPC_EN       = pModuleCfg->moduleConfig.bitfields.PDAF_PDPC_EN;

    pBPSIQSettings->pdpcParameters.moduleCfg.HOT_PIXEL_CORRECTION_DISABLE            =
        pModuleCfg->moduleConfig.bitfields.HOT_PIXEL_CORRECTION_DISABLE;
    pBPSIQSettings->pdpcParameters.moduleCfg.COLD_PIXEL_CORRECTION_DISABLE           =
        pModuleCfg->moduleConfig.bitfields.COLD_PIXEL_CORRECTION_DISABLE;
    pBPSIQSettings->pdpcParameters.moduleCfg.USING_CROSS_CHANNEL_EN                  =
        pModuleCfg->moduleConfig.bitfields.USING_CROSS_CHANNEL_EN;
    pBPSIQSettings->pdpcParameters.moduleCfg.REMOVE_ALONG_EDGE_EN                    =
        pModuleCfg->moduleConfig.bitfields.REMOVE_ALONG_EDGE_EN;
    pBPSIQSettings->pdpcParameters.moduleCfg.BAYER_PATTERN                           =
        pModuleCfg->moduleConfig.bitfields.BAYER_PATTERN;
    pBPSIQSettings->pdpcParameters.moduleCfg.PDAF_HDR_SELECTION                      =
        pModuleCfg->moduleConfig.bitfields.PDAF_HDR_SELECTION;
    pBPSIQSettings->pdpcParameters.moduleCfg.PDAF_ZZHDR_FIRST_RB_EXP                 =
        pModuleCfg->moduleConfig.bitfields.PDAF_ZZHDR_FIRST_RB_EXP;

    pBPSIQSettings->pdpcParameters.pdafGlobalOffsetX                                 =
        pRegCmd->config0.PDAFLocationOffsetConfig.bitfields.X_OFFSET;
    pBPSIQSettings->pdpcParameters.pdafGlobalOffsetY                                 =
        pRegCmd->config0.PDAFLocationOffsetConfig.bitfields.Y_OFFSET;
    pBPSIQSettings->pdpcParameters.pdafTableOffsetX                                  =
        pRegCmd->config1.PDAFTabOffsetConfig.bitfields.X_OFFSET;
    pBPSIQSettings->pdpcParameters.pdafTableOffsetY                                  =
        pRegCmd->config1.PDAFTabOffsetConfig.bitfields.Y_OFFSET;
    pBPSIQSettings->pdpcParameters.pdafXEnd                                          =
        pRegCmd->config0.PDAFLocationEndConfig.bitfields.X_END;
    pBPSIQSettings->pdpcParameters.pdafYEnd                                          =
        pRegCmd->config0.PDAFLocationEndConfig.bitfields.Y_END;

    if (NULL != pInputData->pOEMIQSetting)
    {
        pInputData->pCalculatedData->hotPixelMode = HotPixelModeFast;
    }
    else
    {
        pInputData->pCalculatedData->hotPixelMode = pInputData->pHALTagsData->hotPixelMode;
        if (HotPixelModeOff == pInputData->pCalculatedData->hotPixelMode)
        {
            pBPSIQSettings->pdpcParameters.moduleCfg.HOT_PIXEL_CORRECTION_DISABLE = TRUE;
        }
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20Titan17x::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20Titan17x::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult          result             = CamxResultSuccess;
    ISPInputData*       pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata*  pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*      pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSBPCPDPC20RegCmd* pRegCmd            = static_cast<BPSBPCPDPC20RegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSBPCPDPC20RegConfig0) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSBBPCPDPCData.BPCPDPConfig0));

        CAMX_STATIC_ASSERT(sizeof(BPSBPCPDPC20RegConfig1) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSBBPCPDPCData.BPCPDPConfig1));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSBBPCPDPCData.BPCPDPConfig0,
                      &pRegCmd->config0,
                      sizeof(BPSBPCPDPC20RegConfig0));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata17x.BPSBBPCPDPCData.BPCPDPConfig1,
                      &pRegCmd->config1,
                      sizeof(BPSBPCPDPC20RegConfig1));

        if (TRUE == pBPSIQSettings->pdpcParameters.moduleCfg.EN)
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSBPCPDPC20Register,
                DebugDataTagType::TuningBPSBPCPDPC20Config,
                1,
                &pBPSTuningMetadata->BPSTuningMetadata17x.BPSBBPCPDPCData,
                sizeof(pBPSTuningMetadata->BPSTuningMetadata17x.BPSBBPCPDPCData));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20Titan17x::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSBPCPDPC20Titan17x::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult              result = CamxResultSuccess;
    PDPC20UnpackedField*    pData  = static_cast<PDPC20UnpackedField*>(pInput);
    BPSBPCPDPC20OutputData* pCmd   = static_cast<BPSBPCPDPC20OutputData*>(pOutput);

    BPSBPCPDPC20ModuleConfig* pModuleCfg = static_cast<BPSBPCPDPC20ModuleConfig*>(m_pModuleConfig);
    BPSBPCPDPC20RegCmd*       pRegCmd    = static_cast<BPSBPCPDPC20RegCmd*>(m_pRegCmd);

    if ((NULL != pInput) && (NULL != pData) && (NULL != pCmd))
    {
        /// @todo (CAMX-1399) Verify all the values from unpacked are falls below bitfields
        pRegCmd->config0.PDPCBlackLevel.bitfields.BLACK_LEVEL               = pData->blackLevel;

        pRegCmd->config0.HDRExposureRatio.bitfields.EXP_RATIO               = pData->PDAFHDRExposureRatio;
        pRegCmd->config0.HDRExposureRatio.bitfields.EXP_RATIO_RECIP         = pData->PDAFHDRExposureRatioRecip;

        pRegCmd->config0.BPCPixelThreshold.bitfields.CORRECTION_THRESHOLD   = pData->correctionThreshold;
        pRegCmd->config0.BPCPixelThreshold.bitfields.FMAX_PIXEL_Q6          = pData->fmaxPixelQ6;
        pRegCmd->config0.BPCPixelThreshold.bitfields.FMIN_PIXEL_Q6          = pData->fminPixelQ6;

        pRegCmd->config0.badPixelDetectionOffset.bitfields.BCC_OFFSET       = pData->bccOffset;
        pRegCmd->config0.badPixelDetectionOffset.bitfields.BPC_OFFSET       = pData->bpcOffset;

        pRegCmd->config0.PDAFRGWhiteBalanceGain.bitfields.RG_WB_GAIN        = pData->rgWbGain4096;
        pRegCmd->config0.PDAFBGWhiteBalanceGain.bitfields.BG_WB_GAIN        = pData->bgWbGain4096;
        pRegCmd->config0.PDAFGRWhiteBalanceGain.bitfields.GR_WB_GAIN        = pData->grWbGain4096;
        pRegCmd->config0.PDAFGBWhiteBalanceGain.bitfields.GB_WB_GAIN        = pData->gbWbGain4096;

        pRegCmd->config0.PDAFLocationOffsetConfig.bitfields.Y_OFFSET        = pData->PDAFGlobalOffsetY;
        pRegCmd->config0.PDAFLocationOffsetConfig.bitfields.X_OFFSET        = pData->PDAFGlobalOffsetX;

        pRegCmd->config0.PDAFLocationEndConfig.bitfields.Y_END              = pData->PDAFYend;
        pRegCmd->config0.PDAFLocationEndConfig.bitfields.X_END              = pData->PDAFXend;

        pRegCmd->config1.badPixelDetectionOffsetT2.bitfields.BCC_OFFSET_T2  = pData->bccOffsetT2;
        pRegCmd->config1.badPixelDetectionOffsetT2.bitfields.BPC_OFFSET_T2  = pData->bpcOffsetT2;

        pRegCmd->config1.PDPCSaturationThreshold.bitfields.SAT_THRESHOLD    = pData->saturationThreshold;

        pRegCmd->config1.PDAFTabOffsetConfig.bitfields.Y_OFFSET             = pData->PDAFTableYOffset;
        pRegCmd->config1.PDAFTabOffsetConfig.bitfields.X_OFFSET             = pData->PDAFTableXOffset;

        /// @todo (CAMX-2864) Before re-enabling BPS BPC PDPC, fix buffer overrun here (512 bytes copied into 256 byte buffer)
        Utils::Memcpy(static_cast<VOID*>(pCmd->pDMIDataPtr),
            static_cast<const VOID*>(pData->PDAFPDMask),
            sizeof(UINT32) * 64);

        pModuleCfg->moduleConfig.bitfields.BAYER_PATTERN                 = pData->bayerPattern;
        pModuleCfg->moduleConfig.bitfields.STRIPE_AUTO_CROP_DIS          = 1;
        pModuleCfg->moduleConfig.bitfields.PDAF_PDPC_EN                  = pData->PDAFPDPCEnable;
        pModuleCfg->moduleConfig.bitfields.BPC_EN                        = pData->PDAFDSBPCEnable;
        pModuleCfg->moduleConfig.bitfields.LEFT_CROP_EN                  = pData->leftCropEnable;
        pModuleCfg->moduleConfig.bitfields.RIGHT_CROP_EN                 = pData->rightCropEnable;
        pModuleCfg->moduleConfig.bitfields.HOT_PIXEL_CORRECTION_DISABLE  = pData->hotPixelCorrectionDisable;
        pModuleCfg->moduleConfig.bitfields.COLD_PIXEL_CORRECTION_DISABLE = pData->coldPixelCorrectionDisable;
        pModuleCfg->moduleConfig.bitfields.USING_CROSS_CHANNEL_EN        = pData->usingCrossChannel;
        pModuleCfg->moduleConfig.bitfields.REMOVE_ALONG_EDGE_EN          = pData->removeAlongEdge;
        pModuleCfg->moduleConfig.bitfields.PDAF_HDR_SELECTION            = pData->PDAFHDRSelection;
        pModuleCfg->moduleConfig.bitfields.PDAF_ZZHDR_FIRST_RB_EXP       = pData->PDAFzzHDRFirstrbExposure;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input data is NULL %p %p %p", pInput, pData, pCmd);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20Titan17x::~BPSBPCPDPC20Titan17x
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSBPCPDPC20Titan17x::~BPSBPCPDPC20Titan17x()
{
    CAMX_FREE(m_pModuleConfig);
    m_pModuleConfig = NULL;

    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSBPCPDPC20Titan17x::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSBPCPDPC20Titan17x::DumpRegConfig()
{
    BPSBPCPDPC20ModuleConfig* pModuleCfg = static_cast<BPSBPCPDPC20ModuleConfig*>(m_pModuleConfig);
    BPSBPCPDPC20RegCmd*       pRegCmd    = static_cast<BPSBPCPDPC20RegCmd*>(m_pRegCmd);

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "********  BPS BPCPDPC20 ********  \n");
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "* BPCPDPC20 module CFG [HEX]\n");
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Module Config                = %x\n", pModuleCfg->moduleConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "* BPCPDPC20 CFG [HEX] \n");
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC Black level                = %x\n", pRegCmd->config0.PDPCBlackLevel);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR Exposure ratio              = %x\n", pRegCmd->config0.HDRExposureRatio);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "BPC pixel threshold             = %x\n", pRegCmd->config0.BPCPixelThreshold);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "BPC/BCC detection offset        = %x\n", pRegCmd->config0.badPixelDetectionOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF R/G white balance gain     = %x\n", pRegCmd->config0.PDAFRGWhiteBalanceGain);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF B/G white balance gain     = %x\n", pRegCmd->config0.PDAFBGWhiteBalanceGain);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF G/R white balance gain     = %x\n", pRegCmd->config0.PDAFGRWhiteBalanceGain);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF G/B white balance gain     = %x\n", pRegCmd->config0.PDAFGBWhiteBalanceGain);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF location offset config     = %x\n", pRegCmd->config0.PDAFLocationOffsetConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF location end config        = %x\n", pRegCmd->config0.PDAFLocationEndConfig);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "BPC/BCC  offset for exposure T2 = %x\n", pRegCmd->config1.badPixelDetectionOffsetT2);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC saturation threshold       = %x\n", pRegCmd->config1.PDPCSaturationThreshold);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF tab offset config          = %x\n", pRegCmd->config1.PDAFTabOffsetConfig);
}


CAMX_NAMESPACE_END