////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graphselectortable.h
/// @brief CHI feature graph selector table definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2GRAPHSELECTORTABLE_H
#define CHIFEATURE2GRAPHSELECTORTABLE_H

#include <set>

// NOWHINE ENTIRE FILE - Temporarily bypassing for this file, required for table

/// @brief set of Capture Intents
std::set<UINT> CaptureIntentAll =
{
    ControlCaptureIntentCustom,
    ControlCaptureIntentPreview,
    ControlCaptureIntentStillCapture,
    ControlCaptureIntentVideoRecord,
    ControlCaptureIntentVideoSnapshot,
    ControlCaptureIntentZeroShutterLag,
    ControlCaptureIntentManual,
};

/// @brief set of Scene modes
std::set<UINT> SceneModeAll =
{
    ControlSceneModeDisabled,
    ControlSceneModeFacePriority,
    ControlSceneModeAction,
    ControlSceneModePortrait,
    ControlSceneModeLandscape,
    ControlSceneModeNight,
    ControlSceneModeNightPortrait,
    ControlSceneModeTheatre,
    ControlSceneModeBeach,
    ControlSceneModeSnow,
    ControlSceneModeSunset,
    ControlSceneModeSteadyphoto,
    ControlSceneModeFireworks,
    ControlSceneModeSports,
    ControlSceneModeParty,
    ControlSceneModeCandlelight,
    ControlSceneModeBarcode,
    ControlSceneModeHighSpeedVideo,
    ControlSceneModeHDR,
    ControlSceneModeFacePriorityLowLight,
};

/// @brief set of Noise Reduction modes
std::set<UINT> noiseReductionmodeAll =
{
    NoiseReductionModeOff,
    NoiseReductionModeFast,
    NoiseReductionModeHighQuality,
    NoiseReductionModeMinimal,
    NoiseReductionModeZeroShutterLag,
};

#endif // CHIFEATURE2GRAPHSELECTORTABLE_H
