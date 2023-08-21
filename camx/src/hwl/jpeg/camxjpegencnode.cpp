////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxjpegencnode.cpp
/// @brief JPEG Encoder Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// @todo (CAMX-1988) Code clean up apply NC001 coding standard on entire project
/// @todo (CAMX-1989) Code clean up apply GR030 coding standard on entire project

#include "camxtrace.h"
#include "camxcsljumptable.h"
#include "camxcslresourcedefs.h"
#include "camxhwcontext.h"
#include "camxvendortags.h"
#include "camximagebuffer.h"
#include "camximagebuffermanager.h"
#include "camxpropertyblob.h"
#include "camxtitan17xdefs.h"
#include "camxtrace.h"
#include "camxjpegencnode.h"

CAMX_NAMESPACE_BEGIN

static const UINT   JPEGMaxCmdBufferManagerCnt      = 3;            ///< Buffer manager for packet, cmdbuf, inputparam
                                                                    ///  1 extra is for Initial Configuration packet
static const UINT   MaxJPEGOutput                   = 1;            ///< Max output ports
static const UINT   MaxJPEGInput                    = 1;            ///< Max input ports
static const UINT   MaxJPEGCmdEntry                 = 300;          ///< Max input ports
static const UINT   SizePerEntry                    = 12;           ///< Max input ports
static const UINT   JPEGKMDCmdBufferMaxSize         = 256;          ///< Reserved KMD Cmd Buffer Size
static const UINT32 MaxJpegThumbnailQuality         = 85;           ///< Max Jpeg Thumbnail quality
static const DOUBLE MaxAspectRatioDiffTolerence     = 0.001f;       ///< Tolerance for aspect ratio comaparison
static const UINT   JPEGThumbnailWidthDefault       = 320;          ///< Default thumbnail width
static const UINT   JPEGThumbnailHeightDefault      = 240;          ///< Default thumbnail height
static const UINT32 OptimalThumbnailScalingFactor   = 2;            ///< Optimal thumbnail downscale ratio

// @brief list of metadata tags published by the JPEG encoder main node
static const UINT32 JPEGEncMainOutputTags[] =
{
    PropertyIDJPEGEncodeOutInfo,
};

