////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifecsstats14.cpp
/// @brief CS Stats v1.4 Implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifecsstats14.h"
#include "camxifecsstats14titan17x.h"
#include "camxifecsstats14titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSStats14::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSStats14::Create(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        CSStats14* pModule = CAMX_NEW CSStats14();
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
            CAMX_LOG_ERROR(CamxLogGroupISP, "Failed to create CSStats14 object.");
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
// CSStats14::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSStats14::Initialize(
    IFEStatsModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFECSStats14Titan480;
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFECSStats14Titan17x;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_cmdLength = m_pHWSetting->GetCommandLength();
        result = m_pHWSetting->GetHWCapability(&m_hwCapability);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create CS Setting Class, no memory");
        result = CamxResultENoMemory;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSStats14::CheckDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CSStats14::CheckDependency(
    ISPInputData* pInputData)
{
    BOOL            result              = TRUE;
    UINT32          inputWidth = 0;
    UINT32          inputHeight = 0;
    CropInfo*       pSensorCAMIFCrop    = &pInputData->pStripeConfig->CAMIFCrop;
    CSStatsConfig*  pCSConfig           = &pInputData->pStripeConfig->CSStatsUpdateData.statsConfig;

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
    if ((0 != Utils::Memcmp(&m_CSConfig.CSConfig, pCSConfig, sizeof(CSStatsConfig))) ||
        (m_CAMIFHeight                      != inputHeight) ||
        (m_CAMIFWidth                       != inputWidth)  ||
        (TRUE                               == pInputData->forceTriggerUpdate))
    {
        m_CSConfig.CSConfig = *pCSConfig;
        m_CAMIFWidth        = inputWidth;
        m_CAMIFHeight       = inputHeight;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "CS Config H/V Num [%u * %u], ROI [%u, %u, %u, %u]",
                         m_CSConfig.CSConfig.statsHNum,
                         m_CSConfig.CSConfig.statsVNum,
                         m_CSConfig.CSConfig.statsHOffset,
                         m_CSConfig.CSConfig.statsVOffset,
                         m_CSConfig.CSConfig.statsRgnWidth,
                         m_CSConfig.CSConfig.statsRgnHeight);

        result = TRUE;
    }
    else
    {
        result = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSStats14::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSStats14::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (NULL != pInputData->pStripingInput)
        {
            // Check if dependent is valid and been updated compared to last request
            if (TRUE == CheckDependency(pInputData))
            {
                AdjustROI();
            }

            pInputData->pStripingInput->stripingInput.RSCSInput.CSEnable          = m_moduleEnable;
            pInputData->pStripingInput->stripingInput.RSCSInput.CSRgnHorNum       = m_CSConfig.CSConfig.statsHNum - 1;
            pInputData->pStripingInput->stripingInput.RSCSInput.CSRgnVerNum       = m_CSConfig.CSConfig.statsVNum - 1;
            pInputData->pStripingInput->stripingInput.RSCSInput.CSRgnWidth       = m_CSConfig.CSConfig.statsRgnWidth - 1;
            pInputData->pStripingInput->stripingInput.RSCSInput.CSRgnHorOffset    = m_CSConfig.CSConfig.statsHOffset;
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
// CSStats14::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSStats14::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if (NULL != pInputData)
    {
        m_inputConfigData.pISPInputData = pInputData;

        // Check if dependent is valid and been updated compared to last request
        if (TRUE == CheckDependency(pInputData))
        {
            AdjustROI();
            m_pHWSetting->PackIQRegisterSetting(&m_inputConfigData, NULL);
            result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
        }

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
// CSStats14::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CSStats14::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    /// Publish CS Configuration to be used by the parser
    pInputData->pCalculatedData->metadata.CSStats = m_CSConfig;

    m_pHWSetting->SetupRegisterSetting(&m_inputConfigData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSStats14::CSStats14
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSStats14::CSStats14()
{
    m_type          = ISPStatsModuleType::IFECS;
    m_moduleEnable  = TRUE;

    m_CAMIFWidth    = 0;
    m_CAMIFHeight   = 0;

    m_inputConfigData.pCSConfig = &m_CSConfig;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSStats14::~CSStats14
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CSStats14::~CSStats14()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CSStats14::AdjustROI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CSStats14::AdjustROI()
{
    CamxResult result               = CamxResultSuccess;
    UINT32     regionHorizNum       = 0;
    UINT32     regionVertNum        = 0;
    UINT32     regionHeight         = 0;
    UINT32     regionWidth          = 0;
    UINT32     regionHorizOffset    = 0;
    UINT32     regionVertOffset     = 0;
    UINT32     effectiveFrameWidth  = 0;
    UINT32     effectiveFrameHeight = 0;
    UINT32     minROIWidth          = 0;
    UINT32     minROIHeight         = 0;

    // Minimum ROI width - min horizontal regions * minimum region width
    minROIWidth = m_hwCapability.minHorizRegions * m_hwCapability.minRegionWidth;
    // Minimum ROI height - min vertical regions * minimum region height
    minROIHeight = m_hwCapability.minVertRegions * m_hwCapability.minRegionHeight;

    // Configure region offset`
    // Horizontal offset - minimum is 0 and maximum is image_width - 1
    regionHorizOffset = m_CSConfig.CSConfig.statsHOffset;
    regionHorizOffset = Utils::ClampUINT32(regionHorizOffset, 0, (m_CAMIFWidth - minROIWidth) - 1);

    // Vertical offset - minimum is 0 and maximum is image_height - 1
    regionVertOffset = m_CSConfig.CSConfig.statsVOffset;
    regionVertOffset = Utils::ClampUINT32(regionVertOffset, 0, (m_CAMIFHeight - minROIHeight) - 1);

    effectiveFrameWidth  = m_CAMIFWidth - regionHorizOffset;
    effectiveFrameHeight = m_CAMIFHeight - regionVertOffset;

    // Setting Horizontal and Vertical region numbers
    regionHorizNum = m_CSConfig.CSConfig.statsHNum;
    if ((0 == regionHorizNum) || (regionHorizNum > m_hwCapability.maxHorizRegions))
    {
        regionHorizNum = m_hwCapability.maxHorizRegions;
    }

    regionVertNum = m_CSConfig.CSConfig.statsVNum;
    if ((0 == regionVertNum) || (regionVertNum > m_hwCapability.maxVertRegions))
    {
        regionVertNum = m_hwCapability.maxVertRegions;
    }

    // Region width - minimum is 2 pixels and maximum is 4 pixels
    // Get the ceiling value for regionWidth
    regionWidth = m_CSConfig.CSConfig.statsRgnWidth;
    if (0 == regionWidth)
    {
        regionWidth = (effectiveFrameWidth + regionHorizNum - 1) / regionHorizNum;
        // Adjust horizontal regions based on new regionWidth again
        regionHorizNum = effectiveFrameWidth / Utils::MaxUINT32(regionWidth, 1);
    }

    // Number of lines for a given region - minimum is 1 and maximum is image_height lines
    regionHeight = m_CSConfig.CSConfig.statsRgnHeight;
    if (0 == regionHeight)
    {
        regionHeight = effectiveFrameHeight / Utils::MaxUINT32(regionVertNum, m_hwCapability.minRegionHeight);
    }

    // Region height and vertical offset must always multiple of 2
    regionHeight        = Utils::EvenFloorUINT32(regionHeight);
    regionVertOffset    = Utils::EvenFloorUINT32(regionVertOffset);

    // Also clamp parameters to Min and Max of Hardware limitations
    regionWidth    = Utils::ClampUINT32(regionWidth, m_hwCapability.minRegionWidth, m_hwCapability.maxRegionWidth);
    regionHeight   = Utils::ClampUINT32(regionHeight, m_hwCapability.minRegionHeight, effectiveFrameHeight);
    regionHorizNum = Utils::ClampUINT32(regionHorizNum, m_hwCapability.minHorizRegions, m_hwCapability.maxHorizRegions);
    regionVertNum  = Utils::ClampUINT32(regionVertNum, m_hwCapability.minVertRegions, m_hwCapability.maxVertRegions);

    // Sanity check to ensure configuration doesn't exceed camif boundaries
    if ((regionWidth * regionHorizNum) > effectiveFrameWidth)
    {
        regionWidth = effectiveFrameWidth / regionHorizNum;
    }

    if ((regionHeight * regionVertNum) > effectiveFrameHeight)
    {
        regionHeight = effectiveFrameHeight / regionVertNum;
    }

    // Calculate shift bits - used to yield 16 bits column sum by right shifting the accumulated column sum
    m_inputConfigData.shiftBits =
        static_cast<UINT16>(Utils::CalculateShiftBits(regionHeight * regionWidth,
                                                      m_hwCapability.inputDepth,
                                                      m_hwCapability.outputDepth));

    // Save the configuration data locally as well.
    m_CSConfig.CSConfig.statsHNum      = regionHorizNum;
    m_CSConfig.CSConfig.statsVNum      = regionVertNum;
    m_CSConfig.CSConfig.statsRgnWidth  = regionWidth;
    m_CSConfig.CSConfig.statsRgnHeight = regionHeight;
    m_CSConfig.CSConfig.statsHOffset   = regionHorizOffset;
    m_CSConfig.CSConfig.statsVOffset   = regionVertOffset;

    return result;
}

CAMX_NAMESPACE_END
