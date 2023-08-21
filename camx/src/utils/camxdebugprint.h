////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxdebugprint.h
/// @brief Debug print related defines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXDEBUGPRINT_H
#define CAMXDEBUGPRINT_H

#include <stdio.h>
#include <stdarg.h>
#include "camxdefs.h"
#include "camxtrace.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

extern struct DebugLogInfo g_logInfo;
extern        BOOL         g_logInfoUpdated;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the Log class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Log
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief The maximum length for a single debug print message
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const UINT MaxLogLength = 1024;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFileName
    ///
    /// @brief  Extracts just the file name from a file's path plus name
    ///
    /// @param  pFilePath String to parse for the filename
    ///
    /// @return File name
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const CHAR* GetFileName(
        const CHAR* pFilePath);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GroupToString
    ///
    /// @brief  Converts a LogGroup into a human readable string
    ///
    /// @param  group Group tag to stringize
    ///
    /// @return Stringized LogGroup
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const CHAR* GroupToString(
        CamxLogGroup group)
    {
        const CHAR* pString = NULL;
        switch (group)
        {
            case CamxLogGroupSensor:    pString = "[SENSOR ]";    break;
            case CamxLogGroupTracker:   pString = "[TRACKER]";    break;
            case CamxLogGroupISP:       pString = "[ISP    ]";    break;
            case CamxLogGroupPProc:     pString = "[PPROC  ]";    break;
            case CamxLogGroupMemMgr:    pString = "[MEMMGR ]";    break;
            case CamxLogGroupPower:     pString = "[POWER  ]";    break;
            case CamxLogGroupHAL:       pString = "[HAL    ]";    break;
            case CamxLogGroupJPEG:      pString = "[JPEG   ]";    break;
            case CamxLogGroupStats:     pString = "[STATS  ]";    break;
            case CamxLogGroupCSL:       pString = "[CSL    ]";    break;
            case CamxLogGroupApp:       pString = "[APP    ]";    break;
            case CamxLogGroupUtils:     pString = "[UTILS  ]";    break;
            case CamxLogGroupSync:      pString = "[SYNC   ]";    break;
            case CamxLogGroupMemSpy:    pString = "[MEMSPY ]";    break;
            case CamxLogGroupFormat:    pString = "[FORMAT ]";    break;
            case CamxLogGroupCore:      pString = "[CORE   ]";    break;
            case CamxLogGroupHWL:       pString = "[HWL    ]";    break;
            case CamxLogGroupDRQ:       pString = "[DRQ    ]";    break;
            case CamxLogGroupFD:        pString = "[FD     ]";    break;
            case CamxLogGroupIQMod:     pString = "[IQMod  ]";    break;
            case CamxLogGroupLRME:      pString = "[LRME/CVP]";   break;
            case CamxLogGroupChi:       pString = "[CHI    ]";    break;
            case CamxLogGroupNCS:       pString = "[NCS    ]";    break;
            case CamxLogGroupMeta:      pString = "[META   ]";    break;
            case CamxLogGroupAEC:       pString = "[STATS_AEC]";  break;
            case CamxLogGroupAWB:       pString = "[STATS_AWB]";  break;
            case CamxLogGroupAF:        pString = "[STATS_AF]";   break;
            case CamxLogGroupSWP:       pString = "[SW_PROC]";    break;
            case CamxLogGroupHist:      pString = "[HIST   ]";    break;
            case CamxLogGroupBPS:       pString = "[BPS    ]";    break;
            case CamxLogGroupDebugData: pString = "[DD_FWK ]";    break;
            case CamxLogGroupAFD:       pString = "[STATS_AFD]";  break;
            default:                    pString = "[UNKNOWN]";    break;
        }
        return pString;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateLogInfo
    ///
    /// @brief  Updates the debug log global with current settings
    ///
    /// @param  pNewLogInfo             New log info masks, enable, and file
    /// @param  bSetMaskToOfflineLogger Store log info mask to offlinelogger if needed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID UpdateLogInfo(
        DebugLogInfo* pNewLogInfo,
        BOOL          bSetMaskToOfflineLogger);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LogSystem
    ///
    /// @brief  Logs the given format, va_args to the system's standard output mechanism
    ///
    /// @param  group    Group to log
    /// @param  pLevel   One char to represent the log level
    /// @param  level    Verbosity level of the log
    /// @param  pFormat  Format string, printf style
    /// @param  ...      Parameters required by format
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID LogSystem(
        CamxLogGroup    group,
        const CHAR*     pLevel,
        UINT32          level,
        const CHAR*     pFormat,
        ...);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Log
    ///
    /// @brief Default constructor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Log() = default;
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Prints a string name on creation and destruction
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LogAuto
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LogAuto
    ///
    /// @brief  Constructor to print at the entry of a function and save the name
    ///
    /// @param  group     Group to log
    /// @param  pFile     File path
    /// @param  pLine     Line number
    /// @param  pName     Further identification
    /// @param  id        Frame/Request ID if applicable
    /// @param  isScope   pName generated from scope event type
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    LogAuto(
        CamxLogGroup    group,
        const CHAR*     pFile,
        const CHAR*     pLine,
        const CHAR*     pName,
        UINT64          id,
        BOOL            isScope);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~LogAuto
    ///
    /// @brief  Destructor to print at the exit of a function
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~LogAuto();

