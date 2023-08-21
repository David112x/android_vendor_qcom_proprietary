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
 * Request to get Lte Direct Discovery Service Status
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : std::shared_ptr<qcril::interfaces::lte_direct::ServiceStatus>
 */
class QcRilRequestGetServiceStatusMessage
    : public QcRilRequestMessage,
      public add_message_id<QcRilRequestGetServiceStatusMessage> {

public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestGetServiceStatusMessage";

  QcRilRequestGetServiceStatusMessage() = delete;

  ~QcRilRequestGetServiceStatusMessage() {}

  inline QcRilRequestGetServiceStatusMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  virtual string dump() { return QcRilRequestMessage::dump() + "{}"; }
};
