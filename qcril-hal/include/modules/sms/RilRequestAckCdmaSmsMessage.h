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

/* Request to acknowlege a CDMA SMS MT message
   @Receiver: SmsModule

   Response:
    errorCode    : Valid error codes
    responseData : nullptr
*/

class RilRequestAckCdmaSmsMessage : public QcRilRequestMessage,
                                    public add_message_id<RilRequestAckCdmaSmsMessage> {
public:
  static constexpr const char *MESSAGE_NAME = "RIL_REQUEST_CDMA_SMS_ACKNOWLEDGE";

  RilRequestAckCdmaSmsMessage() = delete;
  ~RilRequestAckCdmaSmsMessage();

  inline RilRequestAckCdmaSmsMessage(std::shared_ptr<MessageContext> context, bool success,
                                     int32_t cause)
      : QcRilRequestMessage(get_class_message_id(), context), mSuccess(success), mCause(cause) {
    mName = MESSAGE_NAME;
  }

  bool isSuccess();
  int32_t getCause();

  string dump();

private:
  bool mSuccess;
  int32_t mCause;
};
