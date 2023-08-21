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
 * Request to make a MO call
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : nullptr
 */
class QcRilRequestDialMessage : public QcRilRequestMessage {
 private:
  // Calling number/address
  std::optional<std::string> mAddress;
  std::optional<int> mClir;
  std::optional<std::shared_ptr<qcril::interfaces::UusInfo>> mUusInfo;
  std::optional<qcril::interfaces::CallType> mCallType;
  std::optional<qcril::interfaces::CallDomain> mCallDomain;
  // std::optional<Presentation> mPresentation;
  std::optional<bool> mIsConferenceUri;
  std::optional<bool> mIsCallPull;
  std::optional<bool> mIsEncrypted;
  std::optional<std::string> mDisplayText;
  std::optional<qcril::interfaces::RttMode> mRttMode;
  std::optional<std::string> mOriginatingNumber;
  std::optional<bool> mIsSecondary;
  std::optional<bool> mIsEmergency;
  std::optional<unsigned int> mCategories;
  std::vector<std::string> mUrns;
  std::optional<qcril::interfaces::EmergencyCallRouting> mRouting;
  std::optional<bool> mIsIntentionEcc;  /* For resolving ambiguity */
  std::optional<bool> mIsForEccTesting; /* For test purpose */
  std::optional<qcril::interfaces::CallFailCause>
      mRetryCallFailReason;                              /* Reason for which previous call failed */
  std::optional<RIL_RadioTechnology> mRetryCallFailMode; /* Mode on which previous call failed */
  std::optional<qcril::interfaces::CallComposerInfo> mCallComposerInfo;

 public:
  QcRilRequestDialMessage() = delete;

  ~QcRilRequestDialMessage() {
  }

