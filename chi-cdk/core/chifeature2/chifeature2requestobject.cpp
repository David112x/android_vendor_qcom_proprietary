////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2requestobject.cpp
/// @brief Definitions for CHI feature request object class
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chibinarylog.h"
#include "chifeature2requestobject.h"
#include "chifeature2base.h"

/// @brief Enable  Global FID to Local FID mapping.
static const BOOL EnableFIDMapping = TRUE;

/// @brief Invalid Process Sequence Number.
static const INT InvalidProcessSequenceNumber = -1;

/// @brief ChiFeature2RequestObjectOpsType translation for map.
static const INT OpType0    = 0;
static const INT OpType1    = 1;
static const INT OpType2    = 2;

/// @brief Max Session that is possible for translation map.
static const UINT8  MaxSessionCount     = 8;

/// @brief Max Port that is possible for translation map.
static const UINT8  MaxPortCount        = 32;

/// @brief Max Pipeline that is possible for translation map.
static const UINT8  MaxPipelineCount    = 20;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RequestObject* ChiFeature2RequestObject::Create(
    const ChiFeature2RequestObjectCreateInputInfo* pCreateInputInfo)
{
    ChiFeature2RequestObject* pFeatureRequestObject = NULL;

    if (NULL != pCreateInputInfo)
    {
        pFeatureRequestObject = CHX_NEW ChiFeature2RequestObject;

        if (NULL != pFeatureRequestObject)
        {
            if (CDKResultSuccess != pFeatureRequestObject->InitializeRequestInfo(pCreateInputInfo))
            {
                pFeatureRequestObject->Destroy();
                pFeatureRequestObject = NULL;
            }
        }
    }

    return pFeatureRequestObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::InitializeRequestInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::InitializeRequestInfo(
    const ChiFeature2RequestObjectCreateInputInfo* pCreateInputInfo)
{
    CDKResult result = CDKResultSuccess;

    m_pUsecaseRequestObj                    = pCreateInputInfo->pUsecaseRequestObj;
    m_pGraphPrivateData                     = pCreateInputInfo->pGraphPrivateData;
    m_pFeatureBase                          = pCreateInputInfo->pFeatureBase;
    m_numRequests                           = pCreateInputInfo->numRequests;
    m_usecaseMode                           = pCreateInputInfo->usecaseMode;
    m_pBatchOutputRequestInfo               = NULL;
    m_curRequestId                          = 0;
    m_outputPortsProcessingCompleteNotified = FALSE;
    m_pMutex                                = Mutex::Create();

    CdkUtils::SNPrintF(m_identifierString, sizeof(m_identifierString), "FRO-URO:%d_%s:%s_%d",
                       m_pUsecaseRequestObj->GetAppFrameNumber(),
                       pCreateInputInfo->pGraphName,
                       m_pFeatureBase->GetFeatureName(),
                       m_pFeatureBase->GetInstanceProps()->instanceId);

    if ((0 < m_numRequests) && (NULL != pCreateInputInfo->pRequestTable))
    {
        m_pBatchOutputRequestInfo = static_cast<ChiFeature2BatchOutputRequestInfo*>
            (CHX_CALLOC(sizeof(ChiFeature2BatchOutputRequestInfo) * m_numRequests));
        if (NULL != m_pBatchOutputRequestInfo)
        {
            for (UINT8 requestIndex = 0; requestIndex < m_numRequests; requestIndex++)
            {
                m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo = NULL;
                m_pBatchOutputRequestInfo[requestIndex].pRequestInfo      =
                    static_cast<ChiFeature2RequestInfo*>(CHX_CALLOC(sizeof(ChiFeature2RequestInfo)));
                if (NULL != m_pBatchOutputRequestInfo[requestIndex].pRequestInfo)
                {
                    ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
                    pRequestInfo->nextProcessSequenceId     = InvalidProcessSequenceId;
                    pRequestInfo->curProcessSequenceId      = InvalidProcessSequenceId;
                    m_maxInfo.maxSequence                   = InvalidProcessSequenceNumber;
                    pRequestInfo->numRequestOutputs         = pCreateInputInfo->pRequestTable[requestIndex].numRequestOutputs;
                    pRequestInfo->numOutputNotified         = 0;
                    pRequestInfo->numOutputReleased         = 0;
                    pRequestInfo->pRequestOutput            = static_cast<ChiFeature2RequestOutputInfo*>(
                        CHX_CALLOC(sizeof(ChiFeature2RequestOutputInfo) * pRequestInfo->numRequestOutputs));
                    pRequestInfo->pResultMutex              = Mutex::Create();
                    pRequestInfo->pResultAvailable          = Condition::Create();
                    pRequestInfo->pDependencyMutex          = Mutex::Create();

                    if (NULL != pRequestInfo->pRequestOutput)
                    {
                        pRequestInfo->requestIndex          = pCreateInputInfo->pRequestTable[requestIndex].requestIndex;

                        for (UINT32 portDescIndex = 0; portDescIndex < pRequestInfo->numRequestOutputs; portDescIndex++)
                        {
                            ChiFeature2RequestOutputInfo outputInfo =
                                pCreateInputInfo->pRequestTable[requestIndex].pRequestOutputs[portDescIndex];
                            pRequestInfo->pRequestOutput[portDescIndex] = outputInfo;

                            if (TRUE == IsPortUnique(outputInfo.pPortDescriptor->globalId.port))
                            {
                                ChiFeature2OutputPortProcessingInfo portProcessingInfo =
                                { outputInfo.pPortDescriptor->globalId.port, FALSE };
                                m_outputPortProcessingInfo.push_back(portProcessingInfo);
                            }
                        }
                    }
                    else
                    {
                        CHX_LOG_ERROR("No memory allocated for FRO Port Descriptor Info");
                        result = CDKResultENoMemory;
                        break;
                    }

                    pRequestInfo->pOutputNotified = static_cast<BOOL*>(
                        CHX_CALLOC(sizeof(BOOL) * pRequestInfo->numRequestOutputs));
                    if (NULL == pRequestInfo->pOutputNotified)
                    {
                        CHX_LOG_ERROR("No memory allocated for FRO OutputNotified");
                        result = CDKResultENoMemory;
                        break;
                    }

                    pRequestInfo->pReleaseAcked = static_cast<BOOL*>(
                        CHX_CALLOC(sizeof(BOOL) * pRequestInfo->numRequestOutputs));
                    if (NULL == pRequestInfo->pReleaseAcked)
                    {
                        CHX_LOG_ERROR("No memory allocated for FRO ReleaseAcked data");
                        result = CDKResultENoMemory;
                        break;
                    }

                    pRequestInfo->pOutputGenerated = static_cast<BOOL*>(
                        CHX_CALLOC(sizeof(BOOL) * pRequestInfo->numRequestOutputs));
                    if (NULL == pRequestInfo->pOutputGenerated)
                    {
                        CHX_LOG_ERROR("No memory allocated for FRO OutputGenerated");
                        result = CDKResultENoMemory;
                        break;
                    }

                    result = SetCurRequestState(ChiFeature2RequestState::Initialized, requestIndex);

                    if (CDKResultSuccess == result)
                    {
                        m_hFroHandle = reinterpret_cast<CHIFEATUREREQUESTOBJECTHANDLE>(this);
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Error in Creating FRO's Request Info");
                    result = CDKResultENoMemory;
                }
            }
        }
        else
        {
            CHX_LOG_ERROR("No memory allocated for Batch Output Request Info");
            result = CDKResultENoMemory;
        }
    }
    else
    {
        CHX_LOG_ERROR("Number of requests is %d or pRequestTable invalid %p", m_numRequests, pCreateInputInfo->pRequestTable);
        result = CDKResultENoMemory;
    }

    m_pHint = pCreateInputInfo->pFeatureHint;

    if (CDKResultSuccess == result)
    {
        UINT                          frameNumber       = m_pUsecaseRequestObj->GetAppFrameNumber();
        UINT32                        featureId         = m_pFeatureBase->GetFeatureId();
        UINT32                        featureInstanceId = m_pFeatureBase->GetInstanceProps()->instanceId;
        auto                          hFroHandle        = m_hFroHandle;
        UINT8                         numRequests       = m_numRequests;
        BINARY_LOG(LogEvent::FT2_FRO_Init, frameNumber, featureId, featureInstanceId, hFroHandle, numRequests);
        CHX_LOG_INFO("%s: Created for numRequests:%d", IdentifierString(), numRequests);
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RequestObject::Destroy()
{
    m_hFroHandle = NULL;

    if (NULL != m_pBatchOutputRequestInfo)
    {
        for (UINT8 requestIndex = 0; requestIndex < m_numRequests; requestIndex++)
        {
            ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
            if (NULL != pRequestInfo)
            {
                SetCurRequestState(ChiFeature2RequestState::InvalidMax, requestIndex);
                if (InvalidProcessSequenceId != pRequestInfo->curProcessSequenceId)
                {
                    UINT8 numDependencyConfig   = 0;
                    UINT8 numPipelines          = 0;
                    UINT8 numPorts              = 0;
                    for (INT32 seqId = 0; seqId <= pRequestInfo->curProcessSequenceId; seqId++)
                    {
                        numDependencyConfig = pRequestInfo->ppProcessSequenceInfo[seqId]->numDependencyConfig;
                        if (NULL != pRequestInfo->ppProcessSequenceInfo[seqId]->pMap)
                        {
                            DeleteFeatureIdentifierMap(pRequestInfo->ppProcessSequenceInfo[seqId]->pMap);
                        }
                        if (NULL != pRequestInfo->ppProcessSequenceInfo[seqId]->pSequencePrivData)
                        {
                            ChiFeature2Base::ChiFeatureSequenceData* pData =
                                static_cast<ChiFeature2Base::ChiFeatureSequenceData*>
                                (pRequestInfo->ppProcessSequenceInfo[seqId]->pSequencePrivData);

                            pData->frameCbData.pInputPorts.clear();
                            pData->frameCbData.pInputPorts.shrink_to_fit();
                            pData->frameCbData.pOutputPorts.clear();
                            pData->frameCbData.pOutputPorts.shrink_to_fit();

                            if (NULL != pData)
                            {
                                CHX_DELETE(pData);
                                pData = NULL;
                                pRequestInfo->ppProcessSequenceInfo[seqId]->pSequencePrivData = NULL;
                            }
                        }
                        for (INT32 sessionIndex = 0; sessionIndex < numDependencyConfig; sessionIndex++)
                        {
                            numPipelines =
                                pRequestInfo->ppProcessSequenceInfo[seqId]->pDependencyConfig[sessionIndex].numPipelines;
                            for (INT32 pipelineIndex = 0; pipelineIndex < numPipelines; pipelineIndex++)
                            {
                                numPorts = pRequestInfo->ppProcessSequenceInfo[seqId]->
                                    pDependencyConfig[sessionIndex].pPipelineInfo[pipelineIndex].numHandles;
                                if (ChiFeature2HandleType::DependencyConfigInfo ==
                                    pRequestInfo->ppProcessSequenceInfo[seqId]->
                                    pDependencyConfig[sessionIndex].pPipelineInfo[pipelineIndex].type)
                                {
                                    ChiFeature2DependencyConfigInfo* pConfigInfo =
                                        static_cast<ChiFeature2DependencyConfigInfo*>
                                        (pRequestInfo->ppProcessSequenceInfo[seqId]->
                                            pDependencyConfig[sessionIndex].pPipelineInfo[pipelineIndex].handle);
                                    if (NULL != pConfigInfo)
                                    {
                                        if (0 != pConfigInfo->inputConfig.num)
                                        {
                                            if (NULL != pConfigInfo->inputConfig.pPortDescriptor)
                                            {
                                                // NOWHINE CP036a:
                                                CHX_FREE(const_cast<ChiFeature2PortDescriptor*>
                                                    (pConfigInfo->inputConfig.pPortDescriptor));
                                                pConfigInfo->inputConfig.pPortDescriptor = NULL;
                                            }
                                            if (NULL != pConfigInfo->inputConfig.phTargetBufferHandle)
                                            {
                                                CHX_FREE(pConfigInfo->inputConfig.phTargetBufferHandle);
                                                pConfigInfo->inputConfig.phTargetBufferHandle = NULL;
                                            }
                                            if (NULL != pConfigInfo->inputConfig.pBufferErrorPresent)
                                            {
                                                CHX_FREE(pConfigInfo->inputConfig.pBufferErrorPresent);
                                                pConfigInfo->inputConfig.pBufferErrorPresent = NULL;
                                            }
                                            if (NULL != pConfigInfo->inputConfig.pKey)
                                            {
                                                CHX_FREE(pConfigInfo->inputConfig.pKey);
                                                pConfigInfo->inputConfig.pKey = NULL;
                                            }
                                            if (NULL != pConfigInfo->inputConfig.phMetadata)
                                            {
                                                CHX_FREE(pConfigInfo->inputConfig.phMetadata);
                                                pConfigInfo->inputConfig.pKey = NULL;
                                            }

                                            pConfigInfo->inputConfig.num = 0;
                                        }
                                        if (0 != pConfigInfo->outputConfig.num)
                                        {
                                            if (NULL != pConfigInfo->outputConfig.pPortDescriptor)
                                            {
                                                // NOWHINE CP036a:
                                                CHX_FREE(const_cast<ChiFeature2PortDescriptor*>
                                                    (pConfigInfo->outputConfig.pPortDescriptor));
                                                pConfigInfo->outputConfig.pPortDescriptor = NULL;
                                            }
                                            if (NULL != pConfigInfo->outputConfig.phTargetBufferHandle)
                                            {
                                                CHX_FREE(pConfigInfo->outputConfig.phTargetBufferHandle);
                                                pConfigInfo->outputConfig.phTargetBufferHandle = NULL;
                                            }
                                            if (NULL != pConfigInfo->outputConfig.pBufferErrorPresent)
                                            {
                                                CHX_FREE(pConfigInfo->outputConfig.pBufferErrorPresent);
                                                pConfigInfo->outputConfig.pBufferErrorPresent = NULL;
                                            }
                                            if (NULL != pConfigInfo->outputConfig.pKey)
                                            {
                                                CHX_FREE(pConfigInfo->outputConfig.pKey);
                                                pConfigInfo->outputConfig.pKey = NULL;
                                            }
                                            if (NULL != pConfigInfo->outputConfig.phMetadata)
                                            {
                                                CHX_FREE(pConfigInfo->outputConfig.phMetadata);
                                                pConfigInfo->outputConfig.pKey = NULL;
                                            }

                                            pConfigInfo->outputConfig.num = 0;
                                        }
                                        if (0 != pConfigInfo->numInputDependency)
                                        {
                                            for (INT32 dependencyIndex = 0; dependencyIndex < pConfigInfo->numInputDependency;
                                                dependencyIndex++)
                                            {
                                                if ((NULL != pConfigInfo->pInputDependency) &&
                                                    (0 != pConfigInfo->pInputDependency[dependencyIndex].num))
                                                {
                                                    if (NULL != pConfigInfo->pInputDependency[dependencyIndex].pPortDescriptor)
                                                    {
                                                        // NOWHINE CP036a:
                                                        CHX_FREE(const_cast<ChiFeature2PortDescriptor*>
                                                            (pConfigInfo->pInputDependency[dependencyIndex].pPortDescriptor));
                                                        pConfigInfo->pInputDependency[dependencyIndex].pPortDescriptor = NULL;
                                                    }
                                                    if (NULL !=
                                                        pConfigInfo->pInputDependency[dependencyIndex].phTargetBufferHandle)
                                                    {
                                                        CHX_FREE(pConfigInfo->pInputDependency[dependencyIndex].
                                                            phTargetBufferHandle);
                                                        pConfigInfo->pInputDependency[dependencyIndex].phTargetBufferHandle =
                                                            NULL;
                                                    }
                                                    if (NULL !=
                                                        pConfigInfo->pInputDependency[dependencyIndex].pBufferErrorPresent)
                                                    {
                                                        CHX_FREE(pConfigInfo->pInputDependency[dependencyIndex].
                                                                 pBufferErrorPresent);
                                                        pConfigInfo->pInputDependency[dependencyIndex].pBufferErrorPresent =
                                                            NULL;
                                                    }
                                                    if (NULL !=
                                                        pConfigInfo->pInputDependency[dependencyIndex].pPortBufferStatus)
                                                    {
                                                        CHX_FREE(pConfigInfo->pInputDependency[dependencyIndex].
                                                                 pPortBufferStatus);
                                                        pConfigInfo->pInputDependency[dependencyIndex].pPortBufferStatus =
                                                            NULL;
                                                    }
                                                    if (NULL != pConfigInfo->pInputDependency[dependencyIndex].pKey)
                                                    {
                                                        CHX_FREE(pConfigInfo->pInputDependency[dependencyIndex].pKey);
                                                        pConfigInfo->pInputDependency[dependencyIndex].pKey = NULL;
                                                    }
                                                    if (NULL != pConfigInfo->pInputDependency[dependencyIndex].phMetadata)
                                                    {
                                                        for (UINT8 portIndex = 0; portIndex < pConfigInfo->
                                                            pInputDependency[dependencyIndex].num;
                                                                portIndex++)
                                                        {
                                                            if ((NULL != pConfigInfo->
                                                                pInputDependency[dependencyIndex].phMetadata[portIndex]) &&
                                                                    (NULL != m_pFeatureBase))
                                                            {
                                                                CHIMETAHANDLE  hSetting = pConfigInfo->
                                                                    pInputDependency[dependencyIndex].phMetadata[portIndex];
                                                                if (NULL == hSetting)
                                                                {
                                                                    continue;
                                                                }
                                                                ChiMetadata* pFeatureSettings = m_pFeatureBase->
                                                                    GetMetadataFromHandle(hSetting);
                                                                if (NULL != pFeatureSettings)
                                                                {
                                                                    pFeatureSettings->Destroy(TRUE);
                                                                }
                                                                pConfigInfo->pInputDependency[dependencyIndex].
                                                                    phMetadata[portIndex]=NULL;
                                                            }
                                                        }
                                                        CHX_FREE(pConfigInfo->pInputDependency[dependencyIndex].phMetadata);
                                                        pConfigInfo->pInputDependency[dependencyIndex].phMetadata = NULL;
                                                    }

                                                    if (NULL != pConfigInfo->pRequestInput[dependencyIndex].phMetadata)
                                                    {
                                                        CHX_FREE(pConfigInfo->pRequestInput[dependencyIndex].phMetadata);
                                                        pConfigInfo->pRequestInput[dependencyIndex].phMetadata = NULL;
                                                    }
                                                    pConfigInfo->pInputDependency[dependencyIndex].num = 0;
                                                }
                                            }
                                            pConfigInfo->numInputDependency = 0;
                                            if (NULL != pConfigInfo->pInputDependency)
                                            {
                                                CHX_FREE(pConfigInfo->pInputDependency);
                                                pConfigInfo->pInputDependency = NULL;
                                            }
                                            if (NULL != pConfigInfo->pRequestInput)
                                            {
                                                CHX_FREE(pConfigInfo->pRequestInput);
                                                pConfigInfo->pRequestInput = NULL;
                                            }
                                        }
                                        CHX_FREE(pConfigInfo);
                                        pConfigInfo = NULL;
                                    }
                                }
                            }
                        }

                        ChiFeature2SessionInfo* pDependencyConfig =
                         pRequestInfo->ppProcessSequenceInfo[seqId]->pDependencyConfig;

                        if (NULL != pDependencyConfig)
                        {
                            if (NULL != pDependencyConfig->pPipelineInfo)
                            {
                                CHX_FREE(pDependencyConfig->pPipelineInfo);
                                pDependencyConfig->pPipelineInfo = NULL;
                            }

                            if (NULL != pDependencyConfig)
                            {
                                CHX_FREE(pDependencyConfig);
                                pDependencyConfig = NULL;
                            }
                        }
                    }
                }

                if (NULL != pRequestInfo->ppProcessSequenceInfo)
                {
                    CHX_FREE(pRequestInfo->ppProcessSequenceInfo);
                    pRequestInfo->ppProcessSequenceInfo = NULL;
                }
                if (NULL != pRequestInfo->pOutputNotified)
                {
                    CHX_FREE(pRequestInfo->pOutputNotified);
                    pRequestInfo->pOutputNotified = NULL;
                }
                if (NULL != pRequestInfo->pReleaseAcked)
                {
                    CHX_FREE(pRequestInfo->pReleaseAcked);
                    pRequestInfo->pReleaseAcked = NULL;
                }
                if (NULL != pRequestInfo->pOutputGenerated)
                {
                    CHX_FREE(pRequestInfo->pOutputGenerated);
                    pRequestInfo->pOutputGenerated = NULL;
                }
                if (NULL != pRequestInfo->pRequestOutput)
                {
                    CHX_FREE(pRequestInfo->pRequestOutput);
                    pRequestInfo->pRequestOutput = NULL;
                }
                if (NULL != pRequestInfo->pResultMutex)
                {
                    pRequestInfo->pResultMutex->Destroy();
                    pRequestInfo->pResultMutex = NULL;
                }
                if (NULL != pRequestInfo->pResultAvailable)
                {
                    pRequestInfo->pResultAvailable->Destroy();
                    pRequestInfo->pResultAvailable = NULL;
                }
                if (NULL != pRequestInfo->pDependencyMutex)
                {
                    pRequestInfo->pDependencyMutex->Destroy();
                    pRequestInfo->pDependencyMutex = NULL;
                }

                CHX_FREE(pRequestInfo);
                pRequestInfo = NULL;
            }
            if (NULL != m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo)
            {
                CHX_FREE(m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo);
                m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo = NULL;
            }
        }
        CHX_FREE(m_pBatchOutputRequestInfo);
        m_pBatchOutputRequestInfo = NULL;
    }

    if (NULL != m_pMutex)
    {
        m_pMutex->Destroy();
        m_pMutex = NULL;
    }
    CHX_LOG_INFO("%s: Destroyed", IdentifierString());
    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetCurRequestState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetCurRequestState(
    ChiFeature2RequestState requestState,
    UINT8                   requestIndex)
{

    CDKResult   result                      = CDKResultSuccess;
    BOOL        isRequestInfoVaild          = FALSE;


    isRequestInfoVaild = (NULL != m_pBatchOutputRequestInfo) &&
                         (requestIndex < m_numRequests) &&
                         (NULL != m_pBatchOutputRequestInfo[requestIndex].pRequestInfo);

    if (TRUE == isRequestInfoVaild)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        pRequestInfo->pDependencyMutex->Lock();

        if (requestState != pRequestInfo->state)
        {
            UINT32                        frameNumber       = m_pUsecaseRequestObj->GetAppFrameNumber();
            auto                          hFroHandle        = m_hFroHandle;
            ChiFeature2RequestState       prevState         = pRequestInfo->state;
            BOOL                          isValidTransition = ChiFeature2RequestStateTable[static_cast<UINT>
                                                            (pRequestInfo->state)][static_cast<UINT>(requestState)];
            BINARY_LOG(LogEvent::FT2_FRO_StateInfo, frameNumber, hFroHandle, prevState,
                requestState, requestIndex, isValidTransition);
            if (TRUE == isValidTransition)
            {
                if (ChiFeature2RequestState::InvalidMax != requestState)
                {
                    CHX_LOG_INFO("%s: FeatureRequest State [%s]->[%s] for RequestIndex:%d",
                                 IdentifierString(),
                                 ChiFeature2RequestStateStrings[static_cast<UINT>(pRequestInfo->state)],
                                 ChiFeature2RequestStateStrings[static_cast<UINT>(requestState)],
                                 requestIndex);
                }
                pRequestInfo->state = requestState;
            }
            else
            {
                if (ChiFeature2RequestState::Complete == pRequestInfo->state)
                {
                    CHX_LOG_INFO("%s: FeatureRequest State [%s]->[%s] is not permitted  for RequestIndex:%d",
                        IdentifierString(),
                        ChiFeature2RequestStateStrings[static_cast<UINT>(pRequestInfo->state)],
                        ChiFeature2RequestStateStrings[static_cast<UINT>(requestState)],
                        requestIndex);
                }
                else
                {
                    CHX_LOG_ERROR("%s: FeatureRequest State [%s]->[%s] is not permitted  for RequestIndex:%d",
                        IdentifierString(),
                        ChiFeature2RequestStateStrings[static_cast<UINT>(pRequestInfo->state)],
                        ChiFeature2RequestStateStrings[static_cast<UINT>(requestState)],
                        requestIndex);
                }
                result = CDKResultEInvalidState;
            }
        }
        pRequestInfo->pDependencyMutex->Unlock();
    }
    else
    {
        CHX_LOG_ERROR("RequestInfo invaild, Request index:%d numRequest %d",
            requestIndex,
            m_numRequests);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetCurRequestState
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2RequestState ChiFeature2RequestObject::GetCurRequestState(
    UINT8   requestIndex
    ) const
{
    ChiFeature2RequestState state = ChiFeature2RequestState::InvalidMax;

    if (requestIndex < m_numRequests)
    {
        state = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo->state;
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }
    return state;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetPrivContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetPrivContext(
    VOID* pClientData)
{
    CDKResult result = CDKResultSuccess;

    m_pFeaturePrivateData = pClientData;

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetPrivContext
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2RequestObject::GetPrivContext() const
{
    return m_pFeaturePrivateData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetSequencePrivData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetSequencePrivData(
    ChiFeature2SequenceOrder    sequenceOrder,
    VOID*                       pData,
    UINT8                       requestIndex)
{
    CDKResult result = CDKResultSuccess;
    UINT8     num    = 0;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        INT32                   sequenceID      = pRequestInfo->curProcessSequenceId;

        if (ChiFeature2SequenceOrder::Next == sequenceOrder)
        {
            sequenceID = pRequestInfo->nextProcessSequenceId;
        }

        if (InvalidProcessSequenceId != sequenceID)
        {
            pRequestInfo->ppProcessSequenceInfo[sequenceID]->pSequencePrivData = pData;
        }
    }
    else
    {
        result = CDKResultEInvalidArg;
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetSequencePrivData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2RequestObject::GetSequencePrivData(
    ChiFeature2SequenceOrder    sequenceOrder,
    UINT8                       requestIndex
    ) const

{
    UINT8 num       = 0;
    VOID* pData     = NULL;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        INT32                   sequenceID      = pRequestInfo->curProcessSequenceId;

        if (ChiFeature2SequenceOrder::Next == sequenceOrder)
        {
            sequenceID = pRequestInfo->nextProcessSequenceId;
        }

        if (InvalidProcessSequenceId != sequenceID)
        {
            pData = pRequestInfo->ppProcessSequenceInfo[sequenceID]->pSequencePrivData;
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }
    return pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetSequencePrivDataById
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetSequencePrivDataById(
    INT32   sequenceId,
    VOID*   pData,
    UINT8   requestIndex)

{
    CDKResult result = CDKResultSuccess;
    UINT8     num    = 0;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;

        if (InvalidProcessSequenceId != sequenceId)
        {
            pRequestInfo->ppProcessSequenceInfo[sequenceId]->pSequencePrivData = pData;
        }
    }
    else
    {
        result = CDKResultEInvalidArg;
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetSequencePrivDataById
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2RequestObject::GetSequencePrivDataById(
    INT32   sequenceId,
    UINT8   requestIndex
    ) const
{
    UINT8 num       = 0;
    VOID* pData     = NULL;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;

        if (InvalidProcessSequenceId != sequenceId)
        {
            pData = pRequestInfo->ppProcessSequenceInfo[sequenceId]->pSequencePrivData;
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }
    return pData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RequestObject::GetHandle
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CHIFEATUREREQUESTOBJECTHANDLE ChiFeature2RequestObject::GetHandle() const
{
    return m_hFroHandle;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetCurrentStageInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::GetCurrentStageInfo(
    ChiFeature2StageInfo* pStageInfo,
    UINT8                 requestIndex
    ) const
{
    CDKResult   result  = CDKResultSuccess;
    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        INT32                   sequenceID      = pRequestInfo->curProcessSequenceId;

        if (InvalidProcessSequenceId != sequenceID)
        {
            pStageInfo->stageId         = pRequestInfo->ppProcessSequenceInfo[sequenceID]->stageId;
            pStageInfo->stageSequenceId = pRequestInfo->ppProcessSequenceInfo[sequenceID]->stageSequenceId;
        }
        else
        {
            pStageInfo->stageId         = InvalidStageId;
            pStageInfo->stageSequenceId = InvalidStageSequenceId;
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetNextStageInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::GetNextStageInfo(
    ChiFeature2StageInfo* pStageInfo,
    UINT8                 requestIndex
    ) const
{
    CDKResult   result  = CDKResultSuccess;
    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        INT32                   sequenceID      = pRequestInfo->nextProcessSequenceId;

        if (InvalidProcessSequenceId != sequenceID)
        {
            pStageInfo->stageId         = pRequestInfo->ppProcessSequenceInfo[sequenceID]->stageId;
            pStageInfo->stageSequenceId = pRequestInfo->ppProcessSequenceInfo[sequenceID]->stageSequenceId;
        }
        else
        {
            pStageInfo->stageId         = InvalidStageId;
            pStageInfo->stageSequenceId = InvalidStageSequenceId;
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetUsecaseRequestObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2UsecaseRequestObject* ChiFeature2RequestObject::GetUsecaseRequestObject() const
{
    return m_pUsecaseRequestObj;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetGraphPrivateData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::GetGraphPrivateData(
    VOID** ppGraphPrivateData
    ) const
{
    *ppGraphPrivateData = m_pGraphPrivateData;
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetExternalRequestOutput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::GetExternalRequestOutput(
    UINT8*                                  pNumRequestOutputInfos,
    const ChiFeature2RequestOutputInfo**    ppRequestOutputInfo,
    UINT8                                   requestIndex
    ) const
{

    CDKResult result = CDKResultSuccess;
    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        *pNumRequestOutputInfos                 = pRequestInfo->numRequestOutputs;
        *ppRequestOutputInfo                    = pRequestInfo->pRequestOutput;
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetProcessSequenceId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
INT32  ChiFeature2RequestObject::GetProcessSequenceId(
    ChiFeature2SequenceOrder order,
    UINT8                    requestIndex
    ) const
{

    INT32 processSequenceId = InvalidProcessSequenceId;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        if (ChiFeature2SequenceOrder::Current == order)
        {
            processSequenceId = pRequestInfo->curProcessSequenceId;
        }
        else
        {
            processSequenceId = pRequestInfo->nextProcessSequenceId;
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }
    return processSequenceId;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::MoveToNextProcessSequenceInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::MoveToNextProcessSequenceInfo(
    UINT8   requestIndex )
{
    CDKResult result = CDKResultSuccess;
    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        if (InvalidProcessSequenceId != pRequestInfo->nextProcessSequenceId)
        {
            pRequestInfo->curProcessSequenceId    = pRequestInfo->nextProcessSequenceId;
            pRequestInfo->nextProcessSequenceId   = InvalidProcessSequenceId;
            CHX_LOG_INFO("%s Move to NextProcess SequenceID:%d BatchIndex:%d",
                IdentifierString(),
                pRequestInfo->curProcessSequenceId,
                requestIndex);
        }
        else
        {
            result = CDKResultENoSuch;
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetDependencyConfigBySequenceId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 ChiFeature2RequestObject::GetDependencyConfigBySequenceId(
    INT32                       sequenceID,
    ChiFeature2SessionInfo**    ppConfigInfo,
    UINT8                       batchIndex
    ) const
{
    UINT8 num = 0;

    if (batchIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[batchIndex].pRequestInfo;

        if ((NULL != pRequestInfo) && (InvalidProcessSequenceId != sequenceID))
        {
            *ppConfigInfo   = pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig;
            num             = pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig;
        }
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d", batchIndex, m_numRequests);
    }
    return num;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetDependencyConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 ChiFeature2RequestObject::GetDependencyConfig(
    ChiFeature2SequenceOrder    sequenceOrder,
    ChiFeature2SessionInfo**    ppConfigInfo,
    UINT8                       requestIndex
    ) const
{
    UINT8 num           = 0;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        INT32                   sequenceID      = pRequestInfo->curProcessSequenceId;

        if (ChiFeature2SequenceOrder::Next == sequenceOrder)
        {
            sequenceID = pRequestInfo->nextProcessSequenceId;
        }

        if (InvalidProcessSequenceId != sequenceID)
        {
            *ppConfigInfo   = pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig;
            num             = pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig;
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }
    return num;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetFinalBufferMetadataInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::GetFinalBufferMetadataInfo(
    ChiFeature2Identifier               identifier,
    ChiFeature2BufferMetadataInfo**     ppBufferMetaInfo,
    UINT8                               requestIndex
    ) const
{
    CDKResult result = CDKResultEFailed;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;

        for (UINT8 portIndex = 0; portIndex < pRequestInfo->numRequestOutputs; portIndex++)
        {
            if (identifier == pRequestInfo->pRequestOutput[portIndex].pPortDescriptor->globalId)
            {
                *ppBufferMetaInfo   = &pRequestInfo->pRequestOutput[portIndex].bufferMetadataInfo;
                result              = CDKResultSuccess;
                break;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetFinalGlobalIdentifiers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NOWHINE CP006: Need whiner update: std::vector allowed in exceptional cases
std::vector<const ChiFeature2Identifier*> ChiFeature2RequestObject::GetFinalGlobalIdentifiers(
    UINT8 requestIndex)
{
    // NOWHINE CP006: Need whiner update: std::vector allowed in exceptional cases
    std::vector<const ChiFeature2Identifier*> pGlobalIdentifierVector;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        pGlobalIdentifierVector.reserve(pRequestInfo->numRequestOutputs);
        for (UINT8 portIndex = 0; portIndex < pRequestInfo->numRequestOutputs; portIndex++)
        {
            pGlobalIdentifierVector.push_back(&pRequestInfo->pRequestOutput[portIndex].pPortDescriptor->globalId);
        }
    }
    return pGlobalIdentifierVector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetConfigInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetConfigInfo(
    UINT                                maxSequence,
    UINT8                               numSessions,
    const ChiFeature2SessionDescriptor* pSession)
{
    CDKResult   result = CDKResultSuccess;

    m_maxInfo.maxSequence = maxSequence;
    m_maxInfo.maxSession  = numSessions;
    m_maxInfo.maxPipeline = 0;
    m_maxInfo.maxPort     = 0;
    for (UINT8 sessionIndex = 0; sessionIndex < numSessions; sessionIndex++)
    {
        m_maxInfo.maxPipeline = pSession->numPipelines;
        for (UINT pipelineIndex = 0; pipelineIndex < pSession->numPipelines; pipelineIndex++)
        {
            if (m_maxInfo.maxPort < pSession->pPipeline[pipelineIndex].numInputPorts)
            {
                m_maxInfo.maxPort = pSession->pPipeline[pipelineIndex].numInputPorts;
            }
            if (m_maxInfo.maxPort < pSession->pPipeline[pipelineIndex].numOutputPorts)
            {
                m_maxInfo.maxPort = pSession->pPipeline[pipelineIndex].numOutputPorts;
            }
        }
    }
    CHX_LOG_INFO("%s Max info for FRO Seq=%d Session=%d Pipeline=%d Port=%d",
                 IdentifierString(),
                 m_maxInfo.maxSequence,
                 m_maxInfo.maxSession,
                 m_maxInfo.maxPipeline,
                 m_maxInfo.maxPort);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetNextStageInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetNextStageInfo(
    UINT8                           stageId,
    UINT8                           stageSequenceId,
    UINT8                           numDependencyConfigDescriptor,
    const ChiFeature2SessionInfo*   pDependencyConfigDescriptor,
    UINT8                           numMaxDependencies,
    UINT8                           requestIndex)
{
    CDKResult   result      = CDKResultSuccess;

    ChiFeature2RequestInfo* pRequestInfo = NULL;

    if (requestIndex < m_numRequests)
    {
        pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        INT32   sequenceID = pRequestInfo->curProcessSequenceId;
        if ((0 != numDependencyConfigDescriptor) && (NULL != pDependencyConfigDescriptor))
        {
            if (m_maxInfo.maxSequence == InvalidProcessSequenceNumber)
            {
                CHX_LOG_ERROR("Maximum number of process sequences not set");
                result = CDKResultEInvalidState;
            }

            if ((CDKResultSuccess == result) && (NULL == m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo))
            {
                m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo = static_cast<ChiFeature2ProcessSequenceInfo*>
                    (CHX_CALLOC(sizeof(ChiFeature2ProcessSequenceInfo) * m_maxInfo.maxSequence));
                pRequestInfo->ppProcessSequenceInfo                     = static_cast<ChiFeature2ProcessSequenceInfo**>
                    (CHX_CALLOC(sizeof(ChiFeature2ProcessSequenceInfo*) * m_maxInfo.maxSequence));
                if ((NULL == m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo) ||
                    (NULL == pRequestInfo->ppProcessSequenceInfo))
                {
                    if (NULL != pRequestInfo->ppProcessSequenceInfo)
                    {
                        CHX_FREE(pRequestInfo->ppProcessSequenceInfo);
                        pRequestInfo->ppProcessSequenceInfo = NULL;
                    }
                    if (NULL != m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo)
                    {
                        CHX_FREE(m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo);
                        m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo = NULL;
                    }
                    result = CDKResultENoMemory;
                }
            }

            if (CDKResultSuccess == result)
            {
                sequenceID = pRequestInfo->curProcessSequenceId + 1;
                if (sequenceID < m_maxInfo.maxSequence)
                {
                    pRequestInfo->nextProcessSequenceId                                   = sequenceID;
                    pRequestInfo->ppProcessSequenceInfo[sequenceID]                       =
                        &m_pBatchOutputRequestInfo[requestIndex].pBaseSequenceInfo[sequenceID];
                    pRequestInfo->ppProcessSequenceInfo[sequenceID]->stageId              = stageId;
                    pRequestInfo->ppProcessSequenceInfo[sequenceID]->stageSequenceId      = stageSequenceId;
                    pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig  =
                        numDependencyConfigDescriptor;
                    pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig    =
                        static_cast<ChiFeature2SessionInfo*>
                        (CHX_CALLOC(sizeof(ChiFeature2SessionInfo) * numDependencyConfigDescriptor));
                    if (NULL != pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig)
                    {
                        for (UINT sessionIndex = 0; sessionIndex < numDependencyConfigDescriptor; sessionIndex++)
                        {
                            ChiFeature2SessionInfo* pDependencyConfig =
                                pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig;

                            pDependencyConfig[sessionIndex].numPipelines         =
                                pDependencyConfigDescriptor[sessionIndex].numPipelines;
                            pDependencyConfig[sessionIndex].sessionId            =
                                pDependencyConfigDescriptor[sessionIndex].sessionId;
                            pDependencyConfig[sessionIndex].pPipelineInfo        = static_cast<ChiFeature2PipelineInfo*>
                                (CHX_CALLOC(sizeof(ChiFeature2PipelineInfo) *
                                    pDependencyConfigDescriptor[sessionIndex].numPipelines));
                            if (NULL != pDependencyConfig[sessionIndex].pPipelineInfo)
                            {
                                for (UINT pipelineIndex = 0; pipelineIndex < pDependencyConfig[sessionIndex].numPipelines;
                                    pipelineIndex++)
                                {
                                    ChiFeature2PipelineInfo*    pPipelineInfo = pDependencyConfig[sessionIndex].pPipelineInfo;

                                    pPipelineInfo[pipelineIndex].numHandles   =
                                        pDependencyConfigDescriptor[sessionIndex].pPipelineInfo[pipelineIndex].numHandles;
                                    pPipelineInfo[pipelineIndex].pipelineId   =
                                        pDependencyConfigDescriptor[sessionIndex].pPipelineInfo[pipelineIndex].pipelineId;

                                    if (ChiFeature2HandleType::DependencyConfigInfo ==
                                        pDependencyConfigDescriptor[sessionIndex].pPipelineInfo[pipelineIndex].type)
                                    {
                                        ChiFeature2DependencyConfigDescriptor* pConfigDesc =
                                            reinterpret_cast<ChiFeature2DependencyConfigDescriptor*>
                                            (pDependencyConfigDescriptor[sessionIndex].
                                                pPipelineInfo[pipelineIndex].handle);
                                        ChiFeature2DependencyConfigInfo* pConfigInfo       =
                                            static_cast<ChiFeature2DependencyConfigInfo*>
                                            (CHX_CALLOC(sizeof(ChiFeature2DependencyConfigInfo)));
                                        if (pConfigInfo != NULL)
                                        {
                                            pPipelineInfo[pipelineIndex].type    = ChiFeature2HandleType::DependencyConfigInfo;
                                            pPipelineInfo[pipelineIndex].handle  =
                                                static_cast<CHIFEATURE2HANDLE>(pConfigInfo);
                                            result = PopulateConfigInfoWithDescriptor(pConfigInfo, pConfigDesc);
                                            result = PopulateDependencyInfo(pConfigInfo, pConfigDesc, numMaxDependencies);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Maximum number of process sequences exceeded");
                    result = CDKResultEOutOfBounds;
                }

                if (CDKResultSuccess == result)
                {
                    result = CreateFeatureIdentifierMap(&pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap);
                }
            }
        }
        else
        {
            result = CDKResultEInvalidArg;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::PopulateConfigInfoWithDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::PopulateConfigInfoWithDescriptor(
    ChiFeature2DependencyConfigInfo*             pConfigInfo,
    const ChiFeature2DependencyConfigDescriptor* pConfigDesc)
{
    CDKResult result = CDKResultSuccess;

    if (0 != pConfigDesc->numInputConfig)
    {
        pConfigInfo->inputConfig.num                    = pConfigDesc->numInputConfig;
        pConfigInfo->inputConfig.pPortDescriptor        = static_cast<ChiFeature2PortDescriptor*>
            (CHX_CALLOC(sizeof(ChiFeature2PortDescriptor) * pConfigInfo->inputConfig.num));
        pConfigInfo->inputConfig.phTargetBufferHandle   = static_cast<CHITARGETBUFFERINFOHANDLE*>
            (CHX_CALLOC(sizeof(CHITARGETBUFFERINFOHANDLE) * pConfigInfo->inputConfig.num));
        pConfigInfo->inputConfig.pBufferErrorPresent = static_cast<BOOL*>
            (CHX_CALLOC(sizeof(BOOL) * pConfigInfo->inputConfig.num));
        pConfigInfo->inputConfig.pKey                   = static_cast<UINT64*>
            (CHX_CALLOC(sizeof(UINT64) * pConfigInfo->inputConfig.num));
        pConfigInfo->inputConfig.phMetadata             = static_cast<CHIMETAHANDLE*>
            (CHX_CALLOC(sizeof(CHIMETAHANDLE) * pConfigInfo->inputConfig.num));
        pConfigInfo->numEnabledInputPorts               = 0;
    }

    if ((0 != pConfigInfo->inputConfig.num) &&
        ((NULL == pConfigInfo->inputConfig.pPortDescriptor) ||
         (NULL == pConfigInfo->inputConfig.phTargetBufferHandle) ||
         (NULL == pConfigInfo->inputConfig.pBufferErrorPresent) ||
         (NULL == pConfigInfo->inputConfig.pKey)))
    {
        CHX_LOG_ERROR("Allocation Failed");
        result = CDKResultEFailed;
    }

    if ((CDKResultSuccess == result) && (0 != pConfigDesc->numOutputConfig))
    {
        pConfigInfo->outputConfig.num                       = pConfigDesc->numOutputConfig;
        pConfigInfo->outputConfig.pPortDescriptor           = static_cast<ChiFeature2PortDescriptor*>
            (CHX_CALLOC(sizeof(ChiFeature2PortDescriptor) * pConfigInfo->outputConfig.num));
        pConfigInfo->outputConfig.phTargetBufferHandle      = static_cast<CHITARGETBUFFERINFOHANDLE*>
            (CHX_CALLOC(sizeof(CHITARGETBUFFERINFOHANDLE) * pConfigInfo->outputConfig.num));
        pConfigInfo->outputConfig.pBufferErrorPresent = static_cast<BOOL*>
            (CHX_CALLOC(sizeof(BOOL) * pConfigInfo->outputConfig.num));
        pConfigInfo->outputConfig.pKey                      = static_cast<UINT64*>
            (CHX_CALLOC(sizeof(UINT64) * pConfigInfo->outputConfig.num));
        pConfigInfo->numEnabledOutputPorts                  = 0;
        pConfigInfo->outputConfig.phMetadata                = static_cast<CHIMETAHANDLE*>
            (CHX_CALLOC(sizeof(CHIMETAHANDLE) * pConfigInfo->outputConfig.num));

    }

    if ((0 != pConfigInfo->outputConfig.num) &&
        ((NULL == pConfigInfo->outputConfig.pPortDescriptor) ||
         (NULL == pConfigInfo->outputConfig.phTargetBufferHandle) ||
         (NULL == pConfigInfo->outputConfig.pBufferErrorPresent) ||
         (NULL == pConfigInfo->outputConfig.pKey)))
    {
        CHX_LOG_ERROR("Allocation Failed");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::PopulateDependencyInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::PopulateDependencyInfo(
    ChiFeature2DependencyConfigInfo*             pConfigInfo,
    const ChiFeature2DependencyConfigDescriptor* pConfigDesc,
    UINT8                                        maxDependencies)
{

    CDKResult result = CDKResultSuccess;

    if (0 != maxDependencies)
    {
        // Identify maximum number of ports possible
        UINT8 maxDependencyPorts = 0;

        for (UINT dependencyIndex = 0; dependencyIndex < pConfigDesc->numInputDependency; dependencyIndex++)
        {
            if (maxDependencyPorts < pConfigDesc->pInputDependencyList[dependencyIndex].numInputDependency)
            {
                maxDependencyPorts = pConfigDesc->pInputDependencyList[dependencyIndex].numInputDependency;
            }
        }
        pConfigInfo->numRequestInput    = maxDependencies;
        pConfigInfo->numInputDependency = maxDependencies;
        pConfigInfo->pInputDependency   = static_cast<ChiFeature2PortBufferMetadataInfo*>
            (CHX_CALLOC(sizeof(ChiFeature2PortBufferMetadataInfo) * maxDependencies));
        pConfigInfo->pRequestInput      = static_cast<ChiFeature2MetadataInfo*>
            (CHX_CALLOC(sizeof(ChiFeature2MetadataInfo) * maxDependencies));
        if ((0 != pConfigInfo->numRequestInput) &&
            ((NULL == pConfigInfo->pInputDependency) || (NULL == pConfigInfo->pRequestInput)))
        {
            CHX_LOG_ERROR("Allocation Failed");
            result = CDKResultEFailed;
        }

        for (UINT dependencyIndex = 0; dependencyIndex < maxDependencies; dependencyIndex++)
        {
            if ((CDKResultSuccess == result))
            {
                pConfigInfo->pInputDependency[dependencyIndex].num                  =
                    maxDependencyPorts;
                pConfigInfo->pRequestInput[dependencyIndex].num                     =
                    maxDependencyPorts;
                pConfigInfo->pInputDependency[dependencyIndex].pPortDescriptor      =
                    static_cast<ChiFeature2PortDescriptor*>(CHX_CALLOC(sizeof(ChiFeature2PortDescriptor) *
                        maxDependencyPorts));
                pConfigInfo->pInputDependency[dependencyIndex].phTargetBufferHandle =
                    static_cast<CHITARGETBUFFERINFOHANDLE*>(CHX_CALLOC(sizeof(CHITARGETBUFFERINFOHANDLE) *
                        maxDependencyPorts));
                pConfigInfo->pInputDependency[dependencyIndex].pBufferErrorPresent  =
                    static_cast<BOOL*>(CHX_CALLOC(sizeof(BOOL) *
                        maxDependencyPorts));
                pConfigInfo->pInputDependency[dependencyIndex].pPortBufferStatus  =
                    static_cast<ChiFeature2PortBufferStatus*>(CHX_CALLOC(sizeof(BOOL) *
                        maxDependencyPorts));
                pConfigInfo->pInputDependency[dependencyIndex].pKey                 =
                    static_cast<UINT64*>(CHX_CALLOC(sizeof(UINT64) *
                        maxDependencyPorts));
                pConfigInfo->pInputDependency[dependencyIndex].phMetadata           =
                    static_cast<CHIMETAHANDLE*>(CHX_CALLOC(sizeof(CHIMETAHANDLE) *
                        maxDependencyPorts));

                pConfigInfo->pRequestInput[dependencyIndex].phMetadata               =
                    static_cast<CHIMETAHANDLE*>(CHX_CALLOC(sizeof(CHIMETAHANDLE) *
                        maxDependencyPorts));
                if ((0 != pConfigInfo->pInputDependency[dependencyIndex].num)                      &&
                    ((NULL == pConfigInfo->pInputDependency[dependencyIndex].pPortDescriptor)      ||
                     (NULL == pConfigInfo->pInputDependency[dependencyIndex].phTargetBufferHandle) ||
                     (NULL == pConfigInfo->pInputDependency[dependencyIndex].pBufferErrorPresent)  ||
                     (NULL == pConfigInfo->pInputDependency[dependencyIndex].pPortBufferStatus)    ||
                     (NULL == pConfigInfo->pInputDependency[dependencyIndex].pKey)                 ||
                     (NULL == pConfigInfo->pRequestInput[dependencyIndex].phMetadata)))
                {
                    CHX_LOG_ERROR("Allocation Failed");
                    result = CDKResultEFailed;
                }
            }
        }
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetPortDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetPortDescriptor(
    ChiFeature2SequenceOrder            sequenceOrder,
    ChiFeature2RequestObjectOpsType     type,
    const ChiFeature2Identifier*        pGlobalIdentifier,
    const ChiFeature2PortDescriptor*    pPortDescriptor,
    UINT8                               batchIndex,
    UINT8                               inputDependencyIndex)
{
    CDKResult               result          = CDKResultSuccess;
    INT32                   sequenceID      = InvalidProcessSequenceId;
    ChiFeature2RequestInfo* pRequestInfo    = NULL;
    ChiFeature2Identifier   localIdentifier;

    if (batchIndex < m_numRequests)
    {
        pRequestInfo    = m_pBatchOutputRequestInfo[batchIndex].pRequestInfo;
        sequenceID      = pRequestInfo->curProcessSequenceId;
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d",
            batchIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if ((CDKResultSuccess == result) && pRequestInfo->state != ChiFeature2RequestState::Executing)
    {
        CHX_LOG_ERROR("PortDescriptor cannot be set while in %s", ChiFeature2RequestStateStrings
            [static_cast<UINT>(pRequestInfo->state)]);
        result = CDKResultEInvalidState;
    }

    if ((CDKResultSuccess == result) && (ChiFeature2SequenceOrder::Next == sequenceOrder))
    {
        sequenceID = pRequestInfo->nextProcessSequenceId;
    }

    if (CDKResultSuccess == result)
    {
        if (NULL == pGlobalIdentifier)
        {
            result = CDKResultEInvalidPointer;
        }
        else
        {
            result = SetAndGetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap,
                type,
                pGlobalIdentifier,
                &localIdentifier);
        }
    }

    if ((CDKResultSuccess == result) && (InvalidProcessSequenceId != sequenceID))
    {
        if (!((localIdentifier.session  < pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig) &&
            (localIdentifier.pipeline <
                pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].numPipelines)))
        {
            CHX_LOG_ERROR("Sessionid :%d Pipelineid:%d greater than configured",
                localIdentifier.session,
                localIdentifier.pipeline);
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CHX_LOG_ERROR("Unable to set due to map failure");
        result = CDKResultEOutOfBounds;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DependencyConfigInfo* pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);

        switch (type)
        {
            case ChiFeature2RequestObjectOpsType::RequestInput:
                CHX_LOG_ERROR("Not supported");
                break;
            case ChiFeature2RequestObjectOpsType::InputDependency:
                if ((0 != pConfigInfo->numInputDependency) && (inputDependencyIndex < pConfigInfo->numInputDependency))
                {
                    if (pConfigInfo->pInputDependency[inputDependencyIndex].num > localIdentifier.port)
                    {
                        // NOWHINE CP036a:
                        ChxUtils::Memcpy(const_cast<VOID*>
                            (static_cast<const VOID*>
                            (&pConfigInfo->pInputDependency[inputDependencyIndex].pPortDescriptor[localIdentifier.port])),
                            pPortDescriptor,
                            sizeof(ChiFeature2PortDescriptor));

                        pConfigInfo->pInputDependency[inputDependencyIndex].pPortBufferStatus[localIdentifier.port] =
                            ChiFeature2PortBufferStatus::DependencyPending;

                        CHX_LOG_INFO("%s %s For PortName:%s GID[Session:%d Pipeline:%d Port:%d Type:%d]"
                            " BatchIndex:%d InputdependencyIndex:%d",
                            IdentifierString(),
                            ChiFeature2RequestObjectOpsTypeStrings[static_cast<UINT>(type)],
                            pPortDescriptor->pPortName,
                            pGlobalIdentifier->session,
                            pGlobalIdentifier->pipeline,
                            pGlobalIdentifier->port,
                            pGlobalIdentifier->portType,
                            batchIndex,
                            inputDependencyIndex);
                        pRequestInfo->pDependencyMutex->Lock();
                        pRequestInfo->ppProcessSequenceInfo[sequenceID]->inputBuffersDependenciesToBeStatisfied++;
                        pRequestInfo->ppProcessSequenceInfo[sequenceID]->inputMetadataDependenciesToBeStatisfied++;
                        pRequestInfo->ppProcessSequenceInfo[sequenceID]->maxPossibleErrors++;
                        pRequestInfo->ppProcessSequenceInfo[sequenceID]->totalInputDependencies++;
                        pRequestInfo->pDependencyMutex->Unlock();
                    }
                }
                break;
            case ChiFeature2RequestObjectOpsType::InputConfiguration:
                if (pConfigInfo->inputConfig.num > localIdentifier.port)
                {
                    // NOWHINE CP036a:
                    ChxUtils::Memcpy(const_cast<VOID*>
                        (static_cast<const VOID*>
                        (&pConfigInfo->inputConfig.pPortDescriptor[localIdentifier.port])),
                        pPortDescriptor,
                        sizeof(ChiFeature2PortDescriptor));
                    CHX_LOG_INFO("%s %s For PortName:%s GID[Session:%d Pipeline:%d Port:%d Type:%d] BatchIndex:%d",
                        IdentifierString(),
                        ChiFeature2RequestObjectOpsTypeStrings[static_cast<UINT>(type)],
                        pPortDescriptor->pPortName,
                        pGlobalIdentifier->session,
                        pGlobalIdentifier->pipeline,
                        pGlobalIdentifier->port,
                        pGlobalIdentifier->portDirectionType,
                        batchIndex);

                    pConfigInfo->numEnabledInputPorts++;
                }
                break;
            case ChiFeature2RequestObjectOpsType::OutputConfiguration:
                if (pConfigInfo->outputConfig.num > localIdentifier.port)
                {
                    // NOWHINE CP036a:
                    ChxUtils::Memcpy(const_cast<VOID*>
                        (static_cast<const VOID*>
                        (&pConfigInfo->outputConfig.pPortDescriptor[localIdentifier.port])),
                        pPortDescriptor,
                            sizeof(ChiFeature2PortDescriptor));
                    CHX_LOG_INFO("%s %s For PortName:%s GID[Session:%d Pipeline:%d Port:%d Type:%d] BatchIndex:%d",
                        IdentifierString(),
                        ChiFeature2RequestObjectOpsTypeStrings[static_cast<UINT>(type)],
                        pPortDescriptor->pPortName,
                        pGlobalIdentifier->session,
                        pGlobalIdentifier->pipeline,
                        pGlobalIdentifier->port,
                        pGlobalIdentifier->portDirectionType,
                        batchIndex);

                    pConfigInfo->numEnabledOutputPorts++;
                }
                break;
            default:
                CHX_LOG_ERROR("Unhandled case");
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetPortDescriptor
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::GetPortDescriptor(
    ChiFeature2SequenceOrder              sequenceOrder,
    ChiFeature2RequestObjectOpsType       type,
    const ChiFeature2Identifier*          pGlobalIdentifier,
    const ChiFeature2PortDescriptor**     ppPortDescriptor,
    UINT8                                 batchIndex,
    UINT8                                 inputDependencyIndex
    ) const
{
    CDKResult               result           = CDKResultSuccess;
    ChiFeature2RequestInfo* pRequestInfo     = NULL;
    INT32                   sequenceID       = InvalidProcessSequenceId;
    ChiFeature2Identifier  localIdentifier;
    if (batchIndex < m_numRequests)
    {
        pRequestInfo        = m_pBatchOutputRequestInfo[batchIndex].pRequestInfo;
        sequenceID          = pRequestInfo->curProcessSequenceId;
        *ppPortDescriptor   = NULL;
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d",
            batchIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if ((CDKResultSuccess == result) && (ChiFeature2SequenceOrder::Next == sequenceOrder))
    {
        sequenceID = pRequestInfo->nextProcessSequenceId;
    }

    if (CDKResultSuccess == result)
    {
        if (NULL == pGlobalIdentifier)
        {
            result = CDKResultEInvalidPointer;
        }
        else
        {
            result = GetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap,
                type,
                pGlobalIdentifier,
                &localIdentifier);
        }
    }

    if ((CDKResultSuccess == result) && (InvalidProcessSequenceId != sequenceID))
    {
        if (!((localIdentifier.session  < pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig) &&
              (localIdentifier.pipeline <
                  pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].numPipelines)))
        {
            CHX_LOG_ERROR("Sessionid :%d Pipelineid:%d greater than configured",
                localIdentifier.session,
                localIdentifier.pipeline);
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        result = CDKResultEOutOfBounds;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DependencyConfigInfo*  pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);

        switch (type)
        {
            case ChiFeature2RequestObjectOpsType::RequestInput:
                CHX_LOG_ERROR("Not supported");
                break;
            case ChiFeature2RequestObjectOpsType::InputDependency:
                if ((0 != pConfigInfo->numInputDependency) && (inputDependencyIndex < pConfigInfo->numInputDependency))
                {
                    if (pConfigInfo->pInputDependency[inputDependencyIndex].num > localIdentifier.port)
                    {
                        *ppPortDescriptor =
                            &pConfigInfo->pInputDependency[inputDependencyIndex].pPortDescriptor[localIdentifier.port];
                    }
                }
                break;
            case ChiFeature2RequestObjectOpsType::InputConfiguration:
                if (pConfigInfo->inputConfig.num > localIdentifier.port)
                {
                    *ppPortDescriptor = &pConfigInfo->inputConfig.pPortDescriptor[localIdentifier.port];
                }
                break;
            case ChiFeature2RequestObjectOpsType::OutputConfiguration:
                if (pConfigInfo->outputConfig.num > localIdentifier.port)
                {
                    *ppPortDescriptor = &pConfigInfo->outputConfig.pPortDescriptor[localIdentifier.port];
                }
                break;
            default:
                CHX_LOG_ERROR("Unhandled case");
                break;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetBufferInfo(
    ChiFeature2RequestObjectOpsType         type,
    const ChiFeature2Identifier*            pGlobalIdentifier,
    const CHITARGETBUFFERINFOHANDLE         hTargetBufferHandle,
    UINT64                                  key,
    BOOL                                    setBufferError,
    UINT8                                   batchIndex,
    UINT8                                   inputDependencyIndex)
{
    CDKResult               result                      = CDKResultSuccess;
    ChiFeature2RequestInfo* pRequestInfo                = NULL;
    INT32                   sequenceID                  = InvalidProcessSequenceId;
    UINT8                   totalErrorsSeen             = 0;
    UINT8                   buffersRecieved             = 0;
    UINT8                   totalNumDependencies        = 0;
    UINT8                   numOutstandingDependencies  = 0;
    ChiFeature2Identifier   localIdentifier;

    if (batchIndex < m_numRequests)
    {
        pRequestInfo    = m_pBatchOutputRequestInfo[batchIndex].pRequestInfo;
        sequenceID      = pRequestInfo->curProcessSequenceId;
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d",
            batchIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        if (NULL == pGlobalIdentifier)
        {
            result = CDKResultEInvalidPointer;
        }
        else
        {
            result = SetAndGetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap,
                type,
                pGlobalIdentifier,
                &localIdentifier);
        }
    }

    if ((CDKResultSuccess == result) && (InvalidProcessSequenceId != sequenceID))
    {
        if (!((localIdentifier.session  < pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig) &&
            (localIdentifier.pipeline <
                pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].numPipelines)))
        {
            CHX_LOG_ERROR("Sessionid :%d Pipelineid:%d greater than configured",
                localIdentifier.session,
                localIdentifier.pipeline);
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CHX_LOG_ERROR("Unable to set due to map failure");
        result = CDKResultEOutOfBounds;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DependencyConfigInfo*  pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);

        switch (type)
        {
            case ChiFeature2RequestObjectOpsType::RequestInput:
            {
                CHX_LOG_ERROR("Not supported");
                break;
            }
            case ChiFeature2RequestObjectOpsType::InputDependency:
            {
                if ((0 != pConfigInfo->numInputDependency) && (inputDependencyIndex < pConfigInfo->numInputDependency))
                {
                    ChiFeature2PortBufferMetadataInfo dependencyInfo = pConfigInfo->pInputDependency[inputDependencyIndex];

                    if (dependencyInfo.num > localIdentifier.port)
                    {
                        if (NULL == dependencyInfo.phTargetBufferHandle[localIdentifier.port])
                        {
                            dependencyInfo.phTargetBufferHandle[localIdentifier.port] = hTargetBufferHandle;
                            dependencyInfo.pKey[localIdentifier.port]                 = key;

                            // Don't overwrite the error if it's already there
                            if (TRUE != pConfigInfo->pInputDependency[inputDependencyIndex].
                                pBufferErrorPresent[localIdentifier.port])
                            {
                                dependencyInfo.
                                    pBufferErrorPresent[localIdentifier.port] = setBufferError;

                                pRequestInfo->pDependencyMutex->Lock();
                                totalNumDependencies        =
                                    pRequestInfo->ppProcessSequenceInfo[sequenceID]->totalInputDependencies;
                                numOutstandingDependencies  =
                                    pRequestInfo->ppProcessSequenceInfo[sequenceID]->inputBuffersDependenciesToBeStatisfied;
                                totalErrorsSeen             =
                                    (pRequestInfo->ppProcessSequenceInfo[sequenceID]->totalInputDependencies -
                                     pRequestInfo->ppProcessSequenceInfo[sequenceID]->maxPossibleErrors);
                                buffersRecieved             = (totalNumDependencies - numOutstandingDependencies);
                                pRequestInfo->pDependencyMutex->Unlock();

                                // If we are setting an error, mark error by decrementing out of total possible errors
                                // Otherwise mark the input dependency as satisfied
                                if (TRUE == setBufferError)
                                {
                                    pRequestInfo->pDependencyMutex->Lock();
                                    pRequestInfo->ppProcessSequenceInfo[sequenceID]->maxPossibleErrors--;
                                    pRequestInfo->pDependencyMutex->Unlock();

                                    dependencyInfo.pPortBufferStatus[localIdentifier.port] =
                                        ChiFeature2PortBufferStatus::DependencyMetWithError;
                                    CHX_LOG_VERBOSE("%s Setting a buffer error, for batch:%d; TotalInputDependencies:%d"
                                        " ErrorsObserved:%d BuffersRecieved:%d",
                                        IdentifierString(),
                                        batchIndex,
                                        totalNumDependencies,
                                        totalErrorsSeen,
                                        buffersRecieved);
                                }
                                else
                                {
                                    pRequestInfo->pDependencyMutex->Lock();
                                    pRequestInfo->ppProcessSequenceInfo[sequenceID]->
                                        inputBuffersDependenciesToBeStatisfied--;
                                    pRequestInfo->pDependencyMutex->Unlock();

                                    dependencyInfo.pPortBufferStatus[localIdentifier.port] =
                                        ChiFeature2PortBufferStatus::DependencyMetWithSuccess;
                                    CHX_LOG_VERBOSE("%s Resolving an input dependency, for batch:%d; TotalInputDependencies:%d"
                                        " ErrorsObserved:%d BuffersRecieved:%d BufferHandle:%p",
                                        IdentifierString(),
                                        batchIndex,
                                        totalNumDependencies,
                                        totalErrorsSeen,
                                        buffersRecieved,
                                        hTargetBufferHandle);
                                }
                            }

                            CHX_LOG_INFO("%s %s For GID[Session:%d Pipeline:%d Port:%d Type:%d]"
                                            " BatchIndex:%d InputDependencyIndex:%d, toBeSatisfied:%d, BufferHandle:%p",
                                            IdentifierString(),
                                            ChiFeature2RequestObjectOpsTypeStrings[static_cast<UINT>(type)],
                                            pGlobalIdentifier->session,
                                            pGlobalIdentifier->pipeline,
                                            pGlobalIdentifier->port,
                                            pGlobalIdentifier->portType,
                                            batchIndex,
                                            inputDependencyIndex,
                                            numOutstandingDependencies,
                                            hTargetBufferHandle);
                        }
                    }
                }
                break;
            }
            case ChiFeature2RequestObjectOpsType::InputConfiguration:
            {
                if (pConfigInfo->inputConfig.num > localIdentifier.port)
                {
                    pConfigInfo->inputConfig.phTargetBufferHandle[localIdentifier.port] = hTargetBufferHandle;
                    pConfigInfo->inputConfig.pKey[localIdentifier.port]                 = key;
                    CHX_LOG_INFO("%s %s For GID[Session:%d Pipeline:%d Port:%d Type:%d] BatchIndex:%d, BufferHandle:%p",
                        IdentifierString(),
                        ChiFeature2RequestObjectOpsTypeStrings[static_cast<UINT>(type)],
                        pGlobalIdentifier->session,
                        pGlobalIdentifier->pipeline,
                        pGlobalIdentifier->port,
                        pGlobalIdentifier->portType,
                        batchIndex,
                        hTargetBufferHandle);
                }
                break;
            }
            case ChiFeature2RequestObjectOpsType::OutputConfiguration:
            {
                if (pConfigInfo->outputConfig.num > localIdentifier.port)
                {
                    pConfigInfo->outputConfig.phTargetBufferHandle[localIdentifier.port] = hTargetBufferHandle;
                    pConfigInfo->outputConfig.pKey[localIdentifier.port]                 = key;
                    CHX_LOG_INFO("%s %s For GID[Session:%d Pipeline:%d Port:%d Type:%d] BatchIndex:%d, BufferHandle:%p",
                        IdentifierString(),
                        ChiFeature2RequestObjectOpsTypeStrings[static_cast<UINT>(type)],
                        pGlobalIdentifier->session,
                        pGlobalIdentifier->pipeline,
                        pGlobalIdentifier->port,
                        pGlobalIdentifier->portType,
                        batchIndex,
                        hTargetBufferHandle);
                }
                break;
            }
            default:
            {
                CHX_LOG_ERROR("Unhandled case");
                break;
            }
        }
    }
    else
    {
        result = CDKResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetBufferStatus
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetBufferStatus(
    ChiFeature2RequestObjectOpsType         type,
    const ChiFeature2Identifier*            pGlobalIdentifier,
    UINT8                                   sequenceId,
    ChiFeature2PortBufferStatus             portBufferStatus,
    UINT8                                   batchIndex,
    UINT8                                   inputDependencyIndex)
{
    CDKResult               result                      = CDKResultSuccess;
    ChiFeature2RequestInfo* pRequestInfo                = NULL;

    ChiFeature2Identifier   localIdentifier;

    if (batchIndex < m_numRequests)
    {
        pRequestInfo    = m_pBatchOutputRequestInfo[batchIndex].pRequestInfo;
        if ((pRequestInfo->state != ChiFeature2RequestState::Executing             &&
             pRequestInfo->state != ChiFeature2RequestState::InputResourcePending  &&
             pRequestInfo->state != ChiFeature2RequestState::OutputResourcePending &&
             pRequestInfo->state != ChiFeature2RequestState::OutputNotificationPending))
        {
            CHX_LOG_WARN("BufferStatus cannot be set while in %s", ChiFeature2RequestStateStrings
                [static_cast<UINT>(pRequestInfo->state)]);
            result = CDKResultEInvalidState;
        }
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d",
            batchIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        if (NULL == pGlobalIdentifier)
        {
            result = CDKResultEInvalidPointer;
        }
        else
        {
            result = SetAndGetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceId]->pMap,
                type,
                pGlobalIdentifier,
                &localIdentifier);

            if (CDKResultSuccess == result)
            {
                if (!((localIdentifier.session  < pRequestInfo->ppProcessSequenceInfo[sequenceId]->numDependencyConfig) &&
                    (localIdentifier.pipeline <
                        pRequestInfo->ppProcessSequenceInfo[sequenceId]->pDependencyConfig[localIdentifier.session]
                        .numPipelines)))
                {
                    CHX_LOG_ERROR("Sessionid :%d Pipelineid:%d greater than configured",
                        localIdentifier.session,
                        localIdentifier.pipeline);
                    result = CDKResultEOutOfBounds;
                }
            }
            else
            {
                CHX_LOG_ERROR("Unable to set due to map failure");
                result = CDKResultEOutOfBounds;
            }
        }
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DependencyConfigInfo*  pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceId]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);

        switch (type)
        {
            case ChiFeature2RequestObjectOpsType::InputDependency:
            {
                if ((0 != pConfigInfo->numInputDependency) && (inputDependencyIndex < pConfigInfo->numInputDependency))
                {
                    if (pConfigInfo->pInputDependency[inputDependencyIndex].num > localIdentifier.port)
                    {
                        pConfigInfo->pInputDependency[inputDependencyIndex].pPortBufferStatus[localIdentifier.port]    =
                            portBufferStatus;
                    }
                }
                break;
            }
            default:
                CHX_LOG_ERROR("Unsupported operation");
                break;
        }
    }
    return result;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetBufferInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::GetBufferInfo(
    ChiFeature2RequestObjectOpsType    type,
    const ChiFeature2Identifier*       pGlobalIdentifier,
    CHITARGETBUFFERINFOHANDLE*         phTargetBufferHandle,
    UINT64*                            pKey,
    UINT8                              batchIndex,
    UINT8                              inputDependencyIndex
    ) const
{
    CDKResult               result            = CDKResultSuccess;
    INT32                   sequenceID        = InvalidProcessSequenceId;
    ChiFeature2RequestInfo* pRequestInfo      = NULL;
    ChiFeature2Identifier   localIdentifier;

    if (batchIndex < m_numRequests)
    {
        pRequestInfo    = m_pBatchOutputRequestInfo[batchIndex].pRequestInfo;
        sequenceID      = pRequestInfo->curProcessSequenceId;
        if (NULL == pGlobalIdentifier)
        {
            result = CDKResultEInvalidPointer;
        }
        else
        {
            result = GetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap,
                type,
                pGlobalIdentifier,
                &localIdentifier);
        }
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d",
            batchIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if ((CDKResultSuccess == result) && (InvalidProcessSequenceId != sequenceID))
    {
        if (!((localIdentifier.session  < pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig) &&
              (localIdentifier.pipeline <
                  pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].numPipelines)))
        {
            CHX_LOG_ERROR("Sessionid :%d Pipelineid:%d greater than configured",
                localIdentifier.session,
                localIdentifier.pipeline);
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CHX_LOG_WARN("Buffer not set, result:%d", result);
        result = CDKResultEOutOfBounds;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DependencyConfigInfo*  pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);

        switch (type)
        {
            case ChiFeature2RequestObjectOpsType::RequestInput:
                CHX_LOG_ERROR("Not supported");
                break;
            case ChiFeature2RequestObjectOpsType::InputDependency:
                if ((0 != pConfigInfo->numInputDependency) && (inputDependencyIndex < pConfigInfo->numInputDependency))
                {
                    if (pConfigInfo->pInputDependency[inputDependencyIndex].num > localIdentifier.port)
                    {
                        *phTargetBufferHandle =
                            pConfigInfo->pInputDependency[inputDependencyIndex].phTargetBufferHandle[localIdentifier.port];
                        *pKey                 =
                            pConfigInfo->pInputDependency[inputDependencyIndex].pKey[localIdentifier.port];
                    }
                }
                break;
            case ChiFeature2RequestObjectOpsType::InputConfiguration:
                if (pConfigInfo->inputConfig.num > localIdentifier.port)
                {
                    *phTargetBufferHandle = pConfigInfo->inputConfig.phTargetBufferHandle[localIdentifier.port];
                    *pKey                 = pConfigInfo->inputConfig.pKey[localIdentifier.port];
                }
                break;
            case ChiFeature2RequestObjectOpsType::OutputConfiguration:
                if (pConfigInfo->outputConfig.num > localIdentifier.port)
                {
                    *phTargetBufferHandle = pConfigInfo->outputConfig.phTargetBufferHandle[localIdentifier.port];
                    *pKey                 = pConfigInfo->outputConfig.pKey[localIdentifier.port];
                }
                break;
            default:
                CHX_LOG_ERROR("Unhandled case");
                break;
        }
    }
    else
    {
        result = CDKResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetMetadataInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetMetadataInfo(
    ChiFeature2RequestObjectOpsType type,
    const ChiFeature2Identifier*    pGlobalIdentifier,
    const CHIMETAHANDLE             hMetadata,
    UINT8                           batchIndex,
    UINT8                           inputDependencyIndex)
{
    CDKResult               result           = CDKResultSuccess;
    ChiFeature2RequestInfo* pRequestInfo     = NULL;
    INT32                   sequenceID       = InvalidProcessSequenceId;
    ChiFeature2Identifier   localIdentifier;

    if (batchIndex < m_numRequests)
    {
        pRequestInfo = m_pBatchOutputRequestInfo[batchIndex].pRequestInfo;
        sequenceID   = pRequestInfo->curProcessSequenceId;

        if (pRequestInfo->state != ChiFeature2RequestState::Executing &&
            pRequestInfo->state != ChiFeature2RequestState::InputResourcePending)
        {
            CHX_LOG_ERROR("MetadataInfo cannot be set while in %s",
                ChiFeature2RequestStateStrings[static_cast<UINT>(pRequestInfo->state)]);
            result = CDKResultEInvalidState;
        }
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d",
            batchIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if (CDKResultSuccess == result)
    {
        if (NULL == pGlobalIdentifier)
        {
            result = CDKResultEInvalidPointer;
        }
        else
        {
            result = SetAndGetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap,
                type,
                pGlobalIdentifier,
                &localIdentifier);
        }
    }

    if ((CDKResultSuccess == result) && (InvalidProcessSequenceId != sequenceID))
    {
        if (!((localIdentifier.session  < pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig) &&
              (localIdentifier.pipeline <
                  pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].numPipelines)))
        {
            CHX_LOG_ERROR("Sessionid :%d Pipelineid:%d greater than configured",
                localIdentifier.session,
                localIdentifier.pipeline);
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CHX_LOG_ERROR("Unable to set metadata due to map failure");
        result = CDKResultEOutOfBounds;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DependencyConfigInfo*  pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);

        switch (type)
        {
            case ChiFeature2RequestObjectOpsType::RequestInput:
            {
                CHX_LOG_ERROR("Not supported");
                break;
            }
            case ChiFeature2RequestObjectOpsType::InputDependency:
            {
                if ((0 != pConfigInfo->numInputDependency) && (inputDependencyIndex < pConfigInfo->numInputDependency))
                {
                    if (pConfigInfo->pInputDependency[inputDependencyIndex].num > localIdentifier.port)
                    {
                        if (NULL == pConfigInfo->pInputDependency[inputDependencyIndex].phMetadata[localIdentifier.port])
                        {
                            pConfigInfo->pInputDependency[inputDependencyIndex].phMetadata[localIdentifier.port] = hMetadata;
                            pRequestInfo->pDependencyMutex->Lock();
                            pRequestInfo->ppProcessSequenceInfo[sequenceID]->inputMetadataDependenciesToBeStatisfied--;
                            pRequestInfo->pDependencyMutex->Unlock();
                            CHX_LOG_INFO("%s %s For GID[Session:%d Pipeline:%d Port:%d Type:%d]"
                                            " BatchIndex:%d InputdependencyIndex:%d",
                                            IdentifierString(),
                                            ChiFeature2RequestObjectOpsTypeStrings[static_cast<UINT>(type)],
                                            pGlobalIdentifier->session,
                                            pGlobalIdentifier->pipeline,
                                            pGlobalIdentifier->port,
                                            pGlobalIdentifier->portType,
                                            batchIndex,
                                            inputDependencyIndex);
                        }
                    }
                }
                break;
            }
            case ChiFeature2RequestObjectOpsType::InputConfiguration:
            {
                if (pConfigInfo->inputConfig.num > localIdentifier.port)
                {
                    pConfigInfo->inputConfig.phMetadata[localIdentifier.port] = hMetadata;
                    CHX_LOG_INFO("%s %s For GID[Session:%d Pipeline:%d Port:%d Type:%d] BatchIndex:%d",
                        IdentifierString(),
                        ChiFeature2RequestObjectOpsTypeStrings[static_cast<UINT>(type)],
                        pGlobalIdentifier->session,
                        pGlobalIdentifier->pipeline,
                        pGlobalIdentifier->port,
                        pGlobalIdentifier->portType,
                        batchIndex);
                }
                break;
            }
            case ChiFeature2RequestObjectOpsType::OutputConfiguration:
            {
                if (pConfigInfo->outputConfig.num > localIdentifier.port)
                {
                    pConfigInfo->outputConfig.phMetadata[localIdentifier.port] = hMetadata;
                    CHX_LOG_INFO("%s %s For GID[Session:%d Pipeline:%d Port:%d Type:%d] BatchIndex:%d",
                        IdentifierString(),
                        ChiFeature2RequestObjectOpsTypeStrings[static_cast<UINT>(type)],
                        pGlobalIdentifier->session,
                        pGlobalIdentifier->pipeline,
                        pGlobalIdentifier->port,
                        pGlobalIdentifier->portType,
                        batchIndex);
                }
                break;
            }
            default:
            {
                CHX_LOG_ERROR("Unhandled case");
                break;
            }
        }
    }
    else
    {
        result = CDKResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetMetadataInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::GetMetadataInfo(
    ChiFeature2RequestObjectOpsType  type,
    const ChiFeature2Identifier*     pGlobalIdentifier,
    CHIMETAHANDLE*                   phMetadata,
    UINT8                            batchIndex,
    UINT8                            inputDependencyIndex
    ) const
{
    CDKResult               result          = CDKResultSuccess;
    INT32                   sequenceID      = InvalidProcessSequenceId;
    ChiFeature2RequestInfo* pRequestInfo    = NULL;
    ChiFeature2Identifier   localIdentifier;

    if (batchIndex < m_numRequests)
    {
        pRequestInfo    = m_pBatchOutputRequestInfo[batchIndex].pRequestInfo;
        sequenceID      = pRequestInfo->curProcessSequenceId;
        if (NULL == pGlobalIdentifier)
        {
            result = CDKResultEInvalidPointer;
        }
        else
        {
            result = GetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap,
                type,
                pGlobalIdentifier,
                &localIdentifier);
        }
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d",
            batchIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if ((CDKResultSuccess == result) && (InvalidProcessSequenceId != sequenceID))
    {
        if (!((localIdentifier.session  < pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig) &&
              (localIdentifier.pipeline <
                  pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].numPipelines)))
        {
            CHX_LOG_ERROR("Sessionid :%d Pipelineid:%d greater than configured",
                localIdentifier.session,
                localIdentifier.pipeline);
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CHX_LOG_ERROR("Metadata not set");
        result = CDKResultEOutOfBounds;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DependencyConfigInfo*  pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);

        switch (type)
        {
            case ChiFeature2RequestObjectOpsType::RequestInput:
                CHX_LOG_ERROR("Not supported");
                break;
            case ChiFeature2RequestObjectOpsType::InputDependency:
                if ((0 != pConfigInfo->numInputDependency) && (inputDependencyIndex < pConfigInfo->numInputDependency))
                {
                    if (pConfigInfo->pInputDependency[inputDependencyIndex].num > localIdentifier.port)
                    {
                        *phMetadata = pConfigInfo->pInputDependency[inputDependencyIndex].phMetadata[localIdentifier.port];
                    }
                }
                break;
            case ChiFeature2RequestObjectOpsType::InputConfiguration:
                if (pConfigInfo->inputConfig.num > localIdentifier.port)
                {
                    *phMetadata = pConfigInfo->inputConfig.phMetadata[localIdentifier.port];
                }
                break;
            case ChiFeature2RequestObjectOpsType::OutputConfiguration:
                if (pConfigInfo->outputConfig.num > localIdentifier.port)
                {
                    *phMetadata = pConfigInfo->outputConfig.phMetadata[localIdentifier.port];
                }
                break;
            default:
                CHX_LOG_ERROR("Unhandled case");
                break;
        }
    }
    else
    {
        result = CDKResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetRequestInputInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetRequestInputInfo(
    ChiFeature2SequenceOrder        sequenceOrder,
    const ChiFeature2Identifier*    pGlobalIdentifier,
    CHIMETAHANDLE                   hMetadata,
    UINT8                           metadataHandleIndex,
    UINT8                           requestIndex)
{
    CDKResult               result          = CDKResultSuccess;
    ChiFeature2RequestInfo* pRequestInfo    = NULL;
    INT32                   sequenceID      = InvalidProcessSequenceId;
    ChiFeature2Identifier   localIdentifier;

    if (requestIndex < m_numRequests)
    {
        pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        sequenceID   = pRequestInfo->curProcessSequenceId;
        if (ChiFeature2SequenceOrder::Next == sequenceOrder)
        {
            sequenceID = pRequestInfo->nextProcessSequenceId;
        }

        if (NULL == pGlobalIdentifier)
        {
            result = CDKResultEInvalidPointer;
        }
        else
        {
            result = SetAndGetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap,
                ChiFeature2RequestObjectOpsType::RequestInput,
                pGlobalIdentifier,
                &localIdentifier);
        }

        if (pRequestInfo->state != ChiFeature2RequestState::Executing)
        {
            CHX_LOG_ERROR("Request Input Info cannot be set while in %s", ChiFeature2RequestStateStrings
                [static_cast<UINT>(pRequestInfo->state)]);
            result = CDKResultEInvalidState;
        }
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if ((CDKResultSuccess == result) && (InvalidProcessSequenceId != sequenceID))
    {
        if (!((localIdentifier.session  < pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig) &&
              (localIdentifier.pipeline <
                  pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].numPipelines)))
        {
            CHX_LOG_ERROR("Sessionid :%d Pipelineid:%d greater than configured",
                localIdentifier.session,
                localIdentifier.pipeline);
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CHX_LOG_ERROR("Unable to set due to map failure");
        result = CDKResultEOutOfBounds;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DependencyConfigInfo*  pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);

        if ((0 != pConfigInfo->numInputDependency) && (metadataHandleIndex < pConfigInfo->numInputDependency))
        {
            if (pConfigInfo->pInputDependency[metadataHandleIndex].num > localIdentifier.port)
            {
                if (NULL == pConfigInfo->pInputDependency[metadataHandleIndex].phMetadata[localIdentifier.port])
                {
                    pConfigInfo->pInputDependency[metadataHandleIndex].phMetadata[localIdentifier.port] = hMetadata;
                    CHX_LOG_INFO("%s For GID[Session:%d Pipeline:%d Port:%d Type:%d]"
                        " BatchIndex:%d InputdependencyIndex:%d hMetadata = %p",
                        IdentifierString(),
                        pGlobalIdentifier->session,
                        pGlobalIdentifier->pipeline,
                        pGlobalIdentifier->port,
                        pGlobalIdentifier->portType,
                        requestIndex,
                        metadataHandleIndex,
                        hMetadata);
                }
            }
        }
    }
    else
    {
        result = CDKResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::ResetRequestInputInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::ResetRequestInputInfo(
    ChiFeature2SequenceOrder        sequenceOrder,
    const ChiFeature2Identifier*    pGlobalIdentifier,
    UINT8                           metadataHandleIndex,
    UINT8                           requestIndex)
{
    CDKResult               result          = CDKResultSuccess;
    ChiFeature2RequestInfo* pRequestInfo    = NULL;
    INT32                   sequenceID      = InvalidProcessSequenceId;
    ChiFeature2Identifier   localIdentifier;

    if (requestIndex < m_numRequests)
    {
        pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        sequenceID   = pRequestInfo->curProcessSequenceId;
        if (ChiFeature2SequenceOrder::Next == sequenceOrder)
        {
            sequenceID = pRequestInfo->nextProcessSequenceId;
        }

        if (NULL == pGlobalIdentifier)
        {
            result = CDKResultEInvalidPointer;
        }
        else
        {
            result = GetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap,
                ChiFeature2RequestObjectOpsType::RequestInput,
                pGlobalIdentifier,
                &localIdentifier);
        }
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if ((CDKResultSuccess == result) && (InvalidProcessSequenceId != sequenceID))
    {
        if (!((localIdentifier.session  < pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig) &&
              (localIdentifier.pipeline <
                  pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].numPipelines)))
        {
            CHX_LOG_ERROR("Sessionid :%d Pipelineid:%d greater than configured",
                localIdentifier.session,
                localIdentifier.pipeline);
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CHX_LOG_ERROR("Unable to set due to map failure");
        result = CDKResultEOutOfBounds;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DependencyConfigInfo*  pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);

        if ((0 != pConfigInfo->numInputDependency) && (metadataHandleIndex < pConfigInfo->numInputDependency))
        {
            if (pConfigInfo->pInputDependency[metadataHandleIndex].num > localIdentifier.port)
            {
                pConfigInfo->pInputDependency[metadataHandleIndex].phMetadata[localIdentifier.port] = NULL;
            }
        }
    }
    else
    {
        result = CDKResultENoSuch;
    }

    return result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetRequestInputInfo
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::GetRequestInputInfo(
    ChiFeature2SequenceOrder        sequenceOrder,
    const ChiFeature2Identifier*    pGlobalIdentifier,
    CHIMETAHANDLE*                  phMetadata,
    UINT8                           metadataHandleIndex,
    UINT8                           requestIndex
    ) const
{
    CDKResult               result           = CDKResultSuccess;
    ChiFeature2RequestInfo* pRequestInfo     = NULL;
    INT32                   sequenceID       = InvalidProcessSequenceId;
    ChiFeature2Identifier   localIdentifier;

    if (requestIndex < m_numRequests)
    {
        pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        sequenceID      = pRequestInfo->curProcessSequenceId;
        if (ChiFeature2SequenceOrder::Next == sequenceOrder)
        {
            sequenceID = pRequestInfo->nextProcessSequenceId;
        }

        if (NULL == pGlobalIdentifier)
        {
            result = CDKResultEInvalidPointer;
        }
        else
        {
            result = GetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap,
                ChiFeature2RequestObjectOpsType::RequestInput,
                pGlobalIdentifier,
                &localIdentifier);
        }
    }
    else
    {
        CHX_LOG_ERROR("Batch index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    if ((CDKResultSuccess == result) && (InvalidProcessSequenceId != sequenceID))
    {
        if (!((localIdentifier.session  < pRequestInfo->ppProcessSequenceInfo[sequenceID]->numDependencyConfig) &&
              (localIdentifier.pipeline <
                pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].numPipelines)))
        {
            CHX_LOG_ERROR("Sessionid :%d Pipelineid:%d greater than configured",
                localIdentifier.session,
                localIdentifier.pipeline);
            result = CDKResultEOutOfBounds;
        }
    }
    else
    {
        CHX_LOG_ERROR("Input info not set");
        result = CDKResultEOutOfBounds;
    }

    if (CDKResultSuccess == result)
    {
        ChiFeature2DependencyConfigInfo*  pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);

        if ((0 != pConfigInfo->numInputDependency) && (metadataHandleIndex < pConfigInfo->numInputDependency))
        {
            if (pConfigInfo->pInputDependency[metadataHandleIndex].num > localIdentifier.port)
            {
                *phMetadata = pConfigInfo->pInputDependency[metadataHandleIndex].phMetadata[localIdentifier.port];
            }
        }
        else
        {
            result = CDKResultENoSuch;
        }

    }
    else
    {
        result = CDKResultENoSuch;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetNumRequestInputs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 ChiFeature2RequestObject:: GetNumRequestInputs(
    const ChiFeature2Identifier*    pGlobalIdentifier,
    UINT8                           requestIndex)
{
    UINT8 num = 0;
    ChiFeature2RequestInfo* pRequestInfo  = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
    INT32                   sequenceID    = pRequestInfo->curProcessSequenceId;
    ChiFeature2Identifier   localIdentifier;

    CDKResult result = GetLocalFeatureIdentifier(pRequestInfo->ppProcessSequenceInfo[sequenceID]->pMap,
            ChiFeature2RequestObjectOpsType::RequestInput,
            pGlobalIdentifier,
            &localIdentifier);

    if ((CDKResultSuccess == result) && (InvalidProcessSequenceId != sequenceID))
    {
        ChiFeature2DependencyConfigInfo*  pConfigInfo = static_cast<ChiFeature2DependencyConfigInfo*>
            (pRequestInfo->ppProcessSequenceInfo[sequenceID]->pDependencyConfig[localIdentifier.session].
                pPipelineInfo[localIdentifier.pipeline].handle);
        if (NULL != pConfigInfo)
        {
            num = pConfigInfo->numRequestInput;
        }
    }
    else
    {
        CHX_LOG_ERROR("Request Inputs not set");
        result = CDKResultEOutOfBounds;
    }

    return num;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetOutputNotifiedForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetOutputNotifiedForPort(
    ChiFeature2Identifier  portIdentifier,
    UINT8                  requestIndex)
{
    CDKResult result = CDKResultENoSuch;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        for (UINT i = 0; i < pRequestInfo->numRequestOutputs; i++)
        {
            if (portIdentifier == pRequestInfo->pRequestOutput[i].pPortDescriptor->globalId)
            {
                if (pRequestInfo->pOutputNotified[i] != TRUE)
                {
                    pRequestInfo->pOutputNotified[i] = TRUE;
                    pRequestInfo->pDependencyMutex->Lock();
                    pRequestInfo->numOutputNotified++;
                    pRequestInfo->pDependencyMutex->Unlock();
                    result = CDKResultSuccess;
                    CHX_LOG_INFO("%s OutputNotified For port %s : GID[Session:%d Pipeline:%d Port:%d Type:%d] BatchIndex:%d",
                        IdentifierString(),
                        pRequestInfo->pRequestOutput[i].pPortDescriptor->pPortName,
                        portIdentifier.session,
                        portIdentifier.pipeline,
                        portIdentifier.port,
                        portIdentifier.portType,
                        requestIndex);
                }
                else
                {
                    CHX_LOG_INFO("%s OutputNotified Already notified For GID[Session:%d Pipeline:%d Port:%d Type:%d]"
                                 "BatchIndex:%d",
                        IdentifierString(),
                        portIdentifier.session,
                        portIdentifier.pipeline,
                        portIdentifier.port,
                        portIdentifier.portType,
                        requestIndex);
                    result = CDKResultEExists;
                }
                break;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetOutputNotifiedForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RequestObject::GetOutputNotifiedForPort(
    ChiFeature2Identifier portIdentifier,
    UINT8                 requestIndex)
{
    BOOL bNotified = FALSE;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        for (UINT i = 0; i < pRequestInfo->numRequestOutputs; i++)
        {
            if (portIdentifier == pRequestInfo->pRequestOutput[i].pPortDescriptor->globalId)
            {
                if (pRequestInfo->pOutputNotified[i] == TRUE)
                {
                    bNotified = TRUE;
                }
                break;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }

    return bNotified;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetOutputGeneratedForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetOutputGeneratedForPort(
    ChiFeature2Identifier  portIdentifier,
    UINT8                  requestIndex)
{
    CDKResult result = CDKResultENoSuch;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        for (UINT i = 0; i < pRequestInfo->numRequestOutputs; i++)
        {
            if (portIdentifier == pRequestInfo->pRequestOutput[i].pPortDescriptor->globalId)
            {
                if (pRequestInfo->pOutputGenerated[i] != TRUE)
                {
                    pRequestInfo->pOutputGenerated[i] = TRUE;
                    pRequestInfo->pDependencyMutex->Lock();
                    pRequestInfo->numOutputGenerated++;
                    pRequestInfo->pDependencyMutex->Unlock();
                    result = CDKResultSuccess;
                    CHX_LOG_VERBOSE("%s OutputGenerated For GID[Session:%d Pipeline:%d Port:%d Type:%d] BatchIndex:%d",
                        IdentifierString(),
                        portIdentifier.session,
                        portIdentifier.pipeline,
                        portIdentifier.port,
                        portIdentifier.portType,
                        requestIndex);
                }
                else
                {
                    CHX_LOG_VERBOSE("%s Output Already Generated For GID[Session:%d Pipeline:%d Port:%d Type:%d] BatchIndex:%d",
                        IdentifierString(),
                        portIdentifier.session,
                        portIdentifier.pipeline,
                        portIdentifier.port,
                        portIdentifier.portType,
                        requestIndex);
                    result = CDKResultEExists;
                }
                break;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetOutputGeneratedForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RequestObject::GetOutputGeneratedForPort(
    ChiFeature2Identifier portIdentifier,
    UINT8                 requestIndex)
{
    BOOL bGenerated = FALSE;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        for (UINT i = 0; i < pRequestInfo->numRequestOutputs; i++)
        {
            if (portIdentifier == pRequestInfo->pRequestOutput[i].pPortDescriptor->globalId)
            {
                if (pRequestInfo->pOutputGenerated[i] == TRUE)
                {
                    bGenerated = TRUE;
                }
                break;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }

    return bGenerated;
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetReleaseAcknowledgedForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetReleaseAcknowledgedForPort(
    ChiFeature2Identifier  portIdentifier,
    UINT8                  requestIndex)
{
    CDKResult result = CDKResultENoSuch;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        for (UINT i = 0; i < pRequestInfo->numRequestOutputs; i++)
        {
            if (portIdentifier == pRequestInfo->pRequestOutput[i].pPortDescriptor->globalId)
            {
                if (pRequestInfo->pReleaseAcked[i] != TRUE)
                {
                    pRequestInfo->pReleaseAcked[i] = TRUE;
                    pRequestInfo->pDependencyMutex->Lock();
                    pRequestInfo->numOutputReleased++;
                    pRequestInfo->pDependencyMutex->Unlock();
                    result = CDKResultSuccess;
                    CHX_LOG_INFO("%s Release Ack For port %s: GID[Session:%d Pipeline:%d Port:%d Type:%d] BatchIndex:%d",
                        IdentifierString(),
                        pRequestInfo->pRequestOutput[i].pPortDescriptor->pPortName,
                        portIdentifier.session,
                        portIdentifier.pipeline,
                        portIdentifier.port,
                        portIdentifier.portType,
                        requestIndex);
                }
                else
                {
                    result = CDKResultEExists;
                }
                break;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
        result = CDKResultEInvalidArg;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetReleaseAcknowledgedForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RequestObject::GetReleaseAcknowledgedForPort(
    ChiFeature2Identifier  portIdentifier,
    UINT8                  requestIndex)
{
    BOOL bNotified = FALSE;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        for (UINT i = 0; i < pRequestInfo->numRequestOutputs; i++)
        {
            if (portIdentifier == pRequestInfo->pRequestOutput[i].pPortDescriptor->globalId)
            {
                if (pRequestInfo->pReleaseAcked[i] == TRUE)
                {
                    bNotified = TRUE;
                }
                break;
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }

    return bNotified;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::AreOutputsNotified
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RequestObject::AreOutputsNotified(
    UINT8   requestIndex
    ) const
{
    BOOL bNotify = FALSE;
    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        pRequestInfo->pDependencyMutex->Lock();
        if (pRequestInfo->numOutputNotified == pRequestInfo->numRequestOutputs)
        {
            CHX_LOG_INFO("%s: Outputs notified for batch:%d with numRequestOutputs=%d",
                         IdentifierString(),
                         requestIndex,
                         pRequestInfo->numRequestOutputs);
            bNotify = TRUE;
        }
        pRequestInfo->pDependencyMutex->Unlock();
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }
    return bNotify;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::AreOutputsReleased
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RequestObject::AreOutputsReleased(
    UINT8   requestIndex
    ) const
{
    BOOL bReleased = FALSE;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        pRequestInfo->pDependencyMutex->Lock();
        if (pRequestInfo->numOutputReleased == pRequestInfo->numRequestOutputs)
        {
            CHX_LOG_INFO("%s: Outputs released for batch:%d with numRequestOutputs=%d",
                         IdentifierString(),
                         requestIndex,
                         pRequestInfo->numRequestOutputs);
            bReleased = TRUE;
        }
        pRequestInfo->pDependencyMutex->Unlock();
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }

    return bReleased;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::AreOutputsGenerated
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RequestObject::AreOutputsGenerated(
    UINT8   requestIndex
    ) const
{
    BOOL bGenerated = FALSE;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        pRequestInfo->pDependencyMutex->Lock();
        if (pRequestInfo->numOutputGenerated == pRequestInfo->numRequestOutputs)
        {
            CHX_LOG_INFO("%s: Outputs generated for batch:%d",
                IdentifierString(),
                requestIndex);
            bGenerated = TRUE;
        }
        pRequestInfo->pDependencyMutex->Unlock();
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d",
            requestIndex,
            m_numRequests);
    }

    return bGenerated;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::AreInputDependenciesStatisfied
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2InputDependencyStatus ChiFeature2RequestObject::AreInputDependenciesStatisfied(
    ChiFeature2SequenceOrder    sequenceOrder,
    UINT8                       requestIndex
    ) const
{
    ChiFeature2InputDependencyStatus    result                      =
        ChiFeature2InputDependencyStatus::inputDependenciesNotSatisfied;
    UINT8                               totalNumDependencies        = 0;
    UINT8                               numOutstandingDependencies  = 0;
    UINT8                               totalErrorsSeen             = 0;
    UINT8                               buffersRecieved             = 0;

    if (requestIndex < m_numRequests)
    {
        ChiFeature2RequestInfo* pRequestInfo    = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        INT32                   sequenceID      = pRequestInfo->curProcessSequenceId;

        if (ChiFeature2SequenceOrder::Next == sequenceOrder)
        {
            sequenceID = pRequestInfo->nextProcessSequenceId;
        }

        if (InvalidProcessSequenceId != sequenceID)
        {
            pRequestInfo->pDependencyMutex->Lock();
            totalNumDependencies        = pRequestInfo->ppProcessSequenceInfo[sequenceID]->totalInputDependencies;
            numOutstandingDependencies  =
                pRequestInfo->ppProcessSequenceInfo[sequenceID]->inputBuffersDependenciesToBeStatisfied;
            totalErrorsSeen             = (pRequestInfo->ppProcessSequenceInfo[sequenceID]->totalInputDependencies -
                                           pRequestInfo->ppProcessSequenceInfo[sequenceID]->maxPossibleErrors);
            buffersRecieved             = (totalNumDependencies - numOutstandingDependencies);
            pRequestInfo->pDependencyMutex->Unlock();

            if (0 == numOutstandingDependencies)
            {
                CHX_LOG_INFO("%s: Input Dependency Satisfied for batch:%d", IdentifierString(), requestIndex);
                result = ChiFeature2InputDependencyStatus::inputDependenciesSatisfied;
            }
            else if (totalNumDependencies == totalErrorsSeen)
            {
                CHX_LOG_ERROR("%s Input Dependencies errored out for batch:%d; TotalInputDependencies:%d"
                              " ErrorsObserved:%d BuffersRecieved:%d",
                              IdentifierString(),
                              requestIndex,
                              totalNumDependencies,
                              totalErrorsSeen,
                              buffersRecieved);
                result = ChiFeature2InputDependencyStatus::inputDependenciesErroredOut;
            }
            else if (totalNumDependencies == (totalErrorsSeen + buffersRecieved) && (0 < totalErrorsSeen))
            {
                CHX_LOG_WARN("%s Input Dependencies satisfied with error for batch:%d; TotalInputDependencies:%d"
                             " ErrorsObserved:%d BuffersRecieved:%d",
                             IdentifierString(),
                             requestIndex,
                             totalNumDependencies,
                             totalErrorsSeen,
                             buffersRecieved);
                result = ChiFeature2InputDependencyStatus::inputDependenciesSatisfiedWithError;
            }
            else
            {
                CHX_LOG_VERBOSE("%s Input Dependencies yet to satisfied for batch:%d; TotalInputDependencies:%d"
                                " ErrorsObserved:%d BuffersRecieved:%d",
                                IdentifierString(),
                                requestIndex,
                                totalNumDependencies,
                                totalErrorsSeen,
                                buffersRecieved);
            }
        }
    }
    else
    {
        CHX_LOG_ERROR("Request index:%d is more than supported:%d", requestIndex, m_numRequests);
        result = ChiFeature2InputDependencyStatus::unknownInputDependencyStatus;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::CreateFeatureIdentifierMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::CreateFeatureIdentifierMap(
    CHIFeature2IdentifierMap**       ppMap)
{
    CDKResult result = CDKResultSuccess;

    CHIFeature2IdentifierMap* pMap = static_cast<CHIFeature2IdentifierMap*>(CHX_CALLOC(sizeof(CHIFeature2IdentifierMap)));
    if ((NULL != pMap) && (TRUE == EnableFIDMapping))
    {
        pMap->maxSession        = m_maxInfo.maxSequence;
        pMap->maxPipeline       = m_maxInfo.maxPipeline;
        pMap->maxPort           = m_maxInfo.maxPort;
        pMap->lastSessionIndex  = 0;
        pMap->pSessionMap       =
            static_cast<ChiFeature2SessionMap*>(CHX_CALLOC(sizeof(ChiFeature2SessionMap) * pMap->maxSession));
        if (NULL != pMap->pSessionMap)
        {
            for (UINT8 sessionindex = 0; sessionindex < pMap->maxSession; sessionindex++)
            {
                pMap->pSessionMap[sessionindex].lastPipelineIndex   = 0;
                pMap->pSessionMap[sessionindex].sessionIndex        = InvalidFeatureIndex;
                pMap->pSessionMap[sessionindex].pPipelineMap        =
                    static_cast<ChiFeature2PipelineMap*>(CHX_CALLOC(sizeof(ChiFeature2PipelineMap) * pMap->maxPipeline));
                if (NULL != pMap->pSessionMap[sessionindex].pPipelineMap)
                {
                    for (UINT8 pipelineIndex = 0; pipelineIndex < pMap->maxPipeline; pipelineIndex++)
                    {
                        pMap->pSessionMap[sessionindex].pPipelineMap[pipelineIndex].pipelineIndex = InvalidFeatureIndex;
                        for (UINT8 portmapindex = 0; portmapindex < OpTypeMax; portmapindex++)
                        {
                            pMap->pSessionMap[sessionindex].pPipelineMap[pipelineIndex].portMap[portmapindex].lastPortIndex = 0;
                            pMap->pSessionMap[sessionindex].pPipelineMap[pipelineIndex].portMap[portmapindex].pPortIndexs   =
                                static_cast<UINT8*>(CHX_CALLOC(sizeof(UINT8) * pMap->maxPort));
                            if (NULL !=
                                pMap->pSessionMap[sessionindex].pPipelineMap[pipelineIndex].portMap[portmapindex].pPortIndexs)
                            {
                                for (UINT8 portIndex = 0; portIndex < pMap->maxPort; portIndex++)
                                {
                                    pMap->pSessionMap[sessionindex].pPipelineMap[pipelineIndex].portMap[portmapindex].
                                        pPortIndexs[portIndex] = InvalidFeatureIndex;
                                }
                                *ppMap = pMap;
                                result = CDKResultSuccess;
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
/// ChiFeature2RequestObject::DeleteFeatureIdentifierMap
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RequestObject::DeleteFeatureIdentifierMap(
    CHIFeature2IdentifierMap* pMap)
{
    if (NULL != pMap)
    {
        for (UINT sessionindex = 0; sessionindex < pMap->maxSession; sessionindex++)
        {
            if (NULL != pMap->pSessionMap[sessionindex].pPipelineMap)
            {
                for (UINT pipelineIndex = 0; pipelineIndex < pMap->maxPipeline; pipelineIndex++)
                {
                    for (UINT opType = 0; opType < OpTypeMax; opType++)
                    {
                        if (NULL != pMap->pSessionMap[sessionindex].pPipelineMap[pipelineIndex].portMap[opType].pPortIndexs)
                        {
                            CHX_FREE(pMap->pSessionMap[sessionindex].pPipelineMap[pipelineIndex].portMap[opType].pPortIndexs);
                        }
                    }
                }
                CHX_FREE(pMap->pSessionMap[sessionindex].pPipelineMap);
            }
        }

        if (NULL != pMap->pSessionMap)
        {
            CHX_FREE(pMap->pSessionMap);
            pMap->pSessionMap = NULL;
        }

        CHX_FREE(pMap);
        pMap = NULL;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::MapFeatureIdentifier
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::MapFeatureIdentifier(
    CHIFeature2IdentifierMap*       pMap,
    ChiFeature2RequestObjectOpsType opsType,
    const ChiFeature2Identifier*    pGlobalFID,
    ChiFeature2Identifier*          pLocalFID)
{
    CDKResult result = CDKResultSuccess;

    UINT8 type = 0;
    switch (opsType)
    {
        case ChiFeature2RequestObjectOpsType::RequestInput:
        case ChiFeature2RequestObjectOpsType::InputDependency:
        {
            type = OpType0;
            break;
        }
        case ChiFeature2RequestObjectOpsType::InputConfiguration:
        {
            type = OpType1;
            break;
        }
        case ChiFeature2RequestObjectOpsType::OutputConfiguration:
        {
            type = OpType2;
            break;
        }
        default:
            break;
    }

    if ((NULL == pGlobalFID) || (NULL == pLocalFID))
    {
        result = CDKResultEInvalidArg;
    }
    else
    {
        if ((pGlobalFID->session > pMap->maxSession) ||
            (pGlobalFID->pipeline > pMap->maxPipeline) ||
            (pGlobalFID->port > pMap->maxPort))
        {

            CHX_LOG_ERROR("  Mapping [%d, %d, %d] not possible as configured max [%d, %d ,%d]",
                pGlobalFID->session,
                pGlobalFID->pipeline,
                pGlobalFID->port,
                pMap->maxSession,
                pMap->maxPipeline,
                pMap->maxPort);
            result = CDKResultEInvalidArg;
        }
    }

    if (CDKResultSuccess == result)
    {
        if (pMap->pSessionMap[pGlobalFID->session].sessionIndex == InvalidFeatureIndex)
        {
            if (pMap->lastSessionIndex < pMap->maxSession)
            {
                pMap->pSessionMap[pGlobalFID->session].sessionIndex = pMap->lastSessionIndex++;
            }
            else
            {
                result = CDKResultEOutOfBounds;
            }
        }
        if (pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].pipelineIndex == InvalidFeatureIndex)
        {
            if (pMap->pSessionMap[pGlobalFID->session].lastPipelineIndex < pMap->maxPipeline)
            {
                pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].pipelineIndex =
                    pMap->pSessionMap[pGlobalFID->session].lastPipelineIndex++;
            }
            else
            {
                result = CDKResultEOutOfBounds;
            }
        }
        if (pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].portMap[type].
            pPortIndexs[pGlobalFID->port] == InvalidFeatureIndex)
        {
            if (pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].
                portMap[type].lastPortIndex < pMap->maxPort)
            {
                pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].portMap[type].
                    pPortIndexs[pGlobalFID->port] = pMap->pSessionMap[pGlobalFID->session].
                    pPipelineMap[pGlobalFID->pipeline].portMap[type].lastPortIndex++;
            }
            else
            {
                result = CDKResultEOutOfBounds;
            }
        }
        if (CDKResultSuccess == result)
        {
            pLocalFID->session  = pMap->pSessionMap[pGlobalFID->session].sessionIndex;
            pLocalFID->pipeline = pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].pipelineIndex;
            pLocalFID->port     = pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].
                portMap[type].pPortIndexs[pGlobalFID->port];
            CHX_LOG_VERBOSE("Mapping [%d,%d,%d] --> [%d %d %d]",
                pGlobalFID->session,
                pGlobalFID->pipeline,
                pGlobalFID->port,
                pLocalFID->session,
                pLocalFID->pipeline,
                pLocalFID->port);
        }
        else
        {
            CHX_LOG_ERROR("Mapping [%d, %d, %d] not possible as configured max [%d, %d ,%d]",
                pGlobalFID->session,
                pGlobalFID->pipeline,
                pGlobalFID->port,
                pMap->maxSession,
                pMap->maxPipeline,
                pMap->maxPort);
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetAndGetLocalFeatureIdentifier
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetAndGetLocalFeatureIdentifier(
    CHIFeature2IdentifierMap*        pMap,
    ChiFeature2RequestObjectOpsType  opsType,
    const ChiFeature2Identifier*     pGlobalFID,
    ChiFeature2Identifier*           pLocalFID)
{
    CDKResult result = CDKResultSuccess;

    UINT8 type = 0;

    switch (opsType)
    {
        case ChiFeature2RequestObjectOpsType::RequestInput:
        case ChiFeature2RequestObjectOpsType::InputDependency:
            type = OpType0;
            break;
        case ChiFeature2RequestObjectOpsType::InputConfiguration:
            type = OpType1;
            break;
        case ChiFeature2RequestObjectOpsType::OutputConfiguration:
            type = OpType2;
            break;
        default:
            break;
    }

    if (TRUE == EnableFIDMapping)
    {
        if ((pMap->pSessionMap[pGlobalFID->session].sessionIndex != InvalidFeatureIndex) &&
            (pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].pipelineIndex != InvalidFeatureIndex) &&
            (pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].portMap[type].
                pPortIndexs[pGlobalFID->port] != InvalidFeatureIndex))
        {
            pLocalFID->session  = pMap->pSessionMap[pGlobalFID->session].sessionIndex;
            pLocalFID->pipeline = pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].pipelineIndex;
            pLocalFID->port     =
                pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].portMap[type].
                pPortIndexs[pGlobalFID->port];
            CHX_LOG_VERBOSE("Mapped Feature Identifier [%d, %d, %d] --> [%d, %d, %d]",
                pGlobalFID->session,
                pGlobalFID->pipeline,
                pGlobalFID->port,
                pLocalFID->session,
                pLocalFID->pipeline,
                pLocalFID->port);
        }
        else
        {
            result = MapFeatureIdentifier(pMap, opsType, pGlobalFID, pLocalFID);
        }
    }
    else
    {
        pLocalFID->session  = pGlobalFID->session;
        pLocalFID->pipeline = pGlobalFID->pipeline;
        pLocalFID->port     = pGlobalFID->port;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::GetLocalFeatureIdentifier
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::GetLocalFeatureIdentifier(
    CHIFeature2IdentifierMap*       pMap,
    ChiFeature2RequestObjectOpsType opsType,
    const ChiFeature2Identifier*    pGlobalFID,
    ChiFeature2Identifier*          pLocalFID
    ) const
{
    CDKResult result = CDKResultSuccess;

    UINT8 type = 0;
    switch (opsType)
    {
        case ChiFeature2RequestObjectOpsType::RequestInput:
        case ChiFeature2RequestObjectOpsType::InputDependency:
            type = OpType0;
            break;
        case ChiFeature2RequestObjectOpsType::InputConfiguration:
            type = OpType1;
            break;
        case ChiFeature2RequestObjectOpsType::OutputConfiguration:
            type = OpType2;
            break;
        default:
            break;
    }

    if (TRUE == EnableFIDMapping)
    {
        if ((pMap->pSessionMap[pGlobalFID->session].sessionIndex != InvalidFeatureIndex) &&
            (pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].pipelineIndex != InvalidFeatureIndex) &&
            (pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].portMap[type].
                pPortIndexs[pGlobalFID->port] != InvalidFeatureIndex))
        {
            pLocalFID->session  = pMap->pSessionMap[pGlobalFID->session].sessionIndex;
            pLocalFID->pipeline = pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].pipelineIndex;
            pLocalFID->port     =
                pMap->pSessionMap[pGlobalFID->session].pPipelineMap[pGlobalFID->pipeline].portMap[type].
                pPortIndexs[pGlobalFID->port];
            CHX_LOG_VERBOSE("Mapped Feature Identifier [%d,%d,%d] --> [%d %d %d]",
                pGlobalFID->session,
                pGlobalFID->pipeline,
                pGlobalFID->port,
                pLocalFID->session,
                pLocalFID->pipeline,
                pLocalFID->port);
        }
        else
        {
            result = CDKResultENoSuch;
            CHX_LOG_VERBOSE("No such mapping, %s GlobalId[%d,%d,%d]",
                IdentifierString(),
                pGlobalFID->session,
                pGlobalFID->pipeline,
                pGlobalFID->port);
        }

    }
    else
    {
        pLocalFID->session  = pGlobalFID->session;
        pLocalFID->pipeline = pGlobalFID->pipeline;
        pLocalFID->port     = pGlobalFID->port;
    }
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RequestObject::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RequestObject::Dump(
    INT fd
    ) const
{
    CDK_UNREFERENCED_PARAM(fd);
    for (UINT8 requestIndex = 0; requestIndex < m_numRequests; requestIndex++)
    {
        ChiFeature2RequestInfo* pRequestInfo = m_pBatchOutputRequestInfo[requestIndex].pRequestInfo;
        if (NULL != pRequestInfo)
        {
            ChiFeature2StageInfo stageInfo;
            GetCurrentStageInfo(&stageInfo, requestIndex);

            CHX_LOG_DUMP("    FeatureRequest Handle:%p State:%s StageID:%d Process SequenceID :%d "
                "(Cur:%d, Next:%d, MaxSeq:%d, , MaxSession:%d, , MaxPipeline:%d, , MaxPort:%d)",
                m_hFroHandle,
                ChiFeature2RequestStateStrings[static_cast<UINT>(GetCurRequestState(0))],
                stageInfo.stageId,
                stageInfo.stageSequenceId,
                pRequestInfo->curProcessSequenceId,
                pRequestInfo->nextProcessSequenceId,
                m_maxInfo.maxSequence,
                m_maxInfo.maxSession,
                m_maxInfo.maxPipeline,
                m_maxInfo.maxPort);
            for (UINT cnt = 0; cnt < pRequestInfo->numRequestOutputs; cnt++)
            {
                if (FALSE == pRequestInfo->pOutputNotified[cnt])
                {
                    CHX_LOG_DUMP("    Output yet to be notified: GlobalFID[%d:%d:%d:%d]",
                        pRequestInfo->pRequestOutput[cnt].pPortDescriptor->globalId.session,
                        pRequestInfo->pRequestOutput[cnt].pPortDescriptor->globalId.pipeline,
                        pRequestInfo->pRequestOutput[cnt].pPortDescriptor->globalId.port,
                        pRequestInfo->pRequestOutput[cnt].pPortDescriptor->globalId.portDirectionType);
                }
                if (FALSE == pRequestInfo->pReleaseAcked[cnt])
                {
                    CHX_LOG_DUMP("    Release yet to be acked: GlobalFID[%d:%d:%d:%d]",
                        pRequestInfo->pRequestOutput[cnt].pPortDescriptor->globalId.session,
                        pRequestInfo->pRequestOutput[cnt].pPortDescriptor->globalId.pipeline,
                        pRequestInfo->pRequestOutput[cnt].pPortDescriptor->globalId.port,
                        pRequestInfo->pRequestOutput[cnt].pPortDescriptor->globalId.portDirectionType);
                }
            }
            if (InvalidProcessSequenceId != pRequestInfo->curProcessSequenceId)
            {
                UINT8 numDependencyConfig = 0;
                UINT8 numPipelines = 0;
                UINT8 numPorts = 0;
                for (INT32 seqId = 0; seqId <= pRequestInfo->curProcessSequenceId; seqId++)
                {
                    numDependencyConfig = pRequestInfo->ppProcessSequenceInfo[seqId]->numDependencyConfig;
                    for (INT32 sessionIndex = 0; sessionIndex < numDependencyConfig; sessionIndex++)
                    {
                        numPipelines = pRequestInfo->ppProcessSequenceInfo[seqId]->pDependencyConfig[sessionIndex].numPipelines;
                        if ((0 != pRequestInfo->ppProcessSequenceInfo[seqId]->inputBuffersDependenciesToBeStatisfied) ||
                            (0 != pRequestInfo->ppProcessSequenceInfo[seqId]->inputMetadataDependenciesToBeStatisfied))
                        {
                            CHX_LOG_DUMP("    [StageID:%d StageSequenceID:%d] InputNotification to be Satisfied:"
                                " Buffers:%d Metadata:%d",
                                pRequestInfo->ppProcessSequenceInfo[seqId]->stageId,
                                pRequestInfo->ppProcessSequenceInfo[seqId]->stageSequenceId,
                                pRequestInfo->ppProcessSequenceInfo[seqId]->inputBuffersDependenciesToBeStatisfied,
                                pRequestInfo->ppProcessSequenceInfo[seqId]->inputMetadataDependenciesToBeStatisfied);
                        }
                        for (INT32 pipelineIndex = 0; pipelineIndex < numPipelines; pipelineIndex++)
                        {
                            numPorts = pRequestInfo->ppProcessSequenceInfo[seqId]->
                                pDependencyConfig[sessionIndex].pPipelineInfo[pipelineIndex].numHandles;
                            if (ChiFeature2HandleType::DependencyConfigInfo ==
                                pRequestInfo->ppProcessSequenceInfo[seqId]->
                                pDependencyConfig[sessionIndex].pPipelineInfo[pipelineIndex].type)
                            {
                                ChiFeature2DependencyConfigInfo* pConfigInfo =
                                    static_cast<ChiFeature2DependencyConfigInfo*>
                                    (pRequestInfo->ppProcessSequenceInfo[seqId]->
                                        pDependencyConfig[sessionIndex].pPipelineInfo[pipelineIndex].handle);
                                if (NULL != pConfigInfo)
                                {
                                    DumpFIDMapping(fd, pRequestInfo->ppProcessSequenceInfo[seqId]->pMap);
                                    if (0 != pConfigInfo->inputConfig.num)
                                    {
                                        CHX_LOG_DUMP("    Input Config");
                                        DumpConfig(fd,
                                            sessionIndex,
                                            pipelineIndex,
                                            &pConfigInfo->inputConfig, pConfigInfo->inputConfig.num);
                                    }
                                    if (0 != pConfigInfo->outputConfig.num)
                                    {
                                        CHX_LOG_DUMP("    Output Config");
                                        DumpConfig(fd,
                                            sessionIndex,
                                            pipelineIndex,
                                            &pConfigInfo->inputConfig,
                                            pConfigInfo->inputConfig.num);
                                    }
                                    if (0 != pConfigInfo->numInputDependency)
                                    {
                                        CHX_LOG_DUMP("    Input Dependency");
                                        for (INT32 dependencyIndex = 0; dependencyIndex < pConfigInfo->numInputDependency;
                                            dependencyIndex++)
                                        {
                                            CHX_LOG_DUMP("        [%d]", dependencyIndex);
                                            if (0 != pConfigInfo->pInputDependency[dependencyIndex].num)
                                            {
                                                DumpConfig(fd,
                                                    sessionIndex,
                                                    pipelineIndex,
                                                    &pConfigInfo->pInputDependency[dependencyIndex],
                                                    pConfigInfo->pInputDependency[dependencyIndex].num);
                                            }
                                        }
                                    }
                                    if (0 != pConfigInfo->numRequestInput)
                                    {
                                        CHX_LOG_DUMP("    RequestInput");
                                        for (UINT8 index = 0; index < pConfigInfo->numRequestInput; index++)
                                        {
                                            CHX_LOG_DUMP("        [%d]", index);
                                            if (0 != pConfigInfo->pRequestInput[index].num)
                                            {
                                                DumpRequestInput(fd,
                                                    sessionIndex,
                                                    pipelineIndex,
                                                    &pConfigInfo->pRequestInput[index],
                                                    pConfigInfo->pRequestInput[index].num);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RequestObject::DumpConfig
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RequestObject::DumpConfig(
    INT                                fd,
    UINT8                              sessionIndex,
    UINT8                              pipelineIndex,
    ChiFeature2PortBufferMetadataInfo* pInfo,
    UINT8                              cnt
    ) const
{
    CDK_UNREFERENCED_PARAM(fd);
    if (NULL != pInfo)
    {
        for (UINT index = 0; index < cnt; index++)
        {
            CHX_LOG_DUMP("            [SessionID:%d PipelineID:%d] MetdataHandle:%p",
                         sessionIndex,
                         pipelineIndex,
                         pInfo[index].phMetadata[0]);
            for (UINT num = 0; num < pInfo[index].num; num++)
            {
                CHX_LOG_DUMP("                [PortID:%d PortType:%d]- TBH:%p Key:%" PRIu64,
                             num,
                             (NULL != pInfo[index].pPortDescriptor) ?
                             pInfo[index].pPortDescriptor[num].globalId.portDirectionType :
                                static_cast<ChiFeature2PortDirectionType>(-1),
                             (NULL != pInfo[index].phTargetBufferHandle) ?
                             pInfo[index].phTargetBufferHandle[num] : NULL,
                             (NULL != pInfo[index].pKey) ?
                             pInfo[index].pKey[num] : -1);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RequestObject::DumpRequestInput
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RequestObject::DumpRequestInput(
    INT                      fd,
    UINT8                    sessionIndex,
    UINT8                    pipelineIndex,
    ChiFeature2MetadataInfo* pInfo,
    UINT8                    cnt
    ) const
{
    CDK_UNREFERENCED_PARAM(fd);
    for (UINT index = 0; index < cnt; index++)
    {
        if (NULL != pInfo)
        {
            for (UINT num = 0; num < pInfo[index].num; num++)
            {
                CHX_LOG_DUMP("            FID:[SessionID:%d PipelineID:%d] - MetdataHandle[%d]:%p",
                             sessionIndex,
                             pipelineIndex,
                             num,
                             (NULL != pInfo[index].phMetadata) ?
                             pInfo[index].phMetadata[num] : NULL);
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RequestObject::DumpFIDMapping
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RequestObject::DumpFIDMapping(
    INT                             fd,
    CHIFeature2IdentifierMap*       pMap
    ) const
{
    CDK_UNREFERENCED_PARAM(fd);
    if (TRUE == EnableFIDMapping)
    {
        for (UINT8 sessionIndex = 0; sessionIndex < pMap->maxSession; sessionIndex++)
        {
            for (UINT8 pipelineIndex = 0; pipelineIndex < pMap->maxPipeline; pipelineIndex++)
            {
                for (UINT8 portIndex = 0; portIndex < pMap->maxPort; portIndex++)
                {
                    for (UINT type = 0; type < OpTypeMax; type++)
                    {
                        if ((pMap->pSessionMap[sessionIndex].sessionIndex != InvalidFeatureIndex) &&
                            (pMap->pSessionMap[sessionIndex].pPipelineMap[pipelineIndex].pipelineIndex !=
                                InvalidFeatureIndex) &&
                            (pMap->pSessionMap[sessionIndex].pPipelineMap[pipelineIndex].portMap[type].
                                pPortIndexs[portIndex] != InvalidFeatureIndex))
                        {
                            CHX_LOG_DUMP("    [FIDMapping Type:%d] Global[Session:%d Pipeline:%d Port:%d]->"
                                "Local[Global[Session:%d Pipeline:%d Port:%d]",
                                type,
                                sessionIndex, pipelineIndex, portIndex,
                                pMap->pSessionMap[sessionIndex].sessionIndex,
                                pMap->pSessionMap[sessionIndex].pPipelineMap[pipelineIndex].pipelineIndex,
                                pMap->pSessionMap[sessionIndex].pPipelineMap[pipelineIndex].
                                portMap[type].pPortIndexs[portIndex]);
                        }
                    }
                }
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RequestObject::IsPortUnique
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RequestObject::IsPortUnique(
    UINT8 port)
{
    BOOL isPortUnique = TRUE;
    for (UINT i = 0; i < m_outputPortProcessingInfo.size(); ++i)
    {
        if (port == m_outputPortProcessingInfo[i].port)
        {
            isPortUnique = FALSE;
            break;
        }
    }

    return isPortUnique;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RequestObject::GetAppRequestSetting
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RequestObject::GetAppRequestSetting(
    ChiMetadata** ppChiMetadata
    ) const
{
    *ppChiMetadata = m_pUsecaseRequestObj->GetAppSettings();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::SetProcessingCompleteForPort
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2RequestObject::SetProcessingCompleteForPort(
    ChiFeature2Identifier portIdentifier)
{
    for (UINT i = 0; i < m_outputPortProcessingInfo.size(); ++i)
    {
        if ((portIdentifier.port == m_outputPortProcessingInfo[i].port) &&
            (FALSE == m_outputPortProcessingInfo[i].isPortDoneProcessing))
        {
            CHX_LOG_VERBOSE("%s changing value of port:%d", IdentifierString(), m_outputPortProcessingInfo[i].port);
            m_outputPortProcessingInfo[i].isPortDoneProcessing = TRUE;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeature2RequestObject::AllOutputPortsProcessingComplete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL ChiFeature2RequestObject::AllOutputPortsProcessingComplete()
{
    BOOL portsDoneProcessing = TRUE;

    for (UINT i = 0; i < m_outputPortProcessingInfo.size(); ++i)
    {
        if (FALSE == m_outputPortProcessingInfo[i].isPortDoneProcessing)
        {
            portsDoneProcessing = FALSE;
            break;
        }
    }

    return portsDoneProcessing;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RequestObject::SetInterFeatureRequestPrivateData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::SetInterFeatureRequestPrivateData(
    UINT32  featureId,
    UINT32  cameraId,
    UINT32  instanceId,
    VOID*   pPrivateData)
{
    return m_pUsecaseRequestObj->SetInterFeatureRequestPrivateData(featureId, cameraId, instanceId, pPrivateData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RequestObject::GetInterFeatureRequestPrivateData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID* ChiFeature2RequestObject::GetInterFeatureRequestPrivateData(
    UINT32  featureId,
    UINT32  cameraId,
    UINT32  instanceId)
{
    return m_pUsecaseRequestObj->GetInterFeatureRequestPrivateData(featureId, cameraId, instanceId);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2RequestObject::RemoveInterFeatureRequestPrivateData
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2RequestObject::RemoveInterFeatureRequestPrivateData(
    UINT32  featureId,
    UINT32  cameraId,
    UINT32  instanceId)
{
    return m_pUsecaseRequestObj->RemoveInterFeatureRequestPrivateData(featureId, cameraId, instanceId);
}
