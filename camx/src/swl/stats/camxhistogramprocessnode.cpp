////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxhistogramprocessnode.cpp
/// @brief Histogram Process node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxhistogramprocessnode.h"
#include "camxhwdefs.h"
#include "camxpipeline.h"
#include "camxpropertyblob.h"
#include "camxtrace.h"
#include "camxstatscommon.h"
#include "camxvendortags.h"

#include "camxutils.h"
#include "iqcommondefs.h"
#include "chihistalgointerface.h"

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::HistogramProcessNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HistogramProcessNode::HistogramProcessNode()
    : m_perFrameInfo( { 0 } )
{
    CAMX_LOG_INFO(CamxLogGroupHist, "HistogramProcessNode()");

    m_pNodeName = "HistogramProcess";
    m_derivedNodeHandlesMetaDone = TRUE;
    m_pHDRAlgorithmHandler       = NULL;
    m_pHDRAlgorithm              = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::~HistogramProcessNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HistogramProcessNode::~HistogramProcessNode()
{
    const StaticSettings* pStaticSettings = HwEnvironment::GetInstance()->GetStaticSettings();
    CAMX_ASSERT_MESSAGE(NULL != pStaticSettings, "pStaticSettings NULL pointer");

    StatsCameraInfo cameraInfo = m_cameraInfo;

    // Destroy all the created objects.
    if (NULL != m_pHDRAlgorithm)
    {
        HistAlgoProcessDestroyParamList destroyParamList                                           = { 0 };
        HistAlgoProcessDestroyParam     destroyParams[HistAlgoProcessDestroyParamTypeCount]   = {};

        UINT  overrideCameraClose                                                           =
            pStaticSettings->overrideCameraClose;
        destroyParams[HistAlgoProcessDestroyParamTypeCameraCloseIndicator].destroyParamType =
            HistAlgoProcessDestroyParamTypeCameraCloseIndicator;
        destroyParams[HistAlgoProcessDestroyParamTypeCameraCloseIndicator].pParam           = &overrideCameraClose;
        destroyParams[HistAlgoProcessDestroyParamTypeCameraCloseIndicator].sizeOfParam      = sizeof(UINT);

        destroyParams[HistAlgoProcessDestroyParamTypeCameraInfo].destroyParamType           =
            HistAlgoProcessDestroyParamTypeCameraInfo;
        destroyParams[HistAlgoProcessDestroyParamTypeCameraInfo].pParam                     =
            static_cast<VOID*>(&cameraInfo);
        destroyParams[HistAlgoProcessDestroyParamTypeCameraInfo].sizeOfParam                = sizeof(StatsCameraInfo);

        destroyParamList.paramCount                                             = HistAlgoProcessDestroyParamTypeCount;
        destroyParamList.pParamList                                             = &destroyParams[0];

        m_pHDRAlgorithm->HistAlgoDestroy(m_pHDRAlgorithm, &destroyParamList);
        m_pHDRAlgorithm = NULL;
    }

    if (NULL != m_pHDRAlgorithmHandler)
    {
        CAMX_DELETE m_pHDRAlgorithmHandler;
        m_pHDRAlgorithmHandler = NULL;
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HistogramProcessNode* HistogramProcessNode::Create(
    const NodeCreateInputData*  pCreateInputData,
    NodeCreateOutputData*       pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    CamxResult result = CamxResultSuccess;

    HistogramProcessNode* pHistogramProcessNode     = CAMX_NEW HistogramProcessNode();;

    if (NULL == pHistogramProcessNode)
    {
        CAMX_LOG_ERROR(CamxLogGroupHist, "HistogramProcessNode::Create failed - out of memory");
    }
    else
    {
        if (NULL != pCreateInputData->pHistAlgoCallbacks)
        {
            pHistogramProcessNode->m_cameraInfo.cameraId                                                  =
                pCreateInputData->pPipeline->GetCameraId();
            HistAlgoProcessCreateParamList  createParamList                                          = { 0 };
            HistAlgoProcessCreateParam      createParams[HistAlgoProcessCreateParamTypeCount]   = {};

            // To create respective camera algo
            createParams[HistAlgoProcessCreateParamTypeCameraInfo].createParamType  =
                HistAlgoProcessCreateParamTypeCameraInfo;
            createParams[HistAlgoProcessCreateParamTypeCameraInfo].pParam           =
                static_cast<VOID*>(&pHistogramProcessNode->m_cameraInfo);
            createParams[HistAlgoProcessCreateParamTypeCameraInfo].sizeOfParam      = sizeof(StatsCameraInfo);

            createParamList.paramCount = HistAlgoProcessCreateParamTypeCount;
            createParamList.pParamList = &createParams[0];

            pHistogramProcessNode->m_pCreate = pCreateInputData->pHistAlgoCallbacks->pfnCreate;

            pHistogramProcessNode->m_pHDRAlgorithmHandler = CAMX_NEW HistAlgorithmHandler();

            if (NULL == pHistogramProcessNode->m_pHDRAlgorithmHandler)
            {
                CAMX_LOG_ERROR(CamxLogGroupHist, "HDR Algorithm Handler create Failed - out of memory");
                result = CamxResultENoMemory;
            }

            if (CamxResultSuccess == result)
            {

                result = pHistogramProcessNode->m_pHDRAlgorithmHandler->CreateHistAlgorithm(
                                                                            &createParamList,
                                                                            &(pHistogramProcessNode->m_pHDRAlgorithm),
                                                                            pHistogramProcessNode->m_pCreate);
            }
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupHist, "HDR algo creation failed due to no memory");
                if (NULL != pHistogramProcessNode->m_pHDRAlgorithmHandler)
                {
                    CAMX_DELETE pHistogramProcessNode->m_pHDRAlgorithmHandler;
                    pHistogramProcessNode->m_pHDRAlgorithmHandler = NULL;

                }

                CAMX_DELETE pHistogramProcessNode;
                pHistogramProcessNode = NULL;
            }
        }
    }

    return pHistogramProcessNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HistogramProcessNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::ProcessingNodeInitialize(
    const NodeCreateInputData*  pCreateInputData,
    NodeCreateOutputData*       pCreateOutputData)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupHist, SCOPEEventHistogramProcessNodeNodeInitialize);
    CamxResult result = CamxResultSuccess;

    m_pChiContext                = pCreateInputData->pChiContext;
    pCreateOutputData->pNodeName = m_pNodeName;
    m_bufferOffset               = GetMaximumPipelineDelay();

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL == pFinalizeInitializationData)
    {
        result = CamxResultEFailed;
    }

    UINT inputPortId[StatsInputPortMaxCount];
    UINT numInputPort = 0;

    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
    {
        // need request - 3 buffer
        SetInputPortBufferDelta(inputIndex, m_bufferOffset);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::GetPropertyDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::GetPropertyDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupHist);
    CAMX_ASSERT_MESSAGE(NULL != pExecuteProcessRequestData, "pExecuteProcessRequestData NULL pointer");

    CamxResult              result           = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    StatsDependency         statsDependency  = { 0 };

    CAMX_ASSERT_MESSAGE(NULL != pNodeRequestData, "Node capture request data NULL pointer");

    // Check property dependencies
    // result = m_pStatsProcessorManager->GetDependencies(pStatsProcessRequestDataInfo, &statsDependency);
    if (CamxResultSuccess == result)
    {
        // Add property dependencies
        pNodeRequestData->dependencyInfo[0].propertyDependency.count = statsDependency.propertyCount;
        for (INT32 i = 0; i < statsDependency.propertyCount; i++)
        {
            if (TRUE == IsTagPresentInPublishList(statsDependency.properties[i].property))
            {
                pNodeRequestData->dependencyInfo[0].propertyDependency.properties[i] =
                    statsDependency.properties[i].property;
                pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[i]    =
                    statsDependency.properties[i].offset;
            }
            else
            {
                CAMX_LOG_VERBOSE(CamxLogGroupHist, "property: %08x is not published in the pipeline count %d ",
                    statsDependency.properties[i].property, pNodeRequestData->dependencyInfo[0].propertyDependency.count);
                pNodeRequestData->dependencyInfo[0].propertyDependency.count--;
            }
        }

        // For First request (and first request after flush), we should not set any dependency and it should just be bypassed
        UINT64 requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(pNodeRequestData->pCaptureRequest->requestId);
        if ((CamxResultSuccess == result) && (requestIdOffsetFromLastFlush > 1))
        {
            for (UINT32 i = 0; i < NumHistogramProcessPropertyReadTags; i++)
            {
                if (TRUE == IsTagPresentInPublishList(HistogramProcessPropertyReadTags[i]))
                {
                    pNodeRequestData->dependencyInfo[0].propertyDependency.properties[
                        pNodeRequestData->dependencyInfo[0].propertyDependency.count] =
                        HistogramProcessPropertyReadTags[i];
                    pNodeRequestData->dependencyInfo[0].propertyDependency.offsets[
                        pNodeRequestData->dependencyInfo[0].propertyDependency.count++] =
                        HistogramProcessPropertyReadTagsOffset[i];
                }
                else
                {
                    CAMX_LOG_VERBOSE(CamxLogGroupHist,
                        "property: %08x is not published in the pipeline", HistogramProcessPropertyReadTags[i]);
                }
            }
        }

        if (pNodeRequestData->dependencyInfo[0].propertyDependency.count >= 1)
        {
            // Update dependency request data for topology to consume
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency               = TRUE;
            pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency   = TRUE;
            pNodeRequestData->dependencyInfo[0].processSequenceId                                   = 1;
            pNodeRequestData->numDependencyLists = 1;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::GetBufferDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::GetBufferDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData
    ) const
{
    CAMX_ENTRYEXIT(CamxLogGroupHist);

    if (NULL == pExecuteProcessRequestData)
    {
        CAMX_LOG_ERROR(CamxLogGroupHist, "pExecuteProcessRequestData pointer is NULL");
        return CamxResultEInvalidPointer;
    }

    PerRequestActivePorts*  pEnabledPorts    = pExecuteProcessRequestData->pEnabledPortsInfo;
    NodeProcessRequestData* pNodeRequestData = pExecuteProcessRequestData->pNodeProcessRequestData;
    CamxResult              result           = CamxResultSuccess;

    if ((NULL == pNodeRequestData) || (NULL == pEnabledPorts))
    {
        CAMX_LOG_ERROR(CamxLogGroupHist, "pNodeRequestData: %p or pEnabledPorts: %p pointer is NULL",
            pNodeRequestData, pEnabledPorts);
        return CamxResultEInvalidPointer;
    }

    for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
    {
        PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[i];

        if (NULL == pPerRequestInputPort)
        {
            CAMX_LOG_ERROR(CamxLogGroupHist, "Per Request Port info a Null pointer for index: %d", i);
            return CamxResultEInvalidPointer;
        }

        switch (pPerRequestInputPort->portId)
        {
            case HistogramProcessInputPortFD:
                pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[0] =
                    pEnabledPorts->pInputPorts[i].phFence;
                pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[0] =
                    pEnabledPorts->pInputPorts[i].pIsFenceSignaled;
                pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount = 1;

                pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency    = TRUE;
                pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
                pNodeRequestData->dependencyInfo[0].dependencyFlags.hasPropertyDependency             = TRUE;
                pNodeRequestData->dependencyInfo[0].processSequenceId                                 = 1;
                pNodeRequestData->numDependencyLists = 1;
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupHist, "Need to add support to new portid: %u", pPerRequestInputPort->portId);
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CAMX_ENTRYEXIT_SCOPE_ID(CamxLogGroupHist, SCOPEEventHistogramProcessNodeExecuteProcessRequest,
        pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest->requestId);

    CamxResult                  result              = CamxResultSuccess;
    HistAlgoProcessInputList    inputAlgoParam      = { 0 };
    HistAlgoProcessOutputList   outputList          = { 0 };
    NodeProcessRequestData*     pNodeRequestData    = pExecuteProcessRequestData->pNodeProcessRequestData;
    m_perFrameInfo.pPerRequestPorts                 = pExecuteProcessRequestData->pEnabledPortsInfo;
    m_perFrameInfo.requestID                        = pNodeRequestData->pCaptureRequest->requestId;

    PrepareHDROutputBuffers(&outputList); // Can init only once in connstructor and acquire reference to that here.


    CAMX_LOG_VERBOSE(CamxLogGroupHist, "HistogramProcessNode execute request for id %llu seqId %d",
                    m_perFrameInfo.requestID,
                    pNodeRequestData->processSequenceId);

    if (pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask != 0)
    {
        CAMX_LOG_ERROR(CamxLogGroupHist, "pending dependencies! dependencyFlagsMask = %d",
            pNodeRequestData->dependencyInfo[0].dependencyFlags.dependencyFlagsMask);
        result = CamxResultEFailed;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupHist,
        "HistogramProcessNode execute request for requestId %llu pipeline %d seqId %d",
        m_perFrameInfo.requestID,
        GetPipeline()->GetPipelineId(),
        pNodeRequestData->processSequenceId);

    if ((CamxResultSuccess == result) && (0 == pNodeRequestData->processSequenceId))
    {
        // Initialize number of dependency lists to 0
        pNodeRequestData->numDependencyLists = 0;

        if (CamxResultSuccess == result)
        {
            result = GetPropertyDependencies(pExecuteProcessRequestData);
        }

        if (CamxResultSuccess == result)
        {
            result = GetBufferDependencies(pExecuteProcessRequestData);
        }
    }

    if ((CamxResultSuccess == result) &&
        (FALSE == Node::HasAnyDependency(pNodeRequestData->dependencyInfo)))
    {
        if (TRUE == CheckDependencyChange())
        {
            //  Histogram node's algo executes here
            SetInputParamForAlgo(&inputAlgoParam);

            // HDR Process And pOutput will hold the data.
            result = m_pHDRAlgorithm->HistAlgoProcess(m_pHDRAlgorithm, &inputAlgoParam, &outputList);
        }
        if (CamxResultSuccess == result)
        {
            result = PublishMetadata(&outputList);
        }

        if (CamxResultSuccess == result)
        {
            NotifyJobProcessRequestDone(m_perFrameInfo.requestID);
            CAMX_LOG_VERBOSE(CamxLogGroupHist, "Histogram node ProcessRequestDone %llu", m_perFrameInfo.requestID);
        }

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::PostPipelineCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::PostPipelineCreate()
{
    CamxResult result = CamxResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::NotifyJobProcessRequestDone
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::NotifyJobProcessRequestDone(
    UINT64 requestId)
{
    CamxResult result = CamxResultSuccess;

    ProcessPartialMetadataDone(requestId);
    ProcessMetadataDone(requestId);
    ProcessRequestIdDone(requestId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HistogramProcessNode::GetAECFrameInfoMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::GetAECFrameInfoMetadata()
{
    CamxResult result = CamxResultSuccess;

    static const UINT32  GetProps[]              = { PropertyIDAECFrameInfo };
    static const UINT32  GerPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*                pData[GerPropsLength]   = { 0 };
    UINT64               offsets[GerPropsLength] = { 0 };

    result = GetDataList(GetProps, pData, offsets, GerPropsLength);
    if (CamxResultSuccess == result)
    {
        if (NULL != pData[0])
        {
            Utils::Memcpy(&(m_perFrameInfo.AECframeInfo), pData[0], sizeof(AECFrameInformation));
            CAMX_LOG_VERBOSE(CamxLogGroupHist,
                "HIST: Get FrameInfo for ReqId=%llu Settled=%d Lux=%f "
                "Preflash=%d SnapshotInd=%d BrightnessValue=%f FL:%f ",
                m_perFrameInfo.requestID,
                m_perFrameInfo.AECframeInfo.AECSettled,
                m_perFrameInfo.AECframeInfo.luxIndex,
                m_perFrameInfo.AECframeInfo.AECPreFlashState,
                m_perFrameInfo.AECframeInfo.snapshotIndicator,
                m_perFrameInfo.AECframeInfo.brightnessValue,
                m_perFrameInfo.AECframeInfo.frameLuma);

            if (FALSE == Utils::FEqual(m_perFrameInfo.AECframeInfo.exposureInfo[ExposureIndexShort].sensitivity, 0.0f))
            {
                if (TRUE == m_perFrameInfo.isIHDRMode)
                {
                    m_perFrameInfo.DRCGain = IHDR_DRC_GAIN;
                }
                else
                {
                    m_perFrameInfo.DRCGain = m_perFrameInfo.AECframeInfo.exposureInfo[ExposureIndexSafe].sensitivity /
                        m_perFrameInfo.AECframeInfo.exposureInfo[ExposureIndexShort].sensitivity;
                }
            }
            else
            {
                m_perFrameInfo.DRCGain = 0.0f;
            }

            if (FALSE == Utils::FEqual(m_perFrameInfo.AECframeInfo.exposureInfo[ExposureIndexSafe].sensitivity, 0.0f))
            {
                if (TRUE == m_perFrameInfo.isIHDRMode)
                {
                    m_perFrameInfo.DRCDarkGain = IHDR_DRC_DARK_GAIN;
                }
                else
                {
                    m_perFrameInfo.DRCDarkGain = m_perFrameInfo.AECframeInfo.exposureInfo[ExposureIndexLong].sensitivity /
                        m_perFrameInfo.AECframeInfo.exposureInfo[ExposureIndexSafe].sensitivity;
                }
            }
            else
            {
                m_perFrameInfo.DRCDarkGain = 0.0f;
            }
        }

        if (FALSE == m_perFrameInfo.AECframeInfo.AECSettled)
        {
            result = CamxResultEUnsupported;
        }

    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::GetHistNodeInputMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::GetHistNodeInputMetadata()
{
    CamxResult result = CamxResultSuccess;

    static const UINT32  GetProps[]              = { PropertyIDIFEADRCInfoOutput, PropertyIDSensorMetaData};
    static const UINT32  GerPropsLength          = CAMX_ARRAY_SIZE(GetProps);
    VOID*                pData[GerPropsLength]   = { 0 };
    UINT64               offsets[GerPropsLength] = { 0, 0 };

    result = GetDataList(GetProps, pData, offsets, GerPropsLength);

    if (CamxResultSuccess == result)
    {
        if (NULL != pData[0])
        {
            ADRCData* pADRCData = static_cast<ADRCData*>(pData[0]);
            m_perFrameInfo.DRCGainLTM = pow(m_perFrameInfo.DRCGain, pADRCData->ltmPercentage);
            CAMX_LOG_VERBOSE(CamxLogGroupHist, "Input Meta: DRCGain=%f, ltmPercentage = %f, DRCGainLTM=%f",
                m_perFrameInfo.DRCGain,
                pADRCData->ltmPercentage,
                m_perFrameInfo.DRCGainLTM);
        }

        if (NULL != pData[1])
        {
            SensorMetaData* pSensorMetaData = static_cast<SensorMetaData*>(pData[1]);
            m_perFrameInfo.sensorGain = pSensorMetaData->sensorGain;
            CAMX_LOG_VERBOSE(CamxLogGroupHist, "Input Meta: sensor gain = %f", m_perFrameInfo.sensorGain);
        }
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HistogramProcessNode::GetInputFrameImageBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::GetInputFrameImageBufferInfo()
{
    PerRequestInputPortInfo* pInputPort;
    ImageFormat              imageFormat;
    CamxResult               result                   = CamxResultEFailed;
    PerFrameInfo*            pPerFrameInfo            = &m_perFrameInfo;
    PerRequestActivePorts*   pEnabledPorts            = pPerFrameInfo->pPerRequestPorts;

    CAMX_ASSERT(NULL != pEnabledPorts);
    CAMX_ASSERT(NULL != pPerFrameInfo);

    for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
    {
        pInputPort = &pEnabledPorts->pInputPorts[i];

        if (HistogramProcessInputPortFD == pInputPort->portId)
        {
            if ((NULL != pInputPort->pImageBuffer) &&
                (NULL != pInputPort->pImageBuffer->GetFormat()) &&
                (NULL != pInputPort->pImageBuffer->GetPlaneVirtualAddr(0, 0)))
            {
                pPerFrameInfo->pImageBuffer = pInputPort->pImageBuffer;
                imageFormat = *pInputPort->pImageBuffer->GetFormat();

                if (FALSE == ImageFormatUtils::IsYUV(&imageFormat))
                {
                    CAMX_LOG_ERROR(CamxLogGroupHist, "Buffer format is not YUV");

                    result = CamxResultEUnsupported;
                }
                else
                {
                    pPerFrameInfo->pImageAddress = pInputPort->pImageBuffer->GetPlaneVirtualAddr(0, 0);
                    CAMX_LOG_VERBOSE(CamxLogGroupHist, "pImageAddress %p", pPerFrameInfo->pImageAddress);

                    if ((NULL != pInputPort->pImageBuffer->GetPlaneVirtualAddr(0, 1)) &&
                        ((FD_DL_ARM == GetStaticSettings()->FDFilterEngine) ||
                         (FD_DL_DSP == GetStaticSettings()->FDFilterEngine)))
                    {
                        pPerFrameInfo->pUVImageAddress = pInputPort->pImageBuffer->GetPlaneVirtualAddr(0, 1);

                        result = CamxResultSuccess;
                    }
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupHist, "Input Port %p pInputPort->portId %d pInputPort->pImageBuffer %p ",
                               pInputPort, pInputPort->portId, pInputPort->pImageBuffer);

                result = CamxResultEInvalidArg;
            }
            break;
        }
    }

    if (NULL == pPerFrameInfo->pImageAddress)
    {
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HistogramProcessNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result = CamxResultSuccess;
    UINT32 tagCount   = 0;
    UINT32 tagID;

    for (UINT32 tagIndex = 0; tagIndex < NumHistOutputVendorTags; ++tagIndex)
    {
        result = VendorTagManager::QueryVendorTagLocation(
                HistOutputVendorTags[tagIndex].pSectionName,
                HistOutputVendorTags[tagIndex].pTagName,
                &tagID);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                HistOutputVendorTags[tagIndex].pSectionName,
                HistOutputVendorTags[tagIndex].pTagName);
            break;
        }
        pPublistTagList->tagArray[tagCount++] = tagID;
        m_vendorTagArray[tagIndex]            = tagID;
    }

    CAMX_LOG_VERBOSE(CamxLogGroupMeta, "Hist node: %d tags will be published", pPublistTagList->tagCount);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// HistogramProcessNode::PublishMetadata
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult HistogramProcessNode::PublishMetadata(
    const HistAlgoProcessOutputList* pOutput)
{
    CamxResult result         = CamxResultSuccess;
    UINT8*     pLTCRatioIndex = NULL;

    if (0 != pOutput->pHistAlgoProcessOutputList[HistAlgoProcessOutputTypeLTCRatio].sizeOfWrittenOutput)
    {
        pLTCRatioIndex =
            static_cast<UINT8*>(
                pOutput->pHistAlgoProcessOutputList[HistAlgoProcessOutputTypeLTCRatio].pHistAlgoProcessOutput);
        // Can use that m_ltcRatio variable used to initialize the output buffer but checking size good idea!
    }

    const UINT VendorTag[1] = { m_vendorTagArray[HistOutputVendorTagsIndex::HistNodeLTCRatioIndex] };
    const VOID* pDstData[1] = { &pLTCRatioIndex };
    UINT pDataCount[1]      = { sizeof(UINT8) };

    result = WriteDataList(VendorTag, pDstData, pDataCount, 1);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupHist, "Histogram node failed at publishing meta");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::SetInputParamForAlgo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HistogramProcessNode::SetInputParamForAlgo(
    HistAlgoProcessInputList* pInputList)
{
    pInputList->inputCount = 0;

    UpdateHDRInputParam(pInputList, HistAlgoProcessInputTypeYPlaneImageAddress,
        sizeof(UINT8), static_cast<VOID*>(m_perFrameInfo.pImageAddress));

    UpdateHDRInputParam(pInputList, HistAlgoProcessInputTypeDRCGainLTM,
        sizeof(FLOAT), static_cast<VOID*>(&m_perFrameInfo.DRCGainLTM));

    UpdateHDRInputParam(pInputList, HistAlgoProcessInputTypeDRCDarkGain,
        sizeof(FLOAT), static_cast<VOID*>(&m_perFrameInfo.DRCDarkGain));

    UpdateHDRInputParam(pInputList, HistAlgoProcessInputTypeSensorGain,
        sizeof(FLOAT), static_cast<VOID*>(&m_perFrameInfo.sensorGain));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::CheckDependencyChange
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL HistogramProcessNode::CheckDependencyChange()
{
    BOOL isChanged      = FALSE;
    CamxResult result   = CamxResultSuccess;

    result = GetAECFrameInfoMetadata();

    if (result == CamxResultSuccess)
    {
        result = GetHistNodeInputMetadata();
    }

    if (result == CamxResultSuccess)
    {
        result = GetInputFrameImageBufferInfo();
    }

    if (result == CamxResultSuccess)
    {
        isChanged = TRUE;
    }

    return isChanged;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::UpdateHDRInputParam
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HistogramProcessNode::UpdateHDRInputParam(
    HistAlgoProcessInputList* pInputList,
    HistAlgoProcessInputType  inputType,
    UINT32                         inputSize,
    VOID*                          pValue)
{
    pInputList->pHistAlgoProcessInputs[pInputList->inputCount].inputType                  = inputType;
    pInputList->pHistAlgoProcessInputs[pInputList->inputCount].sizeOfInputType            =
        static_cast<UINT32>(inputSize);
    pInputList->pHistAlgoProcessInputs[pInputList->inputCount].pHistAlgoProcessInput = pValue;
    pInputList->inputCount++;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HistogramProcessNode::PrepareHDROutputBuffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID HistogramProcessNode::PrepareHDROutputBuffers(
    HistAlgoProcessOutputList*       pOutput)
{
    pOutput->outputCount = 0;

    pOutput->pHistAlgoProcessOutputList[HistAlgoProcessOutputTypeLTCRatio].outputType =
        HistAlgoProcessOutputTypeLTCRatio;
    pOutput->pHistAlgoProcessOutputList[HistAlgoProcessOutputTypeLTCRatio].pHistAlgoProcessOutput = &m_LTCRatio;
    pOutput->pHistAlgoProcessOutputList[HistAlgoProcessOutputTypeLTCRatio].sizeOfOutput = sizeof(UINT8);
    pOutput->outputCount = pOutput->outputCount + 1;

    pOutput->pHistAlgoProcessOutputList[HistAlgoProcessOutputTypeLTCRatioPercentage].outputType =
        HistAlgoProcessOutputTypeLTCRatioPercentage;
    pOutput->pHistAlgoProcessOutputList[HistAlgoProcessOutputTypeLTCRatioPercentage].pHistAlgoProcessOutput =
        &m_LTCRatioPercentage;
    pOutput->pHistAlgoProcessOutputList[HistAlgoProcessOutputTypeLTCRatioPercentage].sizeOfOutput = sizeof(INT32);
    pOutput->outputCount = pOutput->outputCount + 1;

}


CAMX_NAMESPACE_END
