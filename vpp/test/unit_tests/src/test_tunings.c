/*!
 * @file test_tunings.c
 *
 * @cr
 * Copyright (c) 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <cutils/properties.h>

#include "vpp_dbg.h"
#include "dvpTest.h"
#include "dvpTest_tb.h"

#include "vpp_tunings.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/
#define TEST_TUNINGS_FILE_NAME "/data/vendor/vpp/test/test_tunings.txt"
#define MAX_TUNING_VAL_COUNT 10

enum {
    // Valid
    TUNINGTEST_VALID_U32_MIN,
    TUNINGTEST_VALID_U32_MAX,
    TUNINGTEST_VALID_U32_MID,
    TUNINGTEST_VALID_S32_MIN,
    TUNINGTEST_VALID_S32_MAX,
    TUNINGTEST_VALID_S32_MID,
    TUNINGTEST_VALID_FLOAT_MIN,
    TUNINGTEST_VALID_FLOAT_MAX,
    TUNINGTEST_VALID_FLOAT_MID,
    TUNINGTEST_VALID_AU32_9,
    TUNINGTEST_VALID_AS32_7,
    TUNINGTEST_VALID_AFLOAT_5,
    TUNINGTEST_VALID_U32_MULTIPLE,
    TUNINGTEST_VALID_EXTERNAL_VALIDATE,
    TUNING_TEST_VALID_COUNT, // Not a tuning parameter
    // Invalid
    TUNINGTEST_INVALID_U32_OVER = TUNING_TEST_VALID_COUNT,
    TUNINGTEST_INVALID_U32_UNDER,
    TUNINGTEST_INVALID_U32_COUNT,
    TUNINGTEST_INVALID_S32_OVER,
    TUNINGTEST_INVALID_S32_UNDER,
    TUNINGTEST_INVALID_S32_COUNT,
    TUNINGTEST_INVALID_FLOAT_OVER,
    TUNINGTEST_INVALID_FLOAT_UNDER,
    TUNINGTEST_INVALID_FLOAT_NAN,
    TUNINGTEST_INVALID_FLOAT_COUNT,
    TUNINGTEST_INVALID_AU32_3_OVER,
    TUNINGTEST_INVALID_AU32_5_UNDER,
    TUNINGTEST_INVALID_AU32_10_COUNT,
    TUNINGTEST_INVALID_AS32_3_OVER,
    TUNINGTEST_INVALID_AS32_5_UNDER,
    TUNINGTEST_INVALID_AS32_10_COUNT,
    TUNINGTEST_INVALID_AFLOAT_3_OVER,
    TUNINGTEST_INVALID_AFLOAT_5_UNDER,
    TUNINGTEST_INVALID_AFLOAT_7_NAN,
    TUNINGTEST_INVALID_AFLOAT_10_COUNT,
    TUNINGTEST_INVALID_U32_MULTIPLE,
    TUNINGTEST_INVALID_EXTERNAL_VALIDATE,
    TUNING_TEST_TOTAL_COUNT, // Not a tuning parameter
};

/************************************************************************
 * Forward Declarations
 ************************************************************************/
static uint32_t u32ValidateTuningIsOdd(t_StTuning *pstTuning);

/************************************************************************
 * Local static variables
 ***********************************************************************/
