////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camximagebuffermanager.h
///
/// @brief Camera image buffer manager definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXIMAGEBUFFERMANAGER_H
#define CAMXIMAGEBUFFERMANAGER_H

#include "camximagebuffer.h"
#include "camxlist.h"
#include "camxosutils.h"
#include "camxthreadmanager.h"
#include "camxtypes.h"
#include "camxmempoolmgr.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImageBufferManager;
class ImageBuffer;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief  Buffer manager types. This enum describes whether it is Camx internal or Chi buffer manager.
enum class BufferManagerType
{
    CamxBufferManager,  ///< Buffer manager that's created internally in camx driver
    ChiBufferManager,   ///< Buffer manager that's created in chi usecases through bufferManagerOps
};

/// @brief Bufferproperties structure that is initialized by looking at the corresponding XML structure
struct BufferProperties
{
    ImageFormat   imageFormat;                      ///< Buffer format info
    UINT          immediateAllocImageBuffers;       ///< Initial number of image buffers to be allocated
    UINT          maxImageBuffers;                  ///< Maximum number of image buffers can be allocated
    UINT          memFlags;                         ///< Memory flags
    CSLBufferHeap bufferHeap;                       ///< Buffer heap
    UINT64        producerFlags;                    ///< Buffer producer gralloc flags
    UINT64        consumerFlags;                    ///< Buffer consumer gralloc flags
    UINT32        grallocFormat;                    ///< Buffer gralloc format
                                                    ///  This property is not needed if buffer will be allocated through CSL
};

/// @brief LinkBufferManagerProperties structure that is populated by looking at the pipeline information
struct LinkBufferManagerProperties
{
    VOID*   pNode;                          ///< Node to which the Buffer Manager belongs to
    BOOL    isFromIFENode;                  ///< Whether this link Buffer Manager is from IFE node
    BOOL    isPartOfRealTimePipeline;       ///< Whether this link Buffer Manager is part of a real time pipeline
    BOOL    isPartOfPreviewVideoPipeline;   ///< Whether this link Buffer Manager is part of a real time pipeline
    BOOL    isPartOfSnapshotPipeline;       ///< Whether this link Buffer Manager is part of a snapshot pipeline
};

