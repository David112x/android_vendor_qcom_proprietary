////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecaseselector.h
/// @brief CHX usecase selector utility class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXUSECASEUTILS_H
#define CHXUSECASEUTILS_H

#include <assert.h>
#include "chxincs.h"
#include "chxextensionmodule.h"
#if (!defined(LE_CAMERA)) // ANDROID
#include <hardware/gralloc1.h>
#endif // ANDROID
#include "chxmetadata.h"
// generated headers
#include "g_pipelines.h"
#if (!defined(LE_CAMERA)) // ANDROID
#include "gralloc_priv.h"
#else // LE_CAMERA
#include "linuxembeddedgralloc.h"
#endif // LE_CAMERA

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

#define WAIT_BUFFER_TIMEOUT 700 // should be less than the timeout used for each request (800ms)

/// Forward Declaration
struct ChiUsecase;
class  Usecase;

/// @brief Gralloc1 interface functions
struct Gralloc1Interface
{
    INT32 (*CreateDescriptor)(
        gralloc1_device_t*             pGralloc1Device,
        gralloc1_buffer_descriptor_t*  pCreatedDescriptor);
    INT32 (*DestroyDescriptor)(
        gralloc1_device_t*            pGralloc1Device,
        gralloc1_buffer_descriptor_t  descriptor);
    INT32 (*SetDimensions)(
        gralloc1_device_t*           pGralloc1Device,
        gralloc1_buffer_descriptor_t descriptor,
        UINT32                       width,
        UINT32                       height);
    INT32 (*SetFormat)(
        gralloc1_device_t*           pGralloc1Device,
        gralloc1_buffer_descriptor_t descriptor,
        INT32                        format);
    INT32 (*SetProducerUsage)(
        gralloc1_device_t*           pGralloc1Device,
        gralloc1_buffer_descriptor_t descriptor,
        UINT64                       usage);
    INT32 (*SetConsumerUsage)(
        gralloc1_device_t*           pGralloc1Device,
        gralloc1_buffer_descriptor_t descriptor,
        UINT64                       usage);
    INT32 (*Allocate)(
        gralloc1_device_t*                  pGralloc1Device,
        UINT32                              numDescriptors,
        const gralloc1_buffer_descriptor_t* pDescriptors,
        buffer_handle_t*                    pAllocatedBuffers);
    INT32 (*GetStride)(
        gralloc1_device_t* pGralloc1Device,
        buffer_handle_t    buffer,
        UINT32*            pStride);
    INT32 (*Release)(
        gralloc1_device_t* pGralloc1Device,
        buffer_handle_t    buffer);
    INT32 (*Lock)(
            gralloc1_device_t*      device,
            buffer_handle_t         buffer,
            uint64_t                producerUsage,
            uint64_t                consumerUsage,
            const gralloc1_rect_t*  accessRegion,
            void**                  outData,
            int32_t                 acquireFence);
    gralloc1_error_t (*Perform)(
            gralloc1_device_t*   device,
            int                  operation, ...);
};

enum class SnapshotStreamType
{
    UNKNOWN = -1,
    INVALID = 0,
    JPEG    = 1,
    HEIC    = 2
};

enum class InputOutputType
{
    NO_SPECIAL,                         ///< Normal Input output request
    YUV_OUT                             ///< Snapshot YUV output request
};


/// @breif Describes framework Snapshot Stream Configuration
struct SnapshotStreamConfig
{
    SnapshotStreamType type;             ///< SnapshotType of this stream config
    CHISTREAM*         pSnapshotStream;  ///< Pointer to the main snapshot stream. The width x height of this stream are the
                                         ///  correct snapshot dimensions
    CHISTREAM*         pThumbnailStream; ///< Pointer to the thumbnail stream. This stream is only valid if type == HEIC
    CHISTREAM*         pRawStream;       ///< Pointer to the thumbnail stream. This stream is only valid if type == HEIC
};

/// @breif Generic lightweight doubly lined list node
struct LightweightDoublyLinkedListNode
{
    VOID*                                   pData; ///< Pointer to the data that needs to be stored.
    struct LightweightDoublyLinkedListNode* pPrev; ///< Pointer to the previous element in the list.
    struct LightweightDoublyLinkedListNode* pNext; ///< Pointer to the next element in the list.
};

