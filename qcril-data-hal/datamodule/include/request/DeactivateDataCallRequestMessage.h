/*===========================================================================

  Copyright (c) 2018-2020 Qualcomm Technologies, Inc.
  All Rights Reserved.
  Confidential and Proprietary - Qualcomm Technologies, Inc.

===========================================================================*/

#pragma once

#include "MessageCommon.h"

using namespace std;

namespace rildata {

#ifndef RIL_FOR_LOW_RAM
class DeactivateDataCallRadioResponseIndMessage : public UnSolicitedMessage,
                          public add_message_id<DeactivateDataCallRadioResponseIndMessage> {
private:
  ResponseError_t response;
  int32_t serial;
  Message::Callback::Status status;

public:
  static constexpr const char *MESSAGE_NAME = "DeactivateDataCallRadioResponseIndMessage";

  DeactivateDataCallRadioResponseIndMessage() = delete;
  ~DeactivateDataCallRadioResponseIndMessage() = default;
  DeactivateDataCallRadioResponseIndMessage(ResponseError_t setResponse, int32_t setSerial, Message::Callback::Status setStatus):
    UnSolicitedMessage(get_class_message_id()), response(setResponse), serial(setSerial), status(setStatus) {}

  string dump(){return mName;}

  std::shared_ptr<UnSolicitedMessage> clone() {
    return std::make_shared<DeactivateDataCallRadioResponseIndMessage>(response, serial, status);
  };

  ResponseError_t getResponse() { return response; }
  int32_t getSerial() { return serial; }
  Message::Callback::Status getStatus() { return status; }
};
#else
class DeactivateDataCallRadioResponseIndMessage : public UnSolicitedMessage {
protected:
  ResponseError_t response;
  int32_t serial;
  Message::Callback::Status status;

public:
  static constexpr const char *MESSAGE_NAME = "DeactivateDataCallRadioResponseIndMessage";

  DeactivateDataCallRadioResponseIndMessage() = delete;
  ~DeactivateDataCallRadioResponseIndMessage() = default;
  DeactivateDataCallRadioResponseIndMessage(message_id_t id, ResponseError_t setResponse, int32_t setSerial, Message::Callback::Status setStatus):
    UnSolicitedMessage(id), response(setResponse), serial(setSerial), status(setStatus) {}

  string dump(){return mName;}

  ResponseError_t getResponse() { return response; }
  int32_t getSerial() { return serial; }
  Message::Callback::Status getStatus() { return status; }
};

class DeactivateDataCallRadioResponseIndMessage_1_4 : public DeactivateDataCallRadioResponseIndMessage,
                          public add_message_id<DeactivateDataCallRadioResponseIndMessage_1_4> {
public:
  static constexpr const char *MESSAGE_NAME = "DeactivateDataCallRadioResponseIndMessage_1_4";
  DeactivateDataCallRadioResponseIndMessage_1_4(ResponseError_t setResponse, int32_t setSerial, Message::Callback::Status setStatus):
      DeactivateDataCallRadioResponseIndMessage(get_class_message_id(), setResponse, setSerial, setStatus) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    return std::make_shared<DeactivateDataCallRadioResponseIndMessage_1_4>(response, serial, status);
  }
};

class DeactivateDataCallRadioResponseIndMessage_1_0 : public DeactivateDataCallRadioResponseIndMessage,
                          public add_message_id<DeactivateDataCallRadioResponseIndMessage_1_0> {
public:
  static constexpr const char *MESSAGE_NAME = "DeactivateDataCallRadioResponseIndMessage_1_0";
  DeactivateDataCallRadioResponseIndMessage_1_0(ResponseError_t setResponse, int32_t setSerial, Message::Callback::Status setStatus):
      DeactivateDataCallRadioResponseIndMessage(get_class_message_id(), setResponse, setSerial, setStatus) {
    mName = MESSAGE_NAME;
  }

  std::shared_ptr<UnSolicitedMessage> clone() {
    return std::make_shared<DeactivateDataCallRadioResponseIndMessage_1_0>(response, serial, status);
  }
};
#endif

class DeactivateDataCallIWlanResponseIndMessage : public UnSolicitedMessage,
                          public add_message_id<DeactivateDataCallIWlanResponseIndMessage> {
private:
  ResponseError_t response;
  int32_t serial;
  Message::Callback::Status status;

public:
  static constexpr const char *MESSAGE_NAME = "DeactivateDataCallIWlanResponseIndMessage";

  DeactivateDataCallIWlanResponseIndMessage() = delete;
  ~DeactivateDataCallIWlanResponseIndMessage() = default;
  DeactivateDataCallIWlanResponseIndMessage(ResponseError_t setResponse, int32_t setSerial, Message::Callback::Status setStatus):
    UnSolicitedMessage(get_class_message_id()), response(setResponse), serial(setSerial), status(setStatus) {}

  string dump(){return mName;}

  std::shared_ptr<UnSolicitedMessage> clone() {
    return std::make_shared<DeactivateDataCallIWlanResponseIndMessage>(response, serial, status);
  };

  ResponseError_t getResponse() { return response; }
  int32_t getSerial() { return serial; }
  Message::Callback::Status getStatus() { return status; }
};

class DeactivateDataCallRequestMessage : public SolicitedMessage<ResponseError_t>,
                          public add_message_id<DeactivateDataCallRequestMessage> {
  private:
    int32_t mSerial;
    int32_t mCid;
    DataRequestReason_t mReason;
    shared_ptr<function<void(int32_t)>> mAcknowlegeRequestCb;

  public:
    static constexpr const char *MESSAGE_NAME = "com.qualcomm.qti.qcril.data.DeactivateDataCallRequestMessage";
    DeactivateDataCallRequestMessage() = delete;
    DeactivateDataCallRequestMessage(
      const int32_t serial,
      const int32_t cid,
      const DataRequestReason_t reason,
      const shared_ptr<function<void(int32_t)>> ackCb
      ):SolicitedMessage<ResponseError_t>(get_class_message_id()) {

      mName = MESSAGE_NAME;
      mSerial = serial;
      mCid = cid;
      mReason = reason;
      mAcknowlegeRequestCb = ackCb;
    }
    ~DeactivateDataCallRequestMessage() = default;

    string dump(){return mName;}
    int32_t getSerial() {return mSerial;}
    int32_t getCid() {return mCid;}
    DataRequestReason_t getDataRequestReason() {return mReason;}
    shared_ptr<function<void(int32_t)>> getAcknowlegeRequestCb() {return mAcknowlegeRequestCb;}
};

}
