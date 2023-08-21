/*
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
#include "ConfigUtils.h"

void parse_boost_mode_config(const char* cfg_path,
                      unordered_map<string, boost_mode_info * > *mode_map){
    ALOGD("parse_boost_mode_config cfg: %s",cfg_path);

    XMLDocument doc_xml;
    if(doc_xml.LoadFile(cfg_path) != 0){
        doc_xml.Clear();
        ALOGE("file(%s) read failed:", cfg_path);
        return;
    }

    mode_map->clear();

    XMLElement *elmt_root = doc_xml.RootElement();
    XMLElement *elmt_mode = elmt_root->FirstChildElement("Mode");

    while (elmt_mode) {
        string mode_name;
        uint32_t hint_id = INVALID_HINT_ID;
        int32_t  hint_type = -1;
        boost_mode_info *cur_mode = NULL;

        const XMLAttribute *attr_of_elet_mode = elmt_mode->FirstAttribute();

        if (attr_of_elet_mode->Name() != NULL && ! strcasecmp(attr_of_elet_mode->Name(), "name")){
            mode_name = to_lower(attr_of_elet_mode->Value());
            ALOGD("mode: %s", mode_name.c_str());

            XMLElement *mode_child = elmt_mode->FirstChildElement();
            while (mode_child) {
                if (! strcasecmp(mode_child->Name(), "hint_id")){
                    if(mode_child->GetText() != NULL){
                        ALOGD("hint_id : %s", mode_child->GetText());
                        hint_id = strtol(mode_child->GetText(), NULL, 16);
                    }
                } else if (! strcasecmp(mode_child->Name(), "hint_type")){
                    if(mode_child->GetText() != NULL){
                        ALOGD("hint_type : %s", mode_child->GetText());
                        hint_type = strtol(mode_child->GetText(), NULL, 10);
                    }
                }

                mode_child = mode_child->NextSiblingElement();
            }

            if ( hint_id != INVALID_HINT_ID){
                cur_mode = new boost_mode_info(mode_name, hint_id, hint_type);
                (*mode_map)[(string) mode_name] = cur_mode;
            }
            elmt_mode = elmt_mode->NextSiblingElement();
        }
    }
    doc_xml.Clear();
}

void parse_boost_cap_config(const char* cfg_path,
                    unordered_map<string, boost_cap_info * > *cap_map, unordered_map<string, string> *tar_cap_map){
    ALOGI("parse_boost_cap_config cfg: %s",cfg_path);

    XMLDocument doc_xml;
    if(doc_xml.LoadFile(cfg_path) != 0){
        ALOGE("file(%s) read failed:", cfg_path);
        doc_xml.Clear();
        return;
    }

    cap_map->clear();
    tar_cap_map->clear();

    XMLElement *elmt_root = doc_xml.RootElement();
    XMLElement *elmt_mode = elmt_root->FirstChildElement("Cap");
    while (elmt_mode) {
        string target;
        string capability;
        string value;
        int32_t *opcodes;
        int32_t num_args;
        boost_cap_info *cur_cap = NULL;

        const XMLAttribute *attr_of_elet_mode = elmt_mode->FirstAttribute();
        while (attr_of_elet_mode){
            if (attr_of_elet_mode->Name() != NULL && ! strcasecmp(attr_of_elet_mode->Name(), "target")){
                target = to_lower(attr_of_elet_mode->Value());
                ALOGI("target: %s", target.c_str());
            } else if (attr_of_elet_mode->Name() != NULL && ! strcasecmp(attr_of_elet_mode->Name(), "capability")){
                capability = to_lower(attr_of_elet_mode->Value());
                ALOGD("capability: %s", capability.c_str());
            } else if (attr_of_elet_mode->Name() != NULL && ! strcasecmp(attr_of_elet_mode->Name(), "value")){
                value = to_lower(attr_of_elet_mode->Value());
                ALOGD("value: %s", value.c_str());
            } else if (attr_of_elet_mode->Name() != NULL && ! strcasecmp(attr_of_elet_mode->Name(), "opcode_value")){
                string list_str = attr_of_elet_mode->Value();
                ALOGD("opcode_value: %s", list_str.c_str());

                char *tok, *outer_ptr;
                int i = 0;
                for (tok = strtok_r(const_cast<char*>(list_str.c_str()), ", ", &outer_ptr); tok; tok = strtok_r(NULL, ", ", &outer_ptr)){
                    //The first value should be opcodes' number.
                    if ( i==0 ){
                        num_args = atoi(tok);
                        ALOGD("opcodes' size: %d", num_args);
                        if ( num_args > 0){
                            opcodes = new int32_t[num_args];
                        }
                    } else if ( i > 0 && num_args > 0){
                        ALOGD("opcode[%d]: %s", i-1, tok);
                        opcodes[i-1] = strtoul(tok, NULL, 16);
                    }
                    i++;
                }
            }
            attr_of_elet_mode = attr_of_elet_mode->Next();
        }

        if ( !target.empty() && !capability.empty() && !value.empty()){
            string cap_map_key;
            cap_map_key.append(target).append(CAP_KEY_DELI).append(capability).append(CAP_KEY_DELI).append(value);
            string tar_cap_map_key;
            tar_cap_map_key.append(target).append(CAP_KEY_DELI).append(capability);
            cur_cap = new boost_cap_info(target, capability, value, opcodes, num_args);
            (*cap_map)[cap_map_key] = cur_cap;
            (*tar_cap_map)[tar_cap_map_key] = "";
        }

        elmt_mode = elmt_mode->NextSiblingElement();
    }
    doc_xml.Clear();
}

/** get specified cpu's current freq by sysnode scaling_max_freq */
int32_t get_cpu_cur_freq(int32_t cpu_no) {
    int32_t cur_freq;
    char sys_node[100];
    snprintf(sys_node, sizeof(sys_node), CPU_CUR_FREQ_SYS_NODE, cpu_no);

    FILE *file_cur_freq = NULL;
    file_cur_freq = fopen(sys_node, "r");
    if (file_cur_freq != NULL){
        char hold_cur_freq[20];
        fread(hold_cur_freq, 1, 19, file_cur_freq);
        fclose(file_cur_freq);
        cur_freq = atoi(hold_cur_freq);
        ALOGD("get cpu%d 's current freq: %d", cpu_no, cur_freq);
    } else {
        ALOGE("Failed to get cpu%d 's current freq", cpu_no);
    }

    return cur_freq;
}

