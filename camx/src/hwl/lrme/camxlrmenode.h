////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxlrmenode.h
/// @brief LRMENode class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXLRMENODE_H
#define CAMXLRMENODE_H

#include "camxmem.h"
#include "camxnode.h"
#include "camxcmdbuffermanager.h"
#include "camxhwcontext.h"
#include "camxcsllrmedefs.h"

CAMX_NAMESPACE_BEGIN

/// @brief LRME buffer managers index
enum lrmeCmdBufferManagers
{
    lrmeCmdBufferManager = 0,    ///< Command buffer manager index
    lrmePacketManager,           ///< Packet manager index
    lrmeBufferManagersMax        ///< Max buffer managers
};

/// @brief LRME node capabilities
struct LrmeCapability
{
    UINT   minInputWidth;          ///< Min input width that LRME can take
    UINT   minInputHeight;         ///< Min input height that LRME can take
    UINT   maxInputWidth;          ///< Max input width that LRME can take
    UINT   maxInputHeight;         ///< Max input height that LRME can take
    UINT   optimalInputWidth;      ///< Max input width that LRME can take
    UINT   optimalInputHeight;     ///< Max input height that LRME can take
    UINT   searchAreaRangex;       ///< The range for vector calculation search
    UINT   searchAreaRangey;       ///< The range for vector calculation search
    UINT   stepx;                  ///< The step for each block in vector calculation search
    UINT   stepy;                  ///< The step for each block in vector calculation search
    UINT   maxWidthDownscale;      ///< The width downscale that LRME can support
    UINT   maxHeightDownscale;     ///< The height downscale that LRME can support
    UINT   maxWidthCrop;           ///< The width crop that LRME can support
    UINT   maxHeightCrop;          ///< The height crop that LRME can support
    UINT32 formats;                ///< Formats supported by LRME
};

/// @brief LRME register list progtammed by this node
enum lrmeCLCRegistersList
{
    lrmeCLCModuleConfig = 0,                  ///< module config register
    lrmeCLCModuleFormat,                      ///< module format register
    lrmeCLCModuleRangeStep,                   ///< range step register
    lrmeCLCModuleOffset,                      ///< module offset register
    lrmeCLCModuleMaxAllowedSad,               ///< max allowed sad register
    lrmeCLCModuleMinAllowedTarMad,            ///< tar mad register
    lrmeCLCModuleMeaningfulSaDDiff,           ///< Algo Specific register
    lrmeCLCModulMinSaDDiffDenom,              ///< Algo Specific register
    lrmeCLCModuleRobustnessMeasureDistMap0,   ///< Algo Specific register
    lrmeCLCModuleRobustnessMeasureDistMap1,   ///< Algo Specific register
    lrmeCLCModuleRobustnessMeasureDistMap2,   ///< Algo Specific register
    lrmeCLCModuleRobustnessMeasureDistMap3,   ///< Algo Specific register
    lrmeCLCModuleRobustnessMeasureDistMap4,   ///< Algo Specific register
    lrmeCLCModuleRobustnessMeasureDistMap5,   ///< Algo Specific register
    lrmeCLCModuleRobustnessMeasureDistMap6,   ///< Algo Specific register
    lrmeCLCModuleRobustnessMeasureDistMap7,   ///< Algo Specific register
    lrmeCLCModuleDsCropHorizontal,            ///< Crop of tar input
    lrmeCLCModuleDsCropVertical,              ///< Crop of tar input
    lrmeCLCModuleTarPdUnpacker,               ///< Crop of tar input
    lrmeCLCModuleRefPdUnpacker,               ///< Crop of tar input
    lrmeCLCModuleSwOverride,                  ///< Chicken bits
    lrmeCLCModuleTarHeight,                   ///< tar input height
    lrmeCLCModuleRefHeight,                   ///< ref input height
    lrmeRegisterListMax                       ///< Max registers count
};

static const CHAR* LRMEPortName[] =
{
    "Vector",       // 0 - Output
    "DS2",          // 1 - Output
    "TARIFEFull",   // 2 - Input
    "REFIFEFull",   // 3 - Input
    "TARIFEDS4",    // 4 - Input
    "REFIFEDS4",    // 5 - Input
    "TARIFEDS16",   // 6 - Input
    "REFIFEDS16",   // 7 - Input
    "Unknown",      // 8 - Unknown
};

static const UINT LRMEPortNameMaxIndex = CAMX_ARRAY_SIZE(LRMEPortName) - 1; ///< Number of strings in LRMEPortName array

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the LRME node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class LRMENode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief     Static method to create LRMENode Object.
    ///
    /// @param     pCreateInputData  Node create input data
    /// @param     pCreateOutputData Node create output data
    ///
    /// @return    Pointer to the concrete LRMENode object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static LRMENode* Create(
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

