////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXHAFCUSTOM_H
#define CAMXHAFCUSTOM_H

#include "camxdefs.h"
#include "chistatsalgo.h"
#include "camxdebug.h"
#include "camxdebugprint.h"
#include "chiafinterface.h"
#include "chihafalgorithminterface.h"
#include "camxmem.h"

// Logging function pointer for custom HAF algorithm
extern StatsLoggingFunction gStatsLogger;
#define HAF_MSG_ERROR(fmt, ...)      gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAF,StatsLogError,fmt, ##__VA_ARGS__)
#define HAF_MSG_VERBOSE(fmt, ...)    gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAF,StatsLogVerbose,fmt, ##__VA_ARGS__)
#define HAF_MSG_INFO(fmt, ...)       gStatsLogger(CamX::Log::GetFileName(__FILE__),__LINE__,__FUNCTION__,StatsLogGroupAF,StatsLogInfo,fmt, ##__VA_ARGS__)

#define ABS(x) (x > 0 ? x : -x)

VOID* GetParamFromCreateList(
    const AFAlgoCreateParamList* pCreateParams,
    AFAlgoCreateParamType paramType);

VOID* GetParamFromMonitorInputList(
    const HAFAlgoMonitorInputParamList*   pMonitorInputParamList,
    HAFAlgoMonitorInputParamType paramType);

VOID* GetParamFromSearchInputList(
    HAFAlgoSearchInputParamList*    pSearchInputParamList,
    HAFAlgoSearchInputParamType     paramType);

#endif
