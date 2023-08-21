////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxstatsparsenode.cpp
/// @brief Implements the node to parse stats
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camximagebuffer.h"
#include "camxstatscommon.h"
#include "camxstatsparsenode.h"
#include "camxvendortags.h"
CAMX_NAMESPACE_BEGIN

// Properties published by given input port

static const PropertyID StatsParserMetadataAWBBGPortOutputTags[] =
{
    PropertyIDParsedAWBBGStatsOutput
};

static const PropertyID StatsParserMetadataBHistPortOutputTags[] =
{
    PropertyIDParsedBHistStatsOutput
};

static const PropertyID StatsParserMetadataHDRBEPortOutputTags[] =
{
    PropertyIDParsedHDRBEStatsOutput
};

static const PropertyID StatsParserMetadataIHistPortOutputTags[] =
{
    PropertyIDParsedIHistStatsOutput
};

static const PropertyID StatsParserMetadataTintlessBGPortOutputTags[] =
{
    PropertyIDParsedTintlessBGStatsOutput
};

static const PropertyID StatsParserMetadataHDRBHistPortOutputTags[] =
{
    PropertyIDParsedHDRBHistStatsOutput
};

static const PropertyID StatsParserMetadataBPSAWBBGPortOutputTags[] =
{
    PropertyIDParsedAWBBGStatsOutput
};

// Vendor tags published by a given input port
static const struct NodeVendorTag StatsParserAWBBGPortOutputVendorTags[] =
{
    { "org.codeaurora.qcamera3.bayer_grid"     , "enable"      },
    { "org.codeaurora.qcamera3.bayer_grid"     , "stats_type"  },
    { "org.codeaurora.qcamera3.bayer_grid"     , "height"      },
    { "org.codeaurora.qcamera3.bayer_grid"     , "width"       },
    { "org.codeaurora.qcamera3.bayer_grid"     , "r_stats"     },
    { "org.codeaurora.qcamera3.bayer_grid"     , "b_stats"     },
    { "org.codeaurora.qcamera3.bayer_grid"     , "g_stats"     }
};

static const struct NodeVendorTag StatsParserBFPortOutputVendorTags[] =
{
    { "org.quic.camera.bafstats"               , "stats"       }
};

static const struct NodeVendorTag StatsParserBHistPortOutputVendorTags[] =
{
    { "org.codeaurora.qcamera3.histogram"      , "enable"      },
    { "org.codeaurora.qcamera3.histogram"      , "stats_type"  },
    { "org.codeaurora.qcamera3.histogram"      , "buckets"     },
    { "org.codeaurora.qcamera3.histogram"      , "max_count"   },
    { "org.codeaurora.qcamera3.histogram"      , "stats"       }
};

