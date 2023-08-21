////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxadvancedcamerausecase.h
/// @brief CHX Advanced camera usecase class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXADVANCEDCAMERAUSECASE_H
#define CHXADVANCEDCAMERAUSECASE_H

#include "chistatspropertydefines.h"
#include "chxdefs.h"
#include "chxextensionmodule.h"
#include "chxfeature.h"
#include "chxpipeline.h"
#include "chxusecase.h"
#include "chxusecaseutils.h"
#include "cdkutils.h"
#include "chistatsproperty.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

#define DYNAMIC_FEATURE_SWITCH 1

const static INT    MaxFeatureSupported    = 10;
/// Threshold of ASD severity, it can be set by OEM
const static UINT32 ThresholdOfASDSeverity = 50;
const static UINT32 MaxRealTimePipelines   = 8;
const static UINT32 MaxPipelines           = 256;
const static UINT32 PairCount              = 2;
/// InstanceOffset is used to generate different pipeline ID for different feature instance
const static UINT32 InstanceOffset         = 6;
const static UINT32 PipelineTypeMask       = ((1 << InstanceOffset) -1);
const static UINT32 InstanceMask           = (0x7 << InstanceOffset);
const static UINT32 MaxFeaturesPerSnapshot = 2; ///< max features supported for one snapshot request

#define GET_PIPELINE_TYPE_BY_ID(pipelineID)    ((pipelineID) & PipelineTypeMask)
#define GET_FEATURE_INSTANCE_BY_ID(pipelineID) (((pipelineID) & InstanceMask) >> InstanceOffset)

/// Forward declarations
struct ChiPipelineTargetCreateDescriptor;
class  Session;

/// @brief Mapping of pipelines included in a session
struct SessionPipelines
{
    UINT pipelineId[MaxPipelinesPerSession];  ///< Indices of pipelines in the usecase
    UINT numPipelines;                        ///< Count of valid entries in pipelineId
};

struct PipelineStatus
{
    UINT32   pipelineId;
    BOOL     used;
};

struct FeatureInputInfo
{
    BOOL   valid;                                 ///< If the data is valid or not
    UINT32 inputBufferQIdx;                       ///< Input target buffer index
    UINT32 numOfInputBuffers;                     ///< Number of input buffers
    UINT32 lastSeqId;                             ///< Last ready SeqId of input target buffer
    UINT32 sensorModeIndex;                       ///< Used for filling tunning data
};

struct FeatureOutputInfo
{
    BOOL       valid;                             ///< If the data is valid or not
    UINT32     NumOfOutputTargets;                ///< For output, may support multi output target streams/buffers
    UINT32     outputTargetIndex[MaxChiStreams];  ///< Index of all output targets
    CHISTREAM* outputStreams[MaxChiStreams];      ///< Chi streams of output buffers
};

struct SnapshotFeatureInfo
{
    Feature*          pFeature;                   ///< Internal frame number for this request
    FeatureInputInfo  featureInput;               ///< Internal frame number for this request
    FeatureOutputInfo featureOutput;              ///< Internal frame number for this request
};

struct SnapshotFeatureList
{
    UINT32              appFrameNum;                          ///< App frame number for this snapshot request
    UINT32              numOfFeatures;                        ///< Number of Fetaures used for this snapshot request
    SnapshotFeatureInfo featureInfo[MaxFeaturesPerSnapshot];  ///< Feature input/output info
};

struct SessionRequestIdFeatureMapping
{
    UINT        sessionId;
    UINT64      requestId;
    Feature*    pFeature;
};

/// @brief Quadra CFA sensor specific info
struct QuadCFASensorInfo
{
    ChiSensorModePickHint sensorModePickHint;    ///< Flags used for sensor mode selection
    ChiDimension          fullSizeSensorOutput;  ///< Sensor full size mode output dimension
    // RemosaicType remosaicType;                ///< Quadra CFA sensor remosaic type
};

struct OfflineInputInfo
{
    CHISTREAMBUFFER         RDIBufferArray[MaxRealTimePipelines][BufferQueueDepth]; ///< Chi Stream buffer will be the buffer array for offline process
    CHISTREAMBUFFER         FDBufferArray[MaxRealTimePipelines][BufferQueueDepth];  ///< Chi Stream buffer will be the buffer array for offline process
    ChiMetadata*            pRDIMetadata[MaxRealTimePipelines][BufferQueueDepth];   ///< Metadata array will be the metadata array for offline process
    UINT32                  numOfBuffers;                                           ///< number of buffers needed for offline process
    UINT32                  inputFrameNumber;                                       ///< Indicate which buffer will be picked for offline process
    UINT32                  frameNumber;                                            ///< Indicate current snapshot frame number
    ChiMetadata*            pAppMetadata;                                           ///< Application metadata
    UINT32                  featureMask;                                            ///< Mask to to map camera and feature
    UINT32                  snapshotMask;                                           ///< Mask to to map camera and snapshot
    BOOL                    manualMode;                                             ///< if TRUE, we need frame with new setting for snapshot
    BOOL                    dualZoneRequest;                                        ///< Is this a snapshot fusion request
    BOOL                    isFDStreamRequired;                                     ///< If FD stream is required
};

