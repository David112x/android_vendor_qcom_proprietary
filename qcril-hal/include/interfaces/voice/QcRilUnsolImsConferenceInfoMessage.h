/******************************************************************************
#  Copyright (c) 2018 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include <framework/UnSolicitedMessage.h>
#include <framework/add_message_id.h>
#include <interfaces/voice/voice.h>
#include <optional>

/*
 * Unsol message to notify clients about conference information
 */
class QcRilUnsolImsConferenceInfoMessage
    : public UnSolicitedMessage,
      public add_message_id<QcRilUnsolImsConferenceInfoMessage> {
 private:
  std::optional<qcril::interfaces::ConferenceCallState> mConferenceCallState;
  std::optional<std::vector<uint8_t>> mConferenceInfoUri;

 public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolImsConferenceInfoMessage";
  ~QcRilUnsolImsConferenceInfoMessage() {}

  QcRilUnsolImsConferenceInfoMessage() : UnSolicitedMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolImsConferenceInfoMessage> msg =
        std::make_shared<QcRilUnsolImsConferenceInfoMessage>();
    if (msg) {
      if (mConferenceCallState) {
        msg->setConferenceCallState(*mConferenceCallState);
      }
      if (mConferenceInfoUri) {
        msg->setConferenceInfoUri(*mConferenceInfoUri);
      }
    }
    return msg;
  }

  bool hasConferenceCallState() { return mConferenceCallState ? true : false; }
  qcril::interfaces::ConferenceCallState getConferenceCallState() { return *mConferenceCallState; }
  void setConferenceCallState(qcril::interfaces::ConferenceCallState val) {
    mConferenceCallState = val;
  }

  bool hasConferenceInfoUri() { return mConferenceInfoUri ? true : false; }
  const std::vector<uint8_t> &getConferenceInfoUri() { return *mConferenceInfoUri; }
  void setConferenceInfoUri(const std::vector<uint8_t> &val) { mConferenceInfoUri = val; }

  std::string dump() {
    std::string os;
    os += mName;
    os += "{";
    os += ".mConferenceCallState=" +
          (mConferenceCallState ? toString(*mConferenceCallState) : "<invalid>");
    os += "}";
    return os;
  }
};
