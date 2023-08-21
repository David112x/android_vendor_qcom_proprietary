////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxfeature.h
/// @brief CHX feature base class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXFEATURE_H
#define CHXFEATURE_H

#include <assert.h>

#include "chxincs.h"
#include "chxusecase.h"
#include "chxusecaseutils.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

////////////////////////
/// Forward Declarations
////////////////////////
class AdvancedCameraUsecase;

enum AdvancedPipelineType
{
    ZSLSnapshotJpegType = 0,        // snapshot to JPEG
    ZSLSnapshotYUVType,             // snapshot to YUV
    InternalZSLYuv2JpegType,        // YUV to JPEG
    InternalZSLYuv2JpegMFNRType,    // YUV to JPEG for MFNR
    Merge3YuvCustomTo1YuvType,      // merge 3 YUV to 1 YUV
    ZSLPreviewRawType,              // preview
    ZSLPreviewRawFSType,            // FS preview
    ZSLPreviewRawYUVType,           // preview+Video
    MFNRPrefilterType,              // MFNR prefilter
    MFNRBlendType,                  // MFNR blend
    MFNRPostFilterType,             // MFNR post filter
    SWMFMergeYuvType,               // SW multi frame
    SWMFMergeRawType,               // SW multi frame
    ZSLSnapshotYUVAuxType,          // Bayer to Yuv for aux
    InternalZSLYuv2JpegMFNRAuxType, // YUV to JPEG for MFNR Aux
    MFNRPrefilterAuxType,           // MFNR prefilter Aux
    MFNRBlendAuxType,               // MFNR blend Aux
    MFNRPostFilterAuxType,          // MFNR post filter Aux
    ZSLYuv2YuvType,                 // Yuv to Yuv reprocess
    ZSLSnapshotJpegGPUType,         // snapshot to GPU to JPEG
    InternalZSLYuv2JpegMFSRType,    // YUV to JPEG for MFSR
    MFSRPrefilterType,              // MFSR prefilter
    MFSRBlendType,                  // MFSR blend
    MFSRPostFilterType,             // MFSR post filter
    InternalZSLYuv2JpegMFSRAuxType, // YUV to JPEG for MFSR Aux
    MFSRPrefilterAuxType,           // MFSR prefilter Aux
    MFSRBlendAuxType,               // MFSR blend Aux
    MFSRPostFilterAuxType,          // MFSR post filter Aux
    QuadCFAFullSizeRawType,         // QuadCFA full size raw
    QuadCFARemosaicType,            // QuadCFA remosaic snapshot
    QuadCFASnapshotYuvType,         // QuadCFA Yuv snapshot (bps-ipe)
    OfflineNoiseReprocessType,      // Offline Noise Reprocess
    OfflineNoiseReprocessAuxType,   // Offline Noise Reprocess Aux
    ZSLSnapshotYUVHALType,          // SnapshotYUV pipeline for HAL
    PipelineCount                   // the pipelines count
};

enum FeatureType
{
    UNINIT = 0,
    ZSL    = 1,
    SWMF   = 2,
    MFNR   = 3,
    HDR    = 4,
    MFSR   = 5,
    QuadCFA = 6,
    Feature2 = 7,
};


enum class SnapshotProcessType
{
    COMMON  = 0,        // non special process type
    MultiFrame,         // Hardware Multi frame process type
    SoftwareMultiframe, // Software multi frame process type
    HDR,                // HDR process type
    B2Y,                // Bayer to Yuv process type
    FUSION,             // Fusion snapshot
    BOKEH               // Bokeh snapshot
};
enum class FeatureRequestType
{
    COMMON  = 0,
    LLS,
    InSensorHDR3Exp,
};

enum class FeatureStatus
{
    NOTINIT     = 0,
    READY       = 1,
    BUSY        = 2,
};

enum CameraType
{
    Wide = 0,
    Tele = 1,
    Count
};

/// Metadata pair associated with the request
struct ChiMetadataRequestInfo
{
    ChiMetadata* pInputMetadata;    ///< Input metadata object pointer
    ChiMetadata* pOutputMetadata;   ///< Output metadata object pointer
};

struct FeatureRequestInfo
{
    UINT32                    numOfRequest;                              ///< Number of input buffers needes
    camera3_capture_request_t request[MaxOutstandingRequests];           ///< Request array from feature2
    BOOL                      isReturnResult[MaxOutstandingRequests];    ///< If it is required to return result
    ChiMetadataRequestInfo    metadataInfo[MaxOutstandingRequests];      ///< Parital metadata required
    BOOL                      isFdStreamRequired;                        ///< If Fd stream is required
};

