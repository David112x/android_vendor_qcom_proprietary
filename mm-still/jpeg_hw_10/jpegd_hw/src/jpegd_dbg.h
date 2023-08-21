/*============================================================================
   Copyright (c) 2012 Qualcomm Technologies, Inc.  All Rights Reserved.
   Qualcomm Technologies Proprietary and Confidential.
============================================================================*/

#ifndef __JPEGD_DBG_H__
#define __JPEGD_DBG_H__

#include <stdio.h>

#define JD_USE_DYNAMIC_LOGGING
#define JDDBG_LOG_LEVEL 3

/** Loglevels
 * g_jdloglevel <= 1 -> only error logs
 * g_jdloglevel = 2 -> error and high
 * g_jdloglevel = 3 -> error, high, medium
 * g_jdloglevel = 4 -> error, high, medium and low
 */
extern volatile uint32_t g_jdloglevel;

#undef JDDBG
#ifdef JD_USE_DYNAMIC_LOGGING
    #ifdef _ANDROID_
        #undef LOG_NIDEBUG
        #undef LOG_TAG
        #define LOG_NIDEBUG 0
        #define LOG_TAG "mm-still-jpegdhw"
        #include <utils/Log.h>
        #define JDDBG_HIGH(fmt, args...) ALOGD_IF(g_jdloglevel >= 2, fmt, ##args)
        #define JDDBG_MED(fmt, args...) ALOGD_IF(g_jdloglevel >= 3, fmt, ##args)
        #define JDDBG_LOW(fmt, args...) ALOGD_IF(g_jdloglevel >= 4, fmt, ##args)
    #else
        #define JDDBG_HIGH(fmt, args...) fprintf(stderr, fmt, ##args)
        #define JDDBG_MED(fmt, args...) fprintf(stderr, fmt, ##args)
        #define JDDBG_LOW(fmt, args...) fprintf(stderr, fmt, ##args)
    #endif
#elif (JDDBG_LOG_LEVEL > 0)
    #ifdef _ANDROID_
        #undef LOG_NIDEBUG
        #undef LOG_TAG
        #define LOG_NIDEBUG 0
        #define LOG_TAG "mm-still-jpegdhw"
        #include <utils/Log.h>
        #define JDDBG(fmt, args...) ALOGE(fmt, ##args)
        #define JDNDBG(fmt, args...) do{}while(0)
    #else
        #define JDDBG(fmt, args...) fprintf(stderr, fmt, ##args)
        #define JDNDBG(fmt, args...) do{}while(0)
    #endif
#else
    #define JDDBG(fmt, args...) do{}while(0)
    #define JDNDBG(fmt, args...) do{}while(0)
#endif

#define JDDBG_ERROR(fmt, args...) ALOGE(fmt, ##args)

#ifndef JD_USE_DYNAMIC_LOGGING
#if (JDDBG_LOG_LEVEL >= 4)
  #define JDDBG_HIGH(...)   JDDBG(__VA_ARGS__)
  #define JDDBG_MED(...)    JDDBG(__VA_ARGS__)
  #define JDDBG_LOW(...)    JDDBG(__VA_ARGS__)
#elif (JDDBG_LOG_LEVEL == 3)
  #define JDDBG_HIGH(...)   JDDBG(__VA_ARGS__)
  #define JDDBG_MED(...)    JDDBG(__VA_ARGS__)
  #define JDDBG_LOW(...)    JDNDBG(__VA_ARGS__)
#elif (JDDBG_LOG_LEVEL == 2)
  #define JDDBG_HIGH(...)   JDDBG(__VA_ARGS__)
  #define JDDBG_MED(...)    JDNDBG(__VA_ARGS__)
  #define JDDBG_LOW(...)    JDNDBG(__VA_ARGS__)
#elif (JDDBG_LOG_LEVEL == 1)
  #define JDDBG_HIGH(...)   JDNDBG(__VA_ARGS__)
  #define JDDBG_MED(...)    JDNDBG(__VA_ARGS__)
  #define JDDBG_LOW(...)    JDNDBG(__VA_ARGS__)
#else
  #define JDDBG_HIGH(...)   JDNDBG(__VA_ARGS__)
  #define JDDBG_MED(...)    JDNDBG(__VA_ARGS__)
  #define JDDBG_LOW(...)    JDNDBG(__VA_ARGS__)
#endif
#endif
#endif /* __JPEGD_DBG_H__ */