static const t_StTuningDef astTuningDef[] = {
    TUNING_DEF(TUNINGTEST_VALID_U32_MIN, U32, 1, 0, 64, NULL),
    TUNING_DEF(TUNINGTEST_VALID_U32_MAX, U32, 1, 127, 1023, NULL),
    TUNING_DEF(TUNINGTEST_VALID_U32_MID, U32, 1, 0, 2, NULL),
    TUNING_DEF(TUNINGTEST_VALID_S32_MIN, S32, 1, -127, 128, NULL),
    TUNING_DEF(TUNINGTEST_VALID_S32_MAX, S32, 1, -1023, 1024, NULL),
    TUNING_DEF(TUNINGTEST_VALID_S32_MID, S32, 1, -1, 1, NULL),
    TUNING_DEF(TUNINGTEST_VALID_FLOAT_MIN, FLOAT, 1, -1.50, 1.50, NULL),
    TUNING_DEF(TUNINGTEST_VALID_FLOAT_MAX, FLOAT, 1, -10.10, 10.10, NULL),
    TUNING_DEF(TUNINGTEST_VALID_FLOAT_MID, FLOAT, 1, 1.00, 2.00, NULL),
    TUNING_DEF(TUNINGTEST_VALID_AU32_9, U32, 9, 0, 63, NULL),
    TUNING_DEF(TUNINGTEST_VALID_AS32_7, S32, 7, -127, 127, NULL),
    TUNING_DEF(TUNINGTEST_VALID_AFLOAT_5, FLOAT, 5, -10.00, 10.00, NULL),
    TUNING_DEF(TUNINGTEST_VALID_U32_MULTIPLE, U32, 1, 0, 100, NULL),
    TUNING_DEF(TUNINGTEST_VALID_EXTERNAL_VALIDATE, U32, 1, 0, 100, u32ValidateTuningIsOdd),
    TUNING_DEF(TUNINGTEST_INVALID_U32_OVER, U32, 1, 0, 64, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_U32_UNDER, U32, 1, 127, 1023, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_U32_COUNT, U32, 1, 0, 2, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_S32_OVER, S32, 1, -127, 128, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_S32_UNDER, S32, 1, -1023, 1024, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_S32_COUNT, S32, 1, -1, 1, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_FLOAT_OVER, FLOAT, 1, -1.50, 1.50, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_FLOAT_UNDER, FLOAT, 1, -10.10, 10.10, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_FLOAT_NAN, FLOAT, 1, 1.00, 2.00, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_FLOAT_COUNT, FLOAT, 1, 1.00, 2.00, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_AU32_3_OVER, U32, 3, 0, 1023, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_AU32_5_UNDER, U32, 5, 512, 1023, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_AU32_10_COUNT, U32, 10, 0, 255, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_AS32_3_OVER, S32, 3, -127, 128, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_AS32_5_UNDER, S32, 5, -1023, 1024, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_AS32_10_COUNT, S32, 10, -1023, 1024, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_AFLOAT_3_OVER, FLOAT, 3, 0.00, 1.00, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_AFLOAT_5_UNDER, FLOAT, 5, -1.50, 1.50, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_AFLOAT_7_NAN, FLOAT, 7, -10.00, 10.00, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_AFLOAT_10_COUNT, FLOAT, 10, 0.00, 10.00, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_U32_MULTIPLE, U32, 1, 0, 100, NULL),
    TUNING_DEF(TUNINGTEST_INVALID_EXTERNAL_VALIDATE, U32, 1, 0, 100, u32ValidateTuningIsOdd),
};

static const uint32_t u32TuningCnt = sizeof(astTuningDef) / sizeof(t_StTuningDef);

static const t_UTuningValue auExpectedVal[TUNING_TEST_VALID_COUNT][MAX_TUNING_VAL_COUNT] = {
    [TUNINGTEST_VALID_U32_MIN] = {{.U32 = 0}},
    [TUNINGTEST_VALID_U32_MAX] = {{.U32 = 1023}},
    [TUNINGTEST_VALID_U32_MID] = {{.U32 = 1}},
    [TUNINGTEST_VALID_S32_MIN] = {{.S32 = -127}},
    [TUNINGTEST_VALID_S32_MAX] = {{.S32 = 1024}},
    [TUNINGTEST_VALID_S32_MID] = {{.S32 = 0}},
    [TUNINGTEST_VALID_FLOAT_MIN] = {{.FLOAT = -1.5}},
    [TUNINGTEST_VALID_FLOAT_MAX] = {{.FLOAT = 10.1}},
    [TUNINGTEST_VALID_FLOAT_MID] = {{.FLOAT = 1.5}},
    [TUNINGTEST_VALID_AU32_9] = {{.U32 = 56}, {.U32 = 49}, {.U32 = 42}, {.U32 = 35},
        {.U32 = 28}, {.U32 = 21}, {.U32 = 14}, {.U32 = 7}, {.U32 = 0}},
    [TUNINGTEST_VALID_AS32_7] = {{.S32 = -3}, {.S32 = -2}, {.S32 = -1}, {.S32 = 0},
        {.S32 = 1}, {.S32 = 2}, {.S32 = 3}},
    [TUNINGTEST_VALID_AFLOAT_5] = {{.FLOAT = 1.0}, {.FLOAT = 0.5}, {.FLOAT = 0},
        {.FLOAT = -0.5}, {.FLOAT = -1.0}},
    [TUNINGTEST_VALID_U32_MULTIPLE] = {{.U32 = 75}},
    [TUNINGTEST_VALID_EXTERNAL_VALIDATE] = {{.U32 = 11}},
};

