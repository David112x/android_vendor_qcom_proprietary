////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifehvx.cpp
/// @brief ife hvx implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxifehvx.h"
#include "camxifeproperty.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::Create(
    IFEModuleCreateData * pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pHvxInitializeData))
    {
        pCreateData->pModule = CAMX_NEW IFEHVX(pCreateData->pHvxInitializeData);

        if (NULL == pCreateData->pModule)
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Memory allocation failed");
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
// IFEHVX::GetHVXInputResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::GetHVXInputResolution(
    ISPInputData * pInputData)
{
    CDKResult          cdkresult;
    CamxResult         result = CamxResultSuccess;
    HVXResolutionInfo* pResInfo = NULL;

    if (NULL != m_HVXParams.pHVXAlgoCallbacks->pHVXInitialize)
    {
        cdkresult = m_HVXParams.pHVXAlgoCallbacks->pHVXInitialize(&m_HVXParams.pOEMData);
        if (cdkresult != CDKResultSuccess)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "HVX Initialize failed result %d ", cdkresult);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "HVX CHI callback is not supported\n");;
        result = CamxResultEUnsupported;
    }

    if (NULL != m_HVXParams.pHVXAlgoCallbacks->pHVXGetResolutionInfo)
    {
        pResInfo = &m_HVXParams.resInfo;
        pResInfo->sensorWidth = pInputData->HVXData.sensorInput.width;
        pResInfo->sensorHeight = pInputData->HVXData.sensorInput.height;
        pResInfo->outputFormat = static_cast<UINT>(pInputData->HVXData.format);
        pInputData->HVXData.HVXOut.height = pResInfo->sensorHeight;
        pInputData->HVXData.HVXOut.width = pResInfo->sensorWidth;


        cdkresult = m_HVXParams.pHVXAlgoCallbacks->pHVXGetResolutionInfo(m_HVXParams.pOEMData, &pResInfo);

        if (cdkresult != CDKResultSuccess)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "HVX Initialize failed result %d ", cdkresult);
        }
        else
        {
            m_tapPoint = pResInfo->tappingPoint;
            if ((pResInfo->hvxOutWidth > 0) &&
                (pResInfo->hvxOutHeight > 0) &&
                ((pResInfo->hvxOutWidth != pResInfo->sensorWidth) ||
                (pResInfo->hvxOutHeight != pResInfo->sensorHeight)))
            {
                if ((m_tapPoint == IFEBEGINNING) &&
                    ((pResInfo->hvxOutWidth < pResInfo->sensorWidth) ||
                    (pResInfo->hvxOutHeight < pResInfo->sensorHeight)))
                {
                    pInputData->HVXData.HVXOut.height = pResInfo->hvxOutHeight;
                    pInputData->HVXData.HVXOut.width = pResInfo->hvxOutWidth;

                    pInputData->HVXData.DSEnabled = TRUE;

                    CAMX_LOG_INFO(CamxLogGroupISP, "[HVX_DBG]: Width x Height = %d x %d tapPoint %d DSEnabled %d",
                        pResInfo->hvxOutWidth, pResInfo->hvxOutHeight, m_tapPoint,
                        pInputData->HVXData.DSEnabled);

                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupISP, "Width x Height = %d x %d tapPoint %d ",
                        pResInfo->hvxOutWidth, pResInfo->hvxOutHeight, m_tapPoint);
                    m_moduleEnable = FALSE;
                }
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupISP, "No change in resolution");
                pInputData->HVXData.DSEnabled = FALSE;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::InitConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::InitConfiguration(
    ISPInputData*  pInputData)
{
    HVXStubQueryCaps queryCaps;
    HVXGetSetInfo*   pHVXInfo;
    CDKResult        cdkresult;
    CamxResult       result = CamxResultSuccess;
    INT              numUnits = 1;

    Utils::Memset(&queryCaps, 0, sizeof(HVXStubQueryCaps));
    pHVXInfo = &m_HVXParams.HVXInfo;
    if (result == CamxResultSuccess)
    {
        result = IFEDSPInterface::QueryCapabilities(0, &queryCaps);
    }
    if ((result == CamxResultSuccess) &&
        (NULL != m_HVXParams.pHVXAlgoCallbacks->pHVXGetSetHVXInfo))
    {
        pHVXInfo->availableHVXUnits       = queryCaps.maxHvxUnit;
        pHVXInfo->availableHVXVectorMode  = static_cast<HVXVectorMode>(queryCaps.hvxStubVectorMode);
        pHVXInfo->availableL2Size         = queryCaps.maxL2Size;


        cdkresult = m_HVXParams.pHVXAlgoCallbacks->pHVXGetSetHVXInfo(m_HVXParams.pOEMData, pHVXInfo);
        if (cdkresult != CDKResultSuccess)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "HVXGetSetHVXInfo failed result %d ", cdkresult);
        }
        else
        {
            m_moduleEnable = pHVXInfo->hvxEnable;

            CAMX_LOG_INFO(CamxLogGroupISP, "HVX m_moduleEnable %d m_tapPoint %d ",
                m_moduleEnable, m_tapPoint);
        }
    }

    if (IFEModuleMode::DualIFENormal == pInputData->pipelineIFEData.moduleMode)
    {
        numUnits = 2;
        m_config.isp_type = HVX_STUB_IFE_BOTH;
    }
    else
    {
        m_config.isp_type = HVX_STUB_IFE1;
    }

    if ((numUnits * pHVXInfo->requestHVXUnits) > pHVXInfo->availableHVXUnits)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "insufficient hvx units %d available %d\n",
            (numUnits * pHVXInfo->requestHVXUnits),
            pHVXInfo->availableHVXUnits);

        result = CamxResultEUnsupported;
    }

    if (FALSE == m_moduleEnable)
    {
        result = CamxResultEUnsupported;
    }

    if ((result == CamxResultSuccess) && (TRUE == pHVXInfo->isStatsNeeded))
    {
        m_HVXParams.setCallbackFunc.callbackdata  = m_HVXParams.pHVXAlgoCallbacks->pHVXCallbackData;
        m_HVXParams.setCallbackFunc.ErrorCallback = NULL;
        IFEDSPInterface::SetCallbackFunctions(static_cast<VOID*>(&m_HVXParams.setCallbackFunc));
    }

    if (result == CamxResultSuccess)
    {
        result = IFEConvertSensorFormat(&pInputData->sensorData.format,
                                        &m_config.sensor_info.bayer_format);
    }

    if (result == CamxResultSuccess)
    {
        m_dspState = DSP_STATE_INITIALIZE;
    }
    else
    {
        m_dspState = DSP_STATE_INVALID;
    }

    CAMX_LOG_ERROR(CamxLogGroupISP, "last  enter m_moduleEnable %d m_tapPoint %d ",
        m_moduleEnable, m_tapPoint);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::PrepareStripingParameters
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::PrepareStripingParameters(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (NULL != pInputData->pStripingInput)
        {
            pInputData->pStripingInput->stripingInput.tapoffPointHVX = static_cast<int16_t>(m_tapPoint);
            pInputData->pStripingInput->stripingInput.tapoffPointHVX = static_cast<int16_t>(m_HVXParams.HVXInfo.kernelSize);
            m_pState = &pInputData->pStripeConfig->stateHVX;
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Data");
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::PrepareStreamOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::PrepareStreamOn()
{
    CamxResult  result = CamxResultSuccess;
    HVXStubOpen HVXOpen;

    if (m_dspState != DSP_STATE_INITIALIZE)
    {
        m_moduleEnable = FALSE;
    }

    if (TRUE == m_moduleEnable)
    {
        HVXOpen.IFEType = static_cast<HVXStubVFEType>(m_config.isp_type);
        OsUtils::StrLCpy(HVXOpen.name, m_HVXParams.HVXInfo.algorithmName, sizeof(m_HVXParams.HVXInfo.algorithmName));
        result = IFEDSPInterface::DSPOpen(&m_hDSPHandle, &HVXOpen);
        if (result != CamxResultSuccess)
        {
            m_moduleEnable = FALSE;
            result         = CamxResultEFailed;
        }
        else
        {
            m_dspState = DSP_STATE_OPENED;
        }
    }

    if ((TRUE == m_moduleEnable) && (result == CamxResultSuccess))
    {
        result = IFEDSPInterface::DSPReset(&m_hDSPHandle);
        if (result != CamxResultSuccess)
        {
            m_moduleEnable = FALSE;
            result         = CamxResultEFailed;
        }
    }

    if ((TRUE == m_moduleEnable) && (result == CamxResultSuccess))
    {
        result = SetConfiguration();
        if (result != CamxResultSuccess)
        {
            m_moduleEnable = FALSE;
            result         = CamxResultEFailed;
        }
        else
        {
            m_dspState = DSP_STATE_CONFIGURED;
        }
    }

    if ((TRUE == m_moduleEnable) && (result == CamxResultSuccess))
    {
        result = IFEDSPInterface::DSPStart(&m_hDSPHandle);
        if (result != CamxResultSuccess)
        {
            m_moduleEnable = FALSE;
            result         = CamxResultEFailed;
        }
        else
        {
            m_dspState = DSP_STATE_STREAMING;
        }
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::Execute(
    ISPInputData * pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        m_pState = &pInputData->pStripeConfig->stateHVX;
        if (TRUE == CheckDependenceChange(pInputData))
        {
            result = RunCalculation();
            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("HVX module calculation Failed.");
            }
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
// IFEHVX::UnInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::UnInitialize()
{
    CamxResult result = CamxResultSuccess;

    if (DSP_STATE_STREAMING == m_dspState)
    {
        result = IFEDSPInterface::DSPStop(&m_hDSPHandle);
        m_dspState = DSP_STATE_CONFIGURED;
    }

    if ((DSP_STATE_CONFIGURED == m_dspState) || (DSP_STATE_OPENED == m_dspState))
    {
        result = IFEDSPInterface::DSPClose(&m_hDSPHandle);
        m_dspState = DSP_STATE_INVALID;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::~IFEHVX
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHVX::~IFEHVX()
{
    UnInitialize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::IFEHVX
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHVX::IFEHVX()
{
    m_type         = ISPIQModuleType::IFEHVX;
    m_moduleEnable = FALSE;
    m_tapPoint     = HVXTapPoint::IFEBEGINNING;
    m_dspState     = DSP_STATE_INVALID;
    Utils::Memset(&m_HVXParams, 0, sizeof(CAMXHVXParameters));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::IFEHVX
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEHVX::IFEHVX(
    IFEHVXInfo* pHvxInitializeData)
{
    m_type                       = ISPIQModuleType::IFEHVX;
    m_HVXParams.pHVXAlgoCallbacks = pHvxInitializeData->pHVXAlgoCallbacks;

    m_moduleEnable               = FALSE;
    m_tapPoint                   = HVXTapPoint::IFEBEGINNING;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEHVX::CheckDependenceChange(
    const ISPInputData * pInputData)
{
    BOOL              isChanged       = FALSE;
    AECFrameControl*  pNewAECUpdate   = pInputData->pAECUpdateData;
    AWBFrameControl*  pNewAWBUpdate   = pInputData->pAWBUpdateData;
    FLOAT             newExposureTime = 0.0f;

    if ((NULL != pInputData->pAECUpdateData) &&
        (NULL != pInputData->pAWBUpdateData))
    {
        newExposureTime = static_cast<FLOAT>(pNewAECUpdate->exposureInfo[ExposureIndexShort].exposureTime);

        if (((TRUE == m_moduleEnable) &&
            (m_dspState == DSP_STATE_STREAMING)) &&
            ((FALSE == Utils::FEqual(m_dependenceData.luxIndex, pNewAECUpdate->luxIndex))         ||
            (FALSE == Utils::FEqual(m_dependenceData.realGain,
                                    pNewAECUpdate->exposureInfo[ExposureIndexSafe].linearGain))   ||
            (FALSE == Utils::FEqual(m_dependenceData.AECSensitivity,
                                    pNewAECUpdate->exposureInfo[ExposureIndexShort].sensitivity)) ||
            (FALSE == Utils::FEqual(m_dependenceData.AECSensitivity,
                                    pNewAECUpdate->exposureInfo[ExposureIndexSafe].sensitivity))  ||
            (FALSE == Utils::FEqual(m_dependenceData.exposureTime, newExposureTime))              ||
            (FALSE == Utils::FEqual(m_dependenceData.GGain, pNewAWBUpdate->AWBGains.gGain))       ||
            (FALSE == Utils::FEqual(m_dependenceData.BGain, pNewAWBUpdate->AWBGains.bGain))       ||
            (FALSE == Utils::FEqual(m_dependenceData.RGain, pNewAWBUpdate->AWBGains.rGain))       ||
            (TRUE  == pInputData->forceTriggerUpdate)))
        {
            m_dependenceData.luxIndex       = pNewAECUpdate->luxIndex;
            m_dependenceData.realGain       = pNewAECUpdate->exposureInfo[ExposureIndexSafe].linearGain;
            m_dependenceData.AECSensitivity = pNewAECUpdate->exposureInfo[ExposureIndexSafe].sensitivity;
            m_dependenceData.exposureTime   = newExposureTime;
            m_dependenceData.GGain          = pNewAWBUpdate->AWBGains.gGain;
            m_dependenceData.BGain          = pNewAWBUpdate->AWBGains.bGain;
            m_dependenceData.RGain          = pNewAWBUpdate->AWBGains.rGain;

            if (FALSE == Utils::FEqual(pNewAECUpdate->exposureInfo[ExposureIndexShort].sensitivity, 0.0f))
            {
                m_dependenceData.DRCGain = pNewAECUpdate->exposureInfo[ExposureIndexSafe].sensitivity /
                    pNewAECUpdate->exposureInfo[ExposureIndexShort].sensitivity;
            }
            else
            {
                m_dependenceData.DRCGain = 0.0f;
            }

            isChanged = TRUE;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP,
            "Invalid Input: pAECUpdateData %p  pNewAWBUpdate %p",
            pInputData->pAECUpdateData,
            pInputData->pAWBUpdateData);
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::SetConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::SetConfiguration()
{
    CamxResult           result        = CamxResultSuccess;
    CDKResult            cdkresult     = CDKResultSuccess;
    HVXStubStaticConfig* pStaticConfig = &m_config.stubStaticConfig;
    HVXGetSetInfo*       pHVXInfo;
    HVXResolutionInfo*   pResInfo;
    HVXDSPConfig         dspConfig;

    pHVXInfo = &m_HVXParams.HVXInfo;
    pResInfo = &m_HVXParams.resInfo;
    dspConfig.pDSPStaticConfig  = IFEDSPInterface::StaticConfig;
    dspConfig.pDSPDynamicConfig = IFEDSPInterface::DynamicConfig;

    pStaticConfig->bitsPerPixel      = 10;
    pStaticConfig->width             = pResInfo->sensorWidth;
    pStaticConfig->height            = pResInfo->sensorHeight;
    pStaticConfig->hvxStubVectorMode = pHVXInfo->requestHVXVectorMode;
    pStaticConfig->hvxUnitNo[0]      = pHVXInfo->requestHVXUnits;
    pStaticConfig->hvxUnitNo[1]      = pHVXInfo->requestHVXUnits;
    pStaticConfig->OutWidth          = pResInfo->hvxOutWidth;
    pStaticConfig->OutHeight         = pResInfo->hvxOutHeight;
    pStaticConfig->tappingPoint      = m_tapPoint;

    if (m_config.isp_type == HVX_STUB_IFE_BOTH)
    {
        pStaticConfig->IFEid            = HVX_STUB_IFE_BOTH;
        pStaticConfig->imageOverlap     = m_pState->overlap;
        pStaticConfig->rightImageOffset = m_pState->rightImageOffset;
    }
    else
    {
        // @todo (CAMX-2059) Need to Add Support for which IFE in use
        pStaticConfig->IFEid = HVX_STUB_IFE0;
    }
    pStaticConfig->pixelFormat       = m_config.sensor_info.bayer_format;

    if (NULL != m_HVXParams.pHVXAlgoCallbacks->pHVXSetConfig)
    {
        cdkresult = m_HVXParams.pHVXAlgoCallbacks->pHVXSetConfig(m_HVXParams.pOEMData,
                                                              &m_config,
                                                              &dspConfig,
                                                              &m_hDSPHandle);

        if (cdkresult != CDKResultSuccess)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "HVXSetConfig failed result %d ", cdkresult);
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEHVX::IFEConvertSensorFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::IFEConvertSensorFormat(
    const PixelFormat*              pPixelFormat,
    HVXSensorFilterArrangement*     pHVXSensor)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pPixelFormat) && (NULL != pHVXSensor))
    {
        switch (*pPixelFormat)
        {
            case PixelFormat::BayerRGGB:
                *pHVXSensor = HVXSensorFilterArrangement::HVX_SENSOR_RGGB;
                break;
            case PixelFormat::BayerBGGR:
                *pHVXSensor = HVXSensorFilterArrangement::HVX_SENSOR_BGGR;
                break;
            case PixelFormat::BayerGBRG:
                *pHVXSensor = HVXSensorFilterArrangement::HVX_SENSOR_GBRG;
                break;
            case PixelFormat::BayerGRBG:
                *pHVXSensor = HVXSensorFilterArrangement::HVX_SENSOR_GRBG;
                break;
            case PixelFormat::YUVFormatUYVY:
                *pHVXSensor = HVXSensorFilterArrangement::HVX_SENSOR_UYVY;
                break;
            case PixelFormat::YUVFormatYUYV:
                *pHVXSensor = HVXSensorFilterArrangement::HVX_SENSOR_YUYV;
                break;
            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("No Corresponding Format. Format = %d", *pPixelFormat);
                result = CamxResultEUnsupported;
                break;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_ASSERT_ALWAYS_MESSAGE("Null data ");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::RunCalculation()
{
    CamxResult   result    = CamxResultSuccess;
    CDKResult    cdkresult = CDKResultSuccess;
    HVXDSPConfig dspConfig;

    dspConfig.pDSPDynamicConfig = IFEDSPInterface::DynamicConfig;
    if (NULL != m_HVXParams.pHVXAlgoCallbacks->pHVXExecute)
    {
        cdkresult = m_HVXParams.pHVXAlgoCallbacks->pHVXExecute(m_HVXParams.pOEMData,
                                                            &m_dependenceData,
                                                            &dspConfig,
                                                            &m_hDSPHandle);
        if (cdkresult != CDKResultSuccess)
        {
            result = CamxResultEFailed;
            CAMX_LOG_ERROR(CamxLogGroupISP, "HVXExecute failed result %d ", cdkresult);
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_VERBOSE(CamxLogGroupISP, "InValid Pointer ");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEHVX::OnStreamOff
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEHVX::OnStreamOff(
    CHIDEACTIVATEPIPELINEMODE modeBitmask)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(modeBitmask);

    if (TRUE == m_moduleEnable)
    {
        result = IFEDSPInterface::DSPStop(&m_hDSPHandle);

        if (CamxResultSuccess == result)
        {
            m_dspState = DSP_STATE_CONFIGURED;
        }
    }

    return result;
}


CAMX_NAMESPACE_END
