/*!
 * @file vpp_ip_c2d.h
 *
 * @cr
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 */

#ifndef _VPP_IP_C2D_H_
#define _VPP_IP_C2D_H_

/***************************************************************************
 * Function Prototypes
 ***************************************************************************/

void *vpVppIpC2D_Init(t_StVppCtx *pstCtx, uint32_t u32Flags, t_StVppCallback cbs);
void vVppIpC2D_Term(void *ctx);
uint32_t u32VppIpC2D_Open(void *ctx);
uint32_t u32VppIpC2D_Close(void *ctx);
uint32_t u32VppIpC2D_SetParam(void *ctx, enum vpp_port port,
                              struct vpp_port_param param);
uint32_t u32VppIpC2D_SetCtrl(void *ctx, struct hqv_control ctrl);
uint32_t u32VppIpC2D_GetBufferRequirements(void *ctx,
                                           t_StVppIpBufReq *pstInputBufReq,
                                           t_StVppIpBufReq *pstOutputBufReq);
uint32_t u32VppIpC2D_QueueBuf(void *ctx, enum vpp_port ePort,
                              t_StVppBuf *pBuf);
uint32_t u32VppIpC2D_Flush(void *ctx, enum vpp_port ePort);
uint32_t u32VppIpC2D_Drain(void *ctx);
uint32_t u32VppIpC2D_Reconfigure(void *ctx,
                                 struct vpp_port_param in_param,
                                 struct vpp_port_param out_param);

// The APIs below can be used for inline synchronous processing
// An inline session cannot be used with the asynchrounous APIs above
void *vpVppIpC2D_InlineInit(t_StVppCtx *pstCtx,
                            struct vpp_port_param stInParam,
                            struct vpp_port_param stOutParam);
void vVppIpC2D_InlineTerm(void *ctx);
uint32_t u32VppIpC2D_InlineReconfigure(void *ctx,
                                       struct vpp_port_param stInParam,
                                       struct vpp_port_param stOutParam);
uint32_t u32VppIpC2D_InlineProcess(void *ctx,
                                   t_StVppBuf *pstBufIn,
                                   t_StVppBuf *pstBufOut);
#endif /* _VPP_IP_C2D_H_ */
