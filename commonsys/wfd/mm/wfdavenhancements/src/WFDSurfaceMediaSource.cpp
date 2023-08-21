/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
//#define LOG_NDEBUG 0
#define LOG_TAG "WFDSurfaceMediaSource"

#include <inttypes.h>

#include "WFDSurfaceMediaSource.h"
#include <OMX_IVCommon.h>
#include <media/hardware/HardwareAPI.h>
#include <media/hardware/MetadataBufferType.h>

#include <ui/GraphicBuffer.h>
#include <gui/BufferItem.h>
#include <gui/ISurfaceComposer.h>
#include <OMX_Component.h>

#include <utils/Log.h>
#include <utils/String8.h>

#include <private/gui/ComposerService.h>
#ifdef QTI_BSP
#include <gralloc_priv.h>
#endif

WFDMediaBuffer::WFDMediaBuffer(size_t size) :
    m_Size(size),
    mRefCount(0),
    m_RangeOffset(0),
    m_RangeLength(size),
    m_pObserver(NULL)
{
    m_Data = malloc(size);
}

WFDMediaBuffer::~WFDMediaBuffer()
{
    if (m_Data)
    {
        free(m_Data);
        m_Data = NULL;
    }
}

void WFDMediaBuffer::setObserver(WFDMediaBufferObserver *observer)
{
    m_pObserver = observer;
}


bool WFDSmsKeyValueMap::findInt64(unsigned int key, int64_t& value) const
{
    bool bOk = false;
    value = 0;

    Int64MapType::const_iterator iter =  m_Int64Map.find(key);
    if (iter != m_Int64Map.end())
    {
        bOk = true;
        value = iter->second;
    }

    return bOk;
}

bool WFDSmsKeyValueMap::setInt64(unsigned int key, int64_t value)
{
    m_Int64Map[key] = value;
    return true;
}

unsigned int WFDMediaBuffer::refcount() const
{
    return mRefCount;
}

void WFDMediaBuffer::release()
{
    int prevCount = __sync_fetch_and_sub(&mRefCount, 1);

    if (1 == prevCount)
    {
        if (m_pObserver)
        {
            //ALOGV("WFDMediaBuffer %p: signalBufferReturned", __FUNCTION__, m_pObserver);
            m_pObserver->signalBufferReturned(this);
        }
    }
}

void WFDMediaBuffer::add_ref()
{
    int prevCount = __sync_fetch_and_add(&mRefCount, 1);
    //ALOGE("WFDMediaBuffer %p: refcnt=%u (add_ref)", prevCount);
}

void* WFDMediaBuffer::data()
{
    return m_Data;
}

unsigned int WFDMediaBuffer::range_offset() const
{
    return m_RangeOffset;
}

unsigned int WFDMediaBuffer::range_length() const
{
    return m_RangeLength;
}

unsigned int WFDMediaBuffer::size() const
{
    return m_Size;
}

WFDSmsKeyValueMap& WFDMediaBuffer::meta_data()
{
    return WFDSmsKeyValueMap;
}

