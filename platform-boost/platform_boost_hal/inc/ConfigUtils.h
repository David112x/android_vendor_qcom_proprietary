/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#include "Common.h"

#include <tinyxml2.h>

#define INVALID_HINT_ID -9999999

using namespace tinyxml2;

void parse_boost_mode_config(const char* cfg_path, unordered_map<string, boost_mode_info * > *mode_map);

void parse_boost_cap_config(const char* cfg_path, unordered_map<string, boost_cap_info * > *cap_map, unordered_map<string, string> *tar_cap_map);

void init_inline_mode_data(unordered_map<string, int32_t> *cpu_cluster_map, unordered_map<string, int32_t> *inline_mode_opcode_map,
    unordered_map<string, int32_t> *inline_mode_value_map);

int32_t get_cpu_cur_freq(int32_t cpu_no);

int32_t get_cpu_max_freq(int32_t cpu_no);

int32_t get_cpu_min_freq(int32_t cpu_no);

int32_t* rank_cpu_freq(int32_t min_freq, int32_t max_freq, int32_t rank_num);

int32_t get_cpu_cur_freq_level(int32_t cpu_no);

string to_lower(string org_str);

void split_string(char* content, vector<char*>& result_vec, const char* delimiter);

void get_cpu_cluster_info(unordered_map<string, int32_t> *cpu_cluster_map);

void string_replase(string &str, string old_str, string new_str);