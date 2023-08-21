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

class StartLCERequestMessage : public SolicitedMessage<RIL_LceStatusInfo>,
                              public add_message_id<StartLCERequestMessage>
{
private:
  int32_t mSerial;
public:
  int32_t mInterval;
  int32_t mMode;
  static constexpr const char* MESSAGE_NAME = "StartLCERequestMessage";
  StartLCERequestMessage( const int32_t serial,int32_t interval,int32_t mode
  ):SolicitedMessage<RIL_LceStatusInfo>(get_class_message_id()), mInterval(interval), mMode(mode) {
    mName = MESSAGE_NAME;
    mSerial = serial;
  }
  ~StartLCERequestMessage() = default;
  string dump(){
    return mName;
  }
  int32_t getSerial() {
    return mSerial;
  }
  int32_t getLCEinterval() {
    return mInterval;
  }
  int32_t getLCEmode() {
    return mMode;
  }
};
}
#endif
