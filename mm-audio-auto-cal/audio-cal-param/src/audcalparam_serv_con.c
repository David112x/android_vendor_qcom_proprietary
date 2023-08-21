/**
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "audcalparam_serv_con.h"
#include "audcalparam_api.h"

#ifdef USE_LIBACDBLOADER

#define LIB_ACDB_LOADER "libacdbloader.so"
typedef int  (*acdb_init_t)(const char *, const char *, int);
typedef void (*acdb_deinit_t)(void);
typedef int (*acdb_get_cal_t)(void *caldata, void* data, unsigned int *datalen);
typedef int (*acdb_set_cal_t)(void *caldata, void* data, unsigned int datalen);
typedef int (*acdb_get_asm_top_t)(struct audio_cal_asm_top *audstrm_top);
typedef int (*acdb_get_adm_top_t)(struct audio_cal_adm_top *adm_top);

#endif

typedef struct {
    void *con;
    void *acdb_handle;
#ifdef USE_LIBACDBLOADER
    acdb_get_cal_t acdb_get_cal;
    acdb_set_cal_t acdb_set_cal;
    acdb_get_asm_top_t acdb_get_asm_top;
    acdb_get_adm_top_t acdb_get_adm_top;
    acdb_deinit_t acdb_deinit;
#endif
}audcalparam_serv_con_t;

void *audcalparam_serv_con_init(char *snd_card_name){

    audcalparam_serv_con_t * sh = NULL;

#ifdef USE_LIBACDBLOADER
    //init con-handle
    int32_t r=AUDCALPARAM_SERVCONERR;
    fprintf(stdout,"%s for sndcard=%s\n", __func__,snd_card_name);
    sh = malloc(sizeof(audcalparam_serv_con_t));
    acdb_init_t acdb_init;
    sh->acdb_handle = dlopen(LIB_ACDB_LOADER, RTLD_NOW);
    if (sh->acdb_handle == NULL) {
        fprintf(stderr,"%s: ERROR. dlopen failed for %s\n", __func__, LIB_ACDB_LOADER);
        goto cleanup;
    }

    acdb_init = (acdb_init_t)dlsym(sh->acdb_handle,"acdb_loader_init_v2");
    if (acdb_init == NULL) {
        fprintf(stderr,"%s: dlsym error %s for acdb_init\n", __func__, dlerror());
    goto cleanup;
    }

    sh->acdb_deinit = (acdb_deinit_t)dlsym(sh->acdb_handle, "acdb_loader_deallocate_ACDB");
    if (sh->acdb_deinit == NULL) {
        fprintf(stderr,"%s: dlsym error %s for acdb_loader_deallocate_ACDB\n", __func__, dlerror());
        goto cleanup;
    }
    
    sh->acdb_get_cal = (acdb_get_cal_t)dlsym(sh->acdb_handle, "acdb_loader_get_audio_cal_v2");
    if (sh->acdb_get_cal == NULL) {
        fprintf(stderr,"%s: ERROR. dlsym Error:%s acdb_loader_get_audio_cal_v2\n", __func__,dlerror());
        goto cleanup;
    }

    sh->acdb_set_cal = (acdb_set_cal_t)dlsym(sh->acdb_handle, "acdb_loader_set_audio_cal_v2");
    if (sh->acdb_set_cal == NULL) {
        fprintf(stderr,"%s: ERROR. dlsym Error:%s acdb_loader_set_audio_cal_v2\n", __func__, dlerror());
        goto cleanup;
    }
    sh->acdb_get_asm_top = (acdb_get_asm_top_t)dlsym(sh->acdb_handle, "acdb_loader_get_asm_topology");
    if (sh->acdb_get_asm_top == NULL) {
        fprintf(stderr,"%s: ERROR. dlsym Error:%s acdb_loader_get_asm_topology\n", __func__, dlerror());
        goto cleanup;
    }
    
    sh->acdb_get_adm_top = (acdb_get_adm_top_t)dlsym(sh->acdb_handle, "acdb_loader_get_adm_topology");
    if (sh->acdb_get_adm_top == NULL) {
        fprintf(stderr,"%s: ERROR. dlsym Error:%s acdb_loader_get_adm_topology\n", __func__, dlerror());
        goto cleanup;
    }

    r=acdb_init(snd_card_name, NULL, 0);

    if (r){
        fprintf(stderr,"acdb_loader_init for %s returned %d\n",snd_card_name,r);
        goto cleanup;
    }
    goto exit;
cleanup:
    if (sh->acdb_handle)
        dlclose(sh->acdb_handle);
    free(sh);
    sh=NULL;
#endif
exit:
    return (void*)sh;
}


int32_t audcalparam_serv_con_param_set(void *h, audcalparam_cmd_tunnel_cfg_t *cfg, uint8_t *pbuf, uint32_t *pbuf_len){
    
    audcalparam_serv_con_t * sh=(audcalparam_serv_con_t *)h;
    int32_t r=AUDCALPARAM_SERVCONERR;
    if (sh==NULL || sh->acdb_set_cal==NULL)
        goto exit;  
#ifdef USE_LIBACDBLOADER
#ifdef AUDCALPARAM_DBG  
    fprintf(stdout,"tunnel_cmd->persist=%d\n",cfg->persist);
    fprintf(stdout,"tunnel_cmd->sampling_rate=%d\n",cfg->sampling_rate);
    fprintf(stdout,"tunnel_cmd->acdb_dev_id=%d\n",cfg->acdb_dev_id);
    fprintf(stdout,"tunnel_cmd->topo=%d\n",cfg->topo_id);
    fprintf(stdout,"tunnel_cmd->app_type=%d\n",cfg->app_type);
    fprintf(stdout,"tunnel_cmd->cal_type=%d\n",cfg->cal_type);
    fprintf(stdout,"tunnel_cmd->module_id=%d\n",cfg->module_id);
    fprintf(stdout,"tunnel_cmd->instance_id=%d\n",cfg->instance_id);
    fprintf(stdout,"tunnel_cmd->reserved=%d\n",cfg->reserved);
    fprintf(stdout,"tunnel_cmd->param_id=%d\n",cfg->param_id);
#endif
    r=sh->acdb_set_cal(cfg, pbuf, *pbuf_len);
#ifdef AUDCALPARAM_DBG  
    if (*pbuf_len==4)
        fprintf(stdout,"%s:%d\n",__func__,*(uint32_t*)pbuf);
#endif  
#endif
exit:
    return r; 
}
#define AUDCAL_GET_BUF_MOD_ID_POS 0
#define AUDCAL_GET_BUF_INST_ID_POS (AUDCAL_GET_BUF_MOD_ID_POS+sizeof(int32_t))
#define AUDCAL_GET_BUF_PARAMID_POS (AUDCAL_GET_BUF_INST_ID_POS+sizeof(int32_t))
#define AUDCAL_GET_BUF_PAYLOAD_LEN_POS (AUDCAL_GET_BUF_PARAMID_POS+sizeof(int32_t))
#define AUDCAL_GET_BUF_PAYLOAD_POS (AUDCAL_GET_BUF_PAYLOAD_LEN_POS+sizeof(int32_t))

int32_t audcalparam_serv_con_param_get(void *h, audcalparam_cmd_tunnel_cfg_t *cfg, uint8_t *pbuf, uint32_t *pbuf_len,  uint8_t get_buf_pars_en){
    
    // fprintf(stdout,"%s\n", __func__);
    audcalparam_serv_con_t * sh=(audcalparam_serv_con_t *)h;
    int32_t r=AUDCALPARAM_SERVCONERR;
    if (sh==NULL || sh->acdb_get_cal==NULL)
        goto exit;
#ifdef USE_LIBACDBLOADER
    // uint8_t *pbuf=*ppbuf;
#ifdef AUDCALPARAM_DBG  
    fprintf(stdout,"tunnel_cmd->persist=%d\n",cfg->persist);
    fprintf(stdout,"tunnel_cmd->sampling_rate=%d\n",cfg->sampling_rate);
    fprintf(stdout,"tunnel_cmd->acdb_dev_id=%d\n",cfg->acdb_dev_id);
    fprintf(stdout,"tunnel_cmd->topo_id=%d\n",cfg->topo_id);
    fprintf(stdout,"tunnel_cmd->app_type=%d\n",cfg->app_type);
    fprintf(stdout,"tunnel_cmd->cal_type=%d\n",cfg->cal_type);
    fprintf(stdout,"tunnel_cmd->module_id=%d\n",cfg->module_id);
    fprintf(stdout,"tunnel_cmd->instance_id=%d\n",cfg->instance_id);
    fprintf(stdout,"tunnel_cmd->reserved=%d\n",cfg->reserved);
    fprintf(stdout,"tunnel_cmd->param_id=%d\n",cfg->param_id);  
#endif
    r=sh->acdb_get_cal(cfg, pbuf, pbuf_len);
#ifdef AUDCALPARAM_DBG  
    fprintf(stdout,"Buf len=%d ret=%d\n",*pbuf_len,r);
    int32_t i;
    for (i=0;i<*pbuf_len;i++){
        fprintf(stdout,"%02x",pbuf[i]);
    }
    printf("\n");
#endif
    if (r == AUDCALPARAM_OK && get_buf_pars_en && cfg->persist==0){
        if (*pbuf_len>AUDCAL_GET_BUF_PAYLOAD_POS){
            // parsing only when persist NOT set
            uint32_t modid=0;
            uint32_t instid=0;
            uint32_t parid=0;
            uint32_t payloadlen=0;
            // uint32_t payload_len=0;
            // uint8_t *payload;
            modid=*(uint32_t*)(&pbuf[AUDCAL_GET_BUF_MOD_ID_POS]);
            instid=*(uint32_t*)(&pbuf[AUDCAL_GET_BUF_INST_ID_POS]);
            parid=*(uint32_t*)(&pbuf[AUDCAL_GET_BUF_PARAMID_POS]);
            // return only payload
            payloadlen=*(uint32_t*)(&pbuf[AUDCAL_GET_BUF_PAYLOAD_LEN_POS]);//return len of the payload only
#ifdef AUDCALPARAM_DBG
            fprintf(stdout,"%s parsing: modid=%d, instid=%d, parid=%d, payloadlen=%d\n",__func__,modid,instid,parid,payloadlen);
#endif
            if (payloadlen>0 && payloadlen+AUDCAL_GET_BUF_PAYLOAD_POS==*pbuf_len){
                // *ppbuf=(uint8_t*)&pbuf[AUDCAL_GET_BUF_PAYLOAD_POS];
                if (payloadlen<AUDCAL_GET_BUF_PAYLOAD_POS){
                    //null first bytes
                    memset(&pbuf[0],0,AUDCAL_GET_BUF_PAYLOAD_POS);
                }
                memmove(&pbuf[0], &pbuf[AUDCAL_GET_BUF_PAYLOAD_POS], payloadlen);
                *pbuf_len=payloadlen;
            }else{
                *pbuf_len=0;
                fprintf(stderr,"Wrong payload length %d for get buffer!\n",payloadlen);
            }
        }
        else
            fprintf(stderr,"Provided buffer not long enough. Parsing format for get buffer not known!\n");
    }

#endif
exit:
    return r;
}

int32_t audcalparam_serv_con_get_asm_topology(void *h, struct audio_cal_asm_top *asm_top){
    audcalparam_serv_con_t * sh=(audcalparam_serv_con_t *)h;
    int32_t r=AUDCALPARAM_SERVCONERR;
    if (sh==NULL || sh->acdb_get_asm_top==NULL)
        goto exit;
#ifdef USE_LIBACDBLOADER
    r=sh->acdb_get_asm_top(asm_top);
#endif
exit:
    return r;
}

int32_t audcalparam_serv_con_get_adm_topology(void *h, struct audio_cal_adm_top *adm_top){
    audcalparam_serv_con_t * sh=(audcalparam_serv_con_t *)h;
    int32_t r=AUDCALPARAM_SERVCONERR;
    if (sh==NULL || sh->acdb_get_adm_top==NULL)
        goto exit;
#ifdef USE_LIBACDBLOADER
    r=sh->acdb_get_adm_top(adm_top);
#endif
exit:
    return r;
}

int32_t audcalparam_serv_con_close(void *h){
    
    audcalparam_serv_con_t * sh=(audcalparam_serv_con_t *)h;
    int32_t r=AUDCALPARAM_SERVCONERR;
    if (sh==NULL || sh->acdb_deinit==NULL)
        goto exit;
#ifdef USE_LIBACDBLOADER
        sh->acdb_deinit();
        dlclose(sh->acdb_handle);
        r=0;
#endif
    if (sh!=NULL)
        free(sh);
exit:
    return r;
}