/************************************************************************
 * Local Functions
 ***********************************************************************/
#define RUN_IF_ENABLED() { \
    if (!isTestHarnessRun()) { \
        LOGI("%s: test is only enabled when run by harness", __func__); \
        return; \
    } \
}

static uint32_t isTestHarnessRun()
{
    return property_get_bool("vendor.media.vpp.harness_tunings", VPP_FALSE);
}

#define IS_ODD(x)   ((x) & 0x1)

static uint32_t u32ValidateTuningIsOdd(t_StTuning *pstTuning)
{
    VPP_RET_IF_NULL(pstTuning, VPP_ERR_PARAM);

    if (IS_ODD(pstTuning->puVal[0].U32))
        return VPP_OK;

    return VPP_ERR;
}

#define VALIDATE_TUNING_VALUES(_type, _logfmt, _pdef, _pval) \
    if ((_pdef)->eType == TUNING_TYPE_##_type) \
    { \
        uint32_t i; \
        for (i = 0; i < (_pdef)->u32Count; i++) \
        { \
            if ((_pval)[i]._type != auExpectedVal[(_pdef)->u32Id][i]._type) \
            { \
                LOGE("Value mismatch for Id=%u(%s). Expected=" _logfmt " Actual=" _logfmt, \
                     (_pdef)->u32Id, (_pdef)->acId, \
                     auExpectedVal[(_pdef)->u32Id][i]._type, \
                     (_pval)[i]._type); \
                DVP_ASSERT_FAIL(); \
                return VPP_ERR; \
            } \
        } \
        return VPP_OK; \
    }

static uint32_t u32ValidateValues(uint32_t u32Id, t_UTuningValue *puValues)
{
    const t_StTuningDef *pstDef;

    VPP_RET_IF_NULL(puValues, VPP_ERR_PARAM);

    if (u32Id >= TUNING_TEST_TOTAL_COUNT)
        return VPP_ERR_PARAM;

    pstDef = &astTuningDef[u32Id];
    VPP_RET_IF_NULL(pstDef, VPP_ERR_PARAM);

    VALIDATE_TUNING_VALUES(U32, "%u", pstDef, puValues);
    VALIDATE_TUNING_VALUES(S32, "%d", pstDef, puValues);
    VALIDATE_TUNING_VALUES(FLOAT, "%f", pstDef, puValues);

    return VPP_ERR;
}


static uint32_t u32ValidateTuningValues(t_StTuning *pstTuning)
{
    VPP_RET_IF_NULL(pstTuning, VPP_ERR_PARAM)

    return u32ValidateValues(pstTuning->pstDef->u32Id, pstTuning->puVal);
}

/************************************************************************
 * Test Functions
 ***********************************************************************/
TEST_SUITE_INIT(TuningsSuiteInit)
{
}

TEST_SUITE_TERM(TuningsSuiteTerm)
{
}

TEST_SETUP(TuningsTestInit)
{
}

TEST_CLEANUP(TuningsTestTerm)
{
}

TEST(InvalidInput)
{
    void *pvCtx;
    t_StTuning *pstTuning;
    t_StTuning stTuning;
    uint32_t u32Ret, u32Cnt;
    t_UTuningValue auTuningVal[MAX_TUNING_VAL_COUNT];

    pvCtx = vpVppTunings_Init(NULL, astTuningDef, u32TuningCnt);
    DVP_ASSERT_PTR_NULL(pvCtx);

    pvCtx = vpVppTunings_Init(TEST_TUNINGS_FILE_NAME, NULL, u32TuningCnt);
    DVP_ASSERT_PTR_NULL(pvCtx);

    pvCtx = vpVppTunings_Init(TEST_TUNINGS_FILE_NAME, astTuningDef, 0);
    DVP_ASSERT_PTR_NULL(pvCtx);

    pvCtx = vpVppTunings_Init("NonExistantFile.txt", astTuningDef, u32TuningCnt);
    DVP_ASSERT_PTR_NULL(pvCtx);

    u32Cnt = u32VppTunings_GetValidTuningsCount(NULL);
    DVP_ASSERT_EQUAL(u32Cnt, 0);

    pstTuning = pstVppTunings_GetTuningByIndex(NULL, 0);
    DVP_ASSERT_PTR_NULL(pstTuning);

    pstTuning = pstVppTunings_GetTuningById(NULL, 0);
    DVP_ASSERT_PTR_NULL(pstTuning);

    u32Cnt = u32VppTunings_GetTuningCount(NULL);
    DVP_ASSERT_EQUAL(u32Cnt, 0);

    u32Cnt = u32VppTunings_GetTuningCountById(NULL, 0);
    DVP_ASSERT_EQUAL(u32Cnt, 0);

    u32Cnt = u32VppTunings_GetTuningCountByIndex(NULL, 0);
    DVP_ASSERT_EQUAL(u32Cnt, 0);

    u32Ret = u32VppTunings_GetTuningValues(NULL, auTuningVal,
                                           MAX_TUNING_VAL_COUNT);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppTunings_GetTuningValues(&stTuning, NULL, 0);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppTunings_GetTuningValuesByIndex(NULL, 0, auTuningVal,
                                                  MAX_TUNING_VAL_COUNT);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    u32Ret = u32VppTunings_GetTuningValuesById(NULL, 0, auTuningVal,
                                               MAX_TUNING_VAL_COUNT);
    DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

    pvCtx = vpVppTunings_Init(TEST_TUNINGS_FILE_NAME, astTuningDef,
                              u32TuningCnt);
    if (pvCtx)
    {
        u32Cnt = u32VppTunings_GetTuningCountById(pvCtx, TUNING_TEST_TOTAL_COUNT);
        DVP_ASSERT_EQUAL(u32Cnt, 0);

        u32Cnt = u32VppTunings_GetTuningCountByIndex(pvCtx, TUNING_TEST_TOTAL_COUNT);
        DVP_ASSERT_EQUAL(u32Cnt, 0);

        u32Ret = u32VppTunings_GetTuningValuesByIndex(pvCtx, 0, NULL, 0);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        u32Ret = u32VppTunings_GetTuningValuesById(pvCtx, 0, NULL, 0);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        // Length smaller than required
        u32Ret = u32VppTunings_GetTuningValuesById(pvCtx, TUNINGTEST_VALID_AU32_9,
                                                   auTuningVal, 8);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);

        vVppTunings_Term(pvCtx);
    }
}

