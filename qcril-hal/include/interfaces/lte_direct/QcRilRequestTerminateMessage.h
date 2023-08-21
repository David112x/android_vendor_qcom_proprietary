/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include <framework/add_message_id.h>
#include <interfaces/QcRilRequestMessage.h>
#include <interfaces/voice/voice.h>
#include <optional>

/*
 * Request to stop all ongoing Lte Direct Discovery operations
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : nullptr
 */
class QcRilRequestTerminateMessage : public QcRilRequestMessage,
                                     public add_message_id<QcRilRequestTerminateMessage> {
private:
  std::optional<std::string> mOsAppId;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestTerminateMessage";

  QcRilRequestTerminateMessage() = delete;

  ~QcRilRequestTerminateMessage() {}

  inline QcRilRequestTerminateMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  bool hasOsAppId() { return mOsAppId ? true : false; }
  const std::string &getOsAppId() { return *mOsAppId; }
  void setOsAppId(const std::string &val) { mOsAppId = val; }
  inline string dumpOsAppId() { return "mOsAppId = " + (mOsAppId ? *mOsAppId : "<invalid>"); }

  virtual string dump() { return QcRilRequestMessage::dump() + "{" + dumpOsAppId() + "}"; }
};
