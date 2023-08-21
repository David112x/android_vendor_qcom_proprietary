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
 * Unsol message to notify App authorization failure from LTE-D server
 */
class QcRilUnsolAuthorizationResultMessage
    : public UnSolicitedMessage,
      public add_message_id<QcRilUnsolAuthorizationResultMessage> {
private:
  std::optional<std::string> mOsAppId;
  std::optional<qcril::interfaces::lte_direct::Result> mResult;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolAuthorizationResultMessage";

  ~QcRilUnsolAuthorizationResultMessage() {}

  QcRilUnsolAuthorizationResultMessage() : UnSolicitedMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolAuthorizationResultMessage> msg =
        std::make_shared<QcRilUnsolAuthorizationResultMessage>();
    return msg;
  }

  bool hasOsAppId() { return mOsAppId ? true : false; }
  const std::string &getOsAppId() { return *mOsAppId; }
  void setOsAppId(const std::string &val) { mOsAppId = val; }
  inline string dumpOsAppId() { return " mOsAppId = " + (mOsAppId ? *mOsAppId : "<invalid>"); }

  bool hasResult() { return mResult ? true : false; }
  qcril::interfaces::lte_direct::Result getResult() { return *mResult; }
  void setResult(qcril::interfaces::lte_direct::Result val) { mResult = val; }
  inline string dumpResult() {
    return " mResult = " + (mResult ? toString(*mResult) : "<invalid>");
  }

  std::string dump() { return mName + "{" + dumpOsAppId() + dumpResult() + "}"; }
};
