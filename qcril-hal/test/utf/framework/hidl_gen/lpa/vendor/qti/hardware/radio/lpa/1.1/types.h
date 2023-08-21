/******************************************************************************
#  Copyright (c) 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ---------------------------------------------------------------------------

  @file framework/hidl_gen/lpa/vendor/qti/hardware/radio/lpa/1.1/types.h
  DESCRIPTION
  ---------------------------------------------------------------------------
******************************************************************************/
#ifndef HIDL_GENERATED_VENDOR_QTI_HARDWARE_RADIO_LPA_V1_1_TYPES_H
#define HIDL_GENERATED_VENDOR_QTI_HARDWARE_RADIO_LPA_V1_1_TYPES_H

#include <vendor/qti/hardware/radio/lpa/1.0/types.h>

#include <hidl/HidlSupport.h>
#include <hidl/MQDescriptor.h>
#include <utils/NativeHandle.h>
#include <utils/misc.h>

namespace vendor {
namespace qti {
namespace hardware {
namespace radio {
namespace lpa {
namespace V1_1 {

// Forward declaration for forward reference support:
enum class UimLpaUserEventId : uint32_t;
enum class UimLpaAddProfileStatus : uint32_t;
struct UimLpaUserReq;
struct UimLpaAddProfileProgressIndV1_1;

enum class UimLpaUserEventId : uint32_t {
    UIM_LPA_UNKNOWN_EVENT_ID = 0u,
    UIM_LPA_ADD_PROFILE = 1u,
    UIM_LPA_ENABLE_PROFILE = 2u,
    UIM_LPA_DISABLE_PROFILE = 3u,
    UIM_LPA_DELETE_PROFILE = 4u,
    UIM_LPA_EUICC_MEMORY_RESET = 5u,
    UIM_LPA_GET_PROFILE = 6u,
    UIM_LPA_UPDATE_NICKNAME = 7u,
    UIM_LPA_GET_EID = 8u,
    UIM_LPA_USER_CONSENT = 9u,
    UIM_LPA_SRV_ADDR_OPERATION = 10u,
    UIM_LPA_CONFIRM_CODE = 11u,
};

enum class UimLpaAddProfileStatus : uint32_t {
    UIM_LPA_ADD_PROFILE_STATUS_NONE = 0u,
    UIM_LPA_ADD_PROFILE_STATUS_ERROR = 1u,
    UIM_LPA_ADD_PROFILE_STATUS_DOWNLOAD_PROGRESS = 2u,
    UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_PROGRESS = 3u,
    UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_COMPLETE = 4u,
    UIM_LPA_ADD_PROFILE_STATUS_GET_USER_CONSENT = 5u,
    UIM_LPA_ADD_PROFILE_STATUS_SEND_CONF_CODE_REQ = 6u,
};

struct UimLpaUserReq final {
    ::vendor::qti::hardware::radio::lpa::V1_0::UimLpaUserReq base __attribute__ ((aligned(8)));
    int32_t nok_reason __attribute__ ((aligned(4)));
};

static_assert(offsetof(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq, base) == 0, "wrong offset");
static_assert(offsetof(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq, nok_reason) == 104, "wrong offset");
static_assert(sizeof(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq) == 112, "wrong size");
static_assert(__alignof(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq) == 8, "wrong alignment");

struct UimLpaAddProfileProgressIndV1_1 final {
    ::vendor::qti::hardware::radio::lpa::V1_0::UimLpaAddProfileProgressInd base __attribute__ ((aligned(4)));
    ::android::hardware::hidl_string profileName __attribute__ ((aligned(8)));
};

static_assert(offsetof(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1, base) == 0, "wrong offset");
static_assert(offsetof(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1, profileName) == 24, "wrong offset");
static_assert(sizeof(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1) == 40, "wrong size");
static_assert(__alignof(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1) == 8, "wrong alignment");

//
// type declarations for package
//

template<typename>
static inline std::string toString(uint32_t o);
static inline std::string toString(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId o);
static inline void PrintTo(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId o, ::std::ostream* os);
constexpr uint32_t operator|(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator|(const uint32_t lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId rhs) {
    return static_cast<uint32_t>(lhs | static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator|(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId lhs, const uint32_t rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) | rhs);
}
constexpr uint32_t operator&(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator&(const uint32_t lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId rhs) {
    return static_cast<uint32_t>(lhs & static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator&(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId lhs, const uint32_t rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) & rhs);
}
constexpr uint32_t &operator|=(uint32_t& v, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId e) {
    v |= static_cast<uint32_t>(e);
    return v;
}
constexpr uint32_t &operator&=(uint32_t& v, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId e) {
    v &= static_cast<uint32_t>(e);
    return v;
}

template<typename>
static inline std::string toString(uint32_t o);
static inline std::string toString(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus o);
static inline void PrintTo(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus o, ::std::ostream* os);
constexpr uint32_t operator|(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator|(const uint32_t lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus rhs) {
    return static_cast<uint32_t>(lhs | static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator|(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus lhs, const uint32_t rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) | rhs);
}
constexpr uint32_t operator&(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator&(const uint32_t lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus rhs) {
    return static_cast<uint32_t>(lhs & static_cast<uint32_t>(rhs));
}
constexpr uint32_t operator&(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus lhs, const uint32_t rhs) {
    return static_cast<uint32_t>(static_cast<uint32_t>(lhs) & rhs);
}
constexpr uint32_t &operator|=(uint32_t& v, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus e) {
    v |= static_cast<uint32_t>(e);
    return v;
}
constexpr uint32_t &operator&=(uint32_t& v, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus e) {
    v &= static_cast<uint32_t>(e);
    return v;
}

static inline std::string toString(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& o);
static inline void PrintTo(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& o, ::std::ostream*);
static inline bool operator==(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& rhs);
static inline bool operator!=(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& rhs);

static inline std::string toString(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& o);
static inline void PrintTo(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& o, ::std::ostream*);
static inline bool operator==(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& rhs);
static inline bool operator!=(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& rhs);

//
// type header definitions for package
//

template<>
inline std::string toString<::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId>(uint32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId> flipped = 0;
    bool first = true;
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_UNKNOWN_EVENT_ID) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_UNKNOWN_EVENT_ID)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_UNKNOWN_EVENT_ID";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_UNKNOWN_EVENT_ID;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_ADD_PROFILE) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_ADD_PROFILE)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_ADD_PROFILE";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_ADD_PROFILE;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_ENABLE_PROFILE) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_ENABLE_PROFILE)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_ENABLE_PROFILE";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_ENABLE_PROFILE;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_DISABLE_PROFILE) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_DISABLE_PROFILE)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_DISABLE_PROFILE";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_DISABLE_PROFILE;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_DELETE_PROFILE) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_DELETE_PROFILE)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_DELETE_PROFILE";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_DELETE_PROFILE;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_EUICC_MEMORY_RESET) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_EUICC_MEMORY_RESET)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_EUICC_MEMORY_RESET";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_EUICC_MEMORY_RESET;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_GET_PROFILE) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_GET_PROFILE)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_GET_PROFILE";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_GET_PROFILE;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_UPDATE_NICKNAME) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_UPDATE_NICKNAME)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_UPDATE_NICKNAME";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_UPDATE_NICKNAME;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_GET_EID) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_GET_EID)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_GET_EID";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_GET_EID;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_USER_CONSENT) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_USER_CONSENT)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_USER_CONSENT";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_USER_CONSENT;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_SRV_ADDR_OPERATION) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_SRV_ADDR_OPERATION)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_SRV_ADDR_OPERATION";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_SRV_ADDR_OPERATION;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_CONFIRM_CODE) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_CONFIRM_CODE)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_CONFIRM_CODE";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_CONFIRM_CODE;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId o) {
    using ::android::hardware::details::toHexString;
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_UNKNOWN_EVENT_ID) {
        return "UIM_LPA_UNKNOWN_EVENT_ID";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_ADD_PROFILE) {
        return "UIM_LPA_ADD_PROFILE";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_ENABLE_PROFILE) {
        return "UIM_LPA_ENABLE_PROFILE";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_DISABLE_PROFILE) {
        return "UIM_LPA_DISABLE_PROFILE";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_DELETE_PROFILE) {
        return "UIM_LPA_DELETE_PROFILE";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_EUICC_MEMORY_RESET) {
        return "UIM_LPA_EUICC_MEMORY_RESET";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_GET_PROFILE) {
        return "UIM_LPA_GET_PROFILE";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_UPDATE_NICKNAME) {
        return "UIM_LPA_UPDATE_NICKNAME";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_GET_EID) {
        return "UIM_LPA_GET_EID";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_USER_CONSENT) {
        return "UIM_LPA_USER_CONSENT";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_SRV_ADDR_OPERATION) {
        return "UIM_LPA_SRV_ADDR_OPERATION";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_CONFIRM_CODE) {
        return "UIM_LPA_CONFIRM_CODE";
    }
    std::string os;
    os += toHexString(static_cast<uint32_t>(o));
    return os;
}

