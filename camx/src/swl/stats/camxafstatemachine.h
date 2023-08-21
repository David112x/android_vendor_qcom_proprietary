////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxafstatemachine.h
/// @brief The class declaration for Android's State Machine for AF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXAFSTATEMACHINE_H
#define CAMXAFSTATEMACHINE_H

#include "chiafcommon.h"

#include "camxincs.h"
#include "camxhal3metadatatags.h"

CAMX_NAMESPACE_BEGIN

/// @brief The enum which defines AF State Command
enum class AFStateCommand
{
    StartFocus,     ///< Command to Start Focus
    CancelFocus,    ///< Command to Cancel Focus
    UnlockLens,     ///< Command to Lock Lens
    LockLens,       ///< Command to Unlock Lens
    Max,            ///< Max Command
};

/// @brief The string name of the type of the AFStateCommand. Must be in order of AFStateCommand.
#if __GNUC__
static const CHAR* CamxAFStateCommandStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CamxAFStateCommandStrings[] =
#endif // _GNUC__
{
    "StartFocus",     ///< Command to Start Focus
    "CancelFocus",    ///< Command to Cancel Focus
    "UnlockLens",     ///< Command to Lock Lens
    "LockLens",       ///< Command to Unlock Lens
    "InvalidMax"      ///< Max Command
};

/// @brief The enum which defines AF State machine.
///        corresponding to metadata tag ANDROID_CONTROL_AF_STATE
enum class AFState
{
    Inactive,               ///< AF is not active
    PassiveScan,            ///< Algorithm is in CAF mode and is focusing
    PassiveFocused,         ///< Algorithm is in CAF mode and is focused
    ActiveScan,             ///< Algorithm is doing a scan initiated by App request
    FocusedLocked,          ///< Lens is locked and image is in Focus
    NotFocusedLocked,       ///< Lens is locked and image is in not in focus
    PassiveUnfocused,       ///< Algorithm is in CAF mode but not in focus
    PassiveScanWithTrigger, ///< Algorithm is in CAF mode and is focusing and App has
                            ///  initiated a trigger
    Invalid,                ///< invalid AF state
    Max = Invalid           ///< max AF state
};

/// @brief The string name of the type of the AFState. Must be in order of AFState.
#if __GNUC__
static const CHAR* CamxAFStateStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CamxAFStateStrings[] =
#endif // _GNUC__
{
    "Inactive",               ///< AF is not active
    "PassiveScan",            ///< Algorithm is in CAF mode and is focusing
    "PassiveFocused",         ///< Algorithm is in CAF mode and is focused
    "ActiveScan",             ///< Algorithm is doing a scan initiated by App request
    "FocusedLocked",          ///< Lens is locked and image is in Focus
    "NotFocusedLocked",       ///< Lens is locked and image is in not in focus
    "PassiveUnfocused",       ///< Algorithm is in CAF mode but not in focus
    "PassiveScanWithTrigger", ///< Algorithm is in CAF mode and is focusing and App has
                              ///  initiated a trigger
    "Invalid",                ///< invalid AF state
};

/// @brief The enum which defines AF State transition Causes
enum class AFStateTransitionType
{
    EventTrigger,           ///< Trigger event initiated by App
    EventCancel,            ///< Cancel event initiated by App
    EventModeChange,        ///< Mode change initiated by App
    EventCAFModeChange,     ///< Mode change within continuous Auto Focus
    EventFocusDoneSuccess,  ///< Algorithm is done with Focus
    EventFocusDoneFailure,  ///< Algorithm failed to Focus
    EventStartScan,         ///< Algorithm is focusing
    EventIdle,              ///< Algorithm in the idle
    Max                     ///< Max AFTransition
};

/// @brief The string name of the type of the AFStateTransitionType. Must be in order of AFStateTransitionType.
#if __GNUC__
static const CHAR* CamxAFStateTransitionTypeStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CamxAFStateTransitionTypeStrings[] =
#endif // _GNUC__
{
    "EventTrigger",           ///< Trigger event initiated by App
    "EventCancel",            ///< Cancel event initiated by App
    "EventModeChange",        ///< Mode change initiated by App
    "EventCAFModeChange",     ///< Mode change within continuous Auto Focus
    "EventFocusDoneSuccess",  ///< Algorithm is done with Focus
    "EventFocusDoneFailure",  ///< Algorithm failed to Focus
    "EventStartScan",         ///< Algorithm is focusing
    "EventIdle",              ///< Algorithm in the idle
    "InvalidMax"              ///< Max AFTransition
};


