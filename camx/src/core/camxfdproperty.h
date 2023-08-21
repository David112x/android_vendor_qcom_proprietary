////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxfdproperty.h
/// @brief Define Qualcomm Technologies, Inc. FD proprietary data for holding internal properties/events
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXFDPROPERTY_H
#define CAMXFDPROPERTY_H

#include "chifdproperty.h"
#include "camximagebuffer.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describes Common FD data structures
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const UINT32 FDFrameWidth    = 640; ///< FD Frame width
static const UINT32 FDFrameHeight   = 480; ///< FD Frame height

/// @brief Structure FD HW frame settings
struct FDFrameHwSettings
{
    BOOL skipProcess;  ///< Whether to skip processing this frame
};

/// @brief Structure FD SW frame settings
struct FDFrameSwSettings
{
    BOOL skipProcess;  ///< Whether to skip processing this frame
};

/// @brief Structure FD frame settings
struct FDPropertyFrameSettings
{
    BOOL                enableFD;   ///< Whether to enable FD
    FDFrameHwSettings   hwSettings; ///< Frame settings for HW detection
    FDFrameSwSettings   swSettings; ///< Frame settings for SW detection
};

/// @brief FDManagerSequence processing sequence
enum FDManagerSequence
{
    FDSWIntermittent = 0,        ///< Use SW Intermittent execution
    FDFPFiltering,               ///< Use FP filtering
};

/// @brief Structure FD frame settings
struct FDPerFrameSettings
{
    UINT64                          requestId;                     ///< Frame request ID
    BOOL                            skipProcess;                   ///< Whether to skip complete frame
    FDPropertyFrameSettings         frameSettings;                 ///< FD node frame settings
    StatisticsFaceDetectModeValues  fdModeReceived;                ///< FD mode received
    ControlModeValues               controlMode;                   ///< Control mode received
    ControlSceneModeValues          sceneMode;                     ///< Scene mode received
    BOOL                            ptEnable;                      ///< the received enable setting of facial parts detection
    BOOL                            smEnable;                      ///< the received enable setting of smile detection
    BOOL                            gzEnable;                      ///< the received enable setting of gaze detection
    BOOL                            bkEnable;                      ///< the received enable setting of blink detection
    BOOL                            ctEnable;                      ///< the received enable setting of facial contour detection
    CHIRectangle                    inputFrameMap;                 ///< Input frame mapping w.r.t CAMIF
    CHIRectangle                    finalFOVCrop;                  ///< Final FOV crop w.r.t FD input frame
    BOOL                            postResultsToMetaData;         ///< Flag to indicate if results need to be published to
                                                                   ///< metadata
    BOOL                            postResultsToProperty;         ///< Flag to indicate if results need to be published to
                                                                   ///< property
    ImageBuffer*                    pImageBuffer;                  ///< ImageBuffer handle for input frame buffer
    BYTE*                           pUVImageAddress;               ///< UV plane image address
    BYTE*                           pImageAddress;                 ///< Y plane image address
    FDConfig                        FDConfig;                      ///< Face detection tuning configuration
    BOOL                            FDConfigUpdated;               ///< Whether configuration changed compared to previous
                                                                   ///< request
    INT32                           deviceOrientation;             ///< Device orientation angle
    BOOL                            enablePreprocessing;           ///< Whether preprocessing is required for this frame
    BOOL                            inputBufferInvalidateRequired; ///< Whether input buffer need to be cache invalidated
    BOOL                            frameSettingsPublished;        ///< Whether frame settings is published to results pool
    FDManagerSequence               FDManagerExecutionSequence;    ///< Control the FD Manager executon sequence
};

CAMX_NAMESPACE_END

#endif // CAMXFDPROPERTY_H
