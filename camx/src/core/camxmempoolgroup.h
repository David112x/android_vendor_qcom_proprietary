////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxmempoolgroup.h
///
/// @brief CamX Memory Pool Group declarations. Memory Pool Group is used to manage and share buffers to ImageBufferManagers
///        having similar Buffer requirement.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXMEMPOOLGROUP_H
#define CAMXMEMPOOLGROUP_H

#include "camxmempoolmgr.h"
#include "camximagebuffermanager.h"

CAMX_NAMESPACE_BEGIN

static const UINT MaxIncrBufMappings = 4;    ///< Maximum number of incremental mappings allowed for a given buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MemPoolMgr;
class MemPoolGroup;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief struct defining Memory Pool Manager statistics
struct MemPoolManagerStats
{
    UINT    numBuffersAllocated;                ///< Number of allocations at any given point of time
    UINT    peakNumBuffersAllocated;            ///< Peak number of allocations
    SIZE_T  sizeOfMemoryAllocated;              ///< Size of Memory allocated at any given point of time
    SIZE_T  peakSizeOfMemoryAllocated;          ///< Peak size of Memory allocated

    UINT    numBuffersMappedToCPU;              ///< Number of maps at any given point of time
    UINT    peakNumBuffersMappedToCPU;          ///< Peak number of maps
    SIZE_T  sizeOfMemoryMappedToCPU;            ///< Size of Memory mapped at any given point of time
    SIZE_T  peakSizeOfMemoryMappedToCPU;        ///< Peak size of Memory mapped

    UINT    numBuffersMappedToDevices;          ///< Number of maps at any given point of time
    UINT    peakNumBuffersMappedToDevices;      ///< Peak number of maps
    SIZE_T  sizeOfMemoryMappedToDevices;        ///< Size of Memory mapped at any given point of time
    SIZE_T  peakSizeOfMemoryMappedToDevices;    ///< Peak size of Memory mapped
};

/// @brief struct defining Memory Pool Buffer manager properties
struct MemPoolBufferManager
{
    CHAR                        name[MaxStringLength256];   ///< Name of the Buffer Manager
    MemPoolGroup*               pMemPoolGroup;              ///< Memory Pool Group to which this Buffer Manager is attached
    LDLLNode*                   pNode;                      ///< Node containing this MemPoolBuffer Manager object
    BufferManagerCreateData     createData;                 ///< Client Buffer Manager's create data
    BOOL                        bActivated;                 ///< Whether this Buffer manager is in active state
    BOOL                        bEverActivated;             ///< Whether this Buffer manager is ever activated in its lifetime
    UINT                        peakBuffersUsed;            ///< Peak number of buffers used by this Buffer Manager
    SIZE_T                      sizeRequired;               ///< Exact Buffer size required by this Buffer Manager
    LightweightDoublyLinkedList memPoolBufferList;          ///< List of Buffers acquired by this Buffer Manager
};

/// @brief Struct defining Memory Pool Buffer properties
struct MemPoolBuffer
{
    CHAR                    name[MaxStringLength64];            ///< Name of this buffer. Set while allocation
    MemPoolGroup*           pMemPoolGroup;                      ///< Group to which this buffer belongs to.
    LDLLNode*               pNode;                              ///< Node container of this buffer. Node part of List
    MemPoolBufferManager*   pMemPoolBufMgr;                     ///< Buffer Manager to which this buffer is given.
                                                                ///  Valid only if this buffer is acquired by a Buffer Manager,
                                                                ///  NULL otherwise.
    CSLBufferHeap           bufferHeap;                         ///< Buffer heap using which this buffer is allocated.
    BufferHandle            hGrallocBuffer;                     ///< Gralloc buffer handle, only valid if Memory is
                                                                ///  allocated using gralloc
    CSLBufferInfo           bufferInfo;                         ///< CSL Buffer information.
    CSLBufferInfo           incrBufferInfo[MaxIncrBufMappings]; ///< CSL Buffer information for incremental mappings
    UINT                    numIncrMappings;                    ///< Number of times this buffer is incrementally mapped
    INT32                   deviceIndices[CamxMaxDeviceIndex];  ///< Array of device indices that this buffer is currently
                                                                ///  mapped to.
    UINT                    deviceCount;                        ///< Number of valid entries in deviceIndices
    UINT32                  allocatedStride;                    ///< Stride allocated by Gralloc
    SIZE_T                  allocatedSize;                      ///< Size of the buffer allocated
    BOOL                    bCurrentlyCached;                   ///< Whether this buffer is curently cached.
};

