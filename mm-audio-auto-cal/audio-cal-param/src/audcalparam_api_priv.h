/**
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
/*! Private */
#ifndef AUDCALPARAM_API_PRIV_H
#define AUDCALPARAM_API_PRIV_H
#include "../inc/audcalparam_api.h"
#include <linux/msm_audio_calibration.h>
#include <json-c/json.h>
#include "acdb-loader.h"
typedef struct _audcalparam_usecase_t{
    uint32_t acdb_dev_id;
    uint32_t app_type;
    uint32_t topo_id;
    uint32_t type;
    char * name;
    struct _audcalparam_usecase_t* next;
}audcalparam_usecase_t;

typedef struct {
    int32_t use_case_num;
    audcalparam_usecase_t* usecase_list;
}audcalparam_usecase_list_t;

typedef  acdb_audio_cal_cfg_t audcalparam_cmd_tunnel_cfg_t;

typedef struct _audcalparam_command_instance_t{
    char * inst_name;
    struct _audcalparam_command_instance_t * nxt_command_instance;
    char* use_case_name;
    uint32_t topo_id;
    uint32_t cal_type;
    uint32_t module_id;
    uint16_t inst_id;
    uint16_t reserved;
    uint32_t param_id;
    uint8_t *parambuf;
    uint32_t parambuflen;
}audcalparam_command_instance_t;

typedef struct {
    char * type_name;
    uint8_t inst_num;
    audcalparam_command_instance_t * command_instance_list;
}audcalparam_command_t;

typedef struct {
    audcalparam_command_t *cmd_bmt;
    audcalparam_command_t *cmd_delay;
    audcalparam_command_t *cmd_fnb;
    audcalparam_command_t *cmd_mute;
    audcalparam_command_t *cmd_volume_idx;
    audcalparam_command_t *cmd_gain;
    audcalparam_command_t *cmd_module_param;
}audcalparam_command_set_t;

#endif