/// @brief Buffer manager create data.
struct BufferManagerCreateData
{
    BufferProperties            bufferProperties;                  ///< Buffer properties
    INT32                       deviceIndices[CamxMaxDeviceIndex]; ///< Pointer to array of device indices that may
                                                                   ///  access this buffer
    UINT                        deviceCount;                       ///< The number of valid entries in the pDeviceIndices array
    BOOL                        allocateBufferMemory;              ///< Boolean indicating whether buffer memory
                                                                   ///  should be allocated
    UINT                        numBatchedFrames;                  ///< Greater than 1 for a link that has batch mode enabled,
                                                                   ///  otherwise it MUST be set to 1
    UINT                        immediateAllocBufferCount;         ///< Number of buffers to be allocated immediately
    UINT                        maxBufferCount;                    ///< Maximum number of buffers to be allocated
    BOOL                        bNeedDedicatedBuffers;             ///< Whether this Buffer Manager needs dedicated buffers
    BOOL                        bEnableLateBinding;                ///< Whether to enable late binding of backing buffers to
                                                                   ///  ImageBuffer holder. Used only when allocateBufferMemory
                                                                   ///  is TRUE
    BOOL                        bDisableSelfShrinking;             ///< Whether to explicitly disable self shrinking for the
                                                                   ///  buffers allocated for this ImageBufferManager
    BufferManagerType           bufferManagerType;                 ///< The type indicating whether it is camx or chi
                                                                   ///  buffer manager
    LinkBufferManagerProperties linkProperties;                    ///< Link properties
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The ImageBufferManager manages a pool of homogeneous frame buffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImageBufferManager
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create a new image buffer manager.
    ///
    /// @param  pBufferManagerName   Name of the image buffer manager.
    /// @param  pCreateData          Buffer Manager create data
    /// @param  ppImageBufferManager Newly created image buffer manager.
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        const CHAR*              pBufferManagerName,
        BufferManagerCreateData* pCreateData,
        ImageBufferManager**     ppImageBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy this image buffer manager.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeBuffers
    ///
    /// @brief  Initialize image buffers for a given format and queue in the buffer manager.
    ///
    /// @param  pCreateData          Buffer Manager create data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeBuffers(
        BufferManagerCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetImageBuffer
    ///
    /// @brief  Retrieve an available buffer from the buffer pool. Buffers are retrieved in FIFO order.
    ///
    /// @return Pointer to an ImageBuffer. If buffers are not allocated, not available, or some other error, NULL is returned.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ImageBuffer* GetImageBuffer();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxImageBufferCount
    ///
    /// @brief  Retrieve the maximum number of buffers to be allocated passed in the create data.
    ///
    /// @return the maximum number of buffers allowed to be created.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetMaxImageBufferCount() {
        return m_maxBufferCount;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Activate
    ///
    /// @brief  Activate this buffer manager. This gives a hint to manager underlying Image Buffer allocations
    ///
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Activate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Deactivate
    ///
    /// @brief  Deactivate this Buffer Manager. This releases all or some buffers allocated by the buffer manager.
    ///
    /// @param  isPartialRelease   If TRUE, release buffers until m_immediateAllocBufferCount is reached; else release all
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Deactivate(
        BOOL isPartialRelease);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddReference
    ///
    /// @brief  Add reference to the image buffer
    ///
    /// @param  pImageBuffer   Image buffer object
    ///
    /// @return The reference count of the ImageBuffer after adding reference
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT AddReference(
        ImageBuffer* pImageBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseReference
    ///
    /// @brief  Release reference to the image buffer
    ///
    /// @param  pImageBuffer   Image buffer object
    ///
    /// @return The reference count of the ImageBuffer after releasing
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT ReleaseReference(
        ImageBuffer* pImageBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BindBufferManagerImageBuffer
    ///
    /// @brief  Bind backing buffer to the image buffer
    ///
    /// @param  pImageBuffer   Image buffer object
    ///
    /// @return CamxResultSuccess if succeeded
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult BindBufferManagerImageBuffer(
        ImageBuffer* pImageBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferManagerName
    ///
    /// @brief  Return the buffer manager's name
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHAR* GetBufferManagerName();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferManagerType
    ///
    /// @brief  Return the buffer manager's type
    ///
    /// @return BufferManagerType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BufferManagerType GetBufferManagerType()
    {
        return m_createData.bufferManagerType;
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
private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImageBufferManager
    ///
    /// @brief  Create a new buffer manager instance. The buffer manager does not contain any buffers when initialized.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ImageBufferManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ImageBufferManager
    ///
    /// @brief  The destructor. Destructing a buffer manager with buffers will be release them. The client should take care to
    ///         ensure there are no outstanding references to buffers when the buffer manager is destroyed.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~ImageBufferManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize the buffer manager. This method must be called before any other methods after construction.
    ///
    /// @param  pBufferManagerName   Name of the image buffer manager.
    /// @param  pCreateData          Buffer Manager create data
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        const CHAR*                 pBufferManagerName,
        BufferManagerCreateData*    pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBuffersListPartiallyCleared
    ///
    /// @brief  Check if the partial buffer release threshold is met or not
    ///
    /// @param  isPartialRelease   If TRUE, to check for free and busy buffers list count <= m_immediateAllocBufferCount
    ///
    /// @return TRUE if list is partially cleared else FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsBuffersListPartiallyCleared(
        BOOL isPartialRelease);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeBufferFromList
    ///
    /// @brief  Free all image buffers (if any) including backing memory.
    ///
    /// @param  pBufferList            list from which buffer is to be released
    /// @param  isForced               if TRUE, force release the image buffer
    /// @param  pBufferListDescription suitable description of the list (e.g., FreeList, BusyList etc.)
    ///
    /// @return TRUE if list is empty else FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL FreeBufferFromList(
        LightweightDoublyLinkedList* pBufferList,
        BOOL                         isForced,
        const CHAR*                  pBufferListDescription);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearFreeListBuffers
    ///
    /// @brief  Free all image buffers (if any) including backing memory.
    ///
    /// @param  isPartialRelease   If TRUE, release buffers until m_immediateAllocBufferCount is reached; else release all
    /// @param  isForced           if TRUE, force release the image buffers
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID ClearFreeListBuffers(
        BOOL isPartialRelease,
        BOOL isForced);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearBusyListBuffers
    ///
    /// @brief  Free all image buffers (if any) including backing memory.
    ///
    /// @param  isPartialRelease   If TRUE, release buffers until m_immediateAllocBufferCount is reached; else release all
    /// @param  isForced           if TRUE, force release the image buffers
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID ClearBusyListBuffers(
        BOOL isPartialRelease,
        BOOL isForced);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeBuffers
    ///
    /// @brief  Free all image buffers (if any) including backing memory.
    ///
    /// @param  isPartialRelease   If TRUE, release buffers until m_immediateAllocBufferCount is reached; else release all
    /// @param  isForced           if TRUE, force release the image buffers
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID FreeBuffers(
        BOOL isPartialRelease,
        BOOL isForced);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeResources
    ///
    /// @brief  Free all internal resources allocated by the ImageBufferManager.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FreeResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateImageFormat
    ///
    /// @brief  Update ImageFormat properties to import csl buffer information
    ///
    /// @param  pFormat   Pointer to ImageFormat to update
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateImageFormat(
        const ImageFormat*  pFormat);

    ImageBufferManager(const ImageBufferManager& rOther)            = delete;
    ImageBufferManager& operator=(const ImageBufferManager& rOther) = delete;

    CHAR           m_pBufferManagerName[MaxStringLength256]; ///< Name of the buffer manager
    UINT           m_immediateAllocBufferCount;              ///< Initial number of buffers to be allocated
    UINT           m_maxBufferCount;                         ///< Maximum number of buffers to be allocated
    Mutex*         m_pLock;                                  ///< Mutex to protect buffer manager internal state.
    Condition*     m_pWaitFreeBuffer;                        ///< Wait till free buffers are available in the buffer manager

    BufferManagerCreateData      m_createData;               ///< Buffer Manager CreateData.
    ImageFormat                  m_currentFormat;            ///< Pointer to the image format using which buffer information has
                                                             ///  to be populated. In normal cases, this is same as
                                                             ///  imageFormat in createData, unless some clients want to
                                                             ///  allocate using a format and use the same buffer with
                                                             ///  different format properties.
    LightweightDoublyLinkedList  m_freeBufferList;           ///< Free list of buffers which are available for use at any time.
    LightweightDoublyLinkedList  m_busyBufferList;           ///< List of buffers given to the client.
    MemPoolBufMgrHandle          m_hMemPoolBufMgrHandle;     ///< Memory Pool handle for this Buffer manager
    UINT                         m_peakBufferHolders;        ///< Peak number of ImageBuffer holders used by this Buffer Manager
};

CAMX_NAMESPACE_END

#endif // CAMXIMAGEBUFFERMANAGER_H
