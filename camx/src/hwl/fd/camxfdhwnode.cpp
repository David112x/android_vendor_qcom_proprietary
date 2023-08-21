////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxfdhwnode.cpp
/// @brief FDHw Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxmem.h"
#include "camxtrace.h"
#include "camxthreadmanager.h"
#include "camxhwenvironment.h"
#include "camxcdmdefs.h"
#include "camxcmdbuffermanager.h"
#include "camxcslresourcedefs.h"
#include "camxhwcontext.h"
#include "camximagebuffer.h"
#include "camximageformatutils.h"
#include "camxvendortags.h"
#include "camxpacketbuilder.h"
#include "camxtitan17xcontext.h"
#include "camxtitan17xdefs.h"
#include "titan170_base.h"
#include "titan170_fd_wrapper.h"
#include "camxfdproperty.h"
#include "camxfdhwnode.h"
#include "camxfdutils.h"
#include "camxpipeline.h"

CAMX_NAMESPACE_BEGIN

/// @brief This enum is used to index into node request dependency array to update any dependency not satisfied yet
enum NodeRequestDependencyIndex
{
    BufferDependencyIndex = 0,  ///< Index to be used for Buffer dependency
    PropertyDependencyIndex     ///< Index to be used for Property dependency
};

/// @brief This enum is used sequence execute process based on dependencies per request id
enum DependencySequence
{
    DependencySetNone          = 0,    ///< No dependencys set, default
    DependencySetFrameSettings = 1     ///< Input frame dependency set
};

