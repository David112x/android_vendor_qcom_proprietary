////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifepdpc30titan480.cpp
/// @brief CAMXIFEPDPC30TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifepdpc30titan480.h"
#include "pdpc30setting.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30Titan480::IFEPDPC30Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPDPC30Titan480::IFEPDPC30Titan480()
{
    SetCommandLength(PacketBuilder::RequiredWriteRegRangeSizeInDwords(IFEPDPC30RegLengthDword) +
                     PacketBuilder::RequiredWriteDMISizeInDwords() * 2);
    Set32bitDMILength(IFEPDPC30DMILengthDword);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30Titan480::CreateCmdList(
    VOID*   pSettingData,
    UINT32* pDMIBufferOffset)
{
    CamxResult    result       = CamxResultSuccess;
    ISPInputData* pInputData   = static_cast<ISPInputData*>(pSettingData);
    UINT32        offset       = (*pDMIBufferOffset +
                                 (pInputData->pStripeConfig->stripeId * IFEPDPC30DMILengthDword)) * sizeof(UINT32);
    UINT32        lengthInByte;

    if (NULL != pInputData                  &&
        NULL != pInputData->pCmdBuffer      &&
        NULL != pInputData->p32bitDMIBuffer)
    {
        CmdBuffer*    pCmdBuffer   = pInputData->pCmdBuffer;
        CmdBuffer*    pDMIBuffer   = pInputData->p32bitDMIBuffer;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_BPC_PDPC_DMI_LUT_BANK_CFG,
                                              sizeof(IFEPDPC30ModuleConfig) / sizeof(UINT32),
                                              reinterpret_cast<UINT32*>(&m_regCmd.config));
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_BPC_PDPC_PDPC_BLACK_LEVEL,
                                              sizeof(IFEPDPC30RegConfig0) / sizeof(UINT32),
                                              reinterpret_cast<UINT32*>(&m_regCmd.config0));
        }
        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_BPC_PDPC_BAD_PIXEL_THRESHOLDS_FLAT,
                                              sizeof(IFEPDPC30RegConfig1) / sizeof(UINT32),
                                              reinterpret_cast<UINT32*>(&m_regCmd.config1));
        }
        if (CamxResultSuccess != result)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to write command buffer");
        }

        if (CamxResultSuccess == result)
        {
            lengthInByte = IFEPDPC30PDAFDMISize;

            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_PP_CLC_BPC_PDPC_DMI_CFG,
                                             PDAFLUT,
                                             pDMIBuffer,
                                             offset,
                                             lengthInByte);

            if (CamxResultSuccess == result)
            {
                offset      += lengthInByte;
                lengthInByte = IFEPDPC30NoiseDMISize;

                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_BPC_PDPC_DMI_CFG,
                                                 NoiseLUT,
                                                 pDMIBuffer,
                                                 offset,
                                                 lengthInByte);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupIQMod, "Failed to program DMI buffer");
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30Titan480::UpdateTuningMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30Titan480::UpdateTuningMetadata(
    VOID* pTuningMetaData)
{
    CamxResult         result             = CamxResultSuccess;
    IFETuningMetadata* pIFETuningMetadata = static_cast<IFETuningMetadata*>(pTuningMetaData);

    // Post tuning metadata if setting is enabled
    if (NULL != pIFETuningMetadata)
    {
        CAMX_STATIC_ASSERT(sizeof(IFEPDPC30RegCmd) <= sizeof(pIFETuningMetadata->metadata480.IFEPDPCData.PDPCConfig));
        Utils::Memcpy(&pIFETuningMetadata->metadata480.IFEPDPCData.PDPCConfig, &m_regCmd, sizeof(IFEPDPC30RegCmd));

        if (NULL != m_pDMIPDAFMaskLUTDataPtr)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata480.IFEDMIPacked.PDPCLUT.IFEPDPCMaskLUT,
                          m_pDMIPDAFMaskLUTDataPtr,
                          IFEPDPC30PDAFDMISize);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "IFE Tuning PDAF Mask LUT pointer is NULL");
        }

        if (NULL != m_pDMINoiseLUTDataPtr)
        {
            Utils::Memcpy(pIFETuningMetadata->metadata480.IFEDMIPacked.PDPCLUT.IFEPDPCNoiseLUT,
                          m_pDMINoiseLUTDataPtr,
                          IFEPDPC30NoiseDMISize);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "IFE Tuning Noise LUT pointer is NULL");
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "IFE Tuning metadata is Null");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult              result        = CamxResultSuccess;
    PDPC30UnpackedField*    pData         = static_cast<PDPC30UnpackedField*>(pInput);
    IFEPDPC30OutputData*    pOutputData   = static_cast<IFEPDPC30OutputData*>(pOutput);
    ISPInputData*           pInputData    = NULL;

    if (NULL != pData && NULL != pOutputData)
    {
        pInputData = static_cast<ISPInputData*>(pOutputData->pSettingsData);

        /// @todo (CAMX-1399) Verify all the values from unpacked are falls below bitfields
        m_regCmd.config0.pdpcBlackLevel.bitfields.BLACK_LEVEL               = pData->blackLevel;
        m_regCmd.config0.hdrExposureRatio.bitfields.EXP_RATIO               = pData->PDAFHDRExposureRatio;
        m_regCmd.config0.hdrExposureRatio.bitfields.EXP_RATIO_RECIP         = pData->PDAFHDRExposureRatioRecip;
        m_regCmd.config0.bpcPixelThreshold.bitfields.FMAX_PIXEL             = pData->fmax;
        m_regCmd.config0.bpcPixelThreshold.bitfields.FMIN_PIXEL             = pData->fmin;
        m_regCmd.config0.badPixelDetectionOffset.bitfields.BCC_OFFSET       = pData->bccOffset;
        m_regCmd.config0.badPixelDetectionOffset.bitfields.BPC_OFFSET       = pData->bpcOffset;
        m_regCmd.config0.pdafRGWhiteBalanceGain.bitfields.RG_WB_GAIN        = pData->rgWbGain4096;
        m_regCmd.config0.pdafBGWhiteBalanceGain.bitfields.BG_WB_GAIN        = pData->bgWbGain4096;
        m_regCmd.config0.pdafGRWhiteBalanceGain.bitfields.GR_WB_GAIN        = pData->grWbGain4096;
        m_regCmd.config0.pdafGBWhiteBalanceGain.bitfields.GB_WB_GAIN        = pData->gbWbGain4096;
        m_regCmd.config0.pdafLocationOffsetConfig.bitfields.Y_OFFSET        = pData->PDAFGlobalOffsetY;
        m_regCmd.config0.pdafLocationOffsetConfig.bitfields.X_OFFSET        = pData->PDAFGlobalOffsetX;
        m_regCmd.config0.pdafLocationEndConfig.bitfields.Y_END              = pData->PDAFYend;
        m_regCmd.config0.pdafLocationEndConfig.bitfields.X_END              = pData->PDAFXend;
        m_regCmd.config1.pdpcSaturationThreshold.bitfields.SAT_THRESHOLD    = pData->saturationThreshold;
        m_regCmd.config1.pdafTabOffsetConfig.bitfields.Y_OFFSET             = pData->PDAFTableYOffset;
        m_regCmd.config1.pdafTabOffsetConfig.bitfields.X_OFFSET             = pData->PDAFTableXOffset;
        if ((NULL != pInputData) && (NULL != pInputData->pStripeConfig) && (NULL != pInputData->pStripeConfig->pStripeOutput))
        {
            if (TRUE == pInputData->pStripeConfig->overwriteStripes)
            {
                m_regCmd.config1.pdafTabOffsetConfig.bitfields.X_OFFSET =
                    pInputData->pStripeConfig->pStripeOutput->PDPCOutv30.PDAFTableOffset;
            }
        }
        m_regCmd.config1.ddThresholdRatio.bitfields.DIR_TK                  = pData->dirTk;
        m_regCmd.config1.ddThresholdRatio.bitfields.DIR_OFFSET              = pData->dirOffset;
        m_regCmd.config1.bpcPixelThresholdFlat.bitfields.FMAX_PIXEL         = pData->fmaxFlat;
        m_regCmd.config1.bpcPixelThresholdFlat.bitfields.FMIN_PIXEL         = pData->fminFlat;
        m_regCmd.config1.flatThresholdReciprocal.bitfields.VALUE            = pData->flatThRecip;
        m_regCmd.config1.bpcDetectOffsetFlat.bitfields.BPC_OFFSET_FLAT      = pData->bpcOffsetFlat;
        m_regCmd.config1.bpcDetectOffsetFlat.bitfields.BCC_OFFSET_FLAT      = pData->bccOffsetFlat;
        m_regCmd.config1.gicThinLineNoiseOffset.bitfields.VALUE             = pData->gicThinLineNoiseOffset;
        m_regCmd.config1.gicFilterStrength.bitfields.GIC_FILTER_STRENGTH    = pData->gicFilterStrength;
        m_regCmd.config1.fmaxGIC.bitfields.VALUE                            = pData->fmaxGIC;
        m_regCmd.config1.bpcOffsetGIC.bitfields.VALUE                       = pData->bpcOffsetGIC;

        Utils::Memcpy(static_cast<VOID*>(pOutputData->pDMINoiseStdLUTDataPtr),
                      static_cast<const VOID*>(&pData->noiseStdLUTLevel0[pData->LUTBankSelection][0]),
                      sizeof(UINT32) * PDPC30_NOISESTD_LENGTH);

        Utils::Memcpy(static_cast<VOID*>(pOutputData->pDMIPDAFMaskLUTDataPtr),
                      static_cast<const VOID*>(&pData->PDAFPDMask[pData->LUTBankSelection][0]),
                      IFEPDPC30PDAFDMISize);

        m_regCmd.config.moduleConfig.bitfields.EN                      = pData->enable;
        m_regCmd.config.moduleConfig.bitfields.STRIPE_AUTO_CROP_DIS    = 1;
        m_regCmd.config.moduleConfig.bitfields.PDAF_PDPC_EN            = pData->PDAFPDPCEnable;
        m_regCmd.config.moduleConfig.bitfields.BPC_EN                  = pData->PDAFBPCEnable;
        m_regCmd.config.moduleConfig.bitfields.USESAMECHANNEL_ONLY     = pData->useSameChannelOnly;
        m_regCmd.config.moduleConfig.bitfields.SINGLEBPC_ONLY          = pData->singleBPCOnly;
        m_regCmd.config.moduleConfig.bitfields.FLAT_DETECTION_EN       = pData->flatDetectionEn;
        m_regCmd.config.moduleConfig.bitfields.DBPC_EN                 = pData->directionalBPCEnable;
        m_regCmd.config.moduleConfig.bitfields.BAYER_PATTERN           = pData->bayerPattern;
        m_regCmd.config.moduleConfig.bitfields.PDAF_HDR_SELECTION      = pData->PDAFHDRSelection;
        m_regCmd.config.moduleConfig.bitfields.PDAF_ZZHDR_FIRST_RB_EXP = pData->PDAFzzHDRFirstrbExposure;

        if ((NULL != pInputData) && (NULL != pInputData->pCalculatedMetadata))
        {
            m_regCmd.config.moduleConfig.bitfields.CHANNEL_BALANCE_EN = pInputData->pCalculatedMetadata->demuxEnable;
        }

        m_regCmd.config.moduleConfig.bitfields.GIC_EN                  = pData->PDAFGICEnable;
        m_regCmd.config.DMILUTBankConfig.bitfields.BANK_SEL            = pData->LUTBankSelection;
        m_regCmd.config.moduleLUTBankConfig.bitfields.BANK_SEL         = pData->LUTBankSelection;

        m_pDMINoiseLUTDataPtr                                          = pOutputData->pDMINoiseStdLUTDataPtr;
        m_pDMIPDAFMaskLUTDataPtr                                       = pOutputData->pDMIPDAFMaskLUTDataPtr;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Input params are NULL pData=0x%p", pData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30Titan480::CreateSubCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30Titan480::CreateSubCmdList(
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
            regIFE_IFE_0_PP_CLC_BPC_PDPC_MODULE_CFG,
            sizeof(m_regCmd.config.moduleConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.config.moduleConfig));
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupIQMod, "Null Input Data");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEPDPC30Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult result = CamxResultSuccess;

    BOOL* pModuleEnable = static_cast<BOOL*>(pInput);

    if (NULL != pInput)
    {
        m_regCmd.config.moduleConfig.bitfields.EN = *pModuleEnable;
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30Titan480::~IFEPDPC30Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEPDPC30Titan480::~IFEPDPC30Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEPDPC30Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEPDPC30Titan480::DumpRegConfig()
{
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 Module config           0x%x",
        m_regCmd.config.moduleConfig.u32All);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 Black Level             0x%x",
        m_regCmd.config0.pdpcBlackLevel.bitfields.BLACK_LEVEL);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 EXP Ratio               0x%x",
        m_regCmd.config0.hdrExposureRatio.bitfields.EXP_RATIO);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 EXP Ratio Recipe        0x%x",
        m_regCmd.config0.hdrExposureRatio.bitfields.EXP_RATIO_RECIP);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 BCC Offset              0x%x",
        m_regCmd.config0.badPixelDetectionOffset.bitfields.BCC_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 BPC Offset              0x%x",
        m_regCmd.config0.badPixelDetectionOffset.bitfields.BPC_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 RG WB Gain              0x%x",
        m_regCmd.config0.pdafRGWhiteBalanceGain.bitfields.RG_WB_GAIN);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 BG WB Gain              0x%x",
        m_regCmd.config0.pdafBGWhiteBalanceGain.bitfields.BG_WB_GAIN);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 GR WB Gain              0x%x",
        m_regCmd.config0.pdafGRWhiteBalanceGain.bitfields.GR_WB_GAIN);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 GB WB Gain              0x%x",
        m_regCmd.config0.pdafGBWhiteBalanceGain.bitfields.GB_WB_GAIN);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 PDAF Y Offset           0x%x",
        m_regCmd.config0.pdafLocationOffsetConfig.bitfields.Y_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 PDAF X Offset           0x%x",
        m_regCmd.config0.pdafLocationOffsetConfig.bitfields.X_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 PDAF Y End              0x%x",
        m_regCmd.config0.pdafLocationEndConfig.bitfields.Y_END);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 PDAF X End              0x%x",
        m_regCmd.config0.pdafLocationEndConfig.bitfields.X_END);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 Saturation Threshold    0x%x",
        m_regCmd.config1.pdpcSaturationThreshold.bitfields.SAT_THRESHOLD);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 PDAF Table Y Offset     0x%x",
        m_regCmd.config1.pdafTabOffsetConfig.bitfields.Y_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 PDAF Table X Offset     0x%x",
        m_regCmd.config1.pdafTabOffsetConfig.bitfields.X_OFFSET);
    CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "PDPC30 Bayer pattern           0x%x",
        m_regCmd.config.moduleConfig.bitfields.BAYER_PATTERN);
}

CAMX_NAMESPACE_END
