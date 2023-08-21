////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file   chiofflinepostproctest.cpp
/// @brief  Test code to validate offline postproc feature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define LOG_TAG "PostProcTest"
#define LOG_NDEBUG 0

#include <sys/mman.h>
#include <utils/Log.h>
#include <time.h>
#include <pthread.h>
#include <cutils/properties.h>
#include <system/camera_metadata.h>

#include <vendor/qti/hardware/camera/postproc/1.0/IPostProcService.h>
#include <vendor/qti/hardware/camera/postproc/1.0/IPostProcSession.h>

#include <android/hardware/graphics/allocator/3.0/IAllocator.h>
#include <android/hardware/graphics/mapper/3.0/IMapper.h>
#include "chiofflinepostproccallbacks.h"

using android::hardware::graphics::allocator::V3_0::IAllocator;
using android::hardware::graphics::common::V1_2::PixelFormat;
using android::hardware::graphics::common::V1_0::BufferUsage;
using android::hardware::graphics::mapper::V3_0::BufferDescriptor;
using android::hardware::graphics::mapper::V3_0::IMapper;
using android::hidl::base::V1_0::IBase;
using android::hardware::hidl_death_recipient;
using android::hardware::hidl_handle;
using android::hardware::hidl_vec;
using android::hardware::Return;
using android::sp;
using android::wp;
using vendor::qti::hardware::camera::postproc::V1_0::BufferParams;
using vendor::qti::hardware::camera::postproc::V1_0::CameraMetadata;
using vendor::qti::hardware::camera::postproc::V1_0::CreateParams;
using vendor::qti::hardware::camera::postproc::V1_0::Error;
using vendor::qti::hardware::camera::postproc::V1_0::HandleParams;
using vendor::qti::hardware::camera::postproc::V1_0::IPostProcService;
using vendor::qti::hardware::camera::postproc::V1_0::IPostProcSession;
using vendor::qti::hardware::camera::postproc::V1_0::JpegCapabilities;
using vendor::qti::hardware::camera::postproc::V1_0::PostProcCapabilities;
using vendor::qti::hardware::camera::postproc::V1_0::PostProcType;
using vendor::qti::hardware::camera::postproc::V1_0::ProcessRequestParams;

using vendor::qti::hardware::camera::postproc::V1_0::implementation::CameraPostProcCallBacks;

sp<IAllocator> g_Allocator;
sp<IMapper> g_Mapper;

pthread_mutex_t mutex;
pthread_cond_t encodeWait;


const native_handle_t* outh;
static uint32_t inbuf_size;

static uint32_t width_12M = 4608;
static uint32_t height_12M = 3456;

static uint32_t width_48M = 8000;
static uint32_t height_48M = 6000;

uint32_t requestId = 0;
uint8_t bServiceDied = false;


struct PostProcClientDeathRecipient : hidl_death_recipient
{
public:
    PostProcClientDeathRecipient() = default;
    virtual ~PostProcClientDeathRecipient() = default;
    virtual void serviceDied(uint64_t cookie, const wp<IBase> &who);
};


void PostProcClientDeathRecipient::serviceDied(uint64_t /*cookie*/, const wp<IBase>& /*who*/)
{
    ALOGE("%s, received from service, PostProcClientDeathRecipient", __FUNCTION__);
    bServiceDied = true;
    pthread_cond_signal(&encodeWait);
}

sp<PostProcClientDeathRecipient> mClientDeathReceipient = nullptr;

static const native_handle_t* GrallocAlloc(PixelFormat format, uint32_t width, uint32_t height, uint64_t usage,
                                            /*output*/ uint32_t *stride)
{
    IMapper::BufferDescriptorInfo info = {
      .width = width,
      .height = height,
      .layerCount = 1,
      .format = format,
      .usage = usage,
    };

    const native_handle_t * handle = nullptr;

    auto descriptor = BufferDescriptor();
    g_Mapper->createDescriptor(info, [&](const auto &/*_error*/, const auto &_descriptor){
        descriptor = _descriptor;
        });

    g_Allocator->allocate(descriptor, 1, [&](const auto &/*_error*/, const auto &_stride, const auto &_buffers)
    {
        if (_buffers.size() == 1)
        {
            g_Mapper->importBuffer(_buffers[0], [&](const auto &/*e*/, const auto &b) {
                                                handle = static_cast<const native_handle_t *>(b);
                                            });
            if (stride)
            {
                *stride = _stride;
            }
        }
    });
    return handle;
}

