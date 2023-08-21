////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chxpipeline.cpp
/// @brief CHX pipeline class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <assert.h>

#include "chxincs.h"
#include "chxpipeline.h"
#include "chxsession.h"
#include "cdkutils.h"
#include "chxsensorselectmode.h"
#include "chxutils.h"

// NOWHINE ENTIRE FILE - Temporarily bypassing for existing CHI files

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constants
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo Better way would be to make it programmable
static const UINT IPENode = 6;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Pipeline* Pipeline::Create(
    UINT32       cameraId,
    PipelineType type,
    const CHAR*  pName)
{
    Pipeline* pPipeline = CHX_NEW Pipeline;

    if (NULL != pPipeline)
    {
        pPipeline->Initialize(cameraId, type);

        pPipeline->m_pPipelineName = pName;
    }

    return pPipeline;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::Destroy()
{
    if (NULL != m_hPipelineHandle)
    {
        ExtensionModule::GetInstance()->DestroyPipelineDescriptor(m_hPipelineHandle);

        m_hPipelineHandle = NULL;
    }
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::SetOutputBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SetOutputBuffers(
    UINT                     numOutputs,
    CHIPORTBUFFERDESCRIPTOR* pOutputs)
{
    m_numOutputBuffers = numOutputs;

    ChxUtils::Memcpy(&m_pipelineOutputBuffer[0], pOutputs, (sizeof(CHIPORTBUFFERDESCRIPTOR) * numOutputs));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::SetInputBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SetInputBuffers(
    UINT                     numInputs,
    CHIPORTBUFFERDESCRIPTOR* pInputs)
{
    m_numInputBuffers = numInputs;
    m_numInputOptions = 0;

    if (numInputs > 0)
    {
        ChxUtils::Memcpy(&m_pipelineInputBuffer[0], pInputs, (sizeof(CHIPORTBUFFERDESCRIPTOR) * numInputs));
        for (UINT i = 0; i < m_numInputBuffers; i++)
        {
            m_numInputOptions += m_pipelineInputBuffer[i].numNodePorts;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::SetPipelineNameF
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SetPipelineNameF(const CHAR * const pPipelineName,
                                UINT               instanceId)
{
    const UINT maxNameSize = 256;
    CHAR*      pFinalName  = static_cast<CHAR*>(ChxUtils::Calloc(maxNameSize));
    CdkUtils::SNPrintF(pFinalName, maxNameSize, "%s%u", pPipelineName, instanceId);
    SetPipelineName(pFinalName);
    m_isNameAllocated = TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::SetAndroidMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Pipeline::SetAndroidMetadata(
    camera3_stream_configuration * pStreamConfig)
{
    CDKResult result = CDKResultSuccess;

#if defined(CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
    CHX_LOG_INFO("Trying to set the Android Metadata...");
    if (NULL != pStreamConfig)
    {
        if (NULL != pStreamConfig->session_parameters)
        {
            m_pPipelineDescriptorMetadata->SetAndroidMetadata(pStreamConfig->session_parameters);
        }
        else
        {
            CHX_LOG_WARN("pStreamConfig->session_parameters is NULL");
        }
    }
    else
    {
        CHX_LOG_ERROR("pStreamConfig is NULL!")
    }
#endif
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::SetVendorTag
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Pipeline::SetVendorTag(
    UINT32       tagID,
    const VOID*  pData,
    UINT32       count)
{
    CDKResult result = m_pPipelineDescriptorMetadata->SetTag(tagID, pData, count);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::HasSensorNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL Pipeline::HasSensorNode(
    const ChiPipelineCreateDescriptor* pCreateDesc)
{
    BOOL hasSensorNode = FALSE;

    for (UINT i = 0; i < pCreateDesc->numNodes; i++)
    {
        if (pCreateDesc->pNodes[i].nodeId == SensorNodeId)
        {
            hasSensorNode = TRUE;
        }
    }

    return hasSensorNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::GetNumberOfIFEsUsed
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32 Pipeline::GetNumberOfIFEsUsed()
{
    INT32 numOfIFEsUsed = 0;
    if(IsRealTime())
    {
        INT32* pNumIFEsUsed = static_cast<INT32*>(m_pPipelineDescriptorMetadata->GetTag(
            "org.quic.camera.ISPConfigData", "numIFEsUsed"));
        if (NULL != pNumIFEsUsed)
        {
            numOfIFEsUsed = *pNumIFEsUsed;
        }
        else
        {
            CHX_LOG_WARN("Pipeline[%s] numIFEsUsed tag not found! numOfIFEsUsed will not be updated",
                m_pPipelineName);
        }
    }
    return numOfIFEsUsed;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FindHighestWidthInputIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static UINT FindHighestWidthInputIndex(CHIPIPELINEINPUTOPTIONS *pInputOptions, UINT numInputBuffers)
{
    UINT index = 0;
    UINT maxWidth = 0;
    CHIPIPELINEINPUTOPTIONS *pInput = pInputOptions;

    CHX_LOG("FindHighestWidthInputIndex:: numInputBuffers %d", numInputBuffers);
    if (numInputBuffers > 1)
    {
        for (UINT i = 0; i < numInputBuffers; i++)
        {
            CHX_LOG("FindHighestWidthInputIndex:: i %d maxWidth %d inputWidth %d",
                i, maxWidth, pInput[i].bufferOptions.optimalDimension.width);
            if (maxWidth < pInput[i].bufferOptions.optimalDimension.width)
            {
                index    = i;
                maxWidth = pInput[i].bufferOptions.optimalDimension.width;
            }
        }
    }
    CHX_LOG("FindHighestWidthInputIndex:: max Input Option Index %d", index);
    return index;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::QueryMetadataTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Pipeline::QueryMetadataTags(
   CHIHANDLE hSession)
{
    CDKResult result = CDKResultSuccess;

    result = ExtensionModule::GetInstance()->QueryPipelineMetadataInfo(hSession,
                                                                       m_pipelineInfo.hPipelineDescriptor,
                                                                       &m_pPipelineMetadataInfo);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::CreateDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Pipeline::CreateDescriptor()
{
    CDKResult          result                    = CDKResultSuccess;
    PipelineCreateData pipelineCreateData        = { 0 };

    m_pipelineDescriptor.isRealTime              = HasSensorNode(&m_pipelineDescriptor);

    // m_cameraId from usecase side must be correct, even for pipelines without sensor Node
    m_pipelineDescriptor.cameraId                = m_cameraId;

    pipelineCreateData.pPipelineName             = m_pPipelineName;
    pipelineCreateData.numOutputs                = m_numOutputBuffers;
    pipelineCreateData.pOutputDescriptors        = &m_pipelineOutputBuffer[0];
    pipelineCreateData.numInputs                 = m_numInputBuffers;
    pipelineCreateData.pInputOptions             = &m_pipelineInputOptions[0];
    pipelineCreateData.pPipelineCreateDescriptor = &m_pipelineDescriptor;

    pipelineCreateData.pPipelineCreateDescriptor->numBatchedFrames          =
        ExtensionModule::GetInstance()->GetNumBatchedFrames();
    pipelineCreateData.pPipelineCreateDescriptor->HALOutputBufferCombined   =
        ExtensionModule::GetInstance()->GetHALOutputBufferCombined();
    pipelineCreateData.pPipelineCreateDescriptor->maxFPSValue               =
        ExtensionModule::GetInstance()->GetUsecaseMaxFPS();

    const CHAR* pClientName = "Chi::Pipeline::CreateDescriptor";
    SetTuningUsecase();

    m_pPipelineDescriptorMetadata->AddReference(pClientName);
    m_pipelineDescriptor.hPipelineMetadata = m_pPipelineDescriptorMetadata->GetHandle();


    CHX_LOG_CONFIG("Pipeline[%s] pipeline pointer %p numInputs=%d, numOutputs=%d stream w x h: %d x %d",
            m_pPipelineName, this, pipelineCreateData.numInputs, pipelineCreateData.numOutputs,
            pipelineCreateData.pOutputDescriptors->pStream->width,
            pipelineCreateData.pOutputDescriptors->pStream->height);

    // Update stats skip pattern in node property with value from override

    for (UINT node = 0; node < m_pipelineDescriptor.numNodes; node++)
    {
        ChiNode* pChiNode = &m_pipelineDescriptor.pNodes[node];

        for (UINT i = 0; i < pChiNode->numProperties; i++)
        {
            if (pChiNode->pNodeProperties[i].id == NodePropertyStatsSkipPattern)
            {
                m_statsSkipPattern = ExtensionModule::GetInstance()->GetStatsSkipPattern();
                pChiNode->pNodeProperties[i].pValue = &m_statsSkipPattern;
            }
            if (pChiNode->pNodeProperties[i].id == NodePropertyEnableFOVC)
            {
                m_enableFOVC = ExtensionModule::GetInstance()->EnableFOVCUseCase();
                pChiNode->pNodeProperties[i].pValue = &m_enableFOVC;
            }
        }

    }

    m_hPipelineHandle = ExtensionModule::GetInstance()->CreatePipelineDescriptor(&pipelineCreateData);

    m_pPipelineDescriptorMetadata->ReleaseReference(pClientName);

    if (NULL == m_hPipelineHandle)
    {
        result = CDKResultEFailed;
        CHX_LOG_ERROR("Fail due to NULL pipeline handle");
    }
    else
    {
        if (FALSE == ExtensionModule::GetInstance()->IsTorchWidgetUsecase())
        {
            // sensor mode selection not required for torch widget usecase.
            DesiredSensorMode desiredSensorMode = { 0 };
            desiredSensorMode.frameRate = ExtensionModule::GetInstance()->GetUsecaseMaxFPS();
            if (ExtensionModule::GetInstance()->GetVideoHDRMode())
            {
                desiredSensorMode.sensorModeCaps.u.ZZHDR = 1;
            }
            else if (SelectInSensorHDR3ExpUsecase::InSensorHDR3ExpPreview ==
                     ExtensionModule::GetInstance()->SelectInSensorHDR3ExpUsecase())
            {
                desiredSensorMode.sensorModeCaps.u.IHDR = 1;
            }

            UINT index = FindHighestWidthInputIndex(m_pipelineInputOptions, m_numInputOptions);
            // @todo Select the highest width/height from all the input buffer requirements
            desiredSensorMode.optimalWidth  = m_pipelineInputOptions[index].bufferOptions.optimalDimension.width;
            desiredSensorMode.optimalHeight = m_pipelineInputOptions[index].bufferOptions.optimalDimension.height;
            desiredSensorMode.maxWidth      = m_pipelineInputOptions[index].bufferOptions.maxDimension.width;
            desiredSensorMode.maxHeight     = m_pipelineInputOptions[index].bufferOptions.maxDimension.height;
            desiredSensorMode.minWidth      = m_pipelineInputOptions[index].bufferOptions.minDimension.width;
            desiredSensorMode.minHeight     = m_pipelineInputOptions[index].bufferOptions.minDimension.height;
            desiredSensorMode.forceMode     = ExtensionModule::GetInstance()->GetForceSensorMode();

            if (NULL != m_pSensorModePickhint)
            {
                CHX_LOG("input option:%dx%d, upscale:%d, override optimal size:%dx%d, sensor mode caps:%x",
                    desiredSensorMode.optimalWidth, desiredSensorMode.optimalHeight,
                    m_pSensorModePickhint->postSensorUpscale,
                    m_pSensorModePickhint->sensorOutputSize.width,
                    m_pSensorModePickhint->sensorOutputSize.height,
                    m_pSensorModePickhint->sensorModeCaps.value);

                if ((TRUE == m_pSensorModePickhint->postSensorUpscale) &&
                    (m_pSensorModePickhint->sensorOutputSize.width  < desiredSensorMode.optimalWidth) &&
                    (m_pSensorModePickhint->sensorOutputSize.height < desiredSensorMode.optimalHeight))
                {
                    desiredSensorMode.optimalWidth  = m_pSensorModePickhint->sensorOutputSize.width;
                    desiredSensorMode.optimalHeight = m_pSensorModePickhint->sensorOutputSize.height;
                    desiredSensorMode.maxWidth      = desiredSensorMode.optimalWidth;
                    desiredSensorMode.maxHeight     = desiredSensorMode.optimalHeight;
                    desiredSensorMode.minWidth      = desiredSensorMode.optimalWidth;
                    desiredSensorMode.minHeight     = desiredSensorMode.optimalHeight;
                }

                if (0 != m_pSensorModePickhint->sensorModeCaps.value)
                {
                    desiredSensorMode.sensorModeCaps.value = m_pSensorModePickhint->sensorModeCaps.value;
                }
            }
            if (StreamConfigModeFastShutter == ExtensionModule::GetInstance()->GetOpMode(m_cameraId))
            {
                desiredSensorMode.sensorModeCaps.u.FS = 1;
            }

            m_pSelectedSensorMode                   = ChxSensorModeSelect::FindBestSensorMode(m_cameraId, &desiredSensorMode);
            m_pSelectedSensorMode->batchedFrames    = ExtensionModule::GetInstance()->GetNumBatchedFrames();
            m_pSelectedSensorMode->HALOutputBufferCombined = ExtensionModule::GetInstance()->GetHALOutputBufferCombined();
        }

        if (TRUE == m_pipelineDescriptor.isRealTime)
        {
            m_pipelineInfo.pipelineInputInfo.isInputSensor              = TRUE;
            m_pipelineInfo.pipelineInputInfo.sensorInfo.cameraId        = m_cameraId;
            m_pipelineInfo.pipelineInputInfo.sensorInfo.pSensorModeInfo = m_pSelectedSensorMode;
            CHX_LOG_CONFIG("Pipeline[%s] Pipeline pointer %p Selected sensor Mode W=%d, H=%d",
                            m_pPipelineName,
                            this,
                            m_pipelineInfo.pipelineInputInfo.sensorInfo.pSensorModeInfo->frameDimension.width,
                            m_pipelineInfo.pipelineInputInfo.sensorInfo.pSensorModeInfo->frameDimension.height);
        }
        else
        {
            m_pipelineInfo.pipelineInputInfo.isInputSensor                           = FALSE;
            m_pipelineInfo.pipelineInputInfo.inputBufferInfo.numInputBuffers         = m_numInputBuffers;
            m_pipelineInfo.pipelineInputInfo.inputBufferInfo.pInputBufferDescriptors = GetInputBufferDescriptors();
        }

        m_pipelineInfo.hPipelineDescriptor                = reinterpret_cast<CHIPIPELINEDESCRIPTOR>(m_hPipelineHandle);
        m_pipelineInfo.pipelineOutputInfo.hPipelineHandle = NULL;
        m_pipelineInfo.pipelineResourcePolicy             = m_resourcePolicy;
        m_pipelineInfo.isDeferFinalizeNeeded              = m_isDeferFinalizeNeeded;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult Pipeline::Initialize(
    UINT32       cameraId,
    PipelineType type)
{
    CDKResult result = CDKResultSuccess;

    m_cameraId              = cameraId;
    m_type                  = type;
    m_pipelineActivated     = FALSE;
    m_isDeferFinalizeNeeded = FALSE;
    m_pSensorModePickhint   = NULL;
    m_isNameAllocated       = FALSE;

    m_pPipelineDescriptorMetadata = ChiMetadata::Create();
    if (NULL == m_pPipelineDescriptorMetadata)
    {
        result = CDKResultENoMemory;
        CHX_LOG_ERROR("Failed to allocate memory for Pipeline Metadata");
    }

    if (m_type == PipelineType::OfflinePreview)
    {
        m_numInputBuffers  = 1; // Sensor - so no input buffer
        m_numOutputBuffers = 1; // Preview
        SetupRealtimePreviewPipelineDescriptor();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::~Pipeline
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Pipeline::~Pipeline()
{
    m_pPipelineDescriptorMetadata->Destroy();
    FreeName();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Pipeline::SetupRealtimePreviewPipelineDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Pipeline::SetupRealtimePreviewPipelineDescriptor()
{
    m_pipelineDescriptor.size       = sizeof(CHIPIPELINECREATEDESCRIPTOR);
    m_pipelineDescriptor.numNodes   = 1;
    m_pipelineDescriptor.pNodes     = &m_nodes[0];
    m_pipelineDescriptor.numLinks   = 1;
    m_pipelineDescriptor.pLinks     = &m_links[0];
    m_pipelineDescriptor.isRealTime = FALSE;

    // Nodes
    UINT32 nodeIndex = 0;
#if 0
    // ---------------------------------------------------------------------------
    // ---------------------------------- BPS ------------------------------------
    // ---------------------------------------------------------------------------
    m_nodes[nodeIndex].nodeId                      = 65539;
    m_nodes[nodeIndex].nodeInstanceId              = 0;
    m_nodes[nodeIndex].nodeAllPorts.numInputPorts  = 1;
    m_nodes[nodeIndex].nodeAllPorts.pInputPorts    = &m_inputPorts[BPSNode];
    m_nodes[nodeIndex].nodeAllPorts.numOutputPorts = 1;
    m_nodes[nodeIndex].nodeAllPorts.pOutputPorts   = &m_outputPorts[BPSNode];

    // BPS output port
    m_outputPorts[BPSNode].portId                  = 1;
    m_outputPorts[BPSNode].isSinkPort              = FALSE;
    m_outputPorts[BPSNode].isOutputStreamBuffer    = FALSE;
    // BPS input port
    m_inputPorts[BPSNode].portId                   = 0;
    m_inputPorts[BPSNode].isInputStreamBuffer      = TRUE;

    // ---------------------------------------------------------------------------
    // ---------------------------------- IPE ------------------------------------
    // ---------------------------------------------------------------------------
    nodeIndex++;
#endif

    m_nodes[nodeIndex].nodeId                      = 65538;
    m_nodes[nodeIndex].nodeInstanceId              = 0;
    m_nodes[nodeIndex].nodeAllPorts.numInputPorts  = 1;
    m_nodes[nodeIndex].nodeAllPorts.pInputPorts    = &m_inputPorts[IPENode];
    m_nodes[nodeIndex].nodeAllPorts.numOutputPorts = 1;
    m_nodes[nodeIndex].nodeAllPorts.pOutputPorts   = &m_outputPorts[IPENode];

    // IPE output port
    m_outputPorts[IPENode].portId                  = 8;
    m_outputPorts[IPENode].isSinkPort              = TRUE;
    m_outputPorts[IPENode].isOutputStreamBuffer    = TRUE;
    // IPE input port
    m_inputPorts[IPENode].portId                   = 0;
    m_inputPorts[IPENode].isInputStreamBuffer      = TRUE;

#if 0
    // ---------------------------------------------------------------------------
    // ---------------------------------- JPEG -----------------------------------
    // ---------------------------------------------------------------------------
    nodeIndex++;

    m_nodes[nodeIndex].nodeId                        = 65537;
    m_nodes[nodeIndex].nodeInstanceId                = 0;
    m_nodes[nodeIndex].nodeAllPorts.numInputPorts    = 1;
    m_nodes[nodeIndex].nodeAllPorts.pInputPorts      = &m_inputPorts[JPEGNode];
    m_nodes[nodeIndex].nodeAllPorts.numOutputPorts   = 1;
    m_nodes[nodeIndex].nodeAllPorts.pOutputPorts     = &m_outputPorts[JPEGNode];

    // JPEG output port
    m_outputPorts[JPEGNode].portId                   = 1;
    m_outputPorts[JPEGNode].isSinkPort               = FALSE;
    m_outputPorts[JPEGNode].isOutputStreamBuffer     = FALSE;
    // JPEG input port
    m_inputPorts[JPEGNode].portId                    = 0;
    m_inputPorts[JPEGNode].isInputStreamBuffer       = FALSE;

    // ---------------------------------------------------------------------------
    // ---------------------------------- JPEG AGRREGATOR ------------------------
    // ---------------------------------------------------------------------------
    nodeIndex++;

    m_nodes[nodeIndex].nodeId                        = 6;
    m_nodes[nodeIndex].nodeInstanceId                = 0;
    m_nodes[nodeIndex].nodeAllPorts.numInputPorts    = 1;
    m_nodes[nodeIndex].nodeAllPorts.pInputPorts      = &m_inputPorts[JPEGAgrregatorNode];
    m_nodes[nodeIndex].nodeAllPorts.numOutputPorts   = 1;
    m_nodes[nodeIndex].nodeAllPorts.pOutputPorts     = &m_outputPorts[JPEGAgrregatorNode];

    // JPEG output port
    m_outputPorts[JPEGAgrregatorNode].portId                = 1;
    m_outputPorts[JPEGAgrregatorNode].isSinkPort            = TRUE;
    m_outputPorts[JPEGAgrregatorNode].isOutputStreamBuffer  = TRUE;
    // JPEG input port
    m_inputPorts[JPEGAgrregatorNode].portId                 = 0;
    m_inputPorts[JPEGAgrregatorNode].isInputStreamBuffer    = FALSE;
#endif
    // ---------------------------------------------------------------------------
    // --------------------------------- Links -----------------------------------
    // ---------------------------------------------------------------------------

#if 0
    // BPS --> IPE
    m_links[0].srcNode.nodeId                     = 65539;
    m_links[0].srcNode.nodeInstanceId             = 0;
    m_links[0].srcNode.nodePortId                 = 1;
    m_links[0].numDestNodes                       = 1;
    m_links[0].pDestNodes                         = &m_linkNodeDescriptors[0];

    m_linkNodeDescriptors[0].nodeId               = 65538;
    m_linkNodeDescriptors[0].nodeInstanceId       = 0;
    m_linkNodeDescriptors[0].nodePortId           = 0;

    m_links[0].bufferProperties.bufferFlags       = BufferMemFlagHw;
    m_links[0].bufferProperties.bufferFormat      = ChiFormatUBWCTP10;
    m_links[0].bufferProperties.bufferHeap        = BufferHeapIon;
    m_links[0].bufferProperties.bufferQueueDepth  = 8;

    // IPE --> JPEG
    m_links[1].srcNode.nodeId                     = 65538;
    m_links[1].srcNode.nodeInstanceId             = 0;
    m_links[1].srcNode.nodePortId                 = 8;
    m_links[1].numDestNodes                       = 1;
    m_links[1].pDestNodes                         = &m_linkNodeDescriptors[1];

    m_linkNodeDescriptors[1].nodeId               = 65537;
    m_linkNodeDescriptors[1].nodeInstanceId       = 0;
    m_linkNodeDescriptors[1].nodePortId           = 0;

    m_links[1].bufferProperties.bufferFlags       = (BufferMemFlagHw | BufferMemFlagLockable);
    m_links[1].bufferProperties.bufferFormat      = ChiFormatYUV420NV12;
    m_links[1].bufferProperties.bufferHeap        = BufferHeapIon;
    m_links[1].bufferProperties.bufferQueueDepth  = 8;

    // JPEG --> JPEG Agrregator
    m_links[2].srcNode.nodeId                     = 65537;
    m_links[2].srcNode.nodeInstanceId             = 0;
    m_links[2].srcNode.nodePortId                 = 1;
    m_links[2].numDestNodes                       = 1;
    m_links[2].pDestNodes                         = &m_linkNodeDescriptors[2];

    m_linkNodeDescriptors[2].nodeId               = 6;
    m_linkNodeDescriptors[2].nodeInstanceId       = 0;
    m_linkNodeDescriptors[2].nodePortId           = 0;

    m_links[2].bufferProperties.bufferFlags       = (BufferMemFlagHw | BufferMemFlagLockable);
    m_links[2].bufferProperties.bufferFormat      = ChiFormatYUV420NV12;
    m_links[2].bufferProperties.bufferHeap        = BufferHeapIon;
    m_links[2].bufferProperties.bufferQueueDepth  = 8;

    // JPEG Aggregator --> Sink Buffer
    m_links[3].srcNode.nodeId                     = 6;
    m_links[3].srcNode.nodeInstanceId             = 0;
    m_links[3].srcNode.nodePortId                 = 1;
    m_links[3].numDestNodes                       = 1;
    m_links[3].pDestNodes                         = &m_linkNodeDescriptors[3];

    m_linkNodeDescriptors[3].nodeId               = 2;
    m_linkNodeDescriptors[3].nodeInstanceId       = 0;
    m_linkNodeDescriptors[3].nodePortId           = 0;
#endif

    m_links[0].srcNode.nodeId                     = 65538;
    m_links[0].srcNode.nodeInstanceId             = 0;
    m_links[0].srcNode.nodePortId                 = 8;
    m_links[0].numDestNodes                       = 1;
    m_links[0].pDestNodes                         = &m_linkNodeDescriptors[0];

    m_linkNodeDescriptors[0].nodeId               = 2;
    m_linkNodeDescriptors[0].nodeInstanceId       = 0;
    m_linkNodeDescriptors[0].nodePortId           = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::GetPipelineInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const CHIPIPELINEINFO Pipeline::GetPipelineInfo() const
{
    return m_pipelineInfo;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pipeline::SetTuningUsecase
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VOID Pipeline::SetTuningUsecase()
{
    ChiTuningModeParameter chiTuningModeParameter          = {0};
    UINT                      tuningUsecaseMask            = 0;
    chiTuningModeParameter.noOfSelectionParameter          = MaxTuningMode;
    chiTuningModeParameter.TuningMode[2].mode              = ChiModeType::Usecase;
    chiTuningModeParameter.TuningMode[2].subMode.usecase   = ChiModeUsecaseSubModeType::Preview;

    chiTuningModeParameter.TuningMode[0].mode              = ChiModeType::Default;
    chiTuningModeParameter.TuningMode[0].subMode.value     = 0;
    chiTuningModeParameter.TuningMode[1].mode              = ChiModeType::Sensor;
    chiTuningModeParameter.TuningMode[2].mode              = ChiModeType::Usecase;
    chiTuningModeParameter.TuningMode[3].mode              = ChiModeType::Feature1;
    chiTuningModeParameter.TuningMode[4].mode              = ChiModeType::Feature2;
    chiTuningModeParameter.TuningMode[5].mode              = ChiModeType::Scene;
    chiTuningModeParameter.TuningMode[6].mode              = ChiModeType::Effect;

    //generating Dummy request for getting usecase tuning mode
    camera3_capture_request_t dummyRequest                 = { 0 };
    dummyRequest.frame_number                              = -1;
    dummyRequest.input_buffer                              = NULL;
    dummyRequest.num_output_buffers                        = m_numOutputBuffers;
    dummyRequest.settings                                  = NULL;

    camera3_stream_buffer_t   outputBuffer[m_numOutputBuffers];
    for (UINT i = 0; i < m_numOutputBuffers; i++)
    {
        outputBuffer[i].acquire_fence = InvalidNativeFence;
        outputBuffer[i].release_fence = InvalidNativeFence;
        outputBuffer[i].stream        = reinterpret_cast<camera3_stream_t*>(m_pipelineOutputBuffer[i].pStream);
        outputBuffer[i].buffer        = NULL;
    }
    dummyRequest.output_buffers  = outputBuffer;
    chiTuningModeParameter.TuningMode[2].subMode.usecase   =  ChxUtils::GetUsecaseMode(&dummyRequest);

    ChxUtils::SetVendorTagValue(m_pPipelineDescriptorMetadata, VendorTag::TuningMode, sizeof(ChiTuningModeParameter),
        &chiTuningModeParameter);
}
