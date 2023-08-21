/******************************************************************************
#  Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include "framework/GenericCallback.h"
#include "framework/SolicitedMessage.h"
#include "framework/Message.h"
#include "framework/add_message_id.h"
#include "framework/message_translator.h"
#include "framework/legacy.h"
#include "modules/android/ril_request_info.h"
#include "modules/sms/qcril_qmi_sms_types.h"
#include <interfaces/QcRilRequestMessage.h>

/* Request to set SMSC address Message
   @Receiver: SmsModule

   Response:
    errorCode    : Valid error codes
    responseData : nullptr
*/

class RilRequestSetSmscAddressMessage : public QcRilRequestMessage,
                                        public add_message_id<RilRequestSetSmscAddressMessage> {
public:
  static constexpr const char *MESSAGE_NAME = "RIL_REQUEST_SET_SMSC_ADDRESS";

  RilRequestSetSmscAddressMessage() = delete;
  ~RilRequestSetSmscAddressMessage();

  template <typename T>
  explicit RilRequestSetSmscAddressMessage(std::shared_ptr<MessageContext> context, T &&addr)
      : QcRilRequestMessage(get_class_message_id(), context), mSmscAddr(std::forward<T>(addr)) {
    mName = MESSAGE_NAME;
  }

  const string &getSmscAddr();

  string dump();

private:
  string mSmscAddr;
};
