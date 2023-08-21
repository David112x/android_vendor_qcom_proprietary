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
 * Unsol message to notifies match event details
 */
class QcRilUnsolMatchEventMessage : public UnSolicitedMessage,
                                    public add_message_id<QcRilUnsolMatchEventMessage> {
private:
  std::optional<std::string> mOsAppId;
  std::optional<std::string> mExpression;
  std::optional<std::string> mMatchedExpression;
  std::optional<uint32_t> mState;
  std::optional<uint32_t> mMetaDataIndex;
  std::optional<std::string> mMetaData;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolMatchEventMessage";

  ~QcRilUnsolMatchEventMessage() {}

  QcRilUnsolMatchEventMessage() : UnSolicitedMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolMatchEventMessage> msg =
        std::make_shared<QcRilUnsolMatchEventMessage>();
    return msg;
  }

  bool hasOsAppId() { return mOsAppId ? true : false; }
  const std::string &getOsAppId() { return *mOsAppId; }
  void setOsAppId(const std::string &val) { mOsAppId = val; }
  inline string dumpOsAppId() { return " mOsAppId = " + (mOsAppId ? *mOsAppId : "<invalid>"); }

  bool hasExpression() { return mExpression ? true : false; }
  const std::string &getExpression() { return *mExpression; }
  void setExpression(const std::string &val) { mExpression = val; }
  inline string dumpExpression() {
    return " mExpression = " + (mExpression ? *mExpression : "<invalid>");
  }

  bool hasMatchedExpression() { return mMatchedExpression ? true : false; }
  const std::string &getMatchedExpression() { return *mMatchedExpression; }
  void setMatchedExpression(const std::string &val) { mMatchedExpression = val; }
  inline string dumpMatchedExpression() {
    return " mMatchedExpression = " + (mMatchedExpression ? *mMatchedExpression : "<invalid>");
  }

  bool hasState() { return mState ? true : false; }
  uint32_t getState() { return *mState; }
  void setState(uint32_t val) { mState = val; }
  inline string dumpState() {
    return " mState = " + (mState ? std::to_string(*mState) : "<invalid>");
  }

  bool hasMetaDataIndex() { return mMetaDataIndex ? true : false; }
  uint32_t getMetaDataIndex() { return *mMetaDataIndex; }
  void setMetaDataIndex(uint32_t val) { mMetaDataIndex = val; }
  inline string dumpMetaDataIndex() {
    return " mMetaDataIndex = " + (mMetaDataIndex ? std::to_string(*mMetaDataIndex) : "<invalid>");
  }

  bool hasMetaData() { return mMetaData ? true : false; }
  const std::string &getMetaData() { return *mMetaData; }
  void setMetaData(const std::string &val) { mMetaData = val; }
  inline string dumpMetaData() { return " mMetaData = " + (mMetaData ? *mMetaData : "<invalid>"); }

  std::string dump() {
    return mName + "{" + dumpOsAppId() + dumpExpression() + dumpMatchedExpression() + dumpState() +
           dumpMetaDataIndex() + dumpMetaData() + "}";
  }
};
