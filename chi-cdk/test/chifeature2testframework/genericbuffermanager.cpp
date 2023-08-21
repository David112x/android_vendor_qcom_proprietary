////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  genericbuffermanager.cpp
/// @brief Implementation for generic buffer manager.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "genericbuffermanager.h"

CamxMemAllocFunc              GenericBufferManager::m_pCamxMemAlloc;
CamxMemReleaseFunc            GenericBufferManager::m_pCamxMemRelease;
CamxMemGetImageSizeFunc       GenericBufferManager::m_pCamxMemGetImageSize;
CamxMemGetImageSizeStrideFunc GenericBufferManager::m_pCamxMemGetImageSizeStride;

GenericBufferManager::GenericBufferManager() :
    m_pInputImageQueue(NULL),
    m_pEmptyQueue(NULL),
    m_pFilledQueue(NULL),
    m_bBufferAvailable(false),
    m_outFileName(""),
    m_streamType(-1)
{
    // Initialize mutex and condition member variables
    m_pQueueMutex = Mutex::Create();
    m_pQueueCond = Condition::Create();
}

GenericBufferManager::~GenericBufferManager()
{
    m_pInputImageQueue  = NULL;
    m_pEmptyQueue       = NULL;
    m_pFilledQueue      = NULL;
    m_bBufferAvailable = false;
    m_outFileName      = "";
    m_streamType       = -1;

    // Destroy mutex and condition member variables
    if (NULL != m_pQueueMutex)
    {
        m_pQueueMutex->Destroy();
        m_pQueueMutex = NULL;
    }
    if (NULL != m_pQueueCond)
    {
        m_pQueueCond->Destroy();
        m_pQueueCond = NULL;
    }
}

/*******************************************************************************************************************************
*   GenericBufferManager::LoadBufferLibs
*
*   @brief
*       Initialize the function pointers from the library
*   @return
*       0 on success or -1 on failure
*******************************************************************************************************************************/
int GenericBufferManager::LoadBufferLibs(
    void* pLib) //[in] library to be loaded
{
    // CamxMemGetImageSizeFunc available on both Android and Windows
    m_pCamxMemGetImageSize = reinterpret_cast<CamxMemGetImageSizeFunc>(ChxUtils::LibGetAddr(pLib, "CamxMemGetImageSize"));
    if (m_pCamxMemGetImageSize == NULL)
    {
        CF2_LOG_ERROR("CamxMemGetImageSize not available in the library");
        return -1;
    }

    m_pCamxMemGetImageSizeStride =
        reinterpret_cast<CamxMemGetImageSizeStrideFunc>(ChxUtils::LibGetAddr(pLib, "CamxMemGetImageSizeStride"));
    if (m_pCamxMemGetImageSizeStride == NULL)
    {
        CF2_LOG_ERROR("CamxMemGetImageSizeStride not available in the library");
        return -1;
    }

#ifndef _LINUX
    // CamxMemAllocFunc and CamxMemReleaseFunc not available on Android
    m_pCamxMemAlloc = reinterpret_cast<CamxMemAllocFunc>(ChxUtils::LibGetAddr(pLib, "CamxMemAlloc"));
    if (m_pCamxMemAlloc)
    {
        CF2_LOG_ERROR("CamxMemAllocFunc not available in the library");
        return -1;
    }

    m_pCamxMemRelease = reinterpret_cast<CamxMemReleaseFunc>(ChxUtils::LibGetAddr(pLib, "CamxMemRelease"));
    if (m_pCamxMemRelease)
    {
        CF2_LOG_ERROR("CamxMemRelease not available in the library");
        return -1;
    }
#endif // _LINUX
    return 0;
}

