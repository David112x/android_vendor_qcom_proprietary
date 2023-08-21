/*!
 * @file test_ipc_utils.cpp
 *
 * @cr
 * Copyright (c) 2017, 2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.

 * @services
 */

#include <sys/types.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <cutils/native_handle.h>
#include <vendor/qti/hardware/vpp/1.1/types.h>

#include "dvpTest.h"
#include "dvpTest_tb.h"

#include "test_utils.h"
#include "buf_pool.h"

#define VPP_LOG_TAG     VPP_LOG_UT_IPC_UTILS_TAG
#define VPP_LOG_MODULE  VPP_LOG_UT_IPC_UTILS
#include "vpp_dbg.h"
#include "vpp.h"
#include "vpp_utils.h"
#include "vpp_uc.h"
#include "HidlVpp.h"

using namespace qti_vpp;
using vendor::qti::hardware::vpp::V1_1::HqvMode;
using vendor::qti::hardware::vpp::V1_1::HqvControlType;
using vendor::qti::hardware::vpp::V1_1::VidPropType;
using vendor::qti::hardware::vpp::V1_1::VppCodecType;
using vendor::qti::hardware::vpp::V1_1::VppEventType;

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
TEST_SUITE_INIT(vppHidlUtilsSuiteInit)
{
}

TEST_SUITE_TERM(vppHidlUtilsSuiteTerm)
{
}

TEST_SETUP(vppHidlUtilsTestInit)
{
}

TEST_CLEANUP(vppHidlUtilsTestTerm)
{
}

TEST(vppHidlUtilsHqvCtrl)
{
    uint32_t u32;
    struct hqv_control ctrl;
    HqvControl hidlHqvCtrl;

    // HqvControl to hqv_control
    memset(&ctrl, 0, sizeof(struct hqv_control));
    memset(&hidlHqvCtrl, 0, sizeof(HqvControl));

    // Invalid control types
    hidlHqvCtrl.mode = (HqvMode)HQV_MODE_OFF;
    hidlHqvCtrl.ctrlType = (HqvControlType)HQV_CONTROL_MAX;
    u32 = HidlVpp::HidlVppUtils::hidlToHqvCtrl(ctrl, hidlHqvCtrl);
    DVP_ASSERT_EQUAL(u32, VPP_ERR_PARAM);

    hidlHqvCtrl.ctrlType = (HqvControlType)HQV_CONTROL_CUST;
    u32 = HidlVpp::HidlVppUtils::hidlToHqvCtrl(ctrl, hidlHqvCtrl);
    DVP_ASSERT_EQUAL(u32, VPP_ERR_PARAM);

    hidlHqvCtrl.ctrlType = (HqvControlType)123456;
    u32 = HidlVpp::HidlVppUtils::hidlToHqvCtrl(ctrl, hidlHqvCtrl);
    DVP_ASSERT_EQUAL(u32, VPP_ERR_PARAM);

    // Valid control types
    hidlHqvCtrl.ctrlType = (HqvControlType)HQV_CONTROL_NONE;
    u32 = HidlVpp::HidlVppUtils::hidlToHqvCtrl(ctrl, hidlHqvCtrl);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    hidlHqvCtrl.ctrlType = (HqvControlType)HQV_CONTROL_MEAS;
    u32 = HidlVpp::HidlVppUtils::hidlToHqvCtrl(ctrl, hidlHqvCtrl);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
}

TEST(vppHidlUtilsVppReq)
{
    uint32_t u32;

    struct vpp_requirements req;
    VppRequirements hidlReq;

    // vpp_requirements to VppRequirements
    memset(&req, 0, sizeof(struct vpp_requirements));
    memset(&hidlReq, 0, sizeof(VppRequirements));

    u32 = HidlVpp::HidlVppUtils::vppReqToHidl(NULL, hidlReq);
    DVP_ASSERT_EQUAL(u32, VPP_ERR_PARAM);

    u32 = HidlVpp::HidlVppUtils::vppReqToHidl(&req, hidlReq);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

    // VppRequirements to vpp_requirements
    u32 = HidlVpp::HidlVppUtils::hidlToVppReq(NULL, hidlReq);
    DVP_ASSERT_EQUAL(u32, VPP_ERR_PARAM);

    u32 = HidlVpp::HidlVppUtils::hidlToVppReq(&req, hidlReq);
    DVP_ASSERT_EQUAL(u32, VPP_OK);

}

