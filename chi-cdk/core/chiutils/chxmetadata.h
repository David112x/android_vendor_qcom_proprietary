////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxmetadata.h
/// @brief CHX Metadata class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXMETADATA_H
#define CHXMETADATA_H

#include <assert.h>

#include "chi.h"
#include "chioverride.h"
#include "camxcdktypes.h"
#include "chxextensionmodule.h"
#include <array>
#include <mutex>
#include <unordered_set>
#include <vector>

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

// Forward decl
class ChiMetadata;

// type definitions
typedef std::chrono::steady_clock             CMBTime;
typedef std::chrono::steady_clock::time_point CMBTimePoint;

/// Static definitions
static const UINT32 ChxInfiniteTimeout = UINT32_MAX;

/// Usage type of the metadata
enum ChiMetadataUsage
{
    Generic,        ///< Any general purpose metadata
    FrameworkInput, ///< Metadata corresponding to framework input
    RealtimeOutput, ///< Metadata corresponding to realtime output
    OfflineOutput,  ///< Metadata corresponding to offline output
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief ChiMetadata Manager
///
/// @brief Metadata Manager implementation class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiMetadataManager
{
public:
    static const UINT32 ChxMaxMetadataClients    = 64;                         ///< Mamimum number of metadata clients
                                                                               ///  it is fine to increase value as requirements as
                                                                               ///  the related holders are small
    static const UINT32 InvalidClientId          = 0xFFFFFFFF;                 ///< Invalid client Identifier
    static const UINT32 AndroidFrameworkClientId = 0;                          ///< Reserved Client identifier for android
                                                                               ///  input

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Creates an instance of metadata manager. Its recommended to instantiate one metadata manager per usecase
    ///
    /// @param inputFps Input frame rate associated with this instance of manager
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiMetadataManager* Create(
        UINT32 inputFps = 30);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroys the instance of metadata manager
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeFrameworkInputClient
    ///
    /// @brief  Initializes android framework input client to the Metadata manager. Framework input client contains the
    ///         tags which are exposed to the framework client. This function must be called only once per metadata manager
    ///         instance
    ///
    /// @param  bufferCount            Number of buffers that needs to be allocated
    /// @param  bSupportMultipleInputs Flag to indicate whether to support multiple outputs
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult InitializeFrameworkInputClient(
        UINT32 bufferCount,
        bool   bSupportMultipleInputs = false);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RegisterClient
    ///
    /// @brief  Registers pipeline output metadata client with the metadata manager.
    ///
    /// @param  isExclusive Flag to indicate whether client needs an exclusive access to the buffers or shared access. It is
    ///                     recommended to have realtime clients registered as exclusive clients and non-real time as shared
    ///                     clients
    /// @param  pTagList    Taglist provided by the pipeline using the QueryPipelineInfo() API. CHI client can add additional
    ///                     tags
    /// @param  tagCount    Number of tags present in the tag list
    /// @param  bufferCount Number of buffers that needs to be allocated
    /// @param  usage       Usage type of the client.
    ///
    /// @return 32-bit identifier. InvalidClientId is returned in case of failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 RegisterClient(
        BOOL             isExclusive,
        UINT32*          pTagList,
        UINT32           tagCount,
        UINT32           partialTagCount,
        UINT32           bufferCount,
        ChiMetadataUsage usage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UnregisterClient
    ///
    /// @brief  Unregisters pipeline output metadata client from the metadata manager.
    ///
    /// @param  clientId    Identifier of the client to be registered
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult UnregisterClient(
        UINT32  clientId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief  Flush metadata for all the clients
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Flush();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPipelineId
    ///
    /// @brief  Sets the pipeline Id of the client. This is an optional API for debug purpose
    ///
    /// @param  clientId    Identifier of the client to be registered
    /// @param  pipelineId   Identifier of the pipeline corresponding to the client
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetPipelineId(
        UINT32 clientId,
        UINT32 pipelineId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Get
    ///
    /// @brief  Gets the free chimetadata object specific to the client Id
    ///
    /// @param  clientId    Identifier of the client from which the metadata must be obtained
    /// @param  frameNumber Frame number corresponding to the metadata. this is optionally added for debug purpose
    ///
    /// @return Pointer to the ChiMetadata object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiMetadata* Get(
        UINT32 clientId,
        UINT32 frameNumber);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInput
    ///
    /// @brief  Gets the free chimetadata object given the framework settings
    ///
    /// @param  pFrameworkInput  Input metadata from framework
    /// @param  frameNumber      Frame number corresponding to the metadata. this is optionally added for debug purpose
    /// @param  bUseSticky       Flag to indicate if the metadata should contain previous input tags which are not present in
    ///                          the current request
    /// @param  bReuseBuffers    Flag to indicate if the metadata should contain previous input tags
    ///
    /// @return Pointer to the ChiMetadata object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiMetadata* GetInput(
        const camera_metadata_t* pFrameworkInput,
        UINT32                   frameNumber,
        bool                     bUseSticky = true,
        bool                     bReuseBuffers = true);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAndroidFrameworkOutputMetadata
    ///
    /// @brief  Gets the free android camera metadata structure given the framework settings
    ///
    /// @param  sparseMetadata Specifies if the metadata object needs to be sparse or not. Sparse metadata should be only used
    ///                        to send empty metadata with timestamps (eg: HFR usecase)
    ///
    /// @return Pointer to the camera_metadata_t structure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    camera_metadata_t* GetAndroidFrameworkOutputMetadata(
       bool sparseMetadata = false);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Release
    ///
    /// @brief  Releases the chi metadata object back to the metadata pool
    ///
    /// @param  pMetadata Pointer to the CHI metadata object
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Release(
        ChiMetadata* pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseAndroidFrameworkOutputMetadata
    ///
    /// @brief  Releases the camera_metadata structure back to the metadata pool
    ///
    /// @param  pMetadata Pointer to the android metadata structure
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ReleaseAndroidFrameworkOutputMetadata(
        const camera_metadata_t* pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataFromHandle
    ///
    /// @brief  Obtains the CHI Metadata object given the handle
    ///
    /// @param  hMetaHandle Metadata handle
    ///
    /// @return Pointer to the ChiMetadata object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiMetadata* GetMetadataFromHandle(
        CHIMETADATAHANDLE hMetaHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintAllBuffers
    ///
    /// @brief  Prints all metadata buffer information specific to the client
    ///
    /// @param  hMetaHandle Metadata handle
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult PrintAllBuffers(
        UINT32  clientId = AndroidFrameworkClientId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrievePartialTags
    ///
    /// @brief  Gets the Partial Tag pointer
    ///
    /// @param  clientId of client to be accessed
    ///
    /// @return pointer partial tags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32* RetrievePartialTags(
        UINT clientId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RetrievePartialTagCount
    ///
    /// @brief  Gets the count of Partial Tags
    ///
    /// @param  clientId of client to be accessed
    ///
    /// @return count of partial tags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32  RetrievePartialTagCount(
        UINT clientId);

private:
    /// Does not support the copy constructor or assignment operator
    ChiMetadataManager(const ChiMetadataManager& pOther) = delete;
    ChiMetadataManager(const ChiMetadataManager&& pOther) = delete;
    ChiMetadataManager& operator=(const ChiMetadataManager& pOther) = delete;
    ChiMetadataManager& operator=(const ChiMetadataManager&& pOther) = delete;

    /// Internal function to get the free holder given the client and subclient index
    ChiMetadata* GetFreeHolder(
        UINT32        index,
        UINT32        subIndex,
        UINT32        frameNumber,
        CMBTimePoint& timePoint);

    /// Internal function to release holder given the client and subclient index
    CDKResult ReleaseHolder(
        UINT32       index,
        UINT32       subIndex,
        ChiMetadata* pMetadata);

    /// Internal function to release holder given the client and subclient index
    CDKResult ReleaseHolderByHandle(
        UINT32            index,
        UINT32            subIndex,
        CHIMETADATAHANDLE hMetadataHandle);

    /// Internal function for initialization
    CDKResult Initialize();

    /// Constructor
    ChiMetadataManager(
        UINT32 clientId,
        UINT32 inputFps);

    /// Destructor
    ~ChiMetadataManager();

    /// Registers realtime pipeline output client
    UINT32 RegisterExclusiveClient(
        UINT32*          pTagList,
        UINT32           tagCount,
        UINT32           partialTagCount,
        UINT32           bufferCount,
        ChiMetadataUsage usage);

    /// Registers offline pipeline output client
    UINT32 RegisterSharedClient(
        UINT32*          pTagList,
        UINT32           tagCount,
        UINT32           partialTagCount,
        UINT32           bufferCount,
        ChiMetadataUsage usage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RestoreNonstickyMetadata
    ///
    /// @brief  Restore the original tags which came from framework in ChiMetadata
    ///
    /// @param  pFrameworkInput Input metadata from framework
    /// @param  pInputMetadata  Input Chimetadata from Metadata manager
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID RestoreNonstickyMetadata(
        const camera_metadata_t* pFrameworkInput,
        ChiMetadata*             pInputMetadata);

    // print buffers
    CDKResult PrintAllBuffers(
        UINT32       index,
        UINT32       subIndex);

    /// Class to represent a metadata client
    struct MetaClient
    {
        /// Constructor
        MetaClient();

        /// Destructor
        ~MetaClient();

        /// Release all the buffers
        CDKResult ReleaseBuffers();

        /// Allocate buffers
        CDKResult AllocateBuffers(
            UINT32* pTags,
            UINT32  tagCount,
            UINT32  partialTagCount,
            UINT32  bufferCount,
            UINT32  clientId);

        // Track the buffers and report the ones that are not released within the threshold time
        VOID TrackBuffers(
            CMBTimePoint& currentTimePoint,
            UINT32        timeoutValue);

        /// Class to represent a metadata sub-client
        struct SubClient
        {
            /// Constructor
            SubClient();

            UINT16 pipelineId; ///< Pipeline index
            bool   isUsed;     ///< Flag to indicate if the client is used
        };

        /// Class to represent a metadata holder
        struct MetaHolder
        {
            /// Constructor
            MetaHolder(
                ChiMetadata* pArgMetadata);

            ChiMetadata* pMetadata; ///< Pointer to metadata object
            bool         isFree;    ///< Flag to indicate whether the holder is free
        };

        std::vector<MetaHolder>                      m_bufferList;      ///< Vector of buffers
        std::array<SubClient, ChxMaxMetadataClients> m_subClient;       ///< List of sub clients
        ChiMetadataUsage                             m_type;            ///< Usage of the metadata
        std::mutex                                   m_lock;            ///< Lock for vector access
        bool                                         m_isUsed;          ///< Flag to check if the client is free
        UINT32                                       m_prevBufIndex;    ///< Index of the previous buffer
        bool                                         m_isShared;        ///< Is shared client
        std::unordered_set<UINT32>                   m_tagSet;          ///< Set of tags published
        UINT32                                       m_clientIndex;     ///< Index of this client
        UINT32*                                      m_pPartialTag;     ///< Pointer to Partial Tags
        UINT32                                       m_PartialTagCount; ///< Count of Partial Tags

        // Used to metadata manipulation
        friend class ChiMetadata;
    };

    struct AndroidMetadataHolder
    {
        // constructor
        AndroidMetadataHolder();

        // constructor
        AndroidMetadataHolder(
            camera_metadata_t* pArgMetadata,
            bool               argIsUsed,
            bool               argIsEmpty);

        // destructor
        ~AndroidMetadataHolder();

        camera_metadata_t* pMetadata; ///< Pointer to the android metadata
        bool               isUsed;    ///< Flag to indicate whether the metadata holder
        bool               isSparse;  ///< Sparse metadata with timestamps
    };

    /// Function to check if the meta client is free
    static bool IsFreeClient(
        const MetaClient& client);

    /// Function to check if the meta client is an offline client
    static bool IsOfflineClient(
        const MetaClient& client);

    /// Function to check if the meta client is an realtime client
    static bool IsRealtimeClient(
        const MetaClient& client);

    /// Function to check if the sub client is free
    static bool IsFreeSubClient(
        const MetaClient::SubClient& client);

    /// Private function to allocate framework metadata
    CDKResult AllocateFrameworkMetadata(
        UINT32 entryCapacity,
        UINT32 dataCapacity,
        bool   isSparse,
        UINT32 startIndex,
        UINT32 endIndex);

    // Tracker for unreleased buffers
    VOID TrackBuffers(
        CMBTimePoint& timePoint);

    ChiMetadata*                                  m_stickyInput;      ///< Sticky input buffer
    std::array<MetaClient, ChxMaxMetadataClients> m_clients;          ///< Array of clients
    CHIMETADATAOPS                                m_metadataOps;      ///< Metadata operations
    std::vector<CHIMETADATAENTRY>                 m_metadataTable;    ///< Metadata table
    UINT32                                        m_cameraId;         ///< Logical camera Id
    std::mutex                                    m_lock;             ///< Lock for client access
    UINT32                                        m_InputFrameNumber; ///< Input frame number
    std::mutex                                    m_afwomLock;        ///< Lock for Android FrameWork Output Metadata access
    std::vector<AndroidMetadataHolder>            m_afwoMetaList;     ///< Android FrameWork Output Metadata list
    bool                                          m_bUseMultipleInputs; ///< Flag to indicate whether to support multiple inputs
    bool                                          m_enableTracking;    ///< Enable meta buffer tracking
    UINT32                                        m_lastTrackedFrame;  ///< Input frame number at which the tracker has executed
    UINT32                                        m_inputFps;          ///< Input frame rate
    bool                                          m_reuseBuffers;      /// release old buffers based on LFU scheme
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the ChiMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiMetadata
{
public:
    /// Max number of clients
    static const UINT32 MaxChiMetaClients = 30;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Creates a CHI Metadata objects
    ///
    /// @param  pTagList                Default list of tags to be pre-allocated
    /// @param  tagCount                Count of tags specified by pTagList to be pre-allocated
    /// @param  useDefaultFrameworkKeys Flag to indicate to preallocate framework keys
    /// @param  pManager                Pointer to Metadata manager object
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiMetadata* Create(
        const UINT32*       pTagList = NULL,
        UINT32              tagCount = 0,
        bool                useDefaultFrameworkKeys = false,
        ChiMetadataManager* pManager = NULL);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Creates a CHI Metadata object from the metadata file
    ///
    /// @param  pMetadataFileName Metadata file name
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiMetadata* Create(
        const CHAR* pMetadataFileName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroys the instance of this object
    ///
    /// @param  force   Flag to indicate whether to force destroy or not. If force flag is disabled, destroy will proceed only
    ///                 if the reference count becomes zero
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Destroy(
       bool force = true);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FindTag
    ///
    /// @brief  Find the entry with the given tag value
    ///
    /// @param  tag     Tag identifier to be searched
    /// @param  pEntry  Tag entry containing all the tag information
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FindTag(
       uint32_t                 tag,
       camera_metadata_entry_t* pEntry);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTag
    ///
    /// @brief  Returns the data of the tag value
    ///
    /// @param  tagID     Tag identifier to be searched
    ///
    /// @return Pointer to the data if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetTag(
        UINT32 tagID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTag
    ///
    /// @brief  Returns the data of the tag value
    ///
    /// @param  tagID     Tag identifier to be searched
    /// @param  entry     Pointer to the data if successful.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult GetTag(
        UINT32            tagID,
        ChiMetadataEntry& entry);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTag
    ///
    /// @brief  Returns the data of the tag value
    ///
    /// @param  pTagSectionName     Section name of the vendor tag
    /// @param  pTagName            Tag name of the vendor tag
    ///
    /// @return Pointer to the data if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetTag(
        const CHAR* pTagSectionName,
        const CHAR* pTagName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetTag
    ///
    /// @brief  Sets the data of the tag value
    ///
    /// @param  tagID     Tag identifier to be set
    /// @param  pData     Pointer to the data
    /// @param  count     Count of the data
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetTag(
        UINT32       tagID,
        const VOID*  pData,
        UINT32       count);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetTag
    ///
    /// @brief  Sets the data of the vendor tag
    ///
    /// @param  pTagSectionName     Section name of the vendor tag
    /// @param  pTagName            Tag name of the vendor tag
    /// @param  pData               Pointer to the data
    /// @param  count               Count of the data
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetTag(
        const CHAR* pTagSectionName,
        const CHAR* pTagName,
        const VOID* pData,
        UINT32      count);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAndroidMetadata
    ///
    /// @brief  Sets all the tags that are present in the android camera metadata structure, pCameraMetadata
    ///
    /// @param  pCameraMetadata  Pointer to the Android metadata structure
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetAndroidMetadata(
        const camera_metadata_t* pCameraMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeleteTag
    ///
    /// @brief  Deletes the tag given by tagID
    ///
    /// @param  tagID     Tag identifier to be deleted
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DeleteTag(
        UINT32 tagID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Count
    ///
    /// @brief  Returns the count of the tags inside the metadata buffer
    ///
    /// @return Count of tags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 Count();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Invalidate
    ///
    /// @brief  Invalidate all the tags in the metadata buffer. Upon successful invalidate, the external metadata reference
    ///         count must be zero.
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Invalidate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Merge
    ///
    /// @brief  Function to merge source metadata with the destination metadata. Tags from source buffer wont be deep copied
    ///         to the destination metadata buffer. After the successful merge operation, destination buffer will have tags
    ///         referring the source buffer
    ///
    /// @param  srcMetadata  Source metadata object from which tags must be copied
    /// @param  disjoint     Flag to indicate whether the merge is disjoint. If disjoint flag is set, only the tags
    ///                      which are not present in the destination buffer will be merged, Else, in case of
    ///                      overlapping tags, entries in hDstMetaHandle will be overwritten with entries from
    ///                      hSrcMetaHandle
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Merge(
        ChiMetadata& srcMetadata,
        bool         disjoint = false);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MergeMultiCameraMetadata
    ///
    /// @brief  Function to merge multi-camera metadata(multiple) into the destination metadata.
    ///         Tags from source buffers are referenced by the destination metadata buffer. After the successful merge
    ///         operation, destination buffer will have tag referring the source buffer
    ///
    /// @param  metadataCount      Count of metadata objects in srcMetadataArray and pCameraIdArray
    /// @param  ppSrcMetadataArray Array of ChiMetadata objects
    /// @param  pCameraIdArray     Array of CameraIds corresponding to each ChiMetadata
    /// @param  primaryCameraId    Identifier of the primary camera
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult MergeMultiCameraMetadata(
        UINT32        metadataCount,
        ChiMetadata** ppSrcMetadataArray,
        UINT32*       pCameraIdArray,
        UINT32        primaryCameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Copy
    ///
    /// @brief  Function to copy source metadata with the destination metadata. Tags from source buffer will be allocated and
    ///         copied to the destination metadata buffer.
    ///
    /// @param  srcMetadata  Source metadata object from which tags must be copied
    /// @param  disjoint     Flag to indicate whether the copy is disjoint. If disjoint flag is set, only the tags
    ///                      which are not present in the destination buffer will be copied. Else, in case of
    ///                      overlapping tags, entries in this object will be overwritten with entries from
    ///                      srcMetadata
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Copy(
        ChiMetadata& srcMetadata,
        bool         disjoint = false);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslateToCameraPartialMetadata
    ///
    /// @brief  Translate CHI PartialMetadata to Camera partial metadata
    ///
    /// @param  pDstCameraMetadata  Destination camera2 metadata buffer to which the tags must be copied
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult TranslateToCameraPartialMetadata(
        camera_metadata*            pDstCameraMetadata,
        UINT32*                     pPartial,
        UINT32                      partialCount);
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TranslateToCameraMetadata
    ///
    /// @brief  Translate CHI Metadata to Camera metadata
    ///
    /// @param  pDstCameraMetadata  Destination camera2 metadata buffer to which the tags must be copied
    /// @param  frameworkTagsOnly   Flag to indicate whether only the framework tags only must be translated
    /// @param  filterProperties    Flag to indicate whether to filter out property tags
    /// @param  filterTagCount      Count of the tags specified by pFilterTagArray.
    /// @param  pFilterTagArray     List of camera metadata tags and vendor tags that needs to be removed from
    ///                                     framework metadata output, pAndroidMeta
    ///
    /// @return CDKResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult TranslateToCameraMetadata(
        camera_metadata*  pDstCameraMetadata,
        bool              frameworkTagsOnly = true,
        bool              filterProperties  = true,
        UINT32            filterTagCount    = 0,
        UINT32*           pFilterTagArray   = NULL);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpAndroidMetadata
    ///
    /// @brief  Static function to dump the android metadata to the file
    ///
    /// @param  pMetadata     Android metadata structure pointer
    /// @param  pFilename     Filename to which the metadata needs to be dumped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DumpAndroidMetadata(
        const camera_metadata* pMetadata,
        const CHAR*            pFilename);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHandle
    ///
    /// @brief  Get the metadata handle corresponding to the CHI Metadata object
    ///
    /// @return Metadata handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE CHIMETAHANDLE GetHandle()
    {
        return m_metaHandle;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintDetails
    ///
    /// @brief  Prints the details of the metadata to the file
    ///
    /// @param  pFileName Filename intoto which the metadata info needs to be dumped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpDetailsToFile(
        const CHAR* pFilename);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// BinaryDump
    ///
    /// @brief  Binary dump metadata info to the file
    ///
    /// @param  pFileName Filename intoto which the metadata info needs to be dumped
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID BinaryDump(
        const CHAR* pFilename)
    {
        CDKResult result = m_metadataOps.pBinaryDump(m_metaHandle, pFilename);
        CHX_ASSERT(CDKResultSuccess == result);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintDetails
    ///
    /// @brief  Prints the details of the metadata to the log
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintDetails();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFrameNumber
    ///
    /// @brief  Frame number associated with the oldest frame corresponding to the metadata buffer
    ///
    /// @return 32-bit client identifier
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 GetFrameNumber()
    {
        return m_metadataClientId.frameNumber;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetClientId
    ///
    /// @brief  Metadata manager Client Id of this buffer. If the buffer is not allocated through the metadata manager,
    ///         InvalidClientId will be returned
    ///
    /// @return 32-bit client identifier
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 GetClientId()
    {
        return m_metadataManagerClientId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReferenceCount
    ///
    /// @brief  Returns reference count of the metadat buffer
    ///
    /// @return Reference count after the operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 ReferenceCount();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddReference
    ///
    /// @brief  Add reference to the metadata
    ///
    /// @param  pClientName     Name of the client that adds the reference. (Optional)
    ///
    /// @return Reference count after the operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 AddReference(
        const CHAR* pClientName = NULL);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseReference
    ///
    /// @brief  Release reference from the metadata
    ///
    /// @param  pClientName     Name of the client that releases the reference. (Optional)
    ///
    /// @return Reference count after the operation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 ReleaseReference(
        const CHAR* pClientName = NULL);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief Class that implements the Iterator for ChiMetadata
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class Iterator
    {
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Iterator
        ///
        /// @brief  Constructor
        ///
        /// @param  metadata     Reference to the CHI metadata object
        ///
        /// @return None
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        Iterator(
            ChiMetadata& metadata);

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// ~Iterator
        ///
        /// @brief  Destructor
        ///
        /// @return None
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        ~Iterator();

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Begin
        ///
        /// @brief  Resets the metadata iterator to the first element. If there are no elements in the metadata,
        ///         false will be returned
        ///
        /// @return true if the current position is valid, false otherwise.
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        bool Begin();

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Next
        ///
        /// @brief  Resets the metadata iterator to the next element. If there are no elements in the metadata,
        ///         false will be returned
        ///
        /// @return true if the current position is valid, false otherwise.
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        bool Next();

        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Get
        ///
        /// @brief  Get the entry corresponding to the current position of the iterator. If the iterator position is invalid,
        ///         false will be returned
        ///
        /// @param  entry     Entry corresponding to the tag Identifier
        ///
        /// @return true if successful.
        ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        bool Get(
            ChiMetadataEntry& entry);

    private:
        CHIMETADATAITERATOR m_iterator; ///< Iterator handle
        ChiMetadata&        m_metadata; ///< Reference to chimetadata

        friend class ChiMetadata;
    };

private:
    /// Constructor
    ChiMetadata() = default;

    /// Destructor
    ~ChiMetadata() = default;

    /// Initializes the camera metadata
    CDKResult Initialize(
        const UINT32* pTagList,
        UINT32        tagCount);

    /// Resets the camera metadata
    CDKResult Reset();

    /// Releases all the references of the metadata buffer
    CDKResult ReleaseAllReferences(
       BOOL bCHIAndCamXReferences);

    /// Initializes the camera metadata ops
    static CDKResult InitializeMetadataOps(
        CHIMETADATAOPS* pMetadataOps);

    /// Clones the metadata
    ChiMetadata* Clone();

    // Internal API to perform destroy
    CDKResult DestroyInternal(
        bool force);

    // Read binary metadata from file
    static CDKResult ReadDataFromFile(
        const CHAR*        pMetadataFileName,
        std::vector<BYTE>& rBuffer);

    // Parse the metadata binary buffer, extract content and update metadata objects
    CDKResult ParseAndSetMetadata(
        std::vector<BYTE>& rBuffer);

    CHIMETADATAHANDLE   m_metaHandle;       ///< Metadata handle
    CHIMETADATAOPS      m_metadataOps;      ///< Metadata operations table
    CHIMETADATACLIENTID m_metadataClientId; ///< Client Identity for CamX Metadata Buffer
    ChiMetadataUsage    m_type;             ///< Usage type of the metadata
    ChiMetadataManager* m_pManager;         ///< ChiMetadata manager
    CMBTimePoint        m_timePoint;        ///< Time point at which metadata is consumed by the client

    UINT32              m_metadataManagerClientId; ///< Client identifier for the metadata manager

    const CHAR* m_clientName[MaxChiMetaClients]; /// Client name strings

    /// Provides access
    friend class ChiMetadataManager;
};

#endif // CHXMETADATA_H