/*******************************************************************************************************************************
*   GenericBufferManager::Initialize
*
*   @brief
*       Initialize new instance of GenericBufferManager
*   @return
*       Pointer to initialized GenericBufferManager object or NULL on failure
*******************************************************************************************************************************/
GenericBufferManager* GenericBufferManager::Initialize(
    int cameraId,                       //[in] camera Id for generating buffers
    GenericStream* pConfiguredStream,   //[in] streams to for generating buffers
    int streamId,                       //[in] stream Id for generating buffers
    const char* testCaseName,           //[in] test case name
    const char* inputFileName,          //[in] inputFileName for generating buffers
    bool multiFrame)
{
    // Update CHISTREAMPARAMS first
    NativeChiStream* pChiStream  = pConfiguredStream->pChiStream;
    size_t           bufferSize  = (*m_pCamxMemGetImageSizeStride)(static_cast<uint32_t>(pChiStream->width),
                                                                   static_cast<uint32_t>(pChiStream->height),
                                                                   static_cast<uint32_t>(pChiStream->format),
                                                                   static_cast<uint32_t>(pChiStream->grallocUsage),
                                                                   static_cast<uint32_t*>(&pChiStream->streamParams.planeStride),
                                                                   static_cast<uint32_t*>(&pChiStream->streamParams.sliceHeight));

    CF2_LOG_INFO("Stream WxH = %dx%d, format = 0x%X, gUsage = 0x%X, planeStride = %d, sliceHeight = %d, bufferSize = %zu",
                 pChiStream->width,
                 pChiStream->height,
                 pChiStream->format,
                 pChiStream->grallocUsage,
                 pChiStream->streamParams.planeStride,
                 pChiStream->streamParams.sliceHeight,
                 bufferSize);

    int result = GenerateBuffers(cameraId, pConfiguredStream, streamId, testCaseName, inputFileName, multiFrame);

    if (result != 0)
    {
        DestroyBuffers();
        return NULL;
    }

    return this;
}

/*******************************************************************************************************************************
*   GenericBufferManager::GenerateBuffers
*
*   @brief
*       Generates output buffers for given stream
*   @return
*       0 on success or -1 on failure
*******************************************************************************************************************************/
int GenericBufferManager::GenerateBuffers(
    int cameraId,                       //[in] camera Id for the inputFileName
    GenericStream* pConfiguredStream,   //[in] configured stream for a given session
    int streamId,                       //[in] stream Id for stream
    const char* testCaseName,           //[in] test case name
    const char* inputFileName,          //[in] inputFileName for input stream to be read
    bool multiFrame)                    //[in] indicator for multi frame input images
{
    if (pConfiguredStream == NULL)
    {
        CF2_LOG_ERROR("Cannot create buffers for NULL stream");
        return -1;
    }

    m_streamType = GetTypeFromStream(pConfiguredStream);
    if (m_streamType == CAMERA3_STREAM_INPUT)
    {
        m_pInputImageQueue = CF2_NEW std::deque<GenericBuffer*>();
    }
    else if(m_streamType == CAMERA3_STREAM_BIDIRECTIONAL)
    {
        m_pInputImageQueue = CF2_NEW std::deque<GenericBuffer*>();
        m_pEmptyQueue      = CF2_NEW std::queue<GenericBuffer*>();
        m_pFilledQueue     = CF2_NEW std::queue<GenericBuffer*>();
    }
    else
    {
        m_pEmptyQueue = CF2_NEW std::queue<GenericBuffer*>();
        m_pFilledQueue = CF2_NEW std::queue<GenericBuffer*>();
    }

    m_outFileName = ConstructOutputFileName(cameraId, testCaseName);

    uint32_t maxBuffers = GetMaxBuffersFromStream(pConfiguredStream);
    if (0 == maxBuffers)
    {
        CF2_LOG_ERROR("Stream %d cannot have maxBuffers of 0!", streamId);
        return -1;
    }

    char        lkgInputImageName[256] = { 0 };
    char        inputImageName[256]    = { 0 };

    std::string imageNameStr           = std::string(inputFileName);
    SIZE_T      pos                    = imageNameStr.find("_0.");
    std::string nameBefore_0           = imageNameStr.erase(pos);

    imageNameStr                       = std::string(inputFileName);
    pos                                = imageNameStr.find('.');
    std::string dotExtension           = imageNameStr.substr(pos);

    for (uint32_t bufferIndex = 0; bufferIndex < maxBuffers; bufferIndex++)
    {
        GenericBuffer* streamBuffer = CreateGenericBuffer();
        if (streamBuffer == NULL)
        {
            return -1;
        }

        if (m_streamType == CAMERA3_STREAM_INPUT)
        {
            if (TRUE == multiFrame)
            {
                CdkUtils::SNPrintF(inputImageName, sizeof(inputImageName), "%s_%d%s",
                    nameBefore_0.c_str(), bufferIndex, dotExtension.c_str());
                if (0 == bufferIndex)
                {
                    CF2_LOG_INFO("Loading input image files %s_x%s", nameBefore_0.c_str(), dotExtension.c_str());
                }
            }
            else
            {
                CdkUtils::SNPrintF(inputImageName, sizeof(inputImageName), "%s", inputFileName);
                if (0 == bufferIndex)
                {
                    CF2_LOG_INFO("Loading input image files %s", inputImageName);
                }
            }
            if (-2 == GenerateInputBuffer(streamBuffer, pConfiguredStream, inputImageName))
            {
                // Input image file not found, reuse last known good input image file
                CF2_LOG_WARN("Load %s instead", lkgInputImageName);
                if (0 != GenerateInputBuffer(streamBuffer, pConfiguredStream, lkgInputImageName))
                {
                    // Still fail, give up
                    CF2_LOG_ERROR("Fail to import input image file !");
                    return -1;
                }
            }
            else
            {
                // Save file name to last known good image file name
                ChxUtils::Memcpy(lkgInputImageName, inputImageName, sizeof(lkgInputImageName));
            }

            m_pInputImageQueue->push_back(streamBuffer);
        }
        else
        {
            if (0 != GenerateOutputBuffer(streamBuffer, pConfiguredStream))
            {
                return -1;
            }
            m_pEmptyQueue->push(streamBuffer);
        }
    }

    return 0;
}

