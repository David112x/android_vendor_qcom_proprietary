////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2014-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifebfstats25.cpp
/// @brief IFEBFStats25 Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxcslifedefs.h"
#include "camxcslispdefs.h"
#include "camxhal3module.h"
#include "camxifebfstats25.h"
#include "camxifebfstats25titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BFStats25::Create(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        BFStats25* pModule = CAMX_NEW BFStats25;
        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultEFailed;
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to create BFStats25 object.");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Input Null pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BFStats25::Initialize(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEBFStats25Titan480;
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength      = m_pHWSetting->GetCommandLength();

        // For both Gamma LUT and ROI Index LUT are under 32-bit DMI
        m_32bitDMILength = m_pHWSetting->Get32bitDMILength();

        m_64bitDMILength = 0;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BFStats25::Execute(
    ISPInputData* pInputData)
{
    CamxResult result       = CamxResultSuccess;

    if (NULL != pInputData)
    {
        // Check if dependent is valid and been updated compared to last request
        result = ValidateDependenceParams(pInputData);
        if ((CamxResultSuccess    == result)                             &&
            (CamxResultSuccess    == CheckDependenceChange(pInputData))  &&
            ((m_ROIConfigUpdate   == TRUE)    ||
             (m_inputConfigUpdate == TRUE)    ||
             (TRUE == pInputData->forceTriggerUpdate)))
        {
            // Pack both ISP and BF stat config data for input
            BFStats25ConfigData  inputData = {};

            // Before invoking RunCalculation(), only ISPInputData is needed.
            inputData.pISPInputData = pInputData;

            RunCalculation(&inputData);

            UINT DMIOffsetArray[1] = { m_32bitDMIBufferOffsetDword };

            result = m_pHWSetting->CreateCmdList(&inputData, &DMIOffsetArray[0]);
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
// BFStats25::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BFStats25::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult  result           = CamxResultSuccess;
    UINT32      numberOfValidROI = 0;

    if (NULL != pInputData)
    {
        if (NULL != pInputData->pStripingInput)
        {
            pInputData->pStripingInput->enableBits.BAF              = m_moduleEnable;
            pInputData->pStripingInput->stripingInput.BAFEnable    = static_cast<int16_t>(m_moduleEnable);

            pInputData->pStripingInput->stripingInput.BAFInputv25.BAFHorizScalerEn = static_cast<uint16_t>(
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.BFScaleEnable);
            pInputData->pStripingInput->stripingInput.BAFInputv25.BAF_fir_h1_en   = static_cast<uint16_t>(
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFFilterConfig->BFFIRFilterConfig.enable);
            pInputData->pStripingInput->stripingInput.BAFInputv25.BAF_iir_h1_en   = static_cast<uint16_t>(
                pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFFilterConfig->BFIIRFilterConfig.enable);

            CAMX_LOG_VERBOSE(CamxLogGroupISP, "Striping input: en=%d, baf_enable=%d, scaler_en=%d, fir_h1=%d, iir_h1=%d",
                             pInputData->pStripingInput->enableBits.BAF,
                             pInputData->pStripingInput->stripingInput.BAFEnable,
                             pInputData->pStripingInput->stripingInput.BAFInputv25.BAFHorizScalerEn,
                             pInputData->pStripingInput->stripingInput.BAFInputv25.BAF_fir_h1_en,
                             pInputData->pStripingInput->stripingInput.BAFInputv25.BAF_iir_h1_en);

            m_CAMIFWidth  = pInputData->pStripeConfig->CAMIFCrop.lastPixel
                            - pInputData->pStripeConfig->CAMIFCrop.firstPixel + 1;
            m_CAMIFHeight = pInputData->pStripeConfig->CAMIFCrop.lastLine
                            - pInputData->pStripeConfig->CAMIFCrop.firstLine + 1;

            AFConfigParams*         pAFConfig   = &pInputData->pStripeConfig->AFStatsUpdateData.statsConfig;
            BFStatsROIConfigType    hwROIConfig;

            ValidateAndAdjustROIBoundary(pInputData, pAFConfig);

            if (pAFConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension > 0)
            {
                for (UINT index = 0; index < pAFConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension; index++)
                {
                    if (TRUE  == pAFConfig->BFStats.BFStatsROIConfig.BFStatsROIDimension[index].isValid)
                    {
                        numberOfValidROI++;
                    }
                }

            }

            if (numberOfValidROI < (pAFConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension / 2))
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

            UpdateHWROI(&pAFConfig->BFStats.BFStatsROIConfig, &hwROIConfig, 0, pInputData->pStripeConfig->overwriteStripes);

            UpdateROIDMITableForStripingInput(&hwROIConfig,
                pInputData->pStripingInput->stripingInput.BAFInputv25.BAFROIIndexLUT,
                sizeof(pInputData->pStripingInput->stripingInput.BAFInputv25.BAFROIIndexLUT));

            DownscalerConfig(pInputData, pAFConfig);
            if (TRUE == m_downscalerConfigInput.enableScaler)
            {
                pInputData->pStripingInput->stripingInput.BAFInputv25.mndsParam.enable =
                    static_cast<UINT16>(m_downscalerConfigInput.enableScaler);
                pInputData->pStripingInput->stripingInput.BAFInputv25.mndsParam.input =
                    static_cast<UINT16>(m_downscalerConfigInput.scalerImageSizeIn + 1);   // n + 1 means n
                pInputData->pStripingInput->stripingInput.BAFInputv25.mndsParam.output =
                    static_cast<UINT16>(m_downscalerConfigInput.scalerImageSizeOut + 1);  // n + 1 means n
                pInputData->pStripingInput->stripingInput.BAFInputv25.mndsParam.pixelOffset =
                    static_cast<UINT16>(m_downscalerConfigInput.scalerSkipCount);
                pInputData->pStripingInput->stripingInput.BAFInputv25.mndsParam.cntInit =
                    static_cast<UINT16>(m_downscalerConfigInput.scalerMNInitialValue);
                pInputData->pStripingInput->stripingInput.BAFInputv25.mndsParam.interpReso =
                    static_cast<UINT16>(m_downscalerConfigInput.scalerPhaseInterpolationResolution);
                pInputData->pStripingInput->stripingInput.BAFInputv25.mndsParam.roundingOptionHor =
                    static_cast<UINT16>(m_downscalerConfigInput.scalerRoundingPattern);
                pInputData->pStripingInput->stripingInput.BAFInputv25.mndsParam.inputProcessedLength =
                    static_cast<UINT16>(m_downscalerConfigInput.scalerInputWidth + 1); // n + 1 means n

                // Greater than 16-bit values, hence no need to casting.
                pInputData->pStripingInput->stripingInput.BAFInputv25.mndsParam.phaseInit =
                    m_downscalerConfigInput.scalerPhaseInitialValue;
                pInputData->pStripingInput->stripingInput.BAFInputv25.mndsParam.phaseStep =
                    m_downscalerConfigInput.scalerPhaseMultiplicationFactor;
            }
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
// BFStats25::GetDMITable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::GetDMITable(
    UINT32** ppDMITable)
{
    if (NULL == ppDMITable)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid pointer to write DMI into");
        return;
    }

    Utils::Memcpy(*ppDMITable, &m_ROIDMIConfig, (m_hwROIConfig.numBFStatsROIDimension * sizeof(BFStats25ROIIndexLUTType)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // Update crop Info metadata
    pInputData->pCalculatedData->metadata.BFStats.BFConfig = m_BFStatsConfig;

    pInputData->pCalculatedData->metadata.BFStats.isAdjusted = m_isInputConfigAdjusted;

    // Update WM Config
    UINT32 wmUpdateIndex = pInputData->pCalculatedData->WMUpdate.numberOfWMUpdates;

    if (ISPMaxOutputPorts > wmUpdateIndex)
    {
        pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].portID = IFEOutputStatsBF;
        pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].width  = BFStats25Width;
        pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].height = m_hwROIConfig.numBFStatsROIDimension;
        pInputData->pCalculatedData->WMUpdate.WMData[wmUpdateIndex].mode   = IFEWMModeIndexBased;
        pInputData->pCalculatedData->WMUpdate.numberOfWMUpdates++;
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupIQMod, "Invalid Number of Ports %d", wmUpdateIndex);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::BFStats25
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BFStats25::BFStats25()
    : m_gammaSupported(GammaSupported)
    , m_downScalerSupported(DownScalerSupported)
    , m_pHorizontalRegionCount(NULL)
    , m_inputConfigUpdate(FALSE)
    , m_ROIConfigUpdate(FALSE)
{

    m_DMITableLength = sizeof(m_ROIDMIConfig) + sizeof(m_gammaLUT);

    m_moduleEnable          = TRUE;
    m_type                  = ISPStatsModuleType::IFEBF;
    m_isInputConfigAdjusted = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::~BFStats25
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BFStats25::~BFStats25()
{
    // Free up allocated memory
    if (NULL != m_pHorizontalRegionCount)
    {
        CAMX_FREE(m_pHorizontalRegionCount);
        m_pHorizontalRegionCount = NULL;
    }
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BFStats25::ValidateDependenceParams(
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

        const CropInfo* pSensorCAMIFCrop    = &pInputData->pStripeConfig->CAMIFCrop;
        const INT32     inputWidth          = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
        const INT32     inputHeight         = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;

        // Validate CAMIF dimension
        if ((inputWidth <= 0) || (inputHeight <= 0))
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid CAMIF dimesion width = %d, height = %d", inputWidth, inputHeight);
            result = CamxResultEInvalidArg;
        }

        // Validate Input config
        if ((CamxResultSuccess  == result)                        &&
            (TRUE               == pBFInputConfigParams->isValid) &&
            (BFChannelSelectMax >  pBFInputConfigParams->BFChannelSel))
        {
            if (BFChannelSelectY == pBFInputConfigParams->BFChannelSel)
            {
                for (index = 0; index < MaxYConfig; index++)
                {
                    if ((YAConfigMin <= pBFInputConfigParams->YAConfig[index]) &&
                        (YAConfigMax >= pBFInputConfigParams->YAConfig[index]))
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
                if ((YAConfigMax       <  inputConfigCoefficientSum) &&
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
             (YAConfigCoEffSum           <  (pBFScaleConfigType->scaleM / pBFScaleConfigType->scaleN))))
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
// BFStats25::HandleEmptyROI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::HandleEmptyROI(
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
// BFStats25::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BFStats25::CheckDependenceChange(
    ISPInputData* pInputData)
{
    UINT32          inputWidth       = 0;
    UINT32          inputHeight      = 0;
    CropInfo*       pSensorCAMIFCrop = NULL;
    AFConfigParams* pAFConfig        = NULL;
    CamxResult      result           = CamxResultSuccess;
    UINT32          numberofValidROI = 0;

    // Reset the config update flag to FALSE.
    // Update BF calculation and register programming only if the config has been changed.
    m_ROIConfigUpdate   = FALSE;
    m_inputConfigUpdate = FALSE;

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
            if (TRUE == pAFConfig->BFStats.BFScaleConfig.isValid &&
                FALSE == pInputData->pStripeConfig->overwriteStripes)
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

                if ((0 == numberofValidROI)                                 ||
                    ((FALSE == pInputData->pStripeConfig->overwriteStripes) &&
                    (numberofValidROI < (m_BFStatsConfig.BFStats.BFStatsROIConfig.numBFStatsROIDimension / 2))))
                {
                    // This is a case Where all regions are invalid.. Add Dummy region
                    pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.
                        BFStats.BFStatsROIConfig.numBFStatsROIDimension = 0;
                    HandleEmptyROI(pInputData, &m_BFStatsConfig.BFStats.BFStatsROIConfig);
                    if (FALSE == pInputData->pStripeConfig->overwriteStripes)
                    {
                        // Update the previous valid config
                        Utils::Memcpy(&m_BFStatsConfig, &m_previousAFConfig, sizeof(AFConfigParams));
                        CAMX_LOG_WARN(CamxLogGroupISP, "Since all AF Config ROIS are invalid Using previous Valid Config for"
                                                       "Request ID %llu",
                                                       pInputData->frameNum);
                    }
                }
                else
                {
                    if (FALSE == pInputData->pStripeConfig->overwriteStripes)
                    {
                        Utils::Memcpy(&m_previousAFConfig, &m_BFStatsConfig, sizeof(AFConfigParams));
                    }
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
// BFStats25::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::RunCalculation(
    BFStats25ConfigData* pInputData)
{

    // 1. Calculate for luma conversion config
    if (TRUE == m_inputConfigUpdate)
    {
        CalculateGammaDownscaleFactor();
    }

    // 2. Downscaler calculations
    if (TRUE == m_downScalerSupported)
    {
        DownscalerConfig(pInputData->pISPInputData, &m_BFStatsConfig);
    }

    // 4a. Update the HW ROI config for single and Dual IFE and Store ROI for DMI write
    if (TRUE == m_ROIConfigUpdate)
    {
        SortRegionsEndPixelOrderBased();

        CheckAndAdjustEndPixelROILimitation();

        // Stripe ID is useful for debugging ROI configuration for dual IFE case.
        const UINT32    stripeId        = pInputData->pISPInputData->pStripeConfig->stripeId;
        BOOL            overwriteStripe = pInputData->pISPInputData->pStripeConfig->overwriteStripes;

        UpdateHWROI(&m_BFStatsConfig.BFStats.BFStatsROIConfig, &m_hwROIConfig, stripeId, overwriteStripe);

        UpdateROIDMITable(&m_hwROIConfig, m_ROIDMIConfig, sizeof(m_ROIDMIConfig), stripeId);
    }

    // 5. Configure Gamma
    if (TRUE == m_gammaSupported)
    {
        ConfigureGamma(pInputData->pISPInputData);
    }

    // Assign the member variable data as input data
    pInputData->pStatsConfig          = &m_BFStatsConfig;
    pInputData->pLumaConversionConfig = &m_lumaConversionConfigInput;
    pInputData->pDownscalerConfig     = &m_downscalerConfigInput;
    pInputData->inputConfigUpdate     = m_inputConfigUpdate;
    pInputData->downScalerSupported   = m_downScalerSupported;
    pInputData->pROIDMIConfig         = &m_ROIDMIConfig[0];     ///< DMI ROI configuration
    pInputData->pGammaLUT             = &m_gammaLUT[0];         ///< Gamma LUT
    pInputData->pHwROIConfig          = &m_hwROIConfig;
    pInputData->enableGammaLUT        = m_enableGammaLUT;

    // Pass pInputData as const data.
    m_pHWSetting->PackIQRegisterSetting(pInputData, NULL);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::AdjustScaledROIBoundary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::AdjustScaledROIBoundary(
    ISPInputData*   pInputData,
    RectangleCoordinate* pRoi)
{
    UINT32 scaleM = pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.scaleM;
    UINT32 scaleN = pInputData->pStripeConfig->AFStatsUpdateData.statsConfig.BFStats.BFScaleConfig.scaleN;
    UINT32 left   = 0;
    UINT64 width  = 0;
    INT32  temp   = 0;
    if ((scaleM != 0) && (scaleN != 0))
    {
        left = pRoi->left * scaleM;
        pRoi->left = (left + (scaleN - 1)) / scaleN;

        width = (pRoi->width + 1) * scaleM;
        temp  = (width / scaleN) - 1;
        if (temp < 0)
        {
            pRoi->width = 0;
        }
        else
        {
            pRoi->width = static_cast<UINT32>(temp);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::ValidateAndAdjustROIBoundary
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BFStats25::ValidateAndAdjustROIBoundary(
    ISPInputData*   pInputData,
    AFConfigParams* pBFStatsConfig)
{
    FLOAT                      scaleRatio               = 1.0f;
    INT32                      index                    = 0;
    BOOL                       isInputConfigAdjusted    = FALSE;
    BFStatsROIDimensionParams* pROI                     = NULL;
    INT32                      camifHeight              = 0;
    INT32                      camifWidth               = 0;
    CropInfo*                  pSensorCAMIFCrop         = NULL;
    INT32                      minWidthAdjustedDelta    = 0;
    INT32                      adjustedIndex            = 0;
    BFStatsROIConfigType*      pROIConfig               = &pBFStatsConfig->BFStats.BFStatsROIConfig;
    const INT32                numROIs                  = pROIConfig->numBFStatsROIDimension;
    INT32                      left                     = 0;
    INT32                      top                      = 0;
    INT32                      width                    = 0;
    INT32                      height                   = 0;
    INT32                      orgLeft                  = 0;
    INT32                      orgTop                   = 0;

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

    BFStatsROIDimensionParams* pROIArray = &pBFStatsConfig->BFStats.BFStatsROIConfig.BFStatsROIDimension[0];

    // Validate the ROI, if does not meet the characterstics of the BF stats modify it
    for (index = 0; index < static_cast<INT32>(pBFStatsConfig->BFStats.BFStatsROIConfig.numBFStatsROIDimension); index++)
    {
        pROI = &pROIArray[index];

        // Set the region to valid by default
        pROI->isValid = TRUE;

        left    = pROI->ROI.left;
        top     = pROI->ROI.top;
        width   = pROI->ROI.width;
        height  = pROI->ROI.height;

        if ((width  <  (MinWidth -1))     ||
            (height <  (MinHeight - 1))   ||
            (left   <   0)                ||
            (top    <   0)                ||
            (width  > camifWidth)         ||
            (height > camifHeight)        ||
            ((left + width) > camifWidth) ||
            ((top + height) > camifHeight))
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid ROI at index %d, dimension %d * %d, offset (%d, %d)",
                          index, width, height, left, top);
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
        if (MinXOffset > (scaleRatio * left))
        {
            orgLeft = left;
            left    = static_cast<INT32>(((MinXOffset + 1) / scaleRatio) - 1);
            left    = Utils::EvenCeilingINT32(left);
            width   = width - (left - orgLeft);

            if (MinWidth > (scaleRatio * (width + 1)))
            {
                // not valid ROI
                pROI->isValid = FALSE;
            }

            pROI->ROI.left  = left;
            pROI->ROI.width = width;
            isInputConfigAdjusted   = TRUE;
        }
        else if (Utils::IsINT32Odd(left))
        {
            pROI->ROI.left          = Utils::EvenFloorINT32(left);
            isInputConfigAdjusted   = TRUE;
        }

        if (MinYOffset > (scaleRatio * top))
        {
            orgTop   = top;
            top     = static_cast<INT32>(((MinYOffset + 1) / scaleRatio) - 1);
            top     = Utils::EvenCeilingINT32(top);
            height  = height - (top - orgTop);


            if (height < MinHeight)
            {
                // not valid ROI
                pROI->isValid = FALSE;
            }
            pROI->ROI.top    = top;
            pROI->ROI.height = height;

            isInputConfigAdjusted   = TRUE;
        }
        else if (Utils::IsINT32Odd(top))
        {
            top           = Utils::EvenFloorINT32(top);
            isInputConfigAdjusted   = TRUE;
        }

        // Width and height should be even, Programed value should be odd
        if (Utils::IsINT32Even(width))
        {
            width         = Utils::OddFloorINT32(width);
            isInputConfigAdjusted   = TRUE;
        }

        if (Utils::IsINT32Even(height))
        {
            height        = Utils::OddFloorINT32(height);
            isInputConfigAdjusted   = TRUE;
        }

        const INT32 currentROIScaledWidth = (scaleRatio * (width + 1));
        if (MinWidth > currentROIScaledWidth)
        {
            const INT32 upscaledMinWidth = static_cast<INT32>(((MinWidth + 1) / scaleRatio) - 1);

            // Since "pROI->ROI.width" has been updated as a programmed value, the width should be calculated
            // as "pROI->ROI.width + 1"
            minWidthAdjustedDelta   = upscaledMinWidth - (width + 1);
            adjustedIndex           = index;

            // Need to check the next ROI is not affected by increase of the current ROI width to the min ROI width.
            // Assume that the ROIs are given as *rectangular grid pattern*. Therefore, any increase of width will impact
            // the next ROI width which will be ROI at (index+1). The following check will ensure the next ROI to have
            // enough width so that the adjustment can be safely applied.
            if (index + 1 < numROIs)
            {
                const RectangleCoordinate* pCurrentROI = &pROI->ROI;
                const RectangleCoordinate* pNextROI    = &pROIArray[index + 1].ROI;

                INT32 nextLeft      = pNextROI->left;
                INT32 nextTop       = pNextROI->top;
                INT32 nextWidth     = pNextROI->width;
                INT32 nextHeight    = pNextROI->height;

                // 1. Ensure while adjusting the current ROI, we don't overshoot the next ROI start
                // 2. After the adjustment, ensure the next ROI still has enough width to comply with
                //    the minimum requirement specification.
                if ((nextLeft <= left + upscaledMinWidth) &&
                    (nextTop  >= top) &&
                    (nextTop  <= top + height) &&
                    (nextWidth - minWidthAdjustedDelta < upscaledMinWidth))
                {
                    CAMX_LOG_WARN(CamxLogGroupISP,
                                   "Unable to adjust both the current and the next ROIs having too small width. "
                                   "Current ROI index=%u (l=%u, t=%u, w=%u, h=%u) and "
                                   "Next ROI index=%u (l=%u, t=%u, w=%u, h=%u)",
                                   index, pCurrentROI->left, pCurrentROI->top, pCurrentROI->width, pCurrentROI->height,
                                   index + 1, pNextROI->left, pNextROI->top, pNextROI->width, pNextROI->height);

                    pROI->isValid = FALSE;
                    continue;
                }
            }

            // Ensure the modified ROI width follows the programmed value specification (i.e. intended width - 1)
            width = upscaledMinWidth - 1;
            width = Utils::OddCeilingINT32(width);

            isInputConfigAdjusted   = TRUE;
        }
        else if (minWidthAdjustedDelta > 0)
        {

            INT32 adjustedLeft      = pROIArray[adjustedIndex].ROI.left;
            INT32 adjustedWidth     = pROIArray[adjustedIndex].ROI.width;

            if (left <= (adjustedLeft + adjustedWidth + 1))
            {
                left += minWidthAdjustedDelta;
                left = Utils::EvenFloorINT32(left);

                width -= minWidthAdjustedDelta;
                width = Utils::OddCeilingINT32(width);

                CAMX_LOG_ERROR(CamxLogGroupISP, "Adjusted ROI = (%d, %d, %d, %d)",
                              left,
                              top,
                              width,
                              height);

                isInputConfigAdjusted = TRUE;
            }

            // Reset minWidthAdjustedDelta to 0
            minWidthAdjustedDelta = 0;
        }

        if (width > (camifWidth >> 1))
        {
            width         = camifWidth >> 1;
            width         = Utils::OddFloorINT32(width);
            isInputConfigAdjusted   = TRUE;
        }

        // Validate minimum margin on right side
        if (static_cast<INT32>(((left + width) * scaleRatio)) >
            ((camifWidth * scaleRatio) - HorizontalFIRMargin))
        {
            pROI->isValid = FALSE;
            continue;
        }

        // Validate min height
        if (height < MinHeight)
        {
            height        = MinHeight - 1;
            isInputConfigAdjusted   = TRUE;
        }

        // Validate minimum margin at bottom
        if ((top + height) > (camifHeight - VerticalFIRMargin))
        {
            pROI->isValid = FALSE;
            continue;
        }

        // Validate max h
        if (height > (camifHeight >> 1))
        {
            height        = camifHeight >> 1;
            height        = Utils::OddFloorINT32(height);
            isInputConfigAdjusted   = TRUE;
        }

        pROI->ROI.left      = left;
        pROI->ROI.top       = top;
        pROI->ROI.width     = width;
        pROI->ROI.height    = height;
        if ((left < 0) || (top < 0) || (width <= 0) || (height <= 0))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "Invalid ROI[%d, %d, %d, %d", left, top, width, height);
            pROI->isValid = FALSE;
        }

    }

    return isInputConfigAdjusted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::SortRegionsStartPixelOrderBased
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::SortRegionsStartPixelOrderBased()
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
// BFStats25::CheckAndAdjustStartPixelROILimitation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::CheckAndAdjustStartPixelROILimitation()
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

            const UINT32 deltaROILeft = (pROI->ROI.left > pNextROI->ROI.left) ?
                (pROI->ROI.left - pNextROI->ROI.left) : (pNextROI->ROI.left - pROI->ROI.left);

            if (((pROI->ROI.left == pNextROI->ROI.left)                    &&
                 (pROI->ROI.top  == pNextROI->ROI.top))                    ||
                ((TRUE           == RegionsOverlapping(*pROI, *pNextROI))  &&
                (deltaROILeft    <  MinStartPixelOverlap)))
            {
                if ((pNextROI->ROI.width - MinStartPixelOverlap) >= MinWidth)
                {
                    // Move ROI to right with reduced width
                    pNextROI->ROI.left      += MinStartPixelOverlap;
                    m_isInputConfigAdjusted = TRUE;
                }
                else if ((pNextROI->ROI.left + pNextROI->ROI.width + MinStartPixelOverlap) <=
                         (m_CAMIFWidth - HorizontalFIRMargin))
                {
                    // Move ROI to right
                    pNextROI->ROI.left      += MinStartPixelOverlap;
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
// BFStats25::RegionsOverlapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BFStats25::RegionsOverlapping(
    BFStatsROIDimensionParams currentROI,
    BFStatsROIDimensionParams nextROI)
{
    BOOL result = FALSE;

    // Check if the addition overflow
    UINT32 leftROIPlusWidth     = 0;
    UINT32 topROIPlusHeight     = 0;
    UINT32 nextLeftROIPlusWidth = 0;

    if (currentROI.ROI.left < UINT_MAX - currentROI.ROI.width)
    {
        leftROIPlusWidth = currentROI.ROI.left + currentROI.ROI.width;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "currentROI left will cause overflow, left: %d, width: %d",
                       currentROI.ROI.left, currentROI.ROI.width);
        leftROIPlusWidth = UINT_MAX;
    }

    if (currentROI.ROI.top < UINT_MAX - currentROI.ROI.height)
    {
        topROIPlusHeight = currentROI.ROI.top + currentROI.ROI.height;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "currentROI top will cause overflow, top: %d, height: %d",
                       currentROI.ROI.top, currentROI.ROI.height);
        topROIPlusHeight = UINT_MAX;
    }

    if (nextROI.ROI.left < UINT_MAX - nextROI.ROI.width)
    {
        nextLeftROIPlusWidth = nextROI.ROI.left + nextROI.ROI.width;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "nextROI left will cause overflow, left: %d, width: %d",
                       nextROI.ROI.left, nextROI.ROI.width);
        nextLeftROIPlusWidth = UINT_MAX;
    }

    // Check if the current and next ROI are overlapping
    if ((nextROI.ROI.left < (leftROIPlusWidth)) &&
        (nextROI.ROI.top  < (topROIPlusHeight)))
    {
        if ((nextROI.ROI.left >= currentROI.ROI.left))
        {
            result = TRUE;
        }
        else if ((nextLeftROIPlusWidth) >= currentROI.ROI.left)
        {
            result = TRUE;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::CheckMaxHorizontalLimitationPerRow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BFStats25::CheckMaxHorizontalLimitationPerRow()
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
            if (m_pHorizontalRegionCount[pCurrentROI->ROI.top] > MaxHorizontalRegions)
            {
                // Can't adjust ROI's, return error and do not consume entire ROI config
                result = CamxResultEInvalidState;
                CAMX_ASSERT_ALWAYS_MESSAGE("No of horizontal regions exceeded limit %d for row %d",
                                           MaxHorizontalRegions, index);
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
// BFStats25::CheckMaxVerticalSpacingAndDisjoint
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BFStats25::CheckMaxVerticalSpacingAndDisjoint()
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
        if ((index + MaxHorizontalRegions) >= pROIConfig->numBFStatsROIDimension)
        {
            break;
        }

        pCurrentROI   = &m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[index];
        pReferenceROI = &m_BFStatsConfig.BFStats.BFStatsROIConfig.BFStatsROIDimension[index + MaxHorizontalRegions];

        if ((FALSE == pCurrentROI->isValid) || (FALSE == pReferenceROI->isValid))
        {
            continue;
        }

        // Check whether current index and (index + maxHorizontalRegions) are spaced apart by maxVerticalSpacing lines

        if (((pReferenceROI->ROI.top + pReferenceROI->ROI.height) -
            (pCurrentROI->ROI.top + pCurrentROI->ROI.height)) < MaxVerticalSpacing)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP,
                           "vertical spacing constrain failed at %d [%d,%d]and %d [%d, %d]",
                           index,
                           pCurrentROI->ROI.top,
                           pCurrentROI->ROI.height,
                           (index + MaxHorizontalRegions),
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
                           (index + MaxHorizontalRegions),
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
// BFStats25::CheckMaxRegionOverlap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BFStats25::CheckMaxRegionOverlap()
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
                CAMX_LOG_ERROR(CamxLogGroupISP, "Regions are overlapping: "
                               "current ROI=(%u, %u, %u, %u), next ROI=(%u, %u, %u, %u)",
                               pCurrentROI->ROI.left, pCurrentROI->ROI.top, pCurrentROI->ROI.width, pCurrentROI->ROI.height,
                               pNextROI->ROI.left, pNextROI->ROI.top, pNextROI->ROI.width, pNextROI->ROI.height);
                pCurrentROI->isValid = FALSE;
            }

            // Check whether overlapCount is within limits
            if (overlapCount > MaxOverlappingRegions)
            {
                CAMX_LOG_ERROR(CamxLogGroupISP, "failed: overlap_count exceeded max %d", MaxOverlappingRegions);
                // Cannot adjust ROI's, return error and do not consume enter ROI config
                result = CamxResultEInvalidArg;
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::CalculateGammaDownscaleFactor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::CalculateGammaDownscaleFactor()
{
    BOOL  LSBAlignment         = FALSE;
    BOOL  forceYChannel        = FALSE;
    BOOL  overrideYCoefficient = FALSE;
    FLOAT ratio                = 1.0f;

    m_gammaDownscaleFactor = 1;

    if ((TRUE == BFGammaUseYChannel) || (TRUE == LSBAlignment))
    {
        if (BFChannelSelectG == m_BFStatsConfig.BFStats.BFInputConfig.BFChannelSel)
        {
            forceYChannel          = TRUE;
            m_gammaDownscaleFactor = BFGammaDownscaleFactorTwo;
        }
        else if (BFChannelSelectY == m_BFStatsConfig.BFStats.BFInputConfig.BFChannelSel)
        {
            if (TRUE == YCoefficientSumCheck())
            {
                overrideYCoefficient   = TRUE;
                m_gammaDownscaleFactor = BFGammaDownscaleFactorTwo;
            }
            else
            {
                overrideYCoefficient   = FALSE;
                m_gammaDownscaleFactor = BFGammaDownscaleFactorFour;
            }
        }
        // In HDR mode only 10 bits of LSB is valid, Gamma lookup is indexed by MSB's of 5 bits,
        // So in zzHDR mode only 1 bit is valid. To convert 5 bit index to 2 bit index dividing by 8/16
        if (TRUE == LSBAlignment)
        {
            m_gammaDownscaleFactor *= BFGammaDownscaleFactorFour;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "forceYChannel %d, overrideYCoefficient %d",
                         forceYChannel,
                         overrideYCoefficient);
    }

    m_lumaConversionConfigInput.overrideYCoefficient    = overrideYCoefficient;
    m_lumaConversionConfigInput.forceYChannel           = forceYChannel;
    m_lumaConversionConfigInput.ratio                   = ratio;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BFStats25::YCoefficientSumCheck
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BFStats25::YCoefficientSumCheck() const
{
    FLOAT sum    = 0.0f;
    BOOL  result = FALSE;

    sum = m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[0] +
          m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[1] +
          m_BFStatsConfig.BFStats.BFInputConfig.YAConfig[2];

    if (sum <= YAConfigMax)
    {
        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::DownscalerConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::DownscalerConfig(
    const ISPInputData*   pInputData,
    AFConfigParams* pBFStatsConfig)
{
    UINT32 interpolationResolution = 0;
    UINT32 inputWidth              = 0;
    UINT32 inputHeight             = 0;
    CAMX_ASSERT(NULL != pBFStatsConfig);

    inputWidth  = pInputData->pStripeConfig->CAMIFCrop.lastPixel - pInputData->pStripeConfig->CAMIFCrop.firstPixel + 1;
    inputHeight = pInputData->pStripeConfig->CAMIFCrop.lastLine - pInputData->pStripeConfig->CAMIFCrop.firstLine + 1;

    if ((FALSE == pBFStatsConfig->BFStats.BFScaleConfig.BFScaleEnable) ||
            (0 == pBFStatsConfig->BFStats.BFScaleConfig.scaleN)        ||
            (0 == pBFStatsConfig->BFStats.BFScaleConfig.scaleM))
    {
        m_downscalerConfigInput.enableScaler = FALSE;
    }
    else
    {
        m_downscalerConfigInput.enableScaler = TRUE;

        const UINT32 horizontalImageWidth = Utils::MinUINT32(ScaleMaxHorizontalConfig, inputWidth);

        m_downscalerConfigInput.scalerImageSizeIn  = horizontalImageWidth - 1;
        m_downscalerConfigInput.scalerImageSizeOut =
            ((horizontalImageWidth * (pBFStatsConfig->BFStats.BFScaleConfig.scaleM)) /
             (pBFStatsConfig->BFStats.BFScaleConfig.scaleN)) - 1;

        m_downscalerConfigInput.scalerRoundingPattern = 0;

        if (TRUE == pInputData->pStripeConfig->overwriteStripes)
        {
            m_downscalerConfigInput.scalerPhaseInterpolationResolution =
                pBFStatsConfig->BFStats.BFScaleConfig.interpolationResolution;

            m_downscalerConfigInput.scalerPhaseMultiplicationFactor =
                pBFStatsConfig->BFStats.BFScaleConfig.phaseStep;

            m_downscalerConfigInput.scalerPhaseInitialValue =
                pBFStatsConfig->BFStats.BFScaleConfig.phaseInit;

            m_downscalerConfigInput.scalerMNInitialValue =
                pBFStatsConfig->BFStats.BFScaleConfig.mnInit;

            m_downscalerConfigInput.scalerSkipCount =
                pBFStatsConfig->BFStats.BFScaleConfig.pixelOffset;

            m_downscalerConfigInput.scalerInputWidth =
                m_downscalerConfigInput.scalerImageSizeIn;
        }
        else
        {
            interpolationResolution = DownscalerCalculateInterpolationResolution(pBFStatsConfig);

            m_downscalerConfigInput.scalerPhaseInterpolationResolution =
                DownscalerCalculateInterpolationResolution(pBFStatsConfig);

            m_downscalerConfigInput.scalerPhaseMultiplicationFactor =
                (m_CAMIFWidth << (interpolationResolution + PhaseAdder)) /
                (m_downscalerConfigInput.scalerImageSizeOut + 1);

            m_downscalerConfigInput.scalerPhaseInitialValue = 0;
            m_downscalerConfigInput.scalerMNInitialValue = 0;
            m_downscalerConfigInput.scalerSkipCount = 0;
            m_downscalerConfigInput.scalerInputWidth =
                m_downscalerConfigInput.scalerImageSizeIn;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::DownscalerCalculateInterpolationResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 BFStats25::DownscalerCalculateInterpolationResolution(
    const AFConfigParams* pBFStatsConfig
    ) const
{
    UINT32 ratio                   = 0;
    UINT32 interpolationResolution = InterpolationResolution3;

    CAMX_ASSERT(NULL != pBFStatsConfig);

    if (0 != pBFStatsConfig->BFStats.BFScaleConfig.scaleM)
    {
        ratio = (pBFStatsConfig->BFStats.BFScaleConfig.scaleN / pBFStatsConfig->BFStats.BFScaleConfig.scaleM);
        if (ratio >= ScaleRatio16)
        {
            interpolationResolution = InterpolationResolution0;
        }
        else if (ratio >= ScaleRatio8)
        {
            interpolationResolution = InterpolationResolution1;
        }
        else if (ratio >= ScaleRatio4)
        {
            interpolationResolution = InterpolationResolution2;
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
// BFStats25::UpdateROIDMITable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::UpdateROIDMITable(
    BFStatsROIConfigType*       pHWROIConfig,
    BFStats25ROIIndexLUTType*   pROIDMIConfig,
    const UINT32                sizeOfDMIConfig,
    const UINT32                stripeId)
{
    UINT32 index = 0;

    CAMX_ASSERT(NULL != pHWROIConfig);
    CAMX_ASSERT(NULL != pROIDMIConfig);

    Utils::Memset(pROIDMIConfig, 0, sizeOfDMIConfig);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "Num ROIs = %d", pHWROIConfig->numBFStatsROIDimension);

    for (index = 0; index < pHWROIConfig->numBFStatsROIDimension; index++)
    {
        const BFStatsROIDimensionParams*    pCurrentROIConfig   = &(pHWROIConfig->BFStatsROIDimension[index]);
        const BOOL                          isEndOfBuffer       = (pHWROIConfig->numBFStatsROIDimension - 1 == index) ?
                                                                  TRUE : FALSE;
        pROIDMIConfig[index].height      = pCurrentROIConfig->ROI.height;
        pROIDMIConfig[index].width       = pCurrentROIConfig->ROI.width;
        pROIDMIConfig[index].top         = pCurrentROIConfig->ROI.top;
        pROIDMIConfig[index].left        = pCurrentROIConfig->ROI.left;
        pROIDMIConfig[index].regionID    = pCurrentROIConfig->regionNum;
        pROIDMIConfig[index].outputID    = pCurrentROIConfig->outputID;
        pROIDMIConfig[index].needMerged  = pCurrentROIConfig->needMerge;
        pROIDMIConfig[index].endOfBuffer = isEndOfBuffer;
        pROIDMIConfig[index].regionType  = pHWROIConfig->BFStatsROIDimension[index].region;

        CAMX_LOG_VERBOSE(CamxLogGroupISP,
                         "stripeId=%u, index=%d, rect=(%u, %u, %u, %u), rid=%d, oid=%d, merge=%d, eob=%d, type=%d",
                         stripeId,
                         index,
                         pROIDMIConfig[index].left,
                         pROIDMIConfig[index].top,
                         pROIDMIConfig[index].width,
                         pROIDMIConfig[index].height,
                         pROIDMIConfig[index].regionID,
                         pROIDMIConfig[index].outputID,
                         pROIDMIConfig[index].needMerged,
                         pROIDMIConfig[index].endOfBuffer,
                         pROIDMIConfig[index].regionType);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::UpdateROIDMITableForStripingInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::UpdateROIDMITableForStripingInput(
    BFStatsROIConfigType*       pHWROIConfig,
    UINT64                      ppStripingLibROIDMIConfig[][2],
    UINT32                      sizeOfDMIConfig)
{
    UINT32 index = 0;

    CAMX_ASSERT(NULL != pHWROIConfig);
    CAMX_ASSERT(NULL != ppStripingLibROIDMIConfig);

    // Array decayed double-pointer. ppStripingLibROIDMIConfig and ppStripingLibROIDMIConfig[0]
    Utils::Memset(ppStripingLibROIDMIConfig, 0, sizeOfDMIConfig);

    for (index = 0; index < pHWROIConfig->numBFStatsROIDimension; index++)
    {
        const BFStatsROIDimensionParams*    pCurrentROIConfig   = &(pHWROIConfig->BFStatsROIDimension[index]);
        const BOOL                          isEndOfBuffer       = (pHWROIConfig->numBFStatsROIDimension - 1 == index) ?
                                                                  TRUE : FALSE;

        UINT64* pROILower64bit = &(ppStripingLibROIDMIConfig[index][0]);
        *pROILower64bit =
            (static_cast<UINT64>(pCurrentROIConfig->regionNum)   << BF25ROIRegionIDShift) |
            ((static_cast<UINT64>(pCurrentROIConfig->ROI.left)   & BF25ROILeftBits)   << BF25ROILeftShift)  |
            ((static_cast<UINT64>(pCurrentROIConfig->ROI.top)    & BF25ROITopBits)    << BF25ROITopShift)   |
            ((static_cast<UINT64>(pCurrentROIConfig->ROI.width)  & BF25ROIWidthBits)  << BF25ROIWidthShift) |
            ((static_cast<UINT64>(pCurrentROIConfig->ROI.height) & BF25ROIHeightBits) << BF25ROIHeightShift);

        /// Filling some of following information should be done by Striping Library
        /// That is, "merge bit" and "outputId" may needs to be filled by the striping library.
        UINT64* pROIUpper64bit = &(ppStripingLibROIDMIConfig[index][1]);
        *pROIUpper64bit =
            (static_cast<UINT64>(isEndOfBuffer)                  << (BF25ROIEndOfBufferShift - 64))  |
            (static_cast<UINT64>(pCurrentROIConfig->region)      << (BF25ROIRegionTypeShift  - 64));
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::UpdateHWROI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::UpdateHWROI(
    BFStatsROIConfigType*   pInputROIConfig,
    BFStatsROIConfigType*   pHWROIConfig,
    const UINT32            stripeId,
    BOOL                    overwriteStripes)
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
            BFStatsROIDimensionParams* pROIConfig = &(pHWROIConfig->BFStatsROIDimension[numberOfRegions]);

            pROIConfig->ROI.left   = pROI->ROI.left;
            pROIConfig->ROI.top    = pROI->ROI.top;
            pROIConfig->ROI.width  = pROI->ROI.width;
            pROIConfig->ROI.height = pROI->ROI.height;
            pROIConfig->region     = pROI->region;
            pROIConfig->regionNum  = pROI->regionNum;
            pROIConfig->isValid    = TRUE;
            pROIConfig->outputID   = (TRUE == overwriteStripes) ? pROI->outputID : pROI->regionNum;
            pROIConfig->needMerge  = pROI->needMerge;

            CAMX_LOG_VERBOSE(CamxLogGroupISP,
                             "stripeId = %u, index = %d, ROI=(%d %d %d %d), regiontype=%d regionID=%d outputID=%d valid=%d",
                             stripeId,
                             index,
                             pROIConfig->ROI.left,
                             pROIConfig->ROI.top,
                             pROIConfig->ROI.width,
                             pROIConfig->ROI.height,
                             pROIConfig->region,
                             pROIConfig->regionNum,
                             pROIConfig->outputID,
                             pROIConfig->isValid);

            numberOfRegions++;
        }
    }
    pHWROIConfig->numBFStatsROIDimension = numberOfRegions;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::SortRegionsEndPixelOrderBased
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::SortRegionsEndPixelOrderBased()
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
                UINT32 topROIPlusHeight = 0;
                if (pROI->ROI.top < UINT_MAX - pROI->ROI.height)
                {
                    topROIPlusHeight = pROI->ROI.top + pROI->ROI.height;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "ROI top will cause overflow, top: %d, height: %d",
                                   pROI->ROI.top, pROI->ROI.height);
                    topROIPlusHeight = UINT_MAX;
                }

                // Calcuate last pixel in single dimension based on raster scan order
                currentEnd = ((topROIPlusHeight) * m_CAMIFWidth) + pROI->ROI.left + pROI->ROI.width;

                // Secondary loop is required as the next index ROI could be invalid
                for (nextIndex = index + 1; nextIndex < m_endOrderROI.numBFStatsROIDimension; nextIndex++)
                {
                    pNextROI = &m_endOrderROI.BFStatsROIDimension[nextIndex];

                    UINT32 nextTopROIPlusHeight = 0;
                    if (pNextROI->ROI.top < UINT_MAX - pNextROI->ROI.height)
                    {
                        nextTopROIPlusHeight = pNextROI->ROI.top + pNextROI->ROI.height;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupISP, "nextROI top will cause overflow, top: %d, height: %d",
                                       pNextROI->ROI.top, pNextROI->ROI.height);
                        nextTopROIPlusHeight = UINT_MAX;
                    }

                    if (TRUE == pNextROI->isValid)
                    {
                        nextEnd = ((nextTopROIPlusHeight) * m_CAMIFWidth) +
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
// BFStats25::CheckAndAdjustEndPixelROILimitation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::CheckAndAdjustEndPixelROILimitation()
{
    BFStatsROIDimensionParams* pROI       = NULL;
    BFStatsROIDimensionParams* pNextROI   = NULL;
    UINT32                     index      = 0;
    UINT32                     nextIndex  = 0;
    UINT32                     currentEnd = 0;
    UINT32                     nextEnd    = 0;
    INT32                      diff;

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
                    diff = static_cast<INT32>(pROI->ROI.left + pROI->ROI.width) -
                           static_cast<INT32>(pNextROI->ROI.left + pNextROI->ROI.width);

                    // Checking whether both ROI ends on same pixel or ending line apart by min
                    if ((currentEnd == nextEnd) ||
                        (Utils::AbsoluteINT32(diff) < MinStartPixelOverlap))
                    {
                        CAMX_LOG_WARN(CamxLogGroupISP, "two ROI ending on same pixel [%d, %d, %d, %d]",
                            pROI->ROI.left, pROI->ROI.top, pROI->ROI.width, pROI->ROI.height);

                        if ((pNextROI->ROI.left + pNextROI->ROI.width + MinStartPixelOverlap) <=
                            (m_CAMIFWidth - HorizontalFIRMargin))
                        {
                            // Move ROI to right
                            pNextROI->ROI.left += MinStartPixelOverlap;
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
// BFStats25::ConfigureGamma
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BFStats25::ConfigureGamma(
    const ISPInputData* pInputData)
{
    UINT32 indexDownScaled              = 0;
    UINT32 indexRemaining               = 0;
    UINT32 indexAll                     = 0;
    UINT32 tempGammaLUT[BFGammaEntries] = {0};
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
            IQSettingUtils::ClampFLOAT(scaleRatio, BFGammaScaleRatioMinValue, BFGammaScaleRatioMaxValue));
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
                                   BFGammaMaxValue);
        }

        // Continue from (m_BFConfig.BFStats.BFGammaLUTConfig.numGammaLUT / m_gammaDownscaleFactor) to end of the LUT
        for (indexRemaining = indexDownScaled;
             indexRemaining < m_BFStatsConfig.BFStats.BFGammaLUTConfig.numGammaLUT;
             indexRemaining++)
        {
            // BAF gamma values are 14bit unsigned
            tempGammaLUT[indexRemaining] = BFGammaMaxValue;
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
                m_gammaLUT[i] = Utils::ClampUINT32(m_gammaLUT[i], 0, BFGammaMaxValue);
            }
            Utils::Memcpy(tempGammaLUT, m_gammaLUT, sizeof(m_gammaLUT));
        }

        for (indexAll = 0; indexAll < (m_BFStatsConfig.BFStats.BFGammaLUTConfig.numGammaLUT - 1); indexAll++)
        {
            m_gammaLUT[indexAll] = GammaGetHighLowBits(&tempGammaLUT[0], indexAll);
        }

        m_gammaLUT[indexAll] = GammaGetLastValue(tempGammaLUT);

        m_enableGammaLUT = TRUE;
    }
    else
    {
        m_enableGammaLUT = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::GammaGetHighLowBits
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 BFStats25::GammaGetHighLowBits(
    UINT32* pGamma,
    UINT32  index
    ) const
{
    UINT32 hwLUTEntry = 0;
    INT16  deltaLUT   = 0;

    if ((NULL != pGamma) && (index < BFGammaEntries))
    {
        deltaLUT = static_cast<INT16>(Utils::ClampINT32((pGamma[(index + 1)] - pGamma[index]),
                                                        BFGammaDeltaMinValue,
                                                        BFGammaDeltaMaxValue));
        hwLUTEntry = static_cast<UINT32>((deltaLUT << BFGammaDMIDeltaShift) + pGamma[index]);
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid input data");
    }
    return hwLUTEntry;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BFStats25::GammaGetLastValue
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 BFStats25::GammaGetLastValue(
    UINT32* pGamma
    ) const
{
    UINT32 hwLUTEntry = 0;
    INT16  deltaLUT   = 0;

    // consider the next entry of the last entry 2^12- 8084.
    //                                           2^14- 8994.
    // value suggested by system team

    UINT32 nextEntryFinal = 2 << BFGammaDMIDeltaShift;

    // this is effectively  table[255] - table[254] this part is the delta
    // use 256 as next entry of last entry
    deltaLUT = static_cast<INT16>(Utils::ClampINT32((nextEntryFinal - pGamma[(BFGammaEntries - 1)]),
                                                    BFGammaDeltaMinValue,
                                                    BFGammaDeltaMaxValue));

    // scale the delta
    // form the value:  upper byte is delta, lower byte is the entry itself
    hwLUTEntry = static_cast<UINT32>((deltaLUT << BFGammaDMIDeltaShift) + pGamma[(BFGammaEntries - 1)]);

    return hwLUTEntry;
}

CAMX_NAMESPACE_END
