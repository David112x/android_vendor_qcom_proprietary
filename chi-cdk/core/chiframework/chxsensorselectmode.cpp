////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxsensorselectmode.cpp
/// @brief SensorSelectMode class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chxextensionmodule.h"
#include "chxsensorselectmode.h"
#include "chxutils.h"
#include <log/log.h>

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

static const UINT MinHFRFps = 90;

#undef  LOG_TAG
#define LOG_TAG "CHIUSECASE"

/// @todo This is a reference default implementation that can be replaced with customized selection
static const BOOL ModeSelectTable[MaxModeSelectCondition][MaxModeSelectUsecase] =
{
    // FAST AEC  QUAD  HFR   IHDR  RHDR VHDR Snapshot Video/P FS
    {    0,      0,    1,    1,    1,   0,   0,       1,      0  },    // Condition FPS
    {    0,      0,    0,    0,    0,   0,   0,       0,      0  },    // Condition Bounded FPS
    {    0,      1,    0,    1,    1,   0,   0,       1,      1  },    // Condition Aspect ratio
    {    0,      1,    0,    1,    1,   0,   0,       1,      1  },    // Condition Resolution w
    {    0,      1,    0,    1,    1,   0,   0,       1,      1  },    // Condition Resolution h
    {    0,      0,    0,    0,    0,   0,   0,       0,      0  },    // Condition Pix Clk
    {    0,      0,    1,    0,    0,   0,   0,       0,      0  },    // Condition HFR
    {    0,      0,    1,    0,    0,   0,   0,       0,      0  },    // Condition Max Res / MPIX
    {    0,      1,    0,    1,    1,   0,   0,       1,      0  },    // Condition BestRes
    {    0,      0,    0,    0,    0,   0,   0,       0,      1  },    // Condition Fast shutter
    {    0,      1,    1,    1,    1,   1,   0,       1,      0  },    // Condition Capability
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::IsMatchingWidth
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxSensorModeSelect::IsMatchingWidth(
    const ChiSensorModeInfo* pModeInfo,
    const DesiredSensorMode* pDesiredSensorMode)
{
    CHX_ASSERT(StreamImage == pModeInfo->streamtype);

    CHX_LOG("FrameDimension.width %d  optimal width %d maxWidth %d minWidth %d",
             pModeInfo->frameDimension.width, pDesiredSensorMode->optimalWidth,
             pDesiredSensorMode->maxWidth, pDesiredSensorMode->minWidth);

    // Width selected should be greater than or equal to the optimal requested width but it shouldn't exceed the maximum width
    // than the requesting port can handle
    if (pDesiredSensorMode->maxWidth == pDesiredSensorMode->optimalWidth)
    {
        return (pModeInfo->frameDimension.width >= pDesiredSensorMode->optimalWidth);

    }
    else
    {
        return ((pModeInfo->frameDimension.width >= pDesiredSensorMode->optimalWidth) &&
            (pModeInfo->frameDimension.width <= pDesiredSensorMode->maxWidth));

    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::IsMatchingHeight
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxSensorModeSelect::IsMatchingHeight(
    const ChiSensorModeInfo* pModeInfo,
    const DesiredSensorMode* pDesiredSensorMode)
{
    CHX_ASSERT(StreamImage == pModeInfo->streamtype);

    CHX_LOG("FrameDimension.height %d  optimal height %d maxHeight %d minHeight %d",
            pModeInfo->frameDimension.height, pDesiredSensorMode->optimalHeight,
            pDesiredSensorMode->maxHeight, pDesiredSensorMode->minHeight);


    /// Height selected should be greater than or equal to the optimal requested height
    /// but it shouldn't exceed the maximum height that the requesting port can handle.
    if (pDesiredSensorMode->maxHeight == pDesiredSensorMode->optimalHeight)
    {
        return (pModeInfo->frameDimension.height >= pDesiredSensorMode->optimalHeight);

    }
    else
    {
        return ((pModeInfo->frameDimension.height >= pDesiredSensorMode->optimalHeight) &&
            (pModeInfo->frameDimension.height <= pDesiredSensorMode->maxHeight));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::IsMatchingAspectRatio
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxSensorModeSelect::IsMatchingAspectRatio(
    const ChiSensorModeInfo*     pModeInfo,
    const DesiredSensorMode*     pDesiredSensorMode,
    SensorSelectBestAspectRatio* pSelectedAspectRatio)
{
    FLOAT modeAspectRatio      = 0;
    FLOAT requestedAspectRatio = 0;
    FLOAT aspectRatioDiff      = 0;

    CHX_ASSERT(StreamImage == pModeInfo->streamtype);

    modeAspectRatio = (static_cast<FLOAT>(pModeInfo->frameDimension.width) /
                       static_cast<FLOAT>(pModeInfo->frameDimension.height));

    requestedAspectRatio = (static_cast<FLOAT>(pDesiredSensorMode->optimalWidth) /
                            static_cast<FLOAT>(pDesiredSensorMode->optimalHeight));

    aspectRatioDiff = static_cast<FLOAT>(ChxUtils::AbsoluteFLOAT(modeAspectRatio - requestedAspectRatio));

    pSelectedAspectRatio->tempAspectRatioDiff = aspectRatioDiff;

    CHX_LOG(" aspectRatioDiff %f returnValue %d", aspectRatioDiff, (aspectRatioDiff <= pSelectedAspectRatio->lastAspectRatioDiff));

    return ( aspectRatioDiff <= pSelectedAspectRatio->lastAspectRatioDiff );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::IsMatchingFrameRate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxSensorModeSelect::IsMatchingFrameRate(
    const ChiSensorModeInfo* pModeInfo,
    const DesiredSensorMode* pDesiredSensorMode)
{
    FLOAT modeFrameRate      = 0;
    FLOAT requestedFrameRate = 0;

    modeFrameRate      = static_cast<FLOAT>(pModeInfo->frameRate);
    requestedFrameRate = static_cast<FLOAT>(pDesiredSensorMode->frameRate);

    return ChxUtils::AbsoluteFLOAT(modeFrameRate - requestedFrameRate) < 0.5;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::IsMatchingCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxSensorModeSelect::IsMatchingCapability(
    const ChiSensorModeInfo* pModeInfo,
    const DesiredSensorMode* pDesiredSensorMode)
{
    BOOL   isFound = FALSE;

    // Default select normal sensor modes if not configure mode caps
    if (0 == pDesiredSensorMode->sensorModeCaps.value)
    {
        isFound = (TRUE == pModeInfo->sensorModeCaps.u.Normal) ? TRUE : FALSE;
    } // elseif : sensorModeCaps can have multiple capabilities, so check whether all of the desired are matching or not
    else if ((pDesiredSensorMode->sensorModeCaps.value & pModeInfo->sensorModeCaps.value) ==
             pDesiredSensorMode->sensorModeCaps.value)
    {
        isFound = TRUE;
    }

    CHX_LOG("Desired sensor mode caps:%d, pModeInfo sensor mode caps:%d, isFound:%d",
            pDesiredSensorMode->sensorModeCaps.value,
            pModeInfo->sensorModeCaps.value,
            isFound);

    return isFound;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::IsMatchingBestResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxSensorModeSelect::IsMatchingBestResolution(
    const ChiSensorModeInfo*    pModeInfo,
    const DesiredSensorMode*   pDesiredSensorMode,
    SensorSelectBestResolution* pSelectedResolution)
{
    CHX_ASSERT(StreamImage == pModeInfo->streamtype);

    UINT32 modeResolution = pModeInfo->frameDimension.width * pModeInfo->frameDimension.height;

    UINT32 expectedResolution = pDesiredSensorMode->optimalWidth * pDesiredSensorMode->optimalHeight;

    pSelectedResolution->temporaryResolution = modeResolution;

    if (0 == pSelectedResolution->lastResolution)
    {
        pSelectedResolution->lastResolution = modeResolution;
    }

    BOOL returnValue = (modeResolution <= pSelectedResolution->lastResolution) && (modeResolution >= expectedResolution);

    CHX_LOG(" SelectResolution %d expectedRes %d returnValue %d", modeResolution, expectedResolution, returnValue);

    return returnValue;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::IsMatchingMaxResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxSensorModeSelect::IsMatchingMaxResolution(
    const ChiSensorModeInfo*    pModeInfo,
    SensorSelectBestResolution* pSelectedResolution)
{
    CHX_ASSERT(StreamImage == pModeInfo->streamtype);

    UINT32 selectResolution = pModeInfo->frameDimension.width * pModeInfo->frameDimension.height;

    pSelectedResolution->temporaryResolution = selectResolution;

    CHX_LOG(" selectResolution %d returnValue %d", selectResolution, (selectResolution >= pSelectedResolution->lastResolution));

    return ( selectResolution >= pSelectedResolution->lastResolution );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::IsMatchingHFR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxSensorModeSelect::IsMatchingHFR(
    const ChiSensorModeInfo*  pModeInfo,
    const DesiredSensorMode* pDesiredSensorMode)
{
    FLOAT modeFrameRate    = 0;
    FLOAT desiredFrameRate = 0;

    CHX_ASSERT(StreamImage == pModeInfo->streamtype);

    modeFrameRate    = static_cast<FLOAT>(pModeInfo->frameRate);
    desiredFrameRate = static_cast<FLOAT>(pDesiredSensorMode->frameRate);

    return ((modeFrameRate >= MinHFRFps) && (desiredFrameRate >= MinHFRFps) ? TRUE : FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::IsMatchingDefaultMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxSensorModeSelect::IsMatchingDefaultMode(
    const ChiSensorModeInfo* pModeInfo)
{
    BOOL   isFound = FALSE;

    CHX_ASSERT(StreamImage == pModeInfo->streamtype);

    if (1 == pModeInfo->sensorModeCaps.u.Normal)
    {
        isFound = TRUE;
    }

    CHX_LOG(" IsMatching Default %d", isFound);

    return isFound;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::FindClosestMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 ChxSensorModeSelect::FindClosestMode(
    const  ChiSensorModeInfo* pAllModes,
    UINT32                    modeCount,
    const  DesiredSensorMode* pDesiredSensorMode)
{
    // This is a reference default implementation that can be replaced with customized selection
    UINT32 closestModeIndex        = 0;
    UINT32 closestSecondIndex      = 0; // The next best mode
    // we find closest fps first, then find closest width and height
    UINT32 minFpsDelta             = static_cast<UINT32>(-1);
    UINT32 closestWidth            = static_cast<UINT32>(-1);
    UINT32 closestHeight           = static_cast<UINT32>(-1);
    UINT32 closestSecondDiff       = static_cast<UINT32>(-1);
    BOOL   found                   = FALSE;
    BOOL   secondfound             = FALSE;
    UINT32 i                       = 0;

    /// Sometimes, VT call may request 15fps mode, we need to find a closest fps resolution mode.
    /// 1. Get a closest fps delta
    for (i = 0; i < modeCount; i++)
    {
        if ((pAllModes[i].frameRate >= static_cast<UINT32>(pDesiredSensorMode->frameRate)) &&
            (minFpsDelta > (pAllModes[i].frameRate - static_cast<UINT32>(pDesiredSensorMode->frameRate))))
        {
            minFpsDelta      = pAllModes[i].frameRate - static_cast<UINT32>(pDesiredSensorMode->frameRate);
            closestModeIndex = i;
        }
    }

    // 2. Try to find a matching mode which can match width/height/capability/fps
    for (i = 0; i < modeCount; i++)
    {
        if (pAllModes[i].frameRate >= static_cast<UINT32>(pDesiredSensorMode->frameRate) &&
            ChxSensorModeSelect::IsMatchingCapability(&pAllModes[i], pDesiredSensorMode))
        {
            UINT32 currFpsDelta = pAllModes[i].frameRate - static_cast<UINT32>(pDesiredSensorMode->frameRate);

            if (ChxSensorModeSelect::IsMatchingWidth(&pAllModes[i], pDesiredSensorMode))
            {
                if (ChxSensorModeSelect::IsMatchingHeight(&pAllModes[i], pDesiredSensorMode))
                {
                    // If height and width match
                    /// In general, the sensor mode fps is 30.xx, 60.xx or 120.xx, the delta should be same if the fps in same level.
                    /// But, we allow a little threshold.
                    if ((currFpsDelta - minFpsDelta <= 1) &&
                        (closestWidth > pAllModes[i].frameDimension.width) &&
                        (closestHeight > pAllModes[i].frameDimension.height))
                    {
                        found            = TRUE;
                        closestModeIndex = i;
                        closestWidth     = pAllModes[i].frameDimension.width;
                        closestHeight    = pAllModes[i].frameDimension.height;
                    }
                }
                else if (FALSE == found)
                {
                    // Width matches height doesn't, cache the second best selection so far
                    UINT32 currHeightDiff = pDesiredSensorMode->optimalHeight - pAllModes[i].frameDimension.height;

                    if ((currFpsDelta - minFpsDelta <= 1) &&
                        (closestSecondDiff  > currHeightDiff))
                    {
                        secondfound        = TRUE;
                        closestSecondIndex = i;
                        closestSecondDiff  = currHeightDiff;
                    }
                }
            }
            else if (FALSE == found)
            {
                // Width didn't match, height could
                if (ChxSensorModeSelect::IsMatchingHeight(&pAllModes[i], pDesiredSensorMode))
                {
                    // Height matches width doesn't, cache the second best selection so far
                    UINT32 currWidthDiff = pDesiredSensorMode->optimalWidth - pAllModes[i].frameDimension.width;

                    if ((currFpsDelta - minFpsDelta <= 1) &&
                        (closestSecondDiff > currWidthDiff))
                    {
                        secondfound        = TRUE;
                        closestSecondIndex = i;
                        closestSecondDiff  = currWidthDiff;
                    }
                }
            }
        }
    }

    /// Didn't find suitable sensor mode at step 2, then we need to upscale a closest low size sensor mode
    /// 3. Then we need to upscale a sensor mode to support the desired mode, so we find a closest mode which
    /// has a closest width and height and match capability and fps.
    if (FALSE == (found || secondfound))
    {
        UINT32 closestWidthDiff  = static_cast<UINT32>(-1);
        UINT32 closestHeightDiff = static_cast<UINT32>(-1);
        UINT32 closestFpsDiff    = static_cast<UINT32>(-1);
        for (i = 0; i < modeCount; i++)
        {
            if (pAllModes[i].frameRate >= static_cast<UINT32>(pDesiredSensorMode->frameRate) &&
                ChxSensorModeSelect::IsMatchingCapability(&pAllModes[i], pDesiredSensorMode) &&
                (pAllModes[i].frameDimension.width <= pDesiredSensorMode->optimalWidth) &&
                (pAllModes[i].frameDimension.height <= pDesiredSensorMode->optimalHeight))
            {
                 if ((closestWidthDiff  >= pDesiredSensorMode->optimalWidth - pAllModes[i].frameDimension.width) &&
                     (closestHeightDiff >= pDesiredSensorMode->optimalHeight - pAllModes[i].frameDimension.height) &&
                     (closestFpsDiff    >= pAllModes[i].frameRate - (static_cast<UINT32>(pDesiredSensorMode->frameRate))))
                 {
                     found             = TRUE;
                     closestModeIndex  = i;
                     closestWidthDiff  = pDesiredSensorMode->optimalWidth - pAllModes[i].frameDimension.width;
                     closestHeightDiff = pDesiredSensorMode->optimalHeight - pAllModes[i].frameDimension.height;
                     closestFpsDiff    = pAllModes[i].frameRate - (static_cast<UINT32>(pDesiredSensorMode->frameRate));
                 }
            }
        }
    }
    else if (0 == closestModeIndex)
    {
        closestModeIndex = closestSecondIndex;
    }

    return closestModeIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::IsMatchingFSMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChxSensorModeSelect::IsMatchingFSMode(
    const ChiSensorModeInfo* pModeInfo)
{
    BOOL   isFound = FALSE;

    CHX_ASSERT(StreamImage == pModeInfo->streamtype);

    if (1 == pModeInfo->sensorModeCaps.u.FS)
    {
        isFound = TRUE;
    }

    CHX_LOG(" IsMatching FS(Fast shutter) %d", isFound);

    return isFound;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::SelectUseCase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SensorModeSelectUseCase ChxSensorModeSelect::SelectUseCase(
    UINT32                   cameraId,
    const DesiredSensorMode* pDesiredSensorMode)
{
    SensorModeSelectUseCase usecase                  = MaxModeSelectUsecase;
    UINT32                  modeCount                = 0;
    ChiSensorModeInfo*      pAllModes                = NULL;
    BOOL                    isSensorSupportIHDR      = FALSE;
    BOOL                    isQuadraCFASensor        = FALSE;
    BOOL                    isInSensorHDR3ExpPreview = (SelectInSensorHDR3ExpUsecase::InSensorHDR3ExpPreview ==
                                                    ExtensionModule::GetInstance()->SelectInSensorHDR3ExpUsecase())? TRUE:FALSE;
    CDKResult               result                   =
        ExtensionModule::GetInstance()->GetPhysicalCameraSensorModes(cameraId, &modeCount, &pAllModes);

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = modeCount; i > 0; --i)
        {
            if (1 == pAllModes[i - 1].sensorModeCaps.u.IHDR)
            {
                isSensorSupportIHDR = TRUE;
                break;
            }
        }

        for (UINT i = 0; i < modeCount; i++)
        {
            if (1 == pAllModes[i].sensorModeCaps.u.QuadCFA)
            {
                isQuadraCFASensor = TRUE;
                break;
            }
        }
    }

    if ((pDesiredSensorMode->frameRate >= MinHFRFps) || (1 == pDesiredSensorMode->sensorModeCaps.u.HFR))
    {
        usecase = UsecaseHFR;
    }
    else if (1 == pDesiredSensorMode->sensorModeCaps.u.ZZHDR)
    {
        usecase = UsecaseRHDR;
    }
    else if ((TRUE == isSensorSupportIHDR) && (TRUE == isInSensorHDR3ExpPreview))
    {
        usecase = UsecaseIHDR;
    }
    else if ((TRUE == isQuadraCFASensor) && (TRUE == pDesiredSensorMode->sensorModeCaps.u.QuadCFA))
    {
        // UsecaseQuadra for Quadra CFA senosr
        usecase = UsecaseQuadra;
    }
    else if (1 == pDesiredSensorMode->sensorModeCaps.u.FS)
    {
        usecase = UsecaseFS;
    }
    else
    {
        /// @todo Reference implementation to be filled with customized usecase selection
        usecase = UsecaseVideoPreview;
    }

    return usecase;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChxSensorModeSelect::FindBestSensorMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiSensorModeInfo* ChxSensorModeSelect::FindBestSensorMode(
    UINT32                   cameraId,
    const DesiredSensorMode* pDesiredSensorMode)
{
    BOOL                        found               = FALSE;
    CDKResult                   result              = CDKResultSuccess;
    ChiSensorModeInfo*          pSelectedMode       = NULL;
    UINT32                      modeCount           = 0;
    ChiSensorModeInfo*          pAllModes           = NULL;
    SensorSelectBestResolution  selectedResolution  = {0};
    SensorSelectBestAspectRatio selectedAspectRatio = {10};

    result        = ExtensionModule::GetInstance()->GetPhysicalCameraSensorModes(cameraId, &modeCount, &pAllModes);
    pSelectedMode = &pAllModes[0]; // Initialize with default mode

    if ((CDKResultSuccess == result) &&
        (NULL != pDesiredSensorMode) &&
        (pDesiredSensorMode->forceMode >= modeCount))
    {
        CHX_LOG("Desired sensor mode:");
        CHX_LOG("    optimalWidth: %d", pDesiredSensorMode->optimalWidth);
        CHX_LOG("    optimalHeight: %d", pDesiredSensorMode->optimalHeight);
        CHX_LOG("    maxWidth: %d", pDesiredSensorMode->maxWidth);
        CHX_LOG("    maxHeight: %d", pDesiredSensorMode->maxHeight);
        CHX_LOG("    minWidth: %d", pDesiredSensorMode->minWidth);
        CHX_LOG("    minHeight: %d", pDesiredSensorMode->minHeight);
        CHX_LOG("    frameRate: %f", pDesiredSensorMode->frameRate);
        CHX_LOG("    sensorModeCaps: %08x", pDesiredSensorMode->sensorModeCaps.value);
        CHX_LOG("Available sensor modes:");
        for (UINT32 i = 0; i < modeCount; i++)
        {
            CHX_LOG("    ModeInfo[%d]:", i);
            CHX_LOG("        modeIndex: %d", pAllModes[i].modeIndex);
            CHX_LOG("        arraySizeInMPix: %d", pAllModes[i].arraySizeInMPix);
            CHX_LOG("        frameDimension: TL:(%d, %d)  width: %d height: %d",
                    pAllModes[i].frameDimension.left, pAllModes[i].frameDimension.top,
                    pAllModes[i].frameDimension.width, pAllModes[i].frameDimension.height);
            CHX_LOG("        cropInfo: TL:(%d, %d)  width: %d height: %d",
                    pAllModes[i].cropInfo.left, pAllModes[i].cropInfo.top,
                    pAllModes[i].cropInfo.width, pAllModes[i].cropInfo.height);
            CHX_LOG("        aspectRatio: %d / %d",
                    pAllModes[i].aspectRatio.numerator, pAllModes[i].aspectRatio.denominator);
            CHX_LOG("        bpp: %d", pAllModes[i].bpp);
            CHX_LOG("        framerate: %d", pAllModes[i].frameRate);
            CHX_LOG("        batchedFrames: %d", pAllModes[i].batchedFrames);
            CHX_LOG("        caps: %08x", pAllModes[i].sensorModeCaps.value);
            CHX_LOG("        streamType: %08x", pAllModes[i].streamtype);
        }

        SensorModeSelectUseCase useCase = SelectUseCase(cameraId, pDesiredSensorMode);

        for (UINT32 i = modeCount; i > 0; --i)
        {
            BOOL   isFound   = FALSE;
            UINT32 modeIndex = (i - 1);

            /// Go through all the usecases in the select table, if any of the selected usecase fails then use default res
            for (UINT32 condition = 0; condition < MaxModeSelectCondition; condition++)
            {
                if (TRUE == ModeSelectTable[condition][useCase])
                {
                    SensorModeSelectCondition selectCondition = static_cast<SensorModeSelectCondition>(condition);
                    CHX_LOG("Select sensor mode %d condition = %d", modeIndex, selectCondition);

                    switch (selectCondition)
                    {
                        case ConditionFrameRate:
                            isFound = ChxSensorModeSelect::IsMatchingFrameRate(&pAllModes[modeIndex], pDesiredSensorMode);
                            break;

                        case ConditionAspectRatio:
                            isFound = ChxSensorModeSelect::IsMatchingAspectRatio(&pAllModes[modeIndex],
                                                                                 pDesiredSensorMode,
                                                                                 &selectedAspectRatio);
                            break;

                        case ConditionWidth:
                            isFound = ChxSensorModeSelect::IsMatchingWidth(&pAllModes[modeIndex], pDesiredSensorMode);
                            break;

                        case ConditionHeight:
                            isFound = ChxSensorModeSelect::IsMatchingHeight(&pAllModes[modeIndex], pDesiredSensorMode);
                            break;

                        case ConditionMPIX:
                            isFound = ChxSensorModeSelect::IsMatchingMaxResolution(&pAllModes[modeIndex], &selectedResolution);
                            break;

                        case ConditionHFR:
                            isFound = ChxSensorModeSelect::IsMatchingHFR(&pAllModes[modeIndex], pDesiredSensorMode);
                            break;

                        case ConditionBestResolution:
                            if (FALSE == ModeSelectTable[ConditionMPIX][useCase])
                            {
                                if ((TRUE == ModeSelectTable[ConditionAspectRatio][useCase]) &&
                                    (selectedAspectRatio.tempAspectRatioDiff <
                                    selectedAspectRatio.lastAspectRatioDiff))
                                {
                                    isFound = ChxSensorModeSelect::IsMatchingMaxResolution(&pAllModes[modeIndex],
                                                                                           &selectedResolution);
                                }
                                else
                                {
                                    isFound = ChxSensorModeSelect::IsMatchingBestResolution(&pAllModes[modeIndex],
                                                                                            pDesiredSensorMode,
                                                                                            &selectedResolution);
                                }
                            }
                            break;

                        case ConditionFS:
                            isFound = ChxSensorModeSelect::IsMatchingFSMode(&pAllModes[modeIndex]);
                            break;
                        case ConditionCapability:
                            isFound = ChxSensorModeSelect::IsMatchingCapability(&pAllModes[modeIndex],
                                                                                pDesiredSensorMode);
                            break;

                        default:
                            CHX_LOG("Condition %d No implemented", selectCondition);
                            break;
                    }

                    if (FALSE == isFound)
                    {
                        CHX_LOG("Sensor mode %d Condition %d doesn't match", modeIndex, selectCondition);
                        break;
                    }
                }
            }

            /// If isFound is still TRUE at the end , then we select this mode
            if (TRUE == isFound)
            {
                found                                   = TRUE;
                pSelectedMode                           = &pAllModes[modeIndex];
                selectedResolution.lastResolution       = selectedResolution.temporaryResolution;
                selectedAspectRatio.lastAspectRatioDiff = selectedAspectRatio.tempAspectRatioDiff;

                CHX_LOG("Selecting mode %d", modeIndex);
            }
        }

        if (FALSE == found)
        {
            UINT32 modeIndex = ChxSensorModeSelect::FindClosestMode(pAllModes,
                                                                    modeCount,
                                                                    pDesiredSensorMode);
            pSelectedMode = &pAllModes[modeIndex];
            CHX_LOG("Didn't find best sensor mode, select a closest sensor mode %d", modeIndex);
        }

        CHX_LOG_CONFIG("Selected Usecase: %d, SelectedMode W=%d, H=%d, FPS:%d, NumBatchedFrames:%d, modeIndex:%d",
                      useCase,
                      pSelectedMode->frameDimension.width,
                      pSelectedMode->frameDimension.height,
                      pSelectedMode->frameRate,
                      pSelectedMode->batchedFrames,
                      pSelectedMode->modeIndex);
    }
    else
    {
        if ((NULL != pDesiredSensorMode) && (pDesiredSensorMode->forceMode < modeCount))
        {
            pSelectedMode = &pAllModes[pDesiredSensorMode->forceMode];
            CHX_LOG_ERROR("***FORCING SENSOR MODE %d  -  for debug only", pDesiredSensorMode->forceMode);
        }
        else
        {
            ALOGE("Desired Sensor Mode is NULL pointer");
            result = CDKResultEInvalidPointer;
        }
    }

    return pSelectedMode;
}