static void GrallocFree(const native_handle_t *buffer_handle)
{
    if (!buffer_handle)
    {
        return;
    }
    auto buffer = const_cast<native_handle_t *>(buffer_handle);
    g_Mapper->freeBuffer(buffer);
}

int32_t clipIndex = 1;

static uint32_t LoadFrame(char* buffer, uint32_t stride = 64, uint32_t scanline = 64)
{
    FILE* fp;
    uint32_t ret      = 0;
    uint32_t f_w      = width_12M;
    uint32_t f_h      = height_12M;
    uint32_t f_y_off  = 0;
    uint32_t f_uv_off = f_w*f_h;

    char* b_y_off  = 0;
    char* b_uv_off = 0;

    stride = (f_w + stride - 1)/ stride * stride;
    scanline = (f_h + scanline - 1)/ scanline * scanline;

    b_y_off = buffer;
    b_uv_off = buffer + stride * scanline;


    if (1 == clipIndex)
    {
        char file[] = "/data/vendor/camera/3000_4000_NV21.yuv";

        fp = fopen(file, "rb");
        if (!fp)
        {
            ALOGE("%s: fopen failed %s, clipIndex %d error code %s", __FUNCTION__, file, clipIndex, strerror(errno));
            return -1;
        }
    }
    else
    {
        char file[] = "/data/vendor/camera/4102_4032x3008_NV12.yuv";

        fp = fopen(file, "rb");
        if (!fp)
        {
            ALOGE("%s: fopen failed %s, clipIndex %d error code %s", __FUNCTION__, file, clipIndex, strerror(errno));
            return -1;
        }
    }

    ALOGE("%s: f_wxf_h %dx%d, stridexscanline %dx%d, f_uv_off %d,",
          __FUNCTION__, f_w, f_h, stride, scanline, f_uv_off);

    for (uint32_t i = 0; i < f_h; ++i)
    {
        size_t size;

        fseek(fp, f_y_off, SEEK_SET);
        size = fread(b_y_off, 1, f_w, fp);

        if (size != f_w)
        {
            ret = -1;
            ALOGE("readSize %lld is not same as width %lld", (long long)size, (long long)f_w);
            goto error;
        }

        f_y_off += f_w;
        b_y_off += stride;

        if (i < f_h / 2) {
            fseek(fp, f_uv_off, SEEK_SET);
            size = fread(b_uv_off, 1, f_w, fp);
            if (size != f_w) {
                ret = -1;
                ALOGE("readSize %lld is not same as width %lld, file position %d",
                    (long long)size, (long long)f_w, f_uv_off);
                goto error;
            }

            f_uv_off += f_w;
            b_uv_off += stride;
        }
    }

#if 0
    fseek(fp, 0, SEEK_END);
    uint64_t fileSize = ftell(fp);

    ALOGE("Input file %p: size is: %lld", fp, (long long)fileSize);
    fseek(fp, 0, SEEK_SET);
    uint64_t readSize = fread(buffer, 1, fileSize, fp);

    if (readSize != fileSize)
    {
        ret = -1;
        ALOGE("readSize %lld is not same as fileSize %lld", (long long)readSize, (long long)fileSize);
    }
#endif
error:
    fclose(fp);
    return ret;
}


