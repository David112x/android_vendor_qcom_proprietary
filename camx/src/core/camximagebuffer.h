////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camximagebuffer.h
///
/// @brief Image Buffer definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIMAGEBUFFER_H
#define CAMXIMAGEBUFFER_H

#include "camxtypes.h"
#include "camxformats.h"
#include "camxcsl.h"
#include "camximagebuffermanager.h"
#include "camxcommontypes.h"
#include "camxmempoolmgr.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class  ImageBufferManager;
class  ImageBuffer;
struct NativeHandle;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief Bufferproperties structure that is initialized by looking at the corresponding XML structure
struct ImageBufferImportData
{
    BOOL            valid;                              ///< Whether import data is valid
    ImageFormat     format;                             ///< Format info to import
    BOOL            mappedHere;                         ///< Whether the buffer is directly mapped here or called actual
                                                        ///  ImageBuffer for incremental mapping. Always TRUE for HALGralloc,
                                                        ///  ChiGralloc ChiBuffers. TRUE or FALSE for ChiNative buffers
                                                        ///  depending upon Memory Pool Manager usage.
                                                        ///  When TRUE, this ImageBuffer object has to call CSLReleaseBuffer
                                                        ///  to make sure the buffer is unmapped. If FALSE, MPM will take care.
    ChiBufferInfo   bufferInfo;                         ///< Imported Buffer information
    SIZE_T          offset;                             ///< Offset in the buffer
    SIZE_T          size;                               ///< Size of the buffer
    UINT32          flags;                              ///< Mem Flags
    INT32           deviceIndices[CamxMaxDeviceIndex];  ///< List of devices to map
    UINT            deviceCount;                        ///< Number of valid devices in deviceIndices array
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of an image buffer including planes and format definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImageBuffer
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImageBuffer
    ///
    /// @brief  Constructor. Newly created image buffer will not contain any valid memory.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit ImageBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ImageBuffer
    ///
    /// @brief  Destructor. Will release any memory allocated. Buffers are not valid after the destructor is called.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~ImageBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Function for initializing ImageBuffer object
    ///
    /// @param  pBufferMgrName          Pointer to client Buffer Manager name. ImageBuffer class saves this pointer for future
    ///                                 and expects this memory to be valid over the lifetime of this Buffer Manager
    /// @param  pCreateData             Pointer to client Buffer Manager create data. ImageBuffer class saves this pointer
    ///                                 for future reference and expects this memory to be valid over the lifetime of this
    ///                                 Buffer Manager and is not changed.
    /// @param  pCurrentFormat          Current Image Format for this ImageBuffer. This is used to import csl buffer info
    /// @param  pImageBufferManager     Buffer manager that allocated the buffer (optional parameter which should be set
    ///                                 to NULL if no buffer manager is required to be specified)
    /// @param  hMemPoolBufMgrHandle    Memory Pool Buffer Manager handle using which this ImageBuffer can get a buffer
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        const CHAR*                     pBufferMgrName,
        const BufferManagerCreateData*  pCreateData,
        const ImageFormat*              pCurrentFormat,
        ImageBufferManager*             pImageBufferManager,
        MemPoolBufMgrHandle             hMemPoolBufMgrHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Allocate
    ///
    /// @brief  Allocate memory for the image buffer of the given format. Buffer properties based on CreateData passed while
    ///         creating this ImageBuffer object is used to allocate memory
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Allocate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BindBuffer
    ///
    /// @brief  Bind backing buffer for this ImageBuffer. If allocation is required for this ImageBuffer and buffer is not
    ///         yet allocated or bound to this ImageBuffer due to "Delayed Binding", buffer (late) binding is triggerred by
    ///         this API.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult BindBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateImageFormat
    ///
    /// @brief  Update Image Format info for this Image Buffer. This is used to import CSL buffer information.
    ///         This could be different from the ImageFormat used for allocation. But the buffer size calculated using this
    ///         ImageFormat has to be less than or equal to buffer size calculated based on allocation ImageFormat.
    ///         This is to support allocating a bigger buffer and then using the same buffer for different format properties.
    ///
    /// @param  pFormat     The format of the buffer to be used to import csl buffer information.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateImageFormat(
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Import
    ///
    /// @brief  Import previously allocated memory to be used with this memory buffer.
    ///
    /// @param  pFormat             The format of the buffer to import.
    /// @param  pChiBufferInfo      The ChiBufferInfo struct contain native handle of the buffer to import.
    /// @param  offset              Offset relative to the memory described by fd.
    /// @param  size                Size of the memory described by FD.
    /// @param  flags               Usage flags for the memory to be mapped for this image buffer.
    /// @param  pDeviceIndices      List of device indices that may access this buffer.
    /// @param  deviceCount         The number of device indices pointed to by pDeviceIndices.
    /// @param  delayImport         Whether import needs to be delayed until Bind is called
    /// @param  isOutputPortSetup   Whether importing for output port setup
    ///
    /// @return CamxResultSuccess if successful. If the previously allocated memory is not able to contain an image described
    ///         by pFormat, CamxResultEResource will be returned.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Import(
        const ImageFormat*   pFormat,
        const ChiBufferInfo* pChiBufferInfo,
        SIZE_T               offset,
        SIZE_T               size,
        UINT32               flags,
        const INT32*         pDeviceIndices,
        UINT                 deviceCount,
        BOOL                 delayImport,
        BOOL                 isOutputPortSetup);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Release
    ///
    /// @brief  Release any allocated memory for this device and all references to underlying memory.
    ///
    /// @param  isForced  force to release the image buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Release(
        BOOL isForced);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddImageReference
    ///
    /// @brief  Add a reference to this image buffer. Only ImageBufferManager should call this API
    ///         as buffers are all managed through buffer manager.
    ///
    /// @return The updated reference count.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT AddImageReference()
    {
        UINT count = CamxAtomicIncU(&m_aReferenceCount);

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s][%p] ReferenceCount for ImageBuffer is being incremented to %d",
                         GetBufferManagerName(), this, count);

        return count;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseImageReference
    ///
    /// @brief  Release a reference to this image buffer. Only ImageBufferManager should call this API
    ///         as buffers are all managed through buffer manager.
    ///
    /// @return The updated reference count.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT ReleaseImageReference()
    {
        CAMX_ASSERT_MESSAGE(0 <= m_aReferenceCount, "Releasing an ImageBuffer with no references");

        UINT count = CamxAtomicDecU(&m_aReferenceCount);

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "[%s][%p] ReferenceCount is decremented to %d",
                         GetBufferManagerName(), this, count);

        return count;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddBufferManagerImageReference
    ///
    /// @brief  Add a reference to this image buffer via ImageBufferManager.
    ///
    /// @return The updated reference count.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT AddBufferManagerImageReference();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseBufferManagerImageReference
    ///
    /// @brief  Release a reference to this image buffer via ImageBufferManager.
    ///
    /// @return The updated reference count.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT ReleaseBufferManagerImageReference();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetReferenceCount
    ///
    /// @brief  Get the current reference count.
    ///
    /// @return The reference count.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetReferenceCount()
    {
        return CamxAtomicLoadU(&m_aReferenceCount);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFormat
    ///
    /// @brief  Get the format of the buffer
    ///
    /// @return The image format. Will return NULL if the image buffer is not populated.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const ImageFormat* GetFormat() const
    {
        CAMX_ASSERT(TRUE == m_bValidFormat);

        if (FALSE == m_bValidFormat)
        {
            return NULL;
        }

        return &m_currentFormat;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumberOfPlanes
    ///
    /// @brief  Get the number of planes for the buffer
    ///
    /// @return The number of planes.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetNumberOfPlanes() const
    {
        UINT planeCount = 0;

        CAMX_ASSERT(TRUE == m_bValidFormat);

        if (TRUE == m_bValidFormat)
        {
            planeCount = ImageFormatUtils::GetNumberOfPlanes(&m_currentFormat);
        }

        return planeCount;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPlaneSize
    ///
    /// @brief  Get the size of a plane in the buffer in bytes.
    ///
    /// @param  planeIndex  The plane to get.
    ///
    /// @return The number of bytes in the plane. Will return 0 if the image buffer is not populated.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE SIZE_T GetPlaneSize(
        UINT planeIndex
        ) const
    {
        SIZE_T      planeSize   = 0;
        const UINT  planeCount  = GetNumberOfPlanes();
        CamxResult  result      = CamxResultSuccess;

        CAMX_ASSERT(TRUE == m_bValidFormat);
        CAMX_ASSERT(planeIndex < planeCount);

        if ((TRUE == m_bValidFormat) && (planeIndex < planeCount))
        {
            if (Format::Jpeg == m_currentFormat.format)
            {
                BufferHandle* phBufferHandle = reinterpret_cast<BufferHandle*>(m_importData.bufferInfo.phBuffer);

                if (NULL != phBufferHandle)
                {
                    result = ImageFormatUtils::GetUnalignedBufferSize((VOID*)*phBufferHandle, planeSize,
                        m_currentFormat.format, &(planeSize));

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "GetGrallocBufferSize, returned error : %d", result);
                    }
                }
            }

            if (0 == planeSize)
            {
                planeSize = ImageFormatUtils::GetPlaneSize(&m_currentFormat, planeIndex);
            }
        }

        return planeSize;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFileDescriptor
    ///
    /// @brief  Get the fileDescriptor for the image.
    ///
    /// @return File Descriptor.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE INT GetFileDescriptor() const
    {
        if ((FALSE == IsInValidUsageState()) || (FALSE == m_isBacked))
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Not in valid state, m_isBacked=%d returning invalid/incorrect fd=%d",
                           GetBufferManagerName(), this, m_isBacked, m_bufferInfo.fd);
        }

        return m_bufferInfo.fd;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNativeBufferHandle
    ///
    /// @brief  Get the native buffer handle for the image.
    ///
    /// @return The handle to the native buffer.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID* GetNativeBufferHandle() const
    {
        if ((FALSE == IsInValidUsageState()) || (FALSE == m_isBacked))
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Not in valid state, m_isBacked=%d",
                           GetBufferManagerName(), this, m_isBacked);
        }

        return (VOID *)m_phNativeBuffer;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPlaneVirtualAddr
    ///
    /// @brief  Get the virtual address of a plane in the image buffer.
    ///
    /// @param  batchFrameIndex If the imagebuffer is batched, then we set this to the frame index within the batch
    /// @param  planeIndex      The plane to get.
    ///
    /// @return The virtual address for the plane. Will return NULL if the image buffer is not populated or the buffer is not
    ///         mapped into the UMD.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BYTE* GetPlaneVirtualAddr(
        UINT batchFrameIndex,
        UINT planeIndex
        ) const
    {
        BYTE*       pVirtualAddr    = NULL;
        const UINT  planeCount      = GetNumberOfPlanes();

        if ((FALSE == IsInValidUsageState()) || (FALSE == m_isBacked))
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Not in valid state, m_isBacked=%d",
                           GetBufferManagerName(), this, m_isBacked);
        }

        CAMX_ASSERT(TRUE == m_isBacked);
        CAMX_ASSERT(planeIndex < planeCount);
        CAMX_ASSERT(batchFrameIndex < m_numBatchedFrames);

        if ((TRUE == m_isBacked) && (planeIndex < planeCount))
        {
            pVirtualAddr = (static_cast<BYTE*>(m_bufferInfo.pVirtualAddr)   +
                            m_planeStartOffset[planeIndex]                  +
                            (batchFrameIndex * m_planeSize[planeIndex]));
        }

        return pVirtualAddr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPlaneCSLMemHandle
    ///
    /// @brief  Get the CSL Buffer information for a plane in the image buffer.
    ///
    /// @param  batchFrameIndex If the imagebuffer is batched, then we set this to the frame index within the batch
    /// @param  planeIndex      The plane to get.
    /// @param  phMemHandle     CSL memory handle out parameter.
    /// @param  pOffset         Output parameter. Offset of plane within the memory referred to by phMemHandle.
    /// @param  pMetadataSize   Output parameter. Metadatasize of the plane. For linear format Meta Size would be 0.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPlaneCSLMemHandle(
        UINT            batchFrameIndex,
        UINT            planeIndex,
        CSLMemHandle*   phMemHandle,
        SIZE_T*         pOffset,
        SIZE_T*         pMetadataSize
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HasBackingBuffer
    ///
    /// @brief  Check whether the Image buffer is valid, having been backed by memory.
    ///
    /// @return TRUE if the ImageBuffer is valid and backed by memory, FALSE otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL HasBackingBuffer() const
    {
        return m_isBacked;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumFramesInBatch
    ///
    /// @brief  Retrieve the total frame numbers in one batch
    ///
    /// @return Return the number of frames in one batch
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetNumFramesInBatch() const
    {
        return m_numBatchedFrames;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CacheOps
    ///
    /// @brief  Clean and/or invalidate the cache for all memory associated with this image buffer.
    ///
    /// @param  invalidate  Invalidate the cache.
    /// @param  clean       Clean the cache.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult CacheOps(
        BOOL invalidate,
        BOOL clean)
    {
        CamxResult result = CamxResultEInvalidState;

        CAMX_ASSERT(TRUE == m_isBacked);

        if ((TRUE == m_isBacked) && (0 != m_bufferInfo.hHandle))
        {
            CSLBufferCacheOp(m_bufferInfo.hHandle, invalidate, clean);
            result = CamxResultSuccess;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] CacheOps failed, m_isBacked=%d hHandle=%d",
                           GetBufferManagerName(), this, m_isBacked, m_bufferInfo.hHandle);
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCPUAddress
    ///
    /// @brief  Retrieve the base virtual address for the image buffer
    ///
    /// @return Const byte pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const BYTE* GetCPUAddress() const
    {
        if ((FALSE == IsInValidUsageState()) || (FALSE == m_isBacked))
        {
            if (FALSE == m_isBacked)
            {
                CAMX_LOG_WARN(CamxLogGroupMemMgr, "[%s][%p] Not in valid state, m_isBacked=%d",
                               GetBufferManagerName(), this, m_isBacked);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Not in valid state, m_isBacked=%d",
                               GetBufferManagerName(), this, m_isBacked);
            }
        }

        return static_cast<BYTE*>(m_bufferInfo.pVirtualAddr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGrallocBufferHandle
    ///
    /// @brief  Retrieve the gralloc buffer handle
    ///
    /// @return Return the gralloc buffer handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BufferHandle* GetGrallocBufferHandle()
    {
        BufferHandle* phBufferHandle = NULL;

        if ((FALSE == IsInValidUsageState()) || (FALSE == m_isBacked))
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Not in valid state, m_isBacked=%d",
                           GetBufferManagerName(), this, m_isBacked);
        }
        else
        {
            phBufferHandle = &m_hGrallocBuffer;
        }

        return phBufferHandle;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsGrallocBuffer
    ///
    /// @brief  Check if the image buffer is allocated through gralloc
    ///
    /// @return Return true if it's gralloc buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsGrallocBuffer() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCSLBuffer
    ///
    /// @brief  Check if the image buffer is allocated through CSL
    ///
    /// @return Return true if it's CSL buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsCSLBuffer() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLBufferInfo
    ///
    /// @brief  Retrieve the gralloc buffer handle
    ///
    /// @return Return the gralloc buffer handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CSLBufferInfo* GetCSLBufferInfo()
    {
        if ((FALSE == IsInValidUsageState()) || (FALSE == m_isBacked))
        {
            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Not in valid state, m_isBacked=%d",
                           GetBufferManagerName(), this, m_isBacked);
        }

        return &m_bufferInfo;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferManagerName
    ///
    /// @brief  Get parent Buffer manager name
    ///
    /// @return Return the name
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CHAR* GetBufferManagerName() const
    {
        return m_pBufferManagerName;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsLateBindingEnabled
    ///
    /// @brief  Whether late binding of buffer to this ImageBuffer object is enabled.
    ///
    /// @return TRUE if enabled, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsLateBindingEnabled() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BindImportedBuffer
    ///
    /// @brief  Call Bind on ImageBuffer that was being imported into this ImageBuffer and then import buffer information
    ///         in to this ImageBuffer object
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult BindImportedBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapBufferToAdditionalDevices
    ///
    /// @brief  Map the buffer that this ImageBuffer is associated with to additional devices or cpu
    ///
    /// @param  offset          Offset relative to the memory described by fd.
    /// @param  size            Size of the memory described by FD.
    /// @param  flags           Usage flags for the memory to be mapped for this image buffer.
    /// @param  pDeviceIndices  List of device indices that may access this buffer.
    /// @param  deviceCount     The number of device indices pointed to by pDeviceIndices.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult MapBufferToAdditionalDevices(
        SIZE_T          offset,
        SIZE_T          size,
        UINT32          flags,
        const INT32*    pDeviceIndices,
        UINT            deviceCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBusyState
    ///
    /// @brief  Set the state whether the ImageBuffer object is currently in Busy list or free list
    ///
    /// @param  bIsInBusyList   Flag indicating the state of this ImageBuffer object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetBusyState(
        BOOL bIsInBusyList)
    {
        m_bIsInBusyList = bIsInBusyList;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsInValidUsageState
    ///
    /// @brief  Check whether this ImageBuffer is in valid state for clients to access it
    ///
    /// @return TRUE if valid, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsInValidUsageState() const
    {
        BOOL bInValidState = TRUE;

        if (FALSE == m_bIsInBusyList)
        {
            // Client has released the buffer to free list, so clienats are not supposed to access any information
            // from this ImageBuffer object.

            CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s][%p] Client trying to use after releasing the ImageBuffer",
                           GetBufferManagerName(), this);

            // Dont make it fatal error until we fix all clients' illegal access cases. Just print a warning for now.
            if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->validateImageBufferUsageState)
            {
                bInValidState = FALSE;
            }
        }

        return bInValidState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpState
    ///
    /// @brief  Dumps snapshot of current state to a file
    ///
    /// @param  fd      file descriptor
    /// @param  indent  indent spaces.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpState(
        INT     fd,
        UINT32  indent);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBatchImageFormat
    ///
    /// @brief  If the image is batch format, which can hold multiple frames
    ///
    /// @return Return TRUE if the image is batch format
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsBatchImageFormat() const
    {
        BOOL isBatch = FALSE;
        if (m_currentFormat.format == Format::UBWCNV12FLEX)
        {
            isBatch = TRUE;
        }
        return isBatch;
    }
