/******************************************************************************
#  Copyright (c) 2020 Qualcomm Technologies, Inc.
#  All Rights Reserved.
#  Confidential and Proprietary - Qualcomm Technologies, Inc.
  ---------------------------------------------------------------------------

  @file framework/hidl_gen/lpa/vendor/qti/hardware/radio/lpa/1.1/UimLpaAll.cpp
  DESCRIPTION
  ---------------------------------------------------------------------------
******************************************************************************/
#define LOG_TAG "vendor.qti.hardware.radio.lpa@1.1::UimLpa"

#undef UNUSED

#include <vendor/qti/hardware/radio/lpa/1.1/IUimLpa.h>
#include <log/log.h>
#include <cutils/trace.h>

#include <utils/Trace.h>
#include "ril_utf_hidl_services.h"

namespace vendor {
namespace qti {
namespace hardware {
namespace radio {
namespace lpa {
namespace V1_1 {

//std::string toString(const ::android::sp<IUimLpa>& o) {
//    std::string os = "[class or subclass of ";
//    os += ::vendor::qti::hardware::radio::lpa::V1_1::IUimLpa::descriptor;
//    os += "]";
//    os += o->isRemote() ? "@remote" : "@local";
//    return os;
//}

const char* IUimLpa::descriptor("vendor.qti.hardware.radio.lpa@1.1::IUimLpa");

// Methods from ::vendor::qti::hardware::radio::lpa::V1_0::IUimLpa follow.
// no default implementation for: ::android::hardware::Return<void> IUimLpa::setCallback(const ::android::sp<::vendor::qti::hardware::radio::lpa::V1_0::IUimLpaResponse>& responseCallback, const ::android::sp<::vendor::qti::hardware::radio::lpa::V1_0::IUimLpaIndication>& indicationCallback)
// no default implementation for: ::android::hardware::Return<void> IUimLpa::UimLpaUserRequest(int32_t token, const ::vendor::qti::hardware::radio::lpa::V1_0::UimLpaUserReq& userReq)
// no default implementation for: ::android::hardware::Return<void> IUimLpa::UimLpaHttpTxnCompletedRequest(int32_t token, ::vendor::qti::hardware::radio::lpa::V1_0::UimLpaResult result, const ::android::hardware::hidl_vec<uint8_t>& responsePayload, const ::android::hardware::hidl_vec<::vendor::qti::hardware::radio::lpa::V1_0::UimLpaHttpCustomHeader>& customHeaders)

// Methods from ::vendor::qti::hardware::radio::lpa::V1_1::IUimLpa follow.
// no default implementation for: ::android::hardware::Return<void> IUimLpa::UimLpaUserRequest_1_1(int32_t token, const ::vendor::qti::hardware::radio::lpa::V1_1::UimLpaUserReq& userReq)

// Methods from ::android::hidl::base::V1_0::IBase follow.
::android::hardware::Return<void> IUimLpa::interfaceChain(interfaceChain_cb _hidl_cb){
    return ::android::hardware::Void();
}

::android::hardware::Return<void> IUimLpa::debug(const ::android::hardware::hidl_handle& fd, const ::android::hardware::hidl_vec<::android::hardware::hidl_string>& options){
    (void)fd;
    (void)options;
    return ::android::hardware::Void();
}

::android::hardware::Return<void> IUimLpa::interfaceDescriptor(interfaceDescriptor_cb _hidl_cb){
    return ::android::hardware::Void();
}

::android::hardware::Return<void> IUimLpa::getHashChain(getHashChain_cb _hidl_cb){
    _hidl_cb({
        (uint8_t[32]){173,171,101,224,116,23,37,224,232,246,2,152,130,153,80,11,92,217,60,52,138,187,90,238,210,100,52,114,252,26,245,25} /* adab65e0741725e0e8f602988299500b5cd93c348abb5aeed2643472fc1af519 */,
        (uint8_t[32]){76,225,73,109,203,109,170,211,182,21,150,93,149,179,251,44,77,226,24,219,55,201,77,172,185,195,122,91,38,41,24,44} /* 4ce1496dcb6daad3b615965d95b3fb2c4de218db37c94dacb9c37a5b2629182c */,
        (uint8_t[32]){236,127,215,158,208,45,250,133,188,73,148,38,173,174,62,190,35,239,5,36,243,205,105,87,19,147,36,184,59,24,202,76} /* ec7fd79ed02dfa85bc499426adae3ebe23ef0524f3cd6957139324b83b18ca4c */});
    return ::android::hardware::Void();
}

::android::hardware::Return<void> IUimLpa::setHALInstrumentation(){
    return ::android::hardware::Void();
}

::android::hardware::Return<bool> IUimLpa::linkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient, uint64_t cookie){
    (void)cookie;
    return (recipient != nullptr);
}

::android::hardware::Return<void> IUimLpa::ping(){
    return ::android::hardware::Void();
}

::android::hardware::Return<void> IUimLpa::getDebugInfo(getDebugInfo_cb _hidl_cb){
    ::android::hidl::base::V1_0::DebugInfo info = {};
    info.pid = -1;
    info.ptr = 0;
    info.arch = 
    #if defined(__LP64__)
    ::android::hidl::base::V1_0::DebugInfo::Architecture::IS_64BIT
    #else
    ::android::hidl::base::V1_0::DebugInfo::Architecture::IS_32BIT
    #endif
    ;
    _hidl_cb(info);
    return ::android::hardware::Void();
}

::android::hardware::Return<void> IUimLpa::notifySyspropsChanged(){
    return ::android::hardware::Void();
}

::android::hardware::Return<bool> IUimLpa::unlinkToDeath(const ::android::sp<::android::hardware::hidl_death_recipient>& recipient){
    return (recipient != nullptr);
}


::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::lpa::V1_1::IUimLpa>> IUimLpa::castFrom(const ::android::sp<::vendor::qti::hardware::radio::lpa::V1_1::IUimLpa>& parent, bool /* emitError */) {
    return parent;
}

::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::lpa::V1_1::IUimLpa>> IUimLpa::castFrom(const ::android::sp<::vendor::qti::hardware::radio::lpa::V1_0::IUimLpa>& parent, bool emitError) {
    return nullptr;
}

::android::hardware::Return<::android::sp<::vendor::qti::hardware::radio::lpa::V1_1::IUimLpa>> IUimLpa::castFrom(const ::android::sp<::android::hidl::base::V1_0::IBase>& parent, bool emitError) {
    return nullptr;
}

::android::sp<IUimLpa> IUimLpa::tryGetService(const std::string &serviceName, const bool getStub) {
    return nullptr; 
}

::android::sp<IUimLpa> IUimLpa::getService(const std::string &serviceName, const bool getStub) {
    return nullptr;
}

::android::status_t IUimLpa::registerAsService(const std::string &serviceName) {
    return ::android::OK;
}

bool IUimLpa::registerForNotifications(
        const std::string &serviceName,
        const ::android::sp<::android::hidl::manager::V1_0::IServiceNotification> &notification) {
    return true;
}

static_assert(sizeof(::android::hardware::MQDescriptor<char, ::android::hardware::kSynchronizedReadWrite>) == 32, "wrong size");
static_assert(sizeof(::android::hardware::hidl_handle) == 16, "wrong size");
static_assert(sizeof(::android::hardware::hidl_memory) == 40, "wrong size");
static_assert(sizeof(::android::hardware::hidl_string) == 16, "wrong size");
static_assert(sizeof(::android::hardware::hidl_vec<char>) == 16, "wrong size");

}  // namespace V1_1
}  // namespace lpa
}  // namespace radio
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
