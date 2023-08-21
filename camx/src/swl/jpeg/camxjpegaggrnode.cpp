////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxjpegaggrnode.cpp
/// @brief JPEG aggregator node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-1988) Code clean up apply NC001 coding standard on entire project
/// @todo (CAMX-1989) Code clean up apply GR030 coding standard on entire project

#include "camxcsl.h"
#include "camxmem.h"
#include "camxutils.h"
#include "camxtrace.h"
#include "camxhwdefs.h"
#include "camxhwcontext.h"
#include "camxpropertyblob.h"
#include "camxjpegaggrnode.h"
#include "camxcsljumptable.h"
#include "camxcslresourcedefs.h"
#include "camxhal3module.h"

CAMX_NAMESPACE_BEGIN

static const UINT MaxJPEGAggrOutput             = 1;                        ///< Max output ports
static const UINT MaxJPEGAggrInput              = 2;                        ///< Max input ports

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGAggrNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGAggrNode* JPEGAggrNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    return CAMX_NEW JPEGAggrNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGAggrNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGAggrNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGAggrNode::JPEGAggrNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGAggrNode::JPEGAggrNode()
    : m_pEXIFParams(NULL)
    , m_pEXIFComposer(NULL)
{
    m_pNodeName = "JPEG_Aggregator";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGAggrNode::~JPEGAggrNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGAggrNode::~JPEGAggrNode()
{
    if (NULL != m_pEXIFParams)
    {
        CAMX_DELETE m_pEXIFParams;
        m_pEXIFParams = NULL;
    }

    if (NULL != m_pEXIFComposer)
    {
        CAMX_DELETE m_pEXIFComposer;
        m_pEXIFComposer = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGAggrNode::GetMaxJpegSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGAggrNode::GetMaxJpegSize(
    const NodeCreateInputData* pCreateInputData)
{
    CamxResult  result = CamxResultSuccess;
    m_maxjpegsize      = 0;

    UINT32          propertyCount   = pCreateInputData->pNodeInfo->nodePropertyCount;

    for (UINT32 count = 0; count < propertyCount; count++)
    {
        if (NodePropertyStitchMaxJpegSize == pCreateInputData->pNodeInfo->pNodeProperties[count].id)
        {
            m_maxjpegsize =
                atoi(static_cast<const CHAR*>(pCreateInputData->pNodeInfo->pNodeProperties[count].pValue));
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGAggrNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGAggrNode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CamxResult result = CamxResultSuccess;

    if ((NULL == pCreateInputData) || (NULL == pCreateOutputData))
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pCreateInputData %p pCreateOutputData %p",
                       pCreateInputData, pCreateOutputData);

        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        GetMaxJpegSize(pCreateInputData);

        pCreateOutputData->maxOutputPorts = MaxJPEGAggrOutput;
        pCreateOutputData->maxInputPorts  = MaxJPEGAggrInput;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGAggrNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGAggrNode::PostPipelineCreate()
{
    CamxResult result = CamxResultSuccess;

    // Determine if rotation is handled before JPEG encoding
    CHAR gpu[]           = "com.qti.node.gpu";
    CHAR fcv[]           = "com.qti.node.fcv";
    m_rotationHandledGPU = (GetStaticSettings()->overrideGPURotationUsecase &&
                            GetPipeline()->IsNodeExistByNodePropertyValue(gpu));
    m_rotationHandledFCV = (GetStaticSettings()->overrideGPURotationUsecase &&
                            GetPipeline()->IsNodeExistByNodePropertyValue(fcv));

    if ((FALSE == m_rotationHandledFCV) && (FALSE == m_rotationHandledGPU) &&
        (TRUE == GetPipeline()->IsNodeExistByNodePropertyValue(gpu)))
    {
        UINT32 tag;
        result = VendorTagManager::QueryVendorTagLocation("org.quic.camera.overrideGPURotationUsecase",
            "OverrideGPURotationUsecase", &tag);

        if (CamxResultSuccess == result)
        {
            UINT              properties[]      = { tag | UsecaseMetadataSectionMask };
            static const UINT Length            = CAMX_ARRAY_SIZE(properties);
            VOID*             pData[Length]     = { 0 };
            UINT64            offsets[Length]   = { 0 };

            result = GetDataList(properties, pData, offsets, Length);

            if (CamxResultSuccess == result)
            {
                if (NULL != pData[0])
                {
                    m_rotationHandledGPU = *reinterpret_cast<UINT*>(pData[0]);
                }
                else
                {
                    CAMX_LOG_WARN(CamxLogGroupJPEG, "Node: %s cannot get GPU rotation setting",
                        NodeIdentifierString());
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "QueryVendorTagLocation failed");
        }
    }

    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "%s: Rotation GPU %d, FCV %d",
                     NodeIdentifierString(), m_rotationHandledGPU, m_rotationHandledFCV);

    m_pEXIFParams = CAMX_NEW JPEGEXIFParams();
    if (NULL == m_pEXIFParams)
    {
        result = CamxResultENoMemory;
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Memory allocation for EXIF params failed");
    }

    if (CamxResultSuccess == result)
    {
        m_pEXIFComposer = CAMX_NEW JPEGEXIFComposer();
        if (NULL == m_pEXIFComposer)
        {
            result = CamxResultENoMemory;
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "Memory allocation for EXIF composer failed");
        }
    }

    if (CamxResultSuccess == result)
    {
        result = m_pEXIFParams->Initialize(this, IsRealTime(), (m_rotationHandledGPU || m_rotationHandledFCV));
    }

    if (CamxResultSuccess == result)
    {
        result = m_pEXIFComposer->Initialize();
    }

    if (CamxResultSuccess != result)
    {
        if (NULL != m_pEXIFParams)
        {
            CAMX_DELETE m_pEXIFParams;
            m_pEXIFParams = NULL;
        }

        if (NULL != m_pEXIFComposer)
        {
            CAMX_DELETE m_pEXIFComposer;
            m_pEXIFComposer = NULL;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGAggrNode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGAggrNode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    CAMX_ASSERT(NULL != pBufferNegotiationData);

    CamxResult  result                     = CamxResultSuccess;
    UINT        optimalInputWidth          = 0;
    UINT        optimalInputHeight         = 0;
    UINT        perOutputPortOptimalWidth  = 0;
    UINT        perOutputPortOptimalHeight = 0;

    if (NULL == pBufferNegotiationData)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pBufferNegotiationData");
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        // The JPEG node will have to loop through all the output ports which are connected to a child node or a HAL target.
        for (UINT outputIndex = 0; outputIndex < pBufferNegotiationData->numOutputPortsNotified; outputIndex++)
        {
            OutputPortNegotiationData* pOutputPortNegotiationData =
                &pBufferNegotiationData->pOutputPortNegotiationData[outputIndex];

            perOutputPortOptimalWidth = 0;
            perOutputPortOptimalHeight = 0;

            Utils::Memset(&pOutputPortNegotiationData->outputBufferRequirementOptions, 0, sizeof(BufferRequirement));

            // Go through the requirements of the input ports connected to the output port
            for (UINT inputIndex = 0; inputIndex < pOutputPortNegotiationData->numInputPortsNotification; inputIndex++)
            {
                BufferRequirement* pInputPortRequirement = &pOutputPortNegotiationData->inputPortRequirement[inputIndex];

                if (perOutputPortOptimalWidth < pInputPortRequirement->optimalWidth)
                {
                    perOutputPortOptimalWidth = pInputPortRequirement->optimalWidth;
                }

                if (perOutputPortOptimalHeight < pInputPortRequirement->optimalHeight)
                {
                    perOutputPortOptimalHeight = pInputPortRequirement->optimalHeight;
                }
            }

            /// Store the buffer requirements for this output port which will be reused to set, during forward walk.
            /// The values stored here could be final output dimensions unless it is overridden by forward walk.
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth  = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight = perOutputPortOptimalHeight;
            pOutputPortNegotiationData->outputBufferRequirementOptions.minWidth      = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxWidth      = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.minHeight     = perOutputPortOptimalHeight;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxHeight     = perOutputPortOptimalHeight;

            CAMX_LOG_INFO(CamxLogGroupJPEG,
                          "JPEG Aggr Optimal Output Dim, W:%d x H:%d!\n",
                          perOutputPortOptimalWidth,
                          perOutputPortOptimalHeight);

            if (optimalInputWidth < perOutputPortOptimalWidth)
            {
                optimalInputWidth = perOutputPortOptimalWidth;
            }

            if (optimalInputHeight < perOutputPortOptimalHeight)
            {
                optimalInputHeight = perOutputPortOptimalHeight;
            }
        }

        m_outputPortMaxWidth    = optimalInputWidth;
        m_outputPortMaxHeight   = optimalInputHeight;

        if ((0 == optimalInputWidth) || (0 == optimalInputHeight))
        {
            result = CamxResultEFailed;

            CAMX_LOG_ERROR(CamxLogGroupJPEG,
                           "JPEG Aggr Buffer Negotiation Failed, W:%d x H:%d!\n",
                           optimalInputWidth,
                           optimalInputHeight);
        }
        else
        {
            CAMX_LOG_INFO(CamxLogGroupJPEG,
                          "JPEG Aggr Optimal Input Dim, W:%d x H:%d!\n",
                          optimalInputWidth,
                          optimalInputHeight);

            UINT32 numInputPorts = 0;
            UINT32 inputPortId[MaxJPEGAggrInput];

            // Get Input Port List
            GetAllInputPortIds(&numInputPorts, &inputPortId[0]);

            pBufferNegotiationData->numInputPorts = numInputPorts;

            for (UINT input = 0; input < numInputPorts; input++)
            {
                pBufferNegotiationData->inputBufferOptions[input].nodeId     = Type();
                pBufferNegotiationData->inputBufferOptions[input].instanceId = InstanceID();
                pBufferNegotiationData->inputBufferOptions[input].portId     = inputPortId[input];

                BufferRequirement* pInputBufferRequirement =
                    &pBufferNegotiationData->inputBufferOptions[input].bufferRequirement;

                pInputBufferRequirement->optimalWidth   = optimalInputWidth;
                pInputBufferRequirement->optimalHeight  = optimalInputHeight;
                pInputBufferRequirement->minWidth       = optimalInputWidth;
                pInputBufferRequirement->minHeight      = optimalInputHeight;
                pInputBufferRequirement->maxWidth       = optimalInputWidth;
                pInputBufferRequirement->maxHeight      = optimalInputHeight;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGAggrNode::GetThumbnailFormat
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGAggrNode::GetThumbnailFormat(
    ImageFormat* pThumbnailFormat)
{
    CamxResult          result              = CamxResultSuccess;
    UINT                jpegTag[1]          = { 0 };
    VOID*               pData[1]            = { 0 };
    UINT                length              = CAMX_ARRAY_SIZE(jpegTag);
    UINT64              jpegDataOffset[1]   = { 0 };
    DimensionCap*       pThumbnailDim       = NULL;

    jpegTag[0] = InputJPEGThumbnailSize;

    result = GetDataList(jpegTag, pData, jpegDataOffset, length);
    if (NULL == pThumbnailFormat)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "JPEG Thumb dims NULL");
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        pThumbnailDim = reinterpret_cast<DimensionCap*>(pData[0]);
        if (NULL != pThumbnailDim)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "JPEG Thumb dims %d %d",
                             pThumbnailDim->width, pThumbnailDim->height);
            pThumbnailFormat->width  = pThumbnailDim->width;
            pThumbnailFormat->height = pThumbnailDim->height;
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "JPEG Thumb dims NULL");
        }
    }
    else
    {
        CAMX_LOG_WARN(CamxLogGroupJPEG, "JPEG Thumb Dims not available");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGAggrNode::CheckIfThumbNeeded
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL JPEGAggrNode::CheckIfThumbNeeded()
{
    CamxResult  result          = CamxResultSuccess;
    ImageFormat thumbnailFormat = {};
    BOOL        bThumbnailNeeded = FALSE;

    if (FALSE == IsTagPresentInPublishList(PropertyIDJPEGEncodeOutInfoThumbnail))
    {
        bThumbnailNeeded = FALSE;
    }
    else
    {
        result = GetThumbnailFormat(&thumbnailFormat);

        if ((0 == thumbnailFormat.width) || (0 == thumbnailFormat.height) || (CamxResultSuccess != result))
        {
            bThumbnailNeeded = FALSE;
        }
        else
        {
            bThumbnailNeeded = TRUE;
        }

    }

    return bThumbnailNeeded;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGAggrNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGAggrNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CamxResult                  result              = CamxResultSuccess;
    NodeProcessRequestData*     pNodeRequestData    = NULL;
    PerRequestActivePorts*      pPerRequestPorts    = NULL;
    PerRequestOutputPortInfo*   pOutputPort         = NULL;
    ImageBuffer*                pOutputImageBuffer  = NULL;
    CaptureRequest*             pCaptureRequest     = NULL;
    BOOL                        bThumbnailNeeded    = FALSE;
    BOOL                        bMainImageNeeded    = FALSE;

    CAMX_ENTRYEXIT(CamxLogGroupJPEG);

    if (NULL == pExecuteProcessRequestData)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pExecuteProcessRequestData %p", pExecuteProcessRequestData);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
        pPerRequestPorts = pExecuteProcessRequestData->pEnabledPortsInfo;

        if ((NULL == pNodeRequestData) || (NULL == pPerRequestPorts))
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pNodeRequestData %p pPerRequestPorts %p", pNodeRequestData,
                           pPerRequestPorts);

            result = CamxResultEInvalidPointer;
        }
    }
    INT sequenceNumber = 0;
    if (CamxResultSuccess == result)
    {
        pCaptureRequest = pNodeRequestData->pCaptureRequest;
        sequenceNumber  = pNodeRequestData->processSequenceId;

        if ((NULL == pCaptureRequest) || (NULL == pCaptureRequest->pStreamBuffers))
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pCaptureRequest %p or pStreamBuffers = NULL", pCaptureRequest);
            result = CamxResultEInvalidPointer;
        }
    }

    if ((CamxResultSuccess == result) && (0 == sequenceNumber))
    {
        SetDependencies(pNodeRequestData, pPerRequestPorts);
    }

    if ((CamxResultSuccess == result) && (1 == sequenceNumber))
    {
        bMainImageNeeded = CheckIfMainEncNeeded();
        if (TRUE == bMainImageNeeded)
        {
            static const UINT   PropertiesJpeg[]            = { PropertyIDJPEGEncodeOutInfo };
            VOID*               pJPEGEncodeOutInfoData[1]   = { 0 };
            UINT                lengthMain                  = CAMX_ARRAY_SIZE(PropertiesJpeg);
            UINT64              propertyDataJpegOffset[1]   = { 0 };

            GetDataList(PropertiesJpeg, pJPEGEncodeOutInfoData, propertyDataJpegOffset, lengthMain);
            if (NULL != pJPEGEncodeOutInfoData[0])
            {
                m_encodeOutParams = *reinterpret_cast<EncoderOutInfo*>(pJPEGEncodeOutInfoData[0]);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "failed to get Main Encoder OutInfo");
                result = CamxResultEInvalidArg;
            }

            if ((CamxResultSuccess == result) && (m_encodeOutParams.bufferFilledSize <= 0))
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid encoded buffer size! Main Image Encoded size %d",
                               m_encodeOutParams.bufferFilledSize);
                result = CamxResultEOutOfBounds;
            }
        }

        if (CamxResultSuccess == result)
        {
            if (TRUE == m_rotationHandledGPU)
            {
                // Get the GPU rotation metadata
                UINT32 metaTag = 0;
                result = VendorTagManager::QueryVendorTagLocation("com.qti.node.gpu", "FrameDimension", &metaTag);

                if (CamxResultSuccess == result)
                {
                    static const UINT PropertiesGPURotation[]          = { metaTag };
                    VOID*             pGPURotationData[1]              = { 0 };
                    UINT64            propertyDataGPURotationOffset[1] = { 0 };

                    UINT lengthGPU = CAMX_ARRAY_SIZE(PropertiesGPURotation);

                    GetDataList(PropertiesGPURotation, pGPURotationData, propertyDataGPURotationOffset, lengthGPU);
                    if (NULL != pGPURotationData[0])
                    {
                        m_rotatedDimensions[0] = reinterpret_cast<UINT32*>(pGPURotationData[0])[0];
                        m_rotatedDimensions[1] = reinterpret_cast<UINT32*>(pGPURotationData[0])[1];
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupJPEG, "failed to get GPU rotated dimensions");
                        result = CamxResultEInvalidArg;
                    }
                }
            }
            else if (TRUE == m_rotationHandledFCV)
            {
                // Get the GPU rotation metadata
                UINT32 metaTag = 0;
                result = VendorTagManager::QueryVendorTagLocation("com.qti.node.fcv", "FrameDimension", &metaTag);

                if (CamxResultSuccess == result)
                {
                    static const UINT PropertiesFCVRotation[]          = { metaTag };
                    VOID*             pFCVRotationData[1]              = { 0 };
                    UINT64            propertyDataFCVRotationOffset[1] = { 0 };

                    UINT lengthFVC = CAMX_ARRAY_SIZE(PropertiesFCVRotation);

                    GetDataList(PropertiesFCVRotation, pFCVRotationData, propertyDataFCVRotationOffset, lengthFVC);
                    if (NULL != pFCVRotationData[0])
                    {
                        m_rotatedDimensions[0] = reinterpret_cast<UINT32*>(pFCVRotationData[0])[0];
                        m_rotatedDimensions[1] = reinterpret_cast<UINT32*>(pFCVRotationData[0])[1];
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupJPEG, "failed to get FCV rotated dimensions");
                        result = CamxResultEInvalidArg;
                    }
                }
            }
        }

        bThumbnailNeeded = CheckIfThumbNeeded();

        if ((TRUE == bThumbnailNeeded) && (CamxResultSuccess == result))
        {
            static const UINT   PropertiesJpegThumb[]               ={ PropertyIDJPEGEncodeOutInfoThumbnail };
            VOID*               pJPEGEncodeOutInfoThumbnailData[1]  = { 0 };
            UINT                lengthThmb                          = CAMX_ARRAY_SIZE(PropertiesJpegThumb);
            UINT64              JPEGEncodeOutInfoThumbnailOffset[1] = { 0 };

            GetDataList(PropertiesJpegThumb, pJPEGEncodeOutInfoThumbnailData, JPEGEncodeOutInfoThumbnailOffset, lengthThmb);
            if (NULL != pJPEGEncodeOutInfoThumbnailData[0])
            {
                m_encodeOutParamsThumbnail = *reinterpret_cast<EncoderOutInfo*>(pJPEGEncodeOutInfoThumbnailData[0]);
                if (m_encodeOutParamsThumbnail.bufferFilledSize <= 0)
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "Invalid encoded buffer size! Thumbnail Encoded size %d",
                        m_encodeOutParamsThumbnail.bufferFilledSize);
                    result = CamxResultEOutOfBounds;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "failed thumb encoder OutInfo");
                result = CamxResultEInvalidArg;
            }
        }

        if (CamxResultSuccess == result)
        {
            if (TRUE == bMainImageNeeded)
            {
                m_pEXIFParams->SetEncodeOutParams(&m_encodeOutParams);
            }
            if (TRUE == bThumbnailNeeded)
            {
                m_pEXIFParams->SetEncodeOutParamsThumbnail(&m_encodeOutParamsThumbnail);
            }

            m_pEXIFParams->SetMaxOuputDimentions(m_outputPortMaxWidth, m_outputPortMaxHeight);

            for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
            {
                PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];

                if ((NULL != pInputPort) && (NULL != pInputPort->pImageBuffer))
                {
                    if (pInputPort->portId == 0) /* Main Image */
                    {
                        ImageFormat imageFormat;
                        if (NULL != pInputPort->pImageBuffer->GetFormat())
                        {
                            imageFormat = *pInputPort->pImageBuffer->GetFormat();
                            if ((TRUE == m_rotationHandledGPU) || (TRUE == m_rotationHandledFCV))
                            {
                                imageFormat.width    = m_rotatedDimensions[0];
                                imageFormat.height   = m_rotatedDimensions[1];
                                imageFormat.rotation = Rotation::CW0Degrees;
                            }
                            CAMX_LOG_INFO(CamxLogGroupJPEG,
                                          "InPort %d JPEG Aggr reporting Input Main config portid %d"
                                          " width %d height %d rot: %d fence %d req %d",
                                          i,
                                          pInputPort->portId,
                                          imageFormat.width,
                                          imageFormat.height,
                                          imageFormat.rotation,
                                          *pInputPort->phFence,
                                          pNodeRequestData->pCaptureRequest->requestId);

                            result = m_pEXIFParams->SetEXIFImageParams(imageFormat);
                        }
                        m_pEXIFComposer->AddInputMainBuf(pInputPort->pImageBuffer);
                    }
                    else if (pInputPort->portId == 1)  /* Thumbnail Image */
                    {
                        if (NULL != pInputPort->pImageBuffer->GetFormat())
                        {
                            ImageFormat thumbnailFormat = *pInputPort->pImageBuffer->GetFormat();;
                            result = GetThumbnailFormat(&thumbnailFormat);
                            CAMX_LOG_INFO(CamxLogGroupJPEG,
                                          "InPort %d JPEG Aggr reporting Input Thumbnail config portid %d"
                                          " width %d height %d fence %d req %d",
                                          i,
                                          pInputPort->portId,
                                          thumbnailFormat.width,
                                          thumbnailFormat.height,
                                          *pInputPort->phFence,
                                          pNodeRequestData->pCaptureRequest->requestId);

                            if (CamxResultSuccess == result)
                            {
                                if ((TRUE == bThumbnailNeeded) &&
                                    (0 != thumbnailFormat.width) &&
                                    (0 != thumbnailFormat.height))
                                {
                                    m_pEXIFComposer->AddInputThumbBuffer(pInputPort->pImageBuffer);
                                    result = m_pEXIFParams->SetEXIFImageParamsThumb(thumbnailFormat);
                                }
                                else
                                {
                                    result = m_pEXIFParams->SetEXIFImageParamsThumb(thumbnailFormat);
                                }
                            }
                            if (CamxResultSuccess == result)
                            {
                                CHIDATASPACE dataspace = DataspaceUnknown;
                                for (UINT j = 0; j < pCaptureRequest->pStreamBuffers->numOutputBuffers; j++)
                                {
                                    CHISTREAM* pStream = pCaptureRequest->pStreamBuffers->outputBuffers[j].pStream;
                                    if ((NULL != pStream) && (DataspaceJPEGAPPSegments == pStream->dataspace))
                                    {
                                        dataspace = pStream->dataspace;
                                        break;
                                    }
                                }
                                m_pEXIFParams->SetDataSpaceThumb(dataspace);
                            }
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupJPEG, "%s: format is Null ", __FUNCTION__);
                            result = CamxResultEInvalidArg;
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupJPEG, "%s: Unknown Input Port", __FUNCTION__);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "%s: Input Port/Image Buffer is Null ", __FUNCTION__);

                    result = CamxResultEInvalidArg;
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "Input Port: Add IO config failed i %d", i);
                    break;
                }
            }
        }

        if ((CamxResultSuccess == result) && (pPerRequestPorts->numOutputPorts == 1))
        {
            pOutputPort = &pPerRequestPorts->pOutputPorts[0];

            if (NULL != pOutputPort)
            {
                pOutputImageBuffer = pOutputPort->ppImageBuffer[0];
                CAMX_LOG_INFO(CamxLogGroupJPEG,
                              "JPEG Aggr reporting I/O config, portId=%d, hFence=%d, request=%llu",
                              pOutputPort->portId,
                              *(pOutputPort->phFence),
                              pNodeRequestData->pCaptureRequest->requestId);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "%s: Output Port is Null ", __FUNCTION__);
                result = CamxResultEInvalidArg;
            }

            if (NULL != pOutputImageBuffer)
            {
                m_pEXIFComposer->AddOutputBuf(pOutputImageBuffer);
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "%s: Output Image is Null ", __FUNCTION__);
                result = CamxResultEInvalidArg;
            }

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "Output Port: Add IO config failed");
            }
        }

        if (CamxResultSuccess == result)
        {
            const StaticSettings* pStaticSettings    = HwEnvironment::GetInstance()->GetStaticSettings();
            UINT propertiesDebugDataJpeg[] = { 0 };
            if (TRUE == IsRealTime())
            {
                propertiesDebugDataJpeg[0] = PropertyIDDebugDataAll;
            }
            else
            {
                UINT metaTag = 0;
                VendorTagManager::QueryVendorTagLocation("org.quic.camera.debugdata", "DebugDataAll", &metaTag);
                propertiesDebugDataJpeg[0] = metaTag | InputMetadataSectionMask;
            }

            static const UINT Length = CAMX_ARRAY_SIZE(propertiesDebugDataJpeg);
            VOID* pPropertyDebugDataJpegData[Length] = { 0 };
            UINT64 propertyDebugDataJpegOffset[Length] = { 0 };
            GetDataList(propertiesDebugDataJpeg, pPropertyDebugDataJpegData, propertyDebugDataJpegOffset, Length);

            if (NULL != pPropertyDebugDataJpegData[0])
            {
                DebugData* pDebugData = reinterpret_cast<DebugData*>(pPropertyDebugDataJpegData[0]);

                m_pEXIFComposer->SetStatsDebugData(reinterpret_cast<UINT8*>(pDebugData->pData),
                                                    static_cast<UINT32>(pDebugData->size),
                                                    pStaticSettings->debugDataHeaderString);
            }
            else
            {
                CAMX_LOG_INFO(CamxLogGroupJPEG, "DebugDataAll: property not available for writing debug data.");
            }

            // 3A debug data failures are non-fatal
            result = CamxResultSuccess;
        }

        if (CamxResultSuccess == result)
        {
            UINT propertiesOEMAppDataJpeg[] ={ 0 };

            UINT metaTag = 0;
            VendorTagManager::QueryVendorTagLocation("org.quic.camera.oemexifappdata", "OEMEXIFAppData1", &metaTag);
            propertiesOEMAppDataJpeg[0] = metaTag | InputMetadataSectionMask;

            static const UINT Length = CAMX_ARRAY_SIZE(propertiesOEMAppDataJpeg);
            VOID* pData[Length] ={ 0 };
            UINT64 propertyOEMAppDataJpegOffset[Length] ={ 0 };
            GetDataList(propertiesOEMAppDataJpeg, pData, propertyOEMAppDataJpegOffset, Length);

            if (NULL != pData[0])
            {
                OEMJPEGEXIFAppData* pOEMAppData = reinterpret_cast<OEMJPEGEXIFAppData*>(pData[0]);

                CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "OEMEXIFAppData1: %p %d", pOEMAppData->pData, pOEMAppData->size);
                m_pEXIFComposer->SetOEMAppData(reinterpret_cast<UINT8*>(pOEMAppData->pData),
                    static_cast<UINT32>(pOEMAppData->size));
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "OEMEXIFAppData1: property not available for writing data.");
            }

            // Non fatal if data not available.
            result = CamxResultSuccess;
        }

        if (CamxResultSuccess == result)
        {
            m_pEXIFParams->GetEXIFData();

            m_pEXIFComposer->SetParams(m_pEXIFParams);
            result = m_pEXIFComposer->StartComposition();
        }

        if (CamxResultSuccess == result)
        {
            m_pEXIFParams->SetEXIFVendorTags();
        }

        if ((CamxResultSuccess == result) && (NULL != pOutputPort))
        {
            result = CSLFenceSignal(*pOutputPort->phFence, CSLFenceResultSuccess);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupJPEG, "JPEG aggregartion failed!!");
            if (NULL != pOutputPort)
            {
                result = CSLFenceSignal(*pOutputPort->phFence, CSLFenceResultFailed);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGAggrNode::SetPropertyDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGAggrNode::SetPropertyDependency(
    NodeProcessRequestData* pNodeRequestData,
    PerRequestActivePorts*  pEnabledPorts)
{
    CAMX_UNREFERENCED_PARAM(pEnabledPorts);
    CamxResult  result              = CamxResultSuccess;
    UINT32      count               = 0;
    BOOL        bThumbnailNeeded    = FALSE;
    ImageFormat thumbnailFormat;


    if (TRUE == IsTagPresentInPublishList(PropertyIDJPEGEncodeOutInfo))
    {
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
        pNodeRequestData->dependencyInfo[0].processSequenceId                     = 1;
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count]  = PropertyIDJPEGEncodeOutInfo;
        count++;
    }

    bThumbnailNeeded = CheckIfThumbNeeded();

    if (TRUE == bThumbnailNeeded)
    {
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count]  = PropertyIDJPEGEncodeOutInfoThumbnail;
        count++;
    }
    else
    {
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "pipeline cannot publish property: %08x or bThumbnailNeeded %d",
            PropertyIDJPEGEncodeOutInfoThumbnail, bThumbnailNeeded);
    }

    if (m_rotationHandledGPU == TRUE)
    {
        UINT32 metaTag = 0;
        VendorTagManager::QueryVendorTagLocation("com.qti.node.gpu", "FrameDimension", &metaTag);
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count]  = metaTag;

        count++;
    }
    else if (m_rotationHandledFCV == TRUE)
    {
        UINT32 metaTag = 0;
        VendorTagManager::QueryVendorTagLocation("com.qti.node.fcv", "FrameDimension", &metaTag);
        pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency = TRUE;
        pNodeRequestData->dependencyInfo[0].propertyDependency.properties[count]  = metaTag;

        count++;
    }

    pNodeRequestData->dependencyInfo[0].propertyDependency.count = count;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGAggrNode::SetBufferDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGAggrNode::SetBufferDependency(
    NodeProcessRequestData*   pNodeRequestData,
    PerRequestActivePorts*    pEnabledPorts)
{
    UINT fencesNotSignalled = 0;

    for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
    {
        PerRequestInputPortInfo* pPort = &pEnabledPorts->pInputPorts[i];

        if (NULL != pPort)
        {
            pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[fencesNotSignalled]         = pPort->phFence;
            pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[fencesNotSignalled] = pPort->pIsFenceSignaled;
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency        = TRUE;
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency     = TRUE;
            pNodeRequestData->dependencyInfo[0].processSequenceId                                     = 1;

            fencesNotSignalled++;
        }
    }
    pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount = fencesNotSignalled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGAggrNode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGAggrNode::SetDependencies(
    NodeProcessRequestData*   pNodeRequestData,
    PerRequestActivePorts*    pEnabledPorts)
{
    // First set all the buffer dependencies
    SetBufferDependency(pNodeRequestData, pEnabledPorts);
    // Set the property dependencies are satisfied
    SetPropertyDependency(pNodeRequestData, pEnabledPorts);

    if (0 != pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask)
    {
        pNodeRequestData->numDependencyLists = 1;
    }
}

CAMX_NAMESPACE_END
