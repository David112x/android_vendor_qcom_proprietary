/******************************************************************************
#  Copyright (c) 2018 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include <framework/add_message_id.h>
#include <interfaces/QcRilRequestMessage.h>
#include <interfaces/ims/ims.h>

/*
 * Request to send geo loc info to lower layers
 *
 * Response:
 *   errorCode    : Valid error codes
 *                   SUCCESS
 *                   GENERIC_FAILURE
 */

class QcRilRequestImsGeoLocationInfoMessage
    : public QcRilRequestMessage,
      public add_message_id<QcRilRequestImsGeoLocationInfoMessage> {
private:
  std::optional<double> mLatitude;
  std::optional<double> mLongitude;
  std::optional<std::string> mCountryCode;
  std::optional<std::string> mCountry;
  std::optional<std::string> mState;
  std::optional<std::string> mCity;
  std::optional<std::string> mPostalCode;
  std::optional<std::string> mStreet;
  std::optional<std::string> mHouseNumber;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestImsGeoLocationInfoMessage";

  QcRilRequestImsGeoLocationInfoMessage() = delete;

  ~QcRilRequestImsGeoLocationInfoMessage() {};

  inline QcRilRequestImsGeoLocationInfoMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  bool hasLatitude() { return mLatitude ? true : false; }
  double getLatitude() { return *mLatitude; }
  void setLatitude(double val) { mLatitude = val; }
  inline string dumpLatitude() {
    std::ostringstream strs;
    if (mLatitude) {
      strs << *mLatitude;
    } else {
      strs << "<invalid>";
    }
    return " mLatitude = " + strs.str();
  }
  bool hasLongitude() { return mLongitude ? true : false; }
  double getLongitude() { return *mLongitude; }
  void setLongitude(double val) { mLongitude = val; }
  inline string dumpLongitude() {
    std::ostringstream strs;
    if (mLongitude) {
      strs << *mLongitude;
    } else {
      strs << "<invalid>";
    }
    return " mLongitude = " + strs.str();
  }

  bool hasCountryCode() { return mCountryCode ? true : false; }
  const std::string &getCountryCode() { return *mCountryCode; }
  void setCountryCode(const std::string &val) { mCountryCode = val; }
  inline string dumpCountryCode() {
    return " mCountryCode = " + (mCountryCode ? (*mCountryCode) : "<invalid>");
  }

  bool hasCountry() { return mCountry ? true : false; }
  const std::string &getCountry() { return *mCountry; }
  void setCountry(const std::string &val) { mCountry = val; }
  inline string dumpCountry() {
    return " mCountry = " + (mCountry ? (*mCountry) : "<invalid>");
  }

  bool hasState() { return mState ? true : false; }
  const std::string &getState() { return *mState; }
  void setState(const std::string &val) { mState = val; }
  inline string dumpState() {
    return " mState = " + (mState ? (*mState) : "<invalid>");
  }

  bool hasCity() { return mCity ? true : false; }
  const std::string &getCity() { return *mCity; }
  void setCity(const std::string &val) { mCity = val; }
  inline string dumpCity() {
    return " mCity = " + (mCity ? (*mCity) : "<invalid>");
  }


  bool hasPostalCode() { return mPostalCode ? true : false; }
  const std::string &getPostalCode() { return *mPostalCode; }
  void setPostalCode(const std::string &val) { mPostalCode = val; }
  inline string dumpPostalCode() {
    return " mPostalCode = " + (mPostalCode ? (*mPostalCode) : "<invalid>");
  }

  bool hasStreet() { return mStreet ? true : false; }
  const std::string &getStreet() { return *mStreet; }
  void setStreet(const std::string &val) { mStreet = val; }
  inline string dumpStreet() {
    return " mStreet = " + (mStreet ? (*mStreet) : "<invalid>");
  }

  bool hasHouseNumber() { return mHouseNumber ? true : false; }
  const std::string &getHouseNumber() { return *mHouseNumber; }
  void setHouseNumber(const std::string &val) { mHouseNumber = val; }
  inline string dumpHouseNumber() {
    return " mHouseNumber = " + (mHouseNumber ? (*mHouseNumber) : "<invalid>");
  }

  virtual string dump() {
    return QcRilRequestMessage::dump() +
      "{" + dumpLatitude() + dumpLongitude() + dumpCountryCode() + dumpCountry() + dumpState() +
            dumpCity() + dumpPostalCode() + dumpStreet() + dumpHouseNumber() + "}";
  }
};
