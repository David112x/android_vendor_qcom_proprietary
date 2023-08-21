////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxdeferredrequestqueue.h
/// @brief Deferred request queue declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXDEFERREDREQUESTQUEUE_H
#define CAMXDEFERREDREQUESTQUEUE_H

#include "camxcsl.h"
#include "camxchicontext.h"
#include "camxdefs.h"
#include "camxhal3types.h"
#include "camxhashmap.h"
#include "camxmetadatapool.h"
#include "camxosutils.h"
#include "camxpropertydefs.h"
#include "camxthreadmanager.h"
#include "camxtypes.h"
#include "camxchi.h"
#include "camxsession.h"

CAMX_NAMESPACE_BEGIN

/// Forward Declarations
class   DeferredRequestQueue;
class   Node;

struct  DependencyUnit;
struct  ChiFence;

CAMX_BEGIN_PACKED
/// @brief Key in the dependency entry
struct DependencyKey
{
    UINT64 requestId;      ///< Request ID
    UINT64 pipelineId;     ///< pipeline id for the data
    UINT32 dataId;         ///< Property/Metadata/Data identifier
    VOID*  pFence;         ///< Fence pointer
    VOID*  pChiFence;      ///< Chi Fence pointer
} CAMX_PACKED;

CAMX_END_PACKED
/// @brief value in the dependency entry
struct Dependency
{
    DeferredRequestQueue* pInstance;                      ///< Instance of DRQ holding dependency
    Node*                 pNode;                          ///< Pointer to the node with the dependencies
    INT32                 processSequenceId;              ///< Identifier for the node to track its processing order
                                                          ///< -2, -1, and 0 are reserved for core use
    BOOL                  bindIOBuffers;                  ///< Flag indicating whether node requested input, output buffers
                                                          ///  to be available on this Dependency
    UINT32                propertyCount;                  ///< Number of properties it is dependant on
    UINT32                publishedCount;                 ///< Number of properties that are published
    PropertyID            properties[MaxProperties];      ///< Properties which it is dependant on
    UINT64                offsets[MaxProperties];         ///< Offsets from the current request for property dependency
    BOOL                  negate[MaxProperties];          ///< Indicate if offsets are negative value
    UINT                  pipelineIds[MaxProperties];     ///< Pipeline index for the Properties
    UINT32                fenceCount;                     ///< Number of fences it is dependant on
    UINT32                signaledCount;                  ///< Number of fences that are signaled
    CSLFence*             phFences[MaxDependentFences];   ///< Fences to wait for
    UINT64                requestId;                      ///< Id for request
    ChiFence*             pChiFences[MaxDependentFences]; ///< Chi Fences to wait for
    UINT32                chiFenceCount;                  ///< Number of Chi fences it is dependant on
    UINT32                chiSignaledCount;               ///< Number of Chi fences that are signaled
    PFNCHIFENCECALLBACK   pChiFenceCallback;              ///< Callback when all Chi fence dependencies satisfied
    VOID*                 pUserData;                      ///< Client-provided data pointer for Chi callback
    BOOL                  preemptable;                    ///< Can this dependency be preempted
    BOOL                  isInternalDependency;           ///< Is this dependency internal to the base Camx Node?
};

/// @brief A dependency entry in the Deferred Process list
struct DependencyEntry
{
    DependencyKey                key;   ///< Key for hashmap lookup
    LightweightDoublyLinkedList* pVal;  ///< Value for hashmap entry
};

/// @brief DRQ create data
struct DeferredRequestQueueCreateData
{
    ThreadManager*  pThreadManager;                         ///< Pointer to Thread Manager
    UINT            numPipelines;                           ///< Number of Pipelines
    MetadataPool*   pMainPools[MaxPipelinesPerSession];     ///< Main Property Pools
    UINT            requestQueueDepth;                      ///< Depth of the request queue
    Session*        pSession;                               ///< Pointer to Session
};

/// @brief Callback data to identify the fence and the deferred processing object
struct DeferredFenceCallbackData
{
    DeferredRequestQueue*   pDeferredRequestQueue;  ///< Pointer to deferred processing object
    ChiFence*               pChiFence;              ///< Pointer to the chi fence
    UINT64                  requestId;              ///< Request Id that the fence is waiting in
};

/// @brief Preempt dependencies in the case of pipeline/session flush
struct PreemptDependencies
{
    BOOL   isPreemptDependencyEnabled;              ///< Premptdependency flag
    BOOL   pipelineDepenency;                       ///< Pipeline Preempt Dependency flag
    UINT32 pipelineIndex;                           ///< Pipeline Index if it is a pipeline dependency
};