/** get specified cpu's max freq by sysnode scaling_max_freq */
int32_t get_cpu_max_freq(int32_t cpu_no){
    int32_t max_freq;
    char sys_node[100];
    snprintf(sys_node, sizeof(sys_node), CPU_MAX_FREQ_SYS_NODE, cpu_no);

    FILE *file_max_freq = NULL;
    file_max_freq = fopen(sys_node, "r");
    if (file_max_freq != NULL){
        char hold_max_freq[20];
        fread(hold_max_freq, 1, 19, file_max_freq);
        fclose(file_max_freq);
        max_freq = atoi(hold_max_freq);
        ALOGD("get cpu%d 's max freq: %d", cpu_no, max_freq);
    } else {
        ALOGE("Failed to get cpu%d 's max freq", cpu_no);
    }

    return max_freq;
}

/** Set all cpus' min freq as 1GHz in platfromBoost */
int32_t get_cpu_min_freq(int32_t cpu_no){
    int32_t min_freq;

    min_freq = CPU_PERFORMANCE_RANK_BASE_FREQ;
    ALOGD("get cpu%d 's min freq: %d", cpu_no, min_freq);

    return min_freq;
}

int32_t* rank_cpu_freq(int32_t min_freq, int32_t max_freq, int32_t rank_num){
    int32_t* ranks = new int32_t[rank_num];

    ALOGI("rank cpu's freq: min freq: %d, max freq: %d, rank num: %d ", min_freq, max_freq, rank_num);
    if(min_freq <= 0 || max_freq <=0 || rank_num <= 0 || min_freq >= max_freq){
        ALOGE("incorrect params");
        return NULL;
    }

    int32_t average = (max_freq - min_freq)/(rank_num-1);
    for(int32_t index = 0; index < rank_num; index++){
        ranks[index] = min_freq + index * average;
    }
    ranks[rank_num-1] = max_freq;

    return ranks;
}

