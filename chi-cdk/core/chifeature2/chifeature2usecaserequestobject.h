////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2usecaserequestobject.h
/// @brief Chi Feature2 usecase request object class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2USECASEREQUESTOBJECT_H
#define CHIFEATURE2USECASEREQUESTOBJECT_H

#include <map>
#include <list>
#include <vector>
#include "chifeature2types.h"

// NOWHINE FILE CP006:  used standard libraries for performance improvements

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Usecase Request Object Types
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief The valid states of a usecase request object
enum class ChiFeature2UsecaseRequestObjectState
{
    Initialized,        ///< The state the usecase request object is in when created.
    InputConfigPending, ///< The feature graph moves the usecase request object to this state after gathering all input
                        ///  dependencies, populating the usecase request object, and before returning control to the
                        ///  client, indicating that the client must begin satifying the input dependencies.
    OutputPending,      ///< The feature graph moves the usecase request object to this state after all of the input
                        ///  dependencies have been met.
    Complete,           ///< The feature graph moves the usecase request object to this state after all outputs have been
                        ///  delivered and the usecase request has been completely processed.
    Invalid             ///< The wrapper moves the usecase request object to this state after completion.
};

/// @brief The strings corresponding to the usecase request object state names
#if __GNUC__
static const CHAR* ChiFeature2UsecaseRequestObjectStateStrings[] __attribute__((unused)) =
#else // __GNUC__
static const CHAR* ChiFeature2UsecaseRequestObjectStateStrings[] =
#endif // _GNUC__
{
    "Initialized",
    "InputConfigPending",
    "OutputPending",
    "Complete",
    "Invalid"
};


struct ChiFeature2UsecaseRequestObjectFwkStreamData
{
    UINT8                   numOutputs;        ///< The number of output buffers for the stream
    std::vector<ChiStream*> pFwkStream;        ///< framework stream
    UINT8                   numOutputNotified; ///< number of outputs notified
};

/// @brief Feature request object creation information
struct ChiFeature2UsecaseRequestObjectCreateInputInfo
{
    camera3_capture_request_t*  pRequest;           ///< Capture request provided by camera framework
    ChiMetadata*                pAppSettings;       ///< Application-requested settings
    ChiMetadata*                pStatusMetadata;    ///< Result metadata incase of reprocessing.
    std::vector<ChiStream*>     inputSrcStreams;    ///< List of all input source streams
    BOOL                        reprocessFlag;      ///< Bool indicates if the capture request is a reprocess request
    BOOL                        appReprocessFlag;   ///< Bool indicates if the capture request is an app reprocess request
    Feature2ControllerResult    MCCResult;          ///< MCC result
};

/// @brief Information needed by external source to provide input to the feature, provided by feature graph
struct ChiFeature2UsecaseRequestObjectExtSrcStreamData
{
    UINT8                       numInputs;        ///< The number of buffers required for the stream
    ChiStream*                  pChiStream;       ///< A pointer to a ChiStream object
    std::vector<CHIMETAHANDLE>  inputSettings;    ///< input settings, if any, with which buffers need to be
                                                  ///< produced by the source
};

/// @brief Information to set and provide input stream and corresponding TBHs
struct ChiFeature2UsecaseRequestObjectInputStreamBufferMetadataTBHs
{
    ChiStream*                               pInternalStream;     ///< A pointer to a ChiStream object
    std::vector<CHITARGETBUFFERINFOHANDLE>   bufferMetadataTBHs;  ///< Vector of buffer or metadata TBHs
};

