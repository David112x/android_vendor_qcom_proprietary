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
 * Request to set the LTED category for each application
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : nullptr
 */
class QcRilRequestSetLtedCategoryMessage
    : public QcRilRequestMessage,
      public add_message_id<QcRilRequestSetLtedCategoryMessage> {
private:
  std::optional<std::string> mOsAppId;
  std::optional<qcril::interfaces::lte_direct::Category> mCategory;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestSetLtedCategoryMessage";

  QcRilRequestSetLtedCategoryMessage() = delete;

  ~QcRilRequestSetLtedCategoryMessage() {}

  inline QcRilRequestSetLtedCategoryMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  bool hasOsAppId() { return mOsAppId ? true : false; }
  const std::string &getOsAppId() { return *mOsAppId; }
  void setOsAppId(const std::string &val) { mOsAppId = val; }
  inline string dumpOsAppId() { return "mOsAppId = " + (mOsAppId ? *mOsAppId : "<invalid>"); }

  bool hasCategory() { return mCategory ? true : false; }
  qcril::interfaces::lte_direct::Category getCategory() { return *mCategory; }
  void setCategory(qcril::interfaces::lte_direct::Category val) { mCategory = val; }
  inline string dumpCategory() {
    return "mCategory = " + (mCategory ? toString(*mCategory) : "<invalid>");
  }

  virtual string dump() {
    return QcRilRequestMessage::dump() + "{" + dumpOsAppId() + dumpCategory() + "}";
  }
};