private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImportGrallocBuffer
    ///
    /// @brief  Import previously allocated memory to be used with this memory buffer.
    ///
    /// @param  pCurrentFormat  The format of the buffer to import.
    /// @param  phNativeBuffer  The native handle of the buffer to import.
    /// @param  offset          Offset relative to the memory described by fd.
    /// @param  size            Size of the memory described by FD.
    /// @param  flags           Usage flags for the memory to be mapped for this image buffer.
    /// @param  pDeviceIndices  List of device indices that may access this buffer.
    /// @param  deviceCount     The number of device indices pointed to by pDeviceIndices.
    ///
    /// @return CamxResultSuccess if successful. If the previously allocated memory is not able to contain an image described
    ///         by pFormat, CamxResultEResource will be returned.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ImportGrallocBuffer(
        const ImageFormat*  pCurrentFormat,
        const NativeHandle* phNativeBuffer,
        SIZE_T              offset,
        SIZE_T              size,
        UINT32              flags,
        const INT32*        pDeviceIndices,
        UINT                deviceCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImportChiNativeBuffer
    ///
    /// @brief  Import previously allocated memory to be used with this memory buffer.
    ///
    /// @param  pCurrentFormat  The format of the buffer to import.
    /// @param  pImageBuffer    ImageBuffer object to import
    /// @param  offset          Offset relative to the memory described by fd.
    /// @param  size            Size of the memory described by FD.
    /// @param  flags           Usage flags for the memory to be mapped for this image buffer.
    /// @param  pDeviceIndices  List of device indices that may access this buffer.
    /// @param  deviceCount     The number of device indices pointed to by pDeviceIndices.
    ///
    /// @return CamxResultSuccess if successful. If the previously allocated memory is not able to contain an image described
    ///         by pFormat, CamxResultEResource will be returned.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ImportChiNativeBuffer(
        const ImageFormat*  pCurrentFormat,
        ImageBuffer*        pImageBuffer,
        SIZE_T              offset,
        SIZE_T              size,
        UINT32              flags,
        const INT32*        pDeviceIndices,
        UINT                deviceCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImportCSLBuffer
    ///
    /// @brief  Back this image buffer with a CSLBuffer.
    ///
    /// @param  pFormat     The format of the buffer to import.
    /// @param  pBufferInfo The CSL buffer information.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ImportCSLBuffer(
        const ImageFormat* pFormat,
        const CSLBufferInfo* pBufferInfo);

    ImageBuffer(const ImageBuffer& rOther)            = delete;
    ImageBuffer& operator=(const ImageBuffer& rOther) = delete;

    const CHAR*                     m_pBufferManagerName;                 ///< Client Buffer Manager name, client has to keep
                                                                          ///  this memory over the lifetime of this object
    const BufferManagerCreateData*  m_pCreateData;                        ///< Pointer to create data, client has to keep
                                                                          ///< this memory over the lifetime of this object
    BOOL                            m_isBacked;                           ///< Track if the Image buffer is valid, having been
                                                                          ///< backed by memory.
    UINT                            m_numBatchedFrames;                   ///< Batch size of the allocated buffer.
    CSLBufferInfo                   m_bufferInfo;                         ///< CSL Buffer information having memhandle, fd, etc.
    SIZE_T                          m_planeSize[FormatsMaxPlanes];        ///< Per plane size.
    SIZE_T                          m_planeStartOffset[FormatsMaxPlanes]; ///< Per plane start offset in the allocated buffer.
    SIZE_T                          m_metadataSize[FormatsMaxPlanes];     ///< Metadata Size
    ImageFormat                     m_currentFormat;                      ///< The buffer import format. This is used to import
                                                                          ///  csl buffer properties. This can change over the
                                                                          ///  lifetime of this object, but the required size
                                                                          ///  based on this is always less than or equal to
                                                                          ///  size based on format passed while Initialize.
                                                                          ///  Format get APIs are based on this.
    BOOL                            m_bValidFormat;                       ///< Whether m_currentFormat has valid information
    volatile UINT                   m_aReferenceCount;                    ///< The reference count.
    ImageBufferManager*             m_pImageBufferManager;                ///< Buffer manager that allocated this buffer
    MemPoolBufMgrHandle             m_hMemPoolBufMgrHandle;               ///< Memory Pool Manager handle using which this
                                                                          ///  ImageBuffer object can get a buffer
    MemPoolBufferHandle             m_hMemPoolBufferHandle;               ///< Buffer handle that is acquired from
                                                                          ///  MemPool Manager
    const NativeHandle*             m_phNativeBuffer;                     ///< Handle to the imported native buffer
    BufferHandle                    m_hGrallocBuffer;                     ///< Gralloc buffer handle
    ImageBufferImportData           m_importData;                         ///< Import information if this ImageBuffer object
                                                                          ///  is importing a ChiNative Buffer, i.e importing
                                                                          ///  a different ImageBuffer object
    BOOL                            m_bIsInBusyList;                      ///< Whether this ImageBuffer object is in busy list
                                                                          ///  of corresponding ImageBufferManager
};

CAMX_NAMESPACE_END

#endif // CAMXIMAGEBUFFER_H
