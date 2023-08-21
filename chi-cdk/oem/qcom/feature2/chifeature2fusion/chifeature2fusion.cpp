////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2fusion.cpp
/// @brief multi camera fusion feature implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chiifedefs.h"

#include "chifeature2base.h"
#include "chifeature2featurepool.h"
#include "chifeature2fusion.h"
#include "chifeature2utils.h"

// NOWHINE FILE CP006:  Used standard libraries for performance improvements

/// @brief fusion min buffer count to set in Meta TBM
static const UINT32 FEATURE_FUSION_MIN_METADATA_BUFFER_COUNT = 1;
static const UINT32 Feature2MajorVersion                     = 0;
static const UINT32 Feature2MinorVersion                     = 1;
static const CHAR*  VendorName                               = "QTI";

static const CHAR*  Feature2FusionCaps[] =
{
    "FUSION",
    "BOKEH"
};

// Maximum active ports: Two YUV buffer port and Two input metadata port
static const UINT8 MaxActiveInputPorts    = 4;
static const UINT8 MaxInputPortsPerCamera = 2;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::~ChiFeature2Fusion
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Fusion::~ChiFeature2Fusion()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Fusion* ChiFeature2Fusion::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2Fusion* pFeature    = CHX_NEW(ChiFeature2Fusion);
    CDKResult          result      = CDKResultSuccess;

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
    }

    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Fusion::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::DoFlush()
{
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::OnPrepareRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::OnSelectFlowToExecuteRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RequestFlowType ChiFeature2Fusion::OnSelectFlowToExecuteRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNREFERENCED_PARAM(pRequestObject);

    return ChiFeature2RequestFlowType::Type0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::OnPortCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::OnPortCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    result = ChiFeature2Base::OnPortCreate(pKey);

    if (NULL != pKey)
    {
        ChiFeaturePortData* pPortData = GetPortData(pKey);

        if ((NULL != pPortData) && (ChiFeature2PortType::MetaData == pPortData->globalId.portType))
        {
            pPortData->minBufferCount = FEATURE_FUSION_MIN_METADATA_BUFFER_COUNT;
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
// ChiFeature2Fusion::OnPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::OnPipelineCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pKey)
    {
        ChiFeaturePipelineData* pPipelineData = GetPipelineData(pKey);
        if (NULL != pPipelineData)
        {
            pPipelineData->minMetaBufferCount = FEATURE_FUSION_MIN_METADATA_BUFFER_COUNT;
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid key");
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::PopulateDependencyPortsBasedOnMCC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::PopulateDependencyPortsBasedOnMCC(
    ChiFeature2RequestObject*         pRequestObject,
    UINT8                             dependencyIndex,
    const ChiFeature2InputDependency* pInputDependency
    ) const
{
    CDKResult     result                       = CDKResultSuccess;
    ChiMetadata*  pFeatureSettings             = NULL;
    UINT8         requestId                    = pRequestObject->GetCurRequestId();
    const Feature2ControllerResult* pMCCResult = pRequestObject->GetUsecaseRequestObject()->GetMCCResult();

    for (UINT8 portIndex = 0; portIndex < pInputDependency->numInputDependency; ++portIndex)
    {
        ChiFeature2PortDescriptor portDescriptor = pInputDependency->pInputDependency[portIndex];
        ChiFeature2Identifier     portidentifier = portDescriptor.globalId;
        UINT8 camIdx                             = portIndex >> 1;

        CHX_LOG_INFO("portIndex:%d, camIdx:%d,isActive:%d",
            portIndex, camIdx, ChxUtils::IsBitSet(pMCCResult->activeMap, camIdx));

        if (TRUE == ChxUtils::IsBitSet(pMCCResult->activeMap, camIdx))
        {
            result = pRequestObject->SetPortDescriptor(ChiFeature2SequenceOrder::Next,
                ChiFeature2RequestObjectOpsType::InputDependency,
                &portidentifier, &portDescriptor, requestId, dependencyIndex);

            CHX_LOG_INFO("%s: Set dependency on port %s, requestId %d, dependencyIndex %d",
                pRequestObject->IdentifierString(),
                portDescriptor.pPortName,
                requestId,
                dependencyIndex);

            if (portidentifier.portType == ChiFeature2PortType::MetaData)
            {
                pFeatureSettings = ChiMetadata::Create(NULL, 0, true);
                if (NULL == pFeatureSettings)
                {
                    CHX_LOG_ERROR("Alloc memory failed");
                    result = CDKResultENoMemory;
                }

                if (CDKResultSuccess == result)
                {
                    result = PopulateDependencySettings(pRequestObject, dependencyIndex, &portidentifier, pFeatureSettings);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::FillSATOfflinePipelineInputMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::FillSATOfflinePipelineInputMetadata(
    MulticamResultMetadata* pMultiCamResultMetadata,
    ChiMetadata*            pOfflineMetadata
    ) const
{
    CDKResult      result = CDKResultSuccess;
    CameraSettings cameraSettings[MaxDevicePerLogicalCamera];

    // Identify camera ID and metadata
    MultiCameraIds*    pMultiCameraId = NULL;
    UINT32             pCameraId;
    BOOL               pIsMaster;
    UINT32             masterCamId = 0;
    UINT32             masterMetaIdx = 0;
    UINT32             metadataIds[MaxCameras];
    ChiMetadata*       pMetadataArray[MaxCameras];
    UINT32             primaryMetaId = 0;
    UINT32             metaIndex = 0;

    if (NULL == pMultiCamResultMetadata)
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < pMultiCamResultMetadata->numResults; i++)
        {
            cameraSettings[i].pMetadata = pMultiCamResultMetadata->ppResultMetadata[i];

            if (NULL != cameraSettings[i].pMetadata)
            {
                // Get camera ID from metadata
                pCameraId = GetMetaOwner(cameraSettings[i].pMetadata);

                /// Check if current camera is SW master
                pIsMaster = IsMasterCamera(cameraSettings[i].pMetadata, m_pCameraInfo);

                /// Extract Master camera info from metadata
                if (INVALID_INDEX != pCameraId)
                {
                    cameraSettings[i].cameraId = pCameraId;
                    CHX_LOG_INFO("Current Camera Id %d, is SW master %d",
                        cameraSettings[i].cameraId, pIsMaster);

                    metadataIds[metaIndex]    = cameraSettings[i].cameraId;
                    pMetadataArray[metaIndex] = cameraSettings[i].pMetadata;

                    if (TRUE == pIsMaster)
                    {
                        masterCamId = cameraSettings[i].cameraId;
                        masterMetaIdx = i;
                        primaryMetaId = metadataIds[metaIndex];
                        CHX_LOG_INFO("Master Camera Id %d, Meta Index %d primaryMetaId %d",
                            masterCamId, masterMetaIdx, primaryMetaId);
                    }

                    metaIndex++;
                }
                else
                {
                    CHX_LOG_ERROR("MetadataOwner is null");
                    result = CDKResultEFailed;
                }
            }
        }
    }

    CHITAGSOPS  vendorTagOps;
    UINT32      vendorTagCropRegions;
    UINT32      vendorTagMCCmetadataOpticalZoom;
    InputMetadataOpticalZoom* pMCCmetadataOpticalZoom;
    ExtensionModule::GetInstance()->GetVendorTagOps(&vendorTagOps);

    InputMetadataOpticalZoom metadataOpticalZoom;
    ChxUtils::Memset(&metadataOpticalZoom, 0, sizeof(InputMetadataOpticalZoom));

    if (NULL != pMultiCamResultMetadata)
    {
        if (CDKResultSuccess == result)
        {
            for (UINT32 i = 0; i < pMultiCamResultMetadata->numResults; i++)
            {
                if (NULL != cameraSettings[i].pMetadata)
                {
                    ExtractCameraMetadata(cameraSettings[i].pMetadata, &metadataOpticalZoom.cameraMetadata[i]);

                    UINT32 camIdx    = GetCameraIndex(cameraSettings[i].cameraId, m_pCameraInfo);
                    UINT32 masterIdx = GetCameraIndex(masterCamId, m_pCameraInfo);
                    CHX_ASSERT(INVALID_INDEX != camIdx);

                    vendorTagOps.pQueryVendorTagLocation("com.qti.chi.multicamerainputmetadata",
                                                           "InputMetadataOpticalZoom", &vendorTagMCCmetadataOpticalZoom);
                    pMCCmetadataOpticalZoom = static_cast <InputMetadataOpticalZoom *>(cameraSettings[i].pMetadata->GetTag(
                        vendorTagMCCmetadataOpticalZoom));

                    metadataOpticalZoom.cameraMetadata[i].isValid          = TRUE;
                    metadataOpticalZoom.cameraMetadata[i].cameraId         = m_pCameraInfo->ppDeviceInfo[camIdx]->cameraId;
                    metadataOpticalZoom.cameraMetadata[i].masterCameraId   = masterCamId;
                    metadataOpticalZoom.cameraMetadata[i].fovRectIFE       =
                        pMCCmetadataOpticalZoom->cameraMetadata[camIdx].fovRectIFE;
                    metadataOpticalZoom.cameraMetadata[i].fovRectIPE       =
                        pMCCmetadataOpticalZoom->cameraMetadata[camIdx].fovRectIPE;
                    metadataOpticalZoom.cameraMetadata[i].activeArraySize  =
                        pMCCmetadataOpticalZoom->cameraMetadata[camIdx].activeArraySize;

                    metadataOpticalZoom.outputShiftSnapshot.horizonalShift =
                        pMCCmetadataOpticalZoom->outputShiftSnapshot.horizonalShift;
                    metadataOpticalZoom.outputShiftSnapshot.verticalShift  =
                        pMCCmetadataOpticalZoom->outputShiftSnapshot.verticalShift;
                    metadataOpticalZoom.isSnapshot                         = TRUE;

                    CHX_LOG_VERBOSE("camerid:%d, masterid:%d, camindex:%d, masterindex:%d",
                        metadataOpticalZoom.cameraMetadata[i].cameraId,
                        metadataOpticalZoom.cameraMetadata[i].masterCameraId,
                        camIdx,
                        masterIdx);

                    CaptureRequestCropRegions* pCropRegions;
                    vendorTagOps.pQueryVendorTagLocation("com.qti.cropregions",
                                           "crop_regions", &vendorTagCropRegions);
                    pCropRegions = static_cast <CaptureRequestCropRegions *>(cameraSettings[i].pMetadata->GetTag(
                        vendorTagCropRegions));

                    if (NULL != pCropRegions)
                    {
                        metadataOpticalZoom.cameraMetadata[i].userCropRegion = pCropRegions->userCropRegion;
                        metadataOpticalZoom.cameraMetadata[i].pipelineCropRegion = pCropRegions->pipelineCropRegion;
                        metadataOpticalZoom.cameraMetadata[i].ifeLimitCropRegion = pCropRegions->ifeLimitCropRegion;
                        CHX_LOG("Camera id %d Pipeline cropregion:%d,%d,%d,%d", cameraSettings[i].cameraId,
                            pCropRegions->pipelineCropRegion.left, pCropRegions->pipelineCropRegion.top,
                            pCropRegions->pipelineCropRegion.width, pCropRegions->pipelineCropRegion.height);
                    }
                    else
                    {
                        CHX_LOG_ERROR("Metadata is NULL for tag CropRegions");
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Camera metadata for cameraid %d is NULL", cameraSettings[i].cameraId);
                }
            }

            metadataOpticalZoom.frameId = pMultiCamResultMetadata->frameNum;
            metadataOpticalZoom.numInputs = pMultiCamResultMetadata->numResults;

            result = pOfflineMetadata->Invalidate();
            result = pOfflineMetadata->MergeMultiCameraMetadata(pMultiCamResultMetadata->numResults,
                pMetadataArray, metadataIds, primaryMetaId);
            CHX_LOG_INFO("Merge multicamera meta count %d primary %d", metaIndex, primaryMetaId);

            if (CDKResultSuccess != result)
            {
                CHX_LOG("Failure in append meta, dst meta size %ul src meta size %ul",
                    pOfflineMetadata->Count(),
                    cameraSettings[masterMetaIdx].pMetadata->Count());
            }

            if (CDKResultSuccess != pOfflineMetadata->SetTag("com.qti.chi.multicamerainputmetadata",
                                                              "InputMetadataOpticalZoom",
                                                              &metadataOpticalZoom,
                                                              sizeof(InputMetadataOpticalZoom)))
            {
                CHX_LOG("Failure in pSetMetaData of m_vendorTagOpticalZoomInputMeta");
            }

            /// Update master camera ID
            ChxUtils::FillCameraId(pOfflineMetadata, masterCamId);
        }
        else
        {
            CHAR metaFileName[MaxFileLen];
            for (UINT32 i = 0; i < pMultiCamResultMetadata->numResults; i++)
            {
                if (NULL != pMultiCamResultMetadata->ppResultMetadata[i])
                {
                    CdkUtils::SNPrintF(metaFileName, sizeof(metaFileName), "mcam_%d_%d.txt",
                        cameraSettings[i].cameraId,
                        pMultiCamResultMetadata->ppResultMetadata[i]->GetFrameNumber());
                    pMultiCamResultMetadata->ppResultMetadata[i]->DumpDetailsToFile(metaFileName);
                }
            }
            result = CamxResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::FillRTBOfflinePipelineInputMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::FillRTBOfflinePipelineInputMetadata(
    MulticamResultMetadata* pMultiCamResultMetadata,
    ChiMetadata*            pOfflineMetadata
    ) const
{
    CDKResult      result = CDKResultSuccess;
    CameraSettings cameraSettings[MaxDevicePerLogicalCamera];

    // Identify camera ID and metadata
    MultiCameraIds*    pMultiCameraId = NULL;
    UINT32             pCameraId;
    BOOL               pIsMaster;
    UINT32             masterCamId = 0;
    UINT32             masterMetaIdx = 0;
    UINT32             metadataIds[MaxCameras];
    ChiMetadata*       pMetadataArray[MaxCameras];
    UINT32             primaryMetaId = 0;
    UINT32             metaIndex = 0;

    if (NULL == pMultiCamResultMetadata)
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        for (UINT32 i = 0; i < pMultiCamResultMetadata->numResults; i++)
        {
            cameraSettings[i].pMetadata = pMultiCamResultMetadata->ppResultMetadata[i];

            if (NULL != cameraSettings[i].pMetadata)
            {
                // Get camera ID from metadata
                pCameraId = GetMetaOwner(cameraSettings[i].pMetadata);

                /// Check if current camera is SW master
                pIsMaster = IsMasterCamera(cameraSettings[i].pMetadata, m_pCameraInfo);

                /// Extract Master camera info from metadata
                if (INVALID_INDEX != pCameraId)
                {
                    cameraSettings[i].cameraId = pCameraId;
                    CHX_LOG_INFO("Current Camera Id %d, is SW master %d",
                        cameraSettings[i].cameraId, pIsMaster);

                    metadataIds[metaIndex]    = cameraSettings[i].cameraId;
                    pMetadataArray[metaIndex] = cameraSettings[i].pMetadata;

                    if (TRUE == pIsMaster)
                    {
                        masterCamId = cameraSettings[i].cameraId;
                        masterMetaIdx = i;
                        primaryMetaId = metadataIds[metaIndex];
                        CHX_LOG_INFO("Master Camera Id %d, Meta Index %d primaryMetaId %d",
                            masterCamId, masterMetaIdx, primaryMetaId);
                    }

                    metaIndex++;
                }
                else
                {
                    CHX_LOG_ERROR("MetadataOwner is null");
                    result = CDKResultEFailed;
                }
            }
        }
    }

    CHITAGSOPS  vendorTagOps;
    UINT32      vendorTagCropRegions;
    UINT32      vendorTagMCCmetadataBokeh;
    InputMetadataBokeh* pMCCmetadataBokeh = NULL;
    ExtensionModule::GetInstance()->GetVendorTagOps(&vendorTagOps);

    InputMetadataBokeh metadataBokeh = { 0 };

    if (NULL != pMultiCamResultMetadata)
    {
        if (CDKResultSuccess == result)
        {
            for (UINT32 i = 0; i < pMultiCamResultMetadata->numResults; i++)
            {
                if (NULL != cameraSettings[i].pMetadata)
                {
                    ExtractCameraMetadata(cameraSettings[i].pMetadata, &metadataBokeh.cameraMetadata[i]);

                    UINT32 camIdx    = GetCameraIndex(cameraSettings[i].cameraId, m_pCameraInfo);
                    UINT32 masterIdx = GetCameraIndex(masterCamId, m_pCameraInfo);
                    CHX_ASSERT(INVALID_INDEX != camIdx);

                    vendorTagOps.pQueryVendorTagLocation("com.qti.chi.multicamerainputmetadata",
                        "InputMetadataBokeh", &vendorTagMCCmetadataBokeh);
                    pMCCmetadataBokeh = static_cast <InputMetadataBokeh *>(cameraSettings[i].pMetadata->GetTag(
                         vendorTagMCCmetadataBokeh));

                    metadataBokeh.cameraMetadata[i].isValid          = TRUE;
                    metadataBokeh.cameraMetadata[i].cameraId         = m_pCameraInfo->ppDeviceInfo[camIdx]->cameraId;
                    metadataBokeh.cameraMetadata[i].masterCameraId   = masterCamId;
                    if (NULL != pMCCmetadataBokeh)
                    {
                        metadataBokeh.cameraMetadata[i].fovRectIFE       =
                            pMCCmetadataBokeh->cameraMetadata[camIdx].fovRectIFE;
                        metadataBokeh.cameraMetadata[i].fovRectIPE       =
                            pMCCmetadataBokeh->cameraMetadata[camIdx].fovRectIPE;
                        metadataBokeh.cameraMetadata[i].activeArraySize  =
                            pMCCmetadataBokeh->cameraMetadata[camIdx].activeArraySize;
                    }

                    CaptureRequestCropRegions* pCropRegions;
                    vendorTagOps.pQueryVendorTagLocation("com.qti.cropregions", "crop_regions", &vendorTagCropRegions);

                    pCropRegions = static_cast <CaptureRequestCropRegions *>(cameraSettings[i].pMetadata->GetTag(
                        vendorTagCropRegions));

                    if (NULL != pCropRegions)
                    {
                        metadataBokeh.cameraMetadata[i].userCropRegion = pCropRegions->userCropRegion;
                        metadataBokeh.cameraMetadata[i].pipelineCropRegion = pCropRegions->pipelineCropRegion;
                        metadataBokeh.cameraMetadata[i].ifeLimitCropRegion = pCropRegions->ifeLimitCropRegion;
                        CHX_LOG("Camera id %d Pipeline cropregion:%d,%d,%d,%d", cameraSettings[i].cameraId,
                            pCropRegions->pipelineCropRegion.left, pCropRegions->pipelineCropRegion.top,
                            pCropRegions->pipelineCropRegion.width, pCropRegions->pipelineCropRegion.height);
                    }
                    else
                    {
                        CHX_LOG_ERROR("Metadata is NULL for tag CropRegions");
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Camera metadata for cameraid %d is NULL", cameraSettings[i].cameraId);
                }
            }

            metadataBokeh.frameId = pMultiCamResultMetadata->frameNum;
            metadataBokeh.blurLevel = 0;
            metadataBokeh.blurMinValue = 0;
            metadataBokeh.blurMaxValue = 0;
            metadataBokeh.isSnapshot   = TRUE;

            result = pOfflineMetadata->Invalidate();
            result = pOfflineMetadata->MergeMultiCameraMetadata(pMultiCamResultMetadata->numResults,
                pMetadataArray, metadataIds, primaryMetaId);
            CHX_LOG_INFO("Merge multicamera meta count %d primary %d", metaIndex, primaryMetaId);

            if (CDKResultSuccess != result)
            {
                CHX_LOG("Failure in append meta, dst meta size %ul src meta size %ul",
                    pOfflineMetadata->Count(),
                    cameraSettings[masterMetaIdx].pMetadata->Count());
            }

            if (CDKResultSuccess != pOfflineMetadata->SetTag("com.qti.chi.multicamerainputmetadata",
                "InputMetadataBokeh", &metadataBokeh, sizeof(InputMetadataOpticalZoom)))
            {
                CHX_LOG("Failure in pSetMetaData of m_vendorTagOpticalZoomInputMeta");
            }

            /// Update master camera ID
            ChxUtils::FillCameraId(pOfflineMetadata, masterCamId);
        }
        else
        {
            CHAR metaFileName[MaxFileLen];
            for (UINT32 i = 0; i < pMultiCamResultMetadata->numResults; i++)
            {
                if (NULL != pMultiCamResultMetadata->ppResultMetadata[i])
                {
                    CdkUtils::SNPrintF(metaFileName, sizeof(metaFileName), "mcam_%d_%d.txt",
                        cameraSettings[i].cameraId,
                        pMultiCamResultMetadata->ppResultMetadata[i]->GetFrameNumber());
                    pMultiCamResultMetadata->ppResultMetadata[i]->DumpDetailsToFile(metaFileName);
                }
            }
            result = CamxResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2Fusion::ExtractCameraMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Fusion::ExtractCameraMetadata(
    ChiMetadata*    pMetadata,
    CameraMetadata* pExtractedCameraMetadata
    ) const
{
    camera_metadata_entry_t entry = { 0 };
    if (0 == pMetadata->FindTag(ANDROID_CONTROL_AF_REGIONS, &entry))
    {
        pExtractedCameraMetadata->afFocusROI.xMin   = entry.data.i32[0];
        pExtractedCameraMetadata->afFocusROI.yMin   = entry.data.i32[1];
        pExtractedCameraMetadata->afFocusROI.xMax   = entry.data.i32[2];
        pExtractedCameraMetadata->afFocusROI.yMax   = entry.data.i32[3];
        pExtractedCameraMetadata->afFocusROI.weight = entry.data.i32[4];
    }

    if (0 == pMetadata->FindTag(ANDROID_CONTROL_AF_STATE, &entry))
    {
        pExtractedCameraMetadata->afState = entry.data.u8[0];
    }

    if (0 == pMetadata->FindTag(ANDROID_SENSOR_SENSITIVITY, &entry))
    {
        pExtractedCameraMetadata->isoSensitivity = entry.data.i32[0];
    }

    if (0 == pMetadata->FindTag(ANDROID_SENSOR_EXPOSURE_TIME, &entry))
    {
        pExtractedCameraMetadata->exposureTime = entry.data.i64[0];
    }

    if (0 == pMetadata->FindTag(ANDROID_SENSOR_TIMESTAMP, &entry))
    {
        pExtractedCameraMetadata->sensorTimestamp = entry.data.i64[0];
    }

    if (0 == pMetadata->FindTag(ANDROID_STATISTICS_FACE_RECTANGLES, &entry))
    {
        UINT32 numElemsRect = sizeof(CHIRECT) / sizeof(UINT32);
        pExtractedCameraMetadata->fdMetadata.numFaces = entry.count / numElemsRect;

        UINT32 dataIndex = 0;
        for (INT32 i = 0; i < pExtractedCameraMetadata->fdMetadata.numFaces; ++i)
        {
            INT32 xMin = entry.data.i32[dataIndex++];
            INT32 yMin = entry.data.i32[dataIndex++];
            INT32 xMax = entry.data.i32[dataIndex++];
            INT32 yMax = entry.data.i32[dataIndex++];
            pExtractedCameraMetadata->fdMetadata.faceRect[i].left   = xMin;
            pExtractedCameraMetadata->fdMetadata.faceRect[i].top    = yMin;
            pExtractedCameraMetadata->fdMetadata.faceRect[i].width  = xMax - xMin + 1;
            pExtractedCameraMetadata->fdMetadata.faceRect[i].height = yMax - yMin + 1;
        }
    }

    if (0 == pMetadata->FindTag(ANDROID_STATISTICS_FACE_SCORES, &entry))
    {
        for (INT32 i = 0; i < pExtractedCameraMetadata->fdMetadata.numFaces; ++i)
        {
            pExtractedCameraMetadata->fdMetadata.faceScore[i] = entry.data.u8[i];
        }
    }

    CHITAGSOPS  vendorTagOps;
    UINT32      vendorTagIFEResidualCrop;
    UINT32      vendorTagAppliedCrop;
    UINT32      vendorTagSensorIFEAppliedCrop;

    ExtensionModule::GetInstance()->GetVendorTagOps(&vendorTagOps);
    vendorTagOps.pQueryVendorTagLocation("org.quic.camera.ifecropinfo",
                                           "AppliedCrop", &vendorTagAppliedCrop);
    IFECropInfo* pIFEAppliedCrop = static_cast<IFECropInfo*>(pMetadata->GetTag(
        vendorTagAppliedCrop));

    BOOL isFullPortEnabled = TRUE;
    if (NULL != pIFEAppliedCrop)
    {
        // Usecase can use either video/full or preview/display full port.
        isFullPortEnabled  = IsValidRect(pIFEAppliedCrop->fullPath);

        if (TRUE == isFullPortEnabled)
        {
            pExtractedCameraMetadata->ifeAppliedCrop.left   = pIFEAppliedCrop->fullPath.left;
            pExtractedCameraMetadata->ifeAppliedCrop.top    = pIFEAppliedCrop->fullPath.top;
            pExtractedCameraMetadata->ifeAppliedCrop.width  = pIFEAppliedCrop->fullPath.width;
            pExtractedCameraMetadata->ifeAppliedCrop.height = pIFEAppliedCrop->fullPath.height;
        }
        else
        {
            pExtractedCameraMetadata->ifeAppliedCrop.left   = pIFEAppliedCrop->displayFullPath.left;
            pExtractedCameraMetadata->ifeAppliedCrop.top    = pIFEAppliedCrop->displayFullPath.top;
            pExtractedCameraMetadata->ifeAppliedCrop.width  = pIFEAppliedCrop->displayFullPath.width;
            pExtractedCameraMetadata->ifeAppliedCrop.height = pIFEAppliedCrop->displayFullPath.height;
        }
    }

    ExtensionModule::GetInstance()->GetVendorTagOps(&vendorTagOps);
    vendorTagOps.pQueryVendorTagLocation("org.quic.camera.ifecropinfo",
                                           "ResidualCrop", &vendorTagIFEResidualCrop);
    IFECropInfo* pIFEResidualCrop = static_cast<IFECropInfo*>(pMetadata->GetTag(
        vendorTagIFEResidualCrop));
    if (NULL != pIFEResidualCrop)
    {
        if (TRUE == isFullPortEnabled)
        {
            pExtractedCameraMetadata->ifeResidualCrop.left   = pIFEResidualCrop->fullPath.left;
            pExtractedCameraMetadata->ifeResidualCrop.top    = pIFEResidualCrop->fullPath.top;
            pExtractedCameraMetadata->ifeResidualCrop.width  = pIFEResidualCrop->fullPath.width;
            pExtractedCameraMetadata->ifeResidualCrop.height = pIFEResidualCrop->fullPath.height;
        }
        else
        {
            pExtractedCameraMetadata->ifeResidualCrop.left   = pIFEResidualCrop->displayFullPath.left;
            pExtractedCameraMetadata->ifeResidualCrop.top    = pIFEResidualCrop->displayFullPath.top;
            pExtractedCameraMetadata->ifeResidualCrop.width  = pIFEResidualCrop->displayFullPath.width;
            pExtractedCameraMetadata->ifeResidualCrop.height = pIFEResidualCrop->displayFullPath.height;
        }
    }

    ExtensionModule::GetInstance()->GetVendorTagOps(&vendorTagOps);
    vendorTagOps.pQueryVendorTagLocation("org.quic.camera.ifecropinfo",
                                           "SensorIFEAppliedCrop", &vendorTagSensorIFEAppliedCrop);
    IFECropInfo* pSensorIFEAppliedCrop = static_cast<IFECropInfo*>(pMetadata->GetTag(
        vendorTagSensorIFEAppliedCrop));
    if (NULL != pSensorIFEAppliedCrop)
    {
        if (TRUE == isFullPortEnabled)
        {
            pExtractedCameraMetadata->sensorIFEAppliedCrop.left   = pSensorIFEAppliedCrop->fullPath.left;
            pExtractedCameraMetadata->sensorIFEAppliedCrop.top    = pSensorIFEAppliedCrop->fullPath.top;
            pExtractedCameraMetadata->sensorIFEAppliedCrop.width  = pSensorIFEAppliedCrop->fullPath.width;
            pExtractedCameraMetadata->sensorIFEAppliedCrop.height = pSensorIFEAppliedCrop->fullPath.height;
        }
        else
        {
            pExtractedCameraMetadata->sensorIFEAppliedCrop.left   = pSensorIFEAppliedCrop->displayFullPath.left;
            pExtractedCameraMetadata->sensorIFEAppliedCrop.top    = pSensorIFEAppliedCrop->displayFullPath.top;
            pExtractedCameraMetadata->sensorIFEAppliedCrop.width  = pSensorIFEAppliedCrop->displayFullPath.width;
            pExtractedCameraMetadata->sensorIFEAppliedCrop.height = pSensorIFEAppliedCrop->displayFullPath.height;
        }
    }

    UINT32      vendorTagLuxIndex;

    ExtensionModule::GetInstance()->GetVendorTagOps(&vendorTagOps);
    vendorTagOps.pQueryVendorTagLocation("com.qti.chi.statsaec",
                                           "AecLux", &vendorTagLuxIndex);
    FLOAT* pLux = static_cast<FLOAT*>(pMetadata->GetTag(vendorTagLuxIndex));
    if (NULL != pLux)
    {
        pExtractedCameraMetadata->lux = pLux ? *pLux : 0.0f;
    }

    if (0 == pMetadata->FindTag(ANDROID_LENS_FOCUS_DISTANCE, &entry))
    {
        /* Unit of optimal focus distance in diopter, convert it to cm */
        if (entry.data.f[0] > 0.0f)
        {
            pExtractedCameraMetadata->focusDistCm = 100.0f / entry.data.f[0];
        }
        else
        {
            pExtractedCameraMetadata->focusDistCm = FocusDistanceCmMax;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::OnPopulateDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::OnPopulateDependency (
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult             result      = CDKResultSuccess;
    ChiFeature2StageInfo  stageInfo   = { 0 };
    UINT8                 requestId   = 0;

    UINT8                                numRequestOutputs  = 0;
    const ChiFeature2RequestOutputInfo*  pRequestOutputInfo = NULL;
    const ChiFeature2StageDescriptor*    pStageDescriptor   = NULL;

    requestId       = pRequestObject->GetCurRequestId();
    pRequestObject->GetNextStageInfo(&stageInfo, requestId);
    pRequestObject->GetExternalRequestOutput(&numRequestOutputs, &pRequestOutputInfo, requestId);

    if (InvalidStageId != stageInfo.stageId)
    {
        pStageDescriptor = GetStageDescriptor(stageInfo.stageId);
    }

    if (NULL != pStageDescriptor)
    {
        for (UINT8 sessionIdx = 0; sessionIdx < pStageDescriptor->numDependencyConfigDescriptor; sessionIdx++)
        {
            const ChiFeature2SessionInfo* pSessionInfo = &pStageDescriptor->pDependencyConfigDescriptor[sessionIdx];

            for (UINT8 pipelineId = 0; pipelineId < pSessionInfo->numPipelines; pipelineId++)
            {
                const ChiFeature2PipelineInfo*          pPipelineInfo    = &pSessionInfo->pPipelineInfo[pipelineId];
                ChiFeature2DependencyConfigDescriptor*  pDescriptorList  =
                    reinterpret_cast<ChiFeature2DependencyConfigDescriptor*>(pPipelineInfo->handle);

                if (NULL != pDescriptorList)
                {
                    for (UINT8 listIndex = 0; listIndex < pDescriptorList->numInputDependency; ++listIndex)
                    {
                        const ChiFeature2InputDependency* pDependency =
                            GetDependencyListFromStageDescriptor(pStageDescriptor,
                                sessionIdx, pipelineId, listIndex);

                        if (NULL != pDependency)
                        {
                            PopulateDependencyPortsBasedOnMCC(pRequestObject, listIndex, pDependency);
                        }
                    }
                }
            }
        }
    }

    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::BuildInputPortListBasedOnMCC
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::BuildInputPortListBasedOnMCC(
    ChiFeature2RequestObject*           pRequestObject,
    UINT8                               stageId,
    ChiFeature2PortIdList*              pInputPortList,
    std::vector<ChiFeature2Identifier>& rEnabledPortList
    ) const
{
    CDKResult result = CDKResultSuccess;

    if ((NULL == pRequestObject) || (NULL == pInputPortList))
    {
        CHX_LOG_ERROR("Invalid Arg! pRequestObject=%p, pInputPortList=%p",
            pRequestObject, pInputPortList);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2PortIdList inputPortList    = { 0 };
        rEnabledPortList.reserve(MaxActiveInputPorts);

        const Feature2ControllerResult* pMCCResult = pRequestObject->GetUsecaseRequestObject()->GetMCCResult();

        result = GetInputPortsForStage(stageId, &inputPortList);

        if (CDKResultSuccess == result)
        {
            // Build input list based MCC result
            for (UINT8 portIdx = 0 ; portIdx < inputPortList.numPorts; portIdx++)
            {
                UINT8 camIdx = portIdx >> 1;

                CHX_LOG_INFO("portIndex:%d, camIdx:%d,isActive:%d",
                    portIdx, camIdx, ChxUtils::IsBitSet(pMCCResult->activeMap, camIdx));

                if (TRUE == ChxUtils::IsBitSet(pMCCResult->activeMap, camIdx))
                {
                    rEnabledPortList.push_back(inputPortList.pPorts[portIdx]);
                }
            }

            pInputPortList->numPorts = rEnabledPortList.size();
            pInputPortList->pPorts   = rEnabledPortList.data();
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::OnPopulateConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::OnPopulateConfiguration(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult             result      = CDKResultSuccess;
    ChiFeature2StageInfo  stageInfo   = { 0 };
    UINT8                 requestId   = 0;

    UINT8                                numRequestOutputs  = 0;
    const ChiFeature2RequestOutputInfo*  pRequestOutputInfo = NULL;

    requestId       = pRequestObject->GetCurRequestId();
    pRequestObject->GetCurrentStageInfo(&stageInfo, requestId);
    pRequestObject->GetExternalRequestOutput(&numRequestOutputs, &pRequestOutputInfo, requestId);

    if (InvalidStageId == stageInfo.stageId)
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2PortIdList outputPortList = { 0 };
        ChiFeature2PortIdList inputPortList  = { 0 };
        std::vector<ChiFeature2Identifier> enabledPortList;

        result = GetInputPortsForStage(stageInfo.stageId, &inputPortList);

        if (CDKResultSuccess == result)
        {
            result = BuildInputPortListBasedOnMCC(pRequestObject,
                stageInfo.stageId, &inputPortList, enabledPortList);
        }

        if (CDKResultSuccess == result)
        {
            result = GetOutputPortsForStage(stageInfo.stageId, &outputPortList);
        }

        if (CDKResultSuccess == result)
        {
            result = PopulatePortConfiguration(pRequestObject,
                &inputPortList, &outputPortList);
        }

        if (CDKResultSuccess == result)
        {
            CHIMETAHANDLE  hMetadata[2]   = {0};
            UINT8          phMetaIndex     = 0;

            if (CDKResultSuccess == result)
            {
                // Get input metadata
                for (UINT8 inputIndex = 0; inputIndex < inputPortList.numPorts; ++inputIndex)
                {
                    ChiFeature2Identifier portidentifier      = inputPortList.pPorts[inputIndex];
                    if (portidentifier.portType == ChiFeature2PortType::MetaData)
                    {
                        ChiFeature2BufferMetadataInfo inputBufferMetaInfo = { 0 };
                        pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputConfiguration,
                            &portidentifier, &inputBufferMetaInfo.hBuffer, &inputBufferMetaInfo.key, requestId, 0);
                        if (NULL != inputBufferMetaInfo.hBuffer)
                        {
                            CHIMETAHANDLE hMetadataBuffer = NULL;
                            result = GetMetadataBuffer(inputBufferMetaInfo.hBuffer,
                                inputBufferMetaInfo.key, &hMetadataBuffer);
                            if (NULL != hMetadataBuffer)
                            {
                                hMetadata[phMetaIndex] = hMetadataBuffer;
                                phMetaIndex ++;
                            }
                            else
                            {
                                CHX_LOG_ERROR("NULL input metadata");
                                result = CDKResultEInvalidPointer;
                            }
                        }
                        else
                        {
                            CHX_LOG_ERROR("NULL buffer handle");
                            result = CDKResultEInvalidPointer;
                        }
                    }
                }
            }

            // Callback to usecasemc to merge dualzone snapshot metadata
            if (CDKResultSuccess == result)
            {
                result = MergeSnapshotMetadata(
                    pRequestObject, hMetadata[0], hMetadata[1]);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::OnPopulateDependencySettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::OnPopulateDependencySettings(
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
    // By doing below fusion derived class change, we are setting,
    // "ANDROID_CONTROL_EFFECT_MODE_NEGATIVE" as dependency tag so
    // that it is used as input setting for buffers to be generated

    UINT32 mode = ANDROID_CONTROL_EFFECT_MODE_NEGATIVE;
    pFeatureSettings->SetTag(ANDROID_CONTROL_EFFECT_MODE, &mode, 1);

    */
    // EXAMPLE CODE END

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::OnPopulateConfigurationSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::OnPopulateConfigurationSettings(
    ChiFeature2RequestObject*     pRequestObject,
    const ChiFeature2Identifier*  pMetadataPortId,
    ChiMetadata*                  pInputMetadata
    ) const
{
    CDKResult result     = CDKResultSuccess;
    UINT8     requestId  = pRequestObject->GetCurRequestId();

    ChiMetadata* pUpstreamResultMetadata = GetExternalOutputSettings(pRequestObject, pMetadataPortId, requestId, 0);
    if (NULL != pUpstreamResultMetadata)
    {
        if (TRUE == IsMasterCamera(pUpstreamResultMetadata, m_pCameraInfo))
        {
            ChiFeature2Base::OnPopulateConfigurationSettings(pRequestObject, pMetadataPortId, pInputMetadata);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::MergeSnapshotMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::MergeSnapshotMetadata(
    ChiFeature2RequestObject* pRequestObject,
    CHIMETAHANDLE             hMetadata0,
    CHIMETAHANDLE             hMetadata1
    ) const
{
    CDKResult                        result                = CDKResultSuccess;
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObject = pRequestObject->GetUsecaseRequestObject();
    UINT32                           frameNum              = pUsecaseRequestObject->GetChiCaptureRequest()->frameNumber;
    UINT8                            requestId             = pRequestObject->GetCurRequestId();
    ChiFeatureSequenceData*          pRequestData          = static_cast<ChiFeatureSequenceData*>(
        pRequestObject->GetSequencePrivData(ChiFeature2SequenceOrder::Current, requestId));

    if ((NULL != pRequestData) && (NULL != pRequestData->pInputMetadata))
    {
        ChiMetadata* pInputMeta0 = GetMetadataManager()->GetMetadataFromHandle(hMetadata0);
        ChiMetadata* pInputMeta1 = GetMetadataManager()->GetMetadataFromHandle(hMetadata1);

        if ((NULL != pInputMeta0) && (NULL != pInputMeta1))
        {
            MulticamResultMetadata multiCamResultMetadata;
            multiCamResultMetadata.frameNum         = frameNum;
            multiCamResultMetadata.numResults       = 2;
            multiCamResultMetadata.ppResultMetadata =
                static_cast<ChiMetadata**>(CHX_CALLOC(DualCamCount * sizeof(ChiMetadata*)));

            if (NULL != multiCamResultMetadata.ppResultMetadata)
            {
                multiCamResultMetadata.ppResultMetadata[0] = pInputMeta0;
                multiCamResultMetadata.ppResultMetadata[1] = pInputMeta1;
            }
            else
            {
                CHX_LOG_ERROR("CALLOC failed for ppResultMetadata");
                CHX_ASSERT(NULL != multiCamResultMetadata.ppResultMetadata);
                result = CDKResultEFailed;
            }

            if (CDKResultSuccess == result)
            {
                if (static_cast<UINT32>(ChiFeature2Type::FUSION) == pRequestObject->GetFeature()->GetFeatureId())
                {
                    FillSATOfflinePipelineInputMetadata(&multiCamResultMetadata,
                        pRequestData->pInputMetadata);
                }
                else if (static_cast<UINT32>(ChiFeature2Type::BOKEH) == pRequestObject->GetFeature()->GetFeatureId())
                {
                    FillRTBOfflinePipelineInputMetadata(&multiCamResultMetadata,
                        pRequestData->pInputMetadata);
                }

                if (NULL != multiCamResultMetadata.ppResultMetadata)
                {
                    CHX_FREE(multiCamResultMetadata.ppResultMetadata);
                    multiCamResultMetadata.ppResultMetadata = NULL;
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("Input Metadata Null");
            result = CDKResultEInvalidPointer;
        }
    }
    else
    {
        result = CDKResultEInvalidPointer;
        CHX_LOG_ERROR("No request data or input metadata found");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2Fusion* pFeatureFusion = ChiFeature2Fusion::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeatureFusion);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoQueryCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DoQueryCaps(
    VOID*                 pConfig,
    ChiFeature2QueryInfo* pQueryInfo)
{
    CDK_UNUSED_PARAM(pConfig);
    CDKResult result = CDKResultSuccess;

    if (NULL != pQueryInfo)
    {
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2FusionCaps);
        pQueryInfo->ppCapabilities = &Feature2FusionCaps[0];
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
// ChiFeature2Fusion::OnPrepareInputSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::OnPrepareInputSettings(
    ChiMetadata*                  pInputMetadata,
    ChiMetadata*                  pUpstreamResultMetadata
    ) const
{
    CHX_LOG_INFO("merging master camera's OnPrepareInputSettings %p", pUpstreamResultMetadata);
    // Select master camera's metadata.
    CDKResult result = CDKResultSuccess;
    if ((NULL != pInputMetadata) && (NULL != pUpstreamResultMetadata))
    {
        CHITAGSOPS m_vendorTagOps;
        UINT32 m_vendorTagMasterInfo;
        INT32* pIsMaster;


        ExtensionModule::GetInstance()->GetVendorTagOps(&m_vendorTagOps);
        m_vendorTagOps.pQueryVendorTagLocation("com.qti.chi.multicamerainfo",
                "MasterCamera", &m_vendorTagMasterInfo);

        pIsMaster = static_cast<INT32*>(pUpstreamResultMetadata->GetTag(
                m_vendorTagMasterInfo));
        if (NULL != pIsMaster && TRUE == *pIsMaster)
        {
            CHX_LOG_INFO("copying master camera's meta");
            result = pInputMetadata->Copy(*pUpstreamResultMetadata, true);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Fusion::OnPreparePipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Fusion::OnPreparePipelineCreate(
    ChiFeature2Identifier* pKey)
{
    CDKResult result           = CDKResultSuccess;
    CameraConfigs cameraConfig = {0};
    ChiFeaturePipelineData* pPipelineData = GetPipelineData(pKey);
    if ((NULL != pPipelineData) && (NULL != pPipelineData->pPipeline))
    {
        cameraConfig.numPhysicalCameras   = m_pCameraInfo->numPhysicalCameras;
        cameraConfig.primaryCameraId      = m_pCameraInfo->primaryCameraId;

        for (UINT32 camindex = 0; camindex < m_pCameraInfo->numPhysicalCameras; camindex++)
        {
            CameraConfiguration* pDst            = &cameraConfig.cameraConfigs[camindex];
            DeviceInfo*          pSrc            = m_pCameraInfo->ppDeviceInfo[camindex];

            pDst->cameraId                = pSrc->cameraId;
            pDst->transitionZoomRatioLow  = pSrc->pDeviceConfig->transitionZoomRatioMin;
            pDst->transitionZoomRatioHigh = pSrc->pDeviceConfig->transitionZoomRatioMax;
            pDst->enableSmoothTransition  = pSrc->pDeviceConfig->enableSmoothTransition;
            pDst->sensorCaps              = pSrc->m_pDeviceCaps->sensorCaps;
            pDst->lensCaps                = pSrc->m_pDeviceCaps->lensCaps;
        }

        ChiMetadata* pMetadata = pPipelineData->pPipeline->GetDescriptorMetadata();
        result = pMetadata->SetTag("com.qti.chi.cameraconfiguration",
            "PhysicalCameraConfigs", &cameraConfig, sizeof(cameraConfig));

        if (CDKResultSuccess == result)
        {
            ChiPhysicalCameraConfig physicalCameraConfiguration;
            physicalCameraConfiguration.numConfigurations = m_pCameraInfo->numPhysicalCameras;
            ChiTargetPortDescriptorInfo* pSrcTarget       =
                &m_pUsecaseDesc->pPipelineTargetCreateDesc[pKey->pipeline].sourceTarget;

            for (UINT32 configIdx = 0; configIdx < m_pCameraInfo->numPhysicalCameras; configIdx++)
            {
                PhysicalCameraInputConfiguration* pInputConfig = &physicalCameraConfiguration.configuration[configIdx];
                pInputConfig->physicalCameraId                 = m_pCameraInfo->ppDeviceInfo[configIdx]->cameraId;
                pInputConfig->nodeDescriptor                   = pSrcTarget->pTargetPortDesc[configIdx].pNodePort[0];
                CHX_LOG_VERBOSE("map cameraid:%d, port:%d",
                    pInputConfig->physicalCameraId, pInputConfig->nodeDescriptor.nodePortId);
            }

            ChiMetadata* pMetadata = pPipelineData->pPipeline->GetDescriptorMetadata();
            result = pMetadata->SetTag("com.qti.chi.cameraconfiguration",
                                      "PhysicalCameraInputConfig",
                                      &physicalCameraConfiguration,
                                      sizeof(ChiPhysicalCameraConfig));
        }
        else
        {
            result = CDKResultEFailed;
            CHX_LOG_ERROR("Set PhysicalCameraConfigs failed");
        }
    }
    else
    {
        result = CDKResultEFailed;
        CHX_LOG_ERROR("pPipelineData or pipeline is NULL");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoStreamNegotiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DoStreamNegotiation(
    StreamNegotiationInfo*          pNegotiationInfo,
    StreamNegotiationOutput*        pNegotiationOutput)
{
    CDKResult result        = CDKResultSuccess;
    BOOL      isMultiCamera = (pNegotiationInfo->pLogicalCameraInfo->numPhysicalCameras <= 1) ? FALSE : TRUE;

    if ((NULL == pNegotiationInfo) || (pNegotiationOutput == NULL))
    {
        CHX_LOG_ERROR("Invalid Arg! pNegotiation=%p, pDesiredStreamConfig=%p",
            pNegotiationInfo, pNegotiationOutput);

        result = CDKResultEInvalidArg;
    }
    else
    {
        // Clone a stream and put it into the list of streams that will be freed by the feature
        auto CloneStream = [&pNegotiationOutput](CHISTREAM* pSrcCameraStream) -> CHISTREAM* {
            ChiStream* pStream = static_cast<CHISTREAM*>(CHX_CALLOC(sizeof(CHISTREAM)));
            ChxUtils::Memcpy(pStream, pSrcCameraStream, sizeof(CHISTREAM));
            pNegotiationOutput->pOwnedStreams->push_back(pStream);
            return pStream;
        };

        ChiFeature2Type featureId    = static_cast<ChiFeature2Type>(pNegotiationInfo->pFeatureInstanceId->featureId);
        const UINT      numStreams   = pNegotiationInfo->pFwkStreamConfig->numStreams;
        CHISTREAM**     ppChiStreams = pNegotiationInfo->pFwkStreamConfig->pChiStreams;

        CHISTREAM* pFusionInputStream     = NULL;
        CHISTREAM* pFusionOutputStream     = NULL;

        pNegotiationOutput->pStreams->clear();

        for (UINT8 streamIdx = 0; streamIdx < numStreams; streamIdx++)
        {
            if ((ppChiStreams[streamIdx]->streamType == ChiStreamTypeOutput) &&
                (NULL == ppChiStreams[streamIdx]->physicalCameraId))
            {
                pFusionOutputStream  = ppChiStreams[streamIdx];
            }
        }

        if (NULL != pFusionOutputStream)
        {
            pNegotiationOutput->pStreams->push_back(pFusionOutputStream);
        }
        else
        {
            CHX_LOG_ERROR("No out put stream");
            result = CDKResultENoSuch;
        }

        for (UINT8 cameraIndex = 0 ; cameraIndex < pNegotiationInfo->pLogicalCameraInfo->numPhysicalCameras ; ++cameraIndex)
        {
            for (UINT8 portIndex = 0 ; portIndex < MaxInputPortsPerCamera; ++portIndex)
            {
                UINT8 portId = portIndex + (MaxInputPortsPerCamera * cameraIndex);
                pNegotiationOutput->pInputPortMap->push_back(std::pair<UINT8, UINT8>(portId, cameraIndex));
            }

            CdkUtils::SNPrintF(physicalCameraIdName[cameraIndex],
                sizeof(physicalCameraIdName[cameraIndex]),
                "%d", pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[cameraIndex]->cameraId);

            if (TRUE == ExtensionModule::GetInstance()->EnableMultiCameraJPEG() && (featureId == ChiFeature2Type::FUSION))
            {
                for (UINT8 streamIdx = 0; streamIdx < numStreams; streamIdx++)
                {
                    if ((ppChiStreams[streamIdx]->streamType == ChiStreamTypeOutput) &&
                        (NULL != ppChiStreams[streamIdx]->physicalCameraId) &&
                        (ChiStreamFormatYCbCr420_888 == ppChiStreams[streamIdx]->format))
                    {
                        UINT8 streamCameraId = ChxUtils::GetCameraIdFromStream(ppChiStreams[streamIdx]);
                        UINT8 cameraId       = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[cameraIndex]->cameraId;

                        if (cameraId == streamCameraId)
                        {
                            pFusionInputStream = CloneStream(ppChiStreams[streamIdx]);
                            if (NULL != pFusionInputStream)
                            {
                                pFusionInputStream->streamType = ChiStreamTypeInput;
                                pNegotiationOutput->pStreams->push_back(pFusionInputStream);
                            }
                            break;
                        }
                    }
                }
            }
            else
            {
                pFusionInputStream = CloneStream(pFusionOutputStream);
                if (NULL != pFusionInputStream)
                {
                    pFusionInputStream->streamType       = ChiStreamTypeInput;
                    pFusionInputStream->physicalCameraId = physicalCameraIdName[cameraIndex];
                    pNegotiationOutput->pStreams->push_back(pFusionInputStream);
                }
                else
                {
                    CHX_LOG_ERROR("CloneStream failed");
                    result = CDKResultEFailed;
                }
            }
        }

        pNegotiationOutput->pDesiredStreamConfig->numStreams       = pNegotiationOutput->pStreams->size();
        pNegotiationOutput->pDesiredStreamConfig->operationMode    = pNegotiationInfo->pFwkStreamConfig->operationMode;
        pNegotiationOutput->pDesiredStreamConfig->pChiStreams      = &(*(pNegotiationOutput->pStreams))[0];
        pNegotiationOutput->pDesiredStreamConfig->pSessionSettings = NULL;

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
