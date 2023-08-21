////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2wrapper.h
/// @brief CHI feature2 wrapper class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CHIFEATURE2WRAPPER_H
#define CHIFEATURE2WRAPPER_H

#include "chifeature2graphmanager.h"
#include "chifeature2usecaserequestobject.h"
#include "chxadvancedcamerausecase.h"
#include "chxfeature.h"
#include "chxusecase.h"
#include "chxusecaseutils.h"

// NOWHINE FILE CP006:  used standard libraries for performance improvements
// NOWHINE FILE NC004c: CHI files wont be having Camx

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Forward Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CameraUsecaseBase;

/// @brief Feature2 wrapper create input information
struct Feature2WrapperCreateInputInfo
{
    CameraUsecaseBase*      pUsecaseBase;               ///< Usecase base pointer
    ChiMetadataManager*     pMetadataManager;           ///< Metadata manager pointer
    ChiStreamConfigInfo*    pFrameworkStreamConfig;     ///< Framework stream configurations
    std::vector<ChiStream*> internalInputStreams;       ///< Non framework / internal streams
    UINT32                  inputOutputType;            ///< Config stream input out type
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the Feature2 Wrapper.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class Feature2Wrapper : public Feature
{
public:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Create feature2 wrapper object instance.
    ///
    /// @param  pCreateData             Feature2 wrapper object create input information.
    /// @param  physicalCameraIndex     Physical camera index.
    ///
    /// @return Pointer to the Feature2Wrapper upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDK_VISIBILITY_PUBLIC static Feature2Wrapper* Create(
        Feature2WrapperCreateInputInfo* pCreateData,
        UINT32                          physicalCameraIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  Destroy feature2 wrapper object.
    ///
    /// @param  isForced    Wheter or not to force destory the object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy(
        BOOL isForced);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Dump
    ///
    /// @brief  Dump uncompleted usecase request objects
    ///
    /// @param  fd The file descriptor which can be used to write debugging text using dprintf() or write().
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Dump(
        INT fd);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Pause
    ///
    /// @brief  Pause feature2 wrapper object.
    ///
    /// @param  isForced    Wheter or not to force pause the object.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Pause(
        BOOL isForced);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OverrideUsecase
    ///
    /// @brief  Override the usecase
    ///
    /// @param  pCameraInfo     Logical camera information.
    /// @param  pStreamConfig   Stream configurations.
    ///
    /// @return Pointer to the ChiUsecase upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ChiUsecase* OverrideUsecase(
        LogicalCameraInfo*              pCameraInfo,
        camera3_stream_configuration_t* pStreamConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PipelineCreated
    ///
    /// @brief  Feature gets notified through this API when each pipeline is created.
    ///
    /// @param  sessionId       Id of the Session which the created pipeline belongs to.
    /// @param  pipelineId      Id of the pipeline which is created.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID PipelineCreated(
        UINT32 sessionId,
        UINT32 pipelineId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostUsecaseCreated
    ///
    /// @brief  Feature gets notified through this API after the usecase is created.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID PostUsecaseCreated();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestInfo
    ///
    /// @brief  Usecase gets the request information from feature.
    ///
    /// @param  pRequest            Framework request.
    /// @param  pOutputRequests     Inforamtion such as number of buffers and settings requested by the feature.
    /// @param  requestType         Type of the request.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult GetRequestInfo(
        camera3_capture_request_t* pRequest,
        FeatureRequestInfo*        pOutputRequests,
        FeatureRequestType         requestType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetMaxRequiredFramesForSnapshot
    ///
    /// @brief  Get maximum required buffers for this feature
    ///
    /// @param  pMetadata           Input metadata.
    ///
    /// @return Number of required frame buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual UINT32 GetMaxRequiredFramesForSnapshot(
        const camera_metadata_t* pMetadata);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetRequestInfo
    ///
    /// @brief  Usecase gets the request information from feature.
    ///
    /// @param  pRequest            Framework request.
    /// @param  pOutputRequests     Inforamtion such as number of buffers and settings requested by the feature.
    /// @param  requestType         Type of the request.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult GetRequestInfo(
        camera3_capture_request_t*               pRequest,
        std::vector<FeatureRequestInfoForStream> &pOutputRequests,
        FeatureRequestType                       requestType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFeatureType
    ///
    /// @brief  Get the feature type
    ///
    /// @return Feature2
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual FeatureType GetFeatureType()
    {
        return FeatureType::Feature2;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Execute the request
    ///
    /// @param  pRequest            Framework request.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CDKResult ExecuteProcessRequest(
        camera3_capture_request_t* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessCaptureResultCb
    ///
    /// @brief  Feature graph calls this static callback function to process capture result.
    ///
    /// @param  pResult                 CHI capture result.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ProcessCaptureResultCb(
        ChiCaptureResult*           pResult,
        VOID*                       pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessResult
    ///
    /// @brief  Static callback ProcessCaptureResultCb calls this API to process capture result.
    ///
    /// @param  pResult                 CHI capture result.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ProcessResult(
        ChiCaptureResult*           pResult,
        VOID*                       pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMessageCb
    ///
    /// @brief  Feature graph calls this static callback function to process message.
    ///
    /// @param  pMessageDescriptor      CHI feature2 message.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CDKResult ProcessMessageCb(
        const ChiFeature2Messages*  pMessageDescriptor,
        VOID*                       pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessMessage
    ///
    /// @brief  Static callback ProcessMessageCb calls this API to process message.
    ///
    /// @param  pMessageDescriptor      CHI message descriptor.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ProcessMessage(
        const ChiMessageDescriptor* pMessageDescriptor,
        VOID*                       pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessPartialCaptureResultCb
    ///
    /// @brief  Feature graph calls this static callback function to process partial capture result.
    ///
    /// @param  pResult                 CHI partial capture result.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID ProcessPartialCaptureResultCb(
        ChiPartialCaptureResult*    pResult,
        VOID*                       pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessDriverPartialCaptureResult
    ///
    /// @brief  Static callback ProcessPartialCaptureResultCb calls this API to process partial capture result.
    ///
    /// @param  pResult                 CHI partial capture result.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ProcessDriverPartialCaptureResult(
        ChiPartialCaptureResult*    pResult,
        VOID*                       pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessCHIPartialData
    ///
    /// @brief  This will be called by the usecase if CombinedPartialMeta is supported
    ///         Here all the CHI Partial Metadata should be populated and sent to framework as required.
    ///
    /// @param  frameNum   Frame number for which the CHI Partial data should be populated.
    /// @param  sessionId  Corresponding Session Id.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ProcessCHIPartialData(
        UINT32    frameNum,
        UINT32    sessionId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Flush
    ///
    /// @brief  This will be called by the usecase when there is a flush
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Flush();

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Feature2Wrapper
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Feature2Wrapper() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~Feature2Wrapper
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~Feature2Wrapper() = default;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize feature2 wrapper object instance.
    ///
    /// @param  pCreateData    Feature2 wrapper object create input information.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult Initialize(
        Feature2WrapperCreateInputInfo* pCreateData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateFeatureGraphManager
    ///
    /// @brief  Create feature graph manager object instance.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateFeatureGraphManager();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateTargetBufferManagers
    ///
    /// @brief  Create target buffer manager objects for framework streams.
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult CreateTargetBufferManagers();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillInputBufferMeta
    ///
    /// @brief  Fill input buffer and metadata info for this request
    ///
    /// @param  pExtSrcStreamData  Information about external source streams
    /// @param  internalFrameNum   Internal frame number
    /// @param  fwkFrameNum        Framework frame number
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult FillInputBufferMeta(
        ChiFeature2UsecaseRequestObjectExtSrcStreamData* pExtSrcStreamData,
        UINT32                                           internalFrameNum,
        UINT32                                           fwkFrameNum);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateUsecaseRequestObject
    ///
    /// @brief  Create one usecase request object.
    ///
    /// @param  pRequest    Framework request.
    ///
    /// @return Pointer to the ChiFeature2UsecaseRequestObject upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2UsecaseRequestObject* CreateUsecaseRequestObject(
        camera3_capture_request_t* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupInputConfig
    ///
    /// @brief  Create one usecase request object.
    ///
    /// @param  pRequest                Framework request.
    /// @param  pUsecaseRequestObject   Usecase request object
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupInputConfig(
        ChiCaptureRequest*               pRequest,
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObject);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetUROForRequest
    ///
    /// @brief  Get usecaserequestobject for this request from map.
    ///
    /// @param  pRequest    Framework request.
    ///
    /// @return Pointer to the ChiFeature2UsecaseRequestObject upon success or NULL.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ChiFeature2UsecaseRequestObject* GetUROForRequest(
        camera3_capture_request_t* pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessURO
    ///
    /// @brief  Execute process request with this UsecaseRequestObject.
    ///
    /// @param  pUsecaseRequestObject    Usecase request object to pass to execute process request
    /// @param  pRequest                 Framework request associted with this URO
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult ProcessURO(
        ChiFeature2UsecaseRequestObject* pUsecaseRequestObject,
        camera3_capture_request_t*       pRequest);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateFeatureRequestInfo
    ///
    /// @brief  Update output request info with information from input stream data.
    ///
    /// @param  pRequest                 Framework request associted with this URO
    /// @param  pInputStreamData         Input stream data requested by feature graph
    /// @param  pOutputRequestInfo       Output request info to update
    ///
    /// @return CDKResultSuccess if successful or CDK error values in case of error.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CDKResult UpdateFeatureRequestInfo(
        camera3_capture_request_t*                       pRequest,
        ChiFeature2UsecaseRequestObjectExtSrcStreamData* pInputStreamData,
        FeatureRequestInfo*                              pOutputRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFeatureToChiMessage
    ///
    /// @brief  method to process message from the feature.
    ///
    /// @param  pMessageDescriptor      feature message descriptor.
    /// @param  pPrivateCallbackData    Private callback data.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessFeatureToChiMessage(
        const ChiFeature2Messages* pMessageDescriptor,
        VOID*                      pPrivateCallbackData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PassSkipPreviewFlagToUsecase
    ///
    /// @brief  method to pass skip preview flag to m_requestMapInfo in usecase.
    ///
    /// @param  pRequestObject          Usecase request object.
    /// @param  resultFrameIndex        Result frame number index.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PassSkipPreviewFlagToUsecase(
        ChiFeature2UsecaseRequestObject*    pRequestObject,
        UINT32                              resultFrameIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsPreviewStreamFormat
    ///
    /// @brief  method to check if it is preview stream
    ///
    /// @param  format  Stream format.
    ///
    /// @return TRUE if preview stream
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CHX_INLINE BOOL IsPreviewStreamFormat(
        UINT64  format)
    {
        BOOL isPreviewStream = FALSE;

        if (format == ChiStreamFormatImplDefined)
        {
            isPreviewStream = TRUE;
        }

        return isPreviewStream;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckAndSetPreviewBufferSkip
    ///
    /// @brief  method to process message from the feature.
    ///
    /// @param  pCamera3StreamBuffer    Pointer to HAL stream buffer.
    /// @param  format                  Stream format.
    /// @param  resultFrameIndex        Result frame number index.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CheckAndSetPreviewBufferSkip(
        camera3_stream_buffer_t*    pCamera3StreamBuffer,
        UINT64                      format,
        UINT32                      resultFrameIndex);

    Feature2Wrapper(const Feature2Wrapper&)            = delete;   ///< Disallow the copy constructor
    Feature2Wrapper& operator=(const Feature2Wrapper&) = delete;   ///< Disallow assignment operator

    ChiUsecase*                 m_pChiUsecase;                  ///< Pointer to chi usecase
    CameraUsecaseBase*          m_pUsecaseBase;                 ///< Pointer to usecaes base
    ChiFeature2GraphManager*    m_pChiFeatureGraphManager;      ///< Feature graph manager object instance
    LogicalCameraInfo*          m_pLogicalCameraInfo;           ///< Logical camera information
    ChiStreamConfigInfo*        m_pFrameworkStreamConfig;       ///< Framework stream configurations
    UINT32                      m_inputOutputType;              ///< Config stream input out type

    std::map<ChiStream*, CHITargetBufferManager*>           m_frameworkTargetBufferManagers;    ///< Target buffer manager map
    std::map<UINT32, ChiFeature2UsecaseRequestObject*>      m_usecaseRequestObjectMap;          ///< Usecase request object map
    std::vector<CHITargetBufferManager*>                    m_pInputMetadataTBMData;            ///< Usecase metadata manager
    UINT32                                                  m_metaClientId;                     ///< Metadata client Id
    std::map<ChiStream*, ChiStream*> m_internalInputStreamMap;                  ///< Map of usecase internal output streams
                                                                                ///  to FGM external input streams. For example
                                                                                /// UsecaseMC outputRDI stream map to fgm
                                                                                /// externel input RDI stream
    CHIPRIVDATA                      m_offlinePrivData[MaxOutstandingRequests]; ///< private data to be returned to usecase
    CHITAGSOPS                       m_vendorTagOps;                            ///< Vendor Tag Ops
    BOOL                             m_isFlush;                                 ///< Is Flush happeing
    Mutex*                           m_pUROMapLock;                             ///< Lock to synchronize URO Map access
    Mutex*                           m_pFGMLock;                                ///< Lock to synchronize Graph manager access
    UINT32                           m_lastZSLFrameNumber;                      ///< Last ZSL FrameNumber
    Mutex*                           m_pPartialMetadataLock;                    ///< Lock to synchronize Partial Metadata access

};

#endif // CHIFEATURE2WRAPPER_H
