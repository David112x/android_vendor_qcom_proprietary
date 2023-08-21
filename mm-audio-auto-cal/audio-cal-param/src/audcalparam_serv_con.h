/**
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
 
#ifndef AUDCALPARAM_SERV_CON_H
#define AUDCALPARAM_SERV_CON_H
// #include "audcalparam_api.h"
#include "audcalparam_api_priv.h"

void * audcalparam_serv_con_init(char *snd_card_name);

int32_t audcalparam_serv_con_param_set(void *h, audcalparam_cmd_tunnel_cfg_t* cfg, uint8_t *pbuf, uint32_t *pbuf_len);
int32_t audcalparam_serv_con_param_get(void *h, audcalparam_cmd_tunnel_cfg_t* cfg, uint8_t *ppbuf, uint32_t *pbuf_len, uint8_t get_buf_pars_en);

int32_t audcalparam_serv_con_get_asm_topology(void *h, struct audio_cal_asm_top *asm_top);

int32_t audcalparam_serv_con_get_adm_topology(void *h, struct audio_cal_adm_top *adm_top);

int32_t audcalparam_serv_con_close(void *h);

#endif