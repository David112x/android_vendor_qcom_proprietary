////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxafstatemachine.cpp
/// @brief The class that implements AF State Machine
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxafstatemachine.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AFStateMachine::AFStateMachine
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AFStateMachine::AFStateMachine()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AFStateMachine::GetControlCommand
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AFControlCommand AFStateMachine::GetControlCommand() const
{
    AFControlCommand command = AFControlCommand::AFControlCommandTypeMax;

    switch (m_AFState)
    {
        case AFState::Inactive:
            command = AFControlCommand::AFStopCommand;
            break;
        case AFState::PassiveScan:
            command = AFControlCommand::AFUnLockCommand;
            break;
        case AFState::PassiveFocused:
            command = AFControlCommand::AFUnLockCommand;
            break;
        case AFState::ActiveScan:
            command = AFControlCommand::AFStartCommand;
            break;
        case AFState::FocusedLocked:
            command = AFControlCommand::AFLockCommand;
            break;
        case AFState::NotFocusedLocked:
            command = AFControlCommand::AFLockCommand;
            break;
        case AFState::PassiveUnfocused:
            command = AFControlCommand::AFUnLockCommand;
            break;
        case AFState::PassiveScanWithTrigger:
            command = AFControlCommand::AFUnLockCommand;
            break;
        case AFState::Invalid:
            command = AFControlCommand::AFUnLockCommand;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid AF state: %s", CamxAFStateStrings[static_cast<UINT8>(m_AFState)]);
            break;
    }

    return command;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AFStateMachine::GetControlAFState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ControlAFStateValues AFStateMachine::GetControlAFState() const
{
    ControlAFStateValues state = ControlAFStateInactive;

    switch (m_AFState)
    {
        case AFState::Inactive:
            state = ControlAFStateInactive;
            break;
        case AFState::PassiveScan:
            state = ControlAFStatePassiveScan;
            break;
        case AFState::PassiveFocused:
            state = ControlAFStatePassiveFocused;
            break;
        case AFState::ActiveScan:
            state = ControlAFStateActiveScan;
            break;
        case AFState::FocusedLocked:
            state = ControlAFStateFocusedLocked;
            break;
        case AFState::NotFocusedLocked:
            state = ControlAFStateNotFocusedLocked;
            break;
        case AFState::PassiveUnfocused:
            state = ControlAFStatePassiveUnfocused;
            break;
        case AFState::PassiveScanWithTrigger:
            state = ControlAFStatePassiveScan;
            break;
        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid Af state: %s", CamxAFStateStrings[static_cast<UINT8>(m_AFState)]);
            break;
    }

    return state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AFStateMachine::SetAFFocusAndRunMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AFStateMachine::SetAFFocusAndRunMode(
    AFFocusMode focusMode,
    AFRunMode   runMode)
{
    AFStateTransitionType transitionType = AFStateTransitionType::EventModeChange;

    // If there is a change in run mode and if we are
    // transitioning from snapshot mode or to a snapshot mode,
    // then we need not handle any state change
    if ((m_runMode != runMode) && ((AFRunModeSnapshot == m_runMode) || (AFRunModeSnapshot == runMode)))
    {
        m_focusMode = focusMode;
        m_runMode   = runMode;
    }
    // If Focus mode changes or if run mode changes, then  we need to
    // consider it as a mode change
    else if ((m_focusMode != focusMode) || (m_runMode != runMode))
    {
        // This can be a CAF mode change unless we are exiting or entering to
        // snapshot mode
        if ((m_runMode != runMode) && (AFRunModeSnapshot != m_runMode) && (AFRunModeSnapshot != runMode))
        {
            transitionType = AFStateTransitionType::EventCAFModeChange;
        }

        if ((m_focusMode == AFFocusModeContinuousPicture && focusMode == AFFocusModeContinuousVideo) ||
            (m_focusMode == AFFocusModeContinuousVideo && focusMode == AFFocusModeContinuousPicture))
        {
            transitionType = AFStateTransitionType::EventCAFModeChange;
        }

        m_focusMode  = focusMode;
        m_runMode    = runMode;

        HandleAFStateTransition(transitionType);

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AFStateMachine::ProcessAFStatusUpdate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID AFStateMachine::ProcessAFStatusUpdate(
    AFAlgoStatusType focusStatus)
{
    BOOL                  stateTransition   = TRUE;
    AFStateTransitionType cause             = AFStateTransitionType::EventStartScan;

    if (m_focusStatus != focusStatus)
    {
        switch (focusStatus)
        {
            case AFAlgoStatusTypeFocused:
                CAMX_TRACE_ASYNC_END_F(CamxLogGroupAF, 0, "AF: Convergence");
                cause = AFStateTransitionType::EventFocusDoneSuccess;
                break;
            case AFAlgoStatusTypeNotFocused:
                CAMX_TRACE_ASYNC_END_F(CamxLogGroupAF, 0, "AF: Convergence");
                cause = AFStateTransitionType::EventFocusDoneFailure;
                break;
            case AFAlgoStatusTypeFocusing:
                cause = AFStateTransitionType::EventStartScan;
                break;
            case AFAlgoStatusTypeInitialized:
            default:
                stateTransition = FALSE;
                break;
        }

        m_focusStatus = focusStatus;

        if (TRUE == stateTransition)
        {
            HandleAFStateTransition(cause);
        }

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AFStateMachine::GetModeIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
AFInternalFocusMode AFStateMachine::GetModeIndex() const
{
    AFInternalFocusMode modeIndex = AFInternalFocusMode::NonContinuous;

    switch (m_focusMode)
    {
        case AFFocusModeContinuousPicture:
            modeIndex = AFInternalFocusMode::ContinuousPicture;
            break;
        case AFFocusModeContinuousVideo:
            modeIndex = AFInternalFocusMode::ContinuousVideo;
            break;
        case AFFocusModeManual:
        case AFFocusModeInifinity:
            modeIndex = AFInternalFocusMode::ManualORInfinity;
            break;
        default:
            break;
    }
    return modeIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// AFStateMachine::HandleAFStateTransition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AFStateMachine::HandleAFStateTransition(
    AFStateTransitionType event)
{
    AFState newState        = m_AFState;
    BOOL    stateTransition = FALSE;
    UINT32  modeIndex       = static_cast<UINT32>(GetModeIndex());

    if (event < AFStateTransitionType::Max)
    {
        newState = AFStateTable[modeIndex][static_cast<UINT32>(m_AFState)][static_cast<UINT32>(event)];
        CAMX_LOG_INFO(CamxLogGroupAF, "m_focusMode: %d, InternalMode: %s, m_AFState: %s, event: %s, newState: %s",
            m_focusMode,
            CamxAFInternalFocusModeStrings[static_cast<UINT8>(modeIndex)],
            CamxAFStateStrings[static_cast<UINT8>(m_AFState)],
            CamxAFStateTransitionTypeStrings[static_cast<UINT8>(event)],
            CamxAFStateStrings[static_cast<UINT8>(newState)]);
    }
    else
    {
        return stateTransition;
    }

    if (AFState::Invalid != newState)
    {
        if ((newState != m_AFState) &&
            ((AFState::ActiveScan == newState) || (AFState::PassiveScan == newState)))
        {
            CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupAF, 0, "AF: Convergence");
        }
        else if (AFStateTransitionType::EventModeChange == event)
        {
            CAMX_TRACE_ASYNC_BEGIN_F(CamxLogGroupAF, 0, "AF: Convergence");
        }

        m_AFState       = newState;
        stateTransition = TRUE;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupAF, "Cause: %s not handled in state %s",
            CamxAFStateTransitionTypeStrings[static_cast<UINT8>(event)], CamxAFStateStrings[static_cast<UINT8>(m_AFState)]);
    }

    return stateTransition;
}

CAMX_NAMESPACE_END