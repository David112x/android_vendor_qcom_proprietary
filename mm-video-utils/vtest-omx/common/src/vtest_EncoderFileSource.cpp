/*-------------------------------------------------------------------
Copyright (c) 2013-2017 Qualcomm Technologies, Inc. All Rights Reserved.
Qualcomm Technologies Proprietary and Confidential

Copyright (c) 2010 The Linux Foundation. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of The Linux Foundation nor
      the names of its contributors may be used to endorse or promote
      products derived from this software without specific prior written
      permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
--------------------------------------------------------------------------*/

#include "venc/inc/omx_video_common.h"
#include "vtest_ComDef.h"
#include "vtest_Debug.h"
#include "vtest_Thread.h"
#include "vtest_Sleeper.h"
#include "vtest_File.h"
#include "vtest_Time.h"
#include "vtest_SignalQueue.h"
#include "vtest_BufferManager.h"
#include "vtest_EncoderFileSource.h"
#include "vt_ion_allocator.h"
#include "math.h"

#ifdef USE_GBM
#include <gbm.h>
#include <gbm_priv.h>
#else
#include <gralloc_priv.h>
#include "qdMetaData.h"
#endif

//TODO: remove once gerrit : 2680400 is merged
#ifdef USE_GBM
#define VIDC_HAL_PIXEL_FORMAT_NV12_LINEAR_FLEX GBM_FORMAT_NV12_LINEAR_FLEX
#define VIDC_HAL_PIXEL_FORMAT_NV12_UBWC_FLEX GBM_FORMAT_NV12_UBWC_FLEX
#else
#define VIDC_HAL_PIXEL_FORMAT_NV12_LINEAR_FLEX  HAL_PIXEL_FORMAT_NV12_LINEAR_FLEX
#define VIDC_HAL_PIXEL_FORMAT_NV12_UBWC_FLEX HAL_PIXEL_FORMAT_NV12_UBWC_FLEX
#endif

#undef LOG_TAG
#define LOG_TAG "VTEST_ENCODER_FILE_SOURCE"

#ifndef ION_FLAG_CP_PIXEL
#define ION_FLAG_CP_PIXEL 0
#endif