TEST(vppHidlUtilsVppProp)
{
    uint32_t u32;
    video_property prop;
    VideoProperty hidlProp;

    // VideoProperty to video_property
    memset(&prop, 0, sizeof(struct video_property));
    memset(&hidlProp, 0, sizeof(VideoProperty));

    hidlProp.propertyType = (VidPropType)VID_PROP_MAX;
    u32 = HidlVpp::HidlVppUtils::hidlToVppProp(prop, hidlProp);
    DVP_ASSERT_EQUAL(u32, VPP_ERR_PARAM);

    hidlProp.propertyType = (VidPropType)12345;
    u32 = HidlVpp::HidlVppUtils::hidlToVppProp(prop, hidlProp);
    DVP_ASSERT_EQUAL(u32, VPP_ERR_PARAM);

    hidlProp.propertyType = (VidPropType)VID_PROP_CODEC;
    hidlProp.u.codec.eCodec = (VppCodecType)12222;
    u32 = HidlVpp::HidlVppUtils::hidlToVppProp(prop, hidlProp);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    DVP_ASSERT_EQUAL(prop.codec.eCodec, (enum vpp_codec_type)12222);
}

TEST(vppHidlUtilsVppBuf)
{
    uint32_t u32;
    struct vpp_buffer buf;
    VppBuffer hidlBuf;

    // vpp_buffer to VppBuffer
    memset(&buf, 0, sizeof(struct vpp_buffer));
    memset(&hidlBuf, 0, sizeof(VppBuffer));

    u32 = HidlVpp::HidlVppUtils::vppBufferToHidl(NULL, hidlBuf);
    DVP_ASSERT_EQUAL(u32, VPP_ERR_PARAM);

    buf.pvGralloc = nullptr;
    buf.pixel.fd = buf.extradata.fd = -1;
    buf.cookie = (void *)12345;
    u32 = HidlVpp::HidlVppUtils::vppBufferToHidl(&buf, hidlBuf);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
    DVP_ASSERT_EQUAL(hidlBuf.bufferId, 12345);

    // VppBuffer to vpp_buffer
    memset(&buf, 0, sizeof(struct vpp_buffer));
    memset(&hidlBuf, 0, sizeof(VppBuffer));

    u32 = HidlVpp::HidlVppUtils::hidlToVppBuffer(NULL, hidlBuf);
    DVP_ASSERT_EQUAL(u32, VPP_ERR_PARAM);
}

TEST(vppHidlUtilsVppEvent)
{
    uint32_t u32;
    struct vpp_event e;
    VppEvent hidlEvt;

    // vpp_event to VppEvent
    memset(&e, 0, sizeof(struct vpp_event));
    memset(&hidlEvt, 0, sizeof(VppEvent));

    e.type = (enum vpp_event_type)12345;
    u32 = HidlVpp::HidlVppUtils::vppEventToHidl(e, hidlEvt);
    DVP_ASSERT_EQUAL(u32, VPP_ERR_PARAM);

    e.type = VPP_EVENT_FLUSH_DONE;
    e.flush_done.port = VPP_PORT_INPUT;
    u32 = HidlVpp::HidlVppUtils::vppEventToHidl(e, hidlEvt);
    DVP_ASSERT_EQUAL(u32, VPP_OK);
}

/************************************************************************
 * Global Functions
 ***********************************************************************/
TEST_CASES vppHidlUtilsTests[] = {
    TEST_CASE(vppHidlUtilsHqvCtrl),
    TEST_CASE(vppHidlUtilsVppReq),
    TEST_CASE(vppHidlUtilsVppProp),
    TEST_CASE(vppHidlUtilsVppBuf),
    TEST_CASE(vppHidlUtilsVppEvent),
    TEST_CASE_NULL(),
};

TEST_SUITE(vppHidlUtilsSuite,
           "VppHidlUtilsTests",
           vppHidlUtilsSuiteInit,
           vppHidlUtilsSuiteTerm,
           vppHidlUtilsTestInit,
           vppHidlUtilsTestTerm,
           vppHidlUtilsTests);