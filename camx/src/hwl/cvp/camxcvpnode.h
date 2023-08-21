////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxcvpnode.h
/// @brief CVPNode class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NOWHINE FILE CP021: default arguments in functions

#ifndef CAMXCVPNODE_H
#define CAMXCVPNODE_H

#include "camxmem.h"
#include "camxnode.h"
#include "camxcmdbuffermanager.h"
#include "camxhwcontext.h"
#include "camxtitan17xcontext.h"
#include "camxtitan17xdefs.h"
#include "camxcvpproperty.h"
#include "TransformEstimation.h"
#include "camxispiqmodule.h"
#include "camxisppipeline.h"
#include "cvpSession.h"
#include "cvpDme.h"
#include "synx.h"
#include "camxiqinterface.h"

CAMX_NAMESPACE_BEGIN

/// @brief CVP Dme callback private data structure
struct CvpDmeCbPrivData
{
    UINT32      dmeSessionUserData;      ///< dme session user data
    VOID*       pNode;                   ///< pointer to node object
};

typedef CvpDmeCbPrivData CVPDmeCbPrivData;

/// @brief CVP node capabilities
struct CvpCapability
{
    UINT   minInputWidth;          ///< Min input width that CVP can take
    UINT   minInputHeight;         ///< Min input height that CVP can take
    UINT   maxInputWidth;          ///< Max input width that CVP can take
    UINT   maxInputHeight;         ///< Max input height that CVP can take
    UINT   optimalInputWidth;      ///< Max input width that CVP can take
    UINT   optimalInputHeight;     ///< Max input height that CVP can take
    UINT32 formats;                ///< Formats supported by CVP
};

static const CHAR* CVPPortName[] =
{
    "OutputVector",  // 0  -  Output
    "OutputData",    // 1  -  Output
    "OutputImage",   // 2  -  Output
    "TARIFEFull",    // 3  -  Input
    "REFIFEFull",    // 4  -  Input
    "TARIFEDS4",     // 5  -  Input
    "REFIFEDS4",     // 6  -  Input
    "TARIFEDS16",    // 7  -  Input
    "REFIFEDS16",    // 8  -  Input
    "InputData",     // 9  -  Input
    "Unknown",       // 10 -  Unknown
};

/// @brief Fence description
enum CVPFences
{
    FullResImage            = 0, ///< Fence for FullResImage
    ScaledSrcImage,              ///< Fence for ScaledSrcImage
    ScaledSrcFrameCtxt,          ///< Fence for ScaledSrcFrameCtxt
    PespectiveBuffer,            ///< Fence for PespectiveBuffer
    GridBuffer,                  ///< Fence for GridBuffer
    ScaledRefImage,              ///< Fence for ScaledRefImage
    ScaledRefFrameCtxt,          ///< Fence for ScaledRefFrameCtxt
    DMEOutput                    ///< Fence for DMEOutput
};

/// @brief Fence description
enum CVPSessionPriority : UINT32
{
    CVP_SESSION_PRIORITY_DME_VIDEO_PATH     = 0x8004,    ///< DME video path (EIS3)
    CVP_SESSION_PRIORITY_MF_REG_PATH        = 0x8006,    ///< MFNR/MFSR Registration
    CVP_SESSION_PRIORITY_DME_PREVIEW_PATH   = 0x8007     ///< DME preview/ regular video path and EIS2
};

static const UINT32 CVPSessionTypeDME = 0x8001; ///< cvp session type mask for DME usecase

/// @brief CVP Properties for ICA
static const UINT CVPProperties                       = 2;

static const UINT MaxTransformConfidence              = 256;                              ///< Max transform confidence [0-256]
static const UINT CVPPortNameMaxIndex                 = CAMX_ARRAY_SIZE(CVPPortName) - 1; ///< Number of strings in CVPPortName
                                                                                          ///  array
static const UINT MaxCVPIQModule                      = 1;                                ///< Max Number of IQ Modules

/// @brief CVP alignments depend on different input image format
static const UINT CVPStrideAlignmentNV12PlaneY        = 128;
static const UINT CVPStrideAlignmentNV12PlaneUV       = 128;
static const UINT CVPHeightAlignmentNV12PlaneY        = 32;
static const UINT CVPHeightAlignmentNV12PlaneUV       = 16;