/// @brief Internal Mapping enumerator to map android focus modes
enum class AFInternalFocusMode
{
    NonContinuous,         ///< Internal Enumerator for Mapping Auto/Macro modes
    ContinuousPicture,     ///< Internal Enumerator for Mapping Continuous  Preview AF Mode
    ContinuousVideo,       ///< Internal Enumerator for Mapping Continuous  Video AF Mode
    ManualORInfinity,      ///< Internal Enumerator for Mapping Manual/Infinity mode
    Max                    ///< Max Internal modes
};

/// @brief The string name of the type of the AFInternalFocusMode. Must be in order of AFInternalFocusMode.
#if __GNUC__
static const CHAR* CamxAFInternalFocusModeStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* CamxAFInternalFocusModeStrings[] =
#endif // _GNUC__
{
    "NonContinuous",         ///< Internal Enumerator for Mapping Auto/Macro modes
    "ContinuousPicture",     ///< Internal Enumerator for Mapping Continuous  Preview AF Mode
    "ContinuousVideo",       ///< Internal Enumerator for Mapping Continuous  Video AF Mode
    "ManualORInfinity",      ///< Internal Enumerator for Mapping Manual/Infinity mode
    "InvalidMax"             ///< Max Internal modes
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// AFStateTable
///
/// @brief Describes the State Transition table for AutoFocus State as described in
///        https://developer.android.com/reference/android/hardware/camera2/CaptureResult.html#CONTROL_AF_STATE
///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const UINT32 MaxModes        = static_cast<UINT32>(AFInternalFocusMode::Max);
static const UINT32 MaxStates       = static_cast<UINT32>(AFState::Max);
static const UINT32 MaxTransitions  = static_cast<UINT32>(AFStateTransitionType::Max);

static constexpr AFState AFStateTable[MaxModes][MaxStates][MaxTransitions] =
{
    // Mode: Auto
    {
        // Current State: AFStateInactive
        {
            AFState::ActiveScan,                // Cause: AFEventTrigger
            AFState::Inactive,                  // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::Invalid,                   // Cause: AFEventCAFModeChange
            AFState::Inactive,                  // Cause: AFEventFocusDoneSuccess
            AFState::Inactive,                  // Cause: AFEventFocusDoneFailure
            AFState::Inactive,                  // Cause: AFEventStartScan
            AFState::Inactive                   // Cause: AFEventIdle
        },

        // Current State: AFStatePassiveScan
        {
            AFState::Invalid,                   // Cause: AFEventTrigger
            AFState::Invalid,                   // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::Invalid,                   // Cause: AFEventCAFModeChange
            AFState::Invalid,                   // Cause: AFEventFocusDoneSuccess
            AFState::Invalid,                   // Cause: AFEventFocusDoneFailure
            AFState::Invalid,                   // Cause: AFEventStartScan
            AFState::PassiveScan                // Cause: AFEventIdle
        },

        // Current State: AFStatePassiveFocused
        {
            AFState::ActiveScan,                // Cause: AFEventTrigger
            AFState::Invalid,                   // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::Invalid,                   // Cause: AFEventCAFModeChange
            AFState::Invalid,                   // Cause: AFEventFocusDoneSuccess
            AFState::Invalid,                   // Cause: AFEventFocusDoneFailure
            AFState::Invalid,                   // Cause: AFEventStartScan
            AFState::PassiveFocused             // Cause: AFEventIdle
        },

        // Current State: AFStateActiveScan
        {
            AFState::ActiveScan,                // Cause: AFEventTrigger
            AFState::Inactive,                  // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::Invalid,                   // Cause: AFEventCAFModeChange
            AFState::FocusedLocked,             // Cause: AFEventFocusDoneSuccess
            AFState::NotFocusedLocked,          // Cause: AFEventFocusDoneFailure
            AFState::ActiveScan,                // Cause: AFEventStartScan
            AFState::ActiveScan                 // Cause: AFEventIdle
        },

        // Current State: AFStateFocusedLocked
        {
            AFState::ActiveScan,                // Cause: AFEventTrigger
            AFState::Inactive,                  // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::Invalid,                   // Cause: AFEventCAFModeChange
            AFState::FocusedLocked,             // Cause: AFEventFocusDoneSuccess
            AFState::FocusedLocked,             // Cause: AFEventFocusDoneFailure
            AFState::FocusedLocked,             // Cause: AFEventStartScan
            AFState::FocusedLocked              // Cause: AFEventIdle
        },

        // Current State: AFStateNotFocusedLocked
        {
            AFState::ActiveScan,                // Cause: AFEventTrigger
            AFState::Inactive,                  // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::Invalid,                   // Cause: AFEventCAFModeChange
            AFState::NotFocusedLocked,          // Cause: AFEventFocusDoneSuccess
            AFState::NotFocusedLocked,          // Cause: AFEventFocusDoneFailure
            AFState::NotFocusedLocked,          // Cause: AFEventStartScan
            AFState::NotFocusedLocked           // Cause: AFEventIdle
        },

        // Current State: AFStatePassiveUnfocused
        {
            AFState::Invalid,                   // Cause: AFEventTrigger
            AFState::Invalid,                   // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::Invalid,                   // Cause: AFEventCAFModeChange
            AFState::Invalid,                   // Cause: AFEventFocusDoneSuccess
            AFState::Invalid,                   // Cause: AFEventFocusDoneFailure
            AFState::Invalid,                   // Cause: AFEventStartScan
            AFState::PassiveUnfocused           // Cause: AFEventIdle
        },

        // Current State: AFStatePassiveScanWithTrigger
        {
            AFState::Invalid,                   // Cause: AFEventTrigger
            AFState::Invalid,                   // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::Invalid,                   // Cause: AFEventCAFModeChange
            AFState::Invalid,                   // Cause: AFEventFocusDoneSuccess
            AFState::Invalid,                   // Cause: AFEventFocusDoneFailure
            AFState::Invalid,                   // Cause: AFEventStartScan
            AFState::PassiveScanWithTrigger     // Cause: AFEventIdle
        }
    },

    // MODE: Continuous Picture
    {
        /// State: AFStateInactive
        {
            AFState::NotFocusedLocked,          // Cause: AFEventTrigger
            AFState::Inactive,                  // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::Inactive,                  // Cause: AFEventCAFModeChange
            AFState::PassiveFocused,            // Cause: AFEventFocusDoneSuccess
            AFState::PassiveUnfocused,          // Cause: AFEventFocusDoneFailure
            AFState::PassiveScan,               // Cause: AFEventStartScan
            AFState::Inactive                   // Cause: AFEventIdle
        },

        // State: AFStatePassiveScan
        {
            AFState::PassiveScanWithTrigger,    // Cause: AFEventTrigger
            AFState::Inactive,                  // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveScan,               // Cause: AFEventCAFModeChange
            AFState::PassiveFocused,            // Cause: AFEventFocusDoneSuccess
            AFState::PassiveUnfocused,          // Cause: AFEventFocusDoneFailure
            AFState::PassiveScan,               // Cause: AFEventStartScan
            AFState::PassiveScan                // Cause: AFEventIdle
        },

        // State: AFStatePassiveFocused
        {
            AFState::FocusedLocked,             // Cause: AFEventTrigger
            AFState::PassiveFocused,            // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveFocused,            // Cause: AFEventCAFModeChange
            AFState::PassiveFocused,            // Cause: AFEventFocusDoneSuccess
            AFState::PassiveFocused,            // Cause: AFEventFocusDoneFailure
            AFState::PassiveScan,               // Cause: AFEventStartScan
            AFState::PassiveFocused             // Cause: AFEventIdle
        },

        // State: AFStateActiveScan
        {
            AFState::ActiveScan,                // Cause: AFEventTrigger
            AFState::ActiveScan,                // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::ActiveScan,                // Cause: AFEventCAFModeChange
            AFState::ActiveScan,                // Cause: AFEventFocusDoneSuccess
            AFState::ActiveScan,                // Cause: AFEventFocusDoneFailure
            AFState::ActiveScan,                // Cause: AFEventStartScan
            AFState::ActiveScan                 // Cause: AFEventIdle
        },

        // State: AFStateFocusedLocked
        {
            AFState::FocusedLocked,             // Cause: AFEventTrigger
            AFState::PassiveFocused,            // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveFocused,            // Cause: AFEventCAFModeChange
            AFState::FocusedLocked,             // Cause: AFEventFocusDoneSuccess
            AFState::FocusedLocked,             // Cause: AFEventFocusDoneFailure
            AFState::FocusedLocked,             // Cause: AFEventStartScan
            AFState::FocusedLocked              // Cause: AFEventIdle
        },

        // State: AFStateNotFocusedLocked
        {
            AFState::NotFocusedLocked,          // Cause: AFEventTrigger
            AFState::PassiveUnfocused,          // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveUnfocused,          // Cause: AFEventCAFModeChange
            AFState::NotFocusedLocked,          // Cause: AFEventFocusDoneSuccess
            AFState::NotFocusedLocked,          // Cause: AFEventFocusDoneFailure
            AFState::NotFocusedLocked,          // Cause: AFEventStartScan
            AFState::NotFocusedLocked           // Cause: AFEventIdle
        },

        // State: AFStatePassiveUnfocused
        {
            AFState::NotFocusedLocked,          // Cause: AFEventTrigger
            AFState::PassiveUnfocused,          // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveUnfocused,          // Cause: AFEventCAFModeChange
            AFState::PassiveUnfocused,          // Cause: AFEventFocusDoneSuccess
            AFState::PassiveUnfocused,          // Cause: AFEventFocusDoneFailure
            AFState::PassiveScan,               // Cause: AFEventStartScan
            AFState::PassiveUnfocused           // Cause: AFEventIdle
        },

        // State: AFStatePassiveScanWithTrigger
        {
            AFState::PassiveScanWithTrigger,    // Cause: AFEventTrigger
            AFState::Inactive,                  // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveScanWithTrigger,    // Cause: AFEventCAFModeChange
            AFState::FocusedLocked,             // Cause: AFEventFocusDoneSuccess
            AFState::NotFocusedLocked,          // Cause: AFEventFocusDoneFailure
            AFState::PassiveScanWithTrigger,    // Cause: AFEventStartScan
            AFState::PassiveScanWithTrigger     // Cause: AFEventIdle
        },
    },

    // MODE: Continuous Video
    {
        /// State: AFStateInactive
        {
            AFState::NotFocusedLocked,          // Cause: AFEventTrigger
            AFState::Inactive,                  // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::Inactive,                  // Cause: AFEventCAFModeChange
            AFState::PassiveFocused,            // Cause: AFEventFocusDoneSuccess
            AFState::PassiveUnfocused,          // Cause: AFEventFocusDoneFailure
            AFState::PassiveScan,               // Cause: AFEventStartScan
            AFState::Inactive                   // Cause: AFEventIdle
        },

        // State: AFStatePassiveScan
        {
            AFState::NotFocusedLocked,          // Cause: AFEventTrigger
            AFState::Inactive,                  // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveScan,               // Cause: AFEventCAFModeChange
            AFState::PassiveFocused,            // Cause: AFEventFocusDoneSuccess
            AFState::PassiveUnfocused,          // Cause: AFEventFocusDoneFailure
            AFState::PassiveScan,               // Cause: AFEventStartScan
            AFState::PassiveScan                // Cause: AFEventIdle
        },

        // State: AFStatePassiveFocused
        {
            AFState::FocusedLocked,             // Cause: AFEventTrigger
            AFState::PassiveFocused,            // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveFocused,            // Cause: AFEventCAFModeChange
            AFState::PassiveFocused,            // Cause: AFEventFocusDoneSuccess
            AFState::PassiveFocused,            // Cause: AFEventFocusDoneFailure
            AFState::PassiveScan,               // Cause: AFEventStartScan
            AFState::PassiveFocused             // Cause: AFEventIdle
        },

        // State: AFStateActiveScan
        {
            AFState::ActiveScan,                // Cause: AFEventTrigger
            AFState::ActiveScan,                // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::ActiveScan,                // Cause: AFEventCAFModeChange
            AFState::ActiveScan,                // Cause: AFEventFocusDoneSuccess
            AFState::ActiveScan,                // Cause: AFEventFocusDoneFailure
            AFState::ActiveScan,                // Cause: AFEventStartScan
            AFState::ActiveScan                 // Cause: AFEventIdle
        },

        // State: AFStateFocusedLocked
        {
            AFState::FocusedLocked,             // Cause: AFEventTrigger
            AFState::PassiveFocused,            // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveFocused,            // Cause: AFEventCAFModeChange
            AFState::FocusedLocked,             // Cause: AFEventFocusDoneSuccess
            AFState::FocusedLocked,             // Cause: AFEventFocusDoneFailure
            AFState::FocusedLocked,             // Cause: AFEventStartScan
            AFState::FocusedLocked              // Cause: AFEventIdle
        },

        // State: AFStateNotFocusedLocked
        {
            AFState::NotFocusedLocked,          // Cause: AFEventTrigger
            AFState::PassiveUnfocused,          // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveUnfocused,          // Cause: AFEventCAFModeChange
            AFState::NotFocusedLocked,          // Cause: AFEventFocusDoneSuccess
            AFState::NotFocusedLocked,          // Cause: AFEventFocusDoneFailure
            AFState::NotFocusedLocked,          // Cause: AFEventStartScan
            AFState::NotFocusedLocked           // Cause: AFEventIdle
        },

        // State: AFStatePassiveUnfocused
        {
            AFState::NotFocusedLocked,          // Cause: AFEventTrigger
            AFState::PassiveUnfocused,          // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveUnfocused,          // Cause: AFEventCAFModeChange
            AFState::PassiveUnfocused,          // Cause: AFEventFocusDoneSuccess
            AFState::PassiveUnfocused,          // Cause: AFEventFocusDoneFailure
            AFState::PassiveScan,               // Cause: AFEventStartScan
            AFState::PassiveUnfocused           // Cause: AFEventIdle
        },

        // State: AFStatePassiveScanWithTrigger
        {
            AFState::PassiveScanWithTrigger,    // Cause: AFEventTrigger
            AFState::Inactive,                  // Cause: AFEventCancel
            AFState::Inactive,                  // Cause: AFEventModeChange
            AFState::PassiveScanWithTrigger,    // Cause: AFEventCAFModeChange
            AFState::FocusedLocked,             // Cause: AFEventFocusDoneSuccess
            AFState::NotFocusedLocked,          // Cause: AFEventFocusDoneFailure
            AFState::PassiveScan,               // Cause: AFEventStartScan
            AFState::PassiveScanWithTrigger     // Cause: AFEventIdle
        },
    },

    // MODE: ManualORInfinity
    {
         /// State: AFStateInactive
        {
            AFState::Inactive,          // Cause: AFEventTrigger
            AFState::Inactive,          // Cause: AFEventCancel
            AFState::Inactive,          // Cause: AFEventModeChange
            AFState::Inactive,          // Cause: AFEventCAFModeChange
            AFState::Inactive,          // Cause: AFEventFocusDoneSuccess
            AFState::Inactive,          // Cause: AFEventFocusDoneFailure
            AFState::Inactive,          // Cause: AFEventStartScan
            AFState::Inactive           // Cause: AFEventIdle
        },
    },
};

/// Enforce table state assumptions

// Non-Continuous Mode State Machine
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::Invalid ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::NonContinuous)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

