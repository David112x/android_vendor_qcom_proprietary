/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/
#pragma once
#include <framework/add_message_id.h>
#include <interfaces/QcRilRequestMessage.h>
#include <interfaces/lte_direct/lte_direct.h>
#include <optional>

/*
 * Request to provide the PLMNs authorized or provisoned by OMA-DM for LTE-D activity
 *
 * Response:
 *   errorCode    : Valid error codes
 *   responseData : nullptr
 */
class QcRilRequestSetLtedConfigMessage : public QcRilRequestMessage,
                                         public add_message_id<QcRilRequestSetLtedConfigMessage> {
private:
  std::optional<qcril::interfaces::lte_direct::Value128> mOsId;
  std::optional<std::string> mApn;
  std::optional<std::vector<qcril::interfaces::lte_direct::AnnouncingPolicy>> mAnnouncingPolicy;
  std::optional<std::vector<qcril::interfaces::lte_direct::MonitoringPolicy>> mMonitoringPolicy;
  std::optional<std::vector<uint8_t>> mBakPassword;

public:
  static constexpr const char *MESSAGE_NAME = "QcRilRequestSetLtedConfigMessage";

  QcRilRequestSetLtedConfigMessage() = delete;

  ~QcRilRequestSetLtedConfigMessage() {}

  inline QcRilRequestSetLtedConfigMessage(std::shared_ptr<MessageContext> context)
      : QcRilRequestMessage(get_class_message_id(), context) {
    mName = MESSAGE_NAME;
  }

  bool hasOsId() { return mOsId ? true : false; }
  qcril::interfaces::lte_direct::Value128 getOsId() { return *mOsId; }
  void setOsId(qcril::interfaces::lte_direct::Value128 val) { mOsId = val; }
  inline string dumpOsId() { return "mOsId = " + (mOsId ? toString(*mOsId) : "<invalid>"); }

  bool hasApn() { return mApn ? true : false; }
  const std::string &getApn() { return *mApn; }
  void setApn(const std::string &val) { mApn = val; }
  inline string dumpApn() { return "mApn = " + (mApn ? *mApn : "<invalid>"); }

  bool hasAnnouncingPolicy() { return mAnnouncingPolicy ? true : false; }
  const std::vector<qcril::interfaces::lte_direct::AnnouncingPolicy> &getAnnouncingPolicy() {
    return *mAnnouncingPolicy;
  }
  void setAnnouncingPolicy(const std::vector<qcril::interfaces::lte_direct::AnnouncingPolicy> &val) {
    mAnnouncingPolicy = val;
  }
  inline string dumpAnnouncingPolicy() {
    return "mAnnouncingPolicy(count:" +
           (mAnnouncingPolicy ? std::to_string((*mAnnouncingPolicy).size()) : 0) + ")";
  }

  bool hasMonitoringPolicy() { return mMonitoringPolicy ? true : false; }
  const std::vector<qcril::interfaces::lte_direct::MonitoringPolicy> &getMonitoringPolicy() {
    return *mMonitoringPolicy;
  }
  void setMonitoringPolicy(const std::vector<qcril::interfaces::lte_direct::MonitoringPolicy> &val) {
    mMonitoringPolicy = val;
  }
  inline string dumpMonitoringPolicy() {
    return "mMonitoringPolicy(count:" +
           (mMonitoringPolicy ? std::to_string((*mMonitoringPolicy).size()) : 0) + ")";
  }

  bool hasBakPassword() { return mBakPassword ? true : false; }
  const std::vector<uint8_t> &getBakPassword() { return *mBakPassword; }
  void setBakPassword(const std::vector<uint8_t> &val) { mBakPassword = val; }
  inline string dumpBakPassword() {
    return "mBakPassword(count:" + (mBakPassword ? std::to_string((*mBakPassword).size()) : 0) +
           ")";
  }

  virtual string dump() {
    return QcRilRequestMessage::dump() + "{" + dumpOsId() + dumpApn() + dumpAnnouncingPolicy() +
           dumpMonitoringPolicy() + "}";
  }
};
