////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxusecase.h
/// @brief CHX usecase base class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHXUSECASE_H
#define CHXUSECASE_H

#include <assert.h>
#include <mutex>
#include <unordered_map>

#include "chxincs.h"
#include "chxpipeline.h"
#include "chxsession.h"
#include "chxusecaseutils.h"
#include "chivendortag.h"
#include "chxperf.h"
#include "chitargetbuffermanager.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

/// Forward declarations
struct ChiPipelineTargetCreateDescriptor;
class  Session;

/// Static constants
static const UINT32 MaxFileLen             = 256;
static const UINT32 MaxExternalBuffers     = 6;

///@ todo Optimize this
static const UINT32 ReplacedMetadataEntryCapacity = 1024;                  ///< Replaced metadata entry capacity
static const UINT32 ReplacedMetadataDataCapacity  = 256 * 1024;            ///< Replaced metadata data capacity
static const UINT32 RDIBufferQueueDepth           = BufferQueueDepth + 2;  ///< RDI buffer queue depth

/// @brief Data about a created pipeline and its dynamic state
struct PipelineData
{
    UINT        id;
    Pipeline*   pPipeline;
    CHISTREAM*  pStreams[MaxChiStreams];
    UINT        numStreams;
    UINT        seqId;
    UINT        seqIdToFrameNum[MaxOutstandingRequests];
    BOOL        isHALInputStream;
};

/// @brief Data about a created session
struct SessionData
{
    Session*     pSession;                          ///< Pointer to session
    UINT32       numPipelines;                      ///< Number of created pipelines
    UINT32       rtPipelineIndex;                   ///< Pipeline index that is realtime
    PipelineData pipelines[MaxPipelinesPerSession]; ///< Per pipeline information
};

/// @brief Session private data
struct SessionPrivateData
{
    Usecase*  pUsecase;                    ///< Per usecase class
    UINT32    sessionId;                   ///< Session Id that is meaningful to the usecase in which the session belongs
};


/// @brief Buffers information
struct TargetBufferInfo
{
    UINT32             frameNumber;                               ///< Frame number
    ChiMetadata*       pMetadata;                                 ///< Metadata
    VOID*              pDebugData;                                ///< Debug data
    CHISTREAMBUFFER*   pRdiOutputBuffer;                          ///< RDI buffer
    BOOL               isBufferReady;                             ///< Buffer ready flag
    BOOL               isMetadataReady;                           ///< Metadata ready flag
};

/// @brief Collection of information for tracking internal buffers
struct TargetBuffer
{
    TargetBufferInfo    bufferQueue[RDIBufferQueueDepth];  ///< Buffer queues for internal targets
    CHIBufferManager*   pBufferManager;                 ///< Buffer manager to maintain buffers
    Mutex*              pMutex;                         ///< Mutex protecting access to the manager and buffers
    Condition*          pCondition;                     ///< Condition
    UINT32              lastReadySequenceID;            ///< Last sequence id which buffer and metadata are ready
    UINT32              validBufferLength;              ///< Valid RDI buffer length for offline process.
    UINT32              firstReadySequenceID;           ///< The sequence ID of first valid buffer
    ChiStream*          pChiStream;                     ///< which stream is attached
};

/// @brief Metadata information for the framework request
struct ChiMetadataBundle
{
    ChiMetadata* pInputMetadata;   ///< Pointer to the input metadata
    ChiMetadata* pOutputMetadata;  ///< Pointer to the output metadata
    bool         isInputSticky;    ///< Checks if the input metadata is sticky
};

// @brief map for metadata
typedef std::unordered_map<UINT64, ChiMetadataBundle> MetadataHandleMap;

/// @brief Vendor Tag names
struct ChiVendorTagNames
{
    const CHAR* pComponentName; ///< Name of component associated with the vendor tag base
    const CHAR* pTagName;       ///< The tagName of the vendortag
};

