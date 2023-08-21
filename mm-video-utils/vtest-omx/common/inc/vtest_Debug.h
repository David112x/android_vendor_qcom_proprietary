/*-------------------------------------------------------------------
Copyright (c) 2013-2016 Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential
--------------------------------------------------------------------*/

#ifndef _VTEST_DEBUG_H
#define _VTEST_DEBUG_H

#include <stdio.h>
#include <string.h>

// undefine this to print results to the display
#define _ANDROID_LOG_

// define these to print the low/medium logs
#define _ANDROID_LOG_DEBUG_MED
#define _ANDROID_LOG_DEBUG_LOW

#ifdef _ANDROID_LOG_
#include <utils/Log.h>
#include <vtest_XmlComdef.h>
#undef LOG_NDEBUG
#undef LOG_TAG
#define LOG_NDEBUG 0
#define LOG_TAG "VTEST"

enum {
   VTEST_ERROR = 0x1,
   VTEST_FATAL = 0x1,
   VTEST_HIGH = 0x2,
   VTEST_MEDIUM = 0x2,
   VTEST_PROF = 0x2,
   VTEST_LOW = 0x4,
};

#define VTEST_MSG_HIGH(fmt, ...) ({ \
      if (sGlobalStaticVideoProp.logMask & VTEST_HIGH) \
          ALOGE("VT_HIGH %s::%d " fmt, __FUNCTION__, __LINE__,  ##__VA_ARGS__); \
      })
#define VTEST_MSG_PROFILE(fmt, ...) ({ \
      if (sGlobalStaticVideoProp.logMask & VTEST_PROF) \
          ALOGE("VT_PROFILE %s::%d " fmt, __FUNCTION__, __LINE__,  ##__VA_ARGS__); \
      })
#define VTEST_MSG_ERROR(fmt, ...) ({ \
      if (sGlobalStaticVideoProp.logMask & VTEST_ERROR) \
          ALOGE("VT_ERROR %s::%d " fmt, __FUNCTION__, __LINE__,  ##__VA_ARGS__); \
      })
#define VTEST_MSG_FATAL(fmt, ...) ({ \
      if (sGlobalStaticVideoProp.logMask & VTEST_FATAL) \
          ALOGE("VT_ERROR %s::%d " fmt, __FUNCTION__, __LINE__,  ##__VA_ARGS__); \
      })

#ifdef _ANDROID_LOG_DEBUG_MED
#define VTEST_MSG_MEDIUM(fmt, ...) ({ \
      if (sGlobalStaticVideoProp.logMask & VTEST_MEDIUM) \
          ALOGE("VT_MED %s::%d " fmt, __FUNCTION__, __LINE__,  ##__VA_ARGS__); \
      })
#else
#define VTEST_MSG_MEDIUM(fmt, ...)
#endif

#ifdef _ANDROID_LOG_DEBUG_LOW
#define VTEST_MSG_LOW(fmt, ...) ({ \
      if (sGlobalStaticVideoProp.logMask & VTEST_LOW) \
          ALOGE("VT_LOW %s::%d " fmt, __FUNCTION__, __LINE__,  ##__VA_ARGS__); \
      })
#else
#define VTEST_MSG_LOW(fmt, ...)
#endif

#else //#ifdef _ANDROID_LOG_

#define VTEST_MSG_HIGH(fmt, ...) fprintf(stderr, "VT_HIGH %s::%d " fmt"\n",__FUNCTION__, __LINE__, ## __VA_ARGS__)
#define VTEST_MSG_PROFILE(fmt, ...) fprintf(stderr, "VT_PROFILE %s::%d " fmt"\n",__FUNCTION__, __LINE__, ## __VA_ARGS__)
#define VTEST_MSG_ERROR(fmt, ...) fprintf(stderr, "VT_ERROR %s::%d " fmt"\n",__FUNCTION__, __LINE__, ##__VA_ARGS__)
#define VTEST_MSG_FATAL(fmt, ...) fprintf(stderr, "VT_ERROR %s::%d " fmt"\n",__FUNCTION__, __LINE__, ## __VA_ARGS__)

#ifdef _ANDROID_LOG_DEBUG_MED
#define VTEST_MSG_MEDIUM(fmt, ...) fprintf(stderr, "VT_MED %s::%d " fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)
#else
#define VTEST_MSG_MEDIUM(fmt, a, b, c)
#endif

#ifdef _ANDROID_LOG_DEBUG_LOW
#define VTEST_MSG_LOW(fmt, ...) fprintf(stderr, "VT_LOW %s::%d " fmt"\n",__FUNCTION__, __LINE__,## __VA_ARGS__)
#else
#define VTEST_MSG_LOW(fmt, a, b, c)
#endif

#endif //#ifdef _ANDROID_LOG_

#define VTEST_MSG_CONSOLE(fmt, ...) fprintf(stderr, "VT_CONSOLE %s::%d " fmt"\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define VTEST_SIMPLE_MSG_CONSOLE(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

#define FAILED(rv, fmt, ...)    \
    if(rv != OMX_ErrorNone) \
    {                           \
        VTEST_MSG_ERROR(fmt"", ## __VA_ARGS__); \
    }
#define FAILED0(rv, fmt, ...)  \
    if(rv != OMX_ErrorNone) \
    {                           \
        VTEST_MSG_ERROR(fmt"", ## __VA_ARGS__); \
        return;           \
    }
#define FAILED1(rv, fmt, ...)  \
    if(rv != OMX_ErrorNone) \
    {                           \
        VTEST_MSG_ERROR(fmt"", ## __VA_ARGS__); \
        return rv;           \
    }
#define FAILED2(rv, c, fmt, ...)  \
    if(rv != OMX_ErrorNone) \
    {                           \
        VTEST_MSG_ERROR(fmt"", ## __VA_ARGS__); \
        c; \
        return rv;         \
    }
#define FAILED_ALLOC0(rv, fmt, ...)  \
    if(rv == NULL) \
    {                           \
        VTEST_MSG_ERROR(fmt"", ## __VA_ARGS__); \
        return;           \
    }
#define FAILED_ALLOC1(rv, fmt, ...)  \
    if(rv == NULL) \
    {                           \
        VTEST_MSG_ERROR(fmt"", ## __VA_ARGS__); \
        return OMX_ErrorInsufficientResources;  \
    }

#endif // #ifndef _VTEST_DEBUG_H
