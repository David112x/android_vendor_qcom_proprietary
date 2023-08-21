////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2016-2019 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  camxstatsprocessormanager.cpp
/// @brief Implements managing stats processors.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "camxmem.h"
#include "camxcaecstatsprocessor.h"
#include "camxcafdstatsprocessor.h"
#include "camxcafstatsprocessor.h"
#include "camxcasdstatsprocessor.h"
#include "camxsettingsmanager.h"
#include "camxstatsprocessormanager.h"

const CHAR* pStatsIdString[] = {"Sys", "AEC", "AFD", "ASD", "MaxID"};

CAMX_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessorManager::Create(
    StatsNodeCreateData*    pStatsCreateData,
    StatsProcessorManager** ppStatsProcessorManager)
{
    CamxResult             result                   = CamxResultSuccess;
    StatsProcessorManager* pStatsProcessorManager   = NULL;

    if ((NULL != ppStatsProcessorManager) && (NULL != pStatsCreateData))
    {
        pStatsProcessorManager = CAMX_NEW StatsProcessorManager;
        if (NULL != pStatsProcessorManager)
        {
            pStatsProcessorManager->m_statsCreateData = *pStatsCreateData;
            result = pStatsProcessorManager->CreateStatsProcessors();
            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupStats, "StatsProcessorManager::Create failed = %d", result);
                pStatsProcessorManager->Destroy();
                pStatsProcessorManager = NULL;
            }
        }
        else
        {
            CAMX_LOG_ERROR(CamxLogGroupStats, "StatsProcessingNode Create failed - out of memory");
            result = CamxResultENoMemory;
        }

        *ppStatsProcessorManager = pStatsProcessorManager;
    }

    CAMX_LOG_INFO(CamxLogGroupStats,
                  "StatsProcessingNode::Create result = %s",
                  (CamxResultSuccess == result) ? "Success" : "Fail");

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::StatsProcessorManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsProcessorManager::StatsProcessorManager()
    : m_pAECStatsProcessor(NULL)
    , m_pAFDStatsProcessor(NULL)
    , m_pASDStatsProcessor(NULL)
    , m_pStatsInitializeData(NULL)
{
    m_converganceHist = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::CreateStatsProcessors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessorManager::CreateStatsProcessors()
{
    CamxResult result = CamxResultSuccess;

    // Create all the stats processors.
    result = CAECStatsProcessor::Create(&m_pAECStatsProcessor);
    if (CamxResultSuccess == result)
    {
        result = CAFDStatsProcessor::Create(&m_pAFDStatsProcessor);

        if (CamxResultSuccess == result)
        {
            result = CASDStatsProcessor::Create(&m_pASDStatsProcessor);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessorManager::Initialize(
    const StatsInitializeData* pInitializeData)
{
    CamxResult result  = CamxResultSuccess;


    // Save the stats setting pointer.
    m_pStatsInitializeData  = pInitializeData;

    /// @todo  (CAMX-521): Register the remaining threads


    result = m_pAECStatsProcessor->Initialize(m_pStatsInitializeData);

    if (CamxResultSuccess == result)
    {

        result = m_pAFDStatsProcessor->Initialize(m_pStatsInitializeData);
    }

    if (CamxResultSuccess == result)
    {

        result = m_pASDStatsProcessor->Initialize(m_pStatsInitializeData);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::OnProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessorManager::OnProcessRequest(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo)
{
    // NOWHINE CP036a: Preserve current behavior, look into this in the future
    return StatsProcessorManagerJobHandler(const_cast<StatsProcessRequestData*>(pStatsProcessRequestDataInfo));
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::GetDependencies
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessorManager::GetDependencies(
    const StatsProcessRequestData* pStatsProcessRequestDataInfo,
    StatsDependency*               pStatsDependency
    ) const
{
    CamxResult result  = CamxResultSuccess;
    UINT32     errorID = 1;

    result = m_pAECStatsProcessor->GetDependencies(pStatsProcessRequestDataInfo, pStatsDependency);

    if (CamxResultSuccess == result)
    {
        errorID++;
        result = m_pAFDStatsProcessor->GetDependencies(pStatsProcessRequestDataInfo, pStatsDependency);
    }

    if (CamxResultSuccess == result)
    {
        errorID++;
        result = m_pASDStatsProcessor->GetDependencies(pStatsProcessRequestDataInfo, pStatsDependency);
    }
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Stats: %s GetDependencies failed with result= %d", pStatsIdString[errorID], result);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::SetStatsParseFuncPtr
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessorManager::SetStatsParseFuncPtr(
    VOID* pStatsParseFuncPtr)
{
    CamxResult result  = CamxResultSuccess;

    CAECStatsProcessor*    pAECStatsProcessor = static_cast<CAECStatsProcessor*>(m_pAECStatsProcessor);

    result = pAECStatsProcessor->SetStatsParseFuncPtr(pStatsParseFuncPtr);
    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Set Stats parse function pointer failed with result= %d", result);
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::SetVideoHDRInformation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessorManager::SetVideoHDRInformation(
    BOOL             isVideoHDREnabled,
    HDR3ExposureType HDR3ExposureType)
{
    CamxResult result  = CamxResultSuccess;

    CAECStatsProcessor*    pAECStatsProcessor = static_cast<CAECStatsProcessor*>(m_pAECStatsProcessor);

    result = pAECStatsProcessor->SetVideoHDRInformation(isVideoHDREnabled, HDR3ExposureType);

    if (CamxResultSuccess != result)
    {
        CAMX_LOG_ERROR(CamxLogGroupStats, "Set Video HDR Information failed with result= %d", result);
    }
    return result;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsProcessorManager::Destroy()
{
    CAMX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::~StatsProcessorManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
StatsProcessorManager::~StatsProcessorManager()
{
    // Destroy everything, if created.
    if (NULL != m_pAECStatsProcessor)
    {
        m_pAECStatsProcessor->Destroy();
        m_pAECStatsProcessor = NULL;
    }

    if (NULL != m_pAFDStatsProcessor)
    {
        m_pAFDStatsProcessor->Destroy();
        m_pAFDStatsProcessor = NULL;
    }

    if (NULL != m_pASDStatsProcessor)
    {
        m_pASDStatsProcessor->Destroy();
        m_pASDStatsProcessor = NULL;
    }

    m_pStatsInitializeData = NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::StatsProcessorManagerJobHandler
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessorManager::StatsProcessorManagerJobHandler(
    StatsProcessRequestData* pStatsProcessRequestInfo)
{
    CAMX_TRACE_SYNC_BEGIN(CamxLogGroupStats, "StatsProcessorManager::StatsProcessorManagerJobHandler");
    CamxResult               result                     = CamxResultSuccess;
    UINT32                   errorID                    = 1;

    if (NULL != pStatsProcessRequestInfo)
    {
        if (m_converganceHist > HwEnvironment::GetInstance()->GetStaticSettings()->kickTheMachine)
        {
            CAMX_LOG_ERROR(CamxLogGroupStats, "KICKED - %d requests since last converged", m_converganceHist);
            pStatsProcessRequestInfo->reportConverged = TRUE;
        }

        result = m_pAECStatsProcessor->ExecuteProcessRequest(pStatsProcessRequestInfo);

        if (CamxResultSuccess == result)
        {
            errorID++;
            result = m_pAFDStatsProcessor->ExecuteProcessRequest(pStatsProcessRequestInfo);
        }

        if (CamxResultSuccess == result)
        {
            errorID++;
            result = m_pASDStatsProcessor->ExecuteProcessRequest(pStatsProcessRequestInfo);
        }

        if (CamxResultSuccess != result)
        {
            CAMX_LOG_ERROR(CamxLogGroupStats,
                           "Stats: %s StatsProcessorManagerJobHandler failed with result= %d",
                           pStatsIdString[errorID],
                           result);
        }

        static const UINT ConverganceState[] =
        {
            ControlAWBState,
            ControlAFState,
            ControlAEState,
        };
        VOID*   pData[3]        = { 0 };
        UINT64  dataOffset[3]   = { 0 };
        UINT    length          = CAMX_ARRAY_SIZE(ConverganceState);

        m_statsCreateData.pNode->GetDataList(ConverganceState, pData, dataOffset, length);

        if ((NULL != pData[0]) && (NULL != pData[1]) && (NULL != pData[2]))
        {
            if (((AWBAlgoStateConverged == *(static_cast<AWBAlgoState*>(pData[0]))) ||
                 (ControlAWBStateLocked == *(static_cast<CamX::ControlAWBStateValues*>(pData[0])))   )                 &&
                ((ControlAFStatePassiveFocused == *(static_cast<ControlAFStateValues*>(pData[1]))) ||
                 (ControlAFStateFocusedLocked == *(static_cast<ControlAFStateValues*>(pData[1]))))      &&
                ((ControlAEStateConverged == *(static_cast<ControlAEStateValues*>(pData[2]))) ||
                 (ControlAEStateLocked == *(static_cast<ControlAEStateValues*>(pData[2]))))               )
            {
                m_converganceHist = 0;
            }
            else
            {
                m_converganceHist++;
            }
        }

        // Send ProcessRequestDone even in failure cases to make sure resources are freed
        /// @todo (CAMX-1203): Adapt for multi-camera.
        m_statsCreateData.pStatsNotifier->NotifyJobProcessRequestDone(pStatsProcessRequestInfo->requestId);

    }
    else
    {
        result = CamxResultEInvalidArg;
    }
    CAMX_TRACE_SYNC_END(CamxLogGroupStats);

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::StatsProcessorManagerThreadFlushCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID StatsProcessorManager::StatsProcessorManagerThreadFlushCb(
    VOID* pUserData)
{
    CAMX_UNREFERENCED_PARAM(pUserData);

    /// @todo  (CAMX-521): Fill-in details.
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// StatsProcessorManager::GetPublishList
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamxResult StatsProcessorManager::GetPublishList(
    const UINT32    maxTagArraySize,
    UINT32*         pTagArray,
    UINT32*         pTagCount,
    UINT32*         pPartialTagCount)
{
    CamxResult  result   = CamxResultSuccess;
    UINT32      maxCount = maxTagArraySize;

    IStatsProcessor* pStatsProcessors[] = { m_pAECStatsProcessor , m_pAFDStatsProcessor , m_pASDStatsProcessor };

    *pTagCount = 0;

    for (UINT32 index = 0; index < CAMX_ARRAY_SIZE(pStatsProcessors); ++index)
    {
        if (NULL != pStatsProcessors[index])
        {
            UINT32 count           = 0;
            UINT32 partialTagCount = 0;

            result = pStatsProcessors[index]->GetPublishList(maxCount,
                        pTagArray,
                        &count,
                        &partialTagCount);

            if (CamxResultSuccess != result)
            {
                CAMX_LOG_ERROR(CamxLogGroupMeta, "Stats processor index %u not initilized", index);
                break;
            }

            *pTagCount = *pTagCount + count;
            maxCount   =  maxCount  - count;
            pTagArray  =  pTagArray + count;

            ///< We are accounting only for Partial tags from AECStatsProcessor
            if (0 == index)
            {
                *pPartialTagCount = partialTagCount;
            }
        }
    }

    return result;
}

CAMX_NAMESPACE_END