/// Chi stream and request information for the request
struct FeatureRequestInfoForStream
{
    ChiStream*           pStream;               /// A pointer to ChiStream object for the request
    FeatureRequestInfo   featureRequestInfo;    /// Feature request information
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Base Feature class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Feature
{
public:
    virtual ChiUsecase* OverrideUsecase(
        LogicalCameraInfo*              pCameraInfo,
        camera3_stream_configuration_t* pStreamConfig) = 0;

    virtual CDKResult ExecuteProcessRequest(
        camera3_capture_request_t* pRequest) = 0;

    virtual VOID  ProcessResult(
        CHICAPTURERESULT* pResult,
        VOID*             pPrivateCallbackData) = 0;

    virtual VOID ProcessMessage(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
        VOID*                       pPrivateCallbackData) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessCHIPartialData
    ///
    /// @brief  This will be called by the usecase if CombinedPartialMeta is supported
    ///         Here all the CHI Partial Metadata should be populated and sent to framework as required
    ///
    /// @param  frameNum   Frame number for which the CHI Partial data should be populated
    /// @param  sessionId  Corresponding Session Id
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ProcessCHIPartialData(
        UINT32    frameNum,
        UINT32    sessionId) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessDriverPartialCaptureResult
    ///
    /// @brief  This will be called by the usecase when Partial Result is available from the driver
    ///
    /// @param  pResult                 Partial result from the driver
    /// @param  pPrivateCallbackData    Private Data managed by the client
    ///
    /// @return VOID
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ProcessDriverPartialCaptureResult(
        CHIPARTIALCAPTURERESULT*    pResult,
        VOID*                       pPrivateCallbackData) = 0;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// WaitUntilReady
    ///
    /// @brief  Blocks until feature status is FeatureStatus::Ready or timeoutTime is met
    ///
    /// @param  timeoutTime   The amount of time in milliseconds to wait until ready
    ///
    /// @return CDKResultSuccess on ready, otherwise error
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult WaitUntilReady(UINT timeoutTime);

    virtual VOID Destroy(BOOL isForced) = 0;

    virtual VOID Pause(BOOL isForced) = 0;

    virtual VOID Flush();

    virtual VOID PipelineCreated(
        UINT32 sessionId,
        UINT32 pipelineId)
    {
        (void)sessionId;
        (void)pipelineId;
    }

    virtual VOID PipelineDestroyed(
        UINT32 sessionId,
        UINT32 pipelineId)
    {
        (void)sessionId;
        (void)pipelineId;
    }

    virtual CDKResult GetRequestInfo(
        camera3_capture_request_t* pRequest,
        FeatureRequestInfo*        pOutputRequests,
        FeatureRequestType         requestType)
    {
        (VOID)pRequest;
        (VOID)pOutputRequests;
        (VOID)requestType;
        return CDKResultSuccess;
    }

    virtual CDKResult GetRequestInfo(
        camera3_capture_request_t*               pRequest,
        std::vector<FeatureRequestInfoForStream> &pOutputRequests,
        FeatureRequestType                       requestType)
    {
         (VOID)pRequest;
         (VOID)pOutputRequests;
         (VOID)requestType;

         return CDKResultSuccess;
    }

    virtual FeatureType GetFeatureType()
    {
        return FeatureType::UNINIT;
    }

    FeatureStatus GetFeatureStatus()
    {
        return static_cast<FeatureStatus>(ChxUtils::AtomicLoadU32(&m_aFeatureStatus));
    }

    virtual VOID SetFeatureStatus(FeatureStatus currentstate);

    virtual BOOL StreamIsInternal(
        ChiStream* pStream)
    {
        (VOID)pStream;

        return FALSE;
    }

    // Notification function once the usecase is created by the AUC class
    virtual VOID PostUsecaseCreated() {}

    virtual UINT32 GetMaxRequiredFramesForSnapshot(
        const camera_metadata_t *pMetadata)
    {
        (VOID)pMetadata;

        return 1;
    }
    virtual UINT32 GetRequiredFramesForSnapshot(
        const camera_metadata_t *pMetadata)
    {
        (VOID)pMetadata;

        return 1;
    }

    /*
     * fill the required pipeline ids to the given pPipelines
     *
     * return the number of pipelines have been filled
     */
    virtual INT32 GetRequiredPipelines(
        AdvancedPipelineType* pPipelines,
        INT32                 size)
    {
        (VOID)pPipelines;
        (VOID)size;

        return 0;
    }

    /*
     * Set physical camera index for this feature
     */
    CHX_INLINE VOID SetPhysicalCameraIndex(UINT32 index)
    {
        m_physicalCameraIndex = index;
    }

    /*
     * Get physical camera index for this feature
     */
    CHX_INLINE UINT32 GetPhysicalCameraIndex() const
    {
        return m_physicalCameraIndex;
    }

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializePrivateResources
    ///
    /// @brief  Initializes private resources belonging to Feature
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID    InitializePrivateResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroyPrivateResources
    ///
    /// @brief  Destroys private resources belonging to Feature
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID    DestroyPrivateResources();

    virtual ~Feature() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InvalidateRequest
    ///
    /// @brief  Invalidates the input and output references for an offline request
    ///
    /// @param  pSubmitRequest   A pointer to the request to invalidate
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InvalidateRequest(
        CHIPIPELINEREQUEST* pSubmitRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputBufferManager
    ///
    /// @brief Get the Output Buffer Manager object _desc_here
    ///
    /// @param  pOutputBuffer   A pointer to the output buffer whose buffer manager is unknown
    ///
    /// @return Pointer to the output buffer manager or NULL
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CHIBufferManager* GetOutputBufferManager(
        CHISTREAMBUFFER* pOutputBuffer)
    {
        CDK_UNUSED_PARAM(pOutputBuffer);
        return NULL;
    }

    UINT32                     m_physicalCameraIndex;               ///< Physical camera index for this feature
    ChiMetadataManager*        m_pMetadataManager;                  ///< Pointer to the metadata manager
    AdvancedCameraUsecase*     m_pUsecase;                          ///< Pointer to owning usecase class
    CHIMETAHANDLE              m_featureOutputMetaHandle;           ///< Handle for storing output meta from different stages.
private:

    VOID ReleaseInputReferences(
        CHIPIPELINEREQUEST* pSubmitRequest);

    VOID ReleaseOutputReferences(
        CHIPIPELINEREQUEST* pSubmitRequest);

    volatile UINT32            m_aFeatureStatus;                    ///< Feature status
    Mutex*                     m_pStateLock;                        ///< Lock used for FeatureState update signals
    Condition*                 m_pReadyCondition;                   ///< Condition used to broadcast feature ready status

};

#endif // CHXFEATURE_H
