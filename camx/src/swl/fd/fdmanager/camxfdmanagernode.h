////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxfdmanagernode.h
/// @brief FDManagerNode class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXFDMANAGERNODE_H
#define CAMXFDMANAGERNODE_H

#include "camxfdhwutils.h"
#include "camxnode.h"
#include "camxstabilization.h"
#include "camxfdengineinterface.h"
#include "camxswprocessalgo.h"
#include "camxncsservice.h"
#include "camxncssensordata.h"

CAMX_NAMESPACE_BEGIN

CAMX_BEGIN_PACKED

/// FD HW results buffer layout definitions

static const UINT NSCSamplecount = 1;

/// @brief GTM algorithm parameters
static const FLOAT GTMExtraGain        = 1.0f;
static const FLOAT DRCGain8XGamma      = 2.2f;
static const FLOAT DRCGain4XGamma      = 2.0f;
static const FLOAT DRCGain2XGamma      = 1.8f;
static const FLOAT DRCGainDefaultGamma = 1.1f;

/// @brief Structure face detect result count
struct FDHwFaceCount
{
    UINT32  dnum    : 6;    /* 5:0 */
    UINT32  unused0 : 26;   /* 31:6 */
} CAMX_PACKED;

/// @brief Structure Face center X coordinate
struct FDHwFaceCenterX
{
    UINT32  centerX : 10;   /* 9:0 */
    UINT32  unused0 : 22;   /* 31:10 */
} CAMX_PACKED;

/// @brief Structure Face center Y coordiante
struct FDHwFaceCenterY
{
    UINT32  centerY : 9;    /* 8:0 */
    UINT32  unused0 : 23;   /* 31:9 */
} CAMX_PACKED;

/// @brief Structure face angle pose
struct FDHwFaceAnglePose
{
    UINT32  angle   : 9;    /* 8:0 */
    UINT32  pose    : 3;    /* 11:9 */
    UINT32  unused0 : 20;   /* 31:12 */
} CAMX_PACKED;

/// @brief Structure face results from FD hardware
struct FDHwFace
{
    FDHwFaceCenterX     centerX;    ///< Face Center x-position
    FDHwFaceCenterY     centerY;    ///< Face Center y-position
    UINT32              sizeConf;   ///< Face Size, Confidence values
    FDHwFaceAnglePose   anglePose;  ///< Face Angle, Pose values
} CAMX_PACKED;

/// @brief Structure face detect results from FD hardware
struct FDHwResults
{
    FDHwFace        faces[CSLFDMaxFaces];   ///< Array of face information
    FDHwFaceCount   faceCount;              ///< Number of faces detected
    UINT32          reserved[3];            ///< Reserved fields
} CAMX_PACKED;

CAMX_END_PACKED

static const UINT32 DefaultFDManagerThreadDataListSize = 5;  ///< Default size of the list that holds thread data

// forward declare FD manager node
class FDManagerNode;

/// @brief Enum indicates the type of job posted to FD Manager thread
enum FDManagerThreadJob
{
    FDManagerThreadJobFDEngineSetup,         ///< Thread job type to initialize FD Engine
    FDManagerThreadJobProcessRequestPriWork, ///< Thread job type to process request primray work
    FDManagerThreadJobProcessRequestSecWork, ///< Thread job type to process request secondary work
};

/// @brief Structure describing FD Engine initilaization information
struct FDManagerFDEngineSetupData
{
    FDEngineCreateParams createParams;  ///< Create parameters for Fd engine
    FDEngineHandle*      phFDEHandle;   ///< FD Engine handle
};

/// @brief Structure describing the process request information.
struct FDManagerProcessRequestPriWorkData
{
    UINT64              requestId;          ///< The unique frame number for this capture request
    FDPerFrameSettings  perFrameSettings;   ///< Per frame settings
    FDHwResults         hwResults;          ///< FD HW results
};

struct FDManagerProcessRequestSecWorkData
{
    UINT64              requestId;          ///< The unique frame number for this capture request
    FDPerFrameSettings  perFrameSettings;   ///< Per frame settings
    FDResults           fdeResults;         ///< FD engine results
};