static const UINT FDCmdBufferIdGenericSize  = 1024 * 2;     ///< Size of FD Generic Blob command buffer + KMD to use
static const UINT FDCmdBufferIdCDMSize      = 1024;         ///< Size of FD CDM command buffer

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::FDHwNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FDHwNode::FDHwNode()
{
    FDBaseResolutionType baseResolution = GetStaticSettings()->FDBaseResolution;

    m_pNodeName = "FDHw";

    m_baseFDHWDimension.width  = static_cast<UINT32>(baseResolution) & 0xFFFF;
    m_baseFDHWDimension.height = (static_cast<UINT32>(baseResolution) >> 16) & 0xFFFF;

    CAMX_LOG_VERBOSE(CamxLogGroupFD, "FD Base dimensions : %dx%d",
                     m_baseFDHWDimension.width,
                     m_baseFDHWDimension.height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::~FDHwNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FDHwNode::~FDHwNode()
{
    if (TRUE == IsDeviceAcquired())
    {
        ReleaseDevice();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FDHwNode* FDHwNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    return CAMX_NEW FDHwNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID FDHwNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);

    CamxResult      result                  = CamxResultSuccess;
    INT32           deviceIndex             = -1;
    UINT            indicesLengthRequired   = 0;

    CAMX_ASSERT(FDHw == Type());
    CAMX_ASSERT(NULL != pCreateOutputData);

    // CSLTitan150 is Talos
    if ((FDSWOnly == GetStaticSettings()->FDClient) ||
        (FDDLDSP == GetStaticSettings()->FDClient)  ||
        (CSLCameraTitanVersion::CSLTitan150 == static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion()) )
    {
        m_bDisableFDHW = TRUE;
    }
    else
    {
        m_bDisableFDHW = FALSE;
    }

    pCreateOutputData->maxOutputPorts = CSLFDOutputPortIdMax;
    pCreateOutputData->maxInputPorts  = CSLFDInputPortIdMax;

    // Add device indices
    result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeFD, &deviceIndex, 1, &indicesLengthRequired);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupFD, "Failed in GetDeviceIndices, result=%d", result);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupFD, "FDHw:%s, TitanVersion 0x%x, deviceIndex=%d result=%s m_bDisableFDHW=%d ",
                     NodeIdentifierString(),
                     static_cast<Titan17xContext *>(GetHwContext())->GetTitanVersion(),
                     deviceIndex,
                     Utils::CamxResultToString(result),
                     m_bDisableFDHW);

    if ((CamxResultSuccess == result) && (deviceIndex != -1))
    {
        CAMX_ASSERT(indicesLengthRequired == 1);

        result          = AddDeviceIndex(deviceIndex);
        m_deviceIndex   = deviceIndex;
    }

    // Configure FDHw Capability
    result = ConfigureFDHwCapability();

    /// @todo (CAMX-4173) No HW related operations to be perfromed when SW only enabled
    if (TRUE == m_bDisableFDHW)
    {
        // If only SW detection and no FD-HW present then GetDevice would fail
        result = CamxResultSuccess;
    }

    UINT32  groupID         = 1;
    UINT    numOutputPorts  = 0;
    UINT    outputPortId[MaxBufferComposite];

    GetAllOutputPortIds(&numOutputPorts, &outputPortId[0]);

    for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
    {
        pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] = groupID++;
    }

    pCreateOutputData->bufferComposite.hasCompositeMask = FALSE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDHwNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::PostPipelineCreate()
{
    CamxResult      result            = CamxResultSuccess;
    ResourceParams  params            = { 0 };
    UINT            cameraPosition    = 0;
    FDProcessingType fdtype           = InternalFDType;

    // IF no FD-HW acquire device fails, postpipeline should success if SW is enabled
    if (TRUE == m_bDisableFDHW)
    {
        result = CamxResultSuccess;
    }
    else
    {
        m_FDHwCmdPoolCount = GetPipeline()->GetRequestQueueDepth();

        // +1 to account for the packet itself
        result = InitializeCmdBufferManagerList(CSLFDCmdBufferIdMax + 1);

        if (CamxResultSuccess == result)
        {
            params.usageFlags.packet                = 1;
            params.packetParams.maxNumCmdBuffers    = CSLFDCmdBufferIdMax;
            params.packetParams.maxNumIOConfigs     = CSLFDInputPortIdMax + CSLFDOutputPortIdMax;
            params.packetParams.enableAddrPatching  = 0;
            params.packetParams.maxNumPatches       = 0;    // We do not expect any patching
            params.resourceSize                     = Packet::CalculatePacketSize(&params.packetParams);
            params.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
            params.pDeviceIndices                   = &m_deviceIndex;
            params.numDevices                       = 1;

            // Same number as cmd buffers
            params.poolSize     = m_FDHwCmdPoolCount * params.resourceSize;
            params.alignment    = CamxPacketAlignmentInBytes;

            result = CreateCmdBufferManager("PacketManager", &params, &m_pFDPacketManager);
            if (CamxResultSuccess == result)
            {
                params                              = { 0 };
                params.resourceSize                 = FDCmdBufferIdGenericSize;
                params.poolSize                     = m_FDHwCmdPoolCount * params.resourceSize;
                params.usageFlags.cmdBuffer         = 1;
                params.cmdParams.type               = CmdType::Generic;
                params.alignment                    = CamxCommandBufferAlignmentInBytes;
                params.cmdParams.enableAddrPatching = 0;
                params.cmdParams.maxNumNestedAddrs  = 0;
                params.memFlags                     = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
                params.pDeviceIndices               = &m_deviceIndex;
                params.numDevices                   = 1;

                result = CreateCmdBufferManager("CmdBufferIdGeneric", &params, &m_pFDCmdBufferManager[CSLFDCmdBufferIdGeneric]);
                if (CamxResultSuccess == result)
                {
                    params                              = { 0 };
                    params.resourceSize                 = FDCmdBufferIdCDMSize;
                    params.poolSize                     = m_FDHwCmdPoolCount * params.resourceSize;
                    params.usageFlags.cmdBuffer         = 1;
                    params.cmdParams.type               = CmdType::CDMDirect;
                    params.alignment                    = CamxCommandBufferAlignmentInBytes;
                    params.cmdParams.enableAddrPatching = 0;
                    params.cmdParams.maxNumNestedAddrs  = 0;
                    params.memFlags                     = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;
                    params.pDeviceIndices               = &m_deviceIndex;
                    params.numDevices                   = 1;

                    result = CreateCmdBufferManager("CmdBufferIdCDM", &params, &m_pFDCmdBufferManager[CSLFDCmdBufferIdCDM]);
                }
            }
            else
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Failed to Creat Cmd Buffer Manager");
            }
        }

        /// @todo (CAMX-3310) Get the correct flags
        m_isFrontCamera = FALSE;
        m_isVideoMode = FALSE;
        cameraPosition = GetPipeline()->GetCameraId();
        // Convert CameraPosition to CHISENSORPOSITIONTYPE
        cameraPosition += 1;

        if ((TRUE == GetStaticSettings()->useDifferentTuningForFrontCamera) && (FRONT == cameraPosition))
        {
            m_isFrontCamera = TRUE;
        }
        CAMX_LOG_VERBOSE(CamxLogGroupFD, "Is front camera %d", m_isFrontCamera);

        if (CamxResultSuccess == result)
        {
            FDConfigSelector configSelector = (TRUE == m_isVideoMode) ? FDSelectorVideo : FDSelectorDefault;

            if (FDConfigVendorTag != GetStaticSettings()->FDConfigSource)
            {
                result = FDUtils::GetFDConfig(this, GetStaticSettings()->FDConfigSource, TRUE,
                                             fdtype, m_isFrontCamera, configSelector, &m_FDConfig);
            }
            else
            {
                // Vendor tag wont be available by now. Read defualt config and save.
                result = FDUtils::GetFDConfig(this, FDConfigDefault, TRUE, fdtype, m_isFrontCamera,
                                             configSelector, &m_FDConfig);
            }
        }

        if (CamxResultSuccess == result)
        {
            result = AcquireDevice();

            if (CamxResultSuccess == result)
            {
                CSLCameraPlatform CSLPlatform  = {};

                result = CSLQueryCameraPlatform(&CSLPlatform);

                if (CamxResultSuccess == result)
                {
                    m_FDHwVersion = m_FDHwUtils.GetFDHwVersion(&CSLPlatform);

                    CAMX_LOG_VERBOSE(CamxLogGroupFD, "Platform version %d.%d.%d, cpas version %d.%d.%d, FDHw version %d",
                                     CSLPlatform.platformVersion.majorVersion, CSLPlatform.platformVersion.minorVersion,
                                     CSLPlatform.platformVersion.revVersion,   CSLPlatform.CPASVersion.majorVersion,
                                     CSLPlatform.CPASVersion.minorVersion,     CSLPlatform.CPASVersion.revVersion,
                                     m_FDHwVersion);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupFD, "Failed in Acquire device, result=%d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDHwNode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pBufferNegotiationData)
    {
        CAMX_LOG_ERROR(CamxLogGroupFD, "pBufferNegotiationData is NULL");

        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        UINT32          numInputPorts = 0;
        UINT32          inputPortId[CSLFDInputPortIdMax];
        CamxDimension   previewDimension = { 0 };

        // Get Input Port List
        GetAllInputPortIds(&numInputPorts, &inputPortId[0]);

        pBufferNegotiationData->numInputPorts = numInputPorts;

        if (TRUE == GetStaticSettings()->enableOfflineFD)
        {
            previewDimension.width  = 0;
            previewDimension.height = 0;
        }
        else
        {
            result = GetPreviewDimension(&previewDimension);
        }

        if ((CamxResultSuccess != result) || (0 == previewDimension.width) || (0 == previewDimension.height))
        {
            CAMX_LOG_WARN(CamxLogGroupFD,
                          "Failed to get preview dimension, result=%d, preview=%dx%d, Using Base dimensions %dx%d",
                          result, previewDimension.width, previewDimension.height,
                          m_baseFDHWDimension.width, m_baseFDHWDimension.height);

            previewDimension.width  = 0;
            previewDimension.height = 0;

            // If this happens for some reasons, FD frame aspect ratio wont match with preview aspect ratio.
            // Functionally, this wont cause any problem as we always publish w.r.t ActiveArray, but it could cause
            // sensor to select 4:3 mode as base dimensions are 4:3 - causes power consumption.
            // We might detect faces that are outside of preview FOV (say if preview is 16:9) - need to make sure
            //   we filter out such faces.
            // Overwrite result as we can fallback to use base dimensions as FDImage input dimensions
            result = CamxResultSuccess;
        }

        for (UINT input = 0; input < numInputPorts; input++)
        {
            UINT32 width  = m_baseFDHWDimension.width;
            UINT32 height = m_baseFDHWDimension.height;

            pBufferNegotiationData->inputBufferOptions[input].nodeId        = Type();
            pBufferNegotiationData->inputBufferOptions[input].instanceId    = InstanceID();
            pBufferNegotiationData->inputBufferOptions[input].portId        = inputPortId[input];

            BufferRequirement* pInputBufferRequirement =
                &pBufferNegotiationData->inputBufferOptions[input].bufferRequirement;

            if ((0 != previewDimension.width) && (0 != previewDimension.height))
            {
                CamxDimension frameDimension = m_baseFDHWDimension;

                Utils::MatchAspectRatio(&previewDimension, &frameDimension);

                width  = frameDimension.width;
                height = frameDimension.height;
            }

            width  = Utils::EvenFloorUINT32(width);
            height = Utils::EvenFloorUINT32(height);

            pInputBufferRequirement->optimalWidth   = width;
            pInputBufferRequirement->optimalHeight  = height;
            pInputBufferRequirement->minWidth       = width;
            pInputBufferRequirement->minHeight      = height;
            pInputBufferRequirement->maxWidth       = width;
            pInputBufferRequirement->maxHeight      = height;

            pInputBufferRequirement->planeAlignment[0].strideAlignment   =
                Utils::AlignGeneric32(m_baseFDHWDimension.width, 32);
            pInputBufferRequirement->planeAlignment[1].strideAlignment   =
                Utils::AlignGeneric32(m_baseFDHWDimension.width, 32);
            pInputBufferRequirement->planeAlignment[0].scanlineAlignment = m_baseFDHWDimension.height;
            pInputBufferRequirement->planeAlignment[1].scanlineAlignment = (m_baseFDHWDimension.height >> 1);

            CAMX_LOG_VERBOSE(CamxLogGroupFD,
                             "FD node Dimensions, Base[%dx%d], Preview [%dx%d], Request [%dx%d], planeAlignment %dx%d, %dx%d",
                             m_baseFDHWDimension.width, m_baseFDHWDimension.height,
                             previewDimension.width,    previewDimension.height,
                             width,                     height,
                             pInputBufferRequirement->planeAlignment[0].strideAlignment,
                             pInputBufferRequirement->planeAlignment[0].scanlineAlignment,
                             pInputBufferRequirement->planeAlignment[1].strideAlignment,
                             pInputBufferRequirement->planeAlignment[1].scanlineAlignment);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDHwNode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID FDHwNode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    UINT               numInputPort;
    UINT               inputPortId[CSLFDInputPortIdMax];
    const ImageFormat* pImageFormat = NULL;

    CAMX_ASSERT(NULL != pBufferNegotiationData);

    // Get Input Port List
    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    // Loop through input ports to get FD Input Image port
    for (UINT index = 0; index < numInputPort; index++)
    {
        if (pBufferNegotiationData->pInputPortNegotiationData[index].inputPortId == CSLFDInputPortIdImage)
        {
            pImageFormat = pBufferNegotiationData->pInputPortNegotiationData[index].pImageFormat;
            CAMX_ASSERT(NULL != pImageFormat);
            break;
        }
    }

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData   = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        UINT                       outputPortIndex              =
            pBufferNegotiationData->pOutputPortNegotiationData[index].outputPortIndex;
        BufferProperties*          pFinalOutputBufferProperties = pOutputPortNegotiationData->pFinalOutputBufferProperties;

        for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
        {
            BufferRequirement* pInputPortRequirement            =
                &pOutputPortNegotiationData->inputPortRequirement[inputIndex];

            pFinalOutputBufferProperties->imageFormat.width     = pInputPortRequirement->optimalWidth;
            pFinalOutputBufferProperties->imageFormat.height    = pInputPortRequirement->optimalHeight;
        }

        if ((CSLFDOutputPortIdResults == outputPortIndex) || (CSLFDOutputPortIdRawResults == outputPortIndex))
        {
            pFinalOutputBufferProperties->memFlags |= CSLMemFlagKMDAccess;
        }
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::DetermineProcessFrame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::DetermineProcessFrame(
    FDPropertyFrameSettings*    pFrameSettings,
    BOOL*                       pProcessFrame)
{
    CAMX_LOG_VERBOSE(CamxLogGroupFD, "Frame setting enable FD %d, HW skip frame %d, power on %d",
                     pFrameSettings->enableFD, pFrameSettings->hwSettings.skipProcess, m_hwPowerOn);

    /// @todo (CAMX-2764) Control HW clocks (stream-on/off)
    if ((TRUE == m_hwPowerOn) && (FALSE == pFrameSettings->enableFD))
    {
        // StreamOff FD HW
        m_hwPowerOn = FALSE;
    }
    else if ((FALSE == m_hwPowerOn) && (TRUE == pFrameSettings->enableFD))
    {
        // StreamOn FD HW
        m_hwPowerOn = TRUE;
    }

    if ((TRUE == pFrameSettings->enableFD) && (FALSE == pFrameSettings->hwSettings.skipProcess))
    {
        *pProcessFrame = TRUE;
    }
    else
    {
        *pProcessFrame = FALSE;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FDHwNode::SignalFDOutputFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::SignalFDOutputFence(
    PerRequestActivePorts* pPerRequestPorts)
{
    CamxResult  result = CamxResultSuccess;
    for (UINT portIndex = 0; portIndex < pPerRequestPorts->numOutputPorts; portIndex++)
    {
        PerRequestOutputPortInfo* pOutputPort = &pPerRequestPorts->pOutputPorts[portIndex];

        // With late binding, with HW disable early signalling is done by which buffer is not allocated.
        // So don't check the buffers.
        if (FALSE == m_bDisableFDHW)
        {
            ImageBuffer* pFDResultsBuffer = pOutputPort->ppImageBuffer[0];
            BYTE*        pVirtualAddr     = pFDResultsBuffer->GetPlaneVirtualAddr(0, 0);

            if (NULL == pVirtualAddr)
            {
                CAMX_LOG_ERROR(CamxLogGroupFD, "pVirtualAddr is Null %x", pVirtualAddr);
                result = CamxResultEFailed;
                break;
            }
            else
            {
                /// memset only the result buffer but signal on all output ports
                if (FDHwOutputPortResults == portIndex)
                {
                    CamX::Utils::Memset(pVirtualAddr, 0, pFDResultsBuffer->GetPlaneSize(0));
                }
            }
        }

        result = CSLFenceSignal(*pOutputPort->phFence, CSLFenceResultSuccess);

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::HandleProcessFrame
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::HandleProcessFrame(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    FDPropertyFrameSettings*    pFrameSettings)
{
    CAMX_ASSERT(NULL != pExecuteProcessRequestData);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pNodeProcessRequestData);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest);

    Packet*                 pFDPacket                           = NULL;
    CmdBuffer*              pFDCmdBuffer[CSLFDCmdBufferIdMax]   = { NULL };
    NodeProcessRequestData* pNodeRequestData                    = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pPerRequestPorts                    = pExecuteProcessRequestData->pEnabledPortsInfo;
    CamxResult              result                              = CamxResultSuccess;
    UINT64                  requestId;
    UINT32                  cmdBufferIndex                      = 0;

    CAMX_UNREFERENCED_PARAM(pFrameSettings);

    requestId = pNodeRequestData->pCaptureRequest->requestId;
    CAMX_LOG_VERBOSE(CamxLogGroupFD, "HW Node requestId=%lld", requestId);

    // Get CmdBuffer for request
    CAMX_ASSERT(NULL != m_pFDPacketManager);
    CAMX_ASSERT(NULL != m_pFDCmdBufferManager[CSLFDCmdBufferIdGeneric]);
    CAMX_ASSERT(NULL != m_pFDCmdBufferManager[CSLFDCmdBufferIdCDM]);

    pFDPacket                               = GetPacketForRequest(requestId, m_pFDPacketManager);
    pFDCmdBuffer[CSLFDCmdBufferIdGeneric]   =
        GetCmdBufferForRequest(requestId, m_pFDCmdBufferManager[CSLFDCmdBufferIdGeneric]);
    pFDCmdBuffer[CSLFDCmdBufferIdCDM]       =
        GetCmdBufferForRequest(requestId, m_pFDCmdBufferManager[CSLFDCmdBufferIdCDM]);

    // Check for mandatory buffers (even for bypass test)
    if ((NULL == pFDPacket)                             ||
        (NULL == pFDCmdBuffer[CSLFDCmdBufferIdGeneric]) ||
        (NULL == pFDCmdBuffer[CSLFDCmdBufferIdCDM]))
    {
        CAMX_LOG_ERROR(CamxLogGroupFD, "Null IQPacket or CmdBuffer %x, %x",
                       pFDPacket, pFDCmdBuffer[CSLFDCmdBufferIdGeneric]);

        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        result = ProgramFDCDMConfig(pFDCmdBuffer[CSLFDCmdBufferIdCDM]);
    }

    if (CamxResultSuccess == result)
    {
        result = ProgramFDGenericConfig(pFDCmdBuffer[CSLFDCmdBufferIdGeneric]);
    }

    if (CamxResultSuccess == result)
    {
        pFDPacket->SetRequestId(GetCSLSyncId(requestId));
        pFDPacket->SetOpcode(CSLDeviceType::CSLDeviceTypeFD, CSLPacketOpCodesFDFrameUpdate);
    }

    if (CamxResultSuccess == result)
    {
        for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
        {
            PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];

            if ((NULL != pInputPort) && (NULL != pInputPort->pImageBuffer))
            {
                result = pFDPacket->AddIOConfig(pInputPort->pImageBuffer,
                                                pInputPort->portId,
                                                CSLIODirection::CSLIODirectionInput,
                                                pInputPort->phFence,
                                                1,
                                                NULL,
                                                NULL,
                                                0);

                CAMX_ASSERT(NULL != pInputPort->phFence);
                CAMX_ASSERT(NULL != pNodeRequestData->pCaptureRequest);

                CAMX_LOG_VERBOSE(CamxLogGroupFD, "FD reporting Input config, portId=%d, hFence=%d, imgBuf=0x%x, request=%llu",
                                 pInputPort->portId, *(pInputPort->phFence), pInputPort->pImageBuffer,
                                 pNodeRequestData->pCaptureRequest->requestId);

            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupFD, "Input Port/Image Buffer is Null ");

                result = CamxResultEInvalidArg;
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupFD, "Input Port: Add IO config failed i=%d, result=%d", i, result);
                break;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        for (UINT portIndex = 0; portIndex < pPerRequestPorts->numOutputPorts; portIndex++)
        {
            PerRequestOutputPortInfo* pOutputPort   = &pPerRequestPorts->pOutputPorts[portIndex];
            ImageBuffer*              pImageBuffer  = pOutputPort->ppImageBuffer[0];

            CAMX_ASSERT(NULL != pOutputPort);

            // Even though there are 4 buffers on same port, AddIOConfig shall be called only once.
            if (NULL != pImageBuffer)
            {
                result = pFDPacket->AddIOConfig(pImageBuffer,
                                                pOutputPort->portId,
                                                CSLIODirection::CSLIODirectionOutput,
                                                pOutputPort->phFence,
                                                1,
                                                NULL,
                                                NULL,
                                                0);

                CAMX_LOG_VERBOSE(CamxLogGroupFD, "FD reporting Output config, portId=%d, hFence=%d, imgBuf=0x%x, request=%llu",
                                 pOutputPort->portId, *(pOutputPort->phFence), pImageBuffer,
                                 pNodeRequestData->pCaptureRequest->requestId);
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupFD, "Output Port: Add IO config failed, result=%d", result);
                break;
            }
        }

        if (CamxResultSuccess == result)
        {
            result = pFDPacket->CommitPacket();
        }

        if (CamxResultSuccess == result)
        {
            pFDCmdBuffer[CSLFDCmdBufferIdGeneric]->SetMetadata(static_cast<UINT32>(CSLFDCmdBufferIdGeneric));
            result = pFDPacket->AddCmdBufferReference(pFDCmdBuffer[CSLFDCmdBufferIdGeneric], &cmdBufferIndex);
        }

        if (CamxResultSuccess == result)
        {
            pFDCmdBuffer[CSLFDCmdBufferIdCDM]->SetMetadata(static_cast<UINT32>(CSLFDCmdBufferIdCDM));
            result = pFDPacket->AddCmdBufferReference(pFDCmdBuffer[CSLFDCmdBufferIdCDM], NULL);
        }

        if (CamxResultSuccess == result)
        {
            result = pFDPacket->SetKMDCmdBufferIndex(cmdBufferIndex,
                (pFDCmdBuffer[CSLFDCmdBufferIdGeneric]->GetResourceUsedDwords() * sizeof(UINT32)));
        }

        if (CamxResultSuccess == result)
        {
            result = GetHwContext()->Submit(GetCSLSession(), m_hDevice, pFDPacket);
            CAMX_LOG_VERBOSE(CamxLogGroupFD, "ReqID[%lld] Packet submitted to HW", requestId);
        }
    }
    if (CamxResultSuccess != result)
    {
        if (CamxResultECancelledRequest == result)
        {
            CAMX_LOG_INFO(CamxLogGroupChi, "Node::%s CSL Submit Failed: %s due to Ongoing flush",
                NodeIdentifierString(), Utils::CamxResultToString(result));
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupChi, "Node::%s CSL Submit Failed: %s",
                NodeIdentifierString(), Utils::CamxResultToString(result));
        }
        result = SignalFDOutputFence(pPerRequestPorts);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CAMX_ASSERT(NULL != pExecuteProcessRequestData);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pNodeProcessRequestData);
    CAMX_ASSERT(NULL != pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest);

    CamxResult              result             = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData   = pExecuteProcessRequestData->pNodeProcessRequestData;
    FDPropertyFrameSettings frameSettings      = {0};
    BOOL                    processFrame       = FALSE;
    DependencySequence      dependencySequence = static_cast<DependencySequence>(pNodeRequestData->processSequenceId);

    CAMX_LOG_VERBOSE(CamxLogGroupFD, "Instance[%d] ReqId[%llu] : dependencySequence = %d",
                     InstanceID(), pNodeRequestData->pCaptureRequest->requestId, dependencySequence);

    if ((0 == m_FDFrameWidth) &&
        (NULL != pExecuteProcessRequestData->pEnabledPortsInfo))
    {
        UINT32                   numInputPorts = pExecuteProcessRequestData->pEnabledPortsInfo->numInputPorts;
        PerRequestInputPortInfo* pInputPorts   = pExecuteProcessRequestData->pEnabledPortsInfo->pInputPorts;

        for (UINT32 i = 0; i < numInputPorts; i++)
        {
            if (CSLFDInputPortIdImage == pInputPorts[i].portId)
            {
                if (NULL != pInputPorts[i].pImageBuffer)
                {
                    YUVFormat yuvFormat = { 0 };
                    if (NULL != pInputPorts[i].pImageBuffer->GetFormat())
                    {
                        yuvFormat = pInputPorts[i].pImageBuffer->GetFormat()->formatParams.yuvFormat[0];
                        m_FDFrameWidth    = yuvFormat.width;
                        m_FDFrameHeight   = yuvFormat.height;
                        m_FDFrameStride   = yuvFormat.planeStride;
                        m_FDFrameScanline = yuvFormat.sliceHeight;

                        CAMX_LOG_VERBOSE(CamxLogGroupFD, "Input image dim width %d height %d stride %d scanline %d",
                            m_FDFrameWidth, m_FDFrameHeight, m_FDFrameStride, m_FDFrameScanline);
                        break;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupFD, "Input Port[%d] Image format is NULL", pInputPorts[i]);
                        result = CamxResultEInvalidPointer;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupFD, "Input Port[%d] Image buffer is NULL", pInputPorts[i]);
                    result = CamxResultEInvalidPointer;
                }
            }
        }
    }


    if (CamxResultSuccess == result)
    {
        if (DependencySequence::DependencySetNone == dependencySequence)
        {
            if (TRUE == m_bDisableFDHW)
            {
                result = SignalFDOutputFence(pExecuteProcessRequestData->pEnabledPortsInfo);
                CAMX_LOG_VERBOSE(CamxLogGroupFD, "Instance[%d] ReqId[%llu] HW disabled signalling"
                                 "the o/p fence without actual processing result %d",
                                 InstanceID(), pNodeRequestData->pCaptureRequest->requestId, result);
            }
            else
            {
                SetDependencies(pExecuteProcessRequestData);
            }
        }

        if (DependencySequence::DependencySetFrameSettings == dependencySequence)
        {
            static const UINT PropertiesFDFrameSetting[] = { PropertyIDFDFrameSettings };
            VOID*             pData[1] = { 0 };
            UINT64            propertyDataFDFrameSettingsOffset[1] = { 0 };

            GetDataList(PropertiesFDFrameSetting, pData, propertyDataFDFrameSettingsOffset, 1);
            if (NULL != pData[0])
            {
                frameSettings = *reinterpret_cast<FDPropertyFrameSettings*>(pData[0]);
            }

            result = DetermineProcessFrame(&frameSettings, &processFrame);

            CAMX_LOG_VERBOSE(CamxLogGroupFD, "Instance[%d] ReqId[%llu] : processFrame=%d",
                             InstanceID(), pNodeRequestData->pCaptureRequest->requestId, processFrame);

            if ((CamxResultSuccess == result) &&
                (TRUE == processFrame))
            {
                CAMX_LOG_VERBOSE(CamxLogGroupFD, "Instance[%d] ReqId[%llu] : Processing",
                                 InstanceID(), pNodeRequestData->pCaptureRequest->requestId);

                // Check if there is any change in FD configuration.
                CheckFDConfigChange(&m_FDConfig);

                result = HandleProcessFrame(pExecuteProcessRequestData, &frameSettings);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupFD, "Instance[%d] ReqId[%llu] Skipping",
                                 InstanceID(), pNodeRequestData->pCaptureRequest->requestId);

                result = SignalFDOutputFence(pExecuteProcessRequestData->pEnabledPortsInfo);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::AcquireDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::AcquireDevice()
{
    CSLFDAcquireDeviceInfo  FDAcquireInfo           = {};
    CSLDeviceResource       deviceResourceRequest   = { 0 };
    CamxResult              result                  = CamxResultSuccess;

    FDAcquireInfo.priority      = CSLFDPriorityNormal;
    FDAcquireInfo.mode          = CSLFDHWModeFaceDetection;
    FDAcquireInfo.getRawResults = FALSE;

    deviceResourceRequest.resourceID                = 1;
    deviceResourceRequest.pDeviceResourceParam      = static_cast<VOID*>(&FDAcquireInfo);
    deviceResourceRequest.deviceResourceParamSize   = sizeof(CSLFDAcquireDeviceInfo);

    result = CSLAcquireDevice(GetCSLSession(),
                              &m_hDevice,
                              DeviceIndices()[0],
                              &deviceResourceRequest,
                              1,
                              NULL,
                              0,
                              NodeIdentifierString());

    if (CamxResultSuccess == result)
    {
        SetDeviceAcquired(TRUE);
        AddCSLDeviceHandle(m_hDevice);

        result = CSLQueryDeviceCapabilities(DeviceIndices()[0],
                                            &m_hwCaps,
                                            sizeof(CSLFDHWCaps));

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupFD, "Device query cap failed, result=%d", result);
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupFD, "Device query cap core_version:%d.%d.%d, wrapper_version=%d.%d.%d",
                             m_hwCaps.coreVesion.majorVersion,
                             m_hwCaps.coreVesion.minorVersion,
                             m_hwCaps.coreVesion.revVersion,
                             m_hwCaps.wrapperVersion.majorVersion,
                             m_hwCaps.wrapperVersion.minorVersion,
                             m_hwCaps.wrapperVersion.revVersion);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupFD, "FDHw Acquire device failed, result=%d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::ReleaseDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::ReleaseDevice()
{
    CamxResult result = CamxResultSuccess;

    if (NULL != GetHwContext())
    {
        result = CSLReleaseDevice(GetCSLSession(), m_hDevice);

        if (CamxResultSuccess == result)
        {
            SetDeviceAcquired(FALSE);
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to release device, result=%d", result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::ConfigureFDHwCapability
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::ConfigureFDHwCapability()
{
    CamxResult  result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::ProgramFDCDMConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::ProgramFDCDMConfig(
    CmdBuffer* pCDMCmdBuffer)
{
    CamxResult  result          = CamxResultSuccess;
    UINT32      numRegisters    = 0;
    FDRegVal    fdRegs[MaxFDRegistersInCDM];
    UINT32      minFaceSize;

    minFaceSize = FDUtils::GetFaceSize(&m_FDConfig.hwConfig.minFaceSize, FDHwUtils::FDMinFacePixels0, m_FDFrameHeight);

    fdRegs[numRegisters].reg    = regFD_WRAPPER_FD_A_FD_COND;
    result = m_FDHwUtils.UpdateFDHwCondValue(m_FDHwVersion, m_FDConfig.hwConfig.enableHWFP,
                                             minFaceSize, -1, m_FDConfig.hwConfig.angle, &fdRegs[numRegisters].val);
    numRegisters++;

    if (CamxResultSuccess == result)
    {
        fdRegs[numRegisters].reg = regFD_WRAPPER_FD_A_FD_STARTX;
        fdRegs[numRegisters].val = 0x0;
        numRegisters++;

        fdRegs[numRegisters].reg = regFD_WRAPPER_FD_A_FD_STARTY;
        fdRegs[numRegisters].val = 0x0;
        numRegisters++;

        fdRegs[numRegisters].reg = regFD_WRAPPER_FD_A_FD_DHIT;
        fdRegs[numRegisters].val =
            m_FDHwUtils.GetFDHwThresholdRegVal(m_FDHwVersion, m_FDConfig.hwConfig.threshold);
        numRegisters++;

        fdRegs[numRegisters].reg = regFD_WRAPPER_FD_A_FD_IMAGE;
        result = m_FDHwUtils.UpdateFDHwImageValue(m_baseFDHWDimension.width,
                                                  m_baseFDHWDimension.height,
                                                  &fdRegs[numRegisters].val);
        numRegisters++;

        fdRegs[numRegisters].reg    = regFD_WRAPPER_FD_A_FD_LINEBYTES;
        fdRegs[numRegisters].val    = m_FDFrameStride;
        numRegisters++;

        fdRegs[numRegisters].reg = regFD_WRAPPER_FD_A_FD_SIZEX;
        fdRegs[numRegisters].val = m_FDFrameWidth;
        numRegisters++;

        fdRegs[numRegisters].reg = regFD_WRAPPER_FD_A_FD_SIZEY;
        fdRegs[numRegisters].val = m_FDFrameHeight;
        numRegisters++;
    }

    if (CamxResultSuccess == result)
    {
        result = PacketBuilder::WriteInterleavedRegs(pCDMCmdBuffer, numRegisters, reinterpret_cast<UINT32*>(fdRegs));
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupFD, "Failed in writing CDM registers, numRegisters=%d, result=%d",
                             numRegisters, result);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::ProgramFDGenericConfig()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult FDHwNode::ProgramFDGenericConfig(
    CmdBuffer*      pGenericCmdBuffer)
{
    CamxResult  result = CamxResultSuccess;

    UINT32 getRawResults= FALSE;

    result = PacketBuilder::WriteGenericBlobData(pGenericCmdBuffer,
                                                 CSLFDBlobTypeRawResultsRequired,
                                                 static_cast<UINT32>(sizeof(UINT32)),
                                                 reinterpret_cast<BYTE*>(&getRawResults));

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupFD, "Failed in writing Generic blob %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID FDHwNode::SetDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData)
{
    CAMX_ASSERT(NULL != pExecuteProcessRequestData);

    NodeProcessRequestData* pNodeRequestData             = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pEnabledPorts                = pExecuteProcessRequestData->pEnabledPortsInfo;
    UINT                    count                        = 0;
    UINT                    dependencyUnitIndex          = 0;
    DependencyUnit*         pDependencyUnit              = &pNodeRequestData->dependencyInfo[dependencyUnitIndex];
    UINT64                  requestId                    = pNodeRequestData->pCaptureRequest->requestId;
    UINT64                  requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);

    // Set Property dependencies

    // Set a dependency on the completion of the previous ExecuteProcessRequest() call
    // so that we can guarantee serialization of all ExecuteProcessRequest() calls for this node.
    // Skip setting dependency for first request
    if (FirstValidRequestId < requestIdOffsetFromLastFlush)
    {
        pDependencyUnit->propertyDependency.properties[count]  = GetNodeCompleteProperty();
        // Always point to the previous request. Should NOT be tied to the pipeline delay!
        pDependencyUnit->propertyDependency.offsets[count]     = 1;
        count++;
    }
    if (TRUE == IsTagPresentInPublishList(PropertyIDFDFrameSettings))
    {
        pDependencyUnit->propertyDependency.properties[count++] = PropertyIDFDFrameSettings;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "pipeline cannot publish property: %08x ", PropertyIDFDFrameSettings);
    }
    pDependencyUnit->propertyDependency.count               = count;
    pDependencyUnit->dependencyFlags.hasPropertyDependency  = TRUE;

    if (TRUE == GetStaticSettings()->enableImageBufferLateBinding)
    {
        // If latebinding is enabled, we want to delay packet preparation as late as possible, in other terms, we want to
        // prepare and submit to hw when it can really start processing. This is once all the input fences (+ property)
        // dependencies are satisfied. So, lets set input fence dependencies
        SetInputBuffersReadyDependency(pExecuteProcessRequestData, dependencyUnitIndex);
    }


    pDependencyUnit->processSequenceId = static_cast<UINT32>(DependencySequence::DependencySetFrameSettings);
    pDependencyUnit->dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;

    pNodeRequestData->numDependencyLists += 1;

    CAMX_LOG_VERBOSE(CamxLogGroupFD, "Added dependency for frame settings for req id %lld dependency mask %d",
                     pNodeRequestData->pCaptureRequest->requestId, pDependencyUnit->dependencyFlags.dependencyFlagsMask);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// FDHwNode::CheckFDConfigChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL FDHwNode::CheckFDConfigChange(
    FDConfig* pFDConfig)
{
    CamxResult          result          = CamxResultSuccess;
    BOOL                isConfigChanged = FALSE;
    FDConfigSelector    configSelector  = (TRUE == m_isVideoMode) ? FDSelectorVideo : FDSelectorDefault;
    FDProcessingType    fdtype          = InternalFDType;
    if (FDConfigVendorTag == GetStaticSettings()->FDConfigSource)
    {
        // If config is not published for this request, we keep using previous config
        result = FDUtils::GetFDConfig(this, FDConfigVendorTag, TRUE, fdtype, m_isFrontCamera,
                                     configSelector, pFDConfig);
        if (CamxResultSuccess == result)
        {
            isConfigChanged = TRUE;
        }
    }
    else
    {
        /// @todo (CAMX-2793) Check if there is anything that triggers FD config change. Exa - turbo mode.
        /// This will change the configSelector and need to read corresponding config header/binary
        BOOL reloadConfig = FALSE;

        if (TRUE == reloadConfig)
        {
            result = FDUtils::GetFDConfig(this, GetStaticSettings()->FDConfigSource, TRUE,
                                         fdtype, m_isFrontCamera, configSelector, pFDConfig);
            if (CamxResultSuccess == result)
            {
                isConfigChanged = TRUE;
            }
        }
    }

    return isConfigChanged;
}

CAMX_NAMESPACE_END