TEST(ValidTunings)
{
    RUN_IF_ENABLED();

    void *pvCtx;
    uint32_t u32ValidCnt, u32Cnt, u32Ret, i;
    t_StTuning *pstTuning;
    t_UTuningValue auTuningVal[MAX_TUNING_VAL_COUNT];

    pvCtx = vpVppTunings_Init(TEST_TUNINGS_FILE_NAME, astTuningDef,
                              u32TuningCnt);

    DVP_ASSERT_PTR_NNULL(pvCtx);
    if (!pvCtx)
        return;

    u32ValidCnt = u32VppTunings_GetValidTuningsCount(pvCtx);
    DVP_ASSERT_EQUAL(u32ValidCnt, TUNING_TEST_VALID_COUNT);

    for (i = 0; i < u32ValidCnt; i++)
    {
        u32Cnt = u32VppTunings_GetTuningCountByIndex(pvCtx, i);
        DVP_ASSERT_NEQUAL(u32Cnt, 0);

        u32Ret = u32VppTunings_GetTuningValuesByIndex(pvCtx, i, auTuningVal,
                                                      MAX_TUNING_VAL_COUNT);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        pstTuning = pstVppTunings_GetTuningByIndex(pvCtx, i);
        DVP_ASSERT_PTR_NNULL(pstTuning);

        if (!pstTuning)
            continue;

        u32Ret = u32ValidateTuningValues(pstTuning);
        if (u32Ret != VPP_OK)
        {
            LOGE("Validate values failed at Index=%u(Id=%u)",
                 i, pstTuning->pstDef->u32Id);
            DVP_ASSERT_FAIL();
        }

        u32Ret = u32ValidateValues(pstTuning->pstDef->u32Id, auTuningVal);
        if (u32Ret != VPP_OK)
        {
            LOGE("GetTuningValuesByIndex validate failed at Index=%u(Id=%u)",
                 pstTuning->pstDef->u32Id, i);
            DVP_ASSERT_FAIL();
        }

        memset(auTuningVal, 0, sizeof(auTuningVal));
        u32Ret = u32VppTunings_GetTuningValues(pstTuning, auTuningVal,
                                               MAX_TUNING_VAL_COUNT);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32ValidateValues(pstTuning->pstDef->u32Id, auTuningVal);
        if (u32Ret != VPP_OK)
        {
            LOGE("GetTuningValues validate failed at Index=%u(Id=%u)",
                 i, pstTuning->pstDef->u32Id);
            DVP_ASSERT_FAIL();
        }
    }

    for (i = 0; i < TUNING_TEST_VALID_COUNT; i++)
    {
        u32Cnt = u32VppTunings_GetTuningCountById(pvCtx, i);
        DVP_ASSERT_NEQUAL(u32Cnt, 0);

        pstTuning = pstVppTunings_GetTuningById(pvCtx, i);
        DVP_ASSERT_PTR_NNULL(pstTuning);

        u32Ret = u32ValidateTuningValues(pstTuning);
        if (u32Ret != VPP_OK)
        {
            LOGE("Validate values failed for Id=%u", i);
            DVP_ASSERT_FAIL();
        }

        memset(auTuningVal, 0, sizeof(auTuningVal));
        u32Ret = u32VppTunings_GetTuningValuesById(pvCtx, i, auTuningVal,
                                                   MAX_TUNING_VAL_COUNT);
        DVP_ASSERT_EQUAL(u32Ret, VPP_OK);

        u32Ret = u32ValidateValues(i, auTuningVal);
        if (u32Ret != VPP_OK)
        {
            LOGE("GetTuningValuesById validate failed for Id=%u", i);
            DVP_ASSERT_FAIL();
        }
    }

    vVppTunings_Term(pvCtx);
}

