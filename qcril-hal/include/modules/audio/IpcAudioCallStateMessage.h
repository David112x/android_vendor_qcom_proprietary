/*
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include "qcril_am.h"
#include <qtibus/IPCMessage.h>

class IpcAudioCallStateMessage
    : public IPCMessage,
      public add_message_id<IpcAudioCallStateMessage> {
public:
  static constexpr const char *MESSAGE_NAME = "IpcAudioCallStateMessage";

private:
  qcril_am_inter_rild_msg_type mIpcData;

public:
  IpcAudioCallStateMessage() : IPCMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  IpcAudioCallStateMessage(const qcril_am_inter_rild_msg_type &ipcData)
      : IPCMessage(get_class_message_id()), mIpcData(ipcData) {
    mName = MESSAGE_NAME;
  }

  inline ~IpcAudioCallStateMessage() {
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    auto ret = std::make_shared<IpcAudioCallStateMessage>();
    if (ret) {
      ret->mIpcData = mIpcData;
      ret->setIsRemote(getIsRemote());
    }
    return std::make_shared<IpcAudioCallStateMessage>();
  }

  void serialize(IPCOStream &os) {
    os << mIpcData.rild_id;
    uint8_t temp = static_cast<uint8_t>(mIpcData.type);
    os << temp;
    os << mIpcData.param.voice_vsid;
    temp = static_cast<uint8_t>(mIpcData.param.call_state);
    os << temp;
    temp = static_cast<uint8_t>(mIpcData.param.call_mode);
    os << temp;
  }

  void deserialize(IPCIStream &is) {
    uint8_t temp;
    is >> mIpcData.rild_id;
    is >> temp;
    mIpcData.type = static_cast<qcril_am_inter_rild_event_type>(temp);
    is >> mIpcData.param.voice_vsid;
    is >> temp;
    mIpcData.param.call_state = static_cast<qcril_am_call_state_type>(temp);
    is >> temp;
    mIpcData.param.call_mode = static_cast<call_mode_enum_v02>(temp);
  }

  qcril_am_inter_rild_msg_type &getIpcData() {
    return mIpcData;
  }

  string dump() {
    std::string os = mName;
    os += "{";
    os += " .isRemote=";
    os += (getIsRemote() ? "true" : "false");
    os += "}";
    return os;
  }
};
