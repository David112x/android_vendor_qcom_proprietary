/*!
 * @file VppService.cpp
 *
 * @cr
 * Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 *
 * @services
 */

#include <vendor/qti/hardware/vpp/1.1/IHidlVppService.h>

#include <hidl/LegacySupport.h>
#include <hwbinder/ProcessState.h>
#include <cutils/properties.h>

#include <android/hidl/manager/1.2/IClientCallback.h>
#include <android/hidl/manager/1.2/IServiceManager.h>

#define VPP_LOG_TAG     VPP_LOG_IPC_SERVICE_TAG
#define VPP_LOG_MODULE  VPP_LOG_IPC_SERVICE
#include "vpp_dbg.h"
#include "vpp.h"
#include "HidlVppService.h"

#include "vpp_configstore.h"

using android::hidl::manager::V1_2::IServiceManager;
using android::hidl::manager::V1_2::IClientCallback;
using android::hardware::defaultServiceManager1_2;

using android::hardware::Return;
using android::hardware::Status;
using android::sp;

using vendor::qti::hardware::vpp::V1_1::IHidlVppService;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;

static struct {
    bool isBooted;
    sp<IServiceManager> mgr = nullptr;
    sp<IHidlVppService> service = nullptr;

} stService;

#define VPP_SERVICE_NAME "vppService"

static void vShutdown()
{
    if (!stService.isBooted)
    {
        return;
    }

    uint32_t u32 = vpp_shutdown();
    stService.isBooted = false;
    LOGE_IF(u32 != VPP_OK, "vpp_shutdown failed, u32=%u", u32);
}

class VppServiceClientCb : public IClientCallback {
    Return<void> onClients(const sp<IBase> &base, bool clients) override
    {
        (void)base;
        bool bRet = false;

        if (!clients)
        {
            LOG_ALWAYS("onClients: shutting down vppservice...");
            vShutdown();

            LOG_ALWAYS("unregistering dynamic hal callback...");
            stService.mgr->unregisterClientCallback(stService.mgr, this);

            bRet = stService.mgr->tryUnregister(VPP_SERVICE_NAME, VPP_SERVICE_NAME,
                                                stService.service);

            if (bRet)
            {
                LOG_ALWAYS("tryUnregister was successful, exiting process...");
                exit(VPP_OK);
            }

            LOG_ALWAYS("tryUnregister failed, vppservice will stay alive");
        }

        return Status::ok();
    }
};

static bool isDynamicHalEnabled()
{
    bool bIsDisabled = bVppConfigStore_GetBool("vpp",
                                              "disable_dynamic_hal",
                                              static_cast<uint32_t>(false));
    LOG_ALWAYS("dynamic hal is %s", bIsDisabled ? "disabled" : "enabled");
    return !bIsDisabled;
}

int main(int /* argc */, char**)
{
    uint32_t u32;

    // In the case with dynamic HAL where the service is started on demand,
    // vpp needs to be booted before registering with the service manager
    // otherwise it is possible that the client can get an instance of VPP
    // and call init() before we call boot.
    LOG_ALWAYS("Starting vppservice...");
    u32 = vpp_boot();
    stService.isBooted = u32 == VPP_OK;
    LOGE_IF(u32 != VPP_OK, "vpp_boot failed, u32=%u", u32);

#ifdef ARCH_ARM_32
    // Hwbinder optimzation: vppservice page-watermark is 1 : max 1 kernel
    // page is used for this process i.e. 4096 Bytes.
    // Because binder driver divides total space into
    // two parts : sync and async space, initWithMmapSize(4096 * 2)
    android::hardware::ProcessState::initWithMmapSize((size_t)(8192));
#endif //ARCH_ARM_32
    configureRpcThreadpool(10, true /* callerWillJoin */);

    // Setup hwbinder service
    sp<IHidlVppService> service = new qti_vpp::HidlVppService();
    if (service == NULL)
    {
        LOGE("HidlVppService is null");
        return VPP_ERR;
    }

    u32 = service->registerAsService(VPP_SERVICE_NAME);
    if (u32 != 0)
    {
        LOGE("failed to register '%s', u32=%u", VPP_SERVICE_NAME, u32);
        vShutdown();
        return VPP_ERR;
    }

    if (isDynamicHalEnabled())
    {
        bool bRet = false;
        stService.service = service;
        stService.mgr = defaultServiceManager1_2();
        LOG_ALWAYS("dynamic HAL enabled, registering callback...");
        bRet = stService.mgr->registerClientCallback(VPP_SERVICE_NAME,
                                                     VPP_SERVICE_NAME,
                                                     service,
                                                     new VppServiceClientCb);

        // If this occurs, dynamic HAL will be disabled and this service
        // will never be shutdown as the callback will never be called.
        LOGE_IF(!bRet, "failed to register client callback");
    }

    joinRpcThreadpool();
    return 0;
}
