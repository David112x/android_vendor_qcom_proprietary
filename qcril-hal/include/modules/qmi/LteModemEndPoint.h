/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once

#include "framework/Log.h"
#include "modules/qmi/LteModemEndPointModule.h"
#include "modules/qmi/ModemEndPoint.h"

class LteModemEndPoint : public ModemEndPoint {
public:
  static constexpr const char *NAME = "LTE";

  LteModemEndPoint() : ModemEndPoint(NAME) {
    mModule = new LteModemEndPointModule("LteModemEndPointModule", *this);
    mModule->init();
  }

  ~LteModemEndPoint() {
    Log::getInstance().d("[LteModemEndPoint]: destructor");
    // mModule->killLooper();
    delete mModule;
    mModule = nullptr;
  }

  void requestSetup(string clientToken, GenericCallback<string> *cb);
};
