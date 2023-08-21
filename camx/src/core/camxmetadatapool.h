////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file camxmetadatapool.h
/// @brief Define a slot and a pool for camx metadata and property handling
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXMETADATAPOOL_H
#define CAMXMETADATAPOOL_H

// NOWHINE FILE PR007b: Whiner incorrectly concludes as non-library files
// NOWHINE FILE CP006: used standard libraries for performance improvements

#include <array>
#include <list>

#include "camxincs.h"
#include "camxmetabuffer.h"
#include "camxpropertyblob.h"
#include "camxpropertydefs.h"
#include "camxutils.h"
#include "camxthreadmanager.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class IPropertyPoolObserver;
class MetadataPool;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Type definitions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @brief The max number of subscribers supported for each property/tag/all
static const UINT MaxSubscribers = 8;

/// @brief Type of property pool
///
/// @note  Update PoolTypeStrings if changed
enum class PoolType
{
    Static,                ///< Static pool of metadata
    PerFrameInput,         ///< Per frame pool of metadata + properties
    PerFrameResult,        ///< Per frame pool of metadata + properties
    PerFrameResultEarly,   ///< Per frame pool of early return metadata + properties
    PerFrameInternal,      ///< Per frame internal pool of properties
    PerFrameDebugData,     ///< Per frame debug data pool
    PerUsecase             ///< Per usecase internal pool of properties
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Strings of PoolType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const CHAR* PoolTypeStrings[]
{
    "Static",                 ///< Static pool of metadata
    "PerFrameInput",          ///< Per frame pool of metadata input from app
    "PerFrameResult",         ///< Per frame pool of metadata + properties
    "PerFrameResultEarly",    ///< Per frame pool of metadata + properties
    "PerFrameInternal",       ///< Per frame internal pool of properties
    "PerFrameDebugData",      ///< Per frame debug data pool
    "PerUsecase"              ///< Per usecase pool of metadata + properties
};

/// @brief Unit type - property or metadata
enum class UnitType
{
    Property,   ///< this is a property type
    Metadata    ///< this is a metadata tag type
};


/// @brief Pool Status - track the status of the pool
enum class PoolStatus
{
    Uninitialized,  ///< Pool is uninitialized
    Initialized,    ///< Pool is successfully initialized
    Error           ///< Pool initialization failed
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Define subscription parameters for a property/metadata update. All notifications are immediate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct Subscription
{
    UnitType    type;       ///< What type of subscription it is (property type or metadata tag)
    UINT32      unitId;     ///< What is being subscribed to (metadata tag or property type)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure to track the subscription entries
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SubscriptionEntry
{
    IPropertyPoolObserver*  pClient;     ///< Observer for this subscription
    const CHAR*             pClientName; ///< Name of the observer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// IPropertyPoolObserver
///
/// @brief Observer interface for property pool. This interface can be implemented by nodes or by topology itself
///        interested in receiving notification of updates to properties and metadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE CP017,CP018,CP044: Interface does not need copy/assignment/default overrides
class IPropertyPoolObserver
{
public:
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
    virtual VOID OnPropertyUpdate(
        PropertyID  id,
        UINT64      requestId,
        UINT        pipelineId) = 0;

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
    virtual VOID OnMetadataUpdate(
        UINT32 tag,
        UINT64 requestId,
        UINT   pipelineId) = 0;

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
    virtual VOID OnPropertyFailure(
        PropertyID  id,
        UINT64      requestId,
        UINT        pipelineId) = 0;

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
    virtual VOID OnMetadataFailure(
        UINT32 tag,
        UINT64 requestId,
        UINT   pipelineId) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LockForPublish
    ///
    /// @brief  Perform locking necessary to ensure coherency for updating subscriptions
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID LockForPublish() = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnlockAfterPublish
    ///
    /// @brief  Perform unlock after publish
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID UnlockAfterPublish() = 0;

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~IPropertyPoolObserver
    ///
    /// @brief  Protected destructor to prevent accidental deletion of the observer.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~IPropertyPoolObserver() = default;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief MetadataSlot is an utility class that contains a Metadata POD + some utility methods, and is indexed by a Frame ID.
///        The utility methods help the Topology and the individual nodes to get/set a metadata tag or a property type.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MetadataSlot final
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create an instance of MetadataSlot
    ///
    /// @param  entryCapacity   Entry capacity for each metadata in the slot
    /// @param  dataSize        Data capacity for each metadata in the slot
    /// @param  pPool           Pointer to the pool to which this slot belongs
    ///
    /// @return Instance pointer to be returned or NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static MetadataSlot* Create(
        SIZE_T        entryCapacity,
        SIZE_T        dataSize,
        MetadataPool* pPool);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Method to delete an instance of MetadataSlot
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataByTag
    ///
    /// @brief  Retrieve the pointer to the data corresponding to the metadata tag
    ///
    /// @param  tag     Specific metadata tag to look for
    /// @param  pName   Name of the client
    ///
    /// @return Pointer which will point to the data for the tag when the function returns
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOWHINE CP021: client name is added for debug purpose for nodes
    VOID* GetMetadataByTag(
        UINT32      tag,
        const CHAR* pName = "");

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataByCameraId
    ///
    /// @brief  Retrieve the pointer to the data corresponding to the metadata tag for the specified cameraId
    ///
    /// @param  tag         Specific metadata tag to look for
    /// @param  cameraId    CameraId of metadata tag to look for
    /// @param  pName       Name of the client
    ///
    /// @return Pointer which will point to the data for the tag when the function returns
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOWHINE CP021: client name is added for debug purpose for nodes
    VOID* GetMetadataByCameraId(
        UINT32      tag,
        UINT32      cameraId,
        const CHAR* pName = "");

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataCountByTag
    ///
    /// @brief  Retrieve count of published data for metadata tag
    ///
    /// @param  tag         Specific metadata tag to look for
    /// @param  allowSticky For input pool, allow use of sticky count or force use of request metadata specifically
    ///
    /// @return Count
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SIZE_T GetMetadataCountByTag(
        UINT32  tag,
        BOOL    allowSticky);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataListByTags
    ///
    /// @brief  For a list of metadata tags, retrieve the pointer to the data corresponding to the tag
    ///
    /// @param  numTagsRequested Number of metadata tags asked for
    /// @param  pTagList         List of specific metadata tags asked for
    /// @param  ppData           List of pointers (all initially NULL), each of which will point to the data for a specific tag,
    ///                          when the function returns
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetMetadataListByTags(
        UINT    numTagsRequested,
        UINT32* pTagList,
        VOID**  ppData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMetadataByTag
    ///
    /// @brief  Overwrite the contents of the data, corresponding to the metadata tag
    ///
    /// @param  tag     Specific metadata tag to look for
    /// @param  pData   Pointer to the data for the tag
    /// @param  count   Count of data the type of the tag
    /// @param  pName   Name of the client
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetMetadataByTag(
        UINT32      tag,
        const VOID* pData,
        SIZE_T      count,
        const CHAR* pName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPropertyBlob
    ///
    /// @brief  Retrieve the pointer to the PropertyBlob for debug data and internal data
    ///
    /// @param  ppBlob Pointer which will point to the blob when the function returns
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPropertyBlob(
        VOID** ppBlob);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishMetadataList
    ///
    /// @brief  Mark a metadata tag as published and ready to be notified
    ///
    /// @param  pTagList    Tag to be published
    /// @param  numTags     Number of tags in the list
    /// @param  pClientName Name of the client who publishes the metadata
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOWHINE CP021: client name is added for debug purpose for nodes
    CamxResult PublishMetadataList(
        UINT32*     pTagList,
        UINT32      numTags,
        const CHAR* pClientName ="");

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishMetadata
    ///
    /// @brief  Mark a property or metadata as published and ready to be notified
    ///
    /// @param  metadata    Property/Metadata to be published
    /// @param  pClientName Name of the client who publishes the metadata
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // NOWHINE CP021: client name is added for debug purpose for nodes
    CamxResult PublishMetadata(
        UINT32      metadata,
        const CHAR* pClientName = "");

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPublished
    ///
    /// @brief  If someone wants to quickly check the published status of a property for a slot,
    ///         without waiting to be notified for it
    ///
    /// @param  type    Is it a property or metadata?
    /// @param  unitId  Property type or metadata tag
    ///
    /// @return TRUE if published
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsPublished(
        UnitType    type,
        UINT32      unitId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Invalidate
    ///
    /// @brief  Invalidates all tags and properties of this slot
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Invalidate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSlotRequestId
    ///
    /// @brief  Set the request Id corresponding to this metadata slot
    ///
    /// @param  requestId Request Id of the slot
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetSlotRequestId(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSlotRequestId
    ///
    /// @brief  Get the request Id corresponding to this metadata slot
    ///
    /// @return the requestId Request Id of the slot
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT64 GetSlotRequestId() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadLock
    ///
    /// @brief  Lock the slot for reading. No write operations can occur while the slot is locked for reading, but other
    ///         threads are not blocked from reading
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReadLock() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WriteLock
    ///
    /// @brief  Lock the slot for writing. No other threads can read or write the slot while locked for write
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID WriteLock() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Unlock
    ///
    /// @brief  Unlock the slot for either read or write operations
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Unlock() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetMetadata
    ///
    /// @brief  Reset the metadata in the slot
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ResetMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MergeMetadata
    ///
    /// @brief  Merge the metadata in the slot from other metadata
    ///
    /// @param  pSrcMetadata Source metadata to merge with
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID MergeMetadata(
        const Metadata* pSrcMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintTagData
    ///
    /// @brief  Prints the data stored with this tag
    ///
    /// @param  tag     the metadata tag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintTagData(
        UINT32      tag);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AttachMetabuffer
    ///
    /// @brief  Attach input metabuffer to the slot
    ///
    /// @param  pMetabuffer  pointer to the input metabuffer object
    ///
    /// @return Proper result code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AttachMetabuffer(
        MetaBuffer* pMetabuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetabuffer
    ///
    /// @brief  Get result metabuffers from the slot
    ///
    /// @param  ppMetabuffer  double pointer to result metabuffer object
    ///
    /// @return Proper result code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetMetabuffer(
        MetaBuffer** ppMetabuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DetachMetabuffer
    ///
    /// @brief  Detach metabuffer from the slot
    ///
    /// @param  ppMetabuffer  double pointer to result metabuffer object
    ///
    /// @return Proper result code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DetachMetabuffer(
        MetaBuffer** ppMetabuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadata
    ///
    /// @brief  Get the metadata blob
    ///
    /// @return pointer to the metadata
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Metadata* GetMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpMetadata
    ///
    /// @brief  Dumps the metadata buffer information into a file
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpMetadata();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize internal data structures for the MetadataSlot
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeMetaBuffers
    ///
    /// @brief  Initialize metabuffers for the MetadataSlot
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeMetaBuffers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeAndroidMetadata
    ///
    /// @brief  Initialize android metadata for the MetadataSlot
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeAndroidMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeInternalMetadata
    ///
    /// @brief  Initialize internal metadata for the MetadataSlot
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeInternalMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReadPublished
    ///
    /// @brief  Helper to lookup the published state for a tag/property represented by unitId
    ///
    /// @param  type    Is it a property or metadata?
    /// @param  unitId  Property type or metadata tag
    ///
    /// @return Status of published tracking
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL ReadPublished(
        UnitType    type,
        UINT32      unitId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMetadataPublished
    ///
    /// @brief  Helper to lookup if the metadata is published
    ///
    /// @param  tagID    metadata/property tagID
    ///
    /// @return Status of published tracking
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsMetadataPublished(
        UINT32  tagID)
    {
        BOOL published;

        // check if we are waiting for a tagID which will never get published
        // if (m_pPool->GetPublishSet().find(tagID) == m_pPool->GetPublishSet().end())
        // {
        //    CAMX_LOG_VERBOSE(CamxLogGroupMeta,
        //                     "Warning: Requesting tag %x from pool %d of reqId %llu which will never get published",
        //                     tagID, m_pPool->GetPoolType(), m_slotRequestId);
        // }

        UINT32 tagIndex = HAL3MetadataUtil::GetUniqueIndexByTag(tagID);

        if (MaxMetadataTags > tagIndex)
        {
            published = m_metadataPublishCount[tagIndex];
        }
        else
        {
            published = FALSE;
            CAMX_LOG_ERROR(CamxLogGroupMeta, "Invalid tag %x index %d", tagID, tagIndex);
        }

        return published;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsMetadataPublishedByTagIndex
    ///
    /// @brief  Helper to lookup if the metadata is published given tagIndex
    ///
    /// @param  tagIndex    Unique index of the tag
    ///
    /// @return Status of published tracking
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsMetadataPublishedByTagIndex(
        UINT32  tagIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSlotValid
    ///
    /// @brief  Helper to lookup if the slot is valid
    ///
    /// @return TRUE if the slot is valid, false otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSlotValid();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MetadataSlot
    ///
    /// @brief  Constructor for MetadataSlot object.
    ///
    /// @param  entryCapacity   Entry capacity for each metadata in the slot
    /// @param  dataSize        Data capacity for each metadata in the slot
    /// @param  pPool           Pointer to the pool to which this slot belongs
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MetadataSlot(
        SIZE_T        entryCapacity,
        SIZE_T        dataSize,
        MetadataPool* pPool);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~MetadataSlot
    ///
    /// @brief  Destructor for MetadataSlot object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~MetadataSlot();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetPublishList
    ///
    /// @brief  Resets the publish list
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ResetPublishList();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTagFromStickyMeta
    ///
    /// @brief  Resets the publish list
    ///
    /// @return TRUE if the tag can be fetched from sticky meta buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL GetTagFromStickyMeta();

    // Do not implement the copy constructor, assignment and move operators
    MetadataSlot(const MetadataSlot& rMetadataSlot) = delete;
    MetadataSlot(const MetadataSlot&& rrMetadataSlot) = delete;
    MetadataSlot& operator= (const MetadataSlot& rMetadataSlot) = delete;
    MetadataSlot& operator= (const MetadataSlot&& rrMetadataSlot) = delete;

    UINT64                                              m_slotRequestId;        ///< The request Id corresponding to
                                                                                ///< this slot
    SIZE_T                                              m_dataSize;             ///< Data capacity for each metadata
                                                                                ///< in the slot
    SIZE_T                                              m_entryCapacity;        ///< Entry capacity for each metadata
    Metadata*                                           m_pMetadata;            ///< Underlying Android metadata tied
    VOID*                                               m_pMetaBlob;            ///< Underlying internal metadata tied
    MetadataPool*                                       m_pPool;                ///< Pointer to the pool to which
                                                                                ///< this slot belongs
    BOOL                                                m_isValid;              ///< Whether the slot is valid
    ReadWriteLock*                                      m_pRWLock;              ///< Read-write lock used by pool as
                                                                                ///< well as clients
    MetaBuffer*                                         m_pMetaBuffer;          ///< pointer to the metabuffer
    std::array<std::atomic<BOOL>, MaxMetadataTags>      m_metadataPublishCount; ///< Atomic array to
                                                                                ///  check if metadata is published
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief MetadataPool is an array (pool size = max outstanding requests in pipeline) of MetadataSlot-s, recycled in a
///        circular buffer and indexed by Frame ID (modulo pool size). MetadataPool additionally provides subscription and
///        notification interface to interested modules, for a range of metadata tags and property types.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MetadataPool final
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create an instance of MetadataPool
    ///
    /// @param  poolType            Type of property pool being created
    /// @param  pipelineId          Pipeline ID for the MetadataPool
    /// @param  pThreadManager      Pointer to the thread manager, if its NULL then the creation will not be parallelized
    /// @param  numSlots            Number of slots for the pool being created
    /// @param  pPipelineName       Name of the pipeline who owns the metadata pool
    /// @param  numPrePublishedTags Number of pre-published tags to accommodate the pipeline delay for per frame meta pool
    ///
    /// @return Instance pointer to be returned or NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static MetadataPool* Create(
        PoolType        poolType,
        UINT            pipelineId,
        ThreadManager*  pThreadManager,
        UINT            numSlots,
        const CHAR*     pPipelineName,
        UINT            numPrePublishedTags);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Method to delete an instance of MetadataPool
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPoolStatus
    ///
    /// @brief  Return the status of the pool.
    ///         PoolStaus::None         - if pool is created and before initialization.
    ///         PoolStaus::Initialzied  - if pool is successfully initialized.
    ///         PoolStaus::Error        - if pool initialization is failed.
    ///
    /// @return PoolStatus
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE PoolStatus GetPoolStatus()
    {
        return m_poolStatus;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitForMetadataPoolCreation
    ///
    /// @brief  Wait until pool initialization is completed.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID WaitForMetadataPoolCreation();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSlot
    ///
    /// @brief  Get metadata slot from the pool corresponding to the requestId
    ///         Will typically be called by topology for m_currentFrameId or m_requestId
    ///
    /// @param  requestId Frame Id for which the slot is requested
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE MetadataSlot* GetSlot(
        UINT64 requestId)
    {
        CAMX_ASSERT(NULL != m_pSlots);
        CAMX_ASSERT((NULL == m_pSlots) || (NULL != m_pSlots[requestId % m_numSlots]));
        return m_pSlots[requestId % m_numSlots];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCurrentEarlyTagsCountForSlot
    ///
    /// @brief  Get metadata early tag count for slot from the pool corresponding to the requestId
    ///         Will typically be called by topology for m_currentFrameId or m_requestId
    ///
    /// @param  requestId Frame Id for which the slot is requested
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetCurrentEarlyTagsCountForSlot(
        UINT64 requestId)
    {
        CAMX_ASSERT(NULL != m_aCurrentEarlyTagsPerSlot);
        return m_aCurrentEarlyTagsPerSlot[requestId % m_numSlots];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IncCurrentEarlyTagsCountForSlot
    ///
    /// @brief  Inc metadata early tag count for slot from the pool corresponding to the requestId
    ///         Will typically be called by topology for m_currentFrameId or m_requestId
    ///
    /// @param  requestId Frame Id for which the slot is requested
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID IncCurrentEarlyTagsCountForSlot(
        UINT64 requestId)
    {
        CAMX_ASSERT(NULL != m_aCurrentEarlyTagsPerSlot);
        CamxAtomicIncU(&m_aCurrentEarlyTagsPerSlot[requestId % m_numSlots]);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ResetCurrentEarlyTagsCountForSlot
    ///
    /// @brief  Reset  metadata early tag count fir slot from the pool corresponding to the requestId
    ///         Will typically be called by topology for m_currentFrameId or m_requestId
    ///
    /// @param  requestId Frame Id for which the slot is requested
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID ResetCurrentEarlyTagsCountForSlot(
        UINT64 requestId)
    {
        CAMX_ASSERT(NULL != m_aCurrentEarlyTagsPerSlot);
        CamxAtomicStoreU32(&m_aCurrentEarlyTagsPerSlot[requestId % m_numSlots], 0);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Subscribe
    ///
    /// @brief  A client subscribes to a list of properties or tags.
    ///
    /// @note   MUST be done before updates could come in. List not lock protected
    ///
    /// @param  pSubscribe  List (can be one) of properties/tags this client subscribes to
    /// @param  count       Number of properties/tags being subscribed to
    /// @param  pClient     Pointer to client (implements the IObserver interface)
    /// @param  pClientName String for subscriber
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Subscribe(
        const Subscription*     pSubscribe,
        UINT32                  count,
        IPropertyPoolObserver*  pClient,
        const CHAR*             pClientName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SubscribeAll
    ///
    /// @brief  Subscribes an observer for all defined properties/tags
    ///
    /// @note   MUST be done before updates could come in. List not lock protected
    ///
    /// @param  pClient     Observer to subscribe
    /// @param  pClientName String for subscriber
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SubscribeAll(
        IPropertyPoolObserver*  pClient,
        const CHAR*             pClientName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnsubscribeAll
    ///
    /// @brief  Unsubscribes and removes an observer from the subscription list in the property pool.
    ///
    /// @note   MUST be done after all updates done. Lists not lock protected
    ///
    /// @param  pClient Observer to detach/remove
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UnsubscribeAll(
        IPropertyPoolObserver* pClient);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Invalidate
    ///
    /// @brief  Invalidate the slot corresponding to this request Id
    ///
    /// @param  requestId   The request Id corresponding to which the slot will be invalidated
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Invalidate(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateRequestId
    ///
    /// @brief  Update the slot request Id to this request Id
    ///
    /// @param  requestId   The slot will be assigned to this request Id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateRequestId(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief  Invalidates the contents of every slot in the property pool
    ///
    /// @param  lastValidRequestId  Last request Id containing the valid metadata
    /// @param  lastRequestId       Last requestId before the flush is issued
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Flush(
       UINT64 lastValidRequestId,
       UINT64 lastRequestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Lock
    ///
    /// @brief  Lock the metadata pool
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Lock() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Unlock
    ///
    /// @brief  Unlock the metadata pool
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Unlock() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdatePublishSet
    ///
    /// @brief  sets the publish list for the pool
    ///
    /// @return CamxResult enumeration
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdatePublishSet(
        const std::unordered_set<UINT32>& publishSet);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MetadataPool
    ///
    /// @brief  Constructor for MetadataPool object.
    ///
    /// @param  poolType            Type of property pool being created
    /// @param  pipelineId          Pipeline ID for the MetadataPool
    /// @param  pThreadManager      Pointer to the thread manager
    /// @param  numSlots            Number of slots for the pool being created
    /// @param  pPipelineName       Name of the pipeline who owns the metadata pool
    /// @param  numPrePublishedTags Number of pre-published tags to accommodate the pipeline delay for per frame meta pool
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit MetadataPool(
        PoolType        poolType,
        UINT            pipelineId,
        ThreadManager*  pThreadManager,
        UINT            numSlots,
        const CHAR*     pPipelineName,
        UINT            numPrePublishedTags);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~MetadataPool
    ///
    /// @brief  Destructor for MetadataPool object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~MetadataPool();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize internal data structures for the MetadataPool
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PoolTypeIndex
    ///
    /// @brief  Return pooltype index
    ///
    /// @return pooltype index
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 PoolTypeIndex() const
    {
        return static_cast<UINT32>(m_poolType);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyImmediate
    ///
    /// @brief  Notifies all subscribed observers of a particular property or metadata tag, about success or failure of
    ///         the property/tag being published
    ///
    /// @param  unitId          The actual property or tag to be notified about
    /// @param  tagIndex        Index of the tag
    /// @param  slotRequestId   The request Id of the slot in the pool being published for
    /// @param  isSuccess       Whether the publish is successful or failed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyImmediate(
        UINT32      unitId,
        UINT32      tagIndex,
        UINT64      slotRequestId,
        BOOL        isSuccess);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyClient
    ///
    /// @brief  Notifies the observer of a particular property or metadata tag, about success or failure of
    ///         the property/tag being published
    ///
    /// @param  pObserver       Observer for the metadata
    /// @param  unitId          The actual property or tag to be notified about
    /// @param  slotRequestId   The request Id of the slot in the pool being published for
    /// @param  isSuccess       Whether the publish is successful or failed
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyClient(
        IPropertyPoolObserver* pObserver,
        UINT32                 unitId,
        UINT64                 slotRequestId,
        BOOL                   isSuccess);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MetadataPoolThreadCb
    ///
    /// @brief  Thread callback function
    ///
    /// @param  pArg Pointer to callback data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* MetadataPoolThreadCb(
        VOID* pArg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPoolType
    ///
    /// @brief  Fetch the pool type of the property pool
    ///
    /// @return Pool type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE PoolType GetPoolType() const
    {
        return m_poolType;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPoolIdentifier
    ///
    /// @brief  Fetch the pool type string of the property pool
    ///
    /// @return String used to identify this pool
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const CHAR* GetPoolIdentifier() const
    {
        return &m_poolIdentifier[0];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumOfPrePublishedTags
    ///
    /// @brief  Fetch the number of pre-published tags for PoolType::PerFrameResult
    ///
    /// @return Number of pre-published tags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT GetNumOfPrePublishedTags() const
    {
        return m_numPrePublishedTags;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LockMetadataSubscribers
    ///
    /// @brief  Lock all subscribers to ensure coherency of update
    ///
    /// @param  tagIndex identifier for MetadataTag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID LockMetadataSubscribers(
        UINT32 tagIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnlockMetadataSubscribers
    ///
    /// @brief  Unlock all subscribers
    ///
    /// @param  tagIndex identifier for MetadataTag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UnlockMetadataSubscribers(
        UINT32 tagIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UseExternalBuffers
    ///
    /// @brief  Check if the pool uses external buffers
    ///
    /// @return true/false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL UseExternalBuffers() const
    {
        BOOL useDetachedBuffers;
        if ((PoolType::PerFrameInput  == m_poolType) ||
            (PoolType::PerFrameResult == m_poolType))
        {
            useDetachedBuffers = TRUE;
        }
        else
        {
            useDetachedBuffers = FALSE;
        }
        return useDetachedBuffers;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UseMetaBuffers
    ///
    /// @brief  Check if the pool uses metabuffers
    ///
    /// @return true/false
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL UseMetaBuffers() const
    {
        BOOL useMetaBuffers;
        if (PoolType::PerFrameInput  != m_poolType &&
            PoolType::PerFrameResult != m_poolType &&
            PoolType::PerUsecase     != m_poolType)
        {
            useMetaBuffers = FALSE;
        }
        else
        {
            useMetaBuffers = TRUE;
        }
        return useMetaBuffers;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPublishSet
    ///
    /// @brief  get the publish list for the pool
    ///
    /// @return set of publish set
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE const std::unordered_set<UINT32>& GetPublishSet()
    {
        return m_publishTagSet;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLastFlushRequestId
    ///
    /// @brief  Get the requestId of the last Flush call
    ///
    /// @return the Request Id of the last flush call
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT64 GetLastFlushRequestId()
    {
        return m_lastFlushRequestId;
    }

    // Do not implement the copy constructor or assignment operator
    MetadataPool(const MetadataPool& rMetadataPool) = delete;
    MetadataPool(const MetadataPool&& rrMetadataPool) = delete;
    MetadataPool& operator= (const MetadataPool& rMetadataPool) = delete;
    MetadataPool& operator= (const MetadataPool&& rrMetadataPool) = delete;

    UINT m_aCurrentEarlyTagsPerSlot[MaxPerFramePoolWindowSize];  ///< Array of counts for a given
                                                                 ///  request of tags that need to
                                                                 ///  be returned early..must be
                                                                 ///  accessed atomically.

    PoolType                        m_poolType;                             ///< Type of the property pool
    CHAR                            m_poolIdentifier[MaxTraceStringLength]; ///< String used for tracing activity with this pool
    SIZE_T                          m_slotMetadataEntryCount;               ///< Metadata entry count in each slot
    SIZE_T                          m_slotMetadataDataSize;                 ///< Metadata data size in each slot
    UINT                            m_numSlots;                             ///< Number of slots that this pool contains
    Mutex*                          m_pPoolLock;                            ///< Lock for the Pool
    UINT                            m_pipelineId;                           ///< Pipeline ID for this Pool. -1 if not
                                                                            ///< belong to Pipeline.
    const CHAR*                     m_pipelineName;                          ///< Pipleine Name for this Pool.
    MetadataSlot*                   m_pSlots[MaxPerFramePoolWindowSize];    ///< Array of slots in the Pool

    // NOWHINE CP006: optimized linklist from std library
    std::list<SubscriptionEntry>    m_pMetadataClients[MaxMetadataTags];    ///< individual metadata subsribers
    SubscriptionEntry               m_subscribeAllClients[MaxSubscribers];  ///< Array of subscribers
    Mutex*                          m_pSubscriptionLock;                    ///< Lock protecting access to subscriber arrays
    ThreadManager*                  m_pThreadManager;                       ///< Pointer to thread manager
    JobHandle                       m_hThread;                              ///< Thread handle
    Condition*                      m_pMetadataPoolCreateWait;              ///< Wait till Metadata pool is avaiable
    Mutex*                          m_pMetadataPoolCreateLock;              ///< Metadata pool mutex lock
    PoolStatus                      m_poolStatus;                           ///< Indicate if deferred Metadata pool creation
                                                                            ///< is done
    MetaBuffer*                     m_pStickyMetaBuffer;                    ///< Pointer to the metabuffer
    std::unordered_set<UINT32>      m_publishTagSet;                        ///< Publish list from the pipeline
    UINT64                          m_lastFlushRequestId;                   ///< requestId since the last flush
    UINT64                          m_prevlastValidRequestId;               ///< previous m_lastValidRequestId
    UINT                            m_numPrePublishedTags;                  ///< Number of pre-published tag. This is valid
                                                                            ///  for PoolType::PerFrameResult only.

    // NOWHINE CP039: Public access permitted for the metadataslot since the class is part of the metadatapool interface.
    friend class MetadataSlot;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MetadataSlot::IsSlotValid
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BOOL MetadataSlot::IsSlotValid()
{
    BOOL isValid;

    if ((PoolType::PerFrameResult == m_pPool->GetPoolType()) &&
        (m_slotRequestId < (m_pPool->GetLastFlushRequestId() + m_pPool->GetNumOfPrePublishedTags())))
    {
        isValid = TRUE;
    }
    else
    {
        isValid = m_isValid;
    }

    return isValid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MetadataSlot::GetTagFromStickyMeta
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE BOOL MetadataSlot::GetTagFromStickyMeta()
{
    return ((NULL != m_pPool->m_pStickyMetaBuffer) &&
            (PoolType::PerFrameResult == m_pPool->GetPoolType()) &&
            (m_slotRequestId < (m_pPool->GetLastFlushRequestId() + m_pPool->GetNumOfPrePublishedTags())));
}

CAMX_NAMESPACE_END

#endif // CAMXMETADATAPOOL_H