int32_t get_cpu_cur_freq_level(int32_t cpu_no){
    int32_t cur_freq = get_cpu_cur_freq(cpu_no);

    //Binary search
    int32_t cpu_min_freq = get_cpu_min_freq(cpu_no);
    int32_t cpu_max_freq = get_cpu_max_freq(cpu_no);
    int32_t* freq_ranks = rank_cpu_freq(cpu_min_freq, cpu_max_freq, CPU_FREQ_RANK);

    if (sizeof(freq_ranks) > 0){
        int low = 0;
        int high = CPU_FREQ_RANK - 1;
        int mid = 0;

        if (cur_freq <= freq_ranks[low]){
            ALOGD("freq level: %d", low + 1);
            return low + 1;
        } else if (cur_freq >= freq_ranks[high]){
            ALOGD("freq level: %d", high + 1);
            return high + 1;
        } else {
            while(low <= high)
            {
                mid = (low + high)/2;
                if (high - low == 1){
                    ALOGI("freq level: %d", high + 1);
                    return high + 1;
                }
                if (freq_ranks[mid] <= cur_freq){
                    low = mid + 1;
                } else {
                    high=mid-1;
                }
            }
        }
    }
    return -1;
}

string to_lower(string org_str){
    if (org_str.empty()){
        ALOGE("Original string is Null");
        return org_str;
    }

    transform(org_str.begin(), org_str.end(), org_str.begin(), ::tolower);
    return org_str;
}

void init_inline_mode_data(unordered_map<string, int32_t> *cpu_cluster_map,
    unordered_map<string, int32_t> *inline_mode_opcode_map,
    unordered_map<string, int32_t> *inline_mode_value_map){

    cpu_cluster_map->clear();
    inline_mode_opcode_map->clear();
    inline_mode_value_map->clear();

    //dynamic get cpu cluster info
    get_cpu_cluster_info(cpu_cluster_map);

    //init opcodes of inline mode
    (*inline_mode_opcode_map)[INLINE_MODE_CPU_SMALL_MIN] = MPCTLV3_MIN_FREQ_CLUSTER_LITTLE_CORE_0;
    (*inline_mode_opcode_map)[INLINE_MODE_CPU_SMALL_MAX] = MPCTLV3_MAX_FREQ_CLUSTER_LITTLE_CORE_0;
    (*inline_mode_opcode_map)[INLINE_MODE_CPU_BIG_MIN] = MPCTLV3_MIN_FREQ_CLUSTER_BIG_CORE_0;
    (*inline_mode_opcode_map)[INLINE_MODE_CPU_BIG_MAX] = MPCTLV3_MAX_FREQ_CLUSTER_BIG_CORE_0;
    (*inline_mode_opcode_map)[INLINE_MODE_CPU_PRIME_MIN] = MPCTLV3_MIN_FREQ_CLUSTER_PRIME_CORE_0;
    (*inline_mode_opcode_map)[INLINE_MODE_CPU_PRIME_MAX] = MPCTLV3_MAX_FREQ_CLUSTER_PRIME_CORE_0;
    (*inline_mode_opcode_map)[INLINE_MODE_SCHED_BOOST] = MPCTLV3_SCHED_BOOST;
    (*inline_mode_opcode_map)[INLINE_MODE_LPM_DISABLE] = MPCTLV3_TOGGLE_POWER_COLLAPSE;
    (*inline_mode_opcode_map)[INLINE_MODE_STORAGE_BOOST] = MPCTLV3_STORAGE_CLK_SCALING_DISABLE;

    //init some real values of inline mode abstract value.
    if (cpu_cluster_map->empty()){
        ALOGE("Don't exist InlineMode items in boost_mode.xml");
        return;
    }

    unordered_map <string, int32_t>::iterator iter;
    for (iter = (*cpu_cluster_map).begin(); iter != (*cpu_cluster_map).end(); iter++ ){
        string cpu_cluster = iter->first;
        int32_t cpu_no =  iter->second;

        int32_t cpu_min_freq = get_cpu_min_freq(cpu_no);
        int32_t cpu_max_freq = get_cpu_max_freq(cpu_no);
        int32_t* cpu_freq_ranks = rank_cpu_freq(cpu_min_freq, cpu_max_freq, CPU_FREQ_RANK);

        string key_min;
        string key_max;
        if (! strcasecmp(cpu_cluster.c_str(), CPU_SMALL_CLUSTER)) {
            key_min.append(INLINE_MODE_CPU_SMALL_MIN).append(INLINE_MODE_VALUE_MAP_DELIMITER).append(CAP_VALUE_LEVEL);
            key_max.append(INLINE_MODE_CPU_SMALL_MAX).append(INLINE_MODE_VALUE_MAP_DELIMITER).append(CAP_VALUE_LEVEL);
        } else if(! strcasecmp(cpu_cluster.c_str(), CPU_BIG_CLUSTER)) {
            key_min.append(INLINE_MODE_CPU_BIG_MIN).append(INLINE_MODE_VALUE_MAP_DELIMITER).append(CAP_VALUE_LEVEL);
            key_max.append(INLINE_MODE_CPU_BIG_MAX).append(INLINE_MODE_VALUE_MAP_DELIMITER).append(CAP_VALUE_LEVEL);
        } else if(! strcasecmp(cpu_cluster.c_str(), CPU_PRIME_CLUSTER)) {
            key_min.append(INLINE_MODE_CPU_PRIME_MIN).append(INLINE_MODE_VALUE_MAP_DELIMITER).append(CAP_VALUE_LEVEL);
            key_max.append(INLINE_MODE_CPU_PRIME_MAX).append(INLINE_MODE_VALUE_MAP_DELIMITER).append(CAP_VALUE_LEVEL);
        } else{
            return;
        }

        for(int32_t index = 0; index < CPU_FREQ_RANK; index++){
            int32_t value = (cpu_freq_ranks[index]/1000); // convert to MHZ
            string key_min_name = key_min + std::to_string(index + 1);
            string key_max_name = key_max + std::to_string(index + 1);
            (*inline_mode_value_map)[key_min_name] = value;
            (*inline_mode_value_map)[key_max_name] = value;
        }
    }
}

