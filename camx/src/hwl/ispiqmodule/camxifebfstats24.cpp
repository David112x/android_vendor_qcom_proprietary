////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebfstats24.cpp
/// @brief IFEBFStats23 Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhal3module.h"
#include "camxifebfstats24.h"


CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats24::Create(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        pCreateData->pModule = CAMX_NEW IFEBFStats24;
        if (NULL == pCreateData->pModule)
        {
            result = CamxResultENoMemory;
            CAMX_ASSERT_ALWAYS_MESSAGE("Memory allocation failed");
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Null pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::ResetPerFrameData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::ResetPerFrameData(
    ISPInputData* pInputData)
{
    // memset command registers
    Utils::Memset(&m_regCmd, 0, sizeof(m_regCmd));

    // Restore frame state data
    m_regCmd.BFConfig.config.bits.ROI_IND_LUT_BANK_SEL  = pInputData->pStripeConfig->stateBF.ROIIndexLUTBank;
    m_regCmd.BFConfig.config.bits.GAMMA_LUT_BANK_SEL    = pInputData->pStripeConfig->stateBF.gammaLUTBank;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats24::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);
        if ((CamxResultSuccess    == result)                             &&
            (CamxResultSuccess    == CheckDependenceChange(pInputData))  &&
            ((m_ROIConfigUpdate   == TRUE) ||
             (m_inputConfigUpdate == TRUE) ||
             (TRUE == pInputData->forceTriggerUpdate)))
        {
            ResetPerFrameData(pInputData);
            RunCalculation(pInputData);
            result = CreateCmdList(pInputData);
        }

        if (CamxResultSuccess == result)
        {
            // Update CAMX metadata with the values for the current frame
            UpdateIFEInternalData(pInputData);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("%p Invalid Input pointer", pInputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats24::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result         = CamxResultSuccess;
    BOOL       bIsValidConfig = TRUE;

    if (NULL != pInputData)
    {
        if (NULL != pInputData->pStripingInput)
        {
            pInputData->pStripingInput->enableBits.BAF              = m_moduleEnable;
            pInputData->pStripingInput->stripingInput.BAFEnable    = static_cast<int16_t>(m_moduleEnable);

            pInputData->pStripingInput->stripingInput.BAFInputv24.BAFHorizScalerEn = static_cast<uint16_t>(
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.BFScaleEnable);
            pInputData->pStripingInput->stripingInput.BAFInputv24.BAF_fir_h1_en   = static_cast<uint16_t>(
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFFilterConfig->BFFIRFilterConfig.enable);
            pInputData->pStripingInput->stripingInput.BAFInputv24.BAF_iir_h1_en   = static_cast<uint16_t>(
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFFilterConfig->BFIIRFilterConfig.enable);

            m_CAMIFWidth = pInputData->pStripeConfig->CAMIFCrop.lastPixel - pInputData->pStripeConfig->CAMIFCrop.firstPixel + 1;
            m_CAMIFHeight = pInputData->pStripeConfig->CAMIFCrop.lastLine - pInputData->pStripeConfig->CAMIFCrop.firstLine + 1;

            AFConfigParams*         pAFConfig   = &pInputData->pStripeConfig->AFStatsUpdateData.statsConfig;
            BFStatsROIConfigType    hwROIConfig;

            ValidateAndAdjustROIBoundary(pInputData, pAFConfig);

            if (pAFConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension > 0)
            {
                for (UINT index = 0; index < pAFConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension; index++)
                {
                    if (TRUE == pAFConfig->BFStats.BFStatsROIConfig.BFStatsROIDimension[index].isValid)
                    {
                        bIsValidConfig = TRUE;
                        break;
                    }
                }
            }

            if ((FALSE == bIsValidConfig) && (pInputData->frameNum > FirstValidRequestId))
            {
                // Using The Previous Valid Config
                CAMX_LOG_WARN(CamxLogGroupISP, "Using the Previous AF Config");
                Utils::Memcpy(pAFConfig, &m_previousAFConfig, sizeof(AFConfigParams));
            }
            else
            {
                // Update the Working Config
                Utils::Memcpy(&m_previousAFConfig, pAFConfig, sizeof(AFConfigParams));
            }

            UpdateHWROI(&pAFConfig->BFStats.BFStatsROIConfig, &hwROIConfig);
            UpdateROIDMITable(&hwROIConfig,
                pInputData->pStripingInput->stripingInput.BAFInputv24.BAFROIIndexLUT,
                sizeof(pInputData->pStripingInput->stripingInput.BAFInputv24.BAFROIIndexLUT));
            DownscalerConfig(pInputData, pAFConfig);

            pInputData->pStripingInput->stripingInput.BAFInputv24.mndsParam.enable = 1;
            pInputData->pStripingInput->stripingInput.BAFInputv24.mndsParam.input =
                static_cast<UINT16>(m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN + 1);
            pInputData->pStripingInput->stripingInput.BAFInputv24.mndsParam.output =
                static_cast<UINT16>(m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_OUT + 1);
            pInputData->pStripingInput->stripingInput.BAFInputv24.mndsParam.pixelOffset =
                static_cast<UINT16>(m_regCmd.phaseStripeConfig.hScalePadConfig.bits.H_SKIP_CNT);
            pInputData->pStripingInput->stripingInput.BAFInputv24.mndsParam.cntInit =
                static_cast<UINT16>(m_regCmd.phaseStripeConfig.hScaleStripeConfig0.bits.H_MN_INIT);
            pInputData->pStripingInput->stripingInput.BAFInputv24.mndsParam.interpReso =
                static_cast<UINT16>(m_regCmd.phaseStripeConfig.hScalePhaseConfig.bits.H_INTERP_RESO);
            pInputData->pStripingInput->stripingInput.BAFInputv24.mndsParam.roundingOptionHor =
                static_cast<UINT16>(m_regCmd.phaseStripeConfig.hScalePadConfig.bits.ROUNDING_PATTERN);
            pInputData->pStripingInput->stripingInput.BAFInputv24.mndsParam.inputProcessedLength =
                static_cast<UINT16>(m_regCmd.phaseStripeConfig.hScalePadConfig.bits.SCALE_Y_IN_WIDTH + 1);
            pInputData->pStripingInput->stripingInput.BAFInputv24.mndsParam.phaseInit =
                m_regCmd.phaseStripeConfig.hScaleStripeConfig1.bits.H_PHASE_INIT;
            pInputData->pStripingInput->stripingInput.BAFInputv24.mndsParam.phaseStep =
                m_regCmd.phaseStripeConfig.hScalePhaseConfig.bits.H_PHASE_MULT;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("%p Invalid Input pointer", pInputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::GetDMITable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::GetDMITable(
    UINT32** ppDMITable)
{
    if (NULL == ppDMITable)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid pointer to write DMI into");
        return;
    }

    Utils::Memcpy(*ppDMITable, &m_ROIDMIConfig, (m_hwROIConfig.numBFStatsROIDimension * sizeof(UINT64)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // Update crop Info metadata
    pInputData->pCalculatedData->metadata.BFStats.BFConfig = m_BFStatsConfig;

    pInputData->pCalculatedData->metadata.BFStats.isAdjusted = m_isInputConfigAdjusted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::IFEBFStats24
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBFStats24::IFEBFStats24()
    : m_gammaSupported(IFEBF24GammaSupported)
    , m_downScalerSupported(IFEBF24DownScalerSupported)
    , m_pHorizontalRegionCount(NULL)
    , m_inputConfigUpdate(FALSE)
    , m_ROIConfigUpdate(FALSE)
{
    m_cmdLength = PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBF24Config) / RegisterWidthInBytes)        +
                  PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBF24HFIRIIRConfig) / RegisterWidthInBytes) +
                  PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBF24VIIRConfig1) / RegisterWidthInBytes)   +
                  PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBF24VIIRConfig2) / RegisterWidthInBytes)   +
                  PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBF24ActiveWindow) / RegisterWidthInBytes)  +
                  PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFE_IFE_0_VFE_STATS_BAF_SHIFT_BITS_CFG) /
                                                                   RegisterWidthInBytes)                                +
                  PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBF24HThresholdCoringConfig) /
                                                                   RegisterWidthInBytes)                                +
                  PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBF24VThresholdCoringConfig) /
                                                                   RegisterWidthInBytes)                                +
                  PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBF24CoringConfig) / RegisterWidthInBytes)  +
                  PacketBuilder::RequiredWriteRegRangeSizeInDwords(sizeof(IFEBF24PhaseStripeInterruptConfig) /
                                                                   RegisterWidthInBytes);

    m_cmdLength                                       +=
        PacketBuilder::RequiredWriteDMISizeInDwords() * 2;
    m_DMITableLength                                  =
        sizeof(m_ROIDMIConfig) + sizeof(m_gammaLUT);

    m_32bitDMILength = IFEBF24GammaLUTDMILengthDWord;
    m_64bitDMILength = IFEBF24ROIDMILengthDWord;

    // First value of LUT should be Bank0, so set it 1 and then we toggle to select Bank0
    m_regCmd.BFConfig.config.bits.GAMMA_LUT_BANK_SEL   = 1;
    m_regCmd.BFConfig.config.bits.ROI_IND_LUT_BANK_SEL = 1;
    m_moduleEnable                                     = TRUE;
    m_type                                             = ISPStatsModuleType::IFEBF;
    m_isInputConfigAdjusted                            = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::~IFEBFStats24
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEBFStats24::~IFEBFStats24()
{
    // Free up allocated memory
    if (NULL != m_pHorizontalRegionCount)
    {
        CAMX_FREE(m_pHorizontalRegionCount);
        m_pHorizontalRegionCount = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats24::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    INT32                index                     = 0;
    FLOAT                inputConfigCoefficientSum = 0;
    CamxResult           result                    = CamxResultSuccess;
    AFConfigParams*      pAFConfig                 = NULL;
    BFInputConfigParams* pBFInputConfigParams      = NULL;
    BFScaleConfigType*   pBFScaleConfigType        = NULL;

    if (NULL != pInputData->pStripeConfig)
    {
        pAFConfig            = &pInputData->pStripeConfig->AFStatsUpdateData.statsConfig;
        pBFInputConfigParams = &pAFConfig->BFStats.BFInputConfig;
        pBFScaleConfigType   = &pAFConfig->BFStats.BFScaleConfig;

        // Validate Input config
        if ((TRUE               == pBFInputConfigParams->isValid) &&
            (BFChannelSelectMax >  pBFInputConfigParams->BFChannelSel))
        {
            if (BFChannelSelectY == pBFInputConfigParams->BFChannelSel)
            {
                for (index = 0; index < MaxYConfig; index++)
                {
                    if ((IFEBF24YAConfigMin <= pBFInputConfigParams->YAConfig[index]) &&
                        (IFEBF24YAConfigMax >= pBFInputConfigParams->YAConfig[index]))
                    {
                        inputConfigCoefficientSum += pBFInputConfigParams->YAConfig[index];
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP,
                                       "Invalid input config %f",
                                       pBFInputConfigParams->YAConfig[index]);
                        result = CamxResultEInvalidArg;
                        break;
                    }
                }
                if ((IFEBF24YAConfigMax       <  inputConfigCoefficientSum) &&
                    (CamxResultSuccess == result))
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid inputConfigCoefficientSum %f", inputConfigCoefficientSum);
                    result = CamxResultEInvalidArg;
                }
            }
        }

        // Validate  M/N downscaler config
        if ((CamxResultSuccess == result)                            &&
            (TRUE              == pBFScaleConfigType->isValid)       &&
            (TRUE              == pBFScaleConfigType->BFScaleEnable) &&
            ((pBFScaleConfigType->scaleN <  pBFScaleConfigType->scaleM) ||
             (0                          == pBFScaleConfigType->scaleN) ||
             (0                          == pBFScaleConfigType->scaleM) ||
             (IFEBF24YAConfigCoEffSum    <  (pBFScaleConfigType->scaleM / pBFScaleConfigType->scaleN)) ||
             (IFEBF24ScaleRatio2         >  static_cast<UINT32>((pBFScaleConfigType->scaleN / pBFScaleConfigType->scaleM)))))
        {
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "Invalid Scale config %d %d",
                           pBFScaleConfigType->scaleN,
                           pBFScaleConfigType->scaleM);

            result = CamxResultEInvalidArg;
        }

        // Validate ROI config
        if ((CamxResultSuccess == result)                                        &&
            (BFStatsDefaultROI != pAFConfig->BFStats.BFStatsROIConfig.BFROIType) &&
            (BFMaxROIRegions   <  pAFConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension))
        {
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "Invalid number of ROI %d",
                           pAFConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension);
            result = CamxResultEInvalidArg;
        }

        // Gamma entries validation
        if ((CamxResultSuccess == result) &&
            (TRUE              == m_gammaSupported))
        {
            if (TRUE == pAFConfig->BFStats.BFGammaLUTConfig.isValid)
            {
                if (MaxBFGammaEntries != pAFConfig->BFStats.BFGammaLUTConfig.numGammaLUT)
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP,
                                   "Invalid Gamma Entries %d",
                                   pAFConfig->BFStats.BFGammaLUTConfig.numGammaLUT);
                    result = CamxResultEInvalidArg;
                }
            }

        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP,
                       "Af config pointer is Null");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::HandleEmptyROI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::HandleEmptyROI(
    ISPInputData*           pInputData,
    BFStatsROIConfigType*   pROIOut)
{
    BFStatsROIConfigType* pStripeROI        =
        &pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFStatsROIConfig;
    BFStatsROIConfigType* pFrameLevelROI    = &pInputData->pAFStatsUpdateData->statsConfig.BFStats.BFStatsROIConfig;

    // If frame level ROI is empty, then we will use last configuration - so no action here
    // If frame level ROI is valid but stripe ROI is empty, there are two options
    // i. Disable BF for the stripe
    // ii. Configure dummy ROI
    if ((0 < pFrameLevelROI->numBFStatsROIDimension) && (0 == pStripeROI->numBFStatsROIDimension))
    {
        Utils::Memset(pROIOut, 0, sizeof(BFStatsROIConfigType));

        BFStatsROIDimensionParams* pROI = &pROIOut->BFStatsROIDimension[0];

        // Use dummy configuration
        pROIOut->numBFStatsROIDimension = 1;
        pROI->isValid                   = TRUE;
        pROI->region                    = BFStatsPrimaryRegion;
        pROI->regionNum                 = 0;
        pROI->ROI.left                  = 200;
        pROI->ROI.top                   = 100;
        pROI->ROI.width                 = 20;
        pROI->ROI.height                = 20;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats24::CheckDependenceChange(
    ISPInputData* pInputData)
{
    UINT32          inputWidth       = 0;
    UINT32          inputHeight      = 0;
    CropInfo*       pSensorCAMIFCrop = NULL;
    AFConfigParams* pAFConfig        = NULL;
    CamxResult      result           = CamxResultSuccess;
    UINT32          numberofValidROI = 0;

    if (NULL != pInputData->pStripeConfig)
    {
        pSensorCAMIFCrop = &pInputData->pStripeConfig->CAMIFCrop;
        pAFConfig        = &pInputData->pStripeConfig->AFStatsUpdateData.statsConfig;
        inputWidth       = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
        inputHeight      = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;

        // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
        if (TRUE == pInputData->sensorData.isYUV)
        {
            inputWidth >>= 1;
        }

        if ((m_CAMIFWidth  != inputWidth) ||
            (m_CAMIFHeight != inputHeight))
        {
            m_CAMIFWidth  = inputWidth;
            m_CAMIFHeight = inputHeight;

            /// @todo (CAMX-1414): Check if we can move memory allocation to constructor - BF Stats
            if (NULL != m_pHorizontalRegionCount)
            {
                CAMX_FREE(m_pHorizontalRegionCount);
                m_pHorizontalRegionCount = NULL;
            }

            m_pHorizontalRegionCount = static_cast<UINT32*>(CAMX_CALLOC(m_CAMIFHeight * sizeof(UINT32)));
            if (NULL == m_pHorizontalRegionCount)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Failed to allocate memory");
                result = CamxResultENoMemory;
            }
        }

        if (CamxResultSuccess == result)
        {
            // Check if there is any update on the channel select
            if (TRUE == pAFConfig->BFStats.BFInputConfig.isValid)
            {
                Utils::Memcpy(&m_BFStatsConfig.BFStats.BFInputConfig,
                              &pAFConfig->BFStats.BFInputConfig, sizeof(BFInputConfigParams));
                m_inputConfigUpdate = TRUE;
            }

            // Check if there is any update on M/N downscaler config
            if ((TRUE == pAFConfig->BFStats.BFScaleConfig.isValid))
            {
                Utils::Memcpy(&m_BFStatsConfig.BFStats.BFScaleConfig,
                              &pAFConfig->BFStats.BFScaleConfig,
                              sizeof(BFScaleConfigType));
            }

            if (pAFConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension <= 0)
            {
                HandleEmptyROI(pInputData, &m_BFStatsConfig.BFStats.BFStatsROIConfig);
            }
            else
            {
                Utils::Memcpy(&m_BFStatsConfig.BFStats.BFStatsROIConfig,
                              &pAFConfig->BFStats.BFStatsROIConfig,
                              sizeof(BFStatsROIConfigType));
            }

            // For DUAL IFE Striping librray will adjust the ROi as per scale ratio
            // For Single IFE we will adjust here
            if ((TRUE == pAFConfig->BFStats.BFScaleConfig.isValid)       &&
                (TRUE == pAFConfig->BFStats.BFScaleConfig.BFScaleEnable) &&
                (FALSE == pInputData->pStripeConfig->overwriteStripes))
            {
                for (UINT index = 0; index < m_BFStatsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension; index++)
                {
                    AdjustScaledROIBoundary(pInputData,
                                            &m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[index].ROI);
                }
            }

            if (pAFConfig->BFStats.BFStatsROIConfig.BFROIType != BFStatsInvalidROI)
            {
                // Adjust the ROI to match the characteristics of BF stats module
                m_isInputConfigAdjusted = ValidateAndAdjustROIBoundary(pInputData, &m_BFStatsConfig);

                for (UINT index = 0; index < m_BFStatsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension; index++)
                {
                    if (TRUE == m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[index].isValid)
                    {
                        numberofValidROI++;
                    }
                }

                if (0 == numberofValidROI)
                {
                    // This is a case Where all regions are invalid.. Add Dummy region
                    pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.
                        BFStats.BFStatsROIConfig.numBFStatsROIDimension = 0;
                    HandleEmptyROI(pInputData, &m_BFStatsConfig.BFStats.BFStatsROIConfig);
                }

                // Bubble Sort regions based on starting pixel
                SortRegionsStartPixelOrderBased();

                // Check various HW limitations with respect to ROI
                CheckAndAdjustStartPixelROILimitation();

                result = CheckMaxHorizontalLimitationPerRow();

                if (CamxResultSuccess == result)
                {
                    result = CheckMaxVerticalSpacingAndDisjoint();
                }

                if (CamxResultSuccess == result)
                {
                    result = CheckMaxRegionOverlap();
                }

                if (CamxResultSuccess == result)
                {
                    m_ROIConfigUpdate = TRUE;
                }
            }

            // Update Gamma config
            if (TRUE == m_gammaSupported)
            {
                if (TRUE == pAFConfig->BFStats.BFGammaLUTConfig.isValid)
                {
                    Utils::Memcpy(&m_BFStatsConfig.BFStats.BFGammaLUTConfig,
                        &pAFConfig->BFStats.BFGammaLUTConfig,
                        sizeof(BFGammaLUTConfigType));
                }
            }

            // Update filter config
            for (UINT index = 0; index < BFFilterTypeCount; index++)
            {
                if (TRUE == pAFConfig->BFStats.BFFilterConfig[index].isValid)
                {
                    Utils::Memcpy(&m_BFStatsConfig.BFStats.BFFilterConfig[index],
                        &pAFConfig->BFStats.BFFilterConfig[index],
                        sizeof(BFFilterConfigParams));
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP,
                       "Af config pointer is Null");
        result = CamxResultEInvalidArg;
    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::RunCalculation(
    ISPInputData* pInputData)
{
    // 1. Update Y conversion config
    if (TRUE == m_inputConfigUpdate)
    {
        m_inputConfigUpdate = FALSE;
        LumaConversionConfig();
    }

    // 2.Update downscaler cfg
    if (TRUE == m_downScalerSupported)
    {
        DownscalerConfig(pInputData, &m_BFStatsConfig);
    }

    // 3 Update FIR, IIR and Coring config
    ConfigureFilters();

    SetActiveWindow();

    // 4a. Update the HW ROI config for single and Dual VFE and Store ROI for DMI write
    if (TRUE == m_ROIConfigUpdate)
    {
        SortRegionsEndPixelOrderBased();

        CheckAndAdjustEndPixelROILimitation();

        UpdateHWROI(&m_BFStatsConfig.BFStats.BFStatsROIConfig, &m_hwROIConfig);

        UpdateROIDMITable(&m_hwROIConfig, m_ROIDMIConfig,  sizeof(m_ROIDMIConfig));
    }

    // 5. Configure Gamma
    if (TRUE == m_gammaSupported)
    {
        ConfigureGamma(pInputData);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::CreateCmdList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats24::CreateCmdList(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pInputData->pCmdBuffer)        &&
        (NULL != pInputData->p64bitDMIBuffer)   &&
        (NULL != pInputData->p32bitDMIBuffer))
    {
        CmdBuffer* pCmdBuffer   = pInputData->pCmdBuffer;

        // Select DMI LUT Bank
        m_regCmd.BFConfig.config.bits.ROI_IND_LUT_BANK_SEL  = pInputData->pStripeConfig->stateBF.gammaLUTBank;
        m_regCmd.BFConfig.config.bits.GAMMA_LUT_BANK_SEL    = pInputData->pStripeConfig->stateBF.ROIIndexLUTBank;
        // Save it so that next time we will select different bank
        pInputData->pStripeConfig->stateBF.gammaLUTBank     ^= 1;
        pInputData->pStripeConfig->stateBF.ROIIndexLUTBank  ^= 1;

        result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                              regIFE_IFE_0_VFE_STATS_BAF_CFG,
                                              sizeof(IFEBF24Config) / RegisterWidthInBytes,
                                              reinterpret_cast<UINT32*>(&m_regCmd.BFConfig));

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_BAF_H_1_FIR_CFG_0,
                                                  sizeof(IFEBF24HFIRIIRConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.hFIRIIRConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_BAF_V_IIR_CFG_0,
                                                  sizeof(IFEBF24VIIRConfig1) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.vIIRConfig1));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_BAF_V_IIR_CFG_3,
                                                  sizeof(IFEBF24VIIRConfig2) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.vIIRConfig2));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_BAF_ACTIVE_WINDOW_CFG_0,
                                                  sizeof(IFEBF24ActiveWindow) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.activeWindow));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_BAF_SHIFT_BITS_CFG,
                                                  sizeof(IFE_IFE_0_VFE_STATS_BAF_SHIFT_BITS_CFG) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.shiftBitsConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_BAF_H_1_TH_CFG,
                                                  sizeof(IFEBF24HThresholdCoringConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.hThresholdCoringConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_BAF_V_TH_CFG,
                                                  sizeof(IFEBF24VThresholdCoringConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.vThresholdCoringConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_BAF_CORING_GAIN_CFG_0,
                                                  sizeof(IFEBF24CoringConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.coringConfig));
        }

        if (CamxResultSuccess == result)
        {
            result = PacketBuilder::WriteRegRange(pCmdBuffer,
                                                  regIFE_IFE_0_VFE_STATS_BAF_SCALE_H_IMAGE_SIZE_CFG,
                                                  sizeof(IFEBF24PhaseStripeInterruptConfig) / RegisterWidthInBytes,
                                                  reinterpret_cast<UINT32*>(&m_regCmd.phaseStripeConfig));
        }

        // Write 64-bit DMI BF ROI configuration
        if (CamxResultSuccess == result)
        {
            CmdBuffer*  pDMI64Buffer     = pInputData->p64bitDMIBuffer;
            UINT32*     pDMI64BufferAddr = pInputData->p64bitDMIBufferAddr;
            UINT32      offsetDWord      =
                m_64bitDMIBufferOffsetDword + (pInputData->pStripeConfig->stripeId * m_64bitDMILength);
            UINT32      lengthInBytes    = m_hwROIConfig.numBFStatsROIDimension * sizeof(UINT64);

            UINT8       DMISelect        = (IFEBF24RegionIndexLUTBank0SelectValue ==
                                            m_regCmd.BFConfig.config.bits.ROI_IND_LUT_BANK_SEL) ?
                                           IFEBF24RegionIndexLUTBank0 :
                                           IFEBF24RegionIndexLUTBank1;
            const HwEnvironment* pHwEnvironment = HwEnvironment::GetInstance();
            if ((TRUE == pHwEnvironment->IsHWBugWorkaroundEnabled(
                        Titan17xWorkarounds::Titan17xWorkaroundsCDMDMI64EndiannessBug))   &&
                (FALSE == pHwEnvironment->GetStaticSettings()->ifeSWCDMEnable))
            {
                for (UINT32 index = 0; index < m_hwROIConfig.numBFStatsROIDimension; index++)
                {
                    m_ROIDMIConfig[index] = ((m_ROIDMIConfig[index] & CamX::Utils::AllOnes64(32)) << 32) |
                        (((m_ROIDMIConfig[index] >> 32) & CamX::Utils::AllOnes64(32)));
                }
            }

            // Need to account for the frame-tag header hence the size should be ROI dimension (which has end termination) + 1.
            Utils::Memcpy(pDMI64BufferAddr + offsetDWord,
                          &m_ROIDMIConfig,
                          ((m_hwROIConfig.numBFStatsROIDimension) * sizeof(UINT64)));

            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_VFE_DMI_CFG,
                                             DMISelect,
                                             pDMI64Buffer,
                                             offsetDWord * sizeof(UINT32),
                                             lengthInBytes);

            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid BF ROI DMI buffer");
            }
        }

        // Write 32-bit DMI BF gamma LUT configuration
        if (CamxResultSuccess == result)
        {
            CmdBuffer*  pDMI32Buffer     = pInputData->p32bitDMIBuffer;
            UINT32*     pDMI32BufferAddr = pInputData->p32bitDMIBufferAddr;
            UINT32      offsetDWord      =
                m_32bitDMIBufferOffsetDword + (pInputData->pStripeConfig->stripeId * m_32bitDMILength);
            UINT32      lengthInBytes    = IFEBF24GammaLUTDMILengthDWord * sizeof(UINT32);

            UINT8       DMISelect        = (IFEBF24GammaLUTBank0SelectValue ==
                                            m_regCmd.BFConfig.config.bits.GAMMA_LUT_BANK_SEL) ?
                                           IFEBF24GammeLUTBank0 :
                                           IFEBF24GammeLUTBank1;

            Utils::Memcpy(pDMI32BufferAddr + offsetDWord,
                          &m_gammaLUT,
                          sizeof(m_gammaLUT));

            result = PacketBuilder::WriteDMI(pCmdBuffer,
                                             regIFE_IFE_0_VFE_DMI_CFG,
                                             DMISelect,
                                             pDMI32Buffer,
                                             offsetDWord * sizeof(UINT32),
                                             lengthInBytes);

            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid BF gamme LUT DMI buffer");
            }
        }

        if (CamxResultSuccess != result)
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to write command buffer");
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
// IFEBFStats24::AdjustScaledROIBoundary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::AdjustScaledROIBoundary(
    ISPInputData*   pInputData,
    RectangleCoordinate* pRoi)
{
    UINT32 scaleM = pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.scaleM;
    UINT32 scaleN = pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.scaleN;
    UINT32 left   = 0;
    UINT32 width  = 0;

    if ((scaleM != 0) && (scaleN != 0))
    {
        left       = pRoi->left * scaleM;
        pRoi->left = (left + (scaleN -1)) / scaleN;

        width       = (pRoi->width + 1) * scaleM;
        pRoi->width = (width / scaleN) - 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::ValidateAndAdjustROIBoundary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEBFStats24::ValidateAndAdjustROIBoundary(
    ISPInputData*   pInputData,
    AFConfigParams* pBFStatsConfig)
{
    FLOAT                      scaleRatio               = 1.0f;
    UINT32                     index                    = 0;
    BOOL                       isInputConfigAdjusted    = FALSE;
    BFStatsROIDimensionParams* pROI                     = NULL;
    UINT32                     camifHeight              = 0;
    UINT32                     camifWidth               = 0;
    CropInfo*                  pSensorCAMIFCrop         = NULL;


    pSensorCAMIFCrop    = &pInputData->pStripeConfig->CAMIFCrop;
    camifWidth          = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
    camifHeight         = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;

    // Calculate scale ratio, to validate the ROI with respect to Filter input
    if ((TRUE == pBFStatsConfig->BFStats.BFScaleConfig.isValid)       &&
        (TRUE == pBFStatsConfig->BFStats.BFScaleConfig.BFScaleEnable) &&
        (0    != pBFStatsConfig->BFStats.BFScaleConfig.scaleN))
    {
        scaleRatio =
            static_cast<FLOAT>(pBFStatsConfig->BFStats.BFScaleConfig.scaleM) /
            static_cast<FLOAT>(pBFStatsConfig->BFStats.BFScaleConfig.scaleN);
    }

    // Validate the ROI, if does not meet the characterstics of the BF stats modify it
    for (index = 0; index < pBFStatsConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension; index++)
    {
        pROI = &pBFStatsConfig->BFStats.BFStatsROIConfig.BFStatsROIDimension[index];
        // Set the region to valid by default
        pROI->isValid = TRUE;

        if ((0 == pROI->ROI.width)  ||
            (0 == pROI->ROI.height) ||
            (pROI->ROI.width > camifWidth) ||
            (pROI->ROI.height > camifHeight) ||
            ((pROI->ROI.left + pROI->ROI.width) > camifWidth) ||
            ((pROI->ROI.top + pROI->ROI.height) > camifHeight))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, " Invalid ROI at index %d, dimension %d * %d",
                          index, pROI->ROI.width, pROI->ROI.height);
            pROI->isValid = FALSE;
            continue;
        }

        // Region type should be either primary or secondary
        if (BFStatsMaxRegion <= pROI->region)
        {
            pROI->region            = BFStatsSecondaryRegion;
            isInputConfigAdjusted   = TRUE;
        }

        // Check minimum offset requirement and make the left and top even
        if ( IFEBF24MinXOffset > (scaleRatio * pROI->ROI.left))
        {
            UINT32   orgLeft        = pROI->ROI.left;
            pROI->ROI.left          = static_cast<UINT32>(((IFEBF24MinXOffset + 1) / scaleRatio) - 1);
            pROI->ROI.left          = Utils::EvenCeilingUINT32(pROI->ROI.left);
            pROI->ROI.width         = pROI->ROI.width - (pROI->ROI.left - orgLeft);
            if (IFEBF24MinWidth > (scaleRatio * (pROI->ROI.width + 1)))
            {
                // not valid ROI
                pROI->isValid = FALSE;
            }
            isInputConfigAdjusted   = TRUE;
        }
        else if (Utils::IsUINT32Odd(pROI->ROI.left))
        {
            pROI->ROI.left          = Utils::EvenFloorUINT32(pROI->ROI.left);
            isInputConfigAdjusted   = TRUE;
        }

        if (IFEBF24MinYOffset > (scaleRatio * pROI->ROI.top))
        {
            UINT32   orgTop        = pROI->ROI.top;
            pROI->ROI.top           = static_cast<UINT32>(((IFEBF24MinYOffset + 1) / scaleRatio) - 1);
            pROI->ROI.top           = Utils::EvenCeilingUINT32(pROI->ROI.top);
            pROI->ROI.height        = pROI->ROI.height - (pROI->ROI.top - orgTop);
            if (pROI->ROI.height < IFEBF24MinHeight)
            {
                // not valid ROI
                pROI->isValid = FALSE;
            }
            isInputConfigAdjusted   = TRUE;
        }
        else if (Utils::IsUINT32Odd(pROI->ROI.top))
        {
            pROI->ROI.top           = Utils::EvenFloorUINT32(pROI->ROI.top);
            isInputConfigAdjusted   = TRUE;
        }

        // Width and height should be even, Programed value should be odd
        if (Utils::IsUINT32Even(pROI->ROI.width))
        {
            pROI->ROI.width         = Utils::OddFloorUINT32(pROI->ROI.width);
            isInputConfigAdjusted   = TRUE;
        }

        if (Utils::IsUINT32Even(pROI->ROI.height))
        {
            pROI->ROI.height        = Utils::OddFloorUINT32(pROI->ROI.height);
            isInputConfigAdjusted   = TRUE;
        }

        if (IFEBF24MinWidth > (scaleRatio * (pROI->ROI.width + 1)))
        {
            pROI->ROI.width         = static_cast<UINT32> (((IFEBF24MinWidth + 1) / scaleRatio) - 1);
            pROI->ROI.width         = Utils::OddCeilingUINT32(pROI->ROI.width);
            isInputConfigAdjusted   = TRUE;
        }

        if (pROI->ROI.width > (camifWidth >> 1))
        {
            pROI->ROI.width         = camifWidth >> 1;
            pROI->ROI.width         = Utils::OddFloorUINT32(pROI->ROI.width);
            isInputConfigAdjusted   = TRUE;
        }

        // Validate minimum margin on right side
        if (static_cast<UINT32>(((pROI->ROI.left + pROI->ROI.width) * scaleRatio)) >
            ((camifWidth * scaleRatio) - IFEBF24HorizontalFIRMargin))
        {
            pROI->isValid = FALSE;
            continue;
        }

        // Validate min height
        if (pROI->ROI.height < IFEBF24MinHeight)
        {
            pROI->ROI.height        = IFEBF24MinHeight - 1;
            isInputConfigAdjusted   = TRUE;
        }

        // Validate minimum margin at bottom
        if ((pROI->ROI.top + pROI->ROI.height) > (camifHeight - IFEBF24VerticalFIRMargin))
        {
            pROI->isValid = FALSE;
            continue;
        }

        // Validate max h
        if (pROI->ROI.height > (camifHeight >> 1))
        {
            pROI->ROI.height        = camifHeight >> 1;
            pROI->ROI.height        = Utils::OddFloorUINT32(pROI->ROI.height);
            isInputConfigAdjusted   = TRUE;
        }
    }

    return isInputConfigAdjusted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::SortRegionsStartPixelOrderBased
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::SortRegionsStartPixelOrderBased()
{
    BFStatsROIDimensionParams* pROI         = NULL;
    BFStatsROIDimensionParams* pNextROI     = NULL;
    BFStatsROIConfigType*      pROIConfig   = &m_BFStatsConfig.BFStats.BFStatsROIConfig;
    BOOL                       swapped      = TRUE;
    UINT32                     index        = 0;
    UINT32                     nextIndex    = 0;
    UINT32                     currentStart = 0;
    UINT32                     nextStart    = 0;
    BFStatsROIDimensionParams  swapROI;

    while (TRUE == swapped)
    {
        swapped = FALSE;
        for (index = 0; index < pROIConfig->numBFStatsROIDimension; index++)
        {
            pROI = &pROIConfig->BFStatsROIDimension[index];
            if (TRUE == pROI->isValid)
            {
                // Calcuate starting pixel in single dimension based on raster scan order
                currentStart = (pROI->ROI.top * m_CAMIFWidth) + pROI->ROI.left;

                // Secondary loop is required as the next index ROI could be invalid
                for (nextIndex = index + 1; nextIndex < pROIConfig->numBFStatsROIDimension; nextIndex++)
                {
                    pNextROI = &pROIConfig->BFStatsROIDimension[nextIndex];
                    if (TRUE == pNextROI->isValid)
                    {
                        nextStart = (pNextROI->ROI.top * m_CAMIFWidth) + pNextROI->ROI.left;

                        // Bubble sort
                        if (currentStart > nextStart)
                        {
                            swapROI   = *pROI;
                            *pROI     = *pNextROI;
                            *pNextROI = swapROI;
                            swapped   = TRUE;
                        }
                        break;
                    }
                }
            }
        }
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::CheckAndAdjustStartPixelROILimitation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::CheckAndAdjustStartPixelROILimitation()
{
    BFStatsROIDimensionParams* pROI       = NULL;
    BFStatsROIDimensionParams* pNextROI   = NULL;
    BFStatsROIConfigType*      pROIConfig = &m_BFStatsConfig.BFStats.BFStatsROIConfig;
    UINT32                     index      = 0;
    UINT32                     nextIndex  = 0;

    for (index = 0; index < pROIConfig->numBFStatsROIDimension; index++)
    {
        pROI = &m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[index];
        if (FALSE == pROI->isValid)
        {
            continue;
        }

        for (nextIndex = index + 1; nextIndex < pROIConfig->numBFStatsROIDimension; nextIndex++)
        {
            pNextROI = &m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[nextIndex];
            if (FALSE == pNextROI->isValid)
            {
                continue;
            }

            if (((pROI->ROI.left == pNextROI->ROI.left)                    &&
                 (pROI->ROI.top  == pNextROI->ROI.top))                    ||
                ((TRUE           == RegionsOverlapping(*pROI, *pNextROI))  &&
                (Utils::AbsoluteINT32(pROI->ROI.left - pNextROI->ROI.left) <
                                      IFEBF24MinStartPixelOverlap)))
            {
                if ((pNextROI->ROI.width - IFEBF24MinStartPixelOverlap) >= (IFEBF24MinWidth))
                {
                    // Move ROI to right with reduced width
                    pNextROI->ROI.left      += IFEBF24MinStartPixelOverlap;
                    m_isInputConfigAdjusted = TRUE;
                }
                else if ((pNextROI->ROI.left + pNextROI->ROI.width + IFEBF24MinStartPixelOverlap) <=
                         (m_CAMIFWidth - IFEBF24HorizontalFIRMargin))
                {
                    // Move ROI to right
                    pNextROI->ROI.left      += IFEBF24MinStartPixelOverlap;
                    m_isInputConfigAdjusted = TRUE;
                }
                else
                {
                    pNextROI->isValid = FALSE;
                    CAMX_LOG_ERROR(CamxLogGroupISP,
                                   "Invalid ROI %d, %d, %d, %d",
                                   pNextROI->ROI.left, pNextROI->ROI.top,
                                   pNextROI->ROI.width, pNextROI->ROI.height);
                }
            }
            break;
        }
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::RegionsOverlapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEBFStats24::RegionsOverlapping(
    BFStatsROIDimensionParams currentROI,
    BFStatsROIDimensionParams nextROI)
{
    BOOL result = FALSE;

    // Check if the current and next ROI are overlapping
    if ((nextROI.ROI.left < (currentROI.ROI.left + currentROI.ROI.width)) &&
        (nextROI.ROI.top  < (currentROI.ROI.top + currentROI.ROI.height)))
    {
        if ((nextROI.ROI.left >= currentROI.ROI.left))
        {
            result = TRUE;
        }
        else if ((nextROI.ROI.left + nextROI.ROI.width) >= currentROI.ROI.left)
        {
            result = TRUE;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::CheckMaxHorizontalLimitationPerRow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats24::CheckMaxHorizontalLimitationPerRow()
{
    BFStatsROIDimensionParams* pCurrentROI = NULL;
    BFStatsROIConfigType*      pROIConfig  = &m_BFStatsConfig.BFStats.BFStatsROIConfig;
    UINT32                     index       = 0;
    CamxResult                 result      = CamxResultSuccess;

    Utils::Memset(m_pHorizontalRegionCount, 0, (m_CAMIFHeight * sizeof(UINT32)));

    // Count no of horizontal regions in each row and check if it exeeds the threshold per line
    for (index = 0; index < pROIConfig->numBFStatsROIDimension; index++)
    {
        pCurrentROI = &m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[index];
        if (FALSE == pCurrentROI->isValid)
        {
            continue;
        }

        if (pCurrentROI->ROI.top < m_CAMIFHeight)
        {
            m_pHorizontalRegionCount[pCurrentROI->ROI.top]++;
            if (m_pHorizontalRegionCount[pCurrentROI->ROI.top] > IFEBF24MaxHorizontalRegions)
            {
                // Can't adjust ROI's, return error and do not consume entire ROI config
                result = CamxResultEInvalidState;
                CAMX_ASSERT_ALWAYS_MESSAGE("No of horizontal regions exceeded limit %d for row %d",
                                           IFEBF24MaxHorizontalRegions, index);
                break;
            }
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid ROI starting line %d CAMIF height %d",
                                       pCurrentROI->ROI.top, m_CAMIFHeight);
            pCurrentROI->isValid = FALSE;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::CheckMaxVerticalSpacingAndDisjoint
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats24::CheckMaxVerticalSpacingAndDisjoint()
{
    BOOL                       result        = CamxResultSuccess;
    BFStatsROIConfigType*      pROIConfig    = NULL;
    BFStatsROIDimensionParams* pCurrentROI   = NULL;
    BFStatsROIDimensionParams* pReferenceROI = NULL;
    UINT32                     currentEnd    = 0;
    UINT32                     nextStart     = 0;
    UINT32                     index         = 0;

    pROIConfig = &m_BFStatsConfig.BFStats.BFStatsROIConfig;

    for (index = 0; ((index < pROIConfig->numBFStatsROIDimension) && (TRUE == result)); index++)
    {
        // Boundary check to make sure we are not going beyond number of ROI's
        if ((index + IFEBF24MaxHorizontalRegions) >= pROIConfig->numBFStatsROIDimension)
        {
            break;
        }

        pCurrentROI   = &m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[index];
        pReferenceROI = &m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[index + IFEBF24MaxHorizontalRegions];

        if ((FALSE == pCurrentROI->isValid) || (FALSE == pReferenceROI->isValid))
        {
            continue;
        }

        // Check whether current index and (index + maxHorizontalRegions) are spaced apart by maxVerticalSpacing lines

        if (((pReferenceROI->ROI.top + pReferenceROI->ROI.height) -
            (pCurrentROI->ROI.top + pCurrentROI->ROI.height)) < IFEBF24MaxVerticalSpacing)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "vertical spacing constrain failed at %d [%d,%d]and %d [%d, %d]",
                           index,
                           pCurrentROI->ROI.top,
                           pCurrentROI->ROI.height,
                           (index + IFEBF24MaxHorizontalRegions),
                           pReferenceROI->ROI.top,
                           pReferenceROI->ROI.height);

            // Cannot adjust ROI's, return error and do not consume enter ROI config
            result = CamxResultEInvalidArg;
            break;
        }

        // Check whether current index and (index + maxHorizontalRegions) are disjoint
        currentEnd = ((pCurrentROI->ROI.top + pCurrentROI->ROI.height) * m_CAMIFWidth) +
                      pCurrentROI->ROI.left + pCurrentROI->ROI.width;

        nextStart  = (pReferenceROI->ROI.top * m_CAMIFWidth) + pReferenceROI->ROI.left;

        if (currentEnd >= nextStart)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "failed: i (%d,%d,%d,%d) and (%d) (%d %d %d %d) not disjoint",
                           index,
                           pCurrentROI->ROI.left,
                           pCurrentROI->ROI.top,
                           pCurrentROI->ROI.width,
                           pCurrentROI->ROI.height,
                           (index + IFEBF24MaxHorizontalRegions),
                           pReferenceROI->ROI.left,
                           pReferenceROI->ROI.top,
                           pReferenceROI->ROI.width,
                           pReferenceROI->ROI.height);

            // Cannot adjust ROI's, return error and do not consume enter ROI config
            result = CamxResultEInvalidArg;
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::CheckMaxRegionOverlap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEBFStats24::CheckMaxRegionOverlap()
{
    BOOL                       result       = CamxResultSuccess;
    BFStatsROIConfigType*      pROIConfig   = &m_BFStatsConfig.BFStats.BFStatsROIConfig;
    BFStatsROIDimensionParams* pCurrentROI  = NULL;
    BFStatsROIDimensionParams* pNextROI     = NULL;
    UINT32                     overlapCount = 0;
    UINT32                     index        = 0;
    UINT32                     nextIndex    = 0;

    for (index = 0; ((index < pROIConfig->numBFStatsROIDimension) && (CamxResultSuccess == result)); index++)
    {
        pCurrentROI = &m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[index];
        if (FALSE == pCurrentROI->isValid)
        {
            continue;
        }

        overlapCount = 0;
        for (nextIndex = index + 1; (nextIndex < pROIConfig->numBFStatsROIDimension); nextIndex++)
        {
            if (FALSE == pROIConfig->BFStatsROIDimension[nextIndex].isValid)
            {
                continue;
            }
            pNextROI = &m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[nextIndex];

            if (TRUE == RegionsOverlapping(*pCurrentROI, *pNextROI))
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Regions are overlapping curRegNum %d (%d %d %d %d) NexRegNum %d(%d %d %d %d)",
                    pCurrentROI->regionNum, pCurrentROI->ROI.left, pCurrentROI->ROI.top,
                    pCurrentROI->ROI.width, pCurrentROI->ROI.height,
                    pNextROI->regionNum, pNextROI->ROI.left, pNextROI->ROI.top,
                    pNextROI->ROI.width, pNextROI->ROI.height);
                pCurrentROI->isValid = FALSE;
            }

            // Check whether overlapCount is within limits
            if (overlapCount > IFEBF24MaxOverlappingRegions)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "failed: overlap_count exceeded max %d", IFEBF24MaxOverlappingRegions);
                // Cannot adjust ROI's, return error and do not consume enter ROI config
                result = CamxResultEInvalidArg;
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::LumaConversionConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::LumaConversionConfig()
{
    BOOL  LSBAlignment         = FALSE;
    BOOL  forceYChannel        = FALSE;
    BOOL  overrideYCoefficient = FALSE;
    FLOAT ratio                = 1.0f;

    m_gammaDownscaleFactor = 1;

    if ((TRUE == IFEBF24BFGammaUseYChannel) || (TRUE == LSBAlignment))
    {
        if (BFChannelSelectG == m_BFStatsConfig.BFStats.BFInputConfig.BFChannelSel)
        {
            forceYChannel          = TRUE;
            m_gammaDownscaleFactor = IFEBF24BFGammaDownscaleFactorTwo;
        }
        else if (BFChannelSelectY == m_BFStatsConfig.BFStats.BFInputConfig.BFChannelSel)
        {
            if (TRUE == YCoefficientSumCheck())
            {
                overrideYCoefficient   = TRUE;
                m_gammaDownscaleFactor = IFEBF24BFGammaDownscaleFactorTwo;
            }
            else
            {
                overrideYCoefficient   = FALSE;
                m_gammaDownscaleFactor = IFEBF24BFGammaDownscaleFactorFour;
            }
        }
        // In HDR mode only 10 bits of LSB is valid, Gamma lookup is indexed by MSB's of 5 bits,
        // So in zzHDR mode only 1 bit is valid. To convert 5 bit index to 2 bit index dividing by 8/16
        if (TRUE == LSBAlignment)
        {
            m_gammaDownscaleFactor *= IFEBF24BFGammaDownscaleFactorFour;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "forceYChannel %d, overrideYCoefficient %d",
                         forceYChannel,
                         overrideYCoefficient);
    }

    // Configure Luma(Y) conversion registers
    if (BFChannelSelectY == m_BFStatsConfig.BFStats.BFInputConfig.BFChannelSel)
    {
        m_regCmd.BFConfig.config.bits.CH_SEL = IFEBF24ConfigChannelSelectY;
        if (TRUE == overrideYCoefficient)
        {
            ratio = IFEBF24YAConfigOverrideRatioMax;
        }
        else
        {
            ratio = IFEBF24YAConfigNoOverrideRatioMax;
        }

        m_regCmd.BFConfig.yConvConfig0.bits.A0 = Utils::FLOATToSFixed32(
            (m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[0] * ratio), IFEBF24LumaConversionConfigShift, TRUE);
        m_regCmd.BFConfig.yConvConfig0.bits.A1 = Utils::FLOATToSFixed32(
            (m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[1] * ratio), IFEBF24LumaConversionConfigShift, TRUE);
        m_regCmd.BFConfig.yConvConfig1.bits.A2 = Utils::FLOATToSFixed32(
            (m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[2] * ratio), IFEBF24LumaConversionConfigShift, TRUE);
    }
    else if (BFChannelSelectG == m_BFStatsConfig.BFStats.BFInputConfig.BFChannelSel)
    {
        if (TRUE == forceYChannel)
        {
            m_regCmd.BFConfig.yConvConfig0.bits.A0 = Utils::FLOATToSFixed32(0.0f, IFEBF24LumaConversionConfigShift, TRUE);
            m_regCmd.BFConfig.yConvConfig0.bits.A1 = Utils::FLOATToSFixed32(1.99f, IFEBF24LumaConversionConfigShift, TRUE);
            m_regCmd.BFConfig.yConvConfig1.bits.A2 = Utils::FLOATToSFixed32(0.0f, IFEBF24LumaConversionConfigShift, TRUE);

            m_regCmd.BFConfig.config.bits.CH_SEL = IFEBF24ConfigChannelSelectY;
        }
        else
        {
            m_regCmd.BFConfig.config.bits.CH_SEL = IFEBF24ConfigChannelSelectG;
        }
    }

    if (BFInputSelectGr == m_BFStatsConfig.BFStats.BFInputConfig.BFInputGSel)
    {
        m_regCmd.BFConfig.config.bits.G_SEL = IFEBF24ConfigGSelectGR;
    }
    else if (BFInputSelectGb == m_BFStatsConfig.BFStats.BFInputConfig.BFInputGSel)
    {
        m_regCmd.BFConfig.config.bits.G_SEL = IFEBF24ConfigGSelectGB;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                     "Luma conversion coefficients A0 %d A1 %d A2 %d ",
                     m_regCmd.BFConfig.yConvConfig0.bits.A0,
                     m_regCmd.BFConfig.yConvConfig0.bits.A1,
                     m_regCmd.BFConfig.yConvConfig1.bits.A2);
    CAMX_LOG_VERBOSE(CamxLogGroupISP,
                     " Luma conversion Input coefficients ipA0 %f ipA1 %f ipA2 %f",
                     m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[0],
                     m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[1],
                     m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[2]);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEBFStats24::YCoefficientSumCheck
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEBFStats24::YCoefficientSumCheck() const
{
    FLOAT sum    = 0.0f;
    BOOL  result = FALSE;

    sum = m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[0] +
          m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[1] +
          m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[2];

    if (sum <= IFEBF24YAConfigMax)
    {
        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::DownscalerConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::DownscalerConfig(
    ISPInputData*   pInputData,
    AFConfigParams* pBFStatsConfig)
{
    UINT32 interpolationResolution = 0;
    UINT32 inputWidth              = 0;
    UINT32 inputHeight             = 0;
    UINT32 outputWidth             = 0;
    IFEStripeStripeROI input_ROI         = { 0, 0 };
    IFEStripeStripeROI output_ROI        = { 0, 0 };

    IFEStripeMNScaleDownInputV16  mnds_config_y;  ///< Y config from striping
    IFEStripeMNScaleDownInputV16  mnds_config_c;  ///< C config from striping

    CAMX_ASSERT(NULL != pBFStatsConfig);

    inputWidth  = pInputData->pStripeConfig->CAMIFCrop.lastPixel - pInputData->pStripeConfig->CAMIFCrop.firstPixel + 1;
    inputHeight = pInputData->pStripeConfig->CAMIFCrop.lastLine - pInputData->pStripeConfig->CAMIFCrop.firstLine + 1;

    if ((FALSE == pBFStatsConfig->BFStats.BFScaleConfig.BFScaleEnable) ||
            (0 == pBFStatsConfig->BFStats.BFScaleConfig.scaleN)        ||
            (0 == pBFStatsConfig->BFStats.BFScaleConfig.scaleM))
    {
        pBFStatsConfig->BFStats.BFScaleConfig.scaleN = inputWidth;
        pBFStatsConfig->BFStats.BFScaleConfig.scaleM = (inputWidth / 2);

        m_regCmd.BFConfig.config.bits.H_SCALE_EN = 0x0;

        if (CropTypeCentered == pInputData->pStripeConfig->cropType)
        {
            m_regCmd.BFConfig.config.bits.SCALE_EN = 1;
            m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN       =
                Utils::MinUINT32(IFEBF24ScaleMaxHorizontalConfig, inputWidth) - 1;
            m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_OUT      =
                (((m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN + 1) *
                (pBFStatsConfig->BFStats.BFScaleConfig.scaleM)) / (pBFStatsConfig->BFStats.BFScaleConfig.scaleN)) - 1;

            interpolationResolution  = DownscalerCalculateInterpolationResolution(pBFStatsConfig);
            m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN =
               ((m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN + 1) / 2) - 1;
            m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_OUT =
               ((m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_OUT + 1) / 2) - 1;
            m_regCmd.phaseStripeConfig.hScalePhaseConfig.bits.H_INTERP_RESO = interpolationResolution;
            m_regCmd.phaseStripeConfig.hScalePhaseConfig.bits.H_PHASE_MULT   =
                (2 << (interpolationResolution + IFEBF24InterpolationShift));
            m_regCmd.phaseStripeConfig.hScaleStripeConfig1.bits.H_PHASE_INIT = 0;
            m_regCmd.phaseStripeConfig.hScaleStripeConfig0.bits.H_MN_INIT    = 0;
            m_regCmd.phaseStripeConfig.hScalePadConfig.bits.H_SKIP_CNT       = 0;
            m_regCmd.phaseStripeConfig.hScalePadConfig.bits.SCALE_Y_IN_WIDTH =
                m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN;
        }
        pBFStatsConfig->BFStats.BFScaleConfig.scaleN = 0;
        pBFStatsConfig->BFStats.BFScaleConfig.scaleM = 0;
    }
    else
    {
        if (TRUE == pInputData->pStripeConfig->overwriteStripes)
        {
            m_regCmd.BFConfig.config.bits.SCALE_EN                          =
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.BFScaleEnable;
            m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN       =
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.scaleN - 1;
            m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_OUT      =
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.scaleM - 1;
            m_regCmd.phaseStripeConfig.hScalePhaseConfig.bits.H_INTERP_RESO =
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.interpolationResolution;
            m_regCmd.phaseStripeConfig.hScalePhaseConfig.bits.H_PHASE_MULT   =
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.phaseStep;
            m_regCmd.phaseStripeConfig.hScaleStripeConfig1.bits.H_PHASE_INIT =
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.phaseInit;
            m_regCmd.phaseStripeConfig.hScaleStripeConfig0.bits.H_MN_INIT    =
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.mnInit;
            m_regCmd.phaseStripeConfig.hScalePadConfig.bits.H_SKIP_CNT       =
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.pixelOffset;
            m_regCmd.phaseStripeConfig.hScalePadConfig.bits.SCALE_Y_IN_WIDTH =
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.inputImageWidth - 1;
        }
        else
        {
            m_regCmd.BFConfig.config.bits.H_SCALE_EN                    = 0x1;
            m_regCmd.BFConfig.config.bits.SCALE_EN                      = 0x1;
            m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN  =
                Utils::MinUINT32(IFEBF24ScaleMaxHorizontalConfig, inputWidth) - 1;
            m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_OUT =
                (((m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN + 1) *
                (pBFStatsConfig->BFStats.BFScaleConfig.scaleM)) /
                    (pBFStatsConfig->BFStats.BFScaleConfig.scaleN)) - 1;

            interpolationResolution = DownscalerCalculateInterpolationResolution(pBFStatsConfig);

            m_regCmd.phaseStripeConfig.hScalePhaseConfig.bits.H_INTERP_RESO = interpolationResolution;
            m_regCmd.phaseStripeConfig.hScalePhaseConfig.bits.H_PHASE_MULT   =
                (m_CAMIFWidth << (interpolationResolution + IFEBF24PhaseAdder)) /
                (m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_OUT + 1);
            m_regCmd.phaseStripeConfig.hScaleStripeConfig1.bits.H_PHASE_INIT = 0;
            m_regCmd.phaseStripeConfig.hScaleStripeConfig0.bits.H_MN_INIT    = 0;
            m_regCmd.phaseStripeConfig.hScalePadConfig.bits.H_SKIP_CNT       = 0;
            m_regCmd.phaseStripeConfig.hScalePadConfig.bits.SCALE_Y_IN_WIDTH =
                m_regCmd.phaseStripeConfig.hScaleImageSizeConfig.bits.H_IN;
        }
    }
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "CropType %d H_SCALE_EN %d,scale en %d, H_IN %d,H_OUT %d,Reso %d"
        " Mul %d, Init %d, MN %d, Skip %d YWidth %d scale %d N %d, M %d",
        pInputData->pStripeConfig->cropType, m_regCmd.BFConfig.config.bits.H_SCALE_EN,
        pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.BFScaleEnable,
        pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.scaleN - 1,
        pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.scaleM - 1,
        pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.interpolationResolution,
        pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.phaseStep,
        pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.phaseInit,
        pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.mnInit,
        pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.pixelOffset,
        pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.inputImageWidth - 1,
        pBFStatsConfig->BFStats.BFScaleConfig.BFScaleEnable,
        pBFStatsConfig->BFStats.BFScaleConfig.scaleN,
        pBFStatsConfig->BFStats.BFScaleConfig.scaleM);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::DownscalerCalculateInterpolationResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEBFStats24::DownscalerCalculateInterpolationResolution(
    AFConfigParams* pBFStatsConfig
    ) const
{
    UINT32 ratio                   = 0;
    UINT32 interpolationResolution = IFEBF24InterpolationResolution3;

    CAMX_ASSERT(NULL != pBFStatsConfig);

    if (0 != pBFStatsConfig->BFStats.BFScaleConfig.scaleM)
    {
        ratio = (pBFStatsConfig->BFStats.BFScaleConfig.scaleN / pBFStatsConfig->BFStats.BFScaleConfig.scaleM);
        if (ratio >= IFEBF24ScaleRatio16)
        {
            interpolationResolution = IFEBF24InterpolationResolution0;
        }
        else if (ratio >= IFEBF24ScaleRatio8)
        {
            interpolationResolution = IFEBF24InterpolationResolution1;
        }
        else if (ratio >= IFEBF24ScaleRatio4)
        {
            interpolationResolution = IFEBF24InterpolationResolution2;
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupISP,
                          "Invalid ratio %d, default interpolationResolution 3", ratio);
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid scaler configuration");
    }

    return interpolationResolution;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::ConfigureFilters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::ConfigureFilters()
{
    BFFilterConfigParams*       pFilterConfig = NULL;
    BFFIRFilterConfigType*      pFIRConfig    = NULL;
    BFIIRFilterConfigType*      pIIRConfig    = NULL;
    BFFilterCoringConfigParams* pCoringConfig = NULL;

    // 1. Horizontal 1 FIR, IIR configuration
    pFilterConfig = &m_BFStatsConfig.BFStats.BFFilterConfig[BFFilterTypeHorizontal1];
    pFIRConfig    = &(pFilterConfig->BFFIRFilterConfig);
    pIIRConfig    = &(pFilterConfig->BFIIRFilterConfig);

    // 1a. FIR config
    if (TRUE == pFIRConfig->enable)
    {
        m_regCmd.BFConfig.config.bits.H_1_FIR_EN = 0x1;
        ConfigureFIRFilters(pFIRConfig, IFEBF24FilterType::Horizontal1);
    }
    else
    {
        m_regCmd.BFConfig.config.bits.H_1_FIR_EN = 0x0;
    }

    // 1b. IIR Filter
    if (TRUE == pIIRConfig->enable)
    {
        m_regCmd.BFConfig.config.bits.H_1_IIR_EN = 0x1;
        ConfigureIIRFilters(pIIRConfig, IFEBF24FilterType::Horizontal1);
    }
    else
    {
        m_regCmd.BFConfig.config.bits.H_1_IIR_EN = 0x0;
    }


    // 2. Horizontal 2 FIR, IIR configuration
    pFilterConfig = &m_BFStatsConfig.BFStats.BFFilterConfig[BFFilterTypeHorizontal2];
    pFIRConfig    = &(pFilterConfig->BFFIRFilterConfig);
    pIIRConfig    = &(pFilterConfig->BFIIRFilterConfig);

    // 3. Vertical FIR, IIR configuration
    pFilterConfig = &m_BFStatsConfig.BFStats.BFFilterConfig[BFFilterTypeVertical];
    pFIRConfig    = &(pFilterConfig->BFFIRFilterConfig);
    pIIRConfig    = &(pFilterConfig->BFIIRFilterConfig);

    // 3b. IIR Filter
    if (TRUE == pIIRConfig->enable)
    {
        m_regCmd.BFConfig.config.bits.V_IIR_EN = 0x1;
        ConfigureIIRFilters(pIIRConfig, IFEBF24FilterType::Vertical);
    }
    else
    {
        m_regCmd.BFConfig.config.bits.V_IIR_EN = 0x0;
    }

    /// FIGURE: h1_pix_sum and v_pix_sum

    // 4. Shift bit config
    m_regCmd.shiftBitsConfig.bits.H_1 =
        Utils::TwosComplementValidBits(
            m_BFStatsConfig.BFStats.BFFilterConfig[BFFilterTypeHorizontal1].shiftBits, IFEBF24ShifterBits);

    m_regCmd.shiftBitsConfig.bits.V   =
        Utils::TwosComplementValidBits(
            m_BFStatsConfig.BFStats.BFFilterConfig[BFFilterTypeVertical].shiftBits, IFEBF24ShifterBits);

    // 5. Coring Configuration for , H1, H2, V and Gain
    // 5a. Horizontal 1 coring config
    pFilterConfig = &m_BFStatsConfig.BFStats.BFFilterConfig[BFFilterTypeHorizontal1];
    pCoringConfig = &(pFilterConfig->BFFilterCoringConfig);
    ConfigureCoring(pCoringConfig, IFEBF24FilterType::Horizontal1);

    // 5c. Vertical coring config
    pFilterConfig = &m_BFStatsConfig.BFStats.BFFilterConfig[BFFilterTypeVertical];
    pCoringConfig = &(pFilterConfig->BFFilterCoringConfig);
    ConfigureCoring(pCoringConfig, IFEBF24FilterType::Vertical);


    // 5d. Coring Gain config
    m_regCmd.coringConfig.coringGainConfig0.bits.H_1_GAIN =
        m_BFStatsConfig.BFStats.BFFilterConfig[BFFilterTypeHorizontal1].BFFilterCoringConfig.gain & IFEBF24CoringBits;
    m_regCmd.coringConfig.coringGainConfig1.bits.V_GAIN   =
        m_BFStatsConfig.BFStats.BFFilterConfig[BFFilterTypeVertical].BFFilterCoringConfig.gain & IFEBF24CoringBits;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::SetActiveWindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::SetActiveWindow()
{
    /// @todo (CAMX-1403): Change the SetActiveWindow to start from first ROI
    m_regCmd.activeWindow.bafActiveWindow1.bits.X_MIN = IFEBF24HorizontalIIRMargin;
    m_regCmd.activeWindow.bafActiveWindow1.bits.Y_MIN = IFEBF24VerticalIIRMargin;
    m_regCmd.activeWindow.bafActiveWindow2.bits.X_MAX = m_CAMIFWidth - IFEBF24HorizontalIIRMargin;
    m_regCmd.activeWindow.bafActiveWindow2.bits.Y_MAX = m_CAMIFHeight - IFEBF24VerticalIIRMargin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::ConfigureFIRFilters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::ConfigureFIRFilters(
    BFFIRFilterConfigType*      pFIRConfig,
    IFEBF24FilterType             filterType)
{
    if (IFEBF24FilterType::Horizontal1 == filterType)
    {
        m_regCmd.hFIRIIRConfig.h1FIRConfig0.bits.A0  =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[0], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig0.bits.A1  =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[1], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig0.bits.A2  =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[2], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig0.bits.A3  =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[3], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig1.bits.A4  =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[4], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig1.bits.A5  =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[5], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig1.bits.A6  =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[6], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig1.bits.A7  =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[7], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig2.bits.A8  =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[8], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig2.bits.A9  =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[9], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig2.bits.A10 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[10], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig2.bits.A11 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[11], IFEBF24FIRBits);
        m_regCmd.hFIRIIRConfig.h1FIRConfig3.bits.A12 =
            Utils::TwosComplementValidBits(pFIRConfig->FIRFilterCoefficients[12], IFEBF24FIRBits);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::ConfigureIIRFilters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::ConfigureIIRFilters(
    BFIIRFilterConfigType*    pIIRConfig,
    IFEBF24FilterType           filterType)
{
    if (IFEBF24FilterType::Vertical == filterType)
    {
        m_regCmd.vIIRConfig1.vIIRConfig0.bits.B10 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b10, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.vIIRConfig1.vIIRConfig0.bits.B11 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b11, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.vIIRConfig1.vIIRConfig1.bits.B12 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b12, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.vIIRConfig1.vIIRConfig1.bits.B22 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b22, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.vIIRConfig1.vIIRConfig2.bits.A11 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a11, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.vIIRConfig1.vIIRConfig2.bits.A12 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a12, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.vIIRConfig2.vIIRConfig3.bits.B20 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b20, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.vIIRConfig2.vIIRConfig3.bits.B21 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b21, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.vIIRConfig2.vIIRConfig4.bits.A21 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a21, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.vIIRConfig2.vIIRConfig4.bits.A22 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a22, 14, TRUE), IFEBF24IIRBits);
    }
    else if (IFEBF24FilterType::Horizontal1 == filterType)
    {
        m_regCmd.hFIRIIRConfig.h1IIRConfig0.bits.B10 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b10, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig0.bits.B11 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b11, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig1.bits.B12 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b12, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig1.bits.B22 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b22, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig2.bits.A11 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a11, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig2.bits.A12 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a12, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig3.bits.B20 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b20, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig3.bits.B21 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->b21, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig4.bits.A21 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a21, 14, TRUE), IFEBF24IIRBits);
        m_regCmd.hFIRIIRConfig.h1IIRConfig4.bits.A22 =
            Utils::TwosComplementValidBits(Utils::FLOATToSFixed32(pIIRConfig->a22, 14, TRUE), IFEBF24IIRBits);
    }
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::UpdateROIDMITable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::UpdateROIDMITable(
    BFStatsROIConfigType*   pHWROIConfig,
    UINT64*                 pROIDMIConfig,
    UINT32                  sizeOfDMIConfig)
{
    UINT32 index = 0;

    CAMX_ASSERT(NULL != pHWROIConfig);
    CAMX_ASSERT(NULL != pROIDMIConfig);

    // Increment Number of ROI by 1 to account for BF Tag
    pHWROIConfig->numBFStatsROIDimension++;

    Utils::Memset(pROIDMIConfig, 0, sizeOfDMIConfig);

    // first entry will be a  DMI tag - updating data from 2nd entry
    for (index = 0; index < pHWROIConfig->numBFStatsROIDimension; index++)
    {
        pROIDMIConfig[index + 1] =
            ((static_cast<UINT64>(pHWROIConfig->BFStatsROIDimension[index].region)) << IFEBF24DMISelShift) |
            ((static_cast<UINT64>(pHWROIConfig->BFStatsROIDimension[index].regionNum)) << IFEBF24DMIIndexShift) |
            (((static_cast<UINT64>(pHWROIConfig->BFStatsROIDimension[index].ROI.left))   &
                IFEBF24DMILeftBits) << IFEBF24DMILeftShift) |
            (((static_cast<UINT64>(pHWROIConfig->BFStatsROIDimension[index].ROI.top))    &
                IFEBF24DMITopBits) << IFEBF24DMITopShift) |
            (((static_cast<UINT64>(pHWROIConfig->BFStatsROIDimension[index].ROI.width))  &
                IFEBF24DMIWidthBits) << IFEBF24DMIWidthShift) |
            (((static_cast<UINT64>(pHWROIConfig->BFStatsROIDimension[index].ROI.height)) &
                IFEBF24DMIHeightBits) << IFEBF24DMIHeightShift);
    }

    // Write end signature DMI config
    pROIDMIConfig[index + 1] = 0;

    // Increment Number of ROI by 1 to account for last DMI config
    pHWROIConfig->numBFStatsROIDimension++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::UpdateHWROI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::UpdateHWROI(
    BFStatsROIConfigType* pInputROIConfig,
    BFStatsROIConfigType* pHWROIConfig)
{
    UINT32                     index = 0;
    UINT32                     numberOfRegions = 0;
    BFStatsROIDimensionParams* pROI = NULL;

    CAMX_ASSERT(NULL != pInputROIConfig);
    CAMX_ASSERT(NULL != pHWROIConfig);

    Utils::Memset(pHWROIConfig, 0, sizeof(BFStatsROIConfigType));

    for (index = 0; index < pInputROIConfig->numBFStatsROIDimension; index++)
    {
        pROI = &pInputROIConfig->BFStatsROIDimension[index];

        if (TRUE == pROI->isValid)
        {
            pHWROIConfig->BFStatsROIDimension[numberOfRegions].ROI.left = pROI->ROI.left;
            pHWROIConfig->BFStatsROIDimension[numberOfRegions].ROI.top = pROI->ROI.top;
            pHWROIConfig->BFStatsROIDimension[numberOfRegions].ROI.width = pROI->ROI.width;
            pHWROIConfig->BFStatsROIDimension[numberOfRegions].ROI.height = pROI->ROI.height;
            pHWROIConfig->BFStatsROIDimension[numberOfRegions].region = pROI->region;
            pHWROIConfig->BFStatsROIDimension[numberOfRegions].regionNum = pROI->regionNum;
            pHWROIConfig->BFStatsROIDimension[numberOfRegions].isValid = TRUE;
            numberOfRegions++;
        }
    }
    pHWROIConfig->numBFStatsROIDimension = numberOfRegions;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::SortRegionsEndPixelOrderBased
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::SortRegionsEndPixelOrderBased()
{
    BFStatsROIDimensionParams* pROI       = NULL;
    BFStatsROIDimensionParams* pNextROI   = NULL;
    BOOL                       swapped    = TRUE;
    UINT32                     index      = 0;
    UINT32                     nextIndex  = 0;
    UINT32                     currentEnd = 0;
    UINT32                     nextEnd    = 0;
    BFStatsROIDimensionParams  swapROI;

    m_endOrderROI = m_BFStatsConfig.BFStats.BFStatsROIConfig;

    while (TRUE == swapped)
    {
        swapped = FALSE;
        for (index = 0; index < m_endOrderROI.numBFStatsROIDimension; index++)
        {
            pROI = &m_endOrderROI.BFStatsROIDimension[index];
            if (TRUE == pROI->isValid)
            {
                // Calcuate last pixel in single dimension based on raster scan order
                currentEnd = ((pROI->ROI.top + pROI->ROI.height) * m_CAMIFWidth) + pROI->ROI.left + pROI->ROI.width;

                // Secondary loop is required as the next index ROI could be invalid
                for (nextIndex = index + 1; nextIndex < m_endOrderROI.numBFStatsROIDimension; nextIndex++)
                {
                    pNextROI = &m_endOrderROI.BFStatsROIDimension[nextIndex];
                    if (TRUE == pNextROI->isValid)
                    {
                        nextEnd = ((pNextROI->ROI.top + pNextROI->ROI.height) * m_CAMIFWidth) +
                            pNextROI->ROI.left + pNextROI->ROI.width;

                        // Bubble sort
                        if (currentEnd > nextEnd)
                        {
                            swapROI   = *pROI;
                            *pROI     = *pNextROI;
                            *pNextROI = swapROI;
                            swapped   = TRUE;
                        }
                        break;
                    }
                }
            }
        }
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::ConfigureCoring
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::ConfigureCoring(
    BFFilterCoringConfigParams* pCoringConfig,
    IFEBF24FilterType             filterType)
{
    if (filterType == IFEBF24FilterType::Horizontal1)
    {
        m_regCmd.hThresholdCoringConfig.h1ThresholdConfig.bits.THRESHOLD = pCoringConfig->threshold & IFEBF24ThresholdMask;
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
    else  if (filterType == IFEBF24FilterType::Vertical)
    {
        m_regCmd.vThresholdCoringConfig.vThresholdConfig.bits.THRESHOLD  = pCoringConfig->threshold & IFEBF24ThresholdMask;
        m_regCmd.vThresholdCoringConfig.vCoringConfig0.bits.IND_0        = pCoringConfig->core[0];
        m_regCmd.vThresholdCoringConfig.vCoringConfig0.bits.IND_1        = pCoringConfig->core[1];
        m_regCmd.vThresholdCoringConfig.vCoringConfig0.bits.IND_2        = pCoringConfig->core[2];
        m_regCmd.vThresholdCoringConfig.vCoringConfig0.bits.IND_3        = pCoringConfig->core[3];
        m_regCmd.vThresholdCoringConfig.vCoringConfig0.bits.IND_4        = pCoringConfig->core[4];
        m_regCmd.vThresholdCoringConfig.vCoringConfig1.bits.IND_5        = pCoringConfig->core[5];
        m_regCmd.vThresholdCoringConfig.vCoringConfig1.bits.IND_6        = pCoringConfig->core[6];
        m_regCmd.vThresholdCoringConfig.vCoringConfig1.bits.IND_7        = pCoringConfig->core[7];
        m_regCmd.vThresholdCoringConfig.vCoringConfig1.bits.IND_8        = pCoringConfig->core[8];
        m_regCmd.vThresholdCoringConfig.vCoringConfig1.bits.IND_9        = pCoringConfig->core[9];
        m_regCmd.vThresholdCoringConfig.vCoringConfig2.bits.IND_10       = pCoringConfig->core[10];
        m_regCmd.vThresholdCoringConfig.vCoringConfig2.bits.IND_11       = pCoringConfig->core[11];
        m_regCmd.vThresholdCoringConfig.vCoringConfig2.bits.IND_12       = pCoringConfig->core[12];
        m_regCmd.vThresholdCoringConfig.vCoringConfig2.bits.IND_13       = pCoringConfig->core[13];
        m_regCmd.vThresholdCoringConfig.vCoringConfig2.bits.IND_14       = pCoringConfig->core[14];
        m_regCmd.vThresholdCoringConfig.vCoringConfig3.bits.IND_15       = pCoringConfig->core[15];
        m_regCmd.vThresholdCoringConfig.vCoringConfig3.bits.IND_16       = pCoringConfig->core[16];
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::CheckAndAdjustEndPixelROILimitation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::CheckAndAdjustEndPixelROILimitation()
{
    BFStatsROIDimensionParams* pROI       = NULL;
    BFStatsROIDimensionParams* pNextROI   = NULL;
    UINT32                     index      = 0;
    UINT32                     nextIndex  = 0;
    UINT32                     currentEnd = 0;
    UINT32                     nextEnd    = 0;

    for (index = 0; index < m_endOrderROI.numBFStatsROIDimension; index++)
    {
        pROI = &m_endOrderROI.BFStatsROIDimension[index];
        if (TRUE == pROI->isValid)
        {
            // Calcuate last pixel in single dimension based on raster scan order
            currentEnd = (pROI->ROI.top * m_CAMIFWidth) + pROI->ROI.left + pROI->ROI.width;

            // Secondary loop is required as the next index ROI could be invalid
            for (nextIndex = index + 1; nextIndex < m_endOrderROI.numBFStatsROIDimension; nextIndex++)
            {
                pNextROI = &m_endOrderROI.BFStatsROIDimension[nextIndex];
                if (TRUE == pNextROI->isValid)
                {
                    nextEnd = (pNextROI->ROI.top * m_CAMIFWidth) + pNextROI->ROI.left + pNextROI->ROI.width;

                    // Checking whether both ROI ends on same pixel or ending line apart by min
                    if ((currentEnd == nextEnd) ||
                        (Utils::AbsoluteINT32((pROI->ROI.left + pROI->ROI.width) -
                                              (pNextROI->ROI.left + pNextROI->ROI.width))) <
                                               IFEBF24MinStartPixelOverlap)
                    {
                        CAMX_LOG_WARN(CamxLogGroupISP, "two ROI ending on same pixel [%d, %d, %d, %d]",
                            pROI->ROI.left, pROI->ROI.top, pROI->ROI.width, pROI->ROI.height);

                        if ((pNextROI->ROI.left + pNextROI->ROI.width + IFEBF24MinStartPixelOverlap) <=
                            (m_CAMIFWidth - IFEBF24HorizontalFIRMargin))
                        {
                            // Move ROI to right
                            pNextROI->ROI.left += IFEBF24MinStartPixelOverlap;
                        }
                        else
                        {
                            pNextROI->isValid = FALSE;
                        }
                    }
                    break;
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::ConfigureGamma
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEBFStats24::ConfigureGamma(
    ISPInputData* pInputData)
{
    UINT32 indexDownScaled              = 0;
    UINT32 indexRemaining               = 0;
    UINT32 indexAll                     = 0;
    UINT32 tempGammaLUT[IFEBF24BFGammaEntries] = {0};
    FLOAT  scaleRatio                   = 1.0f;
    UINT   lutIndex                     = 0;
    FLOAT  curX                         = 0.0f;
    FLOAT  interpolationRatio           = 0.0f;

    // Using index short for sensor gain/linecount
    AECExposureData* pExposure          = &(pInputData->pAECUpdateData->exposureInfo[ExposureIndexShort]);
    if (TRUE == pInputData->pAFUpdateData->exposureCompensationEnable)
    {
        scaleRatio = m_refSensitivity / (pExposure->exposureTime * pExposure->linearGain);
        scaleRatio = static_cast<FLOAT>(
            IQSettingUtils::ClampFLOAT(scaleRatio, IFEBF24BFGammaScaleRatioMinValue, IFEBF24BFGammaScaleRatioMaxValue));
    }
    else
    {
        m_refSensitivity = pExposure->exposureTime * pExposure->linearGain;
    }

    if ((TRUE == m_BFStatsConfig.BFStats.BFGammaLUTConfig.isValid) && (m_gammaDownscaleFactor > 0))
    {
        for (indexDownScaled = 0;
             indexDownScaled < ((m_BFStatsConfig.BFStats.BFGammaLUTConfig.numGammaLUT) / (m_gammaDownscaleFactor));
             indexDownScaled++)
        {
            tempGammaLUT[indexDownScaled] =
                Utils::ClampUINT32(m_BFStatsConfig.BFStats.BFGammaLUTConfig.gammaLUT[indexDownScaled],
                                   0,
                                   IFEBF24BFGammaMaxValue);
        }

        // Continue from (m_BFConfig.BFStats.BFGammaLUTConfig.numGammaLUT / m_gammaDownscaleFactor) to end of the LUT
        for (indexRemaining = indexDownScaled;
             indexRemaining < m_BFStatsConfig.BFStats.BFGammaLUTConfig.numGammaLUT;
             indexRemaining++)
        {
            // BAF gamma values are 14bit unsigned
            tempGammaLUT[indexRemaining] = IFEBF24BFGammaMaxValue;
        }

        if (FALSE == IQSettingUtils::FEqual(scaleRatio, 1.0f))
        {
            for (UINT32 i = 0; i < m_BFStatsConfig.BFStats.BFGammaLUTConfig.numGammaLUT; i++)
            {
                curX     = i * scaleRatio;
                lutIndex = static_cast<UINT>(curX);
                if (lutIndex > (m_BFStatsConfig.BFStats.BFGammaLUTConfig.numGammaLUT - 2))
                {
                    lutIndex = m_BFStatsConfig.BFStats.BFGammaLUTConfig.numGammaLUT - 2;
                }

                interpolationRatio = curX - lutIndex;
                m_gammaLUT[i] = static_cast<UINT32>(
                    (interpolationRatio * tempGammaLUT[lutIndex + 1]) +
                    ((1 - interpolationRatio) * tempGammaLUT[lutIndex]));
                m_gammaLUT[i] = Utils::ClampUINT32(m_gammaLUT[i], 0, IFEBF24BFGammaMaxValue);
            }
            Utils::Memcpy(tempGammaLUT, m_gammaLUT, sizeof(m_gammaLUT));
        }

        for (indexAll = 0; indexAll < (m_BFStatsConfig.BFStats.BFGammaLUTConfig.numGammaLUT - 1); indexAll++)
        {
            m_gammaLUT[indexAll] = GammaGetHighLowBits(&tempGammaLUT[0], indexAll);
        }

        m_gammaLUT[indexAll] = GammaGetLastValue(tempGammaLUT);

        m_regCmd.BFConfig.config.bits.GAMMA_LUT_EN        = 0x1;
    }
    else
    {
        m_regCmd.BFConfig.config.bits.GAMMA_LUT_EN = 0x0;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::GammaGetHighLowBits
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 IFEBFStats24::GammaGetHighLowBits(
    UINT32* pGamma,
    UINT32  index
    ) const
{
    UINT32 hwLUTEntry = 0;
    INT16  deltaLUT   = 0;

    if ((NULL != pGamma) && (index < IFEBF24BFGammaEntries))
    {
        deltaLUT = static_cast<INT16>(Utils::ClampINT32((pGamma[(index + 1)] - pGamma[index]),
                                                        IFEBF24BFGammaDeltaMinValue,
                                                        IFEBF24BFGammaDeltaMaxValue));
        hwLUTEntry = static_cast<UINT32>((deltaLUT << IFEBF24BFGammaDMIDeltaShift) + pGamma[index]);
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid input data");
    }
    return hwLUTEntry;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEBFStats24::GammaGetLastValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 IFEBFStats24::GammaGetLastValue(
    UINT32* pGamma
    ) const
{
    UINT32 hwLUTEntry = 0;
    INT16  deltaLUT   = 0;

    // consider the next entry of the last entry 2^12- 8084.
    //                                           2^14- 8994.
    // value suggested by system team

    UINT32 nextEntryFinal = 2 << IFEBF24BFGammaDMIDeltaShift;

    // this is effectively  table[255] - table[254] this part is the delta
    // use 256 as next entry of last entry
    deltaLUT = static_cast<INT16>(Utils::ClampINT32((nextEntryFinal - pGamma[(IFEBF24BFGammaEntries - 1)]),
                                                    IFEBF24BFGammaDeltaMinValue,
                                                    IFEBF24BFGammaDeltaMaxValue));

    // scale the delta
    // form the value:  upper byte is delta, lower byte is the entry itself
    hwLUTEntry = static_cast<UINT32>((deltaLUT << IFEBF24BFGammaDMIDeltaShift) + pGamma[(IFEBF24BFGammaEntries - 1)]);

    return hwLUTEntry;
}

CAMX_NAMESPACE_END