// Default fd stream
static CHISTREAM m_DefaultFDStream
{
    ChiStreamTypeOutput,
    640,
    480,
    ChiStreamFormatYCbCr420_888,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
    NULL,
    {0, 0},
    { NULL, NULL, NULL },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that helps select a usecase from the usecases the override module supports
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC UsecaseSelector
{
public:

    /// Utility function to Calculate the FovRectIFE
    /// @TODO: Consider binning mode and sensor crop info in calculation
    static CHX_INLINE VOID CalculateFovRectIFE(
        CHIRECT*      fovRectIFE,
        const CHIRECT frameDimension,
        const CHIRECT activeArray)
    {
        UINT32 xStart = frameDimension.left;
        UINT32 yStart = frameDimension.top;

        fovRectIFE->left   = 0;
        fovRectIFE->top    = 0;
        fovRectIFE->width  = activeArray.width  - (2 * xStart);
        fovRectIFE->height = activeArray.height - (2 * yStart);
    }

    /// Creates an instance of this class
    static UsecaseSelector* Create(
        const ExtensionModule* pExtModule);

    /// Destroys an instance of the class
    VOID Destroy();

    /// Get Camera ID of Max resolution camera in Dualcamera usecase
    static UINT32 FindMaxResolutionCameraID(
        LogicalCameraInfo* pCameraInfo);

    /// Get Sensor Dimension
    static VOID getSensorDimension(
        const UINT32 cameraID, const camera3_stream_configuration_t* pStreamConfig,
        UINT32 *sensorw, UINT32 *sensorh, UINT32 downscaleFactor,
        UINT32 activeAspectRatio = 0);

    /// Get Sensor Mode Info
    static CHISENSORMODEINFO* GetSensorModeInfo(
        const UINT32 cameraID, const camera3_stream_configuration_t* pStreamConfig,
        UINT32 downscaleFactor, UINT32 activeAspectRatio = 0);

    /// Returns a matching usecase
    UsecaseId GetMatchingUsecase(
        const LogicalCameraInfo *pCamInfo,
        camera3_stream_configuration_t* pStreamConfig); ///< Stream configuration

    /// Returns a default matching usecase, Null otherwise
    static ChiUsecase* DefaultMatchingUsecase(
        camera3_stream_configuration_t* pStreamConfig); ///< Stream configuration

    /// Is the stream config a match for MFNR
    static BOOL MFNRMatchingUsecase(
        const camera3_stream_configuration_t* pStreamConfig);

    /// Is the stream config a match for MFSR
    static BOOL MFSRMatchingUsecase(
        const camera3_stream_configuration_t* pStreamConfig);

    /// Is the stream config a match for video liveshot
    static BOOL IsVideoLiveShotConfig(
        const camera3_stream_configuration_t* pStreamConfig);

    /// Is Video EISV2 enabled
    static BOOL IsVideoEISV2Enabled(
        camera3_stream_configuration_t* pStreamConfig);

    /// Is Video EISV3 enabled
    static BOOL IsVideoEISV3Enabled(
        camera3_stream_configuration_t* pStreamConfig);

    /// Is the stream config a match for Quad CFA
    static BOOL QuadCFAMatchingUsecase(
        const LogicalCameraInfo*              pCamInfo,
        const camera3_stream_configuration_t* pStreamConfig);

    /// Is this a Quad CFA sensor
    static BOOL IsQuadCFASensor(
        const LogicalCameraInfo* pCamInfo,
        CHIREMOSAICTYPE*         pRemosaicType);

    /// Is this a preview stream
    static BOOL IsPreviewStream(
        const camera3_stream_t* pStream);

    /// Is this a video stream
    static BOOL IsVideoStream(
        const camera3_stream_t* pStream);

    /// Is this a HEIF stream
    static BOOL IsHEIFStream(
        const camera3_stream_t* pStream);

    /// Is this a snapshot YUV stream
    static BOOL IsYUVSnapshotStream(
        const camera3_stream_t* pStream);

    /// Is this a snapshot YUV stream
    static BOOL IsYUVOutThreshold(
        const camera3_stream_t* pStream);

    /// Is this a snapshot JPEG stream
    static BOOL IsJPEGSnapshotStream(
        const camera3_stream_t* pStream);

    /// Is this a Raw stream
    static BOOL IsRawStream(
        const camera3_stream_t* pStream);

    /// Is this a Raw input stream
    static BOOL IsRawInputStream(
        const camera3_stream_t* pStream);

    static BOOL HasHeicSnapshotStream(
        const camera3_stream_configuration_t* pStreamConfig);

    static BOOL HasHeicSnapshotStream(
        const CHISTREAMCONFIGINFO* pStreamConfig);

#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) //Android-P or better
    static BOOL IsMatchingFormat(
        ChiStream*             pStream,
        UINT32                 numFormats,
        const ChiBufferFormat* pFormats);
#endif

    /// Is this a YUV input stream
    static BOOL IsYUVInStream(
        const camera3_stream_t* pStream);

    /// Is this a YUV output stream
    static BOOL IsYUVOutStream(
        const camera3_stream_t* pStream);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSnapshotStreamConfiguration
    ///
    /// @brief  Populates a SnapshotStreamConfig struct with details for JPEG and HEIC snapshot streams
    ///         rSnapshotStreamConfig.pSnapshotStream will point to the framework stream that is used for snapshot for both
    ///         JPEG and HEIC.
    ///
    /// @param  numStreams            [IN]  The number of ppChiStreams
    /// @param  ppChiStreams          [IN]  An array of framework/CHISTREAMS
    /// @param  rSnapshotStreamConfig [OUT] SnapshotStreamConfig struct to fill
    ///
    /// @return CDKResultSuccess on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult GetSnapshotStreamConfiguration(
        UINT                                  numStreams,
        CHISTREAM**                           ppChiStreams,
        SnapshotStreamConfig&                 rSnapshotStreamConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PruneUsecaseDescriptor
    ///
    /// @brief  Given a usecase descriptor, prune that Usecase.
    ///
    ///         Node descriptors and ports that are sink buffers with are currently the only prunable elements.
    ///         When a prunable element is pruned, that element is removed from the Usecase written to ppPrunedUsecase.
    ///
    ///         A prunable element E that has a non-zero number of PruneVariants will not be pruned if and only if
    ///         For all unique PruneGroups G in {(G, *) in pPruneVariants},
    ///         There exists a PruneVariant (G, T) in E equivalent to any element of pPruneVariants
    ///
    ///         This function will remove any link descriptors that have no DstPorts after pruning.
    ///
    /// @param  pUsecase         [IN]  Pointer to the usecase descriptor to prune
    /// @param  numPruneVariants [IN]  The number of prune settings
    /// @param  pPruneVariants   [IN]  Settings to describe how the pruning should occur
    /// @param  ppPrunedUsecase  [OUT] Pointer to memory that should store the pruned usecase
    ///                                Free with UsecaseSelector::FreeUsecaseDescriptor
    ///
    /// @return CDKResult on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult PruneUsecaseDescriptor(
        const ChiUsecase* const   pUsecase,
        const UINT                numPruneVariants,
        const PruneVariant* const pPruneVariants,
        ChiUsecase**              ppPrunedUsecase);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PruneUsecaseByStreamConfig
    ///
    /// @brief  Prunes a Usecase Descriptor with default settings based on framework stream configuration.
    ///         This function currently only prunes between JPEG and HEIC
    ///         See PruneUsecaseDescriptor for more information on pruning in general
    ///
    /// @param  pStreamConfig             [IN]  Pointer to framework stream configuration
    /// @param  pUsecaseInputDescriptor   [IN]  Pointer to the usecase descriptor to prune
    /// @param  ppUsecaseOutputDescriptor [OUT] Pointer to memory that should store the pruned usecase
    ///                                         Free with UsecaseSelector::FreeUsecaseDescriptor
    ///
    /// @return CDKResult on success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult PruneUsecaseByStreamConfig(
        const camera3_stream_configuration* pStreamConfig,
        const ChiUsecase*                   pUsecaseInputDescriptor,
        ChiUsecase**                        ppUsecaseOutputDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVariantGroup
    ///
    /// @brief  Given the name of a variant group, return the associated VariantGroup
    ///
    /// @param  pVariantName String of variant group name
    ///
    /// @return VariantGroup
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VariantGroup GetVariantGroup(
        const CHAR* pVariantName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetVariantType
    ///
    /// @brief  Given the name of a variant type, return the associated VariantType
    ///
    /// @param  pVariantName String of variant type name
    ///
    /// @return VariantType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VariantType GetVariantType(
        const CHAR* pVariantName);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FreeUsecaseDescriptor
    ///
    /// @brief  Utility function to free pruned usecase structure
    ///
    /// @param  pUsecase Pointer to the pruned usecase descriptor to free
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult FreeUsecaseDescriptor(
        ChiUsecase* pUsecase);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CloneUsecase
    ///
    /// @brief  Util functions to deep copy usecase structure
    ///
    /// @param  pSrcUsecase [IN] The usecase to clone
    /// @param  numDesc     [IN] The number of mappings in pDescIndex
    /// @param  pDescIndex  [IN] Mapping of which pipelines to clone from the original descriptor
    ///
    /// @return Pointer to the cloned usecase
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiUsecase* CloneUsecase(
        const ChiUsecase *pSrcUsecase,
        UINT32            numDesc,
        UINT32 *          pDescIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroyUsecase
    ///
    /// @brief  Utility function to destroy cloned usecase structure
    ///
    /// @param  pUsecase Pointer to the cloned descriptor to destroy
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID DestroyUsecase(
        ChiUsecase *pUsecase);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFDOutStream
    ///
    /// @brief  Utility function to get FD stream
    ///
    /// @return FD stream
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE static ChiStream GetFDOutStream()
    {
        return FDStream;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateFDStream
    ///
    /// @brief  Update FD stream with respect to given aspect ratio
    ///
    /// @param  referenceAspectRatio reference aspect ratio to calculate FD stream resolution
    ///
    /// @return none
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID UpdateFDStream(
        FLOAT referenceAspectRatio);

private:
    /// Initializes the instance
    CDKResult Initialize();

    /// Is the stream config a match for Preview+ZSL-YUV
    static BOOL IsPreviewZSLStreamConfig(
        const camera3_stream_configuration_t* pStreamConfig);

    /// Is the stream config a match for Raw+JPEG usecase
    static BOOL IsRawJPEGStreamConfig(
        const camera3_stream_configuration_t* pStreamConfig);

    /// Is the stream config a match YUV In and Blob out
    static BOOL IsYUVInBlobOutConfig(
        const camera3_stream_configuration_t* pStreamConfig);

    static BOOL IsMatchingUsecase(
        const camera3_stream_configuration_t* pStreamConfig,    ///< Stream config
        const ChiUsecase*                     pUsecase,         ///< Usecase attempted to match
        const PruneSettings*                  pPruneSettings);

#if !(defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)) //Android-P or better
    static BOOL IsMatchingFormat(
        ChiStream*             pStream,
        UINT32                 numFormats,
        const ChiBufferFormat* pFormats);
#endif

    static BOOL IsAllowedImplDefinedFormat(
        ChiBufferFormat format,
        GrallocUsage    streamGrallocUsage);

    /// Clone pipeline descriptor
    static CDKResult ClonePipelineDesc(
        const ChiPipelineTargetCreateDescriptor* pSrcDesc,
        ChiPipelineTargetCreateDescriptor*       pDstDesc);

    /// Clone chi target
    static ChiTarget* ClonePipelineDesc(
        const ChiTarget* pSrcDesc);

    /// Destroy cloned pipeline descriptor
    static VOID DestroyPipelineDesc(
        ChiPipelineTargetCreateDescriptor* pDesc);

    /// Destroy cloned chi target
    static VOID DestroyPipelineDesc(
        ChiTarget* pDesc);

    UsecaseSelector() = default;
    ~UsecaseSelector();
    // Do not support the copy constructor or assignment operator
    UsecaseSelector(const UsecaseSelector& rUsecaseSelector) = delete;
    UsecaseSelector& operator= (const UsecaseSelector& rUsecaseSelector) = delete;

    static UINT            NumImplDefinedFormats;
    static ChiBufferFormat AllowedImplDefinedFormats[16];
    static BOOL            GPURotationUsecase;             ///< Flag to select gpu rotation usecases
    static BOOL            GPUDownscaleUsecase;            ///< Flag to select gpu rotation usecases
    static BOOL            HFRNo3AUsecase;                 ///< Flag to select HFR without 3A usecases
    static UINT            VideoEISV2Usecase;              ///< Flag to select EIS V2 usecases
    static UINT            VideoEISV3Usecase;              ///< Flag to select EIS V3 usecases
    static ChiStream       FDStream;                       ///< FD Stream
    const ExtensionModule* m_pExtModule;              ///< const pointer to the extension module
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the factory to create usecase specific objects
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UsecaseFactory
{
public:
    /// Creates an object of this factory type
    static UsecaseFactory* Create(
        const ExtensionModule* pExtModule);
    /// Destroy the factory
    VOID Destroy();
    /// Create an usecase object of the specified type
    Usecase* CreateUsecaseObject(
        LogicalCameraInfo*              pLogicalCameraInfo, ///< [In] Camera associated with the usecase
        UsecaseId                       usecaseId,          ///< [In] Usecase Id
        camera3_stream_configuration_t* pStreamConfig);     ///< [In] Stream config

private:
    UsecaseFactory() = default;
    ~UsecaseFactory();
    UsecaseFactory(const UsecaseFactory&) = delete;             ///< Disallow the copy constructor
    UsecaseFactory& operator=(const UsecaseFactory&) = delete;  ///< Disallow assignment operator

    const ExtensionModule* m_pExtModule;              ///< const pointer to the extension module
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Generic POD lightweight doubly linked list implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LightweightDoublyLinkedList final
{
public:
    // Insert to Tail of the list
    CHX_INLINE VOID InsertToTail(
        LightweightDoublyLinkedListNode* pNodeToInsert)
    {
        CHX_ASSERT(pNodeToInsert->pPrev == NULL);
        CHX_ASSERT(pNodeToInsert->pNext == NULL);

        if (m_pHead == NULL)
        {
            m_pHead = pNodeToInsert;

            // First node going into the doubly linked list, so our head/tail are the same.
            m_pTail = m_pHead;
        }
        else
        {
            // Since we can always know where the tail it, inserts become trivial.
            m_pTail->pNext = pNodeToInsert;
            pNodeToInsert->pPrev = m_pTail;

            m_pTail = pNodeToInsert;
        }

        m_numNodes++;
    }

    // Insert to Head of the list
    CHX_INLINE VOID InsertToHead(
        LightweightDoublyLinkedListNode* pNodeToInsert)
    {
        CHX_ASSERT(pNodeToInsert->pPrev == NULL);
        CHX_ASSERT(pNodeToInsert->pNext == NULL);

        if (m_pHead == NULL)
        {
            m_pHead = pNodeToInsert;

            // First node going into the doubly linked list, so our head/tail are the same.
            m_pTail = m_pHead;
        }
        else
        {
            // Since we can always know where the head is, inserts become trivial.
            pNodeToInsert->pNext = m_pHead;
            m_pHead->pPrev = pNodeToInsert;
            m_pHead = pNodeToInsert;
        }

        m_numNodes++;
    }

    // Remove and return head node
    CHX_INLINE LightweightDoublyLinkedListNode* RemoveFromHead()
    {
        LightweightDoublyLinkedListNode* pNode = m_pHead;

        if (pNode != NULL)
        {
            // If the only node was removed, the tail must be updated to reflect the empty list.
            if (m_numNodes == 1)
            {
                CHX_ASSERT(pNode == m_pTail);

                m_pTail = NULL;
                m_pHead = NULL;
            }
            else
            {
                m_pHead = pNode->pNext;
                if (NULL != m_pHead)
                {
                    m_pHead->pPrev = NULL;
                }
            }

            pNode->pPrev = NULL;
            pNode->pNext = NULL;

            m_numNodes--;
        }

        return pNode;
    }

    // Remove Node from list
    CHX_INLINE VOID RemoveNode(
        LightweightDoublyLinkedListNode* pNode)
    {
        if (NULL != pNode)
        {
            if (pNode == m_pHead)
            {
                m_pHead = pNode->pNext;

                if (m_pHead != NULL)
                {
                    m_pHead->pPrev = NULL;
                }
            }
            else
            {
                if (NULL != pNode->pPrev)
                {
                    pNode->pPrev->pNext = pNode->pNext;
                }

                if (pNode->pNext != NULL)
                {
                    pNode->pNext->pPrev = pNode->pPrev;
                }
            }

            if (pNode == m_pTail)
            {
                m_pTail = pNode->pPrev;
            }

            pNode->pPrev = NULL;
            pNode->pNext = NULL;

            m_numNodes--;
        }
    }

    // Helper to fetch next node from current node
    CHX_INLINE static LightweightDoublyLinkedListNode* NextNode(
        LightweightDoublyLinkedListNode* pNode)
    {
        return (NULL != pNode) ? pNode->pNext : NULL;
    }

    // Helper to fetch head node
    CHX_INLINE LightweightDoublyLinkedListNode* Head() const { return m_pHead; }

    // Helper to fetch tail node
    CHX_INLINE LightweightDoublyLinkedListNode* Tail() const { return m_pTail; }

    // Helper to fetch node count
    CHX_INLINE UINT NumNodes() const { return m_numNodes; }

    // constructor
    // NOWHINE CP016: Basic final class wont have overrides
    CHX_INLINE LightweightDoublyLinkedList()
    {
        m_pHead     = NULL;
        m_pTail     = NULL;
        m_numNodes  = 0;
    }

    // destructor
    // NOWHINE CP022: Basic final class wont have overrides
    CHX_INLINE ~LightweightDoublyLinkedList()
    {
        CHX_ASSERT(NumNodes() == 0);
    }

private:
    // Member methods
    LightweightDoublyLinkedList(const LightweightDoublyLinkedList&) = delete;
    LightweightDoublyLinkedList& operator=(const LightweightDoublyLinkedList&) = delete;

    LightweightDoublyLinkedListNode* m_pHead;    ///< The first element in the list
    LightweightDoublyLinkedListNode* m_pTail;    ///< The last element in the list
    UINT                             m_numNodes; ///< The number of elements in the list
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the ImageBuffer to allocate gralloc buffers and track reference count
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ImageBuffer
{
protected:
    ImageBuffer();
    ~ImageBuffer();

private:
    /// Static create function to make an instance of this class
    static ImageBuffer* Create(Gralloc1Interface*  pGrallocInterface,
                               gralloc1_device_t*  pGralloc1Device,
                               UINT32              width,
                               UINT32              height,
                               UINT32              format,
                               UINT64              producerUsageFlags,
                               UINT64              consumerUsageFlags,
                               UINT32*             pStride);

    /// Allocate gralloc buffer
    CDKResult AllocateGrallocBuffer(Gralloc1Interface*  pGrallocInterface,
                                    gralloc1_device_t*  pGralloc1Device,
                                    UINT32              width,
                                    UINT32              height,
                                    UINT32              format,
                                    UINT64              producerUsageFlags,
                                    UINT64              consumerUsageFlags,
                                    UINT32*             pStride);

    /// Destroys an instance of the class
    VOID Destroy(Gralloc1Interface*  pGrallocInterface,
                 gralloc1_device_t*  pGralloc1Device);

    /// Return reference count of this image buffer
    UINT32 GetReferenceCount();

    /// Add a reference to this image buffer
    VOID AddReference();

    /// Release a reference to this image buffer
    VOID ReleaseReference();

    /// Return the handle of this image buffer
    CHX_INLINE buffer_handle_t* GetBufferHandle()
    {
        return &pGrallocBuffer;
    }

    /// Return the buffer type of this image buffer
    CHX_INLINE CHIBUFFERTYPE GetBufferHandleType()
    {
        return ChiGralloc;
    }

    friend class CHIBufferManager;
    buffer_handle_t pGrallocBuffer;     ///< The buffer handle
    volatile UINT32 m_aReferenceCount;  ///< The reference count.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief CHI Buffer Manager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC CHIBufferManager
{
public:
    /// Creates an instance of this class
    static CHIBufferManager* Create(
        const CHAR*                 pBufferManagerName,
        CHIBufferManagerCreateData* pCreateData);

    /// Destroys an instance of the class
    VOID Destroy();

    /// Return an Buffer info, which contains buffer handle and buffer type
    CHIBUFFERINFO GetImageBufferInfo();

    /// Add reference to the Buffer
    CDKResult AddReference(const CHIBUFFERINFO* pBufferInfo);

    /// Release reference to the Buffer
    CDKResult ReleaseReference(const CHIBUFFERINFO* pBufferInfo);

    /// Get reference of the Buffer
    UINT GetReference(const CHIBUFFERINFO* pBufferInfo);

    /// Activate this buffer manager. This gives a hint to manager underlying Image Buffer allocations
    CDKResult Activate();

    /// Deactivate this Buffer Manager. This releases all or some buffers allocated by the buffer manager
    CDKResult Deactivate(BOOL isPartialRelease);

    /// Bind backing buffer for this Buffer
    CDKResult BindBuffer(const CHIBUFFERINFO* pBufferInfo);

    // Get the Buffer size in the underlying buffer handle
    static UINT32 GetBufferSize(const CHIBUFFERINFO* pBufferInfo);

    /// Copy Buffer contents to another buffer
    static CDKResult CopyBuffer(const CHIBUFFERINFO*    pSrcBufferInfo,
                                CHIBUFFERINFO*          pDstBufferInfo);

    /// Set PerfMode metadata in this Buffer
    static VOID SetPerfMode(CHIBUFFERINFO* pBufferInfo);

    /// Get CPU virtual address for this Buffer
    static VOID* GetCPUAddress(
        const CHIBUFFERINFO*    pBufferInfo,
        INT                     size);

    /// Put CPU virtual address for this Buffer
    static VOID PutCPUAddress(
        const CHIBUFFERINFO*    pBufferInfo,
        INT                     size,
        VOID*                   pVirtualAddress);

    /// Get fd for this Buffer
    static INT GetFileDescriptor(const CHIBUFFERINFO* pBufferInfo);

    /// Get gralloc buffer_handle_t* address for this Buffer
    static buffer_handle_t* GetGrallocHandle(const CHIBUFFERINFO* pBufferInfo);

    /// Clean and/or invalidate the cache for all memory associated with this image buffer
    CDKResult CacheOps(
        const CHIBUFFERINFO* pBufferInfo,
        BOOL                 invalidate,
        BOOL                 clean);

private:
    CHIBufferManager();
    ~CHIBufferManager();

    /// Do not support the copy constructor or assignment operator
    CHIBufferManager(const CHIBufferManager&) = delete;
    CHIBufferManager& operator= (const CHIBufferManager&) = delete;

    /// Setup gralloc1 interface functions
    CDKResult SetupGralloc1Interface();

    /// Initializes the instance
    CDKResult Initialize(
        const CHAR*                 pBufferManagerName,
        CHIBufferManagerCreateData* pCreateData);

    /// Reverse look up for image buffer pointer given buffer handle
    LightweightDoublyLinkedListNode* LookupImageBuffer(buffer_handle_t* pBufferHandle);

    /// Return an image buffer. Reference count is set to 1 when an image buffer is returned,
    /// Caller needs to release reference when the buffer is no longer needed.
    ImageBuffer* GetImageBuffer();

    /// Free buffers
    VOID FreeBuffers(BOOL isPartialFree);

    CHAR                            m_pBufferManagerName[MaxStringLength64];    ///< Name of the buffer manager
    BOOL                            m_bIsUnifiedBufferManagerEnabled;           ///< Boolean indicating if UBM is enabled
    CHIBUFFERMANAGERHANDLE          m_pUnifiedBufferManager;                    ///< Pointer to the Unified Buffer Manager
    hw_module_t*                    m_pHwModule;                                ///< Gralloc1 module
    gralloc1_device_t*              m_pGralloc1Device;                          ///< Gralloc1 device
    Gralloc1Interface               m_grallocInterface;                         ///< Gralloc1 interface
    Mutex*                          m_pLock;                                    ///< Mutex protects free and busy list
    Condition*                      m_pWaitFreeBuffer;                          ///< Wait for a busy buffer becomes free
    LightweightDoublyLinkedList*    m_pFreeBufferList;                          ///< List manages free buffers
    LightweightDoublyLinkedList*    m_pBusyBufferList;                          ///< List manages busy buffers
    CHIBufferManagerCreateData      m_pBufferManagerData;                       ///< Data structure contains image buffer information
};

#endif // CHXUSECASEUTILS_H
