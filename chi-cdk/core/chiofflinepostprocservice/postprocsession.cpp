////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   postprocsession.cpp
/// @brief  HIDL interface source file for postproc session.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOG_TAG "ChiOfflinePostproc"
// #define LOG_NDEBUG 0

#include "chiofflinepostprocintf.h"
#include "postprocsession.h"
#include <utils/Log.h>

// NOWHINE FILE CP006:  STL keyword used (deque, vector)
// NOWHINE FILE CP011:  namespace defined directly
// NOWHINE FILE CP040:  Standard new/delete functions are used
// NOWHINE FILE GR004:  Ignore indentation, else ~30 lines will be added unnecessarily
// NOWHINE FILE GR017:  standard data types are used (no dependency on CHI)
// NOWHINE FILE GR022:  C style casting is used
// NOWHINE FILE NC009:  These are HIDL service files, chi naming convention not followed
// NOWHINE FILE PR007b: Non-library files (HIDL includes are considered as library )
// NOWHINE FILE PR008:  '/' used for includes

using MapperError2_0 =::android::hardware::graphics::mapper::V2_0::Error;
using MapperError3_0 =::android::hardware::graphics::mapper::V3_0::Error;

namespace vendor {
namespace qti {
namespace hardware {
namespace camera {
namespace postproc {
namespace V1_0 {
namespace implementation {

extern PFN_CameraPostProc_Create            pCameraPostProcCreate;
extern PFN_CameraPostProc_Process           pCameraPostProcProcess;
extern PFN_CameraPostProc_Destroy           pCameraPostProcDestroy;
extern PFN_CameraPostProc_ReleaseResources  pCameraPostProcRelease;

///< Amount of time (in millisec) to wait before releasing resources
///< Do not keep too small value, else pipeline will be created for each request
const uint32_t WaitTime = 10000;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ConvertFromHidl
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static PostProcError ConvertFromHidl(
    const CameraMetadata& rMetadataSrc,
    camera_metadata_t**   ppMetadata)
{
    PostProcError result = PostProcSuccess;

    if (NULL != ppMetadata)
    {
        *ppMetadata = NULL;

        uint32_t metadataSize = rMetadataSrc.size();

        if (0 != metadataSize)
        {
            const uint8_t* pData = rMetadataSrc.data();

            // sanity check the size of CameraMetadata match underlying camera_metadata_t
            if (get_camera_metadata_size((camera_metadata_t*)pData) != metadataSize)
            {
                result = PostProcIncorrectMetadata;
                ALOGE("%s: input CameraMetadata is corrupt!", __FUNCTION__);
            }
            else
            {
                void* pDest = malloc(metadataSize);

                if (NULL != pDest)
                {
                    *ppMetadata = copy_camera_metadata(pDest, metadataSize, (camera_metadata_t*)pData);

                    if (NULL == *ppMetadata)
                    {
                        ALOGE("%s: CameraMetadata copy failed", __FUNCTION__);
                        result = PostProcMallocFail;
                        free(pDest);
                    }
                }
                else
                {
                    ALOGE("%s: CameraMetadata malloc failed", __FUNCTION__);
                    result = PostProcMallocFail;
                }
            }
        }
        else
        {
            ALOGE("%s: CameraMetadata is not proper", __FUNCTION__);
            result = PostProcIncorrectMetadata;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcSession::PostProcSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PostProcSession::PostProcSession()
{
    pthread_mutex_init(&m_bufferMutex, NULL);
    pthread_cond_init(&m_bufferSignal, NULL);
    pthread_cond_init(&m_abortSignal, NULL);

    m_postProcLoop  = PROCLOOP_NOTSTARTED;
    m_validEntries  = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcSession::~PostProcSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PostProcSession::~PostProcSession()
{
    ALOGI("%s: begin, cookie %p", __FUNCTION__, this);

    if (m_spDeathRecipient != NULL)
    {
        m_spDeathRecipient->eraseEntry(this);

        if (m_spClientcallback != NULL)
        {
            m_spClientcallback->unlinkToDeath(m_spDeathRecipient);
        }
    }

    m_spClientcallback.clear();
    m_spClientcallback = NULL;

    /* Before freeing queue, acquire mutex */
    pthread_mutex_lock(&m_bufferMutex);
    ALOGV("sessionQueue size %zu, m_postProcLoop %d",
          m_sessionQueue.size(), m_postProcLoop);

    while (m_sessionQueue.size() > 0)
    {
        FreePostProcParams(m_sessionQueue.front());
        m_sessionQueue.pop_front();
    }

    // Check if thread started or not
    if (PROCLOOP_STARTED == m_postProcLoop)
    {
        m_postProcLoop = PROCLOOP_STOP;
        pthread_cond_signal(&m_bufferSignal);
        pthread_cond_signal(&m_abortSignal);
        pthread_mutex_unlock(&m_bufferMutex);

        ALOGV("%s: waiting for thread to terminate, status %d",
              __FUNCTION__, m_postProcLoop);
        // Wait for thread to join
        (void) pthread_join(m_procLoopThread, NULL);
        ALOGV("%s: thread terminated, status %d",
              __FUNCTION__, m_postProcLoop);
    }
    else
    {
        pthread_mutex_unlock(&m_bufferMutex);
    }

    m_spMapper.clear();
    m_spMapper2_0.clear();
    m_spDeathRecipient.clear();

    m_spMapper          = NULL;
    m_spMapper2_0       = NULL;
    m_spDeathRecipient  = NULL;


    /* Free postproc sessions created for different streams */
    for (uint32_t index = 0; index < m_validEntries; ++index)
    {
        if (NULL != m_pPostProc[index])
        {
            pCameraPostProcDestroy(m_pPostProc[index]);
            m_pPostProc[index] = NULL;
        }
    }

    m_validEntries = 0;
    pthread_mutex_destroy(&m_bufferMutex);
    pthread_cond_destroy(&m_bufferSignal);
    pthread_cond_destroy(&m_abortSignal);
    ALOGV("%s: end", __FUNCTION__);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcSession::ClearCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PostProcSession::ClearCallback()
{
    m_spClientcallback.clear();
    m_spClientcallback = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcSession::InitializeSession
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PostProcError PostProcSession::InitializeSession(
    uint32_t index)
{
    PostProcError         result = PostProcSuccess;
    PostProcCreateParams* pEntry = &(m_createParams[index]);

    m_initFlag[index]  = true;
    m_pPostProc[index]  = pCameraPostProcCreate(pEntry);

    if (NULL != m_pPostProc[index])
    {
        ALOGV("%s: PostProc session created %p, index %d",
              __FUNCTION__, m_pPostProc[index], index);
    }
    else
    {
        result = PostProcMallocFail;
        ALOGE("%s: PostProc session creation failed", __FUNCTION__);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcSession::PostProcLoop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PostProcSession::PostProcLoop()
{
    PostProcSessionParams* pPostProcParam = NULL;

    // Acquire mutex before changing thread status
    pthread_mutex_lock(&m_bufferMutex);
    m_postProcLoop = PROCLOOP_STARTED;
    pthread_mutex_unlock(&m_bufferMutex);

    ALOGI("PostProcessLoop started, m_postProcLoop %d cookie %p", m_postProcLoop, this);

    while (PROCLOOP_STARTED == m_postProcLoop)
    {
        int                 waitRes = 0;
        Error               err     = Error::POSTPROC_FAIL;
        PostProcResultInfo  status  = { 0 };

        pthread_mutex_lock(&m_bufferMutex);

        if ((m_sessionQueue.size() < 1) && (PROCLOOP_STARTED == m_postProcLoop))
        {
            ALOGI("%s: wait for signal, m_releaseResources %d", __FUNCTION__, m_releaseResources);

            /* If resources are released wait for signal indefinitely */
            if (true == m_releaseResources)
            {
                pthread_cond_wait(&m_bufferSignal, &m_bufferMutex);
                /* Reset flag as resources will be acquired */
                m_releaseResources = false;
            }
            else
            {
                struct timespec timeout             = {0};
                time_t          timeoutSeconds      = (WaitTime / 1000UL);
                long            timeoutNanoseconds  = (WaitTime % 1000UL) * 1000000UL;
                clock_gettime(CLOCK_REALTIME, &timeout);
                timeoutSeconds     += static_cast<time_t>((timeout.tv_nsec + timeoutNanoseconds) / 1000000000UL);
                timeoutNanoseconds  = (timeout.tv_nsec + timeoutNanoseconds) % 1000000000UL;
                timeout.tv_sec     += timeoutSeconds;
                timeout.tv_nsec     = timeoutNanoseconds;

                waitRes = pthread_cond_timedwait(&m_bufferSignal, &m_bufferMutex, &timeout);

                if ((ETIMEDOUT == waitRes) && (0 == m_sessionQueue.size()))
                {
                    /* Release resources held by postproc sessions */
                    for (uint32_t index = 0; index < m_validEntries; ++index)
                    {
                        if (NULL != m_pPostProc[index])
                        {
                            pCameraPostProcRelease(m_pPostProc[index]);
                        }
                    }

                    m_releaseResources = true;
                    pthread_mutex_unlock(&m_bufferMutex);
                    continue;
                }
            }
        }

        if (PROCLOOP_STARTED != m_postProcLoop)
        {
            pthread_mutex_unlock(&m_bufferMutex);
            break;
        }

        pPostProcParam = m_sessionQueue.front();
        m_sessionQueue.pop_front();
        pthread_mutex_unlock(&m_bufferMutex);

        if (NULL == pPostProcParam)
        {
            ALOGE("%s: pPostProcParam is NULL, queue size %zu, waitRes %d, m_releaseResources %d",
                  __FUNCTION__, m_sessionQueue.size(), waitRes, m_releaseResources);
            continue;
        }
        else if (true == pPostProcParam->valid)
        {
            if (PROCLOOP_STARTED == m_postProcLoop)
            {
                ALOGV("%s: postproc request given", __FUNCTION__);
                err    = Error::NONE;
                status = pCameraPostProcProcess(m_pPostProc[pPostProcParam->streamId],
                                                pPostProcParam);

                if (POSTPROCFAILED == status.result)
                {
                    err = Error::POSTPROC_FAIL;
                }
                else if (POSTPROCBADSTATE == status.result)
                {
                    err = Error::DEVICE_BAD_STATE;
                }
            }
            else
            {
                ALOGE("Postprocloop is aborted, m_postProcLoop %d", m_postProcLoop);
                err = Error::ABORT;
            }
        }

        ALOGV("%s: notifyResult called, m_Clientcallback %p, streamId %d",
              __FUNCTION__, m_spClientcallback.get(),
              pPostProcParam->streamId);

        // If loop is still active then only give the result to client
        // If loop is destroyed, then client has no interest for result notification
        if ((m_spClientcallback != NULL) && (PROCLOOP_STARTED == m_postProcLoop))
        {
            PostProcResult postprocResult;

            postprocResult.requestId            = pPostProcParam->frameNum;
            postprocResult.streamId             = pPostProcParam->streamId;
            postprocResult.postProcTypeVal      = PostProcType::JPEG;
            postprocResult.jpegResult.frameSize = status.size;

            auto status = m_spClientcallback->notifyResult(err, postprocResult);

            if (status.isOk() == false)
            {
                ALOGE("client unable to send response, Exception %s",
                      status.description().c_str());
                m_spClientcallback.clear();
                m_spClientcallback = NULL;
            }
        }

        FreePostProcParams(pPostProcParam);
        CheckAbortStatus();
    }

    // Update enum to STOPPED so that thread can be joined by main process
    m_postProcLoop = PROCLOOP_STOPPED;
    ALOGV("%s: PostProcessLoop thread is exited", __FUNCTION__);
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcSession::FreePostProcParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PostProcSession::FreePostProcParams(
    PostProcSessionParams* pPostProcParam)
{
    if (NULL != pPostProcParam)
    {
        for (uint32_t index = 0; index < pPostProcParam->inHandle.size(); ++index)
        {
            if (NULL != pPostProcParam->inHandle[index].phHandle)
            {
                if (m_spMapper != NULL)
                {
                    m_spMapper->freeBuffer((void*)(pPostProcParam->inHandle[index].phHandle));
                }
                else
                {
                    m_spMapper2_0->freeBuffer((void*)(pPostProcParam->inHandle[index].phHandle));
                }

                pPostProcParam->inHandle[index].phHandle = NULL;
            }
        }

        for (uint32_t index = 0; index < pPostProcParam->outHandle.size(); ++index)
        {
            if (NULL != pPostProcParam->outHandle[index].phHandle)
            {
                if (m_spMapper != NULL)
                {
                    m_spMapper->freeBuffer((void*)(pPostProcParam->outHandle[index].phHandle));
                }
                else
                {
                    m_spMapper2_0->freeBuffer((void*)(pPostProcParam->outHandle[index].phHandle));
                }

                pPostProcParam->outHandle[index].phHandle = NULL;
            }
        }

        if (NULL != pPostProcParam->pMetadata)
        {
            free(pPostProcParam->pMetadata);
        }

        pPostProcParam->inHandle.resize(0);
        pPostProcParam->outHandle.resize(0);
        delete pPostProcParam;
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcSession::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PostProcError PostProcSession::Initialize(
    const CreateParams&                     createParams,
    const sp<IPostProcServiceCallBacks>&    callback,
    PostProcServiceDeathRecipient*          pServiceDiedObj)
{
    PostProcError result = PostProcSuccess;

    m_postProcLoop      = PROCLOOP_NOTSTARTED;
    m_validEntries      = 1;
    m_frameNum          = 0;
    m_spClientcallback  = callback;
    m_spDeathRecipient  = pServiceDiedObj;
    m_releaseResources  = false;

    if (PostProcType::JPEG == createParams.postProcTypeVal)
    {
        PostProcBufferParams  bufferParams;
        PostProcCreateParams* pEntry = &(m_createParams[0]);

        m_pPostProc[0]  = NULL;
        m_initFlag[0]   = false;

        bufferParams.width  = createParams.input[0].width;
        bufferParams.height = createParams.input[0].height;
        bufferParams.format = createParams.input[0].format;
        pEntry->streamId    = 0;
        pEntry->processMode = YUVToJPEG;
        pEntry->inBuffer    = bufferParams;
        // Output handle params
        bufferParams.width  = createParams.output[0].width;
        bufferParams.height = createParams.output[0].height;
        bufferParams.format = createParams.output[0].format;
        pEntry->outBuffer   = bufferParams;
        ALOGI("%s: input wXh %dx%d, format %d,  output wXh %dx%d, format %d ",
              __FUNCTION__, pEntry->inBuffer.width, pEntry->inBuffer.height, pEntry->inBuffer.format,
              pEntry->outBuffer.width, pEntry->outBuffer.height, pEntry->outBuffer.format);
    }
    else
    {
        ALOGE("%s: unsupported postproc type %d", __FUNCTION__, createParams.postProcTypeVal);
        result = PostProcUnsupportedType;
    }

    if (PostProcSuccess == result)
    {
        m_spMapper2_0 = NULL;
        m_spMapper    = IMapper::getService();

        if (m_spMapper == NULL)
        {
            ALOGE("%s: Mapper service 3.0 is not available", __FUNCTION__);
            m_spMapper2_0 = IMapper_2_0::getService();

            if (m_spMapper2_0 == NULL)
            {
                ALOGE("Mapper 2.0 version is also not available");
                result = PostProcFail;
            }
        }
    }

    if (PostProcSuccess == result)
    {
        result = InitializeSession(0);
    }

    if (PostProcSuccess == result)
    {
        int err = pthread_create(&m_procLoopThread, NULL, PostProcLoopThreadFunction, this);

        if (err < 0)
        {
            ALOGE("%s: postproc loop thread creation failed, m_postProcLoop %d",
                  __FUNCTION__, m_postProcLoop);
            result = PostProcThreadInitFailed;
        }
        else
        {
            pthread_mutex_lock(&m_bufferMutex);

            if (PROCLOOP_STARTED != m_postProcLoop)
            {
                m_postProcLoop = PROCLOOP_INITIALIZED;
            }

            pthread_mutex_unlock(&m_bufferMutex);
            ALOGV("%s: PostProc loop thread creation succeeded, m_postProcLoop %d",
                  __FUNCTION__, m_postProcLoop);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcSession::FreePostProcParams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PostProcError PostProcSession::UpdateHandlePtr(
    const hidl_vec<HandleParams>&       rHandleParams,
    std::vector<PostProcHandleParams>&  rNativeHdlParams)
{
    PostProcError   result      = PostProcSuccess;
    MapperError2_0  mapperErr2  = MapperError2_0::NONE;
    MapperError3_0  mapperErr   = MapperError3_0::NONE;
    uint32_t        index       = 0;

    const native_handle_t* phHandle;

    auto  hidlCb2_0 = [&](const auto& e, const auto& hdl)
    {
        mapperErr2  = e;
        phHandle     = static_cast<const native_handle_t *>(hdl);
    };

    auto  hidlCb = [&](const auto& e, const auto& hdl)
    {
        mapperErr   = e;
        phHandle     = static_cast<const native_handle_t *>(hdl);
    };

    for (index = 0; index < rHandleParams.size(); ++index)
    {
        PostProcHandleParams nativeHandleParams;

        phHandle = rHandleParams[index].bufHandle.getNativeHandle();

        if (m_spMapper != NULL)
        {
            m_spMapper->importBuffer(phHandle, hidlCb);
        }
        else
        {
            m_spMapper2_0->importBuffer(phHandle, hidlCb2_0);
        }

        if ((MapperError3_0::NONE == mapperErr) && (MapperError2_0::NONE == mapperErr2))
        {
            nativeHandleParams.phHandle = phHandle;
            nativeHandleParams.width    = rHandleParams[index].width;
            nativeHandleParams.height   = rHandleParams[index].height;
            nativeHandleParams.format   = rHandleParams[index].format;
            rNativeHdlParams.push_back(nativeHandleParams);
        }
        else
        {
            break;
        }
    }

    if (index != rHandleParams.size())
    {
        ALOGE("%s: hidl handle to native handle mapping failed, failed index %d", __FUNCTION__, index);
        result = PostProcIncorrectHandle;
    }

    return result;
}

// Methods from ::vendor::qti::hardware::camera::postproc::V1_0::IPostProcSession follow.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcSession::process
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Return<void> PostProcSession::process(
    const ProcessRequestParams& rSessionParam,
    process_cb                  hidlCb)
{
    ALOGV("%s: begin", __FUNCTION__);

    PostProcSessionParams*  pPostProcParam  = NULL;
    PostProcError           result          = PostProcSuccess;
    Error                   err             = Error::NONE;

    if (m_validEntries < rSessionParam.streamId)
    {
        ALOGE("%s: iStreamId %d is invalid, max valid index %d",
              __FUNCTION__, rSessionParam.streamId, m_validEntries);
        err = Error::BAD_STREAMID;
    }
    else if (m_spClientcallback == NULL)
    {
        ALOGE("%s: m_Clientcallback is NULL, decode response cannot be given back", __FUNCTION__);
        err = Error::INVALID_CALLBACK_PTR;
    }
    else if (false == m_initFlag[rSessionParam.streamId])
    {
        result = InitializeSession(rSessionParam.streamId);
    }

    if (NULL == m_pPostProc[rSessionParam.streamId])
    {
        ALOGE("%s: iStreamId %d, PostProc session init failed",
              __FUNCTION__, rSessionParam.streamId);
        err = Error::SESSION_NOT_INIT;
    }
    else if (Error::NONE == err)
    {
        pPostProcParam = new PostProcSessionParams;

        if (NULL != pPostProcParam)
        {
            pPostProcParam->pMetadata = NULL;
            pPostProcParam->inHandle.resize(0);
            pPostProcParam->outHandle.resize(0);

            result = ConvertFromHidl(rSessionParam.metadata, &pPostProcParam->pMetadata);

            if (PostProcMallocFail == result)
            {
                ALOGE("%s: pMetadata malloc failed", __FUNCTION__);
                err = Error::MALLOC_FAIL;
                delete pPostProcParam;
            }
            else if (PostProcSuccess != result)
            {
                ALOGE("%s: metadata is not proper", __FUNCTION__);
            }
        }
        else
        {
            ALOGE("%s: pPostProcParam malloc failed", __FUNCTION__);
            err = Error::MALLOC_FAIL;
        }
    }

    if (Error::NONE == err)
    {
        result = UpdateHandlePtr(rSessionParam.input, pPostProcParam->inHandle);

        if (PostProcSuccess == result)
        {
            result = UpdateHandlePtr(rSessionParam.output, pPostProcParam->outHandle);
        }

        if (PostProcSuccess == result)
        {
            pPostProcParam->streamId  = rSessionParam.streamId;
            pPostProcParam->valid     = true;
            pPostProcParam->frameNum  = m_frameNum++;

            pthread_mutex_lock(&m_bufferMutex);
            m_sessionQueue.push_back(pPostProcParam);
            pthread_cond_signal(&m_bufferSignal);
            pthread_mutex_unlock(&m_bufferMutex);
        }
        else
        {
            FreePostProcParams(pPostProcParam);
            err = Error::INVALID_HANDLE;
            ALOGE("%s: invalid handle(s), report error", __FUNCTION__);
        }
    }

    hidlCb(m_frameNum - 1, err);
    ALOGV("%s: end", __FUNCTION__);
    return Void();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// PostProcSession::abort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Return<Error> PostProcSession::abort()
{
    Error err = Error::NONE;

    pthread_mutex_lock(&m_bufferMutex);

    for (auto pIterator = m_sessionQueue.begin(); pIterator != m_sessionQueue.end(); ++pIterator)
    {
        if (NULL != *pIterator)
        {
            (*pIterator)->valid   = false;
        }
    }

    if ((PROCLOOP_STARTED == m_postProcLoop) && (m_sessionQueue.size() > 1))
    {
        m_abortPostProc = true;
        ALOGV("%s: waiting for postproc to finish, aborted %zu requests",
              __FUNCTION__, m_sessionQueue.size());
        pthread_cond_wait(&m_abortSignal, &m_bufferMutex);
        m_abortPostProc   = false;
    }

    pthread_mutex_unlock(&m_bufferMutex);
    return err;
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace postproc
}  // namespace camera
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