enum FlushStatus {
    NotFlushing,
    IsFlushing,
    HasFlushed
};
/// @brief Usecase flags
union UsecaseRequestFlags
{
    struct
    {
        BIT isMessageAvailable          : 1;  ///< App shutter callback msg ready?
        BIT isMessagePending            : 1;  ///< True if the framework is expecting a message for this request
        BIT isInErrorState              : 1;  ///< True if the request is in an error state
        BIT isOutputMetaDataSent        : 1;  ///< pending app output metadata
        BIT isOutputPartialMetaDataSent : 1;  ///< pending app output CHI Partial Metadata
        BIT isDriverPartialMetaDataSent : 1;  ///< pending app output Driver Partial metadata
        BIT isMetadataErrorSent         : 1;  ///< True if the metadata is sent as error
        BIT isBufferErrorSent           : 1;  ///< True if we have sent a buffer error
        BIT isZSLMessageAvailable       : 1;  ///< True if stream is for Snapshot
        BIT reserved                    : 23; ///< Reserved for future use
    };
    UINT value;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Base Usecase class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Usecase
{
public:

    // Perform any base class functionality for processing the request and call the usecase specific derived class
    CDKResult ProcessCaptureRequest(
        camera3_capture_request_t* request);

    // Implemented to reset metadata status
    virtual VOID ResetMetadataStatus(
        camera3_capture_request_t*) { }

    VOID FlushRequestsByFrameNumber(
        BOOL   bLock,
        UINT32 lastChiFrameNum = 0);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushAllSessions
    ///
    /// @brief  Flush sessions
    ///
    /// @param  ppSession     Session pointer array
    /// @param  size          Size of session pointer array
    ///
    /// @return CDKResult if successful, failure otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FlushAllSessions(
        Session* const *    ppSession,
        const UINT32        size);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlushAllSessionsInParallel
    ///
    /// @brief  Flush sessions in parallel
    ///
    /// @param  ppSession     Session pointer array
    /// @param  size          Size of session pointer array
    ///
    /// @return CDKResult if successful, failure otherwise.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FlushAllSessionsInParallel(
        Session* const *    ppSession,
        const UINT32        size);

    // Perform any base class functionality for processing the result and call the usecase specific derived class
    CDKResult ProcessCaptureResult(
        const CHICAPTURERESULT* pResult);

    // Perform any base class functionality for processing the message notification
    CDKResult ProcessMessage(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessErrorMessage
    ///
    /// @brief  Determines how to deal with certain error messages
    ///
    /// @param  pMessageDescriptor     Message Descriptor
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessErrorMessage(
        const CHIMESSAGEDESCRIPTOR* pMessageDescriptor);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// RecoveryThread
    ///
    /// @brief  Triggers recovery
    ///
    /// @param  pArg     Private data
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* RecoveryThread(VOID* pArg);

    // Destroy the usecase object
    VOID DestroyObject(
        BOOL isForced);

    // Update the sensor mode index, per frame
    CDKResult UpdateSensorModeIndex(
        camera_metadata_t* pMetaData);

    // Update the feature mode index, per frame
    CDKResult UpdateFeatureModeIndex(
        camera_metadata_t* pMetaData);

    // Singular interface to return buffers/metadata back to the application
    VOID ReturnFrameworkResult(
        const camera3_capture_result_t* pResult,
        UINT32 cameraID);

    // Singular interface to return message back to the application
    VOID ReturnFrameworkMessage(
        const camera3_notify_msg_t* pMessage,
        UINT32 cameraID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LogFeatureRequestMappings
    ///
    /// @brief  Function to log mappings between the frame number recieved and the frame number sent down to camx
    ///
    /// @param  inFrameNumber   The input frame number to the feature or usecase (framenumber argument to process request-like
    ///                         like functions.
    /// @param  reqFrameNum     The framenumber of the request being sent down.
    /// @param  identifyingData The feature/use of the request being submitted
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID LogFeatureRequestMappings(UINT32 inFrameNum, UINT32 reqFrameNum, const CHAR* identifyingData);

    // To flush the session
    virtual CDKResult Flush();

    // To dump the session
    virtual CDKResult Dump(
        int     fd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndActivatePipeline
    ///
    /// @brief  Function to check and activate the pipeline if not activated already
    ///
    /// @param  pSession               Pointer to the ChxSession
    /// @param  pPipelineData          Pointer to the pipeline data
    /// @param  numPCRsBeforeStreamOn  Number of PCRs before streamON
    /// @param  frameNumber            Frame number of the current frame
    ///
    /// @return CDKResultSucess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE CDKResult CheckAndActivatePipeline(Session* pSession)
    {
        CDKResult result = CDKResultSuccess;

        if (FALSE == pSession->IsPipelineActive())
        {
            result = ExtensionModule::GetInstance()->ActivatePipeline(pSession->GetSessionHandle(),
                                                                      pSession->GetPipelineHandle());

            if (CDKResultSuccess == result)
            {
                pSession->SetPipelineActivateFlag();
            }
        }
        return result;
    }

    // Accessor to results
    CHX_INLINE camera3_capture_result_t* GetCaptureResult(
        UINT resultId
    )
    {
        return &m_captureResult[resultId];
    }

    // Accessor to partial Capture results of Driver
    CHX_INLINE camera3_capture_result_t* GetDriverPartialCaptureResult(
        UINT resultId
    )
    {
        return &m_driverPartialCaptureResult[resultId];
    }

    // Accessor to partial Capture results of CHI
    CHX_INLINE camera3_capture_result_t* GetCHIPartialCaptureResult(
        UINT resultId
    )
    {
        return &m_chiPartialCaptureResult[resultId];
    }

    // Camera Id
    UINT GetCameraId() { return m_cameraId; }

    // Usecase Id
    UsecaseId GetUsecaseId() { return m_usecaseId; }

    // Flush Status
    CHX_INLINE FlushStatus GetFlushStatus() {
        FlushStatus result = HasFlushed; // Assume that we have flushed incase m_pFlushMutex is NULL

        if (NULL != m_pFlushMutex)
        {
            m_pFlushMutex->Lock();
            result = m_flushStatus;
            m_pFlushMutex->Unlock();
        }

        return result;
    }

    // Check if Any Partial Data has been sent
    BOOL CheckIfAnyPartialMetaDataHasBeenSent(UINT32 resultFrameIndexChi);

    // Check if Partial Data can be sent
    BOOL CheckIfPartialDataCanBeSent(PartialResultSender sender, UINT32 resultFrameIndexChi);

    // Get m_pAppResultMutex pointer
    CHX_INLINE Mutex* GetAppResultMutex() { return m_pAppResultMutex; }

    //----------- APIs for CHIMETADATA Adaptations Start -------------------------//

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MetadataCaptureRequestUpdate
    ///
    /// @brief  Adaptation API that updates the CHICAPTUREREQUEST structure with the new Metadata pointers.
    ///         The user can pass the legacy pMetadata pointer, This API handles generating the input and
    ///         output metadata for the Capture Request.
    ///         The user must Ensure that CreateChiMetadataManager is called and initialized
    ///
    /// @param  captureRequest   Reference to the capture request structure
    /// @param  sessionId        Session Id for the request
    /// @param  isInputSticky    Flag to indicate whether the input metadata is sticky. If the android
    ///                          framework metadata is passed, sticky must be set to true
    ///
    /// @return CDKResultSucess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult MetadataCaptureRequestUpdate(
        CHICAPTUREREQUEST& captureRequest,
        UINT32             sessionId,
        bool               isInputSticky = true);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MetadataCaptureResultGet
    ///
    /// @brief  Adaptation API that updates the ChiCaptureResult structure with the new Metadata pointers.
    ///         This API handles generating the android framework metadata(pResultMetadata) corresponding to the output
    ///         metadata and recycling the metabuffers
    ///         The user must Ensure that CreateChiMetadataManager is called and initialized
    ///
    /// @param  captureResult    Reference to the capture result structure
    /// @param  sessionId        Session Id for the request
    ///
    /// @return CDKResultSucess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult MetadataCaptureResultGet(
       ChiCaptureResult& captureResult,
       UINT32            sessionId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// MetadataCaptureResultRelease
    ///
    /// @brief  Adaptation API that is used to release the android metadata obtained using MetadataCaptureResultGet
    ///
    /// @param  pAndroidMetadata Android framework metadata to be released
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID MetadataCaptureResultRelease(
       const VOID* pAndroidMetadata);

    //----------- APIs for CHIMETADATA Adaptations End -------------------------//

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateAppResultMetadata
    ///
    /// @brief  Usecase API to update the result structure, m_captureResult, with the CHI Metadata
    ///
    /// @param  pAndroidMetadata Android framework metadata to be released
    /// @param  resultId         Result index corresponding to the result
    /// @param  clientId         clientId corresponding to the result
    ///
    /// @return CDKResultSucess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult UpdateAppResultMetadata(
        ChiMetadata* pChiMetadata,
        UINT         resultId,
        UINT32       clientId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateAppPartialResultMetadataFromDriver
    ///
    /// @brief  Usecase API to update the result structure, m_captureResult, with the CHI Metadata
    ///
    /// @param  pAndroidMetadata Android framework metadata to be released
    /// @param  resultId         Result index corresponding to the result
    ///
    /// @return CDKResultSucess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult UpdateAppPartialResultMetadataFromDriver(
        ChiMetadata*                pChiMetadata,
        UINT                        resultId,
        UINT32                      resultFrameNumber,
        UINT32                      clientId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateMetadataManager
    ///
    /// @brief  Usecase API to create metadata manager
    ///
    /// @param  cameraId                    Id of the camera corresponding to the metadata manager. This information
    ///                                     is used to generate the default keys. If there are multiple camera, pass
    ///                                     logical camera Id
    /// @param  initFrameworkMetadata       Initialize the CHI metadata framework input pool with the default number
    ///                                     of buffers. For all the usecases,
    /// @param  pDefaultPipeline            Pointer to the default pipeline. Must pass NULL for if the usecase contain
    ///                                     atleast more than one pipeline per usecase.
    /// @param  initGenericMeta             Flag to indicate whether to create a client for the generic pool. Generic
    ///                                     pool is used if the usecase wanted to centralize the intermediate buffer
    ///                                     management
    ///
    /// @return CDKResultSucess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateMetadataManager(
        INT32     cameraId                 = 0,
        bool      initFrameworkMetadata    = true,
        Pipeline* pDefaultPipeline         = NULL,
        bool      initGenericMeta          = true);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareForRecovery
    ///
    /// @brief  Prepare for triggering recovery by setting bad state to true and delete all pending results
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareForRecovery();

    // Get metadata bundle
    CDKResult GetMetadataBundle(
        const camera_metadata_t* pFrameworkInput,
        UINT32                   frameNumber,
        ChiMetadataBundle&       rBundle,
        UINT32                   metadataClientId = ChiMetadataManager::InvalidClientId);

    // Release metadata bundle given the framenumber
    CDKResult ReleaseMetadataBundle(
        INT32 frameNumber);

    CHX_INLINE VOID IsReprocessUsecase(BOOL isReprocess)
    {
        m_isReprocessUsecase = isReprocess;
    }

    VOID DeleteAllPendingResults();

    CHX_INLINE VOID WaitUntilFlushFinishes()
    {
        m_pFlushMutex->Lock();
        while (m_flushStatus == FlushStatus::IsFlushing)
        {
            m_pFlushDone->Wait(m_pFlushMutex->GetNativeHandle());
        }
        m_pFlushMutex->Unlock();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsUsecaseInBadState
    ///
    /// @brief  Return the usecase status to indicate if recovery is required.
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsUsecaseInBadState()
    {
        return m_isUsecaseInBadState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUsecaseState
    ///
    /// @brief  Set the usecase status to indicate if recovery is required.
    ///
    /// @param  isUsecaseInBadState     Flag to the usecase if recovery is required.
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE VOID SetUsecaseInBadState(BOOL isUsecaseInBadState)
    {
        m_isUsecaseInBadState = isUsecaseInBadState;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SubmitRequestMC
    ///
    /// @brief  SubmitRequst for MCusecase to return Cancelled state in case of Flush
    ///
    /// @param  pSubmitRequestData     Session submit request data.
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SubmitRequestMC(CHIPIPELINEREQUEST* pSubmitRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SubmitRequest
    ///
    /// @brief  SubmitRequst to extension module and pre-check if the recover process required
    ///
    /// @param  pSubmitRequestData     Seesion submit request data.
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult SubmitRequest(CHIPIPELINEREQUEST* pSubmitRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGenericMetadataClientId
    ///
    /// @brief  Get Generic Metadata Client Id
    ///
    /// @return UINT32 Generic Metadata Client Id
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetGenericMetadataClientId()
    {
        return m_genericMetadataClientId;
    }

protected:

    // Perform any base class intialization and call the derived class to do usecase specific initialization
    CDKResult Initialize(
        bool initializeMetadataManager = true);

    // Implemented by the derived class to execute the capture request
    virtual CDKResult ExecuteCaptureRequest(
        camera3_capture_request_t* request) = 0;

    // Implemented by the derived class to process the saved results
    virtual VOID ProcessResults() = 0;

    // Deprecated function, use HandleProcessRequestError directly instead
    // todo: Add deprecated marking
    CAMX_INLINE VOID HandleProcessRequestErrorAllPCRs(
        camera3_capture_request_t* pRequest)
    {
        HandleProcessRequestError(pRequest);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HandleProcessRequestError
    ///
    /// @brief  This method should be called when the driver should notify the framework with the error message ERROR_REQUEST.
    ///         This will cancel the request pointed to by pRequest by:
    ///
    ///         - Notifying the framework with a ERROR_REQUEST message
    ///         - Releasing the fences associated with this request
    ///         - Returning invalidated buffers and no metadata to the framework.
    ///
    ///         The driver should not notify nor return any result related to pRequest after this method is invoked.
    ///
    /// @param  pRequest   A pointer to the request that should be invalidated
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleProcessRequestError(
        camera3_capture_request_t* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InvalidateResultBuffers
    ///
    /// @brief  Return all unsent result buffers contained within pRequest as error.
    ///
    /// @param  pRequest                Pointer to the request to invalidate
    /// @param  shouldSendBufferError   If true, then send a buffer error for each invalidated request
    /// @param  shouldReturnInputBuffer If true, then return the request's input buffer with the results
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InvalidateResultBuffers(
        camera3_capture_request_t* pRequest,
        BOOL                       shouldSendBufferError=TRUE,
        BOOL                       shouldReturnInputBuffer=FALSE);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///
    /// HandleResultError
    ///
    /// @brief  This method should be called when the driver should notify the framework with the error message ERROR_RESULT
    ///
    /// @param  pRequest   A pointer to the request whose metadata should be invalidated
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID HandleResultError(
        camera3_capture_request_t* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InvalidateResultMetadata
    ///
    /// @brief  Set all metadata request flags as sent and clear their respective result holders
    ///
    /// @param  pRequest    Pointer to the request to invalidate
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InvalidateResultMetadata(
        camera3_capture_request_t* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OverrideInputMetaForQCFA
    ///
    /// @brief  Override the ROIs in input meta for quadcfa sensor
    ///
    /// @param  pInputAndroidMeta    Pointer to the input metadata from framework
    ///
    /// @return CDKResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult OverrideInputMetaForQCFA(
        camera_metadata_t* pInputAndroidMeta);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OverrideResultMetaForQCFA
    ///
    /// @brief  Override the ROIs in result meta for quadcfa sensor
    ///
    /// @param  pResultAndroidMeta    Pointer to the result metadata to framework
    ///
    /// @return CDKResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult OverrideResultMetaForQCFA(
        camera_metadata_t* pResultAndroidMeta);

    virtual VOID Destroy(BOOL isForced) = 0;

    camera3_capture_request_t* ReturnPendingPCR(UINT frameNumber);

    virtual CDKResult ExecuteFlush() { return CDKResultSuccess; }

    virtual CHX_INLINE BOOL ShouldUseParallelFlush() { return FALSE; }

    Usecase();
    virtual ~Usecase();

    UsecaseId                m_usecaseId;                                           ///< UsecaseId
    camera3_capture_result_t m_captureResult[MaxOutstandingRequests];               ///< Capture results to be sent to the
                                                                                    ///< app/framework
    camera3_capture_result_t m_driverPartialCaptureResult[MaxOutstandingRequests];    ///< Core Driver Partial Capture results to be sent to the
                                                                                    ///< app/framework
    camera3_capture_result_t m_chiPartialCaptureResult[MaxOutstandingRequests]; ///< Usecase Partial Capture results to be sent to the
                                                                                    ///< app/framework
    camera3_notify_msg_t     m_notifyMessage[MaxOutstandingRequests];               ///< App shutter callback msg
    UsecaseRequestFlags      m_requestFlags[MaxOutstandingRequests];                ///< Per request state flags
    UINT32                   m_numBufferErrorMessages[MaxOutstandingRequests];      ///< Number of buffer error messages received
    UINT32                   m_numAppPendingOutputBuffers[MaxOutstandingRequests];  ///< Number of pending app output buffers
                                                                                    ///  not sent out yet per request
    INT64                    m_nextAppResultFrame;                                  ///< Next frame result to be sent back
    INT64                    m_nextAppMessageFrame;                                 ///< Next frame message to be sent back
    INT64                    m_lastAppMessageFrameReceived;                         ///< Last frame message to be sent back
    INT64                    m_lastAppRequestFrame;                                 ///< Last app request frame number
    PerThreadData            m_resultProcessingThread;                              ///< Thread to process the results
    Mutex*                   m_pAppResultMutex;                                     ///< App Result mutex
    Mutex*                   m_pAllResultsMutex;                                    ///< All Results mutex
    Condition*               m_pAppResultAvailable;                                 ///< Wait till app results are available
    Condition*               m_pAllResultsAvailable;                                ///< Wait till all results are available
    volatile BOOL            m_appResultThreadTerminate;                            ///< Indication to app result thread to
                                                                                    ///  termiate itself
    volatile BOOL            m_allResultsAvailable;                                 ///< Are all results available at any point
                                                                                    ///  in time
    UINT32                   m_cameraId;                                            ///< Camera id to which the usecase belongs
    /// @todo Need to add result sequencing mechanism
    INT64                    m_lastResultMetadataFrameNum;                          ///< Frame number whose metadata was sent
                                                                                    ///  last
    ChiUsecase*              m_pChiUsecase;                                         ///< Matched usecase
    ChiUsecase*              m_pClonedUsecase;                                      ///< Matched usecase
    UINT32                   m_selectedSensorModeIndex;                             ///< selected sensor mode index
    CHIPRIVDATA              m_privData[MaxOutstandingRequests];                    ///< private data for request
    ChiMetadataBundle        m_chiMetadataArray[MaxOutstandingRequests];            ///< Array of CHI metadata
    ChiMetadataManager*      m_pMetadataManager;                                    ///< Metadata manager for the usecase
    UINT32                   m_metadataClientId;                                    ///< Metadata clientId for the default
                                                                                    ///  RT pipeline
    UINT32                   m_genericMetadataClientId;                             ///< Metadata clientId for the generic
                                                                                    ///  pool
    BOOL                     m_isReprocessUsecase;                                   ///< Flag to check if reprocess usecase

    Session*                 m_pSession[MaxSessions];                               ///< Session
    SessionPrivateData       m_perSessionPvtData[MaxSessions];                      ///< Per session private data
    Pipeline*                m_pPipeline[MaxPipelinesPerSession];                   ///< Pipelines
    BOOL                     m_bIsFeature2Enabled;                                  ///< Is feature2 enabled
    LogicalCameraInfo*       m_pLogicalCameraInfo;                                  ///< Logical camera info
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAppFrameNum
    ///
    /// @brief  Get the Google framework frame number from the Chi override frame number
    ///
    /// @param  chiOverrideFrameNum   A frame number assigned by Chi
    ///
    /// @return frame number assigned by the Google framework
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 GetAppFrameNum(UINT32 chiOverrideFrameNum) const
    {
        return m_originalIncomingFrameNum[chiOverrideFrameNum % MaxPendingFrameNumber];
    }

    // Vendor tag values
    // Add vendor tag values that need to be stored here

private:

    /// Main entry function for the resul thread
    static VOID* ResultThreadMain(VOID* pArg);

    static VOID* FlushThread(VOID* pThreadData);

    VOID FlushThreadProcessing(VOID* pThreadHandle);

    /// Do not allow the copy constructor or assignment operator
    Usecase(const Usecase& rUsecase) = delete;
    Usecase& operator= (const Usecase& rUsecase) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetChiOverrideFrameNum
    ///
    /// @brief  Get Chi override frame number for last received app request
    ///
    /// @param  None
    ///
    /// @return Current frameNumber assigned by CHI
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 GetChiOverrideFrameNum() const
    {
        return m_chiOverrideFrameNum;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AssignChiOverrideFrameNum
    ///
    /// @brief  Assign a Chi frame number to a Google framework frame number. This method should only be invoked once per Google
    ///         framework frame number.
    ///
    /// @param  appFrameNum   The google framework frame number
    ///
    /// @return A Chi frame number
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE UINT32 AssignChiOverrideFrameNum(UINT32 appFrameNum)
    {
        UINT32 chiFrameNumIdx = (m_chiOverrideFrameNum % MaxPendingFrameNumber);

        m_originalIncomingFrameNum[chiFrameNumIdx] = appFrameNum;

        return m_chiOverrideFrameNum++;
    }

    /// Replace the capture request metadata with a copy of it with additional new settings inserted by the Chi override
    CDKResult ReplaceRequestMetadata(
        const VOID* pMetadata);                            ///< Request parameters

    /// Restore the original metadata that was passed in
    VOID RestoreRequestMetadata(
        camera3_capture_request_t* pRequest);              ///< Request parameters

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReturnFrameworkErrorMessage
    ///
    /// @brief  Return an error message to the Google framework
    ///
    /// @param  pMessage   A pointer to the message to return to the framework. The pMessage->frame_number must be a chi
    ///                    frame number.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReturnFrameworkErrorMessage(
        const camera3_notify_msg_t* pMessage);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReturnPendingAvailableFrameworkMessages
    ///
    /// @brief  Sequentially returns available messages to the framework based on frame number, for a given camera
    ///
    /// @param  cameraID    ID corresponding to the camera for which to return framework messages.
    ///
    /// @return None.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReturnPendingAvailableFrameworkMessages(
        UINT32 cameraID);

    /// Handle Various Metadata result
    BOOL HandleMetadataResultReturn(
        camera3_capture_result_t* pOverrideResult,
        UINT32                    frame_Number,
        UINT32                    resultFrameIndexChi,
        UINT32                    cameraID);

    /// Checks if the Meta data has been sent
    /// Caller is expected to have locked m_pMapLock before invoking the function
    BOOL CheckIfMetaDataHasBeenSent(
        PartialResultSender sender,
        UINT32              resultFrameIndexChi);

    /// Sets the Meta data has been sent for the sender
    /// Caller is expected to have locked m_pMapLock before invoking the function
    VOID SetMetaDataHasBeenSent(
        PartialResultSender sender,
        UINT32              resultFrameIndexChi,
        BOOL                hasSent);

    /// Inject an empty meta data for the given frame number
    VOID InjectFrameworkResult(
        UINT32              frameNumber,
        PartialResultCount  partialResultCount,
        UINT32              cameraID);

    CAMX_INLINE VOID InitializeRequestFlags()
    {
        m_requestFlags[0].value          = 0;
        m_requestFlags[0].isInErrorState = TRUE;
        ChxUtils::Memset(&m_requestFlags[0], m_requestFlags[0].value, sizeof(m_requestFlags));
    }

    VOID DumpDebugInfo();

    UINT32                      m_originalIncomingFrameNum[MaxPendingFrameNumber];      ///< Original incoming app frame num
    UINT32                      m_chiOverrideFrameNum;                                  ///< Sequential frame number
    const VOID*                 m_pOriginalMetadata;                                    ///< Original pointer to the metadata
    camera_metadata_t*          m_pReplacedMetadata;                                    ///< Replaced metadata
    size_t                      m_replacedMetadataSize;                                 ///< Replaced metadata size
    FlushStatus                 m_flushStatus;                                          ///< Flag to check if flush is enabled
    Mutex*                      m_pMapLock;
    camera3_capture_request_t   m_pendingPCRs[MaxOutstandingRequests];
    UINT32                      m_numberOfPendingOutputBuffers[MaxOutstandingRequests];  ///< pending app output buffers
    VOID*                       m_pEmptyMetaData;                                        ///< Empty MetaData

    std::mutex                  m_MetadataLock;                                         ///< Metadata lock
    MetadataHandleMap           m_pMetadataHandleMap[MaxSessions];                      ///< Map for the metadata handle
    PerThreadData               m_FlushProcessThreadData[MaxSessions];
    UINT32                      m_numOfSessionsToBeFlushed;

    Mutex*                      m_pFlushMutex;                                          ///< flush count mutex
    Mutex*                      m_pParallelFlushLock;
    Condition*                  m_pFlushDone;
    Condition*                  m_pParallelFlushDone;                                   ///< Wait till flush is done on all sessions
    Condition*                  m_pFlushstartCondition[MaxSessions];                    ///< Wait to start flush per session
    Mutex*                      m_pFlushThreadMutex[MaxSessions];                       ///< flush count mutex
    UINT32                      m_triggeredFlushedCnt;
    Session*                    m_pSessionsToFlush[MaxSessions];
    BOOL                        m_isUsecaseDestroy;                                     ///< Flag to check if flush is enabled
    BOOL                        m_shouldFlushThreadExecute;
    BOOL                        m_useParallelFlush;
    BOOL                        m_isUsecaseInBadState;                                  ///< Flag to indicate if recovery is required
};

#endif // CHXUSECASE_H
