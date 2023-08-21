////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxdebugprint.h
/// @brief Debug print related defines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CHXDEBUGPRINT_H
#define CHXDEBUGPRINT_H

#include <stdio.h>
#include <string>
#include <time.h>
#include <inttypes.h>
#include "chiofflineloggerinterface.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

/// @brief Bitmask of logs to enable: Bit 0 - Error, Bit 1 - Warn, Bit 2 - Info, Bit 3 - Debug
extern UINT32 g_enableChxLogs;

/// @brief Enables request mapping logs
extern BOOL g_logRequestMapping;

/// @brief Enables System logging
extern BOOL g_enableSystemLog;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the Log class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiLog
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief The maximum length for a single debug print message
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const UINT MaxLogLength = 1024;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief The maximum length for a timestamp debug print message
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const UINT MaxTimeStampLength = 80;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Flag which matches with settings manager whether ASCII logger is enabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDK_VISIBILITY_PUBLIC static BOOL   g_asciiLogEnable;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LogSystem
    ///
    /// @brief  Logs the given format, va_args to the system's standard output mechanism
    ///
    /// @param  level    Verbosity level of the log
    /// @param  pFormat  Format string, printf style
    /// @param  ...      Parameters required by format
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID LogSystem(
        const CHAR*     module,
        const CHAR*     pLoglevelchar,
        const CHAR*     pFileName,
        const CHAR*     pFuncName,
        INT32           lineNum,
        const CHAR*     pFormat,
        ...);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetOfflineLoggerOps
    ///
    /// @brief  To set CamX offlinelogger hooks for CHI to use
    ///
    /// @param  pOfflineLogOps    Function pointers to CamX offlinelogger operation
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDK_VISIBILITY_PUBLIC VOID SetOfflineLoggerOps(
        CHIOFFLINELOGOPS* pOfflineLogOps);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFileName
    ///
    /// @brief  To get full file path for log system
    ///
    /// @param  pFilePath    filepath
    ///
    /// @return CHAR*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static const inline CHAR* GetFileName(
    const CHAR* pFilePath)
    {
        const CHAR* pFileName = strrchr(pFilePath, '/');

        if (NULL != pFileName)
        {
            // StrRChr will return a pointer to the /, advance one to the filename
            pFileName += 1;
        }
        else
        {
            pFileName = pFilePath;
        }

        return pFileName;
    }

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Log
    ///
    /// @brief Default constructor
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiLog() = default;
    ChiLog(const ChiLog&) = delete;
    ChiLog& operator=(const ChiLog&) = delete;
};

static const UINT32 CHX_LOG_ERROR_MASK   = 1;
static const UINT32 CHX_LOG_WARN_MASK    = 2;
static const UINT32 CHX_LOG_CONFIG_MASK  = 4;
static const UINT32 CHX_LOG_INFO_MASK    = 8;
static const UINT32 CHX_LOG_DUMP_MASK    = 16;
static const UINT32 CHX_LOG_VERBOSE_MASK = 32;
static const UINT32 CHX_LOG_MASK         = 64;

#if defined (_WINDOWS)    // This file for Win32 build only

#include <malloc.h>
#include <windows.h>
#define CHX_LOG_ERROR
#define CHX_LOG_WARN
#define CHX_LOG_CONFIG
#define CHX_LOG_INFO
#define CHX_LOG_VERBOSE
#define CHX_LOG
#define CHX_LOG_REQMAP
#define CHX_LOG_DUMP
#define CHX_LOG_DEBUG
#define ATRACE_BEGIN(...)
#define ATRACE_END(...)
#define CHX_LOG_TO_FILE(...)

#define LOG_ERROR(group, fmt, ...)   printf(fmt, __VA_ARGS__)
#define LOG_WARN(group, fmt, ...)    printf(fmt, __VA_ARGS__)
#define LOG_VERBOSE(group, fmt, ...) printf(fmt, __VA_ARGS__)
#define LOG_INFO(group, fmt, ...)    printf(fmt, __VA_ARGS__)