void get_cpu_cluster_info(unordered_map<string, int32_t> *cpu_cluster_map){
    int32_t cpu_big_no = -1;
    int32_t cpu_prime_no = -1;

    cpu_cluster_map->clear();
    (*cpu_cluster_map)[CPU_SMALL_CLUSTER] = 0;
    ALOGD("cpu small number: 0");

    //Get big cpu cluster's first cpu number
    char small_related[100];
    snprintf(small_related, sizeof(small_related), CPU_RELATED_CPUS_SYS_NODE, 0);

    ifstream small_in(small_related, std::ifstream::in);
    if(small_in){
        string line;
        getline(small_in, line, '\n');
        ALOGD("cpu small cluster related: %s", line.c_str());
        vector<char*> *cpus = new vector<char*>();
        split_string(const_cast<char*>(line.c_str()), *cpus, " ");
        cpu_big_no = atoi(cpus->back()) + 1;
        ALOGD("cpu big number: %d", cpu_big_no);
        (*cpu_cluster_map)[CPU_BIG_CLUSTER] = cpu_big_no;
    }
    small_in.close();

    if (cpu_big_no > 0){
        //Try to get prime cpu cluster's first cpu number
        char big_related[100];
        snprintf(big_related, sizeof(big_related), CPU_RELATED_CPUS_SYS_NODE, cpu_big_no);

        ifstream big_in(big_related, std::ifstream::in);
        if(big_in){
            string line;
            getline(big_in, line, '\n');
            ALOGD("cpu big cluster related: %s", line.c_str());
            vector<char*> *cpus = new vector<char*>();
            split_string(const_cast<char*>(line.c_str()), *cpus, " ");
            cpu_prime_no = atoi(cpus->back()) + 1;
            ALOGD("cpu primer number: %d", cpu_prime_no);
        }
        big_in.close();

        if(cpu_prime_no > 0){
            char prime_related[100];
            snprintf(prime_related, sizeof(prime_related), CPU_RELATED_CPUS_SYS_NODE, cpu_prime_no);

            ifstream prime_in(prime_related, std::ifstream::in);
            if(prime_in){
                (*cpu_cluster_map)[CPU_PRIME_CLUSTER] = cpu_prime_no;
            } else {
                ALOGE("Dont exist cpu primer core");
            }
            prime_in.close();
        }
    }
}

void split_string(char* content, vector<char*>& result_vec, const char* delimiter){
    if (strlen(content) == 0){
        ALOGE("Original content is Null");
        return ;
    }

    char* p_token = NULL;
    char* p_save = NULL;
    p_token = strtok_r(content, delimiter, &p_save);
    while(p_token){
        //ALOGI("split string: %s",p_token);
        result_vec.push_back(p_token);
        p_token = strtok_r(NULL, delimiter, &p_save);
    }
}

void string_replase(string &str, string old_str, string new_str){
    string::size_type pos = 0;
    string::size_type old_t = old_str.size();
    string::size_type new_t = new_str.size();
    while ((pos = str.find(old_str,pos)) != string::npos){
        str.replace(pos, old_t, new_str);
        pos += new_t;
    }
}