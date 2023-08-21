////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxifewb12.cpp
/// @brief CAMXIFEWB12 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdefs.h"
#include "camxifewb12titan17x.h"
#include "camxifewb12.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "iqcommondefs.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB12::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB12::Create(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pCreateData)
    {
        IFEWB12* pModule = CAMX_NEW IFEWB12;

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
// IFEWB12::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB12::Initialize(
    IFEModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch(pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            m_pHWSetting = CAMX_NEW IFEWB12Titan17x;
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
// IFEWB12::UpdateIFEInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID IFEWB12::UpdateIFEInternalData(
    ISPInputData* pInputData)
{
    if (NULL != pInputData->pCalculatedData)
    {
        pInputData->pCalculatedData->colorCorrectionGains.blue      = static_cast<FLOAT>(m_bGain) / Q7;
        pInputData->pCalculatedData->colorCorrectionGains.greenEven = static_cast<FLOAT>(m_gGain) / Q7;
        pInputData->pCalculatedData->colorCorrectionGains.greenOdd  = static_cast<FLOAT>(m_gGain) / Q7;
        pInputData->pCalculatedData->colorCorrectionGains.red       = static_cast<FLOAT>(m_rGain) / Q7;

        CAMX_LOG_VERBOSE(CamxLogGroupISP, "Reporting Gains [%f, %f, %f, %f]",
           pInputData->pCalculatedData->colorCorrectionGains.red,
           pInputData->pCalculatedData->colorCorrectionGains.greenEven,
           pInputData->pCalculatedData->colorCorrectionGains.greenOdd,
           pInputData->pCalculatedData->colorCorrectionGains.red);
    }

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pIFETuningMetadata)
    {
        if (CamxResultSuccess != m_pHWSetting->UpdateTuningMetadata(pInputData->pIFETuningMetadata))
        {
            CAMX_LOG_WARN(CamxLogGroupISP, "UpdateTuningMetadata failed.");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB12::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB12::Execute(
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
            UpdateIFEInternalData(pInputData);
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
// IFEWB12::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL IFEWB12::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL             isChanged     = FALSE;
    AWBFrameControl* pNewAWBUpdate = NULL;
    AECFrameControl* pNewAECUpdate = NULL;
    ISPHALTagsData*  pHALTagsData  = pInputData->pHALTagsData;

    if ((NULL                               == pInputData->pOEMIQSetting)         &&
        (NULL                               != pHALTagsData)                      &&
        (ColorCorrectionModeTransformMatrix == pHALTagsData->colorCorrectionMode) &&
        (((ControlAWBModeOff                == pHALTagsData->controlAWBMode) &&
          (ControlModeAuto                  == pHALTagsData->controlMode))   ||
         (ControlModeOff                    == pHALTagsData->controlMode)))
    {
        m_manualGainOverride  = TRUE;
        isChanged             = TRUE;

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
    else if ((NULL != pInputData->pHwContext)   &&
             (NULL != pInputData->pAWBUpdateData) &&
             (NULL != pInputData->pAECUpdateData))
    {
        m_manualGainOverride = FALSE;
        pNewAWBUpdate        = pInputData->pAWBUpdateData;
        pNewAECUpdate        = pInputData->pAECUpdateData;

        if ((FALSE == Utils::FEqual(m_dependenceData.leftGGainWB, pNewAWBUpdate->AWBGains.gGain))       ||
            (FALSE == Utils::FEqual(m_dependenceData.leftBGainWB, pNewAWBUpdate->AWBGains.bGain))       ||
            (FALSE == Utils::FEqual(m_dependenceData.leftRGainWB, pNewAWBUpdate->AWBGains.rGain))       ||
            (FALSE == Utils::FEqual(m_dependenceData.predictiveGain, pNewAECUpdate->predictiveGain))    ||
            (TRUE  == pInputData->forceTriggerUpdate))
        {
            m_dependenceData.predictiveGain = pNewAECUpdate->predictiveGain;

            if (TRUE == pInputData->sensorData.isMono)
            {
                // Set Unity Gains for Mono sensor
                m_dependenceData.leftGGainWB = 1.0f;
                m_dependenceData.leftBGainWB = 1.0f;
                m_dependenceData.leftRGainWB = 1.0f;
            }
            else
            {
                m_dependenceData.leftGGainWB  = pNewAWBUpdate->AWBGains.gGain;
                m_dependenceData.leftBGainWB  = pNewAWBUpdate->AWBGains.bGain;
                m_dependenceData.leftRGainWB  = pNewAWBUpdate->AWBGains.rGain;
            }
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
// IFEWB12::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult IFEWB12::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult     result = CamxResultSuccess;
    WB12OutputData outputData = {0, 0, 0};

    result  = IQInterface::IFEWB12CalculateSetting(&m_dependenceData, &outputData);
    m_rGain = outputData.rGain;
    m_gGain = outputData.gGain;
    m_bGain = outputData.bGain;
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupISP, "WB Calculation Failed. result %d", result);
    }


    if (CamxLogGroupIQMod == (pInputData->dumpRegConfig & CamxLogGroupIQMod))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// IFEWB12::IFEWB12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEWB12::IFEWB12()
{
    m_type      = ISPIQModuleType::IFEWB;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IFEWB12::~IFEWB12
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IFEWB12::~IFEWB12()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