private:
    CamxLogGroup    m_group;        ///< Saved group
    const CHAR*     m_pFile;        ///< Saved file name
    const CHAR*     m_pFunction;    ///< Saved function name
    const CHAR*     m_pName;        ///< Saved further idenfication
    UINT64          m_id;           ///< Saved ID

    LogAuto(const LogAuto& rOther) = delete;
    LogAuto& operator=(const LogAuto& rCount) = delete;
};

#if CAMX_LOGS_ENABLED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief  Macro to log an arbitrary debug print to the system
///
/// @param  level LogVerbosity of the log
/// @param  levelString String to prepend for the level indicator
/// @param  group LogGroup of the log
/// @param  fmt Format string, printf style
/// @param  ... Parameters required by format
///
/// @return None
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG(levelchar, level, levelString, group, fmt, ...)                    \
    CamX::Log::LogSystem(                                                           \
        group,                                                                      \
        levelchar,                                                                  \
        (level),                                                                    \
        levelString "%s %s:" CAMX_STRINGIZE_MACRO(__LINE__) " %s() " fmt,           \
        CamX::Log::GroupToString(group),                                            \
        CamX::Log::GetFileName(__FILE__),                                           \
        __FUNCTION__,                                                               \
        ##__VA_ARGS__);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing debug logs to the system
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_DEBUG(group, fmt, ...)                                             \
    CAMX_LOG("D", CamxLogDebug, "[DEBUG]", group, fmt, ##__VA_ARGS__)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing error logs to the system
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_ERROR(group, fmt, ...)                                             \
    CAMX_LOG("E", CamxLogError, "[ERROR]", group, fmt, ##__VA_ARGS__)               \
    if (TRUE == CamX::g_traceInfo.traceErrorLogEnable)                              \
    {                                                                               \
        CAMX_TRACE_MESSAGE_F(group, "[ERROR]" fmt, ##__VA_ARGS__);                  \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing Dump logs to the system
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_DUMP(group, fmt, ...)                                                          \
    if ((CamX::g_logInfo.groupsEnable[CamxLogDump] & group) || FALSE == CamX::g_logInfoUpdated) \
    {                                                                                           \
        CAMX_LOG("I", CamxLogDump, "[ DUMP]", group, fmt, ##__VA_ARGS__)                        \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing config logs to the system
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_CONFIG(group, fmt, ...)                                                           \
    if ((CamX::g_logInfo.groupsEnable[CamxLogWarning] & group) || FALSE == CamX::g_logInfoUpdated) \
    {                                                                                              \
        CAMX_LOG("I", CamxLogConfig, "[CONFIG]", group, fmt, ##__VA_ARGS__)                        \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing warning logs to the system
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_WARN(group, fmt, ...)                                                             \
    if ((CamX::g_logInfo.groupsEnable[CamxLogWarning] & group) || FALSE == CamX::g_logInfoUpdated) \
    {                                                                                              \
        CAMX_LOG("W", CamxLogWarning, "[ WARN]", group, fmt, ##__VA_ARGS__)                        \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing info logs to the system
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_INFO(group, fmt, ...)                                                          \
    if ((CamX::g_logInfo.groupsEnable[CamxLogInfo] & group) || FALSE == CamX::g_logInfoUpdated) \
    {                                                                                           \
        CAMX_LOG("I", CamxLogInfo, "[ INFO]", group, fmt, ##__VA_ARGS__)                        \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing performance info logs to the system/trace
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_PERF_INFO(group, fmt, ...)                                                         \
    if ((CamX::g_logInfo.groupsEnable[CamxLogPerfInfo] & group) || FALSE == CamX::g_logInfoUpdated) \
    {                                                                                               \
        CAMX_LOG("I", CamxLogPerfInfo, "[PINFO]", group, fmt, ##__VA_ARGS__)                        \
    }                                                                                               \
    CAMX_TRACE_MESSAGE_F(group, fmt, ##__VA_ARGS__);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing performance warning logs to the system/trace
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_PERF_WARN(group, fmt, ...)                                                            \
    if ((CamX::g_logInfo.groupsEnable[CamxLogPerfWarning] & group) || FALSE == CamX::g_logInfoUpdated) \
    {                                                                                                  \
        CAMX_LOG("I", CamxLogPerfWarning, "[PWARN]", group, fmt, ##__VA_ARGS__)                        \
    }                                                                                                  \
    CAMX_TRACE_MESSAGE_F(group, fmt, ##__VA_ARGS__);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing DRQ info for debugging
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_DRQ(fmt, ...)                                                                           \
    if ((CamX::g_logInfo.groupsEnable[CamxLogDRQ] & CamxLogGroupDRQ) || FALSE == CamX::g_logInfoUpdated) \
    {                                                                                                    \
        CAMX_LOG("I", CamxLogDRQ, "[DRQ  ]", CamxLogGroupDRQ, fmt, ##__VA_ARGS__)                        \
    }                                                                                                    \
    CAMX_TRACE_MESSAGE_F(CamxLogGroupDRQ, "DRQ: " fmt, ##__VA_ARGS__);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing Metadata info for debugging
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_META(fmt, ...)                                                                            \
    if ((CamX::g_logInfo.groupsEnable[CamxLogMeta] & CamxLogGroupMeta) || FALSE == CamX::g_logInfoUpdated) \
    {                                                                                                      \
        CAMX_LOG("V", CamxLogMeta, "[META ]", CamxLogGroupMeta, fmt, ##__VA_ARGS__)                        \
    }                                                                                                      \
    CAMX_TRACE_MESSAGE_F(CamxLogGroupMeta, "META: " fmt, ##__VA_ARGS__);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing logs to a fd (always enabled)
///
/// @param fmt      Format string, printf style
/// @param indent   number of spaces to indent
/// @param ...      Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_TO_FILE(fd, indent, fmt, ...)                               \
    CamX::OsUtils::DPrintF(fd, "%*s" fmt "\n", indent, "", ##__VA_ARGS__);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing verbose logs to the system
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_VERBOSE(group, fmt, ...)                                                          \
    if ((CamX::g_logInfo.groupsEnable[CamxLogVerbose] & group) || FALSE == CamX::g_logInfoUpdated) \
    {                                                                                              \
        CAMX_LOG("V", CamxLogVerbose, "[ VERB]", group, fmt, ##__VA_ARGS__)                        \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing request mapping to the system
///
/// @param group LogGroup of the log
/// @param fmt Format string, printf style
/// @param ... Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_REQMAP(group, fmt, ...)                                                          \
    if ((CamX::g_logInfo.groupsEnable[CamxLogReqMap] & group) || FALSE == CamX::g_logInfoUpdated) \
    {                                                                                             \
        CAMX_LOG("I", CamxLogReqMap, "[REQMAP]", group, fmt, ##__VA_ARGS__)                       \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for checking if Verbose log enabled
///
/// @param group LogGroup of the log
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_IS_VERBOSE_ENABLED(group)                                           \
    (CamX::g_logInfo.groupsEnable[CamxLogVerbose] & group)                           \


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing auto entry/exit logs to the system and trace
///
/// @param group LogGroup of the log
/// @param event SCOPE event name to trace
/// @param id    Frame/Request ID to associate with the trace
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_ENTRYEXIT_SCOPE_ID(group, event, id)                              \
    CamX::LogAuto logEntryExitAuto((group), __FILE__, CAMX_STRINGIZE_MACRO(__LINE__), CAMX_STRINGIZE_MACRO(event), id, TRUE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing auto entry/exit logs to the system and trace
///
/// @param group LogGroup of the log
/// @param event SCOPE event name to trace
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_ENTRYEXIT_SCOPE(group, event) \
    CamX::LogAuto logEntryExitAuto((group), __FILE__, CAMX_STRINGIZE_MACRO(__LINE__), CAMX_STRINGIZE_MACRO(event), 0, TRUE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing auto entry/exit logs to the system
///
/// @note  Does not use SCOPE
///
/// @param group LogGroup of the log
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_ENTRYEXIT(group) \
    CamX::LogAuto logEntryExitAuto((group), __FILE__, CAMX_STRINGIZE_MACRO(__LINE__), __FUNCTION__, 0, FALSE)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing auto entry/exit logs to the system based on a name
///
/// @note  Does not use SCOPE
///
/// @param group LogGroup of the log
/// @param name  String name to use instead of standard __FUNCTION__
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_ENTRYEXIT_NAME(group, name) \
    CamX::LogAuto logEntryExitAuto((group), __FILE__, CAMX_STRINGIZE_MACRO(__LINE__), name, 0, FALSE)

#else

#define CAMX_LOG(level, levelString, group, fmt, ...)
#define CAMX_LOG_DEBUG(group, fmt, ...)
#define CAMX_LOG_ERROR(group, fmt, ...)
#define CAMX_LOG_WARN(group, fmt, ...)
#define CAMX_LOG_INFO(group, fmt, ...)
#define CAMX_LOG_VERBOSE(group, fmt, ...)
#define CAMX_LOG_PERF_INFO(group, fmt, ...)
#define CAMX_LOG_PERF_WARN(group, fmt, ...)
#define CAMX_LOG_DRQ(fmt, ...)
#define CAMX_LOG_META(fmt, ...)
#define CAMX_ENTRYEXIT_SCOPE_ID(group, event, id)
#define CAMX_ENTRYEXIT_SCOPE(group, event)
#define CAMX_ENTRYEXIT(group)
#define CAMX_ENTRYEXIT_NAME(group, name)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Macro for printing logs to a fd (always enabled)
///
/// @param fmt      Format string, printf style
/// @param indent   number of spaces to indent
/// @param ...      Parameters required by format
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CAMX_LOG_TO_FILE(fd, indent, fmt, ...)                               \
    CamX::OsUtils::DPrintF(fd, "%*s" fmt "\n", indent, "", ##__VA_ARGS__);


#endif // CAMX_LOGS_ENABLED

CAMX_NAMESPACE_END


#endif // CAMXDEBUGPRINT_H
