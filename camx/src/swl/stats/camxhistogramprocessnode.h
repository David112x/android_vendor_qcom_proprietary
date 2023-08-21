////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhistogramprocessnode.h
/// @brief Histogram Process node class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXHISTOGRAMPROCESSNODE_H
#define CAMXHISTOGRAMPROCESSNODE_H

#include "camxdefs.h"
#include "camxmem.h"
#include "camxnode.h"
#include "camxtypes.h"
#include "camxthreadmanager.h"
#include "camximagebuffer.h"

#include "chihistalgointerface.h"
#include "camxhistalgorithmhandler.h"

CAMX_NAMESPACE_BEGIN

static const FLOAT IHDR_DRC_GAIN        = 1.0f;
static const FLOAT IHDR_DRC_DARK_GAIN   = 1.0f;


/// @brief List of Histogram Process Read Property tags
static UINT32 HistogramProcessPropertyReadTags[] =
{
    PropertyIDAECFrameInfo,
    PropertyIDIFEADRCInfoOutput,
    PropertyIDSensorMetaData
};

/// @brief Number of Histogram Process Read Type Properties
static const UINT NumHistogramProcessPropertyReadTags = sizeof(HistogramProcessPropertyReadTags) / sizeof(UINT32);

/// @brief List of Histogram Process Read Property tags
static UINT64 HistogramProcessPropertyReadTagsOffset[NumHistogramProcessPropertyReadTags] =
{
    0,
    0,
    0,
};

/// @brief List of vendor tags
static const struct NodeVendorTag HistOutputVendorTags[] =
{
    { "org.quic.camera2.statsconfigs", "HistNodeLTCRatioIndex" }
};

/// @brief Index of vendor tags
enum HistOutputVendorTagsIndex
{
    HistNodeLTCRatioIndex   ///< Index for HistNodeLTCRatioIndex
};

static const UINT NumHistOutputVendorTags   = CAMX_ARRAY_SIZE(HistOutputVendorTags);    ///< Number of output vendor tags

struct HistNodeOutData
{
    UINT32 outputLTCRatioIndex;   ///< output LTCRatio for sensor control
};