/*******************************************************************************************************************************
*   GenericBufferManager::GetOutputBufferForRequest
*
*   @brief
*       Provides void pointer to a native buffer for a process capture request
*   @return
*       Valid void pointer on success, NULL on failure
*******************************************************************************************************************************/
void* GenericBufferManager::GetOutputBufferForRequest()
{
    m_pQueueMutex->Lock();

    GenericBuffer* pBuffer = !m_pEmptyQueue->empty() ? m_pEmptyQueue->front() : NULL;
    int retries = 0;
    int bufferCount = 0;
    int statusCode = 0;
    int timeoutMilli = 10 * 1000; // TODO get timeoutMilli from CmdLineParser (10 seconds for now)

    while (NULL == pBuffer && bufferCount < TIMEOUT_RETRIES)
    {
        while (false == m_bBufferAvailable && retries < TIMEOUT_RETRIES)
        {
            m_pQueueCond->TimedWait(m_pQueueMutex->GetNativeHandle(), timeoutMilli);
            retries++;
        }

        if (m_bBufferAvailable == false)
        {
            CF2_LOG_ERROR("There is no buffer available to serve next request");
            statusCode = -1;
            break;
        }
        m_bBufferAvailable = false;
        pBuffer = !m_pEmptyQueue->empty() ? m_pEmptyQueue->front() : NULL;
        bufferCount++;
    }

    if(pBuffer == NULL && bufferCount >= TIMEOUT_RETRIES)
    {
        CF2_LOG_ERROR("All buffers in queue are NULL");
        statusCode = -1;
    }

    if(statusCode == 0)
    {
        m_pEmptyQueue->pop();
        m_pFilledQueue->push(pBuffer);
    }

    m_pQueueMutex->Unlock();
    return (statusCode == 0 && pBuffer != NULL) ? GetNativeBufferFromGeneric(pBuffer) : NULL;
}