// Continuous Picture Mode State Machine
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::PassiveScanWithTrigger ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::FocusedLocked)
                               ][static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);

CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::PassiveScanWithTrigger ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveScanWithTrigger ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::PassiveScanWithTrigger ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousPicture)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);


// Continuous Video Mode State Machine
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::Inactive)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveFocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::ActiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::ActiveScan)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveFocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::FocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::FocusedLocked)
                               ][static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::NotFocusedLocked)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::PassiveUnfocused ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveUnfocused)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

CAMX_STATIC_ASSERT(AFState::PassiveScanWithTrigger ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCancel)]);
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);
CAMX_STATIC_ASSERT(AFState::PassiveScanWithTrigger ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);
CAMX_STATIC_ASSERT(AFState::FocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);
CAMX_STATIC_ASSERT(AFState::NotFocusedLocked ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);
CAMX_STATIC_ASSERT(AFState::PassiveScan ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ContinuousVideo)]
                               [static_cast<UINT32>(AFState::PassiveScanWithTrigger)]
                               [static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

// ManualORInfinity Mode State Machine
CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ManualORInfinity)]
                   [static_cast<UINT32>(AFState::Inactive)]
[static_cast<UINT32>(AFStateTransitionType::EventTrigger)]);

CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ManualORInfinity)]
                   [static_cast<UINT32>(AFState::Inactive)]
