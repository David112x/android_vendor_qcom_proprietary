// NOWHINE ENTIRE FILE
//-------------------------------------------------------------------------
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
//------------------------------------------------------------------------

#ifndef __COMMON_DEFS_H__
#define __COMMON_DEFS_H__

#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif // __cplusplus

#if defined(ANDROID)
#include <android/log.h> // Logcat logging functions
#endif // ANDROID



/* Return Values */
#define NC_LIB_SUCCESS                  (0)                             /**< Success return value */

#define NC_LIB_ERRORE_BASE              (0x04000000)
#define NC_LIB_GENERAL_ERROR            (NC_LIB_ERRORE_BASE + 1)        /**< General error return value */
#define NC_LIB_INVALID_INPUT            (NC_LIB_ERRORE_BASE + 2)        /**< Invalid input */

#define NC_LIB_WARN_INVALID_INPUT       (NC_LIB_ERRORE_BASE + 0x100)    /**< Warning: Invalid input, possible performance or IQ degradation*/

// PASS_TYPE is the power of 4 of the scale of the pass. 4^PASS_TYPE. example: 4^0 = 1. example: 4^3 = 64.
typedef enum
{
    PASS_TYPE_FULL = 0,
    PASS_TYPE_DC4  = 1,
    PASS_TYPE_DC16 = 2,
    PASS_TYPE_DC64 = 3,
    PASS_TYPE_NUM
} PASS_TYPE;


//typedef enum
//{
//  NCLIB_FALSE = 0,
//  NCLIB_TRUE = 1
//} NCLIB_BOOLEAN;

typedef bool NCLIB_BOOLEAN;
#define NCLIB_FALSE (false)
#define NCLIB_TRUE  (true)


// LOG_LEVEL_DEBUG -   print LOGD, LOGW and LOGE
// LOG_LEVEL_WARNING - print LOGW and LOGE
// LOG_LEVEL_ERROR -   print LOGE
#define LOG_LEVEL_DEBUG   (1)
#define LOG_LEVEL_WARNING (2)
#define LOG_LEVEL_ERROR   (3)
#define LOG_LEVEL_NONE    (4)

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_ERROR
#endif /*LOG_LEVEL */

#if defined(ANDROID)

