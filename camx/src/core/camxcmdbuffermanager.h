////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcmdbuffermanager.h
/// @brief CmdBufferManager class declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXCMDBUFFERMANAGER_H
#define CAMXCMDBUFFERMANAGER_H

#include "camxcmdbuffer.h"
#include "camxdefs.h"
#include "camxhal3defs.h"
#include "camxlist.h"
#include "camxpacket.h"
#include "camxpacketresource.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

class CmdBuffer;
class CmdBufferManager;

/// @brief Parameters that describe resources in a pool (size, number)
///        This will be mainly size and number of resources, but also type-specific parameters for initialization.
struct ResourceParams
{
    UINT                        resourceSize;   ///< Size of resource
    UINT                        poolSize;       ///< Pool size
    UINT32                      alignment;      ///< Required alignment
    PacketResourceUsageFlags    usageFlags;     ///< Flags that describe the resource
    UINT32                      memFlags;       ///< Resource memory flags
    const INT32*                pDeviceIndices; ///< Pointer to indices of devices accessing this resource
    UINT32                      numDevices;     ///< Number of indices pointed to by pDeviceIndices
    union
    {
        PacketParams            packetParams;   ///< Packet parameters
        CmdParams               cmdParams;      ///< Command parameters
    };
};

/// @brief struct defining Cmd Buffer Manager statistics
struct CmdBufferManagerStats
{
    UINT    numBuffersAllocated;                ///< Number of allocations at any given point of time
    UINT    peakNumBuffersAllocated;            ///< Peak number of allocations
    SIZE_T  sizeOfMemoryAllocated;              ///< Size of Memory allocated at any given point of time
    SIZE_T  peakSizeOfMemoryAllocated;          ///< Peak size of Memory allocated
};