int main()
{
    g_Allocator = IAllocator::getService();
    g_Mapper = IMapper::getService();

    ALOGE("jpegfeature2 test started");
    bServiceDied = false;

    sp<CameraPostProcCallBacks> pCallbacks = new CameraPostProcCallBacks();

    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&encodeWait, NULL);

    char NV12Format[92];
    property_get("persist.vendor.camera.useNV12", NV12Format, "1");

    if (0 == atoi(NV12Format))
    {
        clipIndex = 1;
        width_12M = 3000;
        height_12M = 4000;
    }
    else
    {
        clipIndex = 0;
        width_12M = 4032;// 4608;
        height_12M = 3008;//3456;
    }

    ALOGE("%s: clipIndex %d, atoi %d, updated wxh %dx%d",
          __FUNCTION__, clipIndex, atoi(NV12Format), width_12M, height_12M);

    CreateParams            encCreate;
    BufferParams            bufferParams;
    HandleParams            inHandle;
    HandleParams            outHandle;
    ProcessRequestParams    encParams;
    PostProcCapabilities    postprocCapabilities;
    hidl_vec<PostProcType>  postprocSupported;

    Error error   = Error::INVALID_HANDLE;
    auto  hidl_cb = [&](const auto reqId, const auto tmpError)
    {
        if (Error::NONE != tmpError)
        {
            error = tmpError;
        }
        else
        {
            error     = Error::NONE;
            requestId = reqId;
        }
    };

    ALOGE("connecting to postprocservice ");
    android::sp<IPostProcService> service = IPostProcService::getService("camerapostprocservice");
    if (service == nullptr)
    {
        ALOGE("getService is NULL for postprocservice");
        pCallbacks = nullptr;
        return 0;
    }

    ALOGE("connected to postprocservice ");
    mClientDeathReceipient = new PostProcClientDeathRecipient();

    service->linkToDeath(mClientDeathReceipient, 0L);

    ALOGE("postprocservice is linkToDeath done, go to sleep for 1sec");
    usleep(1000000);
    ALOGE("postprocservice is linkToDeath done, after sleep");

    service->getPostprocTypes([&](const auto& proctypes)
        {postprocSupported = proctypes;});

    service->getCapabilities(PostProcType::JPEG, [&](const auto& encCapabilities)
        {postprocCapabilities = encCapabilities;});

    // update bufferParams
    bufferParams.width  = width_12M;
    bufferParams.height = height_12M;

    if (0 != clipIndex)
    {
        bufferParams.format = (uint32_t) PixelFormat::YCRCB_420_SP;
    }
    else
    {
        bufferParams.format = (uint32_t) PixelFormat::YCBCR_420_888;
    }

    encCreate.input.resize(1);
    encCreate.output.resize(1);
    encCreate.postProcTypeVal   = PostProcType::JPEG;
    encCreate.input[0]          = bufferParams;
    // output differ by just format type
    bufferParams.format         = (uint32_t) 0x00000021; // blob format
    encCreate.output[0]         = bufferParams;

    encParams.input.resize(1);
    encParams.output.resize(1);
    encParams.streamId  = 0;

    android::sp<IPostProcSession> encoder = service->createPostProcessor(encCreate, pCallbacks);
    if (encoder == nullptr)
    {
        ALOGE("createEncoder returned failure");
        service = nullptr;
        encoder = nullptr;
        pCallbacks = nullptr;
        return 0;
    }

    ALOGE("PostprocTest started, pCallbacks %p, service %p, Encoder %p",
          pCallbacks.get(), service.get(), encoder.get());

    const native_handle_t* inh;

    uint32_t strideVal = 64;

    // Allocate little bit more than needed for safety
    inh  = GrallocAlloc(PixelFormat::YCBCR_420_888, width_12M + strideVal,
                        height_12M + strideVal, 0, &strideVal);
    outh = GrallocAlloc(PixelFormat::YCBCR_420_888,width_12M*2,
                        height_12M*2, 0, NULL);

    if (!inh ||!outh)
    {
        ALOGE("inh or outh are NULL");
        GrallocFree(inh);
        GrallocFree(outh);
        service = nullptr;
        encoder = nullptr;
        pCallbacks = nullptr;
        return 0;
    }

    ALOGE("encode started");

    uint8_t status = true;
    uint8_t async  = true;

    CameraMetadata settings;
    camera_metadata_t*  pCameraMetadata = NULL;

    // Add rotation metadata entry for NV12 format
    if (0 == clipIndex)
    {
        pCameraMetadata = allocate_camera_metadata(2, 0);

        const int32_t orientation = 90;
        add_camera_metadata_entry(pCameraMetadata,
                                ANDROID_JPEG_ORIENTATION,
                                &orientation, 1);

#if 0 // This will be enabled with proper metadata
        const int32_t noiseMode = 0;
        add_camera_metadata_entry(pCameraMetadata,
                                ANDROID_NOISE_REDUCTION_MODE,
                                &noiseMode, 1);
#endif

        settings.setToExternal((uint8_t*)pCameraMetadata,
                               get_camera_metadata_size(pCameraMetadata));
    }

    for (uint32_t t = 0; t < 3 && !bServiceDied; t ++)
    {
        char * inbuf;

        inbuf_size = ((width_12M + 63) / 64 * 64) * ((height_12M + 63) / 64 * 64) * 1.5;
        inbuf = (char*)mmap(NULL, inbuf_size, (PROT_READ | PROT_WRITE), MAP_SHARED, inh->data[0], 0);

        if (!inbuf)
        {
            ALOGE("format verify format %d error: mmap input", t);
            break;
        }
        else
        {
            ALOGE("input buffer size is %d, strideVal %d", inbuf_size, strideVal);
        }

        memset(inbuf, 0, inbuf_size);

        if (LoadFrame(inbuf))
        {
            ALOGE("format verify format %d error: load Frame", t);
            munmap(inbuf, inbuf_size);
            status = false;
            break;
        }

        munmap(inbuf, inbuf_size);

        const hidl_handle in_hidl(inh);
        const hidl_handle out_hidl(outh);

        if (0 == clipIndex)
        {
            inHandle.width      = width_12M;
            inHandle.height     = height_12M;
            inHandle.format     = (uint32_t) PixelFormat::YCBCR_420_888;
            outHandle.width     = width_12M;
            outHandle.height    = height_12M;
            outHandle.format    = (uint32_t) 0x00000021;// blob format
        }
        else
        {
            inHandle.width      = width_12M;
            inHandle.height     = height_12M;
            inHandle.format     = (uint32_t) PixelFormat::YCRCB_420_SP;
            outHandle.width     = width_12M;
            outHandle.height    = height_12M;
            outHandle.format    = (uint32_t) 0x00000021;// blob format
        }

        inHandle.bufHandle  = in_hidl;
        outHandle.bufHandle = out_hidl;
        encParams.input[0]  = inHandle;
        encParams.output[0] = outHandle;
        encParams.streamId  = 0;
        encParams.metadata  = settings;

        // HIDL API call to postprocess
        Return<void> ret = encoder->process(encParams, hidl_cb);

        if (Error::NONE != error)
        {
            ALOGE("process request failed");
            status = false;
            break;
        }
        else
        {
            ALOGE("process request queued, requestId %d", requestId);
        }

        if (bServiceDied)
        {
            ALOGE("serviceDied notification received from service ");
        }
        // usleep(1000000);

        // If multiple resolutions needs to be verified with single handle use this code to serialize
#if 0
        ALOGE("Waiting for signal from pCallbacks ");
        pthread_cond_wait(&encodeWait, &mutex);
        clipIndex = !clipIndex;
        ALOGE("Signal from pCallbacks is received, clipIndex %d", clipIndex);
        async = false;
#endif
    }

    if (!bServiceDied && status && async)
    {
        ALOGE("Waiting for signal from pCallbacks ");
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&encodeWait, &mutex);
        pthread_mutex_unlock(&mutex);
        ALOGE("Signal from pCallbacks is received");
    }

    if (!bServiceDied)
    {
        ALOGE("bServiceDied %d, release resources, status %d", bServiceDied, status);
        service->unlinkToDeath(mClientDeathReceipient);
        service.clear();
        encoder.clear();
        encoder = NULL;
        service = NULL;
        //usleep(100000);
    }

    if (NULL != pCameraMetadata)
    {
        free_camera_metadata(pCameraMetadata);
    }

    ALOGE("encode completed, bServiceDied %d", bServiceDied);

    mClientDeathReceipient.clear();
    mClientDeathReceipient = NULL;


    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&encodeWait);
    pCallbacks.clear();
    pCallbacks = NULL;
    ALOGE(" Test case exited");
    return 0;
}
