/*
* Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
*/

#pragma once

#include <string>
#include <sstream>
#include <string.h>    // For strlen

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

// The only place for version definition/editing
#define TOOL_VERSION_MAJOR  1
#define TOOL_VERSION_MINOR  9
#define TOOL_VERSION_MAINT  0
#define TOOL_VERSION_INTERM 3

#define TOOL_VERSION            TOOL_VERSION_MAJOR,TOOL_VERSION_MINOR,TOOL_VERSION_MAINT,TOOL_VERSION_INTERM
#define TOOL_VERSION_STR        STR(TOOL_VERSION_MAJOR.TOOL_VERSION_MINOR.TOOL_VERSION_MAINT.TOOL_VERSION_INTERM)

#define MIN_DMTOOLS_VERSION     "3.4.0"

inline std::string GetToolsVersion()
{
    return TOOL_VERSION_STR;
}

inline const char* GetMinDmToolsVersion()
{
    return MIN_DMTOOLS_VERSION;
}

inline const char* GetOperatingSystemName()
{
    return
        //should check __ANDROID__ first since __LINUX flag also set in Android
#ifdef __ANDROID__
        "Android"
#elif __linux
        "Linux"
#elif _WINDOWS
        "Windows"
#else //OS3
        "NA"
#endif
        ;
}

#ifndef RC_INVOKED
    #ifndef PLATFORM_NAME
        #error "Build should define a PLATFORM_NAME macro"
    #endif
#endif

inline const char* GetPlatformName()
{
    return STR(PLATFORM_NAME);
}

inline std::string GetToolsBuildInfo()
{
    std::stringstream ss;

    ss << "802.11ad Tools Build Info:" << "\n"
       << "==========================" << "\n"
       << "Tools Version :           " << TOOL_VERSION_STR << "\n"
       << "OS:                       " << GetOperatingSystemName() << "\n"
       << "Build Platform:           " << GetPlatformName() << "\n"
       << "Minimal DmTools required: " << GetMinDmToolsVersion() << "\n";

#ifdef BUILD_ID
    if (strlen(BUILD_ID) > 0)
    {
        ss << "Build ID:                 " << BUILD_ID << "\n";
    }
#endif
#ifdef BUILD_DATE
    if (strlen(BUILD_DATE) > 0)
    {
        ss << "Build Date:               " << BUILD_DATE << "\n";
    }
#endif
#ifdef LAST_CHANGE_ID
    if (strlen(LAST_CHANGE_ID) > 0)
    {
        ss << "Last Change ID:           " << LAST_CHANGE_ID << "\n";
    }
#endif
#ifdef LAST_CHANGE_DATE
    if (strlen(LAST_CHANGE_DATE) > 0)
    {
        ss << "Last Change Date:         " << LAST_CHANGE_DATE;
    }
#endif

    return ss.str();
}