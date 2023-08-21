/******************************************************************************
#  Copyright (c) 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include <framework/UnSolicitedMessage.h>
#include <framework/add_message_id.h>
#include <interfaces/ims/ims.h>

/*
 * Unsol message to notify change in sub config
 *
 */
class QcRilUnsolImsCallComposerInfo : public UnSolicitedMessage,
                                     public add_message_id<QcRilUnsolImsCallComposerInfo> {
 private:
  qcril::interfaces::CallComposerInfo mInfo;

 public:
  static constexpr const char *MESSAGE_NAME = "QcRilUnsolImsCallComposerInfo";
  ~QcRilUnsolImsCallComposerInfo() {}

  QcRilUnsolImsCallComposerInfo(const qcril::interfaces::CallComposerInfo& info)
      : UnSolicitedMessage(get_class_message_id()), mInfo(info) {}

  std::shared_ptr<UnSolicitedMessage> clone() {
    auto msg = std::make_shared<QcRilUnsolImsCallComposerInfo>(mInfo);
    return msg;
  }

  const qcril::interfaces::CallComposerInfo& getComposerInfo() { return mInfo; }

  string dump() {
    return QcRilUnsolImsCallComposerInfo::MESSAGE_NAME;
  }
};