/// @brief Struct defining consolidate buffer allocation properties within a Memory Pool Group
struct MemPoolBufferAllocProperties
{
    BOOL            valid;                              ///< Whether this has valid information
    CSLBufferHeap   bufferHeap;                         ///< Buffer heap using which memory needs to be allocated
    SIZE_T          totalBufferSizeBytes;               ///< Size of buffer to allocate
    SIZE_T          alignment;                          ///< The required alignment in bytes of the starting address
    UINT            flags;                              ///< Memory flags
    INT32           deviceIndices[CamxMaxDeviceIndex];  ///< List of devices to which this buffer need to be mapped
    UINT            deviceCount;                        ///< Number of devices
    UINT32          planeStride;                        ///< Stride of plane, needed if allocating from Gralloc
    UINT32          sliceHeight;                        ///< Height of plane, needed if allocating from Gralloc
    CamX::Format    format;                             ///< Formta of the buffer, needed if allocating from Gralloc
    UINT64          producerUsageFlags;                 ///< Producer usage flags, needed if allocating from Gralloc
    UINT64          consumerUsageFlags;                 ///< Consumer usage flags, needed if allocating from Gralloc
    UINT32          grallocFormat;                      ///< Buffer gralloc format
                                                        ///  This property is not needed if buffer will be allocated through CSL
};

/// @brief Struct defining consolidate link properties that are attached to this Memory Pool Group
struct MemPoolGroupLinkProperties
{
    BOOL    isFromIFENode;                  ///< Whether this Group has a Buffer Manager from IFE node
    BOOL    isPartOfRealTimePipeline;       ///< Whether this Group has a Buffer Manager that is part of a real time pipeline
    BOOL    isPartOfPreviewVideoPipeline;   ///< Whether this Group has a Buffer Manager that is part of a preview pipeline
    BOOL    isPartOfSnapshotPipeline;       ///< Whether this Group has a Buffer Manager that is part of a snapshot pipeline
};

/// @brief struct describing buffer count info
struct MemPoolGroupCount
{
    UINT    min;    ///< Min value among Buffer Manager's buffer count
    UINT    max;    ///< Max value among Buffer Manager's buffer count
    UINT    sum;    ///< Sum of Buffer Manager's buffer count
};

/// @brief struct describing buffer count info
struct MemPoolGroupBufferCount
{
    MemPoolGroupCount   immediateAllocBufferCount;  ///< Immediate alloc buffer count info : min, max, sum
    MemPoolGroupCount   maxBufferCount;             ///< Max buffer count info : min, max, sum
};

