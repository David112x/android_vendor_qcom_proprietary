////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxdebugprint.cpp
/// @brief Debug Print related defines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxdebugprint.h"
#include "camxosutils.h"
#include "camxtrace.h"
#include "camxchiofflinelogger.h"

CAMX_NAMESPACE_BEGIN

// This holds offline logger instances cache for each type
static OfflineLogger* g_pOfflineLogger[OFFLINELOG_MAX_TYPE] = { NULL };

/// @todo (CAMX-944) Set log mask defaults here and in camxsettings.xml to 0x0
struct DebugLogInfo g_logInfo =
{
    {
        0xFFFFFFFF,     ////< CamxLogDebug (Unused)
        0xFFFFFFFF,     ////< CamxLogError (Unused)
        0xFFFFFFFF,     ////< CamxLogWarning
        0xFFFFFFFF,     ////< CamxLogConfig
        0xFFFFFFFF,     ////< CamxLogInfo
        0x0,            ////< CamxLogVerbose
        0x0,            ////< CamxLogPerfInfo
        0x0,            ////< CamxLogPerfWarning
        0x0,            ////< CamxLogDRQ
        0x0,            ////< CamxLogMeta
        0x0,            ////< CamxLogEntryExit
        0x0,            ////< CamxLogReqMap
        0x0,            ////< CamxLogDump
    },
    NULL,               ////< pDebugLogFile
    TRUE,               ////< systemLogEnable
    FALSE,              ////< enableAsciiLogging
    FALSE               ////< isUpdated
};

