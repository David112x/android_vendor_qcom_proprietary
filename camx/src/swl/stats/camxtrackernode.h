////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxtrackernode.h
/// @brief TrackerNode class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXTRACKERNODE_H
#define CAMXTRACKERNODE_H

#include "camxnode.h"
#include "chitrackerinterface.h"
#include "chistatsproperty.h"
#include "camxlist.h"
#include "camxstatsparser.h"
#include "camxutils.h"
#include "camxstatscommon.h"

CAMX_NAMESPACE_BEGIN

class TrackerNode;

static const UINT MaxTrackerBufferSize          = 640 * 480 * 3 / 2;    ///< Buffer size per image buffer YUV420
static const UINT NumOfTrackerNodeThreadData    = 3;                    ///< Num of image buffer queue for processing

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief ThreadData for algorithm thread
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TrackerThreadData
{
    TrackerNode*                pNode;                  ///< this pointer of the tracker Node
    ImageBuffer*                pImgBuf;                ///< Pointer to IFE img buffer
    UINT64                      requestId;              ///< RequestID Processing Request data.
    TrackerCMDType              triggerCmd;             ///< command trigger
    StatsRectangle              registerROI;            ///< ROI used for object registration
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the Tracker node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TrackerNode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create TrackerNode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete Tracker Node object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static TrackerNode* Create(
        const NodeCreateInputData*  pCreateInputData,
        NodeCreateOutputData*       pCreateOutputData);

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
    /// @brief  Initialize the object
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeInitialize(
        const NodeCreateInputData*  pCreateInputData,
        NodeCreateOutputData*       pCreateOutputData);

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
    /// @brief  Virtual method implemented by Tracker node to determine its input buffer requirements based on all the output
    ///         buffer requirements
    ///
    /// @param  pBufferNegotiationData  Negotiation data for all output ports of a node
    ///
    /// @return Success if the negotiation was successful, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeFinalizeInputRequirement(
        BufferNegotiationData* pBufferNegotiationData);

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
    /// ~TrackerNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~TrackerNode();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TrackerNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    TrackerNode();
    TrackerNode(const TrackerNode&)             = delete;               ///< Disallow the copy constructor.
    TrackerNode& operator=(const TrackerNode&)  = delete;               ///< Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferDependencies
    ///
    /// @brief  Sets up dependencies for buffer
    ///
    /// @param  pExecuteProcessRequestData      Pointer to Execute process request data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetBufferDependencies(
        ExecuteProcessRequestData*  pExecuteProcessRequestData
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpTrackerYUV
    ///
    /// @brief  utility function to dump YUV for debug
    ///
    /// @param  requestId      request ID
    /// @param  pImageBuffer   Pointer to CamX ImageBuffer
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DumpTrackerYUV(
        UINT64          requestId,
        ImageBuffer*    pImageBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareThreadData
    ///
    /// @brief  fill the internal buffer by port input buffer
    ///
    /// @param  pExecuteProcessRequestData      Pointer to Execute process request data
    /// @param  pTrackBuffer                    Pointer to input buffer
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PrepareThreadData(
        ExecuteProcessRequestData*  pExecuteProcessRequestData,
        ImageBuffer*                pTrackBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsNodeStreamOff
    ///
    /// @brief  Checks if node is streamed off
    ///
    /// @return TRUE if streamed off else FALSE
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsNodeStreamOff();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TrackerThreadCb
    ///
    /// @brief  thread callback function for tracker cmds
    ///
    /// @param  pArg      Pointer to thread data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static VOID* TrackerThreadCb(
        VOID* pArg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyJobProcessRequestDone
    ///
    /// @brief  Notifies framework about Tracker node processing is done
    ///
    /// @param  requestID       requestID to inform processing is done
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyJobProcessRequestDone(
        UINT64 requestID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishTrackerResultsToVendorTag
    ///
    /// @brief  Publish tracker resoult to vendor tag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishTrackerResultsToVendorTag();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishTrackerResultstoUsecasePool
    ///
    /// @brief  Publish tracker ROI to usecase pool dimensions
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishTrackerResultstoUsecasePool();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetTrackerResultsFromUsecasePool
    ///
    /// @brief  get the latest tracker results from usecase
    ///
    /// @param  pROIInfo  Pointer to Tracker results to from usecasepool
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetTrackerResultsFromUsecasePool(
        TrackerROIInformation* pROIInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFrameCropInfo
    ///
    /// @brief  GetFrameCropInfo
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFrameCropInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateT2TFeatureEnable
    ///
    /// @brief  UpdateT2TFeatureEnable
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateT2TFeatureEnable();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputFromVendortag
    ///
    /// @brief  GetInputFromVendortag
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetInputFromVendortag();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateTrackerAlgorithm
    ///
    /// @brief  Creating tracker algorithm
    ///
    /// @param  pExecuteProcessRequestData  Pointer to EPR data
    ///
    /// @return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateTrackerAlgorithm(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CanSkipAlgoProcessing
    ///
    /// @brief  Decides whether we can skip algo processing for the current frame based on skip factor
    ///
    /// @param  requestId current request id
    ///
    /// @return Boolean
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CanSkipAlgoProcessing(
        UINT64 requestId);

    BOOL                        m_staticSettingsTrackingEnable;             ///< Enable T2T from static settings
    UINT                        m_trackerImgBufDownscaleRatio;              ///< Tracker algorithm Downscale Ratio
    TrackerOperationMode        m_trackerOperationMode;                     ///< Tracker Algorithm operation mode
    TrackerPrecisionMode        m_trackerPrecisionMode;                     ///< Tracker algorithm precision mode

    BOOL                        m_trackingEnable;                           ///< Enable T2T from metadata
    TrackerCMDType              m_trackerTriggerCmd;                        ///< to change to per frame setting implement
    StatsRectangle              m_trackerRegisterROI;                       ///< to change to per frame setting implement
    TrackerResults              m_trackerResultsFromAlgo;                   ///< algo output
    TrackerCMDType              m_previousIncomingCmd;                      ///< Previous command from Application
    StatsRectangle              m_previousRegistrationROI;                  ///< Previous registration ROI from application

    TrackerThreadData*          m_pThreadData[NumOfTrackerNodeThreadData];  ///< Pointer to threadData
    JobHandle                   m_hThread;                                  ///< Thread handle
    Mutex*                      m_pLock;                                    ///< Mutex to protect internal thread state

    CamxDimension               m_baseFDHWDimension;                        ///< Base FD HW dimension
    CHIDimension                m_sensorDimension;                          ///< Sensor frame dimension CAMIF
    CHIDimension                m_sensorActiveArrayDimension;               ///< Active array dimension
    CHIDimension                m_trackerBufDimension;                      ///< FD internal buffer dimension
    StatsRectangle              m_aspectRatioOffset;                        ///< Offset in CAMIF in preview path
    BOOL                        m_aspectRatioCalibrated;                    ///< Whether aspect ratio offset has considered

    CHIRectangle                m_appliedCropFD;                            ///< Input frame mapp w.r.t CAMIF for FD path
    CHIRectangle                m_appliedCropPreview;                       ///< Input frame mapp w.r.t CAMIF for Preview path
    UINT32                      m_trackerFrameStride;                       ///< Tracker Processing frame stride
    UINT32                      m_trackerFrameScanline;                     ///< Tracker Processing frame scanline

    BOOL                        m_isNodeStreamedOff;                        ///< Flag to indicate if node streamed off
    StatsInitializeData         m_statsInitializeData;                      ///< initializeData
    StatsCameraInfo             m_cameraInfo;                               ///< Camera Info
    CHITrackerAlgorithm*        m_pTrackerAlgorithm;                        ///< Poitner to algorithm implementation
    UINT                        m_skipPattern;                              ///< Stats skip pattern
    TrackerROIInformation       m_trackerROIInfo;                           ///< trackerROIInfo stored
};

CAMX_NAMESPACE_END

#endif // CAMXTrackerNODE_H