[static_cast<UINT32>(AFStateTransitionType::EventCancel)]);

CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ManualORInfinity)]
                   [static_cast<UINT32>(AFState::Inactive)]
[static_cast<UINT32>(AFStateTransitionType::EventModeChange)]);

CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ManualORInfinity)]
                   [static_cast<UINT32>(AFState::Inactive)]
[static_cast<UINT32>(AFStateTransitionType::EventCAFModeChange)]);

CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ManualORInfinity)]
                   [static_cast<UINT32>(AFState::Inactive)]
[static_cast<UINT32>(AFStateTransitionType::EventFocusDoneSuccess)]);

CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ManualORInfinity)]
                   [static_cast<UINT32>(AFState::Inactive)]
[static_cast<UINT32>(AFStateTransitionType::EventFocusDoneFailure)]);

CAMX_STATIC_ASSERT(AFState::Inactive ==
                   AFStateTable[static_cast<UINT32>(AFInternalFocusMode::ManualORInfinity)]
                   [static_cast<UINT32>(AFState::Inactive)]
[static_cast<UINT32>(AFStateTransitionType::EventStartScan)]);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The class that implements Android's State Machine for AF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class AFStateMachine
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AFStateMachine
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AFStateMachine();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~AFStateMachine
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~AFStateMachine() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetControlCommand
    ///
    /// @brief  To get the control command to be executed by AF algorithm
    ///
    /// @return AFControlCommand
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AFControlCommand GetControlCommand() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetControlAFState
    ///
    /// @brief  Translates from AFStateValues to ControlAFStateValues
    ///
    /// @return ControlAFStateValues
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ControlAFStateValues GetControlAFState() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAFFocusAndRunMode
    ///
    /// @brief  Sets AF Focus Mode and the Run Mode which comes in from app
    ///
    /// @param  focusMode    New AF Focus Mode
    /// @param  runMode      New AF Run Mode
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAFFocusAndRunMode(
        AFFocusMode    focusMode,
        AFRunMode      runMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessAFStatusUpdate
    ///
    /// @brief  Handles the AF Status update from the algorithm
    ///
    /// @param  focusStatus            AF Algorithm Status
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessAFStatusUpdate(
        AFAlgoStatusType focusStatus);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleAFStateTransition
    ///
    /// @brief  Handle AF State transition due to transition causes
    ///
    /// @param  event    Transition cause due to app/algorithm changes
    ///
    /// @return BOOL If there is a valid state transition
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL HandleAFStateTransition(
        AFStateTransitionType event);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetModeIndex
    ///
    /// @brief  Get the index based on the current focus mode
    ///
    /// @return AFInternalFocusMode
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AFInternalFocusMode GetModeIndex() const;

private:
    AFFocusMode         m_focusMode;      ///< Current AF Focus Mode
    AFRunMode           m_runMode;        ///< Current AF Run Mode
    AFAlgoStatusType    m_focusStatus;    ///< Current Focus Status of Algorithm
    AFState             m_AFState;        ///< Current AF State

    AFStateMachine(const AFStateMachine&) = delete;               ///< Disallow the copy constructor.
    AFStateMachine& operator=(const AFStateMachine&) = delete;    ///< Disallow assignment operator.

};

CAMX_NAMESPACE_END
#endif // CAMXAFSTATEMACHINE_H
