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
 * Unsol message to notify the geo location date retrieval status
 */
class QcRilUnsolImsGeoLocationDataStatus
    : public UnSolicitedMessage,
      public add_message_id<QcRilUnsolImsGeoLocationDataStatus> {
 private:
  std::optional<qcril::interfaces::GeoLocationDataStatus> mGpsStatus;

 public:
  static constexpr const char* MESSAGE_NAME = "QcRilUnsolImsGeoLocationDataStatus";
  ~QcRilUnsolImsGeoLocationDataStatus() {
  }

  QcRilUnsolImsGeoLocationDataStatus() : UnSolicitedMessage(get_class_message_id()) {
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    std::shared_ptr<QcRilUnsolImsGeoLocationDataStatus> msg =
        std::make_shared<QcRilUnsolImsGeoLocationDataStatus>();
    return msg;
  }

  bool hasGeoLocationDataStatus() {
    return mGpsStatus.has_value();
  }
  qcril::interfaces::GeoLocationDataStatus getGeoLocationDataStatus() {
    return *mGpsStatus;
  }
  void setGeoLocationDataStatus(qcril::interfaces::GeoLocationDataStatus val) {
    mGpsStatus = val;
  }

  virtual string dump() {
    std::string os;
    os += QcRilUnsolImsGeoLocationDataStatus::MESSAGE_NAME;
    os += "{";
    os += ".mGpsStatus=";
    os += qcril::interfaces::toString(*mGpsStatus);
    os += "}";
    return os;
  }
};
