////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebfstats25titan480.cpp
/// @brief CAMXIFEBFSTATS25TITAN480 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxutils.h"
#include "camxifebfstats25titan480.h"
#include "camxispstatsmodule.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::IFEBFStats25Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBFStats25Titan480::IFEBFStats25Titan480()
{
    // Accumulate the total command length
    const UINT32 regCmdLengthInBytes[] =
    {
        sizeof(IFEBF25Titan480Config),
        sizeof(IFEBF25Titan480HFIRIIRConfig),
        sizeof(IFEBF25Titan480VIIRConfig),
        sizeof(IFEBF25Titan480ActiveWindow),
        sizeof(IFE_IFE_0_PP_CLC_STATS_BAF_SHIFT_BITS_CFG),
        sizeof(IFEBF25Titan480HThresholdCoringConfig),
        sizeof(IFEBF25Titan480VThresholdCoringConfig),
        sizeof(IFEBF25Titan480CoringConfig),
        sizeof(IFEBF25Titan480PhaseStripeInterruptConfig)
    };

    UINT32 totalCommandLength = 0;
    for (UINT index = 0; index < CAMX_ARRAY_SIZE(regCmdLengthInBytes); index++)
    {
        totalCommandLength += PacketBuilder::RequiredWriteRegRangeSizeInDwords(regCmdLengthInBytes[index] /
                                                                               RegisterWidthInBytes);
    }

    // Acoount for two DMI commands: Gamma LUT and ROI Index LUT
    totalCommandLength += PacketBuilder::RequiredWriteDMISizeInDwords() * 2;
    SetCommandLength(totalCommandLength);

    Set32bitDMILength(IFEBF25GammaLUTDMILengthDWord + IFEBF25ROIDMILengthDWord);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats25Titan480::CreateCmdList(
    VOID*   pInput,
    UINT32* pDMIBufferOffset)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pDMIBufferOffset);

    const BFStats25ConfigData*  pConfigData = static_cast<BFStats25ConfigData*>(pInput);

    const ISPInputData* pInputData  = pConfigData->pISPInputData;
    CmdBuffer*          pCmdBuffer  = NULL;

    if ((NULL != pInputData->pCmdBuffer) &&
        (NULL != pInputData->p32bitDMIBuffer))
    {
        pCmdBuffer = pInputData->pCmdBuffer;

        const UINT32                m_32bitDMIBufferOffsetDword = pDMIBufferOffset[0];
        const BFStatsROIConfigType* pHwROIConfigArray           = pConfigData->pHwROIConfig;

        const BFStats25ROIIndexLUTType*         pROIDMIConfig   = pConfigData->pROIDMIConfig;
        const UINT32*                           pGammaLUTArray  = pConfigData->pGammaLUT;

        if (TRUE == pInputData->bankUpdate.isValid)
        {
            // OverWrite the DMI Bank
            pInputData->pStripeConfig->stateBF.ROIIndexLUTBank = pInputData->bankUpdate.BFStatsDMIBank;
            pInputData->pStripeConfig->stateBF.gammaLUTBank    = pInputData->bankUpdate.BFStatsDMIBank;
        }

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "gammaLUTBank = %d, ROIIndexLUTBank = %d",
                         pInputData->pStripeConfig->stateBF.gammaLUTBank,
                         pInputData->pStripeConfig->stateBF.ROIIndexLUTBank);

        // Enable this module
        m_regCmd.BFConfig.config.bitfields.EN = 1;

        // Set Gamma LUT enable flag
        m_regCmd.BFConfig.config.bitfields.GAMMA_LUT_EN = pConfigData->enableGammaLUT;

        // Select same bank for: (1) DMI LUT bank select configuration and (2) module's DMI LUT bank select
        m_regCmd.BFConfig.DMILUTBankconfig.bitfields.BANK_SEL   = pInputData->pStripeConfig->stateBF.ROIIndexLUTBank;
        m_regCmd.BFConfig.moduleLUTBankConfig.bits.BANK_SEL     = pInputData->pStripeConfig->stateBF.ROIIndexLUTBank;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "DMILUT=%d modLUT=%d, isInitPacket=%d frameNum = %llu",
                         m_regCmd.BFConfig.DMILUTBankconfig.bitfields.BANK_SEL,
                         m_regCmd.BFConfig.moduleLUTBankConfig.bits.BANK_SEL,
                         pInputData->isInitPacket,
                         pInputData->frameNum);

        // Save it so that next time we will select different bank
        pInputData->pStripeConfig->stateBF.gammaLUTBank    ^= 1;
        pInputData->pStripeConfig->stateBF.ROIIndexLUTBank ^= 1;

        // In this set of registers write operation, module level configuration and DMI LUT bank select are programmed together
        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_PP_CLC_STATS_BAF_DMI_LUT_BANK_CFG,
                                              sizeof(IFEBF25Titan480Config) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&m_regCmd.BFConfig));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to write BF module and DMI LUT BANK CFG command.");
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_BAF_H_1_FIR_CFG_0,
                                                  sizeof(IFEBF25Titan480HFIRIIRConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.hFIRIIRConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_BAF_V_IIR_CFG_0,
                                                  sizeof(IFEBF25Titan480VIIRConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.vIIRConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_BAF_ROI_XY_MIN,
                                                  sizeof(IFEBF25Titan480ActiveWindow) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.activeWindow));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_BAF_SHIFT_BITS_CFG,
                                                  sizeof(IFE_IFE_0_PP_CLC_STATS_BAF_SHIFT_BITS_CFG) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.shiftBitsConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_BAF_H_1_TH_CFG,
                                                  sizeof(IFEBF25Titan480HThresholdCoringConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.hThresholdCoringConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_BAF_V_TH_CFG,
                                                  sizeof(IFEBF25Titan480VThresholdCoringConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.vThresholdCoringConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_BAF_CORING_GAIN_CFG_0,
                                                  sizeof(IFEBF25Titan480CoringConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.coringConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_PP_CLC_STATS_BAF_SCALE_H_IMAGE_SIZE_CFG,
                                                  sizeof(IFEBF25Titan480PhaseStripeInterruptConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.phaseStripeConfig));
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to write command buffer");
        }

        // Write 96-bit LUT size of BF ROI configuration in 32-bit DMI
        if (CamxResultSuccess == result)
        {
            // Following 32-bit DMI command buffer and the buffer address will be used for both ROI Index LUT and Gamma LUT
            CmdBuffer*      pDMI32Buffer     = pInputData->p32bitDMIBuffer;
            UINT32*         pDMI32BufferAddr = pInputData->p32bitDMIBufferAddr;

            // ROI Index LUT specific DMI parameters
            const UINT32    ROIIndexDMIOffsetDWord      = m_32bitDMIBufferOffsetDword +
                                                          (pInputData->pStripeConfig->stripeId * Get32bitDMILength());
            const UINT32    numBFStatsROIs              = pHwROIConfigArray->numBFStatsROIDimension;
            const UINT32    ROIIndexDMILengthInBytes    = numBFStatsROIs * sizeof(BFStats25ROIIndexLUTType);

            const HwEnvironment* pHwEnvironment = HwEnvironment::GetInstance();

            if (CamxResultSuccess == result)
            {
                Utils::Memcpy(pDMI32BufferAddr + ROIIndexDMIOffsetDWord,
                              pROIDMIConfig,
                              (numBFStatsROIs * sizeof(BFStats25ROIIndexLUTType)));

                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_STATS_BAF_DMI_CFG,
                                                 IFEBF25ROIIndexLUTSelect,
                                                 pDMI32Buffer,
                                                 ROIIndexDMIOffsetDWord * sizeof(UINT32),
                                                 ROIIndexDMILengthInBytes);
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid BF ROI DMI buffer");
            }

            // Write 32-bit DMI BF gamma LUT configuration
            if (TRUE == pConfigData->enableGammaLUT && CamxResultSuccess == result)
            {
                // Gamma LUT specific DMI parameters
                // Add the previous 32-bit DMI lengthInBytes to the offsetDWord for new 32-bit DMI
                const UINT32    gammaDMIOffsetDWord     = ROIIndexDMIOffsetDWord + ROIIndexDMILengthInBytes;
                const UINT32    gammaDMILengthInBytes   = IFEBF25GammaLUTDMILengthDWord * sizeof(UINT32);

                Utils::Memcpy(pDMI32BufferAddr + gammaDMIOffsetDWord,
                              pGammaLUTArray,
                              sizeof(pGammaLUTArray[0]) * MaxBFGammaEntries);

                result = PacketBuilder::WriteDMI(pCmdBuffer,
                                                 regIFE_IFE_0_PP_CLC_STATS_BAF_DMI_CFG,
                                                 IFEBF25GammaLUTSelect,
                                                 pDMI32Buffer,
                                                 gammaDMIOffsetDWord * sizeof(UINT32),
                                                 gammaDMILengthInBytes);

                if (CamxResultSuccess != result)
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Invalid BF gamma LUT DMI buffer");
                }
            }
        }


        if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
        {
            DumpRegConfig();
        }

    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid cmd buffer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::ResetPerFrameData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats25Titan480::ResetPerFrameData(
    const ISPInputData* pInputData)
{
    // memset command registers
    Utils::Memset(&m_regCmd, 0, sizeof(m_regCmd));

    // Restore frame state data
    m_regCmd.BFConfig.DMILUTBankconfig.bitfields.BANK_SEL   = pInputData->pStripeConfig->stateBF.ROIIndexLUTBank;
    m_regCmd.BFConfig.moduleLUTBankConfig.bits.BANK_SEL     = pInputData->pStripeConfig->stateBF.ROIIndexLUTBank;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::LumaConversionConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats25Titan480::LumaConversionConfig(
    const AFConfigParams*                   pInputBFStatsConfig,
    BFStats25LumaConversionConfigInput*     pLumaConversionConfig)
{
    BOOL        forceYChannel           = pLumaConversionConfig->forceYChannel;
    BOOL        overrideYCoefficient    = pLumaConversionConfig->overrideYCoefficient;
    FLOAT       ratio                   = pLumaConversionConfig->ratio;

    CamxResult  result                  = CamxResultSuccess;

    // Configure Luma(Y) conversion registers
    if (BFChannelSelectY == pInputBFStatsConfig->BFStats.BFInputConfig.BFChannelSel)
    {
        m_regCmd.BFConfig.config.bits.CH_SEL = ConfigChannelSelectY;
        if (TRUE == overrideYCoefficient)
        {
            ratio = YAConfigOverrideRatioMax;
        }
        else
        {
            ratio = YAConfigNoOverrideRatioMax;
        }

        m_regCmd.BFConfig.yConvConfig0.bits.A0 = Utils::FLOATToSFixed32(
            (pInputBFStatsConfig->BFStats.BFInputConfig.YAConfig[0] * ratio), LumaConversionConfigShift, TRUE);
        m_regCmd.BFConfig.yConvConfig0.bits.A1 = Utils::FLOATToSFixed32(
            (pInputBFStatsConfig->BFStats.BFInputConfig.YAConfig[1] * ratio), LumaConversionConfigShift, TRUE);
        m_regCmd.BFConfig.yConvConfig1.bits.A2 = Utils::FLOATToSFixed32(
            (pInputBFStatsConfig->BFStats.BFInputConfig.YAConfig[2] * ratio), LumaConversionConfigShift, TRUE);
    }
    else if (BFChannelSelectG == pInputBFStatsConfig->BFStats.BFInputConfig.BFChannelSel)
    {
        if (TRUE == forceYChannel)
        {
            m_regCmd.BFConfig.yConvConfig0.bits.A0 = Utils::FLOATToSFixed32(0.0f, LumaConversionConfigShift, TRUE);
            m_regCmd.BFConfig.yConvConfig0.bits.A1 = Utils::FLOATToSFixed32(1.99f, LumaConversionConfigShift, TRUE);
            m_regCmd.BFConfig.yConvConfig1.bits.A2 = Utils::FLOATToSFixed32(0.0f, LumaConversionConfigShift, TRUE);

            m_regCmd.BFConfig.config.bits.CH_SEL = ConfigChannelSelectY;
        }
        else
        {
            m_regCmd.BFConfig.config.bits.CH_SEL = ConfigChannelSelectG;
        }
    }

    if (BFInputSelectGr == pInputBFStatsConfig->BFStats.BFInputConfig.BFInputGSel)
    {
        m_regCmd.BFConfig.config.bits.G_SEL = ConfigGSelectGR;
    }
    else if (BFInputSelectGb == pInputBFStatsConfig->BFStats.BFInputConfig.BFInputGSel)
    {
        m_regCmd.BFConfig.config.bits.G_SEL = ConfigGSelectGB;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                     "Luma conversion coefficients A0 %d A1 %d A2 %d ",
                     m_regCmd.BFConfig.yConvConfig0.bits.A0,
                     m_regCmd.BFConfig.yConvConfig0.bits.A1,
                     m_regCmd.BFConfig.yConvConfig1.bits.A2);
    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                     " Luma conversion Input coefficients ipA0 %f ipA1 %f ipA2 %f",
                     pInputBFStatsConfig->BFStats.BFInputConfig.YAConfig[0],
                     pInputBFStatsConfig->BFStats.BFInputConfig.YAConfig[1],
                     pInputBFStatsConfig->BFStats.BFInputConfig.YAConfig[2]);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::FillDownscalerConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats25Titan480::FillDownscalerConfig(
    const BFStats25DownscalerConfigInput* pConfig)
{
    CAMX_ASSERT(NULL != pConfig);

    m_regCmd.BFConfig.config.bits.SCALE_EN                           = pConfig->enableScaler;
    m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN       = pConfig->scalerImageSizeIn;
    m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_OUT      = pConfig->scalerImageSizeOut;
    m_regCmd.phaseStripeConfig.hScalePhaseConfig.bits.H_INTERP_RESO  = pConfig->scalerPhaseInterpolationResolution;
    m_regCmd.phaseStripeConfig.hScalePhaseConfig.bits.H_PHASE_MULT   = pConfig->scalerPhaseMultiplicationFactor;
    m_regCmd.phaseStripeConfig.hScaleStripeConfig1.bits.H_PHASE_INIT = pConfig->scalerPhaseInitialValue;
    m_regCmd.phaseStripeConfig.hScaleStripeConfig0.bits.H_MN_INIT    = pConfig->scalerMNInitialValue;
    m_regCmd.phaseStripeConfig.hScalePadConfig.bits.H_SKIP_CNT       = pConfig->scalerSkipCount;
    m_regCmd.phaseStripeConfig.hScalePadConfig.bits.INPUT_WIDTH      = pConfig->scalerInputWidth;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::ConfigureFilters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats25Titan480::ConfigureFilters(
    const AFConfigParams*   pBFStatsConfig)
{
    CamxResult  result  = CamxResultSuccess;

    const BFFilterConfigParams*         pFilterConfig   = NULL;
    const BFFIRFilterConfigType*        pFIRConfig      = NULL;
    const BFIIRFilterConfigType*        pIIRConfig      = NULL;
    const BFFilterCoringConfigParams*   pCoringConfig   = NULL;

    // 1. Horizontal 1 FIR, IIR configuration
    pFilterConfig = &pBFStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1];
    pFIRConfig    = &(pFilterConfig->BFFIRFilterConfig);
    pIIRConfig    = &(pFilterConfig->BFIIRFilterConfig);

    // 1a. FIR config
    if (TRUE == pFIRConfig->enable)
    {
        m_regCmd.BFConfig.config.bits.H_1_FIR_EN = 0x1;
        ConfigureFIRFilters(pFIRConfig, IFEBFFilterType::Horizontal1);
    }
    else
    {
        m_regCmd.BFConfig.config.bits.H_1_FIR_EN = 0x0;
    }

    // 1b. IIR Filter
    if (TRUE == pIIRConfig->enable)
    {
        m_regCmd.BFConfig.config.bits.H_1_IIR_EN = 0x1;
        ConfigureIIRFilters(pIIRConfig, IFEBFFilterType::Horizontal1);
    }
    else
    {
        m_regCmd.BFConfig.config.bits.H_1_IIR_EN = 0x0;
    }


    // 2. Horizontal 2 FIR, IIR configuration
    //    BFFilterTypeHorizontal2
    //    This no longer supported since 8998, hence no implementation here.

    // 3. Vertical FIR, IIR configuration
    pFilterConfig = &pBFStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical];
    pFIRConfig    = &(pFilterConfig->BFFIRFilterConfig);
    pIIRConfig    = &(pFilterConfig->BFIIRFilterConfig);

    // 3b. IIR Filter
    if (TRUE == pIIRConfig->enable)
    {
        m_regCmd.BFConfig.config.bits.V_IIR_EN = 0x1;
        ConfigureIIRFilters(pIIRConfig, IFEBFFilterType::Vertical);
    }
    else
    {
        m_regCmd.BFConfig.config.bits.V_IIR_EN = 0x0;
    }

    /// FIGURE: h1_pix_sum and v_pix_sum

    // 4. Shift bit config
    m_regCmd.shiftBitsConfig.bits.H_1 = Utils::TwosComplementValidBits(
        pBFStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].shiftBits, ShifterBits);

    m_regCmd.shiftBitsConfig.bits.V = Utils::TwosComplementValidBits(
        pBFStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].shiftBits, ShifterBits);

    // 5. Coring Configuration for , H1, H2, V and Gain
    // 5a. Horizontal 1 coring config
    pFilterConfig = &pBFStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1];
    pCoringConfig = &(pFilterConfig->BFFilterCoringConfig);
    ConfigureCoring(pCoringConfig, IFEBFFilterType::Horizontal1);

    // 5c. Vertical coring config
    pFilterConfig = &pBFStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical];
    pCoringConfig = &(pFilterConfig->BFFilterCoringConfig);
    ConfigureCoring(pCoringConfig, IFEBFFilterType::Vertical);


    // 5d. Coring Gain config
    m_regCmd.coringConfig.coringGainConfig0.bits.H_1_GAIN =
        pBFStatsConfig->BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.gain & CoringBits;
    m_regCmd.coringConfig.coringGainConfig1.bits.V_GAIN =
        pBFStatsConfig->BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.gain & CoringBits;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::SetActiveWindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats25Titan480::SetActiveWindow(
    const ISPInputData*   pInputData)
{
    CamxResult  result = CamxResultSuccess;

    const CropInfo* pCAMIFCrop  = &pInputData->pStripeConfig->CAMIFCrop;
    const UINT32    CAMIFWidth  = pCAMIFCrop->lastPixel - pCAMIFCrop->firstPixel + 1;
    const UINT32    CAMIFHeight = pCAMIFCrop->lastLine  - pCAMIFCrop->firstLine + 1;

    /// @todo (CAMX-1403): Change the SetActiveWindow to start from first ROI
    m_regCmd.activeWindow.bafActiveWindow1.bits.X_MIN = HorizontalIIRMargin;
    m_regCmd.activeWindow.bafActiveWindow1.bits.Y_MIN = VerticalIIRMargin;
    m_regCmd.activeWindow.bafActiveWindow2.bits.X_MAX = CAMIFWidth - HorizontalIIRMargin;
    m_regCmd.activeWindow.bafActiveWindow2.bits.Y_MAX = CAMIFHeight - VerticalIIRMargin;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "CAMIFWidth = %d, CAMIFHeight = %d", CAMIFWidth, CAMIFHeight);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::ConfigureFIRFilters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats25Titan480::ConfigureFIRFilters(
    const BFFIRFilterConfigType*    pFIRConfig,
    IFEBFFilterType                 filterType)
{
    CamxResult  result = CamxResultSuccess;

    if (IFEBFFilterType::Horizontal1 == filterType)
    {
        m_regCmd.hFIRIIRConfig.h1FIRConfig0.bits.A0 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[0], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig0.bits.A1 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[1], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig0.bits.A2 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[2], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig0.bits.A3 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[3], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig1.bits.A4 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[4], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig1.bits.A5 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[5], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig1.bits.A6 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[6], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig1.bits.A7 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[7], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig2.bits.A8 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[8], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig2.bits.A9 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[9], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig2.bits.A10 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[10], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig2.bits.A11 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[11], FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig3.bits.A12 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[12], FIRBits);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::ConfigureIIRFilters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats25Titan480::ConfigureIIRFilters(
    const BFIIRFilterConfigType*    pIIRConfig,
    IFEBFFilterType                 filterType)
{
    CamxResult  result = CamxResultSuccess;
    if (IFEBFFilterType::Vertical == filterType)
    {
        m_regCmd.vIIRConfig.vIIRConfig0.bits.B10 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b10, 14, TRUE), IIRBits);
        m_regCmd.vIIRConfig.vIIRConfig0.bits.B11 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b11, 14, TRUE), IIRBits);
        m_regCmd.vIIRConfig.vIIRConfig1.bits.B12 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b12, 14, TRUE), IIRBits);
        m_regCmd.vIIRConfig.vIIRConfig2.bits.A11 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a11, 14, TRUE), IIRBits);
        m_regCmd.vIIRConfig.vIIRConfig2.bits.A12 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a12, 14, TRUE), IIRBits);
        m_regCmd.vIIRConfig.vIIRConfig3.bits.B20 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b20, 14, TRUE), IIRBits);
        m_regCmd.vIIRConfig.vIIRConfig3.bits.B21 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b21, 14, TRUE), IIRBits);
        m_regCmd.vIIRConfig.vIIRConfig4.bits.B22 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b22, 14, TRUE), IIRBits);
        m_regCmd.vIIRConfig.vIIRConfig5.bits.A21 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a21, 14, TRUE), IIRBits);
        m_regCmd.vIIRConfig.vIIRConfig5.bits.A22 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a22, 14, TRUE), IIRBits);
    }
    else if (IFEBFFilterType::Horizontal1 == filterType)
    {
        m_regCmd.hFIRIIRConfig.h1IIRConfig0.bits.B10 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b10, 14, TRUE), IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig0.bits.B11 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b11, 14, TRUE), IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig1.bits.B12 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b12, 14, TRUE), IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig1.bits.B22 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b22, 14, TRUE), IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig2.bits.A11 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a11, 14, TRUE), IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig2.bits.A12 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a12, 14, TRUE), IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig3.bits.B20 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b20, 14, TRUE), IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig3.bits.B21 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b21, 14, TRUE), IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig4.bits.A21 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a21, 14, TRUE), IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig4.bits.A22 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a22, 14, TRUE), IIRBits);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::ConfigureCoring
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats25Titan480::ConfigureCoring(
    const BFFilterCoringConfigParams*   pCoringConfig,
    IFEBFFilterType                     filterType)
{
    CamxResult  result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(filterType);
    if (filterType == IFEBFFilterType::Horizontal1)
    {
        m_regCmd.hThresholdCoringConfig.h1ThresholdConfig.bits.THRESHOLD = pCoringConfig->threshold & ThresholdMask;
        m_regCmd.hThresholdCoringConfig.h1CoringConfig0.bits.IND_0       = pCoringConfig->core[0];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig0.bits.IND_1       = pCoringConfig->core[1];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig0.bits.IND_2       = pCoringConfig->core[2];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig0.bits.IND_3       = pCoringConfig->core[3];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig0.bits.IND_4       = pCoringConfig->core[4];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig1.bits.IND_5       = pCoringConfig->core[5];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig1.bits.IND_6       = pCoringConfig->core[6];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig1.bits.IND_7       = pCoringConfig->core[7];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig1.bits.IND_8       = pCoringConfig->core[8];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig1.bits.IND_9       = pCoringConfig->core[9];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig2.bits.IND_10      = pCoringConfig->core[10];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig2.bits.IND_11      = pCoringConfig->core[11];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig2.bits.IND_12      = pCoringConfig->core[12];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig2.bits.IND_13      = pCoringConfig->core[13];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig2.bits.IND_14      = pCoringConfig->core[14];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig3.bits.IND_15      = pCoringConfig->core[15];
        m_regCmd.hThresholdCoringConfig.h1CoringConfig3.bits.IND_16      = pCoringConfig->core[16];
    }
    else if (filterType == IFEBFFilterType::Vertical)
    {
        m_regCmd.vThresholdCoringConfig.vThresholdConfig.bits.THRESHOLD = pCoringConfig->threshold & ThresholdMask;
        m_regCmd.vThresholdCoringConfig.vCoringConfig0.bits.IND_0       = pCoringConfig->core[0];
        m_regCmd.vThresholdCoringConfig.vCoringConfig0.bits.IND_1       = pCoringConfig->core[1];
        m_regCmd.vThresholdCoringConfig.vCoringConfig0.bits.IND_2       = pCoringConfig->core[2];
        m_regCmd.vThresholdCoringConfig.vCoringConfig0.bits.IND_3       = pCoringConfig->core[3];
        m_regCmd.vThresholdCoringConfig.vCoringConfig0.bits.IND_4       = pCoringConfig->core[4];
        m_regCmd.vThresholdCoringConfig.vCoringConfig1.bits.IND_5       = pCoringConfig->core[5];
        m_regCmd.vThresholdCoringConfig.vCoringConfig1.bits.IND_6       = pCoringConfig->core[6];
        m_regCmd.vThresholdCoringConfig.vCoringConfig1.bits.IND_7       = pCoringConfig->core[7];
        m_regCmd.vThresholdCoringConfig.vCoringConfig1.bits.IND_8       = pCoringConfig->core[8];
        m_regCmd.vThresholdCoringConfig.vCoringConfig1.bits.IND_9       = pCoringConfig->core[9];
        m_regCmd.vThresholdCoringConfig.vCoringConfig2.bits.IND_10      = pCoringConfig->core[10];
        m_regCmd.vThresholdCoringConfig.vCoringConfig2.bits.IND_11      = pCoringConfig->core[11];
        m_regCmd.vThresholdCoringConfig.vCoringConfig2.bits.IND_12      = pCoringConfig->core[12];
        m_regCmd.vThresholdCoringConfig.vCoringConfig2.bits.IND_13      = pCoringConfig->core[13];
        m_regCmd.vThresholdCoringConfig.vCoringConfig2.bits.IND_14      = pCoringConfig->core[14];
        m_regCmd.vThresholdCoringConfig.vCoringConfig3.bits.IND_15      = pCoringConfig->core[15];
        m_regCmd.vThresholdCoringConfig.vCoringConfig3.bits.IND_16      = pCoringConfig->core[16];
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::PackIQRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats25Titan480::PackIQRegisterSetting(
    VOID*   pInput,
    VOID*   pOutput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutput);
    const BFStats25ConfigData* pConfigData = static_cast<BFStats25ConfigData*>(pInput);

    ResetPerFrameData(pConfigData->pISPInputData);

    // 1. Update Y conversion config
    if (TRUE == pConfigData->inputConfigUpdate)
    {
        LumaConversionConfig(pConfigData->pStatsConfig, pConfigData->pLumaConversionConfig);
    }

    // 2.Update downscaler cfg
    if (TRUE == pConfigData->downScalerSupported)
    {
        FillDownscalerConfig(pConfigData->pDownscalerConfig);
    }

    // 3 Update FIR, IIR and Coring config
    ConfigureFilters(pConfigData->pStatsConfig);

    // SetActiveWindow(pOutputData);
    SetActiveWindow(pConfigData->pISPInputData);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::SetupRegisterSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats25Titan480::SetupRegisterSetting(
    VOID*  pInput)
{
    CamxResult           result      = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::CopyRegCmd
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEBFStats25Titan480::CopyRegCmd(
    VOID* pData)
{
    UINT32 dataCopied = 0;

    if (NULL != pData)
    {
        // Need to find the way to copy regCmd2
        Utils::Memcpy(pData, &m_regCmd, sizeof(m_regCmd));
        dataCopied = sizeof(m_regCmd);
    }

    return dataCopied;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::~IFEBFStats25Titan480
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBFStats25Titan480::~IFEBFStats25Titan480()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats25Titan480::DumpRegConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats25Titan480::DumpRegConfig()
{
    /// @brief Local debug dump register structure
    struct DumpInfo
    {
        UINT32  startRegAddr;    ///< Start address of the register of range
        UINT32  numRegs;         ///< The number of registers to be programmed.
        UINT32* pRegRangeAddr;   ///< The pointer to the structure in memory or a single varaible.
    };

    DumpInfo dumpRegInfoArray[] =
    {
        {
            regIFE_IFE_0_PP_CLC_STATS_BAF_DMI_LUT_BANK_CFG,
            sizeof(IFEBF25Titan480Config) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.BFConfig)
        },

        {
            regIFE_IFE_0_PP_CLC_STATS_BAF_H_1_FIR_CFG_0,
            sizeof(IFEBF25Titan480HFIRIIRConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.hFIRIIRConfig)
        },

        {
            regIFE_IFE_0_PP_CLC_STATS_BAF_V_IIR_CFG_0,
            sizeof(IFEBF25Titan480VIIRConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.vIIRConfig)
        },

        {
            regIFE_IFE_0_PP_CLC_STATS_BAF_ROI_XY_MIN,
            sizeof(IFEBF25Titan480ActiveWindow) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.activeWindow)
        },

        {
            regIFE_IFE_0_PP_CLC_STATS_BAF_SHIFT_BITS_CFG,
            sizeof(IFE_IFE_0_PP_CLC_STATS_BAF_SHIFT_BITS_CFG) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.shiftBitsConfig)
        },

        {
            regIFE_IFE_0_PP_CLC_STATS_BAF_H_1_TH_CFG,
            sizeof(IFEBF25Titan480HThresholdCoringConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.hThresholdCoringConfig)
        },

        {
            regIFE_IFE_0_PP_CLC_STATS_BAF_V_TH_CFG,
            sizeof(IFEBF25Titan480VThresholdCoringConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.vThresholdCoringConfig)
        },

        {
            regIFE_IFE_0_PP_CLC_STATS_BAF_CORING_GAIN_CFG_0,
            sizeof(IFEBF25Titan480CoringConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.coringConfig)
        },

        {
            regIFE_IFE_0_PP_CLC_STATS_BAF_SCALE_H_IMAGE_SIZE_CFG,
            sizeof(IFEBF25Titan480PhaseStripeInterruptConfig) / RegisterWidthInBytes,
            reinterpret_cast<UINT32*>(&m_regCmd.phaseStripeConfig)
        }
    };

    for (UINT i = 0; i < CAMX_ARRAY_SIZE(dumpRegInfoArray); i++)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "SECTION[%d]: %08x: %08x",
                         i, dumpRegInfoArray[i].startRegAddr, *(dumpRegInfoArray[i].pRegRangeAddr));

        for (UINT j = 0; j < dumpRegInfoArray[i].numRegs; j++)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupIQMod, "%08x: %08x",
                             dumpRegInfoArray[i].startRegAddr + j*4, *(dumpRegInfoArray[i].pRegRangeAddr+j));
        }
    }
}

CAMX_NAMESPACE_END
