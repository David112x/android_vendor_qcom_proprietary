/*!
 * @file test_data_queue.c
 *
 * @cr
 * Copyright (c) 2018 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <stdlib.h>
#include <string.h>

#include "dvpTest.h"
#include "dvpTest_tb.h"

#include "vpp_dbg.h"
#include "vpp.h"
#include "vpp_data_queue.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/

/************************************************************************
 * Local static variables
 ***********************************************************************/

/************************************************************************
 * Forward Declarations
 ************************************************************************/

/************************************************************************
 * Local Functions
 ***********************************************************************/

/************************************************************************
 * Test Functions
 ***********************************************************************/
TEST_SUITE_INIT(DataQueueSuiteInit)
{
}

TEST_SUITE_TERM(DataQueueSuiteTerm)
{
}

TEST_SETUP(DataQueueTestInit)
{
}

TEST_CLEANUP(DataQueueTestTerm)
{
}

TEST(DataQueue_TestPop)
{
    uint32_t u32Ret;
    t_StVppDataQueueNode *pstNode;
    t_StVppDataQueue stQ;

    u32Ret = u32VppDataQueue_Init(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    // Invalid pool
    pstNode = pstVppDataQueue_Pop(NULL);
    DVP_ASSERT_PTR_NULL(pstNode);

    // 0 count pool
    pstNode = pstVppDataQueue_Pop(&stQ);
    DVP_ASSERT_PTR_NULL(pstNode);

    u32Ret = u32VppDataQueue_Term(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
}

TEST(DataQueue_TestPush)
{
    uint32_t u32Ret;
    t_StVppDataQueueNode stNode;
    t_StVppDataQueueNode *pstNode;
    t_StVppDataQueue stQ;

    u32Ret = u32VppDataQueue_Init(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    // Invalid buf and pool
    u32Ret = u32VppDataQueue_Push(NULL, NULL);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    // Invalid pool
    u32Ret = u32VppDataQueue_Push(NULL, &stNode);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    // Invalid buf
    u32Ret = u32VppDataQueue_Push(&stQ, NULL);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    // Valid
    u32Ret = u32VppDataQueue_Push(&stQ, &stNode);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    // Can we get it back?
    pstNode = pstVppDataQueue_Pop(&stQ);
    DVP_ASSERT_PTR_NNULL(pstNode);

    // Make sure we don't get back more then we put in
    pstNode = pstVppDataQueue_Pop(&stQ);
    DVP_ASSERT_PTR_NULL(pstNode);

    u32Ret = u32VppDataQueue_Term(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
}

#define NODE_TEST_CNT 10
TEST(DataQueue_TestPutFront)
{
    uint32_t u32Ret, i;
    t_StVppDataQueue stQ;
    t_StVppDataQueueNode stNode, *pstNode;

    t_StVppDataQueueNode astNode[NODE_TEST_CNT];

    u32Ret = u32VppDataQueue_Init(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    // Invalid buf and pool
    u32Ret = u32VppDataQueue_PutFront(NULL, NULL);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    // Invalid pool
    u32Ret = u32VppDataQueue_PutFront(NULL, &stNode);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    // Invalid buf
    u32Ret = u32VppDataQueue_PutFront(&stQ, NULL);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    // Valid
    u32Ret = u32VppDataQueue_Push(&stQ, &stNode);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    DVP_ASSERT_EQUAL(1, u32VppDataQueue_Cnt(&stQ));

    // Can we get it back?
    pstNode = pstVppDataQueue_Pop(&stQ);
    DVP_ASSERT_PTR_NNULL(pstNode);
    DVP_ASSERT_PTR_EQUAL(pstNode, &stNode);

    DVP_ASSERT_EQUAL(0, u32VppDataQueue_Cnt(&stQ));

    // Make sure we don't get back more then we put in
    pstNode = pstVppDataQueue_Pop(&stQ);
    DVP_ASSERT_PTR_NULL(pstNode);

    for (i = 0; i < NODE_TEST_CNT; i++)
    {
        u32Ret = u32VppDataQueue_PutFront(&stQ, &astNode[i]);
        DVP_ASSERT_EQUAL(i + 1, u32VppDataQueue_Cnt(&stQ));
        pstNode = pstVppDataQueue_Peek(&stQ, 0);
        DVP_ASSERT_EQUAL(pstNode, &astNode[i]);
    }

    for (i = 0; i < NODE_TEST_CNT; i++)
    {
        pstNode = pstVppDataQueue_Peek(&stQ, i);
        DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[NODE_TEST_CNT - i - 1]);
    }

    for (i = 0; i < NODE_TEST_CNT; i++)
    {
        pstNode = pstVppDataQueue_Pop(&stQ);
        DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[NODE_TEST_CNT - i - 1]);

        DVP_ASSERT_EQUAL(NODE_TEST_CNT - i - 1, u32VppDataQueue_Cnt(&stQ));
    }

    u32Ret = u32VppDataQueue_Term(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
}

TEST(DataQueue_TestPushPopCnt)
{
    uint32_t u32Ret, u32Cnt, i, j;
    t_StVppDataQueueNode astNode[NODE_TEST_CNT];
    t_StVppDataQueueNode *pstNode;

    t_StVppDataQueue stQ;
    u32Ret = u32VppDataQueue_Init(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    // Nothing in pool
    u32Cnt = u32VppDataQueue_Cnt(&stQ);
    DVP_ASSERT_EQUAL(u32Cnt, 0);

    // Make sure as we queue, we get right count
    for (i = 0; i < NODE_TEST_CNT; i++)
    {
        u32Ret = u32VppDataQueue_Push(&stQ, &astNode[i]);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Cnt = u32VppDataQueue_Cnt(&stQ);
        DVP_ASSERT_EQUAL(u32Cnt, i + 1);
    }

    // Make sure as we dequeue we get right count
    for (i = NODE_TEST_CNT; i > 0; i--)
    {
        pstNode = pstVppDataQueue_Pop(&stQ);
        DVP_ASSERT_PTR_NNULL(pstNode);

        // Make sure that we find the buffer from the original list
        for (j = 0; j < NODE_TEST_CNT; j++)
        {
            if (pstNode == &astNode[j])
                break;
        }
        DVP_ASSERT_TRUE(j < NODE_TEST_CNT);

        u32Cnt = u32VppDataQueue_Cnt(&stQ);
        DVP_ASSERT_EQUAL(u32Cnt, i - 1);
    }

    // should be none left in the pool
    u32Cnt = u32VppDataQueue_Cnt(&stQ);
    DVP_ASSERT_EQUAL(u32Cnt, 0);

    // Queue one, dequeue one. Make sure that they are equal.
    for (i = 0; i < NODE_TEST_CNT; i++)
    {
        u32Ret = u32VppDataQueue_Push(&stQ, &astNode[i]);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Cnt = u32VppDataQueue_Cnt(&stQ);
        DVP_ASSERT_EQUAL(u32Cnt, 1);

        pstNode = pstVppDataQueue_Pop(&stQ);
        DVP_ASSERT_PTR_NNULL(pstNode);
        DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[i]);

        u32Cnt = u32VppDataQueue_Cnt(&stQ);
        DVP_ASSERT_EQUAL(u32Cnt, 0);
    }

    u32Ret = u32VppDataQueue_Term(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
}

TEST(DataQueue_TestPeek)
{
    uint32_t u32Ret,i=0,u32Cnt;
    t_StVppDataQueueNode stNode;
    t_StVppDataQueueNode *pstNode;
    t_StVppDataQueue stQ;

    t_StVppDataQueueNode astNode[NODE_TEST_CNT];

    u32Ret = u32VppDataQueue_Init(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    // Invalid pool
    pstNode = pstVppDataQueue_Peek(NULL,0);
    DVP_ASSERT_PTR_NULL(pstNode);

    // 0 count pool
    pstNode = pstVppDataQueue_Peek(&stQ,0);
    DVP_ASSERT_PTR_NULL(pstNode);

    // Valid
    u32Ret = u32VppDataQueue_Push(&stQ, &stNode);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    // Can we get it back?
    pstNode = pstVppDataQueue_Peek(&stQ,0);
    DVP_ASSERT_PTR_NNULL(pstNode);
    DVP_ASSERT_PTR_EQUAL(pstNode, &stNode);

    //If index out of range
    pstNode = pstVppDataQueue_Peek(&stQ,1);
    DVP_ASSERT_PTR_NULL(pstNode);

    // Can we get it back?
    pstNode = pstVppDataQueue_Pop(&stQ);
    DVP_ASSERT_PTR_NNULL(pstNode);
    DVP_ASSERT_PTR_EQUAL(pstNode, &stNode);

    // Make sure as we queue, we get right count
    for (i = 0; i < NODE_TEST_CNT; i++)
    {
        u32Ret = u32VppDataQueue_Push(&stQ, &astNode[i]);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Cnt = u32VppDataQueue_Cnt(&stQ);
        DVP_ASSERT_EQUAL(u32Cnt, i + 1);
    }

    // Make sure as we peek we get right count
    for (i = 0; i < NODE_TEST_CNT; i++)
    {
        pstNode = pstVppDataQueue_Peek(&stQ,i);
        DVP_ASSERT_PTR_NNULL(pstNode);
        DVP_ASSERT_TRUE(pstNode == &astNode[i]);

        //u32Cnt remains the same
        u32Cnt = u32VppDataQueue_Cnt(&stQ);
        DVP_ASSERT_EQUAL(u32Cnt, NODE_TEST_CNT);
    }

    //If index out of range
    pstNode = pstVppDataQueue_Peek(&stQ,NODE_TEST_CNT);
    DVP_ASSERT_PTR_NULL(pstNode);
    pstNode = pstVppDataQueue_Peek(&stQ,100+NODE_TEST_CNT);
    DVP_ASSERT_PTR_NULL(pstNode);

    u32Ret = u32VppDataQueue_Term(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
}

TEST(DataQueue_TestInitTerm)
{
    t_StVppDataQueue stQ;
    uint32_t u32Ret;

    u32Ret = u32VppDataQueue_Init(NULL);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppDataQueue_Term(NULL);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppDataQueue_Init(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppDataQueue_Term(&stQ);
    DVP_ASSERT_EQUAL(u32Ret, VPP_OK);
}

static uint32_t u32RemoveMatchingPred(t_StVppDataQueueNode *pstNode, void *pv)
{
    if (pstNode == (t_StVppDataQueueNode*)pv)
        return VPP_TRUE;

    return VPP_FALSE;
}

TEST(DataQueue_RemoveMatching)
{
    uint32_t u32;
    t_StVppDataQueueNode *pstNode;
    t_StVppDataQueue stQ;
    t_StVppDataQueueNode astNode[NODE_TEST_CNT];

    fpMatchPred fp = u32RemoveMatchingPred;

    u32 = u32VppDataQueue_Init(&stQ);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // Sanity check parameters
    pstNode = pstVppDataQueue_RemoveMatching(NULL, NULL, NULL);
    DVP_ASSERT_PTR_NULL(pstNode);

    // predicate is required
    pstNode = pstVppDataQueue_RemoveMatching(&stQ, NULL, NULL);
    DVP_ASSERT_PTR_NULL(pstNode);

    // bufpool is required
    pstNode = pstVppDataQueue_RemoveMatching(NULL, fp, NULL);
    DVP_ASSERT_PTR_NULL(pstNode);

    // empty pool
    pstNode = pstVppDataQueue_RemoveMatching(&stQ, fp, NULL);
    DVP_ASSERT_PTR_NULL(pstNode);

    // single node
    {
        u32 = u32VppDataQueue_Push(&stQ, &astNode[0]);
        DVP_ASSERT_EQUAL(u32, VPP_OK);

        pstNode = pstVppDataQueue_RemoveMatching(&stQ, fp, &astNode[0]);
        DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[0]);

        u32 = u32VppDataQueue_Cnt(&stQ);
        DVP_ASSERT_EQUAL(u32, 0);

        pstNode = pstVppDataQueue_Pop(&stQ);
        DVP_ASSERT_PTR_NULL(pstNode);
    }

    // two nodes
    {
        // insert 0, 1, remove 0, 1
        u32 = u32VppDataQueue_Push(&stQ, &astNode[0]);
        DVP_ASSERT_EQUAL(u32, VPP_OK);
        u32 = u32VppDataQueue_Push(&stQ, &astNode[1]);
        DVP_ASSERT_EQUAL(u32, VPP_OK);
        pstNode = pstVppDataQueue_RemoveMatching(&stQ, fp, &astNode[0]);
        DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[0]);
        pstNode = pstVppDataQueue_RemoveMatching(&stQ, fp, &astNode[1]);
        DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[1]);

        // insert 2, 3, remove 3, 2
        u32 = u32VppDataQueue_Push(&stQ, &astNode[2]);
        DVP_ASSERT_EQUAL(u32, VPP_OK);
        u32 = u32VppDataQueue_Push(&stQ, &astNode[3]);
        DVP_ASSERT_EQUAL(u32, VPP_OK);
        pstNode = pstVppDataQueue_RemoveMatching(&stQ, fp, &astNode[3]);
        DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[3]);
        pstNode = pstVppDataQueue_RemoveMatching(&stQ, fp, &astNode[2]);
        DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[2]);

        // sanity check
        u32 = u32VppDataQueue_Cnt(&stQ);
        DVP_ASSERT_EQUAL(u32, 0);
        pstNode = pstVppDataQueue_Pop(&stQ);
        DVP_ASSERT_PTR_NULL(pstNode);
    }

    // middle nodes
    {
        uint32_t i;
        for (i = 0; i < NODE_TEST_CNT; i++)
        {
            u32 = u32VppDataQueue_Push(&stQ, &astNode[i]);
            DVP_ASSERT_EQUAL(u32, VPP_OK);
        }

        // remove 5 to max
        for (i = 5; i < NODE_TEST_CNT; i++)
        {
            pstNode = pstVppDataQueue_RemoveMatching(&stQ, fp, &astNode[i]);
            DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[i]);
        }

        // remove 4 down to 1
        for (i = 4; i != 0; i--)
        {
            pstNode = pstVppDataQueue_RemoveMatching(&stQ, fp, &astNode[i]);
            DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[i]);
        }
        u32 = u32VppDataQueue_Cnt(&stQ);
        DVP_ASSERT_EQUAL(u32, 1);

        pstNode = pstVppDataQueue_RemoveMatching(&stQ, fp, &astNode[0]);
        DVP_ASSERT_PTR_EQUAL(pstNode, &astNode[0]);

        u32 = u32VppDataQueue_Cnt(&stQ);
        DVP_ASSERT_EQUAL(u32, 0);
    }

    u32 = u32VppDataQueue_Term(&stQ);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
}

/************************************************************************
 * Global Functions
 ***********************************************************************/
TEST_CASES DataQueueTests[] = {
    TEST_CASE(DataQueue_TestInitTerm),
    TEST_CASE(DataQueue_TestPop),
    TEST_CASE(DataQueue_TestPush),
    TEST_CASE(DataQueue_TestPutFront),
    TEST_CASE(DataQueue_TestPushPopCnt),
    TEST_CASE(DataQueue_TestPeek),
    TEST_CASE(DataQueue_RemoveMatching),
    TEST_CASE_NULL(),
};

TEST_SUITE(DataQueueSuite,
           "DataQueueTests",
           DataQueueSuiteInit,
           DataQueueSuiteTerm,
           DataQueueTestInit,
           DataQueueTestTerm,
           DataQueueTests);