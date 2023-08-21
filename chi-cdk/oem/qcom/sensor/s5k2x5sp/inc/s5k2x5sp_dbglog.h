////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  s5k2x5sp_dbglog.h
/// @brief The header that defines method to print debug log for s5k2x5sp sensor lib.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <stdio.h>

#ifdef _DEBUG_
#ifdef _ANDROID_
#include <android/log.h>
#include <cutils/log.h>
#undef  S5K2X5SP_SENSOR_LOG_TAG
#define S5K2X5SP_SENSOR_LOG_TAG "s5k2x5sp"
#ifndef SENSOR_LOG_INFO
#define SENSOR_LOG_INFO(...) ((void)ALOG(LOG_INFO, S5K2X5SP_SENSOR_LOG_TAG, __VA_ARGS__))
#endif
#ifndef ALOG
#define ALOG(priority, tag, ...)    \
    LOG_PRI(ANDROID_##priority, tag, __VA_ARGS__)
#endif
#ifndef LOG_PRI
#define LOG_PRI(priority, tag, ...)    \
    __android_log_print(priority, tag, __VA_ARGS__)
#endif
#else
#define SENSOR_LOG_INFO(msg, ...)    \
    printf("<%s:%d> : " msg, __func__, __LINE__, ## __VA_ARGS__)
#endif
#else
#define SENSOR_LOG_INFO(msg, ...)  ((void*)(0))
#endif

