////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2hdr.cpp
/// @brief HDR feature2 implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2hdr.h"
#include "chifeature2utils.h"
#include "chifeature2featurepool.h"
#include "chxdefs.h"

static const UINT32 Feature2MajorVersion  = 0;
static const UINT32 Feature2MinorVersion  = 1;
static const CHAR*  VendorName            = "QTI";

static const CHAR*  Feature2HDRCaps[] =
{
    "HDR"
};

/// @brief This encapsulates HDR information
struct ChiFeature2HDRFeatureInfo
{
    ChiFeature2BufferMetadataInfo inputBufferInfo[BufferQueueDepth];  ///< Input Buffer info
    ChiFeature2BufferMetadataInfo inputMetaInfo[BufferQueueDepth];    ///< Input Metahandle info
    UINT32                        HDRTotalNumFrames;                  ///< number of frames needed for HDRT1
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HDR::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2HDR * ChiFeature2HDR::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2HDR* pFeature    = CHX_NEW(ChiFeature2HDR);
    CDKResult       result      = CDKResultSuccess;

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
            pFeature->m_pFeatureInputPortDescriptors = &HDRT1FeatureInputPortDescriptors[0];
        }
    }

    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HDR::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2HDR::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HDR::~ChiFeature2HDR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2HDR::~ChiFeature2HDR()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HDR::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HDR::DoFlush()
{
    CDKResult result = CDKResultSuccess;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HDR::OnProcessingDependenciesComplete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2HDR::OnProcessingDependenciesComplete(
    ChiFeature2RequestObject* pRequestObject,
    UINT8                     requestId)
{
    // If all of our ports are done processing and our request is the last request (applicable to batched requests)
    // then send the notification
    if (FALSE == pRequestObject->GetOutputPortsProcessingCompleteNotified())
    {
        if ((TRUE == pRequestObject->AllOutputPortsProcessingComplete()) &&
            ((pRequestObject->GetNumRequests() - 1) == requestId))
        {
            // If this is the final stage sequence, notify the graph that the feature's
            // processing is complete on this port
            ChiFeature2StageInfo    stageInfo   = { 0 };
            ChiFeature2Hint*        pHint       = pRequestObject->GetFeatureHint();

            GetCurrentStageInfo(pRequestObject, &stageInfo);

            if (NULL != pHint)
            {
                if (TRUE == IsRequestInFinalStageSequence(stageInfo, pHint))
                {
                    CHX_LOG_VERBOSE("%s All ports are done processing; generating processing dependencies complete"
                                    " feature message",
                                    pRequestObject->IdentifierString());

                    ChiFeature2MessageDescriptor featurePortMessage;

                    ChxUtils::Memset(&featurePortMessage, 0, sizeof(ChiFeature2MessageDescriptor));
                    featurePortMessage.messageType  = ChiFeature2MessageType::ProcessingDependenciesComplete;
                    featurePortMessage.pPrivData    = pRequestObject;
                    pRequestObject->SetCurRequestId(requestId);
                    ProcessFeatureMessage(&featurePortMessage);
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HDR::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HDR::OnPrepareRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pRequestObject)
    {
        ChiFeature2Hint* pFeatureHint = pRequestObject->GetFeatureHint();
        ChiFeature2HDRFeatureInfo* pHDRFeatureInfo =
            static_cast<ChiFeature2HDRFeatureInfo*>(CHX_CALLOC(sizeof(ChiFeature2HDRFeatureInfo)));

        if (NULL == pHDRFeatureInfo)
        {
            CHX_LOG_ERROR("Error Allocating memory");
            result = CDKResultENoMemory;
        }
        else
        {
            if ((NULL != pFeatureHint) && (0 < pFeatureHint->numFrames))
            {
                pHDRFeatureInfo->HDRTotalNumFrames = pFeatureHint->numFrames;
            }
            else
            {
                pHDRFeatureInfo->HDRTotalNumFrames = 3;
            }
            CHX_LOG_INFO("Number of frames to process %d", pHDRFeatureInfo->HDRTotalNumFrames);
            SetFeaturePrivContext(pRequestObject, pHDRFeatureInfo);
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid Request Object");
        result = CDKResultEInvalidState;
    }

    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HDR::OnPopulateDependencySettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HDR::OnPopulateDependencySettings(
    ChiFeature2RequestObject*     pRequestObject,
    UINT8                         dependencyIndex,
    const ChiFeature2Identifier*  pSettingPortId,
    ChiMetadata*                  pFeatureSettings
    ) const
{
    CDKResult result = ChiFeature2Base::OnPopulateDependencySettings(
        pRequestObject, dependencyIndex, pSettingPortId, pFeatureSettings);

    INT32 exposure         = 0;
    UINT32 controlZSLValue = ANDROID_CONTROL_ENABLE_ZSL_FALSE;

    const INT expArray[]    = { 0, -6, 6, -12, 12 };
    const INT expArraySize  = CHX_ARRAY_SIZE(expArray);

    const UINT32 ZSLArray[]   = {ANDROID_CONTROL_ENABLE_ZSL_TRUE,  ANDROID_CONTROL_ENABLE_ZSL_FALSE,
        ANDROID_CONTROL_ENABLE_ZSL_FALSE, ANDROID_CONTROL_ENABLE_ZSL_FALSE,
    ANDROID_CONTROL_ENABLE_ZSL_FALSE, ANDROID_CONTROL_ENABLE_ZSL_FALSE};
    const UINT32 zslArraySize = CHX_ARRAY_SIZE(ZSLArray);

    if (NULL != pFeatureSettings)
    {
        if (CDKResultSuccess == result)
        {
            if (dependencyIndex < expArraySize)
            {
                exposure = expArray[dependencyIndex];
            }

            if (dependencyIndex < zslArraySize)
            {
                controlZSLValue = ZSLArray[dependencyIndex];
            }

            pFeatureSettings->SetTag(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &exposure, 1);
            pFeatureSettings->SetTag(ANDROID_CONTROL_ENABLE_ZSL, &controlZSLValue, 1);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HDR::OnSelectFlowToExecuteRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RequestFlowType ChiFeature2HDR::OnSelectFlowToExecuteRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNREFERENCED_PARAM(pRequestObject);
    return ChiFeature2RequestFlowType::Type1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2HDR::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2HDR::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pRequestObject)
    {
        ChiFeature2HDRFeatureInfo* pHDRFeatureInfo =
            static_cast<ChiFeature2HDRFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

        if (NULL != pHDRFeatureInfo)
        {
            ChxUtils::Free(pHDRFeatureInfo);
            pHDRFeatureInfo = NULL;
            SetFeaturePrivContext(pRequestObject, NULL);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2HDR* pFeaturehdr = ChiFeature2HDR::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeaturehdr);
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
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2HDRCaps);
        pQueryInfo->ppCapabilities = &Feature2HDRCaps[0];
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
    CDKResult result        = CDKResultSuccess;

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

        const UINT  numStreams     = pNegotiationInfo->pFwkStreamConfig->numStreams;
        CHISTREAM** ppChiStreams   = pNegotiationInfo->pFwkStreamConfig->pChiStreams;
        UINT8       physicalCamIdx = pNegotiationInfo->pFeatureInstanceId->cameraId;
        UINT32      physicalCamId  = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[physicalCamIdx]->cameraId;

        UINT32               snapshotWidth      = 0;
        UINT32               snapshotHeight     = 0;
        SnapshotStreamConfig snapshotConfig     = {};
        CHISTREAM*           pYUVSnapshotStream = NULL;
        BOOL                 isNativeResolution = FALSE;

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

        result = UsecaseSelector::GetSnapshotStreamConfiguration(numStreams, ppChiStreams, snapshotConfig);

        if (NULL != snapshotConfig.pSnapshotStream)
        {
            snapshotWidth  = snapshotConfig.pSnapshotStream->width;
            snapshotHeight = snapshotConfig.pSnapshotStream->height;
        }

        if (snapshotWidth * snapshotHeight == 0)
        {
            pYUVSnapshotStream            = GetYUVSnapshotStream(pNegotiationInfo->pFwkStreamConfig);
            if (pYUVSnapshotStream != NULL)
            {
                snapshotWidth  = pYUVSnapshotStream->width;
                snapshotHeight = pYUVSnapshotStream->height;
            }
        }

        // For multi camera, there is just yuv stream configure
        if (pNegotiationInfo->pLogicalCameraInfo->numPhysicalCameras > 1)
        {
            UINT8    physicalCamIdx  = pNegotiationInfo->pFeatureInstanceId->cameraId;
            UINT32   physicalCamId   = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[physicalCamIdx]->cameraId;
            for (UINT streamIdx = 0; streamIdx < numStreams; streamIdx++)
            {
                if ((FALSE == IsFDStream(ppChiStreams[streamIdx])))
                {
                    if ((ppChiStreams[streamIdx]->streamType == ChiStreamTypeOutput) &&
                        ((NULL == ppChiStreams[streamIdx]->physicalCameraId) ||
                         ((NULL != ppChiStreams[streamIdx]->physicalCameraId) &&
                          ((0 == strlen(ppChiStreams[streamIdx]->physicalCameraId)) ||
                           (ChxUtils::GetCameraIdFromStream(ppChiStreams[streamIdx]) == physicalCamId)))))
                    {
                        snapshotWidth      = ppChiStreams[streamIdx]->width;
                        snapshotHeight     = ppChiStreams[streamIdx]->height;
                        pYUVSnapshotStream = ppChiStreams[streamIdx];
                        break;
                    }
                }
            }
        }

        CHISTREAM* pHDRInput1      = NULL;
        CHISTREAM* pHDROutput      = NULL;
        pNegotiationOutput->pStreams->clear();

        pHDRInput1 = CloneStream(&HDRT1StreamsInput1);
        if (NULL != pHDRInput1)
        {
            if (TRUE == isNativeResolution)
            {
                pHDRInput1->width  = RDIStream.width;
                pHDRInput1->height = RDIStream.height;
            }
            else
            {
                pHDRInput1->width  = snapshotWidth;
                pHDRInput1->height = snapshotHeight;
            }
            pNegotiationOutput->pStreams->push_back(pHDRInput1);
        }

        pHDROutput = CloneStream(&HDRT1StreamsOutput1);

        if (NULL != pHDROutput)
        {
            if (TRUE == isNativeResolution)
            {
                pHDROutput->width  = RDIStream.width;
                pHDROutput->height = RDIStream.height;
                pNegotiationOutput->pStreams->push_back(pHDROutput);
            }
            else if (NULL != pYUVSnapshotStream)
            {
                pNegotiationOutput->pStreams->push_back(pYUVSnapshotStream);
            }
            else
            {
                pHDROutput->width  = snapshotWidth;
                pHDROutput->height = snapshotHeight;
                pNegotiationOutput->pStreams->push_back(pHDROutput);
            }
        }

        // configure desired stream
        pNegotiationOutput->pDesiredStreamConfig->numStreams         = pNegotiationOutput->pStreams->size();
        pNegotiationOutput->pDesiredStreamConfig->operationMode      = pNegotiationInfo->pFwkStreamConfig->operationMode;
        pNegotiationOutput->pDesiredStreamConfig->pChiStreams        = &(*(pNegotiationOutput->pStreams))[0];
        pNegotiationOutput->pDesiredStreamConfig->pSessionSettings   = NULL;
        pNegotiationOutput->disableZoomCrop                          = isNativeResolution;
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
        pChiFeature2Ops->size               = sizeof(CHIFEATURE2OPS);
        pChiFeature2Ops->majorVersion       = Feature2MajorVersion;
        pChiFeature2Ops->minorVersion       = Feature2MinorVersion;
        pChiFeature2Ops->pVendorName        = VendorName;
        pChiFeature2Ops->pCreate            = CreateFeature;
        pChiFeature2Ops->pQueryCaps         = DoQueryCaps;
        pChiFeature2Ops->pGetVendorTags     = GetVendorTags;
        pChiFeature2Ops->pStreamNegotiation = DoStreamNegotiation;
    }
}
#ifdef __cplusplus
}
#endif // __cplusplus
