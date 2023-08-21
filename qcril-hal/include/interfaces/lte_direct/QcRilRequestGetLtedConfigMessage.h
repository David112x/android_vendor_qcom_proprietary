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
 * Request to retrieve the LTED authorization/provisioning information
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : std::shared_ptr<qcril::interfaces::lte_direct::GetLtedConfigResp>
 */
class QcRilRequestGetLtedConfigMessage : public QcRilRequestMessage,
                                         public add_message_id<QcRilRequestGetLtedConfigMessage> {

public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestGetLtedConfigMessage";

  QcRilRequestGetLtedConfigMessage() = delete;

  ~QcRilRequestGetLtedConfigMessage() {}

  inline QcRilRequestGetLtedConfigMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  virtual string dump() { return QcRilRequestMessage::dump() + "{}"; }
};