/// @brief Limit to the list of data we keep accumulated to ensure it stays bounded
static const UINT MaxSavedThreadData = 32;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the deferred process node.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class DeferredRequestQueue : public IPropertyPoolObserver
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create DeferredProcess object
    ///
    /// @param  pCreateData  Data needed to create DRQ
    ///
    /// @return Instance pointer to be returned or NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static DeferredRequestQueue* Create(
        DeferredRequestQueueCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy DeferredProcess object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateDependency
    ///
    /// @brief  Update the deferred process with the new dependency info for this request
    ///
    /// @param  id          Property for which update is available
    /// @param  phFence     Fence for which update is available
    /// @param  pChiFence   Chi Fence for which update is available
    /// @param  requestId   Request Id for the property update
    /// @param  pipelineId  Pipeline ID for the property update
    /// @param  isSuccess   Whether update is success or failed
    /// @param  isFlush     Is session being flushed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateDependency(
        PropertyID  id,
        CSLFence*   phFence,
        ChiFence*   pChiFence,
        UINT64      requestId,
        UINT        pipelineId,
        BOOL        isSuccess,
        BOOL        isFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddDeferredNode
    ///
    /// @brief  Add node for deferred processing
    ///
    /// @param  requestId           Id of the request to process
    /// @param  pNode               Node to be deferred
    /// @param  pDependency         Dependency descriptor
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddDeferredNode(
        UINT64           requestId,
        Node*            pNode,
        DependencyUnit*  pDependency);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitForFenceDependency
    ///
    /// @brief  Add node for deferred processing
    ///
    /// @param  ppChiFences Array of fences to wait for
    /// @param  numFences   Number of fences in the above array
    /// @param  pCallback   Function that will be called when all fences singal
    /// @param  pUserData   User data that will be provided back to client upon signal
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WaitForFenceDependency(
        ChiFence**          ppChiFences,
        UINT32              numFences,
        PFNCHIFENCECALLBACK pCallback,
        VOID*               pUserData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FenceErrorSignaledCallback
    ///
    /// @brief  Callback method notifying clients of a dependent fence signaled (for buffers filled at the input ports)
    ///
    /// @param  pipelineId    Pipeline owning the node for which the error occurred
    /// @param  phFence       The fence that just got signaled
    /// @param  requestId     The frame the property applies to.
    /// @param  isFlush       Is session being flushed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FenceErrorSignaledCallback(
        UINT       pipelineId,
        CSLFence*  phFence,
        UINT64     requestId,
        BOOL       isFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FenceSignaledCallback
    ///
    /// @brief  Callback method notifying clients of a dependent fence signaled (for buffers filled at the input ports)
    ///
    /// @param  phFence     The fence that just got signaled
    /// @param  requestId   The frame the property applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FenceSignaledCallback(
        CSLFence* phFence,
        UINT64    requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFenceSignaledCallback
    ///
    /// @brief  Callback method notifying clients of a dependent Chi fence signaled
    ///
    /// @param  pChiFence   The Chi fence that just got signaled
    /// @param  requestId   The frame the property applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ChiFenceSignaledCallback(
        ChiFence* pChiFence,
        UINT64    requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // IPropertyPoolObserver Methods
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPropertyUpdate
    ///
    /// @brief  Callback method notifying clients of an available property. The OnPropertyUpdate method is called
    ///         according to the subscribed properties to notify client that an update is available.
    ///
    /// @param  id          The property that has been updated.
    /// @param  requestId   The frame the property applies to.
    /// @param  pipelineId  The pipeline id of the property applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID OnPropertyUpdate(
        PropertyID  id,
        UINT64      requestId,
        UINT        pipelineId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnMetadataUpdate
    ///
    /// @brief  Callback method notifying clients of an available metadata. The OnMetadataUpdate method is called
    ///         according to the subscribed metadata to notify client that an update is available.
    ///
    /// @param  tag         The metadata that has been updated.
    /// @param  requestId   The frame the metadata applies to.
    /// @param  pipelineId  The pipeline id of the property applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID OnMetadataUpdate(
        UINT32 tag,
        UINT64 requestId,
        UINT   pipelineId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnPropertyFailure
    ///
    /// @brief  Callback method notifying clients that a property update has failed for a request
    ///
    /// @param  id          The property update that has failed
    /// @param  requestId   The frame the property applies to.
    /// @param  pipelineId  The pipeline id of the property applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID OnPropertyFailure(
        PropertyID  id,
        UINT64      requestId,
        UINT        pipelineId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnMetadataFailure
    ///
    /// @brief  Callback method notifying clients that a metadata update has failed for a request
    ///
    /// @param  tag         The metadata that has been updated.
    /// @param  requestId   The frame the metadata applies to.
    /// @param  pipelineId  The pipeline id of the property applies to.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID OnMetadataFailure(
        UINT32 tag,
        UINT64 requestId,
        UINT   pipelineId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LockForPublish
    ///
    /// @brief  Perform locking necessary to ensure coherency for updating subscriptions
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID LockForPublish()
    {
        m_pDeferredQueueLock->Lock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnlockAfterPublish
    ///
    /// @brief  Perform unlock after publish
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UnlockAfterPublish()
    {
        m_pDeferredQueueLock->Unlock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RequestInErrorState
    ///
    /// @brief  Checks to see if there were any errors reported for this request
    ///
    /// @param  requestId   The frame the property applies to.
    /// @param  pipelineId  The pipeline the property applies to.
    ///
    /// @return TRUE if error
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL RequestInErrorState(
        UINT64    requestId,
        UINT      pipelineId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleFenceError
    ///
    /// @brief  Marks the request as an error
    ///
    /// @param  pipelineId    Pipeline owning the node for which the error occurred
    /// @param  requestId     The frame with a bad fence
    /// @param  isFlush       Is session being flushed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleFenceError(
        UINT       pipelineId,
        UINT64     requestId,
        BOOL       isFlush);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// End
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DispatchReadyNodes
    ///
    /// @brief  Evaluate the list of deferred nodes for all satisfied dependencies and dispatch
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DispatchReadyNodes();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpDebugInfo
    ///
    /// @brief  Dump the current state of the DRQ
    ///
    /// @param  id                 Request ID.
    /// @param  forceDump          Irrespective DRQ settings.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpDebugInfo(
        UINT64 id,
        BOOL   forceDump);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpState
    ///
    /// @brief  Deeper dump the current state of the DRQ to a file
    ///
    /// @param  fd          file descriptor.
    /// @param  indent      indent spaces.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpState(
        INT     fd,
        UINT32  indent);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPreemptDependencyFlag
    ///
    /// @brief  Set flag to indicated if DRQ can preempt dependency wait for nodes that have registered for preemption
    ///
    /// @param  isPreemptDependencyEnabled  flag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPreemptDependencyFlag(
        BOOL isPreemptDependencyEnabled);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPreemptPipelineDependency
    ///
    /// @brief  Set flag to indicate if DRQ can preempt dependency wait for nodes that have registered for preemption - for a
    ///         given pipeline Index
    ///
    /// @param  isPreemptDependencyEnabled  Flag stating whether to clear dependencies or not.
    /// @param  pipelineIndex               Pipeline Index to preempt dependency for
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPreemptPipelineDependency(
        BOOL   isPreemptDependencyEnabled,
        UINT32 pipelineIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief  Empty the hashmap and free up the resources in the DRQ
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Flush();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize internal data structures for DeferredProcess
    ///
    /// @param  pCreateData  Data needed to create DRQ
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize(
        DeferredRequestQueueCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeferredRequestQueue
    ///
    /// @brief  Constructor for DeferredRequestQueue object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    DeferredRequestQueue();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~DeferredRequestQueue
    ///
    /// @brief  Destructor for DeferredRequestQueue object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~DeferredRequestQueue();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeferredWorkerCore
    ///
    /// @brief  Common Deferred Worker routine for Topology
    ///
    /// @param  pDependency Pointer to the dependency information
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DeferredWorkerCore(
        Dependency* pDependency);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeferredWorkerWrapper
    ///
    /// @brief  Static Deferred worker function wrapper
    ///
    /// @param  pData Thread payload, which is the DeferredProcess instance pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* DeferredWorkerWrapper(
        VOID* pData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddDependencyEntry
    ///
    /// @brief  Add a dependency entry to the dependency map. A dependency entry can be corresponding to a buffer dependency
    ///         or a single (partial) list of property dependencies
    ///
    /// @param  pDependency Dependency to be added to the lists
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddDependencyEntry(
        Dependency*      pDependency);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUnpublishedList
    ///
    /// @brief  For a given request ID, check the property slot to see which of the supplied property list are signaled
    ///         or published. Return the unpublished list and it's count
    ///
    /// @param  pInput           Dependency unit provided for node
    /// @param  pDependency      Destination of outstanding prop dependencies
    /// @param  requestId        Request ID of the slot to check
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetUnpublishedList(
        DependencyUnit* pInput,
        Dependency*     pDependency,
        UINT64          requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddChiDependencyEntry
    ///
    /// @brief  Add a Chi fence dependency entry to the dependency map.
    ///
    /// @param  pDependency Dependency to be added to the map
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddChiDependencyEntry(
        Dependency* pDependency);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FenceCallbackCSL
    ///
    /// @brief  Callback method for CSL fences
    ///
    /// @param  pUserData   Opaque pointer provided by client to CSLFenceAsyncWait.
    /// @param  hSyncFence  The CSL fence that triggered the callback.
    /// @param  result      Indicates the success/failure status of the signal
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID FenceCallbackCSL(
        VOID*           pUserData,
        CSLFence        hSyncFence,
        CSLFenceResult  result);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DependencyFenceCallbackCSL
    ///
    /// @brief  Dependency Callback method for CSL fences
    ///
    /// @param  pUserData   Opaque pointer provided by client to CSLFenceAsyncWait.
    /// @param  hSyncFence  The CSL fence that triggered the callback.
    /// @param  result      Indicates the success/failure status of the signal
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DependencyFenceCallbackCSL(
        VOID*           pUserData,
        CSLFence        hSyncFence,
        CSLFenceResult  result);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateChiFenceDependency
    ///
    /// @brief  Update the deferred process with the new dependency info
    ///
    /// @param  pChiFence   Pointer to Chi fence for which update is available
    /// @param  isSuccess   Whether update is success or failed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateChiFenceDependency(
        ChiFence*   pChiFence,
        BOOL        isSuccess);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateOrRemoveDependency
    ///
    /// @brief  Remove node dependencies from dependency map
    ///
    /// @param  pMapKey             pointer to dependency map key
    /// @param  pDependencyToRemove Pointer to dependency to remove from map key list
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateOrRemoveDependency(
        DependencyKey* pMapKey,
        Dependency*    pDependencyToRemove);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemoveAllDependencies
    ///
    /// @brief  Drop all dependencies i.e property, fence and chi fence from dependency map
    ///
    /// @param  pDependency Pointer to dependency to drop
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RemoveAllDependencies(
        Dependency* pDependency);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeDependencyMapListData
    ///
    /// @brief  Free data from hashmap
    ///
    /// @param  pData pointer to val in hashmap
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID FreeDependencyMapListData(
        VOID* pData);

    // Do not implement the copy constructor or assignment operator
    DeferredRequestQueue(const DeferredRequestQueue& rDeferredRequestQueue) = delete;
    DeferredRequestQueue& operator= (const DeferredRequestQueue& rDeferredRequestQueue) = delete;


    /// @todo (CAMX-1797) Not sure if we need anything fancy, quickly rethink it
    static  UINT                s_numInstances;         ///< Number of instances of the DRQ to be used to uniquely identify an
                                                        ///  instance
    Hashmap*                    m_pDependencyMap;       ///< Hashmap to store pending dependencies
    Hashmap*                    m_pFenceRequestMap;     ///< Hashmap to store mapping from fence to request
    ThreadManager*              m_pThreadManager;       ///< Pointer to Thread Manager
    Session*                    m_pSession;             ///< pointer to Session to which this DRQ belongs
    JobHandle                   m_hDeferredWorker;      ///< Deferred worker handle
    Mutex*                      m_pDeferredQueueLock;   ///< Lock to secure access to dependency map
    Mutex*                      m_pReadyQueueLock;      ///< Lock to secure access to ready queue
    LightweightDoublyLinkedList m_deferredNodes;        ///< List of deferred nodes
    LightweightDoublyLinkedList m_readyNodes;           ///< List of ready nodes

    UINT                        m_numPipelines;                        ///< Active number of pipelines
    MetadataPool*               m_pMainPools[MaxPipelinesPerSession];  ///< Pointer to metadata/property pool
    LightweightDoublyLinkedList m_CHIFenceDependencies;                ///< List of dependency references for Chi fences
    LightweightDoublyLinkedList m_errorRequests;                       ///< List of requests in error state

    volatile UINT               m_numErrorRequests;                    ///< Atomic count to track the number of error request
    PreemptDependencies         m_preemptDependency;                   ///< Indicate if DRQ can preempt dependency wait
    BOOL                        m_logEnabled;                          ///< Cache of setting logDRQEnable
};

CAMX_NAMESPACE_END

#endif // CAMXDEFERREDREQUESTQUEUE_H
