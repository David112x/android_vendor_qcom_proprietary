/******************************************************************************
#  Copyright (c) 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include "framework/ModuleLooper.h"

/**
 * A ThreadPoolAgent is a Looper which acts as an interface to the ThreadPoolManager2.
 */
class ThreadPoolAgent : public ModuleLooper {
 public:
  ThreadPoolAgent() : ModuleLooper() {
  }
  virtual void dispatch(std::shared_ptr<Message> msg) override;
};
