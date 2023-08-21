/*!
 * @file vpp_data_queue.h
 *
 * @cr
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */


#ifndef _VPP_DATA_QUEUE_H_
#define _VPP_DATA_QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif
/************************************************************************
 * Defines
 ***********************************************************************/

/************************************************************************
 * Structures / Enumerations
 ***********************************************************************/
typedef struct StVppDataQueueNode {
    struct StVppDataQueueNode *pstNext;
    void *pvData;
} t_StVppDataQueueNode;

typedef struct {
    t_StVppDataQueueNode *pstHead;
    t_StVppDataQueueNode *pstTail;
    uint32_t u32Cnt;
} t_StVppDataQueue;

typedef uint32_t (*fpMatchPred)(t_StVppDataQueueNode *, void *);

/************************************************************************
 * Function Prototypes
 ***********************************************************************/
uint32_t u32VppDataQueue_Init(t_StVppDataQueue *pstQ);
uint32_t u32VppDataQueue_Term(t_StVppDataQueue *pstQ);
uint32_t u32VppDataQueue_Push(t_StVppDataQueue *pstQ, t_StVppDataQueueNode *pstNode);
t_StVppDataQueueNode *pstVppDataQueue_Pop(t_StVppDataQueue *pstQ);
t_StVppDataQueueNode *pstVppDataQueue_Peek(t_StVppDataQueue *pstQ, uint32_t index);
uint32_t u32VppDataQueue_PutFront(t_StVppDataQueue *pstQ, t_StVppDataQueueNode *pstNode);
t_StVppDataQueueNode *pstVppDataQueue_RemoveMatching(t_StVppDataQueue *pstQ,
                                                     fpMatchPred pred,
                                                     void *pv);
uint32_t u32VppDataQueue_Cnt(t_StVppDataQueue *pstQ);

#ifdef __cplusplus
}
#endif

#endif /* _VPP_DATA_QUEUE_H_ */