/*!
 * @file vpp_ip_hvx.h
 *
 * @cr
 * Copyright (c) 2015-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#ifndef _VPP_IP_HVX_H_
#define _VPP_IP_HVX_H_

#include <pthread.h>
#include <semaphore.h>

#include "vpp_ion.h"
#include "vpp_queue.h"
#include "vpp_ip.h"
#include "vpp_svc.h"
#include "vpp_ip_hvx_core.h"

#define HVX_MAX_NUM_BUFS 4

/************************************************************************
 * Local definitions
 ***********************************************************************/
#define HVX_CB_GET(ctx) (ctx ? (t_StVppIpHvxCb *)ctx : NULL)

enum
{
    /* Expandable internal flags (bits 24-31) */
    eIpFlag_DI_Algo_Run,
};

#define IP_FLAG_DI_ALGO_RUN (1 << (eVppIpFlag_IpStart + eIpFlag_DI_Algo_Run))

#define HVX_CMD_Q_SZ 30

#define HVX_ALGO_AIE (1 << 0)
#define HVX_ALGO_CNR (1 << 1)
#define HVX_ALGO_TNR (1 << 2)
#define HVX_ALGO_DI  (1 << 3)
#define HVX_ALGO_ROI (1 << 4)
#define HVX_PARAM    (1 << 16)

#define HVX_PARAMID_BIT_LENGTH 6
#define HVX_GLOBAL_LOCAL_BIT_LENGTH 1
#define HVX_PARAMID_BIT_MASK ((1 << (HVX_PARAMID_BIT_LENGTH)) - 1)
#define HVX_GLOBAL_LOCAL_BIT_MASK 1
#define HVX_CUSTOM_HEADER_SIZE 4

#define HVX_AIE_CADE_DE0_LEVEL_MIN   0.0
#define HVX_AIE_CADE_DE0_LEVEL_MAX   4.0
#define HVX_AIE_CADE_DE1_LEVEL_MIN   0.0
#define HVX_AIE_CADE_DE1_LEVEL_MAX   2.0
#define HVX_AIE_CADE_DE2_LEVEL_MIN   0.0
#define HVX_AIE_CADE_DE2_LEVEL_MAX   6.0
#define HVX_AIE_CADE_DE3_LEVEL_MIN   0.0
#define HVX_AIE_CADE_DE3_LEVEL_MAX   1.0
#define HVX_AIE_LTM_CON_LEVEL_MAX    256
#define HVX_AIE_LTM_BRI_LEVEL_MAX    128
#define HVX_AIE_LTM_SAT_GAIN_MAX     512
#define HVX_AIE_LTM_SAT_OFFSET_MAX   512
#define HVX_AIE_LTM_SAT_LEVEL_MIN    256
#define HVX_ROI_PERCENTAGE_MAX       100

enum
{
    HVX_BLOCK_MVP,
    HVX_BLOCK_NR,
    HVX_BLOCK_AIE,
    HVX_BLOCK_MAX,
};

typedef enum {
    HVX_STATE_INITED,         // Initialized state
    HVX_STATE_ACTIVE_START,   // Ready to start active processing, DSP needs all new buffers
    HVX_STATE_ACTIVE,         // Active processing, DSP has a copy of previously sent buffers
} t_EVppHvxState;

typedef struct StCustomAieDe
{
    float de_fDeGain0;
    float de_fDeOff0;
    float de_nDeMin0;
    float de_nDeMax0;
    float de_fDeGain1;
    float de_fDeOff1;
    float de_nDeMin1;
    float de_nDeMax1;
    float de_fDeGain2;
    float de_fDeOff2;
    float de_nDeMin2;
    float de_nDeMax2;
    float de_fDeGain3;
    float de_fDeOff3;
    float de_nDeMin3;
    float de_nDeMax3;
    float de_nDePower;
    float de_fDeSlope;
} t_StCustomAieDe;

typedef struct StCustomAieLtmCaParams
{
    short ltm_CA_params_EN;
    short ltm_CA_params_LMODE;
    short ltm_CA_params_TMODE0;
    short ltm_CA_params_TMODE1;
    short ltm_CA_params_TSIGN0;
    short ltm_CA_params_TSIGN1;
    short ltm_CA_params_HMIN;
    short ltm_CA_params_HMAX;
    unsigned short ltm_CA_params_YMIN;
    unsigned short ltm_CA_params_YMAX;
    unsigned short ltm_CA_params_SMIN;
    unsigned short ltm_CA_params_SMAX;
} t_StCustomAieLtmCaParams;

typedef struct StCustomAieLtm
{
    int ltm_nLTM_EN_MAP;
    int ltm_nLTM_EN_SAT;
    int ltm_nLTM_NUMX;
    int ltm_nLTM_NUMY;
    int ltm_nLTM_BITS_SIZEX;
    int ltm_nLTM_BITS_SIZEY;
    int ltm_nLTM_SAT_GAIN;
    int ltm_nLTM_SAT_OFF;
    int ltm_nLTM_SAT_THR;
    int ltm_nCoefIIR;
    unsigned int ltm_nThrDif;
    unsigned int ltm_nLutCntBlend;
    unsigned int ltm_nLutCoeffLPF;
    float ltm_fTmCoeff;
    float ltm_fLutCoeffIIR;
    float ltm_fLutCoeffMinIIR;
    float ltm_fLutThreshIIR;
} t_StCustomAieLtm;