namespace android {

WFDSurfaceMediaSource::WFDSurfaceMediaSource(uint32_t bufferWidth, uint32_t bufferHeight,
                                             WFDSMSConsumerType eType, bool UBWCSupport) :
    mWidth(bufferWidth),
    mHeight(bufferHeight),
    mCurrentSlot(BufferQueue::INVALID_BUFFER_SLOT),
    mNumPendingBuffers(0),
    mFrameRate(30),
    mStarted(false),
    mNumFramesReceived(0),
    mNumFramesEncoded(0),
    mFirstFrameTimestamp(0),
    mMaxAcquiredBufferCount(4),  // XXX double-check the default
    mUseAbsoluteTimestamps(false),
    mConsumerUsageFlags(0) {
    ALOGV("WFDSurfaceMediaSource");

    if (bufferWidth == 0 || bufferHeight == 0) {
        ALOGE("Invalid dimensions %dx%d", bufferWidth, bufferHeight);
    }

    BufferQueue::createBufferQueue(&mProducer, &mConsumer);
    mConsumer->setDefaultBufferSize(bufferWidth, bufferHeight);

    uint64_t flags = GRALLOC_USAGE_HW_TEXTURE;

    if (eType == WFD_SMS_CONSUMER_HW) {
       flags |= GRALLOC_USAGE_HW_VIDEO_ENCODER;
       if(UBWCSupport) {
#ifdef QTI_BSP
           ALOGV("Setting GRALLOC_USAGE_PRIVATE_ALLOC_UBWC flag");
           flags |= GRALLOC_USAGE_PRIVATE_ALLOC_UBWC;
#endif
       }
#ifdef QTI_BSP
       flags |= GRALLOC_USAGE_PRIVATE_WFD;
#endif
    } else if(eType == WFD_SMS_CONSUMER_SW) {
       flags =  GRALLOC_USAGE_SW_READ_OFTEN;
#ifdef QTI_BSP
       flags |= GRALLOC_USAGE_PRIVATE_WFD;
#endif
    }

    mConsumerUsageFlags = flags;
    ALOGE("set consumerUsage flags = %#" PRIx64, mConsumerUsageFlags);
    mConsumer->setConsumerUsageBits(mConsumerUsageFlags);

    sp<ISurfaceComposer> composer(ComposerService::getComposerService());

    // Note that we can't create an sp<...>(this) in a ctor that will not keep a
    // reference once the ctor ends, as that would cause the refcount of 'this'
    // dropping to 0 at the end of the ctor.  Since all we need is a wp<...>
    // that's what we create.
    wp<ConsumerListener> listener = static_cast<ConsumerListener*>(this);
    sp<BufferQueue::ProxyConsumerListener> proxy = new BufferQueue::ProxyConsumerListener(listener);

    status_t err = mConsumer->consumerConnect(proxy, false);
    if (err != NO_ERROR) {
        ALOGE("WFDSurfaceMediaSource: error connecting to BufferQueue: %s (%d)",
                strerror(-err), err);
    }
}

WFDSurfaceMediaSource::~WFDSurfaceMediaSource() {
    ALOGV("~WFDSurfaceMediaSource");
}

void WFDSurfaceMediaSource::setFrameAvailableListener(
        const sp<FrameAvailableListener>& listener) {
    ALOGV("setFrameAvailableListener");
    Mutex::Autolock lock(mMutex);
    mFrameAvailableListener = listener;
}

void WFDSurfaceMediaSource::dump(String8& result) const
{
    char buffer[1024] = {0};
    dump(result, "", buffer, 1024);
}

void WFDSurfaceMediaSource::dump(
        String8& result,
        const char* /* prefix */,
        char* buffer,
        size_t /* SIZE */) const
{
    Mutex::Autolock lock(mMutex);

    result.append(buffer);
    mConsumer->dumpState(result, "");
}

status_t WFDSurfaceMediaSource::setFrameRate(int32_t fps)
{
    ALOGV("setFrameRate");
    Mutex::Autolock lock(mMutex);
    const int MAX_FRAME_RATE = 60;
    if (fps < 0 || fps > MAX_FRAME_RATE) {
        return BAD_VALUE;
    }
    mFrameRate = fps;
    return OK;
}

MetadataBufferType WFDSurfaceMediaSource::metaDataStoredInVideoBuffers() const {
    ALOGV("isMetaDataStoredInVideoBuffers");
    return kMetadataBufferTypeANWBuffer;
}

int32_t WFDSurfaceMediaSource::getFrameRate( ) const {
    ALOGV("getFrameRate");
    Mutex::Autolock lock(mMutex);
    return mFrameRate;
}

status_t WFDSurfaceMediaSource::start(int32_t bufferCount)
{
    ALOGV("start");

    Mutex::Autolock lock(mMutex);

    if (mStarted)
    {
        ALOGE("SMS already started");
        return BAD_VALUE;
    }

    mStartTimeNs = 0;

        if (bufferCount <= 1) {
            ALOGE("bufferCount %d is too small", bufferCount);
            return BAD_VALUE;
        }

        mMaxAcquiredBufferCount = bufferCount;

    status_t err =
        mConsumer->setMaxAcquiredBufferCount(mMaxAcquiredBufferCount);

    if (err != OK) {
        return err;
    }

    mNumPendingBuffers = 0;
    mStarted = true;

    return OK;
}

status_t WFDSurfaceMediaSource::setMaxAcquiredBufferCount(size_t count) {
    ALOGV("setMaxAcquiredBufferCount(%zu)", count);
    Mutex::Autolock lock(mMutex);

    if (count <= 1)
    {
        ALOGE("setMaxAcquiredBufferCount %d not GT 1");
        return BAD_VALUE;
    }

    mMaxAcquiredBufferCount = count;

    return OK;
}

status_t WFDSurfaceMediaSource::setUseAbsoluteTimestamps() {
    ALOGV("setUseAbsoluteTimestamps");
    Mutex::Autolock lock(mMutex);
    mUseAbsoluteTimestamps = true;

    return OK;
}

status_t WFDSurfaceMediaSource::stop()
{
    ALOGV("stop");
    Mutex::Autolock lock(mMutex);

    if (!mStarted) {
        return OK;
    }

    mStarted = false;
    mFrameAvailableCondition.signal();

    while (mNumPendingBuffers > 0) {
        ALOGI("Still waiting for %zu buffers to be returned.",
                mNumPendingBuffers);

#if DEBUG_PENDING_BUFFERS
        for (size_t i = 0; i < mPendingBuffers.size(); ++i) {
            ALOGI("%d: %p", i, mPendingBuffers.itemAt(i));
        }
#endif

        mMediaBuffersAvailableCondition.wait(mMutex);
    }

    mMediaBuffersAvailableCondition.signal();

    return mConsumer->consumerDisconnect();
}

// Pass the data to the MediaBuffer. Pass in only the metadata
// Note: Call only when you have the lock
void WFDSurfaceMediaSource::passMetadataBuffer_l(WFDMediaBuffer **buffer,
        ANativeWindowBuffer *bufferHandle) const {
    *buffer = new WFDMediaBuffer(sizeof(VideoNativeMetadata));
    if (*buffer == NULL) {
        ALOGE("Cannot allocate memory for metadata mediabuffer!");
        return;
    }
    VideoNativeMetadata *data = (VideoNativeMetadata *)(*buffer)->data();
    if (data == NULL) {
        ALOGE("Cannot allocate memory for metadata buffer!");
        return;
    }
    data->eType = metaDataStoredInVideoBuffers();
    data->pBuffer = bufferHandle;
    data->nFenceFd = -1;
    ALOGV("handle = %p, offset = %zu, length = %zu",
            bufferHandle, (*buffer)->range_length(), (*buffer)->range_offset());
}

bool WFDSurfaceMediaSource::read(
        WFDMediaBuffer **buffer) {
    ALOGV("read");
    Mutex::Autolock lock(mMutex);

    *buffer = NULL;

    while (mStarted && mNumPendingBuffers == mMaxAcquiredBufferCount) {
        mMediaBuffersAvailableCondition.wait(mMutex);
    }

    // Update the current buffer info
    // TODO: mCurrentSlot can be made a bufferstate since there
    // can be more than one "current" slots.

    BufferItem item;
    // If the recording has started and the queue is empty, then just
    // wait here till the frames come in from the client side
    while (mStarted) {

        status_t err = mConsumer->acquireBuffer(&item, 0);
        if (err == BufferQueue::NO_BUFFER_AVAILABLE) {
            // wait for a buffer to be queued
            mFrameAvailableCondition.wait(mMutex);
        } else if (err == OK) {
            err = item.mFence->waitForever("WFDSurfaceMediaSource::read");
            if (err) {
                ALOGW("read: failed to wait for buffer fence: %d", err);
            }

            // First time seeing the buffer?  Added it to the SMS slot
            if (item.mGraphicBuffer != NULL) {
                mSlots[item.mSlot].mGraphicBuffer = item.mGraphicBuffer;
            }
            mSlots[item.mSlot].mFrameNumber = item.mFrameNumber;

            // check for the timing of this buffer
            if (mNumFramesReceived == 0 && !mUseAbsoluteTimestamps) {
                mFirstFrameTimestamp = item.mTimestamp;
                // Initial delay
                if (mStartTimeNs > 0) {
                    if (item.mTimestamp < mStartTimeNs) {
                        // This frame predates start of record, discard
                        mConsumer->releaseBuffer(
                                item.mSlot, item.mFrameNumber, EGL_NO_DISPLAY,
                                EGL_NO_SYNC_KHR, Fence::NO_FENCE);
                        continue;
                    }
                    mStartTimeNs = item.mTimestamp - mStartTimeNs;
                }
            }
            item.mTimestamp = mStartTimeNs + (item.mTimestamp - mFirstFrameTimestamp);

            mNumFramesReceived++;

            break;
        } else {
            ALOGE("read: acquire failed with error code %d", err);
            return false;
        }

    }

    // If the loop was exited as a result of stopping the recording,
    // it is OK
    if (!mStarted) {
        ALOGE("Read: WFDSurfaceMediaSource is stopped. Returning ERROR_END_OF_STREAM.");
        return false;
    }

    mCurrentSlot = item.mSlot;

    // First time seeing the buffer?  Added it to the SMS slot
    if (item.mGraphicBuffer != NULL) {
        mSlots[item.mSlot].mGraphicBuffer = item.mGraphicBuffer;
    }
    mSlots[item.mSlot].mFrameNumber = item.mFrameNumber;

    mCurrentBuffers.push_back(mSlots[mCurrentSlot].mGraphicBuffer);

    mNumFramesEncoded++;
    // Pass the data to the MediaBuffer. Pass in only the metadata
    if (mSlots[mCurrentSlot].mGraphicBuffer == NULL) {
        ALOGV("Read: SurfaceMediaSource mGraphicBuffer is null. Returning"
                          "ERROR_END_OF_STREAM.");
        return false;
    }

    passMetadataBuffer_l(buffer, mSlots[mCurrentSlot].mGraphicBuffer->getNativeBuffer());

    (*buffer)->setObserver(this);
    (*buffer)->add_ref();
    ALOGV("Frames encoded = %d, timestamp = %" PRId64 ", time diff = %" PRId64,
           mNumFramesEncoded);

    ++mNumPendingBuffers;

#if DEBUG_PENDING_BUFFERS
    mPendingBuffers.push_back(*buffer);
#endif

    ALOGV("returning mbuf %p", *buffer);

    return true;
}

buffer_handle_t
WFDSurfaceMediaSource::getMediaBufferHandle(WFDMediaBuffer *buffer) {
    // need to convert to char* for pointer arithmetic and then
    // copy the byte stream into our handle

    if (NULL == buffer)
    {
        return NULL;
    }

    VideoNativeMetadata *data = (VideoNativeMetadata * )buffer->data();
    return (data ? (data->pBuffer ? data->pBuffer->handle : NULL): NULL);
}

void WFDSurfaceMediaSource::signalBufferReturned(WFDMediaBuffer *buffer) {
    ALOGV("signalBufferReturned");

    bool foundBuffer = false;

    Mutex::Autolock lock(mMutex);

    buffer_handle_t bufferHandle = getMediaBufferHandle(buffer);

    for (size_t i = 0; i < mCurrentBuffers.size(); i++) {
        if (mCurrentBuffers[i]->handle == bufferHandle) {
            mCurrentBuffers.removeAt(i);
            foundBuffer = true;
            break;
        }
    }

    if (!foundBuffer) {
        ALOGW("returned buffer was not found in the current buffer list");
    }

    for (int id = 0; id < BufferQueue::NUM_BUFFER_SLOTS; id++) {
        if (mSlots[id].mGraphicBuffer == NULL) {
            continue;
        }

        if (bufferHandle == mSlots[id].mGraphicBuffer->handle) {
            ALOGV("Slot %d returned, matches handle = %p", id,
                    mSlots[id].mGraphicBuffer->handle);

            mConsumer->releaseBuffer(id, mSlots[id].mFrameNumber,
                                        EGL_NO_DISPLAY, EGL_NO_SYNC_KHR,
                    Fence::NO_FENCE);

            buffer->setObserver(0);
            buffer->release();

            foundBuffer = true;
            break;
        }
    }

    if (!foundBuffer) {
        ALOGE("signalBufferReturned: bogus buffer");
    }

#if DEBUG_PENDING_BUFFERS
    for (size_t i = 0; i < mPendingBuffers.size(); ++i) {
        if (mPendingBuffers.itemAt(i) == buffer) {
            mPendingBuffers.removeAt(i);
            break;
        }
    }
#endif

    --mNumPendingBuffers;
    mMediaBuffersAvailableCondition.broadcast();
}

// Part of the BufferQueue::ConsumerListener
void WFDSurfaceMediaSource::onFrameAvailable(const BufferItem& /* item */) {
    ALOGV("onFrameAvailable");

    sp<FrameAvailableListener> listener;
    { // scope for the lock
        Mutex::Autolock lock(mMutex);
        mFrameAvailableCondition.broadcast();
        listener = mFrameAvailableListener;
    }

    if (listener != NULL) {
        ALOGV("actually calling onFrameAvailable");
        listener->onFrameAvailable();
    }
}

// WFDSurfaceMediaSource hijacks this event to assume
// the prodcuer is disconnecting from the BufferQueue
// and that it should stop the recording
void WFDSurfaceMediaSource::onBuffersReleased() {
    ALOGV("onBuffersReleased");

    Mutex::Autolock lock(mMutex);

    mFrameAvailableCondition.signal();

    for (int i = 0; i < BufferQueue::NUM_BUFFER_SLOTS; i++) {
       mSlots[i].mGraphicBuffer = 0;
    }
}

void WFDSurfaceMediaSource::onSidebandStreamChanged() {
    ALOG_ASSERT(false, "WFDSurfaceMediaSource can't consume sideband streams");
}

sp<GraphicBuffer> WFDSurfaceMediaSource::getGraphicBuffer(
    buffer_handle_t bufferHandle)
{
    Mutex::Autolock lock(mMutex);

    for (int id = 0; id < BufferQueue::NUM_BUFFER_SLOTS; id++) {
        if (mSlots[id].mGraphicBuffer == NULL) {
            continue;
        }

        if (bufferHandle == mSlots[id].mGraphicBuffer->handle) {
            return mSlots[id].mGraphicBuffer;
        }
    }

    ALOGE("GetGraphicBuffer: failed to find graphic buffer for %p",
          bufferHandle);

    return NULL;
}

WFDSurfaceMediaSource::
ImageAccessor::ImageAccessor(android::sp<android::WFDSurfaceMediaSource> pSMS,
                             buffer_handle_t bufferHandle) :
    m_pGraphicBuffer(NULL),
    m_pImg(NULL)
{
    m_pGraphicBuffer = pSMS->getGraphicBuffer(bufferHandle);

    if (m_pGraphicBuffer != NULL) {
        // don't send private wfd flag to code which has no use for it.
        uint32_t flags =
            (pSMS->mConsumerUsageFlags & ~GRALLOC_USAGE_PRIVATE_WFD);

        // map the image ptr
        m_pGraphicBuffer->lock(flags, &m_pImg);
    }
    else
    {
        ALOGE("ImageAccessor: failed to map graphic buffer from %p",
              bufferHandle);
    }
}

WFDSurfaceMediaSource::
ImageAccessor::~ImageAccessor()
{
    // unmap the image ptr
    if (m_pGraphicBuffer != NULL) {
        m_pGraphicBuffer->unlock();
    }
}

void *WFDSurfaceMediaSource::ImageAccessor::getImagePtr() const
{
    return m_pImg;
}

} // end of namespace android