/// @brief Params Required to create cmd buffer manager
struct CmdBufferManagerParam
{
    CHAR*              pBufferManagerName;   ///< Pointer to the  string containing buffer manager name
    ResourceParams*    pParams;              ///< Pointer to the structure containing resource params
    CmdBufferManager** ppCmdBufferManager;   ///< Pointer to the cmd buffer manager handle that needs to be created
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements CAMX packet memory manager.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CmdBufferManager
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create and initialize CmdBufferManager instance
    ///
    /// @param  pBufferManagerName  Cmd Buffer Manager name
    /// @param  pParams             Parameters to pass the created manager
    /// @param  ppCmdBufferManager  Pointer to the manager to fill
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult Create(
        const CHAR*           pBufferManagerName,
        const ResourceParams* pParams,
        CmdBufferManager**    ppCmdBufferManager);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateMultiManager
    ///
    /// @brief  Create and initialize Multiple CmdBuffer Manager instances
    ///
    /// @param  bDisableOverrideSettings   Parameters to use overridesetting for cmd buffer managers
    /// @param  pParams                    Parameters to pass to create the cmd buffer managers
    /// @param  numberOfCmdBufferManagers  Number of command buffer managers to create
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult CreateMultiManager(
        BOOL                   bDisableOverrideSettings,
        CmdBufferManagerParam* pParams,
        UINT32                 numberOfCmdBufferManagers);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CmdBufferManager
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CmdBufferManager();


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CmdBufferManager
    ///
    /// @brief  Constructor
    ///
    /// @param  bDisableOverrideSettings        Flag to disable override settings
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit CmdBufferManager(
        BOOL    bDisableOverrideSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CmdBufferManager
    ///
    /// @brief  Constructor
    ///
    /// @param  bDisableOverrideSettings   Flag to disable override settings
    /// @param  pParentCmdBufferManager    Pointer to the Parent command buffer manager
    /// @param  bIgnoreAlignment           Flag to indicate the buffer alignment
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit CmdBufferManager(
        BOOL              bDisableOverrideSettings,
        CmdBufferManager* pParentCmdBufferManager,
        BOOL              bIgnoreAlignment);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CmdBufferManager
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~CmdBufferManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Allocate memory and setup the pools. It will first free all previously-allocated resources.
    ///
    /// @param  pBufferManagerName  Cmd Buffer Manager name
    /// @param  pParams             Array of resource parameters, sorted based on size, in increasing order
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        const CHAR*             pBufferManagerName,
        const ResourceParams*   pParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Uninitialize
    ///
    /// @brief  Uninitialize the resource manager
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Uninitialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Recycle
    ///
    /// @brief  Recycle the buffer. This should be called to recycle resources manually i.e. independent of requests. It is
    ///         assumed the pResource passed in here was obtained by the client using the GetBuffer(..) interface and not
    ///         GetBufferForRequest(..)
    ///
    /// @param  pResource Resource to recycle
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Recycle(
        PacketResource* pResource);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RecycleAll
    ///
    /// @brief  Recycle all the buffers of the given request
    ///
    /// @param  requestId The requestId of the retired request.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RecycleAll(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RecycleAllRequests
    ///
    /// @brief  Recycle all the buffers for all the requests
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RecycleAllRequests();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferForRequest
    ///
    /// @brief  This function gets a buffer from the free pool. It removes the buffer from the free pool and also inserts it
    ///         into the busy pool. If the client uses this interface to get a buffer it is assumed the client will not
    ///         manually recycle this buffer i.e. this class will automatically recycle the buffer once the requestId retires
    ///
    /// @param  requestId The request Id to which this resource's lifecycle will be tied to
    /// @param  ppBuffer  Address of a pointer that will be filled to point to a free buffer
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetBufferForRequest(
        UINT64           requestId,
        PacketResource** ppBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBuffer
    ///
    /// @brief  This function removes the buffer from the free pool but does not insert it into the busy pool. If the client
    ///         invokes this function to get a buffer it is assumed the client will manually recycle this buffer by calling
    ///         the Recycle() interface
    ///
    /// @param  ppBuffer  Address of a pointer that will be filled to point to a free buffer
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetBuffer(
        PacketResource** ppBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckBufferWithRequest
    ///
    /// @brief  This function checks a buffer from the busy pool with request id.
    ///
    /// @param  requestId The request Id to which this resource's lifecycle will be tied to
    /// @param  ppBuffer  Address of a pointer that will be filled to point to a buffer
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckBufferWithRequest(
        UINT64           requestId,
        PacketResource** ppBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMergedCSLBufferInfo
    ///
    /// @brief  This function fills and gives the CSL Buffer Info managed the Parent csl buffer info in Merged command buffer
    ///         manager case, and the current buffer manager csl buffer info for unmerged command buffer manager case
    ///
    /// @return Pointer to the CSL Buffer Information structure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CSLBufferInfo* GetMergedCSLBufferInfo()
    {
        const CSLBufferInfo* pBufferInfo = NULL;

        if (NULL != m_pParentBufferManager)
        {
            pBufferInfo = m_pParentBufferManager->GetCSLBufferInfo();
        }
        else
        {
            pBufferInfo = m_pBufferInfo;
        }

        return pBufferInfo;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLBufferInfo
    ///
    /// @brief  Gets the CSL Buffer Info
    ///
    /// @return CSL Buffer Information structure Pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CSLBufferInfo* GetCSLBufferInfo()
    {
        return m_pBufferInfo;
    }

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializePool
    ///
    /// @brief  Helper method that initializes the pool of free buffers
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult InitializePool();

private:
    CmdBufferManager(const CmdBufferManager&) = delete;             // Disallow the copy constructor.
    CmdBufferManager& operator=(const CmdBufferManager&) = delete;  // Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeMemHandles
    ///
    /// @brief  Free CSL memory handles
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FreeMemHandles();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeResources
    ///
    /// @brief  Free all resources (CSL memory and packet resource objects).
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FreeResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPaddedSize
    ///
    /// @brief  Get the size that accounts for alignment and padding
    ///
    /// @param  sizeInBytes         Size to be padded
    /// @param  alignmentInBytes    The desired alignment
    ///
    /// @return Padded size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetPaddedSize(
        UINT32 sizeInBytes,
        UINT32 alignmentInBytes);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UninitializePools
    ///
    /// @brief  Helper method that uninitializes one single pool of resources
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UninitializePools();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateResource
    ///
    /// @brief  Resource factory method
    ///
    /// @param  pBufferInfo CSL allocation to break up
    /// @param  offset      Offset from the allocation where the resource memory begins
    /// @param  pParams     Object-specific parameters
    /// @param  ppResource  Address of the result resource pointer
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateResource(
        CSLBufferInfo*      pBufferInfo,
        SIZE_T              offset,
        ResourceParams*     pParams,
        PacketResource**    ppResource);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBufferOffset
    ///
    /// @brief  Sets the current CSL Buffer offset
    ///
    /// @param  offset buffer offset
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetBufferOffset(
        SIZE_T offset)
    {
        m_currentBufferOffset = offset;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferOffset
    ///
    /// @brief  Gets the current buffer offset
    ///
    /// @return buffer offset
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE SIZE_T GetBufferOffset()
    {
        return m_currentBufferOffset;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IncrementChildRefCount
    ///
    /// @brief  Incements the Child buffer managers count
    ///
    /// @return Returns the new Child Ref count
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 IncrementChildRefCount()
    {
        m_numberOfChildBufferManagers++;
        return m_numberOfChildBufferManagers;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DecrementChildRefCount
    ///
    /// @brief  Decrements the child buffer managers ref count
    ///
    /// @return Returns the new child Ref count
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 DecrementChildRefCount()
    {
        m_numberOfChildBufferManagers--;
        return m_numberOfChildBufferManagers;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFWBufferAlignedSize
    ///
    /// @brief  Calculates the FW accssible buffer alignment
    ///
    /// @param  sizeInBytes size of the memory in bytes, which is to be aligned
    ///
    /// @return return aligned buffer size
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CAMX_INLINE UINT32 GetFWBufferAlignedSize(
        UINT32 sizeInBytes)
    {
        UINT32 alignment  = 0;
        UINT32 bytesPerKb = 1024;

        if (sizeInBytes < (4 * bytesPerKb))
        {
            alignment = 4 * bytesPerKb;
        }
        else
        {
            if (0 != (sizeInBytes % (64 * bytesPerKb)))
            {
                alignment  = ((sizeInBytes / (64 * bytesPerKb)) + 1) * (64 * bytesPerKb);
            }
            else
            {
                alignment = sizeInBytes;
            }
        }

        return alignment;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateParentManager
    ///
    /// @brief  Create the Parent Manager for the multi buffer manager
    ///
    /// @param  bDisableOverrideSettings   Flag to disable override settings
    /// @param  pParams                    Parameters to pass to create the cmd buffer managers
    /// @param  numberOfCmdBufferManagers  Number of command buffer managers to create
    ///
    /// @return returns the Parent Buffer Manager instance if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CmdBufferManager* CreateParentManager(
        BOOL                   bDisableOverrideSettings,
        CmdBufferManagerParam* pParams,
        UINT32                 numberOfCmdBufferManagers);

    CHAR                            m_pBufferManagerName[MaxStringLength256];   ///< Name of the buffer manager
    BOOL                            m_initialized;                              ///< initialized
    BOOL                            m_bEnableMemoryStats;                       ///< Flag to enable memory stat
    LightweightDoublyLinkedList     m_CSLAllocations;                           ///< list of CSLMemHandle (CSL allocations)
    ResourceParams                  m_params;                                   ///< Parameters of this pool
    UINT32                          m_flags;                                    ///< Final flags used while allocation
    UINT                            m_numResources;                             ///< Number of resources based on the provided
                                                                                ///  parameters
    LightweightDoublyLinkedList     m_freePool;                                 ///< Pool of free buffers
    LightweightDoublyLinkedList     m_busyPool;                                 ///< Pool of busy buffers
    Mutex*                          m_pLock;                                    ///< Mutex to protect internal state
    UINT                            m_peakNumResourcesUsed;                     ///< Peak number of buffers used in this Mgr
    UINT                            m_peakSizeUsedWithinAResource;              ///< Peak size used within a Resource in this
    static Mutex*                   s_pStatsLock;                               ///< Mutex to protect accessing stats
    static CmdBufferManagerStats    s_stats;                                    ///< Cmd Buffer Manager statistics.
                                                                                ///  This is a static variable, so it tracks all
                                                                                ///  allocation stats in all CmdBuffer Managers
    CmdBufferManager*               m_pParentBufferManager;                     ///< Parent command buffer manager
                                                                                ///  for this buffer manager
    UINT32                          m_numberOfChildBufferManagers;              ///< Number of child Buffer managers
                                                                                ///  for this Buffer Manager
    CSLBufferInfo*                  m_pBufferInfo;                              ///< CSL Buffer Info
    SIZE_T                          m_currentBufferOffset;                      ///< Current Buffer Offset
    BOOL                            m_bIgnoreBufferAlignment;                   ///< Indicates to ignore buffer alignment
                                                                                ///  modification by command buffer manager
};

CAMX_NAMESPACE_END

#endif // CAMXCMDBUFFERMANAGER_H
