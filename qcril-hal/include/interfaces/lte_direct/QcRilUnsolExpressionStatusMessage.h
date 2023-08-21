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
 * Unsol message to notifies the status of the Expressions
 */
class QcRilUnsolExpressionStatusMessage : public UnSolicitedMessage,
                                          public add_message_id<QcRilUnsolExpressionStatusMessage> {
private:
  std::optional<std::string> mOsAppId;
  std::optional<std::string> mExpression;
  std::optional<qcril::interfaces::lte_direct::Result> mResult;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolExpressionStatusMessage";

  ~QcRilUnsolExpressionStatusMessage() {}

  QcRilUnsolExpressionStatusMessage() : UnSolicitedMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolExpressionStatusMessage> msg =
        std::make_shared<QcRilUnsolExpressionStatusMessage>();
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

  bool hasResult() { return mResult ? true : false; }
  qcril::interfaces::lte_direct::Result getResult() { return *mResult; }
  void setResult(qcril::interfaces::lte_direct::Result val) { mResult = val; }
  inline string dumpResult() {
    return " mResult = " + (mResult ? toString(*mResult) : "<invalid>");
  }

  std::string dump() { return mName + "{" + dumpOsAppId() + dumpExpression() + dumpResult() + "}"; }
};
