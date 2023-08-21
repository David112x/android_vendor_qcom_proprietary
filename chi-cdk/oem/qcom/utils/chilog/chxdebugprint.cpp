////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxdebugprint.cpp
/// @brief Debug Print related defines
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdarg.h>
#include "chxdebugprint.h"
#include "camxchinodeutil.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

static CHIOFFLINELOGOPS        g_offlineLoggerOps;
BOOL ChiLog::g_asciiLogEnable = TRUE;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Log::LogSystem
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiLog::LogSystem(
    const CHAR*     pModule,
    const CHAR*     pLoglevelchar,
    const CHAR*     pFileName,
    const CHAR*     pFuncName,
    INT32           lineNum,
    const CHAR*     pFormat,
    ...)
{
    if ((TRUE == ChiLog::g_asciiLogEnable) &&
        (NULL != g_offlineLoggerOps.pIsEnableOfflineLog && NULL != g_offlineLoggerOps.pAddLog))
    {
        if (g_offlineLoggerOps.pIsEnableOfflineLog)
        {
#if defined (ANDROID)
            CHAR strBuffer[MaxLogLength];
            CHAR finalStr[MaxLogLength];
            SIZE_T timestampLen = 0;
            SIZE_T logLen = 0;

            struct timespec spec;
            clock_gettime(CLOCK_REALTIME, &spec);
            struct tm* pCurrentTime = localtime((time_t *)&spec.tv_sec);

            if (pCurrentTime != NULL)
            {
                timestampLen = snprintf(strBuffer, MaxLogLength - 1, "%02d-%02d %02d:%02d:%02d:%09ld  %4d  %4d %s ",
                    (pCurrentTime->tm_mon + 1),
                    pCurrentTime->tm_mday,
                    pCurrentTime->tm_hour,
                    pCurrentTime->tm_min,
                    pCurrentTime->tm_sec,
                    spec.tv_nsec,
                    getpid(),
                    gettid(),
                    pLoglevelchar);
            }

            // Generate output string
            va_list args;
            va_start(args, pFormat);
            logLen = snprintf(strBuffer + timestampLen, sizeof(strBuffer) - timestampLen, "CHIUSECASE: %s %s:%d %s() ",
                pModule, pFileName, lineNum, pFuncName);
            logLen += vsnprintf(strBuffer + timestampLen + logLen, sizeof(strBuffer) - timestampLen - logLen, pFormat, args);
            va_end(args);

            SIZE_T len = ((timestampLen + logLen) > (MaxLogLength - 3)) ? MaxLogLength - 3 : timestampLen + logLen;
            strBuffer[len]   = '\n';
            strBuffer[len+1] = '\0';
            g_offlineLoggerOps.pAddLog(OfflineLoggerType::ASCII, strBuffer);
#endif // ANDROID
        }
    }
}

VOID ChiLog::SetOfflineLoggerOps(
        CHIOFFLINELOGOPS* pOfflineLogOps
    )
{
    g_offlineLoggerOps = *pOfflineLogOps;
}
