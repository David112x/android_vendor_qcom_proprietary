////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxchicontext.h
/// @brief Declarations for ChiContext class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CAMXCHICONTEXT_H
#define CAMXCHICONTEXT_H

#include "camxchidefs.h"
#include "camxchitypes.h"
#include "camxdefs.h"
#include "camxhal3stream.h"
#include "camxhwcontext.h"
#include "camxhwenvironment.h"
#include "camxresourcemanager.h"
#include "chi.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class  CHISession;
class  DeferredRequestQueue;
class  HwContext;
class  HwEnvironment;
class  MetadataPool;
class  Pipeline;
class  ThreadManager;

struct PerPipelineInfo;
struct PerNodeInfo;
struct PipelineDescriptor;

enum class ChiFormatType;

/// @todo (CAMX-1512) Fix it the right way
typedef HAL3Stream ChiStreamWrapper;

/// @brief Misc information about each camera
struct PerCameraInfo
{
    MetadataPool* pStaticMetadataPool;      ///< Static Metadatapool
    BOOL          isCameraOpened;           ///< Is camera opened or not indicator
};

/// @brief OverrideOutputFormat flags used to override implimentation defined formats
union OverrideOutputFormat
{
    struct
    {
        BIT isRaw  : 1;                   ///< Set if output format is MIPI raw
        BIT isHDR  : 1;                   ///< Set if output format is UBWCTP10 for HDR video
        BIT unused : 30;                  ///< Unused bits
    };

