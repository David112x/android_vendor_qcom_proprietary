////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxransacnode.cpp
/// @brief RANSAC Node class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxnode.h"
#include "camxransacnode.h"
#include "camxcdmdefs.h"
#include "camxcslresourcedefs.h"
#include "camximagebuffer.h"
#include "camximageformatutils.h"
#include "camxhwcontext.h"
#include "camxtitan17xcontext.h"
#include "camxtitan17xdefs.h"
#include "camxtrace.h"
#include "camxlrmeproperty.h"
#include "camxtitan17xcontext.h"
#include "camxvendortags.h"
#include "Process_LRME.h"


CAMX_NAMESPACE_BEGIN

const static UINT g_RANSACMaxInputPorts  = 1;
const static UINT g_RANSACMaxOutputPorts = 0;

// @brief list of vendor tags published by Ransac
static const struct NodeVendorTag g_RANSACOutputVendorTags[] =
{
    { "org.quic.camera2.ipeicaconfigs", "ICAReferenceParams" }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::RANSACNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RANSACNode::RANSACNode()
{
    m_pNodeName                  = "RANSAC";
    m_numInputPorts              = g_RANSACMaxInputPorts;
    m_numOutputPorts             = g_RANSACMaxOutputPorts;
    m_derivedNodeHandlesMetaDone = TRUE;
    m_forceIdentityTransform     = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::~RANSACNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RANSACNode::~RANSACNode()
{
    Cleanup();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
RANSACNode* RANSACNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    return CAMX_NEW RANSACNode;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::Cleanup
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID RANSACNode::Cleanup()
{
    return;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RANSACNode::ProcessingNodeInitialize(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_UNREFERENCED_PARAM(pCreateInputData);

    CamxResult        result                   = CamxResultSuccess;

    CAMX_ASSERT(RANSAC == Type());
    CAMX_ASSERT(NULL != pCreateOutputData);

    pCreateOutputData->maxOutputPorts = m_numInputPorts;
    pCreateOutputData->maxInputPorts  = m_numOutputPorts;

    if (CamxResultSuccess != result)
    {
        Cleanup();
    }

    UINT32  groupID         = 1;
    UINT    numOutputPorts  = 0;
    UINT    outputPortId[MaxBufferComposite];

    GetAllOutputPortIds(&numOutputPorts, &outputPortId[0]);

    for (UINT outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
    {
        pCreateOutputData->bufferComposite.portGroupID[outputPortId[outputPortIndex]] = groupID++;
    }

    pCreateOutputData->bufferComposite.hasCompositeMask = FALSE;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RANSACNode::ProcessingNodeFinalizeInputRequirement
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RANSACNode::ProcessingNodeFinalizeInputRequirement(
    BufferNegotiationData* pBufferNegotiationData)
{
    UINT32 numInputPorts = 0;
    UINT32 inputPortId[RANSACMaxInput];

    UINT maxBlocks = Utils::Ceiling(540.0f / 12.0f) * Utils::Ceiling(360.0f / 8.0f);
    UINT minBlocks = Utils::Ceiling(36.0f / 12.0f) * Utils::Ceiling(24.0f / 8.0f);
    UINT optBlocks = Utils::Ceiling(240.0f / 12.0f) * Utils::Ceiling(136.0f / 8.0f);
    UINT32 maxOutputWidth = maxBlocks * 6;
    UINT32 minOutputWidth = minBlocks * 6;
    UINT32 optOutputWidth = optBlocks * 6;
    // Get Input Port List
    GetAllInputPortIds(&numInputPorts, &inputPortId[0]);

    for (UINT input = 0; input < numInputPorts; input++)
    {
        pBufferNegotiationData->inputBufferOptions[input].nodeId = Type();
        pBufferNegotiationData->inputBufferOptions[input].instanceId = InstanceID();
        pBufferNegotiationData->inputBufferOptions[input].portId = inputPortId[input];

        BufferRequirement* pInputBufferRequirement = &pBufferNegotiationData->inputBufferOptions[input].bufferRequirement;
        pInputBufferRequirement->maxWidth = maxOutputWidth;
        pInputBufferRequirement->maxHeight = 1;
        pInputBufferRequirement->minWidth = minOutputWidth;
        pInputBufferRequirement->minHeight = 1;
        pInputBufferRequirement->optimalWidth = optOutputWidth;
        pInputBufferRequirement->optimalHeight = 1;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// InterpolateICATransform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CAMX_INLINE VOID InterpolateICATransform(
    CPerspectiveTransform  transform,
    CPerspectiveTransform* pInterpolatedTransform,
    UINT                   numInnerLRMEFrames)
{
    CPerspectiveTransform identityTransform;

    for (UINT i = 0; i < 3 ; i++)
    {
        for (UINT j = 0; j < 3; j++)
        {
            pInterpolatedTransform->m[i][j] =
                ((transform.m[i][j] - identityTransform.m[i][j]) / numInnerLRMEFrames) + identityTransform.m[i][j];

            CAMX_LOG_VERBOSE(CamxLogGroupLRME, "orig[%d][%d]= %f new[%d][%d]= %f",
                             i, j, transform.m[i][j], i, j, pInterpolatedTransform->m[i][j]);
        }
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RANSACNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CAMX_ASSERT(NULL != pExecuteProcessRequestData);

    CamxResult              result            = CamxResultSuccess;
    NodeProcessRequestData* pNodeRequestData  = pExecuteProcessRequestData->pNodeProcessRequestData;
    PerRequestActivePorts*  pEnabledPorts     = pExecuteProcessRequestData->pEnabledPortsInfo;
    UINT                    numBatchedFrames  = pNodeRequestData->pCaptureRequest->numBatchedFrames;
    UINT64                  requestId         = pNodeRequestData->pCaptureRequest->requestId;
    UINT                    dependencyIndex   = 0;

    CAMX_ASSERT(NULL != pNodeRequestData);
    CAMX_ASSERT(NULL != pEnabledPorts);
    CAMX_ASSERT(NULL != pNodeRequestData->pCaptureRequest);

    CAMX_LOG_INFO(CamxLogGroupLRME, "Ransac request %llu sequence %d", requestId, pNodeRequestData->processSequenceId);

    if (0 == pNodeRequestData->processSequenceId)
    {
        for (UINT portIndex = 0; portIndex < pEnabledPorts->numInputPorts; portIndex++)
        {
            PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[portIndex];
            if (NULL != pPerRequestInputPort)
            {
                UINT fenceCount = pNodeRequestData->dependencyInfo[dependencyIndex].bufferDependency.fenceCount;

                pNodeRequestData->dependencyInfo[dependencyIndex].bufferDependency.phFences[fenceCount] =
                    pPerRequestInputPort->phFence;
                pNodeRequestData->dependencyInfo[dependencyIndex].bufferDependency.pIsFenceSignaled[fenceCount] =
                    pPerRequestInputPort->pIsFenceSignaled;

                CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Req[%llu] index[%d] ransac Fence wait %d",
                                 requestId, portIndex, pPerRequestInputPort->phFence[0]);

                pNodeRequestData->dependencyInfo[dependencyIndex].bufferDependency.fenceCount++;
            }
        }

        if (0 < pNodeRequestData->dependencyInfo[dependencyIndex].bufferDependency.fenceCount)
        {
            pNodeRequestData->dependencyInfo[dependencyIndex].dependencyFlags.hasInputBuffersReadyDependency    = TRUE;
            pNodeRequestData->dependencyInfo[dependencyIndex].dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
            pNodeRequestData->dependencyInfo[dependencyIndex].processSequenceId                                 = 1;
            pNodeRequestData->dependencyInfo[dependencyIndex].dependencyFlags.hasPropertyDependency             = TRUE;
            pNodeRequestData->dependencyInfo[dependencyIndex].propertyDependency.count                          = 1;

            pNodeRequestData->dependencyInfo[dependencyIndex].propertyDependency.pipelineIds[0] = GetPipelineId();
            pNodeRequestData->dependencyInfo[dependencyIndex].propertyDependency.properties[0]  = PropertyIDLRMEFrameSettings;

            pNodeRequestData->numDependencyLists = 1;
        }
    }
    else
    {
        // Get the lrme property
        VOID*        pPData[1]                    = { 0 };
        UINT64       pDataOffset[1]               = { 0 };
        UINT64       requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);

        LRMEPropertyFrameSettings* pFrameSettings;

        static const UINT PropertiesLRMEFrameSetting[] = { PropertyIDLRMEFrameSettings };
        result = GetDataList(PropertiesLRMEFrameSetting, pPData, pDataOffset, 1);

        if (CamxResultSuccess == result)
        {
            pFrameSettings = static_cast<LRMEPropertyFrameSettings*>(pPData[0]);
            if (NULL == pFrameSettings)
            {
                CAMX_LOG_ERROR(CamxLogGroupLRME, "Req[%llu] Unable to get PropertyIDLRMEFrameSettings", requestId);
                result = CamxResultEFailed;
            }
        }

        if (CamxResultSuccess == result)
        {
            CAMX_LOG_INFO(CamxLogGroupLRME,
                "Req[%llu] LRME frame setting recv at ransac stepx: %d stepy %d refValid %d resFormat %d"
                " taroffsetx %d taroffsety %d refoffsetx %d refoffsety %d subpelsearch %d fullw %d fullh %d tarw %d tarh %d"
                " upscalefactor %d alternate skip %d",
                requestId,
                pFrameSettings->LRMEStepX, pFrameSettings->LRMEStepY, pFrameSettings->LRMERefValid,
                pFrameSettings->LRMEresultFormat, pFrameSettings->LRMETarOffsetX, pFrameSettings->LRMETarOffsetY,
                pFrameSettings->LRMERefOffsetX, pFrameSettings->LRMERefOffsetY, pFrameSettings->LRMEsubpelSearchEnable,
                pFrameSettings->fullWidth, pFrameSettings->fullHeight, pFrameSettings->LRMETarW, pFrameSettings->LRMETarH,
                pFrameSettings->LRMEUpscaleFactor, pFrameSettings->alternateSkipProcessing);

            m_alternateSkipProcessing = pFrameSettings->alternateSkipProcessing;
            if ((FirstValidRequestId != requestIdOffsetFromLastFlush) ||
                ((1 < numBatchedFrames) && (8 != pFrameSettings->LRMEUpscaleFactor)))
            {
                if ((FALSE == m_alternateSkipProcessing) ||
                    (TRUE == IsPreviewPresent()))
                {
                    ChannelType* pAddr = reinterpret_cast<ChannelType*>
                        (pEnabledPorts->pInputPorts[0].pImageBuffer->GetPlaneVirtualAddr(0, 0));
                    SIZE_T bufferSize = pEnabledPorts->pInputPorts[0].pImageBuffer->GetPlaneSize(0);

                    // Invalidate the input buffer before accessing it since the buffer
                    // is configured as cached buffer.
                    result = pEnabledPorts->pInputPorts[0].pImageBuffer->CacheOps(true, false);

                    CamxTime tv1;
                    CamxTime tv2;
                    UINT64   start;
                    UINT64   end;
                    if (CAMX_LOG_IS_VERBOSE_ENABLED(CamxLogGroupLRME))
                    {
                        CamX::OsUtils::GetTime(&tv1);
                        start = (tv1.seconds * 1000) + (tv1.nanoSeconds / 1000000);
                    }

                    if ((CamxResultSuccess != result) ||
                        (NULL == pAddr) ||
                        (NULL == pEnabledPorts->pInputPorts[0].pImageBuffer->GetFormat()) ||
                        (0 != ProcessMeResult(pAddr, bufferSize,
                            pFrameSettings->fullWidth, pFrameSettings->fullHeight,
                            pFrameSettings->LRMETarW, pFrameSettings->LRMETarH,
                            pFrameSettings->LRMETarOffsetX, pFrameSettings->LRMETarOffsetY,
                            pFrameSettings->LRMERefOffsetX, pFrameSettings->LRMERefOffsetY,
                            pFrameSettings->LRMEStepX, pFrameSettings->LRMEStepY,
                            pFrameSettings->LRMEresultFormat,
                            pFrameSettings->LRMEsubpelSearchEnable, pFrameSettings->LRMEUpscaleFactor,
                            LRMETransform_method, m_transform, m_confidence)))
                    {
                        CAMX_LOG_ERROR(CamxLogGroupLRME, "Req[%llu] - Failed, result=%d, pAddr=%p, bufferSize %u, format %p"
                                       "LRME frame setting for ransac stepx: %d stepy %d refValid %d resFormat %d taroffsetx %d"
                                       " taroffsety %d refoffsetx %d refoffsety %d subpelsearch %d fullw %d fullh %d tarw %d "
                                       "tarh %d upscalefactor %d alternate skip %d",
                                       requestId, result, pAddr, bufferSize,
                                       pEnabledPorts->pInputPorts[0].pImageBuffer->GetFormat(),
                                       pFrameSettings->LRMEStepX, pFrameSettings->LRMEStepY,
                                       pFrameSettings->LRMERefValid, pFrameSettings->LRMEresultFormat,
                                       pFrameSettings->LRMETarOffsetX, pFrameSettings->LRMETarOffsetY,
                                       pFrameSettings->LRMERefOffsetX, pFrameSettings->LRMERefOffsetY,
                                       pFrameSettings->LRMEsubpelSearchEnable,
                                       pFrameSettings->fullWidth, pFrameSettings->fullHeight,
                                       pFrameSettings->LRMETarW, pFrameSettings->LRMETarH,
                                       pFrameSettings->LRMEUpscaleFactor, pFrameSettings->alternateSkipProcessing);

                        result = CamxResultEFailed;
                    }

                    if (CAMX_LOG_IS_VERBOSE_ENABLED(CamxLogGroupLRME))
                    {
                        CamX::OsUtils::GetTime(&tv2);
                        end = (tv2.seconds * 1000) + (tv2.nanoSeconds / 1000000);
                        CAMX_LOG_VERBOSE(CamxLogGroupLRME, "nclib ransac result %d confidence %d time %llu", result,
                            m_confidence, end - start);
                    }
                }
            }
            else
            {
                m_confidence = 0;
            }
        }

        if (CamxResultSuccess == result)
        {
            result = ConfigureICATransformSettings(pFrameSettings, numBatchedFrames, requestId, requestIdOffsetFromLastFlush);
        }

        ProcessPartialMetadataDone(requestId);
        ProcessMetadataDone(requestId);
        ProcessRequestIdDone(requestId);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::ConfigureICATransformSettings
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RANSACNode::ConfigureICATransformSettings(
    LRMEPropertyFrameSettings* pFrameSettings,
    UINT                       numBatchedFrames,
    UINT64                     requestId,
    UINT64                     requestIdOffsetFromLastFlush)
{
    CamxResult result                           = CamxResultSuccess;
    UINT       transformTypeMask                = GetHwContext()->GetStaticSettings()->setLRMETransformTypeMask;
    UINT32     transformConfidence              = m_confidence;
    BOOL       regularTransformEnabled          = (0 != (transformTypeMask & (1 << RegularTransform))) ? TRUE : FALSE;
    BOOL       hfrInterpolationTransformEnabled = (0 != (transformTypeMask & (1 << HFRInterpolationTransform))) ? TRUE : FALSE;
    BOOL       unityTransformEnabled            = (0 != (transformTypeMask & (1 << UnityTransform))) ? TRUE : FALSE;

    CPerspectiveTransform transformUnity;
    CPerspectiveTransform interpolatedTransform;

    if ((TRUE == regularTransformEnabled) || (TRUE == hfrInterpolationTransformEnabled))
    {
        result = ConfigureLRMEConfidenceParameter(&transformConfidence, &m_forceIdentityTransform);
        if (CamxResultSuccess == result)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Posting for request %llu,ransac confidence %d,"
                             "transform confidence %d,Identity forced %d, batchframes %d",
                             requestId, m_confidence, transformConfidence, m_forceIdentityTransform, numBatchedFrames);

            // post indentity transform as per m_forceIdentityTransform state for hysteresis implementation
            if ((1 == m_forceIdentityTransform) ||
                ((1 < numBatchedFrames) && (FALSE == hfrInterpolationTransformEnabled)))
            {
                result = PostICATransform(&transformUnity, transformConfidence, pFrameSettings);
            }
            else
            {
                if (1 < numBatchedFrames)
                {
                    // perform transform Interpolation for HFR
                    if ((FirstValidRequestId == requestIdOffsetFromLastFlush) &&
                        (8 != pFrameSettings->LRMEUpscaleFactor))
                    {
                        InterpolateICATransform(m_transform, &interpolatedTransform, numBatchedFrames - 1);
                    }
                    else
                    {
                        InterpolateICATransform(m_transform, &interpolatedTransform, numBatchedFrames);
                    }

                    result = PostICATransform(&interpolatedTransform, transformConfidence, pFrameSettings);
                }
                else if (TRUE == m_alternateSkipProcessing)
                {
                    InterpolateICATransform(m_transform, &interpolatedTransform, 2);
                    result = PostICATransform(&interpolatedTransform, transformConfidence, pFrameSettings);
                }
                else
                {
                    result = PostICATransform(&m_transform, transformConfidence, pFrameSettings);
                }
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupLRME, "Req[%llu] Failed in Configure LRME Confidence Param, result=%d",
                requestId, result);
        }
    }
    else if (TRUE == unityTransformEnabled)
    {
        // post unity transform and confidence
        transformConfidence = 256;
        result = PostICATransform(&transformUnity, transformConfidence, pFrameSettings);
    }
    else
    {
        result = CamxResultEInvalidArg;
        CAMX_LOG_ERROR(CamxLogGroupLRME, "Req[%llu] Invalid LRME/Ransac transform type 0x%x", transformTypeMask);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupLRME, "Posting for request %llu", requestId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::IsNodeDisabledWithOverride
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL RANSACNode::IsNodeDisabledWithOverride()
{
    BOOL bMCTFDisabled = TRUE;

    if (TRUE == GetHwContext()->GetStaticSettings()->enableMCTF)
    {
        bMCTFDisabled = FALSE;
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupLRME, "Ransac Disabled from settings");
    }

    return bMCTFDisabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::PostICATransform
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RANSACNode::PostICATransform(
    VOID* pTrans,
    UINT32 confidence,
    LRMEPropertyFrameSettings* pFrameSettings)
{
    IPEICAPerspectiveTransform  perspectiveTransform;
    CamxResult result = CamxResultSuccess;
    CPerspectiveTransform* pTransform = reinterpret_cast<CPerspectiveTransform *>(pTrans);

    perspectiveTransform.perspectiveGeometryNumRows       = 1;
    perspectiveTransform.perspetiveGeometryNumColumns     = 1;
    perspectiveTransform.perspectiveConfidence            = confidence;
    perspectiveTransform.byPassAlignmentMatrixAdjustement = (1 == m_forceIdentityTransform) ? TRUE : FALSE;
    memcpy(perspectiveTransform.perspectiveTransformArray, &pTransform->m, 3 * 3 * sizeof(FLOAT));
    perspectiveTransform.transformDefinedOnHeight         = pFrameSettings->fullHeight;
    perspectiveTransform.transformDefinedOnWidth          = pFrameSettings->fullWidth;
    perspectiveTransform.ReusePerspectiveTransform        = 0;
    perspectiveTransform.perspectiveTransformEnable       = 1;

    // Get the lrme property
    const VOID*  pPData[1] = { 0 };
    UINT         pDataSize[1] = { 0 };

    pPData[0] = &perspectiveTransform;
    pDataSize[0] = sizeof(IPEICAPerspectiveTransform);
    UINT IPEICATAGLocation = 0;
    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
                                                                              "ICAReferenceParams",
                                                                              &IPEICATAGLocation));

    result = WriteDataList(&IPEICATAGLocation, pPData, pDataSize, 1);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupLRME, "Failed to publish ICA transform");
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupLRME, "Ransac posted ICA Ref transform: Confidence %d res %dX%d", confidence,
            pFrameSettings->LRMETarH, pFrameSettings->LRMETarW);
    }

    CAMX_ASSERT(CamxResultSuccess == VendorTagManager::QueryVendorTagLocation("org.quic.camera2.ipeicaconfigs",
        "ICAReferenceParamsLookAhead",
        &IPEICATAGLocation));

    result = WriteDataList(&IPEICATAGLocation, pPData, pDataSize, 1);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupLRME, "Failed to publish ICA lookahead transform");
    }
    else
    {
        CAMX_LOG_INFO(CamxLogGroupLRME, "Ransac posted ICA lookahead transform: Confidence %d res %dX%d", confidence,
            pFrameSettings->LRMETarH, pFrameSettings->LRMETarW);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// RANSACNode::ConfigureLRMEConfidenceParameter
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RANSACNode::ConfigureLRMEConfidenceParameter(
    UINT32* pTransformConfidence,
    UINT8*  pForceIdentityTransform)
{
    CamxResult result                    = CamxResultSuccess;
    UINT32     ret                       = 0;
    UINT32     enableTransformConfidence = 0;
    INT32      transformConfidenceVal    = 0;

    LRME_Chromatix lrmeChromatixData;
    Utils::Memset(&lrmeChromatixData, 0, sizeof(LRME_Chromatix));

    // LRME chromatix tuning data
    lrmeChromatixData.en                                                    = LRMEChromatix_enable;
    lrmeChromatixData.enable_transform_confidence                           = LRMEEnable_transform_confidence;
    lrmeChromatixData.transform_confidence_mapping_base                     = LRMETransform_confidence_mapping_base;
    lrmeChromatixData.transform_confidence_mapping_c1                       = LRMETransform_confidence_mapping_c1;
    lrmeChromatixData.transform_confidence_mapping_c2                       = LRMETransform_confidence_mapping_c2;
    lrmeChromatixData.transform_confidence_thr_to_force_identity_transform  = LRMETransform_confidence_thr_to_force_identity;

    // Calculate TF confidence based on LRME post processing (e.g Ransac) transform confidence and
    // check if identity transform needs to be forced based on confidence hsyterisis implementation.
    ret = LRME_ConfigureConfidenceParameter(&lrmeChromatixData,
                                            *pTransformConfidence,
                                            &enableTransformConfidence,
                                            &transformConfidenceVal,
                                            pForceIdentityTransform);
    result = (NC_LIB_SUCCESS == ret) ? CamxResultSuccess : CamxResultEFailed;
    CAMX_LOG_VERBOSE(CamxLogGroupLRME, "LRME_ConfigureConfidenceParameter : result %d", result);

    if (CamxResultSuccess == result)
    {
        *pTransformConfidence = transformConfidenceVal;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID RANSACNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// RANSACNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult RANSACNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult result   = CamxResultSuccess;
    UINT32     numTags  = 0;
    UINT32     tagID;

    for (UINT32 tagIndex = 0; tagIndex < numTags; ++tagIndex)
    {
        result = VendorTagManager::QueryVendorTagLocation(
                    g_RANSACOutputVendorTags[tagIndex].pSectionName,
                    g_RANSACOutputVendorTags[tagIndex].pTagName,
                    &tagID);

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                g_RANSACOutputVendorTags[tagIndex].pSectionName,
                g_RANSACOutputVendorTags[tagIndex].pTagName);
            break;
        }

        pPublistTagList->tagArray[tagIndex] = tagID;
    }

    if (CamxResultSuccess == result)
    {
        pPublistTagList->tagCount = numTags;
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tags will be published", numTags);
    }

    return result;
}

CAMX_NAMESPACE_END