TEST(InvalidTunings)
{
    RUN_IF_ENABLED();

    void *pvCtx;
    uint32_t u32ValidCnt, u32Cnt, u32Ret, i;
    t_StTuning *pstTuning;
    t_UTuningValue auTuningVal[MAX_TUNING_VAL_COUNT];

    pvCtx = vpVppTunings_Init(TEST_TUNINGS_FILE_NAME, astTuningDef,
                              u32TuningCnt);

    DVP_ASSERT_PTR_NNULL(pvCtx);
    if (!pvCtx)
        return;

    u32ValidCnt = u32VppTunings_GetValidTuningsCount(pvCtx);
    DVP_ASSERT_EQUAL(u32ValidCnt, TUNING_TEST_VALID_COUNT);

    u32Cnt = u32VppTunings_GetTuningCountByIndex(pvCtx, u32ValidCnt + 1);
    DVP_ASSERT_EQUAL(u32Cnt, 0);

    pstTuning = pstVppTunings_GetTuningByIndex(pvCtx, u32ValidCnt + 1);
    DVP_ASSERT_PTR_NULL(pstTuning);

    for (i = TUNING_TEST_VALID_COUNT; i < TUNING_TEST_TOTAL_COUNT; i++)
    {
        u32Cnt = u32VppTunings_GetTuningCountById(pvCtx, i);
        DVP_ASSERT_EQUAL(u32Cnt, 0);

        pstTuning = pstVppTunings_GetTuningById(pvCtx, i);
        DVP_ASSERT_PTR_NULL(pstTuning);

        u32Ret = u32VppTunings_GetTuningValuesById(pvCtx, i, auTuningVal,
                                                   MAX_TUNING_VAL_COUNT);
        DVP_ASSERT_NEQUAL(u32Ret, VPP_OK);
    }

    vVppTunings_Term(pvCtx);
}

/************************************************************************
 * Global Functions
 ***********************************************************************/
TEST_CASES TuningsTests[] = {
    TEST_CASE(InvalidInput),
    TEST_CASE(ValidTunings),
    TEST_CASE(InvalidTunings),
    TEST_CASE_NULL(),
};

TEST_SUITE(TuningsSuite,
           "TuningsTests",
           TuningsSuiteInit,
           TuningsSuiteTerm,
           TuningsTestInit,
           TuningsTestTerm,
           TuningsTests);
