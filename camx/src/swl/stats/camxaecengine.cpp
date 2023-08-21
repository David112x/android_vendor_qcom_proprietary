////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxaecengine.cpp
/// @brief The class that implements the AECEngine for AEC.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include "camxaecengine.h"
#include "camxcaecstatsprocessor.h"
#include "camxhal3module.h"
#include "camxstatsdebuginternal.h"
#include "camxtitan17xdefs.h"
#include "camxtrace.h"

CAMX_NAMESPACE_BEGIN

static const FLOAT  MaxROIWeight                    = 1000.0f; ///< Maximum ROI weight

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::CAECEngine
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAECEngine::CAECEngine()
    : m_warmStartDone(FALSE)
    , m_AECState(AECState::Inactive)
    , m_AECPreFlashState(PreFlashStateInactive)
    , m_calibFlashState(CalibrationFlashReady)
    , m_flashTrigger(FlashTrigger::Invalid)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
    m_operationMode = AECAlgoOperationModeCount;
    m_AECLastState = AECState::Inactive;
    SetLEDCalibrationState(LEDCalibrationState::Ready);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::~CAECEngine
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAECEngine::~CAECEngine()
{
    if (NULL != m_pStaticSettings)
    {
        Uninitialize(m_pStaticSettings->overrideCameraClose);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "m_pStaticSettings NULL pointer");
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAECEngine* CAECEngine::Create(
    CREATEAEC        pfnCreate,
    ChiStatsSession* pStatsSession,
    StatsCameraInfo* pCameraInfo)
{
    CamxResult   result          = CamxResultEFailed;
    CAECEngine*  pLocalInstance  = NULL;

    pLocalInstance = CAMX_NEW CAECEngine();
    if (NULL != pLocalInstance)
    {
        result = pLocalInstance->Initialize(pfnCreate, pStatsSession, pCameraInfo);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Engine init failed result: %s", Utils::CamxResultToString(result));
            CAMX_DELETE pLocalInstance;
            pLocalInstance = NULL;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "AEC Engine create failed - out of memory");
    }

    return pLocalInstance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECEngine::Destroy()
{
    // store dynamic inline calibration data when mode is in dynamic inline calibration state
    if (TRUE == m_isDynamicInlineCalibration)
    {
        StoreLEDInlineCalibrationData();

        // free allocated memory for dynamic inline data
        CAMX_FREE(m_LEDCalibration.pDynamicData);
    }

    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::Initialize(
    CREATEAEC        pfnCreate,
    ChiStatsSession* pStatsSession,
    StatsCameraInfo* pCameraInfo)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
    CamxResult result = CamxResultSuccess;

    m_pStaticSettings  = HwEnvironment::GetInstance()->GetStaticSettings();

    if (NULL == m_pStaticSettings || NULL == pStatsSession)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "m_pStaticSettings: %p or pStatsSession: %p pointer is NULL",
            m_pStaticSettings, pStatsSession);
        return CamxResultEInvalidPointer;
    }

    AECAlgoCreateParamList  createParamList                           = { 0 };
    AECAlgoCreateParam      createParams[AECAlgoCreateParamTypeCount] = {};
    UINT                    overrideCameraOpen                        = m_pStaticSettings->overrideCameraOpen;

    // Invalidate all createparam tyoe so that if new enum added which is not used for QTI algo
    // then Logger fucntion is not impacted
    for (UINT i = 0; i < AECAlgoCreateParamTypeCount; i++)
    {
        createParams[i].createParamType = AECAlgoCreateParamsInvalid;
    }

    createParams[AECAlgoCreateParamsLoggerFunctionPtr].createParamType   = AECAlgoCreateParamsLoggerFunctionPtr;
    createParams[AECAlgoCreateParamsLoggerFunctionPtr].pCreateParam      = reinterpret_cast<VOID*>(&StatsLoggerFunction);
    createParams[AECAlgoCreateParamsLoggerFunctionPtr].sizeOfCreateParam = sizeof(StatsLoggingFunction);

    createParams[AECAlgoCreateParamTypeSessionHandle].createParamType   = AECAlgoCreateParamTypeSessionHandle;
    createParams[AECAlgoCreateParamTypeSessionHandle].pCreateParam      = static_cast<VOID*>(pStatsSession);
    createParams[AECAlgoCreateParamTypeSessionHandle].sizeOfCreateParam = sizeof(ChiStatsSession);

    createParams[AECAlgoCreateParamTypeCameraOpenIndicator].createParamType   = AECAlgoCreateParamTypeCameraOpenIndicator;
    createParams[AECAlgoCreateParamTypeCameraOpenIndicator].pCreateParam      = &overrideCameraOpen;
    createParams[AECAlgoCreateParamTypeCameraOpenIndicator].sizeOfCreateParam = sizeof(UINT);

    if (NULL != pCameraInfo)
    {
        createParams[AECAlgoCreateParamTypeCameraInfo].createParamType   = AECAlgoCreateParamTypeCameraInfo;
        createParams[AECAlgoCreateParamTypeCameraInfo].pCreateParam      = static_cast<VOID*>(pCameraInfo);
        createParams[AECAlgoCreateParamTypeCameraInfo].sizeOfCreateParam = sizeof(StatsCameraInfo);
        m_cameraInfo = *pCameraInfo;
    }
    createParamList.numberOfCreateParams = AECAlgoCreateParamTypeCount;
    createParamList.pCreateParamList     = &createParams[0];

    if (NULL != pfnCreate)
    {
        result = (*pfnCreate)(&createParamList, &m_pAECAlgorithm);
    }

    if ((CamxResultSuccess != result) || (NULL == m_pAECAlgorithm) )
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC,
                       "Failed to initialize result: %s, m_pAECAlgorithm: %p, Create pointer: %p",
                       Utils::CamxResultToString(result),
                       m_pAECAlgorithm,
                       reinterpret_cast<VOID*>(pfnCreate));
        result = CamxResultEUnableToLoad;
    }

    // Initialize AEC HAL Params
    m_HALParam.AECompensation = 0;
    m_HALParam.AELock = ControlAELockOff;
    m_HALParam.AEMode = ControlAEModeOn;
    m_HALParam.AEMeteringMode = 0;
    m_HALParam.AETrigger = ControlAEPrecaptureTriggerIdle;
    m_HALParam.AFTrigger = ControlAFTriggerIdle;
    m_HALParam.captureIntent = ControlCaptureIntentPreview;
    m_HALParam.controlMode = ControlModeAuto;
    m_HALParam.controlSceneMode = ControlSceneModeDisabled;
    m_HALParam.flashMode = FlashModeOff;
    m_HALParam.exposureTime = 30000000;
    m_HALParam.sensitivity = 100;
    m_HALParam.AEAntibandingModeValue = ControlAEAntibandingModeOff;
    m_HALParam.ISOExposureTimePriortyMode = ISOExposureTimePriorityMode::DisablePriority;
    m_HALParam.ISOorExposureTimePriorityValue = 0;
    m_HALParam.skipPreflashTriggers = FALSE;
    m_isFlashCaptureIntent = FALSE;
    m_isFixedFocus = FALSE;

    // Initialiize Frame Wait Count, Max frames AF, Max Frames AWB and disable flag under preflash
    m_preFlashMaxFrameWaitLimitAF  = m_pStaticSettings->preFlashMaxFrameWaitLimitAF;
    m_preFlashMaxFrameWaitLimitAWB = m_pStaticSettings->preFlashMaxFrameWaitLimitAWB;
    m_disableAFAWBpreFlash         = m_pStaticSettings->disableAFAWBpreFlash;
    m_precaptureWaitFrames         = 0;

    // flag setting whether dynamic inline calibration feature is enabled or not
    m_isDynamicInlineCalibration   = m_pStaticSettings->dynamicInlineCalibration;

    // Set the calibration mode once at initialization time depending on the settings
    switch(m_pStaticSettings->dualLEDCalibrationMode)
    {
        case DualLEDCalibrationTuning:
            m_LEDCalibration.mode = AECAlgoFlashMeasKindTuning;
            break;
        case DualLEDCalibrationCalibration:
            m_LEDCalibration.mode = AECAlgoFlashMeasKindCalib;
            break;
        case DualLEDCalibrationDisabled:
        default:
            m_LEDCalibration.mode = AECAlgoFlashMeasKindOff;
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::Uninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECEngine::Uninitialize(
    UINT overrideCameraClose)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    if (NULL != m_pAECAlgorithm)
    {
        AECAlgoDestroyParamList destroyParamList                        = { 0 };
        AECAlgoDestroyParam     destroyParams[AECDestroyParamTypeCount] = {};
        StatsCameraInfo         cameraInfo                              = m_cameraInfo;

        destroyParams[AECDestroyParamTypeCameraCloseIndicator].destroyParamType = AECDestroyParamTypeCameraCloseIndicator;
        destroyParams[AECDestroyParamTypeCameraCloseIndicator].pParam = static_cast<VOID*>(&overrideCameraClose);
        destroyParams[AECDestroyParamTypeCameraCloseIndicator].sizeOfParam = sizeof(UINT);

        destroyParams[AECDestroyParamTypeCameraInfo].destroyParamType = AECDestroyParamTypeCameraInfo;
        destroyParams[AECDestroyParamTypeCameraInfo].pParam = static_cast<VOID*>(&cameraInfo);
        destroyParams[AECDestroyParamTypeCameraInfo].sizeOfParam = sizeof(cameraInfo);

        destroyParamList.paramCount = AECDestroyParamTypeCount;
        destroyParamList.pParamList = &destroyParams[0];

        m_pAECAlgorithm->AECDestroy(m_pAECAlgorithm, &destroyParamList);
        m_pAECAlgorithm = NULL;
    }

    if (NULL != m_hHandle)
    {
        OsUtils::LibUnmap(m_hHandle);
        m_hHandle = NULL;
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::HandleCommand
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::HandleCommand(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
    CamxResult result = CamxResultSuccess;
    AECAlgoFPSRange FPSRange;

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AEC: Command = %s EngineState = %s, PreflashState = %s",
                     CamxAECEngineCommandStrings[command < AECCommand::AECCommandMax ?
                        static_cast<UINT8>(command) : static_cast<UINT8>(AECCommand::AECCommandMax)],
                     CamxAECEngineStateStrings[static_cast<UINT8>(m_AECState)],
                     CamxAECPreFlashStateStrings[static_cast<UINT8>(m_AECPreFlashState)]);

    switch (command)
    {
        case AECCommand::StartDriver:
            result = StartDriver(pInput, pOutput);
            break;
        case AECCommand::StopDriver:
            result = StopDriver(pInput, pOutput);
            break;
        case AECCommand::ConfigDriver:
            result = ConfigDriver(pInput, pOutput);
            break;
        case AECCommand::SetChromatix:
            result = SetChromatix(pInput, pOutput);
            break;
        case AECCommand::StartStreaming:
            result = StartStreaming(pInput, pOutput->pFrameControl);
            break;
        case AECCommand::ProcessStats:
            result = ProcessStats(pInput, pOutput);
            break;
        case AECCommand::SetPerFrameControlParam:
            result = SetPerFrameControlParam(pInput, pOutput);
            break;
        case AECCommand::SetNodesUpdate:
            result = SetNodesUpdate(pInput, pOutput);
            break;
        case AECCommand::ProcessHardwareInfo:
            result = ProcessHardwareInfo(pInput, pOutput);
            break;
        case AECCommand::ProcessCropWindow:
            result = ProcessCropWindow(pInput, pOutput);
            break;
        case AECCommand::GetVendorTagFromAlgo:
            result = GetParamForVendorTag(pInput, pOutput);
            break;
        case AECCommand::GetPubVendorTagFromAlgo:
            result = GetParamForPubVendorTag(pInput, pOutput);
            break;
        case AECCommand::GetDefaultValues:
            result = UpdateAEEngineOutputResults(pOutput);
            break;
        case AECCommand::SetPipelineDelay:
            m_maxPipelineDelay = pInput->systemLatency;
            result = SetSingleParamToAlgorithm(AECAlgoSetParamPipelineDelay,
                &pInput->systemLatency,
                sizeof(pInput->systemLatency));
            break;
        case AECCommand::GetLEDCalibrationConfig:
            result = GetLEDCalibrationConfig();
            break;
        case AECCommand::LoadLEDCalibrationData:
            result = LoadLEDCalibrationData();
            break;
        case AECCommand::LoadLEDInlineCalibrationData:
            CAMX_UNREFERENCED_PARAM(pInput);
            CAMX_UNREFERENCED_PARAM(pOutput);
            result = LoadLEDInlineCalibrationData();
            break;
        case AECCommand::SetDCCalibrationData:
            result = SetSingleParamToAlgorithm(AECAlgoSetParamDualcamCalibrationInfo,
                &pInput->dcCalibrationInfo,
                sizeof(pInput->dcCalibrationInfo));
            break;
        case AECCommand::ProcessGYROStats:
            result = SetSingleParamToAlgorithm(AECAlgoSetParamGyroInfo, &pInput->gyroInfo, sizeof(StatsGyroInfo));
            break;
        case AECCommand::SetCameraInformation:
            memcpy(&m_cameraInfo, &pInput->pAlgorithmInput->cameraInfo, sizeof(StatsCameraInfo));
            break;
        case AECCommand::SetFPSRange:
            FPSRange.minimumFPS = static_cast<FLOAT>(pInput->pHALParam->FPSRange.min);
            FPSRange.maximumFPS = static_cast<FLOAT>(pInput->pHALParam->FPSRange.max);
            result = SetSingleParamToAlgorithm(AECAlgoSetParamFPSRange, &FPSRange, sizeof(AECAlgoFPSRange));
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid AEC command: %d", command);
            result = CamxResultEUnsupported;
            break;
    }

    if (CamxResultSuccess == result)
    {
        result = AECStateMachine(command, pInput, pOutput);
    }

    // We need to update the engine output only after processing of commands and state machine changes
    if (AECCommand::ProcessStats == command)
    {
        BOOL bIsPreflashComplete = IsPreflashComplete();

        // In case if preflash completed, we need to go get main flash snapshot gains and restore algo to run in normal mode
        if (TRUE == bIsPreflashComplete)
        {
            RestoreStreaming(pOutput);
        }

        UpdateAEEngineOutputResults(pOutput);

        /* After updating the engine output, we need to reset the engine back to normal if preflash completed */
        if (TRUE == bIsPreflashComplete)
        {
            ResetAEState();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::AECStateMachine
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::AECStateMachine(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CamxResult result = CamxResultSuccess;

    switch (m_AECState)
    {
        case AECState::Inactive:
            result = AECStateMachineInactive(command, pInput, pOutput);
            break;
        case AECState::Manual:
            result = AECStateMachineManual(command, pInput, pOutput);
            break;
        case AECState::Converging:
        case AECState::Converged:
            result = AECStateMachineConvergedAndConverging(command, pInput, pOutput);
            break;
        case AECState::Flash:
            result = AECStateMachineFlash(command, pInput, pOutput);
            break;
        case AECState::LEDCalibration:
            result = AECStateMachineLEDCalibration(command, pInput, pOutput);
            break;
        default:
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid AEC state: %d", m_AECState);
            result = CamxResultEInvalidState;
            break;
    }

    /// AEC Convergence time in ms
    /// If the AEC search is converged after search get the current time and
    /// take difference to measure total AEC engine Convergence time
    if (m_AECLastState == AECState::Converging && m_AECState == AECState::Converged)
    {
        UINT32 AECConvergenceEndTime;
        CamxTime pTime;
        OsUtils::GetTime(&pTime);
        AECConvergenceEndTime = OsUtils::CamxTimeToMillis(&pTime);
        UINT32 AECConvergenceTime = AECConvergenceEndTime - m_AECConvergenceStartTime;
        CAMX_LOG_INFO(CamxLogGroupAEC, "KPI: AEC Engine Convergence done in %u ms StartTime %u EndTime %u",
                      AECConvergenceTime, m_AECConvergenceStartTime, AECConvergenceEndTime );
    }
    /// If the Current AEC state goes into search state get the current time
    if (m_AECLastState != AECState::Converging && m_AECState == AECState::Converging)
    {
        CamxTime pTime;
        OsUtils::GetTime(&pTime);
        m_AECConvergenceStartTime = OsUtils::CamxTimeToMillis(&pTime);
    }
    m_AECLastState = m_AECState;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::AECStateMachineInactive
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::AECStateMachineInactive(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutput);

    /// valid state transition in Inactive state
    /// INACTIVE
    ///     START STREAMING -> CONVERGING   INPUT
    CamxResult result = CamxResultSuccess;

    switch (command)
    {
        case AECCommand::ConfigDriver:
            // configure the driver in preview mode
            break;
        case AECCommand::ProcessStats:
            // check if the output from algo
            // . converging/converged
            break;
        case AECCommand::SetPerFrameControlParam:
            // check if the output from algo
            // . lock
            // . manual
            break;
        case AECCommand::StartStreaming:
            // config the driver in preview mode
            SetAECState(AECState::Converging);
            break;
        case AECCommand::GetDefaultValues:
            // Do nothing.
            break;
        default:
            // other command won't change the AECState
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::AECStateMachineManual
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::AECStateMachineManual(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_UNREFERENCED_PARAM(pOutput);

    /// valid transition in manual mode
    /// MANUAL
    ///     TO AUTO -> CONVERGING   INPUT
    ///     STOP    -> INACTIVE     INPUT
    switch (command)
    {
        case AECCommand::ProcessStats:
            // look algorithm mode to determine if manual is cancelled
            pOutput->pProcessStatsOutput->algorithmOutput.engineFrameControl.exposureData[ExposureIndexShort].exposureTime
                = m_HALParam.exposureTime;
            pOutput->pProcessStatsOutput->algorithmOutput.engineFrameControl.exposureData[ExposureIndexLong].exposureTime
                = m_HALParam.exposureTime;
            pOutput->pProcessStatsOutput->algorithmOutput.engineFrameControl.exposureData[ExposureIndexSafe].exposureTime
                = m_HALParam.exposureTime;
            CAMX_LOG_INFO(CamxLogGroupAEC, " HAL exposureTime: %ld", m_HALParam.exposureTime);
            break;
        case AECCommand::StopDriver:
            // set the AEC State back to inactive mode
            break;
        case AECCommand::SetPerFrameControlParam:
        {
            // look control parameter to determine the manual is cancelled
            AECAlgoModeType algoMode = CAECEngineUtility::GetAECAlgorithmMode(pInput->pHALParam);
            if (AECAlgoModeManualFull != algoMode)
            {
                SetAECState(AECState::Converging);
            }
            else if (FlashModeSingle == pInput->pHALParam->flashMode)
            {
                // 1. Flash starting because of AF trigger or Touch to Focus
                if ((ControlAFTriggerStart == pInput->pHALParam->AFTrigger) && IsLEDAFNeeded())
                {
                    SetFlashTrigger(CAECEngine::FlashTrigger::LEDAF);
                }
                // 2. Flash starting because of AE precapture sequence
                else if (ControlAEPrecaptureTriggerStart == pInput->pHALParam->AETrigger)
                {
                    SetFlashTrigger(CAECEngine::FlashTrigger::AE);
                }
                else if (ControlAEPrecaptureTriggerCancel == pInput->pHALParam->AETrigger)
                {
                    CancelPreFlash();
                }

                if (GetFlashTrigger() != CAECEngine::FlashTrigger::Invalid)
                {
                    m_isPrecaptureInProgress = TRUE;
                    SetAECState(AECState::Flash); // start the flash state machine
                    SetAECPreFlashState(PreFlashStateStart);
                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AEC_DEBUG_CAPTURE: Precapture Started");
                }
            }
            break;
        }
        case AECCommand::GetDefaultValues:
            // Do nothing.
            break;
        default:
            // other command won't change the AECState
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::AECStateMachineConvergedAndConverging
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::AECStateMachineConvergedAndConverging(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CamxResult result = CamxResultSuccess;

    /// valid transition in converged state
    /// CONVERGED
    ///    SEARCHING   -> CONVERGING               ALGO
    ///    SETTLED     -> CONVERGED                ALGO
    ///    MANUAL      -> MANUAL                   INPUT
    ///    LOCK        -> LOCK                     INPUT/ALGO
    ///    TRIGGER     -> FLASH (LOW LIGHT)        INPUT
    ///                -> NO OPS (NO LED NEEDED)
    ///    STOP        -> INACTIVE INPUT           INPUT
    switch (command)
    {
        case AECCommand::SetPerFrameControlParam:
        // look for
        // . trigger
        // . lock
        // . in manual mode
        {
            AECAlgoModeType algoMode = CAECEngineUtility::GetAECAlgorithmMode(pInput->pHALParam);

            if (AECAlgoModeManualFull == algoMode)
            {
                SetAECState(AECState::Manual);

                // If manual mode and Control Triggers are set in the same frame, we can directly move to flash state
                if (FlashModeSingle == pInput->pHALParam->flashMode)
                {
                    // 1. Flash starting because of AF trigger or Touch to Focus
                    if ((ControlAFTriggerStart == pInput->pHALParam->AFTrigger) && IsLEDAFNeeded())
                    {
                        SetFlashTrigger(CAECEngine::FlashTrigger::LEDAF);
                    }
                    // 2. Flash starting because of AE precapture sequence
                    else if (ControlAEPrecaptureTriggerStart == pInput->pHALParam->AETrigger)
                    {
                        SetFlashTrigger(CAECEngine::FlashTrigger::AE);
                    }
                    else if (ControlAEPrecaptureTriggerCancel == pInput->pHALParam->AETrigger)
                    {
                        CancelPreFlash();
                    }

                    if (GetFlashTrigger() != CAECEngine::FlashTrigger::Invalid)
                    {
                        // start the flash state machine
                        SetAECState(AECState::Flash);
                        SetAECPreFlashState(PreFlashStateStart);
                    }
                }
            }
            // Preflash triggers received for request id less than max pipeline delay will be ignored
            // as AEC will not have valid stats till max pipeline delay. Also, only run preflash sequence
            // on master camera.
            else if (FALSE == pInput->pHALParam->skipPreflashTriggers && m_cameraInfo.algoRole != StatsAlgoRoleSlave)
            {
                // Check if Flash is enabled from HAL/framework
                if (TRUE == IsHalPreFlashSettingEnabled())
                {
                    // 1. Flash starting because of AF trigger or Touch to Focus
                    if ((ControlAFTriggerStart == pInput->pHALParam->AFTrigger) && IsLEDAFNeeded() &&
                        (TRUE == IsTouchFlashEnabled()))
                    {
                        SetFlashTrigger(CAECEngine::FlashTrigger::LEDAF);
                    }
                    // 2. Flash starting because of AE precapture sequence
                    else if (ControlAEPrecaptureTriggerStart == pInput->pHALParam->AETrigger)
                    {
                        SetFlashTrigger(CAECEngine::FlashTrigger::AE);
                    }
                    else if (ControlAEPrecaptureTriggerCancel == pInput->pHALParam->AETrigger)
                    {
                        CancelPreFlash();
                    }
                }

                if (GetFlashTrigger() != CAECEngine::FlashTrigger::Invalid)
                {
                    m_isPrecaptureInProgress = TRUE;
                    SetAECState(AECState::Flash); // start the flash state machine
                    SetAECPreFlashState(PreFlashStateStart);
                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AEC_DEBUG_CAPTURE: Precapture Started");
                }
                else if (ControlAEPrecaptureTriggerStart == pInput->pHALParam->AETrigger)
                {
                    m_isPrecaptureInProgress = TRUE;
                }
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupAEC, "Skip preflash triggers till max pipeline delay");
            }
            m_HALParam.AEMode = pInput->pHALParam->AEMode;
            break;
        }
        case AECCommand::StopDriver:
            // set the AEC State back to inactive mode
            SetAECState(AECState::Inactive);
            break;
        case AECCommand::ProcessStats:
            if (IsAECSettled(pOutput->pProcessStatsOutput))
            {
                if (AECAlgoFlashMeasKindOff != m_LEDCalibration.mode)
                {
                    // LED Calibration should only start once AEC is converged
                    SetAECState(AECState::LEDCalibration);
                    SetLEDCalibrationState(LEDCalibrationState::Ready);
                }
                else
                {
                    SetAECState(AECState::Converged);
                }
            }
            else
            {
                SetAECState(AECState::Converging);
            }
            break;
        case AECCommand::GetDefaultValues:
            // Do nothing.
            break;
        default:
            // other command won't change the AECState
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::AECStateMachineFlash
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::AECStateMachineFlash(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CamxResult result = CamxResultSuccess;

    switch (m_AECPreFlashState)
    {
        case PreFlashStateInactive:
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "PreFlash State Inactive: Not handled AECPreFlashState %d Command %d",
                          m_AECPreFlashState, command);
            break;
        case PreFlashStateStart:
            result = AECStateMachineFlashStart(command, pInput, pOutput);
            break;
        case PreFlashStateTriggerFD:
            result = AECStateMachineFlashTriggerFD(command, pInput, pOutput);
            break;
        case PreFlashStateTriggerAF:
            result = AECStateMachineFlashTriggerAF(command, pInput, pOutput);
            break;
        case PreFlashStateTriggerAWB:
            result = AECStateMachineFlashTriggerAWB(command, pInput, pOutput);
            break;
        case PreFlashStateCompleteLED:
            result = AECStateMachineFlashCompleteLED(command, pInput, pOutput);
            break;
        case PreFlashStateCompleteNoLED:
            result = AECStateMachineFlashCompleteNoLED(command, pInput, pOutput);
            break;
        case PreFlashStateRER:
            result = AECStateMachineFlashRER(command, pInput, pOutput);
            break;
        default:
            CAMX_LOG_INFO(CamxLogGroupAEC, "Not handled AECPreFlashState %d Command %d", m_AECPreFlashState, command);
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::AECStateMachineLEDCalibration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::AECStateMachineLEDCalibration(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CamxResult result = CamxResultSuccess;
    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutput);

    switch (m_LEDCalibrationState)
    {
        case LEDCalibrationState::Ready:
            // By now flash should be requested to be ON and LED currents configured - advance to the next state
            // so that we can collect measurements on the next frame
            // m_LEDCalibrationState = LEDCalibrationState::Collecting;
            result = CamxResultSuccess;
            break;
        case LEDCalibrationState::Collecting:
            result = CamxResultSuccess;
            break;
        case LEDCalibrationState::Complete:
            // AE State was converged before starting calibration. Now that we are done, set the state back to converged
            SetAECState(AECState::Converged);
            result = CamxResultSuccess;
            break;
        default:
            CAMX_LOG_INFO(CamxLogGroupAEC, "Not handled AECLEDCalibration state %d Command %d", m_AECPreFlashState, command);
            break;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::AECStateMachineFlashStart
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngine::AECStateMachineFlashStart(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    /// PreFlashStateStart
    /// AE/AF TRIGGER CANCEL    -> PreFlashStateCompleteNoLED                   INPUT
    /// AF TRIGGER              -> Update flash mode to LED AF                    INPUT
    /// AEC SETTLED             -> PreFlashStateTriggerFD/TriggerAF/TriggerAWB  AEC ALGO
    /// STOP DRIVER             -> PreFlashStateInactive, AECState::Inactive    INPUT
    switch (command)
    {
        case AECCommand::StopDriver:
            // set the AEC State back to inactive mode
            // @todo (CAMX-1302) algo state clean up
            SetAECState(AECState::Inactive);
            SetAECPreFlashState(PreFlashStateInactive);
            break;
        case AECCommand::SetPerFrameControlParam:
            // check
            // . trigger cancel
            // . AF trigger
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "PreFlashState: FrameControlParam: AFTrigger=%d AETrigger=%d AELock=%d",
                pInput->pHALParam->AFTrigger,
                pInput->pHALParam->AETrigger,
                pInput->pHALParam->AELock);
            if (ControlAFTriggerCancel == pInput->pHALParam->AFTrigger ||
                ControlAEPrecaptureTriggerCancel == pInput->pHALParam->AETrigger)
            {
                CancelPreFlash();
            }
            else if (ControlAFTriggerStart == pInput->pHALParam->AFTrigger)
            {
                SetFlashTrigger(CAECEngine::FlashTrigger::LEDAF);
            }
            break;
        case AECCommand::ProcessStats:
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "In PreFlashState: ProcessStats: AFTrigger=%d AETrigger=%d",
                            m_HALParam.AFTrigger, m_HALParam.AETrigger);
            if (ControlAFTriggerIdle           == m_HALParam.AFTrigger &&
                ControlAEPrecaptureTriggerIdle == m_HALParam.AETrigger)
            {
                // check AEC settle
                if (IsAECSettled(pOutput->pProcessStatsOutput))
                {
                    GetFlashSnapshotGains();
                    m_AECPreFlashSkipChecking = FALSE;
                    TransitToNextPreFlashState(PreFlashStateStart);
                }
            }
            break;
        default:
            // other command won't change the PreFlashState
            break;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::AECStateMachineFlashTriggerFD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngine::AECStateMachineFlashTriggerFD(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CAMX_UNREFERENCED_PARAM(command);
    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutput);

    /// @todo (CAMX-1302) finish the state transit
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::AECStateMachineFlashTriggerAF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngine::AECStateMachineFlashTriggerAF(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);

    /// PreFlashStateTriggerAF
    /// AE/AF TRIGGER CANCEL    -> PreFlashStateCompleteNoLED                 INPUT
    /// AF DONE                 -> PreFlashStateTriggerAWB                    AF ALGO
    /// STOP DRIVER             -> PreFlashStateInactive, AECState::Inactive  INPUT
    switch (command)
    {
        case AECCommand::StopDriver:
            // set the AEC State back to inactive mode
            // @todo (CAMX-1302) algo state clean up
            SetAECState(AECState::Inactive);
            SetAECPreFlashState(PreFlashStateInactive);
            break;
        case AECCommand::SetPerFrameControlParam:
            // check
            // . trigger cancel
            // . AF trigger
            if (ControlAFTriggerCancel == pInput->pHALParam->AFTrigger ||
                ControlAEPrecaptureTriggerCancel == pInput->pHALParam->AETrigger)
            {
                CancelPreFlash();
            }
            break;
        case AECCommand::SetNodesUpdate:
            if (IsAFSettled(pInput->pNodesUpdate))
            {
                TransitToNextPreFlashState(PreFlashStateTriggerAF);

                // AWB gets triggered in (N+1)th frame and SetNodesUpdate reads data of Nth frame
                // hence need to skip one frame to check correct status of AWB
                // so that AEC preflash state should not move to next state.
                m_AECPreFlashSkipChecking = TRUE;
                m_preflashFrameWaitCount = 0;
            }
            else
            {
                BOOL timedOut = FALSE;
                timedOut = TimedWaitForSettle(m_preFlashMaxFrameWaitLimitAF);
                CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AF Settle count = %d", m_preflashFrameWaitCount);
                if (TRUE == timedOut)
                {
                    m_preflashFrameWaitCount = 0;
                    TransitToNextPreFlashState(PreFlashStateTriggerAF);

                    // AWB gets triggered in (N+1)th frame and SetNodesUpdate reads data of Nth frame
                    // hence need to skip one frame to check correct status of AWB
                    // so that AEC preflash state should not move to next state.
                    m_AECPreFlashSkipChecking = TRUE;
                    CAMX_LOG_WARN(CamxLogGroupAEC, "AF timed out");
                }
            }
            break;
        default:
            // other command won't change the PreFlashState
            break;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::AECStateMachineFlashTriggerAWB
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngine::AECStateMachineFlashTriggerAWB(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);

    /// PreFlashStateTriggerAWB
    /// AE/AF TRIGGER CANCEL    -> PreFlashStateCompleteNoLED                 INPUT
    /// AWB DONE                -> PreFlashStateCompleteLED                   AWB ALGO
    /// STOP DRIVER             -> PreFlashStateInactive, AECState::Inactive  INPUT
    BOOL timedOut = FALSE;
    switch (command)
    {
        case AECCommand::StopDriver:
            // set the AEC State back to inactive mode
            // @todo (CAMX-1302) algo state clean up
            SetAECState(AECState::Inactive);
            SetAECPreFlashState(PreFlashStateInactive);
            break;
        case AECCommand::SetPerFrameControlParam:
            // check
            // . trigger cancel
            // . AF trigger
            if (ControlAFTriggerCancel == pInput->pHALParam->AFTrigger ||
                ControlAEPrecaptureTriggerCancel == pInput->pHALParam->AETrigger)
            {
                CancelPreFlash();
            }
            break;
        case AECCommand::SetNodesUpdate:
            if (IsAWBSettled(pInput->pNodesUpdate))
            {
                TransitToNextPreFlashState(PreFlashStateTriggerAWB);
                m_preflashFrameWaitCount = 0;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AWB Settle count = %d", m_preflashFrameWaitCount);
                timedOut = TimedWaitForSettle(m_preFlashMaxFrameWaitLimitAWB);
                if (TRUE == timedOut)
                {
                    m_preflashFrameWaitCount = 0;
                    TransitToNextPreFlashState(PreFlashStateTriggerAWB);
                }
            }
            break;
        default:
            // other command won't change the PreFlashState
            break;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::AECStateMachineFlashCompleteLED
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngine::AECStateMachineFlashCompleteLED(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CAMX_UNREFERENCED_PARAM(command);
    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutput);

    /// PreFlashStateCompleteLED
    /// AE/AF TRIGGER CANCEL    -> PreFlashStateCompleteNoLED                 INPUT
    /// SNAPSHOT REQUEST        -> PreFlashStateInactive                      INPUT
    /// STOP DRIVER             -> PreFlashStateInactive, AECState::Inactive  INPUT
    switch (command)
    {
        case AECCommand::StopDriver:
            // set the AEC State back to inactive mode
            // @todo (CAMX-1302) algo state clean up
            SetAECState(AECState::Inactive);
            SetAECPreFlashState(PreFlashStateInactive);
            break;
        case AECCommand::SetPerFrameControlParam:
        {
            // Check if RER is required
            if (ControlAEModeOnAutoFlashRedeye == pInput->pHALParam->AEMode)
            {
                TransitToNextPreFlashState(PreFlashStateCompleteLED);
            }
            else
            {
                // check
                // . trigger cancel
                // . AF trigger
                AECAlgoModeType algoMode = CAECEngineUtility::GetAECAlgorithmMode(pInput->pHALParam);
                CAMX_LOG_VERBOSE(CamxLogGroupAEC, "In PreFlashStateComplete: SetPerFrameControlParam: AELock = %d",
                    pInput->pHALParam->AELock);
                if (AECAlgoModeManualFull == algoMode)
                {
                    SetAECState(AECState::Manual);
                }

                if (ControlAFTriggerCancel == pInput->pHALParam->AFTrigger ||
                    ControlAEPrecaptureTriggerCancel == pInput->pHALParam->AETrigger)
                {
                    CancelPreFlash();
                }
            }
            break;
        }
        default:
            // other command won't change the PreFlashState
            break;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::AECStateMachineFlashCompleteNoLED
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngine::AECStateMachineFlashCompleteNoLED(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CAMX_UNREFERENCED_PARAM(pInput);
    CAMX_UNREFERENCED_PARAM(pOutput);

    /// PreFlashStateCompleteNoLED
    /// ON NEXT STATS(TBD)      -> PreFlashStateInactive                      INPUT
    /// STOP DRIVER             -> PreFlashStateInactive, AECState::Inactive  INPUT
    switch (command)
    {
        case AECCommand::StopDriver:
            // set the AEC State back to inactive mode
            // @todo (CAMX-1302) algo state clean up
            SetAECState(AECState::Inactive);
            SetAECPreFlashState(PreFlashStateInactive);
            break;
        case AECCommand::ProcessStats:
            // Reset the AE State after updating AE engine Output. Do nothing here
            break;
        default:
            // other command won't change the PreFlashState
            break;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::AECStateMachineFlashRER
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngine::AECStateMachineFlashRER(
    AECCommand              command,
    AECCommandInputParam*   pInput,
    AECCommandOutputParam*  pOutput)
{
    CAMX_UNREFERENCED_PARAM(pOutput);

    CamxResult result = CamxResultSuccess;

    switch (command)
    {
        case AECCommand::StopDriver:
            // set the AEC State back to inactive mode
            // @todo (CAMX-1302) algo state clean up
            SetAECState(AECState::Inactive);
            SetAECPreFlashState(PreFlashStateInactive);
            break;

        case AECCommand::SetPerFrameControlParam:
        {
            AECAlgoModeType algoMode = CAECEngineUtility::GetAECAlgorithmMode(pInput->pHALParam);
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "In PreFlashStateRER SetPerFrameControlParam: AELock = %d",
                pInput->pHALParam->AELock);
            if (AECAlgoModeManualFull == algoMode)
            {
                SetAECState(AECState::Manual);
            }

            if (ControlAFTriggerCancel == pInput->pHALParam->AFTrigger ||
                ControlAEPrecaptureTriggerCancel == pInput->pHALParam->AETrigger)
            {
                CancelPreFlash();
            }
            break;
        }

        case AECCommand::SetNodesUpdate:
            // Do Nothing here. Reset the AE state after updating the Engine Output
            break;
        default:
            break;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::ProcessStats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::ProcessStats(
    AECCommandInputParam*   pCommandInput,
    AECCommandOutputParam*  pCommandOutput)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CamxResult result = CamxResultSuccess;

    if (TRUE == IsCaptureRequest(&m_HALParam))
    {
        // For non ZSL and QCFA snapshot, requestIDoffset will reset to 1 and snapshot indicator will be reset.
        // So snapshot indicator need to be updated from algo for first request ID from last flush.
        if (FirstValidRequestId == pCommandInput->pAlgorithmInput->requestIdOffsetFromLastFlush)
        {
            AECAlgoSnapshotType snapshotType = AECAlgoSnapshotMax;
            GetAlgoParamForSnapshotType((VOID*)&snapshotType);
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "snapshotType %d", snapshotType);
            m_snapshotIndicator = snapshotType;
        }

        // Check it is flash snapshot or normal
        if ((AECAlgoSnapshotFlash == m_snapshotIndicator) && (TRUE == IsFlashGainsAvailable()))
        {
            CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupAEC, 0, "Flash: CI");
            // We need to skip "processing of stats" for number of frames after main flash
            m_numberOfFramesToSkip = m_pStaticSettings->numberOfFramesToSkip;

            /* May be we need to skip stats processing here during main flash event */
            CAMX_LOG_INFO(CamxLogGroupAEC, "AEC_DEBUG_CAPTURE: Flash Snapshot - RequestId=%llu: "
                "===================================", pCommandInput->pAlgorithmInput->requestId);

            // It means we are in Main flash firing state and we want to avoid processing stats
            // Post same last gains here + MAIN Flash gains stored earlier
            pCommandOutput->pProcessStatsOutput->algorithmOutput                    = m_algoLastOutput;
            pCommandOutput->pProcessStatsOutput->algorithmOutput.engineFrameControl = m_flashFrameControl;
            m_algoLastOutput.engineFrameControl                                     = m_flashFrameControl;
            GetAECDebugData(pCommandInput, pCommandOutput);

            DumpAEEngineOutputResults("AEFC:MainFlashSnapshot", &pCommandOutput->pProcessStatsOutput->algorithmOutput);
            m_isFlashCaptureIntent = TRUE;
            CAMX_TRACE_ASYNC_END_F(CamxLogGroupAEC, 0, "Flash: CI");
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupAEC, "AEC_DEBUG_CAPTURE: Normal Snapshot - RequestId=%llu: "
                "===================================", pCommandInput->pAlgorithmInput->requestId);

            result = ProcessStatsAlgo(pCommandInput, pCommandOutput);
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupAEC, "Failed to process stats %s", Utils::CamxResultToString(result));
            }

            // In ZSL mode, AEC shouldn't query snapshot exposure instead it should use preview exposure & gain
            // In HDR, it should query snapshot exposure
            if (IsQuerySnapshotExposure() || IsZSLDisabled())
            {
                result = ProcessStatsCaptureRequest(pCommandInput, pCommandOutput);
                DumpAEEngineOutputResults("AEFC:NormalSnapshot", &pCommandOutput->pProcessStatsOutput->algorithmOutput);

                // after exposure request for snapshot: update debug data as exposure settings may differ from preview
                GetAECDebugData(pCommandInput, pCommandOutput);
            }
        }
    }
    else
    {
        if (m_numberOfFramesToSkip == 0)
        {
            if (TRUE == m_isFlashCaptureIntent)
            {
                m_isFlashCaptureIntent = FALSE;
                Utils::Memset(&m_flashFrameControl, 0, sizeof(m_flashFrameControl));
                CAMX_LOG_VERBOSE(CamxLogGroupAEC, "clear flash frame control info");
            }
            result = ProcessStatsAlgo(pCommandInput, pCommandOutput);
            // Store the algorithm output to use during main flash
            Utils::Memcpy(&m_algoLastOutput,
                &pCommandOutput->pProcessStatsOutput->algorithmOutput, sizeof(AECEngineAlgorithmOutput));
            DumpAEEngineOutputResults("AEFC:Preview", &pCommandOutput->pProcessStatsOutput->algorithmOutput);
        }
        else
        {
            // It means we are in pre flash complete state and we want to avoid processing stats
            // Post same last gains here
            Utils::Memcpy(&pCommandOutput->pProcessStatsOutput->algorithmOutput,
                &m_algoLastOutput, sizeof(AECEngineAlgorithmOutput));
            GetAECDebugData(pCommandInput, pCommandOutput);
            DumpAEEngineOutputResults("AEFC:RestoreGains", &pCommandOutput->pProcessStatsOutput->algorithmOutput);
            m_numberOfFramesToSkip--;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::ProcessStatsAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::ProcessStatsAlgo(
    AECCommandInputParam*   pCommandInput,
    AECCommandOutputParam*  pCommandOutput)
{
    CamxResult              result = CamxResultSuccess;
    BOOL                    startingPreflash = FALSE;

    // input params
    AECAlgoInputList        algorithmInputList;
    AECAlgoInput            algorithmInput[AECAlgoInputCount];
    AECAlgoInputStatsInfo   statsInfo                           = {0};

    // output params
    AECAlgoOutputList           algorithmOutputList;
    AECAlgoOutput               algorithmOutput[AECAlgoOutputCount];
    AECAlgoExposureSet          algorithmExposureSet;
    AECAlgoExposureInfo         algorithmExposureInfo[AECAlgoExposureCount];
    AECEngineAlgorithmOutput*   pEngineOutput = &(pCommandOutput->pProcessStatsOutput->algorithmOutput);
    StatsVendorTagList*         pVendorTagList = &(pCommandOutput->pProcessStatsOutput->vendorTagList);

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Processing Stats for ReqId = %llu requestIdOffsetFromLastFlush  = %llu",
        pCommandInput->pAlgorithmInput->requestId, pCommandInput->pAlgorithmInput->requestIdOffsetFromLastFlush);

    // Configure the AECAlgoOutputExposureSets
    algorithmExposureSet.pExposureInfo              = &algorithmExposureInfo[0];
    algorithmExposureSet.exposureInfoCount          = AECAlgoExposureCount;
    algorithmExposureSet.exposureInfoSize           = sizeof(AECAlgoExposureInfo);
    algorithmExposureSet.exposureInfoOutputCount    = 0;
    algorithmExposureSet.exposureInfoOutputSize     = 0;

    // Initialize the Input/Output
    CAECEngineUtility::InitializeAlgorithmInput(&algorithmInput[0], AECAlgoInputCount);

    // Set AECAlgoInputStatisticInfo
    statsInfo.requestId = pCommandInput->pAlgorithmInput->requestId;
    statsInfo.bIsTorchAEC = pCommandInput->pAlgorithmInput->bIsTorchAEC;
    // Received the trigger and in PreFlash Init state, set the prepareBeforePreflash
    if ((FALSE                             != m_isPrecaptureInProgress) &&
        (CAECEngine::FlashTrigger::Invalid != GetFlashTrigger())        &&
        (PreFlashStateStart                == GetAECPreFlashState()))
    {
        startingPreflash = TRUE;
        statsInfo.prepareBeforePreflash = TRUE;
    }

    if (NULL != m_pStaticSettings)
    {

        if (m_maxPipelineDelay < pCommandInput->pAlgorithmInput->requestIdOffsetFromLastFlush)
        {
            pCommandInput->pAlgorithmInput->bayerGrid.startupMode   = StatisticsStartUpValid;
            pCommandInput->pAlgorithmInput->bayerHist.startupMode   = StatisticsStartUpValid;
            pCommandInput->pAlgorithmInput->hdrBHist.startupMode    = StatisticsStartUpValid;
            pCommandInput->pAlgorithmInput->imgHist.startupMode     = StatisticsStartUpValid;
        }
        else
        {
            pCommandInput->pAlgorithmInput->bayerGrid.startupMode   = StatisticsStartUpInvalid;
            pCommandInput->pAlgorithmInput->bayerHist.startupMode   = StatisticsStartUpInvalid;
            pCommandInput->pAlgorithmInput->hdrBHist.startupMode    = StatisticsStartUpInvalid;
            pCommandInput->pAlgorithmInput->imgHist.startupMode     = StatisticsStartUpInvalid;
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Initial Frames : StatisticsStartUpInvalid");
        }

        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                  AECAlgoInputStatisticInfo,
                                                  sizeof(AECAlgoInputStatsInfo),
                                                  &statsInfo);

        // Set AECAlgoInputBayerGrid
        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                  AECAlgoInputBayerGrid,
                                                  sizeof(StatsBayerGrid),
                                                  &pCommandInput->pAlgorithmInput->bayerGrid);

        // Set AECAlgoInputBayerGridROI
        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                  AECAlgoInputBayerGridROI,
                                                  sizeof(StatsRectangle),
                                                  &pCommandInput->pAlgorithmInput->bayerGridROI);

        // Set AECAlgoInputBayerHist
        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                  AECAlgoInputBayerHist,
                                                  sizeof(StatsBayerHist),
                                                  &pCommandInput->pAlgorithmInput->bayerHist);
        // Set AECAlgoInputIHist
        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                 AECAlgoInputIHist,
                                                 sizeof(StatsIHist),
                                                 &pCommandInput->pAlgorithmInput->imgHist);

        // Set AECAlgoInputHDRBHist
        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                  AECAlgoInputHDRBHist,
                                                  sizeof(StatsBayerHist),
                                                  &pCommandInput->pAlgorithmInput->hdrBHist);

        // Set AECAlgoInputRDIStats
        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                  AECAlgoInputRDIStats,
                                                  sizeof(HDR3ExposureStats),
                                                  &pCommandInput->pAlgorithmInput->HDR3ExposureStatsData);

        // Set AECAlgoInputVendorTag
        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                  AECAlgoInputVendorTag,
                                                  sizeof(StatsVendorTagList),
                                                  &pCommandInput->pAlgorithmInput->vendorTagInputList);

        // Set AECAlgoInputDebugData
        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                    AECAlgoInputDebugData,
                                                    sizeof(StatsDataPointer),
                                                    &pCommandInput->pAlgorithmInput->debugData);
        // Set Chi Stats Session Data
        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                  AECAlgoInputStatsChiHandle,
                                                  sizeof(ChiStatsSession),
                                                  &pCommandInput->pAlgorithmInput->statsSession);

        // Set Camera Information
        m_cameraInfo = pCommandInput->pAlgorithmInput->cameraInfo;
        CAECEngineUtility::SetAlgorithmInputEntry(&algorithmInput[0],
                                                  AECAlgoInputCameraInfo,
                                                  sizeof(StatsCameraInfo),
                                                  &m_cameraInfo);

        algorithmInputList.numberOfAECInputs    = AECAlgoInputCount;
        algorithmInputList.pAECInputList        = &algorithmInput[0];


        CAECEngineUtility::InitializeAlgorithmOutput(&algorithmOutput[0], AECAlgoOutputCount);
        CAECEngineUtility::SetAlgorithmOutputEntry(&algorithmOutput[0],
                                                   AECAlgoOutputFrameInfo,
                                                   sizeof(AECAlgoFrameInfo),
                                                   &(pEngineOutput->frameInfo));

        CAECEngineUtility::SetAlgorithmOutputEntry(&algorithmOutput[0],
                                                   AECAlgoOutputFrameControl,
                                                   sizeof(AECAlgoFrameControl),
                                                   &(pEngineOutput->engineFrameControl.frameControl));

        CAECEngineUtility::SetAlgorithmOutputEntry(&algorithmOutput[0],
                                                   AECAlgoOutputEXIFApex,
                                                   sizeof(AECAlgoAPEXData),
                                                   &(pEngineOutput->engineFrameControl.apexData));

        CAECEngineUtility::SetAlgorithmOutputEntry(&algorithmOutput[0],
                                                   AECAlgoOutputAdditionalInfo,
                                                   sizeof(AECAlgoAdditionalInfo),
                                                   &(pEngineOutput->engineFrameControl.engineAdditionalControl));

        CAECEngineUtility::SetAlgorithmOutputEntry(&algorithmOutput[0],
                                                    AECAlgoOutputLEDCurrents,
                                                    sizeof(pEngineOutput->engineFrameControl.LEDCurrents),
                                                    &(pEngineOutput->engineFrameControl.LEDCurrents[0]));

        CAECEngineUtility::SetAlgorithmOutputEntry(&algorithmOutput[0],
                                                    AECAlgoOutputExposureSets,
                                                    sizeof(AECAlgoExposureSet),
                                                    &algorithmExposureSet);

        CAECEngineUtility::SetAlgorithmOutputEntry(&algorithmOutput[0],
                                                    AECAlgoOutputBGConfig,
                                                    sizeof(pEngineOutput->engineFrameControl.statsConfig),
                                                    &(pEngineOutput->engineFrameControl.statsConfig));

        CAECEngineUtility::SetAlgorithmOutputEntry(&algorithmOutput[0],
                                                    AECAlgoOutputBHistConfig,
                                                    sizeof(pEngineOutput->engineFrameControl.statsBHISTConfig),
                                                    &(pEngineOutput->engineFrameControl.statsBHISTConfig));

        // vendor tag output
        CAECEngineUtility::SetAlgorithmOutputEntry(&algorithmOutput[0],
                                                    AECAlgoOutputVendorTag,
                                                    sizeof(StatsVendorTagList),
                                                    pVendorTagList);

        algorithmOutputList.numberOfAECOutputs    = AECAlgoOutputCount;
        algorithmOutputList.pAECOutputList        = &algorithmOutput[0];
    }
    else
    {
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        CDKResult cdkResult = m_pAECAlgorithm->AECProcess(m_pAECAlgorithm, &algorithmInputList, &algorithmOutputList);

        result = StatsUtil::CdkResultToCamXResult(cdkResult);
    }

    if (CamxResultSuccess == result && startingPreflash)
    {
        SetOperationModeToAlgo(AECAlgoOperationModePreflash);
        GetAlgoParamForFrameControlType(AECAlgoGetParamStartExposure,
                                        NULL,
                                        &pCommandOutput->pProcessStatsOutput->algorithmOutput.engineFrameControl);
    }

    if (CamxResultSuccess == result)
    {
        GetAlgoParamForMultiCamera(&pCommandOutput->pProcessStatsOutput->algorithmOutput.pPeerInfo);
    }
    if (CamxResultSuccess == result)
    {
        GetAlgoParamForEvCapabilities(&pCommandOutput->pProcessStatsOutput->AECEVCapabilities);
    }

    if ((CamxResultSuccess == result)  && (AECState::LEDCalibration == m_AECState))
    {
        result = ConfigLEDCalibration(&pCommandOutput->pProcessStatsOutput->algorithmOutput.engineFrameControl,
            pCommandOutput->pProcessStatsOutput);
    }

    if (CamxResultSuccess == result)
    {
        result = CAECEngineUtility::CopyExposureSetToExposureData(&(pEngineOutput->engineFrameControl.exposureData[0]),
                                                                  AECAlgoExposureCount,
                                                                  &algorithmExposureSet);
    }

    if ((CamxResultSuccess == result) &&
        (0 == algorithmOutput[AECAlgoOutputBGConfig].sizeOfWrittenAECOutput))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "BG configuration is not filled from Algo");
    }

    if ((CamxResultSuccess == result) &&
        (0 == algorithmOutput[AECAlgoOutputBHistConfig].sizeOfWrittenAECOutput))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "BHIST configuration is not filled from Algo");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::ProcessStatsCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngine::ProcessStatsCaptureRequest(
    AECCommandInputParam*   pCommandInput,
    AECCommandOutputParam*  pCommandOutput)
{
    CAMX_UNREFERENCED_PARAM(pCommandInput);
    CamxResult  result = CamxResultSuccess;

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "m_snapshotIndicator %d", m_snapshotIndicator);

    AECAlgoGetParamInputList inputList;
    AECAlgoGetParamInput inputParams[1];

    AECAlgoSnapshotConfig snapConfig = {};

    snapConfig.snapshotType = (AECAlgoSnapshotLLS == m_snapshotIndicator) ? AECAlgoSnapshotLLS : AECAlgoSnapshotNormal;

    if ((ControlSceneModeHdr == m_HALParam.controlSceneMode) || (TRUE == m_HALParam.AEBracketMode))
    {
        snapConfig.bUseAEBracketing = TRUE;
    }
    snapConfig.EVVal = static_cast<INT8>(m_HALParam.AECompensation);
    inputList.numberOfAECGetInputParams = 1;
    inputList.pAECGetParamList          = inputParams;
    inputParams[0].inputType            = AECAlgoGetParamInputSnapshotType;
    inputParams[0].sizeOfInputData      = sizeof(snapConfig);
    inputParams[0].pInputData           = &snapConfig;
    result = GetAlgoParamForFrameControlType(AECAlgoGetParamSnapshotExposure, &inputList,
        &pCommandOutput->pProcessStatsOutput->algorithmOutput.engineFrameControl);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::OverrideAlgoSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::OverrideAlgoSetting()
{
    // Set all the override settings which are required
    CamxResult result = CamxResultSuccess;
    AECAlgoOverrideSettingParam overrideSetting;

    if (NULL != m_pStaticSettings)
    {
        // Speckle detection enable/disable override setting
        overrideSetting.disableSpeckleDetection = m_pStaticSettings->disableSpeckleDetection;

        // ISO Priority manual enable/disable override setting
        overrideSetting.disablePreviewManualISO = m_pStaticSettings->disablePreviewManualISO;

        // Exposure time manual enable/disable override setting
        overrideSetting.disablePreviewManualExpTime = m_pStaticSettings->disablePreviewManualExpTime;

        // Complete manual enable/disable override setting
        overrideSetting.disablePreviewManualFull = m_pStaticSettings->disablePreviewManualFull;

        // Force enable seamless in-sensor HDR 3 exposure snapshot override setting
        overrideSetting.enableForceSeamlessIHDRSnapshot =
                        (m_pStaticSettings->selectInSensorHDR3ExpUsecase == InSensorHDR3ExpForceSeamlessSnapshot)? TRUE: FALSE;

        // 3A debug data enable/disable override setting
        overrideSetting.enable3ADebugData = m_pStaticSettings->enable3ADebugData;

        // Concise 3A debug data enable/disable override setting
        overrideSetting.enableConcise3ADebugData = m_pStaticSettings->enableConcise3ADebugData;

        // AEC Profiling value
        overrideSetting.profile3A = m_pStaticSettings->profile3A;

        result = SetSingleParamToAlgorithm(AECAlgoSetParamOverrideSetting,
                                           &(overrideSetting),
                                           sizeof(AECAlgoOverrideSettingParam));
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "m_pStaticSettings is NULL");
        result = CamxResultEFailed;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::StartDriver
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::StartDriver(
    AECCommandInputParam*   pCommandInput,
    AECCommandOutputParam*  pCommandOutput)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
    CAMX_UNREFERENCED_PARAM(pCommandInput);
    CAMX_UNREFERENCED_PARAM(pCommandOutput);

    CamxResult result = CamxResultSuccess;
    if (NULL == m_pAECAlgorithm)
    {
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        // Set override settings for AEC algo
        result = OverrideAlgoSetting();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::StopDriver
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::StopDriver(
    AECCommandInputParam*   pCommandInput,
    AECCommandOutputParam*  pCommandOutput)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CAMX_UNREFERENCED_PARAM(pCommandInput);
    CAMX_UNREFERENCED_PARAM(pCommandOutput);

    CamxResult result = CamxResultSuccess;
    if (NULL == m_pAECAlgorithm)
    {
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        result = SetOperationModeToAlgo(AECAlgoOperationModeInit);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::ConfigDriver
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::ConfigDriver(
    AECCommandInputParam*   pCommandInput,
    AECCommandOutputParam*  pCommandOutput)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CAMX_UNREFERENCED_PARAM(pCommandInput);
    CAMX_UNREFERENCED_PARAM(pCommandOutput);

    /// @todo (CAMX-1074) fill the code to configure the driver

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::SetChromatix
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::SetChromatix(
    AECCommandInputParam*   pCommandInput,
    AECCommandOutputParam*  pCommandOutput)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CAMX_UNREFERENCED_PARAM(pCommandOutput);
    CamxResult result = CamxResultSuccess;

    result = SetSingleParamToAlgorithm(AECAlgoSetParamChromatixData, pCommandInput->pTuningData, sizeof(StatsTuningData));

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::StartStreaming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::StartStreaming(
    AECCommandInputParam*    pCommandInput,
    AECEngineFrameControl*   pFrameControl)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CamxResult result = CamxResultSuccess;
    AECAlgoOperationModeType aeOperationMode = AECAlgoOperationModeStreaming;

    switch (pCommandInput->pInitStreamConfig->operationMode)
    {
        case StatsOperationModeNormal:
            if (InstantAecModeAggressive == m_HALParam.instantAECMode)
            {
                aeOperationMode = AECAlgoOperationModeAggressiveAEC;
            }
            else
            {
                aeOperationMode = AECAlgoOperationModeStreaming;
            }
            pCommandInput->pInitStreamConfig->aecStartSensitivity.startMode = AECStartupModeWarmStart;
            break;
        case StatsOperationModeFastConvergence:
            aeOperationMode = AECAlgoOperationModeFastAEC;
            pCommandInput->pInitStreamConfig->aecStartSensitivity.startMode = AECStartupModeFastConvergence;
            break;
        default:
            break;
    }

    CAMX_LOG_INFO(CamxLogGroupAEC, "StartStreaming: aeOperation: %d operationMode: %d startMode:%d",
        aeOperationMode,
        pCommandInput->pInitStreamConfig->operationMode,
        pCommandInput->pInitStreamConfig->aecStartSensitivity.startMode);

    if (NULL == m_pAECAlgorithm)
    {
        result = CamxResultEInvalidState;
    }
    if (CamxResultSuccess == result)
    {
        result = SetOperationModeToAlgo(aeOperationMode);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Set param failed result: %d",
                Utils::CamxResultToString(result));
        }
    }

    if (CamxResultSuccess == result)
    {
        /* If engine has to configure some specific start up exposure, lets set that first before we get initial gains
           Required for warm start  */
        if (AECStartupModeWarmStart == pCommandInput->pInitStreamConfig->aecStartSensitivity.startMode)
        {
            if ((0 != m_HALParam.warmStartSensitivity[AECAlgoExposureShort]) &&
                (0 != m_HALParam.warmStartSensitivity[AECAlgoExposureLong])  &&
                (0 != m_HALParam.warmStartSensitivity[AECAlgoExposureSafe])  &&
                (FALSE == m_warmStartDone))
            {
                // make a copy
                Utils::Memcpy(&pCommandInput->pInitStreamConfig->aecStartSensitivity.sensitivity[0],
                              &m_HALParam.warmStartSensitivity[0],
                              sizeof(m_HALParam.warmStartSensitivity));
                // Set algo with start-up exposure setting
                result = SetSingleParamToAlgorithm(AECAlgoSetParamStartUpExpSensitivity,
                                                   &pCommandInput->pInitStreamConfig->aecStartSensitivity,
                                                   sizeof(pCommandInput->pInitStreamConfig->aecStartSensitivity));
                m_warmStartDone = TRUE;

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupAEC, "Set param failed for startup exp: result: %d",
                        Utils::CamxResultToString(result));
                }
            }
        }
        else if (AECStartupModeFastConvergence  == pCommandInput->pInitStreamConfig->aecStartSensitivity.startMode)
        {
            // Do not do anything for Fast AEC Convergence as it is not implemented at node side.
        }
    }

    if (CamxResultSuccess == result)
    {
        result = GetAlgoParamForFrameControlType(AECAlgoGetParamStartExposure, NULL, pFrameControl);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Get param failed result: %d", Utils::CamxResultToString(result));
        }
    }

    if (CamxResultSuccess == result)
    {
        AECAlgoGetParamOutput outputList[1];
        outputList[0].outputType                = AECAlgoGetParamOutputBGStatsConfig;
        outputList[0].pOutputData               = &pFrameControl->statsConfig;
        outputList[0].sizeOfOutputData          = sizeof(pFrameControl->statsConfig);
        outputList[0].sizeOfWrittenOutputData   = 0;

        // Query the BG Stats configuration
        result = GetParamFromAlgorithm(AECAlgoGetParamBGStatsConfig, NULL, &outputList[0], 1);

        if ((CamxResultSuccess != result) || (0 == outputList[0].sizeOfWrittenOutputData))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Invalid BG Stats configuration from Algo Result:%d Size:%d",
                result, outputList[0].sizeOfWrittenOutputData);
        }
    }
    if (CamxResultSuccess == result)
    {
        AECAlgoGetParamOutput outputList[1];
        outputList[0].outputType              = AECAlgoGetParamOutputBHistStatsConfig;
        outputList[0].pOutputData             = &pFrameControl->statsBHISTConfig;
        outputList[0].sizeOfOutputData        = sizeof(pFrameControl->statsBHISTConfig);
        outputList[0].sizeOfWrittenOutputData = 0;
        // Query the BHIST Stats configuration
        result = GetParamFromAlgorithm(AECAlgoGetParamBHistStatsConfig, NULL, &outputList[0], 1);

        if ((CamxResultSuccess != result) || (0 == outputList[0].sizeOfWrittenOutputData))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Invalid BHIST Stats configuration from Algo Result:%d Size:%d",
                result, outputList[0].sizeOfWrittenOutputData);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetParamForVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetParamForVendorTag(
    AECCommandInputParam*   pCommandInput,
    AECCommandOutputParam*  pCommandOutput)
{
    CAMX_UNREFERENCED_PARAM(pCommandInput);

    CamxResult result = CamxResultSuccess;

    if (NULL == m_pAECAlgorithm)
    {
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        result = GetAlgoParamForDependVendorTagType(AECAlgoGetParamDependentVendorTags, pCommandOutput->pVendorTagInfoList);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetParamForPubVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetParamForPubVendorTag(
    AECCommandInputParam*   pCommandInput,
    AECCommandOutputParam*  pCommandOutput)
{
    CAMX_UNREFERENCED_PARAM(pCommandInput);

    CamxResult result = CamxResultSuccess;

    if (NULL == m_pAECAlgorithm)
    {
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        result = GetAlgoParamForVendorTagType(AECAlgoGetParamPublishingVendorTagsInfo, pCommandOutput->pVendorTagInfoList);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::SetPerFrameControlParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::SetPerFrameControlParam(
    AECCommandInputParam*  pCommandInput,
    AECCommandOutputParam* pCommandOutput)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    CAMX_UNREFERENCED_PARAM(pCommandOutput);
    AECEngineHALParam* pHALParam = NULL;
    CamxResult         result    = CamxResultSuccess;

    if (NULL == m_pAECAlgorithm)
    {
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        pHALParam  = pCommandInput->pHALParam;
        // make a copy
        Utils::Memcpy(&m_HALParam, pHALParam, sizeof(m_HALParam));

        if (ControlAEPrecaptureTriggerStart == m_HALParam.AETrigger)
        {
            m_precaptureWaitFrames = static_cast<UINT8>(m_maxPipelineDelay);
        }

        INT32                          index = 0;
        AECAlgoSetParamList            setParamList;
        AECAlgoDiagConfig              diagConfig = { 1, { 1, 1, 1, 1, 1, 1 } };
        AECAlgoSetParam                setParam[AECAlgoSetParamCount];
        AECAlgoROIList                 touchROIList;
        AECAlgoROIList                 faceROIList;
        AECAlgoROI                     faceROI[MaxFaceROIs];
        UnstabilizedROIInformation     unstabilizedFaceROIInformation;
        UnstabilizedROIInformation     trackerROIInformation;

        faceROIList.ROIs = faceROI;
        CAECEngineUtility::LoadSetParamList(&setParam[index],
                                            &pHALParam->frameId,
                                            AECAlgoSetParamframeID,
                                            sizeof(pHALParam->frameId));
        index++;

        // set AECAlgoLockType
        // We need to set lock before EV to honour EV change in case of lock as well
        AECAlgoLockType lockSetting;
        CAECEngineUtility::SetLockToSetParamList(pHALParam, index, &setParam[0], &lockSetting);
        index++;

        // set AECAlgoSetParamMeteringMode
        switch (pHALParam->AEMeteringMode)
        {
            case ExposureMeteringFrameAverage:
                pHALParam->AEMeteringMode = AECAlgoMeteringModeFrameAverage;
                break;

            case ExposureMeteringCenterWeighted:
                pHALParam->AEMeteringMode = AECAlgoMeteringModeCenterWeighted;
                break;

            case ExposureMeteringSpotMetering:
                pHALParam->AEMeteringMode = AECAlgoMeteringModeSpot;
                break;

            case ExposureMeteringCustom:
                pHALParam->AEMeteringMode = AECAlgoMeteringModeCustom;
                break;

            default:
                pHALParam->AEMeteringMode = AECAlgoMeteringModeFrameAverage;
                break;
        }

        CAECEngineUtility::LoadSetParamList(&setParam[index],
                                            &pHALParam->AEMeteringMode,
                                            AECAlgoSetParamMeteringMode,
                                            sizeof(pHALParam->AEMeteringMode));
        index++;

        // set AECAlgoSetParamExposureCompensation
        CAECEngineUtility::LoadSetParamList(&setParam[index],
                                            &pHALParam->AECompensation,
                                            AECAlgoSetParamExposureCompensation,
                                            sizeof(pHALParam->AECompensation));
        index++;

        // set AECAlgoSetParamFlickerCompensation
        CAECEngineUtility::LoadSetParamList(&setParam[index],
                                            &pHALParam->flickerMode,
                                            AECAlgoSetParamFlickerCompensation,
                                            sizeof(pHALParam->flickerMode));
        index++;

        // set AECAlgoSetParamMultiExpSensorMode
        CAECEngineUtility::LoadSetParamList(&setParam[index],
                                            &pHALParam->videoHDRType,
                                            AECAlgoSetParamMultiExpSensorMode,
                                            sizeof(pHALParam->videoHDRType));
        index++;

        // set AECAlgoSetParamCustomExposureTable
        CAECEngineUtility::LoadSetParamList(&setParam[index],
                                            &pHALParam->customExposureTable,
                                            AECAlgoSetParamCustomExposureTable,
                                            sizeof(pHALParam->customExposureTable));
        index++;

        // set AECAlgoSetParamCustomMeteringTable
        CAECEngineUtility::LoadSetParamList(&setParam[index],
                                            &pHALParam->customMeteringTable,
                                            AECAlgoSetParamCustomMeteringTable,
                                            sizeof(pHALParam->customMeteringTable));
        index++;

        // set AECAlgoSetParamDisableADRC
        CAECEngineUtility::LoadSetParamList(&setParam[index],
                                            &pHALParam->disableADRC,
                                            AECAlgoSetParamDisableADRC,
                                            sizeof(pHALParam->disableADRC));
        index++;

        // set AECAlgoSetParamConvSpeed
        CAECEngineUtility::LoadSetParamList(&setParam[index],
                                            &pHALParam->convergenceSpeed,
                                            AECAlgoSetParamConvergenceSpeed,
                                            sizeof(pHALParam->convergenceSpeed));

        index++;

        if (CamxResultSuccess == CAECEngineUtility::SetAECAlgoDiagConfig(&diagConfig,
            & setParam[index],
            m_AECState))
        {
            index++;
        }
        if (CamxResultSuccess == CAECEngineUtility::SetROIToSetParamList(&setParam[index],
            pHALParam,
            &touchROIList))
        {
            index++;
        }

        if (CamxResultSuccess == CAECEngineUtility::SetFaceROIToSetParamList(&setParam[index],
            pHALParam,
            &faceROIList,
            &m_sensorInfo))
        {
            index++;
        }

        if (CamxResultSuccess == CAECEngineUtility::SetUniformFaceROIToSetParamList(&setParam[index],
            pHALParam,
            &unstabilizedFaceROIInformation,
            &m_sensorInfo))
        {
            index++;
        }

        if (CamxResultSuccess == CAECEngineUtility::SetTrackerROIToSetParamList(&setParam[index],
            pHALParam,
            &trackerROIInformation,
            &m_sensorInfo))
        {
            index++;
        }

        if (NULL != pHALParam->pPeerInfo)
        {
            // set AECAlgoSetParamPeerInfo
            CAECEngineUtility::LoadSetParamList(&setParam[index],
                                                &pHALParam->pPeerInfo,
                                                AECAlgoSetParamPeerInfo,
                                                sizeof(VOID*));
            index++;
        }

        // set AECAlgoSetParamFPSRange
        AECAlgoFPSRange fpsRange;
        CAECEngineUtility::SetFPSRangeToSetParamList(pHALParam, index, &setParam[0], &fpsRange);
        index++;

        // set AECAlgoSetParamLEDMode
        AECAlgoLEDModeType ledMode = AECAlgoLEDModeOff;
        CAECEngineUtility::SetLEDModeToSetParamList(pHALParam, index, &setParam[0], &ledMode);
        index++;

        // set AECAlgoSetParamManualSetting
        AECAlgoManualSetting manualSetting;
        manualSetting.manualMode = CAECEngineUtility::GetAECAlgorithmMode(pHALParam);
        CAECEngineUtility::SetManualSettingToSetParamList(pHALParam, index, &setParam[0], &manualSetting);
        index++;

        // set flash influenced stats flag
        setParam[index].pAECSetParam        = &m_isFlashInfluencedStats;
        setParam[index].setParamType        = AECAlgoSetParamIsFlashInfluencedStats;
        setParam[index].sizeOfAECSetParam   = sizeof(BOOL);
        index++;

        // set camera information
        setParam[index].pAECSetParam      = &m_cameraInfo;
        setParam[index].setParamType      = AECAlgoSetParamCameraInfo;
        setParam[index].sizeOfAECSetParam = sizeof(StatsCameraInfo);
        index++;

        // set AECAlgoSetParamControlPostRawSensitivityBoost
        CAECEngineUtility::LoadSetParamList(&setParam[index],
            &pHALParam->controlPostRawSensitivityBoost,
            AECAlgoSetParamControlPostRawSensitivityBoost,
            sizeof(pHALParam->controlPostRawSensitivityBoost));
        index++;

        setParamList.numberOfAECSetParams   = index;
        setParamList.pAECSetParamList       = &setParam[0];

        CDKResult cdkResult = m_pAECAlgorithm->AECSetParam(m_pAECAlgorithm, &setParamList);

        result = StatsUtil::CdkResultToCamXResult(cdkResult);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Set param failed: %s", Utils::CamxResultToString(result));
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::SetNodesUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::SetNodesUpdate(
    AECCommandInputParam*  pCommandInput,
    AECCommandOutputParam* pCommandOutput)
{
    CAMX_UNREFERENCED_PARAM(pCommandOutput);

    CamxResult result = CamxResultSuccess;
    m_RERDone         = pCommandInput->pNodesUpdate->isRERDone;
    if (NULL == m_pAECAlgorithm)
    {
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        AECEngineNodesUpdate* pNodesUpdate = pCommandInput->pNodesUpdate;
        AECAlgoWhiteBalanceState AWBState  = { 0 };
        AWBState.redGain   = pNodesUpdate->pAWBFrameInfo->AWBGains.rGain;
        AWBState.greenGain = pNodesUpdate->pAWBFrameInfo->AWBGains.gGain;
        AWBState.blueGain  = pNodesUpdate->pAWBFrameInfo->AWBGains.bGain;
        AWBState.CCT       = pNodesUpdate->pAWBFrameInfo->colorTemperature;
        AWBState.isAuto    = pNodesUpdate->isAWBModeAuto;

        result = SetSingleParamToAlgorithm(AECAlgoSetParamAWBState, &AWBState, sizeof(AECAlgoWhiteBalanceState));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Algorithm library does not support AECAlgoSetParamAWBState");
            result = CamxResultSuccess;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::ProcessHardwareInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::ProcessHardwareInfo(
    AECCommandInputParam*  pCommandInput,
    AECCommandOutputParam* pCommandOutput)
{
    CAMX_UNREFERENCED_PARAM(pCommandOutput);

    CamxResult       result         = CamxResultSuccess;
    AECEngineHWInfo* pHardwareInfo  = pCommandInput->pHardwareInfo;

    if (NULL == m_pAECAlgorithm)
    {
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        m_sensorInfo = pHardwareInfo->sensorInfo;
        m_isFixedFocus = pHardwareInfo->isFixedFocus;
        result = SetSingleParamToAlgorithm(AECAlgoSetParamSensorInfo, &pHardwareInfo->sensorInfo,
                                           sizeof(pHardwareInfo->sensorInfo));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Algorithm library does not support AECAlgoSetParamSensorInfo");
            result = CamxResultSuccess;
        }
    }

    if (CamxResultSuccess == result)
    {
        result = SetSingleParamToAlgorithm(AECAlgoSetParamStatsCapability, &pHardwareInfo->statsCapabilities,
                                           sizeof(pHardwareInfo->statsCapabilities));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Algorithm library does not support AECAlgoSetParamStatsCapability");
            result = CamxResultSuccess;
        }

        result = SetSingleParamToAlgorithm(AECAlgoSetParamIsDualCamera, &pHardwareInfo->isDual,
            sizeof(pHardwareInfo->isDual));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Algorithm library does not support AECAlgoSetParamIsDualCamera");
            result = CamxResultSuccess;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::ProcessCropWindow
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::ProcessCropWindow(
    AECCommandInputParam*  pCommandInput,
    AECCommandOutputParam* pCommandOutput)
{
    CAMX_UNREFERENCED_PARAM(pCommandOutput);

    CamxResult result = CamxResultSuccess;
    if (NULL == m_pAECAlgorithm)
    {
        result = CamxResultEInvalidState;
    }

    if (CamxResultSuccess == result)
    {
        StatsRectangle* pCropWindow = pCommandInput->pCropWindow;
        result = SetSingleParamToAlgorithm(AECAlgoSetParamCropWindow, pCropWindow, sizeof(*pCropWindow));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Algorithm library does not support AECAlgoSetParamCropWindow");
            result = CamxResultSuccess;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::SetSingleParamToAlgorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::SetSingleParamToAlgorithm(
    AECAlgoSetParamType paramType,
    VOID*               pParam,
    UINT32              paramSize)
{
    CamxResult result = CamxResultSuccess;

    AECAlgoSetParamList setParamList;
    AECAlgoSetParam     setParam[2];

    setParam[0].pAECSetParam            = pParam;
    setParam[0].setParamType            = paramType;
    setParam[0].sizeOfAECSetParam       = paramSize;

    // set camera information
    setParam[1].pAECSetParam      = &m_cameraInfo;
    setParam[1].setParamType      = AECAlgoSetParamCameraInfo;
    setParam[1].sizeOfAECSetParam = sizeof(StatsCameraInfo);

    setParamList.numberOfAECSetParams   = 2;
    setParamList.pAECSetParamList       = &setParam[0];
    CDKResult cdkResult = m_pAECAlgorithm->AECSetParam(m_pAECAlgorithm, &setParamList);
    result = StatsUtil::CdkResultToCamXResult(cdkResult);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Set param failed: %s", Utils::CamxResultToString(result));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetParamFromAlgorithm
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetParamFromAlgorithm(
    AECAlgoGetParamType       paramType,
    AECAlgoGetParamInputList* pInputList,
    AECAlgoGetParamOutput*    pOutputList,
    UINT32                    numberOfOutputs)
{
    CamxResult      result = CamxResultSuccess;
    AECAlgoGetParam getParam;
    UINT32          numberOfAECGetInputParams = 1;

    if (NULL != pInputList)
    {
        numberOfAECGetInputParams = pInputList->numberOfAECGetInputParams + 1;
    }
    AECAlgoGetParamInput* pInput  = CAMX_NEW AECAlgoGetParamInput[numberOfAECGetInputParams];

    if (NULL != pInput)
    {
        getParam.type                 = paramType;
        pInput[0].inputType           = AECAlgoGetParamInputCameraInfo;
        pInput[0].pInputData          = reinterpret_cast<VOID*>(&m_cameraInfo);
        pInput[0].sizeOfInputData     = sizeof(m_cameraInfo);

        if (NULL != pInputList)
        {
            for (UINT32 i = 0; i < pInputList->numberOfAECGetInputParams; i++)
            {
                pInput[i + 1] = pInputList->pAECGetParamList[i];
            }
        }
        getParam.inputList.numberOfAECGetInputParams = numberOfAECGetInputParams;
        getParam.inputList.pAECGetParamList          = pInput;
        getParam.outputList.numberOfAECOutputs       = numberOfOutputs;
        getParam.outputList.pAECOutputList           = pOutputList;
        CDKResult cdkResult = m_pAECAlgorithm->AECGetParam(m_pAECAlgorithm, &getParam);
        result = StatsUtil::CdkResultToCamXResult(cdkResult);
        CAMX_DELETE[] pInput;
        pInput = NULL;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "pInput create failed - out of memory");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetAlgoParamForMultiCamera
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetAlgoParamForMultiCamera(
    VOID** ppPeerInfo)
{
    CamxResult result = CamxResultSuccess;

    AECAlgoGetParamOutput outputList;

    outputList.outputType              = AECAlgoGetParamOutputPeerInfo;
    outputList.pOutputData             = ppPeerInfo;
    outputList.sizeOfOutputData        = sizeof(VOID**);
    outputList.sizeOfWrittenOutputData = 0;


    result = GetParamFromAlgorithm(AECAlgoGetParamPeerInfo, NULL, &outputList, 1);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetAlgoParamForEvCapabilities
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetAlgoParamForEvCapabilities(
    VOID* pEvCapabilities)
{
    CamxResult result = CamxResultSuccess;

    AECAlgoGetParamOutput outputList;

    outputList.outputType              = AECAlgoGetParamOutputEVCapabilities;
    outputList.pOutputData             = pEvCapabilities;
    outputList.sizeOfOutputData        = sizeof(VOID*);
    outputList.sizeOfWrittenOutputData = 0;


    result = GetParamFromAlgorithm(AECAlgoGetParamEVCapabilities, NULL, &outputList, 1);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetAlgoParamForSnapshotType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetAlgoParamForSnapshotType(
    VOID* pSnapshotType)
{
    CamxResult result = CamxResultSuccess;

    AECAlgoGetParamOutput outputList;

    outputList.outputType              = AECAlgoGetParamOutputSnapshotType;
    outputList.pOutputData             = pSnapshotType;
    outputList.sizeOfOutputData        = sizeof(VOID*);
    outputList.sizeOfWrittenOutputData = 0;

    result = GetParamFromAlgorithm(AECAlgoGetParamSnapshotType, NULL, &outputList, 1);

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetAlgoParamForFrameControlType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetAlgoParamForFrameControlType(
    AECAlgoGetParamType       paramType,
    AECAlgoGetParamInputList* pInputList,
    AECEngineFrameControl*    pFrameControl)
{
    CamxResult result = CamxResultSuccess;

    UINT32                index             = 0;

    const UINT32          numberOfOutputs   = 5;
    AECAlgoGetParamOutput outputList[numberOfOutputs];
    AECAlgoExposureSet    algoExposureSet;
    AECAlgoExposureInfo   algoExposureInfo[AECAlgoExposureCount];

    // initialize the outputList
    outputList[index].outputType                = AECAlgoGetParamOutputFrameControl;
    outputList[index].pOutputData               = &pFrameControl->frameControl;
    outputList[index].sizeOfOutputData          = sizeof(pFrameControl->frameControl);
    outputList[index].sizeOfWrittenOutputData   = 0;

    index++;
    algoExposureSet.exposureInfoSize            = sizeof(AECAlgoExposureInfo);
    algoExposureSet.exposureInfoCount           = AECAlgoExposureCount;
    algoExposureSet.exposureInfoOutputCount     = 0;
    algoExposureSet.exposureInfoOutputSize      = 0;
    algoExposureSet.pExposureInfo               = &algoExposureInfo[0];

    outputList[index].outputType                = AECAlgoGetParamOutputExposureSets;
    outputList[index].pOutputData               = &algoExposureSet;
    outputList[index].sizeOfOutputData          = sizeof(algoExposureSet);
    outputList[index].sizeOfWrittenOutputData   = 0;

    index++;
    outputList[index].outputType                = AECAlgoGetParamOutputLEDCurrents;
    outputList[index].pOutputData               = &pFrameControl->LEDCurrents[0];
    outputList[index].sizeOfOutputData          = sizeof(pFrameControl->LEDCurrents);
    outputList[index].sizeOfWrittenOutputData   = 0;

    index++;
    outputList[index].outputType                = AECAlgoGetParamOutputEXIFApex;
    outputList[index].pOutputData               = &pFrameControl->apexData;
    outputList[index].sizeOfOutputData          = sizeof(pFrameControl->apexData);
    outputList[index].sizeOfWrittenOutputData   = 0;

    index++;
    outputList[index].outputType                = AECAlgoGetParamOutputAdditionalInfo;
    outputList[index].pOutputData               = &pFrameControl->engineAdditionalControl;
    outputList[index].sizeOfOutputData          = sizeof(pFrameControl->engineAdditionalControl);
    outputList[index].sizeOfWrittenOutputData   = 0;

    index++;

    if (numberOfOutputs != index)
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "index out of bound");
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        result = GetParamFromAlgorithm(paramType, pInputList, &outputList[0], numberOfOutputs);
    }

    if (CamxResultSuccess == result)
    {
        // copy the data from algorithm's exposureSet to engine's exposureData
        CAECEngineUtility::CopyExposureSetToExposureData(&pFrameControl->exposureData[0],
                                                         AECAlgoExposureCount,
                                                         &algoExposureSet);
    }

    if (CamxResultSuccess == result)
    {
        outputList[0].outputType                = AECAlgoGetParamOutputBGStatsConfig;
        outputList[0].pOutputData               = &pFrameControl->statsConfig;
        outputList[0].sizeOfOutputData          = sizeof(pFrameControl->statsConfig);
        outputList[0].sizeOfWrittenOutputData   = 0;

        // Query the BG Stats configuration
        result = GetParamFromAlgorithm(AECAlgoGetParamBGStatsConfig, pInputList, &outputList[0], 1);

        if ((CamxResultSuccess != result) || (0 == outputList[0].sizeOfWrittenOutputData))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Invalid BG Stats configuration from Algo");
        }
    }

    if (CamxResultSuccess == result)
    {
        outputList[0].outputType = AECAlgoGetParamOutputBHistStatsConfig;
        outputList[0].pOutputData = &pFrameControl->statsBHISTConfig;
        outputList[0].sizeOfOutputData = sizeof(pFrameControl->statsBHISTConfig);
        outputList[0].sizeOfWrittenOutputData = 0;

        // Query the BG Stats configuration
        result = GetParamFromAlgorithm(AECAlgoGetParamBHistStatsConfig, pInputList, &outputList[0], 1);

        if ((CamxResultSuccess != result) || (0 == outputList[0].sizeOfWrittenOutputData))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Invalid BHIST Stats configuration from Algo");
            result = CamxResultSuccess;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Failed to get frame control info from algo: %s", Utils::CamxResultToString(result));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::GetAECState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AECState CAECEngine::GetAECState()
{
    return m_AECState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::SetAECState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::SetAECState(
    AECState state)
{
    if (m_AECState != state)
    {
        CAMX_LOG_INFO(CamxLogGroupAEC, "AEC_DEBUG_STATE: AECEngine State change: Old = %s to New = %s",
            CamxAECEngineStateStrings[static_cast<UINT8>(m_AECState)],
            CamxAECEngineStateStrings[static_cast<UINT8>(state)]);

        m_AECState = state;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::GetLEDCalibrationState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LEDCalibrationState CAECEngine::GetLEDCalibrationState()
{
    return m_LEDCalibrationState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::SetLEDCalibrationState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::SetLEDCalibrationState(
    LEDCalibrationState state)
{
    if (m_LEDCalibrationState != state)
    {
        CAMX_LOG_INFO(CamxLogGroupAEC, "AEC_DEBUG_STATE: LEDCalibration State change: Old = %s to New = %s",
            CamxLEDCalibrationState[static_cast<UINT8>(m_LEDCalibrationState)],
            CamxLEDCalibrationState[static_cast<UINT8>(state)]);

        m_LEDCalibrationState = state;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::GetAECPreFlashState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PreFlashState CAECEngine::GetAECPreFlashState()
{
    return m_AECPreFlashState;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::SetAECPreFlashState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::SetAECPreFlashState(
    PreFlashState    state)
{
    if (m_AECPreFlashState != state)
    {
        CAMX_LOG_INFO(CamxLogGroupAEC, "AEC_DEBUG_STATE: Preflash State change: Old = %s to New = %s",
            CamxAECPreFlashStateStrings[static_cast<UINT8>(m_AECPreFlashState)],
            CamxAECPreFlashStateStrings[static_cast<UINT8>(state)]);

        m_AECPreFlashState = state;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::GetFlashTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAECEngine::FlashTrigger CAECEngine::GetFlashTrigger()
{
    return m_flashTrigger;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::SetFlashTrigger
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::SetFlashTrigger(
    FlashTrigger trigger)
{
    // if the trigger is Invalid, just reset it
    // otherwise, the higher value of trigger takes priority
    if (trigger == FlashTrigger::Invalid)
    {
        m_flashTrigger = trigger;
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "SetFlashTrigger: Reset");
    }
    else if (trigger > m_flashTrigger)
    {
        m_flashTrigger = trigger;
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "SetFlashTrigger: Start Mode = %d", trigger);
    }

    return CamxResultSuccess;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsLEDAFNeeded
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsLEDAFNeeded()
{
    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Preflash: LED AF needed: %d isFixedFocus: %d", m_LEDAFRequired, m_isFixedFocus);
    return m_LEDAFRequired && !(m_isFixedFocus);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsAELocked
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsAELocked()
{
    BOOL aeLocked = FALSE;

    if (ControlAELockOn == m_HALParam.AELock)
    {
        aeLocked = TRUE;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "IS AEC Locked = %d", aeLocked);
    return aeLocked;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsHalPreFlashSettingEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsHalPreFlashSettingEnabled()
{
    BOOL preFlashNeeded = FALSE;

    switch (m_HALParam.AEMode)
    {
        case ControlAEModeOnAlwaysFlash: /* Flash Mode: Always ON */
            preFlashNeeded = TRUE;
            break;

        case ControlAEModeOnAutoFlash: /* Flash Mode: Auto */
        case ControlAEModeOnAutoFlashRedeye:
        {
            if (AECAlgoSnapshotFlash == m_snapshotIndicator) /* if Algo says flash required */
            {
                preFlashNeeded = TRUE;
            }
            break;
        }

        case ControlAEModeOff: /* Flash Mode: Off */
        {
            if (FlashModeSingle == m_HALParam.flashMode) /* Manual Flash Mode*/
            {
                preFlashNeeded = TRUE;
            }
            break;
        }

        case ControlAEModeOn:  /* Flash Mode: On */
        default:
            break;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "IsHalPreFlashSettingEnabled = %d", preFlashNeeded);
    return preFlashNeeded;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsTouchFlashEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsTouchFlashEnabled()
{
    BOOL isTouchROI = TRUE;

    // By default, return FALSE to enable preflash for precapture AF trigger.
    if (TRUE == m_pStaticSettings->disablePreFlashOnForTouchAE)
    {
        isTouchROI = FALSE;
    }
    else
    {
        if (m_HALParam.touchROI.dx == 0.0 &&
            m_HALParam.touchROI.dy == 0.0)
        {
            isTouchROI = FALSE;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "IsTouchFlashEnabled = %d", isTouchROI);
    return isTouchROI;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::CancelPreFlash
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::CancelPreFlash()
{
    m_isPrecaptureInProgress = FALSE;
    // @todo (CAMX-1302) fill the logic to cancel the preflash logic
    // leave the clean up logic in PreFlashStateCompleteNoLED
    SetAECPreFlashState(PreFlashStateCompleteNoLED);
    Utils::Memset(&m_flashFrameControl, 0, sizeof(m_flashFrameControl));
    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AEC_DEBUG_CAPTURE: Precapture Canceled");

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::TransitToNextPreFlashState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::TransitToNextPreFlashState(
    PreFlashState curState)
{
    CAMX_UNREFERENCED_PARAM(curState);
    // @todo (CAMX-1302) fill the logic to determine the next PreFlashState
    switch (curState)
    {
        case PreFlashStateInactive:
            m_AECPreFlashSkipChecking = FALSE;
            SetAECPreFlashState(PreFlashStateStart);
            break;
        case PreFlashStateStart:
            if (TRUE == m_disableAFAWBpreFlash)
            {
                SetAECPreFlashState(PreFlashStateCompleteLED);
            }
            else
            {
                // . check if need do FD
                // . otherwise check if need do AF
                // . otherwise do AWB

                AECAlgoModeType algoMode = CAECEngineUtility::GetAECAlgorithmMode(&m_HALParam);

                if (IsLEDAFNeeded() ||
                    ((AECAlgoModeManualFull == algoMode) && (FlashModeSingle == m_HALParam.flashMode)))
                {
                    SetAECPreFlashState(PreFlashStateTriggerAF);

                    // AF gets triggered in (N+1)th frame and SetNodesUpdate reads data of Nth frame
                    // hence need to skip one frame to check correct status of AF
                    // so that AEC preflash state should not move to next state.
                    m_AECPreFlashSkipChecking = TRUE;
                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Preflash_AF_TRIGGER");
                    CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupAEC, 0, "Flash: AF");
                }
                else
                {
                    // Is AWB required?
                    SetAECPreFlashState(PreFlashStateTriggerAWB);

                    // AWB gets triggered in (N+1)th frame and SetNodesUpdate reads data of Nth frame
                    // hence need to skip one frame to check correct status of AWB
                    // so that AEC preflash state should not move to next state.
                    m_AECPreFlashSkipChecking = TRUE;
                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Preflash_AWB_TRIGGER");
                }
            }
            break;
        case PreFlashStateTriggerFD:
            // . otherwise check if need do AF
            // . otherwise check if need do AWB
            if (IsLEDAFNeeded())
            {
                SetAECPreFlashState(PreFlashStateTriggerAF);
            }
            else
            {
                SetAECPreFlashState(PreFlashStateTriggerAWB);
            }
            break;
        case PreFlashStateTriggerAF:
            //   Move to AWB State
            SetAECPreFlashState(PreFlashStateTriggerAWB);
            CAMX_TRACE_ASYNC_END_F(CamxLogGroupAEC, 0, "Flash: AF");
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Preflash_AWB_TRIGGER");
            CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupAEC, 0, "Flash: AWB");
            break;
        case PreFlashStateTriggerAWB:
            CAMX_TRACE_ASYNC_END_F(CamxLogGroupAEC, 0, "Flash: AWB");
            SetAECPreFlashState(PreFlashStateCompleteLED);
            CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupAEC, 0, "Flash: PreFlashComplete");
            break;
        case PreFlashStateCompleteLED:
            CAMX_TRACE_ASYNC_END_F(CamxLogGroupAEC, 0, "Flash: PreFlashComplete");
            SetAECPreFlashState(PreFlashStateRER);
            break;
        default:
            break;
    }
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetAlgoParamForVendorTagType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetAlgoParamForVendorTagType(
    AECAlgoGetParamType     paramType,
    StatsVendorTagInfoList* pVendorTagInfoList)
{
    CamxResult result = CamxResultSuccess;

    UINT32                index           = 0;
    const UINT32          numberOfOutputs = 1;
    AECAlgoGetParamOutput outputList[numberOfOutputs];

    // initialize the outputList
    outputList[index].outputType              = AECAlgoGetParamOutputVendorTagInfoList;
    outputList[index].pOutputData             = pVendorTagInfoList;
    outputList[index].sizeOfOutputData        = sizeof(StatsVendorTagInfoList);
    outputList[index].sizeOfWrittenOutputData = 0;

    result = GetParamFromAlgorithm(paramType, NULL, &outputList[0], numberOfOutputs);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetAlgoParamForDependVendorTagType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetAlgoParamForDependVendorTagType(
    AECAlgoGetParamType     paramType,
    StatsVendorTagInfoList* pVendorTagInfoList)
{
    CamxResult result = CamxResultSuccess;

    UINT32                   index             = 0;
    const UINT32             numberOfOutputs   = 1;
    const UINT32             numberOfInputs    = 1;
    AECAlgoGetParamInputList inputList;
    AECAlgoGetParamOutput    outputList[numberOfOutputs];
    AECAlgoGetParamInput     inputParams[numberOfInputs];

    // initialize the outputList
    outputList[index].outputType              = AECGetParamOutputTypeDependentVendorTags;
    outputList[index].pOutputData             = pVendorTagInfoList;
    outputList[index].sizeOfOutputData        = sizeof(StatsVendorTagInfoList);
    outputList[index].sizeOfWrittenOutputData = 0;

    inputList.numberOfAECGetInputParams       = numberOfInputs;
    inputList.pAECGetParamList                = inputParams;
    inputParams[0].inputType                  = AECAlgoGetParamInputIsFixedFocus;
    inputParams[0].sizeOfInputData            = sizeof(m_isFixedFocus);
    inputParams[0].pInputData                 = &m_isFixedFocus;

    result = GetParamFromAlgorithm(paramType, &inputList, &outputList[0], numberOfOutputs);

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsAECSettled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsAECSettled(
    AECEngineProcessStatsOutput* pStatsOutput)
{
    BOOL settled = FALSE;
    if (NULL != pStatsOutput)
    {
        settled = pStatsOutput->algorithmOutput.frameInfo.aecSettled;
    }
    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Is AEC Settled = %d", settled);
    return settled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::UpdateAEStateBasedOnAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::UpdateAEStateBasedOnAlgo(
    AECEngineProcessStatsOutput* pStatsOutput)
{
    // Check the algo out to see if aec settled and update the output, ONLY if AE is not locked/Manual/Flash/Inactive
    if (AECState::Converged  == m_AECState ||
        AECState::Converging == m_AECState)
    {
        if (IsAECSettled(pStatsOutput))
        {
            SetAECState(AECState::Converged);
        }
        else
        {
            SetAECState(AECState::Converging);
        }

    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::ResetAEState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::ResetAEState()
{
    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AEC_DEBUG_CAPTURE: Reset AE after Preflash");
    SetAECState(AECState::Converging);
    SetAECPreFlashState(PreFlashStateInactive);
    SetFlashTrigger(CAECEngine::FlashTrigger::Invalid);
    m_snapshotIndicator = AECAlgoSnapshotNormal;
    SetLEDCalibrationState(LEDCalibrationState::Ready);
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsAFSettled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsAFSettled(
    AECEngineNodesUpdate* pNodesUpdate)
{
    BOOL focused = FALSE;
    if (TRUE == m_AECPreFlashSkipChecking)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Skipped checking IsAFSettled");
        m_AECPreFlashSkipChecking = FALSE;
    }
    else
    {

        if (NULL != pNodesUpdate)
        {
            if ((AFStatusFocused == pNodesUpdate->pAFOutput->status.status) ||
                (AFStatusNotFocused == pNodesUpdate->pAFOutput->status.status))
            {
                focused = TRUE;
            }
        }
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Is AF focused = %d", focused);
    }
    return focused;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsAWBSettled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsAWBSettled(
    AECEngineNodesUpdate* pNodesUpdate)
{
    BOOL settled = FALSE;

    if (TRUE == m_AECPreFlashSkipChecking)
    {
        m_AECPreFlashSkipChecking = FALSE;
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Skipped checking IsAWBSettled");
    }
    else
    {
        if (NULL != pNodesUpdate)
        {
            settled = (pNodesUpdate->pAWBOutputInternal->flashEstimationStatus == AWBFlashEstimationState::AWBEstimationDone);
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AWB Converged = %d, #frames: %d", settled, m_preflashFrameWaitCount);
        }
    }

    return settled;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsCaptureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsCaptureRequest(
    AECEngineHALParam* pHALParam)
{
    BOOL isCaptureRequest = FALSE;

    if (pHALParam != NULL)
    {
        if (ControlCaptureIntentStillCapture == pHALParam->captureIntent ||
            ControlCaptureIntentManual == pHALParam->captureIntent)
        {
            isCaptureRequest = TRUE;
        }
    }

    return isCaptureRequest;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsFlashGainsAvailable
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsFlashGainsAvailable()
{
    BOOL isFlashSnapshot = FALSE;

    GetFlashSnapshotGains();

    if (0.0f != m_flashFrameControl.exposureData[ExposureIndexShort].linearGain &&
        0.0f != m_flashFrameControl.exposureData[ExposureIndexShort].exposureTime)
    {
        isFlashSnapshot = TRUE;
    }

    return isFlashSnapshot;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsPreflashComplete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsPreflashComplete()
{
    BOOL isComplete = FALSE;

    if (ControlAEModeOnAutoFlashRedeye == m_HALParam.AEMode)
    {
        if (PreFlashStateRER == m_AECPreFlashState)
        {
            isComplete = m_RERDone;
        }
    }
    else if (PreFlashStateCompleteLED   == m_AECPreFlashState ||
        PreFlashStateCompleteNoLED == m_AECPreFlashState)
    {
        isComplete = TRUE;
    }

    return isComplete;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::SetOperationModeToAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::SetOperationModeToAlgo(
    AECAlgoOperationModeType operationMode)
{
    CamxResult result = CamxResultSuccess;
    CAMX_LOG_VERBOSE(CamxLogGroupAEC,
        "SetOperationModeToAlgo: dualled measurement on going for measurement %d  completedCount %d",
        m_LEDCalibration.result.measurementCount, m_LEDCalibration.completedCount);

    result = SetSingleParamToAlgorithm(AECAlgoSetParamOperationMode, &operationMode, sizeof(operationMode));
    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "SetOperationModeToAlgo: OldMode=%d NewMode=%d", m_operationMode, operationMode);
    if (CamxResultSuccess == result)
    {
        m_operationMode = operationMode;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::UpdateAEEngineOutputResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::UpdateAEEngineOutputResults(
    AECCommandOutputParam* pCommandOutput)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);

    AECEngineAlgorithmOutput* pAlgoOutput = &pCommandOutput->pProcessStatsOutput->algorithmOutput;

    m_snapshotIndicator                   = pAlgoOutput->frameInfo.snapshotIndicator;

    // Set the flag for Face AEC and LED assisted AF
    m_LEDFDDelay    = pAlgoOutput->frameInfo.LEDFDDelayRequired;
    m_LEDAFRequired = pAlgoOutput->frameInfo.LEDAFRequired;

    /* Update the Main flash gains for AWB when AWB has to get triggered */
    if (PreFlashStateTriggerAWB == m_AECPreFlashState)
    {
        pCommandOutput->pProcessStatsOutput->algorithmOutput.engineFrameControl.mainFlashFrameControl =
            m_flashFrameControl.frameControl;
    }

    UpdateAEStateBasedOnAlgo(pCommandOutput->pProcessStatsOutput);

    if (GetFlashTrigger() != CAECEngine::FlashTrigger::Invalid)
    {
        SetFlashTrigger(CAECEngine::FlashTrigger::Invalid); // Reset the flash trigger
    }

    // set the the engine state to output
    pCommandOutput->pProcessStatsOutput->AEState               = GetControlAEState();
    pCommandOutput->pProcessStatsOutput->AEMode                = m_HALParam.AEMode;
    pCommandOutput->pProcessStatsOutput->controlMode           = m_HALParam.controlMode;
    pCommandOutput->pProcessStatsOutput->preFlashState         = m_AECPreFlashState;
    pCommandOutput->pProcessStatsOutput->AECompensation        = m_HALParam.AECompensation;
    pCommandOutput->pProcessStatsOutput->AEPrecapTrigger       = static_cast<PrecapTriggers>(m_HALParam.AETrigger);
    pCommandOutput->pProcessStatsOutput->currentFPSRange.min   = m_HALParam.FPSRange.min;
    pCommandOutput->pProcessStatsOutput->currentFPSRange.max   = m_HALParam.FPSRange.max;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::DumpAEEngineOutputResults
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::DumpAEEngineOutputResults(
    const CHAR* pTriggerName,
    AECEngineAlgorithmOutput* pAlgoOutput)
{
    CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                     "AEC_DEBUG_OUTPUT: %s FrameControl: G ET (0:%f %ld)(1:%f %ld)(2:%f %ld) LuxInd=%f EngAEState=%d "
                     "PreFlashState=%d FlashType=%d LED 1:2=%d:%d",
                     pTriggerName,
                     pAlgoOutput->engineFrameControl.exposureData[ExposureIndexShort].linearGain,
                     pAlgoOutput->engineFrameControl.exposureData[ExposureIndexShort].exposureTime,
                     pAlgoOutput->engineFrameControl.exposureData[ExposureIndexLong].linearGain,
                     pAlgoOutput->engineFrameControl.exposureData[ExposureIndexLong].exposureTime,
                     pAlgoOutput->engineFrameControl.exposureData[ExposureIndexSafe].linearGain,
                     pAlgoOutput->engineFrameControl.exposureData[ExposureIndexSafe].exposureTime,
                     pAlgoOutput->engineFrameControl.frameControl.luxIndex,
                     m_AECState, m_AECPreFlashState,
                     pAlgoOutput->engineFrameControl.frameControl.flashState,
                     pAlgoOutput->engineFrameControl.LEDCurrents[LEDSetting1],
                     pAlgoOutput->engineFrameControl.LEDCurrents[LEDSetting2]);

    CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                     "AEC_DEBUG_OUTPUT: %s FrameInfo: Settled=%d snapshotInd=%d LEDAFReq=%d FL:%f",
                     pTriggerName, pAlgoOutput->frameInfo.aecSettled,
                     pAlgoOutput->frameInfo.snapshotIndicator, pAlgoOutput->frameInfo.LEDAFRequired,
                     pAlgoOutput->frameInfo.frameLuma);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::RestoreStreaming
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::RestoreStreaming(
    AECCommandOutputParam* pOutput)
{
    CamxResult result = CamxResultEInvalidState;
    /* When preflash completes, we need to do restore back to preview gains
       1. Put the algo back in preview mode
       2. Restore the "last preview gains" from algo -->
          this "restore gain" is what going to be applied for next frame right after preflash turns off */

    // Put back the algo to stream mode again and get the restore gains
    AECCommandInputParam   tmpCommandInput = { 0 };
    StatsStreamInitConfig  tmpStreamConfig = {};

    tmpCommandInput.pInitStreamConfig = &tmpStreamConfig;
    tmpCommandInput.pInitStreamConfig->operationMode = StatsOperationModeNormal;
    /* After preflash completes, algo will restore back the preview gains where it left off */
    result = StartStreaming(&tmpCommandInput,
        &pOutput->pProcessStatsOutput->algorithmOutput.engineFrameControl);
    DumpAEEngineOutputResults("AEFC:Restore", &pOutput->pProcessStatsOutput->algorithmOutput);
    result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetControlAEState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ControlAEStateValues CAECEngine::GetControlAEState()
{
    ControlAEStateValues controlAEStateRet = ControlAEStateInactive;
    // If AE Mode is off then ignore any HAL AEC State change requests
    if (m_HALParam.AEMode == ControlAEModeOff)
    {
        return ControlAEStateInactive;
    }
    // In case of precapture start, regardless of whatever state we are in, we always return AE State as PreCapture
    // Or if AETrigger is set in a frame then update state as Precapture for that frame
    if (m_precaptureWaitFrames > 0)
    {
        controlAEStateRet = ControlAEStatePrecapture;
        m_precaptureWaitFrames--;
        CAMX_LOG_VERBOSE(CamxLogGroupAEC,
            "be in preflash state: m_precaptureWaitFrames:%d", m_precaptureWaitFrames);
    }
    else if (TRUE == IsAELocked())
    {
        // Update Lock state only if AEC is not in Flash state or if Preflash sequence is complete
        if ((AECState::Flash != m_AECState) ||
            (TRUE == IsPreflashComplete()))
        {
            controlAEStateRet = ControlAEStateLocked;
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupAEC,
                "AE is locked and preflash not complete, yet not in Flash state and m_isPrecaptureInProgress == FALSE!");
            controlAEStateRet = ControlAEStatePrecapture;
        }
    }
    else
    {
        switch (m_AECState)
        {
            case AECState::Inactive:
            case AECState::Manual:
                controlAEStateRet = ControlAEStateInactive;
                break;
            case AECState::Converging:
               // controlAEStateRet = ControlAEStateSearching;
                if (TRUE == IsHalPreFlashSettingEnabled())
                {
                    controlAEStateRet = ControlAEStateFlashRequired;
                }
                else
                {
                    if (TRUE == m_isPrecaptureInProgress)
                    {
                        controlAEStateRet = ControlAEStatePrecapture;
                    }
                    else
                    {
                        controlAEStateRet = ControlAEStateSearching;
                    }
                }
                break;
            case AECState::Converged:
            {
                if (TRUE == IsHalPreFlashSettingEnabled())
                {
                    controlAEStateRet = ControlAEStateFlashRequired;
                }
                else
                {
                    controlAEStateRet = ControlAEStateConverged;
                }
                if (TRUE == m_isPrecaptureInProgress)
                {
                    m_isPrecaptureInProgress = FALSE;
                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AEC_DEBUG_CAPTURE: Precapture Completed");
                }
                break;
            }
            case AECState::Flash:
            {
                //   In case of Flash state:
                //   After preflash completes, if AE lock is ON  from HAL, Move to LOCKED State
                //   After preflash completes, if AE lock is OFF from HAL, then Move to FlashRequired or Converged State
                //   Else if PreFlash is still going, then just continue to be in PreCapture State

                if (TRUE == IsPreflashComplete())
                {
                    if (PreFlashStateRER != m_AECPreFlashState)
                    {
                        if (PreFlashStateCompleteLED == m_AECPreFlashState)
                        {
                            // Preflash sequence completed successfully -> New state will be based on Lock
                            controlAEStateRet = ControlAEStateFlashRequired;
                        }
                        else if (PreFlashStateCompleteNoLED == m_AECPreFlashState)
                        {
                            // Preflash sequence completed successfully & Flash Not required for snapshotS
                            controlAEStateRet = ControlAEStateConverged;
                        }
                        if (TRUE == m_isPrecaptureInProgress)
                        {
                            m_isPrecaptureInProgress = FALSE;
                            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AEC_DEBUG_CAPTURE: Precapture Completed");
                        }
                    }
                    else
                    {
                        controlAEStateRet = ControlAEStatePrecapture;
                    }
                }
                else
                {
                    // Preflash sequence is in progress still, just stay back in same old state
                    // and that should be always ControlAEStatePrecapture
                    CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                        "In Flash state and preflash not complete, yet m_isPrecaptureInProgress == FALSE!");
                    controlAEStateRet = ControlAEStatePrecapture;
                }
                break;
            }
            case AECState::LEDCalibration:
                controlAEStateRet = ControlAEStateConverged;
                break;
            default:
                CAMX_LOG_WARN(CamxLogGroupAEC, "ERROR: Current AE State = %s",
                    CamxAECEngineStateStrings[static_cast<UINT8>(m_AECState)]);
                break;
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "AECState = %s ControlAEState=%d isPrecaptureInProgress=%d",
        CamxAECEngineStateStrings[static_cast<UINT8>(m_AECState)], controlAEStateRet, m_isPrecaptureInProgress);

    return controlAEStateRet;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetFlashSnapshotGains
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetFlashSnapshotGains()
{
    CamxResult               result          = CamxResultSuccess;
    AECAlgoGetParamInputList inputList       = { 0 };
    AECAlgoGetParamInput     inputParams[1]  = {};
    AECAlgoSnapshotConfig    snapConfig      = {};

    snapConfig.snapshotType     = AECAlgoSnapshotFlash;
    snapConfig.bUseAEBracketing = FALSE;
    snapConfig.EVVal            = 0;

    inputList.numberOfAECGetInputParams = 1;
    inputList.pAECGetParamList          = inputParams;
    inputParams[0].inputType            = AECAlgoGetParamInputSnapshotType;
    inputParams[0].sizeOfInputData      = sizeof(snapConfig);
    inputParams[0].pInputData           = &snapConfig;

    Utils::Memset(&m_flashFrameControl, 0, sizeof(m_flashFrameControl));

    // Here we are getting main flash snapshot gains
    result = GetAlgoParamForFrameControlType(AECAlgoGetParamSnapshotExposure, &inputList, &m_flashFrameControl);

    CAMX_LOG_VERBOSE(CamxLogGroupAEC,
                     "AEC_DEBUG_OUTPUT: AEFC:Storing MainFlashGains: G=%f ET=%ld LuxInd=%f EngAEState=%d PreFlashState=%d "
                     " FlashType=%d LEDInfluenceRatio=%f RG:BG Ratio=%f:%f LED 1:2=%d:%d snapshotIndicator=%d",
                     m_flashFrameControl.exposureData[ExposureIndexShort].linearGain,
                     m_flashFrameControl.exposureData[ExposureIndexShort].exposureTime,
                     m_flashFrameControl.frameControl.luxIndex,
                     m_AECState, m_AECPreFlashState,
                     m_flashFrameControl.frameControl.flashState,
                     m_flashFrameControl.frameControl.LEDInfluenceRatio,
                     m_flashFrameControl.frameControl.LEDRGRatio,
                     m_flashFrameControl.frameControl.LEDBGRatio,
                     m_flashFrameControl.LEDCurrents[LEDSetting1],
                     m_flashFrameControl.LEDCurrents[LEDSetting2],
                     m_flashFrameControl.frameControl.snapshotIndicator);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngine::GetAECDebugData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetAECDebugData(
    AECCommandInputParam*  pCommandInput,
    AECCommandOutputParam* pCommandOutput)
{
    CamxResult               result          = CamxResultSuccess;
    AECAlgoGetParamInputList inputList       = { 0 };
    AECAlgoGetParamInput     inputParams[1]  = {};
    AECAlgoDebugDataConfig   debugDataConfig = {};

    debugDataConfig.debugData = pCommandInput->pAlgorithmInput->debugData;
    debugDataConfig.sizeofDebugDataWritten =
        pCommandOutput->pProcessStatsOutput->algorithmOutput.frameInfo.DebugDataSizeWritten;

    inputList.numberOfAECGetInputParams = 1;
    inputList.pAECGetParamList          = inputParams;
    inputParams[0].inputType            = AECAlgoGetParamInputDebugDataPointer;
    inputParams[0].sizeOfInputData      = sizeof(debugDataConfig);
    inputParams[0].pInputData           = &debugDataConfig;

    result = GetParamFromAlgorithm(AECAlgoGetParamDebugData, &inputList, NULL, 0);

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngineUtility START
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::CopyExposureSetToExposureData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngineUtility::CopyExposureSetToExposureData(
    AECAlgoExposureData* pData,
    INT32                size,
    AECAlgoExposureSet*  pSet)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
    AECAlgoExposureData* pExposureData = CAECEngineUtility::GetExposureDataByType(pSet, AECAlgoExposureShort);

    if ((NULL != pExposureData) &&
        (size >  AECAlgoExposureShort))
    {
        pData[AECAlgoExposureShort] = *pExposureData;
    }

    pExposureData = CAECEngineUtility::GetExposureDataByType(pSet, AECAlgoExposureLong);

    if ((NULL != pExposureData) &&
        (size >  AECAlgoExposureLong))
    {
        pData[AECAlgoExposureLong] = *pExposureData;
    }

    pExposureData = CAECEngineUtility::GetExposureDataByType(pSet, AECAlgoExposureSafe);

    if ((NULL != pExposureData) &&
        (size >  AECAlgoExposureSafe))
    {
        pData[AECAlgoExposureSafe] = *pExposureData;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::GetExposureDataByType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AECAlgoExposureData* CAECEngineUtility::GetExposureDataByType(
    AECAlgoExposureSet* pExposureSet,
    AECAlgoExposureType type)
{
    AECAlgoExposureData* pExposureData = NULL;

    for (UINT32 i = 0; i < pExposureSet->exposureInfoCount; i++)
    {
        if (pExposureSet->pExposureInfo[i].exposureType == type)
        {
            pExposureData = &pExposureSet->pExposureInfo[i].exposureData;

            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Exposure Time: %llu, gain: %f, sensitivity: %f, delta EV: %f",
                             pExposureData->exposureTime, pExposureData->linearGain, pExposureData->sensitivity,
                             pExposureData->deltaEVFromTarget);
            break;
        }
    }

    return pExposureData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::InitializeAlgorithmInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngineUtility::InitializeAlgorithmInput(
    AECAlgoInput* pInputs,
    UINT32        size)
{
    for (UINT32 i = 0; i < size; i++)
    {
        pInputs[i].inputType = AECAlgoInputInvalid;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::InitializeAlgorithmOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngineUtility::InitializeAlgorithmOutput(
    AECAlgoOutput* pOutputs,
    UINT32         size)
{
    for (UINT32 i = 0; i < size; i++)
    {
        pOutputs[i].outputType = AECAlgoOutputInvalid;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::SetAlgorithmInputEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngineUtility::SetAlgorithmInputEntry(
    AECAlgoInput*    pInputs,
    AECAlgoInputType inputType,
    UINT32           inputSize,
    VOID*            pValue)
{
    CAMX_ENTRYEXIT(CamxLogGroupAEC);
    pInputs[inputType].inputType        = inputType;
    pInputs[inputType].sizeOfAECInput   = inputSize;
    pInputs[inputType].pAECInput        = pValue;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::SetAlgorithmOutputEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngineUtility::SetAlgorithmOutputEntry(
    AECAlgoOutput*      pOutputs,
    AECAlgoOutputType   outputType,
    UINT32              outputSize,
    VOID*               pValue)
{
    pOutputs[outputType].outputType             = outputType;
    pOutputs[outputType].sizeOfAECOutput        = outputSize;
    pOutputs[outputType].pAECOutput             = pValue;
    pOutputs[outputType].sizeOfWrittenAECOutput = 0;

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::GetLEDMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AECAlgoLEDModeType CAECEngineUtility::GetLEDMode(
    const AECEngineHALParam* pHALParam)
{
    AECAlgoLEDModeType ledMode = AECAlgoLEDModeAuto;
    switch (pHALParam->AEMode)
    {
        case ControlAEModeOff:
        case ControlAEModeOn:
            ledMode = AECAlgoLEDModeOff;
            break;

        case ControlAEModeOnAutoFlash:
            ledMode = AECAlgoLEDModeAuto;
            break;

        case ControlAEModeOnAlwaysFlash:
            ledMode = AECAlgoLEDModeOn;
            break;

        case ControlAEModeOnAutoFlashRedeye:
            ledMode = AECAlgoLEDModeAuto;
            break;

        default:
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Unsupported Control Mode: %d",
                pHALParam->AEMode);
            break;
    }

    return ledMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngineUtility::GetAECAlgorithmMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AECAlgoModeType CAECEngineUtility::GetAECAlgorithmMode(
    AECEngineHALParam*  pHALParam)
{
    AECAlgoModeType algoMode = AECAlgoModeAuto;

    if (NULL != pHALParam)
    {
        if ((ControlAEModeOff == pHALParam->AEMode) ||
            (ControlModeOff == pHALParam->controlMode) ||
            (ControlModeOffKeepState == pHALParam->controlMode))
        {
            algoMode = AECAlgoModeManualFull;
        }
        else if (ISOExposureTimePriorityMode::ISOPriority == pHALParam->ISOExposureTimePriortyMode)
        {
            ISOMode isoMode = static_cast<ISOMode>(pHALParam->ISOorExposureTimePriorityValue);

            if (isoMode != ISOMode::ISOModeAuto && isoMode != ISOMode::ISOModeDeblur)
            {
                algoMode = AECAlgoModeManualISO;
            }
        }
        else if (ISOExposureTimePriorityMode::GainPriority == pHALParam->ISOExposureTimePriortyMode)
        {
            algoMode = AECAlgoModeManualGain;
        }
        else if (ISOExposureTimePriorityMode::ExposureTimePriority == pHALParam->ISOExposureTimePriortyMode)
        {
            algoMode = AECAlgoModeManualExpTime;
        }
    }

    return algoMode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::NormalizeROI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngineUtility::NormalizeROI(
    ::StatsDimension* pStatsWindowDimension,
    INT32*            pHALROI,
    AECAlgoROI*       pNormalizedROI)
{
    /// @todo (CAMX-1074) fill in code to normalize the ROI

    CAMX_UNREFERENCED_PARAM(pStatsWindowDimension);
    CAMX_UNREFERENCED_PARAM(pHALROI);
    CAMX_UNREFERENCED_PARAM(pNormalizedROI);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::LoadSetParamList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECEngineUtility::LoadSetParamList(
    AECAlgoSetParam*    pSetParam,
    VOID*               pAECSetParam,
    AECAlgoSetParamType type,
    SIZE_T              size)
{
    pSetParam->pAECSetParam      = pAECSetParam;
    pSetParam->setParamType      = type;
    pSetParam->sizeOfAECSetParam = static_cast<UINT32>(size);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::SetROIToSetParamList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngineUtility::SetROIToSetParamList(
    AECAlgoSetParam*    pSetParam,
    AECEngineHALParam*  pHALParam,
    AECAlgoROIList*     pROIList)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL == pSetParam)      ||
        (NULL == pHALParam)      ||
        (NULL == pROIList))
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid pointer SetParam: %p, HALParam: %p, ROIList: %p",
            pSetParam, pHALParam, pROIList);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        // Need to send 0 ROI Count to disable touch in algo
        pROIList->ROICount            = Utils::FEqual(pHALParam->touchROI.weight, 0.0f) ? 0 : 1;
        pROIList->ROIs                = &pHALParam->touchROI;

        pSetParam->pAECSetParam       = pROIList;
        pSetParam->setParamType       = AECAlgoSetParamTouchROI;
        pSetParam->sizeOfAECSetParam  = sizeof(AECAlgoROIList);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::SetTrackerROIToSetParamList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngineUtility::SetTrackerROIToSetParamList(
    AECAlgoSetParam*                pSetParam,
    AECEngineHALParam*              pHALParam,
    UnstabilizedROIInformation*     pROIList,
    StatsSensorInfo*                pStatsSensorInfo)
{
    CamxResult result = CamxResultSuccess;
    if ((NULL == pSetParam) ||
        (NULL == pHALParam) ||
        (NULL == pROIList))
    {

        CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid pointer SetParam: %p, HALParam: %p, ROIList: %p",
            pSetParam, pHALParam, pROIList);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        pROIList->roiCount = 0;
        for (UINT32 index = 0; index < pHALParam->trackerROI.ROICount; index++)
        {
            RectangleCoordinate* pRect = &pHALParam->trackerROI.ROI[index].rect;

            if (pRect->left + pRect->width > pStatsSensorInfo->sensorResWidth ||
                pRect->top + pRect->height > pStatsSensorInfo->sensorResHeight)
            {
                CAMX_LOG_WARN(CamxLogGroupAEC, "Ignoring invalid ROI SensorDim [%ux%u] < ROI[t:%u l:%u w:%u h:%u]",
                    pStatsSensorInfo->sensorResWidth,
                    pStatsSensorInfo->sensorResHeight,
                    pRect->top,
                    pRect->left,
                    pRect->width,
                    pRect->height);
                continue;
            }

            UINT32 i = pROIList->roiCount;
            pROIList->roi[i].roiID = pHALParam->trackerROI.ROI[index].id;
            pROIList->roi[i].rect.left = pRect->left;
            pROIList->roi[i].rect.top = pRect->top;
            pROIList->roi[i].rect.width = pRect->width;
            pROIList->roi[i].rect.height = pRect->height;
            pROIList->roi[i].weight = 0;
            pROIList->roiCount++;
        }

        pSetParam->pAECSetParam      = pROIList;
        pSetParam->setParamType      = AECAlgoSetParamTrackerROI;
        pSetParam->sizeOfAECSetParam = sizeof(UnstabilizedROIInformation);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::SetFaceROIToSetParamList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult  CAECEngineUtility::SetFaceROIToSetParamList(
    AECAlgoSetParam*    pSetParam,
    AECEngineHALParam*  pHALParam,
    AECAlgoROIList*     pROIList,
    StatsSensorInfo*    pStatsSensorInfo)
{
    CamxResult result = CamxResultSuccess;
    if ((NULL == pSetParam) ||
        (NULL == pHALParam) ||
        (NULL == pROIList))
    {

        CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid pointer SetParam: %p, HALParam: %p, ROIList: %p",
            pSetParam, pHALParam, pROIList);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        pROIList->ROICount = 0;
        for (UINT32 index = 0; index < pHALParam->faceROI.ROICount; index++)
        {
            RectangleCoordinate* pRect = &pHALParam->faceROI.stabilizedROI[index].faceRect;

            if (pRect->left + pRect->width > pStatsSensorInfo->sensorResWidth ||
                pRect->top + pRect->height > pStatsSensorInfo->sensorResHeight)
            {
                CAMX_LOG_ERROR(CamxLogGroupAEC, "Ignoring invalid ROI SensorDim [%ux%u] < ROI[t:%u l:%u w:%u h:%u]",
                    pStatsSensorInfo->sensorResWidth,
                    pStatsSensorInfo->sensorResHeight,
                    pRect->top,
                    pRect->left,
                    pRect->width,
                    pRect->height);
                continue;
            }

            pROIList->ROIs[pROIList->ROICount].x = static_cast<FLOAT> (pRect->left) /
                static_cast<FLOAT> (pStatsSensorInfo->sensorResWidth);
            pROIList->ROIs[pROIList->ROICount].y = static_cast<FLOAT> (pRect->top) /
                static_cast<FLOAT> (pStatsSensorInfo->sensorResHeight);
            pROIList->ROIs[pROIList->ROICount].dx = static_cast<FLOAT> (pRect->width) /
                static_cast<FLOAT> (pStatsSensorInfo->sensorResWidth);
            pROIList->ROIs[pROIList->ROICount].dy = static_cast<FLOAT> (pRect->height) /
                static_cast<FLOAT> (pStatsSensorInfo->sensorResHeight);
            pROIList->ROIs[pROIList->ROICount].weight = MaxROIWeight;
            pROIList->ROICount++;
        }

        pSetParam->pAECSetParam      = pROIList;
        pSetParam->setParamType      = AECAlgoSetParamFaceROI;
        pSetParam->sizeOfAECSetParam = sizeof(AECAlgoROIList);
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngineUtility::SetUniformFaceROIToSetParamList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngineUtility::SetUniformFaceROIToSetParamList(
    AECAlgoSetParam*                pSetParam,
    AECEngineHALParam*              pHALParam,
    UnstabilizedROIInformation*     pUnstabilizedFaceROIInformation,
    StatsSensorInfo*                pStatsSensorInfo)
{
    CamxResult result = CamxResultSuccess;
    if ((NULL == pSetParam) ||
        (NULL == pHALParam) ||
        (NULL == pUnstabilizedFaceROIInformation))
    {

        CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid pointer SetParam: %p, HALParam: %p, pUnstabilizedFaceROIInformation: %p",
            pSetParam, pHALParam, pUnstabilizedFaceROIInformation);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        pUnstabilizedFaceROIInformation->roiCount = 0;
        for (UINT32 index = 0; index < pHALParam->faceROI.ROICount; index++)
        {
            RectangleCoordinate* pRect = &pHALParam->faceROI.unstabilizedROI[index].faceRect;

            if (pRect->left + pRect->width > pStatsSensorInfo->sensorResWidth ||
                pRect->top + pRect->height > pStatsSensorInfo->sensorResHeight)
            {
                CAMX_LOG_ERROR(CamxLogGroupAEC, "Ignoring invalid ROI SensorDim [%ux%u] < ROI[t:%u l:%u w:%u h:%u]",
                    pStatsSensorInfo->sensorResWidth,
                    pStatsSensorInfo->sensorResHeight,
                    pRect->top,
                    pRect->left,
                    pRect->width,
                    pRect->height);
                continue;
            }

            // new face interface with unstablize information
            // use 3A uniform face sturcutre which allow 3A use common utils
            UINT32 i = pUnstabilizedFaceROIInformation->roiCount;
            pUnstabilizedFaceROIInformation->roi[i].roiID       = pHALParam->faceROI.unstabilizedROI[index].id;
            pUnstabilizedFaceROIInformation->roi[i].rect.left   = pRect->left;
            pUnstabilizedFaceROIInformation->roi[i].rect.top    = pRect->top;
            pUnstabilizedFaceROIInformation->roi[i].rect.width  = pRect->width;
            pUnstabilizedFaceROIInformation->roi[i].rect.height = pRect->height;
            pUnstabilizedFaceROIInformation->roi[i].weight      = 0;
            pUnstabilizedFaceROIInformation->roiCount++;
        }

        pSetParam->pAECSetParam      = pUnstabilizedFaceROIInformation;
        pSetParam->setParamType      = AECAlgoSetParamUnstabilizedFaceROI;
        pSetParam->sizeOfAECSetParam = sizeof(UnstabilizedROIInformation);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::SetFPSRangeToSetParamList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngineUtility::SetFPSRangeToSetParamList(
    const AECEngineHALParam* pHALParam,
    const INT32              index,
    AECAlgoSetParam*         pSetParam,
    AECAlgoFPSRange*         pFPSRange)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pSetParam) &&
        (NULL != pHALParam) &&
        (NULL != pFPSRange))
    {
        pFPSRange->minimumFPS              = static_cast<FLOAT>(pHALParam->FPSRange.min);
        pFPSRange->maximumFPS              = static_cast<FLOAT>(pHALParam->FPSRange.max);

        pSetParam[index].pAECSetParam      = pFPSRange;
        pSetParam[index].setParamType      = AECAlgoSetParamFPSRange;
        pSetParam[index].sizeOfAECSetParam = sizeof(AECAlgoFPSRange);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid input SetParam: %p, HALParam: %p, FPSRange: %p",
            pSetParam, pHALParam, pFPSRange);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::SetLEDModeToSetParamList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngineUtility::SetLEDModeToSetParamList(
    const AECEngineHALParam* pHALParam,
    const INT32              index,
    AECAlgoSetParam*         pSetParam,
    AECAlgoLEDModeType*      pLEDMode)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pSetParam) &&
        (NULL != pHALParam) &&
        (NULL != pLEDMode))
    {
        *pLEDMode                          = CAECEngineUtility::GetLEDMode(pHALParam);
        pSetParam[index].pAECSetParam      = pLEDMode;
        pSetParam[index].setParamType      = AECAlgoSetParamLEDMode;
        pSetParam[index].sizeOfAECSetParam = sizeof(AECAlgoLEDModeType);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid input SetParam: %p, HALParam: %p, LEDMode: %p",
            pSetParam, pHALParam, pLEDMode);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::SetManualSettingToSetParamList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngineUtility::SetManualSettingToSetParamList(
    AECEngineHALParam* pHALParam,
    const INT32              index,
    AECAlgoSetParam*         pSetParam,
    AECAlgoManualSetting*    pManual)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pSetParam) &&
        (NULL != pHALParam) &&
        (NULL != pManual))
    {
        pManual->exposureTime              = pHALParam->exposureTime;
        pManual->ISOValue                  = pHALParam->sensitivity;
        pManual->gain                      = pHALParam->gain;

        pSetParam[index].pAECSetParam      = pManual;
        pSetParam[index].setParamType      = AECAlgoSetParamManualSetting;
        pSetParam[index].sizeOfAECSetParam = sizeof(AECAlgoManualSetting);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid input pSetParam: %p, pHALParam: %p, pManual: %p",
            pSetParam, pHALParam, pManual);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngineUtility::SetLockToSetParamList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult   CAECEngineUtility::SetLockToSetParamList(
    const AECEngineHALParam* pHALParam,
    const INT32              index,
    AECAlgoSetParam*         pSetParam,
    AECAlgoLockType*         pLock)
{
    CamxResult result = CamxResultSuccess;
    if (NULL != pSetParam && NULL != pHALParam && NULL != pLock)
    {
        if (ControlAELockOn == pHALParam->AELock)
        {
            *pLock = AECAlgoLockOn;
        }
        else
        {
            *pLock = AECAlgoLockOFF;
        }

        pSetParam[index].pAECSetParam      = pLock;
        pSetParam[index].setParamType      = AECAlgoSetParamAECLock;
        pSetParam[index].sizeOfAECSetParam = sizeof(AECAlgoLockType);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Invalid input pSetParam: %p, pHALParam: %p, pLock: %p",
            pSetParam, pHALParam, pLock);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CAECEngineUtility::SetAECAlgoDiagConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngineUtility::SetAECAlgoDiagConfig(
    AECAlgoDiagConfig*       pDiagConfig,
    AECAlgoSetParam*         pSetParam,
    AECState                 state)
{
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "enable=%x aec_state=%s",
        pStaticSettings->enableAEScan, CamxAECEngineStateStrings[static_cast<UINT8>(state)]);
    if (0 != pStaticSettings->enableAEScan && AECState::Converged == state)
    {
        UINT i;
        for (i = 0; i < AECAlgoTestCaseCount; i++)
        {
            pDiagConfig->testCaseEnable[i] = (pStaticSettings->enableAEScan & (1 << i)) ? TRUE : FALSE;
        }
        CAECEngineUtility::LoadSetParamList(pSetParam,
            pDiagConfig,
            AECAlgoSetParamSetDiagTestConfig,
            sizeof(AECAlgoDiagConfig));
        return CamxResultSuccess;
    }
    return CamxResultEFailed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::TimedWaitForSettle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::TimedWaitForSettle(
    UINT32 waitFramesLimit)
{
    BOOL timedOut = FALSE;

    if (waitFramesLimit > m_preflashFrameWaitCount)
    {
        m_preflashFrameWaitCount++;
    }
    else
    {
        timedOut = TRUE;
    }
    return timedOut;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::StoreLEDCalibrationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECEngine::StoreLEDCalibrationData()
{
    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "inside StoreLEDCalibrationData mode is %d",
        m_LEDCalibration.mode);

    CHAR        pFilePath[FILENAME_MAX];
    FILE*       pOutFile  = NULL;
    const CHAR* pFileName = NULL;

    // Write to a tuning or calibration file depending on whichof the two operations was just performed
    pFileName = (AECAlgoFlashMeasKindTuning == m_LEDCalibration.mode ? pLEDTuningFilename : pLEDCalibrationFilename);
    OsUtils::SNPrintF(pFilePath,
                      FILENAME_MAX,
                      "%s%s%s",
                      ConfigFileDirectory,
                      PathSeparator,
                      pFileName);

    pOutFile = OsUtils::FOpen(pFilePath, "w");
    if (NULL != pOutFile)
    {
        UINT32 calibrationCount = static_cast<UINT32>(m_LEDCalibration.result.measurementCount);

        // Write the count of calibration points
        if (AECAlgoFlashMeasKindCalib == m_LEDCalibration.mode)
        {
            OsUtils::FWrite(&calibrationCount, sizeof(m_LEDCalibration.config.measurementsCount), 1, pOutFile);
        }
        else
        {
            OsUtils::FPrintF(pOutFile, "LED1 Current,LED2 Current,RG,BG,flux\n");
        }

        // Write each calibration point to file
        for (UINT32 i = 0; i < calibrationCount; i++)
        {
            FlashMeasurement* pPoint = &m_LEDCalibration.result.pFlashPoints[i];

            // Write RG Ratio, BG Ratio, followed by flux
            if (AECAlgoFlashMeasKindTuning == m_LEDCalibration.mode)
            {
                UINT32 LED1 = (NULL == m_LEDCalibration.pLEDCurTable) ? 0 : m_LEDCalibration.pLEDCurTable[i * 2];
                UINT32 LED2 = (NULL == m_LEDCalibration.pLEDCurTable) ? 0 : m_LEDCalibration.pLEDCurTable[i * 2 + 1];
                OsUtils::FPrintF(pOutFile, "%d,%d,%f,%f,%f\n",
                    LED1, LED2, pPoint->RGRatio, pPoint->BGRatio, pPoint->flux);
            }
            else
            {
                OsUtils::FWrite(&pPoint->RGRatio, sizeof(pPoint->RGRatio), 1, pOutFile);
                OsUtils::FWrite(&pPoint->BGRatio, sizeof(pPoint->BGRatio), 1, pOutFile);
                OsUtils::FWrite(&pPoint->flux, sizeof(pPoint->flux), 1, pOutFile);
            }
        }

        OsUtils::FClose(pOutFile);
    }
    else
    {
        CHAR errnoStr[100] = {0};
        OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
        CAMX_LOG_ERROR(CamxLogGroupAEC, "Error! (%s) Failed to write the dual LED calibration data to file", errnoStr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::StoreLEDInlineCalibrationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID CAECEngine::StoreLEDInlineCalibrationData()
{
    CHAR        pFilePath[FILENAME_MAX];
    FILE*       pOutFile = NULL;

    OsUtils::SNPrintF(pFilePath,
        FILENAME_MAX,
        "%s%s%s",
        ConfigFileDirectory,
        PathSeparator,
        pLEDInlineCalibrationFilename);

    pOutFile = OsUtils::FOpen(pFilePath, "wb");
    if (NULL != pOutFile)
    {
        CAMX_LOG_VERBOSE(
            CamxLogGroupAEC,
            "inside StoreLEDInlineCalibrationData mode is %d, m_LEDCalibration.dynamicDataSize %d, sizeof %d",
            m_LEDCalibration.mode, m_LEDCalibration.dynamicDataSize, sizeof(m_LEDCalibration.pDynamicData));

        OsUtils::FWrite(m_LEDCalibration.pDynamicData, m_LEDCalibration.dynamicDataSize, 1, pOutFile);
        OsUtils::FClose(pOutFile);
    }
    else
    {
        CHAR errnoStr[100] = { 0 };
        OsUtils::StrError(errnoStr, sizeof(errnoStr), errno);
        CAMX_LOG_ERROR(CamxLogGroupAEC,
            "Error! (%s) Failed to write the dynamic inline LED calibration data to file", errnoStr);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::LoadLEDCalibrationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::LoadLEDCalibrationData()
{
    CamxResult result = CamxResultSuccess;

    CHAR    pFilePath[FILENAME_MAX];
    FILE*   pInFile = NULL;

    OsUtils::SNPrintF(pFilePath,
                      FILENAME_MAX,
                      "%s%s%s",
                      ConfigFileDirectory,
                      PathSeparator,
                      pLEDCalibrationFilename);

    pInFile = OsUtils::FOpen(pFilePath, "r");
    if (NULL != pInFile)
    {
        AECAlgoFlashCalibrationData data = { 0 };

        OsUtils::FRead(&data.count, sizeof(data.count), sizeof(data.count), 1, pInFile);
        if (data.count == 0)
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "No dual LED calibration data to read");
        }
        else
        {
            // For now flux is read but is discarded
            FLOAT dummyFlux = 0.0f;
            data.pRGRatios = reinterpret_cast<FLOAT*>(CAMX_CALLOC(sizeof(FLOAT) * data.count));
            data.pBGRatios = reinterpret_cast<FLOAT*>(CAMX_CALLOC(sizeof(FLOAT) * data.count));

            if (NULL != data.pRGRatios &&
                NULL != data.pBGRatios)
            {
                // Read in the RG/BG ratios and flux for all calibration data points. Flux is not used here at this point
                for (UINT32 i = 0; i < data.count; i++)
                {
                    OsUtils::FRead(&data.pRGRatios[i], sizeof(FLOAT), sizeof(FLOAT), 1, pInFile);
                    OsUtils::FRead(&data.pBGRatios[i], sizeof(FLOAT), sizeof(FLOAT), 1, pInFile);
                    OsUtils::FRead(&dummyFlux, sizeof(FLOAT), sizeof(FLOAT), 1, pInFile);
                }

                result = SetSingleParamToAlgorithm(AECAlgoSetParamFlashCalibrationData, &data, sizeof(data));
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupAEC, "Error! Failed to set the Dual LED calibration data to the algorithm");
                }

                CAMX_FREE(data.pRGRatios);
                CAMX_FREE(data.pBGRatios);
                data.pRGRatios = NULL;
                data.pBGRatios = NULL;
            }
        }

        OsUtils::FClose(pInFile);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Dual LED calibration data file not present!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::LoadLEDInlineCalibrationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::LoadLEDInlineCalibrationData() {
    CamxResult result = CamxResultSuccess;

    CHAR pFilePath[FILENAME_MAX];
    FILE*   pInFile = NULL;

    OsUtils::SNPrintF(pFilePath,
        FILENAME_MAX,
        "%s%s%s",
        ConfigFileDirectory,
        PathSeparator,
        pLEDInlineCalibrationFilename);

    // allocate space for dynamic inline data
    if (NULL == m_LEDCalibration.pDynamicData)
    {
        m_LEDCalibration.dynamicDataSize = 0;
        m_LEDCalibration.pDynamicData = CAMX_CALLOC(m_pStaticSettings->dynamicCalibrationMaxSize);
        if (NULL == m_LEDCalibration.pDynamicData)
        {
            CAMX_LOG_ERROR(CamxLogGroupAEC, "Couldnt allocate memory for dynamic inline data");
            result = CamxResultENoMemory;
        }
    }
    else
    {
        Utils::Memset(m_LEDCalibration.pDynamicData, 0, m_pStaticSettings->dynamicCalibrationMaxSize);
    }

    if (CamxResultSuccess == result)
    {
        // Set AEC algorithm to run as inline calibration mode
        result = SetSingleParamToAlgorithm
                (AECAlgoSetParamFlashInlineCalibrationMode,
                &m_isDynamicInlineCalibration,
                sizeof(m_isDynamicInlineCalibration));

        // open and read as binary file
        pInFile = OsUtils::FOpen(pFilePath, "rb");

        if (NULL != pInFile)
        {
            INT64 fileSize = 0;

            // get the file size to read contents into a char array
            OsUtils::FSeek(pInFile, 0, SEEK_END);

            fileSize = OsUtils::FTell(pInFile);

            OsUtils::FSeek(pInFile, 0, SEEK_SET);
            OsUtils::FRead(m_LEDCalibration.pDynamicData, fileSize, fileSize, 1, pInFile);

            m_LEDCalibration.dynamicDataSize = static_cast<UINT32>(fileSize);

            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "dynamicDataSize %d", m_LEDCalibration.dynamicDataSize);

            // send the data to algo as single param
            result = SetSingleParamToAlgorithm
            (AECAlgoSetParamFlashInlineCalibrationData,
             m_LEDCalibration.pDynamicData,
             m_LEDCalibration.dynamicDataSize);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupAEC,
                    "Error! Failed to set the dynamic inline calibration data to the algorithm");
            }
            OsUtils::FClose(pInFile);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "dynamic inline calibration data file not present!");
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAEC, "LoadLEDInlineCalibrationData failed. Result: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::GetLEDCalibrationConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::GetLEDCalibrationConfig()
{
    CamxResult result = CamxResultSuccess;

    if (TRUE == m_isDynamicInlineCalibration)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Dual LED Dynamic Inline Calibration is enabled");
    }
    else if (AECAlgoFlashMeasKindOff == m_LEDCalibration.mode)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Dual LED Calibration is disabled");
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Dual LED Calibration is enabled for %s",
            (AECAlgoFlashMeasKindTuning == m_LEDCalibration.mode ? "Tuning" : "Calibration"));
    }

    // Set the flash measurement mode(tuning vs calibration)
    result = SetSingleParamToAlgorithm(AECAlgoSetParamFlashMeasurement, &m_LEDCalibration.mode,
                                        sizeof(m_LEDCalibration.mode));

    // Query the algorithm for calibration configuration if LED calibration is turned on
    if ((CamxResultSuccess == result) &&
        (AECAlgoFlashMeasKindOff != m_LEDCalibration.mode))
    {
        AECAlgoGetParamOutput outputList[1];

        outputList[0].outputType = AECAlgoGetParamOutputFlashMeasurementConfig;
        outputList[0].pOutputData = &m_LEDCalibration.config;
        outputList[0].sizeOfOutputData = sizeof(m_LEDCalibration.config);
        outputList[0].sizeOfWrittenOutputData = 0;

        // Get the flash measurement configuration
        result = GetParamFromAlgorithm(AECAlgoGetParamFlashMeasurementConfig, NULL, &outputList[0], 1);
    }

    // query the algorithm for dynamic inline calibration data if inline calibration is turned on
    else if ((CamxResultSuccess == result) &&
        (TRUE == m_isDynamicInlineCalibration))
    {
        AECAlgoGetParamOutput outputList[1];
        outputList[0].outputType                    = AECAlgoGetParamOutputDynamicCalibrationData;
        outputList[0].pOutputData                   = m_LEDCalibration.pDynamicData;

        // max size of output data from algo
        outputList[0].sizeOfOutputData              = m_pStaticSettings->dynamicCalibrationMaxSize;

        // need size of dynamic data from algo
        outputList[0].sizeOfWrittenOutputData       = m_LEDCalibration.dynamicDataSize;

        // Get the calibration data
        result = GetParamFromAlgorithm(AECAlgoGetParamDynamicCalibrationData, NULL, &outputList[0], 1);
        m_LEDCalibration.dynamicDataSize = outputList[0].sizeOfWrittenOutputData;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::ConfigLEDCalibration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult CAECEngine::ConfigLEDCalibration(
    AECEngineFrameControl* pFrameControl,
    AECEngineProcessStatsOutput* pOutput)
{
    CamxResult result = CamxResultSuccess;

    if (LEDCalibrationState::Ready == GetLEDCalibrationState())
    {
        // Need changes here: Turn on flash through FrameControl changes
        SetOperationModeToAlgo(AECAlgoOperationModeFlashMeasurement);
        GetAlgoParamForFrameControlType(AECAlgoGetParamStartExposure, NULL, pFrameControl);

        // create table to save LED currents
        if (NULL != m_LEDCalibration.pLEDCurTable)
        {
            CAMX_DELETE(m_LEDCalibration.pLEDCurTable);
            m_LEDCalibration.pLEDCurTable = NULL;
        }
        if (m_LEDCalibration.config.measurementsCount > 0)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "creating table");
            m_LEDCalibration.pLEDCurTable = CAMX_NEW UINT32[m_LEDCalibration.config.measurementsCount * 2];
        }
        if (NULL != m_LEDCalibration.pLEDCurTable &&
            m_LEDCalibration.completedCount < m_LEDCalibration.config.measurementsCount)
        {
            m_LEDCalibration.pLEDCurTable[m_LEDCalibration.completedCount * 2]     = pFrameControl->LEDCurrents[0];
            m_LEDCalibration.pLEDCurTable[m_LEDCalibration.completedCount * 2 + 1] = pFrameControl->LEDCurrents[1];
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "currents %d %d", pFrameControl->LEDCurrents[0], pFrameControl->LEDCurrents[1]);
        }

        pOutput->calibFlashState                                   = CalibrationFlashReady;
        pOutput->algorithmOutput.engineFrameControl.LEDCurrents[0] = pFrameControl->LEDCurrents[0];
        pOutput->algorithmOutput.engineFrameControl.LEDCurrents[1] = pFrameControl->LEDCurrents[1];
        SetLEDCalibrationState(LEDCalibrationState::Collecting);
        pFrameControl->frameControl.flashState = AECAlgoFlashStateCalibaration;
    }
    // LED calibration is ongoing for the current measurement point
    else if (LEDCalibrationState::Collecting == GetLEDCalibrationState())
    {
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "2:m_LEDCalibrationState is %d!", m_LEDCalibrationState);
        pFrameControl->frameControl.flashState = AECAlgoFlashStateCalibaration;
        AECAlgoGetParamOutput outputList[1];
        outputList[0].outputType              = AECAlgoGetParamOutputFlashMeasurementResult;
        outputList[0].pOutputData             = &m_LEDCalibration.result;
        outputList[0].sizeOfOutputData        = sizeof(m_LEDCalibration.result);
        outputList[0].sizeOfWrittenOutputData = 0;

        // Get the flash measurement result for the current measurement in progress
        result = GetParamFromAlgorithm(AECAlgoGetParamFlashMeasurementResult, NULL, &outputList[0], 1);
        CAMX_LOG_VERBOSE(CamxLogGroupAEC, "1:m_LEDCalibration.result.status is %d! result %d",
            m_LEDCalibration.result.status, result);

        // Stop calibration/tuning if we have measured all the points required, or if calibration fails
        if (CamxResultEFailed == result ||
            AECAlgoFlashMeasStatusOngoing != m_LEDCalibration.result.status)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "3:m_LEDCalibrationState is %d  FlashPoint result status is %d!",
                m_LEDCalibrationState,
                m_LEDCalibration.result.pFlashPoints->status);

            SetLEDCalibrationState(LEDCalibrationState::Complete);
            pOutput->calibFlashState               = CalibrationFlashComplete;
            pFrameControl->frameControl.flashState = AECAlgoFlashStateOff;

            if (CamxResultSuccess == result &&
                AECAlgoFlashMeasStatusPass == m_LEDCalibration.result.status)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Dual LED calibration succeeded!");
                StoreLEDCalibrationData();
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupAEC, "ERROR! Dual LED calibration failed");
            }

            // delete table to store LED currents
            CAMX_DELETE(m_LEDCalibration.pLEDCurTable);
            m_LEDCalibration.pLEDCurTable = NULL;

            m_LEDCalibration.mode = AECAlgoFlashMeasKindOff;
            result                = SetOperationModeToAlgo(AECAlgoOperationModeStreaming);
            GetAlgoParamForFrameControlType(AECAlgoGetParamStartExposure, NULL, pFrameControl);
            pFrameControl->frameControl.flashState = AECAlgoFlashStateCalibaration;
        }
        else
        {
            INT32 idx = m_LEDCalibration.completedCount;
            CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Dual LED calibration measurement on going for measurement %d status %d",
                idx,
                m_LEDCalibration.result.pFlashPoints[idx].status);

            if (AECAlgoFlashMeasStatusOngoing !=
                m_LEDCalibration.result.pFlashPoints[idx].status)
            {
                // This indicates that we just completed another measurement. Cycle through the process if not done
                // and turn off flash & reconfigure with new currents

                SetOperationModeToAlgo(AECAlgoOperationModeFlashMeasurement);
                GetAlgoParamForFrameControlType(AECAlgoGetParamStartExposure, NULL, pFrameControl);
                SetLEDCalibrationState(LEDCalibrationState::PartialComplete);
                pOutput->calibFlashState = CalibrationPartialComplete;

                pOutput->algorithmOutput.engineFrameControl.LEDCurrents[0] = pFrameControl->LEDCurrents[0];
                pOutput->algorithmOutput.engineFrameControl.LEDCurrents[1] = pFrameControl->LEDCurrents[1];

                CAMX_LOG_VERBOSE(CamxLogGroupAEC, "node completed count %d set to match core completed count %d",
                    m_LEDCalibration.completedCount,
                    m_LEDCalibration.result.measurementCount);

                m_LEDCalibration.completedCount = m_LEDCalibration.result.measurementCount;
                if (NULL != m_LEDCalibration.pLEDCurTable &&
                    m_LEDCalibration.completedCount < m_LEDCalibration.config.measurementsCount)
                {
                    m_LEDCalibration.pLEDCurTable[m_LEDCalibration.completedCount * 2] = pFrameControl->LEDCurrents[0];
                    m_LEDCalibration.pLEDCurTable[m_LEDCalibration.completedCount * 2 + 1] = pFrameControl->LEDCurrents[1];
                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "currents %d %d",
                        pFrameControl->LEDCurrents[0], pFrameControl->LEDCurrents[1]);
                }
            }
            else if (AECAlgoFlashMeasStatusBreak !=
                m_LEDCalibration.result.pFlashPoints[idx].status)
            {
                m_LEDCalibrationState                  = LEDCalibrationState::Collecting;
                pOutput->calibFlashState               = CalibrationFlashCollecting;
                pFrameControl->frameControl.flashState = AECAlgoFlashStateCalibaration;

                if (NULL != m_LEDCalibration.pLEDCurTable &&
                    m_LEDCalibration.completedCount < m_LEDCalibration.config.measurementsCount)
                {
                    pOutput->algorithmOutput.engineFrameControl.LEDCurrents[0] =
                        m_LEDCalibration.pLEDCurTable[m_LEDCalibration.completedCount * 2];
                    pOutput->algorithmOutput.engineFrameControl.LEDCurrents[1] =
                        m_LEDCalibration.pLEDCurTable[m_LEDCalibration.completedCount * 2 + 1];

                    CAMX_LOG_VERBOSE(CamxLogGroupAEC, "Ongoing calibration, setting currents: %d %d",
                        pFrameControl->LEDCurrents[0], pFrameControl->LEDCurrents[1]);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupAEC, "completedCount %d measurementsCount %d",
                        m_LEDCalibration.completedCount, m_LEDCalibration.config.measurementsCount);
                }
            }
        }
    }
    else if (LEDCalibrationState::PartialComplete == m_LEDCalibrationState)
    {
        pOutput->calibFlashState = CalibrationPartialComplete;
        m_LEDCalibrationState = LEDCalibrationState::Collecting;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupAEC, " flashState is %d!, calibFlashState %d!",
        pFrameControl->frameControl.flashState, pOutput->calibFlashState);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsZSLDisabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsZSLDisabled()
{
    // ZSL will be enabled/disabled from camxsetting and from HAL

    // From camx settings
    BOOL isZSLDisabled = (HwEnvironment::GetInstance()->GetStaticSettings()->overrideDisableZSL == 1);

    // CONTROL_ENABLE_ZSL allow camera device to enable zero-shutter-lag mode for requests with
    // android.control.captureIntent == STILL_CAPTURE.
    if (ControlCaptureIntentStillCapture == m_HALParam.captureIntent)
    {
        // Disable ZSL, if it is disabled from settings or from hal
        isZSLDisabled |= (m_HALParam.controlZslEnable == 0);
    }

    return isZSLDisabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// CAECEngine::IsQuerySnapshotExposure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL CAECEngine::IsQuerySnapshotExposure()
{
    BOOL isHDRCapture   = FALSE;
    BOOL isLLSCapture   = FALSE;

    if (FALSE == IsCaptureRequest(&m_HALParam))
    {
        return FALSE;
    }

    // In ZSL mode, AEC shouldn't query snapshot exposure instead it should use preview exposure & gain
    // In other cases like HDR/LLS, it should query snapshot exposure
    if ((ControlSceneModeHdr == m_HALParam.controlSceneMode) || (TRUE == m_HALParam.AEBracketMode))
    {
        isHDRCapture = TRUE;
    }
    else
    {
        isHDRCapture = FALSE;
    }
    isLLSCapture = (AECAlgoSnapshotLLS == m_snapshotIndicator) ? TRUE : FALSE;

    return (isHDRCapture || isLLSCapture);
}

CAMX_NAMESPACE_END