#else
#include <stdlib.h>
#include <log/log.h>
#undef LOG_TAG
#define LOG_TAG "CHIUSECASE"

#define CHX_LOG_SYSTEM(module, loglevelchar, fmt, args...)                     \
        ChiLog::LogSystem(                                                     \
        module,                                                                \
        loglevelchar,                                                          \
        ChiLog::GetFileName(__FILE__),                                         \
        __func__,                                                              \
        __LINE__,                                                              \
        fmt,                                                                   \
        ##args);                                                               \

#define CHX_LOG_REQMAP(fmt, args...)                                                                        \
    if (TRUE == g_logRequestMapping)                                                                        \
    {                                                                                                       \
        if (TRUE == g_enableSystemLog)                                                                      \
        {                                                                                                   \
            ALOGI("[REQMAP ] %s:%d %s() " fmt "\n", CHX_FILENAME(__FILE__), __LINE__, __func__, ##args);    \
        }                                                                                                   \
        CHX_LOG_SYSTEM("[REQMAP ]", "I", fmt , ##args);                                                     \
    }

#define CHX_LOG_ERROR(fmt, args...)                                                                        \
    if (CHX_LOG_ERROR_MASK == (g_enableChxLogs & CHX_LOG_ERROR_MASK))                                      \
    {                                                                                                      \
        if (TRUE == g_enableSystemLog)                                                                     \
        {                                                                                                  \
            ALOGE("[ERROR  ] %s:%d %s() " fmt "\n", CHX_FILENAME(__FILE__), __LINE__, __func__, ##args);   \
        }                                                                                                  \
        CHX_LOG_SYSTEM("[ERROR  ]", "E", fmt , ##args);                                                    \
    }

#define CHX_LOG_WARN(fmt, args...)                                                                        \
    if (CHX_LOG_WARN_MASK == (g_enableChxLogs & CHX_LOG_WARN_MASK))                                       \
    {                                                                                                     \
        if (TRUE == g_enableSystemLog)                                                                    \
        {                                                                                                 \
            ALOGW("[WARN   ] %s:%d %s() " fmt "\n", CHX_FILENAME(__FILE__), __LINE__, __func__, ##args);  \
        }                                                                                                 \
        CHX_LOG_SYSTEM("[WARN   ]", "W", fmt , ##args);                                                   \
    }

#define CHX_LOG_CONFIG(fmt, args...)                                                                       \
    if (CHX_LOG_CONFIG_MASK == (g_enableChxLogs & CHX_LOG_CONFIG_MASK))                                    \
    {                                                                                                      \
        if (TRUE == g_enableSystemLog)                                                                     \
        {                                                                                                  \
           ALOGI("[CONFIG ] %s:%d %s() " fmt "\n", CHX_FILENAME(__FILE__), __LINE__, __func__, ##args);    \
        }                                                                                                  \
       CHX_LOG_SYSTEM("[CONFIG ]", "I", fmt , ##args);                                                     \
    }

#define CHX_LOG_INFO(fmt, args...)                                                                         \
    if (CHX_LOG_INFO_MASK == (g_enableChxLogs & CHX_LOG_INFO_MASK))                                        \
    {                                                                                                      \
        if (TRUE == g_enableSystemLog)                                                                     \
        {                                                                                                  \
           ALOGI("[INFO   ] %s:%d %s() " fmt "\n", CHX_FILENAME(__FILE__), __LINE__, __func__, ##args);    \
        }                                                                                                  \
        CHX_LOG_SYSTEM("[INFO   ]", "I", fmt , ##args);                                                    \
    }

#define CHX_LOG_DUMP(fmt, args...)                                                                       \
    if (CHX_LOG_DUMP_MASK == (g_enableChxLogs & CHX_LOG_DUMP_MASK))                                      \
    {                                                                                                    \
        if (TRUE == g_enableSystemLog)                                                                   \
        {                                                                                                \
            ALOGD("[DUMP   ] %s:%d %s() " fmt "\n", CHX_FILENAME(__FILE__), __LINE__, __func__, ##args); \
        }                                                                                                \
        CHX_LOG_SYSTEM("[DUMP   ]", "I", fmt , ##args);                                                  \
    }

#define CHX_LOG_VERBOSE(fmt, args...)                                                                     \
    if (CHX_LOG_VERBOSE_MASK == (g_enableChxLogs & CHX_LOG_VERBOSE_MASK))                                 \
    {                                                                                                     \
        if (TRUE == g_enableSystemLog)                                                                    \
        {                                                                                                 \
            ALOGI("[VERBOSE] %s:%d %s() " fmt "\n", CHX_FILENAME(__FILE__), __LINE__, __func__, ##args);  \
        }                                                                                                 \
        CHX_LOG_SYSTEM("[VERBOSE]", "V", fmt , ##args);                                                   \
    }

#define CHX_IS_VERBOSE_ENABLED()                                                                         \
        (CHX_LOG_VERBOSE_MASK == (g_enableChxLogs & CHX_LOG_VERBOSE_MASK))                               \

#define CHX_LOG(fmt, args...)                                                                            \
    if (CHX_LOG_MASK == (g_enableChxLogs & CHX_LOG_MASK))                                                \
    {                                                                                                    \
        if (TRUE == g_enableSystemLog)                                                                   \
        {                                                                                                \
            ALOGD("[FULL   ] %s:%d %s() " fmt "\n", CHX_FILENAME(__FILE__), __LINE__, __func__, ##args); \
        }                                                                                                \
        CHX_LOG_SYSTEM("[FULL   ]", "D", fmt , ##args);                                                  \
    }

#define CHX_LOG_DEBUG(fmt, args...)                                                                  \
    ALOGD("[DEBUG  ] %s:%d %s() " fmt "\n", CHX_FILENAME(__FILE__), __LINE__, __func__, ##args);     \
    CHX_LOG_SYSTEM("[DEBUG  ]", "D", fmt , ##args);                                                  \

#define CHX_LOG_TO_FILE(fd, indent, fmt, ...)                                                         \
    ChxUtils::DPrintF(fd, "%*s" fmt "\n", indent, "", ##__VA_ARGS__);                                 \

#define LOG_ERROR(group, fmt, args...)                                                  \
    ALOGE("%s():%d " fmt "\n", __func__, __LINE__, ##args);                             \
    CHX_LOG_SYSTEM("[ERROR  ]", "I", fmt , ##args);                                     \

#define LOG_WARN(group, fmt, args...)                                                   \
    ALOGW("%s():%d " fmt "\n", __func__, __LINE__, ##args);                             \
    CHX_LOG_SYSTEM("[WARN    ]", "W", fmt , ##args);                                    \

#define LOG_VERBOSE(group, fmt, args...)                                                \
    ALOGV("%s():%d " fmt "\n", __func__, __LINE__, ##args);                             \
    CHX_LOG_SYSTEM("[VERBOSE]", "V", fmt , ##args);                                     \

#define LOG_INFO(group, fmt, args...)                                                   \
    ALOGI("%s():%d " fmt "\n", __func__, __LINE__, ##args);                             \
    CHX_LOG_SYSTEM("[INFO   ]", "I", fmt , ##args);                                     \

#endif // WIN32

#define CHX_SET_AND_LOG(variable, value, fmt_specifier) do {                                \
    CHX_LOG_INFO("Setting " #variable " from: %" fmt_specifier " to: %" fmt_specifier,      \
                 variable,                                                                  \
                 value);                                                                    \
    variable = value;                                                                       \
} while(0)

#define CHX_INC_AND_LOG(variable, fmt_specifier) do {                                       \
    CHX_LOG_INFO("Incrementing " #variable " from: %" fmt_specifier " to: %" fmt_specifier, \
                 variable,                                                                  \
                 (variable+1));                                                             \
    variable++;                                                                             \
} while(0)

#define CHX_SET_AND_LOG_UINT64(variable, value) CHX_SET_AND_LOG(variable, static_cast<UINT64>(value), PRIu64)
#define CHX_INC_AND_LOG_UINT64(variable) CHX_INC_AND_LOG(variable, PRIu64)

#endif // CHXDEBUGPRINT_H
