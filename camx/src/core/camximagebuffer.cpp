////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camximagebuffer.cpp
///
/// @brief Image Buffer implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxatomic.h"
#include "camxutils.h"
#include "camxcsl.h"
#include "camxhwenvironment.h"
#include "camxsettingsmanager.h"
#include "camximagebuffer.h"
#include "camximageformatutils.h"
#include "camxchi.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::ImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageBuffer::ImageBuffer()
{
    // By default it is 1 i.e. HFR disabled. If HFR is enabled it will be set to some value greater than 1 when the memory is
    // allocated during Allocate()
    m_numBatchedFrames = 1;

    // Set fd value to -1 as 0 is a valid fd.
    m_bufferInfo.fd = -1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::~ImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ImageBuffer::~ImageBuffer()
{
    if (TRUE == m_isBacked)
    {
        Release(TRUE);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::Initialize(
    const CHAR*                     pBufferMgrName,
    const BufferManagerCreateData*  pCreateData,
    const ImageFormat*              pCurrentFormat,
    ImageBufferManager*             pImageBufferManager,
    MemPoolBufMgrHandle             hMemPoolBufMgrHandle)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL == pBufferMgrName)        ||
        (NULL == pCreateData)           ||
        (NULL == pCurrentFormat)        ||
        (NULL == pImageBufferManager)   ||
        (0    == pCreateData->numBatchedFrames))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "Invalid input args pImageBufferManager=%p, pBufferMgrName=%d, pCreateData=%p",
                       pImageBufferManager, pBufferMgrName, pCreateData);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        m_bIsInBusyList         = FALSE;
        m_pBufferManagerName    = pBufferMgrName;
        m_pCreateData           = pCreateData;
        m_pImageBufferManager   = pImageBufferManager;
        m_hMemPoolBufMgrHandle  = hMemPoolBufMgrHandle;

        result                  = UpdateImageFormat(pCurrentFormat);

        // in two cases the image buffer is in batched mode
        // 1. the batch frames for IFE output which has pCreateData->allocateBufferMemory set to TRUE
        // 2. the batch frames for imported HAL buffer which has IsBatchImageFormat() set to TRUE
        m_numBatchedFrames      = ((0 < pCreateData->numBatchedFrames) &&
                                   (TRUE == pCreateData->allocateBufferMemory || TRUE == IsBatchImageFormat())) ?
                                  pCreateData->numBatchedFrames : 1;

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Failed in updating format, result=%d", pBufferMgrName, result);

            m_pBufferManagerName    = NULL;
            m_pCreateData           = NULL;
            m_pImageBufferManager   = NULL;
            m_hMemPoolBufMgrHandle  = NULL;
            m_numBatchedFrames      = 1;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::UpdateImageFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::UpdateImageFormat(
    const ImageFormat*  pCurrentFormat)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCurrentFormat);
    if (NULL == pCurrentFormat)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Invalid input format pointer", GetBufferManagerName(), this);
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) && (TRUE == m_isBacked))
    {
        // Even if we have allocated buffer by now (since we allow client to update Format params),
        // we can update format parameters. But we could do that only if the allocated size is more than or equal to what is
        // required with the new format settings. ImportCSLBuffer checks it.

        // If we are getting an updated pCurrentFormat, upate import csl buf
        result = ImportCSLBuffer(pCurrentFormat, &m_bufferInfo);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in import CSL buffer", GetBufferManagerName(), this);
        }
    }

    if (CamxResultSuccess == result)
    {
        m_currentFormat = *pCurrentFormat;
        m_bValidFormat  = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::Allocate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::Allocate()
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(FALSE == m_isBacked);
    if (TRUE == m_isBacked)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : ImageBuffer[%p] already have backing buffer", GetBufferManagerName(), this);
        result = CamxResultEInvalidState;
    }

    CAMX_ASSERT(NULL != m_pCreateData);
    if (NULL == m_pCreateData)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] : invalid createdata in ImageBuffer[%p]", GetBufferManagerName(), this);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        const ImageFormat* pAllocFormat         = &m_pCreateData->bufferProperties.imageFormat;
        const SIZE_T       totalBufferSizeBytes = ImageFormatUtils::GetTotalSize(pAllocFormat) * m_numBatchedFrames;

        // Update current format (m_currentFormat) only if it is not initialized/updated.
        // Though we allocate with alloc format (m_pCreateData->bufferProperties.imageFormat), we want to populate
        // buffer handles information based on current format.
        if (FALSE == m_bValidFormat)
        {
            m_currentFormat = *pAllocFormat;
            m_bValidFormat  = TRUE;
        }

        if (NULL != m_hMemPoolBufMgrHandle)
        {
            CSLBufferInfo bufferInfo = { };

            CAMX_ASSERT(NULL == m_hMemPoolBufferHandle);

            m_hMemPoolBufferHandle = MemPoolMgr::GetBufferFromPool(m_hMemPoolBufMgrHandle, &bufferInfo, &m_hGrallocBuffer);

            if (NULL != m_hMemPoolBufferHandle)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                 "[%s][%p] Image Buffer GetBufferFromPool returned bufferInfo.fd=%d handle = %d",
                                 GetBufferManagerName(), this, bufferInfo.fd, bufferInfo.hHandle);

                // if ((BufferManagerType::CamxBufferManager == m_pCreateData->bufferManagerType) ||
                //     (CSLBufferHeapIon                     == m_pCreateData->bufferProperties.bufferHeap))

                if (0 != bufferInfo.hHandle)
                {
                    result = ImportCSLBuffer(&m_currentFormat, &bufferInfo);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] failed in import", GetBufferManagerName(), this);
                        MemPoolMgr::ReleaseBufferToPool(m_hMemPoolBufMgrHandle, m_hMemPoolBufferHandle, FALSE);
                    }
                }
                else
                {
                    // We got a buffer to back this ImageBuffer, but it was not mapped to CSL though
                    // lets set m_isBacked to TRUE. Keep an eye if this has any side effects.
                    m_isBacked = TRUE;
                }

                if ((CamxResultSuccess == result) && (NULL != m_hGrallocBuffer))
                {
                    BufferHandle*       phBufferHandle = reinterpret_cast<BufferHandle*>(&m_hGrallocBuffer);
                    const NativeHandle* phNativeBuffer = *phBufferHandle;

                    m_phNativeBuffer = phNativeBuffer;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in getting buffer from Memory Pool",
                               GetBufferManagerName(), this);
                result = CamxResultEFailed;
            }
        }
        else
        {
            const StaticSettings*   pSettings               = HwEnvironment::GetInstance()->GetStaticSettings();
            UINT32                  flags                   = m_pCreateData->bufferProperties.memFlags;

            if ((TRUE == pSettings->autoImageDump)    ||
                (TRUE == pSettings->dynamicImageDump) ||
                (TRUE == pSettings->watermarkImage))
            {
                flags |= CSLMemFlagUMDAccess;
            }

            if (CSLBufferHeapEGL == m_pCreateData->bufferProperties.bufferHeap)
            {
                UINT32      grallocStride;
                Gralloc*    pGralloc = Gralloc::GetInstance();

                CAMX_ASSERT(1 == m_numBatchedFrames);

                if (NULL != pGralloc)
                {
                    UINT32 width;
                    UINT32 height;

                    if (TRUE == ImageFormatUtils::IsYUV(pAllocFormat))
                    {
                        width  = pAllocFormat->formatParams.yuvFormat[0].planeStride;
                        height = pAllocFormat->formatParams.yuvFormat[0].sliceHeight;
                    }
                    else
                    {
                        width  = pAllocFormat->width;
                        height = pAllocFormat->height;
                    }

                    result = pGralloc->AllocateGrallocBuffer(width,
                                                             height,
                                                             m_pCreateData->bufferProperties.grallocFormat,
                                                             &m_hGrallocBuffer,
                                                             &grallocStride,
                                                             m_pCreateData->bufferProperties.producerFlags,
                                                             m_pCreateData->bufferProperties.consumerFlags);

                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "Allocating Stride, sliceHeight [%d x %d][%d x %d], stride %d",
                                     width,
                                     height,
                                     pAllocFormat->width,
                                     pAllocFormat->height,
                                     grallocStride);

                    // Pipelines can create internal buffer managers that allocate gralloc buffers  which will be
                    // imported at the time ImageBuffer objects are being created.
                    // CHI usecase may also create camx buffer managers with gralloc buffers through bufferManagerOps,
                    // those gralloc buffer handles won't be imported at this point. Instead, import will be taken
                    // care of in SetupRequestInputPorts/SetupRequestOutputPorts.
                    if ((CamxResultSuccess                    == result) &&
                        (BufferManagerType::CamxBufferManager == m_pImageBufferManager->GetBufferManagerType()))
                    {
                        BufferHandle* phBufferHandle = reinterpret_cast<BufferHandle*>(&m_hGrallocBuffer);

                        result = ImportGrallocBuffer(&m_currentFormat,
                                                     *phBufferHandle,
                                                     0,
                                                     totalBufferSizeBytes,
                                                     flags,
                                                     m_pCreateData->deviceIndices,
                                                     m_pCreateData->deviceCount);
                        if (CamxResultSuccess != result)
                        {
                            if (CamxResultSuccess == pGralloc->Destroy(m_hGrallocBuffer))
                            {
                                m_hGrallocBuffer = NULL;
                            }
                            else
                            {
                                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Release called on an invalid handle",
                                               GetBufferManagerName(), this);
                            }
                        }
                    }
                    else if (CamxResultSuccess == result)
                    {
                        // We allocated a buffer to back this ImageBuffer, but it was not mapped to CSL though
                        // lets set m_isBacked to TRUE. Keep an eye if this has any side effects.
                        m_isBacked = TRUE;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                                       "[%s][%p] Failed in allocating gralloc buffer, width=%d, height=%d, format=0x%x",
                                       GetBufferManagerName(), this, width, height,
                                       m_pCreateData->bufferProperties.grallocFormat);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Unable to get gralloc handle", GetBufferManagerName(), this);
                    result = CamxResultEInvalidPointer;
                }
            }
            else
            {
                CSLBufferInfo bufferInfo;

                // Allocate all memory in a single allocation
                /// @todo (CAMX-954) Does alignment need to be handled separately for numBatchedFrames > 1?
                result = CSLAlloc(m_pBufferManagerName,
                              &bufferInfo,
                              totalBufferSizeBytes,
                              pAllocFormat->alignment,
                              flags,
                              m_pCreateData->deviceIndices,
                              m_pCreateData->deviceCount);

                // Only update internal state if success
                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                     "[%s][%p] CSLAlloc returned fd=%d handle = %d, size=%d, flags=0x%x, devices=%d",
                                     GetBufferManagerName(), this, bufferInfo.fd, bufferInfo.hHandle, totalBufferSizeBytes,
                                     flags, m_pCreateData->deviceCount);

                    result = ImportCSLBuffer(&m_currentFormat, &bufferInfo);
                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in Importing CSLBuffer for hHandle=%d, result=%s",
                                       GetBufferManagerName(), this, bufferInfo.hHandle, Utils::CamxResultToString(result));

                        if (CamxResultSuccess != CSLReleaseBuffer(bufferInfo.hHandle))
                        {
                            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in releasing CSLBuffer for hHandle=%d",
                                       GetBufferManagerName(), this, bufferInfo.hHandle);
                        }
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                                   "[%s][%p] Allocation failed, size=%d, alignment=%d, flags=0x%x, deviceCount=%d",
                                   GetBufferManagerName(), this, totalBufferSizeBytes, pAllocFormat->alignment,
                                   flags, m_pCreateData->deviceCount);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::BindBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::BindBuffer()
{
    CamxResult result = CamxResultSuccess;

    if (NULL != m_pImageBufferManager)
    {
        result = m_pImageBufferManager->BindBufferManagerImageBuffer(this);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] NULL m_pImageBufferManager", GetBufferManagerName(), this);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::Import
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::Import(
    const ImageFormat*   pCurrentFormat,
    const ChiBufferInfo* pChiBufferInfo,
    SIZE_T               offset,
    SIZE_T               size,
    UINT32               flags,
    const INT32*         pDeviceIndices,
    UINT                 deviceCount,
    BOOL                 delayImport,
    BOOL                 isOutputPortSetup)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCurrentFormat);
    CAMX_ASSERT(TRUE != m_isBacked);

    if ((NULL == pCurrentFormat) || (NULL == pChiBufferInfo) || (FALSE == CamX::IsValidChiBuffer(pChiBufferInfo)))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Invalid arg pCurrentFormat=%p pChiBufferInfo=%p",
                       GetBufferManagerName(), this, pCurrentFormat, pChiBufferInfo);
        result = CamxResultEInvalidArg;
    }
    else if (TRUE == m_isBacked)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Called import with memory allocated internally",
                       GetBufferManagerName(), this);
    }

    if (CamxResultSuccess == result)
    {
        const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();

        if ((TRUE == pSettings->autoImageDump)    ||
            (TRUE == pSettings->dynamicImageDump) ||
            (TRUE == pSettings->reprocessDump)    ||
            (TRUE == pSettings->watermarkImage))
        {
            flags |= CSLMemFlagUMDAccess;
        }

        m_importData.format         = *pCurrentFormat;
        m_importData.bufferInfo     = *pChiBufferInfo;
        m_importData.offset         = offset;
        m_importData.size           = size;
        m_importData.flags          = flags;
        m_importData.deviceCount    = deviceCount;
        m_importData.valid          = TRUE;

        CAMX_ASSERT(CamxMaxDeviceIndex > deviceCount);

        Utils::Memcpy(m_importData.deviceIndices, pDeviceIndices, deviceCount * sizeof(INT32));

        if (FALSE == CamX::IsChiNativeBufferType(&m_importData.bufferInfo))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                             "[%s][%p] HALGralloc or ChiGralloc buffer %d, importing now, size=%d, flags=0x%x, deviceCount=%d, "
                             "delayImport=%d",
                             GetBufferManagerName(), this, m_importData.bufferInfo.bufferType, size, flags, deviceCount,
                             delayImport);

            // i.e Either HALGralloc or ChiGralloc
            // we can directly typecast pChiBufferInfo->phBuffer to BufferHandle
            // We can always import the buffer directly here as the buffer is already allocated.
            // exa -
            //    HAL buffers which are always allocated through Gralloc
            //    ChiBuffers which are allocated using Gralloc in CHI layer using CHIBufferManagers
            // For both of these we will have valid phBufferHandle and can import the buffer
            BufferHandle* phBufferHandle = reinterpret_cast<BufferHandle*>(m_importData.bufferInfo.phBuffer);

            CAMX_ASSERT(NULL != phBufferHandle);

            // For JPEG Node, passing the Gralloc Size for importing buffer,
            // otherwise just validate the gralloc size against the planar size.
            if ((isOutputPortSetup) && (Format::Jpeg == m_importData.format.format))
            {
                // Fetching Gralloc Size For JPEG Node
                result = ImageFormatUtils::GetUnalignedBufferSize((VOID*)*phBufferHandle,
                                                                  m_importData.size,
                                                                  m_importData.format.format,
                                                                  &(m_importData.size));
            }
            else
            {
                // Passing NULL for validation only
                result = ImageFormatUtils::ValidateBufferSize((VOID*)*phBufferHandle, m_importData.size);
            }

            if ((CamxResultSuccess == result) && (FALSE == delayImport))
            {
                result = ImportGrallocBuffer(&m_importData.format,
                                             *phBufferHandle,
                                             m_importData.offset,
                                             m_importData.size,
                                             m_importData.flags,
                                             m_importData.deviceIndices,
                                             m_importData.deviceCount);

                if (CamxResultSuccess == result)
                {
                    m_importData.mappedHere = TRUE;
                }
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in importing Gralloc Buffer type=%d, BufferHandle=%p, "
                               "offset=%d, size=%d, flags=0x%x, deviceCount=%d, WxH %dx%d, format %d",
                               GetBufferManagerName(), this,
                               m_importData.bufferInfo.bufferType, phBufferHandle, m_importData.offset, m_importData.size,
                               m_importData.flags, m_importData.deviceCount, pCurrentFormat->width, pCurrentFormat->height,
                               pCurrentFormat->format);
            }
        }
        else
        {
            // ChiNative Buffer type, i.e CamX ImageBuffer object
            ImageBuffer* pImageBuffer = static_cast<ImageBuffer*>(m_importData.bufferInfo.phBuffer);

            CAMX_ASSERT(NULL != pImageBuffer);

            // Increment the refCount on CHI ImageBuffer which we are now importing.
            // Hold this ref count until camx pipeline is done with using it, i.e release this refCount on
            // RequestIdDone()---> Release()

            // This could actually be dangerous if CHI is not maintaining refCounts properly. By this time, if the pImageBuffer
            // i.e CHI ImageBuffer is in FreeList (because CHI pre-maturely released refCount), below call either :
            //   will move that back from FreeList to BusyList
            //   or
            //   will increment refCount on this ImageBuffer which might have assigned for a different request (can happen if -
            //   once this IB is moved to FreeList, it can be assigned to a new request)
            // "If CHI is not maintaining refCounts properly" - assuming this is anyway a bug which have to be fixed,
            // we can increase the refCount here as we want to hold this buffer during current request.
            pImageBuffer->AddBufferManagerImageReference();

            result = ImportChiNativeBuffer(&m_importData.format,
                                           pImageBuffer,
                                           m_importData.offset,
                                           m_importData.size,
                                           m_importData.flags,
                                           m_importData.deviceIndices,
                                           m_importData.deviceCount);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in importing ImageBuffer [%s][%p] "
                               "offset=%d, size=%d, flags=0x%x, deviceCount=%d",
                               GetBufferManagerName(), this, pImageBuffer->GetBufferManagerName(), pImageBuffer,
                               m_importData.offset, m_importData.size, m_importData.flags, m_importData.deviceCount);
            }
        }
    }

    if ((CamxResultSuccess != result) && (TRUE == CamX::HwEnvironment::GetInstance()->GetStaticSettings()->raisesigabrt))
    {
        OsUtils::RaiseSignalAbort();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::ImportGrallocBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::ImportGrallocBuffer(
    const ImageFormat*  pCurrentFormat,
    const NativeHandle* phNativeBuffer,
    SIZE_T              offset,
    SIZE_T              size,
    UINT32              flags,
    const INT32*        pDeviceIndices,
    UINT                deviceCount)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCurrentFormat);
    CAMX_ASSERT(TRUE != m_isBacked);
    if ((NULL == pCurrentFormat) || (NULL == phNativeBuffer))
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Invalid input argument pCurrentFormat=%p, phNativeBuffer=%p",
                       GetBufferManagerName(), this, pCurrentFormat, phNativeBuffer);
    }
    else if (TRUE == m_isBacked)
    {
        result = CamxResultEInvalidState;
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Called import with memory allocated internally",
                       GetBufferManagerName(), this);
    }

    if (CamxResultSuccess == result)
    {
        CSLBufferInfo   bufferInfo;

        result = CSLMapNativeBuffer(&bufferInfo,
                                    reinterpret_cast<const CSLNativeHandle*>(phNativeBuffer),
                                    offset,
                                    size,
                                    flags,
                                    pDeviceIndices,
                                    deviceCount);
        // Only update internal state if success
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "ImportGrallocBuffer [%s][%p]    fd=%d, flags=0x%x, size=%d, hdl=0x%x",
                              GetBufferManagerName(), this, phNativeBuffer->data[0],
                              flags, size, bufferInfo.hHandle);

            result = ImportCSLBuffer(pCurrentFormat, &bufferInfo);
            if (CamxResultSuccess == result)
            {
                m_currentFormat     = *pCurrentFormat;
                m_bValidFormat      = TRUE;
                m_phNativeBuffer    = phNativeBuffer;
            }
            else
            {
                if (CamxResultSuccess != CSLReleaseBuffer(bufferInfo.hHandle))
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] CSLReleaseBuffer failed handle %d",
                           GetBufferManagerName(), this, bufferInfo.hHandle);
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] CSLMapNativeBuffer failed with %s",
                           GetBufferManagerName(), this, Utils::CamxResultToString(result));
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::ImportChiNativeBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::ImportChiNativeBuffer(
    const ImageFormat*  pCurrentFormat,
    ImageBuffer*        pImageBuffer,
    SIZE_T              offset,
    SIZE_T              size,
    UINT32              flags,
    const INT32*        pDeviceIndices,
    UINT                deviceCount)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCurrentFormat);
    CAMX_ASSERT(NULL != pImageBuffer);

    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                     "[%s][%p] Importing ChiNative buffer [%s][%p] with LateBinding=%d, BackingBuffer=%d, GrallocBuffer=%d, "
                     "offset=%d, size=%d, flags=0x%x, deviceCount=%d",
                     GetBufferManagerName(),               this,
                     pImageBuffer->GetBufferManagerName(), pImageBuffer,
                     pImageBuffer->IsLateBindingEnabled(),
                     pImageBuffer->HasBackingBuffer(),
                     pImageBuffer->IsGrallocBuffer(),
                     offset,
                     size,
                     flags,
                     deviceCount);

    // Check if the ImageBuffer being imported already has a backing buffer.
    // There is a possibility that it may not be having backing buffer if called from Import() if LateBinding is enabled.
    // In such cases, this node will trigger BindBuffers() on its input, output ImageBuffers when it really needs
    // the buffers. This will trigger BindBuffer on ImageBuffer object being imported and then we again come here and
    // import Buffer information into the current ImageBuffer object.
    if (TRUE == pImageBuffer->HasBackingBuffer())
    {
        // ImageBuffer being imported already has backing buffer, so we can import the buffer now.

        // If MPM is not enabled : Map buffer here and release in Release(), i.e map, unmap happens every frame
        // If MPM is enabled     : MPM has capability of incremental mapping and maintaining handles, so call into
        //                         the ImageBuffer being imported to map, which will call MPM to do mapping.
        //                         Since MPM maintains buffers, handles - we will not be doing map-unmap every frame.
        //                         But drawback with this is the buffer is mapped (to current devices) through out its
        //                         lifetime irrespective of the buffer being used or not.
        if (FALSE == HwEnvironment::GetInstance()->GetStaticSettings()->MPMEnable)
        {
            if (TRUE == pImageBuffer->IsGrallocBuffer())
            {
                BufferHandle* phBufferHandle = pImageBuffer->GetGrallocBufferHandle();

                CAMX_ASSERT(NULL != phBufferHandle);
                if (NULL != phBufferHandle)
                {
                    result = ImportGrallocBuffer(pCurrentFormat,
                                                 *phBufferHandle,
                                                 offset,
                                                 size,
                                                 flags,
                                                 pDeviceIndices,
                                                 deviceCount);
                }
            }
            else
            {
                CSLBufferInfo bufferInfo = { };

                result = CSLMapBuffer(&bufferInfo,
                                      pImageBuffer->GetFileDescriptor(),
                                      offset,
                                      size,
                                      flags,
                                      pDeviceIndices,
                                      deviceCount);

                if (CamxResultSuccess == result)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                     "[%s][%p] CSLMap returned fd=%d handle = %d, size=%d, flags=0x%x, devices=%d",
                                     GetBufferManagerName(), this, bufferInfo.fd, bufferInfo.hHandle, size,
                                     flags, m_pCreateData->deviceCount);

                    result = ImportCSLBuffer(pCurrentFormat, &bufferInfo);

                    if (CamxResultSuccess == result)
                    {
                        m_currentFormat = *pCurrentFormat;
                        m_bValidFormat  = TRUE;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] : Failed in ImportCSLBuffer",
                                       GetBufferManagerName(), this);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                                   "[%s][%p] : Failed in CSLMapBuffer [%s][%p] : fd=%d, offset=%d, size=%d, flags=0x%x, "
                                   "deviceCount=%d",
                                   GetBufferManagerName(), this,
                                   pImageBuffer->GetBufferManagerName(), pImageBuffer,
                                   pImageBuffer->GetFileDescriptor(), offset, size, flags,
                                   deviceCount);
                }
            }

            m_importData.mappedHere = TRUE;
        }
        else
        {
            // MPM is enabled. MPM supports incremental mapping, so call into the ImageBuffer being imported for incremental
            // mapping, instead of mapping the buffer locally
            result = pImageBuffer->MapBufferToAdditionalDevices(offset,
                                                                size,
                                                                flags,
                                                                pDeviceIndices,
                                                                deviceCount);

            if (CamxResultSuccess == result)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "CHIBuffer [%s]  bufHandle=%p, memHdl=0x%x, flags=0x%x, size=%d",
                                GetBufferManagerName(), m_importData.bufferInfo.phBuffer,
                                pImageBuffer->GetCSLBufferInfo()->hHandle, flags, size);

                const CSLBufferInfo* pBufferInfo = pImageBuffer->GetCSLBufferInfo();

                result = ImportCSLBuffer(pCurrentFormat, pBufferInfo);

                if (CamxResultSuccess == result)
                {
                    m_currentFormat = *pCurrentFormat;
                    m_bValidFormat  = TRUE;

                    m_phNativeBuffer = reinterpret_cast<const NativeHandle*>(pImageBuffer->GetNativeBufferHandle());
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] : Failed in ImportCSLBuffer", GetBufferManagerName(), this);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                               "[%s][%p] : Failed in MapBuffer : fd=%d, offset=%d, size=%d, flags=0x%x, "
                               "deviceCount=%d",
                               GetBufferManagerName(), this, pImageBuffer->GetFileDescriptor(), offset, size, flags,
                               deviceCount);
            }

            m_importData.mappedHere = FALSE;
        }
    }
    else
    {
        // ImageBuffer being imported has LateBinding enabled, so it is not having backing buffers yet.
        // Just save the ImageBuffer object and return. We will trigger allocation on Imported ImageBuffer object
        // and import the buffer in BindImageBuffers() which will be triggerred when the buffers are really needed.

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s][%p] : Defer importing buffer from [%s][%p]",
                         GetBufferManagerName(), this, pImageBuffer->GetBufferManagerName(), pImageBuffer);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::ImportCSLBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::ImportCSLBuffer(
    const ImageFormat*      pCurrentFormat,
    const CSLBufferInfo*    pBufferInfo)
{
    CamxResult  result              = CamxResultEResource;
    SIZE_T      requiredBufferSize  = ImageFormatUtils::GetTotalSize(pCurrentFormat);
    UINT        numberOfPlanes      = ImageFormatUtils::GetNumberOfPlanes(pCurrentFormat);

    CAMX_ASSERT(pBufferInfo->size >= requiredBufferSize);

    if (pBufferInfo->size >= requiredBufferSize)
    {
        m_bufferInfo            = *pBufferInfo;
        m_planeStartOffset[0]   = 0;

        for (UINT i = 0; i < numberOfPlanes; i++)
        {
            m_planeSize[i] = ImageFormatUtils::GetPlaneSize(pCurrentFormat, i);

            if (i > 0)
            {
                if (TRUE == IsBatchImageFormat())
                {
                    m_planeStartOffset[i] = (m_planeSize[i - 1] + m_planeStartOffset[i - 1]);
                }
                else
                {
                    // All planes reside one after the other i.e. all Y planes together, followed by Cb/Cr/CbCr planes together
                    m_planeStartOffset[i] = (m_planeSize[i - 1] * m_numBatchedFrames);
                }
            }
            m_metadataSize[i]         = pCurrentFormat->formatParams.yuvFormat[i].metadataSize;
        }

        m_isBacked = TRUE;

        result     = CamxResultSuccess;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                       "[%s][%p] Cannot import CSL buffer. Buffer size %d, Required=%d, width=%d, height=%d, format=%d",
                       GetBufferManagerName(), this, pBufferInfo->size, requiredBufferSize,
                       pCurrentFormat->width, pCurrentFormat->height, pCurrentFormat->format);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::Release
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBuffer::Release(
    BOOL isForced)
{
    UINT        referenceCount  = GetReferenceCount();
    CamxResult  result          = CamxResultSuccess;

    if (TRUE == m_isBacked)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s][%p] : Release : m_hMemPoolBufMgrHandle=%p, allocateBufferMemory=%d, "
                         "mappedHere=%d, hHandle=%d, m_hGrallocBuffer=%p, isForced=%d",
                         GetBufferManagerName(), this, m_hMemPoolBufMgrHandle, m_pCreateData->allocateBufferMemory,
                         m_importData.mappedHere, m_bufferInfo.hHandle, m_hGrallocBuffer, isForced);

        if ((NULL != m_hMemPoolBufMgrHandle) &&
            (TRUE == m_pCreateData->allocateBufferMemory))
        {
            CAMX_ASSERT(NULL != m_hMemPoolBufferHandle);

            result = MemPoolMgr::ReleaseBufferToPool(m_hMemPoolBufMgrHandle, m_hMemPoolBufferHandle, FALSE);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in releasing buffer to Mem Pool, result=%s",
                               GetBufferManagerName(), this, Utils::CamxResultToString(result));
            }

            m_hMemPoolBufferHandle = NULL;
        }
        else
        {
            // Release if
            //     1. If this is an ImageBuffer(camx internal or CHI buffers) which allocated buffer without using MPM
            //     2. If this ImageBuffer imported HALGralloc or ChiGralloc ChiBuffers and mapped directly into this object
            //     3. If this ImageBuffer imported ChiNative ChiBuffer which was allocated without using MPM
            // Do not Release if
            //     1. If this ImageBuffer imported ChiNative ChiBuffer which was allocated using MPM. We dont need to release
            //        in this case as MPM maintains incremental mappings for future usage over the lifetyme of buffer.
            //        MPM will release all incremental mappings when the buffer is actually freed.
            if ((TRUE == m_pCreateData->allocateBufferMemory) || (TRUE == m_importData.mappedHere))
            {
                // For buffers that are allocated using Gralloc and not mapped to CSL, we will not have valid hHandle.
                // This can happen for CHI Buffer allocations with Gralloc heap
                if (0 != m_bufferInfo.hHandle)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "Release [%s][%p] : fd=%d, flags=0x%x, size=%d, hdl=0x%x",
                                      GetBufferManagerName(), this, m_bufferInfo.fd, m_bufferInfo.flags, m_bufferInfo.size,
                                      m_bufferInfo.hHandle);

                    if (TRUE == isForced)
                    {
                        result = CSLReleaseBufferForced(m_bufferInfo.hHandle);
                    }
                    else
                    {
                        result = CSLReleaseBuffer(m_bufferInfo.hHandle);
                    }

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in CSLRelease, handle=%d, result=%s",
                                       GetBufferManagerName(), this, m_bufferInfo.hHandle, Utils::CamxResultToString(result));
                    }
                }

                if (NULL != m_hGrallocBuffer)
                {
                    Gralloc* pGralloc =  Gralloc::GetInstance();

                    if (NULL != pGralloc)
                    {
                        if (CamxResultSuccess != pGralloc->Destroy(m_hGrallocBuffer))
                        {
                            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Release called on an invalid handle",
                                           GetBufferManagerName(), this);
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Unable to get gralloc handle",
                                       GetBufferManagerName(), this);
                    }
                }
            }
        }

        Utils::Memset(&m_bufferInfo, 0, sizeof(m_bufferInfo));
        Utils::Memset(&m_metadataSize[0], 0, sizeof(m_metadataSize));

        // Reset fd value to -1 as 0 is a valid fd.
        m_bufferInfo.fd = -1;

        m_hGrallocBuffer = NULL;
        m_phNativeBuffer = NULL;
        m_isBacked       = FALSE;
    }

    // During flush it may happen that import data is already populated with NativeChiBuffer info
    // but camx internal image buffer is not backed up with buffer because of early termination in
    // flush call. So, the binding of buffer to image buffer is not necesarily going to happen.
    // As refCount for NativeChi image buffer is incremented in import, so, it is required decrement
    // the refCount of NativeChi image buffer. So, decrementing here if the import data is valid
    if (TRUE == m_importData.valid)
    {
        if ((FALSE == m_pCreateData->allocateBufferMemory) &&
            (TRUE == CamX::IsChiNativeBufferType(&m_importData.bufferInfo)))
        {
            // This is a holder ImageBuffer object for source/sink ports - which imported a ChiNativeBuffer
            // We had added a refCount while importing m_importData.bufferInfo.phBuffer (which is a CHI ImageBuffer)
            // into current holder ImageBuffer object.
            // Since we are done with using it, release the refCount now.
            ImageBuffer* pImageBuffer = static_cast<ImageBuffer*>(m_importData.bufferInfo.phBuffer);

            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s][%p] : Releasing Native Chi buffer RefCount : NativeChiBuffer:%p"
                " CurRefCount=%d", GetBufferManagerName(), this, pImageBuffer, pImageBuffer->GetReferenceCount());

            // This could actually be dangerous if CHI is not maintaining refCounts properly on this Chi ImageBuffer.
            // Below call may move the Chi ImageBuffer from BusyList to FreeList if refCount is 0
            // If Chi is not properly doing refCount, this may become 0 and the buffer will be released,
            // though CHI override still try to use it.
            // "If CHI is not maintaining refCounts properly" - assuming this is anyway a bug which have to be fixed,
            // we can release the refCount here as we are done with using this buffer during current request.
            pImageBuffer->ReleaseBufferManagerImageReference();
        }

        Utils::Memset(&m_importData, 0, sizeof(m_importData));
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::AddBufferManagerImageReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ImageBuffer::AddBufferManagerImageReference()
{
    UINT count = 0;

    if (NULL != m_pImageBufferManager)
    {
        count = m_pImageBufferManager->AddReference(this);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] NULL m_pImageBufferManager", GetBufferManagerName(), this);
    }

    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::ReleaseBufferManagerImageReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT ImageBuffer::ReleaseBufferManagerImageReference()
{
    UINT count = 0;

    if (NULL != m_pImageBufferManager)
    {
        count = m_pImageBufferManager->ReleaseReference(this);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] NULL m_pImageBufferManager", GetBufferManagerName(), this);
    }

    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::GetPlaneCSLMemHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::GetPlaneCSLMemHandle(
    UINT            batchFrameIndex,
    UINT            planeIndex,
    CSLMemHandle*   phMemHandle,
    SIZE_T*         pOffset,
    SIZE_T*         pMetadataSize
    ) const
{
    CamxResult  result      = CamxResultSuccess;
    const UINT  planeCount  = GetNumberOfPlanes();

    CAMX_ASSERT(TRUE == m_isBacked);
    CAMX_ASSERT((NULL != phMemHandle) && (NULL != pOffset) && (NULL != pMetadataSize) && (planeIndex < planeCount));
    CAMX_ASSERT(batchFrameIndex < m_numBatchedFrames);

    if (TRUE != m_isBacked)
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] : m_isBacked is FALSE", GetBufferManagerName(), this);
        result = CamxResultEInvalidState;
    }
    else if ((NULL == phMemHandle) || (NULL == pOffset) || (NULL == pMetadataSize) || (planeIndex >= planeCount))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr,
                       "[%s][%p] : phMemHandle=%p, pOffset=%p, pMetadataSize=%p, planeIndex=%d, planeCount=%d",
                       GetBufferManagerName(), this,
                       phMemHandle, pOffset, pMetadataSize, planeIndex, planeCount);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        SIZE_T  bufferSize  = 0;
        *phMemHandle        = m_bufferInfo.hHandle;
        if (FALSE == IsBatchImageFormat())
        {
            *pOffset   = (m_planeStartOffset[planeIndex] + (batchFrameIndex * m_planeSize[planeIndex]));
        }
        else
        {
            bufferSize = ImageFormatUtils::GetTotalSize(&m_currentFormat) / m_numBatchedFrames;
            *pOffset   = (m_planeStartOffset[planeIndex] + (batchFrameIndex * bufferSize));
        }
        *pMetadataSize   = m_metadataSize[planeIndex];
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::MapBufferToAdditionalDevices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::MapBufferToAdditionalDevices(
    SIZE_T          offset,
    SIZE_T          size,
    UINT32          flags,
    const INT32*    pDeviceIndices,
    UINT            deviceCount)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(TRUE == m_pCreateData->allocateBufferMemory);

    // Support incremental mapping only if MPM is enabled
    if (NULL != m_hMemPoolBufMgrHandle)
    {
        if (NULL != m_hMemPoolBufferHandle)
        {
            result = MemPoolMgr::MapBufferToDevices(m_hMemPoolBufMgrHandle, m_hMemPoolBufferHandle,
                                                    offset, size, flags, pDeviceIndices, deviceCount);

            if (CamxResultSuccess == result)
            {
                CSLBufferInfo bufferInfo;

                result = MemPoolMgr::GetBufferInfo(m_hMemPoolBufMgrHandle, m_hMemPoolBufferHandle,
                                                   &bufferInfo, &m_hGrallocBuffer);

                if (CamxResultSuccess == result)
                {
                    result = ImportCSLBuffer(&m_currentFormat, &bufferInfo);

                    if ((CamxResultSuccess == result) && (NULL != m_hGrallocBuffer))
                    {
                        BufferHandle*       phBufferHandle = reinterpret_cast<BufferHandle*>(&m_hGrallocBuffer);
                        const NativeHandle* phNativeBuffer = *phBufferHandle;

                        m_phNativeBuffer = phNativeBuffer;
                    }
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] : Failed in MapBufferToDevices, result=%s",
                               GetBufferManagerName(), this, Utils::CamxResultToString(result));
            }
        }
        else
        {
            result = CamxResultEInvalidState;
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] : Invalid m_hMemPoolBufferHandle", GetBufferManagerName(), this);
        }
    }
    else
    {
        result = CamxResultENotImplemented;
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] : Mapping not supported", GetBufferManagerName(), this);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::BindImportedBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult ImageBuffer::BindImportedBuffer()
{
    CamxResult result = CamxResultSuccess;

    if ((FALSE      == m_importData.valid)                  ||
        (NULL       == m_importData.bufferInfo.phBuffer))
    {
        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Invalid BindImportedBuffer request valid=%d, bufferType=%d, phBuffer=%p",
                       GetBufferManagerName(), this,
                       m_importData.valid, m_importData.bufferInfo.bufferType, m_importData.bufferInfo.phBuffer);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        if (FALSE == CamX::IsChiNativeBufferType(&m_importData.bufferInfo))
        {
            BufferHandle* phBufferHandle = reinterpret_cast<BufferHandle*>(m_importData.bufferInfo.phBuffer);

            if (NULL != phBufferHandle)
            {
                result = ImportGrallocBuffer(&m_importData.format,
                                             *phBufferHandle,
                                             m_importData.offset,
                                             m_importData.size,
                                             m_importData.flags,
                                             m_importData.deviceIndices,
                                             m_importData.deviceCount);

                if (CamxResultSuccess == result)
                {
                    m_importData.mappedHere = TRUE;
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in binding imported Gralloc buffer [%p], result=%s",
                               GetBufferManagerName(), this, phBufferHandle, Utils::CamxResultToString(result));
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Invalid Gralloc buffer [%p] to import",
                               GetBufferManagerName(), this, phBufferHandle);
            }
        }
        else
        {
            ImageBuffer* pImageBuffer = static_cast<ImageBuffer*>(m_importData.bufferInfo.phBuffer);

            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                             "[%s][%p] : Binding imported ImageBuffer [%s][%p] and then import buffer information",
                             GetBufferManagerName(), this, pImageBuffer->GetBufferManagerName(), pImageBuffer);

            // First trigger bind buffer on the ImageBuffer object that is being imported into this ImageBuffer object
            // This will allocate buffer if it is not yet backed with a buffer
            result = pImageBuffer->BindBuffer();
            if (CamxResultSuccess == result)
            {
                result = ImportChiNativeBuffer(&m_importData.format,
                                               pImageBuffer,
                                               m_importData.offset,
                                               m_importData.size,
                                               m_importData.flags,
                                               m_importData.deviceIndices,
                                               m_importData.deviceCount);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in importing ImageBuffer [%s][%p] "
                                   "offset=%d, size=%d, flags=0x%x, deviceCount=%d",
                                   GetBufferManagerName(), this, pImageBuffer->GetBufferManagerName(), pImageBuffer,
                                   m_importData.offset, m_importData.size, m_importData.flags, m_importData.deviceCount);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Failed in binding imported ImageBuffer [%s][%p], result=%s",
                               GetBufferManagerName(), this, pImageBuffer->GetBufferManagerName(), pImageBuffer,
                               Utils::CamxResultToString(result));
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageBuffer::IsGrallocBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageBuffer::IsGrallocBuffer() const
{
    BOOL isGralloc = FALSE;

    if (CSLBufferHeapEGL == m_pCreateData->bufferProperties.bufferHeap)
    {
        isGralloc = TRUE;
    }

    return isGralloc;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageBuffer::IsCSLBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageBuffer::IsCSLBuffer() const
{
    BOOL isCSL = FALSE;

    if ((CSLBufferHeapSystem == m_pCreateData->bufferProperties.bufferHeap) ||
        (CSLBufferHeapIon    == m_pCreateData->bufferProperties.bufferHeap) ||
        (CSLBufferHeapDSP    == m_pCreateData->bufferProperties.bufferHeap))
    {
        isCSL = TRUE;
    }

    return isCSL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ImageBuffer::IsLateBindingEnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ImageBuffer::IsLateBindingEnabled() const
{
    BOOL bEnabled = FALSE;

    if ((NULL != m_pCreateData) && (TRUE == m_pCreateData->bEnableLateBinding))
    {
        bEnabled = TRUE;
    }

    return bEnabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ImageBuffer::DumpState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ImageBuffer::DumpState(
    INT     fd,
    UINT32  indent)
{
    CAMX_LOG_TO_FILE(fd, indent,
                    "ImageBuffer                 { refCount:%u, backed:%d, memHandle:%d, virtualAddr:%p, fd:%d, flags:0x%08X}",
                     CamxAtomicLoadU(&m_aReferenceCount),
                     m_isBacked,
                     m_bufferInfo.hHandle,
                     m_bufferInfo.pVirtualAddr,
                     m_bufferInfo.fd,
                     m_bufferInfo.flags);
}


CAMX_NAMESPACE_END
