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

/* Request to send a CDMA SMS message.
   @Receiver: SmsModule

   Response:
     errorCode     : Valid error codes
     responseData  : std::shared_ptr<RilSendSmsResult_t>
*/

class RilRequestCdmaSendSmsMessage : public QcRilRequestMessage,
                                     public add_message_id<RilRequestCdmaSendSmsMessage>
{
  public:
    static constexpr const char *MESSAGE_NAME = "RIL_REQUEST_CDMA_SEND_SMS";

  RilRequestCdmaSendSmsMessage() = delete;
  ~RilRequestCdmaSendSmsMessage();

  inline RilRequestCdmaSendSmsMessage(std::shared_ptr<MessageContext> context,
                                      const RIL_CDMA_SMS_Message &cdmaSms)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
    mCdmaSms = cdmaSms; // structure shallow copy
  }

  const RIL_CDMA_SMS_Message &getCdmaSms();

  string dump();

private:
  RIL_CDMA_SMS_Message mCdmaSms;
};
