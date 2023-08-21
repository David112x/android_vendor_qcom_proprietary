/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include <framework/add_message_id.h>
#include <interfaces/QcRilRequestMessage.h>
#include <interfaces/lte_direct/lte_direct.h>
#include <optional>

/*
 * Request to retrieve the LTED category per OSAppID
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : std::shared_ptr<qcril::interfaces::lte_direct::GetLtedCategoryResp>
 */
class QcRilRequestGetLtedCategoryMessage
    : public QcRilRequestMessage,
      public add_message_id<QcRilRequestGetLtedCategoryMessage> {
private:
  std::optional<std::string> mOsAppId;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestGetLtedCategoryMessage";

  QcRilRequestGetLtedCategoryMessage() = delete;

  ~QcRilRequestGetLtedCategoryMessage() {}

  inline QcRilRequestGetLtedCategoryMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  bool hasOsAppId() { return mOsAppId ? true : false; }
  const std::string &getOsAppId() { return *mOsAppId; }
  void setOsAppId(const std::string &val) { mOsAppId = val; }
  inline string dumpOsAppId() { return "mOsAppId = " + (mOsAppId ? *mOsAppId : "<invalid>"); }

  virtual string dump() { return QcRilRequestMessage::dump() + "{" + dumpOsAppId() + "}"; }
};
