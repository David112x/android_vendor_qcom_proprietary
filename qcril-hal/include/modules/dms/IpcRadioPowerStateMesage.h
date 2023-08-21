/*
 * Copyright (c) 2020 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#pragma once

#include <qtibus/IPCMessage.h>

class IpcRadioPowerStateMesage
    : public IPCMessage,
      public add_message_id<IpcRadioPowerStateMesage> {
public:
  static constexpr const char *MESSAGE_NAME = "IpcRadioPowerStateMesage";

private:
  bool mIsGenuineSignal; // TODO use bool?

public:
  IpcRadioPowerStateMesage() : IPCMessage(get_class_message_id()) {
    mName = MESSAGE_NAME;
  }

  IpcRadioPowerStateMesage(bool isGenuineSignal)
      : IPCMessage(get_class_message_id()), mIsGenuineSignal(isGenuineSignal) {
    mName = MESSAGE_NAME;
  }

  inline ~IpcRadioPowerStateMesage() {}

  std::shared_ptr<UnSolicitedMessage> clone() {
    auto ret = std::make_shared<IpcRadioPowerStateMesage>();
    if (ret) {
      ret->mIsGenuineSignal = mIsGenuineSignal;
      ret->setIsRemote(getIsRemote());
    }
    return std::make_shared<IpcRadioPowerStateMesage>();
  }

  void serialize(IPCOStream &os) {
    os << mIsGenuineSignal;
  }

  void deserialize(IPCIStream &is) {
    is >> mIsGenuineSignal;
  }

  bool getIsGenuineSignal() { return mIsGenuineSignal; }

  string dump() {
    std::string os = mName;
    os += "{";
    os += " .mIsGenuineSignal=";
    os += (mIsGenuineSignal ? "true" : "false");
    os += " .isRemote=";
    os += (getIsRemote() ? "true" : "false");
    os += "}";
    return os;
  }
};
