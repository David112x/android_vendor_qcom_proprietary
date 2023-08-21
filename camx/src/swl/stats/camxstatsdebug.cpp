////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  qcstatsdebug.cpp
/// @brief Statistics debug information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chistatsdebug.h"

#include "camxincs.h"
#include "camxstatsdebuginternal.h"

// NOWHINE ENTIRE FILE - legacy code ported temporarily

static const INT MaxStatsInfoLogLength = 250;

static CAMX_INLINE const CHAR* StatsSubGroupToString(StatsLogGroup group);

static CAMX_INLINE const CHAR* StatsLevelToString(StatsLog level);

static CAMX_INLINE const CHAR* StatsSubGroupToString(StatsLogGroup group)
{
    const CHAR* pString = NULL;
    switch (group)
    {
        case StatsLogGroupNone:
            pString = "[NONE:    ]";
            break;
        case StatsLogGroupAEC:
            pString = "[STATS_AEC]";
            break;
        case StatsLogGroupAF:
            pString = "[STATS_AF ]";
            break;
        case StatsLogGroupAWB:
            pString = "[STATS_AWB]";
            break;
        case StatsLogGroupAFD:
            pString = "[STATS_AFD]";
            break;
        case StatsLogGroupASD:
            pString = "[STATS_ASD]";
            break;
        case StatsLogGroupTracker:
            pString = "[STATS_TRK]";
            break;
        default:
            pString = "[STATS_UNK]";
            break;
    }

    return pString;

}

static CAMX_INLINE const CHAR* StatsLevelToSingleString(StatsLog level)
{
    const CHAR* pString = NULL;
    switch (level)
    {
    case StatsLogDebug:
        pString = "D";
        break;
    case StatsLogError:
        pString = "E";
        break;
    case StatsLogWarning:
        pString = "W";
        break;
    case StatsLogEntryExit:
        pString = "I";
        break;
    case StatsLogInfo:
        pString = "I";
        break;
    case StatsLogVerbose:
        pString = "V";
        break;
    default:
        pString = "U";
        break;
    }

    return pString;
}

static CAMX_INLINE const CHAR* StatsLevelToString(StatsLog level)
{
    const CHAR* pString = NULL;
    switch (level)
    {
        case StatsLogDebug:
            pString = "[DEBUG]";
            break;
        case StatsLogError:
            pString = "[ERROR]";
            break;
        case StatsLogWarning:
            pString = "[ WARN]";
            break;
        case StatsLogEntryExit:
            pString = "[ENTRY]";
            break;
        case StatsLogInfo:
            pString = "[ INFO]";
            break;
        case StatsLogVerbose:
            pString = "[ VERB]";
            break;
        default:
            pString = "[  UNK]";
            break;
    }

    return pString;
}

VOID MapStatsGroupToCamxGroup(StatsLogGroup group, CamxLogGroup* logGroup)
{
    switch (group)
    {
        case StatsLogGroupAEC:
            *logGroup = CamxLogGroupAEC;
            break;
        case StatsLogGroupAF:
            *logGroup = CamxLogGroupAF;
            break;
        case StatsLogGroupAWB:
            *logGroup = CamxLogGroupAWB;
            break;
        case StatsLogGroupAFD:
            *logGroup = CamxLogGroupAFD;
            break;
        case StatsLogGroupTracker:
            *logGroup = CamxLogGroupTracker;
            break;
        default:
            *logGroup = CamxLogGroupStats;
            break;
    }
}


VOID StatsLoggerFunction(
    const CHAR* pFileName,
    const INT lineNumber,
    const CHAR* pFunctionName,
    StatsLogGroup group,
    StatsLog level,
    const CHAR* pFormat,
    ...)
{
    CamxLog logSystemLevel                      = CamxLogError;
    CHAR    logText[CamX::Log::MaxLogLength];
    CHAR    statsInfo[MaxStatsInfoLogLength];
    BOOL    isLogMaskEnabled                    = FALSE;

    // TODO: There could be optimization here

    CamxLogGroup logGroup;
    MapStatsGroupToCamxGroup(group, &logGroup);

    switch (level)
    {
        case StatsLogDebug:
            logSystemLevel = CamxLogDebug;
            if (CamX::g_logInfo.groupsEnable[CamxLogDebug] & logGroup)
            {
                isLogMaskEnabled = TRUE;
            }
            break;
        case StatsLogError:
            logSystemLevel = CamxLogError;
            isLogMaskEnabled = TRUE;
            break;
        case StatsLogWarning:
            logSystemLevel = CamxLogWarning;
            if (CamX::g_logInfo.groupsEnable[CamxLogWarning] & logGroup)
            {
                isLogMaskEnabled = TRUE;
            }
            break;
        case StatsLogEntryExit:
            logSystemLevel = CamxLogEntryExit;
            if (CamX::g_logInfo.groupsEnable[CamxLogEntryExit] & logGroup)
            {
                isLogMaskEnabled = TRUE;
            }
            break;
        case StatsLogInfo:
            logSystemLevel = CamxLogInfo;
            if (CamX::g_logInfo.groupsEnable[CamxLogInfo] & logGroup)
            {
                isLogMaskEnabled = TRUE;
            }
            break;
        case StatsLogVerbose:
            logSystemLevel = CamxLogVerbose;
            if (CamX::g_logInfo.groupsEnable[CamxLogVerbose] & logGroup)
            {
                isLogMaskEnabled = TRUE;
            }
            break;
        default:
            break;
    }

    if (TRUE == isLogMaskEnabled)
    {
        CamX::Utils::Memset(logText, 0, sizeof(CamX::Log::MaxLogLength));
        CamX::Utils::Memset(statsInfo, 0, sizeof(MaxStatsInfoLogLength));

        // Generate output string
        va_list args;
        va_start(args, pFormat);
        CamX::OsUtils::VSNPrintF(logText, sizeof(logText), pFormat, args);
        va_end(args);

        CamX::OsUtils::SNPrintF(statsInfo,
            sizeof(statsInfo),
            "%s%s %s:%d: %s",
            StatsLevelToString(level),
            StatsSubGroupToString(group),
            pFileName,
            lineNumber,
            pFunctionName);

        CamX::Log::LogSystem(group, StatsLevelToSingleString(level), logSystemLevel, "%s %s", statsInfo, logText);
    }
}