static const UINT CVPStrideAlignmentUBWCNV124RPlaneY  = 256;
static const UINT CVPStrideAlignmentUBWCNV124RPlaneUV = 256;
static const UINT CVPHeightAlignmentUBWCNV124RPlaneY  = 16;
static const UINT CVPHeightAlignmentUBWCNV124RPlaneUV = 16;

static const UINT CVPOutputPortDataDefaultSize        = 40000;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the CVP node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CVPNode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief     Static method to create CVPNode Object.
    ///
    /// @param     pCreateInputData  Node create input data
    /// @param     pCreateOutputData Node create output data
    ///
    /// @return    Pointer to the concrete CVPNode object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static CVPNode* Create(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Destroy
    ///
    /// @brief     This method destroys the derived instance of the interface
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID Destroy();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// TriggerRecoveryOnError
    ///
    /// @brief     This method triggers the node recovery on cvp fatal error
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID TriggerRecoveryOnError()
    {
        RequestRecovery();
    }

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CVPNode
    ///
    /// @brief     Constructor to initialize CVP node instance constants
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CVPNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~CVPNode
    ///
    /// @brief     Destructor
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~CVPNode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeInitialize
    ///
    /// @brief     Initialize the hwl object
    ///
    /// @param     pCreateInputData     Node create input data
    /// @param     pCreateOutputData    Node create output data
    ///
    /// @return    CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult ProcessingNodeInitialize(
        const NodeCreateInputData* pCreateInputData,
        NodeCreateOutputData*      pCreateOutputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostPipelineCreate
    ///
    /// @brief     virtual method to be called at NotifyTopologyCreated time; node should take care of updates and initialize
    ///            blocks that has dependency on other nodes in the topology at this time.
    ///
    /// @return    CamxResultSuccess if successful
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
    /// @brief  Virtual method implemented by CVP node to determine its input buffer requirements based on all the output
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
    /// IsNodeDisabledWithOverride
    ///
    /// @brief  virtual method that will be called during NewActiveStreamsSetup. Nodes may use
    ///         this hook to disable processing if theyre disabled through settings.
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsNodeDisabledWithOverride();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetInputPortName
    ///
    /// @brief  Get input port name for the given port id.
    ///
    /// @param  portId  Port Id for which name is required
    ///
    /// @return Pointer to Port name string
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual const CHAR* GetInputPortName(
        UINT portId) const
    {
        if (portId > CVPPortNameMaxIndex)
        {
            portId = CVPPortNameMaxIndex;
        }

        return CVPPortName[portId];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetOutputPortName
    ///
    /// @brief  Get output port name for the given port id.
    ///
    /// @param  portId  Port Id for which name is required
    ///
    /// @return Pointer to Port name string
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual const CHAR* GetOutputPortName(
        UINT portId) const
    {
        if (portId > CVPPortNameMaxIndex)
        {
            portId = CVPPortNameMaxIndex;
        }

        return CVPPortName[portId];
    }

private:
    CVPNode(const CVPNode&) = delete;                 ///< Disallow the copy constructor.
    CVPNode& operator=(const CVPNode&) = delete;      ///< Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Cleanup
    ///
    /// @brief     This method cleans up any resources allocated by node
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Cleanup();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCVPSession
    ///
    /// @brief     This method create CVP Session
    ///
    /// @return    cvpSession handle
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    cvpSession CreateCVPSession();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CleanupSession
    ///
    /// @brief     Helper method to cleanup CVP session
    ///
    /// @return    CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CleanupSession();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupCVPFormats
    ///
    /// @brief     This method sets up CVP formats that are supported by the h/w
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupCVPFormats();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckInputRequirement
    ///
    /// @brief     This method checks if the input resolution covers the range supported by LRME Node
    ///
    /// @param     inputPortRequirement    The input resolution to validate
    /// @param     maxSupportedResolution  The max supported resolution in hardware
    ///
    /// @return    CamxResultSuccess if the input resolution covers the LRME node supported range
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CheckInputRequirement(
        BufferRequirement inputPortRequirement,
        BufferRequirement maxSupportedResolution);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupVectorOutputFinalResolution
    ///
    /// @brief     This method fills in the resolution for LRME vector output port
    ///
    /// @param     pFinalOutputBufferProperties    Out parameter where final buffer resolution is filled
    /// @param     width                           The width of image on which lrme algo runs in hardware
    /// @param     height                          The height of image on which lrme algo runs in hardware
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupVectorOutputFinalResolution(
        BufferProperties* pFinalOutputBufferProperties,
        UINT width,
        UINT height);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupVectorOutputRequirementOptions
    ///
    /// @brief     This method calculates the resolution options for LRME vector output port based on input requirement
    ///
    /// @param     pOutputPortNegotiationData      Negotiation data of vector output port
    ///
    /// @return    CamxResultSuccess if resolution calculation is successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetupVectorOutputRequirementOptions(
        OutputPortNegotiationData* pOutputPortNegotiationData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureCVPCapability
    ///
    /// @brief     This method sets up CVP node capabilities
    ///
    /// @return    CamxResultSuccess if the capabilities were setup
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureCVPCapability();


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
    /// SetDependencies
    ///
    /// @brief  Set dependencies required for processing this request
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDependencies(
        ExecuteProcessRequestData* pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetICADependencies
    ///
    /// @brief  Check the availability of the dependence data. Update the data if it is available
    ///
    /// @param  pNodeRequestData           Pointer to the incoming NodeProcessRequestData
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetICADependencies(
        NodeProcessRequestData*  pNodeRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsStabilizationTypeEIS
    ///
    /// @brief  Helper method to check if CVP node stabilization type is EIS2 or EIS3
    ///
    /// @return True if CVP node stabilization type is EIS2 or EIS3, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsStabilizationTypeEIS() const
    {
        BOOL bStabilizationTypeEIS = FALSE;

        if ((0 != (IPEStabilizationType::IPEStabilizationTypeEIS2 & m_instanceProperty.stabilizationType)) ||
            (0 != (IPEStabilizationType::IPEStabilizationTypeEIS3 & m_instanceProperty.stabilizationType)))
        {
            bStabilizationTypeEIS = TRUE;
        }

        return bStabilizationTypeEIS;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsStabilizationTypeSAT
    ///
    /// @brief  Helper method to check if CVP node stabilization type is SAT
    ///
    /// @return True if CVP node stabilization type is SAT, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsStabilizationTypeSAT() const
    {
        BOOL bStabilizationTypeSAT = FALSE;

        if (0 != (IPEStabilizationType::IPEStabilizationTypeSAT & m_instanceProperty.stabilizationType))
        {
            bStabilizationTypeSAT = TRUE;
        }

        return bStabilizationTypeSAT;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckForNonEISLDCGridDependency
    ///
    /// @brief  Helper method to determine if need to check non EIS LDC Grid dependency
    ///
    /// @return TRUE if ipe instance can have non EIS LDC Grid dependency, FALSE otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL CheckForNonEISLDCGridDependency() const
    {
        BOOL                  bCheckForNonEISLDCGridDependency = FALSE;
        const StaticSettings* pStaticSettings                  = HwEnvironment::GetInstance()->GetStaticSettings();

        if ((CSLCameraTitanVersion::CSLTitan480 == static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion()) &&
            (TRUE == pStaticSettings->enableLDC))
        {
            if ((TRUE == IsCVPWithICAProfile()) ||
                (TRUE == IsCVPInPrefilter()))
            {
                bCheckForNonEISLDCGridDependency = TRUE;
            }
        }

        return bCheckForNonEISLDCGridDependency;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetPortLinkICATransformDependency
    ///
    /// @brief  Set Port link metadata based ICA perspective and grid transform dependency
    ///
    /// @param  pNodeRequestData          Pointer to Node request data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE VOID SetPortLinkICATransformDependency(
        NodeProcessRequestData* pNodeRequestData)
    {
        const Titan17xStaticSettings* pTitan17xStaticSettings = static_cast<Titan17xContext*>(
            GetHwContext())->GetTitan17xSettingsManager()->GetTitan17xStaticSettings();

        if (TRUE == IsStabilizationTypeSAT())
        {
            // set SAT In perspective matrices port link metadata dependency
            SetPSMetadataDependency(m_CVPICATAGLocation[0], CVPInputPortTARIFEDS4, 0, pNodeRequestData);
        }
        else if (TRUE == IsStabilizationTypeEIS())
        {
            // set EIS In perspective matrices port link metadata dependency
            SetPSMetadataDependency(m_CVPICATAGLocation[0], CVPInputPortTARIFEDS4, 0, pNodeRequestData);

            if (TRUE == pTitan17xStaticSettings->enableICAInGrid)
            {
                // Set  EIS ldc grid out2in as port link metadata dependency
                SetPSMetadataDependency(m_CVPICATAGLocation[1], CVPInputPortTARIFEDS4, 0, pNodeRequestData);
            }
        }
        else if ((TRUE ==  CheckForNonEISLDCGridDependency()) &&
            (TRUE == pTitan17xStaticSettings->enableICAInGrid))
        {
            // Set ldc grid out2in as port link metadata dependency
            SetPSMetadataDependency(m_CVPICATAGLocation[1], CVPInputPortTARIFEDS4, 0, pNodeRequestData);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// UpdateICADependencies
    ///
    /// @brief  Update the ICA Input Data
    ///
    /// @param  pInputData                 moduleInput data
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateICADependencies(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PostICATransform
    ///
    /// @brief     This method publishes the ICA transform in metadata
    ///
    /// @param     pTransform               The transform from nclib
    /// @param     confidence               Transform confidence
    /// @param     width                    Width
    /// @param     height                   Height
    /// @param     requestId                RequestId
    ///
    /// @return    CamxResultSuccess on success else failure code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult PostICATransform(
        VOID* pTransform,
        UINT32 confidence,
        UINT32 width,
        UINT32 height,
        UINT64 requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureCVPConfidenceParameter
    ///
    /// @brief     This method configures the transform confidence and check if force Identity transform is enabled
    ///
    /// @param     pTransformConfidence      Transform confidence
    /// @param     requestID                 RequestId
    ///
    /// @return    CamxResultSuccess on success else failure code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureCVPConfidenceParameter(
        UINT32* pTransformConfidence,
        UINT64  requestID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeleteBufferManager
    ///
    /// @brief  delete  Buffer Manager
    ///
    /// @param  pBufferManager   The buffer manager
    /// @param  ppBuffers        The buffers allocated for the buffer manager
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DeleteBufferManager(
    ImageBufferManager* pBufferManager,
    ImageBuffer**       ppBuffers);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateBufferManager
    ///
    /// @brief  Create Buffer Managers
    ///
    /// @param  ppBufferManager   The buffer manager
    /// @param  format            Format information for the buffers
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateBufferManager(
        ImageBufferManager** ppBufferManager,
        ImageFormat         format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateBuffers
    ///
    /// @brief  Allocate Buffer
    ///
    /// @param  pBufferManager   The buffer manager
    /// @param  ppBuffers        The buffers allocated for the buffer manager
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocateBuffers(
        ImageBufferManager* pBufferManager,
        ImageBuffer**       ppBuffers);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProcessingNodeFenceCallback
    ///
    /// @brief  Fence notify mechanism to the derived nodes in case they need to know when a output ports buffer is available
    ///
    /// @param  pFenceHandlerData       Pointer to struct FenceHandlerData that belongs to this node
    /// @param  requestId               Request Id whose fence has come back
    /// @param  portId                  Port Id whose fence has come back
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID ProcessingNodeFenceCallback(
        NodeFenceHandlerData* pFenceHandlerData,
        UINT64 requestId,
        UINT   portId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFPSAndBatchSize
    ///
    /// @brief  Get FPS and batch Size
    ///
    /// @return CamxResultSuccess if successful.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFPSAndBatchSize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// PrintMatrix
    ///
    /// @brief     This util method prints the CPerspectiveTransform matrix
    ///
    /// @param     matrix           The transform from nclib
    /// @param     confidence       Transform confidence
    /// @param     requestId        RequestId
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID PrintMatrix(
        CPerspectiveTransform   matrix,
        UINT32                  confidence,
        UINT64                  requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateCVPIQModules
    ///
    /// @brief  Create IQ Modules of the CVP Block
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateCVPIQModules();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsIQModuleInstalled
    ///
    /// @brief  Check if the given module is installed and applicable to the current HW support
    ///
    /// @param  pIQModuleInfo   the module info to check
    ///
    /// @return TRUE if the given module is installed and HW supported, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsIQModuleInstalled(
        const IPEIQModuleInfo* pIQModuleInfo) const
    {
        return (pIQModuleInfo->installed);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateICAModule
    ///
    /// @brief  Create ICA input path IQ Module of the CVP Block
    ///
    /// @param  pModuleInputData   the module input data pointer for IQ module
    /// @param  pIQModule          the module info pointer to create IQ module
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE CamxResult CreateICAModule(
        IPEModuleCreateData* pModuleInputData,
        IPEIQModuleInfo*     pIQModule)
    {
        CamxResult result = CamxResultSuccess;

        pModuleInputData->path = IPEPath::CVPICA;
        // ICA1 is first in the IPE hardware IQ modules order i.e. 0th index in the list
        if (TRUE == IsIQModuleInstalled(&pIQModule[0]))
        {
            result = pIQModule[0].IQCreate(pModuleInputData);
            if (CamxResultSuccess == result)
            {
                m_pEnabledCVPIQModule[m_numCVPIQModulesEnabled] = pModuleInputData->pModule;
                m_numCVPIQModulesEnabled++;
            }
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupCVP, "IPE:%d ICA input module is disabled, should be always enabled", InstanceID());
        }

        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateIQModulesCmdBufferManager
    ///
    /// @brief  Create IQ Modules Cmd Buffer Manager
    ///
    /// @param  pModuleInputData        module input data
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateIQModulesCmdBufferManager(
        IPEModuleCreateData*     pModuleInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ProgramIQConfig
    ///
    /// @brief  Reprogram the settings for the IQ Modules
    ///
    /// @param  pInputData Pointer to the input data
    ///
    /// @return CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProgramIQConfig(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetAAAInputData
    ///
    /// @brief  Hook up the AEC/AWB settings for the IQ Modules
    ///
    /// @param  pInputData    Pointer to the ISP input data for AAA setting updates
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetAAAInputData(
        ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetFaceROI
    ///
    /// @brief  Returns FD ROI information
    ///
    /// @param  pInputData       Pointer to the input data
    /// @param  parentNodeID     Parent node id
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetFaceROI(
        ISPInputData* pInputData,
        UINT          parentNodeID);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsFDEnabled
    ///
    /// @brief  Determine if FD is enabled
    ///
    ///
    /// @return return enabled/disabled
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsFDEnabled();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // UpdateTuningModeData
    ///
    /// @brief  Update tuning mode data
    ///
    /// @param  pTuningModeData    tining mode data
    /// @param  pModuleInput       ISP module input
    ///
    /// @return return CamxResult
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult UpdateTuningModeData(
        ChiTuningModeParameter* pTuningModeData,
        ISPInputData*           pModuleInput);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // DumpICAFrameConfig
    ///
    /// @brief  Dump ICA Frame Config
    ///
    /// @param  pIcaFrameCfg    Pointer to ICA frame config structure
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpICAFrameConfig(
        cvpDmeFrameConfigIca* pIcaFrameCfg);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCVPInMCTF
    ///
    /// @brief  Helper method to check if CVP node is used for MCTF
    ///
    /// @return True if CVP node is getting used for MCTF, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsCVPInMCTF() const
    {
        BOOL bCVPInMCTF = FALSE;

        if ((CVPProfileIdDME == m_instanceProperty.profileId) ||
            (CVPProfileIdDMEwithICA == m_instanceProperty.profileId))
        {
            bCVPInMCTF = TRUE;
        }

        return bCVPInMCTF;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCVPWithICAProfile
    ///
    /// @brief  Helper method to check if CVP node has ICA enabled
    ///
    /// @return True if CVP node has ICA enabled, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsCVPWithICAProfile() const
    {
        BOOL bCVPWithICAProfile = FALSE;

        if ((CVPProfileIdDMEwithICA == m_instanceProperty.profileId) ||
            (CVPProfileIdICA == m_instanceProperty.profileId))
        {
            bCVPWithICAProfile = TRUE;
        }

        return bCVPWithICAProfile;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsProfileIdRegistration
    ///
    /// @brief  Helper method to check if CVP node profile id is registration
    ///
    /// @return True if CVP node profile id is registration, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsProfileIdRegistration() const
    {
        BOOL bProfileIdRegistration = FALSE;

        if (CVPProfileIdRegistration == m_instanceProperty.profileId)
        {
            bProfileIdRegistration = TRUE;
        }

        return bProfileIdRegistration;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCVPInPrefilter
    ///
    /// @brief  Helper method to check if CVP node is in prefilter pipeline or not
    ///
    /// @return True if CVP node is in prefilter pipeline, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsCVPInPrefilter() const
    {
        BOOL bCVPInPrefilter = FALSE;

        if ((TRUE == IsProfileIdRegistration())  &&
            (CVPProcessingTypePrefilter == m_instanceProperty.processingType))
        {
            bCVPInPrefilter = TRUE;
        }

        return bCVPInPrefilter;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCVPInBlend
    ///
    /// @brief  Helper method to check if CVP node is in blend pipeline or not
    ///
    /// @return True if CVP node is in blend pipeline, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsCVPInBlend() const
    {
        BOOL bCVPInBlend = FALSE;

        if ((TRUE == IsProfileIdRegistration())  &&
            (CVPProcessingTypeBlend == m_instanceProperty.processingType))
        {
            bCVPInBlend = TRUE;
        }

        return bCVPInBlend;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCtcPerspectiveEnabled
    ///
    /// @brief  Helper method to check if ctc perspective in ICA config is enabled or not
    ///
    /// @return True if ctc perspective is enabled in ICA config, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsCtcPerspectiveEnabled() const
    {
        BOOL bCtcPerspectiveEnabled = FALSE;

        if ((0 != m_instanceProperty.stabilizationType)  ||
            (TRUE == IsProfileIdRegistration()))
        {
            bCtcPerspectiveEnabled = TRUE;
        }

        return bCtcPerspectiveEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsCtcGridEnabled
    ///
    /// @brief  Helper method to check if ctc grid in ICA config is enabled or not
    ///
    /// @return True if ctc grid is enabled in ICA config, otherwise FALSE
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsCtcGridEnabled() const
    {
        BOOL bCtcGridEnabled = FALSE;

        if (TRUE == static_cast<Titan17xContext*>(
                        GetHwContext())->GetTitan17xSettingsManager()->GetTitan17xStaticSettings()->enableICAInGrid)
        {
            if ((0 != m_instanceProperty.stabilizationType) ||
                (TRUE == IsProfileIdRegistration()))
            {
                bCtcGridEnabled = TRUE;
            }
        }

        return bCtcGridEnabled;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // SetupICAPerspectiveForRegistration
    ///
    /// @brief  Setup ICA perspective       paramaters for registration case
    ///
    /// @param  pICAInPerspectiveParams     Pointer to ICA perspective config parameters
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupICAPerspectiveForRegistration(
        IPEICAPerspectiveTransform* pICAInPerspectiveParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpCVPICAInputConfig
    ///
    /// @brief  Dump function for CVP ICA input config
    ///
    /// @param  pInputData      moduleInput data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpCVPICAInputConfig(
        const ISPInputData* pInputData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpCVPICAInputPerspectiveParams
    ///
    /// @brief  Dump function for CVP ICA input perspective config
    ///
    /// @param  pOutputString               The pointer to the output char buffer
    /// @param  rDataCount                  The variable which indicate how much data we write to the output char buffer
    /// @param  outputBufferSize            The size of the output char buffer
    /// @param  pICAInPerspectiveParams     The pointer to the CVP ICA input perspective config
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpCVPICAInputPerspectiveParams(
        CHAR*                               pOutputString,
        UINT32&                             rDataCount,
        UINT32                              outputBufferSize,
        const IPEICAPerspectiveTransform*   pICAInPerspectiveParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpCVPICAInputGridParams
    ///
    /// @brief  Dump function for CVP ICA input perspective config
    ///
    /// @param  pOutputString               The pointer to the output char buffer
    /// @param  rDataCount                  The variable which indicate how much data we write to the output char buffer
    /// @param  outputBufferSize            The size of the output char buffer
    /// @param  pICAInGridParams            The pointer to the CVP ICA input grid config
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpCVPICAInputGridParams(
        CHAR*                               pOutputString,
        UINT32&                             rDataCount,
        UINT32                              outputBufferSize,
        const IPEICAGridTransform*          pICAInGridParams);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetGeoLibICAMapping
    ///
    /// @brief  Get geolib ICA mapping from vendor tags and cpoy to moduleInput
    ///
    /// @param  pCvpIcaMapping      Pointer to CVP ICA mapping structure
    /// @param  requestId           current request Id
    ///
    /// @return CamxResultSuccess if the we can the geolib ICA mapping from vendor tags
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetGeoLibICAMapping(
        GeoLibIcaMapping*   pCvpIcaMapping,
        UINT64              requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetCVPDimensions
    ///
    /// @brief  Set CVP dimensions ICA  output and ICA output scale to full dimension
    ///
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetCVPDimensions();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetGeoLibStillFrameConfigurationDependency
    ///
    /// @brief  This function set the dependency for the GeoLib frame configuration
    ///
    /// @param  pNodeRequestData    Pointer to Node request data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult SetGeoLibStillFrameConfigurationDependency(
        NodeProcessRequestData*  pNodeRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DumpGeolibResult
    ///
    /// @brief  This function dumps the geolib result
    ///
    /// @param  pStillFrameConfig   Pointer to geolib still frame config
    /// @param  requestId           current request Id
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DumpGeolibResult(
        GeoLibStillFrameConfig* pStillFrameConfig,
        UINT64                  requestId);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CreateSynxObject
    ///
    /// @brief  Function to create synx object
    ///
    /// @param  phSynxInputHandler  Pointer to the synx handler.
    /// @param  hInputFence         The CSL fence that will be bound to synx
    /// @param  pSynxKey            Pointer to the synx key.
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult CreateSynxObject(
        synx_handle_t*  phSynxInputHandler,
        CSLFence        hInputFence,
        UINT32*         pSynxKey);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseSynxObject
    ///
    /// @brief  Function to release synx object
    ///
    /// @param  requestIdIndex  current request Id mod RequestQueueDepth
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID ReleaseSynxObject(
        UINT32  requestIdIndex);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetStabilizationMargins
    ///
    /// @brief  Get the stabilization margins depending on the Image Stabilization type for this node
    ///
    /// @return CamxResultSuccess if success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult GetStabilizationMargins();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LoadCVPChromatix
    ///
    /// @brief  Load tuning chromatix data for CVP
    ///
    /// @param  pInputData Pointer to the input data
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LoadCVPChromatix(
        const ISPInputData* pInputData = NULL);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AllocateCommonLibraryData
    ///
    /// @brief  Allocate memory required for common library
    ///
    /// @return CamxResult success if the write operation is success
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AllocateCommonLibraryData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// DeallocateCommonLibraryData
    ///
    /// @brief  Deallocate memory required for common library
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID DeallocateCommonLibraryData();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetValuesForGeoLibIcaMapping
    ///
    /// @brief  Set Values for GeoLib ICA Mapping
    ///
    /// @param  pIcaMapping      ICA mapping pointer
    /// @param  widthInPixels    CVP input Width in pixels
    /// @param  heightInLines    CVP input Height in lines
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void SetValuesForGeoLibIcaMapping(
        GeoLibIcaMapping* pIcaMapping,
        UINT32 widthInPixels,
        UINT32 heightInLines);

    UINT                    m_numInputPorts;                                      ///< Number of input ports used by CVP
    UINT                    m_numOutputPorts;                                     ///< Number of output ports used by CVP
    UINT32                  m_inputWidth;                                         ///< Width of input path
    UINT32                  m_inputHeight;                                        ///< Height of input path
    UINT32                  m_fullInputWidth;                                     ///< Width of full input path
    UINT32                  m_fullInputHeight;                                    ///< Height of full input path
    UINT32                  m_scaledWidth;                                         ///< Width of full input path
    UINT32                  m_scaledHeight;                                       ///< Height of full input path
    UINT32                  m_ICAOutWidth;                                        ///< Width of full input path
    UINT32                  m_ICAOutHeight;                                       ///< Height of full input path
    ImageFormat             m_inputFormatInfo;                                    ///< Input path info
    CSLVersion              m_version;                                            ///< CVP Hardware Revision
    struct CvpCapability    m_capability;                                         ///< CVP capabilities
    UINT                    m_selectedTARPort;                                    ///< Input TAR port
    UINT                    m_selectedREFPort;                                    ///< Input REF port
    CSLDeviceHandle         m_hDevice;                                            ///< Device Handle from acquire
    cvpSession              m_hCVPSessionHandle;                                  ///< Session Handle from CVP
    cvpHandle               m_hCVPDME_Handle;                                     ///< DME Init Handle from CVP
    cvpDmeRefBuffReq        m_outMemReq;                                          ///< Output memory requirement
    cvpColorFormat          m_inputTARDS4Format;                                  ///< CVP color format for TAR input image
    cvpColorFormat          m_inputREFDS4Format;                                  ///< CVP color format for REF input image
    ImageBufferManager*     m_pGridBufferManager;                                 ///< Pointer to grid buffer manager
    ImageBuffer**           m_ppGridBuffers;                                      ///< Array of output buffers
    ImageBufferManager*     m_pSourceContextBufferManager;                        ///< Pointer to reference buffer manager
    ImageBuffer**           m_ppSourceContextBuffers;                             ///< Array of output buffers
    synx_handle_t*          m_phSynxSrcImage;                                     ///< Array of output buffers
    synx_handle_t*          m_phSynxDmeOutput;                                    ///< Array of output buffers
    synx_handle_t*          m_phSynxIcaOutput;                                    ///< Array of output buffers
    UINT                    m_FPS;                                                ///< FPS requested
    UINT                    m_maxBatchSize;                                       ///< Number of frames in a batch
    CVPInstanceProperty     m_instanceProperty;                                   ///< CVP Node Instance Property
    BOOL                    m_bICAEnabled;                                        ///< ICA enabled flag
    ISPIQModule*            m_pEnabledCVPIQModule[MaxCVPIQModule];                ///< List of IQ Modules
    UINT                    m_numCVPIQModulesEnabled;                             ///< Number of IQ Modules
    INT32                   m_deviceIndex;                                        ///< ICP device index
    UINT                    m_parentNodeID;                                       ///< Parent node id
    TuningDataManager*      m_pPreTuningDataManager;                              ///< Store previoustuningDataManager
                                                                                  /// to compare if change
    ChiTuningModeParameter  m_tuningData;                                         ///< Tuning data
    UINT32                  m_cameraId;                                           ///< Current camera id
    BOOL                    m_camIdChanged;                                       ///< Flag to check if camera ID has changed
    CVPDmeCbPrivData        m_cvpDmeCbPrivData;                                   ///< Dme callback private data
    UINT                    m_CVPICATAGLocation[CVPProperties];                   ///< ICA Tags
    UINT32                  m_prefilterSrcContextIndex;                           ///< Index for prefilter to get src context
    UINT32                  m_previousRequestIdIndex;                             ///< Previous request id index in blend stage
    BOOL                    m_dumpCVPICAInputConfig;                              ///< Flag to enable CVP ICA input config dump

    // Added for Geolib
    BOOL                    m_dumpGeolibResult;                                   ///< flag to enable the GeoLib dump
    UINT32                  m_stillFrameConfigurationTagIdPrefilter;              ///< Tag Location for prefilter
    UINT32                  m_stillFrameConfigurationTagIdBlending;               ///< Tag Location for blending
    StabilizationMargin     m_stabilizationMargin;                                ///< ICA margin Dimensions
    GeoLibStillFrameConfig* m_pStillFrameConfig;                                  ///< still frame config

    CVP10InputData                          m_dependenceData;                     ///< Dependence Data for this Module
    CVP10OutputData*                        m_pOutputData;                        ///< Pointer to the Output Data
    cvp_1_0_0::chromatix_cvp10_reserveType* m_pReserveData;                       ///< Pointer to CVP reserve data
    cvp_1_0_0::cvp10_rgn_dataType*          m_pRgnDataType;                       ///< Pointer to CVP region data type
    cvp_1_0_0::chromatix_cvp10Type*         m_pChromatix;                         ///< Pointer to CVP chromatix
    UINT32                                  m_requestQueueDepth;                  ///< request queue depth
    FLOAT                                   m_scaleFactorW;                       ///< scale factor W
    FLOAT                                   m_scaleFactorH;                       ///< scale factor H
    FLOAT                                   m_coarseCenterW;                      ///< coarse center W
    FLOAT                                   m_coarseCenterH;                      ///< coarse enter H
    BOOL                                    m_fTexture;                           ///< Ftexture flag
    UINT8*                                  m_pForceIdentityTransform;            ///< Identity transform forced state
    BOOL                                    m_disableCVPDriver;                   ///< Flag from camxsettings to disable CVP HW
    BOOL                                    m_unityTransformEnabled;              ///< Flag to force unity transform from CVP
};

CAMX_NAMESPACE_END

#endif // CAMXCVPNODE_H
