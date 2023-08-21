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
 * Unsol message to notifies the reception status
 */
class QcRilUnsolReceptionStatusMessage : public UnSolicitedMessage,
                                         public add_message_id<QcRilUnsolReceptionStatusMessage> {
private:
  std::optional<std::string> mOsAppId;
  std::optional<std::string> mExpression;
  std::optional<uint32_t> mStatus;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolReceptionStatusMessage";

  ~QcRilUnsolReceptionStatusMessage() {}

  QcRilUnsolReceptionStatusMessage() : UnSolicitedMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolReceptionStatusMessage> msg =
        std::make_shared<QcRilUnsolReceptionStatusMessage>();
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

  bool hasStatus() { return mStatus ? true : false; }
  uint32_t getStatus() { return *mStatus; }
  void setStatus(uint32_t val) { mStatus = val; }
  inline string dumpStatus() {
    return " mStatus = " + (mStatus ? std::to_string(*mStatus) : "<invalid>");
  }

  std::string dump() { return mName + "{" + dumpOsAppId() + dumpExpression() + dumpStatus() + "}"; }
};
