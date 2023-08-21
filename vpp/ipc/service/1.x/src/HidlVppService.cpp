/*!
 * @file HidlVppService.cpp
 *
 * @cr
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 */

#include "HidlVppService.h"
#include "HidlVpp.h"

#define VPP_LOG_TAG     VPP_LOG_IPC_SERVICE_TAG
#define VPP_LOG_MODULE  VPP_LOG_IPC_SERVICE
#include "vpp_dbg.h"

namespace qti_vpp {

/***************************************************************************
 * Methods from ::vendor::qti::hardware::vpp::V1_1::IHidlVppService follow.
 ****************************************************************************/
Return<sp<V1_1::IHidlVpp>> HidlVppService::getNewVppSession(uint32_t flags)
{
    VPP_UNUSED(flags);

    sp<V1_1::IHidlVpp> hidlVppSession = new HidlVpp();
    return hidlVppSession;
}

/***************************************************************************
 * Methods from ::vendor::qti::hardware::vpp::V1_2::IHidlVppService follow.
 ****************************************************************************/
Return<sp<V1_2::IHidlVpp>> HidlVppService::getNewVppSession_1_2(uint32_t flags)
{
    VPP_UNUSED(flags);

    sp<V1_2::IHidlVpp> hidlVppSession = new HidlVpp();
    return hidlVppSession;
}

/***************************************************************************
 * Methods from ::vendor::qti::hardware::vpp::V1_3::IHidlVppService follow.
 ****************************************************************************/
Return<sp<V1_3::IHidlVpp>> HidlVppService::getNewVppSession_1_3(uint32_t flags)
{
    VPP_UNUSED(flags);

    sp<V1_3::IHidlVpp> hidlVppSession = new HidlVpp();
    return hidlVppSession;
}

}  // namespace qti_vpp
