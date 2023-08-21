/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include <framework/UnSolicitedMessage.h>
#include <framework/add_message_id.h>
#include <interfaces/lte_direct/lte_direct.h>

/*
 * Unsol message to notifies the Lte Direct Discovery service status
 */
class QcRilUnsolServiceStatusMessage : public UnSolicitedMessage,
                                       public add_message_id<QcRilUnsolServiceStatusMessage> {
private:
  std::optional<uint32_t> mPublishAllowed;
  std::optional<uint32_t> mSubscribeAllowed;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolServiceStatusMessage";

  ~QcRilUnsolServiceStatusMessage() {}

  QcRilUnsolServiceStatusMessage() : UnSolicitedMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolServiceStatusMessage> msg =
        std::make_shared<QcRilUnsolServiceStatusMessage>();
    return msg;
  }

  bool hasPublishAllowed() { return mPublishAllowed ? true : false; }
  uint32_t getPublishAllowed() { return *mPublishAllowed; }
  void setPublishAllowed(uint32_t val) { mPublishAllowed = val; }
  inline string dumpPublishAllowed() {
    return " mPublishAllowed = " +
           (mPublishAllowed ? std::to_string(*mPublishAllowed) : "<invalid>");
  }

  bool hasSubscribeAllowed() { return mSubscribeAllowed ? true : false; }
  uint32_t getSubscribeAllowed() { return *mSubscribeAllowed; }
  void setSubscribeAllowed(uint32_t val) { mSubscribeAllowed = val; }
  inline string dumpSubscribeAllowed() {
    return " mSubscribeAllowed = " +
           (mSubscribeAllowed ? std::to_string(*mSubscribeAllowed) : "<invalid>");
  }

  std::string dump() { return mName + "{" + dumpPublishAllowed() + dumpSubscribeAllowed() + "}"; }
};
