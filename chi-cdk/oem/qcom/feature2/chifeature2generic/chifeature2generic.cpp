////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2generic.cpp
/// @brief Simple CHX feature2 implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2generic.h"
#include "chifeature2base.h"
#include "chifeature2utils.h"
#include "chifeature2featurepool.h"

// NOWHINE FILE CP006: Need whiner update: std::vector allowed in exceptional cases

/// @brief Generic min buffer count to set in Meta TBM
static const UINT32 FEATURE2_GENERIC_MIN_METADATA_BUFFER_COUNT = 1;
static const UINT32 Feature2MajorVersion                       = 0;
static const UINT32 Feature2MinorVersion                       = 1;
static const CHAR* VendorName                                  = "QTI";

// NOWHINE NC003a: This is a constant
static constexpr UINT8 SNAPSHOT_OUTPUT_PORT_ID_GPU    = 4;
// NOWHINE NC003a: This is a constant
static constexpr UINT8 SNAPSHOT_INPUT_PORT_ID_YUV     = 0;
// NOWHINE NC003a: This is a constant
static constexpr UINT8 SNAPSHOT_INPUT_PORT_ID_YUV_GPU = 2;
// NOWHINE NC003a: This is a constant
static constexpr UINT8 B2Y_OUTPUT_PORT_ID_YUV         = 0;
// NOWHINE NC003a: This is a constant
static constexpr UINT8 B2Y_OUTPUT_PORT_ID_YUV_GPU     = 2;

static std::vector<const CHAR*>  Feature2GenericJPEGCaps =
{
    "Bayer2Yuv",
    "JPEG",
    "JPEGGPU",
    "FormatConvertor"
};

