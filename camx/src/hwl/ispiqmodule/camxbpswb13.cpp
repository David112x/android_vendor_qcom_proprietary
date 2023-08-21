////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxbpswb13.cpp
/// @brief CAMXBPSWB13 class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxbpswb13.h"
#include "camxdefs.h"
#include "camxhwenvironment.h"
#include "camxiqinterface.h"
#include "camxnode.h"
#include "camxtitan17xcontext.h"
#include "iqcommondefs.h"
#include "camxbpswb13titan17x.h"
#include "camxbpswb13titan480.h"
#include "camxisphwsetting.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSWB13::Create(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pCreateData) && (NULL != pCreateData->pNodeIdentifier))
    {
        BPSWB13* pModule = CAMX_NEW BPSWB13(pCreateData->pNodeIdentifier);

        if (NULL != pModule)
        {
            result = pModule->Initialize(pCreateData);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupBPS, "Module initialization failed !!");
                CAMX_DELETE pModule;
                pModule = NULL;
            }
        }
        else
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Memory allocation failed");
        }
        pCreateData->pModule = pModule;
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSWB13::Initialize(
    BPSModuleCreateData* pCreateData)
{
    CamxResult result = CamxResultSuccess;

    switch (pCreateData->titanVersion)
    {
        case CSLCameraTitanVersion::CSLTitan480:
            result = BPSWB13Titan480::Create(&m_pHWSetting);
            break;

        case CSLCameraTitanVersion::CSLTitan150:
        case CSLCameraTitanVersion::CSLTitan160:
        case CSLCameraTitanVersion::CSLTitan170:
        case CSLCameraTitanVersion::CSLTitan175:
            result = BPSWB13Titan17x::Create(&m_pHWSetting);
            break;
        default:
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Invalid hardware version: %d", pCreateData->titanVersion);
            break;
    }

    if (CamxResultSuccess == result)
    {
        m_dependenceData.pHWSetting = static_cast<VOID*>(m_pHWSetting);
        m_cmdLength                 = m_pHWSetting->GetCommandLength();
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Unable to create HW Setting Class, no memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13::CheckDependenceChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL BPSWB13::CheckDependenceChange(
    const ISPInputData* pInputData)
{
    BOOL isChanged = FALSE;

    if (NULL != pInputData)
    {
        ISPHALTagsData* pHALTagsData = pInputData->pHALTagsData;

        if ((NULL                               == pInputData->pOEMIQSetting)         &&
            (NULL                               != pHALTagsData)                      &&
            (ColorCorrectionModeTransformMatrix == pHALTagsData->colorCorrectionMode) &&
            (((ControlAWBModeOff == pHALTagsData->controlAWBMode) &&
              (ControlModeAuto   == pHALTagsData->controlMode))   ||
             (ControlModeOff     == pHALTagsData->controlMode)))
        {
            m_manualGainOverride = TRUE;
            isChanged            = TRUE;
            m_moduleEnable       = TRUE;

            m_manualControl.rGain = pHALTagsData->colorCorrectionGains.red;
            // Use the green even and ignore green odd
            m_manualControl.gGain = pHALTagsData->colorCorrectionGains.greenEven;
            m_manualControl.bGain = pHALTagsData->colorCorrectionGains.blue;

            CAMX_LOG_VERBOSE(CamxLogGroupBPS, "App Gains [%f, %f, %f, %f]",
                m_manualControl.rGain,
                m_manualControl.gGain,
                pHALTagsData->colorCorrectionGains.greenOdd,
                m_manualControl.bGain);
        }
        else if ((NULL != pInputData->pHwContext)   &&
                 (NULL != pInputData->pAWBUpdateData) &&
                 (NULL != pInputData->pAECUpdateData))
        {
            AWBFrameControl* pNewAWBUpdate = pInputData->pAWBUpdateData;
            AECFrameControl* pNewAECUpdate = pInputData->pAECUpdateData;
            m_manualGainOverride = FALSE;

            if ((FALSE == Utils::FEqual(m_dependenceData.leftGGainWB, pNewAWBUpdate->AWBGains.gGain)) ||
                (FALSE == Utils::FEqual(m_dependenceData.leftBGainWB, pNewAWBUpdate->AWBGains.bGain)) ||
                (FALSE == Utils::FEqual(m_dependenceData.leftRGainWB, pNewAWBUpdate->AWBGains.rGain)) ||
                (FALSE == Utils::FEqual(m_dependenceData.predictiveGain, pNewAECUpdate->predictiveGain)))
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
                    m_dependenceData.leftGGainWB  = pNewAWBUpdate->AWBGains.gGain;
                    m_dependenceData.leftBGainWB  = pNewAWBUpdate->AWBGains.bGain;
                    m_dependenceData.leftRGainWB  = pNewAWBUpdate->AWBGains.rGain;
                }

                m_dependenceData.predictiveGain = pNewAECUpdate->predictiveGain;
                isChanged = TRUE;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    return isChanged;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13::RunCalculation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSWB13::RunCalculation(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        WB13OutputData outputData;
        outputData.manualControl.gGain = m_manualControl.gGain;
        outputData.manualControl.bGain = m_manualControl.bGain;
        outputData.manualControl.rGain = m_manualControl.rGain;
        outputData.manualGainOverride  = m_manualGainOverride;

        result = IQInterface::WB13CalculateSetting(&m_dependenceData, pInputData->pOEMIQSetting, &outputData);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "WB Calculation Failed. result %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidPointer;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer");
    }

    if ((NULL != pInputData) && (FALSE != IsDumpRegConfig(pInputData->dumpRegConfig, pInputData->regOffsetIndex)))
    {
        m_pHWSetting->DumpRegConfig();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13::UpdateBPSInternalData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID BPSWB13::UpdateBPSInternalData(
    ISPInputData* pInputData
    ) const
{
    CamxResult result = CamxResultSuccess;

    // Post tuning metadata if setting is enabled
    if (NULL != pInputData->pBPSTuningMetadata)
    {
        result = m_pHWSetting->UpdateTuningMetadata(pInputData);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_WARN(CamxLogGroupBPS, "UpdateTuningMetadata failed.");
            result = CamxResultSuccess; // Non-fatal error
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13::Execute
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult BPSWB13::Execute(
    ISPInputData* pInputData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pInputData)
    {
        if (TRUE == CheckDependenceChange(pInputData))
        {
            if (FALSE == pInputData->useHardcodedRegisterValues)
            {
                result = RunCalculation(pInputData);
            }
        }

        if ((CamxResultSuccess == result) && (TRUE == m_moduleEnable))
        {
            result = m_pHWSetting->CreateCmdList(pInputData, NULL);
        }

        if (CamxResultSuccess == result)
        {
            UpdateBPSInternalData(pInputData);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupBPS, "WB Operation failed %d", result);
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupBPS, "Null Input Pointer %p, m_pHWSetting %p", pInputData, m_pHWSetting);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BPSWB13::BPSWB13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSWB13::BPSWB13(
    const CHAR* pNodeIdentifier)
{
    m_pNodeIdentifier = pNodeIdentifier;
    m_type            = ISPIQModuleType::BPSWB;
    m_moduleEnable    = TRUE;
    m_pHWSetting      = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// BPSWB13::~BPSWB13
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BPSWB13::~BPSWB13()
{
    if (NULL != m_pHWSetting)
    {
        CAMX_DELETE m_pHWSetting;
        m_pHWSetting = NULL;
    }
}

CAMX_NAMESPACE_END
