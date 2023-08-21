////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   postprocserviceintf.cpp
/// @brief  HIDL interface source file for postproc service initialization.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOG_TAG "ChiOfflinePostproc"
#define LOG_NDEBUG 0

#include <hidl/HidlTransportSupport.h>
#include "postprocservice.h"
#include "postprocserviceintf.h"
#include <utils/Log.h>

// NOWHINE FILE NC009:  These are HIDL service files, chi naming convention not followed
// NOWHINE FILE CP040:  Standard new/delete functions are used
// NOWHINE FILE PR008:  Include related error
// NOWHINE FILE PR007b: Include related error

using vendor::qti::hardware::camera::postproc::V1_0::IPostProcService;
using vendor::qti::hardware::camera::postproc::V1_0::implementation::PostProcService;
using android::hardware::configureRpcThreadpool;

///< Number of parallel threads needed in each session
const uint32_t NumberOfThreadsNeeded = 4;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RegisterIPostProcService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RegisterIPostProcService()
{
    static android::sp<IPostProcService> s_pIPostProcService = NULL;

    if (s_pIPostProcService == NULL)
    {
        ALOGI("postprocservice register started");
        s_pIPostProcService = new PostProcService();

        if (s_pIPostProcService->registerAsService("camerapostprocservice") != android::OK)
        {
            ALOGE("postprocservice register failed");
        }
        else
        {
            ALOGI("postprocservice register successfully");
            configureRpcThreadpool(NumberOfThreadsNeeded, true);
        }
    }

    return;
}