  inline QcRilRequestDialMessage(message_id_t msg_id, std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(msg_id, context) {
  }

  bool hasAddress() {
    return mAddress ? true : false;
  }
  const std::string& getAddress() {
    return *mAddress;
  }
  void setAddress(const std::string& val) {
    mAddress = val;
  }

  bool hasClir() {
    return mClir ? true : false;
  }
  int getClir() {
    return *mClir;
  }
  void setClir(int val) {
    mClir = val;
  }

  bool hasUusInfo() {
    return mUusInfo ? true : false;
  }
  std::shared_ptr<qcril::interfaces::UusInfo> getUusInfo() {
    return *mUusInfo;
  }
  void setUusInfo(std::shared_ptr<qcril::interfaces::UusInfo> val) {
    mUusInfo = val;
  }

  bool hasCallType() {
    return mCallType ? true : false;
  }
  qcril::interfaces::CallType getCallType() {
    return *mCallType;
  }
  void setCallType(qcril::interfaces::CallType val) {
    mCallType = val;
  }

  bool hasCallDomain() {
    return mCallDomain ? true : false;
  }
  qcril::interfaces::CallDomain getCallDomain() {
    return *mCallDomain;
  }
  void setCallDomain(qcril::interfaces::CallDomain val) {
    mCallDomain = val;
  }

  bool hasIsConferenceUri() {
    return mIsConferenceUri ? true : false;
  }
  bool getIsConferenceUri() {
    return *mIsConferenceUri;
  }
  void setIsConferenceUri(bool val) {
    mIsConferenceUri = val;
  }

  bool hasIsCallPull() {
    return mIsCallPull ? true : false;
  }
  bool getIsCallPull() {
    return *mIsCallPull;
  }
  void setIsCallPull(bool val) {
    mIsCallPull = val;
  }

  bool hasIsEncrypted() {
    return mIsEncrypted ? true : false;
  }
  bool getIsEncrypted() {
    return *mIsEncrypted;
  }
  void setIsEncrypted(bool val) {
    mIsEncrypted = val;
  }

  bool hasDisplayText() {
    return mDisplayText ? true : false;
  }
  const std::string& getDisplayText() {
    return *mDisplayText;
  }
  void setDisplayText(const std::string& val) {
    mDisplayText = val;
  }

  bool hasRttMode() {
    return mRttMode ? true : false;
  }
  qcril::interfaces::RttMode getRttMode() {
    return *mRttMode;
  }
  void setRttMode(qcril::interfaces::RttMode val) {
    mRttMode = val;
  }

  bool hasIsSecondary() {
    return mIsSecondary ? true : false;
  }
  bool getIsSecondary() {
    return *mIsSecondary;
  }
  void setIsSecondary(bool val) {
    mIsSecondary = val;
  }

  bool hasOriginatingNumber() {
    return mOriginatingNumber ? true : false;
  }
  const std::string& getOriginatingNumber() {
    return *mOriginatingNumber;
  }
  void setOriginatingNumber(const std::string& val) {
    mOriginatingNumber = val;
  }

  bool hasIsEmergency() {
    return mIsEmergency ? true : false;
  }
  bool getIsEmergency() {
    return *mIsEmergency;
  }
  void setIsEmergency(bool val) {
    mIsEmergency = val;
  }

  bool hasIsForEccTesting() {
    return mIsForEccTesting ? true : false;
  }
  bool getIsForEccTesting() {
    return *mIsForEccTesting;
  }
  void setIsForEccTesting(bool val) {
    mIsForEccTesting = val;
  }

  bool hasIsIntentionEcc() {
    return mIsIntentionEcc ? true : false;
  }
  bool getIsIntentionEcc() {
    return *mIsIntentionEcc;
  }
  void setIsIntentionEcc(bool val) {
    mIsIntentionEcc = val;
  }

  bool hasCategories() {
    return mCategories ? true : false;
  }
  unsigned int getCategories() {
    return *mCategories;
  }
  void setCategories(unsigned int val) {
    mCategories = val;
  }

  bool hasRouting() {
    return mCategories ? true : false;
  }
  qcril::interfaces::EmergencyCallRouting getRouting() {
    return *mRouting;
  }
  void setRouting(qcril::interfaces::EmergencyCallRouting val) {
    mRouting = val;
  }

  bool hasRetryCallFailReason() {
    return mRetryCallFailReason ? true : false;
  }
  qcril::interfaces::CallFailCause getRetryCallFailReason() {
    return *mRetryCallFailReason;
  }
  void setRetryCallFailReason(qcril::interfaces::CallFailCause val) {
    mRetryCallFailReason = val;
  }

  bool hasRetryCallFailMode() {
    return mRetryCallFailMode ? true : false;
  }
  RIL_RadioTechnology getRetryCallFailMode() {
    return *mRetryCallFailMode;
  }
  void setRetryCallFailMode(RIL_RadioTechnology val) {
    mRetryCallFailMode = val;
  }

  bool hasCallComposerInfo() {
    return mCallComposerInfo.has_value();
  }
  qcril::interfaces::CallComposerInfo getCallComposerInfo() {
    return *mCallComposerInfo;
  }
  void setCallComposerInfo(qcril::interfaces::CallComposerInfo val) {
    mCallComposerInfo = val;
  }

  virtual std::string dump() {
    std::string os;
    os += QcRilRequestMessage::dump();
    os += "{";
    os += ".mAddress=" + (mAddress ? (*mAddress) : "<invalid>");
    os += ".mClir=" + (mClir ? std::to_string(*mClir) : "<invalid>");
    os += ".mCallType=" + (mCallType ? toString(*mCallType) : "<invalid>");
    os += ".mCallDomain=" + (mCallDomain ? toString(*mCallDomain) : "<invalid>");
    os += std::string(".mIsConferenceUri=") +
          (mIsConferenceUri ? (*mIsConferenceUri ? "true" : "false") : "<invalid>");
    os += std::string(".mIsCallPull=") +
          (mIsCallPull ? (*mIsCallPull ? "true" : "false") : "<invalid>");
    os += std::string(".mIsEncrypted=") +
          (mIsEncrypted ? (*mIsEncrypted ? "true" : "false") : "<invalid>");
    os += ".mRttMode=" + (mRttMode ? toString(*mRttMode) : "<invalid>");
    os += std::string(".mIsSecondary=") +
          (mIsSecondary ? (*mIsSecondary ? "true" : "false") : "<invalid>");
    os += std::string(".mOriginatingNumber=") +
          (mOriginatingNumber ? (*mOriginatingNumber) : "<invalid>");
    os += std::string(".mIsEmergency=") +
          (mIsEmergency ? (*mIsEmergency ? "true" : "false") : "<invalid>");
    os += std::string(".mIsForEccTesting=") +
          (mIsForEccTesting ? (*mIsForEccTesting ? "true" : "false") : "<invalid>");
    os += std::string(".mIsIntentionEcc=") +
          (mIsIntentionEcc ? (*mIsIntentionEcc ? "true" : "false") : "<invalid>");
    os += ".mCategories=" + (mCategories ? std::to_string(*mCategories) : "<invalid>");
    os += ".mRouting=" + (mRouting ? toString(*mRouting) : "<invalid>");
    os += ".mRetryCallFailReason=" +
          (mRetryCallFailReason ? toString(*mRetryCallFailReason) : "<invalid>");
    os += ".mRetryCallFailMode=" +
          (mRetryCallFailMode ? std::to_string(*mRetryCallFailMode) : "<invalid>");
    os += "}";
    return os;
  }
};
