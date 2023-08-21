////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2graphmanager.cpp
/// @brief CHI feature2 graph selector implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chifeature2graphmanager.h"
#include "chifeature2graphselector.h"

// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphManager* ChiFeature2GraphManager::Create(
    FeatureGraphManagerConfig* pConfig)
{
    ChiFeature2GraphManager* pChiFeature2GraphManager = CHX_NEW ChiFeature2GraphManager;

    if (NULL != pConfig)
    {
        pChiFeature2GraphManager->Initialize(pConfig);
    }
    else
    {
        CHX_LOG_ERROR("Feature graph manager initialization failed");
        pChiFeature2GraphManager->Destroy();
        pChiFeature2GraphManager = NULL;
    }

    return pChiFeature2GraphManager;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphManager::Initialize(
    FeatureGraphManagerConfig * pConfig)
{
    CDKResult result = CDKResultSuccess;

    m_pEnabledFeatures.reserve(MaxFeaturesPerUsecase);

    m_pChiMetadataManager   = pConfig->pMetadataManager;

    CloneStreamConfig(pConfig->pCameraStreamConfig,
                      &m_cameraStreamConfig,
                      pConfig->pCameraStreamConfig->numStreams);

    m_pLogicalCameraInfo    = pConfig->pCameraInfo;
    m_pMetaFrameLock        = Mutex::Create();

    if (NULL == m_pMetaFrameLock)
    {
        result = CDKResultENoMemory;
        CHX_LOG_ERROR("Cannot allocate Map lock %p", m_pMetaFrameLock);
    }

    if (NULL != pConfig->pFeatureWrapperCallbacks)
    {
        result = RegisterFGMCallbacks(pConfig->pFeatureWrapperCallbacks);
    }

    if (CDKResultSuccess == result)
    {
        m_featureCallbacks.ChiFeature2ProcessMessage = ChiFeature2Graph::ProcessMessage;

        m_pFeatureGraphSelector = CreateFeatureGraphSelector(pConfig);

        if (NULL != m_pFeatureGraphSelector)
        {
            Feature2Config config      = {};
            config.pCameraInfo         = pConfig->pCameraInfo;
            config.pCameraStreamConfig = pConfig->pCameraStreamConfig;
            config.pSessionParams      = pConfig->pSessionParams;

            m_pFeaturePoolManager = ChiFeaturePoolManager::Create(pConfig, m_featureDescNameSet);

            ChiFeature2NameMap featureNameToOpsMap = m_pFeaturePoolManager->GetListofFeaturesSupported();

            FeatureGraphSelectorConfig selectorOutput = {};

            selectorOutput.featureNameToOpsMap = featureNameToOpsMap;

            if (NULL != m_feature2GraphSelectorOps.pGetFGDListForConfig)
            {
                m_pFeatureGraphDescMapForConfig = m_feature2GraphSelectorOps.pGetFGDListForConfig(
                    pConfig, selectorOutput, m_pFeatureGraphSelector);
            }

            CHIThreadManager::Create(&m_pFeatureThreadManager, "FeatureThreadManager");

            m_pChiFeature2GraphManagerPrivateData = static_cast<FeatureGraphManagerPrivateData*>(
                CHX_CALLOC(sizeof(FeatureGraphManagerPrivateData)));

            if (NULL != m_pChiFeature2GraphManagerPrivateData)
            {
                m_pChiFeature2GraphManagerPrivateData->pFeatureGraphManager             = this;
                m_pChiFeature2GraphManagerPrivateData->pChiFeature2UsecaseRequestObject = NULL;
            }
            else
            {
                CHX_LOG_ERROR("Allocation of private data failed!, callback private data not initialized");
            }

            ChiFeature2GraphManagerCallbacks featureGraphManagerNotifyCallbacks = { 0 };
            featureGraphManagerNotifyCallbacks.ChiFeatureGraphProcessMessage    =
                ChiFeature2GraphManager::ProcessFeatureGraphMessageCb;
            featureGraphManagerNotifyCallbacks.pPrivateCallbackData             = m_pChiFeature2GraphManagerPrivateData;

            featureGraphManagerNotifyCallbacks.ChiFeatureGraphFlush = ChiFeature2GraphManager::ProcessFeatureGraphFlushCb;


            if ((NULL != m_pFeaturePoolManager) && (NULL != m_pFeatureThreadManager))
            {
                for (auto mapIterator = m_pFeatureGraphDescMapForConfig.begin();
                     mapIterator != m_pFeatureGraphDescMapForConfig.end();)
                {
                    ChiFeature2FGDKeysForClonedMap descId = mapIterator->first;
                    FeaturePoolInputData featurePoolInputData;

                    featurePoolInputData.pCameraInfo                    = pConfig->pCameraInfo;
                    featurePoolInputData.pFeatureCallbacks              = &m_featureCallbacks;
                    featurePoolInputData.pCameraStreamConfig            = pConfig->pCameraStreamConfig;
                    featurePoolInputData.pUsecaseDescriptor             = pConfig->pUsecaseDescriptor;
                    featurePoolInputData.pFeatureGraphDesc              = mapIterator->second;
                    featurePoolInputData.pMetadataManager               = pConfig->pMetadataManager;
                    featurePoolInputData.physicalCameraId               = descId.cameraId;
                    featurePoolInputData.pThreadManager                 = m_pFeatureThreadManager;
                    featurePoolInputData.pFeatureGraphManagerCallbacks  = &featureGraphManagerNotifyCallbacks;
                    featurePoolInputData.featureFlags                   = mapIterator->first.featureFlags;

                    if (TRUE == descId.bMultiCamera)
                    {
                        featurePoolInputData.physicalCameraId    = pConfig->pCameraInfo->cameraId;
                        featurePoolInputData.bMultiCameraFeature = TRUE;
                    }
                    else
                    {
                        featurePoolInputData.physicalCameraId    = descId.cameraId;
                        featurePoolInputData.bMultiCameraFeature = FALSE;
                    }

                    CHX_LOG_INFO("graph:%s,physicaCamID:%d,isMultiCamera:%d,flags:%x",
                        featurePoolInputData.pFeatureGraphDesc->pFeatureGraphName,
                        featurePoolInputData.physicalCameraId,
                        featurePoolInputData.bMultiCameraFeature,
                        featurePoolInputData.featureFlags.value);

                    result = m_pFeaturePoolManager->CreateFeatureInstance(&featurePoolInputData, m_pEnabledFeatures);
                    if (CDKResultSuccess != result)
                    {
                        CHX_LOG_ERROR("Skipping feature graph %s", featurePoolInputData.pFeatureGraphDesc->pFeatureGraphName);
                        mapIterator = m_pFeatureGraphDescMapForConfig.erase(mapIterator);
                    }
                    else
                    {
                        mapIterator++;
                    }
                }
            }
            else
            {
                CHX_LOG_ERROR("Failed to create feature pool manager %p thread manager %p",
                    m_pFeaturePoolManager,
                    m_pFeatureThreadManager);
            }
        }
        else
        {
            CHX_LOG_ERROR("Failed to create Feature Graph Selector");
        }
    }
    else
    {
        CHX_LOG_ERROR("Registering callbacks failed");
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::LoadFeatureGraphSelectorOps
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphManager::LoadFeatureGraphSelectorOps(
    CHIFEATURE2GRAPHSELECTOROPS* pFGSOps)
{
    static const UINT32         MaxChiFeature2LibFiles    = 20;
    static const UINT32         NumPaths                  = 2;
    CDKResult                   result                    = CDKResultSuccess;
    UINT32                      fileCount                 = 0;
    const CHAR*                 pLibPaths[NumPaths]       = { 0 };
    OSLIBRARYHANDLE             handle                    = NULL;
    CHIFEATURE2GRAPHSELECTOROPS fgsOps                    = { 0 };
    UINT32                      fileIdx                   = 0;
    UINT32                      fileIdxBitra              = 0;
    const CHAR*                 pD                        = NULL;

    CHAR                        feature2LibFileName[MaxChiFeature2LibFiles][FILENAME_MAX] = { { 0 } };

    // start with oem path, if not found in oem path, then search the default path
    pLibPaths[0] = GetFeature2LibPath(ChiFeature2VendorType::OEM);
    pLibPaths[1] = GetFeature2LibPath(ChiFeature2VendorType::Default);

    for (UINT i = 0; i < NumPaths; i++)
    {
        // find files in the pattern of com.<vendor>.feature2.gs.so
        fileCount = CdkUtils::GetFilesFromPath(pLibPaths[i],
                                               FILENAME_MAX,
                                               &feature2LibFileName[0][0],
                                               "*",
                                               "feature2",
                                               "gs",
                                               "*",
                                               SharedLibraryExtension);

        if (2 < fileCount)
        {
            CHX_LOG_ERROR("Too many files match for graph selector");
            result = CDKResultEFailed;
        }

        if (CDKResultSuccess == result)
        {
            pD = CdkUtils::StrStr(&feature2LibFileName[0][0], "bitra");

            // pD is NULL if Bitra is not present in first file index
            if (pD != NULL)
            {
                fileIdx      = 1;
                fileIdxBitra = 0;
            }
            else
            {
                fileIdx      = 0;
                fileIdxBitra = 1;
            }
        }

        if (CDKResultSuccess == result)
        {
            const CHAR* pFileName = &feature2LibFileName[fileIdx][0];

            if ((ExtensionModule::GetInstance()->GetPlatformID() == CHISocIdSM6350) ||
                (ExtensionModule::GetInstance()->GetPlatformID() == CHISocIdSM7225))
            {
                pFileName = &feature2LibFileName[fileIdxBitra][0];
                handle    = ChxUtils::LibMap(pFileName);
            }
            else
            {
                handle = ChxUtils::LibMap(pFileName);
            }

            if (NULL != handle)
            {
                PCHIFEATURE2GRAPHSELECTOROPSENTRY funcPChiFeature2FGSOpsEntry =
                    reinterpret_cast<PCHIFEATURE2GRAPHSELECTOROPSENTRY>(
                        ChxUtils::LibGetAddr(handle, "ChiFeature2GraphSelectorOpsEntry"));

                if (NULL != funcPChiFeature2FGSOpsEntry)
                {
                    funcPChiFeature2FGSOpsEntry(&fgsOps);
                    CHX_LOG_CONFIG("Loading from:%s, size:%d, pCreateOps:%pK", pFileName, fgsOps.size, fgsOps.pCreate);
                    break;
                }
                else
                {
                    CHX_LOG_WARN("Symbol ChiFeature2FGSOpsEntry not found");
                }
            }

            if ((sizeof(CHIFEATURE2GRAPHSELECTOROPS) == fgsOps.size) &&
                (NULL != fgsOps.pCreate))
            {
                break;
            }
        }
    }

    if ((sizeof(CHIFEATURE2GRAPHSELECTOROPS) == fgsOps.size) &&
        (NULL != fgsOps.pCreate))
    {
        *pFGSOps = fgsOps;
    }
    else
    {
        CHX_LOG_ERROR("Fail to load fgs ops!");
        result = CDKResultEFailed;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::CreateFeatureGraphSelector
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphSelector* ChiFeature2GraphManager::CreateFeatureGraphSelector(
    FeatureGraphManagerConfig* pConfig)
{
    CDKResult                 result                    = CDKResultSuccess;
    ChiFeature2GraphSelector* pFeatureGraphSelector     = NULL;

    result = LoadFeatureGraphSelectorOps(&m_feature2GraphSelectorOps);

    if ((CDKResultSuccess == result) &&
        (sizeof(CHIFEATURE2GRAPHSELECTOROPS) == m_feature2GraphSelectorOps.size) &&
        (NULL != m_feature2GraphSelectorOps.pCreate))
    {
        pFeatureGraphSelector = m_feature2GraphSelectorOps.pCreate(pConfig, m_featureDescNameSet);
    }

    return pFeatureGraphSelector;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::ProcessUsecaseRequestObject
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphManager::ProcessUsecaseRequestObject(
    ChiFeature2UsecaseRequestObject* pRequest)
{
    ChiMetadata* pAppSettings     = pRequest->GetAppSettings();
    camera_metadata_entry_t entry = { 0 };
    INT32 status                  = -1;
    INT32 captureIntent           = -1;
    INT32 orientation             = -1;

    entry.tag = ANDROID_JPEG_ORIENTATION;
    status    = pAppSettings->FindTag(entry.tag, &entry);

    if (0 == status)
    {
        m_isGPURotationNeeded = TRUE;
        orientation = static_cast<INT32>(*(entry.data.i32));
        CHX_LOG_INFO("rotation value in degrees %d", orientation);
    }

    m_isGPURotationNeeded = FALSE;

    if (TRUE == ExtensionModule::GetInstance()->UseGPURotationUsecase())
    {
        m_isGPURotationNeeded = TRUE;
    }

    pRequest->SetGPURotationFlag(m_isGPURotationNeeded);

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::ExecuteProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphManager::ExecuteProcessRequest(
    ChiFeature2UsecaseRequestObject* pRequestObject)
{
    CDKResult             result                        = CDKResultEFailed;
    ChiFeature2GraphDesc* pFeatureGraphDescsForRequest  = NULL;
    ChiFeature2Graph*     pFeatureGraph                 = NULL;
    BOOL                  isFeatureGraphAvailableForReq = FALSE;

    if (NULL != pRequestObject)
    {
        result = ProcessUsecaseRequestObject(pRequestObject);

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Failed to process URO");
        }

        if (NULL != m_feature2GraphSelectorOps.pSelectFGD)
        {
            m_pMetaFrameLock->Lock();
            pFeatureGraphDescsForRequest = m_feature2GraphSelectorOps.pSelectFGD(pRequestObject,
                                                                                 m_metadataFrameNumberMap,
                                                                                 m_pFeatureGraphSelector);
            m_pMetaFrameLock->Unlock();
        }

        if (NULL != pRequestObject)
        {
            pFeatureGraph = m_pFeatureGraphFrameNumMap[pRequestObject->GetAppFrameNumber()];
        }

        if ((NULL == pFeatureGraph) && (NULL != pFeatureGraphDescsForRequest))
        {
            pFeatureGraph = CreateFeatureGraph(pRequestObject,
                                               pFeatureGraphDescsForRequest);
        }
        else
        {
            isFeatureGraphAvailableForReq = TRUE;
        }

        if (NULL != pFeatureGraph)
        {
            result = pFeatureGraph->ExecuteProcessRequest(pRequestObject);

            if (FALSE == isFeatureGraphAvailableForReq)
            {
                m_pFeatureGraphFrameNumMap[pRequestObject->GetAppFrameNumber()] = pFeatureGraph;
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::OnFeatureObjComplete
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphManager::OnFeatureObjComplete(
    ChiFeature2UsecaseRequestObject* pUsecaseRequestObj)
{
    if (NULL != pUsecaseRequestObj)
    {
        if (ChiFeature2UsecaseRequestObjectState::Invalid == pUsecaseRequestObj->GetRequestState())
        {
            UINT32 frameNumber              = pUsecaseRequestObj->GetAppFrameNumber();
            ChiFeature2Graph* pFeatureGraph = m_pFeatureGraphFrameNumMap.at(frameNumber);

            if (NULL != pFeatureGraph)
            {
                pFeatureGraph->Destroy();
                m_pFeatureGraphFrameNumMap.erase(frameNumber);
            }

            FeatureGraphManagerPrivateData* pChiFGMPrivateData = m_pChiFGMPrivateDataMap.at(frameNumber);
            if (NULL != pChiFGMPrivateData)
            {
                CHX_DELETE(pChiFGMPrivateData);
                m_pChiFGMPrivateDataMap.erase(frameNumber);
            }
        }
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::Dump
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphManager::Dump(
    INT                                 fd,
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj
    ) const
{
    if (NULL != pUsecaseRequestObj)
    {
        ChiFeature2Graph* pFeatureGraph = m_pFeatureGraphFrameNumMap.at(pUsecaseRequestObj->GetAppFrameNumber());
        if (NULL != pFeatureGraph)
        {
            pFeatureGraph->Dump(fd);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::ProcessFeatureGraphResultCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphManager::ProcessFeatureGraphResultCb(
    CHICAPTURERESULT*                   pResult,
    ChiFeature2UsecaseRequestObject*    pUsecaseRequestObj,
    VOID*                               pPrivateCallbackData)
{
    FeatureGraphManagerPrivateData* pPrivateData  = static_cast<FeatureGraphManagerPrivateData*>(pPrivateCallbackData);
    ChiFeature2GraphManager* pFeatureGraphManager = static_cast<ChiFeature2GraphManager*>(pPrivateData->pFeatureGraphManager);

    // filter out the framework streams
    CHICAPTUREREQUEST* pChiRequest = pUsecaseRequestObj->GetChiCaptureRequest();


    if ((pResult->numOutputBuffers == 0) && (NULL != pResult->pOutputMetadata))
    {
        UINT mapIndex = pResult->frameworkFrameNum % MaxRequestQueueDepth;
        ChiMetadata* pChiMetadata =
            pFeatureGraphManager->m_pChiMetadataManager->GetMetadataFromHandle(pResult->pOutputMetadata);

        pFeatureGraphManager->m_pMetaFrameLock->Lock();
        pFeatureGraphManager->m_metadataFrameNumberMap[mapIndex] = pChiMetadata;
        pFeatureGraphManager->m_pMetaFrameLock->Unlock();

        CHICAPTURERESULT tempResult   = {};
        tempResult.frameworkFrameNum  = pResult->frameworkFrameNum;
        tempResult.numOutputBuffers   = 0;
        tempResult.numPartialMetadata = pResult->numPartialMetadata;
        tempResult.pOutputBuffers     = NULL;
        tempResult.pOutputMetadata    = pResult->pOutputMetadata;
        tempResult.pPrivData          = pResult->pPrivData;

        pFeatureGraphManager->m_featureWrapperCallbacks.
            ProcessCaptureResultCbToUsecase(&tempResult,
                                            pFeatureGraphManager->m_featureWrapperCallbacks.pPrivateCallbackData);
    }
    else
    {
        UINT                numOutput       = 0;
        CHICAPTURERESULT    tempResult      = {0};
        CHISTREAMBUFFER*    pOutputBuffers  = static_cast<CHISTREAMBUFFER*>(
            CHX_CALLOC(sizeof(CHISTREAMBUFFER) * pResult->numOutputBuffers));
        if (NULL == pOutputBuffers)
        {
            CHX_LOG_ERROR("Out of memory: pOutputBuffers is NULL");
        }
        else
        {
            for (UINT outputIdx = 0; outputIdx < pResult->numOutputBuffers; outputIdx++)
            {
                CHISTREAM* pOutputStream = pResult->pOutputBuffers[outputIdx].pStream;
                for (UINT streamIdx = 0; streamIdx < pChiRequest->numOutputs; streamIdx++)
                {
                    CHISTREAM* pRequestStream = pChiRequest->pOutputBuffers[streamIdx].pStream;
                    if (pRequestStream == pOutputStream)
                    {
                        tempResult.frameworkFrameNum  = pResult->frameworkFrameNum;
                        tempResult.numOutputBuffers   = 1;
                        tempResult.numPartialMetadata = pResult->numPartialMetadata;
                        pOutputBuffers[numOutput++]   = pResult->pOutputBuffers[outputIdx];
                        tempResult.pPrivData          = pResult->pPrivData;
                        if (NULL != pResult->pOutputMetadata)
                        {
                            tempResult.pOutputMetadata = pResult->pOutputMetadata;
                        }
                    }
                }
            }

            if (0 < pResult->numOutputBuffers)
            {
                tempResult.pOutputBuffers = &pOutputBuffers[0];
                pFeatureGraphManager->m_featureWrapperCallbacks.
                    ProcessCaptureResultCbToUsecase(&tempResult,
                                                    pFeatureGraphManager->
                                                    m_featureWrapperCallbacks.pPrivateCallbackData);
            }

            CHX_FREE(pOutputBuffers);
            pOutputBuffers = NULL;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::ProcessFeatureGraphPartialCaptureResultCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphManager::ProcessFeatureGraphPartialCaptureResultCb(
    CHIPARTIALCAPTURERESULT* pCaptureResult,
    VOID*                    pPrivateCallbackData)
{
    FeatureGraphManagerPrivateData* pPrivateData = static_cast<FeatureGraphManagerPrivateData*>(pPrivateCallbackData);
    ChiFeature2GraphManager* pFeatureGraphManager = static_cast<ChiFeature2GraphManager*>(pPrivateData->pFeatureGraphManager);

    pFeatureGraphManager->m_featureWrapperCallbacks.
        ProcessPartialCaptureResultCbToUsecase(pCaptureResult,
                                               pFeatureGraphManager->m_featureWrapperCallbacks.pPrivateCallbackData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::ProcessFeatureGraphMessageCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphManager::ProcessFeatureGraphMessageCb(
    const ChiFeature2Messages*  pMessageDesc,
    VOID*                       pPrivateCallbackData)
{
    CDKResult result = CDKResultSuccess;
    FeatureGraphManagerPrivateData* pPrivateData  = static_cast<FeatureGraphManagerPrivateData*>(pPrivateCallbackData);
    ChiFeature2GraphManager* pFeatureGraphManager = static_cast<ChiFeature2GraphManager*>(pPrivateData->pFeatureGraphManager);

    result = pFeatureGraphManager->m_featureWrapperCallbacks.
        NotifyMessageToUsecase(pMessageDesc,
                               pFeatureGraphManager->m_featureWrapperCallbacks.pPrivateCallbackData);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::MapOutputPortToOutputStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphManager::MapOutputPortToOutputStream(
    ChiFeature2GraphDesc*                        pFeatureGraphDesc,
    std::vector<ChiStream*>                      &outputStreams,
    std::vector<ChiFeature2PortIdToChiStreamMap> &globalPortIdToChiStreamMapForOutput)
{
    UINT8 numFeatureInstances = pFeatureGraphDesc->numFeatureInstances;
    BOOL  mapFound            = FALSE;
    UINT numOfStreamstoMap    = 0;
    BOOL is4KYUVOut           = FALSE;

    CHX_LOG_VERBOSE("Number of framework streams to map %zu", outputStreams.size());

    for (auto StreamIterator = outputStreams.begin(); StreamIterator != outputStreams.end(); StreamIterator++)
    {
        if (TRUE == Is4KYUVOut(*StreamIterator))
        {
            is4KYUVOut = TRUE;
        }
    }

    for (UINT8 i = 0; i < numFeatureInstances; i++)
    {
        const ChiFeature2InstanceDesc pFeatureInstance = pFeatureGraphDesc->pFeatureInstances[i];

        for (UINT8 sinkIdx = 0; sinkIdx < pFeatureGraphDesc->numExtSinkLinks; sinkIdx++)
        {
            mapFound = FALSE;
            ChiFeature2GlobalPortInstanceId featureGraphGlobalId = pFeatureGraphDesc->pExtSinkLinks[sinkIdx].portId;
            if ((featureGraphGlobalId.featureId == pFeatureInstance.pFeatureDesc->featureId) &&
                (featureGraphGlobalId.instanceProps == *pFeatureInstance.pInstanceProps))
            {
                const CHAR* pFeatureName = pFeatureInstance.pFeatureDesc->pFeatureName;
                for (UINT8 sessionIdx = 0; sessionIdx < pFeatureInstance.pFeatureDesc->numSessions; sessionIdx++)
                {
                    const ChiFeature2SessionDescriptor session = pFeatureInstance.pFeatureDesc->pSession[sessionIdx];
                    for (UINT8 pipelineIdx = 0; pipelineIdx < session.numPipelines; pipelineIdx++)
                    {
                        const ChiFeature2PipelineDescriptor pipeline = session.pPipeline[pipelineIdx];
                        for (UINT8 OutputPortIdx = 0; OutputPortIdx < pipeline.numOutputPorts; OutputPortIdx++)
                        {
                            ChiFeature2Identifier featureGlobalId = pipeline.pOutputPortDescriptor[OutputPortIdx].globalId;
                            const CHAR* pPortName = pipeline.pOutputPortDescriptor[OutputPortIdx].pPortName;
                            if ((ChiFeature2PortType::MetaData == featureGlobalId.portType) &&
                                (NULL == pipeline.pOutputPortDescriptor[OutputPortIdx].pTargetDescriptor))
                            {
                                continue;
                            }

                            for (UINT streamIdx = 0; streamIdx < outputStreams.size(); streamIdx++)
                            {
                                ChiFeature2PortIdToChiStreamMap map = {};
                                map.portId                          = featureGraphGlobalId;
                                map.pChiStream                      = outputStreams[streamIdx];
                                UINT32 physicalCameraIndex          = featureGraphGlobalId.instanceProps.cameraId;
                                UINT32 physicalCameraId             =
                                    m_pLogicalCameraInfo->ppDeviceInfo[physicalCameraIndex]->cameraId;
                                ChiStream* pChiStream               = outputStreams[streamIdx];

                                if ((featureGraphGlobalId.portId == featureGlobalId) &&
                                    (NULL != pipeline.pOutputPortDescriptor[OutputPortIdx].pTargetDescriptor) &&
                                    ((featureGraphGlobalId.featureId != static_cast<UINT32>(ChiFeature2Type::STUB_RT)) &&
                                     (featureGraphGlobalId.featureId != static_cast<UINT32>(ChiFeature2Type::STUB_B2Y))) &&
                                    ((NULL == pChiStream->physicalCameraId) ||
                                     ((NULL != pChiStream->physicalCameraId) &&
                                      ((0 == strlen(pChiStream->physicalCameraId)) ||
                                       (ChxUtils::GetCameraIdFromStream(pChiStream) ==
                                           physicalCameraId)))))
                                {
                                    const CHAR* pTargetName =
                                        pipeline.pOutputPortDescriptor[OutputPortIdx].pTargetDescriptor->pTargetName;
                                    if (pChiStream->format == ChiStreamFormatYCbCr420_888)
                                    {
                                        BOOL yuvTarget = ((!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_YUV_OUT")) ||
                                            (!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_YUV_HAL")) ||
                                            (!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_YUV"))     ||
                                            (!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_OUT")) ) ?
                                            TRUE : FALSE;

                                        if ((TRUE == yuvTarget) ||
                                            // For yuv cb using TARGET_BUFFER_VIDEO
                                            ((!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_VIDEO"))  &&
                                            (TRUE != is4KYUVOut))                                     ||
                                            (!CdkUtils::StrCmp(pTargetName, "TARGET_FUSION_SNAPSHOT") &&
                                            (NULL == pChiStream->physicalCameraId)))
                                        {
                                            globalPortIdToChiStreamMapForOutput.push_back(map);
                                            mapFound = TRUE;
                                        }
                                    }
                                    else if (((pChiStream->format == ChiStreamFormatRaw10) ||
                                              (pChiStream->format == ChiStreamFormatRaw16)) &&
                                              (!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_RAW_OUT")))
                                    {
                                        globalPortIdToChiStreamMapForOutput.push_back(map);
                                        mapFound = TRUE;
                                    }
                                    else if (pChiStream->format == ChiStreamFormatImplDefined)
                                    {
                                        if (IsVideoStream(pChiStream->grallocUsage))
                                        {
                                            if (((pChiStream->dataspace != DataspaceHEIF) &&
                                                 (!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_VIDEO"))) ||
                                                ((pChiStream->dataspace == DataspaceHEIF) &&
                                                 (!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_HEIC_YUV"))))
                                            {
                                                globalPortIdToChiStreamMapForOutput.push_back(map);
                                                mapFound = TRUE;
                                            }
                                        }
                                        else if ((!IsVideoStream(pChiStream->grallocUsage)) &&
                                                 (!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_DISPLAY")))
                                        {
                                            globalPortIdToChiStreamMapForOutput.push_back(map);
                                            mapFound = TRUE;
                                        }
                                    }
                                    else if (pChiStream->format == ChiStreamFormatBlob)
                                    {
                                        if (((pChiStream->dataspace == DataspaceJPEGAPPSegments) &&
                                             !CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_HEIC_BLOB")) ||
                                            ((pChiStream->dataspace != DataspaceJPEGAPPSegments) &&
                                             !CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_SNAPSHOT") &&
                                             (FALSE == m_isGPURotationNeeded)) ||
                                             ((pChiStream->dataspace != DataspaceJPEGAPPSegments) &&
                                              !CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_SNAPSHOT2") &&
                                              (TRUE == m_isGPURotationNeeded)))
                                        {
                                            globalPortIdToChiStreamMapForOutput.push_back(map);
                                            mapFound = TRUE;
                                        }
                                    }

                                    if (TRUE == mapFound)
                                    {
                                        numOfStreamstoMap++;
                                        CHX_LOG_VERBOSE("%s: Map output fwk stream %p with format %x to target port %s:"
                                                        "{%d: %d, %d, %d}",
                                                        pFeatureName, pChiStream, pChiStream->format, pPortName,
                                                        map.portId.featureId,
                                                        map.portId.portId.session,
                                                        map.portId.portId.pipeline,
                                                        map.portId.portId.port);

                                        CHX_LOG_VERBOSE("map.pChiStream:%p,cameraid:%s,instanceid:%d, port:%d,%d",
                                                        map.pChiStream, outputStreams[streamIdx]->physicalCameraId,
                                                        featureGraphGlobalId.instanceProps.instanceId,
                                                        featureGlobalId.port,
                                                        featureGlobalId.portDirectionType);

                                        // Return if all streams are mapped
                                        if (numOfStreamstoMap == outputStreams.size())
                                        {
                                            return;
                                        }

                                        break;
                                    }
                                }
                                else
                                {
                                    if ((pChiStream->streamType == ChiStreamTypeOutput) &&
                                        (pChiStream->format == ChiStreamFormatImplDefined) &&
                                        (!IsVideoStream(pChiStream->grallocUsage)) &&
                                        (!CdkUtils::StrCmp(pipeline.pOutputPortDescriptor[OutputPortIdx].pPortName,
                                        "Display_Out") ||
                                        !CdkUtils::StrCmp(pipeline.pOutputPortDescriptor[OutputPortIdx].pPortName,
                                        "Raw_Out") ||
                                        !CdkUtils::StrCmp(pipeline.pOutputPortDescriptor[OutputPortIdx].pPortName,
                                        "Fd_Out") ||
                                        !CdkUtils::StrCmp(pipeline.pOutputPortDescriptor[OutputPortIdx].pPortName,
                                        "Video_Out") ||
                                        !CdkUtils::StrCmp(pipeline.pOutputPortDescriptor[OutputPortIdx].pPortName,
                                        "Zsl") ||
                                        !CdkUtils::StrCmp(pipeline.pOutputPortDescriptor[OutputPortIdx].pPortName,
                                        "rt_metadata_out")) &&
                                        (featureGraphGlobalId.portId == featureGlobalId))
                                    {
                                        globalPortIdToChiStreamMapForOutput.push_back(map);
                                        mapFound = TRUE;
                                    }
                                }
                                if (TRUE == mapFound)
                                {
                                    CHX_LOG_VERBOSE("%s: Map output fwk stream %p with format %x to target port"
                                                    "%s: {%d: %d, %d, %d}",
                                                    pFeatureName, pChiStream, pChiStream->format,
                                                    pPortName,
                                                    map.portId.featureId,
                                                    map.portId.portId.session,
                                                    map.portId.portId.pipeline,
                                                    map.portId.portId.port);
                                    break;
                                }
                            } // streamIdx
                            if (TRUE == mapFound)
                            {
                                break;
                            }
                        } // OutputPortIdx
                        if (TRUE == mapFound)
                        {
                            break;
                        }
                    } // pipelineIdx
                    if (TRUE == mapFound)
                    {
                        break;
                    }
                } // sessionIdx
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::CreateFeatureGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Graph* ChiFeature2GraphManager::CreateFeatureGraph(
    ChiFeature2UsecaseRequestObject* pRequestObject,
    ChiFeature2GraphDesc*            pFeatureGraphDescForRequest)
{
    CDKResult result = CDKResultSuccess;

    ChiStreamConfigInfo              streamConfig                 = {};
    ChiFeature2Graph*                pFeatureGraph                = NULL;
    UINT32                           frameIdx                     = pRequestObject->GetAppFrameNumber() % MaxRequestQueueDepth;

    m_featureGraphCallbacks[frameIdx].ChiFeatureGraphProcessResult = ChiFeature2GraphManager::ProcessFeatureGraphResultCb;
    m_featureGraphCallbacks[frameIdx].ChiFeatureGraphProcessPartialResult =
        ChiFeature2GraphManager::ProcessFeatureGraphPartialCaptureResultCb;
    m_featureGraphCallbacks[frameIdx].ChiFeatureGraphProcessMessage = ChiFeature2GraphManager::ProcessFeatureGraphMessageCb;
    m_featureGraphCallbacks[frameIdx].ChiFeatureGraphFlush          = ChiFeature2GraphManager::ProcessFeatureGraphFlushCb;


    FeatureGraphManagerPrivateData* pChiFeature2GraphManagerPrivateData   = CHX_NEW FeatureGraphManagerPrivateData();
    if (NULL != pChiFeature2GraphManagerPrivateData)
    {
        pChiFeature2GraphManagerPrivateData->pFeatureGraphManager             = this;
        pChiFeature2GraphManagerPrivateData->pChiFeature2UsecaseRequestObject = pRequestObject;
    }
    m_featureGraphCallbacks[frameIdx].pPrivateCallbackData                = pChiFeature2GraphManagerPrivateData;
    m_featureGraphCreateInputInfo[frameIdx].pFeatureGraphManagerCallbacks = &m_featureGraphCallbacks[frameIdx];

    m_pChiFGMPrivateDataMap.insert({ pRequestObject->GetAppFrameNumber(), pChiFeature2GraphManagerPrivateData });

    std::vector<ChiFeature2PortIdToChiStreamMap> globalPortIdToChiStreamMapForInput;
    std::vector<ChiFeature2PortIdToChiStreamMap> globalPortIdToChiStreamMapForOutput;

    std::vector<ChiStream*> inputStreams  = pRequestObject->GetInputSrcStreams();
    std::vector<ChiStream*> outputStreams = pRequestObject->GetOutputSinkStreams();

    MapInputPortToInputStream(pFeatureGraphDescForRequest,
                              inputStreams,
                              globalPortIdToChiStreamMapForInput);

    MapOutputPortToOutputStream(pFeatureGraphDescForRequest,
                                outputStreams,
                                globalPortIdToChiStreamMapForOutput);

    for (UINT i =0; i< outputStreams.size(); i++)
    {
        if (IsVideoStream(outputStreams[i]->grallocUsage))
        {
            CHX_LOG_INFO("Video stream in framework");
        }
    }
    std::vector<ChiFeature2InstanceRequestInfo> featureInstanceReqInfoList;

    m_pFeaturePoolManager->SelectFeaturesForGraph(pFeatureGraphDescForRequest, featureInstanceReqInfoList);

    if (NULL != m_feature2GraphSelectorOps.pAddCustomHints)
    {
        m_pMetaFrameLock->Lock();
        if (m_metadataFrameNumberMap.begin() != m_metadataFrameNumberMap.end())
        {
            pRequestObject->SetMetadataMap(m_metadataFrameNumberMap);
        }

        m_pMetaFrameLock->Unlock();

        m_feature2GraphSelectorOps.pAddCustomHints(pRequestObject,
                                                   featureInstanceReqInfoList,
                                                   m_pFeatureGraphSelector);
    }

    m_featureGraphCreateInputInfo[frameIdx].featureInstanceReqInfoList  = featureInstanceReqInfoList;
    m_featureGraphCreateInputInfo[frameIdx].pFeatureGraphDesc           = pFeatureGraphDescForRequest;

    m_featureGraphCreateInputInfo[frameIdx].extSrcPortIdToChiStreamMap  = globalPortIdToChiStreamMapForInput;
    m_featureGraphCreateInputInfo[frameIdx].extSinkPortIdToChiStreamMap = globalPortIdToChiStreamMapForOutput;
    m_featureGraphCreateInputInfo[frameIdx].pUsecaseRequestObject       = pRequestObject;

    pFeatureGraph = ChiFeature2Graph::Create(&m_featureGraphCreateInputInfo[frameIdx]);
    if (NULL == pFeatureGraph)
    {
        CHX_LOG_ERROR("Failed to create Feature Graph");
    }

    return pFeatureGraph;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::MapInputPortToInputStream
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphManager::MapInputPortToInputStream(
    ChiFeature2GraphDesc*                        pFeatureGraphDesc,
    std::vector<ChiStream*>                      &inputStreams,
    std::vector<ChiFeature2PortIdToChiStreamMap> &globalPortIdToChiStreamMapForInput)
{
    UINT8 numFeatureInstances = pFeatureGraphDesc->numFeatureInstances;
    BOOL  isMultiCamera       = (m_pLogicalCameraInfo->numPhysicalCameras <= 1) ? FALSE : TRUE;

    for (UINT8 i = 0; i < numFeatureInstances; i++)
    {
        const ChiFeature2InstanceDesc pFeatureInstance = pFeatureGraphDesc->pFeatureInstances[i];

        for (UINT8 srcIdx = 0; srcIdx < pFeatureGraphDesc->numExtSrcLinks; srcIdx++)
        {
            ChiFeature2GlobalPortInstanceId featureGraphGlobalId = pFeatureGraphDesc->pExtSrcLinks[srcIdx].portId;
            if ((featureGraphGlobalId.featureId == pFeatureInstance.pFeatureDesc->featureId) &&
                (featureGraphGlobalId.instanceProps == *pFeatureInstance.pInstanceProps))
            {
                for (UINT8 sessionIdx = 0; sessionIdx < pFeatureInstance.pFeatureDesc->numSessions; sessionIdx++)
                {
                    const ChiFeature2SessionDescriptor session = pFeatureInstance.pFeatureDesc->pSession[sessionIdx];
                    for (UINT8 pipelineIdx = 0; pipelineIdx < session.numPipelines; pipelineIdx++)
                    {
                        const ChiFeature2PipelineDescriptor pipeline = session.pPipeline[pipelineIdx];
                        for (UINT8 InputPortIdx = 0; InputPortIdx < pipeline.numInputPorts; InputPortIdx++)
                        {
                            ChiFeature2Identifier featureGlobalId = pipeline.pInputPortDescriptor[InputPortIdx].globalId;
                            // For External source, metadata has target stream associated. Take care of it.
                            // For others metadata port there is no target associated so continue
                            if ((FALSE == isMultiCamera) && (ChiFeature2PortType::MetaData == featureGlobalId.portType))
                            {
                                continue;
                            }
                            for (UINT streamIdx = 0; streamIdx < inputStreams.size(); streamIdx++)
                            {
                                ChiFeature2PortIdToChiStreamMap map = {};
                                ChiStream*  pChiStream              = inputStreams[streamIdx];
                                const CHAR* pTargetName             = NULL;
                                if (NULL != pipeline.pInputPortDescriptor[InputPortIdx].pTargetDescriptor)
                                {
                                    pTargetName =
                                        pipeline.pInputPortDescriptor[InputPortIdx].pTargetDescriptor->pTargetName;
                                }

                                map.portId     = featureGraphGlobalId;
                                map.pChiStream = inputStreams[streamIdx];

                                UINT8  physicalCameraIndex = featureGraphGlobalId.instanceProps.cameraId;
                                UINT32 physicalCameraId    =
                                    m_pLogicalCameraInfo->ppDeviceInfo[physicalCameraIndex]->cameraId;
                                if ((FALSE == isMultiCamera) || ((TRUE == isMultiCamera) &&
                                    ((NULL != inputStreams[streamIdx]->physicalCameraId) &&
                                    (ChxUtils::GetCameraIdFromStream(inputStreams[streamIdx]) ==
                                        physicalCameraId))))
                                {
                                    if ((featureGraphGlobalId.portId == featureGlobalId))
                                    {
                                        if (((NULL == pTargetName)                                  ||
                                            ((NULL != pTargetName)                                  &&
                                            (!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_RAW")))) &&
                                             ((pChiStream->format == ChiStreamFormatRaw16)          ||
                                              (pChiStream->format == ChiStreamFormatRaw10)          ||
                                              (pChiStream->format == ChiStreamFormatRawOpaque)))
                                        {
                                            CHX_LOG_INFO("setting stream %p to portid = %d",
                                                    map.pChiStream, map.portId.portId.port);
                                            globalPortIdToChiStreamMapForInput.push_back(map);
                                            break;
                                        }
                                        else if ((NULL != pTargetName)                         &&
                                                 ((pChiStream->format == ChiStreamFormatRaw10) &&
                                                 (!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_CUSTOM_YUV"))))
                                        {
                                            globalPortIdToChiStreamMapForInput.push_back(map);
                                            break;
                                        }
                                        else if ((NULL != pTargetName)                         &&
                                                 ((pChiStream->format == ChiStreamFormatYCbCr420_888) &&
                                                 (!CdkUtils::StrCmp(pTargetName, "TARGET_BUFFER_FD"))))
                                        {
                                            CHX_LOG_INFO("setting stream %p to portid = %d",
                                                    map.pChiStream, map.portId.portId.port);
                                            globalPortIdToChiStreamMapForInput.push_back(map);
                                            break;
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
// ChiFeature2GraphManager::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeature2GraphManager::Flush()
{
    BOOL isSynchronousFlush = TRUE;
    CHX_LOG_INFO("isSynchronousFlush = %d", isSynchronousFlush);

    return m_pFeaturePoolManager->Flush(isSynchronousFlush);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::ProcessFeatureGraphFlushCb
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphManager::ProcessFeatureGraphFlushCb(
    UINT32      featureId,
    CDKResult   result,
    VOID*       pPrivateCallbackData)
{
    if (CDKResultSuccess != result)
    {
        CHX_LOG_WARN("Flush Error for featureId: %u, result: %u", featureId, result);
    }

    FeatureGraphManagerPrivateData* pPrivateData         = static_cast<FeatureGraphManagerPrivateData*>(pPrivateCallbackData);
    ChiFeature2GraphManager*        pFeatureGraphManager = static_cast<ChiFeature2GraphManager*>(
                                                               pPrivateData->pFeatureGraphManager);

    pFeatureGraphManager->m_pFeaturePoolManager->SignalFlush(featureId);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeature2GraphManager::Destroy()
{
    if ((NULL != m_pFeatureGraphSelector) && (NULL != m_feature2GraphSelectorOps.pDestroy))
    {
        m_feature2GraphSelectorOps.pDestroy(m_pFeatureGraphSelector);
    }

    if (NULL != m_pFeaturePoolManager)
    {
        m_pFeaturePoolManager->NotifyGraphManagerDestroyInProgress();
    }

    for (auto featureGraphIdx = m_pFeatureGraphFrameNumMap.begin(); featureGraphIdx != m_pFeatureGraphFrameNumMap.end();
         featureGraphIdx++)
    {
        ChiFeature2Graph* pFeautureGraph = featureGraphIdx->second;
        if (NULL != pFeautureGraph)
        {
            pFeautureGraph->Destroy();
            pFeautureGraph = NULL;
        }
    }
    m_pFeatureGraphFrameNumMap.clear();

    if (NULL != m_pFeaturePoolManager)
    {
        m_pFeaturePoolManager->Destroy();
        m_pFeaturePoolManager = NULL;
    }

    for (auto fgmPrivateIdx = m_pChiFGMPrivateDataMap.begin(); fgmPrivateIdx != m_pChiFGMPrivateDataMap.end();
         fgmPrivateIdx++)
    {
        CHX_DELETE(fgmPrivateIdx->second);
    }

    m_pChiFGMPrivateDataMap.clear();
    m_metadataFrameNumberMap.clear();


    if (NULL != m_pFeatureThreadManager)
    {
        m_pFeatureThreadManager->Destroy();
        m_pFeatureThreadManager = NULL;
    }

    if (NULL != m_cameraStreamConfig.pChiStreams)
    {
        CHX_FREE(m_cameraStreamConfig.pChiStreams);
        m_cameraStreamConfig.pChiStreams = NULL;
    }

    if (NULL != m_pChiFeature2GraphManagerPrivateData)
    {
        CHX_FREE(m_pChiFeature2GraphManagerPrivateData);
        m_pChiFeature2GraphManagerPrivateData = NULL;
    }

    if (NULL != m_pMetaFrameLock)
    {
        m_pMetaFrameLock->Destroy();
        m_pMetaFrameLock = NULL;
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeature2GraphManager::~ChiFeature2GraphManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2GraphManager::~ChiFeature2GraphManager()
{

}