    UINT32 allFlagsValue;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The ChiContext class provides the implementation used by CHI
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ChiContext
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Creates an instance of the ChiContext class
    ///
    /// @return ChiContext class
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiContext* Create();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroys the instance of the ChiContext class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetNumCameras
    ///
    /// @brief  Get the number of cameras
    ///
    /// @return Number of cameras
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetNumCameras();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCameraInfo
    ///
    /// @brief  Returns the static camera information for the passed in camera id
    ///
    /// @param  cameraId     The index of the camera to return information about
    /// @param  pCameraInfo  Static camera information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetCameraInfo(
        UINT32         cameraId,
        ChiCameraInfo* pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeStaticMetadata
    ///
    /// @brief  Initialize static metadata for the given camera ID. This will be called only once at the HAL3 module creation.
    ///
    /// @param  cameraId     The index of the camera for which the static data need to be populated.
    /// @param  pCameraInfo  The structure to write the static camera information to for a given camera device
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeStaticMetadata(
        UINT32         cameraId,
        ChiCameraInfo* pCameraInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnumerateSensorModes
    ///
    /// @brief  Enumerates the different sensor modes
    ///
    /// @param  cameraId          The cameraId for which the static metadata is queried
    /// @param  numModes          Number of modes in the ModeInfo array
    /// @param  pSensorModeInfo   Mode info array containing numModes elements
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult EnumerateSensorModes(
        UINT32             cameraId,
        UINT32             numModes,
        ChiSensorModeInfo* pSensorModeInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetThreadManager
    ///
    /// @brief  Get the thread manager
    ///
    /// @return Threadmanager pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ThreadManager* GetThreadManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessCameraOpen
    ///
    /// @brief  Called when a camera is opened to do any housekeeping
    ///
    /// @param  cameraId    CameraId that is opened
    ///
    /// @return CamxResultSuccess if successful, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessCameraOpen(
        UINT32 cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessCameraClose
    ///
    /// @brief  Called when a camera is closed to do any housekeeping
    ///
    /// @param  cameraId    CameraId that is closed
    ///
    /// @return CamxResultSuccess if successful, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessCameraClose(
        UINT32 cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreatePipelineDescriptor
    ///
    /// @brief  Creates a pipeline descriptor
    ///
    /// @param  pPipelineName               Pointer to pipeline name
    /// @param  pPipelineCreateDescriptor   Descriptor that defines the pipeline to be created
    /// @param  numOutputs                  Number of output buffers/streams in this pipeline
    /// @param  pOutputBufferDescriptor     Output buffer descriptors
    /// @param  numInputs                   Number of inputs this pipeline can have
    /// @param  pPipelineInputOptions       Input buffer options returned by the driver
    ///
    /// @return PipelineDescriptor*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    PipelineDescriptor* CreatePipelineDescriptor(
        const CHAR*                        pPipelineName,
        const ChiPipelineCreateDescriptor* pPipelineCreateDescriptor,
        UINT32                             numOutputs,
        ChiPortBufferDescriptor*           pOutputBufferDescriptor,
        UINT32                             numInputs,
        CHIPIPELINEINPUTOPTIONS*           pPipelineInputOptions);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroyPipelineDescriptor
    ///
    /// @brief  Destroys a pipeline descriptor
    ///
    /// @param  pPipelineDescriptor   Pipeline descriptor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DestroyPipelineDescriptor(
        PipelineDescriptor* pPipelineDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPipelineDescriptorOutput
    ///
    /// @brief  Sets pipeline descriptor outputs
    ///
    /// @param  pPipelineDescriptor     Pipeline descriptor
    /// @param  numOutputs              Number of outputs
    /// @param  pOutputBufferDescriptor Output buffer descriptors
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPipelineDescriptorOutput(
        PipelineDescriptor*      pPipelineDescriptor,
        UINT                     numOutputs,
        ChiPortBufferDescriptor* pOutputBufferDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPipelineDescriptorInputOptions
    ///
    /// @brief  Sets pipeline descriptor input buffer options
    ///
    /// @param  pPipelineDescriptor         Pipeline descriptor
    /// @param  numInputs                   Number of inputs
    /// @param  pChiPipelineInputOptions    Input options
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetPipelineDescriptorInputOptions(
        PipelineDescriptor*      pPipelineDescriptor,
        UINT32                   numInputs,
        ChiPipelineInputOptions* pChiPipelineInputOptions);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPipelineDescriptorInputStream
    ///
    /// @brief  Sets pipeline descriptor input stream
    ///
    /// @param  pPipelineDescriptor     Pipeline descriptor
    /// @param  pBufferDescriptor       Input buffer descriptor with stream
    /// @param  isStreamWrapperOwner    Is the pipeline descriptor the owner of the stream wrapper i.e. does it need to destroy
    ///                                 it when the pipeline descriptor is destroyed
    ///
    /// @return CamxResultSuccess if successful, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetPipelineDescriptorInputStream(
        PipelineDescriptor*            pPipelineDescriptor,
        const ChiPortBufferDescriptor* pBufferDescriptor,
        BOOL                           isStreamWrapperOwner);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateSession
    ///
    /// @brief  Creates session
    ///
    /// @param  numPipelines                Number of pipelines
    /// @param  pPipelineInfo               Descriptor that defines the pipeline to be created
    /// @param  pCallbacks                  Number of output buffers/streams in this pipeline
    /// @param  pPrivateCallbackData        Output buffer descriptors
    /// @param  flags                       Number of inputs this pipeline can have
    ///
    /// @return CHISession*
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHISession* CreateSession(
        UINT             numPipelines,
        ChiPipelineInfo* pPipelineInfo,
        ChiCallBacks*    pCallbacks,
        VOID*            pPrivateCallbackData,
        CHISESSIONFLAGS  flags);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroySession
    ///
    /// @brief  Destroys a session
    ///
    /// @param  pCHISession    Session to destroy
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DestroySession(
        CHISession* pCHISession);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushSession
    ///
    /// @brief  Flush a session
    ///
    /// @param  pCHISession       Session to Flush
    /// @param  hSessionFlushInfo Flush details
    ///
    /// @return CamxResultSuccess if there is no failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FlushSession(
        CHISession*         pCHISession,
        CHISESSIONFLUSHINFO hSessionFlushInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SubmitRequest
    ///
    /// @brief  Submits a request to pipeline(s) in a session
    ///
    /// @param  pChiSession CHISession pointer
    /// @param  pRequest    Pipeline request
    ///
    /// @return Success or Failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SubmitRequest(
        CHISession*         pChiSession,
        ChiPipelineRequest* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNativeCHI
    ///
    /// @brief  Utility function to determine if it is native CHI or not
    ///
    /// @return TRUE if native CHI, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsNativeCHI() const
    {
        return FALSE;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHwContext
    ///
    /// @brief  Gets the GetHwContext
    ///
    /// @return GetHwContext pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HwContext* GetHwContext() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetResourceManager
    ///
    /// @brief  Get the ResourceManager
    ///
    /// @return ResourceManager pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE ResourceManager* GetResourceManager() const
    {
        return m_pResourceManager;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticSettings
    ///
    /// @brief  Gets the static settings
    ///
    /// @return GetHwContext pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const StaticSettings* GetStaticSettings() const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SelectFormat
    ///
    /// @brief  Function to select format based on the Chi stream
    ///
    /// @param  pStream                         Chi Stream
    /// @param  overrideImplDefinedFormat       Set bits to override output formats
    ///
    /// @return Format
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Format SelectFormat(
        const ChiStream*    pStream,
        OverrideOutputFormat  overrideImplDefinedFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetChiStreamInfo
    ///
    /// @brief  Sets info like gralloc flags, max number of inflight buffers etc that is returned back to the app
    ///
    /// @param  pChiStreamWrapper   Pointer to the ChiStreamWrapper
    /// @param  numBatchFrames      Number of framework frames batched
    /// @param  bExternalNode       Is this stream associated with an external Node
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetChiStreamInfo(
        ChiStreamWrapper* pChiStreamWrapper,
        UINT              numBatchFrames,
        BOOL              bExternalNode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreatePipelineFromDesc
    ///
    /// @brief  Creates a pipeline object from the pipeline descriptor object
    ///
    /// @param  pPipelineDescriptor   Pipeline descriptor handle
    /// @param  pipelineIndex         Pipeline index from Session
    ///
    /// @return Pipeline object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Pipeline* CreatePipelineFromDesc(
        PipelineDescriptor* pPipelineDescriptor,
        UINT                pipelineIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticMetadataPool
    ///
    /// @brief  This method returns the static metdata pool that was created for given cameraId.
    ///
    /// @param  cameraId The cameraId for which the static metadata is queried.
    ///
    /// @return MetadataPool pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    MetadataPool* GetStaticMetadataPool(
        UINT32 cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ActivatePipeline
    ///
    /// @brief  Activate pipeline in session
    ///
    /// @param  pChiSession           CHISession pointer
    /// @param  hPipelineDescriptor   Pipeline handle
    ///
    /// @return Success or Failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ActivatePipeline(
        CHISession*         pChiSession,
        CHIPIPELINEHANDLE   hPipelineDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeactivatePipeline
    ///
    /// @brief  deactivate pipeline in session
    ///
    /// @param  pChiSession           CHISession pointer
    /// @param  hPipelineDescriptor   Pipeline handle
    /// @param  modeBitmask           Deactivate pipeline mode bitmask
    ///
    /// @return Success or Failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DeactivatePipeline(
        CHISession*                 pChiSession,
        CHIPIPELINEHANDLE           hPipelineDescriptor,
        CHIDEACTIVATEPIPELINEMODE   modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateChiFence
    ///
    /// @brief  Creates a Chi fence give the input parameters.
    ///
    /// @param  pInfo       Pointer to the structure containing information about the fence.
    /// @param  phChiFence  Pointer to Chi fence handle to be filled.
    ///
    /// @return CamxResult if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateChiFence(
        CHIFENCECREATEPARAMS*   pInfo,
        CHIFENCEHANDLE*         phChiFence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AttachChiFence
    ///
    /// @brief  Attach to Chi fence give the input parameters.
    ///
    /// @param  hChiFence   Handle to Chi fence to be attached
    ///
    /// @return CamxResult if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AttachChiFence(
        CHIFENCEHANDLE hChiFence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AttachChiFenceCallback
    ///
    /// @brief  Attach to Chi fence callback function
    ///
    /// @param  pObject     Context object pointer
    /// @param  hChiFence   Handle to Chi fence to be attached
    ///
    /// @return CamxResult if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult AttachChiFenceCallback(
        VOID*          pObject,
        CHIFENCEHANDLE hChiFence)
    {
        ChiContext* pContext = static_cast<ChiContext*>(pObject);
        return pContext->AttachChiFence(hChiFence);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseChiFence
    ///
    /// @brief  Releases a Chi fence.
    ///
    /// @param  hChiFence   Handle to Chi fence to be released
    ///
    /// @return CamxResult if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseChiFence(
        CHIFENCEHANDLE hChiFence);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseChiFenceCallback
    ///
    /// @brief  Releases a Chi fence callback function
    ///
    /// @param  pObject     Context object pointer
    /// @param  hChiFence   Handle to Chi fence to be released
    ///
    /// @return CamxResult if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult ReleaseChiFenceCallback(
        VOID*          pObject,
        CHIFENCEHANDLE hChiFence)
    {
        ChiContext* pContext = static_cast<ChiContext*>(pObject);
        return pContext->ReleaseChiFence(hChiFence);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SignalChiFence
    ///
    /// @brief  Signals a Chi fence (only meaningful for internal Chi fences.)
    ///
    /// @param  hChiFence   Handle to Chi fence to be released
    /// @param  result      Result to signal the fence
    ///
    /// @return CamxResult if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SignalChiFence(
        CHIFENCEHANDLE hChiFence,
        CamxResult     result);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SignalChiFenceCallback
    ///
    /// @brief  Signals a Chi fence callback funciton
    ///
    /// @param  pObject     Context object pointer
    /// @param  hChiFence   Handle to Chi fence to be released
    /// @param  result      Result to signal the fence
    ///
    /// @return CamxResult if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CamxResult SignalChiFenceCallback(
        VOID*          pObject,
        CHIFENCEHANDLE hChiFence,
        CamxResult     result)
    {
        ChiContext* pContext = static_cast<ChiContext*>(pObject);
        return pContext->SignalChiFence(hChiFence, result);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitChiFence
    ///
    /// @brief  Signals a Chi fence (only meaninful for internal Chi fences.)
    ///
    /// @param  hChiFence   Handle to Chi fence to be released
    /// @param  pCallback   Pointer to function to be called when the fence signals. If NULL, wait will be blocking.
    /// @param  pUserData   Poiner to user data (if a callback is provided)
    /// @param  waitTime    Time to wait in milliseconds for blocking wait,
    ///                     If this is a non blocking wait i.e pCallBack is not NULL then should be set to UINT64_MAX
    ///
    /// @return CamxResult if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult WaitChiFence(
        CHIFENCEHANDLE          hChiFence,
        PFNCHIFENCECALLBACK     pCallback,
        VOID*                   pUserData,
        UINT64                  waitTime);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpState
    ///
    /// @brief  Dumps anything of value for debugging freezes/hangs
    ///
    /// @param  fd      file descriptor to dump into
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpState(
        INT fd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiFenceResult
    ///
    /// @brief  Gets a chi fence result state
    ///
    /// @param  hChiFence   Handle to Chi fence whose result state to be fetched
    /// @param  pResult     Pointer to the ChiFenceState to be filled, it has the status of the signalled chi fence.
    ///                     CDKResultSuccess        - If the fence is signalled with Success
    ///                     CDKResultEFailed        - If the fence is signalled with Failure
    ///                     CDKResultEInvalidState  - If the fence is not yet signalled
    ///
    /// @return CamxResult  if successful in fetching the result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetChiFenceResult(
        CHIFENCEHANDLE hChiFence,
        CDKResult*     pResult);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataInfo
    ///
    /// @brief  Method to query metadata information
    ///
    /// @param  pChiSession             CHISession pointer
    /// @param  hPipelineDescriptor     Pipeline handle
    /// @param  maxPublishTagArraySize  Publish tag array size
    /// @param  pPublishTagArray        Array of tags published by the pipeline
    /// @param  pPublishTagCount        Pointer to the Count of the tags published by the pipeline
    /// @param  pPartialPublishTagCount Pointer to the Count of the partial tags published by the pipeline
    /// @param  pMaxNumMetaBuffers      Pointer to the maximum number of metadata buffers required by the pipeline
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult QueryMetadataInfo(
        CHISession*             pChiSession,
        const CHIPIPELINEHANDLE hPipelineDescriptor,
        const UINT32            maxPublishTagArraySize,
        UINT32*                 pPublishTagArray,
        UINT32*                 pPublishTagCount,
        UINT32*                 pPartialPublishTagCount,
        UINT32*                 pMaxNumMetaBuffers);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGrallocUsage
    ///
    /// @brief  GetGrallocUsage flags
    ///
    /// @param  pStream                         Stream pointer
    ///
    /// @return grallocUsage 64 bit flags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static GrallocUsage64 GetGrallocUsage(
        const ChiStream* pStream);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetChiFenceResult
    ///
    /// @brief  Sets a chi fence result state
    ///
    /// @param  hChiFence Handle to Chi fence whose result state is to be set
    /// @param  result    Value to be set, of type ChiFenceState
    ///
    /// @return CamxResult if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetChiFenceResult(
        CHIFENCEHANDLE hChiFence,
        CamxResult     result);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OverrideMitigation
    ///
    /// @brief  Override target specific video mitigations
    ///
    /// @param  cameraId    camerId being queried
    ///
    /// @return CamxResult if success or appropriate error code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult OverrideMitigation(
        UINT32 cameraId);
