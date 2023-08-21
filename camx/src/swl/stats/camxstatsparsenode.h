////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxstatsparsenode.h
/// @brief Stats Parse Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXSTATSPARSENODE_H
#define CAMXSTATSPARSENODE_H

#include "camxdefs.h"
#include "camxmem.h"
#include "camxnode.h"
#include "camxtypes.h"
#include "camxpipeline.h"

CAMX_NAMESPACE_BEGIN

static const UINT32 MaxStatsParseDependentFences = 20;   ///< Maximum number of fences statistics node dependent on

/// @brief Structure describing the stats buffer information
struct StatsParseBufferInfo
{
    ISPStatsType    statsType;  ///< Stats type
    ImageBuffer*    pBuffer;    ///< Buffer associated with the stats coming in the port
    CSLFence*       phFences;   ///< Fences associated with the stats coming in the port
};

/// @brief Structure describing the process request information.
struct StatsParseProcessRequestData
{
    Node*                pNode;                                    ///< Node requesting processing
    UINT64               requestId;                                ///< The unique frame number for this capture request
    UINT32               pipelineDelay;                            ///< The pipeline delay of the CamX subsystem
    INT32                bufferCount;                              ///< Buffer count
    StatsParseBufferInfo bufferInfo[MaxStatsParseDependentFences]; ///< The buffer information of stats input buffer
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the Tintless Stats Parse node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class StatsParseNode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create StatsParseNode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete IFENode object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static StatsParseNode* Create(
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
    /// @brief  Initialize the sw processing object
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProcessingNodeInitialize(
        const NodeCreateInputData*  pCreateInputData,
        NodeCreateOutputData*       pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteProcessRequest
    ///
    /// @brief  Pure virtual method to trigger process request for the hwl node object.
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ExecuteProcessRequest(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

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
    /// StatsParseNode
    ///
    /// @brief  Constructor
    ///
    /// @param  pCreateInputData Input state
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    explicit StatsParseNode(
        const NodeCreateInputData* pCreateInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~StatsParseNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~StatsParseNode();

private:
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
        ExecuteProcessRequestData*  pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFrameStatsOutput
    ///
    /// @brief  Gets the member to hold the stats output
    ///
    /// @param  statsType Type of stats being output
    /// @param  requestId Request for which stats is being output
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID* GetFrameStatsOutput(
        ISPStatsType statsType,
        UINT64       requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsForceSkipRequested
    ///
    /// @brief  Is force skip frame requested
    ///
    /// @return Boolean
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsForceSkipRequested();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStatsPortIndex
    ///
    /// @brief  Gets the the portID coresponding to stats type
    ///
    /// @param  statsType       Type of stats for which portID needs to be fetched
    /// @param  pEnabledPorts   Pointer to enabled ports
    /// @param  pPortIndex      port index for the corresponding stats type
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetStatsPortIndex(
        ISPStatsType            statsType,
        PerRequestActivePorts*  pEnabledPorts,
        UINT32*                 pPortIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStatsTypeProperties
    ///
    /// @brief  Gets array of properties coresponding to stats type, and it's length
    ///
    /// @param  statsType        Type of stats for which properties need to be fetched
    /// @param  ppPropertyTags   Pointer to array of properites
    ///
    /// @return Number of properties in array (0 if none)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetStatsTypeProperties(
        ISPStatsType       statsType,
        const PropertyID** ppPropertyTags);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStatsTypeVendorTags
    ///
    /// @brief  Gets array of properties coresponding to stats type, and it's length
    ///
    /// @param  statsType        Type of stats for which vendor tags need to be fetched
    /// @param  ppVendorTags     Pointer to array of vendor tags
    ///
    /// @return Number of vendor tags in array (0 if none)
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetStatsTypeVendorTags(
        ISPStatsType                 statsType,
        const struct NodeVendorTag** ppVendorTags);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddMetadataTagsForStatsType
    ///
    /// @brief  Adds the metadata property tags for the specified ISP stats type
    ///
    /// @param  statsType        Type of stats for which portID needs to be fetched
    /// @param  pPublistTagList  List of tags published by the node
    /// @param  pTagCount        Pointer to the number of already published tags in the list
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddMetadataTagsForStatsType(
        ISPStatsType            statsType,
        NodeMetadataList*       pPublistTagList,
        UINT32*                 pTagCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AddVendorTagsForStatsType
    ///
    /// @brief  Adds the vendor tags for the specified ISP stats type
    ///
    /// @param  statsType        Type of stats for which portID needs to be fetched
    /// @param  pPublistTagList  List of tags published by the node
    /// @param  pTagCount        Pointer to the number of already published tags in the list
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AddVendorTagsForStatsType(
        ISPStatsType            statsType,
        NodeMetadataList*       pPublistTagList,
        UINT32*                 pTagCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPortIndexStatsType
    ///
    /// @brief  Gets the stats type produced by the provided port id
    ///
    /// @param  portIndex      port index for the corresponding stats type
    /// @param  pStatsType     Type of stats for which portID needs to be fetched
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPortIndexStatsType(
        UINT32                  portIndex,
        ISPStatsType*           pStatsType);

    StatsParseNode(const StatsParseNode&) = delete;                 ///< Disallow the copy constructor
    StatsParseNode& operator=(const StatsParseNode&) = delete;      ///< Disallow assignment operator

    ThreadManager*               m_pThreadManager;                  ///< Pointer to thread manager.
    HwContext*                   m_pHwContext;                      ///< Pointer to HW context.
    StatsParser*                 m_pStatsParser;                    ///< ISP stats parser
    UINT32                       m_bufferOffset;                    ///< Offset from which to read buffers
    UINT                         m_skipPattern;                     ///< Value - 1 == number of frames skipped between parses
    UINT32                       m_inputSkipFrameTag;               ///< Cached tagId for skip frame tag
    PerRequestActivePorts        m_defaultPorts;                    ///< Ports to publish default properties for in first frames

    volatile UINT                m_aBuffersCompleted[NumProcReqData]; ///< Number of buffers processed. Atomic

    ParsedAWBBGStatsOutput*       m_pAWBBGStatsOutput;              ///< AWBBG stats data
    ParsedBHistStatsOutput*       m_pBHistStatsOutput;              ///< BHist stats data
    ParsedHDRBEStatsOutput*       m_pHDRBEStatsOutput;              ///< HDRBE stats data
    ParsedHDRBHistStatsOutput*    m_pHDRBHistStatsOutput;           ///< HDRBHist stats data
    ParsedIHistStatsOutput*       m_pIHistStatsOutput;              ///< IHist stats data
    ParsedRSStatsOutput*          m_pRSStatsOutput;                 ///< RS stats data
    ParsedTintlessBGStatsOutput*  m_pTintlessBGStatsOutput;         ///< Tintless BG stats Data

    UINT32                        m_requestQueueDepth;              ///< Request Queue Depth
};

CAMX_NAMESPACE_END
#endif // !CAMXSTATSPARSENODE_H
