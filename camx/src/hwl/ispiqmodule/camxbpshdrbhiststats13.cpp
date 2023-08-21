////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpshdrbhiststats13.cpp
/// @brief BPS HDRBHIST Stats14 Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "bps_data.h"
#include "camxbpshdrbhiststats13.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDRBHist13Stats::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDRBHist13Stats::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;
    if (NULL != pCreateData)
    {
        pCreateData->pModule = CAMX_NEW BPSHDRBHist13Stats;
        if (NULL == pCreateData->pModule)
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
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
// BPSHDRBHist13Stats::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSHDRBHist13Stats::CheckDependenceChange(
    ISPInputData* pInputData)
{
    BOOL            result          = FALSE;
    HDRBHistConfig* pHDRBHistConfig = &pInputData->pAECStatsUpdateData->statsConfig.HDRBHistConfig;

    if ((m_HDRBHistConfig.ROI.left          != pHDRBHistConfig->ROI.left)           ||
        (m_HDRBHistConfig.ROI.top           != pHDRBHistConfig->ROI.top)            ||
        (m_HDRBHistConfig.ROI.width         != pHDRBHistConfig->ROI.width)          ||
        (m_HDRBHistConfig.ROI.height        != pHDRBHistConfig->ROI.height)         ||
        (m_HDRBHistConfig.greenChannelInput != pHDRBHistConfig->greenChannelInput)  ||
        (m_HDRBHistConfig.inputFieldSelect  != pHDRBHistConfig->inputFieldSelect))
    {
        m_HDRBHistConfig.ROI                = pHDRBHistConfig->ROI;
        m_HDRBHistConfig.greenChannelInput  = pHDRBHistConfig->greenChannelInput;
        m_HDRBHistConfig.inputFieldSelect   = pHDRBHistConfig->inputFieldSelect;

        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDRBHist13Stats::CalculateRegionConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHDRBHist13Stats::CalculateRegionConfiguration()
{
    RectangleCoordinate newCoordinate;

    CAMX_ASSERT_MESSAGE(0 != m_HDRBHistConfig.ROI.width, "Invalid ROI width");
    CAMX_ASSERT_MESSAGE(0 != m_HDRBHistConfig.ROI.height, "Invalid ROI height");

    CAMX_LOG_VERBOSE(CamxLogGroupBPS, "ROI: W = %d, H = %d", m_HDRBHistConfig.ROI.width, m_HDRBHistConfig.ROI.height);

    m_isAdjusted = FALSE;

    // Set region
    m_moduleSetting.offsetHorizNum  =   m_HDRBHistConfig.ROI.left;
    m_moduleSetting.offsetVertNum   =   m_HDRBHistConfig.ROI.top;

    m_moduleSetting.regionHorizNum  = m_HDRBHistConfig.ROI.width / HDRBHistStats13RegionWidth;
    m_moduleSetting.regionVertNum   = m_HDRBHistConfig.ROI.height / HDRBHistStats13RegionHeight;

    m_moduleSetting.regionHorizNum  = Utils::MaxUINT32(m_moduleSetting.regionHorizNum, HDRBHist13MinHorizontalregions);
    m_moduleSetting.regionVertNum   = Utils::MaxUINT32(m_moduleSetting.regionVertNum, HDRBHist13MinVerticalregions);

    m_moduleSetting.regionHorizNum  = Utils::MinUINT32(m_moduleSetting.regionHorizNum, HDRBHist13MaxHorizontalregions);
    m_moduleSetting.regionVertNum   = Utils::MinUINT32(m_moduleSetting.regionVertNum, HDRBHist13MaxVerticalregions);

    // Verify if region was adjusted
    newCoordinate.left      = m_moduleSetting.offsetHorizNum;
    newCoordinate.top       = m_moduleSetting.offsetVertNum;
    newCoordinate.width     = m_moduleSetting.regionHorizNum  * HDRBHistStats13RegionWidth;
    newCoordinate.height    = m_moduleSetting.regionVertNum  * HDRBHistStats13RegionHeight;

    if ((newCoordinate.left     != m_HDRBHistConfig.ROI.left)    ||
        (newCoordinate.top      != m_HDRBHistConfig.ROI.top)     ||
        (newCoordinate.width    != m_HDRBHistConfig.ROI.width)   ||
        (newCoordinate.height   != m_HDRBHistConfig.ROI.height))
    {
        m_HDRBHistConfig.ROI = newCoordinate;
        m_isAdjusted                = TRUE;
    }

    // Set green channel
    switch (m_HDRBHistConfig.greenChannelInput)
    {
        case HDRBHistSelectGB:
            m_greenChannelSelect = HDRBHistGreenChannelSelectGb;
            break;
        case HDRBHistSelectGR:
            m_greenChannelSelect = HDRBHistGreenChannelSelectGr;
            break;
        default:
            m_greenChannelSelect                = HDRBHistGreenChannelSelectGb;
            m_HDRBHistConfig.greenChannelInput  = HDRBHistSelectGB;
            m_isAdjusted                         = TRUE;
            break;
    }

    // Set input field/mode
    switch (m_HDRBHistConfig.inputFieldSelect)
    {
        case HDRBHistInputAll:
            m_inputFieldSelect = HDRBHistFieldSelectAll;
            break;
        case HDRBHistInputLongExposure:
            m_inputFieldSelect = HDRBHistFieldSelectT1;
            break;
        case HDRBHistInputShortExposure:
            m_inputFieldSelect = HDRBHistFieldSelectT2;
            break;
        default:
            m_inputFieldSelect  = HDRBHistFieldSelectAll;
            m_isAdjusted        = TRUE;
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDRBHist13Stats::ValidateDependenceParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDRBHist13Stats::ValidateDependenceParams(
    ISPInputData* pInputData)
{
    HDRBHistConfig* pHDRBHistConfig     = &pInputData->pAECStatsUpdateData->statsConfig.HDRBHistConfig;
    CamxResult  result           = CamxResultSuccess;

    // Validate ROI from Stats
    if (((pHDRBHistConfig->ROI.left + pHDRBHistConfig->ROI.width) > pInputData->pipelineBPSData.width)  ||
        ((pHDRBHistConfig->ROI.top + pHDRBHistConfig->ROI.height) > pInputData->pipelineBPSData.height))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "ROI %d, %d, %d, %d",
                       pHDRBHistConfig->ROI.left,
                       pHDRBHistConfig->ROI.top,
                       pHDRBHistConfig->ROI.width,
                       pHDRBHistConfig->ROI.height);

        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS();
    }

    /// @todo (CAMX-856) Validate Region skip pattern, after support from stats

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDRBHist13Stats::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSHDRBHist13Stats::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        // Run through the settings only when module is enabled
        if (TRUE == pInputData->pipelineBPSData.pBPSPathEnabled[BPSOutputPortStatsHDRBHist])
        {
            // Check if dependent is valid and been updated compared to last request
            result = ValidateDependenceParams(pInputData);
            if ((CamxResultSuccess == result) &&
                (TRUE == CheckDependenceChange(pInputData)))
            {
                CalculateRegionConfiguration();
            }
        }
        if (CamxResultSuccess == result)
        {
            // Update CAMX metadata with the values for the current frame
            UpdateBPSInternalData(pInputData);
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
// BPSHDRBHist13Stats::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSHDRBHist13Stats::UpdateBPSInternalData(
    ISPInputData* pInputData)
{
    BpsIQSettings*  pBPSIQSettings = reinterpret_cast<BpsIQSettings*>(pInputData->pipelineBPSData.pIQSettings);
    CAMX_ASSERT(NULL != pBPSIQSettings);

    pBPSIQSettings->bayerHistogramParameters.moduleCfg.EN                   =
        pInputData->pipelineBPSData.pBPSPathEnabled[BPSOutputPortStatsHDRBHist];
    pBPSIQSettings->bayerHistogramParameters.moduleCfg.HDR_BHIST_CHAN_SEL   = m_greenChannelSelect;
    pBPSIQSettings->bayerHistogramParameters.moduleCfg.HDR_BHIST_FIELD_SEL  = m_inputFieldSelect;
    // Always tap out before Demosaic
    pBPSIQSettings->bayerHistogramParameters.moduleCfg.HDR_BHIST_SITE_SEL   = 1;

    // Avoid unsigned integer lower than zero
    UINT32 regionHorizNumMinusOne = (m_moduleSetting.regionHorizNum >= HDRBHist13MinHorizontalregions) ?
                                    (m_moduleSetting.regionHorizNum -1) : HDRBHist13MinHorizontalregions - 1;
    UINT32 regionVertNumMinusOne  = (m_moduleSetting.regionVertNum  >= HDRBHist13MinVerticalregions) ?
                                    (m_moduleSetting.regionVertNum  -1) : HDRBHist13MinVerticalregions - 1;

    pBPSIQSettings->bayerHistogramParameters.bihistRgnHNum      = regionHorizNumMinusOne;
    pBPSIQSettings->bayerHistogramParameters.bihistRgnVNum      = regionVertNumMinusOne;
    pBPSIQSettings->bayerHistogramParameters.bihistRoiHOffset   = m_moduleSetting.offsetHorizNum;
    pBPSIQSettings->bayerHistogramParameters.bihistRoiVOffset   = m_moduleSetting.offsetVertNum;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "En                    %d",
                   pBPSIQSettings->bayerHistogramParameters.moduleCfg.EN);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "HDR_BHIST_CHAN_SEL    %d",
                   pBPSIQSettings->bayerHistogramParameters.moduleCfg.HDR_BHIST_CHAN_SEL);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "HDR_BHIST_FIELD_SEL   %d",
                   pBPSIQSettings->bayerHistogramParameters.moduleCfg.HDR_BHIST_FIELD_SEL);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "HDR_BHIST_SITE_SEL    %d",
                   pBPSIQSettings->bayerHistogramParameters.moduleCfg.HDR_BHIST_SITE_SEL);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bihistRgnHNum     %d",
                   pBPSIQSettings->bayerHistogramParameters.bihistRgnHNum);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bihistRgnVNum     %d",
                   pBPSIQSettings->bayerHistogramParameters.bihistRgnVNum);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bihistRoiHOffset  %d",
                   pBPSIQSettings->bayerHistogramParameters.bihistRoiHOffset);
    CAMX_LOG_VERBOSE(CamxLogGroupISP, "bihistRoiVOffset  %d",
                   pBPSIQSettings->bayerHistogramParameters.bihistRoiVOffset);

    // Update AWB BG config to metadata, for parser and 3A to consume
    pInputData->pCalculatedData->metadata.HDRBHistStatsConfig.HDRBHistConfig    = m_HDRBHistConfig;
    pInputData->pCalculatedData->metadata.HDRBHistStatsConfig.isAdjusted        = m_isAdjusted;
    pInputData->pCalculatedData->metadata.HDRBHistStatsConfig.numBins           = NumHDRBHistBins;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDRBHist13Stats::BPSHDRBHist13Stats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHDRBHist13Stats::BPSHDRBHist13Stats()
{
    m_type         = ISPIQModuleType::BPSHDRBHist;
    m_moduleEnable = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSHDRBHist13Stats::~BPSHDRBHist13Stats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSHDRBHist13Stats::~BPSHDRBHist13Stats()
{
}

CAMX_NAMESPACE_END