protected:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LRMENode
    ///
    /// @brief     Constructor to initialize LRME node instance constants
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    LRMENode();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~LRMENode
    ///
    /// @brief     Destructor
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~LRMENode();

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
    /// @brief  Virtual method implemented by LRME node to determine its input buffer requirements based on all the output
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
        if (portId > LRMEPortNameMaxIndex)
        {
            portId = LRMEPortNameMaxIndex;
        }

        return LRMEPortName[portId];
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
        if (portId > LRMEPortNameMaxIndex)
        {
            portId = LRMEPortNameMaxIndex;
        }

        return LRMEPortName[portId];
    }

private:
    LRMENode(const LRMENode&) = delete;                 ///< Disallow the copy constructor.
    LRMENode& operator=(const LRMENode&) = delete;      ///< Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Cleanup
    ///
    /// @brief     This method cleans up any resources allocated by node
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID Cleanup();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseDevice
    ///
    /// @brief     Helper method to release BPS device
    ///
    /// @return    CamxResultSuccess
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeRegisters
    ///
    /// @brief     This method sets up LRME register addresses and default values
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID InitializeRegisters();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupLRMEFormats
    ///
    /// @brief     This method sets up LRME formats that are supported by the h/w
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupLRMEFormats();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureLRMECapability
    ///
    /// @brief     This method sets up LRME node capabilities
    ///
    /// @return    CamxResultSuccess if the capabilities were setup
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureLRMECapability();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCSLLRMEFormat
    ///
    /// @brief     This method returns the csl format for a given camx format
    ///
    /// @param     format    The camx format
    ///
    /// @return    The csl format value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    UINT32 GetCSLLRMEFormat(
        CamX::Format fmt);

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
    /// FillCmdBufferArray
    ///
    /// @brief     Fills command buffer with register values
    ///
    /// @param     pCmdBuffer    The buffer where commands are to be filled
    ///
    /// @return    CamxResultSuccess if commands were written into buffer
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult FillCmdBufferArray(
        CmdBuffer* pCmdBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillHwFormat
    ///
    /// @brief     Fills in format information in LRME register
    ///
    /// @param     port    The IO port on which buffer is used
    /// @param     format  The format of the buffer
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillHwFormat(
        enum CSLLRMEIO port,
        UINT32 format);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FillImageResolution
    ///
    /// @brief     Fills in format information in LRME register
    ///
    /// @param     port    The IO port on which buffer is used
    /// @param     pFormat The format of the buffer
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID FillImageResolution(
        enum CSLLRMEIO port,
        const ImageFormat* pFormat);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// LRMEPostFrameSettings
    ///
    /// @brief     Posts LRME meta data to be used by ransac
    ///
    /// @param     isRefValid If ref post valid or not
    ///
    /// @return    CamxResultSuccess on success else error code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult LRMEPostFrameSettings(
        INT isRefValid);

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
    /// EnableDS2OutputPath
    ///
    /// @brief     Enable/disable Ds2 output
    ///
    /// @param     en Indicates if ds2 output should be enabled or not
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID EnableDS2OutputPath(
        BOOL en);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// EnableRefPort
    ///
    /// @brief     Enable/disable ref input
    ///
    /// @param     en Indicates if ref input should be enabled or not
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID EnableRefPort(
        BOOL en);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetupDS2OutputFinalResolution
    ///
    /// @brief     Set up resolution of ds2 out port
    ///
    /// @param     pFinalOutputBufferProperties Pointer to ds2 out port format
    /// @param     width                        Width of ds2 out
    /// @param     height                       Height of ds2 out
    ///
    /// @return    None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetupDS2OutputFinalResolution(
         BufferProperties* pFinalOutputBufferProperties,
         UINT width,
         UINT height);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertCSLLRMEToLRMEHwFormat
    ///
    /// @brief     Convert LRME format to the LRME hw format
    ///
    /// @param     format    The LRME format to convert
    ///
    /// @return    Converted format
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 ConvertCSLLRMEToLRMEHwFormat(
        UINT32 format)
    {
        UINT32 result = 0;

        switch (format)
        {
            case CSLLRMEFormatY8:
            case CSLLRMEFormatNV12:
                result = LRME_FORMAT_Y_ONLY_8BPS;
                break;
            case CSLLRMEFormatY10:
                result = LRME_FORMAT_Y_ONLY_10BPS;
                break;
            case CSLLRMEFormatPD10:
                result = LRME_FORMAT_LINEAR_PD10;
                break;
            case CSLLRMEFormatPD8:
                result = LRME_FORMAT_LINEAR_PD8;
                break;
            default:
                result = LRME_FORMAT_INVALID;
                break;
        }
        return result;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConvertCamXToCSLLRMEFormat
    ///
    /// @brief     This method returns the csl format for a given camx format
    ///
    /// @param     format    The camx format
    ///
    /// @return    The csl format value
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE UINT32 ConvertCamXToCSLLRMEFormat(
        const CamX::Format fmt)
    {
        UINT32 result = 0;
        switch (fmt)
        {
            case CamX::Format::Y8:
                result = CSLLRMEFormatY8;
                break;
            case CamX::Format::PD10:
                result = CSLLRMEFormatPD10;
                break;
            case CamX::Format::YUV420NV12:
                result = CSLLRMEFormatNV12;
                break;
            case CamX::Format::P010:
                result = CSLLRMEFormatY10;
                break;
            default:
                break;
        }
        return result;
    }

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
    /// NotifyRequestProcessingError
    ///
    /// @brief  Function that does any error processing required for a fence callback. When CSL notifies the Node of a fence we
    ///         post a job in the threadpool. And when threadpool calls us to process the error fence-signal notification, this
    ///         function does the necessary processing.
    ///
    /// @param  pFenceHandlerData     Pointer to struct FenceHandlerData that belongs to this node
    /// @param  unSignaledFenceCount  Number of un-signalled fence count
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual VOID NotifyRequestProcessingError(
        NodeFenceHandlerData* pFenceHandlerData,
        UINT unSignaledFenceCount);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSkipAlternateLRMEProcessing
    ///
    /// @brief   Checks the if to alternate LRME request to be skipped based on Limitations
    ///
    /// @return  flag is set if LRME alternate request to be skipped
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL IsSkipAlternateLRMEProcessing();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsSkipRequest
    ///
    /// @brief   Checks if request has to be skipped
    ///
    /// @return  flag is set if request is to skipped
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CAMX_INLINE BOOL IsSkipRequest()
    {
        BOOL isSkipReq = (TRUE == m_alternateSkipProcessing) && (FALSE == IsPreviewPresent()) ? TRUE: FALSE;
        return isSkipReq;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SkipandSignalLRMEfences
    ///
    /// @brief   Skips LRME processing and Signals Fences for LRME
    ///
    /// @param   pNodeRequestData  Pointer to struct NodeProcessRequestData that belongs to this node
    /// @param   pPerRequestPorts  Pointer to struct PerRequestActivePorts that belongs to this node
    /// @param   requestId         Request to process
    /// @param   isRefValid        If ref post valid or not
    ///
    /// @return  CamxResultSuccess on success else error code
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL SkipandSignalLRMEfences(
        NodeProcessRequestData*    pNodeRequestData,
        PerRequestActivePorts*     pPerRequestPorts,
        UINT64                     requestId,
        INT                        isRefValid);

    UINT                  m_numInputPorts;                             ///< Number of input ports used by LRME
    UINT                  m_numOutputPorts;                            ///< Number of output ports used by LRME
    UINT                  m_cmdBufferSize;                             ///< The size of command buffer for programming h/w reg
    UINT                  m_numCmdBuffer;                              ///< Number of command buffers used by the node
    UINT                  m_numPacketBuffer;                           ///< Number of packet buffers used by the node
    CmdBufferManager*     m_pLRMECmdBufferManager[2];                  ///< Array of command buffer managers
    UINT32                m_lrmeRegistersValue[lrmeRegisterListMax];   ///< The lrme register values controlled by the node
    UINT32                m_lrmeRegistersAddress[lrmeRegisterListMax]; ///< The lrme register values controlled by the node
    UINT32                m_state;                                     ///< The state of this LRME node
    UINT                  m_deviceIndex;                               ///< The device index
    CSLVersion            m_version;                                   ///< LRME Hardware Revision
    struct LrmeCapability m_capability;                                ///< LRME capabilities
    UINT                  m_ds2Enable;                                 ///< Indicates if DS2 is enabled
    UINT                  m_selectedTARPort;                           ///< Input TAR port
    UINT                  m_selectedREFPort;                           ///< Input REF port
    CSLDeviceHandle       m_hDevice;                                   ///< Device Handle from acquire
    UINT                  m_numRegisters;                              ///< Number of LRME regs being programmed by this node
    CSLLRMEVectorFormat   m_lrmeVectorFormat;                          ///< LRME vector format
    BOOL                  m_lrmeDS2Connected;                          ///< LRME DS2 is connected or not
    BOOL                  m_resetReferenceInput;                       ///< Reset reference input ports
    INT32                 m_fullInputWidth;                            ///< Width of full input path
    INT32                 m_fullInputHeight;                           ///< Height of full input path
    BOOL                  m_alternateSkipProcessing;                   ///< Flag to skip the LRME Processing
};

CAMX_NAMESPACE_END

#endif // CAMXLRMENODE_H