struct ChiFeature2InterFeatureRequestPrivateData
{
    UINT32  featureId;     ///< Feature Id
    UINT32  cameraId;      ///< Camera Id
    UINT32  instanceId;    ///< Instance Id
    VOID*   pPrivateData;  ///< Private data
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Usecase Request Object declaration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CDK_VISIBILITY_PUBLIC ChiFeature2UsecaseRequestObject
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create usecase request object instance.
    ///
    /// @param  pCreateInputInfo    Usecase request object create input information.
    ///
    /// @return Pointer to the ChiFeature2UsecaseRequestObject upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static ChiFeature2UsecaseRequestObject* Create(
        const ChiFeature2UsecaseRequestObjectCreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy this usecase request object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump
    ///
    /// @brief  This method is used by CHI to print debugging state for the usecase request object. It will be called when using
    ///         the dumpsys tool or capturing a bugreport.
    ///
    /// @param  fd The file descriptor which can be used to write debugging text using dprintf() or write().
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Dump(
        INT fd
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetInterFeatureRequestPrivateData
    ///
    /// @brief  Set inter-feature request private data
    ///
    /// @param  featureId       featureId
    /// @param  cameraId        cameraId of feature
    /// @param  instanceId      instanceId of feature
    /// @param  pPrivateData    Private data from feature
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetInterFeatureRequestPrivateData(
        UINT32  featureId,
        UINT32  cameraId,
        UINT32  instanceId,
        VOID*   pPrivateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInterFeatureRequestPrivateData
    ///
    /// @brief  Get inter-feature request private data
    ///
    /// @param  featureId       featureId
    /// @param  cameraId        cameraId of feature
    /// @param  instanceId      instanceId of feature
    ///
    /// @return VOID pointer to private data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetInterFeatureRequestPrivateData(
        UINT32  featureId,
        UINT32  cameraId,
        UINT32  instanceId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RemoveInterFeatureRequestPrivateData
    ///
    /// @brief  Remove inter-feature request private data
    ///
    /// @param  featureId       featureId
    /// @param  cameraId        cameraId of feature
    /// @param  instanceId      instanceId of feature
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult RemoveInterFeatureRequestPrivateData(
        UINT32  featureId,
        UINT32  cameraId,
        UINT32  instanceId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestState
    ///
    /// @brief  Get the current state of this usecase request object.
    ///
    /// @return The current state of this usecase request object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiFeature2UsecaseRequestObjectState GetRequestState() const
    {
        return m_requestState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetRequestState
    ///
    /// @brief  Set the state of this usecase request object.
    ///
    /// @param  requestState    The new usecase request object state.
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE CDKResult SetRequestState(
        ChiFeature2UsecaseRequestObjectState requestState)
    {
        UINT32                                frameNumber = m_appFrameNumber;
        ChiFeature2UsecaseRequestObjectState& rPrevState  = m_requestState;
        BINARY_LOG(LogEvent::FT2_URO_StateInfo, frameNumber, rPrevState, requestState);
        CHX_LOG_INFO("URO:%d_Changing state from %d to %d",
                     m_appFrameNumber, m_requestState, requestState);
        m_requestState = requestState;
        return CDKResultSuccess;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiCaptureRequest
    ///
    /// @brief  Returns a pointer to the ChiCaptureRequest of this usecase request object.
    ///
    /// @return A pointer to the ChiCaptureRequest of this usecase request object.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiCaptureRequest* GetChiCaptureRequest()
    {
        return &m_chiCaptureRequest;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAppSettings
    ///
    /// @brief  Returns the app settings of this usecase request object.
    ///
    /// @return The app settings of this usecase request object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiMetadata* GetAppSettings() const
    {
        return m_pAppSettings;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOfflineDebugData
    ///
    /// @brief  Returns offline debug-data pointer
    ///
    /// @return The offline debug-data pointer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE DebugData* GetOfflineDebugData()
    {
        return &m_offlineDebugData[0];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetaSettings
    ///
    /// @brief  Returns the app settings of this usecase request object.
    ///
    /// @return The app settings of this usecase request object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE const camera_metadata_t* GetMetaSettings() const
    {
        return m_pMetaSettings;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStatusMetadata
    ///
    /// @brief  Returns the previous status of the system
    ///
    /// @return The previous status metadata of this usecase request object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiMetadata* GetStatusMetadata() const
    {
        return m_pStatusMetadata;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsReprocessRequest
    ///
    /// @brief  Returns the flag indicating whether this is a reprocess request.
    ///
    /// @return TRUE if reprocessing is requested; FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsReprocessRequest()
    {
        return m_reprocessFlag;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsAppReprocessRequest
    ///
    /// @brief  Returns the flag indicating whether this is an app reprocess request.
    ///
    /// @return TRUE if reprocessing is requested; FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsAppReprocessRequest()
    {
        return m_appReprocessFlag;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputSrcStreams
    ///
    /// @brief  Returns a list of input source streams.
    ///
    /// @return A vector of input source ChiStream pointers
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE std::vector<ChiStream*>& GetInputSrcStreams()
    {
        return m_inputSrcStreams;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputSinkStreams
    ///
    /// @brief  Get the vector of output sink streams.
    ///
    /// @return A vector of output sink ChiStream pointers
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE std::vector<ChiStream*>& GetOutputSinkStreams()
    {
        return m_outputSinkStreams;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputRTSinkStreams
    ///
    /// @brief  Get the vector of realtime output sink stream data.
    ///
    /// @return A vector of realtime output sink ChiStream data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiFeature2UsecaseRequestObjectFwkStreamData& GetOutputRTSinkStreams()
    {
        return m_fwkRealtimeStreamData;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetInputConfigurations
    ///
    /// @brief  Set the input configuration on this usecase request object. This is called by the feature graph.
    ///
    /// @param  selectedInputSrcStreams     A list of data associated with each selected input source stream.
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetInputConfigurations(
        std::vector<ChiFeature2UsecaseRequestObjectExtSrcStreamData> selectedInputSrcStreams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputSrcNumBuffersNeeded
    ///
    /// @brief  Get the number of buffers associated with the given ChiStream.
    ///
    /// @param  pChiStream  The ChiStream associated with the desired target buffer handle
    ///
    /// @return Number of Buffers
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT GetInputSrcNumBuffersNeeded(
        ChiStream* pChiStream) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetInputTargetBufferHandle
    ///
    /// @brief  Update and store input buffer and metadata info on this usecase request object.
    ///
    /// @param  pInternalStream     Pointer to the internal stream
    /// @param  hMetadataTBH        TBH of metadata
    /// @param  hOutputTBH          TBH of buffer
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetInputTargetBufferHandle(
        ChiStream*                   pInternalStream,
        CHITARGETBUFFERINFOHANDLE    hMetadataTBH,
        CHITARGETBUFFERINFOHANDLE    hOutputTBH);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputTargetBufferHandles
    ///
    /// @brief  Retrieves the list of target buffer handles associated with the given ChiStream. This is called by the feature
    ///         graph during ExecuteProcessRequest().
    ///
    /// @param  pChiStream  The ChiStream associated with the desired target buffer handle
    ///
    /// @return The associated target buffer handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE std::vector<CHITARGETBUFFERINFOHANDLE> GetInputTargetBufferHandles(
       ChiStream* pChiStream)
    {
        std::vector<CHITARGETBUFFERINFOHANDLE> bufferTBHs;
        for (auto inputBufferTBHs : m_inputBufferTBHs)
        {
            if (inputBufferTBHs.pInternalStream == pChiStream)
            {
                bufferTBHs = inputBufferTBHs.bufferMetadataTBHs;
                break;
            }
        }

        if (TRUE == bufferTBHs.empty())
        {
            CHX_LOG_ERROR("Input buffert TBHs on Stream:%p is not found", pChiStream);
        }

        return bufferTBHs;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputMetadataHandles
    ///
    /// @brief  Retrieves the list of input metadata handles. This is called by feature graph during ExecuteProcessRequest().
    ///
    /// @param  pChiStream  The ChiStream associated with the desired target buffer handle
    ///
    /// @return The associated target buffer handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE std::vector<CHITARGETBUFFERINFOHANDLE> GetInputMetadataHandles(
        ChiStream* pChiStream)
    {
        std::vector<CHITARGETBUFFERINFOHANDLE> metadataTBHs;
        for (auto inputMetadataTBHs : m_inputMetadataTBHs)
        {
            if (inputMetadataTBHs.pInternalStream == pChiStream)
            {
                metadataTBHs = inputMetadataTBHs.bufferMetadataTBHs;
                break;
            }
        }

        if (TRUE == metadataTBHs.empty())
        {
            CHX_LOG_ERROR("Input metadata TBHs on Stream:%p is not found", pChiStream);
        }

        return metadataTBHs;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseInputTargetBufferHandle
    ///
    /// @brief  Release the input target buffer handle information on this usecase request object. This is called by the destroy
    ///         of the usecase request object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseInputTargetBufferHandle();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSelectedInputSrcStreamsInfo
    ///
    /// @brief  Retrieves the list of Stream information provided by the feature graph for external source
    ///
    /// @return The associated stream data info objext
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE std::vector<ChiFeature2UsecaseRequestObjectExtSrcStreamData> GetSelectedInputSrcStreamsInfo()
    {
        return m_selectedInputSrcStreamsData;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFDStreamInSelectedSrcStreams
    ///
    /// @brief  Check if FD stream is in selected source streams
    ///
    /// @return TRUE if FD stream is in it
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsFDStreamInSelectedSrcStreams();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataClientId
    ///
    /// @brief  Get Metadata Client Id for input buffers
    ///
    /// @return The associated metadata client Id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 GetMetadataClientId()
    {
        return m_metadataClientId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMCCResult
    ///
    /// @brief  Get MCCResult
    ///
    /// @return MCC result
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE const Feature2ControllerResult* GetMCCResult()
    {
        return &m_MCCResult;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetOutputTargetBufferHandle
    ///
    /// @brief  Set the output target buffer handle on this usecase request object.
    ///
    /// @param  pChiStream                  ChiStream of this output buffer
    /// @param  hTargetBufferInfoHandle     Output target buffer handle from TBM
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SetOutputTargetBufferHandle(
        ChiStream*                  pChiStream,
        CHITARGETBUFFERINFOHANDLE   hTargetBufferInfoHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputTargetBufferHandle
    ///
    /// @brief  Get the output target buffer handle from this usecase request object.
    ///
    /// @param  pChiStream      ChiStream of this output buffer
    ///
    /// @return Output target buffer handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHITARGETBUFFERINFOHANDLE GetOutputTargetBufferHandle(
        ChiStream* pChiStream);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAppFrameNumber
    ///
    /// @brief  Get the framework framenumber.
    ///
    /// @return frame number
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 GetAppFrameNumber()
    {
        return m_appFrameNumber;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUsecaseMode
    ///
    /// @brief  Get the URO's usecase mode
    ///
    /// @return Chi Usecase Mode Type
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE ChiModeUsecaseSubModeType GetUsecaseMode()
    {
        return m_usecaseMode;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCaptureRequest
    ///
    /// @brief  Get the URO's GetCaptureRequest
    ///
    /// @return Chi GetCaptureRequest
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE camera3_capture_request_t* GetCaptureRequest()
    {
        return &m_captureRequest;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetMetadataMap
    ///
    /// @brief  Set the metadata map
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetMetadataMap(
        std::map<UINT32, ChiMetadata*>& rMetadataMap)
    {
        m_metadataFrameNumberMap = rMetadataMap;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMetadataMap
    ///
    /// @brief  Set the metadata map
    ///
    /// @return return the metadata map
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE std::map<UINT32, ChiMetadata*> GetMetadataMap()
    {
        return m_metadataFrameNumberMap;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetShutterTimestamp
    ///
    /// @brief  Get the shutter timestamp for this request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT64 GetShutterTimestamp()
    {
        return m_shutterTimeStamp;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetShutterTimestamp
    ///
    /// @brief  Set the shutter timestamp for this request
    ///
    /// @param  pMessages   Chi feature 2 message pointer
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetShutterTimestamp(
        ChiFeature2Messages* pMessages)
    {
        m_shutterTimeStamp = pMessages->chiNotification.pChiMessages->message.shutterMessage.timestamp;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLastZSLFrameNumber
    ///
    /// @brief  Get the last ZSL TimeStamp for this usecase
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT64 GetLastZSLFrameNumber()
    {
        return m_lastZSLFrameNumber;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetLastZSLFrameNumber
    ///
    /// @brief  Set last ZSL Timestamp for this usecase
    ///         This should be called only from feature wrapper
    ///
    /// @param  lastZSLFrameNumber   Set Last ZSL FrameNumber
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetLastZSLFrameNumber(
        UINT64 lastZSLFrameNumber)
    {
        m_lastZSLFrameNumber = lastZSLFrameNumber;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTuningFeature2Value
    ///
    /// @brief  Get the Feature2 value for tuning
    ///
    /// @return Feature2 value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 GetTuningFeature2Value()
    {
        return m_tuningFeature2Value;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetGPURotationFlag
    ///
    /// @brief  Set the GPU rotation flag
    ///
    /// @param  value   GPU rotation flag value
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetGPURotationFlag(
        BOOL value)
    {
        m_isGPURotationNeeded = value;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGPURotationFlag
    ///
    /// @brief  Get the GPU rotation flag
    ///
    /// @return Rotation flag value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL GetGPURotationFlag()
    {
        return m_isGPURotationNeeded;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetSkipPreviewFlag
    ///
    /// @brief  Set the skip preview flag
    ///
    /// @param  value   BOOL value for skip preview flag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetSkipPreviewFlag(
        BOOL value)
    {
        m_isPreviewSkipped = value;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetSkipPreviewFlag
    ///
    /// @brief  Get the skip preview flag
    ///
    /// @return Skip preview flag value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL GetSkipPreviewFlag()
    {
        return m_isPreviewSkipped;
    }

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ChiFeature2UsecaseRequestObject
    ///
    /// @brief  Default constructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2UsecaseRequestObject() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~ChiFeature2UsecaseRequestObject
    ///
    /// @brief  Virtual Destructor.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~ChiFeature2UsecaseRequestObject();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initializes a newly-created usecase request object
    ///
    /// @param  pCreateInputInfo Usecase request object input creation information
    ///
    /// @return CDKResultSuccess if successful; error code otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        const ChiFeature2UsecaseRequestObjectCreateInputInfo* pCreateInputInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Copy constructor and assignment operator
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2UsecaseRequestObject(const ChiFeature2UsecaseRequestObject&)             = delete;
    ChiFeature2UsecaseRequestObject& operator= (const ChiFeature2UsecaseRequestObject&) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ChiFeature2UsecaseRequestObjectState    m_requestState;             ///< The state of this URO. Set by feature graph.
    camera3_capture_request_t               m_captureRequest;           ///< Capture Request
    ChiCaptureRequest                       m_chiCaptureRequest;        ///< The ChiCaptureRequest associated with this URO.
                                                                        ///  Set during creation.
    ChiMetadata*                            m_pAppSettings;             ///< The application-requested settings. Set during
                                                                        ///  creation.
    ChiMetadata*                            m_pStatusMetadata;          ///< Preview Result metadata
    const camera_metadata_t*                m_pMetaSettings;            ///< The vendor tag settings
    BOOL                                    m_reprocessFlag;            ///< A flag to indicate whether this request is a
                                                                        ///  reprocess request. Set during creation. Consumed
                                                                        ///  by feature graph selector to select a graph.
    BOOL                                    m_appReprocessFlag;         ///< A flag to indicate whether this request is an
                                                                        ///  app reprocess request. Set during creation.
    std::vector<ChiStream*>                 m_inputSrcStreams;          ///< All possible input source streams provided by the
                                                                        ///  usecase for feature graph to select. Set during
                                                                        ///  creation. Consumed by feature graph manager to
                                                                        ///  create port to ChiStream map.
    std::vector<ChiStream*>                 m_outputSinkStreams;        ///< All output sink streams provided by the usecase.
                                                                        ///  Set during creation based on framework capture
                                                                        ///  request. Consumed by feature graph manager to
                                                                        ///  create port to ChiStream map.

    ChiFeature2UsecaseRequestObjectFwkStreamData m_fwkRealtimeStreamData;                          ///< Framework stream data
    std::vector<ChiFeature2UsecaseRequestObjectInputStreamBufferMetadataTBHs>  m_inputMetadataTBHs;///< List of input metadata
                                                                                                   ///  handles
    UINT32                                                                     m_metadataClientId; ///< Input metadata client Id
    std::vector<ChiFeature2UsecaseRequestObjectInputStreamBufferMetadataTBHs>  m_inputBufferTBHs;  ///< List of input buffers
    std::map<ChiStream*, CHITARGETBUFFERINFOHANDLE>                            m_outputBufferTBHs; ///< List of output buffers

    std::vector<ChiFeature2UsecaseRequestObjectExtSrcStreamData>    m_selectedInputSrcStreamsData; ///< Input source streams
                                                                                                   ///  selected by the feature
                                                                                                   ///  graph. Set by feature
                                                                                                   ///  graph during initial
                                                                                                   ///  ExecuteProcessRequest
                                                                                                   ///  call.

    UINT32                          m_appFrameNumber;                               ///< Framework number
    Feature2ControllerResult        m_MCCResult;                                    ///< MCC result
    ChiModeUsecaseSubModeType       m_usecaseMode;                                  ///< Usecase mode of the URO
    UINT64                          m_shutterTimeStamp;                             ///< Shutter timestamp
    UINT64                          m_lastZSLFrameNumber;                           ///< Last ZSL FrameNumber
    std::map<UINT32, ChiMetadata*>  m_metadataFrameNumberMap;                       ///< Associates a metadata structure for a
                                                                                    ///  frame number
    DebugData                       m_offlineDebugData[DebugDataMaxOfflineFrames];  ///< Place holder for debug-data use in
                                                                                    ///  offline process
    UINT32                          m_tuningFeature2Value;                          ///< tuning Feature2Value value

    std::list<ChiFeature2InterFeatureRequestPrivateData> m_interFeatureRequestPrivateData;  ///< List holding private data from
                                                                                            ///  FRO in the URO context.
    BOOL                            m_isGPURotationNeeded;                          ///< flag to check if GPU rotation is
                                                                                    ///< required in the JPEG pipeline
    BOOL                            m_releaseInput;                                 ///< needed to release MNFR buffer early
    BOOL                            m_isPreviewSkipped;                             ///< flag to check if preview skipped
};

#endif // CHIFEATURE2USECASEREQUESTOBJECT_H
