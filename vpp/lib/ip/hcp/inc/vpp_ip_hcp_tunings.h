/*!
 * @file vpp_ip_hcp_tunings.h
 *
 * @cr
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#ifndef _VPP_IP_HCP_TUNINGS_H_
#define _VPP_IP_HCP_TUNINGS_H_

#include <pthread.h>

#include "vpp_ip_hcp_priv.h"
#include "vpp_tunings.h"

#define HCP_TUNINGS_FILE_NAME "/mnt/vendor/persist/vpp/tunings.txt"

typedef enum {
    HCP_TUNING_STATE_NULL,
    HCP_TUNING_STATE_INITED,
    HCP_TUNING_STATE_BUFFER_SEND_PENDING,
    HCP_TUNING_STATE_BUFFER_SENT,
    HCP_TUNING_STATE_BUFFER_RECV_PENDING,
} t_EHcpTuningState;

typedef struct {
    t_EHcpTuningState eState;
    pthread_mutex_t mutex;
    void *pvTuningBlock;
    t_StVppIpHcpSysBuf stSysBuf;
} t_StHcpTuningCb;
/***************************************************************************
 * Function Prototypes
 ***************************************************************************/
uint32_t u32VppIpHcp_TuningInit(t_StVppIpHcpGlobalCb *pstGlobal);
uint32_t u32VppIpHcp_TuningTerm(t_StVppIpHcpGlobalCb *pstGlobal);
uint32_t u32VppIpHcp_TuningLoad(t_StVppIpHcpGlobalCb *pstGlobal);
uint32_t u32VppIpHcp_TuningProcBuffReleasedMsg(t_StVppIpHcpGlobalCb *pstGlobal,
                                               t_StHcpHfiMsgPkt *pstMsg);
uint32_t bVppIpHcp_IsTuningLoadComplete(t_StVppIpHcpGlobalCb *pstGlobal);
uint32_t u32VppIpHcp_GetTuningCount(t_StVppIpHcpGlobalCb *pstGlobal,
                                    uint32_t u32TuningId);
uint32_t u32VppIpHcp_GetTuning(t_StVppIpHcpGlobalCb *pstGlobal, uint32_t u32TuningId,
                               t_UTuningValue *puTuning, uint32_t u32Len);

#endif /* _VPP_IP_HCP_TUNINGS_H_ */
