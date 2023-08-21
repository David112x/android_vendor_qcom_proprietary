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

/* Request to set GSM/WCDMA cell broadcast SMS config.
   @Receiver: SmsModule

   Response:
    errorCode    : Valid error codes
    responseData : nullptr
*/
class RilRequestGsmSetBroadcastSmsConfigMessage
    : public QcRilRequestMessage,
      public add_message_id<RilRequestGsmSetBroadcastSmsConfigMessage> {
public:
  static constexpr const char *MESSAGE_NAME = "RIL_REQUEST_GSM_SET_BROADCAST_SMS_CONFIG";
  RilRequestGsmSetBroadcastSmsConfigMessage() = delete;
  ~RilRequestGsmSetBroadcastSmsConfigMessage();

  template <typename T>
  inline RilRequestGsmSetBroadcastSmsConfigMessage(std::shared_ptr<MessageContext> context,
                                                   T &&configList)
      : QcRilRequestMessage(get_class_message_id(), context),
        mConfigList(std::forward<T>(configList)) {
    mName = MESSAGE_NAME;
  }

  const std::vector<RIL_GSM_BroadcastSmsConfigInfo> &getConfigList();

  string dump();

private:
  std::vector<RIL_GSM_BroadcastSmsConfigInfo> mConfigList;
};