typedef struct StCustomAieLtmCa
{
    int   ltm_nCA_EN_LPF;
    t_StCustomAieLtmCaParams stAieLtmCa_Y;
    t_StCustomAieLtmCaParams stAieLtmCa_U;
    t_StCustomAieLtmCaParams stAieLtmCa_V;
} t_StCustomAieLtmCa;

typedef struct StCustomAieAce
{
    int ace_nStrCon;
    int ace_nStrBriL;
    int ace_nStrBriH;
} t_StCustomAieAce;

typedef struct StCustomAieParams
{
    t_StCustomAieDe stAieDe;
    t_StCustomAieLtmCa stAieLtmCa;
    t_StCustomAieLtm stAieLtm;
    t_StCustomAieAce stAieAce;
} t_StCustomAieParams;

typedef struct StCustomHvxParams
{
    t_StCustomAieParams stCustomAieParams;
} t_StCustomHvxParams;

typedef struct StHvxParam {
    uint32_t u32MvpEn;
    vpp_svc_mvp_params_t stMvpParams;
    vpp_svc_nr_params_t stNrParams;
    vpp_svc_aie_params_t stAieParams;
} t_StHvxParam;

typedef struct {
    struct hqv_ctrl_aie stAie;
    struct hqv_ctrl_cnr stCnr;
    struct hqv_ctrl_di stDi;
    struct hqv_ctrl_global_demo stDemo;
    uint32_t u32AutoHqvEnable;
    uint32_t u32EnableMask;
    uint32_t u32ComputeMask;
    uint32_t u32LoadMask;
} t_StVppIpHvxCfg;

typedef struct {
    t_StVppIpHvxCoreCb *pstHvxCore;

    t_StVppIpBase stBase;

    uint32_t u32InternalFlags;

    sem_t sem;
    pthread_t thread;
    pthread_cond_t cond;
    pthread_mutex_t mutex;

    t_StVppIpPort stInput;
    t_StVppIpPort stOutput;

    t_StVppQueue stCmdQ;
    t_StVppIpCmd astCmdNode[HVX_CMD_Q_SZ];

    uint32_t eState;
    t_EVppHvxState eHvxProcState;

    t_StVppIpHvxCfg stCfg;
    uint32_t u32InputIntoProc; //number of input buff needed for a processing i.e. 1 for AIE and 4 for MVP
    uint32_t u32InputOutofProc; //number of input buff consumed for a processing i.e. 1 for AIE and 2 for MVP
    uint32_t u32OutputIntoProc;
    uint32_t u32OutputOutofProc;

    uint32_t u32InputMaxIntoProc; //max number of input buff needed for a processing
    uint32_t u32OutputMaxIntoProc;//max number of output buff needed for a processing

    //hvx parameters
    vpp_svc_config_hdr_t* pstMvpHeader;
    vpp_svc_mvp_params_t* pstMvpParams;
    vpp_svc_config_hdr_t* pstNrHeader;
    vpp_svc_nr_params_t* pstNrParams;
    vpp_svc_config_hdr_t* pstAieHeader;
    vpp_svc_aie_params_t* pstAieParams;

    t_StHvxParam stHvxParams;

    t_StCustomHvxParams stLocalHvxParams;

    struct {
        uint32_t u32InProcCnt;       // Number of input frames processed
        uint32_t u32OutProcCnt;      // Number of output frames processed into
        uint32_t u32InQueuedCnt;        // Number of input frames queued into
    } stats;

    struct {
        uint32_t u32OpenRet;
        uint32_t u32CloseRet;
        uint32_t u32ReconfigRet;
    } async_res;
} t_StVppIpHvxCb;

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/

uint32_t u32VppIpHvx_Boot();
uint32_t u32VppIpHvx_Shutdown();
void * vpVppIpHvx_Init(t_StVppCtx *pstCtx, uint32_t u32Flags, t_StVppCallback cbs);
void vVppIpHvx_Term(void *ctx);
uint32_t u32VppIpHvx_Open(void *ctx);
uint32_t u32VppIpHvx_Close(void *ctx);
uint32_t u32VppIpHvx_SetParam(void *ctx, enum vpp_port ePort,
                              struct vpp_port_param stParam);
uint32_t u32VppIpHvx_SetCtrl(void *ctx, struct hqv_control stCtrl);
uint32_t u32VppIpHvx_GetBufferRequirements(void *ctx,
                                           t_StVppIpBufReq *pstInputBufReq,
                                           t_StVppIpBufReq *pstOutputBufReq);

uint32_t u32VppIpHvx_QueueBuf(void *ctx, enum vpp_port ePort,
                              t_StVppBuf *pBuf);
uint32_t u32VppIpHvx_Flush(void *ctx, enum vpp_port ePort);

uint32_t u32VppIpHvx_Drain(void *ctx);

uint32_t u32VppIpHvx_Reconfigure(void *ctx,
                                 struct vpp_port_param in_param,
                                 struct vpp_port_param out_param);

#endif /* _VPP_IP_HVX_H_ */
