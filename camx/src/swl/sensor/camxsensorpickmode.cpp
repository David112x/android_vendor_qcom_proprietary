////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxsensorpickmode.cpp
/// @brief SensorPickMode class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxincs.h"
#include "camxsensorpickmode.h"

CAMX_NAMESPACE_BEGIN

#define MIN_HFR_FPS 90.0

/// @todo (CAMX-1773) Better way to do pick table
static const BOOL ModePickTable[MaxSensorPickModeCondition][MaxSensorPickModeUseCase] =
{
    // FAST AEC  QUAD  HFR   IHDR  RHDR VHDR Snapshot Video/P
    {    0,      0,    1,    0,    0,   0,   0,       0    },    // FPS
    {    0,      0,    0,    0,    0,   0,   0,       0    },    // Bounded FPS
    {    0,      0,    0,    0,    0,   0,   0,       0    },    // Aspect ratio
    {    0,      0,    0,    0,    0,   0,   0,       1    },    // Resolution w
    {    0,      0,    0,    0,    0,   0,   0,       1    },    // Resolution h
    {    0,      0,    0,    0,    0,   0,   0,       0    },    // Pix Clk
    {    0,      0,    0,    0,    0,   0,   0,       0    },    // QUADRA
    {    0,      0,    1,    0,    0,   0,   0,       0    },    // HFR
    {    0,      0,    0,    0,    0,   0,   0,       1    },    // NORMAL
    {    0,      0,    0,    0,    0,   0,   0,       0    },    // IHDR
    {    0,      0,    0,    0,    0,   0,   0,       0    },    // ZZHDR
    {    0,      0,    1,    0,    0,   0,   0,       0    },    // Max Res
    {    0,      0,    0,    0,    0,   0,   0,       1    },    // Best Res
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::ValidateWidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorPickMode::ValidateWidth(
    UINT32                           modeIndex,
    UINT32                           streamIndex,
    const  ResolutionInformation*    pModeInfo,
    const  SensorPickModeProperties* pSensorPickMode)
{
    CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                     "frameDimension.width %d  optimal width %d maxWidth %d minWidth %d",
                     pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.width,
                     pSensorPickMode->width,
                     pSensorPickMode->maxWidth,
                     pSensorPickMode->minWidth);

    // Width picked should be greater than or equal to the optimal requested width
    // but it shouldn't exceed the maximum width that the requesting port can handle.
    return ((pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.width >=
            pSensorPickMode->width) &&
            (pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.width <=
            pSensorPickMode->maxWidth));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::ValidateHeight
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorPickMode::ValidateHeight(
    UINT32                           modeIndex,
    UINT32                           streamIndex,
    const  ResolutionInformation*    pModeInfo,
    const  SensorPickModeProperties* pSensorPickMode)
{
    CAMX_LOG_VERBOSE(CamxLogGroupSensor,
                     "frameDimension.height %d optimal height %d maxHeight %d minHeight %d",
                     pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.height,
                     pSensorPickMode->height,
                     pSensorPickMode->maxHeight,
                     pSensorPickMode->minHeight);

    /// Height picked should be greater than or equal to the optimal requested height
    /// but it shouldn't exceed the maximum height that the requesting port can handle.
    return ((pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.height >=
            pSensorPickMode->height) &&
            (pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.height <=
            pSensorPickMode->maxHeight));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::ValidateAspectRatio
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorPickMode::ValidateAspectRatio(
    UINT32                           modeIndex,
    UINT32                           streamIndex,
    const  ResolutionInformation*    pModeInfo,
    const  SensorPickModeProperties* pSensorPickMode)
{
    FLOAT modeAspectRatio      = 0;
    FLOAT requestedAspectRatio = 0;

    modeAspectRatio = static_cast<FLOAT>(
        pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.width) /
        static_cast<FLOAT>(
        pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.height);

    requestedAspectRatio = static_cast<FLOAT>(pSensorPickMode->width) / static_cast<FLOAT>(pSensorPickMode->height);

    return Utils::AbsoluteFLOAT(modeAspectRatio - requestedAspectRatio) < 0.01;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::ValidateFrameRate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorPickMode::ValidateFrameRate(
    UINT32                           modeIndex,
    const  ResolutionInformation*    pModeInfo,
    const  SensorPickModeProperties* pSensorPickMode)
{
    FLOAT modeFrameRate      = 0;
    FLOAT requestedFrameRate = 0;

    modeFrameRate      = static_cast<FLOAT>(pModeInfo->resolutionData[modeIndex].frameRate);
    requestedFrameRate = static_cast<FLOAT>(pSensorPickMode->frameRate);

    return Utils::AbsoluteFLOAT(modeFrameRate - requestedFrameRate) < 0.5;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::ValidateCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorPickMode::ValidateCapability(
    UINT32                           modeIndex,
    const  ResolutionInformation*    pModeInfo,
    const  SensorPickModeProperties* pSensorPickMode)
{
    UINT32 j     = 0;
    BOOL   found = FALSE;

    for (UINT32 i = 0; i < pSensorPickMode->capabilityCount; i++)
    {
        for (j = 0; j < pModeInfo->resolutionData[modeIndex].capabilityCount; j++)
        {
            if (pSensorPickMode->capability[i] == pModeInfo->resolutionData[modeIndex].capability[j])
            {
                found = TRUE;
                break;
            }
        }
    }

    return found;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::ValidateBestResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorPickMode::ValidateBestResolution(
    UINT32                           modeIndex,
    UINT32                           streamIndex,
    const  ResolutionInformation*    pModeInfo,
    const  SensorPickModeProperties* pSensorPickMode,
    SensorPickResolution*            pPickedResolution)
{

    UINT32 modeResolution =
        pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.width *
        pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.height;

    UINT32 expectedRes = pSensorPickMode->width * pSensorPickMode->height;

    pPickedResolution->temporaryResolution = modeResolution;

    if (!pPickedResolution->lastResolution)
    {
        pPickedResolution->lastResolution = modeResolution;
    }

    BOOL retVal = (modeResolution <= pPickedResolution->lastResolution) && (modeResolution >= expectedRes);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, " modeResolution %d expectedRes %d retVal %d",
                     modeResolution, expectedRes, retVal);

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::ValidateMaxResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorPickMode::ValidateMaxResolution(
    UINT32                           modeIndex,
    UINT32                           streamIndex,
    const  ResolutionInformation*    pModeInfo,
    SensorPickResolution*            pPickedResolution)
{

    UINT32 modeResolution =
        pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.width *
        pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].frameDimension.height;

    pPickedResolution->temporaryResolution = modeResolution;

    BOOL retVal = (modeResolution >= pPickedResolution->lastResolution);

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "modeResolution %d retVal %d", modeResolution, retVal);

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::ValidateHFR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorPickMode::ValidateHFR(
    UINT32                           modeIndex,
    const  ResolutionInformation*    pModeInfo,
    const  SensorPickModeProperties* pSensorPickMode)
{
    FLOAT modeFrameRate      = 0;
    FLOAT requestedFrameRate = 0;

    modeFrameRate      = static_cast<FLOAT>(pModeInfo->resolutionData[modeIndex].frameRate);
    requestedFrameRate = static_cast<FLOAT>(pSensorPickMode->frameRate);

    BOOL retVal = (modeFrameRate >= MIN_HFR_FPS) && (requestedFrameRate >= MIN_HFR_FPS);

    return retVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::ValidateDefaultMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL SensorPickMode::ValidateDefaultMode(
    UINT32                           modeIndex,
    const  ResolutionInformation*    pModeInfo)
{
    UINT32 capabilityCount = 0;
    BOOL   found         = FALSE;

    for (capabilityCount = 0; capabilityCount < pModeInfo->resolutionData[modeIndex].capabilityCount; capabilityCount++)
    {
        if (SensorCapability::NORMAL == pModeInfo->resolutionData[modeIndex].capability[capabilityCount])
        {
            found = TRUE;
            break;
        }
    }

    return found;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::GetImageStreamIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorPickMode::GetImageStreamIndex(
    UINT32                           modeIndex,
    UINT32*                          pStreamIndex,
    const    ResolutionInformation*  pModeInfo)
{
    CamxResult result      = CamxResultEFailed;
    UINT32     streamIndex = 0;
    UINT32     streamCount = 0;

    if (NULL != pModeInfo)
    {
        streamCount = pModeInfo->resolutionData[modeIndex].streamInfo.streamConfigurationCount;

        for (streamIndex = 0; streamIndex < streamCount; streamIndex++)
        {
            if (StreamType::IMAGE == pModeInfo->resolutionData[modeIndex].streamInfo.streamConfiguration[streamIndex].type)
            {
                *pStreamIndex = streamIndex;
                result        = CamxResultSuccess;
                break;
            }
        }
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::GetUseCase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorPickModeUseCase SensorPickMode::GetUseCase(
    const SensorPickModeProperties* pSensorPickMode)
{
    SensorPickModeUseCase useCase;

    if (pSensorPickMode->frameRate >= MIN_HFR_FPS)
    {
        useCase = SensorPickModeUseCase::HFR;
    }
    else
    {
        /// @todo (CAMX-1770) complete sensor pick mode for all feature
        useCase = SensorPickModeUseCase::VideoPreview;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "useCases = %d", useCase);
    return useCase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// SensorPickMode::GetModeIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SensorPickMode::GetModeIndex(
    const   ResolutionInformation*    pModeInfo,
    const   SensorPickModeProperties* pSensorPickMode,
    UINT32*                           pSelectedMode)
{
    BOOL                 found             = FALSE;
    UINT32               selectedMode      = 0;
    CamxResult           result            = CamxResultSuccess;
    SensorPickResolution pickedResolution  = {0};
    UINT32               streamIndex       = 0;

    if (NULL != pSensorPickMode)
    {
        SensorPickModeUseCase useCase = GetUseCase(pSensorPickMode);

        for (UINT32 i = pModeInfo->resolutionDataCount; i > 0; --i)
        {
            UINT32 modeIndex = i-1;

            // Get IMAGE stream type for this mode
            result = GetImageStreamIndex(modeIndex, &streamIndex, pModeInfo);
            if (CamxResultEFailed == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "IMAGE stream type not configured, using default");
                streamIndex = 0;
            }

            /// Go through all the usecases in pick table, if any of the pick usecase failed return and use default resolution
            for (UINT8 conditionIndex = 0; conditionIndex < MaxSensorPickModeCondition; conditionIndex++)
            {
                if (TRUE == ModePickTable[conditionIndex][static_cast<UINT8>(useCase)])
                {
                    SensorPickModeCondition condition = static_cast<SensorPickModeCondition>(conditionIndex);
                    CAMX_LOG_VERBOSE(CamxLogGroupSensor, "condition = %d", condition);
                    switch (condition)
                    {
                        case SensorPickModeCondition::FrameRate:
                            found = SensorPickMode::ValidateFrameRate(modeIndex, pModeInfo, pSensorPickMode);
                            break;
                        case SensorPickModeCondition::AspectRatio:
                            found = SensorPickMode::ValidateAspectRatio(modeIndex, streamIndex, pModeInfo, pSensorPickMode);
                            break;
                        case SensorPickModeCondition::Width:
                            found = SensorPickMode::ValidateWidth(modeIndex, streamIndex, pModeInfo, pSensorPickMode);
                            break;
                        case SensorPickModeCondition::Height:
                            found = SensorPickMode::ValidateHeight(modeIndex, streamIndex, pModeInfo, pSensorPickMode);
                            break;
                        case SensorPickModeCondition::MPIX:
                            found = SensorPickMode::ValidateMaxResolution(modeIndex, streamIndex, pModeInfo, &pickedResolution);
                            break;
                        case SensorPickModeCondition::HFR:
                            found = SensorPickMode::ValidateHFR(modeIndex, pModeInfo, pSensorPickMode);
                            break;
                        case SensorPickModeCondition::DEF:
                            found = SensorPickMode::ValidateDefaultMode(modeIndex, pModeInfo);
                            break;
                        case SensorPickModeCondition::BestResolution:
                            if (FALSE ==
                                ModePickTable[static_cast<UINT8>(SensorPickModeCondition::MPIX)][static_cast<UINT8>(useCase)])
                            {
                                found = SensorPickMode::ValidateBestResolution(
                                                                               modeIndex,
                                                                               streamIndex,
                                                                               pModeInfo,
                                                                               pSensorPickMode,
                                                                               &pickedResolution);
                            }
                            break;
                        default:
                            CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Condition %d Not implemented", condition);
                            break;
                    }
                    if (FALSE == found)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Condition %d doesn't match", condition);
                        break;
                    }
                }
            }
            /// If found is still TRUE at the end , then we pick this mode
            if (TRUE == found)
            {
                selectedMode                    = modeIndex;
                pickedResolution.lastResolution = pickedResolution.temporaryResolution;
                CAMX_LOG_VERBOSE(CamxLogGroupSensor, "Selecting mode %d", modeIndex);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupSensor, "NULL pointer");
        result = CamxResultEInvalidPointer;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupSensor, " selectedMode = %d", selectedMode);

    *pSelectedMode = selectedMode;

    return result;
}

CAMX_NAMESPACE_END
