////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxmultistatsoperator.cpp
/// @brief Implements the multi statistics operator for multi camera framework.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxmultistatsoperator.h"
#include "camxvendortags.h"
#include "chivendortag.h"

CAMX_NAMESPACE_BEGIN

// Dependency table for Master/Follower mode
MultiStatsDependency qtiDependencyTable[] =
{
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAEC,    // Represent dependencies from Stats node (Only AEC)
        StatsAlgoRole::StatsAlgoRoleMaster,         // Role is Master
        {
            {PropertyIDAECPeerInfo, TRUE},          // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessRequest                     // Action is to perform process request as Master
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAEC,    // Represent dependencies from Stats node (Only AEC)
        StatsAlgoRole::StatsAlgoRoleSlave,          // Role is Slave
        {
            {PropertyIDAECPeerInfo, TRUE},          // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessMapping                     // Action is to perform mapping procedure as Follower(Slave)
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAWB,    // Represent dependencies from AWB node
        StatsAlgoRole::StatsAlgoRoleMaster,         // Role is Master
        {
            { PropertyIDAWBPeerInfo, TRUE },        // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessRequest                     // Action is to perform process request as Master
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAWB,    // Represent dependencies from AWB node
        StatsAlgoRole::StatsAlgoRoleSlave,          // Role is Slave
        {
            { PropertyIDAWBPeerInfo, TRUE },        // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessMapping                     // Action is to perform mapping procedure as Follower(Slave)
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAF,     // Represent dependencies from AF node
        StatsAlgoRole::StatsAlgoRoleMaster,         // Role is Master
        {
            {PropertyIDAFPeerInfo, TRUE},           // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessRequest                     // Action is to perform process request as Master
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAF,     // Represent dependencies from AF node
        StatsAlgoRole::StatsAlgoRoleSlave,          // Role is Slave
        {
            {PropertyIDAFPeerInfo, TRUE},           // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {},                                     // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessMapping                     // Action is to perform mapping procedure as Follower(Slave)
    }
};

// Dependency table for Role-Switch process
MultiStatsDependency qtiRoleSwitchDependencyTable[] =
{
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAEC,    // Represent dependencies from Stats node (Only AEC)
        StatsAlgoRole::StatsAlgoRoleMaster,         // New role is Master, its previous role was Slave
        {
            {},                                     // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessSwitchingMaster             // Action is to perform procedure of switching to Master
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAEC,    // Represent dependencies from Stats node (Only AEC)
        StatsAlgoRole::StatsAlgoRoleSlave,          // New role is Slave, its previous role was Master
        {
            {},                                     // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessSwitchingSlave              // Action is to perform procedure of switching to Slave
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAWB,    // Represent dependencies from AWB node
        StatsAlgoRole::StatsAlgoRoleMaster,         // New role is Master, its previous role was Slave
        {
            {},                                     // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessSwitchingMaster             // Action is to perform procedure of switching to Master
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAWB,    // Represent dependencies from AWB node
        StatsAlgoRole::StatsAlgoRoleSlave,          // New role is Slave, its previous role was Master
        {
            {},                                     // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessSwitchingSlave              // Action is to perform procedure of switching to Slave
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAF,     // Represent dependencies from AF node
        StatsAlgoRole::StatsAlgoRoleMaster,         // New role is Master, its previous role was Slave
        {
            {},                                     // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessSwitchingMaster             // Action is to perform procedure of switching to Master
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAF,     // Represent dependencies from AF node
        StatsAlgoRole::StatsAlgoRoleSlave,          // New role is Slave, its previous role was Master
        {
            {},                                     // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessSwitchingSlave              // Action is to perform procedure of switching to Slave
    }
};

// Dependency table for Singleton Algo usecase
MultiStatsDependency singletonDependencyTable[] =
{
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAEC,    // Represent dependencies from Stats node (Only AEC)
        StatsAlgoRole::StatsAlgoRoleMaster,         // Role is Master
        {
            {PropertyIDCrossAECStats, TRUE},        // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessRequest                     // Action is to perform process request
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAEC,    // Represent dependencies from Stats node (Only AEC)
        StatsAlgoRole::StatsAlgoRoleSlave,          // Role is Slave
        {
            {PropertyIDCrossAECStats, TRUE},        // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessRequest                     // Action is to perform process request
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAWB,    // Represent dependencies from AF node
        StatsAlgoRole::StatsAlgoRoleMaster,         // Role is Master
        {
            { PropertyIDCrossAWBStats, TRUE },      // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessRequest                     // Action is to perform process request
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAWB,     // Represent dependencies from AF node
        StatsAlgoRole::StatsAlgoRoleSlave,          // Role is Slave
        {
            { PropertyIDCrossAWBStats, TRUE },      // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessRequest                     // Action is to perform process request
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAF,     // Represent dependencies from AWB node
        StatsAlgoRole::StatsAlgoRoleMaster,         // Role is Master
        {
            {PropertyIDCrossAFStats, TRUE},         // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessRequest                     // Action is to perform process request
    },
    {
        StatsAlgoSyncType::StatsAlgoSyncTypeAF,     // Represent dependencies from AWB node
        StatsAlgoRole::StatsAlgoRoleSlave,          // Role is Slave
        {
            {PropertyIDCrossAFStats, TRUE},         // Cross Pipeline Dependency 0
            {},                                     // Cross Pipeline Dependency 1
            {},                                     // Cross Pipeline Dependency 2
            {},                                     // Cross Pipeline Dependency 3
            {}                                      // Cross Pipeline Dependency 4
        },
        StatsAlgoProcessRequest                     // Action is to perform process request
    }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MultiStatsOperator::SetStatsAlgoRole
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiStatsOperator::SetStatsAlgoRole(
    StatsAlgoRole algoRole)
{
    m_multiStatsData.algoRole = algoRole;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MultiStatsOperator::GetStatsAlgoRole
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsAlgoRole MultiStatsOperator::GetStatsAlgoRole()
{
    return m_multiStatsData.algoRole;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MultiStatsOperator::PrintDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID MultiStatsOperator::PrintDependency(
    DependencyUnit*     pStatsDependencies)
{
    UINT index = 0;

    if (NULL == pStatsDependencies)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Null pointer!");
        return;
    }

    CAMX_LOG_INFO(CamxLogGroupStats, "Debug Dependency 1/3: Mask:0x%x BuffReady:%d BuffAvailability:%d Prop:%d",
        pStatsDependencies->dependencyFlags.dependencyFlagsMask,
        pStatsDependencies->dependencyFlags.hasInputBuffersReadyDependency,
        pStatsDependencies->dependencyFlags.hasIOBufferAvailabilityDependency,
        pStatsDependencies->dependencyFlags.hasPropertyDependency);

    CAMX_LOG_INFO(CamxLogGroupStats, "Debug Dependency 2/3: Buffer Dependency Count:%d",
        pStatsDependencies->bufferDependency.fenceCount);
    for (index = 0; index < pStatsDependencies->bufferDependency.fenceCount; index++)
    {
        CAMX_LOG_INFO(CamxLogGroupStats, "Debug Dependency 2/3_%d: Fence:%d Signaled:%s",
            index,
            *(pStatsDependencies->bufferDependency.phFences[index]),
            *(pStatsDependencies->bufferDependency.pIsFenceSignaled[index]) ? "Yes":"No");
    }

    CAMX_LOG_INFO(CamxLogGroupStats, "Debug Dependency 3/3: Property Dependency Count:%d",
        pStatsDependencies->propertyDependency.count);
    for (index = 0; index < pStatsDependencies->propertyDependency.count; index++)
    {
        CAMX_LOG_INFO(CamxLogGroupStats, "Debug Dependency 3/3_%d: Property:%X Pipeline:%d",
            index,
            pStatsDependencies->propertyDependency.properties[index],
            pStatsDependencies->propertyDependency.pipelineIds[index]);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MultiStatsOperator::AddDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult MultiStatsOperator::AddDependency(
    PropertyPair*           pPropertyPair,
    DependencyUnit*         pDependencyUnit,
    UINT64                  requestId,
    UINT64                  offset,
    BOOL                    negative)
{
    UINT        pipelineId  = 0;
    CamxResult  result      = CamxResultSuccess;

    if (pPropertyPair->isPeerDependency)
    {
        // Dependency is peer pipeline property, will check from peer pipeline
        pipelineId  = m_multiStatsData.peerPipelineId;
    }
    else
    {
        // Dependency is own pipeline property, will check from own pipeline
        pipelineId = m_multiStatsData.pipelineId;
    }

    // Dependent property need add to DRQ, and claiming "has property dependency"
    UINT32 count = pDependencyUnit->propertyDependency.count;

    pDependencyUnit->propertyDependency.properties[count]   = pPropertyPair->propertyIDs;
    pDependencyUnit->propertyDependency.pipelineIds[count]  = pipelineId;
    pDependencyUnit->propertyDependency.offsets[count]      = offset;
    pDependencyUnit->propertyDependency.negate[count]       = negative;
    pDependencyUnit->propertyDependency.count++;

    pDependencyUnit->dependencyFlags.hasPropertyDependency = TRUE;

    CAMX_LOG_VERBOSE(CamxLogGroupStats, "Add Prop:%X|Pipeline:%d|Offset:-%lld for type:%d role:%d Req:%llu Pipeline:%d",
                     pPropertyPair->propertyIDs,
                     pipelineId,
                     negative ? (-1 * static_cast<INT64>(offset)) : offset,
                     m_multiStatsData.algoSyncType,
                     m_multiStatsData.algoRole,
                     requestId,
                     m_multiStatsData.pipelineId);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QTIMultiStatsOperator::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID QTIMultiStatsOperator::Initialize(
    const MultiStatsData* pMultiStatsData)
{
    CAMX_ASSERT(NULL != pMultiStatsData);

    m_multiStatsData.algoSyncType       = pMultiStatsData->algoSyncType;
    m_multiStatsData.algoRole           = pMultiStatsData->algoRole;
    m_multiStatsData.pipelineId         = pMultiStatsData->pipelineId;
    m_multiStatsData.pHwContext         = pMultiStatsData->pHwContext;
    m_multiStatsData.algoAction         = StatsAlgoProcessRequest;
    m_multiStatsData.isSlaveOperational = TRUE;
    m_multiStatsData.pNode              = pMultiStatsData->pNode;

    CAMX_ASSERT(StatsAlgoSyncType::StatsAlgoSyncTypeAEC == m_multiStatsData.algoSyncType ||
                StatsAlgoSyncType::StatsAlgoSyncTypeAF  == m_multiStatsData.algoSyncType ||
                StatsAlgoSyncType::StatsAlgoSyncTypeAWB == m_multiStatsData.algoSyncType);
    CAMX_ASSERT(StatsAlgoRole::StatsAlgoRoleDefault == m_multiStatsData.algoRole ||
                StatsAlgoRole::StatsAlgoRoleMaster  == m_multiStatsData.algoRole ||
                StatsAlgoRole::StatsAlgoRoleSlave   == m_multiStatsData.algoRole);
    CAMX_ASSERT(NULL != m_multiStatsData.pHwContext);
    CAMX_ASSERT(NULL != m_multiStatsData.pNode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QTIMultiStatsOperator::UpdateStatsDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult QTIMultiStatsOperator::UpdateStatsDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsRequestData,
    UINT64                      requestIdOffsetFromLastFlush)
{
    CamxResult              result                  = CamxResultSuccess;
    UINT                    index                   = 0;
    MultiStatsDependency*   pMultiStatsDependency   = NULL;
    INT64                   lpmRequestDelta         = 0;
    UINT64                  requestId               = pStatsRequestData->requestId;
    NodeProcessRequestData* pNodeRequestData        = pExecuteProcessRequestData->pNodeProcessRequestData;
    DependencyUnit*         pStatsDependencies      = &(pNodeRequestData->dependencyInfo[0]);
    CaptureRequest*         pCaptureRequest         = pNodeRequestData->pCaptureRequest;

    CAMX_LOG_VERBOSE(CamxLogGroupStats,
                     "Update dependency for Node::%s Master:%d Req:%llu Pipeline:%d requestIdOffsetFromLastFlush:%llu",
                     m_multiStatsData.pNode->NodeIdentifierString(),
                     (m_multiStatsData.algoRole == StatsAlgoRoleMaster) ? 1 : 0,
                     requestId,
                     m_multiStatsData.pipelineId,
                     requestIdOffsetFromLastFlush);

    result = ParseMultiRequestInfo(pExecuteProcessRequestData, pStatsRequestData, requestIdOffsetFromLastFlush);

    if (CamxResultSuccess == result)
    {
        StatsAlgoRole algoRole = (TRUE == pCaptureRequest->peerSyncInfo.isMaster) ? StatsAlgoRoleMaster : StatsAlgoRoleSlave;

        if (algoRole != m_multiStatsData.algoRole)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupStats, "Role switched from %s to %s for Req:%llu",
                             (m_multiStatsData.algoRole == StatsAlgoRoleMaster) ? "Master" : "Slave",
                             (algoRole == StatsAlgoRoleMaster) ? "Master" : "Slave",
                             requestId);
            m_multiStatsData.algoRole = algoRole;
        }

        if (TRUE == pCaptureRequest->peerSyncInfo.needSync)
        {
            m_multiStatsData.peerPipelineId = pCaptureRequest->peerSyncInfo.peerPipelineID;
            lpmRequestDelta                 = pCaptureRequest->peerSyncInfo.requestDelta;
            pMultiStatsDependency           = NULL;

            for (index = 0; index < sizeof(qtiDependencyTable) / sizeof(MultiStatsDependency); index++)
            {
                if (qtiDependencyTable[index].algoRole == m_multiStatsData.algoRole)
                {
                    if (qtiDependencyTable[index].algoSyncType == m_multiStatsData.algoSyncType)
                    {
                        // Found matched item
                        pMultiStatsDependency = &(qtiDependencyTable[index]);
                        break;
                    }
                }
            }

            if (NULL != pMultiStatsDependency)
            {
                index = 0;
                while ((MaxCrossPipelineProperties > index) &&
                       (0 != pMultiStatsDependency->propertyPair[index].propertyIDs))
                {
                    result = AddDependency(&(pMultiStatsDependency->propertyPair[index]),
                                           pStatsDependencies,
                                           requestId,
                                           static_cast<UINT64>(abs(lpmRequestDelta)),
                                           (lpmRequestDelta < 0));
                    if (CamxResultSuccess == result)
                    {
                        index++;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupStats, "Add property %X to dependency failed",
                                       pMultiStatsDependency->propertyPair[index].propertyIDs);
                        break;
                    }
                }

                if (CamxResultSuccess == result)
                {
                    m_multiStatsData.algoAction = pMultiStatsDependency->algoAction;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupStats, "No match item found");
                result = CamxResultENoSuch;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Parsing Multi Request SyncData Failed!!!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QTIMultiStatsOperator::GetStatsAlgoAction
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsAlgoAction QTIMultiStatsOperator::GetStatsAlgoAction()
{
    return m_multiStatsData.algoAction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QTIMultiStatsOperator::RemoveDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID QTIMultiStatsOperator::RemoveDependency(
    StatsDependency* pStatsDependency)
{
    CAMX_UNREFERENCED_PARAM(pStatsDependency);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// QTIMultiStatsOperator::ParseMultiRequestInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult QTIMultiStatsOperator::ParseMultiRequestInfo(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsRequestData,
    UINT64                      requestIdOffsetFromLastFlush)
{
    CamxResult      result              = CamxResultEFailed;
    UINT            index               = 0;
    UINT            numOfActivePipeline = 0;
    BOOL            foundOwnPipeline    = FALSE;
    BOOL            foundPeerPipeline   = FALSE;
    UINT            peerPipelineId      = 0;
    BOOL            isMaster            = FALSE;
    CaptureRequest* pCaptureRequest     = pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest;

    if (NULL == pStatsRequestData)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Error: Multi request data pointer is NULL!");
        return CamxResultEFailed;
    }

    MultiRequestSyncData* pMultiReqSync     = pStatsRequestData->pMultiRequestSync;
    UINT64                requestID         = pStatsRequestData->requestId;
    UINT64                peerRequestID     = 0;

    for (index = 0; index < pMultiReqSync->numPipelines; index++)
    {
        if (TRUE == pMultiReqSync->currReq.isActive[index])
        {
            numOfActivePipeline++;

            if (m_multiStatsData.pipelineId == index)
            {
                foundOwnPipeline = TRUE;
                isMaster = pMultiReqSync->currReq.isMaster[index];
                pCaptureRequest->peerSyncInfo.isMaster = isMaster;
            }
            else
            {
                foundPeerPipeline = TRUE;
                peerPipelineId = index;
            }
        }
    }

    if ((numOfActivePipeline <= MaxActiveRealtimePipeline) && (numOfActivePipeline > 0) && (TRUE == foundOwnPipeline))
    {
        if (TRUE == isMaster)
        {
            // Current owning node is Master
            if (FirstValidRequestId != requestIdOffsetFromLastFlush)
            {
                // Generally, we need current Master depend on previous Slave if previous is a Multi request
                // Otherwise, we still want to depend on previous Master if previous is NOT a Multi request
                UINT    numOfPrevActivePipeline     = 0;
                BOOL    foundPrevSlave              = FALSE;
                BOOL    foundPrevMaster             = FALSE;
                UINT    prevSlavePipelineId         = 0;
                UINT    prevMasterPipelineId        = 0;

                for (index = 0; index < pMultiReqSync->numPipelines; index++)
                {
                    // Search in previous active pipeline that is not current own pipeline
                    if (TRUE == pMultiReqSync->prevReq.isActive[index])
                    {
                        numOfPrevActivePipeline++;
                        if (index != m_multiStatsData.pipelineId)
                        {
                            if (FALSE == pMultiReqSync->prevReq.isMaster[index])
                            {
                                foundPrevSlave       = TRUE;
                                prevSlavePipelineId  = index;
                            }
                            else
                            {
                                foundPrevMaster      = TRUE;
                                prevMasterPipelineId = index;
                            }
                        }
                    }
                }

                if ((numOfPrevActivePipeline <= MaxActiveRealtimePipeline) && (numOfPrevActivePipeline > 0))
                {
                    if (TRUE == foundPrevSlave)
                    {
                        peerPipelineId = prevSlavePipelineId;
                        peerRequestID = pMultiReqSync->prevReq.requestID[peerPipelineId];
                        pCaptureRequest->peerSyncInfo.needSync = TRUE;
                        result = CamxResultSuccess;
                    }
                    else if (TRUE == foundPrevMaster)
                    {
                        peerPipelineId = prevMasterPipelineId;
                        peerRequestID = pMultiReqSync->prevReq.requestID[peerPipelineId];
                        pCaptureRequest->peerSyncInfo.needSync = TRUE;
                        result = CamxResultSuccess;
                    }
                    else
                    {
                        // Sanity check has passed, but neither foundPrevSlave nor foundPrevMaster was found
                        // This happens when consecutive Single request were on same node, no need to sync for same pipeline
                        pCaptureRequest->peerSyncInfo.needSync = FALSE;
                        result = CamxResultSuccess;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupStats, "Error parsing Prev Multi-req info: NumPrev:%d",
                                   numOfPrevActivePipeline);
                    pCaptureRequest->peerSyncInfo.needSync = FALSE;
                    result = CamxResultEFailed;
                }
            }
            else
            {
                // We don't need to set any dependencies to Master with syncSequenceId = 1
                pCaptureRequest->peerSyncInfo.needSync = FALSE;
                result = CamxResultSuccess;
                CAMX_LOG_VERBOSE(CamxLogGroupStats, "No need to add dependency: Offset From Last Flush:%llu Prev Multi:%d",
                                 requestIdOffsetFromLastFlush,
                                 pMultiReqSync->prevReq.isMultiRequest);
            }
        }
        else
        {
            // Current owning node is Slave
            if ((TRUE == foundPeerPipeline) && (TRUE == pMultiReqSync->currReq.isMultiRequest))
            {
                // Found a slave node here, implication is there must be a multi request ongoing,
                // and there must be a parallel master node in peer pipeline, this slave node will
                // therefore depend on current master.
                peerRequestID = pMultiReqSync->currReq.requestID[peerPipelineId];
                pCaptureRequest->peerSyncInfo.needSync = TRUE;
                result = CamxResultSuccess;
            }
            else
            {
                // Parsing error, either we failed to find peer pipeline(Master one), or current is not Multi request
                CAMX_LOG_ERROR(CamxLogGroupStats, "Error parsing Multi-req info: isMaster:%d foundPeerPipeline:%d isMulti:%d",
                               isMaster,
                               foundPeerPipeline,
                               pMultiReqSync->currReq.isMultiRequest);
                pCaptureRequest->peerSyncInfo.needSync = FALSE;
                result = CamxResultEFailed;
            }
        }
    }
    else
    {
        // Sanity check failure case
        CAMX_LOG_ERROR(CamxLogGroupStats, "Error parsing multi-req info: numOfActivePipeline:%d foundOwnPipeline:%d",
                       numOfActivePipeline, foundOwnPipeline);
        pCaptureRequest->peerSyncInfo.needSync = FALSE;
        result = CamxResultEFailed;
    }

    if (TRUE == pCaptureRequest->peerSyncInfo.needSync)
    {
        pCaptureRequest->peerSyncInfo.isMaster        = isMaster;
        pCaptureRequest->peerSyncInfo.peerPipelineID  = peerPipelineId;
        pCaptureRequest->peerSyncInfo.requestDelta    = static_cast<INT64>(requestID) - static_cast<INT64>(peerRequestID);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupStats,
                     "Type:%d result:%d NeedSync:%d Req:%llu pipeline:%d Master:%d peerReq:%llu peerPipeline:%d Delta:%lld",
                     m_multiStatsData.algoSyncType, result, pCaptureRequest->peerSyncInfo.needSync,
                     requestID, m_multiStatsData.pipelineId, isMaster, peerRequestID, peerPipelineId,
                     pCaptureRequest->peerSyncInfo.requestDelta);


    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SingletonStatsOperator::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SingletonStatsOperator::Initialize(
    const MultiStatsData* pMultiStatsData)
{
    CAMX_ASSERT(NULL != pMultiStatsData);

    m_multiStatsData.algoSyncType       = pMultiStatsData->algoSyncType;
    m_multiStatsData.algoRole           = pMultiStatsData->algoRole;
    m_multiStatsData.pipelineId         = pMultiStatsData->pipelineId;
    m_multiStatsData.pHwContext         = pMultiStatsData->pHwContext;
    m_multiStatsData.algoAction         = StatsAlgoProcessRequest;
    m_multiStatsData.isSlaveOperational = TRUE;
    m_multiStatsData.pNode              = pMultiStatsData->pNode;

    CAMX_ASSERT(StatsAlgoSyncType::StatsAlgoSyncTypeAEC == m_multiStatsData.algoSyncType ||
                StatsAlgoSyncType::StatsAlgoSyncTypeAF  == m_multiStatsData.algoSyncType ||
                StatsAlgoSyncType::StatsAlgoSyncTypeAWB == m_multiStatsData.algoSyncType);
    CAMX_ASSERT(StatsAlgoRole::StatsAlgoRoleDefault == m_multiStatsData.algoRole ||
                StatsAlgoRole::StatsAlgoRoleMaster  == m_multiStatsData.algoRole ||
                StatsAlgoRole::StatsAlgoRoleSlave   == m_multiStatsData.algoRole);
    CAMX_ASSERT(NULL != m_multiStatsData.pHwContext);
    CAMX_ASSERT(NULL != m_multiStatsData.pNode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SingletonStatsOperator::UpdateStatsDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SingletonStatsOperator::UpdateStatsDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsRequestData,
    UINT64                      requestIdOffsetFromLastFlush)
{
    CamxResult              result                  = CamxResultSuccess;
    UINT                    index                   = 0;
    MultiStatsDependency*   pMultiStatsDependency   = NULL;
    INT64                   lpmRequestDelta         = 0;
    UINT64                  requestId               = pStatsRequestData->requestId;
    NodeProcessRequestData* pNodeRequestData        = pExecuteProcessRequestData->pNodeProcessRequestData;
    DependencyUnit*         pStatsDependencies      = &(pNodeRequestData->dependencyInfo[0]);
    CaptureRequest*         pCaptureRequest         = pNodeRequestData->pCaptureRequest;

    CAMX_LOG_VERBOSE(CamxLogGroupStats,
                     "Update dependency for Node::%s Master:%d Req:%llu Pipeline:%d requestIdOffsetFromLastFlush %llu",
                     m_multiStatsData.pNode->NodeIdentifierString(),
                     (m_multiStatsData.algoRole == StatsAlgoRoleMaster) ? 1 : 0,
                     requestId,
                     m_multiStatsData.pipelineId,
                     requestIdOffsetFromLastFlush);

    result = ParseMultiRequestInfo(pExecuteProcessRequestData, pStatsRequestData, requestIdOffsetFromLastFlush);

    if (CamxResultSuccess == result)
    {
        StatsAlgoRole algoRole = (TRUE == pCaptureRequest->peerSyncInfo.isMaster) ? StatsAlgoRoleMaster : StatsAlgoRoleSlave;

        if (algoRole != m_multiStatsData.algoRole)
        {
            CAMX_LOG_VERBOSE(CamxLogGroupStats, "Role switched from %s to %s for Req:%llu",
                             (m_multiStatsData.algoRole == StatsAlgoRoleMaster) ? "Master" : "Slave",
                             (algoRole == StatsAlgoRoleMaster) ? "Master" : "Slave",
                             requestId);
            m_multiStatsData.algoRole = algoRole;
        }

        if (TRUE == pCaptureRequest->peerSyncInfo.needSync)
        {
            m_multiStatsData.peerPipelineId = pCaptureRequest->peerSyncInfo.peerPipelineID;
            lpmRequestDelta                 = pCaptureRequest->peerSyncInfo.requestDelta;
            pMultiStatsDependency           = NULL;

            for (index = 0; index < sizeof(singletonDependencyTable) / sizeof(MultiStatsDependency); index++)
            {
                if (singletonDependencyTable[index].algoRole == m_multiStatsData.algoRole)
                {
                    if (singletonDependencyTable[index].algoSyncType == m_multiStatsData.algoSyncType)
                    {
                        // Found matched item
                        pMultiStatsDependency = &(singletonDependencyTable[index]);
                        break;
                    }
                }
            }

            if (NULL != pMultiStatsDependency)
            {
                index = 0;
                while ((MaxCrossPipelineProperties > index) &&
                       (0 != pMultiStatsDependency->propertyPair[index].propertyIDs))
                {
                    result = AddDependency(&(pMultiStatsDependency->propertyPair[index]),
                                           pStatsDependencies,
                                           requestId,
                                           static_cast<UINT64>(abs(lpmRequestDelta)),
                                           (lpmRequestDelta < 0));
                    if (CamxResultSuccess == result)
                    {
                        index++;
                    }
                    else
                    {
                        CAMX_LOG_ERROR(CamxLogGroupStats, "Failed to add property %X to dependency",
                                       pMultiStatsDependency->propertyPair[index].propertyIDs);
                        break;
                    }
                }

                if (CamxResultSuccess == result)
                {
                    m_multiStatsData.algoAction = pMultiStatsDependency->algoAction;
                }
            }
            else
            {
                CAMX_LOG_ERROR(CamxLogGroupStats, "No match item found");
                result = CamxResultENoSuch;
            }
        }
    }
    else
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Parsing Multi Request SyncData Failed!!!");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SingletonStatsOperator::GetStatsAlgoAction
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsAlgoAction SingletonStatsOperator::GetStatsAlgoAction()
{
    return m_multiStatsData.algoAction;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SingletonStatsOperator::RemoveDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID SingletonStatsOperator::RemoveDependency(
    StatsDependency* pStatsDependency)
{
    CAMX_UNREFERENCED_PARAM(pStatsDependency);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SingletonStatsOperator::ParseMultiRequestInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult SingletonStatsOperator::ParseMultiRequestInfo(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsRequestData,
    UINT64                      requestIdOffsetFromLastFlush)
{
    CamxResult      result              = CamxResultEFailed;
    UINT            index               = 0;
    UINT            numOfActivePipeline = 0;
    BOOL            foundOwnPipeline    = FALSE;
    BOOL            foundPeerPipeline   = FALSE;
    UINT            peerPipelineId      = 0;
    BOOL            isMaster            = FALSE;
    CaptureRequest* pCaptureRequest     = pExecuteProcessRequestData->pNodeProcessRequestData->pCaptureRequest;

    if (NULL == pStatsRequestData)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Error: Multi request data pointer is NULL!");
        return CamxResultEFailed;
    }

    MultiRequestSyncData* pMultiReqSync     = pStatsRequestData->pMultiRequestSync;
    UINT64                requestID         = pStatsRequestData->requestId;
    UINT64                peerRequestID     = 0;

    for (index = 0; index < pMultiReqSync->numPipelines; index++)
    {
        if (TRUE == pMultiReqSync->currReq.isActive[index])
        {
            numOfActivePipeline++;

            if (m_multiStatsData.pipelineId == index)
            {
                foundOwnPipeline = TRUE;
                isMaster = pMultiReqSync->currReq.isMaster[index];
                pCaptureRequest->peerSyncInfo.isMaster = isMaster;
            }
            else
            {
                foundPeerPipeline = TRUE;
                peerPipelineId = index;
            }
        }
    }

    if ((numOfActivePipeline <= MaxActiveRealtimePipeline) && (numOfActivePipeline > 0) && (TRUE == foundOwnPipeline))
    {
        if (TRUE == isMaster)
        {
            // Current owning node is Master
            if (FirstValidRequestId != requestIdOffsetFromLastFlush)
            {
                // Generally, we need current Master depend on previous Slave if previous is a Multi request
                // Otherwise, we still want to depend on previous Master if previous is NOT a Multi request
                UINT    numOfPrevActivePipeline     = 0;
                BOOL    foundPrevSlave              = FALSE;
                BOOL    foundPrevMaster             = FALSE;
                UINT    prevSlavePipelineId         = 0;
                UINT    prevMasterPipelineId        = 0;

                for (index = 0; index < pMultiReqSync->numPipelines; index++)
                {
                    // Search in previous active pipeline that is not current own pipeline
                    if (TRUE == pMultiReqSync->prevReq.isActive[index])
                    {
                        numOfPrevActivePipeline++;
                        if (index != m_multiStatsData.pipelineId)
                        {
                            if (FALSE == pMultiReqSync->prevReq.isMaster[index])
                            {
                                foundPrevSlave       = TRUE;
                                prevSlavePipelineId  = index;
                            }
                            else
                            {
                                foundPrevMaster      = TRUE;
                                prevMasterPipelineId = index;
                            }
                        }
                    }
                }

                if ((numOfPrevActivePipeline <= MaxActiveRealtimePipeline) && (numOfPrevActivePipeline > 0))
                {
                    if (TRUE == foundPrevSlave)
                    {
                        peerPipelineId = prevSlavePipelineId;
                        peerRequestID = pMultiReqSync->prevReq.requestID[peerPipelineId];
                        pCaptureRequest->peerSyncInfo.needSync = TRUE;
                        result = CamxResultSuccess;
                    }
                    else if (TRUE == foundPrevMaster)
                    {
                        peerPipelineId = prevMasterPipelineId;
                        peerRequestID = pMultiReqSync->prevReq.requestID[peerPipelineId];
                        pCaptureRequest->peerSyncInfo.needSync = TRUE;
                        result = CamxResultSuccess;
                    }
                    else
                    {
                        // Sanity check has passed, but neither foundPrevSlave nor foundPrevMaster was found
                        // This happens when consecutive Single request were on same node, no need to sync for same pipeline
                        pCaptureRequest->peerSyncInfo.needSync = FALSE;
                        result = CamxResultSuccess;
                    }
                }
                else
                {
                    CAMX_LOG_ERROR(CamxLogGroupStats, "Error parsing Prev Multi-req info: NumPrev:%d",
                                   numOfPrevActivePipeline);
                    pCaptureRequest->peerSyncInfo.needSync = FALSE;
                    result = CamxResultEFailed;
                }
            }
            else
            {
                // We don't need to set any dependencies to Master with syncSequenceId = 1
                pCaptureRequest->peerSyncInfo.needSync = FALSE;
                result = CamxResultSuccess;
                CAMX_LOG_VERBOSE(CamxLogGroupStats, "No need to add dependency: offset From Last Flush:%llu Prev Multi:%d",
                                 requestIdOffsetFromLastFlush,
                                 pMultiReqSync->prevReq.isMultiRequest);
            }
        }
        else
        {
            // Current owning node is Slave
            if ((TRUE == foundPeerPipeline) && (TRUE == pMultiReqSync->currReq.isMultiRequest))
            {
                // Found a slave node here, implication is there must be a multi request ongoing,
                // and there must be a parallel master node in peer pipeline, this slave node will
                // therefore depend on current master.
                peerRequestID = pMultiReqSync->currReq.requestID[peerPipelineId];
                pCaptureRequest->peerSyncInfo.needSync = TRUE;
                result = CamxResultSuccess;
            }
            else
            {
                // Parsing error, either we failed to find peer pipeline(Master one), or current is not Multi request
                CAMX_LOG_ERROR(CamxLogGroupStats, "Error parsing Multi-req info: isMaster:%d foundPeerPipeline:%d isMulti:%d",
                               isMaster,
                               foundPeerPipeline,
                               pMultiReqSync->currReq.isMultiRequest);
                pCaptureRequest->peerSyncInfo.needSync = FALSE;
                result = CamxResultEFailed;
            }
        }
    }
    else
    {
        // Sanity check failure case
        CAMX_LOG_ERROR(CamxLogGroupStats, "Error parsing multi-req info: numOfActivePipeline:%d foundOwnPipeline:%d",
                       numOfActivePipeline, foundOwnPipeline);
        pCaptureRequest->peerSyncInfo.needSync = FALSE;
        result = CamxResultEFailed;
    }

    if (TRUE == pCaptureRequest->peerSyncInfo.needSync)
    {
        pCaptureRequest->peerSyncInfo.isMaster        = isMaster;
        pCaptureRequest->peerSyncInfo.peerPipelineID  = peerPipelineId;
        pCaptureRequest->peerSyncInfo.requestDelta    = static_cast<INT64>(requestID - peerRequestID);
    }

    CAMX_LOG_VERBOSE(CamxLogGroupStats,
                     "Type:%d result:%d NeedSync:%d Req:%llu pipeline:%d Master:%d peerReq:%llu peerPipeline:%d Delta:%lld",
                     m_multiStatsData.algoSyncType, result, pCaptureRequest->peerSyncInfo.needSync,
                     requestID, m_multiStatsData.pipelineId, isMaster, peerRequestID, peerPipelineId,
                     pCaptureRequest->peerSyncInfo.requestDelta);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NoSyncStatsOperator::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NoSyncStatsOperator::Initialize(
    const MultiStatsData* pMultiStatsData)
{
    CAMX_UNREFERENCED_PARAM(pMultiStatsData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NoSyncStatsOperator::UpdateStatsDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NoSyncStatsOperator::UpdateStatsDependencies(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsRequestData,
    UINT64                      requestIdOffsetFromLastFlush)
{
    CAMX_UNREFERENCED_PARAM(pExecuteProcessRequestData);
    CAMX_UNREFERENCED_PARAM(pStatsRequestData);
    CAMX_UNREFERENCED_PARAM(requestIdOffsetFromLastFlush);

    return CamxResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NoSyncStatsOperator::GetStatsAlgoAction
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsAlgoAction NoSyncStatsOperator::GetStatsAlgoAction()
{
    return StatsAlgoProcessRequest;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NoSyncStatsOperator::RemoveDependency
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID NoSyncStatsOperator::RemoveDependency(
    StatsDependency* pStatsDependency)
{
    CAMX_UNREFERENCED_PARAM(pStatsDependency);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NoSyncStatsOperator::ParseMultiRequestInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult NoSyncStatsOperator::ParseMultiRequestInfo(
    ExecuteProcessRequestData*  pExecuteProcessRequestData,
    StatsProcessRequestData*    pStatsRequestData,
    UINT64                      requestIdOffsetFromLastFlush)
{
    CAMX_UNREFERENCED_PARAM(pExecuteProcessRequestData);
    CAMX_UNREFERENCED_PARAM(pStatsRequestData);
    CAMX_UNREFERENCED_PARAM(requestIdOffsetFromLastFlush);

    return CamxResultSuccess;
}

CAMX_NAMESPACE_END
