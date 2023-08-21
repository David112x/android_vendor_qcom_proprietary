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

class PullLCEDataRequestMessage : public SolicitedMessage<RIL_LceDataInfo>,
                              public add_message_id<PullLCEDataRequestMessage>
{
private:
  int32_t mSerial;
public:
  static constexpr const char* MESSAGE_NAME = "PullLCEDataRequestMessage";
  PullLCEDataRequestMessage(
        const int32_t serial):SolicitedMessage<RIL_LceDataInfo>(get_class_message_id()) {
    mName = MESSAGE_NAME;
    mSerial = serial;
  }
  ~PullLCEDataRequestMessage() = default;
  string dump() {
    return mName;
  }
  int32_t getSerial() {
    return mSerial;
  }
};
}
#endif

