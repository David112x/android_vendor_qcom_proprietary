/******************************************************************************
#  Copyright (c) 2018 Qualcomm Technologies, Inc.
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

/* Request to get SMSC address Message
   @Receiver: SmsModule

   Response:
    errorCode    : Valid error codes
    responseData : std::shared_ptr<RilGetSmscAddrResult_t>
*/

class RilRequestGetSmscAddressMessage : public QcRilRequestMessage,
                                        public add_message_id<RilRequestGetSmscAddressMessage> {
public:
  static constexpr const char *MESSAGE_NAME = "RIL_REQUEST_GET_SMSC_ADDRESS";

  ~RilRequestGetSmscAddressMessage();

  inline RilRequestGetSmscAddressMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  string dump();
};
