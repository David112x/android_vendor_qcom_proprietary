////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2rawhdr.cpp
/// @brief rawhdr feature2 implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chxdefs.h"
#include "chifeature2rawhdr.h"
#include "chifeature2utils.h"
#include "chifeature2featurepool.h"

static const UINT32 Feature2MajorVersion  = 0;
static const UINT32 Feature2MinorVersion  = 1;
static const CHAR*  VendorName            = "QTI";

/// @brief This encapsulates RawHDR information
struct ChiFeature2RawHDRFeatureInfo
{
    ChiFeature2BufferMetadataInfo inputBufferInfo[BufferQueueDepth];  ///< Input Buffer info
    ChiFeature2BufferMetadataInfo inputMetaInfo[BufferQueueDepth];    ///< Input Metahandle info
    UINT32                        rawHDRTotalNumFrames;               ///< number of frames needed for RawHDR
};

static CHISTREAM RawHDRStreamsInput
{
    ChiStreamTypeInput,
    4608,
    3456,
    ChiStreamFormatRaw10,   // Raw10 not allocating enough buffer space for the image (needs 2 bytes per pixel).
    // Image might have 6 bytes of padding.
    GRALLOC1_PRODUCER_USAGE_CAMERA |
    GRALLOC1_PRODUCER_USAGE_CPU_READ |
    GRALLOC1_PRODUCER_USAGE_CPU_WRITE,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
#if (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
    NULL,
#endif // (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)
    StreamParams,
    { NULL, NULL, NULL },

};

static CHISTREAM RawHDRStreamsOutput
{
    ChiStreamTypeOutput,
    4608,
    3456,
    ChiStreamFormatRaw10,
    0,
    8,
    NULL,
    DataspaceUnknown,
    StreamRotationCCW0,
    NULL,
#if (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28) // Android-P or better
    NULL,
#endif // (CAMX_ANDROID_API) && (CAMX_ANDROID_API >= 28)
    StreamParams,
    { NULL, NULL, NULL },
};