/*******************************************************************************************************************************
*   GenericBufferManager::GetInputBufferForRequest
*
*   @brief
*       Provides void pointer to a native buffer for a process capture request
*   @return
*       Valid pointer on success, NULL on failure
*******************************************************************************************************************************/
void* GenericBufferManager::GetInputBufferForRequest()
{
    m_pQueueMutex->Lock();
    GenericBuffer* pBuffer = NULL;
    int retries = 0;
    int statusCode = 0;
    int timeoutMilli = 10 * 1000; // TODO get timeoutMilli from CmdLineParser (10 seconds for now)

    while (m_pInputImageQueue->empty() && retries < TIMEOUT_RETRIES)
    {
        while (!m_bBufferAvailable && retries < TIMEOUT_RETRIES)
        {
            m_pQueueCond->TimedWait(m_pQueueMutex->GetNativeHandle(), timeoutMilli);
            retries++;
        }
        if (m_streamType == CAMERA3_STREAM_BIDIRECTIONAL)
        {
            m_bBufferAvailable = false;
        }
    }

    if (!m_pInputImageQueue->empty())
    {
        pBuffer = m_pInputImageQueue->front();
        retries = 0;
    }
    while (NULL == pBuffer && retries < TIMEOUT_RETRIES)
    {
        if (m_streamType == CAMERA3_STREAM_BIDIRECTIONAL)
        {
            while (!m_bBufferAvailable && retries < TIMEOUT_RETRIES)
            {
                m_pQueueCond->TimedWait(m_pQueueMutex->GetNativeHandle(), timeoutMilli);
                retries++;
            }
            m_bBufferAvailable = false;
        }
        else // m_streamType == CAMERA3_STREAM_INPUT
        {
            if (!m_pInputImageQueue->empty())
            {
                m_pInputImageQueue->pop_front();
            }
            m_pInputImageQueue->push_back(pBuffer);
        }

        pBuffer = !m_pInputImageQueue->empty() ? m_pInputImageQueue->front() : NULL;
        retries++;
    }

    if (NULL == pBuffer && retries >= TIMEOUT_RETRIES)
    {
        CF2_LOG_ERROR("All buffers in queue are NULL");
        statusCode = -1;
    }

    if (NULL != pBuffer)
    {
        if (!m_pInputImageQueue->empty())
        {
            m_pInputImageQueue->pop_front();
        }
        m_pInputImageQueue->push_back(pBuffer);
    }

    m_pQueueMutex->Unlock();
    return (statusCode == 0 && pBuffer != NULL) ? GetNativeBufferFromGeneric(pBuffer) : NULL;
}

