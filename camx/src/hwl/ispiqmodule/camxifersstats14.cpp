////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifersstats14.cpp
/// @brief RS Stats v1.4 Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifersstats14.h"
#include "camxifersstats14titan17x.h"
#include "camxifersstats14titan480.h"
#include "camxutils.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RSStats14::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RSStats14::Create(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        RSStats14* pModule = CAMX_NEW RSStats14;
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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create RSStats14 object.");
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
// RSStats14::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RSStats14::Initialize(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFERSStats14Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFERSStats14Titan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength = m_pHWSetting->GetCommandLength();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create RS Setting Class, Titan 0x%x", pCreateData->titanVersion);
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RSStats14::CheckDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RSStats14::CheckDependency(
    ISPInputData* pInputData)
{
    BOOL                result              = TRUE;
    UINT32              inputWidth          = 0;
    UINT32              inputHeight         = 0;
    CropInfo*           pSensorCAMIFCrop    = &pInputData->pStripeConfig->CAMIFCrop;
    AFDStatsRSConfig*   pRSConfig           = &pInputData->pStripeConfig->AFDStatsUpdateData.statsConfig;

    // Calculate the CAMIF size
    inputWidth = pSensorCAMIFCrop->lastPixel - pSensorCAMIFCrop->firstPixel + 1;
    inputHeight = pSensorCAMIFCrop->lastLine - pSensorCAMIFCrop->firstLine + 1;

    // CAMIF input width for YUV sensor would be twice that of sensor width, as each pixel accounts for Y and U/V data
    if (TRUE == pInputData->sensorData.isYUV)
    {
        inputWidth >>= 1;
    }

    // @todo (CAMX-3035) Need to refactor this so as to avoid reconfiguration in DualIFE mode
    // even if no change in configuration for left and right stripes
    // Check if input configuration has changed from the last one
    if ((0 != Utils::Memcmp(&m_RSConfig.RSConfig, pRSConfig, sizeof(AFDStatsRSConfig))) ||
        (m_CAMIFHeight                  != inputHeight) ||
        (m_CAMIFWidth                   != inputWidth)  ||
        (TRUE                           == pInputData->forceTriggerUpdate))
    {
        m_RSConfig.RSConfig       = *pRSConfig;
        m_RSConfig.statsRgnWidth  = inputWidth  / m_RSConfig.RSConfig.statsHNum;
        m_RSConfig.statsRgnHeight = inputHeight / m_RSConfig.RSConfig.statsVNum;
        m_CAMIFWidth        = inputWidth;
        m_CAMIFHeight       = inputHeight;
        result = TRUE;
    }
    else
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RSStats14::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RSStats14::Execute(
    ISPInputData* pInputData)
{
    CamxResult  result          = CamxResultSuccess;
    VOID*       pSettingData    = static_cast<VOID*>(pInputData);

    if (NULL != pInputData)
    {
        m_inputConfigData.pISPInputData = pInputData;

        // Check if dependent is valid and been updated compared to last request
        if (TRUE == CheckDependency(pInputData))
        {
            if ((m_RSConfig.statsRgnWidth == 0) || (m_RSConfig.statsRgnHeight == 0))
            {
                m_RSConfig.statsRgnWidth  = m_calculatedRegnWidth;
                m_RSConfig.statsRgnHeight = m_calculatedRegnHeight;
            }

            if (CamxResultSuccess == AdjustROI())
            {
                // Update the Working Config
                Utils::Memcpy(&m_previousRSConfig, &m_RSConfig, sizeof(ISPRSStatsConfig));
            }
            else
            {
                // Using The Previous Valid Config
                CAMX_LOG_WARN(CamxLogGroupISP, "Using the Previous RS Config for Req ID: %llu",
                    pInputData->frameNum);
                Utils::Memcpy(&m_RSConfig, &m_previousRSConfig, sizeof(ISPRSStatsConfig));
            }

            m_pHWSetting->PackIQRegisterSetting(&m_inputConfigData, NULL);
            // In case of Dual IFE scenario we need to make sure region width and
            // height are made similar for both IFE Regions or else parsed stats
            // would incur inconsistency.
            m_calculatedRegnWidth = m_RSConfig.statsRgnWidth;
            m_calculatedRegnHeight = m_RSConfig.statsRgnHeight;
            result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
        }

        // Always update
        if (CamxResultSuccess == result)
        {
            UpdateIFEInternalData(pInputData);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RSStats14::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RSStats14::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult              result      = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (NULL != pInputData->pStripingInput)
        {
            // Check if dependent is valid and been updated compared to last request
            if (TRUE == CheckDependency(pInputData))
            {
                if (CamxResultSuccess == AdjustROI())
                {
                    // Update the Working Config
                    Utils::Memcpy(&m_previousRSConfig, &m_RSConfig, sizeof(ISPRSStatsConfig));
                }
                else
                {
                    // Using The Previous Valid Config
                    CAMX_LOG_WARN(CamxLogGroupISP, "Using the Previous RS Config for Req ID: %llu",
                        pInputData->frameNum);
                    Utils::Memcpy(&m_RSConfig, &m_previousRSConfig, sizeof(ISPRSStatsConfig));
                }
            }

            pInputData->pStripingInput->stripingInput.RSCSInput.RSEnable       = m_moduleEnable;
            pInputData->pStripingInput->stripingInput.RSCSInput.RSRgnHorNum    = m_RSConfig.RSConfig.statsHNum - 1;
            pInputData->pStripingInput->stripingInput.RSCSInput.RSRgnVerNum    = m_RSConfig.RSConfig.statsVNum - 1;
            pInputData->pStripingInput->stripingInput.RSCSInput.RSRgnWidth     = m_RSConfig.statsRgnWidth - 1;
            pInputData->pStripingInput->stripingInput.RSCSInput.RSRgnHorOffset = m_RSConfig.statsHOffset;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Input pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RSStats14::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID RSStats14::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    // Update RS Stats configuration
    pInputData->pCalculatedData->metadata.RSStats               = m_RSConfig;

    m_pHWSetting->SetupRegisterSetting(&m_inputConfigData);

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "RS: RS En:%d Hnum:%d Vnum:%d RegionH:%d RegionW:%d HOffset:%d VOffset:%d",
        m_moduleEnable,
        m_RSConfig.RSConfig.statsHNum,
        m_RSConfig.RSConfig.statsVNum,
        m_RSConfig.statsRgnHeight,
        m_RSConfig.statsRgnWidth,
        m_RSConfig.statsHOffset,
        m_RSConfig.statsVOffset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RSStats14::RSStats14
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RSStats14::RSStats14()
{
    m_type          = ISPStatsModuleType::IFERS;
    m_moduleEnable  = TRUE;

    m_CAMIFWidth    = 0;
    m_CAMIFHeight   = 0;

    m_inputConfigData.pRSConfig = &m_RSConfig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RSStats14::~RSStats14
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RSStats14::~RSStats14()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RSStats14::AdjustROI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RSStats14::AdjustROI()
{
    CamxResult result               = CamxResultSuccess;
    INT32      regionHorizNum       = 0;
    INT32      regionVertNum        = 0;
    INT32      regionHeight         = 0;
    INT32      regionWidth          = 0;
    INT32      regionHorizOffset    = 0;
    INT32      regionVertOffset     = 0;
    INT32      effectiveFrameWidth  = 0;
    INT32      effectiveFrameHeight = 0;
    UINT32     minROIWidth          = 0;
    UINT32     minROIHeight         = 0;

    // Minimum ROI width - min horizontal regions * minimum region width
    minROIWidth = RSStats14MinHorizRegions * 1;
    // Minimum ROI height - min horizontal regions * minimum region height
    minROIHeight = RSStats14MinVertRegions * RSStats14MinRegionHeight;

    // Configure region offset
    // Horizontal offset - minimum is 0 and maximum is image_width - 1
    regionHorizOffset = m_RSConfig.statsHOffset;
    regionHorizOffset = Utils::ClampINT32(regionHorizOffset, 0, (m_CAMIFWidth - minROIWidth) - 1);

    // Vertical offset - minimum is 0 and maximum is image_height - 1
    regionVertOffset = m_RSConfig.statsVOffset;
    regionVertOffset = Utils::ClampINT32(regionVertOffset, 0, (m_CAMIFHeight - minROIHeight) - 1);
    regionVertOffset = Utils::EvenFloorINT32(regionVertOffset);

    effectiveFrameWidth  = m_CAMIFWidth - regionHorizOffset;
    effectiveFrameHeight = m_CAMIFHeight - regionVertOffset;

    // Setting Horizontal and Vertical region numbers
    regionHorizNum = m_RSConfig.RSConfig.statsHNum;
    if ((0 == regionHorizNum) || (regionHorizNum > static_cast<INT32>(RSStats14MaxHorizRegions)))
    {
        regionHorizNum = RSStats14MaxHorizRegions;
    }

    regionVertNum = m_RSConfig.RSConfig.statsVNum;
    if ((0 == regionVertNum) || (regionVertNum > static_cast<INT32>(RSStats14MaxVertRegions)))
    {
        regionVertNum = RSStats14MaxVertRegions;
    }

    // Region width - minimum is 1 pixel and maximum is image_width pixels
    regionWidth = m_RSConfig.statsRgnWidth;
    if (0 == regionWidth)
    {
        regionWidth = effectiveFrameWidth / Utils::MaxINT32(regionHorizNum, 1);
    }

    // Region height - Number of lines for a given region - minimum is 1 and maximum is 16
    regionHeight = m_RSConfig.statsRgnHeight;
    if (0 == regionHeight)
    {
        regionHeight  = (effectiveFrameHeight + regionVertNum - 1) / Utils::MaxINT32(regionVertNum, 1);
        regionHeight  = Utils::EvenFloorINT32(regionHeight);
        // Adjust horizontal regions based on new regionWidth again
        regionVertNum = effectiveFrameHeight / Utils::MaxINT32(regionHeight, 1);
    }

    // Region height and offset must be even
    regionHeight        = Utils::EvenFloorINT32(regionHeight);
    regionVertOffset    = Utils::EvenFloorINT32(regionVertOffset);

    // Also clamp parameters to Min and Max of Hardware limitations
    regionWidth    = Utils::ClampINT32(regionWidth, 1, effectiveFrameWidth);
    regionHeight   = Utils::ClampINT32(regionHeight, RSStats14MinRegionHeight, RSStats14MaxRegionHeight);
    regionHorizNum = Utils::ClampINT32(regionHorizNum, RSStats14MinHorizRegions, RSStats14MaxHorizRegions);
    regionVertNum  = Utils::ClampINT32(regionVertNum, RSStats14MinVertRegions, RSStats14MaxVertRegions);

    // Sanity check to ensure configuration doesn't exceed camif boundaries
    if ((regionWidth * regionHorizNum) > effectiveFrameWidth)
    {
        // Adjust only HNum dont adjust regionWidth. If its other way then in case of
        // Dual IFE Scenario Left and Right stripe would endup in having different regionWidth
        regionHorizNum = effectiveFrameWidth / regionWidth;
    }

    if ((regionHeight * regionVertNum) > effectiveFrameHeight)
    {
        // Adjust only VNum dont adjust regionHeighth. If its other way then in case of
        // Dual IFE Scenario Left and Right stripe would endup in having different regionHeight
        regionVertNum = effectiveFrameHeight / regionHeight;
    }

    // Calculate shift bits - used to yield 16 bits column sum by right shifting the accumulated column sum
    m_inputConfigData.shiftBits =
        static_cast<UINT16>(Utils::CalculateShiftBits(regionWidth * regionHeight, RSInputDepth, RSOutputDepth));

    // If RS configuration has been adjusted and is different from input, update is adjusted flag
    m_RSConfig.isAdjusted = ((static_cast<INT32>(m_RSConfig.RSConfig.statsHNum) != regionHorizNum) ||
        (static_cast<INT32>(m_RSConfig.RSConfig.statsVNum) != regionVertNum)) ? TRUE : FALSE;

    // Save the configuration data locally as well.
    m_RSConfig.RSConfig.statsHNum       = regionHorizNum;
    m_RSConfig.RSConfig.statsVNum       = regionVertNum;
    m_RSConfig.statsRgnWidth            = regionWidth;
    m_RSConfig.statsHOffset             = regionHorizOffset;
    m_RSConfig.statsVOffset             = regionVertOffset;
    m_RSConfig.statsRgnHeight           = regionHeight;
    m_RSConfig.bitDepth                 = RSOutputDepth - RSInputDepth;

    CAMX_LOG_VERBOSE(CamxLogGroupISP, "ISPRS: Hnum:%d Vnum:%d RegionH:%d RegionW:%d shift %d inputdepth %d outputdepth %d",
        m_RSConfig.RSConfig.statsHNum,
        m_RSConfig.RSConfig.statsVNum,
        m_RSConfig.statsRgnHeight,
        m_RSConfig.statsRgnWidth,
        m_inputConfigData.shiftBits,
        RSInputDepth,
        RSOutputDepth);

    if ((regionHorizNum <= 0) || (regionVertNum <= 0) || (regionHeight <= 0) ||
        (regionWidth <= 0) || (regionHorizOffset < 0) || (regionVertOffset < 0))
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "ISPRS: Invalid Hnum:%d Vnum:%d RegionH:%d RegionW:%d"
            " hOffset: %d vOffset: %d",
            m_RSConfig.RSConfig.statsHNum, m_RSConfig.RSConfig.statsVNum,
            m_RSConfig.statsRgnHeight, m_RSConfig.statsRgnWidth,
            m_RSConfig.statsHOffset, m_RSConfig.statsVOffset);

        result = CamxResultEInvalidArg;
    }

    return result;
}

CAMX_NAMESPACE_END