static const CHAR* Feature2RawHDRCaps[] =
{
    "RawHDR"
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RawHDR::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RawHDR * ChiFeature2RawHDR::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2RawHDR* pFeature = CHX_NEW(ChiFeature2RawHDR);
    CDKResult          result   = CDKResultSuccess;

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
            pFeature->m_pFeatureInputPortDescriptors = &RawHDRInputPortDescriptors[0];
        }
    }
    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RawHDR::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RawHDR::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RawHDR::~ChiFeature2RawHDR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RawHDR::~ChiFeature2RawHDR()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RawHDR::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RawHDR::DoFlush()
{
    CDKResult result = CDKResultSuccess;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RawHDR::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RawHDR::OnPrepareRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pRequestObject)
    {
        ChiFeature2Hint* pFeatureHint = pRequestObject->GetFeatureHint();
        ChiFeature2RawHDRFeatureInfo* pRawHDRFeatureInfo = static_cast<ChiFeature2RawHDRFeatureInfo*>(CHX_CALLOC(
            sizeof(ChiFeature2RawHDRFeatureInfo)));

        if (NULL == pRawHDRFeatureInfo)
        {
            CHX_LOG_ERROR("Error Allocating memory");
            result = CDKResultENoMemory;
        }
        else
        {
            if ((NULL != pFeatureHint) && (0 < pFeatureHint->numFrames))
            {
                pRawHDRFeatureInfo->rawHDRTotalNumFrames = pFeatureHint->numFrames;
            }
            else
            {
                pRawHDRFeatureInfo->rawHDRTotalNumFrames = 3;
            }
            CHX_LOG_INFO("Number of frames to process %d", pRawHDRFeatureInfo->rawHDRTotalNumFrames);

            SetFeaturePrivContext(pRequestObject, pRawHDRFeatureInfo);
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
// ChiFeature2RawHDR::OnPopulateDependencySettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RawHDR::OnPopulateDependencySettings(
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
    ANDROID_CONTROL_ENABLE_ZSL_FALSE};
    const UINT32 zslArraySize = CHX_ARRAY_SIZE(ZSLArray);

    INT32 livePreview                 = 0;
    INT32 livePreviewEnable[]         = { 0, 1, 1, 1, 1 };
    const UINT32 livePreviewArraySize = CHX_ARRAY_SIZE(livePreviewEnable);

    if ((CDKResultSuccess == result) && (NULL != pFeatureSettings))
    {
        if (dependencyIndex < expArraySize)
        {
            exposure = expArray[dependencyIndex];
        }
        if (dependencyIndex < zslArraySize)
        {
            controlZSLValue = ZSLArray[dependencyIndex];
        }
        if (dependencyIndex < livePreviewArraySize)
        {
            livePreview = livePreviewEnable[dependencyIndex];
        }
        pFeatureSettings->SetTag(ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &exposure, 1);
        pFeatureSettings->SetTag(ANDROID_CONTROL_ENABLE_ZSL, &controlZSLValue, 1);
        // ChxUtils::SetVendorTagValue(pFeatureSettings, VendorTag::LivePreview, 1, &livePreview);
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RawHDR::OnSelectFlowToExecuteRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RequestFlowType ChiFeature2RawHDR::OnSelectFlowToExecuteRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNREFERENCED_PARAM(pRequestObject);
    return ChiFeature2RequestFlowType::Type1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RawHDR::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RawHDR::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDK_UNUSED_PARAM(pRequestObject);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CreateFeature
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* CreateFeature(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2RawHDR* pFeatureRawHDR = ChiFeature2RawHDR::Create(pCreateInputInfo);
    return static_cast<VOID*>(pFeatureRawHDR);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DoQueryCaps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult DoQueryCaps(
    VOID*                  pConfig,
    ChiFeature2QueryInfo*  pQueryInfo)
{
    CDK_UNUSED_PARAM(pConfig);
    CDKResult result = CDKResultSuccess;

    if (NULL != pQueryInfo)
    {
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2RawHDRCaps);
        pQueryInfo->ppCapabilities = &Feature2RawHDRCaps[0];
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

        ChiFeature2Type featureId       = static_cast<ChiFeature2Type>(pNegotiationInfo->pFeatureInstanceId->featureId);
        const UINT      numStreams      = pNegotiationInfo->pFwkStreamConfig->numStreams;
        CHISTREAM**     ppChiStreams    = pNegotiationInfo->pFwkStreamConfig->pChiStreams;
        UINT8           physicalCamIdx  = pNegotiationInfo->pFeatureInstanceId->cameraId;
        UINT32          physicalCamId   = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[physicalCamIdx]->cameraId;

        GetSensorOutputDimension(physicalCamId,
                                 pNegotiationInfo->pFwkStreamConfig,
                                 pNegotiationInfo->pFeatureInstanceId->flags,
                                 &RDIStream.width,
                                 &RDIStream.height);

        CHISTREAM* pRawHDRInput1      = NULL;
        CHISTREAM* pRawHDROutput      = NULL;


        pNegotiationOutput->pStreams->clear();

        pRawHDRInput1 = CloneStream(&RawHDRStreamsInput);
        if (NULL != pRawHDRInput1)
        {
            if ((0 < RDIStream.height) && (0 < RDIStream.width))
            {
                pRawHDRInput1->width   = RDIStream.width;
                pRawHDRInput1->height  = RDIStream.height;
            }
            pNegotiationOutput->pStreams->push_back(pRawHDRInput1);
        }

        pRawHDROutput = CloneStream(&RawHDRStreamsOutput);

        if (NULL != pRawHDROutput)
        {
            if ((0 < RDIStream.height) && (0 < RDIStream.width))
            {
                pRawHDROutput->width   = RDIStream.width;
                pRawHDROutput->height  = RDIStream.height;
            }
            pNegotiationOutput->pStreams->push_back(pRawHDROutput);
        }

        // configure desired stream
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