struct RequestMapInfo
{
    UINT32                    frameNumber;                     ///< Internal frame number for this request
    BOOL                      isPreviewReturnNeeded;           ///< Whether to return preview frame to framework
    BOOL                      isSnapshotReturnNeeded;          ///< Whether to return snapshot frame to framework
    BOOL                      isShutterReturnNeeded;           ///< Whether to return shutter to framework
    BOOL                      isMetaReturnNeeded;              ///< Whether to return metadata to framework
    BOOL                      triggerOfflineReprocess;         ///< If need to trigger offline request
    UINT32                    inputFrameNumber;                ///< Input frame number
    Feature*                  pFeature;                        ///< Feature used to process this request
    UINT32                    masterCameraID;                  ///< Master Camera ID
    UINT32                    activePipelineID;                ///< Active Pipeline ID
    BOOL                      isSkipPreview;                   ///< If skip preview frame or not
    UINT32                    snapshotRequestID;               ///< Snapshot request ID
    UINT32                    numOfSnapshotBuffers;            ///< Numbers of Snapshot Buffers.
    BOOL                      isFusionZone;                    ///< If fusion zone or not
    BOOL                      isFDStreamRequired;              ///< IF FD buffer is required
    SnapshotProcessType       snapshotProcessType;             ///< Snapshot process type
    camera3_capture_request_t requestForFeature2;              ///< Capture Request Settings
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Intermediate Class representing the base of most camera operations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CameraUsecaseBase : public Usecase
{
public:
    static ChiUsecase* GetXMLUsecaseByName(const CHAR* usecaseName);

    CHX_INLINE ChiUsecase* GetChiUseCase()
    {
        return m_pChiUsecase;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InvalidateFrameNumber
    ///
    /// @brief  Invalidate the frame number so that processing can continue
    ///
    /// @param  chiFrameNumber  chi frame number to invalidate
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID InvalidateFrameNumber(
        UINT chiFrameNumber)
    {
        ChiMessageDescriptor messageDescriptor;
        messageDescriptor.messageType                            = ChiMessageTypeError;
        messageDescriptor.message.errorMessage.frameworkFrameNum = chiFrameNumber;
        messageDescriptor.message.errorMessage.errorMessageCode  = MessageCodeRequest;
        messageDescriptor.message.errorMessage.pErrorStream      = NULL;

        ProcessErrorMessage(&messageDescriptor);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSessionId
    ///
    /// @brief  Get session id from Session Id Handle
    ///
    /// @param  phSession  A pointer to a session handle
    ///
    /// @return CDKResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT GetSessionId(
        CHIHANDLE phSession) const
    {
        UINT result = InvalidSessionId;
        for (UINT sessionId = 0; sessionId < MaxSessions; sessionId++)
        {
            if (phSession == m_sessions[sessionId].pSession->GetSessionHandle())
            {
                result = sessionId;
                break;
            }
        }
        CHX_ASSERT(result != InvalidSessionId);
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPipelineId
    ///
    /// @brief  Get pipeline id from sessionId and handle
    ///
    /// @param  sessionId        The id of the session
    /// @param  phPipelineHandle A pointer to the pipeline handle
    ///
    /// @return CDKResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT GetPipelineId(
        UINT sessionId,
        CHIPIPELINEHANDLE phPipelineHandle)
    {
        UINT result = INVALID_INDEX;

        if (sessionId < MaxSessions)
        {
            for (UINT pipelineId = 0; pipelineId < MaxPipelinesPerSession; pipelineId++)
            {
                if (phPipelineHandle == m_sessions[sessionId].pipelines[pipelineId].pPipeline->GetPipelineHandle())
                {
                    result = pipelineId;
                    break;
                }
            }
        }
        CHX_ASSERT(result != INVALID_INDEX);
        return result;
    }

    CHX_INLINE const PipelineData* GetPipelineData(
        UINT sessionId,
        UINT pipelineId) const
    {
        const PipelineData* result = NULL;

        if ((sessionId < MaxSessions) && (pipelineId < MaxPipelinesPerSession))
        {
            result = &m_sessions[sessionId].pipelines[pipelineId];
        }
        return result;
    }

    // Get Metdata Client Id
    CHX_INLINE UINT32 GetMetadataClientIdFromPipeline(
        UINT32      sessionId,
        UINT32      pipelineId)
    {
        UINT32        clientId      = 0;
        PipelineData* pPipelineData = const_cast<PipelineData*>(GetPipelineData(sessionId, pipelineId));

        if (NULL != pPipelineData)
        {
            clientId = pPipelineData->pPipeline->GetMetadataClientId();
        }

        return clientId;
    }

    CHX_INLINE const SessionData* GetSessionData(
        UINT sessionId
    ) const
    {
        return &m_sessions[sessionId];
    }

    CHX_INLINE TargetBuffer* GetTargetBufferPointer(
        UINT bufferId)
    {
        TargetBuffer* pTargetBuffer = NULL;

        if (FALSE == IsMultiCameraUsecase())
        {
            if (bufferId < MaxNumOfTargets)
            {
                pTargetBuffer = &m_targetBuffers[bufferId];
            }
        }
        else
        {
            // Take target buffer pointer from m_MCTargetBuffer only if
            // multicamera usecase AND Rdi target buffer
            if (bufferId < MaxChiStreams)
            {
                pTargetBuffer = &m_MCTargetBuffer[bufferId];
            }
        }

        return pTargetBuffer;
    }

    CHX_INLINE VOID SetShutterTimestamp(
        UINT64 frameNumber,
        UINT64 timeStamp)
    {
        m_shutterTimestamp[frameNumber % MaxOutstandingRequests] = timeStamp;
    }

    CHX_INLINE UINT64 GetShutterTimestamp(
        UINT64 frameNumber)
    {
        return m_shutterTimestamp[frameNumber % MaxOutstandingRequests];
    }

    CHX_INLINE BOOL IsMetadataSent(
        UINT requestId
    ) const
    {
        return m_isMetadataSent[requestId];
    }

    CHX_INLINE VOID SetMetadataAvailable(
        UINT requestId)
    {
        m_isMetadataAvailable[requestId] = TRUE;
    }

    CHX_INLINE VOID SetFinalPipelineForPartialMetaData(
        UINT pipelineId)
    {
        m_finalPipelineIDForPartialMetaData = pipelineId;
    }

    CHX_INLINE UINT GetFinalPipelineForPartialMetaData()
    {
        return m_finalPipelineIDForPartialMetaData;
    }

    CHX_INLINE CDKResult SetPipelineToSessionMapping(
        UINT        numSessions,
        UINT        numPipelines,
        const UINT* pMapping)
    {
        CDKResult result = CDKResultSuccess;

        m_numSessions        = numSessions;
        m_pPipelineToSession = reinterpret_cast<UINT*>(CHX_CALLOC(sizeof(UINT) * numPipelines));

        if (NULL != m_pPipelineToSession)
        {
            ChxUtils::Memcpy(m_pPipelineToSession, pMapping, sizeof(UINT) * numPipelines);
        }
        else
        {
            result = CDKResultENoMemory;
        }

        return result;
    }

    CHX_INLINE CDKResult SetPipelineToCameraMapping(
        UINT        numPipelines,
        const UINT* pMapping)
    {
        CDKResult result = CDKResultSuccess;

        m_pPipelineToCamera = reinterpret_cast<UINT*>(CHX_CALLOC(sizeof(UINT) * numPipelines));

        if (NULL != m_pPipelineToCamera)
        {
            ChxUtils::Memcpy(m_pPipelineToCamera, pMapping, sizeof(UINT) * numPipelines);
        }
        else
        {
            result = CDKResultENoMemory;
        }

        return result;
    }
    CHX_INLINE const RequestMapInfo GetRequestMapInfo(
        UINT32      requestFrameNum) const
    {
        return m_requestMapInfo[requestFrameNum % MaxOutstandingRequests];
    }

    CHX_INLINE OfflineInputInfo* GetOfflineInputInfo(UINT32          requestFrameNum)
    {
        RequestMapInfo requestInfo   = m_requestMapInfo[requestFrameNum % MaxOutstandingRequests];
        UINT32 snapshotReqID         = requestInfo.snapshotRequestID;
        UINT32 pipelineIndex         = requestInfo.activePipelineID;
        UINT32 snapIndex             = snapshotReqID % MaxOutstandingRequests;
        return &(m_snapshotInputInfo[snapIndex]);
    }

    // Function to set RDI tags onto snapshot metadata
    VOID UpdateSnapshotMetadataWithRDITags(
        ChiMetadata& inputMeta,
        ChiMetadata& outputMeta);

    UINT32 GetInternalTargetBufferIndex(
        ChiStream* pChiStream);

    VOID GenerateInternalBufferIndex();

    VOID CreateInternalBufferManager();

    VOID DestroyInternalBufferQueue(
        UINT32 bufferID);

    VOID DestroyAllInternalBufferQueues();

    CDKResult AddNewInternalBufferQueue(
        ChiStream*                  pChiStream,
        CHIBufferManagerCreateData* pBufferCreateData,
        UINT32*                     pBufferID,
        const CHAR*                 pBufferManagerName);

    CHIBufferManager* GetBufferManager(
        UINT32 pipelineIndex);

    CHX_INLINE CHIBufferManager* GetFDBufferManager(
        UINT32 pipelineIndex)
    {
        TargetBuffer*     pTargetBuffer        = NULL;
        CHIBufferManager* pTargetBufferManager = NULL;

        if (pipelineIndex < MaxChiStreams)
        {
            if (FALSE == IsMultiCameraUsecase())
            {
                pTargetBuffer = GetTargetBufferPointer(pipelineIndex);
            }
            else
            {
                pTargetBuffer = GetTargetBufferPointer(m_FDBufferID[pipelineIndex]);
            }
        }

        if (NULL != pTargetBuffer)
        {
            pTargetBufferManager = pTargetBuffer->pBufferManager;
        }
        else
        {
            CHX_LOG_WARN("pTargetBuffer is NULL");
        }

        return pTargetBufferManager;
    }

    CDKResult ReleaseRDIBufferReference(
        UINT32          pipelineIndex,
        CHIBUFFERINFO*  pBufferInfo);

    VOID ReserveBufferQueueSlot(
        UINT32          frameNumber,
        UINT32          pipelineIndex,
        CHIBUFFERHANDLE phBuffer);

    // Clear the target buffer queue slot
    VOID ClearBufferQueueSlot(
        UINT32          frameNumber,
        UINT32          pipelineIndex);

    BOOL HasRDIBuffer(
        UINT32 frameNumber,
        UINT32 pipelineIndex);

    CDKResult FlushRDIQueue(
        UINT32 frameNumber,
        UINT32 pipelineIndex,
        BOOL   flushMetadata = TRUE);

    BOOL HasFDBuffer(
        UINT32 frameNumber,
        UINT32 pipelineIndex);

    CDKResult FlushFDQueue(
        UINT32 frameNumber,
        UINT32 pipelineIndex);

    CDKResult ReleaseFDBufferReference(
        UINT32              pipelineIndex,
        CHIBUFFERINFO*      pBufferInfo);

    CDKResult GetOutputBufferFromRDIQueue(
        UINT32              frameNumber,
        UINT32              pipelineIndex,
        CHISTREAMBUFFER*    pOutputbuffer);

    CDKResult GetOutputBufferFromFDQueue(
        UINT32                      frameNumber,
        UINT32                      pipelineIndex,
        CHISTREAMBUFFER*             pOutputbuffer);

    BOOL IsRDIBufferMetaReadyForInput(
        UINT32 frameNumber,
        UINT32 pipelineIndex);

    CDKResult GetInputBufferFromRDIQueue(
        UINT32              frameNumber,
        UINT32              pipelineIndex,
        UINT32              bufferIndex,
        CHISTREAMBUFFER*    pInputBuffer,
        ChiMetadata**       ppMetadata,
        BOOL                releaseBufHdlRef);

    CDKResult GetInputBufferFromFDQueue(
        UINT32                      frameNumber,
        UINT32                      pipelineIndex,
        UINT32                      bufferIndex,
        CHISTREAMBUFFER*            pInputBuffer,
        BOOL                        releaseBufHdlRef);

    ChiMetadata* FillMetadataForRDIQueue(
        UINT32                      frameNumber,
        UINT32                      pipelineIndex,
        ChiMetadata*                pMetadata);

    CDKResult UpdateBufferReadyForRDIQueue(
        UINT32              frameNumber,
        UINT32              pipelineIndex,
        BOOL                isBufferReady);

    CDKResult UpdateBufferReadyForFDQueue(
        UINT32              frameNumber,
        UINT32              pipelineIndex,
        BOOL                isBufferReady);

    CDKResult WaitForBufferMetaReady(
        UINT32 frameNumber,
        UINT32 pipelineIndex);

    CDKResult CreateOfflineInputResource(
        UINT32 requestFrameNum,
        UINT32 pipelineIndex,
        BOOL   bUpdateValidLength);

    CDKResult ReleaseReferenceToInputBuffers(
        CHIPRIVDATA* pPrivData);

    CDKResult ReleaseSingleOffineInputResource(
        UINT32 requestFrameNum,
        UINT32 pipelineIndex,
        UINT32 bufferIndex);

    CDKResult ReleaseSingleOffineFDInputResource(
        UINT32 requestFrameNum,
        UINT32 pipelineIndex,
        UINT32 bufferIndex);

    UINT32 GetValidBufferLength(
        UINT32 pipelineIndex);

    UINT32 GetPipelineIdFromCamId(
        UINT32 physicalCameraId);

    CDKResult UpdateValidRDIBufferLength(
        UINT32 pipelineIndex,
        INT32  delta);

    CDKResult UpdateValidFDBufferLength(
        UINT32 pipelineIndex,
        INT32  delta);

    // Function to release the metadata buffer of targetbuffer back to CMDM
    CDKResult ReleaseMetadataBuffer(
        TargetBuffer*   pTargetBuffer,
        UINT32          releasedBufferIdx);

    // Obtain metadata buffers for the pipeline request
    CDKResult UpdateMetadataBuffers(
        camera3_capture_request_t* pSrcRequest,
        UINT32                     pipelineId,
        CHICAPTUREREQUEST*         pDstRequest,
        UINT32                     sessionIndex,
        UINT32                     pipelineIndex,
        BOOL                       bIsSticky);

    CHX_INLINE UINT32 GetFirstReadyBufferFrameNum(UINT32 pipelineIndex)
    {
        if ((TRUE == IsMultiCameraUsecase()) && (pipelineIndex < MaxChiStreams))
        {
            return m_MCTargetBuffer[m_RDIBufferID[pipelineIndex]].firstReadySequenceID;
        }
        else
        {
            return m_targetBuffers[pipelineIndex].firstReadySequenceID;
        }
    }

    CHX_INLINE UINT32 GetLastReadyFrameNum(UINT32 pipelineIndex)
    {
        if ((TRUE == IsMultiCameraUsecase()) && (pipelineIndex < MaxChiStreams))
        {
            return m_MCTargetBuffer[m_RDIBufferID[pipelineIndex]].lastReadySequenceID;
        }
        else
        {
            return m_targetBuffers[pipelineIndex].lastReadySequenceID;
        }
    }

    CHX_INLINE ChiMetadata* GetLastReadyRDIMetadata(UINT32 pipelineIndex)
    {
        UINT32 lastReadySequenceIndex = 0;
        if ((TRUE == IsMultiCameraUsecase()) && (pipelineIndex < MaxChiStreams))
        {
            lastReadySequenceIndex =  m_MCTargetBuffer[m_RDIBufferID[pipelineIndex]].lastReadySequenceID % BufferQueueDepth;
            return m_MCTargetBuffer[m_RDIBufferID[pipelineIndex]].bufferQueue[lastReadySequenceIndex].pMetadata;
        }
        else
        {
            pipelineIndex           = m_rdiStreamIndex;
            lastReadySequenceIndex  = m_targetBuffers[pipelineIndex].lastReadySequenceID % BufferQueueDepth;
            return m_targetBuffers[pipelineIndex].bufferQueue[lastReadySequenceIndex].pMetadata;
        }
    }

    CHX_INLINE BOOL IsMultiCameraUsecase()
    {
        return (UsecaseId::MultiCamera == GetUsecaseId());
    }

    // Indicates whether batchmode is enabled
    CAMX_INLINE BOOL IsBatchModeEnabled() const
    {
        return ((1 < ExtensionModule::GetInstance()->GetNumBatchedFrames() &&
                 FALSE == ExtensionModule::GetInstance()->GetHALOutputBufferCombined()) ? TRUE : FALSE);
    }

    CHX_INLINE BOOL IsQuadCFAUsecase()
    {
        return (UsecaseId::QuadCFA == GetUsecaseId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiFrameNumFromReqId
    ///
    /// @brief  Translate a pipeline requestId to a Chi Frame Number
    ///
    /// @param  pipelineData  A pointer to the pipeline data
    /// @param  pipelineReqId The requestId to map into a Chi Frame number
    ///
    /// @return               The Chi framenumber associated with this request
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 GetChiFrameNumFromReqId(
        const PipelineData* pipelineData,
        UINT32 pipelineReqId) const
    {
        return pipelineData->seqIdToFrameNum[pipelineReqId % MaxOutstandingRequests];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiFrameNumFromReqId
    ///
    /// @brief Translate a pipeline requestId to a Chi Frame Number
    ///
    /// @param sessionId      The sessionId that owns the pipeline of which pipelineReqId is associated
    /// @param pipelineId     The pipelineId that assigned the pipelineReqId
    /// @param pipelineReqId  The requestId to map into a Chi Frame number
    ///
    /// @return               The Chi framenumber associated with this request
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 GetChiFrameNumFromReqId(
        UINT32 sessionId,
        UINT32 pipelineId,
        UINT32 pipelineReqId) const
    {
        const PipelineData* pipelineData = GetPipelineData(sessionId, pipelineId);

        if (NULL != pipelineData)
        {
            return GetChiFrameNumFromReqId(pipelineData, pipelineReqId);
        }
        else
        {
            CHX_LOG_ERROR("Invalid Pipeline data");
            return INVALIDFRAMENUMBER;
        }

    }


    CHX_INLINE UINT32 GetLastReqId(
        PipelineData* pipelineData) const
    {
        return (pipelineData->seqId);
    }

    // Gets the input buffer and metadata buffer from the target buffer.
    // if pSrcMetadata is not NULL, the destination metadata will contain
    // the source metadata tags as well.
    CDKResult GetTargetBuffer(
        UINT32              frameNumber,
        TargetBuffer*       pTargetBuffer,
        ChiMetadata*        pSrcMetadata,
        CHISTREAMBUFFER*    pInputBuffer,
        ChiMetadata**       ppDstMetadata = NULL);

    CDKResult GetZSLMode(
        UINT32*           zslMode,
        ChiMetadata*      pMetadata);

    BOOL IsFlashRequired(
        ChiMetadata&  metadata);

    VOID ResetMetadataStatus(camera3_capture_request_t* pRequest);

    VOID ProcessAndReturnFinishedResults();

    VOID ProcessAndReturnPartialMetadataFinishedResults(PartialResultSender partialResultSender);

    VOID ReturnFinishedResult(const camera3_capture_result_t *pResult)
    {
        Usecase::ReturnFrameworkResult(pResult, m_cameraId);
    }

    VOID ProcessErrorMessage(
        const ChiMessageDescriptor* pMessageDescriptor);

    virtual VOID ProcessFeatureDone(
        UINT32            internalFrameNum,
        Feature*          pCurrentFeature,
        CHICAPTURERESULT* pResult)
    {
        (VOID)internalFrameNum;
        (VOID)pCurrentFeature;
        (VOID)pResult;
        CHX_LOG_ERROR("ProcessFeatureDone UsecaseBase");
    }

    virtual CHIMETAHANDLE GetAvialableResultMetadata()
    {
        return NULL;
    }


    /// Feature data callback. Features can call this callback to notify any feature data.
    virtual VOID ProcessFeatureDataNotify(
        UINT32   internalFrameNum,
        Feature* pCurrentFeature,
        VOID*    pData)
    {
        (VOID)internalFrameNum;
        (VOID)pCurrentFeature;
        (VOID)pData;
        CHX_LOG_ERROR("ProcessFeatureDataNotify UsecaseBase");
    }
    CHX_INLINE UINT32 GetFeature2InputFrame(UINT32 frameNumber)
    {
        UINT32 frameNumberIndex = frameNumber % MaxOutstandingRequests;
        return m_feature2InputMap[frameNumberIndex];
    }

    CHX_INLINE UINT32 GetRDIInputFrame(UINT32 frameNumber)
    {
        const RequestMapInfo* pRequestInfo  = &(m_requestMapInfo[frameNumber % MaxOutstandingRequests]);
        UINT32 snapshotRequestID            = pRequestInfo->snapshotRequestID;
        OfflineInputInfo*  pInputInfo       = &m_snapshotInputInfo[snapshotRequestID % MaxOutstandingRequests];
        return pInputInfo->inputFrameNumber;
    }

    CHX_INLINE UINT32* GetEffectMode()
    {
        return &m_effectModeValue;
    }

    CHX_INLINE UINT32* GetSceneMode()
    {
        return &m_sceneModeValue;
    }

    CHX_INLINE UINT32* GetFeature1Mode()
    {
        return &m_tuningFeature1Value;
    }

    CHX_INLINE UINT32* GetFeature2Mode()
    {
        return &m_tuningFeature2Value;
    }

    virtual CDKResult ExecuteFlush();

    CDKResult WaitForDeferThread();

    /// Merge offline Tuning/DebugData into metadata
    CDKResult MergeDebugData(
        ChiMetadata*    pMetadata,                                              ///< Ponter to metadata destination
        DebugData*      pOfflineDebugData,                                      ///< Tuning/DebugData to be written in metadata
        const CHAR*     sProcessingStage);                                      ///< Debug helper to print procesing stage

    /// Dump into a file debug data from nodes
    VOID      DumpDebugData(
        const VOID*     pDebugData,                                             ///< Pointer to debug-data to be dump into file
        const SIZE_T    sizeDebugData,                                          ///< Size of debug-data
        const UINT32    sessionId,                                              ///< Session identification number
        const UINT32    cameraId,                                               ///< Corresponding camera ID
        const UINT32    resultFrameNumber);                                     ///< Corresponding result frame

    /// Process debug-data for all usecase and features
    VOID      ProcessDebugData(
        const CHICAPTURERESULT*     pResult,                                    ///< Capture result information
        const VOID*                 pPrivateCallbackData,                       ///< Usefull to get call-back data
        const UINT32                resultFrameNum);                            ///< Corresponding result frame

    CHX_INLINE VOID SetSkipPreviewFlagInRequestMapInfo(
        UINT32  fwkFrameIdx,
        BOOL    value)
    {
        m_requestMapInfo[fwkFrameIdx].isSkipPreview = value;
    }

    CHX_INLINE BOOL GetSkipPreviewFlagInRequestMapInfo(
        UINT32  fwkFrameIdx)
    {
        return m_requestMapInfo[fwkFrameIdx].isSkipPreview;
    }

    BOOL                 m_rejectedSnapshotRequestList[MaxOutstandingRequests]; ///< List of snapshot Rejected Requests

protected:
    static const UINT32 InvalidId       = static_cast<UINT32>(-1);
    static const UINT32 MaxNumOfTargets = ChiMaxNumTargets;

    /// Usecase initialization data
    struct UsecaseInitializationData
    {
        UINT32                             cameraId;                   ///< Camera Id
        UINT32                             numPipelines;               ///< Number of entries in pPipelineTargetCreateDescs
        ChiPipelineTargetCreateDescriptor* pPipelineTargetCreateDescs; ///< Array of complete (with streams) create descriptors
    };

    /// Information about queued request
    struct RequestInfo
    {
        UINT32              frameNumber;
        CHISTREAMBUFFER*    pBuffer;
        const VOID*         pSettings;
    };

    static VOID SessionCbCaptureResult(
        ChiCaptureResult* pCaptureResult,                      ///< Capture result
        VOID*             pPrivateCallbackData);               ///< Private callback data

    static VOID SessionCbPartialCaptureResult(
        CHIPARTIALCAPTURERESULT* pPartialCaptureResult,               ///< Partial Capture result
        VOID*                    pPrivateCallbackData);               ///< Private callback data

    VOID ProcessCHIPartialData(
        UINT32 frameNum);               ///< Frame Number

    static VOID SessionCbNotifyMessage(
        const ChiMessageDescriptor* pMessageDescriptor,        ///< Message Descriptor
        VOID*                       pPrivateCallbackData);     ///< Private callback data

    /// Destroy/Cleanup the object
    virtual VOID Destroy(
        BOOL isForced);

    /// Does one time initialization of the created object
    CDKResult Initialize(
        ChiCallBacks*                   pCallbacks,            ///< Callbacks to be called for each session/pipeline
        camera3_stream_configuration_t* pStreamConfig = NULL);

    /// Called after each pipeline creation for overriding state after each pipeline creation
    virtual VOID PipelineCreated(
        UINT32 sessionId,  ///< Index of the session for which the pipeline was created
        UINT32 pipelineId) ///< Index of the pipeline created (within the context of the session)
    {
        (void)sessionId;
        (void)pipelineId;
        CHX_LOG_ERROR("CameraUsecaseBase pipelineId: %d", pipelineId);
    }

    /// Called after each pipeline creation for overriding state after each pipeline creation
    virtual VOID PipelineDestroyed(
        UINT32 sessionId,  ///< Index of the session for which the pipeline was created
        UINT32 pipelineId) ///< Index of the pipeline created (within the context of the session)
    {
        (void)sessionId;
        (void)pipelineId;
        CHX_LOG_ERROR("CameraUsecaseBase pipelineId: %d", pipelineId);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreatePipelineDescriptorsPerSession
    ///
    /// @brief Create pipeline descriptors for all pipelines in a session
    ///
    /// @param sessionId   Index of the session for which to create its pipeline destriptors
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreatePipelineDescriptorsPerSession(
        UINT32        sessionId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DestroyPipelineDescriptorsPerSession
    ///
    /// @brief Destroy pipeline descriptors for all pipelines in a session
    ///
    /// @param sessionId   Index of the session for which to destroy its pipeline destriptors
    ///
    /// @return CDKResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult DestroyPipelineDescriptorsPerSession(
        UINT32        sessionId);

    CDKResult CreateSessionByID(
        UINT32        sessionId,
        ChiCallBacks* pCallbacks);

    CDKResult DestroySessionByID(
        UINT32 sessionId,
        BOOL   isForced);

    /// Helper for creating sessions based on pipeline mappings
    CDKResult CreateRTSessions(
        ChiCallBacks* pCallbacks);

    /// Helper for creating sessions based on pipeline mappings
    CDKResult CreateOfflineSessions(
        ChiCallBacks* pCallbacks);

    /// Helper for destroy offline sessions
    CDKResult DestroyOfflineSessions();

    /// Helper for CSID binning trigger
    CDKResult CameraCSIDTrigger(CSIDBinningInfo *binninginfo,
        ChiPipelineTargetCreateDescriptor* pPipelineDesc);

    /// Helper for creating a pipeline object from descriptor
    CDKResult CreatePipeline(
        UINT32                              cameraId,
        ChiPipelineTargetCreateDescriptor*  pPipelineDesc,
        PipelineData*                       pPipelineData,
        camera3_stream_configuration_t*     pStreamConfig = NULL);

    virtual const CHISENSORMODEPICKHINT* GetSensorModePickHint(UINT pipelineIndex)
    {
        (VOID)pipelineIndex;

        return NULL;
    }

    /// Execute capture request
    virtual CDKResult ExecuteCaptureRequest(
        camera3_capture_request_t* pRequest);              ///< Request parameters

    /// Used to determine if stream needs buffers allocated
    virtual BOOL StreamIsInternal(
        ChiStream* pStream)
    {
        (void)pStream;

        return FALSE;
    }

    /// Used to determine if stream is FdStream
    virtual BOOL IsFdStream(
        ChiStream* pStream)
    {
        (void)pStream;

        return FALSE;
    }

    /// Used to determine if stream is RdiStream
    virtual BOOL IsRDIStream(
        ChiStream* pStream)
    {
        (void)pStream;

        return FALSE;
    }

    // Checks if metadata update is needed for this request
    bool NeedMetadataUpdate(
       camera3_capture_request_t* pRequest);

    // Function to handle batch mode result
    virtual CDKResult HandleBatchModeResult(
        ChiCaptureResult* pResult,
        ChiMetadata*      pChiOutputMetadata,
        UINT32            resultId,
        UINT32            clientId);

    // Function to prepare HFR Tags that needs to be filterered
    VOID PrepareHFRTagFilterList(UINT32 clientId);

    CameraUsecaseBase() = default;
    virtual ~CameraUsecaseBase();

    UINT         m_numSessions;                                   ///< Represents entry count in m_pSessionPipelines
    UINT*        m_pPipelineToSession;                            ///< Mapping of pipelines to sessions
                                                                  ///  if NULL, 1-1 mapping will be used
    UINT*        m_pPipelineToCamera;                             ///< Mapping of pipelines to cameraId
    BOOL         m_isMetadataAvailable[MaxOutstandingRequests];   ///< Is metadata ready to sent
    BOOL         m_isMetadataSent[MaxOutstandingRequests];        ///< Has metadata been sent already
    TargetBuffer m_targetBuffers[MaxNumOfTargets];                ///< Array of RDI information
    SessionData  m_sessions[MaxSessions];                         ///< Array of session information
    UINT         m_rtSessionIndex;                                ///< Index of session containing last realtime pipeline
    UINT32       m_effectModeValue;                               ///< effect value
    UINT32       m_sceneModeValue;                                ///< scenemode value
    UINT32       m_tuningFeature1Value;                           ///< tuning Feature1Value value
    UINT32       m_tuningFeature2Value;                           ///< tuning Feature2Value value
    ChiStream*   m_pFrameworkOutStreams[MaxChiStreams];           ///< frameworks stream
    ChiStream*   m_pClonedStream[MaxChiStreams];                  ///< reproc internal stream
    BOOL         m_bCloningNeeded;                                ///< cloning case or not
    UINT32       m_numberOfOfflineStreams;                        ///< offline streams count
    TargetBuffer m_MCTargetBuffer[MaxChiStreams];                 ///< target buffer array for multi camera
    UINT32       m_targetBuffersCount;                            ///< target buffer count for multi camera
    UINT32       m_RDIBufferID[MaxChiStreams];                    ///< RDI buffer ID
    BOOL         m_GpuNodePresence;
    CHITAGSOPS   m_vendorTagOps;                                  ///< Vendor Tag Ops
    ChiStream*   m_pRdiStream[MaxRealTimePipelines];                                    ///< Allocated internal RDI stream
    UINT32       m_rdiStreamIndex;                                ///< RDI stream index for single camera usecase
    UINT32       m_fdStreamIndex;                                 ///< FD  stream index for single camera usecase
    ChiStream*   m_pFdStream[MaxRealTimePipelines];                                     ///< Allocated internal FD stream
    UINT32       m_FDBufferID[MaxChiStreams];                     ///< FD buffer ID
    UINT         m_finalPipelineIDForPartialMetaData;             ///< Final Pipeline that will be submitting the metadata


    RequestMapInfo   m_requestMapInfo[MaxOutstandingRequests];    ///< request map information
    OfflineInputInfo m_snapshotInputInfo[MaxOutstandingRequests]; ///< Offline input information
    UINT32           m_feature2InputMap[MaxOutstandingRequests];  ///< Feature2 RDI input frame number
    ChiStream*       m_pYuvInStream;
    UINT64           m_shutterTimestamp[MaxOutstandingRequests];  ///< Shutter timestamp
    UINT32           m_metadataClients[MaxPipelinesPerSession];   ///< Client Ids provided by metadatamanager

    /// Batch mode parameters
    bool                m_isRequestBatchingOn;           ///< Indicates if the batching is enabled
    UINT32              m_batchRequestStartIndex;        ///< Start Index of the batch
    UINT32              m_batchRequestEndIndex;          ///< End Index of the batch
    camera_metadata_t*  m_batchFrameworkOutput;          ///< Framework output for the batch
    ChiMetadata*        m_pBatchMetadataPair[PairCount]; ///< Metadata pair
    UINT32              m_cameraIdMap[MaxRealTimePipelines]; ///< map pipeline index to realtime camera ID
    UINT32              m_numOfPhysicalDevices;            ///< numbers of physical devices
    BOOL                m_isUsecaseCloned;                 ///< flag to indicate if usecase descriptor is cloned

    /// Tag filters per usecase
    std::vector<UINT32> m_hfrTagFilter;        ///< List of tags that must be filtered for HFR batchmode
    std::vector<UINT32> m_hfrTagFilterWithPMD; ///< List of tags and partial metadata tags that must be
                                               ///  filtered for HFR batchmode

private:

    // Do not allow the copy constructor or assignment operator
    CameraUsecaseBase(const CameraUsecaseBase&) = delete;
    CameraUsecaseBase& operator= (const CameraUsecaseBase&) = delete;

    /// Process the saved results
    virtual VOID ProcessResults() { }

    VOID SessionProcessResult(
        ChiCaptureResult*           pCaptureResult,            ///< Result Descriptor
        const SessionPrivateData*   pSessionPrivateData);      ///< Private callback data

    /// Process Partial Result
    VOID SessionPartialCaptureProcessResult(
        ChiPartialCaptureResult*    pPartialCaptureResult,     ///< Message Descriptor
        const SessionPrivateData*   pSessionPrivateData);      ///< Private callback data

    VOID SessionProcessMessage(
        const ChiMessageDescriptor* pMessageDescriptor,        ///< Message Descriptor
        const SessionPrivateData*   pSessionPrivateData);      ///< Private callback data

    CDKResult DeferOfflineSession();

    CDKResult StartDeferThread();

    static VOID* DeferOfflineSessionThread(VOID * pThreadData);

    VOID DestroyDeferResource();

    BOOL PipelineHasHALInputStream(ChiPipelineTargetCreateDescriptor*  pPipelineDesc);

    CDKResult CreateSessionsWithInputHALStream(ChiCallBacks* pCallbacks);

    CDKResult CreateSession(INT sessionId,
        Pipeline** ppPipelines,
        ChiCallBacks* pCallbacks);

    volatile BOOL           m_deferOfflineSessionDone;              ///< Flag indicate if snapshot session
    volatile BOOL           m_deferOfflineThreadCreateDone;         ///< Flag indicate if thread is created
    Condition*              m_pDeferOfflineDoneCondition;           ///< Condition for defer snapshot done
    Mutex*                  m_pDeferOfflineDoneMutex;               ///< Mutex for defer snapshot done
    PerThreadData           m_deferOfflineSessionThreadData;        ///< Thread data for defer
    ChiCallBacks*           m_pCallBacks;                           ///< Notify and captureResult callbacks for each pipeline

    // Dump Debug/Tuning data
    UINT32      m_debugLastResultFrameNumber;                       ///< Helper to track last processed frame number
    UINT32      m_debugMultiPassCount;                              ///< Count proc calls for a single multi-frame capture
    VOID*       m_pEmptyMetaData;                                   ///< Empty MetaData
    UINT32      m_numOfInternalTargetBuffers;                       ///< Number of internal target buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Advanced camera Usecase usecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class AdvancedCameraUsecase : public CameraUsecaseBase
{
public:

    /// Static create function to create an instance of the object
    static AdvancedCameraUsecase* Create(
        LogicalCameraInfo*              pCameraInfo,   ///< Camera info
        camera3_stream_configuration_t* pStreamConfig,  ///< Stream configuration;
        UsecaseId                       usecaseId);     ///< Identifier for the usecase function

    INT32  GetPipelineIdByAdvancedPipelineType(
        AdvancedPipelineType type,
        UINT32               rtIndex);

    INT32  GetAdvancedPipelineTypeByPipelineId(
        INT32 pipelineId);

    UINT64 GetRequestShutterTimestamp(
        UINT64 frameNumber);

    UINT32 GetMaxRequiredFrameCntForOfflineInput(
        UINT32 pipelineIndex);

    VOID   SetRequestToFeatureMapping(
        UINT sessionId,
        UINT64 requestId,
        Feature* pFeature);

    Feature*    GetFeatureFromRequestMapping(
        UINT sessionId,
        UINT64 requestId);

    enum SharedStreamType
    {
        PreviewStream,
        RdiStream,
        RdiStreamAux,
        Bayer2YuvStream,
        JPEGInputStream,
        SnapshotStream,
        YuvCBStream,
        VideoStream,
        YuvInputStream,
        FdStream,
        FdStreamAux,
    };

    FLOAT       m_previewAspectRatio;

    /// Is this a snapshot YUV stream
    CHX_INLINE BOOL IsYUVSnapshotStream()
    {
        return UsecaseSelector::IsYUVSnapshotStream(reinterpret_cast<camera3_stream_t*>(m_pSnapshotStream));
    }

    /// Is this a snapshot JPEG stream
    CAMX_INLINE BOOL IsJPEGSnapshotStream()
    {
        return UsecaseSelector::IsJPEGSnapshotStream(reinterpret_cast<camera3_stream_t*>(m_pSnapshotStream));
    }

    ChiStream*  GetSharedStream(
        SharedStreamType streamType,
        UINT32           pipelineIndex = 0);

    /*
     * return a unique session id for the given pipeline group
     * if any id in the pipelineGroup already has the session id, then
     * all of the pipeline will share this session id
     *
     * return a new session id if none of the pipeline id was associated with any session
     * return the existing if one of the pipeline id is already associated with a session
     */
    UINT32      GetUniqueSessionId(
        AdvancedPipelineType*   pipelineGroup,
        UINT32                  pipelineCount,
        UINT32                  realtimePipelineIndex);

    /*
     * Set the cameraId for each pipeline
     */
    VOID        SetPipelineCameraId(
        AdvancedPipelineType*   pipelineGroup,
        UINT*                   cameraId,
        UINT32                  pipelineCount);

    /*
     * Config the shared Rdi stream
     */
    VOID        ConfigRdiStream(
        ChiSensorModeInfo* pSensorInfo,
        UINT32 pipelineIndex);

    /*
    * Config the shared FD stream
    */
    VOID        ConfigFdStream();

    /*
     * Get the actual physical cameraid per physicalCamera index
     */
    UINT        GetPhysicalCameraId(UINT32 physicalDeviceIndex);

    /*
     * Get the Logical CameraInfo
     */
    LogicalCameraInfo* GetLogicalCameraInfo()
    {
        return m_pLogicalCameraInfo;
    }

    CHX_INLINE BOOL IsLLSNeeded()
    {
        return m_isLLSNeeded;
    }

    CHX_INLINE BOOL IsFlashNeeded()
    {
        return m_isFlashRequired;
    }

    CHX_INLINE BOOL IsFDBuffersNeeded()
    {
        return m_isFDstreamBuffersNeeded;
    }

    CHX_INLINE BOOL IsOfflineNoiseReprocessEnabled() const
    {
        return m_isOfflineNoiseReprocessEnabled;
    }

    INT GetTargetIndex(
        ChiTargetPortDescriptorInfo* pTargetsInfo,
        const char*                  pTargetName);

    /*
     * Gets the metadata manager object pointer
     */
    CHX_INLINE ChiMetadataManager* GetMetadataManager() const
    {
        return m_pMetadataManager;
    }

    /*
     * Gets the metadata client Id
     */
    CHX_INLINE UINT32 GetMetadataClientId(UINT32 id)
    {
        INT32 pipelineType = GET_PIPELINE_TYPE_BY_ID(m_pipelineStatus[id].pipelineId) ;
        UINT32 instanceID  = GET_FEATURE_INSTANCE_BY_ID(m_pipelineStatus[id].pipelineId);
        return m_pipelineToClient[instanceID][pipelineType];
    }

    /*
     * Acquire realtime reconfigure lock
     */
    CHX_INLINE VOID AcquireRTConfigLock()
    {
        if (NULL != m_pRealtimeReconfigDoneMutex)
        {
            m_pRealtimeReconfigDoneMutex->Lock();
        }
    }

    /*
     * Release realtime reconfigure lock
     */
    CHX_INLINE VOID ReleaseRTConfigLock()
    {
        if (NULL != m_pRealtimeReconfigDoneMutex)
        {
            m_pRealtimeReconfigDoneMutex->Unlock();
        }
    }

    CDKResult DestroyOfflinePipelineDescriptors();

    CDKResult CreateOfflinePipelineDescriptors();

    CDKResult RestartOfflineSessions(BOOL reCreatePipeline);

    /*
     * In non-zsl snapshot mode, we have to reconfigure sensor mode and whole camera pipeline,
     * in order to get and process on sensor full size output.
     *
     * In other usecases, we may need to switch different sensor mode (and may different pipeline)
     * for preview, like normal and 3HDR preview.
     *
     * Here we use different pipelineId for different realtime pipelines for simplicity.
     * And this function is to reconfigure realtime pipeline for non-zsl snapshot, or for different preview mode.
     *
     * Currently it will completely destory the original session, and create a totaly new session.
     * May move to activate/deactivate pipeline later, thus to avoid session creation and improve KPI.
     *
     * @param oldSessionId             The current realtime session need to be stopped
     * @param newSessionId             New session need to be started
     * @param restartOfflineSessions   Sensor may output different size, need to reconfigure offline sessions also
     *
     */
    CDKResult ReconfigureRealtimeSession(
        UINT oldSessionId,
        UINT newSessionId,
        BOOL restartOfflineSessions);

    /*
     * Gets the realtime session pointer
     */
    virtual const Session* GetRealtimeSession();

    /*
     * Feature data callback. Features can call this callback to notify any feature data.
     */
    virtual VOID ProcessFeatureDataNotify(
        UINT32   internalFrameNum,
        Feature* pCurrentFeature,
        VOID*    pData);

    /*
     * Feature frame callback
     */
    virtual VOID ProcessFeatureDone(
        UINT32            internalFrameNum,
        Feature*          pCurrentFeature,
        CHICAPTURERESULT* pResult);

    CHIMETAHANDLE GetAvialableResultMetadata()
    {
        return NULL;
    }

    UINT32 GetTotalNumOfFeaturesPerRequest(UINT32 appFrameNum);

    Feature* GetNextFeatureForSnapshot(
        UINT32         appFrameNum,
        const Feature* pCurrentFeature);

    Feature* GetPreviousFeatureForSnapshot(
        UINT32         appFrameNum,
        const Feature* pCurrentFeature);

    SnapshotFeatureInfo* GetSnapshotFeatureInfo(
        UINT32         appFrameNum,
        const Feature* pFeature);

    /// Parse result metadata
    CDKResult ParseResultMetadata(
        ChiMetadata*                 pResultMetadata);

    VOID NotifyFeatureSnapshotDone(
        UINT32                     appFrameNum,
        Feature*                   pCurrentFeature,
        camera3_capture_request_t* pRequest);

   /*
    * Log a string that helps identify the request going from the framework down to camx.
    *
    * @param inFrameNum             the frame number given to the feature in the EPR request call.
    * @param reqFrameNum            The request number going down to session.
    * @param identifyingData        A string to identify the request going down to session  (eg. RAW Jpeg Request).
    *
    */
    virtual VOID LogFeatureRequestMappings(
        UINT32      inFrameNum,
        UINT32      reqFrameNum,
        const CHAR* identifyingData) override;
protected:

    /// Destroy/Cleanup the object
    virtual VOID Destroy(
    BOOL isForced);

    // Flush
    virtual CDKResult ExecuteFlush();

    /// Callback to inform of a pipeline create
    virtual VOID PipelineCreated(
        UINT sessionId,          ///< Id of session created
        UINT pipelineIndex);     ///< Index of the pipeline created (within the context of the session)

    /// Called after each pipeline creation for overriding state after each pipeline creation
    virtual VOID PipelineDestroyed(
        UINT32 sessionId,    ///< Index of the session for which the pipeline was created
        UINT32 pipelineId);  ///< Index of the pipeline created (within the context of the session)

    /// Performs checks for whether a stream is internal
    virtual BOOL StreamIsInternal(
        ChiStream* pStream);

    /// Performs checks for whether a stream is FdStream
    virtual BOOL IsFdStream(
        ChiStream* pStream);

    /// Performs checks for whether a stream is Rdi Stream
    virtual BOOL IsRDIStream(
        ChiStream* pStream);

    virtual const CHISENSORMODEPICKHINT* GetSensorModePickHint(UINT pipelineIndex);

    /// Execute capture request
    virtual CDKResult ExecuteCaptureRequest(
        camera3_capture_request_t* pRequest);              ///< Request parameters

    AdvancedCameraUsecase() = default;
    virtual ~AdvancedCameraUsecase();

protected:

    /// Callback for a result from the driver
    static VOID      ProcessResultCb(
        CHICAPTURERESULT*   pResult,
        VOID*               pPrivateCallbackData)
    {
        SessionPrivateData* pCbData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
        static_cast<AdvancedCameraUsecase*>(pCbData->pUsecase)->ProcessResult(pResult, pPrivateCallbackData);
    }

    /// Callback for Partial Capture result from the driver
    static VOID ProcessDriverPartialCaptureResultCb(
         CHIPARTIALCAPTURERESULT* pCaptureResult,
         VOID* pPrivateCallbackData)
    {
        SessionPrivateData* pCbData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
        static_cast<AdvancedCameraUsecase*>(pCbData->pUsecase)->ProcessDriverPartialCaptureResult(
            pCaptureResult,
            pPrivateCallbackData);
    }

    /// Callback for a message from the driver
    static VOID      ProcessMessageCb(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
        VOID*                       pPrivateCallbackData)
    {
        SessionPrivateData* pCbData = static_cast<SessionPrivateData*>(pPrivateCallbackData);
        static_cast<AdvancedCameraUsecase*>(pCbData->pUsecase)->ProcessMessage(pMessageDescriptor, pPrivateCallbackData);
    }

    // Do not allow the copy constructor or assignment operator
    AdvancedCameraUsecase(const AdvancedCameraUsecase&) = delete;
    AdvancedCameraUsecase& operator= (const AdvancedCameraUsecase&) = delete;

    /// Does one time initialization of the created object
    CDKResult Initialize(
        LogicalCameraInfo*              pCameraInfo,     ///< Camera info
        camera3_stream_configuration_t* pStreamConfig,  ///< Stream configuration
        UsecaseId                       usecaseId);     ///< Identifier for the usecase function

    /// Sets up the class based on enabled features
    CDKResult FeatureSetup(
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream configuration

    /// Performs usecase struct selection and stream matching
    CDKResult SelectUsecaseConfig(
        LogicalCameraInfo*              pCameraInfo,   ///< Camera info
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream configuration

    /// Perform actions after [Camera]Usecase has been created and initialized
    CDKResult PostUsecaseCreation(
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream configuration

    /// Processor called by callback
    VOID      ProcessResult(
        CHICAPTURERESULT*           pResult,
        VOID*                       pPrivateCallbackData);

    /// Processor called by callback
    VOID      ProcessDriverPartialCaptureResult(
        CHIPARTIALCAPTURERESULT* pResult,
        VOID*                    pPrivateCallbackData);

    /// Processor called by callback
    VOID      ProcessMessage(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor,
        VOID*                       pPrivateCallbackData);

    VOID      SelectFeatures(camera3_stream_configuration_t* pStreamConfig);

    Feature*  SelectFeatureToExecuteCaptureRequest(
        camera3_capture_request_t* pRequest,
        UINT32                     physicalCameraIndex);

    Feature*  PickAdvanceFeature(camera3_capture_request_t* pRequest);

    Feature*  PickAdvanceFeatureByGain(UINT32 physicalCameraIndex);

    VOID      SetupSharedPipeline(
        ChiPipelineTargetCreateDescriptor* pPipeline,
        AdvancedPipelineType pipelineType);


    VOID      BuildUsecase(
        LogicalCameraInfo*              pCameraInfo,   ///< Camera info
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream configuration

    VOID      ConfigureStream(
        LogicalCameraInfo*              pCameraInfo,   ///< Camera info
        camera3_stream_configuration_t* pStreamConfig);  ///< Stream configuration

    Feature*  FindFeatureToProcessResult(
        const ChiPrivateData*       pRrivData,
        UINT32                      frameworkFrameNum,
        VOID*                       pPrivateCallbackData);

    Feature*  FindFeaturePerType(
        FeatureType type,
        UINT32      rtIndex = 0);

    ChiPipelineTargetCreateDescriptor GetPipelineDescriptorFromUsecase(
        ChiUsecase* pChiUsecase,
        AdvancedPipelineType id);

    BOOL AdvancedFeatureEnabled()
    {
        return 0 != m_enabledFeaturesCount[0];
    }

    /// check if it is realtime session
    BOOL IsRealtimeSession(
        INT32 sessionId);

    /// check if it is offline session
    BOOL IsOfflineSession(
        INT32 sessionId);

    /// check if there is snapshot stream for this request
    BOOL hasSnapshotStreamRequest(
        camera3_capture_request_t* pRequest) const
    {
        BOOL hasSnapshotRequest = FALSE;
        for (UINT i = 0; i < pRequest->num_output_buffers; i++)
        {
            if ((NULL != m_pSnapshotStream) &&
                (CHISTREAM *)pRequest->output_buffers[i].stream == m_pSnapshotStream)
            {
                hasSnapshotRequest = TRUE;
                break;
            }
        }
        return hasSnapshotRequest;
    }

    /// @todo need feature list instead of static pointers?
    Feature*    m_enabledFeatures[MaxRealTimePipelines][MaxFeatureSupported];
    Feature*    m_pFeature2Wrapper;
    Feature*    m_pActiveFeature;
    Feature*    m_pLastSnapshotFeature;
    UINT32      m_enabledFeaturesCount[MaxRealTimePipelines];

    SnapshotFeatureList m_snapshotFeatures[MaxOutstandingRequests];

    /* a mapping of pPipelineTargetCreateDesc and it's information */
    PipelineStatus m_pipelineStatus[MaxPipelines];
    UINT32         m_pipelineCount;

    /* mapping between the pipeline to the session */
    INT32          m_pipelineToSession[MaxPipelines];
    UINT32         m_uniqueSessionId;
    UINT32         m_realtimeSessionId;

    /* mapping between the pipeline to the cameraid */
    UINT           m_pipelineToCameraId[MaxPipelines];

    Mutex*         m_pResultMutex;     ///< Result mutex
    Mutex*         m_pSetFeatureMutex; ///< SetFeature mutex

    SessionRequestIdFeatureMapping  m_sessionRequestIdFeatureMapping[MaxOutstandingRequests];
    UINT32                          m_sessionRequestIdFeatureMappingCount;

    // preview pipeline
    ChiStream*  m_pPreviewStream;              ///< Tracking of the stream used for preview
    ChiStream*  m_pBayer2YuvStream[MaxRealTimePipelines];
    ChiStream*  m_pJPEGInputStream[MaxRealTimePipelines];
    ChiStream*  m_pVideoStream;
    ChiStream*  m_pYuvCBStream;

    // snapshot pipeline
    ChiStream*  m_pSnapshotStream;             ///< Tracking of the stream used for snapshot

    BOOL        m_isRdiStreamImported;
    BOOL        m_isFdStreamImported;
    BOOL        m_isFdStreamSupported;

    QuadCFASensorInfo     m_QuadCFASensorInfo;
    ChiSensorModePickHint m_defaultSensorModePickHint;
    ASDOutput             m_asdResult;           ///< ASD result
    UINT32                m_inputOutputType;     ///< Config stream input out type

    // flags to control advance snapshot
    BOOL                m_isLLSNeeded;                     ///< Flag to indicate low light snapshot (LLS)
    BOOL                m_isInSensorHDR3ExpCapture;        ///< Flag to indicate in-snesor HDR 3 exposure snapshot
    BOOL                m_isFDstreamBuffersNeeded;         ///< Flag to indicate FD Buffers needed
    BOOL                m_isOfflineNoiseReprocessEnabled;  ///< Flag to indicate if Offline Noise Reprocess enabled or not
    ChiCallBacks*       m_pCallbacks;                      ///< Notify and captureResult callbacks for each pipeline
    BOOL                m_isFlashRequired;                 ///< Flag to indicate if flash is required

    UINT32              m_pipelineToClient[MaxRealTimePipelines][AdvancedPipelineType::PipelineCount]; ///< Map from advanced pipeline type
                                                                                 ///  and client ID
    Mutex*              m_pRealtimeReconfigDoneMutex;      ///< Mutex for realtime reconfig
};

#endif // CHXADVANCEDCAMERAUSECASE_H