/// @brief A unit of FDProcessingThreadData
struct FDManagerThreadData
{
    FDManagerNode*                         pInstance;                    ///< this pointer of the FD manager object
    FDManagerThreadJob                     jobType;                      ///< thread job type
    union
    {
        FDManagerFDEngineSetupData         fdEngineCreateInfo;           ///< FD Engine initialization data
        FDManagerProcessRequestPriWorkData processRequestPriWorkInfo;    ///< FD Processing Request primary work data.
        FDManagerProcessRequestSecWorkData processRequestSecWorkInfo;    ///< FD Processing Request secondary work data.
    } jobInfo;
};

/// @brief Structure describing the preprocessing information.
struct FDPreprocessingData
{
    OSLIBRARYHANDLE     hLibHandle;         ///< Handle to preprocessing library
    SWAlgoProcessType   pfnSWAlgoProcess;   ///< Function pointer for SWAlgoProcess API
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the FDHw node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FDManagerNode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create FDManagerNode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete FD Node object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static FDManagerNode* Create(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief  This method destroys the derived instance of the interface
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeInitialize
    ///
    /// @brief  Initialize the hwl object
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeInitialize(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFinalizeInitialization
    ///
    /// @brief  Method to finalize the initialization of the node in the pipeline
    ///
    /// @param  pFinalizeInitializationData Finalize data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeFinalizeInitialization(
        FinalizeInitializationData* pFinalizeInitializationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostPipelineCreate
    ///
    /// @brief  virtual method to be called at NotifyTopologyCreated time; node should take care of updates and initialize
    ///         blocks that has dependency on other nodes in the topology at this time.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PostPipelineCreate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Pure virtual method to trigger process request for the hwl node object.
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ExecuteProcessRequest(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFinalizeInputRequirement
    ///
    /// @brief  Virtual method implemented by FDHw node to determine its input buffer requirements based on all the output
    ///         buffer requirements
    ///
    /// @param  pBufferNegotiationData  Negotiation data for all output ports of a node
    ///
    /// @return Success if the negotiation was successful, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeFinalizeInputRequirement(
        BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FinalizeBufferProperties
    ///
    /// @brief  Finalize the buffer properties of each output port
    ///
    /// @param  pBufferNegotiationData Buffer negotiation data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID FinalizeBufferProperties(
        BufferNegotiationData* pBufferNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// QueryMetadataPublishList
    ///
    /// @brief  Method to query the publish list from the node
    ///
    /// @param  pPublistTagList List of tags published by the node
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult QueryMetadataPublishList(
        NodeMetadataList* pPublistTagList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnStreamOn
    ///
    /// @brief  virtual method to that will be called after streamOn command is sent to HW. HW nodes may use
    ///         this hook to do any stream on configuration. This is generally called everytime ActivatePipeline is called.
    ///         Nodes may use this to setup things that are required while streaming. For exa, any resources that are needed
    ///         only during streaming can be allocated here. Make sure to do light weight operations here as this might delay
    ///         processing of the first request.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult OnStreamOn();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnStreamOff
    ///
    /// @brief  virtual method to that will be called before streamOff command is sent to HW. HW nodes may use
    ///         this hook to do any preparation. This is generally called on every Deactivate Pipeline.
    ///         Nodes may use this to release things that are not required at the end of streaming. For exa, any resources
    ///         that are not needed after stream-on can be released here. Make sure to do light weight operations here as
    ///         releasing here may result in re-allocating resources in OnStreamOn.
    ///         processing of the first request.
    ///
    /// @param  modeBitmask Stream off mode bitmask
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult OnStreamOff(
        CHIDEACTIVATEPIPELINEMODE modeBitmask);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~FDManagerNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~FDManagerNode();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDManagerNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FDManagerNode();
    FDManagerNode(const FDManagerNode&) = delete;                 ///< Disallow the copy constructor.
    FDManagerNode& operator=(const FDManagerNode&) = delete;      ///< Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillInputBufferRequirments
    ///
    /// @brief  Fill input buffer requirments
    ///
    /// @param  pInputBufferRequirement Pointer to input buffer requirment
    /// @param  portId                  Input port id
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillInputBufferRequirments(
        BufferRequirement* pInputBufferRequirement,
        UINT32             portId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPerFrameMetaDataSettings
    ///
    /// @brief  Checks dependencies to get meta data information for this request id
    ///
    /// @param  pNodeRequestData Pointer to process request data
    /// @param  pPerFrameSettings Frame settings for this request id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetPerFrameMetaDataSettings(
        const NodeProcessRequestData* const pNodeRequestData,
        FDPerFrameSettings*                 pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFrameCropInfo
    ///
    /// @brief  Get crop information for this request
    ///
    /// @param  pNodeRequestData Pointer to process request data
    /// @param  pPerFrameSettings Frame settings for this request id
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFrameCropInfo(
        const NodeProcessRequestData* const pNodeRequestData,
        FDPerFrameSettings*                 pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetDeviceOrientation
    ///
    /// @brief  Get device orientation information
    ///
    /// @param  pNodeRequestData Pointer to process request data
    /// @param  pPerFrameSettings Frame settings for this request id
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetDeviceOrientation(
        const NodeProcessRequestData* const pNodeRequestData,
        FDPerFrameSettings*                 pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DetermineFrameSettings
    ///
    /// @brief  Determine frame settings for this request id
    ///
    /// @param  pNodeRequestData  Pointer to process request data
    /// @param  pPerFrameSettings Frame settings for this request id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DetermineFrameSettings(
        const NodeProcessRequestData* const pNodeRequestData,
        FDPerFrameSettings*                 pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAndProcessFDResults
    ///
    /// @brief  Get results and process
    ///
    /// @param  requestId            Request id
    /// @param  pEnabledPorts        Pointer to active ports
    /// @param  pPerFrameSettings    Frame settings for this request id
    /// @param  pRequestHandlingDone Request handling done flag
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAndProcessFDResults(
        UINT64                 requestId,
        PerRequestActivePorts* pEnabledPorts,
        FDPerFrameSettings*    pPerFrameSettings,
        BOOL*                  pRequestHandlingDone);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertResultsFromProcessingToReference
    ///
    /// @brief  Convert FDResults ROI from w.r.t Processing frame to w.r.t Reference Frame
    ///
    /// @param  pProcessingResults FD Results w.r.t processing frame
    /// @param  pReferenceResults  FD Results w.r.t reference frame
    /// @param  pPerFrameSettings  Frame settings for this request id
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConvertResultsFromProcessingToReference(
        FDResults*          pProcessingResults,
        FDResults*          pReferenceResults,
        FDPerFrameSettings* pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertFromProcessingResultsToReferenceFaceROIInfo
    ///
    /// @brief  Convert FDResults ROI from w.r.t Processing frame to FD Property Results w.r.t Reference Frame
    ///
    /// @param  pUnstabilizedResults Unstabilized FD Results w.r.t processing frame
    /// @param  pStabilizedResults   Stabilized FD Results w.r.t processing frame
    /// @param  pFaceROIInfo         FD Property Results w.r.t reference frame
    /// @param  pPerFrameSettings    Frame settings for this request id
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConvertFromProcessingResultsToReferenceFaceROIInfo(
        FDResults*                  pUnstabilizedResults,
        FDResults*                  pStabilizedResults,
        FaceROIInformation*         pFaceROIInfo,
        const FDPerFrameSettings*   pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckROIBound
    ///
    /// @brief  Check and adjust top, left, width, and height values so that ROI falls within frame.
    ///
    /// @param  pResults FD Results w.r.t processing frame
    /// @param  width    Width of the frame
    /// @param  height   Height of the frame
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID CheckROIBound(
        FDResults*  pResults,
        INT32       width,
        INT32       height);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteFDResultsProcessing
    ///
    /// @brief  Execute FD manager results processing
    ///
    /// @param  pHWResults        Pointer to HW results
    /// @param  pPerFrameSettings Frame settings for this request id
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ExecuteFDResultsProcessing(
        const FDHwResults*      pHWResults,
        FDPerFrameSettings*     pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFDFrameSettings
    ///
    /// @brief  Publish frame settings
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishFDFrameSettings(
        FDPerFrameSettings* pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFDInternalFrameSettings
    ///
    /// @brief  Publish frame settings to internal property pool
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishFDInternalFrameSettings(
        FDPerFrameSettings* pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFDFrameSettings
    ///
    /// @brief  Get FD per frame settings
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFDFrameSettings(
        FDPerFrameSettings* pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFdFilterEngine
    ///
    /// @brief  GEt FD engine type
    ///
    /// @return FDEngineType
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FDEngineType GetFdFilterEngine();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFDResults
    ///
    /// @brief  Publish FD results
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishFDResults(
        FDPerFrameSettings* pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFDResultsToMetadata
    ///
    /// @brief  Publish FD results to app
    ///
    /// @param  pPerFrameSettings   Frame settings for this request id
    /// @param  pFaceROIInfo        Pointer to FD results to publish
    /// @param  pFacePSGBResults    Pointer to face landmark, smile, gaze and glink results to publisht
    /// @param  pFaceContourResults Pointer to face contour results to publish
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishFDResultsToMetadata(
        FDPerFrameSettings*            pPerFrameSettings,
        FaceROIInformation*            pFaceROIInfo,
        FDMetaDataResults*             pFacePSGBResults,
        FDMetaDataFaceContourResults*  pFaceContourResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFDResultsToVendorTag
    ///
    /// @brief  Publish FD results to OEM vendor tag
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    /// @param  pFaceROIInfo      Pointer to FD Face ROI info to publish
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishFDResultsToVendorTag(
        FDPerFrameSettings* pPerFrameSettings,
        FaceROIInformation* pFaceROIInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishFDResultstoUsecasePool
    ///
    /// @brief  Publish FD results to usecase pool
    ///
    /// @param  pFaceROIInfo  Pointer to FD results to publish
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishFDResultstoUsecasePool(
        FaceROIInformation* pFaceROIInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFDResultsFromUsecasePool
    ///
    /// @brief  Get FD results from usecase pool
    ///
    /// @param  pFaceROIInfo  Pointer to FD results to return
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetFDResultsFromUsecasePool(
        FaceROIInformation*  pFaceROIInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishPSGBResultstoUsecasePool
    ///
    /// @brief  Publish PT, SM, and/or GB results to usecase pool
    ///
    /// @param  pPerFrameSettings  Frame settings for this request id
    /// @param  pFDResults         Pointer to PSGB results to publish
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishPSGBResultstoUsecasePool(
        FDPerFrameSettings*     pPerFrameSettings,
        FDResults*              pFDResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPSGBResultsFromUsecasePool
    ///
    /// @brief  Get PT, SM, and/or GB results from usecase pool
    ///
    /// @param  pMetaDataResults  Pointer to PSGB results to return
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetPSGBResultsFromUsecasePool(
        FDMetaDataResults* pMetaDataResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishCTResultstoUsecasePool
    ///
    /// @brief  Publish face contour results to usecase pool
    ///
    /// @param  pPerFrameSettings  Frame settings for this request id
    /// @param  pFDResults         Pointer to face contour results to publish
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PublishCTResultstoUsecasePool(
        FDPerFrameSettings*            pPerFrameSettings,
        FDResults*                     pFDResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCTResultsFromUsecasePool
    ///
    /// @brief  Get face contour results from usecase pool
    ///
    /// @param  pFaceContourResults  Pointer to face contour results to return
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetCTResultsFromUsecasePool(
        FDMetaDataFaceContourResults* pFaceContourResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDependencies
    ///
    /// @brief  Sets up dependencies for properties and buffer
    ///
    /// @param  pNodeRequestData    Pointer to process request data
    /// @param  pEnabledPorts       Pointer to active ports
    /// @param  pPerFrameSettings   Pointer to settings for request
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDependencies(
        NodeProcessRequestData* pNodeRequestData,
        PerRequestActivePorts*  pEnabledPorts,
        FDPerFrameSettings*     pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDManagerThreadCb
    ///
    /// @brief  Thread callback function.
    ///
    /// @param  pArg Pointer to callback data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* FDManagerThreadCb(
        VOID* pArg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDManagerJobFDEngineSetupHandler
    ///
    /// @brief  Handles FD engine setup jobs received from FDManagerThreadCb function
    ///
    /// @param  pThreadData Pointer to thread data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FDManagerJobFDEngineSetupHandler(
        FDManagerThreadData* pThreadData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDManagerJobProcessRequestPriWorkHandler
    ///
    /// @brief  Handles primary frame request jobs received from FDManagerThreadCb function
    ///         Work includes face detection, false postive filtering, and if enabled with the primary thread job family, the
    ///         detection of facial parts, smile, gaze and blink
    ///
    /// @param  pThreadData Pointer to thread data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FDManagerJobProcessRequestPriWorkHandler(
        FDManagerThreadData* pThreadData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDManagerJobProcessRequestSecWorkHandler
    ///
    /// @brief  Handles secondary frame request jobs received from FDManagerThreadCb function
    ///         Work inlcudes detection of facial contour, and if enabled with the secondary thread job family, the
    ///         detection of facial parts, smile, gaze and blink
    ///
    /// @param  pThreadData Pointer to thread data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FDManagerJobProcessRequestSecWorkHandler(
        FDManagerThreadData* pThreadData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnProcessRequest
    ///
    /// @brief  Function to Process a new request.
    ///
    /// @param  pProcessRequestData Pointer to process request structure to be filled
    /// @param  pProcessHandled     Out pointer to indicate if process handled
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult OnProcessRequest(
        FDManagerProcessRequestPriWorkData*    pProcessRequestData,
        BOOL*                                  pProcessHandled);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetLocalThreadDataMemory
    ///
    /// @brief  Get an instance from the Thread Data memory pool
    ///
    /// @return Returns a pointer from Thread Data memory pool
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FDManagerThreadData* GetLocalThreadDataMemory();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseLocalThreadDataMemory
    ///
    /// @brief  Release a memory buffer to the Thread Data memory pool
    ///
    /// @param  pThreadData Pointer to thread data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseLocalThreadDataMemory(
        FDManagerThreadData* pThreadData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// StabilizeFaces
    ///
    /// @brief  Uses the latest data from Face Detection to update the historical stabilization data and updates the latest
    ///         data to stabilized values
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    /// @param  pUnstabilized     Pointer to result data of current data
    /// @param  pStabilized       Pointer to result data that will be filled with stabilized results
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult StabilizeFaces(
        const FDPerFrameSettings* pPerFrameSettings,
        FDResults*                pUnstabilized,
        FDResults*                pStabilized);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostProcessFDEngineResults
    ///
    /// @brief  Postprocessing the FD engine results like stabilization, translation
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    /// @param  pUnstabilized     Pointer to FD engine results
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PostProcessFDEngineResults(
        const FDPerFrameSettings* pPerFrameSettings,
        FDResults*                pUnstabilized);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessFDEngineResults
    ///
    /// @brief  Process results through FD engine
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    /// @param  pHWResults        Current HW results for this frame
    /// @param  pEngineResults    Final results from FD engine for this frame
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessFDEngineResults(
        FDPerFrameSettings*     pPerFrameSettings,
        FDIntermediateResults*  pHWResults,
        FDResults*              pEngineResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PerformFDEnginePTDetection
    ///
    /// @brief  Perform facail parts detection
    ///
    /// @param  hFDEngineHandle   FD engine handle
    /// @param  pFrameInfo        Frame information to FD engine
    /// @param  pFDResults        FD results to and from FD engine
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PerformFDEnginePTDetection(
        FDEngineHandle          hFDEngineHandle,
        FDEnginePerFrameInfo*   pFrameInfo,
        FDResults*              pFDResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PerformFDEngineSGBDetection
    ///
    /// @brief  Perform smile, gaze and blink detection
    ///
    /// @param  hFDEngineHandle   FD engine handle
    /// @param  pPerFrameSettings Frame settings
    /// @param  pFrameInfo        Frame information to FD engine
    /// @param  pFDResults        FD results to and from FD engine
    /// @param  sgbDetectMode     Bit flags of the enabled detections
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PerformFDEngineSGBDetection(
        FDEngineHandle          hFDEngineHandle,
        FDPerFrameSettings*     pPerFrameSettings,
        FDEnginePerFrameInfo*   pFrameInfo,
        FDResults*              pFDResults,
        FacialAttributeMask     sgbDetectMode);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PerformFDEngineCTDetection
    ///
    /// @brief  Perform facail contour detection
    ///
    /// @param  hFDEngineHandle  FD engine handle
    /// @param  pFrameInfo       Frame information to FD engine
    /// @param  pFDResults       FD results to and from FD engine
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PerformFDEngineCTDetection(
        FDEngineHandle          hFDEngineHandle,
        FDEnginePerFrameInfo*   pFrameInfo,
        FDResults*              pFDResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PerformFDEnginePriFacialAttrWork
    ///
    /// @brief  Perform the primary facial attribute detection work
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    /// @param  pFDResults        Input and output data for FD engine facial attribute detection
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PerformFDEnginePriFacialAttrWork(
        FDPerFrameSettings*     pPerFrameSettings,
        FDResults*              pFDResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PerformFDEngineSecFacialAttrWork
    ///
    /// @brief  Perform the secondary facial attribute detection work
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    /// @param  pFDResults        Input and output data for FD engine facial attribute detection
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PerformFDEngineSecFacialAttrWork(
        FDPerFrameSettings*     pPerFrameSettings,
        FDResults*              pFDResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeStabilization
    ///
    /// @brief  Initialize stabilization
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeStabilization();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupFDEngineConfig
    ///
    /// @brief  Process results through FD engine
    ///
    /// @param  pFDConfig     Pointer to FD confiugration
    /// @param  pEngineConfig Pointer to FD engine config to update with tuning values
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupFDEngineConfig(
        const FDConfig* pFDConfig,
        FDEngineConfig* pEngineConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintFDResults
    ///
    /// @brief  Print face detection results
    ///
    /// @param  pLogTag     Log tag that identifies the results
    /// @param  frameId     FrameID corresponding to the results
    /// @param  width       Width of the frame
    /// @param  height      Height of the frame
    /// @param  numFaces    Number of faces
    /// @param  pFaceInfo   Face info array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintFDResults(
        const CHAR* pLogTag,
        UINT64      frameId,
        UINT32      width,
        UINT32      height,
        UINT8       numFaces,
        FDFaceInfo* pFaceInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintFDFaceROIInfo
    ///
    /// @brief  Print face ROI information
    ///
    /// @param  pLogTag         Log tag that identifies the results
    /// @param  frameId         FrameID corresponding to the results
    /// @param  width           Width of the frame
    /// @param  height          Height of the frame
    /// @param  pFaceROIInfo    Face info array
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintFDFaceROIInfo(
        const CHAR*         pLogTag,
        UINT64              frameId,
        UINT32              width,
        UINT32              height,
        FaceROIInformation* pFaceROIInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckFDConfigChange
    ///
    /// @brief  Check and load if there is a change in FD configuration
    ///
    /// @param  pFDConfig   Pointer to load FD configuration
    ///
    /// @return TRUE if config has been updated.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckFDConfigChange(
        FDConfig* pFDConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStabilizationConfig
    ///
    /// @brief  Check and load if there is a change in FD configuration
    ///
    /// @param  pFDConfig               Pointer to FD configuration
    /// @param  pStabilizationConfig    Pointer to load stabilization configuration
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetStabilizationConfig(
        const FDConfig*      pFDConfig,
        StabilizationConfig* pStabilizationConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFacialAttrConfig
    ///
    /// @brief  Check and set face number limits for facial attribute detections
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetFacialAttrConfig();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupNCSLink
    ///
    /// @brief  Set up a link to the NCS service from this node
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupNCSLink();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupNCSLinkForSensor
    ///
    /// @brief  Set up a link to the NCS service for sensor
    ///
    /// @param  sensorType    Sensor type for which NCS service is registered
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupNCSLinkForSensor(
        NCSSensorType  sensorType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PopulateGravityData
    ///
    /// @brief  Retrieves gyro info from NCS interface
    ///
    /// @param  gravityValues    gravity values need to be updated
    ///
    /// @return CamxResultSuccess if successful
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PopulateGravityData(
        float gravityValues[]);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFDManagerNodeStreamOff
    ///
    /// @brief  Checks if FD manager node is streamed off
    ///
    /// @return TRUE if streamed off else FALSE
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsFDManagerNodeStreamOff();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputFrameImageBufferInfo
    ///
    /// @brief  Get input frame image buffer information
    ///
    /// @param  pEnabledPorts        Pointer to active ports
    /// @param  pPerFrameSettings    Frame settings for this request id
    ///
    /// @return CamxResultSuccess if successful
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetInputFrameImageBufferInfo(
        PerRequestActivePorts* pEnabledPorts,
        FDPerFrameSettings*    pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDFramePreprocessingRequired
    ///
    /// @brief  Checks if FD frame preprocessing is required
    ///
    /// @param  pPerFrameSettings    Frame settings for this request id
    ///
    /// @return TRUE if streamed off else FALSE
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL FDFramePreprocessingRequired(
        FDPerFrameSettings* pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DoFDFramePreprocessing
    ///
    /// @brief  Do FD frame pre processing
    ///
    /// @param  pPerFrameSettings    Frame settings for this request id
    ///
    /// @return TRUE if streamed off else FALSE
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL DoFDFramePreprocessing(
        FDPerFrameSettings* pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDSWOnlyExecution
    ///
    /// @brief  FD enigne SW processing
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    /// @param  pSWResults        Final results from SWFD for this frame
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FDSWOnlyExecution(
        FDPerFrameSettings*     pPerFrameSettings,
        FDResults*              pSWResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDDLExecution
    ///
    /// @brief  FD DL processing
    ///
    /// @param  pPerFrameSettings Frame settings for this request id
    /// @param  pDLResults        Final results from DL-FD for this frame
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FDDLExecution(
        FDPerFrameSettings*     pPerFrameSettings,
        FDResults*              pDLResults);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OnSetUpFDEngine
    ///
    /// @brief  Post job to FD Manager thread to create and configure FD engine
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult OnSetUpFDEngine();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetUpFDEngine
    ///
    /// @brief  Initial FD engine creation
    ///
    /// @param  pFDEngineCreateParams   Create parameters for Fd engine
    /// @param  phFDEngineHandle        Handle for FD engine
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetUpFDEngine(
        FDEngineCreateParams*   pFDEngineCreateParams,
        FDEngineHandle*         phFDEngineHandle);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessRequestDone
    ///
    /// @brief  Method to signal the request has been completely handled
    ///
    /// @param  pFDManagerNode     this pointer of the FD manager object
    /// @param  pPerFrameSettings  Frame settings for this request id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ProcessRequestDone(
        FDManagerNode*      pFDManagerNode,
        FDPerFrameSettings* pPerFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsAnyJobToSecThreadJobFamily
    ///
    /// @brief  Check if there is pending job to be posted to the secondary thread job family
    ///
    /// @param  pPerFrameSettings  Frame settings for this request id
    ///
    /// @return TRUE if there is pending job to be posted to the secondary thread job family
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsAnyJobToSecThreadJobFamily(
        FDPerFrameSettings* pPerFrameSettings);

    UINT32                  m_titanVersion;                         ///< Camera Hw version
    FDHwMajorVersion        m_FDHwVersion;                          ///< FD Hw version
    UINT32                  m_FDFrameWidth;                         ///< FD Processing frame width
    UINT32                  m_FDFrameHeight;                        ///< FD Processing frame height
    UINT32                  m_FDFrameStride;                        ///< FD Processing frame stride
    UINT32                  m_FDFrameScanline;                      ///< FD Processing frame scanline
    CHIDimension            m_sensorDimension;                      ///< Sensor frame dimension
    CHIRectangle            m_CAMIFMap;                             ///< CAMIF mapping in Active Array
    CHIDimension            m_activeArrayDimension;                 ///< Active array dimension
    ThreadManager*          m_pThreadManager;                       ///< Pointer to thread manager.
    JobHandle               m_hPriThread;                           ///< Handle of the primary thread job family
    JobHandle               m_hSecThread;                           ///< Handle of the secondary thread job family
    FDHwUtils               m_FDHwUtils;                            ///< HW Util class
    Stabilization*          m_pStabilizer;                          ///< Stabilizer to stabilize FD result data
    FDEngineHandle          m_hFDEnginePriHandle;                   ///< Primary FD Engine handle
    FDEngineHandle          m_hFDEngineSecHandle;                   ///< Secondary FD Engine handle
    FDEngineHandle          m_hFDEngineSWHandle;                    ///< FD EngineSW handle
    CamxDimension           m_baseFDHWDimension;                    ///< Base FD HW dimension
    FDConfig                m_FDConfig;                             ///< Face detection tuning configuration
    BYTE*                   m_pUVImageAddress[MaxRequestQueueDepth];///< Copy of input frame for UV
    BYTE*                   m_pImageAddress[MaxRequestQueueDepth];  ///< Copy of input frame
    UINT32                  m_processedFrameCnt;                    ///< Total number of frames processed
    UINT32                  m_skippedFrameCnt;                      ///< Total number of frames skipped
    BOOL                    m_startTimeSaved;                       ///< Whether start time is saved for determining fps skip
    CamxTime                m_startTime;                            ///< First frame start time
    CamxTime                m_currentTime;                          ///< Current frame time
    UINT32                  m_numFacesDetected;                     ///< Number of faces detected in last processed frame
    BOOL                    m_isFrontCamera;                        ///< Whether this instance is for front camera
    BOOL                    m_isVideoMode;                          ///< Whether current mode is video
    NCSSensor*              m_pNCSSensorHandleGravity;              ///< NCS Sensor Gravity handle.
    NCSService*             m_pNCSServiceObject;                    ///< NCS Service object pointer.
    Mutex*                  m_pLock;                                ///< Mutex to protect FD manager internal thread state
    BOOL                    m_enableSwIntermittent;                 ///< Whether to enable SW Intermittent
    MarginRequest           m_EISMarginRatio;                       ///< The total actual EIS margin ratio

    const CameraConfigurationInformation* m_pCameraConfigInfo;      ///< Pointer to Camera configuration information
    LightweightDoublyLinkedList           m_threadDataList;         ///< List to track the thread data
    BOOL                                  m_isNodeStreamedOff;      ///< Flag to indicate if node streamed off
    PerRequestActivePorts*                m_pPerRequestPorts;       ///< Ports info
    FDPreprocessingData                   m_preprocessingData;      ///< FD Frame preprocessing data
    BOOL                                  m_bPTDEnable;             ///< Facial parts detection is enabled or not
    BOOL                                  m_bSMDEnable;             ///< Smile detection is enabled or not
    BOOL                                  m_bGBDEnable;             ///< Gaze and blink detection is enabled or not
    BOOL                                  m_bCTDEnable;             ///< Facial contour detection is enabled or not
    FDThreadTaskAllocationType            m_FDThreadTaskAllocation; ///< To run facial parts detection on pri or sec thread
    UINT32                                m_ptConfigMaxNumOfFaces;  ///< Max number of faces of facial parts detection
    UINT32                                m_smConfigMaxNumOfFaces;  ///< Max number of faces of smile detection
    UINT32                                m_gbConfigMaxNumOfFaces;  ///< Max number of faces of gaze or blink detection
    UINT32                                m_ctConfigMaxNumOfFaces;  ///< Max number of faces of facial contour detection
    UINT32                                m_currentCameraID;        ///< current CameraID
    FDProcessingType                      m_FDType;                 ///< CVP, Internal, or SW type
    UINT32                                m_vendorTagFDProcessSkip; ///< FD rpocessing skip vendorta
};

CAMX_NAMESPACE_END

#endif // CAMXFDMANAGERNODE_H
