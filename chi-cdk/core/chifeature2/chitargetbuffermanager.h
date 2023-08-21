////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chitargetbuffermanager.h
/// @brief CHI CHITargetBufferManager class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE NC004c: CHI files wont be having Camx
// NOWHINE FILE CP006:  Need vector for returning list
// NOWHINE FILE CP021:  Callback if NULL by default


#ifndef CHITARGETBUFFERMANAGER_H
#define CHITARGETBUFFERMANAGER_H

#include "cdkutils.h"
#include "chxdefs.h"
#include "chxusecaseutils.h"

/// @brief Function to callback when buffer available
typedef CDKResult(*PFNTARGETCALLBACK)(
    CHITARGETBUFFERINFOHANDLE hBuffer,
    VOID*                     pPrivateData);

/// @brief Forward declartion of class
class CHITargetBufferManager;

/// @brief Status of each target
enum class ChiTargetStatus
{
    NotReady,           ///< Target is not ready
    Ready,              ///< Target is ready
    Error,              ///< Target is in error state
};
/// @brief Data needed to created a CHITargetBufferManager instance
struct ChiTargetBufferManagerCreateData
{
    const CHAR*                 pTargetBufferName;                          ///< The string identifier of the target
                                                                            ///  buffer object.
    UINT                        numOfMetadataBuffers;                       ///< The number of metadata target.

    UINT                        minMetaBufferCount;                         ///< The Minimun number of buffers to trigger
                                                                            ///  recycle
    UINT                        maxMetaBufferCount;                         ///< The Maximum number of buffers to be allocated
    UINT64                      metadataIds[MaxChiStreams];                 ///< ID of each metadata target.
                                                                            ///  It must be identical to the metadata client id
                                                                            ///  of the pipeline that registered to the
                                                                            ///  metadata manager.
    ChiMetadataManager*         pChiMetadataManager;                        ///< Pointer to the metadata manager.

    UINT                        numOfInternalStreamBuffers;                 ///< The number of internal stream buffer target.
    UINT64                      internalStreamIds[MaxChiStreams];           ///< ID of each internal stream buffer target.
    const CHAR*                 pBufferManagerNames[MaxChiStreams];         ///< Buffer manager string identifier.
    CHIBufferManagerCreateData* pBufferManagerCreateData[MaxChiStreams];    ///< Buffer manager data.

    UINT                        numOfExternalStreamBuffers;                 ///< The number of external stream buffer target.
    UINT                        minExternalBufferCount;                     ///< The minimum number of external buffers
    UINT                        maxExternalBufferCount;                     ///< The Maximum number of external buffers
    UINT64                      externalStreamIds[MaxChiStreams];           ///< ID of each external stream buffer target.
    BOOL                        isChiFenceEnabled;                          ///< Whether to enable chi fence on this TBM.
};

/// @brief Search options used by SearchList API
enum class SearchOption
{
    SearchProducerList,                 ///< Producer linked list
    SearchConsumerList,                 ///< Consumer linked list
    SearchProducerAndConsumerList,      ///< Both producer and consumer linked list
    SearchImportList                    ///< Import linked list
};

