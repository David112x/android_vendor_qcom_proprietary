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
 * Unsol message to notifies that the BAK password (PSK) in provisioning req has expired and is no
 * longer valid
 */
class QcRilUnsolPskExpirtedMessage : public UnSolicitedMessage,
                                     public add_message_id<QcRilUnsolPskExpirtedMessage> {
private:
public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolPskExpirtedMessage";

  ~QcRilUnsolPskExpirtedMessage() {}

  QcRilUnsolPskExpirtedMessage() : UnSolicitedMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolPskExpirtedMessage> msg =
        std::make_shared<QcRilUnsolPskExpirtedMessage>();
    return msg;
  }

  std::string dump() { return mName + "{" + "}"; }
};
