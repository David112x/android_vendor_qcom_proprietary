////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxmempoolmgr.h
///
/// @brief CamX MemPoolMgr declarations. MemPoolMgr is used to maintain buffers.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXMEMPOOLMGR_H
#define CAMXMEMPOOLMGR_H

#include "camxincs.h"
#include "camxformats.h"
#include "camxcsl.h"
#include "camxlist.h"
#include "camxosutils.h"
#include "camxthreadcommon.h"
#include "camxthreadmanager.h"
#include "camxhwenvironment.h"
#include "camxsettingsmanager.h"
#include "camximageformatutils.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MemPoolMgr;
class MemPoolGroup;

struct MemPoolBufferManager;
struct BufferProperties;
struct BufferManagerCreateData;
struct MemPoolGetBufferResult;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef VOID* MemPoolBufMgrHandle;  ///< Unique MemPoolBufMgr handle associated with each client Buffer Manager
typedef VOID* MemPoolBufferHandle;  ///< Unique handle for Memory Pool Buffer

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The MemPoolMgr class is used to manage memory allocations.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MemPoolMgr
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Public Methods
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // @brief the current state of the memory manager in regards to freeing buffers from any given memory
    //        pool group at a given time. We should only allow freeing or allocating of buffers if freeing
    //        is not in progress. We modify these states on CSLAlloc failure to determine if we can retry
    //        allocating with or without clearing all memory pool groups' free lists
    enum FreeingPoolState
    {
        FreeingNotInProgress,       ///< No freeing any memory pool groups
        FreeingInProgress,          ///< Currently freeing memory pool groups
        FreeingComplete             ///< Freeing of memory pool groups is complete
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSupportedBufferHeap
    ///
    /// @brief  Util function which says whether the given buffer heap type is supported by MemPoolMgr
    ///
    /// @param  bufferHeap  Buffer heap type.
    ///
    /// @return TRUE if supported, FALSE otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IsSupportedBufferHeap(
        CSLBufferHeap bufferHeap);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsValidBufferConfiguration
    ///
    /// @brief  Util function which says whether the buffer properties is valid
    ///
    /// @param  pCreateData  Buffer creation data.
    ///
    /// @return TRUE if valid, FALSE otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static BOOL IsValidBufferConfiguration(
        const BufferManagerCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterBufferManager
    ///
    /// @brief  This function registers a clients Buffer Manager with Memory Pool Manager. Memory Pool Manager works based on
    ///         client Buffer Manager requirements. i.e any client entity that wants buffers, first needs to register with
    ///         MemPoolMgr using this API by passing its buffer requirements. Any further calls, such as GetBufferFromPool
    ///         are based on the handle returned from this function. All buffers that are allocated for and returned to the
    ///         client Buffer manager satisfies the requirements passed with pCreateData.
    ///         General call flow from clients :
    ///         1. RegisterBufferManager - At the start of the use case, once client knows the properties of buffer requirements
    ///         in loop1
    ///         2. ActivateBufferManager - When client knows it will need buffers continuously, suggested to be called after
    ///         ...........................registering all known Buffer Managers. Do not call for each frame/buffer.
    ///         in loop2                 - While use case running
    ///         3. buffer1 = GetBufferFromPool(&bufferInfo1)
    ///         4. buffer2 = GetBufferFromPool(&bufferInfo2)
    ///         5. Use buffers
    ///         6. ReleaseBufferToPool(buffer1)
    ///         7. ReleaseBufferToPool(buffer2)
    ///         end loop2
    ///         8. DeactivateBufferManager - When client knows it will not need buffers until another Activate.
    ///         end loop1
    ///         9. UnregisterBufferManager - At the end of use.
    ///
    /// @param  pBufferManagerName      Name of Buffer manager to attach.
    /// @param  pCreateData             Buffer Manager create data.
    ///
    /// @return Unique MemPoolBufMgrHandle for this client Buffer Manager.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static MemPoolBufMgrHandle RegisterBufferManager(
        const CHAR*                     pBufferManagerName,
        const BufferManagerCreateData*  pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnregisterBufferManager
    ///
    /// @brief  This function unregisters the client buffer manager from Memory Pool Manager. Client must release
    ///         all the buffers that it acquired from Memory Pool Manager before calling this function.
    ///         Client cannot do any operations on this MemPoolBufMgrHandle after this call. Generally called at the end of
    ///         use case.
    ///
    /// @param  hMemPoolBufMgrHandle    Unique Memory Pool Manager Handle for the client Buffer Manager.
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult UnregisterBufferManager(
        MemPoolBufMgrHandle hMemPoolBufMgrHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ActivateBufferManager
    ///
    /// @brief  This function activates the client buffer manager. This helps as a hint to MemPoolMgr as when the buffers
    ///         being used by client Buffer manager. Memory Pool Manager may allocate minimum required buffers in this function
    ///         if not allocated by now (depending upon camxsettings). Call this function when client knows it would
    ///         start needing buffers continuously. If this function is not called by client, the first call to
    ///         GetBufferFromPool is treated as Activate for that particular MemPoolBufMgrHandle.
    ///
    /// @param  hMemPoolBufMgrHandle    Unique Memory Pool Manager Handle for the client Buffer Manager.
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ActivateBufferManager(
        MemPoolBufMgrHandle hMemPoolBufMgrHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeactivateBufferManager
    ///
    /// @brief  This function deactivates the client Buffer Manager. Memory Pool manager may free any buffers allocated for
    ///         for this client if the buffers are not required to share with other Buffer managers.
    ///
    /// @param  hMemPoolBufMgrHandle    Unique Memory Pool Manager Handle for the client Buffer Manager.
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult DeactivateBufferManager(
        MemPoolBufMgrHandle hMemPoolBufMgrHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferFromPool
    ///
    /// @brief  This function is used to get a buffer from shared pool. This will make sure the buffer returned is mapped
    ///         to the required devices listed in createData while Register. Once a buffer is given to a client buffer manager
    ///         it will not be used/shared with any other buffer managers until released back to Memory Pool Manager using
    ///         ReleaseBufferToPool API. If there is no buffer available in free list (allocated already but not in use)
    ///         by now, Memory Pool manager allocates a new buffer and returns.
    ///
    /// @param  hMemPoolBufMgrHandle    Unique Memory Pool Manager Handle for the client Buffer Manager.
    /// @param  pCSLBufferInfo          Pointer to CSLBufferInfo structure to fill information.
    /// @param  phGrallocBuffer         Pointer to fill Gralloc Buffer handle.
    ///
    /// @return Unique handle representing the buffer and buffer information.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static MemPoolBufferHandle GetBufferFromPool(
        MemPoolBufMgrHandle hMemPoolBufMgrHandle,
        CSLBufferInfo*      pCSLBufferInfo,
        BufferHandle*       phGrallocBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseBufferToPool
    ///
    /// @brief  This function is used to release a previously acquired buffer using GetBufferFromPool back to Memory Pool
    ///         Manager. Client Buffer Manager has to release all buffers that were acquired before calling
    ///         DeactivateBufferManager or UnregisterBufferManager.
    ///
    /// @param  hMemPoolBufMgrHandle    Unique Memory Pool Manager Handle for the client Buffer Manager.
    /// @param  hMemPoolBufferHandle    Unique handle representing the Memory Pool Buffer.
    /// @param  bufferCached            Whether memory is cached by the time it is released to Memory Pool Manager.
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ReleaseBufferToPool(
        MemPoolBufMgrHandle     hMemPoolBufMgrHandle,
        MemPoolBufferHandle     hMemPoolBufferHandle,
        BOOL                    bufferCached);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MapBufferToDevices
    ///
    /// @brief  This function maps the Mem Pool Buffer to additional devices/umd.
    ///
    /// @param  hMemPoolBufMgrHandle    Unique Memory Pool Manager Handle for the client Buffer Manager.
    /// @param  hMemPoolBufferHandle    Unique handle representing the Mem Pool Buffer.
    /// @param  offset                  Offset relative to the memory described by hMemPoolBufferHandle.
    /// @param  size                    Size of the memory described by hMemPoolBufferHandle.
    /// @param  flags                   Memory flags
    /// @param  pDeviceIndices          List of devices to which this buffer need to be mapped.
    /// @param  deviceCount             Number of devices valid in pDeviceIndices.
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult MapBufferToDevices(
        MemPoolBufMgrHandle hMemPoolBufMgrHandle,
        MemPoolBufferHandle hMemPoolBufferHandle,
        SIZE_T              offset,
        SIZE_T              size,
        UINT32              flags,
        const INT32*        pDeviceIndices,
        UINT                deviceCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferInfo
    ///
    /// @brief  This function gets the Buffer information of input MemPoolBufferHandle
    ///
    /// @param  hMemPoolBufMgrHandle    Unique Memory Pool Manager Handle for the client Buffer Manager.
    /// @param  hMemPoolBufferHandle    Unique handle representing the Memory Pool Buffer.
    /// @param  pCSLBufferInfo          Pointer to CSLBufferInfo structure to fill information.
    /// @param  phGrallocBuffer         Pointer to fill Gralloc Buffer handle.
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult GetBufferInfo(
        MemPoolBufMgrHandle hMemPoolBufMgrHandle,
        MemPoolBufferHandle hMemPoolBufferHandle,
        CSLBufferInfo*      pCSLBufferInfo,
        BufferHandle*       phGrallocBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintMemPoolMgrState
    ///
    /// @brief  Print Memory Pool Manager state.
    ///
    /// @param  forceTrigger    Whether to force trigger printing state by default
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID PrintMemPoolMgrState(
        BOOL    forceTrigger);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Private Methods
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MemPoolMgr
    ///
    /// @brief  Private constructor for a MemPoolMgr instance
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MemPoolMgr();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~MemPoolMgr
    ///
    /// @brief  Private destructor for a MemPoolMgr instance
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~MemPoolMgr();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstance
    ///
    /// @brief  This function returns the singleton instance of MemPoolMgr.
    ///
    /// @return A pointer to the singleton instance of MemPoolMgr
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static MemPoolMgr* GetInstance();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInstanceLocked
    ///
    /// @brief  This function returns the singleton instance of MemPoolMgr with lock on mutex.
    ///
    /// @return A pointer to the singleton instance of MemPoolMgr
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static MemPoolMgr* GetInstanceLocked();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Unlock
    ///
    /// @brief  This function is used to unlock mutex
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Unlock();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MemPoolGroupExists
    ///
    /// @brief  This function checks if the input MemPoolGroup exists in the list of current groups.
    ///
    /// @param  pMemPoolGroup   Pointer to MemPoolGroup object which needs to be checked.
    ///
    /// @return TRUE if Memory Pool Group exists
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL MemPoolGroupExists(
        MemPoolGroup*   pMemPoolGroup
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterNewMemPoolGroup
    ///
    /// @brief  This function registers the buffer manager to the new memory pool group
    ///
    /// @param  pBufferManagerName     Name of Buffer manager to attach.
    /// @param  pCreateData            Buffer Manager create data.
    /// @param  ppMemPoolGroup         Variable pointing to matching memory pool group
    /// @param  ppMemPoolBufMgr        Variable pointing to memory pool buffer manager
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RegisterNewMemPoolGroup(
        const CHAR*                      pBufferManagerName,
        const BufferManagerCreateData*   pCreateData,
        MemPoolGroup**                   ppMemPoolGroup,
        MemPoolBufferManager**           ppMemPoolBufMgr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterIfExistingGroupMatches
    ///
    /// @brief  This function registers the buffer managers to the matching existing memory pool group
    ///
    /// @param  pBufferManagerName     Name of Buffer manager to attach.
    /// @param  pCreateData            Buffer Manager create data.
    /// @param  ppMemPoolGroup         Variable pointing to matching memory pool group
    /// @param  ppMemPoolBufMgr        Variable pointing to memory pool buffer manager
    ///
    /// @return CamxResultENoSuch if no matching group found
    ///         CamxResultSuccess if a Matching group found and Register is success
    ///         Other errors      if a matching group found but failed in Register.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RegisterIfExistingGroupMatches(
        const CHAR*                      pBufferManagerName,
        const BufferManagerCreateData*   pCreateData,
        MemPoolGroup**                   ppMemPoolGroup,
        MemPoolBufferManager**           ppMemPoolBufMgr);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddMemPoolGroupToList
    ///
    /// @brief  This function adds the input MemPoolGroup object into the list of MemPoolGroups managed by MemPoolMgr
    ///
    /// @param  pMemPoolGroup   Pointer to MemPoolGroup object which needs to be added in the list.
    ///
    /// @return CamxResultSuccess on success.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddMemPoolGroupToList(
        MemPoolGroup*   pMemPoolGroup);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemoveMemPoolGroupFromList
    ///
    /// @brief  This function removes the input MemPoolGroup object from the list of MemPoolGroups managed by MemPoolMgr
    ///
    /// @param  pMemPoolGroup   Pointer to MemPoolGroup object which needs to be removed from the list.
    ///
    /// @return CamxResultSuccess on success..
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult RemoveMemPoolGroupFromList(
        MemPoolGroup*   pMemPoolGroup);

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
    /// MonitorThread
    ///
    /// @param  pArg Payload for the monitor thread
    ///
    /// @brief  MonitorThread
    ///
    /// @return NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* MonitorThread(
        VOID* pArg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Monitoring
    ///
    /// @brief  Actual monitor thread routine
    ///
    /// @return NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* Monitoring();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurMgrFreeingPoolState
    ///
    /// @brief  Get the current Freeing pool state
    ///
    /// @return Current FreeingMemoryPoolGroupState
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE FreeingPoolState GetCurMgrFreeingPoolState()
    {
        FreeingPoolState curMgrFreeingPoolState = FreeingPoolState::FreeingNotInProgress;

        curMgrFreeingPoolState = m_mpmFreeingPoolState;

        return curMgrFreeingPoolState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCurMgrFreeingPoolState
    ///
    /// @brief  Set the current Freeing pool state
    ///
    /// @param  freeingPoolState  State we want to set the Freeing pool state to
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetCurMgrFreeingPoolState(
        FreeingPoolState freeingPoolState)
    {
        m_mpmFreeingPoolState = freeingPoolState;

        // If any other thread is waiting for freeing to complete, signal now
        if (FreeingPoolState::FreeingComplete == freeingPoolState)
        {
            m_pFreeingPoolStateCond->Signal();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitOnFreeingStateComplete
    ///
    /// @brief  Wait for the freeing pool state to be changed to complete
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID WaitOnFreeingStateComplete()
    {
        m_pFreeingPoolStateCond->Wait(m_pLock->GetNativeHandle());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ClearAllMemPoolFreeList
    ///
    /// @brief  Clears all the free buffers from each memory pool group
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ClearAllMemPoolFreeList();

    // Do not implement the copy constructor or assignment operator
    MemPoolMgr(const MemPoolMgr& rMemPoolMgr)             = delete;
    MemPoolMgr& operator= (const MemPoolMgr& rMemPoolMgr) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Private Data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    LightweightDoublyLinkedList m_groupList;                ///< List of existing MemPoolGroups
    Mutex*                      m_pLock;                    ///< Mutex to protect access from multiple threads
    static BOOL                 s_isValid;                  ///< whether the MemPoolMgr singleton is in a valid state
    UINT32                      m_groupNameCounter;         ///< Group Counter for naming
    ThreadConfig                m_mpmMonitorThread;         ///< Memory Pool Manager Monitor thread
    Condition*                  m_pMonitorThreadCond;       ///< Wait on signal to start monitoring
    BOOL                        m_monitorThreadStart;       ///< Boolean indicates start or stop monitoring
    FreeingPoolState            m_mpmFreeingPoolState;      ///< The current state of the memory pool manager in regards
                                                            ///   to freeing memory pool group(s)
    Mutex*                      m_pFreeingPoolStateLock;    ///< Mutex to control who is accessing the current free pool state
    Condition*                  m_pFreeingPoolStateCond;    ///< Wait on pool state condition to start freeing buffers
};

CAMX_NAMESPACE_END

#endif // CAMXMEMPOOLMGR_H
