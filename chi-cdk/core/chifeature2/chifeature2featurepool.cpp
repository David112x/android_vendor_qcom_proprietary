////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2019-2020 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  chifeature2featurepool.cpp
/// @brief CHI feature2 pool manager implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "chibinarylog.h"
#include "chifeature2graphmanager.h"
#include "chifeature2featurepool.h"
#include "chifeature2descriptors.h"

// NOWHINE FILE CP006:  Need whiner update: std::vector allowed in exceptional cases

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeaturePoolManager::Initialize(
    VOID* pConfig)
{
    CDKResult result = CDKResultSuccess;

    m_pFeatureList.reserve(MaxFeaturesPerUsecase * static_cast<UINT>(ChiFeature2Type::MaxFeatureList));
    m_pFeatureList.clear();
    m_pEnabledFeaturesMap.clear();
    m_featureNameToOpsMap.clear();

    m_pStreams.reserve(MaxNumOfStreamsPerFeature);

    m_pConfig = pConfig;

    m_qcfaUsecase  = FALSE;

    // start with OEM features, and skip default features if OEM already has the feature
    result = ProbeChiFeature2Features(GetFeature2LibPath(ChiFeature2VendorType::OEM));

    m_pFlushMutex        = Mutex::Create();
    m_pFlushCountMutex   = Mutex::Create();
    m_pFlushCondition    = Condition::Create();
    m_numFeaturesFlushed = 0;

    if (CDKResultSuccess == result)
    {
        result = ProbeChiFeature2Features(GetFeature2LibPath(ChiFeature2VendorType::Default));
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::ProbeChiFeature2Features
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeaturePoolManager::ProbeChiFeature2Features(
    const CHAR* pLibPath)
{
    static const UINT32 MaxChiFeature2LibFiles                                    = 40;
    CHAR                feature2LibFileName[MaxChiFeature2LibFiles][FILENAME_MAX] = { { 0 } };
    CDKResult           result                                                    = CDKResultSuccess;
    UINT32              fileCount                                                 = 0;
    const CHAR*         pD                                                        = NULL;
    INT                 fileIndexMoorea                                           = FILENAME_MAX;
    INT                 fileIndex                                                 = 0;
    std::string         pFileName;

    // find files in the pattern of com.*.feature2.*.so
    fileCount = CdkUtils::GetFilesFromPath(pLibPath,
                                           FILENAME_MAX,
                                           &feature2LibFileName[0][0],
                                           "*",
                                           "feature2",
                                           "*",
                                           "*",
                                           SharedLibraryExtension);

    for (UINT32 fileIdx = 0; fileIdx < fileCount; ++fileIdx)
    {
        if (CHISocIdSM7250 == ExtensionModule::GetInstance()->GetPlatformID() ||
            CHISocIdSM6350 == ExtensionModule::GetInstance()->GetPlatformID() ||
            CHISocIdSM7225 == ExtensionModule::GetInstance()->GetPlatformID())
        {
            pD = CdkUtils::StrStr(&feature2LibFileName[fileIdx][0], "bitra");

            // pD is not NULL if Bitra is present
            if (pD != NULL)
            {
                if ((ExtensionModule::GetInstance()->GetPlatformID() == CHISocIdSM6350) ||
                    (ExtensionModule::GetInstance()->GetPlatformID() == CHISocIdSM7225))
                {
                    pFileName = &feature2LibFileName[fileIdx][0];
                }
                else
                {
                    continue;
                }
            }
            else
            {
                if (ExtensionModule::GetInstance()->GetPlatformID() == CHISocIdSM7250)
                {
                    pFileName = &feature2LibFileName[fileIdx][0];
                }
                else
                {
                    continue;
                }
            }
        }
        else
        {
            pFileName = &feature2LibFileName[fileIdx][0];
        }
        OSLIBRARYHANDLE handle       = NULL;
        CHIFEATURE2OPS  feature2Ops  = { 0 };

        // graph selector doesn't have ChiFeature2OpsEntry
        if (pFileName.find("gs") == std::string::npos)
        {
            handle = ChxUtils::LibMap(pFileName.c_str());

            if (NULL != handle)
            {
                // Load the feature2 entry
                PCHIFEATURE2OPSENTRY funcPChiFeature2OpsEntry =
                    reinterpret_cast<PCHIFEATURE2OPSENTRY>(ChxUtils::LibGetAddr(handle, "ChiFeature2OpsEntry"));

                if (NULL != funcPChiFeature2OpsEntry)
                {
                    funcPChiFeature2OpsEntry(&feature2Ops);

                    if ((sizeof(CHIFEATURE2OPS) == feature2Ops.size) &&
                        (NULL != feature2Ops.pQueryCaps))
                    {
                        ChiFeature2QueryInfo caps = { 0 };
                        feature2Ops.pQueryCaps(m_pConfig, &caps);

                        for (UINT32 i = 0; i < caps.numCaps; ++i)
                        {
                            const CHAR* pCapability = caps.ppCapabilities[i];
                            CHX_LOG_CONFIG("[%d/%d] capability string:%s", i, caps.numCaps, pCapability);

                            if ((NULL != pCapability))
                            {
                                for (auto &it : m_featureDescNameSet)
                                {
                                    if (!CdkUtils::StrCmp(it, pCapability))
                                    {
                                        m_featureNameToOpsMap.insert({ pCapability, feature2Ops });
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    CHX_LOG_ERROR("Fail to find symbol: ChiFeature2OpsEntry");
                }
            }
            else
            {
                CHX_LOG_ERROR("Failed to open lib :%s", pFileName.c_str());
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::DoFeatureCreate
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeature2Base* ChiFeaturePoolManager::DoFeatureCreate(
    const CHAR*                 pFeatureName,
    ChiFeature2CreateInputInfo* pCreateInputInfo)
{
    ChiFeature2Base* pFeature2Instance = NULL;

    for (auto &it : m_featureNameToOpsMap)
    {
        if (!CdkUtils::StrCmp(it.first, pFeatureName))
        {
            auto feature2Ops = it.second;
            if (NULL != feature2Ops.pCreate)
            {
                CHX_LOG("feature:%s, pCreate:%pK", pFeatureName, feature2Ops.pCreate);
                pFeature2Instance =
                    reinterpret_cast<ChiFeature2Base*>(feature2Ops.pCreate(pCreateInputInfo));
            }
        }
    }

    return pFeature2Instance;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::DoStreamNegotiation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeaturePoolManager::DoStreamNegotiation(
    StreamNegotiationInfo*          pNegotiationInfo,
    StreamNegotiationOutput*        pNegotiationOutput)
{
    CDKResult result         = CDKResultSuccess;
    const CHAR* pFeatureName = pNegotiationInfo->pFeatureName;
    auto      feature2Ops    = m_featureNameToOpsMap.find(pFeatureName);

    for (auto &it : m_featureNameToOpsMap)
    {
        if (!CdkUtils::StrCmp(it.first, pFeatureName))
        {
            auto feature2Ops = it.second;
            if (NULL != feature2Ops.pStreamNegotiation)
            {
                CHX_LOG("feature:%s, pStreamNegotiation:%pK", pFeatureName, feature2Ops.pStreamNegotiation);
                result = feature2Ops.pStreamNegotiation(pNegotiationInfo, pNegotiationOutput);
            }
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::ChiFeaturePoolManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeaturePoolManager::ChiFeaturePoolManager()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::GetCameraIdByPortId
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UINT8 ChiFeaturePoolManager::GetCameraIdByPortId(
    std::vector<PortIdCameraIndexMapPair>& rPortIdMap,
    UINT8                                 portId)
{
    UINT8 cameraIdx = InvalidCameraIndex;

    for (PortIdCameraIndexMapPair& portIdCameraIndexMapPair : rPortIdMap)
    {
        if (portIdCameraIndexMapPair.first == portId)
        {
            cameraIdx = portIdCameraIndexMapPair.second;
            break;
        }
    }

    return cameraIdx;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::UpdateInstancePropsInGraphDesc
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeaturePoolManager::UpdateInstancePropsInGraphDesc(
    ChiFeature2GraphDesc*                  pFeatureGraphDesc,
    const ChiFeature2Descriptor*           pFeatureDescriptor,
    ChiFeature2InstanceId*                 pFeatureInstanceId,
    std::vector<PortIdCameraIndexMapPair>& rInputPortMap,
    std::vector<PortIdCameraIndexMapPair>& rOutputPortMap)
{
    UINT numFeatureInstances = pFeatureGraphDesc->numFeatureInstances;

    for (UINT featureIdx = 0; featureIdx < numFeatureInstances; featureIdx++)
    {
        ChiFeature2InstanceDesc* pChiFeature2InstanceDesc = &pFeatureGraphDesc->pFeatureInstances[featureIdx];

        if ((pChiFeature2InstanceDesc->pFeatureDesc->featureId == pFeatureInstanceId->featureId) &&
            (pChiFeature2InstanceDesc->pInstanceProps->instanceId == pFeatureInstanceId->instanceId))
        {
            pChiFeature2InstanceDesc->pInstanceProps->cameraId      = pFeatureInstanceId->cameraId;
            pChiFeature2InstanceDesc->pInstanceProps->instanceFlags = pFeatureInstanceId->flags;

            CHX_LOG_INFO("%s,%s,featureInstance {%d,%d,%d,%x},pFeatureGraphDesc=%p",
                pFeatureGraphDesc->pFeatureGraphName,
                pFeatureDescriptor->pFeatureName,
                pChiFeature2InstanceDesc->pFeatureDesc->featureId,
                pChiFeature2InstanceDesc->pInstanceProps->cameraId,
                pChiFeature2InstanceDesc->pInstanceProps->instanceId,
                pChiFeature2InstanceDesc->pInstanceProps->instanceFlags.value,
                pFeatureGraphDesc);
        }
    }

    for (UINT linkIdx = 0; linkIdx < pFeatureGraphDesc->numInternalLinks; linkIdx++)
    {
        ChiFeature2GraphInternalLinkDesc* pChiFeature2GraphInternalLinkDesc = &pFeatureGraphDesc->pInternalLinks[linkIdx];

        if ((pFeatureInstanceId->featureId == pChiFeature2GraphInternalLinkDesc->srcPortId.featureId) &&
            (pFeatureInstanceId->instanceId == pChiFeature2GraphInternalLinkDesc->srcPortId.instanceProps.instanceId))
        {
            pChiFeature2GraphInternalLinkDesc->srcPortId.instanceProps.instanceFlags = pFeatureInstanceId->flags;

            // Normally the camera id of port is matched to feature instance camera id. But for some multi camera
            // feature, the camera id of port may be from different physical input, at this condition, derived
            // feature will output portid to cameraId map to update the camera id of port
            UINT8 cameraIndex = GetCameraIdByPortId(rInputPortMap,
                pChiFeature2GraphInternalLinkDesc->srcPortId.portId.port);
            if (InvalidCameraIndex == cameraIndex)
            {
                pChiFeature2GraphInternalLinkDesc->srcPortId.instanceProps.cameraId = pFeatureInstanceId->cameraId;
            }
            else
            {
                pChiFeature2GraphInternalLinkDesc->srcPortId.instanceProps.cameraId = cameraIndex;
            }
        }

        if ((pFeatureInstanceId->featureId == pChiFeature2GraphInternalLinkDesc->sinkPortId.featureId) &&
            (pFeatureInstanceId->instanceId == pChiFeature2GraphInternalLinkDesc->sinkPortId.instanceProps.instanceId))
        {
            pChiFeature2GraphInternalLinkDesc->sinkPortId.instanceProps.instanceFlags = pFeatureInstanceId->flags;

            UINT8 cameraIndex = GetCameraIdByPortId(rOutputPortMap,
                pChiFeature2GraphInternalLinkDesc->sinkPortId.portId.port);
            if (InvalidCameraIndex == cameraIndex)
            {
                pChiFeature2GraphInternalLinkDesc->sinkPortId.instanceProps.cameraId = pFeatureInstanceId->cameraId;
            }
            else
            {
                pChiFeature2GraphInternalLinkDesc->sinkPortId.instanceProps.cameraId = cameraIndex;
            }
        }
    }

    for (UINT linkIdx = 0; linkIdx < pFeatureGraphDesc->numExtSinkLinks; linkIdx++)
    {
        ChiFeature2GraphExtSinkLinkDesc* pChiFeature2GraphExtSinkLinkDesc = &pFeatureGraphDesc->pExtSinkLinks[linkIdx];

        if ((pFeatureInstanceId->featureId == pChiFeature2GraphExtSinkLinkDesc->portId.featureId) &&
            (pFeatureInstanceId->instanceId == pChiFeature2GraphExtSinkLinkDesc->portId.instanceProps.instanceId))
        {
            pChiFeature2GraphExtSinkLinkDesc->portId.instanceProps.instanceFlags = pFeatureInstanceId->flags;

            UINT8 cameraIndex = GetCameraIdByPortId(rOutputPortMap, pChiFeature2GraphExtSinkLinkDesc->portId.portId.port);
            if (InvalidCameraIndex == cameraIndex)
            {
                pChiFeature2GraphExtSinkLinkDesc->portId.instanceProps.cameraId = pFeatureInstanceId->cameraId;
            }
            else
            {
                pChiFeature2GraphExtSinkLinkDesc->portId.instanceProps.cameraId = cameraIndex;
            }
        }
    }

    for (UINT linkIdx = 0; linkIdx < pFeatureGraphDesc->numExtSrcLinks; linkIdx++)
    {
        ChiFeature2GraphExtSrcLinkDesc* pChiFeature2GraphExtSrcLinkDesc = &pFeatureGraphDesc->pExtSrcLinks[linkIdx];

        if ((pFeatureInstanceId->featureId == pChiFeature2GraphExtSrcLinkDesc->portId.featureId) &&
            (pFeatureInstanceId->instanceId == pChiFeature2GraphExtSrcLinkDesc->portId.instanceProps.instanceId))
        {
            pChiFeature2GraphExtSrcLinkDesc->portId.instanceProps.instanceFlags = pFeatureInstanceId->flags;

            UINT8 cameraIndex = GetCameraIdByPortId(rInputPortMap, pChiFeature2GraphExtSrcLinkDesc->portId.portId.port);
            if (InvalidCameraIndex == cameraIndex)
            {
                pChiFeature2GraphExtSrcLinkDesc->portId.instanceProps.cameraId = pFeatureInstanceId->cameraId;
            }
            else
            {
                pChiFeature2GraphExtSrcLinkDesc->portId.instanceProps.cameraId = cameraIndex;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::CreateFeatureInstance
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeaturePoolManager::CreateFeatureInstance(
    FeaturePoolInputData*          pFeaturePoolInputData,
    std::vector<ChiFeature2Base*>& rpEnabledFeatures)
{
    CDKResult             result            = CDKResultSuccess;
    ChiFeature2GraphDesc* pFeatureGraphDesc = pFeaturePoolInputData->pFeatureGraphDesc;

    m_pEnabledFeatures.clear();

    if (NULL != pFeatureGraphDesc)
    {
        UINT   numFeatureInstances = pFeatureGraphDesc->numFeatureInstances;
        UINT32 physicalCameraId    = 0;

        CHX_LOG_INFO(":%s, numFeatureInstances:%d, logicalCameraId:%d",
            pFeatureGraphDesc->pFeatureGraphName,
            numFeatureInstances,
            pFeaturePoolInputData->pCameraInfo->cameraId);

        for (UINT j = 0; j < numFeatureInstances; j++)
        {
            ChiFeature2InstanceId           feature2InstanceId         = { 0 };
            ChiFeature2CreateInputInfo      feature2CreateInputInfo    = { 0 };
            ChiFeature2InstanceProps*       pInstanceProps             = pFeatureGraphDesc->pFeatureInstances[j].pInstanceProps;

            if (pInstanceProps->cameraId >= pFeaturePoolInputData->pCameraInfo->numPhysicalCameras)
            {
                continue;
            }

            UINT featureId           = pFeatureGraphDesc->pFeatureInstances[j].pFeatureDesc->featureId;
            const CHAR* pFeatureName = pFeatureGraphDesc->pFeatureInstances[j].pFeatureDesc->pFeatureName;

            // For snapshot YUV callback mode, no Bokeh or fusion featureinstance need be created
            UINT32 inputOutputType = static_cast<FeatureGraphManagerConfig*>(m_pConfig)->inputOutputType;
            if (static_cast<UINT32>(InputOutputType::YUV_OUT) == inputOutputType)
            {
                if ((static_cast<UINT32>(ChiFeature2Type::BOKEH) == featureId) ||
                    (static_cast<UINT32>(ChiFeature2Type::FUSION) == featureId))
                {
                    CHX_LOG_INFO("Snapshot YUV callback mode, no fusion/bokeh Feature instance will be created");
                    continue;
                }
            }

            m_pStreams.clear();

            feature2CreateInputInfo.pCameraInfo                   = pFeaturePoolInputData->pCameraInfo;
            feature2CreateInputInfo.pFeatureDescriptor            = pFeatureGraphDesc->pFeatureInstances[j].pFeatureDesc;
            feature2CreateInputInfo.pClientCallbacks              = pFeaturePoolInputData->pFeatureCallbacks;
            feature2CreateInputInfo.pUsecaseDescriptor            = g_pUsecaseZSL;
            feature2CreateInputInfo.pMetadataManager              = pFeaturePoolInputData->pMetadataManager;
            feature2CreateInputInfo.pInstanceProps                = pInstanceProps;
            feature2CreateInputInfo.pThreadManager                = pFeaturePoolInputData->pThreadManager;

            feature2CreateInputInfo.pFeatureGraphManagerCallbacks =
                pFeaturePoolInputData->pFeatureGraphManagerCallbacks;
            feature2CreateInputInfo.bFrameworkHEICSnapshot        =
                UsecaseSelector::HasHeicSnapshotStream(pFeaturePoolInputData->pCameraStreamConfig);
            feature2CreateInputInfo.featureFlags                  = pFeaturePoolInputData->featureFlags;

            feature2InstanceId.featureId  = featureId;
            feature2InstanceId.instanceId = pFeatureGraphDesc->pFeatureInstances[j].pInstanceProps->instanceId;
            feature2InstanceId.cameraId   = pInstanceProps->cameraId;
            feature2InstanceId.flags      = pFeaturePoolInputData->featureFlags;
            std::vector<PortIdCameraIndexMapPair>  inputPortMap;
            std::vector<PortIdCameraIndexMapPair>  outputPortMap;

            if (m_pEnabledFeaturesMap.find(feature2InstanceId) == m_pEnabledFeaturesMap.end())
            {
                StreamNegotiationInfo   negotiationInput  = { 0 };
                StreamNegotiationOutput negotiationOutput = { 0 };

                negotiationInput.pFeatureInstanceId = &feature2InstanceId;
                negotiationInput.pFwkStreamConfig   = pFeaturePoolInputData->pCameraStreamConfig;
                negotiationInput.pLogicalCameraInfo = pFeaturePoolInputData->pCameraInfo;
                negotiationInput.pFeatureName       = pFeatureName;
                negotiationInput.pInstanceProps     = pInstanceProps;

                m_desiredStreamConfig                  = { 0 };
                negotiationOutput.pDesiredStreamConfig = &m_desiredStreamConfig;
                negotiationOutput.pStreams             = &m_pStreams;
                negotiationOutput.pOwnedStreams        = &m_pFeatureStreams;
                negotiationOutput.pInputPortMap        = &inputPortMap;
                negotiationOutput.pOutputPortMap       = &outputPortMap;

                DoStreamNegotiation(&negotiationInput, &negotiationOutput);

                m_frameworkVideoStream                        = negotiationOutput.isFrameworkVideoStream;
                feature2CreateInputInfo.pStreamConfig         = &m_desiredStreamConfig;
                feature2CreateInputInfo.pOwnedStreams         = &m_pFeatureStreams;
                feature2CreateInputInfo.bFrameworkVideoStream = m_frameworkVideoStream;
                feature2CreateInputInfo.bDisableZoomCrop      = negotiationOutput.disableZoomCrop;

                UpdateInstancePropsInGraphDesc(pFeatureGraphDesc,
                                              feature2CreateInputInfo.pFeatureDescriptor,
                                              &feature2InstanceId,
                                              inputPortMap,
                                              outputPortMap);

                // check if resource manager is required for realtime features
                if (static_cast<UINT32>(ChiFeature2Type::REALTIME) == featureId)
                {
                    camera3_stream_configuration_t streamConfig = { 0 };
                    streamConfig.num_streams    = pFeaturePoolInputData->pCameraStreamConfig->numStreams;
                    streamConfig.streams        =
                        reinterpret_cast<camera3_stream_t**>(pFeaturePoolInputData->pCameraStreamConfig->pChiStreams);
                    streamConfig.operation_mode = pFeaturePoolInputData->pCameraStreamConfig->operationMode;

                    if ((TRUE == UsecaseSelector::IsQuadCFASensor(pFeaturePoolInputData->pCameraInfo, NULL)) &&
                        (TRUE == UsecaseSelector::QuadCFAMatchingUsecase(pFeaturePoolInputData->pCameraInfo, &streamConfig)))
                    {

                        CHX_LOG_INFO("Using resource manager for qcfa usecase");
                        feature2CreateInputInfo.bEnableResManager = TRUE;
                        m_qcfaUsecase = TRUE;

                    }
                }

                UINT32 featureId    = feature2InstanceId.featureId;
                auto&  rFeatureName = *reinterpret_cast<const CHAR(*)[MaxStringLength256]>(
                                          pFeatureGraphDesc->pFeatureInstances[j].pFeatureDesc->pFeatureName);
                BINARY_LOG(LogEvent::FT2_Pool_CreateInstance, featureId, rFeatureName);
                CHX_LOG_CONFIG("%s, {featureId, instanceId, cameraId, flags}: {%d, %d, %d, %d}",
                    pFeatureGraphDesc->pFeatureInstances[j].pFeatureDesc->pFeatureName,
                    feature2InstanceId.featureId,
                    feature2InstanceId.instanceId,
                    feature2InstanceId.cameraId,
                    feature2InstanceId.flags.value);

                ChiFeature2Base* pChiFeature2Base = DoFeatureCreate(pFeatureName, &feature2CreateInputInfo);

                if (NULL != pChiFeature2Base)
                {
                    m_pFeatureList.push_back(pChiFeature2Base);
                    m_pEnabledFeatures.push_back(pChiFeature2Base);
                    m_pEnabledFeaturesMap.insert({ feature2InstanceId, pChiFeature2Base });
                }
                else
                {
                    m_pEnabledFeaturesMap.insert({ feature2InstanceId, pChiFeature2Base });
                    CHX_LOG_ERROR("Fail to create feature instance for %s",
                        feature2CreateInputInfo.pFeatureDescriptor->pFeatureName);
                    if (FALSE == m_pFeatureStreams.empty())
                    {
                        for (auto& pStream : m_pFeatureStreams)
                        {
                            if (NULL != pStream)
                            {
                                ChxUtils::Free(pStream);
                                pStream = NULL;
                            }
                        }
                    }
                    result = CDKResultEFailed;
                }
            }
            else
            {
                auto featureMap  = m_pEnabledFeaturesMap.find(feature2InstanceId);

                StreamNegotiationInfo   negotiationInput  = { 0 };
                StreamNegotiationOutput negotiationOutput = { 0 };

                negotiationInput.pFeatureInstanceId = &feature2InstanceId;
                negotiationInput.pFwkStreamConfig   = pFeaturePoolInputData->pCameraStreamConfig;
                negotiationInput.pLogicalCameraInfo = pFeaturePoolInputData->pCameraInfo;
                negotiationInput.pFeatureName       = pFeatureName;
                negotiationInput.pInstanceProps     = pInstanceProps;

                m_desiredStreamConfig                  = { 0 };
                negotiationOutput.pDesiredStreamConfig = &m_desiredStreamConfig;
                negotiationOutput.pStreams             = &m_pStreams;
                negotiationOutput.pOwnedStreams        = &m_pFeatureStreams;
                negotiationOutput.pInputPortMap        = &inputPortMap;
                negotiationOutput.pOutputPortMap       = &outputPortMap;

                DoStreamNegotiation(&negotiationInput, &negotiationOutput);

                UpdateInstancePropsInGraphDesc(pFeatureGraphDesc,
                                              feature2CreateInputInfo.pFeatureDescriptor,
                                              &feature2InstanceId,
                                              inputPortMap,
                                              outputPortMap);

                CHX_LOG_CONFIG("Update port: pFeatureName:%s, {featureId, instanceId, cameraId, flag}: {%d, %d, %d, %d}",
                    pFeatureGraphDesc->pFeatureInstances[j].pFeatureDesc->pFeatureName,
                    feature2InstanceId.featureId,
                    feature2InstanceId.instanceId,
                    feature2InstanceId.cameraId,
                    feature2InstanceId.flags.value);

                m_pEnabledFeatures.push_back(featureMap->second);

                if (FALSE == m_pFeatureStreams.empty())
                {
                    for (auto& pStream : m_pFeatureStreams)
                    {
                        if (NULL != pStream)
                        {
                            ChxUtils::Free(pStream);
                            pStream = NULL;
                        }
                    }
                }
            }

            // The feature now owns the streams in m_pFeatureStreams.
            m_pFeatureStreams.clear();

            if (CDKResultSuccess != result)
            {
                break;
            }
        }

        rpEnabledFeatures = m_pEnabledFeatures;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::::SelectFeaturesForGraph
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeaturePoolManager::SelectFeaturesForGraph(
    ChiFeature2GraphDesc*                           pFeatureGraphDescForRequest,
    std::vector<ChiFeature2InstanceRequestInfo>&    rEnabledFeatureInstanceReqInfo)
{
    UINT                  numFeatureInstances = pFeatureGraphDescForRequest->numFeatureInstances;
    ChiFeature2InstanceId feature2InstanceId  = {};

    for (UINT featureIndex = 0; featureIndex < numFeatureInstances; featureIndex++)
    {
        const ChiFeature2InstanceDesc* pInstanceDesc = &(pFeatureGraphDescForRequest->pFeatureInstances[featureIndex]);

        feature2InstanceId.featureId  = pInstanceDesc->pFeatureDesc->featureId;
        feature2InstanceId.instanceId = pInstanceDesc->pInstanceProps->instanceId;
        feature2InstanceId.cameraId   = pInstanceDesc->pInstanceProps->cameraId;
        feature2InstanceId.flags      = pInstanceDesc->pInstanceProps->instanceFlags;

        auto featureMap = m_pEnabledFeaturesMap.find(feature2InstanceId);
        if (featureMap != m_pEnabledFeaturesMap.end())
        {
            ChiFeature2Base* pChiFeature2Base = featureMap->second;
            if (NULL != pChiFeature2Base)
            {
                ChiFeature2InstanceRequestInfo featureInstanceRequestInfo = {0};
                featureInstanceRequestInfo.pFeatureBase = pChiFeature2Base;
                rEnabledFeatureInstanceReqInfo.push_back(featureInstanceRequestInfo);

            }
        }
        else
        {
            CHX_LOG_ERROR("Unable to find features(numFeatureInstances:%d):featureId, instanceId, cameraId flags {%d,%d,%d,%d}",
                numFeatureInstances,
                feature2InstanceId.featureId,
                feature2InstanceId.instanceId,
                feature2InstanceId.cameraId,
                feature2InstanceId.flags.value);
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::ReleaseResource
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeaturePoolManager::ReleaseResource(
    ChiFeature2GraphDesc* pChiFeature2GraphDesc)
{
    CDK_UNUSED_PARAM(pChiFeature2GraphDesc);
    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeaturePoolManager* ChiFeaturePoolManager::Create(
    VOID*                   pConfig,
    std::set<const CHAR*>&  rFeatureDescSet)
{
    ChiFeaturePoolManager* pChiFeaturePoolManager = CHX_NEW ChiFeaturePoolManager;

    pChiFeaturePoolManager->m_featureDescNameSet = rFeatureDescSet;

    pChiFeaturePoolManager->Initialize(pConfig);

    return pChiFeaturePoolManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeaturePoolManager::Flush(
    BOOL isSynchronousFlush)
{
    CDKResult result        = CDKResultSuccess;
    CDKResult flushResult   = CDKResultSuccess;

    for (UINT featureIdx = 0; featureIdx < m_pFeatureList.size(); featureIdx++)
    {
        if (NULL != m_pFeatureList[featureIdx])
        {
            result = m_pFeatureList[featureIdx]->Flush(isSynchronousFlush);
            if (CDKResultSuccess != result)
            {
                flushResult = result;
            }
        }
    }

    if (FALSE == isSynchronousFlush)
    {
        m_pFlushMutex->Lock();

        UINT32 numFeaturesFlushed;
        m_pFlushCountMutex->Lock();
        numFeaturesFlushed = m_numFeaturesFlushed;
        m_pFlushCountMutex->Unlock();

        if (numFeaturesFlushed != m_pFeatureList.size())
        {
            result = m_pFlushCondition->TimedWait(m_pFlushMutex->GetNativeHandle(), 10000);
        }

        if (CDKResultSuccess != result)
        {
            CHX_LOG_ERROR("Flush TIMEOUT!!");
        }

        m_numFeaturesFlushed = 0;
        m_pFlushMutex->Unlock();
    }

    return flushResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::SignalFlush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult ChiFeaturePoolManager::SignalFlush(
    UINT32  featureId)
{
    UINT32 numFeaturesFlushed;
    m_pFlushCountMutex->Lock();
    numFeaturesFlushed = ++m_numFeaturesFlushed;
    m_pFlushCountMutex->Unlock();

    if (numFeaturesFlushed == m_pFeatureList.size())
    {
        CHX_LOG_INFO("Current FeatureId: %u, %u out of %u features flushed, Signaling",
                     featureId,
                     numFeaturesFlushed,
                     static_cast<UINT32>(m_pFeatureList.size()));
        m_pFlushCondition->Signal();
    }
    else
    {
        CHX_LOG_INFO("Current FeatureId: %u, %u out of %u features flushed",
                     featureId,
                     numFeaturesFlushed,
                     static_cast<UINT32>(m_pFeatureList.size()));
    }

    return CDKResultSuccess;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ChiFeaturePoolManager::NotifyGraphManagerDestroyInProgress
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeaturePoolManager::NotifyGraphManagerDestroyInProgress()
{
    for (UINT featureIdx = 0; featureIdx < m_pFeatureList.size(); featureIdx++)
    {
        if (NULL != m_pFeatureList[featureIdx])
        {
            m_pFeatureList[featureIdx]->NotifyGraphManagerDestroyInProgress();
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VOID ChiFeaturePoolManager::Destroy()
{
    if (TRUE == m_qcfaUsecase)
    {
        CHX_LOG_INFO("Deactivate Features before destroy");
        for (UINT featureIdx = 0; featureIdx < m_pFeatureList.size(); featureIdx++)
        {
            if (NULL != m_pFeatureList[featureIdx])
            {
                m_pFeatureList[featureIdx]->Deactivate();
            }
        }
    }

    for (UINT featureIdx = 0; featureIdx < m_pFeatureList.size(); featureIdx++)
    {
        if (NULL != m_pFeatureList[featureIdx])
        {
            m_pFeatureList[featureIdx]->Destroy();
            m_pFeatureList[featureIdx] = NULL;
        }
    }

    m_pEnabledFeatures.clear();
    m_pFeatureList.clear();
    m_pEnabledFeaturesMap.clear();

    if (NULL != m_pFlushMutex)
    {
        m_pFlushMutex->Destroy();
        m_pFlushMutex = NULL;
    }

    if (NULL != m_pFlushCountMutex)
    {
        m_pFlushCountMutex->Destroy();
        m_pFlushCountMutex = NULL;
    }

    if (NULL != m_pFlushCondition)
    {
        m_pFlushCondition->Destroy();
        m_pFlushCondition = NULL;
    }

    CHX_DELETE this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ChiFeaturePoolManager::~ChiFeaturePoolManager
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ChiFeaturePoolManager::~ChiFeaturePoolManager()
{

}
