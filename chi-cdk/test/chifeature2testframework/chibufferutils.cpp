////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chibufferutils.cpp
/// @brief Implementations for buffer utilities.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chibufferutils.h"
#include "chifeature2test.h"

/**************************************************************************************************
*   ChiFeature2BufferUtils::GenerateInputBuffer
*
*   @brief
*       Helper function to read an image file and copy it over to image buffer
*   @return
*       0 on success or -1 on failure
**************************************************************************************************/
int ChiFeature2BufferUtils::GenerateInputBuffer(
    GenericBuffer* requiredBuffer,  //[out] buffer property to be populated
    GenericStream* inputStream,     //[in] input stream to be associated with
    const char* filename)           //[in] filename to read contents
{
    CF2_LOG_ENTRY();
    if (filename == NULL)
    {
        CF2_LOG_ERROR("Filename is NULL");
        CF2_LOG_EXIT();
        return -1;
    }

    void *pData;
    CHAR completeFilePath[128] = { 0 };

    snprintf(completeFilePath,
        sizeof(completeFilePath),
        "%s%s",
        inputImagePath.c_str(),
        filename);

    CF2_LOG_INFO("Input file %s", completeFilePath);

    FILE* pFile = CdkUtils::FOpen(completeFilePath, "rb");

    if (pFile == NULL)
    {
        CF2_LOG_ERROR("Could not open input file!");
        CF2_LOG_EXIT();
        return -1;
    }

    CdkUtils::FSeek(pFile, 0L, SEEK_END);    // Seek to end of file

    long long fileSize = CdkUtils::FTell(pFile);

    CF2_LOG_DEBUG("Input file %s: size is: %lld", completeFilePath, fileSize);

    CdkUtils::FSeek(pFile, 0L, SEEK_SET);    // Seek back to beginning of file

    int res = 0;

    switch (inputStream->pChiStream->format)
    {
    case ChiStreamFormatRaw16:
    case ChiStreamFormatRaw10:
    case ChiStreamFormatBlob:
    case ChiStreamFormatYCbCr420_888:
        if (NULL == m_pBufferManager)
        {
            CreateBufferManager(inputStream);
        }

        if (NULL != m_pBufferManager)
        {
            ////IMP //CPU_WRITE + BindBuffer + GetVaddr + after copy + do cache flush
            //Gralloc1Interface*pGrallo1Interface = m_pBufferManager->GetGralloc1Interface();


            requiredBuffer->pChiBuffer->bufferInfo = m_pBufferManager->GetImageBufferInfo();
            const CHIBUFFERINFO* pBufferInfo = const_cast<CHIBUFFERINFO*>(&requiredBuffer->pChiBuffer->bufferInfo);
            m_pBufferManager->BindBuffer(pBufferInfo);
            pData = m_pBufferManager->GetCPUAddress(pBufferInfo, fileSize);
            //gralloc1_rect_t accessRegion = { 0, 0, (INT32)(inputStream->pChiStream->width), (INT32)(inputStream->pChiStream->height) };

            //if(NULL != pGrallo1Interface)
            //{
            //        pGrallo1Interface->Lock(
            //         m_pBufferManager->GetGralloc1Device(),
            //         *(static_cast<buffer_handle_t*>(requiredBuffer->pChiBuffer->bufferInfo.phBuffer)),
             //        GRALLOC1_PRODUCER_USAGE_CPU_WRITE_OFTEN,
            //         GRALLOC1_CONSUMER_USAGE_CPU_READ_OFTEN,
             //        &accessRegion,
             //        (VOID**)&pData,
             //        -1); // Acquire fence

           if (NULL != pData)
           {
               size_t read = CdkUtils::FRead(pData, /*unknown dest buffer size*/fileSize, 1, fileSize, pFile);

               res = CdkUtils::FClose(pFile);
               if (static_cast<long>(read) != fileSize)
               {
                   CF2_LOG_ERROR("Failed to read file, read: %zu bytes!", read);
                   CF2_LOG_EXIT();
                   return -1;
               }
           }
           else
           {
               CF2_LOG_ERROR("CPU Address obtained is Null");
           }

           m_pBufferManager->CacheOps(pBufferInfo, TRUE, TRUE);

               requiredBuffer->pChiBuffer->pStream = inputStream->pChiStream;

               //TODO Does it required?
               requiredBuffer->pChiBuffer->acquireFence = CreateFence(false);

               CF2_LOG_DEBUG("NATIVE FENCE FD = %d", requiredBuffer->pChiBuffer->acquireFence.nativeFenceFD);
           //}
           //else
           //{
           //    CF2_LOG_ERROR("interface is null");
           //}

        }
        else
        {
            CF2_LOG_ERROR("BufferManager is null");
        }
        break;

    default:
        CF2_LOG_ERROR("Not supported");
        break;
    }
    CF2_LOG_EXIT();
    return 0;
}


