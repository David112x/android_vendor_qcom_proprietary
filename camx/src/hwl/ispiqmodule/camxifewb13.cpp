////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifewb13.cpp
/// @brief CAMXIFEWB13 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifewb13titan480.h"
#include "camxifewb13.h"
#include "camxispiqmodule.h"
#include "camxiqinterface.h"
#include "iqcommondefs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB13::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEWB13* pModule = CAMX_NEW IFEWB13;

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
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupISP, "Memory allocation failed");
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
// IFEWB13::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB13::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            m_pHWSetting = CAMX_NEW IFEWB13Titan480;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupISP, "Not supported Titan 0x%x", pCreateData->titanVersion);
            result = CamxResultEUnsupported;
            break;
    }

    if (NULL != m_pHWSetting)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength                 = m_pHWSetting->GetCommandLength();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Unable to create HW Setting Class, Titan 0x%x", pCreateData->titanVersion);
        m_cmdLength = 0;
        result      = CamxResultENoMemory;
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB13::Execute(
    ISPInputData* pInputData)
{
    CamxResult result  = CamxResultSuccess;
    VOID* pSettingData = static_cast<VOID*>(pInputData);

    if ((NULL != pInputData) && (NULL != m_pHWSetting))
    {
        if (FALSE == pInputData->useHardcodedRegisterValues)
        {
            if (TRUE == CheckDependenceChange(pInputData))
            {
                result = RunCalculation(pInputData);

                if (CamxResultSuccess == result)
                {
                    result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
                }
            }
        }
        else
        {
            result = m_pHWSetting->CreateCmdList(pSettingData, NULL);
        }

        if (CamxResultSuccess == result)
        {
            static_cast<IFEWB13Titan480*>(m_pHWSetting)->UpdateIFEInternalData(pSettingData);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupISP, "Invalid Input: pInputData %p", pInputData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEWB13::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL                isChanged     = FALSE;
    AWBFrameControl*    pNewAWBUpdate = NULL;
    AECFrameControl*    pNewAECUpdate = NULL;

    ISPHALTagsData*  pHALTagsData = pInputData->pHALTagsData;

    if ((NULL == pInputData->pOEMIQSetting)                                       &&
        (NULL != pHALTagsData)                                                    &&
        (ColorCorrectionModeTransformMatrix == pHALTagsData->colorCorrectionMode) &&
        (((ControlAWBModeOff == pHALTagsData->controlAWBMode) &&
        (ControlModeAuto == pHALTagsData->controlMode))       ||
        (ControlModeOff == pHALTagsData->controlMode)))
    {
        isChanged = TRUE;

        m_dependenceData.leftRGainWB = pHALTagsData->colorCorrectionGains.red;
        // Use the green even and ignore green odd
        m_dependenceData.leftGGainWB = pHALTagsData->colorCorrectionGains.greenEven;
        m_dependenceData.leftBGainWB = pHALTagsData->colorCorrectionGains.blue;

        m_dependenceData.predictiveGain = 1.0f;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "App Gains [%f, %f, %f, %f]",
            pHALTagsData->colorCorrectionGains.red,
            pHALTagsData->colorCorrectionGains.greenEven,
            pHALTagsData->colorCorrectionGains.greenOdd,
            pHALTagsData->colorCorrectionGains.blue);
    }
    else if ((NULL != pInputData->pHwContext) &&
        (NULL != pInputData->pAWBUpdateData) &&
        (NULL != pInputData->pAECUpdateData))
    {
        pNewAWBUpdate = pInputData->pAWBUpdateData;
        pNewAECUpdate = pInputData->pAECUpdateData;

        if ((FALSE == Utils::FEqual(m_dependenceData.leftGGainWB, pNewAWBUpdate->AWBGains.gGain))    ||
            (FALSE == Utils::FEqual(m_dependenceData.leftBGainWB, pNewAWBUpdate->AWBGains.bGain))    ||
            (FALSE == Utils::FEqual(m_dependenceData.leftRGainWB, pNewAWBUpdate->AWBGains.rGain))    ||
            (FALSE == Utils::FEqual(m_dependenceData.predictiveGain, pNewAECUpdate->predictiveGain)) ||
            (TRUE  == pInputData->forceTriggerUpdate))
        {
            if (TRUE == pInputData->sensorData.isMono)
            {
                // Set Unity Gains for Mono sensor
                m_dependenceData.leftGGainWB = 1.0f;
                m_dependenceData.leftBGainWB = 1.0f;
                m_dependenceData.leftRGainWB = 1.0f;
            }
            else
            {
                m_dependenceData.leftGGainWB    = pNewAWBUpdate->AWBGains.gGain;
                m_dependenceData.leftBGainWB    = pNewAWBUpdate->AWBGains.bGain;
                m_dependenceData.leftRGainWB    = pNewAWBUpdate->AWBGains.rGain;
            }
            m_dependenceData.predictiveGain = pNewAECUpdate->predictiveGain;
            isChanged = TRUE;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "Null Input Pointer");
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB13::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult     result = CamxResultSuccess;

    if ((NULL != pInputData) &&
        (NULL != m_pHWSetting))
    {
        WB13OutputData outputData;

        result = IQInterface::WB13CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupISP, "WB Calculation Failed. result %d", result);
        }

        if ((CamxResultSuccess == result) &&
            (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex)))
        {
            m_pHWSetting->DumpRegConfig();
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_ASSERT_ALWAYS_MESSAGE("Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB13::IFEWB13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEWB13::IFEWB13()
{
    m_type         = ISPIQModuleType::IFEWB;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEWB13::~IFEWB13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEWB13::~IFEWB13()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
