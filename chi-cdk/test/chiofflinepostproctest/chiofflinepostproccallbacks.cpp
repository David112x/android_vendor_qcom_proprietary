////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   chiofflinepostproccallbacks.cpp
/// @brief  HIDL Callback object interface source file. This is used by test module
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOG_TAG "PostProcTest"
#define LOG_NDEBUG 0

#include "chiofflinepostproccallbacks.h"
#include <utils/Log.h>
#include <sys/mman.h>

#include <android/hardware/graphics/allocator/3.0/IAllocator.h>
#include <android/hardware/graphics/mapper/3.0/IMapper.h>

extern pthread_mutex_t mutex;
extern pthread_cond_t encodeWait;
extern const native_handle_t* outh;
extern int32_t clipIndex;
extern uint32_t requestId;
extern uint8_t bServiceDied;

namespace vendor {
namespace qti {
namespace hardware {
namespace camera {
namespace postproc {
namespace V1_0 {
namespace implementation {

uint32_t imagewrite = 0;

void notifyPostProcResult(
    uint32_t lastRequestIDRcvd,
    uint32_t streamId,
    uint32_t outbuf_size)
{
    if (outbuf_size > 0 && imagewrite < 2)
    {
        imagewrite++;

        FILE* fp = NULL;

        if (1 == clipIndex)
        {
            char filename[] = "/data/vendor/camera/3000_4000_NV21.jpg";
            fp = fopen(filename, "wb");
        }
        else
        {
            char filename[] = "/data/vendor/camera/4102_4032x3008_NV12.jpg";
            fp = fopen(filename, "wb");
        }

        if (NULL == fp)
        {
            ALOGE("%s: format verify format %d, clipIndex %d error: create out file",
                  __FUNCTION__, lastRequestIDRcvd, clipIndex);
        }
        else
        {
            char* outbuf = (char*)mmap(NULL, outbuf_size, (PROT_READ | PROT_WRITE), MAP_SHARED, outh->data[0], 0);
            if (NULL == outbuf)
            {
               ALOGE("%s: format verify format %d error: save Frame", __FUNCTION__, lastRequestIDRcvd);
            }
            else
            {
                fwrite(outbuf, 1, outbuf_size, fp);
                munmap(outbuf, outbuf_size);
                ALOGV("format verify format %d success, requestId waiting %d", lastRequestIDRcvd, requestId);
            }
        }
    }
    else if (outbuf_size > 0)
    {
        ALOGI("%s: encode success, receivedReqId %d, waiting for requestId %d, outbuf_size %d, streamId %d, imagewrite %d",
              __FUNCTION__, lastRequestIDRcvd, requestId, outbuf_size, streamId, imagewrite);
    }
    else
    {
        ALOGE("%s: format verify requestId %d error, waiting for requestId %d, signal test to release",
              __FUNCTION__, lastRequestIDRcvd, requestId);
        lastRequestIDRcvd = requestId;
    }

    if (lastRequestIDRcvd == requestId)
    {
        pthread_cond_signal(&encodeWait);
    }
}

CameraPostProcCallBacks::CameraPostProcCallBacks()
{
    ALOGE("[HIDL]CameraPostProcCallBacks constructor called");
}

CameraPostProcCallBacks::~CameraPostProcCallBacks()
{
    imagewrite = 0;
    ALOGE("[HIDL]CameraPostProcCallBacks1 destructor called");
}

// Methods from ::vendor::qti::imsrtpservice::V1_0::IImsMediaServiceCallBacks follow.
Return<void> CameraPostProcCallBacks::notifyResult(
    Error                   err,
    const PostProcResult&   result)
{
    uint32_t frameSize = result.jpegResult.frameSize;

    if (Error::NONE != err)
    {
        ALOGE("%s: error enum %d received", __FUNCTION__, err);
        frameSize = 0;
    }

    notifyPostProcResult(result.requestId, result.streamId, frameSize);

    ALOGI("%s: return control to service, bServiceDied %d", __FUNCTION__, bServiceDied);
    return Void();
}


IPostProcServiceCallBacks *HIDL_FETCH_IPostProcServiceCallBacks(
    const char * /* name */)
{
    return new CameraPostProcCallBacks();
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace jpegenc
}  // namespace camera
}  // namespace hardware
}  // namespace qti
}  // namespace vendor
