////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2memcpy.cpp
/// @brief memcpy feature2 implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chxdefs.h"
#include "chifeature2memcpy.h"
#include "chifeature2utils.h"
#include "chifeature2featurepool.h"

// NOWHINE FILE CP006:  Used standard libraries for performance improvements

static const UINT32 Feature2MajorVersion  = 0;
static const UINT32 Feature2MinorVersion  = 1;
static const CHAR*  VendorName            = "QTI";

static const CHAR*  Feature2MemcpyCaps[] =
{
    "Memcpy"
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Memcpy::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Memcpy * ChiFeature2Memcpy::Create(
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2Memcpy* pFeature = CHX_NEW(ChiFeature2Memcpy);
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
    }

    return pFeature;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Memcpy::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Memcpy::Destroy()
{
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Memcpy::~ChiFeature2Memcpy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Memcpy::~ChiFeature2Memcpy()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Memcpy::DoFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Memcpy::DoFlush()
{
    CDKResult result = CDKResultSuccess;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Memcpy::OnPrepareRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Memcpy::OnPrepareRequest(
    ChiFeature2RequestObject * pRequestObject
    ) const
{
    CDKResult result = CDKResultSuccess;

    CDK_UNUSED_PARAM(pRequestObject);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Memcpy::OnExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Memcpy::OnExecuteProcessRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult result               = CDKResultSuccess;
    ChiFeature2StageInfo stageInfo = { 0 };
    UINT8     stageId              = InvalidStageId;

    result = GetCurrentStageInfo(pRequestObject, &stageInfo);
    if (CDKResultSuccess == result)
    {
        stageId = stageInfo.stageId;
        if (InvalidStageId != stageId)
        {
            // Get the input buffer and set it to the output port where another feature is connected
            ChiFeature2BufferMetadataInfo      inputBufferInfo   = { 0 };
            ChiFeature2BufferMetadataInfo      inputMetaInfo     = { 0 };
            ChiFeature2BufferMetadataInfo*     pFinalBufferMeta  = NULL;
            ChiFeature2PortIdList              outputPortList    = { 0 };
            std::vector<ChiFeature2Identifier> extPorts;
            ChiFeature2Identifier              portId            = { 0 };

            portId = MemcpyInputPortDescriptors[0].globalId;
            pRequestObject->GetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                                          &portId,
                                          &inputBufferInfo.hBuffer,
                                          &inputBufferInfo.key,
                                          pRequestObject->GetCurRequestId(),
                                          0);

            // Buffer
            portId = MemcpyOutputPortDescriptors[2].globalId;
            pRequestObject->GetFinalBufferMetadataInfo(portId,
                                                       &pFinalBufferMeta,
                                                       pRequestObject->GetCurRequestId());
            // pFinalBufferMeta will be NULL if FD output port is not connected to downstream feature.

            if (NULL != pFinalBufferMeta)
            {
                pFinalBufferMeta->key     = inputBufferInfo.key;
                pFinalBufferMeta->hBuffer = inputBufferInfo.hBuffer;
                extPorts.push_back(MemcpyOutputPortDescriptors[2].globalId);

                CHX_LOG_INFO("Send buffer and metadata message key %" PRIu64 ", hbuffer %p",
                             inputBufferInfo.key, inputBufferInfo.hBuffer);

                // Send buffer result message
                ChiFeature2MessageDescriptor bufferResultMsg;
                ChxUtils::Memset(&bufferResultMsg, 0, sizeof(bufferResultMsg));
                bufferResultMsg.messageType                = ChiFeature2MessageType::ResultNotification;
                bufferResultMsg.message.result.numPorts    = extPorts.size();
                bufferResultMsg.message.result.pPorts      = extPorts.data();
                bufferResultMsg.message.result.resultIndex = pRequestObject->GetCurRequestId();
                bufferResultMsg.pPrivData                  = pRequestObject;
                ProcessFeatureMessage(&bufferResultMsg);
            }
        }
    }

    // Process the framework YUV cb as it requires memcpy
    result = ChiFeature2Base::OnExecuteProcessRequest(pRequestObject);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Memcpy::OnPopulateConfiguration
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Memcpy::OnPopulateConfiguration(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDKResult             result = CDKResultSuccess;
    ChiFeature2StageInfo  stageInfo = { 0 };
    UINT8                 requestId = 0;

    UINT8                                numRequestOutputs = 0;
    const ChiFeature2RequestOutputInfo*  pRequestOutputInfo = NULL;

    requestId = pRequestObject->GetCurRequestId();
    pRequestObject->GetCurrentStageInfo(&stageInfo, requestId);
    pRequestObject->GetExternalRequestOutput(&numRequestOutputs, &pRequestOutputInfo, requestId);

    if (InvalidStageId == stageInfo.stageId)
    {
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2PortIdList inputPortList = { 0 };
        ChiFeature2PortIdList outputPortList = { 0 };

        result = GetInputPortsForStage(stageInfo.stageId, &inputPortList);

        if (CDKResultSuccess == result)
        {
            result = GetOutputPortsForStage(stageInfo.stageId, &outputPortList);
        }

        if (CDKResultSuccess == result)
        {
            std::vector<ChiFeature2Identifier> filteredList;

            for (UINT8 portIndex = 0; portIndex < outputPortList.numPorts; portIndex++)
            {
                ChiFeature2Identifier portId = outputPortList.pPorts[portIndex];

                // The first port needs the memcpy
                if (portIndex < 2)
                {
                    filteredList.push_back(portId);
                }
            }

            ChiFeature2PortIdList filtered = { 0 };

            filtered.numPorts = filteredList.size();
            filtered.pPorts   = filteredList.data();

            result = PopulatePortConfiguration(pRequestObject,
                                               &inputPortList, &filtered);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Memcpy::OnSelectFlowToExecuteRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RequestFlowType ChiFeature2Memcpy::OnSelectFlowToExecuteRequest(
    ChiFeature2RequestObject* pRequestObject
    ) const
{
    CDK_UNREFERENCED_PARAM(pRequestObject);
    return ChiFeature2RequestFlowType::Type0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Memcpy::DoCleanupRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Memcpy::DoCleanupRequest(
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
    ChiFeature2Memcpy* pFeaturememcpy = ChiFeature2Memcpy::Create(pCreateInputInfo);
    return static_cast<CHIHANDLE>(pFeaturememcpy);
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
        pQueryInfo->numCaps        = CHX_ARRAY_SIZE(Feature2MemcpyCaps);
        pQueryInfo->ppCapabilities = &Feature2MemcpyCaps[0];
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

        CHISTREAM* pMemcpyInput      = NULL;
        CHISTREAM* pMemcpyOutput     = NULL;

        pNegotiationOutput->pStreams->clear();
        ChiStream fdStream  = UsecaseSelector::GetFDOutStream();
        pMemcpyOutput       = CloneStream(&fdStream);

        if (NULL != pMemcpyOutput)
        {
            pNegotiationOutput->pStreams->push_back(pMemcpyOutput);
        }

        fdStream.streamType = ChiStreamTypeInput;
        pMemcpyInput        = CloneStream(&fdStream);

        if (NULL != pMemcpyInput)
        {
            pNegotiationOutput->pStreams->push_back(pMemcpyInput);
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
