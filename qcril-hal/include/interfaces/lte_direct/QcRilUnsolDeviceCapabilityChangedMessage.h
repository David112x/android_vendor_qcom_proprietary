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
 * Unsol message to notifies the device LTE-D capability of all SUBs.
 */
class QcRilUnsolDeviceCapabilityChangedMessage
    : public UnSolicitedMessage,
      public add_message_id<QcRilUnsolDeviceCapabilityChangedMessage> {
private:
  std::optional<uint32_t> mCapability;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolDeviceCapabilityChangedMessage";

  ~QcRilUnsolDeviceCapabilityChangedMessage() {}

  QcRilUnsolDeviceCapabilityChangedMessage() : UnSolicitedMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolDeviceCapabilityChangedMessage> msg =
        std::make_shared<QcRilUnsolDeviceCapabilityChangedMessage>();
    return msg;
  }

  bool hasCapability() { return mCapability ? true : false; }
  uint32_t getCapability() { return *mCapability; }
  void setCapability(uint32_t val) { mCapability = val; }
  inline string dumpCapability() {
    return " mCapability = " + (mCapability ? std::to_string(*mCapability) : "<invalid>");
  }

  std::string dump() { return mName + "{" + dumpCapability() + "}"; }
};
