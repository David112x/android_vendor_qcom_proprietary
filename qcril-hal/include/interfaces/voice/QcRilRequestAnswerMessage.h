/******************************************************************************
#  Copyright (c) 2018 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include <framework/add_message_id.h>
#include <interfaces/QcRilRequestMessage.h>
#include <interfaces/voice/voice.h>
#include <optional>

/*
 * Request to answer an incoming call
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : nullptr
 */
class QcRilRequestAnswerMessage : public QcRilRequestMessage {
 private:
  std::optional<qcril::interfaces::CallType> mCallType;
  std::optional<qcril::interfaces::Presentation> mPresentation;
  std::optional<qcril::interfaces::RttMode> mRttMode;

 public:
  QcRilRequestAnswerMessage() = delete;

  ~QcRilRequestAnswerMessage() {
  }

  inline QcRilRequestAnswerMessage(message_id_t msg_id, std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(msg_id, context) {
  }

  bool hasCallType() {
    return mCallType ? true : false;
  }
  const qcril::interfaces::CallType& getCallType() {
    return *mCallType;
  }
  void setCallType(const qcril::interfaces::CallType& val) {
    mCallType = val;
  }

  bool hasPresentation() {
    return mPresentation ? true : false;
  }
  const qcril::interfaces::Presentation& getPresentation() {
    return *mPresentation;
  }
  void setPresentation(qcril::interfaces::Presentation val) {
    mPresentation = val;
  }

  bool hasRttMode() {
    return mRttMode ? true : false;
  }
  const qcril::interfaces::RttMode& getRttMode() {
    return *mRttMode;
  }
  void setRttMode(const qcril::interfaces::RttMode& val) {
    mRttMode = val;
  }

  virtual string dump() {
    std::string os;
    os += QcRilRequestMessage::dump();
    os += "{";
    os += ".mCallType=" + (mCallType ? toString(*mCallType) : "<invalid>");
    os += ".mPresentation=" + (mPresentation ? toString(*mPresentation) : "<invalid>");
    os += ".mRttMode=" + (mRttMode ? toString(*mRttMode) : "<invalid>");
    os += "}";
    return os;
  }
};
