/*!
 * @file vpp_data_queue.c
 *
 * @cr
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vpp_dbg.h"
#include "vpp.h"
#include "vpp_data_queue.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/
// #define DEBUG_DATA_QUEUE

/************************************************************************
 * Local static variables
 ***********************************************************************/

/************************************************************************
 * Forward Declarations
 ***********************************************************************/

/************************************************************************
 * Local functions
 ***********************************************************************/

/************************************************************************
 * Global Functions
 ***********************************************************************/
uint32_t u32VppDataQueue_Init(t_StVppDataQueue *pstQ)
{
    if (!pstQ)
        return VPP_ERR_PARAM;

    memset(pstQ, 0, sizeof(t_StVppDataQueue));

    return VPP_OK;
}

uint32_t u32VppDataQueue_Term(t_StVppDataQueue *pstQ)
{
    if (!pstQ)
        return VPP_ERR_PARAM;

    if (pstQ->u32Cnt)
        LOGE("destroying queue with non zero count, cnt=%u", pstQ->u32Cnt);

    memset(pstQ, 0, sizeof(t_StVppDataQueue));

    return VPP_OK;
}

uint32_t u32VppDataQueue_Push(t_StVppDataQueue *pstQ, t_StVppDataQueueNode *pstNode)
{
    if (!pstQ || !pstNode)
        return VPP_ERR_PARAM;

    pstNode->pstNext = NULL;

    if (!pstQ->u32Cnt)
    {
        pstQ->pstHead = pstNode;
        pstQ->pstTail = pstNode;
    }
    else
    {
        pstQ->pstTail->pstNext = pstNode;
        pstQ->pstTail = pstNode;
    }

    pstQ->u32Cnt += 1;

#ifdef DEBUG_DATA_QUEUE
    LOGI("%s: queue: %p, nodes count: %u, buf: %p, head: %p, tail: %p",
         __func__, pstQ, pstQ->u32Cnt, pstNode, pstQ->pstHead,
         pstQ->pstTail);
#endif

    return VPP_OK;
}

t_StVppDataQueueNode *pstVppDataQueue_Pop(t_StVppDataQueue *pstQ)
{
    t_StVppDataQueueNode *pstTmp = NULL;

    if (!pstQ || !pstQ->pstHead)
        return NULL;

    pstTmp = pstQ->pstHead;
    pstQ->pstHead = pstQ->pstHead->pstNext;
    pstQ->u32Cnt -= 1;

    if (!pstQ->u32Cnt)
        pstQ->pstTail = NULL;

#ifdef DEBUG_DATA_QUEUE
    LOGI("%s: queue: %p, nodes remaining: %u, returning: %p, head: %p, tail: %p",
         __func__, pstQ, pstQ->u32Cnt, pstTmp, pstQ->pstHead,
         pstQ->pstTail);
#endif

    return pstTmp;
}

t_StVppDataQueueNode *pstVppDataQueue_Peek(t_StVppDataQueue *pstQ, uint32_t index)
{
    // returns the node element but will not remove it from the list
    // if index==0, return pstQ->pstHead;
    // if index==1, return pstQ->pstHead->next; and so on

    t_StVppDataQueueNode *pstTmp = NULL;
    uint32_t cnt = 0;

    if (!pstQ || !pstQ->pstHead || index >= pstQ->u32Cnt)
        return NULL;

    pstTmp = pstQ->pstHead;
    while (pstTmp && cnt < index)
    {
        pstTmp = pstTmp->pstNext;
        cnt += 1;
    }

#ifdef DEBUG_DATA_QUEUE
    LOGI("%s: queue: %p, nodes remaining: %u, returning: %p, head: %p, tail: %p, cnt: %d, index:%d",
         __func__, pstQ, pstQ->u32Cnt, pstTmp, pstQ->pstHead,
         pstQ->pstTail, cnt, index);
#endif

    return pstTmp;
}

uint32_t u32VppDataQueue_PutFront(t_StVppDataQueue *pstQ, t_StVppDataQueueNode *pstNode)
{
    if (!pstQ || !pstNode)
        return VPP_ERR_PARAM;

    pstNode->pstNext = pstQ->pstHead;
    pstQ->pstHead = pstNode;

    if (!pstQ->pstTail)
        pstQ->pstTail = pstNode;

    pstQ->u32Cnt += 1;

#ifdef DEBUG_DATA_QUEUE
    LOGI("%s: queue: %p, nodes count: %u, buf: %p, head: %p, tail: %p",
         __func__, pstQ, pstQ->u32Cnt, pstBuf, pstQ->pstHead,
         pstQ->pstTail);
#endif

    return VPP_OK;
}

t_StVppDataQueueNode *pstVppDataQueue_RemoveMatching(t_StVppDataQueue *pstQ,
                                                     fpMatchPred pred,
                                                     void *pv)
{
    t_StVppDataQueueNode *pstCur, *pstPrev;

    VPP_RET_IF_NULL(pstQ, NULL);
    VPP_RET_IF_NULL(pstQ->pstHead, NULL);
    VPP_RET_IF_NULL(pred, NULL);

    pstPrev = NULL;
    pstCur = pstQ->pstHead;
    while (pstCur != NULL)
    {
        if (pred(pstCur, pv))
            break;

        pstPrev = pstCur;
        pstCur = pstCur->pstNext;
    }

    if (!pstCur)
    {
        LOGD("no match, queue_cnt=%u", pstQ->u32Cnt);
        return NULL;
    }

    if (pstCur == pstQ->pstHead)
    {
        pstQ->pstHead = pstQ->pstHead->pstNext;
    }
    else
    {
        pstPrev->pstNext = pstCur->pstNext;
    }

    if (pstCur == pstQ->pstTail)
        pstQ->pstTail = pstPrev;

    pstQ->u32Cnt -= 1;

#ifdef DEBUG_DATA_QUEUE
    LOGI("%s: queue: %p, nodes remaining: %u, returning: %p, head: %p, tail: %p",
         __func__, pstQ, pstQ->u32Cnt, pstCur, pstQ->pstHead,
         pstQ->pstTail);
#endif

    return pstCur;
}

uint32_t u32VppDataQueue_Cnt(t_StVppDataQueue *pstQ)
{
    if (!pstQ)
        return 0;

    return pstQ->u32Cnt;
}