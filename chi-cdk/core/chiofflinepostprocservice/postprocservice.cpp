////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   postprocservice.cpp
/// @brief  HIDL interface source file for postproc service.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOG_TAG "ChiOfflinePostproc"
// #define LOG_NDEBUG 0

#include "chiofflinepostprocintf.h"
#include <dlfcn.h>
#include "postprocservice.h"
#include "postprocsession.h"
#include <utils/Log.h>

// NOWHINE FILE CP011:  namespace defined directly
// NOWHINE FILE CP040:  Standard new/delete functions are used
// NOWHINE FILE GR004:  Ignore indentation, else ~30 lines will be added unnecessarily
// NOWHINE FILE GR017:  standard data types are used (no dependency on CHI)
// NOWHINE FILE GR022:  C style casting is used
// NOWHINE FILE NC009:  These are HIDL service files, chi naming convention not followed
// NOWHINE FILE PR007b: Non-library files (HIDL includes are considered as library )
// NOWHINE FILE PR008:  '/' used for includes

namespace vendor {
namespace qti {
namespace hardware {
namespace camera {
namespace postproc {
namespace V1_0 {
namespace implementation {

PFN_CameraPostProc_Create           pCameraPostProcCreate   = NULL;
PFN_CameraPostProc_Process          pCameraPostProcProcess  = NULL;
PFN_CameraPostProc_Destroy          pCameraPostProcDestroy  = NULL;
PFN_CameraPostProc_ReleaseResources pCameraPostProcRelease  = NULL;


// Methods from ::vendor::qti::hardware::camera::postproc::V1_0::IPostProcService follow.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcService::PostProcService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PostProcService::PostProcService()
{
    void* pLib = dlopen("libcamerapostproc.so", RTLD_NOW | RTLD_LOCAL);

    if (NULL != pLib)
    {
        pCameraPostProcCreate   = (PFN_CameraPostProc_Create)
                                    dlsym(pLib, "CameraPostProc_Create");
        pCameraPostProcProcess  = (PFN_CameraPostProc_Process)
                                    dlsym(pLib, "CameraPostProc_Process");
        pCameraPostProcDestroy  = (PFN_CameraPostProc_Destroy)
                                    dlsym(pLib, "CameraPostProc_Destroy");
        pCameraPostProcRelease  = (PFN_CameraPostProc_ReleaseResources)
                                    dlsym(pLib, "CameraPostProc_ReleaseResources");

        if ((NULL == pCameraPostProcCreate)       ||
            (NULL == pCameraPostProcDestroy)      ||
            (NULL == pCameraPostProcProcess)      ||
            (NULL == pCameraPostProcRelease))
        {
            ALOGE("%s: postproc function pointers are NULL, dlsym failed, %p, %p, %p %p",
                  __FUNCTION__, pCameraPostProcCreate, pCameraPostProcProcess,
                  pCameraPostProcDestroy, pCameraPostProcRelease);
        }
    }
    else
    {
        ALOGE("%s: No postproc lib, dlopen failed", __FUNCTION__);
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcService::getPostprocTypes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Return<void> PostProcService::getPostprocTypes(
    getPostprocTypes_cb hidlCb)
{
    hidl_vec<PostProcType> postprocSupported;
    postprocSupported.resize(1);
    postprocSupported[0] = PostProcType::JPEG;
    hidlCb(postprocSupported);
    return Void();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcService::getCapabilities
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Return<void> PostProcService::getCapabilities(
    PostProcType        postprocenum,
    getCapabilities_cb  hidlCb)
{
    PostProcCapabilities procCapability;
    memset(&procCapability, 0, sizeof(PostProcCapabilities));

    if (PostProcType::JPEG == postprocenum)
    {
        procCapability.jpegStream.maxStreamsSupported = NumberOfStreamsSupported;
    }

    hidlCb(procCapability);
    return Void();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcService::createPostProcessor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Return<sp<IPostProcSession>> PostProcService::createPostProcessor(
    const CreateParams&                     rCreateParams,
    const sp<IPostProcServiceCallBacks>&    rCallback)
{
    PostProcServiceDeathRecipient*  pDeathRecipient = new PostProcServiceDeathRecipient(this);
    PostProcSession*                pSession        = NULL;

    if (NULL == pDeathRecipient)
    {
        ALOGE("%s: Malloc failed for pDeathRecipient object", __FUNCTION__);
    }
    else if ((NULL == pCameraPostProcCreate)       ||
             (NULL == pCameraPostProcDestroy)      ||
             (NULL == pCameraPostProcProcess))
    {
        ALOGE("%s: Function pointers are NULL, dlopen failed", __FUNCTION__);
        delete pDeathRecipient;
        pDeathRecipient = NULL;
    }
    else
    {
        pSession = new PostProcSession();

        if (NULL != pSession)
        {
            PostProcError result = pSession->Initialize(rCreateParams, rCallback, pDeathRecipient);

            if (PostProcSuccess != result)
            {
                // pDeathRecipient will be deleted as part of pSession
                delete pSession;
                pSession        = NULL;
                pDeathRecipient = NULL;
                ALOGE("%s: PostProcSession Initialize failed, error %d", __FUNCTION__, result);
            }
        }
        else
        {
            ALOGE("%s: malloc failure for pSession", __FUNCTION__);
            delete pDeathRecipient;
            pDeathRecipient = NULL;
        }
    }

    if (NULL != pSession)
    {
        // Keep session pointer in service
        m_pPostProcInstance.insert(reinterpret_cast<IPostProcSession*>(pSession));
        rCallback->linkToDeath(pDeathRecipient, (uint64_t)(pSession));
        ALOGI("cookie value is %p is used for linkToDeath, death object %p", pSession, pDeathRecipient);
    }

    return sp<IPostProcSession>(pSession);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcService::serviceDied
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PostProcService::serviceDied(
    uint64_t cookie)
{
    PostProcSession* pSession = (PostProcSession*)cookie;

    // If the cookie value is not found in set, that means session is already destroyed
    // Ignore serviceDied notification in such cases
    // NOWHINE CP006:
    std::set<sp<IPostProcSession>>::iterator it =
                        m_pPostProcInstance.find(reinterpret_cast<IPostProcSession*>(pSession));
    if (it != m_pPostProcInstance.end())
    {
        ALOGE("serviceDied received from client, cookie %p", pSession);
        pSession->ClearCallback();
        pSession->abort();
        // erase meantime sp also decrease the reference
        m_pPostProcInstance.erase(it);
    }
    else
    {
        ALOGE("%s: Ignore serviceDied notification, %p is already deleted", __FUNCTION__, pSession);
        pSession = NULL;
    }

}


// Methods from ::android::hidl::base::V1_0::IBase follow.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HIDL_FETCH_IPostProcService
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IPostProcService* HIDL_FETCH_IPostProcService(
    const char* pName)
{
    ALOGV("%s: service name %s", __FUNCTION__, pName);
    return new PostProcService();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace postproc
}  // namespace camera
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