/*******************************************************************************************************************************
*   GenericBufferManager::ProcessBufferFromResult
*
*   @brief
*       Process the image buffer obtained from result
*   @return
*       0 on success or -1 on failure
*******************************************************************************************************************************/
int GenericBufferManager::ProcessBufferFromResult(
    const int frameNumber,          //[in] frame number of result
    GenericBuffer* pResultBuffer,   //[in] result buffer to process
    bool dump)                      //[in] should image be dumped
{
    int result;

    if (NULL != m_pFilledQueue)
    {
        m_pQueueMutex->Lock();

        result = 0;

        GenericBuffer* pQueuedStreamBuffer = static_cast<GenericBuffer*>(m_pFilledQueue->front());

        if (NULL == pQueuedStreamBuffer)
        {
            CF2_LOG_ERROR("Filled Queue has a NULL buffer!");
            m_pQueueMutex->Unlock();
            return -1;
        }

        m_pFilledQueue->pop();

        if (GetHandleFromBuffer(pQueuedStreamBuffer) == GetHandleFromBuffer(pResultBuffer))
        {
            if (CAMERA3_BUFFER_STATUS_OK == GetStatusFromBuffer(pResultBuffer))
            {
                if (GetReleaseFenceFromBuffer(pResultBuffer) == -1)
                {
                    if (dump)
                    {
                        int width  = pResultBuffer->pChiBuffer->pStream->width;
                        int height = pResultBuffer->pChiBuffer->pStream->height;

                        std::ostringstream outputName;
                        outputName << m_outFileName;

                        if (frameNumber != -1)
                        {
                            // Zero base output file name squence!
                            outputName << "_W" << width << "_H" << height << "_" << frameNumber - 1;
                        }

                        result = SaveImageToFile(pResultBuffer, outputName.str());
                    }

                    if (m_streamType == CAMERA3_STREAM_BIDIRECTIONAL)
                    {
                        if (NULL != m_pInputImageQueue)
                        {
                            if (m_pInputImageQueue->size() == GetMaxBuffersFromBuffer(pResultBuffer))
                            {
                                m_pInputImageQueue->pop_back();
                            }

                            m_pInputImageQueue->push_front(pQueuedStreamBuffer);
                        }
                        else
                        {
                            CF2_LOG_ERROR("inputImage Queue is NULL!");
                            result = -1;
                        }
                    }

                    SetAcquireFenceInBuffer(pQueuedStreamBuffer, -1);
                    m_pEmptyQueue->push(pQueuedStreamBuffer);
                }
                else
                {
                    CF2_LOG_ERROR("Release fence is not set for frame: %d!", frameNumber);
                    result = -1;
                }
            }
            else
            {
                CF2_LOG_ERROR("Received buffer in error for frame: %d!", frameNumber);
                result = -1;
            }
        }
        else
        {
            CF2_LOG_ERROR("Received buffer in non-FIFO order for frame: %d!", frameNumber);
            result = -1;
        }

        m_pQueueMutex->Unlock();

        if (result == 0)
        {
            m_bBufferAvailable = true;
            m_pQueueCond->Signal();
        }
    }
    else
    {
        CF2_LOG_ERROR("Filled buffer queue is NULL!");
        result = -1;
    }

    return result;
}

/*******************************************************************************************************************************
*   GenericBufferManager::DestroyBuffers
*
*   @brief
*       Cleanup function to free all the memory allocated
*   @return
*       0 on success or -1 on failure
*******************************************************************************************************************************/
void GenericBufferManager::DestroyBuffers()
{
    if (m_pEmptyQueue != NULL)
    {
        while (!m_pEmptyQueue->empty())
        {
            GenericBuffer* currBuffer = static_cast<GenericBuffer*>(m_pEmptyQueue->front());
            if (currBuffer == NULL || GetReleaseFenceFromBuffer(currBuffer) != -1)
            {
                m_pEmptyQueue->pop();
                continue;
            }

            native_handle_t* phandle = *static_cast<native_handle_t**>(GetHandleFromBuffer(currBuffer));
            if (phandle == NULL)
            {
                m_pEmptyQueue->pop();
                DestroyGenericBuffer(currBuffer);
                continue;
            }

#ifdef _LINUX
            m_GBmap[phandle].clear();
            m_GBmap.erase(phandle);
#else // _WINDOWS
            (*m_pCamxMemRelease)(reinterpret_cast<CamxMemHandle>(phandle));
#endif // _LINUX
            DestroyGenericBuffer(currBuffer);
            m_pEmptyQueue->pop();
        }

        delete m_pEmptyQueue;
        m_pEmptyQueue = NULL;
    }

    if (m_pFilledQueue != NULL)
    {
        while (!m_pFilledQueue->empty())
        {
            GenericBuffer* currBuffer = reinterpret_cast<GenericBuffer*>(m_pFilledQueue->front());
            if (currBuffer == NULL || GetReleaseFenceFromBuffer(currBuffer) != -1)
            {
                m_pFilledQueue->pop();
                continue;
            }

            native_handle_t* phandle = *static_cast<native_handle_t**>(GetHandleFromBuffer(currBuffer));
            if (phandle == NULL)
            {
                m_pFilledQueue->pop();
                DestroyGenericBuffer(currBuffer);
                continue;
            }

#ifdef _LINUX
            m_GBmap[phandle].clear();
            m_GBmap.erase(phandle);
#else // _WINDOWS
            (*m_pCamxMemRelease)(reinterpret_cast<CamxMemHandle>(phandle));
#endif // _LINUX
            DestroyGenericBuffer(currBuffer);
            m_pFilledQueue->pop();
        }

        delete m_pFilledQueue;
        m_pFilledQueue = NULL;
    }

    if (m_pInputImageQueue != NULL)
    {
        // No new buffers created for m_pInputImageQueue for bidirectional streams, so do not deallocate
        if (m_streamType != CAMERA3_STREAM_BIDIRECTIONAL)
        {
            while (!m_pInputImageQueue->empty())
            {
                GenericBuffer* currBuffer = reinterpret_cast<GenericBuffer*>(m_pInputImageQueue->front());
                if (currBuffer == NULL || GetReleaseFenceFromBuffer(currBuffer) != -1)
                {
                    m_pInputImageQueue->pop_front();
                    continue;
                }

                native_handle_t* phandle = *static_cast<native_handle_t**>(GetHandleFromBuffer(currBuffer));
                if (phandle == NULL)
                {
                    m_pInputImageQueue->pop_front();
                    DestroyGenericBuffer(currBuffer);
                    continue;
                }

#ifdef _LINUX
                m_GBmap[phandle].clear();
                m_GBmap.erase(phandle);
#else // _WINDOWS
                (*m_pCamxMemRelease)(reinterpret_cast<CamxMemHandle>(phandle));
#endif // _LINUX
                DestroyGenericBuffer(currBuffer);
                m_pInputImageQueue->pop_front();
            }
        }

        delete m_pInputImageQueue;
        m_pInputImageQueue = NULL;
    }

    delete this;
}