static inline void PrintTo(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId o, ::std::ostream* os) {
    *os << toString(o);
}

template<>
inline std::string toString<::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus>(uint32_t o) {
    using ::android::hardware::details::toHexString;
    std::string os;
    ::android::hardware::hidl_bitfield<::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus> flipped = 0;
    bool first = true;
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_NONE) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_NONE)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_ADD_PROFILE_STATUS_NONE";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_NONE;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_ERROR) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_ERROR)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_ADD_PROFILE_STATUS_ERROR";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_ERROR;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_DOWNLOAD_PROGRESS) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_DOWNLOAD_PROGRESS)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_ADD_PROFILE_STATUS_DOWNLOAD_PROGRESS";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_DOWNLOAD_PROGRESS;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_PROGRESS) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_PROGRESS)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_PROGRESS";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_PROGRESS;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_COMPLETE) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_COMPLETE)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_COMPLETE";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_COMPLETE;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_GET_USER_CONSENT) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_GET_USER_CONSENT)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_ADD_PROFILE_STATUS_GET_USER_CONSENT";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_GET_USER_CONSENT;
    }
    if ((o & ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_SEND_CONF_CODE_REQ) == static_cast<uint32_t>(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_SEND_CONF_CODE_REQ)) {
        os += (first ? "" : " | ");
        os += "UIM_LPA_ADD_PROFILE_STATUS_SEND_CONF_CODE_REQ";
        first = false;
        flipped |= ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_SEND_CONF_CODE_REQ;
    }
    if (o != flipped) {
        os += (first ? "" : " | ");
        os += toHexString(o & (~flipped));
    }os += " (";
    os += toHexString(o);
    os += ")";
    return os;
}

