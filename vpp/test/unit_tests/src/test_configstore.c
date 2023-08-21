/*!
 * @file test_configstore.c
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
#include <limits.h>

#include <cutils/properties.h>

#include "vpp_dbg.h"
#include "dvpTest.h"
#include "dvpTest_tb.h"

#include "vpp_configstore.h"

/************************************************************************
 * Local definitions
 ***********************************************************************/
#define CS_TEST_AREA "vpp-test"

#define RUN_IF_ENABLED() { \
    if (!isTestHarnessRun()) { \
        LOGI("%s: test is only enabled when run by harness", __func__); \
        return; \
    } \
}

/************************************************************************
 * Local static variables
 ***********************************************************************/

/************************************************************************
 * Forward Declarations
 ************************************************************************/

/************************************************************************
 * Local Functions
 ***********************************************************************/
static uint32_t isTestHarnessRun()
{
    return property_get_bool("vendor.media.vpp.harness_configstore", VPP_FALSE);
}

static void check_default_bool(const char *pcArea,
                               const char *pcConfig,
                               uint32_t u32Expected)
{
    DVP_ASSERT_EQUAL(u32Expected,
                     bVppConfigStore_GetBool(pcArea, pcConfig, u32Expected));
}

static void check_default_uint(const char *pcArea,
                               const char *pcConfig,
                               uint32_t u32Expected)
{
    DVP_ASSERT_EQUAL(u32Expected,
                     u32VppConfigStore_GetUnsignedInt(pcArea, pcConfig, u32Expected));
}

static void check_invalid_string(const char *pcArea,
                                 const char *pcConfig)
{
    static const uint32_t u32Sz = 256;

    uint32_t u32;
    char buf[u32Sz];
    t_StConfigStoreStr stStr;
    stStr.pc = buf;
    stStr.u32Len = u32Sz;

    u32 = u32VppConfigStore_GetString(pcArea, pcConfig, &stStr);
    DVP_ASSERT_NEQUAL(u32, VPP_OK);
}

static uint32_t check_valid_string(const char *pcConfig, const char *pcExpect)
{
    // Return a result from this function so that caller can assert and have
    // stringification of the call dumped to a log, instead of only asserting.

    static const uint32_t u32BufSz = 64; // intentionally small for buffer size test
    char buf[u32BufSz];
    uint32_t u32;
    uint32_t u32Ret = VPP_OK;

    t_StConfigStoreStr stStr;
    stStr.pc = buf;
    stStr.u32Len = u32BufSz;

    u32 = u32VppConfigStore_GetString(CS_TEST_AREA, pcConfig, &stStr);
    if (u32 != VPP_OK)
    {
        DVP_ASSERT_EQUAL(u32, VPP_OK);
        u32Ret = VPP_ERR;
    }

    if (strlen(stStr.pc) != strlen(pcExpect))
    {
        DVP_ASSERT_EQUAL(strlen(stStr.pc), strlen(pcExpect));
        u32Ret = VPP_ERR;
    }

    if (strncmp(stStr.pc, pcExpect, stStr.u32Len) != 0)
    {
        DVP_ASSERT_FAIL();
        u32Ret = VPP_ERR;
    }

    return u32Ret;
}

/************************************************************************
 * Test Functions
 ***********************************************************************/
TEST_SUITE_INIT(ConfigStoreSuiteInit)
{
}

TEST_SUITE_TERM(ConfigStoreSuiteTerm)
{
}

TEST_SETUP(ConfigStoreTestInit)
{
}

TEST_CLEANUP(ConfigStoreTestTerm)
{
}

TEST(InvalidAreaConfigReturnsErrorString)
{
    uint32_t u32;

    u32 = u32VppConfigStore_GetString(NULL, NULL, NULL);
    DVP_ASSERT_NEQUAL(u32, VPP_OK);

    check_invalid_string(NULL, "area");
    check_invalid_string("invalid", NULL);
    check_invalid_string(NULL, "");
    check_invalid_string("", NULL);
    check_invalid_string(NULL, NULL);
}

TEST(InvalidAreaConfigReturnsDefaultInt)
{
    check_default_uint(NULL, "area", rand());
    check_default_uint("invalid", NULL, rand());
    check_default_uint("", NULL, rand());
    check_default_uint(NULL, "", rand());
    check_default_uint(NULL, NULL, rand());

    check_default_uint("invalid", "area", rand());
    check_default_uint("", "", rand());
    check_default_uint("", "", rand());
    check_default_uint("invalid", "", rand());
    check_default_uint("invalid", "", rand());
    check_default_uint("", "area", rand());
    check_default_uint("", "area", rand());
}

