////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2018-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecasedual.h
/// @brief Usecases class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXUSECASEDUAL_H
#define CHXUSECASEDUAL_H

#include <assert.h>
#include <map>
#include <mutex>
#include <vector>
#include <unordered_set>
#include <unordered_map>

#include "chxincs.h"
#include "chxmulticamcontroller.h"
#include "chxpipeline.h"
#include "chxsession.h"
#include "chxusecaseutils.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Dual-camera usecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class UsecaseDualCamera final : public Usecase
{
public:

    /// Static create function to create an instance of the object
    static UsecaseDualCamera* Create(
        LogicalCameraInfo*              cameraInfo,       ///< Camera Info
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream configuration;

    static const UINT32  REALTIMESESSIONIDX    = 0;
    static const UINT32  MAX_REALTIME_PIPELINE = 2;
    static const UINT32  REALTIMEPIPELINEMAIN  = 0;
    static const UINT32  REALTIMEPIPELINEAUX  = 1;
    static const UINT32  REALTIMEPIPELINELOGICAL = 10;
    static const UINT32  DUMMYWIDTH = 640;
    static const UINT32  DUMMYHEIGHT = 480;
    static const UINT32  MAX_NUM_YUV_STREAMS = 3;

    static const UINT32  MetadataOutputNone = 0;             /// Do not send metadata to app
    static const UINT32  MetadataOutputPhysical = 1 << 0;    /// Send metadata to app as physical resut
    static const UINT32  MetadataOutputLogical = 1 << 1;     /// Send metadata to app as logical result

protected:
    /// Destroy/Cleanup the object
    virtual VOID Destroy(BOOL isForced);
private:

    UsecaseDualCamera() = default;
    virtual ~UsecaseDualCamera();

    // Do not allow the copy constructor or assignment operator
    UsecaseDualCamera(const UsecaseDualCamera&) = delete;
    UsecaseDualCamera& operator= (const UsecaseDualCamera&) = delete;

    static VOID SessionCbCaptureResult(
        ChiCaptureResult* pCaptureResult,                       ///< Capture result
        VOID*             pPrivateCallbackData);                ///< Private callback data

    static VOID SessionCbPartialCaptureResult(
        ChiPartialCaptureResult* pPartialCaptureResult,         ///< Partial capture result
        VOID*                    pPrivateCallbackData);         ///< Private callback data

    static VOID SessionCbNotifyMessage(
        const ChiMessageDescriptor* pMessageDescriptor,         ///< Message Descriptor
        VOID*                       pPrivateCallbackData);      ///< Private callback data

    /// This will be called by the usecase when Partial Result is available from the driver
    VOID ProcessDriverPartialCaptureResult(
        CHIPARTIALCAPTURERESULT*    pResult);

    /// This will be called by the usecase when Partial Result is available from CHI
    VOID ProcessCHIPartialData(
        UINT32    frameNum,
        UINT32    sessionId);

    VOID ProcessAndReturnPartialMetadataFinishedResults(PartialResultSender partialResultSender);

    /// Execute capture request
    CDKResult ExecuteCaptureRequest(
        camera3_capture_request_t* pRequest);                  ///< Request parameters

    // Implemented by the derived class to process the saved results
    VOID ProcessResults();

    /// Process the available results
    /// Dispatch a capture request with preview only
    CDKResult GenerateRealtimeRequest(
        camera3_capture_request_t* pRequest);                 ///< Request parameters

    CDKResult GenerateInternalRequest(
            UINT32 sessionId, UINT numRequest, CHICAPTUREREQUEST* pRequest);

    //Remaps the pipeline index coming from usecase to CHI expected way
    UINT32 RemapPipelineIndex(
            UINT32              pipelineIndexFromUsecase);        ///< Pipeline index from usecase

    //Convert use case pipeline id for physical camera id
    UINT32 GetUsecasePipelineId(
            const char*         physicalCameraIdStr);             ///< Physical camera id from framework

    //Match use case's chi streams to the application configured streams
    CDKResult MatchUsecaseChiStreams(
            const ChiUsecase* pChiUsecase,                        ///< Chi use case
            const camera3_stream_configuration_t* pStreamConfig); ///< Stream configuration set by app

    /// Does one time initialization of the created object
    CDKResult Initialize(
        LogicalCameraInfo*              cameraInfo,           ///< Camera Info
        camera3_stream_configuration_t* pStreamConfig);       ///< Stream configuration

    //Update vendor tags for pipeline
    CDKResult UpdateVendorTags(
            ChiMetadata* settings, UINT pipelineId);

    //Remap logical camera setting/result metadata
    CDKResult RemapLogicalSettings(ChiMetadata* metadata, UINT32 pipelineId);
    CDKResult RemapLogicalResultMetadata(ChiMetadata* metadata, UINT32 pipelineId);

    struct SessionPrivateData
    {
        Usecase* pUsecase;  ///< Per usecase class
        UINT32   sessionID; ///< Session Id that is meaningful to the usecase in which the session belongs
    };

    SessionPrivateData      m_perSessionPvtData[MaxSessions];               ///< Per session private data
    Session*                m_pSession[MaxSessions];                        ///< Session
    Pipeline*               m_pPipeline[MaxPipelinesPerSession];            ///< Pipelines
    UINT64                  m_shutterTimestamp[MaxOutstandingRequests];     ///< Is shutter message sent
    INT64                   m_shutterFrameNum;                              ///< last framenum processed.
    CHIPRIVDATA             privRealTime1[MaxOutstandingRequests];
    CHIPRIVDATA             privRealTime2[MaxOutstandingRequests];

    struct PendingChiResult {
        // The pipeline ID that the logical capture_result maps to.
        // It must be in requestedPipelines vector below.
        UINT32 logicalPipelineId;
        // The list of active pipeline IDs for the current request.
        std::unordered_set<UINT32> requestedPipelineIds;
        // Each pipeline's metadata output mask
        UINT32 metadataMasks[MAX_REALTIME_PIPELINE];

        // The list of output and input buffers
        std::vector<CHISTREAMBUFFER> outputBuffers;
        std::vector<CHISTREAMBUFFER> inputBuffer;
        // Logical metadata and physical metadatas
        VOID* logicalMetadata;
        std::vector<VOID*> physicalMetadata;
        // Physical camera IDs
        std::vector<std::string> physicalIds;

        PendingChiResult(UINT32 metadataOutput0, UINT32 metadataOutput1,
                const std::unordered_set<UINT32>& requestedIds)
                : requestedPipelineIds(requestedIds),
                  logicalMetadata(nullptr) {
            metadataMasks[REALTIMEPIPELINEMAIN] = metadataOutput0;
            metadataMasks[REALTIMEPIPELINEAUX] = metadataOutput1;
            logicalPipelineId = (metadataOutput0 & MetadataOutputLogical) ?
                    REALTIMEPIPELINEMAIN : REALTIMEPIPELINEAUX;
        }

        BOOL handleCaptureResult(UsecaseDualCamera* usecase,
            const ChiCaptureResult& captureResult,
            std::string physicalId);
    };

    // The lock for m_pendingCaptureResult array.
    std::mutex              mPendingResultLock;
    // The pending final capture results.
    std::map<UINT32, PendingChiResult> mPendingChiResults;

    CHISTREAM*              m_pDummyYuvStream[MAX_REALTIME_PIPELINE][MAX_NUM_YUV_STREAMS];   ///< Dummy YUV stream
    CHISTREAM*              m_pDummyRawStream[MAX_REALTIME_PIPELINE];       ///< Dummy raw stream
    CHISTREAM*              m_pDummyJpegStream[MAX_REALTIME_PIPELINE];       ///< Dummy jpeg stream

    const LogicalCameraInfo* m_pCameraInfo;
    UINT32                  m_LogicalCameraPipelineId;
    BOOL                    m_IsFirstRequest[MAX_REALTIME_PIPELINE];
    BOOL                    m_HasPhysicalStream;

    UINT32                  m_effectModeValue[MAX_REALTIME_PIPELINE];       ///< effect value
    UINT32                  m_sceneModeValue[MAX_REALTIME_PIPELINE];        ///< scenemode value
    UINT32                  m_tuningFeature1Value[MAX_REALTIME_PIPELINE];   ///< tuning Feature1Value value
    UINT32                  m_tuningFeature2Value[MAX_REALTIME_PIPELINE];   ///< tuning Feature2Value value

    ///< Copies of camera3_stream_t structures for each internal pipelines.
    std::vector<CHISTREAM*> m_pInternalStreams[MAX_REALTIME_PIPELINE];
    ///< Mapping from framework camera3_stream_t struct to index in the internal
    //streams array.
    std::unordered_map<const CHISTREAM*, UINT> m_IncomingStreamMap;
    ///< Mapping from internal camera3_stream_t struct to framework
    //camera3_stream_t struct.
    std::unordered_map<const CHISTREAM*, const CHISTREAM*> m_OutgoingStreamMap;
    CDKResult ModifyCaptureRequestStream(CHICAPTUREREQUEST& chiStream, UINT32 pipelineId);
    CDKResult ModifyCaptureResultStream(ChiCaptureResult* chiResult);

    CHISTREAMBUFFER outputBuffers0[10];
    CHISTREAMBUFFER outputBuffers1[10];
};

#endif // CHXUSECASEMC_H
