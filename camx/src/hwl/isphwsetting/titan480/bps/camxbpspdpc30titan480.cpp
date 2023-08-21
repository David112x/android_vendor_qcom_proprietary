////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpspdpc30titan480.cpp
/// @brief BPSPDPC30Titan480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpspdpc30titan480.h"
#include "pdpc30setting.h"
#include "camxiqinterface.h"
#include "titan480_bps.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC register Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BPSPDPC30RegConfig0
{
    BPS_BPS_0_CLC_BPC_PDPC_PDPC_BLACK_LEVEL      PDPCBlackLevel;            ///< PDPC Black level
    BPS_BPS_0_CLC_BPC_PDPC_PDAF_HDR_EXP_RATIO    HDRExposureRatio;          ///< HDR exposure ratio
    BPS_BPS_0_CLC_BPC_PDPC_BAD_PIXEL_THRESHOLDS  badPixelThreshold;         ///< Bad Pixel threshold
    BPS_BPS_0_CLC_BPC_PDPC_BAD_PIXEL_DET_OFFSET  badPixelDetectionOffset;   ///< Bad Pixel detection offset
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
struct BPSPDPC30RegConfig1
{
    BPS_BPS_0_CLC_BPC_PDPC_BAD_PIXEL_THRESHOLDS_FLAT    badPixelThresholdFlat;      ///< BPC/BCC bad pixle threshold flat
    BPS_BPS_0_CLC_BPC_PDPC_SATURATION_THRESHOLD         PDPCSaturationThreshold;    ///< PDPC saturation threshold
    BPS_BPS_0_CLC_BPC_PDPC_PDAF_TAB_OFFSET_CFG          PDAFTabOffsetConfig;        ///< PDAF tab offset config
    BPS_BPS_0_CLC_BPC_PDPC_DD_THRESHOLD_RATIO           ddThresholdRatio;           ///< DD threshold ratio
    BPS_BPS_0_CLC_BPC_PDPC_FLAT_TH_RECIP                flatThresholdRecip;         ///< Flat threshold reciprocal
    BPS_BPS_0_CLC_BPC_PDPC_BAD_PIXEL_DET_OFFSET_FLAT    badPixelDetOffsetFlat;      ///< Bad pixel detect offset flat
    BPS_BPS_0_CLC_BPC_PDPC_THIN_LINE_NOISE_OFFSET       thinLineNoiseOffset;        ///< Thin line noise offset
    BPS_BPS_0_CLC_BPC_PDPC_GIC_FILTER_CFG               GICFilterConfig;            ///< GIC filter configure
    BPS_BPS_0_CLC_BPC_PDPC_FMAX_GIC                     fmaxGIC;                    ///< GIC Factor threshold
    BPS_BPS_0_CLC_BPC_PDPC_BPC_OFFSET_GIC               bpcOffsetGIC;               ///< GIC bad pixel offset
} CAMX_PACKED;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC register Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BPSPDPC30RegCmd
{
    BPSPDPC30RegConfig0 config0;    ///< BPC/PDPC Config 0
    BPSPDPC30RegConfig1 config1;    ///< BPC/PDPC Config 1
} CAMX_PACKED;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief BPC PDPC module Configuration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BPSPDPC30ModuleConfig
{
    BPS_BPS_0_CLC_BPC_PDPC_MODULE_LUT_BANK_CFG  moduleLUTConfig;   ///< BPCPDPC30 Module config
    BPS_BPS_0_CLC_BPC_PDPC_MODULE_CFG           moduleConfig;      ///< BPCPDPC30 Module LUT config
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 BPSPDPC30RegConfig0LengthDWord = sizeof(BPSPDPC30RegConfig0) / sizeof(UINT32);
static const UINT32 BPSPDPC30RegConfig1LengthDWord = sizeof(BPSPDPC30RegConfig1) / sizeof(UINT32);

static const UINT32 BPSPDPC30LUTBank0   = BPS_BPS_0_CLC_BPC_PDPC_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK0;
static const UINT32 BPSPDPC30LUTBank1   = BPS_BPS_0_CLC_BPC_PDPC_DMI_LUT_BANK_CFG_BANK_SEL_LUT_BANK1;

static const UINT32 BPSPDPCPDAFPDLUTLength      = sizeof(UINT64) * BPSPDPC30DMILengthPDAF;
static const UINT32 BPSPDPCNoiseStdLUTLength    = sizeof(UINT32) * BPSPDPC30DMILengthNoise;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPDPC30Titan480::BPSPDPC30Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSPDPC30Titan480::BPSPDPC30Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPDPC30Titan480::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPDPC30Titan480::Create(
    ISPHWSetting** ppHWSetting)
{
    CamxResult result = CamxResultSuccess;

    BPSPDPC30Titan480* pHWSetting = CAMX_NEW BPSPDPC30Titan480;

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
/// BPSPDPC30Titan480::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPDPC30Titan480::Initialize()
{
    CamxResult result = CamxResultSuccess;

    m_pModuleConfig = CAMX_CALLOC(sizeof(BPSPDPC30ModuleConfig));
    if (NULL == m_pModuleConfig)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "No memory for module configuration!");
    }
    else
    {
        m_pRegCmd = CAMX_CALLOC(sizeof(BPSPDPC30RegCmd));
        if (NULL != m_pRegCmd)
        {
            SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSPDPC30RegConfig0LengthDWord) +
                PacketBuilder::RequiredWriteRegRangeSizeInDwords(BPSPDPC30RegConfig1LengthDWord));
            Set64bitDMILength(0);
            Set32bitDMILength((BPSPDPC30DMILengthPDAF * 2) + BPSPDPC30DMILengthNoise);
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
// BPSPDPC30Titan480::WriteLUTtoDMI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPDPC30Titan480::WriteLUTtoDMI(
    VOID*   pInput)
{
    CamxResult      result      = CamxResultSuccess;
    ISPInputData*   pInputData  = static_cast<ISPInputData*>(pInput);
    UINT32          offset      = 0;

    // CDM pack the DMI buffer and patch the Red and Blue LUT DMI buffer into CDM pack
    if ((NULL != pInputData) && (NULL != pInputData->pDMICmdBuffer))
    {
        result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                         regBPS_BPS_0_CLC_BPC_PDPC_DMI_CFG,
                                         BPS_BPS_0_CLC_BPC_PDPC_DMI_LUT_CFG_LUT_SEL_PDAF_LUT,
                                         pInputData->p32bitDMIBuffer,
                                         offset,
                                         BPSPDPCPDAFPDLUTLength);
        if (CamxResultSuccess == result)
        {
            offset += BPSPDPCPDAFPDLUTLength;
            result = PacketBuilder::WriteDMI(pInputData->pDMICmdBuffer,
                                             regBPS_BPS_0_CLC_BPC_PDPC_DMI_CFG,
                                             BPS_BPS_0_CLC_BPC_PDPC_DMI_LUT_CFG_LUT_SEL_NOISE_STD2_LUT,
                                             pInputData->p32bitDMIBuffer,
                                             offset,
                                             BPSPDPCNoiseStdLUTLength);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write Noise DMI data");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Failed to write PDAF DMI data");
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
// BPSPDPC30Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPDPC30Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    CamxResult      result     = CamxResultSuccess;
    ISPInputData*   pInputData = static_cast<ISPInputData*>(pSettingData);

    BPSPDPC30ModuleConfig* pModuleCfg = static_cast<BPSPDPC30ModuleConfig*>(m_pModuleConfig);
    BPSPDPC30RegCmd*       pRegCmd    = static_cast<BPSPDPC30RegCmd*>(m_pRegCmd);

    if (0 != pModuleCfg->moduleConfig.bitfields.EN)
    {
        if (0 != pModuleCfg->moduleConfig.bitfields.PDAF_PDPC_EN)
        {
            result = WriteLUTtoDMI(pInputData);
        }

        if ((CamxResultSuccess == result) && (NULL != pInputData->pCmdBuffer))
        {
            CmdBuffer* pCmdBuffer = pInputData->pCmdBuffer;
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regBPS_BPS_0_CLC_BPC_PDPC_PDPC_BLACK_LEVEL,
                                                  BPSPDPC30RegConfig0LengthDWord,
                                                  reinterpret_cast<UINT32*>(&pRegCmd->config0));

            if (CamxResultSuccess == result)
            {
                result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                      regBPS_BPS_0_CLC_BPC_PDPC_BAD_PIXEL_THRESHOLDS_FLAT,
                                                      BPSPDPC30RegConfig1LengthDWord,
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
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPDPC30Titan480::UpdateFirmwareData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPDPC30Titan480::UpdateFirmwareData(
    VOID*   pSettingData,
    BOOL    moduleEnable)
{
    ISPInputData*  pInputData     = static_cast<ISPInputData*>(pSettingData);
    BpsIQSettings* pBPSIQSettings = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);

    BPSPDPC30ModuleConfig* pModuleCfg = static_cast<BPSPDPC30ModuleConfig*>(m_pModuleConfig);
    BPSPDPC30RegCmd*       pRegCmd    = static_cast<BPSPDPC30RegCmd*>(m_pRegCmd);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->pdpcParameters_480.moduleCfg.EN                |= moduleEnable;

    pBPSIQSettings->pdpcParameters_480.moduleCfg.PDAF_PDPC_EN       = pModuleCfg->moduleConfig.bitfields.PDAF_PDPC_EN;
    pBPSIQSettings->pdpcParameters_480.moduleCfg.BPC_EN             = pModuleCfg->moduleConfig.bitfields.BPC_EN;

    pBPSIQSettings->pdpcParameters_480.moduleCfg.USESAMECHANNEL_ONLY                =
        pModuleCfg->moduleConfig.bitfields.USESAMECHANNEL_ONLY;
    pBPSIQSettings->pdpcParameters_480.moduleCfg.SINGLEBPC_ONLY                     =
        pModuleCfg->moduleConfig.bitfields.SINGLEBPC_ONLY;
    pBPSIQSettings->pdpcParameters_480.moduleCfg.FLAT_DETECTION_EN                  =
        pModuleCfg->moduleConfig.bitfields.FLAT_DETECTION_EN;


    if (1 == pBPSIQSettings->pdpcParameters_480.moduleCfg.PDAF_PDPC_EN)
    {
        pBPSIQSettings->pdpcParameters_480.moduleCfg.DBPC_EN                            = 1;
    }
    else
    {
        pBPSIQSettings->pdpcParameters_480.moduleCfg.DBPC_EN = pModuleCfg->moduleConfig.bitfields.DBPC_EN;
    }

    pBPSIQSettings->pdpcParameters_480.moduleCfg.BAYER_PATTERN                      =
        pModuleCfg->moduleConfig.bitfields.BAYER_PATTERN;
    pBPSIQSettings->pdpcParameters_480.moduleCfg.PDAF_HDR_SELECTION                 =
        pModuleCfg->moduleConfig.bitfields.PDAF_HDR_SELECTION;
    pBPSIQSettings->pdpcParameters_480.moduleCfg.PDAF_ZZHDR_FIRST_RB_EXP            =
        pModuleCfg->moduleConfig.bitfields.PDAF_ZZHDR_FIRST_RB_EXP;

    pBPSIQSettings->pdpcParameters_480.pdafGlobalOffsetX                            =
        pRegCmd->config0.PDAFLocationOffsetConfig.bitfields.X_OFFSET;
    pBPSIQSettings->pdpcParameters_480.pdafGlobalOffsetY                            =
        pRegCmd->config0.PDAFLocationOffsetConfig.bitfields.Y_OFFSET;
    pBPSIQSettings->pdpcParameters_480.pdafTableOffsetX                             =
        pRegCmd->config1.PDAFTabOffsetConfig.bitfields.X_OFFSET;
    pBPSIQSettings->pdpcParameters_480.pdafTableOffsetY                             =
        pRegCmd->config1.PDAFTabOffsetConfig.bitfields.Y_OFFSET;
    pBPSIQSettings->pdpcParameters_480.pdafXEnd                                     =
        pRegCmd->config0.PDAFLocationEndConfig.bitfields.X_END;
    pBPSIQSettings->pdpcParameters_480.pdafYEnd                                     =
        pRegCmd->config0.PDAFLocationEndConfig.bitfields.Y_END;

    // Post metdata
    pInputData->pCalculatedData->hotPixelMode = pInputData->pHALTagsData->hotPixelMode;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPDPC30Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPDPC30Titan480::UpdateTuningMetadata(
    VOID* pSettingData)
{
    CamxResult         result             = CamxResultSuccess;
    ISPInputData*      pInputData         = static_cast<ISPInputData*>(pSettingData);
    BPSTuningMetadata* pBPSTuningMetadata = static_cast<BPSTuningMetadata*>(pInputData->pBPSTuningMetadata);
    BpsIQSettings*     pBPSIQSettings     = static_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    BPSPDPC30RegCmd*   pRegCmd            = static_cast<BPSPDPC30RegCmd*>(m_pRegCmd);

    // Post tuning metadata if setting is enabled
    if (NULL != pBPSTuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(BPSPDPC30RegConfig0) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSBBPCPDPCData.BPCPDPConfig0));

        CAMX_STATIC_ASSERT(sizeof(BPSPDPC30RegConfig1) <=
            sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSBBPCPDPCData.BPCPDPConfig1));

        CAMX_STATIC_ASSERT(sizeof(BPSPDPC30RegCmd) <= sizeof(pBPSTuningMetadata->BPSTuningMetadata480.BPSBBPCPDPCData));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata480.BPSBBPCPDPCData.BPCPDPConfig0,
                      &pRegCmd->config0,
                      sizeof(BPSPDPC30RegConfig0));

        Utils::Memcpy(&pBPSTuningMetadata->BPSTuningMetadata480.BPSBBPCPDPCData.BPCPDPConfig1,
                      &pRegCmd->config1,
                      sizeof(BPSPDPC30RegConfig1));

        if ((TRUE == pBPSIQSettings->pdpcParameters_480.moduleCfg.EN) &&
            (NULL != pInputData->pipelineBPSData.pDebugDataWriter))
        {
            result = pInputData->pipelineBPSData.pDebugDataWriter->AddDataTag(
                DebugDataTagID::TuningBPSBPCPDPC30Register,
                DebugDataTagType::TuningBPSBPCPDPC30Config,
                1,
                &pBPSTuningMetadata->BPSTuningMetadata480.BPSBBPCPDPCData,
                sizeof(BPSPDPC30RegCmd));

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_WARN(CamxLogGroupBPS, "AddDataTag failed with error: %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPDPC30Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSPDPC30Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult           result = CamxResultSuccess;
    PDPC30UnpackedField* pData  = static_cast<PDPC30UnpackedField*>(pInput);
    BPSPDPC30OutputData* pOut   = static_cast<BPSPDPC30OutputData*>(pOutput);

    BPSPDPC30ModuleConfig* pModuleCfg = static_cast<BPSPDPC30ModuleConfig*>(m_pModuleConfig);
    BPSPDPC30RegCmd*       pRegCmd    = static_cast<BPSPDPC30RegCmd*>(m_pRegCmd);

    if ((NULL != pInput) && (NULL != pData) && (NULL != pOut))
    {
        pRegCmd->config0.PDPCBlackLevel.bitfields.BLACK_LEVEL = pData->blackLevel;

        // for HDR
        pRegCmd->config0.HDRExposureRatio.bitfields.EXP_RATIO = pData->PDAFHDRExposureRatio;
        pRegCmd->config0.HDRExposureRatio.bitfields.EXP_RATIO_RECIP = pData->PDAFHDRExposureRatioRecip;

        pRegCmd->config0.badPixelThreshold.bitfields.FMAX_PIXEL = pData->fmax;
        pRegCmd->config0.badPixelThreshold.bitfields.FMIN_PIXEL = pData->fmin;

        pRegCmd->config0.badPixelDetectionOffset.bitfields.BCC_OFFSET = pData->bccOffset;
        pRegCmd->config0.badPixelDetectionOffset.bitfields.BPC_OFFSET = pData->bpcOffset;

        pRegCmd->config0.PDAFRGWhiteBalanceGain.bitfields.RG_WB_GAIN = pData->rgWbGain4096;
        pRegCmd->config0.PDAFBGWhiteBalanceGain.bitfields.BG_WB_GAIN = pData->bgWbGain4096;
        pRegCmd->config0.PDAFGRWhiteBalanceGain.bitfields.GR_WB_GAIN = pData->grWbGain4096;
        pRegCmd->config0.PDAFGBWhiteBalanceGain.bitfields.GB_WB_GAIN = pData->gbWbGain4096;

        pRegCmd->config0.PDAFLocationOffsetConfig.bitfields.X_OFFSET = pData->PDAFGlobalOffsetX;
        pRegCmd->config0.PDAFLocationOffsetConfig.bitfields.Y_OFFSET = pData->PDAFGlobalOffsetY;

        pRegCmd->config0.PDAFLocationEndConfig.bitfields.X_END = pData->PDAFXend;
        pRegCmd->config0.PDAFLocationEndConfig.bitfields.Y_END = pData->PDAFYend;

        pRegCmd->config1.badPixelThresholdFlat.bitfields.FMAX_PIXEL = pData->fmaxFlat;
        pRegCmd->config1.badPixelThresholdFlat.bitfields.FMIN_PIXEL = pData->fminFlat;

        pRegCmd->config1.PDPCSaturationThreshold.bitfields.SAT_THRESHOLD = pData->saturationThreshold;

        pRegCmd->config1.PDAFTabOffsetConfig.bitfields.X_OFFSET = pData->PDAFTableXOffset;
        pRegCmd->config1.PDAFTabOffsetConfig.bitfields.Y_OFFSET = pData->PDAFTableYOffset;

        pRegCmd->config1.ddThresholdRatio.bitfields.DIR_OFFSET  = pData->dirOffset;
        pRegCmd->config1.ddThresholdRatio.bitfields.DIR_TK      = pData->dirTk;

        pRegCmd->config1.flatThresholdRecip.bitfields.VALUE     = pData->flatThRecip;
        pRegCmd->config1.fmaxGIC.bitfields.VALUE                = pData->fmaxGIC;
        pRegCmd->config1.bpcOffsetGIC.bitfields.VALUE           = pData->bpcOffsetGIC;

        pRegCmd->config1.badPixelDetOffsetFlat.bitfields.BCC_OFFSET_FLAT = pData->bccOffsetFlat;
        pRegCmd->config1.badPixelDetOffsetFlat.bitfields.BPC_OFFSET_FLAT = pData->bpcOffsetFlat;

        pRegCmd->config1.thinLineNoiseOffset.bitfields.VALUE = pData->gicThinLineNoiseOffset;
        pRegCmd->config1.GICFilterConfig.bitfields.GIC_FILTER_STRENGTH = pData->gicFilterStrength;

        pModuleCfg->moduleLUTConfig.bitfields.BANK_SEL = pData->LUTBankSelection;


        // fill PDAF LUT
        for (UINT32 index = 0; index < BPSPDPC30DMILengthPDAF; index++)
        {
            // Need to check why pdpc3.0 use 64 bit in the common library. This might cause data lost
            pOut->pDMIDataPtrPDAF[index] = pData->PDAFPDMask[pData->LUTBankSelection][index];
        }

        // fill Noise LUT
        for (UINT32 index = 0; index < BPSPDPC30DMILengthNoise; index++)
        {
            pOut->pDMIDataPtrPNoise[index] = pData->noiseStdLUTLevel0[pData->LUTBankSelection][index];
        }

        pModuleCfg->moduleConfig.bitfields.EN                        = pData->enable;
        pModuleCfg->moduleConfig.bitfields.STRIPE_AUTO_CROP_DIS      = 1; // disable auto crop
        pModuleCfg->moduleConfig.bitfields.PDAF_PDPC_EN              = pData->PDAFPDPCEnable;
        pModuleCfg->moduleConfig.bitfields.BPC_EN                    = pData->PDAFBPCEnable;
        pModuleCfg->moduleConfig.bitfields.USESAMECHANNEL_ONLY       = pData->useSameChannelOnly;
        pModuleCfg->moduleConfig.bitfields.SINGLEBPC_ONLY            = pData->singleBPCOnly;
        pModuleCfg->moduleConfig.bitfields.FLAT_DETECTION_EN         = pData->flatDetectionEn;
        pModuleCfg->moduleConfig.bitfields.DBPC_EN                   = pData->directionalBPCEnable;
        pModuleCfg->moduleConfig.bitfields.BAYER_PATTERN             = pData->bayerPattern;
        pModuleCfg->moduleConfig.bitfields.PDAF_HDR_SELECTION        = pData->PDAFHDRSelection;
        pModuleCfg->moduleConfig.bitfields.PDAF_ZZHDR_FIRST_RB_EXP   = pData->PDAFzzHDRFirstrbExposure;
        pModuleCfg->moduleConfig.bitfields.GIC_EN                    = pData->PDAFGICEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Input data is NULL %p %p %p", pInput, pData, pOut);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPDPC30Titan480::~BPSPDPC30Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSPDPC30Titan480::~BPSPDPC30Titan480()
{
    CAMX_FREE(m_pModuleConfig);
    m_pModuleConfig = NULL;

    CAMX_FREE(m_pRegCmd);
    m_pRegCmd = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSPDPC30Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSPDPC30Titan480::DumpRegConfig()
{
    BPSPDPC30ModuleConfig*  pModuleCfg = static_cast<BPSPDPC30ModuleConfig*>(m_pModuleConfig);
    BPSPDPC30RegCmd*        pRegCmd = static_cast<BPSPDPC30RegCmd*>(m_pRegCmd);

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "********  BPS BPCPDPC30 ********  \n");

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "* BPCPDPC30 module CFG [HEX]\n");
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Module Config                  = %x\n",
        pModuleCfg->moduleConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "Module LUT Config              = %x\n",
        pModuleCfg->moduleLUTConfig.u32All);

    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "* BPCPDPC30 Register CFG [HEX] \n");
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC Black level               = %x\n",
        pRegCmd->config0.PDPCBlackLevel.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "HDR Exposure ratio             = %x\n",
        pRegCmd->config0.HDRExposureRatio.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "BPC pixel threshold            = %x\n",
        pRegCmd->config0.badPixelThreshold.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "BPC/BCC detection offset       = %x\n",
        pRegCmd->config0.badPixelDetectionOffset.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF R/G white balance gain    = %x\n",
        pRegCmd->config0.PDAFRGWhiteBalanceGain.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF B/G white balance gain    = %x\n",
        pRegCmd->config0.PDAFBGWhiteBalanceGain.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF G/R white balance gain    = %x\n",
        pRegCmd->config0.PDAFGRWhiteBalanceGain.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF G/B white balance gain    = %x\n",
        pRegCmd->config0.PDAFGBWhiteBalanceGain.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF location offset config    = %x\n",
        pRegCmd->config0.PDAFLocationOffsetConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF location end config       = %x\n",
        pRegCmd->config0.PDAFLocationEndConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC saturation threshold      = %x\n",
        pRegCmd->config1.PDPCSaturationThreshold.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDAF tab offset config         = %x\n",
        pRegCmd->config1.PDAFTabOffsetConfig.u32All);
}


CAMX_NAMESPACE_END