private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initializes the created object
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult Initialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetImplDefinedFormat
    ///
    /// @brief  Selects an implementation defined format
    ///
    /// @param  pStream                       Stream pointer
    /// @param  overrideImplDefinedFormat     Structure to decide if the output format needs override
    ///
    /// @return Format
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Format GetImplDefinedFormat(
        const ChiStream*      pStream,
        OverrideOutputFormat  overrideImplDefinedFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCameraOpened
    ///
    /// @brief  Function to check if camera is opened or not
    ///
    /// @param  cameraId    CameraId being queried
    ///
    /// @return TRUE if opened, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsCameraOpened(
        UINT32 cameraId
        ) const
    {
        return m_perCameraInfo[cameraId].isCameraOpened;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeStaticMetadataPool
    ///
    /// @brief  Initialize static metadata pool for the given camera ID. This will be called only once at the Adapter creation.
    ///
    /// @param  cameraId     The cameraId for which the static metadata pool needs to be populated.
    ///
    /// @return CamxResultSuccess if successful, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeStaticMetadataPool(
        UINT32 cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStaticMetadata
    ///
    /// @brief  This method returns the static metdata that was created for given cameraId.
    ///
    /// @param  cameraId The cameraId for which the static metadata is queried.
    ///
    /// @return MetadataPool pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    const Metadata* GetStaticMetadata(
        UINT32 cameraId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessPipelineCreateDesc
    ///
    /// @brief  Convert the information in Chi pipeline create desc to an internal representation used to create the pipeline.
    ///         This facilitates making pipeline creation to become API agnostic.
    ///
    /// @param  pPipelineCreateDescriptor   Pipeline create descriptor
    /// @param  numOutputs                  Number of outputs of the pipeline
    /// @param  pOutputBufferDescriptor     Description of the output buffer
    /// @param  pPipelineDescriptor         Pipeline descriptor
    ///
    /// @return CamxResultSuccess if successful, failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessPipelineCreateDesc(
        const ChiPipelineCreateDescriptor* pPipelineCreateDescriptor,
        UINT                               numOutputs,
        ChiPortBufferDescriptor*           pOutputBufferDescriptor,
        PipelineDescriptor*                pPipelineDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CloneNodeProperties
    ///
    /// @brief  Clone the node properties.
    ///
    /// @param  pChiNode        Pointer to Chi Node
    /// @param  pPerNodeInfo    Pointer to node info
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CloneNodeProperties(
        ChiNode*      pChiNode,
        PerNodeInfo*  pPerNodeInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsUBWCLossySupported
    ///
    /// @brief  Function to check if UBWC is lossy compression or lossless
    ///
    /// @param  pStream             pointer to Chi Stream
    /// @param  pChiStreamWrapper   pointer to Chi stream Wrapper
    ///
    /// @return TRUE if UBWC lossy mode, othrewise lossless
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsUBWCLossySupported(
        const ChiStream*        pStream,
        const ChiStreamWrapper* pChiStreamWrapper);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiContext
    ///
    /// @brief  Default constructor for the CHICONTEXT class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiContext() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiContext
    ///
    /// @brief  Default destructor for the CHICONTEXT class
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~ChiContext();



    // Do not implement the copy constructor or assignment operator
    ChiContext(const ChiContext& rChiContext) = delete;
    ChiContext& operator= (const ChiContext& rChiContext) = delete;

    /// @todo (CAMX-1797) Quickly discuss it one more time to make sure there are no bad side effects - we need to consider
    ///                   two different cameraIds opened simulatoneously so we need to account for that
    ///                   Need to make CSLSessionHandle per CHI session instead of per ChiContext
    ///                   Right place for usecase pool
    ///                   Isstreamed on
    ThreadManager*              m_pThreadManager;                                   ///< Thread manager instance
    HwContext*                  m_pHwContext;                                       ///< HwContext pointer
    HwEnvironment*              m_pHwEnvironment;                                   ///< HwEnvironment instance pointer
    ResourceManager*            m_pResourceManager;                                 ///< Resource manager pointer
    PerCameraInfo               m_perCameraInfo[MaxNumCameras];                     ///< Information per camera
    UINT32                      m_numCamerasOpened;                                 ///< Number of concurrently opened cameras
    DeferredRequestQueue*       m_pDeferredRequestQueue;                            ///< Pointer to the deferred process handler
    Mutex*                      m_pReleaseChiFence;                                 ///< Chifence Mutex
};

CAMX_NAMESPACE_END

#endif // CAMXCHICONTEXT_H
