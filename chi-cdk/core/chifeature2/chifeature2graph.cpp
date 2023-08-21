
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graph.cpp
/// @brief CHI feature graph definition
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <set>
#include "chibinarylog.h"
#include "chifeature2graph.h"

// NOWHINE FILE CP006: Need whiner update: std::vector allowed in exceptional cases

static const UINT32 NumDefaultLinks         = 40;
static const UINT32 NumDefaultGraphNodes    = 10;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Public Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Graph* ChiFeature2Graph::Create(
    ChiFeature2GraphCreateInputInfo* pFeatureGraphCreateInputInfo)
{
    ChiFeature2Graph* pFeatureGraph = NULL;

    if (NULL != pFeatureGraphCreateInputInfo)
    {
        pFeatureGraph = CHX_NEW ChiFeature2Graph();
        if (NULL != pFeatureGraph)
        {
            BOOL isValid = pFeatureGraph->ValidateFeatureGraphDescriptor(pFeatureGraphCreateInputInfo);
            if (TRUE == isValid)
            {
                CDKResult result = pFeatureGraph->Initialize(pFeatureGraphCreateInputInfo);
                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("pFeatureGraph->Initialize() failed with result %d", result);
                    pFeatureGraph->Destroy();
                    pFeatureGraph = NULL;
                }
            }
            else
            {
                CHX_LOG_ERROR("Invalid feature graph detected, aborting");
                pFeatureGraph->Destroy();
                pFeatureGraph = NULL;
            }
        }
        else
        {
            CHX_LOG_ERROR("Could not create ChiFeature2Graph");
        }
    }
    else
    {
        CHX_LOG_ERROR("pFeatureGraphCreateInputInfo is NULL");
    }

    return pFeatureGraph;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Graph::Destroy()
{
    for (UINT32 graphNodeIndex = 0; graphNodeIndex < m_pFeatureGraphNodes.size(); ++graphNodeIndex)
    {
        for (UINT32 featureRequestObjIndex = 0;
             featureRequestObjIndex < m_pFeatureGraphNodes[graphNodeIndex]->pFeatureRequestObjList.size();
             ++featureRequestObjIndex)
        {
            if (NULL != m_pFeatureGraphNodes[graphNodeIndex]->pFeatureRequestObjList[featureRequestObjIndex])
            {
                m_pFeatureGraphNodes[graphNodeIndex]->pFeatureRequestObjList[featureRequestObjIndex]->Destroy();
                m_pFeatureGraphNodes[graphNodeIndex]->pFeatureRequestObjList[featureRequestObjIndex] = NULL;
            }
        }
        CHX_DELETE (m_pFeatureGraphNodes[graphNodeIndex]);
        m_pFeatureGraphNodes[graphNodeIndex] = NULL;
    }

    for (UINT callbackIndex = 0; callbackIndex < m_pFeatureGraphCallbackData.size(); callbackIndex++)
    {
        CHX_DELETE(m_pFeatureGraphCallbackData[callbackIndex]);
        m_pFeatureGraphCallbackData[callbackIndex] = NULL;
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ExecuteProcessRequest(
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObj)
{
    CDKResult result = CDKResultSuccess;

    m_pUsecaseRequestObj = pUsecaseRequestObj;

    // Depending on state, either gather input dependencies for URO or set input dependencies from URO
    ChiFeature2UsecaseRequestObjectState requestState = pUsecaseRequestObj->GetRequestState();

    switch (requestState)
    {
        case ChiFeature2UsecaseRequestObjectState::Initialized:
            // Setup links, feature request objects, and gather input dependencies for the usecase request object
            result = WalkAllExtSinkLinks(pUsecaseRequestObj);
            break;

        case ChiFeature2UsecaseRequestObjectState::InputConfigPending:
            // Set any available input dependencies for the usecase request object
            result = WalkAllExtSrcLinks(pUsecaseRequestObj);
            break;

        default:
            CHX_LOG_ERROR("Unsupported ChiFeature2UsecaseRequestObjectState: %d", requestState);
            result = CDKResultEInvalidArg;
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessMessage(
    ChiFeature2RequestObject* pFeatureRequestObj,
    ChiFeature2Messages*      pMessages)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pMessages)
    {
        CHX_LOG_ERROR("pMessages is NULL");
        result = CDKResultEInvalidArg;
    }
    else
    {
        if (NULL != pMessages->chiNotification.pChiMessages)
        {
            result = ProcessChiMessage(pFeatureRequestObj, pMessages);
        }

        if ((CDKResultSuccess == result) && (NULL != pMessages->pFeatureMessages))
        {
            result = ProcessFeatureMessage(pFeatureRequestObj, pMessages);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::Flush()
{
    CDKResult result = CDKResultSuccess;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2Graph::Dump(
    INT fd
    ) const
{
    CDK_UNREFERENCED_PARAM(fd);

    CHX_LOG_DUMP("+--------------------------+");
    CHX_LOG_DUMP("| FeatureGraph Debug Dump: |");
    CHX_LOG_DUMP("+--------------------------+");
    CHX_LOG_DUMP("  FeatureGraph nodes:");
    for (UINT32 graphNodeIndex = 0; graphNodeIndex < m_pFeatureGraphNodes.size(); ++graphNodeIndex)
    {
        ChiFeature2GraphNode* pGraphNode = m_pFeatureGraphNodes[graphNodeIndex];

        CHX_LOG_DUMP("    Feature %s:", pGraphNode->pFeatureBaseObj->GetFeatureName());
        CHX_LOG_DUMP("      Feature id:         %u", pGraphNode->featureId);
        CHX_LOG_DUMP("      Instance id:        %u", pGraphNode->instanceProps.instanceId);
        CHX_LOG_DUMP("      Camera id:          %u", pGraphNode->instanceProps.cameraId);
        CHX_LOG_DUMP("      FeatureBase:        %p", pGraphNode->pFeatureBaseObj);
        CHX_LOG_DUMP("      FeatureReqObjList:  %p", &(pGraphNode->pFeatureRequestObjList));

        for (UINT32 featureRequestObjIndex = 0;
             featureRequestObjIndex < pGraphNode->pFeatureRequestObjList.size();
             ++featureRequestObjIndex)
        {
            CHX_LOG_DUMP("        FeatureRequestObj:  %p",
                         pGraphNode->pFeatureRequestObjList[featureRequestObjIndex]);
            pGraphNode->pFeatureRequestObjList[featureRequestObjIndex]->Dump(fd);
        }

        CHX_LOG_DUMP("      DwnstrmFeatReqObjs: %p", &(pGraphNode->downstreamFeatureRequestObjMap));

        for (UINT32 featureRequestObjIndex = 0;
             featureRequestObjIndex < pGraphNode->downstreamFeatureRequestObjMap.size();
             ++featureRequestObjIndex)
        {
            CHX_LOG_DUMP("        DwnstrmGraphNode: %p, DwnstrmFeatReqObj: %p",
                         pGraphNode->downstreamFeatureRequestObjMap[featureRequestObjIndex].pGraphNode,
                         pGraphNode->downstreamFeatureRequestObjMap[featureRequestObjIndex].pFeatureRequestObj);
        }

        CHX_LOG_DUMP("      Request table:      %p", &(pGraphNode->requestTable));
        for (UINT8 requestIndex = 0; requestIndex < pGraphNode->requestTable.size(); ++requestIndex)
        {
            CHX_LOG_DUMP("        Request %u:", requestIndex);
            for (UINT8 requestOutputIndex = 0;
                 requestOutputIndex < pGraphNode->requestTable[requestIndex].size();
                 ++requestOutputIndex)
            {
                ChiFeature2RequestOutputInfo* pRequestOutputInfo =
                    &(pGraphNode->requestTable[requestIndex][requestOutputIndex]);
                CHX_LOG_DUMP("          RequestOutputInfo:");
                CHX_LOG_DUMP("            Port descriptor globalId:   {%u, %u, %u, %d, %d}",
                             pRequestOutputInfo->pPortDescriptor->globalId.session,
                             pRequestOutputInfo->pPortDescriptor->globalId.pipeline,
                             pRequestOutputInfo->pPortDescriptor->globalId.port,
                             pRequestOutputInfo->pPortDescriptor->globalId.portDirectionType,
                             pRequestOutputInfo->pPortDescriptor->globalId.portType);
                CHX_LOG_DUMP("            Port descriptor name:       %s",
                             pRequestOutputInfo->pPortDescriptor->pPortName);
                CHX_LOG_DUMP("            Port descriptor target:     %s",
                            (NULL != pRequestOutputInfo->pPortDescriptor->pTargetDescriptor) ?
                             pRequestOutputInfo->pPortDescriptor->pTargetDescriptor->pTargetName : "NULL");
                CHX_LOG_DUMP("            BufferMetaInfo hBuffer:     %p",
                             pRequestOutputInfo->bufferMetadataInfo.hBuffer);
                CHX_LOG_DUMP("            BufferMetaInfo key:         %" PRIu64,
                             pRequestOutputInfo->bufferMetadataInfo.key);
                CHX_LOG_DUMP("            BufferMetaInfo hMetadata:   %p",
                             pRequestOutputInfo->bufferMetadataInfo.hMetadata);
            }
        }

        CHX_LOG_DUMP("      Port -> InputLinkIndex: %p", &(pGraphNode->portToInputLinkIndexMap));
        for (UINT32 portIdIndex = 0;
             portIdIndex < pGraphNode->portToInputLinkIndexMap.size();
             ++portIdIndex)
        {
            const ChiFeature2GlobalPortInstanceId* pPortId =
                &(pGraphNode->portToInputLinkIndexMap[portIdIndex].portId);
            CHX_LOG_DUMP("        {%u, {%u, %u}, {%u, %u, %u, %d %d}} -> %u",
                         pPortId->featureId,
                         pPortId->instanceProps.instanceId,
                         pPortId->instanceProps.cameraId,
                         pPortId->portId.session,
                         pPortId->portId.pipeline,
                         pPortId->portId.port,
                         pPortId->portId.portDirectionType,
                         pPortId->portId.portType,
                         pGraphNode->portToInputLinkIndexMap[portIdIndex].linkIndex);
        }

        CHX_LOG_DUMP("      Port -> OutputLinkIndex: %p",
                     &(pGraphNode->portToOutputLinkIndexMap));
        for (UINT32 portIdIndex = 0;
             portIdIndex < pGraphNode->portToOutputLinkIndexMap.size();
             ++portIdIndex)
        {
            const ChiFeature2GlobalPortInstanceId* pPortId =
                &(pGraphNode->portToOutputLinkIndexMap[portIdIndex].portId);
            CHX_LOG_DUMP("        {%u, {%u, %u}, {%u, %u, %u, %d, %d}} -> %u",
                         pPortId->featureId,
                         pPortId->instanceProps.instanceId,
                         pPortId->instanceProps.cameraId,
                         pPortId->portId.session,
                         pPortId->portId.pipeline,
                         pPortId->portId.port,
                         pPortId->portId.portDirectionType,
                         pPortId->portId.portType,
                         pGraphNode->
                         portToOutputLinkIndexMap[portIdIndex].linkIndex);
        }
    }

    CHX_LOG_DUMP("  Source links (m_pExtSrcLinkData):");
    for (UINT32 linkDataIndex = 0; linkDataIndex < m_pExtSrcLinkData.size(); ++linkDataIndex)
    {
        ChiFeature2GlobalPortInstanceId* pPortId = &(m_pExtSrcLinkData[linkDataIndex]->linkDesc.extSrcLinkDesc.portId);
        CHX_LOG_DUMP("    Source link with sink port {%u, {%u, %u}, {%u, %u, %u, %d, %d}}",
                     pPortId->featureId,
                     pPortId->instanceProps.instanceId,
                     pPortId->instanceProps.cameraId,
                     pPortId->portId.session,
                     pPortId->portId.pipeline,
                     pPortId->portId.port,
                     pPortId->portId.portDirectionType,
                     pPortId->portId.portType);
        CHX_LOG_DUMP("      Type:             %d\n"
                     "      State:            %d\n"
                     "      pSrcGraphNode:    %p\n"
                     "      pSinkGraphNode:   %p\n"
                     "      numBatches:       %u\n"
                     "      numDepsReleased:  %u",
                     m_pExtSrcLinkData[linkDataIndex]->linkType,
                     m_pExtSrcLinkData[linkDataIndex]->linkState,
                     m_pExtSrcLinkData[linkDataIndex]->pSrcGraphNode,
                     m_pExtSrcLinkData[linkDataIndex]->pSinkGraphNode,
                     m_pExtSrcLinkData[linkDataIndex]->numBatches,
                     m_pExtSrcLinkData[linkDataIndex]->numDependenciesReleased);

        for (UINT32 batchIndex = 0;
             batchIndex < m_pExtSrcLinkData[linkDataIndex]->numDependenciesPerBatchList.size();
             ++batchIndex)
        {
            CHX_LOG_DUMP("      numDepsPerBatch[%u]:%u\n",
                batchIndex, m_pExtSrcLinkData[linkDataIndex]->numDependenciesPerBatchList[batchIndex]);
        }

        CHX_LOG_DUMP("      Link Request Table:  %p", &(m_pExtSrcLinkData[linkDataIndex]->linkRequestTable));
        for (UINT32 linkRequestIndex = 0;
             linkRequestIndex < m_pExtSrcLinkData[linkDataIndex]->linkRequestTable.size();
             ++linkRequestIndex)
        {
            CHX_LOG_DUMP("        Request index %u:",
                         m_pExtSrcLinkData[linkDataIndex]->linkRequestTable[linkRequestIndex].requestIndex);
            CHX_LOG_DUMP("          Batch index %u:",
                         m_pExtSrcLinkData[linkDataIndex]->linkRequestTable[linkRequestIndex].batchIndex);
            CHX_LOG_DUMP("          Dependency index %u:",
                         m_pExtSrcLinkData[linkDataIndex]->linkRequestTable[linkRequestIndex].dependencyIndex);
        }
    }

    CHX_LOG_DUMP("  Internal links (m_pInternalLinkData):");
    for (UINT32 linkDataIndex = 0; linkDataIndex < m_pInternalLinkData.size(); ++linkDataIndex)
    {
        ChiFeature2GlobalPortInstanceId* pSrcPortId =
            &(m_pInternalLinkData[linkDataIndex]->linkDesc.internalLinkDesc.srcPortId);
        ChiFeature2GlobalPortInstanceId* pSinkPortId =
            &(m_pInternalLinkData[linkDataIndex]->linkDesc.internalLinkDesc.sinkPortId);
        CHX_LOG_DUMP("    Interal link with source port: {%u, {%u, %u}, {%u, %u, %u, %d, %d}} -> "
                     "sink port: {%u, {%u, %u}, {%u, %u, %u, %d}}",
                     pSrcPortId->featureId,
                     pSrcPortId->instanceProps.instanceId,
                     pSrcPortId->instanceProps.cameraId,
                     pSrcPortId->portId.session,
                     pSrcPortId->portId.pipeline,
                     pSrcPortId->portId.port,
                     pSrcPortId->portId.portDirectionType,
                     pSrcPortId->portId.portType,
                     pSinkPortId->featureId,
                     pSinkPortId->instanceProps.instanceId,
                     pSinkPortId->instanceProps.cameraId,
                     pSinkPortId->portId.session,
                     pSinkPortId->portId.pipeline,
                     pSinkPortId->portId.port,
                     pSinkPortId->portId.portType);
        CHX_LOG_DUMP("      Type:             %d\n"
                     "      State:            %d\n"
                     "      pSrcGraphNode:    %p\n"
                     "      pSinkGraphNode:   %p\n"
                     "      numBatches:       %u\n"
                     "      numDepsReleased:  %u",
                     m_pInternalLinkData[linkDataIndex]->linkType,
                     m_pInternalLinkData[linkDataIndex]->linkState,
                     m_pInternalLinkData[linkDataIndex]->pSrcGraphNode,
                     m_pInternalLinkData[linkDataIndex]->pSinkGraphNode,
                     m_pInternalLinkData[linkDataIndex]->numBatches,
                     m_pInternalLinkData[linkDataIndex]->numDependenciesReleased);

        for (UINT32 batchIndex = 0;
             batchIndex < m_pInternalLinkData[linkDataIndex]->numDependenciesPerBatchList.size();
             ++batchIndex)
        {
            CHX_LOG_DUMP("      numDepsPerBatch[%u]:%u\n",
                batchIndex, m_pInternalLinkData[linkDataIndex]->numDependenciesPerBatchList[batchIndex]);
        }

        CHX_LOG_DUMP("      Link Request Table:  %p", &(m_pInternalLinkData[linkDataIndex]->linkRequestTable));
        for (UINT32 linkRequestIndex = 0;
             linkRequestIndex < m_pInternalLinkData[linkDataIndex]->linkRequestTable.size();
             ++linkRequestIndex)
        {
            CHX_LOG_DUMP("        Request index %u:",
                         m_pInternalLinkData[linkDataIndex]->linkRequestTable[linkRequestIndex].requestIndex);
            CHX_LOG_DUMP("          Batch index %u:",
                         m_pInternalLinkData[linkDataIndex]->linkRequestTable[linkRequestIndex].batchIndex);
            CHX_LOG_DUMP("          Dependency index %u:",
                         m_pInternalLinkData[linkDataIndex]->linkRequestTable[linkRequestIndex].dependencyIndex);
        }
    }

    CHX_LOG_DUMP("  Sink links (m_pExtSinkLinkData):");
    for (UINT32 linkDataIndex = 0; linkDataIndex < m_pExtSinkLinkData.size(); ++linkDataIndex)
    {
        ChiFeature2GlobalPortInstanceId* pPortId = &(m_pExtSinkLinkData[linkDataIndex]->linkDesc.extSinkLinkDesc.portId);
        CHX_LOG_DUMP("    Sink link with source port {%u, {%u, %u}, {%u, %u, %u, %d, %d}}",
                     pPortId->featureId,
                     pPortId->instanceProps.instanceId,
                     pPortId->instanceProps.cameraId,
                     pPortId->portId.session,
                     pPortId->portId.pipeline,
                     pPortId->portId.port,
                     pPortId->portId.portDirectionType,
                     pPortId->portId.portType);
        CHX_LOG_DUMP("      Type:             %d\n"
                     "      State:            %d\n"
                     "      pSrcGraphNode:    %p\n"
                     "      pSinkGraphNode:   %p\n"
                     "      numBatches:       %u\n"
                     "      numDepsReleased:  %u",
                     m_pExtSinkLinkData[linkDataIndex]->linkType,
                     m_pExtSinkLinkData[linkDataIndex]->linkState,
                     m_pExtSinkLinkData[linkDataIndex]->pSrcGraphNode,
                     m_pExtSinkLinkData[linkDataIndex]->pSinkGraphNode,
                     m_pExtSinkLinkData[linkDataIndex]->numBatches,
                     m_pExtSinkLinkData[linkDataIndex]->numDependenciesReleased);

        for (UINT32 batchIndex = 0;
             batchIndex < m_pExtSinkLinkData[linkDataIndex]->numDependenciesPerBatchList.size();
             ++batchIndex)
        {
            CHX_LOG_DUMP("      numDepsPerBatch[%u]:%u\n",
                batchIndex, m_pExtSinkLinkData[linkDataIndex]->numDependenciesPerBatchList[batchIndex]);
        }

        CHX_LOG_DUMP("      Link Request Table:  %p", &(m_pExtSinkLinkData[linkDataIndex]->linkRequestTable));
        for (UINT32 linkRequestIndex = 0;
             linkRequestIndex < m_pExtSinkLinkData[linkDataIndex]->linkRequestTable.size();
             ++linkRequestIndex)
        {
            CHX_LOG_DUMP("        Request index %u:",
                         m_pExtSinkLinkData[linkDataIndex]->linkRequestTable[linkRequestIndex].requestIndex);
            CHX_LOG_DUMP("          Batch index %u:",
                         m_pExtSinkLinkData[linkDataIndex]->linkRequestTable[linkRequestIndex].batchIndex);
            CHX_LOG_DUMP("          Dependency index %u:",
                         m_pExtSinkLinkData[linkDataIndex]->linkRequestTable[linkRequestIndex].dependencyIndex);
        }
    }

    CHX_LOG_DUMP("  FeatureGraphManager callbacks:");
    CHX_LOG_DUMP("    ChiFeatureGraphProcessResult: %p",
                 &(m_featureGraphManagerCallbacks.ChiFeatureGraphProcessResult));
    CHX_LOG_DUMP("    ChiFeatureGraphProcessMessage: %p",
                 &(m_featureGraphManagerCallbacks.ChiFeatureGraphProcessMessage));
    CHX_LOG_DUMP("    ChiFeatureGraphProcessPartialResult: %p",
                 &(m_featureGraphManagerCallbacks.ChiFeatureGraphProcessPartialResult));
    CHX_LOG_DUMP("    pPrivateCallbackData: %p",
                 &(m_featureGraphManagerCallbacks.pPrivateCallbackData));

    CHX_LOG_DUMP("  External Source Port Id <-> ChiStream Mappings <-> Port Flags:");
    for (UINT32 portIdIndex = 0; portIdIndex < m_extSrcPortIdToChiStreamMap.size(); ++portIdIndex)
    {
        const ChiFeature2GlobalPortInstanceId* pPortId = &(m_extSrcPortIdToChiStreamMap[portIdIndex].portId);
        CHX_LOG_DUMP("    {%u, {%u, %u}, {%u, %u, %u, %d, %d}} <-> %p <-> %d",
                     pPortId->featureId,
                     pPortId->instanceProps.instanceId,
                     pPortId->instanceProps.cameraId,
                     pPortId->portId.session,
                     pPortId->portId.pipeline,
                     pPortId->portId.port,
                     pPortId->portId.portDirectionType,
                     pPortId->portId.portType,
                     m_extSrcPortIdToChiStreamMap[portIdIndex].pChiStream,
                     m_extSrcPortIdToChiStreamMap[portIdIndex].portFlags.inputConfigSet);
    }

    CHX_LOG_DUMP("  External Sink Port Id <-> ChiStream Mappings <-> Port Flags:");
    for (UINT32 portIdIndex = 0; portIdIndex < m_extSinkPortIdToChiStreamMap.size(); ++portIdIndex)
    {
        const ChiFeature2GlobalPortInstanceId* pPortId = &(m_extSinkPortIdToChiStreamMap[portIdIndex].portId);
        CHX_LOG_DUMP("    {%u, {%u, %u}, {%u, %u, %u, %d}} <-> %p <-> %d",
                     pPortId->featureId,
                     pPortId->instanceProps.instanceId,
                     pPortId->instanceProps.cameraId,
                     pPortId->portId.session,
                     pPortId->portId.pipeline,
                     pPortId->portId.port,
                     pPortId->portId.portType,
                     m_extSinkPortIdToChiStreamMap[portIdIndex].pChiStream,
                     (false == m_extSrcPortIdToChiStreamMap.empty()) ?
                      m_extSrcPortIdToChiStreamMap[portIdIndex].portFlags.outputResultNotified : -1);
    }

    CHX_LOG_DUMP("  Usecase Request Object Input Stream Data:");
    for (UINT32 inputStreamDataIndex = 0;
         inputStreamDataIndex < m_usecaseRequestObjInputStreamData.size();
         ++inputStreamDataIndex)
    {
        CHX_LOG_DUMP("    numInputs: %u, pChiStream: %p",
                     m_usecaseRequestObjInputStreamData[inputStreamDataIndex].numInputs,
                     m_usecaseRequestObjInputStreamData[inputStreamDataIndex].pChiStream);
    }

    if (NULL != m_pUsecaseRequestObj)
    {
        m_pUsecaseRequestObj->Dump(fd);
    }
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Private Methods
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::~ChiFeature2Graph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Graph::~ChiFeature2Graph()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ValidateFeatureGraphDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Graph::ValidateFeatureGraphDescriptor(
    ChiFeature2GraphCreateInputInfo* pFeatureGraphCreateInputInfo)
{
    BOOL                            isValid             = TRUE;
    ChiFeature2GraphDesc*           pFeatureGraphDesc   = pFeatureGraphCreateInputInfo->pFeatureGraphDesc;
    ChiFeature2GlobalPortInstanceId portId              = {0};

    // Create a set of global port instance ids in order to efficiently determine any duplicates
    std::set<ChiFeature2GlobalPortInstanceId, globalPortInstanceIdLessComparator> globalPortInstanceIds;

    // Each input port must have no more than one source link attached to it
    for (UINT32 link = 0; link < pFeatureGraphDesc->numExtSrcLinks; ++link)
    {
        portId              = pFeatureGraphDesc->pExtSrcLinks[link].portId;
        auto insertResult   = globalPortInstanceIds.insert(portId);
        if (FALSE == insertResult.second)
        {
            // We found two source links connected to the same input port
            CHX_LOG_ERROR("Must have <=1 source links connected to a single input port: Port id: "
                          "{%u, {%u, %u}, {%u, %u, %u, %d, %d}}, pSrcLink:%p",
                          portId.featureId,
                          portId.instanceProps.instanceId, portId.instanceProps.cameraId,
                          portId.portId.session, portId.portId.pipeline, portId.portId.port,
                          portId.portId.portDirectionType, portId.portId.portType,
                          &(pFeatureGraphDesc->pExtSrcLinks[link]));
            isValid = FALSE;
            break;
        }
        else
        {
            CHX_LOG_VERBOSE("Source links connected to a single input port: Port id: "
                            "{%u, {%u, %u}, {%u, %u, %u, %d, %d}}, pSrcLink=%p",
                            portId.featureId,
                            portId.instanceProps.instanceId, portId.instanceProps.cameraId,
                            portId.portId.session, portId.portId.pipeline, portId.portId.port,
                            portId.portId.portDirectionType, portId.portId.portType,
                            &(pFeatureGraphDesc->pExtSrcLinks[link]));
        }
    }

    if (TRUE == isValid)
    {
        // Each input port must have no more than one source or internal link attached to it, so add internal links to the set
        for (UINT32 link = 0; link < pFeatureGraphDesc->numInternalLinks; ++link)
        {
            portId              = pFeatureGraphDesc->pInternalLinks[link].sinkPortId;
            auto insertResult   = globalPortInstanceIds.insert(portId);
            if (FALSE == insertResult.second)
            {
                // We found two links connected to the same input port
                CHX_LOG_ERROR("Must have <=1 links connected to a single input port. Port id: {%u, {%u, %u}, {%u, %u, %u, %d}}",
                              portId.featureId,
                              portId.instanceProps.instanceId, portId.instanceProps.cameraId,
                              portId.portId.session, portId.portId.pipeline, portId.portId.port,
                              portId.portId.portDirectionType);
                isValid = FALSE;
                break;
            }
        }
    }

    // Clear the set so that we only look at output ports now
    globalPortInstanceIds.clear();

    if (TRUE == isValid)
    {
        // Each output port must have no more than one sink link attached to it
        for (UINT32 link = 0; link < pFeatureGraphDesc->numExtSinkLinks; ++link)
        {
            portId              = pFeatureGraphDesc->pExtSinkLinks[link].portId;
            auto insertResult   = globalPortInstanceIds.insert(portId);
            if (FALSE == insertResult.second)
            {
                // We found two sink links connected to the same output port
                CHX_LOG_ERROR("Must have <=1 sink links connected to a single output port. Port id: "
                              "{%u, {%u, %u}, {%u, %u, %u, %d, %d}}",
                              portId.featureId,
                              portId.instanceProps.instanceId, portId.instanceProps.cameraId,
                              portId.portId.session, portId.portId.pipeline, portId.portId.port,
                              portId.portId.portDirectionType, portId.portId.portType);
                isValid = FALSE;
                break;
            }
        }
    }

    if (TRUE == isValid)
    {
        // If the output port has a sink link attached to it, we currently do not allow any internal links to be attached.
        // Keep the set as only sink output ports, meaning do not add the internal output port to the set because we allow
        // multiple internal output links per internal output port.
        for (UINT32 link = 0; link < pFeatureGraphDesc->numInternalLinks; ++link)
        {
            portId              = pFeatureGraphDesc->pInternalLinks[link].srcPortId;
            auto insertResult   = globalPortInstanceIds.find(portId);
            if (insertResult != globalPortInstanceIds.end())
            {
                // We found an internal link connected to the same output port as a sink link
                CHX_LOG_ERROR("Output port already has a sink link attached; additional internal links may not be attached. "
                              "Port id: {%u, {%u, %u}, {%u, %u, %u, %d, %d}}",
                              portId.featureId,
                              portId.instanceProps.instanceId, portId.instanceProps.cameraId,
                              portId.portId.session, portId.portId.pipeline, portId.portId.port,
                              portId.portId.portDirectionType, portId.portId.portType);
                isValid = FALSE;
                break;
            }
        }
    }

    return isValid;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::Initialize(
    ChiFeature2GraphCreateInputInfo* pFeatureGraphCreateInputInfo)
{
    CDKResult result = CDKResultSuccess;

    // Set vectors to default values
    m_linkData.reserve(NumDefaultLinks);
    m_pExtSrcLinkData.reserve(NumDefaultLinks);
    m_pInternalLinkData.reserve(NumDefaultLinks);
    m_pExtSinkLinkData.reserve(NumDefaultLinks);
    m_pFeatureGraphNodes.reserve(NumDefaultGraphNodes);
    m_pFeatureGraphCallbackData.reserve(NumDefaultGraphNodes);

    // Cache feature graph manager callbacks for later use
    if (NULL != pFeatureGraphCreateInputInfo->pFeatureGraphManagerCallbacks)
    {
        m_featureGraphManagerCallbacks = *pFeatureGraphCreateInputInfo->pFeatureGraphManagerCallbacks;
    }
    else
    {
        CHX_LOG_ERROR("Feature Graph Manager Callbacks is NULL");
        result = CDKResultEInvalidPointer;
    }

    if (CDKResultSuccess == result)
    {
        // Copy input port/output port maps directly
        m_extSrcPortIdToChiStreamMap    = pFeatureGraphCreateInputInfo->extSrcPortIdToChiStreamMap;
        m_extSinkPortIdToChiStreamMap   = pFeatureGraphCreateInputInfo->extSinkPortIdToChiStreamMap;
        m_pFeatureGraphName             = pFeatureGraphCreateInputInfo->pFeatureGraphDesc->pFeatureGraphName;

        UINT32 frameNumber = pFeatureGraphCreateInputInfo->pUsecaseRequestObject->GetAppFrameNumber();

        CdkUtils::SNPrintF(m_identifierString, sizeof(m_identifierString), "Graph-URO:%d_Graph:%s",
            frameNumber, m_pFeatureGraphName);

        auto& rGraphId = m_identifierString;
        BINARY_LOG(LogEvent::FT2_Graph_Init, rGraphId, frameNumber);
        CHX_LOG_INFO("%s created with features:", IdentifierString());
        for (UINT featureIndex = 0; featureIndex < pFeatureGraphCreateInputInfo->featureInstanceReqInfoList.size();
             ++featureIndex)
        {
            ChiFeature2Base* pFeatureBase      = pFeatureGraphCreateInputInfo->featureInstanceReqInfoList[featureIndex]
                                                    .pFeatureBase;
            UINT32           featureId         = pFeatureBase->GetFeatureId();
            UINT32           featureInstanceId = pFeatureBase->GetInstanceProps()->instanceId;

            BINARY_LOG(LogEvent::FT2_Graph_FeatureInit, featureId, featureInstanceId, frameNumber);
            CHX_LOG_INFO("  featureName = %s_%d",
                         pFeatureBase->GetFeatureName(),
                         pFeatureBase->GetInstanceProps()->instanceId);
        }

        // Populate link data with feature graph descriptor info
        ChiFeature2GraphDesc*       pFeatureGraphDesc = pFeatureGraphCreateInputInfo->pFeatureGraphDesc;
        ChiFeature2GraphLinkData    linkData;

        // Create all link data structures for external source links
        for (UINT32 linkIndex = 0; linkIndex < pFeatureGraphDesc->numExtSrcLinks; ++linkIndex)
        {
            ChxUtils::Memset(&linkData, 0, sizeof(ChiFeature2GraphLinkData));
            ChxUtils::Memcpy(&(linkData.linkDesc.extSrcLinkDesc),
                             &(pFeatureGraphDesc->pExtSrcLinks[linkIndex]),
                             sizeof(linkData.linkDesc.extSrcLinkDesc));

            linkData.linkState                  = ChiFeature2GraphLinkState::NotVisited;
            linkData.linkType                   = ChiFeature2GraphLinkType::ExternalSource;
            linkData.numBatches                 = 0;
            linkData.numDependenciesReleased    = 0;
            m_linkData.push_back(linkData);
            m_pExtSrcLinkData.push_back(&(m_linkData.back()));
        }

        // Create all link data structures for internal links
        for (UINT32 link = 0; link < pFeatureGraphDesc->numInternalLinks; ++link)
        {
            ChxUtils::Memset(&linkData, 0, sizeof(ChiFeature2GraphLinkData));
            ChxUtils::Memcpy(&(linkData.linkDesc.internalLinkDesc),
                             &(pFeatureGraphDesc->pInternalLinks[link]),
                             sizeof(linkData.linkDesc.internalLinkDesc));

            linkData.linkState                  = ChiFeature2GraphLinkState::NotVisited;
            linkData.linkType                   = ChiFeature2GraphLinkType::Internal;
            linkData.numBatches                 = 0;
            linkData.numDependenciesReleased    = 0;
            m_linkData.push_back(linkData);
            m_pInternalLinkData.push_back(&(m_linkData.back()));
        }

        // Create all link data structures for external sink links
        for (UINT32 link = 0; link < pFeatureGraphDesc->numExtSinkLinks; ++link)
        {
            ChxUtils::Memset(&linkData, 0, sizeof(ChiFeature2GraphLinkData));
            ChxUtils::Memcpy(&(linkData.linkDesc.extSinkLinkDesc),
                             &(pFeatureGraphDesc->pExtSinkLinks[link]),
                             sizeof(linkData.linkDesc.extSinkLinkDesc));

            linkData.linkState                  = ChiFeature2GraphLinkState::NotVisited;
            linkData.linkType                   = ChiFeature2GraphLinkType::ExternalSink;
            linkData.numBatches                 = 0;
            linkData.numDependenciesReleased    = 0;
            m_linkData.push_back(linkData);
            m_pExtSinkLinkData.push_back(&(m_linkData.back()));
        }

        // Iterate through all feature instances to populate graph nodes
        for (UINT32 pFeatureBaseObjIndex = 0;
             pFeatureBaseObjIndex < pFeatureGraphCreateInputInfo->featureInstanceReqInfoList.size();
             ++pFeatureBaseObjIndex)
        {
            ChiFeature2GraphNode* pGraphNode = CHX_NEW ChiFeature2GraphNode();
            if (NULL == pGraphNode)
            {
                CHX_LOG_ERROR("No memory, pGraphNode is NULL");
                result = CDKResultENoMemory;
                break;
            }
            else
            {
                ChiFeature2InstanceRequestInfo* pFeatureInstanceReqInfo =
                    &(pFeatureGraphCreateInputInfo->featureInstanceReqInfoList[pFeatureBaseObjIndex]);

                pGraphNode->featureId       = pFeatureInstanceReqInfo->pFeatureBase->GetFeatureId();
                pGraphNode->instanceProps   = *(pFeatureInstanceReqInfo->pFeatureBase->GetInstanceProps());
                pGraphNode->pFeatureBaseObj = pFeatureInstanceReqInfo->pFeatureBase;
                pGraphNode->featureHint     = pFeatureInstanceReqInfo->featureHint;

                // Iterate through all of the feature's external ports to map ports to link indices
                std::vector<ChiFeature2Identifier> externalGlobalPortIdList =
                    pGraphNode->pFeatureBaseObj->GetExternalGlobalPortIdList();
                for (UINT32 portIdIndex = 0; portIdIndex < externalGlobalPortIdList.size(); ++portIdIndex)
                {
                    ChiFeature2GlobalPortInstanceId portInstanceId = CreateGlobalPortInstanceId(
                        pGraphNode->featureId,
                        pGraphNode->instanceProps,
                        externalGlobalPortIdList[portIdIndex]);

                    const ChiFeature2PortDescriptor* pPortDesc =
                        pFeatureInstanceReqInfo->pFeatureBase->GetPortDescriptorFromPortId(
                            &externalGlobalPortIdList[portIdIndex]);

                    BOOL matchFound = FALSE;

                    // Iterate through all link indices to find matching port
                    for (UINT linkIndex = 0; linkIndex < m_linkData.size(); ++linkIndex)
                    {
                        if (ChiFeature2GraphLinkType::ExternalSource == m_linkData[linkIndex].linkType)
                        {
                            if (portInstanceId == m_linkData[linkIndex].linkDesc.extSrcLinkDesc.portId)
                            {
                                // We have an external source link connected to an input port
                                pGraphNode->portToInputLinkIndexMap.push_back({portInstanceId, linkIndex});
                                m_linkData[linkIndex].pSinkGraphNode = pGraphNode;

                                matchFound = TRUE;
                            }
                        }
                        else if (ChiFeature2GraphLinkType::Internal == m_linkData[linkIndex].linkType)
                        {
                            if (portInstanceId == m_linkData[linkIndex].linkDesc.internalLinkDesc.srcPortId)
                            {
                                // We have an internal link connected to an output port
                                pGraphNode->portToOutputLinkIndexMap.push_back({portInstanceId, linkIndex});
                                m_linkData[linkIndex].pSrcGraphNode = pGraphNode;

                                matchFound = TRUE;
                            }
                            else if (portInstanceId == m_linkData[linkIndex].linkDesc.internalLinkDesc.sinkPortId)
                            {
                                // We have an internal link connected to an input port
                                pGraphNode->portToInputLinkIndexMap.push_back({portInstanceId, linkIndex});
                                m_linkData[linkIndex].pSinkGraphNode = pGraphNode;

                                matchFound = TRUE;
                            }
                        }
                        else if (ChiFeature2GraphLinkType::ExternalSink == m_linkData[linkIndex].linkType)
                        {
                            if (portInstanceId == m_linkData[linkIndex].linkDesc.extSinkLinkDesc.portId)
                            {
                                // We have an external sink link connected to an output port
                                pGraphNode->portToOutputLinkIndexMap.push_back({portInstanceId, linkIndex});
                                m_linkData[linkIndex].pSrcGraphNode = pGraphNode;

                                matchFound = TRUE;
                            }
                        }
                    }

                    if (FALSE == matchFound)
                    {
                        CHX_LOG_INFO("%s: External port {%s: %u, {%u, %u}, {%u, %u, %u, %d, %d}} not connected",
                                     IdentifierString(),
                                     (pPortDesc != NULL ) ? pPortDesc->pPortName : "NULL",
                                     portInstanceId.featureId,
                                     portInstanceId.instanceProps.instanceId,
                                     portInstanceId.instanceProps.cameraId,
                                     portInstanceId.portId.session,
                                     portInstanceId.portId.pipeline,
                                     portInstanceId.portId.port,
                                     portInstanceId.portId.portDirectionType,
                                     portInstanceId.portId.portType);
                    }
                }

                m_pFeatureGraphNodes.push_back(pGraphNode);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::WalkAllExtSrcLinks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::WalkAllExtSrcLinks(
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObj)
{
    CDKResult result = CDKResultSuccess;

    // Iterate through all source links to populate buffer/metadata info and submit request to continue processing
    UINT8 numSrcLinks = m_pExtSrcLinkData.size();
    CHX_LOG_INFO("%s: Walking %u source links", IdentifierString(), numSrcLinks);
    for (UINT32 linkDataIndex = 0; linkDataIndex < numSrcLinks; ++linkDataIndex)
    {
        ChiFeature2GraphLinkData* pExtSrcLinkData = m_pExtSrcLinkData[linkDataIndex];
        if (NULL != pExtSrcLinkData)
        {
            // Get associated input port
            ChiFeature2GlobalPortInstanceId inputPortId = pExtSrcLinkData->linkDesc.extSrcLinkDesc.portId;

            // Get the downstream feature request object
            ChiFeature2RequestObject* pFeatureRequestObj = pExtSrcLinkData->pSinkGraphNode->pFeatureRequestObjList.back();
            if (NULL == pFeatureRequestObj)
            {
                CHX_LOG_ERROR("pFeatureRequestObj is NULL");
                result = CDKResultEInvalidState;
            }
            else
            {
                // Populate incoming stream info to source ports

                // First, find the ChiStream associated with the input port
                ChiStream* pChiStream = NULL;
                UINT32     numInputs  = 0;
                result = GetChiStreamForRequestedPort(inputPortId, &pChiStream, numInputs);

                if (CDKResultSuccess == result)
                {
                    if (NULL != pChiStream)
                    {
                        if (ChiFeature2PortType::ImageBuffer == inputPortId.portId.portType)
                        {
                            std::vector<CHITARGETBUFFERINFOHANDLE> inputBufferTBHs =
                                pUsecaseRequestObj->GetInputTargetBufferHandles(pChiStream);
                            UINT32 bufferIndex                                     = 0;
                            if ((TRUE != inputBufferTBHs.empty()) && (bufferIndex < inputBufferTBHs.size()))
                            {
                                for (UINT32 batchIndex = 0 ; batchIndex < pExtSrcLinkData->numBatches; batchIndex++)
                                {
                                    for (UINT32 dependencyIndex = 0;
                                         dependencyIndex < pExtSrcLinkData->numDependenciesPerBatchList[batchIndex];
                                         dependencyIndex++)
                                    {
                                        pFeatureRequestObj->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                                                                          &(inputPortId.portId),
                                                                          inputBufferTBHs[bufferIndex],
                                                                          reinterpret_cast<UINT64>(pChiStream),
                                                                          FALSE,
                                                                          batchIndex,
                                                                          dependencyIndex);

                                        CHX_LOG_INFO("Setting bufferTBHs = %p, batchIndex = %d,"
                                                     "dependencyIndex = %d, port=%d, porttype = %d size=%zu bufferIndex=%d",
                                                     inputBufferTBHs[bufferIndex], batchIndex,
                                                     dependencyIndex, inputPortId.portId.port,
                                                     inputPortId.portId.portType,
                                                     inputBufferTBHs.size(),
                                                     bufferIndex);
                                        bufferIndex++;
                                    }
                                }
                            }
                        }
                        else
                        {
                            UINT32 metadataIndex    = 0;
                            UINT32 metadataClientId = pUsecaseRequestObj->GetMetadataClientId();

                            std::vector<CHITARGETBUFFERINFOHANDLE> inputMetadataTBHs =
                                pUsecaseRequestObj->GetInputMetadataHandles(pChiStream);

                            if ((TRUE != inputMetadataTBHs.empty()) && (metadataIndex < inputMetadataTBHs.size()))
                            {
                                for (UINT32 batchIndex = 0 ; batchIndex < pExtSrcLinkData->numBatches; batchIndex++)
                                {
                                    for (UINT32 dependencyIndex = 0;
                                         dependencyIndex < pExtSrcLinkData->numDependenciesPerBatchList[batchIndex];
                                         dependencyIndex++)
                                    {
                                        pFeatureRequestObj->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                                                                          &(inputPortId.portId),
                                                                          inputMetadataTBHs[metadataIndex],
                                                                          metadataClientId,
                                                                          FALSE,
                                                                          batchIndex,
                                                                          dependencyIndex);

                                        CHX_LOG_INFO("Setting metadataTBHs = %p, batchIndex=%d,"
                                                     "dependencyIndex = %d, port=%d, porttype = %d metadataIndex=%d",
                                                     inputMetadataTBHs[metadataIndex], batchIndex,
                                                     dependencyIndex,
                                                     inputPortId.portId.port,
                                                     inputPortId.portId.portType,
                                                     metadataIndex);
                                        metadataIndex++;
                                    }
                                }
                            }
                        }

                        // Get the base feature associated with the downstream feature graph node
                        ChiFeature2Base* pFeatureBase = pExtSrcLinkData->pSinkGraphNode->pFeatureBaseObj;

                        // Continue the walk-forward
                        result = pFeatureBase->ProcessRequest(pFeatureRequestObj);
                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("pFeatureBase->ProcessRequest() failed with result %d", result);
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("GetChiStreamForRequestedPort() failed with result %d", result);
                }
            }
        }
    }

    // Indicate to client that we are now expecting output
    result = pUsecaseRequestObj->SetRequestState(ChiFeature2UsecaseRequestObjectState::OutputPending);
    if (CDKResultSuccess != result)
    {
        CHX_LOG_ERROR("pUsecaseRequestObj->SetRequestState(OutputPending) failed with result %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::WalkAllExtSinkLinks
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::WalkAllExtSinkLinks(
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObj)
{
    CDKResult result = CDKResultSuccess;

    // Iterate through all sink links and walk back through feature message to register callbacks and data
    UINT8 numSinkLinks = m_pExtSinkLinkData.size();
    CHX_LOG_INFO("%s: Walking %u sink links", IdentifierString(), numSinkLinks);
    for (UINT32 extSinkLinkIndex = 0; extSinkLinkIndex < numSinkLinks; ++extSinkLinkIndex)
    {
        ChiFeature2GraphLinkData* pExtSinkLinkData = m_pExtSinkLinkData[extSinkLinkIndex];

        // If the link has never been visited, check whether it is in the port->stream map. If not, set the link to Disabled.
        // If so, set the link to OutputPending.
        if (ChiFeature2GraphLinkState::NotVisited == pExtSinkLinkData->linkState)
        {
            pExtSinkLinkData->linkState = ChiFeature2GraphLinkState::Disabled;
            for (UINT8 portIndex = 0; portIndex < m_extSinkPortIdToChiStreamMap.size(); ++portIndex)
            {
                if ((pExtSinkLinkData->linkDesc.extSrcLinkDesc.portId == m_extSinkPortIdToChiStreamMap[portIndex].portId) ||
                    (ChiFeature2PortType::MetaData == pExtSinkLinkData->linkDesc.extSinkLinkDesc.portId.portId.portType))
                {
                    pExtSinkLinkData->linkState = ChiFeature2GraphLinkState::OutputPending;
                    break;
                }
            }
        }

        // Only one request from client, with no associated batch/dependency info, so initialize indices to 0 and assume 1
        // request output.
        ChiFeature2GraphLinkRequestMap linkRequestMap = {0};
        pExtSinkLinkData->linkRequestTable.push_back(linkRequestMap);

        // If the link is associated with a ChiStream, then process the input dependences from the URO
        if (ChiFeature2GraphLinkState::OutputPending == pExtSinkLinkData->linkState)
        {
            // Process the input dependencies for the upstream feature request
            result = ProcessInputDepsForUpstreamFeatureRequest(m_pUsecaseRequestObj, pExtSinkLinkData, 0);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("ProcessInputDepsForUpstreamFeatureRequest failed with result %d", result);
            }
        }

        // Walk back to the source node of the sink link, checking all of its output links to determine next step
        result = WalkBackFromLink(pUsecaseRequestObj, pExtSinkLinkData);
        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("WalkBackFromLink() failed with result %d", result);
            break;
        }
    }

    if (CDKResultSuccess == result)
    {
        // All nodes reachable from sink links have now been visited. If there is any aggregated input configuration collected,
        // populate it into usecase request object and return back to client.
        if (m_usecaseRequestObjInputStreamData.size() > 0)
        {
            result = pUsecaseRequestObj->SetInputConfigurations(m_usecaseRequestObjInputStreamData);
            if (CDKResultSuccess == result)
            {
                // Indicate to client that we are now expecting input dependencies to be satisfied
                result = pUsecaseRequestObj->SetRequestState(ChiFeature2UsecaseRequestObjectState::InputConfigPending);
                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("pUsecaseRequestObj->SetRequestState(InputConfigPending) failed with result %d", result);
                }
            }
        }
        else
        {
            // In flush case, URO is moved to complete within processrequest sequence itself
            if (pUsecaseRequestObj->GetRequestState() != ChiFeature2UsecaseRequestObjectState::Complete)
            {
                // Indicate to client that we are now expecting output
                result = pUsecaseRequestObj->SetRequestState(ChiFeature2UsecaseRequestObjectState::OutputPending);

                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("pUsecaseRequestObj->SetRequestState(OutputPending) failed with result %d", result);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::WalkBackFromLink
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::WalkBackFromLink(
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj,
    ChiFeature2GraphLinkData*           pOutputLinkData)
{
    CDKResult result = CDKResultSuccess;

    // Walk back to link's source node if this is not a source link
    if (ChiFeature2GraphLinkType::ExternalSource != pOutputLinkData->linkType)
    {
        ChiFeature2GraphNode* pSrcGraphNode = pOutputLinkData->pSrcGraphNode;

        // If all output links are disabled, iteratively disable each input link, and recursively walk back on each input link
        if (TRUE == CheckAllOutputLinksDisabled(pSrcGraphNode))
        {
            for (UINT32 portIdIndex = 0; portIdIndex < pSrcGraphNode->portToInputLinkIndexMap.size(); ++portIdIndex)
            {
                ChiFeature2GlobalPortInstanceId* pInputPortId = &(pSrcGraphNode->portToInputLinkIndexMap[portIdIndex].portId);

                // Map input port id to input link data
                ChiFeature2GraphLinkData* pInputLinkData = MapInputPortIdToInputLinkData(pInputPortId, pSrcGraphNode);
                if (NULL == pInputLinkData)
                {
                    CHX_LOG_ERROR("Cannot find input link corresponding to port id {%u, {%u, %u}, {%u, %u, %u, %d}}",
                                  pInputPortId->featureId,
                                  pInputPortId->instanceProps.instanceId, pInputPortId->instanceProps.cameraId,
                                  pInputPortId->portId.session, pInputPortId->portId.pipeline,
                                  pInputPortId->portId.port, pInputPortId->portId.portType);
                }
                else
                {
                    // Disable the input link since no output is needed
                    pInputLinkData->linkState = ChiFeature2GraphLinkState::Disabled;

                    // Recursively walk back to input link's source node
                    result = WalkBackFromLink(pUsecaseRequestObj, pInputLinkData);
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("WalkBackFromLink() failed with result %d", result);
                        break;
                    }
                }
            }
        }
        // If all output links are ready to submit a feature request object, proceed with walk-back
        else if (TRUE == CheckAllOutputLinksReadyToProcessRequest(pSrcGraphNode))
        {
            // Submit the feature request object, if all links have been visited
            result = ProcessUpstreamFeatureRequest(pUsecaseRequestObj, pOutputLinkData->pSrcGraphNode);
            if (CDKResultSuccess != result)
            {
                CHX_LOG_ERROR("ProcessUpstreamFeatureRequest() failed with result %d", result);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessUsecaseRequestObjectInputStreamData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessUsecaseRequestObjectInputStreamData(
    ChiFeature2GraphLinkData*        pInputLinkData,
    ChiFeature2GlobalPortInstanceId  extSrcPortId,
    CHIMETAHANDLE                    hSettings)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pInputLinkData)
    {
        CHX_LOG_ERROR("pInputLinkData is NULL");
        result = CDKResultEInvalidArg;
    }
    else if (ChiFeature2GraphLinkState::Disabled == pInputLinkData->linkState)
    {
        CHX_LOG_ERROR("pLinkData->linkState cannot be Disabled");
        result = CDKResultEInvalidArg;
    }
    else if (ChiFeature2GraphLinkType::ExternalSource != pInputLinkData->linkType)
    {
        CHX_LOG_ERROR("pLinkData->linkType must be ExternalSource");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        // Store input stream configuration for URO to internal data structures. First look for ChiStream
        // corresponding to this input port
        ChiStream* pChiStream = NULL;
        for (UINT32 portIdIndex = 0; portIdIndex < m_extSrcPortIdToChiStreamMap.size(); ++portIdIndex)
        {
            if (extSrcPortId == m_extSrcPortIdToChiStreamMap[portIdIndex].portId)
            {
                pChiStream = m_extSrcPortIdToChiStreamMap[portIdIndex].pChiStream;
                break;
            }
        }

        // Check if there is a valid ChiStream associated with the source link. If not, disable the link.
        // Metadata and image buffers are coupled together. Update them only once.
        if (ChiFeature2PortType::ImageBuffer == extSrcPortId.portId.portType)
        {
            if (NULL == pChiStream)
            {
                pInputLinkData->linkState = ChiFeature2GraphLinkState::Disabled;
                CHX_LOG_ERROR("Could not find corresponding ChiStream pointer; link disabled");
                result = CDKResultENoSuch;
            }
            else
            {
                BOOL updatedInputStreamData = FALSE;

                // Next check if m_usecaseRequestObjectInputStreamData already has an entry for this ChiStream. If
                // so, update it. If not, add it to the list.
                for (UINT32 inputStreamDataIndex = 0;
                     inputStreamDataIndex < m_usecaseRequestObjInputStreamData.size();
                     ++inputStreamDataIndex)
                {
                    if (pChiStream == m_usecaseRequestObjInputStreamData[inputStreamDataIndex].pChiStream)
                    {
                        m_usecaseRequestObjInputStreamData[inputStreamDataIndex].inputSettings.push_back(hSettings);

                        m_usecaseRequestObjInputStreamData[inputStreamDataIndex].numInputs += 1;
                        updatedInputStreamData                                              = TRUE;
                    }
                }

                if (FALSE == updatedInputStreamData)
                {
                    ChiFeature2UsecaseRequestObjectExtSrcStreamData   inputStreamData = {0};
                    inputStreamData.inputSettings.push_back(hSettings);

                    inputStreamData.numInputs  = 1;
                    inputStreamData.pChiStream = pChiStream;

                    m_usecaseRequestObjInputStreamData.push_back(inputStreamData);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessInputDepsForUpstreamFeatureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessInputDepsForUpstreamFeatureRequest(
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj,
    ChiFeature2GraphLinkData*           pLinkData,
    UINT8                               requestIndex)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pUsecaseRequestObj)
    {
        CHX_LOG_ERROR("pUsecaseRequestObj is NULL");
        result = CDKResultEInvalidArg;
    }

    if (NULL == pLinkData)
    {
        CHX_LOG_ERROR("pLinkData is NULL");
        result = CDKResultEInvalidArg;
    }
    else if (ChiFeature2GraphLinkState::Disabled == pLinkData->linkState)
    {
        CHX_LOG_ERROR("pLinkData->linkState cannot be Disabled");
        result = CDKResultEInvalidArg;
    }
    else if (ChiFeature2GraphLinkType::ExternalSource == pLinkData->linkType)
    {
        CHX_LOG_ERROR("pLinkData->linkType cannot be ExternalSource");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2GraphNode*   pUpstreamGraphNode      = pLinkData->pSrcGraphNode;
        ChiFeature2GraphNode*   pDownstreamGraphNode    = pLinkData->pSinkGraphNode;

        // Make sure we have a valid request entry corresponding to the request index
        while (requestIndex >= pUpstreamGraphNode->requestTable.size())
        {
            std::vector<ChiFeature2RequestOutputInfo> requestOutputs;
            pUpstreamGraphNode->requestTable.push_back(requestOutputs);
        }

        // Look up the existing request entry to add additional port dependency entry
        ChiFeature2RequestOutputInfo requestOutputInfo = {0};
        pUpstreamGraphNode->requestTable[requestIndex].push_back(requestOutputInfo);

        // Populate data needed for feature request object creation
        ChiFeature2RequestOutputInfo*   pRequestOutputInfo  = &(pUpstreamGraphNode->requestTable[requestIndex].back());
        ChiFeature2Identifier*          pSrcPortId          = NULL;
        ChiFeature2Base*                pSrcFeatureBase     = pUpstreamGraphNode->pFeatureBaseObj;

        // Data for result propagation
        ChiStream* pChiStream = NULL;

        switch (pLinkData->linkType)
        {
            case ChiFeature2GraphLinkType::Internal:
            {
                // Internal, so no TBH needed
                pRequestOutputInfo->bufferMetadataInfo.hBuffer    = NULL;
                // Internal, so no ChiStream needed
                pRequestOutputInfo->bufferMetadataInfo.key        = 0;
                // Feature will manage outputting hMetadata
                pRequestOutputInfo->bufferMetadataInfo.hMetadata  = NULL;

                ChiFeature2RequestObject* pDownstreamFeatureRequestObj =
                    pDownstreamGraphNode->pFeatureRequestObjList.back();

                // Add the downstream FRO to the upstream node's list if it is not already in it
                BOOL matchFound = FALSE;
                for (UINT featureRequestObjIndex = 0;
                     featureRequestObjIndex < pUpstreamGraphNode->downstreamFeatureRequestObjMap.size();
                     ++featureRequestObjIndex)
                {
                    if (pDownstreamFeatureRequestObj ==
                        pUpstreamGraphNode->downstreamFeatureRequestObjMap[featureRequestObjIndex].pFeatureRequestObj)
                    {
                        matchFound = TRUE;
                        break;
                    }
                }

                if (FALSE == matchFound)
                {
                    ChiFeature2RequestObjectMap pDownstreamFeatureRequestObjectMap =
                    {
                        pDownstreamGraphNode,
                        pDownstreamFeatureRequestObj
                    };

                    pUpstreamGraphNode->downstreamFeatureRequestObjMap.push_back(pDownstreamFeatureRequestObjectMap);
                }

                // Update metadata with downstream metadata
                pSrcPortId = &(pLinkData->linkDesc.internalLinkDesc.srcPortId.portId);
                if (ChiFeature2PortType::MetaData == pSrcPortId->portType)
                {
                    ChiFeature2Identifier*    pSinkPortId            = NULL;

                    pSinkPortId = &(pLinkData->linkDesc.internalLinkDesc.sinkPortId.portId);
                    if ((NULL != pDownstreamFeatureRequestObj) && (NULL != pSinkPortId))
                    {
                        CHIMETAHANDLE                   hMetadata       = NULL;
                        ChiFeature2GraphLinkRequestMap* pLinkRequestMap =
                            &pLinkData->linkRequestTable.back();
                        if (NULL != pLinkRequestMap)
                        {
                            pDownstreamFeatureRequestObj->GetRequestInputInfo(
                                ChiFeature2SequenceOrder::Current,
                                pSinkPortId,
                                &hMetadata,
                                pLinkRequestMap->dependencyIndex,
                                pLinkRequestMap->batchIndex);
                        }

                        if (NULL != hMetadata)
                        {
                            pRequestOutputInfo->bufferMetadataInfo.hMetadata = hMetadata;
                        }
                    }
                }

                break;
            }

            case ChiFeature2GraphLinkType::ExternalSink:
                pSrcPortId = &(pLinkData->linkDesc.extSinkLinkDesc.portId.portId);

                if (pSrcPortId->portType != ChiFeature2PortType::MetaData)
                {
                    // Find the ChiStream associated with the sink link we are processing
                    for (UINT32 portIdIndex = 0; portIdIndex < m_extSinkPortIdToChiStreamMap.size(); ++portIdIndex)
                    {
                        if (pLinkData->linkDesc.extSinkLinkDesc.portId == m_extSinkPortIdToChiStreamMap[portIdIndex].portId)
                        {
                            pChiStream = m_extSinkPortIdToChiStreamMap[portIdIndex].pChiStream;
                            break;
                        }
                    }

                    // Check if there is a valid ChiStream associated with the sink link. If not, disable the link.
                    if (NULL == pChiStream)
                    {
                        pLinkData->linkState = ChiFeature2GraphLinkState::Disabled;
                        CHX_LOG_ERROR("Could not find corresponding ChiStream pointer; link disabled");
                        result = CDKResultENoSuch;
                    }
                    else
                    {
                        // TBH mapped to ChiStream
                        pRequestOutputInfo->bufferMetadataInfo.hBuffer   =
                            pUsecaseRequestObj->GetOutputTargetBufferHandle(pChiStream);
                        // ChiStream pointer from Usecase
                        pRequestOutputInfo->bufferMetadataInfo.key       =
                            reinterpret_cast<UINT64>(pChiStream);
                        // Feature will manage hMetadata output buffer
                        pRequestOutputInfo->bufferMetadataInfo.hMetadata = NULL;
                    }
                }

                break;

            case ChiFeature2GraphLinkType::ExternalSource:
                CHX_LOG_ERROR("pLinkData->linkType cannot be ExternalSource");
                result = CDKResultEInvalidState;
                break;

            default:
                CHX_LOG_ERROR("Invalid link type:%d", pLinkData->linkType);
                result = CDKResultENoSuch;
                break;
        }

        if (CDKResultSuccess == result)
        {
            pRequestOutputInfo->pPortDescriptor = pSrcFeatureBase->GetPortDescriptorFromPortId(pSrcPortId);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessUpstreamFeatureRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessUpstreamFeatureRequest(
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj,
    ChiFeature2GraphNode*               pGraphNode)
{
    CDKResult result = CDKResultSuccess;

    if (NULL == pUsecaseRequestObj)
    {
        CHX_LOG_ERROR("pUsecaseRequestObj is NULL");
        result = CDKResultEInvalidArg;
    }

    if (NULL == pGraphNode)
    {
        CHX_LOG_ERROR("pGraphNode is NULL");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        // Populate feature graph instance data for feature request object
        ChiFeature2GraphCallbackData* pChiFeature2GraphCallbackData = CHX_NEW ChiFeature2GraphCallbackData();
        if (NULL == pChiFeature2GraphCallbackData)
        {
            CHX_LOG_ERROR("No memory: pChiFeature2GraphCallbackData is NULL");
            result = CDKResultENoMemory;
        }
        else
        {
            pChiFeature2GraphCallbackData->pFeatureGraph                    = this;
            pChiFeature2GraphCallbackData->pGraphNode                       = pGraphNode;
            pChiFeature2GraphCallbackData->downstreamFeatureRequestObjMap   = pGraphNode->downstreamFeatureRequestObjMap;

            // Save the callback data for later deletion
            m_pFeatureGraphCallbackData.push_back(pChiFeature2GraphCallbackData);

            // Gather info for upstream feature's feature request object creation
            UINT8                   requestTableSize    = pGraphNode->requestTable.size();
            ChiFeature2RequestMap*  pRequestTable       =
                static_cast<ChiFeature2RequestMap*>(CHX_CALLOC(sizeof(ChiFeature2RequestMap) * requestTableSize));
            if (NULL == pRequestTable)
            {
                CHX_LOG_ERROR("No memory: pRequestTable is NULL");
                result = CDKResultENoMemory;
            }
            else
            {
                for (UINT8 requestIndex = 0; requestIndex < requestTableSize; ++requestIndex)
                {
                    pRequestTable[requestIndex].requestIndex        = requestIndex;
                    pRequestTable[requestIndex].numRequestOutputs   = pGraphNode->requestTable[requestIndex].size();
                    pRequestTable[requestIndex].pRequestOutputs     = &(pGraphNode->requestTable[requestIndex][0]);
                }

                ChiFeature2RequestObjectCreateInputInfo requestObjectCreateInputInfo = {0};
                requestObjectCreateInputInfo.pGraphPrivateData        = pChiFeature2GraphCallbackData;
                requestObjectCreateInputInfo.pUsecaseRequestObj       = pUsecaseRequestObj;
                requestObjectCreateInputInfo.pFeatureBase             = pGraphNode->pFeatureBaseObj;
                requestObjectCreateInputInfo.numRequests              = requestTableSize;
                requestObjectCreateInputInfo.pRequestTable            = pRequestTable;
                requestObjectCreateInputInfo.pFeatureHint             = &(pGraphNode->featureHint);
                requestObjectCreateInputInfo.usecaseMode              = pUsecaseRequestObj->GetUsecaseMode();
                requestObjectCreateInputInfo.pGraphName               = GetFeatureGraphName();

                // Create the feature request object
                ChiFeature2RequestObject* pSrcFeatureRequestObj =
                    ChiFeature2RequestObject::Create(&requestObjectCreateInputInfo);

                CHX_FREE(pRequestTable);
                pRequestTable = NULL;

                pGraphNode->requestTable.clear();
                pGraphNode->downstreamFeatureRequestObjMap.clear();

                if (NULL == pSrcFeatureRequestObj)
                {
                    CHX_LOG_ERROR("Could not create feature request object");
                    result = CDKResultEFailed;
                }
                else
                {
                    // Store the feature request object in the feature graph node
                    pGraphNode->pFeatureRequestObjList.push_back(pSrcFeatureRequestObj);

                    CHX_LOG_INFO("%s_Feature:%s: Submitting Feature Request Object with numRequests=%u",
                                 IdentifierString(),
                                 pGraphNode->pFeatureBaseObj->GetFeatureName(),
                                 requestObjectCreateInputInfo.numRequests);

                    // Give the feature request object to the feature and continue the walk-back
                    result = pGraphNode->pFeatureBaseObj->ProcessRequest(pSrcFeatureRequestObj);
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("pSrcFeatureBase->ProcessRequest failed with result %d", result);
                    }
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::GetInputSettingsForInputBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIMETAHANDLE ChiFeature2Graph::GetInputSettingsForInputBuffer(
    ChiFeature2GlobalPortInstanceId*    pPortInstanceId,
    ChiFeature2RequestObject*           pFeatureRequestObj,
    UINT8                               batchIndex,
    UINT8                               dependencyIndex)
{
    CHIMETAHANDLE hMetadata = NULL;

    if (NULL == pPortInstanceId)
    {
        CHX_LOG_ERROR("Port id is invalid");
    }

    if (NULL == pFeatureRequestObj)
    {
        CHX_LOG_ERROR("Feature request object is invalid");
    }

    if (NULL != pPortInstanceId && NULL != pFeatureRequestObj)
    {
        ChiFeature2Identifier* pPortId = &pPortInstanceId->portId;

        if (NULL != pPortId)
        {
            // Input settings will always be on non metadata port
            if (pPortId->portType == ChiFeature2PortType::MetaData)
            {
                pFeatureRequestObj->GetRequestInputInfo(
                        ChiFeature2SequenceOrder::Current,
                        pPortId,
                        &hMetadata,
                        dependencyIndex,
                        batchIndex);
            }
        }
    }

    return hMetadata;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessChiMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessChiMessage(
    ChiFeature2RequestObject* pFeatureRequestObj,
    ChiFeature2Messages*      pMessages)
{
    CDKResult                           result                      = CDKResultSuccess;
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObject       = NULL;
    VOID*                               pGraphPrivateData           = NULL;
    ChiFeature2GraphCallbackData*       pFeatureGraphCallbackData   = NULL;
    ChiFeature2Graph*                   pFeatureGraph               = NULL;
    CHICAPTUREREQUEST*                  pChiRequest                 = NULL;
    UINT64                              frameNumber                 = 0;
    CHIMESSAGEDESCRIPTOR                chiMessage                  = *pMessages->chiNotification.pChiMessages;
    ChiFeature2Messages                 graphMessage;

    // Set the graph message we are going to propagate to the CHI message
    graphMessage.chiNotification.pChiMessages   = &chiMessage;
    graphMessage.chiNotification.requestIndex   = pMessages->chiNotification.requestIndex;
    graphMessage.chiNotification.pIdentifier    = pMessages->chiNotification.pIdentifier;

    // Handling device/recovery error and SOF/MetaBuffer notification separately because they are not specific to any
    // FRO therefore FRO will be NULL. Handle all other error/chi notifications within the switch case below
    if (FALSE == DoesNotificationTypeHaveFRO(pMessages))
    {
        pMessages->chiNotification.featureGraphManagerCallbacks.ChiFeatureGraphProcessMessage(pMessages,
            pMessages->chiNotification.featureGraphManagerCallbacks.pPrivateCallbackData);
    }
    else
    {
        if (NULL != pFeatureRequestObj)
        {
            // Get the frameNumber associated with the notification, then handle the notification
            pFeatureRequestObj->GetGraphPrivateData(&pGraphPrivateData);
            pUsecaseRequestObject = pFeatureRequestObj->GetUsecaseRequestObject();
            if ((NULL != pGraphPrivateData) && (NULL != pUsecaseRequestObject))
            {
                pFeatureGraphCallbackData   = reinterpret_cast<ChiFeature2GraphCallbackData*>(pGraphPrivateData);
                pFeatureGraph               = pFeatureGraphCallbackData->pFeatureGraph;
                pChiRequest                 = pUsecaseRequestObject->GetChiCaptureRequest();

                if ((NULL != pFeatureGraph) && (NULL != pChiRequest))
                {
                    frameNumber = pChiRequest->frameNumber;
                }
                else
                {
                    CHX_LOG_ERROR("CHI request:%p or Feature graph:%p is NULL, cannot propagate CHI message type:%d",
                                  pChiRequest,
                                  pFeatureGraph,
                                  pMessages->chiNotification.pChiMessages->messageType);
                    result = CDKResultEFailed;
                }
            }
            else
            {
                CHX_LOG_ERROR("Graph data:%p or URO data:%p is NULL, cannot propagate CHI message type:%d",
                              pGraphPrivateData,
                              pUsecaseRequestObject,
                              pMessages->chiNotification.pChiMessages->messageType);
                result = CDKResultEFailed;
            }

            if (CDKResultSuccess == result)
            {
                switch (pMessages->chiNotification.pChiMessages->messageType)
                {
                    case ChiMessageTypeError:
                        result = pFeatureGraph->ProcessErrorMessage(pFeatureGraphCallbackData->pGraphNode,
                                                                    pMessages,
                                                                    pFeatureGraphCallbackData->downstreamFeatureRequestObjMap,
                                                                    frameNumber);
                        break;
                    case ChiMessageTypeShutter:
                        // Check to see if this frame has already had a shutter message sent by checking against the URO
                        if (0 == pUsecaseRequestObject->GetShutterTimestamp())
                        {
                            chiMessage.message.shutterMessage.frameworkFrameNum = frameNumber;
                            pUsecaseRequestObject->SetShutterTimestamp(pMessages);
                            pFeatureGraph->m_featureGraphManagerCallbacks.ChiFeatureGraphProcessMessage(&graphMessage,
                                pFeatureGraph->m_featureGraphManagerCallbacks.pPrivateCallbackData);
                        }
                        break;
                    default:
                        CHX_LOG_WARN("Unhandled CHI message type:%d", pMessages->chiNotification.pChiMessages->messageType);
                        break;
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("FRO is NULL for error message type:%d, cannot propagate notification",
                          pMessages->chiNotification.pChiMessages->messageType);
            result = CDKResultEFailed;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessFeatureMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessFeatureMessage(
    ChiFeature2RequestObject* pFeatureRequestObj,
    ChiFeature2Messages*      pMessages)
{
    CDKResult   result              = CDKResultSuccess;
    VOID*       pGraphPrivateData   = NULL;

    pFeatureRequestObj->GetGraphPrivateData(&pGraphPrivateData);

    ChiFeature2GraphCallbackData*               pFeatureGraphCallbackData       =
        reinterpret_cast<ChiFeature2GraphCallbackData*>(pGraphPrivateData);
    ChiFeature2Graph*                           pFeatureGraph                   =
        pFeatureGraphCallbackData->pFeatureGraph;
    ChiFeature2GraphNode*                       pFeatureGraphNode               =
        pFeatureGraphCallbackData->pGraphNode;
    std::vector<ChiFeature2RequestObjectMap>&   rDownstreamFeatureRequestObjMap =
        pFeatureGraphCallbackData->downstreamFeatureRequestObjMap;

    // Process ChiFeature2 message type
    switch (pMessages->pFeatureMessages->messageType)
    {
        case ChiFeature2MessageType::GetInputDependency:
            result = pFeatureGraph->ProcessGetInputDependencyMessage(pFeatureGraphNode,
                                                                     pMessages);
            break;
        case ChiFeature2MessageType::ReleaseInputDependency:
            result = pFeatureGraph->ProcessReleaseInputDependencyMessage(pFeatureGraphNode,
                                                                         pMessages);
            break;
        case ChiFeature2MessageType::ResultNotification:
        case ChiFeature2MessageType::MetadataNotification:
            result = pFeatureGraph->ProcessResultMetadataMessage(pFeatureGraphNode,
                                                                 rDownstreamFeatureRequestObjMap,
                                                                 pMessages);
            break;
        case ChiFeature2MessageType::PartialMetadataNotification:
            result = pFeatureGraph->ProcessPartialMetadataMessage(pFeatureGraphNode,
                                                                  pMessages);
            break;
        case ChiFeature2MessageType::ProcessingDependenciesComplete:
            result = pFeatureGraph->ProcessProcessingDependenciesCompleteMessage(pFeatureGraphNode);
            break;
        case ChiFeature2MessageType::MessageNotification:
            result = pFeatureGraph->m_featureGraphManagerCallbacks.ChiFeatureGraphProcessMessage(
                pMessages,
                pFeatureGraph->m_featureGraphManagerCallbacks.pPrivateCallbackData);
            break;
        case ChiFeature2MessageType::SubmitRequestNotification:
            result = pFeatureGraph->m_featureGraphManagerCallbacks.ChiFeatureGraphProcessMessage(
                pMessages,
                pFeatureGraph->m_featureGraphManagerCallbacks.pPrivateCallbackData);
            break;
        default:
            CHX_LOG_ERROR("Invalid message type: %d", pMessages->pFeatureMessages->messageType);
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessErrorMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessErrorMessage(
    ChiFeature2GraphNode*                       pGraphNode,
    ChiFeature2Messages*                        pMessages,
    std::vector<ChiFeature2RequestObjectMap>&   rDownstreamFeatureRequestObjMap,
    UINT64                                      frameNumber)
{
    CDKResult result = CDKResultSuccess;

    switch (pMessages->chiNotification.pChiMessages->message.errorMessage.errorMessageCode)
    {
        case MessageCodeResult:
        case MessageCodeBuffer:
            PropagateErrorToDownstreamNode(pGraphNode, pMessages, rDownstreamFeatureRequestObjMap, frameNumber);
            break;
        default:
            CHX_LOG_WARN("Unhandled error type:%s", ChxUtils::ErrorMessageCodeToString(
                pMessages->chiNotification.pChiMessages->message.errorMessage.errorMessageCode));
            break;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::PropagateErrorToDownstreamNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::PropagateErrorToDownstreamNode(
    ChiFeature2GraphNode*                       pGraphNode,
    ChiFeature2Messages*                        pMessages,
    std::vector<ChiFeature2RequestObjectMap>&   rDownstreamFeatureRequestObjMap,
    UINT64                                      frameNumber)
{
    CDKResult                       result              = CDKResultSuccess;
    BOOL                            foundErrorLink      = FALSE;
    ChiFeature2GraphLinkData*       pOutputLinkData     = NULL;
    CHIMESSAGEDESCRIPTOR            chiMessage          = *pMessages->chiNotification.pChiMessages;
    ChiFeature2Messages             graphMessage;
    BOOL                            skipProcessRequest  = FALSE;

    // Set the graph message we are going to propagate
    graphMessage.chiNotification.pChiMessages   = &chiMessage;
    graphMessage.chiNotification.requestIndex   = pMessages->chiNotification.requestIndex;
    graphMessage.chiNotification.pIdentifier    = pMessages->chiNotification.pIdentifier;

    // Iterate over FGN's output ports find the internal link(s). With the internal link, we can grab the downstream node
    // that we want to mark the error in
    for (UINT32 outputPortIdIndex = 0; outputPortIdIndex < pGraphNode->portToOutputLinkIndexMap.size(); ++outputPortIdIndex)
    {
        // Create graph-unique output port instance id
        ChiFeature2GlobalPortInstanceId outputPortInstanceId = CreateGlobalPortInstanceId(pGraphNode->featureId,
            pGraphNode->instanceProps,
            *graphMessage.chiNotification.pIdentifier);

        pOutputLinkData = MapOutputPortIdToOutputLinkData(&outputPortInstanceId, pGraphNode);

        if (NULL != pOutputLinkData)
        {
            if (pOutputLinkData->linkType == ChiFeature2GraphLinkType::Internal)
            {
                if (pOutputLinkData->linkDesc.internalLinkDesc.srcPortId.portId == *pMessages->chiNotification.pIdentifier)
                {
                    CHX_LOG_VERBOSE("%s, error:%s Found Internal link at index:%d  FID:[%d %d %d]"
                                    " to grab downstream node linkdata:%p",
                                    pGraphNode->pFeatureRequestObjList.back()->IdentifierString(),
                                    ChxUtils::ErrorMessageCodeToString(
                                        pMessages->chiNotification.pChiMessages->message.errorMessage.errorMessageCode),
                                    outputPortIdIndex,
                                    pMessages->chiNotification.pIdentifier->session,
                                    pMessages->chiNotification.pIdentifier->pipeline,
                                    pMessages->chiNotification.pIdentifier->port,
                                    pOutputLinkData);
                    foundErrorLink = TRUE;
                    break;
                }
            }
            else if (pOutputLinkData->linkType == ChiFeature2GraphLinkType::ExternalSink)
            {
                if (outputPortInstanceId == pOutputLinkData->linkDesc.extSinkLinkDesc.portId)
                {
                    CHX_LOG_VERBOSE("%s error:%s Found External sink link at index:%d FID:[%d %d %d] linkData:%p featureID:%d",
                                    pGraphNode->pFeatureRequestObjList.back()->IdentifierString(),
                                    ChxUtils::ErrorMessageCodeToString(
                                        pMessages->chiNotification.pChiMessages->message.errorMessage.errorMessageCode),
                                    outputPortIdIndex,
                                    pMessages->chiNotification.pIdentifier->session,
                                    pMessages->chiNotification.pIdentifier->pipeline,
                                    pMessages->chiNotification.pIdentifier->port,
                                    pOutputLinkData,
                                    pOutputLinkData->linkDesc.extSinkLinkDesc.portId.featureId);
                    foundErrorLink = TRUE;
                    break;
                }
            }
        }
    }

    if (foundErrorLink == TRUE)
    {
        if (pOutputLinkData->linkType == ChiFeature2GraphLinkType::Internal)
        {
            ChiFeature2GlobalPortInstanceId sinkPortId                   =
                pOutputLinkData->linkDesc.internalLinkDesc.sinkPortId;
            ChiFeature2GlobalPortInstanceId srcPortId                    =
                pOutputLinkData->linkDesc.internalLinkDesc.srcPortId;
            ChiFeature2RequestObject*       pUpstreamFeatureRequestObj =
                pOutputLinkData->pSrcGraphNode->pFeatureRequestObjList.back();
            UINT8                           batchIndex                   =
                pOutputLinkData->linkRequestTable[0].batchIndex;
            UINT8                           dependencyIndex              =
                pOutputLinkData->linkRequestTable[0].dependencyIndex;

            // Find the downstream FRO that corresponds to the downstream feature
            ChiFeature2RequestObject* pDownstreamFeatureRequestObj = NULL;
            for (UINT featureRequestObjIndex = 0;
                 featureRequestObjIndex < rDownstreamFeatureRequestObjMap.size();
                 ++featureRequestObjIndex)
            {
                if (rDownstreamFeatureRequestObjMap[featureRequestObjIndex].pGraphNode == pOutputLinkData->pSinkGraphNode)
                {
                    pDownstreamFeatureRequestObj = rDownstreamFeatureRequestObjMap[featureRequestObjIndex].pFeatureRequestObj;
                }
            }

            if (NULL == pDownstreamFeatureRequestObj)
            {
                CHX_LOG_ERROR("No matching downstream graph node found, cannot propagate error!");
            }
            else
            {
                CHX_LOG_INFO("%s Error being propagated from %s[%d %d %d]->%s[%d %d %d]",
                             IdentifierString(),
                             pUpstreamFeatureRequestObj->IdentifierString(),
                             srcPortId.portId.session,
                             srcPortId.portId.pipeline,
                             srcPortId.portId.port,
                             pDownstreamFeatureRequestObj->IdentifierString(),
                             sinkPortId.portId.session,
                             sinkPortId.portId.pipeline,
                             sinkPortId.portId.port);

                result = pDownstreamFeatureRequestObj->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                            &sinkPortId.portId, NULL, 0, TRUE, batchIndex, dependencyIndex);

                if (CDKResultSuccess == result)
                {
                    ChiFeature2Base* pFeatureBaseObj = pOutputLinkData->pSinkGraphNode->pFeatureBaseObj;
                    result                           = pFeatureBaseObj->ProcessRequest(pDownstreamFeatureRequestObj);
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_VERBOSE("%s: Current feature not able to proceed, downstream feature %s now propagating error",
                                        IdentifierString(),
                                        pDownstreamFeatureRequestObj->IdentifierString());
                    }
                }
                else
                {
                    CHX_LOG_ERROR("%s: Unable to set error at %s",
                        IdentifierString(), pDownstreamFeatureRequestObj->IdentifierString());
                    result = CDKResultEFailed;
                }
            }
        }
        else if (pOutputLinkData->linkType == ChiFeature2GraphLinkType::ExternalSink)
        {
            UINT32                      extPortIdIndex              = 0;
            BOOL                        matchFound                  = FALSE;
            ChiFeature2RequestObject*   pUpstreamFeatureRequestObj  =
                pOutputLinkData->pSrcGraphNode->pFeatureRequestObjList.back();

            for (UINT32 externalPortIdIndex = 0; externalPortIdIndex < m_extSinkPortIdToChiStreamMap.size();
                 ++externalPortIdIndex)
            {
                if (pOutputLinkData->linkDesc.extSinkLinkDesc.portId ==
                    m_extSinkPortIdToChiStreamMap[externalPortIdIndex].portId)
                {
                    extPortIdIndex  = externalPortIdIndex;
                    matchFound      = TRUE;
                }
            }

            if (ChiFeature2PortType::ImageBuffer == pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.portType)
            {
                // Get the stream buffer and metadata info from feature request object
                ChiFeature2BufferMetadataInfo* pBufferMetadataInfo = NULL;

                result = pUpstreamFeatureRequestObj->GetFinalBufferMetadataInfo(
                    pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId,
                    &pBufferMetadataInfo,
                    pMessages->chiNotification.requestIndex);

                if ((CDKResultSuccess == result) && (NULL != pBufferMetadataInfo))
                {
                    CHICAPTUREREQUEST*  pChiRequest     = m_pUsecaseRequestObj->GetChiCaptureRequest();
                    CHICAPTURERESULT    chiResult       = { 0 };
                    CHISTREAMBUFFER*    pTargetBuffer   = NULL;
                    chiResult.frameworkFrameNum         = pChiRequest->frameNumber;
                    result                              = GetStreamBuffer(pBufferMetadataInfo->hBuffer,
                                                                          pBufferMetadataInfo->key,
                                                                          &pTargetBuffer);
                    if (CDKResultSuccess == result)
                    {
                        // Find the ChiStream associated with the sink link we are processing
                        for (UINT32 portIdIndex = 0; portIdIndex < m_extSinkPortIdToChiStreamMap.size(); ++portIdIndex)
                        {
                            if (pOutputLinkData->linkDesc.extSinkLinkDesc.portId ==
                                m_extSinkPortIdToChiStreamMap[portIdIndex].portId)
                            {
                                CHX_LOG_INFO("%s Found matching port:[%d %d %d] for buffer error"
                                             "using error stream %p format:%d",
                                             IdentifierString(),
                                             pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.session,
                                             pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.pipeline,
                                             pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.port,
                                             m_extSinkPortIdToChiStreamMap[portIdIndex].pChiStream,
                                             m_extSinkPortIdToChiStreamMap[portIdIndex].pChiStream->format);

                                skipProcessRequest = pBufferMetadataInfo->bufferSkipped;

                                // Use the matching stream to populate the capture result information and the callback data
                                pTargetBuffer->pStream          = m_extSinkPortIdToChiStreamMap[portIdIndex].pChiStream;
                                pTargetBuffer->bufferStatus     = 1;
                                chiResult.numOutputBuffers      = 1;
                                chiResult.pOutputBuffers        = pTargetBuffer;
                                chiResult.frameworkFrameNum     = pChiRequest->frameNumber;
                                chiResult.numPartialMetadata    = 0;
                                chiResult.pOutputMetadata       = NULL;

                                chiMessage.message.errorMessage.pErrorStream        =
                                    m_extSinkPortIdToChiStreamMap[portIdIndex].pChiStream;
                                chiMessage.message.errorMessage.errorMessageCode    = MessageCodeBuffer;
                                chiMessage.message.errorMessage.frameworkFrameNum   = pChiRequest->frameNumber;

                                m_featureGraphManagerCallbacks.ChiFeatureGraphProcessMessage(
                                    &graphMessage,
                                    m_featureGraphManagerCallbacks.pPrivateCallbackData);

                                m_featureGraphManagerCallbacks.ChiFeatureGraphProcessResult(
                                    &chiResult,
                                    m_pUsecaseRequestObj,
                                    m_featureGraphManagerCallbacks.pPrivateCallbackData);

                                break;
                            }
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("%s_Feature:%s_%d Issue getting final buffer meta info when trying to propagate buffer error",
                                  IdentifierString(),
                                  pGraphNode->pFeatureBaseObj->GetFeatureName(),
                                  pGraphNode->pFeatureBaseObj->GetInstanceProps()->instanceId);
                    result = CDKResultEFailed;
                }
            }
            else
            {
                CHX_LOG_INFO("%s ResultError for port info:[%d %d %d] being sent to Graph manager",
                    IdentifierString(),
                    pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.session,
                    pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.pipeline,
                    pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.port);

                chiMessage.message.errorMessage.errorMessageCode    = MessageCodeResult;
                chiMessage.message.errorMessage.frameworkFrameNum   = frameNumber;
                m_featureGraphManagerCallbacks.ChiFeatureGraphProcessMessage(
                    &graphMessage,
                    m_featureGraphManagerCallbacks.pPrivateCallbackData);
            }

            if (TRUE == matchFound)
            {
                // Do all the releasing here
                // Find the output port and set the flag indicating we've notified the client with the result
                CHX_LOG_VERBOSE("%s Marking external output port as notified for port info:[%d %d %d %d]",
                                IdentifierString(),
                                pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.session,
                                pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.pipeline,
                                pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.port,
                                pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.portType);
                m_extSinkPortIdToChiStreamMap[extPortIdIndex].portFlags.outputResultNotified = TRUE;
            }

            result = pUpstreamFeatureRequestObj->SetOutputNotifiedForPort(
                *pMessages->chiNotification.pIdentifier,
                pMessages->chiNotification.requestIndex);

            if (CDKResultSuccess == result)
            {
                ChiFeature2Base* pFeatureBaseObj = pOutputLinkData->pSrcGraphNode->pFeatureBaseObj;

                if (NULL != pFeatureBaseObj)
                {
                    // Check if we marked the buffer as a skip buffer. A buffer marked as skip means that we dropped it from
                    // the request before submitting it to camx. In this case, we want the rest of the request to continue on
                    // its own rather than calling ProcessRequest. Calling ProcessRequest during error case is meant only in
                    // situations where we get an error on the result path not the submission path and is used to help clean
                    // up and complete the request
                    if (FALSE == skipProcessRequest)
                    {
                        result = pFeatureBaseObj->ProcessRequest(pUpstreamFeatureRequestObj);
                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("pFeatureBaseObj->ProcessRequest() failed with result %d", result);
                            result = CDKResultEFailed;
                        }
                        if (CDKResultSuccess == result)
                        {
                            CHX_LOG_INFO("%s setting processing complete for port:[%d %d %d %d]",
                                                pUpstreamFeatureRequestObj->IdentifierString(),
                                                pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.session,
                                                pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.pipeline,
                                                pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.port,
                                                pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.portType);

                            pUpstreamFeatureRequestObj->SetProcessingCompleteForPort(
                                       pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId);

                            result = pFeatureBaseObj->ProcessRequest(pUpstreamFeatureRequestObj);
                            if (CDKResultSuccess != result)
                            {
                                CHX_LOG_ERROR("pFeatureBaseObj->ProcessRequest() failed with result %d", result);
                            }
                        }
                    }
                    else
                    {
                        CHX_LOG_VERBOSE("%s Skip Process request! Encountered buffer marked as skip",
                                        pUpstreamFeatureRequestObj->IdentifierString());
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Feature base is NULL!");
                    result = CDKResultEInvalidPointer;
                }

                if (CDKResultSuccess == result)
                {
                    // Check all FROs in all graph nodes to determine whether we can move the URO to the complete state.
                    // This is done after ProcessRequest() from a sink link in order to allow all upstream features a chance
                    // to report ProcessingComplete.
                    BOOL allFROsProcessingComplete = AreAllFROsProcessingComplete();

                    if (TRUE == allFROsProcessingComplete)
                    {
                        CHX_LOG_INFO("%s All FROs reported ProcessingComplete; moving URO to Complete",
                                     IdentifierString());
                        result = m_pUsecaseRequestObj->SetRequestState(ChiFeature2UsecaseRequestObjectState::Complete);
                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("m_pUsecaseRequestObj->SetRequestState() failed with result %d", result);
                            result = CDKResultEFailed;
                        }
                    }
                }
            }
            else
            {
                CHX_LOG_ERROR("%s Failed in marking Notified :[%d %d %d]",
                              IdentifierString(),
                              pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.session,
                              pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.pipeline,
                              pOutputLinkData->linkDesc.extSinkLinkDesc.portId.portId.port);
                result = CDKResultEFailed;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("%s Did not find link to propagate error:%s",
                      pGraphNode->pFeatureRequestObjList.back()->IdentifierString(),
                      ChxUtils::ErrorMessageCodeToString(
                          pMessages->chiNotification.pChiMessages->message.errorMessage.errorMessageCode));
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessGetInputDependencyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessGetInputDependencyMessage(
    ChiFeature2GraphNode*   pGraphNode,
    ChiFeature2Messages*    pMessages)
{
    CDKResult result = CDKResultSuccess;

    // Create a set of global port instance ids in order to efficiently determine any duplicates
    std::set<ChiFeature2GlobalPortInstanceId, globalPortInstanceIdLessComparator> globalPortInstanceIds;

    // Iterate through all batches, all dependencies, and all input ports in message to create two-dimensional dependency table
    // for upstream FRO creation.
    UINT8                       numBatches  =
        pMessages->pFeatureMessages->message.getDependencyData.numBatches;
    ChiFeature2DependencyBatch* pBatches    =
        pMessages->pFeatureMessages->message.getDependencyData.pBatches;

    CHX_LOG_INFO("%s_%d Feature:%s: ChiFeature2MessageType::GetInputDependency for %u batches",
                 pGraphNode->pFeatureBaseObj->GetFeatureName(),
                 pGraphNode->pFeatureBaseObj->GetInstanceProps()->instanceId,
                 IdentifierString(), numBatches);

    for (UINT8 batchIndex = 0; batchIndex < numBatches; ++batchIndex)
    {
        UINT8 numDependencies                   =
            pMessages->pFeatureMessages->message.getDependencyData.pBatches[batchIndex].numDependencies;
        ChiFeature2Dependency* pDependencies    =
            pMessages->pFeatureMessages->message.getDependencyData.pBatches[batchIndex].pDependencies;

        for (UINT8 depIndex = 0; depIndex < numDependencies; ++depIndex)
        {
            UINT8                           numInputPorts   =
                pMessages->pFeatureMessages->message.getDependencyData.pBatches[batchIndex].pDependencies[depIndex].numPorts;
            const ChiFeature2Identifier*    pInputPorts          =
                pMessages->pFeatureMessages->message.getDependencyData.pBatches[batchIndex].pDependencies[depIndex].pPorts;

            // Iterate through all input ports in the message
            for (UINT inputPortIndex = 0; inputPortIndex < numInputPorts; inputPortIndex++)
            {
                UINT8 requestIndex = (numDependencies * batchIndex) + depIndex;

                // Create graph-unique input port instance id
                ChiFeature2GlobalPortInstanceId inputPortInstanceId =
                    CreateGlobalPortInstanceId(pGraphNode->featureId,
                                               pGraphNode->instanceProps,
                                               pInputPorts[inputPortIndex]);

                // Add port instance id to set to come up with all unique ports in the GetInputDependency message
                globalPortInstanceIds.insert(inputPortInstanceId);

                // Map input port id to input link data
                ChiFeature2GraphLinkData* pInputLinkData =
                    MapInputPortIdToInputLinkData(&inputPortInstanceId, pGraphNode);
                if (NULL == pInputLinkData)
                {
                    CHX_LOG_ERROR("Cannot find input link corresponding to port id {%u, {%u, %u}, {%u, %u, %u, %d, %d}}",
                                  inputPortInstanceId.featureId,
                                  inputPortInstanceId.instanceProps.instanceId, inputPortInstanceId.instanceProps.cameraId,
                                  inputPortInstanceId.portId.session, inputPortInstanceId.portId.pipeline,
                                  inputPortInstanceId.portId.port, inputPortInstanceId.portId.portDirectionType,
                                  inputPortInstanceId.portId.portType);
                }
                else
                {
                    // Store the request mapping in the link's table
                    ChiFeature2GraphLinkRequestMap linkRequestMap   = {0};
                    linkRequestMap.requestIndex                     = requestIndex;
                    linkRequestMap.batchIndex                       = batchIndex;
                    linkRequestMap.dependencyIndex                  = depIndex;
                    linkRequestMap.inputDependencyReleased          = FALSE;
                    pInputLinkData->linkRequestTable.push_back(linkRequestMap);

                    // Update link data with book-keeping info
                    pInputLinkData->numBatches = numBatches;

                    while (batchIndex >= pInputLinkData->numDependenciesPerBatchList.size())
                    {
                        UINT8 numDependenciesPerBatch = 0;
                        pInputLinkData->numDependenciesPerBatchList.push_back(numDependenciesPerBatch);
                    }
                    pInputLinkData->numDependenciesPerBatchList[batchIndex] = numDependencies;

                    // Check to see if this is an external source link, since we need to update the usecase request object
                    // rather than pass data to upstream feature
                    if (ChiFeature2GraphLinkType::ExternalSource == pInputLinkData->linkType)
                    {
                        if (pInputPorts[inputPortIndex].portType == ChiFeature2PortType::ImageBuffer)
                        {
                            CHIMETAHANDLE hSettings = NULL;
                            for (UINT metaPortIndex = 0; metaPortIndex < numInputPorts; metaPortIndex++)
                            {
                                if ((pInputPorts[metaPortIndex].portType  == ChiFeature2PortType::MetaData) &&
                                    (pInputPorts[metaPortIndex].session   == pInputPorts[inputPortIndex].session) &&
                                    (pInputPorts[metaPortIndex].pipeline  == pInputPorts[inputPortIndex].pipeline))
                                {
                                    ChiFeature2GlobalPortInstanceId metadataPortInstanceId =
                                        CreateGlobalPortInstanceId(pGraphNode->featureId,
                                                   pGraphNode->instanceProps,
                                                   pInputPorts[metaPortIndex]);
                                    hSettings = GetInputSettingsForInputBuffer(&metadataPortInstanceId,
                                        pInputLinkData->pSinkGraphNode->pFeatureRequestObjList.back(), batchIndex, depIndex);
                                    break;
                                }
                            }

                            // We know we want input on this port, so collect input stream data for the usecase request object
                            result = ProcessUsecaseRequestObjectInputStreamData(pInputLinkData, inputPortInstanceId, hSettings);
                        }
                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("ProcessUsecaseRequestObjectInputStreamData() failed with result = %d", result);
                        }
                    }
                    else
                    {
                        // Store this input port's input dependency info in upstream feature's request table
                        result = ProcessInputDepsForUpstreamFeatureRequest(m_pUsecaseRequestObj, pInputLinkData, requestIndex);
                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("ProcessInputDepsForUpstreamFeatureRequest() failed with result %d", result);
                        }
                    }
                }
            }
        }
    }

    if (CDKResultSuccess == result)
    {
        // Iterate over all input ports. If it appeared in the list above, move it to OutputPending, otherwise, move it to
        // Disabled. Walk back on each source node, taking the appropriate action based on the state of all output ports.
        for (UINT32 portIdIndex = 0; portIdIndex < pGraphNode->portToInputLinkIndexMap.size(); ++portIdIndex)
        {
            ChiFeature2GlobalPortInstanceId* pInputPortId = &(pGraphNode->portToInputLinkIndexMap[portIdIndex].portId);

            // Map input port id to input link data
            ChiFeature2GraphLinkData* pInputLinkData = MapInputPortIdToInputLinkData(pInputPortId, pGraphNode);
            if (NULL == pInputLinkData)
            {
                CHX_LOG_ERROR("Cannot find input link corresponding to port id {%u, {%u, %u}, {%u, %u, %u, %d}}",
                              pInputPortId->featureId,
                              pInputPortId->instanceProps.instanceId, pInputPortId->instanceProps.cameraId,
                              pInputPortId->portId.session, pInputPortId->portId.pipeline,
                              pInputPortId->portId.port, pInputPortId->portId.portType);
            }
            else
            {
                BOOL foundMatchingPortId = FALSE;

                // NOWHINE CF010: Whiner update required for stl container usage
                for (std::set<ChiFeature2GlobalPortInstanceId, globalPortInstanceIdLessComparator>::iterator portItr =
                         globalPortInstanceIds.begin();
                     portItr != globalPortInstanceIds.end();
                     ++portItr)
                {
                    ChiFeature2GlobalPortInstanceId inputDependencyPortId = *portItr;

                    if ((*pInputPortId) == inputDependencyPortId)
                    {
                        foundMatchingPortId = TRUE;
                        break;
                    }
                }

                if (TRUE == foundMatchingPortId)
                {
                    // We have input dependencies for this link, so move it to OutputPending
                    pInputLinkData->linkState = ChiFeature2GraphLinkState::OutputPending;
                }
                else
                {
                    // We do not have input dependencies for this link, so move it to Disabled
                    pInputLinkData->linkState = ChiFeature2GraphLinkState::Disabled;
                }

                // Walk back on the input link, taking the appropriate action based on the state of all output ports
                result = WalkBackFromLink(m_pUsecaseRequestObj, pInputLinkData);
                if (CDKResultSuccess != result)
                {
                    CHX_LOG_ERROR("WalkBackFromLink() failed with result = %d", result);
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessReleaseInputDependencyMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessReleaseInputDependencyMessage(
    ChiFeature2GraphNode*   pGraphNode,
    ChiFeature2Messages*    pMessages)
{
    CDKResult result = CDKResultSuccess;

    // We release input dependencies a single batch at a time
    UINT8 batchIndex = pMessages->pFeatureMessages->message.releaseDependencyData.batchIndex;

    // Iterate through all dependencies and all input ports in message to mark the input resources for release
    UINT8 numDependencies                   =
        pMessages->pFeatureMessages->message.releaseDependencyData.numDependencies;
    ChiFeature2Dependency* pDependencies    =
        pMessages->pFeatureMessages->message.releaseDependencyData.pDependencies;

    CHX_LOG_INFO("%s_Feature:%s_%d: ChiFeature2MessageType::ReleaseInputDependency for batchIndex=%u,"
                 "numDependencies=%u",
                 IdentifierString(),
                 pGraphNode->pFeatureBaseObj->GetFeatureName(),
                 pGraphNode->pFeatureBaseObj->GetInstanceProps()->instanceId,
                 batchIndex,
                 numDependencies);

    for (UINT8 dependencyIndex = 0; dependencyIndex < numDependencies; ++dependencyIndex)
    {
        UINT8                         listIndex     =
            pMessages->pFeatureMessages->message.releaseDependencyData.pDependencies[dependencyIndex].dependencyIndex;
        UINT                          numInputPorts =
            pMessages->pFeatureMessages->message.releaseDependencyData.pDependencies[dependencyIndex].numPorts;
        const ChiFeature2Identifier*  pInputPorts   =
            pMessages->pFeatureMessages->message.releaseDependencyData.pDependencies[dependencyIndex].pPorts;

        // Iterate through all input ports in the message
        for (UINT inputPortIndex = 0; inputPortIndex < numInputPorts; inputPortIndex++)
        {
            // Create graph-unique input port instance id
            ChiFeature2GlobalPortInstanceId inputPortInstanceId = CreateGlobalPortInstanceId(pGraphNode->featureId,
                                                                                             pGraphNode->instanceProps,
                                                                                             pInputPorts[inputPortIndex]);

            // Map input port id to input link data
            ChiFeature2GraphLinkData* pInputLinkData = MapInputPortIdToInputLinkData(&inputPortInstanceId, pGraphNode);
            if (NULL == pInputLinkData)
            {
                CHX_LOG_ERROR("Cannot find input link corresponding to port id {%u, {%u, %u}, {%u, %u, %u, %d, %d}}",
                              inputPortInstanceId.featureId,
                              inputPortInstanceId.instanceProps.instanceId, inputPortInstanceId.instanceProps.cameraId,
                              inputPortInstanceId.portId.session, inputPortInstanceId.portId.pipeline,
                              inputPortInstanceId.portId.port, inputPortInstanceId.portId.portDirectionType,
                              inputPortInstanceId.portId.portType);
            }
            else
            {
                // Update appropriate entry in link request map, indicating which dependency has been released
                UINT8 numDependenciesPerBatch   = pInputLinkData->numDependenciesPerBatchList[batchIndex];
                UINT8 requestIndex              = (numDependenciesPerBatch * batchIndex) + listIndex;
                if (FALSE == pInputLinkData->linkRequestTable[requestIndex].inputDependencyReleased)
                {
                    pInputLinkData->linkRequestTable[requestIndex].inputDependencyReleased = TRUE;
                    pInputLinkData->numDependenciesReleased++;
                }

                // In the case of an external source link, call back immediately to feature graph manager rather than to the
                // upstream feature via its feature request object
                if (ChiFeature2GraphLinkType::ExternalSource == pInputLinkData->linkType)
                {
                    // Call back to the feature graph manager to release input dependencies
                    m_featureGraphManagerCallbacks.ChiFeatureGraphProcessMessage(
                        pMessages,
                        m_featureGraphManagerCallbacks.pPrivateCallbackData);
                }
                else
                {
                    // Get upstream feature info
                    ChiFeature2GraphNode*   pUpstreamFeatureGraphNode   = pInputLinkData->pSrcGraphNode;
                    ChiFeature2Base*        pUpstreamFeatureBase        = pInputLinkData->pSrcGraphNode->pFeatureBaseObj;

                    // Get the upstream output port id
                    ChiFeature2GlobalPortInstanceId* pUpstreamPortId = &pInputLinkData->linkDesc.internalLinkDesc.srcPortId;

                    // NRW: Need to add FRO index
                    // Get the upstream feature request object
                    ChiFeature2RequestObject* pUpstreamFeatureRequestObj =
                        pInputLinkData->pSrcGraphNode->pFeatureRequestObjList.back();

                    // Tell the upstream feature that the downstream feature is finished with the output resources.
                    result = pUpstreamFeatureRequestObj->SetOutputNotifiedForPort(pUpstreamPortId->portId, requestIndex);
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("pUpstreamFeatureRequestObj->SetOutputNotifiedForPort failed with result %d", result);
                        break;
                    }
                    else
                    {
                        // Update the upstream feature with latest status
                        result = pUpstreamFeatureBase->ProcessRequest(pUpstreamFeatureRequestObj);
                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("pUpstreamFeatureBase->ProcessRequest() failed with result = %d", result);
                        }
                    }
                }
            }
        }
    }

    // For every input link, check whether all dependencies across all batches have been released. Then move link to NotVisited
    // if all dependencies for all batches have been released
    for (UINT8 portIdIndex = 0; portIdIndex < pGraphNode->portToInputLinkIndexMap.size(); ++portIdIndex)
    {
        ChiFeature2GlobalPortInstanceId* pInputPortId = &(pGraphNode->portToInputLinkIndexMap[portIdIndex].portId);

        // Map input port id to input link data
        ChiFeature2GraphLinkData* pInputLinkData = MapInputPortIdToInputLinkData(pInputPortId, pGraphNode);
        if (NULL == pInputLinkData)
        {
            CHX_LOG_ERROR("Cannot find input link corresponding to port id {%u, {%u, %u}, {%u, %u, %u, %d, %d}}",
                          pInputPortId->featureId,
                          pInputPortId->instanceProps.instanceId, pInputPortId->instanceProps.cameraId,
                          pInputPortId->portId.session, pInputPortId->portId.pipeline,
                          pInputPortId->portId.port, pInputPortId->portId.portDirectionType,
                          pInputPortId->portId.portType);
        }
        else
        {
            // Move link state to NotVisited if we have relased all input dependencies for all batches and clear the link
            // request table to prevent stale entries for deferred batches
            if ((ChiFeature2GraphLinkState::OutputPending   == pInputLinkData->linkState) &&
                (pInputLinkData->numDependenciesReleased    == pInputLinkData->linkRequestTable.size()))
            {
                pInputLinkData->linkState                   = ChiFeature2GraphLinkState::NotVisited;
                pInputLinkData->numBatches                  = 0;
                pInputLinkData->numDependenciesReleased     = 0;
                pInputLinkData->numDependenciesPerBatchList.clear();
                pInputLinkData->linkRequestTable.clear();
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessResultMetadataMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessResultMetadataMessage(
    ChiFeature2GraphNode*                       pGraphNode,
    std::vector<ChiFeature2RequestObjectMap>&   rDownstreamFeatureRequestObjMap,
    ChiFeature2Messages*                        pMessages)
{
    CDKResult result = CDKResultSuccess;

    UINT8                         requestIndex      = pMessages->pFeatureMessages->message.result.resultIndex;
    UINT8                         numOutputPorts    = pMessages->pFeatureMessages->message.result.numPorts;
    const ChiFeature2Identifier*  pOutputPorts      = pMessages->pFeatureMessages->message.result.pPorts;

    if (numOutputPorts == 0)
    {
        // Check all FROs in all graph nodes to determine whether we can move the URO to the complete state.
        // This is done after ProcessRequest() from a sink link in order to allow all upstream features a chance
        // to report ProcessingComplete.
        BOOL allFROsProcessingComplete = AreAllFROsProcessingComplete();

        if (TRUE == allFROsProcessingComplete)
        {
            CHX_LOG_INFO("%s All FROs reported ProcessingComplete; moving URO to Complete", IdentifierString());
            result = m_pUsecaseRequestObj->SetRequestState(ChiFeature2UsecaseRequestObjectState::Complete);
        }
    }

    // Iterate through all output ports in the message
    for (UINT8 outputPortIndex = 0; outputPortIndex < numOutputPorts; outputPortIndex++)
    {
        // Create graph-unique output port instance id
        ChiFeature2GlobalPortInstanceId outputPortInstanceId = CreateGlobalPortInstanceId(pGraphNode->featureId,
                                                                                          pGraphNode->instanceProps,
                                                                                          pOutputPorts[outputPortIndex]);

        // Map the output port id to the output link data list
        ChiFeature2GraphLinkData* pOutputLinkData = MapOutputPortIdToOutputLinkData(&outputPortInstanceId, pGraphNode);

        if (NULL == pOutputLinkData)
        {
            CHX_LOG_ERROR("Cannot find output link corresponding to port id "
                          "{%u, {%u, %u}, {%u, %u, %u, %d, %d}}",
                          outputPortInstanceId.featureId,
                          outputPortInstanceId.instanceProps.instanceId, outputPortInstanceId.instanceProps.cameraId,
                          outputPortInstanceId.portId.session, outputPortInstanceId.portId.pipeline,
                          outputPortInstanceId.portId.port, outputPortInstanceId.portId.portDirectionType,
                          outputPortInstanceId.portId.portType);
        }
        else
        {
            ChiFeature2RequestObject* pFeatureRequestObj = pGraphNode->pFeatureRequestObjList.back();

            // Check if link is in the OutputPending state
            if (ChiFeature2GraphLinkState::OutputPending != pOutputLinkData->linkState)
            {
                CHX_LOG_WARN("pOutputLinkData->linkState in unexpected state: %d; expected OutputPending",
                              pOutputLinkData->linkState);
                result = CDKResultEInvalidState;
            }
            else
            {
                // Only send the feature message through to the wrapper if there is a buffer error or no downstream node
                if (ChiFeature2GraphLinkType::ExternalSink == pOutputLinkData->linkType)
                {
                    CHICAPTUREREQUEST*  pChiRequest = m_pUsecaseRequestObj->GetChiCaptureRequest();
                    CHICAPTURERESULT    chiResult   = {0};
                    chiResult.frameworkFrameNum     = pChiRequest->frameNumber;

                    // Get the stream buffer and metadata info from feature request object
                    ChiFeature2BufferMetadataInfo* pBufferMetadataInfo = NULL;
                    pFeatureRequestObj->GetFinalBufferMetadataInfo(outputPortInstanceId.portId,
                                                                   &pBufferMetadataInfo,
                                                                   requestIndex);
                    if ((NULL != pBufferMetadataInfo) &&
                        (pMessages->pFeatureMessages->messageType == ChiFeature2MessageType::ResultNotification))
                    {
                        CHX_LOG_INFO("%s_Feature:%s_%d: ExternalSink ChiFeature2MessageType::ResultNotification",
                                     IdentifierString(),
                                     pGraphNode->pFeatureBaseObj->GetFeatureName(),
                                     pGraphNode->pFeatureBaseObj->GetInstanceProps()->instanceId);

                        if (ChiFeature2PortType::ImageBuffer == outputPortInstanceId.portId.portType)
                        {
                            CHISTREAMBUFFER* pTargetBuffer = NULL;

                            result = GetStreamBuffer(pBufferMetadataInfo->hBuffer,
                                                     pBufferMetadataInfo->key,
                                                     &pTargetBuffer);
                            if (CDKResultSuccess == result)
                            {
                                // Overwrite the internal stream with target stream to the chiResult
                                for (UINT portIdIndex = 0;
                                     portIdIndex < m_extSinkPortIdToChiStreamMap.size();
                                     ++portIdIndex)
                                {
                                    if (m_extSinkPortIdToChiStreamMap[portIdIndex].portId == outputPortInstanceId)
                                    {
                                        if (NULL != pTargetBuffer)
                                        {
                                            CHX_LOG_VERBOSE("%s_Feature:%s_%d: Internal Stream %p, external stream %p",
                                                            IdentifierString(),
                                                            pGraphNode->pFeatureBaseObj->GetFeatureName(),
                                                            pGraphNode->pFeatureBaseObj->GetInstanceProps()->instanceId,
                                                            pTargetBuffer->pStream,
                                                            m_extSinkPortIdToChiStreamMap[portIdIndex].pChiStream);

                                            pTargetBuffer->pStream = m_extSinkPortIdToChiStreamMap[portIdIndex].pChiStream;
                                        }

                                        chiResult.numOutputBuffers  = 1;
                                        chiResult.pOutputBuffers    = pTargetBuffer;
                                    }
                                }
                            }
                        }
                        else
                        {
                            CHX_LOG_INFO("%s_Feature:%s_%d: ExternalSink ChiFeature2MessageType::MetadataNotification",
                                IdentifierString(),
                                pGraphNode->pFeatureBaseObj->GetFeatureName(),
                                pGraphNode->pFeatureBaseObj->GetInstanceProps()->instanceId);

                            result = GetMetadataBuffer(pBufferMetadataInfo->hBuffer,
                                                       pBufferMetadataInfo->key,
                                                       &chiResult.pOutputMetadata);

                            if (CDKResultSuccess == result)
                            {
                                chiResult.numOutputBuffers   = 0;
                                chiResult.numPartialMetadata = 1;
                            }
                        }
                    }

                    if (CDKResultSuccess == result)
                    {
                        // No downstream feature, so call feature graph manager's ProcessResult callback
                        m_featureGraphManagerCallbacks.ChiFeatureGraphProcessResult(
                            &chiResult,
                            m_pUsecaseRequestObj,
                            m_featureGraphManagerCallbacks.pPrivateCallbackData);

                        if (pMessages->pFeatureMessages->messageType == ChiFeature2MessageType::ResultNotification)
                        {
                            // Find the output port and set the flag indicating we've notified the client with the result
                            for (UINT32 portIdIndex = 0; portIdIndex < m_extSinkPortIdToChiStreamMap.size(); ++portIdIndex)
                            {
                                if (outputPortInstanceId == m_extSinkPortIdToChiStreamMap[portIdIndex].portId)
                                {
                                    CHX_LOG_INFO("%s marking external output port as notified at index:%d"
                                                 " for featureID:%d [%d %d %d]",
                                                 IdentifierString(),
                                                 portIdIndex,
                                                 m_extSinkPortIdToChiStreamMap[portIdIndex].portId.featureId,
                                                 m_extSinkPortIdToChiStreamMap[portIdIndex].portId.portId.session,
                                                 m_extSinkPortIdToChiStreamMap[portIdIndex].portId.portId.pipeline,
                                                 m_extSinkPortIdToChiStreamMap[portIdIndex].portId.portId.port);
                                    m_extSinkPortIdToChiStreamMap[portIdIndex].portFlags.outputResultNotified = TRUE;
                                }

                                pFeatureRequestObj->SetOutputNotifiedForPort(outputPortInstanceId.portId, requestIndex);

                                ChiFeature2Base* pFeatureBaseObj = pOutputLinkData->pSrcGraphNode->pFeatureBaseObj;
                                result = pFeatureBaseObj->ProcessRequest(pFeatureRequestObj);
                                if (CDKResultSuccess != result)
                                {
                                    CHX_LOG_ERROR("pFeatureBaseObj->ProcessRequest() failed with result %d", result);
                                    break;
                                }
                            }
                        }

                        CHX_LOG_VERBOSE("%s setting processing complete for port:[%d %d %d %d]",
                                        pFeatureRequestObj->IdentifierString(),
                                        outputPortInstanceId.portId.session,
                                        outputPortInstanceId.portId.pipeline,
                                        outputPortInstanceId.portId.port,
                                        outputPortInstanceId.portId.portType);

                        pFeatureRequestObj->SetProcessingCompleteForPort(outputPortInstanceId.portId);

                        ChiFeature2Base* pFeatureBaseObj = pOutputLinkData->pSrcGraphNode->pFeatureBaseObj;
                        result = pFeatureBaseObj->ProcessRequest(pFeatureRequestObj);
                        if (CDKResultSuccess != result)
                        {
                            CHX_LOG_ERROR("pFeatureBaseObj->ProcessRequest() failed with result %d", result);
                            break;
                        }

                        // Check all FROs in all graph nodes to determine whether we can move the URO to the complete state.
                        // This is done after ProcessRequest() from a sink link in order to allow all upstream features a chance
                        // to report ProcessingComplete.
                        BOOL allFROsProcessingComplete = AreAllFROsProcessingComplete();

                        if (TRUE == allFROsProcessingComplete)
                        {
                            CHX_LOG_INFO("%s All FROs reported ProcessingComplete; moving URO to Complete",
                                         pFeatureRequestObj->IdentifierString());
                            result = m_pUsecaseRequestObj->SetRequestState(ChiFeature2UsecaseRequestObjectState::Complete);
                        }
                    }
                }
                else
                {
                    // Send result to downstream feature. First, find the downstream FRO that corresponds to the downstream
                    // feature.
                    ChiFeature2RequestObject* pDownstreamFeatureRequestObj = NULL;
                    for (UINT featureRequestObjIndex = 0;
                         featureRequestObjIndex < rDownstreamFeatureRequestObjMap.size();
                         ++featureRequestObjIndex)
                    {
                        if (rDownstreamFeatureRequestObjMap[featureRequestObjIndex].pGraphNode ==
                            pOutputLinkData->pSinkGraphNode)
                        {
                            pDownstreamFeatureRequestObj =
                                rDownstreamFeatureRequestObjMap[featureRequestObjIndex].pFeatureRequestObj;
                        }
                    }

                    if (NULL == pDownstreamFeatureRequestObj)
                    {
                        CHX_LOG_ERROR("No matching downstream graph node found, cannot propagate result/metadata!");
                    }
                    else
                    {
                        ChiFeature2GlobalPortInstanceId sinkPortId  = pOutputLinkData->linkDesc.internalLinkDesc.sinkPortId;
                        ChiFeature2GlobalPortInstanceId srcPortId   = pOutputLinkData->linkDesc.internalLinkDesc.srcPortId;

                        if (pMessages->pFeatureMessages->messageType == ChiFeature2MessageType::ResultNotification)
                        {
                            if (ChiFeature2PortType::MetaData == outputPortInstanceId.portId.portType)
                            {
                                CHX_LOG_INFO("%s_Feature:%s_%d: Internal ChiFeature2MessageType::Metadata",
                                             IdentifierString(),
                                             pGraphNode->pFeatureBaseObj->GetFeatureName(),
                                             pGraphNode->pFeatureBaseObj->GetInstanceProps()->instanceId);
                            }
                            else
                            {
                                CHX_LOG_INFO("%s_Feature:%s_%d: Internal ChiFeature2MessageType::ResultNotification",
                                             IdentifierString(),
                                             pGraphNode->pFeatureBaseObj->GetFeatureName(),
                                             pGraphNode->pFeatureBaseObj->GetInstanceProps()->instanceId);
                            }

                            // Get upstream buffer and metadata info from upstream feature request object
                            ChiFeature2BufferMetadataInfo* pBufferMetadataInfo = NULL;
                            pFeatureRequestObj->GetFinalBufferMetadataInfo(outputPortInstanceId.portId,
                                                                           &pBufferMetadataInfo,
                                                                           requestIndex);

                            if (NULL != pBufferMetadataInfo)
                            {
                                UINT8 batchIndex        = pOutputLinkData->linkRequestTable[requestIndex].batchIndex;
                                UINT8 dependencyIndex   = pOutputLinkData->linkRequestTable[requestIndex].dependencyIndex;

                                pDownstreamFeatureRequestObj->SetBufferInfo(ChiFeature2RequestObjectOpsType::InputDependency,
                                                                            &sinkPortId.portId,
                                                                            pBufferMetadataInfo->hBuffer,
                                                                            pBufferMetadataInfo->key,
                                                                            FALSE,
                                                                            batchIndex,
                                                                            dependencyIndex);

                                ChiFeature2Base* pFeatureBaseObj = pOutputLinkData->pSinkGraphNode->pFeatureBaseObj;
                                result = pFeatureBaseObj->ProcessRequest(pDownstreamFeatureRequestObj);
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessProcessingDependenciesCompleteMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessProcessingDependenciesCompleteMessage(
    ChiFeature2GraphNode* pGraphNode)
{
    CDKResult result = CDKResultSuccess;

    // For each non-source input link, set the upstream feature's FROs to processing complete and walk back
    for (UINT32 inputPortIdIndex = 0; inputPortIdIndex < pGraphNode->portToInputLinkIndexMap.size(); ++inputPortIdIndex)
    {
        ChiFeature2GlobalPortInstanceId inputPortInstanceId = pGraphNode->portToInputLinkIndexMap[inputPortIdIndex].portId;
        ChiFeature2GraphLinkData*       pInputLinkData      = MapInputPortIdToInputLinkData(&inputPortInstanceId, pGraphNode);

        if ((NULL != pInputLinkData) && (pInputLinkData->linkType != ChiFeature2GraphLinkType::ExternalSource))
        {
            ChiFeature2GlobalPortInstanceId*    pUpstreamPortId             =
                &pInputLinkData->linkDesc.internalLinkDesc.srcPortId;
            ChiFeature2Base*                    pUpstreamFeatureBase        =
                pInputLinkData->pSrcGraphNode->pFeatureBaseObj;

            // Tell each upstream FRO that processing is complete on this link by setting the processing complete flag
            for (UINT featureRequestObjIndex = 0;
                featureRequestObjIndex < pInputLinkData->pSrcGraphNode->pFeatureRequestObjList.size();
                ++featureRequestObjIndex)
            {
                ChiFeature2RequestObject* pUpstreamFeatureRequestObj =
                    pInputLinkData->pSrcGraphNode->pFeatureRequestObjList[featureRequestObjIndex];

                if (NULL != pUpstreamFeatureRequestObj)
                {
                    CHX_LOG_VERBOSE("Current FRO:%s notifying %s processing complete on port: [%d %d %d %d]",
                        pGraphNode->pFeatureRequestObjList.back()->IdentifierString(),
                        pUpstreamFeatureRequestObj->IdentifierString(),
                        pUpstreamPortId->portId.session,
                        pUpstreamPortId->portId.pipeline,
                        pUpstreamPortId->portId.port,
                        pUpstreamPortId->portId.portType);

                    pUpstreamFeatureRequestObj->SetProcessingCompleteForPort(pUpstreamPortId->portId);
                    result = pUpstreamFeatureBase->ProcessRequest(pUpstreamFeatureRequestObj);
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("pUpstreamFeatureBase->ProcessRequest() failed with result %d", result);
                        break;
                    }
                }
                else
                {
                    CHX_LOG_ERROR("pUpstreamFeatureRequestObj is NULL! Cannot SetProcessingCompleteForPort");
                    result = CDKResultEFailed;
                }
            }
        }
        else
        {
            if (pInputLinkData == NULL)
            {
                CHX_LOG_ERROR("pInputLink data was null!");
                result = CDKResultEFailed;
            }
        }
    }

    // The current graph node's feature logic has notified us that all processing is comlpete. Set the processing complete
    // notified flag on all of the current graph node's FROs.
    for (UINT featureRequestObjIndex = 0;
        featureRequestObjIndex < pGraphNode->pFeatureRequestObjList.size();
        ++featureRequestObjIndex)
    {
        pGraphNode->pFeatureRequestObjList[featureRequestObjIndex]->SetOutputPortsProcessingCompleteNotified();
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::ProcessPartialMetadataMessage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::ProcessPartialMetadataMessage(
    ChiFeature2GraphNode*   pGraphNode,
    ChiFeature2Messages*    pMessages)
{
    CDKResult result = CDKResultSuccess;

    CHICAPTUREREQUEST*        pChiRequest        = m_pUsecaseRequestObj->GetChiCaptureRequest();
    CHIPARTIALCAPTURERESULT   chiPartialResult   = { 0 };
    ChiFeature2RequestObject* pFeatureRequestObj = pGraphNode->pFeatureRequestObjList.back();
    ChiFeature2Identifier     identifier         = pMessages->pFeatureMessages->message.result.pPorts[0];

    ChiFeature2BufferMetadataInfo* pBufferMetadataInfo = NULL;

    chiPartialResult.frameworkFrameNum = pChiRequest->frameNumber;

    pFeatureRequestObj->GetFinalBufferMetadataInfo(identifier,
                                                   &pBufferMetadataInfo,
                                                   pMessages->pFeatureMessages->message.result.resultIndex);

    if ((NULL != pBufferMetadataInfo) && (NULL != pBufferMetadataInfo->hBuffer))
    {

        result = GetMetadataBuffer(pBufferMetadataInfo->hBuffer,
                                   pBufferMetadataInfo->key,
                                   &chiPartialResult.pPartialResultMetadata);
    }

    if (CDKResultSuccess == result)
    {
        m_featureGraphManagerCallbacks.ChiFeatureGraphProcessPartialResult(&chiPartialResult,
                                                                           m_featureGraphManagerCallbacks.pPrivateCallbackData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::MapInputPortIdToInputLinkData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphLinkData* ChiFeature2Graph::MapInputPortIdToInputLinkData(
    ChiFeature2GlobalPortInstanceId*    pInputPortId,
    ChiFeature2GraphNode*               pGraphNode)
{
    ChiFeature2GraphLinkData* pLinkData = NULL;

    if ((ChiFeature2PortDirectionType::ExternalInput == pInputPortId->portId.portDirectionType) ||
        (ChiFeature2PortDirectionType::InternalInput == pInputPortId->portId.portDirectionType))
    {
        // Iterate over the port ids to find the matching one and its assocated input link data
        for (UINT32 portIdIndex = 0; portIdIndex < pGraphNode->portToInputLinkIndexMap.size(); ++portIdIndex)
        {
            if ((*pInputPortId) == pGraphNode->portToInputLinkIndexMap[portIdIndex].portId)
            {
                pLinkData = &m_linkData[pGraphNode->portToInputLinkIndexMap[portIdIndex].linkIndex];
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid port type: %d %d",
            pInputPortId->portId.portDirectionType, pInputPortId->portId.portType);
    }

    return pLinkData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::MapOutputPortIdToOutputLinkData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphLinkData* ChiFeature2Graph::MapOutputPortIdToOutputLinkData(
    ChiFeature2GlobalPortInstanceId*    pOutputPortId,
    ChiFeature2GraphNode*               pGraphNode)
{
    ChiFeature2GraphLinkData* pLinkData = NULL;

    if ((ChiFeature2PortDirectionType::ExternalOutput == pOutputPortId->portId.portDirectionType) ||
        (ChiFeature2PortDirectionType::InternalOutput == pOutputPortId->portId.portDirectionType))
    {
        // Iterate over the port ids to find the matching one and its assocated output link data
        for (UINT32 portIdIndex = 0; portIdIndex < pGraphNode->portToOutputLinkIndexMap.size(); ++portIdIndex)
        {
            if ((*pOutputPortId) == pGraphNode->portToOutputLinkIndexMap[portIdIndex].portId)
            {
                pLinkData = &m_linkData[pGraphNode->portToOutputLinkIndexMap[portIdIndex].linkIndex];
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Invalid port type: %d %d",
            pOutputPortId->portId.portDirectionType, pOutputPortId->portId.portType);
    }

    return pLinkData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::CheckAllOutputLinksDisabled
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Graph::CheckAllOutputLinksDisabled(
    ChiFeature2GraphNode* pGraphNode)
{
    BOOL allLinksOutputDisabled = TRUE;

    // Iterate over all the output links attached to this node to determine their state
    for (UINT32 portIdIndex = 0; portIdIndex < pGraphNode->portToOutputLinkIndexMap.size(); ++portIdIndex)
    {
        UINT32 linkIndex = pGraphNode->portToOutputLinkIndexMap[portIdIndex].linkIndex;
        if (ChiFeature2GraphLinkState::Disabled != m_linkData[linkIndex].linkState)
        {
            allLinksOutputDisabled = FALSE;
        }
    }

    return allLinksOutputDisabled;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::CheckAllOutputLinksReadyToProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Graph::CheckAllOutputLinksReadyToProcessRequest(
    ChiFeature2GraphNode* pGraphNode)
{
    BOOL oneOutputPendingLinkFound              = FALSE;
    BOOL allLinksOutputReadyToProcessRequest    = TRUE;

    // Iterate over all the output links attached to this node to determine their state
    for (UINT32 portIdIndex = 0; portIdIndex < pGraphNode->portToOutputLinkIndexMap.size(); ++portIdIndex)
    {
        UINT32 linkIndex = pGraphNode->portToOutputLinkIndexMap[portIdIndex].linkIndex;
        if (ChiFeature2GraphLinkState::OutputPending == m_linkData[linkIndex].linkState)
        {
            oneOutputPendingLinkFound = TRUE;
        }

        if ((ChiFeature2GraphLinkState::OutputPending   != m_linkData[linkIndex].linkState) &&
            (ChiFeature2GraphLinkState::Disabled        != m_linkData[linkIndex].linkState))
        {
            allLinksOutputReadyToProcessRequest = FALSE;
            break;
        }
    }

    return (oneOutputPendingLinkFound && allLinksOutputReadyToProcessRequest);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::GetStreamBuffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::GetStreamBuffer(
    CHITARGETBUFFERINFOHANDLE   handle,
    UINT64                      key,
    CHISTREAMBUFFER**           ppChiStreamBuffer
    ) const
{
    CDKResult result = CDKResultSuccess;

    if (NULL == ppChiStreamBuffer)
    {
        CHX_LOG_ERROR("ppChiStreamBuffer is NULL");
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        CHITargetBufferManager* pTargetBufferManager = CHITargetBufferManager::GetTargetBufferManager(handle);
        if (NULL == pTargetBufferManager)
        {
            CHX_LOG_ERROR("Invalid pTargetBufferManager for handle=%p", handle);
            result = CDKResultEInvalidPointer;
        }
        else
        {
            *ppChiStreamBuffer = reinterpret_cast<CHISTREAMBUFFER*>(pTargetBufferManager->GetTarget(handle, key));
            if (NULL == *ppChiStreamBuffer)
            {
                CHX_LOG_ERROR("Unable to get buffer info for handle=%p", handle);
                result = CDKResultEInvalidPointer;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::GetChiStreamForRequestedPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2Graph::GetChiStreamForRequestedPort(
    ChiFeature2GlobalPortInstanceId& rInputPortId,
    CHISTREAM**                      ppChiStream,
    UINT32&                          rNumInputs
    ) const
{
    CDKResult result = CDKResultSuccess;
    // Find the stream associated to this port
    for (UINT32 portIdIndex = 0; portIdIndex < m_extSrcPortIdToChiStreamMap.size(); ++portIdIndex)
    {
        if (rInputPortId == m_extSrcPortIdToChiStreamMap[portIdIndex].portId)
        {
            *ppChiStream = m_extSrcPortIdToChiStreamMap[portIdIndex].pChiStream;
            break;
        }
    }

    if (NULL != *ppChiStream)
    {
        // Check if this port is requested from downstream link
        BOOL isRequired = FALSE;
        for (UINT32 inputStreamDataIndex = 0;
             inputStreamDataIndex < m_usecaseRequestObjInputStreamData.size();
             ++inputStreamDataIndex)
        {
            if (*ppChiStream == m_usecaseRequestObjInputStreamData[inputStreamDataIndex].pChiStream)
            {
                isRequired = TRUE;
                rNumInputs = m_usecaseRequestObjInputStreamData[inputStreamDataIndex].numInputs;
                break;
            }
        }

        // If this stream is not requested, set it NULL to return
        if (FALSE == isRequired)
        {
            CHX_LOG_INFO("Input link ChiStream: %p is found but not requested, set to be NULL", *ppChiStream);
            *ppChiStream = NULL;
        }
    }
    else
    {
        CHX_LOG_ERROR("Could not find corresponding ChiStream pointer");
        result = CDKResultEInvalidPointer;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2Graph::AreAllFROsProcessingComplete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2Graph::AreAllFROsProcessingComplete()
{
    BOOL allFROsProcessingComplete = TRUE;
    for (UINT8 i = 0; i < m_pFeatureGraphNodes.size(); i++)
    {
        ChiFeature2GraphNode* pGraphNode = m_pFeatureGraphNodes[i];
        for (UINT8 featureRequestObjIndex = 0;
             featureRequestObjIndex < pGraphNode->pFeatureRequestObjList.size();
             ++featureRequestObjIndex)
        {
            BOOL processingComplete =
                pGraphNode->pFeatureRequestObjList[featureRequestObjIndex]->
                GetOutputPortsProcessingCompleteNotified();
            if (FALSE == processingComplete)
            {
                allFROsProcessingComplete = FALSE;
                break;
            }
        }

        if (FALSE == allFROsProcessingComplete)
        {
            break;
        }
    }

    return allFROsProcessingComplete;
}