static const struct NodeVendorTag StatsParserHDRBEPortOutputVendorTags[] =
{
    { "org.codeaurora.qcamera3.bayer_exposure" , "enable"      },
    { "org.codeaurora.qcamera3.bayer_exposure" , "stats_type"  },
    { "org.codeaurora.qcamera3.bayer_exposure" , "height"      },
    { "org.codeaurora.qcamera3.bayer_exposure" , "width"       },
    { "org.codeaurora.qcamera3.bayer_exposure" , "r_stats"     },
    { "org.codeaurora.qcamera3.bayer_exposure" , "b_stats"     },
    { "org.codeaurora.qcamera3.bayer_exposure" , "g_stats"     },
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::GetFrameStatsOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* StatsParseNode::GetFrameStatsOutput(
    ISPStatsType statsType,
    UINT64       requestId)
{
    VOID* pOutput = NULL;

    // Round down to the last filled stats to republish during skipParse
    if (0 != (requestId % m_skipPattern))
    {
        requestId -= (requestId % m_skipPattern);
    }

    if (0 != m_requestQueueDepth)
    {
        switch (statsType)
        {
            case ISPStatsTypeHDRBE:
                pOutput = &m_pHDRBEStatsOutput[requestId % m_requestQueueDepth];
                break;
            case ISPStatsTypeHDRBHist:
                pOutput = &m_pHDRBHistStatsOutput[requestId % m_requestQueueDepth];
                break;
            case ISPStatsTypeBPSAWBBG: // Fall through as BPS and IFE AWB use the same buffer type.
            case ISPStatsTypeAWBBG:
                pOutput = &m_pAWBBGStatsOutput[requestId % m_requestQueueDepth];
                break;
            case ISPStatsTypeBPSRegYUV:
            case ISPStatsTypeRS:
                pOutput = &m_pRSStatsOutput[requestId % m_requestQueueDepth];
                break;
            case ISPStatsTypeBHist:
                pOutput = &m_pBHistStatsOutput[requestId % m_requestQueueDepth];
                break;
            case ISPStatsTypeTintlessBG:
                pOutput = &m_pTintlessBGStatsOutput[requestId % m_requestQueueDepth];
                break;
            case ISPStatsTypeIHist:
                pOutput = &m_pIHistStatsOutput[requestId % m_requestQueueDepth];
                break;
            default:
                CAMX_LOG_ERROR(CamxLogGroupStats, "Invalid stats type input %d", statsType);
                break;
        }
    }
    return pOutput;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::IsForceSkipRequested
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL StatsParseNode::IsForceSkipRequested()
{
    UINT32   isForceSkip = 0;

    if (0 != m_inputSkipFrameTag)
    {
        UINT tagReadInput[]    = { m_inputSkipFrameTag | InputMetadataSectionMask };
        VOID* pDataFrameSkip[] = { 0 };
        UINT64 dataOffset[1]   = { 0 };
        GetDataList(tagReadInput, pDataFrameSkip, dataOffset, 1);

        if (NULL != pDataFrameSkip[0])
        {
            isForceSkip = *static_cast<UINT32*>(pDataFrameSkip[0]);
        }
    }
    return ((isForceSkip) == 0) ? FALSE : TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::GetBufferDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsParseNode::GetBufferDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData)
{
    CAMX_ENTRYEXIT(CamxLogGroupStats);
    CamxResult result = CamxResultSuccess;

    if (NULL != pExecuteProcessRequestData)
    {
        PerRequestActivePorts*  pEnabledPorts                       = pExecuteProcessRequestData->pEnabledPortsInfo;
        NodeProcessRequestData* pNodeRequestData                    = pExecuteProcessRequestData->pNodeProcessRequestData;
        UINT64                  requestId                           = pNodeRequestData->pCaptureRequest->requestId;
        UINT64                  requestIdOffsetFromLastFlush        = GetRequestIdOffsetFromLastFlush(requestId);
        CSLFence                hDependentFence[MaxBufferComposite] = { CSLInvalidHandle };

        if ((NULL == pNodeRequestData) || (NULL == pEnabledPorts))
        {
            CAMX_LOG_ERROR(CamxLogGroupStats, "NULL pointer Access: pNodeRequestData:%p pEnabledPorts:%p",
                pNodeRequestData, pEnabledPorts);
            result = CamxResultEFailed;
        }

        if (CamxResultSuccess == result)
        {
            // A dependency can be reported without any actual dependencies and invocation will happen immediately
            // This helps simplify the special handling of the first requests by using the skipParse parameter to StatsParser
            if (m_bufferOffset >= requestIdOffsetFromLastFlush)
            {
                pEnabledPorts = &m_defaultPorts;
            }

            if (m_bufferOffset < requestIdOffsetFromLastFlush)
            {
                UINT32 count = 0;

                if (0 < pEnabledPorts->numInputPorts)
                {
                    // Use only one dependency for all ports
                    pNodeRequestData->numDependencyLists                                                  = 1;
                    pNodeRequestData->dependencyInfo[0].processSequenceId                                 = 1;
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasInputBuffersReadyDependency    = TRUE;
                    pNodeRequestData->dependencyInfo[0].dependencyFlags.hasIOBufferAvailabilityDependency = TRUE;
                }

                for (UINT portIndex = 0; portIndex < pEnabledPorts->numInputPorts; portIndex++)
                {
                    PerRequestInputPortInfo* pPerRequestInputPort = &pEnabledPorts->pInputPorts[portIndex];

                    for (UINT i = 0; i <= portIndex; i++)
                    {
                        if (hDependentFence[i] == *pPerRequestInputPort->phFence)
                        {
                            CAMX_LOG_VERBOSE(CamxLogGroupStats, "Fence handle %d, already added dependency portIndex %d "
                                "request %llu",
                                *pPerRequestInputPort->phFence,
                                portIndex,
                                requestId);
                            break;
                        }
                        else if (hDependentFence[i] == CSLInvalidHandle)
                        {
                            hDependentFence[i] = *pPerRequestInputPort->phFence;

                            pNodeRequestData->dependencyInfo[0].bufferDependency.phFences[count]         =
                                pPerRequestInputPort->phFence;

                            pNodeRequestData->dependencyInfo[0].bufferDependency.pIsFenceSignaled[count] =
                                pPerRequestInputPort->pIsFenceSignaled;

                            count++;

                            CAMX_LOG_VERBOSE(CamxLogGroupStats, "Add dependency for Fence handle %d, portIndex %d request %llu",
                                *pPerRequestInputPort->phFence,
                                portIndex,
                                requestId);

                            break;
                        }
                    }
                }

                pNodeRequestData->dependencyInfo[0].bufferDependency.fenceCount = count;
            }
            CAMX_LOG_VERBOSE(CamxLogGroupStats, "numDependencyLists %d, request ID %llu",
                pNodeRequestData->numDependencyLists,
                requestId);
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "pExecuteProcessRequestData is NULL");
        result = CamxResultEFailed;
    }

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::GetStatsPortIndex
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsParseNode::GetStatsPortIndex(
    ISPStatsType            statsType,
    PerRequestActivePorts*  pEnabledPorts,
    UINT32*                 pPortIndex)
{
    BOOL    isValidPortToParse  = TRUE;
    UINT    portId              = StatsInputPortMaxCount;
    CamxResult result           = CamxResultEFailed;

    PerRequestInputPortInfo* pPerRequestInputPort = NULL;

    switch (statsType)
    {
        case ISPStatsTypeAWBBG:
            portId = StatsInputPortAWBBG;
            break;
        case ISPStatsTypeBF:
            portId = StatsInputPortBF;
            break;
        case ISPStatsTypeBHist:
            portId = StatsInputPortBHist;
            break;
        case ISPStatsTypeHDRBE:
            portId = StatsInputPortHDRBE;
            break;
        case ISPStatsTypeHDRBHist:
            portId = StatsInputPortHDRBHist;
            break;
        case ISPStatsTypeIHist:
            portId = StatsInputPortIHist;
            break;
        case ISPStatsTypeCS:
            portId = StatsInputPortCS;
            break;
        case ISPStatsTypeRS:
            portId = StatsInputPortRS;
            break;
        case ISPStatsTypeTintlessBG:
            portId = StatsInputPortTintlessBG;
            break;
        case ISPStatsTypeBPSAWBBG:
            portId = StatsInputPortBPSAWBBG;
            break;
        case ISPStatsTypeBPSRegYUV:
            portId = StatsInputPortBPSRegYUV;
            break;
        case ISPStatsTypeRDIRaw:
        case ISPStatsTypeRDIPDAF:
        default:
            CAMX_LOG_VERBOSE(CamxLogGroupStats, "No parsing support for stats type %d", statsType);
            isValidPortToParse = FALSE;
            break;
    }

    if (TRUE == isValidPortToParse)
    {
        for (UINT i = 0; i < pEnabledPorts->numInputPorts; i++)
        {
            pPerRequestInputPort = &pEnabledPorts->pInputPorts[i];

            if (portId == pPerRequestInputPort->portId)
            {
                result    = CamxResultSuccess;
                *pPortIndex = i;
                break;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::GetStatsTypeProperties
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 StatsParseNode::GetStatsTypeProperties(
    ISPStatsType       statsType,
    const PropertyID** ppPropertyTags)
{
    UINT32 propertyTagsCount = 0;

    switch (statsType)
    {
        case ISPStatsTypeAWBBG:
            *ppPropertyTags   = StatsParserMetadataAWBBGPortOutputTags;
            propertyTagsCount = CAMX_ARRAY_SIZE(StatsParserMetadataAWBBGPortOutputTags);
            break;
        case ISPStatsTypeBHist:
            *ppPropertyTags   = StatsParserMetadataBHistPortOutputTags;
            propertyTagsCount = CAMX_ARRAY_SIZE(StatsParserMetadataBHistPortOutputTags);
            break;
        case ISPStatsTypeHDRBE:
            *ppPropertyTags   = StatsParserMetadataHDRBEPortOutputTags;
            propertyTagsCount = CAMX_ARRAY_SIZE(StatsParserMetadataHDRBEPortOutputTags);
            break;
        case ISPStatsTypeIHist:
            *ppPropertyTags   = StatsParserMetadataIHistPortOutputTags;
            propertyTagsCount = CAMX_ARRAY_SIZE(StatsParserMetadataIHistPortOutputTags);
            break;
        case ISPStatsTypeTintlessBG:
            *ppPropertyTags   = StatsParserMetadataTintlessBGPortOutputTags;
            propertyTagsCount = CAMX_ARRAY_SIZE(StatsParserMetadataTintlessBGPortOutputTags);
            break;
        case ISPStatsTypeHDRBHist:
            *ppPropertyTags   = StatsParserMetadataHDRBHistPortOutputTags;
            propertyTagsCount = CAMX_ARRAY_SIZE(StatsParserMetadataHDRBHistPortOutputTags);
            break;
        case ISPStatsTypeBPSAWBBG:
            *ppPropertyTags   = StatsParserMetadataBPSAWBBGPortOutputTags;
            propertyTagsCount = CAMX_ARRAY_SIZE(StatsParserMetadataBPSAWBBGPortOutputTags);
            break;
        case ISPStatsTypeBF:
        case ISPStatsTypeCS:
        case ISPStatsTypeRS:
        default:
            CAMX_LOG_VERBOSE(CamxLogGroupStats, "Stats type %d has no associated property tags.", statsType);
            break;
    }

    return propertyTagsCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::GetStatsTypeVendorTags
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT32 StatsParseNode::GetStatsTypeVendorTags(
    ISPStatsType                 statsType,
    const struct NodeVendorTag** ppVendorTags)
{
    UINT32 vendorTagsCount = 0;

    switch (statsType)
    {
        case ISPStatsTypeAWBBG:
            *ppVendorTags   = StatsParserAWBBGPortOutputVendorTags;
            vendorTagsCount = CAMX_ARRAY_SIZE(StatsParserAWBBGPortOutputVendorTags);
            break;
        case ISPStatsTypeBF:
            *ppVendorTags   = StatsParserBFPortOutputVendorTags;
            vendorTagsCount = CAMX_ARRAY_SIZE(StatsParserBFPortOutputVendorTags);
            break;
        case ISPStatsTypeBHist:
            *ppVendorTags   = StatsParserBHistPortOutputVendorTags;
            vendorTagsCount = CAMX_ARRAY_SIZE(StatsParserBHistPortOutputVendorTags);
            break;
        case ISPStatsTypeHDRBE:
            *ppVendorTags   = StatsParserHDRBEPortOutputVendorTags;
            vendorTagsCount = CAMX_ARRAY_SIZE(StatsParserHDRBEPortOutputVendorTags);
            break;
        case ISPStatsTypeIHist:
        case ISPStatsTypeTintlessBG:
        case ISPStatsTypeHDRBHist:
        case ISPStatsTypeCS:
        case ISPStatsTypeRS:
        case ISPStatsTypeBPSAWBBG:
        default:
            CAMX_LOG_VERBOSE(CamxLogGroupStats, "Stats type %d has no associated property tags.", statsType);
            break;
    }

    return vendorTagsCount;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::AddMetadataTagsForStatsType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsParseNode::AddMetadataTagsForStatsType(
    ISPStatsType            statsType,
    NodeMetadataList*       pPublistTagList,
    UINT32*                 pTagCount)
{
    CamxResult        result          = CamxResultSuccess;
    const PropertyID* pPropertyTags   = NULL;
    UINT32            numPropertyTags = 0;
    UINT32            tagCount        = *pTagCount;

    numPropertyTags = GetStatsTypeProperties(statsType, &pPropertyTags);

    if (numPropertyTags > 0)
    {
        if (tagCount + numPropertyTags < MaxTagsPublished)
        {
            for (UINT32 tagIndex = 0; tagIndex < numPropertyTags; ++tagIndex)
            {
                pPublistTagList->tagArray[tagCount++] = pPropertyTags[tagIndex];
            }

            *pTagCount = tagCount;
        }
        else
        {
            result = CamxResultEOutOfBounds;
            CAMX_LOG_ERROR(CamxLogGroupMeta, "ERROR More space needed to add %d publish metadata tags for stats type %d",
                statsType, numPropertyTags);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::AddVendorTagsForStatsType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsParseNode::AddVendorTagsForStatsType(
    ISPStatsType            statsType,
    NodeMetadataList*       pPublistTagList,
    UINT32*                 pTagCount)
{
    CamxResult                  result        = CamxResultSuccess;
    const struct NodeVendorTag* pVendorTags   = NULL;
    UINT32                      numVendorTags = 0;
    UINT32                      tagCount      = *pTagCount;
    UINT32                      tagID;

    numVendorTags = GetStatsTypeVendorTags(statsType, &pVendorTags);

    if (numVendorTags > 0)
    {
        if (tagCount + numVendorTags < MaxTagsPublished)
        {
            for (UINT32 tagIndex = 0; tagIndex < numVendorTags; ++tagIndex)
            {
                result = VendorTagManager::QueryVendorTagLocation(
                    pVendorTags[tagIndex].pSectionName,
                    pVendorTags[tagIndex].pTagName,
                    &tagID);

                if (CamxResultSuccess != result)
                {
                    CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot query vendor tag %s %s",
                        pVendorTags[tagIndex].pSectionName,
                        pVendorTags[tagIndex].pTagName);
                    break;
                }
                pPublistTagList->tagArray[tagCount++] = tagID;
            }

            *pTagCount = tagCount;
        }
        else
        {
            result = CamxResultEOutOfBounds;
            CAMX_LOG_ERROR(CamxLogGroupMeta, "ERROR More space needed to add %d publish vendor tags for stats type %d",
                statsType, numVendorTags);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::GetPortIndexStatsType
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsParseNode::GetPortIndexStatsType(
    UINT32                  portIndex,
    ISPStatsType*           pStatsType)
{
    CamxResult    result    = CamxResultSuccess;
    ISPStatsType  statsType = ISPStatsTypeMax;

    switch (portIndex)
    {
        case StatsInputPortAWBBG:
            statsType = ISPStatsTypeAWBBG;
            break;
        case StatsInputPortBF:
            statsType = ISPStatsTypeBF;
            break;
        case StatsInputPortBHist:
            statsType = ISPStatsTypeBHist;
            break;
        case StatsInputPortHDRBE:
            statsType = ISPStatsTypeHDRBE;
            break;
        case StatsInputPortHDRBHist:
            statsType = ISPStatsTypeHDRBHist;
            break;
        case StatsInputPortIHist:
            statsType = ISPStatsTypeIHist;
            break;
        case StatsInputPortCS:
            statsType = ISPStatsTypeCS;
            break;
        case StatsInputPortRS:
            statsType = ISPStatsTypeRS;
            break;
        case StatsInputPortTintlessBG:
            statsType = ISPStatsTypeTintlessBG;
            break;
        case StatsInputPortBPSAWBBG:
            statsType = ISPStatsTypeBPSAWBBG;
            break;
        case StatsInputPortBPSRegYUV:
            statsType = ISPStatsTypeBPSRegYUV;
            break;
        default:
            CAMX_LOG_VERBOSE(CamxLogGroupStats, "No parsing support for stats type %d", statsType);
            result = CamxResultEInvalidArg;
            break;
    }

    *pStatsType = statsType;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsParseNode::ExecuteProcessRequest(
    ExecuteProcessRequestData* pExecuteProcessRequestData)
{
    CamxResult result = CamxResultSuccess;

    if (NULL != pExecuteProcessRequestData)
    {
        if ((NULL != pExecuteProcessRequestData->pNodeProcessRequestData) &&
            (NULL != pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest))
        {
            NodeProcessRequestData*      pNodeRequestData             = pExecuteProcessRequestData->pNodeProcessRequestData;
            UINT64                       requestId                    = pNodeRequestData->pCaptureRequest->requestId;
            UINT64                       requestIdOffsetFromLastFlush = GetRequestIdOffsetFromLastFlush(requestId);

            CAMX_LOG_VERBOSE(CamxLogGroupStats, "ExecuteProcessRequest %u request id %llu requestIdOffsetFromLastFlush %llu",
                pNodeRequestData->processSequenceId, requestId, requestIdOffsetFromLastFlush);

            if ((0 == pNodeRequestData->processSequenceId) && (FALSE == IsForceSkipRequested()))
            {
                pNodeRequestData->numDependencyLists            = 0;
                result = GetBufferDependencies(pExecuteProcessRequestData);
            }

            if (CamxResultSuccess == result)
            {
                if (FALSE == Node::HasAnyDependency(pExecuteProcessRequestData->pNodeProcessRequestData->dependencyInfo))
                {
                    // Skip if in skip pattern request or before we have valid buffers
                    BOOL skipParse = ((0 == (requestIdOffsetFromLastFlush % m_skipPattern)) &&
                        (m_bufferOffset < requestIdOffsetFromLastFlush) &&
                        (FALSE == IsForceSkipRequested())) ? FALSE : TRUE;

                    PerRequestActivePorts*   pEnabledPorts        = pExecuteProcessRequestData->pEnabledPortsInfo;
                    PerRequestInputPortInfo* pPerRequestInputPort = NULL;
                    VOID*                    pParsedOutput        = NULL;
                    VOID*                    pRawStatsBuffer      = NULL;
                    ParseData                parseData            = {0};

                    if ((m_bufferOffset >= requestIdOffsetFromLastFlush) || IsForceSkipRequested())
                    {
                        pEnabledPorts = &m_defaultPorts;
                    }

                    for (UINT portIndex = 0; portIndex < pEnabledPorts->numInputPorts; portIndex++)
                    {
                        pPerRequestInputPort       = &pEnabledPorts->pInputPorts[portIndex];

                        if (NULL == pPerRequestInputPort)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupStats, "pPerRequestInputPort is NULL for portIndex:%d",
                                portIndex);
                            result = CamxResultEFailed;
                            break;
                        }

                        const ImageFormat* pFormat   = (NULL == pPerRequestInputPort->pImageBuffer) ?
                                NULL : pPerRequestInputPort->pImageBuffer->GetFormat();
                        UINT               statsType = StatsUtil::GetStatsType(pPerRequestInputPort->portId);

                        pParsedOutput   = GetFrameStatsOutput(static_cast<ISPStatsType>(statsType), requestId);
                        pRawStatsBuffer = (NULL == pPerRequestInputPort->pImageBuffer) ? NULL :
                                            pPerRequestInputPort->pImageBuffer->GetPlaneVirtualAddr(0, 0);
                        parseData       = { this, skipParse, pRawStatsBuffer, pParsedOutput, pFormat };

                        CAMX_LOG_VERBOSE(CamxLogGroupStats, "Request id %llu, statsType %d", requestId, statsType);

                        // don't do buffer ops when skipParse is TRUE
                        if ((NULL != pPerRequestInputPort->pImageBuffer) && (FALSE == skipParse))
                        {
                            result = pPerRequestInputPort->pImageBuffer->CacheOps(TRUE, FALSE);
                        }

                        if (CamxResultSuccess == result)
                        {
                            result = m_pStatsParser->Parse(static_cast<ISPStatsType>(statsType), &parseData);
                        }
                        else
                        {
                            CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to invalidate cached stat buffer.");
                        }

                        if (CamxResultSuccess != result)
                        {
                            CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to parse %d stats result: %s",
                                statsType, Utils::CamxResultToString(result));
                        }

                    }

                    if (CamxResultSuccess == result)
                    {
                        // Parallel processing tracking. Once all the buffers have been individually processed report done
                        // so pipeline doesnt wait for us.
                        // Ideally, as long as we have nodes consuming this data, they will block for us, but don't
                        // assume it
                        ProcessPartialMetadataDone(requestId);
                        ProcessMetadataDone(requestId);
                        ProcessRequestIdDone(requestId);
                    }
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to get buffer dependencies.");
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupStats, "NULL pointer Access: pNodeProcessRequestData:%p",
            pExecuteProcessRequestData->pNodeProcessRequestData);
            result = CamxResultEFailed;
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "pExecuteProcessRequestData is NULL");
        result = CamxResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::ProcessingNodeInitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsParseNode::ProcessingNodeInitialize(
    const NodeCreateInputData*  pCreateInputData,
    NodeCreateOutputData*       pCreateOutputData)
{
    CAMX_ENTRYEXIT_SCOPE(CamxLogGroupStats, SCOPEEventStatsProcessingNodeProcessingNodeInitialize);
    CamxResult result = CamxResultSuccess;

    pCreateOutputData->pNodeName = m_pNodeName;

    CAMX_UNREFERENCED_PARAM(pCreateInputData);

    UINT inputPortId[StatsParseInputPortMaxCount];
    UINT numInputPort = 0;

    m_bufferOffset = GetMaximumPipelineDelay();

    GetAllInputPortIds(&numInputPort, &inputPortId[0]);

    for (UINT inputIndex = 0; inputIndex < numInputPort; inputIndex++)
    {
        // need request - 4 buffer
        SetInputPortBufferDelta(inputIndex, m_bufferOffset);
    }

    // Map Input ports to default structure to use during initial system ramp up.
    m_defaultPorts.pInputPorts = reinterpret_cast<PerRequestInputPortInfo*>(
                                  CAMX_CALLOC(sizeof(PerRequestInputPortInfo) * StatsParseInputPortMaxCount));

    if (NULL == m_defaultPorts.pInputPorts)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Unable to allocate default input port array information");
    }
    else
    {
        for (UINT stat = 0; stat < numInputPort; stat++)
        {
            m_defaultPorts.pInputPorts[stat].portId = inputPortId[stat];
        }
        m_defaultPorts.numInputPorts = numInputPort;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::ProcessingNodeFinalizeInitialization
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsParseNode::ProcessingNodeFinalizeInitialization(
    FinalizeInitializationData* pFinalizeInitializationData)
{
    CAMX_ENTRYEXIT_NAME(CamxLogGroupStats, "StatsParseNode::ProcessingNodeFinalizeInitialization");

    m_requestQueueDepth = GetPipeline()->GetRequestQueueDepth();
    CamxResult   result = CamxResultENoMemory;

    // Store the required input information.
    m_pThreadManager = pFinalizeInitializationData->pThreadManager;
    m_pHwContext     = pFinalizeInitializationData->pHwContext;
    m_pStatsParser   = m_pHwContext->GetStatsParser();


    m_pAWBBGStatsOutput = CAMX_NEW ParsedAWBBGStatsOutput[m_requestQueueDepth];
    if (NULL != m_pAWBBGStatsOutput)
    {
        m_pBHistStatsOutput = CAMX_NEW ParsedBHistStatsOutput[m_requestQueueDepth];
        if (NULL != m_pBHistStatsOutput)
        {
            m_pHDRBEStatsOutput = CAMX_NEW ParsedHDRBEStatsOutput[m_requestQueueDepth];
            if (NULL != m_pHDRBEStatsOutput)
            {
                m_pHDRBHistStatsOutput = CAMX_NEW ParsedHDRBHistStatsOutput[m_requestQueueDepth];
                if (NULL != m_pHDRBHistStatsOutput)
                {
                    m_pRSStatsOutput = CAMX_NEW ParsedRSStatsOutput[m_requestQueueDepth];
                    if (NULL != m_pRSStatsOutput)
                    {
                        m_pTintlessBGStatsOutput = CAMX_NEW ParsedTintlessBGStatsOutput[m_requestQueueDepth];
                        if (NULL != m_pTintlessBGStatsOutput )
                        {
                            m_pIHistStatsOutput = CAMX_NEW ParsedIHistStatsOutput[m_requestQueueDepth];
                            if (NULL != m_pIHistStatsOutput)
                            {
                                result = CamxResultSuccess;
                            }
                        }
                    }
                }
            }
        }
    }


    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Unable to allocate buffers for Parser output");
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::~StatsParseNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsParseNode::~StatsParseNode()
{
    if (NULL != m_defaultPorts.pInputPorts)
    {
        CAMX_FREE(m_defaultPorts.pInputPorts);
        m_defaultPorts.pInputPorts = NULL;
    }

    if (NULL != m_pAWBBGStatsOutput)
    {
        CAMX_DELETE[] m_pAWBBGStatsOutput;
        m_pAWBBGStatsOutput = NULL;
    }

    if (NULL != m_pBHistStatsOutput)
    {
        CAMX_DELETE[] m_pBHistStatsOutput;
        m_pBHistStatsOutput = NULL;
    }

    if (NULL != m_pHDRBEStatsOutput)
    {
        CAMX_DELETE[] m_pHDRBEStatsOutput;
        m_pHDRBEStatsOutput = NULL;
    }

    if (NULL != m_pHDRBHistStatsOutput)
    {
        CAMX_DELETE[] m_pHDRBHistStatsOutput;
        m_pHDRBHistStatsOutput = NULL;
    }

    if (NULL != m_pRSStatsOutput)
    {
        CAMX_DELETE[] m_pRSStatsOutput;
        m_pRSStatsOutput = NULL;
    }

    if (NULL != m_pTintlessBGStatsOutput)
    {
        CAMX_DELETE[] m_pTintlessBGStatsOutput;
        m_pTintlessBGStatsOutput = NULL;
    }

    if (NULL != m_pIHistStatsOutput)
    {
        CAMX_DELETE[] m_pIHistStatsOutput;
        m_pIHistStatsOutput = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::StatsParseNode
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsParseNode::StatsParseNode(
    const NodeCreateInputData* pCreateInputData)
{
    m_pNodeName                  = "StatsParse";
    m_parallelProcessRequests    = TRUE;
    m_derivedNodeHandlesMetaDone = TRUE;
    m_skipPattern                = 1;

    for (UINT32 i = 0; i < pCreateInputData->pNodeInfo->nodePropertyCount; i++)
    {
        if (NodePropertyStatsSkipPattern == pCreateInputData->pNodeInfo->pNodeProperties[i].id)
        {
            m_skipPattern = *static_cast<UINT*>(pCreateInputData->pNodeInfo->pNodeProperties[i].pValue);
        }
    }

    m_inputSkipFrameTag = 0;

    if (CDKResultSuccess !=
            VendorTagManager::QueryVendorTagLocation("com.qti.chi.statsSkip", "skipFrame", &m_inputSkipFrameTag))
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to query stats skip vendor tag");
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsParseNode* StatsParseNode::Create(
    const NodeCreateInputData* pCreateInputData,
    NodeCreateOutputData*      pCreateOutputData)
{
    CAMX_ENTRYEXIT_NAME(CamxLogGroupStats, "StatsParseNodeCreate");
    CAMX_UNREFERENCED_PARAM(pCreateOutputData);

    return CAMX_NEW StatsParseNode(pCreateInputData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsParseNode::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsParseNode::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// StatsParseNode::QueryMetadataPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsParseNode::QueryMetadataPublishList(
    NodeMetadataList* pPublistTagList)
{
    CamxResult   result             = CamxResultSuccess;
    UINT32       tagCount           = 0;
    ISPStatsType statsType;

    for (UINT32 inputPortIdx = 0; inputPortIdx < m_defaultPorts.numInputPorts; inputPortIdx++)
    {
        result = GetPortIndexStatsType(
            m_defaultPorts.pInputPorts[inputPortIdx].portId,
            &statsType);

        if (CamxResultSuccess == result)
        {
            result = AddMetadataTagsForStatsType(statsType, pPublistTagList, &tagCount);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot add stats type %d meatadata tags",
                                statsType);
            break;
        }

        if (CamxResultSuccess == result)
        {
            result = AddVendorTagsForStatsType(statsType, pPublistTagList, &tagCount);
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupMeta, "Cannot add stats type %d vendor tags",
                                statsType);
            break;
        }
    }

    if (CamxResultSuccess == result)
    {
        pPublistTagList->tagCount = tagCount;
        CAMX_LOG_VERBOSE(CamxLogGroupMeta, "%d tags will be published", tagCount);
    }

    return result;
}

CAMX_NAMESPACE_END
