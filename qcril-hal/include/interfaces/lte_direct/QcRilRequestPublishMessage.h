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
 * Request to specify the expression that the App wishes to publish
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : nullptr
 */
class QcRilRequestPublishMessage : public QcRilRequestMessage,
                                   public add_message_id<QcRilRequestPublishMessage> {
private:
  std::optional<std::string> mOsAppId;
  std::optional<std::string> mExpression;
  std::optional<uint32_t> mExpressionValidityTime;
  std::optional<std::string> mMetaData;
  std::optional<qcril::interfaces::lte_direct::DiscoveryType> mDiscoveryType;
  std::optional<uint32_t> mDuration;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestPublishMessage";

  QcRilRequestPublishMessage() = delete;

  ~QcRilRequestPublishMessage() {}

  inline QcRilRequestPublishMessage(std::shared_ptr<MessageContext> context)
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

  bool hasExpressionValidityTime() { return mExpressionValidityTime ? true : false; }
  uint32_t getExpressionValidityTime() { return *mExpressionValidityTime; }
  void setExpressionValidityTime(uint32_t val) { mExpressionValidityTime = val; }
  inline string dumpExpressionValidityTime() {
    return "mExpressionValidityTime = " +
           (mExpressionValidityTime ? std::to_string(*mExpressionValidityTime) : "<invalid>");
  }

  bool hasMetaData() { return mMetaData ? true : false; }
  const std::string &getMetaData() { return *mMetaData; }
  void setMetaData(const std::string &val) { mMetaData = val; }
  inline string dumpMetaData() { return "mMetaData = " + (mMetaData ? *mMetaData : "<invalid>"); }

  bool hasDiscoveryType() { return mDiscoveryType ? true : false; }
  qcril::interfaces::lte_direct::DiscoveryType getDiscoveryType() { return *mDiscoveryType; }
  void setDiscoveryType(qcril::interfaces::lte_direct::DiscoveryType val) { mDiscoveryType = val; }
  inline string dumpDiscoveryType() {
    return "mDiscoveryType = " + (mDiscoveryType ? toString(*mDiscoveryType) : "<invalid>");
  }

  bool hasDuration() { return mDuration ? true : false; }
  uint32_t getDuration() { return *mDuration; }
  void setDuration(uint32_t val) { mDuration = val; }
  inline string dumpDuration() {
    return "mDuration = " + (mDuration ? std::to_string(*mDuration) : "<invalid>");
  }

  virtual string dump() {
    return QcRilRequestMessage::dump() + "{" + dumpOsAppId() + dumpExpression() +
           dumpExpressionValidityTime() + dumpDiscoveryType() + dumpDuration() + "}";
  }
};
