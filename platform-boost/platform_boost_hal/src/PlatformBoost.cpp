/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#include "PlatformBoost.h"
#include "ConfigUtils.h"

namespace vendor {
namespace qti {
namespace platform_boost {
namespace V1_0 {
namespace implementation {

PlatformBoost::PlatformBoost(){

    //Parse Config File: boost_mode.xml
    parse_boost_mode_config(BOOST_MODE_CONFIG_FILE, &mode_map);

    //init inline mode data
    init_inline_mode_data(&cpu_cluster_map, &inline_mode_opcode_map, &inline_mode_value_map);

    //Parse Config File: boost_cap.xml
    parse_boost_cap_config(BOOST_CAP_CONFIG_FILE, &cap_map, &target_cap_map);

    //Open libqti-perfd-client.so
    char lib_path[PATH_MAX] = {0};
    if (property_get("ro.vendor.extension_library", lib_path, nullptr) != 0) {
        handle_perf = dlopen(PERFD_LIB, RTLD_NOW);
        if (!handle_perf) {
            ALOGE("unable to open %s: %s", PERFD_LIB, dlerror());
        } else {
            ALOGI("open successful %s: ", PERFD_LIB);

            perf_lock_acq = (int (*)(unsigned long, int, int *, int))dlsym(handle_perf, "perf_lock_acq");
            if (!perf_lock_acq) {
                ALOGE("couldn't get perf_lock_acq function handle.");
            }

            perf_hint = (int (*)(int, const char *, int, int))dlsym(handle_perf, "perf_hint");
            if (!perf_hint) {
                ALOGE("couldn't get perf_hint function handle.");
            }

            perf_lock_rel = (int (*)(unsigned long))dlsym(handle_perf, "perf_lock_rel");
            if (!perf_lock_rel) {
                ALOGE("couldn't get perf_hint function handle.");
            }
        }
    } else {
        ALOGE("Failed to get perf lib.");
    }
}

PlatformBoost::~PlatformBoost() {
}

/*
*
*
*/
Return<int32_t> PlatformBoost::boostMode(const hidl_string& mode, int32_t duration){
    int32_t ret_handle = -1;
    int32_t ret_not_support = -2;
    ALOGI("start to boostMode: %s, duration: %d", mode.c_str(), duration);

    //Parse mode string
    parse_mode_str(mode, &mode_str_map);

    if (mode_str_map.empty()){
        ALOGI("boostMode fail: param mode is incorrect: %s ", mode.c_str());
        return ret_not_support;
    }

    //If mode exists
    if (mode_str_map.count(MODE_STR_KEY_MODE) == 1){
        string mode_name = mode_str_map[MODE_STR_KEY_MODE];
        ALOGI("mode name : %s ", mode_name.c_str());

        //Mode: inline
        if (!strcasecmp(mode_name.c_str(), MODE_STR_INLINE_MODE)) {

            vector<int32_t> opcodes;

            for (const auto& item : mode_str_map) {
                string key = to_lower(item.first);
                string value = item.second;

                //Ingore InlineMode item and Check if support this key
                if (!strcasecmp(key.c_str(), MODE_STR_KEY_MODE)){
                    continue;
                }

                if (inline_mode_opcode_map.count(key) == 0){
                    ALOGE("Don't support this key: %s", (item.first).c_str());
                    continue;
                }

                if(!strcasecmp(key.c_str(), INLINE_MODE_CPU_SMALL_MIN) ||
                        !strcasecmp(key.c_str(), INLINE_MODE_CPU_SMALL_MAX)){
                    string value_map_key;
                    value_map_key.append(key).append(INLINE_MODE_VALUE_MAP_DELIMITER).append(to_lower(value));
                    if (inline_mode_value_map.count(value_map_key) == 0){
                        ALOGE("Don't support abstract value %s for %s", (item.second).c_str(), (item.first).c_str());
                        continue;
                    }
                    opcodes.push_back(inline_mode_opcode_map[key]);
                    opcodes.push_back(inline_mode_value_map[value_map_key]);

                } else if(!strcasecmp(key.c_str(), INLINE_MODE_CPU_BIG_MIN) ||
                            !strcasecmp(key.c_str(), INLINE_MODE_CPU_BIG_MAX)){
                    string value_map_key;
                    value_map_key.append(key).append(INLINE_MODE_VALUE_MAP_DELIMITER).append(to_lower(value));
                    if (inline_mode_value_map.count(value_map_key) == 0){
                        ALOGE("Don't support abstract value %s for %s", (item.second).c_str(), (item.first).c_str());
                        continue;
                    }

                    opcodes.push_back(inline_mode_opcode_map[key]);
                    opcodes.push_back(inline_mode_value_map[value_map_key]);

                    //Also need boost prime core
                    string_replase(key, "big", "prime");
                    string prime_value_map_key;
                    prime_value_map_key.append(key).append(INLINE_MODE_VALUE_MAP_DELIMITER).append(to_lower(value));
                    if (inline_mode_value_map.count(prime_value_map_key) == 0){
                        ALOGE("Don't exist prime core");
                        continue;
                    }

                    opcodes.push_back(inline_mode_opcode_map[key]);
                    opcodes.push_back(inline_mode_value_map[prime_value_map_key]);

                } else {
                    opcodes.push_back(inline_mode_opcode_map[key]);
                    opcodes.push_back(stoi(value));
                }

            }

            if (handle_perf && perf_lock_acq) {
                duration = duration < 0 ? 0 : duration;
                int32_t *ops = new int32_t[opcodes.size()];
                if (!opcodes.empty()){
                    memcpy(ops, &opcodes[0], opcodes.size()*sizeof(int32_t));
                }
                ret_handle = perf_lock_acq(ret_handle, duration, ops, opcodes.size());
                ALOGD("ret_handle: %d", ret_handle);

                if (ret_handle > 0){
                    //Mutex::Autolock _l(mLock);
                    int32_t pid = android::hardware::IPCThreadState::self()->getCallingPid();
                    handle_info info = {ret_handle, pid, NULL, -1};
                    handle_info_set.insert(info);
                    ALOGE("Boost Mode success: handle_id: %d", ret_handle);
                } else {
                    ALOGE("Boost Mode fail: handle_id: %d", ret_handle);
                }
            } else {
                ALOGE("Boost Mode fail, can not connect to perf HAL");
            }

        } else {
            //Query mode if exists in boost_mode.xml
            unordered_map < string, boost_mode_info * >::iterator iter;
            iter = mode_map.find(mode_name);

            if (iter != mode_map.end()){
                boost_mode_info *mode_info = (boost_mode_info *) (iter->second);

                //Check if open perfd client normal.
                if (handle_perf && perf_hint) {
                    duration = duration < 0 ? 0 : duration;
                    const char* pkg = (mode_str_map[MODE_STR_KEY_PKG]).c_str();
                    ret_handle = perf_hint(mode_info->hint_id, pkg, duration, mode_info->hint_type);

                    if (ret_handle > 0){
                        //Mutex::Autolock _l(mLock);
                        int32_t pid = android::hardware::IPCThreadState::self()->getCallingPid();
                        handle_info info = {ret_handle, pid, mode_info->hint_id, mode_info->hint_type};
                        handle_info_set.insert(info);
                        ALOGE("Boost mode success: handle_id: %d", ret_handle);
                    } else {
                        ALOGE("Boost mode fail: handle_id: %d", ret_handle);
                    }
                } else {
                    ALOGE("Boost mode fail, can not connect to perf HAL");
                }
            } else {
                ALOGE("Don't support this boost mode : %s", mode.c_str());
                return ret_not_support;
            }
        }
    }

    return ret_handle;
}

Return<int32_t> PlatformBoost::boostCap(const hidl_string& target, const hidl_string& cap, const hidl_string& value, int32_t duration){
    int32_t ret_handle = -1;
    int32_t ret_not_support = -2;
    string in_target = to_lower(target);
    string in_cap = to_lower(cap);
    string in_value = to_lower(value);
    ALOGI("start to boostCap: target: %s, cap: %s, value: %s, duration: %d", in_target.c_str(), in_cap.c_str(), in_value.c_str(), duration);

    //Query Cap if support
    string cap_key;
    cap_key.append(in_target).append(CAP_KEY_DELI).append(in_cap).append(CAP_KEY_DELI).append(in_value);
    unordered_map < string, boost_cap_info * >::iterator iter;
    iter = cap_map.find(cap_key);

    if (iter != cap_map.end()){
        boost_cap_info *cap_info = (boost_cap_info *) (iter->second);

        //Check if open perfd client normal.
        if (handle_perf && perf_lock_acq) {
            duration = duration < 0 ? 0 : duration;
            ret_handle = perf_lock_acq(ret_handle, duration, cap_info->opcodes, cap_info->num_args);
            ALOGI("ret_handle: %d", ret_handle);

            if (ret_handle > 0){
                //Mutex::Autolock _l(mLock);
                int32_t pid = android::hardware::IPCThreadState::self()->getCallingPid();
                handle_info info = {ret_handle, pid, NULL, -1};
                handle_info_set.insert(info);
                ALOGE("Boost Cap success: handle_id: %d", ret_handle);
            } else {
                ALOGE("Boost Cap fail: handle_id: %d", ret_handle);
            }
        } else {
            ALOGE("Boost Cap fail, can not connect to perf HAL");
        }
    } else {
        ALOGE("Don't support this boost cap");
        return ret_not_support;
    }

    return ret_handle;
}

Return<PlatformBoostQueryState> PlatformBoost::queryMode(const hidl_string& mode){
    PlatformBoostQueryState state = PlatformBoostQueryState::UNSUPPORTED;
    ALOGI("start to queryMode: mode: %s", mode.c_str());

    if( !mode_map.empty() && mode_map.count(to_lower(mode)) > 0){
        state = PlatformBoostQueryState::SUCCESS;
        ALOGI("You get it! %s ", mode.c_str());
    } else {
        ALOGI("This mode don't support: %s ", mode.c_str());
    }

    return state;
}

Return<PlatformBoostQueryState> PlatformBoost::queryCap(const hidl_string& target, const hidl_string& cap){
    PlatformBoostQueryState state = PlatformBoostQueryState::UNSUPPORTED;
    ALOGI("start to queryCap: target: %s, cap: %s", target.c_str(), cap.c_str());

    string tar_cap;
    tar_cap.append(to_lower(target)).append(CAP_KEY_DELI).append(to_lower(cap));

    if( !target_cap_map.empty() && target_cap_map.count(tar_cap) > 0){
        state = PlatformBoostQueryState::SUCCESS;
        ALOGI("You get it! target: %s , cap: %s ", target.c_str(), cap.c_str());
    } else {
        ALOGI("Don't support this cap: target: %s , cap: %s ", target.c_str(), cap.c_str());
    }

    return state;
}

Return<void> PlatformBoost::boostRelease(int32_t pl_handle){
    ALOGI("start to boostRelease: handle id: %d", pl_handle);

    int ret = perf_lock_rel(pl_handle);

    //Mutex::Autolock _l(mLock);
    handle_info info = {pl_handle, -1, NULL, -1};
    set<handle_info>::iterator iter = handle_info_set.find(info);
    if (iter != handle_info_set.end()) {
        ALOGI("boostRelease successful: handle id: %d", pl_handle);
        handle_info_set.erase(iter);
    } else {
        ALOGE("boostRelease fail: invalid handle id: %d", pl_handle);
    }

    return Void();
}

Return<PlatformBoostResult> PlatformBoost::restore(const PlatformBoostRestoreState state){
    PlatformBoostResult result = PlatformBoostResult::FAILED;
    ALOGI("start to restore");

    if(state == PlatformBoostRestoreState::BOOST_RESTORE_SYS_DEFAULT){
        //Mutex::Autolock _l(mLock);
        int32_t pid = android::hardware::IPCThreadState::self()->getCallingPid();
        set<handle_info>::iterator iter;
        for (iter = handle_info_set.begin(); iter != handle_info_set.end();) {
            if (iter->pid == pid) {
                perf_lock_rel(iter->handle_id);
                ALOGI("release handle_id: %d", iter->handle_id);
                handle_info_set.erase(iter++);
            } else {
                ++iter;
            }
        }
        result = PlatformBoostResult::SUCCESS;
    }

    return result;
}

Return<void> PlatformBoost::getSysStatus(const hidl_string& sys, const hidl_string& subSys, getSysStatus_cb _hidl_cb) {
    std::string status = "";
    ALOGI("start to getSysStatus: sys: %s, sub sys: %s", sys.c_str(), subSys.c_str());

    string sys_subs;
    sys_subs.append(to_lower(sys)).append(CAP_KEY_DELI).append(to_lower(subSys));

    if( !target_cap_map.empty() && target_cap_map.count(sys_subs) > 0){
        if(strcasecmp(sys.c_str(), CAP_TARGET_CPU) == 0&& strcasecmp(subSys.c_str(), CAP_CAPABILITY_PERFORMANCE) == 0){
            int32_t level = get_cpu_cur_freq_level(cpu_cluster_map[CPU_PRIME_CLUSTER]);
            if (level != -1){
                status = "";
                status.append(CAP_VALUE_LEVEL).append(to_string(level));
            } else {
                ALOGE("getSysStatus failed");
            }
        }
    } else {
        ALOGI("Don't exist this sys: %s and sub sys: %s", sys.c_str(), subSys.c_str());
    }

    _hidl_cb(status);
    return Void();
}

Return<PlatformBoostResult> PlatformBoost::registerForSystemEvents(const sp<IPlatformBoostSysEventCallback>& callback){
    PlatformBoostResult result = PlatformBoostResult::FAILED;
    ALOGI("start to registerForSystemEvents");

    int ret = add_callback(callback);
    if (ret)
        ALOGE("Failed to add callback: %d", ret);
    else
        result = PlatformBoostResult::SUCCESS;

    return result;
}

IPlatformBoost* HIDL_FETCH_IPlatformBoost(const char* /* name */) {
    ALOGI("start to HIDL_FETCH_IPlatformBoost");
    return new PlatformBoost();
}

// private methods follow

/**
 * In boostMode, the first param mode's extensible format is mode_name@key1=val1@key2=val2
 * Parse it and store in map.
 */
void PlatformBoost::parse_mode_str(string mode_str, unordered_map<string, string> *map) {
    map->clear();
    ALOGI("Whole mode param : %s", mode_str.c_str());

    if (mode_str.empty()){
        ALOGE("Param mode is Null");
        return;
    }

    char *outer_tok, *outer_ptr;
    for (outer_tok = strtok_r(const_cast<char*>(mode_str.c_str()), MODE_STR_MAIN_DELIMITER, &outer_ptr);
            outer_tok; outer_tok = strtok_r(NULL, MODE_STR_MAIN_DELIMITER, &outer_ptr)){
        if (strstr(outer_tok, MODE_STR_KEY_VALUE_DELIMITER) == NULL){
            ALOGI("mode: %s", outer_tok);
            (*map)[MODE_STR_KEY_MODE] = to_lower((string)outer_tok);
        } else {
            vector<string> key_value_vec;
            char *inner_tok, *inner_ptr;
            for (inner_tok = strtok_r(outer_tok, MODE_STR_KEY_VALUE_DELIMITER, &inner_ptr);
                    inner_tok; inner_tok = strtok_r(NULL, MODE_STR_KEY_VALUE_DELIMITER, &inner_ptr)){
                    key_value_vec.push_back(inner_tok);
            }
            if (key_value_vec.size() == 2){
                string key = to_lower(key_value_vec[0]);
                (*map)[key] = key_value_vec[1];
                ALOGI("key - value: %s - %s", key.c_str(), key_value_vec[1].c_str());
            }
        }
    }
}

/**
 * Add callback to the corresponding list after linking to
 * death on the corresponding hidl object reference.
 */
template <class CallbackType>
int registerForDeathAndAddCallbackHidlObjectToList(
    const android::sp<CallbackType> &callback,
    const std::function<void(const android::sp<CallbackType> &)>
    &on_hidl_died_fctor,
    vector<android::sp<CallbackType>> &callback_list){
    // TODO register for HIDL object death notifications
    callback_list.push_back(callback);
    return 0;
}

int PlatformBoost::add_callback(const sp<IPlatformBoostSysEventCallback> &callback){
    // register for death notification before we add it to our list
    auto on_hidl_died_fctor = std::bind(
        &PlatformBoost::remove_callback, this, std::placeholders::_1);

    return registerForDeathAndAddCallbackHidlObjectToList<
        IPlatformBoostSysEventCallback>(callback, on_hidl_died_fctor, callbacks);
}

void PlatformBoost::remove_callback(const sp<IPlatformBoostSysEventCallback> &callback){
    callbacks.erase(
        std::remove(callbacks.begin(), callbacks.end(), callback),
        callbacks.end());
}

void PlatformBoost::call_with_each_callback(
    const std::function<Return<void>(sp<IPlatformBoostSysEventCallback>)> &method){
    for (const auto &callback : callbacks) {
        if (!method(callback).isOk()) {
            ALOGE("Failed to invoke HIDL callback");
        }
    }
}

}
}  // namespace V1_0
}  // namespace platform_boost
}  // namespace qti
}  // namespace vendor