struct PerFrameInfo
{
    BOOL                    isIHDRMode;         ///< Indicate the sensor is 3HDR mode or not
    BYTE*                   pUVImageAddress;    ///< UV plane image address
    BYTE*                   pImageAddress;      ///< Y plane image address
    UINT64                  requestID;          ///< request ID
    FLOAT                   DRCGain;            ///< DRC gain
    FLOAT                   DRCDarkGain;        ///< DRC dark gain
    FLOAT                   DRCGainLTM;         ///< DRC gain ^ LTM percentage
    FLOAT                   sensorGain;         ///< Sensor real gain programed in this frame
    ImageBuffer*            pImageBuffer;       ///< ImageBuffer handle for input frame buffer
    PerRequestActivePorts*  pPerRequestPorts;   ///< Enabled ports for input images
    AECFrameInformation     AECframeInfo;       ///< AEC frame info
    HistNodeOutData         calculatedData;     ///< Data calculated by histogram node
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the histogram process node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class HistogramProcessNode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create HistogramProcessNode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete HistogramProcessNode object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static HistogramProcessNode* Create(
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
    /// @brief  Initialize the sw processing object
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
    /// GetPropertyDependencies
    ///
    /// @brief  Get the list of property dependencies from all stats processors and check if they are satisfied
    ///
    /// @param  pExecuteProcessRequestData          Pointer to Execute process request data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPropertyDependencies(
        ExecuteProcessRequestData*  pExecuteProcessRequestData
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetBufferDependencies
    ///
    /// @brief  Checks if the buffer dependency list and check if they are satisfied
    ///
    /// @param  pExecuteProcessRequestData      Pointer to Execute process request data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetBufferDependencies(
        ExecuteProcessRequestData*  pExecuteProcessRequestData
    ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Pure virtual method to trigger process request for the stats processing node object.
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ExecuteProcessRequest(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

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
    /// PostPipelineCreate
    ///
    /// @brief  virtual method to be called at NotifyTopologyCreated time; node should take care of updates and initialize
    ///         blocks that has dependency on other nodes in the topology at this time.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult PostPipelineCreate();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// HistogramProcessNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    HistogramProcessNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~HistogramProcessNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~HistogramProcessNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// @brief IStatsNotifier implementation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyJobProcessRequestDone
    ///
    /// @brief  Notifies the statistic node that a job from the worker thread is completed
    ///
    /// @param  requestId   Request Id
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult NotifyJobProcessRequestDone(
        UINT64          requestId);

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
    /// GetInputFrameImageBufferInfo
    ///
    /// @brief  Get input frame image buffer data
    ///
    /// @return CamxResultSuccess if successful
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetInputFrameImageBufferInfo();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAECFrameInfoMetadata
    ///
    /// @brief  Get AECFrameInfo metadata
    ///
    /// @return CamxResultSuccess if successful
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAECFrameInfoMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetHistNodeInputMetadata
    ///
    /// @brief  Get the necessary input metadata for histogram node
    ///
    /// @return CamxResultSuccess if successful
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetHistNodeInputMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PublishMetadata
    ///
    /// @brief  Publish metadata info
    ///
    /// @param  pOutput  Pointer to core algorithm's output
    ///
    /// @return CamxResultSuccess if successful
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PublishMetadata(
        const HistAlgoProcessOutputList* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckDependencyChange
    ///
    /// @brief  Check whether we should execute histogram node or not
    ///
    /// @return BOOL return TRUE if histogram node needs to be executed, otherwise FALSE
    ///
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckDependencyChange();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateHDRInputParam
    ///
    /// @brief  Function to set input into algorithm input handle
    ///
    /// @param  pInputList  List of input parameter
    /// @param  inputType   input parameter type
    /// @param  inputSize   size of input parameter
    /// @param  pValue      payload of the input parameter
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID UpdateHDRInputParam(
        HistAlgoProcessInputList*  pInputList,
        HistAlgoProcessInputType   inputType,
        UINT32                     inputSize,
        VOID*                      pValue);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetInputParamForAlgo
    ///
    /// @brief  Prepare the HDR input params required for core algorithm
    ///
    /// @param  pInputList  Input list to be filled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetInputParamForAlgo(
        HistAlgoProcessInputList* pInputList);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareHDROutputBuffers
    ///
    /// @brief  Prepare output buffers to get HDR algorithm output info
    ///
    /// @param  pOutput  Algo Output to hold
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrepareHDROutputBuffers(
        HistAlgoProcessOutputList*  pOutput);

private:
    HistogramProcessNode(const HistogramProcessNode&)             = delete;       ///< Disallow the copy constructor.
    HistogramProcessNode& operator=(const HistogramProcessNode&)  = delete;       ///< Disallow assignment operator.

    HistAlgorithmHandler*       m_pHDRAlgorithmHandler;                     ///< Pointer to the instance of HDR Algo handler
    ChiHistAlgoProcess*         m_pHDRAlgorithm;                            ///< Pointer to the instance of HDR core interface
    CREATEHISTOGRAMALGOPROCESS  m_pCreate;                                  ///< Pointer to Algo entry function
    UINT32                      m_vendorTagArray[NumHistOutputVendorTags];  ///< Array of tags published by the node
    UINT32                      m_bufferOffset;                             ///< Offset from which to read buffers
    ChiContext*                 m_pChiContext;                              ///< Chi context pointer
    PerFrameInfo                m_perFrameInfo;                             ///< Input per-frame info
    UINT8                       m_LTCRatio;                                 ///< LTC Ratio for Still
    INT32                       m_LTCRatioPercentage;                       ///< LTC Ratio Percentage for Still
    StatsCameraInfo             m_cameraInfo;                               ///< Holds the camera Info
};

CAMX_NAMESPACE_END

#endif // CAMXAWBNODE_H
