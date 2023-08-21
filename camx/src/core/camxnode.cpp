////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxnode.cpp
/// @brief Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <queue>

#include "camxchicontext.h"
#include "camxcmdbuffermanager.h"
#include "camxcsl.h"
#include "camxerrorinducer.h"
#include "camxhal3defs.h"
#include "camxhal3metadatautil.h"
#include "camxhal3metadatautil.h"
#include "camxhal3module.h"
#include "camxhal3types.h"
#include "camxhwcontext.h"
#include "camxhwdefs.h"
#include "camxhwenvironment.h"
#include "camxhwfactory.h"
#include "camximagebuffermanager.h"
#include "camximagedump.h"
#include "camximageformatutils.h"
#include "camxincs.h"
#include "camxmem.h"
#include "camxmetadatapool.h"
#include "camxnode.h"
#include "camxnodeutils.h"
#include "camxpropertyblob.h"
#include "camxpropertydefs.h"
#include "camxsettingsmanager.h"
#include "camxthreadmanager.h"
#include "camxvendortags.h"
#include "parametertuningtypes.h"
#include "camxtitan17xdefs.h"
#include "chi.h"
#include "chibinarylog.h"

// NOWHINE FILE CP006: used standard libraries for performance improvements
// NOWHINE FILE PR007b: Whiner incorrectly concludes as non-library files

CAMX_NAMESPACE_BEGIN

using namespace std;

#define MAX_IFE_FRAMES_PER_BATCH  16

CAMX_TLS_STATIC_CLASS_DEFINE(UINT64, Node, m_tRequestId, FirstValidRequestId);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::Node
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Node::Node()
{
    m_parallelProcessRequests         = FALSE;
    m_publishTagArray.tagCount        = 0;
    m_publishTagArray.partialTagCount = 0;
    m_hThreadPipelineCreate           = InvalidJobHandle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::~Node
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Node::~Node()
{
    if ((0 != m_numCmdBufferManagers) && (NULL != m_ppCmdBufferManagers))
    {
        for (UINT i = 0; i < m_numCmdBufferManagers; i++)
        {
            if (NULL != m_ppCmdBufferManagers[i])
            {
                CAMX_DELETE m_ppCmdBufferManagers[i];
                m_ppCmdBufferManagers[i] = NULL;
            }
        }
    }

    if (NULL != m_ppCmdBufferManagers)
    {
        CAMX_FREE(m_ppCmdBufferManagers);
        m_numCmdBufferManagers    = 0;
        m_maxNumCmdBufferManagers = 0;
        m_ppCmdBufferManagers     = NULL;
    }

    if (NULL != m_pCmdBufferManagersLock)
    {
        m_pCmdBufferManagersLock->Destroy();
        m_pCmdBufferManagersLock = NULL;
    }

    if (NULL != m_pProcessRequestLock)
    {
        m_pProcessRequestLock->Destroy();
        m_pProcessRequestLock = NULL;
    }

    if (NULL != m_pBufferRequestLock)
    {
        m_pBufferRequestLock->Destroy();
        m_pBufferRequestLock = NULL;
    }

    if (NULL != m_pBufferReleaseLock)
    {
        m_pBufferReleaseLock->Destroy();
        m_pBufferReleaseLock = NULL;
    }

    if (NULL != m_pFenceCreateReleaseLock)
    {
        m_pFenceCreateReleaseLock->Destroy();
        m_pFenceCreateReleaseLock = NULL;
    }

    for (UINT tagIndex = 0; tagIndex < MaxPSMetaType; ++tagIndex)
    {
        DeInitializePSMetadata(m_pPSMData[tagIndex].tagId);
    }
    DestroyPSMQueue();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_ENTRYEXIT_NAME(CamxLogGroupCore, "NodeCreate");
    CamxResult result = CamxResultSuccess;

    const HwFactory* pFactory = HwEnvironment::GetInstance()->GetHwFactory();
    Node*            pNode    = pFactory->CreateNode(pCreateInputData, pCreateOutputData);

    if (pNode != NULL)
    {
        result = pNode->Initialize(pCreateInputData, pCreateOutputData);

        if (CamxResultSuccess != result)
        {
            pNode->Destroy();
            pNode = NULL;
        }
    }
    else
    {
        result = CamxResultENoMemory;
    }

    pCreateOutputData->pNode = pNode;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::Destroy()
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "Node Destroy: %s", NodeIdentifierString());

    WaitForDeferPipelineCreate();

    if ((m_outputPortsData.numPorts > 0) && (NULL != m_bufferNegotiationData.pOutputPortNegotiationData))
    {
        CAMX_FREE(m_bufferNegotiationData.pOutputPortNegotiationData);
        m_bufferNegotiationData.pOutputPortNegotiationData = NULL;
    }

    if ((m_inputPortsData.numPorts > 0) && (NULL != m_bufferNegotiationData.pInputPortNegotiationData))
    {
        CAMX_FREE(m_bufferNegotiationData.pInputPortNegotiationData);
        m_bufferNegotiationData.pInputPortNegotiationData = NULL;
    }

    for (UINT i = 0; i < m_outputPortsData.numPorts; i++)
    {
        CamxResult          result         = CamxResultSuccess;
        OutputPort*         pOutputPort    = &m_outputPortsData.pOutputPorts[i];
        ImageBufferManager* pBufferManager = NULL;

        // release any fences
        if ((NULL != pOutputPort) && (NULL != pOutputPort->pFenceHandlerData))
        {
            m_pFenceCreateReleaseLock->Lock();
            for (UINT bufferIndex = 0; bufferIndex < pOutputPort->bufferProperties.maxImageBuffers; bufferIndex++)
            {
                if (CSLInvalidHandle != pOutputPort->pFenceHandlerData[bufferIndex].hFence)
                {
                    if ((TRUE == pOutputPort->pFenceHandlerData[bufferIndex].primaryFence) &&
                        (FALSE == pOutputPort->pFenceHandlerData[bufferIndex].isChiFence))
                    {
                        result = CSLReleaseFence(pOutputPort->pFenceHandlerData[bufferIndex].hFence);
                    }

                    CAMX_LOG_DRQ("Released Fence Output...Node: %08x, Node::%s fence: %08x(%d),"
                        " ImgBuf:%p reqId:%llu result: %d ",
                        this, NodeIdentifierString(), &pOutputPort->pFenceHandlerData[bufferIndex],
                        pOutputPort->pFenceHandlerData[bufferIndex].hFence, pOutputPort->ppImageBuffers[0], m_tRequestId,
                        result);

                    pOutputPort->pFenceHandlerData[bufferIndex].hFence = CSLInvalidHandle;
                }

                if (CSLInvalidHandle != pOutputPort->pDelayedBufferFenceHandlerData[bufferIndex].hFence)
                {
                    if ((TRUE == pOutputPort->pFenceHandlerData[bufferIndex].primaryFence) &&
                        (FALSE == pOutputPort->pFenceHandlerData[bufferIndex].isChiFence))
                    {
                        result = CSLReleaseFence(pOutputPort->pDelayedBufferFenceHandlerData[bufferIndex].hFence);
                    }

                    CAMX_LOG_DRQ("Released Fence.Output..Node: %08x, Node::%s, fence: %08x(%d),"
                        " ImgBuf:%p reqId:%llu result: %d ",
                        this, NodeIdentifierString(), &pOutputPort->pDelayedBufferFenceHandlerData[bufferIndex],
                        pOutputPort->pDelayedBufferFenceHandlerData[bufferIndex].hFence, pOutputPort->ppImageBuffers[0],
                        m_tRequestId, result);

                    pOutputPort->pDelayedBufferFenceHandlerData[bufferIndex].hFence = CSLInvalidHandle;
                }
            }
            m_pFenceCreateReleaseLock->Unlock();
        }

        m_pBufferReleaseLock->Lock();
        if (NULL != pOutputPort)
        {
            if (NULL != pOutputPort->pImageBufferManager)
            {
                pBufferManager = pOutputPort->pImageBufferManager;
                result = pBufferManager->Deactivate(FALSE);

                CAMX_ASSERT(CamxResultSuccess == result);

                pBufferManager->Destroy();
                pBufferManager = NULL;
            }
            else
            {
                if (NULL != pOutputPort->ppImageBuffers)
                {
                    for (UINT bufferIndex = 0; bufferIndex < pOutputPort->bufferProperties.maxImageBuffers; bufferIndex++)
                    {
                        if (NULL != pOutputPort->ppImageBuffers[bufferIndex])
                        {
                            CAMX_DELETE pOutputPort->ppImageBuffers[bufferIndex];
                            pOutputPort->ppImageBuffers[bufferIndex] = NULL;
                        }
                    }
                }
            }
        }
        m_pBufferReleaseLock->Unlock();

        if ((NULL != pOutputPort) && (NULL != pOutputPort->pFenceHandlerData))
        {
            for (UINT j = 0; j < pOutputPort->bufferProperties.maxImageBuffers; j++)
            {
                if (NULL != pOutputPort->pFenceHandlerData[j].pOutputBufferInfo)
                {
                    CAMX_FREE(pOutputPort->pFenceHandlerData[j].pOutputBufferInfo);
                    pOutputPort->pFenceHandlerData[j].pOutputBufferInfo = NULL;
                }
                if (NULL != pOutputPort->pFenceHandlerData[j].delayedOutputBufferData.ppImageBuffer)
                {
                    CAMX_FREE(pOutputPort->pFenceHandlerData[j].delayedOutputBufferData.ppImageBuffer);
                    pOutputPort->pFenceHandlerData[j].delayedOutputBufferData.ppImageBuffer = NULL;
                }
            }
            CAMX_FREE(pOutputPort->pFenceHandlerData);
            pOutputPort->pFenceHandlerData = NULL;
        }

        if ((NULL != pOutputPort) && (NULL != pOutputPort->pDelayedBufferFenceHandlerData))
        {
            for (UINT j = 0; j < pOutputPort->bufferProperties.maxImageBuffers; j++)
            {
                if (NULL != pOutputPort->pDelayedBufferFenceHandlerData[j].pOutputBufferInfo)
                {
                    CAMX_FREE(pOutputPort->pDelayedBufferFenceHandlerData[j].pOutputBufferInfo);
                    pOutputPort->pDelayedBufferFenceHandlerData[j].pOutputBufferInfo = NULL;
                }
                if (NULL != pOutputPort->pDelayedBufferFenceHandlerData[j].delayedOutputBufferData.ppImageBuffer)
                {
                    CAMX_FREE(pOutputPort->pDelayedBufferFenceHandlerData[j].delayedOutputBufferData.ppImageBuffer);
                    pOutputPort->pDelayedBufferFenceHandlerData[j].delayedOutputBufferData.ppImageBuffer = NULL;
                }
            }
            CAMX_FREE(pOutputPort->pDelayedBufferFenceHandlerData);
            pOutputPort->pDelayedBufferFenceHandlerData = NULL;
        }

        if ((NULL != pOutputPort) && (NULL != pOutputPort->ppImageBuffers))
        {
            CAMX_FREE(pOutputPort->ppImageBuffers);
            pOutputPort->ppImageBuffers = NULL;
        }
    }

    ImageBufferManager* pInputPortBufferManager = NULL;
    for (UINT i = 0; i < m_inputPortsData.numPorts; i++)
    {
        InputPort* pInputPort   = &m_inputPortsData.pInputPorts[i];
        pInputPortBufferManager = pInputPort->pImageBufferManager;

        if ((TRUE == IsSourceBufferInputPort(i)) && (NULL != pInputPort->ppImageBuffers))
        {
            if (NULL != pInputPortBufferManager)
            {
                pInputPortBufferManager->Deactivate(FALSE);
                pInputPortBufferManager->Destroy();
                pInputPort->pImageBufferManager = NULL;
            }
            else
            {
                for (UINT bufferIndex = 0; bufferIndex < MaxRequestQueueDepth; bufferIndex++)
                {
                    if (NULL != pInputPort->ppImageBuffers[bufferIndex])
                    {
                        CAMX_DELETE pInputPort->ppImageBuffers[bufferIndex];
                        pInputPort->ppImageBuffers[bufferIndex] = NULL;
                    }
                }
            }

            CAMX_FREE(pInputPort->ppImageBuffers);
            pInputPort->ppImageBuffers = NULL;

            m_pFenceCreateReleaseLock->Lock();

            if (NULL != pInputPort->phFences)
            {
                for (UINT bufferIndex = 0; bufferIndex < MaxRequestQueueDepth; bufferIndex++)
                {
                    if (CSLInvalidHandle != pInputPort->phFences[bufferIndex])
                    {
                        CamxResult result = CSLReleaseFence(pInputPort->phFences[bufferIndex]);

                        CAMX_LOG_DRQ("Released Fence.Input.Node: %08x, Node::%s, fence: %08x(%d), reqId:%llu result: %d ",
                            this,
                            NodeIdentifierString(),
                            &pInputPort->phFences[bufferIndex],
                            pInputPort->phFences[bufferIndex],
                            m_tRequestId,
                            result);

                        pInputPort->phFences[bufferIndex]                   = CSLInvalidHandle;
                        pInputPort->pIsFenceSignaled[bufferIndex]           = FALSE;
                        pInputPort->pFenceSourceFlags[bufferIndex].allFlags = 0;
                    }
                }

                CAMX_FREE(pInputPort->phFences);
                pInputPort->phFences = NULL;
            }

            if (NULL != pInputPort->pIsFenceSignaled)
            {
                CAMX_FREE(pInputPort->pIsFenceSignaled);
                pInputPort->pIsFenceSignaled = NULL;
            }

            if (NULL != pInputPort->pFenceSourceFlags)
            {
                CAMX_FREE(pInputPort->pFenceSourceFlags);
                pInputPort->pFenceSourceFlags = NULL;
            }

            m_pFenceCreateReleaseLock->Unlock();
        }
    }

    if (NULL != m_outputPortsData.pOutputPorts)
    {
        CAMX_FREE(m_outputPortsData.pOutputPorts);
        m_outputPortsData.pOutputPorts = NULL;
    }

    if (NULL != m_inputPortsData.pInputPorts)
    {
        CAMX_FREE(m_inputPortsData.pInputPorts);
        m_inputPortsData.pInputPorts = NULL;
    }

    for (UINT index = 0; index < MaxRequestQueueDepth; index++)
    {
        if (NULL != m_perRequestInfo[index].activePorts.pInputPorts)
        {
            CAMX_FREE(m_perRequestInfo[index].activePorts.pInputPorts);
            m_perRequestInfo[index].activePorts.pInputPorts = NULL;
        }

        if (NULL != m_perRequestInfo[index].activePorts.pOutputPorts)
        {
            for (UINT i = 0; i < m_outputPortsData.numPorts; i++)
            {
                if (NULL != m_perRequestInfo[index].activePorts.pOutputPorts[i].ppImageBuffer)
                {
                    CAMX_FREE(m_perRequestInfo[index].activePorts.pOutputPorts[i].ppImageBuffer);
                    m_perRequestInfo[index].activePorts.pOutputPorts[i].ppImageBuffer = NULL;
                }
            }
            CAMX_FREE(m_perRequestInfo[index].activePorts.pOutputPorts);
            m_perRequestInfo[index].activePorts.pOutputPorts = NULL;
        }
    }

    if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->watermarkImage)
    {
        if (NULL != m_pWatermarkPattern)
        {
            CAMX_FREE(m_pWatermarkPattern);
            m_pWatermarkPattern = NULL;
        }
    }

#if CAMX_CONTINGENCY_INDUCER_ENABLE
    if (NULL != m_pContingencyInducer)
    {
        CAMX_DELETE m_pContingencyInducer;
        m_pContingencyInducer = NULL;
    }
#endif // CAMX_CONTINGENCY_INDUCER_ENABLE

    if (NULL != m_pHalStreamCrop)
    {
        CAMX_DELETE m_pHalStreamCrop;
        m_pHalStreamCrop = NULL;
    }

    CAMX_DELETE this;

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::InitializeNonSinkPortBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::InitializeNonSinkPortBufferProperties(
    UINT            outputPortIndex,
    const PortLink* pPortLink)
{
    if ((Sensor != Type()) && (TRUE != m_skipUpdatingBufferProperties))
    {
        OutputPort*                 pOutputPort           = &m_outputPortsData.pOutputPorts[outputPortIndex];
        const LinkBufferProperties* pLinkBufferProperties = &pPortLink->linkBufferProperties;

        pOutputPort->flags.isBatchMode                              = pPortLink->linkProperties.isBatchMode;
        pOutputPort->bufferProperties.immediateAllocImageBuffers    = pLinkBufferProperties->immediateAllocCount;
        pOutputPort->bufferProperties.maxImageBuffers               = pLinkBufferProperties->queueDepth;
        // numBatchedFrames=1 is the default value - the real value will be known when we finalize the node initialization
        // at the time the pipeline is bound to the session
        pOutputPort->numBatchedFrames                    = 1;
        pOutputPort->HALOutputBufferCombined             = FALSE;
        pOutputPort->bufferProperties.imageFormat.format = static_cast<Format>(pLinkBufferProperties->format);

        if (pLinkBufferProperties->sizeBytes > 0)
        {
            pOutputPort->bufferProperties.imageFormat.width  = pLinkBufferProperties->sizeBytes;
            pOutputPort->bufferProperties.imageFormat.height = 1;
        }

        for (UINT i = 0; i < pPortLink->linkProperties.numLinkFlags; i++)
        {
            switch (pPortLink->linkProperties.LinkFlags[i])
            {
                case LinkPropFlags::DisableLateBinding:
                    pOutputPort->flags.isLateBindingDisabled = TRUE;
                    break;
                case LinkPropFlags::DisableSelfShrinking:
                    pOutputPort->flags.disableSelfShrinking = TRUE;
                    break;
                default:
                    CAMX_ASSERT_ALWAYS_MESSAGE("Invalid link flags (flags=0x%x) coming from XML",
                                               pPortLink->linkProperties.LinkFlags[i]);
                    break;
            }
        }

        switch (pLinkBufferProperties->heap)
        {
            case BufferHeapDSP:
                pOutputPort->bufferProperties.bufferHeap = CSLBufferHeapDSP;
                break;
            case BufferHeapSystem:
                pOutputPort->bufferProperties.bufferHeap = CSLBufferHeapSystem;
                break;
            case BufferHeapEGL:
                pOutputPort->bufferProperties.bufferHeap = CSLBufferHeapEGL;
                break;
            case BufferHeapIon:
                pOutputPort->bufferProperties.bufferHeap = CSLBufferHeapIon;
                break;
            default:
                CAMX_ASSERT_ALWAYS_MESSAGE("Invalid buffer heap (%d) coming from XML", pLinkBufferProperties->heap);
                break;
        }

        for (UINT i = 0; i < pLinkBufferProperties->numMemFlags; i++)
        {
            switch (pLinkBufferProperties->memFlags[i])
            {
                case BufferMemFlag::Cache:
                    pOutputPort->bufferProperties.memFlags |= CSLMemFlagCache;
                    break;
                case BufferMemFlag::Hw:
                    pOutputPort->bufferProperties.memFlags |= CSLMemFlagHw;
                    break;
                case BufferMemFlag::Protected:
                    pOutputPort->bufferProperties.memFlags |= CSLMemFlagProtected;
                    break;
                case BufferMemFlag::CmdBuffer:
                    pOutputPort->bufferProperties.memFlags |= CSLMemFlagCmdBuffer;
                    break;
                case BufferMemFlag::UMDAccess:
                    pOutputPort->bufferProperties.memFlags |= CSLMemFlagUMDAccess;
                    break;
                case BufferMemFlag::PacketBuffer:
                    pOutputPort->bufferProperties.memFlags |= CSLMemFlagPacketBuffer;
                    break;
                case BufferMemFlag::KMDAccess:
                    pOutputPort->bufferProperties.memFlags |= CSLMemFlagKMDAccess;
                    break;
                default:
                    CAMX_ASSERT_ALWAYS_MESSAGE("Invalid buffer flags (flags=%d) coming from XML",
                                               pLinkBufferProperties->memFlags[i]);
                    break;
            }
        }

        if (CSLBufferHeapEGL == pOutputPort->bufferProperties.bufferHeap)
        {
            if (FALSE == GetStaticSettings()->enableEGLCache)
            {
                ///@ todo (CAMX-4152) Can't hardcode the producer and consumer flags always, it depends on the xml use case.
                pOutputPort->bufferProperties.consumerFlags  = ChiGralloc1ConsumerUsageCamera |
                                                           ChiGralloc1ProducerUsageCpuRead;
                pOutputPort->bufferProperties.producerFlags  = ChiGralloc1ProducerUsageCamera;
            }
            else
            {
                /// @note gralloc side if the cpu read and cpu writes enabled EGL buffers are not cachable
                pOutputPort->bufferProperties.consumerFlags  = ChiGralloc1ConsumerUsageCamera;
                pOutputPort->bufferProperties.producerFlags  = ChiGralloc1ProducerUsageCamera;
            }
            Gralloc* pGralloc = Gralloc::GetInstance();
            if (NULL != pGralloc)
            {
                pGralloc->ConvertCamxFormatToGrallocFormat(pOutputPort->bufferProperties.imageFormat.format,
                                                           &(pOutputPort->bufferProperties.grallocFormat));
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s pGralloc is NULL", NodeIdentifierString());
            }
        }

        ///@ todo (CAMX-346) Handle it properly
        pOutputPort->bufferProperties.imageFormat.colorSpace = CamX::ColorSpace::BT601Full;
        pOutputPort->bufferProperties.imageFormat.rotation   = CamX::Rotation::CW0Degrees;
        pOutputPort->bufferProperties.imageFormat.alignment  = 1;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::InitializeSinkPortBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::InitializeSinkPortBufferProperties(
    UINT                        outputPortIndex,
    const NodeCreateInputData*  pNodeCreateInputData,
    const OutputPortInfo*       pOutputPortInfo)
{
    CAMX_ASSERT(Sensor != Type());

    CAMX_UNREFERENCED_PARAM(pNodeCreateInputData);

    OutputPort*       pOutputPort       = &m_outputPortsData.pOutputPorts[outputPortIndex];
    ImageFormat*      pImageFormat      = &pOutputPort->bufferProperties.imageFormat;
    const PortLink*   pPortLink         = &pOutputPortInfo->portLink;
    ChiStreamWrapper* pChiStreamWrapper = m_pPipeline->GetOutputStreamWrapper(Type(), InstanceID(), pOutputPort->portId);
    ChiStream*        pChiStream        = NULL;

    if (NULL != pChiStreamWrapper)
    {
        pChiStream = reinterpret_cast<ChiStream*>(pChiStreamWrapper->GetNativeStream());

        pImageFormat->format = pChiStreamWrapper->GetInternalFormat();
        pImageFormat->width  = pChiStream->width;
        pImageFormat->height = pChiStream->height;

        CAMX_LOG_CONFIG(CamxLogGroupCore,
                      "Topology: Node::%s has a sink port id %d using format %d dim %dx%d",
                      NodeIdentifierString(),
                      pOutputPort->portId,
                      pChiStreamWrapper->GetInternalFormat(),
                      pChiStream->width,
                      pChiStream->height);

        pOutputPort->flags.isBatchMode           = pPortLink->linkProperties.isBatchMode;
        pOutputPort->bufferProperties.memFlags   = CSLMemFlagHw;
        pOutputPort->bufferProperties.bufferHeap = CSLBufferHeapIon;

        // numBatchedFrames=1 is the default value - the real value will be known when we finalize the node initialization
        // at the time the pipeline is bound to the session
        pOutputPort->numBatchedFrames           = 1;
        pOutputPort->HALOutputBufferCombined    = FALSE;
        // We need to wrap the HAL output buffer with ImageBuffer*. Since there could a max of RequestQueueDepth requests
        // outstanding, we'll create a max of (MaxRequestQueueDepth * numBatchedFrames) ImageBuffer*
        pOutputPort->bufferProperties.maxImageBuffers = MaxRequestQueueDepth *
                                                        GetBatchedHALOutputNum(pOutputPort->numBatchedFrames,
                                                                               pOutputPort->HALOutputBufferCombined);

        pImageFormat->alignment = 1;

        switch (pChiStream->dataspace)
        {
            case DataspaceUnknown:
                pImageFormat->colorSpace = ColorSpace::Unknown;
                break;
            case DataspaceJFIF:
            case DataspaceV0JFIF:
                pImageFormat->colorSpace = ColorSpace::BT601Full;
                break;
            case DataspaceBT601_625:
                pImageFormat->colorSpace = ColorSpace::BT601625;
                break;
            case DataspaceBT601_525:
                pImageFormat->colorSpace = ColorSpace::BT601525;
                break;
            case DataspaceBT709:
                pImageFormat->colorSpace = ColorSpace::BT709;
                break;
            case DataspaceDepth:
                pImageFormat->colorSpace = ColorSpace::Depth;
                break;
            default:
                /// @todo (CAMX-346) Add more colorspace here.
                pImageFormat->colorSpace = ColorSpace::Unknown;
                break;
        }

        switch (pChiStream->rotation)
        {
            case ChiStreamRotation::StreamRotationCCW0:
                pImageFormat->rotation = Rotation::CW0Degrees;
                break;
            case ChiStreamRotation::StreamRotationCCW90:
                pImageFormat->rotation = Rotation::CW90Degrees;
                break;
            case ChiStreamRotation::StreamRotationCCW180:
                pImageFormat->rotation = Rotation::CW180Degrees;
                break;
            case ChiStreamRotation::StreamRotationCCW270:
                pImageFormat->rotation = Rotation::CW270Degrees;
                break;
            default:
                pImageFormat->rotation = Rotation::CW0Degrees;
                break;
        }

        OutputPortNegotiationData* pPortNegotiationData = &m_bufferNegotiationData.pOutputPortNegotiationData[outputPortIndex];
        const ChiBufferOptions*    pSinkNegotiations    = pChiStreamWrapper->GetBufferNegotiationOptions();
        BufferRequirement          sinkRequirements;

        if (NULL == pSinkNegotiations)
        {
            sinkRequirements.minWidth          = pChiStream->width;
            sinkRequirements.minHeight         = pChiStream->height;
            sinkRequirements.maxWidth          = pChiStream->width;
            sinkRequirements.maxHeight         = pChiStream->height;
            sinkRequirements.optimalWidth      = pChiStream->width;
            sinkRequirements.optimalHeight     = pChiStream->height;
        }
        else
        {
            sinkRequirements.minWidth           = pSinkNegotiations->minDimension.width;
            sinkRequirements.minHeight          = pSinkNegotiations->minDimension.height;
            sinkRequirements.maxWidth           = pSinkNegotiations->maxDimension.width;
            sinkRequirements.maxHeight          = pSinkNegotiations->maxDimension.height;
            sinkRequirements.optimalWidth       = pSinkNegotiations->optimalDimension.width;
            sinkRequirements.optimalHeight      = pSinkNegotiations->optimalDimension.height;

        }

        for (UINT i = 0; i < pOutputPort->numInputPortsConnected; i++)
        {
            BufferRequirement* pBufferRequirement = &pPortNegotiationData->inputPortRequirement[i];
            pBufferRequirement->minWidth          = sinkRequirements.minWidth;
            pBufferRequirement->minHeight         = sinkRequirements.minHeight;
            pBufferRequirement->maxWidth          = sinkRequirements.maxWidth;
            pBufferRequirement->maxHeight         = sinkRequirements.maxHeight;
            pBufferRequirement->optimalWidth      = sinkRequirements.optimalWidth;
            pBufferRequirement->optimalHeight     = sinkRequirements.optimalHeight;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s pChiStreamWrapper is NULL", NodeIdentifierString());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::InitializeSourcePortBufferFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::InitializeSourcePortBufferFormat(
    UINT              inPortIndex,
    ChiStreamWrapper* pChiStreamWrapper)
{
    CAMX_ASSERT(Sensor != Type());

    InputPort*      pInputPort      = &m_inputPortsData.pInputPorts[inPortIndex];
    ImageFormat*    pImageFormat    = &pInputPort->imageFormat;
    ChiStream*      pChiStream      = reinterpret_cast<ChiStream*>(pChiStreamWrapper->GetNativeStream());
    FormatParamInfo formatParamInfo = {0};
    formatParamInfo.isHALBuffer     = TRUE; // Buffer was externally allocated
    formatParamInfo.pixelFormat     = pChiStream->format;
    formatParamInfo.outputFormat    = GetStaticSettings()->outputFormat;
    formatParamInfo.isImplDefined   = ((pChiStream->format == ChiStreamFormatImplDefined) ||
                                       (pChiStream->format == ChiStreamFormatYCrCb420_SP)) ? TRUE : FALSE;

    formatParamInfo.yuvPlaneAlign   = GetStaticSettings()->yuvPlaneAlignment;
    formatParamInfo.grallocUsage    = pChiStreamWrapper->GetGrallocUsage();
    if (TRUE == GetStaticSettings()->IsStrideSettingEnable)
    {
        formatParamInfo.planeStride = pChiStream->streamParams.planeStride;
        formatParamInfo.sliceHeight = pChiStream->streamParams.sliceHeight;
    }
    else
    {
        formatParamInfo.planeStride = 0;
        formatParamInfo.sliceHeight = 0;
    }

    pImageFormat->format    = pChiStreamWrapper->GetInternalFormat();
    pImageFormat->width     = pChiStream->width;
    pImageFormat->height    = pChiStream->height;
    pImageFormat->alignment = 1;

    CAMX_LOG_CONFIG(CamxLogGroupCore,
                  "Topology: Node::%s has a source port id %d using format %d dim %dx%d",
                  NodeIdentifierString(),
                  pInputPort->portId,
                  pChiStreamWrapper->GetInternalFormat(),
                  pChiStream->width,
                  pChiStream->height);

    ImageFormatUtils::InitializeFormatParams(pImageFormat, &formatParamInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::InitializeNonSinkHALBufferPortBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::InitializeNonSinkHALBufferPortBufferProperties(
    UINT                    outputPortIndex,
    const OutputPort*       pInplaceOutputPort,
    const ChiStreamWrapper* pChiStreamWrapper)
{
    CAMX_ASSERT(Sensor != Type());

    OutputPort*       pOutputPort  = &m_outputPortsData.pOutputPorts[outputPortIndex];
    ImageFormat*      pImageFormat = &pOutputPort->bufferProperties.imageFormat;
    ChiStream*        pChiStream   = NULL;

    if (NULL != pChiStreamWrapper)
    {
        pChiStream = reinterpret_cast<ChiStream*>(pChiStreamWrapper->GetNativeStream());

        /// @todo (CAMX-1797) Delete this function
        pImageFormat->format = pChiStreamWrapper->GetInternalFormat();
        pImageFormat->width  = pChiStream->width;
        pImageFormat->height = pChiStream->height;

        CAMX_LOG_INFO(CamxLogGroupCore,
                      "Topology: Node::%s has a non sink HAL buffer port id %d using format %d dim %dx%d. pImageFormat = %p",
                      NodeIdentifierString(),
                      pOutputPort->portId,
                      pChiStreamWrapper->GetInternalFormat(),
                      pChiStream->width,
                      pChiStream->height,
                      pImageFormat);

        pOutputPort->flags.isBatchMode           = pInplaceOutputPort->flags.isBatchMode;
        pOutputPort->bufferProperties.memFlags   = CSLMemFlagHw;
        pOutputPort->bufferProperties.bufferHeap = CSLBufferHeapIon;

        // numBatchedFrames=1 is the default value - the real value will be known when we finalize the node initialization
        // at the time the pipeline is bound to the session
        pOutputPort->numBatchedFrames            = 1;
        pOutputPort->HALOutputBufferCombined     = FALSE;
        // We need to wrap the HAL output buffer with ImageBuffer*. Since there could a max of RequestQueueDepth requests
        // outstanding, we'll create a max of (MaxRequestQueueDepth * numBatchedFrames) ImageBuffer*
        pOutputPort->bufferProperties.maxImageBuffers = (MaxRequestQueueDepth *
                                                         GetBatchedHALOutputNum(pOutputPort->numBatchedFrames,
                                                                                pOutputPort->HALOutputBufferCombined));

        pImageFormat->alignment = 1;

        switch (pChiStream->dataspace)
        {
            case DataspaceUnknown:
                pImageFormat->colorSpace = ColorSpace::Unknown;
                break;
            case DataspaceJFIF:
            case DataspaceV0JFIF:
                pImageFormat->colorSpace = ColorSpace::BT601Full;
                break;
            case DataspaceBT601_625:
                pImageFormat->colorSpace = ColorSpace::BT601625;
                break;
            case DataspaceBT601_525:
                pImageFormat->colorSpace = ColorSpace::BT601525;
                break;
            case DataspaceBT709:
                pImageFormat->colorSpace = ColorSpace::BT709;
                break;
            case DataspaceDepth:
                pImageFormat->colorSpace = ColorSpace::Depth;
                break;
            default:
                /// @todo (CAMX-346) Add more colorspace here.
                pImageFormat->colorSpace = ColorSpace::Unknown;
                break;
        }

        switch (pChiStream->rotation)
        {
            case ChiStreamRotation::StreamRotationCCW0:
                pImageFormat->rotation = Rotation::CW0Degrees;
                break;
            case ChiStreamRotation::StreamRotationCCW90:
                pImageFormat->rotation = Rotation::CW90Degrees;
                break;
            case ChiStreamRotation::StreamRotationCCW180:
                pImageFormat->rotation = Rotation::CW180Degrees;
                break;
            case ChiStreamRotation::StreamRotationCCW270:
                pImageFormat->rotation = Rotation::CW270Degrees;
                break;
            default:
                pImageFormat->rotation = Rotation::CW0Degrees;
                break;
        }

        OutputPortNegotiationData* pPortNegotiationData = &m_bufferNegotiationData.pOutputPortNegotiationData[outputPortIndex];
        /// @note If an output port has a SINK destination (input port), it can be its only destination
        BufferRequirement*         pBufferRequirement = &pPortNegotiationData->inputPortRequirement[0];

        pBufferRequirement->minWidth      = pChiStream->width;
        pBufferRequirement->minHeight     = pChiStream->height;
        pBufferRequirement->maxWidth      = pChiStream->width;
        pBufferRequirement->maxHeight     = pChiStream->height;
        pBufferRequirement->optimalWidth  = pChiStream->width;
        pBufferRequirement->optimalHeight = pChiStream->height;

        pPortNegotiationData->numInputPortsNotification++;

        if (pPortNegotiationData->numInputPortsNotification == pOutputPort->numInputPortsConnected)
        {
            // When all the input ports connected to the output port have notified the output port, it means the
            // output port has all the buffer requirements it needs to make a decision for the buffer on that output
            // port
            m_bufferNegotiationData.numOutputPortsNotified++;
        }

        // m_outputPortsData.numSinkPorts++;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s pChiStreamWrapper is NULL", NodeIdentifierString());
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::Initialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CamxResult         result          = CamxResultSuccess;
    const PerNodeInfo* pNodeCreateInfo = NULL;

    if ((NULL == pCreateInputData)              ||
        (NULL == pCreateInputData->pNodeInfo)   ||
        (NULL == pCreateInputData->pPipeline)   ||
        (NULL == pCreateInputData->pChiContext) ||
        (NULL == pCreateOutputData))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "pCreateInputData, pCreateOutputData - %p, %p is NULL",
                       pCreateInputData, pCreateOutputData);
        result = CamxResultEInvalidArg;
    }

    if (CamxResultSuccess == result)
    {
        pNodeCreateInfo = pCreateInputData->pNodeInfo;

        m_nodeType    = pNodeCreateInfo->nodeId;
        m_instanceId  = pNodeCreateInfo->instanceId;
        m_maxjpegsize = 0;

        OsUtils::SNPrintF(m_nodeIdentifierString, sizeof(m_nodeIdentifierString), "%s_%s%d",
                          pCreateInputData->pPipeline->GetPipelineName(), Name(), InstanceID());

        CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "NodeInitialize: %s", m_nodeIdentifierString);

        m_pPipeline               = pCreateInputData->pPipeline;
        m_pUsecasePool            = m_pPipeline->GetPerFramePool(PoolType::PerUsecase);
        m_pipelineNodeIndex       = pCreateInputData->pipelineNodeIndex;
        m_inputPortsData.numPorts = pNodeCreateInfo->inputPorts.numPorts;
        m_pHwContext              = pCreateInputData->pChiContext->GetHwContext();
        m_pChiContext             = pCreateInputData->pChiContext;
        m_seqIdNodeStartTime      = 0;

        memset(m_responseTimeSum, 0, sizeof(m_responseTimeSum));
        memset(m_numberOfNodeResponses, 0, sizeof(m_numberOfNodeResponses));

        if (NodeClass::Bypass == pNodeCreateInfo->nodeClass)
        {
            m_nodeFlags.isInplace                       = FALSE;
            pCreateOutputData->createFlags.isInPlace    = FALSE;
            m_nodeFlags.isBypassable                    = TRUE;
            pCreateOutputData->createFlags.isBypassable = TRUE;
        }
        else if (NodeClass::Inplace == pNodeCreateInfo->nodeClass)
        {
            m_nodeFlags.isInplace                       = TRUE;
            pCreateOutputData->createFlags.isInPlace    = TRUE;
            m_nodeFlags.isBypassable                    = FALSE;
            pCreateOutputData->createFlags.isBypassable = FALSE;
        }
        else
        {
            m_nodeFlags.isInplace                       = FALSE;
            m_nodeFlags.isBypassable                    = FALSE;
            pCreateOutputData->createFlags.isBypassable = FALSE;
            pCreateOutputData->createFlags.isInPlace    = FALSE;
        }

        CAMX_LOG_INFO(CamxLogGroupCore, "[%s] Is node identifier %d(:%s):%d bypassable = %d inplace = %d",
                      m_pPipeline->GetPipelineName(), m_nodeType, m_pNodeName, m_instanceId,
                      m_nodeFlags.isBypassable, m_nodeFlags.isInplace);

        m_nodeFlags.isRealTime   = pCreateInputData->pPipeline->IsRealTime();
        m_nodeFlags.isSecureMode = pCreateInputData->pPipeline->IsSecureMode();
        pCreateOutputData->createFlags.isDeferNotifyPipelineCreate = FALSE;

        if (0 != m_nodeExtComponentCount)
        {
            result = HwEnvironment::GetInstance()->SearchExternalComponent(&m_nodeExtComponents[0], m_nodeExtComponentCount);
        }

        if (m_inputPortsData.numPorts > 0)
        {
            m_inputPortsData.pInputPorts = static_cast<InputPort*>(CAMX_CALLOC(sizeof(InputPort) * m_inputPortsData.numPorts));
            m_bufferNegotiationData.pInputPortNegotiationData =
                static_cast<InputPortNegotiationData*>(
                    CAMX_CALLOC(sizeof(InputPortNegotiationData) * m_inputPortsData.numPorts));

            if ((NULL == m_inputPortsData.pInputPorts) || (NULL == m_bufferNegotiationData.pInputPortNegotiationData))
            {
                result = CamxResultENoMemory;
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        m_outputPortsData.numPorts = pNodeCreateInfo->outputPorts.numPorts;

        if (m_outputPortsData.numPorts > 0)
        {
            m_outputPortsData.pOutputPorts =
                static_cast<OutputPort*>(CAMX_CALLOC(sizeof(OutputPort) * m_outputPortsData.numPorts));

            if (NULL == m_outputPortsData.pOutputPorts)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Node::Initialize Cannot allocate memory for output ports");

                result = CamxResultENoMemory;
            }
            else
            {
                m_bufferNegotiationData.pOutputPortNegotiationData =
                    static_cast<OutputPortNegotiationData*>(
                        CAMX_CALLOC(sizeof(OutputPortNegotiationData) * m_outputPortsData.numPorts));

                if (NULL == m_bufferNegotiationData.pOutputPortNegotiationData)
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Node::Initialize Cannot allocate memory for buffer negotiation data");

                    result = CamxResultENoMemory;
                }
            }
        }
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Node::Initialize Cannot allocate memory for input ports");

        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        for (UINT outputPortIndex = 0; outputPortIndex < m_outputPortsData.numPorts; outputPortIndex++)
        {
            /// @todo (CAMX-359) - Have a set of utility functions to extract information from the Usecase structure instead
            ///                    of this multi-level-indirection
            const OutputPortInfo* pOutputPortCreateInfo = &pNodeCreateInfo->outputPorts.pPortInfo[outputPortIndex];
            OutputPort*           pOutputPort           = &m_outputPortsData.pOutputPorts[outputPortIndex];

            pOutputPort->portId                         = pOutputPortCreateInfo->portId;
            pOutputPort->numInputPortsConnected         = pOutputPortCreateInfo->portLink.numInputPortsConnected;
            pOutputPort->flags.isSecurePort             = m_nodeFlags.isSecureMode;
            pOutputPort->portSourceTypeId               = pOutputPortCreateInfo->portSourceTypeId;
            pOutputPort->numSourcePortsMapped           = pOutputPortCreateInfo->numSourceIdsMapped;
            pOutputPort->pMappedSourcePortIds           = pOutputPortCreateInfo->pMappedSourcePortIds;
            pOutputPort->flags.isSinkInplaceBuffer      = (TRUE == m_nodeFlags.isInplace) ?
                                                              IsSinkInplaceBufferPort(pOutputPortCreateInfo->portLink) : FALSE;

            OutputPortNegotiationData* pOutputPortNegotiationData    =
                &m_bufferNegotiationData.pOutputPortNegotiationData[outputPortIndex];

            pOutputPortNegotiationData->outputPortIndex              = outputPortIndex;
            // This will be filled in by the derived node at the end of buffer negotiation
            pOutputPortNegotiationData->pFinalOutputBufferProperties = &pOutputPort->bufferProperties;

            /// @note If an output port has a SINK destination, it can be the only destination for that link
            if ((TRUE == pOutputPortCreateInfo->flags.isSinkBuffer) || (TRUE == pOutputPortCreateInfo->flags.isSinkNoBuffer))
            {
                m_outputPortsData.sinkPortIndices[m_outputPortsData.numSinkPorts] = outputPortIndex;

                if (TRUE == pOutputPortCreateInfo->flags.isSinkBuffer)
                {
                    ChiStreamWrapper* pChiStreamWrapper =
                        m_pPipeline->GetOutputStreamWrapper(static_cast<UINT32>(Type()),
                                                            static_cast<UINT32>(InstanceID()),
                                                            static_cast<UINT32>(pOutputPort->portId));

                    if (NULL != pChiStreamWrapper)
                    {
                        pOutputPort->flags.isSinkBuffer = TRUE;
                        /// @todo (CAMX-1797) sinkTarget, sinkTargetStreamId - need to remove
                        pOutputPort->sinkTargetStreamId = pChiStreamWrapper->GetStreamIndex();

                        ChiStream* pStream              = reinterpret_cast<ChiStream*>(pChiStreamWrapper->GetNativeStream());
                        pOutputPort->streamData.portFPS = pStream->streamParams.streamFPS;

                        if ((TRUE == pChiStreamWrapper->IsVideoStream()) || (TRUE == pChiStreamWrapper->IsPreviewStream()))
                        {
                            pOutputPort->streamData.streamTypeBitmask |= RealtimeStream;
                        }
                        else
                        {
                            pOutputPort->streamData.streamTypeBitmask |= OfflineStream;
                        }
                        m_streamTypeBitmask |= pOutputPort->streamData.streamTypeBitmask;

                        pCreateOutputData->createFlags.isSinkBuffer = TRUE;

                        CAMX_ASSERT(CamxInvalidStreamId != pOutputPort->sinkTargetStreamId);

                        pOutputPort->enabledInStreamMask = (1 << pOutputPort->sinkTargetStreamId);

                        InitializeSinkPortBufferProperties(outputPortIndex, pCreateInputData, pOutputPortCreateInfo);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s pChiStreamWrapper for Port Index at %d is null",
                                       NodeIdentifierString(), outputPortIndex);
                        result = CamxResultEInvalidPointer;
                        break;
                    }
                }
                else
                {
                    pCreateOutputData->createFlags.isSinkNoBuffer = TRUE;
                    pOutputPort->flags.isSinkNoBuffer             = TRUE;
                    /// @note SinkNoBuffer is enabled in all streams
                    pOutputPort->enabledInStreamMask              = ((1 << MaxNumStreams) - 1);
                }

                /// @note If an output port has a SINK destination (input port), it can be its only destination
                CAMX_ASSERT(0 == pOutputPortNegotiationData->numInputPortsNotification);

                pOutputPortNegotiationData->numInputPortsNotification++;

                if (pOutputPortNegotiationData->numInputPortsNotification == pOutputPort->numInputPortsConnected)
                {
                    // When all the input ports connected to the output port have notified the output port, it means the
                    // output port has all the buffer requirements it needs to make a decision for the buffer on that output
                    // port
                    m_bufferNegotiationData.numOutputPortsNotified++;
                }

                m_outputPortsData.numSinkPorts++;
            }
            else
            {
                InitializeNonSinkPortBufferProperties(outputPortIndex, &pOutputPortCreateInfo->portLink);
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
        {
            const InputPortInfo* pInputPortInfo = &pNodeCreateInfo->inputPorts.pPortInfo[inputPortIndex];
            InputPort*           pInputPort     = &m_inputPortsData.pInputPorts[inputPortIndex];

            pInputPort->portSourceTypeId = pInputPortInfo->portSourceTypeId;
            pInputPort->portId           = pInputPortInfo->portId;

            if (TRUE == pInputPortInfo->flags.isSourceBuffer)
            {
                pCreateOutputData->createFlags.isSourceBuffer = TRUE;
                pInputPort->flags.isSourceBuffer              = TRUE;

                pInputPort->ppImageBuffers    = static_cast<ImageBuffer**>(CAMX_CALLOC(sizeof(ImageBuffer*) *
                                                                               MaxRequestQueueDepth));
                pInputPort->phFences          = static_cast<CSLFence*>(CAMX_CALLOC(sizeof(CSLFence) *
                                                                           MaxRequestQueueDepth));
                pInputPort->pIsFenceSignaled  = static_cast<UINT*>(CAMX_CALLOC(sizeof(UINT) *
                                                                       MaxRequestQueueDepth));
                pInputPort->pFenceSourceFlags = static_cast<CamX::InputPort::FenceSourceFlags*>(CAMX_CALLOC(sizeof(UINT) *
                                                                                                    MaxRequestQueueDepth));

                if ((NULL == pInputPort->ppImageBuffers)    ||
                    (NULL == pInputPort->phFences)          ||
                    (NULL == pInputPort->pIsFenceSignaled))
                {
                    result = CamxResultENoMemory;
                    break;
                }
                else
                {
                    BufferManagerCreateData createData = { };

                    createData.deviceIndices[0]                             = 0;
                    createData.deviceCount                                  = 0;
                    createData.maxBufferCount                               = MaxRequestQueueDepth;
                    createData.immediateAllocBufferCount                    = MaxRequestQueueDepth;
                    createData.allocateBufferMemory                         = FALSE;
                    createData.numBatchedFrames                             = 1;
                    createData.bufferManagerType                            = BufferManagerType::CamxBufferManager;
                    createData.linkProperties.pNode                         = this;
                    createData.linkProperties.isPartOfRealTimePipeline      = m_pPipeline->HasIFENode();
                    createData.linkProperties.isPartOfPreviewVideoPipeline  = m_pPipeline->HasIFENode();
                    createData.linkProperties.isPartOfSnapshotPipeline      = m_pPipeline->HasJPEGNode();
                    createData.linkProperties.isFromIFENode                 = (0x10000 == m_nodeType) ? TRUE : FALSE;

                    CHAR bufferManagerName[MaxStringLength256];
                    OsUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "%s_InputPort%d_%s",
                                      NodeIdentifierString(),
                                      pInputPort->portId, GetInputPortName(pInputPort->portId));

                    result = CreateImageBufferManager(bufferManagerName, &createData, &pInputPort->pImageBufferManager);

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "[%s] Create ImageBufferManager failed", bufferManagerName);
                        break;
                    }

                    for (UINT bufferIndex = 0; bufferIndex < MaxRequestQueueDepth; bufferIndex++)
                    {
                        pInputPort->ppImageBuffers[bufferIndex] = pInputPort->pImageBufferManager->GetImageBuffer();

                        if (NULL == pInputPort->ppImageBuffers[bufferIndex])
                        {
                            result = CamxResultENoMemory;
                            break;
                        }
                    }

                    if (CamxResultSuccess != result)
                    {
                        break;
                    }
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            // Initialize the derived hw/sw node objects
            result = ProcessingNodeInitialize(pCreateInputData, pCreateOutputData);

            CAMX_ASSERT(MaxDependentFences >= pCreateOutputData->maxInputPorts);

            if (MaxDependentFences < pCreateOutputData->maxInputPorts)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s ERROR: Need to increase the value of MaxDependentFences",
                               NodeIdentifierString());

                result = CamxResultEFailed;
            }
        }

        if (CamxResultSuccess == result)
        {
            m_maxOutputPorts                         = pCreateOutputData->maxOutputPorts;
            m_maxInputPorts                          = pCreateOutputData->maxInputPorts;
            m_nodeFlags.isInplace                    = pCreateOutputData->createFlags.isInPlace;
            m_nodeFlags.callNotifyConfigDone         = pCreateOutputData->createFlags.willNotifyConfigDone;
            m_nodeFlags.canDRQPreemptOnStopRecording = pCreateOutputData->createFlags.canDRQPreemptOnStopRecording;
            m_nodeFlags.hasDelayedNotification       = pCreateOutputData->createFlags.hasDelayedNotification;
            m_nodeFlags.isDeferNotifyPipelineCreate  = pCreateOutputData->createFlags.isDeferNotifyPipelineCreate;

            // Cache the buffer composite info of the derived node
            Utils::Memcpy(&m_bufferComposite, &pCreateOutputData->bufferComposite, sizeof(BufferGroup));

            for (UINT request = 0; request < MaxRequestQueueDepth; request++)
            {
                if (0 != m_inputPortsData.numPorts)
                {
                    m_perRequestInfo[request].activePorts.pInputPorts =
                        static_cast<PerRequestInputPortInfo*>(CAMX_CALLOC(sizeof(PerRequestInputPortInfo) *
                                                                          m_inputPortsData.numPorts));

                    if (NULL == m_perRequestInfo[request].activePorts.pInputPorts)
                    {
                        result = CamxResultENoMemory;
                        break;
                    }
                }

                if (m_outputPortsData.numPorts > 0)
                {
                    m_perRequestInfo[request].activePorts.pOutputPorts =
                        static_cast<PerRequestOutputPortInfo*>(CAMX_CALLOC(sizeof(PerRequestOutputPortInfo) *
                                                                           m_outputPortsData.numPorts));

                    if (NULL == m_perRequestInfo[request].activePorts.pOutputPorts)
                    {
                        result = CamxResultENoMemory;
                        break;
                    }

                    for (UINT i = 0; i < m_outputPortsData.numPorts; i++)
                    {
                        m_perRequestInfo[request].activePorts.pOutputPorts[i].ppImageBuffer =
                            static_cast<ImageBuffer**>(CAMX_CALLOC(sizeof(ImageBuffer*) *
                                                                   m_pPipeline->GetBatchedHALOutputNum()));

                        if (NULL == m_perRequestInfo[request].activePorts.pOutputPorts[i].ppImageBuffer)
                        {
                            result = CamxResultENoMemory;
                            break;
                        }
                    }
                }
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        CHAR  nodeMutexResource[Mutex::MaxResourceNameSize];

        OsUtils::SNPrintF(nodeMutexResource,
                          Mutex::MaxResourceNameSize,
                          "EPRLock_%s_%d",
                          Name(),
                          InstanceID());

        m_pProcessRequestLock = Mutex::Create(nodeMutexResource);
        m_pBufferReleaseLock  = Mutex::Create("BufferReleaseLock");
        if (NULL == m_pProcessRequestLock)
        {
            result = CamxResultENoMemory;
            CAMX_ASSERT("Node process request mutex creation failed");
        }

        if (NULL == m_pBufferReleaseLock)
        {
            result = CamxResultENoMemory;
            CAMX_ASSERT("Buffer release mutex creation failed");
        }

        m_pFenceCreateReleaseLock = Mutex::Create("Fence_Create_Release");
        if (NULL == m_pFenceCreateReleaseLock)
        {
            result = CamxResultENoMemory;
            CAMX_ASSERT("Node fence mutex creation failed");
        }

        OsUtils::SNPrintF(nodeMutexResource,
            Mutex::MaxResourceNameSize,
            "NodeBufferRequestLock_%s_%d",
            Name(),
            InstanceID());

        m_pBufferRequestLock = Mutex::Create(nodeMutexResource);
        if (NULL == m_pBufferRequestLock)
        {
            result = CamxResultENoMemory;
            CAMX_ASSERT("Node buffer request mutex creation failed");
        }

        m_pCmdBufferManagersLock = Mutex::Create("CmdBufferManagersLock");
        if (NULL == m_pCmdBufferManagersLock)
        {
            result = CamxResultENoMemory;
        }
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == HwEnvironment::GetInstance()->GetStaticSettings()->watermarkImage)
        {
            const StaticSettings* pSettings         = HwEnvironment::GetInstance()->GetStaticSettings();

            switch (Type())
            {
                /// @todo (CAMX-2875) Need a good way to do comparisons with HWL node types and ports.
                case IFE: // IFE  Only watermark on IFE output
                    m_pWatermarkPattern = static_cast<WatermarkPattern*>(CAMX_CALLOC(sizeof(WatermarkPattern)));

                    if (NULL != m_pWatermarkPattern)
                    {
                        const CHAR* pOffset        = pSettings->watermarkOffset;
                        const UINT  length         = CAMX_ARRAY_SIZE(pSettings->watermarkOffset);
                        CHAR        offset[length] = { '\0' };
                        CHAR*       pContext       = NULL;
                        const CHAR* pXOffsetToken  = NULL;
                        const CHAR* pYOffsetToken  = NULL;

                        if (NULL != pOffset && 0 != pOffset[0])
                        {
                            OsUtils::StrLCpy(offset, pOffset, length);
                            pXOffsetToken = OsUtils::StrTokReentrant(offset, "xX", &pContext);
                            pYOffsetToken = OsUtils::StrTokReentrant(NULL, "xX", &pContext);
                            if ((NULL != pXOffsetToken) && (NULL != pYOffsetToken))
                            {
                                m_pWatermarkPattern->watermarkOffsetX = OsUtils::StrToUL(pXOffsetToken, NULL, 0);
                                m_pWatermarkPattern->watermarkOffsetY = OsUtils::StrToUL(pYOffsetToken, NULL, 0);
                            }
                        }
                        result = ImageDump::InitializeWatermarkPattern(m_pWatermarkPattern);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Unable to allocate watermark structure. Out of memory",
                                       NodeIdentifierString());
                        result = CamxResultENoMemory;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    result = CacheVendorTagLocation();

#if CAMX_CONTINGENCY_INDUCER_ENABLE
    if (CamxResultSuccess == result)
    {
        m_pContingencyInducer = CAMX_NEW ContingencyInducer();
        if (NULL != m_pContingencyInducer)
        {
            m_pContingencyInducer->Initialize(pCreateInputData->pChiContext,
                                          pCreateInputData->pPipeline->GetPipelineName(),
                                          Name());
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Unable to allocate Inducer. Out of memory", NodeIdentifierString());
        }
    }
#endif // CONTINGENCY_INDUCER_ENABLE

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);
    CAMX_ASSERT(CamxResultSuccess == result);

    HwCameraInfo    cameraInfo;
    HwEnvironment::GetInstance()->GetCameraInfo(GetPipeline()->GetCameraId(), &cameraInfo);

    if (NULL != cameraInfo.pSensorCaps)
    {
        // store active pixel array info to base node
        m_activePixelArrayWidth  = cameraInfo.pSensorCaps->activeArraySize.width;
        m_activePixelArrayHeight = cameraInfo.pSensorCaps->activeArraySize.height;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s NULL pSensorCaps pointer", NodeIdentifierString());
        result = CamxResultEInvalidPointer;
    }

    if (NULL != pCreateOutputData)
    {
        NodeCreateFlags& rNodeCreateFlags = pCreateOutputData->createFlags;
        UINT             nodeType         = Type();
        UINT             nodeInstanceId   = InstanceID();
        auto             hPipeline        = m_pPipeline->GetPipelineDescriptor();
        BINARY_LOG(LogEvent::Node_Initialize, this, hPipeline, nodeType, nodeInstanceId, result, rNodeCreateFlags);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::InitializeCmdBufferManagerList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::InitializeCmdBufferManagerList(
    UINT maxNumCmdBufferManagers)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((NULL == m_ppCmdBufferManagers) && (0 != maxNumCmdBufferManagers));

    m_maxNumCmdBufferManagers = 0;
    m_ppCmdBufferManagers     =
        static_cast<CmdBufferManager**>(CAMX_CALLOC(sizeof(CmdBufferManager*) * maxNumCmdBufferManagers));

    if (NULL == m_ppCmdBufferManagers)
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("InitializeCmdBufferManagerList: CAMX_CALLOC failed");
        result = CamxResultENoMemory;
    }
    else
    {
        m_maxNumCmdBufferManagers = maxNumCmdBufferManagers;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::IsNodeDisabledWithOverride
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Node::IsNodeDisabledWithOverride()
{
    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DisablePerRequestOutputPortLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Node::DisablePerRequestOutputPortLink(
    UINT64 requestId)
{
    BOOL portStatusUpdated = FALSE;

    // Current changes
    for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
    {
        InputPort* pInputPort                   = &m_inputPortsData.pInputPorts[inputPortIndex];
        UINT64     requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);
        UINT       portId                       = m_inputPortsData.pInputPorts[inputPortIndex].portId;

        if ((requestIdOffsetFromLastFlush > pInputPort->bufferDelta) &&
            (TRUE == Utils::IsBitReset(m_inputPortDisableMask, portId)))
        {
            if (FALSE == m_inputPortsData.pInputPorts[inputPortIndex].portDisabled)
            {
                if (FALSE == IsSourceBufferInputPort(inputPortIndex))
                {
                    pInputPort->pParentNode->RemovePerRequestInputPortLinkForOutputPort(pInputPort->parentOutputPortIndex,
                        (requestId - pInputPort->bufferDelta));
                    portStatusUpdated = TRUE;
                }
            }
            m_inputPortDisableMask |= Utils::BitSet(m_inputPortDisableMask, portId);
        }
    }

    return portStatusUpdated;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::IsPortStatusUpdatedByOverride
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Node::IsPortStatusUpdatedByOverride()
{
    BOOL portStatusUpdated = FALSE;

    for (UINT portIndex = 0; portIndex < m_inputPortsData.numPorts; portIndex++)
    {
        UINT portId = m_inputPortsData.pInputPorts[portIndex].portId;

        if (FALSE == IsSupportedPortConfiguration(portId))
        {
            if (TRUE == Utils::IsBitReset(m_inputPortDisableMask, portId))
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCore, "%s Disable Port: %u", NodeIdentifierString(), portId);
                // Disable either input or reference port
                m_inputPortDisableMask |= Utils::BitSet(m_inputPortDisableMask, portId);
                DisableInputOutputLink(portIndex, TRUE);

                if ((TRUE == m_bHasLoopBackPorts) &&
                    (TRUE == m_inputPortsData.pInputPorts[portIndex].flags.isLoopback))
                {
                    UINT inputPortId = GetCorrespondingPassInputPortForInputRefPort(portId);
                    if (inputPortId != portId)
                    {
                        // Also disable corresponding input port if disabling reference port
                        m_inputPortDisableMask |= Utils::BitSet(m_inputPortDisableMask, inputPortId);
                        DisableInputOutputLink(InputPortIndex(inputPortId), TRUE);
                    }
                }
                portStatusUpdated = TRUE;
            }
        }
        else if (TRUE == Utils::IsBitSet(m_inputPortDisableMask, portId))
        {
            if (TRUE == m_bHasLoopBackPorts)
            {
                if (FALSE == m_inputPortsData.pInputPorts[portIndex].flags.isLoopback)
                {
                    if (FALSE == Utils::IsBitSet(m_inputPortDisableMask, GetCorrespondingInputRefPortForPassInputPort(portId)))
                    {
                        // For node with loopback ports only re-enable input port if corresponding reference port is enabled
                        m_inputPortDisableMask &= Utils::BitReset(m_inputPortDisableMask, portId);
                        EnableInputOutputLink(portIndex);
                        portStatusUpdated = TRUE;
                    }
                }
                else
                {
                    UINT inputPortId = GetCorrespondingPassInputPortForInputRefPort(portId);
                    if (inputPortId != portId)
                    {
                        m_inputPortDisableMask &= Utils::BitReset(m_inputPortDisableMask, inputPortId);
                        // Enable corresponding input port
                        EnableInputOutputLink(InputPortIndex(inputPortId));
                    }

                    m_inputPortDisableMask &= Utils::BitReset(m_inputPortDisableMask, portId);
                    // Enable reference port
                    EnableInputOutputLink(portIndex);
                    portStatusUpdated = TRUE;
                }
            }
            else
            {
                m_inputPortDisableMask &= Utils::BitReset(m_inputPortDisableMask, portId);
                // Enable input port
                EnableInputOutputLink(portIndex);
                portStatusUpdated = TRUE;
            }
        }
    }

    return portStatusUpdated;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SetupRequest(
    PerBatchedFrameInfo* pPerBatchedFrameInfo,
    UINT*                pCurrentActiveStreams,
    BOOL                 differentStreams,
    UINT64               requestId,
    UINT64               syncId,
    BOOL*                pIsNodeEnabled)
{
    CAMX_ASSERT(NULL != pIsNodeEnabled);
    CAMX_ASSERT(NULL != pPerBatchedFrameInfo);

    BOOL                   needPortsSetupUpdate = FALSE;
    UINT                   requestIdIndex       = requestId % MaxRequestQueueDepth;
    PerRequestActivePorts* pRequestPorts        = &m_perRequestInfo[requestIdIndex].activePorts;

    m_tRequestId = requestId;

    CamxResult result = CamxResultSuccess;

    m_perRequestInfo[requestIdIndex].numUnsignaledFences      = 0;
    m_perRequestInfo[requestIdIndex].numUnprocessedFences     = 0;
    m_perRequestInfo[requestIdIndex].requestId                = requestId;
    m_perRequestInfo[requestIdIndex].partialMetadataComplete  = 0;
    m_perRequestInfo[requestIdIndex].metadataComplete         = 0;
    m_perRequestInfo[requestIdIndex].requestComplete          = 0;
    m_perRequestInfo[requestIdIndex].partialPublishedSet.clear();

    memset(m_perRequestInfo[requestIdIndex].nodeProcessingStages, 0,
        sizeof(m_perRequestInfo[requestIdIndex].nodeProcessingStages));

    PerRequestOutputPortInfo* pPort = m_perRequestInfo[requestIdIndex].activePorts.pOutputPorts;

    for (UINT portidx = 0; portidx < m_outputPortsData.numPorts; portidx++)
    {
        pPort[portidx].numPerReqInputPortsDisabled = 0;
    }

    SetRequestStatus(requestId, PerRequestNodeStatus::Setup);

    // Set the CSL sync ID for this request ID
    SetCSLSyncId(requestId, syncId);

    if (NULL != pCurrentActiveStreams)
    {
        if (FALSE == IsRealTime())
        {
            // update node ports status if overriden by node due to limitations
            needPortsSetupUpdate = IsPortStatusUpdatedByOverride();

            // update node parameters which depends on status of ports

            // This vitual function currently used by IPE node only to update number of passes.
            // When node is dynamically enabled/disabled, there is corner case where m_numPasses do not set back for currently
            // enabled Ports.
            // Suppose for a request, offiline pipeline's node is disabled, and it disables all the ports in
            // DisablePerRequestOutputPortLink().
            // Once the node is enabled for a request, IsPortStatusUpdatedByOverride() updates m_inputPortDisableMask based
            // on current active ports. Accordingly m_numPasses should be updated to correct value based on final
            // m_inputPortDisableMask.
            // Below API makes a call in IPE node and update the m_numPasses correctly.

            UpdateNodeParamsOnPortStatus();
        }

        // Has the active streams or port status changed since the last request
        if ((TRUE == differentStreams) || (TRUE == needPortsSetupUpdate))
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "Setup Active streams afresh");
            NewActiveStreamsSetup(*pCurrentActiveStreams);
        }
    }

    // The new set of streams for the current request may have disabled the node. For e.g. if this is a Node with
    // one sink port and that sink port is not active for the current request then the entire node is disabled

    if (TRUE == IsNodeEnabled())
    {
        result = SetupRequestOutputPorts(pPerBatchedFrameInfo);
        result = SetupRequestInputPorts(pPerBatchedFrameInfo);
        *pIsNodeEnabled = TRUE;
    }
    else
    {
        UINT        tag     = GetNodeCompleteProperty();
        const UINT  one     = 1;
        const VOID* pOne[1] = { &one };
        WriteDataList(&tag, pOne, &one, 1);

        SetRequestStatus(requestId, PerRequestNodeStatus::Running);
        ProcessPartialMetadataDone(m_tRequestId);
        ProcessMetadataDone(m_tRequestId);
        ProcessRequestIdDone(m_tRequestId);
        DisablePerRequestOutputPortLink(m_tRequestId);
        *pIsNodeEnabled = FALSE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::ProcessRequest(
    NodeProcessRequestData* pNodeRequestData,
    UINT64                  requestId)
{
    CAMX_ASSERT(pNodeRequestData != NULL);

    // Saving the value of m_parallelProcessRequests in a local bool variable parallelProcessRequests in order to ensure that
    // any change in m_parallelProcessRequests does not cause the m_pProcessRequestLock mutex to be always in locked state.
    const BOOL parallelProcessRequests = m_parallelProcessRequests;

    if (FALSE == parallelProcessRequests)
    {
        m_pProcessRequestLock->Lock();
    }

    // copy the requestId after acquiring the lock to avoid synchronization issues
    m_tRequestId = requestId;

    BOOL       inErrorState         = m_pPipeline->RequestInErrorState(requestId);
    CamxResult result               = CamxResultEFailed;
    UINT       requestIdIndex       = requestId % MaxRequestQueueDepth;
    INT32      processSequenceId    = pNodeRequestData->processSequenceId;
    BOOL       bindIOBuffers        = pNodeRequestData->bindIOBuffers;
    BOOL       isSequenceIdInternal = pNodeRequestData->isSequenceIdInternal;
    UINT32     pipelineId           = GetPipelineId();

    pNodeRequestData->pCaptureRequest = m_pPipeline->GetRequest(requestId);

    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore,
                            "Node::%s ProcessRequest %llu Sequence %d bindIOBuffers %d",
                            NodeIdentifierString(),
                            requestId,
                            pNodeRequestData->processSequenceId,
                            pNodeRequestData->bindIOBuffers);
    CAMX_LOG_DRQ("Node::%s ProcessRequest %llu", NodeIdentifierString(), m_tRequestId);

    // Initially, set dependency pipeline id to own one
    for (UINT32 i = 0; i < MaxDependencies; i++)
    {
        for (UINT32 j = 0; j < MaxProperties; j++)
        {
            pNodeRequestData->dependencyInfo[i].propertyDependency.pipelineIds[j] = pipelineId;
        }
    }

    PerRequestActivePorts*    pRequestPorts      = &m_perRequestInfo[requestIdIndex].activePorts;
    ExecuteProcessRequestData executeProcessData = { 0 };

    executeProcessData.pNodeProcessRequestData   = pNodeRequestData;
    executeProcessData.pEnabledPortsInfo         = pRequestPorts;

    result = FillTuningModeData(reinterpret_cast<VOID**>(&executeProcessData.pTuningModeData));
    CAMX_ASSERT(CamxResultSuccess == result);

    if (0 == executeProcessData.pNodeProcessRequestData->processSequenceId)
    {
        if (TRUE == NodeHasDelayedNotification())
        {
            ProcessPartialMetadataDone(requestId);
            ProcessMetadataDone(requestId);
        }

        // Set start of node processing of sequenceId 0
        SetNodeProcessingTime(m_tRequestId, executeProcessData.pNodeProcessRequestData->processSequenceId, NodeStage::Start);

        // Set buffer dependency for ports that has bypassable parent Node
        SetPendingBufferDependency(&executeProcessData);
    }
    else if (TRUE == pNodeRequestData->isSequenceIdInternal)
    {
        InternalDependencySequenceId id;
        id = static_cast<InternalDependencySequenceId>(executeProcessData.pNodeProcessRequestData->processSequenceId);
        switch (id)
        {
            case ResolveDeferredInputBuffers:
                // Update image buffer info for the port waiting on buffer dependency
                UpdateBufferInfoforPendingInputPorts(pRequestPorts);
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupCore, "%s Unknown internal request sequence id: %d, reqid %llu",
                               NodeIdentifierString(), id, requestId);
                result = CamxResultEInvalidState;
                break;
        }

        if ((TRUE == CanDRQPreemptOnStopRecording()) && (TRUE == IsEndoOfStreamEnabled()))
        {
            // Check if current request has end of stream flag set for preempable node,
            // If so set processSequenceId = -1.
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Is end of stream enabled %d, reqid end of stream enabled on %llu",
                               IsEndoOfStreamEnabled(), GetEndoOfStreamRequestId());
            pNodeRequestData->processSequenceId = -1;
        }
        else
        {
            // Reset sequenceId to 0 so derived node will not see base Node sequenceIds
            pNodeRequestData->processSequenceId = 0;
        }
    }

    BOOL hasInternalDependency = (TRUE == (pNodeRequestData->numDependencyLists > 0)) &&
                                 (TRUE == pNodeRequestData->dependencyInfo[0].dependencyFlags.isInternalDependency);

    if (TRUE == hasInternalDependency)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                         "%s has internal dependency for request: %llu. Bypassing EPR until dependency is met.",
                         NodeIdentifierString(),
                         requestId);
    }
    else
    {
        if (TRUE == m_pPipeline->GetFlushStatus())
        {
            result = CamxResultECancelledRequest;
        }

        if ((CamxResultSuccess == result) && (FALSE == inErrorState))
        {
            if (TRUE == executeProcessData.pNodeProcessRequestData->bindIOBuffers)
            {
                // If LateBinding is enabled, input and output ImageBuffers may not have backing buffers yet.
                // If derived nodes set needBuffersOnDependencyMet for this sequenceId, that means, derived
                // node is going to access input, output buffers now. Lets bind buffers to ImageBuffers if not yet.
                result = BindInputOutputBuffers(executeProcessData.pEnabledPortsInfo, TRUE, TRUE);
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore,
                                   "Node::%s Req[%llu] processSequenceId[%d] Failed in Binding backing buffers "
                                   "to input, output ImageBuffers, result=%s",
                                   NodeIdentifierString(),
                                   requestId, executeProcessData.pNodeProcessRequestData->processSequenceId,
                                   Utils::CamxResultToString(result));
                }
            }

            if (CamxResultSuccess == result)
            {
                SetNodeProcessingTime(m_tRequestId, executeProcessData.pNodeProcessRequestData->processSequenceId,
                    NodeStage::DependenciesMet);

                SetRequestStatus(requestId, PerRequestNodeStatus::Running);
                result = ExecuteProcessRequest(&executeProcessData);

                SetNodeProcessingTime(m_tRequestId, executeProcessData.pNodeProcessRequestData->processSequenceId,
                    NodeStage::EPReqEnd);
            }

        }

        // Flush the request if it was cancelled or completely failed
        if ((CamxResultSuccess != result) || (TRUE == inErrorState))
        {
            CAMX_LOG_CONFIG(CamxLogGroupCore,
                          "Flush called on Node %s requestId %llu Sequence %u",
                          NodeIdentifierString(),
                          m_tRequestId,
                          pNodeRequestData->processSequenceId);

            Flush(requestId);
        }

        // Print the log message with the right level and update the request status accordingly.
        if ((CamxResultSuccess != result) || (TRUE == inErrorState))
        {
            if (CamxResultECancelledRequest == result)
            {
                CAMX_LOG_CONFIG(CamxLogGroupCore,
                              "Canceling RequestId %llu Sequence %u for Node: %s is in flush state.",
                              m_tRequestId,
                              pNodeRequestData->processSequenceId,
                              NodeIdentifierString());
                SetRequestStatus(requestId, PerRequestNodeStatus::Cancelled);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore,
                               "An Error occured with RequestId %llu Sequence %u for Node: %s inErrorState: %d"
                               " ExecuteProcessRequest returned with %s.",
                               m_tRequestId,
                               pNodeRequestData->processSequenceId,
                               NodeIdentifierString(),
                               inErrorState,
                               Utils::CamxResultToString(result));
                SetRequestStatus(requestId, PerRequestNodeStatus::Error);
            }
        }

        if ((CamxResultSuccess == result) && (FALSE == inErrorState))
        {
            // Check if node is registered for config done, send config done notification if no dependencies
            if (0 == executeProcessData.pNodeProcessRequestData->numDependencyLists)
            {
                if (TRUE == m_nodeFlags.callNotifyConfigDone)
                {
                    m_pPipeline->NotifyNodeConfigDone(requestId);
                }
                if (FALSE == m_perRequestInfo[requestId % MaxRequestQueueDepth].requestComplete)
                {
                    SetRequestStatus(requestId, PerRequestNodeStatus::Submit);
                    CAMX_LOG_INFO(CamxLogGroupCore, "Submitted to HW: %s Id: %d", NodeIdentifierString(), requestId);
                }
            }

            for (UINT index = 0; index < executeProcessData.pNodeProcessRequestData->numDependencyLists; index++)
            {
                // Start of next sequenceId
                DependencyUnit* pDependencyInfo = &executeProcessData.pNodeProcessRequestData->dependencyInfo[index];

                SetNodeProcessingTime(m_tRequestId, pDependencyInfo->processSequenceId,
                    NodeStage::Start);
            }
        }

        if ((FALSE == m_derivedNodeHandlesMetaDone) || (CamxResultSuccess != result) || (TRUE == inErrorState))
        {
            BOOL hasDependency = FALSE;
            for (UINT index = 0; index < executeProcessData.pNodeProcessRequestData->numDependencyLists; index++)
            {
                DependencyUnit* pDependencyInfo = &executeProcessData.pNodeProcessRequestData->dependencyInfo[index];

                if (TRUE == HasAnyDependency(pDependencyInfo))
                {
                    hasDependency = TRUE;
                    break;
                }
            }

            if (FALSE == hasDependency)
            {
                ProcessPartialMetadataDone(requestId);
                ProcessMetadataDone(requestId);
            }
        }

        if ((0 == pNodeRequestData->numDependencyLists) || (CamxResultSuccess != result))
        {
            UINT        tag     = GetNodeCompleteProperty();
            const UINT  one     = 1;
            const VOID* pOne[1] = { &one };
            WriteDataList(&tag, pOne, &one, 1);
        }

        // If node is not having any partial Tags to be published then node should send
        // Notification to Pipeline
        if ((0 == m_publishTagArray.partialTagCount) ||
            (m_perRequestInfo[requestIdIndex].partialPublishedSet.size() == m_publishTagArray.partialTagCount))
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "partialTagCount = %d partialPublishedSet.size=%d",
                             m_publishTagArray.partialTagCount, m_perRequestInfo[requestIdIndex].partialPublishedSet.size());
            ProcessPartialMetadataDone(requestId);
        }

        // For offline/reprocess requests, as the fence for a source buffer node is external we don't have the callback
        // We need to do either of the following -
        // 1) Release the input ImageBuffer when all the output buffer fences are signaled for the source buffer node,
        //    but that may be deemed too defensive, OR
        // 2) Use a pool (large enough) for container ImageBuffers for offline requests and recycle the LRU one

        // Going for (1) now, but need to be discussed further

        CAMX_LOG_CONFIG(CamxLogGroupCore, "%s Complete ProcessRequest %llu", NodeIdentifierString(), requestId);
    }

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);

    if (FALSE == parallelProcessRequests)
    {
        m_pProcessRequestLock->Unlock();
    }

    UINT nodeId         = Type();
    UINT nodeInstanceId = InstanceID();
    auto hPipeline      = m_pPipeline->GetPipelineDescriptor();
    BINARY_LOG(LogEvent::Node_ProcessRequest, hPipeline, nodeId, nodeInstanceId, requestId, inErrorState, processSequenceId,
               isSequenceIdInternal, bindIOBuffers, hasInternalDependency, result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::RecycleRetiredCmdBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::RecycleRetiredCmdBuffers(
    UINT64 requestId)
{
    for (UINT i = 0; i < m_numCmdBufferManagers; i++)
    {
        if (NULL != m_ppCmdBufferManagers[i])
        {
            m_ppCmdBufferManagers[i]->RecycleAll(GetCSLSyncId(requestId));
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupCore,
                "Recycle hitting null command Buffers. This is not normal");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::RecycleAllCmdBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::RecycleAllCmdBuffers()
{
    for (UINT i = 0; i < m_numCmdBufferManagers; i++)
    {
        if (NULL != m_ppCmdBufferManagers[i])
        {
            m_ppCmdBufferManagers[i]->RecycleAllRequests();
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupCore,
                "Recycle hitting null command Buffers. This is not normal");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetupRequestInputBypassReferences
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SetupRequestInputBypassReferences(
    InputPort*   pInputPort,
    ImageBuffer* pImageBuffer)
{
    CamxResult result = CamxResultSuccess;

    // Add references for the child nodes if the node is a bypassable node
    if ((TRUE == IsBypassableNode()) && (Type() == ChiExternalNode))
    {
        for (UINT idx = 0; idx < m_outputPortsData.numPorts; idx++)
        {
            OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[idx];

            for (UINT index = 0; index < pOutputPort->numSourcePortsMapped; index++)
            {
                UINT32 portId = pOutputPort->pMappedSourcePortIds[index];
                if (portId == pInputPort->portId)
                {
                    UINT32 totalInputPortsConnected = GetNumChildImageBufferReferences(pOutputPort);
                    for (UINT32 i = 0; i < totalInputPortsConnected; i++)
                    {
                        pImageBuffer->AddBufferManagerImageReference();
                    }
                }
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::InputStreamMatches
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Node::InputStreamMatches(
    const StreamInputBufferInfo& rInputBufferInfo,
    const InputPort&             rInputPort)
{
    BOOL result = FALSE;

    GrallocProperties grallocProperties;
    Format            selectedFormat;
    const ChiStream*  pStream = rInputBufferInfo.inputBuffer.pStream;

    grallocProperties.colorSpace         = static_cast<ColorSpace>(pStream->dataspace);
    grallocProperties.pixelFormat        = pStream->format;
    grallocProperties.grallocUsage       = m_pChiContext->GetGrallocUsage(pStream);
    grallocProperties.isInternalBuffer   = TRUE;
    grallocProperties.isRawFormat        = FALSE;
    grallocProperties.staticFormat       = HwEnvironment::GetInstance()->GetStaticSettings()->outputFormat;
    grallocProperties.isMultiLayerFormat = FALSE;

    if (CamxResultSuccess == ImageFormatUtils::GetFormat(grallocProperties, selectedFormat))
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                         "GetFormat: pixelFormat %d, outputFormat %d, selectedFormat %d usage 0x%llx",
                         grallocProperties.pixelFormat, grallocProperties.staticFormat,
                         selectedFormat, grallocProperties.grallocUsage);

        if ((rInputBufferInfo.portId == rInputPort.portId) &&
            (rInputBufferInfo.inputBuffer.pStream->width == rInputPort.imageFormat.width) &&
            (rInputBufferInfo.inputBuffer.pStream->height == rInputPort.imageFormat.height) &&
            (selectedFormat == rInputPort.imageFormat.format))
        {
            result = TRUE;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore,
                       "GetFormat failed, port %d, node %d, instance %d, wxh %dx%d, pixelFormat %d, outputFormat %d",
                       rInputBufferInfo.portId,
                       Type(),
                       InstanceID(),
                       rInputPort.imageFormat.width,
                       rInputPort.imageFormat.height,
                       grallocProperties.pixelFormat,
                       grallocProperties.staticFormat);

    }


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetupRequestInputPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SetupRequestInputPorts(
    PerBatchedFrameInfo* pPerBatchedFrameInfo)
{
    CamxResult             result                       = CamxResultSuccess;
    UINT64                 requestId                    = m_tRequestId;
    UINT                   requestIdIndex               = requestId % MaxRequestQueueDepth;
    PerRequestActivePorts* pRequestPorts                = &m_perRequestInfo[requestIdIndex].activePorts;
    UINT64                 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);
    UINT                   pipelineId                   = GetPipelineId();
    UINT                   nodeId                       = Type();
    UINT                   nodeInstanceId               = InstanceID();

    const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    pRequestPorts->numInputPorts  = 0;

    for (UINT portIndex = 0; portIndex < m_inputPortsData.numPorts; portIndex++)
    {
        if (TRUE == IsInputPortEnabled(portIndex))
        {
            InputPort*               pInputPort               = &m_inputPortsData.pInputPorts[portIndex];
            PerRequestInputPortInfo& rPerRequestInputPort     = pRequestPorts->pInputPorts[pRequestPorts->numInputPorts];
            OutputPortRequestedData  parentOutputPort         = { 0 };
            BOOL                     isSourceBufferInputPort  = IsSourceBufferInputPort(portIndex);
            UINT64                   requestIdWithBufferDelta = requestId;
            auto                     hPipeline                = m_pPipeline->GetPipelineDescriptor();
            if (requestId >= pInputPort->bufferDelta)
            {
                requestIdWithBufferDelta -= pInputPort->bufferDelta;
            }

            if (FALSE == isSourceBufferInputPort)
            {
                Node* pParentNode           = pInputPort->pParentNode;
                UINT  parentOutputPortIndex = pInputPort->parentOutputPortIndex;

                /// @todo (CAMX-954) Input ports of this node need to fetch all-batched-HAL-output-buffers
                ///                  from the parent in GetOutputPortInfo(..)

                if (requestIdOffsetFromLastFlush > pInputPort->bufferDelta)
                {
                    // If bufferDelta is zero, the ordering of setup nodes is not guaranteed.
                    // For example, IPE -> JPEG. JPEG node may gets setup before its parent node IPE.
                    // In such case, IsOutputPortActive does not give correct info. So we go ahead do GetOutputPortInfo.
                    // If bufferDelta is greater than zero, the parent node setup must have been done before the current node.
                    // So we need to make sure the output port is enabled in order to get port info from it.
                    if ((pInputPort->bufferDelta == 0) ||
                        ((pInputPort->bufferDelta > 0) &&
                         (TRUE == pParentNode->IsOutputPortActive(requestIdWithBufferDelta, parentOutputPortIndex))))
                    {
                        result = pParentNode->GetOutputPortInfo((requestId - pInputPort->bufferDelta),
                            pPerBatchedFrameInfo[0].sequenceId - pInputPort->bufferDelta,
                            parentOutputPortIndex,
                            &parentOutputPort);
                    }
                }

                if (CamxResultSuccess == result)
                {
                    // If the parent node is sensor it will return a NULL imagebuffer
                    if (NULL != parentOutputPort.pImageBuffer)
                    {
                        rPerRequestInputPort.portId           = pInputPort->portId;

                        rPerRequestInputPort.pImageBuffer     = parentOutputPort.pImageBuffer;
                        rPerRequestInputPort.phFence          = parentOutputPort.phFence;
                        rPerRequestInputPort.pIsFenceSignaled = parentOutputPort.pIsFenceSignaled;
                        rPerRequestInputPort.primaryFence     = parentOutputPort.primaryFence;

                        if (TRUE == pParentNode->IsBypassableNode())
                        {
                            rPerRequestInputPort.pDelayedOutputBufferData = parentOutputPort.pDelayedOutputBufferData;
                            rPerRequestInputPort.flags.isPendingBuffer    = TRUE;
                            CAMX_LOG_DRQ("Node::%s Bypassable parent node, buffer selection is pending for port =%d",
                                         NodeIdentifierString(),
                                         rPerRequestInputPort.portId);
                        }
                        pRequestPorts->numInputPorts++;
                    }
                    if (NULL != parentOutputPort.pOutputPort)
                    {
                        rPerRequestInputPort.flags.isInputHALBuffer =
                            parentOutputPort.pOutputPort->flags.isNonSinkHALBufferOutput;
                    }

                    CSLFence hFence   = (NULL != rPerRequestInputPort.phFence) ? *(rPerRequestInputPort.phFence) : 0;
                    UINT parentNodeId = GetParentNodeType(rPerRequestInputPort.portId);
                    BINARY_LOG(LogEvent::Node_SetupRequestInputPort, hPipeline, nodeId, nodeInstanceId, requestId,
                        requestIdOffsetFromLastFlush, rPerRequestInputPort, hFence, requestIdWithBufferDelta,
                        isSourceBufferInputPort, parentNodeId, parentOutputPortIndex);
                }
                else
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Something wrong with node's input port!");
                    break;
                }
            }
            else
            {
                CaptureRequest*        pCaptureRequest  = m_pPipeline->GetRequest(requestId);
                StreamInputBufferInfo* pInputBufferInfo = pCaptureRequest->pStreamBuffers[0].inputBufferInfo;
                ChiStreamBuffer*       pInputBuffer     = NULL;
                BOOL                   matchFound       = FALSE;
                UINT                   bufferId         = 0;

                CAMX_ASSERT(1 == pCaptureRequest->numBatchedFrames);

                // Match the portId with the incoming request
                for (bufferId = 0; bufferId < pCaptureRequest->pStreamBuffers[0].numInputBuffers; bufferId++)
                {
                    if (TRUE == InputStreamMatches(pInputBufferInfo[bufferId], *pInputPort))
                    {
                        pInputBuffer = &(pInputBufferInfo[bufferId].inputBuffer);

                        if (FALSE == IsRealTime())
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                                "Offline Pipeline: port %d, input buffer %p, node %d, instance %d, width %d, height %d",
                                pInputBufferInfo[bufferId].portId,
                                pInputBuffer,
                                Type(),
                                InstanceID(),
                                pInputPort->imageFormat.width,
                                pInputPort->imageFormat.height);
                        }

                        matchFound = TRUE;
                        break;
                    }
                }

                if ((TRUE == matchFound) && (TRUE == CamX::IsValidChiBuffer(&pInputBuffer->bufferInfo)))
                {
                    UINT32          flags          = CSLMemFlagHw;
                    ImageFormat*    pImageFormat   = NULL;
                    ImageBuffer*    pImageBuffer   = NULL;
                    ChiFence*       pChiFence      = NULL;
                    CSLFence*       phCSLFence     = NULL;
                    CHIFENCEHANDLE* phAcquireFence = NULL;

                    CAMX_ASSERT(NULL != pInputPort->ppImageBuffers);

                    pImageFormat = &pInputPort->imageFormat;
                    pImageBuffer = pInputPort->ppImageBuffers[requestIdIndex];

                    CAMX_ASSERT(NULL != pImageBuffer);
                    CAMX_ASSERT(CSLInvalidHandle == pInputPort->phFences[requestIdIndex]);

                    phCSLFence = &(pInputPort->phFences[requestIdIndex]);
                    pInputPort->pFenceSourceFlags[requestIdIndex].isChiAssociatedFence = FALSE;

                    if ((TRUE == pInputBuffer->acquireFence.valid) &&
                        (ChiFenceTypeInternal == pInputBuffer->acquireFence.type))
                    {
                        phAcquireFence = &(pInputBuffer->acquireFence.hChiFence);
                    }

                    if (TRUE == IsValidCHIFence(phAcquireFence))
                    {
                        pChiFence   = reinterpret_cast<ChiFence*>(*phAcquireFence);
                        *phCSLFence = pChiFence->hFence;
                        pInputPort->pFenceSourceFlags[requestIdIndex].isChiAssociatedFence = TRUE;

                        // the fence would be signalled from CHI Override (e.g., MFNR Feature)
                        if ((ChiFenceSuccess == pChiFence->resultState) || (ChiFenceFailed == pChiFence->resultState))
                        {
                            pInputPort->pIsFenceSignaled[requestIdIndex] = TRUE;
                        }
                        else
                        {
                            pInputPort->pIsFenceSignaled[requestIdIndex] = FALSE;

                            // If input chi fence is not signalled yet, whenever this chi fence is signalled it has to be
                            // informed to DRQ so that if some node is waiting on this input fence, it will come out.
                            // Since, chi layer doesn't care about the pipeline while signalling and also DRQ instance is not
                            // known, we cannot inform DRQ directly in the ChiSignalFence flow. So, register a fence callback
                            // here and inform DRQ in there.
                            NodeSourceInputPortChiFenceCallbackData* pData =
                                reinterpret_cast<NodeSourceInputPortChiFenceCallbackData*>(
                                CAMX_CALLOC(sizeof(NodeSourceInputPortChiFenceCallbackData)));

                            CAMX_ASSERT(NULL != pData);

                            if (NULL != pData)
                            {
                                pData->pNode            = this;
                                pData->phFence          = &(pInputPort->phFences[requestIdIndex]);
                                pData->pIsFenceSignaled = &(pInputPort->pIsFenceSignaled[requestIdIndex]);
                                pData->requestId        = requestId;

                                result = CSLFenceAsyncWait(pInputPort->phFences[requestIdIndex],
                                                           &this->NodeSourceInputPortChiFenceCallback,
                                                           pData);
                            }
                            else
                            {
                                result = CamxResultENoMemory;
                                CAMX_LOG_ERROR(CamxLogGroupDRQ, "Node::%s Out of memory", NodeIdentifierString());
                            }
                        }

                        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                                         "Node::%s Req[%llu] ChiFence:[%p][%d] isFenceSignaled:[%p][%d]",
                                         NodeIdentifierString(), requestId,
                                         phCSLFence, *phCSLFence, &pInputPort->pIsFenceSignaled[requestIdIndex],
                                         pInputPort->pIsFenceSignaled[requestIdIndex]);
                    }
                    else
                    {
                        *phCSLFence = pInputBufferInfo[bufferId].fence;
                        pInputPort->inputBufferIndex = bufferId;

                        /// @todo (CAMX-1797) Handle input buffer fence signal status
                        pInputPort->pIsFenceSignaled[requestIdIndex] = TRUE;

                        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node::%s pipeline:%d AcquireFence:%d isFenceSignaled:%d",
                                         NodeIdentifierString(),
                                         GetPipelineId(), *phCSLFence, pInputPort->pIsFenceSignaled[requestIdIndex]);
                    }

                    if (ChiExternalNode == Type())
                    {
                        flags = CSLMemFlagUMDAccess;
                        if (TRUE == IsBypassableNode())
                        {
                            flags |= CSLMemFlagHw;
                        }
                    }

                    if (FDManager == Type())
                    {
                        flags = CSLMemFlagUMDAccess;
                    }

                    CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Node::%s : pImageBuffer=%p pImageFormat=%p phNativeHandle=%p",
                                     NodeIdentifierString(), pImageBuffer, pImageFormat,
                                     pInputBuffer->bufferInfo.phBuffer);

                    if (TRUE == DisableDelayedUnmapRequired(flags, pImageFormat))
                    {
                        flags |= CSLMemFlagDisableDelayedUnmap;
                    }

                    result = pImageBuffer->Import(pImageFormat,
                                                  &pInputBuffer->bufferInfo,
                                                  0, // Offset
                                                  ImageFormatUtils::GetTotalSize(pImageFormat),
                                                  flags,
                                                  &m_deviceIndices[0],
                                                  m_deviceIndexCount,
                                                  FALSE,
                                                  FALSE);

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Import failed for Node::%s", NodeIdentifierString());
                    }

                    result = SetupRequestInputBypassReferences(pInputPort, pImageBuffer);

                    rPerRequestInputPort.flags.isInputHALBuffer = TRUE;
                    rPerRequestInputPort.portId                 = pInputPort->portId;
                    rPerRequestInputPort.pImageBuffer           = pImageBuffer;
                    rPerRequestInputPort.phFence                = &(pInputPort->phFences[requestIdIndex]);
                    rPerRequestInputPort.pIsFenceSignaled       = &(pInputPort->pIsFenceSignaled[requestIdIndex]);
                    rPerRequestInputPort.pParentSinkStream      = static_cast<ChiStreamWrapper*>(
                        pInputBuffer->pParentSinkStreamPrivateInfo);
                    pRequestPorts->numInputPorts++;

                    CSLFence hFence            = (NULL != rPerRequestInputPort.phFence) ? *(rPerRequestInputPort.phFence) : 0;
                    UINT parentNodeId          = GetParentNodeType(pInputPort->portId);
                    UINT parentOutputPortIndex = pInputPort->parentOutputPortIndex;
                    BINARY_LOG(LogEvent::Node_SetupRequestInputPort, hPipeline, nodeId, nodeInstanceId, requestId,
                        requestIdOffsetFromLastFlush, rPerRequestInputPort, hFence, requestIdWithBufferDelta,
                        isSourceBufferInputPort, parentNodeId, parentOutputPortIndex);

                    if ((TRUE == pImageBuffer->HasBackingBuffer()) && (TRUE == pInputPort->pIsFenceSignaled[requestIdIndex]))
                    {
                        DumpInputImageBuffer(pInputPort, pImageBuffer);
                    }
                    else if (TRUE == pSettings->reprocessDump)
                    {
                        // 1. When LateBinding is enabled, there is a possibility that this ImageBuffer is not yet backed up
                        //    with buffer yet. This can happen if requests are submitted to this pipeline and previous
                        //    pipeline which generates the frame, with chifences as dependency for synchronization.
                        // 2. If LateBinding is not enabled, then we will have backing buffers always, but if request is
                        //    submitted to both pipelines with chifences, the input buffer, by this time, may not be having
                        //    valid frame, i.e previous pipleine which generates this frame may not have completed yet.
                        CAMX_LOG_WARN(CamxLogGroupCore,
                                      "Node::%s Can't dump source input frame BufferAllocated=%d, FrameReady=%d",
                                      NodeIdentifierString(), pImageBuffer->HasBackingBuffer(),
                                      pInputPort->pIsFenceSignaled[requestIdIndex]);
                    }
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupCore, "No input buffers match node %d, port %d", m_nodeType, pInputPort->portId);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpInputImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::DumpInputImageBuffer(
    InputPort*   pInputPort,
    ImageBuffer* pImageBuffer)
{
    CamxResult            result           = CamxResultSuccess;
    const StaticSettings* pSettings        = HwEnvironment::GetInstance()->GetStaticSettings();
    CHAR                  inOrOutString[8] = { 0 };

    if (TRUE == pSettings->reprocessDump)
    {
        UINT32 enableNodeMask      = pSettings->autoInputImageDumpMask;
        UINT32 enableInputportMask = 0xFFFFFFFF;
        UINT32 enableInstanceMask  = 0xFFFFFFFF;
        BOOL   dump                = FALSE;
        // Filter for nodeType
        switch (m_nodeType)
        {
            case IFE: // IFE
                dump                = (FALSE == (enableNodeMask & ImageDumpIFE)) ? FALSE : TRUE;
                enableInstanceMask  = pSettings->autoImageDumpIFEInstanceMask;
                enableInputportMask = pSettings->autoImageDumpIFEinputPortMask;
                break;
            case JPEG: // JPEG
                dump                = (FALSE == (enableNodeMask & ImageDumpJPEG)) ? FALSE : TRUE;
                enableInstanceMask  = pSettings->autoImageDumpJpegHwInstanceMask;
                enableInputportMask = pSettings->autoImageDumpJpegHwinputPortMask;
                break;
            case IPE: // IPE
                dump                = (FALSE == (enableNodeMask & ImageDumpIPE)) ? FALSE : TRUE;
                enableInstanceMask  = pSettings->autoImageDumpIPEInstanceMask;
                enableInputportMask = pSettings->autoImageDumpIPEinputPortMask;
                break;
            case BPS: // BPS
                dump                = (FALSE == (enableNodeMask & ImageDumpBPS)) ? FALSE : TRUE;
                enableInstanceMask  = pSettings->autoImageDumpBPSInstanceMask;
                enableInputportMask = pSettings->autoImageDumpBPSinputPortMask;
                break;
            case FDHw: // FDHw
                dump                = (FALSE == (enableNodeMask & ImageDumpFDHw)) ? FALSE : TRUE;
                enableInstanceMask  = pSettings->autoImageDumpFDHwInstanceMask;
                enableInputportMask = pSettings->autoImageDumpFDHwinputPortMask;
                break;
            case LRME: // LRME
                dump                = (FALSE == (enableNodeMask & ImageDumpLRME)) ? FALSE : TRUE;
                enableInstanceMask  = pSettings->autoImageDumpLRMEInstanceMask;
                enableInputportMask = pSettings->autoImageDumpLRMEinputPortMask;
                break;
            case RANSAC: // RANSAC
                dump                = (FALSE == (enableNodeMask & ImageDumpRANSAC)) ? FALSE : TRUE;
                enableInstanceMask  = pSettings->autoImageDumpRANSACInstanceMask;
                enableInputportMask = pSettings->autoImageDumpRANSACinputPortMask;
                break;
            case ChiExternalNode: // ChiExternalNode
                dump                = (FALSE == (enableNodeMask & ImageDumpChiNode)) ? FALSE : TRUE;
                enableInstanceMask  = pSettings->autoImageDumpCHINodeInstanceMask;
                enableInputportMask = pSettings->autoImageDumpCHINodeinputPortMask;
                break;
            case CVP:  // CVP
                dump                = (FALSE == (enableNodeMask & ImageDumpCVP)) ? FALSE : TRUE;
                enableInstanceMask  = pSettings->autoImageDumpCVPInstanceMask;
                enableInputportMask = pSettings->autoImageDumpCVPinputPortMask;
                break;
            default:
                dump = (FALSE == (enableNodeMask & ImageDumpMisc)) ? FALSE : TRUE;
                break;
        }


        if ((TRUE == dump) &&
            (TRUE == Utils::IsBitSet(enableInputportMask, pInputPort->portId)) &&
            (TRUE == Utils::IsBitSet(enableInstanceMask, m_instanceId)))
        {
            ImageDumpInfo dumpInfo    = {0};
            dumpInfo.pPipelineName    = m_pPipeline->GetPipelineName();
            dumpInfo.pNodeName        = m_pNodeName;
            dumpInfo.nodeInstance     = m_instanceId;
            dumpInfo.portId           = pInputPort->portId;
            dumpInfo.requestId        = static_cast<UINT32>(m_tRequestId);
            dumpInfo.batchId          = 0;
            dumpInfo.numFramesInBatch = pImageBuffer->GetNumFramesInBatch();
            dumpInfo.pFormat          = pImageBuffer->GetFormat();
            dumpInfo.width            = pInputPort->imageFormat.width;
            dumpInfo.height           = pInputPort->imageFormat.height;
            dumpInfo.pBaseAddr        = pImageBuffer->GetCPUAddress();
            ImageDump::Dump(&dumpInfo);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ReleaseBufferManagerCompositeImageReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::ReleaseBufferManagerCompositeImageReference(
    CSLFence hFence,
    UINT64   requestId)
{
    const UINT              requestIdIndex      = (requestId % MaxRequestQueueDepth);
    PerRequestActivePorts&  rRequestInfo        = m_perRequestInfo[requestIdIndex].activePorts;

    // Run through all the active ports to see which all buffer reference could be released.
    for (UINT portIndex = 0; portIndex < rRequestInfo.numOutputPorts; portIndex++)
    {
        PerRequestOutputPortInfo& rOutputPortInfo = rRequestInfo.pOutputPorts[portIndex];
        OutputPort*               pOutputPort     = &m_outputPortsData.pOutputPorts[OutputPortIndex(rOutputPortInfo.portId)];

        if (pOutputPort->bufferProperties.maxImageBuffers > 0)
        {
            const UINT            bufferIndex       = (requestId % pOutputPort->bufferProperties.maxImageBuffers);
            NodeFenceHandlerData* pFenceHandlerData = &pOutputPort->pFenceHandlerData[bufferIndex];

            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Request %llu, Node %s, Port Id %d, Fence %d, Prim fence %d",
                           requestId, m_pNodeName, pOutputPort->portId,
                           pFenceHandlerData->hFence, hFence);

            // Check for secondary fences and match with primary fence
            if ((pFenceHandlerData->primaryFence == FALSE) && (pFenceHandlerData->hFence == hFence))
            {
                const UINT numBatchedFrames = (TRUE == pOutputPort->flags.isSinkBuffer) ?
                    GetBatchedHALOutputNum(pOutputPort->numBatchedFrames, pOutputPort->HALOutputBufferCombined) : 1;

                WatermarkImage(pFenceHandlerData);
                DumpData(pFenceHandlerData);

                // release the composite image buffer reference
                for (UINT batchIndex = 0; batchIndex < numBatchedFrames; batchIndex++)
                {
                    ReleaseOutputPortImageBufferReference(pOutputPort, pFenceHandlerData->requestId, batchIndex);
                }

                SignalCompositeFences(pOutputPort, pFenceHandlerData);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SignalCompositeFences
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::SignalCompositeFences(
    OutputPort*              pOutputPort,
    NodeFenceHandlerData*    pFenceHandlerData)
{

    UINT numBatchedFrames = GetBatchedHALOutputNum(pFenceHandlerData->pOutputPort->numBatchedFrames,
                                                   pFenceHandlerData->pOutputPort->HALOutputBufferCombined);

    if (TRUE == CamxAtomicCompareExchangeU(&pFenceHandlerData->isFenceSignaled, 0, 1))
    {
        m_pPipeline->RemoveRequestFence(&pFenceHandlerData->hFence, pFenceHandlerData->requestId);

        if ((FALSE == pOutputPort->flags.isSinkBuffer) || (TRUE == pOutputPort->flags.isSharedSinkBuffer))
        {
            if (TRUE == pFenceHandlerData->isDelayedBufferFence)
            {
                CAMX_LOG_DRQ("Reporting delayed buffer fence callback for Fence (%d), Node::%s : Type:%d, "
                             "pipeline: %d, sequenceId: %d, request: %llu PortId: %d",
                             static_cast<INT32>(pFenceHandlerData->hFence),
                             NodeIdentifierString(), Type(), GetPipelineId(),
                             pFenceHandlerData->pOutputBufferInfo[0].sequenceId,
                             pFenceHandlerData->requestId,
                             pOutputPort->portId);
            }
            else
            {
                CAMX_LOG_DRQ("Reporting non-sink fence callback for Fence (%d), Node::%s : Type:%d, "
                             "pipeline: %d, sequenceId: %d, request: %llu PortId: %d",
                             static_cast<INT32>(pFenceHandlerData->hFence),
                             NodeIdentifierString(), Type(), GetPipelineId(),
                             pFenceHandlerData->pOutputBufferInfo[0].sequenceId,
                             pFenceHandlerData->requestId,
                             pOutputPort->portId);
            }

            if (CSLFenceResultSuccess != pFenceHandlerData->fenceResult)
            {
                if (CSLFenceResultFailed == pFenceHandlerData->fenceResult)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore,
                                   "Fence error detected for Node::%s : Type:%d Port: %d SequenceId: %d"
                                   "requestId: %llu",
                                   NodeIdentifierString(), Type(),
                                   pOutputPort->portId,
                                   pFenceHandlerData->pOutputBufferInfo[0].sequenceId,
                                   pFenceHandlerData->requestId);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupCore,
                                   "Fence error detected during flush for Node::%s : Type:%d Port: %d SequenceId: %d"
                                   "requestId: %llu",
                                   NodeIdentifierString(), Type(),
                                   pOutputPort->portId,
                                   pFenceHandlerData->pOutputBufferInfo[0].sequenceId,
                                   pFenceHandlerData->requestId);
                }

                NodeFenceHandlerDataErrorSubset* pFenceHanderDataSubset = GetFenceErrorBuffer();

                pFenceHanderDataSubset->hFence      = pFenceHandlerData->hFence;
                pFenceHanderDataSubset->portId      = pOutputPort->portId;
                pFenceHanderDataSubset->requestId   = pFenceHandlerData->requestId;

                m_pPipeline->NonSinkPortFenceErrorSignaled(&pFenceHandlerData->hFence, pFenceHandlerData->requestId);
            }
            else
            {
                CAMX_LOG_DRQ("Fence %i signal detected for Node::%s Pipeline:%d requestId: %llu",
                             static_cast<INT32>(pFenceHandlerData->hFence),
                             NodeIdentifierString(), GetPipelineId(),
                             pFenceHandlerData->requestId);

                m_pPipeline->NonSinkPortFenceSignaled(&pFenceHandlerData->hFence, pFenceHandlerData->requestId);
            }
        }
        if (TRUE == pOutputPort->flags.isSinkBuffer)
        {
            for (UINT batchIndex = 0; batchIndex < numBatchedFrames; batchIndex++)
            {
                CAMX_LOG_DRQ("Reporting sink fence callback for Fence (%d) Node::%s port:%d, pipeline: %d, "
                             "sequenceId: %d, requestId: %llu",
                             static_cast<INT32>(pFenceHandlerData->hFence),
                             NodeIdentifierString(),
                             pOutputPort->portId,
                             GetPipelineId(),
                             pFenceHandlerData->pOutputBufferInfo[batchIndex].sequenceId,
                             pFenceHandlerData->requestId);

                if (CSLFenceResultSuccess != pFenceHandlerData->fenceResult)
                {
                    // Signal App that this request is an error
                    m_pPipeline->SinkPortFenceErrorSignaled(pOutputPort->sinkTargetStreamId,
                                                            pFenceHandlerData->pOutputBufferInfo[batchIndex].sequenceId,
                                                            pFenceHandlerData->requestId,
                                                            &pFenceHandlerData->pOutputBufferInfo[batchIndex].bufferInfo);
                }
                else
                {
                    m_pPipeline->SinkPortFenceSignaled(pOutputPort->sinkTargetStreamId,
                                                       pFenceHandlerData->pOutputBufferInfo[batchIndex].sequenceId,
                                                       pFenceHandlerData->requestId,
                                                       &pFenceHandlerData->pOutputBufferInfo[batchIndex].bufferInfo);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetupRequestOutputPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SetupRequestOutputPorts(
    PerBatchedFrameInfo* pPerBatchedFrameInfo)
{
    CamxResult             result            = CamxResultSuccess;
    UINT                   requestIdIndex    = (m_tRequestId % MaxRequestQueueDepth);
    PerRequestActivePorts* pRequestPorts     = &m_perRequestInfo[requestIdIndex].activePorts;

    pRequestPorts->numOutputPorts = 0;

    for (UINT portIndex = 0; portIndex < m_outputPortsData.numPorts; portIndex++)
    {
        if (TRUE == IsOutputPortEnabled(portIndex))
        {
            OutputPort& rOutputPort = m_outputPortsData.pOutputPorts[portIndex];

            if (rOutputPort.bufferProperties.maxImageBuffers > 0)
            {
                const UINT                numBatchedFrames               = GetBatchedHALOutputNum(
                                                                                rOutputPort.numBatchedFrames,
                                                                                rOutputPort.HALOutputBufferCombined);
                const UINT                maxImageBuffers                = rOutputPort.bufferProperties.maxImageBuffers;
                const UINT                fenceIndex                     = (m_tRequestId % maxImageBuffers);
                PerRequestOutputPortInfo* pRequestOutputPort             =
                    &m_perRequestInfo[requestIdIndex].activePorts.pOutputPorts[pRequestPorts->numOutputPorts];
                NodeFenceHandlerData*     pFenceHandlerData              =
                    &rOutputPort.pFenceHandlerData[fenceIndex];
                NodeFenceHandlerData*     pDelayedBufferFenceHandlerData =
                    &rOutputPort.pDelayedBufferFenceHandlerData[fenceIndex];

                pRequestOutputPort->portId           = rOutputPort.portId;
                pRequestOutputPort->numOutputBuffers = 0;

                pRequestOutputPort->flags.isBatch    = (numBatchedFrames > 1) ? TRUE : FALSE;
                pRequestOutputPort->flags.isLoopback = rOutputPort.flags.isLoopback;

                // Since nodes are walked by the topology in a random order, when some Node A requests its input port
                // information like input buffer/fences/etc from the parent node, and if the parent node is not called
                // by the topology yet, the parent node will create whatever is necessary to provide to the child node
                // that is calling it. Later when the parent node is called, it would have for e.g. create the fence for
                // this output port already
                /// @todo (CAMX-3119) remove IsTorchWidgetNode check below and handle this in generic way.
                if (((TRUE == IsSinkPortWithBuffer(portIndex)) || (TRUE == IsNonSinkHALBufferOutput(portIndex))) &&
                    (FALSE == IsTorchWidgetNode()))
                {
                    const UINT baseImageIndex     = ((m_tRequestId * numBatchedFrames) % maxImageBuffers);
                    const UINT outputPortStreamId = rOutputPort.sinkTargetStreamId;

                    pFenceHandlerData->numOutputBuffers = 0;

                    for (UINT batchIndex = 0; ((batchIndex < numBatchedFrames) && (CamxResultSuccess == result)); batchIndex++)
                    {
                        PerBatchedFrameInfo* const pBatchedFrameInfo  = &pPerBatchedFrameInfo[batchIndex];

                        // Is the output port enabled for the frame in the batch

                        if (TRUE == Utils::IsBitSet(pBatchedFrameInfo->activeStreamIdMask, outputPortStreamId))
                        {
                            UINT                outputBufferIndex   = 0;
                            CaptureRequest*     pCaptureRequest     = m_pPipeline->GetRequest(m_tRequestId);
                            StreamBufferInfo*   pStreamBufferInfo   = &(pCaptureRequest->pStreamBuffers[batchIndex]);

                            for (UINT32 i = 0; i < pStreamBufferInfo->numOutputBuffers; i++)
                            {
                                if (pStreamBufferInfo->outputBuffers[i].bufferInfo.phBuffer ==
                                    pBatchedFrameInfo->bufferInfo[outputPortStreamId].phBuffer)
                                {
                                    outputBufferIndex = i;
                                    break;
                                }
                            }

                            result = ProcessSinkPortNewRequest(m_tRequestId, batchIndex, &rOutputPort, &outputBufferIndex);

                            if (CamxResultSuccess == result)
                            {
                                ChiBufferInfo*                pChiBufferInfo          =
                                    &pBatchedFrameInfo->bufferInfo[outputPortStreamId];
                                FenceHandlerBufferInfo* const pFenceHandlerBufferInfo =
                                    &pFenceHandlerData->pOutputBufferInfo[pFenceHandlerData->numOutputBuffers];

                                const ImageFormat* pImageFormat = &rOutputPort.bufferProperties.imageFormat;
                                UINT32             flags        = CSLMemFlagHw;

                                switch (Type())
                                {
                                    case ChiExternalNode:
                                        // clear CSLMemFlagHW and set CSLMemFlagUMDAccess for ChiExternalNode
                                        flags &= ~CSLMemFlagHw;
                                        flags &= ~CSLMemFlagProtected;
                                        if (FALSE == IsSecureMode())
                                        {
                                            flags |= CSLMemFlagUMDAccess;
                                        }
                                        else
                                        {
                                            // This is based on assumption that - if any CHI node exists in secure mode,
                                            // it runs on DSP and so set the below flags while importing the HAL buffer.
                                            // 1. Set ProtectedUMD Access so that Node neither map the buffer to
                                            //    any camera HW nor to UMD.
                                            // 2. Set DSPSecureAccess to indicate this buffer will be accessed by DSP in secure
                                            //    mode. For sink buffers mapping, this flag is not used as of now.
                                            flags &= ~CSLMemFlagUMDAccess;
                                            flags |= CSLMemFlagProtectedUMDAccess;
                                            flags |= CSLMemFlagDSPSecureAccess;
                                        }
                                        break;
                                    case JPEGAggregator:
                                        /// @todo (CAMX-2106) Cannot blindly set UMD access,
                                        /// this breaks the ZSL implementation
                                        flags = CSLMemFlagUMDAccess;
                                        break;
                                    case FDManager:
                                        flags = CSLMemFlagUMDAccess;
                                        break;
                                    default:
                                        break;
                                }

                                // For non sink mode which need to map to HAL buffer, it need UMD access
                                if ((TRUE == rOutputPort.flags.isNonSinkHALBufferOutput) ||
                                    (TRUE == rOutputPort.flags.isSharedSinkBuffer))
                                {
                                    flags |= CSLMemFlagUMDAccess;
                                }

                                if ((TRUE == IsSecureMode()) && (ChiExternalNode != Type()))
                                {
                                    flags = (CSLMemFlagProtected | CSLMemFlagHw);
                                }

                                /// @todo (CAMX-1015) Calculate ImageFormatUtils::GetTotalSize only one and save it
                                const UINT frameIndex = (baseImageIndex + batchIndex);

                                m_pBufferRequestLock->Lock();

                                ImageBuffer* const pImageBuffer = rOutputPort.ppImageBuffers[frameIndex];
                                CAMX_ASSERT(NULL != pImageBuffer);
                                if (0 != flags)
                                {
                                    BOOL delayImport = FALSE;

                                    if (TRUE == DisableDelayedUnmapRequired(flags, pImageFormat))
                                    {
                                        flags |= CSLMemFlagDisableDelayedUnmap;
                                    }
                                    if (TRUE == DelayedMapRequired(flags, pImageFormat))
                                    {
                                        delayImport = TRUE;
                                    }

                                    result = pImageBuffer->Import(pImageFormat,
                                                              pChiBufferInfo,
                                                              0, // Offset
                                                              ImageFormatUtils::GetTotalSize(pImageFormat),
                                                              flags,
                                                              OutputPortDeviceIndices(portIndex),
                                                              OutputPortDeviceCount(portIndex),
                                                              delayImport,
                                                              TRUE);

                                }
                                m_pBufferRequestLock->Unlock();

                                /// @todo (CAMX-2106) If this assert fires it could be because of the UMDAccess flag and
                                ///                   then the app will stop working
                                CAMX_ASSERT(CamxResultSuccess == result);

                                if (CamxResultSuccess != result)
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupCore, "Import failed");
                                }

                                pFenceHandlerBufferInfo->pImageBuffer = pImageBuffer;
                                pFenceHandlerBufferInfo->sequenceId   = pPerBatchedFrameInfo[batchIndex].sequenceId;
                                pFenceHandlerBufferInfo->bufferInfo   = *pChiBufferInfo;

                                // Sent to the derived node
                                const UINT numOutputBuffers = pRequestOutputPort->numOutputBuffers;

                                pRequestOutputPort->ppImageBuffer[numOutputBuffers] = pImageBuffer;
                                pRequestOutputPort->phFence                         = &pFenceHandlerData->hFence;
                                pRequestOutputPort->pIsFenceSignaled                = &pFenceHandlerData->isFenceSignaled;

                                pRequestOutputPort->numOutputBuffers++;
                                pFenceHandlerData->numOutputBuffers++;
                            }
                        }
                    }

                    if (CamxResultSuccess == result)
                    {
                        if ((TRUE == pFenceHandlerData->primaryFence) || (TRUE == pFenceHandlerData->isChiFence))
                        {
                            m_perRequestInfo[requestIdIndex].numUnsignaledFences++;
                            m_perRequestInfo[requestIdIndex].numUnprocessedFences++;
                        }
                        pRequestOutputPort->flags.isOutputHALBuffer = TRUE;
                    }
                }
                else if (FALSE == IsTorchWidgetNode()) // Output port that does not output HAL buffer
                {
                    // For ports outputting non-HAL buffers only the first entry is ever needed
                    UINT sequenceId = pPerBatchedFrameInfo[0].sequenceId;

                    pRequestOutputPort->flags.isOutputHALBuffer = FALSE;
                    pRequestOutputPort->numOutputBuffers        = 1;

                    result = ProcessNonSinkPortNewRequest(m_tRequestId, sequenceId, &rOutputPort);

                    if (CamxResultSuccess == result)
                    {
                        // For ports outputting non-HAL buffers only the first entry is ever needed
                        // Data to be sent to the derived node that implements request processing
                        pRequestOutputPort->ppImageBuffer[0]         = pFenceHandlerData->pOutputBufferInfo[0].pImageBuffer;
                        pRequestOutputPort->phFence                  = &pFenceHandlerData->hFence;
                        pRequestOutputPort->pIsFenceSignaled         = &pFenceHandlerData->isFenceSignaled;
                        pRequestOutputPort->phDelayedBufferFence     = &pDelayedBufferFenceHandlerData->hFence;
                        pRequestOutputPort->pDelayedOutputBufferData = &pFenceHandlerData->delayedOutputBufferData;

                        if ((TRUE == pFenceHandlerData->primaryFence) || (TRUE == pFenceHandlerData->isChiFence))
                        {
                            m_perRequestInfo[requestIdIndex].numUnsignaledFences++;
                            m_perRequestInfo[requestIdIndex].numUnprocessedFences++;
                        }

                        if (TRUE == IsBypassableNode())
                        {
                            pRequestOutputPort->flags.isDelayedBuffer = TRUE;
                            // For bypassable node, a fence for buffer dependency is added.
                            // So, this needs to be accounted here
                            if ((TRUE == pDelayedBufferFenceHandlerData->primaryFence) ||
                                (TRUE == pFenceHandlerData->isChiFence))
                            {
                                m_perRequestInfo[requestIdIndex].numUnsignaledFences++;
                                m_perRequestInfo[requestIdIndex].numUnprocessedFences++;
                            }
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore,
                                       "No free output image buffers available for Node::%s port %d request %llu!!",
                                       NodeIdentifierString(),
                                       rOutputPort.portId,
                                       m_tRequestId);
                    }
                }
            }

            pRequestPorts->numOutputPorts++;
        }
    }

    CAMX_ASSERT(pRequestPorts->numOutputPorts > 0);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessEarlyMetadataDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::ProcessEarlyMetadataDone(
    UINT64 requestId)
{
    UINT pipelineId     = GetPipelineId();
    UINT nodeId         = Type();
    UINT nodeInstanceId = InstanceID();
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "Node::%s ProcessEarlyMetadataDone %llu", NodeIdentifierString(), requestId);

    // Early metadata doesn't care about all nodes being done. When this gets called it's gotten
    // everything it cares about and can skip the check that NotifyNodeMetadataDone normally does.
    m_pPipeline->ProcessMetadataRequestIdDone(requestId, TRUE);

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessMetadataDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::ProcessMetadataDone(
    UINT64 requestId)
{
    if (TRUE == CamxAtomicCompareExchangeU(&m_perRequestInfo[requestId % MaxRequestQueueDepth].metadataComplete, 0, 1))
    {
        UINT nodeId         = Type();
        UINT nodeInstanceId = InstanceID();
        auto hPipeline      = m_pPipeline->GetPipelineDescriptor();
        BINARY_LOG(LogEvent::Node_MetadataDone, hPipeline, nodeId, nodeInstanceId, requestId);

        CAMX_LOG_INFO(CamxLogGroupCore, "Node::%s : Type :%d ProcessMetadataDone %llu",
            NodeIdentifierString(), Type(), requestId);
        CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "Node::%s ProcessMetadataDone %llu", NodeIdentifierString(), requestId);

        m_pPipeline->NotifyNodeMetadataDone(requestId);

        CAMX_TRACE_SYNC_END(CamxLogGroupCore);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessPartialMetadataDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::ProcessPartialMetadataDone(
    UINT64 requestId)
{
    if ((TRUE == m_pPipeline->IsPartialMetadataEnabled()) &&
        (TRUE ==
         CamxAtomicCompareExchangeU(&m_perRequestInfo[requestId % MaxRequestQueueDepth].partialMetadataComplete, 0, 1)))
    {
        UINT nodeType       = Type();
        UINT nodeInstanceId = InstanceID();
        auto hPipeline      = m_pPipeline->GetPipelineDescriptor();
        BINARY_LOG(LogEvent::Node_PartialMetadataDone, hPipeline, nodeType, nodeInstanceId, requestId);

        CAMX_LOG_INFO(CamxLogGroupCore, "Node::%s Type:%d ProcessPartialMetadataDone %llu",
                      NodeIdentifierString(), Type(), requestId);
        CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "Node::%s Type:%d ProcessPartialMetadataDone %llu",
                                NodeIdentifierString(), Type(), requestId);

        m_pPipeline->NotifyNodePartialMetadataDone(requestId);

        CAMX_TRACE_SYNC_END(CamxLogGroupCore);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessRequestIdDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::ProcessRequestIdDone(
    UINT64 requestId)
{
    if (TRUE == CamxAtomicCompareExchangeU(&m_perRequestInfo[requestId % MaxRequestQueueDepth].requestComplete, 0, 1))
    {
        UINT nodeId         = Type();
        UINT nodeInstanceId = InstanceID();
        auto hPipeline      = m_pPipeline->GetPipelineDescriptor();
        BINARY_LOG(LogEvent::Node_RequestIdDone, hPipeline, nodeId, nodeInstanceId, requestId);
        CAMX_LOG_INFO(CamxLogGroupCore, "Node::%s : %d ProcessRequestIdDone %llu", NodeIdentifierString(), Type(), requestId);
        CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "Node::%s ProcessRequestIdDone %llu", NodeIdentifierString(), requestId);

        DumpDebugInternalNodeInfo(requestId, TRUE);

        RecycleRetiredCmdBuffers(requestId);

        /// @note It is assumed that if any output port is enabled, all the input ports are active i.e. all inputs
        ///       are required to generate any (and all) outputs

        UINT inputPortIndex;
        for (inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
        {
            InputPort* pInputPort                   = &m_inputPortsData.pInputPorts[inputPortIndex];
            UINT64     requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);

            // Do not release reference if the buffer is needed for future request
            if (requestIdOffsetFromLastFlush > pInputPort->bufferDelta)
            {
                if (FALSE == IsSourceBufferInputPort(inputPortIndex))
                {
                    pInputPort->pParentNode->NotifyOutputConsumed(pInputPort->parentOutputPortIndex,
                                                                  (requestId - pInputPort->bufferDelta));
                }
                else
                {
                    m_pFenceCreateReleaseLock->Lock();

                    UINT         requestIdIndex = (requestId - pInputPort->bufferDelta) % MaxRequestQueueDepth;
                    ImageBuffer* pImageBuffer   = pInputPort->ppImageBuffers[requestIdIndex];

                    CAMX_ASSERT(NULL != pImageBuffer);

                    // Don't immediately release the hal buffer as it can be used by child node
                    // when node is bypassed
                    if ((TRUE == pImageBuffer->HasBackingBuffer()) && (FALSE == IsBypassableNode()))
                    {
                        pImageBuffer->Release(FALSE);

                        CAMX_ASSERT(CSLInvalidHandle != pInputPort->phFences[requestIdIndex]);

                        /// @note: Code below may also release the CSL fences associated with
                        ///        CHI override feature(s) (e.g., MFNR). Thus when such flows
                        ///        are identified for further changes, the following code has
                        ///        to be modified to release (only those) CSL fences that are
                        ///        created from within the CAMX layer <--> CHI override takes
                        ///        care of releasing it's fences external to CAMX layer. Have
                        ///        a look at ReleaseOutputPortCSLFences() to get the idea.

                        ///@ todo (CAMX-4160) Handle input/Chi fence composite
                        if (FALSE == pInputPort->pFenceSourceFlags[requestIdIndex].isChiAssociatedFence)
                        {
                            CaptureRequest*   pCaptureRequest   = m_pPipeline->GetRequest(requestId);
                            StreamBufferInfo* pStreamBufferInfo = &(pCaptureRequest->pStreamBuffers[0]);

                            if (0 == CamxAtomicDecU(&pStreamBufferInfo->inputBufferInfo[pInputPort->inputBufferIndex]
                                .fenceRefCount))
                            {
                                CamxResult result = CSLReleaseFence(pInputPort->phFences[requestIdIndex]);

                                CAMX_LOG_DRQ("ReleaseFence(Input)...Node: %08x, Node::%s, fence: %08x(%d), "
                                             "ImgBuf:%p reqId:%llu result:%d",
                                             this,
                                             NodeIdentifierString(),
                                             &pInputPort->phFences[requestIdIndex],
                                             pInputPort->phFences[requestIdIndex],
                                             pInputPort->ppImageBuffers[requestIdIndex],
                                             m_tRequestId,
                                             result);

                                if (CamxResultSuccess != result)
                                {
                                    CAMX_LOG_ERROR(CamxLogGroupCore,
                                                 "ReleaseFence(Input) Failed Node: %08x, Node::%s, fence: %08x(%d), "
                                                 "ImgBuf:%p reqId:%llu result:%d",
                                                 this,
                                                 NodeIdentifierString(),
                                                 &pInputPort->phFences[requestIdIndex],
                                                 pInputPort->phFences[requestIdIndex],
                                                 pInputPort->ppImageBuffers[requestIdIndex],
                                                 m_tRequestId,
                                                 result);
                                }
                            }
                        }
                        else
                        {
                            CAMX_LOG_DRQ("Skip ReleaseFence(Input)...Node: %08x, Node::%s, fence: %08x(%d)"
                                         "(isChiAssociatedFence:%d), ImgBuf:%p reqId:%llu",
                                         this,
                                         NodeIdentifierString(),
                                         &pInputPort->phFences[requestIdIndex],
                                         pInputPort->phFences[requestIdIndex],
                                         pInputPort->pFenceSourceFlags[requestIdIndex].isChiAssociatedFence,
                                         pInputPort->ppImageBuffers[requestIdIndex],
                                         m_tRequestId);
                        }

                        pInputPort->phFences[requestIdIndex]                               = CSLInvalidHandle;
                        pInputPort->pIsFenceSignaled[requestIdIndex]                       = FALSE;
                        pInputPort->pFenceSourceFlags[requestIdIndex].isChiAssociatedFence = FALSE;
                    }
                    m_pFenceCreateReleaseLock->Unlock();
                }
            }
        }

        CAMX_LOG_DRQ("Pipeline[%s] Node %s, pipeline: %u, is done with request %llu",
                     m_pPipeline->GetPipelineName(),
                     NodeIdentifierString(),
                     GetPipelineId(),
                     requestId);

        m_pPipeline->NotifyNodeRequestIdDone(requestId);

        SetRequestStatus(requestId, PerRequestNodeStatus::Success);

        // Set the timestamp for end of node processing
        // We pass a dummy value for sequenceID because NodeStage::End is the end of the request
        SetNodeProcessingTime(requestId, 0, NodeStage::End);

        CAMX_TRACE_SYNC_END(CamxLogGroupCore);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetNodeProcessingTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::SetNodeProcessingTime(
    UINT64      requestId,
    INT32       sequenceID,
    NodeStage   stage)
{
    CamxTime pTime;
    UINT32   stageIndex     = static_cast<UINT32>(stage);
    UINT64   requestIdIndex = requestId % MaxRequestQueueDepth;
    UINT32   seqIdIndex;

    if ((TRUE == GetStaticSettings()->dumpNodeProcessingInfo) &&
        (TRUE == IsNodeEnabled()) &&
        (stageIndex < static_cast<UINT32>(NodeStage::MaxNodeStatuses)))
    {
        if (sequenceID <= MaxProcessSequenceIdForTiming - 2)
        {
            // This condition addresses a scenairo where the seqId is set specifically to -2, to handle this condition
            // we roll over the index to end of the array
            if (sequenceID == -2)
            {
                seqIdIndex = MaxProcessSequenceIdForTiming - 1;
            }
            else
            {
                seqIdIndex = static_cast<UINT>(sequenceID);
            }

            OsUtils::GetTime(&pTime);

            UINT32 milliSec = OsUtils::CamxTimeToMillis(&pTime);

            if (m_seqIdNodeStartTime == 0)
            {
                m_seqIdNodeStartTime = milliSec;
            }

            if (0 == m_perRequestInfo[requestIdIndex].nodeProcessingStages[seqIdIndex][stageIndex].timestamp)
            {
                m_perRequestInfo[requestIdIndex].nodeProcessingStages[seqIdIndex][stageIndex].timestamp =
                    milliSec;
            }

            // Running average calculations
            if ((NodeStage::Start != stage) && (NodeStage::End != stage))
            {
                m_responseTimeSum[seqIdIndex][stageIndex] += (milliSec -
                    m_perRequestInfo[requestIdIndex].nodeProcessingStages[seqIdIndex][stageIndex - 1].timestamp);
                m_numberOfNodeResponses[seqIdIndex][stageIndex]++;
            }

            // If we are setting end stage, node is done processing and we should dump
            if (NodeStage::End == stage)
            {
                DumpNodeProcessingTime(requestId, milliSec);
            }
        }
        else
        {
            // If this warning is seen, MaxProcessSequenceIdForTiming should be increased
            CAMX_LOG_WARN(CamxLogGroupCore,
                "Received a seq id greater than the max accounted, collected data may be inaccurate");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpNodeProcessingTime
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DumpNodeProcessingTime(
    UINT64 requestId,
    UINT32 milliSec)
{
    UINT32  requestIdIndex      = (requestId % MaxRequestQueueDepth);
    UINT32  startIndex          = static_cast<UINT32>(NodeStage::Start);
    UINT32  depMetIndex         = static_cast<UINT32>(NodeStage::DependenciesMet);
    UINT32  eprEndIndex         = static_cast<UINT32>(NodeStage::EPReqEnd);
    UINT32  endIndex            = static_cast<UINT32>(NodeStage::End);

    INT32   startOfReqTimestamp =
        static_cast<INT32>(m_perRequestInfo[requestIdIndex].nodeProcessingStages[0][startIndex].timestamp);

    INT32   endOfReqTimestamp =
        static_cast<INT32>(m_perRequestInfo[requestIdIndex].nodeProcessingStages[0][endIndex].timestamp);

    // If we didn't grab the start processing time, set node processing time as 0 instead of the end timestamp
    m_perRequestInfo[requestIdIndex].nodeProcessingTime = (0 == startOfReqTimestamp) ?
        0 : (endOfReqTimestamp - startOfReqTimestamp);

    CAMX_LOG_DUMP(CamxLogGroupCore, "Node[%s], ReqId[%llu], SyncId[%llu], TOOK:%d ms; "
        "[%s: %d ms | %s: %d ms]",
        NodeIdentifierString(), m_perRequestInfo[requestIdIndex].requestId,
        GetCSLSyncId(requestId), m_perRequestInfo[requestIdIndex].nodeProcessingTime,
        NodeStageStrings[startIndex], startOfReqTimestamp,
        NodeStageStrings[endIndex], endOfReqTimestamp);


    for (UINT32 seqIdIndex = 0; seqIdIndex < MaxProcessSequenceIdForTiming; seqIdIndex++)
    {
        INT32   startTimestamp =
            static_cast<INT32>(m_perRequestInfo[requestIdIndex].nodeProcessingStages[seqIdIndex][startIndex].timestamp);
        INT32   depMetTimestamp =
            static_cast<INT32>(m_perRequestInfo[requestIdIndex].nodeProcessingStages[seqIdIndex][depMetIndex].timestamp);
        INT32   epReqTimestamp =
            static_cast<INT32>(m_perRequestInfo[requestIdIndex].nodeProcessingStages[seqIdIndex][eprEndIndex].timestamp);

        // Check to see if indexed sequenceId is being used
        if (depMetTimestamp > 0)
        {
            INT32   startToDepMetDelta = (0 <= (depMetTimestamp - startTimestamp)) ? (depMetTimestamp - startTimestamp) : 0;
            INT32   depMetToEPREndDelta = (0 <= (epReqTimestamp - depMetTimestamp)) ? (epReqTimestamp - depMetTimestamp) : 0;
            INT32   printSeqId;

            // We utilize the last index in the array to account for the special condition of an -2 seqId
            // When printing we should print '-2' instead of this last index
            if ((MaxProcessSequenceIdForTiming - 1) == seqIdIndex)
            {
                printSeqId = -2;
            }
            else
            {
                printSeqId = static_cast<INT>(seqIdIndex);
            }

            CAMX_LOG_DUMP(CamxLogGroupCore, "    Node[%s], ReqId[%llu], SyncId[%llu], SequenceId[%d]; "
                "[%s(%d): 0 ms | %s(%d): %d ms | %s(%d): %d ms]",
                NodeIdentifierString(), m_perRequestInfo[requestIdIndex].requestId,
                GetCSLSyncId(requestId), printSeqId,
                NodeStageStrings[startIndex], startTimestamp,
                NodeStageStrings[depMetIndex], depMetTimestamp, startToDepMetDelta,
                NodeStageStrings[eprEndIndex], epReqTimestamp, depMetToEPREndDelta);
        }

    }
    // Dump node processing average every time we have populated entire m_perRequestInfo buffer
    if (0 == (requestId % MaxRequestQueueDepth))
    {
        DumpNodeProcessingAverage(requestId);
    }

    if (milliSec - m_seqIdNodeStartTime >= TimeToPrintSeqIdAverageInMs)
    {
        DumpSequenceIdProcessingAverage();
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpNodeProcessingAverage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DumpNodeProcessingAverage(
    UINT64 requestId)
{
    UINT32 currProcessingTime       = 0;
    UINT32 averageProcessingTime    = 0;
    UINT32 minProcessingTime        = UINT32_MAX;
    UINT32 maxProcessingTime        = 0;
    UINT32 divisor                  = MaxRequestQueueDepth;

    // Calculate the average, min and max processing times for the current MaxRequestQueueDepth requests
    for (UINT i = 0; i < MaxRequestQueueDepth; i++)
    {
        // Only average valid data from request that came before the request we called ProcessRequestIdDone
        if ((0 != m_perRequestInfo[i].requestId) && (requestId >= m_perRequestInfo[i].requestId))
        {
            currProcessingTime      = m_perRequestInfo[i].nodeProcessingTime;
            averageProcessingTime   += currProcessingTime;
            maxProcessingTime       = Utils::MaxUINT32(maxProcessingTime, currProcessingTime);
            minProcessingTime       = Utils::MinUINT32(minProcessingTime, currProcessingTime);
        }
        else
        {
            // If data wasn't valid, adjust how we calculate the average
            divisor--;
        }
    }

    averageProcessingTime = Utils::DivideAndCeil(averageProcessingTime, divisor);

    CAMX_LOG_PERF_INFO(CamxLogGroupCore, "Node[%s], Processing Times (ms): Avg[%d], Min[%d], Max[%d]",
                       NodeIdentifierString(), averageProcessingTime, minProcessingTime, maxProcessingTime);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpSequenceIdProcessingAverage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DumpSequenceIdProcessingAverage()
{
    UINT32  depMetIndex = static_cast<UINT32>(NodeStage::DependenciesMet);
    UINT32  eprEndIndex = static_cast<UINT32>(NodeStage::EPReqEnd);
    INT32   printSeqId;

    for (UINT sequenceId = 0; sequenceId < MaxProcessSequenceIdForTiming; sequenceId++)
    {
        printSeqId = sequenceId;

        // We utilize the last index in the array to account for the special condition of an -2 seqId
        // When printing we should print '-2' instead of this last index
        if ((MaxProcessSequenceIdForTiming - 1) == sequenceId)
        {
            printSeqId = -2;
        }

        if (m_responseTimeSum[sequenceId][depMetIndex] > 0)
        {
            CAMX_LOG_PERF_INFO(CamxLogGroupCore, "[SeqId Avg] Node[%s], SequenceId[%d]; "
                "Avg[%s: %f ms (%d samples) | %s: %f ms (%d samples) ]",
                NodeIdentifierString(), printSeqId,
                NodeStageStrings[depMetIndex],
                static_cast<FLOAT>(m_responseTimeSum[sequenceId][depMetIndex])
                    / static_cast<FLOAT>(m_numberOfNodeResponses[sequenceId][depMetIndex]),
                m_numberOfNodeResponses[sequenceId][depMetIndex],
                NodeStageStrings[eprEndIndex],
                static_cast<FLOAT>(m_responseTimeSum[sequenceId][eprEndIndex])
                    / static_cast<FLOAT>(m_numberOfNodeResponses[sequenceId][eprEndIndex]),
                m_numberOfNodeResponses[sequenceId][eprEndIndex]);
        }

        m_responseTimeSum[sequenceId][depMetIndex] = 0;
        m_responseTimeSum[sequenceId][eprEndIndex] = 0;
        m_numberOfNodeResponses[sequenceId][depMetIndex] = 0;
        m_numberOfNodeResponses[sequenceId][eprEndIndex] = 0;
    }

    m_seqIdNodeStartTime = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetDataCountFromPipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
SIZE_T Node::GetDataCountFromPipeline(
    const UINT   id,
    INT64        offset,
    UINT         pipelineID,
    BOOL         allowSticky)
{
    MetadataPool* pInputPool    = NULL;
    MetadataPool* pMainPool     = NULL;

    if (pipelineID == GetPipeline()->GetPipelineId())
    {
        pInputPool    = m_pInputPool;
        pMainPool     = m_pMainPool;
    }
    else
    {
        pInputPool    = GetIntraPipelinePerFramePool(PoolType::PerFrameInput, pipelineID);
        pMainPool     = GetIntraPipelinePerFramePool(PoolType::PerFrameResult, pipelineID);

        CAMX_ASSERT((NULL != pInputPool) && (NULL != pMainPool));
    }

    static const UINT InputProperty = ((static_cast<UINT32>(PropertyGroup::Result) << 16) | InputMetadataSectionMask);

    INT64         intReq = static_cast<INT64>(m_tRequestId);
    UINT64        index  = (intReq - offset <= 0) ? FirstValidRequestId : intReq - offset;
    MetadataSlot* pSlot  = NULL;
    SIZE_T        count  = 0;
    UINT          tagId = id & ~DriverInternalGroupMask;

    switch (id & DriverInternalGroupMask)
    {
        case 0:
            pSlot = pMainPool->GetSlot(index);
            pSlot->ReadLock();
            if (TRUE == pSlot->IsPublished(UnitType::Metadata, tagId))
            {
                count = pSlot->GetMetadataCountByTag(tagId, allowSticky);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Attempting to get metadata count for tag %08x when unpublished",
                               NodeIdentifierString(), id);
            }
            pSlot->Unlock();
            break;
        case InputMetadataSectionMask:
            // Input always published
            count = pInputPool->GetSlot(index)->GetMetadataCountByTag(tagId, allowSticky);
            break;
        case UsecaseMetadataSectionMask:
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Reading tag count for usecase pool tag %08x not supported",
                           NodeIdentifierString(), id);
            break;
        case StaticMetadataSectionMask:
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Reading tag count for static pool tag %08x not supported",
                           NodeIdentifierString(), id);
            break;
        case InputProperty:
        case static_cast<UINT32>(PropertyGroup::Result) << 16:
        case static_cast<UINT32>(PropertyGroup::Internal) << 16:
        case static_cast<UINT32>(PropertyGroup::Usecase) << 16:
        case static_cast<UINT32>(PropertyGroup::DebugData) << 16:
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Reading tag count for properties not supported. %08x",
                           NodeIdentifierString(), id);
            break;
        default:
            break;
    }

    return count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetDataListFromMetadataPool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetDataListFromMetadataPool(
    const UINT*  pDataList,
    VOID** const ppData,
    UINT64*      pOffsets,
    SIZE_T       length,
    BOOL*        pNegate,
    UINT         pipelineID,
    UINT32       cameraId)
{
    CamxResult              result          = CamxResultSuccess;
    const StaticSettings*   pSettings       = HwEnvironment::GetInstance()->GetStaticSettings();

    MetadataPool* pInputPool    = NULL;
    MetadataPool* pMainPool     = NULL;
    MetadataPool* pInternalPool = NULL;
    MetadataPool* pUsecasePool  = NULL;

    if (pipelineID == GetPipeline()->GetPipelineId())
    {
        pInputPool    = m_pInputPool;
        pMainPool     = m_pMainPool;
        pInternalPool = m_pInternalPool;
        pUsecasePool  = m_pUsecasePool;
    }
    else
    {
        pInputPool    = GetIntraPipelinePerFramePool(PoolType::PerFrameInput, pipelineID);
        pMainPool     = GetIntraPipelinePerFramePool(PoolType::PerFrameResult, pipelineID);
        pInternalPool = GetIntraPipelinePerFramePool(PoolType::PerFrameInternal, pipelineID);
        pUsecasePool  = GetIntraPipelinePerFramePool(PoolType::PerUsecase, pipelineID);

        CAMX_ASSERT(NULL != pInputPool    &&
                    NULL != pMainPool     &&
                    NULL != pInternalPool &&
                    NULL != pUsecasePool);
    }

    static const UINT InputProperty = ((static_cast<UINT32>(PropertyGroup::Result) << 16) | InputMetadataSectionMask);

    for (SIZE_T i = 0; i < length; i++)
    {
        UINT32 group                        = (pDataList[i] & DriverInternalGroupMask);
        VOID*  pBlob                        = NULL;
        INT64  offset                       = ((NULL != pNegate) && (TRUE == pNegate[i])) ?
                                                (-1 * static_cast<INT64>(pOffsets[i])) : pOffsets[i];
        INT64  intReq                       = static_cast<INT64>(m_tRequestId);
        UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(intReq);
        UINT64 index                        = (static_cast<INT64>(requestIdOffsetFromLastFlush) - offset <= 0) ?
            (GetLastFlushedRequestId() + 1) : (intReq - offset);

        BOOL useCurrentReqId = (((0 == group) ||
            ((static_cast<UINT32>(PropertyGroup::Result) << 16) == group) ||
            ((static_cast<UINT32>(PropertyGroup::Internal) << 16) == group) ||
            ((static_cast<UINT32>(PropertyGroup::DebugData) << 16) == group)) &&
            ((requestIdOffsetFromLastFlush <= m_pPipeline->GetMaxPipelineDelay()) || (TRUE == GetFlushStatus()))) ?
            TRUE : FALSE;

        if ((group != UsecaseMetadataSectionMask) &&
            (group != StaticMetadataSectionMask) &&
            (group != static_cast<UINT32>(PropertyGroup::Usecase) << 16) &&
            (group != InputMetadataSectionMask))
        {
            // Unfortunate race condition that can lead to some properties coming from the requested request and
            // some from others In general, if a request is in Error state it doesnt matter what we give back,
            //  so really only providing a hopefully non-null pointer
            if ((TRUE == m_pPipeline->RequestInErrorState(index)) && (FALSE == useCurrentReqId))
            {
                CAMX_LOG_INFO(CamxLogGroupCore, "Node::%s request: %llu marked as an error and attempting to find "
                    "valid metadata, group: %u original requestId: %llu requestIdOffsetFromLastFlush: %llu flush %d",
                    NodeIdentifierString(),
                    index,
                    group,
                    m_tRequestId,
                    requestIdOffsetFromLastFlush,
                    GetFlushStatus());

                UINT j;
                // adjust Get to point to a prior valid request
                for (j = 1; j <= RequestQueueDepth; j++)
                {
                    // If condition is met, means we will attempt to check/shift the request to 0 which is an invalid
                    // request so trigger recovery and mark as failure
                    if (0 == (index - j))
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Request: %llu marked as an error and attempting to shift to"
                                      "Request 0, triggering recovery!", NodeIdentifierString(), index);

                        ppData[i]   = NULL;
                        result      = CamxResultEFailed;

                        if (FALSE == GetFlushStatus())
                        {
                            m_pPipeline->SendRecoveryError(index);
                        }
                        break;
                    }

                    if (FALSE == m_pPipeline->RequestInErrorState(index - j))
                    {
                        CAMX_LOG_INFO(CamxLogGroupCore,
                                      "Node::%s Requesting data from a request marked as an error."
                                      "Shifting from requestId: %lld to %lld",
                                      NodeIdentifierString(), index, index - j);
                        index = (index - j);
                        break;
                    }

                    if (j == RequestQueueDepth)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Couldn't find a valid request for data. This is really bad"
                                       "...using %lld", NodeIdentifierString(), index);
                        result = CamxResultEFailed;
                        if (FALSE == GetFlushStatus())
                        {
                            m_pPipeline->SendRecoveryError(index);
                        }
                    }
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            MetadataSlot* pSlot = NULL;

            switch (group)
            {
                case 0:
                    pSlot = pMainPool->GetSlot(index);
                    ppData[i] = pSlot->GetMetadataByTag(pDataList[i], NodeIdentifierString());
                    break;
                case InputMetadataSectionMask:
                    // Input always published
                    if (InvalidCameraId == cameraId)
                    {
                        ppData[i] = pInputPool->GetSlot(index)->GetMetadataByTag(
                            pDataList[i] & ~DriverInternalGroupMask, NodeIdentifierString());
                    }
                    else
                    {
                        ppData[i] = pInputPool->GetSlot(index)->GetMetadataByCameraId(
                            pDataList[i] & ~DriverInternalGroupMask, cameraId, NodeIdentifierString());
                    }
                    break;
                case UsecaseMetadataSectionMask:
                    pSlot = pUsecasePool->GetSlot(index);
                    ppData[i] = pSlot->GetMetadataByTag((pDataList[i] & ~DriverInternalGroupMask), NodeIdentifierString());
                    break;
                case StaticMetadataSectionMask:
                    pSlot = m_pChiContext->GetStaticMetadataPool(GetPipeline()->GetCameraId())->GetSlot(index);
                    ppData[i] = pSlot->GetMetadataByTag(pDataList[i] & ~DriverInternalGroupMask, NodeIdentifierString());
                    break;
                case InputProperty:
                    pSlot = pInputPool->GetSlot(index);
                    if (InvalidCameraId == cameraId)
                    {
                        ppData[i] = pSlot->GetMetadataByTag(pDataList[i] & ~InputMetadataSectionMask, NodeIdentifierString());
                    }
                    else
                    {
                        ppData[i] = pSlot->GetMetadataByCameraId(pDataList[i] & ~InputMetadataSectionMask,
                                                                 cameraId, NodeIdentifierString());
                    }
                    break;
                case static_cast<UINT32>(PropertyGroup::Result) << 16:
                    CAMX_ASSERT(PropertyIDPerFrameResultEnd >= pDataList[i]);
                    pSlot = pMainPool->GetSlot(index);

                    // Results that are unpublished can be accessed based on the pipeline delay
                    ppData[i] = pSlot->GetMetadataByTag(pDataList[i], NodeIdentifierString());
                    if (NULL == ppData[i])
                    {
                        CAMX_LOG_INFO(CamxLogGroupCore, "Main Property %08x Node::%s Not Published for Request %llu %s",
                                      pDataList[i],
                                      HAL3MetadataUtil::GetPropertyName(pDataList[i]),
                                      index,
                                      NodeIdentifierString());
                    }
                    break;
                case static_cast<UINT32>(PropertyGroup::Internal) << 16:
                    CAMX_ASSERT(PropertyIDPerFrameInternalEnd >= pDataList[i]);
                    pSlot = pInternalPool->GetSlot(index);
                    if (NULL != pSlot)
                    {
                        if (TRUE == pSlot->IsPublished(UnitType::Property, pDataList[i]))
                        {
                            pSlot->GetPropertyBlob(&pBlob);
                            CAMX_ASSERT(NULL != pBlob);
                            ppData[i] = (pBlob == NULL) ? NULL :
                                Utils::VoidPtrInc(pBlob, InternalPropertyOffsets[pDataList[i] & ~DriverInternalGroupMask]);
                        }
                        else
                        {
                            ppData[i] = NULL;
                            CAMX_LOG_INFO(CamxLogGroupCore,
                                          "Internal Property %08x Node::%s Not Published for Request %llu %s",
                                          pDataList[i],
                                          HAL3MetadataUtil::GetPropertyName(pDataList[i]),
                                          index,
                                          NodeIdentifierString());
                        }
                    }
                    break;
                case static_cast<UINT32>(PropertyGroup::Usecase) << 16:
                    CAMX_ASSERT(PropertyIDUsecaseEnd >= pDataList[i]);
                    pSlot = pUsecasePool->GetSlot(0);
                    if (TRUE == pSlot->IsPublished(UnitType::Property, pDataList[i]))
                    {
                        ppData[i] = pSlot->GetMetadataByTag(pDataList[i], NodeIdentifierString());
                    }
                    else
                    {
                        CAMX_LOG_INFO(CamxLogGroupCore, "Usecase Property %08x Node::%s Not Published for Request %llu %s",
                                      pDataList[i],
                                      HAL3MetadataUtil::GetPropertyName(pDataList[i]),
                                      index,
                                      NodeIdentifierString());
                    }
                    break;
                case static_cast<UINT32>(PropertyGroup::DebugData) << 16:
                    CAMX_ASSERT(PropertyIDPerFrameDebugDataEnd >= pDataList[i]);
                    pSlot = m_pDebugDataPool->GetSlot(index);
                    if (NULL != pSlot)
                    {
                        pSlot->GetPropertyBlob(&pBlob);
                        ppData[i] =
                            Utils::VoidPtrInc(pBlob, DebugDataPropertyOffsets[pDataList[i] & ~DriverInternalGroupMask]);
                    }
                    break;
                default:
                    break;
            }

            if (TRUE == pSettings->logMetaEnable)
            {
                if (NULL != pSlot)
                {
                    CAMX_LOG_META("Node READ: Node::%s, pipeline: %08x, request: %lld, offset: %d",
                                  NodeIdentifierString(), GetPipeline()->GetPipelineId(), intReq, offset);
                    pSlot->PrintTagData(pDataList[i]);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetSensorModeRes0Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetSensorModeRes0Data(
    const SensorMode**    ppSensorData)
{
    CamxResult                    result       = CamxResultSuccess;
    MetadataSlot*                 pCurrentSlot = m_pUsecasePool->GetSlot(0);
    const UsecaseSensorModes*     pUsecaseSensorModes;

    CAMX_ASSERT(NULL != ppSensorData);

    pUsecaseSensorModes = static_cast<UsecaseSensorModes*>(pCurrentSlot->GetMetadataByTag(
        PropertyIDUsecaseSensorModes, NodeIdentifierString()));

    if (NULL != pUsecaseSensorModes)
    {
        *ppSensorData = &pUsecaseSensorModes->allModes[0];
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetSensorModeData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetSensorModeData(
    const SensorMode**    ppSensorData)
{
    CamxResult              result              = CamxResultSuccess;
    MetadataSlot*           pUsecaseSlot        = m_pUsecasePool->GetSlot(0);
    UINT                    currentMode         = MaxSensorModes;
    UsecaseSensorModes*     pUsecaseSensorModes;
    UINT*                   pCurrentSensorMode;

    CAMX_ASSERT(NULL != ppSensorData);

    pCurrentSensorMode = static_cast<UINT*>(pUsecaseSlot->GetMetadataByTag(
        PropertyIDUsecaseSensorCurrentMode, NodeIdentifierString()));
    pUsecaseSensorModes = static_cast<UsecaseSensorModes*>(pUsecaseSlot->GetMetadataByTag(
        PropertyIDUsecaseSensorModes, NodeIdentifierString()));

    if (NULL != pCurrentSensorMode)
    {
        currentMode = *pCurrentSensorMode;
        // Initial Sensor mode can be non 0 as this can be set via CreateSession
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "initialSensorMode=%d", currentMode);
    }

    static const UINT GetProps[] =
    {
        PropertyIDSensorCurrentMode,
    };

    static const UINT GetPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*             pData[GetPropsLength]   = { 0 };
    UINT64            offsets[GetPropsLength] = { 0 };
    GetDataList(GetProps, pData, offsets, GetPropsLength);
    if (NULL != pData[0])
    {
        currentMode = *reinterpret_cast<UINT32*>(pData[0]);
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "currentSensorMode=%d", currentMode);
    }

    if ((currentMode > MaxSensorModes) && (NULL != pCurrentSensorMode))
    {
        currentMode = *pCurrentSensorMode;
        CAMX_LOG_INFO(CamxLogGroupCore, "currentMode beyond the MaxSensorModes : %d", currentMode);
    }

    if ((MaxSensorModes > currentMode) && (NULL != pUsecaseSensorModes))
    {
        *ppSensorData = &pUsecaseSensorModes->allModes[currentMode];
    }
    else
    {
        result = CamxResultENoSuch;
        CAMX_LOG_INFO(CamxLogGroupCore, "Cannot get input resolution for Node name Node::%s, currentMode: %d",
                      NodeIdentifierString(), currentMode);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetCameraConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetCameraConfiguration(
    const CameraConfigurationInformation** ppCameraConfigInfo)
{
    CamxResult              result = CamxResultSuccess;
    MetadataSlot*           pCurrentSlot = m_pUsecasePool->GetSlot(0);

    CAMX_ASSERT(NULL != ppCameraConfigInfo);

    *ppCameraConfigInfo = static_cast<CameraConfigurationInformation*>(pCurrentSlot->GetMetadataByTag(
        PropertyIDUsecaseCameraModuleInfo, NodeIdentifierString()));
    if (NULL == *ppCameraConfigInfo)
    {
        result = CamxResultENoSuch;
        CAMX_LOG_WARN(CamxLogGroupCore, "Cannot get camera config for Node name Node::%s", NodeIdentifierString());
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetIFEInputResolution
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetIFEInputResolution(
    const IFEInputResolution** ppIFEInput)
{
    CamxResult              result       = CamxResultSuccess;
    MetadataSlot*           pCurrentSlot = m_pUsecasePool->GetSlot(0);

    CAMX_ASSERT(NULL != ppIFEInput);
    CAMX_ASSERT(pCurrentSlot->IsPublished(UnitType::Property, PropertyIDUsecaseIFEInputResolution));

    *ppIFEInput = static_cast<IFEInputResolution*>(pCurrentSlot->GetMetadataByTag(
        PropertyIDUsecaseIFEInputResolution, NodeIdentifierString()));
    if (NULL != *ppIFEInput)
    {
        CAMX_LOG_INFO(CamxLogGroupHWL, "Node::%s dims %ux%u", NodeIdentifierString(), (*ppIFEInput)->resolution.width,
            (*ppIFEInput)->resolution.height);
    }
    else
    {
        result = CamxResultENoSuch;
        CAMX_LOG_WARN(CamxLogGroupCore, "Cannot get input res for Node::%s", NodeIdentifierString());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetCSIDBinningInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::GetCSIDBinningInfo(
    CSIDBinningInfo* pCSIDBinningInfo)
{
    CamxResult          result                      = CamxResultSuccess;
    CSIDBinningInfo*    pCSIDVendortTagBinningInfo  = NULL;
    MetadataSlot*       pCurrentSlot                = m_pUsecasePool->GetSlot(0);
    UINT32              tagId;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.ifecsidconfig", "csidbinninginfo", &tagId);
    if (CamxResultSuccess == result)
    {
        pCSIDVendortTagBinningInfo =
            static_cast<CSIDBinningInfo*>(pCurrentSlot->GetMetadataByTag(tagId, NodeIdentifierString()));
        if (NULL != pCSIDVendortTagBinningInfo)
        {
            pCSIDBinningInfo->isBinningEnabled  = pCSIDVendortTagBinningInfo->isBinningEnabled;
            pCSIDBinningInfo->binningMode       = static_cast<CSIDBinningMode>(pCSIDVendortTagBinningInfo->binningMode);
        }
        else
        {
            pCSIDBinningInfo->isBinningEnabled = FALSE;
        }
    }

    // There is no current end-to-end usecase to validate CSID binning, for HW CSID binning validation use static settings
    if (TRUE == GetStaticSettings()->enableCSIDBinning)
    {
        pCSIDBinningInfo->isBinningEnabled  = GetStaticSettings()->enableCSIDBinning;
        pCSIDBinningInfo->binningMode       = static_cast<CSIDBinningMode>(GetStaticSettings()->CSIDBinningMode);
        CAMX_LOG_CONFIG(CamxLogGroupISP, "Node::%s BySetting Binning:isEnabled:%d, Mode:%d (VendorTag overwrited)",
                        NodeIdentifierString(),
                        pCSIDBinningInfo->isBinningEnabled,
                        pCSIDBinningInfo->binningMode);
    }

    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetPreviewDimension
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetPreviewDimension(
    CamxDimension* pPreviewDimension)
{
    CamxResult                        result          = CamxResultSuccess;
    PropertyPipelineOutputDimensions* pPipelineOutput = NULL;
    MetadataPool*                     pPerUsecasePool = m_pPipeline->GetPerFramePool(PoolType::PerUsecase);
    MetadataSlot*                     pCurrentSlot    = pPerUsecasePool->GetSlot(0);

    CAMX_ASSERT(NULL != pPreviewDimension);

    pPipelineOutput = static_cast<PropertyPipelineOutputDimensions*>(
        pCurrentSlot->GetMetadataByTag(PropertyIDUsecasePipelineOutputDimensions, NodeIdentifierString()));

    if (NULL != pPipelineOutput)
    {
        for (UINT i = 0; i < pPipelineOutput->numberOutputs; i++)
        {
            if (ChiPipelineOutputPreview == pPipelineOutput->dimensions[i].outputType)
            {
                CHIBUFFERDIMENSION outputDimension = pPipelineOutput->dimensions[i].outputDimension;

                pPreviewDimension->width           = outputDimension.width;
                pPreviewDimension->height          = outputDimension.height;
                CAMX_LOG_VERBOSE(CamxLogGroupCore, "Preview dimension w %d h %d, type %d",
                                 pPreviewDimension->width,
                                 pPreviewDimension->height,
                                 pPipelineOutput->dimensions[i].outputType);
                break;
            }
        }
    }
    else
    {
        result = CamxResultENoSuch;
        CAMX_LOG_WARN(CamxLogGroupCore, "Cannot get pipeline dims for Node::%s", NodeIdentifierString());
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::WritePreviousData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::WritePreviousData(
    UINT32 dataId,
    UINT   size)
{
    VOID*  pData      = NULL;
    UINT64 offsets[1] = {1};

    GetDataList(&dataId, &pData, &offsets[0], 1);
    // NOWHINE CP036a: Casting pointer to constant
    WriteDataList(&dataId, const_cast<const VOID**>(&pData), &size, 1);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::WriteData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE CamxResult Node::WriteData(
    UINT64      requestId,
    UINT32      dataId,
    SIZE_T      size,
    const VOID* pData)
{
    const StaticSettings* pSettings = HwEnvironment::GetInstance()->GetStaticSettings();

    MetadataSlot* pSlot = NULL;
    UINT32        group = (dataId & DriverInternalGroupMask);
    UINT32        tagId = (dataId & ~DriverInternalGroupMask);
    VOID*         pBlob = NULL;
    VOID*         pPoolData = NULL;

    switch (group)
    {
        case 0:
            // Special tags get to be sent back early
            EarlyReturnTag(requestId, dataId, pData, size);

            pSlot = m_pMainPool->GetSlot(requestId);
            if ((NULL != pData) && (0 != size))
            {
                pSlot->SetMetadataByTag(dataId, pData, size, NodeIdentifierString());
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupMeta, "NULL tag %x data %p size %d", dataId, pData, size);
            }

            pSlot->PublishMetadata(dataId, NodeIdentifierString());
            break;
        case InputMetadataSectionMask:
            CAMX_ASSERT_ALWAYS(); // Cant write Input
            break;
        case UsecaseMetadataSectionMask:
            pSlot = m_pUsecasePool->GetSlot(requestId);
            if ((NULL != pData) && (0 != size))
            {
                pSlot->SetMetadataByTag(tagId, pData, size, NodeIdentifierString());
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupMeta, "NULL usecase tag %x data %p size %d", tagId, pData, size);
            }
            pSlot->PublishMetadata(tagId, NodeIdentifierString());
            break;
        case StaticMetadataSectionMask:
            if ((NULL != pData) && (0 != size))
            {
                m_pChiContext->GetStaticMetadataPool(GetPipeline()->GetCameraId())
                    ->GetSlot(requestId)
                    ->SetMetadataByTag(tagId, pData, size, NodeIdentifierString());
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupMeta, "NULL static tag %x data %p size %d", dataId, pData, size);
            }
            break;
        case static_cast<UINT32>(PropertyGroup::Result) << 16:
            CAMX_ASSERT(PropertyIDPerFrameResultEnd >= dataId);
            pSlot = m_pMainPool->GetSlot(requestId);
            // Node complete properties don't have data to update, just publish
            if (FALSE == ((PropertyIDNodeComplete0 <= dataId) && ((PropertyIDNodeComplete0 + NodeCompleteCount) > dataId)))
            {
                if ((NULL != pData) && (0 != size))
                {
                    pSlot->SetMetadataByTag(dataId, pData, size, NodeIdentifierString());
                }
                else
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupMeta, "NULL result tag %x data %p size %d", dataId, pData, size);
                }
            }
            pSlot->PublishMetadata(dataId, NodeIdentifierString());
            break;
        case static_cast<UINT32>(PropertyGroup::Internal) << 16:
            CAMX_ASSERT(PropertyIDPerFrameInternalEnd >= dataId);
            pSlot = m_pInternalPool->GetSlot(requestId);
            pSlot->GetPropertyBlob(&pBlob);
            CAMX_ASSERT(NULL != pBlob);
            pPoolData = Utils::VoidPtrInc(pBlob, InternalPropertyOffsets[dataId & ~DriverInternalGroupMask]);
            break;
        case static_cast<UINT32>(PropertyGroup::Usecase) << 16:
            CAMX_ASSERT(PropertyIDUsecaseEnd >= dataId);
            if ((NULL != pData) || (0 != size))
            {
                pSlot = m_pUsecasePool->GetSlot(requestId);
                pSlot->SetMetadataByTag(dataId, pData, size, NodeIdentifierString());
                pSlot->PublishMetadata(dataId, NodeIdentifierString());
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupMeta, "NULL usecase tag %x data %p size %d", dataId, pData, size);
            }
            break;
        default:
            break;
    }

    if ((NULL != pPoolData) && (NULL != pData))
    {
        pSlot->WriteLock();
        Utils::Memcpy(pPoolData, pData, size);
        pSlot->Unlock();
        pSlot->PublishMetadata(dataId, NodeIdentifierString());
    }

    if (TRUE == pSettings->logMetaEnable)
    {
        if (NULL != pSlot)
        {
            CAMX_LOG_META("Node::%s WRITE:, pipeline: %08x, request: %lld",
                          NodeIdentifierString(), GetPipeline()->GetPipelineId(), requestId);
            pSlot->PrintTagData(dataId);
        }
    }


    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetPerFramePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataPool* Node::GetPerFramePool(
    PoolType poolType)
{
    MetadataPool* pPool = NULL;

    switch (poolType)
    {
        case PoolType::PerFrameInput:
        case PoolType::PerFrameResult:
        case PoolType::PerFrameInternal:
        case PoolType::PerUsecase:
            pPool = m_pPipeline->GetPerFramePool(poolType);
            break;

        case PoolType::PerFrameDebugData:
            pPool = m_pDebugDataPool;
            break;

        default:
            CAMX_ASSERT_ALWAYS_MESSAGE("Invalid per frame pool");
            break;
    }

    return pPool;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::WriteDataList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::WriteDataList(
    const UINT*  pDataList,
    const VOID** ppData,
    const UINT*  pDataSize,
    SIZE_T       length)
{
    CamxResult result = CamxResultSuccess;

    for (SIZE_T index = 0; index < length; ++index)
    {
        result = WriteData(m_tRequestId, pDataList[index], pDataSize[index], ppData[index]);
        CheckAndUpdatePublishedPartialList(pDataList[index]);
    }

    if (NULL != m_pDRQ)
    {
        // Majority of write occur for result pool, so performing no detection of result updates
        // Taking over dispatch for tags/props so it occurs for batches just once instead of once per update
        m_pDRQ->DispatchReadyNodes();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::CheckAndUpdatePublishedPartialList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::CheckAndUpdatePublishedPartialList(
    const UINT tagID)
{
    // If node is publishing partial Tags then only we need to execute this function
    if ( 0 < m_publishTagArray.partialTagCount)
    {
        UINT requestIdIndex = m_tRequestId % MaxRequestQueueDepth;

        // If we have already got all of the partial metadata then we need not check again
        if (m_perRequestInfo[requestIdIndex].partialPublishedSet.size() < m_publishTagArray.partialTagCount)
        {
            // We need to iterate through the array to check if its partial.
            // We can optimize this by using a Set to have O(1) operation,
            // drawback is there will be a redundant data structure having similar info
            for (UINT i = 0; i < m_publishTagArray.partialTagCount; i++)
            {
                if (m_publishTagArray.tagArray[i] == tagID)
                {
                    m_perRequestInfo[requestIdIndex].partialPublishedSet.insert(tagID);
                    break;
                }
            }
        }
    }
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::InputPortIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Node::InputPortIndex(
    UINT portId)
{
    UINT portIndex = 0;
    UINT port = 0;

    for (port = 0; port < m_inputPortsData.numPorts; port++)
    {
        if (m_inputPortsData.pInputPorts[port].portId == portId)
        {
            portIndex = port;
            break;
        }
    }

    CAMX_ASSERT(port < m_inputPortsData.numPorts);

    return portIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::OutputPortIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Node::OutputPortIndex(
    UINT portId)
{
    UINT portIndex = InvalidPortIndex;
    UINT port      = 0;

    for (port = 0; port < m_outputPortsData.numPorts; port++)
    {
        if (m_outputPortsData.pOutputPorts[port].portId == portId)
        {
            portIndex = port;
            break;
        }
    }

    if (InvalidPortIndex == portIndex)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node[%s] portId %u is not active",
            NodeIdentifierString(), portId);
    }

    CAMX_ASSERT(port < m_outputPortsData.numPorts);

    return portIndex;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetInputLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::SetInputLink(
    UINT  nodeInputLinkIndex,
    UINT  nodeInputPortId,
    Node* pParentNode,
    UINT  parentPortId)
{
    UINT parentOutputPortIndex = pParentNode->OutputPortIndex(parentPortId);

    m_inputPortsData.pInputPorts[nodeInputLinkIndex].portId                = nodeInputPortId;
    m_inputPortsData.pInputPorts[nodeInputLinkIndex].parentOutputPortIndex = parentOutputPortIndex;
    m_inputPortsData.pInputPorts[nodeInputLinkIndex].pParentNode           = pParentNode;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetUpLoopBackPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::SetUpLoopBackPorts(
    UINT  inputPortIndex)
{
    InputPort*           pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];

    // If the node is the same as ports parent node, its a loopback port
    if (this == m_inputPortsData.pInputPorts[inputPortIndex].pParentNode)
    {
        pInputPort->flags.isLoopback = TRUE;
        pInputPort->bufferDelta = 1;

        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[pInputPort->parentOutputPortIndex];
        OutputPortNegotiationData* pOutputPortNegotiationData =
            &m_bufferNegotiationData.pOutputPortNegotiationData[pInputPort->parentOutputPortIndex];
        pOutputPort->flags.isLoopback                         = TRUE;
        pOutputPort->enabledInStreamMask                      = ((1 << MaxNumStreams) - 1);

        /// @note If an output port has a loopback destination (input port), it can be its only destination
        CAMX_ASSERT(0 == pOutputPortNegotiationData->numInputPortsNotification);
        CAMX_ASSERT(1 == pOutputPort->numInputPortsConnected);

        pOutputPortNegotiationData->numInputPortsNotification++;

        if (pOutputPortNegotiationData->numInputPortsNotification == pOutputPort->numInputPortsConnected)
        {
            // When all the input ports connected to the output port have notified the output port, it means the
            // output port has all the buffer requirements it needs to make a decision for the buffer on that output
            // port
            m_bufferNegotiationData.numOutputPortsNotified++;
        }
        m_bHasLoopBackPorts = TRUE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetupSourcePort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::SetupSourcePort(
    UINT  nodeInputLinkIndex,
    UINT  nodeInputPortId)
{
    m_inputPortsData.pInputPorts[nodeInputLinkIndex].portId = nodeInputPortId;
    m_inputPortsData.pInputPorts[nodeInputLinkIndex].parentOutputPortIndex = 0;
    m_inputPortsData.pInputPorts[nodeInputLinkIndex].pParentNode = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DisableInputOutputLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DisableInputOutputLink(
    UINT  nodeInputIndex,
    BOOL  removeIndices)
{
    if (FALSE == m_inputPortsData.pInputPorts[nodeInputIndex].portDisabled)
    {
        if (FALSE == m_inputPortsData.pInputPorts[nodeInputIndex].flags.isSourceBuffer)
        {
            Node* pParentNode = m_inputPortsData.pInputPorts[nodeInputIndex].pParentNode;
            CAMX_ASSERT(NULL != pParentNode);

            CAMX_LOG_INFO(CamxLogGroupHWL, "nodeInputIndex %d Parent Node::%s, parent o/p portID %d",
                          nodeInputIndex, pParentNode->NodeIdentifierString(),
                          m_inputPortsData.pInputPorts[nodeInputIndex].parentOutputPortIndex);

            pParentNode->RemoveOutputDeviceIndices(m_inputPortsData.pInputPorts[nodeInputIndex].parentOutputPortIndex,
                                                   m_deviceIndices, m_deviceIndexCount, removeIndices);
        }
        // Disable the input port
        m_inputPortsData.pInputPorts[nodeInputIndex].portDisabled = TRUE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::EnableInputOutputLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::EnableInputOutputLink(
    UINT  nodeInputIndex)
{
    if (TRUE == m_inputPortsData.pInputPorts[nodeInputIndex].portDisabled)
    {
        Node* pParentNode = m_inputPortsData.pInputPorts[nodeInputIndex].pParentNode;
        CAMX_ASSERT(NULL != pParentNode);

        CAMX_LOG_INFO(CamxLogGroupHWL, "nodeInputIndex %d Parent Node::%s, parent o/p portID %d",
                      nodeInputIndex, pParentNode->NodeIdentifierString(),
                      m_inputPortsData.pInputPorts[nodeInputIndex].parentOutputPortIndex);

        pParentNode->AddOutputDeviceIndices(m_inputPortsData.pInputPorts[nodeInputIndex].parentOutputPortIndex,
            m_deviceIndices, m_deviceIndexCount);
        // Enable the input port
        m_inputPortsData.pInputPorts[nodeInputIndex].portDisabled = FALSE;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetAllInputPortIds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::GetAllInputPortIds(
    UINT* pNumPorts,
    UINT* pPortIds)
{
    CAMX_ASSERT(NULL != pNumPorts);
    CAMX_ASSERT(NULL != pPortIds);

    if ((NULL != pNumPorts) && (NULL != pPortIds))
    {
        *pNumPorts = 0;
        for (UINT i = 0; i < m_inputPortsData.numPorts; i++)
        {
            // Adding only enabled input ports into the array as node input port can get disabled dynamically,
            // this helps to get correct mapping of port index to port id for querying port buffer properties
            if (FALSE == m_inputPortsData.pInputPorts[i].portDisabled)
            {
                pPortIds[*pNumPorts] = m_inputPortsData.pInputPorts[i].portId;
                *pNumPorts = *pNumPorts + 1;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetAllOutputPortIds
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::GetAllOutputPortIds(
    UINT* pNumPorts,
    UINT* pPortIds)
{
    CAMX_ASSERT(NULL != pNumPorts);
    CAMX_ASSERT(NULL != pPortIds);

    if ((NULL != pNumPorts) && (NULL != pPortIds))
    {
        *pNumPorts = 0;
        for (UINT i = 0; i < m_outputPortsData.numPorts; i++)
        {
            if (m_outputPortsData.pOutputPorts[i].numInputPortsConnected >
                m_outputPortsData.pOutputPorts[i].numInputPortsDisabled)
            {
                pPortIds[*pNumPorts] = m_outputPortsData.pOutputPorts[i].portId;
                *pNumPorts = *pNumPorts + 1;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::RemovePerRequestInputPortLinkForOutputPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::RemovePerRequestInputPortLinkForOutputPort(
    UINT portIndex,
    UINT64 requestId)
{
    PerRequestOutputPortInfo* pPort = &m_perRequestInfo[requestId % MaxRequestQueueDepth].activePorts.pOutputPorts[portIndex];
    pPort->numPerReqInputPortsDisabled++;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::RemoveOutputDeviceIndices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::RemoveOutputDeviceIndices(
    UINT         portIndex,
    const INT32* pDeviceIndices,
    UINT         deviceIndexCount,
    BOOL         removeIndices)
{
    OutputPort* pPort = &(m_outputPortsData.pOutputPorts[portIndex]);
    INT32       newDeviceIndices[CamxMaxDeviceIndex];
    UINT        newDeviceIndexCount = 0;
    UINT        i;
    UINT        j;
    UINT        indicesRemoved = 0;

    if (TRUE == removeIndices)
    {
        for (i = 0; i < pPort->deviceCount; i++)
        {
            for (j = 0; j < deviceIndexCount; j++)
            {
                if (pDeviceIndices[j] == pPort->deviceIndices[i])
                {
                    break;
                }
            }
            if (j == deviceIndexCount)
            {
                newDeviceIndices[newDeviceIndexCount++] = pPort->deviceIndices[i];
            }
            else
            {
                indicesRemoved++;
            }
        }

        if (0 < indicesRemoved)
        {
            for (i = 0; i < newDeviceIndexCount; i++)
            {
                pPort->deviceIndices[i] = newDeviceIndices[i];
            }
            pPort->deviceCount = newDeviceIndexCount;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node::%s No device indice found for removal", NodeIdentifierString());
        }
    }
    pPort->numInputPortsDisabled++;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::IsPipelineStreamedOn
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Node::IsPipelineStreamedOn()
{
    return m_pPipeline->IsStreamedOn();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ResetSecureMode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::ResetSecureMode(
    UINT32 portId)
{
    UINT  portIndex = OutputPortIndex(portId);

    m_outputPortsData.pOutputPorts[portIndex].flags.isSecurePort = FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::CSLFenceCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::CSLFenceCallback(
    VOID*           pNodePrivateFenceData,
    CSLFence        hSyncFence,
    CSLFenceResult  fenceResult)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(pNodePrivateFenceData != NULL);
    FenceCallbackData*    pFenceCallbackData    = static_cast<FenceCallbackData*>(pNodePrivateFenceData);
    Node*                 pNode                 = pFenceCallbackData->pNode;
    NodeFenceHandlerData* pNodeFenceHandlerData = NULL;
    NodeFenceHandlerData& rNodeFenceHandlerData = *static_cast<NodeFenceHandlerData*>(pFenceCallbackData->pNodePrivateData);
    OutputPort*           pOutputPort           = NULL;

    if (NULL == pNode)
    {
        BINARY_LOG(LogEvent::FenceCB_UnkownNode, hSyncFence, pNode, fenceResult);
        CAMX_LOG_ERROR(CamxLogGroupCore, "Fence %d signaled on NULL node reference", hSyncFence);
        return;
    }

    auto&  rNodeName    = *reinterpret_cast<const CHAR(*)[MaxStringLength256]>(pNode->NodeIdentifierString());
    UINT   outputPortID = rNodeFenceHandlerData.pOutputPort->portId;
    UINT64 requestID    = rNodeFenceHandlerData.requestId;

    CHIFENCEHANDLE*   phReleaseFence    = NULL;
    CaptureRequest*   pCaptureRequest   = pNode->m_pPipeline->GetRequest(requestID);
    StreamBufferInfo* pStreamBufferInfo = NULL;
    ChiStreamBuffer*  pOutputBuffer     = NULL;
    const UINT        batchIndex        = 0;
    UINT              requestIdIndex    = requestID % MaxRequestQueueDepth;

    if (pNode->m_perRequestInfo[requestIdIndex].numUnsignaledFences >= 1)
    {
        pNode->m_perRequestInfo[requestIdIndex].numUnsignaledFences--;
    }

    pStreamBufferInfo = &(pCaptureRequest->pStreamBuffers[batchIndex]);
    for (UINT bufIdx = 0; bufIdx < pStreamBufferInfo->numOutputBuffers; bufIdx++)
    {
        pOutputBuffer = &(pStreamBufferInfo->outputBuffers[bufIdx]);

        if ((pOutputBuffer->bufferInfo.phBuffer == rNodeFenceHandlerData.pOutputBufferInfo->bufferInfo.phBuffer) &&
            (TRUE == pOutputBuffer->releaseFence.valid) &&
            (ChiFenceTypeInternal == pOutputBuffer->releaseFence.type))
        {
            phReleaseFence = &(pOutputBuffer->releaseFence.hChiFence);
            if (TRUE == IsValidCHIFence(phReleaseFence))
            {
                pNode->m_pChiContext->SetChiFenceResult(*phReleaseFence, fenceResult);
            }
        }
    }

    pNodeFenceHandlerData = static_cast<NodeFenceHandlerData*>(pFenceCallbackData->pNodePrivateData);
    pOutputPort           = pNodeFenceHandlerData->pOutputPort; // Output port to which the fence belongs to

    /// @todo (CAMX-493) Handle returned signal status code properly and deal with implications in the node processing and DRQ
    if (CSLFenceResultSuccess != fenceResult)
    {
        if (FALSE == pNode->GetFlushStatus())
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                           "Node::%s : Type %d portId=%u Fence %d signaled with failure %d in node fence handler FOR %llu",
                           rNodeName,
                           pNode->Type(), outputPortID,
                           hSyncFence,
                           fenceResult,
                           requestID);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupCore,
                          "Node::%s : Type %d portId=%u Fence %d signaled in flush state %d in node fence handler FOR %llu",
                          rNodeName,
                          pNode->Type(), outputPortID,
                          hSyncFence,
                          fenceResult,
                          rNodeFenceHandlerData.requestId);
            fenceResult = CSLFenceResultCanceled;
        }
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupCore,
                      "Node::%s : Type %d portId=%u Fence %d signaled with success in node fence handler FOR %llu",
                      rNodeName,
                      pNode->Type(), pOutputPort->portId,
                      hSyncFence,
                      rNodeFenceHandlerData.requestId);
    }

    CAMX_ASSERT(NULL != pNode);

    pNodeFenceHandlerData->fenceResult = fenceResult;

    VOID* pData[] = { pFenceCallbackData, NULL };

    BOOL fenceCBProcessedSuccessfully = FALSE;
    ///@ todo (CAMX-3203) Verify fenceResult is handled in the job handlers
    if ((NULL             != pNode->GetThreadManager()) &&
        (InvalidJobHandle != pNode->GetJobFamilyHandle()))
    {
        result = pNode->GetThreadManager()->PostJob(pNode->GetJobFamilyHandle(), NULL, &pData[0], FALSE, FALSE);
        if (CamxResultSuccess == result)
        {
            fenceCBProcessedSuccessfully = TRUE;
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node::%s: Type:%d Fence %d signaled with success in node fence handler",
                             rNodeName, pNode->Type(), hSyncFence);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s : Type:%d Fence %d handler failed in node fence handler",
                           rNodeName, pNode->Type(), hSyncFence);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node:[%s] : Type:%d Fence %d signaled with invalid or retired fence handlers",
                       rNodeName, pNode->Type(), hSyncFence);
    }

    CAMX_ASSERT(CamxResultSuccess == result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::NodeThreadJobFamilyCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* Node::NodeThreadJobFamilyCb(
    VOID* pCbData)
{
    CAMX_ASSERT(NULL != pCbData);

    FenceCallbackData* pFenceCallbackData = static_cast<FenceCallbackData*>(pCbData);

    CAMX_ASSERT(pFenceCallbackData->pNode != NULL);

    NodeFenceHandlerData* pNodeFenceHandlerData = static_cast<NodeFenceHandlerData*>(pFenceCallbackData->pNodePrivateData);

    pFenceCallbackData->pNode->ProcessFenceCallback(pNodeFenceHandlerData);

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessFenceCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::ProcessFenceCallback(
    NodeFenceHandlerData* pFenceHandlerData)
{
    CAMX_ASSERT(NULL != pFenceHandlerData);

    OutputPort* pOutputPort    = pFenceHandlerData->pOutputPort; // Output port to which the fence belongs to
    UINT64      requestId      = pFenceHandlerData->requestId;
    UINT        requestIdIndex = requestId % MaxRequestQueueDepth;
    m_tRequestId               = requestId;

    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore,
                            "Node::%s ProcessFenceCallback RequestId %llu Port %u, Fence 0x%08x",
                            NodeIdentifierString(),
                            requestId,
                            pOutputPort->portId,
                            pFenceHandlerData->hFence);

    CAMX_LOG_VERBOSE(CamxLogGroupCore,
                     "Node::%s ProcessFenceCallback RequestId %llu Port %u, type 0x%x, Fence 0x%08x",
                     NodeIdentifierString(),
                     requestId,
                     pOutputPort->portId,
                     Type(),
                     pFenceHandlerData->hFence);

#if CAMX_CONTINGENCY_INDUCER_ENABLE
    // Contingency Inducer will check if fence need drop/mark as error for debug purpose
    FenceDropActionReturnType fenceAction = FenceDropActionReturnType::ACTION_NONE;
    if (NULL != m_pContingencyInducer)
    {
        fenceAction = m_pContingencyInducer->CheckFenceDropNeeded(m_nodeFlags.isRealTime,
                                                                  GetRequestIdOffsetFromLastFlush(requestId),
                                                                  pOutputPort->portId);
    }

    if (FenceDropActionReturnType::ACTION_DROP == fenceAction)
    {
        return;
    }
    else if (FenceDropActionReturnType::ACTION_ERROR == fenceAction)
    {
        pFenceHandlerData->fenceResult = CSLFenceResultFailed;
    }

#endif // CAMX_CONTINGENCY_INDUCER_ENABLE

    // Only do processing if we havent already signalled the fence (for failure cases)
    if (TRUE == CamxAtomicCompareExchangeU(&pFenceHandlerData->isFenceSignaled, 0, 1))
    {
        auto                  hPipeline         = m_pPipeline->GetPipelineDescriptor();
        UINT                  nodeId            = Type();
        UINT                  nodeInstanceId    = InstanceID();
        UINT                  outputPortId      = pOutputPort->portId;
        NodeFenceHandlerData& rFenceHandlerData = *pFenceHandlerData;
        BINARY_LOG(LogEvent::FenceCB_Processed, hPipeline, nodeId, nodeInstanceId, outputPortId, rFenceHandlerData);

        m_pPipeline->RemoveRequestFence(&pFenceHandlerData->hFence, requestId);

        ProcessingNodeFenceCallback(pFenceHandlerData, requestId, outputPortId);

        if (m_perRequestInfo[requestIdIndex].numUnprocessedFences >= 1)
        {
            m_perRequestInfo[requestIdIndex].numUnprocessedFences--;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "NumUnprocessedFences number decrease to below zero for portID:%d %s",
                outputPortId,
                NodeIdentifierString());
        }

        NotifyRequestProcessingError(pFenceHandlerData, m_perRequestInfo[requestIdIndex].numUnprocessedFences);

        CAMX_LOG_VERBOSE(CamxLogGroupCore, "node %s, NumUnprocessedFences %d, requestId %llu",
                       m_pNodeName,
                       m_perRequestInfo[requestIdIndex].numUnprocessedFences,
                       requestId);

        WatermarkImage(pFenceHandlerData);
        DumpData(pFenceHandlerData);

        if (0 == m_perRequestInfo[requestIdIndex].numUnprocessedFences)
        {
            // When all the output fences are signalled for the current node, notify parent node first that the node processing
            // is finished and then inform DRQ about this fence signal. This will help in releasing the current node's
            // input buffers (parent node's output ImageBuffers) first before signalling DRQ. If late binding is enabled,
            // next node in the pipeline starts processing after fence is signalled. This will help in re-using current node's
            // input buffer as next node's  output buffer (if other buffer properties match).
            ProcessRequestIdDone(requestId);
        }

        m_pBufferRequestLock->Lock();

        const UINT numBatchedFrames = (TRUE == pOutputPort->flags.isSinkBuffer) ?
            GetBatchedHALOutputNum(pOutputPort->numBatchedFrames, pOutputPort->HALOutputBufferCombined) : 1;

        // Remove ref count for secondary buffers
        // Composite fences must be removed before releasing primary fences, as there may be a case when primary port
        // fence count becomes 0 and primary port fence is invalidated. In such case, composite port fence won't be
        // released as composite port fence are identified based on primary fence.
        ReleaseBufferManagerCompositeImageReference(pFenceHandlerData->hFence, requestId);

        // All HAL Buffers create their own ImageBuffer
        // In the case where this port is a NonSinkHALBufferOutput,
        // We hold onto ours until our child calls ReleaseOutputPortImageBufferReference on us
        if (FALSE == pOutputPort->flags.isNonSinkHALBufferOutput && FALSE == pFenceHandlerData->isDelayedBufferFence)
        {
            for (UINT batchIndex = 0; batchIndex < numBatchedFrames; batchIndex++)
            {
                ReleaseOutputPortImageBufferReference(pOutputPort, pFenceHandlerData->requestId, batchIndex);
            }
        }
        m_pBufferRequestLock->Unlock();

        if ((FALSE == pOutputPort->flags.isSinkBuffer) || (TRUE == pOutputPort->flags.isSharedSinkBuffer))
        {
            if (TRUE == pFenceHandlerData->isDelayedBufferFence)
            {
                CAMX_LOG_DRQ("Reporting delayed buffer fence callback for Fence (0x%08x), Node::%s : Type:%d, "
                             "pipeline: %d, sequenceId: %d, request: %llu PortId: %d",
                             static_cast<INT32>(pFenceHandlerData->hFence),
                             NodeIdentifierString(), Type(), GetPipelineId(),
                             pFenceHandlerData->pOutputBufferInfo[0].sequenceId,
                             pFenceHandlerData->requestId,
                             outputPortId);
            }
            else
            {
                CAMX_LOG_DRQ("Reporting non-sink fence callback for Fence (0x%08x), Node::%s : Type:%d, "
                             "pipeline: %d, sequenceId: %d, request: %llu PortId: %d",
                             static_cast<INT32>(pFenceHandlerData->hFence),
                             NodeIdentifierString(), Type(), GetPipelineId(),
                             pFenceHandlerData->pOutputBufferInfo[0].sequenceId,
                             pFenceHandlerData->requestId,
                             outputPortId);
            }

            if (CSLFenceResultSuccess != pFenceHandlerData->fenceResult)
            {
                if (CSLFenceResultFailed == pFenceHandlerData->fenceResult)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore,
                        "Fence error detected for %s: Node: %s Type: %u InstanceID: %u  Port: %u SequenceId: %u"
                        " requestId: %lld",
                        m_pPipeline->GetPipelineName(), NodeIdentifierString(), Type(), InstanceID(),
                        outputPortId,
                        pFenceHandlerData->pOutputBufferInfo[0].sequenceId,
                        pFenceHandlerData->requestId);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupCore,
                        "Fence Signalled in flush for %s: Node: %s Type: %u InstanceID: %u  Port: %u SequenceId: %u"
                        " requestId: %lld",
                        m_pPipeline->GetPipelineName(), NodeIdentifierString(), Type(), InstanceID(),
                        outputPortId,
                        pFenceHandlerData->pOutputBufferInfo[0].sequenceId,
                        pFenceHandlerData->requestId);
                }

                NodeFenceHandlerDataErrorSubset* pFenceHanderDataSubset = GetFenceErrorBuffer();

                pFenceHanderDataSubset->hFence      = pFenceHandlerData->hFence;
                pFenceHanderDataSubset->portId      = outputPortId;
                pFenceHanderDataSubset->requestId   = pFenceHandlerData->requestId;

                m_pPipeline->NonSinkPortFenceErrorSignaled(&pFenceHandlerData->hFence, pFenceHandlerData->requestId);
            }
            else
            {
                CAMX_LOG_DRQ("Fence %i signal detected for Node::%s Pipeline:%d requestId: %llu",
                             static_cast<INT32>(pFenceHandlerData->hFence),
                             NodeIdentifierString(), GetPipelineId(),
                             pFenceHandlerData->requestId);

                m_pPipeline->NonSinkPortFenceSignaled(&pFenceHandlerData->hFence, pFenceHandlerData->requestId);
            }
        }
        if (TRUE == pOutputPort->flags.isSinkBuffer)
        {
            for (UINT batchIndex = 0; batchIndex < numBatchedFrames; batchIndex++)
            {
                CAMX_LOG_DRQ("Reporting sink fence callback for Fence (%d) Node::%s port:%d, pipeline: %d, "
                             "sequenceId: %d, requestId: %llu",
                             static_cast<INT32>(pFenceHandlerData->hFence),
                             NodeIdentifierString(),
                             outputPortId,
                             GetPipelineId(),
                             pFenceHandlerData->pOutputBufferInfo[batchIndex].sequenceId,
                             pFenceHandlerData->requestId);

                if (CSLFenceResultSuccess != pFenceHandlerData->fenceResult)
                {
                    // Signal App that this request is an error
                    m_pPipeline->SinkPortFenceErrorSignaled(pOutputPort->sinkTargetStreamId,
                                                            pFenceHandlerData->pOutputBufferInfo[batchIndex].sequenceId,
                                                            pFenceHandlerData->requestId,
                                                            &pFenceHandlerData->pOutputBufferInfo[batchIndex].bufferInfo);
                }
                else
                {
                    m_pPipeline->SinkPortFenceSignaled(pOutputPort->sinkTargetStreamId,
                                                       pFenceHandlerData->pOutputBufferInfo[batchIndex].sequenceId,
                                                       pFenceHandlerData->requestId,
                                                       &pFenceHandlerData->pOutputBufferInfo[batchIndex].bufferInfo);
                }
            }
        }


        if (0 == m_perRequestInfo[requestIdIndex].numUnprocessedFences)
        {
            ProcessPartialMetadataDone(requestId);
            ProcessMetadataDone(requestId);
            ProcessRequestIdDone(requestId);
        }
        else
        {
            CAMX_LOG_DRQ("Node::%s : Pipeline:%d waiting for fenceCount %d for requestID: %llu",
                         NodeIdentifierString(), GetPipelineId(), m_perRequestInfo[requestIdIndex].numUnprocessedFences,
                         requestId);
        }
    }

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DumpData(
    NodeFenceHandlerData* pFenceHandlerData)
{
    const StaticSettings* pSettings        = HwEnvironment::GetInstance()->GetStaticSettings();
    OutputPort*           pOutputPort      = pFenceHandlerData->pOutputPort; // Output port to which the fence belongs to
    UINT                  numBatchedFrames = GetBatchedHALOutputNum(pOutputPort->numBatchedFrames,
                                                                    pOutputPort->HALOutputBufferCombined);
    UINT64                requestId        = pFenceHandlerData->requestId;

    // Get the dump metadata
    CamxResult  result;
    UINT32      metaTag            = 0;
    UINT        debugTag[2]        = { 0 };
    VOID*       pData[2]           = { 0 };
    UINT32      requestIdIndex     = requestId % MaxRequestQueueDepth;

    result = VendorTagManager::QueryVendorTagLocation(
                                   "org.codeaurora.qcamera3.hal_private_data",
                                   "debug_image_dump",
                                   &metaTag);
    CAMX_ASSERT(CamxResultSuccess == result);

    debugTag[0] = (metaTag | InputMetadataSectionMask);
    result = VendorTagManager::QueryVendorTagLocation(
                                   "org.codeaurora.qcamera3.hal_private_data",
                                   "debug_image_dump_mask",
                                   &metaTag);
    debugTag[1] = (metaTag | InputMetadataSectionMask);
    CAMX_ASSERT(CamxResultSuccess == result);

    if (TRUE == pSettings->dynamicImageDump)
    {
        MetadataSlot* pSlot = m_pMainPool->GetSlot(requestIdIndex);

        pData[0] = pSlot->GetMetadataByTag(debugTag[0], NodeIdentifierString());
        pData[1] = pSlot->GetMetadataByTag(debugTag[1], NodeIdentifierString());
    }

    if ((TRUE == pSettings->autoImageDump)                                   ||
        ((TRUE == pSettings->dynamicImageDump) &&
         ((NULL != pData[0]) && (TRUE == *(static_cast<UINT8*>(pData[0]))))) ||
        ((TRUE == pSettings->dynamicImageDump) &&
         (TRUE == pSettings->dynamicImageDumpTrigger)))
    {
        CAMX_ASSERT((FALSE == pOutputPort->flags.isSinkBuffer) || (numBatchedFrames == 1));

        UINT32 enabledNodeForOutputDumpMask      = pSettings->autoImageDumpMask;
        UINT32 enabledNodeForInputDumpMask       = pSettings->autoInputImageDumpMask;

        UINT32 enableInputPortMask  = 0xFFFFFFFF;
        UINT32 enableOutputPortMask = 0xFFFFFFFF;
        UINT32 enableInstanceMask   = 0xFFFFFFFF;

        // HFR/Batch mode at max can reach 16 frames (or 960 fps , with sensor 60 fps stream)
        UINT32 enableOutputBatchNum = MAX_IFE_FRAMES_PER_BATCH;

        BOOL   dumpOutput            = FALSE;
        BOOL   dumpInput             = FALSE;

        // dumpInfo initialization common to input and output dumps.
        ImageDumpInfo dumpInfo = { 0 };
        CHAR pipelineName[256] = { 0 };

        OsUtils::SNPrintF(pipelineName, sizeof(pipelineName), "%s", GetPipelineName());
        dumpInfo.pPipelineName    = pipelineName;
        dumpInfo.pNodeName        = Name();
        dumpInfo.nodeInstance     = m_instanceId;
        dumpInfo.requestId        = static_cast<UINT32>(pFenceHandlerData->requestId);

        // If we are dumping because the per-request dump VT is true, then use the mask in the request.
        if ((TRUE == pSettings->dynamicImageDump) &&
            ((NULL != pData[0]) && (TRUE == *(static_cast<UINT8*>(pData[0])))))
        {
            enabledNodeForOutputDumpMask = *(static_cast<UINT8*>(pData[0]));
        }

        /// @todo (CAMX-2875) Need a good way to do comparisons with HWL node types.  Most of the things we have under
        ///                   the HWL aren't really HW specific anyway.
        switch (m_nodeType)
        {
            case IFE: // IFE
                dumpOutput           = (FALSE == (enabledNodeForOutputDumpMask & ImageDumpIFE))? FALSE: TRUE;
                enableOutputPortMask = pSettings->autoImageDumpIFEoutputPortMask;
                enableInstanceMask   = pSettings->autoImageDumpIFEInstanceMask;
                // In HFR/Batch mode: By default we dump one IFE buffer per batch.
                // Tune the number of buffers to be dumped with below static settings.
                enableOutputBatchNum = pSettings->autoImageDumpIFEoutputBatchNum;
                break;
            case JPEG: // JPEG
                dumpOutput           = (FALSE == (enabledNodeForOutputDumpMask & ImageDumpJPEG))? FALSE: TRUE;
                enableOutputPortMask = pSettings->autoImageDumpJpegHwoutputPortMask;
                enableInstanceMask   = pSettings->autoImageDumpJpegHwInstanceMask;
                break;
            case IPE: // IPE
                dumpOutput           = (FALSE == (enabledNodeForOutputDumpMask & ImageDumpIPE))? FALSE: TRUE;
                enableOutputPortMask = pSettings->autoImageDumpIPEoutputPortMask;
                enableInstanceMask   = pSettings->autoImageDumpIPEInstanceMask;
                dumpInput            = (FALSE == (enabledNodeForInputDumpMask  & ImageDumpIPE))? FALSE: TRUE;
                enableInputPortMask  = pSettings->autoImageDumpIPEinputPortMask;
                break;
            case BPS: // BPS
                dumpOutput           = (FALSE == (enabledNodeForOutputDumpMask & ImageDumpBPS))? FALSE: TRUE;
                enableOutputPortMask = pSettings->autoImageDumpBPSoutputPortMask;
                enableInstanceMask   = pSettings->autoImageDumpBPSInstanceMask;
                dumpInput            = (FALSE == (enabledNodeForInputDumpMask  & ImageDumpBPS))? FALSE: TRUE;
                break;
            case FDHw: // FDHw
                dumpOutput           = (FALSE == (enabledNodeForOutputDumpMask & ImageDumpFDHw))? FALSE: TRUE;
                enableOutputPortMask = pSettings->autoImageDumpFDHwoutputPortMask;
                enableInstanceMask   = pSettings->autoImageDumpFDHwInstanceMask;
                dumpInput            = (FALSE == (enabledNodeForInputDumpMask  & ImageDumpFDHw))? FALSE: TRUE;
                break;
            case LRME:  // LRME
                dumpOutput           = (FALSE == (enabledNodeForOutputDumpMask & ImageDumpLRME))? FALSE: TRUE;
                enableOutputPortMask = pSettings->autoImageDumpLRMEoutputPortMask;
                enableInstanceMask   = pSettings->autoImageDumpLRMEInstanceMask;
                break;
            case RANSAC:  // RANSAC
                dumpOutput           = (FALSE == (enabledNodeForOutputDumpMask & ImageDumpRANSAC))? FALSE: TRUE;
                enableOutputPortMask = pSettings->autoImageDumpRANSACoutputPortMask;
                enableInstanceMask   = pSettings->autoImageDumpRANSACInstanceMask;
                break;
            case ChiExternalNode: // ChiExternalNode
                dumpOutput           = (FALSE == (enabledNodeForOutputDumpMask & ImageDumpChiNode))? FALSE: TRUE;
                enableOutputPortMask = pSettings->autoImageDumpCHINodeoutputPortMask;
                enableInstanceMask   = pSettings->autoImageDumpCHINodeInstanceMask;
                dumpInput            = (FALSE == (enabledNodeForInputDumpMask  & ImageDumpChiNode))? FALSE: TRUE;
                break;
            case CVP:   // CVP
                dumpOutput           = (FALSE == (enabledNodeForOutputDumpMask & ImageDumpCVP))? FALSE: TRUE;
                enableOutputPortMask = pSettings->autoImageDumpCVPoutputPortMask;
                enableInstanceMask   = pSettings->autoImageDumpCVPInstanceMask;
                break;
            default:
                dumpOutput           = (FALSE == (enabledNodeForOutputDumpMask & ImageDumpMisc))? FALSE: TRUE;
                dumpInput            = (FALSE == (enabledNodeForInputDumpMask  & ImageDumpMisc))? FALSE: TRUE;
                break;
        }

        if ((TRUE == dumpOutput) && (TRUE == pSettings->offlineImageDumpOnly))
        {
            if (TRUE == IsRealTime())
            {
                dumpOutput = FALSE;
            }
        }

        // Dump input ports.
        if ((TRUE == pSettings->dumpInputatOutput) && (TRUE == dumpInput) && IsDumpInputPortBuffer(pOutputPort->portId,
            m_perRequestInfo[requestIdIndex].numUnprocessedFences))
        {
            PerRequestInputPortInfo* pPerRequestInputPort = NULL;

            PerRequestActivePorts* pPerRequestActivePorts  =
            &m_perRequestInfo[pFenceHandlerData->requestId % MaxRequestQueueDepth].activePorts;

            if (NULL != pPerRequestActivePorts->pInputPorts)
            {
                for (UINT numInpPorts = 0; numInpPorts < pPerRequestActivePorts->numInputPorts; numInpPorts++)
                {
                    pPerRequestInputPort = &pPerRequestActivePorts->pInputPorts[numInpPorts];

                    // dump if input port is available in activePorts
                    if (NULL != pPerRequestInputPort->pImageBuffer)
                    {
                        dumpInfo.pFormat = pPerRequestInputPort->pImageBuffer->GetFormat();

                        // Check dumpInfo.pFormat before de-referencing it for getting width and height.
                        if (NULL != dumpInfo.pFormat)
                        {
                            dumpInfo.portId             = pPerRequestInputPort->portId;
                            dumpInfo.batchId            = 0;
                            dumpInfo.numFramesInBatch   = pPerRequestInputPort->pImageBuffer->GetNumFramesInBatch();
                            dumpInfo.pBaseAddr          = pPerRequestInputPort->pImageBuffer->GetCPUAddress();
                            dumpInfo.width              = dumpInfo.pFormat->width;
                            dumpInfo.height             = dumpInfo.pFormat->height;

                            if ((enableInputPortMask & (1 << pPerRequestInputPort->portId)) &&
                                (enableInstanceMask & (1 << dumpInfo.nodeInstance)))
                            {
                                ImageDump::Dump(&dumpInfo);
                            }
                        }
                    }
                }
            }
        }

        if (TRUE == dumpOutput)
        {
            if (enableOutputBatchNum < numBatchedFrames)
            {
                // Dump only speicified frame numbers in given Batch (HFR: usecase)
                numBatchedFrames = enableOutputBatchNum;
            }
            for (UINT batchId = 0; batchId < numBatchedFrames; batchId++)
            {
                UINT  batchIndex       = 0;
                if (NULL != pFenceHandlerData->pOutputBufferInfo[batchId].pImageBuffer)
                {
                    batchIndex = batchId;
                }
                if (NULL == pFenceHandlerData->pOutputBufferInfo[batchIndex].pImageBuffer)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Node:[%s] Output port[%d] Image buffer is NULL",
                                   NodeIdentifierString(),
                                   pOutputPort->portId);
                    continue;
                }
                dumpInfo.portId           = pOutputPort->portId;
                dumpInfo.batchId          = batchId;

                // Below code is to support Image dump for by-passable node which works on input buffer only
                if ((TRUE == pFenceHandlerData->delayedOutputBufferData.isParentInputBuffer) &&
                    (TRUE == IsBypassableNode()))
                {
                    dumpInfo.numFramesInBatch =
                        pFenceHandlerData->delayedOutputBufferData.ppImageBuffer[0]->GetNumFramesInBatch();
                    dumpInfo.pFormat   = pFenceHandlerData->delayedOutputBufferData.ppImageBuffer[0]->GetFormat();
                    dumpInfo.pBaseAddr = pFenceHandlerData->delayedOutputBufferData.ppImageBuffer[0]->GetCPUAddress();
                }
                else
                {
                    dumpInfo.numFramesInBatch =
                        pFenceHandlerData->pOutputBufferInfo[batchIndex].pImageBuffer->GetNumFramesInBatch();
                    dumpInfo.pFormat   = pFenceHandlerData->pOutputBufferInfo[batchIndex].pImageBuffer->GetFormat();
                    dumpInfo.pBaseAddr = pFenceHandlerData->pOutputBufferInfo[batchIndex].pImageBuffer->GetCPUAddress();
                }

                dumpInfo.width            = pOutputPort->bufferProperties.imageFormat.width;
                dumpInfo.height           = pOutputPort->bufferProperties.imageFormat.height;

                if ((enableOutputPortMask & (1 << dumpInfo.portId)) && (enableInstanceMask & (1 << dumpInfo.nodeInstance)))
                {
                    ImageDump::Dump(&dumpInfo);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpDebugInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DumpDebugInfo()
{
    DumpFenceErrors(0, 0);

    BOOL needNodeDump = FALSE;

    for (UINT i = 0; i < MaxRequestQueueDepth; i++)
    {
        if (((FALSE == m_perRequestInfo[i].partialMetadataComplete) ||
             (FALSE == m_perRequestInfo[i].metadataComplete) ||
             (FALSE == m_perRequestInfo[i].requestComplete) ||
             (PerRequestNodeStatus::Cancelled != m_perRequestInfo[i].requestStatus) ||
             (PerRequestNodeStatus::Success != m_perRequestInfo[i].requestStatus)) &&
             (m_perRequestInfo[i].requestId != 0))
        {
            needNodeDump = TRUE;
            break;
        }
    }

    if (TRUE == needNodeDump)
    {
        CAMX_LOG_DUMP(CamxLogGroupCore, "+   NODE:[%s]:Type:%d %p", NodeIdentifierString(), Type(), this);

        for (UINT i = 0; i < MaxRequestQueueDepth; i++)
        {
            if (((FALSE == m_perRequestInfo[i].partialMetadataComplete) ||
                (FALSE == m_perRequestInfo[i].metadataComplete) ||
                 (FALSE == m_perRequestInfo[i].requestComplete)) &&
                 (m_perRequestInfo[i].requestId != 0))
            {
                CAMX_LOG_DUMP(CamxLogGroupCore, "+    Request: %llu Unsuccessful. metadataComplete: %d, requestComplete: %d, "
                               "numUnsignaledFences: %d, numUnprocessedFences: %d, status: %s",
                               m_perRequestInfo[i].requestId,
                               m_perRequestInfo[i].metadataComplete,
                               m_perRequestInfo[i].requestComplete,
                               m_perRequestInfo[i].numUnsignaledFences,
                               m_perRequestInfo[i].numUnprocessedFences,
                               GetRequestStatusString(m_perRequestInfo[i].requestStatus));

                CSLFence hPrevFence = CSLInvalidFence;

                for (UINT j = 0; j < m_outputPortsData.numPorts; j++)
                {
                    OutputPort* pOutputPort     = &(m_outputPortsData.pOutputPorts[j]);
                    UINT        imgRefCount     = 0;
                    UINT        maxImageBuffers = pOutputPort->bufferProperties.maxImageBuffers;

                    if (0 != maxImageBuffers)
                    {
                        UINT32                  imageFenceIndex     = m_perRequestInfo[i].requestId % maxImageBuffers;
                        NodeFenceHandlerData*   pFenceHandlerData   = &pOutputPort->pFenceHandlerData[imageFenceIndex];

                        if ((NULL != pOutputPort->ppImageBuffers) && (NULL != pOutputPort->ppImageBuffers[imageFenceIndex]))
                        {
                            ImageBuffer*    pImageBuffer    = pOutputPort->ppImageBuffers[imageFenceIndex];
                            CSLFence        hCurrFence      = pFenceHandlerData->hFence;

                            if ((TRUE == pFenceHandlerData->isDelayedBufferFence) ||
                                (FALSE == pFenceHandlerData->isFenceSignaled) ||
                                (CSLFenceResultFailed == pFenceHandlerData->fenceResult))
                            {
                                if (hPrevFence == hCurrFence)
                                {
                                    CAMX_LOG_DUMP(CamxLogGroupCore, "+                  Output port id: %d; "
                                                  "isDelayedBufferFence?: %d; isFenceSignaled?: %d; fenceResult: %d; "
                                                  "ImgRefCount=%d; maxImageBuffers=%d",
                                                  pOutputPort->portId,
                                                  pFenceHandlerData->isDelayedBufferFence,
                                                  pFenceHandlerData->isFenceSignaled,
                                                  pFenceHandlerData->fenceResult,
                                                  pImageBuffer->GetReferenceCount(),
                                                  maxImageBuffers);
                                }
                                else
                                {
                                    CAMX_LOG_DUMP(CamxLogGroupCore, "+       Fence: %d; Output port id: %d; "
                                                  "isDelayedBufferFence?: %d; isFenceSignaled?: %d; fenceResult: %d; "
                                                  "ImgRefCount=%d; maxImageBuffers=%d",
                                                  pFenceHandlerData->hFence,
                                                  pOutputPort->portId,
                                                  pFenceHandlerData->isDelayedBufferFence,
                                                  pFenceHandlerData->isFenceSignaled,
                                                  pFenceHandlerData->fenceResult,
                                                  pImageBuffer->GetReferenceCount(),
                                                  maxImageBuffers);
                                }
                                hPrevFence = hCurrFence;
                            }
                        }
                    }
                }
            }
        }
        CAMX_LOG_DUMP(CamxLogGroupCore, "+----------------------------+");
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::WatermarkImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::WatermarkImage(
    NodeFenceHandlerData* pFenceHandlerData)
{
    const StaticSettings* pSettings  = HwEnvironment::GetInstance()->GetStaticSettings();

    if (TRUE == pSettings->watermarkImage)
    {
        BOOL               watermark         = FALSE;
        OutputPort*        pOutputPort       = pFenceHandlerData->pOutputPort; // Output port to which the fence belongs to
        UINT               numBatchedFrames  = GetBatchedHALOutputNum(pOutputPort->numBatchedFrames,
                                                                      pOutputPort->HALOutputBufferCombined);
        UINT               portId            = pOutputPort->portId;
        const ImageBuffer* pImageBuffer      = pFenceHandlerData->pOutputBufferInfo[0].pImageBuffer;

        if (NULL == pImageBuffer)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Node::%s Invalid pImageBuffer", NodeIdentifierString());
            return;
        }

        CAMX_ASSERT((FALSE == pOutputPort->flags.isSinkBuffer) || (numBatchedFrames == 1));

        const ImageFormat* pImageFormat      = pImageBuffer->GetFormat();
        if (NULL == pImageFormat)
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Node::%s Invalid pImageFormat", NodeIdentifierString());
            return;
        }

        switch (Type())
        {
            /// @todo (CAMX-2875) Need a good way to do comparisons with HWL node types and ports.
            case IFE: // IFE  Only watermark IFE full output for YUV format
                if (((IFEOutputPortFull == portId) || (IFEOutputPortDisplayFull == portId)) &&
                    (Format::YUV420NV12 == pImageFormat->format))
                {
                    watermark = TRUE;
                }
                break;
            default:
                break;
        }

        if ((TRUE == watermark) && (NULL != m_pWatermarkPattern))
        {
            if (m_pWatermarkPattern->watermarkOffsetX > pImageFormat->width)
            {
                m_pWatermarkPattern->watermarkOffsetX = pImageFormat->width;
            }
            if (m_pWatermarkPattern->watermarkOffsetY > pImageFormat->height)
            {
                m_pWatermarkPattern->watermarkOffsetY = pImageFormat->height;
            }

            for (UINT i = 0; i < numBatchedFrames; i++)
            {
                ImageDumpInfo dumpInfo = { 0 };

                dumpInfo.requestId         = static_cast<UINT32>(pFenceHandlerData->requestId);
                dumpInfo.batchId           = i;
                dumpInfo.numFramesInBatch  = pFenceHandlerData->pOutputBufferInfo[i].pImageBuffer->GetNumFramesInBatch();
                dumpInfo.pFormat           = pFenceHandlerData->pOutputBufferInfo[i].pImageBuffer->GetFormat();
                dumpInfo.pBaseAddr         = pFenceHandlerData->pOutputBufferInfo[i].pImageBuffer->GetCPUAddress();
                dumpInfo.width             = pOutputPort->bufferProperties.imageFormat.width;
                dumpInfo.height            = pOutputPort->bufferProperties.imageFormat.height;
                dumpInfo.pWatermarkPattern = m_pWatermarkPattern;

                if (NULL != dumpInfo.pBaseAddr)
                {
                    ImageDump::Watermark(&dumpInfo);
                }

                pFenceHandlerData->pOutputBufferInfo[i].pImageBuffer->CacheOps(TRUE, TRUE);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::CreateCmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::CreateCmdBufferManager(
    const CHAR*           pBufferManagerName,
    const ResourceParams* pParams,
    CmdBufferManager**    ppCmdBufferManager)
{
    CamxResult  result                                  = CamxResultEOutOfBounds;
    CHAR        bufferManagerName[MaxStringLength256];

    OsUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "CBM_%s_%s", NodeIdentifierString(), pBufferManagerName);
    if (m_numCmdBufferManagers >= m_maxNumCmdBufferManagers)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Out of bounds for creating buffer manager %s", bufferManagerName);
    }
    else
    {
        result = CmdBufferManager::Create(bufferManagerName, pParams, ppCmdBufferManager);

        if ((CamxResultSuccess == result) && (NULL != *ppCmdBufferManager))
        {
            m_pCmdBufferManagersLock->Lock();
            m_ppCmdBufferManagers[m_numCmdBufferManagers++] = *ppCmdBufferManager;
            m_pCmdBufferManagersLock->Unlock();
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to Create command Buffer Manager(%s)",
                bufferManagerName);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::CreateMultiCmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::CreateMultiCmdBufferManager(
    struct CmdBufferManagerParam* pParams,
    UINT32                        numberOfBufferManagers)
{
    CamxResult result = CamxResultSuccess;

    result = CmdBufferManager::CreateMultiManager(FALSE, pParams, numberOfBufferManagers);

    m_pCmdBufferManagersLock->Lock();
    for (UINT count = 0; count < numberOfBufferManagers; count++)
    {
        CAMX_LOG_INFO(CamxLogGroupCore, "Buffer Manager Name %s", pParams[count].pBufferManagerName);
        m_ppCmdBufferManagers[m_numCmdBufferManagers++] = *pParams[count].ppCmdBufferManager;
    }
    m_pCmdBufferManagersLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DestroyCmdBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DestroyCmdBufferManager(
    CmdBufferManager**    ppCmdBufferManager)
{
    UINT       i;

    for (i = 0; i < m_numCmdBufferManagers; i++)
    {
        if (m_ppCmdBufferManagers[i] == *ppCmdBufferManager)
        {
            break;
        }
    }

    if (i < m_numCmdBufferManagers)
    {
        CAMX_DELETE(*ppCmdBufferManager);
        *ppCmdBufferManager = NULL;
        for (UINT j = i + 1; j < m_numCmdBufferManagers; j++)
        {
            m_ppCmdBufferManagers[j - 1] = m_ppCmdBufferManagers[j];
        }
        m_numCmdBufferManagers--;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetCmdBufferForRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBuffer* Node::GetCmdBufferForRequest(
    UINT64              requestId,
    CmdBufferManager*   pCmdBufferManager)
{
    CAMX_ASSERT(NULL != pCmdBufferManager);

    PacketResource* pPacketResource = NULL;

    if (CamxResultSuccess == pCmdBufferManager->GetBufferForRequest(GetCSLSyncId(requestId), &pPacketResource))
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
    }

    // We know pPacketResource actually points to a CmdBuffer so we may static_cast
    return static_cast<CmdBuffer*>(pPacketResource);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::CheckCmdBufferWithRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBuffer* Node::CheckCmdBufferWithRequest(
    UINT64              requestId,
    CmdBufferManager*   pCmdBufferManager)
{
    CAMX_ASSERT(NULL != pCmdBufferManager);

    PacketResource* pPacketResource = NULL;

    if (CamxResultSuccess == pCmdBufferManager->CheckBufferWithRequest(GetCSLSyncId(requestId), &pPacketResource))
    {
        CAMX_LOG_INFO(CamxLogGroupPProc, "checkbuffer");
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
    }

    // We know pPacketResource actually points to a CmdBuffer so we may static_cast
    return static_cast<CmdBuffer*>(pPacketResource);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetPacketForRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Packet* Node::GetPacketForRequest(
    UINT64              requestId,
    CmdBufferManager*   pCmdBufferManager)
{
    CAMX_ASSERT(NULL != pCmdBufferManager);

    PacketResource* pPacketResource = NULL;

    if (CamxResultSuccess == pCmdBufferManager->GetBufferForRequest(GetCSLSyncId(requestId), &pPacketResource))
    {
        CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);
    }

    // We know pPacketResource actually points to a Packet so we may static_cast
    return static_cast<Packet*>(pPacketResource);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetCmdBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CmdBuffer* Node::GetCmdBuffer(
    CmdBufferManager* pCmdBufferManager)
{
    PacketResource* pPacketResource = NULL;
    if (NULL != pCmdBufferManager)
    {
        if (CamxResultSuccess == pCmdBufferManager->GetBuffer(&pPacketResource))
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().cmdBuffer);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "GetCmdBuffer:Cmd Buffer Manager is null");
    }
    return static_cast<CmdBuffer*>(pPacketResource);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetPacket
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Packet* Node::GetPacket(
    CmdBufferManager* pCmdBufferManager)
{
    PacketResource* pPacketResource = NULL;
    if (NULL != pCmdBufferManager)
    {
        if (CamxResultSuccess == pCmdBufferManager->GetBuffer(&pPacketResource))
        {
            CAMX_ASSERT(TRUE == pPacketResource->GetUsageFlags().packet);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "GetPacket:Cmd Buffer Manager is null");
    }
    return static_cast<Packet*>(pPacketResource);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::AddOutputDeviceIndices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::AddOutputDeviceIndices(
    UINT         portId,
    const INT32* pDeviceIndices,
    UINT         deviceIndexCount)
{
    UINT        portIndex = OutputPortIndex(portId);
    OutputPort* pPort     = &(m_outputPortsData.pOutputPorts[portIndex]);

    for (UINT i = 0; i < deviceIndexCount; i++)
    {
        BOOL alreadyInList = FALSE;

        for (UINT j = 0; j < pPort->deviceCount; j++)
        {
            if (pDeviceIndices[i] == pPort->deviceIndices[j])
            {
                alreadyInList = TRUE;
                break;
            }
        }

        if (FALSE == alreadyInList)
        {
            pPort->deviceIndices[pPort->deviceCount++] = pDeviceIndices[i];
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::AddOutputNodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::AddOutputNodes(
    UINT    portId,
    Node*   pDstNode)
{
    UINT        portIndex = OutputPortIndex(portId);
    OutputPort* pPort     = &(m_outputPortsData.pOutputPorts[portIndex]);

    BOOL bAlreadyInList = FALSE;

    for (UINT j = 0; j < pPort->numOfNodesConnected; j++)
    {
        if (pDstNode == pPort->pDstNodes[j])
        {
            bAlreadyInList = TRUE;
            break;
        }
    }

    if (FALSE == bAlreadyInList)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node[%s] Port[%s][%d] Index[%d] : Adding DstNode[%s]",
                         NodeIdentifierString(), GetOutputPortName(portId), portId,
                         pPort->numOfNodesConnected, pDstNode->NodeIdentifierString());

        pPort->pDstNodes[pPort->numOfNodesConnected++] = pDstNode;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::AddDeviceIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::AddDeviceIndex(
    INT32 deviceIndex)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT((m_deviceIndexCount + 1) <= CamxMaxDeviceIndex);

    if ((m_deviceIndexCount + 1) <= CamxMaxDeviceIndex)
    {
        m_deviceIndices[m_deviceIndexCount++] = deviceIndex;
        // Add the added device index to all output ports.
        for (UINT portIndex = 0; portIndex < m_outputPortsData.numPorts; portIndex++)
        {
            UINT portID = m_outputPortsData.pOutputPorts[portIndex].portId;
            AddOutputDeviceIndices(portID, &deviceIndex, 1);
        }
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::AddCSLDeviceHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::AddCSLDeviceHandle(
    CSLDeviceHandle hCslDeiveHandle)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(m_cslDeviceCount < CamxMaxDeviceIndex);

    if (m_cslDeviceCount < CamxMaxDeviceIndex)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Added CSL Handle: %p at index: %d", hCslDeiveHandle, m_cslDeviceCount);
        m_hCSLDeviceHandles[m_cslDeviceCount++] = hCslDeiveHandle;
    }
    else
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::CheckSourcePortBufferRequirements
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::CheckSourcePortBufferRequirements()
{
    CamxResult          result                          = CamxResultSuccess;
    ChiStreamWrapper*   pChiStreamWrapper               = NULL;
    const ImageFormat*  pFormat                         = NULL;
    BOOL                isUBWC                          = FALSE;

    for (UINT input = 0; input < m_inputPortsData.numPorts; input++)
    {
        BufferRequirement* pBufferRequirement = &m_bufferNegotiationData.inputBufferOptions[input].bufferRequirement;

        if (TRUE == IsSourceBufferInputPort(input))
        {
            pChiStreamWrapper = m_pPipeline->GetInputStreamWrapper(Type(), InstanceID(), input);

            if (NULL != pChiStreamWrapper)
            {
                ChiStream*      pChiStream = reinterpret_cast<ChiStream*>(pChiStreamWrapper->GetNativeStream());

                if ((pChiStream->width > pBufferRequirement->maxWidth) ||
                    (pChiStream->height > pBufferRequirement->maxHeight))
                {
                    for (UINT i = 0; i < m_bufferNegotiationData.numOutputPortsNotified; i++)
                    {
                        pFormat = static_cast<const ImageFormat *>
                        (&m_bufferNegotiationData.pOutputPortNegotiationData[i].pFinalOutputBufferProperties->imageFormat);
                        if (TRUE == ImageFormatUtils::IsUBWC(pFormat->format))
                        {
                            isUBWC = TRUE;
                            break;
                        }
                    }

                    if (TRUE == isUBWC)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Input buffer size(%dx%d) greater than maximum supported "
                                       "buffer size(%dx%d) determined during buffer negotiation so trigger renegotiation",
                                       NodeIdentifierString(),
                                       pChiStream->width,
                                       pChiStream->height,
                                       pBufferRequirement->maxWidth,
                                       pBufferRequirement->maxHeight);
                        result = CamxResultEFailed;
                        break;
                    }

                }

            }

        }

    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::TriggerBufferNegotiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::TriggerBufferNegotiation()
{
    CamxResult result = CamxResultSuccess;

    // The node can determine the input buffer requirements only after all its output ports have been notified of the buffer
    // requirements from all the input ports to which they are connected
    if (m_bufferNegotiationData.numOutputPortsNotified == m_outputPortsData.numPorts)
    {
        result = ProcessInputBufferRequirement();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessInputBufferRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::ProcessInputBufferRequirement()
{
    CamxResult          result                          = CamxResultSuccess;
    BOOL                doBufferRequirementNotification = TRUE;
    const ImageFormat*  pFormat                         = NULL;
    BOOL                isUBWC                          = FALSE;
    ChiStreamWrapper*   pChiStreamWrapper               = NULL;

    CAMX_ASSERT(m_bufferNegotiationData.numOutputPortsNotified == m_outputPortsData.numPorts);

    result = ProcessingNodeFinalizeInputRequirement(&m_bufferNegotiationData);

    if (CamxResultSuccess == result)
    {
        // Add the node to the list of nodes the topology needs to call for finalizing its buffer properties
        /// @todo (CAMX-1797) Need to find a better way to do the walk-forward path of buffer negotiation
        doBufferRequirementNotification = m_pPipeline->AddFinalizeBufferPropertiesNodeList(this);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node input finalization failed for pipeline[%s], node[%s]",
                       m_pPipeline->GetPipelineIdentifierString(), m_pNodeName);
        result = CamxResultEFailed;
    }

    // If node was not added to the FinalizeBufferPropertiesNodeList then skip BufferRequirementNotification
    if ((CamxResultSuccess == result) && (TRUE == doBufferRequirementNotification))
    {
        ///@ todo (CAMX-346) Do we need to do anything special if there are multiple input ports?

        for (UINT input = 0; input < m_inputPortsData.numPorts; input++)
        {
            BufferRequirement* pBufferRequirement = &m_bufferNegotiationData.inputBufferOptions[input].bufferRequirement;
            if (FALSE == IsSourceBufferInputPort(input))
            {
                Node*              pParentNode           = m_inputPortsData.pInputPorts[input].pParentNode;
                UINT               parentOutputPortIndex = m_inputPortsData.pInputPorts[input].parentOutputPortIndex;

                // If this warning fires check the FinalizeInputRequirement(..) function in the derived node
                if ((Type() != AutoFocus) && (Type() != StatsProcessing))
                {
                    if (m_inputPortsData.pInputPorts[input].portId !=
                        m_bufferNegotiationData.inputBufferOptions[input].portId)
                    {
                        CAMX_LOG_WARN(CamxLogGroupCore,
                                     "Node::%s Input portId (%d) does not match Buffer Negotiation portId (%d)",
                                      NodeIdentifierString(),
                                      m_inputPortsData.pInputPorts[input].portId,
                                      m_bufferNegotiationData.inputBufferOptions[input].portId);
                    }
                }

                if ((pBufferRequirement->minWidth  > pBufferRequirement->optimalWidth) ||
                    (pBufferRequirement->minHeight > pBufferRequirement->optimalHeight))
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "%s Input Min Dim [%u x %u] > Optimal Dim [%u x %u]",
                                   NodeIdentifierString(),
                                   pBufferRequirement->minWidth,
                                   pBufferRequirement->minHeight,
                                   pBufferRequirement->optimalWidth,
                                   pBufferRequirement->optimalHeight);
                    result = CamxResultEInvalidState;
                }

                else if ((pBufferRequirement->optimalWidth  > pBufferRequirement->maxWidth) ||
                         (pBufferRequirement->optimalHeight > pBufferRequirement->maxHeight))
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "%s Optimal Dim [%u x %u] > Max Dim [%u x %u]",
                                   NodeIdentifierString(),
                                   pBufferRequirement->optimalWidth,
                                   pBufferRequirement->optimalHeight,
                                   pBufferRequirement->maxWidth,
                                   pBufferRequirement->maxHeight);
                    result = CamxResultEInvalidState;
                }
                else
                {
                    result = pParentNode->BufferRequirementNotification(parentOutputPortIndex, pBufferRequirement);
                }

                if (CamxResultSuccess != result)
                {
                    break;
                }
            }
            else
            {
                pChiStreamWrapper = m_pPipeline->GetInputStreamWrapper(Type(), InstanceID(), input);
                if (NULL != pChiStreamWrapper)
                {
                    ChiStream*      pChiStream = reinterpret_cast<ChiStream*>(pChiStreamWrapper->GetNativeStream());
                    if ((pChiStream->width > pBufferRequirement->maxWidth) ||
                        (pChiStream->height > pBufferRequirement->maxHeight))
                    {
                        for (UINT i = 0; i < m_bufferNegotiationData.numOutputPortsNotified; i++)
                        {
                            pFormat = static_cast<const ImageFormat *>
                            (&m_bufferNegotiationData.pOutputPortNegotiationData[i].pFinalOutputBufferProperties->imageFormat);
                            if (TRUE == ImageFormatUtils::IsUBWC(pFormat->format))
                            {
                                isUBWC = TRUE;
                                break;
                            }

                        }
                        if (TRUE == isUBWC)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Input buffer size(%dx%d) greater than maximum supported "
                                           "buffer size(%dx%d) determined during buffer negotiation so trigger renegotiation",
                                           NodeIdentifierString(),
                                           pChiStream->width,
                                           pChiStream->height,
                                           pBufferRequirement->maxWidth,
                                           pBufferRequirement->maxHeight);
                            result = CamxResultEFailed;
                            break;
                        }
                    }
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::BufferRequirementNotification
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::BufferRequirementNotification(
    UINT               outputPortIndex,
    BufferRequirement* pBufferRequirement)
{
    CamxResult                 result               = CamxResultSuccess;
    OutputPort*                pOutputPort          = &m_outputPortsData.pOutputPorts[outputPortIndex];
    OutputPortNegotiationData* pPortNegotiationData = &m_bufferNegotiationData.pOutputPortNegotiationData[outputPortIndex];

    if (FALSE == pOutputPort->flags.isSinkBuffer)
    {
        pPortNegotiationData->inputPortRequirement[pPortNegotiationData->numInputPortsNotification] = *pBufferRequirement;
    }

    pPortNegotiationData->numInputPortsNotification++;

    if (pPortNegotiationData->numInputPortsNotification == pOutputPort->numInputPortsConnected)
    {
        m_bufferNegotiationData.numOutputPortsNotified++;

        if (m_bufferNegotiationData.numOutputPortsNotified == m_outputPortsData.numPorts)
        {
            result = ProcessInputBufferRequirement();
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CamxResult result = CamxResultSuccess;

    UINT32 numInputPorts = 0;
    UINT32 inputPortId[32];

    // Get Input Port List
    GetAllInputPortIds(&numInputPorts, &inputPortId[0]);

    pBufferNegotiationData->numInputPorts = numInputPorts;

    for (UINT input = 0; input < numInputPorts; input++)
    {
        pBufferNegotiationData->inputBufferOptions[input].nodeId     = Type();
        pBufferNegotiationData->inputBufferOptions[input].instanceId = InstanceID();
        pBufferNegotiationData->inputBufferOptions[input].portId     = inputPortId[input];

        BufferRequirement* pInputBufferRequirement = &pBufferNegotiationData->inputBufferOptions[input].bufferRequirement;

        pInputBufferRequirement->optimalWidth  = 0;
        pInputBufferRequirement->optimalHeight = 0;
        pInputBufferRequirement->minWidth      = 0;
        pInputBufferRequirement->maxWidth      = 0;
        pInputBufferRequirement->minHeight     = 0;
        pInputBufferRequirement->maxHeight     = 0;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::TriggerInplaceProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::TriggerInplaceProcessing()
{
    // If the node is an inplace node and it outputs an HAL buffer, it implies its parent node needs to be informed to
    // output a HAL buffer also
    /// @note Inplace nodes can only have one input port

    NotifyParentHALBufferOutput();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::NotifyParentHALBufferOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::NotifyParentHALBufferOutput()
{
    // Inplace node has only one input port
    UINT              parentOutputPortIndex = m_inputPortsData.pInputPorts[0].parentOutputPortIndex;
    UINT              sinkPortIndex;
    OutputPort*       pOutputPort;
    ChiStreamWrapper* pChiStreamWrapper;

    // There can be only one sink port output for an inplace node
    if (1 < m_outputPortsData.numSinkPorts)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Unexpected number of sink ports (%d) for Inplace Node::%s, "
            "Only One sink port is possible",
            m_outputPortsData.numSinkPorts, NodeIdentifierString());
    }
    else
    {
        sinkPortIndex         = m_outputPortsData.sinkPortIndices[0];
        pOutputPort           = &m_outputPortsData.pOutputPorts[sinkPortIndex];

        if (TRUE == pOutputPort->flags.isSinkInplaceBuffer)
        {
            pChiStreamWrapper     = m_pPipeline->GetOutputStreamWrapper(Type(), InstanceID(), pOutputPort->portId);
            if (FALSE == IsSourceBufferInputPort(0))
            {
                m_inputPortsData.pInputPorts[0].pParentNode->NotifyHALBufferOutput(parentOutputPortIndex, pOutputPort,
                    pChiStreamWrapper);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Input for the Inplace Node::%s is source buffer, which is not acceptable",
                    NodeIdentifierString());
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "LinkFlagSinkInplaceBuffer is not added for sink port of Inplace Node::%s",
                NodeIdentifierString());
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::TriggerOutputPortStreamIdSetup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::TriggerOutputPortStreamIdSetup()
{
    CAMX_ASSERT(m_outputPortsData.numSinkPorts > 0); // This function shouldn't get called if it doesn't have sink port(s)

    for (UINT i = 0; i < m_outputPortsData.numSinkPorts; i++)
    {
        UINT        sinkPortIndex        = m_outputPortsData.sinkPortIndices[i];
        OutputPort* pSinkPort            = &m_outputPortsData.pOutputPorts[sinkPortIndex];
        UINT        sinkPortStreamIdMask = pSinkPort->enabledInStreamMask;
        StreamData* pSinkPortStreamData  = &pSinkPort->streamData;

        /// @note It is assumed that if any output port is enabled, all the input ports are active i.e. all inputs are required
        ///       to generate any (and all) outputs
        for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
        {
            if (FALSE == IsSourceBufferInputPort(inputPortIndex))
            {
                InputPort* pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];

                if (this != pInputPort->pParentNode)
                {
                    pInputPort->pParentNode->SetOutputPortEnabledForStreams(pInputPort->parentOutputPortIndex,
                        sinkPortStreamIdMask);
                    pInputPort->pParentNode->SetStreamDataForOutputPorts(pInputPort->parentOutputPortIndex,
                        pSinkPortStreamData);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::EnableParentOutputPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::EnableParentOutputPorts()
{
    UINT streamId = ((1 << MaxNumStreams) - 1);
    for (UINT i = 0; i < m_outputPortsData.numPorts; i++)
    {
        OutputPort*         pOutputPort = &m_outputPortsData.pOutputPorts[i];
        if (pOutputPort->flags.isLoopback)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Loopback port id %d", pOutputPort->portId);
            for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
            {
                if (FALSE == IsSourceBufferInputPort(inputPortIndex))
                {
                    InputPort* pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];
                    Node* const pParentNode = pInputPort->pParentNode;
                    if (this != pParentNode)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupCore, "For input port %d Setting parent Node::%s port %d to stream %x",
                                         pInputPort->portId,
                                         pParentNode->NodeIdentifierString(),
                                         pParentNode->m_outputPortsData.pOutputPorts[pInputPort->parentOutputPortIndex],
                                         streamId);
                        pParentNode->SetOutputPortEnabledForStreams(pInputPort->parentOutputPortIndex,
                                                                    ((1 << MaxNumStreams) - 1));
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetStreamDataForOutputPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::SetStreamDataForOutputPorts(
    UINT        outputPortIndex,
    StreamData* pStreamData)
{
    SetStreamDataForOutputPort(outputPortIndex, pStreamData);

    /// @note It is assumed that if any output port is enabled, all the input ports are active i.e. all inputs are required to
    ///       generate any (and all) outputs
    for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
    {
        if (FALSE == IsSourceBufferInputPort(inputPortIndex))
        {
            InputPort* pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];

            if ((NULL != pInputPort->pParentNode) && (this != pInputPort->pParentNode))
            {
                pInputPort->pParentNode->SetStreamDataForOutputPorts(pInputPort->parentOutputPortIndex, pStreamData);
            }
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetOutputPortEnabledForStreams
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::SetOutputPortEnabledForStreams(
    UINT outputPortIndex,
    UINT streamIdMask)
{
    EnableOutputPortForStreams(outputPortIndex, streamIdMask);

    /// @note It is assumed that if any output port is enabled, all the input ports are active i.e. all inputs are required to
    ///       generate any (and all) outputs
    for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
    {
        if (FALSE == IsSourceBufferInputPort(inputPortIndex))
        {
            InputPort* pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];

            if (this != pInputPort->pParentNode)
            {
                pInputPort->pParentNode->SetOutputPortEnabledForStreams(pInputPort->parentOutputPortIndex, streamIdMask);
            }
            else
            {
                EnableOutputPortForStreams(pInputPort->parentOutputPortIndex, streamIdMask);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GenerateStreamTypeForAllNodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Node::GenerateStreamTypeForAllNodes()
{
    CAMX_LOG_INFO(CamxLogGroupCore, "Node::%s stream process type: %d", NodeIdentifierString(), GetStreamProcessType());

    unordered_set<Node*> parentNodeMap;
    for (UINT portIndex = 0; portIndex < m_inputPortsData.numPorts; portIndex++)
    {
        Node* pParentNode = m_inputPortsData.pInputPorts[portIndex].pParentNode;
        if ((NULL != pParentNode) && (this != pParentNode))
        {
            if (parentNodeMap.find(pParentNode) == parentNodeMap.end())
            {
                parentNodeMap.insert(pParentNode);
                pParentNode->GenerateStreamTypeForAllNodes();
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::NewActiveStreamsSetup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::NewActiveStreamsSetup(
    UINT activeStreamIdMask)
{
    m_nodeFlags.isEnabled = FALSE;

    UINT numOutputPortsEnabled = 0;
    UINT numInputPortsEnabled  = 0;

    for (UINT portIndex = 0; portIndex < m_outputPortsData.numPorts; portIndex++)
    {
        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[portIndex];

        pOutputPort->flags.isEnabled = FALSE;

        // "enabledInStreamMask" is the list of streams for which the output port needs to be enabled. So if any stream in
        // "activeStreamIdMask" is also set in "enabledInStreamMask", it means we need to enable the output port
        if (0 != (pOutputPort->enabledInStreamMask & activeStreamIdMask) &&
            pOutputPort->numInputPortsConnected > pOutputPort->numInputPortsDisabled)
        {
            if ((FALSE == pOutputPort->flags.isSinkBuffer) ||
                ((TRUE == pOutputPort->flags.isSinkBuffer) &&
                 (TRUE == Utils::IsBitSet(activeStreamIdMask, pOutputPort->sinkTargetStreamId))))
            {
                pOutputPort->flags.isEnabled = TRUE;
                numOutputPortsEnabled++;
            }
        }
    }

    CaptureRequest*        pCaptureRequest  = m_pPipeline->GetRequest(m_tRequestId);
    StreamInputBufferInfo* pInputBufferInfo = pCaptureRequest->pStreamBuffers[0].inputBufferInfo;

    // Count the number of disabled input ports
    for (UINT portIndex = 0; portIndex < m_inputPortsData.numPorts; portIndex++)
    {
        BOOL       isPortEnabled = TRUE;
        InputPort& rInputPort    = m_inputPortsData.pInputPorts[portIndex];

        if (TRUE == rInputPort.flags.isSourceBuffer)
        {
            isPortEnabled = FALSE; // When the source buffer cannot be found in the request, consider this port disabled
            for (UINT bufferId = 0; bufferId < pCaptureRequest->pStreamBuffers[0].numInputBuffers; bufferId++)
            {
                if (TRUE == InputStreamMatches(pInputBufferInfo[bufferId], rInputPort))
                {
                    isPortEnabled = TRUE;
                    break;
                }
            }
        }
        else // When parent is a CamX Node
        {
            if (FALSE == rInputPort.pParentNode->m_nodeFlags.isEnabled)
            {
                isPortEnabled = FALSE;
            }
        }

        if ((TRUE == isPortEnabled) || ((TRUE == IsInplace()) && (numOutputPortsEnabled > 0)))
        {
            numInputPortsEnabled++;
        }
    }

    m_nodeFlags.isEnabled = (((0 == m_inputPortsData.numPorts)  || (numInputPortsEnabled  > 0)) &&
                             ((0 == m_outputPortsData.numPorts) || (numOutputPortsEnabled > 0))) ? TRUE : FALSE;

    // Check for override to disable the node
    if (TRUE == IsNodeDisabledWithOverride())
    {
        m_nodeFlags.isEnabled = FALSE;
    }

    for (UINT portIndex = 0; portIndex < m_inputPortsData.numPorts; portIndex++)
    {
        InputPort* pInputPort = &m_inputPortsData.pInputPorts[portIndex];
        if (FALSE == pInputPort->portDisabled)
        {
            BOOL isParentEnabled = TRUE;
            if (NULL != pInputPort->pParentNode)
            {
                isParentEnabled = pInputPort->pParentNode->m_nodeFlags.isEnabled;
            }
            else if (FALSE == pInputPort->flags.isSourceBuffer)
            {
                CAMX_LOG_WARN(CamxLogGroupCore,
                              "%s:%u Null Parent! Req: %u IsLoopback: %u IsSource: %u flags: 0x%08x",
                              NodeIdentifierString(),
                              pInputPort->portId,
                              m_tRequestId,
                              pInputPort->flags.isLoopback,
                              pInputPort->flags.isSourceBuffer,
                              pInputPort->flags.value);
            }
            pInputPort->flags.isEnabled = m_nodeFlags.isEnabled && ((TRUE == IsInplace()) ? TRUE : isParentEnabled);
        }
        else
        {
            pInputPort->flags.isEnabled = FALSE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetOutputPortInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetOutputPortInfo(
    UINT64                   requestId,
    UINT32                   sequenceId,
    UINT                     outputPortIndex,
    OutputPortRequestedData* pOutData)
{
    CamxResult  result      = CamxResultSuccess;
    OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

    if (pOutputPort->bufferProperties.maxImageBuffers > 0)
    {
        if (TRUE == pOutputPort->flags.isSinkBuffer)
        {
            // TOD: There should be an interface to allow for a non-batch index of 0 to be requested
            const UINT batchIndex = 0;
            result = ProcessSinkPortNewRequest(requestId, batchIndex, pOutputPort, NULL);
        }
        else
        {
            result = ProcessNonSinkPortNewRequest(requestId, sequenceId, pOutputPort);
        }

        if (CamxResultSuccess == result)
        {
            UINT                  imageFenceIndex   = requestId % pOutputPort->bufferProperties.maxImageBuffers;
            NodeFenceHandlerData* pFenceHandlerData = &pOutputPort->pFenceHandlerData[imageFenceIndex];
            NodeFenceHandlerData* pDelayedBufferFenceHandlerData =
                &pOutputPort->pDelayedBufferFenceHandlerData[imageFenceIndex];

            if (TRUE == IsBypassableNode() && ChiExternalNode == Type())
            {
                pOutData->phFence                  = &pDelayedBufferFenceHandlerData->hFence;
                pOutData->pIsFenceSignaled         = &pDelayedBufferFenceHandlerData->isFenceSignaled;
                pOutData->pImageBuffer             = reinterpret_cast<ImageBuffer*>(InvalidImageBuffer);
                pOutData->pDelayedOutputBufferData = &pFenceHandlerData->delayedOutputBufferData;
                pOutData->primaryFence             = pDelayedBufferFenceHandlerData->primaryFence;
            }
            else
            {
                pOutData->phFence                  = &pFenceHandlerData->hFence;
                pOutData->pIsFenceSignaled         = &pFenceHandlerData->isFenceSignaled;
                pOutData->pImageBuffer             = pFenceHandlerData->pOutputBufferInfo[0].pImageBuffer;
                pOutData->primaryFence             = pFenceHandlerData->primaryFence;
                pOutData->pDelayedOutputBufferData = NULL;
            }
            pOutData->pOutputPort = pOutputPort;
        }
    }

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetupRequestOutputPortImageBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SetupRequestOutputPortImageBuffer(
    OutputPort* const pOutputPort,
    const UINT64      requestId,
    const UINT32      sequenceId,
    const UINT        imageBufferIndex)
{
    CamxResult            result           = CamxResultSuccess;
    const UINT32          numBatchedFrames = GetBatchedHALOutputNum(pOutputPort->numBatchedFrames,
                                                                    pOutputPort->HALOutputBufferCombined);
    const OutputPortFlags outputFlags      = pOutputPort->flags;
    const BOOL            isHALBuffer      = (outputFlags.isNonSinkHALBufferOutput || outputFlags.isSinkBuffer);
    ImageBuffer*          pNewImageBuffer  = NULL;

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "IsRealTime():%u", IsRealTime());

    // All Output Ports with a HALBuffer request their own unique ImageBuffer

    if ((FALSE == IsInplace()) || (TRUE == isHALBuffer))
    {
        // ImageBufferManager manages correct recycling of buffers
        pNewImageBuffer = pOutputPort->pImageBufferManager->GetImageBuffer();

        if ((NULL != pNewImageBuffer) &&
            (FALSE == GetStaticSettings()->enableImageBufferLateBinding))
        {
            if (TRUE == isHALBuffer)
            {
                CAMX_ASSERT(FALSE == pNewImageBuffer->HasBackingBuffer());
            }
            else
            {
                CAMX_ASSERT(TRUE == pNewImageBuffer->HasBackingBuffer());
            }
        }
    }
    else
    {
        if (TRUE == isHALBuffer)
        {
            // Inplace nodes can only ever have one input port. For Sink port of Inplace node, the output image buffer is
            // the same as its input buffer so we get the parent nodes output buffer and make it this inplace nodes
            // output buffer as well
            if (FALSE == IsSourceBufferInputPort(0))
            {
                InputPort*              pInputPort            = &m_inputPortsData.pInputPorts[0];
                Node*                   pParentNode           = pInputPort->pParentNode;
                UINT                    parentOutputPortIndex = pInputPort->parentOutputPortIndex;
                OutputPortRequestedData parentOutputPort      = {0};

                pParentNode->GetOutputPortInfo(requestId, sequenceId, parentOutputPortIndex, &parentOutputPort);

                pNewImageBuffer = parentOutputPort.pImageBuffer;
            }
        }
        else
        {
            // For Non-Sink port of Inplace node, we need to get a new buffer from the buffer manager
            pNewImageBuffer = pOutputPort->pImageBufferManager->GetImageBuffer();
        }
    }

    if (NULL == pNewImageBuffer)
    {
        ///@ todo (CAMX-325) The requesting node should report a dependency back to Pipeline since it wasn't able
        ///                  to obtain a buffer
        result = CamxResultEResource;
        CAMX_ASSERT_ALWAYS_MESSAGE("pNewImageBuffer is NULL.");
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s pNewImageBuffer is NULL as no buffer was obtained from ImageBufferManager",
                       NodeIdentifierString());
    }

    if (CamxResultSuccess != result)
    {
        if (NULL != pNewImageBuffer)
        {
            /// @note GetBuffer() starts off with a refCnt of 1, so we decrement it by 1 in case of error
            pNewImageBuffer->ReleaseBufferManagerImageReference();
            pNewImageBuffer = NULL;
        }
    }
    else if ((FALSE == isHALBuffer) ||
             ((TRUE == outputFlags.isSharedSinkBuffer) && (0 == (imageBufferIndex % numBatchedFrames))))
    {
        CAMX_ASSERT(NULL != pNewImageBuffer);

        // Add ref count for the child nodes which are connected to the bypassable node
        // numInputPortsConnectedinBypass is 0 for non bypassable nodes
        const UINT32 numberOfImageReferences = GetNumChildImageBufferReferences(pOutputPort);
        for (UINT i = 0; i < numberOfImageReferences; i++)
        {
            pNewImageBuffer->AddBufferManagerImageReference();
        }

        CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node: %08x, Node::%s Number of input ports connected in bypass %d "
                         "for Image Buffer %x Ref count %d",
                         this,
                         NodeIdentifierString(),
                         pOutputPort->numInputPortsConnectedinBypass,
                         pNewImageBuffer,
                         pNewImageBuffer->GetReferenceCount());
    }

    CAMX_LOG_VERBOSE(CamxLogGroupCore,
                     "Node: %08x, Node::%s, Output Port ID: %d, Buffer 0x%x, Ref count %d, RequestId: %d, SequenceId: %d",
                     this, NodeIdentifierString(), pOutputPort->portId, pNewImageBuffer, pNewImageBuffer->GetReferenceCount(),
                     requestId, sequenceId);

    m_pBufferRequestLock->Lock();

    pOutputPort->ppImageBuffers[imageBufferIndex] = pNewImageBuffer;

    m_pBufferRequestLock->Unlock();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetupRequestOutputPortFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SetupRequestOutputPortFence(
    OutputPort* const pOutputPort,
    const UINT64      requestId,
    UINT*             pOutputBufferIndex)
{
    BOOL                  primaryFence                   = FALSE;
    CamxResult            result                         = CamxResultSuccess;
    CSLFence              hNewFence                      = CSLInvalidHandle;
    CSLFence              hNewDelayedBufferFence         = CSLInvalidHandle;
    const UINT32          maxImageBuffers                = pOutputPort->bufferProperties.maxImageBuffers;
    const UINT32          imageFenceIndex                = requestId % maxImageBuffers;
    NodeFenceHandlerData& rFenceHandlerData              = pOutputPort->pFenceHandlerData[imageFenceIndex];
    NodeFenceHandlerData& rDelayedBufferFenceHandlerData = pOutputPort->pDelayedBufferFenceHandlerData[imageFenceIndex];
    UINT                  portId                         = pOutputPort->portId;
    result = ReleaseOutputPortCSLFences(pOutputPort, requestId, pOutputBufferIndex, TRUE);

    CHIFENCEHANDLE* phReleaseFence = NULL;
    BOOL            ChiFenceExists = FALSE;

    if (CamxResultSuccess == result)
    {
        if (NULL != pOutputBufferIndex)
        {
            CaptureRequest*   pCaptureRequest   = m_pPipeline->GetRequest(requestId);
            StreamBufferInfo* pStreamBufferInfo = NULL;
            ChiStreamBuffer*  pOutputBuffer     = NULL;
            const UINT        batchIndex        = 0;

            CAMX_ASSERT(NULL != pOutputBufferIndex);

            pStreamBufferInfo = &(pCaptureRequest->pStreamBuffers[batchIndex]);
            pOutputBuffer     = &(pStreamBufferInfo->outputBuffers[*pOutputBufferIndex]);  // one buffer / req. / port

            if ((TRUE == pOutputBuffer->releaseFence.valid) &&
                (ChiFenceTypeInternal == pOutputBuffer->releaseFence.type))
            {
                phReleaseFence = &(pOutputBuffer->releaseFence.hChiFence);
            }

            CAMX_ASSERT_MESSAGE((pCaptureRequest->requestId == requestId),
                "(pCaptureRequest->requestId:%llu == requestId:%llu)",
                pCaptureRequest->requestId,
                requestId);
        }

        ChiFenceExists = IsValidCHIFence(phReleaseFence);

        if (FALSE == ChiFenceExists)
        {
            CHAR fenceName[MaxStringLength256];
            OsUtils::SNPrintF(fenceName, sizeof(fenceName), "NodeOutputPortFence_%s_PortId_%d",
                              NodeIdentifierString(), pOutputPort->portId);

            // Fetch the composite group ID
            UINT groupID = m_bufferComposite.portGroupID[pOutputPort->portId];

            // Create fence only for ports(Nodes) which dont have composite mask or
            // composite ports which dont have a valid fence yet
            if ((FALSE            == m_bufferComposite.hasCompositeMask)           ||
                (CSLInvalidHandle == m_outputPortsData.hBufferGroupFence[groupID]) ||
                (0                == groupID))
            {
                result = CSLCreatePrivateFence(fenceName, &hNewFence);

                // Update the fence for the composite group
                m_outputPortsData.hBufferGroupFence[groupID] = hNewFence;
                primaryFence                                 = TRUE;
            }
            else
            {
                hNewFence = m_outputPortsData.hBufferGroupFence[groupID];
            }

            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node %s, Request %llu, PortId %d, GroupID %d, Fence %d, primary %d",
                           Name(),
                           requestId,
                           pOutputPort->portId,
                           groupID,
                           hNewFence,
                           primaryFence);

            CAMX_LOG_INFO(CamxLogGroupCore, "%s Node: %08x, Node::%s, fence: %08x(%d), Port: %d, reqId: %llu, ImgBuf: %p, "
                          "result: %d %s",
                          ((FALSE == ChiFenceExists) ? "CreatePrivateFence..." : "Using CHI Fence..."),
                          this,
                          NodeIdentifierString(),
                          &(rFenceHandlerData.hFence),
                          hNewFence,
                          pOutputPort->portId,
                          requestId,
                          pOutputPort->ppImageBuffers[imageFenceIndex],
                          result,
                          ((CamxResultSuccess == result) ? "" : "(Error)"));

            if ((CamxResultSuccess != result) || (CSLInvalidHandle == hNewFence))
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Fence creation failed");
                result = CamxResultEFailed;
            }
            else
            {
                const UINT32 numberOfFenceReferences =
                    1 +                                                                // Reference for this node
                    static_cast<INT32>(pOutputPort->numInputPortsConnected) +          // References for child nodes
                    static_cast<INT32>(pOutputPort->numInputPortsConnectedinBypass) +  // References for descendant nodes
                    (TRUE == pOutputPort->flags.isSinkBuffer ? -1 : 0); // Do not refcount the input port for the sink buffer
                                                                        // because no node connects to it in this pipeline

                UINT32 groupIDRequestIndex = requestId % RequestQueueDepth;

                m_portFenceInfo[groupIDRequestIndex][groupID].hFence     = m_outputPortsData.hBufferGroupFence[groupID];

                UINT32 refcount = numberOfFenceReferences;
                if (TRUE == primaryFence)
                {
                    m_portFenceInfo[groupIDRequestIndex][groupID].pPrimaryOutputPort = pOutputPort;
                    CamxAtomicStoreU32(&m_portFenceInfo[groupIDRequestIndex][groupID].aRefCount, numberOfFenceReferences);
                }
                else
                {
                    refcount = CamxAtomicAddU32(&m_portFenceInfo[groupIDRequestIndex][groupID].aRefCount,
                        numberOfFenceReferences);
                }

                CAMX_LOG_VERBOSE(CamxLogGroupCore, " Node %s,Fence %d, total refCount %d, port[%d] count %d",
                               Name(),
                               m_outputPortsData.hBufferGroupFence[groupID],
                               refcount,
                               pOutputPort->portId,
                               numberOfFenceReferences);
            }
        }
        else
        {
            UINT            groupID                 = m_bufferComposite.portGroupID[pOutputPort->portId];
            UINT32          groupIDRequestIndex     = requestId % RequestQueueDepth;
            const UINT32    numberOfFenceReferences = pOutputPort->numInputPortsConnected +
                                                      pOutputPort->numInputPortsConnectedinBypass -
                                                      (TRUE == pOutputPort->flags.isSharedSinkBuffer ? 1 : 0) + 1;

            hNewFence       = reinterpret_cast<ChiFence*>(*phReleaseFence)->hFence;
            primaryFence    = TRUE;

            m_outputPortsData.hBufferGroupFence[groupID] = hNewFence;

            m_portFenceInfo[groupIDRequestIndex][groupID].hFence             = m_outputPortsData.hBufferGroupFence[groupID];
            m_portFenceInfo[groupIDRequestIndex][groupID].pPrimaryOutputPort = pOutputPort;
            CamxAtomicStoreU32(&m_portFenceInfo[groupIDRequestIndex][groupID].aRefCount, numberOfFenceReferences);

            CAMX_LOG_INFO(CamxLogGroupCore, "%s Node: %08x, Node::%s, fence: %08x(%d), Port: %d, reqId: %llu, ImgBuf: %p",
                          ((FALSE == ChiFenceExists) ? "CreatePrivateFence..." : "Using CHI Fence..."),
                          this,
                          NodeIdentifierString(),
                          phReleaseFence,
                          hNewFence,
                          pOutputPort->portId,
                          requestId,
                          pOutputPort->ppImageBuffers[imageFenceIndex]);
        }
    }

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                        "Node: %08x, Node::%s, m_tRequestId:%llu maxImageBuffers:%u (m_tRequestId %% maxImageBuffers):%llu",
                        this,
                        NodeIdentifierString(),
                        m_tRequestId,
                        maxImageBuffers,
                        m_tRequestId % maxImageBuffers);

        if ((TRUE == primaryFence) || (TRUE == ChiFenceExists))
        {
            result = CSLFenceAsyncWait(hNewFence,
                                       Node::CSLFenceCallback,
                                       &rFenceHandlerData.nodeCSLFenceCallbackData);
        }

        if (CamxResultSuccess == result)
        {
            rFenceHandlerData.hFence           = hNewFence;
            rFenceHandlerData.isFenceSignaled  = FALSE;
            rFenceHandlerData.requestId        = requestId;
            rFenceHandlerData.primaryFence     = primaryFence;
            rFenceHandlerData.isChiFence       = ChiFenceExists;

        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Fence wait failed");
        }
    }

    if (TRUE == IsBypassableNode())
    {
        if (CamxResultSuccess == result)
        {
            CHAR fenceName[MaxStringLength256];
            OsUtils::SNPrintF(fenceName, sizeof(fenceName), "DelayedBufferFence_%s", NodeIdentifierString());

            result = CSLCreatePrivateFence(fenceName, &hNewDelayedBufferFence);

            primaryFence = TRUE;

            CAMX_LOG_INFO(CamxLogGroupCore, "CreatePrivateFence...Node: %08x, Node::%s, fence: %08x(%d), ImgBuf:%p "
                          "reqId:%llu result: %d",
                          this,
                          NodeIdentifierString(),
                          &rDelayedBufferFenceHandlerData.hFence,
                          hNewDelayedBufferFence,
                          pOutputPort->ppImageBuffers[imageFenceIndex],
                          requestId,
                          result);

            if ((CamxResultSuccess != result) || (CSLInvalidHandle == hNewDelayedBufferFence))
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Fence creation failed for Delayed Buffer");
                result = CamxResultEFailed;
            }
            else
            {
                rDelayedBufferFenceHandlerData.requestId       = requestId;
                rDelayedBufferFenceHandlerData.hFence          = hNewDelayedBufferFence;
                rDelayedBufferFenceHandlerData.isFenceSignaled = FALSE;
                rDelayedBufferFenceHandlerData.primaryFence    = primaryFence;
                rDelayedBufferFenceHandlerData.isChiFence      = ChiFenceExists;
            }
        }

        if (CamxResultSuccess == result)
        {
            if ((TRUE == primaryFence) || (TRUE == ChiFenceExists))
            {
                result = CSLFenceAsyncWait(hNewDelayedBufferFence,
                                           Node::CSLFenceCallback,
                                           &(rDelayedBufferFenceHandlerData.nodeCSLFenceCallbackData));
            }

            if (CamxResultSuccess != result)
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Fence wait failed");
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        ReleaseOutputPortCSLFences(pOutputPort, requestId, pOutputBufferIndex, TRUE);
    }
    else
    {
        m_pPipeline->RegisterRequestFence(&rFenceHandlerData.hFence, requestId);

        if ((TRUE == IsBypassableNode()) && (ChiExternalNode == Type()))
        {
            m_pPipeline->RegisterRequestFence(&rDelayedBufferFenceHandlerData.hFence, requestId);
        }
        else
        {
            rDelayedBufferFenceHandlerData.hFence = CSLInvalidHandle;
        }
    }

    auto      hPipeline      = m_pPipeline->GetPipelineDescriptor();
    UINT      nodeId         = Type();
    UINT      nodeInstanceId = InstanceID();
    NodeFlags nodeFlags      = m_nodeFlags;
    BINARY_LOG(LogEvent::Node_SetupRequestOutputPort, hPipeline, nodeId, nodeInstanceId, nodeFlags, requestId, portId,
               rDelayedBufferFenceHandlerData, rFenceHandlerData);

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessSinkPortNewRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::ProcessSinkPortNewRequest(
    UINT64      requestId,
    UINT32      batchIndex,
    OutputPort* pOutputPort,
    UINT*       pOutputBufferIndex)
{
    CamxResult                  result            = CamxResultSuccess;
    const UINT32                numBatchedFrames  = GetBatchedHALOutputNum(pOutputPort->numBatchedFrames,
                                                                           pOutputPort->HALOutputBufferCombined);
    const UINT32                maxImageBuffers   = pOutputPort->bufferProperties.maxImageBuffers;
    const UINT32                fenceIdx          = (requestId % maxImageBuffers);
    NodeFenceHandlerData* const pFenceHandlerData = &pOutputPort->pFenceHandlerData[fenceIdx];

    if ((TRUE == IsInplace()) && (1 < numBatchedFrames))
    {
        /// @todo (CAMX-527) Input ports of this node need to fetch all-batched-HAL-output-buffers
        ///                  from the parent in GetOutputPortInfo(..)
        CAMX_NOT_IMPLEMENTED();
    }

    // Since nodes are walked by the topology in a random order, when some Node A requests output port information to some
    // other Node B, Node B may not have been called by the topology to process the request hence the node would not have
    // created the fence yet. If that is the case, create the fence here

    m_pFenceCreateReleaseLock->Lock();

    if (pFenceHandlerData->requestId != requestId)
    {
        result = SetupRequestOutputPortFence(pOutputPort, requestId, pOutputBufferIndex);

        if (CamxResultSuccess == result)
        {
            m_pFenceCreateReleaseLock->Lock();
            pFenceHandlerData->numOutputBuffers = 0;
            m_pFenceCreateReleaseLock->Unlock();
        }
    }

    m_pFenceCreateReleaseLock->Unlock();

    if (CamxResultSuccess == result)
    {
        const UINT   baseImageIndex   = ((requestId * numBatchedFrames) % maxImageBuffers);
        const UINT   imageBufferIndex = (baseImageIndex + batchIndex);
        ImageBuffer* pImageBuffer     = pOutputPort->ppImageBuffers[imageBufferIndex];

        if (NULL == pImageBuffer)
        {
            // SequenceId is ignored for SinkBuffers in the following method call
            const UINT32 sequenceId = 0;

            result = SetupRequestOutputPortImageBuffer(pOutputPort, requestId, sequenceId, imageBufferIndex);

            if (CamxResultSuccess == result)
            {
                pImageBuffer = pOutputPort->ppImageBuffers[imageBufferIndex];
                CAMX_ASSERT(NULL != pImageBuffer);
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s ImageBuffer is not NULL requestId %llu imageBufferIndex %d"
                " maxImageBuffers %d", NodeIdentifierString(), requestId, imageBufferIndex, maxImageBuffers);
        }

        if (CamxResultSuccess == result)
        {
            CAMX_ASSERT(1 >= m_deviceIndexCount); // buffer is output by 1 device only; device count is 0 for SW nodes
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ProcessNonSinkPortNewRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::ProcessNonSinkPortNewRequest(
    UINT64      requestId,
    UINT32      sequenceId,
    OutputPort* pOutputPort)
{
    CAMX_ASSERT((NULL != pOutputPort)                                &&
                (FALSE == pOutputPort->flags.isSinkBuffer)           &&
                (FALSE == pOutputPort->flags.isNonSinkHALBufferOutput));

    CamxResult result = CamxResultSuccess;

    if ((NULL != pOutputPort) && (pOutputPort->bufferProperties.maxImageBuffers > 0))
    {
        UINT                  imageFenceIndex   = requestId % pOutputPort->bufferProperties.maxImageBuffers;
        NodeFenceHandlerData* pFenceHandlerData = &pOutputPort->pFenceHandlerData[imageFenceIndex];
        BOOL                  isNewRequest      = FALSE;

        // Since nodes are walked by the topology in a random order, when some Node A requests output port information to some
        // other Node B, Node B may not have been called by the topology to process the request hence the node would not have
        // created the fence yet. If that is the case, create the fence here

        m_pFenceCreateReleaseLock->Lock();

        if (pFenceHandlerData->requestId != requestId)
        {
            result = SetupRequestOutputPortFence(pOutputPort, requestId, NULL);
            isNewRequest = TRUE;
        }

        m_pFenceCreateReleaseLock->Unlock();

        if (TRUE == isNewRequest)
        {
            if (CamxResultSuccess == result)
            {
                result = SetupRequestOutputPortImageBuffer(pOutputPort, requestId, sequenceId, imageFenceIndex);
            }
            if (CamxResultSuccess == result)
            {
                ImageBuffer* pNewImageBuffer = pOutputPort->ppImageBuffers[imageFenceIndex];
                m_pFenceCreateReleaseLock->Lock();
                /// @note This function is only called for non-sink ports that will have only one output buffer (even for HFR)
                pFenceHandlerData->numOutputBuffers                          = 1;
                pFenceHandlerData->pOutputBufferInfo[0].sequenceId           = sequenceId;
                pFenceHandlerData->pOutputBufferInfo[0].pImageBuffer         = pNewImageBuffer;
                pFenceHandlerData->delayedOutputBufferData.ppImageBuffer[0] = NULL;
                m_pFenceCreateReleaseLock->Unlock();
            }
        }
    }

    CAMX_ASSERT(CamxResultSuccess == result);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::NotifyPipelineCreated
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::NotifyPipelineCreated(
    BOOL isDefer)
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "Node::%s NotifyPipelineCreated", NodeIdentifierString());
    CamxResult result = CamxResultSuccess;

    if (TRUE == isDefer)
    {
        result =  OsUtils::ThreadCreate(DeferPipelineCreate,
                                        this,
                                        &m_hThreadPipelineCreate);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node[%s]: Failed to Create DeferPipelineCreate Thread ",
                NodeIdentifierString());
            result = PostPipelineCreate();
        }
    }
    else
    {
        result = PostPipelineCreate();
    }

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::PostPipelineCreate()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::CreateBufferManagers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::CreateBufferManagers()
{
    CamxResult result = CamxResultSuccess;
    UINT32 DT = 0;

    if ((Sensor != Type()) && (TRUE != m_skipUpdatingBufferProperties))
    {
        for (UINT outputPortIndex = 0; outputPortIndex < m_outputPortsData.numPorts; outputPortIndex++)
        {
            OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

            CAMX_ASSERT(pOutputPort->numInputPortsConnected >= pOutputPort->numInputPortsDisabled);
            // Unused port after buffer negotiation
            if (pOutputPort->numInputPortsConnected == pOutputPort->numInputPortsDisabled)
            {
                for (UINT i = 0; i < pOutputPort->bufferProperties.maxImageBuffers; i++)
                {
                    if (NULL != pOutputPort->pFenceHandlerData[i].pOutputBufferInfo)
                    {
                        CAMX_FREE(pOutputPort->pFenceHandlerData[i].pOutputBufferInfo);
                        pOutputPort->pFenceHandlerData[i].pOutputBufferInfo = NULL;
                    }
                    if (NULL != pOutputPort->pFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer)
                    {
                        CAMX_FREE(pOutputPort->pFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer);
                        pOutputPort->pFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer = NULL;
                    }
                }
                for (UINT i = 0; i < pOutputPort->bufferProperties.maxImageBuffers; i++)
                {
                    if (NULL != pOutputPort->pDelayedBufferFenceHandlerData[i].pOutputBufferInfo)
                    {
                        CAMX_FREE(pOutputPort->pDelayedBufferFenceHandlerData[i].pOutputBufferInfo);
                        pOutputPort->pDelayedBufferFenceHandlerData[i].pOutputBufferInfo = NULL;
                    }
                    if (NULL != pOutputPort->pDelayedBufferFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer)
                    {
                        CAMX_FREE(pOutputPort->pDelayedBufferFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer);
                        pOutputPort->pDelayedBufferFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer = NULL;
                    }
                }
                CAMX_FREE(pOutputPort->ppImageBuffers);
                CAMX_FREE(pOutputPort->pFenceHandlerData);
                CAMX_FREE(pOutputPort->pDelayedBufferFenceHandlerData);
                pOutputPort->ppImageBuffers                 = NULL;
                pOutputPort->pFenceHandlerData              = NULL;
                pOutputPort->pDelayedBufferFenceHandlerData = NULL;
                pOutputPort->bufferProperties.maxImageBuffers = 0;
            }

            if (pOutputPort->bufferProperties.maxImageBuffers > 0)
            {
                BOOL            isOutputHALBuffer = ((TRUE == IsSinkPort(outputPortIndex)) ||
                                                     (TRUE == IsNonSinkHALBufferOutput(outputPortIndex))) ? TRUE : FALSE;
                UINT            numBatchedFrames  = pOutputPort->numBatchedFrames;
                FormatParamInfo formatParamInfo   = {0};

                if (TRUE == IsSinkPort(outputPortIndex) ||
                    TRUE == IsNonSinkHALBufferOutput(outputPortIndex))
                {
                    ChiStreamWrapper* pChiStreamWrapper = NULL;
                    if (TRUE == IsNonSinkHALBufferOutput(outputPortIndex))
                    {
                        pChiStreamWrapper = pOutputPort->pChiStreamWrapper;
                    }
                    else
                    {
                        pChiStreamWrapper = m_pPipeline->GetOutputStreamWrapper(Type(),
                                                                                InstanceID(),
                                                                                pOutputPort->portId);
                    }

                    if (NULL != pChiStreamWrapper)
                    {
                        ChiStream* pChiStream           = reinterpret_cast<ChiStream*>(pChiStreamWrapper->GetNativeStream());

                        formatParamInfo.isHALBuffer     = isOutputHALBuffer;
                        formatParamInfo.pixelFormat     = pChiStream->format;
                        formatParamInfo.outputFormat    = GetStaticSettings()->outputFormat;
                        formatParamInfo.isImplDefined   = ((pChiStream->format == ChiStreamFormatImplDefined) ||
                                                           (pChiStream->format == ChiStreamFormatYCrCb420_SP)) ? TRUE : FALSE;
                        formatParamInfo.grallocUsage    = pChiStreamWrapper->GetGrallocUsage();
                        formatParamInfo.yuvPlaneAlign   = GetStaticSettings()->yuvPlaneAlignment;

                        if (TRUE == GetStaticSettings()->IsStrideSettingEnable)
                        {
                            formatParamInfo.planeStride = pChiStream->streamParams.planeStride;
                            formatParamInfo.sliceHeight = pChiStream->streamParams.sliceHeight;
                        }
                        else
                        {
                            formatParamInfo.planeStride = 0;
                            formatParamInfo.sliceHeight = 0;
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Node[%s] pChiStreamWrapper cannot be NULL!", NodeIdentifierString());
                        result = CamxResultEFailed;
                    }
                    if (Format::Jpeg == pOutputPort->bufferProperties.imageFormat.format)
                    {
                        HwCameraInfo cameraInfo;
                        HwEnvironment::GetInstance()->GetCameraInfo(GetPipeline()->GetCameraId(), &cameraInfo);
                        formatParamInfo.maxJPEGSize = cameraInfo.pHwEnvironmentCaps->JPEGMaxSizeInBytes;
                        formatParamInfo.debugDataSize = static_cast<UINT32>
                                                       (HAL3MetadataUtil::DebugDataSize(DebugDataType::AllTypes));
                        //  m_maxjpegsize is assigned when NodePropertyStitchMaxJpegSize property is defined for
                        //  Jpeg aggregator node in case of VR Dual Camera stiched snapshot
                        if (m_maxjpegsize != 0 && m_maxjpegsize > formatParamInfo.maxJPEGSize)
                        {
                            formatParamInfo.maxJPEGSize = m_maxjpegsize;
                        }
                    }
                }

                SetUBWCusageflags(pOutputPort, &pOutputPort->bufferProperties.imageFormat, &formatParamInfo);

                if (CamxResultSuccess == result)
                {
                    ImageFormat*      pImageFormat    = &pOutputPort->bufferProperties.imageFormat;
                    const SensorMode* pSensorModeData = NULL;

                    result = Node::GetSensorModeData(&pSensorModeData);
                    if ((CamxResultSuccess == result) && (NULL != pSensorModeData))
                    {
                        if (pSensorModeData->streamConfigCount > MaxSensorModeStreamConfigCount ||
                            pSensorModeData->streamConfigCount == 0)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s no StreamConfig ", NodeIdentifierString());
                            ImageFormatUtils::InitializeFormatParams(pImageFormat, &formatParamInfo);
                        }
                        else
                        {
                            for (UINT32 index= 0 ; index < pSensorModeData->streamConfigCount ; index++)
                            {
                                if (pSensorModeData->streamConfig[index].type == StreamType::IMAGE)
                                {
                                    formatParamInfo.bitWidth = pSensorModeData->streamConfig[index].dt;
                                    ImageFormatUtils::InitializeFormatParams(pImageFormat, &formatParamInfo);
                                }
                                else
                                {
                                    // Update bitWidth parameter only if it is known explicitly.
                                    // If it is not available, FormatUtils will use default value
                                    formatParamInfo.bitWidth = 0;
                                    ImageFormatUtils::InitializeFormatParams(pImageFormat, &formatParamInfo);
                                }
                            }
                        }
                    }
                    else
                    {
                        ImageFormatUtils::InitializeFormatParams(pImageFormat, &formatParamInfo);
                    }

                    BufferManagerCreateData createData = { };

                    createData.bufferProperties          = pOutputPort->bufferProperties;
                    createData.maxBufferCount            = GetMaximumNumOfBuffersToBeAllocated(
                                                           createData.bufferProperties.maxImageBuffers);
                    createData.immediateAllocBufferCount = GetNumOfBuffersToBeAllocatedImmediately(
                                                           createData.bufferProperties.immediateAllocImageBuffers,
                                                           createData.maxBufferCount);
                    createData.deviceCount               = OutputPortDeviceCount(outputPortIndex);

                    CAMX_ASSERT(CamxMaxDeviceIndex >= createData.deviceCount);

                    Utils::Memcpy(&createData.deviceIndices,
                                  OutputPortDeviceIndices(outputPortIndex),
                                  sizeof(INT32) * createData.deviceCount);

                    if (TRUE == pOutputPort->flags.isSecurePort)
                    {
                        CAMX_ASSERT(TRUE == IsSecureMode());
                        createData.bufferProperties.memFlags = (CSLMemFlagProtected | CSLMemFlagHw);

                        if ((FALSE == isOutputHALBuffer) && (TRUE == NonSinkPortDSPSecureAccessNeeded(pOutputPort)))
                        {
                            // Since this internal buffer will be accessed by DSP in secure mode (from a CHI external node),
                            // set DSPSecureAccess flag so that KMD selects the correct heap and set flags accordingly
                            // while allocating the buffers.
                            createData.bufferProperties.memFlags |= CSLMemFlagDSPSecureAccess;
                        }
                    }

                    // Dont allocate buffer memory if it is a sinkport i.e. we just need a wrapper ImageBuffer with no backing
                    // memory allocation
                    createData.allocateBufferMemory     = ((TRUE == isOutputHALBuffer) ? FALSE : TRUE);
                    // Stats parse output buffers are fake
                    if (StatsParse == Type())
                    {
                        createData.allocateBufferMemory = FALSE;
                    }
                    // This is a multiplier for allocated bufferSize; Dont care for sinkports for which buffer memory is not
                    // allocated

                    createData.numBatchedFrames                            = numBatchedFrames;
                    createData.bEnableLateBinding                          =
                        (TRUE == pOutputPort->flags.isLateBindingDisabled) ? FALSE :
                        GetStaticSettings()->enableImageBufferLateBinding;
                    createData.bDisableSelfShrinking                       =
                        ((TRUE == pOutputPort->flags.disableSelfShrinking) || (TRUE == pOutputPort->flags.isSecurePort)) ?
                        TRUE : FALSE;
                    createData.bNeedDedicatedBuffers                       = ((FALSE == createData.bEnableLateBinding) ||
                                                                              (TRUE  == createData.bDisableSelfShrinking)) ?
                                                                             TRUE : FALSE;
                    createData.bufferManagerType                           = BufferManagerType::CamxBufferManager;
                    createData.linkProperties.pNode                        = this;
                    createData.linkProperties.isPartOfRealTimePipeline     = m_pPipeline->HasIFENode();
                    createData.linkProperties.isPartOfPreviewVideoPipeline = m_pPipeline->HasIFENode();
                    createData.linkProperties.isPartOfSnapshotPipeline     = m_pPipeline->HasJPEGNode();
                    createData.linkProperties.isFromIFENode                = (0x10000 == m_nodeType) ? TRUE : FALSE;

                    CHAR bufferManagerName[MaxStringLength256];
                    OsUtils::SNPrintF(bufferManagerName, sizeof(bufferManagerName), "%s_OutputPortId%d_%s",
                                      NodeIdentifierString(),
                                      pOutputPort->portId, GetOutputPortName(pOutputPort->portId));

                    CAMX_LOG_VERBOSE(CamxLogGroupMemMgr,
                                     "[buf.camx] CreateImageBufferManager %s linkFlagLateBinding %d bEnableLateBinding %d "
                                     "bNeedDedicatedBuffers %d bDisableSelfShrinking %d allocateBuffer %d "
                                     "numBatchedFrames %d w x h: %d x %d",
                                     bufferManagerName, pOutputPort->flags.isLateBindingDisabled,
                                     createData.bEnableLateBinding, createData.bNeedDedicatedBuffers,
                                     createData.bDisableSelfShrinking, createData.allocateBufferMemory,
                                     createData.numBatchedFrames,
                                     createData.bufferProperties.imageFormat.width,
                                     createData.bufferProperties.imageFormat.height);

                    ImageFormat tempImageFormat = pOutputPort->bufferProperties.imageFormat;

                    if (TRUE == ImageFormatUtils::IsRAW(&tempImageFormat))
                    {
                        CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "BufferMgrCreate ,[%s] width=%d, height=%d, format=%d,"
                                        "raw stride %d raw scanline %d GetTotalSize %d",
                                        bufferManagerName,
                                        tempImageFormat.width,
                                        tempImageFormat.height,
                                        tempImageFormat.format,
                                        tempImageFormat.formatParams.rawFormat.stride,
                                        tempImageFormat.formatParams.rawFormat.sliceHeight,
                                        ImageFormatUtils::GetTotalSize(&tempImageFormat));
                    }

                    else
                    {
                        UINT numberOfPlanes = ImageFormatUtils::GetNumberOfPlanes(&tempImageFormat);
                        for (UINT planeIndex = 0; planeIndex < numberOfPlanes; planeIndex++)
                        {
                            CAMX_LOG_CONFIG(CamxLogGroupMemMgr, "BufferMgrCreate ,[%s] width=%d, height=%d, format=%d, "
                                            "stride %d scanline %d GetTotalSize %d",
                                             bufferManagerName,
                                             tempImageFormat.width,
                                             tempImageFormat.height,
                                             tempImageFormat.format,
                                             tempImageFormat.formatParams.yuvFormat[planeIndex].planeStride,
                                             tempImageFormat.formatParams.yuvFormat[planeIndex].sliceHeight,
                                             ImageFormatUtils::GetTotalSize(&tempImageFormat));
                        }
                    }

                    result = CreateImageBufferManager(bufferManagerName, &createData, &pOutputPort->pImageBufferManager);

                    if (CamxResultSuccess == result)
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupMemMgr, "Image Buffer Manager create success");
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupMemMgr, "[%s] Buffer Manager creation failed, result=%s",
                                       bufferManagerName, Utils::CamxResultToString(result))
                        break;

                    }
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::CreateImageBufferManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::CreateImageBufferManager(
    const CHAR*              pBufferManagerName,
    BufferManagerCreateData* pCreateData,
    ImageBufferManager**     ppImageBufferManager)
{
    CamxResult result = CamxResultSuccess;

    CAMX_LOG_INFO(CamxLogGroupMemMgr,
                  "BufferManagerName = %s, immediateAllocBufferCount = %d, maxBufferCount = %d",
                  pBufferManagerName,
                  pCreateData->immediateAllocBufferCount,
                  pCreateData->maxBufferCount);

    result = ImageBufferManager::Create(pBufferManagerName, pCreateData, ppImageBufferManager);

    if (CamxResultSuccess != result)
    {
        *ppImageBufferManager = NULL;;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::FillTuningModeData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::FillTuningModeData(
    VOID** ppTuningModeData)
{
    CamxResult    result  = CamxResultSuccess;
    UINT32        metaTag = 0;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera2.tuning.mode",
                                                      "TuningMode",
                                                      &metaTag);

    if (CamxResultSuccess == result)
    {
        UINT   props[]                         = { metaTag | InputMetadataSectionMask };
        VOID*  pData[CAMX_ARRAY_SIZE(props)]   = {0};
        UINT64 offsets[CAMX_ARRAY_SIZE(props)] = {0};
        GetDataList(props, pData, offsets, CAMX_ARRAY_SIZE(props));

        *ppTuningModeData = pData[0];
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DetermineBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DetermineBufferProperties()
{
    BOOL isAnyOutputPortNonSink = FALSE;

    for (UINT outputPortIndex = 0; outputPortIndex < m_outputPortsData.numPorts; outputPortIndex++)
    {
        if (FALSE == IsNonSinkHALBufferOutput(outputPortIndex))
        {
            isAnyOutputPortNonSink = TRUE;
            break;
        }
    }

    if (TRUE == isAnyOutputPortNonSink)
    {
        for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
        {
            if (NULL != m_bufferNegotiationData.pInputPortNegotiationData)
            {
                InputPortNegotiationData* pInputPortNegotiationData =
                    &m_bufferNegotiationData.pInputPortNegotiationData[inputPortIndex];

                InputPort* pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];

                if (FALSE == IsSourceBufferInputPort(inputPortIndex))
                {
                    Node* pParentNode           = pInputPort->pParentNode;
                    UINT  parentOutputPortIndex = pInputPort->parentOutputPortIndex;

                    pInputPortNegotiationData->pImageFormat = pParentNode->GetOutputPortImageFormat(parentOutputPortIndex);
                    pInputPortNegotiationData->inputPortId  = pInputPort->portId;

                    CAMX_ASSERT(NULL != pInputPortNegotiationData->pImageFormat);
                }
                else
                {
                    pInputPortNegotiationData->pImageFormat = &pInputPort->imageFormat;
                    pInputPortNegotiationData->inputPortId  = pInputPort->portId;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s pInputPortNegotiationData is NULL", NodeIdentifierString());
            }
        }

        FinalizeBufferProperties(&m_bufferNegotiationData);
        // Determine if any sink output port shares its buffer with an input port
        for (UINT outputPortIndex = 0; outputPortIndex < m_outputPortsData.numPorts; outputPortIndex++)
        {
            OutputPort* const pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];
            if ((TRUE == pOutputPort->flags.isSinkBuffer))
            {
                ChiStreamWrapper* pChiStreamWrapper = m_pPipeline->GetOutputStreamWrapper(Type(),
                                                                                          InstanceID(),
                                                                                          pOutputPort->portId);

                if (NULL == pChiStreamWrapper)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore,
                                   "%s Output Port: %u is connected as a sink but has a null stream wrapper!",
                                   NodeIdentifierString(),
                                   pOutputPort->portId);
                }
                else if (NULL != pChiStreamWrapper->GetBufferNegotiationOptions())
                {
                    pChiStreamWrapper->SetDimensions(pOutputPort->bufferProperties.imageFormat.width,
                                                     pOutputPort->bufferProperties.imageFormat.height);
                }
                const UINT numActiveInputs = pOutputPort->numInputPortsConnected - pOutputPort->numInputPortsDisabled;
                if (numActiveInputs > 1)
                {
                    pOutputPort->flags.isSharedSinkBuffer = TRUE;
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::FinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::FinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CAMX_TRACE_SYNC_BEGIN_F(CamxLogGroupCore, "Node::%s FinalizeInitialization", NodeIdentifierString());

    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != m_pPipeline);

    m_pInputPool             = m_pPipeline->GetPerFramePool(PoolType::PerFrameInput);
    m_pMainPool              = m_pPipeline->GetPerFramePool(PoolType::PerFrameResult);
    m_pEarlyMainPool         = m_pPipeline->GetPerFramePool(PoolType::PerFrameResultEarly);
    m_pInternalPool          = m_pPipeline->GetPerFramePool(PoolType::PerFrameInternal);
    m_pUsecasePool           = m_pPipeline->GetPerFramePool(PoolType::PerUsecase);
    m_pDebugDataPool         = pFinalizeInitializationData->pDebugDataPool;
    m_pThreadManager         = pFinalizeInitializationData->pThreadManager;
    m_pDRQ                   = pFinalizeInitializationData->pDeferredRequestQueue;
    m_hThreadJobFamilyHandle = pFinalizeInitializationData->hThreadJobFamilyHandle;

    CAMX_ASSERT(m_pInputPool != NULL);
    CAMX_ASSERT(m_pMainPool != NULL);
    CAMX_ASSERT(m_pEarlyMainPool != NULL);
    CAMX_ASSERT(m_pInternalPool != NULL);
    CAMX_ASSERT(m_pUsecasePool != NULL);
    CAMX_ASSERT(m_pDebugDataPool != NULL);

    CAMX_ASSERT(pFinalizeInitializationData->usecaseNumBatchedFrames > 0);

    if ((Sensor != Type()) && (TRUE != m_skipUpdatingBufferProperties))
    {
        for (UINT outputPortIndex = 0; outputPortIndex < m_outputPortsData.numPorts; outputPortIndex++)
        {
            OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

            if (TRUE == pOutputPort->flags.isBatchMode)
            {
                pOutputPort->numBatchedFrames           = pFinalizeInitializationData->usecaseNumBatchedFrames;
                pOutputPort->HALOutputBufferCombined    = pFinalizeInitializationData->HALOutputBufferCombined;
            }
            else
            {
                // When the node is created it should be initialized to 1 as the default value
                CAMX_ASSERT((1 == pOutputPort->numBatchedFrames) || (pOutputPort->flags.isSinkNoBuffer == TRUE));
            }

            if ((FALSE == IsSinkPortNoBuffer(outputPortIndex)) &&
               (pOutputPort->bufferProperties.maxImageBuffers > 0))
            {
                UINT requiredMaxImageBuffers = m_pPipeline->GetMaxLivePendingRequestsOfSession() +
                                               m_pPipeline->GetMaxPipelineDelay() + 1;

                // maxImageBuffers value should be atleast requiredMaxImageBuffers.
                // maxImageBuffers will be used as the depth of pFenceHandlerData pointer.
                if (pOutputPort->bufferProperties.maxImageBuffers < requiredMaxImageBuffers)
                {
                    CAMX_LOG_WARN(CamxLogGroupCore, "Node::%s maxImageBuffers(%d) from link properties should be atleast "
                        "requiredMaxImageBuffers(%d). So replacing maxImageBuffers with requiredMaxImageBuffers",
                        NodeIdentifierString(), pOutputPort->bufferProperties.maxImageBuffers, requiredMaxImageBuffers);
                    pOutputPort->bufferProperties.maxImageBuffers = requiredMaxImageBuffers;
                }
            }
            pOutputPort->bufferProperties.maxImageBuffers *= GetBatchedHALOutputNum(pOutputPort->numBatchedFrames,
                                                                                    pOutputPort->HALOutputBufferCombined);

            if (pOutputPort->bufferProperties.maxImageBuffers > 0)
            {
                pOutputPort->ppImageBuffers = static_cast<ImageBuffer**>(
                    CAMX_CALLOC(sizeof(ImageBuffer*) * pOutputPort->bufferProperties.maxImageBuffers));

                pOutputPort->pFenceHandlerData = static_cast<NodeFenceHandlerData*>(
                    CAMX_CALLOC(sizeof(NodeFenceHandlerData) * pOutputPort->bufferProperties.maxImageBuffers));

                pOutputPort->pDelayedBufferFenceHandlerData = static_cast<NodeFenceHandlerData*>(
                    CAMX_CALLOC(sizeof(NodeFenceHandlerData) * pOutputPort->bufferProperties.maxImageBuffers));

                if ((NULL == pOutputPort->ppImageBuffers) || (NULL == pOutputPort->pFenceHandlerData) ||
                    (NULL == pOutputPort->pDelayedBufferFenceHandlerData))
                {
                    CAMX_ASSERT_ALWAYS_MESSAGE("Node::Initialize Cannot allocate memory!");
                    result = CamxResultENoMemory;
                    break;
                }
                else
                {
                    UINT maxImageBuffers = pOutputPort->bufferProperties.maxImageBuffers;

                    for (UINT i = 0; i < maxImageBuffers; i++)
                    {
                        pOutputPort->pFenceHandlerData[i].pOutputBufferInfo =
                            static_cast<FenceHandlerBufferInfo*>(
                            CAMX_CALLOC(sizeof(FenceHandlerBufferInfo) * m_pPipeline->GetBatchedHALOutputNum()));

                        pOutputPort->pFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer =
                            static_cast<ImageBuffer**>(CAMX_CALLOC(sizeof(ImageBuffer*) *
                                                                   m_pPipeline->GetBatchedHALOutputNum()));

                        pOutputPort->pDelayedBufferFenceHandlerData[i].pOutputBufferInfo =
                            static_cast<FenceHandlerBufferInfo*>(
                            CAMX_CALLOC(sizeof(FenceHandlerBufferInfo) * m_pPipeline->GetBatchedHALOutputNum()));

                        pOutputPort->pDelayedBufferFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer =
                            static_cast<ImageBuffer**>(CAMX_CALLOC(sizeof(ImageBuffer*) *
                                                                   m_pPipeline->GetBatchedHALOutputNum()));

                        if ((NULL == pOutputPort->pFenceHandlerData[i].pOutputBufferInfo) ||
                            (NULL == pOutputPort->pFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer) ||
                            (NULL == pOutputPort->pDelayedBufferFenceHandlerData[i].pOutputBufferInfo) ||
                            (NULL == pOutputPort->pDelayedBufferFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer))
                        {
                            CAMX_ASSERT_ALWAYS_MESSAGE("Node::Initialize Cannot allocate memory!");
                            result = CamxResultENoMemory;
                            break;
                        }
                    }
                    for (UINT imageFenceIndex = 0; imageFenceIndex < maxImageBuffers; imageFenceIndex++)
                    {
                        NodeFenceHandlerData* pFenceHandlerData = &pOutputPort->pFenceHandlerData[imageFenceIndex];
                        NodeFenceHandlerData* pDelayedBufferFenceHandlerData =
                            &pOutputPort->pDelayedBufferFenceHandlerData[imageFenceIndex];

                        // ExtFenceHandlerData is what we pass to CSL when we register a callback
                        // (the external main fence handler), CSL simply passes ExtFenceHandlerData
                        // back to us in the external handler
                        pFenceHandlerData->nodeCSLFenceCallbackData.pNode            = this;
                        pFenceHandlerData->nodeCSLFenceCallbackData.pNodePrivateData = pFenceHandlerData;
                        pFenceHandlerData->pOutputPort                               = pOutputPort;

                        pDelayedBufferFenceHandlerData->nodeCSLFenceCallbackData.pNode            = this;
                        pDelayedBufferFenceHandlerData->nodeCSLFenceCallbackData.pNodePrivateData =
                            pDelayedBufferFenceHandlerData;
                        pDelayedBufferFenceHandlerData->isDelayedBufferFence                      = TRUE;
                        pDelayedBufferFenceHandlerData->pOutputPort                               = pOutputPort;
                    }
                }
            }
            else
            {
                for (UINT i = 0; i < pOutputPort->bufferProperties.maxImageBuffers; i++)
                {
                    if (NULL != pOutputPort->pFenceHandlerData[i].pOutputBufferInfo)
                    {
                        CAMX_FREE(pOutputPort->pFenceHandlerData[i].pOutputBufferInfo);
                        pOutputPort->pFenceHandlerData[i].pOutputBufferInfo = NULL;
                    }
                    if (NULL != pOutputPort->pFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer)
                    {
                        CAMX_FREE(pOutputPort->pFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer);
                        pOutputPort->pFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer = NULL;
                    }
                }
                for (UINT i = 0; i < pOutputPort->bufferProperties.maxImageBuffers; i++)
                {
                    if (NULL != pOutputPort->pDelayedBufferFenceHandlerData[i].pOutputBufferInfo)
                    {
                        CAMX_FREE(pOutputPort->pDelayedBufferFenceHandlerData[i].pOutputBufferInfo);
                        pOutputPort->pDelayedBufferFenceHandlerData[i].pOutputBufferInfo = NULL;
                    }
                    if (NULL != pOutputPort->pDelayedBufferFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer)
                    {
                        CAMX_FREE(pOutputPort->pDelayedBufferFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer);
                        pOutputPort->pDelayedBufferFenceHandlerData[i].delayedOutputBufferData.ppImageBuffer = NULL;
                    }
                }
                CAMX_FREE(pOutputPort->ppImageBuffers);
                CAMX_FREE(pOutputPort->pFenceHandlerData);
                CAMX_FREE(pOutputPort->pDelayedBufferFenceHandlerData);
                pOutputPort->ppImageBuffers                 = NULL;
                pOutputPort->pFenceHandlerData              = NULL;
                pOutputPort->pDelayedBufferFenceHandlerData = NULL;
            }
        }

        if (CamxResultSuccess == result)
        {
            for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
            {
                if (TRUE == IsSourceBufferInputPort(inputPortIndex))
                {
                    InputPort*        pInputPort        = &m_inputPortsData.pInputPorts[inputPortIndex];
                    UINT              inputPortId       = pInputPort->portId;
                    ChiStreamWrapper* pChiStreamWrapper = m_pPipeline->GetInputStreamWrapper(Type(), InstanceID(),
                                                              inputPortId);

                    if (NULL != pChiStreamWrapper)
                    {
                        pInputPort->sourceTargetStreamId = pChiStreamWrapper->GetStreamIndex();
                        InitializeSourcePortBufferFormat(inputPortIndex, pChiStreamWrapper);
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s pChiStreamWrapper for Port Index at %d is null",
                                       NodeIdentifierString(), inputPortIndex);
                        result = CamxResultEInvalidPointer;
                        break;
                    }
                }
            }
        }
    }

    if (CamxResultSuccess == result)
    {
        result = ProcessingNodeFinalizeInitialization(pFinalizeInitializationData);
    }

    CAMX_TRACE_SYNC_END(CamxLogGroupCore);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SwitchNodeOutputFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SwitchNodeOutputFormat(
    Format format)
{
    CamxResult result = CamxResultSuccess;

    for (UINT outputPortIndex = 0; outputPortIndex < m_outputPortsData.numPorts; outputPortIndex++)
    {
        /// @todo (CAMX-1797) Loop thru in the Chi override client
        /// Buffer requirements failed with current format, switch format to NV12 and try again. Ideally, we should loop thru
        /// all valid supported formats

        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        if (TRUE == pOutputPort->flags.isSinkBuffer)
        {
            ChiStreamWrapper* pChiStreamWrapper =
                m_pPipeline->GetOutputStreamWrapper(Type(), InstanceID(), pOutputPort->portId);

            if (NULL != pChiStreamWrapper)
            {
                ChiStream*        pChiStream        = reinterpret_cast<ChiStream*>(pChiStreamWrapper->GetNativeStream());
                Format            selectedFormat    = pChiStreamWrapper->GetInternalFormat();

                if ((pChiStream->format == static_cast<CHISTREAMFORMAT>(HALPixelFormatImplDefined)) &&
                    (TRUE == ImageFormatUtils::IsUBWC(selectedFormat)))
                {
                    GrallocUsage64 grallocUsage = pChiStream->pHalStream->producerUsage;
                    if (pChiStream->streamType == ChiStreamTypeInput)
                    {
                        grallocUsage = pChiStream->pHalStream->consumerUsage;
                    }
                    // Switching from UBWC, update the gralloc flags appropriately
                    grallocUsage &= ~(GrallocUsagePrivate0);
                    grallocUsage &= ~(GrallocUsagePrivate3);
                    if (pChiStream->streamType == ChiStreamTypeInput)
                    {
                        pChiStreamWrapper->SetNativeConsumerGrallocUsage(grallocUsage);
                    }
                    else
                    {
                        pChiStreamWrapper->SetNativeProducerGrallocUsage(grallocUsage);
                    }

                    CAMX_LOG_INFO(CamxLogGroupCore,
                                  "Unable to process with format %d, falling back to %d",
                                  pChiStreamWrapper->GetInternalFormat(),
                                  format);

                    pChiStreamWrapper->SetInternalFormat(format);

                    if (ChiExternalNode == m_nodeType)
                    {
                        m_pChiContext->SetChiStreamInfo(pChiStreamWrapper, m_pPipeline->GetBatchedHALOutputNum(), TRUE);
                    }
                    else
                    {
                        m_pChiContext->SetChiStreamInfo(pChiStreamWrapper, m_pPipeline->GetBatchedHALOutputNum(), FALSE);
                    }

                    OutputPortNegotiationData* pOutputPortNegotiationData =
                        &(m_bufferNegotiationData.pOutputPortNegotiationData[outputPortIndex]);

                    pOutputPortNegotiationData->pFinalOutputBufferProperties->imageFormat.format = format;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s pChiStreamWrapper for Port Index at %d is null",
                               NodeIdentifierString(), outputPortIndex);
                result = CamxResultEInvalidPointer;
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ResetBufferNegotiationData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::ResetBufferNegotiationData()
{
    CAMX_LOG_VERBOSE(CamxLogGroupCore,
                     "node %d instance %d outputports %d ",
                     Type(),
                     InstanceID(),
                     m_bufferNegotiationData.numOutputPortsNotified);

    m_bufferNegotiationData.numOutputPortsNotified = 0;

    for (UINT outputPortIndex = 0; outputPortIndex < m_outputPortsData.numPorts; outputPortIndex++)
    {
        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        OutputPortNegotiationData* pOutputPortNegotiationData =
            &m_bufferNegotiationData.pOutputPortNegotiationData[outputPortIndex];

        pOutputPortNegotiationData->numInputPortsNotification = 0;

        if ((TRUE == pOutputPort->flags.isSinkBuffer) || (TRUE == pOutputPort->flags.isSinkNoBuffer))
        {
            pOutputPortNegotiationData->numInputPortsNotification++;

            if (pOutputPortNegotiationData->numInputPortsNotification == pOutputPort->numInputPortsConnected)
            {
                m_bufferNegotiationData.numOutputPortsNotified++;
            }
        }
        else if (TRUE == pOutputPort->flags.isLoopback)
        {
            pOutputPortNegotiationData->numInputPortsNotification++;
            if (pOutputPortNegotiationData->numInputPortsNotification == pOutputPort->numInputPortsConnected)
            {
                m_bufferNegotiationData.numOutputPortsNotified++;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetTuningDataManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TuningDataManager* Node::GetTuningDataManager()
{
    return m_pPipeline->GetTuningDataManager();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetTuningDataManagerWithCameraId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
TuningDataManager* Node::GetTuningDataManagerWithCameraId(
    UINT32 cameraId)
{
    return HwEnvironment::GetInstance()->GetTuningDataManager(cameraId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetIntraPipelinePerFramePool
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MetadataPool* Node::GetIntraPipelinePerFramePool(
    PoolType poolType,
    UINT     intraPipelineId)
{
    return m_pPipeline->GetIntraPipelinePerFramePool(poolType, intraPipelineId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::FillPipelineInputOptions
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Node::FillPipelineInputOptions(
    ChiPipelineInputOptions* pPipelineInputOptions)
{
    UINT numInputs = 0;

    if (Sensor == Type())
    {
        /// @note Sensor does not have input ports so the requirements are kept on the output ports
        numInputs = m_outputPortsData.numPorts;

        for (UINT output = 0; output < m_outputPortsData.numPorts; output++)
        {
            if (SensorOutputPort0 != m_outputPortsData.pOutputPorts[output].portId)
            {
                // Decrement for the skipped sensor node output port.
                numInputs--;
                continue;
            }
            OutputPortNegotiationData* pOutputPortNegotiationData = &m_bufferNegotiationData.pOutputPortNegotiationData[output];
            BufferRequirement*         pInputPortBuffer           = &pOutputPortNegotiationData->inputPortRequirement[0];
            UINT                       outputPortIndex            = pOutputPortNegotiationData->outputPortIndex;
            ChiBufferOptions*          pChiInputOptions           = &pPipelineInputOptions->bufferOptions;

            pPipelineInputOptions->nodePort.nodeId         = Type();
            pPipelineInputOptions->nodePort.nodeInstanceId = InstanceID();
            pPipelineInputOptions->nodePort.nodePortId     = m_outputPortsData.pOutputPorts[outputPortIndex].portId;

            pChiInputOptions->size                     = sizeof(ChiBufferOptions);
            pChiInputOptions->minDimension.width       = pInputPortBuffer->minWidth;
            pChiInputOptions->minDimension.height      = pInputPortBuffer->minHeight;
            pChiInputOptions->maxDimension.width       = pInputPortBuffer->maxWidth;
            pChiInputOptions->maxDimension.height      = pInputPortBuffer->maxHeight;
            pChiInputOptions->optimalDimension.width   = pInputPortBuffer->optimalWidth;
            pChiInputOptions->optimalDimension.height  = pInputPortBuffer->optimalHeight;

            pPipelineInputOptions++;
        }
    }
    else
    {
        numInputs = 0;

        for (UINT input = 0; input < m_inputPortsData.numPorts; input++)
        {
            if (TRUE == IsSourceBufferInputPort(input))
            {
                PortBufferOptions* pInputBufferOptions = &m_bufferNegotiationData.inputBufferOptions[input];
                BufferRequirement* pInputPortBuffer    = &pInputBufferOptions->bufferRequirement;
                ChiBufferOptions*  pChiInputOptions    = &pPipelineInputOptions->bufferOptions;

                pPipelineInputOptions->nodePort.nodeId         = pInputBufferOptions->nodeId;
                pPipelineInputOptions->nodePort.nodeInstanceId = pInputBufferOptions->instanceId;
                pPipelineInputOptions->nodePort.nodePortId     = pInputBufferOptions->portId;

                pChiInputOptions->size                     = sizeof(ChiBufferOptions);
                pChiInputOptions->minDimension.width       = pInputPortBuffer->minWidth;
                pChiInputOptions->minDimension.height      = pInputPortBuffer->minHeight;
                pChiInputOptions->maxDimension.width       = pInputPortBuffer->maxWidth;
                pChiInputOptions->maxDimension.height      = pInputPortBuffer->maxHeight;
                pChiInputOptions->optimalDimension.width   = pInputPortBuffer->optimalWidth;
                pChiInputOptions->optimalDimension.height  = pInputPortBuffer->optimalHeight;

                pPipelineInputOptions++;
                numInputs++;
            }
        }
    }

    return numInputs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::EarlyReturnTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Node::EarlyReturnTag(
    UINT64       requestId,
    UINT32       tag,
    const VOID*  pData,
    SIZE_T       size)
{
    BOOL earlyReturn = FALSE;
    static const UINT SizeOfFixedList = 13;
    switch (tag)
    {
        case ControlAWBState:
        case ControlAFState:
        case ControlAEState:
        case ControlAELock:
        case ControlAEExposureCompensation:
        case ControlAFMode:
        case ControlAWBMode:
        case ControlAETargetFpsRange:
        case ControlAERegions:
        case ControlAFRegions:
        case ControlMode:
        case FlashMode:
        case FlashState:
        {
            MetadataSlot* pSlot = NULL;
            pSlot = m_pEarlyMainPool->GetSlot(requestId);
            pSlot->SetMetadataByTag(tag, pData, size, NodeIdentifierString());

            m_pEarlyMainPool->IncCurrentEarlyTagsCountForSlot(requestId);

            if (m_pEarlyMainPool->GetCurrentEarlyTagsCountForSlot(requestId) >= SizeOfFixedList)
            {
                ProcessEarlyMetadataDone(requestId);

                m_pEarlyMainPool->ResetCurrentEarlyTagsCountForSlot(requestId);
            }
            break;
        }
        default:
            break;
    }

    return earlyReturn;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ReleaseImageBufferReference
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Node::ReleaseOutputPortImageBufferReference(
    OutputPort* const   pOutputPort,
    const UINT64        requestId,
    const UINT          batchIndex)
{
    UINT                  referenceCount    = 0;
    const OutputPortFlags portFlags         = pOutputPort->flags;
    const UINT            numBatchedFrames  = GetBatchedHALOutputNum(pOutputPort->numBatchedFrames,
                                                                     pOutputPort->HALOutputBufferCombined);
    const UINT            maxImageBuffers   = pOutputPort->bufferProperties.maxImageBuffers;
    const UINT            imageBufferIndex  = (TRUE == portFlags.isSinkBuffer)
                                                  ? ((requestId * numBatchedFrames) % maxImageBuffers) + batchIndex
                                                  : requestId % maxImageBuffers;

    ImageBuffer*          const pImageBuffer      = pOutputPort->ppImageBuffers[imageBufferIndex];
    NodeFenceHandlerData* const pFenceHandlerData = &pOutputPort->pFenceHandlerData[(requestId % maxImageBuffers)];


    if (CamxInvalidRequestId == requestId)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Cannot release output reference for invalid request id!");
    }
    else if (NULL != pImageBuffer)
    {
        if (TRUE == portFlags.isSinkBuffer)
        {
            FenceHandlerBufferInfo* const outputBufferInfo = &pFenceHandlerData->pOutputBufferInfo[batchIndex];
            const ImageFormat*            pSinkImageBufferFormat = pImageBuffer->GetFormat();

            if (NULL != pSinkImageBufferFormat)
            {
                Format sinkImageBufferFormat = pSinkImageBufferFormat->format;

                if ((0 == batchIndex) &&
                    (TRUE == ImageFormatUtils::IsUBWC(sinkImageBufferFormat)))
                {
                    ProcessFrameDataCallBack(requestId,
                        pOutputPort->portId,
                        &outputBufferInfo->bufferInfo,
                        pSinkImageBufferFormat);
                }
            }

        }
        // Add m_pBufferReleaseLock in case of the Release make no effort
        m_pBufferReleaseLock->Lock();
        // Before releasing the last reference, make sure to Release backing buffers for sink/nonSinkHAL ImageBuffers
        if (1 == pImageBuffer->GetReferenceCount())
        {
            if ((TRUE == portFlags.isSinkBuffer) || (TRUE == portFlags.isNonSinkHALBufferOutput))
            {
                // HAL buffer can now be unmapped since the HW is done generating the output
                CAMX_ASSERT(NULL != pFenceHandlerData->pOutputBufferInfo);
                CAMX_ASSERT(NULL != pFenceHandlerData->pOutputBufferInfo[batchIndex].pImageBuffer);
                pFenceHandlerData->pOutputBufferInfo[batchIndex].pImageBuffer->Release(FALSE);
            }
        }

        referenceCount = pImageBuffer->ReleaseBufferManagerImageReference();

        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                         "Node: %08x, Node::%s Output Port ID: %d, "
                         "has released an ImageBuffer reference | Buffer: %p, Count: %d, Fence: %d, RequestId: %llu",
                         this,
                         NodeIdentifierString(),
                         pOutputPort->portId,
                         pImageBuffer,
                         referenceCount,
                         static_cast<INT32>(pFenceHandlerData->hFence),
                         pFenceHandlerData->requestId);

        if (0 == referenceCount)
        {
            pOutputPort->ppImageBuffers[imageBufferIndex] = NULL;
        }
        m_pBufferReleaseLock->Unlock();
    }

    NotifyAndReleaseFence(pOutputPort, requestId);

    return referenceCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ReleaseOutputPortCSLFences
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::ReleaseOutputPortCSLFences(
    OutputPort* const   pOutputPort,
    const UINT64        requestId,
    UINT*               pOutputBufferIndex,
    BOOL                checkIsPrimaryFenceBeforeRelease)
{
    CamxResult      result                 = CamxResultSuccess;
    UINT            imageFenceIndex        = requestId % pOutputPort->bufferProperties.maxImageBuffers;
    CSLFence        hOldFence              = pOutputPort->pFenceHandlerData[imageFenceIndex].hFence;
    CSLFence        hOldDelayedBufferFence = pOutputPort->pDelayedBufferFenceHandlerData[imageFenceIndex].hFence;
    BOOL            ChiFenceExists         = FALSE;

    if (NULL != pOutputBufferIndex)
    {
        CaptureRequest* pCaptureRequest = m_pPipeline->GetRequest(requestId);

        CAMX_ASSERT_MESSAGE((pCaptureRequest->requestId == requestId),
            "(pCaptureRequest->requestId:%llu == requestId:%llu)",
            pCaptureRequest->requestId,
            requestId);
    }

    ChiFenceExists = pOutputPort->pFenceHandlerData[imageFenceIndex].isChiFence;

    if ((FALSE == ChiFenceExists) && (TRUE == IsValidCSLFence(hOldFence)))
    {
        // If checkIsPrimaryFenceBeforeRelease is set to FALSE no need to check for primary fence,
        // Only check if fence is primary if checkIsPrimaryFenceBeforeRelease is TRUE.
        // checkIsPrimaryFenceBeforeRelease is set to TRUE when this function is called from SetupRequestOutputPortFence.
        if ((FALSE == checkIsPrimaryFenceBeforeRelease) ||
            (TRUE == pOutputPort->pFenceHandlerData[imageFenceIndex].primaryFence))
        {
            result = CSLReleaseFence(hOldFence);

            CAMX_LOG_DRQ("ReleaseFence(Output)...Node: %08x, Node::%s fence: %08x(%d), reqId: %llu, ImgBuf: %p, result: %d %s",
                         this,
                         NodeIdentifierString(),
                         &pOutputPort->pFenceHandlerData[imageFenceIndex].hFence,
                         hOldFence,
                         requestId,
                         pOutputPort->ppImageBuffers[imageFenceIndex],
                         result,
                         ((CamxResultSuccess == result) ? "" : "(Error)"));
        }

        pOutputPort->pFenceHandlerData[imageFenceIndex].hFence = CSLInvalidHandle;
        CAMX_ASSERT_MESSAGE((CamxResultSuccess == result), "Fence release failed");
    }
    else if (TRUE == ChiFenceExists)
    {
        /*/
        /// @note: CHI fences are to be released by CHI Override
        /*/
    }

    if (CSLInvalidHandle != hOldDelayedBufferFence)
    {
        // If checkIsPrimaryFenceBeforeRelease is set to FALSE no need to check for primary fence,
        // Only check if fence is primary if checkIsPrimaryFenceBeforeRelease is TRUE.
        // checkIsPrimaryFenceBeforeRelease is set to TRUE when this function is called from SetupRequestOutputPortFence.
        if ((FALSE == checkIsPrimaryFenceBeforeRelease) ||
            (TRUE  == pOutputPort->pDelayedBufferFenceHandlerData[imageFenceIndex].primaryFence))
        {
            result = CSLReleaseFence(hOldDelayedBufferFence);

            CAMX_LOG_DRQ("ReleaseFence(DelayedBuffer)...Node: %08x, Node::%s fence: %08x(%d) reqID:%llu",
                         this,
                         NodeIdentifierString(),
                         &pOutputPort->pDelayedBufferFenceHandlerData[imageFenceIndex].hFence,
                         pOutputPort->pDelayedBufferFenceHandlerData[imageFenceIndex].hFence, requestId);
        }

        pOutputPort->pDelayedBufferFenceHandlerData[imageFenceIndex].hFence = CSLInvalidHandle;
        CAMX_ASSERT(CamxResultSuccess == result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::NotifyAndReleaseFence
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::NotifyAndReleaseFence(
    OutputPort* const   pOutputPort,
    UINT64 requestId)
{
    UINT32 groupIDRequestIndex = requestId % RequestQueueDepth;

    UINT        imageFenceIndex = requestId % pOutputPort->bufferProperties.maxImageBuffers;
    CSLFence    hFence          = pOutputPort->pFenceHandlerData[imageFenceIndex].hFence;

    UINT groupID = m_bufferComposite.portGroupID[pOutputPort->portId];

    // Find the output buffer index
    UINT                outputBufferIndex   = 0;
    const UINT          batchIndex          = 0;
    CaptureRequest*     pCaptureRequest     = m_pPipeline->GetRequest(m_tRequestId);
    StreamBufferInfo*   pStreamBufferInfo   = &(pCaptureRequest->pStreamBuffers[batchIndex]);

    if (CSLInvalidFence != m_portFenceInfo[groupIDRequestIndex][groupID].hFence)
    {
        for (UINT32 i = 0; i < pStreamBufferInfo->numOutputBuffers; i++)
        {
            if ((TRUE == pStreamBufferInfo->outputBuffers[i].releaseFence.valid) &&
                (ChiFenceTypeInternal == pStreamBufferInfo->outputBuffers[i].releaseFence.type) &&
                (reinterpret_cast<ChiFence*>(pStreamBufferInfo->outputBuffers[i].releaseFence.hChiFence)->hFence == hFence))
            {
                outputBufferIndex = i;
                break;
            }
        }

        if (m_portFenceInfo[groupIDRequestIndex][groupID].hFence == hFence)
        {
            UINT32 refcount = CamxAtomicDecU(&m_portFenceInfo[groupIDRequestIndex][groupID].aRefCount);
            if (0 == refcount)
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCore, "%s, Ref count for Fence %d, is zero. Release the fence, requestId %llu",
                                 NodeIdentifierString(), hFence, requestId);
                m_pFenceCreateReleaseLock->Lock();
                auto tempResult = ReleaseOutputPortCSLFences(pOutputPort, requestId, &outputBufferIndex, FALSE);
                m_portFenceInfo[groupIDRequestIndex][groupID].hFence = CSLInvalidFence;

                // Invalidate primary port fence. In case composite fence references are decremented after
                // primary fence and fence is released by composite port, then primary port fence should not  be released
                // again in SetupRequestOutputPortFence. Hence, invalidating  primary port fence.
                OutputPort* pPrimaryOutputPort = m_portFenceInfo[groupIDRequestIndex][groupID].pPrimaryOutputPort;
                if ((NULL != pPrimaryOutputPort) && (NULL != pPrimaryOutputPort->pFenceHandlerData))
                {
                    pPrimaryOutputPort->pFenceHandlerData[imageFenceIndex].hFence = CSLInvalidHandle;
                }

                m_pFenceCreateReleaseLock->Unlock();

                CAMX_ASSERT(CamxResultSuccess == tempResult);
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupCore, "%s Ref count for Fence %d is %d, requestId %llu",
                                 NodeIdentifierString(),
                                 hFence,
                                 refcount,
                                 requestId);
            }
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Not able to find a mapping fence %d, Node %s, Req %llu",
                             hFence,
                             Name(),
                             requestId);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::NotifyOutputConsumed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::NotifyOutputConsumed(
    UINT   outputPortIndex,
    UINT64 requestId)
{
    m_pBufferReleaseLock->Lock();

    OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

    if (pOutputPort->bufferProperties.maxImageBuffers > 0)
    {
        const UINT releaseCount = (TRUE == pOutputPort->flags.isSinkBuffer) ?
            GetBatchedHALOutputNum(pOutputPort->numBatchedFrames, pOutputPort->HALOutputBufferCombined) : 1;
        for (UINT batchIndex = 0; batchIndex < releaseCount; batchIndex++)
        {
            ReleaseOutputPortImageBufferReference(pOutputPort, requestId, batchIndex);
        }
    }

    if (TRUE == IsBypassableNode())
    {
        for (UINT index = 0; index < pOutputPort->numSourcePortsMapped; index++)
        {
            UINT32 portId = pOutputPort->pMappedSourcePortIds[index];
            for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
            {
                InputPort* pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];
                if (portId == pInputPort->portId)
                {
                    NotifyInputConsumed(inputPortIndex, requestId);
                }
            }
        }
    }
    m_pBufferReleaseLock->Unlock();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::NotifyInputConsumed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::NotifyInputConsumed(
    UINT   inputPortIndex,
    UINT64 requestId)
{
    InputPort* pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];

    if (FALSE == IsSourceBufferInputPort(inputPortIndex))
    {
        pInputPort->pParentNode->NotifyOutputConsumed(pInputPort->parentOutputPortIndex, requestId);
    }
    else
    {
        m_pFenceCreateReleaseLock->Lock();
        UINT         requestIdIndex = requestId % MaxRequestQueueDepth;
        ImageBuffer* pImageBuffer   = pInputPort->ppImageBuffers[requestIdIndex];

        CAMX_ASSERT(NULL != pImageBuffer);

        if ((TRUE == IsBypassableNode()) && (1 < pImageBuffer->GetReferenceCount()))
        {
            pImageBuffer->ReleaseBufferManagerImageReference();
        }

        if ((TRUE == pImageBuffer->HasBackingBuffer()) && (1 == pImageBuffer->GetReferenceCount()))
        {
            pImageBuffer->Release(FALSE);

            CAMX_ASSERT(CSLInvalidHandle != pInputPort->phFences[requestIdIndex]);

            CamxResult result = CSLReleaseFence(pInputPort->phFences[requestIdIndex]);

            CAMX_LOG_DRQ("ReleaseFence(Input)...Node: %08x, Node::%s fence: %08x(%d), result: %d reqID:%llu",
                         this,
                         NodeIdentifierString(),
                         &pInputPort->phFences[requestIdIndex],
                         pInputPort->phFences[requestIdIndex],
                         result,
                         requestId);

            CAMX_ASSERT(result == CamxResultSuccess);

            pInputPort->phFences[requestIdIndex] = CSLInvalidHandle;
        }
        m_pFenceCreateReleaseLock->Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetNumOfBuffersToBeAllocatedImmediately
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Node::GetNumOfBuffersToBeAllocatedImmediately(
    UINT immediateBufferCount,
    UINT maxBufferCount)
{
    UINT nNumOfBuffers = immediateBufferCount;

    // Ensure that the immediateBufferCount is not greater than the max number of buffers
    if (nNumOfBuffers > maxBufferCount)
    {
        nNumOfBuffers = maxBufferCount;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "Preallocating %d buffers for Node::%s, instance = %d",
                     nNumOfBuffers, NodeIdentifierString(), InstanceID());
    return nNumOfBuffers;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetMaximumNumOfBuffersToBeAllocated
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Node::GetMaximumNumOfBuffersToBeAllocated(
    UINT maxBufferCount)
{
    UINT nMaxNumOfBuffers = maxBufferCount;

    const StaticSettings* pSettings = GetStaticSettings();
    CAMX_ASSERT(NULL != pSettings);

    if (NULL != pSettings)
    {
        if (TRUE == IsRealTime())
        {
            // If the max image buffer count override value for real-time nodes is set to a number smaller than
            // the maximum possible size, the maximum number of buffers to be allocated is updated accordingly
            if (static_cast<UINT>(-1) != pSettings->overrideMaxImageBufferCountRealTime)
            {
                nMaxNumOfBuffers = pSettings->overrideMaxImageBufferCountRealTime;

                // Ensure that the max number of buffers allocated is not greater than the value in the buffer property setting
                if (nMaxNumOfBuffers > maxBufferCount)
                {
                    nMaxNumOfBuffers = maxBufferCount;
                }

                // Ensure that the max number of buffers allocated is greater than the max pipeline delay
                if (nMaxNumOfBuffers <= GetMaximumPipelineDelay())
                {
                    nMaxNumOfBuffers = GetMaximumPipelineDelay() + 1;
                }
            }
        }
        else
        {
            // If the max image buffer count override value for non-real time nodes is set to a number smaller than
            // the maximum possible size, the maximum number of buffers to be allocated is updated accordingly
            if (static_cast<UINT>(-1) != pSettings->overrideMaxImageBufferCountNonRealTime)
            {
                nMaxNumOfBuffers = pSettings->overrideMaxImageBufferCountNonRealTime;

                // Ensure that the max number of buffers allocated is not greater than the value in the buffer property setting
                if (nMaxNumOfBuffers > maxBufferCount)
                {
                    nMaxNumOfBuffers = maxBufferCount;
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s NULL pointer check for static settings failed", NodeIdentifierString());
    }

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "Maximum %d buffers will be allocated for Node::%s",
                     nMaxNumOfBuffers, NodeIdentifierString());

    return nMaxNumOfBuffers;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetStaticSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const StaticSettings* Node::GetStaticSettings() const
{
    return HwEnvironment::GetInstance()->GetStaticSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetInputPortBufferDelta
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::SetInputPortBufferDelta(
    UINT    portIndex,
    UINT32  delta)
{
    if (portIndex < m_inputPortsData.numPorts)
    {
        m_inputPortsData.pInputPorts[portIndex].bufferDelta = delta;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Invalid port index %u, numPorts %u",
                       NodeIdentifierString(), portIndex, m_inputPortsData.numPorts);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::UpdateBufferInfoforPendingInputPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::UpdateBufferInfoforPendingInputPorts(
    PerRequestActivePorts* pRequestPorts)
{
    CamxResult result = CamxResultSuccess;

    for (UINT8 index = 0; index < pRequestPorts->numInputPorts; index++)
    {
        if (TRUE == pRequestPorts->pInputPorts[index].flags.isPendingBuffer)
        {
            DelayedOutputBufferInfo* pDelayedBufferData = pRequestPorts->pInputPorts[index].pDelayedOutputBufferData;

            if (NULL != pDelayedBufferData)
            {
                pRequestPorts->pInputPorts[index].pImageBuffer = pDelayedBufferData->ppImageBuffer[0];
                pRequestPorts->pInputPorts[index].phFence      = &pDelayedBufferData->hFence;

                pRequestPorts->pInputPorts[index].flags.isParentInputBuffer = pDelayedBufferData->isParentInputBuffer;
                pRequestPorts->pInputPorts[index].flags.isPendingBuffer     = FALSE;
            }
            else
            {
                CAMX_LOG_DRQ("Node::%s Invalid delayed buffer data request = %llu", NodeIdentifierString(), m_tRequestId);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetPendingBufferDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SetPendingBufferDependency(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CamxResult result = CamxResultSuccess;

    PerRequestActivePorts*  pEnabledPorts    = pExecuteProcessRequestData->pEnabledPortsInfo;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    DependencyUnit*         pDependency      = &pNodeRequestData->dependencyInfo[pNodeRequestData->numDependencyLists];

    CAMX_ASSERT_MESSAGE(NULL != pNodeRequestData, "Node capture request data NULL pointer");
    CAMX_ASSERT_MESSAGE(NULL != pEnabledPorts, "Per Request Active Ports NULL pointer");

    for (UINT portIndex = 0; portIndex < pEnabledPorts->numInputPorts; portIndex++)
    {
        PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[portIndex];
        CAMX_ASSERT_MESSAGE(NULL != pPerRequestInputPort, "Per Request Input Port NULL pointer");

        if (TRUE == pPerRequestInputPort->flags.isPendingBuffer)
        {
            if (0 == CamxAtomicLoadU(pPerRequestInputPort->pIsFenceSignaled))
            {
                UINT fenceCount = pDependency->bufferDependency.fenceCount;

                pDependency->bufferDependency.phFences[fenceCount]         = pPerRequestInputPort->phFence;
                pDependency->bufferDependency.pIsFenceSignaled[fenceCount] = pPerRequestInputPort->pIsFenceSignaled;
                pDependency->bufferDependency.fenceCount++;

                CAMX_LOG_DRQ("Setting buffer dependency for Node::%s with fence 0x%08x for request= %llu",
                             NodeIdentifierString(),
                             *pPerRequestInputPort->phFence,
                             pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId);

            }
            // delayed buffer Fence is already signaled for pending node. So, buffer should already be available
            else
            {
                DelayedOutputBufferInfo* pDelayedBufferData = pPerRequestInputPort->pDelayedOutputBufferData;

                if (NULL != pDelayedBufferData)
                {
                    pPerRequestInputPort->pImageBuffer = pDelayedBufferData->ppImageBuffer[0];
                    pPerRequestInputPort->phFence      = &pDelayedBufferData->hFence;

                    pPerRequestInputPort->flags.isParentInputBuffer = pDelayedBufferData->isParentInputBuffer;
                    pPerRequestInputPort->flags.isPendingBuffer     = FALSE;

                    CAMX_LOG_DRQ("Buffer 0x%x is already available for Node::%s with fence 0x%08x for request= %llu",
                        pPerRequestInputPort->pImageBuffer,
                        NodeIdentifierString(),
                        *pPerRequestInputPort->phFence,
                        pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Invalid delayed buffer data request = %llu",
                                   NodeIdentifierString(), m_tRequestId);
                }
            }
        }
    }

    if (pDependency->bufferDependency.fenceCount > 0)
    {
        pDependency->dependencyFlags.hasInputBuffersReadyDependency = TRUE;
        pDependency->dependencyFlags.isInternalDependency           = TRUE;
        pDependency->processSequenceId                              = InternalDependencySequenceId::ResolveDeferredInputBuffers;
        pNodeRequestData->numDependencyLists++;

        CAMX_LOG_DRQ("Added buffer dependencies. reqId = %llu Number of fences = %d",
                     pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId,
                     pDependency->bufferDependency.fenceCount);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::UpdateDeviceIndices
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::UpdateDeviceIndices(
    UINT         outputPortIndex,
    const INT32* pDeviceIndices,
    UINT         deviceCount)
{
    CAMX_ASSERT(outputPortIndex < m_outputPortsData.numPorts);
    CAMX_ASSERT(NULL != pDeviceIndices);
    CAMX_ASSERT(0 != deviceCount);

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "Update Dependencies output port index = %d", outputPortIndex);
    for (UINT count = 0; count < deviceCount; count++)
    {
        BOOL matchFound = FALSE;
        UINT curDeviceIndex = m_outputPortsData.pOutputPorts[outputPortIndex].deviceCount;
        const INT32* pOutputPortDeviceIndices = &m_outputPortsData.pOutputPorts[outputPortIndex].deviceIndices[0];

        // Check duplicate entry. If the device index is already present, don't add
        for (UINT deviceIndex = 0; deviceIndex < curDeviceIndex; deviceIndex++)
        {
            if (pOutputPortDeviceIndices[deviceIndex] == pDeviceIndices[count])
            {
                matchFound = TRUE;
                break;
            }
        }

        if (FALSE == matchFound)
        {
            m_outputPortsData.pOutputPorts[outputPortIndex].deviceIndices[curDeviceIndex] = pDeviceIndices[count];
            m_outputPortsData.pOutputPorts[outputPortIndex].deviceCount++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::BypassNodeProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::BypassNodeProcessing()
{
    // If the node is a bypassable node then it's parent need to be informed with the node device indices that bypassable nodes'
    // output is connected to, inorder to do the buffer mapping
    CAMX_ASSERT(TRUE == IsBypassableNode());

    OutputPort* pOutputPort                       = NULL;
    INT32       deviceIndices[CamxMaxDeviceIndex] = { 0 };
    UINT        deviceIndicesCount                = 0;

    for (UINT outputPortIndex = 0; outputPortIndex < m_outputPortsData.numPorts; outputPortIndex++)
    {
        pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        if (0 < pOutputPort->numSourcePortsMapped)
        {
            for (UINT index = 0; index < pOutputPort->deviceCount; index++)
            {
                BOOL alreadyInList = FALSE;

                for (UINT deviceIndex = 0; deviceIndex < deviceIndicesCount; deviceIndex++)
                {
                    if (deviceIndices[deviceIndex] == pOutputPort->deviceIndices[index])
                    {
                        alreadyInList = TRUE;
                    }
                }

                // if no match found then add device index to the list
                if ((FALSE == alreadyInList) && ((deviceIndicesCount + 1) < CamxMaxDeviceIndex))
                {
                    deviceIndices[deviceIndicesCount++] = pOutputPort->deviceIndices[index];
                }
            }
        }

        for (UINT index = 0; index < pOutputPort->numSourcePortsMapped; index++)
        {
            UINT32 portId = pOutputPort->pMappedSourcePortIds[index];

            // optimize input port index search
            for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
            {
                InputPort* pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];
                if ((portId == pInputPort->portId) && (NULL != pInputPort->pParentNode) &&
                    (FALSE == IsSourceBufferInputPort(inputPortIndex)))
                {
                    pInputPort->pParentNode->UpdateDeviceIndices(pInputPort->parentOutputPortIndex,
                        pOutputPort->deviceIndices, pOutputPort->deviceCount);
                    pInputPort->pParentNode->AddInputConnectedforBypass(pInputPort->parentOutputPortIndex,
                        (pOutputPort->numInputPortsConnected + pOutputPort->numInputPortsConnectedinBypass));
                }
            }
        }
    }

    for (UINT inputPortIndex = 0; inputPortIndex < m_inputPortsData.numPorts; inputPortIndex++)
    {
        // Add device indices of the child node for bypassable node if any of the input port is
        // source buffer port
        if (TRUE == IsSourceBufferInputPort(inputPortIndex))
        {
            for (UINT index = 0; index < deviceIndicesCount; index++)
            {
                CAMX_ASSERT_MESSAGE((m_deviceIndexCount + 1) < CamxMaxDeviceIndex, "Device index is out of bound");
                m_deviceIndices[m_deviceIndexCount++] = deviceIndices[index];
            }
            break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DumpState(
    INT     fd,
    UINT32  indent,
    UINT64  requestId)
{
    UINT32              requestIndex = requestId % MaxRequestQueueDepth;
    NodePerRequestInfo* pInfo        = &m_perRequestInfo[requestIndex];
    const ImageFormat*  pImageFormat = NULL;

    // If the requestId doesn't match, nothing to do...it's gone from the Node...
    if (pInfo->requestId == requestId)
    {
        CAMX_LOG_TO_FILE(fd, indent, "---------------------------------------------------------------------------------");
        CAMX_LOG_TO_FILE(fd, indent, "+ Node::%s: Status::\"%s\" m_nodeFlags: 0x%08X",
                         NodeIdentifierString(),
                         GetRequestStatusString(pInfo->requestStatus),
                         m_nodeFlags);
        CAMX_LOG_TO_FILE(fd, indent, "-----------------------------");

        // Only dump if node has outstanding information
        if ((PerRequestNodeStatus::Success != pInfo->requestStatus) || (0 != pInfo->numUnsignaledFences) ||
                                                                       (0 != pInfo->numUnprocessedFences))
        {
            // Input port dump
            CAMX_LOG_TO_FILE(fd, indent + 2, "+ Pending Input ports:");

            // Keep track of what previous fence was to format primary/composite fences correctly
            CSLFence hPrevInputFence = CSLInvalidFence;

            for (UINT i = 0; i < pInfo->activePorts.numInputPorts; i++)
            {
                const PerRequestInputPortInfo*  pInputPort          = &(pInfo->activePorts.pInputPorts[i]);
                const CSLFence                  inputImgBufFence    = (pInputPort->phFence != NULL) ?
                                                                        *(pInputPort->phFence) : CSLInvalidFence;
                CSLFence                        hCurrInputFence     = inputImgBufFence;

                // Dump the port if there is outstanding information for a valid fence
                if ((0 == *pInputPort->pIsFenceSignaled) && (CSLInvalidFence != inputImgBufFence))
                {
                    if (hPrevInputFence != hCurrInputFence)
                    {
                        CAMX_LOG_TO_FILE(fd, indent + 4, "{ FENCE = %d, Signaled = %d }",
                                         inputImgBufFence,
                                         *pInputPort->pIsFenceSignaled);
                    }
                    // Update fence to keep track of primary/composite fences
                    hPrevInputFence = hCurrInputFence;

                    if (TRUE == pInputPort->flags.isPendingBuffer)
                    {
                        CAMX_LOG_TO_FILE(fd, indent + 8, "InputPort Id = %03u;           "
                                         "{ DelayedOutputBuffer: fence = 0x%08X, sequenceId = %u }",
                                         pInputPort->portId,
                                         pInputPort->pDelayedOutputBufferData->hFence,
                                         pInputPort->pDelayedOutputBufferData->sequenceId);
                    }
                    else
                    {
                        CAMX_LOG_TO_FILE(fd, indent + 8, "InputPort Id = %03u;           ", pInputPort->portId);
                    }

                    CAMX_LOG_TO_FILE(fd, indent + 10, "ImageBuffer: [%p]", pInputPort->pImageBuffer);

                    // When parent node is bypassable chinode, inputport imagebuffer will be set as InvalidImageBuffer
                    if (reinterpret_cast<ImageBuffer*>(InvalidImageBuffer) != pInputPort->pImageBuffer)
                    {
                        // Dump information about input port's corresponding image buffer
                        if ((NULL != pInputPort->pImageBuffer) && (0 < pInputPort->pImageBuffer->GetReferenceCount()))
                        {
                            pInputPort->pImageBuffer->DumpState(fd, indent + 10);
                        }
                    }
                }
            }

            // Output port dump
            CAMX_LOG_TO_FILE(fd, indent + 2, "+ Pending Output ports:");
            CAMX_LOG_TO_FILE(fd, indent + 4, "numUnsignaledFences = %u", pInfo->numUnsignaledFences);
            CAMX_LOG_TO_FILE(fd, indent + 4, "numUnprocessedFences = %u", pInfo->numUnprocessedFences);

            // Keep track of what previous fence was to format primary/composite fences correctly
            CSLFence hPrevOutputFence = CSLInvalidFence;

            for (UINT i = 0; i < m_outputPortsData.numPorts; i++)
            {
                OutputPort* pOutputPort     = &m_outputPortsData.pOutputPorts[i];
                UINT        maxImageBuffers = pOutputPort->bufferProperties.maxImageBuffers;

                if (0 != maxImageBuffers)
                {
                    UINT32                  imageFenceIndex     =
                        m_perRequestInfo[requestIndex].requestId % maxImageBuffers;
                    NodeFenceHandlerData*   pFenceHandlerData   = &pOutputPort->pFenceHandlerData[imageFenceIndex];

                    if ((NULL != pOutputPort->ppImageBuffers) && (NULL != pOutputPort->ppImageBuffers[imageFenceIndex]))
                    {
                        ImageBuffer*    pImageBuffer        = pOutputPort->ppImageBuffers[imageFenceIndex];
                        CSLFence        hCurrOutputFence    = pFenceHandlerData->hFence;

                        // Dump if there is outstanding information for the request
                        if ((TRUE == pFenceHandlerData->isDelayedBufferFence) ||
                            (FALSE == pFenceHandlerData->isFenceSignaled) ||
                            (CSLFenceResultFailed == pFenceHandlerData->fenceResult))
                        {
                            if (hPrevOutputFence != hCurrOutputFence)
                            {
                                CAMX_LOG_TO_FILE(fd, indent + 4, "{ FENCE = %d, Signaled = %d } ",
                                                    pFenceHandlerData->hFence, pFenceHandlerData->isFenceSignaled);
                            }
                            // Update fence to keep track of primary/composite fences
                            hPrevOutputFence = hCurrOutputFence;

                            CAMX_LOG_TO_FILE(fd, indent + 8, "OutputPort id: %03u;           { isDelayedBufferFence?:%d; "
                                                "fenceResult:%d; ImgRefCount:%d; maxImageBuffers:%d }",
                                            pOutputPort->portId,
                                            pFenceHandlerData->isDelayedBufferFence,
                                            pFenceHandlerData->fenceResult,
                                            pImageBuffer->GetReferenceCount(),
                                            maxImageBuffers);

                            if (0 < (pImageBuffer->GetReferenceCount()))
                            {
                                pImageFormat = pImageBuffer->GetFormat();
                                pImageBuffer->DumpState(fd, indent + 10);
                                if (NULL != pImageFormat)
                                {
                                    CAMX_LOG_TO_FILE(fd, indent + 12, "ImageBuffer[%p] { ColorSpace = %u, Format = %u,"
                                                        " Width:%u, Height:%u, Alignment:%d, Lossy:%d, version:%d }",
                                                        pImageBuffer,
                                                        pImageFormat->colorSpace,
                                                        pImageFormat->format,
                                                        pImageFormat->width,
                                                        pImageFormat->height,
                                                        pImageFormat->alignment,
                                                        pImageFormat->ubwcVerInfo.lossy,
                                                        pImageFormat->ubwcVerInfo.version);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpFenceErrors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DumpFenceErrors(
    INT     fd,
    UINT32  indent)
{
    UINT    latestErrorIndex    = 0;
    UINT32  count               = MaxFenceErrorBufferDepth;

    if (m_fenceErrors.freeIndex == 0)
    {
        latestErrorIndex = MaxFenceErrorBufferDepth - 1;
    }
    else
    {
        latestErrorIndex = (m_fenceErrors.freeIndex - 1) % MaxFenceErrorBufferDepth;
    }

    while (count)
    {
        count--;
        if (0 != m_fenceErrors.fenceErrorData[latestErrorIndex].requestId)
        {
            if (0 != fd)
            {
                CAMX_LOG_TO_FILE(fd, indent, "Node::%s, RequestId = %llu, Fence: 0x%08X, PortId: %u",
                                    NodeIdentifierString(),
                                    m_fenceErrors.fenceErrorData[latestErrorIndex].requestId,
                                    m_fenceErrors.fenceErrorData[latestErrorIndex].hFence,
                                    m_fenceErrors.fenceErrorData[latestErrorIndex].portId);
            }
            else
            {
                CAMX_LOG_DUMP(CamxLogGroupCore, "+    Fence Error: RequestId = %llu, Fence: 0x%08X, PortId: %u",
                                m_fenceErrors.fenceErrorData[latestErrorIndex].requestId,
                                m_fenceErrors.fenceErrorData[latestErrorIndex].hFence,
                                m_fenceErrors.fenceErrorData[latestErrorIndex].portId);
            }
        }

        if (latestErrorIndex == 0)
        {
            latestErrorIndex = MaxFenceErrorBufferDepth - 1;
        }
        else
        {
            latestErrorIndex--;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpNodeInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DumpNodeInfo(
    INT     fd,
    UINT32  indent)
{
    CAMX_LOG_TO_FILE(fd, indent, "----------------------------");
    CAMX_LOG_TO_FILE(fd, indent, "Node: %s ", NodeIdentifierString());

    for (UINT i = 0; i < m_inputPortsData.numPorts; i++)
    {
        CAMX_LOG_TO_FILE(fd, indent + 2, "InputPort %u, format: %u, portDisabled: %s, bufferDelta: %llu",
                         m_inputPortsData.pInputPorts[i].portId,
                         static_cast<UINT>(m_inputPortsData.pInputPorts[i].imageFormat.format),
                         Utils::BoolToString(m_inputPortsData.pInputPorts[i].portDisabled),
                         m_inputPortsData.pInputPorts[i].bufferDelta);
    }

    for (UINT i = 0; i < m_outputPortsData.numPorts; i++)
    {
        CAMX_LOG_TO_FILE(fd, indent + 2, "OutputPort %u, format: %u  maxNumBuffers: %u, memFlags: 0x%08x, heap: %d,",
                         m_outputPortsData.pOutputPorts[i].portId,
                         static_cast<UINT>(m_outputPortsData.pOutputPorts[i].bufferProperties.imageFormat.format),
                         m_outputPortsData.pOutputPorts[i].bufferProperties.maxImageBuffers,
                         m_outputPortsData.pOutputPorts[i].bufferProperties.memFlags,
                         m_outputPortsData.pOutputPorts[i].bufferProperties.bufferHeap);

        CAMX_LOG_TO_FILE(fd, indent + 4, "flags: 0x%08X, streamEnableMask: 0x%08X, numBatchedFrames: %u",
                         m_outputPortsData.pOutputPorts[i].flags,
                         m_outputPortsData.pOutputPorts[i].enabledInStreamMask,
                         m_outputPortsData.pOutputPorts[i].numBatchedFrames);

        for (UINT j = 0; j < m_outputPortsData.pOutputPorts[i].deviceCount; j++)
        {
            CAMX_LOG_TO_FILE(fd, indent + 4, "deviceIndex[%d]: %d",
                             j, m_outputPortsData.pOutputPorts[i].deviceIndices[j]);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpLinkInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DumpLinkInfo(
    INT     fd,
    UINT32  indent)
{
    for (UINT i = 0; i < m_inputPortsData.numPorts; i++)
    {
        if (TRUE == m_inputPortsData.pInputPorts[i].flags.isSourceBuffer)
        {
            CAMX_LOG_TO_FILE(fd, indent,
                "SourceBuffer_%u -> Node::%s::InputPort_%u -- bufferDelta = %llu, maxImageBuffer = %u",
                m_inputPortsData.pInputPorts[i].sourceTargetStreamId,
                NodeIdentifierString(),
                m_inputPortsData.pInputPorts[i].portId,
                m_inputPortsData.pInputPorts[i].bufferDelta,
                (NULL == m_inputPortsData.pInputPorts[i].pImageBufferManager)?
                                   0 : m_inputPortsData.pInputPorts[i].pImageBufferManager->GetMaxImageBufferCount());
        }
        else
        {
            Node* pParent = m_inputPortsData.pInputPorts[i].pParentNode;
            UINT  port    = m_inputPortsData.pInputPorts[i].parentOutputPortIndex;

            CAMX_LOG_TO_FILE(fd, indent,
                "Node::%s::OutputPort_%u -> Node::%s::InputPort_%u -- bufferDelta = %llu, maxImageBuffer = %u",
                pParent->NodeIdentifierString(),
                pParent->m_outputPortsData.pOutputPorts[port].portId,
                NodeIdentifierString(),
                m_inputPortsData.pInputPorts[i].portId,
                m_inputPortsData.pInputPorts[i].bufferDelta,
                (NULL == m_inputPortsData.pInputPorts[i].pImageBufferManager)?
                                   0 : m_inputPortsData.pInputPorts[i].pImageBufferManager->GetMaxImageBufferCount());
        }
    }
    for (UINT i = 0; i < m_outputPortsData.numPorts; i++)
    {
        // Every node walked, and links generated from inputs.  Only need to capture sink output ports
        if (TRUE == m_outputPortsData.pOutputPorts[i].flags.isSinkBuffer)
        {
            CAMX_LOG_TO_FILE(fd, indent,
                "Node::%s::OutputPort_%u -> SinkBuffer_%u -- BufferQueueDepth = %llu, maxImageBuffer = %u",
                NodeIdentifierString(),
                m_outputPortsData.pOutputPorts[i].portId,
                m_outputPortsData.pOutputPorts[i].sinkTargetStreamId,
                (NULL == m_outputPortsData.pOutputPorts[i].pBufferProperties)?
                                   0 : m_outputPortsData.pOutputPorts[i].pBufferProperties->BufferQueueDepth,
                (NULL == m_outputPortsData.pOutputPorts[i].pImageBufferManager)?
                                   0 : m_outputPortsData.pOutputPorts[i].pImageBufferManager->GetMaxImageBufferCount());
        }
        else if (TRUE == m_outputPortsData.pOutputPorts[i].flags.isSinkNoBuffer)
        {
            CAMX_LOG_TO_FILE(fd, indent,
                "Node::%s::OutputPort_%u -> SinkNoBuffer -- BufferQueueDepth = %llu, maxImageBuffer = %u",
                NodeIdentifierString(),
                m_outputPortsData.pOutputPorts[i].portId,
                (NULL == m_outputPortsData.pOutputPorts[i].pBufferProperties)?
                                   0 : m_outputPortsData.pOutputPorts[i].pBufferProperties->BufferQueueDepth,
                (NULL == m_outputPortsData.pOutputPorts[i].pImageBufferManager)?
                                   0 : m_outputPortsData.pOutputPorts[i].pImageBufferManager->GetMaxImageBufferCount());
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetMultiCameraInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetMultiCameraInfo(
    BOOL*   pIsMultiCameraUsecase,
    UINT32* pNumberOfCamerasRunning,
    UINT32* pCurrentCameraId,
    BOOL*   pIsMasterCamera)
{
    BOOL       isMultiCameraUsecase     = FALSE;
    UINT32     numberOfCamerasRunning   = 1;
    UINT32     currentCameraId          = 0;

    BOOL       isMasterCamera           = TRUE;
    CamxResult result                   = CamxResultSuccess;
    UINT32     numPhysicalCamerasTag    = 0;
    UINT32*    pNumPhysicalCameras      = NULL;
    UINT32     cameraIdTag              = 0;
    UINT32     masterCameraTag          = 0;
    UINT32     lpmModeTag               = 0;

    if ((NULL == pIsMultiCameraUsecase) && (NULL == pNumberOfCamerasRunning) && (NULL == pIsMasterCamera))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Invalid input args", NodeIdentifierString());
        result = CamxResultEFailed;
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.logicalcamerainfo", "NumPhysicalCameras",
            &numPhysicalCamerasTag);
        if (CamxResultSuccess == result)
        {
            MetaBuffer* pSessionMetaBuffer  = m_pPipeline->GetPipelineDescriptor()->pSessionMetadata;
            pNumPhysicalCameras = static_cast<UINT32*>(pSessionMetaBuffer->GetTag(numPhysicalCamerasTag, TRUE));
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                           "Node::%s : numPhysicalCamerasTag tags location not available %d",
                           NodeIdentifierString(), numPhysicalCamerasTag);
        }

    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainfo", "MultiCameraIds", &cameraIdTag);

        result |= VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainfo", "MasterCamera", &masterCameraTag);

        result |= VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainfo", "LowPowerMode", &lpmModeTag);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore,
                           "Node::%s : MultiCamera vendor tags location not available %d %d %d",
                           NodeIdentifierString(), cameraIdTag, masterCameraTag, lpmModeTag);
        }
    }

    if (CamxResultSuccess == result)
    {
        cameraIdTag           |= InputMetadataSectionMask;
        masterCameraTag       |= InputMetadataSectionMask;
        lpmModeTag            |= InputMetadataSectionMask;

        MultiCameraIds*     pMultiCameraId      = NULL;
        UINT32*             pIsMaster           = NULL;
        LowPowerModeInfo*   pLPMInfo            = NULL;
        const UINT          propertyTag[3]      = { cameraIdTag, masterCameraTag, lpmModeTag };
        VOID*               pData[3]            = { 0 };
        UINT64              propertyOffset[3]   = { 0 };
        UINT                length              = CAMX_ARRAY_SIZE(propertyTag);
        LowPowerModeInfo    lpmInfo;

        GetDataList(propertyTag, pData, propertyOffset, length);

        // Disable LPM by default if the LPM vendor tag is not set
        if (NULL == pData[2])
        {
            lpmInfo = { 0 };
            pData[2] = &lpmInfo;
        }

        if ((NULL != pData[0]) && (NULL != pData[1]) && (NULL != pData[2]))
        {
            pMultiCameraId      = static_cast<MultiCameraIds*>(pData[0]);
            pIsMaster           = static_cast<UINT32*>(pData[1]);
            pLPMInfo            = static_cast<LowPowerModeInfo*>(pData[2]);

            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node::%s : currentCameraId=%d, logicalCameraId=%d, "
                "isMasterCamera=%d, isLPMEnabled = %d, isSlaveOperational=%d",
                NodeIdentifierString(), pMultiCameraId->currentCameraId, pMultiCameraId->logicalCameraId,
                *pIsMaster, pLPMInfo->isLPMEnabled, pLPMInfo->isSlaveOperational);

            if (NULL != pNumPhysicalCameras)
            {
                if (*pNumPhysicalCameras >= 2)
                {
                    isMultiCameraUsecase = TRUE;
                    isMasterCamera       = static_cast<BOOL>(*pIsMaster);
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "numPhysicalCamerasTag data is not available");
            }

            currentCameraId = pMultiCameraId->currentCameraId;

            if (TRUE == isMultiCameraUsecase)
            {
                if (FALSE == isMasterCamera)
                {
                    // EPR coming to aux, there will be an EPR on master as well
                    numberOfCamerasRunning = 2;
                }
                else
                {
                    if (FALSE == pLPMInfo->isLPMEnabled)
                    {
                        numberOfCamerasRunning = 2;
                    }
                    else
                    {
                        if (TRUE == pLPMInfo->isSlaveOperational)
                        {
                            numberOfCamerasRunning = 2;
                        }
                    }
                }
            }
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "Node::%s : MultiCamera vendor tags data not available %p %dp %p",
                          NodeIdentifierString(), pData[0], pData[1], pData[2]);
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupCore,
                     "Node::%s : isMultiCameraUsecase = %d, numberOfCamerasRunning=%d, isMasterCamera=%d",
                     NodeIdentifierString(), isMultiCameraUsecase, numberOfCamerasRunning, isMasterCamera);

    if (NULL != pIsMultiCameraUsecase)
    {
        *pIsMultiCameraUsecase = isMultiCameraUsecase;
    }

    if (NULL != pNumberOfCamerasRunning)
    {
        *pNumberOfCamerasRunning = numberOfCamerasRunning;
    }

    if (NULL != pCurrentCameraId)
    {
        *pCurrentCameraId = currentCameraId;
    }

    if (NULL != pIsMasterCamera)
    {
        *pIsMasterCamera = isMasterCamera;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::SetMultiCameraMasterDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::SetMultiCameraMasterDependency(
    NodeProcessRequestData*  pNodeRequestData)
{
    UINT32& rCount = pNodeRequestData->dependencyInfo[0].propertyDependency.count;
    UINT32  numberOfCamerasRunning;
    UINT32  currentCameraId;
    BOOL    isMultiCameraUsecase;
    BOOL    isMasterCamera;

    GetMultiCameraInfo(&isMultiCameraUsecase, &numberOfCamerasRunning, &currentCameraId, &isMasterCamera);

    if (TRUE == isMultiCameraUsecase)
    {
        UINT32 metaTag = 0;
        if (TRUE == IsTagPresentInPublishList(m_vendorTagMultiCamOutput))
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount++] =
                m_vendorTagMultiCamOutput;
            m_bUseMultiCameraMasterMeta = TRUE;

            CAMX_LOG_VERBOSE(CamxLogGroupPProc, "Multi Camera master/switch dependency set");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetDownscaleDimension
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::GetDownscaleDimension(
    UINT32           intermediateWidth,
    UINT32           intermediateHeight,
    ImageDimensions* pDS4Dimension,
    ImageDimensions* pDS16Dimension,
    ImageDimensions* pDS64Dimension)
{
    UINT   scale;

    if (NULL != pDS4Dimension)
    {
        scale = static_cast<UINT>(pow(4, 1));
        if (0 != scale)
        {
            pDS4Dimension->widthPixels = Utils::EvenCeilingUINT32(Utils::AlignGeneric32(intermediateWidth, scale) / scale);
            pDS4Dimension->heightLines = Utils::EvenCeilingUINT32(Utils::AlignGeneric32(intermediateHeight, scale) / scale);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "scale has 0 value, ds4 dimension not updated");
        }
    }

    if (NULL != pDS16Dimension)
    {
        scale = static_cast<UINT>(pow(4, 2));
        if (0 != scale)
        {
            pDS16Dimension->widthPixels = Utils::EvenCeilingUINT32(Utils::AlignGeneric32(intermediateWidth, scale) / scale);
            pDS16Dimension->heightLines = Utils::EvenCeilingUINT32(Utils::AlignGeneric32(intermediateHeight, scale) / scale);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "scale has 0 value, ds16 dimension not updated");
        }
    }

    if (NULL != pDS64Dimension)
    {
        scale = static_cast<UINT>(pow(4, 3));
        if (0 != scale)
        {
            pDS64Dimension->widthPixels = Utils::EvenCeilingUINT32(Utils::AlignGeneric32(intermediateWidth, scale) / scale);
            pDS64Dimension->heightLines = Utils::EvenCeilingUINT32(Utils::AlignGeneric32(intermediateHeight, scale) / scale);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "scale has 0 value, ds64 dimension not updated");
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ActivateImageBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::ActivateImageBuffers()
{
    CamxResult result = CamxResultSuccess;

    for (UINT outputPortIndex = 0; outputPortIndex < m_outputPortsData.numPorts; outputPortIndex++)
    {
        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        if (NULL != pOutputPort->pImageBufferManager)
        {
            result = pOutputPort->pImageBufferManager->Activate();
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s ReleaseBuffers failed.", NodeIdentifierString());
            break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DeactivateImageBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::DeactivateImageBuffers()
{
    CamxResult result = CamxResultSuccess;
    m_pBufferReleaseLock->Lock();

    for (UINT outputPortIndex = 0; outputPortIndex < m_outputPortsData.numPorts; outputPortIndex++)
    {
        OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];

        if (NULL != pOutputPort->pImageBufferManager)
        {
            result = pOutputPort->pImageBufferManager->Deactivate(TRUE);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s ReleaseBuffers failed.", NodeIdentifierString());
            break;
        }
    }

    m_pBufferReleaseLock->Unlock();
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::SetInputBuffersReadyDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT Node::SetInputBuffersReadyDependency(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    UINT                        dependencyUnitIndex)
{
    NodeProcessRequestData* pNodeRequestData                    = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pEnabledPorts                       = pExecuteProcessRequestData->pEnabledPortsInfo;
    DependencyUnit*         pDependencyUnit                     = &pNodeRequestData->dependencyInfo[dependencyUnitIndex];
    UINT                    fenceCount                          = 0;
    CSLFence                hDependentFence[MaxBufferComposite] = {CSLInvalidHandle};

    for (UINT portIndex = 0; portIndex < pEnabledPorts->numInputPorts; portIndex++)
    {
        PerRequestInputPortInfo* pPort = &pEnabledPorts->pInputPorts[portIndex];

        if ((NULL != pPort) && (NULL != pPort->phFence) && (CSLInvalidFence != *pPort->phFence))
        {
            pDependencyUnit->bufferDependency.phFences[fenceCount]         = pPort->phFence;
            pDependencyUnit->bufferDependency.pIsFenceSignaled[fenceCount] = pPort->pIsFenceSignaled;

            // Iterate to check if a particular fence is already added as dependency in case of composite,
            // and skip adding double dependency
            for (UINT i = 0; i <= portIndex; i++)
            {
                if (hDependentFence[i] == *pPort->phFence)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupStats, "Fence handle %d, already added dependency portIndex %d",
                                   *pPort->phFence,
                                   portIndex);
                    break;
                }
                else if (hDependentFence[i] == CSLInvalidHandle)
                {
                    hDependentFence[i] = *pPort->phFence;
                    fenceCount++;

                    CAMX_LOG_VERBOSE(CamxLogGroupStats, "Add dependency for Fence handle %d, portIndex %d",
                                   *pPort->phFence,
                                   portIndex);

                    break;
                }
            }
        }
    }




    if (0 < fenceCount)
    {
        pDependencyUnit->bufferDependency.fenceCount                    = fenceCount;
        pDependencyUnit->dependencyFlags.hasInputBuffersReadyDependency = TRUE;
    }

    return fenceCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::BindInputOutputBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::BindInputOutputBuffers(
    const PerRequestActivePorts*    pPerRequestPorts,
    BOOL                            bBindInputBuffers,
    BOOL                            bBindOutputBuffers)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pPerRequestPorts)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Invalid input arg", NodeIdentifierString());
        result = CamxResultEInvalidArg;
    }

    if ((CamxResultSuccess == result) &&
        (TRUE              == HwEnvironment::GetInstance()->GetStaticSettings()->enableImageBufferLateBinding))
    {
        // Make sure, all the input, output ImageBuffers are backed with buffer now.
        // Input Buffers  :
        //     Possibility 1 : If this node waits for all input fences before calling this function, the parent node
        //                     would have backed up buffer for corresponding ImageBuffer by now. Parent node would signal its
        //                     output fence only after backing up buffer. We could still call Alloc which will just return
        //                     wihtout doing anything. This is the preferred/suggested approach for every node to follow, i.e
        //                     wait for all input fences (+ Properties) before calling this function.
        //     Possibility 2 : But, if the current node called this function before input fences are signalled, there is a
        //                     possibility that this ImageBuffer might have not backed up buffer yet. Lets allocate now.
        //                     In this case, we end up pre-allocating buffer and keep it in idle state as previous node which
        //                     actually outputs this buffer is still waiting for its dependencies.
        // Output Buffers :
        //     Possibility 1 : This node called this function before the nodes that these ports are connected to, called their
        //                     Bind function. In this case, this node triggers all the output ImageBuffers to back up with
        //                     buffer now. This is preferred approach for all nodes to follow, as we end up binding buffers
        //                     for output ImageBuffers just before processing this node.
        //     Possibility 2 : If the nodes that these ports are connected to, have already called on its BindBuffers function
        //                     by now, i.e those nodes calling this even before current node signals output fences,
        //                     this output port ImageBuffer (which is input ImageBuffers for connecting nodes) would have
        //                     triggerred these ImageBuffers to back up buffer by now. We could still call Alloc which will just
        //                     return wihtout doing anything.

        if (TRUE == bBindInputBuffers)
        {
            for (UINT portIndex = 0; portIndex < pPerRequestPorts->numInputPorts; portIndex++)
            {
                PerRequestInputPortInfo*    pInputPort = &pPerRequestPorts->pInputPorts[portIndex];

                if ((FALSE == pInputPort->flags.isPendingBuffer) && (NULL != pInputPort->pImageBuffer))
                {
                    result = pInputPort->pImageBuffer->BindBuffer();

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore,
                                       "Node::%s Input[%d] : Failed in binding buffer to ImageBuffer, result=%d",
                                       NodeIdentifierString(), portIndex, result);
                        break;
                    }
                }
            }
        }

        if ((CamxResultSuccess == result) && (FALSE == IsSinkNoBufferNode()) && (TRUE == bBindOutputBuffers))
        {
            for (UINT portIndex = 0; portIndex < pPerRequestPorts->numOutputPorts; portIndex++)
            {
                PerRequestOutputPortInfo* pOutputPort = &pPerRequestPorts->pOutputPorts[portIndex];

                for (UINT bufferIndex = 0; bufferIndex < pOutputPort->numOutputBuffers; bufferIndex++)
                {
                    ImageBuffer* pImageBuffer = pOutputPort->ppImageBuffer[bufferIndex];

                    if (NULL != pImageBuffer)
                    {
                        result = pImageBuffer->BindBuffer();

                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCore,
                                           "Node::%s Output[%d] Batch[%d] : Failed in binding buffer to ImageBuffer, result=%s",
                                           NodeIdentifierString(), portIndex, bufferIndex, Utils::CamxResultToString(result));
                            break;
                        }
                    }
                }

                if (CamxResultSuccess != result)
                {
                    break;
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::AddInputConnectedforBypass
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::AddInputConnectedforBypass(
    UINT outputPortIndex,
    UINT numInputPortsConnected)
{
    CAMX_ASSERT(outputPortIndex < m_outputPortsData.numPorts);

    CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node::%s numInputPorts %d", NodeIdentifierString(), numInputPortsConnected);

    m_outputPortsData.pOutputPorts[outputPortIndex].numInputPortsConnectedinBypass += numInputPortsConnected;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::IsEarlyPCREnabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::IsEarlyPCREnabled(
    BOOL* pIsEarlyPCREnabled)
{
    // Set earlyPCR based on static settings and disable if batchmode is enabled
    BOOL isEarlyPCREnabledStaticSettings = ((GetStaticSettings()->numPCRsBeforeStreamOn > 0) ? TRUE : FALSE);
    BOOL isBatchModeEnabled              = ((1 < m_pPipeline->GetBatchedHALOutputNum()) ? TRUE : FALSE);
    *pIsEarlyPCREnabled                  = ((FALSE == isBatchModeEnabled) && (TRUE == isEarlyPCREnabledStaticSettings));
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CAMX_UNREFERENCED_PARAM(pPublistTagList);
    CAMX_LOG_VERBOSE(CamxLogGroupMeta, "No tags are published by Node::%s", NodeIdentifierString());
    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetMetadataPublishList(
    const UINT32** ppTagArray,
    UINT32*        pTagCount,
    UINT32*        pPartialTagCount)
{
    CamxResult result = CamxResultSuccess;
    if ((NULL != ppTagArray) && (NULL != pTagCount) && (NULL != pPartialTagCount))
    {
        result = QueryMetadataPublishList(&m_publishTagArray);
        if (CamxResultSuccess == result)
        {
            *ppTagArray       = &m_publishTagArray.tagArray[0];
            *pTagCount        = m_publishTagArray.tagCount;
            *pPartialTagCount = m_publishTagArray.partialTagCount;
        }
        else
        {
            CAMX_LOG_WARN(CamxLogGroupMeta, "ERROR Node::%s has not published the taglist", NodeIdentifierString());
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupMeta, "Node::%s ppTagArray or pTagCount or pPartialTagCount NULL",
                       NodeIdentifierString(), ppTagArray, pTagCount, pPartialTagCount);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::NodeSourceInputPortChiFenceCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::NodeSourceInputPortChiFenceCallback(
    VOID*           pUserData,
    CSLFence        hSyncFence,
    CSLFenceResult  result)
{
    NodeSourceInputPortChiFenceCallbackData* pData = reinterpret_cast<NodeSourceInputPortChiFenceCallbackData*>(pUserData);

    if ((NULL != pData) && (NULL != pData->pNode))
    {
        pData->pNode->HandleNodeSourceInputPortChiFenceCallback(pUserData, hSyncFence, result);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Fence Callback without valid data pData=%p", pData);
    }

    if (NULL != pData)
    {
        CAMX_FREE(pData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::HandleNodeSourceInputPortChiFenceCallback
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::HandleNodeSourceInputPortChiFenceCallback(
    VOID*           pUserData,
    CSLFence        hSyncFence,
    CSLFenceResult  result)
{
    NodeSourceInputPortChiFenceCallbackData* pData = reinterpret_cast<NodeSourceInputPortChiFenceCallbackData*>(pUserData);

    if ((NULL != pData) && (NULL != pData->phFence) && (hSyncFence == *pData->phFence))
    {

        auto    hPipeline = m_pPipeline->GetPipelineDescriptor();
        UINT    nodeId = Type();
        UINT    nodeInstanceId = InstanceID();
        UINT64  requestId = pData->requestId;
        UINT    hFence = (NULL != pData->phFence) ? *pData->phFence : 0;
        BINARY_LOG(LogEvent::ChiFenceCB_Processed, hPipeline, nodeId, nodeInstanceId, requestId, hFence);
        CAMX_LOG_INFO(CamxLogGroupCore,
            "Node::%s[%p][%p] : ChiFenceCallback for Req[%llu] Fence[%p][%d][%d] Signalled[%p][%d]",
            NodeIdentifierString(), this, pData->pNode, pData->requestId,
            pData->phFence, *pData->phFence, hSyncFence, pData->pIsFenceSignaled, *pData->pIsFenceSignaled);

        if (TRUE == CamxAtomicCompareExchangeU(pData->pIsFenceSignaled, 0, 1))
        {
            if (CSLFenceResultSuccess != result)
            {
                if (CSLFenceResultFailed == result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore,
                        "Fence error detected for Node::%s : Type:%d requestId: %llu  fence: %d",
                        NodeIdentifierString(), Type(), pData->requestId, *pData->phFence);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupCore,
                        "Fence error detected during flush for Node::%s : Type:%d requestId: %llu  fence: %d",
                        NodeIdentifierString(), Type(), pData->requestId, *pData->phFence);
                }

                m_pPipeline->NonSinkPortFenceErrorSignaled(pData->phFence, pData->requestId);
            }
            else
            {
                CAMX_LOG_DRQ("Fence %i signal detected for Node::%s Pipeline:%d requestId: %llu",
                    static_cast<INT32>(*pData->phFence),
                    NodeIdentifierString(), GetPipelineId(), pData->requestId);

                m_pPipeline->NonSinkPortFenceSignaled(pData->phFence, pData->requestId);
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node::%s Fence Callback with invalid data pData=%p", NodeIdentifierString(), pData);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetRequestIdOffsetFromLastFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT64 Node::GetRequestIdOffsetFromLastFlush(
    UINT64 requestId
    ) const
{
    FlushInfo flushInfo;
    UINT64    requestIdOffsetFromLastFlush;

    GetFlushInfo(flushInfo);

    UINT64 lastSubmittedRequestId = flushInfo.lastFlushRequestId;
    requestIdOffsetFromLastFlush = ((requestId < lastSubmittedRequestId) ?  requestId
                                                                         : (requestId - lastSubmittedRequestId));

    return requestIdOffsetFromLastFlush;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::DumpDebugNodeInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::DumpDebugNodeInfo(
    UINT64 requestId)
{
    for (UINT i = 0; i < m_outputPortsData.numPorts; i++)
    {
        CAMX_LOG_DUMP(CamxLogGroupCore, "Pipeline %p Out::%s::port%d  format: %d  width: %d  height: %d",
            m_pPipeline,
            NodeIdentifierString(),
            m_outputPortsData.pOutputPorts[i].portId,
            m_outputPortsData.pOutputPorts[i].bufferProperties.imageFormat.format,
            m_outputPortsData.pOutputPorts[i].bufferProperties.imageFormat.width,
            m_outputPortsData.pOutputPorts[i].bufferProperties.imageFormat.height);
    }
    for (UINT i = 0; i < m_inputPortsData.numPorts; i++)
    {
        CAMX_LOG_DUMP(CamxLogGroupCore, "Pipeline %p In::%s::port%d  format: %d  width: %d  height: %d",
            m_pPipeline,
            NodeIdentifierString(),
            m_inputPortsData.pInputPorts[i].portId,
            m_inputPortsData.pInputPorts[i].imageFormat.format,
            m_inputPortsData.pInputPorts[i].imageFormat.width,
            m_inputPortsData.pInputPorts[i].imageFormat.height);
    }
    DumpDebugInternalNodeInfo(requestId, FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::ClearDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::ClearDependencies(
    UINT64 lastCompletedRequestId)
{
    UINT64     parentRequestId;
    UINT64     bufferDelta;
    UINT64     bufferDelay;
    UINT64     requestIdVal;
    Node*      pParentNode;
    InputPort* pInputPort;
    CAMX_LOG_INFO(CamxLogGroupCore, "Clearing dependencies for Node: %s with lastCompletedRequestId %llu and m_tRequestId %llu",
                  NodeIdentifierString(), lastCompletedRequestId, m_tRequestId);

    // Clear any dependencies for future requests that may never come... or come and we have no valid data for them
    for (UINT32 portIndex = 0; portIndex < m_inputPortsData.numPorts; portIndex++)
    {
        pInputPort  = &m_inputPortsData.pInputPorts[portIndex];
        bufferDelta = pInputPort->bufferDelta;

        for (bufferDelay = 0; bufferDelay < bufferDelta; bufferDelay++)
        {
            requestIdVal = lastCompletedRequestId + bufferDelay + 1;

            if (requestIdVal < bufferDelta)
            {
                CAMX_LOG_WARN(CamxLogGroupCore, "Diff between requestIdVal (%llu) and bufferDelta (%llu) is negative; ignoring",
                              requestIdVal, bufferDelta);
                continue;
            }
            pParentNode     = pInputPort->pParentNode;
            parentRequestId = requestIdVal - bufferDelta;

            if (PerRequestNodeStatus::Uninitialized != pParentNode->GetRequestStatus(parentRequestId))
            {
                CAMX_LOG_INFO(CamxLogGroupCore,
                    "Clearing inputPort - Parent ReqId: %llu Buffer Delta: %llu - %s:%u <- %s:%u",
                    parentRequestId,
                    bufferDelta,
                    NodeIdentifierString(),
                    pInputPort->portId,
                    pParentNode->NodeIdentifierString(),
                    pParentNode->GetOutputPortId(pInputPort->parentOutputPortIndex));

                CAMX_ASSERT(NULL != pInputPort->pParentNode);
                pInputPort->pParentNode->NotifyOutputConsumed(pInputPort->parentOutputPortIndex, parentRequestId);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::IsCurrentlyProcessing
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Node::IsCurrentlyProcessing()
{
    for (UINT requestIdIndex = 0; requestIdIndex < MaxRequestQueueDepth; requestIdIndex++)
    {
        if (PerRequestNodeStatus::Running == GetRequestStatus(requestIdIndex) ||
            PerRequestNodeStatus::Setup == GetRequestStatus(requestIdIndex))
        {
            return TRUE;
        }
    }

    return FALSE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::Flush(
    UINT64 requestId)
{
    CAMX_LOG_CONFIG(CamxLogGroupCore, "Flushing Node %s:%d for requestId %llu on pipeline %s:%d",
        m_pNodeName,
        InstanceID(),
        requestId,
        m_pPipeline->GetPipelineName(),
        GetPipelineId());

    UINT64 requestIdIndex                                         = requestId % MaxRequestQueueDepth;
    PerRequestActivePorts* pRequestPorts                          = &m_perRequestInfo[requestIdIndex].activePorts;
    BOOL                   groupFenceSignaled[MaxBufferComposite] = { 0 };

    if (GetLastFlushedRequestId() > requestId)
    {

        CAMX_LOG_INFO(CamxLogGroupCore, "Cancelling open request: %u", requestId);
        CSLFlushInfo cancelPkt;
        CSLHandle hCSL          = m_pPipeline->GetCSLSession();
        cancelPkt.flushType     = CSLFlushRequest;
        cancelPkt.lastCSLSyncId = GetCSLSyncId(requestId);
        cancelPkt.lastRequestId = requestId;
        cancelPkt.numDevices    = CamxMaxDeviceIndex;
        cancelPkt.numSyncLinks  = 1;
        cancelPkt.phSyncLink    = m_pPipeline->GetCSLLink();
        cancelPkt.phDevices     = m_pPipeline->GetCSLDevices();

        CSLCancelRequest(hCSL, cancelPkt);
    }

    SetRequestStatus(requestId, PerRequestNodeStatus::Cancelled);

    // First cancel the request and then signal the fence
    CancelRequest(requestId);

    // Signal the fences with failure before cleaning up the resources so that ProcessFenceCallback will call
    // processRequestIdDone.
    for (UINT portIndex = 0; portIndex < pRequestPorts->numOutputPorts; portIndex++)
    {
        // Fetch the composite group ID
        UINT groupID = m_bufferComposite.portGroupID[pRequestPorts->pOutputPorts[portIndex].portId];

        if ((NULL             != pRequestPorts->pOutputPorts[portIndex].phFence)    &&
            (CSLInvalidHandle != *(pRequestPorts->pOutputPorts[portIndex].phFence)) &&
            (FALSE            == groupFenceSignaled[groupID]))
        {
            CSLFenceSignal(*(pRequestPorts->pOutputPorts[portIndex].phFence), CSLFenceResultCanceled);
        }

        if ((NULL             != pRequestPorts->pOutputPorts[portIndex].phDelayedBufferFence)    &&
            (CSLInvalidHandle != *(pRequestPorts->pOutputPorts[portIndex].phDelayedBufferFence)) &&
            (FALSE            == groupFenceSignaled[groupID]))
        {
            CSLFenceSignal(*(pRequestPorts->pOutputPorts[portIndex].phDelayedBufferFence), CSLFenceResultCanceled);
        }

        // Signal fence only once for ports(Nodes) which have composite mask
        if ((TRUE  == m_bufferComposite.hasCompositeMask) &&
            (FALSE == groupFenceSignaled[groupID]))
        {
            groupFenceSignaled[groupID] = TRUE;
            CAMX_LOG_INFO(CamxLogGroupCore, "%s for requestId %llu PortId:%u hFence=%u groupID:%u",
                          NodeIdentifierString(),
                          requestId,
                          pRequestPorts->pOutputPorts[portIndex].portId,
                          *(pRequestPorts->pOutputPorts[portIndex].phFence),
                          groupID);
        }
    }

    // for SW modes - FDManager, Stats, Chinodewrapper etc.
    if ((TRUE == IsSinkNoBufferNode()) || (0 == m_nodeType))
    {
        ProcessPartialMetadataDone(requestId);
        ProcessMetadataDone(requestId);
        ProcessRequestIdDone(requestId);
    }

    // Free up DRQ resources
    m_pPipeline->NotifyDRQofRequestError(requestId);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetPortCrop
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetPortCrop(
    Node*         pNode,
    UINT          inputPortId,
    PortCropInfo* pCropInfo,
    UINT32*       pIsFindByRecurssion)
{
    CamxResult  result = CamxResultSuccess;

    // Currently this function will only return IFE applied and residual crop information
    if (NULL != pCropInfo)
    {
        // Get IFE crop info
        IFECropInfo* pResidualCropInfo = NULL;
        IFECropInfo* pAppliedCropInfo  = NULL;

        UINT props[] =
        {
            PropertyIDIFEDigitalZoom,
            PropertyIDIFEAppliedCrop
        };

        if (FALSE == pNode->IsRealTime())
        {
            props[0] |= InputMetadataSectionMask;
            props[1] |= InputMetadataSectionMask;
        }

        VOID*  pData[CAMX_ARRAY_SIZE(props)]  = { 0 };
        UINT64 offset[CAMX_ARRAY_SIZE(props)] = { 0 };

        pNode->GetDataList(props, pData, offset, CAMX_ARRAY_SIZE(props));
        pResidualCropInfo = reinterpret_cast<IFECropInfo*>(pData[0]);
        pAppliedCropInfo  = reinterpret_cast<IFECropInfo*>(pData[1]);

        if ((NULL == pResidualCropInfo) || (NULL == pAppliedCropInfo))
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Cannot get crop info residual crop %p applied crop %p",
                           pResidualCropInfo,
                           pAppliedCropInfo);
            result = CamxResultEInvalidPointer;
        }

        if (CamxResultSuccess == result)
        {
            if (IFE == pNode->GetParentNodeType(inputPortId))
            {
                // Find Input port index based on port id
                BOOL found = FALSE;
                UINT portIndex = 0;

                for (portIndex = 0; portIndex < pNode->m_inputPortsData.numPorts; portIndex++)
                {
                    if (inputPortId == pNode->m_inputPortsData.pInputPorts[portIndex].portId)
                    {
                        found = TRUE;
                        break;
                    }
                }

                if (TRUE == found)
                {
                    UINT  parentOutputPortIndex        = pNode->m_inputPortsData.pInputPorts[portIndex].parentOutputPortIndex;
                    Node* pParentNode                  = pNode->m_inputPortsData.pInputPorts[portIndex].pParentNode;
                    const ImageFormat* pImageFormat    = NULL;

                    CAMX_ASSERT(NULL != pParentNode);
                    UINT parentNodeOutputPortId = pParentNode->m_outputPortsData.pOutputPorts[parentOutputPortIndex].portId;

                    CAMX_LOG_VERBOSE(CamxLogGroupCore, "%s Crop from IFE output port %d, IPE input port %d",
                                     pNode->NodeIdentifierString(),
                                     parentNodeOutputPortId,
                                     inputPortId);

                    pImageFormat                          = pNode->GetInputPortImageFormat(portIndex);
                    if (NULL != pImageFormat)
                    {
                        pCropInfo->residualFullDim.width  = pImageFormat->width;
                        pCropInfo->residualFullDim.height = pImageFormat->height;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Null Image Format for Node[%s] Input port %d",
                            pNode->NodeIdentifierString(), inputPortId);
                    }

                    // Get crop info based on IFE(parent) output port Id
                    switch (parentNodeOutputPortId)
                    {
                        case IFEOutputPortFull:
                        case IFEOutputPortRDI0:
                        case IFEOutputPortRDI1:
                        case IFEOutputPortRDI2:
                        case IFEOutputPortRDI3:
                            pCropInfo->residualCrop = pResidualCropInfo->fullPath;
                            pCropInfo->appliedCrop  = pAppliedCropInfo->fullPath;
                            break;
                        case IFEOutputPortDS4:
                            pCropInfo->residualCrop = pResidualCropInfo->DS4Path;
                            pCropInfo->appliedCrop  = pAppliedCropInfo->DS4Path;
                            break;
                        case IFEOutputPortDS16:
                            pCropInfo->residualCrop = pResidualCropInfo->DS16Path;
                            pCropInfo->appliedCrop  = pAppliedCropInfo->DS16Path;
                            break;
                        case IFEOutputPortDisplayFull:
                            pCropInfo->residualCrop = pResidualCropInfo->displayFullPath;
                            pCropInfo->appliedCrop  = pAppliedCropInfo->displayFullPath;
                            break;
                        case IFEOutputPortDisplayDS4:
                            pCropInfo->residualCrop = pResidualCropInfo->displayDS4Path;
                            pCropInfo->appliedCrop  = pAppliedCropInfo->displayDS4Path;
                            break;
                        case IFEOutputPortDisplayDS16:
                            pCropInfo->residualCrop = pResidualCropInfo->displayDS16Path;
                            pCropInfo->appliedCrop  = pAppliedCropInfo->displayDS16Path;
                            break;
                        case IFEOutputPortFD:
                            pCropInfo->residualCrop = pResidualCropInfo->FDPath;
                            pCropInfo->appliedCrop  = pAppliedCropInfo->FDPath;
                            break;
                        default:
                            CAMX_LOG_ERROR(CamxLogGroupCore, "Crop not supported for IFE output port %d",
                                           parentNodeOutputPortId);
                            break;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "cannot find portId %d", inputPortId);
                    result = CamxResultENoSuch;
                }
            }
            else if ((NULL != pIsFindByRecurssion) && (0 != *pIsFindByRecurssion))
            {
                // Find Input port index based on port id
                BOOL found                      = FALSE;
                UINT portIndex                  = 0;
                const ImageFormat* pImageFormat = NULL;
                FLOAT widthRatio = 0;
                FLOAT heightRatio = 0;

                for (portIndex = 0; portIndex < pNode->m_inputPortsData.numPorts; portIndex++)
                {
                    if (inputPortId == pNode->m_inputPortsData.pInputPorts[portIndex].portId)
                    {
                        found = TRUE;
                        break;
                    }
                }

                if (TRUE == found)
                {
                    Node* pParentNode           = pNode->m_inputPortsData.pInputPorts[portIndex].pParentNode;

                    CAMX_ASSERT(NULL != pParentNode);
                    // Below logic works properly only if parent Node has one Input port.
                    // For multiple input ports, need to change the code below.
                    UINT parentNodeInputPortId  = pParentNode->m_inputPortsData.pInputPorts[0].portId;

                    *pIsFindByRecurssion = *pIsFindByRecurssion + 1;
                    GetPortCrop(pParentNode, parentNodeInputPortId, pCropInfo, pIsFindByRecurssion);
                    *pIsFindByRecurssion = *pIsFindByRecurssion - 1;

                    if (1 == *pIsFindByRecurssion)
                    {
                        pImageFormat                   = pNode->GetInputPortImageFormat(portIndex);
                        if (NULL != pImageFormat)
                        {
                            widthRatio                 = static_cast<FLOAT>(pImageFormat->width) /
                                                              pCropInfo->residualFullDim.width;
                            heightRatio                = static_cast<FLOAT>(pImageFormat->height) /
                                                              pCropInfo->residualFullDim.height;
                        }
                        pCropInfo->residualCrop.left   = static_cast<INT32>((pCropInfo->residualCrop.left * widthRatio));
                        pCropInfo->residualCrop.top    = static_cast<INT32>((pCropInfo->residualCrop.top * heightRatio));
                        pCropInfo->residualCrop.width  = static_cast<INT32>((pCropInfo->residualCrop.width * widthRatio));
                        pCropInfo->residualCrop.height = static_cast<INT32>((pCropInfo->residualCrop.height * heightRatio));
                    }
                }
            }
            else
            {
                pCropInfo->residualCrop = pResidualCropInfo->fullPath;
                pCropInfo->appliedCrop  = pAppliedCropInfo->fullPath;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Get crop falied as pCropInfo %p", pCropInfo);
        result = CamxResultEUnsupported;
    }

    return result;
  }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::GetFDPerFrameMetaDataSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetFDPerFrameMetaDataSettings(
    UINT64               offset,
    BOOL*                pIsFDPostingResultsEnabled,
    FDPerFrameSettings*  pPerFrameSettings)
{
    CAMX_ASSERT((NULL != pIsFDPostingResultsEnabled) || (NULL != pPerFrameSettings));

    static const UINT PropertiesFDSettings[] =
    {
        InputStatisticsFaceDetectMode,
        InputControlMode,
        InputControlSceneMode
    };

    static const UINT               Length                 = CAMX_ARRAY_SIZE(PropertiesFDSettings);
    VOID*                           pData[Length]          = { 0 };
    UINT64                          propertyOffset[Length] = { 0 };
    StatisticsFaceDetectModeValues  fdModeReceived         = StatisticsFaceDetectModeValues::StatisticsFaceDetectModeOff;
    ControlModeValues               controlMode            = ControlModeValues::ControlModeOff;
    ControlSceneModeValues          sceneMode              = ControlSceneModeValues::ControlSceneModeDisabled;
    BOOL                            enableFD               = FALSE;
    BOOL                            postResultsToProperty;
    BOOL                            postResultsToMetaData;
    CamxResult                      result                 = CamxResultSuccess;

    for (UINT i = 0; i < Length; i++)
    {
        propertyOffset[i] = offset;
    }

    GetDataList(PropertiesFDSettings, pData, propertyOffset, Length);

    for (UINT i = 0; i < Length; i++)
    {
        if (NULL != pData[i])
        {
            switch (PropertiesFDSettings[i])
            {
                case InputStatisticsFaceDetectMode:
                    fdModeReceived = *reinterpret_cast<StatisticsFaceDetectModeValues*>(pData[i]);
                    break;
                case InputControlMode:
                    controlMode    = *reinterpret_cast<ControlModeValues*>(pData[i]);
                    break;
                case InputControlSceneMode:
                    sceneMode      = *reinterpret_cast<ControlSceneModeValues*>(pData[i]);
                    break;
                default:
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid tagid %08x", PropertiesFDSettings[i]);
                    result = CamxResultENoSuch;
                    break;
            }
        }
    }

    if (TRUE == GetStaticSettings()->disablePostingResults)
    {
        postResultsToProperty = FALSE;
        postResultsToMetaData = FALSE;
    }
    else
    {
        postResultsToProperty = TRUE;
        postResultsToMetaData = TRUE;
    }

    switch (GetStaticSettings()->FDProcessingControl)
    {
        case FDProcessingControlForceEnable:
            enableFD = TRUE;
            break;
        case FDProcessingControlAppRequest:
            if ((StatisticsFaceDetectModeValues::StatisticsFaceDetectModeOff != fdModeReceived) ||
                ((ControlModeValues::ControlModeUseSceneMode                 == controlMode)    &&
                 (ControlSceneModeValues::ControlSceneModeFacePriority       == sceneMode  )))
            {
                enableFD = TRUE;
            }
            break;
        case FDProcessingControlForceDisable:
        default:
            enableFD = FALSE;
    }

    if (FALSE == enableFD)
    {
        postResultsToProperty = FALSE;
    }

    if (NULL != pIsFDPostingResultsEnabled)
    {
        *pIsFDPostingResultsEnabled = postResultsToProperty;
    }

    if (NULL != pPerFrameSettings)
    {
        pPerFrameSettings->fdModeReceived         = fdModeReceived;
        pPerFrameSettings->controlMode            = ControlModeOff;
        pPerFrameSettings->sceneMode              = sceneMode;
        pPerFrameSettings->frameSettings.enableFD = enableFD;
        pPerFrameSettings->postResultsToProperty  = postResultsToProperty;
        pPerFrameSettings->postResultsToMetaData  = postResultsToMetaData;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetCameraIdForMetadataQuery
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 Node::GetCameraIdForMetadataQuery()
{
    UINT32 cameraId = InvalidCameraId;

    if (TRUE == m_bUseMultiCameraMasterMeta)
    {
        INT64         requestId  = static_cast<INT64>(m_tRequestId);
        MetadataPool* pMainPool  = GetIntraPipelinePerFramePool(PoolType::PerFrameResult, GetPipeline()->GetPipelineId());
        MetadataPool* pInputPool = GetIntraPipelinePerFramePool(PoolType::PerFrameInput, GetPipeline()->GetPipelineId());

        BOOL       hasMasterChanged = FALSE;

        InputMetadataOpticalZoom* pInputMultiCamMetadata =
            static_cast<InputMetadataOpticalZoom*>(pInputPool->GetSlot(requestId)->GetMetadataByTag(
                m_vendorTagMultiCamInput));

        OutputMetadataOpticalZoom* pOutputMultiCamMetadata =
            static_cast<OutputMetadataOpticalZoom*>(pMainPool->GetSlot(requestId)->GetMetadataByTag(
                m_vendorTagMultiCamOutput));

        if ((NULL != pInputMultiCamMetadata) && (NULL != pOutputMultiCamMetadata))
        {
            UINT32 previousMasterId = pInputMultiCamMetadata->cameraMetadata[0].masterCameraId;
            UINT32 newMasterId      = pOutputMultiCamMetadata->masterCameraId;

            if (previousMasterId != newMasterId)
            {
                cameraId = pOutputMultiCamMetadata->masterCameraId;

                CAMX_LOG_INFO(CamxLogGroupCore, "Update metadata for node %s old master %u new master %u for %llu",
                    GetPipelineName(), previousMasterId, newMasterId, requestId);
            }
        }
    }

    return cameraId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::CacheVendorTagLocation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::CacheVendorTagLocation()
{
    CamxResult    result  = CamxResultSuccess;
    UINT32        metaTag = 0;

    result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicameraoutputmetadata",
        "OutputMetadataOpticalZoom",
        &m_vendorTagMultiCamOutput);

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.chi.multicamerainputmetadata",
            "InputMetadataOpticalZoom",
            &m_vendorTagMultiCamInput);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.camera.streamMapMeta",
                                                          "StreamMap",
                                                          &m_streamMapVendorTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.camera.streamCropInfo",
                                                          "AllCropInfo",
                                                          &m_allCropInfoVendorTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("com.qti.camera.streamCropInfo",
                                                          "RequestCropInfo",
                                                          &m_requestCropInfoVendorTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.recording",
                                                          "endOfStream",
                                                          &m_vendorTagEndOfStreamFlag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.recording",
                                                          "endOfStreamRequestId",
                                                          &m_vendorTagEndOfStreamRequestId);
    }

    if (CamxResultSuccess == result)
    {
        UINT32     numVendorTags = CAMX_ARRAY_SIZE(PSVendorTags);
        UINT32     tagID;

        for (UINT32 tagIndex = 0; tagIndex < numVendorTags; ++tagIndex)
        {
            result = VendorTagManager::QueryVendorTagLocation(
                PSVendorTags[tagIndex].pSectionName,
                PSVendorTags[tagIndex].pTagName,
                &tagID);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Error: Cannot query vendor tag %s %s",
                    PSVendorTags[tagIndex].pSectionName,
                    PSVendorTags[tagIndex].pTagName);
                break;
            }

            m_pPSMData[tagIndex].tagId   = tagID;
            m_pPSMData[tagIndex].tagSize = PSVendorTags[tagIndex].unitSize;
        }
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.streamDimension",
                                                          "video",
                                                          &m_streamDimensionVideoVendorTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.streamDimension",
                                                          "preview",
                                                          &m_streamDimensionPreviewVendorTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime",
                                                          "Enabled",
                                                          &m_EISEnabledVendorTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.eisrealtime",
                                                          "MinimalTotalMargins",
                                                          &m_EISMinTotalMarginVendorTag);
    }

    if (CamxResultSuccess == result)
    {
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.ifecropinfo",
                                                          "AppliedCrop",
                                                          &m_IFEAppliedCropTag);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::IsPreviewPresent
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Node::IsPreviewPresent()
{
    CamxResult    result                            = CamxResultSuccess;
    BOOL          isPreviewPresent                  = FALSE;
    UINT32        metaTagIsPreviewPresent           = 0;

    result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.streamTypePresent",
        "preview", &metaTagIsPreviewPresent);

    if (CamxResultSuccess == result)
    {
        static UINT GetProps[] =
        {
            metaTagIsPreviewPresent,
        };

        static const UINT GetPropsLength                = CAMX_ARRAY_SIZE(GetProps);
        VOID*             pData[GetPropsLength]         = { 0 };
        UINT64            offsets[GetPropsLength]       = { 0 };

        GetDataList(GetProps, pData, offsets, GetPropsLength);
        if (NULL != pData[0])
        {
            isPreviewPresent = *reinterpret_cast<BOOL*>(pData[0]);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Query previewPresent failed result %d", result);
    }
    return isPreviewPresent;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::SetUsageFlagsInBypass
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SetUsageFlagsInBypass(
    OutputPort*      pOutputPort,
    ImageFormat*     pFormat,
    FormatParamInfo* pFormatParamInfo)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL == pOutputPort) || (NULL == pFormat) || (NULL == pFormatParamInfo))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid input %p %p %p",
                       pOutputPort, pFormat, pFormatParamInfo);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        if (TRUE == IsBypassableNode())
        {
            for (UINT32 i = 0; i < pOutputPort->numSourcePortsMapped; i++)
            {
                UINT        inputPortId     = pOutputPort->pMappedSourcePortIds[i];
                UINT        inputPortIndex  = InputPortIndex(inputPortId);

                if (FALSE == m_inputPortsData.pInputPorts->flags.isSourceBuffer)
                {
                    UINT        parentOutputPortIndex   = m_inputPortsData.pInputPorts[inputPortIndex].parentOutputPortIndex;
                    Node*       pParentNode             = m_inputPortsData.pInputPorts[inputPortIndex].pParentNode;
                    OutputPort* pParentOutputPort       = pParentNode->GetOutputPort(parentOutputPortIndex);

                    CAMX_ASSERT(pFormat->width  == pParentOutputPort->bufferProperties.imageFormat.width);
                    CAMX_ASSERT(pFormat->height == pParentOutputPort->bufferProperties.imageFormat.height);

                    pParentNode->SetUsageFlagsInBypass(pParentOutputPort, pFormat, pFormatParamInfo);
                }
                else
                {
                    FormatParamInfo formatParamInfo = { 0 };
                    SetProducerFormatParameters(pFormat, &formatParamInfo);
                    pFormatParamInfo->LossyUBWCProducerUsage    &= formatParamInfo.LossyUBWCProducerUsage;
                    pFormatParamInfo->UBWCVersionProducerUsage  &= formatParamInfo.UBWCVersionProducerUsage;
                }
            }
        }
        else
        {
            SetProducerFormatParameters(pFormat, pFormatParamInfo);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::SetUBWCusageflags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SetUBWCusageflags(
    OutputPort*      pOutputPort,
    ImageFormat*     pFormat,
    FormatParamInfo* pFormatParamInfo)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL == pOutputPort) || (NULL == pFormat) || (NULL == pFormatParamInfo))
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Invalid input %p %p %p",
                       pOutputPort, pFormat, pFormatParamInfo);
        result = CamxResultEInvalidPointer;
    }

    if ((CamxResultSuccess == result) && (TRUE == ImageFormatUtils::IsUBWC(pFormat->format)))
    {
        // Initial value
        pFormatParamInfo->UBWCVersionConsumerUsage  = 0xFFFF;
        pFormatParamInfo->UBWCVersionProducerUsage  = 0xFFFF;
        pFormatParamInfo->LossyUBWCProducerUsage    = UBWCLossy;
        pFormatParamInfo->LossyUBWCConsumerUsage    = UBWCLossy;
        SetUsageFlagsInBypass(pOutputPort, pFormat, pFormatParamInfo);

        CAMX_LOG_VERBOSE(CamxLogGroupCore, " type %d, format %d, producer lossy %d, version %d",
                         Type(),
                         pFormat->format,
                         pFormatParamInfo->LossyUBWCProducerUsage,
                         pFormatParamInfo->UBWCVersionProducerUsage);
        for (UINT nodeCount = 0; nodeCount < pOutputPort->numOfNodesConnected; nodeCount++)
        {
            if (NULL != pOutputPort->pDstNodes[nodeCount])
            {
                pOutputPort->pDstNodes[nodeCount]->SetConsumerFormatParameters(pFormat, pFormatParamInfo);
                CAMX_LOG_VERBOSE(CamxLogGroupCore, " type %d, format %d, consumer lossy %d, version %d",
                                 Type(),
                                 pFormat->format,
                                 pFormatParamInfo->LossyUBWCConsumerUsage,
                                 pFormatParamInfo->UBWCVersionConsumerUsage);
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetPSDataList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetPSDataList(
    UINT32       inputPortId,
    const UINT*  pDataList,
    VOID** const ppData,
    UINT64*      pOffsets,
    SIZE_T       length)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != pDataList) && (NULL != ppData) && (NULL != pOffsets))
    {
        for (UINT dataIndex = 0; dataIndex < length; ++dataIndex)
        {
            CamxResult resultLocal = GetPSMetadata(inputPortId,
                pDataList[dataIndex], &ppData[dataIndex], pOffsets[dataIndex]);

            if (CamxResultSuccess != resultLocal)
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) GetLinkMetaData failed port %d tag %x",
                    NodeIdentifierString(), inputPortId, pDataList[dataIndex]);
                result = resultLocal;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) NULL input params port %d",
            NodeIdentifierString(), inputPortId);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::WritePSDataList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::WritePSDataList(
    UINT32       outputPortId,
    const UINT*  pDataList,
    const VOID** ppData,
    const UINT*  pDataSize,
    SIZE_T       length)
{
    CamxResult result          = CamxResultSuccess;
    UINT       outputPortIndex = OutputPortIndex(outputPortId);

    if ((outputPortIndex < m_outputPortsData.numPorts) && (NULL != m_pPSMQueue))
    {
        PSMQueue* pQueue        = &m_pPSMQueue[outputPortIndex];
        UINT      requestIndex  = m_tRequestId % pQueue->maxSize;

        for (UINT index = 0; index < length; index++)
        {
            const UINT tagId    = pDataList[index];
            UINT       tagIndex = GetPSMetaIndex(tagId);

            if (MaxPSMetaType > tagIndex)
            {
                if ((pDataSize[index] <= m_pPSMData[tagIndex].tagSize) &&
                    (TRUE == m_pPSMData[tagIndex].canPublish) &&
                    (NULL != pQueue->pEntry[requestIndex].pPSMData[tagIndex]))
                {
                    VOID* pDstData     = pQueue->pEntry[requestIndex].pPSMData[tagIndex];

                    Utils::Memcpy(pDstData, ppData[index], pDataSize[index]);

                    if (StreamCrop == tagIndex)
                    {
                        StreamCropInfo* pCrop = static_cast<StreamCropInfo*>(pDstData);

                        pCrop->frameDimension.width =
                            m_outputPortsData.pOutputPorts[outputPortIndex].bufferProperties.imageFormat.width;
                        pCrop->frameDimension.height =
                            m_outputPortsData.pOutputPorts[outputPortIndex].bufferProperties.imageFormat.height;
                    }
                    CAMX_LOG_VERBOSE(CamxLogGroupCore, "[PSM_INFO] Node(%s) tag: %x, index %d size %d maxSize %d"
                        " output port id %u index %u",
                        NodeIdentifierString(), pDataList[index], tagIndex, pDataSize[index],
                        m_pPSMData[tagIndex].tagSize, outputPortId, outputPortIndex);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Invalid tag: %x, index %u"
                        "size %d maxSize %d published %d",
                        NodeIdentifierString(), pDataList[index], tagIndex, pDataSize[index],
                        m_pPSMData[tagIndex].tagSize,
                        m_pPSMData[tagIndex].canPublish,
                        pQueue->pEntry[requestIndex].pPSMData[tagIndex]);
                    result = CamxResultEInvalidArg;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Invalid tagIndex %u",
                    NodeIdentifierString(), tagIndex);
                result = CamxResultEInvalidArg;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) invalid outputPortIndex %d of %d ports or NULL m_pPSMQueue",
            NodeIdentifierString(), outputPortIndex, m_outputPortsData.numPorts);
        result = CamxResultEOutOfBounds;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::PublishPSData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::PublishPSData(
    UINT32                   tagId,
    ChiPSMetadataBypassInfo* pBypassInfo)
{
    CamxResult    result;
    UINT32        propertyId;
    MetadataSlot* pSlot;

    CAMX_UNREFERENCED_PARAM(pBypassInfo);

    result = m_pPipeline->GetLinkPropertyId(tagId, this, &propertyId);

    if (CamxResultSuccess == result)
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "[PSM_INFO] published request %lld metadata %x link property %x",
            m_tRequestId, tagId, propertyId);
        pSlot = m_pMainPool->GetSlot(m_tRequestId);
        pSlot->PublishMetadata(propertyId, NodeIdentifierString());
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::QuerySupportedPSMetadataList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::QuerySupportedPSMetadataList(
    ChiMetadataIdArray* pMetadataIdArray)
{
    CamxResult result = CamxResultEFailed;
    if (NULL != pMetadataIdArray)
    {
        m_pPipeline->QuerySupportedMetadataList(*pMetadataIdArray);
        result = CamxResultSuccess;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::SetPSMetadataDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::SetPSMetadataDependency(
    UINT                     tagId,
    UINT                     inputPortId,
    UINT64                   offset,
    NodeProcessRequestData*  pNodeRequestData)
{
    CamxResult result        = CamxResultSuccess;
    UINT32&    rCount         = pNodeRequestData->dependencyInfo[0].propertyDependency.count;
    UINT32     propertyId    = 0;

    UniqueNodePortInfo plmUniquePortInfo;

    GetPSMetaPublisherInfo(tagId, InputPortIndex(inputPortId), plmUniquePortInfo);


    if (NULL != plmUniquePortInfo.pNode)
    {
        result = m_pPipeline->GetLinkPropertyId(tagId, plmUniquePortInfo.pNode, &propertyId);

        if (CamxResultSuccess == result)
        {
            pNodeRequestData->dependencyInfo[0].propertyDependency.properties[rCount] = propertyId;
            pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[rCount]    = offset;

            CAMX_LOG_VERBOSE(CamxLogGroupMeta, "[PSM_INFO] dependency set request %lld metadata %x link property %x"
                " offset %llu node %s",
                m_tRequestId, tagId, propertyId, offset,
                plmUniquePortInfo.pNode->Name());

            rCount++;
        }
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "[PSM_INFO] No publisher found for request %lld metadata %x node %p port %u",
            m_tRequestId, tagId, plmUniquePortInfo.pNode, plmUniquePortInfo.portId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetParentSinkPortWithHALBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetParentSinkPortWithHALBuffer(
    UINT                                   outputPortIndex,
    UINT64                                 requestId,
    std::vector<PerRequestInputPortInfo*>& sourceStreamPortList)
{
    CamxResult result = CamxResultSuccess;

    queue<UniqueNodePortInfo> bfsQueue;

    if (outputPortIndex < m_outputPortsData.numPorts)
    {
        bfsQueue.push(UniqueNodePortInfo(this, outputPortIndex));

        while (!bfsQueue.empty())
        {
            vector<PerRequestInputPortInfo*> activeInputPorts;
            UniqueNodePortInfo               currentNodePortInfo = bfsQueue.front();
            bfsQueue.pop();

            if (currentNodePortInfo.portId < currentNodePortInfo.pNode->m_outputPortsData.numPorts)
            {
                OutputPort* pOutputPort = &currentNodePortInfo.pNode->m_outputPortsData.pOutputPorts[
                    currentNodePortInfo.portId];

                currentNodePortInfo.pNode->GetLinkedActiveInputPorts(pOutputPort->portId,
                    requestId, activeInputPorts);

                for (UINT index = 0; index < activeInputPorts.size(); index++)
                {
                    UINT       inputPortIndex = InputPortIndex(activeInputPorts[index]->portId);

                    if (inputPortIndex < currentNodePortInfo.pNode->m_inputPortsData.numPorts)
                    {
                        InputPort* pInputPort     = &currentNodePortInfo.pNode->m_inputPortsData.pInputPorts[inputPortIndex];
                        Node*      pNode          = pInputPort->pParentNode;
                        UINT32     portIndex      = pInputPort->parentOutputPortIndex;

                        if (TRUE == IsSourceBufferInputPort(inputPortIndex))
                        {
                            sourceStreamPortList.push_back(activeInputPorts[index]);
                        }

                        if (NULL != pNode)
                        {
                            bfsQueue.push(UniqueNodePortInfo(pNode, portIndex));
                        }
                    }
                    else
                    {
                        result   = CamxResultEFailed;
                        bfsQueue = queue<UniqueNodePortInfo>();
                        break;
                    }
                }
            }
            else
            {
                result = CamxResultEFailed;
                break;
            }
        }
    }

    if (CamxResultSuccess != result)
    {
        sourceStreamPortList.clear();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetTagArrayIndicesFromInputMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetTagArrayIndicesFromInputMetadata(
    UINT   tagId,
    UINT64 requestId,
    VOID*  pSourceStream,
    UINT&  rIndex,
    UINT&  rRequestCropIndex)
{
    CamxResult     result   = CamxResultENoSuch;
    UINT           tagIndex = GetPSMetaIndex(tagId);
    MetadataSlot*  pSlot    = m_pInputPool->GetSlot(requestId);

    StreamMapMeta* pStreamMapMeta = static_cast<StreamMapMeta*>(
        pSlot->GetMetadataByTag(m_streamMapVendorTag, NodeIdentifierString()));

    if ((NULL != pStreamMapMeta) && (MaxPSMetaType > tagIndex) && (MaxPerBufferMetadata > tagIndex))
    {
        for (UINT streamIndex = 0; streamIndex < pStreamMapMeta->streamMapEntryCount; ++streamIndex)
        {
            StreamMapMetaEntry* pEntry = &pStreamMapMeta->streamMapEntry[streamIndex];

            if (pSourceStream == pEntry->pSinkStreamPrivateInfo)
            {
                rIndex            = pEntry->index[tagIndex];
                rRequestCropIndex = pEntry->requestCropIndex;

                result  = CamxResultSuccess;
                break;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::InitializePSMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::InitializePSMetadata(
    UINT32 tagId)
{
    CamxResult result = CamxResultSuccess;

    // Allocate holders for the PLM queue if not created already
    if ((NULL == m_pPSMQueue) && (0 < m_outputPortsData.numPorts))
    {
        m_pPSMQueue = CAMX_NEW PSMQueue[m_outputPortsData.numPorts];

        if (NULL != m_pPSMQueue)
        {
            for (UINT portIdx = 0; portIdx < m_outputPortsData.numPorts; ++portIdx)
            {
                PSMQueue* pPortQueue = &m_pPSMQueue[portIdx];

                pPortQueue->maxSize  = DefaultRequestQueueDepth;
                pPortQueue->pEntry   = CAMX_NEW PSMQueueEntry[pPortQueue->maxSize];

                if (NULL == pPortQueue->pEntry)
                {
                    CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Cannot allocate memory for queue entry tag %x",
                        NodeIdentifierString(), tagId);
                    result = CamxResultENoMemory;
                    break;
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Cannot allocate memory for queue  tag %x",
                NodeIdentifierString(), tagId);
            result = CamxResultENoMemory;
        }
    }

    vector<vector<UINT>> portIdVector;
    UINT                 tagIndex = GetPSMetaIndex(tagId);

    if ((MaxPSMetaType > tagIndex) && (CamxResultSuccess == result) && (FALSE == m_pPSMData[tagIndex].canPublish))
    {
        result = GetPLMetaPortGroups(tagId, portIdVector);

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "[PSM_INFO] Node(%s) Tag %x group count %u",
                NodeIdentifierString(), tagId, portIdVector.size());

            for (UINT groupIndex = 0; groupIndex < portIdVector.size(); ++groupIndex)
            {
                for (UINT index = 0; (index < m_pPSMQueue[0].maxSize) && (0 < portIdVector[groupIndex].size()); ++index)
                {
                    VOID* pPayload = CAMX_CALLOC(m_pPSMData[tagIndex].tagSize);
                    if (NULL == pPayload)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Cannot allocate payload memory for  %x",
                            NodeIdentifierString(), tagId);
                        result = CamxResultENoMemory;
                        break;
                    }

                    for (UINT groupSubIndex = 0; groupSubIndex < portIdVector[groupIndex].size(); ++groupSubIndex)
                    {
                        UINT portIndex  = OutputPortIndex(portIdVector[groupIndex][groupSubIndex]);

                        CAMX_LOG_VERBOSE(CamxLogGroupCore, "[PSM_INFO] Node(%s) Tag %x group[%d][%d] portId %u portIndex %u",
                            NodeIdentifierString(), tagId, groupIndex, groupSubIndex,
                            portIdVector[groupIndex][groupSubIndex],
                            portIndex);

                        if (portIndex < m_outputPortsData.numPorts)
                        {
                            PSMQueue* pPortQueue = &m_pPSMQueue[portIndex];

                            pPortQueue->pEntry[index].pPSMData[tagIndex] = pPayload;

                            if (0 == groupSubIndex)
                            {
                                pPortQueue->pEntry[index].isOwner[tagIndex] = TRUE;
                            }
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) portIndex %u > max num ports %u for tag %x",
                                NodeIdentifierString(), portIndex, m_outputPortsData.numPorts, tagId);
                            result = CamxResultENoMemory;
                            break;
                        }
                    }
                }

                if (CamxResultSuccess != result)
                {
                    break;
                }
            }
        }
    }

    if (((MaxPSMetaType > tagIndex)) && (CamxResultSuccess == result))
    {
        m_pPSMData[tagIndex].canPublish = TRUE;
    }
    else
    {
        // Remove partial allocations
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Initialize PLM for tag Index %u Id %x failed",
            NodeIdentifierString(), tagIndex, tagId);
        result = CamxResultENoMemory;
        DeInitializePSMetadata(tagId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::DeInitializePSMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::DeInitializePSMetadata(
    UINT32 tagId)
{
    CamxResult  result   = CamxResultSuccess;
    UINT        tagIndex = GetPSMetaIndex(tagId);

    if (MaxPSMetaType > tagIndex)
    {
        if (NULL != m_pPSMQueue)
        {
            for (UINT portIdx  = 0; portIdx < m_outputPortsData.numPorts; ++portIdx)
            {
                PSMQueue* pQueue = &m_pPSMQueue[portIdx];

                if (NULL != pQueue && NULL != pQueue->pEntry)
                {
                    for (UINT index = 0; index < pQueue->maxSize; index++)
                    {
                        if ((NULL != pQueue->pEntry[index].pPSMData[tagIndex]) &&
                            (TRUE == pQueue->pEntry[index].isOwner[tagIndex]))
                        {
                            CAMX_FREE(pQueue->pEntry[index].pPSMData[tagIndex]);
                            pQueue->pEntry[index].pPSMData[tagIndex] = NULL;
                        }
                    }
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Invalid tag identifier: %x",
            NodeIdentifierString(), tagId);
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::ConstructPSMetaMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::ConstructPSMetaMap(
    UINT32 tagId)
{
    CamxResult result   = CamxResultSuccess;
    UINT       tagIndex = GetPSMetaIndex(tagId);

    if (MaxPSMetaType > tagIndex)
    {
        if (FALSE == m_pPSMData[tagIndex].isMapInitialized)
        {
            for (UINT inputPortIdx = 0; inputPortIdx < m_inputPortsData.numPorts; ++inputPortIdx)
            {
                if (FALSE == m_inputPortsData.pInputPorts[inputPortIdx].portDisabled)
                {
                    InputPort* pInputPort    = &m_inputPortsData.pInputPorts[inputPortIdx];

                    if (NULL != pInputPort->pParentNode)
                    {
                        vector<UniqueNodePortInfo>  ancestorList;

                        pInputPort->pParentNode->GetAncestorNodePortInfo(pInputPort->parentOutputPortIndex,
                            tagId, ancestorList);

                        m_pPSMData[tagIndex].hasBypassableAncestor |=
                            pInputPort->pParentNode->m_pPSMData[tagIndex].hasBypassableAncestor;

                        CAMX_LOG_VERBOSE(CamxLogGroupCore, "[PSM_INFO] Node(%s) Tag %x ancestor count %u",
                            NodeIdentifierString(), tagId, ancestorList.size());

                        if (0 < ancestorList.size())
                        {
                            m_pPSMData[tagIndex].nodePortInfoMap.insert({ inputPortIdx, ancestorList } );
                            CAMX_LOG_VERBOSE(CamxLogGroupCore, "[PSM_INFO] Node(%s) inputPortIdx %u Tag %x ancestor count %u "
                                "publisher node %s port index %u",
                                NodeIdentifierString(), inputPortIdx, tagId, ancestorList.size(),
                                ancestorList[0].pNode->Name(),
                                ancestorList[0].portId);
                        }
                        else
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupCore, "[PSM_INFO] Node(%s) Tag %x ancestor count %u",
                                NodeIdentifierString(), tagId, ancestorList.size());
                        }
                    }
                }
            }
            m_pPSMData[tagIndex].isMapInitialized = TRUE;
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Node::AnyPortHasLateBindingDisabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Node::AnyPortHasLateBindingDisabled(
    PerRequestActivePorts*  pPerRequestPorts)
{
    BOOL result = FALSE;

    // If late binding is enabled globally, check if any of the active ports
    // have late binding disabled via link properties or if we have Sink Buffer
    // on this link.
    if (TRUE == GetStaticSettings()->enableImageBufferLateBinding)
    {
        for (UINT port = 0; port < pPerRequestPorts->numOutputPorts; port++)
        {
            OutputPort* pOutPort = GetOutputPort(OutputPortIndex(pPerRequestPorts->pOutputPorts[port].portId));

            if (TRUE == pOutPort->flags.isLateBindingDisabled)
            {
                result = TRUE;
                break;
            }
        }
    }
    else
    {
        // Late binding is disabled globally.
        result = TRUE;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetHal3CropInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetHal3CropInfo(
    VOID** const     ppData,
    UINT64           offset)
{
    CamxResult     result         = CamxResultSuccess;
    CropWindow*    pHalCropWindow = NULL;

    const UINT  PropertiesHal3Crop[] =
    {
        InputScalerCropRegion
    };
    const SIZE_T length                             = CAMX_ARRAY_SIZE(PropertiesHal3Crop);
    VOID*        pData[length]                      = { 0 };
    UINT64       propertyDataHal3CropOffset[length] = { offset };

    GetDataList(PropertiesHal3Crop, pData, propertyDataHal3CropOffset, length);
    pHalCropWindow = (static_cast<CropWindow*>(pData[0]));

    if (NULL == m_pHalStreamCrop)
    {
        m_pHalStreamCrop = CAMX_NEW StreamCropInfo;
    }

    if ((NULL != m_pHalStreamCrop) && (NULL != pHalCropWindow))
    {
        m_pHalStreamCrop->crop.left      = pHalCropWindow->left;
        m_pHalStreamCrop->crop.top       = pHalCropWindow->top;
        m_pHalStreamCrop->crop.width     = pHalCropWindow->width;
        m_pHalStreamCrop->crop.height    = pHalCropWindow->height;

        m_pHalStreamCrop->fov.left   = 0;
        m_pHalStreamCrop->fov.top    = 0;
        m_pHalStreamCrop->fov.width  = m_activePixelArrayWidth;
        m_pHalStreamCrop->fov.height = m_activePixelArrayHeight;

        m_pHalStreamCrop->frameDimension.width   = m_activePixelArrayWidth;
        m_pHalStreamCrop->frameDimension.height  = m_activePixelArrayHeight;

        *ppData = m_pHalStreamCrop;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "%s NULL m_pHalStreamCrop:%p OR pHalCropWindow: %p ReqId:%llu",
                            NodeIdentifierString(), m_pHalStreamCrop, pHalCropWindow, m_tRequestId);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetLinkedInputPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetLinkedInputPorts(
    UINT                outputPortIndex,
    vector<InputPort*>& rInputPort)
{
    CamxResult  result         = CamxResultSuccess;
    UINT        inputPortCount = 0;

    CAMX_UNREFERENCED_PARAM(outputPortIndex);

    for (UINT index = 0; index < m_inputPortsData.numPorts; ++index)
    {
        if ((FALSE == m_inputPortsData.pInputPorts[index].portDisabled) &&
            (FALSE == m_inputPortsData.pInputPorts[index].flags.isLoopback))
        {
            rInputPort.push_back(&m_inputPortsData.pInputPorts[index]);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetAncestorNodePortInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetAncestorNodePortInfo(
    UINT                             outputPortIndex,
    UINT                             tagId,
    std::vector<UniqueNodePortInfo>& ancestorList)
{
    CamxResult result            = CamxResultSuccess;
    UINT       tagIndex          = GetPSMetaIndex(tagId);
    BOOL       hasBypassAncestor = FALSE;

    UniqueNodePortInfo portInfo;

    if (MaxPSMetaType > tagIndex)
    {
        if (TRUE == m_pPSMData[tagIndex].canPublish)
        {
            ancestorList.push_back(UniqueNodePortInfo(this, outputPortIndex));
        }
        else
        {
            OutputPort*        pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];
            vector<InputPort*> inputPorts;

            if (TRUE == IsBypassableNode())
            {
                m_pPSMData[tagIndex].hasBypassableAncestor = TRUE;
            }

            GetLinkedInputPorts(outputPortIndex, inputPorts);

            for (UINT index = 0; index < inputPorts.size(); index++)
            {
                InputPort* pInputPort  = inputPorts[index];
                Node*      pNode       = pInputPort->pParentNode;
                UINT32     portIndex   = pInputPort->parentOutputPortIndex;

                if (NULL != pNode)
                {
                    if (TRUE == pNode->m_pPSMData[tagIndex].canPublish)
                    {
                        ancestorList.push_back(UniqueNodePortInfo(pNode, portIndex));
                    }
                    else
                    {
                        result = pNode->GetAncestorNodePortInfo(portIndex, tagId, ancestorList);

                        if (CamxResultSuccess == result)
                        {
                            m_pPSMData[tagIndex].hasBypassableAncestor |= pNode->m_pPSMData[tagIndex].hasBypassableAncestor;
                        }
                    }
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Invalid tag %x provided for outport index %d",
            NodeIdentifierString(), tagId, outputPortIndex);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetStreamCropInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetStreamCropInfo(
    UINT                inputPortIndex,
    UniqueNodePortInfo& rPublisherInfo,
    UINT                tagId,
    VOID** const        ppData,
    UINT64              offset)
{
    CamxResult      result         = CamxResultSuccess;
    VOID*           pInputPayload  = NULL;
    StreamCropInfo* pCropInfo      = NULL;

    if (NULL != rPublisherInfo.pNode)
    {
        PSMQueue* pQueue       = &rPublisherInfo.pNode->m_pPSMQueue[rPublisherInfo.portId];
        UINT      requestIndex = (m_tRequestId + offset) % pQueue->maxSize;

        *ppData   = pQueue->pEntry[requestIndex].pPSMData[StreamCrop];
        pCropInfo = static_cast<StreamCropInfo*>(*ppData);

        UINT width  = rPublisherInfo.pNode->m_outputPortsData.pOutputPorts[
                        rPublisherInfo.portId].bufferProperties.imageFormat.width;
        UINT height = rPublisherInfo.pNode->m_outputPortsData.pOutputPorts[
                            rPublisherInfo.portId].bufferProperties.imageFormat.height;

        if ((NULL  != pCropInfo) &&
           ((width != pCropInfo->frameDimension.width) || (height != pCropInfo->frameDimension.height)))
        {
            CAMX_LOG_ERROR(CamxLogGroupPProc, "Invalid width:%d or height:%d", width, height);
            result = CamxResultEInvalidArg;
        }
    }
    else
    {
        VOID*              pData        = NULL;
        UniqueNodePortInfo nodePortInfo;
        UINT               sourceInputPortIndex;

        GetSourceNodePortInfo(inputPortIndex, nodePortInfo, sourceInputPortIndex);

        if (NULL != nodePortInfo.pNode)
        {
            result = nodePortInfo.pNode->GetPreviousPipelineMeta(sourceInputPortIndex, tagId, m_tRequestId, &pData);
        }

        // Get previous pipeline meta pData pointer is NULL, then get Hal3 crop
        if (NULL == pData)
        {
            result = GetHal3CropInfo(ppData, offset);
        }

        if ((CamxResultSuccess == result) && (NULL != pData))
        {
            *ppData = pData;
        }

        if (NULL != ppData)
        {
            pCropInfo = static_cast<StreamCropInfo*>(*ppData);
        }

    }

    if (NULL == pCropInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupPProc, "NULL pCropInfo pointer");
        result = CamxResultEInvalidPointer;
    }
    else if (pCropInfo->crop.left + pCropInfo->crop.width  >
            static_cast<INT32>(pCropInfo->frameDimension.width)                                      ||
        pCropInfo->crop.top  + pCropInfo->crop.height >
            static_cast<INT32>(pCropInfo->frameDimension.width)                                      ||
        pCropInfo->fov.left  + pCropInfo->fov.width   > static_cast<INT32>(m_activePixelArrayWidth)  ||
        pCropInfo->fov.top   + pCropInfo->fov.height  > static_cast<INT32>(m_activePixelArrayHeight) ||
        pCropInfo->frameDimension.width                     > m_activePixelArrayWidth                ||
        pCropInfo->frameDimension.height                    > m_activePixelArrayHeight )
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Out of bound crop info = "
            "crop left: %d, top: %d, width: %d, height: %d; "
            "fov  left: %d, top: %d, width: %d, height: %d; "
            "frameDimension width: %d, height: %d",
            NodeIdentifierString(),
            pCropInfo->crop.left,
            pCropInfo->crop.top,
            pCropInfo->crop.width,
            pCropInfo->crop.height,
            pCropInfo->fov.left,
            pCropInfo->fov.top,
            pCropInfo->fov.width,
            pCropInfo->fov.height,
            pCropInfo->frameDimension.width,
            pCropInfo->frameDimension.height);

        result = CamxResultEOutOfBounds;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetActiveAncestorNodeInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetActiveAncestorNodePortInfo(
    UINT                             outputPortIndex,
    UINT                             tagId,
    UINT64                           requestId,
    std::vector<UniqueNodePortInfo>& ancestorList)
{
    CamxResult result            = CamxResultSuccess;
    UINT       tagIndex          = GetPSMetaIndex(tagId);
    BOOL       hasBypassAncestor = FALSE;

    UniqueNodePortInfo portInfo;

    if (MaxPSMetaType > tagIndex)
    {
        if (TRUE == m_pPSMData[tagIndex].canPublish)
        {
            ancestorList.push_back(UniqueNodePortInfo(this, outputPortIndex));
        }
        else
        {
            OutputPort*                      pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];
            vector<PerRequestInputPortInfo*> activeInputPorts;

            GetLinkedActiveInputPorts(pOutputPort->portId,
                requestId,
                activeInputPorts);

            for (UINT index = 0; index < activeInputPorts.size(); index++)
            {
                PerRequestInputPortInfo* pPerReqInputPort  = activeInputPorts[index];
                UINT                     inputPortIndex    = InputPortIndex(pPerReqInputPort->portId);
                InputPort*               pInputPort        = &m_inputPortsData.pInputPorts[inputPortIndex];
                Node*                    pNode             = pInputPort->pParentNode;
                UINT32                   portIndex         = pInputPort->parentOutputPortIndex;

                if (NULL != pNode)
                {
                    if (TRUE == pNode->m_pPSMData[tagIndex].canPublish)
                    {
                        ancestorList.push_back(UniqueNodePortInfo(pNode, portIndex));
                    }
                    else
                    {
                        pNode->GetAncestorNodePortInfo(portIndex, tagId, ancestorList);
                    }
                }
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Invalid tag %x provided for outport index %d",
            NodeIdentifierString(), tagId, outputPortIndex);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetPSMetaPublisherInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::GetPSMetaPublisherInfo(
    UINT                tagId,
    UINT                inputPortIndex,
    UniqueNodePortInfo& rPublishPortInfo)
{
    CamxResult result   = CamxResultSuccess;
    UINT       tagIndex = GetPSMetaIndex(tagId);

    if (MaxPSMetaType > tagIndex)
    {
        if (FALSE == m_pPSMData[tagIndex].isMapInitialized)
        {
            result = ConstructPSMetaMap(tagId);
        }

        if (CamxResultSuccess == result)
        {
            auto    psmEntry      = m_pPSMData[tagIndex].nodePortInfoMap.find(inputPortIndex);
            VOID*   pInputPayload = NULL;

            if (psmEntry != m_pPSMData[tagIndex].nodePortInfoMap.end())
            {
                auto psmIterator = psmEntry->second.begin();

                if (psmIterator != psmEntry->second.end())
                {
                    rPublishPortInfo = *psmIterator;
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetPSMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetPSMetadata(
    UINT         inputPortId,
    UINT         dataId,
    VOID** const ppData,
    UINT64       offset)
{
    CamxResult          result         = CamxResultSuccess;
    UINT                inputPortIndex = InputPortIndex(inputPortId);
    UINT                tagIndex       = (dataId != m_allCropInfoVendorTag) ? GetPSMetaIndex(dataId) : StreamCrop;
    UINT64              requestId      = m_tRequestId;
    UniqueNodePortInfo  plmUniquePortInfo;

    if ((inputPortIndex < m_inputPortsData.numPorts) && (MaxPSMetaType > tagIndex) && (NULL != ppData))
    {
        if (FALSE == m_pPSMData[tagIndex].hasBypassableAncestor)
        {
            GetPSMetaPublisherInfo(dataId, inputPortIndex, plmUniquePortInfo);
        }
        else
        {
            UINT outputPortIndex = GetLinkedOutputPortIndex(inputPortIndex);

            if (InvalidIndex != outputPortIndex)
            {
                vector<UniqueNodePortInfo> ancestorList;
                InputPort* pInputPort = &m_inputPortsData.pInputPorts[inputPortIndex];
                pInputPort->pParentNode->GetActiveAncestorNodePortInfo(outputPortIndex,
                    m_pPSMData[tagIndex].tagId, requestId, ancestorList);

                if (0 < ancestorList.size())
                {
                    plmUniquePortInfo.pNode  = ancestorList[0].pNode;
                    plmUniquePortInfo.portId = ancestorList[0].portId;
                }
            }
        }

        // Handle Crop separately
        if (dataId == m_allCropInfoVendorTag)
        {
            result = GetAllStreamCropInfo(inputPortIndex, offset, ppData);
        }
        else if (dataId == m_pPSMData[StreamCrop].tagId)
        {
            result = GetStreamCropInfo(inputPortIndex,
                                       plmUniquePortInfo,
                                       dataId,
                                       ppData,
                                       offset);
        }
        else if ((NULL != plmUniquePortInfo.pNode) &&
                 (plmUniquePortInfo.pNode->m_outputPortsData.numPorts > plmUniquePortInfo.portId))
        {
            Node* pPublisherNode = plmUniquePortInfo.pNode;
            UINT  publisherPort  = plmUniquePortInfo.portId;
            VOID* pInputPayload  = NULL;

            // Publisher must have the valid queue entry
            PSMQueue* pQueue       = &pPublisherNode->m_pPSMQueue[publisherPort];
            UINT      requestIndex = (requestId + offset) % pQueue->maxSize;

            pInputPayload = pQueue->pEntry[requestIndex].pPSMData[tagIndex];

            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node(%s) fetch metadata %x portId %d PortIndex: %d, "
                "port count %d  tagIndex:%d reqIndex %u publisherPortIndex %u payload %p publisherNode %s",
                NodeIdentifierString(), dataId, inputPortId, inputPortIndex, m_inputPortsData.numPorts, tagIndex,
                requestIndex, publisherPort, pInputPayload, pPublisherNode->NodeIdentifierString());

            *ppData = pInputPayload;
        }
        else
        {
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node(%s) Cannot fetch metadata %x portId %d PortIndex: %d, "
                "port count %d  tagIndex:%d publisher node %p port %u",
                NodeIdentifierString(), dataId, inputPortId, inputPortIndex, m_inputPortsData.numPorts, tagIndex,
                plmUniquePortInfo.pNode, plmUniquePortInfo.portId);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Invalid input portId %d PortIndex: %d, "
            "port count %d  tagIndex:%d",
            NodeIdentifierString(), inputPortId, inputPortIndex, m_inputPortsData.numPorts, tagIndex);
        result = CamxResultEOutOfBounds;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetLinkedActiveInputPorts
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetLinkedActiveInputPorts(
    UINT                              outputPortId,
    UINT64                            requestId,
    vector<PerRequestInputPortInfo*>& rInputPortInfo)
{
    CamxResult          result          = CamxResultSuccess;
    UINT                inputPortCount  = 0;
    NodePerRequestInfo* pPerRequestPort = &m_perRequestInfo[requestId % MaxRequestQueueDepth];

    for (UINT outPortIndex = 0; outPortIndex < pPerRequestPort->activePorts.numOutputPorts; ++outPortIndex)
    {
        const PerRequestOutputPortInfo* pOutputPort = &(pPerRequestPort->activePorts.pOutputPorts[outPortIndex]);

        if ((FALSE == pOutputPort->flags.isLoopback) && (outputPortId == pOutputPort->portId))
        {
            for (UINT inPortIndex = 0; inPortIndex < pPerRequestPort->activePorts.numInputPorts; ++inPortIndex)
            {
                PerRequestInputPortInfo* pInputPort = &(pPerRequestPort->activePorts.pInputPorts[inPortIndex]);

                if ((FALSE == IsBypassableNode()) ||
                    (pOutputPort->mappedInputPortd == pInputPort->portId))
                {
                    rInputPortInfo.push_back(pInputPort);
                }
            }
        }
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetAllAncestorNodes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetAllAncestorNodes(
    UINT                                outputPortIndex,
    UINT                                tagId,
    UINT64                              requestId,
    std::vector<UniqueNodePortInfo>&    ancestorList)
{
    CamxResult result            = CamxResultSuccess;
    UINT       tagIndex          = GetPSMetaIndex(tagId);
    BOOL       hasBypassAncestor = FALSE;

    UniqueNodePortInfo portInfo;

    if (MaxPSMetaType > tagIndex)
    {
        if (TRUE == m_pPSMData[tagIndex].canPublish)
        {
            ancestorList.push_back(UniqueNodePortInfo(this, outputPortIndex));
        }

        OutputPort*                      pOutputPort = &m_outputPortsData.pOutputPorts[outputPortIndex];
        vector<PerRequestInputPortInfo*> activeInputPorts;

        GetLinkedActiveInputPorts(pOutputPort->portId,
                                  requestId,
                                  activeInputPorts);

        Node*     pNodeMax     = NULL;
        Dimension dim          = {0, 0};
        UINT32    portIndexMax = 0;

        for (UINT index = 0; index < activeInputPorts.size(); index++)
        {
            PerRequestInputPortInfo* pPerReqInputPort  = activeInputPorts[index];
            UINT                     inputPortIndex    = InputPortIndex(pPerReqInputPort->portId);
            InputPort*               pInputPort        = &m_inputPortsData.pInputPorts[inputPortIndex];
            Node*                    pNode             = pInputPort->pParentNode;
            UINT32                   portIndex         = pInputPort->parentOutputPortIndex;

            if (NULL != pNode)
            {
                ImageFormat* pImageFormat = &pNode->m_outputPortsData.pOutputPorts[portIndex].bufferProperties.imageFormat;

                if ((pImageFormat->width > dim.width) || (pImageFormat->height > dim.height))
                {
                    pNodeMax     = pNode;
                    portIndexMax = portIndex;
                    dim.width    = pImageFormat->width;
                    dim.height   = pImageFormat->height;
                }
            }
        }

        if (NULL != pNodeMax)
        {
            pNodeMax->GetAllAncestorNodes(portIndexMax, tagId, requestId, ancestorList);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Invalid tag %x provided for outport index %d",
            NodeIdentifierString(), tagId, outputPortIndex);
        result = CamxResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetPLMetaPortGroups
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetPLMetaPortGroups(
    UINT                  tagId,
    vector<vector<UINT>>& plmGroups)
{
    CamxResult  result   = CamxResultSuccess;
    UINT        tagIndex = GetPSMetaIndex(tagId);

    if (MaxPSMetaType > tagIndex)
    {
        if (FALSE == PSVendorTags[tagIndex].isPerPort)
        {
            vector<UINT> portIds;

            for (UINT portIndex = 0; portIndex < m_outputPortsData.numPorts; ++portIndex)
            {
                OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[portIndex];

                if (FALSE == pOutputPort->flags.isLoopback)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupCore, "[PSM_INFO] Node(%s) push tagId %x portId %d", NodeIdentifierString(),
                        tagId,
                        pOutputPort->portId);
                    portIds.push_back(pOutputPort->portId);
                }
            }
            plmGroups.push_back(portIds);
        }
        else
        {
            for (UINT portIndex = 0; portIndex < m_outputPortsData.numPorts; ++portIndex)
            {
                OutputPort* pOutputPort = &m_outputPortsData.pOutputPorts[portIndex];

                if (FALSE == pOutputPort->flags.isLoopback)
                {
                    plmGroups.push_back(vector<UINT>({ pOutputPort->portId }));
                    CAMX_LOG_VERBOSE(CamxLogGroupCore, "[PSM_INFO] Node(%s) push tagId %x portId %d", NodeIdentifierString(),
                        tagId,
                        pOutputPort->portId);
                }
            }
        }
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) Invalid tagId %x", NodeIdentifierString(), tagId);
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::PopulateRequestCropInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::PopulateRequestCropInfo(
    UINT                outputPortIndex,
    UINT                tagId,
    UINT64              requestId,
    RequestCropInfo&    rRequestCropInfo,
    UINT&               rRequestCropIndex)
{
    CamxResult          result                  = CamxResultENoSuch;
    StreamCropLinkInfo* pPrevStreamCropLinkInfo = NULL;
    BOOL                found                   = FALSE;

    vector<UniqueNodePortInfo> ancestorList;

    if (outputPortIndex < m_outputPortsData.numPorts)
    {
        result = GetAllAncestorNodes(outputPortIndex, tagId, requestId, ancestorList);

        for (UINT listIndex = 0; listIndex < ancestorList.size(); listIndex++)
        {
            UniqueNodePortInfo  nodePortInfo = ancestorList[listIndex];

            if (NULL != nodePortInfo.pNode)
            {
                // look up if entry is already present in payload
                for (INT index = 0; index < static_cast<INT>(rRequestCropInfo.streamCropCount); index++)
                {
                    StreamCropLinkInfo streamCropLinkInfo = rRequestCropInfo.streamCropArray[index];

                    if ((streamCropLinkInfo.publisherInfo.instanceId == nodePortInfo.pNode->InstanceID()) &&
                        (streamCropLinkInfo.publisherInfo.portId     == nodePortInfo.portId)              &&
                        (streamCropLinkInfo.publisherInfo.nodeType   == nodePortInfo.pNode->Type()))
                    {
                        if (0 < listIndex)
                        {
                            pPrevStreamCropLinkInfo->publisherInfo.previousEntry = index;
                        }
                        else
                        {
                            rRequestCropIndex = index;
                        }
                        found = TRUE;
                        break;
                    }
                }

                // Break if port entry is already present in request crop info payload
                if (TRUE == found)
                {
                    break;
                }

                VOID* pData = NULL;
                nodePortInfo.pNode->GetPSMetaByRequestId(nodePortInfo.portId, tagId, requestId, &pData);

                StreamCropLinkInfo* pStreamCropLinkInfo =
                    &rRequestCropInfo.streamCropArray[rRequestCropInfo.streamCropCount];

                pStreamCropLinkInfo->streamCropInfo                 = *(static_cast<StreamCropInfo*>(pData));
                pStreamCropLinkInfo->publisherInfo.nodeType         = nodePortInfo.pNode->Type();
                pStreamCropLinkInfo->publisherInfo.portId           = nodePortInfo.portId;
                pStreamCropLinkInfo->publisherInfo.instanceId       = nodePortInfo.pNode->InstanceID();
                pStreamCropLinkInfo->publisherInfo.previousEntry    = -1;

                CAMX_LOG_VERBOSE(CamxLogGroupCore, "Node(%s) Populate request crop info for Node: %s, "
                    "InstanceId: %d, Port: %d, stream crop count: %d",
                    NodeIdentifierString(),
                    nodePortInfo.pNode->NodeIdentifierString(),
                    nodePortInfo.pNode->InstanceID(),
                    nodePortInfo.portId,
                    rRequestCropInfo.streamCropCount);

                if ((NULL != pPrevStreamCropLinkInfo) && (0 < listIndex))
                {
                    pPrevStreamCropLinkInfo->publisherInfo.previousEntry = rRequestCropInfo.streamCropCount;
                }

                pPrevStreamCropLinkInfo = &rRequestCropInfo.streamCropArray[rRequestCropInfo.streamCropCount];
                rRequestCropIndex = rRequestCropInfo.streamCropCount;
                rRequestCropInfo.streamCropCount++;
            }
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetAllStreamCropInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetAllStreamCropInfo(
    UINT                            inputPortIndex,
    UINT64                          offset,
    VOID** const                    ppData)
{
    CamxResult         result               = CamxResultSuccess;
    INT                payloadIndex         = -1;
    VOID*              pData                = NULL;
    UniqueNodePortInfo nodePortInfo         = {0, 0};
    UINT               sourceInputPortIndex = 0;
    UINT               tagDataIndex         = 0;
    UINT               requestCropIndex     = 0;

    vector<UniqueNodePortInfo> ancestorList;
    Node*                      pParentNode     = m_inputPortsData.pInputPorts[inputPortIndex].pParentNode;
    UINT                       outputPortIndex = m_inputPortsData.pInputPorts[inputPortIndex].parentOutputPortIndex;

    pParentNode->GetAllAncestorNodes(outputPortIndex, m_pPSMData[StreamCrop].tagId, m_tRequestId, ancestorList);

    vector<StreamCropInfo>* pStreamCropInfoList = CAMX_NEW vector<StreamCropInfo>();

    // Loop through all ancestor in current stream and append crop info output crop info list
    for (UINT index = 0; index < ancestorList.size(); index++)
    {
        if (NULL != ancestorList[index].pNode)
        {
            VOID* pCropData = NULL;
            result = GetStreamCropInfo(0, // input port is ignored
                                       ancestorList[index],
                                       m_pPSMData[StreamCrop].tagId,
                                       &pCropData,
                                       offset);

            if ((CamxResultSuccess == result) && (NULL != pCropData))
            {
                StreamCropInfo crop = *static_cast<StreamCropInfo*>(pCropData);
                pStreamCropInfoList->push_back(crop);
            }
        }
    }

    // Get node port info corresponding to first node in given stream
    GetSourceNodePortInfo(inputPortIndex, nodePortInfo, sourceInputPortIndex);

    if (NULL != nodePortInfo.pNode)
    {
        MetadataSlot*  pSlot  = nodePortInfo.pNode->m_pInputPool->GetSlot(m_tRequestId);

        if (TRUE == nodePortInfo.pNode->IsSourceBufferInputPort(sourceInputPortIndex))
        {
            InputPort* pInputPort       = &nodePortInfo.pNode->m_inputPortsData.pInputPorts[sourceInputPortIndex];
            UINT       inputPortId       = pInputPort->portId;

            ChiStreamWrapper* pChiStreamWrapper
                = nodePortInfo.pNode->m_pPipeline->GetInputStreamWrapper(nodePortInfo.pNode->Type(),
                                                                         nodePortInfo.pNode->InstanceID(),
                                                                         inputPortId);
            if (NULL != pChiStreamWrapper)
            {
                result = nodePortInfo.pNode->GetTagArrayIndicesFromInputMetadata(
                    nodePortInfo.pNode->m_pPSMData[StreamCrop].tagId,
                    m_tRequestId,
                    pChiStreamWrapper,
                    tagDataIndex,
                    requestCropIndex);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s): NULL pChiStreamWrapper", NodeIdentifierString());
            }
        }

    }

    MetadataSlot* pSlot  = m_pInputPool->GetSlot(m_tRequestId);
    pData                = pSlot->GetMetadataByTag(m_requestCropInfoVendorTag, NodeIdentifierString());

    if (NULL != pData)
    {
        RequestCropInfo* pRequestCropInfo = reinterpret_cast<RequestCropInfo*>(pData);

        payloadIndex = requestCropIndex;

        while ((payloadIndex >= 0) && (payloadIndex < static_cast<INT>(pRequestCropInfo->streamCropCount)))
        {
            pStreamCropInfoList->push_back(pRequestCropInfo->streamCropArray[payloadIndex].streamCropInfo);

            // Get index of previous entry
            payloadIndex = pRequestCropInfo->streamCropArray[payloadIndex].publisherInfo.previousEntry;
        }

    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s) NULL pData or NULL pNode", NodeIdentifierString());
        result = CamxResultEInvalidPointer;
    }
    *ppData = pStreamCropInfoList;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetSourceNodePortInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::GetSourceNodePortInfo(
    UINT                            inputPortIndex,
    UniqueNodePortInfo&             rNodePortInfo,
    UINT&                           rSourceInputPortIndex)
{
    InputPort* pInputPort   = &m_inputPortsData.pInputPorts[inputPortIndex];
    Node*      pParentNode  = pInputPort->pParentNode;

    if (NULL != pParentNode)
    {
        for (UINT idx = 0; idx < pParentNode->m_inputPortsData.numPorts; idx++)
        {
            pParentNode->GetSourceNodePortInfo(idx, rNodePortInfo, rSourceInputPortIndex);

            if (NULL != rNodePortInfo.pNode)
            {
                break;
            }
        }
    }
    else
    {
        if (TRUE == IsSourceBufferInputPort(inputPortIndex))
        {
            rNodePortInfo.pNode   = this;
            rSourceInputPortIndex = inputPortIndex;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetPreviousPipelineMeta
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetPreviousPipelineMeta(
    UINT   inputPortIndex,
    UINT   tagId,
    UINT64 requestId,
    VOID** ppData)
{
    CamxResult             result            = CamxResultSuccess;
    UINT                   index             = 0;
    UINT                   requestCropIndex  = 0;
    MetadataSlot*          pSlot             = m_pInputPool->GetSlot(requestId);
    InputPort*             pInputPort        = &m_inputPortsData.pInputPorts[inputPortIndex];
    UINT                   inputPortId       = pInputPort->portId;
    UINT                   requestIdIndex    = requestId % MaxRequestQueueDepth;
    PerRequestActivePorts* pRequestPorts     = &m_perRequestInfo[requestIdIndex].activePorts;
    ChiStreamWrapper*      pSourceStream     = NULL;

    if (TRUE == IsSourceBufferInputPort(inputPortIndex))
    {
        for (UINT activePortIndex = 0; activePortIndex < pRequestPorts->numInputPorts; activePortIndex++)
        {
            if (pRequestPorts->pInputPorts[activePortIndex].portId == inputPortId)
            {
                pSourceStream = pRequestPorts->pInputPorts[activePortIndex].pParentSinkStream;
                break;
            }
        }

        if (NULL != pSourceStream)
        {
            result = GetTagArrayIndicesFromInputMetadata(tagId, requestId, pSourceStream, index, requestCropIndex);

            if (CamxResultSuccess == result)
            {
                UINT tagIndex = GetPSMetaIndex(tagId);

                if (CAMX_ARRAY_SIZE(PSVendorTags) > tagIndex)
                {
                    *ppData = static_cast<BYTE*>(pSlot->GetMetadataByTag(tagId, NodeIdentifierString())) +
                        (index*PSVendorTags[tagIndex].unitSize);
                }
                else
                {
                    CAMX_LOG_INFO(CamxLogGroupCore, "Node(%s): Invalid tagIndex: %d", NodeIdentifierString(), tagIndex);
                    result = CamxResultEOutOfBounds;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupCore, "Node(%s): Failed GetTagArrayIndicesFromInputMetadata",
                    NodeIdentifierString());
            }
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupCore, "Node(%s): NULL chpChiStreamWrapper", NodeIdentifierString());
            result = CamxResultEInvalidPointer;
        }

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::DeferPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* Node::DeferPipelineCreate(
    VOID * pThreadData)
{
    CamxResult result = CamxResultSuccess;
    Node * pNodeThreadData = reinterpret_cast<Node*>(pThreadData);

    result = pNodeThreadData->PostPipelineCreate();
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupCore, "Node[%s]: Failed to PostPipelineCreate ",
            pNodeThreadData->NodeIdentifierString());
    }

    return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::WaitForDeferPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Node::WaitForDeferPipelineCreate()
{
    if (InvalidJobHandle != m_hThreadPipelineCreate)
    {
        OsUtils::ThreadWait(m_hThreadPipelineCreate);
        m_hThreadPipelineCreate = InvalidJobHandle;
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Node::GetImageStabilizationMargins()
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult Node::GetImageStabilizationMargins(
    StabilizationMargin&    rStabilizationMargin,
    MarginRequest&          rStabilizationMarginRatio)
{
    CamxResult              result              = CamxResultSuccess;
    IFEOutputResolution*    pIFEResolution      = NULL;
    ChiBufferDimension*     pVideoDimension     = NULL;
    ChiBufferDimension*     pPreviewDimension   = NULL;
    BOOL                    EISEnabled          = FALSE;
    MarginRequest*          pMinTotalMargin     = NULL;
    IFECropInfo*            pIFEAppliedCrop     = NULL;

    StabilizationMargin     stabilizationMargin = { 0 };

    const UINT marginTags[] =
    {
        PropertyIDUsecaseIFEOutputResolution,
        m_streamDimensionVideoVendorTag   | UsecaseMetadataSectionMask,
        m_streamDimensionPreviewVendorTag | UsecaseMetadataSectionMask,
        m_EISEnabledVendorTag             | UsecaseMetadataSectionMask,
        m_EISMinTotalMarginVendorTag      | UsecaseMetadataSectionMask,
        m_IFEAppliedCropTag
    };

    const SIZE_T length         = CAMX_ARRAY_SIZE(marginTags);
    VOID*        pData[length]  = { 0 };
    UINT64       offset[length] = { 0, 0, 0, 0, 0, 0 };

    if (CamxResultSuccess == result)
    {
        result = GetDataList(marginTags, pData, offset, length);
        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Failed to get tag values (%d)", result);
        }
    }

    if (CamxResultSuccess == result)
    {
        if (NULL != pData[0])
        {
            pIFEResolution = reinterpret_cast<IFEOutputResolution*>(pData[0]);
            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                             "IFE FullPath(Enabled: %d Dimension(%dx%d)) DisplayFullPath(Enabled: %d Dimension(%dx%d))",
                             pIFEResolution->fullPortEnable,
                             pIFEResolution->fullPortDimension.width,
                             pIFEResolution->fullPortDimension.height,
                             pIFEResolution->displayPortEnable,
                             pIFEResolution->displayPortDimension.width,
                             pIFEResolution->displayPortDimension.height);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "IFE output resolution is null");
            result = CamxResultEInvalidPointer;
        }

        if (NULL != pData[1])
        {
            pVideoDimension = reinterpret_cast<ChiBufferDimension*>(pData[1]);
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "videoResolution(%ux%u)", pVideoDimension->width, pVideoDimension->height);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Video resolution is null");
            result = CamxResultEInvalidPointer;
        }

        if (NULL != pData[2])
        {
            pPreviewDimension = reinterpret_cast<ChiBufferDimension*>(pData[2]);
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "previewResolution(%ux%u)", pPreviewDimension->width, pPreviewDimension->height);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "Preview resolution is null");
            result = CamxResultEInvalidPointer;
        }

        if (NULL != pData[3])
        {
            EISEnabled = *reinterpret_cast<BOOL*>(pData[3]);
            CAMX_LOG_VERBOSE(CamxLogGroupCore, "EISEnabled: %d", EISEnabled);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "EIS enable setting is null");
            result = CamxResultEInvalidPointer;
        }

        if (NULL != pData[4])
        {
            pMinTotalMargin = reinterpret_cast<MarginRequest*>(pData[4]);
            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                             "%s EIS Minimum Total Margin: (%f x %f)",
                             GetPipelineName(),
                             pMinTotalMargin->widthMargin, pMinTotalMargin->heightMargin);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "EIS minimum total margin is null");
            result = CamxResultEInvalidPointer;
        }

        if (NULL != pData[5])
        {
            pIFEAppliedCrop = reinterpret_cast<IFECropInfo*>(pData[5]);
            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                             "%s IFE full path applied crop: (%d x %d)",
                             GetPipelineName(),
                             pIFEAppliedCrop->fullPath.width, pIFEAppliedCrop->fullPath.height);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupCore, "IFE applied crop is null");
            result = CamxResultEInvalidPointer;
        }
    }

    if ((CamxResultSuccess == result)            && (TRUE == EISEnabled)      &&
        (NULL              != pPreviewDimension) && (NULL != pVideoDimension) &&
        (NULL              != pIFEResolution))
    {
        IFEOutputDimensions* pIFEPreviewDimension;

        // Calculate EIS margin
        if (FALSE == pIFEResolution->displayPortEnable)
        {
            pIFEPreviewDimension = &(pIFEResolution->fullPortDimension);

            if ((pPreviewDimension->width     >  pVideoDimension->width)   &&
                (pIFEPreviewDimension->width  >= pPreviewDimension->width) &&
                (pIFEPreviewDimension->height >= pPreviewDimension->height))
            {
                stabilizationMargin.widthPixels = pIFEPreviewDimension->width  - pPreviewDimension->width;
                stabilizationMargin.heightLines = pIFEPreviewDimension->height - pPreviewDimension->height;
            }
            else if ((pIFEPreviewDimension->width  >= pVideoDimension->width) &&
                     (pIFEPreviewDimension->height >= pVideoDimension->height))
            {
                stabilizationMargin.widthPixels = pIFEPreviewDimension->width  - pVideoDimension->width;
                stabilizationMargin.heightLines = pIFEPreviewDimension->height - pVideoDimension->height;
            }
        }
        else
        {
            // display port is enabled for preview stream
            pIFEPreviewDimension = &(pIFEResolution->displayPortDimension);

            if ((pIFEPreviewDimension->width  >= pPreviewDimension->width) &&
                (pIFEPreviewDimension->height >= pPreviewDimension->height))
            {
                stabilizationMargin.widthPixels = pIFEPreviewDimension->width  - pPreviewDimension->width;
                stabilizationMargin.heightLines = pIFEPreviewDimension->height - pPreviewDimension->height;
            }
        }

        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                         "%s: CameraId(%d) EIS margin(%ux%u)",
                         GetPipelineName(),
                         GetPipeline()->GetCameraId(),
                         stabilizationMargin.widthPixels,
                         stabilizationMargin.heightLines);

        StabilizationMargin minMargin = { 0 };
        if ((NULL != pMinTotalMargin) && (pMinTotalMargin->widthMargin > 0.0f) && (pMinTotalMargin->heightMargin > 0.0f))
        {
            if (FALSE == pIFEResolution->displayPortEnable)
            {
                // Minimum margin ratio is w.r.t the larger one of video and preview streams
                if (pPreviewDimension->width > pVideoDimension->width)
                {
                    minMargin.widthPixels = static_cast<UINT32>(pPreviewDimension->width  * pMinTotalMargin->widthMargin);
                    minMargin.heightLines = static_cast<UINT32>(pPreviewDimension->height * pMinTotalMargin->heightMargin);
                }
                else
                {
                    minMargin.widthPixels = static_cast<UINT32>(pVideoDimension->width  * pMinTotalMargin->widthMargin);
                    minMargin.heightLines = static_cast<UINT32>(pVideoDimension->height * pMinTotalMargin->heightMargin);
                }
            }
            else
            {
                minMargin.widthPixels = static_cast<UINT32>(pPreviewDimension->width  * pMinTotalMargin->widthMargin);
                minMargin.heightLines = static_cast<UINT32>(pPreviewDimension->height * pMinTotalMargin->heightMargin);
            }
        }

        if ((minMargin.widthPixels > stabilizationMargin.widthPixels) ||
            (minMargin.heightLines > stabilizationMargin.heightLines))
        {
            stabilizationMargin = minMargin;

            CAMX_LOG_VERBOSE(CamxLogGroupCore,
                             "%s: CameraId(%d) After applying minimum total margin, EIS margin(%ux%u)",
                             GetPipelineName(),
                             GetPipeline()->GetCameraId(),
                             stabilizationMargin.widthPixels,
                             stabilizationMargin.heightLines);
        }

        FLOAT widthConversionRatio;
        FLOAT heightConversionRatio;

        if (FALSE == pIFEResolution->displayPortEnable)
        {
            rStabilizationMarginRatio.widthMargin  = static_cast<FLOAT>(stabilizationMargin.widthPixels) /
                                                     pIFEResolution->fullPortDimension.width;
            rStabilizationMarginRatio.heightMargin = static_cast<FLOAT>(stabilizationMargin.heightLines) /
                                                     pIFEResolution->fullPortDimension.height;

            widthConversionRatio  = static_cast<FLOAT>(pIFEAppliedCrop->fullPath.width)  /
                                    pIFEResolution->fullPortDimension.width;
            heightConversionRatio = static_cast<FLOAT>(pIFEAppliedCrop->fullPath.height) /
                                    pIFEResolution->fullPortDimension.height;
        }
        else
        {
            rStabilizationMarginRatio.widthMargin  = static_cast<FLOAT>(stabilizationMargin.widthPixels) /
                                                     pIFEResolution->displayPortDimension.width;
            rStabilizationMarginRatio.heightMargin = static_cast<FLOAT>(stabilizationMargin.heightLines) /
                                                     pIFEResolution->displayPortDimension.height;

            widthConversionRatio  = static_cast<FLOAT>(pIFEAppliedCrop->fullPath.width)  /
                                    pIFEResolution->displayPortDimension.width;
            heightConversionRatio = static_cast<FLOAT>(pIFEAppliedCrop->fullPath.height) /
                                    pIFEResolution->displayPortDimension.height;
        }

        // Convert margin from IFE output dimension to CAMIF dimension
        rStabilizationMargin.widthPixels = static_cast<UINT32>(stabilizationMargin.widthPixels * widthConversionRatio);
        rStabilizationMargin.heightLines = static_cast<UINT32>(stabilizationMargin.heightLines * heightConversionRatio);

        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                         "CAMIF EIS margins(%ux%u), IFE applied crop(%ux%u), Conversion ratio(%fx%f), Margin ratio(%fx%f)",
                         rStabilizationMargin.widthPixels,
                         rStabilizationMargin.heightLines,
                         pIFEAppliedCrop->fullPath.width,
                         pIFEAppliedCrop->fullPath.height,
                         widthConversionRatio,
                         heightConversionRatio,
                         rStabilizationMarginRatio.widthMargin,
                         rStabilizationMarginRatio.heightMargin);
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupCore,
                         "EISEnabled(%d) pIFEResolution(%pK) pVideoDimension(%pK) pPreviewDimension(%pK)",
                         EISEnabled,
                         pIFEResolution,
                         pVideoDimension,
                         pPreviewDimension);
    }

    return result;
}

CAMX_NAMESPACE_END
