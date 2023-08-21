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
 * Request to hangup a call
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : nullptr
 */
class QcRilRequestHangupMessage : public QcRilRequestMessage {
 private:
  bool mRetryAttempted;

  std::optional<uint32_t> mCallIndex;
  std::optional<bool> mIsMultiParty;
  std::optional<std::string> mConnectionUri;
  std::optional<uint32_t> mConferenceId;
  std::optional<qcril::interfaces::CallFailCause> mRejectCause;
  std::optional<uint32_t> mRejectCauseRaw;

 public:
  QcRilRequestHangupMessage() = delete;

  ~QcRilRequestHangupMessage() {}

  inline QcRilRequestHangupMessage(message_id_t msg_id, std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(msg_id, context), mRetryAttempted(false) {}

  bool hasCallIndex() { return mCallIndex ? true : false; }
  uint32_t getCallIndex() { return *mCallIndex; }
  void setCallIndex(uint32_t val) { mCallIndex = val; }

  bool hasIsMultiParty() { return mIsMultiParty ? true : false; }
  bool getIsMultiParty() { return *mIsMultiParty; }
  void setIsMultiParty(bool val) { mIsMultiParty = val; }

  bool hasConnectionUri() { return mConnectionUri ? true : false; }
  const std::string &getConnectionUri() { return *mConnectionUri; }
  void setConnectionUri(const std::string &val) { mConnectionUri = val; }

  bool hasConferenceId() { return mConferenceId ? true : false; }
  uint32_t getConferenceId() { return *mConferenceId; }
  void setConferenceId(uint32_t val) { mConferenceId = val; }

  bool hasRejectCause() { return mRejectCause ? true : false; }
  qcril::interfaces::CallFailCause getRejectCause() { return *mRejectCause; }
  void setRejectCause(qcril::interfaces::CallFailCause val) { mRejectCause = val; }

  bool hasRejectCauseRaw() { return mRejectCauseRaw ? true : false; }
  uint32_t getRejectCauseRaw() { return *mRejectCauseRaw; }
  void setRejectCauseRaw(uint32_t val) { mRejectCauseRaw = val; }

  virtual std::string dump() {
    std::string os;
    os += QcRilRequestMessage::dump();
    os += "{";
    os += ".mCallIndex=" + (mCallIndex ? std::to_string(*mCallIndex) : "<invalid>");
    os += std::string(".mIsMultiParty=") +
          (mIsMultiParty ? (*mIsMultiParty ? "true" : "false") : "<invalid>");
    os += ".mConnectionUri=" + (mConnectionUri ? (*mConnectionUri) : "<invalid>");
    os += ".mConferenceId=" + (mConferenceId ? std::to_string(*mConferenceId) : "<invalid>");
    os += ".mRejectCause=" + (mRejectCause ? toString(*mRejectCause) : "<invalid>");
    os += ".mRejectCauseRaw=" + (mRejectCauseRaw ? std::to_string(*mRejectCauseRaw) : "<invalid>");
    os += "}";
    return os;
  }

  // For Internal use
  bool isHangupRetryAttempted() { return mRetryAttempted; }
  void setHangupRetryAttempted() { mRetryAttempted = true; }
};
