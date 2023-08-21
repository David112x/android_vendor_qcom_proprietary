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
#include "interfaces/sms/sms.h"
#include <interfaces/QcRilRequestMessage.h>

/* Request to acknowlege a IMS SMS MT message (including GSM and CDMA)
   @Receiver: SmsModule

   Response:
    errorCode    : Valid error codes
    responseData : nullptr
*/
class RilRequestAckImsSmsMessage : public QcRilRequestMessage,
                                   public add_message_id<RilRequestAckImsSmsMessage> {
public:
  static constexpr const char *MESSAGE_NAME = "RIL_REQUEST_IMS_SMS_ACKNOWLEDGE";

  RilRequestAckImsSmsMessage() = delete;
  ~RilRequestAckImsSmsMessage();

  inline RilRequestAckImsSmsMessage(std::shared_ptr<MessageContext> context, uint32_t messageRef,
                                    qcril::interfaces::DeliverStatus status)
      : QcRilRequestMessage(get_class_message_id(), context), mMessageRef(messageRef),
        mDeliverStatus(status) {
    mName = MESSAGE_NAME;
  }

  uint32_t getMessageRef();
  qcril::interfaces::DeliverStatus getDeliverStatus();

  string dump();

private:
  uint32_t mMessageRef;
  qcril::interfaces::DeliverStatus mDeliverStatus;
};
