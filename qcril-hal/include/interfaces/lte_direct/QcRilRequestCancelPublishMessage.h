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
 * Request to stop the publishing an Expression for OsAppId
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : nullptr
 */
class QcRilRequestCancelPublishMessage : public QcRilRequestMessage,
                                         public add_message_id<QcRilRequestCancelPublishMessage> {
private:
  std::optional<std::string> mOsAppId;
  std::optional<std::string> mExpression;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestCancelPublishMessage";

  QcRilRequestCancelPublishMessage() = delete;

  ~QcRilRequestCancelPublishMessage() {}

  inline QcRilRequestCancelPublishMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  bool hasOsAppId() { return mOsAppId ? true : false; }
  const std::string &getOsAppId() { return *mOsAppId; }
  void setOsAppId(const std::string &val) { mOsAppId = val; }
  inline string dumpOsAppId() { return "mOsAppId = " + (mOsAppId ? *mOsAppId : "<invalid>"); }

  bool hasExpression() { return mExpression ? true : false; }
  const std::string &getExpression() { return *mExpression; }
  void setExpression(const std::string &val) { mExpression = val; }
  inline string dumpExpression() {
    return "mExpression = " + (mExpression ? *mExpression : "<invalid>");
  }

  virtual string dump() {
    return QcRilRequestMessage::dump() + "{" + dumpOsAppId() + dumpExpression() + "}";
  }
};