struct ChiTargetBufferCallbackData
{
    PFNTARGETCALLBACK pCallback;        ///< Registered callback function for when buffer is available
    VOID*             pPrivateData;     ///< Client Private Data provided with callback
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief CHITargetBufferManager Class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC CHITargetBufferManager
{
public:

    /// @brief Type of each target
    enum class ChiTargetType
    {
        Metadata,           ///< Metadata target
        InternalBuffer,     ///< Internal stream buffer target, e.g. RDI, FD, etc.
        ExternalBuffer,     ///< External stream buffer target, e.g. fwk preview, video, etc.
    };

    /// @brief ChiTargetBufferInfo node struct
    struct ChiTargetBufferInfo
    {
        UINT32                       seqId;                              ///< Pipeline sequence ID.
        UINT                         refCount;                           ///< Reference count to this node.
        ChiMetadata*                 pMetadata[MaxChiStreams];           ///< Metadata pointer per target.
        CHISTREAMBUFFER              chiStreamBuffer[MaxChiStreams];     ///< Chi stream buffer per target.
        ChiTargetStatus              targetStatus[MaxChiStreams];        ///< Status of each target.
        CHITargetBufferManager*      pCHITargetBufferManager;            ///< Pointer to manager who setup buffer.
        BOOL                         isImported;                         ///< Flag to indicate if this target buffer is imported
        ChiTargetBufferCallbackData  callbackData;                       ///< CallbackData
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create an instance of this class
    ///
    /// @param  pCreateData  data needed to create class
    ///
    /// @return CHITargetBufferManager*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHITargetBufferManager* Create(
        ChiTargetBufferManagerCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy an instance of the class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupTargetBuffer
    ///
    /// @brief  Setup target buffer and return a TargetBufferInfo handle
    ///
    /// @param  seqId  Id for requested target buffer
    ///
    /// @return CHITARGETBUFFERINFOHANDLE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHITARGETBUFFERINFOHANDLE SetupTargetBuffer(
        UINT32 seqId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTarget
    ///
    /// @brief  Get one target given the TargetBufferInfo handle and target ID
    ///
    /// @param  hTargetBufferInfo  Info to get target buffer
    /// @param  targetId           Id to find correct target from buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetTarget(
        CHITARGETBUFFERINFOHANDLE   hTargetBufferInfo,
        UINT64                      targetId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateTarget
    ///
    /// @brief  Update one target to ready status
    ///
    /// @param  seqId              Id for requested target buffer
    /// @param  targetId           Id to find correct target from buffer
    /// @param  pSrcTarget         Source target to be updated
    /// @param  status             The status of the target
    /// @param  phTargetBufferInfo Info to get target buffer
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult UpdateTarget(
        UINT32                     seqId,
        UINT64                     targetId,
        VOID*                      pSrcTarget,
        ChiTargetStatus            status,
        CHITARGETBUFFERINFOHANDLE* phTargetBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCHIHandle
    ///
    /// @brief  Get CHITARGETBUFFERINFOHANDLE without Updating Target
    ///
    /// @param  seqId               Id for requested target buffer
    /// @param  targetId            Id to find correct target from buffer
    /// @param  phTargetBufferInfo  Handle to target buffers
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetCHIHandle(
        UINT32                      seqId,
        UINT64                      targetId,
        CHITARGETBUFFERINFOHANDLE*  phTargetBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTargetBuffers
    ///
    /// @brief  Get a number of TargetBuffer handles.
    ///         By default, this is a blocking call - It waits for all the target buffers become ready and then return handles.
    ///         It is a non-blocking call when all following conditions are satisfied, and target buffers will be returned
    ///         with acquire fences:
    ///         1) Chi fence is enabled.
    ///         2) Target does not contain metatdata.
    ///         3) waitFlag is explicitly set to FALSE by client.
    ///         Alternatively it could also be a non-blocking call if a callback function is provided. In this case, client will
    ///         get a callback when the buffer becomes available
    ///
    /// @param  startSeqId              Id for requested target buffer
    /// @param  numBuffers              Number of buffers requested
    /// @param  phTargetBufferInfos     Handle to target buffers
    /// @param  waitFlag                A flag indicates whether to wait for the targets become ready before return.
    /// @param  pCallbackData           Optional parameter to provide callback when buffer is available

    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetTargetBuffers(
        UINT32                        startSeqId,
        UINT32                        numBuffers,
        CHITARGETBUFFERINFOHANDLE*    phTargetBufferInfos,
        BOOL                          waitFlag,
        ChiTargetBufferCallbackData*  pCallbackData = NULL);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseTargetBuffer
    ///
    /// @brief  Release reference to a TargetBuffer
    ///
    /// @param  hTargetBufferInfo  Handle to target buffer
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ReleaseTargetBuffer(
        CHITARGETBUFFERINFOHANDLE   hTargetBufferInfo);

    /// Import external chi stream buffer target into the object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ImportExternalTargetBuffer
    ///
    /// @brief  Setup target buffer and return a TargetBufferInfo handle
    ///
    /// @param  seqId             Id for requested target buffer
    /// @param  targetId          Id to find correct target from buffer
    /// @param  pTarget           Pointer to external target
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ImportExternalTargetBuffer(
        UINT32              seqId,
        UINT64              targetId,
        VOID*               pTarget);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemoveCallbackFromTarget
    ///
    /// @brief  Remove a callback from Target
    ///
    /// @param  seqId  Id for requested target buffer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RemoveCallbackFromTarget(
        UINT32 seqId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemoveTargetBuffer
    ///
    /// @brief  Remove a TargetBuffer from the object
    ///
    /// @param  seqId  Id for requested target buffer
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult RemoveTargetBuffer(
        UINT32 seqId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFirstReadySequenceID
    ///
    /// @brief  Get the sequence id of the first ready TargetBuffer
    ///
    /// @return FirstReadySequenceID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetFirstReadySequenceID();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLastReadySequenceID
    ///
    /// @brief  Get the sequence id of the last ready TargetBuffer
    ///
    /// @return LastReadySequenceID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetLastReadySequenceID();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTargetBufferStatus
    ///
    /// @brief  Get the status of the TargetBuffer
    ///
    /// @param  pTargetBufferInfo   Pointer to TargetBufferInfo structure
    ///
    /// @return ChiTargetStatus
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiTargetStatus GetTargetBufferStatus(
        ChiTargetBufferInfo* pTargetBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTargetBufferManager
    ///
    /// @brief  Get the pointer to the buffer manager that set up a given buffer
    ///
    /// @param  hTargetBufferInfo  Handle of buffer manager
    ///
    /// @return CHITargetBufferManager* pointer to the buffer manager that setup buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CHITargetBufferManager* GetTargetBufferManager(
        CHITARGETBUFFERINFOHANDLE   hTargetBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSequenceId
    ///
    /// @brief  Get the sequenceId from target buffer info handle
    ///
    /// @param  hTargetBufferInfo  Handle of buffer manager
    ///
    /// @return UINT32 sequenceId of the target buffer info handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static UINT32 GetSequenceId(
        CHITARGETBUFFERINFOHANDLE   hTargetBufferInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAllSequenceId
    ///
    /// @brief  Get List of all currently ready sequenceIds
    ///
    /// @param  option  Option to decide which list to search
    ///
    /// @return std::vector<UINT32> List of all requested sequence Id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    std::vector<UINT32> GetAllSequenceId(
        SearchOption option);

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CHITargetBufferManager
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHITargetBufferManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CHITargetBufferManager
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~CHITargetBufferManager() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize the instanceGet the pointer to the buffer manager that set up a given buffer
    ///
    /// @param  pCreateData  pointer to data needed to create TargetBufferManager
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        ChiTargetBufferManagerCreateData* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateTargetBufferInfoNode
    ///
    /// @brief  Allocate memory for a node and TargetBufferInfo struct
    ///
    /// @return A pointer to node if success, or null pointer if failed
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    LightweightDoublyLinkedListNode* AllocateTargetBufferInfoNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LookupTargetIndex
    ///
    /// @brief  Find the target index given targetId
    ///
    /// @param  targetId  Id of target used to find index
    ///
    /// @return Index of target
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 LookupTargetIndex(
        UINT64 targetId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TryMoveToConsumerList
    ///
    /// @brief  Try to move ChiTargetBufferInfo node from producer to consumer list
    ///
    /// @param  seqId   Id of TargetBuffer to search
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID TryMoveToConsumerList(
        UINT32 seqId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SearchList
    ///
    /// @brief  Search lists based on sequence id
    ///
    /// @param  seqId   Id of TargetBuffer to search
    /// @param  option  Option to decide which list to search
    ///
    /// @return LightweightDoublyLinkedListNode* pointer to the TargetBuffer node
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    LightweightDoublyLinkedListNode* SearchList(
        UINT32          seqId,
        SearchOption    option);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemoveTargetBufferPrivate
    ///
    /// @brief  Remove a TargetBuffer from the object
    ///
    /// @param  seqId  Id of TargetBuffer to remove
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult RemoveTargetBufferPrivate(
        UINT32 seqId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RecycleTargetBuffersFromConsumerList
    ///
    /// @brief  Recycle TargetBuffer based on criteria
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RecycleTargetBuffersFromConsumerList();

    CHITargetBufferManager(const CHITargetBufferManager&)             = delete;    ///< Disallow the copy constructor
    CHITargetBufferManager& operator= (const CHITargetBufferManager&) = delete;    ///< Disallow assignment operator

    CHAR                m_targetBufferName[MaxStringLength64];  ///< Name of the Target Buffer instance.
    UINT                m_numOfTargets;                         ///< Total number of targets managed by this object.
    UINT64              m_targetIds[MaxChiStreams];             ///< ID of each target.
    ChiTargetType          m_targetType[MaxChiStreams];            ///< Type of each target.
    const ChiStream*    m_pChiStreams[MaxChiStreams];           ///< Array of pointers to each target.
                                                                ///  Applicaple to InternalBuffer target.
    ChiMetadataManager* m_pMetadataManager;                     ///< Pointer to the metadata manager.
    CHIBufferManager*   m_pBufferManagers[MaxChiStreams];       ///< Array of buffer managers of each target.
                                                                ///  Applicaple to InternalBuffer target.
    UINT                m_minNumOfBuffers;                      ///< The minimum number of nodes in producer and consumer lists
    UINT                m_maxNumOfBuffers;                      ///< The total number of nodes that producer and consumer lists
                                                                ///  should not exceed.
    static const UINT   DefaultMaxNode = 32;                    ///< Default max number of nodes.
    Mutex*              m_pLock;                                ///< Mutex protects accessing to the object.
    Condition*          m_pCondition;                           ///< Condition waits when buffer or metadata is not ready.
    BOOL                m_isChiFenceEnabled;                    ///< Boolean indicates if chi fence is enabled on this TBM.

    LightweightDoublyLinkedList*    m_pImportList;      ///< Import list manages external targets from framework.
    LightweightDoublyLinkedList*    m_pProducerList;    ///< Producer list manages targets that are being produced.
    LightweightDoublyLinkedList*    m_pConsumerList;    ///< Consumer list manages targets that are ready or being consumed.
};


#endif // CHITARGETBUFFERMANAGER_H
