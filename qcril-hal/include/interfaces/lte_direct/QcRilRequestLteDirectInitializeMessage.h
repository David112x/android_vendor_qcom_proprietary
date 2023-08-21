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
 * Request to initialize Lte Direct discovery service
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : nullptr
 */
class QcRilRequestLteDirectInitializeMessage
    : public QcRilRequestMessage,
      public add_message_id<QcRilRequestLteDirectInitializeMessage> {
public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestLteDirectInitializeMessage";

  QcRilRequestLteDirectInitializeMessage() = delete;

  ~QcRilRequestLteDirectInitializeMessage() {}

  inline QcRilRequestLteDirectInitializeMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  virtual string dump() { return QcRilRequestMessage::dump() + "{}"; }
};
