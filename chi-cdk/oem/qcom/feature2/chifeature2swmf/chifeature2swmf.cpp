////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2swmf.cpp
/// @brief swmf feature2 implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chxdefs.h"
#include "chifeature2swmf.h"
#include "chifeature2utils.h"
#include "chifeature2featurepool.h"

static const UINT32 Feature2MajorVersion  = 0;
static const UINT32 Feature2MinorVersion  = 1;
static const CHAR*  VendorName            = "QTI";

static const CHAR*  Feature2SWMFCaps[] =
{
    "SWMF"
};

/// @brief This encapsulates SWMF information
struct ChiFeature2SWMFFeatureInfo
{
    ChiFeature2BufferMetadataInfo inputBufferInfo[BufferQueueDepth];  ///< Input Buffer info
    ChiFeature2BufferMetadataInfo inputMetaInfo[BufferQueueDepth];    ///< Input Metahandle info
    UINT32                        swmfTotalNumFrames;                 ///< number of frames needed for SWMF
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2SWMultiframe::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2SWMultiframe * ChiFeature2SWMultiframe::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2SWMultiframe*    pFeature    = CHX_NEW(ChiFeature2SWMultiframe);
    CDKResult                   result      = CDKResultSuccess;

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
            pFeature->m_pFeatureInputPortDescriptors = &SWMFFeatureInputPortDescriptors[0];
        }
    }

    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2SWMultiframe::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2SWMultiframe::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2SWMultiframe::~ChiFeature2SWMultiframe
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2SWMultiframe::~ChiFeature2SWMultiframe()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2SWMultiframe::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2SWMultiframe::DoFlush()
{
    CDKResult result = CDKResultSuccess;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2SWMultiframe::OnProcessingDependenciesComplete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2SWMultiframe::OnProcessingDependenciesComplete(
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
// ChiFeature2SWMultiframe::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2SWMultiframe::OnPrepareRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pRequestObject)
    {
        ChiFeature2Hint* pFeatureHint = pRequestObject->GetFeatureHint();

        ChiFeature2SWMFFeatureInfo* pSWMFFeatureInfo =
            static_cast<ChiFeature2SWMFFeatureInfo*>(CHX_CALLOC(sizeof(ChiFeature2SWMFFeatureInfo)));

        if (NULL == pSWMFFeatureInfo)
        {
            CHX_LOG_ERROR("Error Allocating memory");
            result = CDKResultENoMemory;
        }
        else
        {
            if ((NULL != pFeatureHint) && (0 < pFeatureHint->numFrames))
            {
                pSWMFFeatureInfo->swmfTotalNumFrames = pFeatureHint->numFrames;
            }
            else
            {
                pSWMFFeatureInfo->swmfTotalNumFrames = 5;
            }
            CHX_LOG_INFO("Number of frames to process %d", pSWMFFeatureInfo->swmfTotalNumFrames);
            SetFeaturePrivContext(pRequestObject, pSWMFFeatureInfo);
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
// ChiFeature2SWMultiframe::OnSelectFlowToExecuteRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RequestFlowType ChiFeature2SWMultiframe::OnSelectFlowToExecuteRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNREFERENCED_PARAM(pRequestObject);
    return ChiFeature2RequestFlowType::Type1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2SWMultiframe::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2SWMultiframe::DoCleanupRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL != pRequestObject)
    {
        ChiFeature2SWMFFeatureInfo* pSWMFFeatureInfo =
            static_cast<ChiFeature2SWMFFeatureInfo*>(GetFeaturePrivContext(pRequestObject));

        if (NULL != pSWMFFeatureInfo)
        {
            ChxUtils::Free(pSWMFFeatureInfo);
            pSWMFFeatureInfo = NULL;
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
    ChiFeature2SWMultiframe* pFeatureswmf = ChiFeature2SWMultiframe::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeatureswmf);
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
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2SWMFCaps);
        pQueryInfo->ppCapabilities = &Feature2SWMFCaps[0];
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

        ChiFeature2Type featureId   = static_cast<ChiFeature2Type>(pNegotiationInfo->pFeatureInstanceId->featureId);
        const UINT   numStreams     = pNegotiationInfo->pFwkStreamConfig->numStreams;
        CHISTREAM**  ppChiStreams   = pNegotiationInfo->pFwkStreamConfig->pChiStreams;
        UINT8        physicalCamIdx = pNegotiationInfo->pFeatureInstanceId->cameraId;
        UINT32       physicalCamId  = pNegotiationInfo->pLogicalCameraInfo->ppDeviceInfo[physicalCamIdx]->cameraId;

        GetSensorOutputDimension(physicalCamId,
                                pNegotiationInfo->pFwkStreamConfig,
                                pNegotiationInfo->pFeatureInstanceId->flags,
                                &RDIStream.width,
                                &RDIStream.height);

        UINT32               snapshotWidth      = 0;
        UINT32               snapshotHeight     = 0;
        SnapshotStreamConfig snapshotConfig     = {};
        CHISTREAM*           pYUVSnapshotStream = NULL;

        result = UsecaseSelector::GetSnapshotStreamConfiguration(numStreams, ppChiStreams, snapshotConfig);

        if (NULL != snapshotConfig.pSnapshotStream)
        {
            snapshotWidth  = snapshotConfig.pSnapshotStream->width;
            snapshotHeight = snapshotConfig.pSnapshotStream->height;
        }

        if (snapshotWidth * snapshotHeight == 0)
        {
            pYUVSnapshotStream            = GetYUVSnapshotStream(pNegotiationInfo->pFwkStreamConfig);
            if (NULL != pYUVSnapshotStream )
            {
                snapshotWidth  = pYUVSnapshotStream->width;
                snapshotHeight = pYUVSnapshotStream->height;
            }
        }


        CHISTREAM* pSWMFInput1      = NULL;
        CHISTREAM* pSWMFOutput      = NULL;

        pNegotiationOutput->pStreams->clear();

        pSWMFInput1 = CloneStream(&HDRT1StreamsInput1);
        if (NULL != pSWMFInput1)
        {
            pSWMFInput1->width  = snapshotWidth;
            pSWMFInput1->height = snapshotHeight;
            pNegotiationOutput->pStreams->push_back(pSWMFInput1);
        }

        pSWMFOutput = CloneStream(&HDRT1StreamsOutput1);

        if (NULL != pSWMFOutput)
        {
            pSWMFOutput->width  = snapshotWidth;
            pSWMFOutput->height = snapshotHeight;
            if (NULL != pYUVSnapshotStream)
            {
                pNegotiationOutput->pStreams->push_back(pYUVSnapshotStream);
            }
            else
            {
                pNegotiationOutput->pStreams->push_back(pSWMFOutput);
            }
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