TEST(InvalidAreaConfigReturnsDefaultBool)
{
    check_default_bool(NULL, "area", VPP_TRUE);
    check_default_bool("invalid", NULL, VPP_TRUE);
    check_default_bool(NULL, "", VPP_TRUE);
    check_default_bool("", NULL, VPP_TRUE);
    check_default_bool(NULL, NULL, VPP_TRUE);

    check_default_bool(NULL, "area", VPP_FALSE);
    check_default_bool("invalid", NULL, VPP_FALSE);
    check_default_bool(NULL, "", VPP_FALSE);
    check_default_bool("", NULL, VPP_FALSE);
    check_default_bool(NULL, NULL, VPP_FALSE);

    check_default_bool("invalid", "area", VPP_TRUE);
    check_default_bool("invalid", "area", VPP_FALSE);
    check_default_bool("", "", VPP_TRUE);
    check_default_bool("", "", VPP_FALSE);
    check_default_bool("invalid", "", VPP_TRUE);
    check_default_bool("invalid", "", VPP_FALSE);
    check_default_bool("", "area", VPP_TRUE);
    check_default_bool("", "area", VPP_FALSE);
}

TEST(RealConfigStoreBool)
{
    RUN_IF_ENABLED();

    static const uint32_t u32TrueTests = 11;
    static const uint32_t u32FalseTests = 15;
    static const uint32_t u32BufSz = 128;

    uint32_t i;
    char key[u32BufSz];

    for (i = 0; i < u32TrueTests; i++)
    {
        snprintf(key, u32BufSz, "bool_true_%u", i);
        DVP_ASSERT_EQUAL(VPP_TRUE, bVppConfigStore_GetBool(CS_TEST_AREA, key, VPP_FALSE));
    }

    for (i = 0; i < u32FalseTests; i++)
    {
        snprintf(key, u32BufSz, "bool_false_%u", i);
        DVP_ASSERT_EQUAL(VPP_FALSE, bVppConfigStore_GetBool(CS_TEST_AREA, key, VPP_TRUE));
    }
}

TEST(RealConfigStoreString)
{
    RUN_IF_ENABLED();

    DVP_ASSERT_EQUAL(VPP_OK, check_valid_string("str_empty", ""));
    DVP_ASSERT_EQUAL(VPP_OK, check_valid_string("str_valid", "some_string"));
    DVP_ASSERT_EQUAL(VPP_OK, check_valid_string("str_exceed_buf_size",
                       "morethan64charsmorethan64charsmorethan64charsmorethan64charsone"));
}

TEST(RealConfigStoreInteger)
{
    RUN_IF_ENABLED();
    DVP_ASSERT_EQUAL(0, u32VppConfigStore_GetUnsignedInt(CS_TEST_AREA, "int_empty", 1));
    DVP_ASSERT_EQUAL(0x1234abcd, u32VppConfigStore_GetUnsignedInt(CS_TEST_AREA, "int_hex", 0));
    DVP_ASSERT_EQUAL((uint32_t)0xF0000001, u32VppConfigStore_GetUnsignedInt(CS_TEST_AREA, "int_hex_negative", 0));
    DVP_ASSERT_EQUAL(22412, u32VppConfigStore_GetUnsignedInt(CS_TEST_AREA, "int_positive", 0));
    DVP_ASSERT_EQUAL((uint32_t)-1123, u32VppConfigStore_GetUnsignedInt(CS_TEST_AREA, "int_negative", 0));
    DVP_ASSERT_EQUAL(ULONG_MAX, u32VppConfigStore_GetUnsignedInt(CS_TEST_AREA, "int_overflow", 0));
}

/************************************************************************
 * Global Functions
 ***********************************************************************/
TEST_CASES ConfigStoreTests[] = {
    TEST_CASE(InvalidAreaConfigReturnsErrorString),
    TEST_CASE(InvalidAreaConfigReturnsDefaultInt),
    TEST_CASE(InvalidAreaConfigReturnsDefaultBool),
    TEST_CASE(RealConfigStoreBool),
    TEST_CASE(RealConfigStoreString),
    TEST_CASE(RealConfigStoreInteger),
    TEST_CASE_NULL(),
};

TEST_SUITE(ConfigStoreSuite,
           "ConfigStoreTests",
           ConfigStoreSuiteInit,
           ConfigStoreSuiteTerm,
           ConfigStoreTestInit,
           ConfigStoreTestTerm,
           ConfigStoreTests);