/// Flag to indicate the copy of camxutils has been updated by central instance
BOOL g_logInfoStored   = FALSE; // Stored the log to offlinelogger
BOOL g_logInfoUpdated  = FALSE; // Updated by central instance

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OfflineLoggerGetInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static CAMX_INLINE OfflineLogger* OfflineLoggerGetInstance(
    OfflineLoggerType type)
{
    if (NULL == g_pOfflineLogger[static_cast<UINT>(type)])
    {
        g_pOfflineLogger[static_cast<UINT>(type)] = OfflineLogger::GetInstance(type);
    }
    return g_pOfflineLogger[static_cast<UINT>(type)];
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Log::UpdateLogInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Log::UpdateLogInfo(
    DebugLogInfo* pNewLogInfo,
    BOOL          bSetMaskToOfflineLogger)
{
    // Update the debug log file
    if (NULL != g_logInfo.pDebugLogFile)
    {
        OsUtils::FClose(g_logInfo.pDebugLogFile);
        g_logInfo.pDebugLogFile = NULL;
    }

    g_logInfo.groupsEnable[CamxLogConfig]    = pNewLogInfo->groupsEnable[CamxLogConfig];
    g_logInfo.groupsEnable[CamxLogDump]      = pNewLogInfo->groupsEnable[CamxLogDump];
    g_logInfo.groupsEnable[CamxLogWarning]   = pNewLogInfo->groupsEnable[CamxLogWarning];
    g_logInfo.groupsEnable[CamxLogEntryExit] = pNewLogInfo->groupsEnable[CamxLogEntryExit];
    g_logInfo.groupsEnable[CamxLogInfo]      = pNewLogInfo->groupsEnable[CamxLogInfo];
    g_logInfo.groupsEnable[CamxLogPerfInfo]  = pNewLogInfo->groupsEnable[CamxLogPerfInfo];
    g_logInfo.groupsEnable[CamxLogVerbose]   = pNewLogInfo->groupsEnable[CamxLogVerbose];
    g_logInfo.groupsEnable[CamxLogDRQ]       = pNewLogInfo->groupsEnable[CamxLogDRQ];
    g_logInfo.groupsEnable[CamxLogMeta]      = pNewLogInfo->groupsEnable[CamxLogMeta];
    g_logInfo.groupsEnable[CamxLogReqMap]    = pNewLogInfo->groupsEnable[CamxLogReqMap];
    g_logInfo.pDebugLogFile                  = pNewLogInfo->pDebugLogFile;
    g_logInfo.systemLogEnable                = pNewLogInfo->systemLogEnable;
    g_logInfo.enableAsciiLogging             = pNewLogInfo->enableAsciiLogging;
    g_logInfo.isUpdated                      = pNewLogInfo->isUpdated;

#if defined (ANDROID)
    // Save the log mask to the offlinelogger. As there are potentially many clients calling UpdateLogInfo,
    // we use the bSetMaskToOfflineLogger flag to ensure that only one client sets the mask for the offline logger.
    if ((TRUE == g_logInfo.isUpdated) && (FALSE == g_logInfoStored) && (TRUE == bSetMaskToOfflineLogger))
    {
        g_logInfoStored  = TRUE;
        g_logInfoUpdated = TRUE;

        OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(OfflineLoggerType::ASCII);
        if (NULL != pOfflineLogger)
        {
            pOfflineLogger->SetLogInfoMask(&g_logInfo);
        }
    }
#endif // ANDROID
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Log::GetFileName
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHAR* Log::GetFileName(
    const CHAR* pFilePath)
{
    return OsUtils::GetFileName(pFilePath);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Log::LogSystem
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Log::LogSystem(
    CamxLogGroup group,
    const CHAR*  pLevel,
    CamxLog      level,
    const CHAR*  pFormat,
    ...)
{
    BOOL bNeedPrintLog = TRUE;

#if defined (ANDROID)
    // To check there is a distinct copies of g_logInfo and update it.
    if ((FALSE == g_logInfo.isUpdated) && (FALSE == g_logInfoUpdated))
    {
        OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(OfflineLoggerType::ASCII);

        if (NULL != pOfflineLogger)
        {
            DebugLogInfo storedLogInfo = pOfflineLogger->GetLogInfoMask();

            if (TRUE == storedLogInfo.isUpdated)
            {
                // Save stored log info into own copy of log info
                UpdateLogInfo(&storedLogInfo, FALSE);

                g_logInfoUpdated = TRUE;

                if (FALSE == (g_logInfo.groupsEnable[level] & group))
                {
                    // The first time entering this condition should NOT print log as we are not checking
                    // log group. This condition is forced entered to fetch the stored log info at init state.
                    bNeedPrintLog = FALSE;
                }
            }
        }
    }
#else
    // So far Offlinelogger only can be used in Android, so we need to skip the setLogMask part for Windows
    g_logInfo.isUpdated = TRUE;
#endif // ANDROID

    if (TRUE == bNeedPrintLog)
    {
        if ((TRUE == g_logInfo.systemLogEnable)    ||
            (TRUE == g_logInfo.enableAsciiLogging) ||
            (NULL != g_logInfo.pDebugLogFile))
        {
            CHAR logText[MaxLogLength];

            // Generate output string
            va_list args;
            va_start(args, pFormat);
            OsUtils::VSNPrintF(logText, sizeof(logText), pFormat, args);
            va_end(args);

            if (TRUE == g_logInfo.systemLogEnable)
            {
                OsUtils::LogSystem(level, logText);
            }

            if (TRUE == g_logInfo.enableAsciiLogging)
            {
#if defined (ANDROID)
                CHAR    finalText[MaxLogLength];
                SIZE_T  finalTxtLen                 = 0;

                OfflineLogger* pOfflineLogger = OfflineLoggerGetInstance(OfflineLoggerType::ASCII);
                if (NULL != pOfflineLogger)
                {
                    if (pOfflineLogger->IsEnableOfflinelogger())
                    {
                        struct timespec spec;
                        clock_gettime(CLOCK_REALTIME, &spec);

                        struct tm* pCurrentTime = localtime((time_t *)&spec.tv_sec);
                        if (pCurrentTime != NULL)
                        {
                            finalTxtLen = OsUtils::SNPrintF(finalText,
                                sizeof(finalText),
                                "%02d-%02d %02d:%02d:%02d:%09ld  %4d  %4d %s CamX: %s\n",
                                (pCurrentTime->tm_mon + 1),
                                pCurrentTime->tm_mday,
                                pCurrentTime->tm_hour,
                                pCurrentTime->tm_min,
                                pCurrentTime->tm_sec,
                                spec.tv_nsec,
                                OsUtils::GetProcessID(),
                                OsUtils::GetThreadID(),
                                pLevel,
                                logText);
                        }
                        else
                        {
                            finalTxtLen = OsUtils::SNPrintF(finalText, sizeof(finalText), "%s\n", logText);
                        }
                        pOfflineLogger->AddLog(finalText, static_cast<UINT>(finalTxtLen));
                    }
                }
#endif // ANDROID
            }

            if (NULL != g_logInfo.pDebugLogFile)
            {
                CamxDateTime systemDateTime;
                OsUtils::GetDateTime(&systemDateTime);
                OsUtils::FPrintF(g_logInfo.pDebugLogFile, "%02d-%02d %02d:%02d:%02d:%03d %s\n", systemDateTime.month,
                    systemDateTime.dayOfMonth, systemDateTime.hours, systemDateTime.minutes, systemDateTime.seconds,
                    systemDateTime.microseconds / 1000, logText);
            }
        }
    }


    /// @todo (CAMX-6) Debug print to xlog
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LogAuto::LogAuto
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LogAuto::LogAuto(
    CamxLogGroup    group,
    const CHAR*     pFile,
    const CHAR*     pLine,
    const CHAR*     pName,
    UINT64          id,
    BOOL            isScope)
    : m_group(group)
    , m_pName(pName)
    , m_id(id)
{
    if (TRUE == isScope)
    {
        if ('C' == m_pName[0])
        {
            // Skip CamX::SCOPEEvent in string
            m_pName += (sizeof(CHAR) * 16);
        }
        else
        {
            // Skip SCOPEEvent in string
            m_pName += (sizeof(CHAR) * 10);
        }
    }

    m_pFile = OsUtils::GetFileName(pFile);

    // Logging
    if (g_logInfo.groupsEnable[CamxLogEntryExit] & m_group)
    {
        Log::LogSystem(m_group,
                       "I",
                       CamxLogEntryExit,
                       "[ENTRY]%s %s:%s Entering %s()",
                       CamX::Log::GroupToString(m_group),
                       m_pFile,
                       pLine,
                       m_pName);
    }

    if (0 == m_id)
    {
        CAMX_TRACE_SYNC_BEGIN(m_group, m_pName);
    }
    else
    {
        CAMX_TRACE_SYNC_BEGIN_F(m_group, "%s : %llu", m_pName, m_id);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LogAuto::~LogAuto()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LogAuto::~LogAuto()
{
    if (g_logInfo.groupsEnable[CamxLogEntryExit] & m_group)
    {
        Log::LogSystem(m_group,
                       "I",
                       CamxLogEntryExit,
                       "[ EXIT]%s %s Exiting %s()",
                       CamX::Log::GroupToString(m_group),
                       m_pFile,
                       m_pName);
    }

    CAMX_TRACE_SYNC_END(m_group);
}

CAMX_NAMESPACE_END