static inline std::string toString(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus o) {
    using ::android::hardware::details::toHexString;
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_NONE) {
        return "UIM_LPA_ADD_PROFILE_STATUS_NONE";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_ERROR) {
        return "UIM_LPA_ADD_PROFILE_STATUS_ERROR";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_DOWNLOAD_PROGRESS) {
        return "UIM_LPA_ADD_PROFILE_STATUS_DOWNLOAD_PROGRESS";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_PROGRESS) {
        return "UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_PROGRESS";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_COMPLETE) {
        return "UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_COMPLETE";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_GET_USER_CONSENT) {
        return "UIM_LPA_ADD_PROFILE_STATUS_GET_USER_CONSENT";
    }
    if (o == ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_SEND_CONF_CODE_REQ) {
        return "UIM_LPA_ADD_PROFILE_STATUS_SEND_CONF_CODE_REQ";
    }
    std::string os;
    os += toHexString(static_cast<uint32_t>(o));
    return os;
}

static inline void PrintTo(::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus o, ::std::ostream* os) {
    *os << toString(o);
}

static inline std::string toString(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".base = ";
    os += ::vendor::qti::hardware::radio::lpa::V1_0::toString(o.base);
    os += ", .nok_reason = ";
    os += ::android::hardware::toString(o.nok_reason);
    os += "}"; return os;
}

static inline void PrintTo(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& o, ::std::ostream* os) {
    *os << toString(o);
}

static inline bool operator==(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& rhs) {
    if (lhs.base != rhs.base) {
        return false;
    }
    if (lhs.nok_reason != rhs.nok_reason) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& rhs){
    return !(lhs == rhs);
}

static inline std::string toString(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& o) {
    using ::android::hardware::toString;
    std::string os;
    os += "{";
    os += ".base = ";
    os += ::vendor::qti::hardware::radio::lpa::V1_0::toString(o.base);
    os += ", .profileName = ";
    os += ::android::hardware::toString(o.profileName);
    os += "}"; return os;
}

static inline void PrintTo(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& o, ::std::ostream* os) {
    *os << toString(o);
}

static inline bool operator==(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& rhs) {
    if (lhs.base != rhs.base) {
        return false;
    }
    if (lhs.profileName != rhs.profileName) {
        return false;
    }
    return true;
}

static inline bool operator!=(const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& lhs, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileProgressIndV1_1& rhs){
    return !(lhs == rhs);
}


}  // namespace V1_1
}  // namespace lpa
}  // namespace radio
}  // namespace hardware
}  // namespace qti
}  // namespace vendor

//
// global type declarations for package
//

namespace android {
namespace hardware {
namespace details {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
template<> inline constexpr std::array<::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId, 12> hidl_enum_values<::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId> = {
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_UNKNOWN_EVENT_ID,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_ADD_PROFILE,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_ENABLE_PROFILE,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_DISABLE_PROFILE,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_DELETE_PROFILE,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_EUICC_MEMORY_RESET,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_GET_PROFILE,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_UPDATE_NICKNAME,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_GET_EID,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_USER_CONSENT,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_SRV_ADDR_OPERATION,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserEventId::UIM_LPA_CONFIRM_CODE,
};
#pragma clang diagnostic pop
}  // namespace details
}  // namespace hardware
}  // namespace android

namespace android {
namespace hardware {
namespace details {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++17-extensions"
template<> inline constexpr std::array<::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus, 7> hidl_enum_values<::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus> = {
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_NONE,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_ERROR,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_DOWNLOAD_PROGRESS,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_PROGRESS,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_INSTALLATION_COMPLETE,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_GET_USER_CONSENT,
    ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaAddProfileStatus::UIM_LPA_ADD_PROFILE_STATUS_SEND_CONF_CODE_REQ,
};
#pragma clang diagnostic pop
}  // namespace details
}  // namespace hardware
}  // namespace android


#endif  // HIDL_GENERATED_VENDOR_QTI_HARDWARE_RADIO_LPA_V1_1_TYPES_H
