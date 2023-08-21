////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxfdhwnode.h
/// @brief FDHwNode class declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef CAMXFDHWNODE_H
#define CAMXFDHWNODE_H

#include "camxcslfddefs.h"
#include "camxfdhwutils.h"
#include "camxnode.h"
#include "camxtitan17xdefs.h"

CAMX_NAMESPACE_BEGIN

struct FDRegVal
{
    UINT32  reg;    ///< Register offset
    UINT32  val;    ///< Register value
};

static const CHAR* FDInputPortName[] =
{
    "Image",    // 0
    "Unknown",  // 1 Unknown
};

static const CHAR* FDOutputPortName[] =
{
    "Results",  // 0
    "Raw",      // 1
    "Pyramid",  // 2
    "Unknown",  // 3 Unknown
};

static const UINT FDInputPortNameMaxIndex  = CAMX_ARRAY_SIZE(FDInputPortName) - 1;  ///< Number of strings in
                                                                                    ///  FDInputPortName array
static const UINT FDOutputPortNameMaxIndex = CAMX_ARRAY_SIZE(FDOutputPortName) - 1; ///< Number of strings in
                                                                                    ///  FDOutputPortName array

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class that implements the FDHw node class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class FDHwNode final : public Node
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Create
    ///
    /// @brief  Static method to create FDHwNode Object.
    ///
    /// @param  pCreateInputData  Node create input data
    /// @param  pCreateOutputData Node create output data
    ///
    /// @return Pointer to the concrete FD Node object
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static FDHwNode* Create(
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
    /// @brief  Initialize the hwl object
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
    /// SignalFDOutputFence
    ///
    /// @brief  Pure virtual method to signal the FD output fence in failure case or frame skip case
    ///
    /// @param  pPerRequestPorts port data
    ///
    /// @return CamxResultSuccess if successful and 0 dependencies, dependency information otherwise
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual CamxResult SignalFDOutputFence(
        PerRequestActivePorts* pPerRequestPorts);

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
    CamxResult ProcessingNodeFinalizeInputRequirement(
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
        if (portId > FDInputPortNameMaxIndex)
        {
            portId = FDInputPortNameMaxIndex;
        }

        return FDInputPortName[portId];
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
        if (portId > FDOutputPortNameMaxIndex)
        {
            portId = FDOutputPortNameMaxIndex;
        }

        return FDOutputPortName[portId];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~FDHwNode
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~FDHwNode();

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FDHwNode
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    FDHwNode();
    FDHwNode(const FDHwNode&) = delete;             ///< Disallow the copy constructor.
    FDHwNode& operator=(const FDHwNode&) = delete;  ///< Disallow assignment operator.

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// AcquireDevice
    ///
    /// @brief  Helper method to acquire FD device
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult AcquireDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ReleaseDevice
    ///
    /// @brief  Helper method to release FD device
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ReleaseDevice();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SetDependencies
    ///
    /// @brief  Set dependencies for this request
    ///
    /// @param  pExecuteProcessRequestData Process request data
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    VOID SetDependencies(
        ExecuteProcessRequestData*  pExecuteProcessRequestData);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ConfigureFDHwCapability
    ///
    /// @brief  Set up FDHw capability based on FD Hw revision number
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ConfigureFDHwCapability();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ProgramFDGenericConfig
    ///
    /// @brief  Program FD Generic configuration
    ///
    /// @param  pGenericCmdBuffer   Generic Blob cmd buffer pointer to be used for filling FD blobs
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProgramFDGenericConfig(
        CmdBuffer*  pGenericCmdBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // ProgramFDCDMConfig
    ///
    /// @brief  Program FD CDM configuration
    ///
    /// @param  pCDM    CDM cmd buffer pointer to be used for filling CDM commands
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult ProgramFDCDMConfig(
        CmdBuffer*  pCDM);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // DetermineProcessFrame
    ///
    /// @brief  Determine whether to process this frame and handle HW power on/off based on enable FD mode.
    ///
    /// @param  pFrameSettings  Frame settings info
    /// @param  pProcessFrame   Pointer to indicate whether to process this frame request or not
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult DetermineProcessFrame(
        FDPropertyFrameSettings*    pFrameSettings,
        BOOL*                       pProcessFrame);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // HandleProcessFrame
    ///
    /// @brief  Handle Execute process request for this request id
    ///
    /// @param  pExecuteProcessRequestData Process request data
    /// @param  pFrameSettings             Frame settings info
    ///
    /// @return CamxResultSuccess if successful
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    CamxResult HandleProcessFrame(
        ExecuteProcessRequestData*  pExecuteProcessRequestData,
        FDPropertyFrameSettings*    pFrameSettings);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// CheckFDConfigChange
    ///
    /// @brief  Check and load if there is a change in FD configuration
    ///
    /// @param  pFDConfig   Pointer to load FD confiugration
    ///
    /// @return TRUE if config has been updated.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    BOOL CheckFDConfigChange(
        FDConfig* pFDConfig);


    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GetCorrespondingInputPortForOutputPort
    ///
    /// @brief  method to get the corresponding Input Port for the output port.
    ///
    /// @param  outputPortId to check
    /// @param  pInputPortId to update
    ///
    /// @return True if the Input port matched the corresponding output port
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL GetCorrespondingInputPortForOutputPort(
        UINT outputPortId,
        UINT* pInputPortId
    ) const
    {
        CAMX_ASSERT(outputPortId <= CSLFDOutputPortIdWorkBuffer);

        BOOL dumpInputPort = FALSE;

        if (NULL != pInputPortId)
        {
            switch (outputPortId)
            {
                case CSLFDOutputPortIdResults:
                    *pInputPortId = CSLFDInputPortIdImage;
                    dumpInputPort = TRUE;
                    break;
                default:
                    break;
            }
        }
        return dumpInputPort;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// IsDumpInputPortBuffer
    ///
    /// @brief  Check whether to dump all inport buffers.
    ///
    /// @param  outputPortId          to check
    /// @param  unSignaledFenceCount  Number of un-signaled fence count
    ///
    /// @return True if the Output port is Full out
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual BOOL IsDumpInputPortBuffer(
        UINT outputPortId,
        UINT unSignaledFenceCount
    ) const
    {
        CAMX_UNREFERENCED_PARAM(unSignaledFenceCount);

        BOOL result = FALSE;

        if (outputPortId <= FDHwOutputPortWorkBuffer)
        {
            result = ((FDHwOutputPortResults == outputPortId) ? TRUE : FALSE);
        }

        return result;
    }

    static const UINT MaxFDRegistersInCDM   = 20;                                           ///< Max number of FD registers
    static const UINT FDMaxBufferId         = CSLFDInputPortIdMax + CSLFDOutputPortIdMax;   ///<  Max Buffers

    CSLFDHWCaps         m_hwCaps;                                       ///< FD hw capabilities
    FDHwMajorVersion    m_FDHwVersion;                                  ///< FD Hw version
    CmdBufferManager*   m_pFDPacketManager;                             ///< FD Packet buffer manager
    CmdBufferManager*   m_pFDCmdBufferManager[CSLFDCmdBufferIdMax];     ///< Array Cmd buffer managers for FDHw node
    INT32               m_deviceIndex;                                  ///< FD device index
    CSLDeviceHandle     m_hDevice;                                      ///< FD device handle
    BOOL                m_hwPowerOn;                                    ///< Whether FD HW is powered on
    UINT32              m_FDFrameWidth;                                 ///< FD Processing frame width
    UINT32              m_FDFrameHeight;                                ///< FD Processing frame height
    UINT32              m_FDFrameStride;                                ///< FD Processing frame stride
    UINT32              m_FDFrameScanline;                              ///< FD Processing frame scanline
    CamxDimension       m_baseFDHWDimension;                            ///< Base FD HW dimension
    FDHwUtils           m_FDHwUtils;                                    ///< HW Util class
    FDConfig            m_FDConfig;                                     ///< Face detection tuning configuration
    BOOL                m_isFrontCamera;                                ///< Whether this instance is for front camera
    BOOL                m_isVideoMode;                                  ///< Whether current mode is video
    UINT                m_FDHwCmdPoolCount;                             ///< Number of blob command buffers in circulation
    BOOL                m_bDisableFDHW;                                 ///< Whether HW Detection is Disabled
};

CAMX_NAMESPACE_END

#endif // CAMXFDHWNODE_H
