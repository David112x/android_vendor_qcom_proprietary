/**
* Copyright (c) 2019 Qualcomm Technologies, Inc.
* All Rights Reserved.
* Confidential and Proprietary - Qualcomm Technologies, Inc.
**/

#pragma once
#include "framework/Message.h"
#include "framework/UnSolicitedMessage.h"
#include "framework/add_message_id.h"
#include "MessageCommon.h"

namespace rildata {

/********************** Class Definitions *************************/
class UiInfoFragmentMessage: public UnSolicitedMessage,
                           public add_message_id<UiInfoFragmentMessage> {
  std::optional<NrIconEnum_t> mIcon;
public:
  static constexpr const char *MESSAGE_NAME = "UiInfoFragmentMessage";

  UiInfoFragmentMessage(): UnSolicitedMessage(get_class_message_id()) { mName = MESSAGE_NAME; }
  ~UiInfoFragmentMessage() {}

  bool hasIcon() { return mIcon ? true : false; }
  NrIconEnum_t getIcon() { return *mIcon; }
  void setIcon(NrIconEnum_t icon) { mIcon = icon; }

  std::shared_ptr<UnSolicitedMessage> clone() {
    auto msg = std::make_shared<UiInfoFragmentMessage>();
    if (msg != nullptr && hasIcon()) {
      msg->setIcon(getIcon());
    }
    return msg;
  }
  string dump() {
    return std::string(MESSAGE_NAME);
  }
};

} //namespace