namespace vtest {

static const OMX_S32 MAX_BUFFER_ASSUME = 16;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
EncoderFileSource::EncoderFileSource()
    : ISource(),
      m_nFrames(0),
      m_nFramerate(0),
      m_nFrameWidth(0),
      m_nFrameHeight(0),
      m_nBuffers(0),
      m_nDVSXOffset(0),
      m_nDVSYOffset(0),
      m_pFile(NULL),
      m_bIsProfileMode(OMX_FALSE),
      m_bSecureSession(OMX_FALSE),
      m_bIsVTPath(OMX_FALSE),
      m_pInCopyBuf(NULL),
      m_nInCopyBufSize(0),
      m_bMetaMode(OMX_FALSE),
      m_eYuvColorSpace(ITUR601),
      m_nProfileCount(0),
      m_nColorFormat(0),
      m_nEosType(VIDC_FRAME_FLAG_EOS0),
      m_nConcealColor8bit(DEADVALUE),
      m_sStaticConfig{},
      m_pStreamParser(NULL),
      m_eMetaBufferType(CameraSource){
#ifdef USE_GBM
    deviceFd = -1;
#endif
    snprintf(m_pName, MAX_NAME_LENGTH, "%s", "EncoderFileSource");
    VTEST_MSG_LOW("%s: created...", Name());
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
EncoderFileSource::~EncoderFileSource() {

    VTEST_MSG_LOW("start");
    if (m_pFile != NULL) {
        (void)m_pFile->Close();
        delete m_pFile;
        m_pFile = NULL;
    }
    if (m_pInCopyBuf != NULL) {
        delete m_pInCopyBuf;
        m_pInCopyBuf = NULL;
    }
    if (m_pStreamParser)
    {
        m_pStreamParser->Close(m_pStreamParser);
        free(m_pStreamParser);
        m_pStreamParser = NULL;
    }
#ifdef USE_GBM
        gbm_device_destroy(gbmDevice);
        close(deviceFd);
#endif

    VTEST_MSG_LOW("done");
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
PortBufferCapability EncoderFileSource::GetBufferRequirements(OMX_U32 ePortIndex) {

    PortBufferCapability sBufCap;

    Mutex::Autolock autoLock(m_pLock);
    memset(&sBufCap, 0, sizeof(PortBufferCapability));

    if (ePortIndex == PORT_INDEX_OUT) {
        if (m_bMetaMode) {
            sBufCap.bAllocateBuffer = OMX_TRUE;
            sBufCap.bUseBuffer = OMX_FALSE;
        } else {
            sBufCap.bAllocateBuffer = OMX_FALSE;
            sBufCap.bUseBuffer = OMX_TRUE;
        }
        sBufCap.pSource = this;
        sBufCap.ePortIndex = ePortIndex;
        sBufCap.nMinBufferSize = m_nInCopyBufSize;
        sBufCap.nMinBufferCount = 1;
        sBufCap.nExtraBufferCount = 0;
        sBufCap.nBufferUsage = 0;
    } else {
        VTEST_MSG_ERROR("Error: invalid port selection");
    }
    return sBufCap;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE EncoderFileSource::Configure(CodecConfigType *pConfig,
        BufferManager *pBufManager, ISource *pSource, ISource *pSink) {

    OMX_ERRORTYPE result = OMX_ErrorNone;
    VidcStatus ret = VidcStatusSuccess;

    result = ISource::Configure(pConfig, pBufManager, pSource, pSink);
    if (result != OMX_ErrorNone) {
        VTEST_MSG_ERROR("EncoderFileSource configure failed");
        return result;
    }

    Mutex::Autolock autoLock(m_pLock);

    char sFileName[VTEST_MAX_STRING];
    m_bSecureSession = pConfig->bSecureSession;
    m_bMetaMode = pConfig->bMetaMode;
    m_eYuvColorSpace = pConfig->eYuvColorSpace;
    m_nColorFormat = pConfig->nInputColorFormat;
    m_nConcealColor8bit = pConfig->nConcealColor8bit;
    m_nEosType = pConfig->nEosType;
    m_eMetaBufferType = pConfig->eMetaBufferType;
    VideoResolution res;

    SNPRINTF(sFileName,VTEST_MAX_STRING,"%s%s",sGlobalStaticVideoProp.sInputRoot,pConfig->cInFileName);

    VTEST_MSG_HIGH("MetaMode %s", m_bMetaMode ? "Enabled" : "Disabled");
    VTEST_MSG_MEDIUM("YuvColorSpace : 0x%x", m_eYuvColorSpace);
    VTEST_MSG_MEDIUM("nInputColorFormat : 0x%x", m_nColorFormat);

    if (pConfig->nFrames > 0 &&
        pConfig->nFramerate > 0 &&
        pConfig->nFrameWidth > 0 &&
        pConfig->nFrameHeight > 0 &&
        pConfig->nDVSXOffset >= 0 &&
        pConfig->nDVSYOffset >= 0 &&
        pConfig->nInBufferCount > 0 &&
        pConfig->nInBufferCount <= MAX_BUFFER_ASSUME) {
        m_nFrames = pConfig->nFrames;
        m_nFramerate = pConfig->nFramerate;
        m_nFrameWidth = pConfig->nFrameWidth;
        m_nFrameHeight = pConfig->nFrameHeight;
        m_nBuffers = pConfig->nInBufferCount;
        m_nDVSXOffset = pConfig->nDVSXOffset;
        m_nDVSYOffset = pConfig->nDVSYOffset;
        m_bIsProfileMode = pConfig->bProfileMode;

        /* create a parser instance
         * bailout in case of error .
         * initialize with default otherwise*/
        m_pStreamParser = (VideoStreamParser*)malloc(sizeof(VideoStreamParser));
        if (!m_pStreamParser)
        {
            VTEST_MSG_ERROR("Memory allocation failed for VideoStreamParser!! ");
            return OMX_ErrorUndefined;
        }
        m_pStreamParser->m_nlogMask = sGlobalStaticVideoProp.logMask;
        ret = VideoStreamParserInit(m_pStreamParser);
        if (ret)
        {
            VTEST_MSG_ERROR("VideoStreamParserInit failed with %d!!",ret);
            return OMX_ErrorUndefined;
        }

        /* Configure stream parser with codec details*/
        m_sStaticConfig.sBufferFormat = ParseEnumValue(g_pColorFormat, pConfig->nInputColorFormat); //not valid for decoder
        m_sStaticConfig.sCodecType =  ParseEnumValue(g_pXmlOmxCodecMap, pConfig->eCodec);
        m_sStaticConfig.nFrameCnt = pConfig->nFrames;
        /* for parser superframe is always 1 for encoder */
        m_sStaticConfig.nSuperFrame = 1;
        m_sStaticConfig.nSkipReadAfterNFrames = pConfig->nSkipReadAfterNFrames;

        ret = m_pStreamParser->Configure(m_pStreamParser, &m_sStaticConfig,
                                         sGlobalStaticVideoProp.sInputRoot,
                                                                     FALSE,
                                           sGlobalStaticVideoProp.sOutRoot,
                                                      pConfig->cInFileName,
                                                     pConfig->cOutFileName,
                                                         m_bSecureSession);
        if (ret)
        {
            VTEST_MSG_ERROR("VideoStreamParserConfigure failed with %d!!",ret);
            return OMX_ErrorUndefined;
        }
        /* update superframe after parser configuration is done */
        m_sStaticConfig.nSuperFrame = pConfig->nSuperFrame;

        res.nColorFmt = ParseEnum( (ConfigEnum*)pYUVFormatMap, m_sStaticConfig.sBufferFormat);
        res.nWidth = pConfig->nFrameWidth;
        res.nHeight = pConfig->nFrameHeight;
        res.nStartX = 0;
        res.nStartY = 0;
        res.nCropWidth =  pConfig->nFrameWidth;
        res.nCropHeight = pConfig->nFrameHeight;

        if(res.nColorFmt == V4L2_PIX_FMT_NV12 ||
           res.nColorFmt == V4L2_PIX_FMT_NV21) {
            res.nStride = VENUS_Y_STRIDE(COLOR_FMT_NV12, pConfig->nFrameWidth);
            res.nScanlines = VENUS_Y_SCANLINES(COLOR_FMT_NV12, pConfig->nFrameHeight);
            m_nInCopyBufSize = (OMX_S32)GetFrameSize(m_nColorFormat, res.nStride, res.nScanlines);
        } else if(res.nColorFmt == V4L2_PIX_FMT_SDE_Y_CBCR_H2V2_P010_VENUS) {
            res.nStride = VENUS_Y_STRIDE(COLOR_FMT_P010, pConfig->nFrameWidth);
            res.nScanlines = VENUS_Y_SCANLINES(COLOR_FMT_P010, pConfig->nFrameHeight);
            m_nInCopyBufSize = (OMX_S32)GetFrameSize(m_nColorFormat, res.nStride, res.nScanlines);
        } else if(res.nColorFmt == V4L2_PIX_FMT_RGB32) {
            res.nStride = pConfig->nFrameWidth << 2;
            res.nScanlines = pConfig->nFrameHeight;
            m_nInCopyBufSize = (OMX_S32)GetFrameSize(m_nColorFormat, pConfig->nFrameWidth, res.nScanlines);
        } else {
            res.nStride = pConfig->nFrameWidth;
            res.nScanlines = pConfig->nFrameHeight;
            m_nInCopyBufSize = (OMX_S32)GetFrameSize(m_nColorFormat, m_nFrameWidth, m_nFrameHeight);
        }

        ret = m_pStreamParser->SetResolution(m_pStreamParser, res, (OMX_S32)GetFrameSize(m_nColorFormat, m_nFrameWidth, m_nFrameHeight));
        if (ret)
        {
            VTEST_MSG_ERROR("VideoStreamParserSetResolution failed with %d!!",ret);
            return OMX_ErrorUndefined;
        }
        m_pStreamParser->SetFrameRate(m_pStreamParser, m_nFramerate, 1);
        if ((OMX_S32)m_nConcealColor8bit != DEADVALUE)
            m_pStreamParser->SetConcealColor(m_pStreamParser, m_nConcealColor8bit);
#ifdef USE_GBM
        if (deviceFd < 0) {
            char gbm_drm_name[50] = {0};
            gbm_perform(GBM_PERFORM_GET_DRM_DEVICE_NAME, gbm_drm_name, sizeof(gbm_drm_name));
            int drm_fd = open(gbm_drm_name, O_RDWR | O_CLOEXEC);
            if (drm_fd < 0 ) {
                result = OMX_ErrorBadParameter;
                VTEST_MSG_ERROR("Can't open device");
                return result;
            }
            deviceFd = drm_fd;
        }
        gbmDevice = gbm_create_device(deviceFd);
        if (gbm_device_get_fd(gbmDevice) != deviceFd) {
            result = OMX_ErrorBadParameter;
            VTEST_MSG_ERROR("File descriptor for the gbm device is not matching with deviceFd");
            return result;
        }
        const char* backend_name = gbm_device_get_backend_name(gbmDevice);
        if (!backend_name) {
            result = OMX_ErrorBadParameter;
            VTEST_MSG_ERROR("Invalid backend name");
            return result;
        }
        VTEST_MSG_MEDIUM("gbm device created successfully");
#endif

    } else {
        VTEST_MSG_ERROR("bad params");
        result = OMX_ErrorBadParameter;
    }

    return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE EncoderFileSource::SetBuffer(
        BufferInfo *pBuffer, ISource *pSource, OMX_U32 framenum) {
    (void) framenum;
    OMX_ERRORTYPE result = OMX_ErrorNone;

    result = ISource::SetBuffer(pBuffer, pSource);
    if (result != OMX_ErrorNone) {
        return result;
    }

    VTEST_MSG_LOW("queue push (%p %p)", pBuffer->pHeaderIn, pBuffer->pHeaderOut);
    result = m_pBufferQueue->Push(&pBuffer, sizeof(BufferInfo **));
    return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE EncoderFileSource::AllocateBuffers(BufferInfo **pBuffers,
        OMX_BUFFERHEADERTYPE **pExtraDataBuffers, OMX_U32 nWidth,
        OMX_U32 nHeight, OMX_U32 nBufferCount, OMX_U32 nBufferSize,
        OMX_U32 nExtraDataBufferCount, OMX_U32 nExtraDataBufferSize,
        OMX_U32 ePortIndex, OMX_U32 nBufferUsage) {
    (void)pExtraDataBuffers, (void)nExtraDataBufferCount, (void)nExtraDataBufferSize;
    (void)nWidth, (void)nHeight, (void)nBufferUsage;
    OMX_ERRORTYPE result = OMX_ErrorNone;

    Mutex::Autolock autoLock(m_pLock);
    VTEST_MSG_HIGH("alloc %s size %u count %u",
            OMX_PORT_NAME(ePortIndex), (unsigned int)nBufferSize, (unsigned int)nBufferCount);

    if (ePortIndex != PORT_INDEX_OUT) {
        return OMX_ErrorBadParameter;
    }

    if (pBuffers == NULL) {
        VTEST_MSG_ERROR("%s - invalid input", __FUNCTION__);
        return OMX_ErrorBadParameter;
    }

    *pBuffers = new BufferInfo[nBufferCount];
    if(*pBuffers==NULL)
    {
        VTEST_MSG_ERROR("BufferInfo creation failed with a buffer count %d", nBufferCount);
        return OMX_ErrorBadParameter;
    }

    memset(*pBuffers, 0, sizeof(BufferInfo) * nBufferCount);
    nBufferSize *= m_sStaticConfig.nSuperFrame;

    for (OMX_U32 i = 0; i < nBufferCount; i++) {

        BufferInfo *pBuffer = &((*pBuffers)[i]);

        result = AllocateBuffer(pBuffer, ePortIndex, nBufferSize);
        FAILED1(result, "error allocating buffer");

        VTEST_MSG_HIGH("%s alloc_ct=%u sz=%u handle=%p hdr=(%p %p)",
                OMX_PORT_NAME(ePortIndex), (unsigned int)i+1, (unsigned int)nBufferSize,
                (void *)pBuffer->pHandle, pBuffer->pHeaderIn, pBuffer->pHeaderOut);
    }
    return result;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE EncoderFileSource::ChangeFrameRate(OMX_S32 nFramerate) {

    OMX_ERRORTYPE result = OMX_ErrorNone;
    Mutex::Autolock autoLock(m_pLock);

    if (nFramerate > 0) {
        m_nFramerate = nFramerate;
    } else {
        VTEST_MSG_ERROR("bad frame rate");
        result = OMX_ErrorBadParameter;
    }
    m_pStreamParser->SetFrameRate(m_pStreamParser, m_nFramerate, 1);

    return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE EncoderFileSource::ThreadRun(OMX_PTR pThreadData) {

    (void)pThreadData;
    OMX_ERRORTYPE result = OMX_ErrorNone;
    OMX_BUFFERHEADERTYPE *pHeader = NULL;
    BufferInfo *pBuffer = NULL;
    OMX_S32 nFrameBytes = (OMX_S32)GetFrameSize(m_nColorFormat, m_nFrameWidth, m_nFrameHeight);
    OMX_TICKS nTimeStamp = 0;
    OMX_TICKS nStartTime = 0;
    OMX_TICKS nEndTime = 0;
    OMX_TICKS nDelayTime = 0;
    OMX_S64 nAvgDelayBetweenFrames = (unsigned int)((1000 * 1000) / m_nFramerate);
    VidcStatus ret = VidcStatusSuccess;
    ParserFrameInfoType sFrameInfo;

    //In Skip reading mode, determine reverse reading point based on input buffer count
    if(m_pStreamParser->m_nSkipReadAfterNFrames > 0) {
        BufferPool *pInputBufferPool = m_pBufferManager->GetBufferPool(this, PORT_INDEX_OUT);
        OMX_U32 nBufferCount = pInputBufferPool->nBufferCount;
        if(m_pStreamParser->m_nSkipReadAfterNFrames >= nBufferCount)
        {
            m_pStreamParser->m_nReverseReadingBasis =
                (nBufferCount * m_sStaticConfig.nSuperFrame) / 2 + 2;
            VTEST_MSG_HIGH("m_nReverseReadingBasis %d nBufferCount %d nSuperFrame %d",
                m_pStreamParser->m_nReverseReadingBasis, nBufferCount,
                m_sStaticConfig.nSuperFrame);
        }
        else
        {
            VTEST_MSG_ERROR("Skip reading frame number %d should be equal or larger than buffer count %d.", m_pStreamParser->m_nSkipReadAfterNFrames, nBufferCount);
            m_bThreadStop = OMX_TRUE;
            result = OMX_ErrorUndefined;
        }
    }

    for (OMX_S32 i = 1; !m_bThreadStop; i += m_sStaticConfig.nSuperFrame) {
        // Since frame rate can change at any time, let's make sure that we use
        // the same frame rate for the duration of this loop iteration
        OMX_S32 nFramerate = m_nFramerate;

        // If in live mode we deliver frames in a real-time fashion
        if (m_bIsProfileMode) {
            VTEST_MSG_LOW("delaying frame %lld us", (unsigned int)((1000 * 1000)/ nFramerate) - nDelayTime);
            Sleeper::SleepUs(((1000 * 1000) / nFramerate) - (nDelayTime));
        }

        nStartTime = Time::GetTimeMicrosec();
        result = m_pBufferQueue->Pop(&pBuffer, sizeof(BufferInfo**), 0);
        VTEST_MSG_LOW("queue pop %u of %u...", (unsigned int)i, (unsigned int)m_nFrames);

        if ((pBuffer == NULL) || (result != OMX_ErrorNone)) {
            /* Can only happen if stop is called or someone else ran into an
             * error */
            VTEST_MSG_HIGH("Stopping thread");
            result = OMX_ErrorNone;
            continue;
        }
        pHeader = pBuffer->pHeaderOut;
        pHeader->nOffset = 0;
        pHeader->nTimeStamp = 0;
        pHeader->nFlags = 0;
        pHeader->nFilledLen = 0;

        memset(&sFrameInfo, 0, sizeof(ParserFrameInfoType));
        if(m_bSecureSession)
        {
            unsigned long nBufferFd = -1;

            if (m_bMetaMode)
            {
                struct ion_buffer_data *pIonBuffer =
                                (struct ion_buffer_data *)pHeader->pAppPrivate;
                if (pIonBuffer)
                {
                    m_nInCopyBufSize = pIonBuffer->alloc_data.len;
                    sFrameInfo.nIonfd = pIonBuffer->data_fd;
                    sFrameInfo.nIonHeap = pIonBuffer->alloc_data.heap_id_mask;
                }
            }
            else
            {
                struct pmem *pPmem = (struct pmem *)pHeader->pInputPortPrivate;
                if (pPmem != NULL && pPmem->fd >= 0)
                {
                    m_nInCopyBufSize = pPmem->size;
                    sFrameInfo.nIonfd = pPmem->fd;
                }
            }

            sFrameInfo.nAllocLen = m_nInCopyBufSize;
        }
        else
        {
            sFrameInfo.pBuffer = pHeader->pBuffer;
            sFrameInfo.nAllocLen = pHeader->nAllocLen;
        }
        /* Get Next frame from Parser */
        sFrameInfo.nFrameCounter = i;
        for (int j = 0; j < m_sStaticConfig.nSuperFrame; j++) {
            ret = m_pStreamParser->GetNextFrame(m_pStreamParser, &sFrameInfo);
            if (ret)
            {
                VTEST_MSG_ERROR("GetNextFrame failed with %d!!",ret);
                result = OMX_ErrorUndefined;
                break;
            }
            sFrameInfo.nOffset += nFrameBytes;
        }
        pHeader->nFilledLen = sFrameInfo.nFilledLen * m_sStaticConfig.nSuperFrame;
        if (pHeader->nFilledLen > pHeader->nAllocLen) {
            VTEST_MSG_ERROR("Bytes read can never be more than allocated size! Erroring out");
            SetError();
            result = OMX_ErrorUndefined;
            break;
        }

        if (sFrameInfo.bLastFrame == 1) {
            pHeader->nFlags |= OMX_BUFFERFLAG_EOS;
            VTEST_MSG_HIGH("enable OMX_BUFFERFLAG_EOS on frame %u", (unsigned int)i);
        if (m_nEosType == VIDC_FRAME_FLAG_EOS0)
        {
            m_bThreadStop = OMX_TRUE;
        } else if (m_nEosType == VIDC_FRAME_FLAG_EOS1) {
               if (pHeader->nFilledLen != 0)
               {
                    pHeader->nFlags &= ~OMX_BUFFERFLAG_EOS;
               }
               else
               {
                    m_bThreadStop = OMX_TRUE;
               }
        } else if (m_nEosType == VIDC_FRAME_FLAG_EOS2) {
            pHeader->pBuffer = NULL;
            m_bThreadStop = OMX_TRUE;
        }else {
            pHeader->nFlags &= ~OMX_BUFFERFLAG_EOS;
            m_bThreadStop = OMX_TRUE;
            pBuffer->m_Eos3 = OMX_TRUE;
        }
        }

        pHeader->nFilledLen = nFrameBytes;
        if (m_bIsProfileMode) {
            nTimeStamp = (OMX_TICKS)Time::GetTimeMicrosec();
        } else if (i != 1) {
            // Avoid for first frame to keep its timestamp value as zero
            float ts = (float)(1000 * m_sStaticConfig.nSuperFrame) / nFramerate;
            nTimeStamp = nTimeStamp + ((OMX_TICKS)(round(ts) * 1000));
        }
        pHeader->nTimeStamp = nTimeStamp;

        VTEST_MSG_MEDIUM("delivering frame %u", (unsigned int)i);
        pHeader->nOffset = ((m_nFrameWidth * m_nDVSYOffset) + m_nDVSXOffset) * 3 / 2;
        m_pSink->SetBuffer(pBuffer, this);

        nEndTime = Time::GetTimeMicrosec();
        nDelayTime = nEndTime - nStartTime ;

        if ((nDelayTime + 500) >= nAvgDelayBetweenFrames) {
            nDelayTime = nAvgDelayBetweenFrames;
        } else {
            // some times usleep has overhead of 200us to 400us
            // Add 500us to avoid overhead especially for high frame rate.
            nDelayTime += 500;
        }
        VTEST_SIMPLE_MSG_CONSOLE("%d.", i);
    } // end for-loop
    VTEST_SIMPLE_MSG_CONSOLE("\n\n");
    //clean up
    while(m_pBufferQueue->GetSize() > 0) {
        VTEST_MSG_LOW("cleanup: q-wait (qsize %u)", (unsigned int)m_pBufferQueue->GetSize());
        m_pBufferQueue->Pop(&pBuffer, sizeof(BufferInfo **), 0);
        m_pSink->SetBuffer(pBuffer, this);
    }

    VTEST_MSG_HIGH("thread exiting...");
    return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE EncoderFileSource::FillBuffer(
        OMX_BUFFERHEADERTYPE *pHeader, OMX_S32 nFrameBytes) {

    OMX_ERRORTYPE result = OMX_ErrorNone;
    OMX_S32 nBytesRead;
    OMX_U8 *buffer;

    if (m_pFile == NULL) {
        VTEST_MSG_ERROR("EncoderFileSource file is null");
        return OMX_ErrorBadParameter;
    }

    if (nFrameBytes > (OMX_S32)pHeader->nAllocLen) {
        VTEST_MSG_ERROR("EncoderFileSource buffer too small");
        return OMX_ErrorBadParameter;
    }

    // allocate scratch buffer for secure sessions
    if (m_bSecureSession && m_pInCopyBuf == NULL) {
        m_nInCopyBufSize = pHeader->nAllocLen;
        m_pInCopyBuf = new OMX_U8[m_nInCopyBufSize];
        VTEST_MSG_HIGH("EncoderFileSource secure buf alloc %p (%u bytes)",
            m_pInCopyBuf, (unsigned int)m_nInCopyBufSize);
    }

    if (m_bSecureSession) {
        VTEST_MSG_LOW("select sec buf %p", m_pInCopyBuf);
        buffer = m_pInCopyBuf;
    } else {
        buffer = pHeader->pBuffer;
    }


    result = m_pFile->Read(buffer, m_nFrameWidth,
            m_nFrameHeight, &nBytesRead, 0, m_nColorFormat);
    if (result != OMX_ErrorNone || nFrameBytes != nBytesRead) {
        VTEST_MSG_HIGH("YUV file too small. Seeking to start.");
        result = m_pFile->SeekStart(0);
        result = m_pFile->Read(buffer, m_nFrameWidth,
                m_nFrameHeight, &nBytesRead, 0, m_nColorFormat);
    }
    VTEST_MSG_LOW("EncoderFileSource: read %u bytes", (unsigned int)nBytesRead);

    if (m_bSecureSession) {
        unsigned long nBufferFd = -1;
        VidcStatus status = VidcStatusSuccess;
		ParserFrameInfoType frm = { .pBuffer = buffer };
        if (m_bMetaMode) {
            struct ion_buffer_data *pIonBuffer =
                (struct ion_buffer_data *)pHeader->pAppPrivate;
            if (pIonBuffer) {
                nBufferFd = pIonBuffer->data_fd;
                frm.nIonHeap = pIonBuffer->alloc_data.heap_id_mask;
            }
        } else {
            struct pmem *pPmem = (struct pmem *)pHeader->pInputPortPrivate;
            if (pPmem != NULL && pPmem->fd >= 0) {
                nBufferFd = pPmem->fd;
            }
        }
        frm.nAllocLen = m_nInCopyBufSize;
        frm.nFilledLen = m_nInCopyBufSize;
        frm.nIonfd = nBufferFd;
        status = VideoCryptoCopyBuffer(frm, COPY_NONSECURE_TO_SECURE);
        if (status != VidcStatusSuccess) {
            VTEST_MSG_ERROR("Secure Copy failed, status is %d", status);
			result = OMX_ErrorMax;
        }
    }
    return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE EncoderFileSource::AllocateBuffer(
        BufferInfo *pBuffer, OMX_U32 ePortIndex, OMX_U32 nBufferSize) {

    OMX_ERRORTYPE result = OMX_ErrorNone;
    OMX_S32 nFds = 1;
    OMX_S32 nInts = 3;
    OMX_S32 nFlags = 0;

    OMX_BUFFERHEADERTYPE **pHeaderPtr = pBuffer->GetHeaderPtr(ePortIndex);
    *pHeaderPtr = (OMX_BUFFERHEADERTYPE*)calloc(sizeof(OMX_BUFFERHEADERTYPE), 1);
    if (*pHeaderPtr == NULL) {
        VTEST_MSG_ERROR("Error allocating pHeader");
        return OMX_ErrorInsufficientResources;
    }

    if (m_bSecureSession) {
        nFlags = ION_FLAG_SECURE | ION_FLAG_CP_PIXEL;
    }

#ifdef USE_GBM
     struct gbm_bo *bo = NULL;
     bo = gbm_bo_create(gbmDevice, m_nFrameWidth, m_nFrameHeight, GBM_FORMAT_NV12_ENCODEABLE, GBM_BO_USE_RENDERING);
     if (!bo) {
         result = OMX_ErrorBadParameter;
         VTEST_MSG_ERROR("gbm buffer object creation failed");
         return result;
     }
     (*pHeaderPtr)->nAllocLen = nBufferSize;
     (*pHeaderPtr)->pBuffer = (OMX_U8 *) mmap(NULL, nBufferSize, PROT_READ | PROT_WRITE, MAP_SHARED,bo->ion_fd, 0);
     pBuffer->pHandle = (unsigned long)bo;
     VTEST_MSG_MEDIUM("Allocated buffer of size : %u, fd: %u, handle=0x%lx", (unsigned int)nBufferSize,(unsigned int)bo->ion_fd, bo);
#else
    if (vt_ion_allocate(m_bSecureSession,
                &((*pHeaderPtr)->pAppPrivate), nBufferSize, nFlags) != 0) {
        VTEST_MSG_ERROR("Error allocating ion buffer of size : %u",
                (unsigned int)nBufferSize);
        free(*pHeaderPtr);
        *pHeaderPtr = NULL;
        return OMX_ErrorInsufficientResources;
    }

    struct ion_buffer_data *pIonBuffer =
        (struct ion_buffer_data *)(*pHeaderPtr)->pAppPrivate;
    (*pHeaderPtr)->nAllocLen = nBufferSize;
    (*pHeaderPtr)->pBuffer = (OMX_U8 *)pIonBuffer->data;

    if(m_eMetaBufferType == CameraSource) {
        BufferHandle pMetaHandle = (BufferHandle)calloc((
                    sizeof(struct NativeHandle)+ sizeof(OMX_S32)*(nFds + nInts)), 1);
        if (pMetaHandle == NULL) {
            VTEST_MSG_ERROR("Error allocating MetaHandle");
            vt_ion_free((*pHeaderPtr)->pAppPrivate);
            free(*pHeaderPtr);
            *pHeaderPtr = NULL;
            return OMX_ErrorInsufficientResources;
        }

        pMetaHandle->version = sizeof(NativeHandle);
        pMetaHandle->numFds = nFds;
        pMetaHandle->numInts = nInts;
        pMetaHandle->data[0] = pIonBuffer->data_fd;
        pMetaHandle->data[1] = 0; //offset
        pMetaHandle->data[2] = nBufferSize;
        pMetaHandle->data[3] = m_eYuvColorSpace;
        if (m_nColorFormat == QOMX_COLOR_FORMATYUV420PackedSemiPlanar32mCompressed)
            pMetaHandle->data[3] |= private_handle_t::PRIV_FLAGS_UBWC_ALIGNED;
        pBuffer->pHandle = (unsigned long)pMetaHandle;
        VTEST_MSG_MEDIUM("Allocated buffer of size : %u, fd : %u, data : %p, flags: %d",
            (unsigned int)nBufferSize, (unsigned int)pIonBuffer->data_fd,
            pIonBuffer->data, pMetaHandle->data[3]);
    } else {
        OMX_U32 colorFormat = m_nColorFormat;
        if (m_sStaticConfig.nSuperFrame > 1) {
            if (m_nColorFormat == QOMX_COLOR_FORMATYUV420PackedSemiPlanar32mCompressed) {
                colorFormat = VIDC_HAL_PIXEL_FORMAT_NV12_UBWC_FLEX;
            } else if (m_nColorFormat == QOMX_COLOR_FORMATYUV420PackedSemiPlanar32m) {
                colorFormat = VIDC_HAL_PIXEL_FORMAT_NV12_LINEAR_FLEX;
            }
        }
        private_handle_t * pMetaHandle = new private_handle_t(pIonBuffer->data_fd, nBufferSize, 0, 1, colorFormat, m_nFrameWidth, m_nFrameHeight);

        if (pMetaHandle == NULL) {
            VTEST_MSG_ERROR("Error allocating MetaHandle");
            vt_ion_free((*pHeaderPtr)->pAppPrivate);
            free(*pHeaderPtr);
            *pHeaderPtr = NULL;
            return OMX_ErrorInsufficientResources;
        }

        if (vt_ion_allocate(m_bSecureSession,
                &(pBuffer->pMetadata), sizeof(MetaData_t), nFlags) != 0) {
            VTEST_MSG_ERROR("Error allocating metadata ion buffer of size : %u",
                    (unsigned int)sizeof(MetaData_t));
            free(pHeaderPtr);
            pHeaderPtr = NULL;
            return OMX_ErrorInsufficientResources;
        }

        struct ion_buffer_data *pIonMetaBuffer =
             (struct ion_buffer_data *)pBuffer->pMetadata;
        pMetaHandle->base_metadata = (uint64_t)pIonMetaBuffer->data;
        pMetaHandle->fd_metadata = pIonMetaBuffer->data_fd;
        pMetaHandle->unaligned_width = m_nFrameWidth;
        pMetaHandle->unaligned_height = m_nFrameHeight;
        pMetaHandle->flags |= private_handle_t::PRIV_FLAGS_UBWC_ALIGNED;
        if (m_nColorFormat == QOMX_COLOR_Format32bitRGBA8888) {
            pMetaHandle->flags &= ~(private_handle_t::PRIV_FLAGS_UBWC_ALIGNED);
            pMetaHandle->format = HAL_PIXEL_FORMAT_RGBA_8888;
        }
        if (m_nColorFormat == QOMX_COLOR_Format32bitRGBA8888Compressed) {
            pMetaHandle->format = HAL_PIXEL_FORMAT_RGBA_8888;
            pMetaHandle->width = VENUS_RGB_STRIDE(COLOR_FMT_RGBA8888_UBWC, m_nFrameWidth) >> 2;
        }
        pBuffer->pHandle = (unsigned long)pMetaHandle;
        VTEST_MSG_MEDIUM("Allocated buffer of size : %u, fd : %u, data : %p, flags: %x",
            (unsigned int)nBufferSize, (unsigned int)pIonBuffer->data_fd,
            pIonBuffer->data, pMetaHandle->flags);
    }
#endif

    return result;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OMX_ERRORTYPE EncoderFileSource::FreeBuffer(
        BufferInfo *pBuffer, OMX_U32 ePortIndex) {

    (void)ePortIndex;
    OMX_ERRORTYPE result = OMX_ErrorNone;

    VTEST_MSG_MEDIUM("FreeBuffer: handle : %p pHeader: (%p %p)",
            (void *)pBuffer->pHandle, pBuffer->pHeaderIn, pBuffer->pHeaderOut);

    if (pBuffer->pHandle == 0) {
        return result;
    }

#ifdef USE_GBM
    struct gbm_bo * pMetaHandle = (struct gbm_bo *) pBuffer->pHandle;
    gbm_bo_destroy(pMetaHandle);
    pMetaHandle = NULL;
#else
    if(m_eMetaBufferType == GrallocSource) {
        private_handle_t *pMetaHandle = (private_handle_t *)pBuffer->pHandle;
        if(pBuffer->pMetadata) {
            vt_ion_free(pBuffer->pMetadata);
            pBuffer->pMetadata = NULL;
        }
        free(pMetaHandle);
        pMetaHandle = NULL;
    } else {
        BufferHandle pMetaHandle = (BufferHandle)pBuffer->pHandle;
        free(pMetaHandle);
        pMetaHandle = NULL;
    }
#endif

    OMX_BUFFERHEADERTYPE **pHeaderPtr = pBuffer->GetHeaderPtr(ePortIndex);
    vt_ion_free((*pHeaderPtr)->pAppPrivate);
    free(*pHeaderPtr);
    *pHeaderPtr = NULL;
    FAILED(result, "Error freeing %p", (void *)pBuffer->pHandle);
    return result;
}


} // namespace vtest
