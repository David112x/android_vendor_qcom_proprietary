////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  feature2buffermanager.cpp
/// @brief Implementation for feature2 buffer manager.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "feature2buffermanager.h"

/*******************************************************************************************************************************
*   Feature2BufferManager::GenerateOutputBuffer
*
*   @brief
*       Overloaded helper function to generate a Chi native buffer and NativeChiBuffer for a given output stream
*   @return
*       0 on success or -1 on failure
*******************************************************************************************************************************/
int Feature2BufferManager::GenerateOutputBuffer(
    GenericBuffer* pRequiredBuffer, //[out] buffer to be populated
    GenericStream* pOutputStream)   //[in] output stream to be associated with
{
    uint32_t width = pOutputStream->pChiStream->width;
    uint32_t height = pOutputStream->pChiStream->height;
    int outputFormat = pOutputStream->pChiStream->format;
    int dataspace = pOutputStream->pChiStream->dataspace;

    pRequiredBuffer->pChiBuffer->pStream = pOutputStream->pChiStream;

#ifdef _LINUX
    // Generate native buffer
    uint32_t usage = pOutputStream->pChiStream->grallocUsage;

    int      planeStride = pOutputStream->pChiStream->streamParams.planeStride;
    int      sliceHeight = pOutputStream->pChiStream->streamParams.sliceHeight;

    Size grallocResolution(0, 0);
    if ((0 == planeStride) || (0 == sliceHeight))
    {
        grallocResolution = GetGrallocSize(width, height, outputFormat, dataspace);
    }
    else
    {
        grallocResolution.width  = planeStride;
        grallocResolution.height = sliceHeight;
    }

    if (ChiStreamFormatPD10 == outputFormat)
    {
        CF2_LOG_WARN("PD10 format = 0x%X, force to 0x%X to work around Gralloc issue!",
            ChiStreamFormatPD10, ChiStreamFormatRaw12);
        outputFormat = ChiStreamFormatRaw12;
    }
    android::sp<android::GraphicBuffer> nativeBuffer = CF2_NEW android::GraphicBuffer(
        grallocResolution.width,
        grallocResolution.height,
        outputFormat,
        usage);

    ANativeWindowBuffer *anbOutput = nativeBuffer->getNativeBuffer();
    if (anbOutput == NULL)
    {
        CF2_LOG_ERROR("Native window buffer for output stream is NULL!");
        return -1;
    }
    pRequiredBuffer->pChiBuffer->bufferInfo.phBuffer = &(anbOutput->handle);
    pRequiredBuffer->pChiBuffer->bufferInfo.bufferType = CHIBUFFERTYPE::ChiGralloc;
    native_handle_t* pHandle = *static_cast<native_handle_t**>(pRequiredBuffer->pChiBuffer->bufferInfo.phBuffer);
    m_GBmap[pHandle] = nativeBuffer;

#else // _WINDOWS
    CamxMemHandle bufferHandle;
    (*m_pCamxMemAlloc)(&bufferHandle, width, height, outputFormat, 0);
    pRequiredBuffer->pChiBuffer->bufferInfo.phBuffer = reinterpret_cast<NativeChiBufferHandle*>(bufferHandle);
    pRequiredBuffer->pChiBuffer->bufferInfo.bufferType = CHIBUFFERTYPE::ChiNative;

#endif // _LINUX

    pRequiredBuffer->pChiBuffer->acquireFence = CreateFence(false);
    return 0;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::GenerateInputBuffer
*
*   @brief
*       Helper function to read an image file and copy it over to image buffer
*   @return
*       0 on success or -1 on failure
*******************************************************************************************************************************/
int Feature2BufferManager::GenerateInputBuffer(
    GenericBuffer* pRequiredBuffer, //[out] buffer property to be populated
    GenericStream* pInputStream,    //[in] input stream to be associated with
    const char* filename)           //[in] filename to read contents
{
    if (filename == NULL)
    {
        CF2_LOG_ERROR("Filename is NULL!");
        return -1;
    }

    void *pData;
    std::ostringstream completeFilePath;
    if (NULL == strchr(filename, '/'))
    {
        completeFilePath << inputImagePath << filename;
    }
    else
    {
        completeFilePath << filename;
    }

    FILE* pFile = CdkUtils::FOpen(completeFilePath.str().c_str(), "rb");
    if (pFile == NULL)
    {
        CF2_LOG_WARN("%s open failed!", filename);
        return -2;
    }

    CdkUtils::FSeek(pFile, 0L, SEEK_END);    // Seek to end of file
    long long fileSize = CdkUtils::FTell(pFile);
    CF2_LOG_DEBUG("Input file %s: size is: %lld", completeFilePath.str().c_str(), fileSize);
    CdkUtils::FSeek(pFile, 0L, SEEK_SET);    // Seek back to beginning of file

    int res = 0;

#ifdef _LINUX
    uint32_t width       = static_cast<uint32_t>(pInputStream->pChiStream->width);
    uint32_t height      = static_cast<uint32_t>(pInputStream->pChiStream->height);
    uint32_t format      = static_cast<uint32_t>(pInputStream->pChiStream->format);
    int      dataspace   = pInputStream->pChiStream->dataspace;
    uint32_t usage       = static_cast<uint32_t>(pInputStream->pChiStream->grallocUsage);
    uint32_t planeStride = static_cast<uint32_t>(pInputStream->pChiStream->streamParams.planeStride);
    uint32_t sliceHeight = static_cast<uint32_t>(pInputStream->pChiStream->streamParams.sliceHeight);

    Size grallocResolution(0, 0);
    if ((ChiStreamFormatRaw10     == pInputStream->pChiStream->format) ||
        (ChiStreamFormatRaw16     == pInputStream->pChiStream->format) ||
        (ChiStreamFormatRawOpaque == pInputStream->pChiStream->format))
    {
        // can ignore stride and scanline since imageFormatUtils will handle them
        grallocResolution.width  =
            static_cast<uint32_t>((*m_pCamxMemGetImageSize)(width, height, format, usage));
        grallocResolution.height = 1;
    }
    else if ((0 == planeStride) || (0 == sliceHeight))
    {
        grallocResolution = GetGrallocSize(width, height, format, dataspace);
    }
    else
    {
        grallocResolution.width  = planeStride;
        grallocResolution.height = sliceHeight;
    }

    android::sp<android::GraphicBuffer> nativeBuffer = CF2_NEW android::GraphicBuffer(
        grallocResolution.width,
        grallocResolution.height,
        format,
        usage);

    ANativeWindowBuffer *anbOutput = nativeBuffer->getNativeBuffer();
    if (anbOutput == NULL)
    {
        CF2_LOG_ERROR("Native window buffer for input stream is NULL!");
        return -1;
    }

    res = nativeBuffer->lock(android::GraphicBuffer::USAGE_SW_WRITE_OFTEN, reinterpret_cast<void**>(&pData));

    if (res != 0)
    {
        CF2_LOG_ERROR("Failed to lock the nativebuffer for reading!");
        return -1;
    }

    pRequiredBuffer->pChiBuffer->bufferInfo.phBuffer = &(anbOutput->handle);
    pRequiredBuffer->pChiBuffer->bufferInfo.bufferType = CHIBUFFERTYPE::ChiGralloc;
    native_handle_t* pHandle = *static_cast<native_handle_t**>(pRequiredBuffer->pChiBuffer->bufferInfo.phBuffer);
    m_GBmap[pHandle] = nativeBuffer;

#else // _WINDOWS
    CamxMemHandle bufferHandle;
    (*m_pCamxMemAlloc)(&bufferHandle, pInputStream->pChiStream->width, pInputStream->pChiStream->height,
        pInputStream->pChiStream->format, 0);
    pRequiredBuffer->pChiBuffer->bufferInfo.phBuffer = reinterpret_cast<buffer_handle_t*>(bufferHandle);
    pRequiredBuffer->pChiBuffer->bufferInfo.bufferType = CHIBUFFERTYPE::ChiNative;

    native_handle_t* pHandle = *reinterpret_cast<native_handle_t**>(pRequiredBuffer->pChiBuffer->bufferInfo.phBuffer);
    pData = *reinterpret_cast<void**>(pHandle->data);

#endif // _LINUX
    size_t bytesRead = 0;
    if (NULL != pData)
    {
        bytesRead = CdkUtils::FRead(pData, /*unknown dest buffer size*/fileSize, 1, fileSize, pFile);
    }
    res = CdkUtils::FClose(pFile);
    if (static_cast<long>(bytesRead) != fileSize)
    {
        CF2_LOG_ERROR("Failed to read file, read: %zu bytes!", bytesRead);
        // return -1;  // TODO fix issue with raw10 input image (needs more space than expected)
    }

#ifdef _LINUX
    if (NULL != pData)
    {
        res = nativeBuffer->unlock();
    }
#endif // _LINUX

    pRequiredBuffer->pChiBuffer->pStream = pInputStream->pChiStream;
    pRequiredBuffer->pChiBuffer->acquireFence = CreateFence(false);
    return 0;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::SaveImageToFile
*
*   @brief
*       Helper function to save an image to the file
*   @return
*       0 on success or -1 on failure
*******************************************************************************************************************************/
int Feature2BufferManager::SaveImageToFile(
    GenericBuffer* pResultBuffer, //[in] result to save image from
    std::string filename)               //[in] filename to save image as
{
    int res;
    void* pData = NULL;
    native_handle_t* phBuffer = *static_cast<native_handle_t**>(pResultBuffer->pChiBuffer->bufferInfo.phBuffer);
    NativeChiStream* resultStream = pResultBuffer->pChiBuffer->pStream;

    uint32_t  output_width  = resultStream->width;
    uint32_t  output_height = resultStream->height;
    int       planeStride   = resultStream->streamParams.planeStride;
    int       sliceHeight   = resultStream->streamParams.sliceHeight;
    uint32_t  format        = resultStream->format;
    uint32_t  usage         = resultStream->grallocUsage;

#ifdef _LINUX

    if (m_GBmap.find(phBuffer) == m_GBmap.end())
    {
        CF2_LOG_ERROR("Cannot find nativehandle for outputBuffer!");
        return -1;
    }
    res = m_GBmap.at(phBuffer)->lock(android::GraphicBuffer::USAGE_SW_READ_OFTEN, reinterpret_cast<void**>(&pData));
    if (res != 0)
    {
        CF2_LOG_ERROR("outputNativeBuffer lock failed!");
        return -1;
    }

    // Ensure capture folder gets created or exists
    // Make upper camera folder first
    if (mkdir(inputImagePath.c_str(), 0777) != 0 && EEXIST != errno)
    {
        CF2_LOG_ERROR("Failed to create capture camera folder, Error: %d", errno);
        return -1;
    }

    // Make lower nativetest folder second
    if (mkdir(outputImagePath.c_str(), 0777) != 0 && EEXIST != errno)
    {
        CF2_LOG_ERROR("Failed to create capture root folder, Error: %d", errno);
        return -1;
    }
#else // _WINDOWS

    pData = *reinterpret_cast<void**>(phBuffer->data);

    if (pData == NULL)
    {
        CF2_LOG_ERROR("Buffer data is NULL!");
        return -1;
    }

    // Make sure capture folder gets created or exists
    if (_mkdir(outputImagePath.c_str()) != 0)
    {
        DWORD error = GetLastError();
        if (ERROR_ALREADY_EXISTS != error)
        {
            CF2_LOG_ERROR("Failed to create capture folder, Error: %d", error);
            return -1;
        }
    }

#endif // _LINUX

    size_t imageSize = 0;
    // Image formats we know how to calculate size for...
    if (format == HAL_PIXEL_FORMAT_BLOB ||
        format == HAL_PIXEL_FORMAT_RAW_OPAQUE ||
        format == HAL_PIXEL_FORMAT_RAW10 ||
        format == HAL_PIXEL_FORMAT_Y8 ||
        format == HAL_PIXEL_FORMAT_Y16)
    {
        imageSize = GetGrallocSize(output_width, output_height, format, resultStream->dataspace, pData).width;
    }
    else // Unknown image format size calculations: PD10, RAW64, NV12HEIF, P010, YCbCr_420_888
    {
        imageSize =
            (*m_pCamxMemGetImageSize)(output_width, output_height, format, usage);
    }

    if (0 == imageSize)
    {
        CF2_LOG_ERROR("Calculated gralloc image size should not be 0!");
        return -1;
    }
    CF2_LOG_INFO("Image size to be saved is: %zu", imageSize);
    filename.append(GetFileExt(format));
    CF2_LOG_INFO("Saving image to file: %s", filename.c_str());

    char out_fname[256];
    CdkUtils::SNPrintF(out_fname, sizeof(out_fname), "%s", filename.c_str());

    FILE* pFile = CdkUtils::FOpen(out_fname, "wb");
    if (pFile == NULL)
    {
        CF2_LOG_ERROR("Output file creation failed!");
        return -1;
    }

    size_t written = 0;
    written = CdkUtils::FWrite(reinterpret_cast<unsigned char *>(pData), 1, imageSize, pFile);
    if (written != imageSize)
    {
        CF2_LOG_ERROR("Failed to write image: %s!", out_fname);
        return -1;
    }

#ifdef _LINUX
    res = m_GBmap.at(phBuffer)->unlock();
    if (res != 0)
    {
        CF2_LOG_ERROR("outputNativeBuffer unlock failed!");
        return -1;
    }
#endif // _LINUX

    res = CdkUtils::FClose(pFile);
    if (res != 0)
    {
        CF2_LOG_ERROR("Close file failed!");
        return -1;
    }
    return 0;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::CreateGenericBuffer
*
*   @brief
*       Helper function to initialize a generic buffer with a chi buffer
*   @return
*       GenericBuffer pointer on success, NULL otherwise
*******************************************************************************************************************************/
GenericBuffer* Feature2BufferManager::CreateGenericBuffer()
{
    GenericBuffer* newGenericBuffer = CF2_NEW GenericBuffer();
    if (NULL != newGenericBuffer)
    {
        NativeChiBuffer* newChiBuffer = CF2_NEW NativeChiBuffer();
        if (NULL != newChiBuffer)
        {
            newChiBuffer->size = sizeof(NativeChiBuffer);
            newGenericBuffer->pChiBuffer = newChiBuffer;
            return newGenericBuffer;
        }
    }
    return NULL;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::DestroyGenericBuffer
*
*   @brief
*       Helper function to destroy a generic buffer along with its associated chi buffer
*   @return
*       int 0 on success, otherwise -1
*******************************************************************************************************************************/
void Feature2BufferManager::DestroyGenericBuffer(GenericBuffer* pBuffer)
{
    if (NULL != pBuffer)
    {
        if (NULL != pBuffer->pChiBuffer)
        {
            DestroyFence(pBuffer->pChiBuffer->acquireFence);
            DestroyFence(pBuffer->pChiBuffer->releaseFence);

            delete pBuffer->pChiBuffer;
        }
        delete pBuffer;
    }
}

/*******************************************************************************************************************************
*   Feature2BufferManager::GetTypeFromStream
*
*   @brief
*       Get stream type from generic stream
*   @return
*       int stream type
*******************************************************************************************************************************/
int Feature2BufferManager::GetTypeFromStream(GenericStream* pStream)
{
    return pStream->pChiStream->streamType;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::GetMaxBuffersFromStream
*
*   @brief
*       Get max number of buffers from generic stream
*   @return
*       uint32_t max number of buffers
*******************************************************************************************************************************/
uint32_t Feature2BufferManager::GetMaxBuffersFromStream(GenericStream* pStream)
{
    return pStream->pChiStream->maxNumBuffers;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::GetNativeBufferFromGeneric
*
*   @brief
*       Get void pointer to a NativeChiBuffer from a GenericBuffer
*   @return
*       void pointer to the associated NativeChiBuffer
*******************************************************************************************************************************/
void* Feature2BufferManager::GetNativeBufferFromGeneric(GenericBuffer* pBuffer)
{
    return static_cast<void*>(pBuffer->pChiBuffer);
}

/*******************************************************************************************************************************
*   Feature2BufferManager::GetHandleFromBuffer
*
*   @brief
*       Get buffer handle from generic buffer
*   @return
*       NativeChiBufferHandle
*******************************************************************************************************************************/
NativeChiBufferHandle Feature2BufferManager::GetHandleFromBuffer(GenericBuffer* pBuffer)
{
    return pBuffer->pChiBuffer->bufferInfo.phBuffer;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::GetStatusFromBuffer
*
*   @brief
*       Get buffer status from generic buffer
*   @return
*       int buffer status
*******************************************************************************************************************************/
int Feature2BufferManager::GetStatusFromBuffer(GenericBuffer* pBuffer)
{
    return pBuffer->pChiBuffer->bufferStatus;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::GetReleaseFenceFromBuffer
*
*   @brief
*       Get release fence from generic buffer
*   @return
*       int release fence status, where -1 means no waiting
*******************************************************************************************************************************/
int Feature2BufferManager::GetReleaseFenceFromBuffer(GenericBuffer* pBuffer)
{
    if (pBuffer->pChiBuffer->releaseFence.valid == true)
    {
        CDKResult* pFenceResult = NULL;
        CDKResult res = m_pChiModule->GetFenceOps()->pGetFenceStatus(m_pChiModule->GetContext(),
            pBuffer->pChiBuffer->releaseFence.hChiFence, pFenceResult);
        if (CDKResultSuccess == res && NULL != pFenceResult)
        {
            if (CDKResultSuccess != *pFenceResult)
            {
                return 0;   // TODO fence is busy?
            }
            else
            {
                return -1;  // Fence not busy
            }
        }
        CF2_LOG_ERROR("Failure when getting release fence status!");
        return -1;  // TODO what to return for failure?
    }
    // If fence not valid, then assume no waiting
    return -1;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::SetAcquireFenceInBuffer
*
*   @brief
*       Set the acquire fence in generic buffer
*   @return
*       void
*******************************************************************************************************************************/
void Feature2BufferManager::SetAcquireFenceInBuffer(GenericBuffer* pBuffer, int setVal)
{
    if (-1 == setVal)
    {
        DestroyFence(pBuffer->pChiBuffer->acquireFence);
    }
    else if (pBuffer->pChiBuffer->acquireFence.valid == true)
    {
        // TODO hold the fence?
        PFNCHIFENCECALLBACK dummyCallback = NULL;
        VOID* pData = NULL;
        m_pChiModule->GetFenceOps()->pWaitFenceAsync(m_pChiModule->GetContext(), dummyCallback,
            pBuffer->pChiBuffer->acquireFence.hChiFence, pData);
    }
    // If not valid, do nothing
}

/*******************************************************************************************************************************
*   Feature2BufferManager::GetMaxBuffersFromBuffer
*
*   @brief
*       Get the max number of buffers from generic buffer's associated stream
*   @return
*       uint32_t max number of buffers
*******************************************************************************************************************************/
uint32_t Feature2BufferManager::GetMaxBuffersFromBuffer(GenericBuffer* pBuffer)
{
    return pBuffer->pChiBuffer->pStream->maxNumBuffers;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::CreateFence
*
*   @brief
*       Creates a fence handle using the driver and assembles a CHIFENCEINFO object out of it
*   @params
*       [in]    bool    isValid     if set false, the returned fence will not have any memory allocated for its handle
*   @return
*       CHIFENCEINFO object, which may have its valid flag true or false
*******************************************************************************************************************************/
CHIFENCEINFO Feature2BufferManager::CreateFence(bool isValid)
{
    CHIFENCEINFO fenceInfo = { 0 };
    fenceInfo.valid = false;

    if (isValid)
    {
        CHIFENCEHANDLE* pFenceHandle = CF2_NEW CHIFENCEHANDLE;
        CDKResult res = m_pChiModule->GetFenceOps()->pCreateFence(m_pChiModule->GetContext(), NULL/*TODO*/, pFenceHandle);

        if (res == CDKResultSuccess)
        {
            fenceInfo.valid = true;
            fenceInfo.type = ChiFenceTypeInternal;
            fenceInfo.hChiFence = *pFenceHandle;
        }
    }

    return fenceInfo;
}

/*******************************************************************************************************************************
*   Feature2BufferManager::DestroyFence
*
*   @brief
*       Destroys a fence by releasing it on driver side and deallocating its fence handle. No action if fence not valid.
*   @return
*       None
*******************************************************************************************************************************/
void Feature2BufferManager::DestroyFence(CHIFENCEINFO fenceInfo)
{
    if (fenceInfo.valid == true)
    {
        fenceInfo.valid = false;
        CHIFENCEHANDLE* pFenceHandle = &(fenceInfo.hChiFence);

        m_pChiModule->GetFenceOps()->pReleaseFence(m_pChiModule->GetContext(), fenceInfo.hChiFence);
        if (pFenceHandle != NULL)
        {
            delete pFenceHandle;
        }
    }
    // If not valid, then no action needed
}