// @brief list of metadata tags published by the JPEG encoder thumbnail
static const UINT32 JPEGEncThumbnailOutputTags[] =
{
    PropertyIDJPEGEncodeOutInfoThumbnail,
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGEncNode* JPEGEncNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    return CAMX_NEW JPEGEncNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEncNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);

    CamxResult                  result                          = CamxResultSuccess;
    INT32                       deviceIndex                     = -1;
    UINT                        indicesLengthRequired           = 0;
    const PlatformStaticCaps*   pStaticCaps                     = NULL;

    CAMX_ASSERT_MESSAGE(JPEG == Type(), "Node type not JPEG");

    if ((NULL == pCreateInputData) || (NULL == pCreateOutputData))
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pCreateInputData %p pCreateOutputData %p", pCreateInputData,
            pCreateOutputData);
        result = CamxResultEInvalidPointer;
    }

    if (CamxResultSuccess == result)
    {
        pCreateOutputData->maxOutputPorts = MaxJPEGOutput;
        pCreateOutputData->maxInputPorts  = MaxJPEGInput;

        // Add device indices
        result = HwEnvironment::GetInstance()->GetDeviceIndices(CSLDeviceTypeJPEGE, &deviceIndex, 1, &indicesLengthRequired);
    }

    if (CamxResultSuccess == result)
    {
        m_bThumbnailEncode    = FALSE;

        if (NULL != pCreateInputData->pNodeInfo)
        {
            UINT32 propertyCount = pCreateInputData->pNodeInfo->nodePropertyCount;
            CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "nodePropertyCount %d", propertyCount);
            for (UINT32 count = 0; count < propertyCount; count++)
            {
                // There can be multiple JPEG instances in a pipeline ProfileId 1 Thumbnail or 0 MainImage (Default)
                if (NodePropertyProfileId == pCreateInputData->pNodeInfo->pNodeProperties[count].id)
                {
                    m_bThumbnailEncode = static_cast<INT>(
                        atoi(static_cast<const CHAR*>(
                            pCreateInputData->pNodeInfo->pNodeProperties[count].pValue)));
                }
            }

        }

        CAMX_LOG_INFO(CamxLogGroupJPEG, "isThumbEncode= %d %p %d\n", m_bThumbnailEncode, this, InstanceID());

        if (TRUE == m_bThumbnailEncode)
        {
            UINT32 widthScalingMax  = 0;
            UINT32 heightScalingMax = 0;

            pStaticCaps = HwEnvironment::GetInstance()->GetPlatformStaticCaps();

            if ((NULL != pStaticCaps) && (0 < pStaticCaps->numJPEGThumbnailSizes))
            {
                CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "numJPEGThumbnailSizes %d", pStaticCaps->numJPEGThumbnailSizes);
                INT32 maxSize = 0x0;
                INT32 minSize = 0x7FFFFFFF;
                for (UINT ti = 0; ti < pStaticCaps->numJPEGThumbnailSizes; ti++)
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "JPEGThumbnailSizes @ %d - w %d h %d", ti,
                        pStaticCaps->JPEGThumbnailSizes[ti].width, pStaticCaps->JPEGThumbnailSizes[ti].height);
                    if (maxSize < (pStaticCaps->JPEGThumbnailSizes[ti].width * pStaticCaps->JPEGThumbnailSizes[ti].height))
                    {
                        maxSize = (pStaticCaps->JPEGThumbnailSizes[ti].width * pStaticCaps->JPEGThumbnailSizes[ti].height);
                        m_thumbnailWidthMax  = pStaticCaps->JPEGThumbnailSizes[ti].width;
                        m_thumbnailHeightMax = pStaticCaps->JPEGThumbnailSizes[ti].height;
                    }
                    if ((minSize > (pStaticCaps->JPEGThumbnailSizes[ti].width * pStaticCaps->JPEGThumbnailSizes[ti].height)) &&
                        (0 != pStaticCaps->JPEGThumbnailSizes[ti].width) && (0 != pStaticCaps->JPEGThumbnailSizes[ti].height))
                    {
                        minSize = (pStaticCaps->JPEGThumbnailSizes[ti].width * pStaticCaps->JPEGThumbnailSizes[ti].height);
                        m_thumbnailWidthMin  = pStaticCaps->JPEGThumbnailSizes[ti].width;
                        m_thumbnailHeightMin = pStaticCaps->JPEGThumbnailSizes[ti].height;
                    }
                }

                m_imageWidthMax      = pStaticCaps->defaultImageSizes[0].width;
                m_imageHeightMax     = pStaticCaps->defaultImageSizes[0].height;

                if ((0 == m_thumbnailWidthMin) ||
                    (0 == m_thumbnailHeightMin) ||
                    (0 == m_thumbnailWidthMax) ||
                    (0 == m_thumbnailHeightMax) ||
                    (0 == m_imageWidthMax) ||
                    (0 == m_imageHeightMax))
                {
                    CAMX_LOG_INFO(CamxLogGroupJPEG, "invalid dims %d %d, %d %d, %d %d",
                                  m_thumbnailWidthMin,
                                  m_thumbnailHeightMin,
                                  m_thumbnailWidthMax,
                                  m_thumbnailHeightMax,
                                  m_imageWidthMax,
                                  m_imageHeightMax);

                    m_thumbnailWidthMin  = JPEGThumbnailWidthDefault;
                    m_thumbnailHeightMin = JPEGThumbnailHeightDefault;
                    m_thumbnailWidthMax  = JPEGThumbnailWidthDefault;
                    m_thumbnailHeightMax = JPEGThumbnailHeightDefault;
                }
            }
            else
            {
                result = CamxResultEFailed;
            }

            CAMX_ASSERT(m_thumbnailWidthMin != 0);
            CAMX_ASSERT(m_thumbnailHeightMin != 0);
            widthScalingMax  = Utils::Ceiling(static_cast<FLOAT>(m_imageWidthMax) / static_cast<FLOAT>(m_thumbnailWidthMin));
            heightScalingMax = Utils::Ceiling(static_cast<FLOAT>(m_imageHeightMax) / static_cast<FLOAT>(m_thumbnailHeightMin));

            CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "scalingMax %d", Utils::MaxUINT32(widthScalingMax, heightScalingMax));
            m_prevModuleScaleFactorMax =
                Utils::Ceiling(static_cast<FLOAT>(Utils::MaxUINT32(widthScalingMax, heightScalingMax)) / JPEGMaxDownScaling);

            CAMX_LOG_INFO(CamxLogGroupJPEG, "Prev Module ScaleFactor Max %d", m_prevModuleScaleFactorMax);
        }

    }

    if (CamxResultSuccess == result)
    {
        CAMX_ASSERT(indicesLengthRequired == 1);
        SetDeviceIndex(deviceIndex);
        SetCmdBufResourceSize(MaxJPEGCmdEntry*SizePerEntry + JPEGKMDCmdBufferMaxSize);
        result = AddDeviceIndex(m_deviceIndex);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::PostPipelineCreate()
{
    CamxResult      result = CamxResultSuccess;
    ResourceParams  params = { 0 };

    m_JPEGCmdBlobCount     = GetPipeline()->GetRequestQueueDepth() + 1;

    m_pQuantTables = CAMX_NEW JPEGQuantTable[NumQuantTables];
    if (NULL != m_pQuantTables)
    {
        result = JPEGUtil::SetDefaultQuantizationTables(m_pQuantTables);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Failed to alloc mem for Quantization table %p", m_pQuantTables);
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        result = JPEGEncConfig::Create(&m_pJPEGEncConfigMod);
    }

    if (CamxResultSuccess == result)
    {
        // Initializate Command Buffer
        result = InitializeCmdBufferManagerList(JPEGMaxCmdBufferManagerCnt);
    }

    if (CamxResultSuccess == result)
    {
        params.usageFlags.packet                = 1;
        params.packetParams.maxNumCmdBuffers    = 3;
        params.packetParams.maxNumIOConfigs     = MaxJPEGInput + MaxJPEGOutput;
        params.packetParams.enableAddrPatching  = 1;
        params.packetParams.maxNumPatches       = 32;
        params.resourceSize                     = Packet::CalculatePacketSize(&params.packetParams);
        params.poolSize                         = m_JPEGCmdBlobCount * params.resourceSize;
        params.alignment                        = CamxPacketAlignmentInBytes;
        params.pDeviceIndices                   = &m_deviceIndex;
        params.numDevices                       = 1;
        params.memFlags                         = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        if (CamxResultSuccess == CreateCmdBufferManager("PacketManager", &params, &m_pJPEGPacketManager))
        {
            params                              = { 0 };
            params.resourceSize                 = m_cmdBufResourceSize;
            params.poolSize                     = m_JPEGCmdBlobCount * params.resourceSize;
            params.usageFlags.cmdBuffer         = 1;
            params.cmdParams.type               = CmdType::CDMDirect;
            params.cmdParams.enableAddrPatching = 1;
            params.cmdParams.maxNumNestedAddrs  = 32;
            params.alignment                    = CamxCommandBufferAlignmentInBytes;
            params.pDeviceIndices               = &m_deviceIndex;
            params.numDevices                   = 1;
            params.memFlags                     = CSLMemFlagUMDAccess;

            result = CreateCmdBufferManager("CmdBufferManager", &params, &m_pJPEGCmdBufferManager);
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to Create packet Manager");
        }
    }

    if (CamxResultSuccess == result)
    {
        params                              = { 0 };
        params.resourceSize                 = sizeof(CSLJPEGConfigInputInfo) * 12;
        params.poolSize                     = m_JPEGCmdBlobCount * params.resourceSize;
        params.usageFlags.cmdBuffer         = 1;
        params.cmdParams.type               = CmdType::Generic;
        params.cmdParams.enableAddrPatching = 1;
        params.cmdParams.maxNumNestedAddrs  = 32;
        params.alignment                    = CamxCommandBufferAlignmentInBytes;
        params.pDeviceIndices               = &m_deviceIndex;
        params.numDevices                   = 1;
        params.memFlags                     = CSLMemFlagKMDAccess | CSLMemFlagUMDAccess;

        result = CreateCmdBufferManager("InputParamCmdManager", &params, &m_pInputParamCmdManager);
    }
    else
    {
        CAMX_ASSERT_ALWAYS_MESSAGE("Failed to Create input param cmd buffer");
    }

    if ((CamxResultSuccess != result) && (NULL != m_pJPEGEncConfigMod))
    {
        if (NULL != m_pQuantTables)
        {
            CAMX_DELETE [] m_pQuantTables;
            m_pQuantTables = NULL;
        }
        m_pJPEGEncConfigMod->Destroy();
        m_pJPEGEncConfigMod = NULL;
    }

    if (CamxResultSuccess == result)
    {
        // Determine if rotation is handled before JPEG encoding
        CHAR gpu[]           = "com.qti.node.gpu";
        CHAR fcv[]           = "com.qti.node.fcv";
        m_rotationHandledGPU = (GetStaticSettings()->overrideGPURotationUsecase &&
                                GetPipeline()->IsNodeExistByNodePropertyValue(gpu));
        m_rotationHandledFCV = (GetStaticSettings()->overrideGPURotationUsecase &&
                                GetPipeline()->IsNodeExistByNodePropertyValue(fcv));

        // If static setting is not enabled, then check if Vendor tags are enabled or not
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

        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "%d: Rotation GPU %d, FCV %d",
                         InstanceID(), m_rotationHandledGPU, m_rotationHandledFCV);
        result = AcquireDevice();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncNode::PublishEncoderOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::PublishEncoderOutput(
    UINT64 requestId)
{
    UINT           requestIdIndex = (requestId % MaxRequestQueueDepth);
    EncoderOutInfo out            = { 0 };
    UINT           writeList[]    = { PropertyIDJPEGEncodeOutInfo };
    const VOID*    pData[]        = { &out };
    UINT           pDataCount[]   = { sizeof(out) };

    out.bufferFilledSize = *(m_fenceCallbackData[requestIdIndex].pBufferFilledSize);

    if (TRUE == m_bThumbnailEncode)
    {
        CAMX_LOG_INFO(CamxLogGroupJPEG, "Thumb encoded size %d", out.bufferFilledSize);
        writeList[0] = PropertyIDJPEGEncodeOutInfoThumbnail;
        return WriteDataList(writeList, pData, pDataCount, CAMX_ARRAY_SIZE(writeList));
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupJPEG, "Main encoded size %d", out.bufferFilledSize);
        return WriteDataList(writeList, pData, pDataCount, CAMX_ARRAY_SIZE(writeList));
    }


}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::UpdateCropForOutAspectRatio
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::UpdateCropForOutAspectRatio(
    JPEGEScaleConfig * pScaleCropReq,
    BOOL bCropWidth)
{
    CamxResult  result                   = CamxResultSuccess;
    INT32       croppedWidth             = 0;
    INT32       croppedHeight            = 0;

    if (TRUE == bCropWidth)
    {
        // Keep height constant
        croppedHeight = pScaleCropReq->cropHeight;
        croppedWidth  = static_cast<INT32>(floor((croppedHeight * pScaleCropReq->outputWidth) / pScaleCropReq->outputHeight));
        if (croppedWidth % 2)
        {
            croppedWidth -= 1;
        }
    }
    else
    {
        // Keep width constant
        croppedWidth  = pScaleCropReq->cropWidth;
        croppedHeight = static_cast<INT32>(floor((croppedWidth * pScaleCropReq->outputHeight) / pScaleCropReq->outputWidth));
        if (croppedHeight % 2)
        {
            croppedHeight -= 1;
        }
    }
    pScaleCropReq->hOffset = pScaleCropReq->hOffset + static_cast<INT32>(floor((pScaleCropReq->cropWidth - croppedWidth) / 2));
    if (pScaleCropReq->hOffset % 2)
    {
        pScaleCropReq->hOffset -= 1;
    }
    pScaleCropReq->vOffset = pScaleCropReq->vOffset +
        static_cast<INT32>(floor((pScaleCropReq->cropHeight - croppedHeight) / 2));
    if (pScaleCropReq->vOffset % 2)
    {
        pScaleCropReq->vOffset -= 1;
    }
    pScaleCropReq->cropWidth  = croppedWidth;
    pScaleCropReq->cropHeight = croppedHeight;

    pScaleCropReq->hOffset = 0;
    pScaleCropReq->vOffset = 0;

    CAMX_LOG_VERBOSE(CamxLogGroupJPEG,
                     "updated thumbnail crop: left %d, top %d, crop width %d, crop height %d",
                     pScaleCropReq->hOffset,
                     pScaleCropReq->vOffset,
                     pScaleCropReq->cropWidth,
                     pScaleCropReq->cropHeight);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CamxResult              result                = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData      = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pPerRequestPorts      = pExecuteProcessRequestData->pEnabledPortsInfo;
    CmdBuffer*              pSettingsCmdBuffer    = NULL;
    CmdBuffer*              pInputParamsCmdBuffer = NULL;
    Packet*                 pPacket               = NULL;
    CaptureRequest*         pCaptureRequest       = pNodeRequestData->pCaptureRequest;
    JPEGInputData           moduleInput           = {};
    CSLJPEGConfigInputInfo  inputInfo             = {};
    CSLJPEGConfigInputInfo* pInputInfo            = NULL;
    UINT64                  requestId             = pCaptureRequest->requestId;
    UINT                    requestIdIndex        = 0;
    BOOL                    skipMainProcessing    = FALSE;
    INT                     sequenceNumber        = pNodeRequestData->processSequenceId;
    UINT32                  cmdBufferIndex        = 0;
    BOOL                    bHasDependencies      = FALSE;

    if (0 == sequenceNumber)
    {
        SetDependencies(pExecuteProcessRequestData);
    }

    if (pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask)
    {
        bHasDependencies = TRUE;
    }

    if (1 == sequenceNumber)
    {
        if (TRUE == m_rotationHandledGPU)
        {
            // Get the GPU rotation metadata
            UINT32 metaTag = 0;
            result = VendorTagManager::QueryVendorTagLocation("com.qti.node.gpu", "FrameDimension", &metaTag);

            if (CamxResultSuccess == result)
            {
                static const UINT PropertiesGPURotation[]          = { metaTag };
                VOID*             pData[1]                         = { 0 };
                UINT              length                           = CAMX_ARRAY_SIZE(PropertiesGPURotation);
                UINT64            propertyDataGPURotationOffset[1] = { 0 };

                GetDataList(PropertiesGPURotation, pData, propertyDataGPURotationOffset, length);
                m_rotatedDimensions[0] = reinterpret_cast<UINT32*>(pData[0])[0];
                m_rotatedDimensions[1] = reinterpret_cast<UINT32*>(pData[0])[1];
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
                VOID*             pData[1]                         = { 0 };
                UINT              length                           = CAMX_ARRAY_SIZE(PropertiesFCVRotation);
                UINT64            propertyDataFCVRotationOffset[1] = { 0 };

                GetDataList(PropertiesFCVRotation, pData, propertyDataFCVRotationOffset, length);
                m_rotatedDimensions[0] = reinterpret_cast<UINT32*>(pData[0])[0];
                m_rotatedDimensions[1] = reinterpret_cast<UINT32*>(pData[0])[1];
            }
        }
    }
    else if (2 == sequenceNumber)
    {
        PublishEncoderOutput(requestId);
        skipMainProcessing = TRUE; // Do not submit again
    }

    if ((CamxResultSuccess == result) &&
        (FALSE == skipMainProcessing) &&
        ((FALSE == bHasDependencies) || (1 == sequenceNumber)))
    {
        UINT fenceCount  = 0;

        if (CamxResultSuccess == result)
        {
            pSettingsCmdBuffer = GetCmdBufferForRequest(pNodeRequestData->pCaptureRequest->requestId,
                                                        m_pJPEGCmdBufferManager);
            pInputParamsCmdBuffer = GetCmdBufferForRequest(pNodeRequestData->pCaptureRequest->requestId,
                                                           m_pInputParamCmdManager);
            pPacket = GetPacketForRequest(pNodeRequestData->pCaptureRequest->requestId,
                                          m_pJPEGPacketManager);

            if ((NULL == pSettingsCmdBuffer) || (NULL == pPacket) || (NULL == pInputParamsCmdBuffer))
            {
                CAMX_ASSERT_ALWAYS_MESSAGE("Null CmdBuffer/IQPacket %x, %x %x",
                                           pSettingsCmdBuffer,
                                           pPacket,
                                           pInputParamsCmdBuffer);
                result = CamxResultENoMemory;
            }
        }

        if (CamxResultSuccess == result)
        {
            requestId = pNodeRequestData->pCaptureRequest->requestId;
            requestIdIndex = (requestId % MaxRequestQueueDepth);

            pPacket->SetOpcode(CSLDeviceTypeJPEGE, CSLPacketOpcodesIPEUpdate);

            moduleInput.pHwContext = GetHwContext();
            inputInfo.clk_index = 3;
            inputInfo.output_size = -1;

            VOID* pCmdBegin = pInputParamsCmdBuffer->BeginCommands(sizeof(CSLJPEGConfigInputInfo) / sizeof(UINT32));

            if (NULL != pCmdBegin)
            {
                pInputInfo = reinterpret_cast<CSLJPEGConfigInputInfo*>(pCmdBegin);
                *pInputInfo = inputInfo;
                result = pInputParamsCmdBuffer->CommitCommands();

#if defined (__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Waddress"
                // Forcing alignment for struct CSLJPEGConfigInputInfo so that we can safely ignore the warning
                m_fenceCallbackData[requestIdIndex].pBufferFilledSize = &pInputInfo->output_size;
#pragma GCC diagnostic pop
#endif // __GNUC__
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "NULL params pCmdBegin %p", pCmdBegin);
                result = CamxResultEInvalidPointer;
            }
        }

        if (CamxResultSuccess == result)
        {
            if (MaxJPEGInput != pPerRequestPorts->numInputPorts)
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "unexpected number of input ports %d", pPerRequestPorts->numInputPorts);
            }

            for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
            {
                PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];

                if ((NULL != pInputPort) && (NULL != pInputPort->pImageBuffer))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Input Plane 0 (width %d height %d stride %d scanline %d size %d), "
                                   "Plane1 (width %d height %d stride %d slice %d size %d), fd %d",
                                   pInputPort->pImageBuffer->GetFormat()->formatParams.yuvFormat[0].width,
                                   pInputPort->pImageBuffer->GetFormat()->formatParams.yuvFormat[0].height,
                                   pInputPort->pImageBuffer->GetFormat()->formatParams.yuvFormat[0].planeStride,
                                   pInputPort->pImageBuffer->GetFormat()->formatParams.yuvFormat[0].sliceHeight,
                                   static_cast<UINT32>(pInputPort->pImageBuffer->GetPlaneSize(0)),
                                   pInputPort->pImageBuffer->GetFormat()->formatParams.yuvFormat[1].width,
                                   pInputPort->pImageBuffer->GetFormat()->formatParams.yuvFormat[1].height,
                                   pInputPort->pImageBuffer->GetFormat()->formatParams.yuvFormat[1].planeStride,
                                   pInputPort->pImageBuffer->GetFormat()->formatParams.yuvFormat[1].sliceHeight,
                                   static_cast<UINT32>(pInputPort->pImageBuffer->GetPlaneSize(1)),
                                   pInputPort->pImageBuffer->GetFileDescriptor());

                    result = pPacket->AddIOConfig(pInputPort->pImageBuffer,
                                                  pInputPort->portId,
                                                  CSLIODirection::CSLIODirectionInput,
                                                  pInputPort->phFence,
                                                  1,
                                                  NULL,
                                                  NULL,
                                                  0);

                    CAMX_LOG_VERBOSE(CamxLogGroupJPEG,
                                     "JPEG Enc reporting Input config, portId=%d, imgBuf=0x%x, hFence=%d, request=%llu",
                                     pInputPort->portId,
                                     pInputPort->pImageBuffer,
                                     *(pInputPort->phFence),
                                     pNodeRequestData->pCaptureRequest->requestId);

                    if (CamxResultSuccess == result)
                    {
                        moduleInput.inFormat = *(pInputPort->pImageBuffer->GetFormat());
                        if ((TRUE == m_rotationHandledGPU) || (TRUE == m_rotationHandledFCV))
                        {
                            moduleInput.inFormat.width = m_rotatedDimensions[0];
                            moduleInput.inFormat.height = m_rotatedDimensions[1];
                        }
                        moduleInput.pInYBufAddr = pInputPort->pImageBuffer->GetPlaneVirtualAddr(0, 0);
                        moduleInput.pInCBCRBufAddr = pInputPort->pImageBuffer->GetPlaneVirtualAddr(0, 1);
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "Input Port/Image Buffer is Null ");

                    result = CamxResultEInvalidArg;
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "Input Port: Add IO config failed i %d", i);
                    break;
                }
            }
        }

        if ((CamxResultSuccess == result) && (TRUE == m_bThumbnailEncode))
        {
            UINT   jpegTag[1]        = { 0 };
            VOID*  pData[1]          = { 0 };
            UINT   length            = CAMX_ARRAY_SIZE(jpegTag);
            UINT64 jpegDataOffset[1] = { 0 };
            DimensionCap*   pThumbnailDim = NULL;

            jpegTag[0] = InputJPEGThumbnailSize;

            result = GetDataList(jpegTag, pData, jpegDataOffset, length);

            if (CamxResultSuccess == result)
            {
                pThumbnailDim = reinterpret_cast<DimensionCap*>(pData[0]);
                if (NULL != pThumbnailDim)
                {
                    CAMX_LOG_INFO(CamxLogGroupJPEG, "Input JPEG Thumb dims %d %d", pThumbnailDim->width, pThumbnailDim->height);
                    m_curThumbnailDim.width = pThumbnailDim->width;
                    m_curThumbnailDim.height = pThumbnailDim->height;
                }
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupJPEG, "JPEGQuality not available, using previous setting");
            }

            if ((0 == m_curThumbnailDim.width) || (0 == m_curThumbnailDim.height))
            {
                m_curThumbnailDim.width  = JPEGThumbnailWidthDefault;
                m_curThumbnailDim.height = JPEGThumbnailHeightDefault;
            }

            CAMX_LOG_INFO(CamxLogGroupJPEG, "JPEG Thumb dims %d %d", m_curThumbnailDim.width, m_curThumbnailDim.height);

        }

        if (CamxResultSuccess == result)
        {
            if (MaxJPEGOutput != pPerRequestPorts->numOutputPorts)
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "unexpected number of output ports %d", pPerRequestPorts->numOutputPorts);
            }

            for (UINT portIndex = 0; portIndex < pPerRequestPorts->numOutputPorts; portIndex++)
            {
                PerRequestOutputPortInfo* pOutputPort = &pPerRequestPorts->pOutputPorts[portIndex];

                CAMX_ASSERT(NULL != pOutputPort);

                for (UINT bufferIndex = 0; bufferIndex < pOutputPort->numOutputBuffers; bufferIndex++)
                {
                    ImageBuffer* pImageBuffer = pOutputPort->ppImageBuffer[bufferIndex];

                    if (NULL != pImageBuffer)
                    {
                        if (m_bThumbnailEncode == FALSE &&
                        (0x03FFFFFF <= pImageBuffer->GetPlaneSize(0) + pImageBuffer->GetPlaneSize(1)))
                        {

                            Utils::Memset(static_cast<VOID*>(pImageBuffer->GetPlaneVirtualAddr(0, 0)), '\0',
                                pImageBuffer->GetPlaneSize(0) + pImageBuffer->GetPlaneSize(1));

                            if (pImageBuffer->CacheOps(FALSE, TRUE) != CamxResultSuccess)
                            {
                                CAMX_LOG_ERROR(CamxLogGroupJPEG, "Main image output buffer cache flush failed");
                            }
                        }

                        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Output Plane 0 (width  %d height %d stride %d scanline %d size %d),"
                                       "Plane1 (width %d height %d stride %d slice %d size %d)fd %d",
                                       pImageBuffer->GetFormat()->formatParams.yuvFormat[0].width,
                                       pImageBuffer->GetFormat()->formatParams.yuvFormat[0].height,
                                       pImageBuffer->GetFormat()->formatParams.yuvFormat[0].planeStride,
                                       pImageBuffer->GetFormat()->formatParams.yuvFormat[0].sliceHeight,
                                       static_cast<UINT32>(pImageBuffer->GetPlaneSize(0)),
                                       pImageBuffer->GetFormat()->formatParams.yuvFormat[1].width,
                                       pImageBuffer->GetFormat()->formatParams.yuvFormat[1].height,
                                       pImageBuffer->GetFormat()->formatParams.yuvFormat[1].planeStride,
                                       pImageBuffer->GetFormat()->formatParams.yuvFormat[1].sliceHeight,
                                       static_cast<UINT32>(pImageBuffer->GetPlaneSize(1)),
                                       pImageBuffer->GetFileDescriptor());

                        if (TRUE == m_bThumbnailEncode)
                        {
                            if ((m_curThumbnailDim.width >
                                static_cast<INT32>(pImageBuffer->GetFormat()->formatParams.yuvFormat[0].width)) ||
                                (m_curThumbnailDim.height >
                                    static_cast<INT32>(pImageBuffer->GetFormat()->formatParams.yuvFormat[0].height)))
                            {
                                CAMX_LOG_ERROR(CamxLogGroupJPEG, "Thumbnail output requirement is more than output"
                                              "buffer, so chance of page fault, buffer W %d H %d"
                                              "and thumbnail  W %d H %d",
                                              pImageBuffer->GetFormat()->formatParams.yuvFormat[0].width,
                                              pImageBuffer->GetFormat()->formatParams.yuvFormat[0].height,
                                              m_curThumbnailDim.width, m_curThumbnailDim.height);

                                result = CamxResultEInvalidArg;
                            }
                        }

                        if (CamxResultSuccess == result)
                        {
                            result = pPacket->AddIOConfig(pImageBuffer,
                                                          pOutputPort->portId,
                                                          CSLIODirection::CSLIODirectionOutput,
                                                          pOutputPort->phFence,
                                                          1,
                                                          NULL,
                                                          NULL,
                                                          0);

                            pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[fenceCount] =
                                pOutputPort->pIsFenceSignaled;
                            pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[fenceCount++]       =
                                pOutputPort->phFence;

                            CAMX_LOG_INFO(CamxLogGroupJPEG,
                                          "JPEG Enc reporting I/O config, portId=%d, hFence=%d, request=%llu",
                                          pOutputPort->portId,
                                          *(pOutputPort->phFence),
                                          pNodeRequestData->pCaptureRequest->requestId);
                        }

                        if (CamxResultSuccess == result)
                        {
                            moduleInput.outFormat = *(pImageBuffer->GetFormat());
                            if ((TRUE == m_rotationHandledGPU) || (TRUE == m_rotationHandledFCV))
                            {
                                moduleInput.outFormat.width = m_rotatedDimensions[0];
                                moduleInput.outFormat.height = m_rotatedDimensions[1];
                            }
                            moduleInput.pOutYBufAddr = pImageBuffer->GetPlaneVirtualAddr(0, 0);
                            moduleInput.pOutCBCRBufAddr = pImageBuffer->GetPlaneVirtualAddr(0, 1);
                        }
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Output Port/Image is Null ");
                        result = CamxResultEInvalidArg;
                    }

                    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, " ModuleInput inputformat width %d height %d"
                                   "output format width %d height %d", moduleInput.inFormat.width,
                                   moduleInput.inFormat.height, moduleInput.outFormat.width,
                                   moduleInput.outFormat.height);

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Output Port: Add IO config failed rc %d", result);
                        break;
                    }
                }
                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "Output Port: Add IO config failed");
                    break;
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            if (TRUE == m_bThumbnailEncode)
            {
                moduleInput.scaleCropReq.outputWidth   = m_curThumbnailDim.width;
                moduleInput.scaleCropReq.outputHeight  = m_curThumbnailDim.height;

                moduleInput.scaleCropReq.inputWidth    = moduleInput.inFormat.width;
                moduleInput.scaleCropReq.inputHeight   = moduleInput.inFormat.height;
                moduleInput.scaleCropReq.bCropEnabled  = TRUE;
                moduleInput.scaleCropReq.hOffset       = 0;
                moduleInput.scaleCropReq.vOffset       = 0;
                moduleInput.scaleCropReq.cropWidth     = moduleInput.scaleCropReq.inputWidth;
                moduleInput.scaleCropReq.cropHeight    = moduleInput.scaleCropReq.inputHeight;
                moduleInput.scaleCropReq.bScaleEnabled = TRUE;

                /* Adjust crop for aspect ratio. If the thumbnail crop aspect ratio image and thumbnail dest aspect
                ratio are different, reset the thumbnail crop */
                DOUBLE cropAspectRatio = static_cast<DOUBLE>(moduleInput.scaleCropReq.cropWidth) /
                    static_cast<DOUBLE>(moduleInput.scaleCropReq.cropHeight);
                DOUBLE outputAspectRatio = static_cast<DOUBLE>(moduleInput.scaleCropReq.outputWidth) /
                    static_cast<DOUBLE>(moduleInput.scaleCropReq.outputHeight);

                CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "outputAspectRatio:%f, cropAspectRatio:%f",
                outputAspectRatio, cropAspectRatio);

                if ((outputAspectRatio - cropAspectRatio) > MaxAspectRatioDiffTolerence)
                {
                    UpdateCropForOutAspectRatio(&moduleInput.scaleCropReq, FALSE);
                }
                else if ((cropAspectRatio - outputAspectRatio) > MaxAspectRatioDiffTolerence)
                {
                    UpdateCropForOutAspectRatio(&moduleInput.scaleCropReq, TRUE);
                }
            }
            else
            {
                moduleInput.scaleCropReq.outputWidth   = moduleInput.outFormat.width;
                moduleInput.scaleCropReq.outputHeight  = moduleInput.outFormat.height;

                moduleInput.scaleCropReq.inputWidth    = moduleInput.inFormat.width;
                moduleInput.scaleCropReq.inputHeight   = moduleInput.inFormat.height;
                moduleInput.scaleCropReq.bCropEnabled  = FALSE;
                moduleInput.scaleCropReq.hOffset       = 0;
                moduleInput.scaleCropReq.vOffset       = 0;
                moduleInput.scaleCropReq.cropWidth     = moduleInput.scaleCropReq.inputWidth;
                moduleInput.scaleCropReq.cropHeight    = moduleInput.scaleCropReq.inputHeight;
                moduleInput.scaleCropReq.bScaleEnabled = TRUE;
            }
        }

        if (CamxResultSuccess == result)
        {
            UINT32 metaTag           = 0;
            UINT   jpegTag[4]        = { 0 };
            VOID*  pData[4]          = { 0 };
            UINT   length            = CAMX_ARRAY_SIZE(jpegTag);
            UINT64 jpegDataOffset[4] = { 0 };

            result = VendorTagManager::QueryVendorTagLocation(QuantizationTableVendorTagSection,
                                                              QuantizationTableLumaVendorTagName,
                                                              &metaTag);
            CAMX_ASSERT(CamxResultSuccess == result);
            jpegTag[0] = (metaTag | InputMetadataSectionMask);

            result = VendorTagManager::QueryVendorTagLocation(QuantizationTableVendorTagSection,
                                                              QuantizationTableChromaVendorTagName,
                                                              &metaTag);
            jpegTag[1] = (metaTag | InputMetadataSectionMask);
            CAMX_ASSERT(CamxResultSuccess == result);

            jpegTag[2] = InputJPEGQuality;

            jpegTag[3] = InputJPEGThumbnailQuality;

            result = GetDataList(jpegTag, pData, jpegDataOffset, length);

            CAMX_ASSERT(NULL != m_pQuantTables);

            if (NULL != pData[0])
            {
                m_pQuantTables[static_cast<INT>(QuantTableType::QuantTableLuma)].SetTable(
                    reinterpret_cast<UINT16*>(pData[0]));
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupJPEG, "Chroma quant table not ready, using previous table");
            }

            if (NULL != pData[1])
            {
                m_pQuantTables[static_cast<INT>(QuantTableType::QuantTableChroma)].SetTable(
                    reinterpret_cast<UINT16*>(pData[1]));
            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupJPEG, "Luma quant table not ready, using previous table");
            }

            if (CamxResultSuccess == result)
            {
                UINT32 quality = DefaultQuality;
                if (TRUE == m_bThumbnailEncode)
                {
                    if (NULL != pData[3])
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Thumbnail Quality received %d", *reinterpret_cast<BYTE*>(pData[3]));
                        quality = *reinterpret_cast<BYTE*>(pData[3]);
                        quality = (quality > MaxJpegThumbnailQuality) ? MaxJpegThumbnailQuality : quality; // Clamping to max
                    }
                }
                else
                {
                    if (NULL != pData[2])
                    {
                        CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Quality received %d", *reinterpret_cast<BYTE*>(pData[2]));
                        quality = *reinterpret_cast<BYTE*>(pData[2]);
                    }
                }

                if ((1 <= quality) && (100 >= quality))
                {
                    m_quality = quality;
                }

                JPEGUtil::UpdateQuantizationTableQuality(m_pQuantTables, m_quality);

            }
            else
            {
                CAMX_LOG_WARN(CamxLogGroupJPEG, "JPEGQuality not available, using previous setting");
            }

            moduleInput.pQuantTables = m_pQuantTables;

            result = FillJPEGCmdBuffer(pSettingsCmdBuffer, (&moduleInput));
        }

        if (CamxResultSuccess == result)
        {
            if (MaxJPEGInput != pPerRequestPorts->numInputPorts)
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "unexpected number of input ports %d", pPerRequestPorts->numInputPorts);
            }

            for (UINT i = 0; i < pPerRequestPorts->numInputPorts; i++)
            {
                PerRequestInputPortInfo* pInputPort = &pPerRequestPorts->pInputPorts[i];

                if ((NULL != pInputPort) && (NULL != pInputPort->pImageBuffer))
                {
                    SIZE_T          offset       = 0;
                    SIZE_T          metadataSize = 0;
                    CSLMemHandle    hMems[CSLMaxNumPlanes];

                    for (UINT pi = 0; pi < pInputPort->pImageBuffer->GetNumberOfPlanes(); pi++)
                    {
                        result = pInputPort->pImageBuffer->GetPlaneCSLMemHandle(0,
                                                                                pi,
                                                                                &hMems[pi],
                                                                                &offset,
                                                                                &metadataSize);
                        if (CamxResultSuccess == result)
                        {
                            if (0 == pi)
                            {
                                result = pSettingsCmdBuffer->AddNestedBufferInfo(moduleInput.inYBufOffset,
                                                                                 hMems[pi],
                                                                                 static_cast<UINT32>(offset));
                            }

                            if (1 == pi)
                            {
                                result = pSettingsCmdBuffer->AddNestedBufferInfo(moduleInput.inCBCRBufOffset,
                                                                                 hMems[pi],
                                                                                 static_cast<UINT32>(offset));
                            }

                            if (2 == pi)
                            {
                                result = pSettingsCmdBuffer->AddNestedBufferInfo(moduleInput.inPLN2BufOffset,
                                                                                 hMems[pi],
                                                                                 static_cast<UINT32>(offset));
                            }
                        }
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "Input Port/Image Buffer is Null ");
                    result = CamxResultEInvalidArg;
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "Input Port: Add Nested Buffer Info Failed i %d", i);
                    break;
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            if (MaxJPEGOutput != pPerRequestPorts->numOutputPorts)
            {
                CAMX_LOG_ERROR(CamxLogGroupJPEG, "unexpected number of output ports %d", pPerRequestPorts->numOutputPorts);
            }

            for (UINT portIndex = 0; portIndex < pPerRequestPorts->numOutputPorts; portIndex++)
            {
                PerRequestOutputPortInfo* pOutputPort = &pPerRequestPorts->pOutputPorts[portIndex];

                CAMX_ASSERT(NULL != pOutputPort);

                for (UINT bufferIndex = 0; bufferIndex < pOutputPort->numOutputBuffers; bufferIndex++)
                {
                    ImageBuffer* pImageBuffer = pOutputPort->ppImageBuffer[bufferIndex];

                    if (NULL != pImageBuffer)
                    {
                        SIZE_T          offset       = 0;
                        SIZE_T          metadataSize = 0;
                        CSLMemHandle    hMems[CSLMaxNumPlanes];

                        for (UINT i = 0; i < pImageBuffer->GetNumberOfPlanes(); i++)
                        {
                            result = pImageBuffer->GetPlaneCSLMemHandle(0,
                                                                        i,
                                                                        &hMems[i],
                                                                        &offset,
                                                                        &metadataSize);

                            if (CamxResultSuccess == result)
                            {
                                if (0 == i)
                                {
                                    result = pSettingsCmdBuffer->AddNestedBufferInfo(moduleInput.outYBufOffset,
                                                                                     hMems[i],
                                                                                     static_cast<UINT32>(offset));
                                }

                                if (1 == i)
                                {
                                    result = pSettingsCmdBuffer->AddNestedBufferInfo(moduleInput.outCBCRBufOffset,
                                                                                     hMems[i],
                                                                                     static_cast<UINT32>(offset));
                                }

                                if (2 == i)
                                {
                                    result = pSettingsCmdBuffer->AddNestedBufferInfo(moduleInput.outPLN2BufOffset,
                                                                                     hMems[i],
                                                                                     static_cast<UINT32>(offset));
                                }
                            }
                        }

                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Output Port/Image is Null ");
                        result = CamxResultEInvalidArg;
                    }

                    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "JPEG Enc result = %d ", result);

                    if (CamxResultSuccess != result)
                    {
                        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Output Port: Add Nested buffer info failed rc %d", result);
                        break;
                    }
                }

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "Output Port: Add IO config failed");
                    break;
                }
            }
        }

        if (CamxResultSuccess == result)
        {
            result = pPacket->AddCmdBufferReference(pSettingsCmdBuffer, &cmdBufferIndex);
            CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "AddCmdBufferReference result %d", result);
        }

        if (CamxResultSuccess == result)
        {
            result = pPacket->AddCmdBufferReference(pInputParamsCmdBuffer, NULL);
            CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "AddCmdBufferReference result %d", result);
        }

        if (CamxResultSuccess == result)
        {
            result = pPacket->SetKMDCmdBufferIndex(cmdBufferIndex,
                (pSettingsCmdBuffer->GetResourceUsedDwords() * sizeof(UINT32)));
        }

        if (CamxResultSuccess == result)
        {
            result = pPacket->CommitPacket();
            CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "CommitPacket result %d", result);
        }

        if (CamxResultSuccess == result)
        {
            result = GetHwContext()->Submit(GetCSLSession(), m_hDevice, pPacket);
            CAMX_ASSERT_MESSAGE(CamxResultSuccess == result, "submit result %d", result);
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
                if (NULL != pInputInfo)
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG,
                        "JPEG Encoder FAILED result %d , output size %d",
                        result,
                        pInputInfo->output_size);
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupJPEG, "JPEG Encoder FAILED result %d", result);
                }
            }
        }

        if ((CamxResultSuccess == result) && (0 < fenceCount))
        {
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency  = TRUE;
            pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount                     = fenceCount;
            pNodeRequestData->dependencyInfo[0].processSequenceId                               = 2;
            pNodeRequestData->numDependencyLists++;
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncNode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
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
        // The input buffer requirement will be the super resolution after looping through all the output ports.
        // The super resolution may have different aspect ratio compared to the original output port aspect ratio, but
        // this will be taken care of by the crop hardware associated with the output port.
        for (UINT outputIndex = 0; outputIndex < pBufferNegotiationData->numOutputPortsNotified; outputIndex++)
        {
            OutputPortNegotiationData* pOutputPortNegotiationData =
                &pBufferNegotiationData->pOutputPortNegotiationData[outputIndex];

            perOutputPortOptimalWidth  = 0;
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

            // Store the buffer requirements for this output port which will be reused to set, during forward walk.
            // The values stored here could be final output dimensions unless it is overridden by forward walk.
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth   = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight  = perOutputPortOptimalHeight;
            ///@ todo (CAMX-346) Fill the minimum and maximum based on scaler capabilities.
            pOutputPortNegotiationData->outputBufferRequirementOptions.minWidth   = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxWidth   = perOutputPortOptimalWidth;
            pOutputPortNegotiationData->outputBufferRequirementOptions.minHeight  = perOutputPortOptimalHeight;
            pOutputPortNegotiationData->outputBufferRequirementOptions.maxHeight  = perOutputPortOptimalHeight;

            CAMX_LOG_INFO(CamxLogGroupJPEG,
                          "JPEG Enc Optimal Output Dim, W:%d x H:%d!\n",
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

        if ((0 == optimalInputWidth) || (0 == optimalInputHeight))
        {
            result = CamxResultEFailed;

            CAMX_LOG_ERROR(CamxLogGroupJPEG,
                            "JPEG Enc Buffer Negotiation Failed, W:%d x H:%d!\n",
                            optimalInputWidth,
                            optimalInputHeight);
        }
        else
        {
            if (TRUE == this->m_bThumbnailEncode)
            {
                CamxDimension   optimalDimension        = { 0 };
                const PlatformStaticCaps*   pStaticCaps = NULL;
                UINT32 optimalthumbnailwidth            = 0;
                UINT32 optimalthumbnailheight           = 0;

                pStaticCaps = HwEnvironment::GetInstance()->GetPlatformStaticCaps();
                if ((NULL != pStaticCaps) && (0 < pStaticCaps->numJPEGThumbnailSizes))
                {
                    FLOAT referenceAspectRatio = static_cast<FLOAT>(optimalInputWidth) / static_cast<FLOAT>(optimalInputHeight);
                    FLOAT currentAspectRatio   = 0.0f;
                    BOOL aspectRatioMatch      = FALSE;

                    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "numJPEGThumbnailSizes %d ref ratio %f",
                        pStaticCaps->numJPEGThumbnailSizes, referenceAspectRatio);

                    for (UINT ti = (pStaticCaps->numJPEGThumbnailSizes) -1; ti > 0; ti--)
                    {
                        currentAspectRatio   = static_cast<FLOAT>(pStaticCaps->JPEGThumbnailSizes[ti].width) /
                                static_cast<FLOAT>(pStaticCaps->JPEGThumbnailSizes[ti].height);

                        if (Utils::FEqualCoarse(referenceAspectRatio, currentAspectRatio))
                        {
                            optimalthumbnailwidth  = pStaticCaps->JPEGThumbnailSizes[ti].width;
                            optimalthumbnailheight = pStaticCaps->JPEGThumbnailSizes[ti].height;
                            aspectRatioMatch       = TRUE;
                            CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Matched Thumbnail - width: %d, height: %d",
                            optimalthumbnailwidth, optimalthumbnailheight);
                            break;
                        }
                    }
                    if (FALSE == aspectRatioMatch)
                    {
                        CAMX_LOG_INFO(CamxLogGroupJPEG, "Aspect ratio for thumbnail is not matched "
                                      "referenceAspectRatio: %f, currentAspectRatio: %f",
                                      referenceAspectRatio, currentAspectRatio);

                        optimalthumbnailwidth  = m_thumbnailWidthMax;
                        optimalthumbnailheight = m_thumbnailHeightMax;
                    }
                }

                optimalDimension.width  = optimalthumbnailwidth  * OptimalThumbnailScalingFactor;
                optimalDimension.height = optimalthumbnailheight * OptimalThumbnailScalingFactor;

                optimalDimension.width  = Utils::EvenFloorUINT32(optimalDimension.width);
                optimalDimension.height = Utils::EvenFloorUINT32(optimalDimension.height);

                if ((optimalInputWidth  > optimalDimension.width) ||
                    (optimalInputHeight > optimalDimension.height))
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupJPEG, "Thumbnail Optimal input updated from %dx%d to %dx%d",
                                     optimalInputWidth, optimalInputHeight, optimalDimension.width, optimalDimension.height);

                    optimalInputWidth    = optimalDimension.width;
                    optimalInputHeight   = optimalDimension.height;
                    m_bPrevModuleScaleOn = TRUE;
                }
            }

            CAMX_LOG_INFO(CamxLogGroupJPEG, "JPEG Enc Optimal Input Dim, W:%d x H:%d", optimalInputWidth, optimalInputHeight);

            UINT32 numInputPorts = 0;
            UINT32 inputPortId[MaxJPEGInput];

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

                pInputBufferRequirement->optimalWidth  = optimalInputWidth;
                pInputBufferRequirement->optimalHeight = optimalInputHeight;
                // If IPE is enabling SIMO and if one of the output is smaller than the other, then the scale capabilities
                // (min,max) needs to be adjusted after accounting for the scaling needed on the smaller output port.
                ///@ todo (CAMX-346) Fill the minimum and maximum based on scaler capabilities.
                pInputBufferRequirement->minWidth  = optimalInputWidth;
                pInputBufferRequirement->maxWidth  = optimalInputWidth;
                pInputBufferRequirement->minHeight = optimalInputHeight;
                pInputBufferRequirement->maxHeight = optimalInputHeight;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncNode::FinalizeBufferProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEncNode::FinalizeBufferProperties(
    BufferNegotiationData* pBufferNegotiationData)
{
    UINT               numInputPort;
    UINT               inputPortId[MaxJPEGInput];
    const ImageFormat* pImageFormat = NULL;

    CAMX_ASSERT(NULL != pBufferNegotiationData);

    // Get Input Port List
    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    // Loop through input ports to get IPEInputPortFull
    for (UINT index = 0; index < numInputPort; index++)
    {
        if (JPEGInputPort0 == pBufferNegotiationData->pInputPortNegotiationData[index].inputPortId)
        {
            pImageFormat = pBufferNegotiationData->pInputPortNegotiationData[index].pImageFormat;
            break;
        }
    }

    CAMX_ASSERT(NULL != pImageFormat);

    for (UINT index = 0; index < pBufferNegotiationData->numOutputPortsNotified; index++)
    {
        OutputPortNegotiationData* pOutputPortNegotiationData   = &pBufferNegotiationData->pOutputPortNegotiationData[index];
        BufferProperties*          pFinalOutputBufferProperties = pOutputPortNegotiationData->pFinalOutputBufferProperties;

        if ((FALSE == IsSinkPortWithBuffer(pOutputPortNegotiationData->outputPortIndex)) &&
            (FALSE == IsNonSinkHALBufferOutput(pOutputPortNegotiationData->outputPortIndex)))
        {
            UINT outputPortId = GetOutputPortId(pOutputPortNegotiationData->outputPortIndex);
            switch (outputPortId)
            {
                case JPEGOutputPort0:
                    if (TRUE == m_bThumbnailEncode)
                    {
                        pFinalOutputBufferProperties->imageFormat.width  = m_thumbnailWidthMax;
                        pFinalOutputBufferProperties->imageFormat.height = m_thumbnailHeightMax;
                    }
                    else
                    {
                        pFinalOutputBufferProperties->imageFormat.width =
                            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalWidth;
                        pFinalOutputBufferProperties->imageFormat.height =
                            pOutputPortNegotiationData->outputBufferRequirementOptions.optimalHeight;
                    }

                    CAMX_LOG_VERBOSE(CamxLogGroupJPEG,
                                     "JPEG Enc FinalizeBuffer Thumb? %d Optimal Output Dim, W:%d x H:%d!\n",
                                     m_bThumbnailEncode,
                                     pFinalOutputBufferProperties->imageFormat.width,
                                     pFinalOutputBufferProperties->imageFormat.height);
                    break;
                default:
                    break;
            }
        }
    }
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::AcquireDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::AcquireDevice()
{
    CamxResult                 result                = CamxResultSuccess;
    CSLJPEGAcquireDeviceInfo*  pDevInfo              = NULL;
    CSLDeviceResource          deviceResourceRequest = { 0 };

    pDevInfo = static_cast<CSLJPEGAcquireDeviceInfo*>(CAMX_CALLOC(sizeof(CSLJPEGAcquireDeviceInfo)));
    if (NULL == pDevInfo)
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Allocation failed %p", pDevInfo);
        result = CamxResultENoMemory;
    }

    if (CamxResultSuccess == result)
    {
        result = SetupDeviceResource(pDevInfo, &deviceResourceRequest);
    }

    if (CamxResultSuccess == result)
    {
        result = CSLAcquireDevice(GetCSLSession(),
                                  &m_hDevice,
                                  DeviceIndices()[0],
                                  &deviceResourceRequest,
                                  1,
                                  NULL,
                                  0,
                                  NodeIdentifierString());
    }

    if (NULL != pDevInfo)
    {
        CAMX_FREE(pDevInfo);
        pDevInfo = NULL;
    }

    if (CamxResultSuccess == result)
    {
        SetDeviceAcquired(TRUE);
        AddCSLDeviceHandle(m_hDevice);
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupJPEG, "Acquire JPEG Device Failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::ReleaseDevice
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::ReleaseDevice()
{
    CamxResult result = CamxResultSuccess;

    if ((NULL != GetHwContext()) && (0 != m_hDevice))
    {
        result = CSLReleaseDevice(GetCSLSession(), m_hDevice);

        if (CamxResultSuccess == result)
        {
            SetDeviceAcquired(FALSE);
        }
        else
        {
            CAMX_ASSERT_ALWAYS_MESSAGE("Failed to release device");
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::FillJPEGCmdBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::FillJPEGCmdBuffer(
    CmdBuffer*     pCmdBuf,
    JPEGInputData* pModuleInput)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT(NULL != pCmdBuf);
    CAMX_ASSERT(NULL != pModuleInput);

    pModuleInput->pHwContext = GetHwContext();
    pModuleInput->pCmdBuffer = pCmdBuf;

    result = m_pJPEGEncConfigMod->Execute(pCmdBuf, pModuleInput);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::JPEGEncNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGEncNode::JPEGEncNode()
    : m_quality(DefaultQuality)
{
    m_pNodeName = "JPEG_Encoder";
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::~JPEGEncNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
JPEGEncNode::~JPEGEncNode()
{
    if (NULL != m_pJPEGEncConfigMod)
    {
        m_pJPEGEncConfigMod->Destroy();
        m_pJPEGEncConfigMod = NULL;
    }

    if (NULL != m_pQuantTables)
    {
        CAMX_DELETE [] m_pQuantTables;
        m_pQuantTables = NULL;
    }

    if (TRUE == IsDeviceAcquired())
    {
        ReleaseDevice();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::SetupDeviceResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::SetupDeviceResource(
    CSLJPEGAcquireDeviceInfo* pJPEGResource,
    CSLDeviceResource*        pResource)
{
    CamxResult result = CamxResultSuccess;

    CAMX_ASSERT_MESSAGE(NULL != pResource, "Pointer device resource NULL");
    CAMX_ASSERT_MESSAGE(NULL != pJPEGResource, "Pointer JPEG resource NULL");

    pJPEGResource->resourceType = CSLJPEGDeviceTypeEncoder;

    // Add to the resource list
    pResource[0].resourceID = CSLJPEGResourceIDEncoder;
    pResource[0].pDeviceResourceParam = static_cast<VOID*>(pJPEGResource);
    pResource[0].deviceResourceParamSize = sizeof(CSLJPEGAcquireDeviceInfo);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::SetDeviceIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void JPEGEncNode::SetDeviceIndex(
    INT32 idx)
{
    m_deviceIndex = idx;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::SetCmdBufResourceSize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void JPEGEncNode::SetCmdBufResourceSize(
    UINT size)
{
    m_cmdBufResourceSize = size;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::SetPropertyDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEncNode::SetPropertyDependency(
    NodeProcessRequestData*  pNodeRequestData)
{
    UINT32          count           = 0;
    DependencyUnit* pDependencyUnit = &pNodeRequestData->dependencyInfo[0];

    if (m_rotationHandledGPU == TRUE)
    {
        UINT32 metaTag = 0;

        VendorTagManager::QueryVendorTagLocation("com.qti.node.gpu", "FrameDimension", &metaTag);

        pDependencyUnit->dependencyFlags.hasPropertyDependency              = TRUE;
        pDependencyUnit->dependencyFlags.hasIOBufferAvailabilityDependency  = TRUE;
        pDependencyUnit->processSequenceId                                  = 1;
        pDependencyUnit->propertyDependency.properties[count]               = metaTag;
        count++;
    }
    else if (m_rotationHandledFCV == TRUE)
    {
        UINT32 metaTag = 0;

        VendorTagManager::QueryVendorTagLocation("com.qti.node.fcv", "FrameDimension", &metaTag);

        pDependencyUnit->dependencyFlags.hasPropertyDependency             = TRUE;
        pDependencyUnit->dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
        pDependencyUnit->processSequenceId                                 = 1;
        pDependencyUnit->propertyDependency.properties[count]              = metaTag;
        count++;
    }

    pDependencyUnit->propertyDependency.count = count;
    if (0 != pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask)
    {
        pNodeRequestData->numDependencyLists = 1;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// JPEGEncNode::SetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID JPEGEncNode::SetDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData)
{
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;

    // Initialize number of dependency lists to 0
    pNodeRequestData->numDependencyLists = 0;

    if (TRUE == GetStaticSettings()->enableImageBufferLateBinding)
    {
        // If latebinding is enabled, we want to delay packet preparation as late as possible, in other terms, we want to
        // prepare and submit to hw when it can really start processing. This is once all the input fences (+ property)
        // dependencies are satisfied. So, lets set input fence dependencies
        UINT fenceCount = SetInputBuffersReadyDependency(pExecuteProcessRequestData, 0);

        if (0 < fenceCount)
        {
            pNodeRequestData->dependencyInfo[0].processSequenceId                                   = 1;
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;

            pNodeRequestData->numDependencyLists = 1;
        }
    }

    // Set the property dependencies are satisfied
    SetPropertyDependency(pNodeRequestData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// JPEGEncNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult JPEGEncNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    const UINT32*   pJPEGOutputTags = NULL;
    UINT32          numJpegTags     = 0;

    if (TRUE == m_bThumbnailEncode)
    {
        pJPEGOutputTags = JPEGEncThumbnailOutputTags;
        numJpegTags     = CAMX_ARRAY_SIZE(JPEGEncThumbnailOutputTags);
    }
    else
    {
        pJPEGOutputTags = JPEGEncMainOutputTags;
        numJpegTags     = CAMX_ARRAY_SIZE(JPEGEncMainOutputTags);
    }

    for (UINT32 tagIndex = 0; tagIndex < numJpegTags; ++tagIndex)
    {
        pPublistTagList->tagArray[tagIndex] = pJPEGOutputTags[tagIndex];
    }

    pPublistTagList->tagCount = numJpegTags;
    CAMX_LOG_VERBOSE(CamxLogGroupMeta, "bThumbnailEncode %d: %u tags will be published",
                     m_bThumbnailEncode,
                     numJpegTags);

    return CamxResultSuccess;
}

CAMX_NAMESPACE_END
