////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxautofocusnode.h
/// @brief AutoFocus node class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXAUTOFOCUSNODE_H
#define CAMXAUTOFOCUSNODE_H

#include "camxcafstatsprocessor.h"
#include "camxdefs.h"
#include "camxistatsprocessor.h"
#include "camxmem.h"
#include "camxnode.h"
#include "camxstatscommon.h"
#include "camxmultistatsoperator.h"
#include "camxtypes.h"

CAMX_NAMESPACE_BEGIN

/// Forward Declarations
class AutoFocusNode;
class CAFAlgorithmHandler;
class CAFIOUtil;

/// @brief Describes the type of PDAF link that needs to be disabled
enum PDAFDisableType
{
    DisableNone,        ///< Do not disable any PDAF Link
    DisableType2,       ///< Disable PDAF Type2 Link
    DisableType3,       ///< Disable PDAF Type3 Link
    DisableTypePDHW,    ///< Disable PDAF PDHW Link
    DisableTypeLCRHW,   ///< Disable PDAF LCRHW Link
    DisableTypeRDIRaw,  ///< Disbale port for LCR SW
    DisableALL,         ///< Disable ALL PDAF Links
} ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the AutoFocus node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class AutoFocusNode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create AutoFocusNode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete AutoFocusNode object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static AutoFocusNode* Create(
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
    /// AutoFocusNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    AutoFocusNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~AutoFocusNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~AutoFocusNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeInitialize
    ///
    /// @brief  Initialize the AutoFocus processing object
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
    /// ExecuteProcessRequest
    ///
    /// @brief  Pure virtual method to trigger process request for the AutoFocus node object.
    ///
    /// @param  pExecuteProcessRequestData Pointer to Execute process request data
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ExecuteProcessRequest(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

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

private:

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsControlAFTriggerPresent
    ///
    /// @brief  Is control AF trigger present in the request
    ///
    /// @return Boolean
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsControlAFTriggerPresent();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPropertyDependencies
    ///
    /// @brief  Get the the list of property dependencies from all stats processors and check if they are satisfied
    ///
    /// @param  pExecuteProcessRequestData          Pointer to Execute process request data
    /// @param  pAutoFocusProcessRequestDataInfo    Pointer to AF process request structure to be filled
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPropertyDependencies(
        ExecuteProcessRequestData*  pExecuteProcessRequestData,
        StatsProcessRequestData*    pAutoFocusProcessRequestDataInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetPDPropertyDependencies
    ///
    /// @brief  Get PD property dependency and check if they are satisfied.
    ///         Add a dependency to receive sequenceIds in the right order for each requestId
    ///         (it means sequenceId 2 should come after seqenceId 1).
    ///
    /// @param  pExecuteProcessRequestData          Pointer to Execute process request data
    /// @param  pAutoFocusProcessRequestDataInfo    Pointer to AF process request structure to be filled
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetPDPropertyDependencies(
        ExecuteProcessRequestData*  pExecuteProcessRequestData,
        StatsProcessRequestData*    pAutoFocusProcessRequestDataInfo
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
    /// GetMultiStatsDependencies
    ///
    /// @brief  Checks if the multi stats dependency list and check if they are satisfied
    ///
    /// @param  pExecuteProcessRequestData          Pointer to Execute process request data
    /// @param  pAutoFocusProcessRequestDataInfo    Pointer to Stats process request data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetMultiStatsDependencies(
        ExecuteProcessRequestData*  pExecuteProcessRequestData,
        StatsProcessRequestData*    pAutoFocusProcessRequestDataInfo
        ) const;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrepareAFProcessRequestData
    ///
    /// @brief  Prepare data to pass to AF ProcessRequest
    ///
    /// @param  pExecuteProcessRequestData          Pointer to Execute process request data
    /// @param  pAutoFocusProcessRequestDataInfo    Pointer to AF process request structure to be filled
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PrepareAFProcessRequestData(
        ExecuteProcessRequestData*  pExecuteProcessRequestData,
        StatsProcessRequestData*    pAutoFocusProcessRequestDataInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// NotifyJobProcessRequestDone
    ///
    /// @brief  Notifies framework about AF node processing is done
    ///
    /// @param  pAutoFocusProcessRequestInfo       Pointer to node process data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID NotifyJobProcessRequestDone(
        StatsProcessRequestData* pAutoFocusProcessRequestInfo);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CanSkipAlgoProcessing
    ///
    /// @brief  Decides whether we can skip algo processing for the current frame based on skip factor
    ///
    /// @param  requestId current request id
    ///
    /// @return TRUE if we can skip otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CanSkipAlgoProcessing(
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeMultiStats
    ///
    /// @brief  Initialize multi camera stats operator for multi camera usecase
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult InitializeMultiStats();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DisableUnwantedPDHWLCRLink
    ///
    /// @brief  Function to disable unused PDHW and LCR link at runtime
    ///
    /// @param  pNumPorts           Number of input ports to be filled in by this function
    /// @param  pPortIds            Array of port Ids filled in by this function (array allocated by caller)
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DisableUnwantedPDHWLCRLink(
        UINT*           pNumPorts,
        UINT*           pPortIds);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DisableUnwantedPDAFLink
    ///
    /// @brief  Function to disable unused PDAF link at runtime
    ///
    /// @param  pNumPorts           Number of input ports to be filled in by this function
    /// @param  pPortIds            Array of port Ids filled in by this function (array allocated by caller)
    /// @param  PDAFLinkDisableType PDAFLinkType that needs to be disabled
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DisableUnwantedPDAFLink(
        UINT*           pNumPorts,
        UINT*           pPortIds,
        PDAFDisableType PDAFLinkDisableType);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsForceSkipRequested
    ///
    /// @brief  Is force skip frame requested
    ///
    /// @return Boolean
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsForceSkipRequested();

    AutoFocusNode(const AutoFocusNode&)            = delete;    ///< Disallow the copy constructor.
    AutoFocusNode& operator=(const AutoFocusNode&) = delete;    ///< Disallow assignment operator.

    StatsInitializeData         m_autoFocusInitializeData;      ///< Save the stats settings pointer
    IStatsProcessor*            m_pAFStatsProcessor;            ///< Pointer to AF Stats processor
    CAFAlgorithmHandler*        m_pAFAlgorithmHandler;          ///< Pointer to the instance of AF Algo handler
    CAFIOUtil*                  m_pAFIOUtil;                    ///< Pointer to the instance of IO Utility handler
    StatsInitializeCallback     m_pStatscallback;               ///< Save the stats callbacks
    ChiContext*                 m_pChiContext;                  ///< Chi context pointer
    UINT32                      m_bufferOffset;                 ///< Offset from which to read buffers
    UINT32                      m_inputSkipFrameTag;            ///< Cached tagId for skip frame tag
    UINT                        m_skipPattern;                  ///< Frame skip pattern
    BOOL                        m_enableFOVC;                   ///< FOVC Enable
    MultiStatsOperator*         m_pMultiStatsOperator;          ///< Multi stats operator for multi camera usecase
    PDAFEnablementConditions*   m_pPDAFEnablementConditions;    ///< pointer to instance of Include PDAFEnablementConditions
    BOOL                        m_isFixedFocus;                 ///< Fixed Focus flag
    UINT32                      m_FOVCFrameControlTag;          ///< FOVC frame control tag
};

CAMX_NAMESPACE_END

#endif // CAMXAUTOFOCUSNODE_H
