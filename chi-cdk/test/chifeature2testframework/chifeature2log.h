////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2log.h
/// @brief Declarations of logging framework.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2LOG_H
#define CHIFEATURE2LOG_H

#include <stdio.h>
#include <vector>
#include "cmdlineparser.h"
#include "chxutils.h"

// Determine the compiler's environment
#if ((ULONG_MAX) == (UINT_MAX))
#define ENVIRONMENT32
#else
#define ENVIRONMENT64
#endif

// Define ChiFeature2Test App version
#define CF2_VERSION "Version 1.0.0"

#ifndef __FILENAME__
#define __FILENAME__ ChxUtils::GetFileName(__FILE__)
#endif

// Maximum string length
#define CF2_STRLEN_MAX 1024

// Definition for unused parameter
#define CF2_UNUSED_PARAM(expr) (void)(expr)

// Definition for CF2_LOG macro
#define CF2_LOG(logLevel, ...) if(logLevel >= CF2Log.m_eLogLevel) { CF2Log.Log( logLevel, ##__VA_ARGS__); }

// Define logging levels
#define CF2_LOG_ENTRY() \
    CF2_LOG(eEntry, "%s: %s: Line %d", __FILENAME__, __FUNCTION__, __LINE__);\
    CHX_LOG_VERBOSE("ENTRY");
#define CF2_LOG_EXIT() \
    CF2_LOG(eExit,  "%s: %s: Line %d", __FILENAME__, __FUNCTION__, __LINE__);\
    CHX_LOG_VERBOSE("EXIT");
#define CF2_LOG_DEBUG( FMT, ... ) \
    CF2_LOG(eDebug, FMT, ##__VA_ARGS__ );\
    CHX_LOG_DEBUG( FMT, ##__VA_ARGS__ );
#define CF2_LOG_INFO( FMT, ... ) \
    CF2_LOG(eInfo, FMT, ##__VA_ARGS__ );\
    CHX_LOG_INFO( FMT, ##__VA_ARGS__ );
#define CF2_LOG_PERF( FMT, ... ) \
    CF2_LOG(ePerformance, FMT, ##__VA_ARGS__ );\
    CHX_LOG_INFO( FMT, ##__VA_ARGS__ );
#define CF2_LOG_UNSUPP( FMT, ... ) \
    CF2_LOG(eUnsupported, FMT, ##__VA_ARGS__ );\
    CHX_LOG_WARN( FMT, ##__VA_ARGS__ );
#define CF2_LOG_WARN( FMT, ... ) \
    CF2_LOG(eWarning, FMT, ##__VA_ARGS__ );\
    CHX_LOG_WARN( FMT, ##__VA_ARGS__ );
#define CF2_LOG_ERROR( FMT, ... ) \
    CF2_LOG(eError, FMT, ##__VA_ARGS__ );\
    CHX_LOG_ERROR( FMT, ##__VA_ARGS__ );

enum verboseSeverity
{
    eEntry       = 0, // verbose developer function entry debug messages
    eExit        = 1, // verbose developer function exit debug messages
    eDebug       = 2, // verbose developer debug messages
    eInfo        = 3, // generic developer info messages
    ePerformance = 4, // generic developer performance info messages
    eUnsupported = 5, // typical end user unsupported messages
    eWarning     = 6, // non-critical warning messages
    eError       = 7  // critical error messages
};

class ChiFeature2Log
{
public:
    verboseSeverity m_eLogLevel;

    ChiFeature2Log();
    ~ChiFeature2Log() = default;

    static void Log(verboseSeverity severity, const char* mFormat, ...);
    static std::string GetMSGPrefixString(verboseSeverity severity);
    int SetLogLevel(verboseSeverity level);
};

extern ChiFeature2Log CF2Log;

#endif //CHIFEATURE2LOG_H
