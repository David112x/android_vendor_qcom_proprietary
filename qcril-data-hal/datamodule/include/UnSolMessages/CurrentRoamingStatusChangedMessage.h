/**
* Copyright (c) 2020 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
**/

#pragma once
#include "framework/Message.h"
#include "framework/UnSolicitedMessage.h"
#include "framework/add_message_id.h"

namespace rildata {

/********************** Class Definitions *************************/
class CurrentRoamingStatusChangedMessage: public UnSolicitedMessage,
                           public add_message_id<CurrentRoamingStatusChangedMessage> {
private:
  bool mIsRoaming;

public:
  static constexpr const char *MESSAGE_NAME = "CurrentRoamingStatusChangedMessage";

  CurrentRoamingStatusChangedMessage(bool isRoaming):
    UnSolicitedMessage(get_class_message_id()), mIsRoaming(isRoaming) {}
  ~CurrentRoamingStatusChangedMessage() {}

  bool isRoaming() {
    return mIsRoaming;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    return std::make_shared<CurrentRoamingStatusChangedMessage>(mIsRoaming);
  }

  string dump() {
    return get_message_name() + ": " + (mIsRoaming ? "ON" : "OFF");
  }
};

} //namespace