/*******************************************************************************************************************************
*   GenericBufferManager::ConstructOutputFileName
*
*   @brief
*       Construct filename for an output file
*   @return
*       string
*******************************************************************************************************************************/
std::string GenericBufferManager::ConstructOutputFileName(int cameraId, const char* testCaseName)
{
    CF2_UNUSED_PARAM(cameraId);

    std::ostringstream outputName;
    outputName << outputImagePath;

   // Get the name of the input image file specified in the spectra config parser file
    std::string spectraConfigImageName = SpectraConfigParser::GetImageFileName();

    // Get the prefix to be added to the output file name
    std::string prefix = SpectraConfigParser::GetOutputFileNamePrefix();

    if ((FALSE == spectraConfigImageName.empty()) && (FALSE == prefix.empty()))
    {
        size_t pos = spectraConfigImageName.find('.');

        outputName << prefix << "_" << testCaseName << "_" << spectraConfigImageName.erase(pos);
    }

    else
    {
        outputName << testCaseName;
    }

    return outputName.str();
}

/*******************************************************************************************************************************
*   GenericBufferManager::ConstructOutputFileName
*
*   @brief
*       Construct filename for an output file
*   @return
*       string
*******************************************************************************************************************************/
std::string GenericBufferManager::GetFileExt(int format)
{
    switch (format)
    {
    case HAL_PIXEL_FORMAT_BLOB:
        return ".jpg";
    case HAL_PIXEL_FORMAT_RAW10:
    case HAL_PIXEL_FORMAT_RAW16:
    case HAL_PIXEL_FORMAT_RAW_OPAQUE:
    case PRIVATE_PIXEL_FORMAT_RAW64:
        return ".raw";
    case HAL_PIXEL_FORMAT_YCbCr_420_888:
    case HAL_PIXEL_FORMAT_YCrCb_420_SP:
    case HAL_PIXEL_FORMAT_Y8:
    case HAL_PIXEL_FORMAT_Y16:
        return ".yuv";
    case HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED:
        return ".ubwc";
    case PRIVATE_PIXEL_FORMAT_UBWCTP10:
        return ".ubwctp10";
    case PRIVATE_PIXEL_FORMAT_UBWCNV12:
        return ".ubwcnv12";
    case PRIVATE_PIXEL_FORMAT_P010:
        return ".p010";
    case PRIVATE_PIXEL_FORMAT_NV12HEIF:
        return ".heif";
    // Unverified formats
    case PRIVATE_PIXEL_FORMAT_PD10:
        return ".pd10";
    default:
        CF2_LOG_ERROR("Unknown capture format: %d!", format);
        return ".unknown";
    }
}