/**************************************************************************************************************************
*   ChiFeature2BufferUtils::CreateFence
*
*   @brief
*       Creates a fence handle using the driver and assembles a CHIFENCEINFO object out of it
*   @params
*       [in]    bool    isValid     if set false, the returned fence will not have any memory allocated for its handle
*   @return
*       CHIFENCEINFO object, which may have its valid flag true or false
**************************************************************************************************************************/
CHIFENCEINFO ChiFeature2BufferUtils::CreateFence(bool isValid)
{
    CF2_LOG_ENTRY();
    CHIFENCEINFO fenceInfo;
    fenceInfo.valid = false;
    fenceInfo.type = ChiFenceTypeInternal;

    if (isValid)
    {
        CHIFENCEHANDLE* pFenceHandle = CF2_NEW CHIFENCEHANDLE;
        CDKResult res = ExtensionModule::GetInstance()->CreateChiFence(NULL, pFenceHandle);
        //m_pChiModule->GetFenceOps()->pCreateFence(m_pChiModule->GetContext(), NULL/*TODO*/, pFenceHandle);

        if (res == CDKResultSuccess)
        {
            fenceInfo.valid = true;
            fenceInfo.type = ChiFenceTypeInternal;
            fenceInfo.hChiFence = *pFenceHandle;
        }
    }
    CF2_LOG_EXIT();
    return fenceInfo;
}


/**************************************************************************************************************************
*   ChiFeature2BufferUtils::CreateGenericBuffer
*
*   @brief
*       Helper function to initialize a generic buffer with a chi buffer
*   @return
*       GenericBuffer pointer on success, NULL otherwise
**************************************************************************************************************************/
GenericBuffer* ChiFeature2BufferUtils::CreateGenericBuffer()
{
    CF2_LOG_ENTRY();
    GenericBuffer* newGenericBuffer = CF2_NEW GenericBuffer();
    if (newGenericBuffer != NULL)
    {
        NativeChiBuffer* newChiBuffer = CF2_NEW NativeChiBuffer();
        if (newChiBuffer != NULL)
        {
            newChiBuffer->size = sizeof(NativeChiBuffer);
            newGenericBuffer->pChiBuffer = newChiBuffer;
            CF2_LOG_EXIT();
            return newGenericBuffer;
        }
    }
    CF2_LOG_EXIT();
    return NULL;
}

/**************************************************************************************************************************
*   ChiFeature2BufferUtils::DestroyFence
*
*   @brief
*       Destroys a fence by releasing it on driver side and deallocating its fence handle. No action if fence not valid.
*   @return
*       None
**************************************************************************************************************************/
void ChiFeature2BufferUtils::DestroyFence(CHIFENCEINFO fenceInfo)
{
    CF2_LOG_ENTRY();
    if (fenceInfo.valid == true)
    {
        fenceInfo.valid = false;
        CHIFENCEHANDLE* pFenceHandle = &(fenceInfo.hChiFence);

        ExtensionModule::GetInstance()->ReleaseChiFence(fenceInfo.hChiFence);
        if (pFenceHandle != NULL)
        {
            delete pFenceHandle;
        }
    }
    // If not valid, then no action needed
    CF2_LOG_EXIT();
}

/**************************************************************************************************************************
*   ChiFeature2BufferUtils::DestroyGenericBuffer
*
*   @brief
*       Helper function to destroy a generic buffer along with its associated chi buffer
*   @return
*       int 0 on success, otherwise -1
**************************************************************************************************************************/
void ChiFeature2BufferUtils::DestroyGenericBuffer(GenericBuffer* buffer)
{
    CF2_LOG_ENTRY();
    if (buffer != NULL)
    {
        if (buffer->pChiBuffer != NULL)
        {
            DestroyFence(buffer->pChiBuffer->acquireFence);
            DestroyFence(buffer->pChiBuffer->releaseFence);

            CHX_DELETE buffer->pChiBuffer;
        }
        CHX_DELETE buffer;
    }
    CF2_LOG_EXIT();
}


void ChiFeature2BufferUtils::CreateBufferManager(GenericStream* inputStream)
{
    CF2_LOG_ENTRY();
    if (NULL == m_pBufferManager)
    {
        CHIBufferManagerCreateData createInputBuffers = { 0 };

        createInputBuffers.width = inputStream->pChiStream->width;
        createInputBuffers.height = inputStream->pChiStream->height;
        createInputBuffers.format = inputStream->pChiStream->format;
        createInputBuffers.producerFlags = GRALLOC1_PRODUCER_USAGE_CAMERA | GRALLOC1_PRODUCER_USAGE_CPU_READ |
            GRALLOC1_PRODUCER_USAGE_CPU_WRITE;
        createInputBuffers.consumerFlags = GRALLOC1_CONSUMER_USAGE_CAMERA | GRALLOC1_CONSUMER_USAGE_CPU_READ;
        createInputBuffers.immediateBufferCount = 0;
        createInputBuffers.maxBufferCount = 2;
        createInputBuffers.bEnableLateBinding = FALSE;
        createInputBuffers.bufferHeap = BufferHeapDefault;
        createInputBuffers.pChiStream = inputStream->pChiStream;

        m_pBufferManager = CHIBufferManager::Create("RAWBufferManager", &createInputBuffers);
    }
    CF2_LOG_EXIT();
}

void ChiFeature2BufferUtils::DestroyBufferManager()
{
    CF2_LOG_ENTRY();
    if (NULL != m_pBufferManager)
    {
        m_pBufferManager->Destroy();
    }
    CF2_LOG_EXIT();
}

void ChiFeature2BufferUtils::Destroy()
{
    CF2_LOG_ENTRY();
    DestroyBufferManager();
    CF2_LOG_EXIT();
}

//TODO Destroy buffer manager
//TODO Free buffer data
