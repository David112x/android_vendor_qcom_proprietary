/******************************************************************************
#  Copyright (c) 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#include "framework/ThreadPoolAgent.h"
#include "framework/Module.h"
#include "framework/ThreadPoolManager2.h"
#include <framework/Log.h>

#define TAG "RILQ"

void ThreadPoolAgent::dispatch(std::shared_ptr<Message> msg) {
  ThreadPoolManager2& ThreadPoolManager2 = ThreadPoolManager2::getThreadPoolManager();
  ThreadPoolManager2.schedule(std::make_shared<ThreadPoolTask>(
      mModule, msg, [this](std::shared_ptr<Message> msg) { mModule->handleMessage(msg); }));
}
