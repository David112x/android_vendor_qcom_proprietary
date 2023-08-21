/**
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include <strings.h>
#include <sys/stat.h>

#include "audcalparam_api_priv.h"
// #include "audcalparam_api.h"
#include "audcalparam_serv_con.h"

#define TEST_PROLOG(type)\
    if (type==0) \
    {\
        system("paplay --raw /dev/urandom &");  \
        fprintf(stdout,"***Playback started. press any key to run the test***\n"); \
        scanf("%c",&ch); \
    }

#define TEST_EPILOG(type)\
    if (type==0) \
    {\
        fprintf(stdout,"Press any key to finish\n"); \
        scanf("%c",&ch); \
        system("kill `pidof paplay`"); \
    }

#define SET_TEST_TYPE(type) \
    type=0; \
    if (argc==3 && !strcmp(argv[2],"1")) \
    { \
        type=1; \
    } 


int main(int argc, char* argv[]){

    char* fname="/etc/audcalparam_commands.cfg";
    char *snd_card_name = "apq8096-adp-agave-snd-card";// snd card name!
    struct stat fstatus;

    if (stat(fname, &fstatus) != 0) {
        fprintf(stderr, "Cfg file %s not found\n", fname);
        return 1;
    }

    fprintf(stdout, "Start %s parsing cfg file\n",fname);
    
    audcalparam_session_t *h;
    audcalparam_err_t r, r1;

    r = audcalparam_session_init(&h, fname, snd_card_name);
    fprintf(stdout,"Session init ret %d\n",r);
    if (r!=AUDCALPARAM_OK){
        goto exit;
    }

    r = audcalparam_usecase_list_print(h);  
    fprintf(stdout,"Usecase list print ret %d\n",r);

    r = audcalparam_cmd_set_print(h);
    fprintf(stdout,"CMD set print ret %d\n",r);
    char ch;
    uint32_t cache_flag;
    // test command
    audcalparam_cmd_module_param_cfg_t mpcfg;
    mpcfg.base_cfg.sampling_rate=48000;// set sampling_rate
    audcalparam_cmd_base_cfg_t bcfg;
    bcfg.sampling_rate=48000;// set sampling_rate
    SET_TEST_TYPE(cache_flag)
    TEST_PROLOG(cache_flag)
    if (argc==1){
        fprintf(stdout,"***Please run testapi <test-number> <cache-flag>***\n");
        fprintf(stdout,"***cache-flag: 0- runtime test, 1-test for caching***\n");
    } else if (argc>=2 && !strcmp(argv[1],"1")) {
        fprintf(stdout,"***Test for Module_param***\n");

        mpcfg.base_cfg.cache=cache_flag;

        mpcfg.module_id=0x00010C37;
        mpcfg.param_id=0x00010C38;

        int32_t gain;
        uint8_t buf[64];
        uint8_t *pbuf=buf;
        int32_t len=sizeof(buf);

        r = audcalparam_cmd_module_param(h, "copp_general_playback",  AUDCALPARAM_GET, pbuf, &len, &mpcfg);
        *(int32_t*)&pbuf[0]=*(int32_t*)&pbuf[0]+(int32_t)1;
        gain=*(int32_t*)&pbuf[0];
        len=sizeof(gain);
        r |= audcalparam_cmd_module_param(h, "copp_general_playback",  AUDCALPARAM_SET, (uint8_t*)&gain, &len, &mpcfg);
        len=sizeof(buf);
        r |= audcalparam_cmd_module_param(h, "copp_general_playback",  AUDCALPARAM_GET, pbuf, &len, &mpcfg);
        fprintf(stdout,"Test ret %d, len=%d, gain=0x%x\n",r,len,*(int32_t*)&pbuf[0]);
        if (r==AUDCALPARAM_OK && *(int32_t*)&pbuf[0]==gain)
            fprintf(stdout,"***TEST OK ***\n");
        else
            fprintf(stdout,"***TEST NOK ***\n");

    } else if (argc>=2 && !strcmp(argv[1],"2")) {
        fprintf(stdout,"***Run BMT GET-incrSET-GET test***\n");

        bcfg.cache=cache_flag;
        
        audcalparam_cmd_bmt_t bmt_val={0};
        bmt_val.filter_mask=0x7;//all fields
        
        r = audcalparam_cmd_bmt(h, "BMT_auto_amp_general_playback", AUDCALPARAM_GET, &bmt_val, &bcfg);
        fprintf(stdout,"BMT Get ret %d:%d %d %d\n",r,bmt_val.bass,bmt_val.mid,bmt_val.treble);
#if 1   
        audcalparam_cmd_bmt_t tmp;
        bmt_val.bass+=1;
        bmt_val.mid+=1;
        bmt_val.treble+=1;
        memcpy(&tmp, &bmt_val, sizeof(bmt_val));
        r = audcalparam_cmd_bmt(h, "BMT_auto_amp_general_playback", AUDCALPARAM_SET, &bmt_val, &bcfg);
        r = audcalparam_cmd_bmt(h, "BMT_auto_amp_general_playback", AUDCALPARAM_GET, &bmt_val, &bcfg);
        fprintf(stdout,"BMT Get after incr ret %d:%d %d %d\n",r,bmt_val.bass,bmt_val.mid,bmt_val.treble);
        if (r==AUDCALPARAM_OK) {
            if (bmt_val.bass==tmp.bass && bmt_val.mid==tmp.mid && bmt_val.treble==tmp.treble){
                fprintf(stdout,"***TEST OK ***\n");
            }
            else {
                fprintf(stdout,"***TEST NOK ***\n");
            }
        }
#endif

    } else if (argc>=2 && !strcmp(argv[1],"3")) {
        fprintf(stdout,"***Run FnB GET-incrSET-GET test***\n");

        bcfg.cache=cache_flag;

        audcalparam_cmd_fnb_t fnb_val={0};
        fnb_val.filter_mask=0x3;//all fields

        r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_GET, &fnb_val, &bcfg);
        fprintf(stdout,"FnB Get ret %d:%d %d\n",r,fnb_val.balance,fnb_val.fade);
        fnb_val.balance+=1;
        fnb_val.fade+=1;
        audcalparam_cmd_fnb_t tmp;
        fnb_val.filter_mask=0x3;//all fields
        memcpy(&tmp, &fnb_val, sizeof(fnb_val));
        r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_SET, &fnb_val, &bcfg);
        r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_GET, &fnb_val, &bcfg);
        fprintf(stdout,"FnB Get after incr ret %d:%d %d\n",r,fnb_val.balance,fnb_val.fade);
        if (r==AUDCALPARAM_OK) {
            if (fnb_val.balance==tmp.balance && fnb_val.fade==tmp.fade){
                fprintf(stdout,"***TEST OK ***\n");
            }
            else {
                fprintf(stdout,"***TEST NOK ***\n");
            }
        }

    } else if (argc>=2 && !strcmp(argv[1],"4")) {
        fprintf(stdout,"***Run VolIdx test***\n");
        audcalparam_cmd_volume_idx_t volume_idx={0};
        volume_idx.num_el=AUDCALPARAM_CMD_VOL_IDX_EL_NUM_MAX;
        uint32_t idx=0;
        volume_idx.value=&idx;
        bcfg.cache=cache_flag;

        volume_idx.value[0]=15;//half the gain

        r = audcalparam_cmd_volume_idx(h, "popp_volume_idx_auto", AUDCALPARAM_SET, &volume_idx, &bcfg);
        fprintf(stdout,"***SET VolId ret %d.***\n",r);

    }  else if (argc>=2 && !strcmp(argv[1],"5")) {

        fprintf(stdout,"***Run Gain test ***\n");

        bcfg.cache=cache_flag;

        int32_t i;
        audcalparam_cmd_gain_t gain_val={0};
        gain_val.num_el=AUDCALPARAM_CMD_GAIN_EL_NUM_MAX;
        uint32_t gain[AUDCALPARAM_CMD_GAIN_EL_NUM_MAX] = {0x0};
        uint32_t type[AUDCALPARAM_CMD_GAIN_EL_NUM_MAX] = {0x0};
        gain_val.value = gain;
        gain_val.type = type;

        r = audcalparam_cmd_gain(h, "popp_gain_auto", AUDCALPARAM_GET, &gain_val, &bcfg);
        fprintf(stdout,"Gain Get ret %d:len=%d,[0]=%d\n",r,gain_val.num_el,gain_val.value[0]);
#if 1
        for (i=0;i<gain_val.num_el;i++)
            gain_val.value[i]=gain_val.value[i]>>1;
        uint32_t tmp[AUDCALPARAM_CMD_GAIN_EL_NUM_MAX];
        memcpy(tmp, gain_val.value, gain_val.num_el*sizeof(tmp[0]));
        r = audcalparam_cmd_gain(h, "popp_gain_auto", AUDCALPARAM_SET, &gain_val, &bcfg);
        fprintf(stdout,"Set ret %d\n",r);
        gain_val.num_el=AUDCALPARAM_CMD_GAIN_EL_NUM_MAX;
        r = audcalparam_cmd_gain(h, "popp_gain_auto", AUDCALPARAM_GET, &gain_val, &bcfg);
        fprintf(stdout,"Gain Get after incr ret %d:[0]=%d, len=%d\n",r,gain_val.value[0],gain_val.num_el);
        if (r==AUDCALPARAM_OK)
            for (i=0;i<gain_val.num_el;i++){
                if (tmp[i]==gain_val.value[i])
                    fprintf(stdout,"***TEST OK ***\n");
                else
                    fprintf(stdout,"***TEST NOK:expected %d, ret %d ***\n",tmp[i],gain_val.value[i]);
            }
#endif

    } else if (argc>=2 && !strcmp(argv[1],"6")) {
        fprintf(stdout,"***Run Mute test ***\n");
        
        bcfg.cache=cache_flag;
        
        audcalparam_cmd_mute_t mute_val={0};
        mute_val.num_el=AUDCALPARAM_CMD_MUTE_EL_NUM_MAX;
        uint32_t mute[AUDCALPARAM_CMD_MUTE_EL_NUM_MAX] = {0};
        uint32_t type[AUDCALPARAM_CMD_MUTE_EL_NUM_MAX] = {0};
        mute_val.value = mute;
        mute_val.type = type;

        r = audcalparam_cmd_mute(h, "popp_mute_auto", AUDCALPARAM_GET, &mute_val, &bcfg);
        fprintf(stdout,"Mute GET ret %d:[0]=%d, len=%d\n",r,mute_val.value[0],mute_val.num_el);
#if 1
        int32_t i,n=mute_val.num_el;
        for (i=0;i<n;i++){
            mute_val.value[i]=1;
        }
        uint32_t tmp[AUDCALPARAM_CMD_MUTE_EL_NUM_MAX];
        memcpy(tmp, mute_val.value, mute_val.num_el*sizeof(tmp[0]));
        r = audcalparam_cmd_mute(h, "popp_mute_auto", AUDCALPARAM_SET, &mute_val, &bcfg);
        fprintf(stdout,"Mute SET ret %d\n",r);
        mute_val.num_el=AUDCALPARAM_CMD_MUTE_EL_NUM_MAX;
        r = audcalparam_cmd_mute(h, "popp_mute_auto", AUDCALPARAM_GET, &mute_val, &bcfg);
        fprintf(stdout,"Mute GET ret %d\n",r);
        if (r==AUDCALPARAM_OK)
            for (i=0;i<mute_val.num_el;i++){
                if (tmp[i]==mute_val.value[i])
                    fprintf(stdout,"***TEST OK ***\n");
                else
                    fprintf(stdout,"***TEST NOK:expected %d, ret %d ***\n",tmp[i],mute_val.value[i]);
            }
#endif
        
    } else if (argc>=2 && !strcmp(argv[1],"7")) {
        fprintf(stdout,"***Run Delay for test ***\n");

        bcfg.cache=cache_flag;

        int32_t i;
        audcalparam_cmd_delay_t delay_val={0};
        delay_val.num_el=AUDCALPARAM_CMD_DELAY_EL_NUM_MAX;
        uint32_t delay[AUDCALPARAM_CMD_DELAY_EL_NUM_MAX] = {0x0};
        uint32_t mask_lsb[AUDCALPARAM_CMD_DELAY_EL_NUM_MAX] = {0x0};
        uint32_t mask_msb[AUDCALPARAM_CMD_DELAY_EL_NUM_MAX] = {0x0};
        delay_val.value = delay;
        delay_val.mask_msb = mask_msb;
        delay_val.mask_lsb = mask_lsb;

        r = audcalparam_cmd_delay(h, "copp_delay", AUDCALPARAM_GET, &delay_val, &bcfg);
        fprintf(stdout,"Delay Get ret %d:len=%d,[0]=%d\n",r,delay_val.num_el,delay_val.value[0]);
#if 1
        delay_val.num_el=2;
        fprintf(stdout,"Number of delay cfg=%d\n",delay_val.num_el);
        for (i=0;i<delay_val.num_el;i++){
            delay_val.value[i]+=100000;
            delay_val.mask_lsb[i]=i+1;
        }
        uint32_t tmp[AUDCALPARAM_CMD_DELAY_EL_NUM_MAX];
        memcpy(tmp, delay_val.value, delay_val.num_el*sizeof(tmp[0]));
        r = audcalparam_cmd_delay(h, "copp_delay", AUDCALPARAM_SET, &delay_val, &bcfg);
        fprintf(stdout,"Set ret %d\n",r);
        delay_val.num_el=AUDCALPARAM_CMD_DELAY_EL_NUM_MAX;
        r = audcalparam_cmd_delay(h, "copp_delay", AUDCALPARAM_GET, &delay_val, &bcfg);
        fprintf(stdout,"Delay Get after incr ret %d:[0]=%d, len=%d\n",r,delay_val.value[0],delay_val.num_el);
        if (r==AUDCALPARAM_OK)
            for (i=0;i<delay_val.num_el;i++){
                if (tmp[i]==delay_val.value[i])
                    fprintf(stdout,"***TEST OK ***\n");
                else
                    fprintf(stdout,"***TEST NOK:expected %d, ret %d ***\n",tmp[i],delay_val.value[i]);
            }
#endif

    } else if (argc>=2 && !strcmp(argv[1],"8")) {
        fprintf(stdout,"***Run BMT audio test ***\n");
        
        // enable module
        mpcfg.module_id=0x123A2000;
        mpcfg.param_id=0x123A2001;
        mpcfg.base_cfg.cache=cache_flag;
        uint32_t enable=1;
        int32_t len;    
        uint8_t buf[64];
        uint8_t *pbuf=buf;
        len=sizeof(buf);

        r = audcalparam_cmd_module_param(h, "copp_auto_amp_general_playback",  AUDCALPARAM_GET, pbuf, &len, &mpcfg);
        if (r==AUDCALPARAM_OK){
            if (*(uint32_t*)pbuf==0){
                fprintf(stdout,"***BMT is disabled. Enable it. ***\n");
                // enable=1;
                *(uint32_t*)pbuf=1;
                r = audcalparam_cmd_module_param(h, "copp_auto_amp_general_playback",  AUDCALPARAM_SET, pbuf, &len, &mpcfg);
            }
            if (r==AUDCALPARAM_OK){
                bcfg.cache=cache_flag;
            
                audcalparam_cmd_bmt_t bmt_val={0};
                audcalparam_cmd_bmt_t tmp;
                bmt_val.filter_mask=0x7;//all fields
                
                bmt_val.bass=-9;
                bmt_val.mid=-9;
                bmt_val.treble=9;
                fprintf(stdout,"***Setting Treble high. ***\n");
                r = audcalparam_cmd_bmt(h, "BMT_auto_amp_general_playback", AUDCALPARAM_SET, &bmt_val, &bcfg);


                memcpy(&tmp, &bmt_val, sizeof(bmt_val));
                r = audcalparam_cmd_bmt(h, "BMT_auto_amp_general_playback", AUDCALPARAM_GET, &bmt_val, &bcfg);
                fprintf(stdout,"BMT Get after SET ret %d:%d %d %d\n",r,bmt_val.bass,bmt_val.mid,bmt_val.treble);
                if (r==AUDCALPARAM_OK) {
                    if (bmt_val.bass==tmp.bass && bmt_val.mid==tmp.mid && bmt_val.treble==tmp.treble){
                        fprintf(stdout,"***TEST OK ***\n");
                    }
                    else {
                        fprintf(stdout,"***TEST NOK ***\n");
                    }
                }
                fprintf(stdout,"Press any key to continue\n");
                scanf("%c",&ch);
                
                bmt_val.bass=-9;
                bmt_val.mid=9;
                bmt_val.treble=-9;
                fprintf(stdout,"***Setting Mid high. ***\n");
                r = audcalparam_cmd_bmt(h, "BMT_auto_amp_general_playback", AUDCALPARAM_SET, &bmt_val, &bcfg);

                memcpy(&tmp, &bmt_val, sizeof(bmt_val));
                r = audcalparam_cmd_bmt(h, "BMT_auto_amp_general_playback", AUDCALPARAM_GET, &bmt_val, &bcfg);
                fprintf(stdout,"BMT Get after SET ret %d:%d %d %d\n",r,bmt_val.bass,bmt_val.mid,bmt_val.treble);
                if (r==AUDCALPARAM_OK) {
                    if (bmt_val.bass==tmp.bass && bmt_val.mid==tmp.mid && bmt_val.treble==tmp.treble){
                        fprintf(stdout,"***TEST OK ***\n");
                    }
                    else {
                        fprintf(stdout,"***TEST NOK ***\n");
                    }
                }
                fprintf(stdout,"Press any key to continue\n");
                scanf("%c",&ch);
                
                bmt_val.bass=9;
                bmt_val.mid=-9;
                bmt_val.treble=-9;
                fprintf(stdout,"***Setting Bass high. ***\n");
                r = audcalparam_cmd_bmt(h, "BMT_auto_amp_general_playback", AUDCALPARAM_SET, &bmt_val, &bcfg);
                
                memcpy(&tmp, &bmt_val, sizeof(bmt_val));
                r = audcalparam_cmd_bmt(h, "BMT_auto_amp_general_playback", AUDCALPARAM_GET, &bmt_val, &bcfg);
                fprintf(stdout,"BMT Get after SET ret %d:%d %d %d\n",r,bmt_val.bass,bmt_val.mid,bmt_val.treble);
                if (r==AUDCALPARAM_OK) {
                    if (bmt_val.bass==tmp.bass && bmt_val.mid==tmp.mid && bmt_val.treble==tmp.treble){
                        fprintf(stdout,"***TEST OK ***\n");
                    }
                    else {
                        fprintf(stdout,"***TEST NOK ***\n");
                    }
                }

            }
            else
                fprintf(stderr,"***Error when enabling BMT. Finish ***\n");
        }
    } else if (argc>=2 && !strcmp(argv[1],"9")) {
        fprintf(stdout,"***Run FNB audio test ***\n");

        // enable module
        mpcfg.module_id=0x123A3000;
        mpcfg.param_id=0x123A3001; // enable paramter
        mpcfg.base_cfg.cache=cache_flag;
        uint32_t enable=1;
        int32_t len;    
        uint8_t buf[64];
        uint8_t *pbuf=buf;
        len=sizeof(buf);

        r = audcalparam_cmd_module_param(h, "copp_auto_amp_general_playback",  AUDCALPARAM_GET, pbuf, &len, &mpcfg);
        if (r==AUDCALPARAM_OK){
            if (*(uint32_t*)pbuf==0){
                fprintf(stdout,"***FNB is disabled. Enable it. ***\n");
                // enable=1;
                *(uint32_t*)pbuf=1;
                r = audcalparam_cmd_module_param(h, "copp_auto_amp_general_playback",  AUDCALPARAM_SET, pbuf, &len, &mpcfg);
            }
            if (r==AUDCALPARAM_OK){
                bcfg.cache=cache_flag;

                audcalparam_cmd_fnb_t fnb_val={0};
                audcalparam_cmd_fnb_t tmp;
                fnb_val.filter_mask=0x3;//all fields

                fnb_val.balance=9;
                fnb_val.fade=0;
                r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_SET, &fnb_val, &bcfg);
                fprintf(stdout,"FnB Set ret %d:%d %d\n",r,fnb_val.balance,fnb_val.fade);

                memcpy(&tmp, &fnb_val, sizeof(fnb_val));
                r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_GET, &fnb_val, &bcfg);
                fprintf(stdout,"FNB Get after SET ret %d:%d %d\n",r,fnb_val.balance,fnb_val.fade);
                if (r==AUDCALPARAM_OK) {
                    if (fnb_val.balance==tmp.balance && fnb_val.fade==tmp.fade){
                        fprintf(stdout,"***TEST OK ***\n");
                    }
                    else {
                        fprintf(stdout,"***TEST NOK ***\n");
                    }
                }
                fprintf(stdout,"Press any key to continue\n");
                scanf("%c",&ch);


                fnb_val.balance=-9;
                fnb_val.fade=0;
                r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_SET, &fnb_val, &bcfg);
                fprintf(stdout,"FnB Set ret %d:%d %d\n",r,fnb_val.balance,fnb_val.fade);

                memcpy(&tmp, &fnb_val, sizeof(fnb_val));
                r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_GET, &fnb_val, &bcfg);
                fprintf(stdout,"FNB Get after SET ret %d:%d %d\n",r,fnb_val.balance,fnb_val.fade);
                if (r==AUDCALPARAM_OK) {
                    if (fnb_val.balance==tmp.balance && fnb_val.fade==tmp.fade){
                        fprintf(stdout,"***TEST OK ***\n");
                    }
                    else {
                        fprintf(stdout,"***TEST NOK ***\n");
                    }
                }
                fprintf(stdout,"Press any key to continue\n");
                scanf("%c",&ch);


                fnb_val.balance=0;
                fnb_val.fade=9;
                r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_SET, &fnb_val, &bcfg);
                fprintf(stdout,"FnB Get ret %d:%d %d\n",r,fnb_val.balance,fnb_val.fade);

                memcpy(&tmp, &fnb_val, sizeof(fnb_val));
                r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_GET, &fnb_val, &bcfg);
                fprintf(stdout,"FNB Get after SET ret %d:%d %d\n",r,fnb_val.balance,fnb_val.fade);
                if (r==AUDCALPARAM_OK) {
                    if (fnb_val.balance==tmp.balance && fnb_val.fade==tmp.fade){
                        fprintf(stdout,"***TEST OK ***\n");
                    }
                    else {
                        fprintf(stdout,"***TEST NOK ***\n");
                    }
                }
                fprintf(stdout,"Press any key to continue\n");
                scanf("%c",&ch);
                
                fnb_val.balance=0;
                fnb_val.fade=-9;
                r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_SET, &fnb_val, &bcfg);
                fprintf(stdout,"FnB Get ret %d:%d %d\n",r,fnb_val.balance,fnb_val.fade);

                memcpy(&tmp, &fnb_val, sizeof(fnb_val));
                r = audcalparam_cmd_fnb(h, "FNB_auto_amp_general_playback", AUDCALPARAM_GET, &fnb_val, &bcfg);
                fprintf(stdout,"FNB Get after SET ret %d:%d %d\n",r,fnb_val.balance,fnb_val.fade);
                if (r==AUDCALPARAM_OK) {
                    if (fnb_val.balance==tmp.balance && fnb_val.fade==tmp.fade){
                        fprintf(stdout,"***TEST OK ***\n");
                    }
                    else {
                        fprintf(stdout,"***TEST NOK ***\n");
                    }
                }
                
                fprintf(stdout,"Test done. Press any key to continue\n");
                scanf("%c",&ch);
            }
            else
                fprintf(stderr,"***Error when enabling FNB. Finish ***\n");
        }
    } else {
        r = 1;
        fprintf(stderr,"Unknown test!\n");
    }
    TEST_EPILOG(cache_flag)
    fprintf(stdout,"***Test Cmd  ret %d***\n",r);
exit:

    r1 = audcalparam_session_deinit(h);
    fprintf(stdout,"Session de-init ret %d\n",r1);

    return r;
}