static std::vector<const CHAR*>  Feature2GenericYUVCaps =
{
    "Bayer2Yuv",
    "FormatConvertor"
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Generic* ChiFeature2Generic::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2Generic* pFeature    = CHX_NEW(ChiFeature2Generic);
    CDKResult           result      = CDKResultSuccess;

    if (NULL == pFeature)
    {
        CHX_LOG_ERROR("Out of memory: pFeature is NULL");
    }
    else
    {
        result = pFeature->Initialize(pCreateInputInfo);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Feature failed to initialize!!");
            pFeature->Destroy();
            pFeature = NULL;
        }
        else
        {
            pFeature->m_disableZoomCrop = pCreateInputInfo->bDisableZoomCrop;
        }
    }

    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Generic::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::~ChiFeature2Generic
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Generic::~ChiFeature2Generic()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::DoFlush()
{
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::OnPrepareRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::OnPopulateConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::OnPopulateConfiguration(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult             result = CDKResultSuccess;
    ChiFeature2StageInfo  stageInfo = { 0 };
    UINT8                 requestId = 0;

    UINT8                                numRequestOutputs = 0;
    const ChiFeature2RequestOutputInfo*  pRequestOutputInfo = NULL;

    if (!CdkUtils::StrCmp(pRequestObject->GetFeature()->GetFeatureName(), "JPEGGPU"))
    {
        BOOL isGPURotationNeeded = pRequestObject->GetUsecaseRequestObject()->GetGPURotationFlag();

        requestId = pRequestObject->GetCurRequestId();
        pRequestObject->GetCurrentStageInfo(&stageInfo, requestId);

        if (InvalidStageId == stageInfo.stageId)
        {
            result = CDKResultEInvalidArg;
        }

        if (CDKResultSuccess == result)
        {
            ChiFeature2PortIdList inputPortList  = { 0 };
            ChiFeature2PortIdList outputPortList = { 0 };

            result = GetInputPortsForStage(stageInfo.stageId, &inputPortList);

            if (CDKResultSuccess == result)
            {
                result = GetOutputPortsForStage(stageInfo.stageId, &outputPortList);
            }

            if ((CDKResultSuccess == result) && (inputPortList.numPorts > 1))
            {
                std::vector<ChiFeature2Identifier> filteredOutputList;
                for (UINT8 portIndex = 0; portIndex < outputPortList.numPorts; portIndex++)
                {
                    ChiFeature2Identifier portId = outputPortList.pPorts[portIndex];

                    if (ChiFeature2PortType::MetaData == portId.portType)
                    {
                        filteredOutputList.push_back(portId);
                    }
                    else
                    {
                        // first subpipeline ports
                        if ((FALSE == isGPURotationNeeded) && (portId.port < SNAPSHOT_OUTPUT_PORT_ID_GPU))
                        {
                            filteredOutputList.push_back(portId);
                        }
                        // second subpipeline ports
                        else if ((TRUE == isGPURotationNeeded) && (portId.port >= SNAPSHOT_OUTPUT_PORT_ID_GPU))
                        {
                            filteredOutputList.push_back(portId);
                        }
                    }
                }

                std::vector<ChiFeature2Identifier> filteredInputList;
                for (UINT8 portIndex = 0; portIndex < inputPortList.numPorts; portIndex++)
                {
                    ChiFeature2Identifier portId = inputPortList.pPorts[portIndex];

                    if (ChiFeature2PortType::MetaData == portId.portType)
                    {
                        filteredInputList.push_back(portId);
                    }
                    else
                    {
                        // first subpipeline ports
                        if ((FALSE == isGPURotationNeeded) && (SNAPSHOT_INPUT_PORT_ID_YUV == portId.port))
                        {
                            filteredInputList.push_back(portId);
                        }
                        // second subpipeline ports
                        else if ((TRUE == isGPURotationNeeded) && (SNAPSHOT_INPUT_PORT_ID_YUV_GPU == portId.port))
                        {
                            filteredInputList.push_back(portId);
                        }
                    }
                }

                ChiFeature2PortIdList filteredOutput = { 0 };

                filteredOutput.numPorts = filteredOutputList.size();
                filteredOutput.pPorts   = filteredOutputList.data();

                ChiFeature2PortIdList filteredInput = { 0 };

                filteredInput.numPorts = filteredInputList.size();
                filteredInput.pPorts   = filteredInputList.data();

                for (UINT i = 0; i < filteredInputList.size(); i++)
                {
                    CHX_LOG_VERBOSE("input port {%d, %d, %d}", filteredInputList[i].session,
                                    filteredInputList[i].pipeline, filteredInputList[i].port);
                }

                for (UINT i = 0; i < filteredOutputList.size(); i++)
                {
                    CHX_LOG_VERBOSE("output port {%d, %d, %d}", filteredOutputList[i].session,
                                    filteredOutputList[i].pipeline, filteredOutputList[i].port);
                }

                result = PopulatePortConfiguration(pRequestObject,
                                                   &filteredInput, &filteredOutput);
            }
        }
    }
    else if (static_cast<UINT32>(ChiFeature2Type::B2Y) == pRequestObject->GetFeature()->GetFeatureId())
    {
        BOOL isGPURotationNeeded = pRequestObject->GetUsecaseRequestObject()->GetGPURotationFlag();

        requestId = pRequestObject->GetCurRequestId();
        pRequestObject->GetCurrentStageInfo(&stageInfo, requestId);

        if (InvalidStageId == stageInfo.stageId)
        {
            result = CDKResultEInvalidArg;
        }

        if (CDKResultSuccess == result)
        {
            ChiFeature2PortIdList inputPortList  = { 0 };
            ChiFeature2PortIdList outputPortList = { 0 };

            result = GetInputPortsForStage(stageInfo.stageId, &inputPortList);

            if (CDKResultSuccess == result)
            {
                result = GetOutputPortsForStage(stageInfo.stageId, &outputPortList);
            }

            if ((CDKResultSuccess == result) && (outputPortList.numPorts > 1))
            {
                std::vector<ChiFeature2Identifier> filteredOutputList;
                for (UINT8 portIndex = 0; portIndex < outputPortList.numPorts; portIndex++)
                {
                    ChiFeature2Identifier portId = outputPortList.pPorts[portIndex];

                    if (ChiFeature2PortType::MetaData == portId.portType)
                    {
                        filteredOutputList.push_back(portId);
                    }
                    else
                    {
                        if ((FALSE == isGPURotationNeeded) && (B2Y_OUTPUT_PORT_ID_YUV == portId.port))
                        {
                            filteredOutputList.push_back(portId);
                        }
                        else if ((TRUE == isGPURotationNeeded) && (B2Y_OUTPUT_PORT_ID_YUV_GPU == portId.port))
                        {
                            filteredOutputList.push_back(portId);
                        }
                    }
                }

                ChiFeature2PortIdList filteredOutput = { 0 };

                filteredOutput.numPorts = filteredOutputList.size();
                filteredOutput.pPorts   = filteredOutputList.data();

                for (UINT i = 0; i < filteredOutputList.size(); i++)
                {
                    CHX_LOG_VERBOSE("output port {%d, %d, %d}", filteredOutputList[i].session,
                                    filteredOutputList[i].pipeline, filteredOutputList[i].port);
                }

                result = PopulatePortConfiguration(pRequestObject,
                                                   &inputPortList, &filteredOutput);
            }
        }
    }
    else
    {
        ChiFeature2Base::OnPopulateConfiguration(pRequestObject);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::OnPopulateDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::OnPopulateDependency(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult             result = CDKResultSuccess;
    ChiFeature2StageInfo  stageInfo = { 0 };
    UINT8                 requestId = 0;

    if (static_cast<UINT32>(ChiFeature2Type::JPEG) == pRequestObject->GetFeature()->GetFeatureId())
    {
        UINT8                                numRequestOutputs   = 0;
        const ChiFeature2RequestOutputInfo*  pRequestOutputInfo  = NULL;
        const ChiFeature2StageDescriptor*    pStageDescriptor    = NULL;
        BOOL                                 isGPURotationNeeded =
            pRequestObject->GetUsecaseRequestObject()->GetGPURotationFlag();

        requestId = pRequestObject->GetCurRequestId();
        pRequestObject->GetNextStageInfo(&stageInfo, requestId);
        pRequestObject->GetExternalRequestOutput(&numRequestOutputs, &pRequestOutputInfo, requestId);

        if (InvalidStageId != stageInfo.stageId)
        {
            pStageDescriptor = GetStageDescriptor(stageInfo.stageId);
        }

        if (NULL != pStageDescriptor)
        {
            const ChiFeature2InputDependency* pInputDependency = NULL;
            // filter input dependency based on GPU rotation
            if (FALSE == isGPURotationNeeded)
            {
                pInputDependency = GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 0);
            }
            else
            {
                pInputDependency = GetDependencyListFromStageDescriptor(pStageDescriptor, 0, 0, 1);
            }
            if (NULL != pInputDependency)
            {
                PopulateDependencyPorts(pRequestObject, 0, pInputDependency);
            }
        }
    }
    else
    {
        result = ChiFeature2Base::OnPopulateDependency(pRequestObject);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::OnSelectFlowToExecuteRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RequestFlowType ChiFeature2Generic::OnSelectFlowToExecuteRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNREFERENCED_PARAM(pRequestObject);

    return ChiFeature2RequestFlowType::Type0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::OnPortCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::OnPortCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    result = ChiFeature2Base::OnPortCreate(pKey);

    if (NULL != pKey)
    {
        ChiFeaturePortData* pPortData = GetPortData(pKey);

        if ((NULL != pPortData) && (ChiFeature2PortType::MetaData == pPortData->globalId.portType))
        {
            pPortData->minBufferCount = FEATURE2_GENERIC_MIN_METADATA_BUFFER_COUNT;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Key");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::OnPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::OnPipelineCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pKey)
    {
        ChiFeaturePipelineData* pPipelineData = GetPipelineData(pKey);
        if (NULL != pPipelineData)
        {
            pPipelineData->minMetaBufferCount  = FEATURE2_GENERIC_MIN_METADATA_BUFFER_COUNT;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid key");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::OnMetadataResult
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Generic::OnMetadataResult(
    ChiFeature2RequestObject*  pRequestObject,
    UINT8                      resultId,
    ChiFeature2StageInfo*      pStageInfo,
    ChiFeature2Identifier*     pPortIdentifier,
    ChiMetadata*               pMetadata,
    UINT32                     frameNumber,
    VOID*                      pPrivateData)
{
    BOOL                sendToGraph         = FALSE;
    ExtensionModule*    pExtensionModule    = ExtensionModule::GetInstance();

    if (DumpDebugDataPerSnapshot <= pExtensionModule->EnableDumpDebugData())
    {
        ProcessDebugData(pRequestObject,
                         pStageInfo,
                         pPortIdentifier,
                         pMetadata,
                         frameNumber);
    }

    sendToGraph = ChiFeature2Base::OnMetadataResult(pRequestObject,
                                                    resultId,
                                                    pStageInfo,
                                                    pPortIdentifier,
                                                    pMetadata,
                                                    frameNumber,
                                                    pPrivateData);

    return sendToGraph;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::OnPopulateDependencySettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::OnPopulateDependencySettings(
    ChiFeature2RequestObject*     pRequestObject,
    UINT8                         dependencyIndex,
    const ChiFeature2Identifier*  pSettingPortId,
    ChiMetadata*                  pFeatureSettings
    ) const
{
    CDK_UNREFERENCED_PARAM(dependencyIndex);
    CDKResult result = ChiFeature2Base::OnPopulateDependencySettings(
        pRequestObject, dependencyIndex, pSettingPortId, pFeatureSettings);

    // EXAMPLE CODE START

    /*
    // By doing below generic derived class change, we are setting,
    // "ANDROID_CONTROL_EFFECT_MODE_NEGATIVE" as dependency tag so
    // that it is used as input setting for buffers to be generated

    UINT32 mode = ANDROID_CONTROL_EFFECT_MODE_NEGATIVE;
    pFeatureSettings->SetTag(ANDROID_CONTROL_EFFECT_MODE, &mode, 1);


    // By doing below generic derived class change, we are setting,
    // "ZSLTimestampRange" as dependency tag so
    // all ZSL buffers captured between the last 60 to 120 ms will
    // be picked

    INT64 timeRange [] = { 60000000, 120000000};
    if (static_cast<UINT32>(ChiFeature2Type::B2Y) == GetFeatureId())
    {
        ChxUtils::SetVendorTagValue(pFeatureSettings, ZSLTimestampRange, 2, &timeRange);
    }
    */
    // EXAMPLE CODE END

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::OnPopulateConfigurationSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::OnPopulateConfigurationSettings(
    ChiFeature2RequestObject*     pRequestObject,
    const ChiFeature2Identifier*  pMetadataPortId,
    ChiMetadata*                  pInputMetadata
    ) const
{
    CDKResult result     = CDKResultSuccess;
    UINT8     requestId  = pRequestObject->GetCurRequestId();

    result = ChiFeature2Base::OnPopulateConfigurationSettings(
        pRequestObject, pMetadataPortId, pInputMetadata);

    // EXAMPLE CODE START

    /*
    // By doing below generic derived class change, we are setting,
    // "ANDROID_CONTROL_EFFECT_MODE_NEGATIVE" tag input metadata for pipeline

    UINT32 value = 1;
    pInputMetadata->SetTag(ANDROID_JPEG_ORIENTATION, &value, 1);

    */

    // EXAMPLE CODE END

    if (static_cast<UINT32>(ChiFeature2Type::B2Y) == pRequestObject->GetFeature()->GetFeatureId())
    {
        if (TRUE == m_disableZoomCrop)
        {
            BOOL bDisableZoomCrop = TRUE;
            result = pInputMetadata->SetTag("org.quic.camera2.ref.cropsize", "DisableZoomCrop",
                                            &bDisableZoomCrop, 1);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("Cannot set disable Zoom crop into metadata");
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Generic::OnPreparePipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::OnPreparePipelineCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult               result          = CDKResultSuccess;
    ChiFeaturePipelineData* pPipelineData   = GetPipelineData(pKey);

    // Use Session Metadata only for JPEG pipelines
    if ((NULL != pPipelineData) && (NULL != pPipelineData->pPipeline) &&
        ((0 == CdkUtils::StrCmp(pPipelineData->pPipelineName, "InternalZSLYuv2Jpeg")) ||
         (0 == CdkUtils::StrCmp(pPipelineData->pPipelineName, "InternalZSLYuv2JpegGPU"))))
    {
        ChiMetadata*    pMetadata                   = pPipelineData->pPipeline->GetDescriptorMetadata();
        const VOID*     pSessionSettings            = GetSessionSettings();
        UINT8*          pOverrideScaleValue         = NULL;
        UINT8*          pOverrideGPURotation        = NULL;
        UINT8           overrideScaleTag            = 0;
        UINT8           overrideGPURotation         = 0;

        if (NULL != pSessionSettings)
        {
            ChxUtils::AndroidMetadata::GetVendorTagValue(pSessionSettings, VendorTag::IPEOverrideScale,
                                                         reinterpret_cast<VOID**>(&pOverrideScaleValue));
            ChxUtils::AndroidMetadata::GetVendorTagValue(pSessionSettings, VendorTag::GPUOverrideRotation,
                                                         reinterpret_cast<VOID**>(&pOverrideGPURotation));

            if (NULL != pOverrideScaleValue)
            {
                overrideScaleTag = *pOverrideScaleValue;
            }

            if (NULL != pOverrideGPURotation)
            {
                overrideGPURotation = *pOverrideGPURotation;
            }

            CHX_LOG_VERBOSE("OverrideGPURotationUsecase %d, OverrideScaleProfile %d",
                            overrideGPURotation, overrideScaleTag);
        }
        else
        {
            CHX_LOG_VERBOSE("pSessionSettings is NULL");
        }

        if (NULL != pMetadata)
        {
            if (0 != overrideScaleTag)
            {
                result = pMetadata->SetTag("org.quic.camera.overrideIPEScaleProfile",
                                           "OverrideIPEScaleProfile", pOverrideScaleValue, 1);

                if (CDKResultSuccess != result)
                {
                    CHX_LOG_WARN("SetTag failed to set override scale");
                    result = CDKResultSuccess;
                }
            }

            if (0 != overrideGPURotation)
            {
                result = pMetadata->SetTag("org.quic.camera.overrideGPURotationUsecase",
                                           "OverrideGPURotationUsecase", pOverrideGPURotation, 1);

                if (CDKResultSuccess != result)
                {
                    CHX_LOG_WARN("SetTag failed to set override GPU rotation");
                    result = CDKResultSuccess;
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("pMetadata is NULL");
        }
    }
    else
    {
        CHX_LOG_WARN("pPipelineData %p or pipeline is NULL", pPipelineData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Generic::OnPruneUsecaseDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::OnPruneUsecaseDescriptor(
    const ChiFeature2CreateInputInfo* pCreateInputInfo,
    std::vector<PruneVariant>&        rPruneVariants
    ) const
{
    PruneVariant snapshotVariant;
    snapshotVariant.group = UsecaseSelector::GetVariantGroup("Snapshot");
    snapshotVariant.type  = UsecaseSelector::GetVariantType(pCreateInputInfo->bFrameworkHEICSnapshot ? "HEIC" : "JPEG");

    rPruneVariants.reserve(1);
    rPruneVariants.push_back(snapshotVariant);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Generic::OnPipelineSelect
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Generic::OnPipelineSelect(
    const CHAR*                         pPipelineName,
    const ChiFeature2CreateInputInfo*   pCreateInputInfo
    ) const
{
    BOOL isPipelineSupported = FALSE;

    if ((NULL == pPipelineName) ||
        (NULL == pCreateInputInfo) ||
        (NULL == pCreateInputInfo->pInstanceProps) ||
        (NULL == pCreateInputInfo->pStreamConfig) ||
        (NULL == pCreateInputInfo->pFeatureDescriptor))
    {
        CHX_LOG_ERROR("Invalid input");
    }
    else
    {
        if (!CdkUtils::StrCmp(pCreateInputInfo->pFeatureDescriptor->pFeatureName, "Bayer2Yuv"))
        {
            if (!CdkUtils::StrCmp(pPipelineName, "ZSLSnapshotYUVHAL"))
            {
                isPipelineSupported = TRUE;
            }
        }

        if (TRUE == isPipelineSupported)
        {
            CHX_LOG_INFO("featureName:%s, pipelineName:%s, instanceFlags {nzsl:%d, swremosaic:%d, hwremosaic:%d, bpscam:%d}",
                pCreateInputInfo->pFeatureDescriptor->pFeatureName,
                pPipelineName,
                pCreateInputInfo->pInstanceProps->instanceFlags.isNZSLSnapshot,
                pCreateInputInfo->pInstanceProps->instanceFlags.isSWRemosaicSnapshot,
                pCreateInputInfo->pInstanceProps->instanceFlags.isHWRemosaicSnapshot,
                pCreateInputInfo->pInstanceProps->instanceFlags.isBPSCamera);
        }
    }

    return isPipelineSupported;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2Generic* pFeatureGeneric = ChiFeature2Generic::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeatureGeneric);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoQueryCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DoQueryCaps(
    VOID*                  pConfig,
    ChiFeature2QueryInfo*  pQueryInfo)
{
    CDKResult                 result = CDKResultSuccess;
    std::vector<const CHAR*>* pCaps  = NULL;

    if (NULL != pConfig)
    {
        Feature2Config*      pFeature2Config = static_cast<Feature2Config*>(pConfig);
        SnapshotStreamConfig snapshotConfig  = {};

        UsecaseSelector::GetSnapshotStreamConfiguration(pFeature2Config->pCameraStreamConfig->numStreams,
                                                        pFeature2Config->pCameraStreamConfig->pChiStreams,
                                                        snapshotConfig);

        BOOL isMultiCamera = (pFeature2Config->pCameraInfo->numPhysicalCameras <= 1) ? FALSE : TRUE;

        CHX_LOG_INFO("SnapshotType: %u multicamera %d", snapshotConfig.type, isMultiCamera);

        BOOL YUVCaps = ((SnapshotStreamType::UNKNOWN == snapshotConfig.type) && (FALSE == isMultiCamera));
        pCaps = (TRUE == YUVCaps) ? &Feature2GenericYUVCaps : &Feature2GenericJPEGCaps;
    }

    if ((NULL != pQueryInfo) && (NULL != pCaps))
    {
        pQueryInfo->numCaps        = pCaps->size();
        pQueryInfo->ppCapabilities = pCaps->data();
    }
    else
    {
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GetVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID GetVendorTags(
    VOID* pVendorTags)
{
    CDK_UNUSED_PARAM(pVendorTags);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoStreamNegotiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DoStreamNegotiation(
    StreamNegotiationInfo*          pNegotiationInfo,
    StreamNegotiationOutput*        pNegotiationOutput)
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pNegotiationInfo) ||
        (NULL == pNegotiationInfo->pFeatureInstanceId) ||
        (pNegotiationOutput == NULL))
    {
        CHX_LOG_ERROR("Invalid Arg! pNegotiation=%p, pDesiredStreamConfig=%p",
            pNegotiationInfo, pNegotiationOutput);

        result = CDKResultEInvalidArg;
    }
    else
    {
        ChiFeature2Type featureId   = static_cast<ChiFeature2Type>(pNegotiationInfo->pFeatureInstanceId->featureId);
        const UINT   numStreams     = pNegotiationInfo->pFwkStreamConfig->numStreams;
        CHISTREAM**  ppChiStreams   = pNegotiationInfo->pFwkStreamConfig->pChiStreams;
        UINT8       physicalCamIdx  = pNegotiationInfo->pFeatureInstanceId->cameraId;
        UINT32      physicalCamId   = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[physicalCamIdx]->cameraId;
        BOOL        isRaw16Format   = FALSE;

        BOOL        isNativeResolution                 = FALSE;
        const ChiFeature2InstanceProps* pInstanceProps = pNegotiationInfo->pInstanceProps;

        if (NULL != pInstanceProps)
        {
            for (UINT i = 0; i < pInstanceProps->numFeatureProps; ++i)
            {
                CHX_LOG_INFO("[%d/%d], propId:%d, propName:%s, size:%d, value:%s", i,
                    pInstanceProps->numFeatureProps,
                    pInstanceProps->pFeatureProperties[i].propertyId,
                    pInstanceProps->pFeatureProperties[i].pPropertyName,
                    pInstanceProps->pFeatureProperties[i].size,
                    pInstanceProps->pFeatureProperties[i].pValue);

                if (!CdkUtils::StrCmp(pInstanceProps->pFeatureProperties[i].pPropertyName, "nativeinputresolution"))
                {
                    if (!CdkUtils::StrCmp(static_cast<const CHAR*>(pInstanceProps->pFeatureProperties[i].pValue), "TRUE"))
                    {
                        isNativeResolution = TRUE;
                        break;
                    }
                }
            }
        }

        GetSensorOutputDimension(physicalCamId,
                                 pNegotiationInfo->pFwkStreamConfig,
                                 pNegotiationInfo->pFeatureInstanceId->flags,
                                 &RDIStream.width,
                                 &RDIStream.height);

        for (UINT32 stream = 0; stream < numStreams; stream++)
        {
            if (UsecaseSelector::IsRawStream(reinterpret_cast<camera3_stream_t*>(ppChiStreams[stream])))
            {
                if (ppChiStreams[stream]->format == ChiStreamFormatRaw16)
                {
                    isRaw16Format = TRUE;
                }
            }
        }

        UINT32     snapshotWidth          = 0;
        UINT32     snapshotHeight         = 0;
        CHISTREAM* pBayer2YuvInputStream  = NULL;
        CHISTREAM* pBayer2YuvOutputStream = NULL;
        CHISTREAM* pBPSInputStream        = NULL;
        CHISTREAM* pBPSOutputStream       = NULL;
        CHISTREAM* pIPEInputStream        = NULL;
        CHISTREAM* pIPEOutputStream       = NULL;
        CHISTREAM* pJPEGInputStream       = NULL;

        // Clone a stream and put it into the list of streams that will be freed by the feature
        auto CloneStream = [&pNegotiationOutput](CHISTREAM* pSrcCameraStream) -> CHISTREAM* {
            ChiStream* pStream = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
            ChxUtils::Memcpy(pStream, pSrcCameraStream, sizeof(CHISTREAM));
            pNegotiationOutput->pOwnedStreams->push_back(pStream);
            return pStream;
        };

        SnapshotStreamConfig snapshotConfig = {};

        UsecaseSelector::GetSnapshotStreamConfiguration(numStreams, ppChiStreams, snapshotConfig);

        if ((CDKResultSuccess == result) && (NULL != snapshotConfig.pSnapshotStream))
        {
            snapshotWidth  = snapshotConfig.pSnapshotStream->width;
            snapshotHeight = snapshotConfig.pSnapshotStream->height;
        }
        else
        {
            CHX_LOG_INFO("Could not extract snapshot stream - Error: %u SnapshotType: %u", result, snapshotConfig.type);
        }

        pNegotiationOutput->pStreams->clear();

        switch(featureId)
        {
            case ChiFeature2Type::B2Y:
                if (pNegotiationInfo->pLogicalCameraInfo->numPhysicalCameras == 1)
                {
                    if (FALSE == pNegotiationInfo->pFeatureInstanceId->flags.isNZSLSnapshot)
                    {
                        pBayer2YuvInputStream = CloneStream(&Bayer2YuvStreamsInput);

                        if (NULL != pBayer2YuvInputStream)
                        {
                            if (TRUE == isRaw16Format)
                            {
                                pBayer2YuvInputStream->format = ChiStreamFormatRaw16;
                            }
                            // configure input stream
                            pBayer2YuvInputStream->width  = RDIStream.width;
                            pBayer2YuvInputStream->height = RDIStream.height;
                            pNegotiationOutput->pStreams->push_back(pBayer2YuvInputStream);
                        }
                    }
                    else
                    {
                        if ((NULL != pNegotiationInfo->pFeatureInstanceId) &&
                            (TRUE == pNegotiationInfo->pFeatureInstanceId->flags.isSWRemosaicSnapshot))
                        {
                            NZSLBayer2YUVStreamsInput.format = ChiStreamFormatRaw16;
                        }

                        pBayer2YuvInputStream = CloneStream(&NZSLBayer2YUVStreamsInput);

                        // get the max sensor output
                        UINT32 maxSensorWidth  = 0;
                        UINT32 maxSensorHeight = 0;
                        for (UINT i = 0; i < pNegotiationInfo->pLogicalCameraInfo->m_cameraCaps.numSensorModes; i++)
                        {
                            CHIRECT* pSensorDimension =
                                &(pNegotiationInfo->pLogicalCameraInfo->pSensorModeInfo[i].frameDimension);
                            if ((pSensorDimension->width  > maxSensorWidth) ||
                                (pSensorDimension->height > maxSensorHeight))
                            {
                                maxSensorWidth  = pSensorDimension->width;
                                maxSensorHeight = pSensorDimension->height;
                            }
                        }

                        if (NULL != pBayer2YuvInputStream)
                        {
                            if (TRUE == isRaw16Format)
                            {
                                pBayer2YuvInputStream->format = ChiStreamFormatRaw16;
                            }

                            // configure input stream
                            pBayer2YuvInputStream->width  = maxSensorWidth;
                            pBayer2YuvInputStream->height = maxSensorHeight;
                            pNegotiationOutput->pStreams->push_back(pBayer2YuvInputStream);

                            CHX_LOG_CONFIG(
                                "non-zsl bayer2yuv input stream, size:%dx%d, format:%d",
                                pBayer2YuvInputStream->width,
                                pBayer2YuvInputStream->height,
                                pBayer2YuvInputStream->format);
                        }
                    }

                    pBayer2YuvOutputStream = CloneStream(&Bayer2YuvStreamsOutput);
                    if (NULL != pBayer2YuvOutputStream)
                    {
                        if (NULL != snapshotConfig.pSnapshotStream)
                        {
                            CHX_LOG_INFO("Internal YUV out stream");
                            pBayer2YuvOutputStream->width  = snapshotWidth;
                            pBayer2YuvOutputStream->height = snapshotHeight;

                            if ((TRUE == isNativeResolution) && (NULL != pBayer2YuvInputStream))
                            {
                                pBayer2YuvOutputStream->width  = pBayer2YuvInputStream->width;
                                pBayer2YuvOutputStream->height = pBayer2YuvInputStream->height;
                            }

                            pNegotiationOutput->pStreams->push_back(pBayer2YuvOutputStream);
                        }
                        else
                        {
                            // configure output stream
                            if (NULL != ppChiStreams)
                            {
                                for (UINT i = 0; i < numStreams; i++)
                                {
                                    if (TRUE == UsecaseSelector::IsYUVSnapshotStream(
                                        reinterpret_cast<camera3_stream_t*>(ppChiStreams[i])))
                                    {
                                        CHX_LOG_INFO("framework YUV out stream");
                                        pNegotiationOutput->pStreams->push_back(ppChiStreams[i]);
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (UINT8 streamIdx = 0; streamIdx < numStreams; streamIdx++)
                    {
                        // For multi camera dual zone, the output stream physical camrea Id will be NULL,
                        // It will not picked for B2Y feature, just input RDI stream and output yuv stream
                        // will be picked for B2Y
                        if ((FALSE == IsFDStream(ppChiStreams[streamIdx]))    &&
                            ((NULL == ppChiStreams[streamIdx]->physicalCameraId) ||
                            ((NULL != ppChiStreams[streamIdx]->physicalCameraId) &&
                            (ChxUtils::GetCameraIdFromStream(ppChiStreams[streamIdx]) == physicalCamId))))
                        {
                            pNegotiationOutput->pStreams->push_back(ppChiStreams[streamIdx]);
                        }

                    }
                }
                break;
            case ChiFeature2Type::BPS:
                if (pNegotiationInfo->pLogicalCameraInfo->numPhysicalCameras == 1)
                {
                    if (FALSE == pNegotiationInfo->pFeatureInstanceId->flags.isNZSLSnapshot)
                    {
                        pBPSInputStream = CloneStream(&BPSStreamsInput);
                        if (NULL != pBPSInputStream)
                        {
                            if (TRUE == isRaw16Format)
                            {
                                pBPSInputStream->format = ChiStreamFormatRaw16;
                            }
                            // configure input stream
                            pBPSInputStream->width  = RDIStream.width;
                            pBPSInputStream->height = RDIStream.height;
                            pNegotiationOutput->pStreams->push_back(pBPSInputStream);
                        }
                    }
                    else
                    {
                        pBPSInputStream = CloneStream(&NZSLBPSStreamsInput);

                        if (NULL != pBPSInputStream)
                        {
                            // get the max sensor output
                            UINT32 maxSensorWidth = 0;
                            UINT32 maxSensorHeight = 0;
                            for (UINT i = 0; i < pNegotiationInfo->pLogicalCameraInfo->m_cameraCaps.numSensorModes; i++)
                            {
                                CHIRECT* pSensorDimension =
                                    &(pNegotiationInfo->pLogicalCameraInfo->pSensorModeInfo[i].frameDimension);
                                if ((pSensorDimension->width  > maxSensorWidth) ||
                                    (pSensorDimension->height > maxSensorHeight))
                                {
                                    maxSensorWidth = pSensorDimension->width;
                                    maxSensorHeight = pSensorDimension->height;
                                }
                            }

                            if (TRUE == isRaw16Format)
                            {
                                pBPSInputStream->format = ChiStreamFormatRaw16;
                            }

                            // configure input stream
                            pBPSInputStream->width  = maxSensorWidth;
                            pBPSInputStream->height = maxSensorHeight;
                            pNegotiationOutput->pStreams->push_back(pBPSInputStream);

                            CHX_LOG_CONFIG(
                                "non-zsl bps input stream, size:%dx%d, format:%d",
                                pBPSInputStream->width,
                                pBPSInputStream->height,
                                pBPSInputStream->format);
                        }
                    }

                    pBPSOutputStream = CloneStream(&BPSStreamsOutput);
                    if (NULL != pBPSOutputStream)
                    {
                        // configure output stream
                        if (NULL != ppChiStreams)
                        {
                            for (UINT i = 0; i < numStreams; i++)
                            {
                                if (TRUE == UsecaseSelector::IsYUVSnapshotStream(
                                    reinterpret_cast<camera3_stream_t*>(ppChiStreams[i])))
                                {
                                    pBPSOutputStream->width  = ppChiStreams[i]->width;
                                    pBPSOutputStream->height = ppChiStreams[i]->height;
                                    CHX_LOG_INFO("framework YUV out stream");

                                    pNegotiationOutput->pStreams->push_back(pBPSOutputStream);
                                }
                            }
                        }

                        if (NULL != snapshotConfig.pSnapshotStream)
                        {
                            CHX_LOG_INFO("Internal YUV out stream");
                            pBPSOutputStream->width  = snapshotWidth;
                            pBPSOutputStream->height = snapshotHeight;
                            pNegotiationOutput->pStreams->push_back(pBPSOutputStream);
                        }
                    }
                }
                else
                {
                    for (UINT8 streamIdx = 0; streamIdx < numStreams; streamIdx++)
                    {
                        // For multi camera dual zone, the output stream physical camrea Id will be NULL,
                        // It will not picked for BPS feature, just input RDI stream and output yuv stream
                        // will be picked for BPS
                        if ((FALSE == IsFDStream(ppChiStreams[streamIdx])) &&
                            ((NULL == ppChiStreams[streamIdx]->physicalCameraId) ||
                            ((NULL != ppChiStreams[streamIdx]->physicalCameraId) &&
                             (ChxUtils::GetCameraIdFromStream(ppChiStreams[streamIdx]) == physicalCamId))))
                        {
                            pNegotiationOutput->pStreams->push_back(ppChiStreams[streamIdx]);
                        }

                    }
                }
                break;
            case ChiFeature2Type::FORMATCONVERTOR:
                if (pNegotiationInfo->pLogicalCameraInfo->numPhysicalCameras == 1)
                {
                    if (FALSE == pNegotiationInfo->pFeatureInstanceId->flags.isNZSLSnapshot)
                    {
                        pBPSInputStream = CloneStream(&BPSStreamsInput);
                        if (NULL != pBPSInputStream)
                        {
                            // configure input stream
                            pBPSInputStream->width  = RDIStream.width;
                            pBPSInputStream->height = RDIStream.height;
                            pNegotiationOutput->pStreams->push_back(pBPSInputStream);
                        }
                    }
                    else
                    {
                        pBPSInputStream = CloneStream(&NZSLBPSStreamsInput);

                        if (NULL != pBPSInputStream)
                        {
                            // get the max sensor output
                            UINT32 maxSensorWidth = 0;
                            UINT32 maxSensorHeight = 0;
                            for (UINT i = 0; i < pNegotiationInfo->pLogicalCameraInfo->m_cameraCaps.numSensorModes; i++)
                            {
                                CHIRECT* pSensorDimension =
                                    &(pNegotiationInfo->pLogicalCameraInfo->pSensorModeInfo[i].frameDimension);
                                if ((pSensorDimension->width  > maxSensorWidth) ||
                                    (pSensorDimension->height > maxSensorHeight))
                                {
                                    maxSensorWidth = pSensorDimension->width;
                                    maxSensorHeight = pSensorDimension->height;
                                }
                            }

                            // configure input stream
                            pBPSInputStream->width  = maxSensorWidth;
                            pBPSInputStream->height = maxSensorHeight;
                            pNegotiationOutput->pStreams->push_back(pBPSInputStream);

                            CHX_LOG_CONFIG(
                                "non-zsl bps input stream, size:%dx%d, format:%d",
                                pBPSInputStream->width,
                                pBPSInputStream->height,
                                pBPSInputStream->format);
                        }
                    }

                    pBPSOutputStream = CloneStream(&BPSStreamsOutput);
                    if ((NULL != pBPSOutputStream) && (NULL != pBPSInputStream))
                    {
                        // configure output stream
                        if (NULL != snapshotConfig.pSnapshotStream)
                        {
                            CHX_LOG_INFO("Internal YUV out stream");
                            pBPSOutputStream->streamType    = ChiStreamTypeOutput;
                            pBPSOutputStream->format        = ChiStreamFormatRaw16;
                            pBPSOutputStream->width         = pBPSInputStream->width;
                            pBPSOutputStream->height        = pBPSInputStream->height;
                            pNegotiationOutput->pStreams->push_back(pBPSOutputStream);
                        }
                    }
                }
                else
                {
                    for (UINT8 streamIdx = 0; streamIdx < numStreams; streamIdx++)
                    {
                        if ((FALSE == IsFDStream(ppChiStreams[streamIdx])) &&
                            ((NULL == ppChiStreams[streamIdx]->physicalCameraId) ||
                            ((NULL != ppChiStreams[streamIdx]->physicalCameraId) &&
                             (ChxUtils::GetCameraIdFromStream(ppChiStreams[streamIdx]) == physicalCamId))))
                        {
                            pNegotiationOutput->pStreams->push_back(ppChiStreams[streamIdx]);
                        }

                    }
                }
                break;
            case ChiFeature2Type::IPE:
                pIPEInputStream = CloneStream(&IPEStreamsInput);

                if (NULL != pIPEInputStream)
                {
                    // configure input stream
                    if (TRUE == isNativeResolution)
                    {
                        pIPEInputStream->width  = BPSStreamsOutput.width;
                        pIPEInputStream->height = BPSStreamsOutput.height;
                    }
                    else
                    {
                        pIPEInputStream->width  = snapshotWidth;
                        pIPEInputStream->height = snapshotHeight;
                    }
                    pNegotiationOutput->pStreams->push_back(pIPEInputStream);
                }

                if (NULL != snapshotConfig.pSnapshotStream)
                {
                    pNegotiationOutput->pStreams->push_back(snapshotConfig.pSnapshotStream);
                }
                else
                {
                    CHISTREAM* pDummyStream = CloneStream(&IPEStreamsOutput);
                    pNegotiationOutput->pStreams->push_back(pDummyStream);
                }

                // Used only in HEIC Snapshots
                if (NULL != snapshotConfig.pThumbnailStream)
                {
                    pNegotiationOutput->pStreams->push_back(snapshotConfig.pThumbnailStream);
                }
                break;
            case ChiFeature2Type::JPEG:
                pJPEGInputStream = CloneStream(&YUV2JPEGStreamsInput);
                if (NULL != pJPEGInputStream)
                {
                    // configure input stream
                    if (TRUE == isNativeResolution)
                    {
                        pJPEGInputStream->width  = RDIStream.width;
                        pJPEGInputStream->height = RDIStream.height;
                    }
                    else
                    {
                        pJPEGInputStream->width  = snapshotWidth;
                        pJPEGInputStream->height = snapshotHeight;
                    }
                    pNegotiationOutput->pStreams->push_back(pJPEGInputStream);
                }

                // Used in both HEIC + JPEG Snapshots
                if (NULL != snapshotConfig.pSnapshotStream)
                {
                    pNegotiationOutput->pStreams->push_back(snapshotConfig.pSnapshotStream);
                }
                else
                {
                    CHISTREAM* pDummyStream = CloneStream(&YUV2JPEGStreamsOutput);
                    pNegotiationOutput->pStreams->push_back(pDummyStream);
                }

                // Used only in HEIC Snapshots
                if (NULL != snapshotConfig.pThumbnailStream)
                {
                    pNegotiationOutput->pStreams->push_back(snapshotConfig.pThumbnailStream);
                }
                break;
            default:
                CHX_LOG_ERROR("Invalid featureID! featureId:%d", featureId);
                result = CDKResultEInvalidArg;
                break;
        }
        if (CDKResultSuccess == result)
        {
            pNegotiationOutput->pDesiredStreamConfig->numStreams         = pNegotiationOutput->pStreams->size();
            pNegotiationOutput->pDesiredStreamConfig->operationMode      = pNegotiationInfo->pFwkStreamConfig->operationMode;
            pNegotiationOutput->pDesiredStreamConfig->pChiStreams        = &(*(pNegotiationOutput->pStreams))[0];
            pNegotiationOutput->pDesiredStreamConfig->pSessionSettings   = NULL;
            pNegotiationOutput->disableZoomCrop                          = isNativeResolution;

            CHX_LOG_INFO("numStream:%d", pNegotiationOutput->pDesiredStreamConfig->numStreams);
        }
    }

    return result;
}

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2OpsEntry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDK_VISIBILITY_PUBLIC VOID ChiFeature2OpsEntry(
    CHIFEATURE2OPS* pChiFeature2Ops)
{
    if (NULL != pChiFeature2Ops)
    {
        pChiFeature2Ops->size                   = sizeof(CHIFEATURE2OPS);
        pChiFeature2Ops->majorVersion           = Feature2MajorVersion;
        pChiFeature2Ops->minorVersion           = Feature2MinorVersion;
        pChiFeature2Ops->pVendorName            = VendorName;
        pChiFeature2Ops->pCreate                = CreateFeature;
        pChiFeature2Ops->pQueryCaps             = DoQueryCaps;
        pChiFeature2Ops->pGetVendorTags         = GetVendorTags;
        pChiFeature2Ops->pStreamNegotiation     = DoStreamNegotiation;
    }
}

#ifdef __cplusplus
}
#endif // __cplusplus