#define NCLIB_LOG(level, levelString, fmt, ...) \
    { __android_log_print((level), "NcLib", levelString " NcLib %s(%u): " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
      printf(levelString "%s: " fmt "\n", __FUNCTION__, ##__VA_ARGS__); }

#else // ANDROID

#define ANDROID_LOG_DEBUG  LOG_LEVEL_DEBUG
#define ANDROID_LOG_WARN   LOG_LEVEL_WARNING
#define ANDROID_LOG_ERROR  LOG_LEVEL_ERROR
#define ANDROID_LOG_SILENT LOG_LEVEL_NONE
#define NCLIB_LOG(level, levelString, ...) \
    { printf(levelString "%s: ", __FUNCTION__ );printf(__VA_ARGS__);printf("\n"); }

#endif // ANDROID

// LOGS
#if (LOG_LEVEL <= LOG_LEVEL_DEBUG)
#define  LOGD(...)  NCLIB_LOG(ANDROID_LOG_DEBUG, "[DEBUG]   ", __VA_ARGS__)
#else
#define  LOGD(...)  {}
#endif
#if (LOG_LEVEL <= LOG_LEVEL_WARNING)
#define  LOGW(...)  NCLIB_LOG(ANDROID_LOG_WARN, "[WARNING] ", __VA_ARGS__)
#else
#define  LOGW(...)  {}
#endif
#if (LOG_LEVEL <= LOG_LEVEL_ERROR)
#define  LOGE(...)  NCLIB_LOG(ANDROID_LOG_ERROR, "[SERIOUS WARNING] ", __VA_ARGS__)
#else
#define  LOGE(...)  {}
#endif

#define NCLIB_DO_NOTHING (void)0
#if defined(ANDROID) || defined(__KLOCWORK__)
#define NCLIB_ASSERT_ALWAYS(c, doOnFail) { if (!(c)) { LOGE("Assertion failed: %s, file %s, line %d", #c, __FILE__, __LINE__ ); assert(false); doOnFail; } }
#define NCLIB_ASSERT_DEBUG assert
#else
#define NCLIB_ASSERT_ALWAYS(c, unused) { if (!(c)) { LOGE("Assertion failed: %s, file %s, line %d", #c, __FILE__, __LINE__ ); __debugbreak(); } }
#define NCLIB_ASSERT_DEBUG(c) NCLIB_ASSERT_ALWAYS(c, NCLIB_DO_NOTHING)
#endif

#define NCLIB_VERIFY_POINTER(c) { if (NULL == (c)) { LOGE("NULL pointer: %s", #c); return NC_LIB_INVALID_INPUT; } }
#define NCLIB_VERIFY_RET(ret, ...) { if (NC_LIB_SUCCESS != (ret)) { LOGE(__VA_ARGS__); return ret; } }


#ifdef NCLIB_CRASH_ON_RESTRICTION_VIOLATION
#define NCLIB_RESTRICTION_VIOLATION(msg) {LOGE("RESTRICTION_VIOLATION: replace invalid setting");LOGE(msg);NCLIB_ASSERT_DEBUG(false);}
#define NCLIB_RESTRICTION_VIOLATION_CONDITIONAL(condition, msg) if(false == (condition)){LOGE("RESTRICTION_VIOLATION: replace invalid setting");LOGE(msg);NCLIB_ASSERT_DEBUG(false);}
#else
#define NCLIB_RESTRICTION_VIOLATION(...) {LOGE("RESTRICTION_VIOLATION: replace invalid setting");LOGE(__VA_ARGS__);}
#define NCLIB_RESTRICTION_VIOLATION_CONDITIONAL(condition, ...) if(false == (condition)){LOGE("RESTRICTION_VIOLATION: replace invalid setting");LOGE(__VA_ARGS__)}
#endif

#define NCLIB_RESTRICTION_WARNING(msg)  {LOGW("RESTRICTION_WARNING: ");LOGW(msg);}


typedef uint32_t FIELD_UINT;
typedef uint64_t FIELD_UINT64;
typedef int32_t FIELD_INT;

typedef uint32_t PARAM_UINT;
typedef int32_t PARAM_INT;

typedef float PARAM_FLOAT;


///////////////////////////////////////////////////
// General utility macros

#define NCLIB_ARRAYSIZE(arr) (sizeof(arr)/sizeof(*(arr)))

#define COPY_ARRAY( destArrayName, srcArrayName ) \
    { \
        static_assert((sizeof(srcArrayName) == sizeof(destArrayName)), "Array sizes do not match!"); \
        memcpy(destArrayName, srcArrayName, sizeof(srcArrayName)); \
    }

#define NCLIB_ABS(a)               ( ( a > 0 ) ? (a) : (-a) )
#define NCLIB_MIN(a, b)            ( (a) < (b) ? (a) : (b) )
#define NCLIB_MAX(a, b)            ( (a) > (b) ? (a) : (b) )
#define NCLIB_TRIM(a, lo, hi)      ( (a) < (lo) ? (lo) : ( ((a) < (hi)) ? (a) : (hi) ) )
#define NCLIB_CEIL_DIV(a,b)        ( (( a ) + ( b ) - 1 ) / ( b ) )
#define NCLIB_LIMIT(retVal, checkedVal, min, max)\
{                                                \
    if ( checkedVal < min )                      \
    {                                            \
        retVal = min;                            \
    }                                            \
    else if (checkedVal > max)                   \
    {                                            \
        retVal = max;                            \
    }                                            \
    else                                         \
    {                                            \
        retVal = checkedVal;                     \
    }                                            \
}

#define NCLIB_LUT_LIMIT(retVal, checkedVal, lutSize ,min, max)\
{                                                             \
    uint32_t i;                                               \
    for (i = 0; i < lutSize; i++)                             \
    {                                                         \
        NCLIB_LIMIT(retVal[i], checkedVal[i], min, max)       \
    }                                                         \
}


#endif
