/******************************************************************************
#  Copyright (c) 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#ifdef RIL_FOR_LOW_RAM
#include "framework/SolicitedMessage.h"
#include "framework/add_message_id.h"
#include "MessageCommon.h"
#include "telephony/ril.h"


namespace rildata {

class StopLCERequestMessage : public SolicitedMessage<RIL_LceStatusInfo>,
                              public add_message_id<StopLCERequestMessage>
{
private:
  int32_t mSerial;
public:
  static constexpr const char* MESSAGE_NAME = "StopLCERequestMessage";
  StopLCERequestMessage(
    const int32_t serial
  ):SolicitedMessage<RIL_LceStatusInfo>(get_class_message_id()) {

    mName = MESSAGE_NAME;
    mSerial = serial;
  }
  ~StopLCERequestMessage() = default;
  string dump(){
    return mName;
  }
  int32_t getSerial() {
    return mSerial;
  }
};
}
#endif