/*******************************************************************************************************************************
*   GenericBufferManager::GetGrallocSize
*
*   @brief
*       Calculate the gralloc size for the given format
*   @return
*       Size of the buffer
*******************************************************************************************************************************/
Size GenericBufferManager::GetGrallocSize(
    uint32_t width,
    uint32_t height,
    int format,
    int dataspace,
    void* pNativeBuffer)
{
    // Gralloc implementation maps Raw Opaque to BLOB
    // Support upto 12 bpp
    Size newResolution;

    switch (format)
    {
    case HAL_PIXEL_FORMAT_BLOB:
        if (dataspace == HAL_DATASPACE_V0_JFIF ||
            dataspace == HAL_DATASPACE_JFIF)
        {
            newResolution.width = (NULL == pNativeBuffer) ? (width * height * 4) : GetExactJpegBufferSize(pNativeBuffer);
            newResolution.height = 1;
        }
        else
        {
            newResolution.width = static_cast<uint32_t>(width * height * (3.0 / 2));
            newResolution.height = 1;
        }
        break;
    case HAL_PIXEL_FORMAT_RAW_OPAQUE:
        newResolution.width = static_cast<uint32_t>(width * height * (3.0 / 2));
        newResolution.height = 1;
        break;
    case HAL_PIXEL_FORMAT_RAW10:
        // 4 pixels packed into 5 bytes, width must be multiple of 4 pixels
        newResolution.width = static_cast<uint32_t>(ceil(width / 4.0) * 5 * height);
        newResolution.height = 1;
        break;
    case HAL_PIXEL_FORMAT_Y8:
        // 1 pixel packed into 1 byte (8 bits), width must be multiple of 2 pixels
        newResolution.width = static_cast<uint32_t>(ceil(width / 2.0) * 2 * height);
        newResolution.height = 1;
        break;
    case HAL_PIXEL_FORMAT_Y16:
        // Same as Y8, but double the bits per pixel (1 pixel packed into 2 bytes)
        newResolution.width = static_cast<uint32_t>(ceil(width / 2.0) * 4 * height);
        newResolution.height = 1;
        break;
    default:
        newResolution.width = width;
        newResolution.height = height;
        break;
    }

    return newResolution;
}

/*******************************************************************************************************************************
*   GenericBufferManager::GetExactJpegBufferSize
*
*   @brief
*       The buffer size after trimming data after end of image bytes 0xFF 0xD9
*   @return
*       uint32_t new buffer size after trimming
*******************************************************************************************************************************/
uint32_t GenericBufferManager::GetExactJpegBufferSize(
    void* pNativeBuffer)
{
    uint32_t actualSize = 0;
    bool endOfImage = false;
    unsigned char* pCurrentBuffer = static_cast<unsigned char*>(pNativeBuffer);
    unsigned char* pNextBuffer = static_cast<unsigned char*>(pNativeBuffer);
    while (!endOfImage)
    {
        ++pNextBuffer;
        if (static_cast<uint32_t>(*pCurrentBuffer) == 255) //0xFF
        {
            if (static_cast<uint32_t>(*pNextBuffer) == 217) //0xD9
            {
                endOfImage = true;
            }
        }
        ++actualSize;
        pCurrentBuffer = pNextBuffer;
    }

    return ++actualSize;    // increment by 1 to include the last byte
}
