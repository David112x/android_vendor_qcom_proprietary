/******************************************************************************
#  Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include <framework/UnSolicitedMessage.h>
#include <framework/add_message_id.h>
#include <interfaces/sms/sms.h>
#include <optional>

/*
 * Unsol message to notify clients about conference information
 */
class QcRilUnsolMessageWaitingInfoMessage
    : public UnSolicitedMessage,
      public add_message_id<QcRilUnsolMessageWaitingInfoMessage> {
 private:
  std::optional<std::vector<qcril::interfaces::MessageSummary>> mMessageSummary;
  std::optional<std::string> mUeAddress;
  std::optional<std::vector<qcril::interfaces::MessageDetails>> mMessageDetails;

 public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolMessageWaitingInfoMessage";
  ~QcRilUnsolMessageWaitingInfoMessage() {}

  QcRilUnsolMessageWaitingInfoMessage() : UnSolicitedMessage(get_class_message_id()) {}

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolMessageWaitingInfoMessage> msg =
        std::make_shared<QcRilUnsolMessageWaitingInfoMessage>();
    return msg;
  }

  bool hasMessageSummary() { return mMessageSummary ? true : false; }
  const std::vector<qcril::interfaces::MessageSummary> &getMessageSummary() { return *mMessageSummary; }
  void setMessageSummary(const std::vector<qcril::interfaces::MessageSummary> &val) {
    mMessageSummary = val;
  }

  bool hasUeAddress() { return mUeAddress ? true : false; }
  const std::string &getUeAddress() { return *mUeAddress; }
  void setUeAddress(const std::string &val) { mUeAddress = val; }

  bool hasMessageDetails() { return mMessageDetails ? true : false; }
  const std::vector<qcril::interfaces::MessageDetails> &getMessageDetails() { return *mMessageDetails; }
  void setMessageDetails(const std::vector<qcril::interfaces::MessageDetails> &val) {
    mMessageDetails = val;
  }

  string dump() {
    std::string os;
    os += mName;
    os += "{";
    os += ".mUeAddress = " + (mUeAddress ? (*mUeAddress) : "<invalid>");
    os += "}";
    return os;
  }
};
