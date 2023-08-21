/******************************************************************************
#  Copyright (c) 2019 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
#******************************************************************************/

#pragma once

#include "interfaces/common.h"
#include <string>
#include <vector>

namespace qcril {
namespace interfaces {
namespace lte_direct {

enum class DiscoveryType {
  UNKNOWN,
  OPEN,
  RESTRICTED,
};
inline std::string toString(const DiscoveryType &o) {
  if (o == DiscoveryType::UNKNOWN) {
    return "UNKNOWN";
  }
  if (o == DiscoveryType::OPEN) {
    return "OPEN";
  }
  if (o == DiscoveryType::RESTRICTED) {
    return "RESTRICTED";
  }
  return "<invalid>";
}

enum class Result {
  UNKNOWN,
  SUCCESS,
  GENERIC_FAILURE,
  IN_PROGRESS,
  INVALID_EXPRESSION_SCOPE,
  UNKNOWN_EXPRESSION,
  INVALID_DISCOVERY_TYPE,
  SERVICE_NOT_AVAILABLE,
  APP_AUTH_FAILURE,
  NOT_SUPPORTED,
};

inline std::string toString(const Result &o) {
  if (o == Result::UNKNOWN) {
    return "UNKNOWN";
  }
  if (o == Result::SUCCESS) {
    return "SUCCESS";
  }
  if (o == Result::GENERIC_FAILURE) {
    return "GENERIC_FAILURE";
  }
  if (o == Result::IN_PROGRESS) {
    return "IN_PROGRESS";
  }
  if (o == Result::INVALID_EXPRESSION_SCOPE) {
    return "INVALID_EXPRESSION_SCOPE";
  }
  if (o == Result::UNKNOWN_EXPRESSION) {
    return "UNKNOWN_EXPRESSION";
  }
  if (o == Result::INVALID_DISCOVERY_TYPE) {
    return "INVALID_DISCOVERY_TYPE";
  }
  if (o == Result::SERVICE_NOT_AVAILABLE) {
    return "SERVICE_NOT_AVAILABLE";
  }
  if (o == Result::APP_AUTH_FAILURE) {
    return "APP_AUTH_FAILURE";
  }
  if (o == Result::NOT_SUPPORTED) {
    return "NOT_SUPPORTED";
  }
  return "<invalid>";
}

enum class Category {
  UNKNOWN,
  HIGH,
  MEDIUM,
  LOW,
  VERY_LOW,
  INVALID,
};
inline std::string toString(const Category &o) {
  if (o == Category::UNKNOWN) {
    return "UNKNOWN";
  }
  if (o == Category::HIGH) {
    return "HIGH";
  }
  if (o == Category::MEDIUM) {
    return "MEDIUM";
  }
  if (o == Category::LOW) {
    return "LOW";
  }
  if (o == Category::VERY_LOW) {
    return "VERY_LOW";
  }
  if (o == Category::INVALID) {
    return "INVALID";
  }
  return "<invalid>";
}

enum class Range {
  UNKNOWN,
  SHORT,
  MEDIUM,
  LONG,
  RESERVED,
  INVALID,
};
inline std::string toString(const Range &o) {
  if (o == Range::UNKNOWN) {
    return "UNKNOWN";
  }
  if (o == Range::SHORT) {
    return "SHORT";
  }
  if (o == Range::MEDIUM) {
    return "MEDIUM";
  }
  if (o == Range::LONG) {
    return "LONG";
  }
  if (o == Range::RESERVED) {
    return "RESERVED";
  }
  if (o == Range::INVALID) {
    return "INVALID";
  }
  return "<invalid>";
}

class Plmn {
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(std::string, Mcc);
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(std::string, Mnc);

public:
  Plmn() {
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(Mcc);
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(Mnc);
  }
  Plmn(const Plmn &from) {
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, Mcc);
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, Mnc);
  }
};
inline std::string toString(const Plmn &o) {
  std::ostringstream ostr;
  ostr << "Plmn{mcc:";
  ostr << (o.hasMcc() ? o.getMcc() : "<invalid>");
  ostr << ", mnc:";
  ostr << (o.hasMnc() ? o.getMnc() : "<invalid>");
  ostr << "}";
  return ostr.str();
}

class Value128 {
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(uint64_t, Lsb);
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(uint64_t, Msb);

public:
  Value128() {
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(Lsb);
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(Msb);
  }
  Value128(const Value128 &from) {
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, Lsb);
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, Msb);
  }
};
inline std::string toString(const Value128 &o) {
  std::ostringstream ostr;
  ostr << "Value128{msb:" << o.getMsb() << ", lsb:" << o.getMsb() << "}";
  return ostr.str();
}

class AnnouncingPolicy {
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR_SHARED_PTR(Plmn, Plmn);
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(uint32_t, ValidityTime);
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(Range, Range);

public:
  AnnouncingPolicy() {
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR_SHARED_PTR(Plmn);
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(ValidityTime);
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(Range);
  }
  AnnouncingPolicy(const AnnouncingPolicy &from) {
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR_SHARED_PTR(from, Plmn);
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, ValidityTime);
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, Range);
  }
};
class MonitoringPolicy {
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR_SHARED_PTR(Plmn, Plmn);
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(uint32_t, ValidityTime);
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(uint32_t, RemainingTime);

public:
  MonitoringPolicy() {
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR_SHARED_PTR(Plmn);
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(ValidityTime);
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(RemainingTime);
  }
  MonitoringPolicy(const MonitoringPolicy &from) {
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR_SHARED_PTR(from, Plmn);
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, ValidityTime);
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, RemainingTime);
  }
};

class GetLtedConfigResp : public qcril::interfaces::BasePayload {
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(std::string, Apn);
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR_VECTOR(AnnouncingPolicy, AnnouncingPolicy);
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR_VECTOR(MonitoringPolicy, MonitoringPolicy);

public:
  GetLtedConfigResp() {
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(Apn);
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR_VECTOR(AnnouncingPolicy);
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR_VECTOR(MonitoringPolicy);
  }
  ~GetLtedConfigResp() {}
  GetLtedConfigResp(const GetLtedConfigResp &from) {
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, Apn);
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR_VECTOR(from, AnnouncingPolicy);
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR_VECTOR(from, MonitoringPolicy);
  }
};

class GetLtedCategoryResp : public qcril::interfaces::BasePayload {
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(std::string, OsAppId);
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(Category, Category);

public:
  GetLtedCategoryResp() {
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(OsAppId);
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(Category);
  }
  ~GetLtedCategoryResp() {}
  GetLtedCategoryResp(const GetLtedCategoryResp &from) {
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, OsAppId);
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, Category);
  }
};

class ServiceStatus : public qcril::interfaces::BasePayload {
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(uint32_t, PublishAllowed);
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(uint32_t, SubscribeAllowed);

public:
  ServiceStatus() {
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(PublishAllowed);
    QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(SubscribeAllowed);
  }
  ~ServiceStatus() {}
  ServiceStatus(const ServiceStatus &from) {
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, PublishAllowed);
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, SubscribeAllowed);
  }
};

class DeviceCapability : public qcril::interfaces::BasePayload {
  QCRIL_INTERFACE_OPTIONAL_MEMBER_VAR(uint32_t, Capability);

public:
  DeviceCapability() { QCRIL_INTERFACE_RESET_OPTIONAL_MEMBER_VAR(Capability); }
  ~DeviceCapability() {}
  DeviceCapability(const DeviceCapability &from) {
    QCRIL_INTERFACE_COPY_OPTIONAL_MEMBER_VAR(from, Capability);
  }
};

} // namespace lte_direct
} // namespace interfaces
} // namespace qcril
