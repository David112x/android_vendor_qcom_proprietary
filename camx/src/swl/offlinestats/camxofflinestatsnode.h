////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2018 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxofflinestatsnode.h
/// @brief Offline Stats Node class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXOFFLINESTATSNODE_H
#define CAMXOFFLINESTATSNODE_H
#include "chiawbinterface.h"

CAMX_NAMESPACE_BEGIN
static const UINT32 BGConfigHorizontalRegions         = 64;   ///< Number of horizontal regions configured for BG stats
static const UINT32 BGConfigVerticalRegions           = 48;   ///< Number of vertical regions configured for BG stats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the offline stats node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class OfflineStatsNode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create OfflineStatsNode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete OfflineStatsNode object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static OfflineStatsNode* Create(
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
    /// @brief  Final initialize the hwl object
    ///
    /// @param  pFinalizeInitializationData  init data
    ///
    /// @return CamxResult if successful
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
    /// @brief  Virtual method implemented by IPE node to determine its input buffer requirements based on all the output
    ///         buffer requirements
    ///
    /// @param  pBufferNegotiationData  Negotiation data for all output ports of a node
    ///
    /// @return Success if the negotiation was successful, Failure otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeFinalizeInputRequirement(
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
    /// ~OfflineStatsNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~OfflineStatsNode();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// OfflineStatsNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    OfflineStatsNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostMetadata
    ///
    /// @brief  Posts the results of the offline stats algo
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PostMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareStatsProcessRequestData
    ///
    /// @brief  Prepare data to pass to Stats ProcessRequest
    ///
    /// @param  pExecuteProcessRequestData  Pointer to Execute process request data
    /// @param  pStatsProcessRequestData    Pointer to Stats process request structure to be filled
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PrepareStatsProcessRequestData(
        ExecuteProcessRequestData*  pExecuteProcessRequestData,
        StatsProcessRequestData*    pStatsProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetBufferDependency
    ///
    /// @brief  Set the buffer dependencies
    ///
    /// @param  pNodeRequestData    Pointer to process request data
    /// @param  pEnabledPorts       Pointer to active ports
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetBufferDependency(
        NodeProcessRequestData*   pNodeRequestData,
        PerRequestActivePorts*    pEnabledPorts);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDependencies
    ///
    /// @brief  Check if JPEG aggregator dependencies are met.
    ///
    /// @param  pNodeRequestData    Pointer to process request data
    /// @param  pEnabledPort        Pointer to active ports
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDependencies(
        NodeProcessRequestData*   pNodeRequestData,
        PerRequestActivePorts*    pEnabledPort);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAWBAlgoSetParamInput
    ///
    /// @brief  Gets the required inputs for the core algorithm GetParam
    ///
    /// @param  pInput Pointer to the list of SetParam input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAWBAlgoSetParamInput(
        AWBAlgoSetParamList* pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAWBAlgoProcessInput
    ///
    /// @brief  Gets the required inputs for the core algorithm Process
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    /// @param  pInput                       Pointer to the core input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAWBAlgoProcessInput(
        const StatsProcessRequestData*  pStatsProcessRequestDataInfo,
        AWBAlgoInputList*               pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAWBAlgoExpectedOutputList
    ///
    /// @brief  Populates the algo output structure with required buffers
    ///
    /// @param  pOutput Pointer to the core output data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAWBAlgoExpectedOutputList(
        AWBAlgoOutputList* pOutput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAWBStatistics
    ///
    /// @brief  Get AWB statistics needed by AWB algorithm and add them to the input list.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pInput                          Pointer to the core input data
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetAWBStatistics(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AWBAlgoInputList*              pInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ParseAWBStats
    ///
    /// @brief  Arrange ISP HW stats in common format.
    ///
    /// @param  pStatsProcessRequestDataInfo Pointer to process request information
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ParseAWBStats(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAWBBayerGrid
    ///
    /// @brief  Once statistics have been parsed, this function arranges the info into the algorithm structure.
    ///
    /// @param  pBayerGridOutput Pointer to the internal property of parsed output
    /// @param  pStatsConfig     Pointer to the internal property for stats config
    /// @param  pBayerGrid       Pointer to the stats structure used by the algorithm.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAWBBayerGrid(
        ParsedAWBBGStatsOutput* pBayerGridOutput,
        ISPAWBBGStatsConfig*    pStatsConfig,
        StatsBayerGrid*         pBayerGrid);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetAWBFrameControl
    ///
    /// @brief  Gets the current settings of frameControl and statsControl
    ///
    /// @param  pFrameControl    Frame control to fill
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID GetAWBFrameControl(
        AWBFrameControl*   pFrameControl);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillBGConfigurationData
    ///
    /// @brief  Fill AWB BG statistics configuration data, either use algorithm output or use default configuration defined by
    ///         this node.
    ///
    /// @param  pAWBConfigData  Pointer to the stats configuration to be filled
    /// @param  pAWBAlgoConfig  Pointer to the AWB algorithm configuration requested. If null or invalid, default configuration
    ///                         will be fill.
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillBGConfigurationData(
        AWBConfig*                                  pAWBConfigData,
        const StatsBayerGridBayerExposureConfig*    pAWBAlgoConfig);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrePublishMetadata
    ///
    /// @brief  Publish the frame controls for initial frames
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrePublishMetadata();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateAWBAlgorithm
    ///
    /// @brief  Create AWB algorithm
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateAWBAlgorithm();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddChiStatsSessionHandle
    ///
    /// @brief  Get the stats chi session data.
    ///
    /// @param  pStatsProcessRequestDataInfo    Pointer to process request information
    /// @param  pInputList                      Pointer to the input list
    ///
    /// @return Success or failure
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddChiStatsSessionHandle(
        const StatsProcessRequestData* pStatsProcessRequestDataInfo,
        AWBAlgoInputList*              pInputList);

    CREATEAWB                 m_pfnCreate;                      ///< Pointer to algorithm entry function
    CamX::OSLIBRARYHANDLE     m_hHandle;                        ///< handle for custom algo.
    CHIAWBAlgorithm*          m_pAWBAlgorithm;                  ///< Pointer to instance of AWB algorithm interface
    ExecuteProcessRequestData m_procReqData[NumProcReqData];    ///< old requests

    AWBAlgoInput            m_processInputArray[AWBInputTypeLastIndex];                 ///< Array of algo Process input
    AWBAlgoOutput           m_processOutputArray[AWBOutputTypeLastIndex];                ///< Array of algo Process outputs
    AWBAlgoGetParamInput    m_getParamInputArray[AWBGetParamInputTypeLastIndex];         ///< Array of algo getparam inputs
    AWBAlgoGetParamOutput   m_getParamOutputArray[AWBGetParamOutputTypeLastIndex];       ///< Array of algo getparam outputs
    AWBAlgoSetParam         m_setParamInputArray[AWBGetParamInputTypeLastIndex];         ///< Array of algo setparam inputs
    StatsParser*                      m_pStatsParser;                                    ///< ISP stats parser

    AWBAlgoGains                      m_gains;                                      ///< Holds algorithm AWB gain output
    UINT32                            m_cct;                                        ///< Holds algorithm AWB CCT output
    StatsIlluminantType               m_sampleDecision[AWBAlgoDecisionMapSize];     ///< Holds the array of AWB map decision
    StatsBayerGridBayerExposureConfig m_BGConfig;                                   ///< Holds the stats configuration requested
                                                                                    ///  by AWB algorithm
    AWBAlgoState                      m_state;                                      ///< Holds the AWB algorithm state
    AWBAlgoMode                       m_mode;                                       ///< Holds the AWB algorithm operation mode.
    BOOL                              m_lock;                                       ///< Holds the AWB algorithm lock status.
    StatsAWBCCMList                   m_CCMList;                                    ///< Holds Algo's CCM output.
    StatsBayerGrid                    m_statsBayerGrid;                             ///< Holds the base struct that
                                                                                    /// have the ptr to current stats
    UINT64                            m_requestId;                                  ///< Request Frame Number
    ChiStatsSession                   m_chiStatsSession;                            ///< Holds stats session data used for
                                                                                    /// handling vendor tag operations
    ChiContext*                       m_pChiContext;                                ///< Chi context pointer
    StatsInitializeData               m_statsInitializeData;                        ///< Save the stats settings pointer
    BOOL                              m_isOffline;                                  ///< Offline/Online stats check
    BOOL                              m_bUsePreviousRequestBuffers;                 ///< Whether offline stats node uses
                                                                                    ///  previous request buffers

    OfflineStatsNode(const OfflineStatsNode&) = delete;                             ///< Disallow the copy constructor.
    OfflineStatsNode& operator=(const OfflineStatsNode&) = delete;                  ///< Disallow assignment operator.

};

CAMX_NAMESPACE_END

#endif // CAMXOFFLINESTATSNODE_H
