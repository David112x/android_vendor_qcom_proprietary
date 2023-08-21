/*!
 * @file vpp_ip_hvx_debug.c
 *
 * @cr
 * Copyright (c) 2015-2017, 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <stdio.h>
#include <utils/Log.h>
#include <stdlib.h>

//#define LOG_NDEBUG 0
#define VPP_LOG_TAG VPP_LOG_MODULE_HVX_TAG
#define VPP_LOG_MODULE VPP_LOG_MODULE_HVX
#include "vpp_dbg.h"

#include "hvx_debug.h"

//for debug
int print_vpp_svc_buffer_attribute_t(vpp_svc_frame_group_descriptor_t *frame_group_descp)
{
    int i ;
    int j ;

    LOGD("frame_group_descp->width =%d\n", frame_group_descp->width);
    LOGD("frame_group_descp->height =%d\n", frame_group_descp->height);
    LOGD("frame_group_descp->pixelformat =%d\n", frame_group_descp->pixelformat);
    LOGD("frame_group_descp->fieldformat =%d\n", frame_group_descp->fieldformat);
    LOGD("frame_group_descp->numbuffers =%d\n", frame_group_descp->numbuffers);
    LOGD("frame_group_descp->pixel_dataLen =%d\n", frame_group_descp->pixel_dataLen);
    for (i = 0; i<frame_group_descp->pixel_dataLen; i++)
    {
        //pixel_data
        LOGD("frame_group_descp->pixel_data[%d].user_dataLen = %d\n", i,
             frame_group_descp->pixel_data[i].user_dataLen);
        LOGD("frame_group_descp->pixel_data[%d].user_data= %p\n", i,
             (void *) frame_group_descp->pixel_data[i].user_data);//print data pointer
    }

    //bufferattributes[4]
    for (i=0; i<4; i++)
    {
        LOGD("frame_group_descp->bufferattributes[%d].index=%d\n", i,
             frame_group_descp->bufferattributes[i].index);
        for (j=0; j<3; j++)
        {
            LOGD("frame_group_descp->bufferattributes[%d].plane_stride[%d]=%d\n", i, j,
                 frame_group_descp->bufferattributes[i].plane_stride[j]);
            LOGD("frame_group_descp->bufferattributes[%d].plane_sizebytes[%d]=%d\n", i, j,
                 frame_group_descp->bufferattributes[i].plane_sizebytes[j]);
        }
    }
    return 0;
}

int print_vpp_svc_params(vpp_svc_params_t *vpp_svc_params)
{
    LOGD("vpp_svc_params->headerLen = %d \n", vpp_svc_params->headerLen);
    LOGD("vpp_svc_params->user_data = %p \n", (void*)vpp_svc_params->user_data);
    LOGD("vpp_svc_params->user_dataLen = %d \n", vpp_svc_params->user_dataLen);

    LOGD("vpp_svc_params->header->vpp_func_id = %d \n", vpp_svc_params->header->vpp_func_id);
    LOGD("vpp_svc_params->header->process_flags = %d \n", vpp_svc_params->header->process_flags);
    return 0;
}

int print_vpp_svc_mvp_params(vpp_svc_mvp_params_t *vpp_svc_mvp_params)
{
    LOGD("sizeof(vpp_svc_mvp_params_t) =%zu\n", sizeof(vpp_svc_mvp_params_t));
    LOGD("vpp_svc_mvp_params->update_flags =%d\n", vpp_svc_mvp_params->update_flags);
    LOGD("vpp_svc_mvp_params->in_frame_width =%d\n", vpp_svc_mvp_params->in_frame_width);
    LOGD("vpp_svc_mvp_params->in_frame_height =%d\n", vpp_svc_mvp_params->in_frame_height);
    LOGD("vpp_svc_mvp_params->mode(mvp_mode) =%d\n", vpp_svc_mvp_params->mode);
    return 0;
}

int print_vpp_svc_ie_params(vpp_svc_aie_params_t *vpp_svc_ie_params)
{
    LOGD("vpp_svc_ie_params->update_flags =%d", vpp_svc_ie_params->update_flags);
    LOGD("vpp_svc_ie_params->in_frame_width =%u", vpp_svc_ie_params->in_frame_width);
    LOGD("vpp_svc_ie_params->in_frame_height =%u", vpp_svc_ie_params->in_frame_height);
    LOGD("vpp_svc_ie_params->mode =%d\n", vpp_svc_ie_params->mode);
    LOGD("vpp_svc_ie_params->ltm_nEN_DE =%d", vpp_svc_ie_params->ltm_nEN_DE);
    LOGD("vpp_svc_ie_params->de_fDeGain0 =%f", vpp_svc_ie_params->de_fDeGain0);
    LOGD("vpp_svc_ie_params->de_fDeOff0 =%f", vpp_svc_ie_params->de_fDeOff0);
    LOGD("vpp_svc_ie_params->de_nDeMin0 =%d", vpp_svc_ie_params->de_nDeMin0);
    LOGD("vpp_svc_ie_params->de_nDeMax0 =%d", vpp_svc_ie_params->de_nDeMax0);
    LOGD("vpp_svc_ie_params->de_fDeGain1 =%f", vpp_svc_ie_params->de_fDeGain1);
    LOGD("vpp_svc_ie_params->de_fDeOff1 =%f", vpp_svc_ie_params->de_fDeOff1);
    LOGD("vpp_svc_ie_params->de_nDeMin1 =%d", vpp_svc_ie_params->de_nDeMin1);
    LOGD("vpp_svc_ie_params->de_nDeMax1 =%d", vpp_svc_ie_params->de_nDeMax1);
    LOGD("vpp_svc_ie_params->de_fDeGain2 =%f", vpp_svc_ie_params->de_fDeGain2);
    LOGD("vpp_svc_ie_params->de_fDeOff2 =%f", vpp_svc_ie_params->de_fDeOff2);
    LOGD("vpp_svc_ie_params->de_nDeMin2 =%d", vpp_svc_ie_params->de_nDeMin2);
    LOGD("vpp_svc_ie_params->de_nDeMax2 =%d", vpp_svc_ie_params->de_nDeMax2);
    LOGD("vpp_svc_ie_params->de_fDeGain3 =%f", vpp_svc_ie_params->de_fDeGain3);
    LOGD("vpp_svc_ie_params->de_fDeOff3 =%f", vpp_svc_ie_params->de_fDeOff3);
    LOGD("vpp_svc_ie_params->de_nDeMin3 =%d", vpp_svc_ie_params->de_nDeMin3);
    LOGD("vpp_svc_ie_params->de_nDeMax3 =%d", vpp_svc_ie_params->de_nDeMax3);
    LOGD("vpp_svc_ie_params->de_nDePower =%d", vpp_svc_ie_params->de_nDePower);
    LOGD("vpp_svc_ie_params->de_fDeSlope =%f", vpp_svc_ie_params->de_fDeSlope);
    LOGD("vpp_svc_ie_params->ltm_nEN_CA =%d", vpp_svc_ie_params->ltm_nEN_CA);
    LOGD("vpp_svc_ie_params->ltm_nCA_EN_LPF =%d", vpp_svc_ie_params->ltm_nCA_EN_LPF);
    LOGD("vpp_svc_ie_params->ltm_CA_params_EN_Y =%d", vpp_svc_ie_params->ltm_CA_params_EN_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_LMODE_Y =%d", vpp_svc_ie_params->ltm_CA_params_LMODE_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TMODE0_Y =%d", vpp_svc_ie_params->ltm_CA_params_TMODE0_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TMODE1_Y =%d", vpp_svc_ie_params->ltm_CA_params_TMODE1_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TSIGN0_Y =%d", vpp_svc_ie_params->ltm_CA_params_TSIGN0_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TSIGN1_Y =%d", vpp_svc_ie_params->ltm_CA_params_TSIGN1_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_HMIN_Y =%d", vpp_svc_ie_params->ltm_CA_params_HMIN_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_HMAX_Y =%d", vpp_svc_ie_params->ltm_CA_params_HMAX_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_YMIN_Y =%u", vpp_svc_ie_params->ltm_CA_params_YMIN_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_YMAX_Y =%u", vpp_svc_ie_params->ltm_CA_params_YMAX_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_SMIN_Y =%u", vpp_svc_ie_params->ltm_CA_params_SMIN_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_SMAX_Y =%u", vpp_svc_ie_params->ltm_CA_params_SMAX_Y);
    LOGD("vpp_svc_ie_params->ltm_CA_params_EN_U =%d", vpp_svc_ie_params->ltm_CA_params_EN_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_LMODE_U =%d", vpp_svc_ie_params->ltm_CA_params_LMODE_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TMODE0_U =%d", vpp_svc_ie_params->ltm_CA_params_TMODE0_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TMODE1_U =%d", vpp_svc_ie_params->ltm_CA_params_TMODE1_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TSIGN0_U =%d", vpp_svc_ie_params->ltm_CA_params_TSIGN0_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TSIGN1_U =%d", vpp_svc_ie_params->ltm_CA_params_TSIGN1_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_HMIN_U =%d", vpp_svc_ie_params->ltm_CA_params_HMIN_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_HMAX_U =%d", vpp_svc_ie_params->ltm_CA_params_HMAX_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_YMIN_U =%u", vpp_svc_ie_params->ltm_CA_params_YMIN_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_YMAX_U =%u", vpp_svc_ie_params->ltm_CA_params_YMAX_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_SMIN_U =%u", vpp_svc_ie_params->ltm_CA_params_SMIN_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_SMAX_U =%u", vpp_svc_ie_params->ltm_CA_params_SMAX_U);
    LOGD("vpp_svc_ie_params->ltm_CA_params_EN_V =%d", vpp_svc_ie_params->ltm_CA_params_EN_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_LMODE_V =%d", vpp_svc_ie_params->ltm_CA_params_LMODE_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TMODE0_V =%d", vpp_svc_ie_params->ltm_CA_params_TMODE0_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TMODE1_V =%d", vpp_svc_ie_params->ltm_CA_params_TMODE1_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TSIGN0_V =%d", vpp_svc_ie_params->ltm_CA_params_TSIGN0_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_TSIGN1_V =%d", vpp_svc_ie_params->ltm_CA_params_TSIGN1_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_HMIN_V =%d", vpp_svc_ie_params->ltm_CA_params_HMIN_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_HMAX_V =%d", vpp_svc_ie_params->ltm_CA_params_HMAX_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_YMIN_V =%u", vpp_svc_ie_params->ltm_CA_params_YMIN_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_YMAX_V =%u", vpp_svc_ie_params->ltm_CA_params_YMAX_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_SMIN_V =%u", vpp_svc_ie_params->ltm_CA_params_SMIN_V);
    LOGD("vpp_svc_ie_params->ltm_CA_params_SMAX_V =%u", vpp_svc_ie_params->ltm_CA_params_SMAX_V);
    LOGD("vpp_svc_ie_params->ltm_nLTM_EN_MAP =%d", vpp_svc_ie_params->ltm_nLTM_EN_MAP);
    LOGD("vpp_svc_ie_params->ltm_nLTM_EN_SAT =%d", vpp_svc_ie_params->ltm_nLTM_EN_SAT);
    LOGD("vpp_svc_ie_params->ltm_nLTM_NUMX =%d", vpp_svc_ie_params->ltm_nLTM_NUMX);
    LOGD("vpp_svc_ie_params->ltm_nLTM_NUMY =%d", vpp_svc_ie_params->ltm_nLTM_NUMY);
    LOGD("vpp_svc_ie_params->ltm_nLTM_BITS_SIZEX =%d", vpp_svc_ie_params->ltm_nLTM_BITS_SIZEX);
    LOGD("vpp_svc_ie_params->ltm_nLTM_BITS_SIZEY =%d", vpp_svc_ie_params->ltm_nLTM_BITS_SIZEY);
    LOGD("vpp_svc_ie_params->ltm_nLTM_SAT_GAIN =%d", vpp_svc_ie_params->ltm_nLTM_SAT_GAIN);
    LOGD("vpp_svc_ie_params->ltm_nLTM_SAT_OFF =%d", vpp_svc_ie_params->ltm_nLTM_SAT_OFF);
    LOGD("vpp_svc_ie_params->ltm_nLTM_SAT_THR =%d", vpp_svc_ie_params->ltm_nLTM_SAT_THR);
    LOGD("vpp_svc_ie_params->ace_nStrCon =%d", vpp_svc_ie_params->ace_nStrCon);
    LOGD("vpp_svc_ie_params->ace_nStrBriL =%d", vpp_svc_ie_params->ace_nStrBriL);
    LOGD("vpp_svc_ie_params->ace_nStrBriH =%d", vpp_svc_ie_params->ace_nStrBriH);
    return 0;
}

int print_vpp_svc_nr_params(vpp_svc_nr_params_t *vpp_svc_nr_params)
{
    LOGD("vpp_svc_nr_params->update_flags =%d\n", vpp_svc_nr_params->update_flags);
    LOGD("vpp_svc_nr_params->in_frame_width =%d\n", vpp_svc_nr_params->in_frame_width);
    LOGD("vpp_svc_nr_params->in_frame_height =%d\n", vpp_svc_nr_params->in_frame_height);
    LOGD("vpp_svc_nr_params->mode =%d\n", vpp_svc_nr_params->mode);
    LOGD("vpp_svc_nr_params->level =%d\n", vpp_svc_nr_params->level);
    return 0;
}

int print_vpp_svc_frc_params(vpp_svc_frc_params_t *frc_params_p)
{
    LOGD("frc_params_p->update_flags = %d", frc_params_p->update_flags);
    LOGD("frc_params_p->mode = %d", frc_params_p->mode);
    LOGD("frc_params_p->mc_quality = %d", frc_params_p->mc_quality);
    LOGD("frc_params_p->NUM_INTERP_FRAMES = %d", frc_params_p->NUM_INTERP_FRAMES);
    LOGD("frc_params_p->interp_cnt = %d", frc_params_p->interp_cnt);
    LOGD("frc_params_p->RepeatMode_repeatPeriod = %d", frc_params_p->RepeatMode_repeatPeriod);
    LOGD("frc_params_p->TH_MOTION = %d", frc_params_p->TH_MOTION);
    LOGD("frc_params_p->TH_MOTION_LOW = %d", frc_params_p->TH_MOTION_LOW);
    LOGD("frc_params_p->TH_MVOUTLIER_COUNT = %d", frc_params_p->TH_MVOUTLIER_COUNT);
    LOGD("frc_params_p->TH_MVOUTLIER_COUNT_LOW = %d", frc_params_p->TH_MVOUTLIER_COUNT_LOW);
    LOGD("frc_params_p->TH_OCCLUSION = %d", frc_params_p->TH_OCCLUSION);
    LOGD("frc_params_p->TH_OCCLUSION_LOW = %d", frc_params_p->TH_OCCLUSION_LOW);
    LOGD("frc_params_p->TH_MOTION00 = %d", frc_params_p->TH_MOTION00);
    LOGD("frc_params_p->TH_MOTION00_LOW = %d", frc_params_p->TH_MOTION00_LOW);
    LOGD("frc_params_p->TH_MVOUTLIER_VARIANCE_COUNT = %d",
         frc_params_p->TH_MVOUTLIER_VARIANCE_COUNT);
    LOGD("frc_params_p->TH_MVOUTLIER_VARIANCE_COUNT_LOW = %d",
         frc_params_p->TH_MVOUTLIER_VARIANCE_COUNT_LOW);
    LOGD("frc_params_p->TH_SCENECUT = %d", frc_params_p->TH_SCENECUT);
    LOGD("frc_params_p->TH_VARIANCE = %d", frc_params_p->TH_VARIANCE);
    LOGD("frc_params_p->TH_SAD_FR_RATIO = %d", frc_params_p->TH_SAD_FR_RATIO);

    return 0;
}

void print_data(char* infor, char* data, int len)
{
    int i ;
    printf("%s\n",infor);
    for (i=0; i<len; i++)
    {
        printf("%x ",data[i]);
        if((i+1)%32==0)
            printf("\n");
    }
    printf("\n");
}

void print_proc_data(vpp_svc_frame_group_descriptor_t *pstBufferdataIn)
{
    int i ;
    unsigned int j;
    for(i=0; i<pstBufferdataIn->numbuffers; i++)
    {
        printf("buff[%d]:",i);
        for(j=0; j<(MIN(20,pstBufferdataIn->bufferattributes[i].plane_sizebytes[PLANE_ID_Y])); j++)
        {
            printf("%02x ", pstBufferdataIn->pixel_data[i].user_data[j]);
        }
        printf("\n");
    }
}
