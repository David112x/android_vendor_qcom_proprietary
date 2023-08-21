////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxdefs.h
/// @brief CHX common definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXDEFS_H
#define CHXDEFS_H

#include <hardware/camera3.h>
#include "camxcdktypes.h"
#include "chivendortag.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

#define CHX_STATIC_ASSERT(condition)   static_assert(condition, #condition)
#define CHX_ARRAY_SIZE(array) (sizeof((array)) / sizeof((array)[0]))

/// Constants
static const UINT   MaxChiStreams                  = 16;                 ///< Maximum number of CHI streams
static const UINT32 MaxOutstandingRequests         = 128;                ///< Number is bigger to prevent throttling of the
                                                                         ///  preview pipeline
static const UINT32 MaxPendingFrameNumber          = 200;                ///< Max valid number mapping between Framework and CHI
                                                                         ///  about 6s in 30fps

static const INT64  InvalidFrameNumber             = 0x7FFFFFFFFFFFFFFF; ///< Invalid frame number

static const UINT32 InvalidIndex                   = 0xFFFFFFFF;         ///< Invalid index
static const UINT32 INVALIDSEQUENCEID              = 0xFFFFFFFF;         ///< Invalid sequence id
static const UINT32 INVALIDFRAMENUMBER             = 0xFFFFFFFF;         ///< Invalid frame number

static const UINT   tuningUsecasePreviewMask       = 1;                  ///< Mask to indicate to preview usecase.
static const UINT   tuningUsecaseVideoMask         = 2;                  ///< Mask to indicate to video usecase.
static const UINT   tuningUsecaseSnapshotMask      = 4;                  ///< Mask to indicate to snapshot usecase.
static const UINT   tuningUsecaseZSLMask           = 8;                  ///< Mask to indicate to ZSL usecase.

static const UINT   CHIImmediateBufferCountDefault = 2;                  ///< Number of buffers to allocate immediately when
                                                                         ///  chi buffer manager is being created.
static const UINT   CHIImmediateBufferCountZSL     = 3;                  ///< Number of buffers to allocate immediately when
                                                                         ///  chi buffer manager is being created for zsl usecase.

static const UINT32 UHDResolutionWidth             = 3840;               ///< UHD resolution width
static const UINT32 UHDResolutionHeight            = 2160;               ///< UHD resolution height

/// @brief Usecase identifying enums
enum class UsecaseId
{
    NoMatch             = 0,
    Default             = 1,
    Preview             = 2,
    PreviewZSL          = 3,
    MFNR                = 4,
    MFSR                = 5,
    MultiCamera         = 6,
    QuadCFA             = 7,
    RawJPEG             = 8,
    MultiCameraVR       = 9,
    Torch               = 10,
    YUVInBlobOut        = 11,
    VideoLiveShot       = 12,
    SuperSlowMotionFRC  = 13,
    MaxUsecases         = 14,
};

// List of Vendor Tags
enum class VendorTag
{
    SensorBpsModeIndex  = 0,        ///< SensorBpsModeIndex
    SensorBpsGain,                  ///< SensorBpsGain
    DebugDataTag,                   ///< DebugData
    SensorModeIndex,                ///< SensorModeIndex
    CropRegions,                    ///< Crop Regions
    TuningMode,                     ///< TagTuningMode
    RefCropSize,                    ///< Ref crop size
    MultiCamera,                    ///< multicamera
    IsFlashRequiredTag,             ///< Is Flash Required for Snapshot
    Feature1Mode,                   ///< Feature1Mode
    Feature2Mode,                   ///< Feature1Mode
    VideoHDR10Mode,                 ///< Video HDR10 Mode
    StatsSkipMode,                  ///< Force skip stats processing
    BurstFps,                       ///< Burst Fps
    CustomNoiseReduction,           ///< MFNR  Hint
    FastShutterMode,                ///< FS mode
    IHDRControl,                    ///< IHDR Control
    HistNodeLTCRatioIndex,          ///< LTC ratio from histogram node
    IFEMaxWidth,                    ///< Max single IFE supported
    ZSLTimestampRange,              ///< Range of time within which to pull ZSL Buffers
    IPEOverrideScale,               ///< IPE Override Scale Flag
    LivePreview,                    ///< Live Preview Enablement flag
    DebugDumpConfig,                ///< Record timestamp and app frame number
    GPUOverrideRotation,            ///< GPU Node Rotation
    T2TConfigRegROI,                ///< Tracker registration ROI from application
    T2TResultROI,                   ///< Tracker result ROI from driver
    NumVendorTags                   ///< Max Tags
};

// Enum Class for Partial Result Count
enum class PartialResultCount
{
    NoPartialResult     = 0,
    FirstPartialResult  = 1,
    SecondPartialResult = 2,
    ThirdPartialResult  = 3,
};

// Enum Class for Total Partial Metadata that will be sent
enum class MetaDataResultCount
{
    NoMetaDataCount     = 0,
    OneMetaDataCount    = 1,
    TwoMetaDataCount    = 2,
    ThreeMetaDataCount  = 3,
};

// Enum Class for Partial Metadata sender
enum class PartialResultSender
{
    DriverPartialData = 0,
    CHIPartialData    = 1,
    DriverMetaData    = 2,
};

// Enum Class for Partial Metadata support from CHI
enum class PartialMetaSupport
{
    NoSupport           = 0,
    SeperatePartialMeta = 1,
    CombinedPartialMeta = 2,
};

#endif // CHXDEFS_H
