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
 * Sends a burst Dual-Tone Multifrequency (DTMF)
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : nullptr
 */
class QcRilRequestCdmaBurstDtmfMessage : public QcRilRequestMessage,
                                public add_message_id<QcRilRequestCdmaBurstDtmfMessage> {
 private:
  std::optional<std::string> mDigitBuffer;
  std::optional<uint32_t> mDtmfOnLength;
  std::optional<uint32_t> mDtmfOffLength;

 public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestCdmaBurstDtmfMessage";

  QcRilRequestCdmaBurstDtmfMessage() = delete;

  ~QcRilRequestCdmaBurstDtmfMessage() {}

  inline QcRilRequestCdmaBurstDtmfMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  bool hasDigitBuffer() { return mDigitBuffer ? true : false; }
  const std::string &getDigitBuffer() { return *mDigitBuffer; }
  void setDigitBuffer(const std::string &val) { mDigitBuffer = val; }

  bool hasDtmfOnLength() { return mDtmfOnLength ? true : false; }
  uint32_t getDtmfOnLength() { return *mDtmfOnLength; }
  void setDtmfOnLength(uint32_t val) { mDtmfOnLength = val; }

  bool hasDtmfOffLength() { return mDtmfOffLength ? true : false; }
  uint32_t getDtmfOffLength() { return *mDtmfOffLength; }
  void setDtmfOffLength(uint32_t val) { mDtmfOffLength = val; }

  virtual string dump() {
    std::string os;
    os += QcRilRequestMessage::dump();
    os += "{";
    os += ".mDigitBuffer=" + (mDigitBuffer ? *mDigitBuffer : "<invalid>");
    os += ".mDtmfOnLength=" + (mDtmfOnLength ? std::to_string(*mDtmfOnLength) : "<invalid>");
    os += ".mDtmfOffLength=" + (mDtmfOffLength ? std::to_string(*mDtmfOffLength) : "<invalid>");
    os += "}";
    return os;
  }
};
