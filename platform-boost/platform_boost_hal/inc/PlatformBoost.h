/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#ifndef VENDOR_PLATFORM_BOOST_SERVICE_V1_0_IPLATFORM_BOOST_H
#define VENDOR_PLATFORM_BOOST_SERVICE_V1_0_IPLATFORM_BOOST_H

#include "Common.h"
#include "mp-ctl.h"
#include <vendor/qti/platform_boost/1.0/IPlatformBoost.h>

#include <hidl/Status.h>
#include <hwbinder/IPCThreadState.h>

#include <dlfcn.h>
#include <cutils/properties.h>

namespace vendor {
namespace qti {
namespace platform_boost {
namespace V1_0 {
namespace implementation {

using ::vendor::qti::platform_boost::V1_0::IPlatformBoost;
using ::vendor::qti::platform_boost::V1_0::IPlatformBoostSysEventCallback;
using ::vendor::qti::platform_boost::V1_0::PlatformBoostResult;
using ::vendor::qti::platform_boost::V1_0::PlatformBoostQueryState;
using ::vendor::qti::platform_boost::V1_0::PlatformBoostRestoreState;

using ::android::hidl::base::V1_0::IBase;
using ::android::hardware::hidl_string;
using ::android::hardware::Return;
using ::android::hardware::Void;

using ::std::string;
using ::android::sp;

struct PlatformBoost : public IPlatformBoost {
    PlatformBoost();
    ~PlatformBoost();
    Return<int32_t> boostMode(const hidl_string& mode, int32_t duration) override;
    Return<int32_t> boostCap(const hidl_string& target, const hidl_string& cap, const hidl_string& value, int32_t duration) override;
    Return<PlatformBoostQueryState> queryMode(const hidl_string& mode) override;
    Return<PlatformBoostQueryState> queryCap(const hidl_string& target, const hidl_string& cap) override;
    Return<void> boostRelease(int32_t pl_handle) override;
    Return<PlatformBoostResult> restore(const PlatformBoostRestoreState state) override;
    Return<void> getSysStatus(const hidl_string& sys, const hidl_string& subSys, getSysStatus_cb _hidl_cb) override;
    Return<PlatformBoostResult> registerForSystemEvents(const sp<IPlatformBoostSysEventCallback>& callback) override;

private:
    //Mutex mLock;
    void *handle_perf = NULL;

    //Map for boost_mode.xml's Mode items
    unordered_map<string, boost_mode_info *> mode_map;

    //Map for cpu name of different cpu cluster
    unordered_map<string, int32_t> cpu_cluster_map;

    //Map for param mode in boostMode method
    unordered_map<string, string> mode_str_map;

    //Map for opcodes of inline modes
    unordered_map<string, int32_t> inline_mode_opcode_map;

    //Map for values of inline modes
    unordered_map<string, int32_t> inline_mode_value_map;

    //Map for boost_cap.xml or Map for dynamic generated capabilities
    unordered_map<string, boost_cap_info *> cap_map;

    //For query cap efficient: key is <target>_<capability>, value is NULL
    unordered_map<string, string> target_cap_map;

    set<handle_info> handle_info_set;

    int32_t *cpu_freq_ranks;

    std::vector<sp<IPlatformBoostSysEventCallback>> callbacks;

    void parse_mode_str(string mode_str, unordered_map<string, string> *map);
    int add_callback(const sp<IPlatformBoostSysEventCallback> &callback);
    void remove_callback(const sp<IPlatformBoostSysEventCallback> &callback);
    void call_with_each_callback(const std::function<Return<void>(sp<IPlatformBoostSysEventCallback>)> &method);

    int (*perf_lock_acq)(unsigned long handle, int duration, int list[], int numArgs);
    int (*perf_lock_rel)(unsigned long handle);
    int (*perf_hint)(int, const char *, int, int);
    int (*perf_ux_engine_events)(int, int, const char *, int);
};

extern "C" IPlatformBoost* HIDL_FETCH_IPlatformBoost(const char* name);

}
}  // namespace V1_0
}  // namespace platform_boost
}  // namespace qti
}  // namespace vendor

#endif // VENDOR_PLATFORM_BOOST_SERVICE_V1_0_IPLATFORM_BOOST_H