/// @brief structure containing the result of GetBufferPool which contains the result and buffer handle
struct MemPoolGetBufferResult
{
    CamxResult          result;         ///< Result of GetBufferPool
    MemPoolBufferHandle hBufferHandle;  ///< Buffer handle returned by GetBufferPool
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Representation of an image buffer including planes and format definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MemPoolGroup
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MemPoolGroup
    ///
    /// @brief  Constructor for MemPoolGroup.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit MemPoolGroup();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~MemPoolGroup
    ///
    /// @brief  Destructor. Will release any memory allocated and any resources.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~MemPoolGroup();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize function for MemPoolGroup class.
    ///
    /// @param  groupCounter            Group Counter for name
    /// @param  pMemPoolMgr             Pointer to singleton Memory Pool Manager object
    /// @param  bufferHeap              Buffer heap
    /// @param  bDedicatedGroup         Whether this group is dedicated for a single Buffer Manager, i.e no sharing
    /// @param  bDisableSelfShrinking   Whether to explicitly disable self shrinking of buffers allocated in this group
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        UINT32          groupCounter,
        MemPoolMgr*     pMemPoolMgr,
        CSLBufferHeap   bufferHeap,
        BOOL            bDedicatedGroup,
        BOOL            bDisableSelfShrinking);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMemPoolGroupName
    ///
    /// @brief  Get this Memory Pool group name
    ///
    /// @return Name of this group.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CHAR* GetMemPoolGroupName() const
    {
        return m_pGroupName;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMemPoolBufMgrName
    ///
    /// @brief  Get the Memory Pool Buffer Manager name
    ///
    /// @param  pMemPoolBufMgr  Pointer to MemPoolBufferManager corresponding to client Buffer Manager
    ///
    /// @return Name of the MemPoolBufMgr.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CHAR* GetMemPoolBufMgrName(
        MemPoolBufferManager* pMemPoolBufMgr) const
    {
        CAMX_ASSERT(NULL != pMemPoolBufMgr);

        return pMemPoolBufMgr->name;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterBufferManager
    ///
    /// @brief  This function first checks if this Memory Pool Group can be attached to the incoming Buffer Manager by calling
    ///         IsMatchingGroup. If the Group matches, then the incoming Buffer Manager will be grouped into this MemPoolGroup.
    ///         If this Group already have Buffer Managers attached, it means the incoming Buffer Manager along with the
    ///         previously attached Buffer Managers shares buffers allocated in this Group. The incoming Buffer Manager will
    ///         NOT be attached if the Group doesn't match. This function returns matching group status and unique valid
    ///         MemPoolBufferManager handle.
    ///
    /// @param  pBufferManagerName  Name of Buffer manager to attach.
    /// @param  pCreateData         Buffer Manager create data.
    /// @param  pIsMatchingResult   Variable pointing to matching group status.
    ///
    /// @return On successful registration, returns valid MemPoolBufferManager handle and CamxResultSuccess as matching status.
    ///         On registartion failure, returns NULL MemPoolBufferManager handle and CamxResultSuccess as matching status.
    ///         On matching failure, returns NULL MemPoolBufferManager handle and CamxResultENoSuch as matching status.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MemPoolBufferManager* RegisterBufferManager(
        const CHAR*                     pBufferManagerName,
        const BufferManagerCreateData*  pCreateData,
        CamxResult*                     pIsMatchingResult);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnregisterBufferManager
    ///
    /// @brief  Unregisters the Buffer Manager from current group.
    ///
    /// @param  pMemPoolBufMgr  Pointer to MemPoolBufferManager corresponding to client Buffer Manager
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UnregisterBufferManager(
        MemPoolBufferManager* pMemPoolBufMgr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ActivateBufferManager
    ///
    /// @brief  This function activates the client buffer manager. This helps as a hint to MemPoolGroup as when the buffers
    ///         being used by client Buffer manager. Memory Pool Group may allocate minimum required buffers in this function
    ///         if not allocated by now (depending upon camxsettings). If this function is not called by client, the first call
    ///         to GetBufferFromPool is treated as Activate for that particular MemPoolBufferManager.
    ///
    /// @param  pMemPoolBufMgr  Pointer to MemPoolBufferManager corresponding to client Buffer Manager
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ActivateBufferManager(
        MemPoolBufferManager* pMemPoolBufMgr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeactivateBufferManager
    ///
    /// @brief  This function deactivates the client Buffer Manager. Memory Pool Group may free any buffers allocated for
    ///         for this client if the buffers are not required to share with other Buffer managers.
    ///
    /// @param  pMemPoolBufMgr  Pointer to MemPoolBufferManager corresponding to client Buffer Manager
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DeactivateBufferManager(
        MemPoolBufferManager* pMemPoolBufMgr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferFromPool
    ///
    /// @brief  This function is used to get a buffer from this Memory Pool Group. This will make sure the buffer returned is
    ///         mapped to the required devices listed in createData while Register. Once a buffer is given to a client
    ///         buffer manager it will not be used/shared with any other buffer managers until released back to Memory Pool
    ///         Manager using ReleaseBufferToPool API. If there is no buffer available in free list (allocated already but not
    ///         in use) by now, Memory Pool Group allocates a new buffer and returns.
    ///
    /// @param  pMemPoolBufMgr      Pointer to MemPoolBufferManager corresponding to client Buffer Manager
    /// @param  pCSLBufferInfo      Pointer to CSLBufferInfo structure to fill information.
    /// @param  phGrallocBuffer     Pointer to fill Gralloc Buffer handle.
    ///
    /// @return MemPoolBufferHandle and CamxResultSuccess if successful, failure otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MemPoolGetBufferResult GetBufferFromPool(
        MemPoolBufferManager* pMemPoolBufMgr,
        CSLBufferInfo*        pCSLBufferInfo,
        BufferHandle*         phGrallocBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseBufferToPool
    ///
    /// @brief  This function is used to release a previously acquired buffer using GetBufferFromPool back to Memory Pool Group.
    ///         Client Buffer Manager has to release all buffers that were acquired before calling DeactivateBufferManager or
    ///         UnregisterBufferManager.
    ///
    /// @param  pMemPoolBufMgr          Pointer to MemPoolBufferManager corresponding to client Buffer Manager
    /// @param  hMemPoolBufferHandle    Unique handle representing the Memory Pool Buffer.
    /// @param  bufferCached            Whether memory is cached by the time it is released to Memory Pool Manager.
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseBufferToPool(
        MemPoolBufferManager*   pMemPoolBufMgr,
        MemPoolBufferHandle     hMemPoolBufferHandle,
        BOOL                    bufferCached);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapBufferToDevices
    ///
    /// @brief  This function maps the Mem Pool Buffer to additional devices/umd.
    ///
    /// @param  pMemPoolBufMgr          Pointer to MemPoolBufferManager corresponding to client Buffer Manager
    /// @param  hMemPoolBufferHandle    Unique handle representing the Mem Pool Buffer.
    /// @param  offset                  Offset relative to the memory described by hMemPoolBufferHandle.
    /// @param  size                    Size of the memory described by hMemPoolBufferHandle.
    /// @param  flags                   Memory flags
    /// @param  pDeviceIndices          List of devices to which this buffer need to be mapped.
    /// @param  deviceCount             Number of devices valid in pDeviceIndices.
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult MapBufferToDevices(
        MemPoolBufferManager*   pMemPoolBufMgr,
        MemPoolBufferHandle     hMemPoolBufferHandle,
        SIZE_T                  offset,
        SIZE_T                  size,
        UINT32                  flags,
        const INT32*            pDeviceIndices,
        UINT                    deviceCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferInfo
    ///
    /// @brief  This function gets the Buffer information of input MemPoolBufferHandle
    ///
    /// @param  pMemPoolBufMgr          Pointer to MemPoolBufferManager corresponding to client Buffer Manager
    /// @param  hMemPoolBufferHandle    Unique handle representing the Memory Pool Buffer.
    /// @param  pCSLBufferInfo          Pointer to CSLBufferInfo structure to fill information.
    /// @param  phGrallocBuffer         Pointer to fill Gralloc Buffer handle.
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetBufferInfo(
        MemPoolBufferManager*   pMemPoolBufMgr,
        MemPoolBufferHandle     hMemPoolBufferHandle,
        CSLBufferInfo*          pCSLBufferInfo,
        BufferHandle*           phGrallocBuffer
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CanDestroyMemPoolGroup
    ///
    /// @brief  This function determines whether this MemPoolGroup can be destroyed
    ///
    /// @return TRUE if can be destroyed.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CanDestroyMemPoolGroup() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintMemPoolGroupState
    ///
    /// @brief  This function prints the state of this Memory Pool Group
    ///
    /// @param  forceTrigger    Whether to force trigger printing state by default
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintMemPoolGroupState(
        BOOL    forceTrigger
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetMemoryPoolManagerStats
    ///
    /// @brief  Reset Memory Pool Manager statistics
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ResetMemoryPoolManagerStats();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintMemoryPoolManagerStats
    ///
    /// @brief  Print Memory Pool Manager statistics
    ///
    /// @param  forceTrigger    Whether to force trigger printing state by default
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID PrintMemoryPoolManagerStats(
        BOOL    forceTrigger);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeIdleBuffers
    ///
    /// @brief  Free idle buffers for this group
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID FreeIdleBuffers()
    {
        m_pLock->Lock();

        if (0 < m_numBuffersIdle)
        {
            UINT32 selfShrinkSizeLimit = GetStaticSettings()->MPMMonitorSelfShrinkSizeLimit;

            CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s] : m_numBuffersIdle=%d, BufferSize=%zu, sizeLimit=%d",
                             m_pGroupName, m_numBuffersIdle, m_bufferAllocProperties.totalBufferSizeBytes,
                             selfShrinkSizeLimit);

            // Print Memory Pool Buffer Manager statistics of this group.
            // It can be helpful to identify which buffer manager is allocating extra buffers
            // and tune immediateAllocCount accordingly.
            PrintMemPoolGroupState(FALSE);

            // Free idle buffers
            if ((FALSE == m_bDisableSelfShrinking) && (m_bufferAllocProperties.totalBufferSizeBytes > selfShrinkSizeLimit))
            {
                CAMX_LOG_INFO(CamxLogGroupMemMgr,
                              "MemPoolGroup[%s] : Self Timer freeing m_numBuffersIdle=%d, BufferSize=%zu, sizeLimit=%d",
                              m_pGroupName, m_numBuffersIdle, m_bufferAllocProperties.totalBufferSizeBytes,
                              selfShrinkSizeLimit);
                FreeBuffers(m_numBuffersIdle);
            }
        }

        // Reset the number of idle buffers
        m_numBuffersIdle = m_freeBufferList.NumNodes();

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolGroup[%s] : Starting IdleBuffer Counter for next time period=%d",
                         m_pGroupName, m_numBuffersIdle);

        m_pLock->Unlock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeAllFreeListBuffers
    ///
    /// @brief  Free all buffers in free list for this group
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID FreeAllFreeListBuffers()
    {
        m_pLock->Lock();
        // Only free non-IFE buffers for now
        if ((FALSE == m_linkProperties.isFromIFENode) && ( 0 < m_freeBufferList.NumNodes()))
        {
            // Print Memory Pool Buffer Manager statistics of this group.
            PrintMemPoolGroupState(FALSE);
            CAMX_LOG_CONFIG(CamxLogGroupMemMgr,
                            "MemPoolGroup[%s]: freeing all buffers in freeList, num free list nodes:%u, "
                            "total buffers allocated:%u num buffer managers:%u, total size of buffer:%zu",
                            m_pGroupName,
                            m_freeBufferList.NumNodes(),
                            m_numBuffersAllocated,
                            m_numActiveBufMgrs,
                            m_bufferAllocProperties.totalBufferSizeBytes);

            FreeBuffers(m_freeBufferList.NumNodes());

            // Reset the number of idle buffers
            m_numBuffersIdle = Utils::MinUINT(m_freeBufferList.NumNodes(), m_numBuffersIdle);
        }
        m_pLock->Unlock();
    }

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMatchingGroup
    ///
    /// @brief  Checks whether current Memory Pool Group matches input Buffer Manager requirements. If matched, incoming
    ///         Buffer Manager will be attached to this Group using RegisterBufferManager
    ///
    /// @param  pBufferManagerName  Buffer Manager name
    /// @param  pCreateData         Pointer to create data information of the incoming Buffer Manager
    ///
    /// @return TRUE if matches.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsMatchingGroup(
        const CHAR*                     pBufferManagerName,
        const BufferManagerCreateData*  pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupBufferAllocProperties
    ///
    /// @brief  This function iterates through all attached Buffer Managers in the current Group and sets up consolidated
    ///         Buffer allocation properties. This information is used to allocate any new buffers
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupBufferAllocProperties();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateBufferAllocProperties
    ///
    /// @brief  This function updates the consolidated Buffer allocation properties considering the input Buffer Manager's
    ///         requirements. This information is used to allocate any new buffers..
    ///
    /// @param  pMemPoolBufMgr  Pointer to MemPool Buffer Manager
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateBufferAllocProperties(
        MemPoolBufferManager* pMemPoolBufMgr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateBuffers
    ///
    /// @brief  Allocate new buffers in this group. Once allocated, new Buffer holder (MemPoolBuffer) is added to free list
    ///         so that they are available for client Buffer managers to acquire. The buffers are allocated using consolidated
    ///         Buffer Alloc properties that were setup using SetupBufferAllocProperties or UpdateBufferAllocProperties
    ///
    /// @param  numBuffersToAllocate    Number of buffers to allocate.
    ///
    /// @return Number of buffers allocated
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 AllocateBuffers(
        UINT32 numBuffersToAllocate);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeBuffers
    ///
    /// @brief  Free buffers that are in free list. This will free the memory.
    ///
    /// @param  numBuffersToFree    Number of buffers to free from free_list.
    ///
    /// @return Number of buffers succesfully freed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 FreeBuffers(
        UINT32 numBuffersToFree);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsBufMgrHoldingBuffers
    ///
    /// @brief  Is this Buffer Manager currently holding any Buffers
    ///
    /// @param  pMemPoolBufMgr    Pointer to Memory Pool Buffer manager
    ///
    /// @return TRUE if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsBufMgrHoldingBuffers(
        MemPoolBufferManager* pMemPoolBufMgr
    ) const
    {
        UINT32  numBuffersInUse = pMemPoolBufMgr->memPoolBufferList.NumNodes();

        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "MemPoolBufMgr[%s] : Number of Buffers in use %d",
                         pMemPoolBufMgr->name, numBuffersInUse);

        return (numBuffersInUse > 0) ? TRUE : FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintMemPoolBufMgrState
    ///
    /// @brief  Print state of input Memory Pool Buffer Manager
    ///
    /// @param  pMemPoolBufMgr  Pointer to MemPoolBufferManager
    /// @param  forceTrigger    Whether to force trigger printing state by default
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintMemPoolBufMgrState(
        MemPoolBufferManager*   pMemPoolBufMgr,
        BOOL                    forceTrigger
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintMemPoolBufferState
    ///
    /// @brief  Print state of input Memory Pool Buffer
    ///
    /// @param  pMemPoolBuffer  Pointer to Memory Pool Buffer
    /// @param  forceTrigger    Whether to force trigger printing state by default
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintMemPoolBufferState(
        MemPoolBuffer*  pMemPoolBuffer,
        BOOL            forceTrigger
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMemPoolBufMgrActivated
    ///
    /// @brief  Set Memory Pool Buffer Manager's state
    ///
    /// @param  pMemPoolBufMgr  Pointer to MemPoolBufferManager instance
    /// @param  bActivated      Flag to set
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetMemPoolBufMgrActivated(
        MemPoolBufferManager*   pMemPoolBufMgr,
        BOOL                    bActivated)
    {
        CAMX_ASSERT(NULL != pMemPoolBufMgr);

        if (pMemPoolBufMgr->bActivated != bActivated)
        {
            if (TRUE == bActivated)
            {
                m_numActiveBufMgrs++;

                CAMX_ASSERT(m_numActiveBufMgrs <= m_bufferManagerList.NumNodes());
            }
            else
            {
                CAMX_ASSERT(0 < m_numActiveBufMgrs);

                m_numActiveBufMgrs--;
            }

            pMemPoolBufMgr->bActivated = bActivated;
        }

        if (TRUE == bActivated)
        {
            // This is for debugging/self tuning purpose. Whether a Buffer Manager that was created was ever activated.
            // Note - If its activated, we will pre-allocate immediateCount buffers on Activate.
            // We can cross check with pMemPoolBufMgr->peakBuffersUsed to self tune.
            // i.e if bEverActivated is TRUE (i.e we might have allocated immediate count buffers), but
            // if pMemPoolBufMgr->peakBuffersUsed is 0 (i.e though the pipeline that has this link Buffer Manager is activated,
            // this port was never enabled), we might have allocated some buffers unnecessarily.
            // Exa - memcpy node in simple zsl use case. It will be activated since it is in preview pipeline, but will never
            // use buffers since corresponding target output is never enabled.
            pMemPoolBufMgr->bEverActivated = TRUE;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupMemPoolGroupBufferCounts
    ///
    /// @brief  Setup MemPool Group's Buffer count information by iterating through all Buffer Managers that are attached to
    ///         this group.
    ///
    /// @param  bRegisterBufferCount    Whether to setup consolidated registered counts considering all Registered
    ///                                 Buffer Managers
    /// @param  bActivatedBufferCount   Whether to setup consolidated active counts considering all Buffer Managers that are
    ///                                 currently active
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupMemPoolGroupBufferCounts(
        BOOL bRegisterBufferCount,
        BOOL bActivatedBufferCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMemPoolGroupBufferCounts
    ///
    /// @brief  Update MemPool Group's Buffer count information by considering input Buffer Managers
    ///
    /// @param  pMemPoolBufMgr          Pointer to input MemPool Buffer Manager
    /// @param  bRegisterBufferCount    Whether to setup consolidated registered counts considering all Registered
    ///                                 Buffer Managers
    /// @param  bActivatedBufferCount   Whether to setup consolidated active counts considering all Buffer Managers that are
    ///                                 currently active
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateMemPoolGroupBufferCounts(
        MemPoolBufferManager*   pMemPoolBufMgr,
        BOOL                    bRegisterBufferCount,
        BOOL                    bActivatedBufferCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupMemPoolGroupLinkProperties
    ///
    /// @brief  Setup MemPool Group's link properties information by iterating through all Buffer Managers that are attached to
    ///         this group.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupMemPoolGroupLinkProperties();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateMemPoolGroupLinkProperties
    ///
    /// @brief  Update MemPool Group's link properties information by considering input Buffer Managers
    ///
    /// @param  pMemPoolBufMgr          Pointer to input MemPool Buffer Manager
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateMemPoolGroupLinkProperties(
        MemPoolBufferManager*   pMemPoolBufMgr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DetermineBufferAllocationRequired
    ///
    /// @brief  Determine whether buffer allocation is required. Returns the number of buffers that are needed to be allocated.
    ///
    /// @param  pMemPoolBufMgr  Pointer to input MemPool Buffer Manager
    ///
    /// @return Number of buffers to allocate
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT DetermineBufferAllocationRequired(
        MemPoolBufferManager* pMemPoolBufMgr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DetermineBufferFreeRequired
    ///
    /// @brief  Determine whether buffers need to be freed.
    ///
    /// @param  pMemPoolBufMgr  Pointer to input MemPool Buffer Manager
    /// @param  bOnUnregister   Whehter determing free buffers on a MemPoolBufMgr unregister
    ///
    /// @return Number of buffers to free
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT DetermineBufferFreeRequired(
        MemPoolBufferManager*   pMemPoolBufMgr,
        BOOL                    bOnUnregister);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCompatibleHeap
    ///
    /// @brief  Check whether current Group's heap and incoming Buffer Manager's heap are compatible.
    ///         If matched, we might be able to attach the current Buffer Manager in this Group
    ///
    /// @param  bAlreadyAllocated   Whether this group already has some buffers allocated
    /// @param  groupHeap           Heap type currently configured for the group
    /// @param  bufMgrHeap          Heap type of current Buffer Manager
    ///
    /// @return TRUE if size are within range
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsCompatibleHeap(
        BOOL            bAlreadyAllocated,
        CSLBufferHeap   groupHeap,
        CSLBufferHeap   bufMgrHeap
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SizeWithinRange
    ///
    /// @brief  Check whether current Group's buffer size and incoming Buffer Manager's buffer size are matching.
    ///         If matched, we might be able to attach the current Buffer Manager in this Group
    ///
    /// @param  bAlreadyAllocated   Whether this group already has some buffers allocated
    /// @param  groupSize           Buffer size currently configured for this group
    /// @param  bufMgrSize          Buffer size of current Buffer Manager
    /// @param  minGroupSize        Size of smallest buffer in group
    ///
    /// @return TRUE if size are within range
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL SizeWithinRange(
        BOOL   bAlreadyAllocated,
        SIZE_T groupSize,
        SIZE_T bufMgrSize,
        SIZE_T minGroupSize
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AlignmentMatch
    ///
    /// @brief  Check whether current Group's alignment and incoming Buffer Manager's alignment are matching.
    ///         If matched, we might be able to attach the current Buffer Manager in this Group
    ///
    /// @param  bAlreadyAllocated   Whether this group already has some buffers allocated
    /// @param  groupAlignment      Buffer alignment currently configured for this group
    /// @param  bufMgrAlignment     Buffer alignment of this Buffer Manager
    ///
    /// @return TRUE if alignments are within range
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL AlignmentMatch(
        BOOL   bAlreadyAllocated,
        SIZE_T groupAlignment,
        SIZE_T bufMgrAlignment
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CSLMemFlagsMatch
    ///
    /// @brief  Check whether current Group's Mem flags and incoming Buffer Manager's Mem Flgas are matching.
    ///         If matched, we might be able to attach the current Buffer Manager in this Group
    ///
    /// @param  bAlreadyAllocated   Whether this group already has some buffers allocated
    /// @param  groupFlags          Mem flags currently configured for this group
    /// @param  bufMgrFlags         Mem flags of this Buffer Manager
    ///
    /// @return TRUE if compatible mem flags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CSLMemFlagsMatch(
        BOOL bAlreadyAllocated,
        UINT groupFlags,
        UINT bufMgrFlags
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GrallocMemFlagsMatch
    ///
    /// @brief  Check whether current Group's Gralloc Mem flags and incoming Buffer Manager's Gralloc Mem Flgas are matching.
    ///         If matched, we might be able to attach the current Buffer Manager in this Group
    ///
    /// @param  bAlreadyAllocated   Whether this group already has some buffers allocated
    /// @param  groupProducerFlags  Producer gralloc flags currently configured for this group
    /// @param  groupConsumerFlags  Consumer gralloc flags currently configured for this group
    /// @param  bufMgrProducerFlags Producer gralloc flags of this Buffer Manager
    /// @param  bufMgrConsumerFlags Consumer gralloc flags of this Buffer Manager
    ///
    /// @return TRUE if compatible mem flags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL GrallocMemFlagsMatch(
        BOOL    bAlreadyAllocated,
        UINT64  groupProducerFlags,
        UINT64  groupConsumerFlags,
        UINT64  bufMgrProducerFlags,
        UINT64  bufMgrConsumerFlags
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DevicesMatch
    ///
    /// @brief  Check whether current Group's devices and incoming Buffer Manager's devices are matching.
    ///         If matched, we might be able to attach the current Buffer Manager in this Group
    ///
    /// @param  bAlreadyAllocated       Whether this group already has some buffers allocated
    /// @param  pGroupDeviceIndices     List of devices currently configured for this group
    /// @param  groupDeviceCount        Number of devices in Group's device list
    /// @param  pBufMgrDeviceIndices    List of devices for this Buffer Manager
    /// @param  bufMgrDeviceCount       Number of devices in Buffer Manager's device list
    ///
    /// @return TRUE if compatible devices
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL DevicesMatch(
        BOOL            bAlreadyAllocated,
        INT32*          pGroupDeviceIndices,
        UINT            groupDeviceCount,
        const INT32*    pBufMgrDeviceIndices,
        UINT            bufMgrDeviceCount
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndMapBufferToDevices
    ///
    /// @brief  Map the current buffer to the list of devices. This function checks if there are any additional devices in
    ///         input device list that are not currently being mapped. If there are some devices, map the buffer to those
    ///         devices and update the buffer properties with the newly added devices.
    ///
    /// @param  pBuffer             Memory Pool Buffer that has to be mapped to devices
    /// @param  pDeviceIndices      List of devices that this buffer need to be mapped to, if not already mapped
    /// @param  deviceCountt        Number of devices to map
    /// @param  flags               Memory flags
    ///
    /// @return CamxResultSuccess if succesful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckAndMapBufferToDevices(
        MemPoolBuffer*  pBuffer,
        const INT32*    pDeviceIndices,
        UINT            deviceCountt,
        UINT            flags);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SelfTuneImmediateCount
    ///
    /// @brief  Self tune immediate count buffers for buffer managers. This is based on different properties that
    ///         Memory Pool Manager gets in Buffer Manager create data. For exa, Buffer Managers from IFE node will have fixed
    ///         number for immediate count, offline buffer managers have fixed number of immediate count.
    ///
    /// @param  pMemPoolBufMgr  Pointer to input MemPool Buffer Manager
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SelfTuneImmediateCount(
        MemPoolBufferManager*   pMemPoolBufMgr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticSettings
    ///
    /// @brief  Helper method to return the settings pointer.
    ///
    /// @return HwContext*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE static const StaticSettings* GetStaticSettings()
    {
        return HwEnvironment::GetInstance()->GetStaticSettings();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Private Data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    MemPoolGroup(const MemPoolGroup& rOther)            = delete;
    MemPoolGroup& operator=(const MemPoolGroup& rOther) = delete;

    CHAR                            m_pGroupName[MaxStringLength64];    ///< Name of the group
    MemPoolMgr*                     m_pMemPoolMgr;                      ///< Singleton Memory Pool manager handle
    Mutex*                          m_pLock;                            ///< Mutex to protect access from multiple threads
    BOOL                            m_bDedicatedGroup;                  ///< Whether this group is dedicated for a single
                                                                        ///  Buffer Manager, i.e no sharing
    BOOL                            m_bDisableSelfShrinking;            ///< Whether to explicitly disable self shrinking of
                                                                        ///  Buffers allocated in this group

    LightweightDoublyLinkedList     m_bufferManagerList;                ///< List of Buffer Managers attached to this group
    UINT32                          m_numActiveBufMgrs;                 ///< Number of active Buffer managers in this group
    CSLBufferHeap                   m_bufferHeap;                       ///< Buffer heap
    SIZE_T                          m_leastBufferSizeAllocated;         ///< Least size that a Buffer allocated in this group
    SIZE_T                          m_maxBufferSizeAllocated;           ///< Max size that a Buffer allocated in this group
    MemPoolBufferAllocProperties    m_bufferAllocProperties;            ///< Consolidated Buffer allocation properties for
                                                                        ///  this group
    MemPoolGroupBufferCount         m_registeredBufferCount;            ///< Consolidated buffer alloc counts based on
                                                                        ///  all Registered Buffer Managers
    MemPoolGroupBufferCount         m_activatedBufferCount;             ///< Consolidated buffer alloc count properties based on
                                                                        ///  all Activated Buffer Managers
    MemPoolGroupLinkProperties      m_linkProperties;                   ///< Consolidated link properties for this group
    UINT32                          m_bufferNameCounter;                ///< Counter to assign name to the buffers
    UINT                            m_numBuffersAllocated;              ///< Number of buffers currently allocated in this group
    UINT                            m_peakNumBuffersAllocated;          ///< Peak number of buffers allocated in this group
    UINT                            m_peakNumBuffersUsed;               ///< Peak number of buffers used in this group
    UINT                            m_numBuffersIdle;                   ///< Number of buffers in idle state over the last
                                                                        ///  monitor period
    LightweightDoublyLinkedList     m_freeBufferList;                   ///< Free list of buffers which are available for use

    static Mutex*                   s_pMPGStatsLock;                    ///< Mutex to protect accessing Mem Pool Manager stats
    static MemPoolManagerStats      s_memPoolMgrStats;                  ///< Memory Pool Manager statistics.
                                                                        ///  This is a static variable, so it tracks overall
                                                                        ///  allocation statistics in all groups.
};


CAMX_NAMESPACE_END

#endif // CAMXMEMPOOLGROUP_H
