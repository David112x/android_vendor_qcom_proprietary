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
 * Unsol message to notifies the transmission status
 */
class QcRilUnsolTransmissionStatusMessage
    : public UnSolicitedMessage,
      public add_message_id<QcRilUnsolTransmissionStatusMessage> {
private:
  std::optional<std::string> mOsAppId;
  std::optional<std::string> mExpression;
  std::optional<uint32_t> mStatus;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolTransmissionStatusMessage";

  ~QcRilUnsolTransmissionStatusMessage() {}

  QcRilUnsolTransmissionStatusMessage() : UnSolicitedMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolTransmissionStatusMessage> msg =
        std::make_shared<QcRilUnsolTransmissionStatusMessage>();
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
