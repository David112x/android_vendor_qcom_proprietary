/******************************************************************************
#  Copyright (c) 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include "framework/SolicitedSyncMessage.h"
#include "framework/Util.h"
#include "framework/add_message_id.h"
#include "framework/Dispatcher.h"

class RegisterForRoamingStatusChangedMessage : public SolicitedSyncMessage<int>,
                                    public add_message_id<RegisterForRoamingStatusChangedMessage>
{
private:
  bool mRegister;

public:
  static constexpr const char *MESSAGE_NAME = "RegisterForRoamingStatusChangedMessage";
  inline RegisterForRoamingStatusChangedMessage(GenericCallback<int> *) :
    SolicitedSyncMessage<int>(get_class_message_id())
  {
    mName = MESSAGE_NAME;
  }

  void setParams(const bool _register)
  {
    mRegister = _register;
  }

  bool getParams()
  {
    return mRegister;
  }
  ~RegisterForRoamingStatusChangedMessage() {}

  string dump() {
    return mName;
  }
};