/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */

#ifndef __COMMON_H
#define __COMMON_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <cstdlib>
#include <unordered_map>
#include <set>

#include <vector>
#include <string>
#include <string.h>
#include <cutils/log.h>

#include <sstream>
#include <fstream>
#include <iomanip>

#include "mp-ctl.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "vendor.qti.platform_boost@1.0-service"
#endif

#define PERFD_LIB  "libqti-perfd-client.so"
#define BOOST_MODE_CONFIG_FILE "/vendor/etc/platform_boost/boost_mode.xml"
#define BOOST_CAP_CONFIG_FILE "/vendor/etc/platform_boost/boost_cap.xml"

//All definition strings are lower-case, the relative input from Clients also convert to lowercase.
#define MODE_STR_MAIN_DELIMITER  "@"
#define MODE_STR_KEY_VALUE_DELIMITER  "="
#define MODE_STR_INLINE_MODE "inline"
#define MODE_STR_KEY_MODE  "mode"
#define MODE_STR_KEY_PKG  "pkg"
#define MODE_STR_KEY_VER  "ver"
#define CAP_KEY_DELI  "_"

#define CPU_SMALL_CLUSTER "cpu_small"
#define CPU_BIG_CLUSTER "cpu_big"
#define CPU_PRIME_CLUSTER "cpu_prime"
#define INLINE_MODE_ITEM_CPU_PRIME "cpu_prime"
#define INLINE_MODE_CPU_SMALL_MIN "cpu_small_min"
#define INLINE_MODE_CPU_SMALL_MAX "cpu_small_max"
#define INLINE_MODE_CPU_BIG_MIN "cpu_big_min"
#define INLINE_MODE_CPU_BIG_MAX "cpu_big_max"
#define INLINE_MODE_CPU_PRIME_MIN "cpu_prime_min"
#define INLINE_MODE_CPU_PRIME_MAX "cpu_prime_max"
#define INLINE_MODE_SCHED_BOOST "schedule_boost"
#define INLINE_MODE_LPM_DISABLE "lpm_disable"
#define INLINE_MODE_STORAGE_BOOST "storage_boost"

#define INLINE_MODE_VALUE_MAP_DELIMITER "@"

#define CPU_PERFORMANCE_RANK_BASE_FREQ 1000000 /** Unit: KHZ **/
#define CPU_MIN_FREQ_SYS_NODE "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_min_freq"
#define CPU_CUR_FREQ_SYS_NODE "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq"
#define CPU_MAX_FREQ_SYS_NODE "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_max_freq"
#define CPU_RELATED_CPUS_SYS_NODE "/sys/devices/system/cpu/cpu%d/cpufreq/related_cpus"

#define CPU_FREQ_RANK 10
#define CAP_TARGET_CPU "cpu"
#define CAP_CAPABILITY_PERFORMANCE "performance"
#define CAP_VALUE_LEVEL "level"

using namespace std;

class boost_mode_info {
    public:
        string mode_name;
        uint32_t hint_id;
        int32_t  hint_type;

    boost_mode_info() {}

    boost_mode_info(string mode_name, uint32_t hint_id, int32_t hint_type) {
        this->mode_name = mode_name;
        this->hint_id = hint_id;
        this->hint_type = hint_type;
    }

    boost_mode_info(string mode_name, uint32_t hint_id) {
        boost_mode_info(mode_name, hint_id, -1);
    }
};

class boost_cap_info {
    public:
        string target;
        string capability;
        string value;
        int32_t num_args;
        int32_t *opcodes;

    boost_cap_info() {}

    boost_cap_info(string target, string capability, string value, int32_t *opcodes, int32_t num_args) {
        this->target = target;
        this->capability = capability;
        this->value = value;
        this->opcodes = opcodes;
        this->num_args = num_args;
    }
};

struct handle_info {
    int32_t handle_id;
    int32_t pid;
    uint32_t hint_id;
    int32_t hint_type;

    bool operator < (const handle_info &hi) const {
        return handle_id < hi.handle_id;
    }
};

#endif
